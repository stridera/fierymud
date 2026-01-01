#include "spell_system.hpp"
#include "actor.hpp"
#include "class_config.hpp"
#include "commands/command_system.hpp"
#include "core/logging.hpp"
#include "text/string_utils.hpp"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <magic_enum/magic_enum.hpp>

using fierymud::ClassConfigRegistry;
using fierymud::ClassSpellConfig;
using fierymud::SpellProgression;

// ============================================================================
// Spell Implementation
// ============================================================================

bool Spell::can_cast(const Actor& caster) const {
    // Gods can cast any spell
    if (const auto* player = dynamic_cast<const Player*>(&caster)) {
        if (player->is_god()) {
            return true;
        }
    }

    // Check if caster has required level for this circle
    if (caster.stats().level < circle) {
        return false;
    }

    // Check if caster has spell slots for this circle
    // This is data-driven: warriors can cast if they have racial spell slots,
    // casters can cast their class spells, etc.
    if (!caster.has_spell_slots(circle)) {
        return false;
    }

    return true;
}

Result<void> Spell::cast(Actor& caster, const CommandContext& ctx) const {
    // Basic spell casting implementation
    Log::info("SPELL: {} casts '{}' (Circle {})", caster.name(), name, circle);

    // Send messages to all participants
    std::string cast_message = fmt::format("You cast '{}'!", name);
    std::string room_message = fmt::format("{} casts '{}'!", caster.display_name(), name);

    ctx.send(cast_message);
    ctx.send_to_room(room_message, true);

    // Basic spell effects based on type
    switch (type) {
        case SpellType::Healing: {
            int heal_amount = circle * 10 + 5; // Simple healing formula
            auto& stats = caster.stats();
            int old_hp = stats.hit_points;
            stats.hit_points = std::min(stats.max_hit_points, stats.hit_points + heal_amount);
            int actual_heal = stats.hit_points - old_hp;

            if (actual_heal > 0) {
                ctx.send(fmt::format("You are healed for {} hit points!", actual_heal));
                ctx.send_to_room(fmt::format("{} glows with healing energy!", caster.display_name()), true);

                // Send GMCP vitals update if caster is a player
                if (auto* player = dynamic_cast<Player*>(&caster)) {
                    player->send_gmcp_vitals_update();
                }
            } else {
                ctx.send("You are already at full health.");
            }
            break;
        }
        
        case SpellType::Offensive: {
            // For now, just show message - full combat integration would be more complex
            int damage = circle * 8 + 2; // Simple damage formula
            ctx.send(fmt::format("Your spell would deal {} damage to enemies!", damage));
            ctx.send_to_room(fmt::format("{}'s spell crackles with destructive energy!", caster.display_name()), true);
            break;
        }
        
        case SpellType::Utility: {
            ctx.send("Your utility spell takes effect!");
            ctx.send_to_room(fmt::format("{}'s spell shimmers with magical energy!", caster.display_name()), true);
            break;
        }
        
        default: {
            ctx.send("Your spell takes effect!");
            ctx.send_to_room(fmt::format("{}'s spell radiates magical energy!", caster.display_name()), true);
            break;
        }
    }
    
    return Success();
}

nlohmann::json Spell::to_json() const {
    return {
        {"name", name},
        {"description", description},
        {"type", std::string{magic_enum::enum_name(type)}},
        {"circle", circle},
        {"cast_time_seconds", cast_time_seconds},
        {"range_meters", range_meters},
        {"duration_seconds", duration_seconds}
    };
}

Result<Spell> Spell::from_json(const nlohmann::json& json) {
    try {
        Spell spell;
        spell.name = json.at("name").get<std::string>();
        spell.description = json.at("description").get<std::string>();
        spell.circle = json.at("circle").get<int>();
        
        if (json.contains("type")) {
            if (auto type_opt = magic_enum::enum_cast<SpellType>(json["type"].get<std::string>())) {
                spell.type = type_opt.value();
            }
        }
        
        if (json.contains("cast_time_seconds")) {
            spell.cast_time_seconds = json["cast_time_seconds"].get<int>();
        }
        
        if (json.contains("range_meters")) {
            spell.range_meters = json["range_meters"].get<int>();
        }
        
        if (json.contains("duration_seconds")) {
            spell.duration_seconds = json["duration_seconds"].get<int>();
        }
        
        return spell;
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Spell JSON parsing error", e.what()));
    }
}

