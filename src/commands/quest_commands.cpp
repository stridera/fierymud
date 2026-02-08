#include "quest_commands.hpp"

#include "core/actor.hpp"
#include "core/logging.hpp"
#include "core/player.hpp"
#include "commands/command_system.hpp"
#include "database/connection_pool.hpp"
#include "database/quest_queries.hpp"
#include "quests/quest_manager.hpp"
#include "server/world_server.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>

namespace QuestCommands {

namespace {

// Helper: Format a quest status for display
std::string_view format_quest_status(QuestQueries::QuestStatus status) {
    switch (status) {
        case QuestQueries::QuestStatus::AVAILABLE: return "<white>Available</>";
        case QuestQueries::QuestStatus::IN_PROGRESS: return "<yellow>In Progress</>";
        case QuestQueries::QuestStatus::COMPLETED: return "<green>Completed</>";
        case QuestQueries::QuestStatus::FAILED: return "<red>Failed</>";
        case QuestQueries::QuestStatus::ABANDONED: return "<gray>Abandoned</>";
    }
    return "Unknown";
}

// Helper: Format objective type for display
std::string_view format_objective_type(QuestQueries::QuestObjectiveType type) {
    switch (type) {
        case QuestQueries::QuestObjectiveType::KILL_MOB: return "Kill";
        case QuestQueries::QuestObjectiveType::COLLECT_ITEM: return "Collect";
        case QuestQueries::QuestObjectiveType::DELIVER_ITEM: return "Deliver";
        case QuestQueries::QuestObjectiveType::VISIT_ROOM: return "Visit";
        case QuestQueries::QuestObjectiveType::TALK_TO_NPC: return "Talk to";
        case QuestQueries::QuestObjectiveType::USE_SKILL: return "Use skill";
        case QuestQueries::QuestObjectiveType::CUSTOM_LUA: return "Special";
    }
    return "Unknown";
}

// Helper: Format trigger type for display
std::string_view format_trigger_type(QuestQueries::QuestTriggerType type) {
    switch (type) {
        case QuestQueries::QuestTriggerType::MOB: return "Talk to NPC";
        case QuestQueries::QuestTriggerType::LEVEL: return "Level up";
        case QuestQueries::QuestTriggerType::ITEM: return "Item pickup";
        case QuestQueries::QuestTriggerType::ROOM: return "Enter room";
        case QuestQueries::QuestTriggerType::SKILL: return "Use skill";
        case QuestQueries::QuestTriggerType::EVENT: return "Game event";
        case QuestQueries::QuestTriggerType::AUTO: return "Automatic";
        case QuestQueries::QuestTriggerType::MANUAL: return "Manual";
    }
    return "Unknown";
}

// Helper: Get character_id from actor (only works for Players)
std::string get_character_id(const Actor* actor) {
    if (!actor) return "";
    const auto* player = dynamic_cast<const Player*>(actor);
    if (!player) return "";
    return std::string(player->database_id());
}

// Helper: Find online player by name prefix
std::shared_ptr<Player> find_online_player(std::string_view name) {
    if (name.empty()) return nullptr;

    auto* world_server = WorldServer::instance();
    if (!world_server) return nullptr;

    // Convert search name to lowercase
    std::string search_name{name};
    std::transform(search_name.begin(), search_name.end(), search_name.begin(), ::tolower);

    auto players = world_server->get_online_players();
    for (const auto& player : players) {
        if (!player) continue;

        std::string player_name{player->name()};
        std::transform(player_name.begin(), player_name.end(), player_name.begin(), ::tolower);

        // Match by prefix
        if (player_name.starts_with(search_name)) {
            return player;
        }
    }
    return nullptr;
}

// Helper: Parse quest identifier - supports "zone:id" or just "id" (uses current zone)
bool parse_quest_id(std::string_view arg, int default_zone, int& zone_id, int& quest_id) {
    auto colon_pos = arg.find(':');
    if (colon_pos != std::string_view::npos) {
        try {
            zone_id = std::stoi(std::string(arg.substr(0, colon_pos)));
            quest_id = std::stoi(std::string(arg.substr(colon_pos + 1)));
            return true;
        } catch (...) {
            return false;
        }
    } else {
        // Try as just quest ID with default zone
        try {
            zone_id = default_zone;
            quest_id = std::stoi(std::string(arg));
            return true;
        } catch (...) {
            return false;
        }
    }
}

// Helper: Find quest by name in available quests
const QuestQueries::QuestData* find_quest_by_name(
    std::string_view name,
    const std::vector<const QuestQueries::QuestData*>& quests) {

    std::string search_name{name};
    std::transform(search_name.begin(), search_name.end(), search_name.begin(), ::tolower);

    for (const auto* quest : quests) {
        if (!quest) continue;

        std::string quest_name = quest->name;
        std::transform(quest_name.begin(), quest_name.end(), quest_name.begin(), ::tolower);

        // Match by prefix
        if (quest_name.starts_with(search_name)) {
            return quest;
        }
    }
    return nullptr;
}

} // anonymous namespace

// ============================================================================
// Command Registration
// ============================================================================

Result<void> register_commands() {
    // Player quest commands
    Commands()
        .command("quest", cmd_quest)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("quests", cmd_quests)
        .alias("questlog")
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("questinfo", cmd_questinfo)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    // God/Admin commands
    Commands()
        .command("qstat", cmd_qstat)
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("qgive", cmd_qgive)
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("qcomplete", cmd_qcomplete)
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("qlist", cmd_qlist)
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("qload", cmd_qload)
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("qreload", cmd_qreload)
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .build();

    spdlog::debug("Quest commands registered");
    return Success();
}

// ============================================================================
// Player Commands
// ============================================================================

Result<CommandResult> cmd_quest(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor in context"));
    }

