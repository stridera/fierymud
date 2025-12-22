#pragma once

#include <expected>
#include <functional>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>

#include "core/result.hpp"

namespace FieryMUD {

/**
 * FormulaContext provides variable values for formula evaluation.
 * Variables like "skill", "level", "str_bonus" are resolved from this context.
 */
struct FormulaContext {
    int skill_level = 0;      // Current skill proficiency (0-100)
    int actor_level = 1;      // Actor's level
    int str_bonus = 0;        // Strength modifier
    int dex_bonus = 0;        // Dexterity modifier
    int con_bonus = 0;        // Constitution modifier
    int int_bonus = 0;        // Intelligence modifier
    int wis_bonus = 0;        // Wisdom modifier
    int cha_bonus = 0;        // Charisma modifier
    int weapon_damage = 0;    // Base weapon damage
    int armor_class = 0;      // Target's armor class
    int target_level = 1;     // Target's level

    // Custom variables can be added here
    std::unordered_map<std::string, int> custom_vars;

    // Get variable value by name
    std::expected<int, Error> get_variable(std::string_view name) const;
};

/**
 * FormulaParser evaluates damage/effect formulas from the database.
 *
 * Supported syntax:
 * - Numbers: 42, -5
 * - Dice: 4d6, 2d8+5, 1d20-2
 * - Variables: skill, level, str_bonus, dex_bonus, etc.
 * - Operators: +, -, *, /, ()
 * - Functions: pow(base, exp), min(a, b), max(a, b), dice(count, sides)
 *
 * Examples:
 * - "skill / 2"
 * - "skill + str_bonus"
 * - "4d6 + level"
 * - "dice(level, 8) + level"
 * - "pow(skill, 1.35)"
 */
class FormulaParser {
public:
    /**
     * Evaluate a formula string with the given context.
     * @param formula The formula expression to evaluate
     * @param context Variable values for the formula
     * @return The calculated integer result, or an error
     */
    static std::expected<int, Error> evaluate(std::string_view formula, const FormulaContext& context);

    /**
     * Roll dice in NdM format.
     * @param count Number of dice to roll
     * @param sides Number of sides per die
     * @return Sum of all dice rolls
     */
    static int roll_dice(int count, int sides);

private:
    struct Parser {
        std::string_view input;
        size_t pos = 0;
        const FormulaContext& context;

        Parser(std::string_view input, const FormulaContext& ctx)
            : input(input), context(ctx) {}

        std::expected<double, Error> parse();
        std::expected<double, Error> parse_expression();
        std::expected<double, Error> parse_term();
        std::expected<double, Error> parse_factor();
        std::expected<double, Error> parse_primary();
        std::expected<double, Error> parse_number();
        std::expected<double, Error> parse_identifier();
        std::expected<double, Error> parse_function(std::string_view name);
        std::expected<double, Error> parse_dice(int count);

        void skip_whitespace();
        char peek() const;
        char advance();
        bool match(char c);
        bool at_end() const;
    };

    static thread_local std::mt19937 rng_;
    static bool rng_initialized_;
};

} // namespace FieryMUD
