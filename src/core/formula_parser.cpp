#include "core/formula_parser.hpp"

#include <cctype>
#include <cmath>
#include <random>

#include <fmt/format.h>

namespace FieryMUD {

thread_local std::mt19937 FormulaParser::rng_;
bool FormulaParser::rng_initialized_ = false;

std::expected<int, Error> FormulaContext::get_variable(std::string_view name) const {
    // Check built-in actor variables
    if (name == "skill") return skill_level;
    if (name == "level") return actor_level;
    if (name == "str_bonus" || name == "str") return str_bonus;
    if (name == "dex_bonus" || name == "dex") return dex_bonus;
    if (name == "con_bonus" || name == "con") return con_bonus;
    if (name == "int_bonus" || name == "int") return int_bonus;
    if (name == "wis_bonus" || name == "wis") return wis_bonus;
    if (name == "cha_bonus" || name == "cha") return cha_bonus;
    if (name == "weapon_damage" || name == "weapon") return weapon_damage;
    if (name == "base_damage" || name == "base") return base_damage;

    // Actor's detection stats
    if (name == "perception") return perception;
    if (name == "concealment") return concealment;

    // Target stats for contested checks
    if (name == "armor_rating" || name == "ar") return armor_rating;
    if (name == "target_level") return target_level;
    if (name == "target_perception") return target_perception;
    if (name == "target_concealment") return target_concealment;

    // Check custom variables
    std::string name_str(name);
    auto it = custom_vars.find(name_str);
    if (it != custom_vars.end()) {
        return it->second;
    }

    return std::unexpected(Errors::NotFound(fmt::format("variable '{}'", name)));
}

int FormulaParser::roll_dice(int count, int sides) {
    if (!rng_initialized_) {
        std::random_device rd;
        rng_.seed(rd());
        rng_initialized_ = true;
    }

    if (count <= 0 || sides <= 0) return 0;

    std::uniform_int_distribution<int> dist(1, sides);
    int total = 0;
    for (int i = 0; i < count; ++i) {
        total += dist(rng_);
    }
    return total;
}

std::expected<int, Error> FormulaParser::evaluate(std::string_view formula, const FormulaContext& context) {
    if (formula.empty()) {
        return 0;
    }

    Parser parser(formula, context);
    auto result = parser.parse();
    if (!result) {
        return std::unexpected(result.error());
    }

    return static_cast<int>(std::round(*result));
}

void FormulaParser::Parser::skip_whitespace() {
    while (pos < input.size() && std::isspace(input[pos])) {
        ++pos;
    }
}

char FormulaParser::Parser::peek() const {
    if (pos >= input.size()) return '\0';
    return input[pos];
}

char FormulaParser::Parser::advance() {
    if (pos >= input.size()) return '\0';
    return input[pos++];
}

bool FormulaParser::Parser::match(char c) {
    skip_whitespace();
    if (peek() == c) {
        advance();
        return true;
    }
    return false;
}

bool FormulaParser::Parser::at_end() const {
    return pos >= input.size();
}

std::expected<double, Error> FormulaParser::Parser::parse() {
    auto result = parse_expression();
    if (!result) return result;

    skip_whitespace();
    if (!at_end()) {
        return std::unexpected(Errors::ParseError(
            fmt::format("Unexpected character '{}' at position {}", peek(), pos)));
    }

    return result;
}

std::expected<double, Error> FormulaParser::Parser::parse_expression() {
    auto left = parse_term();
    if (!left) return left;

    while (true) {
        skip_whitespace();
        if (match('+')) {
            auto right = parse_term();
            if (!right) return right;
            left = *left + *right;
        } else if (match('-')) {
            auto right = parse_term();
            if (!right) return right;
            left = *left - *right;
        } else {
            break;
        }
    }

    return left;
}

std::expected<double, Error> FormulaParser::Parser::parse_term() {
    auto left = parse_factor();
    if (!left) return left;

    while (true) {
        skip_whitespace();
        if (match('*')) {
            auto right = parse_factor();
            if (!right) return right;
            left = *left * *right;
        } else if (match('/')) {
            auto right = parse_factor();
            if (!right) return right;
            if (*right == 0) {
                return std::unexpected(Errors::InvalidArgument("divisor", "cannot be zero"));
            }
            left = *left / *right;
        } else {
            break;
        }
    }

    return left;
}

std::expected<double, Error> FormulaParser::Parser::parse_factor() {
    skip_whitespace();

    // Handle unary minus
    if (match('-')) {
        auto val = parse_factor();
        if (!val) return val;
        return -*val;
    }

    // Handle unary plus
    if (match('+')) {
        return parse_factor();
    }

    return parse_primary();
}

std::expected<double, Error> FormulaParser::Parser::parse_primary() {
    skip_whitespace();

    // Parenthesized expression
    if (match('(')) {
        auto result = parse_expression();
        if (!result) return result;
        skip_whitespace();
        if (!match(')')) {
            return std::unexpected(Errors::ParseError("Expected closing parenthesis"));
        }
        return result;
    }

    // Number (possibly followed by 'd' for dice)
    if (std::isdigit(peek())) {
        return parse_number();
    }

    // Identifier (variable or function)
    if (std::isalpha(peek()) || peek() == '_') {
        return parse_identifier();
    }

    return std::unexpected(Errors::ParseError(
        fmt::format("Unexpected character '{}' at position {}", peek(), pos)));
}

std::expected<double, Error> FormulaParser::Parser::parse_number() {
    size_t start = pos;
    bool has_decimal = false;

    while (std::isdigit(peek()) || (peek() == '.' && !has_decimal)) {
        if (peek() == '.') has_decimal = true;
        advance();
    }

    std::string num_str(input.substr(start, pos - start));
    double value = std::stod(num_str);

    // Check for dice notation (e.g., 4d6)
    skip_whitespace();
    if (peek() == 'd' || peek() == 'D') {
        return parse_dice(static_cast<int>(value));
    }

    return value;
}

std::expected<double, Error> FormulaParser::Parser::parse_dice(int count) {
    // Consume 'd'
    advance();
    skip_whitespace();

    if (!std::isdigit(peek())) {
        return std::unexpected(Errors::ParseError("Expected number of sides after 'd'"));
    }

    size_t start = pos;
    while (std::isdigit(peek())) {
        advance();
    }

    std::string sides_str(input.substr(start, pos - start));
    int sides = std::stoi(sides_str);

    return static_cast<double>(roll_dice(count, sides));
}

std::expected<double, Error> FormulaParser::Parser::parse_identifier() {
    size_t start = pos;

    while (std::isalnum(peek()) || peek() == '_') {
        advance();
    }

    std::string_view name = input.substr(start, pos - start);
    skip_whitespace();

    // Check if it's a function call
    if (peek() == '(') {
        return parse_function(name);
    }

    // It's a variable
    auto value = context.get_variable(name);
    if (!value) {
        return std::unexpected(value.error());
    }

    return static_cast<double>(*value);
}

std::expected<double, Error> FormulaParser::Parser::parse_function(std::string_view name) {
    // Consume '('
    advance();

    std::vector<double> args;

    // Parse arguments
    skip_whitespace();
    if (peek() != ')') {
        while (true) {
            auto arg = parse_expression();
            if (!arg) return arg;
            args.push_back(*arg);

            skip_whitespace();
            if (!match(',')) break;
        }
    }

    skip_whitespace();
    if (!match(')')) {
        return std::unexpected(Errors::ParseError("Expected closing parenthesis in function call"));
    }

    // Execute function
    if (name == "pow") {
        if (args.size() != 2) {
            return std::unexpected(Errors::InvalidArgument("pow", "requires 2 arguments"));
        }
        return std::pow(args[0], args[1]);
    }

    if (name == "min") {
        if (args.size() != 2) {
            return std::unexpected(Errors::InvalidArgument("min", "requires 2 arguments"));
        }
        return std::min(args[0], args[1]);
    }

    if (name == "max") {
        if (args.size() != 2) {
            return std::unexpected(Errors::InvalidArgument("max", "requires 2 arguments"));
        }
        return std::max(args[0], args[1]);
    }

    if (name == "dice" || name == "roll_dice") {
        if (args.size() != 2) {
            return std::unexpected(Errors::InvalidArgument(std::string(name), "requires 2 arguments"));
        }
        return static_cast<double>(roll_dice(static_cast<int>(args[0]), static_cast<int>(args[1])));
    }

    if (name == "abs") {
        if (args.size() != 1) {
            return std::unexpected(Errors::InvalidArgument("abs", "requires 1 argument"));
        }
        return std::abs(args[0]);
    }

    if (name == "floor") {
        if (args.size() != 1) {
            return std::unexpected(Errors::InvalidArgument("floor", "requires 1 argument"));
        }
        return std::floor(args[0]);
    }

    if (name == "ceil") {
        if (args.size() != 1) {
            return std::unexpected(Errors::InvalidArgument("ceil", "requires 1 argument"));
        }
        return std::ceil(args[0]);
    }

    if (name == "round") {
        if (args.size() != 1) {
            return std::unexpected(Errors::InvalidArgument("round", "requires 1 argument"));
        }
        return std::round(args[0]);
    }

    return std::unexpected(Errors::NotFound(fmt::format("function '{}'", name)));
}

} // namespace FieryMUD
