#include "housing/housing_manager.hpp"

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "core/logging.hpp"
#include "core/object.hpp"
#include "core/player.hpp"
#include "database/connection_pool.hpp"
#include "world/world_manager.hpp"

// ============================================================================
// Singleton
// ============================================================================

HousingManager &HousingManager::instance() {
    static HousingManager mgr;
    return mgr;
}

// ============================================================================
// Initialization / Shutdown
// ============================================================================

Result<void> HousingManager::initialize() {
    Log::info("Initializing housing manager");

    auto result = evacuate_stranded_players();
    if (!result) {
        Log::warn("Failed to evacuate stranded players: {}", result.error().message);
    }

    Log::info("Housing manager initialized");
    return {};
}

void HousingManager::shutdown() {
    Log::info("Shutting down housing manager");

    save_dirty_houses();

    // Unload all active houses
    std::unique_lock lock(mutex_);
    std::vector<std::string> owners;
    for (const auto &[owner, _] : active_houses_) {
        owners.push_back(owner);
    }
    lock.unlock();

    for (const auto &owner : owners) {
        unload_house(owner);
    }
}

// ============================================================================
// House lifecycle
// ============================================================================

Result<void> HousingManager::create_house(const std::string &character_id, int entrance_zone_id, int entrance_room_id) {
    return ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
        // Check if already has a house
        auto existing = HousingQueries::has_house(txn, character_id);
        if (!existing)
            return std::unexpected(existing.error());
        if (*existing)
            return std::unexpected(Errors::AlreadyExists("house"));

        auto house_id = HousingQueries::create_house(txn, character_id, entrance_zone_id, entrance_room_id);
        if (!house_id)
            return std::unexpected(house_id.error());

        Log::info("Created house {} for character {}", *house_id, character_id);
        return {};
    });
}

bool HousingManager::has_house(const std::string &character_id) {
    // Check cache first
    {
        std::shared_lock lock(mutex_);
        if (active_houses_.contains(character_id))
            return true;
    }

    // Check database
    auto result = ConnectionPool::instance().execute(
        [&](pqxx::work &txn) { return HousingQueries::has_house(txn, character_id); });
    return result.has_value() && *result;
}

Result<void> HousingManager::revoke_house(const std::string &character_id) {
    // Unload if active
    unload_house(character_id);

    return ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
        auto house = HousingQueries::load_house(txn, character_id);
        if (!house)
            return std::unexpected(house.error());
        if (!house->has_value())
            return std::unexpected(Errors::NotFound("house"));

        return HousingQueries::delete_house(txn, house->value().id);
    });
}

// ============================================================================
// Enter / Exit
// ============================================================================

Result<void> HousingManager::enter_house(std::shared_ptr<Player> player, const std::string &owner_character_id) {
    // Load the house if not already loaded
    auto house_result = get_or_load_house(owner_character_id);
    if (!house_result)
        return std::unexpected(house_result.error());

    auto *house = *house_result;

    // Find the foyer (local_index 0)
    std::shared_ptr<HousingRoom> foyer;
    for (const auto &[db_id, room] : house->rooms) {
        if (room->is_foyer()) {
            foyer = room;
            break;
        }
    }

    if (!foyer) {
        return std::unexpected(Errors::InvalidState("house has no foyer"));
    }

    // Save player's current location as return point
    auto player_actor = std::static_pointer_cast<Actor>(player);
    auto current_room_ptr = player_actor->current_room();
    auto current_room = current_room_ptr ? current_room_ptr->id() : INVALID_ENTITY_ID;
    if (current_room.is_valid() && !is_housing_room(current_room)) {
        ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
            return HousingQueries::save_return_location(txn, house->house_id, current_room.zone_id(),
                                                        current_room.local_id());
        });
    }

    // Move player to foyer
    auto move_result = WorldManager::instance().move_actor_to_room(player_actor, foyer->id());
    if (!move_result.success) {
        return std::unexpected(Errors::InvalidState(move_result.failure_reason));
    }

    {
        std::unique_lock lock(mutex_);
        house->occupant_count++;
    }

    Log::info("Player entered house of {}", owner_character_id);
    return {};
}

