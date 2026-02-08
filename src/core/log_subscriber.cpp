#include "log_subscriber.hpp"

#include "actor.hpp"
#include "player.hpp"

#include <algorithm>
#include <fmt/format.h>

LogSubscriber &LogSubscriber::instance() {
    static LogSubscriber instance;
    return instance;
}

void LogSubscriber::subscribe(std::shared_ptr<Player> player, LogLevel level,
                              const std::set<std::string> &components) {
    if (!player) {
        return;
    }

    std::lock_guard lock(mutex_);

    Subscription sub;
    sub.player = player;
    sub.min_level = level;
    sub.components = components;

    subscriptions_[std::string(player->database_id())] = std::move(sub);
}

void LogSubscriber::unsubscribe(std::shared_ptr<Player> player) {
    if (!player) {
        return;
    }

    std::lock_guard lock(mutex_);
    subscriptions_.erase(std::string(player->database_id()));
}

bool LogSubscriber::is_subscribed(std::shared_ptr<Player> player) const {
    if (!player) {
        return false;
    }

    std::lock_guard lock(mutex_);
    return subscriptions_.contains(std::string(player->database_id()));
}

const LogSubscriber::Subscription *LogSubscriber::get_subscription(std::shared_ptr<Player> player) const {
    if (!player) {
        return nullptr;
    }

    std::lock_guard lock(mutex_);
    auto it = subscriptions_.find(std::string(player->database_id()));
    if (it != subscriptions_.end()) {
        return &it->second;
    }
    return nullptr;
}

void LogSubscriber::broadcast(const std::string &component, LogLevel level, std::string_view message) {
    std::lock_guard lock(mutex_);

    // Format the log message with color based on level
    std::string level_color;
    std::string_view level_str = level_to_string(level);

    switch (level) {
    case LogLevel::Trace:
        level_color = "blue";
        break;
    case LogLevel::Debug:
        level_color = "cyan";
        break;
    case LogLevel::Info:
        level_color = "green";
        break;
    case LogLevel::Warning:
        level_color = "yellow";
        break;
    case LogLevel::Error:
        level_color = "red";
        break;
    case LogLevel::Critical:
        level_color = "magenta";
        break;
    default:
        level_color = "white";
        break;
    }

    // Format: [LEVEL] [component] message
    std::string formatted = fmt::format("<b:{}>[{}]</> [<b:cyan>{}</>] {}\r\n", level_color, level_str, component, message);

    // Iterate through subscriptions and send to matching players
    for (auto it = subscriptions_.begin(); it != subscriptions_.end();) {
        auto player = it->second.player.lock();
        if (!player) {
            // Player disconnected, remove subscription
            it = subscriptions_.erase(it);
            continue;
        }

        if (it->second.matches(component, level)) {
            player->send_message(formatted);
        }
        ++it;
    }
}

std::vector<std::string> LogSubscriber::available_components() {
    return {"game", "combat", "movement", "commands", "network", "persistence", "scripting", "database"};
}

std::optional<LogLevel> LogSubscriber::parse_level(std::string_view str) {
    std::string lower{str};
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "trace") return LogLevel::Trace;
    if (lower == "debug") return LogLevel::Debug;
    if (lower == "info") return LogLevel::Info;
    if (lower == "warn" || lower == "warning") return LogLevel::Warning;
    if (lower == "error") return LogLevel::Error;
    if (lower == "critical" || lower == "crit") return LogLevel::Critical;

    return std::nullopt;
}

std::string_view LogSubscriber::level_to_string(LogLevel level) {
    switch (level) {
    case LogLevel::Trace:
        return "TRACE";
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Critical:
        return "CRIT";
    case LogLevel::Off:
        return "OFF";
    }
    return "UNKNOWN";
}

void LogSubscriber::cleanup_expired() {
    std::lock_guard lock(mutex_);

    std::erase_if(subscriptions_, [](const auto &pair) { return pair.second.player.expired(); });
}

std::size_t LogSubscriber::subscription_count() const {
    std::lock_guard lock(mutex_);
    return subscriptions_.size();
}

// ============================================================================
// PlayerLogSink implementation
// ============================================================================

std::shared_ptr<PlayerLogSink> PlayerLogSink::instance() {
    static auto instance = std::shared_ptr<PlayerLogSink>(new PlayerLogSink());
    return instance;
}

void PlayerLogSink::sink_it_(const spdlog::details::log_msg &msg) {
    // Skip if no subscriptions (optimization)
    if (LogSubscriber::instance().subscription_count() == 0) {
        return;
    }

    // Extract component from logger name
    std::string component{msg.logger_name.data(), msg.logger_name.size()};

    // Convert spdlog level to our LogLevel
    LogLevel level;
    switch (msg.level) {
    case spdlog::level::trace:
        level = LogLevel::Trace;
        break;
    case spdlog::level::debug:
        level = LogLevel::Debug;
        break;
    case spdlog::level::info:
        level = LogLevel::Info;
        break;
    case spdlog::level::warn:
        level = LogLevel::Warning;
        break;
    case spdlog::level::err:
        level = LogLevel::Error;
        break;
    case spdlog::level::critical:
        level = LogLevel::Critical;
        break;
    default:
        level = LogLevel::Info;
        break;
    }

    // Extract message payload (without timestamp/level prefix)
    std::string_view message{msg.payload.data(), msg.payload.size()};

    // Broadcast to subscribers
    LogSubscriber::instance().broadcast(component, level, message);
}
