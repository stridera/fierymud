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
};

/**
 * @brief Null implementation of PlayerOutput for testing
 *
 * This captures all output for testing purposes without requiring network connections.
 */
class NullPlayerOutput : public PlayerOutput {
  public:
    void send_message(std::string_view message) override { captured_output += std::string(message) + "\n"; }

    void send_prompt(std::string_view prompt) override { captured_output += "[PROMPT: " + std::string(prompt) + "]\n"; }

    void send_line(std::string_view line) override { captured_output += std::string(line) + "\n"; }

    bool is_connected() const override { return true; }

    std::string remote_address() const override { return "test-harness"; }

    /** Get captured output for testing */
    const std::string &get_captured_output() const { return captured_output; }

    /** Clear captured output */
    void clear_captured_output() { captured_output.clear(); }

  private:
    std::string captured_output;
};