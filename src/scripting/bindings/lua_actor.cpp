#include "lua_actor.hpp"
#include "core/actor.hpp"
#include "core/combat.hpp"
#include "core/mobile.hpp"
#include "core/spell_system.hpp"
#include "core/skill_system.hpp"
#include "core/player.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"
#include "commands/command_system.hpp"
#include "database/generated/db_enums.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

namespace FieryMUD {

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
        // God/immortal check - works on any Actor, returns false for non-players
        // Defined as method (not property) so scripts can call actor:is_god()
        "is_god", [](const Actor& a) {
            if (auto* player = dynamic_cast<const Player*>(&a)) {
                return player->is_god();
            }
            return false;
        },

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

        // Check if actor has a specific effect by name (e.g., "Invisible", "Sanctuary")
        "has_effect_named", [](const Actor& a, const std::string& effect_name) -> bool {
            return a.has_effect(effect_name);
        },

        // Methods - Movement (requires Room binding)
        "teleport", [](Actor& a, std::shared_ptr<Room> room) -> bool {
            if (!room) return false;
            auto result = a.move_to(room);
            return result.has_value();
        },

        // Methods - Spawn object into actor's inventory
        // Usage: actor:spawn_object(zone_id, local_id)
        // Returns the object if successful, nil otherwise
        "spawn_object", [](std::shared_ptr<Actor> self, int zone_id, int local_id) -> std::shared_ptr<Object> {
            if (!self) return nullptr;

            EntityId prototype_id(zone_id, local_id);
            auto object = WorldManager::instance().create_object_instance(prototype_id);
            if (!object) {
                spdlog::warn("actor:spawn_object: Failed to create object {}:{}", zone_id, local_id);
                return nullptr;
            }

            auto result = self->give_item(object);
            if (!result) {
                spdlog::warn("actor:spawn_object: Failed to give item to actor: {}", result.error().message);
                return nullptr;
            }

            spdlog::debug("actor:spawn_object: Created object {}:{} for {}", zone_id, local_id, self->name());
            return object;
        },

        // Methods - Command execution
        // Execute a game command as this actor (e.g., self:command("smile bob"))
        "command", [](std::shared_ptr<Actor> self, const std::string& cmd) -> bool {
            if (!self || cmd.empty()) return false;
            auto result = CommandSystem::instance().execute_command(self, cmd);
            return result.has_value() && *result == CommandResult::Success;
        },

        // Methods - Combat wrappers (convenience methods that delegate to combat namespace)
        "engage", [](std::shared_ptr<Actor> self, std::shared_ptr<Actor> target) -> bool {
            if (!self || !target || self == target) return false;
            if (CombatManager::is_in_combat(*self)) return false;
            CombatManager::start_combat(self, target);
            return true;
        },

        "rescue", [](std::shared_ptr<Actor> self, std::shared_ptr<Actor> target) -> bool {
            if (!self || !target || self == target) return false;
            if (!CombatManager::is_in_combat(*target)) return false;

            auto target_attackers = target->get_all_enemies();
            for (auto& attacker : target_attackers) {
                if (attacker && attacker != self) {
                    CombatManager::add_combat_pair(attacker, self);
                    attacker->remove_enemy(target);
                }
            }
            if (!CombatManager::is_in_combat(*self) && !target_attackers.empty()) {
                self->set_position(Position::Fighting);
            }
            return true;
        },

        "disengage", [](std::shared_ptr<Actor> self) -> bool {
            if (!self) return false;
            if (!CombatManager::is_in_combat(*self)) return false;
            CombatManager::end_combat(self);
            return true;
        },

        // Methods - Spell casting wrapper
        "cast", [](Actor& self, const std::string& spell_name, sol::optional<Actor*> target) -> bool {
            if (spell_name.empty()) return false;
            auto& spell_system = SpellSystem::instance();
            if (!spell_system.spell_exists(spell_name)) return false;
            Actor* target_actor = target.value_or(nullptr);
            auto result = spell_system.cast_spell(self, spell_name, target_actor, self.stats().level);
            return result.has_value();
        },

        // Methods - Communication
        "whisper", [](Actor& self, std::shared_ptr<Actor> target, const std::string& msg) {
            if (!target || msg.empty()) return;
            std::string to_target = fmt::format("{} whispers to you, '{}'\n", self.display_name(), msg);
            std::string to_self = fmt::format("You whisper to {}, '{}'\n", target->display_name(), msg);
            target->send_message(to_target);
            self.send_message(to_self);
        },

        // Methods - Movement in a direction
        "move", [](std::shared_ptr<Actor> self, const std::string& dir_str) -> bool {
            if (!self) return false;
            // Execute the direction as a command (north, south, etc.)
            auto result = CommandSystem::instance().execute_command(self, dir_str);
            return result.has_value() && *result == CommandResult::Success;
        },

        // Gender pronoun properties (he/she/it, him/her/it, his/her/its)
        "subject", sol::property([](const Actor& a) -> std::string {
            auto gender = a.gender();
            if (gender == "male") return "he";
            if (gender == "female") return "she";
            return "it";
        }),
        "object", sol::property([](const Actor& a) -> std::string {
            auto gender = a.gender();
            if (gender == "male") return "him";
            if (gender == "female") return "her";
            return "it";
        }),
        "possessive", sol::property([](const Actor& a) -> std::string {
            auto gender = a.gender();
            if (gender == "male") return "his";
            if (gender == "female") return "her";
            return "its";
        }),

