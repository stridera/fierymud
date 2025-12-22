#include "text_format.hpp"
#include "core/actor.hpp"
#include "string_utils.hpp"
#include <fmt/format.h>
#include <cctype>

namespace TextFormat {

namespace {

// =============================================================================
// Static Data
// =============================================================================

constexpr Pronouns MALE_PRONOUNS = {"he", "him", "his", "himself"};
constexpr Pronouns FEMALE_PRONOUNS = {"she", "her", "her", "herself"};
constexpr Pronouns NEUTRAL_PRONOUNS = {"they", "them", "their", "themselves"};

const std::unordered_map<std::string, std::string> COLOR_CODES = {
    // Standard colors
    {"black", "\033[30m"},
    {"red", "\033[31m"},
    {"green", "\033[32m"},
    {"yellow", "\033[33m"},
    {"blue", "\033[34m"},
    {"magenta", "\033[35m"},
    {"cyan", "\033[36m"},
    {"white", "\033[37m"},
    {"gray", "\033[90m"},
    {"grey", "\033[90m"},

    // Bright colors
    {"b:black", "\033[90m"},
    {"b:red", "\033[91m"},
    {"b:green", "\033[92m"},
    {"b:yellow", "\033[93m"},
    {"b:blue", "\033[94m"},
    {"b:magenta", "\033[95m"},
    {"b:cyan", "\033[96m"},
    {"b:white", "\033[97m"},

    // Text styles
    {"b", "\033[1m"},
    {"bold", "\033[1m"},
    {"i", "\033[3m"},
    {"italic", "\033[3m"},
    {"u", "\033[4m"},
    {"underline", "\033[4m"},
    {"s", "\033[9m"},
    {"strikethrough", "\033[9m"},

    // Semantic colors
    {"damage", "\033[91m"},
    {"healing", "\033[92m"},
    {"info", "\033[94m"},
    {"warning", "\033[93m"},
    {"error", "\033[91m"},
    {"success", "\033[92m"},
    {"mana", "\033[96m"},
    {"stamina", "\033[93m"},
    {"experience", "\033[95m"},
};

constexpr std::string_view RESET_CODE = "\033[0m";

// =============================================================================
// Helper Functions
// =============================================================================

std::string hex_to_ansi(std::string_view hex) {
    if (!hex.empty() && hex[0] == '#') {
        hex = hex.substr(1);
    }

    if (hex.length() != 6) {
        return "";
    }

    try {
        unsigned int r = std::stoul(std::string{hex.substr(0, 2)}, nullptr, 16);
        unsigned int g = std::stoul(std::string{hex.substr(2, 2)}, nullptr, 16);
        unsigned int b = std::stoul(std::string{hex.substr(4, 2)}, nullptr, 16);
        return fmt::format("\033[38;2;{};{};{}m", r, g, b);
    } catch (...) {
        return "";
    }
}

std::string get_actor_name(const Actor* actor) {
    if (!actor) {
        return "someone";
    }
    return actor->display_name();
}

} // anonymous namespace

// =============================================================================
// Gender and Pronouns
// =============================================================================

Gender get_actor_gender(const Actor& /* actor */) {
    // TODO: When gender is properly exposed in the Actor class, use it.
    return Gender::Neutral;
}

const Pronouns& get_pronouns(Gender gender) {
    switch (gender) {
        case Gender::Male:
            return MALE_PRONOUNS;
        case Gender::Female:
            return FEMALE_PRONOUNS;
        case Gender::Neutral:
        default:
            return NEUTRAL_PRONOUNS;
    }
}

// =============================================================================
// Variable Resolution
// =============================================================================

std::string substitute(std::string_view template_msg, VariableResolver resolver) {
    std::string result;
    result.reserve(template_msg.length() * 2);

    std::size_t i = 0;
    while (i < template_msg.length()) {
        if (template_msg[i] == '{') {
            std::size_t end = template_msg.find('}', i + 1);
            if (end != std::string_view::npos) {
                std::string_view var = template_msg.substr(i + 1, end - i - 1);
                auto resolved = resolver(var);
                if (resolved) {
                    result += *resolved;
                } else {
                    // Keep the original variable if not resolved
                    result += '{';
                    result += var;
                    result += '}';
                }
                i = end + 1;
                continue;
            }
        }
        result += template_msg[i];
        ++i;
    }

    return result;
}

// =============================================================================
// Color Processing
// =============================================================================

std::string apply_colors(std::string_view message) {
    std::string result;
    result.reserve(message.length() * 2);

    std::size_t i = 0;
    while (i < message.length()) {
        if (message[i] == '<') {
            // Check for closing tag
            if (i + 1 < message.length() && message[i + 1] == '/') {
                std::size_t end = message.find('>', i + 2);
                if (end != std::string_view::npos) {
                    result += RESET_CODE;
                    i = end + 1;
                    continue;
                }
            }

            // Opening tag
            std::size_t end = message.find('>', i + 1);
            if (end != std::string_view::npos) {
                std::string_view tag_content = message.substr(i + 1, end - i - 1);

                // Hex color
                if (!tag_content.empty() && tag_content[0] == '#') {
                    std::string ansi = hex_to_ansi(tag_content);
                    if (!ansi.empty()) {
                        result += ansi;
                        i = end + 1;
                        continue;
                    }
                }

                // Named color or style
                std::string tag_name = to_lower(tag_content);
                auto it = COLOR_CODES.find(tag_name);
                if (it != COLOR_CODES.end()) {
                    result += it->second;
                    i = end + 1;
                    continue;
                }
            }
        }

        result += message[i];
        ++i;
    }

    return result;
}

std::string strip_colors(std::string_view message) {
    std::string result;
    result.reserve(message.length());

    std::size_t i = 0;
    while (i < message.length()) {
        if (message[i] == '<') {
            std::size_t end = message.find('>', i + 1);
            if (end != std::string_view::npos) {
                i = end + 1;
                continue;
            }
        }
        result += message[i];
        ++i;
    }

    return result;
}

bool has_colors(std::string_view message) {
    std::size_t pos = 0;
    while ((pos = message.find('<', pos)) != std::string_view::npos) {
        std::size_t end = message.find('>', pos + 1);
        if (end != std::string_view::npos) {
            std::string_view tag = message.substr(pos + 1, end - pos - 1);
            if (!tag.empty() && (tag[0] == '/' || tag[0] == '#' ||
                std::isalpha(static_cast<unsigned char>(tag[0])))) {
                return true;
            }
        }
        pos = end == std::string_view::npos ? std::string_view::npos : end + 1;
    }
    return false;
}

// =============================================================================
// Text Utilities
// =============================================================================

std::string capitalize(std::string_view str) {
    if (str.empty()) {
        return {};
    }

    std::string result{str};

    // Skip any leading ANSI escape sequences
    std::size_t i = 0;
    while (i < result.length()) {
        if (result[i] == '\033' && i + 1 < result.length() && result[i + 1] == '[') {
            i += 2;
            while (i < result.length() && result[i] != 'm') {
                ++i;
            }
            if (i < result.length()) {
                ++i;
            }
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(result[i]))) {
            result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
            break;
        }
        ++i;
    }

