#include "character_commands.hpp"
#include "command_parser.hpp"

#include "core/actor.hpp"
#include "core/mobile.hpp"
#include "core/player.hpp"
#include "core/logging.hpp"
#include "core/money.hpp"
#include "core/object.hpp"
#include "database/connection_pool.hpp"
#include "database/world_queries.hpp"
#include "game/composer_system.hpp"
#include "server/persistence_manager.hpp"
#include "text/string_utils.hpp"
#include "world/world_manager.hpp"
#include <fmt/format.h>
#include <algorithm>

namespace CharacterCommands {

// =============================================================================
// Character Status Commands
// =============================================================================

/**
 * Convert effect duration from rounds to human-readable MUD time.
 * 1 round = 4 real seconds, 1 MUD hour = 75 real seconds
 * So approximately 19 rounds = 1 MUD hour
 */
static std::string format_effect_duration(int hours) {
    if (hours < 0) {
        return "permanent";
    }
    if (hours == 1) {
        return "1 hour";
    }
    return fmt::format("{} hours", hours);
}

Result<CommandResult> cmd_affects(const CommandContext &ctx) {
    ctx.send("--- Active Effects ---");

    const auto& effects = ctx.actor->active_effects();
    if (effects.empty()) {
        ctx.send("You are not affected by any spells.");
    } else {
        for (const auto& effect : effects) {
            std::string duration_str = format_effect_duration(effect.duration_hours);

            std::string modifier_str;
            if (effect.modifier_value != 0 && !effect.modifier_stat.empty()) {
                modifier_str = fmt::format(" ({:+} {})", effect.modifier_value, effect.modifier_stat);
            }

            ctx.send(fmt::format("  {} - {}{}", effect.name, duration_str, modifier_str));
        }
    }

    ctx.send("--- End of Effects ---");

    return CommandResult::Success;
}

// Helper to get proficiency description
static std::string_view proficiency_description(int proficiency) {
    if (proficiency >= 95) return "(superb)";
    if (proficiency >= 85) return "(excellent)";
    if (proficiency >= 75) return "(very good)";
    if (proficiency >= 65) return "(good)";
    if (proficiency >= 55) return "(fair)";
    if (proficiency >= 45) return "(average)";
    if (proficiency >= 35) return "(below average)";
    if (proficiency >= 25) return "(poor)";
    if (proficiency >= 15) return "(very poor)";
    return "(awful)";
}

Result<CommandResult> cmd_skills(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players have skills.");
        return CommandResult::InvalidState;
    }

    // Check if abilities are loaded; if not, load them from database
    if (player->get_abilities().empty()) {
        // Load abilities from database for this character
        std::string player_name(player->name());
        auto result = ConnectionPool::instance().execute([&player_name](pqxx::work& txn)
            -> Result<std::vector<WorldQueries::CharacterAbilityData>> {
            return WorldQueries::load_character_abilities(txn, player_name);
        });

        if (result) {
            // Also load all ability definitions to get names and types
            auto abilities_result = ConnectionPool::instance().execute([](pqxx::work& txn)
                -> Result<std::vector<WorldQueries::AbilityData>> {
                return WorldQueries::load_all_abilities(txn);
            });

            if (abilities_result) {
                // Build a map of ability_id -> AbilityData for quick lookups
                std::unordered_map<int, const WorldQueries::AbilityData*> ability_map;
                for (const auto& ability : *abilities_result) {
                    ability_map[ability.id] = &ability;
                }

                // Populate player's abilities from character data
                for (const auto& char_ability : *result) {
                    auto it = ability_map.find(char_ability.ability_id);
                    if (it != ability_map.end()) {
                        LearnedAbility learned;
                        learned.ability_id = char_ability.ability_id;
                        learned.name = it->second->name;
                        learned.plain_name = it->second->plain_name;
                        learned.known = char_ability.known;
                        learned.proficiency = char_ability.proficiency;
                        if (char_ability.last_used) {
                            learned.last_used = *char_ability.last_used;
                        }

                        // Set type string
                        switch (it->second->type) {
                            case WorldQueries::AbilityType::Spell: learned.type = "SPELL"; break;
                            case WorldQueries::AbilityType::Skill: learned.type = "SKILL"; break;
                            case WorldQueries::AbilityType::Chant: learned.type = "CHANT"; break;
                            case WorldQueries::AbilityType::Song:  learned.type = "SONG"; break;
                        }
                        learned.min_level = it->second->min_position;
                        learned.violent = it->second->violent;

                        player->set_ability(learned);
                    }
                }
            }
        }
    }

    // Get known abilities grouped by type
    auto known_abilities = player->get_known_abilities();

    if (known_abilities.empty()) {
        ctx.send("--- Your Abilities ---");
        ctx.send("You have not learned any abilities yet.");
        ctx.send("Use 'practice' at a trainer to learn new skills.");
        ctx.send("--- End of Abilities ---");
        return CommandResult::Success;
    }

    // Sort by type then by name
    std::sort(known_abilities.begin(), known_abilities.end(),
        [](const LearnedAbility* a, const LearnedAbility* b) {
            if (a->type != b->type) return a->type < b->type;
            return a->plain_name < b->plain_name;
        });

    // Display abilities grouped by type
    std::string output;
    output += "--- Your Abilities ---\n";

    std::string current_type;
    for (const auto* ability : known_abilities) {
        if (ability->type != current_type) {
            current_type = ability->type;
            if (current_type == "SKILL") {
                output += "\n<b:white>Skills:</>\n";
            } else if (current_type == "SPELL") {
                output += "\n<b:white>Spells:</>\n";
            } else if (current_type == "CHANT") {
                output += "\n<b:white>Chants:</>\n";
            } else if (current_type == "SONG") {
                output += "\n<b:white>Songs:</>\n";
            }
        }

        // Format: "  skill name                   (proficiency) XX%"
        // Proficiency stored as 0-1000, display as 0-100
        int display_prof = ability->proficiency / 10;
        std::string prof_desc(proficiency_description(display_prof));
        output += fmt::format("  {:<30} {:12} {:3}%\n",
            ability->name, prof_desc, display_prof);
    }

    output += "\n--- End of Abilities ---";
    ctx.send(output);

    return CommandResult::Success;
}

// =============================================================================
// Trainer Commands
// =============================================================================

// Helper to find a teacher mob in the current room
// Returns a teacher if:
// 1. Mob has TEACHER flag (generic teacher), OR
// 2. Mob has a class_id matching the player's class (class-specific guildmaster)
static std::shared_ptr<Mobile> find_teacher_in_room(const std::shared_ptr<Room>& room, int player_class_id) {
    if (!room) return nullptr;

    std::shared_ptr<Mobile> class_teacher = nullptr;
    std::shared_ptr<Mobile> generic_teacher = nullptr;

    for (const auto& actor : room->contents().actors) {
        auto mobile = std::dynamic_pointer_cast<Mobile>(actor);
        if (!mobile) continue;

        // Check for TEACHER flag (generic teacher for any class)
        if (mobile->is_teacher()) {
            generic_teacher = mobile;
        }

        // Check for class-specific guildmaster (mob's class_id matches player's)
        if (mobile->class_id() > 0 && mobile->class_id() == player_class_id) {
            class_teacher = mobile;
        }
    }

    // Prefer class-specific teacher over generic
    return class_teacher ? class_teacher : generic_teacher;
}

// Helper to get class ID from class name
// IDs match the Class table in the database
static int get_class_id(std::string_view class_name) {
    static const std::unordered_map<std::string, int> class_ids = {
        {"sorcerer", 1}, {"cleric", 2}, {"thief", 3}, {"warrior", 4},
        {"paladin", 5}, {"anti-paladin", 6}, {"ranger", 7}, {"druid", 8},
        {"shaman", 9}, {"assassin", 10}, {"mercenary", 11}, {"necromancer", 12},
        {"conjurer", 13}, {"monk", 14}, {"berserker", 15}, {"priest", 16},
        {"diabolist", 17}, {"mystic", 18}, {"rogue", 19}, {"bard", 20},
        {"pyromancer", 21}, {"cryomancer", 22}, {"illusionist", 23}, {"hunter", 24}
    };

    std::string lower_name = to_lowercase(class_name);

    auto it = class_ids.find(lower_name);
    return (it != class_ids.end()) ? it->second : 0;
}

