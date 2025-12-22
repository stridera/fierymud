#include "clan_commands.hpp"

#include "../core/clan.hpp"
#include "../core/logging.hpp"
#include "../game/player_output.hpp"
#include "../text/string_utils.hpp"
#include "command_context.hpp"

#include <algorithm>
#include <sstream>

namespace ClanCommands {

// =============================================================================
// Clan Command Implementations
// =============================================================================

/**
 * Clan information and basic commands
 * Usage: clan [info|who|motd]
 */
Result<CommandResult> cmd_clan(const CommandContext &ctx) {
    const auto& args = ctx.args();
    
    // No arguments - show clan info
    if (args.empty()) {
        auto clan = get_clan(ctx.actor()->legacy_character());
        if (!clan) {
            PlayerOutput::send_to_actor(ctx.actor(), "You are not a member of any clan.");
            return Ok(CommandResult::Success);
        }
        
        return cmd_clan_info(ctx, **clan);
    }
    
    std::string subcmd = to_lowercase(args.first());
    
    if (subcmd == "info" || subcmd == "information") {
        return cmd_clan_info(ctx);
    } else if (subcmd == "who" || subcmd == "list") {
        return cmd_clan_who(ctx);
    } else if (subcmd == "motd" || subcmd == "message") {
        return cmd_clan_motd(ctx);
    } else if (subcmd == "chat" || subcmd == "say") {
        return cmd_clan_chat(ctx);
    } else if (subcmd == "ranks") {
        return cmd_clan_ranks(ctx);
    } else {
        PlayerOutput::send_to_actor(ctx.actor(), 
            "Usage: clan [info|who|motd|chat <message>|ranks]");
        return Ok(CommandResult::Success);
    }
}

/**
 * Show clan information
 */
Result<CommandResult> cmd_clan_info(const CommandContext &ctx) {
    auto clan = get_clan(ctx.actor()->legacy_character());
    if (!clan) {
        PlayerOutput::send_to_actor(ctx.actor(), "You are not a member of any clan.");
        return Ok(CommandResult::Success);
    }
    
    return cmd_clan_info(ctx, **clan);
}

Result<CommandResult> cmd_clan_info(const CommandContext &ctx, const Clan &clan) {
    std::ostringstream output;
    
    output << fmt::format("<b:cyan>--- {} ({}) ---</>\n", clan.name(), clan.abbreviation());

    if (!clan.description().empty()) {
        output << fmt::format("<cyan>Description:</> {}\n", clan.description());
    }

    output << fmt::format("<cyan>Members:</> {}\n", clan.member_count());
    output << fmt::format("<cyan>Ranks:</> {}\n", clan.rank_count());

    if (!clan.motd().empty()) {
        output << fmt::format("<cyan>Message of the Day:</>\n{}\n", clan.motd());
    }
    
    PlayerOutput::send_to_actor(ctx.actor(), output.str());
    return Ok(CommandResult::Success);
}

/**
 * Show clan member list
 */
Result<CommandResult> cmd_clan_who(const CommandContext &ctx) {
    auto clan = get_clan(ctx.actor()->legacy_character());
    if (!clan) {
        PlayerOutput::send_to_actor(ctx.actor(), "You are not a member of any clan.");
        return Ok(CommandResult::Success);
    }
    
    std::ostringstream output;
    output << fmt::format("<b:cyan>--- {} Members ---</>\n", (*clan)->name());
    
    const auto& ranks = (*clan)->ranks();
    
    // Group members by rank and show online status
    for (size_t rank_idx = 0; rank_idx < ranks.size(); ++rank_idx) {
        const auto& rank = ranks[rank_idx];
        auto members_in_rank = (*clan)->get_members_by_rank_index(static_cast<int>(rank_idx));
        
        if (!members_in_rank.empty()) {
            output << fmt::format("<cyan>{}:</>\n", rank.title());

            for (const auto& member : members_in_rank) {
                // Check if member is online
                CharData* online_char = clan_security::find_player_safe(member.name);
                std::string status = online_char ? "<green>[Online]</>" : "<red>[Offline]</>";
                
                output << fmt::format("  {} {}\n", member.name, status);
            }
        }
    }
    
    PlayerOutput::send_to_actor(ctx.actor(), output.str());
    return Ok(CommandResult::Success);
}

/**
 * Show clan message of the day
 */
Result<CommandResult> cmd_clan_motd(const CommandContext &ctx) {
    auto clan = get_clan(ctx.actor()->legacy_character());
    if (!clan) {
        PlayerOutput::send_to_actor(ctx.actor(), "You are not a member of any clan.");
        return Ok(CommandResult::Success);
    }
    
    const std::string& motd = (*clan)->motd();
    if (motd.empty()) {
        PlayerOutput::send_to_actor(ctx.actor(), "Your clan has no message of the day set.");
    } else {
        std::ostringstream output;
        output << fmt::format("<b:cyan>--- {} Message of the Day ---</>\n{}\n",
                             (*clan)->name(), motd);
        PlayerOutput::send_to_actor(ctx.actor(), output.str());
    }
    
    return Ok(CommandResult::Success);
}

/**
 * Send message to clan channel
 * Usage: clan chat <message>
 */
Result<CommandResult> cmd_clan_chat(const CommandContext &ctx) {
    const auto& args = ctx.args();
    
    if (args.size() < 2) {
        PlayerOutput::send_to_actor(ctx.actor(), "Usage: clan chat <message>");
        return Ok(CommandResult::Success);
    }
    
    auto clan = get_clan(ctx.actor()->legacy_character());
    if (!clan) {
        PlayerOutput::send_to_actor(ctx.actor(), "You are not a member of any clan.");
        return Ok(CommandResult::Success);
    }
    
    // Check permission
    if (!has_clan_permission(ctx.actor()->legacy_character(), ClanPermission::CLAN_CHAT)) {
        PlayerOutput::send_to_actor(ctx.actor(), "You don't have permission to use the clan chat.");
        return Ok(CommandResult::Success);
    }
    
    // Build message from remaining arguments
    std::ostringstream message;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) message << " ";
        message << args[i];
    }
    
    // Format and send the clan message
    auto member = get_clan_member(ctx.actor()->legacy_character());
    std::string rank_title = member ? (*clan)->ranks()[member->rank_index].title() : "Member";
    
    std::string formatted_message = fmt::format("<cyan>[Clan] {} ({}): {}</>",
                                               ctx.actor()->name(), rank_title, message.str());
    
    // Send to all clan members (including self)
    (*clan)->notify(nullptr, formatted_message);
    
    return Ok(CommandResult::Success);
}

