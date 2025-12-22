#include "movement_commands.hpp"
#include "builtin_commands.hpp"

#include "../core/actor.hpp"
#include "../world/room.hpp"

namespace MovementCommands {

// =============================================================================
// Movement Commands
// =============================================================================

Result<CommandResult> cmd_north(const CommandContext &ctx) { 
    return BuiltinCommands::Helpers::execute_movement(ctx, Direction::North); 
}

Result<CommandResult> cmd_south(const CommandContext &ctx) { 
    return BuiltinCommands::Helpers::execute_movement(ctx, Direction::South); 
}

Result<CommandResult> cmd_east(const CommandContext &ctx) { 
    return BuiltinCommands::Helpers::execute_movement(ctx, Direction::East); 
}

Result<CommandResult> cmd_west(const CommandContext &ctx) { 
    return BuiltinCommands::Helpers::execute_movement(ctx, Direction::West); 
}

Result<CommandResult> cmd_up(const CommandContext &ctx) { 
    return BuiltinCommands::Helpers::execute_movement(ctx, Direction::Up); 
}

Result<CommandResult> cmd_down(const CommandContext &ctx) { 
    return BuiltinCommands::Helpers::execute_movement(ctx, Direction::Down); 
}

Result<CommandResult> cmd_exits(const CommandContext &ctx) {
    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    std::string exits = BuiltinCommands::Helpers::format_exits(ctx.room);
    ctx.send_line(exits);

    return CommandResult::Success;
}

// =============================================================================
// Position Commands
// =============================================================================

Result<CommandResult> cmd_fly(const CommandContext &ctx) {
    // Check if already flying
    if (ctx.actor->has_flag(ActorFlag::Flying)) {
        ctx.send("You are already flying.");
        return CommandResult::InvalidState;
    }

    // Check if actor has the ability to fly (magic, race, item, etc.)
    // TODO: Add proper fly ability check (spell effect, race, equipment)
    // For now, allow anyone to fly for testing purposes

    // Check if in combat
    if (ctx.actor->is_fighting()) {
        ctx.send_error("You can't fly while fighting!");
        return CommandResult::InvalidState;
    }

    // Check position - must be standing
    if (ctx.actor->position() != Position::Standing) {
        ctx.send_error("You need to stand up first.");
        return CommandResult::InvalidState;
    }

    // Start flying
    ctx.actor->set_flag(ActorFlag::Flying, true);

    ctx.send("You rise into the air and begin flying.");
    ctx.send_to_room(fmt::format("{} rises into the air and begins flying.", ctx.actor->display_name()), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_land(const CommandContext &ctx) {
    // Check if actually flying
    if (!ctx.actor->has_flag(ActorFlag::Flying)) {
        ctx.send("You aren't flying.");
        return CommandResult::InvalidState;
    }

    // Stop flying
    ctx.actor->set_flag(ActorFlag::Flying, false);

    ctx.send("You gently descend and land on the ground.");
    ctx.send_to_room(fmt::format("{} gently descends and lands on the ground.", ctx.actor->display_name()), true);

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands()
        .command("north", cmd_north)
        .alias("n")
        .category("Movement")
        .description("Move north")
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("south", cmd_south)
        .alias("s")
        .category("Movement")
        .description("Move south")
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("east", cmd_east)
        .alias("e")
        .category("Movement")
        .description("Move east")
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("west", cmd_west)
        .alias("w")
        .category("Movement")
        .description("Move west")
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("up", cmd_up)
        .alias("u")
        .category("Movement")
        .description("Move up")
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("down", cmd_down)
        .alias("d")
        .category("Movement")
        .description("Move down")
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("exits", cmd_exits)
        .category("Movement")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("fly", cmd_fly)
        .category("Movement")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("land", cmd_land)
        .category("Movement")
        .privilege(PrivilegeLevel::Player)
        .build();

    return Success();
}

} // namespace MovementCommands