// trigger_manager.cpp - Trigger loading, caching, and dispatch

#include "trigger_manager.hpp"
#include "script_engine.hpp"
#include "coroutine_scheduler.hpp"

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

    // TODO: Load from database via TriggerQueries
    // For now, just mark the zone as loaded (triggers will be registered manually)
    loaded_zones_.insert(zone_id);
    spdlog::debug("Zone {} marked as loaded (database loading not yet implemented)", zone_id);

    return 0;
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
        return TriggerResult::Error;
    }

    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        last_error_ = "ScriptEngine not initialized";
        return TriggerResult::Error;
    }

    // Create a thread for this script (enables coroutine support)
    sol::thread thread = engine.create_thread();
    sol::state_view thread_lua(thread.state());

    // Set up the Lua environment with context on the thread
    setup_lua_context(thread_lua, context);

    // Load the script in the thread's state
    sol::load_result loaded = thread_lua.load(trigger->commands, trigger->name);
    if (!loaded.valid()) {
        sol::error err = loaded;
        last_error_ = fmt::format("Trigger '{}' load error: {}", trigger->name, err.what());
        spdlog::error("{}", last_error_);
        return TriggerResult::Error;
    }

    // Create coroutine from the loaded function
    sol::coroutine coro(thread.state(), loaded.get<sol::function>());

    // Run the coroutine
    auto result = coro();

    if (!result.valid()) {
        sol::error err = result;
        last_error_ = fmt::format("Trigger '{}' execution failed: {}", trigger->name, err.what());
        spdlog::error("{}", last_error_);
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

            spdlog::debug("Trigger '{}' yielded for {} seconds", trigger->name, delay);
        } else {
            spdlog::warn("Trigger '{}' called wait() but CoroutineScheduler not initialized",
                         trigger->name);
        }
        return TriggerResult::Continue;
    }

    // Coroutine completed - check return value
    // If script returns false, halt further processing
    if (result.get_type() == sol::type::boolean) {
        if (!result.get<bool>()) {
            return TriggerResult::Halt;
        }
    }

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
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::COMMAND);

    for (const auto& trigger : triggers) {
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(actor)
            .set_command(command)
            .set_argument(argument)
            .set_room(owner->current_room())
            .build();

        auto result = execute_trigger(trigger, context);
        if (result == TriggerResult::Halt) {
            return TriggerResult::Halt;
        }
        if (result == TriggerResult::Error) {
            // Log but continue to next trigger
            spdlog::warn("Command trigger error on '{}', continuing", trigger->name);
        }
    }

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
    auto triggers = find_triggers(entity_id, ScriptType::MOB, TriggerFlag::FIGHT);

    for (const auto& trigger : triggers) {
        auto context = ScriptContext::Builder()
            .set_trigger(trigger)
            .set_owner(owner)
            .set_actor(opponent)  // opponent as actor
            .set_room(owner->current_room())
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
