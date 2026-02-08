#pragma once

/**
 * @file event_types.hpp
 * @brief Event types for Muditor bridge integration
 *
 * Defines the event types and structures used for publishing game events
 * to external systems (Muditor web editor, Discord bot) via Redis pub/sub.
 */

#include <chrono>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace fierymud::events {

/**
 * @brief Categories of game events published to external systems
 */
enum class GameEventType {
    // Player lifecycle events
    PLAYER_LOGIN,
    PLAYER_LOGOUT,
    PLAYER_DEATH,
    PLAYER_LEVEL_UP,
    PLAYER_QUIT,
    PLAYER_ZONE_ENTER,

    // Communication channel events
    CHAT_GOSSIP,
    CHAT_SHOUT,
    CHAT_OOC,
    CHAT_CLAN,
    CHAT_GROUP,
    CHAT_TELL,
    CHAT_SAY,
    CHAT_EMOTE,

    // Admin/system alerts
    ADMIN_CRASH,
    ADMIN_ZONE_RESET,
    ADMIN_WARNING,
    ADMIN_SHUTDOWN,
    ADMIN_BROADCAST,

    // World events
    ZONE_LOADED,
    ZONE_RESET,
    MOB_KILLED,
    BOSS_SPAWN,
    QUEST_COMPLETE
};

/**
 * @brief A game event to be published to external systems
 */
struct GameEvent {
    GameEventType type;
    std::chrono::system_clock::time_point timestamp;
    std::optional<std::string> player_name;
    std::optional<int> zone_id;
    std::optional<std::string> room_id; // Format: "zone:local" (EntityId string)
    std::string message;
    nlohmann::json metadata;

    /**
     * @brief Create a new event with the current timestamp
     */
    static GameEvent create(GameEventType type, std::string message);

    /**
     * @brief Create a player-related event
     */
    static GameEvent player_event(GameEventType type, std::string player_name, std::string message);

    /**
     * @brief Create a chat event
     */
    static GameEvent chat_event(GameEventType type, std::string player_name, std::string message,
                                std::optional<std::string> target = std::nullopt);

    /**
     * @brief Create a zone-related event
     */
    static GameEvent zone_event(GameEventType type, int zone_id, std::string message);

    /**
     * @brief Serialize to JSON for Redis publishing
     */
    [[nodiscard]] nlohmann::json to_json() const;

    /**
     * @brief Get the Redis channel this event should be published to
     */
    [[nodiscard]] std::string_view channel() const noexcept;
};

} // namespace fierymud::events
