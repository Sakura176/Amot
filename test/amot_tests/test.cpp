#include "amot/common/log.h"
#include "amot/common/threadPool.h"
#include <spdlog/spdlog.h>

void ThreadLogTask(int i, int cnt) {
    for (int j = 0; j < 1000; j++) {
        spdlog::info("PID:[{}]======= {} ========= ", gettid(), cnt++);
    }
}

void TestThreadPool() {
    amot::ThreadPool threadpool;
    for (int i = 0; i < 3; i++) {
        threadpool.scheduleById(std::bind(ThreadLogTask, i % 4, i * 10000));
    }
    getchar();
}

int main() {
    TestThreadPool();
}
