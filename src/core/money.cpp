/**
 * @file money.cpp
 * @brief Implementation of the Money class
 */

#include "money.hpp"
#include "logging.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace fiery {

// =============================================================================
// Parsing Functions
// =============================================================================

std::optional<CoinType> parse_coin_type(std::string_view input) {
    // Convert to lowercase for comparison
    std::string lower;
    lower.reserve(input.size());
    for (char c : input) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    // Check each coin type
    for (int i = 0; i < NUM_COIN_TYPES; ++i) {
        const auto& def = COIN_DEFS[i];
        if (lower == def.name || lower == def.short_name || lower == def.initial) {
            return static_cast<CoinType>(i);
        }
    }

    return std::nullopt;
}

std::optional<Money> parse_money(std::string_view input) {
    long total_copper = 0;

    // Trim whitespace
    while (!input.empty() && std::isspace(static_cast<unsigned char>(input.front()))) {
        input.remove_prefix(1);
    }
    while (!input.empty() && std::isspace(static_cast<unsigned char>(input.back()))) {
        input.remove_suffix(1);
    }

    if (input.empty()) {
        return std::nullopt;
    }

    // Parse tokens
    size_t pos = 0;
    while (pos < input.size()) {
        // Skip whitespace
        while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) {
            ++pos;
        }
        if (pos >= input.size()) break;

        // Parse number
        size_t num_start = pos;
        while (pos < input.size() && std::isdigit(static_cast<unsigned char>(input[pos]))) {
            ++pos;
        }

        if (pos == num_start) {
            // No number found
            return std::nullopt;
        }

        long amount = 0;
        for (size_t i = num_start; i < pos; ++i) {
            amount = amount * 10 + (input[i] - '0');
        }

        if (amount <= 0) {
            return std::nullopt;
        }

        // Skip whitespace between number and type
        while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) {
            ++pos;
        }

        if (pos >= input.size()) {
            // Number without type - assume copper
            total_copper += amount;
            break;
        }

        // Parse coin type
        size_t type_start = pos;
        while (pos < input.size() &&
               !std::isspace(static_cast<unsigned char>(input[pos])) &&
               !std::isdigit(static_cast<unsigned char>(input[pos]))) {
            ++pos;
        }

        std::string_view type_str = input.substr(type_start, pos - type_start);
        auto coin_type = parse_coin_type(type_str);

        if (!coin_type) {
            // Unknown coin type - assume copper
            total_copper += amount;
        } else {
            total_copper += amount * get_coin_value(*coin_type);
        }
    }

    if (total_copper <= 0) {
        return std::nullopt;
    }

    return Money(total_copper);
}

std::string_view get_money_pile_description(long copper_value) {
    // Convert to rough coin count for description (assume mostly copper)
    if (copper_value <= 1) return "coin";
    if (copper_value <= 9) return "tiny pile of coins";
    if (copper_value <= 20) return "handful of coins";
    if (copper_value <= 75) return "little pile of coins";
    if (copper_value <= 200) return "small pile of coins";
    if (copper_value <= 1000) return "pile of coins";
    if (copper_value <= 5000) return "big pile of coins";
    if (copper_value <= 10000) return "large heap of coins";
    if (copper_value <= 20000) return "huge mound of coins";
    if (copper_value <= 75000) return "enormous mound of coins";
    if (copper_value <= 150000) return "small mountain of coins";
    if (copper_value <= 250000) return "mountain of coins";
    if (copper_value <= 500000) return "huge mountain of coins";
    if (copper_value <= 1000000) return "enormous mountain of coins";
    return "colossal mountain of coins";
}

// =============================================================================
// Money Class Implementation
// =============================================================================

Money::Money(const nlohmann::json& json) {
    // Support both old format (separate coins) and new format (copper only)
    if (json.contains("copper") && json.is_object()) {
        if (json.contains("platinum") || json.contains("gold") || json.contains("silver")) {
            // Old format with separate denominations
            long plat = json.value("platinum", 0);
            long gold = json.value("gold", 0);
            long silver = json.value("silver", 0);
            long copper = json.value("copper", 0);
            copper_ = plat * PLATINUM_VALUE + gold * GOLD_VALUE +
                      silver * SILVER_VALUE + copper;
        } else {
            // New format with just copper value
            copper_ = json.value("copper", 0L);
        }
    } else if (json.is_number()) {
        // Direct copper value
        copper_ = json.get<long>();
    } else {
        copper_ = 0;
    }

    if (copper_ < 0) copper_ = 0;
}

int Money::get(CoinType type) const noexcept {
    switch (type) {
        case CoinType::Platinum: return platinum();
        case CoinType::Gold: return gold();
        case CoinType::Silver: return silver();
        case CoinType::Copper: return copper();
    }
    return 0;
}

