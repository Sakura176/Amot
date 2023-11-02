#pragma once

#include <coroutine>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <optional>
#include <list>

#include "result.h"
#include "awaiter.h"

namespace amot {

template<typename AwaiterImpl, typename R>
concept AwaiterImplRestriction = std::is_base_of<Awaiter<R>, AwaiterImpl>::value;

template <typename ResultType, typename Executor>
struct Task;

template <typename ResultType, typename Executor>
struct Promise {
	// 协程立即执行
	DispatchAwaiter initial_suspend() { return DispatchAwaiter{&executor}; }

	// 协程结束后挂起，等待外部销毁
	std::suspend_always final_suspend() noexcept { return {}; }

	// 构造协程的返回值对象
	Task<ResultType, Executor> get_return_object() {
		return Task{std::coroutine_handle<Promise>::from_promise(*this)};
	}

	template <typename _ResultType, typename _Executor>
	TaskAwaiter<_ResultType, _Executor> await_transform(Task<_ResultType, _Executor> &&task) {
		return await_transform(TaskAwaiter<_ResultType, _Executor>(std::move(task)));
	}

	template<typename _Rep, typename _Period>
	SleepAwaiter await_transform(std::chrono::duration<_Rep, _Period> &&duration) {
		return await_transform(SleepAwaiter(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()));
	}

	template<typename AwaiterImpl>
	requires AwaiterImplRestriction<AwaiterImpl, typename AwaiterImpl::ResultType>
	AwaiterImpl await_transform(AwaiterImpl awaiter) {
		awaiter.install_executor(&executor);
		return awaiter;
	}

	void return_value(ResultType val) {
		std::lock_guard lock(completion_lock);
		result = Result<ResultType>(std::move(val));
		// 通知 get_result 当中的 wait
		completion.notify_all();
		// 调用回调
		notify_callbacks();
	}

	void unhandled_exception() { 
		std::lock_guard lock(completion_lock);
		result = Result<ResultType>(std::current_exception());
		completion.notify_all();
		// 调用回调
		notify_callbacks();
	}

	ResultType get_result() {
		// 如果 result 没有值，说明协程还没有运行完，等待值被写入再返回
		std::unique_lock lock(completion_lock);
		if (!result.has_value()) {
			// 等待写入值之后调用 notify_all
			completion.wait(lock);
		}
		// 如果有值，则直接返回（或抛出异常）
		return result->get_or_throw();
	}

	void on_completed(std::function<void(Result<ResultType>)> &&func) {
		std::unique_lock lock(completion_lock);
		// 加锁判断 result
		if (result.has_value()) {
			// result 已经有值
			auto value = result.value();
			// 解锁之后再调用 func
			lock.unlock();
			func(value);
		} else {
			// 否则添加回调函数，等待调用
			completion_callbacks.emplace_back(func);
		}
	}
private:
	void notify_callbacks() {
		auto value = result.value();
		for (auto& callback : completion_callbacks) {
			callback(value);
		}
		// 调用完成，清空回调
		completion_callbacks.clear();
	}
private:
	std::optional<Result<ResultType>> result;

	std::mutex completion_lock;
	std::condition_variable completion;
	// 回调列表，我们允许对同一个 Task 添加多个回调
	std::list<std::function<void(Result<ResultType>)>> completion_callbacks;

	Executor executor;
};

template <typename Executor>
struct Promise<void, Executor> {
	// 协程立即执行
	DispatchAwaiter initial_suspend() { return DispatchAwaiter{&executor}; }

	// 协程结束后挂起，等待外部销毁
	std::suspend_always final_suspend() noexcept { return {}; }

	// 构造协程的返回值对象
	Task<void, Executor> get_return_object() {
		return Task{std::coroutine_handle<Promise>::from_promise(*this)};
	}

	template <typename _ResultType, typename _Executor>
	TaskAwaiter<_ResultType, _Executor> await_transform(Task<_ResultType, _Executor> &&task) {
		return await_transform(TaskAwaiter<_ResultType, _Executor>(&executor, std::move(task)));
	}

	template<typename _Rep, typename _Period>
	SleepAwaiter await_transform(std::chrono::duration<_Rep, _Period> &&duration) {
		return await_transform(SleepAwaiter(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()));
	}

	template<typename AwaiterImpl>
	requires AwaiterImplRestriction<AwaiterImpl, typename AwaiterImpl::ResultType>
	AwaiterImpl await_transform(AwaiterImpl &&awaiter) {
		awaiter.install_executor(&executor);
		return awaiter;
	}

	void unhandled_exception() { 
		std::lock_guard lock(completion_lock);
		result = Result<void>(std::current_exception());
		completion.notify_all();
		// 调用回调
		notify_callbacks();
	}

	void get_result() {
		// 如果 result 没有值，说明协程还没有运行完，等待值被写入再返回
		std::unique_lock lock(completion_lock);
		if (!result.has_value()) {
			// 等待写入值之后调用 notify_all
			completion.wait(lock);
		}
		// 如果有值，则直接返回（或抛出异常）
		result->get_or_throw();
	}

	void return_void() {
		std::lock_guard lock(completion_lock);
		result = Result<void>();
		completion.notify_all();
		notify_callbacks();
	}

	void on_completed(std::function<void(Result<void>)> &&func) {
		std::unique_lock lock(completion_lock);
		// 加锁判断 result
		if (result.has_value()) {
			// result 已经有值
			auto value = result.value();
			// 解锁之后再调用 func
			lock.unlock();
			func(value);
		} else {
			// 否则添加回调函数，等待调用
			completion_callbacks.emplace_back(func);
		}
	}
private:
	void notify_callbacks() {
		auto value = result.value();
		for (auto& callback : completion_callbacks) {
			callback(value);
		}
		// 调用完成，清空回调
		completion_callbacks.clear();
	}
private:
	std::optional<Result<void>> result;

	std::mutex completion_lock;
	std::condition_variable completion;
	// 回调列表，我们允许对同一个 Task 添加多个回调
	std::list<std::function<void(Result<void>)>> completion_callbacks;

	Executor executor;
};

} // namespace amot