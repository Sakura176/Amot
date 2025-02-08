#pragma once

#include "amot/co_async/task.hpp"
#include "amot/utils/rbtree.hpp"
#include <chrono>
#include <coroutine>
#include <optional>
#include <thread>

namespace amot {
struct SleepUntilPromise : RbTree<SleepUntilPromise>::RbNode, Promise<void> {
    std::chrono::system_clock::time_point mExpireTime;

    auto get_return_object() {
        return std::coroutine_handle<SleepUntilPromise>::from_promise(*this);
    }

    SleepUntilPromise &operator=(SleepUntilPromise &&) = delete;

    friend bool operator<(SleepUntilPromise const &lhs,
                          SleepUntilPromise const &rhs) noexcept {
        return lhs.mExpireTime < rhs.mExpireTime;
    }
};

struct Scheduler {
    RbTree<SleepUntilPromise> mRbTimer{};

    bool hasEvent() const noexcept {
        return !mRbTimer.empty();
    }

    void addTimer(SleepUntilPromise &promise) {
        mRbTimer.insert(promise);
    }

    std::optional<std::chrono::system_clock::duration> run() {
        while (!mRbTimer.empty()) {
            auto now = std::chrono::system_clock::now();
            auto &promise = mRbTimer.front();
            if (promise.mExpireTime < now) {
                mRbTimer.erase(promise);
                std::coroutine_handle<SleepUntilPromise>::from_promise(promise)
                    .resume();
            } else {
                return promise.mExpireTime - now;
            }
        }
        return std::nullopt;
    }

    Scheduler &operator=(Scheduler &&) = delete;
};

inline Scheduler &get_scheduler() {
    static Scheduler scheduler;
    return scheduler;
}

struct SleepAwaiter {
    using ClockType = std::chrono::system_clock;
    Scheduler &mLoop;
    ClockType::time_point mExpireTime;

    bool await_ready() const noexcept {
        return false;
    }

    void
    await_suspend(std::coroutine_handle<SleepUntilPromise> coroutine) const {
        auto &promise = coroutine.promise();
        promise.mExpireTime = mExpireTime;
        mLoop.addTimer(promise);
    }

    void await_resume() const noexcept {}
};

template <class Clock, class Dur>
inline Task<void, SleepUntilPromise>
sleep_util(Scheduler &loop, std::chrono::time_point<Clock, Dur> expire_time) {
    co_await SleepAwaiter(
        loop, std::chrono::time_point_cast<SleepAwaiter::ClockType::duration>(
                  expire_time));
}

template <class Rep, class Period>
inline Task<void, SleepUntilPromise>
sleep_for(Scheduler &loop, std::chrono::duration<Rep, Period> duration) {
    auto d =
        std::chrono::duration_cast<SleepAwaiter::ClockType::duration>(duration);
    if (d.count() > 0) {
        co_await SleepAwaiter(loop, SleepAwaiter::ClockType::now() + d);
    }
}
} // namespace amot
