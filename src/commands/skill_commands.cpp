#include "skill_commands.hpp"

#include <algorithm>
#include <random>
#include <unordered_map>

#include <fmt/format.h>

#include "commands/command_system.hpp"
#include "core/ability_executor.hpp"
#include "core/actor.hpp"
#include "core/logging.hpp"
#include "core/object.hpp"
#include "core/player.hpp"
#include "text/string_utils.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"

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
// Utility / Movement Skills
// =============================================================================

Result<CommandResult> cmd_visible(const CommandContext &ctx) {
    bool was_invisible = false;
    if (ctx.actor->has_flag(ActorFlag::Invisible)) {
        ctx.actor->set_flag(ActorFlag::Invisible, false);
        was_invisible = true;
    }
    if (ctx.actor->has_flag(ActorFlag::Hide)) {
        ctx.actor->set_flag(ActorFlag::Hide, false);
        was_invisible = true;
    }
    if (ctx.actor->has_flag(ActorFlag::Sneak)) {
        ctx.actor->set_flag(ActorFlag::Sneak, false);
        was_invisible = true;
    }
    ctx.actor->stats().concealment = 0;

    if (was_invisible) {
        ctx.send("You break your concealment and become visible.");
        ctx.send_to_room(fmt::format("{} fades into view.", ctx.actor->display_name()), true);
    } else {
        ctx.send("You are already visible.");
    }
    return CommandResult::Success;
}

Result<CommandResult> cmd_pick(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Pick which lock?");
        return CommandResult::InvalidSyntax;
    }

    // Map direction argument to Direction
    std::string dir_arg = to_lower(std::string(ctx.arg(0)));
    using Dir = Direction;
    static const std::unordered_map<std::string, Direction> dir_map = {
        {"north", Dir::North}, {"n", Dir::North}, {"east", Dir::East}, {"e", Dir::East},
        {"south", Dir::South}, {"s", Dir::South}, {"west", Dir::West}, {"w", Dir::West},
        {"up", Dir::Up},       {"u", Dir::Up},    {"down", Dir::Down}, {"d", Dir::Down},
    };

    auto dir_it = dir_map.find(dir_arg);
    if (dir_it == dir_map.end()) {
        ctx.send_error("Pick the lock in which direction?");
        return CommandResult::InvalidSyntax;
    }
    Direction direction = dir_it->second;

    if (!ctx.room || !ctx.room->has_exit(direction)) {
        ctx.send_error("There's no door there.");
        return CommandResult::InvalidTarget;
    }

    auto *exit = ctx.room->get_exit(direction);
    if (!exit || !exit->has_door) {
        ctx.send_error("There's no door there.");
        return CommandResult::InvalidTarget;
    }
    if (!exit->is_locked) {
        ctx.send_error("It's not locked.");
        return CommandResult::InvalidState;
    }

    // Skill check: base 40% + DEX bonus
    int success_chance = 40 + (ctx.actor->stats().dexterity - 10) / 2 * 5;
    success_chance = std::clamp(success_chance, 5, 90);

    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> roll(1, 100);

    if (roll(gen) > success_chance) {
        ctx.send("You fail to pick the lock.");
        return CommandResult::Success;
    }

    auto *mutable_exit = ctx.room->get_exit_mutable(direction);
    if (mutable_exit) {
        mutable_exit->is_locked = false;
    }
    ctx.send("*click* You pick the lock.");
    ctx.send_to_room(fmt::format("{} picks a lock.", ctx.actor->display_name()), true);
    return CommandResult::Success;
}

Result<CommandResult> cmd_hunt(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Hunt for whom?");
        return CommandResult::InvalidSyntax;
    }
    // Hunt shows direction to a target mob/player
    ctx.send(fmt::format("You begin hunting for {}...", ctx.arg(0)));
    ctx.send("You sense a trail leading somewhere nearby.");
    return CommandResult::Success;
}

Result<CommandResult> cmd_track(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Track whom?");
        return CommandResult::InvalidSyntax;
    }
    ctx.send(fmt::format("You search for tracks left by {}...", ctx.arg(0)));
    ctx.send("You find some faint tracks.");
    return CommandResult::Success;
}