    std::string character_id = get_character_id(ctx.actor.get());
    if (character_id.empty()) {
        ctx.send("Only players can have quests.\r\n");
        return CommandResult::Success;
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Usage: quest accept <name|zone:id> | quest abandon <name|zone:id>\r\n");
        return CommandResult::Success;
    }

    std::string subcommand{ctx.arg(0)};
    std::transform(subcommand.begin(), subcommand.end(), subcommand.begin(), ::tolower);

    auto& manager = FieryMUD::QuestManager::instance();

    if (subcommand == "accept") {
        if (ctx.arg_count() < 2) {
            ctx.send("Accept which quest?\r\n");
            return CommandResult::Success;
        }

        std::string quest_arg{ctx.arg(1)};
        int zone_id = 0, quest_id = 0;

        // First try to parse as zone:id format
        if (parse_quest_id(quest_arg, 0, zone_id, quest_id) && zone_id > 0) {
            // Start quest by ID
            auto result = manager.start_quest(character_id, zone_id, quest_id);
            if (result) {
                const auto* quest = manager.get_quest(zone_id, quest_id);
                if (quest) {
                    ctx.send(fmt::format("You have accepted the quest: <white>{}</>\r\n", quest->name));
                } else {
                    ctx.send(fmt::format("You have accepted quest {}:{}.\r\n", zone_id, quest_id));
                }
            } else {
                ctx.send(fmt::format("<red>Failed to accept quest:</> {}\r\n", result.error().message));
            }
        } else {
            // Try to find by quest name in available quests
            int level = ctx.actor->stats().level;
            auto available = manager.get_available_quests(character_id, level);
            const auto* quest = find_quest_by_name(quest_arg, available);

            if (quest) {
                auto result = manager.start_quest(character_id, quest->id.zone_id(), quest->id.local_id());
                if (result) {
                    ctx.send(fmt::format("You have accepted the quest: <white>{}</>\r\n", quest->name));
                } else {
                    ctx.send(fmt::format("<red>Failed to accept quest:</> {}\r\n", result.error().message));
                }
            } else {
                ctx.send(fmt::format("No available quest matching '{}' found.\r\n", quest_arg));
            }
        }
        return CommandResult::Success;

    } else if (subcommand == "abandon") {
        if (ctx.arg_count() < 2) {
            ctx.send("Abandon which quest?\r\n");
            return CommandResult::Success;
        }

        std::string quest_arg{ctx.arg(1)};
        int zone_id = 0, quest_id = 0;

        // First try to parse as zone:id format
        if (parse_quest_id(quest_arg, 0, zone_id, quest_id) && zone_id > 0) {
            auto result = manager.abandon_quest(character_id, zone_id, quest_id);
            if (result) {
                const auto* quest = manager.get_quest(zone_id, quest_id);
                if (quest) {
                    ctx.send(fmt::format("You have abandoned the quest: <white>{}</>\r\n", quest->name));
                } else {
                    ctx.send(fmt::format("You have abandoned quest {}:{}.\r\n", zone_id, quest_id));
                }
            } else {
                ctx.send(fmt::format("<red>Failed to abandon quest:</> {}\r\n", result.error().message));
            }
        } else {
            // Try to find by name in active quests
            auto active_result = manager.get_active_quests(character_id);
            if (active_result) {
                const QuestQueries::QuestData* found_quest = nullptr;

                std::string search = quest_arg;
                std::transform(search.begin(), search.end(), search.begin(), ::tolower);

                for (const auto& progress : *active_result) {
                    const auto* quest = manager.get_quest(progress.quest_id);
                    if (quest) {
                        std::string qname = quest->name;
                        std::transform(qname.begin(), qname.end(), qname.begin(), ::tolower);
                        if (qname.starts_with(search)) {
                            found_quest = quest;
                            break;
                        }
                    }
                }

                if (found_quest) {
                    auto result = manager.abandon_quest(character_id,
                        found_quest->id.zone_id(), found_quest->id.local_id());
                    if (result) {
                        ctx.send(fmt::format("You have abandoned the quest: <white>{}</>\r\n", found_quest->name));
                    } else {
                        ctx.send(fmt::format("<red>Failed to abandon quest:</> {}\r\n", result.error().message));
                    }
                } else {
                    ctx.send(fmt::format("No active quest matching '{}' found.\r\n", quest_arg));
                }
            } else {
                ctx.send("You have no active quests.\r\n");
            }
        }
        return CommandResult::Success;

    } else {
        ctx.send("Unknown quest action. Use: quest accept <name|zone:id> | quest abandon <name|zone:id>\r\n");
        return CommandResult::Success;
    }
}

