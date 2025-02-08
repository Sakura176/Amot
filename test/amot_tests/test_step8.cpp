#include "amot/co_async/scheduler.hpp"
#include "amot/co_async/task.hpp"
#include "amot/co_async/when_any.hpp"
#include "amot/utils/expected.hpp"
#include <cerrno>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <system_error>

using namespace std::chrono_literals;

namespace amot {

struct EpollFilePromise : Promise<void> {
    auto get_return_object() {
        return std::coroutine_handle<EpollFilePromise>::from_promise(*this);
    }

    EpollFilePromise &operator=(EpollFilePromise &&) = delete;

    inline ~EpollFilePromise();

    struct EpollLoop *mLoop;
    int mFileNo;
    uint32_t mEvents;
};

struct EpollLoop {
    void addListener(EpollFilePromise &promise) {
        struct epoll_event event;
        event.events = promise.mEvents;
        event.data.ptr = &promise;
        convert_error(epoll_ctl(mEpoll, EPOLL_CTL_ADD, promise.mFileNo, &event))
            .expect("epoll_ctl->EPOLL_CTL_ADD");
    }

    void removeListener(int fileNo) {
        convert_error(epoll_ctl(mEpoll, EPOLL_CTL_DEL, fileNo, NULL))
            .expect("epoll_ctl->EPOLL_CTL_DEL");
    }

    void tryRun(int timeout) {
        int res = convert_error(epoll_wait(mEpoll, mEventBuf,
                                           std::size(mEventBuf), timeout))
                      .expect("epoll_wait");
        for (int i = 0; i < res; ++i) {
            auto &event = mEventBuf[i];
            auto &promise = *(EpollFilePromise *)event.data.ptr;
            std::coroutine_handle<EpollFilePromise>::from_promise(promise)
                .resume();
        }
    }

    EpollLoop &operator=(EpollLoop &&) = delete;

    ~EpollLoop() {
        close(mEpoll);
    }

    int mEpoll = convert_error(epoll_create1(0)).expect("epoll_create1");
    struct epoll_event mEventBuf[64];
};

EpollFilePromise::~EpollFilePromise() {
    if (mLoop) {
        mLoop->removeListener(mFileNo);
    }
}

struct EpollFileAwaiter {
    bool await_ready() const noexcept {
        return false;
    }

    void
    await_suspend(std::coroutine_handle<EpollFilePromise> coroutine) const {
        auto &promise = coroutine.promise();
        promise.mLoop = &mLoop;
        promise.mFileNo = mFileNo;
        promise.mEvents = mEvents;
        mLoop.addListener(promise);
    }

    void await_resume() const noexcept {}

    EpollLoop &mLoop;
    int mFileNo;
    uint32_t mEvents;
};

inline Task<void, EpollFilePromise> wait_file(EpollLoop &loop, int fileNo,
                                              uint32_t events) {
    co_await EpollFileAwaiter(loop, fileNo, events | EPOLLONESHOT);
}
} // namespace amot

amot::EpollLoop loop;
amot::Scheduler timerLoop;

amot::Task<std::string> reader(int fileNo) {
    co_await wait_file(loop, 0, EPOLLIN);
    std::string s;
    size_t chunk = 8;
    while (true) {
        size_t exist = s.size();
        s.resize(exist + chunk);
        ssize_t len = read(fileNo, s.data() + exist, chunk);
        if (len == -1) {
            if (errno != EWOULDBLOCK) [[unlikely]] {
                throw std::system_error(errno, std::system_category());
            }
        }
        if (len != chunk) {
            s.resize(exist + len);
            break;
        }
        if (chunk < 65536) {
            chunk *= 4;
        }
    }
    co_return s;
}

amot::Task<void> async_main() {
    SPDLOG_DEBUG("async_main");
    int file = amot::convert_error(
                   open("home/yc/Amot/CMakeLists.txt", O_RDONLY | O_NONBLOCK))
                   .expect("open");
    SPDLOG_DEBUG("read file no is {}", file);
    while (true) {
        auto v = co_await amot::when_any(reader(STDIN_FILENO), reader(file));
        std::string s;
        std::visit([&](std::string const &v) { s = v; }, v);
        spdlog::info("read {}", s);
        if (s == "quit\n") {
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::debug);
    int attr = 1;
    ioctl(0, FIONBIO, &attr);

    auto t = async_main();
    t.m_handle.resume();
    while (!t.m_handle.done()) {
        if (auto delay = timerLoop.run()) {
            auto ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(*delay)
                    .count();
            loop.tryRun(ms);
        } else {
            loop.tryRun(-1);
        }
    }
    return 0;
}