        // Methods - Following (uses master/follower system)
        // Make this actor follow another actor
        // Usage: actor:follow(target)
        // Note: target is optional to handle nil values gracefully (avoids segfault)
        "follow", [](std::shared_ptr<Actor> self, sol::optional<std::shared_ptr<Actor>> target_opt) -> bool {
            if (!self) return false;
            if (!target_opt.has_value() || !target_opt.value()) return false;
            auto target = target_opt.value();
            if (self == target) return false;  // Can't follow yourself
            // Clear any previous master
            if (auto old_master = self->get_master()) {
                old_master->remove_follower(self);
            }
            // Set new master and register as follower
            self->set_master(target);
            target->add_follower(self);
            spdlog::debug("actor:follow: {} now following {}", self->name(), target->name());
            return true;
        },

        // Stop following
        // Usage: actor:stop_following()
        "stop_following", [](std::shared_ptr<Actor> self) -> bool {
            if (!self) return false;
            if (auto master = self->get_master()) {
                master->remove_follower(self);
            }
            self->clear_master();
            return true;
        },

        // Get who this actor is following
        "master", sol::property([](const Actor& self) -> std::shared_ptr<Actor> {
            return self.get_master();
        }),

        // Check if following someone
        "has_master", sol::property([](const Actor& self) -> bool {
            return self.has_master();
        }),

        // Methods - Inventory management
        // Destroy an item in actor's inventory by keyword
        // Usage: actor:destroy_item("keyword")
        "destroy_item", [](std::shared_ptr<Actor> self, const std::string& keyword) -> bool {
            if (!self || keyword.empty()) return false;

            // Find item in inventory matching keyword
            auto items = self->inventory().find_items_by_keyword(keyword);
            if (items.empty()) {
                spdlog::debug("actor:destroy_item: No item matching '{}' found on {}", keyword, self->name());
                return false;
            }

            // Remove and destroy the first matching item
            auto& item = items[0];
            self->inventory().remove_item(item);
            spdlog::debug("actor:destroy_item: Destroyed '{}' from {}", item->name(), self->name());
            return true;
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

        // Mob trait/behavior/profession checks (new system)
        "has_trait", [](const Mobile& m, db::MobTrait trait) -> bool {
            return m.has_trait(trait);
        },
        "has_behavior", [](const Mobile& m, db::MobBehavior behavior) -> bool {
            return m.has_behavior(behavior);
        },
        "has_profession", [](const Mobile& m, db::MobProfession profession) -> bool {
            return m.has_profession(profession);
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

    // MobTrait enum - what the mob IS (identity)
    lua.new_enum<db::MobTrait>("MobTrait",
        {
            {"Illusion", db::MobTrait::Illusion},
            {"Animated", db::MobTrait::Animated},
            {"PlayerPhantasm", db::MobTrait::PlayerPhantasm},
            {"Aquatic", db::MobTrait::Aquatic},
            {"Mount", db::MobTrait::Mount},
            {"Summoned", db::MobTrait::Summoned},
            {"Pet", db::MobTrait::Pet}
        }
    );

    // MobBehavior enum - how the mob ACTS
    lua.new_enum<db::MobBehavior>("MobBehavior",
        {
            {"Sentinel", db::MobBehavior::Sentinel},
            {"StayZone", db::MobBehavior::StayZone},
            {"Scavenger", db::MobBehavior::Scavenger},
            {"Track", db::MobBehavior::Track},
            {"SlowTrack", db::MobBehavior::SlowTrack},
            {"FastTrack", db::MobBehavior::FastTrack},
            {"Wimpy", db::MobBehavior::Wimpy},
            {"Aware", db::MobBehavior::Aware},
            {"Helper", db::MobBehavior::Helper},
            {"Protector", db::MobBehavior::Protector},
            {"Peacekeeper", db::MobBehavior::Peacekeeper},
            {"NoBash", db::MobBehavior::NoBash},
            {"NoSummon", db::MobBehavior::NoSummon},
            {"NoVicious", db::MobBehavior::NoVicious},
            {"Memory", db::MobBehavior::Memory},
            {"Teacher", db::MobBehavior::Teacher},
            {"Meditate", db::MobBehavior::Meditate},
            {"NoScript", db::MobBehavior::NoScript},
            {"NoClassAi", db::MobBehavior::NoClassAi},
            {"Peaceful", db::MobBehavior::Peaceful},
            {"NoKill", db::MobBehavior::NoKill}
        }
    );

    // MobProfession enum - services the mob provides
    lua.new_enum<db::MobProfession>("MobProfession",
        {
            {"Banker", db::MobProfession::Banker},
            {"Shopkeeper", db::MobProfession::Shopkeeper},
            {"Receptionist", db::MobProfession::Receptionist},
            {"Postmaster", db::MobProfession::Postmaster},
            {"Guildmaster", db::MobProfession::Guildmaster},
            {"Trainer", db::MobProfession::Trainer}
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

    // Note: Effects are data-driven (stored in Effect table), not enum-based.
    // Use actor:has_effect("EffectName") or actor:has_effect_named("EffectName") in Lua scripts.

    spdlog::debug("Lua Actor bindings registered");
}

} // namespace FieryMUD
