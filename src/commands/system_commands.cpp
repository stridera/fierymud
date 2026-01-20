#include "system_commands.hpp"
#include "../text/rich_text.hpp"
#include "../text/terminal_capabilities.hpp"
#include "../core/actor.hpp"
#include "../core/ability_executor.hpp"
#include "../core/logging.hpp"
#include "../core/money.hpp"
#include "../core/object.hpp"
#include "../database/connection_pool.hpp"
#include "../database/social_queries.hpp"
#include "../database/world_queries.hpp"
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

    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send("Only players can quit.");
        return CommandResult::InvalidTarget;
    }

    ctx.send("Goodbye! Thanks for playing FieryMUD.");
    player->set_position(Position::Standing);  // Player remains standing until logout

    // For mortals, remove from room BEFORE saving so they return to recall on login
    // Gods keep their saved location regardless of how they log out
    if (!player->is_god()) {
        if (ctx.room) {
            ctx.room->remove_actor(player->id());
        }
        player->set_current_room({});  // Ensure current_room is cleared for save
    }

    // Save player data before logout
    auto save_result = PersistenceManager::instance().save_player(*player);
    if (!save_result) {
        Log::error("Failed to save player {}: {}", player->name(), save_result.error().message);
        ctx.send("Warning: Character save failed!");
    } else {
        Log::info("Player {} saved successfully on quit", player->name());
    }

    Log::info("Player {} quit the game", player->name());

    // Remove god from room (mortals already removed above)
    if (player->is_god() && ctx.room) {
        ctx.room->remove_actor(player->id());
    }

    // Disconnect the player connection
    if (auto output = player->get_output()) {
        output->disconnect("Quit");
    }

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

/**
 * Helper to remove temporary items from player inventory/equipment
 * Returns count of items removed
 */
static int remove_temporary_items(Player& player, const CommandContext& ctx) {
    int removed = 0;

    // Remove from inventory
    auto inv_items = player.inventory().get_all_items();
    for (const auto& item : inv_items) {
        if (item && item->has_flag(ObjectFlag::Temporary)) {
            player.inventory().remove_item(item);
            ctx.send(fmt::format("  {} crumbles to dust.", item->display_name()));
            removed++;
        }
    }

    // Remove from equipment
    auto equipped = player.equipment().get_all_equipped_with_slots();
    for (const auto& [slot, item] : equipped) {
        if (item && item->has_flag(ObjectFlag::Temporary)) {
            player.equipment().unequip_item(slot);
            ctx.send(fmt::format("  {} crumbles to dust.", item->display_name()));
            removed++;
        }
    }

    return removed;
}

