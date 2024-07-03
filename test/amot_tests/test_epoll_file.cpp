#include "amot/common/log.h"
#include <fcntl.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
auto logger = amot::LoggerMgr::GetInstance()->GetRoot();

int main(int argc, char *argv[]) {
    logger->set_level(spdlog::level::debug);
    // 把0号输入流设为非阻塞的，这样当read时如果没有数据，会直接返回EWOULDBLOCK错误
    int flags = fcntl(0, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(0, F_SETFL);

    // 创建异步控制器
    int epfd = epoll_create1(0);

    // 创建异步监听器（目标：0号输入流，事件：有输入数据）
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = 0;
    epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &event);

    // 开始异步读取输入流
    while (true) {
        struct epoll_event ebuf[10];
        int res = epoll_wait(epfd, ebuf, 10, 1000);
        if (res == -1) {
            logger->debug("epoll出错了：{}", strerror(errno));
        }
        if (res == 0) {
            logger->debug("epoll超时了，1秒内没有等到任何输入");
        }
        for (int i = 0; i < res; i++) {
            logger->debug("等到了输入事件！");
            int fd = ebuf[i].data.fd;
            char c;
            while (true) {
                int len = read(fd, &c, 1);
                if (len <= 0) { // 表示需要阻塞了
                    if (errno == EWOULDBLOCK) {
                        logger->debug("read: 前面的区域，以后再来探索8～");
                        break;
                    }
                    logger->debug("read出错了 {}", strerror(errno));
                }
                logger->debug(c);
            }
        }
    }

    return 0;
}
