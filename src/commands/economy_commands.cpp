#include "economy_commands.hpp"

#include "../core/actor.hpp"
#include "../core/money.hpp"
#include "../core/object.hpp"
#include "../database/connection_pool.hpp"
#include "../database/world_queries.hpp"
#include "../world/world_manager.hpp"
#include "command_context.hpp"

#include <cctype>
#include <charconv>

namespace EconomyCommands {

// =============================================================================
// NPC Role Detection Helpers
// =============================================================================

namespace {

/**
 * Find a shopkeeper in the current room
 * @return The first shopkeeper found, or nullptr if none
 */
std::shared_ptr<Mobile> find_shopkeeper(const CommandContext &ctx) {
    if (!ctx.room)
        return nullptr;

    const auto &room_contents = ctx.room->contents();
    for (const auto &actor : room_contents.actors) {
        if (auto mobile = std::dynamic_pointer_cast<Mobile>(actor)) {
            if (mobile->is_shopkeeper()) {
                return mobile;
            }
        }
    }
    return nullptr;
}

/**
 * Find a banker in the current room
 * @return The first banker found, or nullptr if none
 */
std::shared_ptr<Mobile> find_banker(const CommandContext &ctx) {
    if (!ctx.room)
        return nullptr;

    const auto &room_contents = ctx.room->contents();
    for (const auto &actor : room_contents.actors) {
        if (auto mobile = std::dynamic_pointer_cast<Mobile>(actor)) {
            if (mobile->is_banker()) {
                return mobile;
            }
        }
    }
    return nullptr;
}

} // anonymous namespace

// =============================================================================
// Shop Enhancement Commands
// =============================================================================

Result<CommandResult> cmd_value(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send("Value what item?");
        ctx.send("Usage: value <item>");
        return CommandResult::InvalidSyntax;
    }

    // Check if there's a shopkeeper in the room
    auto shopkeeper = find_shopkeeper(ctx);
    if (!shopkeeper) {
        ctx.send_error("You need to find a shopkeeper to appraise items.");
        return CommandResult::InvalidState;
    }

    auto target = ctx.resolve_target(ctx.arg(0));
    if (!target.object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    auto obj = target.object;

    // Get the shopkeeper's buy price for the item
    int base_value = obj->value();
    int sell_price = base_value / 2; // Shops typically pay half

    ctx.send(fmt::format("{} tells you, 'I'll give you {} for {}.'", shopkeeper->display_name(),
                         fiery::Money::from_copper(sell_price).to_string(), obj->display_name()));

    return CommandResult::Success;
}

Result<CommandResult> cmd_identify(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send("Identify what item?");
        ctx.send("Usage: identify <item>");
        return CommandResult::InvalidSyntax;
    }

    // Check if there's a shopkeeper who offers identify service
    auto shopkeeper = find_shopkeeper(ctx);
    if (!shopkeeper) {
        ctx.send_error("You need to find a shopkeeper to identify items.");
        return CommandResult::InvalidState;
    }

    auto target = ctx.resolve_target(ctx.arg(0));
    if (!target.object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    auto obj = target.object;

    // Charge gold for identification (100 copper = 1 gold)
    constexpr int IDENTIFY_COST = 100;
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (player && !player->is_god()) {
        auto cost = fiery::Money::from_copper(IDENTIFY_COST);
        if (!player->spend(cost)) {
            ctx.send_error(fmt::format("{} tells you, 'That will cost {}. You don't have enough.'",
                                       shopkeeper->display_name(), cost.to_string()));
            return CommandResult::ResourceError;
        }
        ctx.send(fmt::format("{} takes {} for the identification.", shopkeeper->display_name(), cost.to_string()));
    }

    // Mark the item as identified
    obj->set_flag(ObjectFlag::Identified);

    ctx.send(fmt::format("--- {} ---", obj->display_name_with_condition()));
    ctx.send(fmt::format("Type: {}", magic_enum::enum_name(obj->type())));
    ctx.send(fmt::format("Weight: {} lbs", obj->weight()));
    auto value_money = fiery::Money::from_copper(obj->value());
    ctx.send(fmt::format("Value: {}", value_money.to_string()));

    // TODO: Show magical properties, stats, etc.
    ctx.send("Magical Properties: None detected");
    ctx.send("--- End of Identification ---");

    return CommandResult::Success;
}

Result<CommandResult> cmd_repair(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send("Repair what item?");
        ctx.send("Usage: repair <item>");
        return CommandResult::InvalidSyntax;
    }

