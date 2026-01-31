#include "../src/core/entity.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

static const std::set<std::string> test_keywords{
    "silly",
    "little",
    "test",
    "case",
};

TEST_CASE("Entity Utils", "matches_target_string") {
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

TEST_CASE("Entity Utils", "normalize_keyword") {
    SECTION("Normalize input") {
        std::string normalized = EntityUtils::normalize_keyword("Mixed Case  with strange--parts");
        REQUIRE(normalized == "mixed case with strange-parts");
    }
}

TEST_CASE("Entity Utils", "parse_target_string") {
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