Result<CommandResult> cmd_practice(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can practice.");
        return CommandResult::InvalidState;
    }

    // Get player's class ID
    int class_id = get_class_id(player->player_class());
    if (class_id == 0) {
        ctx.send_error("Your class cannot learn abilities from trainers.");
        return CommandResult::InvalidState;
    }

    // Check for a teacher in the room (class-specific guildmaster or generic trainer)
    auto room = player->current_room();
    auto teacher = find_teacher_in_room(room, class_id);
    if (!teacher) {
        ctx.send("There is no trainer here to teach you.");
        return CommandResult::InvalidTarget;
    }

    // Load available abilities for this class from database
    auto class_abilities_result = ConnectionPool::instance().execute([class_id](pqxx::work& txn)
        -> Result<std::vector<WorldQueries::ClassAbilityData>> {
        return WorldQueries::load_class_abilities(txn, class_id);
    });

    if (!class_abilities_result || class_abilities_result->empty()) {
        ctx.send("This trainer cannot teach you anything.");
        return CommandResult::Success;
    }

    // Load all ability definitions
    auto abilities_result = ConnectionPool::instance().execute([](pqxx::work& txn)
        -> Result<std::vector<WorldQueries::AbilityData>> {
        return WorldQueries::load_all_abilities(txn);
    });

    if (!abilities_result) {
        ctx.send_error(fmt::format("Failed to load ability information: {}", abilities_result.error().message));
        return CommandResult::SystemError;
    }

    // Build ability lookup maps
    std::unordered_map<int, const WorldQueries::AbilityData*> ability_map;
    for (const auto& ability : *abilities_result) {
        ability_map[ability.id] = &ability;
    }

    struct ClassAbilityInfo {
        int min_level;
        int circle;
    };
    std::unordered_map<int, ClassAbilityInfo> class_ability_info;  // ability_id -> info
    for (const auto& ca : *class_abilities_result) {
        class_ability_info[ca.ability_id] = {ca.min_level, ca.circle};
    }

    if (ctx.arg_count() == 0) {
        // Show practiceable abilities
        std::string output;
        output += fmt::format("{} can teach you the following abilities:\n\n", teacher->display_name());

        int player_level = player->level();
        std::vector<std::tuple<std::string, int, int, std::string>> learnable;  // name, min_level, proficiency, type

        for (const auto& [ability_id, info] : class_ability_info) {
            auto it = ability_map.find(ability_id);
            if (it == ability_map.end()) continue;

            const auto* ability = it->second;

            // Check if player can learn this ability at their level
            if (info.min_level > player_level) continue;

            // Get current proficiency
            int proficiency = player->get_proficiency(ability_id);

            // Determine type string
            std::string type_str;
            switch (ability->type) {
                case WorldQueries::AbilityType::Spell: type_str = "spell"; break;
                case WorldQueries::AbilityType::Skill: type_str = "skill"; break;
                case WorldQueries::AbilityType::Chant: type_str = "chant"; break;
                case WorldQueries::AbilityType::Song:  type_str = "song"; break;
            }

            learnable.emplace_back(ability->name, info.min_level, proficiency, type_str);
        }

        // Sort by min level then name
        std::sort(learnable.begin(), learnable.end(),
            [](const auto& a, const auto& b) {
                if (std::get<1>(a) != std::get<1>(b)) return std::get<1>(a) < std::get<1>(b);
                return std::get<0>(a) < std::get<0>(b);
            });

        if (learnable.empty()) {
            output += "  (Nothing available at your level)\n";
        } else {
            output += fmt::format("  {:<30} {:>5}  {:>6}  {}\n", "Ability", "Level", "Prof%", "Type");
            output += fmt::format("  {:-<30} {:->5}  {:->6}  {:-<6}\n", "", "", "", "");
            for (const auto& [name, min_lv, prof, type] : learnable) {
                std::string prof_str = (prof > 0) ? fmt::format("{:3}%", prof) : "  -";
                output += fmt::format("  {:<30} {:>5}  {:>6}  {}\n", name, min_lv, prof_str, type);
            }
        }

        output += "\nUsage: practice <ability name>";
        ctx.send(output);
        return CommandResult::Success;
    }

    // Player wants to practice a specific ability
    std::string ability_name = ctx.args_from(0);

    // Find the ability by name (case-insensitive partial match)
    const WorldQueries::AbilityData* target_ability = nullptr;
    int target_min_level = 0;
    int target_circle = 0;

    std::string lower_search = to_lowercase(ability_name);

    for (const auto& [ability_id, info] : class_ability_info) {
        auto it = ability_map.find(ability_id);
        if (it == ability_map.end()) continue;

        const auto* ability = it->second;
        std::string lower_name = to_lowercase(ability->plain_name);

        // Check for prefix match
        if (lower_name.starts_with(lower_search) ||
            lower_name.find(lower_search) != std::string::npos) {
            target_ability = ability;
            target_min_level = info.min_level;
            target_circle = info.circle;
            break;
        }
    }

    if (!target_ability) {
        ctx.send(fmt::format("'{}' is not an ability you can practice here.", ability_name));
        return CommandResult::InvalidTarget;
    }

    // Check if player is high enough level
    if (target_min_level > player->level()) {
        ctx.send(fmt::format("You must be at least level {} to learn {}.",
                             target_min_level, target_ability->name));
        return CommandResult::InvalidState;
    }

    // Check current proficiency - can't practice above 75% at trainers
    int current_prof = player->get_proficiency(target_ability->id);
    constexpr int MAX_TRAINER_PROFICIENCY = 75;

    if (current_prof >= MAX_TRAINER_PROFICIENCY) {
        ctx.send(fmt::format("You have practiced {} as much as you can here. "
                             "You must use it to improve further.",
                             target_ability->name));
        return CommandResult::Success;
    }

    // TODO: Check for practice sessions (when implemented)
    // For now, just improve the skill

    // Calculate improvement (diminishing returns)
    int improvement = std::max(1, (MAX_TRAINER_PROFICIENCY - current_prof) / 10);
    int new_prof = std::min(MAX_TRAINER_PROFICIENCY, current_prof + improvement);

    // Update or create the ability on the player
    auto* existing = player->get_ability_mutable(target_ability->id);
    if (existing) {
        existing->proficiency = new_prof;
        existing->known = true;
    } else {
        LearnedAbility learned;
        learned.ability_id = target_ability->id;
        learned.name = target_ability->name;
        learned.plain_name = target_ability->plain_name;
        learned.known = true;
        learned.proficiency = new_prof;
        switch (target_ability->type) {
            case WorldQueries::AbilityType::Spell: learned.type = "SPELL"; break;
            case WorldQueries::AbilityType::Skill: learned.type = "SKILL"; break;
            case WorldQueries::AbilityType::Chant: learned.type = "CHANT"; break;
            case WorldQueries::AbilityType::Song:  learned.type = "SONG"; break;
        }
        learned.min_level = target_min_level;
        learned.violent = target_ability->violent;
        learned.circle = target_circle;
        player->set_ability(learned);
    }

    // Save to database
    std::string player_name(player->name());
    auto save_result = ConnectionPool::instance().execute(
        [&player_name, ability_id = target_ability->id, new_prof](pqxx::work& txn) -> Result<void> {
            return WorldQueries::save_character_ability(txn, player_name, ability_id, true, new_prof);
        });

    if (!save_result) {
        // Log but don't fail the command
        // (ability was updated in memory, just not persisted)
    }

    // Send feedback
    if (current_prof == 0) {
        ctx.send(fmt::format("You learn {}! ({}%)", target_ability->name, new_prof));
    } else {
        ctx.send(fmt::format("You practice {} and improve to {}%.",
                             target_ability->name, new_prof));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_train(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can train.");
        return CommandResult::InvalidState;
    }

    // TODO: Check if there's a trainer mob in the room
    // TODO: Implement stat training (uses practice sessions)

    if (ctx.arg_count() == 0) {
        ctx.send("You can train: str int wis dex con cha");
        ctx.send("You need to find a trainer and have practice sessions available.");
        ctx.send("Usage: train <stat>");
        return CommandResult::Success;
    }

    ctx.send("There is no trainer here.");
    return CommandResult::InvalidTarget;
}