    // Check if there's a shopkeeper who offers repair service
    auto shopkeeper = find_shopkeeper(ctx);
    if (!shopkeeper) {
        ctx.send_error("You need to find a shopkeeper to repair items.");
        return CommandResult::InvalidState;
    }

    auto target = ctx.resolve_target(ctx.arg(0));
    if (!target.object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    auto obj = target.object;
    ctx.send(fmt::format("{} examines {}...", shopkeeper->display_name(), obj->display_name()));

    // Handle light sources (lanterns, torches) - refill fuel
    if (obj->is_light_source()) {
        auto light = obj->light_info();

        // Permanent lights don't need refilling
        if (light.permanent || light.duration < 0) {
            ctx.send(fmt::format("{} tells you, 'This {} never runs out of fuel.'", shopkeeper->display_name(),
                                 obj->display_name()));
            return CommandResult::Success;
        }

        // Check if it needs refilling (less than full duration, assume 24 hours max)
        constexpr int MAX_LIGHT_DURATION = 24;
        if (light.duration >= MAX_LIGHT_DURATION) {
            ctx.send(fmt::format("{} tells you, 'This {} is already fully fueled.'", shopkeeper->display_name(),
                                 obj->display_name()));
            return CommandResult::Success;
        }

        // Calculate refill cost based on how much fuel is needed
        int hours_needed = MAX_LIGHT_DURATION - light.duration;
        int refill_cost = hours_needed * 10; // 10 copper per hour of fuel

        auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
        if (player && !player->is_god()) {
            auto cost = fiery::Money::from_copper(refill_cost);
            if (!player->spend(cost)) {
                ctx.send_error(fmt::format("{} tells you, 'That will cost {} to refill. You don't have enough.'",
                                           shopkeeper->display_name(), cost.to_string()));
                return CommandResult::ResourceError;
            }
            ctx.send(fmt::format("{} takes {} for the fuel.", shopkeeper->display_name(), cost.to_string()));
        }

        // Refill the light source
        light.duration = MAX_LIGHT_DURATION;
        obj->set_light_info(light);

        ctx.send(fmt::format("{} refills {} with fresh oil.", shopkeeper->display_name(), obj->display_name()));
        ctx.send(fmt::format("Your {} now has {} hours of fuel.", obj->display_name(), MAX_LIGHT_DURATION));

        return CommandResult::Success;
    }

    // No repairs needed for other items (no durability system yet)
    ctx.send(fmt::format("{} tells you, 'This item is in perfect condition. No repairs needed.'",
                         shopkeeper->display_name()));

    return CommandResult::Success;
}

// =============================================================================
// Banking Commands
// =============================================================================

Result<CommandResult> cmd_deposit(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use banks.");
        return CommandResult::InvalidState;
    }

    // Check if there's a banker in the room
    auto banker = find_banker(ctx);
    if (!banker) {
        ctx.send_error("You need to visit a banker to make deposits.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Deposit how much?");
        ctx.send("Usage: deposit <amount> [currency] or deposit all");
        ctx.send("Examples: deposit 100 gold, deposit 5p3g, deposit all");
        return CommandResult::InvalidSyntax;
    }

    // Handle "deposit all"
    if (ctx.arg(0) == "all") {
        if (player->wallet().is_zero()) {
            ctx.send("You have no money to deposit.");
            return CommandResult::InvalidState;
        }
        fiery::Money to_deposit = player->wallet();
        player->wallet() = fiery::Money();
        player->bank() += to_deposit;
        ctx.send(fmt::format("You deposit {}.", to_deposit.to_string()));
        return CommandResult::Success;
    }

    // Parse the money amount
    auto money = fiery::parse_money(ctx.args_from(0));
    if (!money) {
        ctx.send_error("Invalid amount. Use: deposit 100 gold, deposit 5p3g, etc.");
        return CommandResult::InvalidSyntax;
    }

