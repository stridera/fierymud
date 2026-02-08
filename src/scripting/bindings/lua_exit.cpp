#include "lua_exit.hpp"

namespace FieryMUD {

std::optional<Direction> parse_direction(const std::string &dir_str) {
    static const std::unordered_map<std::string, Direction> dir_map = {{"north", Direction::North},
                                                                       {"n", Direction::North},
                                                                       {"east", Direction::East},
                                                                       {"e", Direction::East},
                                                                       {"south", Direction::South},
                                                                       {"s", Direction::South},
                                                                       {"west", Direction::West},
                                                                       {"w", Direction::West},
                                                                       {"up", Direction::Up},
                                                                       {"u", Direction::Up},
                                                                       {"down", Direction::Down},
                                                                       {"d", Direction::Down},
                                                                       {"northeast", Direction::Northeast},
                                                                       {"ne", Direction::Northeast},
                                                                       {"northwest", Direction::Northwest},
                                                                       {"nw", Direction::Northwest},
                                                                       {"southeast", Direction::Southeast},
                                                                       {"se", Direction::Southeast},
                                                                       {"southwest", Direction::Southwest},
                                                                       {"sw", Direction::Southwest},
                                                                       {"in", Direction::In},
                                                                       {"out", Direction::Out}};

    std::string lower = dir_str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto it = dir_map.find(lower);
    if (it != dir_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

void register_exit_bindings(sol::state &lua) {
    // Register the ExitWrapper as "Exit" usertype
    lua.new_usertype<ExitWrapper>(
        "Exit", sol::no_constructor,

        // Validity check
        "is_valid", &ExitWrapper::is_valid,

        // Read-only properties
        "description", sol::property(&ExitWrapper::description), "name", sol::property(&ExitWrapper::name), "has_door",
        sol::property(&ExitWrapper::has_door), "is_closed", sol::property(&ExitWrapper::is_closed), "is_locked",
        sol::property(&ExitWrapper::is_locked), "is_pickproof", sol::property(&ExitWrapper::is_pickproof), "is_hidden",
        sol::property(&ExitWrapper::is_hidden), "is_passable", sol::property(&ExitWrapper::is_passable), "destination",
        sol::property(&ExitWrapper::destination), "direction", sol::property(&ExitWrapper::direction), "room",
        sol::property(&ExitWrapper::room),

        // Methods
        "open", &ExitWrapper::open, "close", &ExitWrapper::close, "lock", &ExitWrapper::lock, "unlock",
        &ExitWrapper::unlock, "set_state", &ExitWrapper::set_state, "set_destination", &ExitWrapper::set_destination);

    spdlog::debug("Registered Exit Lua bindings");
}

std::shared_ptr<ExitWrapper> create_exit_wrapper(std::shared_ptr<Room> room, const std::string &dir_str) {
    if (!room)
        return nullptr;

    auto dir = parse_direction(dir_str);
    if (!dir) {
        spdlog::warn("room:exit: Invalid direction '{}'", dir_str);
        return nullptr;
    }

    if (!room->has_exit(*dir)) {
        spdlog::debug("room:exit: No exit {} in room {}", dir_str, room->id().to_string());
        return nullptr;
    }

    return std::make_shared<ExitWrapper>(room, *dir);
}

} // namespace FieryMUD