// ============================================================================
// SpellSlotCircle Implementation
// ============================================================================

nlohmann::json SpellSlotCircle::to_json() const {
    return {
        {"current_slots", current_slots},
        {"max_slots", max_slots}
    };
}

Result<SpellSlotCircle> SpellSlotCircle::from_json(const nlohmann::json& json) {
    try {
        SpellSlotCircle circle;
        circle.current_slots = json.at("current_slots").get<int>();
        circle.max_slots = json.at("max_slots").get<int>();
        return circle;
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("SpellSlotCircle JSON parsing error", e.what()));
    }
}

// ============================================================================
// SpellSlotRestoring Implementation
// ============================================================================

nlohmann::json SpellSlotRestoring::to_json() const {
    return {
        {"circle", circle},
        {"ticks_remaining", ticks_remaining}
    };
}

Result<SpellSlotRestoring> SpellSlotRestoring::from_json(const nlohmann::json& json) {
    try {
        SpellSlotRestoring restoring;
        restoring.circle = json.at("circle").get<int>();
        restoring.ticks_remaining = json.at("ticks_remaining").get<int>();
        return restoring;
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("SpellSlotRestoring JSON parsing error", e.what()));
    }
}

// ============================================================================
// SpellSlots Implementation
// ============================================================================

void SpellSlots::initialize_for_class(std::string_view character_class, int level) {
    slots_.clear();
    restoration_queue_.clear();

    // Get class configuration from registry
    const auto& registry = ClassConfigRegistry::instance();
    const auto* config = registry.get_config(character_class);

    if (!config || !config->is_caster()) {
        // Non-caster class - no spell slots
        return;
    }

    // Initialize slots based on class configuration
    initialize_slots_from_config(*config, level);
}

void SpellSlots::initialize_slots_from_config(const ClassSpellConfig& config, int level) {
    for (const auto& circle_access : config.circles) {
        // Check if character level meets the minimum requirement
        if (level < circle_access.min_level) {
            continue;
        }

        // Calculate slots based on progression type and level
        int slots_for_circle = calculate_slots(config.progression, level,
                                                circle_access.min_level,
                                                circle_access.max_slots);

        if (slots_for_circle > 0) {
            SpellSlotCircle slot_circle;
            slot_circle.max_slots = slots_for_circle;
            slot_circle.current_slots = slots_for_circle; // Start with full slots
            slots_[circle_access.circle] = slot_circle;
        }
    }
}

int SpellSlots::calculate_slots(SpellProgression progression, int level,
                                 int required_level, int max_slots) {
    if (level < required_level) {
        return 0;
    }

    int levels_above_requirement = level - required_level;
    int slots = 0;

    switch (progression) {
        case SpellProgression::Full:
            // Full casters: 1 slot at access, +1 every 4 levels, capped at max_slots
            slots = 1 + levels_above_requirement / 4;
            break;

        case SpellProgression::Half:
            // Half casters: 1 slot at access, +1 every 6 levels, capped at max_slots
            slots = 1 + levels_above_requirement / 6;
            break;

        case SpellProgression::Third:
            // Third casters: 1 slot at access, +1 every 8 levels, capped at max_slots
            slots = 1 + levels_above_requirement / 8;
            break;

        case SpellProgression::None:
            return 0;
    }

    return std::min(slots, max_slots);
}

bool SpellSlots::has_slots(int circle) const {
    auto it = slots_.find(circle);
    return it != slots_.end() && it->second.has_slots();
}

bool SpellSlots::has_any_slots() const {
    for (const auto& [circle, slot_circle] : slots_) {
        if (slot_circle.max_slots > 0) {
            return true;
        }
    }
    return false;
}

