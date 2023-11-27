#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <future>

#include <spdlog/spdlog.h>
#include "amot/coroutine/task.h"

using namespace amot;

using Texts = std::vector<std::string>;

Task<Texts, amot::AsyncExecutor> ReadFile(const std::string &filename) {
	Texts result;
	std::ifstream infile(filename);
	std::string line;
	while(std::getline(infile, line)) {
		result.emplace_back(line);
	}
	co_return result;
}

Task<int, amot::AsyncExecutor> CountLineChar(const std::string &line, char c) {
	//spdlog::info("CountLineChar");
	co_return std::count(line.begin(), line.end(), c);
}

Task<int, amot::AsyncExecutor> CountTextChar(const Texts &content, char c) {
	//spdlog::info("CountTextChar");
	int result = 0;
	for (const auto &line : content)
		result += co_await CountLineChar(line, c);
	co_return result;
}

Task<int, amot::AsyncExecutor> CountFileCharNum(const std::string &filename, char c) {
	//spdlog::info("CountFileCharNum");
	Texts contents = co_await ReadFile(filename);
	co_return co_await CountTextChar(contents, c);
}

int CountFileCharNumBlock(const std::string &filename, char c) {
	
	Texts result;
	std::ifstream infile(filename);
	std::string line;
	while(std::getline(infile, line)) {
		result.emplace_back(line);
	}

	int num = 0;
	for (auto& line : result) {
		num += std::count(line.begin(), line.end(), c);
	}
	return num;
}

int main() {
	auto filepath = "./bin/file.txt";

	spdlog::info("*************** begin test char counter *************");
	auto countFileCharNum = CountFileCharNum(filepath, 'x');
	countFileCharNum.then([](int i) {
		//spdlog::info("simple task end: {}", i);
	}).catching([](std::exception &e) {
		//spdlog::info("error occurred {}", e.what());
	});
	try {
		auto i = countFileCharNum.get_result();
		spdlog::info("The number of {} in {} is {}", 'x', filepath, i);
	} catch (std::exception &e) {
		spdlog::info("error: {}", e.what());
	}
	spdlog::info("*************** end test char counter *************");

	spdlog::info("*************** begin test block char counter *************");
	int num = CountFileCharNumBlock(filepath, 'x');
	spdlog::info("The number of {} in {} is {}", 'x', filepath, num);
	spdlog::info("*************** end test block char counter *************");
}