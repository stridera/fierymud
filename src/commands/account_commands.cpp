#include "account_commands.hpp"

#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../database/connection_pool.hpp"
#include "../game/player_output.hpp"

#include <chrono>
#include <pqxx/pqxx>

namespace AccountCommands {

// =============================================================================
// Account Information Command
// =============================================================================

Result<CommandResult> cmd_account(const CommandContext &ctx) {
    auto *player = dynamic_cast<Player *>(ctx.actor.get());
    if (!player) {
        ctx.send_error("Only players can use account commands.");
        return CommandResult::InvalidState;
    }

    // Check if there's a subcommand
    if (ctx.arg_count() > 0) {
        auto subcmd = ctx.arg(0);
        if (subcmd == "link") {
            return cmd_account_link(ctx);
        } else if (subcmd == "unlink") {
            return cmd_account_unlink(ctx);
        } else if (subcmd == "delete") {
            return cmd_account_delete(ctx);
        } else if (subcmd == "characters" || subcmd == "chars") {
            return cmd_account_characters(ctx);
        } else {
            ctx.send_error(fmt::format("Unknown account subcommand: {}", subcmd));
            ctx.send_usage("account [link|unlink|delete|characters]");
            return CommandResult::InvalidSyntax;
        }
    }

    // No subcommand - show account information
    if (!player->has_user_account()) {
        ctx.send_header("Account Status");
        ctx.send_line("Your character is not linked to an account.");
        ctx.send_line("");
        ctx.send_line("Use 'account link <email>' to link to an existing account.");
        return CommandResult::Success;
    }

    // Query account information from database
    auto result = ConnectionPool::instance().execute_read_only([&](pqxx::nontransaction &txn) -> Result<void> {
        auto row =
            txn.exec1(fmt::format("SELECT id, email, username, role, created_at, last_login_at, account_wealth "
                                  "FROM \"Users\" WHERE id = {} AND deleted_at IS NULL",
                                  txn.quote(std::string(player->user_id()))));

        ctx.send_header("Account Information");
        ctx.send_line(fmt::format("Username: {}", row["username"].as<std::string>()));
        ctx.send_line(fmt::format("Email: {}", row["email"].as<std::string>()));
        ctx.send_line(fmt::format("Role: {}", row["role"].as<std::string>()));

        // Account wealth (in copper, display as platinum/gold/silver/copper)
        auto wealth_copper = row["account_wealth"].as<int64_t>(0);
        int64_t platinum = wealth_copper / 1000;
        int64_t gold = (wealth_copper % 1000) / 100;
        int64_t silver = (wealth_copper % 100) / 10;
        int64_t copper = wealth_copper % 10;
        ctx.send_line(fmt::format("Account Bank: {}p {}g {}s {}c", platinum, gold, silver, copper));

        // Get character count
        auto char_count =
            txn.exec1(fmt::format("SELECT COUNT(*) FROM \"Characters\" WHERE user_id = {} AND deleted_at IS NULL",
                                  txn.quote(std::string(player->user_id()))))[0]
                .as<int>();
        ctx.send_line(fmt::format("Characters: {}", char_count));

        ctx.send_separator();
        ctx.send_line("Commands: account link | account unlink | account delete | account characters");

        return {};
    });

    if (!result) {
        ctx.send_error("Failed to retrieve account information.");
        Log::error("Account query failed for user {}: {}", player->user_id(), result.error().message);
        return CommandResult::ResourceError;
    }

    return CommandResult::Success;
}

// =============================================================================
// Account Link Command
// =============================================================================

Result<CommandResult> cmd_account_link(const CommandContext &ctx) {
    auto *player = dynamic_cast<Player *>(ctx.actor.get());
    if (!player) {
        ctx.send_error("Only players can use account commands.");
        return CommandResult::InvalidState;
    }

    if (player->has_user_account()) {
        ctx.send_error("Your character is already linked to an account.");
        ctx.send_info("Use 'account unlink' first if you want to link to a different account.");
        return CommandResult::InvalidState;
    }

    // Need at least the email argument (after 'link' subcommand)
    if (ctx.arg_count() < 2) {
        ctx.send_usage("account link <email>");
        ctx.send_info("Links your character to an existing account by email address.");
        return CommandResult::InvalidSyntax;
    }

    auto email = std::string(ctx.arg(1));

    // Look up the account by email
    auto result = ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<std::string> {
        auto rows = txn.exec(fmt::format("SELECT id, username FROM \"Users\" WHERE email = {} AND deleted_at IS NULL",
                                         txn.quote(email)));

        if (rows.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound, "No account found with that email."});
        }

