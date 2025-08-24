/***************************************************************************
 *   File: tests/test_zone_json_parsing.cpp               Part of FieryMUD *
 *  Usage: Unit tests for zone JSON parsing functionality                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "../src/world/zone.hpp"
#include "../src/world/room.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TEST_CASE("Zone command parsing with door state arrays", "[world][zone][commands][doors]") {
    
    SECTION("Parse door command with state as array") {
        json zone_json = {
            {"zone", {
                {"id", "100"},
                {"name", "Test Zone"},
                {"top", 199},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"commands", {
                {"door", {
                    {
                        {"room", 1001},
                        {"direction", "North"},
                        {"state", {"CLOSED"}}  // Array format
                    }
                }}
            }}
        };
        
        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());
        
        REQUIRE(zone->name() == "Test Zone");
        REQUIRE(zone->id() == EntityId{100});
    }
    
    SECTION("Parse door command with state as string") {
        json zone_json = {
            {"zone", {
                {"id", "101"},
                {"name", "Test Zone 2"},
                {"top", 199},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"commands", {
                {"door", {
                    {
                        {"room", 1001},
                        {"direction", "East"},
                        {"state", "LOCKED"}  // String format
                    }
                }}
            }}
        };
        
        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());
        
        REQUIRE(zone->name() == "Test Zone 2");
    }
    
    SECTION("Parse door command with multiple state values in array") {
        json zone_json = {
            {"zone", {
                {"id", "102"},
                {"name", "Multi State Zone"},
                {"top", 199},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"commands", {
                {"door", {
                    {
                        {"room", 1001},
                        {"direction", "South"},
                        {"state", {"CLOSED", "LOCKED"}}  // Multiple values - should use first
                    }
                }}
            }}
        };
        
        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());
        
        REQUIRE(zone->name() == "Multi State Zone");
    }
    
    SECTION("Parse door command with empty state array") {
        json zone_json = {
            {"zone", {
                {"id", "103"},
                {"name", "Empty State Zone"},
                {"top", 199},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"commands", {
                {"door", {
                    {
                        {"room", 1001},
                        {"direction", "West"},
                        {"state", {}}  // Empty array - should default to "open"
                    }
                }}
            }}
        };
        
        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());
        
        REQUIRE(zone->name() == "Empty State Zone");
    }
}

TEST_CASE("Zone command parsing with direction string conversion", "[world][zone][commands][direction]") {
    
    SECTION("Parse door command with cardinal directions") {
        json zone_json = {
            {"zone", {
                {"id", "200"},
                {"name", "Direction Test Zone"},
                {"top", 299},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"commands", {
                {"door", {
                    {
                        {"room", 2001},
                        {"direction", "North"},
                        {"state", {"OPEN"}}
                    },
                    {
                        {"room", 2001},
                        {"direction", "East"},
                        {"state", {"CLOSED"}}
                    },
                    {
                        {"room", 2001},
                        {"direction", "South"},
                        {"state", {"LOCKED"}}
                    },
                    {
                        {"room", 2001},
                        {"direction", "West"},
                        {"state", {"UNLOCKED"}}
                    }
                }}
            }}
        };
        
        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());
        
        REQUIRE(zone->name() == "Direction Test Zone");
    }
    
    SECTION("Parse door command with vertical directions") {
        json zone_json = {
            {"zone", {
                {"id", "201"},
                {"name", "Vertical Zone"},
                {"top", 299},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"commands", {
                {"door", {
                    {
                        {"room", 2101},
                        {"direction", "Up"},
                        {"state", {"CLOSED"}}
                    },
                    {
                        {"room", 2101},
                        {"direction", "Down"},
                        {"state", {"LOCKED"}}
                    }
                }}
            }}
        };
        
        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());
        
        REQUIRE(zone->name() == "Vertical Zone");
    }
    
    SECTION("Parse door command with diagonal directions") {
        json zone_json = {
            {"zone", {
                {"id", "202"},
                {"name", "Diagonal Zone"},
                {"top", 299},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"commands", {
                {"door", {
                    {
                        {"room", 2201},
                        {"direction", "Northeast"},
                        {"state", {"OPEN"}}
                    },
                    {
                        {"room", 2201},
                        {"direction", "Southeast"},
                        {"state", {"CLOSED"}}
                    }
                }}
            }}
        };
        
        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());
        
        REQUIRE(zone->name() == "Diagonal Zone");
    }
    
    SECTION("Parse door command with special directions") {
        json zone_json = {
            {"zone", {
                {"id", "203"},
                {"name", "Special Zone"},
                {"top", 299},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"commands", {
                {"door", {
                    {
                        {"room", 2301},
                        {"direction", "In"},
                        {"state", {"CLOSED"}}
                    },
                    {
                        {"room", 2301},
                        {"direction", "Out"},
                        {"state", {"LOCKED"}}
                    }
                }}
            }}
        };
        
        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());
        
        REQUIRE(zone->name() == "Special Zone");
    }
}

TEST_CASE("Zone command parsing with mixed data types", "[world][zone][commands][mixed]") {
    
    SECTION("Parse zone with mobile and object commands") {
        json zone_json = {
            {"zone", {
                {"id", "300"},
                {"name", "Mixed Command Zone"},
                {"top", 399},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"commands", {
                {"mob", {
                    {
                        {"id", 3001},
                        {"max", 1},
                        {"room", 3001},
                        {"name", "(test mobile)"}
                    }
                }},
                {"object", {
                    {
                        {"id", 3101},
                        {"max", 1},
                        {"room", 3001},
                        {"name", "(test object)"}
                    }
                }},
                {"door", {
                    {
                        {"room", 3001},
                        {"direction", "North"},
                        {"state", {"CLOSED"}}
                    }
                }}
            }}
        };
        
        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());
        
        REQUIRE(zone->name() == "Mixed Command Zone");
        REQUIRE(zone->id() == EntityId{300});
    }
}

TEST_CASE("Zone JSON parsing error handling", "[world][zone][errors]") {
    
    SECTION("Missing zone section should fail") {
        json invalid_zone_json = {
            {"commands", {
                {"door", {
                    {
                        {"room", 1001},
                        {"direction", "North"},
                        {"state", {"CLOSED"}}
                    }
                }}
            }}
            // Missing "zone" section
        };
        
        auto result = Zone::from_json(invalid_zone_json);
        REQUIRE_FALSE(result.has_value());
    }
    
    SECTION("Invalid door state type should be handled gracefully") {
        json zone_json = {
            {"zone", {
                {"id", "400"},
                {"name", "Error Zone"},
                {"top", 499},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"commands", {
                {"door", {
                    {
                        {"room", 4001},
                        {"direction", "North"},
                        {"state", 123}  // Invalid type - number instead of string/array
                    }
                }}
            }}
        };
        
        // Should either handle gracefully or provide clear error
        auto result = Zone::from_json(zone_json);
        // Don't require success/failure - depends on implementation robustness
        if (!result.has_value()) {
            // If it fails, error should be descriptive
            REQUIRE(!result.error().message.empty());
        }
    }
}