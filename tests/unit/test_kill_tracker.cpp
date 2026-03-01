#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "../src/core/kill_tracker.hpp"

using Catch::Approx;
using namespace fiery;

TEST_CASE("KillTracker: mob multiplier formula", "[unit][kill_tracker]") {
    SECTION("Fresh mob gives 100% XP") {
        KillTracker tracker;
        EntityId mob(30, 5);
        REQUIRE(tracker.mob_multiplier(mob) == Approx(1.0));
    }

    SECTION("First solo kill is free — still 100% XP") {
        KillTracker tracker;
        EntityId mob(30, 5);
        tracker.record_kill(mob, 30, 1.0);
        // weight=1.0, effective = max(0, 1-1) = 0 -> 1/(1+0) = 1.0
        REQUIRE(tracker.mob_multiplier(mob) == Approx(1.0).margin(0.01));
    }

    SECTION("Two kills give ~83% XP") {
        KillTracker tracker;
        EntityId mob(30, 5);
        tracker.record_kill(mob, 30, 1.0);
        tracker.record_kill(mob, 30, 1.0);
        // weight~2.0, effective = 1.0, mult = 1/(1+1/5) = 1/1.2 = 0.833
        REQUIRE(tracker.mob_multiplier(mob) == Approx(1.0 / 1.2).margin(0.01));
    }

    SECTION("Three kills give ~71% XP") {
        KillTracker tracker;
        EntityId mob(30, 5);
        for (int i = 0; i < 3; i++)
            tracker.record_kill(mob, 30, 1.0);
        // weight~3.0, effective = 2.0, mult = 1/(1+2/5) = 1/1.4 = 0.714
        REQUIRE(tracker.mob_multiplier(mob) == Approx(1.0 / 1.4).margin(0.01));
    }

    SECTION("Five kills give ~56% XP") {
        KillTracker tracker;
        EntityId mob(30, 5);
        for (int i = 0; i < 5; i++)
            tracker.record_kill(mob, 30, 1.0);
        // weight~5.0, effective = 4.0, mult = 1/(1+4/5) = 1/1.8 = 0.556
        REQUIRE(tracker.mob_multiplier(mob) == Approx(1.0 / 1.8).margin(0.01));
    }

    SECTION("Seven kills give ~45% XP (matches legacy)") {
        KillTracker tracker;
        EntityId mob(30, 5);
        for (int i = 0; i < 7; i++)
            tracker.record_kill(mob, 30, 1.0);
        // weight~7.0, effective = 6.0, mult = 1/(1+6/5) = 1/2.2 = 0.455
        REQUIRE(tracker.mob_multiplier(mob) == Approx(1.0 / 2.2).margin(0.01));
    }

    SECTION("Ten kills give ~36% XP") {
        KillTracker tracker;
        EntityId mob(30, 5);
        for (int i = 0; i < 10; i++)
            tracker.record_kill(mob, 30, 1.0);
        // weight~10.0, effective = 9.0, mult = 1/(1+9/5) = 1/2.8 = 0.357
        REQUIRE(tracker.mob_multiplier(mob) == Approx(1.0 / 2.8).margin(0.02));
    }

    SECTION("Extreme farming floors at 30%") {
        KillTracker tracker;
        EntityId mob(30, 5);
        for (int i = 0; i < 50; i++)
            tracker.record_kill(mob, 30, 1.0);
        REQUIRE(tracker.mob_multiplier(mob) == Approx(MIN_MOB_MULTIPLIER).margin(0.01));
    }

    SECTION("Group share reduces weight added") {
        KillTracker tracker;
        EntityId mob(30, 5);
        tracker.record_kill(mob, 30, 0.25); // Group of 4
        // weight=0.25, effective = max(0, 0.25-1) = 0 -> mult = 1.0
        REQUIRE(tracker.mob_multiplier(mob) == Approx(1.0).margin(0.01));
    }

    SECTION("Different mobs are tracked independently") {
        KillTracker tracker;
        EntityId mob1(30, 5);
        EntityId mob2(30, 10);
        tracker.record_kill(mob1, 30, 1.0);
        REQUIRE(tracker.mob_multiplier(mob1) == Approx(1.0).margin(0.01));
        REQUIRE(tracker.mob_multiplier(mob2) == Approx(1.0));
    }
}

TEST_CASE("KillTracker: zone diversity bonus", "[unit][kill_tracker]") {
    SECTION("Unvisited zone gives full bonus") {
        KillTracker tracker;
        REQUIRE(tracker.zone_bonus(30) == Approx(MAX_DIVERSITY_BONUS));
    }

    SECTION("Heavily farmed zone gives no bonus") {
        KillTracker tracker;
        EntityId mob(30, 5);
        tracker.record_kill(mob, 30, 1.0);
        // After 1 kill, zone weight ~= 1.0 which exceeds 0.5 threshold
        REQUIRE(tracker.zone_bonus(30) == Approx(0.0));
    }
}

TEST_CASE("KillTracker: combined XP modifier", "[unit][kill_tracker]") {
    SECTION("Fresh mob in fresh zone gives > 1.0 modifier") {
        KillTracker tracker;
        EntityId mob(30, 5);
        double mod = tracker.xp_modifier(mob, 30);
        // mob_mult=1.0, zone_bonus=0.15 -> 1.0 * 1.15 = 1.15
        REQUIRE(mod == Approx(1.15));
    }

    SECTION("First kill: full mob XP but no zone bonus") {
        KillTracker tracker;
        EntityId mob(30, 5);
        tracker.record_kill(mob, 30, 1.0);
        double mod = tracker.xp_modifier(mob, 30);
        // mob_mult=1.0 (first kill free), zone_bonus=0.0 (weight > threshold) -> 1.0
        REQUIRE(mod == Approx(1.0).margin(0.01));
    }
}

