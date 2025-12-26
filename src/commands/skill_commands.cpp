#include "skill_commands.hpp"
#include "builtin_commands.hpp"

#include "core/ability_executor.hpp"
#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../text/string_utils.hpp"

#include <fmt/format.h>

namespace SkillCommands {

// =============================================================================
// Toggle Skill Helper - Uses data-driven ability system
// =============================================================================

/**
 * Execute a toggle skill command using the data-driven ability system.
 * Handles toggling on/off, prerequisite checks, and effect removal.
 * All logic that can be data-driven comes from abilities.json.
 */
static Result<CommandResult> execute_toggle_skill(
    const CommandContext &ctx,
    std::string_view skill_name,
    bool requires_sitting = false,
    bool blocked_in_combat = true) {

    // Look up the ability to check if it's a toggle
    auto& cache = FieryMUD::AbilityCache::instance();
    const auto* ability = cache.get_ability_by_name(skill_name);

    if (!ability) {
        ctx.send_error(fmt::format("Unknown ability: {}", skill_name));
        return CommandResult::InvalidTarget;
    }

    // Check if this ability's effect is currently active (toggle off)
    if (ctx.actor->has_effect(ability->name)) {
        // Remove the effect and its associated flag
        ctx.actor->remove_effect(ability->name);

        // Send toggle-off message from database if available, otherwise generic
        const auto* messages = cache.get_ability_messages(ability->id);
        if (messages && !messages->wearoff_to_target.empty()) {
            ctx.send(messages->wearoff_to_target);
        } else {
            ctx.send(fmt::format("You stop {}.", to_lowercase(ability->name)));
        }

        // Handle position change for meditate-like skills
        if (requires_sitting && ctx.actor->position() == Position::Sitting) {
            ctx.actor->set_position(Position::Standing);
        }

        return CommandResult::Success;
    }

    // Pre-requisite checks
    if (blocked_in_combat && ctx.actor->position() == Position::Fighting) {
        ctx.send_error(fmt::format("You can't {} while fighting!", skill_name));
        return CommandResult::InvalidState;
    }

    // Handle sitting requirement
    if (requires_sitting) {
        if (ctx.actor->position() != Position::Sitting &&
            ctx.actor->position() != Position::Resting) {
            ctx.send("You need to sit down first.");
            ctx.actor->set_position(Position::Sitting);
            ctx.send("You sit down.");
        }
    }

    // Execute via data-driven ability system
    return FieryMUD::execute_skill_command(ctx, skill_name, false, false);
}

// =============================================================================
// Stealth Skill Commands
// =============================================================================

Result<CommandResult> cmd_hide(const CommandContext &ctx) {
    return execute_toggle_skill(ctx, "hide", false, true);
}

Result<CommandResult> cmd_sneak(const CommandContext &ctx) {
    return execute_toggle_skill(ctx, "sneak", false, false);
}

Result<CommandResult> cmd_meditate(const CommandContext &ctx) {
    return execute_toggle_skill(ctx, "meditate", true, true);
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands()
        .command("hide", cmd_hide)
        .category("Skills")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("sneak", cmd_sneak)
        .category("Skills")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("meditate", cmd_meditate)
        .alias("med")
        .category("Skills")
        .privilege(PrivilegeLevel::Player)
        .build();

    return Success();
}

} // namespace SkillCommands
