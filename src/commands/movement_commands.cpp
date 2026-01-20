#include "movement_commands.hpp"
#include "builtin_commands.hpp"

#include "../core/actor.hpp"
#include "../text/text_format.hpp"
#include "../core/object.hpp"
#include "../world/room.hpp"
#include "../world/world_manager.hpp"

#include <algorithm>

namespace MovementCommands {

namespace {

/**
 * Check if actor's race has natural flying ability.
 * Races like sprites, pixies, and avians can fly naturally.
 */
bool race_can_fly(std::string_view race) {
    // Convert to lowercase for comparison
    std::string lower_race{race};
    std::transform(lower_race.begin(), lower_race.end(), lower_race.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Races with natural flight ability
    static const std::unordered_set<std::string> flying_races = {
        "sprite", "pixie", "fairy", "faerie", "fae",
        "avian", "aarakocra", "kenku", "tengu",
        "imp", "demon", "devil", "succubus", "incubus",
        "dragon", "dragonborn",  // Some dragon variants can fly
        "air elemental", "djinn", "djinni", "genie",
        "ghost", "specter", "spectre", "wraith", "banshee",
        "angel", "archon", "deva", "solar",
        "harpy", "gargoyle", "pteranodon"
    };

    return flying_races.contains(lower_race);
}

/**
 * Check if any equipped item grants the Fly effect.
 */
bool equipment_grants_flying(const Actor& actor) {
    for (const auto& item : actor.equipment().get_all_equipped()) {
        if (item && item->has_effect(EffectFlag::Fly)) {
            return true;
        }
    }
    return false;
}

} // anonymous namespace

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

    // Check for Flying effect (from spell or other magical effects)
    if (ctx.actor->has_flag(ActorFlag::Flying)) {
        can_fly = true;
    }

    // Check if race has natural flying ability
    if (!can_fly && race_can_fly(ctx.actor->race())) {
        can_fly = true;
    }

