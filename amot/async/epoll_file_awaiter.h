#include "amot/coroutine/awaiter.h"
#include <cstdint>
#include <spdlog/spdlog.h>

namespace amot {

struct EpollFile {
    int file_no;
    uint32_t events;

    EpollFile() = default;

    EpollFile(int _file_no, uint32_t _events)
        : file_no(_file_no),
          events(_events) {}
};

template <typename R>
struct EpollFileAwaiter : public Awaiter<R> {
public:
    EpollFileAwaiter(EpollFile &epoll_file) : m_epoll_file(epoll_file) {
        spdlog::trace("EpollFileAwaiter begin!!!");
    }

    void after_suspend() override {
        spdlog::trace("EpollFileAwaiter after_suspend!!!");
        // TODO 将m_epoll_file内的值传递到executor中
    }

    void before_resume() override {
        spdlog::trace("EpollFileAwaiter before_resume!!!");
    }

private:
    EpollFile m_epoll_file;
};
} // namespace amot
