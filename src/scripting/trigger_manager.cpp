// trigger_manager.cpp - Trigger loading, caching, and dispatch

#include "trigger_manager.hpp"
#include "script_engine.hpp"
#include "coroutine_scheduler.hpp"
#include "../database/connection_pool.hpp"
#include "../database/trigger_queries.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace FieryMUD {

TriggerManager& TriggerManager::instance() {
    static TriggerManager instance;
    return instance;
}

bool TriggerManager::initialize() {
    if (initialized_) {
        spdlog::warn("TriggerManager::initialize() called when already initialized");
        return true;
    }

    // Ensure ScriptEngine is initialized
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        last_error_ = "ScriptEngine must be initialized before TriggerManager";
        spdlog::error("{}", last_error_);
        return false;
    }

    initialized_ = true;
    spdlog::info("TriggerManager initialized");
    return true;
}

void TriggerManager::shutdown() {
    if (!initialized_) {
        return;
    }

    spdlog::info("TriggerManager shutting down...");
    clear_all_triggers();
    initialized_ = false;
    spdlog::info("TriggerManager shutdown complete");
}

// ============================================================================
// Trigger Loading
// ============================================================================

std::expected<std::size_t, std::string> TriggerManager::load_zone_triggers(std::uint32_t zone_id) {
    if (!initialized_) {
        return std::unexpected("TriggerManager not initialized");
    }

    // Check if zone already loaded
    if (loaded_zones_.contains(zone_id)) {
        spdlog::debug("Zone {} triggers already loaded", zone_id);
        return 0;
    }

    // Check if connection pool is available
    auto& pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        spdlog::debug("Database not initialized, skipping trigger load for zone {}", zone_id);
        return 0;
    }

    // Load triggers from database using execute pattern
    auto result = pool.execute([zone_id](pqxx::work& txn)
        -> Result<std::vector<FieryMUD::TriggerDataPtr>> {
        return TriggerQueries::load_triggers_for_zone(txn, static_cast<int>(zone_id));
    });

    if (!result) {
        return std::unexpected(fmt::format("Failed to load triggers for zone {}: {}",
                                           zone_id, result.error().message));
    }

    // Register all loaded triggers
    std::size_t count = 0;
    for (const auto& trigger : *result) {
        register_trigger(trigger);
        count++;
    }

    loaded_zones_.insert(zone_id);
    spdlog::info("Loaded {} triggers for zone {}", count, zone_id);

    return count;
}

std::expected<std::size_t, std::string> TriggerManager::reload_zone_triggers(std::uint32_t zone_id) {
    clear_zone_triggers(zone_id);
    return load_zone_triggers(zone_id);
}

void TriggerManager::clear_zone_triggers(std::uint32_t zone_id) {
    // Remove all triggers for entities in this zone
    std::erase_if(trigger_cache_, [zone_id](const auto& pair) {
        return pair.first.entity_id.zone_id() == zone_id;
    });
    loaded_zones_.erase(zone_id);
    spdlog::debug("Cleared triggers for zone {}", zone_id);
}

void TriggerManager::clear_all_triggers() {
    trigger_cache_.clear();
    loaded_zones_.clear();
    spdlog::debug("Cleared all triggers");
}

void TriggerManager::register_trigger(TriggerDataPtr trigger) {
    if (!trigger) {
        return;
    }

    auto entity_id = trigger->attached_entity_id();
    if (!entity_id) {
        spdlog::warn("Trigger '{}' has no attached entity", trigger->name);
        return;
    }

    CacheKey key{trigger->attach_type, *entity_id};
    trigger_cache_[key].add(trigger);

    spdlog::debug("Registered trigger '{}' ({}) for entity {}:{}",
        trigger->name,
        static_cast<int>(trigger->attach_type),
        entity_id->zone_id(),
        entity_id->local_id());
}

// ============================================================================
// Trigger Lookup
// ============================================================================

TriggerSet TriggerManager::get_mob_triggers(const EntityId& mob_id) const {
    CacheKey key{ScriptType::MOB, mob_id};
    auto it = trigger_cache_.find(key);
    if (it != trigger_cache_.end()) {
        return it->second;
    }
    return {};
}