Result<CommandResult> cmd_rent(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send("Only players can rent rooms.");
        return CommandResult::InvalidTarget;
    }

    if (!ctx.room) {
        return std::unexpected(Errors::InvalidState("No room context"));
    }

    // Check for receptionist in the room
    bool has_receptionist = false;
    for (const auto& actor : ctx.room->contents().actors) {
        if (auto mob = std::dynamic_pointer_cast<Mobile>(actor)) {
            if (mob->is_receptionist()) {
                has_receptionist = true;
                break;
            }
        }
    }

    if (!has_receptionist) {
        ctx.send("You need to find an inn with a receptionist to rent a room.");
        return CommandResult::InvalidState;
    }

    // Calculate rent cost based on player level and items
    long rent_cost = player->level() * 10; // Base: 10 copper per level
    Log::info("Rent calculation: player {} level {} * 10 = {} copper, current gold = {}",
              player->name(), player->level(), rent_cost, player->stats().gold);

    if (!player->can_afford(rent_cost)) {
        auto cost_money = fiery::Money::from_copper(rent_cost);
        ctx.send(fmt::format("The receptionist says, 'That will be {}. You don't have enough!'",
                             cost_money.to_string()));
        return CommandResult::ResourceError;
    }

    // Charge the player
    player->take_wealth(rent_cost);
    auto cost_money = fiery::Money::from_copper(rent_cost);
    ctx.send(fmt::format("The receptionist takes {} for your room.", cost_money.to_string()));

    // Remove temporary items
    int removed = remove_temporary_items(*player, ctx);
    if (removed > 0) {
        ctx.send(fmt::format("{} temporary item{} could not be stored.",
                             removed, removed == 1 ? "" : "s"));
    }

    // Save the player (while still in room, so location is preserved)
    auto save_result = PersistenceManager::instance().save_player(*player);
    if (!save_result) {
        Log::error("Rent save failed for player {}: {}", player->name(), save_result.error().message);
        ctx.send("Error saving your character! Please try again.");
        return CommandResult::ResourceError;
    }

    ctx.send("The receptionist shows you to your room. Sweet dreams!");
    ctx.send("Goodbye! Come back soon to continue your adventures.");
    Log::info("Player {} rented at inn for {}", player->name(), cost_money.to_string());

    // Remove player from room
    if (ctx.room) {
        ctx.room->remove_actor(player->id());
    }

    // Disconnect the player connection
    if (auto output = player->get_output()) {
        output->disconnect("Renting a room");
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_camp(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send("Only players can make camp.");
        return CommandResult::InvalidTarget;
    }

    // Check if player is in combat
    if (player->is_fighting()) {
        ctx.send("You can't make camp while fighting!");
        return CommandResult::InvalidState;
    }

    // Check player position
    if (player->position() != Position::Standing && player->position() != Position::Sitting) {
        ctx.send("You need to be standing or sitting to make camp.");
        return CommandResult::InvalidState;
    }

    ctx.send("You begin setting up camp...");

    // Remove temporary items
    int removed = remove_temporary_items(*player, ctx);
    if (removed > 0) {
        ctx.send(fmt::format("{} temporary item{} faded away as you made camp.",
                             removed, removed == 1 ? "" : "s"));
    }

    // Save the player
    auto save_result = PersistenceManager::instance().save_player(*player);
    if (!save_result) {
        Log::error("Camp save failed for player {}: {}", player->name(), save_result.error().message);
        ctx.send("Error saving your character! Please try again.");
        return CommandResult::ResourceError;
    }

    ctx.send("You have made camp and saved your progress.");
    Log::info("Player {} camped and saved", player->name());

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
        const LearnedAbility* learned = player->find_ability_by_name(topic);
        if (learned) {
            // Get full ability data from cache
            auto& cache = FieryMUD::AbilityCache::instance();
            const auto* ability = cache.get_ability(learned->ability_id);

            ctx.send("===============================================");

            // Usage line
            std::string type_lower = learned->type;
            std::transform(type_lower.begin(), type_lower.end(), type_lower.begin(),
                          [](unsigned char c) { return std::tolower(c); });
            if (type_lower == "spell") {
                ctx.send(fmt::format("Usage         : cast '{}' [target]", learned->plain_name));
            } else if (type_lower == "skill") {
                ctx.send(fmt::format("Usage         : {} [target]", learned->plain_name));
            } else {
                ctx.send(fmt::format("Usage         : {} [target]", learned->plain_name));
            }

            if (ability) {
                // Min position - parse_position returns: 5=prone, 7=sitting, 9=standing
                std::string position_str = "standing";
                if (ability->min_position <= 5) {
                    position_str = "resting";
                } else if (ability->min_position <= 7) {
                    position_str = "sitting";
                }
                ctx.send(fmt::format("Min. position : {}", position_str));

                // Combat OK
                ctx.send(fmt::format("OK in combat  : {}", ability->combat_ok ? "yes" : "no"));

                // Aggressive
                ctx.send(fmt::format("Aggressive    : {}", ability->violent ? "yes" : "no"));

                // Area of effect
                ctx.send(fmt::format("Area of effect: {}", ability->is_area ? "area" : "target"));

                // Sphere/School
                if (!ability->sphere.empty()) {
                    std::string sphere_lower = ability->sphere;
                    std::transform(sphere_lower.begin(), sphere_lower.end(),
                                  sphere_lower.begin(), ::tolower);
                    ctx.send(fmt::format("Sphere        : {}", sphere_lower));
                }

                // Cast time - use database value if set, otherwise circle-based default
                int cast_rounds = ability->cast_time_rounds > 0
                    ? ability->cast_time_rounds
                    : (learned->circle + 1);
                ctx.send(fmt::format("Cast time     : {} round{}",
                                    cast_rounds,
                                    cast_rounds == 1 ? "" : "s"));
            }

            // Classes and circles
            auto classes = cache.get_ability_classes(learned->ability_id);
            if (!classes.empty()) {
                bool first = true;
                for (const auto& cls : classes) {
                    if (first) {
                        ctx.send(fmt::format("Classes       : {:<12} Circle {}",
                                            cls.class_name, cls.circle));
                        first = false;
                    } else {
                        ctx.send(fmt::format("              : {:<12} Circle {}",
                                            cls.class_name, cls.circle));
                    }
                }
            }

            ctx.send("===============================================");

            // Description
            if (!learned->description.empty()) {
                ctx.send(fmt::format("\n{}", learned->description));
            } else {
                ctx.send("\nNo description available.");
            }

            // Related topics
            if (!learned->sphere.empty()) {
                std::string sphere_upper = learned->sphere;
                std::transform(sphere_upper.begin(), sphere_upper.end(), sphere_upper.begin(),
                              [](unsigned char c) { return std::toupper(c); });
                ctx.send(fmt::format("\nSee also: {}", sphere_upper));
            }

            return CommandResult::Success;
        }
    }

    // Check for general topic help - special cases first
    if (topic == "commands") {
        return cmd_commands(ctx);
    } else if (topic == "socials") {
        // Redirect to socials command
        ctx.send("Use the 'socials' command to see available social actions.");
        ctx.send("Socials are loaded from the database and include actions like:");
        ctx.send("  smile, nod, wave, bow, laugh, hug, etc.");
        return CommandResult::Success;
    }

    // Try to find help entry in database (covers all topics including lore, cities, etc.)
    auto help_result = ConnectionPool::instance().execute([&topic](pqxx::work& txn)
        -> Result<std::optional<WorldQueries::HelpEntryData>> {
        return WorldQueries::load_help_entry(txn, topic);
    });

    if (help_result && *help_result) {
        const auto& entry = **help_result;

        // Check level requirement
        int player_level = 0;
        if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
            player_level = player->level();
        }
        if (entry.min_level > player_level) {
            ctx.send_error(fmt::format("No help available for '{}'.", topic));
            ctx.send("Try 'help' for a list of topics, or 'commands' for all commands.");
            return CommandResult::InvalidTarget;
        }

        // Display help entry
        ctx.send(fmt::format("Help: {}", entry.title));
        ctx.send(std::string(6 + entry.title.length(), '='));

        // Show category and sphere if present
        if (entry.category) {
            ctx.send(fmt::format("Category: {}", *entry.category));
        }
        if (entry.sphere) {
            ctx.send(fmt::format("Sphere: {}", *entry.sphere));
        }

        // Show content
        ctx.send(fmt::format("\n{}", entry.content));

        // Show usage if present
        if (entry.usage) {
            ctx.send(fmt::format("\nUsage: {}", *entry.usage));
        }

        // Show duration if present (for spells)
        if (entry.duration) {
            ctx.send(fmt::format("Duration: {}", *entry.duration));
        }

        // Show related keywords
        if (entry.keywords.size() > 1) {
            std::string keywords_str;
            for (size_t i = 0; i < entry.keywords.size(); ++i) {
                if (i > 0) keywords_str += ", ";
                keywords_str += entry.keywords[i];
            }
            ctx.send(fmt::format("\nSee also: {}", keywords_str));
        }

        return CommandResult::Success;
    }

    // Fallback to hardcoded topics (backward compatibility)
    if (topic == "movement") {
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

/**
 * Prompt command - manage the player's command line prompt
 *
 * Usage:
 *   prompt           - Show current prompt format
 *   prompt ?         - Show preset prompts (same as 'help prompt')
 *   prompt <#>       - Select a preset prompt by number
 *   prompt <format>  - Set a custom prompt format
 *
 * Prompt format codes:
 *   %h - current hit points    %H - max hit points
 *   %v - current stamina       %V - max stamina
 *   %l - level                 %g - gold
 *   %x - experience            %X - exp to next level
 *   %t - tank condition        %T - target condition
 *   %n - newline               %% - literal %
 *
 * Color markup (XML-lite format):
 *   <red>text</red>     - Named colors (red, green, blue, yellow, etc.)
 *   <bred>bold red</bred> - Bright/bold variants
 *   <#FF0000>rgb</...>  - RGB hex colors (24-bit)
 *   <c196>indexed</c196> - 256-color palette
 *   </> - Reset to normal
 */
Result<CommandResult> cmd_prompt(const CommandContext &ctx) {
    // Preset prompts
    static const std::vector<std::pair<std::string, std::string>> preset_prompts = {
        {"minimal",     "<%h/%Hhp %v/%Vs>"},
        {"standard",    "<%h/%Hhp %v/%Vs %x/exp>"},
        {"combat",      "<%h/%Hhp %v/%Vs %T>"},
        {"full",        "<%h/%Hhp %v/%Vs %x/exp %g/gold %T>"},
        {"colored",     "<<red>%h</>/%Hhp <yellow>%v</>/%Vs>"},
        {"classic",     "<%h/%Hhp %v/%Vs>"},
    };

    // Get the player
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use the prompt command.");
        return CommandResult::InvalidState;
    }

    // No arguments - show current prompt
    if (ctx.arg_count() == 0) {
        ctx.send(fmt::format("Your current prompt is: {}", player->prompt()));
        return CommandResult::Success;
    }

    std::string_view arg = ctx.arg(0);

    // Help/list presets
    if (arg == "?" || arg == "help" || arg == "list") {
        ctx.send("Available preset prompts:");
        ctx.send("");
        int i = 0;
        for (const auto& [name, format] : preset_prompts) {
            ctx.send(fmt::format("  {:2d}. {:<12} {}", i++, name, format));
        }
        ctx.send("");
        ctx.send("Usage: prompt <#> to select a preset, or prompt <format> for custom.");
        ctx.send("");
        ctx.send("Prompt format codes:");
        ctx.send("  %h/%H - current/max hit points    %v/%V - current/max stamina");
        ctx.send("  %l - level                        %g - gold");
        ctx.send("  %x/%X - exp/exp to next level     %t/%T - tank/target condition");
        ctx.send("  %n - newline                      %% - literal %");
        ctx.send("");
        ctx.send("Color markup: <red>text</red>, <bred>bold</bred>, <#FF0000>rgb</...>");
        return CommandResult::Success;
    }

    // Check if it's a number (preset selection)
    if (arg.length() <= 2 && std::all_of(arg.begin(), arg.end(), ::isdigit)) {
        int index = std::stoi(std::string{arg});
        if (index < 0 || index >= static_cast<int>(preset_prompts.size())) {
            ctx.send_error(fmt::format("Invalid preset number. Use 0-{}.", preset_prompts.size() - 1));
            return CommandResult::InvalidSyntax;
        }

        const auto& [name, format] = preset_prompts[index];
        player->set_prompt(format);
        ctx.send(fmt::format("Your prompt is now set to the '{}' preset.", name));
        ctx.send(fmt::format("Format: {}", format));
        return CommandResult::Success;
    }

    // Custom prompt - get entire argument string
    std::string custom_prompt = ctx.args_from(0);

    // Basic validation - limit length
    if (custom_prompt.length() > 200) {
        ctx.send_error("Prompt too long. Maximum 200 characters.");
        return CommandResult::InvalidSyntax;
    }

    player->set_prompt(custom_prompt);
    ctx.send(fmt::format("Your prompt is now: {}", custom_prompt));
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
// Feedback Commands
// =============================================================================

namespace {
/**
 * Helper function to save a report to the database.
 * Returns the report ID on success, or an error message on failure.
 */
Result<int> save_report_to_db(
    const std::string& report_type,
    const std::string& reporter_name,
    const std::optional<std::string>& reporter_id,
    const std::optional<int>& room_zone_id,
    const std::optional<int>& room_id,
    const std::string& message) {

    return ConnectionPool::instance().execute(
        [&](pqxx::work& txn) -> Result<int> {
            return WorldQueries::save_report(
                txn, report_type, reporter_name, reporter_id,
                room_zone_id, room_id, message);
        });
}
} // anonymous namespace

Result<CommandResult> cmd_bug(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Please enter a message describing the bug.");
        ctx.send("Usage: bug <description>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = ctx.args_from(0);
    std::string player_name = ctx.actor ? std::string{ctx.actor->name()} : "Unknown";

    // Get reporter ID if they're a player
    std::optional<std::string> reporter_id;
    if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        auto db_id = player->database_id();
        if (!db_id.empty()) {
            reporter_id = std::string{db_id};
        }
    }

    // Get room location
    std::optional<int> room_zone_id;
    std::optional<int> room_id;
    if (ctx.room) {
        room_zone_id = ctx.room->id().zone_id();
        room_id = ctx.room->id().local_id();
    }

    // Save to database
    auto result = save_report_to_db("BUG", player_name, reporter_id, room_zone_id, room_id, message);
    if (result) {
        Log::info("[BUG] Report #{} from {} [Room {}:{}]: {}",
            *result, player_name,
            room_zone_id.value_or(-1), room_id.value_or(-1), message);
        ctx.send("Thank you for reporting this bug! Your report has been saved.");
    } else {
        // Fall back to logging if database save fails
        Log::warn("[BUG] Database save failed for {}: {} (logging to file instead)",
            player_name, result.error().message);
        Log::info("[BUG] {} [Room {}:{}]: {}", player_name,
            room_zone_id.value_or(-1), room_id.value_or(-1), message);
        ctx.send("Thank you for reporting this bug! The immortals have been notified.");
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_idea(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Please enter a message describing your idea.");
        ctx.send("Usage: idea <description>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = ctx.args_from(0);
    std::string player_name = ctx.actor ? std::string{ctx.actor->name()} : "Unknown";

    // Get reporter ID if they're a player
    std::optional<std::string> reporter_id;
    if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        auto db_id = player->database_id();
        if (!db_id.empty()) {
            reporter_id = std::string{db_id};
        }
    }

    // Get room location
    std::optional<int> room_zone_id;
    std::optional<int> room_id;
    if (ctx.room) {
        room_zone_id = ctx.room->id().zone_id();
        room_id = ctx.room->id().local_id();
    }

    // Save to database
    auto result = save_report_to_db("IDEA", player_name, reporter_id, room_zone_id, room_id, message);
    if (result) {
        Log::info("[IDEA] Report #{} from {} [Room {}:{}]: {}",
            *result, player_name,
            room_zone_id.value_or(-1), room_id.value_or(-1), message);
        ctx.send("Thank you for sharing your idea! Your suggestion has been saved.");
    } else {
        // Fall back to logging if database save fails
        Log::warn("[IDEA] Database save failed for {}: {} (logging to file instead)",
            player_name, result.error().message);
        Log::info("[IDEA] {} [Room {}:{}]: {}", player_name,
            room_zone_id.value_or(-1), room_id.value_or(-1), message);
        ctx.send("Thank you for sharing your idea! The immortals have been notified.");
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_typo(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Please enter a message describing the typo.");
        ctx.send("Usage: typo <description>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = ctx.args_from(0);
    std::string player_name = ctx.actor ? std::string{ctx.actor->name()} : "Unknown";

    // Get reporter ID if they're a player
    std::optional<std::string> reporter_id;
    if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        auto db_id = player->database_id();
        if (!db_id.empty()) {
            reporter_id = std::string{db_id};
        }
    }

    // Get room location
    std::optional<int> room_zone_id;
    std::optional<int> room_id;
    if (ctx.room) {
        room_zone_id = ctx.room->id().zone_id();
        room_id = ctx.room->id().local_id();
    }

    // Save to database
    auto result = save_report_to_db("TYPO", player_name, reporter_id, room_zone_id, room_id, message);
    if (result) {
        Log::info("[TYPO] Report #{} from {} [Room {}:{}]: {}",
            *result, player_name,
            room_zone_id.value_or(-1), room_id.value_or(-1), message);
        ctx.send("Thank you for reporting this typo! Your report has been saved.");
    } else {
        // Fall back to logging if database save fails
        Log::warn("[TYPO] Database save failed for {}: {} (logging to file instead)",
            player_name, result.error().message);
        Log::info("[TYPO] {} [Room {}:{}]: {}", player_name,
            room_zone_id.value_or(-1), room_id.value_or(-1), message);
        ctx.send("Thank you for reporting this typo! The immortals have been notified.");
    }

    return CommandResult::Success;
}

// =============================================================================
// Utility Commands
// =============================================================================

Result<CommandResult> cmd_date(const CommandContext &ctx) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm* tm_now = std::localtime(&time_t_now);

    char buffer[128];
    std::strftime(buffer, sizeof(buffer), "%A, %B %d, %Y at %H:%M:%S", tm_now);

    ctx.send(fmt::format("Current server time: {}", buffer));

    return CommandResult::Success;
}

Result<CommandResult> cmd_clear(const CommandContext &ctx) {
    // Send ANSI escape sequence to clear screen and move cursor to home
    ctx.send("\033[2J\033[H");
    return CommandResult::Success;
}

Result<CommandResult> cmd_color(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can set color preferences.");
        return CommandResult::InvalidState;
    }

    static const std::vector<std::pair<std::string, ColorLevel>> color_levels = {
        {"off", ColorLevel::Off},
        {"sparse", ColorLevel::Sparse},
        {"normal", ColorLevel::Normal},
        {"complete", ColorLevel::Complete}
    };

    auto level_to_name = [](ColorLevel level) -> std::string_view {
        switch (level) {
            case ColorLevel::Off: return "off";
            case ColorLevel::Sparse: return "sparse";
            case ColorLevel::Normal: return "normal";
            case ColorLevel::Complete: return "complete";
            default: return "normal";
        }
    };

    if (ctx.arg_count() == 0) {
        // Show current color level
        ctx.send(fmt::format("Your current color level is: {}", level_to_name(player->color_level())));
        ctx.send("Usage: color <off|sparse|normal|complete>");
        return CommandResult::Success;
    }

    std::string level_arg{ctx.arg(0)};
    std::transform(level_arg.begin(), level_arg.end(), level_arg.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    for (const auto& [name, level] : color_levels) {
        if (name.starts_with(level_arg)) {
            player->set_color_level(level);
            ctx.send(fmt::format("Your color level is now: {}", name));
            return CommandResult::Success;
        }
    }

    ctx.send_error("Invalid color level.");
    ctx.send("Usage: color <off|sparse|normal|complete>");
    return CommandResult::InvalidSyntax;
}

Result<CommandResult> cmd_display(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can set prompt preferences.");
        return CommandResult::InvalidState;
    }

    // Preset prompts
    static const std::vector<std::pair<std::string, std::string>> preset_prompts = {
        {"minimal", "<&h/&H hp &m/&M mv>"},
        {"standard", "<&h/&Hhp &m/&Mmv &E/&eexp>"},
        {"combat", "<&h/&Hhp &m/&Mmv $T> "},
        {"full", "<&h/&H hp &m/&M mv &E/&e exp &g gold $T>"},
        {"spellcaster", "<&h/&Hhp &m/&Mmv &l/&Lmana $T>"},
    };

    if (ctx.arg_count() == 0 || !std::isdigit(ctx.arg(0)[0])) {
        ctx.send("Available pre-set prompts:");
        int i = 0;
        for (const auto& [name, prompt] : preset_prompts) {
            ctx.send(fmt::format("{:2d}. {:<12} {}", i++, name, prompt));
        }
        ctx.send(fmt::format("\nYour current prompt: {}", player->prompt()));
        ctx.send("Usage: display <number>");
        return CommandResult::Success;
    }

    int index = std::stoi(std::string{ctx.arg(0)});
    if (index < 0 || index >= static_cast<int>(preset_prompts.size())) {
        ctx.send_error(fmt::format("Invalid prompt number. Range is 0-{}.", preset_prompts.size() - 1));
        return CommandResult::InvalidSyntax;
    }

    player->set_prompt(preset_prompts[index].second);
    ctx.send(fmt::format("Your prompt is now set to the '{}' preset.", preset_prompts[index].first));

    return CommandResult::Success;
}

Result<CommandResult> cmd_alias(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use aliases.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        // List all aliases
        const auto& aliases = player->get_aliases();
        if (aliases.empty()) {
            ctx.send("You have no aliases defined.");
        } else {
            ctx.send("Currently defined aliases:");
            for (const auto& [name, command] : aliases) {
                ctx.send(fmt::format("  {} -> {}", name, command));
            }
        }
        ctx.send("");
        ctx.send("Usage: alias <name> <command>");
        ctx.send("       alias <name>       - Delete alias");
        return CommandResult::Success;
    }

    std::string_view alias_name = ctx.arg(0);

    if (alias_name == "alias") {
        ctx.send_error("You can't alias 'alias'.");
        return CommandResult::InvalidSyntax;
    }

    if (ctx.arg_count() == 1) {
        // Delete alias
        if (player->has_alias(alias_name)) {
            player->remove_alias(alias_name);
            ctx.send(fmt::format("Alias '{}' deleted.", alias_name));
        } else {
            ctx.send(fmt::format("No alias '{}' defined.", alias_name));
        }
        return CommandResult::Success;
    }

    // Create/update alias
    // The replacement is everything after the alias name
    std::string replacement;
    for (size_t i = 1; i < ctx.arg_count(); ++i) {
        if (!replacement.empty()) replacement += " ";
        replacement += ctx.arg(i);
    }

    player->set_alias(alias_name, replacement);
    ctx.send(fmt::format("Alias '{}' set to '{}'.", alias_name, replacement));

    return CommandResult::Success;
}

Result<CommandResult> cmd_ignore(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use the ignore command.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        // List ignored players
        const auto& ignored = player->get_ignored_players();
        if (ignored.empty()) {
            ctx.send("You are not ignoring anyone.");
        } else {
            ctx.send("Currently ignored players:");
            for (const auto& name : ignored) {
                ctx.send(fmt::format("  {}", name));
            }
        }
        ctx.send("");
        ctx.send("Usage: ignore <player>  - Toggle ignoring a player");
        ctx.send("       ignore off       - Stop ignoring everyone");
        return CommandResult::Success;
    }

    if (ctx.arg(0) == "off") {
        player->clear_ignored_players();
        ctx.send("You feel sociable and stop ignoring anyone.");
        return CommandResult::Success;
    }

    std::string_view target_name = ctx.arg(0);

    // Check if already ignoring - toggle behavior
    if (player->is_ignoring(target_name)) {
        player->unignore_player(target_name);
        ctx.send(fmt::format("You stop ignoring {}.", target_name));
        return CommandResult::Success;
    }

    // Try to find the target player in the game
    auto target = ctx.find_actor_global(target_name);
    if (target) {
        // Can't ignore NPCs
        if (!std::dynamic_pointer_cast<Player>(target)) {
            ctx.send_error("You can only ignore players.");
            return CommandResult::InvalidTarget;
        }
        target_name = target->name();  // Use proper capitalization
    }
    // Allow ignoring by name even if player is not online

    player->ignore_player(target_name);
    ctx.send(fmt::format("You now ignore {}.", target_name));

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands()
        .command("quit", cmd_quit)
        .alias("ex")
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("save", cmd_save)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Save your character")
        .build();

    Commands()
        .command("rent", cmd_rent)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Rent a room at an inn to save and quit")
        .build();

    Commands()
        .command("camp", cmd_camp)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Make camp to save your progress")
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

    // Feedback commands
    Commands()
        .command("bug", cmd_bug)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Report a bug to the immortals")
        .usage("bug <description>")
        .build();

    Commands()
        .command("idea", cmd_idea)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Submit an idea to the immortals")
        .usage("idea <description>")
        .build();

    Commands()
        .command("typo", cmd_typo)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Report a typo to the immortals")
        .usage("typo <description>")
        .build();

    // Utility commands
    Commands()
        .command("date", cmd_date)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Show current server date and time")
        .build();

    Commands()
        .command("clear", cmd_clear)
        .alias("cls")
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Clear your screen")
        .build();

    Commands()
        .command("color", cmd_color)
        .alias("colour")
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Set your color preference level")
        .usage("color [off|sparse|normal|complete]")
        .build();

    Commands()
        .command("display", cmd_display)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Select a preset prompt")
        .usage("display [number]")
        .build();

    Commands()
        .command("alias", cmd_alias)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Create, modify, or delete command aliases")
        .usage("alias [name] [command]")
        .build();

    Commands()
        .command("ignore", cmd_ignore)
        .category("System")
        .privilege(PrivilegeLevel::Player)
        .description("Ignore a player's communications")
        .usage("ignore [player|off]")
        .build();

    return Success();
}

} // namespace SystemCommands