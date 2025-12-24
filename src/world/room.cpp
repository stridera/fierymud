#include "room.hpp"

#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../core/object.hpp"
#include "../text/string_utils.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_map>

// ExitInfo Implementation

std::string ExitInfo::door_state_description() const {
    if (!has_door) {
        return "";
    }

    std::string result;
    if (is_hidden) {
        result += "hidden ";
    }
    if (is_closed) {
        result += "closed ";
        if (is_locked) {
            result += "locked ";
        }
    } else {
        result += "open ";
    }

    if (is_pickproof) {
        result += "pickproof ";
    }

    return result + "door";
}

nlohmann::json ExitInfo::to_json() const {
    nlohmann::json json;

    json["to_room"] = to_room.value();
    if (!description.empty()) {
        json["description"] = description;
    }
    if (!keyword.empty()) {
        json["keyword"] = keyword;
    }

    if (has_door) {
        json["has_door"] = true;
        json["is_closed"] = is_closed;
        json["is_locked"] = is_locked;
        json["is_hidden"] = is_hidden;
        json["is_pickproof"] = is_pickproof;

        if (key_id.is_valid()) {
            json["key_id"] = key_id.value();
        }
        if (difficulty > 0) {
            json["difficulty"] = difficulty;
        }
    }

    return json;
}

Result<ExitInfo> ExitInfo::from_json(const nlohmann::json &json) {
    try {
        ExitInfo exit;

        // Handle destination (JSON) -> to_room (internal)
        if (json.contains("destination")) {
            if (json["destination"].is_string()) {
                std::string dest_str = json["destination"].get<std::string>();
                // Convert "-1" string to invalid room (no destination)
                if (dest_str != "-1") {
                    exit.to_room = EntityId{std::stoull(dest_str)};
                }
                // else: leave to_room as INVALID_ENTITY_ID
            } else if (json["destination"].is_number()) {
                int64_t dest_num = json["destination"].get<int64_t>();
                if (dest_num != -1) {
                    exit.to_room = EntityId{static_cast<std::uint64_t>(dest_num)};
                }
                // else: leave to_room as INVALID_ENTITY_ID
            }
        } else if (json.contains("to_room")) {
            exit.to_room = EntityId{json["to_room"].get<std::uint64_t>()};
        }

        if (json.contains("description")) {
            exit.description = json["description"].get<std::string>();
        }

        if (json.contains("keyword")) {
            exit.keyword = json["keyword"].get<std::string>();
        }

        // Handle key field - convert "-1" string to no key
        if (json.contains("key")) {
            if (json["key"].is_string()) {
                std::string key_str = json["key"].get<std::string>();
                if (key_str != "-1") {
                    exit.key_id = EntityId{std::stoull(key_str)};
                }
            } else if (json["key"].is_number()) {
                int64_t key_num = json["key"].get<int64_t>();
                if (key_num != -1) {
                    exit.key_id = EntityId{static_cast<std::uint64_t>(key_num)};
                }
            }
        } else if (json.contains("key_id")) {
            exit.key_id = EntityId{json["key_id"].get<std::uint64_t>()};
        }

        if (json.contains("has_door")) {
            exit.has_door = json["has_door"].get<bool>();
        }

        if (json.contains("is_closed")) {
            exit.is_closed = json["is_closed"].get<bool>();
        }

        if (json.contains("is_locked")) {
            exit.is_locked = json["is_locked"].get<bool>();
        }

        if (json.contains("is_hidden")) {
            exit.is_hidden = json["is_hidden"].get<bool>();
        }

        if (json.contains("is_pickproof")) {
            exit.is_pickproof = json["is_pickproof"].get<bool>();
        }

        if (json.contains("difficulty")) {
            exit.difficulty = json["difficulty"].get<int>();
        }

        // Parse door object with state field
        if (json.contains("door") && json["door"].is_object()) {
            const auto &door_obj = json["door"];
            exit.has_door = true; // If door object exists, this exit has a door

            // Parse door state - support both string and array formats
            if (door_obj.contains("state")) {
                const auto &state = door_obj["state"];

                if (state.is_string()) {
                    // String format: "CLOSED", "LOCKED", etc.
                    std::string state_str = state.get<std::string>();
                    if (state_str == "CLOSED") {
                        exit.is_closed = true;
                    } else if (state_str == "LOCKED") {
                        exit.is_locked = true;
                        exit.is_closed = true; // Locked doors are also closed
                    }
                } else if (state.is_array()) {
                    // Array format: ["CLOSED"], ["LOCKED", "CLOSED"], etc.
                    for (const auto &state_item : state) {
                        if (state_item.is_string()) {
                            std::string state_str = state_item.get<std::string>();
                            if (state_str == "CLOSED") {
                                exit.is_closed = true;
                            } else if (state_str == "LOCKED") {
                                exit.is_locked = true;
                                exit.is_closed = true; // Locked doors are also closed
                            }
                        }
                    }
                }
            }

            // Also support direct door properties
            if (door_obj.contains("is_closed")) {
                exit.is_closed = door_obj["is_closed"].get<bool>();
            }
            if (door_obj.contains("is_locked")) {
                exit.is_locked = door_obj["is_locked"].get<bool>();
            }
            if (door_obj.contains("is_hidden")) {
                exit.is_hidden = door_obj["is_hidden"].get<bool>();
            }
            if (door_obj.contains("is_pickproof")) {
                exit.is_pickproof = door_obj["is_pickproof"].get<bool>();
            }
        }

        return exit;

    } catch (const nlohmann::json::exception &e) {
        return std::unexpected(Errors::ParseError("ExitInfo JSON parsing", e.what()));
    }
}

