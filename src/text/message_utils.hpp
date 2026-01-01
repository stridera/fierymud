#pragma once

#include <random>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fiery {

/**
 * @brief Utility functions for message handling with random selection
 *
 * Used with database message arrays (CombatMessage, PositionMessage, SystemMessage)
 * to provide variety in game output.
 */
class MessageUtils {
public:
    /**
     * @brief Select a random message from an array of messages
     *
     * @param messages Span of message strings to choose from
     * @return A random message, or empty string if array is empty
     */
    [[nodiscard]] static std::string_view select_random(std::span<const std::string> messages);

    /**
     * @brief Select a random message from a vector of messages
     *
     * @param messages Vector of message strings to choose from
     * @return A random message, or empty string if vector is empty
     */
    [[nodiscard]] static std::string_view select_random(const std::vector<std::string>& messages);

    /**
     * @brief Substitute placeholders in a message template
     *
     * Replaces placeholders like {actor}, {target}, {damage}, {weapon} with actual values.
     *
     * @param message_template The template string with placeholders
     * @param actor_name Name of the actor performing the action
     * @param target_name Name of the target (optional)
     * @param damage Damage amount (optional, default -1 means not included)
     * @param weapon_name Name of the weapon (optional)
     * @return Formatted message with placeholders replaced
     */
    [[nodiscard]] static std::string format_message(
        std::string_view message_template,
        std::string_view actor_name,
        std::string_view target_name = "",
        int damage = -1,
        std::string_view weapon_name = ""
    );

    /**
     * @brief Get random number generator (thread-local for thread safety)
     */
    static std::mt19937& get_rng();

private:
    // Thread-local random number generator
    static thread_local std::mt19937 rng_;
    static thread_local bool rng_initialized_;
};

} // namespace fiery
