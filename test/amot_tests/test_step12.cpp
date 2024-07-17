#include "amot/async/epoll_executor.h"
#include "amot/coroutine/executor.h"
#include "amot/coroutine/promise.h"
#include "amot/coroutine/task.h"
#include <asm-generic/ioctls.h>
#include <cerrno>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <spdlog/tweakme.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <system_error>
#include <unistd.h>

using namespace amot;

Task<std::string> reader() {
    SPDLOG_INFO("async_main reader begin!!!");
    // TODO 完善下述步骤
    // 1. 传入参数，文件句柄的epoll事件
    // 2. 数据写入到awaiter中
    // 3. 数据传递到executor中进行注册并监听
    // co_await EpollFile(0, EPOLLIN);
    std::string s = "test task";
    SPDLOG_INFO("reader string writed!!!");
    // while (true) {
    //     char c;
    //     ssize_t len = read(0, &c, 1);
    //     if (len == 1) {
    //         if (errno != EWOULDBLOCK) [[unlikely]] {
    //             throw std::system_error(errno, std::system_category());
    //         }
    //         break;
    //     }
    //     s.push_back(c);
    // }
    co_return s;
}

Task<void> async_main() {
    SPDLOG_INFO("async_main begin!!!");
    while (true) {
        SPDLOG_INFO("async_main while begin!!!");
        auto s = co_await reader();
        SPDLOG_INFO("succeed read: {}", s);
        if (s == "quit\n") {
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    spdlog::set_pattern(
        "[%Y-%m-%d %H:%M:%S.%e] [%s:%#] [%^%l%$] [thread %t] : %v");
    SPDLOG_INFO("co_async step12 begin!!!");
    int attr = 1;
    ioctl(0, FIONBIO, &attr);
    auto func = async_main();
    func.then([]() { SPDLOG_INFO("async_main running succeed!"); })
        .catching([](std::exception &e) {
            SPDLOG_INFO("error occurred {}", e.what());
        });
    try {
        func.get_result();
        SPDLOG_INFO("async_main task end");
    } catch (std::exception &e) {
        SPDLOG_INFO("error: {}", e.what());
    }

    return 0;
}
