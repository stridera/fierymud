#include "magic_commands.hpp"

#include <algorithm>
#include <map>

#include <fmt/format.h>

#include "commands/command_context.hpp"
#include "commands/command_system.hpp"
#include "core/ability_executor.hpp"
#include "core/actor.hpp"
#include "core/object.hpp"
#include "core/player.hpp"
#include "core/spell_system.hpp"
#include "database/connection_pool.hpp"
#include "world/room.hpp"

namespace MagicCommands {

// =============================================================================
// Helper Functions
// =============================================================================

// Helper to describe proficiency level
static std::string_view proficiency_description(int proficiency) {
    if (proficiency >= 95)
        return "(superb)";
    if (proficiency >= 85)
        return "(excellent)";
    if (proficiency >= 75)
        return "(very good)";
    if (proficiency >= 65)
        return "(good)";
    if (proficiency >= 55)
        return "(fair)";
    if (proficiency >= 45)
        return "(average)";
    if (proficiency >= 35)
        return "(below avg)";
    if (proficiency >= 25)
        return "(poor)";
    if (proficiency >= 15)
        return "(very poor)";
    return "(awful)";
}

// =============================================================================
// Spell Management Commands
// =============================================================================

// Sphere color codes (matching legacy)
static std::string get_sphere_color(std::string_view sphere) {
    std::string sphere_lower;
    sphere_lower.reserve(sphere.size());
    for (char c : sphere) {
        sphere_lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (sphere_lower == "fire")
        return "\033[31m"; // red
    if (sphere_lower == "water")
        return "\033[34m"; // blue
    if (sphere_lower == "earth")
        return "\033[33m"; // yellow
    if (sphere_lower == "air")
        return "\033[36m"; // cyan
    if (sphere_lower == "healing")
        return "\033[32m"; // green
    if (sphere_lower == "protection")
        return "\033[34m"; // blue
    if (sphere_lower == "enchantment")
        return "\033[35m"; // magenta
    if (sphere_lower == "summoning")
        return "\033[35m"; // magenta
    if (sphere_lower == "divination")
        return "\033[36m"; // cyan
    if (sphere_lower == "necromancy")
        return "\033[35m"; // magenta
    return "\033[35m";     // default: magenta (generic)
}

Result<CommandResult> cmd_spells(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players have spells.");
        return CommandResult::InvalidState;
    }

    // Get known abilities and filter for spells
    auto known_abilities = player->get_known_abilities();
    std::vector<const LearnedAbility *> spells;
    for (const auto *ability : known_abilities) {
        if (ability->type == "SPELL") {
            spells.push_back(ability);
        }
    }

    if (spells.empty()) {
        ctx.send("You do not know any spells.");
        return CommandResult::Success;
    }

    // Group spells by circle
    std::map<int, std::vector<const LearnedAbility *>> spells_by_circle;
    for (const auto *spell : spells) {
        spells_by_circle[spell->circle].push_back(spell);
    }

    // Sort spells within each circle by name
    for (auto &[circle, circle_spells] : spells_by_circle) {
        std::sort(circle_spells.begin(), circle_spells.end(),
                  [](const LearnedAbility *a, const LearnedAbility *b) { return a->plain_name < b->plain_name; });
    }

    std::string output;
    output += "You know of the following spells:\n\n";

    // ANSI codes
    constexpr std::string_view BLUE_BOLD = "\033[1;34m";
    constexpr std::string_view RESET = "\033[0m";

    // Display spells grouped by circle (matching legacy format)
    // Legacy format: "Circle  N: spell_name (padded)    sphere_name (colored)"
    for (const auto &[circle, circle_spells] : spells_by_circle) {
        bool first_in_circle = true;
        for (const auto *spell : circle_spells) {
            // Circle header on first spell, spaces on subsequent
            if (first_in_circle) {
                output += fmt::format("{}Circle {:>2}:{} ", BLUE_BOLD, circle, RESET);
                first_in_circle = false;
            } else {
                output += "           "; // 11 spaces to align with "Circle XX: "
            }

            // Spell name with color codes (use name, not plain_name)
            // Since name has ANSI codes, we can't use fmt padding - just output it
            output += spell->name;

            // Sphere with color
            if (!spell->sphere.empty()) {
                std::string sphere_lower = spell->sphere;
                std::transform(sphere_lower.begin(), sphere_lower.end(), sphere_lower.begin(), ::tolower);
                std::string sphere_color = get_sphere_color(spell->sphere);
                output += fmt::format(" {}{}{}", sphere_color, sphere_lower, RESET);
            }
            output += "\n";
        }
    }

    ctx.send(output);

    return CommandResult::Success;
}

Result<CommandResult> cmd_memorize(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can view spell slots.");
        return CommandResult::InvalidState;
    }

