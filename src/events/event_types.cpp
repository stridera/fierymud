#include "event_types.hpp"

namespace fierymud::events {

/**
 * @brief Convert GameEventType to string for JSON serialization
 */
[[nodiscard]] static constexpr std::string_view to_string(GameEventType type) noexcept {
    switch (type) {
        case GameEventType::PLAYER_LOGIN:
            return "PLAYER_LOGIN";
        case GameEventType::PLAYER_LOGOUT:
            return "PLAYER_LOGOUT";
        case GameEventType::PLAYER_DEATH:
            return "PLAYER_DEATH";
        case GameEventType::PLAYER_LEVEL_UP:
            return "PLAYER_LEVEL_UP";
        case GameEventType::PLAYER_QUIT:
            return "PLAYER_QUIT";
        case GameEventType::PLAYER_ZONE_ENTER:
            return "PLAYER_ZONE_ENTER";
        case GameEventType::CHAT_GOSSIP:
            return "CHAT_GOSSIP";
        case GameEventType::CHAT_SHOUT:
            return "CHAT_SHOUT";
        case GameEventType::CHAT_OOC:
            return "CHAT_OOC";
        case GameEventType::CHAT_CLAN:
            return "CHAT_CLAN";
        case GameEventType::CHAT_GROUP:
            return "CHAT_GROUP";
        case GameEventType::CHAT_TELL:
            return "CHAT_TELL";
        case GameEventType::CHAT_SAY:
            return "CHAT_SAY";
        case GameEventType::CHAT_EMOTE:
            return "CHAT_EMOTE";
        case GameEventType::ADMIN_CRASH:
            return "ADMIN_CRASH";
        case GameEventType::ADMIN_ZONE_RESET:
            return "ADMIN_ZONE_RESET";
        case GameEventType::ADMIN_WARNING:
            return "ADMIN_WARNING";
        case GameEventType::ADMIN_SHUTDOWN:
            return "ADMIN_SHUTDOWN";
        case GameEventType::ADMIN_BROADCAST:
            return "ADMIN_BROADCAST";
        case GameEventType::ZONE_LOADED:
            return "ZONE_LOADED";
        case GameEventType::ZONE_RESET:
            return "ZONE_RESET";
        case GameEventType::MOB_KILLED:
            return "MOB_KILLED";
        case GameEventType::BOSS_SPAWN:
            return "BOSS_SPAWN";
        case GameEventType::QUEST_COMPLETE:
            return "QUEST_COMPLETE";
    }
    return "UNKNOWN";
}

/**
 * @brief Get the Redis channel for a given event type
 */
[[nodiscard]] static constexpr std::string_view get_channel(GameEventType type) noexcept {
    switch (type) {
        case GameEventType::PLAYER_LOGIN:
        case GameEventType::PLAYER_LOGOUT:
        case GameEventType::PLAYER_DEATH:
        case GameEventType::PLAYER_LEVEL_UP:
        case GameEventType::PLAYER_QUIT:
        case GameEventType::PLAYER_ZONE_ENTER:
            return "fierymud:events:player";

        case GameEventType::CHAT_GOSSIP:
        case GameEventType::CHAT_SHOUT:
        case GameEventType::CHAT_OOC:
        case GameEventType::CHAT_CLAN:
        case GameEventType::CHAT_GROUP:
        case GameEventType::CHAT_TELL:
        case GameEventType::CHAT_SAY:
        case GameEventType::CHAT_EMOTE:
            return "fierymud:events:chat";

        case GameEventType::ADMIN_CRASH:
        case GameEventType::ADMIN_ZONE_RESET:
        case GameEventType::ADMIN_WARNING:
        case GameEventType::ADMIN_SHUTDOWN:
        case GameEventType::ADMIN_BROADCAST:
            return "fierymud:events:admin";

        case GameEventType::ZONE_LOADED:
        case GameEventType::ZONE_RESET:
        case GameEventType::MOB_KILLED:
        case GameEventType::BOSS_SPAWN:
        case GameEventType::QUEST_COMPLETE:
            return "fierymud:events:world";
    }
    return "fierymud:events:unknown";
}

GameEvent GameEvent::create(GameEventType type, std::string message) {
    return GameEvent{
        .type = type,
        .timestamp = std::chrono::system_clock::now(),
        .player_name = std::nullopt,
        .zone_id = std::nullopt,
        .room_id = std::nullopt,
        .message = std::move(message),
        .metadata = nlohmann::json::object()};
}

GameEvent GameEvent::player_event(GameEventType type, std::string player_name,
                              std::string message) {
    auto event = create(type, std::move(message));
    event.player_name = std::move(player_name);
    return event;
}

GameEvent GameEvent::chat_event(GameEventType type, std::string player_name,
                            std::string message,
                            std::optional<std::string> target) {
    auto event = player_event(type, std::move(player_name), std::move(message));
    if (target) {
        event.metadata["target"] = *target;
    }
    return event;
}

GameEvent GameEvent::zone_event(GameEventType type, int zone_id, std::string message) {
    auto event = create(type, std::move(message));
    event.zone_id = zone_id;
    return event;
}

nlohmann::json GameEvent::to_json() const {
    nlohmann::json j;
    j["type"] = std::string(to_string(type));
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                         timestamp.time_since_epoch())
                         .count();
    j["message"] = message;

    if (player_name) {
        j["playerName"] = *player_name;
    }
    if (zone_id) {
        j["zoneId"] = *zone_id;
    }
    if (room_id) {
        j["roomId"] = *room_id;
    }
    if (!metadata.empty()) {
        j["metadata"] = metadata;
    }

    return j;
}

std::string_view GameEvent::channel() const noexcept {
    return get_channel(type);
}

}  // namespace fierymud::events
