/*
 * @breif 简单协程实现，实现多个函数同时加入调度器
 */
#include "amot/coroutine/concepts.hpp"
#include "amot/coroutine/uninitialized.hpp"
#include "amot/utils/log.hpp"
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <deque>
#include <exception>
#include <queue>
#include <thread>
#include <utility>

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
            log_info("PreviousAwaiter return m_previous");
            return m_previous;
        } else {
            log_info("PreviousAwaiter return std::noop_coroutine");
            return std::noop_coroutine();
        }
    }

    void await_resume() const noexcept {
        log_debug("PreviousAwaiter await_resume!");
    }
};

template <class T>
struct Promise {
    Promise &operator=(Promise &&) = delete;

    std::suspend_always initial_suspend() noexcept {
        log_debug("Promise initial_suspend");
        return std::suspend_always{};
    }

    auto final_suspend() noexcept {
        log_debug("Promise final_suspend");
        return PreviousAwaiter(m_previou);
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
    std::coroutine_handle<> m_previou{};
};

template <>
struct Promise<void> {
    std::suspend_always initial_suspend() noexcept {
        log_debug("Promise initial_suspend");
        return std::suspend_always{};
    }

    auto final_suspend() noexcept {
        log_debug("Promise final_suspend");
        return PreviousAwaiter(m_previou);
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
    std::coroutine_handle<> m_previou{};
};

template <class T = void>
struct Task {
    using promise_type = Promise<T>;

    Task(std::coroutine_handle<promise_type> handle) noexcept
        : m_handle(handle) {}

    ~Task() {
        log_info("~Task");
        m_handle.destroy();
    }

    struct Awaiter {
        bool await_ready() noexcept {
            log_debug("into await_ready");
            return false;
        }

        auto await_suspend(std::coroutine_handle<> handle) noexcept {
            log_debug("into await_suspend");
            _handle.promise().m_previou = handle;
            return _handle;
        }

        T await_resume() noexcept {
            log_debug("into await_resume");
            return _handle.promise().result();
        }

        std::coroutine_handle<promise_type> _handle;
    };

    auto operator co_await() const {
        log_info("Awaiter co_await");
        return Awaiter(m_handle);
    }

    operator std::coroutine_handle<>() const noexcept {
        return m_handle;
    }

    std::coroutine_handle<promise_type> m_handle;
};

struct Scheduler {
    struct TimerEntry {
        std::chrono::system_clock::time_point expire_time;
        std::coroutine_handle<> coroutine;

        bool operator<(TimerEntry const &that) const noexcept {
            return expire_time > that.expire_time;
        }
    };

    void add_task(std::coroutine_handle<> coroutine) {
        m_ready_queue.push_front(coroutine);
    }

    void add_timer(std::chrono::system_clock::time_point expire_time,
                   std::coroutine_handle<> coroutine) {
        m_timer_heap.push({expire_time, coroutine});
    }

    void run_loop() {
        while (!m_timer_heap.empty() || !m_ready_queue.empty()) {
            // 队列中存在任务，则弹出任务并恢复协程
            while (!m_ready_queue.empty()) {
                auto coroutine = m_ready_queue.front();
                m_ready_queue.pop_front();
                coroutine.resume(); // 进入任务协程
                log_info("m_ready_queue resume back");
            }
            // 时间堆中任务处理
            if (!m_timer_heap.empty()) {
                auto now = std::chrono::system_clock::now();
                auto timer = std::move(m_timer_heap.top());
                if (timer.expire_time < now) {
                    m_timer_heap.pop();
                    log_info("before timer resume");
                    // BUG: 协程析构后再次resume会崩溃，如何清楚析构协程
                    timer.coroutine.resume();
                } else {
                    std::this_thread::sleep_until(timer.expire_time);
                }
            }
        }
    }

    Scheduler &operator=(Scheduler &&) = delete;
    std::deque<std::coroutine_handle<>> m_ready_queue; // 存储普通协程句柄
    std::priority_queue<TimerEntry> m_timer_heap;      // 存储时间点和协程句柄
};

Scheduler &get_scheduler() {
    static Scheduler scheduler;
    return scheduler;
}

struct SleepAwaiter {
    std::chrono::system_clock::time_point m_expire_time;

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<> coroutine) const {
        get_scheduler().add_timer(m_expire_time, coroutine);
    }

    void await_resume() const noexcept {}
};

Task<void> sleep_util(std::chrono::system_clock::time_point expire_time) {
    co_await SleepAwaiter(expire_time);
    co_return;
}

Task<void> sleep_for(std::chrono::system_clock::duration duration) {
    co_await SleepAwaiter(std::chrono::system_clock::now() + duration);
    co_return;
}

struct ReturnPreviousPromise {
    auto initial_suspend() noexcept {
        return std::suspend_always();
    }

