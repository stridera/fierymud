#pragma once

#include <chrono>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/ids.hpp"
#include "core/result.hpp"
#include "housing/housing_queries.hpp"
#include "housing/housing_room.hpp"

class Actor;
class Player;
class Object;

/**
 * Manages player housing instances.
 *
 * Houses are loaded lazily when a player enters and unloaded
 * after a period of no occupancy. Rooms are registered with
 * WorldManager so existing commands work transparently.
 */
class HousingManager {
  public:
    static HousingManager &instance();

    /** Initialize manager and run crash recovery. */
    Result<void> initialize();

    /** Shutdown — save dirty houses and unload all. */
    void shutdown();

    // ========================================================================
    // House lifecycle
    // ========================================================================

    /** Create a new house for a character. */
    Result<void> create_house(const std::string &character_id, int entrance_zone_id, int entrance_room_id);

    /** Check if a character has a house. */
    bool has_house(const std::string &character_id);

    /** Delete a house (admin command). */
    Result<void> revoke_house(const std::string &character_id);

    // ========================================================================
    // Enter / Exit
    // ========================================================================

    /** Enter a house (owner or guest). Loads house if needed, teleports player. */
    Result<void> enter_house(std::shared_ptr<Player> player, const std::string &owner_character_id);

    /** Exit a house. Returns player to entrance room. */
    Result<void> exit_house(std::shared_ptr<Player> player);

    // ========================================================================
    // Room management
    // ========================================================================

    /** Add a new room to a house in the given direction. Returns the new room's db ID. */
    Result<int> expand_house(const std::string &character_id, int from_room_db_id, Direction direction,
                             const std::string &room_name);

    /** Set a room's description (player must be in the room). */
    Result<void> set_room_description(int room_db_id, std::string_view description);

    /** Set a room's name. */
    Result<void> set_room_name(int room_db_id, std::string_view name);

    /** Get the room count for a house. */
    int room_count(const std::string &character_id) const;

    /** Maximum rooms per house */
    static constexpr int MAX_ROOMS = 10;

    // ========================================================================
    // Item placement
    // ========================================================================

    /** Place an item from inventory into the current housing room. */
    Result<void> place_item(std::shared_ptr<Player> player, std::shared_ptr<Object> item);

    /** Remove a placed item and give it back to the player. */
    Result<std::shared_ptr<Object>> remove_placed_item(std::shared_ptr<Player> player, int placed_item_db_id);

    // ========================================================================
    // Guest management
    // ========================================================================

    Result<void> add_guest(const std::string &owner_character_id, const std::string &guest_character_id,
                           bool can_place = false);
    Result<void> remove_guest(const std::string &owner_character_id, const std::string &guest_character_id);
    bool is_guest(const std::string &owner_character_id, const std::string &guest_character_id);
    Result<std::vector<HousingQueries::HouseGuestData>> get_guests(const std::string &owner_character_id);

    /** Check if guest_id is on owner_id's guest list. Checks cache first, falls back to DB. */
    Result<bool> check_guest_access(const std::string &owner_id, const std::string &guest_id);

    // ========================================================================
    // Queries
    // ========================================================================

    /** Check if an EntityId is a housing room. */
    static bool is_housing_room(EntityId room_id) { return HousingRoom::is_housing_room(room_id); }

    /** Get the HousingRoom for an EntityId, if loaded. */
    std::shared_ptr<HousingRoom> get_housing_room(EntityId room_id) const;

    /** Get the owner character ID for a loaded housing room. */
    std::string get_room_owner(EntityId room_id) const;

    /** Get house info for display. */
    Result<HousingQueries::HouseData> get_house_info(const std::string &character_id);

    /** Check if a player is currently in any housing room. */
    bool is_in_housing(EntityId current_room) const { return is_housing_room(current_room); }

    /** Check if a player is the owner of the housing room they're in. */
    bool is_owner_of_room(const std::string &character_id, EntityId room_id) const;

    /** Check if a player is a god (bypasses ownership checks except chest). */
    static bool is_god_override(const Player *player);

    // ========================================================================
    // Periodic maintenance
    // ========================================================================

    /** Save any modified house data. Called from game loop. */
    void save_dirty_houses();

    /** Unload houses with no occupants after timeout. */
    void check_unload_timers();

  private:
    HousingManager() = default;

    /** Track an active (loaded) house */
    struct ActiveHouse {
        int house_id = 0;
        std::string character_id;
        HousingQueries::HouseData data;
        std::unordered_map<int, std::shared_ptr<HousingRoom>> rooms; // db_id -> room
        std::unordered_set<std::string> guests_cache;
        bool dirty = false;
        std::chrono::steady_clock::time_point last_occupant_left;
        int occupant_count = 0;
    };

    /** How long to keep an empty house loaded */
    static constexpr auto UNLOAD_TIMEOUT = std::chrono::minutes(5);

    // Active houses indexed by owner character_id
    std::unordered_map<std::string, ActiveHouse> active_houses_;

    // Reverse lookup: housing room EntityId -> owner character_id
    std::unordered_map<EntityId, std::string, EntityId::Hash> room_to_owner_;

    mutable std::shared_mutex mutex_;

    // ========================================================================
    // Internal helpers
    // ========================================================================

    /** Load a house from database and register its rooms with WorldManager. */
    Result<void> load_house(const std::string &character_id);

    /** Unload a house — remove rooms from WorldManager. */
    void unload_house(const std::string &character_id);

    /** Register a housing room with WorldManager. */
    void register_room(std::shared_ptr<HousingRoom> room, const std::string &owner_id);

    /** Unregister a housing room from WorldManager. */
    void unregister_room(EntityId room_id);

    /** Crash recovery: move stranded players back to their return locations. */
    Result<void> evacuate_stranded_players();

    /** Get (or load) the active house for a character. */
    Result<ActiveHouse *> get_or_load_house(const std::string &character_id);

    /** Convert DB sector string to SectorType. */
    static SectorType parse_sector(const std::string &sector_str);

    /** Convert Direction enum to DB string. */
    static std::string direction_to_db_string(Direction dir);
};
