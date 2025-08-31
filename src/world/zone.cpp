/***************************************************************************
 *   File: src/world/zone.cpp                             Part of FieryMUD *
 *  Usage: Modern zone system implementation                               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "zone.hpp"

#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../core/object.hpp"
#include "room.hpp"

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>

// ZoneCommand Implementation

bool ZoneCommand::should_execute() const {
    // TODO: Implement condition checking based on if_flag
    // For now, always execute
    return true;
}

std::string ZoneCommand::to_string() const {
    return fmt::format("{}: entity={} room={} container={} max={} - {}", ZoneUtils::get_command_type_name(command_type),
                       entity_id.is_valid() ? std::to_string(entity_id.value()) : "0",
                       room_id.is_valid() ? std::to_string(room_id.value()) : "0",
                       container_id.is_valid() ? std::to_string(container_id.value()) : "0", max_count, comment);
}

nlohmann::json ZoneCommand::to_json() const {
    nlohmann::json json;

    json["command_type"] = std::string(magic_enum::enum_name(command_type));
    json["if_flag"] = if_flag;

    if (entity_id.is_valid()) {
        json["entity_id"] = entity_id.value();
    }
    if (room_id.is_valid()) {
        json["room_id"] = room_id.value();
    }
    if (container_id.is_valid()) {
        json["container_id"] = container_id.value();
    }
    if (max_count != 1) {
        json["max_count"] = max_count;
    }
    if (!comment.empty()) {
        json["comment"] = comment;
    }

    return json;
}

Result<ZoneCommand> ZoneCommand::from_json(const nlohmann::json &json) {
    try {
        ZoneCommand cmd;

        if (json.contains("command_type")) {
            auto type_name = json["command_type"].get<std::string>();
            if (auto type = ZoneUtils::parse_command_type(type_name)) {
                cmd.command_type = type.value();
            } else {
                return std::unexpected(Errors::ParseError("Invalid command type", type_name));
            }
        }

        if (json.contains("if_flag")) {
            cmd.if_flag = json["if_flag"].get<int>();
        }

        // Handle both legacy field names and JSON format field names
        if (json.contains("entity_id")) {
            cmd.entity_id = EntityId{json["entity_id"].get<std::uint64_t>()};
        } else if (json.contains("id")) {
            cmd.entity_id = EntityId{json["id"].get<std::uint64_t>()};
        }

        if (json.contains("room_id")) {
            cmd.room_id = EntityId{json["room_id"].get<std::uint64_t>()};
        } else if (json.contains("room")) {
            cmd.room_id = EntityId{json["room"].get<std::uint64_t>()};
        }

        if (json.contains("container_id")) {
            cmd.container_id = EntityId{json["container_id"].get<std::uint64_t>()};
        } else if (json.contains("container")) {
            cmd.container_id = EntityId{json["container"].get<std::uint64_t>()};
        }

        if (json.contains("max_count")) {
            cmd.max_count = json["max_count"].get<int>();
        } else if (json.contains("max")) {
            cmd.max_count = json["max"].get<int>();
        }

        if (json.contains("comment")) {
            cmd.comment = json["comment"].get<std::string>();
        }

        return cmd;

    } catch (const nlohmann::json::exception &e) {
        return std::unexpected(Errors::ParseError("ZoneCommand JSON parsing", e.what()));
    }
}

// Zone Implementation

Zone::Zone(EntityId id, std::string_view name, int reset_minutes, ResetMode mode)
    : Entity(id, name), reset_minutes_(reset_minutes), reset_mode_(mode), min_level_(0), max_level_(100),
      first_room_(INVALID_ENTITY_ID), last_room_(INVALID_ENTITY_ID) {

    stats_.creation_time = std::chrono::steady_clock::now();
    stats_.last_reset = stats_.creation_time;
}

Result<std::unique_ptr<Zone>> Zone::create(EntityId id, std::string_view name, int reset_minutes, ResetMode mode) {
    if (!id.is_valid()) {
        return std::unexpected(Errors::InvalidArgument("id", "must be valid"));
    }

    if (name.empty()) {
        return std::unexpected(Errors::InvalidArgument("name", "cannot be empty"));
    }

    if (reset_minutes < 0) {
        return std::unexpected(Errors::InvalidArgument("reset_minutes", "cannot be negative"));
    }

    auto zone = std::unique_ptr<Zone>(new Zone(id, name, reset_minutes, mode));

    TRY(zone->validate());

    return zone;
}

Result<std::unique_ptr<Zone>> Zone::from_json_file(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return std::unexpected(Errors::FileNotFound(filename));
    }

    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::exception &e) {
        return std::unexpected(Errors::ParseError(fmt::format("Failed to parse {}", filename), e.what()));
    }

    return from_json(json);
}

Result<std::unique_ptr<Zone>> Zone::from_json(const nlohmann::json &json) {
    try {
        // Support both legacy nested schema { "zone": { ... } } and the flat
        // schema produced by Zone::to_json().
        const nlohmann::json *zone_data_ptr = nullptr;
        bool legacy_nested = false;

        if (json.contains("zone") && json["zone"].is_object()) {
            zone_data_ptr = &json["zone"];
            legacy_nested = true;
        } else {
            // Assume flat schema if no nested "zone" object exists
            if (!json.is_object()) {
                return std::unexpected(Errors::ParseError("Invalid JSON root for Zone", "object expected"));
            }
            zone_data_ptr = &json;
        }

        const auto &zone_data = *zone_data_ptr;

        // First create base entity
        auto base_result = Entity::from_json(zone_data);
        if (!base_result) {
            return std::unexpected(base_result.error());
        }

        auto base_entity = std::move(base_result.value());

        // Parse zone-specific fields from zone_data
        int reset_minutes = 30;
        if (zone_data.contains("lifespan")) {
            reset_minutes = zone_data["lifespan"].get<int>();
        } else if (zone_data.contains("reset_minutes")) {
            reset_minutes = zone_data["reset_minutes"].get<int>();
        }

        ResetMode mode = ResetMode::Empty;
        if (zone_data.contains("reset_mode")) {
            if (auto parsed_mode = ZoneUtils::parse_reset_mode(zone_data["reset_mode"].get<std::string>())) {
                mode = parsed_mode.value();
            }
        }

        auto zone = std::unique_ptr<Zone>(new Zone(base_entity->id(), base_entity->name(), reset_minutes, mode));

        // Copy base entity properties
        zone->set_keywords(base_entity->keywords());
        zone->set_description(base_entity->description());
        zone->set_short_description(base_entity->short_description());

        // Parse zone-specific properties from zone_data
        if (zone_data.contains("min_level")) {
            zone->set_min_level(zone_data["min_level"].get<int>());
        }

        if (zone_data.contains("max_level")) {
            zone->set_max_level(zone_data["max_level"].get<int>());
        }

        if (zone_data.contains("builders")) {
            zone->set_builders(zone_data["builders"].get<std::string>());
        }

        if (zone_data.contains("top")) {
            // Map "top" field to last room
            zone->set_last_room(EntityId{zone_data["top"].get<std::uint64_t>()});
        }

        if (zone_data.contains("first_room")) {
            zone->set_first_room(EntityId{zone_data["first_room"].get<std::uint64_t>()});
        }

        if (zone_data.contains("last_room")) {
            zone->set_last_room(EntityId{zone_data["last_room"].get<std::uint64_t>()});
        }

        // Parse flags from zone_data
        if (zone_data.contains("flags") && zone_data["flags"].is_array()) {
            for (const auto &flag_name : zone_data["flags"]) {
                if (flag_name.is_string()) {
                    std::string flag_str = flag_name.get<std::string>();
                    if (auto flag = ZoneUtils::parse_zone_flag(flag_str)) {
                        zone->set_flag(flag.value());
                    }
                }
            }
        }

        // Parse rooms
        if (legacy_nested) {
            // Legacy nested schema: json["rooms"]["rooms"] is an array of room objects with string ids
            if (json.contains("rooms") && json["rooms"].is_object() && json["rooms"].contains("rooms") &&
                json["rooms"]["rooms"].is_array()) {
                for (const auto &room_json : json["rooms"]["rooms"]) {
                    if (room_json.contains("id")) {
                        std::string room_id_str = room_json["id"].get<std::string>();
                        auto room_id = std::stoull(room_id_str);
                        zone->add_room(EntityId{room_id});
                    }
                }
            }
        } else {
            // Flat schema: an array of numeric room ids under "rooms"
            if (json.contains("rooms") && json["rooms"].is_array()) {
                for (const auto &rid : json["rooms"]) {
                    if (rid.is_number_unsigned()) {
                        zone->add_room(EntityId{rid.get<std::uint64_t>()});
                    }
                }
            }
        }

        // Parse commands - support both flat array and nested object formats
        if (zone_data.contains("commands")) {
            const auto &cmds = zone_data["commands"];
            if (cmds.is_array()) {
                // Flat array format
                for (const auto &cmd_json : cmds) {
                    auto cmd_res = ZoneCommand::from_json(cmd_json);
                    if (cmd_res) {
                        zone->add_command(cmd_res.value());
                    } else {
                        // If a command fails to parse, surface the error
                        return std::unexpected(cmd_res.error());
                    }
                }
            } else if (cmds.is_object()) {
                // Nested object format with mob/object sections
                auto parse_result = parse_nested_zone_commands(cmds, zone.get());
                if (!parse_result) {
                    return std::unexpected(parse_result.error());
                }
            }
        }

        TRY(zone->validate());

        return zone;

    } catch (const nlohmann::json::exception &e) {
        return std::unexpected(Errors::ParseError("Zone JSON parsing", e.what()));
    }
}

bool Zone::has_flag(ZoneFlag flag) const { return flags_.contains(flag); }

void Zone::set_flag(ZoneFlag flag, bool value) {
    if (value) {
        flags_.insert(flag);
    } else {
        flags_.erase(flag);
    }
}

void Zone::add_room(EntityId room_id) {
    if (room_id.is_valid()) {
        rooms_.insert(room_id);
    }
}

void Zone::remove_room(EntityId room_id) { rooms_.erase(room_id); }

bool Zone::contains_room(EntityId room_id) const { return rooms_.contains(room_id); }

void Zone::add_command(const ZoneCommand &cmd) { commands_.push_back(cmd); }

bool Zone::needs_reset() const {
    if (reset_mode_ == ResetMode::Never || reset_mode_ == ResetMode::Manual) {
        return false;
    }

    if (reset_mode_ == ResetMode::OnReboot) {
        // TODO: Check if this is a reboot situation
        return false;
    }

    auto time_since_reset = stats_.time_since_reset();
    auto reset_interval = std::chrono::minutes(reset_minutes_);

    if (time_since_reset < reset_interval) {
        return false;
    }

    if (reset_mode_ == ResetMode::Empty) {
        return is_empty_of_players();
    }

    return true; // ResetMode::Always
}

void Zone::force_reset() {
    stats_.last_reset = std::chrono::steady_clock::now();
    stats_.reset_count++;

    auto logger = Log::game();
    logger->info("Zone {} ({}) forced reset", name(), id());

    // Clean up existing mobiles in this zone before respawning
    if (cleanup_zone_mobiles_callback_) {
        logger->debug("Cleaning up existing mobiles in zone {}", id());
        cleanup_zone_mobiles_callback_(id());
    }

    // Execute reset commands
    auto result = execute_reset();
    if (!result) {
        logger->error("Zone {} reset failed: {}", name(), result.error().message);
    }
}

Result<void> Zone::execute_reset() {
    auto logger = Log::game();
    logger->debug("Executing reset for zone {} ({})", name(), id());

    for (const auto &cmd : commands_) {
        if (!cmd.should_execute()) {
            continue;
        }

        auto result = execute_command(cmd);
        if (!result) {
            logger->warn("Zone {} command failed: {} - {}", name(), cmd.to_string(), result.error().message);
            continue;
        }

        // If command returned false, halt processing
        if (!result.value()) {
            logger->debug("Zone {} reset halted at command: {}", name(), cmd.to_string());
            break;
        }
    }

    update_statistics();

    return Success();
}

Result<void> Zone::save_to_file(const std::string &filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return std::unexpected(Errors::FileAccessError(fmt::format("Cannot write to {}", filename)));
    }

    try {
        nlohmann::json json = to_json();
        file << json.dump(2) << std::endl;
        return Success();
    } catch (const nlohmann::json::exception &e) {
        return std::unexpected(
            Errors::SerializationError(fmt::format("Failed to save zone to {}", filename), e.what()));
    }
}

Result<void> Zone::reload_from_file(const std::string &filename) {
    auto new_zone_result = from_json_file(filename);
    if (!new_zone_result) {
        return std::unexpected(new_zone_result.error());
    }

    auto new_zone = std::move(new_zone_result.value());

    // Update this zone with new data while preserving runtime state
    set_name(new_zone->name());
    set_description(new_zone->description());
    set_keywords(new_zone->keywords());

    reset_minutes_ = new_zone->reset_minutes_;
    reset_mode_ = new_zone->reset_mode_;
    min_level_ = new_zone->min_level_;
    max_level_ = new_zone->max_level_;
    builders_ = new_zone->builders_;
    flags_ = new_zone->flags_;
    first_room_ = new_zone->first_room_;
    last_room_ = new_zone->last_room_;
    commands_ = new_zone->commands_;

    // Preserve runtime statistics
    // Don't update stats_, rooms_ to maintain current state

    return Success();
}

nlohmann::json Zone::to_json() const {
    nlohmann::json json = Entity::to_json();

    json["reset_minutes"] = reset_minutes_;
    json["reset_mode"] = std::string(magic_enum::enum_name(reset_mode_));
    json["min_level"] = min_level_;
    json["max_level"] = max_level_;

    if (!builders_.empty()) {
        json["builders"] = builders_;
    }

    if (first_room_.is_valid()) {
        json["first_room"] = first_room_.value();
    }

    if (last_room_.is_valid()) {
        json["last_room"] = last_room_.value();
    }

    // Serialize flags
    std::vector<std::string> flag_names;
    for (ZoneFlag flag : flags_) {
        flag_names.emplace_back(magic_enum::enum_name(flag));
    }
    if (!flag_names.empty()) {
        json["flags"] = flag_names;
    }

    // Serialize rooms
    std::vector<std::uint64_t> room_ids;
    for (EntityId room_id : rooms_) {
        room_ids.push_back(room_id.value());
    }
    if (!room_ids.empty()) {
        json["rooms"] = room_ids;
    }

    // Serialize commands
    std::vector<nlohmann::json> cmd_array;
    for (const auto &cmd : commands_) {
        cmd_array.push_back(cmd.to_json());
    }
    if (!cmd_array.empty()) {
        json["commands"] = cmd_array;
    }

    return json;
}

Result<void> Zone::validate() const {
    TRY(Entity::validate());

    if (reset_minutes_ < 0) {
        return std::unexpected(Errors::InvalidState("Zone reset minutes cannot be negative"));
    }

    if (min_level_ < 0 || max_level_ < min_level_) {
        return std::unexpected(Errors::InvalidState("Zone level range is invalid"));
    }

    return Success();
}

Result<bool> Zone::execute_command(const ZoneCommand &cmd) {
    auto logger = Log::game();

    switch (cmd.command_type) {
    case ZoneCommandType::Comment:
        return true; // Always continue

    case ZoneCommandType::Halt:
        return false; // Stop processing

    case ZoneCommandType::Wait:
        // TODO: Implement wait functionality
        logger->debug("Zone {} executing wait command ({}s)", name(), cmd.max_count);
        return true;

    case ZoneCommandType::Load_Mobile:
        return execute_load_mobile(cmd);

    case ZoneCommandType::Load_Object:
        return execute_load_object(cmd);

    case ZoneCommandType::Give_Object:
        return execute_give_object(cmd);

    case ZoneCommandType::Equip_Object:
        return execute_equip_object(cmd);

    case ZoneCommandType::Put_Object:
        return execute_put_object(cmd);

    case ZoneCommandType::Open_Door:
    case ZoneCommandType::Close_Door:
    case ZoneCommandType::Lock_Door:
    case ZoneCommandType::Unlock_Door:
        // TODO: Implement door manipulation
        logger->debug("Zone {} door command {} room {} dir {}", name(), cmd.command_type, cmd.room_id, cmd.entity_id);
        return true;

    default:
        logger->warn("Zone {} unknown command type: {}", name(), cmd.command_type);
        return true;
    }
}

bool Zone::is_empty_of_players() const {
    // TODO: Check if any players are in zone rooms
    return stats_.player_count == 0;
}

void Zone::update_statistics() {
    // TODO: Count actual players, mobiles, and objects in zone
    // For now, just update the reset time
    stats_.last_reset = std::chrono::steady_clock::now();
}

// ZoneCommandParser Implementation

Result<ZoneCommand> ZoneCommandParser::parse_command_line(std::string_view line) {
    // Skip empty lines and comments
    if (line.empty() || line[0] == '*') {
        ZoneCommand cmd;
        cmd.command_type = ZoneCommandType::Comment;
        cmd.comment = std::string(line);
        return cmd;
    }

    // Parse command format: "C if-flag arg1 arg2 arg3 arg4"
    std::regex cmd_regex(R"(([MOEPDGLRFTW*]) +(\d+) +(\d+) +(\d+) +(\d+) +(\d+))");
    std::smatch match;
    std::string line_str{line};

    if (!std::regex_match(line_str, match, cmd_regex)) {
        return std::unexpected(Errors::ParseError("Invalid zone command format", line));
    }

    char cmd_char = match[1].str()[0];
    auto command_type = ZoneUtils::get_command_type_from_char(cmd_char);
    if (!command_type) {
        return std::unexpected(Errors::ParseError("Unknown zone command type", std::string{cmd_char}));
    }

    ZoneCommand cmd;
    cmd.command_type = command_type.value();
    cmd.if_flag = std::stoi(match[2].str());
    cmd.entity_id = EntityId{static_cast<std::uint64_t>(std::stoul(match[3].str()))};
    cmd.room_id = EntityId{static_cast<std::uint64_t>(std::stoul(match[4].str()))};
    cmd.container_id = EntityId{static_cast<std::uint64_t>(std::stoul(match[5].str()))};
    cmd.max_count = std::stoi(match[6].str());
    cmd.comment = std::string(line);

    return cmd;
}

Result<std::vector<ZoneCommand>> ZoneCommandParser::parse_commands(std::string_view text) {
    std::vector<ZoneCommand> commands;
    std::istringstream stream{std::string{text}};
    std::string line;

    while (std::getline(stream, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) {
            continue;
        }

        auto cmd_result = parse_command_line(line);
        if (cmd_result) {
            commands.push_back(cmd_result.value());
        } else {
            return std::unexpected(cmd_result.error());
        }
    }

    return commands;
}

std::string ZoneCommandParser::format_command(const ZoneCommand &cmd) {
    char cmd_char = ZoneUtils::get_char_from_command_type(cmd.command_type);

    // Base formatted command
    auto base =
        fmt::format("{} {} {} {} {} {}", cmd_char, cmd.if_flag, cmd.entity_id.is_valid() ? cmd.entity_id.value() : 0,
                    cmd.room_id.is_valid() ? cmd.room_id.value() : 0,
                    cmd.container_id.is_valid() ? cmd.container_id.value() : 0, cmd.max_count);

    // Only append comment delimiter and text if a comment is present
    if (!cmd.comment.empty()) {
        return fmt::format("{} ; {}", base, cmd.comment);
    }
    return base;
}

// ZoneUtils Implementation

namespace ZoneUtils {
std::string_view get_reset_mode_name(ResetMode mode) {
    auto name = magic_enum::enum_name(mode);
    return name.empty() ? "Unknown" : name;
}

std::optional<ResetMode> parse_reset_mode(std::string_view mode_name) {
    return magic_enum::enum_cast<ResetMode>(mode_name);
}

std::string_view get_flag_name(ZoneFlag flag) {
    auto name = magic_enum::enum_name(flag);
    return name.empty() ? "Unknown" : name;
}

std::optional<ZoneFlag> parse_zone_flag(std::string_view flag_name) {
    return magic_enum::enum_cast<ZoneFlag>(flag_name);
}

std::string_view get_command_type_name(ZoneCommandType type) {
    auto name = magic_enum::enum_name(type);
    return name.empty() ? "Unknown" : name;
}

std::optional<ZoneCommandType> parse_command_type(std::string_view type_name) {
    return magic_enum::enum_cast<ZoneCommandType>(type_name);
}

std::optional<ZoneCommandType> get_command_type_from_char(char cmd_char) {
    static const std::unordered_map<char, ZoneCommandType> char_map = {
        {'M', ZoneCommandType::Load_Mobile},  {'O', ZoneCommandType::Load_Object},
        {'E', ZoneCommandType::Equip_Object}, {'P', ZoneCommandType::Put_Object},
        {'G', ZoneCommandType::Give_Object},  {'D', ZoneCommandType::Open_Door},
        {'L', ZoneCommandType::Close_Door},   {'R', ZoneCommandType::Remove_Object},
        {'T', ZoneCommandType::Trigger},      {'F', ZoneCommandType::Follow_Mobile},
        {'W', ZoneCommandType::Wait},         {'*', ZoneCommandType::Comment}};

    auto it = char_map.find(cmd_char);
    return it != char_map.end() ? std::optional<ZoneCommandType>{it->second} : std::nullopt;
}

char get_char_from_command_type(ZoneCommandType type) {
    static const std::unordered_map<ZoneCommandType, char> type_map = {
        {ZoneCommandType::Load_Mobile, 'M'},  {ZoneCommandType::Load_Object, 'O'},
        {ZoneCommandType::Equip_Object, 'E'}, {ZoneCommandType::Put_Object, 'P'},
        {ZoneCommandType::Give_Object, 'G'},  {ZoneCommandType::Open_Door, 'D'},
        {ZoneCommandType::Close_Door, 'L'},   {ZoneCommandType::Remove_Object, 'R'},
        {ZoneCommandType::Trigger, 'T'},      {ZoneCommandType::Follow_Mobile, 'F'},
        {ZoneCommandType::Wait, 'W'},         {ZoneCommandType::Comment, '*'}};

    auto it = type_map.find(type);
    return it != type_map.end() ? it->second : '?';
}

bool zone_file_exists(const std::string &filename) {
    std::ifstream file(filename);
    return file.good();
}

std::string get_zone_file_path(int zone_number) { return fmt::format("lib/world/zon/{}.zon", zone_number); }

std::optional<int> extract_zone_number(const std::string &filename) {
    std::regex zone_regex(R"((\d+)\.zon$)");
    std::smatch match;

    if (std::regex_search(filename, match, zone_regex)) {
        return std::stoi(match[1].str());
    }

    return std::nullopt;
}

Result<void> validate_zone_file(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return std::unexpected(Errors::FileNotFound(filename));
    }

    try {
        nlohmann::json json;
        file >> json;

        // Basic validation
        if (!json.contains("name")) {
            return std::unexpected(Errors::ParseError("Zone file missing required 'name' field", filename));
        }

        if (!json.contains("id")) {
            return std::unexpected(Errors::ParseError("Zone file missing required 'id' field", filename));
        }

        return Success();

    } catch (const nlohmann::json::exception &e) {
        return std::unexpected(Errors::ParseError(fmt::format("Invalid JSON in {}", filename), e.what()));
    }
}
} // namespace ZoneUtils

// Zone command implementations

Result<bool> Zone::execute_load_mobile(const ZoneCommand &cmd) {
    auto logger = Log::game();

    EntityId mobile_id = cmd.entity_id;
    EntityId room_id = cmd.room_id;

    logger->debug("Zone {} loading mobile {} in room {}", name(), mobile_id, room_id);

    if (!spawn_mobile_callback_) {
        logger->warn("Zone {} has no spawn mobile callback set", name());
        return true; // Continue processing but don't spawn
    }

    // Verify room exists
    if (get_room_callback_) {
        auto room = get_room_callback_(room_id);
        if (!room) {
            logger->error("Zone {} cannot load mobile {} - room {} not found", name(), mobile_id, room_id);
            return true; // Continue processing
        }
    }

    // Spawn the mobile
    auto mobile = spawn_mobile_callback_(mobile_id, room_id);
    if (mobile) {
        logger->info("Zone {} spawned mobile '{}' ({}) in room {}", name(), mobile->display_name(), mobile_id, room_id);
        stats_.mobile_count++; // Track spawning stats
    } else {
        logger->warn("Zone {} failed to spawn mobile {} in room {}", name(), mobile_id, room_id);
    }

    return true; // Continue processing
}

Result<void> Zone::parse_nested_zone_commands(const nlohmann::json& commands_json, Zone* zone) {
    try {
        // Parse mobile commands
        if (commands_json.contains("mob") && commands_json["mob"].is_array()) {
            for (const auto& mob_cmd : commands_json["mob"]) {
                // Load mobile command
                ZoneCommand load_mobile;
                load_mobile.command_type = ZoneCommandType::Load_Mobile;
                load_mobile.entity_id = EntityId{mob_cmd["id"].get<std::uint64_t>()};
                load_mobile.room_id = EntityId{mob_cmd["room"].get<std::uint64_t>()};
                load_mobile.max_count = mob_cmd.contains("max") ? mob_cmd["max"].get<int>() : 1;
                zone->add_command(load_mobile);
                
                // Parse carried objects
                if (mob_cmd.contains("carrying") && mob_cmd["carrying"].is_array()) {
                    for (const auto& carry_obj : mob_cmd["carrying"]) {
                        ZoneCommand give_obj;
                        give_obj.command_type = ZoneCommandType::Give_Object;
                        give_obj.entity_id = EntityId{carry_obj["id"].get<std::uint64_t>()};
                        give_obj.container_id = load_mobile.entity_id; // Mobile ID
                        give_obj.max_count = carry_obj.contains("max") ? carry_obj["max"].get<int>() : 1;
                        zone->add_command(give_obj);
                        
                        // Handle nested containers
                        if (carry_obj.contains("contains") && carry_obj["contains"].is_array()) {
                            for (const auto& nested_obj : carry_obj["contains"]) {
                                ZoneCommand put_obj;
                                put_obj.command_type = ZoneCommandType::Put_Object;
                                put_obj.entity_id = EntityId{nested_obj["id"].get<std::uint64_t>()};
                                put_obj.container_id = give_obj.entity_id; // Container object ID
                                put_obj.max_count = nested_obj.contains("max") ? nested_obj["max"].get<int>() : 1;
                                zone->add_command(put_obj);
                            }
                        }
                    }
                }
                
                // Parse equipped objects
                if (mob_cmd.contains("equipped") && mob_cmd["equipped"].is_array()) {
                    for (const auto& equip_obj : mob_cmd["equipped"]) {
                        ZoneCommand equip_cmd;
                        equip_cmd.command_type = ZoneCommandType::Equip_Object;
                        equip_cmd.entity_id = EntityId{equip_obj["id"].get<std::uint64_t>()};
                        equip_cmd.container_id = load_mobile.entity_id; // Mobile ID
                        equip_cmd.max_count = equip_obj.contains("location") ? equip_obj["location"].get<int>() : 0;
                        zone->add_command(equip_cmd);
                    }
                }
            }
        }
        
        // Parse standalone object commands
        if (commands_json.contains("object") && commands_json["object"].is_array()) {
            for (const auto& obj_cmd : commands_json["object"]) {
                ZoneCommand load_object;
                load_object.command_type = ZoneCommandType::Load_Object;
                load_object.entity_id = EntityId{obj_cmd["id"].get<std::uint64_t>()};
                load_object.room_id = EntityId{obj_cmd["room"].get<std::uint64_t>()};
                load_object.max_count = obj_cmd.contains("max") ? obj_cmd["max"].get<int>() : 1;
                zone->add_command(load_object);
                
                // Parse objects to create inside this object
                if (obj_cmd.contains("create_objects") && obj_cmd["create_objects"].is_array()) {
                    for (const auto& create_obj : obj_cmd["create_objects"]) {
                        ZoneCommand put_obj;
                        put_obj.command_type = ZoneCommandType::Put_Object;
                        put_obj.entity_id = EntityId{create_obj["id"].get<std::uint64_t>()};
                        put_obj.container_id = load_object.entity_id; // Container object ID
                        put_obj.max_count = create_obj.contains("max") ? create_obj["max"].get<int>() : 1;
                        zone->add_command(put_obj);
                    }
                }
            }
        }
        
        // Parse remove commands
        if (commands_json.contains("remove") && commands_json["remove"].is_array()) {
            for (const auto& remove_cmd : commands_json["remove"]) {
                ZoneCommand remove_obj;
                remove_obj.command_type = ZoneCommandType::Remove_Object;
                remove_obj.entity_id = EntityId{remove_cmd["id"].get<std::uint64_t>()};
                remove_obj.room_id = EntityId{remove_cmd["room"].get<std::uint64_t>()};
                zone->add_command(remove_obj);
            }
        }
        
        // Parse door commands
        if (commands_json.contains("door") && commands_json["door"].is_array()) {
            for (const auto& door_cmd : commands_json["door"]) {
                ZoneCommand door_command;
                std::string state = "open";
                if (door_cmd.contains("state")) {
                    const auto& state_value = door_cmd["state"];
                    if (state_value.is_string()) {
                        state = state_value.get<std::string>();
                    } else if (state_value.is_array() && !state_value.empty()) {
                        state = state_value[0].get<std::string>();
                    }
                }
                
                if (state == "open") {
                    door_command.command_type = ZoneCommandType::Open_Door;
                } else if (state == "close" || state == "closed") {
                    door_command.command_type = ZoneCommandType::Close_Door;
                } else if (state == "lock" || state == "locked") {
                    door_command.command_type = ZoneCommandType::Lock_Door;
                } else if (state == "unlock" || state == "unlocked") {
                    door_command.command_type = ZoneCommandType::Unlock_Door;
                } else {
                    door_command.command_type = ZoneCommandType::Close_Door; // Default
                }
                
                door_command.room_id = EntityId{door_cmd["room"].get<std::uint64_t>()};
                
                // Parse direction string to Direction enum
                std::uint64_t direction_value = 0;
                if (door_cmd.contains("direction")) {
                    std::string dir_str = door_cmd["direction"].get<std::string>();
                    if (auto dir_enum = magic_enum::enum_cast<Direction>(dir_str)) {
                        direction_value = static_cast<std::uint64_t>(dir_enum.value());
                    }
                }
                door_command.entity_id = EntityId{direction_value};
                zone->add_command(door_command);
            }
        }
        
        return Success();
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Failed to parse nested zone commands", e.what()));
    }
}

Result<bool> Zone::execute_load_object(const ZoneCommand &cmd) {
    auto logger = Log::game();

    EntityId object_id = cmd.entity_id;
    EntityId room_id = cmd.room_id;

    logger->debug("Zone {} loading object {} in room {}", name(), object_id, room_id);

    if (!spawn_object_callback_) {
        logger->warn("Zone {} has no spawn object callback set", name());
        return true; // Continue processing but don't spawn
    }

    // Verify room exists
    if (get_room_callback_) {
        auto room = get_room_callback_(room_id);
        if (!room) {
            logger->error("Zone {} cannot load object {} - room {} not found", name(), object_id, room_id);
            return true; // Continue processing
        }
    }

    // Spawn the object
    auto object = spawn_object_callback_(object_id, room_id);
    if (object) {
        logger->info("Zone {} spawned object '{}' ({}) in room {}", name(), object->display_name(), object_id, room_id);
        stats_.object_count++; // Track spawning stats
    } else {
        logger->warn("Zone {} failed to spawn object {} in room {}", name(), object_id, room_id);
    }

    return true; // Continue processing
}

Result<bool> Zone::execute_give_object(const ZoneCommand &cmd) {
    auto logger = Log::game();

    EntityId object_id = cmd.entity_id;
    EntityId mobile_id = cmd.container_id;

    logger->debug("Zone {} giving object {} to mobile {}", name(), object_id, mobile_id);

    if (!spawn_object_callback_) {
        logger->warn("Zone {} has no spawn object callback set", name());
        return true; // Continue processing but don't give
    }

    // For object giving, we need to create the object and add it to mobile's inventory
    // Since we don't have direct mobile lookup, we'll spawn the object "on" the mobile
    // The spawn callback should handle placing it in the correct mobile's inventory
    auto object = spawn_object_callback_(object_id, EntityId{mobile_id.value()});
    if (object) {
        logger->info("Zone {} gave object '{}' ({}) to mobile {}", name(), object->display_name(), object_id, mobile_id);
        stats_.object_count++; // Track spawning stats
    } else {
        logger->warn("Zone {} failed to give object {} to mobile {}", name(), object_id, mobile_id);
    }

    return true; // Continue processing
}

Result<bool> Zone::execute_equip_object(const ZoneCommand &cmd) {
    auto logger = Log::game();

    EntityId object_id = cmd.entity_id;
    EntityId mobile_id = cmd.container_id;
    int equipment_slot = cmd.max_count; // Equipment slot stored in max_count field

    logger->debug("Zone {} equipping object {} on mobile {} slot {}", name(), object_id, mobile_id, equipment_slot);

    if (!spawn_object_callback_) {
        logger->warn("Zone {} has no spawn object callback set", name());
        return true; // Continue processing but don't equip
    }

    // For object equipping, we create the object and it should be equipped on the mobile
    // Use a special encoding for equipment: negative room ID to indicate equipment
    // Equipment slot is encoded in the max_count field
    EntityId equip_room_id{mobile_id.value() | (static_cast<uint64_t>(equipment_slot) << 32)};
    auto object = spawn_object_callback_(object_id, equip_room_id);
    if (object) {
        logger->info("Zone {} equipped object '{}' ({}) on mobile {} slot {}", 
                    name(), object->display_name(), object_id, mobile_id, equipment_slot);
        stats_.object_count++; // Track spawning stats
    } else {
        logger->warn("Zone {} failed to equip object {} on mobile {} slot {}", 
                    name(), object_id, mobile_id, equipment_slot);
    }

    return true; // Continue processing
}

Result<bool> Zone::execute_put_object(const ZoneCommand &cmd) {
    auto logger = Log::game();

    EntityId object_id = cmd.entity_id;
    EntityId container_id = cmd.container_id;

    logger->debug("Zone {} putting object {} in container {}", name(), object_id, container_id);

    if (!spawn_object_callback_) {
        logger->warn("Zone {} has no spawn object callback set", name());
        return true; // Continue processing but don't put
    }

    // For putting objects in containers, we create the object inside the container
    // Use a special encoding: container object ID as the room ID
    auto object = spawn_object_callback_(object_id, container_id);
    if (object) {
        logger->info("Zone {} put object '{}' ({}) in container {}", name(), object->display_name(), object_id, container_id);
        stats_.object_count++; // Track spawning stats
    } else {
        logger->warn("Zone {} failed to put object {} in container {}", name(), object_id, container_id);
    }

    return true; // Continue processing
}
