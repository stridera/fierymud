#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "core/ids.hpp"

namespace fiery {

/** Constants for kill tracking decay and bonuses */
constexpr double KILL_HALF_LIFE_HOURS = 2.0;
constexpr double ZONE_HALF_LIFE_HOURS = 4.0;
constexpr double MAX_DIVERSITY_BONUS = 0.15;
constexpr double DIVERSITY_THRESHOLD = 0.5;
constexpr double PRUNE_THRESHOLD = 0.01;
constexpr double MOB_FREE_KILLS = 1.0;     // First kill has no penalty (like legacy 0-2 range)
constexpr double MOB_PENALTY_SCALE = 5.0;  // Controls how fast penalties ramp up
constexpr double MIN_MOB_MULTIPLIER = 0.3; // Floor: always get at least 30% XP (matches legacy)

/** Time-decaying weight for a single mob prototype's kill history */
struct KillWeight {
    double weight = 0.0;
    std::chrono::system_clock::time_point last_kill{};

    /** Get decayed weight at current time */
    double decayed() const;

    /** Get decayed weight at a specific time */
    double decayed_at(std::chrono::system_clock::time_point now) const;
};

/** Time-decaying activity weight for a zone */
struct ZoneActivity {
    double weight = 0.0;
    std::chrono::system_clock::time_point last_kill{};

    /** Get decayed weight at current time */
    double decayed() const;

    /** Get decayed weight at a specific time */
    double decayed_at(std::chrono::system_clock::time_point now) const;
};

/** Entry for trophy display */
struct TrophyEntry {
    EntityId mob_id;
    double weight;
    double multiplier;
};

/** Entry for zone diversity display */
struct ZoneTrophyEntry {
    int zone_id;
    double weight;
    double bonus;
};

/**
 * Tracks kill history per player for XP diminishing returns and zone diversity bonus.
 *
 * - Each kill of a mob prototype adds weight (1.0 solo, 1.0/N for groups)
 * - Weight decays exponentially with a 2-hour half-life
 * - XP modifier: max(0.3, 1/(1 + max(0, w-1)/5)) — legacy-matched diminishing returns
 * - Zone diversity: up to +15% bonus for killing in zones with low recent activity
 * - No tick processing — decay is computed lazily on access
 */
class KillTracker {
  public:
    /** Record a kill of a mob prototype.
     *  @param mob_id   Prototype EntityId of the killed mob
     *  @param zone_id  Zone number where the kill occurred
     *  @param share    Kill share (1.0 solo, 1.0/N for group of N)
     */
    void record_kill(EntityId mob_id, int zone_id, double share = 1.0);

    /** Get the combined XP modifier for killing a mob in a zone.
     *  Returns mob_multiplier * (1.0 + zone_bonus).
     */
    double xp_modifier(EntityId mob_id, int zone_id) const;

    /** Get the mob-specific diminishing returns multiplier.
     *  Returns max(0.3, 1/(1 + max(0, w - 1) / 5)).
     *  First kill is free, then gradual decline to 30% floor.
     */
    double mob_multiplier(EntityId mob_id) const;

    /** Get the zone diversity bonus (0.0 to MAX_DIVERSITY_BONUS). */
    double zone_bonus(int zone_id) const;

    /** Get sorted trophy entries for display (highest weight first). */
    std::vector<TrophyEntry> get_trophy_entries() const;

    /** Get sorted zone entries for display (highest weight first). */
    std::vector<ZoneTrophyEntry> get_zone_entries() const;

    /** Check if tracker has any data. */
    bool empty() const { return mob_kills_.empty() && zone_activity_.empty(); }

    /** Serialize to JSON. */
    nlohmann::json to_json() const;

    /** Deserialize from JSON. */
    static KillTracker from_json(const nlohmann::json &json);

  private:
    std::unordered_map<EntityId, KillWeight, EntityId::Hash> mob_kills_;
    std::unordered_map<int, ZoneActivity> zone_activity_;
};

} // namespace fiery
