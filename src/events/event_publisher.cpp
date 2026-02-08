/**
 * @file event_publisher.cpp
 * @brief Implementation of Redis-based event publishing
 */

#include "events/event_publisher.hpp"

#include <cstdlib>

#include <fmt/format.h>
#include <hiredis.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "events/event_types.hpp"

namespace fierymud::events {

EventPublisher& EventPublisher::instance() {
    static EventPublisher instance;
    return instance;
}

EventPublisher::~EventPublisher() { shutdown(); }

bool EventPublisher::initialize(const EventPublisherConfig& config) {
    std::lock_guard lock(redis_mutex_);

    if (running_) {
        spdlog::warn("EventPublisher already initialized");
        return true;
    }

    config_ = config;
    enabled_ = config.enabled;

    if (!enabled_) {
        spdlog::info("EventPublisher disabled by configuration");
        return true;
    }

    // Try initial connection
    if (!connect()) {
        spdlog::warn("EventPublisher: Initial Redis connection failed, will retry in "
                     "background");
    }

    // Start background publisher thread
    running_ = true;
    publisher_thread_ = std::thread(&EventPublisher::publisher_thread, this);

    spdlog::info("EventPublisher initialized (host={}:{}, enabled={})", config_.redis_host,
                 config_.redis_port, enabled_.load());

    return true;
}

bool EventPublisher::initialize_from_env() {
    EventPublisherConfig config;

    // REDIS_URL format: redis://[:password@]host[:port]
    if (const char* url = std::getenv("REDIS_URL"); url != nullptr) {
        std::string_view url_view(url);

        // Skip redis:// prefix
        if (url_view.starts_with("redis://")) {
            url_view = url_view.substr(8);
        }

        // Check for password (format: :password@host)
        if (auto at_pos = url_view.find('@'); at_pos != std::string_view::npos) {
            auto auth_part = url_view.substr(0, at_pos);
            if (auth_part.starts_with(':')) {
                config.redis_password = std::string(auth_part.substr(1));
            }
            url_view = url_view.substr(at_pos + 1);
        }

        // Parse host:port
        if (auto colon_pos = url_view.find(':'); colon_pos != std::string_view::npos) {
            config.redis_host = std::string(url_view.substr(0, colon_pos));
            config.redis_port = std::stoi(std::string(url_view.substr(colon_pos + 1)));
        } else {
            config.redis_host = std::string(url_view);
        }
    } else {
        // Individual environment variables
        if (const char* host = std::getenv("REDIS_HOST"); host != nullptr) {
            config.redis_host = host;
        }
        if (const char* port = std::getenv("REDIS_PORT"); port != nullptr) {
            config.redis_port = std::stoi(port);
        }
        if (const char* pass = std::getenv("REDIS_PASSWORD"); pass != nullptr) {
            config.redis_password = pass;
        }
    }

    // Check if events are enabled
    if (const char* enabled = std::getenv("FIERYMUD_EVENTS_ENABLED"); enabled != nullptr) {
        std::string_view enabled_view(enabled);
        config.enabled = (enabled_view == "1" || enabled_view == "true" ||
                          enabled_view == "yes" || enabled_view == "on");
    }

    return initialize(config);
}

void EventPublisher::shutdown() {
    if (!running_.exchange(false)) {
        return;  // Already shutdown
    }

    spdlog::info("EventPublisher shutting down...");

    // Wake up the publisher thread
    {
        std::lock_guard lock(queue_mutex_);
        queue_cv_.notify_all();
    }

    // Wait for publisher thread to finish
    if (publisher_thread_.joinable()) {
        publisher_thread_.join();
    }

    // Disconnect from Redis
    disconnect();

    spdlog::info("EventPublisher shutdown complete (published={}, dropped={})",
                 events_published_.load(), events_dropped_.load());
}

bool EventPublisher::is_connected() const noexcept {
    std::lock_guard lock(redis_mutex_);
    return redis_ctx_ != nullptr && redis_ctx_->err == 0;
}

bool EventPublisher::is_enabled() const noexcept { return enabled_.load(); }

void EventPublisher::publish(GameEvent event) {
    if (!enabled_.load()) {
        return;
    }

    std::string channel(event.channel());
    std::string message = event.to_json().dump();

    publish_raw(channel, std::move(message));
}

void EventPublisher::publish_raw(std::string_view channel, std::string message) {
    if (!enabled_.load()) {
        return;
    }

    {
        std::lock_guard lock(queue_mutex_);

        // Drop events if queue is too large
        if (event_queue_.size() >= MAX_QUEUE_SIZE) {
            ++events_dropped_;
            return;
        }

        event_queue_.push(QueuedEvent{.channel = std::string(channel), .message = std::move(message)});
    }

    queue_cv_.notify_one();
}

void EventPublisher::publish_chat(GameEventType type, std::string_view player_name,
                                  std::string_view message,
                                  std::optional<std::string_view> target) {
    std::optional<std::string> target_str;
    if (target) {
        target_str = std::string(*target);
    }
    publish(GameEvent::chat_event(type, std::string(player_name), std::string(message),
                                  target_str));
}

void EventPublisher::publish_player(GameEventType type, std::string_view player_name,
                                    std::string_view message, std::optional<int> zone_id,
                                    std::optional<std::string> room_id) {
    auto event =
        GameEvent::player_event(type, std::string(player_name), std::string(message));
    event.zone_id = zone_id;
    event.room_id = room_id;
    publish(std::move(event));
}

void EventPublisher::publish_admin(GameEventType type, std::string_view message) {
    publish(GameEvent::create(type, std::string(message)));
}

void EventPublisher::publish_zone(GameEventType type, int zone_id,
                                  std::string_view message) {
    publish(GameEvent::zone_event(type, zone_id, std::string(message)));
}

EventPublisher::Stats EventPublisher::get_stats() const noexcept {
    return Stats{.events_published = events_published_.load(),
                 .events_dropped = events_dropped_.load(),
                 .reconnect_attempts = reconnect_attempts_.load(),
                 .connected = is_connected()};
}

void EventPublisher::publisher_thread() {
    spdlog::debug("EventPublisher thread started");

    while (running_.load()) {
        QueuedEvent event;
        bool has_event = false;

        {
            std::unique_lock lock(queue_mutex_);

            // Wait for events or shutdown signal
            queue_cv_.wait_for(lock, std::chrono::milliseconds(100),
                               [this] { return !event_queue_.empty() || !running_.load(); });

            if (!event_queue_.empty()) {
                event = std::move(event_queue_.front());
                event_queue_.pop();
                has_event = true;
            }
        }

        if (has_event) {
            // Try to publish, reconnecting if needed
            if (!is_connected()) {
                ++reconnect_attempts_;
                if (!connect()) {
                    ++events_dropped_;
                    continue;
                }
            }

            if (do_publish(event.channel, event.message)) {
                ++events_published_;
            } else {
                ++events_dropped_;
                // Connection may be broken, try to reconnect on next event
                disconnect();
            }
        }
    }

    spdlog::debug("EventPublisher thread stopped");
}

bool EventPublisher::connect() {
    if (redis_ctx_ != nullptr) {
        disconnect();
    }

    struct timeval timeout;
    timeout.tv_sec = config_.connection_timeout_ms / 1000;
    timeout.tv_usec = (config_.connection_timeout_ms % 1000) * 1000;

    redis_ctx_ =
        redisConnectWithTimeout(config_.redis_host.c_str(), config_.redis_port, timeout);

    if (redis_ctx_ == nullptr) {
        spdlog::error("EventPublisher: Failed to allocate Redis context");
        return false;
    }

    if (redis_ctx_->err) {
        spdlog::error("EventPublisher: Redis connection error: {}", redis_ctx_->errstr);
        redisFree(redis_ctx_);
        redis_ctx_ = nullptr;
        return false;
    }

    // Authenticate if password is set
    if (!config_.redis_password.empty()) {
        auto* reply = static_cast<redisReply*>(
            redisCommand(redis_ctx_, "AUTH %s", config_.redis_password.c_str()));
        if (reply == nullptr || redis_ctx_->err) {
            spdlog::error("EventPublisher: Redis AUTH failed");
            redisFree(redis_ctx_);
            redis_ctx_ = nullptr;
            return false;
        }
        freeReplyObject(reply);
    }

    spdlog::info("EventPublisher: Connected to Redis at {}:{}", config_.redis_host,
                 config_.redis_port);
    return true;
}

void EventPublisher::disconnect() {
    if (redis_ctx_ != nullptr) {
        redisFree(redis_ctx_);
        redis_ctx_ = nullptr;
    }
}

bool EventPublisher::do_publish(std::string_view channel, std::string_view message) {
    std::lock_guard lock(redis_mutex_);

    if (redis_ctx_ == nullptr) {
        return false;
    }

    auto* reply = static_cast<redisReply*>(redisCommand(
        redis_ctx_, "PUBLISH %b %b", channel.data(), channel.size(), message.data(),
        message.size()));

    if (reply == nullptr) {
        spdlog::error("EventPublisher: PUBLISH failed: {}", redis_ctx_->errstr);
        return false;
    }

    bool success = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);

    return success;
}

}  // namespace fierymud::events
