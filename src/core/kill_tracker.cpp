#include "kill_tracker.hpp"

#include <algorithm>
#include <cmath>

#include <nlohmann/json.hpp>

namespace fiery {

// Decay formula: weight * 0.5^(hours_elapsed / half_life)
static double compute_decay(double weight, std::chrono::system_clock::time_point last_kill,
                            std::chrono::system_clock::time_point now, double half_life_hours) {
    if (weight <= 0.0)
        return 0.0;
    auto elapsed = std::chrono::duration<double, std::ratio<3600>>(now - last_kill);
    if (elapsed.count() <= 0.0)
        return weight;
    return weight * std::pow(0.5, elapsed.count() / half_life_hours);
}

double KillWeight::decayed() const { return decayed_at(std::chrono::system_clock::now()); }

double KillWeight::decayed_at(std::chrono::system_clock::time_point now) const {
    return compute_decay(weight, last_kill, now, KILL_HALF_LIFE_HOURS);
}

double ZoneActivity::decayed() const { return decayed_at(std::chrono::system_clock::now()); }

double ZoneActivity::decayed_at(std::chrono::system_clock::time_point now) const {
    return compute_decay(weight, last_kill, now, ZONE_HALF_LIFE_HOURS);
}

void KillTracker::record_kill(EntityId mob_id, int zone_id, double share) {
    auto now = std::chrono::system_clock::now();
    double added = std::max(0.0, std::min(1.0, share));

    // Update mob kill weight (decay existing, then add)
    auto &kill = mob_kills_[mob_id];
    kill.weight = kill.decayed_at(now) + added;
    kill.last_kill = now;

    // Update zone activity weight (decay existing, then add)
    auto &zone = zone_activity_[zone_id];
    zone.weight = zone.decayed_at(now) + added;
    zone.last_kill = now;
}

double KillTracker::xp_modifier(EntityId mob_id, int zone_id) const {
    return mob_multiplier(mob_id) * (1.0 + zone_bonus(zone_id));
}

double KillTracker::mob_multiplier(EntityId mob_id) const {
    auto it = mob_kills_.find(mob_id);
    if (it == mob_kills_.end())
        return 1.0;
    double w = it->second.decayed();
    double effective = std::max(0.0, w - MOB_FREE_KILLS);
    return std::max(MIN_MOB_MULTIPLIER, 1.0 / (1.0 + effective / MOB_PENALTY_SCALE));
}

double KillTracker::zone_bonus(int zone_id) const {
    auto it = zone_activity_.find(zone_id);
    if (it == zone_activity_.end())
        return MAX_DIVERSITY_BONUS; // Never been here — full bonus
    double w = it->second.decayed();
    return MAX_DIVERSITY_BONUS * std::max(0.0, 1.0 - w / DIVERSITY_THRESHOLD);
}

std::vector<TrophyEntry> KillTracker::get_trophy_entries() const {
    std::vector<TrophyEntry> entries;
    entries.reserve(mob_kills_.size());
    for (const auto &[id, kw] : mob_kills_) {
        double w = kw.decayed();
        if (w < PRUNE_THRESHOLD)
            continue;
        double effective = std::max(0.0, w - MOB_FREE_KILLS);
        double mult = std::max(MIN_MOB_MULTIPLIER, 1.0 / (1.0 + effective / MOB_PENALTY_SCALE));
        entries.push_back({id, w, mult});
    }
    std::ranges::sort(entries, [](const TrophyEntry &a, const TrophyEntry &b) { return a.weight > b.weight; });
    return entries;
}

std::vector<ZoneTrophyEntry> KillTracker::get_zone_entries() const {
    std::vector<ZoneTrophyEntry> entries;
    entries.reserve(zone_activity_.size());
    for (const auto &[zid, za] : zone_activity_) {
        double w = za.decayed();
        if (w < PRUNE_THRESHOLD)
            continue;
        double bonus = MAX_DIVERSITY_BONUS * std::max(0.0, 1.0 - w / DIVERSITY_THRESHOLD);
        entries.push_back({zid, w, bonus});
    }
    std::ranges::sort(entries, [](const ZoneTrophyEntry &a, const ZoneTrophyEntry &b) { return a.weight > b.weight; });
    return entries;
}

nlohmann::json KillTracker::to_json() const {
    auto now = std::chrono::system_clock::now();
    auto epoch_seconds = [](std::chrono::system_clock::time_point tp) {
        return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
    };

    nlohmann::json kills = nlohmann::json::array();
    for (const auto &[id, kw] : mob_kills_) {
        double w = kw.decayed_at(now);
        if (w < PRUNE_THRESHOLD)
            continue;
        kills.push_back({{"z", id.zone_id()}, {"id", id.local_id()}, {"w", w}, {"t", epoch_seconds(now)}});
    }

    nlohmann::json zones = nlohmann::json::array();
    for (const auto &[zid, za] : zone_activity_) {
        double w = za.decayed_at(now);
        if (w < PRUNE_THRESHOLD)
            continue;
        zones.push_back({{"z", zid}, {"w", w}, {"t", epoch_seconds(now)}});
    }

    return {{"kills", kills}, {"zones", zones}};
}

KillTracker KillTracker::from_json(const nlohmann::json &json) {
    KillTracker tracker;

    auto tp_from_epoch = [](int64_t seconds) {
        return std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
    };

    if (json.contains("kills") && json["kills"].is_array()) {
        for (const auto &entry : json["kills"]) {
            auto zone = entry.value("z", 0u);
            auto local = entry.value("id", 0u);
            double w = entry.value("w", 0.0);
            int64_t t = entry.value("t", int64_t{0});

            EntityId mob_id(zone, local);
            tracker.mob_kills_[mob_id] = {w, tp_from_epoch(t)};
        }
    }

    if (json.contains("zones") && json["zones"].is_array()) {
        for (const auto &entry : json["zones"]) {
            int zid = entry.value("z", 0);
            double w = entry.value("w", 0.0);
            int64_t t = entry.value("t", int64_t{0});

            tracker.zone_activity_[zid] = {w, tp_from_epoch(t)};
        }
    }

    return tracker;
}

} // namespace fiery
