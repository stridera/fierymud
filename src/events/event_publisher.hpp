#pragma once

/**
 * @file event_publisher.hpp
 * @brief Redis-based event publishing for Muditor bridge integration
 *
 * Publishes game events (chat, player activity, admin alerts) to Redis
 * for consumption by Muditor and Discord bot.
 */

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <thread>

#include "events/event_types.hpp"

// Forward declare hiredis types
struct redisContext;

namespace fierymud::events {

/**
 * @brief Configuration for the event publisher
 */
struct EventPublisherConfig {
    std::string redis_host = "127.0.0.1";
    int redis_port = 6379;
    std::string redis_password;  // Empty = no auth
    int connection_timeout_ms = 1000;
    int reconnect_delay_ms = 5000;
    bool enabled = true;  // Allow disabling event publishing
};

/**
 * @brief Publishes game events to Redis for external consumption
 *
 * This class provides a thread-safe, non-blocking interface for publishing
 * game events to Redis. Events are queued and published asynchronously
 * to avoid blocking the game loop.
 *
 * Usage:
 *   EventPublisher::instance().publish(
 *       GameEvent::chat_event(GameEventType::CHAT_GOSSIP, "Player", "Hello!"));
 *
 * Configuration via environment variables:
 *   REDIS_HOST - Redis server hostname (default: 127.0.0.1)
 *   REDIS_PORT - Redis server port (default: 6379)
 *   REDIS_PASSWORD - Redis password (default: none)
 *   FIERYMUD_EVENTS_ENABLED - Enable/disable publishing (default: true)
 */
class EventPublisher {
  public:
    /**
     * @brief Get the singleton instance
     */
    static EventPublisher& instance();

    /**
     * @brief Initialize the publisher with configuration
     * @param config Publisher configuration
     * @return true if initialization successful
     */
    bool initialize(const EventPublisherConfig& config = {});

    /**
     * @brief Initialize from environment variables
     *
     * Reads configuration from:
     *   REDIS_HOST, REDIS_PORT, REDIS_PASSWORD, FIERYMUD_EVENTS_ENABLED
     */
    bool initialize_from_env();

    /**
     * @brief Shutdown the publisher and disconnect from Redis
     */
    void shutdown();

    /**
     * @brief Check if the publisher is connected to Redis
     */
    [[nodiscard]] bool is_connected() const noexcept;

    /**
     * @brief Check if the publisher is enabled
     */
    [[nodiscard]] bool is_enabled() const noexcept;

    /**
     * @brief Publish an event to Redis (non-blocking)
     *
     * Events are queued and published asynchronously by a background thread.
     * This method returns immediately and never blocks the game loop.
     *
     * @param event The event to publish
     */
    void publish(GameEvent event);

    /**
     * @brief Publish a raw message to a specific channel (non-blocking)
     *
     * @param channel Redis channel name
     * @param message JSON message to publish
     */
    void publish_raw(std::string_view channel, std::string message);

    // Convenience methods for common event types

    /**
     * @brief Publish a chat message event
     */
    void publish_chat(GameEventType type, std::string_view player_name,
                      std::string_view message,
                      std::optional<std::string_view> target = std::nullopt);

    /**
     * @brief Publish a player lifecycle event (login, logout, death, etc.)
     */
    void publish_player(GameEventType type, std::string_view player_name,
                        std::string_view message, std::optional<int> zone_id = std::nullopt,
                        std::optional<int> room_vnum = std::nullopt);

    /**
     * @brief Publish an admin alert
     */
    void publish_admin(GameEventType type, std::string_view message);

    /**
     * @brief Publish a zone/world event
     */
    void publish_zone(GameEventType type, int zone_id, std::string_view message);

    /**
     * @brief Get statistics about published events
     */
    struct Stats {
        uint64_t events_published = 0;
        uint64_t events_dropped = 0;
        uint64_t reconnect_attempts = 0;
        bool connected = false;
    };
    [[nodiscard]] Stats get_stats() const noexcept;

  private:
    EventPublisher() = default;
    ~EventPublisher();

    // Disable copy/move
    EventPublisher(const EventPublisher&) = delete;
    EventPublisher& operator=(const EventPublisher&) = delete;
    EventPublisher(EventPublisher&&) = delete;
    EventPublisher& operator=(EventPublisher&&) = delete;

    /**
     * @brief Background thread function that processes the event queue
     */
    void publisher_thread();

    /**
     * @brief Connect to Redis server
     */
    bool connect();

    /**
     * @brief Disconnect from Redis server
     */
    void disconnect();

    /**
     * @brief Actually publish a message to Redis (called from publisher thread)
     */
    bool do_publish(std::string_view channel, std::string_view message);

    // Configuration
    EventPublisherConfig config_;

    // Redis connection
    redisContext* redis_ctx_ = nullptr;
    mutable std::mutex redis_mutex_;

    // Event queue for async publishing
    struct QueuedEvent {
        std::string channel;
        std::string message;
    };
    std::queue<QueuedEvent> event_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // Background publisher thread
    std::thread publisher_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> enabled_{false};

    // Statistics
    mutable std::atomic<uint64_t> events_published_{0};
    mutable std::atomic<uint64_t> events_dropped_{0};
    mutable std::atomic<uint64_t> reconnect_attempts_{0};

    // Maximum queue size before dropping events
    static constexpr size_t MAX_QUEUE_SIZE = 10000;
};

}  // namespace fierymud::events