TEST_CASE("KillTracker: trophy entries", "[unit][kill_tracker]") {
    SECTION("Empty tracker returns no entries") {
        KillTracker tracker;
        REQUIRE(tracker.get_trophy_entries().empty());
        REQUIRE(tracker.get_zone_entries().empty());
    }

    SECTION("Entries sorted by weight descending") {
        KillTracker tracker;
        EntityId mob1(30, 5);
        EntityId mob2(30, 10);
        tracker.record_kill(mob1, 30, 1.0);
        tracker.record_kill(mob2, 30, 1.0);
        tracker.record_kill(mob2, 30, 1.0);

        auto entries = tracker.get_trophy_entries();
        REQUIRE(entries.size() == 2);
        REQUIRE(entries[0].mob_id == mob2); // mob2 has higher weight
        REQUIRE(entries[1].mob_id == mob1);
    }

    SECTION("Trophy entries use new formula with floor") {
        KillTracker tracker;
        EntityId mob(30, 5);
        for (int i = 0; i < 50; i++)
            tracker.record_kill(mob, 30, 1.0);

        auto entries = tracker.get_trophy_entries();
        REQUIRE(entries.size() == 1);
        REQUIRE(entries[0].multiplier == Approx(MIN_MOB_MULTIPLIER).margin(0.01));
    }
}

TEST_CASE("KillTracker: JSON round-trip", "[unit][kill_tracker]") {
    SECTION("Empty tracker serializes and deserializes") {
        KillTracker tracker;
        auto json = tracker.to_json();
        auto restored = KillTracker::from_json(json);
        REQUIRE(restored.empty());
    }

    SECTION("Tracker with kills survives round-trip") {
        KillTracker tracker;
        EntityId mob1(30, 5);
        EntityId mob2(45, 10);
        tracker.record_kill(mob1, 30, 1.0);
        tracker.record_kill(mob2, 45, 0.5);

        auto json = tracker.to_json();
        auto json_str = json.dump();
        auto parsed = nlohmann::json::parse(json_str);
        auto restored = KillTracker::from_json(parsed);

        // Verify mob multipliers are approximately preserved
        REQUIRE(restored.mob_multiplier(mob1) == Approx(tracker.mob_multiplier(mob1)).margin(0.05));
        REQUIRE(restored.mob_multiplier(mob2) == Approx(tracker.mob_multiplier(mob2)).margin(0.05));

        // Verify zone bonuses are approximately preserved
        REQUIRE(restored.zone_bonus(30) == Approx(tracker.zone_bonus(30)).margin(0.05));
        REQUIRE(restored.zone_bonus(45) == Approx(tracker.zone_bonus(45)).margin(0.05));
    }

    SECTION("JSON format contains expected keys") {
        KillTracker tracker;
        EntityId mob(30, 5);
        tracker.record_kill(mob, 30, 1.0);

        auto json = tracker.to_json();
        REQUIRE(json.contains("kills"));
        REQUIRE(json.contains("zones"));
        REQUIRE(json["kills"].is_array());
        REQUIRE(json["zones"].is_array());
        REQUIRE(json["kills"].size() == 1);
        REQUIRE(json["zones"].size() == 1);

        auto &kill = json["kills"][0];
        REQUIRE(kill.contains("z"));
        REQUIRE(kill.contains("id"));
        REQUIRE(kill.contains("w"));
        REQUIRE(kill.contains("t"));
        REQUIRE(kill["z"].get<int>() == 30);
        REQUIRE(kill["id"].get<int>() == 5);
    }

    SECTION("Malformed JSON doesn't crash") {
        auto tracker = KillTracker::from_json(nlohmann::json::object());
        REQUIRE(tracker.empty());

        auto tracker2 = KillTracker::from_json(nlohmann::json::parse(R"({"kills": "bad", "zones": 123})"));
        REQUIRE(tracker2.empty());
    }
}

TEST_CASE("KillTracker: decay over time", "[unit][kill_tracker]") {
    SECTION("KillWeight decays correctly") {
        KillWeight kw;
        kw.weight = 4.0;
        auto now = std::chrono::system_clock::now();
        kw.last_kill = now;

        // At t=0, full weight
        REQUIRE(kw.decayed_at(now) == Approx(4.0));

        // After 2 hours (one half-life), weight should halve
        auto two_hours = now + std::chrono::hours(2);
        REQUIRE(kw.decayed_at(two_hours) == Approx(2.0).margin(0.01));

        // After 4 hours (two half-lives), weight should quarter
        auto four_hours = now + std::chrono::hours(4);
        REQUIRE(kw.decayed_at(four_hours) == Approx(1.0).margin(0.01));
    }

    SECTION("ZoneActivity decays with 4-hour half-life") {
        ZoneActivity za;
        za.weight = 2.0;
        auto now = std::chrono::system_clock::now();
        za.last_kill = now;

        // After 4 hours (one half-life), weight should halve
        auto four_hours = now + std::chrono::hours(4);
        REQUIRE(za.decayed_at(four_hours) == Approx(1.0).margin(0.01));
    }
}
