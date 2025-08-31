/***************************************************************************
 *   File: src/commands/object_commands.hpp                Part of FieryMUD *
 *  Usage: Object interaction command declarations                           *
 *                                                                           *
 *  All rights reserved.  See license.doc for complete information.         *
 *                                                                           *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "command_system.hpp"

namespace ObjectCommands {

// Object interaction commands
Result<CommandResult> cmd_get(const CommandContext &ctx);
Result<CommandResult> cmd_drop(const CommandContext &ctx);
Result<CommandResult> cmd_put(const CommandContext &ctx);
Result<CommandResult> cmd_give(const CommandContext &ctx);
Result<CommandResult> cmd_wear(const CommandContext &ctx);
Result<CommandResult> cmd_wield(const CommandContext &ctx);
Result<CommandResult> cmd_remove(const CommandContext &ctx);

// Container and object interaction commands
Result<CommandResult> cmd_open(const CommandContext &ctx);
Result<CommandResult> cmd_close(const CommandContext &ctx);
Result<CommandResult> cmd_lock(const CommandContext &ctx);
Result<CommandResult> cmd_unlock(const CommandContext &ctx);

// Consumable and utility commands
Result<CommandResult> cmd_light(const CommandContext &ctx);
Result<CommandResult> cmd_eat(const CommandContext &ctx);
Result<CommandResult> cmd_drink(const CommandContext &ctx);

// Shop commands
Result<CommandResult> cmd_list(const CommandContext &ctx);
Result<CommandResult> cmd_buy(const CommandContext &ctx);
Result<CommandResult> cmd_sell(const CommandContext &ctx);

} // namespace ObjectCommands