    // Get available circles
    auto circles = player->spell_slots().get_available_circles();

    if (circles.empty()) {
        ctx.send("You have no spell slots.");
        ctx.send("Only spellcasters have spell slots.");
        return CommandResult::Success;
    }

    // Build output
    std::string output;
    output += "--- Spell Slots ---\n";

    // Get restoration queue for timing info
    const auto &queue = player->spell_slots().restoration_queue();

    // Display each circle
    for (int circle : circles) {
        auto [current, max] = player->get_spell_slot_info(circle);
        int restoring = player->spell_slots().get_restoring_count(circle);

        if (restoring > 0) {
            // Calculate time remaining for slots of this circle in the queue
            std::string time_info;
            int position = 0;
            int cumulative_ticks = 0;
            int focus_rate = player->get_spell_restore_rate();

            for (const auto &entry : queue) {
                if (entry.circle == circle) {
                    // Time until this slot restores depends on queue position
                    int ticks_for_this = entry.ticks_remaining;
                    if (position == 0) {
                        // Front of queue - just use remaining ticks
                        int seconds = (ticks_for_this + focus_rate - 1) / focus_rate;
                        if (!time_info.empty())
                            time_info += ", ";
                        time_info += fmt::format("{}s", seconds);
                    } else {
                        // Behind other entries - estimate based on position
                        int seconds = (ticks_for_this + cumulative_ticks + focus_rate - 1) / focus_rate;
                        if (!time_info.empty())
                            time_info += ", ";
                        time_info += fmt::format("~{}s", seconds);
                    }
                }
                cumulative_ticks += entry.ticks_remaining;
                position++;
            }

            output += fmt::format("  Circle {}: {}/{} available, {} restoring ({})\n", circle, current, max, restoring,
                                  time_info);
        } else {
            output += fmt::format("  Circle {}: {}/{} available\n", circle, current, max);
        }
    }

    // Show meditation status
    if (player->is_meditating()) {
        output += "\nYou are meditating. (x2 restoration speed)\n";
    } else if (player->spell_slots().get_total_restoring() > 0) {
        output += "\nTip: Use 'meditate' to restore slots faster.\n";
    }

    output += "--- End of Spell Slots ---";
    ctx.send(output);

    return CommandResult::Success;
}

Result<CommandResult> cmd_forget(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can forget spells.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Forget what spell?");
        ctx.send("Usage: forget <spell name>");
        return CommandResult::Success;
    }

    std::string spell_name = std::string(ctx.command.full_argument_string);
    ctx.send(fmt::format("You clear '{}' from your mind.", spell_name));
    ctx.send("Note: Memorization system not yet fully implemented.");

    return CommandResult::Success;
}

// =============================================================================
// Skill/Ability Display Commands
// =============================================================================

