#define SOL_ALL_SAFETIES_ON 1
#include "script_timer_manager.hpp"

#include <vector>

#include <sol/sol.hpp>

#include "core/logging.hpp"

namespace FieryMUD {

ScriptTimerManager &ScriptTimerManager::instance() {
    static ScriptTimerManager instance;
    return instance;
}

std::expected<std::uint64_t, TimerError> ScriptTimerManager::schedule(double delay_seconds,
                                                                      sol::protected_function callback) {

    if (delay_seconds < 0) {
        return std::unexpected(TimerError{"Delay must be non-negative"});
    }

    if (!callback.valid()) {
        return std::unexpected(TimerError{"Invalid callback function"});
    }

    std::lock_guard<std::mutex> lock(mutex_);

    std::uint64_t timer_id = next_timer_id_++;

    TimerEntry entry;
    entry.id = timer_id;
    entry.expires_at =
        std::chrono::steady_clock::now() +
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(delay_seconds));
    entry.callback = std::move(callback);

    timers_[timer_id] = std::move(entry);

    Log::debug("Timer scheduled: id={}, delay={}s, pending={}", timer_id, delay_seconds, timers_.size());

    return timer_id;
}

bool ScriptTimerManager::cancel(std::uint64_t timer_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = timers_.find(timer_id);
    if (it == timers_.end()) {
        return false;
    }

    timers_.erase(it);
    Log::debug("Timer cancelled: id={}", timer_id);
    return true;
}

void ScriptTimerManager::tick() {
    // Collect expired timers
    std::vector<TimerEntry> expired;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::steady_clock::now();
        std::vector<std::uint64_t> to_remove;

        for (auto &[id, entry] : timers_) {
            if (entry.expires_at <= now) {
                expired.push_back(std::move(entry));
                to_remove.push_back(id);
            }
        }

        // Remove expired timers
        for (auto id : to_remove) {
            timers_.erase(id);
        }
    }

    // Execute callbacks outside the lock
    for (auto &entry : expired) {
        try {
            auto result = entry.callback();
            if (!result.valid()) {
                sol::error err = result;
                Log::error("Timer callback error (id={}): {}", entry.id, err.what());
            }
        } catch (const std::exception &e) {
            Log::error("Timer callback exception (id={}): {}", entry.id, e.what());
        }
    }

    if (!expired.empty()) {
        Log::debug("Executed {} timers, {} remaining", expired.size(), pending_count());
    }
}

size_t ScriptTimerManager::pending_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timers_.size();
}

void ScriptTimerManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    timers_.clear();
    Log::debug("All timers cleared");
}

} // namespace FieryMUD
