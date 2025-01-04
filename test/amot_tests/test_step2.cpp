/*
 * @breif 简单协程实现，实现协程函数嵌套
 */
#include "amot/utils/log.hpp"
#include <coroutine>

struct PreviousAwaiter {
    std::coroutine_handle<> m_previous;

    bool await_ready() const noexcept {
        log_debug("PreviousAwaiter await_ready!");
        return false;
    }

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> handle) const noexcept {
        log_debug("PreviousAwaiter await_suspend!");
        if (m_previous) {
            return m_previous;
        } else {
            return std::noop_coroutine();
        }
    }

    void await_resume() const noexcept {
        log_debug("PreviousAwaiter await_resume!");
    }
};

template <class T>
struct Promise {
    std::suspend_always initial_suspend() noexcept {
        log_debug("Promise initial_suspend");
        return std::suspend_always{};
    }

    auto final_suspend() noexcept {
        log_debug("Promise final_suspend");
        return PreviousAwaiter(previous);
    }

    std::suspend_always unhandled_exception() {
        return {};
    }

    std::coroutine_handle<Promise> get_return_object() noexcept {
        return std::coroutine_handle<Promise>::from_promise(*this);
    }

    // std::suspend_always await_transform() {}

    void return_value(T ret) {
        m_value = ret;
    }

    std::suspend_always yield_value(T ret) noexcept {
        m_value = ret;
        return {};
    }

    T m_value;
    std::coroutine_handle<> previous = nullptr;
};

template <class T>
struct Task {
    using promise_type = Promise<T>;

    Task(std::coroutine_handle<promise_type> handle) noexcept
        : m_handle(handle) {}

    ~Task() {
        m_handle.destroy();
    }

    struct Awaiter {
        bool await_ready() noexcept {
            log_debug("into await_ready");
            return false;
        }

        auto await_suspend(std::coroutine_handle<> handle) noexcept {
            log_debug("into await_suspend");
            _handle.promise().previous = handle;
            return _handle;
        }

        T await_resume() noexcept {
            log_debug("into await_resume");
            return _handle.promise().m_value;
        }

        std::coroutine_handle<promise_type> _handle;
    };

    auto operator co_await() const {
        return Awaiter(m_handle);
    }

    std::coroutine_handle<promise_type> m_handle;
};

Task<std::string> str_task() {
    log_info("into str_task");
    co_return "str";
}

Task<double> sub_task() {
    log_info("into sub task");
    co_return 41.1;
}

Task<int> hello() {
    log_info("into task hello");
    auto i = co_await sub_task();
    log_info("sub task ret {}", i);
    co_return i + 1;
}

int main(int argc, char *argv[]) {
    log_info("coroutine begin.......");
    auto task = hello();
    log_info("call hello!!!");
    while (!task.m_handle.done()) {
        log_info("before hello resume");
        task.m_handle.resume();
        log_info("task hello {}", task.m_handle.promise().m_value);
    }
    log_info("coroutine end........");
    return 0;
}