Result<void> HousingManager::exit_house(std::shared_ptr<Player> player) {
    auto player_actor = std::static_pointer_cast<Actor>(player);
    auto current_room_ptr = player_actor->current_room();
    auto current_room_id = current_room_ptr ? current_room_ptr->id() : INVALID_ENTITY_ID;

    if (!is_housing_room(current_room_id)) {
        return std::unexpected(Errors::InvalidState("not in a housing room"));
    }

    // Find the owner of this room
    std::string owner_id;
    EntityId destination = INVALID_ENTITY_ID;
    {
        std::shared_lock lock(mutex_);
        auto it = room_to_owner_.find(current_room_id);
        if (it == room_to_owner_.end()) {
            return std::unexpected(Errors::NotFound("housing room owner"));
        }
        owner_id = it->second;

        auto house_it = active_houses_.find(owner_id);
        if (house_it != active_houses_.end()) {
            auto &house = house_it->second;
            // Use return location if available, otherwise use entrance
            if (house.data.return_room_zone_id && house.data.return_room_id) {
                destination = EntityId(static_cast<uint32_t>(*house.data.return_room_zone_id),
                                       static_cast<uint32_t>(*house.data.return_room_id));
            } else {
                destination = EntityId(static_cast<uint32_t>(house.data.entrance_room_zone_id),
                                       static_cast<uint32_t>(house.data.entrance_room_id));
            }
        }
    }

    if (!destination.is_valid()) {
        return std::unexpected(Errors::InvalidState("no valid exit destination"));
    }

    // Verify destination room exists
    auto dest_room = WorldManager::instance().get_room(destination);
    if (!dest_room) {
        // Fall back to entrance
        std::shared_lock lock(mutex_);
        auto house_it = active_houses_.find(owner_id);
        if (house_it != active_houses_.end()) {
            destination = EntityId(static_cast<uint32_t>(house_it->second.data.entrance_room_zone_id),
                                   static_cast<uint32_t>(house_it->second.data.entrance_room_id));
        }
        dest_room = WorldManager::instance().get_room(destination);
        if (!dest_room) {
            // Last resort: world start room
            destination = WorldManager::instance().get_start_room();
        }
    }

    // Move player out
    auto move_result = WorldManager::instance().move_actor_to_room(player_actor, destination);
    if (!move_result.success) {
        return std::unexpected(Errors::InvalidState(move_result.failure_reason));
    }

    // Clear return location
    ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
        auto house = HousingQueries::load_house(txn, owner_id);
        if (house && house->has_value()) {
            return HousingQueries::clear_return_location(txn, house->value().id);
        }
        return {};
    });

    // Decrement occupant count
    {
        std::unique_lock lock(mutex_);
        auto house_it = active_houses_.find(owner_id);
        if (house_it != active_houses_.end()) {
            house_it->second.occupant_count--;
            if (house_it->second.occupant_count <= 0) {
                house_it->second.occupant_count = 0;
                house_it->second.last_occupant_left = std::chrono::steady_clock::now();
            }
        }
    }

    return {};
}

// ============================================================================
// Room management
// ============================================================================

