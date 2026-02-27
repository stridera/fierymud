#include "combat_commands.hpp"

#include <algorithm>
#include <array>
#include <random>
#include <unordered_map>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "commands/command_system.hpp"
#include "core/ability_executor.hpp"
#include "core/actor.hpp"
#include "core/combat.hpp"
#include "core/logging.hpp"
#include "core/money.hpp"
#include "core/object.hpp"
#include "core/player.hpp"
#include "core/spell_system.hpp"
#include "information_commands.hpp"
#include "text/string_utils.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"

namespace CombatCommands {

// Default casting blackout duration (in milliseconds)
// This prevents spam-casting and gives combat more strategic pacing
constexpr auto DEFAULT_CAST_BLACKOUT_MS = std::chrono::milliseconds{3000};

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

    // Check if player is currently casting a spell - queue the next spell
    if (ctx.actor->is_casting()) {
        auto state_opt = ctx.actor->casting_state();
        std::string full_args = ctx.command.full_argument_string;

        // If they don't already have a spell queued, queue this one
        if (!ctx.actor->has_queued_spell()) {
            std::string spell_display = std::string(trim(full_args));
            if (spell_display.length() > 30) {
                spell_display = spell_display.substr(0, 27) + "...";
            }
            ctx.actor->queue_spell(full_args, "");

            if (state_opt.has_value()) {
                const auto &state = state_opt.value();
                double remaining_sec = state.ticks_remaining * 0.5; // 500ms per tick
                ctx.send(
                    fmt::format("You are still casting {} ({:.1f}s remaining).", state.ability_name, remaining_sec));
            }
            ctx.send(fmt::format("Your next spell '{}' has been queued.", spell_display));
        } else {
            if (state_opt.has_value()) {
                const auto &state = state_opt.value();
                double remaining_sec = state.ticks_remaining * 0.5;
                ctx.send(
                    fmt::format("You are still casting {} ({:.1f}s remaining).", state.ability_name, remaining_sec));
            }
            ctx.send("You already have a spell queued - use 'abort' to cancel.");
        }
        return CommandResult::Cooldown;
    }

