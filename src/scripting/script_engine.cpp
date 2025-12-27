#include "script_engine.hpp"
#include "coroutine_scheduler.hpp"
#include "bindings/lua_actor.hpp"
#include "bindings/lua_room.hpp"
#include "bindings/lua_object.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

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
        // Open Lua state
        lua_.open_libraries(
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

        initialized_ = true;
        spdlog::info("Lua scripting engine initialized successfully");

        return {};

    } catch (const sol::error& e) {
        last_error_ = fmt::format("Lua initialization failed: {}", e.what());
        spdlog::error("{}", last_error_);
        return std::unexpected(ScriptError::InvalidState);
    }
}

void ScriptEngine::shutdown() {
    if (!initialized_) {
        return;
    }

    spdlog::info("Shutting down Lua scripting engine...");

    // Clear bytecode cache
    bytecode_cache_.clear();

    // Lua state will be cleaned up by destructor
    initialized_ = false;

    spdlog::info("Lua scripting engine shutdown complete");
}

void ScriptEngine::configure_sandbox() {
    // Remove dangerous base library functions
    lua_["loadfile"] = sol::nil;
    lua_["dofile"] = sol::nil;
    lua_["load"] = sol::nil;      // Could load arbitrary bytecode
    lua_["loadstring"] = sol::nil;
    lua_["rawset"] = sol::nil;    // Could modify protected tables
    lua_["rawget"] = sol::nil;
    lua_["rawequal"] = sol::nil;
    lua_["collectgarbage"] = sol::nil;  // Could cause performance issues
    lua_["getmetatable"] = sol::nil;    // Could access internal structures
    lua_["setmetatable"] = sol::nil;    // Could modify behavior

    // These libraries are NOT loaded (blocked entirely):
    // - io: File system access
    // - os: Operating system calls
    // - debug: Debugging/introspection
    // - package: Module loading
    // - ffi: Foreign function interface (LuaJIT)

    spdlog::debug("Lua sandbox configured - dangerous functions removed");
}

void ScriptEngine::register_utility_functions() {
    // Thread-local random engine for dice/random functions
    static thread_local std::mt19937 rng{std::random_device{}()};

    // dice(count, sides) - Roll dice: dice(2, 6) = 2d6
    lua_.set_function("dice", [](int count, int sides) -> int {
        if (count <= 0 || sides <= 0) return 0;
        std::uniform_int_distribution<int> dist(1, sides);
        int total = 0;
        for (int i = 0; i < count; ++i) {
            total += dist(rng);
        }
        return total;
    });

    // random(min, max) - Random integer in range [min, max]
    lua_.set_function("random", [](int min_val, int max_val) -> int {
        if (min_val > max_val) std::swap(min_val, max_val);
        std::uniform_int_distribution<int> dist(min_val, max_val);
        return dist(rng);
    });

    // percent_chance(n) - Returns true n% of the time
    lua_.set_function("percent_chance", [](int percent) -> bool {
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(rng) <= percent;
    });

    // echo(message) - Debug output to server logs
    lua_.set_function("echo", [](std::string_view message) {
        spdlog::info("[Lua] {}", message);
    });

    // log_debug(message) - Debug level logging
    lua_.set_function("log_debug", [](std::string_view message) {
        spdlog::debug("[Lua] {}", message);
    });

    // log_warn(message) - Warning level logging
    lua_.set_function("log_warn", [](std::string_view message) {
        spdlog::warn("[Lua] {}", message);
    });

    // log_error(message) - Error level logging
    lua_.set_function("log_error", [](std::string_view message) {
        spdlog::error("[Lua] {}", message);
    });

    // MUD time function (placeholder - will connect to time_system)
    lua_.set_function("mud_time", [this]() -> sol::table {
        // TODO: Connect to TimeSystem when available
        auto result = lua_.create_table();
        result["hour"] = 12;
        result["day"] = 1;
        result["month"] = 1;
        result["year"] = 650;
        return result;
    });

    // wait(seconds) - Yield coroutine for delayed execution
    // The actual scheduling is handled by TriggerManager when it sees the yield
    // This function yields with the delay value using sol::yielding wrapper
    lua_.set_function("wait", sol::yielding([](double seconds) -> double {
        // Clamp delay to valid range
        seconds = std::clamp(seconds,
                             CoroutineScheduler::MIN_DELAY_SECONDS,
                             CoroutineScheduler::MAX_DELAY_SECONDS);

        spdlog::debug("[Lua] wait({}) called - yielding coroutine", seconds);

        // Return the delay value - it will be yielded to the caller
        // TriggerManager will read this value and schedule resume
        return seconds;
    }));

    spdlog::debug("Lua utility functions registered");
}

void ScriptEngine::register_bindings() {
    // Register game object bindings
    register_actor_bindings(lua_);
    register_room_bindings(lua_);
    register_object_bindings(lua_);

    spdlog::debug("Lua game bindings registered");
}

sol::thread ScriptEngine::create_thread() {
    return sol::thread::create(lua_.lua_state());
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
        sol::load_result loaded = lua_.load(script_code, std::string{script_name});

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
        auto result = lua_.safe_script(script_code,
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

} // namespace FieryMUD