    return result;
}

bool has_variables(std::string_view message) {
    return message.find('{') != std::string_view::npos &&
           message.find('}') != std::string_view::npos;
}

// =============================================================================
// Message Context Builder
// =============================================================================

Message& Message::actor(const Actor* a) {
    actor_ = a;
    return *this;
}

Message& Message::target(const Actor* t) {
    target_ = t;
    return *this;
}

Message& Message::object(const Object* o) {
    object_ = o;
    return *this;
}

Message& Message::set(std::string_view name, std::string_view value) {
    custom_vars_[std::string{name}] = std::string{value};
    return *this;
}

Message& Message::set(std::string_view name, std::string value) {
    custom_vars_[std::string{name}] = std::move(value);
    return *this;
}

Message& Message::set(std::string_view name, int value) {
    custom_vars_[std::string{name}] = std::to_string(value);
    return *this;
}

Message& Message::set(std::string_view name, double value) {
    custom_vars_[std::string{name}] = fmt::format("{:.1f}", value);
    return *this;
}

std::string Message::format(std::string_view template_msg) const {
    // Substitute variables
    std::string result = substitute(template_msg, [this](std::string_view var) {
        return resolve(var);
    });

    // Apply colors
    if (has_colors(result)) {
        result = apply_colors(result);
    }

    // Capitalize
    result = capitalize(result);

    return result;
}