TriggerSet TriggerManager::get_object_triggers(const EntityId& obj_id) const {
    CacheKey key{ScriptType::OBJECT, obj_id};
    auto it = trigger_cache_.find(key);
    if (it != trigger_cache_.end()) {
        return it->second;
    }
    return {};
}

TriggerSet TriggerManager::get_world_triggers(std::uint32_t zone_id) const {
    CacheKey key{ScriptType::WORLD, EntityId{zone_id, 0}};
    auto it = trigger_cache_.find(key);
    if (it != trigger_cache_.end()) {
        return it->second;
    }
    return {};
}

std::vector<TriggerDataPtr> TriggerManager::find_triggers(
    const EntityId& entity_id,
    ScriptType type,
    TriggerFlag flag) const
{
    CacheKey key{type, entity_id};
    auto it = trigger_cache_.find(key);
    if (it != trigger_cache_.end()) {
        return it->second.find_by_flag(flag);
    }
    return {};
}

// ============================================================================
// Trigger Dispatch
// ============================================================================

void TriggerManager::setup_lua_context(sol::state_view lua, const ScriptContext& context) {
    // Set up standard context variables
    if (auto actor = context.owner_as_actor()) {
        lua["self"] = actor;
    } else if (auto object = context.owner_as_object()) {
        lua["self"] = object;
    } else if (auto room = context.owner_as_room()) {
        lua["self"] = room;
    }

    if (context.actor()) {
        lua["actor"] = context.actor();
    }

    if (context.target()) {
        lua["target"] = context.target();
    }

    if (context.object()) {
        lua["object"] = context.object();
    }

    if (context.room()) {
        lua["room"] = context.room();
    }

    // Command-specific variables
    if (!context.command().empty()) {
        lua["cmd"] = std::string{context.command()};
    }

    if (!context.argument().empty()) {
        lua["arg"] = std::string{context.argument()};
    }

    if (!context.speech().empty()) {
        lua["speech"] = std::string{context.speech()};
    }

    if (context.direction()) {
        lua["direction"] = *context.direction();
    }

    if (context.amount()) {
        lua["amount"] = *context.amount();
        lua["damage"] = *context.amount();  // Alias for ATTACK/DEFEND triggers
    }

    // Load trigger-specific variables from JSON
    if (context.trigger() && !context.trigger()->variables.empty()) {
        lua["vars"] = lua.create_table();
        for (auto& [key, value] : context.trigger()->variables.items()) {
            if (value.is_string()) {
                lua["vars"][key] = value.get<std::string>();
            } else if (value.is_number_integer()) {
                lua["vars"][key] = value.get<int>();
            } else if (value.is_number_float()) {
                lua["vars"][key] = value.get<double>();
            } else if (value.is_boolean()) {
                lua["vars"][key] = value.get<bool>();
            }
        }
    }
}

