#pragma once
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <source_location>
#include <thread>

#define LOG_FOREACH_LOG_LEVEL(f) \
    f(trace) f(debug) f(info) f(critical) f(warn) f(error) f(fatal)

enum class log_level : std::uint8_t {
#define _FUNCTION(name) name,
    LOG_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
};

namespace details {

#if defined(__linux__) || defined(__APPLE__)
inline constexpr char
    k_level_ansi_colors[(std::uint8_t)log_level::fatal + 1][8] = {
        "\E[37m", "\E[35m", "\E[32m", "\E[34m", "\E[33m", "\E[31m", "\E[31;1m",
};

inline constexpr char k_reset_ansi_color[4] = "\E[m";
# define _LOG_IF_HAS_ANSI_COLORS(x) x
#else
# define _LOG_IF_HAS_ANSI_COLORS(x)
inline constexpr char l_level_ansi_colors[(std::uint8_t)log_level::fatal = 1]
                                         [1] = {
                                             "", "", "", "", "", "", "",
};
inline constexpr char k_reset_ansi_color[1] = "";
#endif

inline std::string log_level_name(log_level level) {
    switch (level) {
#define _FUNCTION(name) \
    case log_level::name: return #name;
        LOG_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
    }
    return "unknown";
}

inline log_level log_level_from_name(std::string level) {
#define _FUNCTION(name) \
    if (level == #name) \
        return log_level::name;
    LOG_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
    return log_level::info;
}

template <class T>
struct with_source_location {
private:
    T inner;
    std::source_location loc;

public:
    template <class U>
        requires std::constructible_from<T, U>
    consteval with_source_location(
        U &&inner, std::source_location loc = std::source_location::current())
        : inner(std::forward<U>(inner)),
          loc(std::move(loc)) {}

    constexpr T const &format() const {
        return inner;
    }

    constexpr std::source_location const &location() const {
        return loc;
    }
};

inline log_level g_max_level = []() -> log_level {
    if (auto level = std::getenv("LOG_LEVEL")) {
        return details::log_level_from_name(level);
    }
#ifdef NDEBUG
    return log_level::info;
#else
    return log_level::debug;
#endif
}();

inline std::ofstream g_log_file = []() -> std::ofstream {
    if (auto path = std::getenv("LOG_FILE")) {
        return std::ofstream(path, std::ios::app);
    }
    return std::ofstream();
}();

inline void output_log(log_level level, std::string msg,
                       std::source_location const &loc) {
    std::chrono::zoned_time now{std::chrono::current_zone(),
                                std::chrono::high_resolution_clock::now()};
    size_t thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
    // TODO: 考虑如何传入当前的工程目录
    std::string file_path(loc.file_name());
    msg = std::format("[{:%Y-%m-%d %H:%M:%S}] [T: {:06}] {}:{} [{}] {}", now,
                      thread_id % 100000, file_path.substr(14), loc.line(),
                      details::log_level_name(level), msg);
    if (g_log_file) {
        g_log_file << msg + '\n';
    }
    if (level >= g_max_level) {
        std::cout << _LOG_IF_HAS_ANSI_COLORS(
                         k_level_ansi_colors[(std::uint8_t)level] +)
                             msg _LOG_IF_HAS_ANSI_COLORS(+k_reset_ansi_color) +
                         '\n';
    }
}

} // namespace details

inline void set_log_file(std::string path) {
    details::g_log_file = std::ofstream(path, std::ios::app);
}

inline void set_log_level(log_level level) {
    details::g_max_level = level;
}

template <typename... Args>
void generic_log(log_level level,
                 details::with_source_location<std::format_string<Args...>> fmt,
                 Args &&...args) {
    auto const &loc = fmt.location();
    auto msg = std::vformat(fmt.format().get(), std::make_format_args(args...));
    details::output_log(level, std::move(msg), loc);
}

#define _FUNCTION(name) \
    template <typename... Args> \
    void log_##name( \
        details::with_source_location<std::format_string<Args...>> fmt, \
        Args &&...args) { \
        return generic_log(log_level::name, std::move(fmt), \
                           std::forward<Args>(args)...); \
    }
LOG_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION

#define LOG_P(x) ::log::log_debug(#x "={}", x)
