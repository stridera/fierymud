/***************************************************************************
 *   File: src/commands/movement_commands.cpp              Part of FieryMUD *
 *  Usage: Movement command implementations                                  *
 *                                                                           *
 *  All rights reserved.  See license.doc for complete information.         *
 *                                                                           *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "movement_commands.hpp"
#include "builtin_commands.hpp"

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

} // namespace MovementCommands