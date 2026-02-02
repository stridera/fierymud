#pragma once

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/forward.hpp>
#include <sol/error.hpp>
#include <sol/thread.hpp>

#include <spdlog/spdlog.h>

namespace FieryMUD {

/// Error types for script operations
enum class ScriptError {
    NotInitialized,
    CompilationFailed,
    ExecutionFailed,
    SandboxViolation,
    InvalidState,
    Timeout  // Script exceeded instruction limit
};

/// Result type for script operations
template<typename T>
using ScriptResult = std::expected<T, ScriptError>;

/**
 * ScriptEngine - Central Lua scripting system manager
 *
 * Manages the Sol3/Lua state, provides sandboxed script execution,
 * and caches compiled scripts for performance.
 *
 * Thread Safety: NOT thread-safe. Must be called from game loop strand.
 */
class ScriptEngine {
public:
    /// Get the singleton instance
    static ScriptEngine& instance();

    /// Initialize the Lua engine (call once at startup)
    ScriptResult<void> initialize();

    /// Shutdown and clean up resources
    void shutdown();

    /// Check if engine is initialized
    [[nodiscard]] bool is_initialized() const { return initialized_; }

    /// Compile a script and cache the bytecode
    /// @param script_code The Lua source code
    /// @param script_name Identifier for error messages
    /// @return Success or compilation error
    ScriptResult<void> compile_script(std::string_view script_code,
                                       std::string_view script_name);

    /// Load a script, using cached bytecode if available
    /// @param thread_state The thread's Lua state to load into
    /// @param script_code The Lua source code
    /// @param cache_key Unique key for bytecode caching (e.g., "trigger:30:0:quest_start")
    /// @return Loaded function or error
    ScriptResult<sol::function> load_cached(sol::state_view thread_state,
                                             std::string_view script_code,
                                             std::string_view cache_key);

    /// Check if a script is in the bytecode cache
    [[nodiscard]] bool is_cached(std::string_view cache_key) const;

    /// Get bytecode cache statistics
    [[nodiscard]] std::size_t cache_size() const { return bytecode_cache_.size(); }

    /// Get failed script cache size
    [[nodiscard]] std::size_t failed_cache_size() const { return failed_script_cache_.size(); }

    /// Clear the bytecode cache (and failed script cache)
    void clear_cache() { bytecode_cache_.clear(); failed_script_cache_.clear(); }

    /// Clear just the failed script cache (allows retry of failed scripts)
    void clear_failed_cache() { failed_script_cache_.clear(); }

    /// Create an isolated thread for trigger execution
    /// Each trigger runs in its own thread to support coroutines
    [[nodiscard]] sol::thread create_thread();

    /// Execute a script in the main state (for simple expressions)
    /// @param script_code The Lua code to execute
    /// @param script_name Identifier for error messages
    /// @return Success or execution error
    ScriptResult<sol::protected_function_result> execute(
        std::string_view script_code,
        std::string_view script_name = "script");

    /// Execute a pre-compiled function with error handling
    template<typename... Args>
    ScriptResult<sol::protected_function_result> safe_call(
        sol::protected_function& func, Args&&... args);

    /// Access the main Lua state (for binding registration)
    [[nodiscard]] sol::state& lua_state() { return *lua_; }

    /// Get error message for last failed operation
    [[nodiscard]] std::string_view last_error() const { return last_error_; }

    /// Script execution timeout settings
    static constexpr int DEFAULT_MAX_INSTRUCTIONS = 100000;  // ~100K instructions
    static constexpr int HOOK_INTERVAL = 1000;  // Check every 1000 instructions

    /// Set maximum instructions before timeout (0 = no limit)
    void set_max_instructions(int max) { max_instructions_ = max; }
    [[nodiscard]] int max_instructions() const { return max_instructions_; }

    /// Install timeout hook on a Lua state
    void install_timeout_hook(lua_State* L);

    /// Remove timeout hook from a Lua state
    void remove_timeout_hook(lua_State* L);

    /// Reset instruction counter (call before each script execution)
    void reset_instruction_counter() { instruction_count_ = 0; timed_out_ = false; }

    /// Check if last execution timed out
    [[nodiscard]] bool timed_out() const { return timed_out_; }

    // Delete copy/move operations (singleton)
    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;
    ScriptEngine(ScriptEngine&&) = delete;
    ScriptEngine& operator=(ScriptEngine&&) = delete;

private:
    ScriptEngine() = default;
    ~ScriptEngine();

    /// Configure sandbox by removing dangerous functions
    void configure_sandbox();

    /// Register all game API bindings
    void register_bindings();

    /// Register utility functions (dice, random, wait, etc.)
    void register_utility_functions();

    /// Register Effect table for typo-safe effect name lookups
    void register_effect_table();

    std::unique_ptr<sol::state> lua_;  // unique_ptr for explicit destruction during shutdown
    std::unordered_map<std::string, sol::bytecode> bytecode_cache_;
    std::unordered_map<std::string, std::string> failed_script_cache_;  // cache_key -> error message
    std::string last_error_;
    bool initialized_ = false;

    // Timeout protection
    int max_instructions_ = DEFAULT_MAX_INSTRUCTIONS;
    std::atomic<int> instruction_count_{0};
    std::atomic<bool> timed_out_{false};

    /// Lua debug hook callback for timeout protection
    static void timeout_hook(lua_State* L, lua_Debug* ar);
};

// Template implementation
template<typename... Args>
ScriptResult<sol::protected_function_result> ScriptEngine::safe_call(
    sol::protected_function& func, Args&&... args)
{
    if (!initialized_) {
        last_error_ = "ScriptEngine not initialized";
        return std::unexpected(ScriptError::NotInitialized);
    }

    auto result = func(std::forward<Args>(args)...);

    if (!result.valid()) {
        sol::error err = result;
        last_error_ = err.what();
        spdlog::error("Lua execution error: {}", last_error_);
        return std::unexpected(ScriptError::ExecutionFailed);
    }

    return result;
}

} // namespace FieryMUD