    // Check if equipment grants flying (wings, flying carpet, etc.)
    if (!can_fly && equipment_grants_flying(*ctx.actor)) {
        can_fly = true;
    }

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
// Enter/Leave Commands
// =============================================================================

Result<CommandResult> cmd_enter(const CommandContext &ctx) {
    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    // Can't enter while fighting
    if (ctx.actor->is_fighting()) {
        ctx.send_error("You are too busy fighting to leave right now.");
        return CommandResult::InvalidState;
    }

    // If argument provided, try to enter a portal/object
    if (ctx.arg_count() > 0) {
        std::string_view target_name = ctx.arg(0);

        // Look for a portal object in the room
        auto portals = ctx.room->find_objects_by_keyword(target_name);
        if (portals.empty()) {
            ctx.send_error(fmt::format("There is no '{}' here.", target_name));
            return CommandResult::InvalidTarget;
        }

        auto portal = portals.front();

        // Check if it's a portal
        if (portal->type() != ObjectType::Portal) {
            ctx.send_error("You can't enter that!");
            return CommandResult::InvalidTarget;
        }

        // Check level requirement
        if (portal->level() > ctx.actor->stats().level) {
            ctx.send_error("You are not powerful enough to enter this portal.");
            return CommandResult::InvalidState;
        }

        // Move actor through portal
        ctx.send(fmt::format("You enter {}.", portal->short_description()));
        ctx.send_to_room(fmt::format("{} enters {}.", ctx.actor->display_name(), portal->short_description()), true);

        // Portal travel requires destination room storage on Object class
        // When implemented: get destination from portal, use WorldManager to move actor
        ctx.send("Portal travel not yet fully implemented - portal destination storage needed.");
        return CommandResult::Success;
    }

    // No argument - try to enter an adjacent indoor room
    // Check sector type for indoor status (Inside sector = indoors)
    if (ctx.room->sector_type() == SectorType::Inside) {
        ctx.send("You are already indoors.");
        return CommandResult::InvalidState;
    }

    // Look for an exit that is passable
    // Note: Without WorldManager access we can't easily check destination room flags
    // For now, just find first valid exit and try to move there
    for (int dir = 0; dir < 6; ++dir) {
        Direction direction = static_cast<Direction>(dir);
        const auto* exit = ctx.room->get_exit(direction);
        if (exit && exit->is_passable()) {
            // Try to move - execute_movement handles validation
            return BuiltinCommands::Helpers::execute_movement(ctx, direction);
        }
    }

    ctx.send("You can't seem to find anything to enter.");
    return CommandResult::InvalidTarget;
}

Result<CommandResult> cmd_leave(const CommandContext &ctx) {
    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    // Can't leave while fighting
    if (ctx.actor->is_fighting()) {
        ctx.send_error("You are too busy fighting to leave!");
        return CommandResult::InvalidState;
    }

    // Must be indoors to leave (sector type Inside = indoors)
    if (ctx.room->sector_type() != SectorType::Inside) {
        ctx.send("You are outside... where do you want to go?");
        return CommandResult::InvalidState;
    }

    // Look for an exit to an outdoor room
    for (int dir = 0; dir < 6; ++dir) {
        Direction direction = static_cast<Direction>(dir);
        const auto* exit = ctx.room->get_exit(direction);
        if (exit && !exit->is_closed) {
            auto dest_room = WorldManager::instance().get_room(exit->to_room);
            if (dest_room && dest_room->sector_type() != SectorType::Inside) {
                // Found an outdoor room - move there
                return BuiltinCommands::Helpers::execute_movement(ctx, direction);
            }
        }
    }

    ctx.send("I see no obvious exits to the outside.");
    return CommandResult::InvalidTarget;
}

// =============================================================================
// Mount Commands
// =============================================================================

Result<CommandResult> cmd_mount(const CommandContext &ctx) {
    // Check if already mounted
    if (ctx.actor->is_mounted()) {
        ctx.send_error("You are already mounted.");
        return CommandResult::InvalidState;
    }

    if (ctx.actor->is_fighting()) {
        ctx.send_error("You are too busy fighting to try that right now.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send_error("Mount what?");
        return CommandResult::InvalidSyntax;
    }

    std::string_view target_name = ctx.arg(0);

    // Find the target in the room
    auto target = ctx.find_actor_target(target_name);
    if (!target) {
        ctx.send_error("There is no one by that name here.");
        return CommandResult::InvalidTarget;
    }

    // Can't mount players
    if (std::dynamic_pointer_cast<Player>(target)) {
        ctx.send_error("Ehh... no.");
        return CommandResult::InvalidTarget;
    }

    // Check if target is mountable
    auto mob = std::dynamic_pointer_cast<Mobile>(target);
    if (!mob || !mob->has_trait(MobTrait::Mount)) {
        ctx.send_error("You can't mount that!");
        return CommandResult::InvalidTarget;
    }

    // Check if mount already has a rider
    if (target->has_rider()) {
        ctx.send_error(fmt::format("{} already has a rider.", target->display_name()));
        return CommandResult::InvalidState;
    }

    // Mount the target - set both sides of the relationship
    ctx.actor->set_mounted_on(target);
    target->set_rider(ctx.actor);

    ctx.send(fmt::format("You mount {}.", target->display_name()));
    ctx.send_to_room(fmt::format("{} mounts {}.", ctx.actor->display_name(), target->display_name()), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_dismount(const CommandContext &ctx) {
    // Check if actually mounted
    if (!ctx.actor->is_mounted()) {
        ctx.send("You aren't riding anything.");
        return CommandResult::InvalidState;
    }

    if (ctx.actor->is_fighting()) {
        ctx.send_error("You would get hacked to pieces if you dismount now!");
        return CommandResult::InvalidState;
    }

    auto mount = ctx.actor->get_mount();
    ctx.actor->dismount();

    if (mount) {
        ctx.send(fmt::format("You dismount {}.", mount->display_name()));
        ctx.send_to_room(fmt::format("{} dismounts {}.", ctx.actor->display_name(), mount->display_name()), true);
    } else {
        ctx.send("You dismount.");
        ctx.send_to_room(fmt::format("{} dismounts.", ctx.actor->display_name()), true);
    }

    return CommandResult::Success;
}

// =============================================================================
// Recall Command
// =============================================================================

// Recall cooldown effect name and duration
// 1 MUD hour = 75 real seconds, so 12 MUD hours = 900 seconds = 15 real minutes
constexpr std::string_view RECALL_COOLDOWN_EFFECT = "Recall Cooldown";
constexpr double RECALL_COOLDOWN_HOURS = 12.0;  // 15 real minutes

Result<CommandResult> cmd_recall(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Error{ErrorCode::InvalidState, "No actor context"});
    }

    // Check for recall cooldown
    if (ctx.actor->has_effect(std::string{RECALL_COOLDOWN_EFFECT})) {
        ctx.send_error("You must wait before recalling again.");
        return CommandResult::InvalidState;
    }

    // Can't recall while fighting
    if (ctx.actor->is_fighting()) {
        ctx.send_error("You can't concentrate enough to recall while fighting!");
        return CommandResult::InvalidState;
    }

    // Can't recall while sitting/resting/sleeping
    if (ctx.actor->position() != Position::Standing) {
        ctx.send_error("You need to be standing to recall.");
        return CommandResult::InvalidState;
    }

    auto current_room = ctx.actor->current_room();
    if (!current_room) {
        ctx.send_error("You are nowhere!");
        return CommandResult::InvalidState;
    }

    // Get the recall destination from player's recall_room (touchstone), or default to Midgaard Temple
    EntityId recall_room_id(30, 0);  // Default recall point
    if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        if (player->recall_room() != INVALID_ENTITY_ID) {
            recall_room_id = player->recall_room();
        }
    }
    auto dest_room = WorldManager::instance().get_room(recall_room_id);

    if (!dest_room) {
        ctx.send_error("The recall point no longer exists!");
        return CommandResult::SystemError;
    }

    // Already at recall point?
    if (current_room.get() == dest_room.get()) {
        ctx.send_info("You are already at your recall point.");
        return CommandResult::Success;
    }

    // Can't recall while already recalling/casting
    if (ctx.actor->is_casting()) {
        ctx.send_error("You are already concentrating on something.");
        return CommandResult::InvalidState;
    }

    // Start the recall concentration (3 second delay = 6 ticks at 0.5s per tick)
    constexpr int RECALL_TICKS = 6;  // 3 seconds

    Actor::CastingState state{
        .ability_id = RECALL_ABILITY_ID,  // From header
        .ability_name = "recall",
        .ticks_remaining = RECALL_TICKS,
        .total_ticks = RECALL_TICKS,
        .target = {},
        .target_name = "",
        .circle = 0,
        .quickcast_applied = false
    };
    ctx.actor->start_casting(state);

    // Send concentration messages
    ctx.send("<cyan>You close your eyes and begin to concentrate on your recall point...</>");
    ctx.send_to_room(TextFormat::format(
        "<cyan>{actor} closes {actor.pos} eyes and begins to concentrate...</>",
        ctx.actor.get()), true);

    return CommandResult::Success;
}

void complete_recall(std::shared_ptr<Actor> actor) {
    if (!actor) return;

    auto current_room = actor->current_room();
    if (!current_room) {
        actor->send_message("<red>Your recall fails - you are nowhere!</>\n");
        return;
    }

    // Get the recall destination from player's recall_room (touchstone), or default to Midgaard Temple
    EntityId recall_room_id(30, 0);  // Default recall point
    if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
        if (player->recall_room() != INVALID_ENTITY_ID) {
            recall_room_id = player->recall_room();
        }
    }
    auto dest_room = WorldManager::instance().get_room(recall_room_id);

