#include "socket.h"
#include "../common/fd_manager.h"
#include "../common/macro.h"

#include <limits.h>
#include <sstream>
#include <spdlog/spdlog.h>

namespace amot {

static auto logger = spdlog::get("system");

Socket::ptr Socket::CreateTCP(amot::Address::ptr address) {
	Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
	return sock;
}

Socket::ptr Socket::CreateUDP(amot::Address::ptr address) {
	Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
	sock->newSock();
	sock->m_isConnected = true;
	return sock;
}

Socket::ptr Socket::CreateTCPSocket() {
	Socket::ptr sock(new Socket(IPv4, TCP, 0));
	return sock;
}

Socket::ptr Socket::CreateUDPSocket() {
	Socket::ptr sock(new Socket(IPv4, UDP, 0));
	sock->newSock();
	sock->m_isConnected = true;
	return sock;
}

Socket::ptr Socket::CreateTCPSocket6() {
	Socket::ptr sock(new Socket(IPv6, TCP, 0));
	return sock;
}

Socket::ptr Socket::CreateUDPSocket6() {
	Socket::ptr sock(new Socket(IPv6, UDP, 0));
	sock->newSock();
	sock->m_isConnected = true;
	return sock;
}

Socket::ptr Socket::CreateUnixTCPSocket() {
	Socket::ptr sock(new Socket(UNIX, TCP, 0));
	return sock;
}

Socket::ptr Socket::CreateUnixUDPSocket() {
	Socket::ptr sock(new Socket(UNIX, UDP, 0));
	sock->newSock();
	sock->m_isConnected = true;
	return sock;
}

Socket::Socket(int family, int type, int protocol)
	:m_sock(-1)
	,m_family(family)
	,m_type(type)
	,m_protocol(protocol)
	,m_isConnected(false) 
{ }

Socket::~Socket() {
	close();
}

int64_t Socket::getSendTimeout() {
	// 获取文件句柄，保证为socket句柄
	FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
	if(ctx) {
		return ctx->getTimeout(SO_SNDTIMEO);
	}
	return -1;
}

void Socket::setSendTimeout(int64_t v) {
	struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
	setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout() {
	FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
	if(ctx) {
		return ctx->getTimeout(SO_RCVTIMEO);
	}
	return -1;
}

void Socket::setRecvTimeout(int64_t v) {
	struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
	setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void* result, socklen_t* len) {
	int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
	if(rt) {
		logger->debug("getOption sock={} level={} option={} errno={} errstr={}", 
			m_sock, level, option, errno, strerror(errno));
		return false;
	}
	return true;
}

bool Socket::setOption(int level, int option, const void* result, socklen_t len) {
	if(setsockopt(m_sock, level, option, result, (socklen_t)len)) {
		logger->debug("setOption sock={} level={} option={} errno={} errstr={}", 
			m_sock, level, option, errno, strerror(errno));
		return false;
	}
	return true;
}

Socket::ptr Socket::accept() {
	Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
	int newsock = ::accept(m_sock, nullptr, nullptr);
	if(newsock == -1) {
		logger->debug("accept({}) errno={} errstr={}", m_sock, errno, strerror(errno));
		return nullptr;
	}
	if(sock->init(newsock)) {
		return sock;
	}
	return nullptr;
}

bool Socket::bind(const Address::ptr addr) {
	// 判断本socket是否可用
	if(!isValid()) {
		newSock();
		if(AMOT_UNLIKELY(!isValid())) {
			return false;
		}
	}

	// 判断协议是否相同
	if(AMOT_UNLIKELY(addr->getFamily() != m_family)) {
		logger->error("bind sock.family({}) addr.family({}) not equal, addr={}", 
			m_family, addr->getFamily(), addr->toString());
		return false;
	}

	if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
		logger->error("bind error errno={} strerr={}", errno, strerror(errno));
		return false;
	}
	return true;
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
	m_remoteAddress = addr;
	// 判断本socket是否可用
	if(!isValid()) {
		newSock();
		if(AMOT_UNLIKELY(!isValid())) {
			return false;
		}
	}

	// 判断协议是否相同
	if(AMOT_UNLIKELY(addr->getFamily() != m_family)) {
		logger->error("bind sock.family({}) addr.family({}) not equal, addr={}", 
			m_family, addr->getFamily(), addr->toString());
		return false;
	}

	if(timeout_ms == (uint64_t)-1) {
		if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
			logger->error("sock={} connect({}) errno={} strerr={}", 
				m_sock, addr->toString(), errno, strerror(errno));
			close();
			return false;
		}
	} else {
		// if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
		// 	spdlog::error("sock={} connect({}) errno={} strerr={}", 
		// 		m_sock, addr->toString(), errno, strerror(errno));
		// 	close();
		// 	return false;
		// }
	}
	m_isConnected = true;
	// 初始化
	getRemoteAddress();
	getLocalAddress();
	return true;
}

bool Socket::reconnect(uint64_t timeout_ms) {
	if(!m_remoteAddress) {
		logger->error("remote address is null");
		return false;
	}
	m_localAddress.reset();
	return connect(m_remoteAddress, timeout_ms);
}

