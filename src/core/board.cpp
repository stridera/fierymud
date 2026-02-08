/**
 * @file board.cpp
 * @brief Implementation of the modern bulletin board system.
 */

#include "board.hpp"

#include "../database/connection_pool.hpp"
#include "../database/world_queries.hpp"
#include "logging.hpp"

#include <algorithm>

namespace {
// Global board system instance
std::unique_ptr<BoardSystem> g_board_system;
} // namespace

BoardSystem &board_system() {
    if (!g_board_system) {
        g_board_system = std::make_unique<BoardSystem>();
    }
    return *g_board_system;
}

void BoardSystem::load_boards() {
    auto logger = Log::game();
    logger->info("Loading boards from database...");

    boards_.clear();

    auto result = ConnectionPool::instance().execute([this, logger](pqxx::work &txn) -> Result<void> {
        auto boards_result = WorldQueries::load_all_boards(txn);

        if (!boards_result) {
            return std::unexpected(boards_result.error());
        }

        auto &db_boards = *boards_result;
        for (auto &db_board : db_boards) {
            auto board = std::make_unique<BoardData>();
            board->id = db_board.id;
            board->alias = std::move(db_board.alias);
            board->title = std::move(db_board.title);
            board->locked = db_board.locked;

            // Convert messages
            for (auto &db_msg : db_board.messages) {
                BoardMessage msg;
                msg.id = db_msg.id;
                msg.board_id = db_msg.board_id;
                msg.poster = std::move(db_msg.poster);
                msg.poster_level = db_msg.poster_level;
                msg.posted_at = db_msg.posted_at;
                msg.subject = std::move(db_msg.subject);
                msg.content = std::move(db_msg.content);
                msg.sticky = db_msg.sticky;

                // Convert edits
                for (auto &db_edit : db_msg.edits) {
                    BoardMessageEdit edit;
                    edit.id = db_edit.id;
                    edit.editor = std::move(db_edit.editor);
                    edit.edited_at = db_edit.edited_at;
                    msg.edits.push_back(std::move(edit));
                }

                board->messages.push_back(std::move(msg));
            }

            boards_.push_back(std::move(board));
        }

        return {};
    });

    if (!result) {
        logger->error("Failed to load boards: {}", result.error().message);
        return;
    }

    logger->info("Loaded {} boards with messages", boards_.size());
}

const BoardData *BoardSystem::get_board(int board_id) const {
    for (const auto &board : boards_) {
        if (board->id == board_id) {
            return board.get();
        }
    }
    return nullptr;
}

const BoardData *BoardSystem::get_board_by_alias(const std::string &alias) const {
    for (const auto &board : boards_) {
        if (board->alias == alias) {
            return board.get();
        }
    }
    return nullptr;
}

bool BoardSystem::reload_board(int board_id) {
    auto logger = Log::game();
    logger->info("Reloading board {}", board_id);

    auto result = ConnectionPool::instance().execute([this, board_id, logger](pqxx::work &txn) -> Result<bool> {
        auto load_result = WorldQueries::load_board(txn, board_id);

        if (!load_result) {
            return std::unexpected(load_result.error());
        }

        auto &db_board_opt = *load_result;
        if (!db_board_opt) {
            logger->warn("Board {} not found in database", board_id);
            return false;
        }

        auto &db_board = *db_board_opt;

        // Find and replace existing board, or add new one
        auto it = std::find_if(boards_.begin(), boards_.end(), [board_id](const auto &b) { return b->id == board_id; });

        auto board = std::make_unique<BoardData>();
        board->id = db_board.id;
        board->alias = std::move(db_board.alias);
        board->title = std::move(db_board.title);
        board->locked = db_board.locked;

        // Convert messages
        for (auto &db_msg : db_board.messages) {
            BoardMessage msg;
            msg.id = db_msg.id;
            msg.board_id = db_msg.board_id;
            msg.poster = std::move(db_msg.poster);
            msg.poster_level = db_msg.poster_level;
            msg.posted_at = db_msg.posted_at;
            msg.subject = std::move(db_msg.subject);
            msg.content = std::move(db_msg.content);
            msg.sticky = db_msg.sticky;

            for (auto &db_edit : db_msg.edits) {
                BoardMessageEdit edit;
                edit.id = db_edit.id;
                edit.editor = std::move(db_edit.editor);
                edit.edited_at = db_edit.edited_at;
                msg.edits.push_back(std::move(edit));
            }

            board->messages.push_back(std::move(msg));
        }

        if (it != boards_.end()) {
            *it = std::move(board);
        } else {
            boards_.push_back(std::move(board));
        }

        return true;
    });

    if (!result) {
        logger->error("Failed to reload board {}: {}", board_id, result.error().message);
        return false;
    }

    if (*result) {
        logger->info("Successfully reloaded board {}", board_id);
    }
    return *result;
}

void BoardSystem::reload_all() { load_boards(); }

Result<int> BoardSystem::post_message(int board_id, const std::string &poster, int poster_level,
                                      const std::string &subject, const std::string &content) {
    auto logger = Log::game();

    auto result = ConnectionPool::instance().execute(
        [board_id, &poster, poster_level, &subject, &content](pqxx::work &txn) -> Result<int> {
            return WorldQueries::post_board_message(txn, board_id, poster, poster_level, subject, content);
        });

    if (!result) {
        logger->error("Failed to post message to board {}: {}", board_id, result.error().message);
        return std::unexpected(result.error());
    }

    // Reload the board to get the new message in cache
    reload_board(board_id);

    logger->info("Posted message {} to board {} by {}", *result, board_id, poster);
    return *result;
}

Result<bool> BoardSystem::remove_message(int board_id, int message_index) {
    auto logger = Log::game();

    // Get the message to find its database ID
    const auto *msg = get_message(board_id, message_index);
    if (!msg) {
        return std::unexpected(Error{ErrorCode::NotFound, "Message not found"});
    }

    int message_id = msg->id;

    auto result = ConnectionPool::instance().execute(
        [message_id](pqxx::work &txn) -> Result<bool> { return WorldQueries::delete_board_message(txn, message_id); });

    if (!result) {
        logger->error("Failed to delete message {} from board {}: {}", message_index, board_id, result.error().message);
        return std::unexpected(result.error());
    }

    // Reload the board to remove the message from cache
    reload_board(board_id);

    logger->info("Deleted message {} from board {}", message_index, board_id);
    return *result;
}

const BoardMessage *BoardSystem::get_message(int board_id, int message_index) const {
    const auto *board = get_board(board_id);
    if (!board) {
        return nullptr;
    }

    // message_index is 1-based, displayed newest first (highest number = newest)
    // So message N maps to index (size - N) in the array
    if (message_index < 1 || message_index > static_cast<int>(board->messages.size())) {
        return nullptr;
    }

    size_t index = board->messages.size() - static_cast<size_t>(message_index);
    return &board->messages[index];
}
