#pragma once

#include <coroutine>

#include "executor.h"
#include "scheduler.h"

namespace amot {

template <typename ResultType, typename Executor>
struct Task;

/**
 * @brief co_await 表达式操作的对象
 * 
 * @tparam R 结果类别
 */
template <typename R>
struct Awaiter {
	using ResultType = R;

	// 简单处理，永远挂起
	bool await_ready() const { return false; }

	// 协程挂起后调用
	void await_suspend(std::coroutine_handle<> handle) {
		// 记录当前协程的 handle，方便后续恢复
		this->_handle = handle;
		// 调用 after_suspend，逻辑由子类定义
		after_suspend();
	}

	// 协程恢复执行后调用，返回值为co_await 表达式的返回值
	ResultType await_resume() {
		before_resume();
		// 返回 co_await 的结果
		return _result->get_or_throw();
	}

	void resume(ResultType value) {
		dispatch([this, value]() {
			_result = Result<ResultType>(static_cast<R>(value));
			_handle.resume();
		});
	}

	void resume_unsafe() {
		dispatch([this]() { _handle.resume(); });
	}

	void resume_exception(std::exception_ptr &&e) {
		dispatch([this, e]() {
			_result = Result<R>(static_cast<std::exception_ptr>(e));
			_handle.resume();
		});
	}

	void install_executor(AbstractExecutor *executor) {
		_executor = executor;
	}
protected:
	/**
	 * @brief 虚函数，在子类中实现具体逻辑
	 * 
	 */
	virtual void after_suspend() {}

	virtual void before_resume() {}
private:
	// 方便用调度器调度任意逻辑
	void dispatch(std::function<void()> &&f) {
		if (_executor) {
			_executor->execute(std::move(f));
		} else {
			f();
		}
	}

protected:
	std::optional<Result<R>> _result{};
private:
	AbstractExecutor *_executor = nullptr;
	std::coroutine_handle<> _handle = nullptr;
};

template <>
struct Awaiter<void> {
	using ResultType = void;

	bool await_ready() const { return false; }

	void await_suspend(std::coroutine_handle<> handle) {
		this->_handle = handle;
		after_suspend();
	}

	ResultType await_resume() {
		before_resume();
		return _result->get_or_throw();
	}

	void resume() {
		dispatch([this]() {
			_result = Result<ResultType>();
			_handle.resume();
		});
	}

	void resume_exception(std::exception_ptr &&e) {
		dispatch([this, e]() {
			_result = Result<void>(static_cast<std::exception_ptr>(e));
			_handle.resume();
		});
	}

	void install_executor(AbstractExecutor *executor) {
		_executor = executor;
	}
protected:
	virtual void after_suspend() {}

	virtual void before_resume() {}
private:
	void dispatch(std::function<void()> &&f) {
		if (_executor) {
			_executor->execute(std::move(f));
		} else {
			f();
		}
	}

protected:
	std::optional<Result<ResultType>> _result{};
private:
	AbstractExecutor *_executor = nullptr;
	std::coroutine_handle<> _handle = nullptr;
};

// 调度器的类型有多种，因此专门提供一个模板参数 Executor
template <typename R, typename Executor>
struct TaskAwaiter : public Awaiter<R> {

	// 构造时传入调度器的具体实现
	explicit TaskAwaiter(Task<R, Executor> &&task) noexcept
		: task(std::move(task)) {}

	TaskAwaiter(TaskAwaiter &&awaiter) noexcept
		: Awaiter<R>(awaiter), task(std::move(awaiter.task)) {}
	
	TaskAwaiter(TaskAwaiter &) = delete;

	TaskAwaiter &operator=(TaskAwaiter &) = delete;

protected:
	void after_suspend() override {
		task.finally([this]() {
			this->resume_unsafe();
		});
	}

	void before_resume() override {
		this->_result = Result(task.get_result());
	}

private:
	Task<R, Executor> task;
};

struct DispatchAwaiter {
	explicit DispatchAwaiter(AbstractExecutor *executor) noexcept
		: _executor(executor) {}
	
	bool await_ready() const { return false; }

	void await_suspend(std::coroutine_handle<> handle) const {
		_executor->execute([handle]() {
			handle.resume();
		});
	}

	void await_resume() {}
private:
	AbstractExecutor *_executor;
};

struct SleepAwaiter : public Awaiter<void> {
	explicit SleepAwaiter(long long duration) noexcept
		: _duration(duration) {}

	template<typename _Rep, typename _Period>
	explicit SleepAwaiter(std::chrono::duration<_Rep, _Period> &&duration) noexcept
		: _duration(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()) {}

	void after_suspend() override {
		static Scheduler scheduler;
		scheduler.execute([this] { resume(); }, _duration);
	}
private:
	long long _duration;
};

template <typename ValueType>
struct Channel;

template <typename ValueType>
struct WriterAwaiter : public Awaiter<void> {
	WriterAwaiter(Channel<ValueType> *channel, ValueType value)
		: channel(channel), _value(value) {}
	
	WriterAwaiter(WriterAwaiter &&other) noexcept
		: Awaiter(other),
		  channel(std::exchange(other.channel, nullptr)),
		  _value(other._value) {}

	void after_suspend() override {
		channel->try_push_writer(this);
	}

	void before_resume() override {
		channel->check_closed();
		channel = nullptr;
	}

	~WriterAwaiter() {
		if (channel) channel->remove_writer(this);
	}

	Channel<ValueType> *channel;
	ValueType _value;
};

template <typename ValueType>
struct ReaderAwaiter : public Awaiter<ValueType> {
	explicit ReaderAwaiter(Channel<ValueType> *channel) 
		: Awaiter<ValueType>(), channel(channel) {}
	
	ReaderAwaiter(ReaderAwaiter &&other) noexcept
		: Awaiter<ValueType>(other),
		  channel(std::exchange(other.channel, nullptr)),
		  p_value(std::exchange(other.p_value, nullptr)) {}

	void after_suspend() override {
		channel->try_push_reader(this);
	}

	void before_resume() override {
		channel->check_closed();
		if (p_value) {
			*p_value = this->_result->get_or_throw();
		}
		channel = nullptr;
	}

	~ReaderAwaiter() {
		if (channel) channel->remove_reader(this);
	}

	Channel<ValueType> *channel;
	ValueType *p_value = nullptr;
};

template<typename R>
struct FutureAwaiter : public Awaiter<R> {
	explicit FutureAwaiter(std::future<R> &&future) noexcept
		: _future(std::move(future)) {}

	FutureAwaiter(FutureAwaiter &&awaiter) noexcept
		: Awaiter<R>(awaiter), _future(std::move(awaiter._future)) {}

	FutureAwaiter(FutureAwaiter &) = delete;

	FutureAwaiter &operator=(FutureAwaiter &) = delete;

	protected:
	void after_suspend() override {
		std::thread([this](){
		this->resume(this->_future.get());
		}).detach();
	}

	private:
	std::future<R> _future;
};
} // namespace amot