    auto final_suspend() noexcept {
        return PreviousAwaiter(m_previous);
    }

    void unhandled_exception() {
        throw;
    }

    void return_value(std::coroutine_handle<> previous) noexcept {
        m_previous = previous;
    }

    auto get_return_object() {
        return std::coroutine_handle<ReturnPreviousPromise>::from_promise(
            *this);
    }

    std::coroutine_handle<> m_previous{};
};

struct ReturnPreviousTask {
    using promise_type = ReturnPreviousPromise;

    ReturnPreviousTask(std::coroutine_handle<promise_type> coroutine) noexcept
        : m_coroutine(coroutine) {}

    ReturnPreviousTask(ReturnPreviousTask &&) = delete;

    ~ReturnPreviousTask() {
        m_coroutine.destroy();
    }

    std::coroutine_handle<promise_type> m_coroutine;
};

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
        log_info("WhenALLAwaiter await_suspend!");
        if (m_tasks.empty()) {
            return coroutine;
        }
        m_control.m_previous = coroutine;
        // 从第二个元素开始遍历容器，将任务加入到调度器
        for (auto const &t: m_tasks.subspan(1)) {
            log_info("add task!");
            get_scheduler().add_task(t.m_coroutine);
        }
        log_info("push task finish!");
        // 返回第一个任务协程句柄，因此无需将该任务加入调度器
        return m_tasks.front().m_coroutine;
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
        log_info("whenAllHelper put value");
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
    log_info("co_return nullptr");
    // TODO: 返回nullptr时协程会如何运行
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
auto whenAll(Ts &&...ts) {
    return whenAllImpl(std::make_index_sequence<sizeof...(Ts)>{},
                       std::forward<Ts>(ts)...);
}

struct WhenAnyCtlBlock {
    static constexpr std::size_t kNullIndex = std::size_t(-1);

    // 初始化为最大值，避免任务数大于计数执行错误
    std::size_t m_index{kNullIndex};
    std::coroutine_handle<> m_previous{};
    std::exception_ptr m_exception{};
};

struct WhenAnyAwaiter {
    bool await_ready() const noexcept {
        return false;
    }

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> coroutine) const {
        if (m_tasks.empty()) {
            return coroutine;
        }
        m_control.m_previous = coroutine;
        for (auto const &t: m_tasks.subspan(1)) {
            get_scheduler().add_task(t.m_coroutine);
        }
        return m_tasks.front().m_coroutine;
    }

    void await_resume() const {
        if (m_control.m_exception) [[unlikely]] {
            std::rethrow_exception(m_control.m_exception);
        }
    }

    WhenAnyCtlBlock &m_control;
    std::span<ReturnPreviousTask const> m_tasks;
};

template <class T>
ReturnPreviousTask whenAnyHelper(auto const &t, WhenAnyCtlBlock &control,
                                 Uninitialized<T> &result, std::size_t index) {
    try {
        result.put_value(co_await t);
    } catch (...) {
        control.m_exception = std::current_exception();
        co_return control.m_previous;
    }
    // 记录当前任务索引
    --control.m_index = index;
    co_return control.m_previous;
};

template <std::size_t... Is, class... Ts>
Task<std::variant<typename AwaitableTraits<Ts>::NonVoidRetType...>>
whenAnyImpl(std::index_sequence<Is...>, Ts &&...ts) {
    WhenAnyCtlBlock control{};
    std::tuple<Uninitialized<typename AwaitableTraits<Ts>::RetType>...> result;
    ReturnPreviousTask taskArray[]{
        whenAnyHelper(ts, control, std::get<Is>(result), Is)...};
    co_await WhenAnyAwaiter(control, taskArray);
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
auto whenAny(Ts &&...ts) {
    return whenAnyImpl(std::make_index_sequence<sizeof...(Ts)>{},
                       std::forward<Ts>(ts)...);
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

Task<int> hello3() {
    log_info("hello3开始睡5秒");
    co_await sleep_for(5s); // 2s 等价于 std::chrono::seconds(2)
    log_info("hello3睡醒了");
    co_return 3;
}

Task<int> hello4() {
    log_info("hello4开始睡1秒");
    co_await sleep_for(1s); // 2s 等价于 std::chrono::seconds(2)
    log_info("hello1睡醒了");
    co_return 3;
}

Task<int> hello() {
    log_info("hello 开始等1和2");
    auto v = co_await whenAny(hello4(), hello3(), hello1(), hello2());
    log_info("hello看到{}睡醒了", (int)v.index() + 1);
    co_return std::get<0>(v);
}

int main() {
    auto t = hello();
    auto t1 = hello4();
    get_scheduler().add_task(t);
    get_scheduler().run_loop();
    log_info("主函数中得到hello结果: {}", t.m_handle.promise().result());
    return 0;
}
