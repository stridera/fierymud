#include "time_system.hpp"
#include "../core/logging.hpp"
#include <fmt/format.h>

using namespace TimeConstants;

// Month names mapping
constexpr std::string_view MONTH_NAMES[] = {
    "the Month of Deepwinter",      // 0
    "the Month of the Claw",         // 1
    "the Month of the Grand Struggle", // 2
    "the Month of the Running",      // 3
    "the Month of the Planting",     // 4
    "the Month of the Long Day",     // 5
    "the Month of the Time of Famine", // 6
    "the Month of the High Sun",     // 7
    "the Month of the Ripening",     // 8
    "the Month of the Lowering",     // 9
    "the Month of the Fade",         // 10
    "the Month of the Dying",        // 11
    "the Month of the Shadows",      // 12
    "the Month of the Great Frost",  // 13
    "the Month of the Drawing",      // 14
    "the Month of the Long Night"    // 15
};

// GameTime methods
Season GameTime::get_season() const noexcept {
    int m = static_cast<int>(month);

    // Winter: months 0-3 (Deepwinter, Claw, Grand Struggle, Running)
    if (m >= 0 && m <= 3) return Season::Winter;

    // Spring: months 4-7 (Planting, Long Day, Time of Famine, High Sun)
    if (m >= 4 && m <= 7) return Season::Spring;

    // Summer: months 8-11 (Ripening, Lowering, Fade, Dying)
    if (m >= 8 && m <= 11) return Season::Summer;

    // Autumn: months 12-15 (Shadows, Great Frost, Drawing, Long Night)
    return Season::Autumn;
}

std::string_view GameTime::month_name() const noexcept {
    int m = static_cast<int>(month);
    if (m < 0 || m >= MONTHS_PER_YEAR) {
        return "Invalid Month";
    }
    return MONTH_NAMES[m];
}

bool GameTime::is_season(Season season) const noexcept {
    return get_season() == season;
}

// TimeSystem singleton
TimeSystem& TimeSystem::instance() {
    static TimeSystem instance;
    return instance;
}

Result<void> TimeSystem::initialize(const GameTime& start_time) {
    if (initialized_) {
        return std::unexpected(Errors::InvalidState("TimeSystem already initialized"));
    }

    current_time_ = start_time;
    last_update_time_ = std::chrono::steady_clock::now();
    accumulated_time_ = std::chrono::milliseconds{0};

    // Calculate initial sunlight states
    sunlight_ne_ = calculate_sunlight(current_time_.hour, Hemisphere::Northeast);
    sunlight_nw_ = calculate_sunlight(current_time_.hour, Hemisphere::Northwest);
    sunlight_se_ = calculate_sunlight(current_time_.hour, Hemisphere::Southeast);
    sunlight_sw_ = calculate_sunlight(current_time_.hour, Hemisphere::Southwest);

    initialized_ = true;

    auto logger = Log::game();
    logger->info("TimeSystem initialized: {} (Year {})", format_date(), current_time_.year);
    logger->info("Starting time: Hour {}, {}", current_time_.hour, format_time());

    return Success();
}

void TimeSystem::shutdown() {
    if (!initialized_) {
        return;
    }

    hour_callbacks_.clear();
    day_callbacks_.clear();
    month_callbacks_.clear();
    year_callbacks_.clear();
    sunlight_callbacks_.clear();

    initialized_ = false;

    auto logger = Log::game();
    logger->info("TimeSystem shutdown");
}

void TimeSystem::advance_hour() {
    if (!initialized_) {
        return;
    }

    GameTime old_time = current_time_;

    // Store old sunlight states
    SunlightState old_ne = sunlight_ne_;
    SunlightState old_nw = sunlight_nw_;
    SunlightState old_se = sunlight_se_;
    SunlightState old_sw = sunlight_sw_;

    // Advance hour
    current_time_.hour++;

    // Handle day rollover
    if (current_time_.hour >= HOURS_PER_DAY) {
        current_time_.hour = 0;
        current_time_.day++;

        // Handle month rollover
        if (current_time_.day >= DAYS_PER_MONTH) {
            current_time_.day = 0;
            int month_val = static_cast<int>(current_time_.month) + 1;

            // Handle year rollover
            if (month_val >= MONTHS_PER_YEAR) {
                month_val = 0;
                current_time_.year++;
                trigger_year_callbacks(old_time);
            }

            current_time_.month = static_cast<Month>(month_val);
            trigger_month_callbacks(old_time);
        }

        trigger_day_callbacks(old_time);
    }

    // Update sunlight states
    sunlight_ne_ = calculate_sunlight(current_time_.hour, Hemisphere::Northeast);
    sunlight_nw_ = calculate_sunlight(current_time_.hour, Hemisphere::Northwest);
    sunlight_se_ = calculate_sunlight(current_time_.hour, Hemisphere::Southeast);
    sunlight_sw_ = calculate_sunlight(current_time_.hour, Hemisphere::Southwest);

    // Trigger callbacks
    trigger_hour_callbacks(old_time);

    // Check for sunlight changes
    if (sunlight_ne_ != old_ne) {
        for (auto& callback : sunlight_callbacks_) {
            callback(old_ne, sunlight_ne_, Hemisphere::Northeast);
        }
    }
    if (sunlight_nw_ != old_nw) {
        for (auto& callback : sunlight_callbacks_) {
            callback(old_nw, sunlight_nw_, Hemisphere::Northwest);
        }
    }
    if (sunlight_se_ != old_se) {
        for (auto& callback : sunlight_callbacks_) {
            callback(old_se, sunlight_se_, Hemisphere::Southeast);
        }
    }
    if (sunlight_sw_ != old_sw) {
        for (auto& callback : sunlight_callbacks_) {
            callback(old_sw, sunlight_sw_, Hemisphere::Southwest);
        }
    }
}