// =============================================================================
// Toggle Commands
// =============================================================================

Result<CommandResult> cmd_wimpy(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can set wimpy.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        int current = player->wimpy_threshold();
        if (current == 0) {
            ctx.send("Wimpy is currently disabled.");
        } else {
            ctx.send(fmt::format("Your current wimpy level is {} hit points.", current));
        }
        ctx.send("Usage: wimpy <hp threshold> - automatically flee when HP drops below this");
        return CommandResult::Success;
    }

    // Parse the threshold
    int threshold = 0;
    try {
        threshold = std::stoi(std::string(ctx.arg(0)));
    } catch (...) {
        ctx.send_error("Invalid number. Usage: wimpy <hp threshold>");
        return CommandResult::InvalidSyntax;
    }

    if (threshold < 0) {
        ctx.send_error("Wimpy threshold must be 0 or higher.");
        return CommandResult::InvalidSyntax;
    }

    if (threshold > player->stats().max_hit_points) {
        ctx.send_error("Wimpy threshold cannot exceed your maximum hit points.");
        return CommandResult::InvalidSyntax;
    }

    player->set_wimpy_threshold(threshold);

    if (threshold == 0) {
        ctx.send("Wimpy disabled. You will fight to the death!");
    } else {
        ctx.send(fmt::format("Wimpy set to {} hit points. You will flee when below this.", threshold));
    }

    return CommandResult::Success;
}

// =============================================================================
// Social Interaction Commands
// =============================================================================
// Note: Individual toggle commands (brief, compact, autoloot, autogold, autosplit)
// have been removed. Use the unified "toggle" command instead.

Result<CommandResult> cmd_afk(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can set AFK status.");
        return CommandResult::InvalidState;
    }

    // Toggle AFK status
    player->toggle_player_flag(PlayerFlag::Afk);

    if (player->is_afk()) {
        // Going AFK
        if (ctx.arg_count() > 0) {
            std::string message = std::string(ctx.command.full_argument_string);
            player->set_afk_message(message);
            ctx.send(fmt::format("You are now AFK: {}", message));
        } else {
            player->set_afk_message("");
            ctx.send("You are now marked as AFK (away from keyboard).");
        }
        ctx.send_to_room(fmt::format("{} has gone AFK.", player->display_name()), true);
    } else {
        // Returning from AFK
        player->set_afk_message("");
        ctx.send("You are no longer AFK. Welcome back!");
        ctx.send_to_room(fmt::format("{} has returned from AFK.", player->display_name()), true);
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_title(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can set titles.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        if (player->title().empty()) {
            ctx.send("You have no title set.");
        } else {
            ctx.send(fmt::format("Your current title: {} {}", player->name(), player->title()));
        }
        ctx.send("Usage: title <your title>  or  title clear");
        ctx.send("Example: title the Brave");
        return CommandResult::Success;
    }

    std::string new_title = std::string(ctx.command.full_argument_string);

    // Handle clearing the title
    if (new_title == "clear" || new_title == "none") {
        player->set_title("");
        ctx.send("Your title has been cleared.");
        return CommandResult::Success;
    }

    // Validate title length
    if (new_title.length() > 45) {
        ctx.send_error("Title is too long. Maximum 45 characters.");
        return CommandResult::InvalidSyntax;
    }

    player->set_title(new_title);
    ctx.send(fmt::format("Your title is now: {} {}", player->name(), new_title));

    return CommandResult::Success;
}

Result<CommandResult> cmd_description(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can set descriptions.");
        return CommandResult::InvalidState;
    }

    // Show current description and start composer if no arguments
    if (ctx.arg_count() == 0) {
        ctx.send("Your current description:");
        if (player->description().empty()) {
            ctx.send("  (No description set)");
        } else {
            // Split multi-line descriptions for display
            std::string desc{player->description()};
            size_t pos = 0;
            while ((pos = desc.find('\n')) != std::string::npos) {
                ctx.send(fmt::format("  {}", desc.substr(0, pos)));
                desc = desc.substr(pos + 1);
            }
            if (!desc.empty()) {
                ctx.send(fmt::format("  {}", desc));
            }
        }
        ctx.send("");
        ctx.send("Starting description editor...");

        // Start composer for multi-line description
        ComposerConfig config;
        config.header_message = "Enter your description (what others see when they look at you):";
        config.save_message = "Description saved.";
        config.cancel_message = "Description unchanged.";
        config.max_lines = 20;  // Reasonable limit for descriptions

        auto composer = std::make_shared<ComposerSystem>(
            std::weak_ptr<Player>(player), config);

        composer->set_completion_callback([player](ComposerResult result) {
            if (result.success) {
                if (result.combined_text.empty()) {
                    player->send_message("No description entered. Description unchanged.");
                } else if (result.combined_text.length() > 2000) {
                    player->send_message("Description too long. Maximum 2000 characters. Description unchanged.");
                } else {
                    player->set_description(result.combined_text);
                    player->send_message("Your description has been updated.");
                }
            }
        });

        player->start_composing(composer);
        return CommandResult::Success;
    }

    std::string new_desc = std::string(ctx.command.full_argument_string);

    // Handle clearing the description
    if (new_desc == "clear" || new_desc == "none") {
        player->set_description("");
        ctx.send("Your description has been cleared.");
        return CommandResult::Success;
    }

    // Single-line description (legacy behavior)
    if (new_desc.length() > 500) {
        ctx.send_error("Single-line description is too long. Maximum 500 characters.");
        ctx.send("Use 'description' without arguments for the multi-line editor.");
        return CommandResult::InvalidSyntax;
    }

    player->set_description(new_desc);
    ctx.send("Your description has been updated.");
    ctx.send(fmt::format("New description: {}", new_desc));

    return CommandResult::Success;
}

// Toggle definition structure for database-driven toggles
struct ToggleDefinition {
    std::string name;
    std::string display_name;
    std::string description;
    PlayerFlag flag;
    int min_level;      // 0 = all players, 100+ = immortal
    std::string category;
    std::function<bool(Player*)> getter;
};

// Static toggle definitions - maps to PlayerFlag enum
// Descriptions can be overridden from database
static const std::vector<ToggleDefinition> TOGGLE_DEFINITIONS = {
    {"brief", "Brief", "Show brief room descriptions", PlayerFlag::Brief, 0, "DISPLAY",
        [](Player* p) { return p->is_brief(); }},
    {"compact", "Compact", "Reduce blank lines in output", PlayerFlag::Compact, 0, "DISPLAY",
        [](Player* p) { return p->is_compact(); }},
    {"autoexit", "Auto Exit", "Automatically show exits", PlayerFlag::AutoExit, 0, "DISPLAY",
        [](Player* p) { return p->is_autoexit(); }},
    {"autoloot", "Auto Loot", "Automatically loot corpses", PlayerFlag::AutoLoot, 0, "COMBAT",
        [](Player* p) { return p->is_autoloot(); }},
    {"autogold", "Auto Gold", "Automatically take gold from corpses", PlayerFlag::AutoGold, 0, "COMBAT",
        [](Player* p) { return p->is_autogold(); }},
    {"autosplit", "Auto Split", "Automatically split gold with group", PlayerFlag::AutoSplit, 0, "COMBAT",
        [](Player* p) { return p->is_autosplit(); }},
    {"showdice", "Show Dice Rolls", "Display detailed dice rolls and damage calculations in combat", PlayerFlag::ShowDiceRolls, 0, "COMBAT",
        [](Player* p) { return p->is_show_dice_rolls(); }},
    {"deaf", "Deaf", "Block shouts and gossip", PlayerFlag::Deaf, 0, "SOCIAL",
        [](Player* p) { return p->is_deaf(); }},
    {"notell", "No Tell", "Block tells from non-gods", PlayerFlag::NoTell, 0, "SOCIAL",
        [](Player* p) { return p->is_notell(); }},
    {"afk", "AFK", "Away from keyboard", PlayerFlag::Afk, 0, "SOCIAL",
        [](Player* p) { return p->is_afk(); }},
    {"holylight", "Holy Light", "See everything (invis, dark, hidden)", PlayerFlag::HolyLight, 100, "IMMORTAL",
        [](Player* p) { return p->is_holylight(); }},
    {"showids", "Show IDs", "Show entity IDs on mobs/objects", PlayerFlag::ShowIds, 100, "IMMORTAL",
        [](Player* p) { return p->is_show_ids(); }},
};

