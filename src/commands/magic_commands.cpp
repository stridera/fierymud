#include "magic_commands.hpp"

#include "../core/actor.hpp"
#include "../core/spell_system.hpp"
#include <algorithm>
#include <fmt/format.h>

namespace MagicCommands {

// =============================================================================
// Helper Functions
// =============================================================================

// Helper to describe proficiency level
static std::string_view proficiency_description(int proficiency) {
    if (proficiency >= 95) return "(superb)";
    if (proficiency >= 85) return "(excellent)";
    if (proficiency >= 75) return "(very good)";
    if (proficiency >= 65) return "(good)";
    if (proficiency >= 55) return "(fair)";
    if (proficiency >= 45) return "(average)";
    if (proficiency >= 35) return "(below avg)";
    if (proficiency >= 25) return "(poor)";
    if (proficiency >= 15) return "(very poor)";
    return "(awful)";
}

// =============================================================================
// Spell Management Commands
// =============================================================================

Result<CommandResult> cmd_spells(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players have spells.");
        return CommandResult::InvalidState;
    }

    // Get known abilities and filter for spells
    auto known_abilities = player->get_known_abilities();
    std::vector<const LearnedAbility*> spells;
    for (const auto* ability : known_abilities) {
        if (ability->type == "SPELL") {
            spells.push_back(ability);
        }
    }

    // Sort spells by name
    std::sort(spells.begin(), spells.end(),
        [](const LearnedAbility* a, const LearnedAbility* b) {
            return a->plain_name < b->plain_name;
        });

    std::string output;
    output += "--- Known Spells ---\n";

    // Check if player has any spell slots
    bool has_slots = false;
    for (int circle = 1; circle <= 9; circle++) {
        if (player->has_spell_slots(circle)) {
            has_slots = true;
            break;
        }
    }

    if (has_slots) {
        // Show spell slots
        output += "\nSpell Slots:\n";
        for (int circle = 1; circle <= 9; circle++) {
            if (player->has_spell_slots(circle)) {
                auto [current, max] = player->get_spell_slot_info(circle);
                output += fmt::format("  Circle {}: {}/{}\n", circle, current, max);
            }
        }
    }

    if (spells.empty()) {
        output += "\nNo spells learned yet.\n";
        output += "Use 'practice' at a guild to learn new spells.\n";
    } else {
        output += fmt::format("\nYou know {} spell{}:\n", spells.size(), spells.size() == 1 ? "" : "s");
        for (const auto* spell : spells) {
            // Proficiency stored as 0-1000, display as 0-100
            int display_prof = spell->proficiency / 10;
            std::string prof_desc(proficiency_description(display_prof));
            output += fmt::format("  {:<30} {:12} {:3}%\n",
                spell->name, prof_desc, display_prof);
        }
    }

    output += "--- End of Spells ---";
    ctx.send(output);

    return CommandResult::Success;
}

Result<CommandResult> cmd_memorize(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can memorize spells.");
        return CommandResult::InvalidState;
    }

    // TODO: Implement spell memorization system
    // This would involve:
    // - Checking if player knows the spell
    // - Adding to memorized spell list
    // - Time-based memorization process

    if (ctx.arg_count() == 0) {
        ctx.send("Memorize what spell?");
        ctx.send("Usage: memorize <spell name>");
        ctx.send("");
        ctx.send("Note: Spell memorization system not yet implemented.");
        return CommandResult::Success;
    }

    std::string spell_name = std::string(ctx.command.full_argument_string);

    // Check if spell exists in registry
    SpellRegistry& registry = SpellRegistry::instance();
    const Spell* spell = registry.find_spell(spell_name);

    if (!spell) {
        ctx.send_error(fmt::format("You don't know any spell called '{}'.", spell_name));
        return CommandResult::InvalidTarget;
    }

    ctx.send(fmt::format("You begin memorizing '{}'...", spell->name));
    ctx.send("Note: Memorization system not yet fully implemented.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_forget(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can forget spells.");
        return CommandResult::InvalidState;
    }

    // TODO: Remove spell from memorized list

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

Result<CommandResult> cmd_innate(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players have innate abilities.");
        return CommandResult::InvalidState;
    }

    // TODO: Query for race-based and class-based innate abilities

    ctx.send("--- Innate Abilities ---");
    ctx.send("");

    // Race-based abilities would go here
    ctx.send("Racial Abilities:");
    ctx.send("  (None for your race)");
    ctx.send("");

    // Class-based innate abilities
    ctx.send("Class Abilities:");
    ctx.send("  (None for your class)");
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

    // Check position
    if (ctx.actor->position() == Position::Fighting) {
        ctx.send_error("You can't meditate while fighting!");
        return CommandResult::InvalidState;
    }

    if (ctx.actor->position() == Position::Standing) {
        ctx.send("You sit down and begin to meditate.");
        ctx.actor->set_position(Position::Sitting);
    } else if (ctx.actor->position() == Position::Sitting ||
               ctx.actor->position() == Position::Resting) {
        ctx.send("You close your eyes and begin to meditate.");
    } else {
        ctx.send_error("You need to be sitting or standing to meditate.");
        return CommandResult::InvalidState;
    }

    // TODO: Set meditation flag for enhanced mana regen
    // TODO: Implement meditation skill check for effectiveness

    ctx.send("You enter a meditative state, focusing your mind.");
    ctx.send("Note: Enhanced mana regeneration not yet implemented.");
    ctx.send_to_room(fmt::format("{} closes their eyes and begins to meditate.",
                     player->display_name()), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_concentrate(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can concentrate.");
        return CommandResult::InvalidState;
    }

    // TODO: Concentration would improve spell effectiveness or reduce cast time
    // This is a toggle or timed state

    if (ctx.actor->position() == Position::Fighting) {
        ctx.send("You focus your concentration on the battle.");
        ctx.send("Note: Combat concentration bonuses not yet implemented.");
    } else {
        ctx.send("You focus your mind, preparing for spellcasting.");
        ctx.send("Note: Concentration system not yet implemented.");
    }

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    // Spell management
    Commands()
        .command("spells", cmd_spells)
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("memorize", cmd_memorize)
        .alias("mem")
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("forget", cmd_forget)
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Skill/ability display
    Commands()
        .command("abilities", cmd_abilities)
        .alias("abil")
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("innate", cmd_innate)
        .category("Magic")
        .privilege(PrivilegeLevel::Player)
        .build();

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

    return Success();
}

} // namespace MagicCommands
