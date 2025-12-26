#include "system_commands.hpp"
#include "../text/rich_text.hpp"
#include "../text/terminal_capabilities.hpp"
#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../database/social_queries.hpp"
#include "../net/player_connection.hpp"
#include "../server/persistence_manager.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <map>

namespace SystemCommands {

// =============================================================================
// System Command Implementations
// =============================================================================

Result<CommandResult> cmd_quit(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    ctx.send("Goodbye! Thanks for playing FieryMUD.");
    ctx.actor->set_position(Position::Standing);  // Player remains standing until logout

    // Save player data before logout
    if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        auto save_result = PersistenceManager::instance().save_player(*player);
        if (!save_result) {
            Log::error("Failed to save player {}: {}", player->name(), save_result.error().message);
            ctx.send("Warning: Character save failed!");
        } else {
            Log::info("Player {} saved successfully on quit", player->name());
        }
    }

    Log::info("Player {} quit the game", ctx.actor->name());

    return CommandResult::Success;
}

Result<CommandResult> cmd_save(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    ctx.send("Saving your character...");
    
    if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        auto save_result = PersistenceManager::instance().save_player(*player);
        if (!save_result) {
            Log::error("Manual save failed for player {}: {}", player->name(), save_result.error().message);
            ctx.send("Save failed: " + save_result.error().message);
            return CommandResult::ResourceError;
        } else {
            Log::info("Manual save successful for player {}", player->name());
            ctx.send("Character saved successfully.");
        }
    } else {
        ctx.send("Only players can be saved.");
        return CommandResult::InvalidTarget;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_help(const CommandContext &ctx) {
    auto& cmd_system = CommandSystem::instance();

    if (ctx.arg_count() == 0) {
        ctx.send("FieryMUD Help System");
        ctx.send("===================");
        ctx.send("Use 'help <topic>' for specific help on:");
        ctx.send("  commands      - List all available commands");
        ctx.send("  socials       - List available social commands");
        ctx.send("  <command>     - Help on a specific command");
        ctx.send("\nGeneral Topics:");
        ctx.send("  movement      - Moving around the world");
        ctx.send("  combat        - Fighting and combat");
        ctx.send("  objects       - Interacting with items");
        ctx.send("  communication - Talking to other players");
        ctx.send("\nFor a complete command list, type 'commands'.");
        return CommandResult::Success;
    }

    std::string topic{ctx.arg(0)};

    // First, check if it's a registered command
    const CommandInfo* cmd_info = cmd_system.find_command(topic);
    if (cmd_info) {
        // Display command help
        ctx.send(fmt::format("Help: {}", cmd_info->name));
        ctx.send(std::string(6 + cmd_info->name.length(), '='));

        // Show description if available
        if (!cmd_info->description.empty()) {
            ctx.send(fmt::format("\n{}", cmd_info->description));
        }

        // Show usage if available
        if (!cmd_info->usage.empty()) {
            ctx.send(fmt::format("\nUsage: {}", cmd_info->usage));
        }

        // Show extended help if available
        if (!cmd_info->help_text.empty()) {
            ctx.send(fmt::format("\n{}", cmd_info->help_text));
        }

        // Show aliases
        if (!cmd_info->aliases.empty()) {
            std::string alias_str;
            for (size_t i = 0; i < cmd_info->aliases.size(); ++i) {
                if (i > 0) alias_str += ", ";
                alias_str += cmd_info->aliases[i];
            }
            ctx.send(fmt::format("\nAliases: {}", alias_str));
        }

        // Show category
        if (!cmd_info->category.empty()) {
            ctx.send(fmt::format("Category: {}", cmd_info->category));
        }

        // If no description, usage, or help_text, show a minimal message
        if (cmd_info->description.empty() && cmd_info->usage.empty() && cmd_info->help_text.empty()) {
            ctx.send("\nNo detailed help available for this command yet.");
        }

        return CommandResult::Success;
    }

    // Check if it's a spell, skill, or ability
    if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        const LearnedAbility* ability = player->find_ability_by_name(topic);
        if (ability) {
            ctx.send(fmt::format("Help: {}", ability->name));
            ctx.send(std::string(6 + ability->name.length(), '='));
            ctx.send(fmt::format("Type: {}", ability->type));

            if (!ability->description.empty()) {
                ctx.send(fmt::format("\n{}", ability->description));
            } else {
                ctx.send("\nNo description available.");
            }

            if (ability->violent) {
                ctx.send("\nThis is a combat ability that requires a target.");
            }

            return CommandResult::Success;
        }
    }

    // Check for general topic help
    if (topic == "commands") {
        return cmd_commands(ctx);
    } else if (topic == "movement") {
        ctx.send("Movement Help");
        ctx.send("=============");
        ctx.send("north/n, south/s, east/e, west/w - Move in cardinal directions");
        ctx.send("up/u, down/d - Move vertically");
        ctx.send("exits - Show available exits from current room");
        ctx.send("look/l - Look around your current location");
    } else if (topic == "combat") {
        ctx.send("Combat Help");
        ctx.send("===========");
        ctx.send("kill <target> - Attack a target");
        ctx.send("flee - Attempt to escape from combat");
        ctx.send("release - Return to life as a ghost");
    } else if (topic == "objects") {
        ctx.send("Object Interaction Help");
        ctx.send("=======================");
        ctx.send("get <item> - Pick up an item");
        ctx.send("drop <item> - Drop an item");
        ctx.send("put <item> <container> - Put item in container");
        ctx.send("wear <item> - Wear equipment");
        ctx.send("remove <item> - Remove equipment");
        ctx.send("light <item> - Light a torch or lantern");
        ctx.send("eat <food> - Consume food");
        ctx.send("drink <item> - Drink from containers");
    } else if (topic == "communication") {
        ctx.send("Communication Help");
        ctx.send("==================");
        ctx.send("say <message> - Speak to everyone in the room");
        ctx.send("tell <player> <message> - Send private message");
        ctx.send("emote <action> - Perform an emote");
        ctx.send("whisper <player> <message> - Whisper to someone nearby");
        ctx.send("shout <message> - Shout to nearby areas");
        ctx.send("gossip <message> - Talk on gossip channel");
    } else if (topic == "socials") {
        // Redirect to socials command
        ctx.send("Use the 'socials' command to see available social actions.");
        ctx.send("Socials are loaded from the database and include actions like:");
        ctx.send("  smile, nod, wave, bow, laugh, hug, etc.");
    } else {
        ctx.send_error(fmt::format("No help available for '{}'.", topic));
        ctx.send("Try 'help' for a list of topics, or 'commands' for all commands.");
        return CommandResult::InvalidTarget;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_commands(const CommandContext &ctx) {
    auto& cmd_system = CommandSystem::instance();

    ctx.send("Available Commands:");
    ctx.send("===================");

    // Get all categories
    auto categories = cmd_system.get_categories();

    // Build a map of category -> commands that the actor can use
    std::map<std::string, std::vector<std::string>> category_commands;

    // Get all commands available to this actor
    auto available_commands = cmd_system.get_available_commands(ctx.actor);

    // Group commands by category
    for (const auto& cmd_name : available_commands) {
        const CommandInfo* info = cmd_system.find_command(cmd_name);
        if (info) {
            std::string category = info->category.empty() ? "Other" : info->category;

            // Build command string with aliases
            std::string cmd_display = info->name;
            if (!info->aliases.empty()) {
                // Show first alias as shortcut
                cmd_display += "/" + info->aliases[0];
            }

            // Avoid duplicates (aliases might cause the same command to appear multiple times)
            auto& cmds = category_commands[category];
            if (std::find(cmds.begin(), cmds.end(), cmd_display) == cmds.end()) {
                cmds.push_back(cmd_display);
            }
        }
    }

    // Define category display order (most common first)
    std::vector<std::string> category_order = {
        "Information", "Communication", "Movement", "Position",
        "Object", "Combat", "Social", "System",
        "Building", "Administration", "Other"
    };

    // Display commands by category
    for (const auto& cat_name : category_order) {
        auto it = category_commands.find(cat_name);
        if (it != category_commands.end() && !it->second.empty()) {
            ctx.send(fmt::format("\n{}:", cat_name));

            // Sort commands in this category
            auto& cmds = it->second;
            std::sort(cmds.begin(), cmds.end());

            // Format commands in columns (roughly 4 per line, 18 chars each)
            constexpr int COLUMN_WIDTH = 18;
            constexpr int COLUMNS = 4;

            std::string line = "  ";
            int col = 0;
            for (const auto& cmd : cmds) {
                // Truncate if too long
                std::string display = cmd;
                if (display.length() > COLUMN_WIDTH - 2) {
                    display = display.substr(0, COLUMN_WIDTH - 4) + "..";
                }

                line += fmt::format("{:<{}}", display, COLUMN_WIDTH);
                col++;

                if (col >= COLUMNS) {
                    ctx.send(line);
                    line = "  ";
                    col = 0;
                }
            }

            // Send remaining commands
            if (col > 0) {
                ctx.send(line);
            }
        }
    }

    // Show any categories not in the predefined order
    for (const auto& [cat_name, cmds] : category_commands) {
        if (std::find(category_order.begin(), category_order.end(), cat_name) == category_order.end()) {
            if (!cmds.empty()) {
                ctx.send(fmt::format("\n{}:", cat_name));

                std::string line = "  ";
                int col = 0;
                for (const auto& cmd : cmds) {
                    line += fmt::format("{:<18}", cmd);
                    col++;
                    if (col >= 4) {
                        ctx.send(line);
                        line = "  ";
                        col = 0;
                    }
                }
                if (col > 0) {
                    ctx.send(line);
                }
            }
        }
    }

    ctx.send("\nUse 'help <command>' for more information on a specific command.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_socials(const CommandContext &ctx) {
    auto& cache = SocialCache::instance();

    if (!cache.is_loaded()) {
        ctx.send_error("Socials have not been loaded from the database.");
        return CommandResult::SystemError;
    }

    const auto& socials = cache.all();

    ctx.send("Available Social Commands:");
    ctx.send("==========================");
    ctx.send(fmt::format("({} socials loaded from database)\n", socials.size()));

    // Collect all social names and sort them
    std::vector<std::string> names;
    names.reserve(socials.size());
    for (const auto& [name, social] : socials) {
        if (!social.hide) {
            names.push_back(name);
        }
    }
    std::sort(names.begin(), names.end());

    // Display in columns (6 per line, 12 chars each)
    constexpr int COLUMN_WIDTH = 12;
    constexpr int COLUMNS = 6;

    std::string line = "  ";
    int col = 0;
    for (const auto& name : names) {
        std::string display = name;
        if (display.length() > COLUMN_WIDTH - 1) {
            display = display.substr(0, COLUMN_WIDTH - 2) + ".";
        }

        line += fmt::format("{:<{}}", display, COLUMN_WIDTH);
        col++;

        if (col >= COLUMNS) {
            ctx.send(line);
            line = "  ";
            col = 0;
        }
    }

    // Send remaining socials
    if (col > 0) {
        ctx.send(line);
    }

    ctx.send("\nUse a social with no argument, or with a target name.");
    ctx.send("Example: smile, wave bob, hug self");

    return CommandResult::Success;
}

Result<CommandResult> cmd_prompt(const CommandContext &ctx) {
    ctx.send("Prompt command executed successfully.");
    return CommandResult::Success;
}

/**
 * Test command to demonstrate the rich text formatting system.
 * Usage: richtest [demo_type]
 * 
 * Demo types:
 * - colors: Show color palette
 * - progress: Show progress bars
 * - table: Show table formatting
 * - combat: Show combat message formatting
 * - all: Show all demos
 */
Result<CommandResult> cmd_richtest(const CommandContext& ctx) {
    std::string demo_type = ctx.arg_or(0, "all");
    
    // Detect terminal capabilities first
    auto caps = TerminalCapabilities::detect_capabilities();
    
    if (demo_type == "capabilities" || demo_type == "all") {
        ctx.send_header("Terminal Capabilities");
        
        RichText cap_info;
        cap_info.text("Terminal: ").bold(caps.terminal_name).text("\n");
        cap_info.text("Color support: ");
        cap_info.colored(caps.supports_color ? "Yes" : "No", 
                        caps.supports_color ? Color::BrightGreen : Color::BrightRed);
        cap_info.text("\n");
        
        cap_info.text("256-color support: ");
        cap_info.colored(caps.supports_256_color ? "Yes" : "No",
                        caps.supports_256_color ? Color::BrightGreen : Color::BrightRed);
        cap_info.text("\n");
        
        cap_info.text("True color (RGB): ");
        cap_info.colored(caps.supports_true_color ? "Yes" : "No",
                        caps.supports_true_color ? Color::BrightGreen : Color::BrightRed);
        cap_info.text("\n");
        
        cap_info.text("Unicode support: ");
        cap_info.colored(caps.supports_unicode ? "Yes" : "No",
                        caps.supports_unicode ? Color::BrightGreen : Color::BrightRed);
        cap_info.text("\n");
        
        std::string level_name;
        Color level_color = Color::White;
        switch (caps.overall_level) {
            case TerminalCapabilities::SupportLevel::None:
                level_name = "None (Text only)";
                level_color = Color::BrightRed;
                break;
            case TerminalCapabilities::SupportLevel::Basic:
                level_name = "Basic (16 colors)";
                level_color = Color::Yellow;
                break;
            case TerminalCapabilities::SupportLevel::Standard:
                level_name = "Standard (ANSI colors)";
                level_color = Color::BrightYellow;
                break;
            case TerminalCapabilities::SupportLevel::Extended:
                level_name = "Extended (256 colors)";
                level_color = Color::BrightCyan;
                break;
            case TerminalCapabilities::SupportLevel::Full:
                level_name = "Full (True color + Unicode)";
                level_color = Color::BrightGreen;
                break;
        }
        
        cap_info.text("Overall level: ");
        cap_info.colored(level_name, level_color);
        
        ctx.send_rich(cap_info);
        ctx.send_separator();
    }
    
    if (demo_type == "colors" || demo_type == "all") {
        ctx.send_header("Color Palette Demo");
        
        RichText color_demo;
        color_demo.text("Standard Colors: ");
        color_demo.colored("Red ", Color::Red);
        color_demo.colored("Green ", Color::Green);
        color_demo.colored("Blue ", Color::Blue);
        color_demo.colored("Yellow ", Color::Yellow);
        color_demo.colored("Cyan ", Color::Cyan);
        color_demo.colored("Magenta", Color::Magenta);
        color_demo.text("\n");
        
        color_demo.text("Bright Colors: ");
        color_demo.colored("Red ", Color::BrightRed);
        color_demo.colored("Green ", Color::BrightGreen);
        color_demo.colored("Blue ", Color::BrightBlue);
        color_demo.colored("Yellow ", Color::BrightYellow);
        color_demo.colored("Cyan ", Color::BrightCyan);
        color_demo.colored("Magenta", Color::BrightMagenta);
        color_demo.text("\n");
        
        color_demo.text("RGB Colors: ");
        color_demo.rgb("Health ", Colors::Health);
        color_demo.rgb("Mana ", Colors::Mana);
        color_demo.rgb("Damage ", Colors::Damage);
        color_demo.rgb("Healing ", Colors::Healing);
        color_demo.rgb("Experience", Colors::Experience);
        color_demo.text("\n");
        
        ctx.send_rich(color_demo);
        ctx.send_separator();
    }
    
    if (demo_type == "progress" || demo_type == "all") {
        ctx.send_header("Progress Bar Demo");
        
        ctx.send_progress_bar("Health", 0.85f);
        ctx.send_progress_bar("Mana", 0.42f);  
        ctx.send_progress_bar("Movement", 1.0f);
        ctx.send_progress_bar("Experience", 0.15f);
        
        RichText custom_bars;
        custom_bars.text("Custom Progress Bars:\n");
        
        // Use adaptive characters based on terminal capabilities
        auto progress_chars = TerminalCapabilities::get_progress_chars(caps);
        
        custom_bars.text("Loading: ");
        custom_bars.progress_bar(0.7f, 30, progress_chars.filled, progress_chars.empty);
        custom_bars.text("\nDownload: ");
        custom_bars.progress_bar(0.3f, 20, "=", ".");
        
        ctx.send_rich(custom_bars);
        ctx.send_separator();
    }
    
    if (demo_type == "table" || demo_type == "all") {
        ctx.send_header("Table Formatting Demo");
        
        std::vector<std::string> headers = {"Player", "Level", "Class", "HP", "Status"};
        std::vector<std::vector<std::string>> rows = {
            {"Gandalf", "50", "Wizard", "450/450", "Healthy"},
            {"Legolas", "45", "Ranger", "380/400", "Injured"},
            {"Gimli", "48", "Warrior", "500/500", "Healthy"},
            {"Frodo", "25", "Rogue", "220/250", "Tired"}
        };
        
        ctx.send_table(headers, rows);
        ctx.send_separator();
    }
    
    if (demo_type == "combat" || demo_type == "all") {
        ctx.send_header("Combat Message Demo");
        
        ctx.send_damage_report(25, "longsword");
        ctx.send_healing_report(15, "healing potion");
        
        RichText combat_scene;
        combat_scene.text("The ");
        combat_scene.bold("orc warrior");
        combat_scene.text(" swings his ");
        combat_scene.colored("rusty axe", Color::BrightRed);
        combat_scene.text(" at you, dealing ");
        combat_scene.rgb("18", Colors::Damage);
        combat_scene.text(" damage!\n");
        
        combat_scene.text("You cast ");
        combat_scene.italic("heal");
        combat_scene.text(" on yourself, restoring ");
        combat_scene.rgb("+12", Colors::Healing);
        combat_scene.text(" hit points.");
        
        ctx.send_rich(combat_scene);
        ctx.send_separator();
    }
    
    if (demo_type == "text" || demo_type == "all") {
        ctx.send_header("Text Styling Demo");
        
        RichText styles;
        styles.text("Text Styles: ");
        styles.bold("Bold ");
        styles.italic("Italic ");
        styles.underline("Underlined ");
        styles.strikethrough("Strikethrough");
        styles.text("\n");
        
        styles.text("Highlights: ");
        styles.highlight("Important", Color::Black, BackgroundColor::BrightYellow);
        styles.text(" ");
        styles.highlight("Warning", Color::White, BackgroundColor::Red);
        styles.text(" ");
        styles.highlight("Success", Color::Black, BackgroundColor::Green);
        
        ctx.send_rich(styles);
        ctx.send_separator();
    }
    
    RichText summary;
    summary.colored("✓ Rich text formatting system is working!", Color::BrightGreen);
    ctx.send_rich(summary);
    
    return CommandResult::Success;
}

Result<CommandResult> cmd_clientinfo(const CommandContext& ctx) {
    if (!ctx.actor) {
        ctx.send_line("Error: No actor available.");
        return CommandResult::InvalidSyntax;
    }
    
    // Cast Actor to Player to access get_output()
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_line("Error: Only players can view client information.");
        return CommandResult::InvalidSyntax;
    }
    
    // Cast PlayerOutput to PlayerConnection to access capabilities
    auto output = player->get_output();
    auto connection = std::dynamic_pointer_cast<PlayerConnection>(output);
    if (!connection) {
        ctx.send_line("Error: No connection available or not a player connection.");
        return CommandResult::InvalidSyntax;
    }
    
    const auto& caps = connection->get_terminal_capabilities();
    
    ctx.send_header("Client Capability Information");
    
    // Basic client info
    RichText client_info;
    client_info.colored("Client: ", Color::BrightCyan);
    if (!caps.client_version.empty()) {
        client_info.text(fmt::format("{} {}\n", caps.client_name, caps.client_version));
    } else {
        client_info.text(fmt::format("{}\n", caps.client_name));
    }
    
    client_info.colored("Terminal: ", Color::BrightCyan);
    client_info.text(fmt::format("{}\n", caps.terminal_name));
    
    client_info.colored("Detection Method: ", Color::BrightCyan);
    std::string method_name;
    switch (caps.detection_method) {
        case TerminalCapabilities::DetectionMethod::Environment:
            method_name = "Server Environment";
            break;
        case TerminalCapabilities::DetectionMethod::MTTS:
            method_name = fmt::format("MTTS (bitvector: {})", caps.mtts_bitvector);
            break;
        case TerminalCapabilities::DetectionMethod::GMCP:
            method_name = "GMCP Client Info";
            break;
        case TerminalCapabilities::DetectionMethod::NewEnviron:
            method_name = "NEW-ENVIRON Protocol";
            break;
    }
    client_info.text(method_name);
    ctx.send_rich(client_info);
    
    ctx.send_separator();
    
    // Display capabilities in a table
    std::vector<std::string> headers = {"Capability", "Supported"};
    std::vector<std::vector<std::string>> rows;
    
    auto add_capability = [&](const std::string& name, bool supported) {
        rows.push_back({name, supported ? "✓ Yes" : "✗ No"});
    };
    
    add_capability("Basic Colors", caps.supports_color);
    add_capability("256 Colors", caps.supports_256_color);
    add_capability("True Color (24-bit)", caps.supports_true_color);
    add_capability("Unicode/UTF-8", caps.supports_unicode);
    add_capability("Bold Text", caps.supports_bold);
    add_capability("Italic Text", caps.supports_italic);
    add_capability("Underline Text", caps.supports_underline);
    add_capability("GMCP Protocol", caps.supports_gmcp);
    add_capability("Mouse Support", caps.supports_mouse);
    add_capability("Hyperlinks", caps.supports_hyperlinks);
    add_capability("Screen Reader", caps.supports_screen_reader);
    add_capability("TLS Encryption", caps.supports_tls);
    
    ctx.send_table(headers, rows);
    
    ctx.send_separator();
    
    // Overall support level
    RichText support_level;
    support_level.colored("Overall Support Level: ", Color::BrightYellow);
    
    Color level_color = Color::White;
    std::string level_name;
    
    switch (caps.overall_level) {
        case TerminalCapabilities::SupportLevel::None:
            level_name = "None (Basic text only)";
            level_color = Color::Red;
            break;
        case TerminalCapabilities::SupportLevel::Basic:
            level_name = "Basic (Limited colors)";
            level_color = Color::Yellow;
            break;
        case TerminalCapabilities::SupportLevel::Standard:
            level_name = "Standard (16 colors + formatting)";
            level_color = Color::BrightBlue;
            break;
        case TerminalCapabilities::SupportLevel::Extended:
            level_name = "Extended (256 colors + advanced)";
            level_color = Color::BrightGreen;
            break;
        case TerminalCapabilities::SupportLevel::Full:
            level_name = "Full (True color + all features)";
            level_color = Color::BrightMagenta;
            break;
    }
    
    support_level.colored(level_name, level_color);
    ctx.send_rich(support_level);
    
    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands()
        .command("quit", cmd_quit)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("save", cmd_save)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("help", cmd_help)
        .alias("h")
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Get help on commands and topics")
        .usage("help [topic|command]")
        .build();

    Commands()
        .command("commands", cmd_commands)
        .alias("comm")
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("List all available commands")
        .build();

    Commands()
        .command("socials", cmd_socials)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("List all available social commands")
        .build();

    Commands()
        .command("prompt", cmd_prompt)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("richtest", cmd_richtest)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("clientinfo", cmd_clientinfo)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .build();

    return Success();
}

} // namespace SystemCommands