/***************************************************************************
 *   File: src/world/zone.cpp                             Part of FieryMUD *
 *  Usage: Modern zone system implementation                               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "zone.hpp"
#include "room.hpp"
#include "../core/actor.hpp"
#include "../core/object.hpp"
#include "../core/logging.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

// ZoneCommand Implementation

bool ZoneCommand::should_execute() const {
    // TODO: Implement condition checking based on if_flag
    // For now, always execute
    return true;
}

std::string ZoneCommand::to_string() const {
    return fmt::format("{}: entity={} room={} container={} max={} - {}", 
                      ZoneUtils::get_command_type_name(command_type),
                      entity_id.is_valid() ? std::to_string(entity_id.value()) : "0",
                      room_id.is_valid() ? std::to_string(room_id.value()) : "0", 
                      container_id.is_valid() ? std::to_string(container_id.value()) : "0",
                      max_count,
                      comment);
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

Result<ZoneCommand> ZoneCommand::from_json(const nlohmann::json& json) {
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
        
        if (json.contains("entity_id")) {
            cmd.entity_id = EntityId{json["entity_id"].get<std::uint64_t>()};
        }
        
        if (json.contains("room_id")) {
            cmd.room_id = EntityId{json["room_id"].get<std::uint64_t>()};
        }
        
        if (json.contains("container_id")) {
            cmd.container_id = EntityId{json["container_id"].get<std::uint64_t>()};
        }
        
        if (json.contains("max_count")) {
            cmd.max_count = json["max_count"].get<int>();
        }
        
        if (json.contains("comment")) {
            cmd.comment = json["comment"].get<std::string>();
        }
        
        return cmd;
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("ZoneCommand JSON parsing", e.what()));
    }
}

// Zone Implementation

Zone::Zone(EntityId id, std::string_view name, int reset_minutes, ResetMode mode)
    : Entity(id, name), reset_minutes_(reset_minutes), reset_mode_(mode),
      min_level_(0), max_level_(100), first_room_(INVALID_ENTITY_ID), 
      last_room_(INVALID_ENTITY_ID) {
    
    stats_.creation_time = std::chrono::steady_clock::now();
    stats_.last_reset = stats_.creation_time;
}

Result<std::unique_ptr<Zone>> Zone::create(EntityId id, std::string_view name, 
                                          int reset_minutes, ResetMode mode) {
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

Result<std::unique_ptr<Zone>> Zone::from_json_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return std::unexpected(Errors::FileNotFound(filename));
    }
    
    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError(fmt::format("Failed to parse {}", filename), e.what()));
    }
    
    return from_json(json);
}

Result<std::unique_ptr<Zone>> Zone::from_json(const nlohmann::json& json) {
    try {
        // First create base entity
        auto base_result = Entity::from_json(json);
        if (!base_result) {
            return std::unexpected(base_result.error());
        }
        
        auto base_entity = std::move(base_result.value());
        
        // Parse zone-specific fields
        int reset_minutes = 30;
        if (json.contains("reset_minutes")) {
            reset_minutes = json["reset_minutes"].get<int>();
        }
        
        ResetMode mode = ResetMode::Empty;
        if (json.contains("reset_mode")) {
            if (auto parsed_mode = ZoneUtils::parse_reset_mode(json["reset_mode"].get<std::string>())) {
                mode = parsed_mode.value();
            }
        }
        
        auto zone = std::unique_ptr<Zone>(new Zone(base_entity->id(), base_entity->name(), reset_minutes, mode));
        
        // Copy base entity properties
        zone->set_keywords(base_entity->keywords());
        zone->set_description(base_entity->description());
        zone->set_short_description(base_entity->short_description());
        
        // Parse zone-specific properties
        if (json.contains("min_level")) {
            zone->set_min_level(json["min_level"].get<int>());
        }
        
        if (json.contains("max_level")) {
            zone->set_max_level(json["max_level"].get<int>());
        }
        
        if (json.contains("builders")) {
            zone->set_builders(json["builders"].get<std::string>());
        }
        
        if (json.contains("first_room")) {
            zone->set_first_room(EntityId{json["first_room"].get<std::uint64_t>()});
        }
        
        if (json.contains("last_room")) {
            zone->set_last_room(EntityId{json["last_room"].get<std::uint64_t>()});
        }
        
        // Parse flags
        if (json.contains("flags") && json["flags"].is_array()) {
            for (const auto& flag_name : json["flags"]) {
                if (flag_name.is_string()) {
                    std::string flag_str = flag_name.get<std::string>();
                    if (auto flag = ZoneUtils::parse_zone_flag(flag_str)) {
                        zone->set_flag(flag.value());
                    }
                }
            }
        }
        
        // Parse rooms
        if (json.contains("rooms") && json["rooms"].is_array()) {
            for (const auto& room_id : json["rooms"]) {
                if (room_id.is_number()) {
                    zone->add_room(EntityId{room_id.get<std::uint64_t>()});
                }
            }
        }
        
        // Parse commands
        if (json.contains("commands") && json["commands"].is_array()) {
            for (const auto& cmd_json : json["commands"]) {
                auto cmd_result = ZoneCommand::from_json(cmd_json);
                if (cmd_result) {
                    zone->add_command(cmd_result.value());
                }
            }
        }
        
        TRY(zone->validate());
        
        return zone;
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Zone JSON parsing", e.what()));
    }
}

bool Zone::has_flag(ZoneFlag flag) const {
    return flags_.contains(flag);
}

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

void Zone::remove_room(EntityId room_id) {
    rooms_.erase(room_id);
}

bool Zone::contains_room(EntityId room_id) const {
    return rooms_.contains(room_id);
}

void Zone::add_command(const ZoneCommand& cmd) {
    commands_.push_back(cmd);
}

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
    
    // Execute reset commands
    auto result = execute_reset();
    if (!result) {
        logger->error("Zone {} reset failed: {}", name(), result.error().message);
    }
}

Result<void> Zone::execute_reset() {
    auto logger = Log::game();
    logger->debug("Executing reset for zone {} ({})", name(), id());
    
    for (const auto& cmd : commands_) {
        if (!cmd.should_execute()) {
            continue;
        }
        
        auto result = execute_command(cmd);
        if (!result) {
            logger->warn("Zone {} command failed: {} - {}", 
                        name(), cmd.to_string(), result.error().message);
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

Result<void> Zone::save_to_file(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return std::unexpected(Errors::FileAccessError(fmt::format("Cannot write to {}", filename)));
    }
    
    try {
        nlohmann::json json = to_json();
        file << json.dump(2) << std::endl;
        return Success();
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::SerializationError(fmt::format("Failed to save zone to {}", filename), e.what()));
    }
}

Result<void> Zone::reload_from_file(const std::string& filename) {
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
    for (const auto& cmd : commands_) {
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

Result<bool> Zone::execute_command(const ZoneCommand& cmd) {
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
            // TODO: Implement object giving
            logger->debug("Zone {} giving object {} to mobile {}", 
                         name(), cmd.entity_id, cmd.container_id);
            return true;
            
        case ZoneCommandType::Equip_Object:
            // TODO: Implement object equipping
            logger->debug("Zone {} equipping object {} on mobile {} slot {}", 
                         name(), cmd.entity_id, cmd.container_id, cmd.max_count);
            return true;
            
        case ZoneCommandType::Put_Object:
            // TODO: Implement object placing
            logger->debug("Zone {} putting object {} in container {}", 
                         name(), cmd.entity_id, cmd.container_id);
            return true;
            
        case ZoneCommandType::Open_Door:
        case ZoneCommandType::Close_Door:
        case ZoneCommandType::Lock_Door:
        case ZoneCommandType::Unlock_Door:
            // TODO: Implement door manipulation
            logger->debug("Zone {} door command {} room {} dir {}", 
                         name(), cmd.command_type, cmd.room_id, cmd.entity_id);
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

std::string ZoneCommandParser::format_command(const ZoneCommand& cmd) {
    char cmd_char = ZoneUtils::get_char_from_command_type(cmd.command_type);
    
    return fmt::format("{} {} {} {} {} {} ; {}",
                      cmd_char,
                      cmd.if_flag,
                      cmd.entity_id.is_valid() ? cmd.entity_id.value() : 0,
                      cmd.room_id.is_valid() ? cmd.room_id.value() : 0,
                      cmd.container_id.is_valid() ? cmd.container_id.value() : 0,
                      cmd.max_count,
                      cmd.comment);
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
            {'M', ZoneCommandType::Load_Mobile},
            {'O', ZoneCommandType::Load_Object},
            {'E', ZoneCommandType::Equip_Object},
            {'P', ZoneCommandType::Put_Object},
            {'G', ZoneCommandType::Give_Object},
            {'D', ZoneCommandType::Open_Door},
            {'L', ZoneCommandType::Close_Door},
            {'R', ZoneCommandType::Remove_Object},
            {'T', ZoneCommandType::Trigger},
            {'F', ZoneCommandType::Follow_Mobile},
            {'W', ZoneCommandType::Wait},
            {'*', ZoneCommandType::Comment}
        };
        
        auto it = char_map.find(cmd_char);
        return it != char_map.end() ? std::optional<ZoneCommandType>{it->second} : std::nullopt;
    }
    
    char get_char_from_command_type(ZoneCommandType type) {
        static const std::unordered_map<ZoneCommandType, char> type_map = {
            {ZoneCommandType::Load_Mobile, 'M'},
            {ZoneCommandType::Load_Object, 'O'},
            {ZoneCommandType::Equip_Object, 'E'},
            {ZoneCommandType::Put_Object, 'P'},
            {ZoneCommandType::Give_Object, 'G'},
            {ZoneCommandType::Open_Door, 'D'},
            {ZoneCommandType::Close_Door, 'L'},
            {ZoneCommandType::Remove_Object, 'R'},
            {ZoneCommandType::Trigger, 'T'},
            {ZoneCommandType::Follow_Mobile, 'F'},
            {ZoneCommandType::Wait, 'W'},
            {ZoneCommandType::Comment, '*'}
        };
        
        auto it = type_map.find(type);
        return it != type_map.end() ? it->second : '?';
    }
    
    bool zone_file_exists(const std::string& filename) {
        std::ifstream file(filename);
        return file.good();
    }
    
    std::string get_zone_file_path(int zone_number) {
        return fmt::format("lib/world/zon/{}.zon", zone_number);
    }
    
    std::optional<int> extract_zone_number(const std::string& filename) {
        std::regex zone_regex(R"((\d+)\.zon$)");
        std::smatch match;
        
        if (std::regex_search(filename, match, zone_regex)) {
            return std::stoi(match[1].str());
        }
        
        return std::nullopt;
    }
    
    Result<void> validate_zone_file(const std::string& filename) {
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
            
        } catch (const nlohmann::json::exception& e) {
            return std::unexpected(Errors::ParseError(fmt::format("Invalid JSON in {}", filename), e.what()));
        }
    }
}

// Zone command implementations

Result<bool> Zone::execute_load_mobile(const ZoneCommand& cmd) {
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
            logger->error("Zone {} cannot load mobile {} - room {} not found", 
                         name(), mobile_id, room_id);
            return true; // Continue processing
        }
    }
    
    // Spawn the mobile
    auto mobile = spawn_mobile_callback_(mobile_id, room_id);
    if (mobile) {
        logger->info("Zone {} spawned mobile '{}' ({}) in room {}", 
                    name(), mobile->name(), mobile_id, room_id);
        stats_.mobile_count++; // Track spawning stats
    } else {
        logger->warn("Zone {} failed to spawn mobile {} in room {}", 
                    name(), mobile_id, room_id);
    }
    
    return true; // Continue processing
}

Result<bool> Zone::execute_load_object(const ZoneCommand& cmd) {
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
            logger->error("Zone {} cannot load object {} - room {} not found", 
                         name(), object_id, room_id);
            return true; // Continue processing
        }
    }
    
    // Spawn the object
    auto object = spawn_object_callback_(object_id, room_id);
    if (object) {
        logger->info("Zone {} spawned object '{}' ({}) in room {}", 
                    name(), object->name(), object_id, room_id);
        stats_.object_count++; // Track spawning stats
    } else {
        logger->warn("Zone {} failed to spawn object {} in room {}", 
                    name(), object_id, room_id);
    }
    
    return true; // Continue processing
}