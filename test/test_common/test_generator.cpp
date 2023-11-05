#include <iostream>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>

#include "test/generator/generator.h"

amot::Generator<int> fibonacci() {
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

void test_fib() {
	// if (has_next()) {
	// 	spdlog::info("fib : {}", fibonacci().next());
	// }
	auto generator = fibonacci();
	spdlog::info("fib : {}", generator.next());
	spdlog::info("fib : {}", generator.next());
	spdlog::info("fib : {}", generator.next());
	spdlog::info("fib : {}", generator.next());
}

void test_array1() {
	std::string array[] = {"hello", "world", "C++", "coroutine"};
	auto generator = amot::Generator<std::string>::from_array(array, 4);
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
}

void test_array2() {
	std::list<std::string> array = {"hello", "world", "C++", "coroutine"};
	auto generator = amot::Generator<std::string>::from_array(array);
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
}

void test_list1() {
	auto generator = amot::Generator<std::string>::from(
			{"hello", "world", "C++", "coroutine"});
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
}

void test_list2() {
	auto generator = amot::Generator<std::string>::from(
			"hello", "world", "C++", "coroutine");
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
	spdlog::debug("fib : {}", generator.next());
}

void test_array_nocor() {
	std::string array[] = {"hello", "world", "C++", "coroutine"};
	for (int i = 0; i < 4; ++i) {
		spdlog::debug("fib : {}", array[i]);
	}
}

int main() {
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] : %v");
	spdlog::set_level(spdlog::level::debug);
	spdlog::info("================ test fibonacci ================");
	for (int i = 0; i < 1; ++i) {
		test_list2();
	}
	spdlog::info("================ test fibonacci ================");
	return 0;
}