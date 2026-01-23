#include "lua_doors.hpp"
#include "../../world/room.hpp"

#include <algorithm>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <spdlog/spdlog.h>

namespace FieryMUD {

namespace {

// Convert string direction to Direction enum
std::optional<Direction> parse_direction(const std::string& dir_str) {
    static const std::unordered_map<std::string, Direction> dir_map = {
        {"north", Direction::North}, {"n", Direction::North},
        {"east", Direction::East}, {"e", Direction::East},
        {"south", Direction::South}, {"s", Direction::South},
        {"west", Direction::West}, {"w", Direction::West},
        {"up", Direction::Up}, {"u", Direction::Up},
        {"down", Direction::Down}, {"d", Direction::Down}
    };

    std::string lower = dir_str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto it = dir_map.find(lower);
    if (it != dir_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // anonymous namespace

void register_door_bindings(sol::state& lua) {
    auto doors_table = lua.create_named_table("doors");

    // doors.open(room, direction) - Open a door
    // Returns: (bool success, string? error)
    doors_table["open"] = [](std::shared_ptr<Room> room, const std::string& dir_str)
        -> std::tuple<bool, sol::optional<std::string>> {
        if (!room) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        auto dir = parse_direction(dir_str);
        if (!dir) {
            return std::make_tuple(false, std::string("invalid_direction"));
        }

        if (!room->has_exit(*dir)) {
            return std::make_tuple(false, std::string("no_exit"));
        }

        auto result = room->open_door(*dir);
        if (!result) {
            return std::make_tuple(false, result.error().message);
        }

        spdlog::debug("doors.open: opened door {} in room {}", dir_str, room->id().to_string());
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    // doors.close(room, direction) - Close a door
    // Returns: (bool success, string? error)
    doors_table["close"] = [](std::shared_ptr<Room> room, const std::string& dir_str)
        -> std::tuple<bool, sol::optional<std::string>> {
        if (!room) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        auto dir = parse_direction(dir_str);
        if (!dir) {
            return std::make_tuple(false, std::string("invalid_direction"));
        }

        if (!room->has_exit(*dir)) {
            return std::make_tuple(false, std::string("no_exit"));
        }

        auto result = room->close_door(*dir);
        if (!result) {
            return std::make_tuple(false, result.error().message);
        }

        spdlog::debug("doors.close: closed door {} in room {}", dir_str, room->id().to_string());
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    // doors.lock(room, direction) - Lock a door
    // Returns: (bool success, string? error)
    doors_table["lock"] = [](std::shared_ptr<Room> room, const std::string& dir_str)
        -> std::tuple<bool, sol::optional<std::string>> {
        if (!room) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        auto dir = parse_direction(dir_str);
        if (!dir) {
            return std::make_tuple(false, std::string("invalid_direction"));
        }

        if (!room->has_exit(*dir)) {
            return std::make_tuple(false, std::string("no_exit"));
        }

        auto result = room->lock_door(*dir);
        if (!result) {
            return std::make_tuple(false, result.error().message);
        }

        spdlog::debug("doors.lock: locked door {} in room {}", dir_str, room->id().to_string());
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    // doors.unlock(room, direction) - Unlock a door
    // Returns: (bool success, string? error)
    doors_table["unlock"] = [](std::shared_ptr<Room> room, const std::string& dir_str)
        -> std::tuple<bool, sol::optional<std::string>> {
        if (!room) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        auto dir = parse_direction(dir_str);
        if (!dir) {
            return std::make_tuple(false, std::string("invalid_direction"));
        }

        if (!room->has_exit(*dir)) {
            return std::make_tuple(false, std::string("no_exit"));
        }

        auto result = room->unlock_door(*dir);
        if (!result) {
            return std::make_tuple(false, result.error().message);
        }

        spdlog::debug("doors.unlock: unlocked door {} in room {}", dir_str, room->id().to_string());
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    // doors.set_state(room, direction, flags) - Set door state directly
    // flags table: { closed = bool, locked = bool, hidden = bool }
    // Returns: (bool success, string? error)
    doors_table["set_state"] = [](std::shared_ptr<Room> room, const std::string& dir_str, sol::table flags)
        -> std::tuple<bool, sol::optional<std::string>> {
        if (!room) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        auto dir = parse_direction(dir_str);
        if (!dir) {
            return std::make_tuple(false, std::string("invalid_direction"));
        }

        if (!room->has_exit(*dir)) {
            return std::make_tuple(false, std::string("no_exit"));
        }

        DoorState state;
        state.closed = flags.get_or("closed", false);
        state.locked = flags.get_or("locked", false);
        state.hidden = flags.get_or("hidden", false);

        auto result = room->set_door_state(*dir, state);
        if (!result) {
            return std::make_tuple(false, result.error().message);
        }

        spdlog::debug("doors.set_state: set door {} state in room {}", dir_str, room->id().to_string());
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    spdlog::debug("Registered doors Lua bindings");
}

} // namespace FieryMUD
