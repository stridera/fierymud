/**
 * @file test_database_persistence.cpp
 * @brief Unit tests for database persistence using MockPlayerRepository
 *
 * Tests the save/load paths through the repository abstraction without
 * requiring a real database connection.
 */

#include <catch2/catch_test_macros.hpp>
#include <core/actor.hpp>
#include <core/money.hpp>
#include <core/player.hpp>
#include <database/player_repository.hpp>
#include <database/world_queries.hpp>
#include <magic_enum/magic_enum.hpp>

#include "../common/mock_player_repository.hpp"

// ============================================================================
// Repository Mock Tests
// ============================================================================

TEST_CASE("MockPlayerRepository basic operations", "[database][mock][repository]") {

    MockRepositoryGuard guard;
    auto &mock = guard.mock();

    SECTION("Save and load character data") {
        WorldQueries::CharacterData char_data;
        char_data.id = "test-uuid-123";
        char_data.name = "TestWarrior";
        char_data.level = 50;
        char_data.alignment = 500;
        char_data.strength = 18;
        char_data.intelligence = 12;
        char_data.wisdom = 14;
        char_data.dexterity = 15;
        char_data.constitution = 17;
        char_data.charisma = 10;
        char_data.hit_points = 350;
        char_data.hit_points_max = 400;
        char_data.stamina = 150;
        char_data.stamina_max = 180;
        char_data.wealth = 50000; // 50 platinum in copper
        char_data.current_room_zone_id = 30;
        char_data.current_room_id = 15;
        char_data.recall_room_zone_id = 3;
        char_data.recall_room_id = 0;
        char_data.hit_roll = 10;
        char_data.damage_roll = 15;
        char_data.armor_class = 40;
        char_data.position = "STANDING";
        char_data.description = "A battle-worn warrior.";
        char_data.title = "the Brave";
        char_data.prompt = "<%h/%Hhp>";

        // Save through repository
        auto save_result = mock.save_character(char_data);
        REQUIRE(save_result.has_value());

        // Load through repository
        auto load_result = mock.load_player_by_name("TestWarrior");
        REQUIRE(load_result.has_value());

        auto &player = *load_result;
        REQUIRE(player->name() == "TestWarrior");
        REQUIRE(player->stats().level == 50);
        REQUIRE(player->stats().alignment == 500);
        REQUIRE(player->stats().strength == 18);
        REQUIRE(player->stats().hit_points == 350);
        REQUIRE(player->wealth() == 50000);
        REQUIRE(player->start_room().zone_id() == 30);
        REQUIRE(player->start_room().local_id() == 15);
        REQUIRE(player->recall_room().zone_id() == 3);
        REQUIRE(player->recall_room().local_id() == 0);
        REQUIRE(player->description() == "A battle-worn warrior.");
        REQUIRE(player->title() == "the Brave");
        REQUIRE(player->prompt() == "<%h/%Hhp>");
        REQUIRE(player->position() == Position::Standing);
    }

    SECTION("Case-insensitive player lookup") {
        WorldQueries::CharacterData char_data;
        char_data.id = "test-uuid-456";
        char_data.name = "MixedCase";
        char_data.level = 10;

        mock.save_character(char_data);

        REQUIRE(mock.player_exists("MixedCase"));
        REQUIRE(mock.player_exists("mixedcase"));
        REQUIRE(mock.player_exists("MIXEDCASE"));
        REQUIRE(mock.player_exists("mIxEdCaSe"));
    }

    SECTION("Player count") {
        REQUIRE(mock.get_player_count() == 0);

        WorldQueries::CharacterData char1;
        char1.id = "uuid-1";
        char1.name = "Player1";
        char1.level = 5;
        mock.save_character(char1);

        REQUIRE(mock.get_player_count() == 1);

        WorldQueries::CharacterData char2;
        char2.id = "uuid-2";
        char2.name = "Player2";
        char2.level = 10;
        mock.save_character(char2);

        REQUIRE(mock.get_player_count() == 2);
    }

    SECTION("Not found returns error") {
        auto result = mock.load_player_by_name("NonExistent");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().code == ErrorCode::NotFound);
    }

    SECTION("Zone 0 and room 0 are valid locations") {
        WorldQueries::CharacterData char_data;
        char_data.id = "zone-zero-uuid";
        char_data.name = "ZoneZeroPlayer";
        char_data.level = 5;
        char_data.current_room_zone_id = 0; // Zone 0 is valid
        char_data.current_room_id = 0;      // Room 0 is valid
        char_data.recall_room_zone_id = 0;
        char_data.recall_room_id = 0;

        auto save_result = mock.save_character(char_data);
        REQUIRE(save_result.has_value());

        auto load_result = mock.load_player_by_name("ZoneZeroPlayer");
        REQUIRE(load_result.has_value());

        auto &player = *load_result;
        REQUIRE(player->start_room().zone_id() == 0);
        REQUIRE(player->start_room().local_id() == 0);
        REQUIRE(player->recall_room().zone_id() == 0);
        REQUIRE(player->recall_room().local_id() == 0);
    }
}

