#include "amot/common/util.h"
#include "amot/coroutine/executor.h"
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace amot {

class EpollExecutor : public AbstractExecutor {
public:
    EpollExecutor() {
        spdlog::trace("EpollExecutor begin!!!");
        m_efd = CheckError(epoll_create1(0));
    }

    void execute(std::function<void()> &&func) override {
        spdlog::trace("EpollExecutor execute!!!");
        struct epoll_event event;
        // TODO 应传入promise，考虑替换方案
        event.events = 1;
        event.data.ptr = nullptr;
        // TODO 第三个参数待考虑
        CheckError(epoll_ctl(m_efd, EPOLL_CTL_ADD, 0, &event));
        spdlog::trace("EpollExecutor execute finish");
    }

    ~EpollExecutor() {
        close(m_efd);
    }

private:
    void run_loop() {
        spdlog::trace("EpollExecutor run_loop!!!");
        // TODO 需增加结束条件
        while (true) {
            int res = CheckError(epoll_wait(m_efd, m_ebuf, 10, -1));
            for (int i = 0; i < res; ++i) {
                struct epoll_event &event = m_ebuf[i];
                CheckError(epoll_ctl(m_efd, EPOLL_CTL_DEL, 0, NULL));
            }
        }
    }

private:
    int m_efd;
    struct epoll_event m_ebuf[10];
};
} // namespace amot