    // Check if player is in casting blackout
    if (ctx.actor->is_casting_blocked()) {
        auto remaining = ctx.actor->get_blackout_remaining();
        auto remaining_sec = std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count() / 1000.0;

        // Store the full argument string for queuing
        std::string full_args = ctx.command.full_argument_string;

        // If they don't already have a spell queued, offer to queue this one
        if (!ctx.actor->has_queued_spell()) {
            // Parse spell name to display
            std::string spell_display = std::string(trim(full_args));
            if (spell_display.length() > 30) {
                spell_display = spell_display.substr(0, 27) + "...";
            }
            ctx.actor->queue_spell(full_args, ""); // Store full args for later parsing
            ctx.send(fmt::format("You are still recovering from your last spell ({:.1f}s remaining).", remaining_sec));
            ctx.send(fmt::format("Your next spell '{}' has been queued.", spell_display));
        } else {
            ctx.send(fmt::format("You are still recovering from your last spell ({:.1f}s remaining).", remaining_sec));
            ctx.send("You already have a spell queued - wait for it to cast first.");
        }
        return CommandResult::Cooldown;
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

        const LearnedAbility *best_match = nullptr;
        size_t best_match_len = 0;

        for (const auto *ability : known_abilities) {
            // Accept SPELL, CHANT, and SONG types for cast/perform/chant
            if (ability->type != "SPELL" && ability->type != "CHANT" && ability->type != "SONG")
                continue;

            // Try matching against display name (with spaces)
            std::string name_lower = to_lowercase(ability->name);

            // Check if input starts with this spell's display name
            if (full_args_lower.find(name_lower) == 0) {
                // Make sure it's a complete word match (followed by space or end)
                if (full_args_lower.size() == name_lower.size() || full_args_lower[name_lower.size()] == ' ') {
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
    const LearnedAbility *known_spell = nullptr;

    for (const auto *ability : known_abilities) {
        // Accept SPELL, CHANT, and SONG types
        if (ability->type != "SPELL" && ability->type != "CHANT" && ability->type != "SONG")
            continue;

        // Compare against both plain_name (with underscores) and display name (with spaces)
        std::string plain_lower = to_lowercase(ability->plain_name);
        std::string name_lower = to_lowercase(ability->name);

        // Allow partial match from the beginning against either name format
        if (plain_lower.find(search_normalized) == 0 || name_lower.find(search_lower) == 0) {
            known_spell = ability;
            break;
        }

        // Try per-word fuzzy match: "det inv" matches "detect invisible"
        if (matches_words(search_lower, name_lower) || matches_words(search_normalized, plain_lower)) {
            known_spell = ability;
            break;
        }
    }

    if (!known_spell) {
        ctx.send_error(fmt::format("You don't know any spell, chant, or song called '{}'.", spell_name));
        return CommandResult::InvalidTarget;
    }

    // Check minimum level
    if (ctx.actor->stats().level < known_spell->min_level) {
        ctx.send_error(
            fmt::format("You need to be at least level {} to cast '{}'.", known_spell->min_level, known_spell->name));
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
    } else if (ctx.actor->position() == Position::Fighting && known_spell->violent) {
        // Violent spells auto-target opponent; non-violent (heals/buffs) will self-target later
        target = FieryMUD::CombatManager::get_opponent(*ctx.actor);
    }

    // Violent single-target spells require a target; AOE spells auto-target enemies
    auto &ability_cache = FieryMUD::AbilityCache::instance();
    const auto *ability_data = ability_cache.get_ability(known_spell->ability_id);
    bool is_area_spell = ability_data && ability_data->is_area;
    bool targets_corpse = ability_data && ability_data->targets_corpse;

    // Check if target is dead/ghost (unless spell specifically targets corpses)
    if (target && !target->is_alive() && !targets_corpse) {
        ctx.send_error(fmt::format("{} is already dead!", target->display_name()));
        return CommandResult::InvalidState;
    }

    if (known_spell->violent && !target && !is_area_spell) {
        ctx.send_error(fmt::format("Cast '{}' on whom?", known_spell->name));
        return CommandResult::InvalidTarget;
    }

    // Gods cast instantly, others have casting time
    bool is_god = player && player->is_god();

    // Non-gods must have spell slots available for this circle (allows upcasting)
    if (!is_god && known_spell->circle > 0) {
        // Check if any slot from this circle or higher is available (upcasting)
        bool has_available_slot = false;
        for (int c = known_spell->circle; c <= 9; ++c) {
            if (player->has_spell_slots(c)) {
                has_available_slot = true;
                break;
            }
        }
        if (!has_available_slot) {
            ctx.send_error(
                fmt::format("You don't have any spell slots available for circle {} spells.", known_spell->circle));
            return CommandResult::InvalidState;
        }

        // Non-god casting a spell with a circle - use casting time
        auto cast_result =
            FieryMUD::AbilityExecutor::begin_casting(ctx.actor, known_spell->ability_id, target, known_spell->circle);

        if (!cast_result) {
            ctx.send_error(fmt::format("Failed to begin casting: {}", cast_result.error().message));
            return CommandResult::InvalidState;
        }

        // Casting started successfully - spell will complete after casting time
        return CommandResult::Success;
    }

    // Gods cast instantly, or skill/chant with no circle - execute immediately
    auto exec_result =
        FieryMUD::AbilityExecutor::execute(ctx, known_spell->plain_name, target, known_spell->proficiency / 10);

    if (exec_result && exec_result->success && !exec_result->effect_results.empty()) {
        // Send the generated messages
        if (!exec_result->attacker_message.empty()) {
            ctx.send(exec_result->attacker_message);
        }
        if (target && !exec_result->target_message.empty()) {
            ctx.send_to_actor(target, exec_result->target_message);
        }
        if (!exec_result->room_message.empty()) {
            // Exclude target from room message if they got a personalized message
            if (target && !exec_result->target_message.empty()) {
                ctx.send_to_room(exec_result->room_message, true, std::array{target});
            } else {
                ctx.send_to_room(exec_result->room_message, true);
            }
        }

        // Check if we dealt damage (only violent spells trigger combat)
        if (target && known_spell->violent && exec_result->total_damage > 0) {
            if (target->stats().hit_points <= 0) {
                // Target died
                target->stats().hit_points = 0;
                FieryMUD::CombatManager::end_combat(target);

                // Send death messages based on actor type
                bool target_is_player = (target->type_name() == "Player");

                if (target_is_player) {
                    ctx.send_to_actor(target, "You are DEAD! You feel your spirit leave your body...");
                    ctx.send_to_room(
                        fmt::format("{} is DEAD! Their spirit rises from their body.", target->display_name()), true);
                } else {
                    ctx.send_to_room(fmt::format("{} is DEAD! {} corpse falls to the ground.", target->display_name(),
                                                 target->display_name()),
                                     true);
                }

                // Polymorphic death handling - Player becomes ghost, Mobile creates corpse and despawns
                // die() returns the corpse for Mobiles, nullptr for Players
                auto corpse = target->die();

                // Handle autoloot and autogold for Player attackers killing Mobiles
                if (!target_is_player && player && corpse) {
                    // Autogold: Find and take money from corpse
                    if (player->is_autogold()) {
                        auto contents = corpse->get_contents();
                        std::vector<std::shared_ptr<Object>> items_copy(contents.begin(), contents.end());
                        for (const auto &item : items_copy) {
                            if (item && item->type() == ObjectType::Money) {
                                int coin_value = item->value();
                                if (coin_value > 0) {
                                    player->receive(coin_value);
                                    corpse->remove_item(item);
                                    auto received = fiery::Money::from_copper(coin_value);
                                    ctx.send(fmt::format("You get {} from the corpse.", received.to_string()));
                                }
                            }
                        }
                    }

                    // Autoloot: Take all non-money items from corpse
                    if (player->is_autoloot()) {
                        auto contents = corpse->get_contents();
                        std::vector<std::shared_ptr<Object>> items_copy(contents.begin(), contents.end());
                        int looted_count = 0;
                        for (const auto &item : items_copy) {
                            if (!item || item->type() == ObjectType::Money)
                                continue;

                            int obj_weight = item->weight();
                            if (!player->inventory().can_carry(obj_weight, player->max_carry_weight())) {
                                ctx.send("You can't carry any more weight.");
                                break;
                            }

                            if (corpse->remove_item(item)) {
                                auto add_result = player->inventory().add_item(item);
                                if (add_result) {
                                    looted_count++;
                                } else {
                                    corpse->add_item(item);
                                }
                            }
                        }
                        if (looted_count > 0) {
                            ctx.send(
                                fmt::format("You autoloot {} item{}.", looted_count, looted_count == 1 ? "" : "s"));
                        }
                    }
                }

                // Award experience
                long exp_gain = FieryMUD::CombatSystem::calculate_experience_gain(*ctx.actor, *target);
                ctx.actor->gain_experience(exp_gain);

                ctx.send(fmt::format("You have killed {}! You gain {} experience.", target->display_name(), exp_gain));
            } else {
                // Target survived - start combat if not already fighting
                if (ctx.actor->position() != Position::Fighting) {
                    FieryMUD::CombatManager::start_combat(ctx.actor, target);
                }
            }
        }

        Log::info("SPELL_CAST: {} cast '{}' on {} via AbilityExecutor (damage: {})", ctx.actor->name(),
                  known_spell->plain_name, target ? target->name() : "no target", exec_result->total_damage);
    } else {
        // AbilityExecutor failed or has no effects - this is a configuration error
        Log::warn("SPELL_ERROR: {} tried to cast '{}' but spell has no effects defined in database!", ctx.actor->name(),
                  known_spell->plain_name);
        ctx.send(
            fmt::format("<red>Error:</> The spell '{}' is not configured correctly. Please report this to an immortal.",
                        known_spell->name));
        // Don't show fake cast messages - the spell didn't actually work
        return CommandResult::SystemError;
    }

    // Apply casting blackout to prevent spam-casting
    // Clear any queued spell since we just cast successfully
    ctx.actor->clear_queued_spell();
    ctx.actor->set_casting_blackout(DEFAULT_CAST_BLACKOUT_MS);

    return CommandResult::Success;
}

Result<CommandResult> cmd_abort(const CommandContext &ctx) {
    bool aborted_anything = false;

    // Abort current casting
    if (ctx.actor->is_casting()) {
        auto state_opt = ctx.actor->casting_state();
        if (state_opt.has_value()) {
            ctx.send(fmt::format("You abort your {} spell.", state_opt->ability_name));
        } else {
            ctx.send("You abort your spell.");
        }
        ctx.actor->stop_casting(true); // Interrupted = true
        aborted_anything = true;
    }

    // Clear queued spell
    if (ctx.actor->has_queued_spell()) {
        ctx.actor->clear_queued_spell();
        ctx.send("Your queued spell has been cancelled.");
        aborted_anything = true;
    }

    if (!aborted_anything) {
        ctx.send("You are not casting or have anything queued.");
        return CommandResult::InvalidState;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_flee(const CommandContext &ctx) {
    // Abort casting if in progress (flee interrupts spellcasting)
    if (ctx.actor->is_casting()) {
        auto state_opt = ctx.actor->casting_state();
        if (state_opt.has_value()) {
            ctx.send(fmt::format("You abort your {} spell as you panic!", state_opt->ability_name));
        }
        ctx.actor->stop_casting(true);   // Interrupted = true
        ctx.actor->clear_queued_spell(); // Also clear queued spell
    }

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

    // Show the room after fleeing (respects brief mode)
    ctx.send(InformationCommands::format_room_for_actor(ctx.actor));

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
        create_actor_corpse(ctx.actor, current_room);
        ctx.send_to_room(fmt::format("The ghost of {} fades away, leaving behind a corpse.", ctx.actor->display_name()),
                         true);
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

    // Show the starting room (respects brief mode)
    ctx.send(InformationCommands::format_room_for_actor(ctx.actor));

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
        // Exclude target from room message if they got a personalized message
        if (target && !result.target_message.empty()) {
            ctx.send_to_room(result.room_message, true, std::array{target});
        } else {
            ctx.send_to_room(result.room_message, true);
        }
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

void create_actor_corpse(std::shared_ptr<Actor> actor, std::shared_ptr<Room> room) {
    if (!actor || !room) {
        Log::error("create_actor_corpse called with null actor or room");
        return;
    }

    // Calculate corpse capacity based on items to transfer
    // Note: get_all_equipped returns a vector (safe copy), get_all_items returns a span (view)
    // We need to copy the span to a vector because we'll modify the inventory during iteration
    auto equipped_items = actor->equipment().get_all_equipped();
    auto inventory_span = actor->inventory().get_all_items();
    std::vector<std::shared_ptr<Object>> inventory_items(inventory_span.begin(), inventory_span.end());
    int corpse_capacity = static_cast<int>(equipped_items.size() + inventory_items.size() + 5); // +5 buffer

    // Create a unique ID for the corpse (using actor ID + offset for corpses)
    EntityId corpse_id{actor->id().value() + 100000}; // Add offset to avoid conflicts

    // Create corpse as a Container to hold items
    std::string corpse_name = fmt::format("the corpse of {}", actor->display_name());
    auto corpse_result = Container::create(corpse_id, corpse_name, corpse_capacity, ObjectType::Corpse);
    if (!corpse_result) {
        Log::error("Failed to create corpse for {}: {}", actor->name(), corpse_result.error().message);
        return;
    }

    auto corpse = std::shared_ptr<Container>(corpse_result.value().release());

    // Set corpse description
    corpse->set_description(fmt::format("The lifeless body of {} lies here, still and cold.", actor->display_name()));

    // Add keywords: "corpse" plus all keywords from the dead actor
    corpse->add_keyword("corpse");
    for (const auto &kw : actor->keywords()) {
        corpse->add_keyword(kw);
    }

    // Transfer all equipment to the corpse
    for (auto &item : equipped_items) {
        if (item) {
            actor->equipment().unequip_item(item->id());
            auto result = corpse->add_item(item);
            if (result.has_value()) {
                Log::debug("Transferred equipped item '{}' to {}'s corpse", item->display_name(),
                           actor->display_name());
            } else {
                Log::warn("Failed to add equipped item '{}' to corpse: {}", item->display_name(),
                          result.error().message);
                room->add_object(item); // Drop in room if corpse full
            }
        }
    }

    // Transfer all inventory to the corpse
    for (auto &item : inventory_items) {
        if (item) {
            actor->inventory().remove_item(item->id());
            auto result = corpse->add_item(item);
            if (result.has_value()) {
                Log::debug("Transferred inventory item '{}' to {}'s corpse", item->display_name(),
                           actor->display_name());
            } else {
                Log::warn("Failed to add inventory item '{}' to corpse: {}", item->display_name(),
                          result.error().message);
                room->add_object(item); // Drop in room if corpse full
            }
        }
    }

    // Add corpse to the room
    room->add_object(corpse);

    // Check if corpse should fall (if in flying sector)
    World().check_and_handle_object_falling(corpse, room);

    Log::info("Created corpse for {} with {} items in room {}", actor->display_name(), corpse->current_capacity(),
              room->id());
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

    // Skill check for rescue success
    // Base chance: 50% + (rescuer_level - enemy_level) * 2% + dexterity_bonus
    int rescuer_level = ctx.actor->stats().level;
    int enemy_level = enemy->stats().level;
    int dex_bonus = (ctx.actor->stats().dexterity - 10) / 2; // D&D-style modifier

    int success_chance = 50 + (rescuer_level - enemy_level) * 2 + dex_bonus * 3;
    success_chance = std::clamp(success_chance, 10, 95); // Always 10-95% chance

    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> roll(1, 100);

    if (roll(gen) > success_chance) {
        ctx.send(fmt::format("You fail to rescue {} from {}!", target->display_name(), enemy->display_name()));
        ctx.send_to_room(
            fmt::format("{} attempts to rescue {} but fails!", ctx.actor->display_name(), target->display_name()),
            true);
        // Failed rescue still provokes the enemy
        if (!FieryMUD::CombatManager::is_in_combat(*ctx.actor)) {
            ctx.actor->set_position(Position::Fighting);
            FieryMUD::CombatManager::start_combat(ctx.actor, enemy);
        }
        return CommandResult::Success; // Command executed, just failed the attempt
    }

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
    ctx.send_to_room(fmt::format("{} heroically rescues {} from {}!", ctx.actor->display_name(), target->display_name(),
                                 enemy->display_name()),
                     true);

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
    ctx.send_to_actor(enemy,
                      fmt::format("{} assists {} against you!", ctx.actor->display_name(), target->display_name()));
    ctx.send_to_room(fmt::format("{} assists {} against {}!", ctx.actor->display_name(), target->display_name(),
                                 enemy->display_name()),
                     true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_kick(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "kick", true, true);
}

Result<CommandResult> cmd_bash(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "bash", true, true);
}

Result<CommandResult> cmd_backstab(const CommandContext &ctx) {
    if (!ctx.actor->has_flag(ActorFlag::Hide) && !ctx.actor->has_flag(ActorFlag::Sneak)) {
        ctx.send_error("You need to be hidden to backstab someone!");
        return CommandResult::InvalidState;
    }
    if (ctx.actor->position() == Position::Fighting) {
        ctx.send_error("You can't backstab while fighting!");
        return CommandResult::InvalidState;
    }
    auto weapon = ctx.actor->equipment().get_main_weapon();
    if (!weapon) {
        ctx.send_error("You need a weapon to backstab!");
        return CommandResult::InvalidState;
    }
    ctx.actor->set_flag(ActorFlag::Hide, false);
    ctx.actor->set_flag(ActorFlag::Sneak, false);
    ctx.actor->stats().concealment = 0;
    return FieryMUD::execute_skill_command(ctx, "backstab", true, true);
}

// =============================================================================
// High-Priority Combat Skills
// =============================================================================

Result<CommandResult> cmd_disarm(const CommandContext &ctx) {
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You need to be fighting to disarm someone!");
        return CommandResult::InvalidState;
    }
    return FieryMUD::execute_skill_command(ctx, "disarm", true, false);
}

Result<CommandResult> cmd_disengage(const CommandContext &ctx) {
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You aren't fighting anyone.");
        return CommandResult::InvalidState;
    }
    auto enemy = FieryMUD::CombatManager::get_opponent(*ctx.actor);
    if (!enemy) {
        ctx.send_error("You aren't fighting anyone.");
        return CommandResult::InvalidState;
    }

    // Skill check: base 40% + level diff + DEX bonus
    int success_chance =
        40 + (ctx.actor->stats().level - enemy->stats().level) * 2 + (ctx.actor->stats().dexterity - 10) / 2 * 3;
    success_chance = std::clamp(success_chance, 10, 90);

    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> roll(1, 100);

    if (roll(gen) > success_chance) {
        ctx.send(fmt::format("You fail to disengage from {}!", enemy->display_name()));
        ctx.send_to_room(fmt::format("{} tries to disengage from combat but fails!", ctx.actor->display_name()), true);
        return CommandResult::Success;
    }

    FieryMUD::CombatManager::end_combat(ctx.actor);
    ctx.actor->set_position(Position::Standing);
    ctx.send("You skillfully disengage from combat.");
    ctx.send_to_room(fmt::format("{} skillfully disengages from combat.", ctx.actor->display_name()), true);
    ctx.send_to_actor(enemy, fmt::format("{} disengages from you.", ctx.actor->display_name()));
    return CommandResult::Success;
}

Result<CommandResult> cmd_retreat(const CommandContext &ctx) {
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You aren't fighting anyone.");
        return CommandResult::InvalidState;
    }
    if (ctx.arg_count() == 0) {
        ctx.send_error("Retreat in which direction?");
        return CommandResult::InvalidSyntax;
    }

    // Map direction argument to Direction enum
    std::string dir_arg = to_lower(std::string(ctx.arg(0)));
    using Dir = Direction;
    static const std::unordered_map<std::string, Direction> dir_map = {
        {"north", Dir::North}, {"n", Dir::North}, {"east", Dir::East}, {"e", Dir::East},
        {"south", Dir::South}, {"s", Dir::South}, {"west", Dir::West}, {"w", Dir::West},
        {"up", Dir::Up},       {"u", Dir::Up},    {"down", Dir::Down}, {"d", Dir::Down},
    };

    auto dir_it = dir_map.find(dir_arg);
    if (dir_it == dir_map.end()) {
        ctx.send_error("That's not a valid direction.");
        return CommandResult::InvalidSyntax;
    }
    Direction direction = dir_it->second;

    // Check if exit exists
    if (!ctx.room || !ctx.room->has_exit(direction)) {
        ctx.send_error("You can't retreat that way!");
        return CommandResult::InvalidTarget;
    }

    // Skill check: base 50% + DEX bonus
    int success_chance = 50 + (ctx.actor->stats().dexterity - 10) / 2 * 4;
    success_chance = std::clamp(success_chance, 15, 90);

    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> roll(1, 100);

    if (roll(gen) > success_chance) {
        ctx.send("You fail to retreat!");
        ctx.send_to_room(fmt::format("{} tries to retreat but stumbles!", ctx.actor->display_name()), true);
        return CommandResult::Success;
    }

    // End combat and move
    FieryMUD::CombatManager::end_combat(ctx.actor);
    ctx.actor->set_position(Position::Standing);

    auto dir_name = magic_enum::enum_name(direction);
    std::string dir_lower(dir_name);
    std::transform(dir_lower.begin(), dir_lower.end(), dir_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    ctx.send(fmt::format("You retreat {}!", dir_lower));
    ctx.send_to_room(fmt::format("{} retreats {}.", ctx.actor->display_name(), dir_lower), true);

    // Move actor through the exit
    auto *exit = ctx.room->get_exit(direction);
    if (exit && exit->to_room.is_valid()) {
        WorldManager::instance().move_actor_to_room(ctx.actor, exit->to_room);
        auto new_room = WorldManager::instance().get_room(exit->to_room);
        if (new_room) {
            auto room_desc = InformationCommands::format_room_for_actor(ctx.actor, new_room);
            ctx.send(room_desc);
        }
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_guard(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Guard who?");
        return CommandResult::InvalidSyntax;
    }

    auto target = ctx.find_actor_target(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }
    if (target == ctx.actor) {
        ctx.send_error("You can't guard yourself!");
        return CommandResult::InvalidTarget;
    }

    ctx.send(fmt::format("You begin guarding {}.", target->display_name()));
    ctx.send_to_actor(target, fmt::format("{} begins guarding you.", ctx.actor->display_name()));
    ctx.send_to_room(fmt::format("{} moves to guard {}.", ctx.actor->display_name(), target->display_name()), true);
    return CommandResult::Success;
}

Result<CommandResult> cmd_hitall(const CommandContext &ctx) {
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You need to be in combat to hit all enemies!");
        return CommandResult::InvalidState;
    }
    return FieryMUD::execute_skill_command(ctx, "hit all", false, false);
}

Result<CommandResult> cmd_bandage(const CommandContext &ctx) {
    if (ctx.actor->position() == Position::Fighting) {
        ctx.send_error("You can't bandage wounds while fighting!");
        return CommandResult::InvalidState;
    }
    return FieryMUD::execute_skill_command(ctx, "bandage", false, false);
}

Result<CommandResult> cmd_berserk(const CommandContext &ctx) {
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You need to be fighting to go berserk!");
        return CommandResult::InvalidState;
    }
    return FieryMUD::execute_skill_command(ctx, "berserk", false, false);
}

Result<CommandResult> cmd_breathe(const CommandContext &ctx) {
    // Breathe has sub-types: fire, frost, acid, gas, lightning
    std::string breath_type = "breathe fire"; // default
    if (ctx.arg_count() > 0) {
        std::string arg = to_lower(std::string(ctx.arg(0)));
        if (arg == "fire")
            breath_type = "breathe fire";
        else if (arg == "frost" || arg == "cold" || arg == "ice")
            breath_type = "breathe frost";
        else if (arg == "acid")
            breath_type = "breathe acid";
        else if (arg == "gas" || arg == "poison")
            breath_type = "breathe gas";
        else if (arg == "lightning" || arg == "electric")
            breath_type = "breathe lightning";
        else {
            // Treat as target name, default to fire
            breath_type = "breathe fire";
        }
    }
    return FieryMUD::execute_skill_command(ctx, breath_type, true, true);
}

Result<CommandResult> cmd_layhands(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "lay hands", false, false);
}

Result<CommandResult> cmd_throatcut(const CommandContext &ctx) {
    if (!ctx.actor->has_flag(ActorFlag::Hide) && !ctx.actor->has_flag(ActorFlag::Sneak)) {
        ctx.send_error("You need to be hidden to cut someone's throat!");
        return CommandResult::InvalidState;
    }
    if (ctx.actor->position() == Position::Fighting) {
        ctx.send_error("You can't cut throats while fighting!");
        return CommandResult::InvalidState;
    }
    auto weapon = ctx.actor->equipment().get_main_weapon();
    if (!weapon) {
        ctx.send_error("You need a weapon to cut someone's throat!");
        return CommandResult::InvalidState;
    }
    ctx.actor->set_flag(ActorFlag::Hide, false);
    ctx.actor->set_flag(ActorFlag::Sneak, false);
    ctx.actor->stats().concealment = 0;
    return FieryMUD::execute_skill_command(ctx, "throatcut", true, true);
}

Result<CommandResult> cmd_claw(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "claw", true, true);
}

// =============================================================================
// Medium-Priority Combat Skills
// =============================================================================

Result<CommandResult> cmd_buck(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "buck", true, false);
}

Result<CommandResult> cmd_cartwheel(const CommandContext &ctx) {
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You need to be fighting to cartwheel!");
        return CommandResult::InvalidState;
    }
    return FieryMUD::execute_skill_command(ctx, "cartwheel", true, false);
}