// Helper to find a toggle definition by name
static const ToggleDefinition* find_toggle(std::string_view name) {
    std::string lower_name = to_lowercase(name);
    for (const auto& toggle : TOGGLE_DEFINITIONS) {
        if (toggle.name == lower_name) {
            return &toggle;
        }
    }
    // Also check common aliases
    if (lower_name == "noshout" || lower_name == "nogossip") {
        return find_toggle("deaf");
    }
    return nullptr;
}

Result<CommandResult> cmd_toggle(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use toggle.");
        return CommandResult::InvalidState;
    }

    // Helper to format on/off
    auto on_off = [](bool value) { return value ? "ON " : "OFF"; };

    // Try to load toggle definitions from database for display
    std::unordered_map<std::string, WorldQueries::PlayerToggleData> db_toggles;
    auto db_result = ConnectionPool::instance().execute([](pqxx::work& txn)
        -> Result<std::vector<WorldQueries::PlayerToggleData>> {
        return WorldQueries::load_all_player_toggles(txn);
    });
    if (db_result) {
        for (auto& t : *db_result) {
            db_toggles[t.name] = std::move(t);
        }
    }

    if (ctx.arg_count() == 0) {
        // Show all toggles grouped by category
        bool is_immortal = player->is_god();

        // Group toggles by category
        std::map<std::string, std::vector<const ToggleDefinition*>> by_category;
        for (const auto& toggle : TOGGLE_DEFINITIONS) {
            // Check level requirement
            if (toggle.min_level > 0 && !is_immortal) continue;
            by_category[toggle.category].push_back(&toggle);
        }

        // Display order
        std::vector<std::pair<std::string, std::string>> category_order = {
            {"DISPLAY", "Display Options"},
            {"COMBAT", "Combat Options"},
            {"SOCIAL", "Social Options"},
            {"IMMORTAL", "Immortal Options"},
        };

        for (const auto& [cat_key, cat_name] : category_order) {
            auto it = by_category.find(cat_key);
            if (it == by_category.end() || it->second.empty()) continue;

            ctx.send(fmt::format("--- {} ---", cat_name));
            for (const auto* toggle : it->second) {
                bool value = toggle->getter(player.get());

                // Use DB description if available, else use hardcoded
                std::string description = toggle->description;
                auto db_it = db_toggles.find(toggle->name);
                if (db_it != db_toggles.end()) {
                    description = db_it->second.description;
                }

                ctx.send(fmt::format("  {:<12}: {}  - {}",
                    toggle->name, on_off(value), description));
            }
        }

        ctx.send("--- End of Options ---");
        ctx.send("Usage: toggle <option> to flip a setting");
        return CommandResult::Success;
    }

    std::string_view option = ctx.arg(0);

    // Find the toggle definition
    const ToggleDefinition* toggle = find_toggle(option);
    if (!toggle) {
        ctx.send_error(fmt::format("Unknown toggle option: {}", option));
        ctx.send("Type 'toggle' with no arguments to see available options.");
        return CommandResult::InvalidSyntax;
    }

    // Check level requirement
    if (toggle->min_level > 0 && !player->is_god()) {
        ctx.send_error(fmt::format("Only immortals can toggle {}.", toggle->name));
        return CommandResult::InsufficientPrivs;
    }

    // Toggle the flag
    player->toggle_player_flag(toggle->flag);
    bool new_value = toggle->getter(player.get());

    // Get display name from DB if available
    std::string display_name = toggle->display_name;
    auto db_it = db_toggles.find(toggle->name);
    if (db_it != db_toggles.end()) {
        display_name = db_it->second.display_name;
    }

    ctx.send(fmt::format("{} is now {}.", display_name, new_value ? "ON" : "OFF"));

    // Special messages for certain toggles
    if (toggle->flag == PlayerFlag::HolyLight && new_value) {
        ctx.send("You can now see invisible, hidden, and in darkness.");
    }

    // Save the player to persist preference changes
    auto save_result = PersistenceManager::instance().save_player(*player);
    if (!save_result) {
        Log::warn("Failed to save player {} after toggle change: {}",
                  player->name(), save_result.error().message);
    }

    return CommandResult::Success;
}

// =============================================================================
// Consent/PvP Commands
// =============================================================================

Result<CommandResult> cmd_consent(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can manage consent.");
        return CommandResult::InvalidState;
    }

    // Consent allows specific players to attack/steal/etc from you
    // Currently only implements "consent all" - per-player consent is a future TODO

    if (ctx.arg_count() == 0) {
        ctx.send("--- Consent Status ---");
        if (player->has_player_flag(PlayerFlag::Consent)) {
            ctx.send("You are consenting to ALL players (dangerous!)");
        } else {
            ctx.send("You have not given consent to anyone.");
        }
        ctx.send("");
        ctx.send("Usage:");
        ctx.send("  consent all        - Allow anyone (dangerous!)");
        ctx.send("  consent none       - Revoke all consent");
        ctx.send("--- End of Consent ---");
        return CommandResult::Success;
    }

    std::string_view arg = ctx.arg(0);

    if (arg == "all") {
        if (player->has_player_flag(PlayerFlag::Consent)) {
            ctx.send("You are already consenting to all players.");
            return CommandResult::InvalidState;
        }
        player->set_player_flag(PlayerFlag::Consent, true);
        ctx.send("You are now consenting to ALL players. Be careful!");
        ctx.send_to_room(fmt::format("{} throws caution to the wind!", player->display_name()), true);
    } else if (arg == "none") {
        if (!player->has_player_flag(PlayerFlag::Consent)) {
            ctx.send("You are not currently consenting to anyone.");
            return CommandResult::InvalidState;
        }
        player->set_player_flag(PlayerFlag::Consent, false);
        ctx.send("You have revoked consent from everyone.");
    } else {
        ctx.send_error("Usage: consent all|none");
        ctx.send("Note: Per-player consent is not yet implemented.");
        return CommandResult::InvalidSyntax;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_pk(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can toggle PK status.");
        return CommandResult::InvalidState;
    }

    // PK mode allows you to attack and be attacked by other PK players

    if (ctx.arg_count() == 0) {
        ctx.send("--- PK Status ---");
        ctx.send(fmt::format("PK Mode: {}", player->is_pk_enabled() ? "ON" : "OFF"));
        ctx.send("");
        ctx.send("PK (Player Killing) mode allows you to engage in combat");
        ctx.send("with other players who also have PK mode enabled.");
        ctx.send("");
        ctx.send("Usage: pk on|off");
        ctx.send("WARNING: PK mode cannot be disabled for 24 hours once enabled!");
        ctx.send("--- End of PK Status ---");
        return CommandResult::Success;
    }

    std::string_view arg = ctx.arg(0);

    if (arg == "on" || arg == "yes" || arg == "enable") {
        if (player->is_pk_enabled()) {
            ctx.send("PK mode is already enabled.");
            return CommandResult::InvalidState;
        }
        ctx.send("*** WARNING ***");
        ctx.send("Enabling PK mode allows other PK players to attack you!");
        ctx.send("Once enabled, you cannot disable it for 24 real-world hours.");
        ctx.send("Type 'pk confirm' to enable PK mode.");
        return CommandResult::Success;
    } else if (arg == "confirm") {
        if (player->is_pk_enabled()) {
            ctx.send("PK mode is already enabled.");
            return CommandResult::InvalidState;
        }
        player->set_player_flag(PlayerFlag::PkEnabled, true);
        player->set_pk_enabled_time(std::time(nullptr));
        ctx.send("PK Mode: ENABLED");
        ctx.send("You may now attack and be attacked by other PK players.");
        ctx.send_to_room(fmt::format("{} has enabled PK mode!", player->display_name()), true);
    } else if (arg == "off" || arg == "no" || arg == "disable") {
        if (!player->is_pk_enabled()) {
            ctx.send("PK mode is not enabled.");
            return CommandResult::InvalidState;
        }
        // Check 24-hour lockout
        if (!player->can_disable_pk()) {
            std::time_t enabled = player->pk_enabled_time();
            std::time_t now = std::time(nullptr);
            std::time_t elapsed = now - enabled;
            std::time_t remaining = (24 * 60 * 60) - elapsed;
            int hours = static_cast<int>(remaining / 3600);
            int minutes = static_cast<int>((remaining % 3600) / 60);
            ctx.send(fmt::format("PK mode cannot be disabled yet. {} hours, {} minutes remaining.",
                                hours, minutes));
            return CommandResult::InvalidState;
        }
        player->set_player_flag(PlayerFlag::PkEnabled, false);
        player->set_pk_enabled_time(0);
        ctx.send("PK Mode: DISABLED");
        ctx.send("You may no longer attack or be attacked by other PK players.");
    } else {
        ctx.send_error("Usage: pk on|off");
        return CommandResult::InvalidSyntax;
    }

    return CommandResult::Success;
}