        auto user_id = rows[0]["id"].as<std::string>();
        auto username = rows[0]["username"].as<std::string>();

        // Link the character to the account
        txn.exec(fmt::format("UPDATE \"Characters\" SET user_id = {} WHERE name = {}", txn.quote(user_id),
                             txn.quote(std::string(player->name()))));

        return username;
    });

    if (!result) {
        ctx.send_error(result.error().message);
        return CommandResult::ResourceError;
    }

    // Update the player object
    // Note: We need to query for the user_id since result only has username
    auto user_result =
        ConnectionPool::instance().execute_read_only([&](pqxx::nontransaction &txn) -> Result<std::string> {
            auto row = txn.exec1(
                fmt::format("SELECT id FROM \"Users\" WHERE email = {} AND deleted_at IS NULL", txn.quote(email)));
            return row["id"].as<std::string>();
        });

    if (user_result) {
        player->set_user_id(*user_result);
    }

    ctx.send_success(fmt::format("Character successfully linked to account '{}'.", *result));
    Log::info("Player {} linked to account {}", player->name(), email);

    return CommandResult::Success;
}

// =============================================================================
// Account Unlink Command
// =============================================================================

Result<CommandResult> cmd_account_unlink(const CommandContext &ctx) {
    auto *player = dynamic_cast<Player *>(ctx.actor.get());
    if (!player) {
        ctx.send_error("Only players can use account commands.");
        return CommandResult::InvalidState;
    }

    if (!player->has_user_account()) {
        ctx.send_error("Your character is not linked to any account.");
        return CommandResult::InvalidState;
    }

    // Unlink from database
    auto result = ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
        txn.exec(fmt::format("UPDATE \"Characters\" SET user_id = NULL WHERE name = {}",
                             txn.quote(std::string(player->name()))));
        return {};
    });

    if (!result) {
        ctx.send_error("Failed to unlink character from account.");
        Log::error("Account unlink failed for {}: {}", player->name(), result.error().message);
        return CommandResult::ResourceError;
    }

    player->set_user_id("");
    ctx.send_success("Character unlinked from account.");
    Log::info("Player {} unlinked from account", player->name());

    return CommandResult::Success;
}

// =============================================================================
// Account Delete Command (Soft Delete)
// =============================================================================

