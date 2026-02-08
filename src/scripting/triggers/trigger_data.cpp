#include "trigger_data.hpp"

#include "trigger_types.hpp"

namespace FieryMUD {

bool TriggerData::has_flag(TriggerFlag check) const { return FieryMUD::has_flag(flags, check); }

std::optional<EntityId> TriggerData::attached_entity_id() const {
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

TriggerFlag TriggerData::parse_flags(const std::vector<std::string> &flag_strings) {
    TriggerFlag result{};
    for (const auto &str : flag_strings) {
        if (auto flag = string_to_trigger_flag(str)) {
            result |= *flag;
        }
    }
    return result;
}

std::string TriggerData::cache_key() const {
    // Format: "trigger:<id>:<name_hash>"
    // Using ID + name hash to handle script updates
    std::size_t name_hash = std::hash<std::string>{}(name + commands);
    return fmt::format("trigger:{}:{:x}", id, name_hash);
}

std::string TriggerData::flags_string() const {
    std::ostringstream oss;
    bool first = true;

    // Common flags (all types)
    constexpr TriggerFlag common_flags[] = {TriggerFlag::GLOBAL, TriggerFlag::RANDOM, TriggerFlag::COMMAND,
                                            TriggerFlag::LOAD,   TriggerFlag::CAST,   TriggerFlag::LEAVE,
                                            TriggerFlag::TIME};

    // MOB-specific flags
    constexpr TriggerFlag mob_flags[] = {TriggerFlag::SPEECH,    TriggerFlag::ACT,       TriggerFlag::DEATH,
                                         TriggerFlag::GREET,     TriggerFlag::GREET_ALL, TriggerFlag::ENTRY,
                                         TriggerFlag::RECEIVE,   TriggerFlag::FIGHT,     TriggerFlag::HIT_PERCENT,
                                         TriggerFlag::BRIBE,     TriggerFlag::MEMORY,    TriggerFlag::DOOR,
                                         TriggerFlag::SPEECH_TO, TriggerFlag::LOOK,      TriggerFlag::AUTO};

    // OBJECT-specific flags (some share bits with WORLD flags)
    constexpr TriggerFlag object_flags[] = {
        TriggerFlag::ATTACK, TriggerFlag::DEFEND, TriggerFlag::TIMER,  TriggerFlag::GET, TriggerFlag::DROP,
        TriggerFlag::GIVE,   TriggerFlag::WEAR,   TriggerFlag::REMOVE, TriggerFlag::USE, TriggerFlag::CONSUME};

    // WORLD-specific flags (some share bits with OBJECT flags)
    constexpr TriggerFlag world_flags[] = {TriggerFlag::RESET, TriggerFlag::PREENTRY, TriggerFlag::POSTENTRY};

    auto append_flag = [&](TriggerFlag flag) {
        if (has_flag(flag)) {
            if (!first)
                oss << " ";
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

} // namespace FieryMUD
