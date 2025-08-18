#include <catch2/catch_test_macros.hpp>

#include "clan.hpp"
#include "chars.hpp"
#include "structs.hpp"
#include "utils.hpp"
#include "logging.hpp"
#include "comm.hpp"
#include "defines.hpp"

#include <memory>
#include <stdexcept>
#include <string>

// Test fixture for clan permission testing
class ClanPermissionTestFixture {
public:
    ClanPermissionTestFixture() {
        // Create a test clan
        test_clan = clan_repository.create(1, "TestClan", "TEST");
        
        // Create test ranks with different permissions
        create_test_ranks();
        
        // Create test characters
        create_test_characters();
    }
    
    ~ClanPermissionTestFixture() {
        // Clean up
        clan_repository.remove(1);
    }
    
protected:
    std::shared_ptr<Clan> test_clan;
    std::shared_ptr<CharData> leader_char;
    std::shared_ptr<CharData> officer_char;
    std::shared_ptr<CharData> member_char;
    std::shared_ptr<CharData> clanless_char;
    std::shared_ptr<CharData> god_char;
    
    void init_character_data(CharData* ch, const std::string& name, int level) {
        // Initialize minimal character data for testing
        ch->player.short_descr = strdup(name.c_str());
        ch->player.level = level;
        ch->player_specials = new PlayerSpecialData();
    }

private:
    void create_test_ranks() {
        // Leader rank with LEADER_OVERRIDE
        PermissionSet leader_perms;
        leader_perms.set(static_cast<size_t>(ClanPermission::LEADER_OVERRIDE));
        auto leader_result = test_clan->admin_add_rank(ClanRank("Leader", leader_perms));
        if (!leader_result) {
            throw std::runtime_error("Failed to add leader rank: " + leader_result.error());
        }
        
        // Officer rank with specific permissions
        PermissionSet officer_perms;
        officer_perms.set(static_cast<size_t>(ClanPermission::CLAN_CHAT));
        officer_perms.set(static_cast<size_t>(ClanPermission::INVITE_MEMBERS));
        officer_perms.set(static_cast<size_t>(ClanPermission::KICK_MEMBERS));
        officer_perms.set(static_cast<size_t>(ClanPermission::PROMOTE_MEMBERS));
        auto officer_result = test_clan->admin_add_rank(ClanRank("Officer", officer_perms));
        if (!officer_result) {
            throw std::runtime_error("Failed to add officer rank: " + officer_result.error());
        }
        
        // Member rank with basic permissions
        PermissionSet member_perms;
        member_perms.set(static_cast<size_t>(ClanPermission::CLAN_CHAT));
        member_perms.set(static_cast<size_t>(ClanPermission::CLAN_WHO));
        auto member_result = test_clan->admin_add_rank(ClanRank("Member", member_perms));
        if (!member_result) {
            throw std::runtime_error("Failed to add member rank: " + member_result.error());
        }
    }
    
    void create_test_characters() {
        // Create test characters
        leader_char = std::make_shared<CharData>();
        officer_char = std::make_shared<CharData>();
        member_char = std::make_shared<CharData>();
        clanless_char = std::make_shared<CharData>();
        god_char = std::make_shared<CharData>();
        
        // Initialize basic character data
        init_character_data(leader_char.get(), "TestLeader", 1);
        init_character_data(officer_char.get(), "TestOfficer", 1);
        init_character_data(member_char.get(), "TestMember", 1);
        init_character_data(clanless_char.get(), "TestClanless", 1);
        init_character_data(god_char.get(), "TestGod", LVL_IMMORT);
        
        // Add characters to clan with different ranks
        [[maybe_unused]] bool leader_added = test_clan->add_member_by_name("TestLeader", 0);   // Leader rank
        [[maybe_unused]] bool officer_added = test_clan->add_member_by_name("TestOfficer", 1);  // Officer rank  
        [[maybe_unused]] bool member_added = test_clan->add_member_by_name("TestMember", 2);   // Member rank
        
        // Set clan IDs for members
        set_clan_id(leader_char.get(), 1);
        set_clan_id(officer_char.get(), 1);
        set_clan_id(member_char.get(), 1);
        set_clan_id(clanless_char.get(), CLAN_ID_NONE);
        set_clan_id(god_char.get(), CLAN_ID_NONE); // Gods don't need clan membership
    }
};

