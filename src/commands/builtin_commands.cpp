#include "builtin_commands.hpp"

#include "core/money.hpp"
#include "scripting/trigger_manager.hpp"

// Command module headers for registration
#include "account_commands.hpp"
#include "admin_commands.hpp"
#include "character_commands.hpp"
#include "combat_commands.hpp"
#include "skill_commands.hpp"
#include "communication_commands.hpp"
#include "economy_commands.hpp"
#include "postmaster_commands.hpp"
#include "group_commands.hpp"
#include "information_commands.hpp"
#include "magic_commands.hpp"
#include "movement_commands.hpp"
#include "object_commands.hpp"
#include "position_commands.hpp"
#include "quest_commands.hpp"
#include "social_commands.hpp"
#include "system_commands.hpp"

// Core dependencies used by helper functions
#include "core/ability_executor.hpp"
#include "core/actor.hpp"
#include "core/mobile.hpp"
#include "core/player.hpp"
#include "core/logging.hpp"
#include "core/object.hpp"
#include "text/string_utils.hpp"
#include "text/text_format.hpp"
#include "text/rich_text.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"

#include <algorithm>
#include <set>
#include <sstream>
#include <magic_enum/magic_enum.hpp>

namespace BuiltinCommands {

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_all_commands() {
    Log::info("Registering built-in commands...");

    // Register commands from each module
    if (auto result = InformationCommands::register_commands(); !result) {
        Log::error("Failed to register information commands: {}", result.error().message);
        return result;
    }

    if (auto result = CommunicationCommands::register_commands(); !result) {
        Log::error("Failed to register communication commands: {}", result.error().message);
        return result;
    }

    if (auto result = MovementCommands::register_commands(); !result) {
        Log::error("Failed to register movement commands: {}", result.error().message);
        return result;
    }

    if (auto result = PositionCommands::register_commands(); !result) {
        Log::error("Failed to register position commands: {}", result.error().message);
        return result;
    }

    if (auto result = CombatCommands::register_commands(); !result) {
        Log::error("Failed to register combat commands: {}", result.error().message);
        return result;
    }

    if (auto result = EconomyCommands::register_commands(); !result) {
        Log::error("Failed to register economy commands: {}", result.error().message);
        return result;
    }

    if (auto result = PostmasterCommands::register_commands(); !result) {
        Log::error("Failed to register postmaster commands: {}", result.error().message);
        return result;
    }

    if (auto result = GroupCommands::register_commands(); !result) {
        Log::error("Failed to register group commands: {}", result.error().message);
        return result;
    }

    if (auto result = CharacterCommands::register_commands(); !result) {
        Log::error("Failed to register character commands: {}", result.error().message);
        return result;
    }

    if (auto result = SkillCommands::register_commands(); !result) {
        Log::error("Failed to register skill commands: {}", result.error().message);
        return result;
    }

    if (auto result = MagicCommands::register_commands(); !result) {
        Log::error("Failed to register magic commands: {}", result.error().message);
        return result;
    }

    if (auto result = ObjectCommands::register_commands(); !result) {
        Log::error("Failed to register object commands: {}", result.error().message);
        return result;
    }

    if (auto result = SystemCommands::register_commands(); !result) {
        Log::error("Failed to register system commands: {}", result.error().message);
        return result;
    }

    if (auto result = AdminCommands::register_commands(); !result) {
        Log::error("Failed to register admin commands: {}", result.error().message);
        return result;
    }

    if (auto result = AccountCommands::register_commands(); !result) {
        Log::error("Failed to register account commands: {}", result.error().message);
        return result;
    }

    if (auto result = QuestCommands::register_commands(); !result) {
        Log::error("Failed to register quest commands: {}", result.error().message);
        return result;
    }

    // Social commands are loaded from database
    if (auto result = SocialCommands::initialize(); !result) {
        Log::error("Failed to initialize social commands: {}", result.error().message);
        return std::unexpected(result.error());
    }
    Log::info("Loaded {} social commands", SocialCommands::social_count());

    Log::info("Built-in commands registered successfully.");
    return Success();
}

Result<void> unregister_all_commands() {
    Log::info("Unregistering built-in commands...");

    auto &cmd_system = CommandSystem::instance();

    // Information Commands
    const std::vector<std::string> commands_to_unregister = {
        "look", "examine", "who", "where", "inventory", "equipment", "score", "time", "weather", "stat",
        // Communication Commands
        "say", "tell", "emote", "whisper", "shout", "gossip",
        // Movement Commands
        "north", "south", "east", "west", "up", "down", "exits",
        // Combat Commands
        "kill", "hit", "cast", "flee", "release",
        // Object Commands
        "get", "drop", "put", "give", "wear", "wield", "remove", "light", "eat", "drink", "open", "close", "lock",
        "unlock", "list", "buy", "sell",
        // System Commands
        "quit", "save", "help", "commands", "richtest", "clientinfo", "prompt",
        // Social Commands
        "smile", "nod", "wave", "bow", "laugh",
        // Admin Commands
        "shutdown", "goto", "teleport", "summon", "setweather", "reloadzone", "savezone", "reloadallzones", "filewatch",
        "dumpworld"};

    for (const auto &cmd : commands_to_unregister) {
        cmd_system.unregister_command(cmd);
    }

    Log::info("Unregistered {} built-in commands", commands_to_unregister.size());
    return Success();
}

// =============================================================================
// Helper Functions Namespace
// =============================================================================

namespace Helpers {

// Forward declaration
std::string get_equipment_slot_display_name(EquipSlot slot);

std::string format_room_description(std::shared_ptr<Room> room, std::shared_ptr<Actor> viewer) {
    if (!room) {
        return "You are in the void.";
    }

    std::ostringstream desc;
    desc << fmt::format("<green>{}</>\n", room->name());
    desc << fmt::format("{}\n", room->description());

    // Add exits
    std::string exits = format_exits(room);
    if (!exits.empty()) {
        desc << fmt::format("\n{}\n", exits);
    }

    // Add objects in room
    auto objects = room->contents().objects;
    bool found_objects = false;
    for (const auto &obj : objects) {
        if (obj) {
            if (!found_objects) {
                desc << "\n<yellow>You see:</>\n";
                found_objects = true;
            }
            desc << fmt::format("  {}\n", obj->short_description());
        }
    }

    // Add other actors (check visibility)
    auto actors = room->contents().actors;
    bool found_others = false;
    for (const auto &actor : actors) {
        if (actor && actor != viewer && actor->is_visible_to(*viewer)) {
            if (!found_others) {
                desc << "\n<yellow>Also here:</>\n";
                found_others = true;
            }
            // Use display_name_for_observer for detection indicators, fall back to short_description
            auto short_desc = actor->short_description();
            std::string actor_desc;
            if (short_desc.empty()) {
                actor_desc = actor->display_name_for_observer(*viewer);
            } else {
                // Prepend detection indicators to short_description
                std::string indicators;
                if (actor->has_flag(ActorFlag::Invisible) && viewer->has_flag(ActorFlag::Detect_Invis)) {
                    indicators = "(invis) ";
                }
                if (viewer->has_flag(ActorFlag::Detect_Align)) {
                    int alignment = actor->stats().alignment;
                    if (alignment <= -350) {
                        indicators += "(evil) ";
                    } else if (alignment >= 350) {
                        indicators += "(good) ";
                    }
                }
                actor_desc = indicators + std::string(short_desc);
            }
            desc << fmt::format("  {}\n", actor_desc);
        }
    }

    return desc.str();
}

std::string format_object_description(std::shared_ptr<Object> obj, std::shared_ptr<Actor> viewer) {
    if (!obj) {
        return "You see nothing special.";
    }

    std::ostringstream desc;
    desc << fmt::format("{}\n", obj->short_description());

    // Show magical aura if viewer has Detect_Magic and object is magical
    if (viewer && viewer->has_flag(ActorFlag::Detect_Magic)) {
        if (obj->has_flag(ObjectFlag::Magic) || obj->has_flag(ObjectFlag::Glow) ||
            obj->has_flag(ObjectFlag::Hum) || obj->has_flag(ObjectFlag::Bless)) {
            desc << "<magenta>It glows with a magical aura.</>\n";
        }
    }

    // Show examine description if available, otherwise show ground description
    if (!obj->examine_description().empty()) {
        desc << fmt::format("{}\n", obj->examine_description());
    } else if (!obj->description().empty()) {
        desc << fmt::format("{}\n", obj->description());
    }

    // Add object-specific details based on type
    if (obj->type() == ObjectType::Drinkcontainer) {
        // Drink container - show liquid info, not item capacity
        const auto& liquid = obj->liquid_info();
        if (liquid.remaining > 0) {
            std::string fullness;
            int percent = (liquid.capacity > 0) ? (liquid.remaining * 100 / liquid.capacity) : 0;
            if (percent >= 90) fullness = "full";
            else if (percent >= 60) fullness = "mostly full";
            else if (percent >= 40) fullness = "about half full";
            else if (percent >= 20) fullness = "less than half full";
            else fullness = "almost empty";

            std::string liquid_name = liquid.liquid_type.empty() ? "liquid" : to_lowercase(liquid.liquid_type);
            desc << fmt::format("It is {} of {}.\n", fullness, liquid_name);
        } else {
            desc << "It is empty.\n";
        }
    } else if (obj->type() == ObjectType::Container) {
        // Regular container - show item capacity
        const auto &container_info = obj->container_info();
        if (container_info.closeable) {
            desc << fmt::format("It appears to be {}.\n", container_info.closed ? "closed" : "open");
            if (container_info.lockable) {
                desc << fmt::format("It is {}.\n", container_info.locked ? "locked" : "unlocked");
            }
        }
        // Show capacity if not full/empty and container is open
        if (!container_info.closed) {
            if (container_info.capacity > 0) {
                desc << fmt::format("It can hold {} items.\n", container_info.capacity);
            }
        }
    }

    // Show lit status for light sources (duration -1 = infinite)
    if (obj->is_light_source()) {
        const auto& light = obj->light_info();
        if (light.permanent) {
            desc << "It glows with a permanent light.\n";
        } else if (light.lit) {
            desc << "It is lit and providing light.\n";
        } else if (light.duration != 0) {
            desc << "It is not lit.\n";
        } else {
            desc << "It has burned out.\n";
        }
    }

    if (obj->is_weapon()) {
        const auto &damage = obj->damage_profile();
        desc << fmt::format("Weapon damage: {}\n", damage.to_dice_string());
    }

    if (obj->is_armor()) {
        desc << fmt::format("Armor class: {}\n", obj->armor_class());
    }

    // Show condition and quality
    if (obj->condition() < 100) {
        desc << fmt::format("Condition: {} ({})\n", obj->condition(), obj->quality_description());
    }

    // Show weight and value for examination
    // weight() is polymorphic - containers include contents automatically
    desc << fmt::format("It weighs {} pounds", obj->weight());
    if (obj->value() > 0) {
        // Object values are stored in copper
        auto value_money = fiery::Money::from_copper(obj->value());
        desc << fmt::format(" and is worth {}", value_money.to_string());
    }
    desc << ".\n";

    // Show extra descriptions available (if any exist)
    // Collect all unique keywords from all extra descriptions
    const auto &extras = obj->get_all_extra_descriptions();
    if (!extras.empty()) {
        std::set<std::string> unique_keywords;
        for (const auto &extra : extras) {
            for (const auto &keyword : extra.keywords) {
                unique_keywords.insert(keyword);
            }
        }

        if (!unique_keywords.empty()) {
            desc << "\nYou notice some details you could examine more closely: ";
            bool first = true;
            for (const auto &keyword : unique_keywords) {
                if (!first)
                    desc << ", ";
                desc << keyword;
                first = false;
            }
            desc << "\n";
        }
    }

    return desc.str();
}

std::string format_actor_description(std::shared_ptr<Actor> target, std::shared_ptr<Actor> viewer) {
    if (!target) {
        return "You see nothing special.";
    }

    std::ostringstream desc;

    // Get gender-appropriate pronouns
    auto gender = TextFormat::get_actor_gender(*target);
    const auto& pronouns = TextFormat::get_pronouns(gender);

    // Capitalize first letter of pronoun for sentence start
    auto cap_subjective = [&pronouns]() {
        std::string s{pronouns.subjective};
        if (!s.empty()) s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
        return s;
    };
    auto cap_possessive = [&pronouns]() {
        std::string s{pronouns.possessive};
        if (!s.empty()) s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
        return s;
    };

    // Show the longer description first (like "Before you stands a mage.")
    if (!target->description().empty()) {
        desc << fmt::format("{}\n", target->description());
    } else if (!target->short_description().empty()) {
        // Fallback to short description if no long description
        desc << fmt::format("{}\n", target->short_description());
    }

    // Show health status for living actors
    if (target->is_alive()) {
        const auto &stats = target->stats();
        double health_percent = static_cast<double>(stats.hit_points) / stats.max_hit_points * 100.0;

        if (health_percent < 25.0) {
            desc << fmt::format("{} looks nearly dead.\n", target->display_name());
        } else if (health_percent < 50.0) {
            desc << fmt::format("{} looks badly wounded.\n", target->display_name());
        } else if (health_percent < 75.0) {
            desc << fmt::format("{} looks wounded.\n", target->display_name());
        } else if (health_percent < 100.0) {
            desc << fmt::format("{} looks slightly hurt.\n", target->display_name());
        } else {
            desc << fmt::format("{} is in excellent condition.\n", target->display_name());
        }
    }

    // Viewer-based detection information
    if (viewer) {
        // Show alignment if viewer has Detect_Align
        if (viewer->has_flag(ActorFlag::Detect_Align)) {
            int alignment = target->stats().alignment;
            if (alignment <= -350) {
                desc << fmt::format("{} radiates an evil aura.\n", cap_subjective());
            } else if (alignment >= 350) {
                desc << fmt::format("{} radiates a holy aura.\n", cap_subjective());
            }
        }

        // Show magical aura if viewer has Detect_Magic and target has active effects
        if (viewer->has_flag(ActorFlag::Detect_Magic) && !target->active_effects().empty()) {
            desc << fmt::format("{} {} surrounded by a magical aura.\n", cap_subjective(), pronouns.verb_be);
        }

        // Show poison if viewer has Detect_Poison
        if (viewer->has_flag(ActorFlag::Detect_Poison) && target->has_flag(ActorFlag::Poison)) {
            desc << fmt::format("{} {} poisoned.\n", cap_subjective(), pronouns.verb_be);
        }
    }

    // Show position if not standing
    if (target->position() != Position::Standing && target->position() != Position::Fighting) {
        desc << fmt::format("{} is {}.\n", target->display_name(), ActorUtils::get_position_name(target->position()));
    }

    // Show size if available
    auto size_str = target->size();
    if (!size_str.empty()) {
        // Convert to lowercase for display
        std::string size_lower{size_str};
        for (char& c : size_lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        desc << fmt::format("{} {} {} in size.\n", cap_subjective(), pronouns.verb_be, size_lower);
    }

    // Show spell effects from active effects (data-driven from database)
    auto& ability_cache = FieryMUD::AbilityCache::instance();
    for (const auto& effect : target->active_effects()) {
        // Look up the ability that applied this effect
        auto* ability = ability_cache.get_ability_by_name(effect.source);
        if (ability) {
            auto* messages = ability_cache.get_ability_messages(ability->id);
            if (messages && !messages->look_message.empty()) {
                // Process template: replace {pronoun.possessive}, {pronoun.subjective}, {actor}
                std::string msg = messages->look_message;
                // Simple template replacement
                auto replace_all = [](std::string& str, std::string_view from, std::string_view to) {
                    size_t pos = 0;
                    while ((pos = str.find(from, pos)) != std::string::npos) {
                        str.replace(pos, from.length(), to);
                        pos += to.length();
                    }
                };
                replace_all(msg, "{pronoun.possessive}", cap_possessive());
                replace_all(msg, "{pronoun.subjective}", cap_subjective());
                replace_all(msg, "{actor}", std::string{target->display_name()});
                desc << msg << "\n";
            }
        }
    }

    // Show composition for mobiles (NPCs)
    auto mobile = std::dynamic_pointer_cast<Mobile>(target);
    if (mobile) {
        auto composition = mobile->composition();
        if (!composition.empty() && composition != "Flesh") {
            // Convert to lowercase for display
            std::string comp_lower{composition};
            for (char& c : comp_lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            desc << fmt::format("{} body seems to be made of {}!\n", cap_possessive(), comp_lower);
        }
    }

    // Show life force for undead/construct mobiles
    if (mobile) {
        auto life_force = mobile->life_force();
        if (!life_force.empty() && life_force != "Life") {
            // Convert to lowercase for display
            std::string lf_lower{life_force};
            for (char& c : lf_lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            desc << fmt::format("{} {} animated by {}.\n", cap_subjective(), pronouns.verb_be, lf_lower);
        }
    }

    // Show fighting status
    if (target->is_fighting()) {
        desc << fmt::format("{} is engaged in combat!\n", target->display_name());
    }

    // Show invisible (special case - they might not have an active effect for this)
    if (target->has_flag(ActorFlag::Invisible)) {
        desc << fmt::format("{} appears translucent.\n", target->display_name());
    }

    // Show flying (might be natural ability, not spell)
    if (target->has_flag(ActorFlag::Flying)) {
        desc << fmt::format("{} is hovering above the ground.\n", target->display_name());
    }

    // Show equipment
    auto equipment = target->equipment().get_all_equipped_with_slots();
    if (!equipment.empty()) {
        desc << fmt::format("{} is using:\n", target->display_name());
        for (const auto &[slot, item] : equipment) {
            if (item) {
                desc << fmt::format("{} {}\n", get_equipment_slot_display_name(slot), item->short_description());
            }
        }
    }

    return desc.str();
}

std::string format_inventory(std::shared_ptr<Actor> actor) {
    if (!actor) {
        return "You have no inventory.";
    }

    auto inventory = actor->inventory().get_all_items();
    if (inventory.empty()) {
        return "You are not carrying anything.";
    }

    // Stack items by prototype ID for display
    // Different liquid types have different prototype IDs, so they'll stack separately
    std::vector<std::pair<EntityId, int>> item_counts;
    std::unordered_map<EntityId, size_t> id_to_index;
    std::unordered_map<EntityId, std::string> id_to_description;

    for (const auto &item : inventory) {
        if (!item) continue;

        EntityId proto_id = item->id();
        auto it = id_to_index.find(proto_id);
        if (it != id_to_index.end()) {
            item_counts[it->second].second++;
        } else {
            id_to_index[proto_id] = item_counts.size();
            item_counts.emplace_back(proto_id, 1);
            // Use display_name for drinks to show liquid contents
            id_to_description[proto_id] = item->display_name(true);
        }
    }

    std::ostringstream inv;
    inv << "You are carrying:\n";
    for (const auto &[proto_id, count] : item_counts) {
        const auto &desc = id_to_description[proto_id];
        if (count > 1) {
            inv << fmt::format("  ({}) {}\n", count, desc);
        } else {
            inv << fmt::format("  {}\n", desc);
        }
    }

    return inv.str();
}

std::string get_equipment_slot_display_name(EquipSlot slot) {
    // ANSI color codes: cyan for most slots, bright yellow for wielded/held
    constexpr std::string_view cyan = "\033[36m";       // Cyan for worn items
    constexpr std::string_view bright_cyan = "\033[96m"; // Bright cyan for light
    constexpr std::string_view yellow = "\033[33m";     // Yellow for wielded/held
    constexpr std::string_view magenta = "\033[35m";    // Magenta for floating
    constexpr std::string_view reset = "\033[0m";

    switch (slot) {
    case EquipSlot::Light:
        return fmt::format("{}<used as light>{}", bright_cyan, reset);
    case EquipSlot::Finger_R:
        return fmt::format("{}<worn on right finger>{}", cyan, reset);
    case EquipSlot::Finger_L:
        return fmt::format("{}<worn on left finger>{}", cyan, reset);
    case EquipSlot::Neck1:
        return fmt::format("{}<worn around neck>{}", cyan, reset);
    case EquipSlot::Neck2:
        return fmt::format("{}<worn around neck>{}", cyan, reset);
    case EquipSlot::Body:
        return fmt::format("{}<worn on body>{}", cyan, reset);
    case EquipSlot::Head:
        return fmt::format("{}<worn on head>{}", cyan, reset);
    case EquipSlot::Legs:
        return fmt::format("{}<worn on legs>{}", cyan, reset);
    case EquipSlot::Feet:
        return fmt::format("{}<worn on feet>{}", cyan, reset);
    case EquipSlot::Hands:
        return fmt::format("{}<worn on hands>{}", cyan, reset);
    case EquipSlot::Arms:
        return fmt::format("{}<worn on arms>{}", cyan, reset);
    case EquipSlot::Shield:
        return fmt::format("{}<worn as shield>{}", cyan, reset);
    case EquipSlot::About:
        return fmt::format("{}<worn about body>{}", cyan, reset);
    case EquipSlot::Waist:
        return fmt::format("{}<worn around waist>{}", cyan, reset);
    case EquipSlot::Wrist_R:
        return fmt::format("{}<worn on right wrist>{}", cyan, reset);
    case EquipSlot::Wrist_L:
        return fmt::format("{}<worn on left wrist>{}", cyan, reset);
    case EquipSlot::Wield:
        return fmt::format("{}<wielded>{}", yellow, reset);
    case EquipSlot::Hold:
        return fmt::format("{}<held>{}", yellow, reset);
    case EquipSlot::Float:
        return fmt::format("{}<floating nearby>{}", magenta, reset);
    default:
        return fmt::format("{}<worn>{}", cyan, reset);
    }
}

std::string format_equipment(std::shared_ptr<Actor> actor) {
    if (!actor) {
        return "You have no equipment.";
    }

    auto equipment = actor->equipment().get_all_equipped_with_slots();
    if (equipment.empty()) {
        return "You are not using anything.";
    }

    std::ostringstream eq;
    eq << "You are using:\n";
    for (const auto &[slot, item] : equipment) {
        if (item) {
            eq << fmt::format("{} {}\n", get_equipment_slot_display_name(slot), item->short_description());
        }
    }

    return eq.str();
}

std::string format_who_list(const std::vector<std::shared_ptr<Actor>> &actors) {
    std::ostringstream who;
    who << fmt::format("Players currently online ({}):\n", actors.size());

    for (const auto &actor : actors) {
        if (actor) {
            who << fmt::format("  {} (Level {})\n", actor->display_name(), actor->stats().level);
        }
    }

    return who.str();
}

std::string format_exits(std::shared_ptr<Room> room) {
    if (!room) {
        return "";
    }

    auto exits = room->get_visible_exits();
    if (exits.empty()) {
        return "<dim>There are no obvious exits.</>";
    }

    std::ostringstream exit_str;
    exit_str << "<cyan>Obvious exits:</> ";

    bool first = true;
    for (auto dir : exits) {
        if (!first)
            exit_str << ", ";
        exit_str << "<b:cyan>" << RoomUtils::get_direction_name(dir) << "</>";
        first = false;
    }

    return exit_str.str();
}

bool can_move_direction(std::shared_ptr<Actor> actor, Direction dir, std::string &failure_reason) {
    if (!actor) {
        failure_reason = "Invalid actor";
        return false;
    }

    auto room = actor->current_room();
    if (!room) {
        failure_reason = "You are not in a room";
        return false;
    }

    if (!room->has_exit(dir)) {
        failure_reason = "There's no exit in that direction";
        return false;
    }

    // Check if actor is in combat
    if (actor->position() == Position::Fighting) {
        failure_reason = "You can't leave while fighting! Use 'flee' to escape combat";
        return false;
    }

    // Check if actor is dead (but allow ghosts limited movement)
    if (actor->position() == Position::Dead) {
        failure_reason = "You can't move while you're dead!";
        return false;
    }
    // Ghosts can move but with restrictions (this will be handled elsewhere)

    // Check sector-based restrictions for the destination room
    auto exit = room->get_exit(dir);
    if (exit) {
        auto dest_room = World().get_room(exit->to_room);
        if (dest_room) {
            SectorType sector = dest_room->sector_type();

            // Check if trying to go UP into a flying sector without flying ability
            if (sector == SectorType::Flying && dir == Direction::Up) {
                // Check if actor can fly
                bool can_fly = actor->has_flag(ActorFlag::Flying);

                // Check if actor is a god
                if (auto* player = dynamic_cast<const Player*>(actor.get())) {
                    if (player->is_god()) {
                        can_fly = true;
                    }
                }

                if (!can_fly) {
                    failure_reason = "You can't fly up there!";
                    return false;
                }
            }

            // Check underwater sector
            if (sector == SectorType::Underwater) {
                bool can_breathe = actor->has_flag(ActorFlag::Underwater_Breathing);
                if (auto* player = dynamic_cast<const Player*>(actor.get())) {
                    if (player->is_god()) can_breathe = true;
                }
                if (!can_breathe) {
                    failure_reason = "You can't breathe underwater!";
                    return false;
                }
            }

            // Check water sectors
            if (RoomUtils::requires_swimming(sector)) {
                bool can_traverse = actor->has_flag(ActorFlag::Waterwalk) ||
                                   actor->has_flag(ActorFlag::Flying);
                if (auto* player = dynamic_cast<const Player*>(actor.get())) {
                    if (player->is_god()) can_traverse = true;
                }
                if (!can_traverse) {
                    failure_reason = "You need to be able to swim or walk on water!";
                    return false;
                }
            }
        }
    }

    return true;
}

Result<CommandResult> execute_movement(const CommandContext &ctx, Direction dir) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    std::string failure_reason;
    if (!can_move_direction(ctx.actor, dir, failure_reason)) {
        ctx.send_error(fmt::format("Cannot move {}: {}", magic_enum::enum_name(dir), failure_reason));
        return CommandResult::InvalidState;
    }

    // Calculate movement cost based on destination room sector
    int move_cost = 1;  // Default cost
    auto room = ctx.actor->current_room();
    if (room) {
        auto exit = room->get_exit(dir);
        if (exit) {
            auto dest_room = World().get_room(exit->to_room);
            if (dest_room) {
                move_cost = RoomUtils::get_movement_cost(dest_room->sector_type());

                // Flying reduces movement cost to 1 for most terrain
                if (ctx.actor->position() == Position::Flying ||
                    ctx.actor->has_flag(ActorFlag::Flying)) {
                    move_cost = 1;
                }

                // Mounted actors use mount's movement (reduced cost)
                if (ctx.actor->is_mounted()) {
                    move_cost = std::max(1, move_cost / 2);
                }

                // Waterwalk reduces water sector cost
                if (ctx.actor->has_flag(ActorFlag::Waterwalk) &&
                    RoomUtils::is_water_sector(dest_room->sector_type())) {
                    move_cost = 1;
                }
            }
        }
    }

    // Check if actor has enough movement points (skip for gods)
    bool is_god = false;
    if (auto* player = dynamic_cast<const Player*>(ctx.actor.get())) {
        is_god = player->is_god();
    }

    if (!is_god && ctx.actor->stats().stamina < move_cost) {
        ctx.send_error("You are too exhausted to move!");
        return CommandResult::ResourceError;
    }

    // Deduct stamina (unless god)
    if (!is_god) {
        ctx.actor->stats().stamina -= move_cost;
    }

    // Interrupt concentration-based activities (like meditation)
    ctx.actor->interrupt_concentration();

    // Get the current room before movement for LEAVE triggers
    auto old_room = ctx.actor->current_room();

    // Dispatch LEAVE triggers to mobs in the old room
    auto& trigger_mgr = FieryMUD::TriggerManager::instance();
    if (trigger_mgr.is_initialized() && old_room) {
        for (const auto& other : old_room->contents().actors) {
            // Only mobs have triggers, skip self and players
            if (other == ctx.actor || other->type_name() != "Mobile") {
                continue;
            }

            auto result = trigger_mgr.dispatch_leave(other, ctx.actor, dir);
            if (result == FieryMUD::TriggerResult::Halt) {
                // Movement blocked by LEAVE trigger
                Log::debug("Movement blocked by LEAVE trigger on {}", other->name());
                ctx.send_error("Something prevents you from leaving.");
                return CommandResult::InvalidState;
            }
        }
    }

    auto result = ctx.move_actor_direction(dir);
    if (!result) {
        ctx.send_error(fmt::format("Movement failed: {}", result.error().message));
        return CommandResult::ResourceError;
    }

    // Get the new room after movement
    auto new_room = ctx.actor->current_room();

    // Dispatch GREET triggers to mobs in the new room
    if (trigger_mgr.is_initialized() && new_room) {
        // Calculate the direction the actor came FROM (opposite of movement)
        Direction from_direction = RoomUtils::get_opposite_direction(dir);

        for (const auto& other : new_room->contents().actors) {
            // Only mobs have triggers, skip self and players
            if (other == ctx.actor || other->type_name() != "Mobile") {
                continue;
            }

            // GREET triggers fire for all arrivals
            trigger_mgr.dispatch_greet(other, ctx.actor, from_direction);

            // GREET_ALL fires even for mobs entering (not just players)
            // This is handled by checking the flag inside dispatch_greet_all if implemented
        }

        // If the moving actor is a mob, dispatch ENTRY triggers on itself
        if (ctx.actor->type_name() == "Mobile") {
            trigger_mgr.dispatch_entry(ctx.actor, new_room, from_direction);
        }
    }

    // Send movement confirmation message
    std::string dir_name = to_lowercase(magic_enum::enum_name(dir));
    ctx.send(fmt::format("You move {}.", dir_name));

    // Check if actor falls (if in flying sector without flight ability)
    World().check_and_handle_falling(ctx.actor);

    // Automatically show the new room after movement (or after falling)
    // Create a context copy with cleared arguments so look shows the room, not a target
    auto look_ctx = ctx;
    look_ctx.command.arguments.clear();
    auto look_result = InformationCommands::cmd_look(look_ctx);
    if (!look_result) {
        ctx.send_error("Movement succeeded but could not display new room");
    }

    return CommandResult::Success;
}

void send_communication(const CommandContext &ctx, std::string_view message, MessageType type,
                        std::string_view channel_name) {
    std::string formatted_message = format_communication(ctx.actor, message, channel_name, type);

    switch (type) {
    case MessageType::Say:
        ctx.send(fmt::format("<white>You say, '{}'</>", message));
        ctx.send_to_room(formatted_message, true);
        break;
    case MessageType::Broadcast:
        ctx.send(fmt::format("<b:red>You {}, '{}'</>", channel_name, message));
        ctx.send_to_all(formatted_message, true);  // exclude self
        break;
    case MessageType::Channel:
        ctx.send(fmt::format("<magenta>You {}, '{}'</>", channel_name, message));
        ctx.send_to_all(formatted_message, true);  // exclude self
        break;
    default:
        ctx.send_to_room(formatted_message, false);
        break;
    }
}

std::string format_communication(std::shared_ptr<Actor> sender, std::string_view message,
                                 std::string_view channel, MessageType type) {
    if (!sender) {
        return std::string{message};
    }

    // Choose color based on message type
    std::string_view color;
    switch (type) {
    case MessageType::Say:
        color = "white";
        break;
    case MessageType::Broadcast:
        color = "b:red";
        break;
    case MessageType::Channel:
        color = "magenta";
        break;
    case MessageType::Tell:
        color = "cyan";
        break;
    case MessageType::Emote:
        color = "yellow";
        break;
    default:
        color = "white";
        break;
    }

    if (channel.empty()) {
        return fmt::format("<{}>{} says, '{}'</>", color, sender->display_name(), message);
    } else {
        return fmt::format("<{}>{} {}s, '{}'</>", color, sender->display_name(), channel, message);
    }
}

} // namespace Helpers

} // namespace BuiltinCommands
