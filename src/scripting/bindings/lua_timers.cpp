#define SOL_ALL_SAFETIES_ON 1
#include "lua_timers.hpp"

#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

#include "../script_timer_manager.hpp"

namespace FieryMUD {

void register_timer_bindings(sol::state &lua) {
    auto timers_table = lua.create_named_table("timers");

    // timers.after(seconds, callback) - Execute callback after delay
    // Returns: (timer_id, string? error)
    // The timer_id can be used to cancel the timer later
    timers_table["after"] = [&lua](double seconds, sol::protected_function callback)
        -> std::tuple<sol::optional<std::uint64_t>, sol::optional<std::string>> {
        if (!callback.valid()) {
            return std::make_tuple(sol::optional<std::uint64_t>{}, std::string("invalid_callback"));
        }

        // Clamp delay to reasonable bounds
        constexpr double MIN_DELAY = 0.1;
        constexpr double MAX_DELAY = 3600.0; // 1 hour max
        seconds = std::clamp(seconds, MIN_DELAY, MAX_DELAY);

        auto &timer_manager = ScriptTimerManager::instance();
        auto result = timer_manager.schedule(seconds, std::move(callback));

        if (!result) {
            return std::make_tuple(sol::optional<std::uint64_t>{}, result.error().message);
        }

        spdlog::debug("timers.after: scheduled timer {} for {} seconds", *result, seconds);
        return std::make_tuple(sol::optional<std::uint64_t>{*result}, sol::optional<std::string>{});
    };

    // timers.cancel(timer_id) - Cancel a pending timer
    // Returns: bool (true if cancelled, false if not found)
    timers_table["cancel"] = [](std::uint64_t timer_id) -> bool {
        auto &timer_manager = ScriptTimerManager::instance();
        bool cancelled = timer_manager.cancel(timer_id);

        if (cancelled) {
            spdlog::debug("timers.cancel: cancelled timer {}", timer_id);
        }

        return cancelled;
    };

    spdlog::debug("Registered timers Lua bindings");
}

} // namespace FieryMUD
