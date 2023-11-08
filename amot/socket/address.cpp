#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>
#include <sstream>
#include <spdlog/spdlog.h>

#include "address.h"
#include "../common/endian.h"

namespace amot {

static auto logger = spdlog::get("system");

/**
 * @brief 创建掩码
 * 
 * @tparam T 类型
 * @param bits 位数
 * @return T 
 */
template<class T>
static T CreateMask(uint32_t bits) {
	return (1 << (sizeof(T) * 8 - bits)) - 1;
}

/**
 * @brief 计算二进制位中1的个数
 * 
 * @tparam T 模版类型
 * @param value 输入的数
 * @return uint32_t 1的个数
 */
template<class T>
static uint32_t CountBytes(T value) {
	uint32_t result = 0;
	for (; value; ++result) {
		value &= value - 1;
	}
	return result;
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen) {
	if (addr == nullptr) {
		return nullptr;
	}

	Address::ptr result;
	switch(addr->sa_family) {
		case AF_INET:
			result.reset(new IPv4Address(*(const sockaddr_in*)addr));
			break;
		case AF_INET6:
			result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
		default:
			break;
		}
	return result;
}

bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
					int family, int type, int protocol) {
	addrinfo hints, *results, *next;
	hints.ai_flags = 0;
	hints.ai_family = family;
	hints.ai_socktype = type;
	hints.ai_protocol = protocol;
	hints.ai_addrlen = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	std::string node;
	const char *service = NULL;

	// 检查 ipv6 address service host www.sylar.top[:80]
	if(!host.empty() && host[0] == '[') {
		// 在host中寻找 ']'
		const char *endipv6 = (const char *)memchr(host.c_str() + 1, ']', host.size() - 1);
		spdlog::info(endipv6);
		if (endipv6) {
			if(*(endipv6 + 1) == ':') {
				service = endipv6 + 2;
			}
			node = host.substr(1, endipv6 - host.c_str() - 1);
		}
	}

	// 检查 node service www.sylar.top[:80]
	if(node.empty()) {
		// 找到 ':' 的指针地址
		service = (const char*)memchr(host.c_str(), ':', host.size());
		// SERVER_LOG_INFO(g_logger) << service;
		if (service) {
			// 如果找不到第二个冒号
			if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
				// 获取冒号前的域名
				node = host.substr(0, service - host.c_str());
				++service;
			}
		}
	}
	// SERVER_LOG_INFO(g_logger) << "node -->" << node;
	if (node.empty()) {
		node = host;
	}
	// 将域名转换为IP地址
	int error = getaddrinfo(node.c_str(), service, &hints, &results);
	if(error) {
		logger->info("Address::Lookup getaddress({}, {}, {}) err={} errstr={}", 
			host, family, type, error, gai_strerror(error));
		return false;
	}

	next = results;
	while(next) {
		result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
		next = next->ai_next;
	}

	freeaddrinfo(results);
	return !result.empty();
}

Address::ptr Address::LookupAny(const std::string& host, 
								int family, int type, int protocol) {
	std::vector<Address::ptr> result;
	if(Lookup(result, host, family, type, protocol)) {
		return result[0];
	}
	return nullptr;
}

IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host,
			int family, int type, int protocol) {
	std::vector<Address::ptr> result;
	if(Lookup(result, host, family, type, protocol)) {
		for(auto& i : result) {
			IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
			if(v) {
				return v;
			}
		}
	}
	return nullptr;
}

