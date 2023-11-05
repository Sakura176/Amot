#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <chrono>
#include <spdlog/spdlog.h>

namespace amot {

/**
 * @brief 延时执行器
 */
class DelayedExecutable {

public:
	DelayedExecutable(std::function<void()> &&func, long long delay) : func(std::move(func)) {
		auto now = std::chrono::system_clock::now();
		auto current = std::chrono::duration_cast<std::chrono::milliseconds>
												(now.time_since_epoch()).count();
		scheduled_time = current + delay;
	}
	
	long long delay() const {
		auto now = std::chrono::system_clock::now();
		auto current = std::chrono::duration_cast<std::chrono::milliseconds>
												(now.time_since_epoch()).count();
		return scheduled_time - current;
	}

	long long get_scheduled_time() const {
		return scheduled_time;
	}

	void operator()() {
		func();
	}

private:
	long long scheduled_time;		// 计划执行时间
	std::function<void()> func;		// 任务
};

// 排序，在队列中按执行时间进行排序
class DelayedExecutableCompare {
public:
	bool operator()(DelayedExecutable &left, DelayedExecutable &right) {
		return left.get_scheduled_time() > right.get_scheduled_time();
	}
};

class Scheduler {
public:
	using ExecutableQueue = std::priority_queue<
			DelayedExecutable, std::vector<DelayedExecutable>, DelayedExecutableCompare>;
	/**
	 * @brief 创建调度器
	 */
	Scheduler() {
		is_active.store(true, std::memory_order_relaxed);
		// 初始化线程同时绑定循环任务
		work_thread = std::thread(&Scheduler::run_loop, this);
	}

	~Scheduler() {
		shutdown(false);
		join();
	}

	void execute(std::function<void()> &&func, long long delay) {
		delay = delay < 0 ? 0 : delay;
		std::unique_lock lock(queue_lock);
		if (is_active.load(std::memory_order_relaxed)) {
			bool need_notify = executable_queue.empty() || executable_queue.top().delay() > delay;
			executable_queue.push(DelayedExecutable(std::move(func), delay));
			lock.unlock();
			if (need_notify) {
				queue_condition.notify_one();
			}
		}
	}

	void shutdown(bool wait_for_complete = true) {
		// TODO 在推出后仍然需要再调用join函数
		is_active.store(false, std::memory_order_relaxed);
		if (!wait_for_complete) {
			std::unique_lock lock(queue_lock);
			decltype(executable_queue) empty_queue;
			std::swap(executable_queue, empty_queue);
			lock.unlock();
		} 
		queue_condition.notify_all();
	}

	void join() {
		if (work_thread.joinable()) {
			work_thread.join();
		}
	}
private:
	void run_loop() {
		// 检查当前事件循环是否是工作状态，或者队列没有清空
		while (is_active.load(std::memory_order_relaxed) || !executable_queue.empty()) {
			std::unique_lock lock(queue_lock);
			if (executable_queue.empty()) {
				// 队列为空时，阻塞等待新任务加入队列或者关闭事件循环的通知
				queue_condition.wait(lock);
				if (executable_queue.empty()) {
					continue;
				}
			}
			// 队列不为空，则先取出任务，解锁后执行
			// 注意：func 是外部逻辑，不需要锁保护；func 当中可能请求锁，导致死锁
			auto executable = executable_queue.top();
			long long delay = executable.delay();
			// 时间大于0， 则等待该时间
			if (delay > 0) {
				// 条件变量进行等待
				auto status = queue_condition.wait_for(lock, std::chrono::milliseconds(delay));
				// TODO 此处未理解
				if (status != std::cv_status::timeout) {
					continue;
				}
			}
			executable_queue.pop();
			lock.unlock();
			executable();
		}
		spdlog::debug("run_loop exit.");
	}

private:
	std::condition_variable queue_condition;
	std::mutex queue_lock;
	ExecutableQueue executable_queue;

	std::atomic<bool> is_active;
	std::thread work_thread;
};
}