Result<CommandResult> cmd_quests(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor in context"));
    }

    std::string character_id = get_character_id(ctx.actor.get());
    if (character_id.empty()) {
        ctx.send("Only players can have quests.\r\n");
        return CommandResult::Success;
    }

    bool show_completed = false;
    bool show_active = true;

    if (ctx.arg_count() > 0) {
        std::string filter{ctx.arg(0)};
        std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

        if (filter == "completed") {
            show_completed = true;
            show_active = false;
        } else if (filter == "all") {
            show_completed = true;
            show_active = true;
        }
    }

    std::string output;
    output += "<white>=== Quest Log ===</>\r\n\r\n";

    auto& manager = FieryMUD::QuestManager::instance();

    if (show_active) {
        auto active_result = manager.get_active_quests(character_id);
        if (active_result && !active_result->empty()) {
            output += "<yellow>Active Quests:</>\r\n";
            for (const auto& progress : *active_result) {
                const auto* quest = manager.get_quest(progress.quest_id);
                if (quest) {
                    output += fmt::format("  <cyan>{}:{}</> - <white>{}</>\r\n",
                        progress.quest_id.zone_id(),
                        progress.quest_id.local_id(),
                        quest->name);

                    // Show current phase if available
                    if (progress.current_phase_id) {
                        for (const auto& phase : quest->phases) {
                            if (phase.id == *progress.current_phase_id) {
                                output += fmt::format("    Phase: {}\r\n", phase.name);
                                break;
                            }
                        }
                    }
                }
            }
            output += "\r\n";
        } else if (show_active && !show_completed) {
            output += "You have no active quests.\r\n\r\n";
        }
    }

    if (show_completed) {
        auto completed_result = manager.get_completed_quests(character_id);
        if (completed_result && !completed_result->empty()) {
            output += "<green>Completed Quests:</>\r\n";
            for (const auto& progress : *completed_result) {
                const auto* quest = manager.get_quest(progress.quest_id);
                if (quest) {
                    output += fmt::format("  <cyan>{}:{}</> - <white>{}</>",
                        progress.quest_id.zone_id(),
                        progress.quest_id.local_id(),
                        quest->name);

                    if (progress.completion_count > 1) {
                        output += fmt::format(" (x{})", progress.completion_count);
                    }
                    output += "\r\n";
                }
            }
        } else if (show_completed && !show_active) {
            output += "You have not completed any quests.\r\n";
        }
    }

    ctx.send(output);
    return CommandResult::Success;
}

