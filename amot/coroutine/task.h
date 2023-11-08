#pragma once

#include <coroutine>

#include "promise.h"

namespace amot {

template <typename ResultType, typename Executor = NoopExecutor>
struct Task {
	using promise_type = Promise<ResultType, Executor>;
	using Handle = std::coroutine_handle<promise_type>;

	explicit Task(Handle handle) noexcept : handle(handle) {}

	Task(Task &) = delete;
	Task(Task &&task) noexcept : handle(std::exchange(task.handle, {})) {}

	Task &operator=(Task &) = delete;

	~Task() {
		if (handle)
			handle.destroy();
	}

	void resume() {
		if (handle)
			handle.resume();
	}

	auto as_awaiter() {
		return TaskAwaiter<ResultType, Executor>(std::move(*this));
	}

	ResultType get_result() {
		return handle.promise().get_result();
	}

	Task& then(std::function<void(ResultType)> &&func) {
		handle.promise().on_completed([func](auto result) {
			try {
				func(result.get_or_throw());
			} catch (std::exception &e) {
				// ignore
			}
		});
		return *this;
	}

	Task& catching(std::function<void(std::exception &)> &&func) {
		handle.promise().on_completed([func](auto result) {
			try {
				result.get_or_throw();
			} catch (std::exception &e) {
				func(e);
			}
		});
		return *this;
	}

	Task& finally(std::function<void()> &&func) {
		handle.promise().on_completed([func](auto result) {
			func();
		});
		return *this;
	}

private:
	Handle handle;
};

template <typename Executor>
struct Task<void, Executor> {
	using promise_type = Promise<void, Executor>;
	using Handle = std::coroutine_handle<promise_type>;

	explicit Task(Handle handle) noexcept : handle(handle) {}

	Task(Task &) = delete;
	Task(Task &&task) noexcept : handle(std::exchange(task.handle, {})) {}

	Task &operator=(Task &) = delete;

	~Task() {
		if (handle)
			handle.destroy();
	}

	auto as_awaiter() {
		return TaskAwaiter<void, Executor>(std::move(*this));
	}

	// void偏特化无返回值，该函数用于阻塞当前线程等待协程执行完成
	void get_result() {
		handle.promise().get_result();
	}

	Task& then(std::function<void()> &&func) {
		handle.promise().on_completed([func](auto result) {
			try {
				result.get_or_throw();
				func();
			} catch (std::exception &e) {
				// ignore
			}
		});
		return *this;
	}

	Task& catching(std::function<void(std::exception &)> &&func) {
		handle.promise().on_completed([func](auto result) {
			try {
				result.get_or_throw();
			} catch (std::exception &e) {
				func(e);
			}
		});
		return *this;
	}

	Task& finally(std::function<void()> &&func) {
		handle.promise().on_completed([func](auto result) {
			func();
		});
		return *this;
	}

private:
	Handle handle;
};
}