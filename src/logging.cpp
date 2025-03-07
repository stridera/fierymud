#include "logging.hpp"

#include "comm.hpp"
#include "defines.hpp"
#include "string_utils.hpp"
#include "utils.hpp"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/printf.h>

void log(LogSeverity severity, int level, std::string_view str) {
    DescriptorData *i;
    auto now = std::chrono::system_clock::now();
    fmt::print(stderr, "{:%c} :: {}\n", now, str);

    level = std::max(level, LVL_GOD);
    for (i = descriptor_list; i; i = i->next)
        if (!i->connected && !PLR_FLAGGED(i->character, PLR_WRITING) && !EDITING(i))
            if (GET_LEVEL(i->character) >= level && GET_LOG_VIEW(i->character) <= (int)severity)
                string_to_output(i, "LOG: {}\n", str);
}

const std::string_view sprint_log_severity(int severity) {
    return log_severities[std::clamp(0, (severity - 1) / 10, 6)];
}

int parse_log_severity(std::string_view severity) {
    // Find the index of the severity in the array
    for (int i = 0; i < log_severities.size(); i++)
        if (matches_start(severity, log_severities[i]))
            return (i + 1) * 10;
    return 0;
}