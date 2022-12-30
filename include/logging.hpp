#pragma once

#include "time.hpp"

#include <array>
#include <fmt/core.h>

enum class LogSeverity {
    Crit = 70,  // serious errors (data corruption, need reboot)
    Error = 60, // other non-fatal errors: something's broke
    Warn = 50,  // warnings: stuff is still working
    Stat = 40,  // mud status: players login, deaths, etc
    Debug = 30, // coding debug/script errors
    Info = 20,  // monotonous info: zone resets, etc
    Trace = 10, // trace info: hard core debug maybe?
};

constexpr std::array log_severities = {
    "fine",        /* 10 */
    "informative", /* 20 */
    "debug",       /* 30 */
    "status",      /* 40 */
    "warning",     /* 50 */
    "error",       /* 60 */
    "critical",    /* 70 */
};

void log(LogSeverity severity, int level, std::string_view str);
inline void log(std::string_view str) { log(LogSeverity::Info, 0, str); }
template <typename... Args> void log(LogSeverity severity, int level, fmt::string_view str, Args &&...args) {
    log(severity, level, fmt::vformat(str, fmt::make_format_args(args...)));
}
template <typename... Args> void log(fmt::string_view str, Args &&...args) {
    log(LogSeverity::Info, 0, fmt::vformat(str, fmt::make_format_args(args...)));
}

const char *sprint_log_severity(int severity);
int parse_log_severity(std::string_view severity);