Result<CommandResult> cmd_steal(const CommandContext &ctx) {
    if (ctx.arg_count() < 2) {
        ctx.send_error("Steal what from whom?");
        return CommandResult::InvalidSyntax;
    }
    auto target = ctx.find_actor_target(ctx.arg(1));
    if (!target) {
        ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(1)));
        return CommandResult::InvalidTarget;
    }
    if (target == ctx.actor) {
        ctx.send_error("Steal from yourself? That's... creative.");
        return CommandResult::InvalidTarget;
    }

    // Skill check
    int success_chance = 30 + (ctx.actor->stats().dexterity - 10) / 2 * 5;
    success_chance = std::clamp(success_chance, 5, 80);

    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> roll(1, 100);

    if (roll(gen) > success_chance) {
        ctx.send(fmt::format("You fail to steal from {}!", target->display_name()));
        ctx.send_to_actor(target, fmt::format("{} tried to steal from you!", ctx.actor->display_name()));
        return CommandResult::Success;
    }

    ctx.send(fmt::format("You steal from {}!", target->display_name()));
    return CommandResult::Success;
}

Result<CommandResult> cmd_conceal(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "conceal", false, false);
}

Result<CommandResult> cmd_douse(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "douse", false, false);
}

Result<CommandResult> cmd_palm(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "palm", false, false);
}

Result<CommandResult> cmd_walk(const CommandContext &ctx) {
    // Toggle walk mode (slower but quieter movement)
    if (ctx.actor->has_flag(ActorFlag::Sneak)) {
        ctx.send("You are already moving quietly while sneaking.");
        return CommandResult::Success;
    }
    ctx.send("You begin walking carefully.");
    return CommandResult::Success;
}

Result<CommandResult> cmd_drag(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "drag", false, false);
}

Result<CommandResult> cmd_stow(const CommandContext &ctx) {
    // Quickly sheathe wielded weapon
    auto *player = dynamic_cast<Player *>(ctx.actor.get());
    if (!player) {
        ctx.send_error("Only players can stow weapons.");
        return CommandResult::InvalidState;
    }
    auto weapon = player->equipment().get_equipped(EquipSlot::Wield);
    if (!weapon) {
        ctx.send_error("You aren't wielding anything.");
        return CommandResult::InvalidState;
    }
    // Remove from equipment and add to inventory
    player->equipment().unequip_item(EquipSlot::Wield);
    player->inventory().add_item(weapon);
    ctx.send(fmt::format("You quickly stow {}.", weapon->short_description()));
    ctx.send_to_room(fmt::format("{} quickly stows a weapon.", ctx.actor->display_name()), true);
    return CommandResult::Success;
}

Result<CommandResult> cmd_tame(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "tame", true, false);
}

Result<CommandResult> cmd_lure(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "lure", true, false);
}

Result<CommandResult> cmd_corner(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "corner", true, false);
}

Result<CommandResult> cmd_firstaid(const CommandContext &ctx) {
    if (ctx.actor->position() == Position::Fighting) {
        ctx.send_error("You can't administer first aid while fighting!");
        return CommandResult::InvalidState;
    }
    return FieryMUD::execute_skill_command(ctx, "first aid", false, false);
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

    // Utility / movement skills
    Commands()
        .command("visible", cmd_visible)
        .alias("vis")
        .category("Skills")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands().command("pick", cmd_pick).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("hunt", cmd_hunt).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("track", cmd_track).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("steal", cmd_steal).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("conceal", cmd_conceal).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("douse", cmd_douse).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("palm", cmd_palm).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("walk", cmd_walk).category("Movement").privilege(PrivilegeLevel::Player).build();

    Commands().command("drag", cmd_drag).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("stow", cmd_stow).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("tame", cmd_tame).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("lure", cmd_lure).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("corner", cmd_corner).category("Skills").privilege(PrivilegeLevel::Player).build();

    Commands().command("firstaid", cmd_firstaid).category("Skills").privilege(PrivilegeLevel::Player).build();

    return Success();
}

} // namespace SkillCommands
