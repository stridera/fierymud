/***************************************************************************
 *   File: src/commands/combat_commands.cpp                Part of FieryMUD *
 *  Usage: Combat command implementations                                   *
 *                                                                           *
 *  All rights reserved.  See license.doc for complete information.         *
 *                                                                           *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "combat_commands.hpp"
#include "builtin_commands.hpp"

#include "../core/actor.hpp"
#include "../core/combat.hpp"
#include "../core/logging.hpp"
#include "../core/object.hpp"
#include "../core/spell_system.hpp"
#include "../world/room.hpp"
#include "../world/world_manager.hpp"

#include <algorithm>
#include <random>

namespace CombatCommands {

// =============================================================================
// Combat Commands
// =============================================================================

Result<CommandResult> cmd_kill(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Kill who?");
        return CommandResult::InvalidTarget;
    }

    // Find target in current room
    auto target = ctx.find_actor_target(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Can't kill yourself
    if (target == ctx.actor) {
        ctx.send_error("You can't kill yourself!");
        return CommandResult::InvalidState;
    }

    // Check if target is already fighting this actor or someone else
    if (target->position() == Position::Fighting) {
        ctx.send(fmt::format("{} is already fighting!", target->display_name()));
        return CommandResult::InvalidState;
    }

    // Check if actor is already fighting
    if (ctx.actor->position() == Position::Fighting) {
        ctx.send("You are already fighting!");
        return CommandResult::InvalidState;
    }

    // Check if actor is dead (including ghost form)
    if (!ctx.actor->is_alive()) {
        if (ctx.actor->position() == Position::Ghost) {
            ctx.send("You are a ghost and cannot engage in combat! Use 'release' to return to the living.");
        } else {
            ctx.send("You can't attack while you're dead!");
        }
        return CommandResult::InvalidState;
    }

    // Start combat
    ctx.actor->set_position(Position::Fighting);
    target->set_position(Position::Fighting);

    // Add to combat manager for ongoing combat rounds
    FieryMUD::CombatManager::start_combat(ctx.actor, target);

    ctx.send(fmt::format("You attack {}!", target->display_name()));
    ctx.send_to_room(fmt::format("{} attacks {}!", ctx.actor->display_name(), target->display_name()), true);
    ctx.send_to_actor(target, fmt::format("{} attacks you!", ctx.actor->display_name()));

    // Perform initial attack
    return perform_attack(ctx, target);
}

Result<CommandResult> cmd_hit(const CommandContext &ctx) {
    // Hit is the same as kill for now
    return cmd_kill(ctx);
}

Result<CommandResult> cmd_cast(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Cast what spell?");
        return CommandResult::InvalidTarget;
    }

    // Initialize spell registry if needed
    SpellRegistry& registry = SpellRegistry::instance();
    static bool initialized = false;
    if (!initialized) {
        registry.initialize_default_spells();
        initialized = true;
    }

    std::string spell_name = ctx.command.full_argument_string;
    
    // Find the spell
    const Spell* spell = registry.find_spell(spell_name);
    if (!spell) {
        ctx.send_error(fmt::format("You don't know any spell called '{}'.", spell_name));
        return CommandResult::InvalidTarget;
    }
    
    // Check if actor can cast this spell
    if (!spell->can_cast(*ctx.actor)) {
        if (const auto* player = dynamic_cast<const Player*>(ctx.actor.get())) {
            std::string player_class = player->player_class();
            if (player_class != "cleric" && player_class != "sorcerer") {
                ctx.send_error("Only clerics and sorcerers can cast spells.");
                return CommandResult::InvalidTarget;
            }
        }
        
        if (ctx.actor->stats().level < spell->circle) {
            ctx.send_error(fmt::format("You need to be at least level {} to cast '{}'.", 
                                     spell->circle, spell->name));
            return CommandResult::InvalidTarget;
        }
        
        ctx.send_error(fmt::format("You cannot cast '{}' for some reason.", spell->name));
        return CommandResult::InvalidTarget;
    }
    
    // Check spell slots
    if (!ctx.actor->has_spell_slots(spell->circle)) {
        auto [current, max] = ctx.actor->get_spell_slot_info(spell->circle);
        ctx.send_error(fmt::format("You have no circle {} spell slots remaining. ({}/{})", 
                                 spell->circle, current, max));
        return CommandResult::InvalidTarget;
    }
    
    // Use spell slot
    if (!ctx.actor->use_spell_slot(spell->circle)) {
        ctx.send_error("Failed to use spell slot.");
        return CommandResult::SystemError;
    }
    
    // Cast the spell
    auto cast_result = spell->cast(*ctx.actor, ctx);
    if (!cast_result) {
        ctx.send_error(fmt::format("Failed to cast '{}': {}", spell->name, cast_result.error().message));
        return CommandResult::SystemError;
    }
    
    Log::info("SPELL_CAST: {} cast '{}' (Circle {})", ctx.actor->name(), spell->name, spell->circle);
    return CommandResult::Success;
}

Result<CommandResult> cmd_flee(const CommandContext &ctx) {
    // Check if fighting
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You are not fighting anyone!");
        return CommandResult::InvalidState;
    }

    // Get current room and find available exits
    auto room = ctx.actor->current_room();
    if (!room) {
        ctx.send_error("You are nowhere to flee to!");
        return CommandResult::InvalidState;
    }

    auto exits = room->get_visible_exits();
    if (exits.empty()) {
        ctx.send_error("There are no exits! You are trapped!");
        return CommandResult::InvalidState;
    }

    // Pick a random exit to flee to
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> exit_choice(0, exits.size() - 1);
    Direction flee_direction = exits[exit_choice(gen)];

    // End combat through combat manager
    FieryMUD::CombatManager::end_combat(ctx.actor);

    ctx.send("You flee from combat!");
    ctx.send_to_room(fmt::format("{} flees from combat!", ctx.actor->display_name()), true);

    // Move in the chosen direction
    auto result = ctx.move_actor_direction(flee_direction);
    if (!result) {
        ctx.send_error(fmt::format("You failed to flee: {}", result.error().message));
        return CommandResult::ResourceError;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_release(const CommandContext &ctx) {
    // Only ghosts can use the release command
    if (ctx.actor->position() != Position::Ghost) {
        ctx.send_error("You are not dead! The 'release' command is only for ghosts.");
        return CommandResult::InvalidState;
    }

    // Create a corpse with the player's items in their current location
    auto current_room = ctx.actor->current_room();
    if (current_room) {
        create_player_corpse(ctx.actor, current_room);
        ctx.send_to_room(fmt::format("The ghost of {} fades away, leaving behind a corpse.", ctx.actor->display_name()), true);
    }

    // Move player to their personal starting room
    auto world_manager = &WorldManager::instance();
    
    // Try to get player's personal start room first
    EntityId start_room_id = INVALID_ENTITY_ID;
    if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        start_room_id = player->start_room();
    }
    
    // If player doesn't have a personal start room, use world default
    if (!start_room_id.is_valid()) {
        start_room_id = world_manager->get_start_room();
        Log::debug("Player {} has no personal start room, using world default: {}", ctx.actor->name(), start_room_id);
    } else {
        Log::debug("Player {} using personal start room: {}", ctx.actor->name(), start_room_id);
    }
    
    auto start_room = world_manager->get_room(start_room_id);

    if (!start_room) {
        ctx.send_error("Error: Could not find starting room! Contact an administrator.");
        Log::error("Failed to find starting room for {}: {}", ctx.actor->name(), start_room_id);
        return CommandResult::SystemError;
    }

    // Move the player to starting room and restore to living
    auto move_result = ctx.actor->move_to(start_room);
    if (!move_result) {
        ctx.send_error("Error: Could not move to starting room! Contact an administrator.");
        Log::error("Failed to move {} to starting room {}: {}", ctx.actor->name(), start_room_id,
                   move_result.error().message);
        return CommandResult::SystemError;
    }
    ctx.actor->set_position(Position::Standing);

    // Reset player's HP to full (they get a new body)
    auto &stats = ctx.actor->stats();
    stats.hit_points = stats.max_hit_points;

    ctx.send("You feel yourself drawn back to the mortal realm in a new body...");
    ctx.send("You have been returned to life!");
    ctx.send_to_room(fmt::format("{} materializes out of thin air!", ctx.actor->display_name()), true);

    return CommandResult::Success;
}

// =============================================================================
// Combat Helper Functions
// =============================================================================

Result<CommandResult> perform_attack(const CommandContext &ctx, std::shared_ptr<Actor> target) {
    if (!ctx.actor || !target) {
        return std::unexpected(Errors::InvalidArgument("actor or target", "cannot be null"));
    }

    // Use the modern combat system
    FieryMUD::CombatResult result = FieryMUD::CombatSystem::perform_attack(ctx.actor, target);

    // Send messages from the combat result
    if (!result.attacker_message.empty()) {
        ctx.send(result.attacker_message);
    }

    if (!result.target_message.empty()) {
        ctx.send_to_actor(target, result.target_message);
    }

    if (!result.room_message.empty()) {
        ctx.send_to_room(result.room_message, true);
    }

    return CommandResult::Success;
}

bool is_valid_target(std::shared_ptr<Actor> attacker, std::shared_ptr<Actor> target) {
    if (!attacker || !target) {
        return false;
    }

    // Can't attack yourself
    if (attacker == target) {
        return false;
    }

    // Can't attack dead actors
    if (target->position() == Position::Dead) {
        return false;
    }

    // Must be in same room
    if (attacker->current_room() != target->current_room()) {
        return false;
    }

    return true;
}

// =============================================================================
// Death Helper Functions
// =============================================================================

void create_player_corpse(std::shared_ptr<Actor> actor, std::shared_ptr<Room> room) {
    // Create a unique ID for the corpse (using player ID + offset for corpses)
    EntityId corpse_id{actor->id().value() + 100000}; // Add offset to avoid conflicts

    // Create corpse object
    std::string corpse_name = fmt::format("the corpse of {}", actor->display_name());
    auto corpse_result = Object::create(corpse_id, corpse_name, ObjectType::Corpse);
    if (!corpse_result) {
        Log::error("Failed to create corpse for {}: {}", actor->name(), corpse_result.error().message);
        return;
    }

    auto corpse = std::shared_ptr<Object>(corpse_result.value().release());

    // Set corpse description
    corpse->set_description(fmt::format("The lifeless body of {} lies here, still and cold.", actor->display_name()));

    // Transfer all equipment to the corpse
    auto equipped_items = actor->equipment().get_all_equipped();
    for (auto &item : equipped_items) {
        if (item) {
            // Remove from player equipment and add to corpse
            actor->equipment().unequip_item(item->id());
            
            // Add item to corpse container
            if (auto* container = dynamic_cast<Container*>(corpse.get())) {
                auto result = container->add_item(item);
                if (result.has_value()) {
                    Log::info("Transferred equipped item '{}' to {}'s corpse", item->display_name(), 
                             actor->display_name());
                } else {
                    Log::warn("Failed to add equipped item '{}' to corpse: {}", 
                             item->display_name(), result.error().message);
                    // If corpse is full, drop item in room
                    room->add_object(item);
                }
            }
        }
    }

    // Transfer all inventory to the corpse
    auto inventory_items = actor->inventory().get_all_items();
    for (auto &item : inventory_items) {
        if (item) {
            // Remove from player inventory and add to corpse
            actor->inventory().remove_item(item->id());
            
            // Add item to corpse container
            if (auto* container = dynamic_cast<Container*>(corpse.get())) {
                auto result = container->add_item(item);
                if (result.has_value()) {
                    Log::info("Transferred inventory item '{}' to {}'s corpse", item->display_name(), 
                             actor->display_name());
                } else {
                    Log::warn("Failed to add inventory item '{}' to corpse: {}", 
                             item->display_name(), result.error().message);
                    // If corpse is full, drop item in room
                    room->add_object(item);
                }
            }
        }
    }

    // Add corpse to the room
    room->add_object(corpse);

    Log::info("Created corpse for {} in room {}", 
             actor->display_name(), room->id().value());
}

} // namespace CombatCommands