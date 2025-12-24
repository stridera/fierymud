#include "economy_commands.hpp"

#include "../core/actor.hpp"
#include "../core/object.hpp"
#include "command_context.hpp"

namespace EconomyCommands {

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

// Currency type enumeration for banking operations
enum class CurrencyType {
    Copper,
    Silver,
    Gold,
    Platinum,
    Invalid
};

CurrencyType parse_currency(std::string_view input) {
    std::string lower;
    for (char c : input) {
        lower += static_cast<char>(std::tolower(c));
    }
    if (lower == "copper" || lower == "cop" || lower == "c") return CurrencyType::Copper;
    if (lower == "silver" || lower == "sil" || lower == "s") return CurrencyType::Silver;
    if (lower == "gold" || lower == "gol" || lower == "g") return CurrencyType::Gold;
    if (lower == "platinum" || lower == "plat" || lower == "p") return CurrencyType::Platinum;
    return CurrencyType::Invalid;
}

std::string_view currency_name(CurrencyType type) {
    switch (type) {
        case CurrencyType::Copper: return "copper";
        case CurrencyType::Silver: return "silver";
        case CurrencyType::Gold: return "gold";
        case CurrencyType::Platinum: return "platinum";
        default: return "coins";
    }
}

Result<CommandResult> cmd_deposit(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use banks.");
        return CommandResult::InvalidState;
    }

    // TODO: Check if there's a bank/banker in the room

    if (ctx.arg_count() == 0) {
        ctx.send("Deposit how much?");
        ctx.send("Usage: deposit <amount> [currency]");
        ctx.send("Example: deposit 100 gold");
        return CommandResult::InvalidSyntax;
    }

    int amount = 0;
    try {
        amount = std::stoi(std::string(ctx.arg(0)));
    } catch (...) {
        ctx.send_error("Invalid amount.");
        return CommandResult::InvalidSyntax;
    }

    if (amount <= 0) {
        ctx.send_error("You must deposit a positive amount.");
        return CommandResult::InvalidSyntax;
    }

    CurrencyType currency = CurrencyType::Gold;  // Default to gold
    if (ctx.arg_count() > 1) {
        currency = parse_currency(ctx.arg(1));
        if (currency == CurrencyType::Invalid) {
            ctx.send_error("Invalid currency type. Use copper, silver, gold, or platinum.");
            return CommandResult::InvalidSyntax;
        }
    }

    // Check if player has enough currency and perform transfer
    bool success = false;
    switch (currency) {
        case CurrencyType::Copper:
            if (player->copper() >= amount) {
                player->set_copper(player->copper() - amount);
                player->set_bank_copper(player->bank_copper() + amount);
                success = true;
            }
            break;
        case CurrencyType::Silver:
            if (player->silver() >= amount) {
                player->set_silver(player->silver() - amount);
                player->set_bank_silver(player->bank_silver() + amount);
                success = true;
            }
            break;
        case CurrencyType::Gold:
            if (player->gold() >= amount) {
                player->set_gold(player->gold() - amount);
                player->set_bank_gold(player->bank_gold() + amount);
                success = true;
            }
            break;
        case CurrencyType::Platinum:
            if (player->platinum() >= amount) {
                player->set_platinum(player->platinum() - amount);
                player->set_bank_platinum(player->bank_platinum() + amount);
                success = true;
            }
            break;
        default:
            break;
    }

