#pragma once

#include <string.h>
#include <assert.h>
#include <spdlog/spdlog.h>
#include "util.h"

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#   define AMOT_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define AMOT_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define AMOT_LIKELY(x)      (x)
#   define AMOT_UNLIKELY(x)      (x)
#endif

/// 断言宏封装
#define AMOT_ASSERT(x) \
    if(AMOT_UNLIKELY(!(x))) { \
    spdlog::error("ASSERTION: {}\nbacktrace:\n{}", #x, \
        amot::BacktraceToString(100, 2, "    ")); \
        assert(x); \
    }

/// 断言宏封装
#define AMOT_ASSERT2(x, w) \
    if(AMOT_UNLIKELY(!(x))) { \
    spdlog::error("ASSERTION: {}\n{}\nbacktrace:\n{}", #x, w, \
        amot::BacktraceToString(100, 2, "    ")); \
        assert(x); \
    }
