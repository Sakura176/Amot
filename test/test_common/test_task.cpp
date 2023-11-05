#include <iostream>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>

#include "amot/common/task.h"
#include "amot/common/executor.h"
#include "amot/common/scheduler.h"
#include "amot/common/channel.h"

using namespace std::chrono_literals;

amot::Task<void, amot::LooperExecutor> Producer(amot::Channel<int> &channel) {
	int i = 0;
	while (i < 10) {
		spdlog::info("send: {}", i);
		//    co_await channel.write(i++);
		co_await (channel << i++);
		co_await 50ms;
	}

	co_await 5s;
	channel.close();
	spdlog::info("close channel, exit.");
}

amot::Task<void, amot::LooperExecutor> Consumer(amot::Channel<int> &channel) {
	while (channel.is_active()) {
		try {
			//	auto received = co_await channel.read();
			int received;
			co_await (channel >> received);
			spdlog::info("receive: {}", received);
			co_await 500ms;
		} catch (std::exception &e) {
			spdlog::debug("exception: {}", e.what());
		}
	}

	spdlog::info("exit.");
}

amot::Task<void, amot::LooperExecutor> Consumer2(amot::Channel<int> &channel) {
	while (channel.is_active()) {
		try {
			auto received = co_await channel.read();
			spdlog::info("receive2: {}", received);
			co_await 300ms;
		} catch (std::exception &e) {
			spdlog::debug("exception2: {}", e.what());
		}
	}

	spdlog::info("exit.");
}

void test_channel() {
	auto channel = amot::Channel<int>(5);
	auto producer = Producer(channel);
	auto consumer = Consumer(channel);
	auto consumer2 = Consumer2(channel);

	std::this_thread::sleep_for(10s);
}

void test_scheduler() {
	auto scheduler = amot::Scheduler();

	spdlog::info("scheduler test begin ...");
	
	scheduler.execute([]() { spdlog::info("2"); }, 100);
	scheduler.execute([]() { spdlog::info("1"); }, 50);
	scheduler.execute([]() { spdlog::info("6"); }, 1000);
	scheduler.execute([]() { spdlog::info("5"); }, 500);
	scheduler.execute([]() { spdlog::info("3"); }, 200);
	scheduler.execute([]() { spdlog::info("4"); }, 300);

	scheduler.shutdown();
	scheduler.join();
}

amot::Task<int, amot::AsyncExecutor> simple_task2() {
	spdlog::info("task 2 start ...");
	using namespace std::chrono_literals;
	co_await 1s;
	spdlog::info("task 2 returns after 1s.");
	co_return 2;
}

amot::Task<int, amot::NewThreadExecutor> simple_task3() {
	spdlog::info("in task 3 start ...");
	using namespace std::chrono_literals;
	co_await 2s;
	spdlog::info("task 3 returns after 2s.");
	co_return 3;
}

amot::Task<int, amot::LooperExecutor> simple_task() {
	spdlog::info("task start ...");
	auto result2 = co_await simple_task2();
	spdlog::info("returns from task2: ", result2);
	auto result3 = co_await simple_task3();
	spdlog::info("returns from task3: ", result3);
	co_return 1 + result2 + result3;
}

void test_tasks() {
	auto simpleTask = simple_task();
	simpleTask.then([](int i) {
		spdlog::info("simple task end: {}", i);
	}).catching([](std::exception &e) {
		spdlog::info("error occurred {}", e.what());
	});
	try {
		auto i = simpleTask.get_result();
		spdlog::info("simple task end from get: {}", i);
	} catch (std::exception &e) {
		spdlog::info("error: {}", e.what());
	}
}

// amot::Task<void> test_void() {
// 	spdlog::info("test_void 1");
// 	co_yield void;
// 	spdlog::info("test_void 2");
// }

amot::Task<int> fibonacci() {
	co_yield 0;
	co_yield 1;

	int a = 0;
	int b = 1;
	while(true) {
		co_yield a + b;
		b = a + b;
		a = b - a;
	}
}

void test_fibonacci() {
	auto fib = fibonacci();
	spdlog::info("fib : {}", fib.get_result());
	fib.resume();
	spdlog::info("fib : {}", fib.get_result());
}

int main() {
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] : %v");
	spdlog::set_level(spdlog::level::debug);
	spdlog::info("================== fibonacci test begin ==================");
	test_fibonacci();
	spdlog::info("=================== fibonacci test end ===================");

	// spdlog::info("=================== task test begin ===================");
	// test_tasks();
	// spdlog::info("==================== task test end ====================");

	// spdlog::info("================== scheduler test begin ==================");
	// test_scheduler();
	// spdlog::info("=================== scheduler test end ===================");

	// spdlog::info("================== channel test begin ==================");
	// test_channel();
	// spdlog::info("=================== channel test end ===================");
	return 0;
}