// RoomContents Implementation

void RoomContents::add_object(std::shared_ptr<Object> obj) {
    if (obj) {
        objects.push_back(std::move(obj));
    }
}

bool RoomContents::remove_object(EntityId obj_id) {
    auto it =
        std::find_if(objects.begin(), objects.end(), [obj_id](const auto &obj) { return obj && obj->id() == obj_id; });

    if (it != objects.end()) {
        objects.erase(it);
        return true;
    }
    return false;
}

void RoomContents::add_actor(std::shared_ptr<Actor> actor) {
    if (actor) {
        actors.push_back(std::move(actor));
    }
}

bool RoomContents::remove_actor(EntityId actor_id) {
    auto it = std::find_if(actors.begin(), actors.end(),
                           [actor_id](const auto &actor) { return actor && actor->id() == actor_id; });

    if (it != actors.end()) {
        actors.erase(it);
        return true;
    }
    return false;
}

std::shared_ptr<Object> RoomContents::find_object(EntityId id) const {
    auto it = std::find_if(objects.begin(), objects.end(), [id](const auto &obj) { return obj && obj->id() == id; });

    return it != objects.end() ? *it : nullptr;
}

std::shared_ptr<Actor> RoomContents::find_actor(EntityId id) const {
    auto it =
        std::find_if(actors.begin(), actors.end(), [id](const auto &actor) { return actor && actor->id() == id; });

    return it != actors.end() ? *it : nullptr;
}

std::vector<std::shared_ptr<Object>> RoomContents::find_objects_by_keyword(std::string_view keyword) const {
    std::vector<std::shared_ptr<Object>> results;

    for (const auto &obj : objects) {
        if (obj && obj->matches_keyword(keyword)) {
            results.push_back(obj);
        }
    }

    return results;
}

std::vector<std::shared_ptr<Actor>> RoomContents::find_actors_by_keyword(std::string_view keyword) const {
    std::vector<std::shared_ptr<Actor>> results;

    for (const auto &actor : actors) {
        if (actor && actor->matches_keyword(keyword)) {
            results.push_back(actor);
        }
    }

    return results;
}

void RoomContents::clear() {
    objects.clear();
    actors.clear();
}

// Room Implementation

Room::Room(EntityId id, std::string_view name, SectorType sector)
    : Entity(id, name), sector_type_(sector), light_level_(0), zone_id_(INVALID_ENTITY_ID) {}

Result<std::unique_ptr<Room>> Room::create(EntityId id, std::string_view name, SectorType sector) {
    if (!id.is_valid()) {
        return std::unexpected(Errors::InvalidArgument("id", "must be valid"));
    }

    if (name.empty()) {
        return std::unexpected(Errors::InvalidArgument("name", "cannot be empty"));
    }

    if (sector == SectorType::Undefined) {
        return std::unexpected(Errors::InvalidArgument("sector", "cannot be undefined"));
    }

    auto room = std::unique_ptr<Room>(new Room(id, name, sector));

    // Set natural light level based on sector
    room->light_level_ = RoomUtils::get_sector_light_level(sector);

    TRY(room->validate());

    return room;
}

