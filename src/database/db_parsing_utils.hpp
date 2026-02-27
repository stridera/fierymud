/**
 * @file db_parsing_utils.hpp
 * @brief Utility functions for parsing PostgreSQL data types
 *
 * These functions are used by world_queries.cpp to parse PostgreSQL-specific
 * data formats like arrays and enums. Exposed in header for unit testing.
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../core/object.hpp"
#include "../database/generated/db_object.hpp"
#include "../world/room.hpp"

namespace DbParsingUtils {

/**
 * Convert a database ObjectFlag enum to the game ObjectFlag enum.
 *
 * These enums have different underlying values so a direct static_cast is WRONG.
 * Some DB flags (Buoyant, Vehicle, Soulbound) have no game equivalent yet.
 *
 * @param f The database ObjectFlag
 * @return Optional game ObjectFlag, nullopt if no game equivalent
 */
std::optional<ObjectFlag> object_flag_to_game(db::ObjectFlag f);

/**
 * Parse a PostgreSQL array string into a vector of strings.
 *
 * PostgreSQL arrays are formatted as: {elem1,elem2,elem3}
 * Handles quoted strings and escaped characters.
 *
 * @param pg_array The PostgreSQL array string (e.g., "{NORTH,SOUTH}")
 * @return Vector of parsed string elements
 */
std::vector<std::string> parse_pg_array(const std::string &pg_array);

/**
 * Parse sector type from database string.
 *
 * Maps database enum values like "INSIDE", "CITY", "FIELD" to SectorType.
 *
 * @param sector_str The sector type string from database
 * @return Optional SectorType, nullopt if unknown
 */
std::optional<SectorType> sector_from_db_string(const std::string &sector_str);

// room_flag_from_db_string REMOVED - RoomFlag replaced by baseLightLevel and Lua restrictions

} // namespace DbParsingUtils
