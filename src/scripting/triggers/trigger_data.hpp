#pragma once

#include "trigger_types.hpp"
#include "core/ids.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace FieryMUD {

/// Trigger data structure - mirrors PostgreSQL Triggers table
struct TriggerData {
    /// Database primary key
    std::int32_t id{0};

    /// Trigger name for identification/debugging
    std::string name;

    /// Type of entity this trigger is attached to
    ScriptType attach_type{ScriptType::MOB};

    /// Combined trigger flags (bitfield)
    TriggerFlag flags{};

    /// The Lua script code to execute
    std::string commands;

    /// Number of expected arguments
    std::int32_t num_args{0};

    /// Argument names/descriptions
    std::vector<std::string> arg_list;

    /// Script-local variables (loaded from JSON)
    nlohmann::json variables;

    /// Numeric argument for certain trigger types (e.g., HIT_PERCENT threshold)
    std::int32_t numeric_arg{0};

    // Attachment references (only one set per attach_type)

    /// Zone ID for WORLD triggers
    std::optional<std::int32_t> zone_id;

    /// Mob attachment (zone_id, local_id)
    std::optional<EntityId> mob_id;

    /// Object attachment (zone_id, local_id)
    std::optional<EntityId> object_id;

    /// Check if trigger has a specific flag
    [[nodiscard]] constexpr bool has_flag(TriggerFlag check) const {
        return FieryMUD::has_flag(flags, check);
    }

    /// Get the entity ID this trigger is attached to
    [[nodiscard]] std::optional<EntityId> attached_entity_id() const {
        switch (attach_type) {
            case ScriptType::MOB:
                return mob_id;
            case ScriptType::OBJECT:
                return object_id;
            case ScriptType::WORLD:
                if (zone_id.has_value()) {
                    // World triggers use zone_id as the entity
                    return EntityId{static_cast<std::uint32_t>(*zone_id), 0};
                }
                return std::nullopt;
        }
        return std::nullopt;
    }

    /// Parse trigger flags from array of strings
    static TriggerFlag parse_flags(const std::vector<std::string>& flag_strings) {
        TriggerFlag result{};
        for (const auto& str : flag_strings) {
            if (auto flag = string_to_trigger_flag(str)) {
                result |= *flag;
            }
        }
        return result;
    }

    /// Generate a unique cache key for bytecode caching
    [[nodiscard]] std::string cache_key() const {
        // Format: "trigger:<id>:<name_hash>"
        // Using ID + name hash to handle script updates
        std::size_t name_hash = std::hash<std::string>{}(name + commands);
        return fmt::format("trigger:{}:{:x}", id, name_hash);
    }

    /// Get human-readable string of all active flags
    /// Note: Some flags share bit positions (ATTACK/RESET, DEFEND/PREENTRY, TIMER/POSTENTRY)
    /// so we filter based on attach_type to show the appropriate flag names
    [[nodiscard]] std::string flags_string() const {
        std::ostringstream oss;
        bool first = true;

        // Common flags (all types)
        constexpr TriggerFlag common_flags[] = {
            TriggerFlag::GLOBAL, TriggerFlag::RANDOM, TriggerFlag::COMMAND,
            TriggerFlag::LOAD, TriggerFlag::CAST, TriggerFlag::LEAVE,
            TriggerFlag::TIME
        };

        // MOB-specific flags
        constexpr TriggerFlag mob_flags[] = {
            TriggerFlag::SPEECH, TriggerFlag::ACT, TriggerFlag::DEATH,
            TriggerFlag::GREET, TriggerFlag::GREET_ALL, TriggerFlag::ENTRY,
            TriggerFlag::RECEIVE, TriggerFlag::FIGHT, TriggerFlag::HIT_PERCENT,
            TriggerFlag::BRIBE, TriggerFlag::MEMORY, TriggerFlag::DOOR,
            TriggerFlag::SPEECH_TO, TriggerFlag::LOOK, TriggerFlag::AUTO
        };

        // OBJECT-specific flags (some share bits with WORLD flags)
        constexpr TriggerFlag object_flags[] = {
            TriggerFlag::ATTACK, TriggerFlag::DEFEND, TriggerFlag::TIMER,
            TriggerFlag::GET, TriggerFlag::DROP, TriggerFlag::GIVE,
            TriggerFlag::WEAR, TriggerFlag::REMOVE, TriggerFlag::USE,
            TriggerFlag::CONSUME
        };

        // WORLD-specific flags (some share bits with OBJECT flags)
        constexpr TriggerFlag world_flags[] = {
            TriggerFlag::RESET, TriggerFlag::PREENTRY, TriggerFlag::POSTENTRY
        };

        auto append_flag = [&](TriggerFlag flag) {
            if (has_flag(flag)) {
                if (!first) oss << " ";
                oss << trigger_flag_to_string(flag);
                first = false;
            }
        };

        // Always check common flags
        for (auto flag : common_flags) {
            append_flag(flag);
        }

        // Check type-specific flags based on attach_type
        switch (attach_type) {
            case ScriptType::MOB:
                for (auto flag : mob_flags) {
                    append_flag(flag);
                }
                break;
            case ScriptType::OBJECT:
                for (auto flag : object_flags) {
                    append_flag(flag);
                }
                break;
            case ScriptType::WORLD:
                for (auto flag : world_flags) {
                    append_flag(flag);
                }
                break;
        }

        return oss.str();
    }
};

/// Shared pointer type for trigger data
using TriggerDataPtr = std::shared_ptr<TriggerData>;

/// Collection of triggers attached to an entity
struct TriggerSet {
    std::vector<TriggerDataPtr> triggers;

    /// Find all triggers matching a specific flag
    [[nodiscard]] std::vector<TriggerDataPtr> find_by_flag(TriggerFlag flag) const {
        std::vector<TriggerDataPtr> result;
        for (const auto& trigger : triggers) {
            if (trigger->has_flag(flag)) {
                result.push_back(trigger);
            }
        }
        return result;
    }

    /// Add a trigger to the set
    void add(TriggerDataPtr trigger) {
        triggers.push_back(std::move(trigger));
    }

    /// Remove all triggers
    void clear() {
        triggers.clear();
    }

    /// Check if set is empty
    [[nodiscard]] bool empty() const {
        return triggers.empty();
    }

    /// Get number of triggers
    [[nodiscard]] std::size_t size() const {
        return triggers.size();
    }

    // Iterator support for range-based for loops
    using iterator = std::vector<TriggerDataPtr>::iterator;
    using const_iterator = std::vector<TriggerDataPtr>::const_iterator;

    [[nodiscard]] iterator begin() { return triggers.begin(); }
    [[nodiscard]] iterator end() { return triggers.end(); }
    [[nodiscard]] const_iterator begin() const { return triggers.begin(); }
    [[nodiscard]] const_iterator end() const { return triggers.end(); }
    [[nodiscard]] const_iterator cbegin() const { return triggers.cbegin(); }
    [[nodiscard]] const_iterator cend() const { return triggers.cend(); }
};

} // namespace FieryMUD