std::string Message::format_plain(std::string_view template_msg) const {
    std::string result = substitute(template_msg, [this](std::string_view var) {
        return resolve(var);
    });
    return capitalize(result);
}

std::optional<std::string> Message::resolve(std::string_view var_name) const {
    // Check custom variables first (exact match)
    auto it = custom_vars_.find(std::string{var_name});
    if (it != custom_vars_.end()) {
        return it->second;
    }

    // Split by periods
    std::vector<std::string> parts;
    std::string current;
    for (char c : var_name) {
        if (c == '.') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }

    if (parts.empty()) {
        return std::nullopt;
    }

    std::string entity_type = to_lower(parts[0]);
    std::string property = parts.size() > 1 ? to_lower(parts[1]) : "name";

    // Route to appropriate entity resolver
    if (entity_type == "actor" || entity_type == "char" || entity_type == "self" ||
        entity_type == "attacker" || entity_type == "caster") {
        return resolve_actor(parts.size() > 1 ? std::string_view{parts[1]} : "name");
    }

    if (entity_type == "target" || entity_type == "vict" || entity_type == "victim" ||
        entity_type == "defender") {
        return resolve_target(parts.size() > 1 ? std::string_view{parts[1]} : "name");
    }

    if (entity_type == "object" || entity_type == "obj" || entity_type == "item" ||
        entity_type == "weapon") {
        return resolve_object(parts.size() > 1 ? std::string_view{parts[1]} : "name");
    }

    return std::nullopt;
}

std::optional<std::string> Message::resolve_actor(std::string_view property) const {
    return resolve_entity(actor_, property);
}

std::optional<std::string> Message::resolve_target(std::string_view property) const {
    return resolve_entity(target_, property);
}

std::optional<std::string> Message::resolve_object(std::string_view property) const {
    if (!object_) {
        return "something";
    }

    std::string prop = to_lower(property);
    if (prop == "name") {
        // TODO: When Object class is available, use object_->name()
        return "an object";
    }

    return std::nullopt;
}

std::optional<std::string> Message::resolve_entity(const Actor* entity, std::string_view property) const {
    std::string prop = to_lower(property);

    if (prop == "name") {
        return get_actor_name(entity);
    }

    if (!entity) {
        // For pronouns, use neutral if no entity
        Gender gender = Gender::Neutral;
        const Pronouns& pronouns = get_pronouns(gender);

        if (prop == "he" || prop == "she" || prop == "they" || prop == "subjective" || prop == "sub") {
            return std::string{pronouns.subjective};
        }
        if (prop == "him" || prop == "her" || prop == "them" || prop == "objective" || prop == "obj") {
            return std::string{pronouns.objective};
        }
        if (prop == "his" || prop == "hers" || prop == "their" || prop == "possessive" || prop == "pos") {
            return std::string{pronouns.possessive};
        }
        if (prop == "himself" || prop == "herself" || prop == "themselves" || prop == "reflexive" || prop == "ref") {
            return std::string{pronouns.reflexive};
        }
        return std::nullopt;
    }

    Gender gender = get_actor_gender(*entity);
    const Pronouns& pronouns = get_pronouns(gender);

    // Direct pronoun shortcuts
    if (prop == "he" || prop == "she" || prop == "they" || prop == "subjective" || prop == "sub") {
        return std::string{pronouns.subjective};
    }
    if (prop == "him" || prop == "her" || prop == "them" || prop == "objective" || prop == "obj") {
        return std::string{pronouns.objective};
    }
    if (prop == "his" || prop == "hers" || prop == "their" || prop == "possessive" || prop == "pos") {
        return std::string{pronouns.possessive};
    }
    if (prop == "himself" || prop == "herself" || prop == "themselves" || prop == "reflexive" || prop == "ref") {
        return std::string{pronouns.reflexive};
    }

    // Handle {actor.pronoun.type} pattern - property is "pronoun", need third part
    // This is handled at the resolve() level by passing the full property chain

    return std::nullopt;
}

// =============================================================================
// Convenience Functions
// =============================================================================

std::string format(std::string_view template_msg, const Actor* actor, const Actor* target) {
    return Message().actor(actor).target(target).format(template_msg);
}

std::string format_colors(std::string_view message) {
    std::string result = apply_colors(message);
    return capitalize(result);
}

} // namespace TextFormat
