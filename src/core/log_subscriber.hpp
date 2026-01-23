#pragma once

#include "logging.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

/**
 * LogSubscriber - Manages in-game syslog subscriptions for gods
 *
 * Allows god-level players to subscribe to real-time log messages,
 * filtering by log level and/or component (e.g., scripting, database).
 *
 * Thread Safety: Uses mutex protection for subscription management.
 */
class LogSubscriber {
  public:
    /// Subscription information for a player
    struct Subscription {
        std::weak_ptr<Player> player;
        LogLevel min_level = LogLevel::Info;
        std::set<std::string> components; // Empty = all components

        /// Check if this subscription matches a log message
        [[nodiscard]] bool matches(const std::string &component, LogLevel level) const {
            // Level must be >= min_level
            if (static_cast<int>(level) < static_cast<int>(min_level)) {
                return false;
            }
            // If no specific components, match all
            if (components.empty()) {
                return true;
            }
            // Otherwise, must match one of the specified components
            return components.contains(component);
        }
    };

    /// Get singleton instance
    static LogSubscriber &instance();

    /// Subscribe a player to log messages
    /// @param player The player to subscribe
    /// @param level Minimum log level to receive
    /// @param components Set of component names to filter (empty = all)
    void subscribe(std::shared_ptr<Player> player, LogLevel level,
                   const std::set<std::string> &components = {});

    /// Unsubscribe a player from log messages
    void unsubscribe(std::shared_ptr<Player> player);

    /// Check if a player is subscribed
    [[nodiscard]] bool is_subscribed(std::shared_ptr<Player> player) const;

    /// Get subscription info for a player (returns nullptr if not subscribed)
    [[nodiscard]] const Subscription *get_subscription(std::shared_ptr<Player> player) const;

    /// Broadcast a log message to all subscribed players
    /// Called by the custom spdlog sink
    void broadcast(const std::string &component, LogLevel level, std::string_view message);

    /// Get list of available log components
    [[nodiscard]] static std::vector<std::string> available_components();

    /// Parse log level from string
    [[nodiscard]] static std::optional<LogLevel> parse_level(std::string_view str);

    /// Convert log level to string
    [[nodiscard]] static std::string_view level_to_string(LogLevel level);

    /// Clean up expired subscriptions (players who logged out)
    void cleanup_expired();

    /// Get count of active subscriptions
    [[nodiscard]] std::size_t subscription_count() const;

  private:
    LogSubscriber() = default;

    /// Map of player database ID -> subscription
    std::unordered_map<std::string, Subscription> subscriptions_;
    mutable std::mutex mutex_;
};

/**
 * Custom spdlog sink that broadcasts to subscribed players
 *
 * This sink is registered with spdlog during Logger initialization
 * and forwards log messages to LogSubscriber for in-game viewing.
 */
class PlayerLogSink : public spdlog::sinks::base_sink<std::mutex> {
  public:
    /// Get the singleton instance
    static std::shared_ptr<PlayerLogSink> instance();

  protected:
    void sink_it_(const spdlog::details::log_msg &msg) override;
    void flush_() override {}

  private:
    PlayerLogSink() = default;
};
