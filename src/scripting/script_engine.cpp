#include <sol/sol.hpp>
#include "script_engine.hpp"
#include "coroutine_scheduler.hpp"
#include "trigger_manager.hpp"
#include "bindings/lua_actor.hpp"
#include "bindings/lua_room.hpp"
#include "bindings/lua_object.hpp"
#include "bindings/lua_quest.hpp"
#include "bindings/lua_combat.hpp"
#include "bindings/lua_world.hpp"
#include "bindings/lua_spells.hpp"
#include "bindings/lua_zone.hpp"
#include "bindings/lua_exit.hpp"
#include "bindings/lua_skills.hpp"
#include "bindings/lua_effects.hpp"
#include "bindings/lua_timers.hpp"
#include "bindings/lua_vars.hpp"
#include "bindings/lua_templates.hpp"
#include "world/time_system.hpp"

#include <fmt/format.h>

#include <random>

namespace FieryMUD {

ScriptEngine& ScriptEngine::instance() {
    static ScriptEngine instance;
    return instance;
}

ScriptEngine::~ScriptEngine() {
    if (initialized_) {
        shutdown();
    }
}

ScriptResult<void> ScriptEngine::initialize() {
    if (initialized_) {
        spdlog::warn("ScriptEngine::initialize() called when already initialized");
        return {};
    }

    spdlog::info("Initializing Lua scripting engine (Sol3)...");

    try {
        // Create Lua state
        lua_ = std::make_unique<sol::state>();

        // Open Lua libraries
        lua_->open_libraries(
            sol::lib::base,
            sol::lib::coroutine,
            sol::lib::string,
            sol::lib::table,
            sol::lib::math
        );

        // Configure sandbox (remove dangerous functions)
        configure_sandbox();

        // Register utility functions
        register_utility_functions();

        // Register game bindings (to be implemented in Phase 2)
        register_bindings();

        // Create persistent globals table for scripts to share variables
        // This mimics DG Script's "global" variables that persist across trigger executions
        (*lua_)["globals"] = lua_->create_table();

        initialized_ = true;
        spdlog::info("Lua scripting engine initialized successfully");

        return {};

    } catch (const sol::error& e) {
        last_error_ = fmt::format("Lua initialization failed: {}", e.what());
        spdlog::error("{}", last_error_);
        lua_.reset();  // Clean up on failure
        return std::unexpected(ScriptError::InvalidState);
    }
}

void ScriptEngine::shutdown() {
    if (!initialized_) {
        return;
    }

    spdlog::info("Shutting down Lua scripting engine...");

    // Clear bytecode cache and failed script cache first
    // (these contain sol::bytecode which references lua state)
    bytecode_cache_.clear();
    failed_script_cache_.clear();

    // Force garbage collection before destroying state
    if (lua_) {
        spdlog::debug("Running Lua garbage collection...");
        lua_->collect_garbage();
        spdlog::debug("Garbage collection complete");

        // Explicitly destroy Lua state NOW, not during static destruction
        // This ensures proper destruction order: CoroutineScheduler clears its
        // sol::thread/coroutine objects first, THEN we destroy the Lua state
        spdlog::debug("Destroying Lua state...");
        lua_.reset();
        spdlog::debug("Lua state destroyed");
    }

    initialized_ = false;

    spdlog::info("Lua scripting engine shutdown complete");
}

void ScriptEngine::configure_sandbox() {
    // Remove dangerous base library functions
    (*lua_)["loadfile"] = sol::nil;
    (*lua_)["dofile"] = sol::nil;
    (*lua_)["load"] = sol::nil;      // Could load arbitrary bytecode
    (*lua_)["loadstring"] = sol::nil;
    (*lua_)["rawset"] = sol::nil;    // Could modify protected tables
    (*lua_)["rawget"] = sol::nil;
    (*lua_)["rawequal"] = sol::nil;
    (*lua_)["collectgarbage"] = sol::nil;  // Could cause performance issues
    (*lua_)["getmetatable"] = sol::nil;    // Could access internal structures
    (*lua_)["setmetatable"] = sol::nil;    // Could modify behavior

    // These libraries are NOT loaded (blocked entirely):
    // - io: File system access
    // - os: Operating system calls
    // - debug: Debugging/introspection
    // - package: Module loading
    // - ffi: Foreign function interface (LuaJIT)

    spdlog::debug("Lua sandbox configured - dangerous functions removed");
}

void ScriptEngine::register_utility_functions() {
    // Static random engine for dice/random functions
    // Note: Game logic runs serialized on the world strand, so no mutex needed
    static std::mt19937 rng{std::random_device{}()};

    // dice(count, sides) - Roll dice: dice(2, 6) = 2d6
    lua_->set_function("dice", [](int count, int sides) -> int {
        if (count <= 0 || sides <= 0) return 0;
        std::uniform_int_distribution<int> dist(1, sides);
        int total = 0;
        for (int i = 0; i < count; ++i) {
            total += dist(rng);
        }
        return total;
    });

    // random(min, max) - Random integer in range [min, max]
    lua_->set_function("random", [](int min_val, int max_val) -> int {
        if (min_val > max_val) std::swap(min_val, max_val);
        std::uniform_int_distribution<int> dist(min_val, max_val);
        return dist(rng);
    });

    // percent_chance(n) - Returns true n% of the time
    lua_->set_function("percent_chance", [](int percent) -> bool {
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(rng) <= percent;
    });

    // echo(message) - Debug output to server logs
    lua_->set_function("echo", [](std::string_view message) {
        spdlog::info("[Lua] {}", message);
    });

    // log_debug(message) - Debug level logging
    lua_->set_function("log_debug", [](std::string_view message) {
        spdlog::debug("[Lua] {}", message);
    });

    // log_warn(message) - Warning level logging
    lua_->set_function("log_warn", [](std::string_view message) {
        spdlog::warn("[Lua] {}", message);
    });

    // log_error(message) - Error level logging
    lua_->set_function("log_error", [](std::string_view message) {
        spdlog::error("[Lua] {}", message);
    });

    // MUD time function - returns current in-game time
    lua_->set_function("mud_time", [this]() -> sol::table {
        const auto& time = TimeSystem::instance().current_time();
        auto result = lua_->create_table();
        result["hour"] = time.hour;
        result["day"] = time.day + 1;  // Convert 0-indexed to 1-indexed for Lua
        result["month"] = static_cast<int>(time.month) + 1;  // Convert 0-indexed to 1-indexed
        result["year"] = time.year;
        return result;
    });

    // wait(seconds) - Yield coroutine for delayed execution
    // The actual scheduling is handled by TriggerManager when it sees the yield
    // This function yields with the delay value using sol::yielding wrapper
    lua_->set_function("wait", sol::yielding([](double seconds) -> double {
        // Clamp delay to valid range
        seconds = std::clamp(seconds,
                             CoroutineScheduler::MIN_DELAY_SECONDS,
                             CoroutineScheduler::MAX_DELAY_SECONDS);

        spdlog::debug("[Lua] wait({}) called - yielding coroutine", seconds);

        // Return the delay value - it will be yielded to the caller
        // TriggerManager will read this value and schedule resume
        return seconds;
    }));

    // run_room_trigger(legacy_id) - Execute a world trigger by ID
    // Used to invoke shared room-level effects (door manipulation, spawning, etc.)
    lua_->set_function("run_room_trigger", [](int legacy_id) {
        int zone_id = legacy_id / 100;
        if (zone_id == 0) zone_id = 1000;
        int local_id = legacy_id % 100;

        auto& trigger_mgr = TriggerManager::instance();
        EntityId trigger_eid(zone_id, local_id);

        auto trigger = trigger_mgr.find_trigger_by_id(trigger_eid);
        if (!trigger) {
            spdlog::warn("run_room_trigger: Trigger {}:{} not found", zone_id, local_id);
            return;
        }

        // Execute the trigger with minimal context (world triggers don't need owner)
        ScriptContext ctx = ScriptContext::Builder()
            .set_trigger(trigger)
            .build();

        trigger_mgr.debug_execute_trigger(trigger, ctx);
    });

    spdlog::debug("Lua utility functions registered");
}

void ScriptEngine::register_bindings() {
    // Register game object bindings
    register_actor_bindings(*lua_);
    register_room_bindings(*lua_);
    register_object_bindings(*lua_);
    register_quest_bindings(*lua_);

    // Register namespace-based bindings
    register_combat_bindings(*lua_);
    register_world_bindings(*lua_);
    register_spell_bindings(*lua_);
    register_zone_bindings(*lua_);
    register_exit_bindings(*lua_);
    register_skill_bindings(*lua_);
    register_effect_bindings(*lua_);
    register_timer_bindings(*lua_);
    register_var_bindings(*lua_);
    register_template_bindings(*lua_);

    // Register Effect table for typo-safe effect name lookups
    // Usage: actor:has_effect(Effect.Invisible) instead of actor:has_effect("Invisible")
    // If Effect.Invisibl is used (typo), it returns nil which evaluates to false
    register_effect_table();

    spdlog::debug("Lua game bindings registered");
}

void ScriptEngine::register_effect_table() {
    // Create Effect table mapping effect names to their string values
    // This allows typo-safe effect checks: Effect.Invisible returns "Invisible"
    // If a typo like Effect.Invisibl is used, it returns nil
    sol::table effect_table = lua_->create_table();

    // Common spell effects
    effect_table["Invisible"] = "Invisible";
    effect_table["Sanctuary"] = "Sanctuary";
    effect_table["Bless"] = "Bless";
    effect_table["Armor"] = "Armor";
    effect_table["Shield"] = "Shield";
    effect_table["Stoneskin"] = "Stoneskin";
    effect_table["Haste"] = "Haste";
    effect_table["Slow"] = "Slow";
    effect_table["Fly"] = "Fly";
    effect_table["Flying"] = "Flying";
    effect_table["Levitate"] = "Levitate";
    effect_table["Infravision"] = "Infravision";
    effect_table["DetectInvis"] = "DetectInvis";
    effect_table["DetectMagic"] = "DetectMagic";
    effect_table["DetectAlign"] = "DetectAlign";
    effect_table["SenseLife"] = "SenseLife";
    effect_table["Waterwalk"] = "Waterwalk";
    effect_table["WaterBreathing"] = "WaterBreathing";

    // Status effects
    effect_table["Blind"] = "Blind";
    effect_table["Poison"] = "Poison";
    effect_table["Curse"] = "Curse";
    effect_table["Paralyzed"] = "Paralyzed";
    effect_table["Sleep"] = "Sleep";
    effect_table["Charm"] = "Charm";
    effect_table["Berserk"] = "Berserk";

    // Stealth effects
    effect_table["Sneak"] = "Sneak";
    effect_table["Hide"] = "Hide";

    // Protection effects
    effect_table["ProtectionEvil"] = "ProtectionEvil";
    effect_table["ProtectionGood"] = "ProtectionGood";
    effect_table["FireShield"] = "FireShield";
    effect_table["ColdShield"] = "ColdShield";
    effect_table["HeatResistance"] = "HeatResistance";
    effect_table["ColdResistance"] = "ColdResistance";

    // Expose as global Effect table
    (*lua_)["Effect"] = effect_table;

    spdlog::debug("Lua Effect table registered with {} entries", effect_table.size());
}

sol::thread ScriptEngine::create_thread() {
    return sol::thread::create(lua_->lua_state());
}

bool ScriptEngine::is_cached(std::string_view cache_key) const {
    return bytecode_cache_.contains(std::string{cache_key});
}

ScriptResult<sol::function> ScriptEngine::load_cached(
    sol::state_view thread_state,
    std::string_view script_code,
    std::string_view cache_key)
{
    if (!initialized_) {
        last_error_ = "ScriptEngine not initialized";
        return std::unexpected(ScriptError::NotInitialized);
    }

    std::string key{cache_key};

    // Check if this script previously failed to compile (prevents repeated compilation attempts)
    auto failed_it = failed_script_cache_.find(key);
    if (failed_it != failed_script_cache_.end()) {
        // Script previously failed - return cached error without logging (prevents log spam)
        last_error_ = failed_it->second;
        return std::unexpected(ScriptError::CompilationFailed);
    }

    // Check if bytecode is already cached
    auto it = bytecode_cache_.find(key);
    if (it != bytecode_cache_.end()) {
        // Load from cached bytecode
        try {
            sol::load_result loaded = thread_state.load(it->second.as_string_view(), key,
                                                         sol::load_mode::binary);
            if (!loaded.valid()) {
                sol::error err = loaded;
                last_error_ = fmt::format("Failed to load cached bytecode '{}': {}",
                                           cache_key, err.what());
                spdlog::error("{}", last_error_);
                // Fall through to recompile
            } else {
                spdlog::trace("Loaded script from bytecode cache: {}", cache_key);
                return loaded.get<sol::function>();
            }
        } catch (const sol::error& e) {
            spdlog::warn("Failed to load bytecode for '{}', recompiling: {}",
                         cache_key, e.what());
            // Fall through to recompile
        }
    }

    // Compile the script
    try {
        sol::load_result loaded = thread_state.load(script_code, key);

        if (!loaded.valid()) {
            sol::error err = loaded;
            last_error_ = fmt::format("Compilation error in '{}': {}",
                                       cache_key, err.what());
            spdlog::error("{}", last_error_);
            // Cache the failure to prevent repeated compilation attempts
            failed_script_cache_[key] = last_error_;
            return std::unexpected(ScriptError::CompilationFailed);
        }

        // Cache the bytecode
        sol::function func = loaded;
        sol::bytecode bc = func.dump();
        bytecode_cache_[key] = std::move(bc);

        spdlog::debug("Compiled and cached script: {}", cache_key);
        return func;

    } catch (const sol::error& e) {
        last_error_ = fmt::format("Unexpected error compiling '{}': {}",
                                   cache_key, e.what());
        spdlog::error("{}", last_error_);
        // Cache the failure to prevent repeated compilation attempts
        failed_script_cache_[key] = last_error_;
        return std::unexpected(ScriptError::CompilationFailed);
    }
}

ScriptResult<void> ScriptEngine::compile_script(
    std::string_view script_code,
    std::string_view script_name)
{
    if (!initialized_) {
        last_error_ = "ScriptEngine not initialized";
        return std::unexpected(ScriptError::NotInitialized);
    }

    try {
        // Compile the script
        sol::load_result loaded = lua_->load(script_code, std::string{script_name});

        if (!loaded.valid()) {
            sol::error err = loaded;
            last_error_ = fmt::format("Compilation error in '{}': {}",
                                       script_name, err.what());
            spdlog::error("{}", last_error_);
            return std::unexpected(ScriptError::CompilationFailed);
        }

        // Get the function and dump to bytecode for caching
        sol::function func = loaded;
        sol::bytecode bc = func.dump();

        // Cache the bytecode
        bytecode_cache_[std::string{script_name}] = std::move(bc);

        spdlog::debug("Compiled and cached script: {}", script_name);
        return {};

    } catch (const sol::error& e) {
        last_error_ = fmt::format("Unexpected error compiling '{}': {}",
                                   script_name, e.what());
        spdlog::error("{}", last_error_);
        return std::unexpected(ScriptError::CompilationFailed);
    }
}

ScriptResult<sol::protected_function_result> ScriptEngine::execute(
    std::string_view script_code,
    std::string_view script_name)
{
    if (!initialized_) {
        last_error_ = "ScriptEngine not initialized";
        return std::unexpected(ScriptError::NotInitialized);
    }

    try {
        auto result = lua_->safe_script(script_code,
                                        sol::script_pass_on_error);

        if (!result.valid()) {
            sol::error err = result;
            last_error_ = fmt::format("Execution error in '{}': {}",
                                       script_name, err.what());
            spdlog::error("{}", last_error_);
            return std::unexpected(ScriptError::ExecutionFailed);
        }

        return result;

    } catch (const sol::error& e) {
        last_error_ = fmt::format("Unexpected error in '{}': {}",
                                   script_name, e.what());
        spdlog::error("{}", last_error_);
        return std::unexpected(ScriptError::ExecutionFailed);
    }
}

// ============================================================================
// Timeout Protection Implementation
// ============================================================================

void ScriptEngine::timeout_hook(lua_State* L, lua_Debug* /*ar*/) {
    // Get the ScriptEngine instance and increment instruction count
    auto& engine = ScriptEngine::instance();

    // Increment instruction counter by the hook interval
    int count = engine.instruction_count_.fetch_add(HOOK_INTERVAL, std::memory_order_relaxed);
    count += HOOK_INTERVAL;

    // Check if we've exceeded the limit
    if (engine.max_instructions_ > 0 && count >= engine.max_instructions_) {
        engine.timed_out_.store(true, std::memory_order_release);
        spdlog::error("Script execution timeout: exceeded {} instructions - aborting!",
                      engine.max_instructions_);

        // Force a Lua error to abort execution
        luaL_error(L, "Script execution timeout: exceeded %d instructions",
                   engine.max_instructions_);
    }
}

void ScriptEngine::install_timeout_hook(lua_State* L) {
    if (max_instructions_ <= 0) {
        return;  // No limit set
    }

    // Reset counters before installing hook
    reset_instruction_counter();

    // Install the debug hook to be called every HOOK_INTERVAL instructions
    lua_sethook(L, timeout_hook, LUA_MASKCOUNT, HOOK_INTERVAL);
}

void ScriptEngine::remove_timeout_hook(lua_State* L) {
    // Remove the debug hook
    lua_sethook(L, nullptr, 0, 0);
}

} // namespace FieryMUD
