#include "spell_system.hpp"
#include "actor.hpp"
#include "commands/command_system.hpp"
#include "core/logging.hpp"
#include "text/string_utils.hpp"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <magic_enum/magic_enum.hpp>

// ============================================================================
// Spell Implementation
// ============================================================================

bool Spell::can_cast(const Actor& caster) const {
    // Check class and level restrictions - only for players
    if (const auto* player = dynamic_cast<const Player*>(&caster)) {
        // Gods can cast any spell
        if (player->is_god()) {
            return true;
        }

        // Check if caster has required level for this circle
        if (caster.stats().level < circle) {
            return false;
        }

        std::string player_class = to_lowercase(player->player_class());
        if (player_class != "cleric" && player_class != "sorcerer") {
            return false;
        }
    } else {
        // Non-player actors (mobs) - just check level
        if (caster.stats().level < circle) {
            return false;
        }
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

bool SpellSlotCircle::use_slot() {
    if (current_slots <= 0) {
        return false;
    }
    
    current_slots--;
    return true;
}

void SpellSlotCircle::update_refresh() {
    using namespace std::chrono;
    
    auto now = steady_clock::now();
    auto elapsed = duration_cast<seconds>(now - last_refresh);
    
    if (elapsed >= refresh_time && current_slots < max_slots) {
        current_slots = std::min(max_slots, current_slots + 1);
        last_refresh = now;
        Log::debug("Spell slot refreshed: {} / {}", current_slots, max_slots);
    }
}

nlohmann::json SpellSlotCircle::to_json() const {
    return {
        {"current_slots", current_slots},
        {"max_slots", max_slots},
        {"refresh_time_seconds", static_cast<int>(refresh_time.count())},
        {"last_refresh_epoch", last_refresh.time_since_epoch().count()}
    };
}

Result<SpellSlotCircle> SpellSlotCircle::from_json(const nlohmann::json& json) {
    try {
        SpellSlotCircle circle;
        circle.current_slots = json.at("current_slots").get<int>();
        circle.max_slots = json.at("max_slots").get<int>();
        
        if (json.contains("refresh_time_seconds")) {
            circle.refresh_time = std::chrono::seconds{json["refresh_time_seconds"].get<int>()};
        }
        
        if (json.contains("last_refresh_epoch")) {
            auto epoch_count = json["last_refresh_epoch"].get<int64_t>();
            circle.last_refresh = std::chrono::steady_clock::time_point{
                std::chrono::steady_clock::duration{epoch_count}
            };
        } else {
            circle.last_refresh = std::chrono::steady_clock::now();
        }
        
        return circle;
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("SpellSlotCircle JSON parsing error", e.what()));
    }
}

// ============================================================================
// SpellSlots Implementation
// ============================================================================

void SpellSlots::initialize_for_class(std::string_view character_class, int level) {
    slots_.clear();
    
    // Different classes get different spell slot progressions
    if (character_class == "cleric" || character_class == "sorcerer") {
        // Calculate slots based on level (simplified D&D-style progression)
        for (int circle = 1; circle <= 9; ++circle) {
            int slots_for_circle = 0;
            
            // Circle availability based on level
            int required_level = (circle - 1) * 2 + 1; // Circle 1 at level 1, Circle 2 at level 3, etc.
            if (level >= required_level) {
                // Base slots + bonus slots based on level
                slots_for_circle = 1 + (level - required_level) / 4;
                slots_for_circle = std::min(slots_for_circle, 4); // Cap at 4 slots per circle
            }
            
            if (slots_for_circle > 0) {
                SpellSlotCircle slot_circle;
                slot_circle.max_slots = slots_for_circle;
                slot_circle.current_slots = slots_for_circle; // Start with full slots
                slot_circle.refresh_time = std::chrono::seconds{1800 + (circle - 1) * 600}; // Longer refresh for higher circles
                slot_circle.last_refresh = std::chrono::steady_clock::now();
                
                slots_[circle] = slot_circle;
            }
        }
    }
}

bool SpellSlots::has_slots(int circle) const {
    auto it = slots_.find(circle);
    return it != slots_.end() && it->second.has_slots();
}

bool SpellSlots::use_slot(int circle) {
    auto it = slots_.find(circle);
    if (it == slots_.end()) {
        return false;
    }
    
    return it->second.use_slot();
}

std::pair<int, int> SpellSlots::get_slot_info(int circle) const {
    auto it = slots_.find(circle);
    if (it == slots_.end()) {
        return {0, 0};
    }
    
    return {it->second.current_slots, it->second.max_slots};
}

void SpellSlots::update_refresh() {
    for (auto& [circle, slot_circle] : slots_) {
        slot_circle.update_refresh();
    }
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

nlohmann::json SpellSlots::to_json() const {
    nlohmann::json json = nlohmann::json::object();
    
    for (const auto& [circle, slot_circle] : slots_) {
        json[std::to_string(circle)] = slot_circle.to_json();
    }
    
    return json;
}

Result<SpellSlots> SpellSlots::from_json(const nlohmann::json& json) {
    try {
        SpellSlots spell_slots;
        
        for (const auto& [circle_str, circle_json] : json.items()) {
            int circle = std::stoi(circle_str);
            auto circle_result = SpellSlotCircle::from_json(circle_json);
            if (!circle_result) {
                return std::unexpected(circle_result.error());
            }
            spell_slots.slots_[circle] = circle_result.value();
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