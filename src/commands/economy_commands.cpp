#include "economy_commands.hpp"

#include "../core/actor.hpp"
#include "../core/money.hpp"
#include "../core/object.hpp"
#include "command_context.hpp"

namespace EconomyCommands {

// =============================================================================
// NPC Role Detection Helpers
// =============================================================================

namespace {

/**
 * Find a shopkeeper in the current room
 * @return The first shopkeeper found, or nullptr if none
 */
std::shared_ptr<Mobile> find_shopkeeper(const CommandContext& ctx) {
    if (!ctx.room) return nullptr;

    const auto& room_contents = ctx.room->contents();
    for (const auto& actor : room_contents.actors) {
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
std::shared_ptr<Mobile> find_banker(const CommandContext& ctx) {
    if (!ctx.room) return nullptr;

    const auto& room_contents = ctx.room->contents();
    for (const auto& actor : room_contents.actors) {
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

    // TODO: Check if there's a shopkeeper in the room
    // TODO: Get the shopkeeper's buy price for the item

    auto target = ctx.resolve_target(ctx.arg(0));
    if (!target.object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    auto obj = target.object;

    // Simple value calculation (would be modified by shopkeeper)
    int base_value = obj->value();
    int sell_price = base_value / 2; // Shops typically pay half

    ctx.send(fmt::format("The shopkeeper will give you {} gold for {}.",
             sell_price, obj->display_name()));
    ctx.send("Note: Shop pricing not yet fully implemented.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_identify(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send("Identify what item?");
        ctx.send("Usage: identify <item>");
        return CommandResult::InvalidSyntax;
    }

    // TODO: Check if there's a shopkeeper who offers identify service
    // TODO: Charge gold for identification

    auto target = ctx.resolve_target(ctx.arg(0));
    if (!target.object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    auto obj = target.object;

    ctx.send(fmt::format("--- {} ---", obj->display_name()));
    ctx.send(fmt::format("Type: {}", magic_enum::enum_name(obj->type())));
    ctx.send(fmt::format("Weight: {} lbs", obj->weight()));
    ctx.send(fmt::format("Value: {} gold", obj->value()));

    // TODO: Show magical properties, stats, etc.
    ctx.send("Magical Properties: None detected");
    ctx.send("--- End of Identification ---");
    ctx.send("Note: Full identification not yet implemented.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_repair(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send("Repair what item?");
        ctx.send("Usage: repair <item>");
        return CommandResult::InvalidSyntax;
    }

    // TODO: Check if there's a shopkeeper who offers repair service
    // TODO: Implement item condition/durability system

    auto target = ctx.resolve_target(ctx.arg(0));
    if (!target.object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    auto obj = target.object;

    ctx.send(fmt::format("{} is in perfect condition.", obj->display_name()));
    ctx.send("Note: Item durability system not yet implemented.");

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

    // TODO: Check if there's a bank/banker in the room

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

    // TODO: Check if there's a bank/banker in the room

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

    // TODO: Check if there's a bank/banker in the room

    const auto& bank = player->bank();
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
        if (!money_str.empty()) money_str += " ";
        money_str += ctx.arg(i);
    }

    auto money = fiery::parse_money(money_str);
    if (!money || money->is_zero()) {
        ctx.send_error("Invalid amount. Use: transfer Bob 100 gold");
        return CommandResult::InvalidSyntax;
    }

    // TODO: Find target player and transfer
    // For now, just show what would happen
    ctx.send(fmt::format("You would transfer {} to {}'s bank account.", money->to_string(), target_name));
    ctx.send("Note: Inter-player transfers not yet connected to database.");

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

    const auto& wallet = player->wallet();
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

    // No arguments = list items
    if (ctx.arg_count() == 0) {
        ctx.send("--- Account Storage ---");
        ctx.send("Your account storage is currently empty.");
        ctx.send("Note: Account storage database integration not yet implemented.");
        ctx.send("--- End of Storage ---");
        ctx.send("");
        ctx.send("Usage:");
        ctx.send("  account             - List stored items");
        ctx.send("  account store <item> - Store an item");
        ctx.send("  account get <item>   - Retrieve an item");
        return CommandResult::Success;
    }

    std::string_view subcommand = ctx.arg(0);

    // Handle "store" subcommand
    if (subcommand == "store" || subcommand == "deposit" || subcommand == "put") {
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

        // TODO: Actually store the item in database
        ctx.send(fmt::format("{} says, 'I'll keep {} safe for you.'",
            banker->display_name(), target.object->display_name()));
        ctx.send(fmt::format("You store {} in your account.", target.object->display_name()));
        ctx.send("Note: Account storage database integration not yet implemented.");
        return CommandResult::Success;
    }

    // Handle "retrieve/get/withdraw" subcommand
    if (subcommand == "get" || subcommand == "retrieve" || subcommand == "withdraw" || subcommand == "take") {
        if (ctx.arg_count() < 2) {
            ctx.send("Retrieve what item?");
            ctx.send("Usage: account get <item>");
            return CommandResult::InvalidSyntax;
        }

        // TODO: Query database for matching item
        ctx.send_error(fmt::format("No item matching '{}' found in your account storage.", ctx.arg(1)));
        ctx.send("Note: Account storage database integration not yet implemented.");
        return CommandResult::InvalidTarget;
    }

    // Handle "list" explicitly
    if (subcommand == "list" || subcommand == "items") {
        ctx.send("--- Account Storage ---");
        ctx.send("Your account storage is currently empty.");
        ctx.send("Note: Account storage database integration not yet implemented.");
        ctx.send("--- End of Storage ---");
        return CommandResult::Success;
    }

    // Unknown subcommand
    ctx.send_error(fmt::format("Unknown account command: {}", subcommand));
    ctx.send("Usage:");
    ctx.send("  account             - List stored items");
    ctx.send("  account store <item> - Store an item");
    ctx.send("  account get <item>   - Retrieve an item");
    return CommandResult::InvalidSyntax;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    // Shop enhancement commands
    Commands()
        .command("value", cmd_value)
        .alias("val")
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("identify", cmd_identify)
        .alias("id")
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("repair", cmd_repair)
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

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

    Commands()
        .command("transfer", cmd_transfer)
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

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

    return Success();
}

} // namespace EconomyCommands