// Test ClanPermission enum and basic functionality
TEST_CASE("ClanPermission enum values", "[clan][permissions]") {
    SECTION("Basic permission values are correct") {
        REQUIRE(static_cast<int>(ClanPermission::NONE) == 0);
        REQUIRE(static_cast<int>(ClanPermission::CLAN_CHAT) == 1);
        REQUIRE(static_cast<int>(ClanPermission::CLAN_WHO) == 2);
        REQUIRE(static_cast<int>(ClanPermission::LEADER_OVERRIDE) == 51);
        REQUIRE(static_cast<int>(ClanPermission::CLAN_ADMIN) == 52);
    }
    
    SECTION("magic_enum works with ClanPermission") {
        auto name = magic_enum::enum_name(ClanPermission::CLAN_CHAT);
        REQUIRE(name == "CLAN_CHAT");
        
        auto invite_name = magic_enum::enum_name(ClanPermission::INVITE_MEMBERS);
        REQUIRE(invite_name == "INVITE_MEMBERS");
    }
}

// Test ClanRank permission checking
TEST_CASE("ClanRank permission checking", "[clan][permissions][rank]") {
    SECTION("Basic permission checking") {
        PermissionSet perms;
        perms.set(static_cast<size_t>(ClanPermission::CLAN_CHAT));
        perms.set(static_cast<size_t>(ClanPermission::INVITE_MEMBERS));
        
        ClanRank rank("TestRank", perms);
        
        REQUIRE(rank.has_permission(ClanPermission::CLAN_CHAT));
        REQUIRE(rank.has_permission(ClanPermission::INVITE_MEMBERS));
        REQUIRE_FALSE(rank.has_permission(ClanPermission::KICK_MEMBERS));
    }
    
    SECTION("LEADER_OVERRIDE grants all permissions") {
        PermissionSet perms;
        perms.set(static_cast<size_t>(ClanPermission::LEADER_OVERRIDE));
        
        ClanRank leader_rank("Leader", perms);
        
        REQUIRE(leader_rank.has_permission(ClanPermission::CLAN_CHAT));
        REQUIRE(leader_rank.has_permission(ClanPermission::INVITE_MEMBERS));
        REQUIRE(leader_rank.has_permission(ClanPermission::KICK_MEMBERS));
        REQUIRE(leader_rank.has_permission(ClanPermission::MANAGE_RANKS));
        REQUIRE(leader_rank.has_permission(ClanPermission::WITHDRAW_FUNDS));
    }
    
    SECTION("Permission modification methods") {
        ClanRank rank("TestRank", PermissionSet{});
        
        // Add permission
        rank.add_permission(ClanPermission::CLAN_CHAT);
        REQUIRE(rank.has_permission(ClanPermission::CLAN_CHAT));
        
        // Remove permission
        rank.remove_permission(ClanPermission::CLAN_CHAT);
        REQUIRE_FALSE(rank.has_permission(ClanPermission::CLAN_CHAT));
        
        // Set permission
        rank.set_permission(ClanPermission::INVITE_MEMBERS, true);
        REQUIRE(rank.has_permission(ClanPermission::INVITE_MEMBERS));
        
        rank.set_permission(ClanPermission::INVITE_MEMBERS, false);
        REQUIRE_FALSE(rank.has_permission(ClanPermission::INVITE_MEMBERS));
    }
}

