#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register Quest system bindings with Lua.
 *
 * Provides quest management functions for scripts to trigger
 * quest events and query quest state.
 *
 * Lua API:
 *   quest.start(actor, zone_id, quest_id)     - Start a quest for actor
 *   quest.complete(actor, zone_id, quest_id)  - Complete a quest for actor
 *   quest.abandon(actor, zone_id, quest_id)   - Abandon a quest
 *   quest.status(actor, zone_id, quest_id)    - Get quest status string
 *   quest.has_quest(actor, zone_id, quest_id) - Check if actor has quest
 *   quest.is_available(actor, zone_id, quest_id) - Check if quest available
 *   quest.is_completed(actor, zone_id, quest_id) - Check if completed before
 *
 *   quest.advance_objective(actor, zone_id, quest_id, objective_id, count)
 *       - Advance an objective by count (default 1)
 *
 *   quest.set_variable(actor, zone_id, quest_id, name, value)
 *       - Set a quest variable for the actor
 *
 *   quest.get_variable(actor, zone_id, quest_id, name)
 *       - Get a quest variable value
 *
 * Quest Status Values:
 *   "NONE"        - Quest not started or doesn't exist
 *   "AVAILABLE"   - Quest is available to accept
 *   "IN_PROGRESS" - Quest is active
 *   "COMPLETED"   - Quest has been completed
 *   "FAILED"      - Quest was failed
 *   "ABANDONED"   - Quest was abandoned
 */
void register_quest_bindings(sol::state &lua);

} // namespace FieryMUD
