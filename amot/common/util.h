#pragma once

#include "singleton.h"
#include <cxxabi.h>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <vector>

namespace amot {
/**
 * @brief 返回当前线程的ID
 */
long GetThreadId();

uint32_t GetFiberId();

uint64_t GetCurrentMS();

void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);
std::string BacktraceToString(int size = 64, int skip = 2,
                              std::string const &prefix = "");

template <class T>
char const *TypeToName() {
    static char const *s_name =
        abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}

class Nocopyable {
public:
    Nocopyable() = default;

    ~Nocopyable() = default;

    Nocopyable(Nocopyable const &) = delete;

    Nocopyable &operator=(Nocopyable const &) = delete;
};

auto CheckError(int res) {
    if (res == -1) [[unlikely]] {
        throw std::system_error(errno, std::system_category());
    }
    return res;
}

} // namespace amot
