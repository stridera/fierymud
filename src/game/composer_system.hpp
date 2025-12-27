#pragma once

#include "../core/result.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// Forward declarations
class Player;
class PlayerConnection;

/**
 * @brief State machine states for the composer
 */
enum class ComposerState {
    Inactive,   // Not composing
    Composing,  // Actively composing text
    Completed,  // Finished composing (. entered)
    Cancelled   // Cancelled composing (~q entered)
};

/**
 * @brief Result of a completed composition
 */
struct ComposerResult {
    bool success{false};                // true if completed, false if cancelled
    std::vector<std::string> lines;     // Individual lines entered
    std::string combined_text;          // Lines joined with \n

    /** Get combined text for database storage */
    std::string to_database_text() const { return combined_text; }
};

/**
 * @brief Configuration for a composer session
 */
struct ComposerConfig {
    std::string header_message{"Enter text (. to save, ~q to cancel, ~h for help):"};
    std::string save_message{"Text saved."};
    std::string cancel_message{"Cancelled."};
    std::string prompt{">"};
    int max_lines{100};
    int max_line_length{2000};  // Prevent abuse
};

/**
 * @brief Multi-line text composer system
 *
 * Allows players to enter multi-line text for mail, descriptions, notes, etc.
 * While composing, the player is "dead to the world" - game output pauses
 * and all input goes to the composer.
 *
 * Commands:
 *   .   - Save and finish composing
 *   ~q  - Cancel and discard all text
 *   ~p  - Preview current text
 *   ~c  - Clear all text and continue
 *   ~h  - Show help
 *
 * Pattern follows LoginSystem - state machine with callback-based completion.
 */
class ComposerSystem {
  public:
    using CompletionCallback = std::function<void(ComposerResult)>;

    /**
     * @brief Create a composer for a player
     * @param player Weak reference to the player composing
     * @param config Configuration for this composer session
     */
    ComposerSystem(std::weak_ptr<Player> player, ComposerConfig config = {});
    ~ComposerSystem() = default;

    // Non-copyable, movable
    ComposerSystem(const ComposerSystem&) = delete;
    ComposerSystem& operator=(const ComposerSystem&) = delete;
    ComposerSystem(ComposerSystem&&) = default;
    ComposerSystem& operator=(ComposerSystem&&) = default;

    /**
     * @brief Start the composer session
     * Displays header and help instructions to player
     */
    void start();

    /**
     * @brief Process a line of input from the player
     * @param input The raw input line from the player
     *
     * Handles special commands (~q, ~p, ~c, ~h) and regular text input.
     * When the player enters "." alone, completion callback is invoked.
     */
    void process_input(std::string_view input);

    /**
     * @brief Set the callback to invoke when composing is complete
     * @param callback Function to call with the result
     */
    void set_completion_callback(CompletionCallback callback) { completion_callback_ = std::move(callback); }

    /**
     * @brief Get current composer state
     */
    ComposerState state() const { return state_; }

    /**
     * @brief Check if composer is active
     */
    bool is_active() const { return state_ == ComposerState::Composing; }

    /**
     * @brief Get current lines entered
     */
    const std::vector<std::string>& lines() const { return lines_; }

    /**
     * @brief Get current line count
     */
    size_t line_count() const { return lines_.size(); }

  private:
    // Command handlers
    void cmd_save();       // "." - Save and complete
    void cmd_cancel();     // "~q" - Cancel and discard
    void cmd_preview();    // "~p" - Show current text
    void cmd_clear();      // "~c" - Clear and continue
    void cmd_help();       // "~h" - Show help

    // Helper methods
    void send_message(std::string_view message);
    void send_prompt();
    void finish(bool success);

    /**
     * @brief Build combined text from lines
     */
    std::string build_combined_text() const;

    // State
    ComposerState state_{ComposerState::Inactive};
    std::vector<std::string> lines_;
    std::weak_ptr<Player> player_;
    ComposerConfig config_;
    CompletionCallback completion_callback_;
};