// =============================================================================
// Follower Commands
// =============================================================================

Result<CommandResult> cmd_call(const CommandContext &ctx) {
    // Call all followers to the actor's location
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can call followers.");
        return CommandResult::InvalidState;
    }

    // Check if player has any followers
    const auto& followers = player->get_followers();
    if (followers.empty()) {
        ctx.send("You have no followers to call.");
        return CommandResult::InvalidState;
    }

    int called_count = 0;
    auto player_room = ctx.actor->current_room();
    for (const auto& follower_weak : followers) {
        auto follower = follower_weak.lock();
        if (!follower) continue;

        // Skip if follower is already in the room
        if (follower->current_room() == player_room) {
            continue;
        }

        // TODO: Move follower to player's room
        // This requires follower movement implementation
        called_count++;
    }

    if (called_count == 0) {
        ctx.send("All your followers are already here.");
    } else {
        ctx.send("You call for your followers.");
        ctx.send_to_room(fmt::format("{} calls for their followers.", ctx.actor->display_name()), true);
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_order(const CommandContext &ctx) {
    // Order followers or charmies to perform actions
    if (ctx.arg_count() < 2) {
        ctx.send("Usage: order <follower|all> <command>");
        ctx.send("Examples:");
        ctx.send("  order all follow me");
        ctx.send("  order guard kill orc");
        return CommandResult::InvalidSyntax;
    }

    std::string target_name{ctx.arg(0)};
    std::string command_str = ctx.args_from(1);

    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can order followers.");
        return CommandResult::InvalidState;
    }

    bool order_all = (target_name == "all" || target_name == "followers");

    if (order_all) {
        const auto& followers = player->get_followers();
        if (followers.empty()) {
            ctx.send("You have no followers to order.");
            return CommandResult::InvalidState;
        }

        ctx.send(fmt::format("You order your followers to '{}'.", command_str));
        ctx.send_to_room(fmt::format("{} gives orders to their followers.", ctx.actor->display_name()), true);

        // TODO: Execute command for each follower
        // This requires command execution on behalf of followers
        for (const auto& follower_weak : followers) {
            auto follower = follower_weak.lock();
            if (!follower) continue;
            // Commands::execute(follower, command_str);
        }
    } else {
        // Find specific follower by name
        auto target = ctx.find_actor_target(target_name);
        if (!target) {
            ctx.send_error(fmt::format("You don't see '{}' here.", target_name));
            return CommandResult::InvalidTarget;
        }

        auto mob = std::dynamic_pointer_cast<Mobile>(target);
        if (!mob) {
            ctx.send_error("You can only order NPCs.");
            return CommandResult::InvalidTarget;
        }

        // Check if the mob is charmed (via ActorFlag)
        // TODO: Add check for who charmed the mob and follower relationships
        if (!target->has_flag(ActorFlag::Charm)) {
            ctx.send_error(fmt::format("{} refuses to follow your orders.", target->display_name()));
            return CommandResult::InvalidState;
        }

        ctx.send(fmt::format("You order {} to '{}'.", target->display_name(), command_str));
        ctx.send_to_room(fmt::format("{} gives an order to {}.", ctx.actor->display_name(), target->display_name()), true);

        // TODO: Execute command for the follower
        // Commands::execute(mob, command_str);
    }

    return CommandResult::Success;
}

// =============================================================================
// Class Ability Commands
// =============================================================================

Result<CommandResult> cmd_subclass(const CommandContext &ctx) {
    // Choose or view subclass specialization
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can have subclasses.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        // Show current subclass status
        ctx.send("Subclass System:");
        ctx.send("----------------");
        // TODO: Show current subclass if any
        ctx.send("You have not yet chosen a subclass specialization.");
        ctx.send("");
        ctx.send("Usage: subclass <specialization>");
        ctx.send("This choice is permanent and will shape your abilities.");
        return CommandResult::Success;
    }

    std::string subclass_name{ctx.arg(0)};

    // Check if player meets requirements (typically level 20+)
    if (player->stats().level < 20) {
        ctx.send_error("You must be at least level 20 to choose a subclass.");
        return CommandResult::InvalidState;
    }

    // TODO: Validate subclass choice based on player's class
    // TODO: Check if player already has a subclass

    ctx.send(fmt::format("Subclass selection '{}' is not yet implemented.", subclass_name));
    ctx.send("Please check back when the subclass system is complete.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_shapechange(const CommandContext &ctx) {
    // Druids can shapechange into various forms
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can shapechange.");
        return CommandResult::InvalidState;
    }

    // TODO: Check if player is a druid or has shapechange ability
    // For now, stub implementation

    if (ctx.arg_count() == 0) {
        ctx.send("Shapechange Forms:");
        ctx.send("------------------");
        ctx.send("  wolf     - Swift predator form");
        ctx.send("  bear     - Powerful combat form");
        ctx.send("  eagle    - Flying scout form");
        ctx.send("  natural  - Return to natural form");
        ctx.send("");
        ctx.send("Usage: shapechange <form>");
        return CommandResult::Success;
    }

    std::string form_name{ctx.arg(0)};

    if (form_name == "natural" || form_name == "normal" || form_name == "human") {
        // Return to normal form
        ctx.send("You shift back into your natural form.");
        ctx.send_to_room(fmt::format("{}'s body ripples and transforms back to normal.", ctx.actor->display_name()), true);
        return CommandResult::Success;
    }

    // TODO: Validate form type
    // TODO: Apply shapechange effects
    // TODO: Change actor's display name and stats temporarily

    ctx.send(fmt::format("You focus your will and attempt to become a {}...", form_name));
    ctx.send("Shapechange is not yet fully implemented.");

    return CommandResult::Success;
}

// =============================================================================
// Communication Commands
// =============================================================================

Result<CommandResult> cmd_write(const CommandContext &ctx) {
    // Write on objects that can be written on (paper, boards, etc.)
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can write.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Usage: write <object> [message]");
        ctx.send("Examples:");
        ctx.send("  write note Hello everyone!");
        ctx.send("  write board Meeting tomorrow at noon.");
        return CommandResult::InvalidSyntax;
    }

    std::string target_name{ctx.arg(0)};

    // Check inventory first, then room
    auto objects = ctx.find_objects_matching(target_name);
    if (objects.empty()) {
        ctx.send_error(fmt::format("You don't have '{}' and it's not here.", target_name));
        return CommandResult::InvalidTarget;
    }

    auto target = objects.front();

    // Check if object is writable
    // TODO: Check for ITEM_WRITE flag or similar
    // For now, check if it's a note or has "paper" in keywords

    if (ctx.arg_count() < 2) {
        ctx.send(fmt::format("What do you want to write on {}?", target->display_name()));
        return CommandResult::InvalidSyntax;
    }

    std::string message = ctx.args_from(1);

    // TODO: Actually write the message to the object
    // This would involve updating the object's extra descriptions

    ctx.send(fmt::format("You write on {}:", target->display_name()));
    ctx.send(fmt::format("  \"{}\"", message));
    ctx.send_to_room(fmt::format("{} writes something on {}.", ctx.actor->display_name(), target->display_name()), true);

    return CommandResult::Success;
}

