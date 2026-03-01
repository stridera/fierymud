#include "housing/housing_room.hpp"

#include <algorithm>

#include <fmt/format.h>

#include "core/object.hpp"

// ============================================================================
// Factory
// ============================================================================

Result<std::shared_ptr<HousingRoom>> HousingRoom::create(int db_id, int house_id, int local_index,
                                                         std::string_view owner_character_id, std::string_view name,
                                                         std::string_view description, SectorType sector) {
    EntityId entity_id = make_entity_id(db_id);

    // Use shared_ptr with private constructor via friend-like trick
    auto room = std::shared_ptr<HousingRoom>(new HousingRoom(entity_id, name, sector));
    room->db_id_ = db_id;
    room->house_id_ = house_id;
    room->local_index_ = local_index;
    room->owner_character_id_ = std::string(owner_character_id);
    room->custom_description_ = std::string(description);

    // Housing rooms are always peaceful, well-lit, and indoors
    room->set_peaceful(true);
    room->set_base_light_level(5);

    return room;
}

HousingRoom::HousingRoom(EntityId id, std::string_view name, SectorType sector) : Room(id, name, sector) {}

// ============================================================================
// Description
// ============================================================================

void HousingRoom::set_custom_name(std::string_view name) { set_name(name); }

std::string HousingRoom::get_room_description(const Actor * /*observer*/) const {
    std::string desc = custom_description_;

    // Append placed items as visible furnishings
    if (!placed_items_.empty()) {
        desc += "\n";
        for (const auto &item : placed_items_) {
            if (item) {
                // Use the object's ground description (what it looks like on the ground)
                auto ground_desc = item->ground();
                if (!ground_desc.empty()) {
                    desc += fmt::format("\n   {}", ground_desc);
                } else {
                    desc += fmt::format("\n   {} is here.", item->short_desc());
                }
            }
        }
    }

    return desc;
}

// ============================================================================
// Placed items
// ============================================================================

void HousingRoom::add_placed_item(std::shared_ptr<Object> item, int placed_item_db_id) {
    placed_item_db_ids_[item.get()] = placed_item_db_id;
    placed_items_.push_back(std::move(item));
}

Result<std::shared_ptr<Object>> HousingRoom::remove_placed_item(int placed_item_db_id) {
    // Find the item with this database ID
    for (auto it = placed_items_.begin(); it != placed_items_.end(); ++it) {
        auto db_id_it = placed_item_db_ids_.find(it->get());
        if (db_id_it != placed_item_db_ids_.end() && db_id_it->second == placed_item_db_id) {
            auto item = std::move(*it);
            placed_item_db_ids_.erase(db_id_it);
            placed_items_.erase(it);
            return item;
        }
    }
    return std::unexpected(Errors::NotFound("placed item"));
}

int HousingRoom::get_placed_item_db_id(const Object *item) const {
    auto it = placed_item_db_ids_.find(item);
    if (it != placed_item_db_ids_.end()) {
        return it->second;
    }
    return -1;
}
