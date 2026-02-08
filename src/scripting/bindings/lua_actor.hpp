#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register Actor, Mobile, and Player bindings with Lua.
 *
 * Exposes read-only properties and safe methods for scripting.
 * Scripts can query actor state and perform actions but cannot
 * directly modify internals.
 *
 * Lua API:
 *   actor.name           - Display name
 *   actor.level          - Character level
 *   actor.hp             - Current hit points
 *   actor.max_hp         - Maximum hit points
 *   actor.mana           - Current mana
 *   actor.max_mana       - Maximum mana
 *   actor.alignment      - Good/evil alignment (-1000 to 1000)
 *   actor.gold           - Gold carried
 *   actor.position       - Current position (string)
 *   actor.room           - Current room (Room userdata)
 *   actor.is_npc         - True if NPC, false if player
 *   actor.is_fighting    - True if in combat
 *   actor.is_alive       - True if alive
 *
 *   actor:send(msg)      - Send message to actor
 *   actor:say(msg)       - Actor says message to room
 *   actor:emote(msg)     - Actor emotes to room
 *   actor:damage(n)      - Deal damage to actor
 *   actor:heal(n)        - Heal actor
 *   actor:has_item(kw)   - Check if actor has item by keyword
 *   actor:give_gold(n)   - Give gold to actor
 *   actor:take_gold(n)   - Take gold from actor
 *   actor:teleport(room) - Move actor to room
 *   actor:has_effect(name) - Check if actor has effect
 */
void register_actor_bindings(sol::state &lua);

} // namespace FieryMUD
