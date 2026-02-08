// coroutine_scheduler.hpp - Lua coroutine scheduling with ASIO timers
//
// Enables wait() in Lua scripts for delayed execution:
//   self:say("Halt!")
//   wait(2)  -- Yields, resumes after 2 seconds
//   self:emote("draws sword.")

#pragma once

#include "script_context.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/forward.hpp>
#include <sol/thread.hpp>
#include <sol/coroutine.hpp>

#include <asio.hpp>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace FieryMUD {

/**
 * Represents a suspended coroutine waiting to be resumed
 */
struct PendingCoroutine {
    std::uint64_t id;                              // Unique ID for this coroutine
    sol::thread thread;                            // Lua thread (coroutine)
    sol::coroutine coroutine;                      // The coroutine function
    ScriptContext context;                         // Execution context
    std::shared_ptr<asio::steady_timer> timer;     // Timer for resume
    EntityId owner_id;                             // Entity that owns this script
    std::chrono::steady_clock::time_point created; // When it was scheduled

    PendingCoroutine(std::uint64_t id_, sol::thread t, sol::coroutine co,
                     ScriptContext ctx, std::shared_ptr<asio::steady_timer> timer_,
                     EntityId owner)
        : id(id_), thread(std::move(t)), coroutine(std::move(co)),
          context(std::move(ctx)), timer(std::move(timer_)), owner_id(owner),
          created(std::chrono::steady_clock::now()) {}
};

/**
 * CoroutineScheduler - Manages Lua script coroutines with timed delays
 *
 * Thread Safety:
 * - All resume operations are posted to the world_strand_ for safe execution
 * - Timer callbacks execute on the io_context and post to strand
 * - Entity death cancellation must be called from the strand
 *
 * Usage:
 * 1. Call schedule_wait() when a script calls wait(seconds)
 * 2. The coroutine yields and a timer is created
 * 3. After the delay, the coroutine resumes on the world strand
 * 4. If the owning entity dies, call cancel_for_entity() to clean up
 */
class CoroutineScheduler {
public:
    /**
     * Initialize the scheduler with ASIO context and strand
     * @param io_context The ASIO io_context for creating timers
     * @param strand The world strand for thread-safe resume
     */
    void initialize(asio::io_context& io_context,
                    asio::strand<asio::io_context::executor_type>& strand);

    /**
     * Shut down the scheduler, canceling all pending coroutines
     */
    void shutdown();

    /**
     * Check if scheduler is initialized
     */
    [[nodiscard]] bool is_initialized() const { return initialized_; }

    /**
     * Schedule a coroutine to resume after a delay
     * @param thread The Lua thread containing the coroutine
     * @param coroutine The coroutine to resume
     * @param context The script execution context
     * @param owner_id Entity ID of the script owner (for cancellation)
     * @param delay_seconds How long to wait before resuming
     * @return The coroutine ID (for tracking/debugging)
     */
    std::uint64_t schedule_wait(sol::thread thread, sol::coroutine coroutine,
                                ScriptContext context, EntityId owner_id,
                                double delay_seconds);

    /**
     * Cancel all pending coroutines for an entity (e.g., when it dies)
     * @param entity_id The entity whose coroutines should be cancelled
     * @return Number of coroutines cancelled
     */
    std::size_t cancel_for_entity(EntityId entity_id);

    /**
     * Cancel a specific coroutine by ID
     * @param coroutine_id The coroutine to cancel
     * @return true if found and cancelled
     */
    bool cancel_coroutine(std::uint64_t coroutine_id);

    /**
     * Get count of pending coroutines
     */
    [[nodiscard]] std::size_t pending_count() const { return pending_.size(); }

    /**
     * Get count of pending coroutines for a specific entity
     */
    [[nodiscard]] std::size_t pending_count_for(EntityId entity_id) const;

    /**
     * Maximum allowed delay in seconds (prevent abuse)
     */
    static constexpr double MAX_DELAY_SECONDS = 300.0; // 5 minutes

    /**
     * Minimum delay in seconds (prevent busy loops)
     */
    static constexpr double MIN_DELAY_SECONDS = 0.1; // 100ms

    /**
     * Maximum pending coroutines (prevent memory exhaustion)
     */
    static constexpr std::size_t MAX_PENDING_COROUTINES = 10000;

    /**
     * Maximum pending coroutines per entity (prevent single script abuse)
     */
    static constexpr std::size_t MAX_PENDING_PER_ENTITY = 100;

private:
    /**
     * Resume a coroutine after its timer fires
     * Called on the world strand
     */
    void resume_coroutine(std::uint64_t coroutine_id);

    /**
     * Remove a coroutine from pending (after completion or cancellation)
     */
    void remove_coroutine(std::uint64_t coroutine_id);

    // State (initialized_ is atomic for thread-safe access from timer callbacks)
    std::atomic<bool> initialized_{false};
    asio::io_context* io_context_ = nullptr;
    asio::strand<asio::io_context::executor_type>* strand_ = nullptr;

    // Pending coroutines keyed by ID
    std::unordered_map<std::uint64_t, PendingCoroutine> pending_;

    // Next coroutine ID
    std::uint64_t next_id_ = 1;

    // Statistics
    std::uint64_t total_scheduled_ = 0;
    std::uint64_t total_completed_ = 0;
    std::uint64_t total_cancelled_ = 0;
};

/**
 * Get the global CoroutineScheduler instance
 */
CoroutineScheduler& get_coroutine_scheduler();

} // namespace FieryMUD