    if (success) {
        ctx.send(fmt::format("You deposit {} {}.", amount, currency_name(currency)));
    } else {
        ctx.send_error(fmt::format("You don't have {} {}.", amount, currency_name(currency)));
        return CommandResult::InvalidState;
    }

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
        ctx.send("Usage: withdraw <amount> [currency]");
        ctx.send("Example: withdraw 50 gold");
        return CommandResult::InvalidSyntax;
    }

    int amount = 0;
    try {
        amount = std::stoi(std::string(ctx.arg(0)));
    } catch (...) {
        ctx.send_error("Invalid amount.");
        return CommandResult::InvalidSyntax;
    }

    if (amount <= 0) {
        ctx.send_error("You must withdraw a positive amount.");
        return CommandResult::InvalidSyntax;
    }

    CurrencyType currency = CurrencyType::Gold;  // Default to gold
    if (ctx.arg_count() > 1) {
        currency = parse_currency(ctx.arg(1));
        if (currency == CurrencyType::Invalid) {
            ctx.send_error("Invalid currency type. Use copper, silver, gold, or platinum.");
            return CommandResult::InvalidSyntax;
        }
    }

    // Check if bank has enough currency and perform transfer
    bool success = false;
    switch (currency) {
        case CurrencyType::Copper:
            if (player->bank_copper() >= amount) {
                player->set_bank_copper(player->bank_copper() - amount);
                player->set_copper(player->copper() + amount);
                success = true;
            }
            break;
        case CurrencyType::Silver:
            if (player->bank_silver() >= amount) {
                player->set_bank_silver(player->bank_silver() - amount);
                player->set_silver(player->silver() + amount);
                success = true;
            }
            break;
        case CurrencyType::Gold:
            if (player->bank_gold() >= amount) {
                player->set_bank_gold(player->bank_gold() - amount);
                player->set_gold(player->gold() + amount);
                success = true;
            }
            break;
        case CurrencyType::Platinum:
            if (player->bank_platinum() >= amount) {
                player->set_bank_platinum(player->bank_platinum() - amount);
                player->set_platinum(player->platinum() + amount);
                success = true;
            }
            break;
        default:
            break;
    }

    if (success) {
        ctx.send(fmt::format("You withdraw {} {}.", amount, currency_name(currency)));
    } else {
        ctx.send_error(fmt::format("You don't have {} {} in your bank account.", amount, currency_name(currency)));
        return CommandResult::InvalidState;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_balance(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can check bank balance.");
        return CommandResult::InvalidState;
    }

    // TODO: Check if there's a bank/banker in the room

    ctx.send("--- Bank Balance ---");
    ctx.send(fmt::format("  Platinum: {}", player->bank_platinum()));
    ctx.send(fmt::format("  Gold:     {}", player->bank_gold()));
    ctx.send(fmt::format("  Silver:   {}", player->bank_silver()));
    ctx.send(fmt::format("  Copper:   {}", player->bank_copper()));
    ctx.send("--- End of Balance ---");

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
    int amount = 0;
    try {
        amount = std::stoi(std::string(ctx.arg(1)));
    } catch (...) {
        ctx.send_error("Invalid amount.");
        return CommandResult::InvalidSyntax;
    }

    if (amount <= 0) {
        ctx.send_error("You must transfer a positive amount.");
        return CommandResult::InvalidSyntax;
    }

    std::string currency = "gold";
    if (ctx.arg_count() > 2) {
        currency = std::string(ctx.arg(2));
    }

    // TODO: Verify target player exists
    // TODO: Transfer between bank accounts

    ctx.send(fmt::format("You transfer {} {} to {}'s account.", amount, currency, target_name));
    ctx.send("Note: Banking system not yet connected to database.");

    return CommandResult::Success;
}

// =============================================================================
// Currency Commands
// =============================================================================

Result<CommandResult> cmd_exchange(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can exchange currency.");
        return CommandResult::InvalidState;
    }

    // TODO: Check if there's a money changer in the room

    if (ctx.arg_count() < 3) {
        ctx.send("Exchange rates:");
        ctx.send("  1 platinum = 10 gold");
        ctx.send("  1 gold = 10 silver");
        ctx.send("  1 silver = 10 copper");
        ctx.send("");
        ctx.send("Usage: exchange <amount> <from_currency> <to_currency>");
        ctx.send("Example: exchange 100 gold platinum");
        return CommandResult::Success;
    }

    int amount = 0;
    try {
        amount = std::stoi(std::string(ctx.arg(0)));
    } catch (...) {
        ctx.send_error("Invalid amount.");
        return CommandResult::InvalidSyntax;
    }

    std::string from_currency = std::string(ctx.arg(1));
    std::string to_currency = std::string(ctx.arg(2));

    // TODO: Calculate exchange and perform transaction

    ctx.send(fmt::format("You exchange {} {} for {}.", amount, from_currency, to_currency));
    ctx.send("Note: Currency exchange not yet implemented.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_coins(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can check their coins.");
        return CommandResult::InvalidState;
    }

    ctx.send("--- Your Coins ---");
    ctx.send(fmt::format("  Platinum: {}", player->platinum()));
    ctx.send(fmt::format("  Gold:     {}", player->gold()));
    ctx.send(fmt::format("  Silver:   {}", player->silver()));
    ctx.send(fmt::format("  Copper:   {}", player->copper()));
    ctx.send("--- End of Coins ---");

    return CommandResult::Success;
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
        .command("exchange", cmd_exchange)
        .alias("exch")
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("coins", cmd_coins)
        .alias("gold")
        .alias("money")
        .alias("wealth")
        .category("Economy")
        .privilege(PrivilegeLevel::Player)
        .build();

    return Success();
}

} // namespace EconomyCommands
