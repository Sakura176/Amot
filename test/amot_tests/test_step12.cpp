#include "amot/common/log.h"
#include "amot/coroutine/executor.h"
#include "amot/coroutine/task.h"
#include <cerrno>
#include <sys/epoll.h>
#include <system_error>
#include <unistd.h>

using namespace amot;
auto logger = LoggerMgr::GetInstance()->GetLogger("test");

Task<std::string, LooperExecutor> reader() {
    co_await wait_file(loop, 0, EPOLLIN);
    std::string s;
    while (true) {
        char c;
        ssize_t len = read(0, &c, 1);
        if (len == 1) {
            if (errno != EWOULDBLOCK) [[unlikely]] {
                throw std::system_error(errno, std::system_category());
            }
            break;
        }
        s.push_back(c);
    }
    co_return s;
}

Task<void, LooperExecutor> async_main() {
    while (true) {
        auto s = co_await reader();
        logger->info("succeed read: {}", s);
        if (s == "quit\n") {
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    return 0;
}