TriggerResult TriggerManager::execute_trigger(const TriggerDataPtr& trigger, ScriptContext& context) {
    if (!trigger) {
        ++stats_.failed_executions;
        return TriggerResult::Error;
    }

    ++stats_.total_executions;
    spdlog::debug("Executing trigger '{}' (id:{})", trigger->name, trigger->id);

    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        last_error_ = "ScriptEngine not initialized";
        ++stats_.failed_executions;
        return TriggerResult::Error;
    }

    // Create a thread for this script (enables coroutine support)
    sol::thread thread = engine.create_thread();
    sol::state_view thread_lua(thread.state());

    // Set up the Lua environment with context on the thread
    setup_lua_context(thread_lua, context);

    // Load the script using bytecode cache for performance
    auto loaded = engine.load_cached(thread_lua, trigger->commands, trigger->cache_key());
    if (!loaded) {
        last_error_ = fmt::format("Trigger '{}' (id:{}) load error: {}",
                                   trigger->name, trigger->id, engine.last_error());
        spdlog::error("{}", last_error_);
        ++stats_.failed_executions;
        return TriggerResult::Error;
    }

    // Create coroutine from the loaded function
    sol::coroutine coro(thread.state(), *loaded);

    // Install timeout protection before running the script
    engine.install_timeout_hook(thread.state());

    // Run the coroutine with exception handling
    sol::protected_function_result result;
    try {
        result = coro();
    } catch (const sol::error& e) {
        // Always remove the timeout hook, even on error
        engine.remove_timeout_hook(thread.state());

        // Check if this was a timeout
        if (engine.timed_out()) {
            last_error_ = fmt::format("Trigger '{}' (id:{}) execution timeout: exceeded {} instructions",
                                       trigger->name, trigger->id, engine.max_instructions());
            spdlog::error("{}", last_error_);
            ++stats_.failed_executions;
            return TriggerResult::Error;
        }

        last_error_ = fmt::format("Trigger '{}' (id:{}) sol exception: {}",
                                   trigger->name, trigger->id, e.what());
        spdlog::error("{}", last_error_);
        ++stats_.failed_executions;
        return TriggerResult::Error;
    } catch (const std::exception& e) {
        engine.remove_timeout_hook(thread.state());
        last_error_ = fmt::format("Trigger '{}' (id:{}) C++ exception: {}",
                                   trigger->name, trigger->id, e.what());
        spdlog::error("{}", last_error_);
        ++stats_.failed_executions;
        return TriggerResult::Error;
    } catch (...) {
        engine.remove_timeout_hook(thread.state());
        last_error_ = fmt::format("Trigger '{}' (id:{}) unknown exception",
                                   trigger->name, trigger->id);
        spdlog::error("{}", last_error_);
        ++stats_.failed_executions;
        return TriggerResult::Error;
    }

    // Remove timeout hook after successful execution
    engine.remove_timeout_hook(thread.state());

    // Check for timeout (luaL_error may result in invalid result rather than exception)
    if (engine.timed_out()) {
        last_error_ = fmt::format("Trigger '{}' (id:{}) execution timeout: exceeded {} instructions",
                                   trigger->name, trigger->id, engine.max_instructions());
        spdlog::error("{}", last_error_);
        ++stats_.failed_executions;
        return TriggerResult::Error;
    }

    if (!result.valid()) {
        sol::error err = result;
        // Include entity context in error message for easier debugging
        std::string entity_info;
        if (auto actor = context.owner_as_actor()) {
            entity_info = fmt::format(" [owner: {} ({}:{})]",
                                       actor->name(), actor->id().zone_id(), actor->id().local_id());
        }
        last_error_ = fmt::format("Trigger '{}' (id:{}) execution failed: {}{}",
                                   trigger->name, trigger->id, err.what(), entity_info);
        spdlog::error("{}", last_error_);
        ++stats_.failed_executions;
        return TriggerResult::Error;
    }

    // Check coroutine status
    auto status = coro.status();

    if (status == sol::call_status::yielded) {
        // Script called wait() - extract delay and schedule resume
        auto& scheduler = get_coroutine_scheduler();
        if (scheduler.is_initialized()) {
            // Get the delay value from the result (yielded by wait())
            double delay = CoroutineScheduler::MIN_DELAY_SECONDS;
            if (result.valid() && result.return_count() > 0) {
                delay = result.get<double>(0);
            }

            // Get entity ID for cancellation tracking
            EntityId owner_id{0, 0};
            if (auto actor = context.owner_as_actor()) {
                owner_id = actor->id();
            } else if (auto object = context.owner_as_object()) {
                owner_id = object->id();
            }

            // Schedule the coroutine to resume
            scheduler.schedule_wait(std::move(thread), std::move(coro),
                                    std::move(context), owner_id, delay);

            spdlog::debug("Trigger '{}' (id:{}) yielded for {} seconds",
                          trigger->name, trigger->id, delay);
        } else {
            spdlog::warn("Trigger '{}' (id:{}) called wait() but CoroutineScheduler not initialized",
                         trigger->name, trigger->id);
        }
        ++stats_.yielded_executions;
        return TriggerResult::Continue;
    }

    // Coroutine completed - check return value
    // If script returns false, halt further processing
    if (result.get_type() == sol::type::boolean) {
        if (!result.get<bool>()) {
            ++stats_.halted_executions;
            return TriggerResult::Halt;
        }
    }

    ++stats_.successful_executions;
    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_command(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Actor> actor,
    std::string_view command,
    std::string_view argument)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    // Get the entity ID for lookup
    auto entity_id = owner->id();
    spdlog::debug("dispatch_command: looking up triggers for entity {}:{}",
                  entity_id.zone_id(), entity_id.local_id());
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::COMMAND);
    spdlog::debug("dispatch_command: found {} COMMAND triggers", triggers.size());

    for (const auto& trigger : triggers) {
        spdlog::debug("dispatch_command: executing trigger '{}'", trigger->name);
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(actor)
            .set_command(command)
            .set_argument(argument)
            .set_room(owner->current_room())
            .build();

        auto result = execute_trigger(trigger, context);
        spdlog::debug("dispatch_command: trigger '{}' returned {}",
                      trigger->name, static_cast<int>(result));
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
        if (result == TriggerResult::Error) {
            // Log but continue to next trigger
            spdlog::warn("Command trigger error on '{}', continuing", trigger->name);
        }
    }

    spdlog::debug("dispatch_command: finished, returning Continue");
    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_speech(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Actor> actor,
    std::string_view speech)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::SPEECH);

    for (const auto& trigger : triggers) {
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(actor)
            .set_speech(speech)
            .set_room(owner->current_room())
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_greet(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Actor> actor,
    Direction from_direction)
{
    if (!initialized_ || !owner || !actor) {
        return TriggerResult::Continue;
    }

    // Don't trigger on self
    if (owner == actor) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();

    // Check both GREET and GREET_ALL
    std::vector<TriggerDataPtr> triggers;
    auto greet_triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::GREET);
    auto greet_all_triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::GREET_ALL);

    triggers.insert(triggers.end(), greet_triggers.begin(), greet_triggers.end());
    triggers.insert(triggers.end(), greet_all_triggers.begin(), greet_all_triggers.end());

    for (const auto& trigger : triggers) {
        // GREET only fires for players, GREET_ALL fires for everyone
        bool is_greet_all = trigger->has_flag(TriggerFlag::GREET_ALL);
        if (!is_greet_all && actor->type_name() != "Player") {
            continue;
        }

        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(actor)
            .set_direction(from_direction)
            .set_room(owner->current_room())
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_entry(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Room> room,
    Direction from_direction)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::ENTRY);

    for (const auto& trigger : triggers) {
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_room(room)
            .set_direction(from_direction)
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_leave(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Actor> actor,
    Direction to_direction)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::LEAVE);

    for (const auto& trigger : triggers) {
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(actor)
            .set_direction(to_direction)
            .set_room(owner->current_room())
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_receive(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Actor> actor,
    std::shared_ptr<Object> object)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::RECEIVE);

    for (const auto& trigger : triggers) {
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(actor)
            .set_object(object)
            .set_room(owner->current_room())
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_bribe(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Actor> actor,
    int amount)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::BRIBE);

    for (const auto& trigger : triggers) {
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(actor)
            .set_amount(amount)
            .set_room(owner->current_room())
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_death(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Actor> killer)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::DEATH);

    for (const auto& trigger : triggers) {
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(killer)  // killer as actor
            .set_room(owner->current_room())
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_fight(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Actor> opponent)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    spdlog::trace("dispatch_fight: checking triggers for {} (id:{}:{})",
                 owner->name(), entity_id.zone_id(), entity_id.local_id());

    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::FIGHT);
    spdlog::trace("dispatch_fight: found {} FIGHT triggers", triggers.size());

    for (const auto& trigger : triggers) {
        spdlog::trace("dispatch_fight: executing trigger '{}'", trigger->name);
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(opponent)  // opponent as actor
            .set_room(owner->current_room())
            .build();

        auto result = execute_trigger(trigger, context);
        spdlog::trace("dispatch_fight: trigger '{}' returned {}", trigger->name, static_cast<int>(result));
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
    }

    spdlog::trace("dispatch_fight: done");
    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_hit_percent(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Actor> attacker,
    int hp_percent)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::HIT_PERCENT);

    for (const auto& trigger : triggers) {
        // Check if HP is below the trigger threshold
        // numeric_arg stores the HP percentage threshold (e.g., 50 for 50%)
        int threshold = trigger->numeric_arg > 0 ? trigger->numeric_arg : 25;
        if (hp_percent > threshold) {
            continue;
        }

        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(attacker)
            .set_room(owner->current_room())
            .set_amount(hp_percent)  // Current HP percentage
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_random(std::shared_ptr<Actor> owner) {
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::RANDOM);

    for (const auto& trigger : triggers) {
        // Check numeric_arg for random percentage
        int chance = trigger->numeric_arg > 0 ? trigger->numeric_arg : 100;

        // Roll for random trigger
        if (std::rand() % 100 >= chance) {
            continue;
        }

        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_room(owner->current_room())
            .build();

        execute_trigger(trigger, context);
        // Random triggers don't halt
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_load(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Room> room)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::LOAD);

    for (const auto& trigger : triggers) {
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_room(room)
            .build();

        execute_trigger(trigger, context);
        // Load triggers don't halt
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_cast(
    std::shared_ptr<Actor> owner,
    std::shared_ptr<Actor> caster,
    std::string_view spell_name)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::CAST);

    for (const auto& trigger : triggers) {
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(caster)
            .set_room(owner->current_room())
            .set_argument(std::string(spell_name))
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;  // Spell can be blocked
        }
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_time(
    std::shared_ptr<Actor> owner,
    int hour)
{
    if (!initialized_ || !owner) {
        return TriggerResult::Continue;
    }

    auto entity_id = owner->id();
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::TIME);

    for (const auto& trigger : triggers) {
        // Check if this trigger should fire at the current hour
        // numeric_arg stores the hour (0-23)
        int target_hour = trigger->numeric_arg;
        if (target_hour >= 0 && target_hour < 24 && target_hour != hour) {
            continue;
        }

        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_room(owner->current_room())
            .set_amount(hour)  // Current hour as amount
            .build();

        execute_trigger(trigger, context);
        // Time triggers don't halt
    }

    return TriggerResult::Continue;
}

