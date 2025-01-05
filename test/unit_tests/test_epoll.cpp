#include "co_async/scheduler.hpp"
#include "co_async/task.hpp"
#include "co_async/when_all.hpp"
#include "test/unittest.h"
#include "utils/log.hpp"
#include <cerrno>
#include <cstring>
#include <gtest/gtest.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>

using namespace std::chrono_literals;
using namespace amot;

class EpollTest : public FUTURE_TESTBASE {
public:
    void caseSetUp() override {}

    void caseTearDown() override {}
};

TEST_F(EpollTest, async_read) {
    int attr = 1;
    ioctl(0, FIONBIO, &attr);

    int epfd = epoll_create1(0);

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = 0;
    epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &event);

    while (true) {
        struct epoll_event ebuf[10];
        int res = epoll_wait(epfd, ebuf, 10, 1000);
        if (res == -1) {
            log_error("epoll error:{}", strerror(errno));
        }
        for (int i = 0; i < res; ++i) {
            log_debug("input event");
            int fd = ebuf[i].data.fd;
            char c;
            while (true) {
                int len = read(fd, &c, 1);
                if (len <= 0) {
                    if (errno == EWOULDBLOCK) {
                        log_warn("read err: EWOULDBLOCK");
                        break;
                    }
                    log_debug("read error: {}", strerror(errno));
                }
                log_info("read char: {}", c);
                EXPECT_EQ('h', c);
            }
        }
    }
}