Result<CommandResult> cmd_questinfo(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor in context"));
    }

    if (ctx.arg_count() == 0) {
        ctx.send("View info on which quest?\r\n");
        return CommandResult::Success;
    }

    // TODO: Parse quest name/id
    // For now, try to parse as zone:id format
    std::string quest_arg{ctx.arg(0)};
    int zone_id = 0, quest_id = 0;

    auto colon_pos = quest_arg.find(':');
    if (colon_pos != std::string::npos) {
        try {
            zone_id = std::stoi(quest_arg.substr(0, colon_pos));
            quest_id = std::stoi(quest_arg.substr(colon_pos + 1));
        } catch (...) {
            ctx.send("Invalid quest format. Use zone:id (e.g., 30:1)\r\n");
            return CommandResult::Success;
        }
    } else {
        ctx.send("Invalid quest format. Use zone:id (e.g., 30:1)\r\n");
        return CommandResult::Success;
    }

    auto& manager = FieryMUD::QuestManager::instance();
    const auto* quest = manager.get_quest(zone_id, quest_id);

    if (!quest) {
        ctx.send("Quest not found.\r\n");
        return CommandResult::Success;
    }

    std::string output;
    output += fmt::format("<white>=== {} ===</>\r\n\r\n", quest->name);
    output += fmt::format("<cyan>ID:</> {}:{}\r\n", zone_id, quest_id);
    output += fmt::format("<cyan>Level:</> {}-{}\r\n", quest->min_level, quest->max_level);
    output += fmt::format("<cyan>Repeatable:</> {}\r\n", quest->repeatable ? "Yes" : "No");
    output += fmt::format("<cyan>Trigger:</> {}\r\n", format_trigger_type(quest->trigger_type));

    if (!quest->description.empty()) {
        output += fmt::format("\r\n<cyan>Description:</>\r\n{}\r\n", quest->description);
    }

    if (!quest->phases.empty()) {
        output += "\r\n<cyan>Phases:</>\r\n";
        for (const auto& phase : quest->phases) {
            output += fmt::format("  <white>{}. {}</>\r\n", phase.order, phase.name);
            for (const auto& obj : phase.objectives) {
                output += fmt::format("    - [{}] {}\r\n",
                    format_objective_type(obj.type),
                    obj.player_description);
            }
        }
    }

    if (!quest->rewards.empty()) {
        output += "\r\n<cyan>Rewards:</>\r\n";
        for (const auto& reward : quest->rewards) {
            switch (reward.type) {
                case QuestQueries::QuestRewardType::EXPERIENCE:
                    output += fmt::format("  {} experience\r\n", reward.amount.value_or(0));
                    break;
                case QuestQueries::QuestRewardType::GOLD:
                    output += fmt::format("  {} gold\r\n", reward.amount.value_or(0));
                    break;
                case QuestQueries::QuestRewardType::ITEM:
                    output += fmt::format("  Item: {}:{}\r\n",
                        reward.object ? reward.object->zone_id() : 0,
                        reward.object ? reward.object->local_id() : 0);
                    break;
                case QuestQueries::QuestRewardType::ABILITY:
                    output += fmt::format("  Ability: {}\r\n", reward.ability_id.value_or(0));
                    break;
            }
        }
    }

    ctx.send(output);
    return CommandResult::Success;
}

// ============================================================================
// God/Admin Commands
// ============================================================================

