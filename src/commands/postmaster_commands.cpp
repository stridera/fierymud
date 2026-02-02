#include "postmaster_commands.hpp"

#include "core/mobile.hpp"
#include "core/money.hpp"
#include "core/object.hpp"
#include "core/player.hpp"
#include "database/connection_pool.hpp"
#include "database/world_queries.hpp"
#include "game/composer_system.hpp"
#include "world/world_manager.hpp"

#include <charconv>
#include <chrono>
#include <fmt/chrono.h>

namespace PostmasterCommands {

// =============================================================================
// Helper Functions
// =============================================================================

namespace {

/**
 * Sanitize player-provided message content by closing any unclosed markup tags.
 */
std::string sanitize_player_message(std::string_view message) {
    return fmt::format("{}</>", message);
}

/**
 * Find a postmaster in the current room
 * @return The first postmaster found, or nullptr if none
 */
std::shared_ptr<Mobile> find_postmaster(const CommandContext& ctx) {
    if (!ctx.room) return nullptr;

    const auto& room_contents = ctx.room->contents();
    for (const auto& actor : room_contents.actors) {
        if (auto mobile = std::dynamic_pointer_cast<Mobile>(actor)) {
            if (mobile->is_postmaster()) {
                return mobile;
            }
        }
    }
    return nullptr;
}

/**
 * Format a timestamp for display
 */
std::string format_timestamp(const std::chrono::system_clock::time_point& tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time);
    return fmt::format("{:%b %d, %Y}", tm);
}

/**
 * Get a sender name for display (handles legacy IDs and missing characters)
 */
std::string get_sender_name(pqxx::work& txn, const WorldQueries::PlayerMailData& mail) {
    // Try to get name from sender character ID
    if (mail.sender_character_id) {
        auto name_result = WorldQueries::get_character_name_by_id(txn, *mail.sender_character_id);
        if (name_result && *name_result) {
            return **name_result;
        }
    }

    // Fall back to legacy ID display
    if (mail.legacy_sender_id) {
        return fmt::format("Legacy Player #{}", *mail.legacy_sender_id);
    }

    return "<deleted>";
}

/**
 * Send mail after composer completes
 */
void send_composed_mail(std::shared_ptr<Player> player,
                        const std::string& recipient_id,
                        const std::string& recipient_name,
                        const std::string& message) {
    // Sanitize and send the mail
    std::string sanitized = sanitize_player_message(message);

    auto result = ConnectionPool::instance().execute([&](pqxx::work& txn) {
        return WorldQueries::send_player_mail(
            txn,
            std::string{player->database_id()},
            recipient_id,
            sanitized,
            0, 0, 0, 0,  // No wealth attachment
            std::nullopt  // No object attachment
        );
    });

    if (!result) {
        player->send_message("Failed to send mail. Please try again later.");
        return;
    }

    player->send_message(fmt::format("You send mail to {}.", recipient_name));
}

} // anonymous namespace

// =============================================================================
// Mail Command - Send mail to another player
// =============================================================================