bool Address::GetInterfaceAddresses(std::multimap<std::string
						,std::pair<Address::ptr, uint32_t> >& result,
						int family) {
	struct ifaddrs *next, *results;
	if(getifaddrs(&results) != 0) {
		logger->info("Address::GetInterfaceAddresses getifaddrs err={} errstr={}",
			errno, strerror(errno));
		return false;
	}

	try {
		for (next = results; next; next = next->ifa_next)
		{
			Address::ptr addr;
			uint32_t prefix_len = ~0u;
			if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
				continue;
			}
			switch (next->ifa_addr->sa_family) {
				case AF_INET: {
						addr = Create(next->ifa_addr, sizeof(sockaddr_in));
						uint32_t netmask = ((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
						prefix_len = CountBytes(netmask);
					}
					break;
				case AF_INET6: {
						addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
						in6_addr& netmask = ((sockaddr_in6 *)next->ifa_netmask)->sin6_addr;
						prefix_len = 0;
						for (int i = 0; i < 16; ++i) {
							prefix_len += CountBytes(netmask.s6_addr[i]);
						}
					}
					break;
				default:
					break;
			}
			if(addr) {
				result.insert(std::make_pair(next->ifa_name,
												std::make_pair(addr, prefix_len)));
			}
		}
	} catch(...) {
		logger->info("Addres::GetInterfaceAddresses exception");
		freeifaddrs(results);
		return false;
	}
	freeifaddrs(results);
	return !result.empty();
}

bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> > &result
									,const std::string& iface, int family) {
	if(iface.empty() || iface == "*") {
		if(family == AF_INET || family == AF_UNSPEC) {
			result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
		}
		if(family == AF_INET6 || family == AF_UNSPEC) {
			result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
		}
		return true;
	}

	std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results;

	if(!GetInterfaceAddresses(results, family)) {
		return false;
	}
	auto its = results.equal_range(iface);
	for (; its.first != its.second; ++its.first) {
		result.push_back(its.first->second);
	}
	return !result.empty();
}

int Address::getFamily() const {
	return getAddr()->sa_family;
}

std::string Address::toString() const {
	std::stringstream ss;
	insert(ss);
	return ss.str();
}

bool Address::operator<(const Address& rhs) const {
	socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
	int result = memcmp(getAddr(), rhs.getAddr(), minlen);
	if(result < 0) {
		return true;
	} else if(result > 0) {
		return false;
	} else if(getAddrLen() < rhs.getAddrLen()) {
		return true;
	}
	return false;
}

bool Address::operator==(const Address& rhs) const {
	return getAddrLen() == rhs.getAddrLen() 
		&& memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const {
	return !(*this == rhs);
}

IPAddress::ptr IPAddress::Create(const char* address, uint16_t port) {
	addrinfo hints, *results;
	memset(&hints, 0, sizeof(addrinfo));

	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_UNSPEC;

	int error = getaddrinfo(address, NULL, &hints, &results);
	if(error) {
		logger->info("IPAddress::Create({}, {}) error={} errno={} errstr={}", 
			address, port, error, errno, strerror(errno));
	}

	try	{
		IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
			Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen)
		);
		if(result) {
			result->setPort(port);
		}
		freeaddrinfo(results);
		return result;
	} catch(...) {
		freeaddrinfo(results);
		return nullptr;
	}
}

IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port) {
	IPv4Address::ptr rt(new IPv4Address);
	rt->m_addr.sin_port = byteswapOnLittleEndian(port);
	int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
	if(result <= 0) {
		logger->debug("IPAddress::Create({}, {}) error={} errno={} errstr={}", 
			address, port, result, errno, strerror(errno));
		return nullptr;
	}
	return rt;
}

IPv4Address::IPv4Address(const sockaddr_in& address) {
	m_addr = address;
}

IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
	memset(&m_addr, 0, sizeof(m_addr));
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = byteswapOnLittleEndian(port);
	m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

sockaddr* IPv4Address::getAddr() {
	return (sockaddr*)&m_addr;
}

const sockaddr* IPv4Address::getAddr() const {
	return (sockaddr*)&m_addr;
}

socklen_t IPv4Address::getAddrLen() const {
	return sizeof(m_addr);
}

std::ostream& IPv4Address::insert(std::ostream& os) const {
	uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
	os << ((addr >> 24) & 0xff) << "."
		<< ((addr >> 16) & 0xff) << "."
		<< ((addr >> 8) & 0xff) << "."
		<< (addr & 0xff);
	os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
	return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
	if(prefix_len > 32) {
		return nullptr;
	}

	sockaddr_in baddr(m_addr);
	baddr.sin_addr.s_addr |= byteswapOnLittleEndian(
		CreateMask<uint32_t>(prefix_len));
	return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
	if(prefix_len > 32) {
		return nullptr;
	}

	sockaddr_in baddr(m_addr);
	baddr.sin_addr.s_addr &= byteswapOnLittleEndian(
		CreateMask<uint32_t>(prefix_len));
	return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
	sockaddr_in subnet;
	memset(&subnet, 0, sizeof(subnet));
	subnet.sin_family = AF_INET;
	subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
	return IPv4Address::ptr(new IPv4Address(subnet));
}