// =============================================================================
// Set Command (god-level configuration)
// =============================================================================

// Helper to parse a number from string, returns nullopt if invalid
std::optional<int> parse_number(std::string_view str) {
    if (str.empty()) return std::nullopt;
    try {
        size_t pos = 0;
        int value = std::stoi(std::string(str), &pos);
        if (pos == str.size()) return value;
    } catch (...) {}
    return std::nullopt;
}

// Helper to parse a long from string, returns nullopt if invalid
std::optional<long> parse_long(std::string_view str) {
    if (str.empty()) return std::nullopt;
    try {
        size_t pos = 0;
        long value = std::stol(std::string(str), &pos);
        if (pos == str.size()) return value;
    } catch (...) {}
    return std::nullopt;
}

Result<CommandResult> cmd_set(const CommandContext &ctx) {
    // Usage: set <player> <field> <value>
    // Or: set <field> <value> (applies to self)

    if (ctx.arg_count() == 0) {
        ctx.send("<b:yellow>Set command - modify player/character properties</>");
        ctx.send("");
        ctx.send("<b:white>Usage:</> set <target> <field> <value>");
        ctx.send("        Use 'self' or 'me' to target yourself");
        ctx.send("");
        ctx.send("<b:cyan>Stats:</> str, int, wis, dex, con, cha");
        ctx.send("<b:cyan>Points:</> hp, maxhp, move, maxmove");
        ctx.send("<b:cyan>Offensive:</> accuracy, attackpower, spellpower, penetration");
        ctx.send("<b:cyan>Defensive:</> evasion, armor, soak, hardness, ward");
        ctx.send("<b:cyan>Resist:</> fire, cold, lightning, acid, poison");
        ctx.send("<b:cyan>Other:</> perception, concealment, focus");
        ctx.send("<b:cyan>Progress:</> level, exp, align");
        ctx.send("<b:cyan>Character:</> class, race, gender, size, title");
        ctx.send("<b:cyan>Player:</> home, godlevel");
        ctx.send("<b:cyan>Flags:</> brief, compact, autoloot, autogold, autosplit,");
        ctx.send("        autoexit, wimpy, afk, deaf, notell, pk, holylight, showids");
        ctx.send("<b:cyan>Skills:</> skill <skill_name> <value>");
        ctx.send("<b:cyan>Currency:</> wallet, bank (use money format: 3p 2g 5s 10c)");
        ctx.send("");
        ctx.send("<b:white>Example:</> set Lokari str 18");
        ctx.send("         set self level 50");
        ctx.send("         set me wallet 3p 2g");
        ctx.send("         set self skill fireball 95");
        return CommandResult::Success;
    }

    // Require at least 3 args: target field value
    if (ctx.arg_count() < 3) {
        ctx.send_error("Usage: set <target> <field> <value>");
        ctx.send_info("Use 'self' or 'me' to target yourself.");
        return CommandResult::InvalidSyntax;
    }

    // Parse target - "self" or "me" means the actor
    std::shared_ptr<Actor> target;
    std::string target_arg = std::string{ctx.arg(0)};
    std::transform(target_arg.begin(), target_arg.end(), target_arg.begin(), ::tolower);

    if (target_arg == "self" || target_arg == "me") {
        target = ctx.actor;
    } else {
        target = ctx.find_actor_global(ctx.arg(0));
        if (!target) {
            ctx.send_error(fmt::format("Player '{}' not found.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }
    }

    std::string field = std::string{ctx.arg(1)};
    std::string value_str = std::string{ctx.arg(2)};
    size_t value_arg_index = 2;

    // Lowercase the field for matching
    std::transform(field.begin(), field.end(), field.begin(), ::tolower);

    auto target_player = std::dynamic_pointer_cast<Player>(target);
    Stats& stats = target->stats();

    // =========================================================================
    // PRIMARY STATS
    // =========================================================================
    if (field == "str" || field == "strength") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Strength must be a number.");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 1, 100);
        stats.strength = clamped;
        ctx.send_success(fmt::format("{}'s strength set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    if (field == "int" || field == "intelligence") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Intelligence must be a number.");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 1, 100);
        stats.intelligence = clamped;
        ctx.send_success(fmt::format("{}'s intelligence set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    if (field == "wis" || field == "wisdom") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Wisdom must be a number.");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 1, 100);
        stats.wisdom = clamped;
        ctx.send_success(fmt::format("{}'s wisdom set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    if (field == "dex" || field == "dexterity") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Dexterity must be a number.");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 1, 100);
        stats.dexterity = clamped;
        ctx.send_success(fmt::format("{}'s dexterity set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    if (field == "con" || field == "constitution") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Constitution must be a number.");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 1, 100);
        stats.constitution = clamped;
        ctx.send_success(fmt::format("{}'s constitution set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    if (field == "cha" || field == "charisma") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Charisma must be a number.");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 1, 100);
        stats.charisma = clamped;
        ctx.send_success(fmt::format("{}'s charisma set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    // =========================================================================
    // HIT POINTS / MANA / MOVEMENT
    // =========================================================================
    if (field == "hp" || field == "hitpoints") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Hit points must be a number.");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, -9, stats.max_hit_points);
        stats.hit_points = clamped;
        ctx.send_success(fmt::format("{}'s hit points set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    if (field == "maxhp" || field == "maxhitpoints") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Max hit points must be a number.");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 1, 500000);
        stats.max_hit_points = clamped;
        if (stats.hit_points > clamped) stats.hit_points = clamped;
        ctx.send_success(fmt::format("{}'s max hit points set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    if (field == "stamina" || field == "st" || field == "move" || field == "movement") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Stamina must be a number.");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 0, stats.max_stamina);
        stats.stamina = clamped;
        ctx.send_success(fmt::format("{}'s stamina set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    if (field == "maxstamina" || field == "maxst" || field == "maxmove" || field == "maxmovement") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Max stamina must be a number.");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 1, 500000);
        stats.max_stamina = clamped;
        if (stats.stamina > clamped) stats.stamina = clamped;
        ctx.send_success(fmt::format("{}'s max stamina set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    // =========================================================================
    // COMBAT STATS
    // =========================================================================
    if (field == "accuracy" || field == "hitroll") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Accuracy must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.accuracy = *val;
        ctx.send_success(fmt::format("{}'s accuracy set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "attackpower" || field == "damroll") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Attack power must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.attack_power = *val;
        ctx.send_success(fmt::format("{}'s attack power set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "evasion") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Evasion must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.evasion = *val;
        ctx.send_success(fmt::format("{}'s evasion set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "armor" || field == "ac" || field == "armorrating") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Armor rating must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.armor_rating = *val;
        ctx.send_success(fmt::format("{}'s armor rating set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "perception") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Perception must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.perception = *val;
        ctx.send_success(fmt::format("{}'s perception set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "concealment" || field == "hiddenness") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Concealment must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.concealment = *val;
        ctx.send_success(fmt::format("{}'s concealment set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "focus") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Focus must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.focus = *val;
        ctx.send_success(fmt::format("{}'s focus set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    // =========================================================================
    // ADDITIONAL OFFENSIVE STATS
    // =========================================================================
    if (field == "spellpower" || field == "spell_power") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Spell power must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.spell_power = *val;
        ctx.send_success(fmt::format("{}'s spell power set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "penetration" || field == "armorpen") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Penetration must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.penetration_flat = *val;
        ctx.send_success(fmt::format("{}'s armor penetration set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    // =========================================================================
    // ADDITIONAL DEFENSIVE STATS
    // =========================================================================
    if (field == "soak") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Soak must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.soak = *val;
        ctx.send_success(fmt::format("{}'s soak set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "hardness") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Hardness must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.hardness = *val;
        ctx.send_success(fmt::format("{}'s hardness set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "ward") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Ward must be a number (0-100).");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 0, 100);
        stats.ward_percent = clamped;
        ctx.send_success(fmt::format("{}'s ward set to {}%.", target->name(), clamped));
        return CommandResult::Success;
    }

    // =========================================================================
    // ELEMENTAL RESISTANCES
    // =========================================================================
    if (field == "fire" || field == "fire_resist") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Fire resistance must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.resistance_fire = *val;
        ctx.send_success(fmt::format("{}'s fire resistance set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "cold" || field == "cold_resist") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Cold resistance must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.resistance_cold = *val;
        ctx.send_success(fmt::format("{}'s cold resistance set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "lightning" || field == "lightning_resist" || field == "electric") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Lightning resistance must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.resistance_lightning = *val;
        ctx.send_success(fmt::format("{}'s lightning resistance set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "acid" || field == "acid_resist") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Acid resistance must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.resistance_acid = *val;
        ctx.send_success(fmt::format("{}'s acid resistance set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    if (field == "poison" || field == "poison_resist") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Poison resistance must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.resistance_poison = *val;
        ctx.send_success(fmt::format("{}'s poison resistance set to {}.", target->name(), *val));
        return CommandResult::Success;
    }

    // =========================================================================
    // LEVEL / EXPERIENCE / ALIGNMENT
    // =========================================================================
    if (field == "level" || field == "lvl") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Level must be a number.");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 1, 100);
        stats.level = clamped;
        if (target_player) {
            target_player->set_level(clamped);
        }
        ctx.send_success(fmt::format("{}'s level set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    if (field == "exp" || field == "experience") {
        auto val = parse_long(value_str);
        if (!val) {
            ctx.send_error("Experience must be a number.");
            return CommandResult::InvalidSyntax;
        }
        stats.experience = std::max(0L, *val);
        ctx.send_success(fmt::format("{}'s experience set to {}.", target->name(), stats.experience));
        return CommandResult::Success;
    }

    if (field == "align" || field == "alignment") {
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("Alignment must be a number (-1000 to 1000).");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, -1000, 1000);
        stats.alignment = clamped;
        ctx.send_success(fmt::format("{}'s alignment set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    // =========================================================================
    // CHARACTER PROPERTIES (CLASS, RACE, GENDER, SIZE, TITLE)
    // =========================================================================
    if (field == "class") {
        if (target_player) {
            target_player->set_class(value_str);
            ctx.send_success(fmt::format("{}'s class set to {}.", target->name(), value_str));
        } else {
            ctx.send_error("Only players have a class.");
        }
        return CommandResult::Success;
    }

    if (field == "race") {
        target->set_race(value_str);
        ctx.send_success(fmt::format("{}'s race set to {}.", target->name(), value_str));
        return CommandResult::Success;
    }

    if (field == "gender" || field == "sex") {
        std::string gender_lower = value_str;
        std::transform(gender_lower.begin(), gender_lower.end(), gender_lower.begin(), ::tolower);

        std::string gender_proper;
        if (gender_lower == "male" || gender_lower == "m") {
            gender_proper = "Male";
        } else if (gender_lower == "female" || gender_lower == "f") {
            gender_proper = "Female";
        } else if (gender_lower == "neutral" || gender_lower == "n" || gender_lower == "neuter") {
            gender_proper = "Neuter";
        } else if (gender_lower == "nonbinary" || gender_lower == "nb") {
            gender_proper = "Nonbinary";
        } else {
            ctx.send_error("Gender must be: male, female, neutral, or nonbinary.");
            return CommandResult::InvalidSyntax;
        }

        target->set_gender(gender_proper);
        ctx.send_success(fmt::format("{}'s gender set to {}.", target->name(), gender_proper));
        return CommandResult::Success;
    }

    if (field == "size") {
        std::string size_lower = value_str;
        std::transform(size_lower.begin(), size_lower.end(), size_lower.begin(), ::tolower);

        std::string size_proper;
        if (size_lower == "tiny" || size_lower == "t") {
            size_proper = "Tiny";
        } else if (size_lower == "small" || size_lower == "s") {
            size_proper = "Small";
        } else if (size_lower == "medium" || size_lower == "m") {
            size_proper = "Medium";
        } else if (size_lower == "large" || size_lower == "l") {
            size_proper = "Large";
        } else if (size_lower == "huge" || size_lower == "h") {
            size_proper = "Huge";
        } else if (size_lower == "giant" || size_lower == "gargantuan" || size_lower == "g") {
            size_proper = "Giant";
        } else {
            ctx.send_error("Size must be: tiny, small, medium, large, huge, or giant.");
            return CommandResult::InvalidSyntax;
        }

        target->set_size(size_proper);
        ctx.send_success(fmt::format("{}'s size set to {}.", target->name(), size_proper));
        return CommandResult::Success;
    }

    if (field == "title") {
        if (!target_player) {
            ctx.send_error("Only players have titles.");
            return CommandResult::InvalidTarget;
        }
        // Collect all remaining args as title
        std::string title;
        for (size_t i = value_arg_index; i < ctx.arg_count(); ++i) {
            if (!title.empty()) title += " ";
            title += std::string{ctx.arg(i)};
        }
        target_player->set_title(title);
        ctx.send_success(fmt::format("{}'s title set to: {}", target->name(), title));
        return CommandResult::Success;
    }

    // =========================================================================
    // PLAYER SPECIFIC PROPERTIES
    // =========================================================================
    if (field == "godlevel" || field == "god") {
        if (!target_player) {
            ctx.send_error("Only players have god level.");
            return CommandResult::InvalidTarget;
        }
        auto val = parse_number(value_str);
        if (!val) {
            ctx.send_error("God level must be a number (0=mortal, 1-5=immortal).");
            return CommandResult::InvalidSyntax;
        }
        int clamped = std::clamp(*val, 0, 5);
        target_player->set_god_level(clamped);
        ctx.send_success(fmt::format("{}'s god level set to {}.", target->name(), clamped));
        return CommandResult::Success;
    }

    if (field == "home" || field == "homeroom") {
        if (!target_player) {
            ctx.send_error("Only players have home rooms.");
            return CommandResult::InvalidTarget;
        }

        EntityId room_id;
        if (value_str.empty() || value_str == "here") {
            if (!ctx.room) {
                ctx.send_error("You're not in a valid room.");
                return CommandResult::InvalidState;
            }
            room_id = ctx.room->id();
        } else {
            auto room_id_opt = CommandParserUtils::parse_entity_id(value_str);
            if (!room_id_opt || !room_id_opt->is_valid()) {
                ctx.send_error("Invalid room ID.");
                return CommandResult::InvalidSyntax;
            }
            room_id = *room_id_opt;
        }

        auto room = WorldManager::instance().get_room(room_id);
        if (!room) {
            ctx.send_error(fmt::format("Room {}:{} does not exist.", room_id.zone_id(), room_id.local_id()));
            return CommandResult::InvalidTarget;
        }

        target_player->set_start_room(room_id);
        ctx.send_success(fmt::format("{}'s home room set to {}:{} ({}).",
            target->name(), room_id.zone_id(), room_id.local_id(), room->name()));
        return CommandResult::Success;
    }

    // =========================================================================
    // PLAYER FLAGS (BOOLEAN PREFERENCES)
    // =========================================================================
    auto set_player_flag = [&](PlayerFlag flag, std::string_view flag_name) -> Result<CommandResult> {
        if (!target_player) {
            ctx.send_error("Only players have preference flags.");
            return CommandResult::InvalidTarget;
        }

        std::string val_lower = value_str;
        std::transform(val_lower.begin(), val_lower.end(), val_lower.begin(), ::tolower);

        bool on = (val_lower == "on" || val_lower == "yes" || val_lower == "true" || val_lower == "1");
        bool off = (val_lower == "off" || val_lower == "no" || val_lower == "false" || val_lower == "0");

        if (!on && !off) {
            ctx.send_error(fmt::format("{} must be on/off, yes/no, or true/false.", flag_name));
            return CommandResult::InvalidSyntax;
        }

        target_player->set_player_flag(flag, on);
        ctx.send_success(fmt::format("{}'s {} flag set to {}.", target->name(), flag_name, on ? "ON" : "OFF"));
        return CommandResult::Success;
    };

    if (field == "brief") return set_player_flag(PlayerFlag::Brief, "brief");
    if (field == "compact") return set_player_flag(PlayerFlag::Compact, "compact");
    if (field == "autoloot") return set_player_flag(PlayerFlag::AutoLoot, "autoloot");
    if (field == "autogold") return set_player_flag(PlayerFlag::AutoGold, "autogold");
    if (field == "autosplit") return set_player_flag(PlayerFlag::AutoSplit, "autosplit");
    if (field == "autoexit") return set_player_flag(PlayerFlag::AutoExit, "autoexits");
    if (field == "autoassist") return set_player_flag(PlayerFlag::AutoAssist, "autoassist");
    if (field == "wimpy") return set_player_flag(PlayerFlag::Wimpy, "wimpy");
    if (field == "afk") return set_player_flag(PlayerFlag::Afk, "afk");
    if (field == "deaf") return set_player_flag(PlayerFlag::Deaf, "deaf");
    if (field == "notell" || field == "no_tell") return set_player_flag(PlayerFlag::NoTell, "no-tell");
    if (field == "nosummon" || field == "no_summon") return set_player_flag(PlayerFlag::NoSummon, "no-summon");
    if (field == "pk" || field == "pkenabled") return set_player_flag(PlayerFlag::PkEnabled, "pk");
    if (field == "holylight") return set_player_flag(PlayerFlag::HolyLight, "holylight");
    if (field == "showids") return set_player_flag(PlayerFlag::ShowIds, "show-ids");
    if (field == "showdice" || field == "dicerolls") return set_player_flag(PlayerFlag::ShowDiceRolls, "show-dice");
    if (field == "colorblind") return set_player_flag(PlayerFlag::ColorBlind, "colorblind");

    // =========================================================================
    // SKILL / ABILITY SETTING
    // =========================================================================
    if (field == "skill" || field == "spell" || field == "ability") {
        if (!target_player) {
            ctx.send_error("Only players have skills.");
            return CommandResult::InvalidTarget;
        }

        // Need: set skill <player> <skill_name> <value>
        // or: set <player> skill <skill_name> <value>
        // We already consumed field as "skill", so next args are skill name and value
        if (ctx.arg_count() < value_arg_index + 2) {
            ctx.send_error("Usage: set <player> skill <skill_name> <proficiency>");
            return CommandResult::InvalidSyntax;
        }

        std::string skill_name = std::string{ctx.arg(value_arg_index)};
        auto prof_val = parse_number(ctx.arg(value_arg_index + 1));
        if (!prof_val) {
            ctx.send_error("Proficiency must be a number (0-100).");
            return CommandResult::InvalidSyntax;
        }
        int proficiency = std::clamp(*prof_val, 0, 100);

        // Find the ability by name
        auto ability = target_player->find_ability_by_name(skill_name);
        if (!ability) {
            ctx.send_error(fmt::format("Ability '{}' not found.", skill_name));
            return CommandResult::InvalidTarget;
        }

        if (target_player->set_proficiency(ability->ability_id, proficiency)) {
            if (proficiency > 0 && !ability->known) {
                target_player->learn_ability(ability->ability_id);
            }
            ctx.send_success(fmt::format("{}'s {} proficiency set to {}%.",
                target->name(), ability->name, proficiency));
        } else {
            ctx.send_error(fmt::format("Failed to set proficiency for {}.", skill_name));
        }
        return CommandResult::Success;
    }

    // =========================================================================
    // CURRENCY (supports money strings like "3p 2g 5s 10c" or plain copper)
    // =========================================================================
    if (field == "wallet" || field == "wealth" || field == "money") {
        if (!target_player) {
            ctx.send_error("Only players have currency.");
            return CommandResult::InvalidTarget;
        }
        // Collect all remaining args as money string (e.g., "3p 2g 5s")
        std::string money_str;
        for (size_t i = value_arg_index; i < ctx.arg_count(); ++i) {
            if (!money_str.empty()) money_str += " ";
            money_str += std::string{ctx.arg(i)};
        }
        auto money = fiery::parse_money(money_str);
        if (!money) {
            ctx.send_error("Invalid money format. Use: 3p 2g 5s 10c (or plain copper amount)");
            return CommandResult::InvalidSyntax;
        }
        target_player->wallet() = *money;
        ctx.send_success(fmt::format("{}'s wallet set to {}.", target->name(),
            target_player->wallet().to_brief()));
        return CommandResult::Success;
    }

    if (field == "bank") {
        if (!target_player) {
            ctx.send_error("Only players have bank accounts.");
            return CommandResult::InvalidTarget;
        }
        // Collect all remaining args as money string
        std::string money_str;
        for (size_t i = value_arg_index; i < ctx.arg_count(); ++i) {
            if (!money_str.empty()) money_str += " ";
            money_str += std::string{ctx.arg(i)};
        }
        auto money = fiery::parse_money(money_str);
        if (!money) {
            ctx.send_error("Invalid money format. Use: 3p 2g 5s 10c (or plain copper amount)");
            return CommandResult::InvalidSyntax;
        }
        target_player->bank() = *money;
        ctx.send_success(fmt::format("{}'s bank set to {}.", target->name(),
            target_player->bank().to_brief()));
        return CommandResult::Success;
    }

    // Unknown field
    ctx.send_error(fmt::format("Unknown set field: '{}'. Type 'set' for help.", field));
    return CommandResult::InvalidSyntax;
}

// =============================================================================
// Touch Command (for touchstones and interactive objects)
// =============================================================================

Result<CommandResult> cmd_touch(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_usage("touch <object>");
        return CommandResult::InvalidSyntax;
    }

    // Find the object in the room
    auto target = ctx.find_object_target(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("You don't see '{}' here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if it's a touchstone
    if (target->is_touchstone()) {
        auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
        if (!player) {
            ctx.send_error("Only players can use touchstones.");
            return CommandResult::InvalidTarget;
        }

        if (!ctx.room) {
            ctx.send_error("You're not in a valid room.");
            return CommandResult::InvalidState;
        }

        player->set_start_room(ctx.room->id());
        ctx.send_success(fmt::format("You touch {} and feel a warm connection to this place.",
            target->display_name()));
        ctx.send_info(fmt::format("Your home is now set to {}:{} ({}).",
            ctx.room->id().zone_id(), ctx.room->id().local_id(), ctx.room->name()));
        ctx.send_to_room(fmt::format("{} touches {} reverently.",
            ctx.actor->display_name(), target->display_name()), true);

        return CommandResult::Success;
    }

    // Not a touchstone - just a regular touch
    ctx.send(fmt::format("You touch {}.", target->display_name()));
    ctx.send_to_room(fmt::format("{} touches {}.",
        ctx.actor->display_name(), target->display_name()), true);

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    // Status commands
    Commands()
        .command("affects", cmd_affects)
        .alias("aff")
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("skills", cmd_skills)
        .alias("sk")
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Trainer commands
    Commands()
        .command("practice", cmd_practice)
        .alias("prac")
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("train", cmd_train)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Toggle commands
    Commands()
        .command("wimpy", cmd_wimpy)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Social interaction commands
    // Note: afk has special message functionality so it's kept as a standalone command
    Commands()
        .command("afk", cmd_afk)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("title", cmd_title)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("description", cmd_description)
        .alias("desc")
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("toggle", cmd_toggle)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Consent/PvP commands
    Commands()
        .command("consent", cmd_consent)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("pk", cmd_pk)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Follower commands
    Commands()
        .command("call", cmd_call)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("order", cmd_order)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Class ability commands
    Commands()
        .command("subclass", cmd_subclass)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("shapechange", cmd_shapechange)
        .alias("shape")
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Communication commands
    Commands()
        .command("write", cmd_write)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Configuration commands
    Commands()
        .command("set", cmd_set)
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .description("Set various character and game properties")
        .usage("set <subcommand> [args...]")
        .help(
            "<b:yellow>Subcommands:</>\n"
            "  set home           - Set current room as your home\n"
            "  set home <player>  - Set current room as player's home\n"
            "  set home <player> <room_id> - Set specific room as player's home")
        .build();

    // Interactive object commands
    Commands()
        .command("touch", cmd_touch)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .description("Touch an object (use touchstones to set home)")
        .usage("touch <object>")
        .build();

    return Success();
}

} // namespace CharacterCommands
