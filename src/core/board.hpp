#pragma once

/**
 * @file board.hpp
 * @brief Modern bulletin board system for FieryMUD.
 *
 * Boards are message containers that can be accessed through board objects
 * placed in rooms. Each board object has a board_number that references
 * a specific board's messages and permissions.
 */

#include "result.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

/**
 * Record of an edit made to a board message.
 */
struct BoardMessageEdit {
    int id;
    std::string editor; // Character name who edited
    std::chrono::system_clock::time_point edited_at;
};

/**
 * A single message posted to a board.
 */
struct BoardMessage {
    int id;
    int board_id;
    std::string poster; // Character name who posted
    int poster_level;   // Poster's level at time of posting
    std::chrono::system_clock::time_point posted_at;
    std::string subject;
    std::string content; // Message body with color codes
    bool sticky;         // Sticky messages float to top
    std::vector<BoardMessageEdit> edits;
};

/**
 * Board privilege types.
 * Each board can have different rules for each privilege.
 */
enum class BoardPrivilege {
    Read = 0,        // Can read messages
    WriteNew = 1,    // Can post new messages
    RemoveOwn = 2,   // Can remove own messages
    EditOwn = 3,     // Can edit own messages
    RemoveAny = 4,   // Can remove any message
    EditAny = 5,     // Can edit any message
    WriteSticky = 6, // Can write sticky messages
    Lock = 7         // Can lock/unlock the board
};

constexpr int NUM_BOARD_PRIVILEGES = 8;

/**
 * A bulletin board containing messages.
 */
struct BoardData {
    int id;            // Board ID (matches object's board_number)
    std::string alias; // Short name used as filename (e.g., "mortal", "god")
    std::string title; // Display title
    bool locked;       // If true, no new posts allowed

    // Messages sorted by sticky (desc), then posted_at (desc)
    std::vector<BoardMessage> messages;

    // TODO: Privilege rules (stored as JSON in database)
    // For now, privileges are handled by the legacy code
};

/**
 * Board system manager.
 * Handles loading boards from database and caching for in-game access.
 */
class BoardSystem {
  public:
    BoardSystem() = default;
    ~BoardSystem() = default;

    // Non-copyable
    BoardSystem(const BoardSystem &) = delete;
    BoardSystem &operator=(const BoardSystem &) = delete;

    /**
     * Load all boards from the database.
     * Called during world initialization.
     */
    void load_boards();

    /**
     * Get a board by its ID.
     * @param board_id The board's database ID
     * @return Pointer to board data, or nullptr if not found
     */
    const BoardData *get_board(int board_id) const;

    /**
     * Get a board by its alias.
     * @param alias The board's alias (e.g., "mortal", "god")
     * @return Pointer to board data, or nullptr if not found
     */
    const BoardData *get_board_by_alias(const std::string &alias) const;

    /**
     * Get the number of loaded boards.
     */
    size_t board_count() const { return boards_.size(); }

    /**
     * Reload a specific board from the database.
     * @param board_id The board's database ID
     * @return true if reload succeeded
     */
    bool reload_board(int board_id);

    /**
     * Reload all boards from the database.
     */
    void reload_all();

    /**
     * Post a new message to a board.
     * @param board_id The board's database ID
     * @param poster Character name of the poster
     * @param poster_level Poster's current level
     * @param subject Message subject line
     * @param content Message body
     * @return Message ID on success, error on failure
     */
    Result<int> post_message(int board_id, const std::string &poster, int poster_level, const std::string &subject,
                             const std::string &content);

    /**
     * Remove a message from a board.
     * @param board_id The board's database ID
     * @param message_index 1-based message index (as displayed to users)
     * @return true if deleted, false if not found
     */
    Result<bool> remove_message(int board_id, int message_index);

    /**
     * Get a message from a board by its display index.
     * @param board_id The board's database ID
     * @param message_index 1-based message index
     * @return Pointer to message, or nullptr if not found
     */
    const BoardMessage *get_message(int board_id, int message_index) const;

  private:
    std::vector<std::unique_ptr<BoardData>> boards_;
};

// Global board system instance
extern BoardSystem &board_system();
