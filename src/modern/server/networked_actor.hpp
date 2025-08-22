#pragma once

#include "../core/actor.hpp"
#include "player_connection.hpp"
#include "../world/world_manager.hpp"
#include "../world/room.hpp"
#include <memory>

// NetworkedPlayer class - combines Player with network connection
class NetworkedPlayer : public Player, public std::enable_shared_from_this<NetworkedPlayer> {
public:
    NetworkedPlayer(std::shared_ptr<PlayerConnection> conn, std::string_view name)
        : Player(EntityId{9999UL}, name), connection_(conn) {
        // Room placement will be done after construction in initialize()
    }

    void initialize() {
        // Place the player in the starting room (room 100)
        auto& world = WorldManager::instance();
        auto starting_room = world.get_room(EntityId{100UL});
        if (starting_room) {
            set_current_room(starting_room);
            starting_room->add_actor(shared_from_this());
        }
    }

    ~NetworkedPlayer() {
        // Clean up - remove from current room
        auto room = current_room();
        if (room) {
            room->remove_actor(id());
        }
    }

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
            connection_->send(msg);
        }
    }

    void receive_message(std::string_view message) override {
        // For now, just echo back
        send_message(message);
    }

private:
    std::shared_ptr<PlayerConnection> connection_;
};