Result<int> HousingManager::expand_house(const std::string &character_id, int from_room_db_id, Direction direction,
                                         const std::string &room_name) {
    return ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<int> {
        auto house = HousingQueries::load_house(txn, character_id);
        if (!house || !house->has_value())
            return std::unexpected(Errors::NotFound("house"));

        int house_id = house->value().id;

        // Check room count
        auto count = HousingQueries::count_house_rooms(txn, house_id);
        if (!count)
            return std::unexpected(count.error());
        if (*count >= MAX_ROOMS)
            return std::unexpected(
                Errors::InvalidState(fmt::format("house already has maximum rooms ({})", MAX_ROOMS)));

        int new_index = *count; // Next sequential index

        // Create the new room
        auto new_room_id = HousingQueries::add_room(txn, house_id, new_index, room_name, "An empty room.");
        if (!new_room_id)
            return std::unexpected(new_room_id.error());

        // Create exit from source room to new room
        auto dir_str = direction_to_db_string(direction);
        auto exit_result = HousingQueries::add_exit(txn, from_room_db_id, *new_room_id, dir_str);
        if (!exit_result)
            return std::unexpected(exit_result.error());

        // Create reverse exit
        auto reverse_dir = RoomUtils::get_opposite_direction(direction);
        auto reverse_str = direction_to_db_string(reverse_dir);
        auto reverse_result = HousingQueries::add_exit(txn, *new_room_id, from_room_db_id, reverse_str);
        if (!reverse_result)
            return std::unexpected(reverse_result.error());

        // If the house is loaded, hot-add the room
        {
            std::unique_lock lock(mutex_);
            auto house_it = active_houses_.find(character_id);
            if (house_it != active_houses_.end()) {
                auto room = HousingRoom::create(*new_room_id, house_id, new_index, character_id, room_name,
                                                "An empty room.", SectorType::Inside);
                if (room) {
                    // Set up exits
                    auto from_room_it = house_it->second.rooms.find(from_room_db_id);
                    if (from_room_it != house_it->second.rooms.end()) {
                        ExitInfo exit_to_new;
                        exit_to_new.to_room = (*room)->id();
                        from_room_it->second->set_exit(direction, exit_to_new);

                        ExitInfo exit_back;
                        exit_back.to_room = from_room_it->second->id();
                        (*room)->set_exit(reverse_dir, exit_back);
                    }

                    register_room(*room, character_id);
                    house_it->second.rooms[*new_room_id] = std::move(*room);
                }
            }
        }

        return *new_room_id;
    });
}

Result<void> HousingManager::set_room_description(int room_db_id, std::string_view description) {
    // Update in-memory
    auto entity_id = HousingRoom::make_entity_id(room_db_id);
    auto room = get_housing_room(entity_id);
    if (room) {
        room->set_custom_description(description);
    }

    // Persist
    return ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
        std::string name = room ? std::string(room->name()) : "";
        return HousingQueries::update_room_description(txn, room_db_id, name, std::string(description));
    });
}

Result<void> HousingManager::set_room_name(int room_db_id, std::string_view name) {
    auto entity_id = HousingRoom::make_entity_id(room_db_id);
    auto room = get_housing_room(entity_id);
    if (room) {
        room->set_custom_name(name);
    }

    return ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
        std::string desc = room ? room->get_room_description() : "";
        return HousingQueries::update_room_description(txn, room_db_id, std::string(name), desc);
    });
}

int HousingManager::room_count(const std::string &character_id) const {
    std::shared_lock lock(mutex_);
    auto it = active_houses_.find(character_id);
    if (it != active_houses_.end()) {
        return static_cast<int>(it->second.rooms.size());
    }
    return 0;
}

// ============================================================================
// Item placement
// ============================================================================

Result<void> HousingManager::place_item(std::shared_ptr<Player> player, std::shared_ptr<Object> item) {
    auto player_actor = std::static_pointer_cast<Actor>(player);
    auto current_room_ptr = player_actor->current_room();
    auto room_id = current_room_ptr ? current_room_ptr->id() : INVALID_ENTITY_ID;
    auto room = get_housing_room(room_id);
    if (!room) {
        return std::unexpected(Errors::InvalidState("not in a housing room"));
    }

    if (room->placed_item_count() >= HousingRoom::MAX_ITEMS_PER_ROOM) {
        return std::unexpected(
            Errors::InvalidState(fmt::format("room already has maximum items ({})", HousingRoom::MAX_ITEMS_PER_ROOM)));
    }

    // Persist the placement
    auto result = ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<int> {
        return HousingQueries::place_item(txn, room->db_id(), item->id().zone_id(), item->id().local_id(), 100);
    });

    if (!result)
        return std::unexpected(result.error());

    // Add to room's placed items
    room->add_placed_item(item, *result);

    return {};
}

