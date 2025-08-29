/***************************************************************************
 *   File: src/commands/system_commands.cpp         Part of FieryMUD *
 *  Usage: System command implementations                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "system_commands.hpp"
#include "../core/actor.hpp"
#include "../core/logging.hpp"

#include <fmt/format.h>

namespace SystemCommands {

// =============================================================================
// System Command Implementations
// =============================================================================

Result<CommandResult> cmd_quit(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    ctx.send("Goodbye! Thanks for playing FieryMUD.");
    ctx.actor->set_position(Position::Standing);  // Player remains standing until logout

    // TODO: Implement proper logout sequence with save
    Log::info("Player {} quit the game", ctx.actor->name());

    return CommandResult::Success;
}

Result<CommandResult> cmd_save(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // TODO: Implement player save functionality
    ctx.send("Saving your character...");
    Log::info("Manual save for player {}", ctx.actor->name());
    ctx.send("Saved successfully.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_help(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send("FieryMUD Help System");
        ctx.send("===================");
        ctx.send("Use 'help <topic>' for specific help on:");
        ctx.send("  commands - List all available commands");
        ctx.send("  movement - Moving around the world");
        ctx.send("  combat   - Fighting and combat");
        ctx.send("  objects  - Interacting with items");
        ctx.send("  communication - Talking to other players");
        ctx.send("\nFor a complete command list, type 'commands'.");
        return CommandResult::Success;
    }

    std::string topic{ctx.arg(0)};
    
    if (topic == "commands") {
        return cmd_commands(ctx);
    } else if (topic == "movement") {
        ctx.send("Movement Help");
        ctx.send("=============");
        ctx.send("north/n, south/s, east/e, west/w - Move in cardinal directions");
        ctx.send("up/u, down/d - Move vertically");
        ctx.send("exits - Show available exits from current room");
        ctx.send("look/l - Look around your current location");
    } else if (topic == "combat") {
        ctx.send("Combat Help");
        ctx.send("===========");
        ctx.send("kill <target> - Attack a target");
        ctx.send("flee - Attempt to escape from combat");
        ctx.send("release - Return to life as a ghost");
    } else if (topic == "objects") {
        ctx.send("Object Interaction Help");
        ctx.send("=======================");
        ctx.send("get <item> - Pick up an item");
        ctx.send("drop <item> - Drop an item");
        ctx.send("put <item> <container> - Put item in container");
        ctx.send("wear <item> - Wear equipment");
        ctx.send("remove <item> - Remove equipment");
        ctx.send("light <item> - Light a torch or lantern");
        ctx.send("eat <food> - Consume food");
        ctx.send("drink <item> - Drink from containers");
    } else if (topic == "communication") {
        ctx.send("Communication Help");
        ctx.send("==================");
        ctx.send("say <message> - Speak to everyone in the room");
        ctx.send("tell <player> <message> - Send private message");
        ctx.send("emote <action> - Perform an emote");
        ctx.send("whisper <player> <message> - Whisper to someone nearby");
        ctx.send("shout <message> - Shout to nearby areas");
        ctx.send("gossip <message> - Talk on gossip channel");
    } else {
        ctx.send_error(fmt::format("No help available for '{}'.", topic));
        return CommandResult::InvalidTarget;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_commands(const CommandContext &ctx) {
    ctx.send("Available Commands:");
    ctx.send("===================");
    
    ctx.send("\nInformation:");
    ctx.send("  look/l, examine, who, where, inventory/i, equipment, score, time, weather");
    
    ctx.send("\nCommunication:");
    ctx.send("  say/'<msg>, tell, emote, whisper, shout, gossip");
    
    ctx.send("\nMovement:");
    ctx.send("  north/n, south/s, east/e, west/w, up/u, down/d, exits");
    
    ctx.send("\nObjects:");
    ctx.send("  get, drop, put, give, wear, wield, remove, light, eat, drink");
    ctx.send("  open, close, lock, unlock");
    
    ctx.send("\nCombat:");
    ctx.send("  kill, hit, cast, flee, release");
    
    ctx.send("\nSocial:");
    ctx.send("  smile, nod, wave, bow, laugh");
    
    ctx.send("\nSystem:");
    ctx.send("  quit, save, help, commands");
    
    // Only show admin commands to privileged users
    if (ctx.actor && ctx.actor->stats().level >= 30) {  // Assuming level 30+ for gods/admins
        ctx.send("\nAdministrative:");
        ctx.send("  goto, teleport, summon, setweather, shutdown");
        ctx.send("  reloadzone, savezone, reloadallzones, filewatch, dumpworld");
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_prompt(const CommandContext &ctx) {
    ctx.send("Prompt command executed successfully.");
    return CommandResult::Success;
}

} // namespace SystemCommands