    if (money->is_zero()) {
        ctx.send_error("You must deposit a positive amount.");
        return CommandResult::InvalidSyntax;
    }

    if (!player->deposit(*money)) {
        ctx.send_error(fmt::format("You don't have {}.", money->to_string()));
        return CommandResult::InvalidState;
    }

    ctx.send(fmt::format("You deposit {}.", money->to_string()));
    return CommandResult::Success;
}

Result<CommandResult> cmd_withdraw(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use banks.");
        return CommandResult::InvalidState;
    }

    // Check if there's a banker in the room
    auto banker = find_banker(ctx);
    if (!banker) {
        ctx.send_error("You need to visit a banker to make withdrawals.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Withdraw how much?");
        ctx.send("Usage: withdraw <amount> [currency] or withdraw all");
        ctx.send("Examples: withdraw 50 gold, withdraw 2p5g, withdraw all");
        return CommandResult::InvalidSyntax;
    }

    // Handle "withdraw all"
    if (ctx.arg(0) == "all") {
        if (player->bank().is_zero()) {
            ctx.send("You have no money in your bank account.");
            return CommandResult::InvalidState;
        }
        fiery::Money to_withdraw = player->bank();
        player->bank() = fiery::Money();
        player->wallet() += to_withdraw;
        ctx.send(fmt::format("You withdraw {}.", to_withdraw.to_string()));
        return CommandResult::Success;
    }

    // Parse the money amount
    auto money = fiery::parse_money(ctx.args_from(0));
    if (!money) {
        ctx.send_error("Invalid amount. Use: withdraw 50 gold, withdraw 2p5g, etc.");
        return CommandResult::InvalidSyntax;
    }

    if (money->is_zero()) {
        ctx.send_error("You must withdraw a positive amount.");
        return CommandResult::InvalidSyntax;
    }

    if (!player->withdraw(*money)) {
        ctx.send_error(fmt::format("You don't have {} in your bank account.", money->to_string()));
        return CommandResult::InvalidState;
    }

    ctx.send(fmt::format("You withdraw {}.", money->to_string()));
    return CommandResult::Success;
}

Result<CommandResult> cmd_balance(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can check bank balance.");
        return CommandResult::InvalidState;
    }

    // Check if there's a banker in the room
    auto banker = find_banker(ctx);
    if (!banker) {
        ctx.send_error("You need to visit a banker to check your balance.");
        return CommandResult::InvalidState;
    }

    const auto &bank = player->bank();
    if (bank.is_zero()) {
        ctx.send("Your bank account is empty.");
    } else {
        ctx.send(fmt::format("Bank balance: {}", bank.to_string()));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_transfer(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can transfer funds.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() < 2) {
        ctx.send("Transfer to whom and how much?");
        ctx.send("Usage: transfer <player> <amount> [currency]");
        ctx.send("Example: transfer Bob 100 gold");
        return CommandResult::InvalidSyntax;
    }

    std::string_view target_name = ctx.arg(0);

    // Parse the money amount from remaining arguments
    std::string money_str;
    for (size_t i = 1; i < ctx.arg_count(); ++i) {
        if (!money_str.empty())
            money_str += " ";
        money_str += ctx.arg(i);
    }

    auto money = fiery::parse_money(money_str);
    if (!money || money->is_zero()) {
        ctx.send_error("Invalid amount. Use: transfer Bob 100 gold");
        return CommandResult::InvalidSyntax;
    }

    // Find target player in the room
    auto target_actor = ctx.find_actor_target(target_name);
    if (!target_actor) {
        ctx.send_error(fmt::format("You don't see '{}' here.", target_name));
        return CommandResult::InvalidTarget;
    }

    auto target_player = std::dynamic_pointer_cast<Player>(target_actor);
    if (!target_player) {
        ctx.send_error("You can only transfer money to other players.");
        return CommandResult::InvalidTarget;
    }

    // Can't transfer to yourself
    if (target_player.get() == player.get()) {
        ctx.send_error("You can't transfer money to yourself.");
        return CommandResult::InvalidTarget;
    }

    // Check if sender has enough money
    if (!player->spend(*money)) {
        ctx.send_error(fmt::format("You don't have {}.", money->to_string()));
        return CommandResult::ResourceError;
    }

    // Transfer to target
    target_player->receive(*money);

    ctx.send(fmt::format("You transfer {} to {}.", money->to_string(), target_player->display_name()));
    ctx.send_to_actor(target_player,
                      fmt::format("{} transfers {} to you.", player->display_name(), money->to_string()));

    return CommandResult::Success;
}

