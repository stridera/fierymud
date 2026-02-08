#pragma once

#include "core/result.hpp"
#include <pqxx/pqxx>
#include <string_view>
#include <memory>

class Player;

/**
 * Player/Character query layer for PostgreSQL database.
 *
 * Provides SQL queries for loading player characters from the
 * Characters table with their stats, equipment, and inventory.
 *
 * All queries use prepared statements for security and performance.
 */
namespace PlayerQueries {

/**
 * Load a player character by name (case-insensitive).
 *
 * @param txn Database transaction
 * @param name Character name to search for
 * @return Result containing Player pointer or error
 */
Result<std::unique_ptr<Player>> load_player_by_name(
    pqxx::work& txn,
    std::string_view name);

/**
 * Load a player character by ID.
 *
 * @param txn Database transaction
 * @param player_id Character ID (UUID string)
 * @return Result containing Player pointer or error
 */
Result<std::unique_ptr<Player>> load_player_by_id(
    pqxx::work& txn,
    std::string_view player_id);

/**
 * Check if a player character exists by name.
 *
 * @param txn Database transaction
 * @param name Character name to check
 * @return true if player exists, false otherwise
 */
bool player_exists(pqxx::work& txn, std::string_view name);

/**
 * Get count of all player characters in database.
 *
 * @param txn Database transaction
 * @return Number of characters
 */
int get_player_count(pqxx::work& txn);

} // namespace PlayerQueries
