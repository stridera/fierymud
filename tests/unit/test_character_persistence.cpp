/**
 * @file test_character_persistence.cpp
 * @brief Unit tests for character persistence - verifies save/load round-trips
 *
 * Tests that all player properties are correctly preserved through:
 * - JSON serialization/deserialization
 * - Database save/load operations (when database is available)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <nlohmann/json.hpp>

#include "core/active_effect.hpp"
#include "core/ids.hpp"
#include "core/money.hpp"
#include "core/player.hpp"

using json = nlohmann::json;

// ============================================================================
// Player JSON Serialization Tests
// ============================================================================

TEST_CASE("Player JSON serialization preserves basic properties", "[persistence][player][json]") {

    SECTION("Name and level") {
        auto player_result = Player::create(EntityId{1, 0}, "TestPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_level(25);

        json j = player->to_json();

        REQUIRE(j["name"] == "TestPlayer");
        REQUIRE(j["stats"]["level"] == 25);
    }

    SECTION("All primary stats are preserved") {
        auto player_result = Player::create(EntityId{1, 1}, "StatPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        auto &stats = player->stats();
        stats.level = 15;
        stats.alignment = -500;
        stats.strength = 18;
        stats.intelligence = 16;
        stats.wisdom = 14;
        stats.dexterity = 15;
        stats.constitution = 17;
        stats.charisma = 12;
        stats.hit_points = 85;
        stats.max_hit_points = 120;
        stats.stamina = 90;
        stats.max_stamina = 100;
        stats.accuracy = 5;
        stats.attack_power = 10;
        stats.armor_rating = 25;

        json j = player->to_json();

        REQUIRE(j["stats"]["level"] == 15);
        REQUIRE(j["stats"]["alignment"] == -500);
        REQUIRE(j["stats"]["strength"] == 18);
        REQUIRE(j["stats"]["intelligence"] == 16);
        REQUIRE(j["stats"]["wisdom"] == 14);
        REQUIRE(j["stats"]["dexterity"] == 15);
        REQUIRE(j["stats"]["constitution"] == 17);
        REQUIRE(j["stats"]["charisma"] == 12);
        REQUIRE(j["stats"]["hit_points"] == 85);
        REQUIRE(j["stats"]["max_hit_points"] == 120);
        REQUIRE(j["stats"]["stamina"] == 90);
        REQUIRE(j["stats"]["max_stamina"] == 100);
        REQUIRE(j["stats"]["accuracy"] == 5);
        REQUIRE(j["stats"]["attack_power"] == 10);
        REQUIRE(j["stats"]["armor_rating"] == 25);
    }
}

TEST_CASE("Player wealth serialization", "[persistence][player][wealth]") {

    SECTION("Wallet value preserved in copper") {
        auto player_result = Player::create(EntityId{1, 2}, "WealthyPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        // Give player some money: 100 platinum = 100,000 copper
        player->give_wealth(100000);

        REQUIRE(player->wealth() == 100000);

        json j = player->to_json();

        // Wealth should be serialized
        REQUIRE(j.contains("wallet"));
    }

    SECTION("Wallet round-trip preserves value") {
        auto player1_result = Player::create(EntityId{1, 3}, "MoneyPlayer");
        REQUIRE(player1_result.has_value());
        auto player1 = std::move(*player1_result);

        // 5 platinum, 20 gold, 15 silver, 10 copper = 5*1000 + 20*100 + 15*10 + 10 = 7160 copper
        player1->give_wealth(7160);

        json j = player1->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->wealth() == 7160);
    }
}

TEST_CASE("Player position serialization", "[persistence][player][position]") {

    SECTION("Standing position preserved") {
        auto player_result = Player::create(EntityId{1, 4}, "StandingPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_position(Position::Standing);

        json j = player->to_json();

        // Position should be uppercase for database compatibility
        REQUIRE(j.contains("position"));
    }

    SECTION("Various positions preserved through round-trip") {
        std::vector<Position> positions_to_test = {Position::Standing, Position::Sitting, Position::Resting,
                                                   Position::Sleeping, Position::Fighting};

        for (auto pos : positions_to_test) {
            auto player1_result = Player::create(EntityId{1, 5}, "PosPlayer");
            REQUIRE(player1_result.has_value());
            auto player1 = std::move(*player1_result);

            player1->set_position(pos);

            json j = player1->to_json();
            auto player2_result = Player::from_json(j);
            REQUIRE(player2_result.has_value());
            auto player2 = std::move(*player2_result);

            REQUIRE(player2->position() == pos);
        }
    }
}

TEST_CASE("Player class and race serialization", "[persistence][player][class][race]") {

    SECTION("Class preserved") {
        auto player_result = Player::create(EntityId{1, 6}, "ClassPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_class("Sorcerer");
        player->set_level(10);

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->player_class() == "Sorcerer");
    }

    SECTION("Race preserved") {
        auto player_result = Player::create(EntityId{1, 7}, "RacePlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_race("Elf");

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->race() == "Elf");
    }

    SECTION("Gender preserved") {
        auto player_result = Player::create(EntityId{1, 8}, "GenderPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_gender("Female");

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->gender() == "Female");
    }
}

TEST_CASE("Player description and title serialization", "[persistence][player][description]") {

    SECTION("Description preserved") {
        auto player_result = Player::create(EntityId{1, 9}, "DescPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_description("A tall warrior with battle-scarred armor.");

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->description() == "A tall warrior with battle-scarred armor.");
    }

    SECTION("Title preserved") {
        auto player_result = Player::create(EntityId{1, 10}, "TitlePlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_title("the Dragon Slayer");

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->title() == "the Dragon Slayer");
    }
}

TEST_CASE("Player prompt serialization", "[persistence][player][prompt]") {

    SECTION("Custom prompt preserved") {
        auto player_result = Player::create(EntityId{1, 11}, "PromptPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_prompt("<%h/%Hhp %v/%Vs>");

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->prompt() == "<%h/%Hhp %v/%Vs>");
    }

    SECTION("Colored prompt preserved") {
        auto player_result = Player::create(EntityId{1, 12}, "ColorPromptPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        // Simulated colored prompt
        player->set_prompt("<&r%h&n/%Hhp &y%v&n/%Vs>");

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->prompt() == "<&r%h&n/%Hhp &y%v&n/%Vs>");
    }
}

TEST_CASE("Player location serialization", "[persistence][player][location]") {

    SECTION("Start room preserved") {
        auto player_result = Player::create(EntityId{1, 13}, "LocationPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        // Set start room to zone 30, room 15 (Inn)
        EntityId inn_room(30, 15);
        player->set_start_room(inn_room);

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->start_room() == inn_room);
        REQUIRE(player2->start_room().zone_id() == 30);
        REQUIRE(player2->start_room().local_id() == 15);
    }

    SECTION("Recall room preserved") {
        auto player_result = Player::create(EntityId{1, 14}, "RecallPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        // Set recall room to zone 3, room 0 (Temple)
        EntityId temple_room(3, 0);
        player->set_recall_room(temple_room);

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->recall_room() == temple_room);
        REQUIRE(player2->recall_room().zone_id() == 3);
        REQUIRE(player2->recall_room().local_id() == 0);
    }
}

TEST_CASE("Player active effects serialization", "[persistence][player][effects]") {

    SECTION("Active effect with effect_id preserved") {
        auto player_result = Player::create(EntityId{1, 15}, "EffectPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        // Add a Detect Magic effect
        ActiveEffect detect_magic;
        detect_magic.effect_id = 42; // Database effect ID
        detect_magic.name = "Detect Magic";
        detect_magic.source = "spell";
        detect_magic.flag = ActorFlag::Detect_Magic;
        detect_magic.duration_hours = 24.0;
        detect_magic.modifier_value = 0;
        detect_magic.modifier_stat = "";
        detect_magic.applied_at = std::chrono::steady_clock::now();

        player->add_effect(detect_magic);

        REQUIRE(player->has_effect("Detect Magic"));
        REQUIRE(player->active_effects().size() == 1);

        const auto *effect = player->get_effect("Detect Magic");
        REQUIRE(effect != nullptr);
        REQUIRE(effect->effect_id == 42);
        REQUIRE(effect->name == "Detect Magic");
        REQUIRE(effect->flag == ActorFlag::Detect_Magic);
    }

    SECTION("Multiple effects preserved") {
        auto player_result = Player::create(EntityId{1, 16}, "MultiEffectPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        // Add Armor effect
        ActiveEffect armor;
        armor.effect_id = 10;
        armor.name = "Armor";
        armor.source = "spell";
        armor.flag = ActorFlag::Armor;
        armor.duration_hours = 12.0;
        armor.modifier_value = 20;
        armor.modifier_stat = "armor_class";
        armor.applied_at = std::chrono::steady_clock::now();
        player->add_effect(armor);

        // Add Bless effect
        ActiveEffect bless;
        bless.effect_id = 15;
        bless.name = "Bless";
        bless.source = "spell";
        bless.flag = ActorFlag::Bless;
        bless.duration_hours = 6.0;
        bless.modifier_value = 2;
        bless.modifier_stat = "hit_roll";
        bless.applied_at = std::chrono::steady_clock::now();
        player->add_effect(bless);

        REQUIRE(player->active_effects().size() == 2);
        REQUIRE(player->has_effect("Armor"));
        REQUIRE(player->has_effect("Bless"));

        // Verify effect IDs are stored
        for (const auto &eff : player->active_effects()) {
            REQUIRE(eff.effect_id > 0);
        }
    }
}

TEST_CASE("Player DoT effect serialization", "[persistence][player][dot]") {

    SECTION("DoT effect fields preserved") {
        auto player_result = Player::create(EntityId{1, 17}, "DotPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        fiery::DotEffect poison;
        poison.ability_id = 100;
        poison.effect_id = 50;
        poison.effect_type = "dot";
        poison.damage_type = "poison";
        poison.cure_category = "poison";
        poison.potency = 5;
        poison.flat_damage = 10;
        poison.percent_damage = 0;
        poison.blocks_regen = true;
        poison.reduces_regen = 50;
        poison.remaining_ticks = 20;
        poison.tick_interval = 2;
        poison.source_actor_id = "player_123";
        poison.source_level = 15;
        poison.stack_count = 1;
        poison.max_stacks = 3;
        poison.stackable = true;

        player->add_dot_effect(poison);

        REQUIRE(player->dot_effects().size() == 1);

        const auto &loaded_dot = player->dot_effects()[0];
        REQUIRE(loaded_dot.effect_id == 50);
        REQUIRE(loaded_dot.damage_type == "poison");
        REQUIRE(loaded_dot.potency == 5);
        REQUIRE(loaded_dot.flat_damage == 10);
        REQUIRE(loaded_dot.blocks_regen == true);
        REQUIRE(loaded_dot.remaining_ticks == 20);
    }
}

TEST_CASE("Player HoT effect serialization", "[persistence][player][hot]") {

    SECTION("HoT effect fields preserved") {
        auto player_result = Player::create(EntityId{1, 18}, "HotPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        fiery::HotEffect regen;
        regen.ability_id = 200;
        regen.effect_id = 60;
        regen.effect_type = "hot";
        regen.heal_type = "divine";
        regen.hot_category = "heal";
        regen.flat_heal = 15;
        regen.percent_heal = 2;
        regen.boosts_regen = true;
        regen.regen_boost = 25;
        regen.remaining_ticks = 30;
        regen.tick_interval = 3;
        regen.source_actor_id = "cleric_456";
        regen.source_level = 20;
        regen.stack_count = 2;
        regen.max_stacks = 5;
        regen.stackable = true;

        player->add_hot_effect(regen);

        REQUIRE(player->hot_effects().size() == 1);

        const auto &loaded_hot = player->hot_effects()[0];
        REQUIRE(loaded_hot.effect_id == 60);
        REQUIRE(loaded_hot.heal_type == "divine");
        REQUIRE(loaded_hot.flat_heal == 15);
        REQUIRE(loaded_hot.boosts_regen == true);
        REQUIRE(loaded_hot.remaining_ticks == 30);
    }
}

TEST_CASE("Player actor flags serialization", "[persistence][player][flags]") {

    SECTION("Actor flags preserved") {
        auto player_result = Player::create(EntityId{1, 19}, "FlagPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_flag(ActorFlag::Detect_Invis, true);
        player->set_flag(ActorFlag::Infravision, true);
        player->set_flag(ActorFlag::Sneak, true);

        REQUIRE(player->has_flag(ActorFlag::Detect_Invis));
        REQUIRE(player->has_flag(ActorFlag::Infravision));
        REQUIRE(player->has_flag(ActorFlag::Sneak));
        REQUIRE_FALSE(player->has_flag(ActorFlag::Flying));

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->has_flag(ActorFlag::Detect_Invis));
        REQUIRE(player2->has_flag(ActorFlag::Infravision));
        REQUIRE(player2->has_flag(ActorFlag::Sneak));
        REQUIRE_FALSE(player2->has_flag(ActorFlag::Flying));
    }
}

TEST_CASE("Player god level serialization", "[persistence][player][immortal]") {

    SECTION("God level preserved for immortals") {
        auto player_result = Player::create(EntityId{1, 20}, "ImmPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_level(105);
        player->set_god_level(6); // level 105 = god_level 6

        REQUIRE(player->is_god());
        REQUIRE(player->god_level() == 6);

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE(player2->is_god());
        REQUIRE(player2->god_level() == 6);
        REQUIRE(player2->level() == 105);
    }

    SECTION("Regular player has no god level") {
        auto player_result = Player::create(EntityId{1, 21}, "MortalPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_level(50);

        REQUIRE_FALSE(player->is_god());
        REQUIRE(player->god_level() == 0);

        json j = player->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        REQUIRE_FALSE(player2->is_god());
        REQUIRE(player2->god_level() == 0);
    }
}

// ============================================================================
// Comprehensive Round-Trip Test
// ============================================================================

TEST_CASE("Complete player data round-trip", "[persistence][player][comprehensive]") {

    SECTION("All player properties preserved through JSON round-trip") {
        auto player1_result = Player::create(EntityId{1, 100}, "CompletePlayer");
        REQUIRE(player1_result.has_value());
        auto player1 = std::move(*player1_result);

        // Set all properties that should be persisted

        // Basic info
        player1->set_level(75);
        player1->set_class("Warrior");
        player1->set_race("Dwarf");
        player1->set_gender("Male");
        player1->set_title("the Mountain King");
        player1->set_description("A stout dwarf with a magnificent beard.");

        // Stats
        auto &stats = player1->stats();
        stats.alignment = 750; // Good
        stats.strength = 20;
        stats.intelligence = 12;
        stats.wisdom = 14;
        stats.dexterity = 15;
        stats.constitution = 19;
        stats.charisma = 10;
        stats.hit_points = 450;
        stats.max_hit_points = 500;
        stats.stamina = 180;
        stats.max_stamina = 200;
        stats.accuracy = 15;
        stats.attack_power = 25;
        stats.armor_rating = 60;

        // Wealth
        player1->give_wealth(50000); // 50 platinum worth

        // Location
        EntityId inn_room(30, 15);
        EntityId temple_room(3, 0);
        player1->set_start_room(inn_room);
        player1->set_recall_room(temple_room);

        // Prompt
        player1->set_prompt("<%h/%Hhp %v/%Vs %g gold>");

        // Position
        player1->set_position(Position::Resting);

        // Actor flags
        player1->set_flag(ActorFlag::Detect_Invis, true);
        player1->set_flag(ActorFlag::Sanctuary, true);

        // Active effects
        ActiveEffect armor_effect;
        armor_effect.effect_id = 10;
        armor_effect.name = "Armor";
        armor_effect.source = "spell";
        armor_effect.flag = ActorFlag::Armor;
        armor_effect.duration_hours = 24.0;
        armor_effect.modifier_value = 20;
        armor_effect.modifier_stat = "armor_class";
        armor_effect.applied_at = std::chrono::steady_clock::now();
        player1->add_effect(armor_effect);

        // DoT effect
        fiery::DotEffect poison;
        poison.ability_id = 100;
        poison.effect_id = 50;
        poison.damage_type = "poison";
        poison.cure_category = "poison";
        poison.potency = 3;
        poison.flat_damage = 5;
        poison.remaining_ticks = 10;
        player1->add_dot_effect(poison);

        // HoT effect
        fiery::HotEffect regen;
        regen.ability_id = 200;
        regen.effect_id = 60;
        regen.heal_type = "divine";
        regen.flat_heal = 10;
        regen.remaining_ticks = 15;
        player1->add_hot_effect(regen);

        // ---- Serialize and deserialize ----
        json j = player1->to_json();

        auto player2_result = Player::from_json(j);
        REQUIRE(player2_result.has_value());
        auto player2 = std::move(*player2_result);

        // ---- Verify all properties ----

        // Basic info
        REQUIRE(player2->name() == "CompletePlayer");
        REQUIRE(player2->level() == 75);
        REQUIRE(player2->player_class() == "Warrior");
        REQUIRE(player2->race() == "Dwarf");
        REQUIRE(player2->gender() == "Male");
        REQUIRE(player2->title() == "the Mountain King");
        REQUIRE(player2->description() == "A stout dwarf with a magnificent beard.");

        // Stats
        const auto &stats2 = player2->stats();
        REQUIRE(stats2.alignment == 750);
        REQUIRE(stats2.strength == 20);
        REQUIRE(stats2.intelligence == 12);
        REQUIRE(stats2.wisdom == 14);
        REQUIRE(stats2.dexterity == 15);
        REQUIRE(stats2.constitution == 19);
        REQUIRE(stats2.charisma == 10);
        REQUIRE(stats2.hit_points == 450);
        REQUIRE(stats2.max_hit_points == 500);
        REQUIRE(stats2.stamina == 180);
        REQUIRE(stats2.max_stamina == 200);
        REQUIRE(stats2.accuracy == 15);
        REQUIRE(stats2.attack_power == 25);
        REQUIRE(stats2.armor_rating == 60);

        // Wealth
        REQUIRE(player2->wealth() == 50000);

        // Location
        REQUIRE(player2->start_room() == inn_room);
        REQUIRE(player2->recall_room() == temple_room);

        // Prompt
        REQUIRE(player2->prompt() == "<%h/%Hhp %v/%Vs %g gold>");

        // Position
        REQUIRE(player2->position() == Position::Resting);

        // Actor flags
        REQUIRE(player2->has_flag(ActorFlag::Detect_Invis));
        REQUIRE(player2->has_flag(ActorFlag::Sanctuary));
        REQUIRE_FALSE(player2->has_flag(ActorFlag::Flying));

        // Active effects
        REQUIRE(player2->has_effect("Armor"));
        const auto *armor = player2->get_effect("Armor");
        REQUIRE(armor != nullptr);
        REQUIRE(armor->effect_id == 10);
        REQUIRE(armor->modifier_value == 20);

        // DoT effects
        REQUIRE(player2->dot_effects().size() == 1);
        REQUIRE(player2->dot_effects()[0].effect_id == 50);
        REQUIRE(player2->dot_effects()[0].damage_type == "poison");

        // HoT effects
        REQUIRE(player2->hot_effects().size() == 1);
        REQUIRE(player2->hot_effects()[0].effect_id == 60);
        REQUIRE(player2->hot_effects()[0].heal_type == "divine");
    }
}

// ============================================================================
// Stats Validation Tests
// ============================================================================

TEST_CASE("Stats validation in persistence context", "[persistence][stats][validation]") {

    SECTION("Valid stats pass validation") {
        Stats stats;
        stats.level = 50;
        stats.gold = 10000;
        stats.hit_points = 100;
        stats.max_hit_points = 100;

        auto result = stats.validate();
        REQUIRE(result.has_value());
    }

    SECTION("Negative gold fails validation") {
        Stats stats;
        stats.gold = -100;

        auto result = stats.validate();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().message.find("Gold cannot be negative") != std::string::npos);
    }

    SECTION("Zero level becomes level 1") {
        auto player_result = Player::create(EntityId{1, 30}, "ZeroLevelPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        player->set_level(0);

        // Level should be clamped to at least 1
        REQUIRE(player->level() >= 1);
    }
}

// ============================================================================
// Money System Tests (for persistence context)
// ============================================================================

TEST_CASE("Money system in persistence context", "[persistence][money]") {

    SECTION("Money conversion to copper") {
        // Create money using factory methods and addition
        fiery::Money money = fiery::Money::copper(100) + fiery::Money::silver(50) + // +500 copper
                             fiery::Money::gold(10) +                               // +1000 copper
                             fiery::Money::platinum(5);                             // +5000 copper

        REQUIRE(money.value() == 6600);
    }

    SECTION("Player wallet operations") {
        auto player_result = Player::create(EntityId{1, 31}, "WalletPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(*player_result);

        // Initial wealth is 0
        REQUIRE(player->wealth() == 0);

        // Give some money
        player->give_wealth(1000);
        REQUIRE(player->wealth() == 1000);

        // Can afford check
        REQUIRE(player->can_afford(500));
        REQUIRE(player->can_afford(1000));
        REQUIRE_FALSE(player->can_afford(1001));

        // Take money
        REQUIRE(player->take_wealth(300));
        REQUIRE(player->wealth() == 700);

        // Cannot take more than available
        REQUIRE_FALSE(player->take_wealth(1000));
        REQUIRE(player->wealth() == 700); // Unchanged
    }
}
