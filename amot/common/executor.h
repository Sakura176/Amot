#pragma once

#include <queue>
#include <mutex>
#include <functional>
#include <future>
#include <spdlog/spdlog.h>

namespace amot {

class AbstractExecutor {
public:
	virtual void execute(std::function<void()> &&func) = 0;
};

class NoopExecutor : public AbstractExecutor {
public:
	void execute(std::function<void()> &&func) override {
		func();
	}
};

class NewThreadExecutor : public AbstractExecutor {
public:
	void execute(std::function<void()> &&func) override {
		std::thread(func).detach();
	}
};

class AsyncExecutor : public AbstractExecutor {
public:
	void execute(std::function<void()> &&func) override {
		auto future = std::async(func);
	}
};

/**
 * @brief 循环调度器，协程的恢复位置都在同一线程上
 */
class LooperExecutor : public AbstractExecutor {
public:
	LooperExecutor() {
		is_active.store(true, std::memory_order_relaxed);
		work_thread = std::thread(&LooperExecutor::run_loop, this);
	}

	~LooperExecutor() {
		shutdown(false);
		if (work_thread.joinable()) {
			work_thread.join();
		}
	}

	void execute(std::function<void()> &&func) override {
		std::unique_lock lock(queue_lock);
		if (is_active.load(std::memory_order_relaxed)) {
			executable_queue.push(func);
			lock.unlock();
			// 往队列中加入任务进行通知
			queue_condition.notify_one();
		}
	}

	void shutdown(bool wait_for_complete = true) {
		is_active.store(false, std::memory_order_relaxed);
		if (!wait_for_complete) {
			std::unique_lock lock(queue_lock);
			decltype(executable_queue) empty_queue;
			std::swap(executable_queue, empty_queue);
			lock.unlock();
		}
		queue_condition.notify_all();
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
			auto func = executable_queue.front();
			executable_queue.pop();
			lock.unlock();

			func();
		}
		spdlog::debug("executor run_loop exit.");
	}
private:
	std::condition_variable queue_condition;
	std::mutex queue_lock;
	std::queue<std::function<void()>> executable_queue;

	std::atomic<bool> is_active;
	std::thread work_thread;
};

class SharedLooperExecutor : public AbstractExecutor {
public:
	void execute(std::function<void()> &&func) override {
		static LooperExecutor sharedLooperExecutor;
		sharedLooperExecutor.execute(std::move(func));
	}
};
}