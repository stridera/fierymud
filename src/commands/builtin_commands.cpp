#include "builtin_commands.hpp"

// Command module headers for registration
#include "admin_commands.hpp"
#include "character_commands.hpp"
#include "combat_commands.hpp"
#include "communication_commands.hpp"
#include "economy_commands.hpp"
#include "group_commands.hpp"
#include "information_commands.hpp"
#include "magic_commands.hpp"
#include "movement_commands.hpp"
#include "object_commands.hpp"
#include "position_commands.hpp"
#include "social_commands.hpp"
#include "system_commands.hpp"

// Core dependencies used by helper functions
#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../core/object.hpp"
#include "../text/string_utils.hpp"
#include "../world/room.hpp"

#include <algorithm>
#include <sstream>

namespace BuiltinCommands {

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_all_commands() {
    Log::info("Registering built-in commands...");

    // Register commands from each module
    if (auto result = InformationCommands::register_commands(); !result) {
        Log::error("Failed to register information commands: {}", result.error().message);
        return result;
    }

    if (auto result = CommunicationCommands::register_commands(); !result) {
        Log::error("Failed to register communication commands: {}", result.error().message);
        return result;
    }

    if (auto result = MovementCommands::register_commands(); !result) {
        Log::error("Failed to register movement commands: {}", result.error().message);
        return result;
    }

    if (auto result = PositionCommands::register_commands(); !result) {
        Log::error("Failed to register position commands: {}", result.error().message);
        return result;
    }

    if (auto result = CombatCommands::register_commands(); !result) {
        Log::error("Failed to register combat commands: {}", result.error().message);
        return result;
    }

    if (auto result = EconomyCommands::register_commands(); !result) {
        Log::error("Failed to register economy commands: {}", result.error().message);
        return result;
    }

    if (auto result = GroupCommands::register_commands(); !result) {
        Log::error("Failed to register group commands: {}", result.error().message);
        return result;
    }

    if (auto result = CharacterCommands::register_commands(); !result) {
        Log::error("Failed to register character commands: {}", result.error().message);
        return result;
    }

    if (auto result = MagicCommands::register_commands(); !result) {
        Log::error("Failed to register magic commands: {}", result.error().message);
        return result;
    }

    if (auto result = ObjectCommands::register_commands(); !result) {
        Log::error("Failed to register object commands: {}", result.error().message);
        return result;
    }

    if (auto result = SystemCommands::register_commands(); !result) {
        Log::error("Failed to register system commands: {}", result.error().message);
        return result;
    }

    if (auto result = AdminCommands::register_commands(); !result) {
        Log::error("Failed to register admin commands: {}", result.error().message);
        return result;
    }

    // Social commands are loaded from database
    if (auto result = SocialCommands::initialize(); !result) {
        Log::error("Failed to initialize social commands: {}", result.error().message);
        return std::unexpected(result.error());
    }
    Log::info("Loaded {} social commands", SocialCommands::social_count());

    Log::info("Built-in commands registered successfully.");
    return Success();
}

Result<void> unregister_all_commands() {
    Log::info("Unregistering built-in commands...");

    auto &cmd_system = CommandSystem::instance();

    // Information Commands
    const std::vector<std::string> commands_to_unregister = {
        "look", "examine", "who", "where", "inventory", "equipment", "score", "time", "weather", "stat",
        // Communication Commands
        "say", "tell", "emote", "whisper", "shout", "gossip",
        // Movement Commands
        "north", "south", "east", "west", "up", "down", "exits",
        // Combat Commands
        "kill", "hit", "cast", "flee", "release",
        // Object Commands
        "get", "drop", "put", "give", "wear", "wield", "remove", "light", "eat", "drink", "open", "close", "lock",
        "unlock", "list", "buy", "sell",
        // System Commands
        "quit", "save", "help", "commands", "richtest", "clientinfo", "prompt",
        // Social Commands
        "smile", "nod", "wave", "bow", "laugh",
        // Admin Commands
        "shutdown", "goto", "teleport", "summon", "setweather", "reloadzone", "savezone", "reloadallzones", "filewatch",
        "dumpworld"};

    for (const auto &cmd : commands_to_unregister) {
        cmd_system.unregister_command(cmd);
    }

    Log::info("Unregistered {} built-in commands", commands_to_unregister.size());
    return Success();
}

// =============================================================================
// Helper Functions Namespace
// =============================================================================

namespace Helpers {

std::string format_room_description(std::shared_ptr<Room> room, std::shared_ptr<Actor> viewer) {
    if (!room) {
        return "You are in the void.";
    }

    std::ostringstream desc;
    desc << fmt::format("{}\n", room->name());
    desc << fmt::format("{}\n", room->description());

    // Add exits
    std::string exits = format_exits(room);
    if (!exits.empty()) {
        desc << fmt::format("\n{}\n", exits);
    }

    // Add objects in room
    auto objects = room->contents().objects;
    if (!objects.empty()) {
        desc << "\nYou see:\n";
        for (const auto &obj : objects) {
            if (obj) {
                desc << fmt::format("  {}\n", obj->short_description());
            }
        }
    }

    // Add other actors
    auto actors = room->contents().actors;
    bool found_others = false;
    for (const auto &actor : actors) {
        if (actor && actor != viewer) {
            if (!found_others) {
                desc << "\nAlso here:\n";
                found_others = true;
            }
            desc << fmt::format("  {}\n", actor->short_description());
        }
    }

    return desc.str();
}

std::string format_object_description(std::shared_ptr<Object> obj, [[maybe_unused]] std::shared_ptr<Actor> viewer) {
    if (!obj) {
        return "You see nothing special.";
    }

    std::ostringstream desc;
    desc << fmt::format("{}\n", obj->short_description());
    desc << fmt::format("{}\n", obj->description());

    // Add object-specific details based on type
    if (obj->is_container()) {
        const auto &container_info = obj->container_info();
        if (container_info.closeable) {
            desc << fmt::format("It appears to be {}.\n", container_info.closed ? "closed" : "open");
            if (container_info.lockable) {
                desc << fmt::format("It is {}.\n", container_info.locked ? "locked" : "unlocked");
            }
        }
        // Show capacity if not full/empty and container is open
        if (!container_info.closed) {
            if (container_info.capacity > 0) {
                desc << fmt::format("It can hold {} items.\n", container_info.capacity);
            }
        }
    }

    if (obj->is_weapon()) {
        const auto &damage = obj->damage_profile();
        desc << fmt::format("Weapon damage: {}\n", damage.to_dice_string());
    }

    if (obj->is_armor()) {
        desc << fmt::format("Armor class: {}\n", obj->armor_class());
    }

    // Show condition and quality
    if (obj->condition() < 100) {
        desc << fmt::format("Condition: {} ({})\n", obj->condition(), obj->quality_description());
    }

    // Show weight and value for examination
    desc << fmt::format("It weighs {} pounds", obj->weight());
    if (obj->value() > 0) {
        desc << fmt::format(" and is worth {} gold coins", obj->value());
    }
    desc << ".\n";

    // Show extra descriptions available (if any exist)
    const auto &extras = obj->get_all_extra_descriptions();
    if (!extras.empty()) {
        desc << "\nYou notice some details you could examine more closely: ";
        bool first = true;
        for (const auto &extra : extras) {
            if (!first)
                desc << ", ";
            // Show the first keyword as the examinable feature name
            if (!extra.keywords.empty()) {
                desc << extra.keywords[0];
            }
            first = false;
        }
        desc << "\n";
    }

    return desc.str();
}

std::string format_actor_description(std::shared_ptr<Actor> target, [[maybe_unused]] std::shared_ptr<Actor> viewer) {
    if (!target) {
        return "You see nothing special.";
    }

    std::ostringstream desc;

    // Show the actor's name and short description
    desc << fmt::format("{}\n", target->short_description());

    // Show the longer description if available and different from short description
    if (!target->description().empty() && target->description() != target->short_description()) {
        desc << fmt::format("{}\n", target->description());
    }

    // Show position and status information
    if (target->position() != Position::Standing) {
        desc << fmt::format("{} is {}.\n", target->display_name(), ActorUtils::get_position_name(target->position()));
    }

    // Show health status for living actors
    if (target->is_alive()) {
        const auto &stats = target->stats();
        double health_percent = static_cast<double>(stats.hit_points) / stats.max_hit_points * 100.0;

        if (health_percent < 25.0) {
            desc << fmt::format("{} looks nearly dead.\n", target->display_name());
        } else if (health_percent < 50.0) {
            desc << fmt::format("{} looks badly wounded.\n", target->display_name());
        } else if (health_percent < 75.0) {
            desc << fmt::format("{} looks wounded.\n", target->display_name());
        } else if (health_percent < 100.0) {
            desc << fmt::format("{} looks slightly hurt.\n", target->display_name());
        } else {
            desc << fmt::format("{} is in excellent condition.\n", target->display_name());
        }
    }

    // Show equipment highlights - main weapon and armor
    if (auto main_weapon = target->equipment().get_main_weapon()) {
        desc << fmt::format("{} is wielding {}.\n", target->display_name(), main_weapon->short_description());
    }

    if (auto off_weapon = target->equipment().get_off_weapon()) {
        desc << fmt::format("{} is holding {} in the off-hand.\n", target->display_name(),
                            off_weapon->short_description());
    }

    // Show visible armor or clothing
    auto equipped_items = target->equipment().get_all_equipped();
    for (const auto &item : equipped_items) {
        if (item && item->is_armor()) {
            desc << fmt::format("{} is wearing {}.\n", target->display_name(), item->short_description());
            break; // Just show one piece of notable armor
        }
    }

    // Show fighting status
    if (target->is_fighting()) {
        desc << fmt::format("{} is engaged in combat!\n", target->display_name());
    }

    // Show special flags/conditions
    if (target->has_flag(ActorFlag::Invisible)) {
        desc << fmt::format("{} appears translucent.\n", target->display_name());
    }
    if (target->has_flag(ActorFlag::Sanctuary)) {
        desc << fmt::format("{} glows with a white aura.\n", target->display_name());
    }
    if (target->has_flag(ActorFlag::Flying)) {
        desc << fmt::format("{} is hovering above the ground.\n", target->display_name());
    }

    return desc.str();
}

std::string format_inventory(std::shared_ptr<Actor> actor) {
    if (!actor) {
        return "You have no inventory.";
    }

    auto inventory = actor->inventory().get_all_items();
    if (inventory.empty()) {
        return "You are not carrying anything.";
    }

    std::ostringstream inv;
    inv << "You are carrying:\n";
    for (const auto &item : inventory) {
        if (item) {
            inv << fmt::format("  {}\n", item->short_description());
        }
    }

    return inv.str();
}

std::string get_equipment_slot_display_name(EquipSlot slot) {
    switch (slot) {
    case EquipSlot::Light:
        return "<used as light>";
    case EquipSlot::Finger_R:
        return "<worn on right finger>";
    case EquipSlot::Finger_L:
        return "<worn on left finger>";
    case EquipSlot::Neck1:
        return "<worn around neck>";
    case EquipSlot::Neck2:
        return "<worn around neck>";
    case EquipSlot::Body:
        return "<worn on body>";
    case EquipSlot::Head:
        return "<worn on head>";
    case EquipSlot::Legs:
        return "<worn on legs>";
    case EquipSlot::Feet:
        return "<worn on feet>";
    case EquipSlot::Hands:
        return "<worn on hands>";
    case EquipSlot::Arms:
        return "<worn on arms>";
    case EquipSlot::Shield:
        return "<worn as shield>";
    case EquipSlot::About:
        return "<worn about body>";
    case EquipSlot::Waist:
        return "<worn around waist>";
    case EquipSlot::Wrist_R:
        return "<worn on right wrist>";
    case EquipSlot::Wrist_L:
        return "<worn on left wrist>";
    case EquipSlot::Wield:
        return "<wielded>";
    case EquipSlot::Hold:
        return "<held>";
    case EquipSlot::Float:
        return "<floating nearby>";
    default:
        return "<worn>";
    }
}

std::string format_equipment(std::shared_ptr<Actor> actor) {
    if (!actor) {
        return "You have no equipment.";
    }

    auto equipment = actor->equipment().get_all_equipped_with_slots();
    if (equipment.empty()) {
        return "You are not wearing anything.";
    }

    std::ostringstream eq;
    eq << "You are wearing:\n";
    for (const auto &[slot, item] : equipment) {
        if (item) {
            eq << fmt::format("{} {}\n", get_equipment_slot_display_name(slot), item->short_description());
        }
    }

    return eq.str();
}

std::string format_who_list(const std::vector<std::shared_ptr<Actor>> &actors) {
    std::ostringstream who;
    who << fmt::format("Players currently online ({}):\n", actors.size());

    for (const auto &actor : actors) {
        if (actor) {
            who << fmt::format("  {} (Level {})\n", actor->display_name(), actor->stats().level);
        }
    }

    return who.str();
}

std::string format_exits(std::shared_ptr<Room> room) {
    if (!room) {
        return "";
    }

    auto exits = room->get_visible_exits();
    if (exits.empty()) {
        return "There are no obvious exits.";
    }

    std::ostringstream exit_str;
    exit_str << "Obvious exits: ";

    bool first = true;
    for (auto dir : exits) {
        if (!first)
            exit_str << ", ";
        exit_str << RoomUtils::get_direction_name(dir);
        first = false;
    }

    return exit_str.str();
}

bool can_move_direction(std::shared_ptr<Actor> actor, Direction dir, std::string &failure_reason) {
    if (!actor) {
        failure_reason = "Invalid actor";
        return false;
    }

    auto room = actor->current_room();
    if (!room) {
        failure_reason = "You are not in a room";
        return false;
    }

    if (!room->has_exit(dir)) {
        failure_reason = "There's no exit in that direction";
        return false;
    }

    // Check if actor is in combat
    if (actor->position() == Position::Fighting) {
        failure_reason = "You can't leave while fighting! Use 'flee' to escape combat";
        return false;
    }

    // Check if actor is dead (but allow ghosts limited movement)
    if (actor->position() == Position::Dead) {
        failure_reason = "You can't move while you're dead!";
        return false;
    }
    // Ghosts can move but with restrictions (this will be handled elsewhere)

    // TODO: Add more movement checks (doors, etc.)

    return true;
}

Result<CommandResult> execute_movement(const CommandContext &ctx, Direction dir) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    std::string failure_reason;
    if (!can_move_direction(ctx.actor, dir, failure_reason)) {
        ctx.send_error(fmt::format("Cannot move {}: {}", magic_enum::enum_name(dir), failure_reason));
        return CommandResult::InvalidState;
    }