// =============================================================================
// Currency Commands
// =============================================================================

Result<CommandResult> cmd_coins(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can check their coins.");
        return CommandResult::InvalidState;
    }

    const auto &wallet = player->wallet();
    if (wallet.is_zero()) {
        ctx.send("You are broke.");
    } else {
        ctx.send(fmt::format("You have {}.", wallet.to_string()));
    }

    return CommandResult::Success;
}

// =============================================================================
// Account Storage Commands
// =============================================================================

Result<CommandResult> cmd_account(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use account storage.");
        return CommandResult::InvalidState;
    }

    // Check if player has a linked account
    if (player->account().empty()) {
        ctx.send_error("Account storage requires your character to be linked to an account.");
        ctx.send("Create an account at our website to access this feature!");
        ctx.send("Benefits: Store items accessible by all your characters.");
        return CommandResult::InvalidState;
    }

    // Check if there's a banker in the room
    auto banker = find_banker(ctx);
    if (!banker) {
        ctx.send_error("You need to visit a banker to access account storage.");
        return CommandResult::InvalidState;
    }

    // No arguments = show status and usage
    if (ctx.arg_count() == 0) {
        ctx.send("--- Account Status ---");
        const auto &account_balance = player->account_bank();
        if (account_balance.is_zero()) {
            ctx.send("Account bank: Empty");
        } else {
            ctx.send(fmt::format("Account bank: {}", account_balance.to_string()));
        }
        ctx.send("");

        // Get item count from database
        int item_count = 0;
        auto count_result = ConnectionPool::instance().execute(
            [&](pqxx::work &txn) { return WorldQueries::count_account_items(txn, std::string{player->user_id()}); });
        if (count_result) {
            item_count = *count_result;
        }

        if (item_count == 0) {
            ctx.send("Items in account storage: None");
        } else {
            ctx.send(fmt::format("Items in account storage: {} (use 'account list' to view)", item_count));
        }
        ctx.send("--- End of Status ---");
        ctx.send("");
        ctx.send("Usage:");
        ctx.send("  account                    - Show account status");
        ctx.send("  account balance            - Show account bank balance");
        ctx.send("  account deposit <amount>   - Deposit money to account bank");
        ctx.send("  account withdraw <amount>  - Withdraw money from account bank");
        ctx.send("  account store <item>       - Store an item in account");
        ctx.send("  account retrieve <item>    - Retrieve an item from account");
        ctx.send("  account list               - List stored items");
        return CommandResult::Success;
    }

    std::string_view subcommand = ctx.arg(0);

    // =========================================================================
    // Money Commands (deposit/withdraw to account bank)
    // =========================================================================

    // Handle "deposit" for money
    if (subcommand == "deposit" || subcommand == "dep") {
        if (ctx.arg_count() < 2) {
            ctx.send("Deposit how much to your account bank?");
            ctx.send("Usage: account deposit <amount> or account deposit all");
            ctx.send("Examples: account deposit 100 gold, account deposit 5p3g");
            return CommandResult::InvalidSyntax;
        }

        // Handle "deposit all"
        if (ctx.arg(1) == "all") {
            if (player->wallet().is_zero()) {
                ctx.send("You have no money to deposit.");
                return CommandResult::InvalidState;
            }
            fiery::Money to_deposit = player->wallet();
            player->wallet() = fiery::Money();
            player->account_bank() += to_deposit;
            ctx.send(fmt::format("{} says, 'I'll transfer {} to your account.'", banker->display_name(),
                                 to_deposit.to_string()));
            ctx.send(fmt::format("You deposit {} to your account bank.", to_deposit.to_string()));
            return CommandResult::Success;
        }

        // Parse the money amount
        auto money = fiery::parse_money(ctx.args_from(1));
        if (!money) {
            ctx.send_error("Invalid amount. Use: account deposit 100 gold, account deposit 5p3g");
            return CommandResult::InvalidSyntax;
        }

        if (money->is_zero()) {
            ctx.send_error("You must deposit a positive amount.");
            return CommandResult::InvalidSyntax;
        }

        if (!player->account_deposit(*money)) {
            ctx.send_error(fmt::format("You don't have {}.", money->to_string()));
            return CommandResult::InvalidState;
        }

        ctx.send(
            fmt::format("{} says, 'I'll transfer {} to your account.'", banker->display_name(), money->to_string()));
        ctx.send(fmt::format("You deposit {} to your account bank.", money->to_string()));
        return CommandResult::Success;
    }

    // Handle "withdraw" for money
    if (subcommand == "withdraw" || subcommand == "with") {
        if (ctx.arg_count() < 2) {
            ctx.send("Withdraw how much from your account bank?");
            ctx.send("Usage: account withdraw <amount> or account withdraw all");
            ctx.send("Examples: account withdraw 50 gold, account withdraw 2p5g");
            return CommandResult::InvalidSyntax;
        }

        // Handle "withdraw all"
        if (ctx.arg(1) == "all") {
            if (player->account_bank().is_zero()) {
                ctx.send("Your account bank is empty.");
                return CommandResult::InvalidState;
            }
            fiery::Money to_withdraw = player->account_bank();
            player->account_bank() = fiery::Money();
            player->wallet() += to_withdraw;
            ctx.send(fmt::format("{} says, 'Here's {} from your account.'", banker->display_name(),
                                 to_withdraw.to_string()));
            ctx.send(fmt::format("You withdraw {} from your account bank.", to_withdraw.to_string()));
            return CommandResult::Success;
        }

        // Parse the money amount
        auto money = fiery::parse_money(ctx.args_from(1));
        if (!money) {
            ctx.send_error("Invalid amount. Use: account withdraw 50 gold, account withdraw 2p5g");
            return CommandResult::InvalidSyntax;
        }

        if (money->is_zero()) {
            ctx.send_error("You must withdraw a positive amount.");
            return CommandResult::InvalidSyntax;
        }

        if (!player->account_withdraw(*money)) {
            ctx.send_error(fmt::format("You don't have {} in your account bank.", money->to_string()));
            return CommandResult::InvalidState;
        }

        ctx.send(fmt::format("{} says, 'Here's {} from your account.'", banker->display_name(), money->to_string()));
        ctx.send(fmt::format("You withdraw {} from your account bank.", money->to_string()));
        return CommandResult::Success;
    }

    // Handle "balance" for money
    if (subcommand == "balance" || subcommand == "bal") {
        const auto &account_balance = player->account_bank();
        if (account_balance.is_zero()) {
            ctx.send("Your account bank is empty.");
        } else {
            ctx.send(fmt::format("Account bank balance: {}", account_balance.to_string()));
        }
        return CommandResult::Success;
    }

    // =========================================================================
    // Item Commands (store/retrieve items)
    // =========================================================================

    // Handle "store" for items
    if (subcommand == "store" || subcommand == "put") {
        if (ctx.arg_count() < 2) {
            ctx.send("Store what item?");
            ctx.send("Usage: account store <item>");
            return CommandResult::InvalidSyntax;
        }

        auto target = ctx.resolve_target(ctx.arg(1));
        if (!target.object) {
            ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(1)));
            return CommandResult::InvalidTarget;
        }

        auto obj = target.object;

        // Store the item in the database
        auto store_result = ConnectionPool::instance().execute([&](pqxx::work &txn) {
            return WorldQueries::store_account_item(txn, std::string{player->user_id()}, obj->id(),
                                                    1, // quantity
                                                    std::optional<std::string>{std::string{player->database_id()}},
                                                    "{}" // custom_data
            );
        });

        if (!store_result) {
            ctx.send_error("Failed to store item in account. Please try again later.");
            return CommandResult::SystemError;
        }

        // Remove item from player's inventory
        player->inventory().remove_item(obj);

        ctx.send(fmt::format("{} says, 'I'll keep {} safe for you.'", banker->display_name(), obj->display_name()));
        ctx.send(fmt::format("You store {} in your account.", obj->display_name()));
        return CommandResult::Success;
    }

    // Handle "retrieve/get" for items
    if (subcommand == "retrieve" || subcommand == "get" || subcommand == "take") {
        if (ctx.arg_count() < 2) {
            ctx.send("Retrieve what item?");
            ctx.send("Usage: account retrieve <item> or account retrieve #<slot>");
            return CommandResult::InvalidSyntax;
        }

        std::string_view item_arg = ctx.arg(1);

        // Load all account items to search through
        auto items_result = ConnectionPool::instance().execute(
            [&](pqxx::work &txn) { return WorldQueries::load_account_items(txn, std::string{player->user_id()}); });

        if (!items_result) {
            ctx.send_error("Failed to access account storage. Please try again later.");
            return CommandResult::SystemError;
        }

        auto &items = *items_result;
        if (items.empty()) {
            ctx.send_error("Your account storage is empty.");
            return CommandResult::InvalidState;
        }

        // Find the matching item
        std::optional<WorldQueries::AccountItemData> found_item;

        // Check if user specified a slot number (e.g., "#1" or "1")
        if (item_arg.starts_with('#') || std::isdigit(static_cast<unsigned char>(item_arg[0]))) {
            std::string_view num_str = item_arg.starts_with('#') ? item_arg.substr(1) : item_arg;
            int slot_num = 0;
            auto result = std::from_chars(num_str.data(), num_str.data() + num_str.size(), slot_num);
            if (result.ec == std::errc() && slot_num >= 1 && static_cast<size_t>(slot_num) <= items.size()) {
                found_item = items[slot_num - 1]; // Convert to 0-based index
            }
        }

        // If not found by slot, search by keyword
        if (!found_item) {
            auto &world = WorldManager::instance();
            for (const auto &acct_item : items) {
                auto obj_proto = world.get_object_prototype(acct_item.object_id);
                if (obj_proto) {
                    // Check if the object's keywords match
                    if (obj_proto->matches_keyword(item_arg)) {
                        found_item = acct_item;
                        break;
                    }
                }
            }
        }

        if (!found_item) {
            ctx.send_error(fmt::format("No item matching '{}' found in your account storage.", item_arg));
            ctx.send("Use 'account list' to see available items.");
            return CommandResult::InvalidTarget;
        }

        // Create an instance of the object
        auto &world = WorldManager::instance();
        auto new_obj = world.create_object_instance(found_item->object_id);
        if (!new_obj) {
            ctx.send_error("The stored item no longer exists in the world.");
            return CommandResult::SystemError;
        }

        // Remove from database
        auto remove_result = ConnectionPool::instance().execute(
            [&](pqxx::work &txn) { return WorldQueries::remove_account_item(txn, found_item->id); });

        if (!remove_result) {
            ctx.send_error("Failed to retrieve item. Please try again later.");
            return CommandResult::SystemError;
        }

        // Give item to player
        player->inventory().add_item(new_obj);

        ctx.send(
            fmt::format("{} says, 'Here's {} from your account.'", banker->display_name(), new_obj->display_name()));
        ctx.send(fmt::format("You retrieve {} from your account.", new_obj->display_name()));
        return CommandResult::Success;
    }

    // Handle "list" for items
    if (subcommand == "list" || subcommand == "items" || subcommand == "inventory" || subcommand == "inv") {
        auto items_result = ConnectionPool::instance().execute(
            [&](pqxx::work &txn) { return WorldQueries::load_account_items(txn, std::string{player->user_id()}); });

        if (!items_result) {
            ctx.send_error("Failed to access account storage. Please try again later.");
            return CommandResult::SystemError;
        }

        auto &items = *items_result;

        ctx.send("--- Account Item Storage ---");
        if (items.empty()) {
            ctx.send("Your account storage is empty.");
        } else {
            auto &world = WorldManager::instance();
            int slot_num = 1;
            for (const auto &acct_item : items) {
                auto obj_proto = world.get_object_prototype(acct_item.object_id);
                if (obj_proto) {
                    ctx.send(
                        fmt::format("  #{}: {} (qty: {})", slot_num++, obj_proto->display_name(), acct_item.quantity));
                } else {
                    ctx.send(fmt::format("  #{}: [Unknown Item {}:{}] (qty: {})", slot_num++,
                                         acct_item.object_id.zone_id(), acct_item.object_id.local_id(),
                                         acct_item.quantity));
                }
            }
            ctx.send(fmt::format("Total: {} item(s)", items.size()));
        }
        ctx.send("--- End of Storage ---");
        return CommandResult::Success;
    }

    // Unknown subcommand
    ctx.send_error(fmt::format("Unknown account command: {}", subcommand));
    ctx.send("Usage:");
    ctx.send("  account                    - Show account status");
    ctx.send("  account balance            - Show account bank balance");
    ctx.send("  account deposit <amount>   - Deposit money to account bank");
    ctx.send("  account withdraw <amount>  - Withdraw money from account bank");
    ctx.send("  account store <item>       - Store an item in account");
    ctx.send("  account retrieve <item>    - Retrieve an item from account");
    ctx.send("  account list               - List stored items");
    return CommandResult::InvalidSyntax;
}