// Test clan permission checking with fixture
TEST_CASE_METHOD(ClanPermissionTestFixture, "Character clan permission checking", "[clan][permissions][character]") {
    
    SECTION("Leaders have all permissions via LEADER_OVERRIDE") {
        REQUIRE(has_clan_permission(leader_char.get(), ClanPermission::CLAN_CHAT));
        REQUIRE(has_clan_permission(leader_char.get(), ClanPermission::INVITE_MEMBERS));
        REQUIRE(has_clan_permission(leader_char.get(), ClanPermission::KICK_MEMBERS));
        REQUIRE(has_clan_permission(leader_char.get(), ClanPermission::MANAGE_RANKS));
        REQUIRE(has_clan_permission(leader_char.get(), ClanPermission::WITHDRAW_FUNDS));
    }
    
    SECTION("Officers have specific permissions only") {
        REQUIRE(has_clan_permission(officer_char.get(), ClanPermission::CLAN_CHAT));
        REQUIRE(has_clan_permission(officer_char.get(), ClanPermission::INVITE_MEMBERS));
        REQUIRE(has_clan_permission(officer_char.get(), ClanPermission::KICK_MEMBERS));
        REQUIRE(has_clan_permission(officer_char.get(), ClanPermission::PROMOTE_MEMBERS));
        
        // Should NOT have these permissions
        REQUIRE_FALSE(has_clan_permission(officer_char.get(), ClanPermission::MANAGE_RANKS));
        REQUIRE_FALSE(has_clan_permission(officer_char.get(), ClanPermission::WITHDRAW_FUNDS));
    }
    
    SECTION("Members have limited permissions") {
        REQUIRE(has_clan_permission(member_char.get(), ClanPermission::CLAN_CHAT));
        REQUIRE(has_clan_permission(member_char.get(), ClanPermission::CLAN_WHO));
        
        // Should NOT have these permissions
        REQUIRE_FALSE(has_clan_permission(member_char.get(), ClanPermission::INVITE_MEMBERS));
        REQUIRE_FALSE(has_clan_permission(member_char.get(), ClanPermission::KICK_MEMBERS));
        REQUIRE_FALSE(has_clan_permission(member_char.get(), ClanPermission::MANAGE_RANKS));
    }
    
    SECTION("Clanless characters have no permissions") {
        REQUIRE_FALSE(has_clan_permission(clanless_char.get(), ClanPermission::CLAN_CHAT));
        REQUIRE_FALSE(has_clan_permission(clanless_char.get(), ClanPermission::CLAN_WHO));
        REQUIRE_FALSE(has_clan_permission(clanless_char.get(), ClanPermission::INVITE_MEMBERS));
    }
    
    SECTION("Gods have all permissions via has_clan_permission_or_god") {
        REQUIRE(has_clan_permission_or_god(god_char.get(), ClanPermission::CLAN_CHAT));
        REQUIRE(has_clan_permission_or_god(god_char.get(), ClanPermission::INVITE_MEMBERS));
        REQUIRE(has_clan_permission_or_god(god_char.get(), ClanPermission::KICK_MEMBERS));
        REQUIRE(has_clan_permission_or_god(god_char.get(), ClanPermission::MANAGE_RANKS));
        REQUIRE(has_clan_permission_or_god(god_char.get(), ClanPermission::WITHDRAW_FUNDS));
    }
}

// Test modern C++23 permission checking with std::expected
TEST_CASE_METHOD(ClanPermissionTestFixture, "Modern permission checking with std::expected", "[clan][permissions][modern]") {
    using namespace clan_permissions;
    
    SECTION("Successful permission check") {
        auto result = check_permission(leader_char.get(), ClanPermission::CLAN_CHAT);
        REQUIRE(result.has_value());
    }
    
    SECTION("Failed permission check returns error") {
        auto result = check_permission(member_char.get(), ClanPermission::KICK_MEMBERS);
        REQUIRE_FALSE(result.has_value());
        
        const auto& error = result.error();
        REQUIRE(error.required_permission == ClanPermission::KICK_MEMBERS);
        REQUIRE(error.is_clan_member == true);
        INFO("Error reason: '" << error.reason << "'");
        REQUIRE(error.reason.find("KICK_MEMBERS") != std::string::npos);
    }
    
    SECTION("Clanless character error") {
        auto result = check_permission(clanless_char.get(), ClanPermission::CLAN_CHAT);
        REQUIRE_FALSE(result.has_value());
        
        const auto& error = result.error();
        REQUIRE(error.required_permission == ClanPermission::CLAN_CHAT);
        REQUIRE(error.is_clan_member == false);
        REQUIRE(error.reason == "You are not a member of any clan.");
    }
    
    SECTION("Clan membership check") {
        auto member_result = check_clan_member(member_char.get());
        REQUIRE(member_result.has_value());
        
        auto clanless_result = check_clan_member(clanless_char.get());
        REQUIRE_FALSE(clanless_result.has_value());
        REQUIRE(clanless_result.error().reason == "You are not a member of any clan.");
    }
}

