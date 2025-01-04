#include "amot/utils/log.hpp"
#include "test/unittest.h"
#include <fstream>
#include <future>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
#include <string>
#include <utility>

namespace amot {
class LogTest : public FUTURE_TESTBASE {
public:
    void caseSetUp() override {}

    void caseTearDown() override {}

    std::pair<std::string, std::string> get_log_level_name(log_level level) {
        return std::make_pair("[" + details::log_level_name(level) + "] ",
                              "\x1B[m");
    }

    std::string extract(log_level const &level, std::string const &log_line) {
        auto val = get_log_level_name(level);
        std::cout << val.first << std::endl;
        size_t pos = log_line.find(val.first);
        size_t end = log_line.find(val.second);
        std::cout << "pos: " << pos << " - end: " << end << std::endl;
        if (pos != std::string::npos) {
            // 从"info "的后面开始截取，去掉中括号
            size_t start = pos + val.first.length();
            if (end == std::string::npos) {
                end = log_line.length(); // 如果没有换行符，取到字符串末尾
            }
            return log_line.substr(start, end - start); // 提取子字符串
        }
        return ""; // 如果没有找到，返回空字符串
    }
};

TEST_F(LogTest, TestBase) {
    set_log_level(log_level::debug);

    testing::internal::CaptureStdout();
    log_info("test log_info");
    std::string output =
        extract(log_level::info, testing::internal::GetCapturedStdout());
    EXPECT_EQ(output, "test log_info");
    output.clear();

    testing::internal::CaptureStdout();
    log_warn("test log_warn");
    output = extract(log_level::warn, testing::internal::GetCapturedStdout());
    EXPECT_EQ(output, "test log_warn");

    testing::internal::CaptureStdout();
    log_debug("test log_debug");
    output = extract(log_level::debug, testing::internal::GetCapturedStdout());
    EXPECT_EQ(output, "test log_debug");

    testing::internal::CaptureStdout();
    log_error("test log_error");
    output = extract(log_level::error, testing::internal::GetCapturedStdout());
    EXPECT_EQ(output, "test log_error");
}

TEST_F(LogTest, TestLogLevel) {
    testing::internal::CaptureStdout();
    set_log_level(log_level::info);
    log_debug("test log_debug");
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "");
}

TEST_F(LogTest, TestLogConsoleEnable) {
    testing::internal::CaptureStdout();
    set_log_level(log_level::info);
    set_console_enable(false);
    log_info("test log_debug");
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "");
}

TEST_F(LogTest, TestLogFile) {
    auto log_path = "./test_log.log";
    set_log_file(log_path);
    set_log_level(log_level::info);
    log_info("test log_info");
    log_debug("test log_debug");

    std::ifstream ifs(log_path);
    if (!ifs) {
        std::cerr << "can not open file!" << std::endl;
        return;
    }
    std::string line;
    while (std::getline(ifs, line)) {
        EXPECT_EQ("test log_info", extract(log_level::info, line));
    }
}

TEST_F(LogTest, TestLogThreadPrint) {
    auto log_path = "./test_log.log";
    set_log_file(log_path);
    set_log_level(log_level::debug);
    std::vector<std::future<void>> futures;

    int const threads = 2;
    int const per_thread_nums = 2;
    for (int i = 0; i < threads; ++i) {
        futures.emplace_back(std::async(std::launch::async, [i]() {
            for (int j = 0; j < per_thread_nums; ++j) {
                log_info("Thread {} - Message {}", i, j);
            }
        }));
    }

    std::ifstream ifs(log_path);
    if (!ifs) {
        std::cerr << "can not open file!" << std::endl;
        return;
    } // 验证日志文件的内容
    std::ifstream log_file(log_path);
    std::stringstream buffer;
    buffer << log_file.rdbuf();
    std::string log_content = buffer.str();
    // 检查日志内容是否包含所有线程的消息
    for (int i = 0; i < threads; ++i) {
        for (int j = 0; j < per_thread_nums; ++j) {
            std::string expected_message = "Thread " + std::to_string(i) +
                                           " - Message " + std::to_string(j);
            log_debug("{}", expected_message);
            EXPECT_TRUE(log_content.find(expected_message) !=
                        std::string::npos);
        }
    }
}
} // namespace amot
