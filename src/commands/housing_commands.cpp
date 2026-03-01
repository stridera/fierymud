#include "housing_commands.hpp"

#include <algorithm>
#include <cmath>

#include <fmt/format.h>

#include "commands/command_system.hpp"
#include "core/actor.hpp"
#include "core/money.hpp"
#include "core/player.hpp"
#include "database/config_loader.hpp"
#include "database/player_repository.hpp"
#include "housing/housing_manager.hpp"
#include "housing/housing_room.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"

namespace HousingCommands {

namespace {

// ============================================================================
// Helpers
// ============================================================================

/** Get the player from context, or nullptr if not a player. */
std::shared_ptr<Player> get_player(const CommandContext &ctx) { return std::dynamic_pointer_cast<Player>(ctx.actor); }

/** Get the HousingRoom the player is in, or nullptr. */
std::shared_ptr<HousingRoom> get_current_housing_room(const CommandContext &ctx) {
    if (!ctx.room)
        return nullptr;
    return HousingManager::instance().get_housing_room(ctx.room->id());
}

/** Check if actor is the owner of the housing room or a god. */
bool is_owner_or_god(const CommandContext &ctx, const std::shared_ptr<HousingRoom> &room) {
    auto player = get_player(ctx);
    if (!player)
        return false;

    // Gods can act as owners (except for chest access)
    if (ctx.actor_privilege >= PrivilegeLevel::God)
        return true;

    return std::string(player->database_id()) == room->owner_character_id();
}

/** Check if actor is the owner (strict, no god override). */
bool is_strict_owner(const CommandContext &ctx, const std::shared_ptr<HousingRoom> &room) {
    auto player = get_player(ctx);
    if (!player)
        return false;
    return std::string(player->database_id()) == room->owner_character_id();
}

/** Find a player globally, including self (find_actor_global skips self). */
std::shared_ptr<Actor> find_player_including_self(const CommandContext &ctx, std::string_view name) {
    // Check self first (normalize to lowercase for keyword matching)
    if (ctx.actor) {
        std::string normalized(name);
        std::ranges::transform(normalized, normalized.begin(), ::tolower);
        std::vector<std::string> keywords{normalized};
        if (ctx.actor->matches_all_keywords(keywords)) {
            return ctx.actor;
        }
    }
    return ctx.find_actor_global(name);
}

// Forward declarations for subcommands
Result<CommandResult> cmd_house_leave(const CommandContext &ctx);
Result<CommandResult> cmd_house_describe(const CommandContext &ctx);
Result<CommandResult> cmd_house_name(const CommandContext &ctx);
Result<CommandResult> cmd_house_place(const CommandContext &ctx);
Result<CommandResult> cmd_house_remove(const CommandContext &ctx);
Result<CommandResult> cmd_house_expand(const CommandContext &ctx);
Result<CommandResult> cmd_house_guest(const CommandContext &ctx);
Result<CommandResult> cmd_house_info(const CommandContext &ctx);

// Forward declaration for visit command
Result<CommandResult> cmd_visit(const CommandContext &ctx);

// Forward declarations for god commands
Result<CommandResult> cmd_hgoto(const CommandContext &ctx);
Result<CommandResult> cmd_hgrant(const CommandContext &ctx);
Result<CommandResult> cmd_hrevoke(const CommandContext &ctx);
Result<CommandResult> cmd_hinfo(const CommandContext &ctx);

// ============================================================================
// Player Commands
// ============================================================================

Result<CommandResult> cmd_home(const CommandContext &ctx) {
    auto player = get_player(ctx);
    if (!player) {
        ctx.send_error("Only players can use this command.");
        return CommandResult::InvalidState;
    }

    auto &housing = HousingManager::instance();

    // Check if already in own house
    if (ctx.room && HousingManager::is_housing_room(ctx.room->id())) {
        auto hroom = housing.get_housing_room(ctx.room->id());
        if (hroom && std::string(player->database_id()) == hroom->owner_character_id()) {
            ctx.send("You are already home.");
            return CommandResult::Success;
        }
    }

    std::string char_id(player->database_id());
    if (!housing.has_house(char_id)) {
        ctx.send_error("You don't have a house yet. Complete the housing quest to earn one!");
        return CommandResult::InvalidState;
    }

    ctx.send_to_room(fmt::format("{} vanishes in a warm shimmer of light.", player->name()));

    auto result = housing.enter_house(player, char_id);
    if (!result) {
        ctx.send_error(fmt::format("Failed to enter your house: {}", result.error().message));
        return CommandResult::SystemError;
    }

    ctx.send("<warm>You feel a warm sensation as you are transported home.</>");
    // Show the room (use player's current room, not ctx.room which is stale after teleport)
    auto new_room = player->current_room();
    if (new_room) {
        ctx.send_line(new_room->get_room_description(ctx.actor.get()));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_house(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.show_help();
        return CommandResult::Success;
    }

    auto subcmd = ctx.arg(0);

    // Dispatch subcommands
    if (subcmd == "leave")
        return cmd_house_leave(ctx);
    if (subcmd == "describe")
        return cmd_house_describe(ctx);
    if (subcmd == "name")
        return cmd_house_name(ctx);
    if (subcmd == "place")
        return cmd_house_place(ctx);
    if (subcmd == "remove")
        return cmd_house_remove(ctx);
    if (subcmd == "expand")
        return cmd_house_expand(ctx);
    if (subcmd == "guest")
        return cmd_house_guest(ctx);
    if (subcmd == "info")
        return cmd_house_info(ctx);

    ctx.send_error(fmt::format("Unknown house subcommand: {}", subcmd));
    ctx.show_help();
    return CommandResult::InvalidSyntax;
}

// --- house leave ---
Result<CommandResult> cmd_house_leave(const CommandContext &ctx) {
    auto player = get_player(ctx);
    if (!player) {
        ctx.send_error("Only players can use this command.");
        return CommandResult::InvalidState;
    }

    if (!ctx.room || !HousingManager::is_housing_room(ctx.room->id())) {
        ctx.send_error("You aren't in a house.");
        return CommandResult::InvalidState;
    }

    ctx.send("<warm>You step outside and find yourself back in the world.</>");

    auto result = HousingManager::instance().exit_house(player);
    if (!result) {
        ctx.send_error(fmt::format("Failed to leave: {}", result.error().message));
        return CommandResult::SystemError;
    }

    // Show the new room (use player's current room, not ctx.room which is stale after teleport)
    auto new_room = player->current_room();
    if (new_room) {
        ctx.send_line(new_room->get_room_description(ctx.actor.get()));
    }

    return CommandResult::Success;
}

// --- house describe ---
Result<CommandResult> cmd_house_describe(const CommandContext &ctx) {
    auto room = get_current_housing_room(ctx);
    if (!room) {
        ctx.send_error("You must be in your house to set a description.");
        return CommandResult::InvalidState;
    }
    if (!is_owner_or_god(ctx, room)) {
        ctx.send_error("You can only describe rooms in your own house.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() < 2) {
        ctx.send("Usage: house describe <description text>");
        return CommandResult::InvalidSyntax;
    }

    auto desc = ctx.args_from(1);
    if (desc.size() > 2000) {
        ctx.send_error("Description too long (max 2000 characters).");
        return CommandResult::InvalidSyntax;
    }

    auto result = HousingManager::instance().set_room_description(room->db_id(), desc);
    if (!result) {
        ctx.send_error(fmt::format("Failed to set description: {}", result.error().message));
        return CommandResult::SystemError;
    }

    ctx.send_success("Room description updated.");
    return CommandResult::Success;
}

// --- house name ---
Result<CommandResult> cmd_house_name(const CommandContext &ctx) {
    auto room = get_current_housing_room(ctx);
    if (!room) {
        ctx.send_error("You must be in your house to set a name.");
        return CommandResult::InvalidState;
    }
    if (!is_owner_or_god(ctx, room)) {
        ctx.send_error("You can only name rooms in your own house.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() < 2) {
        ctx.send("Usage: house name <room name>");
        return CommandResult::InvalidSyntax;
    }

    auto name = ctx.args_from(1);
    if (name.size() > 80) {
        ctx.send_error("Room name too long (max 80 characters).");
        return CommandResult::InvalidSyntax;
    }

    auto result = HousingManager::instance().set_room_name(room->db_id(), name);
    if (!result) {
        ctx.send_error(fmt::format("Failed to set name: {}", result.error().message));
        return CommandResult::SystemError;
    }

    ctx.send_success(fmt::format("Room name set to: {}", name));
    return CommandResult::Success;
}

// --- house place ---
Result<CommandResult> cmd_house_place(const CommandContext &ctx) {
    auto player = get_player(ctx);
    if (!player) {
        ctx.send_error("Only players can place items.");
        return CommandResult::InvalidState;
    }

    auto room = get_current_housing_room(ctx);
    if (!room) {
        ctx.send_error("You must be in your house to place items.");
        return CommandResult::InvalidState;
    }
    if (!is_owner_or_god(ctx, room)) {
        // Check guest permission
        auto &housing = HousingManager::instance();
        std::string char_id(player->database_id());
        if (!housing.is_guest(room->owner_character_id(), char_id)) {
            ctx.send_error("You don't have permission to place items here.");
            return CommandResult::InvalidState;
        }
    }

    if (ctx.arg_count() < 2) {
        ctx.send("Usage: house place <item>");
        return CommandResult::InvalidSyntax;
    }

    auto item_name = ctx.args_from(1);

    // Find the item in the player's inventory
    auto items = ctx.find_objects_matching(item_name);
    if (items.empty()) {
        ctx.send_error(fmt::format("You don't have '{}'.", item_name));
        return CommandResult::InvalidTarget;
    }

    auto item = items[0];

    auto result = HousingManager::instance().place_item(player, item);
    if (!result) {
        ctx.send_error(fmt::format("Failed to place item: {}", result.error().message));
        return CommandResult::SystemError;
    }

    // Remove from player's inventory
    ctx.actor->inventory().remove_item(item);

    ctx.send_success(fmt::format("You carefully place {} in the room.", item->short_desc()));
    ctx.send_to_room(fmt::format("{} places {} in the room.", player->name(), item->short_desc()));

    return CommandResult::Success;
}

// --- house remove ---
Result<CommandResult> cmd_house_remove(const CommandContext &ctx) {
    auto player = get_player(ctx);
    if (!player) {
        ctx.send_error("Only players can remove items.");
        return CommandResult::InvalidState;
    }

    auto room = get_current_housing_room(ctx);
    if (!room) {
        ctx.send_error("You must be in your house to remove items.");
        return CommandResult::InvalidState;
    }
    if (!is_owner_or_god(ctx, room)) {
        ctx.send_error("You don't have permission to remove items here.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() < 2) {
        ctx.send("Usage: house remove <item>");
        ctx.send("Placed items:");
        for (const auto &item : room->placed_items()) {
            if (item) {
                int db_id = room->get_placed_item_db_id(item.get());
                ctx.send(fmt::format("  [{}] {}", db_id, item->short_desc()));
            }
        }
        return CommandResult::InvalidSyntax;
    }

    auto item_name = ctx.args_from(1);

    // Find matching placed item by keyword
    std::shared_ptr<Object> found_item;
    int found_db_id = -1;
    for (const auto &item : room->placed_items()) {
        if (item && item->matches_target_string(item_name)) {
            found_item = item;
            found_db_id = room->get_placed_item_db_id(item.get());
            break;
        }
    }

    if (!found_item || found_db_id < 0) {
        ctx.send_error(fmt::format("No placed item matching '{}' found.", item_name));
        return CommandResult::InvalidTarget;
    }

    auto result = HousingManager::instance().remove_placed_item(player, found_db_id);
    if (!result) {
        ctx.send_error(fmt::format("Failed to remove item: {}", result.error().message));
        return CommandResult::SystemError;
    }

    // Give the item back to the player
    ctx.actor->inventory().add_item(*result);

    ctx.send_success(fmt::format("You pick up {}.", (*result)->short_desc()));
    ctx.send_to_room(fmt::format("{} picks up {}.", player->name(), (*result)->short_desc()));

    return CommandResult::Success;
}

// --- house expand ---

/** Default values for housing expansion costs (used if not in GameConfig). */
constexpr int DEFAULT_EXPAND_BASE_COST = 10000; // 10 platinum in copper
constexpr int DEFAULT_EXPAND_COST_MULTIPLIER = 10;

/** Calculate expansion cost in copper based on current room count. */
long calculate_expand_cost(int current_room_count) {
    using namespace fierymud::config;
    auto &config = ConfigLoader::instance();
    long base_cost = config.get_int_or("housing", "expand_base_cost", DEFAULT_EXPAND_BASE_COST);
    int multiplier = config.get_int_or("housing", "expand_cost_multiplier", DEFAULT_EXPAND_COST_MULTIPLIER);

    // cost = base_cost * multiplier^(room_count - 1)
    // room_count=1 (foyer only) → cost = base_cost (for 2nd room)
    // room_count=2 → cost = base_cost * multiplier (for 3rd room)
    int exponent = std::max(0, current_room_count - 1);
    return static_cast<long>(base_cost * std::pow(multiplier, exponent));
}

Result<CommandResult> cmd_house_expand(const CommandContext &ctx) {
    auto player = get_player(ctx);
    if (!player) {
        ctx.send_error("Only players can expand a house.");
        return CommandResult::InvalidState;
    }

    auto room = get_current_housing_room(ctx);
    if (!room) {
        ctx.send_error("You must be in your house to expand it.");
        return CommandResult::InvalidState;
    }
    if (!is_owner_or_god(ctx, room)) {
        ctx.send_error("You can only expand your own house.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() < 2) {
        ctx.send("Usage: house expand <direction>");
        ctx.send("Directions: north, south, east, west, up, down");
        return CommandResult::InvalidSyntax;
    }

    auto dir_str = ctx.arg(1);
    auto dir = RoomUtils::parse_direction(dir_str);
    if (!dir) {
        ctx.send_error(fmt::format("Invalid direction: {}", dir_str));
        return CommandResult::InvalidSyntax;
    }

    // Don't allow expanding in the "out" direction from foyer
    if (*dir == Direction::Out) {
        ctx.send_error("You can't expand in that direction.");
        return CommandResult::InvalidSyntax;
    }

    // Check if exit already exists
    if (room->has_exit(*dir)) {
        ctx.send_error(fmt::format("There's already an exit to the {}.", RoomUtils::get_direction_name(*dir)));
        return CommandResult::InvalidState;
    }

    std::string char_id(player->database_id());

    // Calculate and check expansion cost (gods skip the cost)
    bool is_god = ctx.actor_privilege >= PrivilegeLevel::God;
    int current_rooms = HousingManager::instance().room_count(char_id);
    long cost_copper = calculate_expand_cost(current_rooms);
    auto cost = fiery::Money::copper(cost_copper);

    if (!is_god) {
        if (!player->can_afford(cost)) {
            ctx.send_error(fmt::format("Expanding your house costs {}. You can't afford it.", cost.to_string(true)));
            return CommandResult::InvalidState;
        }
    }

    auto result = HousingManager::instance().expand_house(char_id, room->db_id(), *dir, "A New Room");
    if (!result) {
        ctx.send_error(fmt::format("Failed to expand: {}", result.error().message));
        return CommandResult::SystemError;
    }

    // Deduct cost after successful expansion
    if (!is_god) {
        player->spend(cost);
        ctx.send_success(fmt::format("A new room has been created to the {}! (Cost: {})",
                                     RoomUtils::get_direction_name(*dir), cost.to_string(true)));
    } else {
        ctx.send_success(fmt::format("A new room has been created to the {}!", RoomUtils::get_direction_name(*dir)));
    }
    ctx.send("Use 'house name' and 'house describe' in the new room to customize it.");

    return CommandResult::Success;
}

// --- house guest ---
Result<CommandResult> cmd_house_guest(const CommandContext &ctx) {
    auto player = get_player(ctx);
    if (!player) {
        ctx.send_error("Only players can manage guests.");
        return CommandResult::InvalidState;
    }

    std::string char_id(player->database_id());

    if (ctx.arg_count() < 2) {
        ctx.send("Usage: house guest <add|remove|list> [name]");
        return CommandResult::InvalidSyntax;
    }

    auto action = ctx.arg(1);

    if (action == "list") {
        auto guests = HousingManager::instance().get_guests(char_id);
        if (!guests) {
            ctx.send_error("Failed to load guest list.");
            return CommandResult::SystemError;
        }
        if (guests->empty()) {
            ctx.send("You have no guests on your guest list.");
            return CommandResult::Success;
        }
        ctx.send("<b>House Guests:</>");
        for (const auto &guest : *guests) {
            ctx.send(fmt::format("  {} {}", guest.character_name, guest.can_place ? "(can place items)" : ""));
        }
        return CommandResult::Success;
    }

    if (ctx.arg_count() < 3) {
        ctx.send(fmt::format("Usage: house guest {} <player name>", action));
        return CommandResult::InvalidSyntax;
    }

    auto target_name = ctx.arg(2);

    if (action == "add") {
        // Try to find online, then fall back to DB for offline players
        std::string guest_id;
        std::string guest_name;

        auto target = find_player_including_self(ctx, target_name);
        if (target) {
            auto target_player = std::dynamic_pointer_cast<Player>(target);
            if (!target_player) {
                ctx.send_error("You can only add players as guests.");
                return CommandResult::InvalidTarget;
            }
            guest_id = std::string(target_player->database_id());
            guest_name = std::string(target_player->name());
        } else {
            auto &repo = PlayerRepositoryProvider::instance().get();
            auto loaded = repo.load_player_by_name(target_name);
            if (!loaded) {
                ctx.send_error(fmt::format("Player '{}' not found.", target_name));
                return CommandResult::InvalidTarget;
            }
            guest_id = std::string((*loaded)->database_id());
            guest_name = std::string((*loaded)->name());
        }

        bool can_place = (ctx.arg_count() > 3 && ctx.arg(3) == "--place");

        auto result = HousingManager::instance().add_guest(char_id, guest_id, can_place);
        if (!result) {
            ctx.send_error(fmt::format("Failed to add guest: {}", result.error().message));
            return CommandResult::SystemError;
        }

        ctx.send_success(fmt::format("{} has been added to your guest list.", guest_name));
        return CommandResult::Success;
    }

    if (action == "remove") {
        // Try to find online, then fall back to DB for offline players
        std::string guest_id;
        std::string guest_name;

        auto target = find_player_including_self(ctx, target_name);
        if (target) {
            auto target_player = std::dynamic_pointer_cast<Player>(target);
            if (!target_player) {
                ctx.send_error("Invalid target.");
                return CommandResult::InvalidTarget;
            }
            guest_id = std::string(target_player->database_id());
            guest_name = std::string(target_player->name());
        } else {
            auto &repo = PlayerRepositoryProvider::instance().get();
            auto loaded = repo.load_player_by_name(target_name);
            if (!loaded) {
                ctx.send_error(fmt::format("Player '{}' not found.", target_name));
                return CommandResult::InvalidTarget;
            }
            guest_id = std::string((*loaded)->database_id());
            guest_name = std::string((*loaded)->name());
        }

        auto result = HousingManager::instance().remove_guest(char_id, guest_id);
        if (!result) {
            ctx.send_error(fmt::format("Failed to remove guest: {}", result.error().message));
            return CommandResult::SystemError;
        }

        ctx.send_success(fmt::format("{} has been removed from your guest list.", guest_name));
        return CommandResult::Success;
    }

    ctx.send_error(fmt::format("Unknown guest action: {}", action));
    return CommandResult::InvalidSyntax;
}

// --- house info ---
Result<CommandResult> cmd_house_info(const CommandContext &ctx) {
    auto player = get_player(ctx);
    if (!player) {
        ctx.send_error("Only players can view house info.");
        return CommandResult::InvalidState;
    }

    std::string char_id(player->database_id());

    auto info = HousingManager::instance().get_house_info(char_id);
    if (!info) {
        ctx.send_error("You don't have a house.");
        return CommandResult::InvalidState;
    }

    ctx.send("<b>House Information:</>");
    ctx.send(fmt::format("  Entrance: Zone {}, Room {}", info->entrance_room_zone_id, info->entrance_room_id));

    auto room_count = HousingManager::instance().room_count(char_id);
    ctx.send(fmt::format("  Rooms: {}/{}", room_count > 0 ? room_count : 1, HousingManager::MAX_ROOMS));

    auto guests = HousingManager::instance().get_guests(char_id);
    if (guests && !guests->empty()) {
        ctx.send(fmt::format("  Guests: {}", guests->size()));
    } else {
        ctx.send("  Guests: None");
    }

    return CommandResult::Success;
}

// ============================================================================
// Visit Command (guest access to another player's house)
// ============================================================================

Result<CommandResult> cmd_visit(const CommandContext &ctx) {
    auto player = get_player(ctx);
    if (!player) {
        ctx.send_error("Only players can use this command.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Usage: visit <player name>");
        return CommandResult::InvalidSyntax;
    }

    auto target_name = ctx.arg(0);

    // Try to find the target player online first, then fall back to DB
    std::string owner_char_id;
    std::string owner_name;

    auto target = find_player_including_self(ctx, target_name);
    if (target) {
        auto target_player = std::dynamic_pointer_cast<Player>(target);
        if (!target_player) {
            ctx.send_error("You can only visit a player's house.");
            return CommandResult::InvalidTarget;
        }
        owner_char_id = std::string(target_player->database_id());
        owner_name = std::string(target_player->name());
    } else {
        // Try loading from database (offline player)
        auto &repo = PlayerRepositoryProvider::instance().get();
        auto loaded = repo.load_player_by_name(target_name);
        if (!loaded) {
            ctx.send_error(fmt::format("Player '{}' not found.", target_name));
            return CommandResult::InvalidTarget;
        }
        owner_char_id = std::string((*loaded)->database_id());
        owner_name = std::string((*loaded)->name());
    }

    auto &housing = HousingManager::instance();

    // Check the target has a house
    if (!housing.has_house(owner_char_id)) {
        ctx.send_error(fmt::format("{} doesn't have a house.", owner_name));
        return CommandResult::InvalidState;
    }

    // Owner can always enter their own house (redirect to 'home')
    std::string player_char_id(player->database_id());
    if (player_char_id == owner_char_id) {
        ctx.send("That's your own house! Use 'home' instead.");
        return CommandResult::InvalidState;
    }

    // Gods bypass guest check
    bool is_god = ctx.actor_privilege >= PrivilegeLevel::God;
    if (!is_god) {
        auto guest_check = housing.check_guest_access(owner_char_id, player_char_id);
        if (!guest_check) {
            ctx.send_error("Failed to check guest access.");
            return CommandResult::SystemError;
        }
        if (!*guest_check) {
            ctx.send_error(fmt::format("You are not on {}'s guest list.", owner_name));
            return CommandResult::InvalidState;
        }
    }

    ctx.send_to_room(fmt::format("{} vanishes in a warm shimmer of light.", player->name()));

    auto result = housing.enter_house(player, owner_char_id);
    if (!result) {
        ctx.send_error(fmt::format("Failed to enter house: {}", result.error().message));
        return CommandResult::SystemError;
    }

    ctx.send(fmt::format("<warm>You step into {}'s home.</>", owner_name));

    // Show the room
    auto new_room = player->current_room();
    if (new_room) {
        ctx.send_line(new_room->get_room_description(ctx.actor.get()));
    }

    return CommandResult::Success;
}

// ============================================================================
// God/Admin Commands
// ============================================================================

Result<CommandResult> cmd_hgoto(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send("Usage: hgoto <player> [room#]");
        return CommandResult::InvalidSyntax;
    }

    auto target_name = ctx.arg(0);
    auto target = find_player_including_self(ctx, target_name);
    if (!target) {
        ctx.send_error(fmt::format("Player '{}' not found.", target_name));
        return CommandResult::InvalidTarget;
    }

    auto target_player = std::dynamic_pointer_cast<Player>(target);
    if (!target_player) {
        ctx.send_error("Target must be a player.");
        return CommandResult::InvalidTarget;
    }

    std::string char_id(target_player->database_id());
    auto &housing = HousingManager::instance();

    if (!housing.has_house(char_id)) {
        ctx.send_error(fmt::format("{} doesn't have a house.", target_player->name()));
        return CommandResult::InvalidState;
    }

    auto player = get_player(ctx);
    if (!player)
        return CommandResult::InvalidState;

    auto result = housing.enter_house(player, char_id);
    if (!result) {
        ctx.send_error(fmt::format("Failed to enter house: {}", result.error().message));
        return CommandResult::SystemError;
    }

    ctx.send(fmt::format("You teleport to {}'s house.", target_player->name()));
    return CommandResult::Success;
}

Result<CommandResult> cmd_hgrant(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send("Usage: hgrant <player>");
        return CommandResult::InvalidSyntax;
    }

    auto target_name = ctx.arg(0);
    auto target = find_player_including_self(ctx, target_name);
    if (!target) {
        ctx.send_error(fmt::format("Player '{}' not found.", target_name));
        return CommandResult::InvalidTarget;
    }

    auto target_player = std::dynamic_pointer_cast<Player>(target);
    if (!target_player) {
        ctx.send_error("Target must be a player.");
        return CommandResult::InvalidTarget;
    }

    std::string char_id(target_player->database_id());

    // Use the target player's current room as the entrance
    auto target_room = target_player->current_room();
    int entrance_zone = target_room ? static_cast<int>(target_room->id().zone_id()) : 30;
    int entrance_room = target_room ? static_cast<int>(target_room->id().local_id()) : 0;

    auto result = HousingManager::instance().create_house(char_id, entrance_zone, entrance_room);
    if (!result) {
        ctx.send_error(fmt::format("Failed to grant house: {}", result.error().message));
        return CommandResult::SystemError;
    }

    ctx.send_success(fmt::format("Granted a house to {}.", target_player->name()));
    ctx.send_to_actor(target, "<warm>You have been granted a house! Use 'home' to visit it.</>");

    return CommandResult::Success;
}

Result<CommandResult> cmd_hrevoke(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send("Usage: hrevoke <player>");
        return CommandResult::InvalidSyntax;
    }

    auto target_name = ctx.arg(0);
    auto target = find_player_including_self(ctx, target_name);
    if (!target) {
        ctx.send_error(fmt::format("Player '{}' not found.", target_name));
        return CommandResult::InvalidTarget;
    }

    auto target_player = std::dynamic_pointer_cast<Player>(target);
    if (!target_player) {
        ctx.send_error("Target must be a player.");
        return CommandResult::InvalidTarget;
    }

    std::string char_id(target_player->database_id());

    auto result = HousingManager::instance().revoke_house(char_id);
    if (!result) {
        ctx.send_error(fmt::format("Failed to revoke house: {}", result.error().message));
        return CommandResult::SystemError;
    }

    ctx.send_success(fmt::format("Revoked {}'s house.", target_player->name()));
    return CommandResult::Success;
}

Result<CommandResult> cmd_hinfo(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send("Usage: hinfo <player>");
        return CommandResult::InvalidSyntax;
    }

    auto target_name = ctx.arg(0);
    auto target = find_player_including_self(ctx, target_name);
    if (!target) {
        ctx.send_error(fmt::format("Player '{}' not found.", target_name));
        return CommandResult::InvalidTarget;
    }

    auto target_player = std::dynamic_pointer_cast<Player>(target);
    if (!target_player) {
        ctx.send_error("Target must be a player.");
        return CommandResult::InvalidTarget;
    }

    std::string char_id(target_player->database_id());

    auto info = HousingManager::instance().get_house_info(char_id);
    if (!info) {
        ctx.send_error(fmt::format("{} doesn't have a house.", target_player->name()));
        return CommandResult::InvalidState;
    }

    ctx.send(fmt::format("<b>House Info for {}:</>{}", target_player->name(), ""));
    ctx.send(fmt::format("  House ID: {}", info->id));
    ctx.send(fmt::format("  Entrance: {}:{}", info->entrance_room_zone_id, info->entrance_room_id));
    if (info->return_room_zone_id) {
        ctx.send(fmt::format("  Return Location: {}:{} (player may be stranded!)", *info->return_room_zone_id,
                             *info->return_room_id));
    }

    auto guests = HousingManager::instance().get_guests(char_id);
    if (guests && !guests->empty()) {
        ctx.send("  Guests:");
        for (const auto &guest : *guests) {
            ctx.send(fmt::format("    {} {}", guest.character_name, guest.can_place ? "[can place]" : ""));
        }
    }

    return CommandResult::Success;
}

} // anonymous namespace

// ============================================================================
// Registration
// ============================================================================

Result<void> register_commands() {
    // Player commands
    Commands().command("home", cmd_home).category("Housing").privilege(PrivilegeLevel::Player).build();

    Commands()
        .command("house", cmd_house)
        .category("Housing")
        .privilege(PrivilegeLevel::Player)
        .help(
            "Manage your player house.\n"
            "Subcommands:\n"
            "  house leave              - Exit your house\n"
            "  house describe <text>    - Set room description\n"
            "  house name <text>        - Set room name\n"
            "  house place <item>       - Place an item from inventory\n"
            "  house remove <item>      - Pick up a placed item\n"
            "  house expand <direction> - Add a new room\n"
            "  house guest add <name>   - Add a guest\n"
            "  house guest remove <name>- Remove a guest\n"
            "  house guest list         - List guests\n"
            "  house info               - Show house stats")
        .build();

    Commands()
        .command("visit", cmd_visit)
        .category("Housing")
        .privilege(PrivilegeLevel::Player)
        .description("Visit another player's house (must be on their guest list)")
        .build();

    // God commands
    Commands().command("hgoto", cmd_hgoto).category("Housing").privilege(PrivilegeLevel::God).build();

    Commands().command("hgrant", cmd_hgrant).category("Housing").privilege(PrivilegeLevel::God).build();

    Commands().command("hrevoke", cmd_hrevoke).category("Housing").privilege(PrivilegeLevel::God).build();

    Commands().command("hinfo", cmd_hinfo).category("Housing").privilege(PrivilegeLevel::God).build();

    return Success();
}

} // namespace HousingCommands
