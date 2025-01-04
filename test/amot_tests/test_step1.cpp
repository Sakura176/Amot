/*
 * @breif 简单协程实现
 */
#include "amot/utils/log.hpp"
#include <coroutine>

struct Promise {
    int m_value;

    std::suspend_always initial_suspend() noexcept {
        return std::suspend_always{};
    }

    std::suspend_always final_suspend() noexcept {
        return std::suspend_always{};
    }

    std::suspend_always unhandled_exception() {
        return {};
    }

    std::coroutine_handle<Promise> get_return_object() noexcept {
        return std::coroutine_handle<Promise>::from_promise(*this);
    }

    void return_void() {
        m_value = 0;
        return;
    }

    std::suspend_always yield_value(int ret) noexcept {
        m_value = ret;
        return {};
    }
};

struct Task {
    using promise_type = Promise;

    Task(std::coroutine_handle<promise_type> handle) : m_handle(handle) {}

    std::coroutine_handle<promise_type> m_handle;
};

Task hello() {
    log_info("hello 42");
    co_yield 42;
    log_info("hello 12");
    co_yield 12;
    log_info("hello 6");
    co_yield 6;
    log_info("hello 结束");
    co_return;
}

int main(int argc, char *argv[]) {
    log_info("coroutine begin.......");
    auto task = hello();
    log_info("call hello!!!");
    while (!task.m_handle.done()) {
        task.m_handle.resume();
        log_info("task hello {}", task.m_handle.promise().m_value);
    }
    log_info("coroutine end........");
    return 0;
}
