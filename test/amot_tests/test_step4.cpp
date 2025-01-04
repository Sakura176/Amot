/*
 * @breif 简单协程实现，实现定时函数
 */
#include "amot/utils/log.hpp"
#include <chrono>
#include <coroutine>
#include <deque>
#include <queue>
#include <thread>

using namespace std::chrono_literals;

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

    T result() {
        return m_value;
    }

    T m_value;
    std::coroutine_handle<> previous = nullptr;
};

template <>
struct Promise<void> {
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

    void return_void() noexcept {}

    void result() {}

    std::coroutine_handle<> previous = nullptr;
};

template <class T = void>
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
            return _handle.promise().result();
        }

        std::coroutine_handle<promise_type> _handle;
    };

    auto operator co_await() const {
        return Awaiter(m_handle);
    }

    operator std::coroutine_handle<>() const noexcept {
        return m_handle;
    }

    std::coroutine_handle<promise_type> m_handle;
};

struct Scheduler {
    std::deque<std::coroutine_handle<>> m_ready_queue;

    struct TimerEntry {
        std::chrono::system_clock::time_point expire_time;
        std::coroutine_handle<> coroutine;

        bool operator<(TimerEntry const &that) const noexcept {
            return expire_time > that.expire_time;
        }
    };

    std::priority_queue<TimerEntry> m_timer_heap;

    void add_task(std::coroutine_handle<> coroutine) {
        m_ready_queue.push_front(coroutine);
    }

    void add_timer(std::chrono::system_clock::time_point expire_time,
                   std::coroutine_handle<> coroutine) {
        m_timer_heap.push({expire_time, coroutine});
    }

    void run_loop() {
        log_info("run_loop begin");
        while (!m_timer_heap.empty() || !m_ready_queue.empty()) {
            // 队列中存在任务，则弹出任务并恢复协程
            while (!m_ready_queue.empty()) {
                auto coroutine = m_ready_queue.front();
                m_ready_queue.pop_front();
                log_info("before coroutine resume");
                coroutine.resume(); // NOTE: 协程如何返回
                log_info("after coroutine resume");
            }
            // 时间堆中任务处理
            if (!m_timer_heap.empty()) {
                auto now = std::chrono::system_clock::now();
                auto timer = std::move(m_timer_heap.top());
                if (timer.expire_time < now) {
                    m_timer_heap.pop();
                    timer.coroutine.resume();
                } else {
                    std::this_thread::sleep_until(timer.expire_time);
                }
            }
        }
    }

    Scheduler &operator=(Scheduler &&) = delete;
};

Scheduler &get_scheduler() {
    static Scheduler scheduler;
    return scheduler;
}

struct SleepAwaiter {
    std::chrono::system_clock::time_point m_expire_time;

    // 返回false则刮起协程
    bool await_ready() const noexcept {
        log_info("SleepAwaiter await_ready");
        return false;
    }

    // 协程挂起后执行该函数
    void await_suspend(std::coroutine_handle<> coroutine) const {
        log_info("SleepAwaiter await_suspend before add_timer");
        get_scheduler().add_timer(m_expire_time, coroutine);
        log_info("SleepAwaiter await_suspend after add_timer");
    }

    void await_resume() const noexcept {
        log_info("SleepAwaiter await_resume");
    }
};

Task<void> sleep_util(std::chrono::system_clock::time_point expire_time) {
    co_await SleepAwaiter(expire_time);
    co_return;
}

Task<void> sleep_for(std::chrono::system_clock::duration duration) {
    co_await SleepAwaiter(std::chrono::system_clock::now() + duration);
    log_info("sleep_for return");
    co_return;
}

Task<int> hello1() {
    log_info("hello1开始睡1秒");
    co_await sleep_for(1s); // 1s 等价于 std::chrono::seconds(1)
    log_info("hello1睡醒了");
    co_return 1;
}

Task<int> hello2() {
    log_info("hello2开始睡2秒");
    co_await sleep_for(2s); // 2s 等价于 std::chrono::seconds(2)
    log_info("hello2睡醒了");
    co_return 2;
}

int main() {
    auto t1 = hello1();
    auto t2 = hello2();
    get_scheduler().add_task(t1);
    get_scheduler().add_task(t2);
    get_scheduler().run_loop();
    log_info("主函数中得到hello1结果: {}", t1.m_handle.promise().result());
    log_info("主函数中得到hello2结果: {}", t2.m_handle.promise().result());
    return 0;
}
