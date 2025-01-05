#include "amot/utils/log.hpp"
#include <cstring>
#include <sys/epoll.h>
#include <sys/ioctl.h>

void epoll_read() {
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
        if (res == 0) {
            log_error("epoll wait timeout");
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
            }
        }
    }
}

void normal_read() {
    int fd = 0;
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
        for (int i = 0; i < 1000; ++i) {
            for (int j = 0; j < 1000; ++j) {
                int k = i + j;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    normal_read();
    return 0;
}