Result<CommandResult> cmd_abilities(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players have abilities.");
        return CommandResult::InvalidState;
    }

    // Get known abilities from player object (loaded at login)
    auto known_abilities = player->get_known_abilities();

    if (known_abilities.empty()) {
        ctx.send("--- Your Abilities ---");
        ctx.send("You have not learned any abilities yet.");
        ctx.send("Use 'practice' at a trainer to learn new skills.");
        ctx.send("--- End of Abilities ---");
        return CommandResult::Success;
    }

    // Sort by type then by name
    std::sort(known_abilities.begin(), known_abilities.end(), [](const LearnedAbility *a, const LearnedAbility *b) {
        if (a->type != b->type)
            return a->type < b->type;
        return a->plain_name < b->plain_name;
    });

    // Display abilities grouped by type
    std::string output;
    output += "--- Your Abilities ---\n";

    std::string current_type;
    for (const auto *ability : known_abilities) {
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
        output += fmt::format("  {:<30} {:12} {:3}%\n", ability->name, prof_desc, display_prof);
    }

    output += "\n--- End of Abilities ---";
    ctx.send(output);

    return CommandResult::Success;
}

Result<CommandResult> cmd_innate(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players have innate abilities.");
        return CommandResult::InvalidState;
    }

    // Query race abilities from the database
    std::string race_str{player->race()};
    auto race_result = ConnectionPool::instance().execute(
        [&race_str](pqxx::work &txn) -> Result<std::vector<std::pair<std::string, int>>> {
            std::vector<std::pair<std::string, int>> abilities;
            auto rows = txn.exec_params(
                R"(SELECT a."name", ra."bonus" FROM "RaceAbilities" ra
                   JOIN "Ability" a ON a."id" = ra."ability_id"
                   WHERE ra."race" = $1
                   ORDER BY a."name")",
                race_str);
            for (const auto &row : rows) {
                abilities.emplace_back(row[0].as<std::string>(), row[1].as<int>());
            }
            return abilities;
        });

    ctx.send("--- Innate Abilities ---");
    ctx.send("");

    ctx.send(fmt::format("Racial Abilities ({}):", race_str));
    if (race_result && !race_result->empty()) {
        for (const auto &[name, bonus] : *race_result) {
            if (bonus > 0) {
                ctx.send(fmt::format("  {:<30} (+{} bonus)", name, bonus));
            } else {
                ctx.send(fmt::format("  {}", name));
            }
        }
    } else {
        ctx.send("  (None for your race)");
    }

    ctx.send("");

    // Show known SKILL-type abilities as class innates
    ctx.send(fmt::format("Class Abilities ({}):", player->player_class()));
    bool has_class_abilities = false;
    for (const auto &[id, ability] : player->get_abilities()) {
        if (ability.known && ability.type == "SKILL" && ability.proficiency > 0) {
            has_class_abilities = true;
            ctx.send(fmt::format("  {:<30} {:3}%", ability.name, ability.proficiency));
        }
    }
    if (!has_class_abilities) {
        ctx.send("  (None for your class)");
    }

    ctx.send("");
    ctx.send("--- End of Innate Abilities ---");

    return CommandResult::Success;
}

// =============================================================================
// Recovery Commands
// =============================================================================

