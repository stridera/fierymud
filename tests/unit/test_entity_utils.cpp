#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <memory>
#include <string>

#include "../src/core/entity.hpp"
#include "../src/core/object.hpp"
#include "../src/core/actor.hpp"
#include "../src/world/room.hpp"

static const std::set<std::string> test_keywords{
    "silly",
    "little",
    "test",
    "case",
};
static const std::vector<std::string> test_keywords_vec(test_keywords.begin(), test_keywords.end());

TEST_CASE("Entity Utils - matches_target_string", "[entity][utils]") {
    SECTION("Prefix match with single keyword") {
        REQUIRE(EntityUtils::matches_target_string(test_keywords, "silly") == true);
        REQUIRE(EntityUtils::matches_target_string(test_keywords, "little") == true);
        REQUIRE(EntityUtils::matches_target_string(test_keywords, "test") == true);
        REQUIRE(EntityUtils::matches_target_string(test_keywords, "case") == true);
        REQUIRE(EntityUtils::matches_target_string(test_keywords, "lit") == true);
        REQUIRE(EntityUtils::matches_target_string(test_keywords, "zany") == false);
    }

    SECTION("Prefix match with multiple keywords") {
        REQUIRE(EntityUtils::matches_target_string(test_keywords, "silly-little") == true);
        REQUIRE(EntityUtils::matches_target_string(test_keywords, "case-little") == true);
        REQUIRE(EntityUtils::matches_target_string(test_keywords, "lit-cas") == true);
        REQUIRE(EntityUtils::matches_target_string(test_keywords, "cas-lit") == true);
        REQUIRE(EntityUtils::matches_target_string(test_keywords, "zany-something") == false);
    }
}

TEST_CASE("Entity Utils - normalize_keyword", "[entity][utils]") {
    SECTION("Normalize input") {
        std::string normalized = EntityUtils::normalize_keyword("Mixed Case  with strange--parts");
        REQUIRE(normalized == "mixed case with strange-parts");
    }
}

TEST_CASE("Entity Utils - parse_target_string", "[entity][utils]") {
    SECTION("Normalize input") {
        auto tokens = EntityUtils::parse_target_string("--Mixed--funky-sTring");
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens.contains("mixed"));
        REQUIRE(tokens.contains("funky"));
        REQUIRE(tokens.contains("string"));
    }

    SECTION("Deduplicate keywords") {
        auto tokens = EntityUtils::parse_target_string("run-forrest-run");
        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens.contains("run"));
        REQUIRE(tokens.contains("forrest"));
    }
}

TEST_CASE("Entity - matches_all_keywords", "[entity][utils]") {
    SECTION("Prefix match with single keyword") {
        auto obj = Object::create(EntityId{1001}, "object", ObjectType::Other).value();
        obj->set_keywords(test_keywords_vec);

        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "obj" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "silly" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "little" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "test" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "case" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "lit" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "zany" }) == false);
    }

    SECTION("Prefix match with multiple keywords") {
        auto obj = Object::create(EntityId{1001}, "object", ObjectType::Other).value();
        obj->set_keywords(test_keywords_vec);

        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "obj", "little" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "silly", "little" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "case", "little" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "lit", "cas" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "cas", "lit" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "zany", "something" }) == false);
    }

    SECTION("Match liquid in drink container") {
        auto obj = Object::create(EntityId{1001}, "beverage", ObjectType::Other).value();
        obj->set_keywords(test_keywords_vec);

        LiquidInfo liquid;
        liquid.liquid_type = "wine";
        liquid.capacity = 10;
        liquid.remaining = 10;
        liquid.identified = true;

        // can target a drink container by an identified liquid if it isn't empty
        obj->set_liquid_info(liquid);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "beverage" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "wine" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "win", "bev" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "water" }) == false);

        // empty container can't be targeted by liquid
        liquid.remaining = 0;
        obj->set_liquid_info(liquid);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "beverage" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "wine" }) == false);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "win", "bev" }) == false);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "water" }) == false);

        // unidentified liquid can't be targeted
        liquid.remaining = 10;
        liquid.identified = false;
        obj->set_liquid_info(liquid);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "beverage" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "wine" }) == false);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "win", "bev" }) == false);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "water" }) == false);

        // undefined liquid can't be targeted
        liquid.liquid_type.clear();
        liquid.identified = true;
        obj->set_liquid_info(liquid);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "beverage" }) == true);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "wine" }) == false);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "win", "bev" }) == false);
        REQUIRE(obj->matches_all_keywords(std::vector<std::string>{ "water" }) == false);
    }
}
