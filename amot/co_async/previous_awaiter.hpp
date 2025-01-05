#pragma once

#include <coroutine>

namespace amot {
struct PreviousAwaiter {
    std::coroutine_handle<> m_previous;

    bool await_ready() const noexcept {
        return false;
    }

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> handle) const noexcept {
        if (m_previous) {
            return m_previous;
        } else {
            return std::noop_coroutine();
        }
    }

    void await_resume() const noexcept {}
};
} // namespace amot