/**
 * Show clan rank structure
 */
Result<CommandResult> cmd_clan_ranks(const CommandContext &ctx) {
    auto clan = get_clan(ctx.actor()->legacy_character());
    if (!clan) {
        PlayerOutput::send_to_actor(ctx.actor(), "You are not a member of any clan.");
        return Ok(CommandResult::Success);
    }
    
    std::ostringstream output;
    output << fmt::format("<b:cyan>--- {} Rank Structure ---</>\n", (*clan)->name());
    
    const auto& ranks = (*clan)->ranks();
    for (size_t i = 0; i < ranks.size(); ++i) {
        const auto& rank = ranks[i];
        auto members_in_rank = (*clan)->get_members_by_rank_index(static_cast<int>(i));
        
        output << fmt::format("<cyan>{}:</> {} member{}\n",
                             rank.title(), members_in_rank.size(),
                             members_in_rank.size() == 1 ? "" : "s");
    }
    
    PlayerOutput::send_to_actor(ctx.actor(), output.str());
    return Ok(CommandResult::Success);
}

// =============================================================================
// Administrative Commands (God-level or high-ranking clan members)
// =============================================================================

/**
 * Clan management commands
 * Usage: cset <clan> <property> <value>
 */
Result<CommandResult> cmd_cset(const CommandContext &ctx) {
    const auto& args = ctx.args();
    
    if (args.size() < 3) {
        PlayerOutput::send_to_actor(ctx.actor(), 
            "Usage: cset <clan> <property> <value>\n"
            "Properties: name, abbr, desc, motd, dues, appfee, applevel");
        return Ok(CommandResult::Success);
    }
    
    // Check god privileges
    if (ctx.actor()->level() < LVL_IMMORT) {
        PlayerOutput::send_to_actor(ctx.actor(), "You don't have permission to use this command.");
        return Ok(CommandResult::Success);
    }
    
    std::string clan_name = args[0];
    std::string property = args[1];
    std::string value = args[2];
    
    auto clan = clan_repository.find_by_name(clan_name);
    if (!clan) {
        clan = clan_repository.find_by_abbreviation(clan_name);
    }
    
    if (!clan) {
        PlayerOutput::send_to_actor(ctx.actor(), 
            fmt::format("Clan '{}' not found.", clan_name));
        return Ok(CommandResult::Success);
    }
    
    property = to_lowercase(property);
    
    if (property == "motd") {
        // Build complete message from remaining args
        std::ostringstream motd;
        for (size_t i = 2; i < args.size(); ++i) {
            if (i > 2) motd << " ";
            motd << args[i];
        }
        (*clan)->admin_set_motd(motd.str());
        PlayerOutput::send_to_actor(ctx.actor(), 
            fmt::format("Set MOTD for clan '{}'.", (*clan)->name()));
    } else if (property == "dues") {
        try {
            unsigned int dues = std::stoul(value);
            (*clan)->admin_set_dues(dues);
            PlayerOutput::send_to_actor(ctx.actor(), 
                fmt::format("Set dues for clan '{}' to {} coins.", (*clan)->name(), dues));
        } catch (const std::exception&) {
            PlayerOutput::send_to_actor(ctx.actor(), "Invalid dues amount.");
        }
    } else if (property == "appfee") {
        try {
            unsigned int fee = std::stoul(value);
            (*clan)->admin_set_app_fee(fee);
            PlayerOutput::send_to_actor(ctx.actor(), 
                fmt::format("Set application fee for clan '{}' to {} coins.", (*clan)->name(), fee));
        } catch (const std::exception&) {
            PlayerOutput::send_to_actor(ctx.actor(), "Invalid application fee amount.");
        }
    } else if (property == "applevel") {
        try {
            unsigned int level = std::stoul(value);
            (*clan)->admin_set_min_application_level(level);
            PlayerOutput::send_to_actor(ctx.actor(), 
                fmt::format("Set minimum application level for clan '{}' to {}.", (*clan)->name(), level));
        } catch (const std::exception&) {
            PlayerOutput::send_to_actor(ctx.actor(), "Invalid application level.");
        }
    } else {
        PlayerOutput::send_to_actor(ctx.actor(), 
            "Unknown property. Valid properties: motd, dues, appfee, applevel");
    }
    
    return Ok(CommandResult::Success);
}

/**
 * Register all clan commands with the command system
 */
Result<void> register_all_clan_commands() {
    Log::info("Registering clan commands...");
    
    try {
        Commands()
            .command("clan", cmd_clan)
            .alias("cl")
            .category("Clan")
            .privilege(PrivilegeLevel::Player)
            .description("Clan information and communication")
            .build();
            
        Commands()
            .command("cset", cmd_cset)
            .category("Administrative")
            .privilege(PrivilegeLevel::God)
            .description("Set clan properties (gods only)")
            .build();
            
        Log::info("Clan commands registered successfully.");
        return Ok();
    } catch (const std::exception& e) {
        Log::error("Failed to register clan commands: {}", e.what());
        return Err("Failed to register clan commands");
    }
}

Result<void> unregister_all_clan_commands() {
    Log::info("Unregistering clan commands...");
    
    // The command system will handle cleanup automatically when the registry is destroyed
    
    return Ok();
}

} // namespace ClanCommands