    if (!dest_room) {
        actor->send_message("<red>Your recall fails - the recall point no longer exists!</>\n");
        return;
    }

    // Already at recall point?
    if (current_room.get() == dest_room.get()) {
        actor->send_message("<cyan>You complete your concentration, but you're already at your recall point.</>\n");
        return;
    }

    // Send departure message to old room
    auto departure_msg = TextFormat::format(
        "<cyan>{actor} vanishes in a shimmer of light.</>",
        actor.get());
    for (const auto& room_actor : current_room->contents().actors) {
        if (room_actor && room_actor != actor) {
            room_actor->send_message(departure_msg + "\n");
        }
    }

    // Perform the teleport
    current_room->remove_actor(actor->id());
    dest_room->add_actor(actor);
    actor->move_to(dest_room);

    // Send messages to actor
    actor->send_message("<cyan>You vanish in a shimmer of light and appear at your recall point!</>\n");

    // Send arrival message to destination room
    auto arrival_msg = TextFormat::format(
        "<cyan>{actor} appears in a shimmer of light.</>",
        actor.get());
    for (const auto& room_actor : dest_room->contents().actors) {
        if (room_actor && room_actor != actor) {
            room_actor->send_message(arrival_msg + "\n");
        }
    }

    // Show the new room
    actor->send_message(dest_room->get_room_description(actor.get()));

    // Apply recall cooldown effect
    ActiveEffect cooldown{
        .name = std::string{RECALL_COOLDOWN_EFFECT},
        .source = "recall",
        .flag = ActorFlag::None,  // No flag needed for cooldown effects
        .duration_hours = RECALL_COOLDOWN_HOURS,
        .modifier_value = 0,
        .modifier_stat = "",
        .applied_at = std::chrono::steady_clock::now()
    };
    actor->add_effect(cooldown);
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

    Commands()
        .command("enter", cmd_enter)
        .category("Movement")
        .privilege(PrivilegeLevel::Player)
        .description("Enter a portal or indoor area")
        .usable_while_sitting(false)
        .usable_in_combat(false)
        .build();

    Commands()
        .command("leave", cmd_leave)
        .category("Movement")
        .privilege(PrivilegeLevel::Player)
        .description("Leave an indoor area")
        .usable_while_sitting(false)
        .usable_in_combat(false)
        .build();

    Commands()
        .command("mount", cmd_mount)
        .category("Movement")
        .privilege(PrivilegeLevel::Player)
        .description("Mount a creature")
        .usable_while_sitting(false)
        .usable_in_combat(false)
        .build();

    Commands()
        .command("dismount", cmd_dismount)
        .category("Movement")
        .privilege(PrivilegeLevel::Player)
        .description("Dismount from a creature")
        .usable_in_combat(false)
        .build();

    Commands()
        .command("recall", cmd_recall)
        .category("Movement")
        .privilege(PrivilegeLevel::Player)
        .description("Return to your recall point")
        .usable_while_sitting(false)
        .usable_in_combat(false)
        .build();

    return Success();
}

} // namespace MovementCommands