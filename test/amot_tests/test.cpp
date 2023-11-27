#include "amot/common/threadPool.h"
#include "amot/common/log.h"

auto logger = amot::LoggerMgr::GetInstance()->GetFileLogger("system", "./logs/system.log");

void ThreadLogTask(int i, int cnt) {
    for(int j = 0; j < 1000; j++ ){
        logger->info("PID:[{}]======= {} ========= ", gettid(), cnt++);
    }
}

void TestThreadPool() {
	
    amot::ThreadPool threadpool;
    for(int i = 0; i < 3; i++) {
        threadpool.scheduleById(std::bind(ThreadLogTask, i % 4, i * 10000));
    }
    getchar();
}

int main() {
	TestThreadPool();
}