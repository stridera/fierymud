#include "game_data_cache.hpp"
#include "connection_pool.hpp"
#include "world_queries.hpp"
#include "../core/logging.hpp"

#include <algorithm>
#include <cctype>

// =============================================================================
// ClassData Implementation
// =============================================================================

std::string ClassData::lookup_key() const {
    std::string key;
    key.reserve(plain_name.size());
    for (char c : plain_name) {
        key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return key;
}

// =============================================================================
// RaceData Implementation
// =============================================================================

std::string RaceData::lookup_key() const {
    std::string key;
    key.reserve(plain_name.size());
    for (char c : plain_name) {
        if (c == '-' || c == '_' || c == ' ') {
            continue;  // Skip separators for flexible matching
        }
        key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return key;
}

// =============================================================================
// LiquidData Implementation
// =============================================================================

std::string LiquidData::lookup_key() const {
    std::string key;
    key.reserve(alias.size());
    for (char c : alias) {
        if (c == '-' || c == '_' || c == ' ') {
            continue;  // Skip separators for flexible matching
        }
        key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return key;
}

// =============================================================================
// GameDataCache Implementation
// =============================================================================

GameDataCache& GameDataCache::instance() {
    static GameDataCache instance;
    return instance;
}

std::string GameDataCache::normalize_key(std::string_view input) {
    std::string key;
    key.reserve(input.size());
    for (char c : input) {
        if (c == '-' || c == '_' || c == ' ') {
            continue;  // Skip separators
        }
        key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return key;
}

Result<void> GameDataCache::load_all() {
    auto class_result = load_classes();
    if (!class_result) {
        return class_result;
    }

    auto race_result = load_races();
    if (!race_result) {
        return race_result;
    }

    auto liquid_result = load_liquids();
    if (!liquid_result) {
        return liquid_result;
    }

    loaded_ = true;
    last_load_time_ = std::chrono::system_clock::now();

    Log::info("GameDataCache loaded: {} classes, {} races, {} liquids",
              classes_.size(), races_.size(), liquids_.size());

    return Success();
}

Result<void> GameDataCache::load_classes() {
    auto result = ConnectionPool::instance().execute([this](pqxx::work& txn) -> Result<void> {
        try {
            auto query_result = txn.exec(R"(
                SELECT id, name, plain_name, description, hit_dice, primary_stat,
                       bonus_hitroll, bonus_damroll, base_ac, hp_per_level,
                       thac0_base, thac0_per_level
                FROM "Class"
                ORDER BY id
            )");

            std::lock_guard<std::mutex> lock(mutex_);

            classes_.clear();
            class_by_id_.clear();
            class_by_name_.clear();

            for (const auto& row : query_result) {
                ClassData data;
                data.id = row["id"].as<int>();
                data.name = row["name"].as<std::string>();
                data.plain_name = row["plain_name"].as<std::string>();

                if (!row["description"].is_null()) {
                    data.description = row["description"].as<std::string>();
                }
                if (!row["hit_dice"].is_null()) {
                    data.hit_dice = row["hit_dice"].as<std::string>();
                }
                if (!row["primary_stat"].is_null()) {
                    data.primary_stat = row["primary_stat"].as<std::string>();
                }

                // Load combat modifiers from database (data-driven)
                data.bonus_hitroll = row["bonus_hitroll"].as<int>();
                data.bonus_damroll = row["bonus_damroll"].as<int>();
                data.base_ac = row["base_ac"].as<int>();
                data.hp_per_level = row["hp_per_level"].as<int>();
                data.thac0_base = row["thac0_base"].as<int>();
                data.thac0_per_level = row["thac0_per_level"].as<double>();

                size_t index = classes_.size();
                classes_.push_back(std::move(data));

                // Build lookup maps
                class_by_id_[classes_[index].id] = index;
                class_by_name_[classes_[index].lookup_key()] = index;

                // Also add common aliases
                if (classes_[index].lookup_key() == "thief") {
                    class_by_name_["rogue"] = index;
                }
            }

            return Success();

        } catch (const std::exception& e) {
            return std::unexpected(Errors::DatabaseError(
                fmt::format("Failed to load classes: {}", e.what())));
        }
    });

    if (!result) {
        Log::error("Failed to load class data: {}", result.error().message);
        return std::unexpected(result.error());
    }

    return Success();
}

Result<void> GameDataCache::load_races() {
    auto result = ConnectionPool::instance().execute([this](pqxx::work& txn) -> Result<void> {
        try {
            auto query_result = txn.exec(R"(
                SELECT race, name, plain_name, playable, humanoid,
                       bonus_hitroll, bonus_damroll, default_size,
                       max_strength, max_dexterity, max_intelligence,
                       max_wisdom, max_constitution, max_charisma,
                       exp_factor, hp_factor, hit_damage_factor, ac_factor,
                       start_room_zone_id, start_room_id
                FROM "Races"
                ORDER BY race
            )");

            std::lock_guard<std::mutex> lock(mutex_);

            races_.clear();
            race_by_key_.clear();
            race_by_name_.clear();

            for (const auto& row : query_result) {
                RaceData data;
                data.race_key = row["race"].as<std::string>();
                data.name = row["name"].as<std::string>();
                data.plain_name = row["plain_name"].as<std::string>();
                data.playable = row["playable"].as<bool>();
                data.humanoid = row["humanoid"].as<bool>();
                data.bonus_hitroll = row["bonus_hitroll"].as<int>();
                data.bonus_damroll = row["bonus_damroll"].as<int>();

                // Parse size
                std::string size_str = row["default_size"].as<std::string>();
                if (size_str == "TINY") {
                    data.size = CharacterSize::Tiny;
                } else if (size_str == "SMALL") {
                    data.size = CharacterSize::Small;
                } else if (size_str == "LARGE") {
                    data.size = CharacterSize::Large;
                } else if (size_str == "HUGE") {
                    data.size = CharacterSize::Huge;
                } else {
                    data.size = CharacterSize::Medium;
                }

                // Stat caps
                data.max_strength = row["max_strength"].as<int>();
                data.max_dexterity = row["max_dexterity"].as<int>();
                data.max_intelligence = row["max_intelligence"].as<int>();
                data.max_wisdom = row["max_wisdom"].as<int>();
                data.max_constitution = row["max_constitution"].as<int>();
                data.max_charisma = row["max_charisma"].as<int>();

                // Scaling factors
                data.exp_factor = row["exp_factor"].as<int>();
                data.hp_factor = row["hp_factor"].as<int>();
                data.damage_factor = row["hit_damage_factor"].as<int>();
                data.ac_factor = row["ac_factor"].as<int>();

                // Load optional start room
                if (!row["start_room_zone_id"].is_null() && !row["start_room_id"].is_null()) {
                    data.start_room_zone_id = row["start_room_zone_id"].as<int>();
                    data.start_room_id = row["start_room_id"].as<int>();
                }

                size_t index = races_.size();
                races_.push_back(std::move(data));

                // Build lookup maps
                race_by_key_[normalize_key(races_[index].race_key)] = index;
                race_by_name_[races_[index].lookup_key()] = index;

                // Add common aliases
                std::string key = races_[index].lookup_key();
                if (key == "halfelf") {
                    race_by_name_["half-elf"] = index;
                    race_by_name_["half_elf"] = index;
                }
            }

            return Success();

        } catch (const std::exception& e) {
            return std::unexpected(Errors::DatabaseError(
                fmt::format("Failed to load races: {}", e.what())));
        }
    });

    if (!result) {
        Log::error("Failed to load race data: {}", result.error().message);
        return std::unexpected(result.error());
    }

    return Success();
}

Result<void> GameDataCache::load_liquids() {
    auto result = ConnectionPool::instance().execute([this](pqxx::work& txn) -> Result<void> {
        try {
            auto query_result = txn.exec(R"(
                SELECT id, name, alias, color_desc,
                       drunk_effect, hunger_effect, thirst_effect
                FROM "Liquids"
                ORDER BY id
            )");

            std::lock_guard<std::mutex> lock(mutex_);

            liquids_.clear();
            liquid_by_name_.clear();
            liquid_by_alias_.clear();

            for (const auto& row : query_result) {
                LiquidData data;
                data.id = row["id"].as<int>();
                data.name = row["name"].as<std::string>();
                data.alias = row["alias"].as<std::string>();
                data.color_desc = row["color_desc"].as<std::string>();
                data.drunk_effect = row["drunk_effect"].as<int>();
                data.hunger_effect = row["hunger_effect"].as<int>();
                data.thirst_effect = row["thirst_effect"].as<int>();

                size_t index = liquids_.size();
                liquids_.push_back(std::move(data));

                // Build lookup maps (case-insensitive)
                liquid_by_name_[normalize_key(liquids_[index].name)] = index;
                liquid_by_alias_[normalize_key(liquids_[index].alias)] = index;
            }

            return Success();

        } catch (const std::exception& e) {
            return std::unexpected(Errors::DatabaseError(
                fmt::format("Failed to load liquids: {}", e.what())));
        }
    });

    if (!result) {
        Log::error("Failed to load liquid data: {}", result.error().message);
        return std::unexpected(result.error());
    }

    return Success();
}

Result<void> GameDataCache::reload() {
    loaded_ = false;
    return load_all();
}

const ClassData* GameDataCache::find_class_by_id(int id) const {
    auto it = class_by_id_.find(id);
    if (it != class_by_id_.end() && it->second < classes_.size()) {
        return &classes_[it->second];
    }
    return nullptr;
}

const ClassData* GameDataCache::find_class_by_name(std::string_view name) const {
    std::string key = normalize_key(name);
    auto it = class_by_name_.find(key);
    if (it != class_by_name_.end() && it->second < classes_.size()) {
        return &classes_[it->second];
    }
    return nullptr;
}

const RaceData* GameDataCache::find_race_by_key(std::string_view key) const {
    std::string normalized = normalize_key(key);
    auto it = race_by_key_.find(normalized);
    if (it != race_by_key_.end() && it->second < races_.size()) {
        return &races_[it->second];
    }
    return nullptr;
}

const RaceData* GameDataCache::find_race_by_name(std::string_view name) const {
    std::string key = normalize_key(name);
    auto it = race_by_name_.find(key);
    if (it != race_by_name_.end() && it->second < races_.size()) {
        return &races_[it->second];
    }
    return nullptr;
}

std::vector<const RaceData*> GameDataCache::playable_races() const {
    std::vector<const RaceData*> result;
    for (const auto& race : races_) {
        if (race.playable) {
            result.push_back(&race);
        }
    }
    return result;
}

const LiquidData* GameDataCache::find_liquid_by_name(std::string_view name) const {
    std::string key = normalize_key(name);
    auto it = liquid_by_name_.find(key);
    if (it != liquid_by_name_.end() && it->second < liquids_.size()) {
        return &liquids_[it->second];
    }
    return nullptr;
}

const LiquidData* GameDataCache::find_liquid_by_alias(std::string_view alias) const {
    std::string key = normalize_key(alias);
    auto it = liquid_by_alias_.find(key);
    if (it != liquid_by_alias_.end() && it->second < liquids_.size()) {
        return &liquids_[it->second];
    }
    return nullptr;
}
