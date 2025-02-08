#pragma once

#include "amot/co_async/concepts.hpp"
#include "amot/co_async/return_prevoius.hpp"
#include "amot/co_async/task.hpp"
#include "amot/co_async/uninitialized.hpp"
#include <coroutine>
#include <cstddef>
#include <exception>
#include <utility>
#include <variant>

namespace amot {
struct WhenAnyCtlBlock {
    static constexpr std::size_t kNullIndex = std::size_t(-1);

    // 初始化为最大值，避免任务数大于计数执行错误
    std::size_t m_index{kNullIndex};
    std::coroutine_handle<> mPrevious{};
    std::exception_ptr mException{};
};

struct whenAnyAwaiter {
    bool await_ready() const noexcept {
        return false;
    }

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> coroutine) noexcept {
        if (mTasks.empty()) {
            return coroutine;
        }
        mControl.mPrevious = coroutine;
        for (auto const &t: mTasks.subspan(0, mTasks.size() - 1)) {
            t.m_coroutine.resume();
        }
        return mTasks.front().m_coroutine;
    }

    void await_resume() const {
        if (mControl.mException) [[likely]] {
            std::rethrow_exception(mControl.mException);
        }
    }

    WhenAnyCtlBlock &mControl;
    std::span<ReturnPreviousTask const> mTasks;
};

template <class T>
ReturnPreviousTask whenAnyHelper(auto &&t, WhenAnyCtlBlock &control,
                                 Uninitialized<T> &result, std::size_t index) {
    try {
        result.put_value(
            (co_await std::forward<decltype(t)>(t), NonVoidHelper<>()));
    } catch (...) {
        control.mException = std::current_exception();
        co_return control.mPrevious;
    }
    --control.m_index = index;
    co_return control.mPrevious;
};

template <std::size_t... Is, class... Ts>
Task<std::variant<typename AwaitableTraits<Ts>::NonVoidRetType...>>
whenAnyImpl(std::index_sequence<Is...>, Ts &&...ts) {
    WhenAnyCtlBlock control{};
    std::tuple<Uninitialized<typename AwaitableTraits<Ts>::RetType>...> result;

    ReturnPreviousTask taskArray[]{
        whenAnyHelper(ts, control, std::get<Is>(result), Is)...};
    co_await whenAnyAwaiter(control, taskArray);
    Uninitialized<std::variant<typename AwaitableTraits<Ts>::NonVoidRetType...>>
        varResult;
    ((control.m_index == Is &&
      (varResult.put_value(std::in_place_index<Is>,
                           std::get<Is>(result).move_value()),
       0)),
     ...);
    co_return varResult.move_value();
}

template <Awaitable... Ts>
    requires(sizeof...(Ts) != 0)
auto when_any(Ts &&...ts) {
    return whenAnyImpl(std::make_index_sequence<sizeof...(Ts)>{},
                       std::forward<Ts>(ts)...);
};

} // namespace amot