Result<CommandResult> cmd_mail(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can send mail.");
        return CommandResult::InvalidState;
    }

    // Check for postmaster
    auto postmaster = find_postmaster(ctx);
    if (!postmaster) {
        ctx.send_error("You need to visit a postmaster to send mail.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Send mail to whom?");
        ctx.send("Usage: mail <player> [message]");
        ctx.send("       mail <player>         - Opens multi-line composer");
        ctx.send("       mail <player> <text>  - Send single-line message");
        return CommandResult::InvalidSyntax;
    }

    std::string recipient_input = std::string{ctx.arg(0)};

    // Validate recipient exists before starting composer
    auto validate_result = ConnectionPool::instance().execute([&](pqxx::work& txn)
        -> Result<std::pair<std::string, std::string>> {
        std::string recipient_id;
        std::string actual_name;

        // Check if recipient looks like an email address
        if (recipient_input.find('@') != std::string::npos) {
            // Look up user by email
            auto user_result = WorldQueries::load_user_by_email(txn, recipient_input);
            if (!user_result) {
                return std::unexpected(Error{ErrorCode::NotFound, "No account found with that email"});
            }

            // Get characters for this user
            auto chars_result = WorldQueries::get_user_characters(txn, user_result->id);
            if (!chars_result || chars_result->empty()) {
                return std::unexpected(Error{ErrorCode::NotFound, "That account has no characters"});
            }

            // Use the first character
            auto& first_char = (*chars_result)[0];
            recipient_id = first_char.id;
            actual_name = first_char.name;
        } else {
            // Find recipient by character name
            auto recipient_result = WorldQueries::find_character_id_by_name(txn, recipient_input);
            if (!recipient_result) {
                return std::unexpected(recipient_result.error());
            }

            if (!*recipient_result) {
                return std::unexpected(Error{ErrorCode::NotFound, "Player not found"});
            }

            recipient_id = **recipient_result;

            // Get recipient's actual name (for proper capitalization)
            auto name_result = WorldQueries::get_character_name_by_id(txn, recipient_id);
            if (!name_result || !*name_result) {
                return std::unexpected(Error{ErrorCode::NotFound, "Player not found"});
            }

            actual_name = **name_result;
        }

        return std::make_pair(recipient_id, actual_name);
    });

    if (!validate_result) {
        ctx.send_error(fmt::format("{}", validate_result.error().message));
        return CommandResult::InvalidTarget;
    }

    auto& [recipient_id, recipient_name] = *validate_result;

    // If no message provided, use the composer
    if (ctx.arg_count() < 2) {
        ComposerConfig config;
        config.header_message = fmt::format("Composing mail to {}:", recipient_name);
        config.save_message = "Mail composed.";
        config.cancel_message = "Mail cancelled.";

        // Capture recipient info for the callback
        std::string captured_id = recipient_id;
        std::string captured_name = recipient_name;

        auto composer = std::make_shared<ComposerSystem>(
            std::weak_ptr<Player>(player), config);

        composer->set_completion_callback([player, captured_id, captured_name](ComposerResult result) {
            if (result.success && !result.combined_text.empty()) {
                send_composed_mail(player, captured_id, captured_name, result.combined_text);
            } else if (result.success && result.combined_text.empty()) {
                player->send_message("No message entered. Mail not sent.");
            }
            // Cancelled mail doesn't need additional message - composer already sent "Mail cancelled."
        });

        player->start_composing(composer);

        ctx.send(fmt::format("{} says, 'Take your time composing your message to {}.'",
            postmaster->display_name(), recipient_name));

        return CommandResult::Success;
    }

    // Single-line message - send directly
    std::string message = sanitize_player_message(ctx.args_from(1));

    auto result = ConnectionPool::instance().execute([&](pqxx::work& txn) {
        return WorldQueries::send_player_mail(
            txn,
            std::string{player->database_id()},
            recipient_id,
            message,
            0, 0, 0, 0,  // No wealth attachment for now
            std::nullopt  // No object attachment
        );
    });

    if (!result) {
        ctx.send_error("Failed to send mail. Please try again later.");
        return CommandResult::SystemError;
    }

    ctx.send(fmt::format("{} says, 'I'll make sure {} gets your message.'",
        postmaster->display_name(), recipient_name));
    ctx.send(fmt::format("You send mail to {}.", recipient_name));

    return CommandResult::Success;
}

// =============================================================================
// Check Command - Check for waiting mail
// =============================================================================

