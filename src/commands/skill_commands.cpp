#include "skill_commands.hpp"

#include <algorithm>

#include <fmt/format.h>

#include "core/ability_executor.hpp"
#include "core/actor.hpp"
#include "core/logging.hpp"
#include "text/string_utils.hpp"

namespace SkillCommands {

// =============================================================================
// Toggle Skill Helper - Uses data-driven ability system
// =============================================================================

/**
 * Execute a toggle skill command using the data-driven ability system.
 * Handles toggling on/off, prerequisite checks, and effect removal.
 * All logic that can be data-driven comes from abilities.json.
 */
static Result<CommandResult> execute_toggle_skill(const CommandContext &ctx, std::string_view skill_name,
                                                  bool requires_sitting = false, bool blocked_in_combat = true) {

    // Look up the ability to check if it's a toggle
    auto &cache = FieryMUD::AbilityCache::instance();
    const auto *ability = cache.get_ability_by_name(skill_name);

    if (!ability) {
        ctx.send_error(fmt::format("Unknown ability: {}", skill_name));
        return CommandResult::InvalidTarget;
    }

    // Check if this ability's effect is currently active (toggle off)
    if (ctx.actor->has_effect(ability->name)) {
        // Remove the effect and its associated flag
        ctx.actor->remove_effect(ability->name);

        // Send toggle-off message from database if available, otherwise generic
        const auto *messages = cache.get_ability_messages(ability->id);
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
        if (ctx.actor->position() != Position::Sitting && ctx.actor->position() != Position::Resting) {
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

Result<CommandResult> cmd_hide(const CommandContext &ctx) { return execute_toggle_skill(ctx, "hide", false, true); }

Result<CommandResult> cmd_sneak(const CommandContext &ctx) { return execute_toggle_skill(ctx, "sneak", false, false); }

Result<CommandResult> cmd_meditate(const CommandContext &ctx) {
    return execute_toggle_skill(ctx, "meditate", true, true);
}

// =============================================================================
// Effect Management Commands
// =============================================================================

Result<CommandResult> cmd_cancel(const CommandContext &ctx) {
    // Cancel (remove) a non-permanent buff from yourself
    const auto &effects = ctx.actor->active_effects();

    // Filter to only non-permanent effects
    std::vector<const ActiveEffect *> cancellable;
    for (const auto &effect : effects) {
        if (!effect.is_permanent()) {
            cancellable.push_back(&effect);
        }
    }

    if (ctx.arg_count() == 0) {
        // No argument - list cancellable effects
        if (cancellable.empty()) {
            ctx.send("You have no active effects that can be cancelled.");
            return CommandResult::Success;
        }

        ctx.send("You can cancel the following effects:");
        for (const auto *effect : cancellable) {
            std::string duration_str;
            if (effect->duration_hours > 0) {
                int hours = static_cast<int>(effect->duration_hours);
                int minutes = static_cast<int>((effect->duration_hours - hours) * 60);
                if (hours > 0) {
                    duration_str = fmt::format(" ({} hour{}, {} min remaining)", hours, hours == 1 ? "" : "s", minutes);
                } else {
                    duration_str = fmt::format(" ({} min remaining)", minutes);
                }
            }
            ctx.send(fmt::format("  - {}{}", effect->name, duration_str));
        }
        ctx.send("Usage: cancel <effect name>");
        return CommandResult::Success;
    }

    // Find the effect to cancel
    std::string_view target_name = ctx.arg(0);
    std::string search_lower = std::string(target_name);
    std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

    const ActiveEffect *target_effect = nullptr;
    for (const auto *effect : cancellable) {
        std::string effect_lower = effect->name;
        std::transform(effect_lower.begin(), effect_lower.end(), effect_lower.begin(), ::tolower);

        // Support partial matching
        if (effect_lower.find(search_lower) != std::string::npos) {
            target_effect = effect;
            break;
        }
    }

    if (!target_effect) {
        // Check if they're trying to cancel a permanent effect
        for (const auto &effect : effects) {
            if (effect.is_permanent()) {
                std::string effect_lower = effect.name;
                std::transform(effect_lower.begin(), effect_lower.end(), effect_lower.begin(), ::tolower);
                if (effect_lower.find(search_lower) != std::string::npos) {
                    ctx.send_error(fmt::format("'{}' is a permanent effect and cannot be cancelled.", effect.name));
                    return CommandResult::InvalidTarget;
                }
            }
        }

        ctx.send_error(fmt::format("You don't have an effect called '{}'.", target_name));
        return CommandResult::InvalidTarget;
    }

    // Store the name before removing
    std::string effect_name = target_effect->name;

    // Remove the effect
    ctx.actor->remove_effect(effect_name);

    ctx.send(fmt::format("You cancel the {} effect.", effect_name));
    ctx.send_to_room(fmt::format("{}'s {} effect fades away.", ctx.actor->display_name(), effect_name), true);

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands().command("hide", cmd_hide).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("sneak", cmd_sneak).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands()
        .command("meditate", cmd_meditate)
        .alias("med")
        .category("Skills")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands().command("cancel", cmd_cancel).category("Skills").privilege(PrivilegeLevel::Player).build();

    return Success();
}

} // namespace SkillCommands