// Test permission wrapper functions
TEST_CASE_METHOD(ClanPermissionTestFixture, "Permission wrapper functions", "[clan][permissions][wrappers]") {
    using namespace clan_permissions;
    
    SECTION("execute_with_clan_permission works for valid permissions") {
        bool executed = false;
        auto command_func = [&executed]() { executed = true; };
        
        bool success = execute_with_clan_permission(leader_char.get(), ClanPermission::CLAN_CHAT, command_func);
        REQUIRE(success);
        REQUIRE(executed);
    }
    
    SECTION("execute_with_clan_permission fails for invalid permissions") {
        bool executed = false;
        auto command_func = [&executed]() { executed = true; };
        
        bool success = execute_with_clan_permission(member_char.get(), ClanPermission::KICK_MEMBERS, command_func);
        REQUIRE_FALSE(success);
        REQUIRE_FALSE(executed);
    }
    
    SECTION("execute_with_clan_membership works for clan members") {
        bool executed = false;
        auto command_func = [&executed]() { executed = true; };
        
        bool success = execute_with_clan_membership(member_char.get(), command_func);
        REQUIRE(success);
        REQUIRE(executed);
    }
    
    SECTION("execute_with_clan_membership fails for non-members") {
        bool executed = false;
        auto command_func = [&executed]() { executed = true; };
        
        bool success = execute_with_clan_membership(clanless_char.get(), command_func);
        REQUIRE_FALSE(success);
        REQUIRE_FALSE(executed);
    }
    
    SECTION("God override works in wrapper functions") {
        bool executed = false;
        auto command_func = [&executed]() { executed = true; };
        
        bool success = execute_with_clan_permission(god_char.get(), ClanPermission::MANAGE_RANKS, command_func);
        REQUIRE(success);
        REQUIRE(executed);
    }
}

// Test legacy conversion functions
TEST_CASE("Legacy permission conversion", "[clan][permissions][legacy]") {
    using namespace legacy_conversion;
    
    SECTION("Legacy privilege conversion") {
        REQUIRE(convert_legacy_privilege(1) == ClanPermission::SET_DESCRIPTION);   // Description
        REQUIRE(convert_legacy_privilege(2) == ClanPermission::SET_MOTD);          // Motd
        REQUIRE(convert_legacy_privilege(3) == ClanPermission::LEADER_OVERRIDE);   // Grant
        REQUIRE(convert_legacy_privilege(6) == ClanPermission::INVITE_MEMBERS);    // Enroll
        REQUIRE(convert_legacy_privilege(7) == ClanPermission::KICK_MEMBERS);      // Expel
        REQUIRE(convert_legacy_privilege(18) == ClanPermission::CLAN_CHAT);        // Chat
        
        // Invalid legacy values
        REQUIRE(convert_legacy_privilege(99) == std::nullopt);
        REQUIRE(convert_legacy_privilege(-1) == std::nullopt);
    }
    
    SECTION("Legacy bitset conversion") {
        std::bitset<64> legacy_bits;
        legacy_bits.set(1);  // Description
        legacy_bits.set(6);  // Enroll (invite)
        legacy_bits.set(18); // Chat
        
        auto new_permissions = convert_legacy_permissions(legacy_bits);
        
        REQUIRE(new_permissions.test(static_cast<size_t>(ClanPermission::SET_DESCRIPTION)));
        REQUIRE(new_permissions.test(static_cast<size_t>(ClanPermission::INVITE_MEMBERS)));
        REQUIRE(new_permissions.test(static_cast<size_t>(ClanPermission::CLAN_CHAT)));
        REQUIRE_FALSE(new_permissions.test(static_cast<size_t>(ClanPermission::KICK_MEMBERS)));
    }
}

// Test error cases and edge conditions
TEST_CASE_METHOD(ClanPermissionTestFixture, "Permission system edge cases", "[clan][permissions][edge]") {
    
    SECTION("Null character pointer") {
        REQUIRE_FALSE(has_clan_permission(nullptr, ClanPermission::CLAN_CHAT));
        REQUIRE_FALSE(has_clan_permission_or_god(nullptr, ClanPermission::CLAN_CHAT));
    }
    
    SECTION("Invalid permission values") {
        ClanRank rank("Test", PermissionSet{});
        
        // Test with out-of-bounds permission (should be safe)
        auto invalid_perm = static_cast<ClanPermission>(999);
        REQUIRE_FALSE(rank.has_permission(invalid_perm));
    }
    
    SECTION("Character with invalid clan ID") {
        auto invalid_char = std::make_shared<CharData>();
        init_character_data(invalid_char.get(), "Invalid", 1);
        set_clan_id(invalid_char.get(), 999); // Non-existent clan
        
        REQUIRE_FALSE(has_clan_permission(invalid_char.get(), ClanPermission::CLAN_CHAT));
        
        using namespace clan_permissions;
        auto result = check_permission(invalid_char.get(), ClanPermission::CLAN_CHAT);
        REQUIRE_FALSE(result.has_value());
    }
}