Result<std::shared_ptr<Object>> HousingManager::remove_placed_item(std::shared_ptr<Player> player,
                                                                   int placed_item_db_id) {
    auto player_actor = std::static_pointer_cast<Actor>(player);
    auto current_room_ptr = player_actor->current_room();
    auto room_id = current_room_ptr ? current_room_ptr->id() : INVALID_ENTITY_ID;
    auto room = get_housing_room(room_id);
    if (!room) {
        return std::unexpected(Errors::InvalidState("not in a housing room"));
    }

    // Remove from database
    auto db_result = ConnectionPool::instance().execute(
        [&](pqxx::work &txn) { return HousingQueries::remove_item(txn, placed_item_db_id); });
    if (!db_result)
        return std::unexpected(db_result.error());

    // Remove from room
    return room->remove_placed_item(placed_item_db_id);
}

// ============================================================================
// Guest management
// ============================================================================

Result<void> HousingManager::add_guest(const std::string &owner_character_id, const std::string &guest_character_id,
                                       bool can_place) {
    return ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
        auto house = HousingQueries::load_house(txn, owner_character_id);
        if (!house || !house->has_value())
            return std::unexpected(Errors::NotFound("house"));

        auto result = HousingQueries::add_guest(txn, house->value().id, guest_character_id, can_place);
        if (!result)
            return std::unexpected(result.error());

        // Update cache
        {
            std::unique_lock lock(mutex_);
            auto it = active_houses_.find(owner_character_id);
            if (it != active_houses_.end()) {
                it->second.guests_cache.insert(guest_character_id);
            }
        }

        return {};
    });
}

Result<void> HousingManager::remove_guest(const std::string &owner_character_id,
                                          const std::string &guest_character_id) {
    return ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
        auto house = HousingQueries::load_house(txn, owner_character_id);
        if (!house || !house->has_value())
            return std::unexpected(Errors::NotFound("house"));

        auto result = HousingQueries::remove_guest(txn, house->value().id, guest_character_id);
        if (!result)
            return std::unexpected(result.error());

        // Update cache
        {
            std::unique_lock lock(mutex_);
            auto it = active_houses_.find(owner_character_id);
            if (it != active_houses_.end()) {
                it->second.guests_cache.erase(guest_character_id);
            }
        }

        return {};
    });
}

bool HousingManager::is_guest(const std::string &owner_character_id, const std::string &guest_character_id) {
    std::shared_lock lock(mutex_);
    auto it = active_houses_.find(owner_character_id);
    if (it != active_houses_.end()) {
        return it->second.guests_cache.contains(guest_character_id);
    }
    return false;
}

Result<bool> HousingManager::check_guest_access(const std::string &owner_id, const std::string &guest_id) {
    // Check in-memory cache first (if house is loaded)
    {
        std::shared_lock lock(mutex_);
        auto it = active_houses_.find(owner_id);
        if (it != active_houses_.end()) {
            return it->second.guests_cache.contains(guest_id);
        }
    }

    // Fall back to database query
    return ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<bool> {
        auto house = HousingQueries::load_house(txn, owner_id);
        if (!house || !house->has_value())
            return std::unexpected(Errors::NotFound("house"));

        return HousingQueries::is_guest_of(txn, house->value().id, guest_id);
    });
}

Result<std::vector<HousingQueries::HouseGuestData>> HousingManager::get_guests(const std::string &owner_character_id) {
    return ConnectionPool::instance().execute([&](pqxx::work &txn) {
        auto house = HousingQueries::load_house(txn, owner_character_id);
        if (!house || !house->has_value())
            return Result<std::vector<HousingQueries::HouseGuestData>>(std::unexpected(Errors::NotFound("house")));

        return HousingQueries::load_house_guests(txn, house->value().id);
    });
}

// ============================================================================
// Queries
// ============================================================================

std::shared_ptr<HousingRoom> HousingManager::get_housing_room(EntityId room_id) const {
    if (!is_housing_room(room_id))
        return nullptr;

    std::shared_lock lock(mutex_);
    auto owner_it = room_to_owner_.find(room_id);
    if (owner_it == room_to_owner_.end())
        return nullptr;

    auto house_it = active_houses_.find(owner_it->second);
    if (house_it == active_houses_.end())
        return nullptr;

    int db_id = static_cast<int>(room_id.local_id());
    auto room_it = house_it->second.rooms.find(db_id);
    if (room_it == house_it->second.rooms.end())
        return nullptr;

    return room_it->second;
}

