#include "amot/coroutine/channel.h"
#include "amot/coroutine/executor.h"
#include "amot/coroutine/task.h"
#include <spdlog/spdlog.h>
#include <string>
#include <unistd.h>

using namespace amot;
using namespace std::chrono_literals;

Task<void> writer(Channel<std::string> &channel) {
    while (channel.is_active()) {
        //    co_await channel.write(i++);
        int co_await (channel << i++);
        co_await 50ms;
    }

    co_await 5s;
    channel.close();
    spdlog::info("close channel, exit.");
}

Task<void> reader(Channel<std::string> &channel) {
    while (channel.is_active()) {
        try {
            //	auto received = co_await channel.read();
            std::string received;
            co_await (channel >> received);
            spdlog::info("receive: {}", received);
            co_await 500ms;
        } catch (std::exception &e) {
            spdlog::debug("exception: {}", e.what());
        }
    }
    spdlog::info("reader channel exit");
}

Task<void> async_main() {
    co_return;
}

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::trace);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] : %v");
    spdlog::info("co_async step12 begin!!!");
    int attr = 1;
    ioctl(0, FIONBIO, &attr);
    auto func = async_main();
    func.then([]() { spdlog::info("async_main running succeed!"); })
        .catching([](std::exception &e) {
            spdlog::info("error occurred {}", e.what());
        });
    try {
        func.get_result();
        spdlog::info("async_main task end");
    } catch (std::exception &e) {
        spdlog::info("error: {}", e.what());
    }

    return 0;
}