Result<CommandResult> cmd_check(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can check mail.");
        return CommandResult::InvalidState;
    }

    // Check for postmaster
    auto postmaster = find_postmaster(ctx);
    if (!postmaster) {
        ctx.send_error("You need to visit a postmaster to check mail.");
        return CommandResult::InvalidState;
    }

    // Load all mail for this character
    auto result = ConnectionPool::instance().execute([&](pqxx::work& txn)
        -> Result<std::vector<WorldQueries::PlayerMailData>> {
        return WorldQueries::load_character_mail(txn, std::string{player->database_id()});
    });

    if (!result) {
        ctx.send_error("Failed to check mail. Please try again later.");
        return CommandResult::SystemError;
    }

    auto& mails = *result;

    if (mails.empty()) {
        ctx.send(fmt::format("{} says, 'Sorry, you don't have any mail.'",
            postmaster->display_name()));
        return CommandResult::Success;
    }

    // Count unread and mails with unretrieved attachments
    int unread_count = 0;
    int with_attachments = 0;
    for (const auto& mail : mails) {
        if (!mail.read_at.has_value()) {
            ++unread_count;
        }
        if (mail.has_any_unretrieved()) {
            ++with_attachments;
        }
    }

    ctx.send(fmt::format("{} says, 'You have {} message{}!'",
        postmaster->display_name(),
        mails.size(),
        mails.size() == 1 ? "" : "s"));
    ctx.send("");

    // Display mail list
    ctx.send("--- Your Mail ---");

    // Get sender names for all mails
    auto names_result = ConnectionPool::instance().execute([&](pqxx::work& txn)
        -> Result<std::vector<std::string>> {
        std::vector<std::string> names;
        names.reserve(mails.size());
        for (const auto& mail : mails) {
            names.push_back(get_sender_name(txn, mail));
        }
        return names;
    });

    std::vector<std::string> sender_names;
    if (names_result) {
        sender_names = std::move(*names_result);
    }

    int index = 1;
    for (size_t i = 0; i < mails.size(); ++i) {
        const auto& mail = mails[i];
        std::string sender = i < sender_names.size() ? sender_names[i] : "<unknown>";
        std::string date = format_timestamp(mail.sent_at);

        std::string status;
        if (!mail.read_at.has_value()) {
            status = " [NEW]";
        }
        if (mail.has_unretrieved_wealth()) {
            status += " [COINS]";
        }
        if (mail.has_unretrieved_object()) {
            status += " [ITEM]";
        }

        ctx.send(fmt::format("  #{}: From {} - {}{}", index, sender, date, status));
        ++index;
    }

    ctx.send("--- End of Mail ---");
    ctx.send("");
    ctx.send("Use 'receive <number>' to read a message and claim attachments.");
    if (with_attachments > 0) {
        ctx.send(fmt::format("You have {} message{} with unretrieved attachments.",
            with_attachments, with_attachments == 1 ? "" : "s"));
    }

    return CommandResult::Success;
}

// =============================================================================
// Receive Command - Read mail and retrieve attachments
// =============================================================================

