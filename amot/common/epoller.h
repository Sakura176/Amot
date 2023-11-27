/**
 * @file epoller.h
 * @author your name (you@domain.com)
 * @brief 封装epoll，使用更加方便
 * @version 0.1
 * @date 2023-11-26
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <sys/epoll.h>  //epoll_ctl()
#include <fcntl.h>      // fcntl()
#include <unistd.h>     // close()
#include <assert.h>     // close()
#include <vector>
#include <errno.h>

class Epoller {
public:
	explicit Epoller(int maxEvent = 1024);

	~Epoller();

	bool AddFd(int fd, uint32_t events);

	bool ModFd(int fd, uint32_t events);

	bool DelFd(int fd);

	int Wait(int timeoutMs = -1);

	/**
	 * @brief 获取事件句柄
	 * 
	 * @param i			事件索引
	 * @return int 
	 */
	int GetEventFd(size_t i) const;

	/**
	 * @brief 获取事件
	 * 
	 * @param i			事件索引
	 * @return uint32_t 
	 */
	uint32_t GetEvents(size_t i) const;

private:
	int epollFd_;

	std::vector<struct epoll_event> events_;
};