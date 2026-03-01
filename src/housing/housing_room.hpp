#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "core/ids.hpp"
#include "core/result.hpp"
#include "world/room.hpp"

class Object;

/**
 * A player-owned housing room.
 *
 * Extends Room so it integrates seamlessly with the existing movement,
 * look, and object interaction commands via WorldManager::get_room().
 *
 * All housing rooms use a reserved zone ID (9999) with the database
 * auto-increment row ID as the local_id, avoiding collisions with
 * world rooms.
 */
class HousingRoom : public Room {
  public:
    /** Reserved zone ID for all housing rooms */
    static constexpr std::uint32_t HOUSING_ZONE_ID = 9999;

    /** Create a HousingRoom from database data */
    static Result<std::shared_ptr<HousingRoom>> create(int db_id, int house_id, int local_index,
                                                       std::string_view owner_character_id, std::string_view name,
                                                       std::string_view description, SectorType sector);

    /** Check if an EntityId belongs to the housing zone */
    static bool is_housing_room(EntityId id) { return id.is_valid() && id.zone_id() == HOUSING_ZONE_ID; }

    /** Get the EntityId for a housing room database ID */
    static EntityId make_entity_id(int db_id) { return EntityId(HOUSING_ZONE_ID, static_cast<std::uint32_t>(db_id)); }

    // Accessors
    int db_id() const { return db_id_; }
    int house_id() const { return house_id_; }
    int local_index() const { return local_index_; }
    bool is_foyer() const { return local_index_ == 0; }
    const std::string &owner_character_id() const { return owner_character_id_; }

    // Custom description (player-written)
    void set_custom_description(std::string_view desc) { custom_description_ = std::string(desc); }
    void set_custom_name(std::string_view name);

    // Placed items (decorative furnishings)
    const std::vector<std::shared_ptr<Object>> &placed_items() const { return placed_items_; }
    void add_placed_item(std::shared_ptr<Object> item, int placed_item_db_id);
    Result<std::shared_ptr<Object>> remove_placed_item(int placed_item_db_id);
    int placed_item_count() const { return static_cast<int>(placed_items_.size()); }

    /** Maximum placed items per room */
    static constexpr int MAX_ITEMS_PER_ROOM = 20;

    /** Get the database ID associated with a placed item object */
    int get_placed_item_db_id(const Object *item) const;

    // Override room description to show player content
    std::string get_room_description(const Actor *observer = nullptr) const override;

  private:
    /** Private constructor — use create() factory */
    HousingRoom(EntityId id, std::string_view name, SectorType sector);

    int db_id_;
    int house_id_;
    int local_index_;
    std::string owner_character_id_;
    std::string custom_description_;
    std::vector<std::shared_ptr<Object>> placed_items_;

    // Map placed objects to their database IDs (for removal)
    std::unordered_map<const Object *, int> placed_item_db_ids_;
};