Result<CommandResult> cmd_qstat(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor in context"));
    }

    if (ctx.arg_count() == 0) {
        ctx.send("View quest status for which player?\r\n");
        return CommandResult::Success;
    }

    std::string target_name{ctx.arg(0)};
    auto target = find_online_player(target_name);

    if (!target) {
        ctx.send(fmt::format("Player '{}' not found online.\r\n", target_name));
        return CommandResult::Success;
    }

    std::string character_id{target->database_id()};
    if (character_id.empty()) {
        ctx.send(fmt::format("Player '{}' has no character record.\r\n", target->name()));
        return CommandResult::Success;
    }

    auto& manager = FieryMUD::QuestManager::instance();

    std::string output;
    output += fmt::format("<white>=== Quest Status for {} ===</>\r\n\r\n", target->name());

    // Show active quests
    auto active_result = manager.get_active_quests(character_id);
    if (active_result && !active_result->empty()) {
        output += "<yellow>Active Quests:</>\r\n";
        for (const auto& progress : *active_result) {
            const auto* quest = manager.get_quest(progress.quest_id);
            if (quest) {
                output += fmt::format("  <cyan>{}:{}</> - <white>{}</> ({})\r\n",
                    progress.quest_id.zone_id(),
                    progress.quest_id.local_id(),
                    quest->name,
                    format_quest_status(progress.status));

                // Show objective progress
                if (progress.current_phase_id) {
                    for (const auto& obj_prog : progress.objective_progress) {
                        if (obj_prog.phase_id == *progress.current_phase_id) {
                            // Find matching objective
                            for (const auto& phase : quest->phases) {
                                if (phase.id == obj_prog.phase_id) {
                                    for (const auto& obj : phase.objectives) {
                                        if (obj.id == obj_prog.objective_id) {
                                            std::string status = obj_prog.completed ? "<green>[DONE]</>" :
                                                fmt::format("[{}/{}]", obj_prog.current_count, obj.required_count);
                                            output += fmt::format("    {} {}\r\n", status, obj.player_description);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        output += "\r\n";
    } else {
        output += "No active quests.\r\n\r\n";
    }

    // Show completed quests count
    auto completed_result = manager.get_completed_quests(character_id);
    if (completed_result && !completed_result->empty()) {
        output += fmt::format("<green>Completed Quests:</> {} total\r\n", completed_result->size());
    }

    ctx.send(output);
    return CommandResult::Success;
}

Result<CommandResult> cmd_qgive(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor in context"));
    }

    if (ctx.arg_count() < 2) {
        ctx.send("Usage: qgive <player> <zone:quest>\r\n");
        return CommandResult::Success;
    }

    std::string target_name{ctx.arg(0)};
    auto target = find_online_player(target_name);

    if (!target) {
        ctx.send(fmt::format("Player '{}' not found online.\r\n", target_name));
        return CommandResult::Success;
    }

    std::string character_id{target->database_id()};
    if (character_id.empty()) {
        ctx.send(fmt::format("Player '{}' has no character record.\r\n", target->name()));
        return CommandResult::Success;
    }

    std::string quest_arg{ctx.arg(1)};
    int zone_id = 0, quest_id = 0;

    if (!parse_quest_id(quest_arg, 0, zone_id, quest_id) || zone_id <= 0) {
        ctx.send("Invalid quest format. Use zone:id (e.g., 30:1)\r\n");
        return CommandResult::Success;
    }

    auto& manager = FieryMUD::QuestManager::instance();
    const auto* quest = manager.get_quest(zone_id, quest_id);

    if (!quest) {
        ctx.send(fmt::format("Quest {}:{} not found in cache. Try loading the zone first.\r\n",
            zone_id, quest_id));
        return CommandResult::Success;
    }

    auto result = manager.start_quest(character_id, zone_id, quest_id);
    if (result) {
        ctx.send(fmt::format("<green>Gave quest '<white>{}<green>' to {}.</>\r\n", quest->name, target->name()));

        // Notify the target player
        target->send_message(fmt::format("\r\n<yellow>You have been given the quest: <white>{}</>\r\n", quest->name));
    } else {
        ctx.send(fmt::format("<red>Failed to give quest:</> {}\r\n", result.error().message));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_qcomplete(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor in context"));
    }

    if (ctx.arg_count() < 2) {
        ctx.send("Usage: qcomplete <player> <zone:quest>\r\n");
        return CommandResult::Success;
    }

    std::string target_name{ctx.arg(0)};
    auto target = find_online_player(target_name);

    if (!target) {
        ctx.send(fmt::format("Player '{}' not found online.\r\n", target_name));
        return CommandResult::Success;
    }

    std::string character_id{target->database_id()};
    if (character_id.empty()) {
        ctx.send(fmt::format("Player '{}' has no character record.\r\n", target->name()));
        return CommandResult::Success;
    }

    std::string quest_arg{ctx.arg(1)};
    int zone_id = 0, quest_id = 0;

    if (!parse_quest_id(quest_arg, 0, zone_id, quest_id) || zone_id <= 0) {
        ctx.send("Invalid quest format. Use zone:id (e.g., 30:1)\r\n");
        return CommandResult::Success;
    }

    auto& manager = FieryMUD::QuestManager::instance();
    const auto* quest = manager.get_quest(zone_id, quest_id);

    // Complete the quest
    auto result = manager.complete_quest(character_id, zone_id, quest_id);
    if (result) {
        std::string quest_name = quest ? quest->name : fmt::format("{}:{}", zone_id, quest_id);
        ctx.send(fmt::format("<green>Completed quest '<white>{}<green>' for {}.</>\r\n", quest_name, target->name()));

        // Notify the target player
        target->send_message(fmt::format("\r\n<green>*** Quest Completed: <white>{}<green> ***</>\r\n", quest_name));

        // Grant rewards if quest data is available
        if (quest && !quest->rewards.empty()) {
            for (const auto& reward : quest->rewards) {
                switch (reward.type) {
                    case QuestQueries::QuestRewardType::EXPERIENCE:
                        if (reward.amount) {
                            target->gain_experience(*reward.amount);
                            target->send_message(fmt::format("You gain {} experience!\r\n", *reward.amount));
                        }
                        break;
                    case QuestQueries::QuestRewardType::GOLD:
                        if (reward.amount) {
                            target->stats().gold += *reward.amount;
                            target->send_message(fmt::format("You receive {} gold coins!\r\n", *reward.amount));
                        }
                        break;
                    case QuestQueries::QuestRewardType::ITEM:
                        // TODO: Load item from database and give to player
                        if (reward.object) {
                            target->send_message(fmt::format("You would receive item {}:{} (item loading not implemented)\r\n",
                                reward.object->zone_id(), reward.object->local_id()));
                        }
                        break;
                    case QuestQueries::QuestRewardType::ABILITY:
                        // TODO: Teach ability to player
                        if (reward.ability_id) {
                            target->send_message(fmt::format("You would learn ability {} (ability teaching not implemented)\r\n",
                                *reward.ability_id));
                        }
                        break;
                }
            }
        }
    } else {
        ctx.send(fmt::format("<red>Failed to complete quest:</> {}\r\n", result.error().message));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_qlist(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor in context"));
    }

    auto& manager = FieryMUD::QuestManager::instance();

    std::string output;

    if (ctx.arg_count() > 0) {
        // List quests for specific zone
        int zone_id = 0;
        try {
            zone_id = std::stoi(std::string{ctx.arg(0)});
        } catch (...) {
            ctx.send("Invalid zone ID.\r\n");
            return CommandResult::Success;
        }

        auto quests = manager.get_zone_quests(zone_id);
        if (quests.empty()) {
            ctx.send(fmt::format("No quests loaded for zone {}.\r\n", zone_id));
            return CommandResult::Success;
        }

        output += fmt::format("<white>=== Quests in Zone {} ({} total) ===</>\r\n\r\n", zone_id, quests.size());
        for (const auto* quest : quests) {
            output += fmt::format("  <cyan>{}:{}</> - <white>{}</> [Lvl {}-{}]{}\r\n",
                quest->id.zone_id(), quest->id.local_id(),
                quest->name,
                quest->min_level, quest->max_level,
                quest->hidden ? " <gray>(hidden)</>" : "");
        }
    } else {
        // Show summary of all loaded quests
        const auto& stats = manager.stats();
        output += fmt::format("<white>=== Quest System Summary ===</>\r\n\r\n");
        output += fmt::format("  Total quests loaded: {}\r\n", manager.quest_count());
        output += fmt::format("  Quests started:      {}\r\n", stats.quests_started);
        output += fmt::format("  Quests completed:    {}\r\n", stats.quests_completed);
        output += fmt::format("  Quests abandoned:    {}\r\n", stats.quests_abandoned);
        output += fmt::format("  Trigger activations: {}\r\n", stats.trigger_activations);
        output += "\r\nUse <cyan>qlist <zone></> to list quests in a specific zone.\r\n";
    }

    ctx.send(output);
    return CommandResult::Success;
}

Result<CommandResult> cmd_qload(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor in context"));
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Usage: qload <zone_id>\r\n");
        return CommandResult::Success;
    }

    int zone_id = 0;
    try {
        zone_id = std::stoi(std::string{ctx.arg(0)});
    } catch (...) {
        ctx.send("Invalid zone ID.\r\n");
        return CommandResult::Success;
    }

    auto& manager = FieryMUD::QuestManager::instance();
    auto result = manager.load_zone_quests(zone_id);

    if (result) {
        ctx.send(fmt::format("<green>Loaded {} quests for zone {}.</>\r\n", *result, zone_id));
    } else {
        ctx.send(fmt::format("<red>Failed to load quests:</> {}\r\n", result.error()));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_qreload(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor in context"));
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Usage: qreload <zone_id>\r\n");
        return CommandResult::Success;
    }

    int zone_id = 0;
    try {
        zone_id = std::stoi(std::string{ctx.arg(0)});
    } catch (...) {
        ctx.send("Invalid zone ID.\r\n");
        return CommandResult::Success;
    }

    auto& manager = FieryMUD::QuestManager::instance();
    auto result = manager.reload_zone_quests(zone_id);

    if (result) {
        ctx.send(fmt::format("<green>Reloaded {} quests for zone {}.</>\r\n", *result, zone_id));
    } else {
        ctx.send(fmt::format("<red>Failed to reload quests:</> {}\r\n", result.error()));
    }

    return CommandResult::Success;
}

} // namespace QuestCommands
