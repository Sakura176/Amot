#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "fd_manager.h"
// #include "../include/hook.h"

namespace amot {
static auto logger = spdlog::get("system");

FdCtx::FdCtx(int fd)
	:m_isInit(false)
	,m_isSocket(false)
	,m_sysNonblock(false)
	,m_userNonblock(false)
	,m_isClosed(false)
	,m_fd(fd)
	,m_recvTimeout(-1)
	,m_sendTimeout(-1)
{ init(); }

FdCtx::~FdCtx()
{ }

bool FdCtx::init() {
	logger->info("fdctx init");
	if(m_isInit) {
		return false;
	}
	m_recvTimeout = -1;
	m_sendTimeout = -1;

	// stat结构体，保存文件状态信息
	struct stat fd_stat;
	// 通过文件描述符获取文件状态信息，-1表示获取失败
	if(-1 == fstat(m_fd, &fd_stat)) {
		m_isInit = false;
		m_isSocket = false;
	} else {
		m_isInit = true;
		// 判断是否为socket文件
		m_isSocket = S_ISSOCK(fd_stat.st_mode);
	}

	if(m_isSocket) {
		// 获取文件状态标志
		int flags = fcntl(m_fd, F_GETFL, 0);
		if(!(flags & O_NONBLOCK)) {
			// 设置非阻塞
			fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);
		}
		m_sysNonblock = true;
	} else {
		m_sysNonblock = false;
	}

	m_userNonblock = false;
	m_isClosed = false;
	return m_isInit;
}

void FdCtx::setTimeout(int type, uint64_t v) {
	if(type == SO_RCVTIMEO) {
		m_recvTimeout = v;
	} else {
		m_sendTimeout = v;
	}
}

uint64_t FdCtx::getTimeout(int type) {
	if(type == SO_RCVTIMEO) {
		return m_recvTimeout;
	}
	else {
		return m_sendTimeout;
	}
}

FdManager::FdManager() {
	m_datas.resize(64);
}

FdCtx::ptr FdManager::get(int fd, bool auto_create) {
	std::shared_lock lock(m_mutex);
	// 若句柄值小于储存值，且不允许自动创建
	if((int)m_datas.size() <= fd) {
		if(auto_create == false) {
			return nullptr;
		}
	} else {
		if(m_datas[fd] || !auto_create) {
			//spdlog::info("return: {}", m_datas[fd]->isSocket());
			return m_datas[fd];
		}
	}
	lock.unlock();

	std::unique_lock lock2(m_mutex);
	FdCtx::ptr ctx(new FdCtx(fd));
	if(fd >= (int)m_datas.size()) {
		m_datas.resize(fd * 1.5);
	}
	m_datas[fd] = ctx;
	return ctx;
}

void FdManager::del(int fd) {
	std::unique_lock lock(m_mutex);
	if((int)m_datas.size() <= fd) {
		return;
	}
	m_datas[fd].reset();
}
}