bool SpellSlots::consume_slot(int spell_circle) {
    // Try exact circle first
    if (has_slots(spell_circle)) {
        slots_[spell_circle].current_slots--;
        restoration_queue_.push_back({spell_circle, get_base_ticks(spell_circle)});
        Log::debug("Consumed circle {} slot, {} remaining, queue size {}",
                   spell_circle, slots_[spell_circle].current_slots, restoration_queue_.size());
        return true;
    }

    // Try higher circles (upcast) - use a higher slot for a lower circle spell
    for (int c = spell_circle + 1; c <= 9; ++c) {
        if (has_slots(c)) {
            slots_[c].current_slots--;
            restoration_queue_.push_back({c, get_base_ticks(c)});
            Log::debug("Upcast: consumed circle {} slot for circle {} spell, queue size {}",
                       c, spell_circle, restoration_queue_.size());
            return true;
        }
    }

    return false; // No slots available
}

int SpellSlots::restore_tick(int focus_rate) {
    if (restoration_queue_.empty()) {
        return 0;
    }

    // Subtract from front of queue (FIFO)
    restoration_queue_.front().ticks_remaining -= focus_rate;

    // Restore slot if ready
    if (restoration_queue_.front().ticks_remaining <= 0) {
        int circle = restoration_queue_.front().circle;
        slots_[circle].current_slots++;
        restoration_queue_.erase(restoration_queue_.begin());
        Log::debug("Restored circle {} slot, now {}/{}, queue size {}",
                   circle, slots_[circle].current_slots, slots_[circle].max_slots,
                   restoration_queue_.size());
        return circle; // Return the circle that was restored
    }

    return 0; // Nothing restored yet
}

std::pair<int, int> SpellSlots::get_slot_info(int circle) const {
    auto it = slots_.find(circle);
    if (it == slots_.end()) {
        return {0, 0};
    }

    return {it->second.current_slots, it->second.max_slots};
}

int SpellSlots::get_restoring_count(int circle) const {
    int count = 0;
    for (const auto& entry : restoration_queue_) {
        if (entry.circle == circle) {
            count++;
        }
    }
    return count;
}

int SpellSlots::get_total_restoring() const {
    return static_cast<int>(restoration_queue_.size());
}

std::vector<int> SpellSlots::get_available_circles() const {
    std::vector<int> circles;
    for (const auto& [circle, slot_circle] : slots_) {
        if (slot_circle.max_slots > 0) {
            circles.push_back(circle);
        }
    }
    std::sort(circles.begin(), circles.end());
    return circles;
}

int SpellSlots::get_base_ticks(int circle) {
    if (circle < 1 || circle > 9) {
        return 30; // Default to circle 1 time for invalid circles
    }
    return CIRCLE_RECOVERY_TICKS[circle];
}

nlohmann::json SpellSlots::to_json() const {
    nlohmann::json json = nlohmann::json::object();

    // Serialize slot circles
    nlohmann::json circles_json = nlohmann::json::object();
    for (const auto& [circle, slot_circle] : slots_) {
        circles_json[std::to_string(circle)] = slot_circle.to_json();
    }
    json["circles"] = circles_json;

    // Serialize restoration queue
    nlohmann::json queue_json = nlohmann::json::array();
    for (const auto& entry : restoration_queue_) {
        queue_json.push_back(entry.to_json());
    }
    json["restoration_queue"] = queue_json;

    return json;
}

Result<SpellSlots> SpellSlots::from_json(const nlohmann::json& json) {
    try {
        SpellSlots spell_slots;

        // Check for new format with "circles" key
        if (json.contains("circles")) {
            for (const auto& [circle_str, circle_json] : json["circles"].items()) {
                int circle = std::stoi(circle_str);
                auto circle_result = SpellSlotCircle::from_json(circle_json);
                if (!circle_result) {
                    return std::unexpected(circle_result.error());
                }
                spell_slots.slots_[circle] = circle_result.value();
            }

            // Load restoration queue if present
            if (json.contains("restoration_queue")) {
                for (const auto& entry_json : json["restoration_queue"]) {
                    auto entry_result = SpellSlotRestoring::from_json(entry_json);
                    if (!entry_result) {
                        return std::unexpected(entry_result.error());
                    }
                    spell_slots.restoration_queue_.push_back(entry_result.value());
                }
            }
        } else {
            // Legacy format: circles are at top level
            for (const auto& [circle_str, circle_json] : json.items()) {
                int circle = std::stoi(circle_str);
                auto circle_result = SpellSlotCircle::from_json(circle_json);
                if (!circle_result) {
                    return std::unexpected(circle_result.error());
                }
                spell_slots.slots_[circle] = circle_result.value();
            }
        }

        return spell_slots;

    } catch (const std::exception& e) {
        return std::unexpected(Errors::ParseError("SpellSlots JSON parsing error", e.what()));
    }
}

