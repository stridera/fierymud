/**
 * @file db_parsing_utils.cpp
 * @brief Implementation of PostgreSQL parsing utilities
 */

#include "db_parsing_utils.hpp"
#include <unordered_map>

namespace DbParsingUtils {

std::vector<std::string> parse_pg_array(const std::string& pg_array) {
    std::vector<std::string> result;

    if (pg_array.empty() || pg_array == "{}") {
        return result;
    }

    // Remove outer braces
    std::string content = pg_array;
    if (content.front() == '{' && content.back() == '}') {
        content = content.substr(1, content.length() - 2);
    }

    if (content.empty()) {
        return result;
    }

    // Parse comma-separated values, handling quoted strings
    std::string current;
    bool in_quotes = false;
    bool escape_next = false;

    for (size_t i = 0; i < content.size(); ++i) {
        char c = content[i];

        if (escape_next) {
            current += c;
            escape_next = false;
            continue;
        }

        if (c == '\\') {
            escape_next = true;
            continue;
        }

        if (c == '"') {
            in_quotes = !in_quotes;
            continue;
        }

        if (c == ',' && !in_quotes) {
            if (!current.empty()) {
                result.push_back(current);
                current.clear();
            }
            continue;
        }

        current += c;
    }

    // Don't forget the last element
    if (!current.empty()) {
        result.push_back(current);
    }

    return result;
}

std::optional<SectorType> sector_from_db_string(const std::string& sector_str) {
    static const std::unordered_map<std::string, SectorType> sector_map = {
        {"STRUCTURE", SectorType::Inside},
        {"CITY", SectorType::City},
        {"FIELD", SectorType::Field},
        {"FOREST", SectorType::Forest},
        {"HILLS", SectorType::Hills},
        {"MOUNTAIN", SectorType::Mountains},
        {"SHALLOWS", SectorType::Water_Swim},
        {"WATER", SectorType::Water_Noswim},
        {"UNDERWATER", SectorType::Underwater},
        {"AIR", SectorType::Flying},
        {"ROAD", SectorType::Road},
        {"GRASSLANDS", SectorType::Field},
        {"CAVE", SectorType::Underground},
        {"RUINS", SectorType::Inside},
        {"SWAMP", SectorType::Swamp},
        {"BEACH", SectorType::Beach},
        {"UNDERDARK", SectorType::Underground},
        {"ASTRALPLANE", SectorType::Astral},
        {"AIRPLANE", SectorType::Flying},
        {"FIREPLANE", SectorType::Fire},
        {"EARTHPLANE", SectorType::Underground},
        {"ETHEREALPLANE", SectorType::Spirit},
        {"AVERNUS", SectorType::Fire},
    };

    auto it = sector_map.find(sector_str);
    if (it != sector_map.end()) {
        return it->second;
    }
    return std::nullopt;  // Returns nullopt for unknown instead of default
}

std::optional<RoomFlag> room_flag_from_db_string(const std::string& flag_str) {
    static const std::unordered_map<std::string, RoomFlag> flag_map = {
        // Core flags
        {"DARK", RoomFlag::Dark},
        {"DEATH", RoomFlag::Death},
        {"NO_MOB", RoomFlag::NoMob},
        {"INDOORS", RoomFlag::Indoors},
        {"PEACEFUL", RoomFlag::Peaceful},
        {"SOUNDPROOF", RoomFlag::Soundproof},
        {"NO_TRACK", RoomFlag::NoTrack},
        {"NO_MAGIC", RoomFlag::NoMagic},
        {"TUNNEL", RoomFlag::Tunnel},
        {"PRIVATE", RoomFlag::Private},
        {"GODROOM", RoomFlag::Godroom},
        {"HOUSE", RoomFlag::House},
        {"HOUSE_CRASH", RoomFlag::HouseCrash},
        {"HOUSECRASH", RoomFlag::HouseCrash},  // Alternative spelling from Prisma
        {"ATRIUM", RoomFlag::Atrium},
        {"OLC", RoomFlag::OLC},
        {"BFS_MARK", RoomFlag::BFS_Mark},
        {"VEHICLE", RoomFlag::Vehicle},
        {"UNDERGROUND", RoomFlag::Underground},
        {"CURRENT", RoomFlag::Current},
        {"TIMED_DT", RoomFlag::Timed_DT},
        {"EARTH_SECT", RoomFlag::Earth},
        {"AIR_SECT", RoomFlag::Air},
        {"FIRE_SECT", RoomFlag::Fire},
        {"WATER_SECT", RoomFlag::Water},
        {"NO_RECALL", RoomFlag::NoRecall},
        {"NO_SUMMON", RoomFlag::NoSummon},
        {"NO_LOCATE", RoomFlag::NoLocate},
        {"NO_TELEPORT", RoomFlag::NoTeleport},
        {"ALWAYS_LIT", RoomFlag::AlwaysLit},

        // Special flags
        {"ARENA", RoomFlag::Arena},
        {"SHOP", RoomFlag::Shop},
        {"TEMPLE", RoomFlag::Temple},
        {"BANK", RoomFlag::Bank},

        // Map and navigation flags
        {"WORLDMAP", RoomFlag::Worldmap},
        {"FERRY_DEST", RoomFlag::FerryDest},
        {"ISOLATED", RoomFlag::Isolated},
        {"ALT_EXIT", RoomFlag::AltExit},
        {"OBSERVATORY", RoomFlag::Observatory},

        // Room size flags
        {"LARGE", RoomFlag::Large},
        {"MEDIUM_LARGE", RoomFlag::MediumLarge},
        {"MEDIUM", RoomFlag::Medium},
        {"MEDIUM_SMALL", RoomFlag::MediumSmall},
        {"SMALL", RoomFlag::Small},
        {"VERY_SMALL", RoomFlag::VerySmall},
        {"ONE_PERSON", RoomFlag::OnePerson},

        // Special ability flags
        {"GUILDHALL", RoomFlag::Guildhall},
        {"NO_WELL", RoomFlag::NoWell},
        {"NO_SCAN", RoomFlag::NoScan},
        {"UNDERDARK", RoomFlag::Underdark},
        {"NO_SHIFT", RoomFlag::NoShift},
        {"EFFECTS_NEXT", RoomFlag::EffectsNext},
    };

    auto it = flag_map.find(flag_str);
    if (it != flag_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace DbParsingUtils
