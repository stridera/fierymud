/***************************************************************************
 *   File: src/scripting/bindings/lua_actor.cpp              Part of FieryMUD *
 *  Usage: Lua bindings for Actor/Mobile/Player classes                     *
 ***************************************************************************/

#include "lua_actor.hpp"
#include "../../core/actor.hpp"
#include "../../world/room.hpp"
#include "../../database/generated/db_enums.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

namespace FieryMUD {

namespace {

// Helper to convert legacy DG script abbreviated effect flags to EffectFlag enum
// Supports both legacy abbreviations (INVIS) and full names (INVISIBLE)
std::optional<db::EffectFlag> effect_flag_from_string(std::string_view s) {
    // First try the database string converter (handles full names like "INVISIBLE")
    auto result = db::effect_flag_from_db(s);
    if (result) {
        return result;
    }

    // Legacy DG script abbreviations
    static const std::unordered_map<std::string_view, db::EffectFlag> legacy_lookup = {
        {"INVIS", db::EffectFlag::Invisible},
        {"DETECT_INVIS", db::EffectFlag::DetectInvis},
        {"DETECT_MAGIC", db::EffectFlag::DetectMagic},
        {"SENSE_LIFE", db::EffectFlag::SenseLife},
        {"SANCT", db::EffectFlag::Sanctuary},
        {"PROT_EVIL", db::EffectFlag::ProtectEvil},
        {"PROT_GOOD", db::EffectFlag::ProtectGood},
        {"INFRA", db::EffectFlag::Infravision},
        {"PARA", db::EffectFlag::MajorParalysis},
        {"MINOR_PARA", db::EffectFlag::MinorParalysis},
        {"PROT_FIRE", db::EffectFlag::ProtectFire},
        {"PROT_COLD", db::EffectFlag::ProtectCold},
        {"PROT_AIR", db::EffectFlag::ProtectAir},
        {"PROT_EARTH", db::EffectFlag::ProtectEarth},
    };

    auto it = legacy_lookup.find(s);
    if (it != legacy_lookup.end()) {
        return it->second;
    }

    return std::nullopt;
}

} // anonymous namespace

void register_actor_bindings(sol::state& lua) {
    // Position enum as string table
    lua.new_enum<Position>("Position",
        {
            {"Dead", Position::Dead},
            {"Ghost", Position::Ghost},
            {"MortallyWounded", Position::Mortally_Wounded},
            {"Incapacitated", Position::Incapacitated},
            {"Stunned", Position::Stunned},
            {"Sleeping", Position::Sleeping},
            {"Resting", Position::Resting},
            {"Sitting", Position::Sitting},
            {"Prone", Position::Prone},
            {"Fighting", Position::Fighting},
            {"Standing", Position::Standing},
            {"Flying", Position::Flying}
        }
    );

    // Actor base class (read-only properties, safe methods)
    lua.new_usertype<Actor>("Actor",
        sol::no_constructor,

        // Read-only properties
        "name", sol::property([](const Actor& a) { return std::string(a.name()); }),
        "display_name", sol::property([](const Actor& a) { return std::string(a.display_name()); }),
        "level", sol::property([](const Actor& a) { return a.stats().level; }),
        "hp", sol::property([](const Actor& a) { return a.stats().hit_points; }),
        "max_hp", sol::property([](const Actor& a) { return a.stats().max_hit_points; }),
        "mana", sol::property([](const Actor& a) { return a.stats().mana; }),
        "max_mana", sol::property([](const Actor& a) { return a.stats().max_mana; }),
        "stamina", sol::property([](const Actor& a) { return a.stats().stamina; }),
        "max_stamina", sol::property([](const Actor& a) { return a.stats().max_stamina; }),
        "move", sol::property([](const Actor& a) { return a.stats().stamina; }),  // Alias for backwards compat
        "max_move", sol::property([](const Actor& a) { return a.stats().max_stamina; }),  // Alias for backwards compat
        "alignment", sol::property([](const Actor& a) { return a.stats().alignment; }),
        "wealth", sol::property(&Actor::wealth),

        // Primary stats
        "strength", sol::property([](const Actor& a) { return a.stats().strength; }),
        "dexterity", sol::property([](const Actor& a) { return a.stats().dexterity; }),
        "intelligence", sol::property([](const Actor& a) { return a.stats().intelligence; }),
        "wisdom", sol::property([](const Actor& a) { return a.stats().wisdom; }),
        "constitution", sol::property([](const Actor& a) { return a.stats().constitution; }),
        "charisma", sol::property([](const Actor& a) { return a.stats().charisma; }),

        // Position and state
        "position", sol::property([](const Actor& a) { return a.position(); }),
        "position_name", sol::property([](const Actor& a) {
            return std::string(ActorUtils::get_position_name(a.position()));
        }),

        // Room access (returns Room userdata or nil)
        "room", sol::property([](const Actor& a) -> std::shared_ptr<Room> {
            return a.current_room();
        }),

        // Type checks
        "is_npc", sol::property([](const Actor& a) {
            return a.type_name() == "Mobile";
        }),
        "is_player", sol::property([](const Actor& a) {
            return a.type_name() == "Player";
        }),

        // State checks
        "is_fighting", sol::property(&Actor::is_fighting),
        "is_alive", sol::property(&Actor::is_alive),
        "can_act", sol::property(&Actor::can_act),
        "is_afk", sol::property(&Actor::is_afk),

        // Gender and race
        "gender", sol::property([](const Actor& a) { return std::string(a.gender()); }),
        "race", sol::property([](const Actor& a) { return std::string(a.race()); }),
        "size", sol::property([](const Actor& a) { return std::string(a.size()); }),

        // Methods - Communication
        "send", [](Actor& a, const std::string& msg) {
            a.send_message(msg);
        },

        // Methods - Actions (direct room broadcast, bypasses command system for script efficiency)
        "say", [](Actor& a, const std::string& msg) {
            auto room = a.current_room();
            if (room) {
                std::string formatted = fmt::format("{} says, '{}'\n", a.display_name(), msg);
                for (auto& actor : room->contents().actors) {
                    actor->send_message(formatted);
                }
            }
        },

        "emote", [](Actor& a, const std::string& msg) {
            auto room = a.current_room();
            if (room) {
                std::string formatted = fmt::format("{} {}\n", a.display_name(), msg);
                for (auto& actor : room->contents().actors) {
                    actor->send_message(formatted);
                }
            }
        },

        // Methods - Combat/Stats
        "damage", [](Actor& a, int amount) {
            if (amount <= 0) return;
            a.stats().hit_points = std::max(0, a.stats().hit_points - amount);
            if (a.stats().hit_points == 0) {
                a.set_position(Position::Mortally_Wounded);
            }
        },

        "heal", [](Actor& a, int amount) {
            if (amount <= 0) return;
            a.stats().hit_points = std::min(a.stats().max_hit_points,
                                            a.stats().hit_points + amount);
        },

        // Methods - Inventory queries
        "has_item", [](const Actor& a, const std::string& keyword) -> bool {
            auto items = a.inventory().find_items_by_keyword(keyword);
            return !items.empty();
        },

        "item_count", [](const Actor& a, const std::string& keyword) -> int {
            auto items = a.inventory().find_items_by_keyword(keyword);
            return static_cast<int>(items.size());
        },

        // Methods - Economy (unified interface)
        "give_wealth", &Actor::give_wealth,
        "take_wealth", &Actor::take_wealth,
        "has_wealth", &Actor::can_afford,

        // Methods - Effects
        "has_effect", [](const Actor& a, const std::string& name) -> bool {
            return a.has_effect(name);
        },

        "has_flag", [](const Actor& a, ActorFlag flag) -> bool {
            return a.has_flag(flag);
        },

        // Legacy DG script method for checking effect flags (accepts string like "INVIS")
        "get_aff_flagged", [](const Actor& a, const std::string& flag_name) -> bool {
            auto flag = effect_flag_from_string(flag_name);
            if (!flag) {
                spdlog::warn("Unknown effect flag: {}", flag_name);
                return false;
            }
            // Convert EffectFlag to string and use the string-based has_effect
            // (Actor::has_effect(string) checks ActiveEffects by name)
            auto flag_str = std::string(magic_enum::enum_name(*flag));
            return a.has_effect(flag_str);
        },

        // Methods - Movement (requires Room binding)
        "teleport", [](Actor& a, std::shared_ptr<Room> room) -> bool {
            if (!room) return false;
            auto result = a.move_to(room);
            return result.has_value();
        }
    );

    // Mobile (NPC) class - inherits Actor, adds NPC-specific properties
    lua.new_usertype<Mobile>("Mobile",
        sol::no_constructor,
        sol::base_classes, sol::bases<Actor>(),

        // NPC-specific properties
        "is_aggressive", sol::property(&Mobile::is_aggressive),
        "is_shopkeeper", sol::property(&Mobile::is_shopkeeper),
        "is_banker", sol::property(&Mobile::is_banker),
        "is_teacher", sol::property(&Mobile::is_teacher),

        // Prototype reference
        "prototype_id", sol::property([](const Mobile& m) -> std::string {
            auto id = m.prototype_id();
            return fmt::format("{}:{}", id.zone_id(), id.local_id());
        }),

        // Description
        "description", sol::property([](const Mobile& m) {
            return std::string(m.description());
        }),

        // Combat dice
        "damage_dice_num", sol::property(&Mobile::bare_hand_damage_dice_num),
        "damage_dice_size", sol::property(&Mobile::bare_hand_damage_dice_size),

        // Mob flags
        "has_mob_flag", [](const Mobile& m, MobFlag flag) -> bool {
            return m.has_flag(flag);
        }
    );

    // Player class - inherits Actor, adds player-specific properties
    lua.new_usertype<Player>("Player",
        sol::no_constructor,
        sol::base_classes, sol::bases<Actor>(),

        // Player-specific properties
        "is_god", sol::property(&Player::is_god),
        "god_level", sol::property(&Player::god_level),
        "is_online", sol::property(&Player::is_online),
        "is_linkdead", sol::property(&Player::is_linkdead),

        // Class and title
        "class", sol::property([](const Player& p) { return p.player_class(); }),
        "title", sol::property([](const Player& p) { return std::string(p.title()); }),

        // Clan info
        "in_clan", sol::property(&Player::in_clan),
        "clan_name", sol::property([](const Player& p) { return std::string(p.clan_name()); }),
        "clan_rank", sol::property([](const Player& p) { return std::string(p.clan_rank_title()); }),

        // Player flags
        "is_brief", sol::property(&Player::is_brief),
        "is_autoloot", sol::property(&Player::is_autoloot),
        "is_pk_enabled", sol::property(&Player::is_pk_enabled)
    );

    // MobFlag enum for script access (subset of commonly used flags)
    lua.new_enum<MobFlag>("MobFlag",
        {
            {"Aggressive", MobFlag::Aggressive},
            {"Sentinel", MobFlag::Sentinel},
            {"Scavenger", MobFlag::Scavenger},
            {"Memory", MobFlag::Memory},
            {"Helper", MobFlag::Helper},
            {"NoCharm", MobFlag::NoCharm},
            {"NoSummon", MobFlag::NoSummon},
            {"NoSleep", MobFlag::NoSleep},
            {"NoBash", MobFlag::NoBash},
            {"NoBlind", MobFlag::NoBlind},
            {"NoKill", MobFlag::NoKill},
            {"Peaceful", MobFlag::Peaceful},
            {"Spec", MobFlag::Spec},
            {"Banker", MobFlag::Banker},
            {"Teacher", MobFlag::Teacher}
        }
    );

    // ActorFlag enum for script access
    lua.new_enum<ActorFlag>("ActorFlag",
        {
            {"Blind", ActorFlag::Blind},
            {"Invisible", ActorFlag::Invisible},
            {"DetectInvis", ActorFlag::Detect_Invis},
            {"DetectMagic", ActorFlag::Detect_Magic},
            {"SenseLife", ActorFlag::Sense_Life},
            {"Sanctuary", ActorFlag::Sanctuary},
            {"Poison", ActorFlag::Poison},
            {"Flying", ActorFlag::Flying},
            {"Sneak", ActorFlag::Sneak},
            {"Hide", ActorFlag::Hide},
            {"Charm", ActorFlag::Charm},
            {"Haste", ActorFlag::Haste},
            {"Slow", ActorFlag::Slow},
            {"Berserk", ActorFlag::Berserk},
            {"Paralyzed", ActorFlag::Paralyzed}
        }
    );

    // EffectFlag enum for script access (commonly used effect flags)
    lua.new_enum<db::EffectFlag>("EffectFlag",
        {
            {"Blind", db::EffectFlag::Blind},
            {"Invisible", db::EffectFlag::Invisible},
            {"DetectAlign", db::EffectFlag::DetectAlign},
            {"DetectInvis", db::EffectFlag::DetectInvis},
            {"DetectMagic", db::EffectFlag::DetectMagic},
            {"SenseLife", db::EffectFlag::SenseLife},
            {"Waterwalk", db::EffectFlag::Waterwalk},
            {"Sanctuary", db::EffectFlag::Sanctuary},
            {"Confusion", db::EffectFlag::Confusion},
            {"Curse", db::EffectFlag::Curse},
            {"Infravision", db::EffectFlag::Infravision},
            {"Poison", db::EffectFlag::Poison},
            {"ProtectEvil", db::EffectFlag::ProtectEvil},
            {"ProtectGood", db::EffectFlag::ProtectGood},
            {"Sleep", db::EffectFlag::Sleep},
            {"Berserk", db::EffectFlag::Berserk},
            {"Sneak", db::EffectFlag::Sneak},
            {"Stealth", db::EffectFlag::Stealth},
            {"Fly", db::EffectFlag::Fly},
            {"Charm", db::EffectFlag::Charm},
            {"StoneSkin", db::EffectFlag::StoneSkin},
            {"Haste", db::EffectFlag::Haste},
            {"Blur", db::EffectFlag::Blur},
            {"MajorParalysis", db::EffectFlag::MajorParalysis},
            {"MinorParalysis", db::EffectFlag::MinorParalysis},
            {"Silence", db::EffectFlag::Silence},
            {"ProtectFire", db::EffectFlag::ProtectFire},
            {"ProtectCold", db::EffectFlag::ProtectCold},
            {"Fireshield", db::EffectFlag::Fireshield},
            {"Coldshield", db::EffectFlag::Coldshield},
            {"Fear", db::EffectFlag::Fear},
            {"Disease", db::EffectFlag::Disease},
            {"Bless", db::EffectFlag::Bless},
            {"Hex", db::EffectFlag::Hex}
        }
    );

    spdlog::debug("Lua Actor bindings registered");
}

} // namespace FieryMUD
