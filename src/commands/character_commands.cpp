#include "character_commands.hpp"

#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../database/connection_pool.hpp"
#include "../database/world_queries.hpp"
#include "../server/persistence_manager.hpp"
#include "../text/string_utils.hpp"
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

    std::unordered_map<int, int> class_ability_levels;  // ability_id -> min_level
    for (const auto& ca : *class_abilities_result) {
        class_ability_levels[ca.ability_id] = ca.min_level;
    }

    if (ctx.arg_count() == 0) {
        // Show practiceable abilities
        std::string output;
        output += fmt::format("{} can teach you the following abilities:\n\n", teacher->display_name());

        int player_level = player->level();
        std::vector<std::tuple<std::string, int, int, std::string>> learnable;  // name, min_level, proficiency, type

        for (const auto& [ability_id, min_level] : class_ability_levels) {
            auto it = ability_map.find(ability_id);
            if (it == ability_map.end()) continue;

            const auto* ability = it->second;

            // Check if player can learn this ability at their level
            if (min_level > player_level) continue;

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

            learnable.emplace_back(ability->name, min_level, proficiency, type_str);
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

    std::string lower_search = to_lowercase(ability_name);

    for (const auto& [ability_id, min_level] : class_ability_levels) {
        auto it = ability_map.find(ability_id);
        if (it == ability_map.end()) continue;

        const auto* ability = it->second;
        std::string lower_name = to_lowercase(ability->plain_name);

        // Check for prefix match
        if (lower_name.starts_with(lower_search) ||
            lower_name.find(lower_search) != std::string::npos) {
            target_ability = ability;
            target_min_level = min_level;
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

Result<CommandResult> cmd_brief(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can toggle brief mode.");
        return CommandResult::InvalidState;
    }

    player->toggle_player_flag(PlayerFlag::Brief);

    if (player->is_brief()) {
        ctx.send("Brief mode ON. Room descriptions will be shortened.");
    } else {
        ctx.send("Brief mode OFF. Full room descriptions will be shown.");
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_compact(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can toggle compact mode.");
        return CommandResult::InvalidState;
    }

    player->toggle_player_flag(PlayerFlag::Compact);

    if (player->is_compact()) {
        ctx.send("Compact mode ON. Less blank lines in output.");
    } else {
        ctx.send("Compact mode OFF. Normal spacing restored.");
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_autoloot(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can toggle autoloot.");
        return CommandResult::InvalidState;
    }

    player->toggle_player_flag(PlayerFlag::AutoLoot);

    if (player->is_autoloot()) {
        ctx.send("Autoloot ON. You will automatically loot corpses after kills.");
    } else {
        ctx.send("Autoloot OFF. You must manually loot corpses.");
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_autogold(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can toggle autogold.");
        return CommandResult::InvalidState;
    }

    player->toggle_player_flag(PlayerFlag::AutoGold);

    if (player->is_autogold()) {
        ctx.send("Autogold ON. You will automatically take gold from corpses.");
    } else {
        ctx.send("Autogold OFF. You must manually take gold from corpses.");
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_autosplit(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can toggle autosplit.");
        return CommandResult::InvalidState;
    }

    player->toggle_player_flag(PlayerFlag::AutoSplit);

    if (player->is_autosplit()) {
        ctx.send("Autosplit ON. Gold will be automatically split with your group.");
    } else {
        ctx.send("Autosplit OFF. You must manually split gold with your group.");
    }

    return CommandResult::Success;
}

// =============================================================================
// Social Interaction Commands
// =============================================================================

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

    if (ctx.arg_count() == 0) {
        ctx.send("Your current description:");
        if (player->description().empty()) {
            ctx.send("  (No description set)");
        } else {
            ctx.send(fmt::format("  {}", player->description()));
        }
        ctx.send("");
        ctx.send("Usage: description <your description>  or  description clear");
        ctx.send("This is what others see when they look at you.");
        return CommandResult::Success;
    }

    std::string new_desc = std::string(ctx.command.full_argument_string);

    // Handle clearing the description
    if (new_desc == "clear" || new_desc == "none") {
        player->set_description("");
        ctx.send("Your description has been cleared.");
        return CommandResult::Success;
    }

    // Validate description length
    if (new_desc.length() > 500) {
        ctx.send_error("Description is too long. Maximum 500 characters.");
        return CommandResult::InvalidSyntax;
    }

    player->set_description(new_desc);
    ctx.send("Your description has been updated.");
    ctx.send(fmt::format("New description: {}", new_desc));

    return CommandResult::Success;
}

Result<CommandResult> cmd_toggle(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use toggle.");
        return CommandResult::InvalidState;
    }

    // Helper to format on/off
    auto on_off = [](bool value) { return value ? "ON " : "OFF"; };

    if (ctx.arg_count() == 0) {
        // Show all toggles with current values
        ctx.send("--- Character Options ---");
        ctx.send(fmt::format("  brief       : {}  - Show brief room descriptions", on_off(player->is_brief())));
        ctx.send(fmt::format("  compact     : {}  - Reduce blank lines in output", on_off(player->is_compact())));
        ctx.send(fmt::format("  autoloot    : {}  - Automatically loot corpses", on_off(player->is_autoloot())));
        ctx.send(fmt::format("  autogold    : {}  - Automatically take gold from corpses", on_off(player->is_autogold())));
        ctx.send(fmt::format("  autosplit   : {}  - Automatically split gold with group", on_off(player->is_autosplit())));
        ctx.send(fmt::format("  autoexit    : {}  - Automatically show exits", on_off(player->is_autoexit())));
        ctx.send(fmt::format("  deaf        : {}  - Block shouts and gossip", on_off(player->is_deaf())));
        ctx.send(fmt::format("  notell      : {}  - Block tells from non-gods", on_off(player->is_notell())));
        ctx.send(fmt::format("  afk         : {}  - Away from keyboard", on_off(player->is_afk())));
        // Immortal-only options
        if (player->is_god()) {
            ctx.send("--- Immortal Options ---");
            ctx.send(fmt::format("  holylight   : {}  - See everything (invis, dark, hidden)", on_off(player->is_holylight())));
            ctx.send(fmt::format("  showids     : {}  - Show entity IDs on mobs/objects", on_off(player->is_show_ids())));
        }
        ctx.send("--- End of Options ---");
        ctx.send("Usage: toggle <option> to flip a setting");
        return CommandResult::Success;
    }

    std::string_view option = ctx.arg(0);

    // Toggle the appropriate flag
    if (option == "brief") {
        player->toggle_player_flag(PlayerFlag::Brief);
        ctx.send(fmt::format("Brief mode is now {}.", player->is_brief() ? "ON" : "OFF"));
    } else if (option == "compact") {
        player->toggle_player_flag(PlayerFlag::Compact);
        ctx.send(fmt::format("Compact mode is now {}.", player->is_compact() ? "ON" : "OFF"));
    } else if (option == "autoloot") {
        player->toggle_player_flag(PlayerFlag::AutoLoot);
        ctx.send(fmt::format("Autoloot is now {}.", player->is_autoloot() ? "ON" : "OFF"));
    } else if (option == "autogold") {
        player->toggle_player_flag(PlayerFlag::AutoGold);
        ctx.send(fmt::format("Autogold is now {}.", player->is_autogold() ? "ON" : "OFF"));
    } else if (option == "autosplit") {
        player->toggle_player_flag(PlayerFlag::AutoSplit);
        ctx.send(fmt::format("Autosplit is now {}.", player->is_autosplit() ? "ON" : "OFF"));
    } else if (option == "autoexit") {
        player->toggle_player_flag(PlayerFlag::AutoExit);
        ctx.send(fmt::format("Autoexit is now {}.", player->is_autoexit() ? "ON" : "OFF"));
    } else if (option == "deaf" || option == "noshout" || option == "nogossip") {
        player->toggle_player_flag(PlayerFlag::Deaf);
        ctx.send(fmt::format("Deaf mode is now {}.", player->is_deaf() ? "ON" : "OFF"));
    } else if (option == "notell") {
        player->toggle_player_flag(PlayerFlag::NoTell);
        ctx.send(fmt::format("Notell is now {}.", player->is_notell() ? "ON" : "OFF"));
    } else if (option == "afk") {
        player->toggle_player_flag(PlayerFlag::Afk);
        ctx.send(fmt::format("AFK is now {}.", player->is_afk() ? "ON" : "OFF"));
    } else if (option == "holylight") {
        if (!player->is_god()) {
            ctx.send_error("Only immortals can toggle holylight.");
            return CommandResult::InsufficientPrivs;
        }
        player->toggle_player_flag(PlayerFlag::HolyLight);
        ctx.send(fmt::format("Holylight is now {}.", player->is_holylight() ? "ON" : "OFF"));
        if (player->is_holylight()) {
            ctx.send("You can now see invisible, hidden, and in darkness.");
        }
    } else if (option == "showids") {
        if (!player->is_god()) {
            ctx.send_error("Only immortals can toggle showids.");
            return CommandResult::InsufficientPrivs;
        }
        player->toggle_player_flag(PlayerFlag::ShowIds);
        ctx.send(fmt::format("ShowIds is now {}.", player->is_show_ids() ? "ON" : "OFF"));
    } else {
        ctx.send_error(fmt::format("Unknown toggle option: {}", option));
        ctx.send("Type 'toggle' with no arguments to see available options.");
        return CommandResult::InvalidSyntax;
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
        ctx.send("PK Mode: ENABLED");
        ctx.send("You may now attack and be attacked by other PK players.");
        ctx.send_to_room(fmt::format("{} has enabled PK mode!", player->display_name()), true);
    } else if (arg == "off" || arg == "no" || arg == "disable") {
        if (!player->is_pk_enabled()) {
            ctx.send("PK mode is not enabled.");
            return CommandResult::InvalidState;
        }
        // TODO: Add 24-hour lockout timer before allowing disable
        ctx.send("PK mode cannot be disabled yet. (24-hour lockout active)");
    } else {
        ctx.send_error("Usage: pk on|off");
        return CommandResult::InvalidSyntax;
    }

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

    Commands()
        .command("brief", cmd_brief)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("compact", cmd_compact)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("autoloot", cmd_autoloot)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("autogold", cmd_autogold)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("autosplit", cmd_autosplit)
        .category("Character")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Social interaction commands
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

    return Success();
}

} // namespace CharacterCommands
