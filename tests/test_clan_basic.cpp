#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    return Catch::Session().run(argc, argv);
}

#include "clan.hpp"

TEST_CASE("Basic Clan Test", "[clan]") {
    SECTION("ClanRank creation and basic operations") {
        PermissionSet perms;
        perms.set(static_cast<size_t>(ClanPermission::CLAN_CHAT));
        perms.set(static_cast<size_t>(ClanPermission::SET_DESCRIPTION));
        
        ClanRank rank("Test Rank", perms);
        
        REQUIRE(rank.title() == "Test Rank");
        REQUIRE(rank.has_permission(ClanPermission::CLAN_CHAT));
        REQUIRE(rank.has_permission(ClanPermission::SET_DESCRIPTION));
        REQUIRE_FALSE(rank.has_permission(ClanPermission::INVITE_MEMBERS));
        
        // Test permission modification
        rank.add_permission(ClanPermission::INVITE_MEMBERS);
        REQUIRE(rank.has_permission(ClanPermission::INVITE_MEMBERS));
        
        rank.remove_permission(ClanPermission::CLAN_CHAT);
        REQUIRE_FALSE(rank.has_permission(ClanPermission::CLAN_CHAT));
        
        // Test set_permission
        rank.set_permission(ClanPermission::STORE_ITEMS, true);
        REQUIRE(rank.has_permission(ClanPermission::STORE_ITEMS));
        
        rank.set_permission(ClanPermission::STORE_ITEMS, false);
        REQUIRE_FALSE(rank.has_permission(ClanPermission::STORE_ITEMS));
    }
    
    SECTION("ClanRank comparison") {
        PermissionSet perms1;
        perms1.set(static_cast<size_t>(ClanPermission::CLAN_CHAT));
        
        PermissionSet perms2;
        perms2.set(static_cast<size_t>(ClanPermission::SET_DESCRIPTION));
        
        ClanRank rank1("Alpha", perms1);
        ClanRank rank2("Beta", perms2);
        ClanRank rank3("Alpha", perms1);
        
        REQUIRE(rank1 < rank2);
        REQUIRE_FALSE(rank2 < rank1);
        REQUIRE(rank1 == rank3);
        REQUIRE_FALSE(rank1 == rank2);
    }
    
    SECTION("ClanRepository basic operations") {
        // Clear any existing clans
        while (clan_repository.count() > 0) {
            auto clans = clan_repository.all();
            for (const auto& clan : clans) {
                clan_repository.remove(clan->id());
            }
        }
        
        // Create a clan
        auto clan = clan_repository.create(1, "Test Clan", "TEST");
        
        REQUIRE(clan != nullptr);
        REQUIRE(clan->id() == 1);
        REQUIRE(clan->name() == "Test Clan");
        REQUIRE(clan->abbreviation() == "TEST");
        REQUIRE(clan->member_count() == 0);
        
        // Test repository operations
        REQUIRE(clan_repository.count() == 1);
        
        auto found = clan_repository.find_by_id(1);
        REQUIRE(found.has_value());
        REQUIRE(found->get()->name() == "Test Clan");
        
        found = clan_repository.find_by_name("Test Clan");
        REQUIRE(found.has_value());
        REQUIRE(found->get()->id() == 1);
        
        found = clan_repository.find_by_abbreviation("TEST");
        REQUIRE(found.has_value());
        REQUIRE(found->get()->id() == 1);
        
        // Test not found
        auto not_found = clan_repository.find_by_id(999);
        REQUIRE_FALSE(not_found.has_value());
        
        // Clean up
        clan_repository.remove(1);
        REQUIRE(clan_repository.count() == 0);
    }
    
    SECTION("JSON serialization basic test") {
        // Clear any existing clans
        while (clan_repository.count() > 0) {
            auto clans = clan_repository.all();
            for (const auto& clan : clans) {
                clan_repository.remove(clan->id());
            }
        }
        
        // Create a clan with some data
        auto clan = clan_repository.create(1, "Test Clan", "TEST");
        
        // Test JSON serialization
        nlohmann::json j = *clan;
        
        REQUIRE(j["id"] == 1);
        REQUIRE(j["name"] == "Test Clan");
        REQUIRE(j["abbreviation"] == "TEST");
        REQUIRE(j.contains("treasure"));
        REQUIRE(j.contains("storage"));
        REQUIRE(j.contains("ranks"));
        
        // Clean up
        clan_repository.remove(1);
    }
    
    SECTION("Error handling tests") {
        // Test static error strings
        REQUIRE(AccessError::ClanNotFound == "Clan not found.");
        REQUIRE(AccessError::PermissionDenied == "You do not have permission to do that.");
        REQUIRE(AccessError::InvalidOperation == "Invalid operation");
    }
}