std::string HousingManager::get_room_owner(EntityId room_id) const {
    std::shared_lock lock(mutex_);
    auto it = room_to_owner_.find(room_id);
    return it != room_to_owner_.end() ? it->second : "";
}

Result<HousingQueries::HouseData> HousingManager::get_house_info(const std::string &character_id) {
    return ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<HousingQueries::HouseData> {
        auto house = HousingQueries::load_house(txn, character_id);
        if (!house)
            return std::unexpected(house.error());
        if (!house->has_value())
            return std::unexpected(Errors::NotFound("house"));
        return house->value();
    });
}

bool HousingManager::is_owner_of_room(const std::string &character_id, EntityId room_id) const {
    auto owner = get_room_owner(room_id);
    return !owner.empty() && owner == character_id;
}

// ============================================================================
// Periodic maintenance
// ============================================================================

void HousingManager::save_dirty_houses() {
    // Currently a no-op — all mutations are written to DB immediately.
    // This hook exists for future batched writes if needed.
}

void HousingManager::check_unload_timers() {
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> to_unload;

    {
        std::shared_lock lock(mutex_);
        for (const auto &[owner, house] : active_houses_) {
            if (house.occupant_count <= 0 && (now - house.last_occupant_left) >= UNLOAD_TIMEOUT) {
                to_unload.push_back(owner);
            }
        }
    }

    for (const auto &owner : to_unload) {
        Log::debug("Unloading empty house for {}", owner);
        unload_house(owner);
    }
}

// ============================================================================
// Internal helpers
// ============================================================================

Result<void> HousingManager::load_house(const std::string &character_id) {
    return ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
        // Load house data
        auto house_opt = HousingQueries::load_house(txn, character_id);
        if (!house_opt)
            return std::unexpected(house_opt.error());
        if (!house_opt->has_value())
            return std::unexpected(Errors::NotFound("house"));

        auto &house_data = house_opt->value();

        // Load rooms
        auto rooms_data = HousingQueries::load_house_rooms(txn, house_data.id);
        if (!rooms_data)
            return std::unexpected(rooms_data.error());

        // Load exits
        auto exits_data = HousingQueries::load_house_exits(txn, house_data.id);
        if (!exits_data)
            return std::unexpected(exits_data.error());

        // Load guests
        auto guests_data = HousingQueries::load_house_guests(txn, house_data.id);

        // Create ActiveHouse
        ActiveHouse active;
        active.house_id = house_data.id;
        active.character_id = character_id;
        active.data = house_data;

        // Create HousingRoom instances
        for (const auto &room_data : *rooms_data) {
            auto sector = parse_sector(room_data.sector);
            auto room = HousingRoom::create(room_data.id, house_data.id, room_data.local_index, character_id,
                                            room_data.name, room_data.description, sector);
            if (!room) {
                Log::warn("Failed to create housing room {} for {}: {}", room_data.id, character_id,
                          room.error().message);
                continue;
            }

            (*room)->set_peaceful(room_data.is_peaceful);
            (*room)->set_base_light_level(room_data.base_light_level);
            (*room)->set_capacity(room_data.capacity);

            // Load placed items for this room
            auto items_data = HousingQueries::load_room_items(txn, room_data.id);
            if (items_data) {
                for (const auto &item_data : *items_data) {
                    EntityId obj_id(static_cast<uint32_t>(item_data.object_zone_id),
                                    static_cast<uint32_t>(item_data.object_id));
                    auto obj = WorldManager::instance().create_object_instance(obj_id);
                    if (obj) {
                        (*room)->add_placed_item(obj, item_data.id);
                    }
                }
            }

            active.rooms[room_data.id] = *room;
        }

        // Set up exits between rooms
        for (const auto &exit_data : *exits_data) {
            auto from_it = active.rooms.find(exit_data.from_room_id);
            auto to_it = active.rooms.find(exit_data.to_room_id);
            if (from_it != active.rooms.end() && to_it != active.rooms.end()) {
                auto dir = RoomUtils::parse_direction(exit_data.direction);
                if (dir) {
                    ExitInfo exit;
                    exit.to_room = to_it->second->id();
                    from_it->second->set_exit(*dir, exit);
                }
            }
        }

        // Set up foyer exit to world entrance
        for (auto &[db_id, room] : active.rooms) {
            if (room->is_foyer()) {
                ExitInfo world_exit;
                world_exit.to_room = EntityId(static_cast<uint32_t>(house_data.entrance_room_zone_id),
                                              static_cast<uint32_t>(house_data.entrance_room_id));
                world_exit.description = "The way back outside.";
                world_exit.keyword = "outside";
                room->set_exit(Direction::Out, world_exit);
            }
        }

        // Cache guests
        if (guests_data) {
            for (const auto &guest : *guests_data) {
                active.guests_cache.insert(guest.character_id);
            }
        }

        // Register all rooms with WorldManager
        {
            std::unique_lock lock(mutex_);
            for (const auto &[db_id, room] : active.rooms) {
                register_room(room, character_id);
            }
            active_houses_[character_id] = std::move(active);
        }

        Log::info("Loaded house for {} ({} rooms)", character_id, rooms_data->size());
        return {};
    });
}

