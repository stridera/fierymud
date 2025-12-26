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
    // Check if already flying (position)
    if (ctx.actor->position() == Position::Flying) {
        ctx.send("You are already flying.");
        return CommandResult::InvalidState;
    }

    // Check if actor has the ability to fly
    bool can_fly = false;

    // Gods can always fly
    if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        if (player->is_god()) {
            can_fly = true;
        }
    }

    // Check for Flying effect (from spell, race ability, or equipment)
    if (ctx.actor->has_flag(ActorFlag::Flying)) {
        can_fly = true;
    }

    // TODO: Add race-based flying check when race system is implemented
    // TODO: Add equipment-based flying check (wings, flying carpet, etc.)

    if (!can_fly) {
        ctx.send_error("You don't have the ability to fly.");
        return CommandResult::InvalidState;
    }

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

    // Start flying - change position to flying
    ctx.actor->set_position(Position::Flying);

    ctx.send("You rise into the air and begin flying.");
    ctx.send_to_room(fmt::format("{} rises into the air and begins flying.", ctx.actor->display_name()), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_land(const CommandContext &ctx) {
    // Check if actually flying (position, not flag)
    if (ctx.actor->position() != Position::Flying) {
        ctx.send("You aren't flying.");
        return CommandResult::InvalidState;
    }

    // Land - change position back to standing
    ctx.actor->set_position(Position::Standing);

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