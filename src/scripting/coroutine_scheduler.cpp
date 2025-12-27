// coroutine_scheduler.cpp - Lua coroutine scheduling implementation

#include "coroutine_scheduler.hpp"
#include "../core/logging.hpp"

#include <algorithm>

namespace FieryMUD {

// Global instance
static CoroutineScheduler g_scheduler;

CoroutineScheduler& get_coroutine_scheduler() {
    return g_scheduler;
}

void CoroutineScheduler::initialize(asio::io_context& io_context,
                                     asio::strand<asio::io_context::executor_type>& strand) {
    if (initialized_) {
        Log::warn("CoroutineScheduler already initialized");
        return;
    }

    io_context_ = &io_context;
    strand_ = &strand;
    initialized_ = true;

    Log::info("CoroutineScheduler initialized");
}

void CoroutineScheduler::shutdown() {
    if (!initialized_) {
        return;
    }

    // Cancel all pending coroutines
    for (auto& [id, pending] : pending_) {
        if (pending.timer) {
            pending.timer->cancel();
        }
        total_cancelled_++;
    }
    pending_.clear();

    initialized_ = false;
    io_context_ = nullptr;
    strand_ = nullptr;

    Log::info("CoroutineScheduler shutdown - scheduled: {}, completed: {}, cancelled: {}",
              total_scheduled_, total_completed_, total_cancelled_);
}

std::uint64_t CoroutineScheduler::schedule_wait(sol::thread thread, sol::coroutine coroutine,
                                                 ScriptContext context, EntityId owner_id,
                                                 double delay_seconds) {
    if (!initialized_) {
        Log::error("CoroutineScheduler not initialized");
        return 0;
    }

    // Clamp delay to valid range
    delay_seconds = std::clamp(delay_seconds, MIN_DELAY_SECONDS, MAX_DELAY_SECONDS);

    // Create timer
    auto timer = std::make_shared<asio::steady_timer>(
        *io_context_,
        std::chrono::milliseconds(static_cast<long>(delay_seconds * 1000))
    );

    // Generate ID
    std::uint64_t id = next_id_++;
    total_scheduled_++;

    // Store pending coroutine
    pending_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(id, std::move(thread), std::move(coroutine),
                              std::move(context), timer, owner_id)
    );

    // Set up timer callback - posts to strand for thread safety
    timer->async_wait([this, id](const asio::error_code& ec) {
        if (ec) {
            if (ec != asio::error::operation_aborted) {
                Log::error("Coroutine timer error: {}", ec.message());
            }
            return;
        }

        // Post resume to world strand for thread-safe execution
        asio::post(*strand_, [this, id]() {
            resume_coroutine(id);
        });
    });

    Log::debug("Scheduled coroutine {} for {} seconds (owner: {}:{})",
               id, delay_seconds, owner_id.zone_id(), owner_id.local_id());

    return id;
}

void CoroutineScheduler::resume_coroutine(std::uint64_t coroutine_id) {
    auto it = pending_.find(coroutine_id);
    if (it == pending_.end()) {
        // Already cancelled or completed
        return;
    }

    auto& pending = it->second;

    Log::debug("Resuming coroutine {} (owner: {}:{})",
               coroutine_id, pending.owner_id.zone_id(), pending.owner_id.local_id());

    // Check coroutine status
    sol::state_view lua(pending.thread.lua_state());
    auto status = pending.coroutine.status();

    if (status != sol::call_status::yielded) {
        Log::warn("Coroutine {} not in yielded state (status: {})", coroutine_id,
                  static_cast<int>(status));
        remove_coroutine(coroutine_id);
        return;
    }

    // Resume the coroutine
    auto result = pending.coroutine();

    if (!result.valid()) {
        sol::error err = result;
        Log::error("Coroutine {} error on resume: {}", coroutine_id, err.what());
        remove_coroutine(coroutine_id);
        total_completed_++;
        return;
    }

    // Check if coroutine completed or yielded again
    status = pending.coroutine.status();
    if (status == sol::call_status::yielded) {
        // Coroutine yielded again - wait() was called again
        // The wait() function will reschedule this coroutine
        Log::debug("Coroutine {} yielded again", coroutine_id);
    } else {
        // Coroutine completed
        Log::debug("Coroutine {} completed", coroutine_id);
        remove_coroutine(coroutine_id);
        total_completed_++;
    }
}

void CoroutineScheduler::remove_coroutine(std::uint64_t coroutine_id) {
    auto it = pending_.find(coroutine_id);
    if (it != pending_.end()) {
        if (it->second.timer) {
            it->second.timer->cancel();
        }
        pending_.erase(it);
    }
}

std::size_t CoroutineScheduler::cancel_for_entity(EntityId entity_id) {
    std::size_t cancelled = 0;

    // Find all coroutines for this entity
    std::vector<std::uint64_t> to_cancel;
    for (const auto& [id, pending] : pending_) {
        if (pending.owner_id == entity_id) {
            to_cancel.push_back(id);
        }
    }

    // Cancel them
    for (auto id : to_cancel) {
        if (cancel_coroutine(id)) {
            cancelled++;
        }
    }

    if (cancelled > 0) {
        Log::debug("Cancelled {} coroutines for entity {}:{}",
                   cancelled, entity_id.zone_id(), entity_id.local_id());
    }

    return cancelled;
}

bool CoroutineScheduler::cancel_coroutine(std::uint64_t coroutine_id) {
    auto it = pending_.find(coroutine_id);
    if (it == pending_.end()) {
        return false;
    }

    if (it->second.timer) {
        it->second.timer->cancel();
    }
    pending_.erase(it);
    total_cancelled_++;

    return true;
}

std::size_t CoroutineScheduler::pending_count_for(EntityId entity_id) const {
    std::size_t count = 0;
    for (const auto& [id, pending] : pending_) {
        if (pending.owner_id == entity_id) {
            count++;
        }
    }
    return count;
}

} // namespace FieryMUD