uint32_t IPv4Address::getPort() const {
	return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPv4Address::setPort(uint16_t v) {
	m_addr.sin_port = byteswapOnLittleEndian(v);
}

/***************************************       IPv6       ****************************************/
IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port) {
	// 创建IPv6地址对象
	IPv6Address::ptr rt(new IPv6Address);
	// 设置端口
	rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
	// 转换IP地址
	int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
	if(result <= 0) {
		logger->debug("IPAddress::Create({}, {}) error={} errno={} errstr={}", 
			address, port, result, errno, strerror(errno));
		return nullptr;
	}
	return rt;
}

IPv6Address::IPv6Address() {
	memset(&m_addr, 0, sizeof(m_addr));
	m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6& address) {
	m_addr = address;
}

IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
	memset(&m_addr, 0, sizeof(m_addr));
	m_addr.sin6_family = AF_INET6;
	m_addr.sin6_port = byteswapOnLittleEndian(port);
	memcpy(m_addr.sin6_addr.s6_addr, address, 16);
}

const sockaddr* IPv6Address::getAddr() const {
	return (sockaddr *)&m_addr;
}

sockaddr* IPv6Address::getAddr() {
	return (sockaddr *)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const {
	return sizeof(m_addr);
}

std::ostream& IPv6Address::insert(std::ostream& os) const {
	os << "[";
	uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
	bool used_zero = false;
	for (size_t i = 0; i < 8; ++i) {
		// 如果当前位为0且前一位不为0，则跳过
		if(addr[i] == 0 && !used_zero) {
			continue;
		}
		if(i && addr[i-1] == 0 && !used_zero) {
			os << ":";
			used_zero = true;
		}
		if(i) {
			os << ":";
		}
		os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
	}

	if(used_zero && addr[7] == 0) {
		os << "::";
	}

	os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);

	return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
	sockaddr_in6 baddr(m_addr);
	baddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
	for(int i = prefix_len / 8 + 1; i < 16; ++i) {
		baddr.sin6_addr.s6_addr[i] = 0xff;
	}
	return IPAddress::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
	sockaddr_in6 baddr(m_addr);
	baddr.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
	for(int i = prefix_len / 8 + 1; i < 16; ++i) {
		baddr.sin6_addr.s6_addr[i] = 0x00;
	}
	return IPAddress::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
	sockaddr_in6 subnet;
	memset(&subnet, 0, sizeof(subnet));
	subnet.sin6_family = AF_INET6;
	subnet.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
	for(int i = prefix_len / 8 + 1; i < 16; ++i) {
		subnet.sin6_addr.s6_addr[i] = 0xff;
	}
	return IPAddress::ptr(new IPv6Address(subnet));
}

uint32_t IPv6Address::getPort() const {
	return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t v) {
	m_addr.sin6_port = byteswapOnLittleEndian(v);
}

/******************* unix *********************/
UnixAddress::UnixAddress() {
	memset(&m_addr, 0, sizeof(m_addr));
	m_addr.sun_family = AF_UNIX;
	// 计算结构体中变量与结构体地址之间的偏移量
	m_length = offsetof(sockaddr_un, sun_path);
}

UnixAddress::UnixAddress(const std::string& path) {
	memset(&m_addr, 0, sizeof(m_addr));
	m_addr.sun_family = AF_UNIX;
	m_length = path.size() + 1;

	if(!path.empty() && path[0] == '\0') {
		--m_length;
	}

	if(m_length > sizeof(m_addr.sun_path)) {
		throw std::logic_error("path too long");
	}

	memcpy(m_addr.sun_path, path.c_str(), m_length);
	m_length += offsetof(sockaddr_un, sun_path);

}

void UnixAddress::setAddrLen(uint32_t v) {
	m_length = v;
}

sockaddr* UnixAddress::getAddr() {
	return (sockaddr*)&m_addr;
}

const sockaddr* UnixAddress::getAddr() const {
	return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const {
	return m_length;
}

std::string UnixAddress::getPath() const {
	std::stringstream ss;
	if(m_length > offsetof(sockaddr_un, sun_path)
			&& m_addr.sun_path[0] == '\0') {
		ss << "\\0" << std::string(m_addr.sun_path + 1,
				m_length - offsetof(sockaddr_un, sun_path) - 1);
	} else {
		ss << m_addr.sun_path;
	}
	return ss.str();
}

std::ostream& UnixAddress::insert(std::ostream& os) const {
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

}
