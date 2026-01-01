/**
 * @file money.hpp
 * @brief Money class for handling currency in FieryMUD
 *
 * Internally stores all money as copper for simplicity. Displays as
 * platinum/gold/silver/copper denominations for immersion. No need for
 * money changers or exact change - the system handles conversions automatically.
 */

#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <nlohmann/json.hpp>
#include <fmt/format.h>

namespace fiery {

/**
 * Currency denomination types (for display and parsing)
 */
enum class CoinType {
    Platinum = 0,
    Gold = 1,
    Silver = 2,
    Copper = 3
};

// Number of coin types
constexpr int NUM_COIN_TYPES = 4;

// Exchange rates (value in copper)
constexpr long PLATINUM_VALUE = 1000;
constexpr long GOLD_VALUE = 100;
constexpr long SILVER_VALUE = 10;
constexpr long COPPER_VALUE = 1;

/**
 * Coin definition for display and parsing
 */
struct CoinDef {
    std::string_view name;        // Full name (e.g., "platinum")
    std::string_view short_name;  // Short name (e.g., "plat")
    std::string_view initial;     // Single letter (e.g., "p")
    std::string_view open_tag;    // XML-lite markup opening tag
    std::string_view close_tag;   // XML-lite markup closing tag
    long value;                   // Value in copper
};

// Coin definitions array with XML-lite markup tags
// Platinum: bold white, Gold: bold yellow, Silver: white, Copper: yellow
inline constexpr CoinDef COIN_DEFS[NUM_COIN_TYPES] = {
    {"platinum", "plat", "p", "<b:white>", "</b>", PLATINUM_VALUE},
    {"gold", "gol", "g", "<b:yellow>", "</b>", GOLD_VALUE},
    {"silver", "sil", "s", "<white>", "</white>", SILVER_VALUE},
    {"copper", "cop", "c", "<yellow>", "</yellow>", COPPER_VALUE}
};

/**
 * Get coin definition for a coin type
 */
inline const CoinDef& get_coin_def(CoinType type) {
    return COIN_DEFS[static_cast<int>(type)];
}

/**
 * Get coin value in copper for a coin type
 */
inline long get_coin_value(CoinType type) {
    return COIN_DEFS[static_cast<int>(type)].value;
}

/**
 * Parse a coin type from a string (e.g., "gold", "g", "gol")
 * @return The coin type, or std::nullopt if not recognized
 */
std::optional<CoinType> parse_coin_type(std::string_view input);

/**
 * @class Money
 * @brief Represents currency as a copper value with denomination display
 *
 * Internally stores everything as copper for simplicity. When displayed,
 * automatically converts to the most readable denomination format.
 * No need for exact change - transactions work on total value.
 *
 * Example usage:
 * @code
 *   Money purse = Money::gold(5) + Money::silver(3);  // 530 copper
 *   Money cost = Money::gold(2);  // 200 copper
 *
 *   if (purse.can_afford(cost)) {
 *       purse -= cost;  // Now 330 copper, displays as "3 gold, 3 silver"
 *   }
 * @endcode
 */
class Money {
public:
    /** Default constructor - zero value */
    constexpr Money() = default;

    /** Constructor from copper value */
    constexpr explicit Money(long copper) : copper_(copper < 0 ? 0 : copper) {}

    /** Constructor from individual coin amounts (converted to copper) */
    constexpr Money(int platinum, int gold, int silver, int copper)
        : copper_(static_cast<long>(platinum) * PLATINUM_VALUE +
                  static_cast<long>(gold) * GOLD_VALUE +
                  static_cast<long>(silver) * SILVER_VALUE +
                  static_cast<long>(copper)) {}

    /** Constructor from JSON */
    explicit Money(const nlohmann::json& json);

    // =========================================================================
    // Factory Methods
    // =========================================================================

    /** Create Money from copper amount */
    static constexpr Money copper(long amount) { return Money(amount); }

    /** Create Money from silver amount */
    static constexpr Money silver(long amount) { return Money(amount * SILVER_VALUE); }

    /** Create Money from gold amount */
    static constexpr Money gold(long amount) { return Money(amount * GOLD_VALUE); }

    /** Create Money from platinum amount */
    static constexpr Money platinum(long amount) { return Money(amount * PLATINUM_VALUE); }

    /** Create Money from copper value (alias for compatibility) */
    static Money from_copper(long copper_value) { return Money(copper_value); }

    // =========================================================================
    // Value Access
    // =========================================================================

    /** Get total value in copper */
    [[nodiscard]] constexpr long value() const noexcept { return copper_; }

    /** Check if value is zero */
    [[nodiscard]] constexpr bool is_zero() const noexcept { return copper_ == 0; }

    /** Check if value is negative (shouldn't happen, but for safety) */
    [[nodiscard]] constexpr bool is_negative() const noexcept { return copper_ < 0; }

    // =========================================================================
    // Denomination Display (calculated from copper)
    // =========================================================================

    /** Get platinum portion for display */
    [[nodiscard]] constexpr int platinum() const noexcept {
        return static_cast<int>(copper_ / PLATINUM_VALUE);
    }

    /** Get gold portion for display (after platinum) */
    [[nodiscard]] constexpr int gold() const noexcept {
        return static_cast<int>((copper_ % PLATINUM_VALUE) / GOLD_VALUE);
    }

    /** Get silver portion for display (after gold) */
    [[nodiscard]] constexpr int silver() const noexcept {
        return static_cast<int>((copper_ % GOLD_VALUE) / SILVER_VALUE);
    }