Result<std::unique_ptr<Room>> Room::from_json(const nlohmann::json &json) {
    try {
        // First create base entity
        auto base_result = Entity::from_json(json);
        if (!base_result) {
            return std::unexpected(base_result.error());
        }

        auto base_entity = std::move(base_result.value());

        // Parse room-specific fields with field mapping
        SectorType sector = SectorType::Inside;
        if (json.contains("sector_type")) {
            if (auto parsed_sector = RoomUtils::parse_sector_type(json["sector_type"].get<std::string>())) {
                sector = parsed_sector.value();
            }
        } else if (json.contains("sector")) {
            // Handle numeric or string sector
            if (json["sector"].is_string()) {
                std::string sector_str = json["sector"].get<std::string>();
                if (auto parsed_sector = RoomUtils::parse_sector_type(sector_str)) {
                    sector = parsed_sector.value();
                } else {
                    // Try converting numeric string to SectorType
                    try {
                        int sector_num = std::stoi(sector_str);
                        sector = RoomUtils::sector_from_number(sector_num);
                    } catch (...) {
                        // Use default sector
                    }
                }
            } else {
                int sector_num = json["sector"].get<int>();
                sector = RoomUtils::sector_from_number(sector_num);
            }
        }

        auto room = std::unique_ptr<Room>(new Room(base_entity->id(), base_entity->name(), sector));

        // Copy base entity properties
        room->set_keywords(base_entity->keywords());
        room->set_description(base_entity->description());
        room->set_short_description(base_entity->short_description());

        // Parse room-specific properties
        if (json.contains("light_level")) {
            room->set_light_level(json["light_level"].get<int>());
        }

        if (json.contains("zone_id")) {
            room->set_zone_id(EntityId{json["zone_id"].get<std::uint64_t>()});
        }

        // Parse flags from comma-separated string
        if (json.contains("flags")) {
            if (json["flags"].is_string()) {
                std::string flags_str = json["flags"].get<std::string>();
                // Split comma-separated flags
                std::istringstream iss(flags_str);
                std::string flag;
                while (std::getline(iss, flag, ',')) {
                    flag = std::string(trim(flag));

                    if (!flag.empty()) {
                        if (auto parsed_flag = RoomUtils::parse_room_flag(flag)) {
                            room->set_flag(parsed_flag.value());
                        }
                    }
                }
            } else if (json["flags"].is_array()) {
                // Also handle array format for compatibility
                for (const auto &flag_name : json["flags"]) {
                    if (flag_name.is_string()) {
                        std::string flag_str = flag_name.get<std::string>();
                        if (auto flag = RoomUtils::parse_room_flag(flag_str)) {
                            room->set_flag(flag.value());
                        }
                    }
                }
            }
        }

        // Parse exits
        if (json.contains("exits") && json["exits"].is_object()) {
            for (const auto &[dir_name, exit_json] : json["exits"].items()) {
                if (auto dir = RoomUtils::parse_direction(dir_name)) {
                    auto exit_result = ExitInfo::from_json(exit_json);
                    if (exit_result) {
                        room->set_exit(dir.value(), exit_result.value());
                    }
                }
            }
        }

        TRY(room->validate());

        return room;

    } catch (const nlohmann::json::exception &e) {
        return std::unexpected(Errors::ParseError("Room JSON parsing", e.what()));
    }
}

bool Room::has_flag(RoomFlag flag) const { return flags_.contains(flag); }

void Room::set_flag(RoomFlag flag, bool value) {
    if (value) {
        flags_.insert(flag);
    } else {
        flags_.erase(flag);
    }
}

bool Room::has_exit(Direction dir) const { return exits_.contains(dir); }

const ExitInfo *Room::get_exit(Direction dir) const {
    auto it = exits_.find(dir);
    return it != exits_.end() ? &it->second : nullptr;
}

ExitInfo *Room::get_exit_mutable(Direction dir) {
    auto it = exits_.find(dir);
    return it != exits_.end() ? &it->second : nullptr;
}