TEST_CASE("MockPlayerRepository effect persistence", "[database][mock][effects]") {

    MockRepositoryGuard guard;
    auto &mock = guard.mock();

    SECTION("Save and load character effects") {
        std::string char_id = "effect-test-uuid";

        // Create character first
        WorldQueries::CharacterData char_data;
        char_data.id = char_id;
        char_data.name = "EffectPlayer";
        char_data.level = 20;
        mock.save_character(char_data);

        // Save effects
        std::vector<WorldQueries::CharacterEffectData> effects;

        WorldQueries::CharacterEffectData armor_effect;
        armor_effect.character_id = char_id;
        armor_effect.effect_id = 10;
        armor_effect.effect_name = "Armor";
        armor_effect.effect_type = "status";
        armor_effect.source_type = "spell";
        armor_effect.duration_seconds = 3600; // 1 hour in seconds
        armor_effect.modifier_data = R"({"effect_name":"Armor","flag":"Armor"})";
        effects.push_back(armor_effect);

        WorldQueries::CharacterEffectData detect_effect;
        detect_effect.character_id = char_id;
        detect_effect.effect_id = 42;
        detect_effect.effect_name = "Detect Magic";
        detect_effect.effect_type = "status";
        detect_effect.source_type = "spell";
        detect_effect.duration_seconds = 7200; // 2 hours
        detect_effect.modifier_data = R"({"effect_name":"Detect Magic","flag":"Detect_Magic"})";
        effects.push_back(detect_effect);

        auto save_result = mock.save_character_effects(char_id, effects);
        REQUIRE(save_result.has_value());

        // Load effects
        auto load_result = mock.load_character_effects(char_id);
        REQUIRE(load_result.has_value());
        REQUIRE(load_result->size() == 2);

        // Verify effect data
        auto &loaded_effects = *load_result;
        REQUIRE(loaded_effects[0].effect_id == 10);
        REQUIRE(loaded_effects[0].effect_name == "Armor");
        REQUIRE(loaded_effects[1].effect_id == 42);
        REQUIRE(loaded_effects[1].effect_name == "Detect Magic");
    }

    SECTION("Empty effects for new character") {
        auto result = mock.load_character_effects("non-existent-uuid");
        REQUIRE(result.has_value());
        REQUIRE(result->empty());
    }
}

TEST_CASE("MockPlayerRepository item persistence", "[database][mock][items]") {

    MockRepositoryGuard guard;
    auto &mock = guard.mock();

    SECTION("Save and verify character items") {
        std::string char_id = "item-test-uuid";

        // Create character
        WorldQueries::CharacterData char_data;
        char_data.id = char_id;
        char_data.name = "ItemPlayer";
        char_data.level = 15;
        mock.save_character(char_data);

        // Save items
        std::vector<WorldQueries::CharacterItemData> items;

        WorldQueries::CharacterItemData sword;
        sword.character_id = char_id;
        sword.object_id = EntityId{30, 100};
        sword.equipped_location = "RightHand";
        sword.condition = 100;
        sword.charges = 0;
        items.push_back(sword);

        WorldQueries::CharacterItemData potion;
        potion.character_id = char_id;
        potion.object_id = EntityId{30, 50};
        potion.equipped_location = ""; // In inventory
        potion.condition = 100;
        potion.charges = 3;
        items.push_back(potion);

        auto save_result = mock.save_character_items(char_id, items);
        REQUIRE(save_result.has_value());

        // Verify stored items
        auto stored = mock.get_items(char_id);
        REQUIRE(stored != nullptr);
        REQUIRE(stored->size() == 2);
        REQUIRE((*stored)[0].equipped_location == "RightHand");
        REQUIRE((*stored)[1].charges == 3);
    }
}

