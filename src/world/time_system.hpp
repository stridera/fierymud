#pragma once

#include "../core/result.hpp"
#include "weather.hpp"
#include <string_view>
#include <chrono>

// Silence spurious warnings in <functional> header
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <functional>
#pragma GCC diagnostic pop

// Time constants matching legacy behavior
namespace TimeConstants {
    constexpr int HOURS_PER_DAY = 24;
    constexpr int DAYS_PER_WEEK = 7;
    constexpr int WEEKS_PER_MONTH = 4;
    constexpr int MONTHS_PER_YEAR = 16;
    constexpr int DAYS_PER_MONTH = DAYS_PER_WEEK * WEEKS_PER_MONTH;
    constexpr int WEEKS_PER_YEAR = WEEKS_PER_MONTH * MONTHS_PER_YEAR;
    constexpr int DAYS_PER_YEAR = DAYS_PER_MONTH * MONTHS_PER_YEAR;

    // Real-time mapping: 1 game hour = 75 real seconds
    constexpr int SECS_PER_MUD_HOUR = 75;
    constexpr int SECS_PER_MUD_DAY = HOURS_PER_DAY * SECS_PER_MUD_HOUR;
    constexpr int SECS_PER_MUD_MONTH = DAYS_PER_MONTH * SECS_PER_MUD_DAY;
    constexpr int SECS_PER_MUD_YEAR = MONTHS_PER_YEAR * SECS_PER_MUD_MONTH;

    // Calendar epoch (matches legacy: year 0 = 1000 AD in-game lore)
    constexpr int EPOCH_YEAR = 0;
}

// Sunlight states for day/night cycle
enum class SunlightState {
    Dark,      // Nighttime - no natural light
    Rise,      // Sunrise - light increasing
    Light,     // Daytime - full natural light
    Set        // Sunset - light decreasing
};

// Hemisphere types for regional day/night variance
enum class Hemisphere {
    Northeast,  // Standard cycle
    Northwest,  // Offset cycle (opposite NE)
    Southeast,  // Standard cycle (same as NE)
    Southwest   // Offset cycle (same as NW)
};

// Month names for in-game calendar
enum class Month {
    Deepwinter = 0,   // Winter months
    TheClaw,
    TheGrandStruggle,
    TheRunning,
    ThePlanting,      // Spring months
    TheLongDay,
    TheTimeFamine,
    TheHighSun,       // Summer months
    TheRipening,
    TheLowering,
    TheFade,
    TheDying,         // Autumn months
    TheShadows,
    TheGreatFrost,
    TheDrawing,
    TheLongNight      // Back to winter
};

// Game time structure
struct GameTime {
    int hour{0};      // 0-23
    int day{0};       // 0-27 (28 days per month)
    Month month{Month::Deepwinter};  // 0-15
    int year{TimeConstants::EPOCH_YEAR};

    // Calculate total hours since epoch
    [[nodiscard]] constexpr int64_t total_hours() const noexcept {
        return static_cast<int64_t>(year) * TimeConstants::DAYS_PER_YEAR * TimeConstants::HOURS_PER_DAY
             + static_cast<int>(month) * TimeConstants::DAYS_PER_MONTH * TimeConstants::HOURS_PER_DAY
             + day * TimeConstants::HOURS_PER_DAY
             + hour;
    }

    // Get season based on month
    [[nodiscard]] Season get_season() const noexcept;

    // Get month name as string
    [[nodiscard]] std::string_view month_name() const noexcept;

    // Check if time is in a specific season
    [[nodiscard]] bool is_season(Season season) const noexcept;
};

// Time change callback signature
using TimeChangeCallback = std::function<void(const GameTime& old_time, const GameTime& new_time)>;

// Global game time manager (singleton)
class TimeSystem {
public:
    static TimeSystem& instance();

    // Prevent copying
    TimeSystem(const TimeSystem&) = delete;
    TimeSystem& operator=(const TimeSystem&) = delete;

    // Initialize time system with starting time
    Result<void> initialize(const GameTime& start_time = GameTime{});

    // Shutdown and cleanup
    void shutdown();

    // Advance game time by one hour
    void advance_hour();

    // Update time based on elapsed real time
    void update(std::chrono::milliseconds elapsed_real_time);

    // Get current game time
    [[nodiscard]] const GameTime& current_time() const noexcept { return current_time_; }

    // Get current sunlight state for a hemisphere
    [[nodiscard]] SunlightState get_sunlight(Hemisphere hemisphere) const noexcept;

    // Get current season
    [[nodiscard]] Season get_season() const noexcept { return current_time_.get_season(); }

    // Check if it's currently daytime in a hemisphere
    [[nodiscard]] bool is_daytime(Hemisphere hemisphere) const noexcept;

    // Check if it's currently nighttime in a hemisphere
    [[nodiscard]] bool is_nighttime(Hemisphere hemisphere) const noexcept;

    // Register callback for time changes (hour, day, month, year)
    void on_hour_changed(TimeChangeCallback callback);
    void on_day_changed(TimeChangeCallback callback);
    void on_month_changed(TimeChangeCallback callback);
    void on_year_changed(TimeChangeCallback callback);

    // Register callback for sunlight changes
    void on_sunlight_changed(std::function<void(SunlightState old_state, SunlightState new_state, Hemisphere hemisphere)> callback);

    // Format time as readable string
    [[nodiscard]] std::string format_time() const;
    [[nodiscard]] std::string format_date() const;

    // Get time since last update (for interpolation)
    [[nodiscard]] std::chrono::milliseconds time_since_last_update() const noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - last_update_time_
        );
    }

private:
    TimeSystem() = default;

    // Calculate sunlight state for a hemisphere at a given hour
    [[nodiscard]] static SunlightState calculate_sunlight(int hour, Hemisphere hemisphere) noexcept;

    // Trigger callbacks
    void trigger_hour_callbacks(const GameTime& old_time);
    void trigger_day_callbacks(const GameTime& old_time);
    void trigger_month_callbacks(const GameTime& old_time);
    void trigger_year_callbacks(const GameTime& old_time);
    void trigger_sunlight_callbacks();

    GameTime current_time_;
    std::chrono::steady_clock::time_point last_update_time_;
    std::chrono::milliseconds accumulated_time_{0};

    // Sunlight state cache for each hemisphere
    SunlightState sunlight_ne_{SunlightState::Dark};
    SunlightState sunlight_nw_{SunlightState::Dark};
    SunlightState sunlight_se_{SunlightState::Dark};
    SunlightState sunlight_sw_{SunlightState::Dark};

    // Callbacks
    std::vector<TimeChangeCallback> hour_callbacks_;
    std::vector<TimeChangeCallback> day_callbacks_;
    std::vector<TimeChangeCallback> month_callbacks_;
    std::vector<TimeChangeCallback> year_callbacks_;
    std::vector<std::function<void(SunlightState, SunlightState, Hemisphere)>> sunlight_callbacks_;

    bool initialized_{false};
};