Result<void> Room::set_exit(Direction dir, const ExitInfo &exit) {
    if (dir == Direction::None) {
        return std::unexpected(Errors::InvalidArgument("direction", "cannot be None"));
    }

    if (!exit.to_room.is_valid()) {
        return std::unexpected(Errors::InvalidArgument("exit.to_room", "must be valid"));
    }

    exits_[dir] = exit;
    return Success();
}

void Room::remove_exit(Direction dir) { exits_.erase(dir); }

std::vector<Direction> Room::get_available_exits() const {
    std::vector<Direction> directions;
    for (const auto &[dir, exit] : exits_) {
        if (exit.to_room.is_valid()) {
            directions.push_back(dir);
        }
    }
    return directions;
}

std::vector<Direction> Room::get_visible_exits(const Actor * /* observer */) const {
    std::vector<Direction> directions;
    for (const auto &[dir, exit] : exits_) {
        if (exit.to_room.is_valid() && !exit.is_hidden) {
            directions.push_back(dir);
        }
    }
    return directions;
}

void Room::add_object(std::shared_ptr<Object> obj) { contents_.add_object(std::move(obj)); }

bool Room::remove_object(EntityId obj_id) { return contents_.remove_object(obj_id); }

void Room::add_actor(std::shared_ptr<Actor> actor) { contents_.add_actor(std::move(actor)); }

bool Room::remove_actor(EntityId actor_id) { return contents_.remove_actor(actor_id); }

std::shared_ptr<Object> Room::find_object(EntityId id) const { return contents_.find_object(id); }

std::shared_ptr<Actor> Room::find_actor(EntityId id) const { return contents_.find_actor(id); }

std::vector<std::shared_ptr<Object>> Room::find_objects_by_keyword(std::string_view keyword) const {
    return contents_.find_objects_by_keyword(keyword);
}

std::vector<std::shared_ptr<Actor>> Room::find_actors_by_keyword(std::string_view keyword) const {
    return contents_.find_actors_by_keyword(keyword);
}

bool Room::is_dark() const { return has_flag(RoomFlag::Dark) || calculate_effective_light() <= 0; }

bool Room::is_naturally_lit() const { return sector_provides_light() && !has_flag(RoomFlag::Dark); }

int Room::calculate_effective_light() const {
    // AlwaysLit rooms are always lit regardless of other factors
    if (has_flag(RoomFlag::AlwaysLit)) {
        return 10; // High light level
    }

    if (has_flag(RoomFlag::Dark)) {
        return 0;
    }

    int total_light = light_level_;

    // Add natural sector light
    if (sector_provides_light()) {
        total_light += RoomUtils::get_sector_light_level(sector_type_);
    }

    // Add light from objects in room
    for (const auto &obj : contents_.objects) {
        if (obj && obj->is_light_source() && obj->light_info().lit) {
            total_light += obj->light_info().brightness;
        }
    }

    // Add light from objects carried by actors in room
    for (const auto &actor : contents_.actors) {
        if (actor) {
            auto inventory_items = actor->inventory().get_all_items();
            for (const auto &obj : inventory_items) {
                if (obj && obj->is_light_source() && obj->light_info().lit) {
                    total_light += obj->light_info().brightness;
                }
            }
        }
    }

    return std::max(0, total_light);
}

bool Room::can_see_in_room(const Actor *observer) const {
    if (!observer) {
        return false;
    }

    // Check if actor is blind (ActorFlag::Blind in actor.hpp)
    if (observer->has_flag(ActorFlag::Blind)) {
        return false;
    }

    // Get effective light level
    int light = calculate_effective_light();

    // Dark room - check for special vision abilities
    if (light <= 0) {
        // Infravision allows seeing in dark rooms (but not magical darkness)
        if (observer->has_flag(ActorFlag::Infravision)) {
            return true;
        }
        // Check if actor is an immortal/god
        auto* player = dynamic_cast<const Player*>(observer);
        if (player && player->is_god()) {
            return true;
        }
        return false;
    }

    return true;
}

int Room::max_occupants() const {
    if (has_flag(RoomFlag::Tunnel)) {
        return 1;
    }
    if (has_flag(RoomFlag::Private)) {
        return 2;
    }
    if (has_flag(RoomFlag::Atrium)) {
        return 10;
    }
    return 100; // Default max
}