// ============================================================================
// OBJECT Trigger Dispatch
// ============================================================================

TriggerResult TriggerManager::dispatch_attack(
    std::shared_ptr<Object> weapon,
    std::shared_ptr<Actor> attacker,
    std::shared_ptr<Actor> target,
    int damage,
    std::shared_ptr<Room> room)
{
    if (!initialized_ || !weapon) {
        return TriggerResult::Continue;
    }

    auto entity_id = weapon->id();
    auto triggers = find_triggers(entity_id, ScriptType::OBJECT, TriggerFlag::ATTACK);

    for (const auto& trigger : triggers) {
        spdlog::debug("ATTACK trigger '{}' firing for weapon {}:{}",
                       trigger->name, entity_id.zone_id(), entity_id.local_id());

        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(weapon)        // self = the weapon
            .set_actor(attacker)      // actor = who's wielding the weapon
            .set_target(target)       // target = who's being attacked
            .set_room(room)           // room = where combat is happening
            .set_amount(damage)       // amount/damage = damage dealt
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
    }

    return TriggerResult::Continue;
}

TriggerResult TriggerManager::dispatch_defend(
    std::shared_ptr<Object> armor,
    std::shared_ptr<Actor> defender,
    std::shared_ptr<Actor> attacker,
    int damage,
    std::shared_ptr<Room> room)
{
    if (!initialized_ || !armor) {
        return TriggerResult::Continue;
    }

    auto entity_id = armor->id();
    auto triggers = find_triggers(entity_id, ScriptType::OBJECT, TriggerFlag::DEFEND);

    for (const auto& trigger : triggers) {
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(armor)         // self = the armor/shield
            .set_actor(defender)      // actor = who's wearing the armor
            .set_target(attacker)     // target = who's attacking
            .set_room(room)           // room = where combat is happening
            .set_amount(damage)       // amount/damage = incoming damage
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
    }

    return TriggerResult::Continue;
}

// ============================================================================
// Diagnostics
// ============================================================================

std::size_t TriggerManager::trigger_count() const {
    std::size_t count = 0;
    for (const auto& [key, set] : trigger_cache_) {
        count += set.size();
    }
    return count;
}

std::size_t TriggerManager::trigger_count(ScriptType type) const {
    std::size_t count = 0;
    for (const auto& [key, set] : trigger_cache_) {
        if (key.type == type) {
            count += set.size();
        }
    }
    return count;
}

} // namespace FieryMUD