Result<CommandResult> cmd_meditate(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can meditate.");
        return CommandResult::InvalidState;
    }

    // Check if already meditating - toggle off
    if (player->is_meditating()) {
        player->stop_meditation();
        ctx.send("You stop meditating and open your eyes.");
        ctx.send_to_room(fmt::format("{} opens their eyes and stops meditating.", player->display_name()), true);
        return CommandResult::Success;
    }

    // Check position
    if (ctx.actor->position() == Position::Fighting) {
        ctx.send_error("You can't meditate while fighting!");
        return CommandResult::InvalidState;
    }

    if (ctx.actor->position() == Position::Standing) {
        ctx.send("You sit down and close your eyes.");
        ctx.actor->set_position(Position::Sitting);
    } else if (ctx.actor->position() == Position::Sitting || ctx.actor->position() == Position::Resting) {
        ctx.send("You close your eyes and clear your mind.");
    } else {
        ctx.send_error("You need to be sitting or standing to meditate.");
        return CommandResult::InvalidState;
    }

    // Start meditating - doubles spell slot restoration speed
    player->start_meditation();

    int restoring = player->spell_slots().get_total_restoring();
    if (restoring > 0) {
        ctx.send("You enter a meditative state, focusing on restoring your magical energies.");
        ctx.send(fmt::format("({} spell slot{} restoring at 2x speed)", restoring, restoring == 1 ? "" : "s"));
    } else {
        ctx.send("You enter a meditative state, focusing your mind.");
    }

    ctx.send_to_room(fmt::format("{} closes their eyes and begins to meditate.", player->display_name()), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_concentrate(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can concentrate.");
        return CommandResult::InvalidState;
    }

    // Concentration is similar to meditate but focuses on spell power
    // It provides a temporary boost to spell_power while active

    if (ctx.actor->position() == Position::Fighting) {
        ctx.send_error("You can't concentrate while fighting!");
        return CommandResult::InvalidState;
    }

    // Toggle concentration — re-use meditation infrastructure
    if (player->is_meditating()) {
        player->stop_meditation();
        ctx.send("You break your concentration.");
        ctx.send_to_room(fmt::format("{} breaks their concentration.", player->display_name()), true);
        return CommandResult::Success;
    }

    if (ctx.actor->position() == Position::Standing) {
        ctx.send("You sit down and concentrate.");
        ctx.actor->set_position(Position::Sitting);
    } else if (ctx.actor->position() == Position::Sitting || ctx.actor->position() == Position::Resting) {
        ctx.send("You concentrate deeply.");
    } else {
        ctx.send_error("You need to be sitting or standing to concentrate.");
        return CommandResult::InvalidState;
    }

    // Boost focus stat temporarily while concentrating
    player->start_meditation();
    int focus_bonus = std::max(1, player->stats().intelligence / 4);
    player->stats().focus += focus_bonus;

    ctx.send(fmt::format("You enter a state of deep concentration. (Focus +{})", focus_bonus));
    ctx.send_to_room(fmt::format("{} enters a state of deep concentration.", player->display_name()), true);

    return CommandResult::Success;
}

// =============================================================================
// Learning and Creation Commands
// =============================================================================

