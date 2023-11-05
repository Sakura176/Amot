#include <iostream>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>

#include "amot/common/task.h"
#include "amot/common/executor.h"
#include "amot/common/scheduler.h"
#include "amot/common/channel.h"

using namespace std::chrono_literals;

amot::Task<void, amot::LooperExecutor> test_cor1() {
	spdlog::info("test_cor1 begin");
	spdlog::info("before test_cor1 await");
	co_await 1s;
	spdlog::info("after test_cor1 await");
	spdlog::info("test_cor1 end");
}

amot::Task<void, amot::LooperExecutor> test_cor2() {
	spdlog::info("test_cor2 begin");
	co_await 1s;
	spdlog::info("test_cor2 end");
}

void test_cor3() {
	spdlog::info("test_cor3 begin");
	spdlog::info("test_cor3 end");
}

int main() {
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] : %v");
	spdlog::info("main begin");

	auto scheduler = amot::Scheduler();

	scheduler.execute(test_cor3, 1);

	scheduler.shutdown();
	scheduler.join();

	spdlog::info("main end");
	return 0;
}