bool Money::charge(long copper_cost) {
    if (copper_cost <= 0) {
        return true;  // Nothing to charge
    }

    if (copper_ < copper_cost) {
        return false;  // Insufficient funds
    }

    copper_ -= copper_cost;
    return true;
}

// =============================================================================
// Serialization
// =============================================================================

nlohmann::json Money::to_json() const {
    // Store as simple copper value
    return nlohmann::json{{"copper", copper_}};
}

Money Money::from_json(const nlohmann::json& json) {
    return Money(json);
}

// =============================================================================
// String Formatting
// =============================================================================

std::string Money::to_string(bool include_color) const {
    if (is_zero()) {
        return "nothing";
    }

    std::vector<std::string> parts;

    int plat = platinum();
    int gol = gold();
    int sil = silver();
    int cop = copper();

    // Track what the last denomination with coins is for plural
    bool final_plural = false;

    if (plat > 0) {
        if (include_color) {
            parts.push_back(fmt::format("{}{} platinum{}",
                COIN_DEFS[0].open_tag, plat, COIN_DEFS[0].close_tag));
        } else {
            parts.push_back(fmt::format("{} platinum", plat));
        }
        final_plural = (plat > 1);
    }

    if (gol > 0) {
        if (include_color) {
            parts.push_back(fmt::format("{}{} gold{}",
                COIN_DEFS[1].open_tag, gol, COIN_DEFS[1].close_tag));
        } else {
            parts.push_back(fmt::format("{} gold", gol));
        }
        final_plural = (gol > 1);
    }

    if (sil > 0) {
        if (include_color) {
            parts.push_back(fmt::format("{}{} silver{}",
                COIN_DEFS[2].open_tag, sil, COIN_DEFS[2].close_tag));
        } else {
            parts.push_back(fmt::format("{} silver", sil));
        }
        final_plural = (sil > 1);
    }

    if (cop > 0) {
        if (include_color) {
            parts.push_back(fmt::format("{}{} copper{}",
                COIN_DEFS[3].open_tag, cop, COIN_DEFS[3].close_tag));
        } else {
            parts.push_back(fmt::format("{} copper", cop));
        }
        final_plural = (cop > 1);
    }

    // Build the string with proper conjunctions
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            if (i == parts.size() - 1) {
                result += " and ";
            } else {
                result += ", ";
            }
        }
        result += parts[i];
    }

    result += final_plural ? " coins" : " coin";
    return result;
}

std::string Money::to_brief(int max_width) const {
    if (is_zero()) {
        return "0c";
    }

    std::string result;

    int plat = platinum();
    int gol = gold();
    int sil = silver();
    int cop = copper();

    if (plat > 0) {
        result += fmt::format("{}p", plat);
    }
    if (gol > 0) {
        result += fmt::format("{}g", gol);
    }
    if (sil > 0) {
        result += fmt::format("{}s", sil);
    }
    if (cop > 0) {
        result += fmt::format("{}c", cop);
    }

    // Truncate if needed
    if (max_width > 0 && static_cast<int>(result.size()) > max_width) {
        // Show just the total value in copper if too long
        result = fmt::format("{}c", copper_);
        if (static_cast<int>(result.size()) > max_width) {
            result = result.substr(0, max_width);
        }
    }

    return result;
}

std::string Money::to_shop_format(bool include_color) const {
    if (is_zero()) {
        if (include_color) {
            return fmt::format("{}0{}c", COIN_DEFS[3].open_tag, COIN_DEFS[3].close_tag);
        }
        return "0c";
    }

    int plat = platinum();
    int gol = gold();
    int sil = silver();
    int cop = copper();

    std::string result;

    // Only show non-zero denominations
    if (plat > 0) {
        if (include_color) {
            result += fmt::format("{}{}{}p", COIN_DEFS[0].open_tag, plat, COIN_DEFS[0].close_tag);
        } else {
            result += fmt::format("{}p", plat);
        }
    }
    if (gol > 0) {
        if (!result.empty()) result += ",";
        if (include_color) {
            result += fmt::format("{}{}{}g", COIN_DEFS[1].open_tag, gol, COIN_DEFS[1].close_tag);
        } else {
            result += fmt::format("{}g", gol);
        }
    }
    if (sil > 0) {
        if (!result.empty()) result += ",";
        if (include_color) {
            result += fmt::format("{}{}{}s", COIN_DEFS[2].open_tag, sil, COIN_DEFS[2].close_tag);
        } else {
            result += fmt::format("{}s", sil);
        }
    }
    if (cop > 0) {
        if (!result.empty()) result += ",";
        if (include_color) {
            result += fmt::format("{}{}{}c", COIN_DEFS[3].open_tag, cop, COIN_DEFS[3].close_tag);
        } else {
            result += fmt::format("{}c", cop);
        }
    }

    return result;
}

} // namespace fiery