// =============================================================================
// Currency Exchange
// =============================================================================

Result<CommandResult> cmd_exchange(const CommandContext &ctx) {
    // Exchange can only be done at money changers
    // TODO: Check for money changer NPC in room

    if (ctx.arg_count() < 3) {
        ctx.send("Usage: exchange <amount> <from-type> <to-type>");
        ctx.send("Example: exchange 10 copper silver");
        ctx.send("Available types: copper, silver, gold, platinum");
        ctx.send("");
        ctx.send("Note: This command requires you to be at a money changer.");
        return CommandResult::InvalidSyntax;
    }

    // Parse amount
    std::string amount_str{ctx.arg(0)};
    int amount = 0;
    try {
        amount = std::stoi(amount_str);
    } catch (...) {
        ctx.send_error("Invalid amount specified.");
        return CommandResult::InvalidSyntax;
    }

    if (amount <= 0) {
        ctx.send_error("You must exchange at least 1 coin.");
        return CommandResult::InvalidSyntax;
    }

    std::string_view from_type = ctx.arg(1);
    std::string_view to_type = ctx.arg(2);

    // Validate coin types
    static const std::vector<std::string> valid_types = {"copper", "silver", "gold", "platinum"};
    auto is_valid_type = [&](std::string_view type) {
        for (const auto &t : valid_types) {
            if (t.starts_with(type))
                return true;
        }
        return false;
    };

    if (!is_valid_type(from_type) || !is_valid_type(to_type)) {
        ctx.send_error("Invalid coin type. Use: copper, silver, gold, or platinum");
        return CommandResult::InvalidSyntax;
    }

    // TODO: Check for money changer NPC and perform actual exchange
    ctx.send("You are not at a money changer.");
    ctx.send("Note: Money exchange system not yet fully implemented.");

    return CommandResult::InvalidState;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    // Shop enhancement commands
    Commands().command("value", cmd_value).alias("val").category("Economy").privilege(PrivilegeLevel::Player).build();

    Commands()
        .command("identify", cmd_identify)
        .alias("id")
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands().command("repair", cmd_repair).category("Economy").privilege(PrivilegeLevel::Player).build();

    // Banking commands
    Commands()
        .command("deposit", cmd_deposit)
        .alias("dep")
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("withdraw", cmd_withdraw)
        .alias("with")
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("balance", cmd_balance)
        .alias("bal")
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands().command("transfer", cmd_transfer).category("Economy").privilege(PrivilegeLevel::Player).build();

    // Currency commands
    Commands()
        .command("coins", cmd_coins)
        .alias("gold")
        .alias("money")
        .alias("wealth")
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Account storage commands
    Commands()
        .command("account", cmd_account)
        .alias("storage")
        .alias("vault")
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Currency exchange
    Commands()
        .command("exchange", cmd_exchange)
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .description("Exchange coins at a money changer")
        .usage("exchange <amount> <from-type> <to-type>")
        .build();

    return Success();
}

} // namespace EconomyCommands
