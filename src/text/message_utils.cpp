#include "message_utils.hpp"

#include <fmt/format.h>

namespace fiery {

// Thread-local random number generator
thread_local std::mt19937 MessageUtils::rng_;
thread_local bool MessageUtils::rng_initialized_ = false;

std::mt19937 &MessageUtils::get_rng() {
    if (!rng_initialized_) {
        std::random_device rd;
        rng_.seed(rd());
        rng_initialized_ = true;
    }
    return rng_;
}

std::string_view MessageUtils::select_random(std::span<const std::string> messages) {
    if (messages.empty()) {
        return "";
    }
    if (messages.size() == 1) {
        return messages[0];
    }

    std::uniform_int_distribution<size_t> dist(0, messages.size() - 1);
    return messages[dist(get_rng())];
}

std::string_view MessageUtils::select_random(const std::vector<std::string> &messages) {
    return select_random(std::span<const std::string>(messages));
}

std::string MessageUtils::format_message(std::string_view message_template, std::string_view actor_name,
                                         std::string_view target_name, int damage, std::string_view weapon_name) {
    std::string result(message_template);

    // Replace {actor} placeholder
    size_t pos = 0;
    while ((pos = result.find("{actor}", pos)) != std::string::npos) {
        result.replace(pos, 7, actor_name);
        pos += actor_name.length();
    }

    // Replace {target} placeholder
    pos = 0;
    while ((pos = result.find("{target}", pos)) != std::string::npos) {
        result.replace(pos, 8, target_name);
        pos += target_name.length();
    }

    // Replace {damage} placeholder (only if damage >= 0)
    if (damage >= 0) {
        pos = 0;
        std::string damage_str = std::to_string(damage);
        while ((pos = result.find("{damage}", pos)) != std::string::npos) {
            result.replace(pos, 8, damage_str);
            pos += damage_str.length();
        }
    }

    // Replace {weapon} placeholder
    pos = 0;
    while ((pos = result.find("{weapon}", pos)) != std::string::npos) {
        result.replace(pos, 8, weapon_name);
        pos += weapon_name.length();
    }

    return result;
}

} // namespace fiery