bool Room::can_accommodate(const Actor *actor) const {
    if (!actor) {
        return false;
    }

    // Check if actor is a Mobile (NPC)
    bool is_mobile = (dynamic_cast<const Mobile*>(actor) != nullptr);

    // Check capacity
    if (static_cast<int>(contents_.actors.size()) >= max_occupants()) {
        return false;
    }

    // Check god room restriction - only immortals may enter
    // For now, mobiles cannot enter god rooms
    if (has_flag(RoomFlag::Godroom)) {
        if (is_mobile) {
            return false;  // NPCs cannot enter god rooms
        }
        // Players need god level check - handled in movement restrictions
    }

    // Check no-mob restriction - NPCs cannot enter
    if (has_flag(RoomFlag::NoMob) && is_mobile) {
        return false;  // NPCs blocked from NoMob rooms
    }

    return true;
}

bool Room::is_full() const { return static_cast<int>(contents_.actors.size()) >= max_occupants(); }

std::string Room::get_room_description(const Actor *observer) const {
    std::string desc{description()};

    if (!can_see_in_room(observer)) {
        return "It is too dark to see anything.";
    }

    // Add sector-specific atmospheric details
    switch (sector_type_) {
    case SectorType::Forest:
        if (desc.find("trees") == std::string::npos) {
            desc += " Tall trees surround you.";
        }
        break;
    case SectorType::Desert:
        if (desc.find("sand") == std::string::npos) {
            desc += " Hot sand stretches in all directions.";
        }
        break;
    case SectorType::Water_Swim:
    case SectorType::Water_Noswim:
        if (desc.find("water") == std::string::npos) {
            desc += " You are in the water.";
        }
        break;
    default:
        break;
    }

    return desc;
}

std::string Room::get_exits_description(const Actor *observer) const {
    auto visible_exits = get_visible_exits(observer);

    if (visible_exits.empty()) {
        return "There are no obvious exits.";
    }

    std::string exits_str = "Exits: ";
    for (size_t i = 0; i < visible_exits.size(); ++i) {
        if (i > 0) {
            exits_str += ", ";
        }
        exits_str += RoomUtils::get_direction_name(visible_exits[i]);

        // Add door state info
        if (auto exit = get_exit(visible_exits[i])) {
            if (exit->has_door) {
                exits_str += fmt::format(" ({})", exit->door_state_description());
            }
        }
    }

    return exits_str;
}

std::string Room::get_contents_description(const Actor *observer) const {
    if (!can_see_in_room(observer)) {
        return "";
    }

    std::string contents_desc;

    // Describe objects
    for (const auto &obj : contents_.objects) {
        if (obj) {
            contents_desc += fmt::format("{} is here.\n", obj->display_name_with_condition());
        }
    }

    // Describe other actors
    for (const auto &actor : contents_.actors) {
        if (actor && actor.get() != observer) {
            contents_desc += fmt::format("{} is here.\n", actor->display_name());
        }
    }

    return contents_desc;
}

nlohmann::json Room::to_json() const {
    nlohmann::json json = Entity::to_json();

    json["sector_type"] = std::string(magic_enum::enum_name(sector_type_));
    json["light_level"] = light_level_;

    if (zone_id_.is_valid()) {
        json["zone_id"] = zone_id_.value();
    }

    // Serialize flags
    std::vector<std::string> flag_names;
    for (RoomFlag flag : flags_) {
        flag_names.emplace_back(magic_enum::enum_name(flag));
    }
    if (!flag_names.empty()) {
        json["flags"] = flag_names;
    }

    // Serialize exits
    if (!exits_.empty()) {
        nlohmann::json exits_json;
        for (const auto &[dir, exit] : exits_) {
            exits_json[std::string(magic_enum::enum_name(dir))] = exit.to_json();
        }
        json["exits"] = exits_json;
    }

    return json;
}

Result<void> Room::validate() const {
    TRY(Entity::validate());

    if (sector_type_ == SectorType::Undefined) {
        return std::unexpected(Errors::InvalidState("Room sector type cannot be undefined"));
    }

    if (light_level_ < 0) {
        return std::unexpected(Errors::InvalidState("Room light level cannot be negative"));
    }

    // Validate exits
    for (const auto &[dir, exit] : exits_) {
        if (dir == Direction::None) {
            return std::unexpected(Errors::InvalidState("Room cannot have None direction exit"));
        }

        if (!exit.to_room.is_valid()) {
            return std::unexpected(Errors::InvalidState("Exit must have valid destination room"));
        }
    }

    return Success();
}

