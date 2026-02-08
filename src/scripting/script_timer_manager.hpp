#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <functional>
#include <map>
#include <mutex>
#include <string>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace FieryMUD {

/**
 * Error information for timer operations.
 */
struct TimerError {
    std::string message;
};

/**
 * ScriptTimerManager handles non-blocking delayed execution for Lua scripts.
 *
 * Timers are scheduled with a delay and executed when tick() is called
 * from the game loop.
 */
class ScriptTimerManager {
  public:
    static ScriptTimerManager &instance();

    /**
     * Schedule a callback to execute after a delay.
     * @param delay_seconds Time to wait before executing
     * @param callback Lua function to call
     * @return Timer ID for cancellation, or error
     */
    std::expected<std::uint64_t, TimerError> schedule(double delay_seconds, sol::protected_function callback);

    /**
     * Cancel a pending timer.
     * @param timer_id ID returned from schedule()
     * @return true if cancelled, false if not found
     */
    bool cancel(std::uint64_t timer_id);

    /**
     * Process pending timers (called from game loop).
     * Executes all timers whose expiration time has passed.
     */
    void tick();

    /**
     * Get count of pending timers.
     */
    size_t pending_count() const;

    /**
     * Clear all pending timers.
     */
    void clear();

  private:
    ScriptTimerManager() = default;

    struct TimerEntry {
        std::uint64_t id;
        std::chrono::steady_clock::time_point expires_at;
        sol::protected_function callback;
    };

    std::uint64_t next_timer_id_ = 1;
    std::map<std::uint64_t, TimerEntry> timers_; // id -> timer
    mutable std::mutex mutex_;
};

} // namespace FieryMUD