    /** Get copper portion for display (remainder) */
    [[nodiscard]] constexpr int copper() const noexcept {
        return static_cast<int>(copper_ % SILVER_VALUE);
    }

    /** Get coin count by type (for display) */
    [[nodiscard]] int get(CoinType type) const noexcept;

    // =========================================================================
    // Affordability
    // =========================================================================

    /** Check if this money can afford a cost (in copper) */
    [[nodiscard]] constexpr bool can_afford(long copper_cost) const noexcept {
        return copper_ >= copper_cost;
    }

    /** Check if this money can afford another Money amount */
    [[nodiscard]] constexpr bool can_afford(const Money& cost) const noexcept {
        return copper_ >= cost.copper_;
    }

    /**
     * Deduct an amount from this money
     * @param copper_cost The cost in copper to deduct
     * @return true if successful, false if insufficient funds
     */
    bool charge(long copper_cost);

    /**
     * Deduct a Money amount from this money
     * @param cost The Money amount to deduct
     * @return true if successful, false if insufficient funds
     */
    bool charge(const Money& cost) { return charge(cost.copper_); }

    /**
     * Add copper to this money
     * @param copper_amount The copper amount to add
     */
    void receive(long copper_amount) {
        if (copper_amount > 0) copper_ += copper_amount;
    }

    /**
     * Transfer all money from another Money object to this one
     * @param other The money to take (will be zeroed out)
     */
    void transfer_from(Money& other) {
        copper_ += other.copper_;
        other.copper_ = 0;
    }

    // =========================================================================
    // Arithmetic Operators
    // =========================================================================

    Money operator+(const Money& other) const noexcept {
        return Money(copper_ + other.copper_);
    }

    Money& operator+=(const Money& other) noexcept {
        copper_ += other.copper_;
        return *this;
    }

    Money operator-(const Money& other) const noexcept {
        return Money(copper_ - other.copper_);
    }

    Money& operator-=(const Money& other) noexcept {
        copper_ -= other.copper_;
        if (copper_ < 0) copper_ = 0;
        return *this;
    }

    Money operator*(long multiplier) const noexcept {
        return Money(copper_ * multiplier);
    }

    Money& operator*=(long multiplier) noexcept {
        copper_ *= multiplier;
        return *this;
    }

    Money operator/(long divisor) const noexcept {
        if (divisor == 0) return *this;
        return Money(copper_ / divisor);
    }

    Money& operator/=(long divisor) noexcept {
        if (divisor != 0) copper_ /= divisor;
        return *this;
    }

    // =========================================================================
    // Comparison Operators
    // =========================================================================

    bool operator==(const Money& other) const noexcept { return copper_ == other.copper_; }
    bool operator!=(const Money& other) const noexcept { return copper_ != other.copper_; }
    bool operator<(const Money& other) const noexcept { return copper_ < other.copper_; }
    bool operator<=(const Money& other) const noexcept { return copper_ <= other.copper_; }
    bool operator>(const Money& other) const noexcept { return copper_ > other.copper_; }
    bool operator>=(const Money& other) const noexcept { return copper_ >= other.copper_; }

    // =========================================================================
    // Serialization
    // =========================================================================

    /** Convert to JSON (stores as copper for simplicity) */
    [[nodiscard]] nlohmann::json to_json() const;

    /** Load from JSON */
    static Money from_json(const nlohmann::json& json);

    // =========================================================================
    // String Formatting
    // =========================================================================

    /**
     * Format money as a human-readable string
     * Example: "5 gold, 3 silver, and 10 copper coins"
     * @param include_color If true, include XML-lite color markup
     */
    [[nodiscard]] std::string to_string(bool include_color = false) const;

    /**
     * Format money in brief notation for space-constrained displays
     * Example: "5g3s10c" or "1p2g"
     * @param max_width Maximum width to use (0 = no limit)
     */
    [[nodiscard]] std::string to_brief(int max_width = 0) const;

    /**
     * Format money for shop listings with comma-separated denominations
     * Only shows non-zero denominations for cleaner display
     * Examples: "272p,2g,5s" or "4g,3s,5c" or "5s,4c"
     * @param include_color If true, include XML-lite color markup for each coin type
     */
    [[nodiscard]] std::string to_shop_format(bool include_color = false) const;

private:
    long copper_ = 0;
};

/**
 * Parse money from a string like "5 gold 3 silver" or "5g3s"
 * @param input The string to parse
 * @return The parsed Money, or std::nullopt if parsing failed
 */
std::optional<Money> parse_money(std::string_view input);

/**
 * Get a descriptive name for a pile of coins based on total value
 * @param copper_value Total value in copper
 * @return Description like "pile", "heap", "mountain", etc.
 */
std::string_view get_money_pile_description(long copper_value);

} // namespace fiery

// =============================================================================
// fmt::formatter specialization for Money
// =============================================================================

template<>
struct fmt::formatter<fiery::Money> {
    bool use_color = false;
    bool use_brief = false;

    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end() && *it != '}') {
            if (*it == 'c') use_color = true;
            else if (*it == 'b') use_brief = true;
            ++it;
        }
        return it;
    }

    template<typename FormatContext>
    auto format(const fiery::Money& money, FormatContext& ctx) const {
        if (use_brief) {
            return fmt::format_to(ctx.out(), "{}", money.to_brief());
        }
        return fmt::format_to(ctx.out(), "{}", money.to_string(use_color));
    }
};

template<>
struct fmt::formatter<fiery::CoinType> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(fiery::CoinType type, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", fiery::get_coin_def(type).name);
    }
};