bool Room::sector_provides_light() const { return RoomUtils::get_sector_light_level(sector_type_) > 0; }

// RoomUtils Implementation

namespace RoomUtils {
std::string_view get_direction_name(Direction dir) {
    auto name = magic_enum::enum_name(dir);
    return name.empty() ? "none" : name;
}

std::optional<Direction> parse_direction(std::string_view dir_name) {
    // Try full name first (PascalCase - matches enum names)
    auto result = magic_enum::enum_cast<Direction>(dir_name);
    if (result) {
        return result;
    }

    // Try lowercase, uppercase, and abbreviations (for database and user input)
    static const std::unordered_map<std::string_view, Direction> dir_map = {
        // Lowercase
        {"n", Direction::North},      {"north", Direction::North},
        {"e", Direction::East},       {"east", Direction::East},
        {"s", Direction::South},      {"south", Direction::South},
        {"w", Direction::West},       {"west", Direction::West},
        {"u", Direction::Up},         {"up", Direction::Up},
        {"d", Direction::Down},       {"down", Direction::Down},
        {"ne", Direction::Northeast}, {"northeast", Direction::Northeast},
        {"nw", Direction::Northwest}, {"northwest", Direction::Northwest},
        {"se", Direction::Southeast}, {"southeast", Direction::Southeast},
        {"sw", Direction::Southwest}, {"southwest", Direction::Southwest},
        // Uppercase (for database enum values)
        {"NORTH", Direction::North},
        {"EAST", Direction::East},
        {"SOUTH", Direction::South},
        {"WEST", Direction::West},
        {"UP", Direction::Up},
        {"DOWN", Direction::Down},
        {"NORTHEAST", Direction::Northeast},
        {"NORTHWEST", Direction::Northwest},
        {"SOUTHEAST", Direction::Southeast},
        {"SOUTHWEST", Direction::Southwest}};

    auto it = dir_map.find(dir_name);
    return it != dir_map.end() ? std::optional<Direction>{it->second} : std::nullopt;
}

Direction get_opposite_direction(Direction dir) {
    static const std::unordered_map<Direction, Direction> opposites = {{Direction::North, Direction::South},
                                                                       {Direction::South, Direction::North},
                                                                       {Direction::East, Direction::West},
                                                                       {Direction::West, Direction::East},
                                                                       {Direction::Up, Direction::Down},
                                                                       {Direction::Down, Direction::Up},
                                                                       {Direction::Northeast, Direction::Southwest},
                                                                       {Direction::Southwest, Direction::Northeast},
                                                                       {Direction::Northwest, Direction::Southeast},
                                                                       {Direction::Southeast, Direction::Northwest},
                                                                       {Direction::In, Direction::Out},
                                                                       {Direction::Out, Direction::In}};

    auto it = opposites.find(dir);
    return it != opposites.end() ? it->second : Direction::None;
}

std::string_view get_direction_abbrev(Direction dir) {
    static const std::unordered_map<Direction, std::string_view> abbrevs = {
        {Direction::North, "n"},      {Direction::East, "e"},       {Direction::South, "s"},
        {Direction::West, "w"},       {Direction::Up, "u"},         {Direction::Down, "d"},
        {Direction::Northeast, "ne"}, {Direction::Northwest, "nw"}, {Direction::Southeast, "se"},
        {Direction::Southwest, "sw"}, {Direction::In, "in"},        {Direction::Out, "out"}};

    auto it = abbrevs.find(dir);
    return it != abbrevs.end() ? it->second : "none";
}

std::string_view get_sector_name(SectorType sector) {
    auto name = magic_enum::enum_name(sector);
    return name.empty() ? "Undefined" : name;
}

std::optional<SectorType> parse_sector_type(std::string_view sector_name) {
    return magic_enum::enum_cast<SectorType>(sector_name);
}

SectorType sector_from_number(int sector_num) {
    // Map legacy CircleMUD sector numbers to modern SectorType enum
    // CircleMUD sector values: 0=Inside, 1=City, 2=Field, 3=Forest, 4=Hills,
    // 5=Mountain, 6=Water_Swim, 7=Water_NoSwim, 8=Air/Flying, 9=Underwater
    switch (sector_num) {
    case 0: return SectorType::Inside;
    case 1: return SectorType::City;
    case 2: return SectorType::Field;
    case 3: return SectorType::Forest;
    case 4: return SectorType::Hills;
    case 5: return SectorType::Mountains;
    case 6: return SectorType::Water_Swim;
    case 7: return SectorType::Water_Noswim;
    case 8: return SectorType::Flying;
    case 9: return SectorType::Underwater;
    case 10: return SectorType::Desert;
    case 11: return SectorType::Swamp;
    case 12: return SectorType::Beach;
    case 13: return SectorType::Road;
    case 14: return SectorType::Underground;
    case 15: return SectorType::Lava;
    case 16: return SectorType::Ice;
    case 17: return SectorType::Astral;
    case 18: return SectorType::Fire;
    case 19: return SectorType::Lightning;
    case 20: return SectorType::Spirit;
    case 21: return SectorType::Badlands;
    case 22: return SectorType::Void;
    default: return SectorType::Undefined;
    }
}

std::string_view get_flag_name(RoomFlag flag) {
    auto name = magic_enum::enum_name(flag);
    return name.empty() ? "Unknown" : name;
}

std::optional<RoomFlag> parse_room_flag(std::string_view flag_name) {
    // Handle legacy flag names that don't match enum names exactly
    if (flag_name == "ALWAYSLIT") {
        return RoomFlag::AlwaysLit;
    }

    return magic_enum::enum_cast<RoomFlag>(flag_name);
}

int get_movement_cost(SectorType sector) {
    static const std::unordered_map<SectorType, int> costs = {
        {SectorType::Inside, 1},      {SectorType::City, 1},         {SectorType::Field, 2},
        {SectorType::Forest, 3},      {SectorType::Hills, 3},        {SectorType::Mountains, 4},
        {SectorType::Water_Swim, 3},  {SectorType::Water_Noswim, 1}, // Can't move without boat
        {SectorType::Underwater, 4},  {SectorType::Flying, 1},       {SectorType::Desert, 3},
        {SectorType::Swamp, 4},       {SectorType::Beach, 2},        {SectorType::Road, 1},
        {SectorType::Underground, 2}, {SectorType::Lava, 5},         {SectorType::Ice, 2}};

    auto it = costs.find(sector);
    return it != costs.end() ? it->second : 2;
}

bool sector_allows_flying(SectorType sector) {
    return sector != SectorType::Underwater && sector != SectorType::Underground;
}

bool is_water_sector(SectorType sector) {
    return sector == SectorType::Water_Swim || sector == SectorType::Water_Noswim || sector == SectorType::Underwater;
}

bool requires_swimming(SectorType sector) {
    return sector == SectorType::Water_Swim || sector == SectorType::Underwater;
}

int get_sector_light_level(SectorType sector) {
    static const std::unordered_map<SectorType, int> light_levels = {
        {SectorType::Inside, 0},     {SectorType::City, 2},         {SectorType::Field, 3},
        {SectorType::Forest, 1},     {SectorType::Hills, 3},        {SectorType::Mountains, 3},
        {SectorType::Water_Swim, 2}, {SectorType::Water_Noswim, 2}, {SectorType::Underwater, 0},
        {SectorType::Flying, 4},     {SectorType::Desert, 4},       {SectorType::Swamp, 1},
        {SectorType::Beach, 3},      {SectorType::Road, 3},         {SectorType::Underground, 0},
        {SectorType::Lava, 5},       {SectorType::Ice, 2},          {SectorType::Fire, 6}};

    auto it = light_levels.find(sector);
    return it != light_levels.end() ? it->second : 2;
}

std::string_view get_sector_color(SectorType sector) {
    static const std::unordered_map<SectorType, std::string_view> colors = {
        {SectorType::Inside, "\033[0;37m"},       // White
        {SectorType::City, "\033[0;33m"},         // Yellow
        {SectorType::Field, "\033[0;32m"},        // Green
        {SectorType::Forest, "\033[0;32m"},       // Green
        {SectorType::Hills, "\033[0;33m"},        // Yellow
        {SectorType::Mountains, "\033[0;37m"},    // White
        {SectorType::Water_Swim, "\033[0;34m"},   // Blue
        {SectorType::Water_Noswim, "\033[0;34m"}, // Blue
        {SectorType::Underwater, "\033[0;36m"},   // Cyan
        {SectorType::Flying, "\033[0;37m"},       // White
        {SectorType::Desert, "\033[0;33m"},       // Yellow
        {SectorType::Swamp, "\033[0;32m"},        // Green
        {SectorType::Beach, "\033[0;33m"},        // Yellow
        {SectorType::Road, "\033[0;37m"},         // White
        {SectorType::Underground, "\033[0;30m"},  // Black
        {SectorType::Lava, "\033[0;31m"},         // Red
        {SectorType::Ice, "\033[0;36m"},          // Cyan
        {SectorType::Fire, "\033[0;31m"}          // Red
    };

    auto it = colors.find(sector);
    return it != colors.end() ? it->second : "\033[0;37m";
}
} // namespace RoomUtils

