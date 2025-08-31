/***************************************************************************
 *   File: src/game/player_output.hpp          Part of FieryMUD *
 *  Usage: Player output interface for dependency inversion                 *
 ***************************************************************************/

#pragma once

#include <string_view>

/**
 * @brief Abstract interface for player output
 *
 * This interface allows Player to send messages without directly depending
 * on PlayerConnection, enabling better testability and loose coupling.
 */
class PlayerOutput {
  public:
    virtual ~PlayerOutput() = default;

    /** Send a message to the player */
    virtual void send_message(std::string_view message) = 0;

    /** Send a prompt to the player */
    virtual void send_prompt(std::string_view prompt) = 0;

    /** Send a line with newline to the player */
    virtual void send_line(std::string_view line) = 0;

    /** Check if the output is still connected/valid */
    virtual bool is_connected() const = 0;

    /** Get remote address for logging purposes */
    virtual std::string remote_address() const = 0;

    /** Disconnect the player with optional reason */
    virtual void disconnect(std::string_view reason = "") = 0;
};

// Removed NullPlayerOutput class - not used, tests use MockGameSession instead