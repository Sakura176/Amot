#pragma once

#include "amot/co_async/concepts.hpp"
#include "amot/co_async/return_prevoius.hpp"
#include "amot/co_async/task.hpp"
#include "amot/co_async/uninitialized.hpp"
#include "amot/utils/log.hpp"
#include <coroutine>
#include <exception>

namespace amot {
struct WhenAllCtlBlock {
    std::size_t m_count;
    std::coroutine_handle<> m_previous{};
    std::exception_ptr m_exception{};
};

/**
 * @struct WhenAllAwaiter
 * @brief
 *
 */
struct WhenAllAwaiter {
    bool await_ready() const noexcept {
        return false;
    }

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> coroutine) const {
        log_debug("WhenALLAwaiter await_suspend!");
        if (m_tasks.empty()) {
            return coroutine;
        }
        m_control.m_previous = coroutine;
        // 从第二个元素开始遍历容器，将任务加入到调度器
        for (auto const &t: m_tasks.subspan(0, m_tasks.size() - 1)) {
            t.m_coroutine.resume();
        }
        log_debug("push task finish!");
        // 返回第一个任务协程句柄，因此无需将该任务加入调度器
        return m_tasks.back().m_coroutine;
    }

    void await_resume() const {
        if (m_control.m_exception) [[unlikely]] {
            std::rethrow_exception(m_control.m_exception);
        }
    }

    WhenAllCtlBlock &m_control;
    std::span<ReturnPreviousTask const> m_tasks;
};

/**
 * @brief 等待多个任务的辅助函数，并处理异常。
 *
 * 此函数接收一个可等待任务、一个用于管理任务状态的控制块，以及一个结果容器。
 * 它等待任务并将结果存储在提供的结果容器中。如果发生异常，
 * 它会在控制块中捕获异常并返回之前的任务。
 *
 * @tparam T 要存储的结果类型。
 * @param t 要等待的可等待任务。
 * @param control 管理任务状态的控制块。
 * @param result 存储等待结果的结果容器。
 * @return ReturnPreviousTask 如果发生异常或所有任务都已等待，返回之前的任务。
 */
template <class T>
ReturnPreviousTask whenAllHelper(auto const &t, WhenAllCtlBlock &control,
                                 Uninitialized<T> &result) {
    try {
        log_debug("whenAllHelper put value");
        result.put_value(co_await t);
    } catch (...) {
        control.m_exception = std::current_exception();
        co_return control.m_previous;
    }
    --control.m_count;
    // 所有任务均处理完成，则返回之前的协程
    if (control.m_count == 0) {
        co_return control.m_previous;
    }
    log_debug("co_return nullptr");
    co_return nullptr;
};

/**
 * @brief 使用索引序列实现 when_all，等待多个任务。
 *
 * 此函数负责设置控制块并为每个任务调用辅助函数。它收集结果并将其作为元组返回。
 * 同时确保在返回结果之前所有任务都已被等待。
 *
 * @tparam Is 用于解包任务的索引序列。
 * @tparam Ts 可等待任务的类型。
 * @param is 生成的任务索引序列。
 * @param ts 要等待的可等待任务。
 * @return Task<std::tuple<typename AwaitableTraits<Ts>::NonVoidRetType...>>
 * 解析为包含 等待任务结果的元组的任务。
 */
template <std::size_t... Is, class... Ts>
Task<std::tuple<typename AwaitableTraits<Ts>::NonVoidRetType...>>
whenAllImpl(std::index_sequence<Is...>, Ts &&...ts) {
    // 初始化block,设置conut值
    WhenAllCtlBlock control{sizeof...(Ts)};
    std::tuple<Uninitialized<typename AwaitableTraits<Ts>::RetType>...> result;
    // 列表初始化，创建多个协程
    ReturnPreviousTask taskArray[]{
        whenAllHelper(ts, control, std::get<Is>(result))...};
    co_await WhenAllAwaiter(control, taskArray);
    co_return std::tuple<typename AwaitableTraits<Ts>::NonVoidRetType...>(
        std::get<Is>(result).move_value()...);
}

/**
 * @brief 并发等待多个可等待任务。
 *
 * 此函数为同时等待多个任务提供了方便的接口。它使用变长模板接收任意数量的
 * 可等待任务，并返回解析为包含每个等待任务结果的元组的任务。
 *
 * @tparam Ts 可等待任务的类型。
 * @param ts 要等待的可等待任务。
 * @return auto 解析为包含等待任务结果的元组的任务。
 * @requires sizeof...(Ts) != 0 确保至少提供一个任务。
 */
template <Awaitable... Ts>
    requires(sizeof...(Ts) != 0)
auto when_all(Ts &&...ts) {
    return whenAllImpl(std::make_index_sequence<sizeof...(Ts)>{},
                       std::forward<Ts>(ts)...);
}
} // namespace amot