    auto result = ctx.move_actor_direction(dir);
    if (!result) {
        ctx.send_error(fmt::format("Movement failed: {}", result.error().message));
        return CommandResult::ResourceError;
    }

    // Send movement confirmation message
    std::string dir_name = to_lowercase(magic_enum::enum_name(dir));
    ctx.send(fmt::format("You move {}.", dir_name));

    // Automatically show the new room after movement
    auto look_result = InformationCommands::cmd_look(ctx);
    if (!look_result) {
        ctx.send_error("Movement succeeded but could not display new room");
    }

    return CommandResult::Success;
}

void send_communication(const CommandContext &ctx, std::string_view message, MessageType type,
                        std::string_view channel_name) {
    std::string formatted_message = format_communication(ctx.actor, message, channel_name);

    switch (type) {
    case MessageType::Say:
        ctx.send(fmt::format("You say, '{}'", message));
        ctx.send_to_room(formatted_message, true);
        break;
    case MessageType::Broadcast:
        ctx.send(fmt::format("You {}, '{}'", channel_name, message));
        ctx.send_to_all(formatted_message);
        break;
    case MessageType::Channel:
        ctx.send(fmt::format("You {}, '{}'", channel_name, message));
        ctx.send_to_all(formatted_message);
        break;
    default:
        ctx.send_to_room(formatted_message, false);
        break;
    }
}

std::string format_communication(std::shared_ptr<Actor> sender, std::string_view message, std::string_view channel) {
    if (!sender) {
        return std::string{message};
    }

    if (channel.empty()) {
        return fmt::format("{} says, '{}'", sender->display_name(), message);
    } else {
        return fmt::format("{} {}s, '{}'", sender->display_name(), channel, message);
    }
}

} // namespace Helpers

} // namespace BuiltinCommands