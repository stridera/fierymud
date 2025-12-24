#include "combat_commands.hpp"
#include "builtin_commands.hpp"

#include "core/ability_executor.hpp"
#include "../core/actor.hpp"
#include "../core/combat.hpp"
#include "../core/logging.hpp"
#include "../core/object.hpp"
#include "../core/spell_system.hpp"
#include "../text/string_utils.hpp"
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

    // Check if target is dead or a ghost
    if (!target->is_alive()) {
        if (target->position() == Position::Ghost) {
            ctx.send_error(fmt::format("{} is already dead! You cannot attack a ghost.", target->display_name()));
        } else {
            ctx.send_error(fmt::format("{} is already dead!", target->display_name()));
        }
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

    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can cast spells.");
        return CommandResult::InvalidState;
    }

    // Parse spell name and optional target from arguments
    // Format: "cast <spell> [target]" or "cast '<multi word spell>' [target]"
    std::string full_args = ctx.command.full_argument_string;
    std::string spell_name;
    std::string target_name;

    // Trim whitespace
    full_args = std::string(trim(full_args));

    // Check for quoted spell name
    if (!full_args.empty() && full_args[0] == '\'') {
        auto end_quote = full_args.find('\'', 1);
        if (end_quote != std::string::npos) {
            spell_name = full_args.substr(1, end_quote - 1);
            if (end_quote + 1 < full_args.size()) {
                target_name = std::string(trim(full_args.substr(end_quote + 1)));
            }
        } else {
            spell_name = full_args.substr(1);
        }
    } else {
        // Try to match against known spells to find longest match
        auto known_abilities = player->get_known_abilities();
        std::string full_args_lower = to_lowercase(full_args);

        const LearnedAbility* best_match = nullptr;
        size_t best_match_len = 0;

        for (const auto* ability : known_abilities) {
            if (ability->type != "SPELL") continue;

            // Try matching against display name (with spaces)
            std::string name_lower = to_lowercase(ability->name);

            // Check if input starts with this spell's display name
            if (full_args_lower.find(name_lower) == 0) {
                // Make sure it's a complete word match (followed by space or end)
                if (full_args_lower.size() == name_lower.size() ||
                    full_args_lower[name_lower.size()] == ' ') {
                    if (name_lower.size() > best_match_len) {
                        best_match = ability;
                        best_match_len = name_lower.size();
                    }
                }
            }
        }

        if (best_match) {
            spell_name = best_match->plain_name;
            if (best_match_len < full_args.size()) {
                target_name = std::string(trim(full_args.substr(best_match_len)));
            }
        } else {
            // Fall back to first word as spell name
            auto space_pos = full_args.find(' ');
            if (space_pos != std::string::npos) {
                spell_name = full_args.substr(0, space_pos);
                target_name = std::string(trim(full_args.substr(space_pos + 1)));
            } else {
                spell_name = full_args;
            }
        }
    }

    // Convert search term to lowercase for matching
    std::string search_lower = to_lowercase(spell_name);

    // Also create a version with spaces replaced by underscores for plain_name matching
    std::string search_normalized = search_lower;
    std::replace(search_normalized.begin(), search_normalized.end(), ' ', '_');

    // Check if player knows this spell from their abilities
    auto known_abilities = player->get_known_abilities();
    const LearnedAbility* known_spell = nullptr;

    for (const auto* ability : known_abilities) {
        if (ability->type != "SPELL") continue;

        // Compare against both plain_name (with underscores) and display name (with spaces)
        std::string plain_lower = to_lowercase(ability->plain_name);
        std::string name_lower = to_lowercase(ability->name);

        // Allow partial match from the beginning against either name format
        if (plain_lower.find(search_normalized) == 0 || name_lower.find(search_lower) == 0) {
            known_spell = ability;
            break;
        }
    }

    if (!known_spell) {
        ctx.send_error(fmt::format("You don't know any spell called '{}'.", spell_name));
        return CommandResult::InvalidTarget;
    }

    // Check minimum level
    if (ctx.actor->stats().level < known_spell->min_level) {
        ctx.send_error(fmt::format("You need to be at least level {} to cast '{}'.",
                                 known_spell->min_level, known_spell->name));
        return CommandResult::InvalidTarget;
    }

    // Find target if specified or if we're fighting
    std::shared_ptr<Actor> target;
    if (!target_name.empty()) {
        target = ctx.find_actor_target(target_name);
        if (!target) {
            ctx.send_error(fmt::format("You don't see '{}' here.", target_name));
            return CommandResult::InvalidTarget;
        }
        // Check if target is dead/ghost
        if (!target->is_alive()) {
            ctx.send_error(fmt::format("{} is already dead!", target->display_name()));
            return CommandResult::InvalidState;
        }
    } else if (ctx.actor->position() == Position::Fighting) {
        // If no target specified but we're fighting, use our opponent
        target = FieryMUD::CombatManager::get_opponent(*ctx.actor);
    }

    // Violent spells (damage spells) require a target
    if (known_spell->violent && !target) {
        ctx.send_error(fmt::format("Cast '{}' on whom?", known_spell->name));
        return CommandResult::InvalidTarget;
    }

    // Try to execute using the AbilityExecutor for data-driven effects
    auto exec_result = FieryMUD::AbilityExecutor::execute(ctx, known_spell->plain_name, target,
                                                           known_spell->proficiency / 10);

    if (exec_result && exec_result->success && !exec_result->effect_results.empty()) {
        // Send the generated messages
        if (!exec_result->attacker_message.empty()) {
            ctx.send(exec_result->attacker_message);
        }
        if (target && !exec_result->target_message.empty()) {
            ctx.send_to_actor(target, exec_result->target_message);
        }
        if (!exec_result->room_message.empty()) {
            ctx.send_to_room(exec_result->room_message, true);
        }

        // Check if we dealt damage
        if (target && exec_result->total_damage > 0) {
            if (target->stats().hit_points <= 0) {
                // Target died
                target->stats().hit_points = 0;
                FieryMUD::CombatManager::end_combat(target);
                target->set_position(Position::Ghost);

                // Award experience
                long exp_gain = FieryMUD::CombatSystem::calculate_experience_gain(*ctx.actor, *target);
                ctx.actor->gain_experience(exp_gain);

                ctx.send(fmt::format("You have killed {}! You gain {} experience.",
                                    target->display_name(), exp_gain));
                ctx.send_to_actor(target, "You are DEAD!");
                ctx.send_to_room(fmt::format("{} is DEAD!", target->display_name()), true);
            } else {
                // Target survived - start combat if not already fighting
                if (ctx.actor->position() != Position::Fighting) {
                    FieryMUD::CombatManager::start_combat(ctx.actor, target);
                }
            }
        }

        Log::info("SPELL_CAST: {} cast '{}' on {} via AbilityExecutor (damage: {})",
                 ctx.actor->name(), known_spell->plain_name,
                 target ? target->name() : "no target", exec_result->total_damage);
    } else {
        // AbilityExecutor doesn't have effects for this spell - use generic casting
        if (target) {
            ctx.send(fmt::format("You cast '{}' at {}!", known_spell->name, target->display_name()));
            ctx.send_to_actor(target, fmt::format("{} casts '{}' at you!", player->display_name(), known_spell->name));
            ctx.send_to_room(fmt::format("{} casts '{}' at {}!", player->display_name(),
                            known_spell->name, target->display_name()), true);
        } else {
            ctx.send(fmt::format("You cast '{}'!", known_spell->name));
            ctx.send_to_room(fmt::format("{} casts '{}'!", player->display_name(), known_spell->name), true);
        }
        Log::info("SPELL_CAST: {} cast '{}' (generic - no effects defined)", ctx.actor->name(), known_spell->name);
    }

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

    // Can't attack dead or ghost actors
    if (!target->is_alive()) {
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
             actor->display_name(), room->id());
}