// ============================================================================
// SpellRegistry Implementation
// ============================================================================

SpellRegistry& SpellRegistry::instance() {
    static SpellRegistry instance;
    return instance;
}

void SpellRegistry::register_spell(const Spell& spell) {
    spells_[spell.name] = spell;
    Log::debug("Registered spell: '{}' (Circle {})", spell.name, spell.circle);
}

const Spell* SpellRegistry::find_spell(std::string_view name) const {
    // Try exact match first
    auto it = spells_.find(std::string{name});
    if (it != spells_.end()) {
        return &it->second;
    }

    // Try partial match (case-insensitive) - whole string prefix
    std::string lower_name = to_lowercase(name);

    for (const auto& [spell_name, spell] : spells_) {
        std::string lower_spell_name = to_lowercase(spell_name);

        if (lower_spell_name.find(lower_name) == 0) { // Starts with the search term
            return &spell;
        }
    }

    // Try per-word fuzzy match: "det inv" matches "detect invisibility"
    for (const auto& [spell_name, spell] : spells_) {
        std::string lower_spell_name = to_lowercase(spell_name);
        if (matches_words(lower_name, lower_spell_name)) {
            return &spell;
        }
    }

    return nullptr;
}

std::vector<const Spell*> SpellRegistry::get_spells_by_circle(int circle) const {
    std::vector<const Spell*> result;
    
    for (const auto& [name, spell] : spells_) {
        if (spell.circle == circle) {
            result.push_back(&spell);
        }
    }
    
    return result;
}

std::vector<const Spell*> SpellRegistry::get_spells_by_type(SpellType type) const {
    std::vector<const Spell*> result;
    
    for (const auto& [name, spell] : spells_) {
        if (spell.type == type) {
            result.push_back(&spell);
        }
    }
    
    return result;
}

void SpellRegistry::initialize_default_spells() {
    spells_.clear();
    
    // Circle 1 spells
    register_spell(Spell{
        .name = "cure light wounds",
        .description = "Heals minor injuries",
        .type = SpellType::Healing,
        .circle = 1,
        .cast_time_seconds = 2,
        .range_meters = 0,
        .duration_seconds = 0
    });
    
    register_spell(Spell{
        .name = "magic missile",
        .description = "Launches unerring projectiles of magical force",
        .type = SpellType::Offensive,
        .circle = 1,
        .cast_time_seconds = 3,
        .range_meters = 10,
        .duration_seconds = 0
    });
    
    register_spell(Spell{
        .name = "detect magic",
        .description = "Reveals magical auras",
        .type = SpellType::Divination,
        .circle = 1,
        .cast_time_seconds = 2,
        .range_meters = 0,
        .duration_seconds = 300
    });
    
    // Circle 2 spells
    register_spell(Spell{
        .name = "cure moderate wounds",
        .description = "Heals moderate injuries",
        .type = SpellType::Healing,
        .circle = 2,
        .cast_time_seconds = 3,
        .range_meters = 0,
        .duration_seconds = 0
    });
    
    register_spell(Spell{
        .name = "burning hands",
        .description = "Creates a cone of fire",
        .type = SpellType::Offensive,
        .circle = 2,
        .cast_time_seconds = 2,
        .range_meters = 3,
        .duration_seconds = 0
    });
    
    // Circle 3 spells
    register_spell(Spell{
        .name = "fireball",
        .description = "Launches an explosive sphere of fire",
        .type = SpellType::Offensive,
        .circle = 3,
        .cast_time_seconds = 4,
        .range_meters = 15,
        .duration_seconds = 0
    });
    
    register_spell(Spell{
        .name = "cure serious wounds",
        .description = "Heals serious injuries",
        .type = SpellType::Healing,
        .circle = 3,
        .cast_time_seconds = 4,
        .range_meters = 0,
        .duration_seconds = 0
    });
    
    Log::info("Initialized {} default spells", spells_.size());
}