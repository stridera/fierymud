/**
 * @file db_parsing_utils.cpp
 * @brief Implementation of PostgreSQL parsing utilities
 */

#include "db_parsing_utils.hpp"

#include <unordered_map>

namespace DbParsingUtils {

std::vector<std::string> parse_pg_array(const std::string &pg_array) {
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

std::optional<SectorType> sector_from_db_string(const std::string &sector_str) {
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
    return std::nullopt; // Returns nullopt for unknown instead of default
}

// room_flag_from_db_string REMOVED - RoomFlag replaced by baseLightLevel and Lua restrictions

} // namespace DbParsingUtils
