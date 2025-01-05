#include "co_async/scheduler.hpp"
#include "co_async/task.hpp"
#include "co_async/when_all.hpp"
#include "test/unittest.h"
#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace amot;

class WhenAllTest : public FUTURE_TESTBASE {
public:
    void caseSetUp() override {}

    void caseTearDown() override {}

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

    Task<int> hello() {
        log_info("hello 开始等1和2");
        auto v = co_await when_all(hello1(), hello2(), hello3());
        // log_info("hello看到{}睡醒了", (int)v.index() + 1);
        // BUG: 该处只能取首个元素，当when_any中执行的非首个元素时会崩溃
        co_return std::get<0>(v);
    }
};

TEST_F(WhenAllTest, when_all) {
    auto t = hello();
    auto t1 = hello1();
    get_scheduler().run(t);
    get_scheduler().run(t1);
    EXPECT_EQ(1, t.m_handle.promise().result());
}
