#include <catch2/catch_test_macros.hpp>
#include "clan.hpp"

TEST_CASE("Clan Snoop Basic Functionality", "[clan][snoop]") {
    // Clear any existing clan snoops to start clean
    clan_snoop_table.clear();
    
    SECTION("Add and remove clan snoop") {
        CharData test_char{};
        ClanID test_clan_id = 1;
        
        // Initially not snooping
        REQUIRE_FALSE(is_snooping_clan(&test_char, test_clan_id));
        
        // Add snoop
        add_clan_snoop(&test_char, test_clan_id);
        REQUIRE(is_snooping_clan(&test_char, test_clan_id));
        
        // Remove snoop
        remove_clan_snoop(&test_char, test_clan_id);
        REQUIRE_FALSE(is_snooping_clan(&test_char, test_clan_id));
    }
    
    SECTION("Multiple clan snooping") {
        CharData test_char{};
        ClanID clan1 = 1;
        ClanID clan2 = 2;
        
        // Add multiple snoops
        add_clan_snoop(&test_char, clan1);
        add_clan_snoop(&test_char, clan2);
        
        REQUIRE(is_snooping_clan(&test_char, clan1));
        REQUIRE(is_snooping_clan(&test_char, clan2));
        
        auto snooped_clans = get_snooped_clans(&test_char);
        REQUIRE(snooped_clans.size() == 2);
        
        // Remove all snoops
        remove_all_clan_snoops(&test_char);
        REQUIRE_FALSE(is_snooping_clan(&test_char, clan1));
        REQUIRE_FALSE(is_snooping_clan(&test_char, clan2));
        
        snooped_clans = get_snooped_clans(&test_char);
        REQUIRE(snooped_clans.empty());
    }
    
    SECTION("Multiple characters snooping same clan") {
        CharData char1{};
        CharData char2{};
        ClanID test_clan_id = 1;
        
        add_clan_snoop(&char1, test_clan_id);
        add_clan_snoop(&char2, test_clan_id);
        
        REQUIRE(is_snooping_clan(&char1, test_clan_id));
        REQUIRE(is_snooping_clan(&char2, test_clan_id));
        
        // Remove one character's snoop
        remove_clan_snoop(&char1, test_clan_id);
        REQUIRE_FALSE(is_snooping_clan(&char1, test_clan_id));
        REQUIRE(is_snooping_clan(&char2, test_clan_id));
        
        // Remove the other character's snoop
        remove_clan_snoop(&char2, test_clan_id);
        REQUIRE_FALSE(is_snooping_clan(&char2, test_clan_id));
        
        // Table should be clean
        REQUIRE(clan_snoop_table.empty());
    }
    
    SECTION("Null character handling") {
        ClanID test_clan_id = 1;
        
        // Functions should handle null pointers gracefully
        REQUIRE_FALSE(is_snooping_clan(nullptr, test_clan_id));
        
        add_clan_snoop(nullptr, test_clan_id);
        remove_clan_snoop(nullptr, test_clan_id);
        remove_all_clan_snoops(nullptr);
        
        auto result = get_snooped_clans(nullptr);
        REQUIRE(result.empty());
    }
    
    // Clean up
    clan_snoop_table.clear();
}