Result<CommandResult> cmd_doorbash(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Bash which door?");
        return CommandResult::InvalidSyntax;
    }
    std::string dir_arg = to_lower(std::string(ctx.arg(0)));
    using Dir = Direction;
    static const std::unordered_map<std::string, Direction> dir_map = {
        {"north", Dir::North}, {"n", Dir::North}, {"east", Dir::East}, {"e", Dir::East},
        {"south", Dir::South}, {"s", Dir::South}, {"west", Dir::West}, {"w", Dir::West},
        {"up", Dir::Up},       {"u", Dir::Up},    {"down", Dir::Down}, {"d", Dir::Down},
    };

    auto dir_it = dir_map.find(dir_arg);
    if (dir_it == dir_map.end()) {
        ctx.send_error("Bash the door in which direction?");
        return CommandResult::InvalidSyntax;
    }
    Direction direction = dir_it->second;

    if (!ctx.room || !ctx.room->has_exit(direction)) {
        ctx.send_error("There's no door there.");
        return CommandResult::InvalidTarget;
    }

    auto *exit = ctx.room->get_exit(direction);
    if (!exit || !exit->is_closed) {
        ctx.send_error("It's already open!");
        return CommandResult::InvalidState;
    }

    // Skill check: base 60% + STR bonus
    int success_chance = 60 + (ctx.actor->stats().strength - 10) / 2 * 5;
    success_chance = std::clamp(success_chance, 15, 95);

    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> roll(1, 100);

    auto dir_name = magic_enum::enum_name(direction);
    std::string dir_lower(dir_name);
    std::transform(dir_lower.begin(), dir_lower.end(), dir_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (roll(gen) > success_chance) {
        ctx.send(fmt::format("You slam into the {} door but it holds fast!", dir_lower));
        ctx.send_to_room(
            fmt::format("{} slams into the {} door but it holds fast!", ctx.actor->display_name(), dir_lower), true);
        return CommandResult::Success;
    }

    // Success - open the door via mutable exit
    auto *mutable_exit = ctx.room->get_exit_mutable(direction);
    if (mutable_exit) {
        mutable_exit->is_closed = false;
        mutable_exit->is_locked = false;
    }
    ctx.send(fmt::format("You bash open the {} door!", dir_lower));
    ctx.send_to_room(fmt::format("{} bashes open the {} door!", ctx.actor->display_name(), dir_lower), true);
    return CommandResult::Success;
}

