#pragma once

#include <optional>
#include <string>
#include <vector>

#include <pqxx/pqxx>

#include "core/ids.hpp"
#include "core/result.hpp"
#include "world/room.hpp"

/**
 * Database operations for the player housing system.
 *
 * All functions take a pqxx transaction reference and return Result<T>.
 * Follows the same pattern as WorldQueries and QuestQueries.
 */
namespace HousingQueries {

// ============================================================================
// Data structures
// ============================================================================

struct HouseData {
    int id;
    std::string character_id;
    std::optional<int> return_room_zone_id;
    std::optional<int> return_room_id;
    int entrance_room_zone_id;
    int entrance_room_id;
};

struct HouseRoomData {
    int id;
    int house_id;
    int local_index;
    std::string name;
    std::string description;
    std::string sector;
    bool is_peaceful;
    int base_light_level;
    int capacity;
};

struct HouseExitData {
    int from_room_id;
    int to_room_id;
    std::string direction;
};

struct HouseItemData {
    int id;
    int room_id;
    int object_zone_id;
    int object_id;
    int condition;
    std::string custom_values; // JSON string
};

struct HouseGuestData {
    std::string character_id;
    std::string character_name; // Joined from Characters table for display
    bool can_place;
};

// ============================================================================
// Load operations
// ============================================================================

/** Load a player's house by character ID. Returns nullopt if no house exists. */
Result<std::optional<HouseData>> load_house(pqxx::work &txn, const std::string &character_id);

/** Load all rooms for a house. */
Result<std::vector<HouseRoomData>> load_house_rooms(pqxx::work &txn, int house_id);

/** Load all exits within a house (joins through rooms). */
Result<std::vector<HouseExitData>> load_house_exits(pqxx::work &txn, int house_id);

/** Load all placed items in a house room. */
Result<std::vector<HouseItemData>> load_room_items(pqxx::work &txn, int room_id);

/** Load the guest list for a house. */
Result<std::vector<HouseGuestData>> load_house_guests(pqxx::work &txn, int house_id);

/** Check if a character has a house (lightweight query). */
Result<bool> has_house(pqxx::work &txn, const std::string &character_id);

/** Check if a character is on a house's guest list. */
Result<bool> is_guest_of(pqxx::work &txn, int house_id, const std::string &character_id);

// ============================================================================
// Mutation operations
// ============================================================================

/** Create a new house for a character. Returns the new house ID. */
Result<int> create_house(pqxx::work &txn, const std::string &character_id, int entrance_zone_id, int entrance_room_id);

/** Add a room to a house. Returns the new room's database ID. */
Result<int> add_room(pqxx::work &txn, int house_id, int local_index, const std::string &name,
                     const std::string &description);

/** Add a directional exit between two house rooms. */
Result<void> add_exit(pqxx::work &txn, int from_room_id, int to_room_id, const std::string &direction);

/** Update a room's name and description. */
Result<void> update_room_description(pqxx::work &txn, int room_id, const std::string &name,
                                     const std::string &description);

/** Place an item in a house room. Returns the placed item's database ID. */
Result<int> place_item(pqxx::work &txn, int room_id, int object_zone_id, int object_id, int condition);

/** Remove a placed item by its database ID. */
Result<void> remove_item(pqxx::work &txn, int item_id);

/** Count placed items in a room. */
Result<int> count_room_items(pqxx::work &txn, int room_id);

/** Save the player's return location (where to go on exit/crash). */
Result<void> save_return_location(pqxx::work &txn, int house_id, int zone_id, int room_id);

/** Clear the return location (player has exited the house). */
Result<void> clear_return_location(pqxx::work &txn, int house_id);

/** Add a guest to a house. */
Result<void> add_guest(pqxx::work &txn, int house_id, const std::string &character_id, bool can_place);

/** Remove a guest from a house. */
Result<void> remove_guest(pqxx::work &txn, int house_id, const std::string &character_id);

/** Delete a house and all its rooms/items/guests (cascading). */
Result<void> delete_house(pqxx::work &txn, int house_id);

/** Get all houses with non-null return locations (for crash recovery). */
Result<std::vector<HouseData>> load_stranded_houses(pqxx::work &txn);

/** Update a character's current room location directly (for crash recovery). */
Result<void> update_character_location(pqxx::work &txn, const std::string &character_id, int zone_id, int room_id);

/** Count rooms in a house. */
Result<int> count_house_rooms(pqxx::work &txn, int house_id);

} // namespace HousingQueries
