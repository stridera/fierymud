// Character position management commands

#include "position_commands.hpp"

#include <fmt/format.h>

#include "commands/command_system.hpp"
#include "core/actor.hpp"
#include "core/logging.hpp"
#include "world/room.hpp"

namespace PositionCommands {

// =============================================================================
// Helper Functions
// =============================================================================

std::string_view position_name(Position pos) {
    switch (pos) {
    case Position::Dead:
        return "dead";
    case Position::Ghost:
        return "a ghost";
    case Position::Mortally_Wounded:
        return "mortally wounded";
    case Position::Incapacitated:
        return "incapacitated";
    case Position::Stunned:
        return "stunned";
    case Position::Sleeping:
        return "sleeping";
    case Position::Resting:
        return "resting";
    case Position::Sitting:
        return "sitting";
    case Position::Fighting:
        return "fighting";
    case Position::Standing:
        return "standing";
    default:
        return "in an unknown position";
    }
}

bool can_change_position(std::shared_ptr<Actor> actor, Position new_pos, std::string &reason) {
    if (!actor) {
        reason = "Invalid actor.";
        return false;
    }

    Position current = actor->position();

    // Can't change position if dead or ghost (except through special means)
    if (current == Position::Dead) {
        reason = "You can't do that while dead!";
        return false;
    }

    if (current == Position::Ghost) {
        reason = "You are a ghost! Use 'release' to return to the living.";
        return false;
    }

    // Can't change position if mortally wounded or incapacitated
    if (current == Position::Mortally_Wounded) {
        reason = "You are mortally wounded and cannot move!";
        return false;
    }

    if (current == Position::Incapacitated) {
        reason = "You are incapacitated and cannot move!";
        return false;
    }

    // Can't change position if stunned
    if (current == Position::Stunned) {
        reason = "You are stunned and cannot move!";
        return false;
    }

    // Can't voluntarily change position while fighting (except flee)
    if (current == Position::Fighting) {
        reason = "You can't do that while fighting! Try 'flee' instead.";
        return false;
    }

    // Position-specific transition rules
    // When sleeping, you must use "wake" to stand up - other commands don't work directly.
    // The wake command has its own validation and doesn't call this function.
    if (current == Position::Sleeping) {
        reason = "You need to wake up first!";
        return false;
    }

    // Validate the requested transition is possible
    // Most transitions are valid, but we check for nonsensical ones
    if (current == new_pos) {
        // Already in the desired position - commands handle their own messages
        // but we allow this to pass through so commands can show appropriate feedback
        return true;
    }

    return true;
}

// =============================================================================
// Basic Position Commands
// =============================================================================

Result<CommandResult> cmd_sit(const CommandContext &ctx) {
    std::string reason;
    if (!can_change_position(ctx.actor, Position::Sitting, reason)) {
        ctx.send_error(reason);
        return CommandResult::InvalidState;
    }

    Position current = ctx.actor->position();

    if (current == Position::Sitting) {
        ctx.send("You are already sitting.");
        return CommandResult::Success;
    }

    ctx.actor->set_position(Position::Sitting);

    if (current == Position::Standing) {
        ctx.send("You sit down.");
        ctx.send_to_room(fmt::format("{} sits down.", ctx.actor->display_name()), true);
    } else if (current == Position::Resting) {
        ctx.send("You stop resting and sit up.");
        ctx.send_to_room(fmt::format("{} stops resting and sits up.", ctx.actor->display_name()), true);
    } else {
        ctx.send("You sit down.");
        ctx.send_to_room(fmt::format("{} sits down.", ctx.actor->display_name()), true);
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_stand(const CommandContext &ctx) {
    std::string reason;
    if (!can_change_position(ctx.actor, Position::Standing, reason)) {
        ctx.send_error(reason);
        return CommandResult::InvalidState;
    }

    Position current = ctx.actor->position();

    if (current == Position::Standing) {
        ctx.send("You are already standing.");
        return CommandResult::Success;
    }

    // Interrupt concentration-based activities (like meditation)
    ctx.actor->interrupt_concentration();

    ctx.actor->set_position(Position::Standing);

    if (current == Position::Sitting) {
        ctx.send("You stand up.");
        ctx.send_to_room(fmt::format("{} stands up.", ctx.actor->display_name()), true);
    } else if (current == Position::Resting) {
        ctx.send("You stop resting and stand up.");
        ctx.send_to_room(fmt::format("{} stops resting and stands up.", ctx.actor->display_name()), true);
    } else {
        ctx.send("You stand up.");
        ctx.send_to_room(fmt::format("{} stands up.", ctx.actor->display_name()), true);
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_rest(const CommandContext &ctx) {
    std::string reason;
    if (!can_change_position(ctx.actor, Position::Resting, reason)) {
        ctx.send_error(reason);
        return CommandResult::InvalidState;
    }

    Position current = ctx.actor->position();

    if (current == Position::Resting) {
        ctx.send("You are already resting.");
        return CommandResult::Success;
    }

    ctx.actor->set_position(Position::Resting);

    if (current == Position::Standing) {
        ctx.send("You sit down and rest your tired bones.");
        ctx.send_to_room(fmt::format("{} sits down and rests.", ctx.actor->display_name()), true);
    } else if (current == Position::Sitting) {
        ctx.send("You lean back and rest.");
        ctx.send_to_room(fmt::format("{} leans back and rests.", ctx.actor->display_name()), true);
    } else {
        ctx.send("You rest your tired bones.");
        ctx.send_to_room(fmt::format("{} rests.", ctx.actor->display_name()), true);
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_sleep(const CommandContext &ctx) {
    std::string reason;
    if (!can_change_position(ctx.actor, Position::Sleeping, reason)) {
        ctx.send_error(reason);
        return CommandResult::InvalidState;
    }

    Position current = ctx.actor->position();

    if (current == Position::Sleeping) {
        ctx.send("You are already sound asleep.");
        return CommandResult::Success;
    }

    ctx.actor->set_position(Position::Sleeping);

    ctx.send("You lie down and drift off to sleep.");
    ctx.send_to_room(fmt::format("{} lies down and falls asleep.", ctx.actor->display_name()), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_wake(const CommandContext &ctx) {
    Position current = ctx.actor->position();

    // Check for invalid states (dead, ghost, etc.)
    if (current == Position::Dead) {
        ctx.send_error("You can't wake up from death!");
        return CommandResult::InvalidState;
    }

    if (current == Position::Ghost) {
        ctx.send_error("You are a ghost! Use 'release' to return to the living.");
        return CommandResult::InvalidState;
    }

    if (current != Position::Sleeping) {
        ctx.send("You are not asleep.");
        return CommandResult::Success;
    }

    // Wake target if specified
    if (ctx.arg_count() > 0) {
        auto target = ctx.find_actor_target(ctx.arg(0));
        if (!target) {
            ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }

        if (target->position() != Position::Sleeping) {
            ctx.send(fmt::format("{} is not asleep.", target->display_name()));
            return CommandResult::InvalidState;
        }

        target->set_position(Position::Sitting);
        ctx.send(fmt::format("You wake {} up.", target->display_name()));
        ctx.send_to_actor(target, fmt::format("{} wakes you up.", ctx.actor->display_name()));
        ctx.send_to_room(fmt::format("{} wakes {} up.", ctx.actor->display_name(), target->display_name()), true);

        return CommandResult::Success;
    }

    // Wake self
    ctx.actor->set_position(Position::Sitting);
    ctx.send("You wake up and sit up.");
    ctx.send_to_room(fmt::format("{} wakes up.", ctx.actor->display_name()), true);

    return CommandResult::Success;
}

// =============================================================================
// Extended Position Commands
// =============================================================================

Result<CommandResult> cmd_kneel(const CommandContext &ctx) {
    std::string reason;
    if (!can_change_position(ctx.actor, Position::Sitting, reason)) {
        ctx.send_error(reason);
        return CommandResult::InvalidState;
    }

    Position current = ctx.actor->position();

    // Kneeling is treated as a form of sitting for game mechanics
    if (current == Position::Sitting) {
        ctx.send("You are already in a lowered position.");
        return CommandResult::Success;
    }

    if (current == Position::Sleeping) {
        ctx.send_error("You need to wake up first!");
        return CommandResult::InvalidState;
    }

    ctx.actor->set_position(Position::Sitting);

    if (current == Position::Standing) {
        ctx.send("You kneel down.");
        ctx.send_to_room(fmt::format("{} kneels down.", ctx.actor->display_name()), true);
    } else if (current == Position::Resting) {
        ctx.send("You stop resting and kneel.");
        ctx.send_to_room(fmt::format("{} stops resting and kneels.", ctx.actor->display_name()), true);
    } else {
        ctx.send("You kneel down.");
        ctx.send_to_room(fmt::format("{} kneels down.", ctx.actor->display_name()), true);
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_recline(const CommandContext &ctx) {
    std::string reason;
    if (!can_change_position(ctx.actor, Position::Resting, reason)) {
        ctx.send_error(reason);
        return CommandResult::InvalidState;
    }

    Position current = ctx.actor->position();

    // Reclining is treated as a form of resting for game mechanics
    if (current == Position::Resting) {
        ctx.send("You are already reclining.");
        return CommandResult::Success;
    }

    if (current == Position::Sleeping) {
        ctx.send_error("You are already lying down. Use 'wake' to wake up first.");
        return CommandResult::InvalidState;
    }

    ctx.actor->set_position(Position::Resting);

    if (current == Position::Standing) {
        ctx.send("You recline comfortably.");
        ctx.send_to_room(fmt::format("{} reclines comfortably.", ctx.actor->display_name()), true);
    } else if (current == Position::Sitting) {
        ctx.send("You lean back and recline.");
        ctx.send_to_room(fmt::format("{} leans back and reclines.", ctx.actor->display_name()), true);
    } else {
        ctx.send("You recline.");
        ctx.send_to_room(fmt::format("{} reclines.", ctx.actor->display_name()), true);
    }

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands()
        .command("sit", cmd_sit)
        .category("Position")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .build();

    Commands()
        .command("stand", cmd_stand)
        .category("Position")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .build();

    Commands()
        .command("rest", cmd_rest)
        .category("Position")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .build();

    Commands()
        .command("sleep", cmd_sleep)
        .category("Position")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .build();

    Commands()
        .command("wake", cmd_wake)
        .category("Position")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sleeping(true)
        .build();

    Commands()
        .command("kneel", cmd_kneel)
        .category("Position")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .build();

    Commands()
        .command("recline", cmd_recline)
        .category("Position")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .build();

    return Success();
}

} // namespace PositionCommands