void HousingManager::unload_house(const std::string &character_id) {
    std::unique_lock lock(mutex_);
    auto it = active_houses_.find(character_id);
    if (it == active_houses_.end())
        return;

    // Unregister all rooms from WorldManager
    for (const auto &[db_id, room] : it->second.rooms) {
        unregister_room(room->id());
    }

    active_houses_.erase(it);
}

void HousingManager::register_room(std::shared_ptr<HousingRoom> room, const std::string &owner_id) {
    room_to_owner_[room->id()] = owner_id;
    WorldManager::instance().add_room(room);
}

void HousingManager::unregister_room(EntityId room_id) {
    room_to_owner_.erase(room_id);
    WorldManager::instance().remove_room(room_id);
}

Result<void> HousingManager::evacuate_stranded_players() {
    return ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
        auto stranded = HousingQueries::load_stranded_houses(txn);
        if (!stranded)
            return std::unexpected(stranded.error());

        for (const auto &house : *stranded) {
            int dest_zone = house.return_room_zone_id.value_or(house.entrance_room_zone_id);
            int dest_room = house.return_room_id.value_or(house.entrance_room_id);

            Log::info("Evacuating stranded player {} to room {}:{}", house.character_id, dest_zone, dest_room);

            // Update the character's saved location
            auto update = HousingQueries::update_character_location(txn, house.character_id, dest_zone, dest_room);
            if (!update) {
                Log::warn("Failed to update location for {}: {}", house.character_id, update.error().message);
            }

            // Clear the return location
            auto clear = HousingQueries::clear_return_location(txn, house.id);
            if (!clear) {
                Log::warn("Failed to clear return location for house {}: {}", house.id, clear.error().message);
            }
        }

        if (!stranded->empty()) {
            Log::info("Evacuated {} stranded player(s) from housing", stranded->size());
        }
        return {};
    });
}

Result<HousingManager::ActiveHouse *> HousingManager::get_or_load_house(const std::string &character_id) {
    // Check if already loaded
    {
        std::shared_lock lock(mutex_);
        auto it = active_houses_.find(character_id);
        if (it != active_houses_.end())
            return &it->second;
    }

    // Load from database
    auto result = load_house(character_id);
    if (!result)
        return std::unexpected(result.error());

    std::shared_lock lock(mutex_);
    auto it = active_houses_.find(character_id);
    if (it != active_houses_.end())
        return &it->second;

    return std::unexpected(Errors::InvalidState("house loaded but not found in cache"));
}

SectorType HousingManager::parse_sector(const std::string &sector_str) {
    // Match the sector strings from the database enum
    if (sector_str == "STRUCTURE")
        return SectorType::Inside;
    if (sector_str == "CITY")
        return SectorType::City;
    if (sector_str == "FIELD")
        return SectorType::Field;
    if (sector_str == "FOREST")
        return SectorType::Forest;
    return SectorType::Inside; // Default for housing
}

std::string HousingManager::direction_to_db_string(Direction dir) {
    // Convert Direction enum to the uppercase format used in the Prisma Direction enum
    auto name = std::string(magic_enum::enum_name(dir));
    std::ranges::transform(name, name.begin(), ::toupper);
    return name;
}