Result<CommandResult> cmd_account_delete(const CommandContext &ctx) {
    auto *player = dynamic_cast<Player *>(ctx.actor.get());
    if (!player) {
        ctx.send_error("Only players can use account commands.");
        return CommandResult::InvalidState;
    }

    if (!player->has_user_account()) {
        ctx.send_error("Your character is not linked to any account.");
        ctx.send_info("There is no account to delete.");
        return CommandResult::InvalidState;
    }

    // Check for confirmation
    bool confirmed = false;
    if (ctx.arg_count() >= 2) {
        auto confirm_arg = ctx.arg(1);
        confirmed = (confirm_arg == "confirm" || confirm_arg == "CONFIRM");
    }

    if (!confirmed) {
        ctx.send_header("Account Deletion Warning", '!');
        ctx.send_line("This will mark your account and ALL characters for deletion!");
        ctx.send_line("");
        ctx.send_line("Affected characters will be:");

        // List characters on account
        auto list_result = ConnectionPool::instance().execute_read_only([&](pqxx::nontransaction &txn) -> Result<void> {
            auto rows = txn.exec(fmt::format(
                "SELECT name, level FROM \"Characters\" WHERE user_id = {} AND deleted_at IS NULL ORDER BY level DESC",
                txn.quote(std::string(player->user_id()))));
            for (const auto &row : rows) {
                ctx.send_line(fmt::format("  - {} (level {})", row["name"].as<std::string>(), row["level"].as<int>()));
            }
            return {};
        });

        ctx.send_line("");
        ctx.send_line("To confirm deletion, type: account delete confirm");
        ctx.send_separator('!');
        return CommandResult::Success;
    }

    // Perform soft delete
    auto user_id = std::string(player->user_id());
    auto result = ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<int> {
        // Soft delete all characters on the account
        auto char_result = txn.exec(fmt::format(
            "UPDATE \"Characters\" SET deleted_at = NOW(), deletion_reason = 'Account deletion requested by user' "
            "WHERE user_id = {} AND deleted_at IS NULL",
            txn.quote(user_id)));
        int chars_deleted = char_result.affected_rows();

        // Soft delete the account
        txn.exec(
            fmt::format("UPDATE \"Users\" SET deleted_at = NOW(), deletion_reason = 'Deletion requested by user' "
                        "WHERE id = {} AND deleted_at IS NULL",
                        txn.quote(user_id)));

        return chars_deleted;
    });

    if (!result) {
        ctx.send_error("Failed to delete account.");
        Log::error("Account deletion failed for {}: {}", user_id, result.error().message);
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("Account marked for deletion ({} characters affected).", *result));
    ctx.send_info("You will be disconnected shortly.");
    Log::warn("Account {} deleted by user request ({} characters)", user_id, *result);

    // Clear the user_id so the player can't access account functions
    player->set_user_id("");

    // Disconnect the player after the messages are sent
    if (auto output = player->get_output()) {
        output->disconnect("Account deleted");
    }

    return CommandResult::Success;
}

// =============================================================================
// Account Characters Command
// =============================================================================

Result<CommandResult> cmd_account_characters(const CommandContext &ctx) {
    auto *player = dynamic_cast<Player *>(ctx.actor.get());
    if (!player) {
        ctx.send_error("Only players can use account commands.");
        return CommandResult::InvalidState;
    }

    if (!player->has_user_account()) {
        ctx.send_error("Your character is not linked to any account.");
        return CommandResult::InvalidState;
    }

    auto result = ConnectionPool::instance().execute_read_only([&](pqxx::nontransaction &txn) -> Result<void> {
        auto rows =
            txn.exec(fmt::format("SELECT name, level, race, player_class, last_login, is_online "
                                 "FROM \"Characters\" WHERE user_id = {} AND deleted_at IS NULL "
                                 "ORDER BY last_login DESC NULLS LAST",
                                 txn.quote(std::string(player->user_id()))));

        ctx.send_header("Characters on Account");

        if (rows.empty()) {
            ctx.send_line("No characters found.");
        } else {
            for (const auto &row : rows) {
                auto name = row["name"].as<std::string>();
                auto level = row["level"].as<int>();
                auto race = row["race"].as<std::string>("Unknown");
                auto pclass = row["player_class"].as<std::string>("Unknown");
                auto online = row["is_online"].as<bool>(false);

                std::string status = online ? " [ONLINE]" : "";
                ctx.send_line(fmt::format("  {} - Level {} {} {}{}", name, level, race, pclass, status));
            }
        }

        ctx.send_separator();
        ctx.send_line(fmt::format("Total: {} character(s)", rows.size()));

        return {};
    });

    if (!result) {
        ctx.send_error("Failed to retrieve character list.");
        Log::error("Character list query failed: {}", result.error().message);
        return CommandResult::ResourceError;
    }

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands()
        .command("account", cmd_account)
        .category("Account")
        .privilege(PrivilegeLevel::Player)
        .description("Manage your account and linked characters")
        .usage("account [link|unlink|delete|characters]")
        .build();

    return {};
}

} // namespace AccountCommands
