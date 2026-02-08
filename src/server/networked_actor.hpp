#pragma once

#include "../core/actor.hpp"
#include "../core/config.hpp"
#include "../core/logging.hpp"
#include "../net/player_connection.hpp"
#include "../world/room.hpp"
#include "../world/world_manager.hpp"

#include <memory>

// NetworkedPlayer class - combines Player with network connection
class NetworkedPlayer : public Player, public std::enable_shared_from_this<NetworkedPlayer> {
  public:
    NetworkedPlayer(std::shared_ptr<PlayerConnection> conn, std::string_view name)
        : Player(EntityId{9999UL}, name), connection_(conn) {
        // Room placement will be done after construction in initialize()
    }

    void initialize() { place_in_safe_room(); }

    void place_in_safe_room() {
        auto &world = WorldManager::instance();
        std::shared_ptr<Room> target_room = nullptr;

        // First, check if player already has a valid room (from save file)
        auto current = current_room();
        if (current) {
            target_room = current;
        }

        // If no current room or invalid, try WorldManager's starting room
        if (!target_room) {
            auto starting_room_id = world.get_start_room();
            if (starting_room_id.is_valid()) {
                target_room = world.get_room(starting_room_id);

                // Set this as the player's personal start room if they don't have one
                if (!start_room().is_valid()) {
                    set_start_room(starting_room_id);
                }
            }
        }

        // If starting room doesn't exist, try to find any available room
        if (!target_room) {
            target_room = world.get_first_available_room();
        }

        // If still no room available, log error but don't crash
        if (target_room) {
            set_current_room(target_room);
            target_room->add_actor(shared_from_this());
        } else {
            // This should rarely happen, but log it for debugging
            // The player will be in a "nowhere" state but won't crash the server
            Log::error("Could not place player '{}' in any room - no rooms available!", name());
        }
    }

    ~NetworkedPlayer() {
        // Clean up - remove from current room
        auto room = current_room();
        if (room) {
            room->remove_actor(id());
        }
    }

  private:
    void send_message(std::string_view message) override {
        if (connection_) {
            // Ensure message ends with proper line terminator for telnet
            std::string msg(message);
            if (!msg.empty() && !msg.ends_with('\n')) {
                msg += "\r\n";
            } else if (msg.ends_with('\n') && !msg.ends_with("\r\n")) {
                // Replace \n with \r\n for proper telnet line endings
                if (msg.back() == '\n') {
                    msg.pop_back();
                    msg += "\r\n";
                }
            }
            connection_->send_message(msg);
        }
    }

    void receive_message(std::string_view message) override {
        // For now, just echo back
        send_message(message);
    }

  private:
    std::shared_ptr<PlayerConnection> connection_;
};
