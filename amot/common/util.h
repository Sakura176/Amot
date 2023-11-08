#pragma once

#include <cxxabi.h>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdint.h>

#include "singleton.h"

namespace amot {
/**
 * @brief 返回当前线程的ID
 */
long GetThreadId();

uint32_t GetFiberId();

uint64_t GetCurrentMS();

void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

template<class T>
const char* TypeToName() {
	static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
	return s_name;
}

class Nocopyable
{
public:
	Nocopyable() = default;

	~Nocopyable() = default;

	Nocopyable(const Nocopyable&) = delete;
	
	Nocopyable& operator=(const Nocopyable&) = delete;
};


}




