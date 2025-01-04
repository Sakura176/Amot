#include "amot/coroutine/task.h"
#include "amot/utils/log.hpp"
using namespace amot;

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
    getLoop().addTask(t1);
    getLoop().addTask(t2);
    getLoop().runAll();
    log_info("主函数中得到hello1结果: {}", t1.mCoroutine.promise().result());
    log_info("主函数中得到hello2结果: {}", t2.mCoroutine.promise().result());
    return 0;
}
