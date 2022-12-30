#pragma once

#include <chrono>
#include <fmt/chrono.h>

using Clock = std::chrono::system_clock;
using Time = Clock::time_point;
using Seconds = std::chrono::seconds;
using Minutes = std::chrono::minutes;

inline auto formatted_time(Time time) { return fmt::format("{:%Y-%m-%d %H:%M:%S}Z", fmt::gmtime(time)); }
