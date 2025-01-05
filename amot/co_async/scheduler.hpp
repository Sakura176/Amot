#pragma once

#include "amot/co_async/task.hpp"
#include "amot/utils/rbtree.hpp"
#include <coroutine>
#include <thread>

namespace amot {
struct SleepUntilPromise : RbTree<SleepUntilPromise>::RbNode, Promise<void> {
    std::chrono::system_clock::time_point m_expire_time;

    auto get_return_object() {
        return std::coroutine_handle<SleepUntilPromise>::from_promise(*this);
    }

    SleepUntilPromise &operator=(SleepUntilPromise &&) = delete;

    friend bool operator<(SleepUntilPromise const &lhs,
                          SleepUntilPromise const &rhs) noexcept {
        return lhs.m_expire_time < rhs.m_expire_time;
    }
};

struct Scheduler {
    RbTree<SleepUntilPromise> mRbTimer{};

    void addTimer(SleepUntilPromise &promise) {
        mRbTimer.insert(promise);
    }

    void run(std::coroutine_handle<> coroutine) {
        while (!coroutine.done()) {
            coroutine.resume();
            while (!mRbTimer.empty()) {
                if (!mRbTimer.empty()) {
                    auto now = std::chrono::system_clock::now();
                    auto &promise = mRbTimer.front();
                    if (promise.m_expire_time < now) {
                        mRbTimer.erase(promise);
                        std::coroutine_handle<SleepUntilPromise>::from_promise(
                            promise)
                            .resume();
                    } else {
                        std::this_thread::sleep_until(promise.m_expire_time);
                    }
                }
            }
        }
    }

    Scheduler &operator=(Scheduler &&) = delete;
};

inline Scheduler &get_scheduler() {
    static Scheduler scheduler;
    return scheduler;
}

struct SleepAwaiter {
    std::chrono::system_clock::time_point m_expire_time;

    bool await_ready() const noexcept {
        return false;
    }

    void
    await_suspend(std::coroutine_handle<SleepUntilPromise> coroutine) const {
        auto &promise = coroutine.promise();
        promise.m_expire_time = m_expire_time;
        get_scheduler().addTimer(promise);
    }

    void await_resume() const noexcept {}
};

inline Task<void, SleepUntilPromise>
sleep_util(std::chrono::system_clock::time_point expire_time) {
    co_await SleepAwaiter(expire_time);
    co_return;
}

inline Task<void, SleepUntilPromise>
sleep_for(std::chrono::system_clock::duration duration) {
    co_await SleepAwaiter(std::chrono::system_clock::now() + duration);
    co_return;
}
} // namespace amot
