#pragma once

#include "amot/co_async/previous_awaiter.hpp"
#include "amot/utils/log.hpp"
#include "co_async/uninitialized.hpp"
#include <coroutine>

namespace amot {

template <class T>
struct Promise {
    Promise &operator=(Promise &&) = delete;

    std::suspend_always initial_suspend() noexcept {
        return std::suspend_always{};
    }

    auto final_suspend() noexcept {
        return PreviousAwaiter(m_previous);
    }

    void unhandled_exception() {
        m_exception = std::current_exception();
    }

    std::coroutine_handle<Promise> get_return_object() noexcept {
        return std::coroutine_handle<Promise>::from_promise(*this);
    }

    void return_value(T &&ret) {
        m_value.put_value(std::move(ret));
    }

    void return_value(T const &ret) {
        m_value.put_value(ret);
    }

    std::suspend_always yield_value(T ret) noexcept {
        m_value = ret;
        return {};
    }

    T result() {
        if (m_exception) [[unlikely]] {
            std::rethrow_exception(m_exception);
        }
        return m_value.move_value();
    }

    Uninitialized<T> m_value;
    std::exception_ptr m_exception{};
    std::coroutine_handle<> m_previous{};
};

template <>
struct Promise<void> {
    std::suspend_always initial_suspend() noexcept {
        log_debug("Promise initial_suspend");
        return std::suspend_always{};
    }

    auto final_suspend() noexcept {
        log_debug("Promise final_suspend");
        return PreviousAwaiter(m_previous);
    }

    void unhandled_exception() noexcept {
        m_exception = std::current_exception();
    }

    std::coroutine_handle<Promise> get_return_object() noexcept {
        return std::coroutine_handle<Promise>::from_promise(*this);
    }

    void return_void() noexcept {}

    void result() {
        if (m_exception) [[unlikely]] {
            std::rethrow_exception(m_exception);
        }
    }

    std::exception_ptr m_exception{};
    std::coroutine_handle<> m_previous{};
};

template <class T = void, class P = Promise<T>>
struct [[nodiscard]] Task {
    using promise_type = P;

    Task(std::coroutine_handle<promise_type> handle) noexcept
        : m_handle(handle) {
        log_debug("Task");
    }

    ~Task() {
        log_debug("~Task");
        m_handle.destroy();
    }

    struct Awaiter {
        bool await_ready() noexcept {
            log_debug("into await_ready");
            return false;
        }

        auto await_suspend(std::coroutine_handle<> handle) noexcept {
            log_debug("into await_suspend");
            _handle.promise().m_previous = handle;
            return _handle;
        }

        T await_resume() noexcept {
            log_debug("into await_resume");
            return _handle.promise().result();
        }

        std::coroutine_handle<promise_type> _handle;
    };

    auto operator co_await() const {
        log_debug("Awaiter co_await");
        return Awaiter(m_handle);
    }

    operator std::coroutine_handle<>() const noexcept {
        return m_handle;
    }

    std::coroutine_handle<promise_type> m_handle;
};

} // namespace amot