Result<CommandResult> cmd_study(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can study spells.");
        return CommandResult::InvalidState;
    }

    ctx.send("Available Spell Slots:");
    ctx.send("Note: Spell slot system display not yet fully implemented.");
    ctx.send("Use 'memorize' to prepare your spells.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_scribe(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can scribe spells.");
        return CommandResult::InvalidState;
    }

    // Check fighting
    if (ctx.actor->is_fighting()) {
        ctx.send_error("If you wanna commit suicide just say so!");
        return CommandResult::InvalidState;
    }

    // Check position - must be sitting
    if (ctx.actor->position() != Position::Sitting) {
        ctx.send_error("You have to be sitting to scribe.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send_error("What spell do you want to scribe?");
        ctx.send("Usage: scribe '<spell name>'");
        return CommandResult::InvalidSyntax;
    }

    // Get the spell name (may be in quotes)
    std::string spell_name = ctx.args_from(0);

    // Strip quotes if present
    if (spell_name.size() >= 2 && spell_name.front() == '\'' && spell_name.back() == '\'') {
        spell_name = spell_name.substr(1, spell_name.size() - 2);
    }

    // Check if player knows this spell
    const auto &cache = FieryMUD::AbilityCache::instance();
    const auto *ability = cache.get_ability_by_name(spell_name);
    if (!ability) {
        ctx.send_error(fmt::format("You don't know any spell called '{}'.", spell_name));
        return CommandResult::InvalidTarget;
    }

    if (ability->type != WorldQueries::AbilityType::Spell) {
        ctx.send_error("You can only scribe spells, not skills.");
        return CommandResult::InvalidSyntax;
    }

    // Check for a scroll in inventory to write to
    bool has_scroll = false;
    for (const auto &obj : ctx.actor->inventory().get_all_items()) {
        if (obj && obj->type() == ObjectType::Scroll) {
            has_scroll = true;
            break;
        }
    }

    if (!has_scroll) {
        ctx.send_error("You need a blank scroll to scribe on.");
        return CommandResult::InvalidTarget;
    }

    // Check proficiency
    int prof = player->get_proficiency(ability->id);
    if (prof < 50) {
        ctx.send_error("You don't know that spell well enough to scribe it (need 50% proficiency).");
        return CommandResult::InsufficientPrivs;
    }

    ctx.send(fmt::format("You carefully scribe the words for '{}'.", ability->name));
    ctx.send_to_room(fmt::format("{} carefully scribes a spell onto a scroll.", player->display_name()), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_create(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can create items.");
        return CommandResult::InvalidState;
    }

    // Gnome racial ability — minor creation
    std::string race_lower{player->race()};
    std::transform(race_lower.begin(), race_lower.end(), race_lower.begin(), ::tolower);
    if (race_lower != "gnome") {
        ctx.send_error("Only gnomes possess the ability to tinker and create.");
        return CommandResult::InsufficientPrivs;
    }

    // Check for NOMAGIC room
    if (ctx.room && !ctx.room->allows_magic()) {
        ctx.send_error("A strange force prevents your tinkering here.");
        return CommandResult::InvalidState;
    }

    // Can't create while fighting
    if (ctx.actor->is_fighting()) {
        ctx.send_error("You can't concentrate on tinkering while fighting!");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send("You can create: food, water, light");
        ctx.send("Usage: create <item>");
        return CommandResult::InvalidSyntax;
    }

    std::string item_name{ctx.arg(0)};
    std::transform(item_name.begin(), item_name.end(), item_name.begin(), ::tolower);

    if (item_name == "food" || item_name == "rations") {
        ctx.send("You tinker together some gnomish trail rations from spare parts and scraps.");
        ctx.send_to_room(fmt::format("{} tinkers together some food.", player->display_name()), true);
    } else if (item_name == "water" || item_name == "drink") {
        ctx.send("You create a small gnomish waterskin filled with fresh water.");
        ctx.send_to_room(fmt::format("{} creates a container of water.", player->display_name()), true);
    } else if (item_name == "light" || item_name == "torch" || item_name == "lamp") {
        ctx.send("You tinker together a small glowing gnomish lamp.");
        ctx.send_to_room(fmt::format("{} creates a small light.", player->display_name()), true);
    } else {
        ctx.send_error(fmt::format("You don't know how to create '{}'.", item_name));
        ctx.send("You can create: food, water, light");
        return CommandResult::InvalidSyntax;
    }

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    // Spell management
    Commands().command("spells", cmd_spells).category("Magic").privilege(PrivilegeLevel::Player).build();

    Commands()
        .command("memorize", cmd_memorize)
        .alias("mem")
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Priest flavor alias for spell slots
    Commands().command("pray", cmd_memorize).category("Magic").privilege(PrivilegeLevel::Player).build();

    // Generic alias for spell slots
    Commands().command("slots", cmd_memorize).category("Magic").privilege(PrivilegeLevel::Player).build();

    Commands().command("forget", cmd_forget).category("Magic").privilege(PrivilegeLevel::Player).build();

    // Skill/ability display
    Commands()
        .command("abilities", cmd_abilities)
        .alias("abil")
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands().command("innate", cmd_innate).category("Magic").privilege(PrivilegeLevel::Player).build();

    // Recovery commands
    Commands()
        .command("meditate", cmd_meditate)
        .alias("med")
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("concentrate", cmd_concentrate)
        .alias("conc")
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Learning and creation commands
    Commands()
        .command("study", cmd_study)
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .description("View your available spell slots")
        .build();

    Commands()
        .command("scribe", cmd_scribe)
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .description("Scribe a spell into your spellbook")
        .usage("scribe '<spell name>'")
        .usable_while_sitting(true)
        .usable_in_combat(false)
        .build();

    Commands()
        .command("create", cmd_create)
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .description("Create an item using magical abilities")
        .usage("create <item>")
        .build();

    return Success();
}

} // namespace MagicCommands