void TimeSystem::update(std::chrono::milliseconds elapsed_real_time) {
    if (!initialized_) {
        return;
    }

    accumulated_time_ += elapsed_real_time;
    last_update_time_ = std::chrono::steady_clock::now();

    // Convert real time to game time (1 game hour = 75 real seconds)
    constexpr auto HOUR_DURATION = std::chrono::seconds{SECS_PER_MUD_HOUR};

    while (accumulated_time_ >= HOUR_DURATION) {
        advance_hour();
        accumulated_time_ -= HOUR_DURATION;
    }
}

SunlightState TimeSystem::get_sunlight(Hemisphere hemisphere) const noexcept {
    switch (hemisphere) {
        case Hemisphere::Northeast: return sunlight_ne_;
        case Hemisphere::Northwest: return sunlight_nw_;
        case Hemisphere::Southeast: return sunlight_se_;
        case Hemisphere::Southwest: return sunlight_sw_;
    }
    return SunlightState::Dark;
}

bool TimeSystem::is_daytime(Hemisphere hemisphere) const noexcept {
    SunlightState state = get_sunlight(hemisphere);
    return state == SunlightState::Light || state == SunlightState::Rise;
}

bool TimeSystem::is_nighttime(Hemisphere hemisphere) const noexcept {
    SunlightState state = get_sunlight(hemisphere);
    return state == SunlightState::Dark || state == SunlightState::Set;
}

SunlightState TimeSystem::calculate_sunlight(int hour, Hemisphere hemisphere) noexcept {
    // Northeast and Southeast: Standard cycle
    // Hour 6 = sunrise, 7 = day, 19 = sunset, 20 = night
    // Northwest and Southwest: Offset cycle (opposite)
    // Hour 6 = sunset, 7 = night, 19 = sunrise, 20 = day

    bool is_offset = (hemisphere == Hemisphere::Northwest || hemisphere == Hemisphere::Southwest);

    if (!is_offset) {
        // Standard cycle (NE, SE)
        if (hour == 6) return SunlightState::Rise;
        if (hour >= 7 && hour < 19) return SunlightState::Light;
        if (hour == 19) return SunlightState::Set;
        return SunlightState::Dark;
    } else {
        // Offset cycle (NW, SW) - opposite of standard
        if (hour == 6) return SunlightState::Set;
        if (hour >= 7 && hour < 19) return SunlightState::Dark;
        if (hour == 19) return SunlightState::Rise;
        return SunlightState::Light;
    }
}

void TimeSystem::on_hour_changed(TimeChangeCallback callback) {
    hour_callbacks_.push_back(std::move(callback));
}

void TimeSystem::on_day_changed(TimeChangeCallback callback) {
    day_callbacks_.push_back(std::move(callback));
}

void TimeSystem::on_month_changed(TimeChangeCallback callback) {
    month_callbacks_.push_back(std::move(callback));
}

void TimeSystem::on_year_changed(TimeChangeCallback callback) {
    year_callbacks_.push_back(std::move(callback));
}

void TimeSystem::on_sunlight_changed(std::function<void(SunlightState, SunlightState, Hemisphere)> callback) {
    sunlight_callbacks_.push_back(std::move(callback));
}

std::string TimeSystem::format_time() const {
    int display_hour = current_time_.hour % 12;
    if (display_hour == 0) display_hour = 12;

    const char* period = (current_time_.hour < 12) ? "am" : "pm";

    return fmt::format("{}o'clock {}", display_hour, period);
}

std::string TimeSystem::format_date() const {
    return fmt::format("Day {} of {}",
        current_time_.day + 1,  // Display as 1-28 instead of 0-27
        current_time_.month_name());
}

void TimeSystem::trigger_hour_callbacks(const GameTime& old_time) {
    for (auto& callback : hour_callbacks_) {
        callback(old_time, current_time_);
    }
}

void TimeSystem::trigger_day_callbacks(const GameTime& old_time) {
    for (auto& callback : day_callbacks_) {
        callback(old_time, current_time_);
    }
}

void TimeSystem::trigger_month_callbacks(const GameTime& old_time) {
    for (auto& callback : month_callbacks_) {
        callback(old_time, current_time_);
    }
}

void TimeSystem::trigger_year_callbacks(const GameTime& old_time) {
    for (auto& callback : year_callbacks_) {
        callback(old_time, current_time_);
    }
}

void TimeSystem::trigger_sunlight_callbacks() {
    // Called when sunlight changes are detected
    // Individual hemisphere checks are done in advance_hour()
}