// =============================================================================
// Combat Skill Commands
// =============================================================================

Result<CommandResult> cmd_rescue(const CommandContext &ctx) {
    // Rescue pulls a target out of combat, taking their place

    if (ctx.arg_count() == 0) {
        ctx.send_error("Rescue who?");
        return CommandResult::InvalidSyntax;
    }

    // Find target in room
    auto target = ctx.find_actor_target(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Can't rescue yourself
    if (target == ctx.actor) {
        ctx.send_error("You can't rescue yourself!");
        return CommandResult::InvalidTarget;
    }

    // Target must be fighting
    if (target->position() != Position::Fighting) {
        ctx.send_error(fmt::format("{} isn't fighting anyone.", target->display_name()));
        return CommandResult::InvalidState;
    }

    // Get target's opponent
    auto enemy = FieryMUD::CombatManager::get_opponent(*target);
    if (!enemy) {
        ctx.send_error(fmt::format("{} isn't fighting anyone.", target->display_name()));
        return CommandResult::InvalidState;
    }

    // Rescuer must be standing or fighting
    if (ctx.actor->position() != Position::Standing && ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You need to be on your feet to rescue someone!");
        return CommandResult::InvalidState;
    }

    // TODO: Add skill check for rescue success rate

    // End target's combat
    FieryMUD::CombatManager::end_combat(target);
    target->set_position(Position::Standing);

    // Start combat between rescuer and enemy
    ctx.actor->set_position(Position::Fighting);
    enemy->set_position(Position::Fighting);
    FieryMUD::CombatManager::start_combat(ctx.actor, enemy);

    ctx.send(fmt::format("You rescue {} from {}!", target->display_name(), enemy->display_name()));
    ctx.send_to_actor(target, fmt::format("{} rescues you from {}!", ctx.actor->display_name(), enemy->display_name()));
    ctx.send_to_actor(enemy, fmt::format("{} rescues {} from you!", ctx.actor->display_name(), target->display_name()));
    ctx.send_to_room(fmt::format("{} heroically rescues {} from {}!", ctx.actor->display_name(),
                     target->display_name(), enemy->display_name()), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_assist(const CommandContext &ctx) {
    // Assist joins combat on someone's side

    if (ctx.arg_count() == 0) {
        ctx.send_error("Assist who?");
        return CommandResult::InvalidSyntax;
    }

    // Find target in room
    auto target = ctx.find_actor_target(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Can't assist yourself
    if (target == ctx.actor) {
        ctx.send_error("You can't assist yourself!");
        return CommandResult::InvalidTarget;
    }

    // Target must be fighting
    if (target->position() != Position::Fighting) {
        ctx.send_error(fmt::format("{} isn't fighting anyone.", target->display_name()));
        return CommandResult::InvalidState;
    }

    // Get target's opponent
    auto enemy = FieryMUD::CombatManager::get_opponent(*target);
    if (!enemy) {
        ctx.send_error(fmt::format("{} isn't fighting anyone.", target->display_name()));
        return CommandResult::InvalidState;
    }

    // Assister must not be fighting
    if (ctx.actor->position() == Position::Fighting) {
        ctx.send_error("You're already in combat!");
        return CommandResult::InvalidState;
    }

    // Assister must be standing
    if (ctx.actor->position() != Position::Standing) {
        ctx.send_error("You need to be standing to assist someone!");
        return CommandResult::InvalidState;
    }

    // Start combat between assister and enemy
    ctx.actor->set_position(Position::Fighting);
    FieryMUD::CombatManager::start_combat(ctx.actor, enemy);

    ctx.send(fmt::format("You assist {} against {}!", target->display_name(), enemy->display_name()));
    ctx.send_to_actor(target, fmt::format("{} joins the fight on your side!", ctx.actor->display_name()));
    ctx.send_to_actor(enemy, fmt::format("{} assists {} against you!", ctx.actor->display_name(), target->display_name()));
    ctx.send_to_room(fmt::format("{} assists {} against {}!", ctx.actor->display_name(),
                     target->display_name(), enemy->display_name()), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_kick(const CommandContext &ctx) {
    // Kick is a data-driven combat skill
    return FieryMUD::execute_skill_command(ctx, "kick", true, true);
}

Result<CommandResult> cmd_bash(const CommandContext &ctx) {
    // Bash is a data-driven combat skill
    return FieryMUD::execute_skill_command(ctx, "bash", true, true);
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands()
        .command("kill", cmd_kill)
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("hit", cmd_hit)
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("cast", cmd_cast)
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("flee", cmd_flee)
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("release", cmd_release)
        .category("Death")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Combat skill commands
    Commands()
        .command("rescue", cmd_rescue)
        .alias("res")
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("assist", cmd_assist)
        .alias("ass")
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("kick", cmd_kick)
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("bash", cmd_bash)
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    return Success();
}

} // namespace CombatCommands