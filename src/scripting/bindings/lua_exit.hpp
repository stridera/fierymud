#pragma once

#include "../../world/room.hpp"
#include "../../world/world_manager.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <memory>
#include <string>
#include <algorithm>

#include <spdlog/spdlog.h>

namespace FieryMUD {

/**
 * ExitWrapper - A Lua-facing object that represents an exit from a room.
 *
 * This wrapper holds a reference to the room and direction, allowing
 * scripts to read and modify exit properties in an object-oriented way.
 */
class ExitWrapper {
public:
    ExitWrapper(std::shared_ptr<Room> room, Direction dir)
        : room_(std::move(room)), direction_(dir) {}

    // Check if this exit wrapper is valid
    bool is_valid() const {
        return room_ && room_->has_exit(direction_);
    }

    // Get the underlying ExitInfo (const)
    const ExitInfo* get_exit() const {
        if (!room_) return nullptr;
        return room_->get_exit(direction_);
    }

    // Get the underlying ExitInfo (mutable)
    ExitInfo* get_exit_mutable() {
        if (!room_) return nullptr;
        return room_->get_exit_mutable(direction_);
    }

    // Properties
    std::string description() const {
        auto* exit = get_exit();
        return exit ? exit->description : "";
    }

    std::string name() const {
        auto* exit = get_exit();
        return exit ? exit->keyword : "";
    }

    bool has_door() const {
        auto* exit = get_exit();
        return exit ? exit->has_door : false;
    }

    bool is_closed() const {
        auto* exit = get_exit();
        return exit ? exit->is_closed : false;
    }

    bool is_locked() const {
        auto* exit = get_exit();
        return exit ? exit->is_locked : false;
    }

    bool is_pickproof() const {
        auto* exit = get_exit();
        return exit ? exit->is_pickproof : false;
    }

    bool is_hidden() const {
        auto* exit = get_exit();
        return exit ? exit->is_hidden : false;
    }

    bool is_passable() const {
        auto* exit = get_exit();
        return exit ? exit->is_passable() : false;
    }

    std::shared_ptr<Room> destination() const {
        auto* exit = get_exit();
        if (!exit || !exit->to_room.is_valid()) {
            return nullptr;
        }
        return WorldManager::instance().get_room(exit->to_room);
    }

    Direction direction() const {
        return direction_;
    }

    std::shared_ptr<Room> room() const {
        return room_;
    }

    // Methods
    bool open() {
        auto* exit = get_exit_mutable();
        if (!exit || !exit->has_door) return false;
        if (!exit->is_closed) return true; // Already open
        if (exit->is_locked) return false; // Can't open locked door
        exit->is_closed = false;
        return true;
    }

    bool close() {
        auto* exit = get_exit_mutable();
        if (!exit || !exit->has_door) return false;
        if (exit->is_closed) return true; // Already closed
        exit->is_closed = true;
        return true;
    }

    bool lock() {
        auto* exit = get_exit_mutable();
        if (!exit || !exit->has_door) return false;
        if (!exit->is_closed) return false; // Must be closed first
        exit->is_locked = true;
        return true;
    }

    bool unlock() {
        auto* exit = get_exit_mutable();
        if (!exit || !exit->has_door) return false;
        exit->is_locked = false;
        return true;
    }

    // Set multiple properties at once (partial update)
    bool set_state(sol::table state) {
        auto* exit = get_exit_mutable();
        if (!exit) return false;

        // Only update fields that are specified in the table
        if (auto val = state.get<sol::optional<bool>>("has_door")) {
            exit->has_door = *val;
        }
        if (auto val = state.get<sol::optional<bool>>("closed")) {
            exit->is_closed = *val;
        }
        if (auto val = state.get<sol::optional<bool>>("locked")) {
            exit->is_locked = *val;
        }
        if (auto val = state.get<sol::optional<bool>>("pickproof")) {
            exit->is_pickproof = *val;
        }
        if (auto val = state.get<sol::optional<bool>>("hidden")) {
            exit->is_hidden = *val;
        }
        if (auto val = state.get<sol::optional<std::string>>("name")) {
            exit->keyword = *val;
        }
        if (auto val = state.get<sol::optional<std::string>>("description")) {
            exit->description = *val;
        }

        spdlog::debug("exit:set_state: updated exit in room {}",
                      room_->id().to_string());
        return true;
    }

    // Set the destination room for this exit
    // Usage: exit:set_destination(room) or exit:set_destination(zone_id, local_id)
    bool set_destination(std::shared_ptr<Room> dest_room) {
        auto* exit = get_exit_mutable();
        if (!exit) return false;
        if (!dest_room) {
            // Clear the exit destination
            exit->to_room = EntityId();
            spdlog::debug("exit:set_destination: cleared destination for exit in room {}",
                          room_->id().to_string());
            return true;
        }
        exit->to_room = dest_room->id();
        spdlog::debug("exit:set_destination: set destination to {} for exit in room {}",
                      dest_room->id().to_string(), room_->id().to_string());
        return true;
    }

private:
    std::shared_ptr<Room> room_;
    Direction direction_;
};

/** Register Exit Lua bindings */
void register_exit_bindings(sol::state& lua);

/** Create an ExitWrapper for a room's exit in the given direction */
std::shared_ptr<ExitWrapper> create_exit_wrapper(std::shared_ptr<Room> room, const std::string& dir_str);

} // namespace FieryMUD