std::string Room::get_stat_info() const {
    std::ostringstream output;

    // Room name and basic info
    output << fmt::format("Room name: {}\n", name());

    output << fmt::format("Zone: [{}], ID: [{}], Sector: {}\n",
                          id().zone_id(), id(), magic_enum::enum_name(sector_type()));

    // Room flags
    std::string flag_str = "<None>";
    if (!flags_.empty()) {
        std::vector<std::string> flag_names;
        for (const auto &flag : flags_) {
            flag_names.push_back(std::string{magic_enum::enum_name(flag)});
        }
        std::string joined = "";
        for (size_t i = 0; i < flag_names.size(); ++i) {
            if (i > 0)
                joined += " ";
            joined += flag_names[i];
        }
        flag_str = joined;
    }
    output << fmt::format("SpecProc: None, Flags: {}\n", flag_str);

    output << fmt::format("Room effects: <None>\n");
    output << fmt::format("Ambient Light : {}\n", light_level_);

    // Description
    output << fmt::format("Description:\n{}\n", description().empty() ? "  None." : description());

    // Extra descriptions
    // Note: Extra descriptions feature pending Entity::extra_descriptions implementation
    output << "Extra descs: <None>\n";

    // Characters in room
    const auto &contents = contents_;
    output << "Chars present:";
    bool first_char = true;
    for (const auto &actor : contents.actors) {
        if (actor) {
            // Cast to specific types to get more information
            auto player = std::dynamic_pointer_cast<const Player>(actor);
            auto mobile = std::dynamic_pointer_cast<const Mobile>(actor);
            output << fmt::format("{} {}({})", first_char ? "" : ",", actor->name(),
                                  player ? "PC" : (mobile ? "MOB" : "NPC"));
            first_char = false;
        }
    }
    output << "\n";

    // Objects in room
    if (!contents.objects.empty()) {
        output << "Contents:";
        bool first_obj = true;
        for (const auto &obj : contents.objects) {
            if (obj) {
                output << fmt::format("{} {}", first_obj ? "" : ",",
                                      obj->short_description().empty() ? obj->name() : obj->short_description());
                first_obj = false;
            }
        }
        output << "\n";
    }

    // Exits information - iterate through all existing exits
    for (const auto &[direction, exit] : exits_) {
        std::string dir_name{magic_enum::enum_name(direction)};

        std::string to_room = "NONE";
        if (exit.to_room.is_valid()) {
            to_room = fmt::format("{}", static_cast<uint32_t>(exit.to_room.value()));
        }

        // Exit flags
        std::string exit_flags = "<None>";
        if (exit.has_door) {
            std::vector<std::string> flags;
            flags.push_back("DOOR");
            if (exit.is_closed)
                flags.push_back("CLOSED");
            if (exit.is_locked)
                flags.push_back("LOCKED");
            if (exit.is_pickproof)
                flags.push_back("PICKPROOF");
            std::string joined_flags = "";
            for (size_t i = 0; i < flags.size(); ++i) {
                if (i > 0)
                    joined_flags += " ";
                joined_flags += flags[i];
            }
            exit_flags = joined_flags;
        }

        output << fmt::format("Exit {:>5}:  To: [{}], Key: [{}], Keywrd: {}, Type: {}\n", dir_name, to_room,
                              exit.key_id.is_valid() ? static_cast<uint32_t>(exit.key_id.value()) : 0,
                              exit.keyword.empty() ? "None" : exit.keyword, exit_flags);

        if (!exit.description.empty()) {
            output << fmt::format("Extra Desc: {}\n", exit.description);
        }
    }

    return output.str();
}