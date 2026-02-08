#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "core/ids.hpp"
#include "trigger_enums.hpp"

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
    [[nodiscard]] bool has_flag(TriggerFlag check) const;

    /// Get the entity ID this trigger is attached to
    [[nodiscard]] std::optional<EntityId> attached_entity_id() const;

    /// Parse trigger flags from array of strings
    static TriggerFlag parse_flags(const std::vector<std::string> &flag_strings);

    /// Generate a unique cache key for bytecode caching
    [[nodiscard]] std::string cache_key() const;

    /// Get human-readable string of all active flags
    /// Note: Some flags share bit positions (ATTACK/RESET, DEFEND/PREENTRY, TIMER/POSTENTRY)
    /// so we filter based on attach_type to show the appropriate flag names
    [[nodiscard]] std::string flags_string() const;
};

/// Shared pointer type for trigger data
using TriggerDataPtr = std::shared_ptr<TriggerData>;

/// Collection of triggers attached to an entity
struct TriggerSet {
    std::vector<TriggerDataPtr> triggers;

    /// Find all triggers matching a specific flag
    [[nodiscard]] std::vector<TriggerDataPtr> find_by_flag(TriggerFlag flag) const {
        std::vector<TriggerDataPtr> result;
        for (const auto &trigger : triggers) {
            if (trigger->has_flag(flag)) {
                result.push_back(trigger);
            }
        }
        return result;
    }

    /// Add a trigger to the set
    void add(TriggerDataPtr trigger) { triggers.push_back(std::move(trigger)); }

    /// Remove all triggers
    void clear() { triggers.clear(); }

    /// Check if set is empty
    [[nodiscard]] bool empty() const { return triggers.empty(); }

    /// Get number of triggers
    [[nodiscard]] std::size_t size() const { return triggers.size(); }

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
