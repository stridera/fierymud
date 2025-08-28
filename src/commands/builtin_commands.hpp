/***************************************************************************
 *   File: src/commands/builtin_commands.hpp           Part of FieryMUD *
 *  Usage: Core built-in command implementations                          *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "command_system.hpp"
#include "command_context.hpp"

/**
 * Core built-in commands for FieryMUD.
 * 
 * This module provides essential command implementations that demonstrate
 * the modern command system functionality, including:
 * 
 * - Information commands (look, who, inventory)
 * - Communication commands (say, tell, emote)
 * - Movement commands (north, south, etc.)
 * - Object manipulation (get, drop, wear, remove)
 * - System commands (quit, save, help)
 */

namespace BuiltinCommands {
    /** Register all built-in commands with the command system */
    Result<void> register_all_commands();
    
    /** Unregister all built-in commands */
    void unregister_all_commands();
    
    // Information Commands
    Result<CommandResult> cmd_look(const CommandContext& ctx);
    Result<CommandResult> cmd_examine(const CommandContext& ctx);
    Result<CommandResult> cmd_who(const CommandContext& ctx);
    Result<CommandResult> cmd_where(const CommandContext& ctx);
    Result<CommandResult> cmd_inventory(const CommandContext& ctx);
    Result<CommandResult> cmd_equipment(const CommandContext& ctx);
    Result<CommandResult> cmd_score(const CommandContext& ctx);
    Result<CommandResult> cmd_stats(const CommandContext& ctx);
    Result<CommandResult> cmd_time(const CommandContext& ctx);
    Result<CommandResult> cmd_weather(const CommandContext& ctx);
    
    // Communication Commands
    Result<CommandResult> cmd_say(const CommandContext& ctx);
    Result<CommandResult> cmd_tell(const CommandContext& ctx);
    Result<CommandResult> cmd_emote(const CommandContext& ctx);
    Result<CommandResult> cmd_pose(const CommandContext& ctx);
    Result<CommandResult> cmd_whisper(const CommandContext& ctx);
    Result<CommandResult> cmd_shout(const CommandContext& ctx);
    Result<CommandResult> cmd_gossip(const CommandContext& ctx);
    Result<CommandResult> cmd_ooc(const CommandContext& ctx);
    
    // Movement Commands  
    Result<CommandResult> cmd_north(const CommandContext& ctx);
    Result<CommandResult> cmd_south(const CommandContext& ctx);
    Result<CommandResult> cmd_east(const CommandContext& ctx);
    Result<CommandResult> cmd_west(const CommandContext& ctx);
    Result<CommandResult> cmd_up(const CommandContext& ctx);
    Result<CommandResult> cmd_down(const CommandContext& ctx);
    Result<CommandResult> cmd_northeast(const CommandContext& ctx);
    Result<CommandResult> cmd_northwest(const CommandContext& ctx);
    Result<CommandResult> cmd_southeast(const CommandContext& ctx);
    Result<CommandResult> cmd_southwest(const CommandContext& ctx);
    Result<CommandResult> cmd_exits(const CommandContext& ctx);
    Result<CommandResult> cmd_flee(const CommandContext& ctx);
    Result<CommandResult> cmd_release(const CommandContext& ctx);
    
    // Death Helper Functions
    void create_player_corpse(std::shared_ptr<Actor> actor, std::shared_ptr<Room> room);
    
    // Combat Commands
    Result<CommandResult> cmd_kill(const CommandContext& ctx);
    Result<CommandResult> cmd_hit(const CommandContext& ctx);
    Result<CommandResult> cmd_cast(const CommandContext& ctx);
    
    // Object Manipulation Commands
    Result<CommandResult> cmd_get(const CommandContext& ctx);
    Result<CommandResult> cmd_drop(const CommandContext& ctx);
    Result<CommandResult> cmd_put(const CommandContext& ctx);
    Result<CommandResult> cmd_give(const CommandContext& ctx);
    Result<CommandResult> cmd_wear(const CommandContext& ctx);
    Result<CommandResult> cmd_remove(const CommandContext& ctx);
    Result<CommandResult> cmd_wield(const CommandContext& ctx);
    Result<CommandResult> cmd_hold(const CommandContext& ctx);
    Result<CommandResult> cmd_eat(const CommandContext& ctx);
    Result<CommandResult> cmd_drink(const CommandContext& ctx);
    Result<CommandResult> cmd_light(const CommandContext& ctx);
    
    // Object Interaction Commands
    Result<CommandResult> cmd_open(const CommandContext& ctx);
    Result<CommandResult> cmd_close(const CommandContext& ctx);
    Result<CommandResult> cmd_lock(const CommandContext& ctx);
    Result<CommandResult> cmd_unlock(const CommandContext& ctx);
    