bool Socket::listen(int backlog) {
	// 判断本socket是否可用
	if(!isValid()) {
		logger->error("listen error sock=-1");
		return false;
	}

	if(::listen(m_sock, backlog)) {
		logger->error("listen error sock={} errno={} errstr={}", 
			m_sock, errno, strerror(errno));
		return false;
	}
	return true;
}

bool Socket::close() {
	if(!m_isConnected && m_sock == -1) {
		return true;
	}
	m_isConnected = false;
	if(m_sock != -1) {
		::close(m_sock);
		m_sock = -1;
	}
	return false;
}

int Socket::send(const void* buffer, size_t length, int flags) {
	if(isConnected()) {
		return ::send(m_sock, buffer, length, flags);
	}
	return -1;
}

int Socket::send(const iovec* buffers, size_t length, int flags) {
	if(isConnected()) {
		msghdr msg;
		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = (iovec *)buffers;
		msg.msg_iovlen = length;
		return ::sendmsg(m_sock, &msg, flags);
	}
	return -1;
}

int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
	if(isConnected()) {
		return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
	}
	return -1;
}

int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags) {
	if(isConnected()) {
		msghdr msg;
		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = (iovec *)buffers;
		msg.msg_iovlen = length;
		msg.msg_name = to->getAddr();
		msg.msg_namelen = to->getAddrLen();
		return ::sendmsg(m_sock, &msg, flags);
	}
	return -1;
}

int Socket::recv(void* buffer, size_t length, int flags) {
	if(isConnected()) {
		return ::recv(m_sock, buffer, length, flags);
	}
	return -1;
}

int Socket::recv(iovec* buffers, size_t length, int flags) {
	if(isConnected()) {
		msghdr msg;
		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = buffers;
		msg.msg_iovlen = length;
		return ::recvmsg(m_sock, &msg, flags);
	}
	return -1;
}

int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
	if(isConnected()) {
		socklen_t len = from->getAddrLen();
		return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
	}
	return -1;
}

int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags) {
	if(isConnected()) {
		msghdr msg;
		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = buffers;
		msg.msg_iovlen = length;
		msg.msg_name = from->getAddr();
		msg.msg_namelen = from->getAddrLen();
		return recvmsg(m_sock, &msg, flags);
	}
	return -1;
}

Address::ptr Socket::getRemoteAddress() {
	if(m_remoteAddress) {
		return m_remoteAddress;
	}
	return nullptr;
}

Address::ptr Socket::getLocalAddress() {
	if(m_localAddress) {
		return m_localAddress;
	}
	return nullptr;
}

bool Socket::isValid() const {
	return m_sock != -1;
}

int Socket::getError() {
	int error = 0;
	socklen_t len = sizeof(error);
	// ? 为何需要使用此函数获取错误信息，直接返回错误信息不行吗？
	if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len))
	{
		error = errno;
	}
	return error;
}

std::ostream& Socket::dump(std::ostream& os) const {
	os << "[Socket sock=" << m_sock << " is_connected=" << m_isConnected
	   << " family=" << m_family << " type=" << m_type << " protocol=" << m_protocol;
	
	if(m_localAddress) {
		os << " local_address=" << m_localAddress;
	}

	if(m_remoteAddress) {
		os << " remote_address=" << m_remoteAddress;
	}
	os << "]";
	return os;
}

std::string Socket::toString() const {
	std::stringstream ss;
	dump(ss);
	return ss.str();
}

// bool Socket::cancelRead()
// {
// 	return IOmanager::GetThis()->cancelEvent(m_sock, IOmanager::READ);
// }

// bool Socket::cancelWrite()
// {
// 	return IOmanager::GetThis()->cancelEvent(m_sock, IOmanager::WRITE);
// }

// bool Socket::cancelAccept()
// {
// 	return IOmanager::GetThis()->cancelEvent(m_sock, IOmanager::READ);
// }

// bool Socket::cancelAll()
// {
// 	return IOmanager::GetThis()->cancelAll(m_sock);
// }

void Socket::initSock() {
	// 若不进行下述操作，socket地址不会正常释放，在退出程序后短时间内再次启动，会出现地址已使用的错误
    int val = 1;
	// socket关闭后，不会立即关闭端口和地址，而是会等待一段时间，此时再次使用这些地址便会报错
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if(m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }

}

void Socket::newSock() {
	m_sock = socket(m_family, m_type, m_protocol);
	if(AMOT_UNLIKELY(m_sock != -1)) {
		initSock();
	}
	else {
		logger->error("sock={} errno={} strerr={}", m_sock, errno, strerror(errno));
	}
}

bool Socket::init(int sock) {
	// 使用文件句柄类确保为sock句柄
	FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock, true);
	// 判断是否获取句柄成功、是否为sock句柄、是否关闭
	if(ctx && ctx->isSocket() && !ctx->isClose()) {
		// 初始化
		m_sock = sock;
		m_isConnected = true;
		initSock();
		// 此处用于初始化这两个变量
		getLocalAddress();
		getRemoteAddress();
		return true;
	}
	return false;
}
}