TEST_CASE("MockPlayerRepository account wealth", "[database][mock][wealth]") {

    MockRepositoryGuard guard;
    auto &mock = guard.mock();

    SECTION("Save and verify account wealth") {
        std::string user_id = "user-account-uuid";

        auto save_result = mock.save_account_wealth(user_id, 1000000); // 1000 platinum
        REQUIRE(save_result.has_value());

        auto stored = mock.get_account_wealth(user_id);
        REQUIRE(stored.has_value());
        REQUIRE(*stored == 1000000);
    }

    SECTION("Non-existent account returns nullopt") {
        auto stored = mock.get_account_wealth("non-existent-user");
        REQUIRE_FALSE(stored.has_value());
    }
}

TEST_CASE("PlayerRepositoryProvider switching", "[database][mock][provider]") {

    SECTION("Default is PostgreSQL implementation") {
        // Reset to ensure we're starting fresh
        PlayerRepositoryProvider::instance().reset();
        REQUIRE_FALSE(PlayerRepositoryProvider::instance().is_mock());
    }

    SECTION("Guard installs and removes mock") {
        REQUIRE_FALSE(PlayerRepositoryProvider::instance().is_mock());

        {
            MockRepositoryGuard guard;
            REQUIRE(PlayerRepositoryProvider::instance().is_mock());
        }

        REQUIRE_FALSE(PlayerRepositoryProvider::instance().is_mock());
    }
}

// ============================================================================
// Full Persistence Round-Trip Tests
// ============================================================================

TEST_CASE("Complete player save/load round-trip via repository", "[database][mock][roundtrip]") {

    MockRepositoryGuard guard;
    auto &mock = guard.mock();

    SECTION("Full player data preserved through repository") {
        // Create character data matching what PersistenceManager would build
        WorldQueries::CharacterData char_data;
        char_data.id = "roundtrip-test-uuid";
        char_data.name = "RoundTripPlayer";
        char_data.level = 75;
        char_data.alignment = -250; // Slightly evil
        char_data.strength = 16;
        char_data.intelligence = 18;
        char_data.wisdom = 14;
        char_data.dexterity = 15;
        char_data.constitution = 13;
        char_data.charisma = 12;
        char_data.hit_points = 280;
        char_data.hit_points_max = 320;
        char_data.stamina = 140;
        char_data.stamina_max = 160;
        char_data.wealth = 123456; // Various copper
        char_data.current_room_zone_id = 45;
        char_data.current_room_id = 20;
        char_data.recall_room_zone_id = 30;
        char_data.recall_room_id = 5;
        char_data.hit_roll = 12;
        char_data.damage_roll = 18;
        char_data.armor_class = 35;
        char_data.position = "RESTING";
        char_data.description = "A mysterious figure cloaked in shadows.";
        char_data.title = "the Enigmatic";
        char_data.prompt = "<&r%h&n/%Hhp &y%v&n/%Vs>";

        // Save
        auto save_result = mock.save_character(char_data);
        REQUIRE(save_result.has_value());

        // Load via repository interface (same as real code would use)
        auto &repo = PlayerRepositoryProvider::instance().get();
        auto load_result = repo.load_player_by_name("RoundTripPlayer");
        REQUIRE(load_result.has_value());

        auto &player = *load_result;

        // Verify all fields
        REQUIRE(player->name() == "RoundTripPlayer");
        REQUIRE(player->stats().level == 75);
        REQUIRE(player->stats().alignment == -250);
        REQUIRE(player->stats().strength == 16);
        REQUIRE(player->stats().intelligence == 18);
        REQUIRE(player->stats().wisdom == 14);
        REQUIRE(player->stats().dexterity == 15);
        REQUIRE(player->stats().constitution == 13);
        REQUIRE(player->stats().charisma == 12);
        REQUIRE(player->stats().hit_points == 280);
        REQUIRE(player->stats().max_hit_points == 320);
        REQUIRE(player->stats().stamina == 140);
        REQUIRE(player->stats().max_stamina == 160);
        REQUIRE(player->wealth() == 123456);
        REQUIRE(player->start_room().zone_id() == 45);
        REQUIRE(player->start_room().local_id() == 20);
        REQUIRE(player->recall_room().zone_id() == 30);
        REQUIRE(player->recall_room().local_id() == 5);
        REQUIRE(player->stats().accuracy == 12);
        REQUIRE(player->stats().attack_power == 18);
        REQUIRE(player->stats().armor_rating == 65); // 100 - 35
        REQUIRE(player->position() == Position::Resting);
        REQUIRE(player->description() == "A mysterious figure cloaked in shadows.");
        REQUIRE(player->title() == "the Enigmatic");
        REQUIRE(player->prompt() == "<&r%h&n/%Hhp &y%v&n/%Vs>");
    }
}