    // System Commands
    Result<CommandResult> cmd_quit(const CommandContext& ctx);
    Result<CommandResult> cmd_save(const CommandContext& ctx);
    Result<CommandResult> cmd_help(const CommandContext& ctx);
    Result<CommandResult> cmd_commands(const CommandContext& ctx);
    Result<CommandResult> cmd_motd(const CommandContext& ctx);
    Result<CommandResult> cmd_bug(const CommandContext& ctx);
    Result<CommandResult> cmd_idea(const CommandContext& ctx);
    Result<CommandResult> cmd_typo(const CommandContext& ctx);
    
    // Social Commands
    Result<CommandResult> cmd_smile(const CommandContext& ctx);
    Result<CommandResult> cmd_nod(const CommandContext& ctx);
    Result<CommandResult> cmd_wave(const CommandContext& ctx);
    Result<CommandResult> cmd_bow(const CommandContext& ctx);
    Result<CommandResult> cmd_dance(const CommandContext& ctx);
    Result<CommandResult> cmd_laugh(const CommandContext& ctx);
    Result<CommandResult> cmd_cry(const CommandContext& ctx);
    Result<CommandResult> cmd_hug(const CommandContext& ctx);
    
    // Administrative Commands (require higher privileges)
    Result<CommandResult> cmd_shutdown(const CommandContext& ctx);
    Result<CommandResult> cmd_reboot(const CommandContext& ctx);
    Result<CommandResult> cmd_goto(const CommandContext& ctx);
    Result<CommandResult> cmd_teleport(const CommandContext& ctx);
    Result<CommandResult> cmd_summon(const CommandContext& ctx);
    Result<CommandResult> cmd_transfer(const CommandContext& ctx);
    Result<CommandResult> cmd_force(const CommandContext& ctx);
    Result<CommandResult> cmd_wizlock(const CommandContext& ctx);
    Result<CommandResult> cmd_advance(const CommandContext& ctx);
    Result<CommandResult> cmd_restore(const CommandContext& ctx);
    Result<CommandResult> cmd_weather_control(const CommandContext& ctx);
    
    // Helper functions
    namespace Helpers {
        /** Format room description for look command */
        std::string format_room_description(std::shared_ptr<Room> room, std::shared_ptr<Actor> viewer);
        
        /** Format object description for examine command */
        std::string format_object_description(std::shared_ptr<Object> obj, std::shared_ptr<Actor> viewer);
        
        /** Format actor description for look command */
        std::string format_actor_description(std::shared_ptr<Actor> target, std::shared_ptr<Actor> viewer);
        
        /** Format inventory list */
        std::string format_inventory(std::shared_ptr<Actor> actor);
        
        /** Format equipment list */
        std::string format_equipment(std::shared_ptr<Actor> actor);
        
        /** Format who list */
        std::string format_who_list(const std::vector<std::shared_ptr<Actor>>& actors);
        
        /** Format exits list */
        std::string format_exits(std::shared_ptr<Room> room);
        
        /** Check if movement is possible in direction */
        bool can_move_direction(std::shared_ptr<Actor> actor, Direction dir, std::string& failure_reason);
        
        /** Execute movement in direction */
        Result<CommandResult> execute_movement(const CommandContext& ctx, Direction dir);
        
        /** Format social action message */
        std::string format_social_message(std::string_view action, 
                                         std::shared_ptr<Actor> actor, 
                                         std::shared_ptr<Actor> target = nullptr);
        
        /** Validate target for social command */
        bool validate_social_target(const CommandContext& ctx, std::shared_ptr<Actor> target);
        
        /** Send communication message */
        void send_communication(const CommandContext& ctx, 
                               std::string_view message,
                               MessageType type,
                               std::string_view channel_name = "");
        
        /** Format communication message */
        std::string format_communication(std::shared_ptr<Actor> sender,
                                       std::string_view message,
                                       std::string_view channel = "");
    }
}

/** Command registration utility macros */
#define REGISTER_BUILTIN_COMMAND(name, func, category, privilege) \
    Commands().command(name, func) \
        .category(category) \
        .privilege(privilege) \
        .build()

#define REGISTER_MOVEMENT_COMMAND(name, direction, aliases, description) \
    Commands().command(name, [](const CommandContext& ctx) { \
        return BuiltinCommands::Helpers::execute_movement(ctx, direction); \
    }) \
        .aliases(aliases) \
        .category("Movement") \
        .description(description) \
        .usable_while_sitting(false) \
        .build()

#define REGISTER_SOCIAL_COMMAND(name, action_text) \
    Commands().command(name, [](const CommandContext& ctx) { \
        return BuiltinCommands::cmd_##name(ctx); \
    }) \
        .category("Social") \
        .description(action_text) \
        .build()