Result<CommandResult> cmd_electrify(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "electrify", false, false);
}

Result<CommandResult> cmd_gouge(const CommandContext &ctx) {
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You need to be fighting to gouge someone!");
        return CommandResult::InvalidState;
    }
    return FieryMUD::execute_skill_command(ctx, "eye gouge", true, false);
}

Result<CommandResult> cmd_rend(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "rend", true, true);
}

Result<CommandResult> cmd_roar(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "roar", false, false);
}

Result<CommandResult> cmd_roundhouse(const CommandContext &ctx) {
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You need to be fighting to roundhouse kick!");
        return CommandResult::InvalidState;
    }
    return FieryMUD::execute_skill_command(ctx, "roundhouse", true, false);
}

Result<CommandResult> cmd_springleap(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "springleap", true, true);
}

Result<CommandResult> cmd_stomp(const CommandContext &ctx) {
    return FieryMUD::execute_skill_command(ctx, "ground shaker", true, true);
}

Result<CommandResult> cmd_sweep(const CommandContext &ctx) {
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You need to be fighting to sweep!");
        return CommandResult::InvalidState;
    }
    return FieryMUD::execute_skill_command(ctx, "sweep", true, false);
}

Result<CommandResult> cmd_tripup(const CommandContext &ctx) {
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You need to be fighting to trip someone!");
        return CommandResult::InvalidState;
    }
    return FieryMUD::execute_skill_command(ctx, "trip up", true, false);
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands().command("kill", cmd_kill).alias("murder").category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("hit", cmd_hit).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands()
        .command("cast", cmd_cast)
        .alias("c")
        .alias("chant")
        .alias("perform")
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands().command("abort", cmd_abort).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("flee", cmd_flee).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("release", cmd_release).category("Death").privilege(PrivilegeLevel::Player).build();

    // Combat skill commands
    Commands().command("rescue", cmd_rescue).alias("res").category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("assist", cmd_assist).alias("ass").category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("kick", cmd_kick).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands()
        .command("bash", cmd_bash)
        .alias("bodyslam")
        .alias("maul")
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("backstab", cmd_backstab)
        .alias("bs")
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    // High-priority combat skills
    Commands().command("disarm", cmd_disarm).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("disengage", cmd_disengage).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("retreat", cmd_retreat).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("guard", cmd_guard).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands()
        .command("hitall", cmd_hitall)
        .alias("tantrum")
        .category("Combat")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands().command("bandage", cmd_bandage).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("berserk", cmd_berserk).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("breathe", cmd_breathe).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("layhands", cmd_layhands).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("throatcut", cmd_throatcut).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("claw", cmd_claw).category("Combat").privilege(PrivilegeLevel::Player).build();

    // Medium-priority combat skills
    Commands().command("buck", cmd_buck).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("cartwheel", cmd_cartwheel).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("doorbash", cmd_doorbash).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("electrify", cmd_electrify).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("gouge", cmd_gouge).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("rend", cmd_rend).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("roar", cmd_roar).alias("howl").category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("roundhouse", cmd_roundhouse).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("springleap", cmd_springleap).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("stomp", cmd_stomp).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("sweep", cmd_sweep).category("Combat").privilege(PrivilegeLevel::Player).build();

    Commands().command("tripup", cmd_tripup).category("Combat").privilege(PrivilegeLevel::Player).build();

    return Success();
}

} // namespace CombatCommands