Result<CommandResult> cmd_receive(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can receive mail.");
        return CommandResult::InvalidState;
    }

    // Check for postmaster
    auto postmaster = find_postmaster(ctx);
    if (!postmaster) {
        ctx.send_error("You need to visit a postmaster to receive mail.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Receive which message?");
        ctx.send("Usage: receive <number> [delete|wealth|item]");
        ctx.send("       receive all       - Retrieve all attachments from all mail");
        ctx.send("");
        ctx.send("Examples:");
        ctx.send("  receive 1       - Read message #1 and retrieve all attachments");
        ctx.send("  receive 1 delete - Read message #1 and delete it");
        ctx.send("  receive 1 wealth - Only retrieve coins from message #1");
        ctx.send("  receive 1 item   - Only retrieve item from message #1");
        return CommandResult::InvalidSyntax;
    }

    std::string_view arg = ctx.arg(0);

    // Handle "receive all" - retrieve all attachments from all mail
    if (arg == "all") {
        auto result = ConnectionPool::instance().execute([&](pqxx::work& txn)
            -> Result<std::pair<fiery::Money, int>> {
            auto mails_result = WorldQueries::load_character_mail(
                txn, std::string{player->database_id()});
            if (!mails_result) {
                return std::unexpected(mails_result.error());
            }

            fiery::Money total_wealth;
            int items_received = 0;

            for (const auto& mail : *mails_result) {
                // Retrieve wealth if not already retrieved
                if (mail.has_unretrieved_wealth()) {
                    fiery::Money mail_wealth(
                        mail.attached_platinum,
                        mail.attached_gold,
                        mail.attached_silver,
                        mail.attached_copper
                    );
                    total_wealth += mail_wealth;

                    auto mark_result = WorldQueries::mark_mail_wealth_retrieved(
                        txn, mail.id, std::string{player->database_id()});
                    if (!mark_result) {
                        return std::unexpected(mark_result.error());
                    }
                }

                // Retrieve object if not already retrieved
                if (mail.has_unretrieved_object() && mail.attached_object_id) {
                    auto mark_result = WorldQueries::mark_mail_object_retrieved(
                        txn, mail.id, std::string{player->database_id()}, false);
                    if (!mark_result) {
                        return std::unexpected(mark_result.error());
                    }
                    ++items_received;
                }

                // Mark as read if not already
                if (!mail.read_at.has_value()) {
                    WorldQueries::mark_mail_read(txn, mail.id);
                }
            }

            return std::make_pair(total_wealth, items_received);
        });

        if (!result) {
            ctx.send_error("Failed to retrieve attachments. Please try again later.");
            return CommandResult::SystemError;
        }

        auto& [total_wealth, items_received] = *result;

        if (total_wealth.is_zero() && items_received == 0) {
            ctx.send("You have no attachments to retrieve.");
            return CommandResult::Success;
        }

        // Give player the wealth
        if (!total_wealth.is_zero()) {
            player->wallet() += total_wealth;
            ctx.send(fmt::format("{} says, 'Here's {} from your mail.'",
                postmaster->display_name(), total_wealth.to_string()));
        }

        // Create object instances
        if (items_received > 0) {
            ctx.send(fmt::format("You receive {} item{} from your mail.",
                items_received, items_received == 1 ? "" : "s"));
            ctx.send("Note: Item instantiation not yet fully implemented.");
        }

        return CommandResult::Success;
    }

    // Parse mail number
    int mail_num = 0;
    auto parse_result = std::from_chars(arg.data(), arg.data() + arg.size(), mail_num);
    if (parse_result.ec != std::errc() || mail_num < 1) {
        ctx.send_error("Invalid mail number. Use 'check' to see your mail list.");
        return CommandResult::InvalidSyntax;
    }

    // Parse optional subcommand
    bool delete_after = false;
    bool wealth_only = false;
    bool item_only = false;
    if (ctx.arg_count() > 1) {
        std::string_view subcmd = ctx.arg(1);
        if (subcmd == "delete" || subcmd == "del") {
            delete_after = true;
        } else if (subcmd == "wealth" || subcmd == "coins" || subcmd == "money") {
            wealth_only = true;
        } else if (subcmd == "item" || subcmd == "object") {
            item_only = true;
        }
    }

    // Load the specific mail
    auto result = ConnectionPool::instance().execute([&](pqxx::work& txn)
        -> Result<std::tuple<WorldQueries::PlayerMailData, std::string, bool>> {
        // Load all mail to find by index
        auto mails_result = WorldQueries::load_character_mail(
            txn, std::string{player->database_id()});
        if (!mails_result) {
            return std::unexpected(mails_result.error());
        }

        auto& mails = *mails_result;
        if (static_cast<size_t>(mail_num) > mails.size()) {
            return std::unexpected(Error{ErrorCode::NotFound, "Mail not found"});
        }

        auto& mail = mails[mail_num - 1];
        std::string sender = get_sender_name(txn, mail);

        // Mark as read
        bool was_unread = !mail.read_at.has_value();
        if (was_unread) {
            WorldQueries::mark_mail_read(txn, mail.id);
        }

        return std::make_tuple(mail, sender, was_unread);
    });

    if (!result) {
        if (result.error().code == ErrorCode::NotFound) {
            ctx.send_error(fmt::format("You don't have a message #{}. Use 'check' to see your mail.", mail_num));
        } else {
            ctx.send_error("Failed to retrieve mail. Please try again later.");
        }
        return CommandResult::InvalidTarget;
    }

    auto& [mail, sender_name, was_unread] = *result;

    // Display the mail
    ctx.send(fmt::format("--- Mail from {} ---", sender_name));
    ctx.send(fmt::format("Date: {}", format_timestamp(mail.sent_at)));
    ctx.send("");
    ctx.send(mail.body);
    ctx.send("");

    // Handle wealth retrieval
    if (mail.has_unretrieved_wealth() && !item_only) {
        fiery::Money wealth(
            mail.attached_platinum,
            mail.attached_gold,
            mail.attached_silver,
            mail.attached_copper
        );

        // Mark wealth as retrieved
        auto wealth_result = ConnectionPool::instance().execute([&](pqxx::work& txn) {
            return WorldQueries::mark_mail_wealth_retrieved(
                txn, mail.id, std::string{player->database_id()});
        });

        if (wealth_result) {
            player->wallet() += wealth;
            ctx.send(fmt::format("{} hands you {}.", postmaster->display_name(), wealth.to_string()));
        } else {
            ctx.send_error("Failed to retrieve coins. Try again with 'receive <num> wealth'.");
        }
    } else if (mail.attached_copper > 0 || mail.attached_silver > 0 ||
               mail.attached_gold > 0 || mail.attached_platinum > 0) {
        if (mail.wealth_retrieved_at.has_value()) {
            ctx.send("(Coins already retrieved)");
        }
    }

    // Handle object retrieval
    if (mail.has_unretrieved_object() && mail.attached_object_id && !wealth_only) {
        auto& world = WorldManager::instance();
        auto obj = world.create_object_instance(*mail.attached_object_id);

        if (obj) {
            // Mark object as retrieved
            auto obj_result = ConnectionPool::instance().execute([&](pqxx::work& txn) {
                return WorldQueries::mark_mail_object_retrieved(
                    txn, mail.id, std::string{player->database_id()}, false);
            });

            if (obj_result) {
                player->inventory().add_item(obj);
                ctx.send(fmt::format("{} hands you {}.",
                    postmaster->display_name(), obj->display_name()));
            } else {
                ctx.send_error("Failed to retrieve item. Try again with 'receive <num> item'.");
            }
        } else {
            ctx.send_error("The attached item no longer exists in the world.");
            // Still mark as retrieved so it doesn't keep showing
            ConnectionPool::instance().execute([&](pqxx::work& txn) {
                return WorldQueries::mark_mail_object_retrieved(
                    txn, mail.id, std::string{player->database_id()}, false);
            });
        }
    } else if (mail.attached_object_id) {
        if (mail.object_retrieved_at.has_value()) {
            ctx.send("(Item already retrieved)");
        }
    }

    ctx.send("--- End of Mail ---");

    // Handle deletion if requested
    if (delete_after) {
        auto delete_result = ConnectionPool::instance().execute([&](pqxx::work& txn) {
            return WorldQueries::delete_mail(txn, mail.id);
        });

        if (delete_result) {
            ctx.send("Message deleted.");
        } else {
            ctx.send_error("Failed to delete message.");
        }
    }

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    // Mail command
    Commands()
        .command("mail", cmd_mail)
        .alias("post")
        .alias("send")
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Check mail command
    Commands()
        .command("check", cmd_check)
        .alias("mailbox")
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Receive mail command
    Commands()
        .command("receive", cmd_receive)
        .alias("getmail")
        .alias("readmail")
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    return Success();
}

} // namespace PostmasterCommands
