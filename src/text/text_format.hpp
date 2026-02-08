// Template variable substitution and color code parsing for all MUD messages

#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>

// Silence spurious warnings in <functional> header
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <functional>
#pragma GCC diagnostic pop

// Forward declarations
class Actor;
class Object;

/**
 * Generic text formatting system for FieryMUD.
 *
 * Provides template variable substitution and color code parsing for all
 * message types: socials, combat, spells, world effects, system messages, etc.
 *
 * Template Variables:
 *   {actor.name}              - Actor's display name
 *   {actor.pronoun.subjective} - he/she/they
 *   {actor.pronoun.objective}  - him/her/them
 *   {actor.pronoun.possessive} - his/her/their
 *   {actor.pronoun.reflexive}  - himself/herself/themselves
 *   {target.*}                 - Same for target
 *   {object.name}              - Object's name
 *   {damage}, {amount}, etc.   - Custom numeric/string variables
 *
 * Color Tags (XML-like):
 *   <red>text</red>           - Standard colors
 *   <b:red>text</b:red>       - Bright colors
 *   <#FF0000>text</#FF0000>   - Hex colors
 *   <b>text</b>               - Bold
 *   <u>text</u>               - Underline
 *   <i>text</i>               - Italic
 */
namespace TextFormat {

// =============================================================================
// Gender and Pronouns
// =============================================================================

enum class Gender {
    Male,
    Female,
    Neutral
};

struct Pronouns {
    std::string_view subjective;   // he/she/they
    std::string_view objective;    // him/her/them
    std::string_view possessive;   // his/her/their
    std::string_view reflexive;    // himself/herself/themselves
    std::string_view verb_be;      // is/is/are (for subject-verb agreement)
};

/**
 * Get the gender of an actor.
 * Maps actor.gender() string ("Male", "Female", "Neuter") to Gender enum.
 */
Gender get_actor_gender(const Actor& actor);

/**
 * Get pronouns for a gender.
 */
const Pronouns& get_pronouns(Gender gender);

// =============================================================================
// Variable Resolution
// =============================================================================

/**
 * Function type for resolving custom variables.
 * Returns std::nullopt if the variable is not recognized.
 */
using VariableResolver = std::function<std::optional<std::string>(std::string_view var_name)>;

/**
 * Substitute template variables in a message.
 *
 * @param template_msg The message with {variable} placeholders
 * @param resolver Function to resolve variable names to values
 * @return Message with variables substituted
 */
std::string substitute(std::string_view template_msg, VariableResolver resolver);

// =============================================================================
// Color Processing
// =============================================================================

/**
 * Convert color tags to ANSI escape codes.
 *
 * Supported tags:
 *   <black>, <red>, <green>, <yellow>, <blue>, <magenta>, <cyan>, <white>
 *   <b:black>, <b:red>, etc. (bright variants)
 *   <gray> (alias for bright black)
 *   <#RRGGBB> (24-bit hex colors)
 *   <b>, <u>, <i>, <s> (bold, underline, italic, strikethrough)
 */
std::string apply_colors(std::string_view message);

/**
 * Strip all color tags from a message.
 */
std::string strip_colors(std::string_view message);

/**
 * Check if message contains color tags.
 */
bool has_colors(std::string_view message);

// =============================================================================
// Text Utilities
// =============================================================================

/**
 * Capitalize the first visible letter of a string.
 * Handles ANSI codes at the start of the string.
 */
std::string capitalize(std::string_view str);

/**
 * Check if message contains template variables.
 */
bool has_variables(std::string_view message);

// =============================================================================
// Message Context Builder
// =============================================================================

/**
 * Fluent builder for formatting messages with multiple entities and variables.
 *
 * Example usage:
 *   auto msg = TextFormat::Message()
 *       .actor(attacker)
 *       .target(victim)
 *       .set("damage", 42)
 *       .set("weapon", "sword")
 *       .format("You hit {target.name} for <red>{damage}</red> damage!");
 */
class Message {
public:
    Message() = default;

    // Entity setters
    Message& actor(const Actor* a);
    Message& target(const Actor* t);
    Message& object(const Object* o);

    // Custom variable setters
    Message& set(std::string_view name, std::string_view value);
    Message& set(std::string_view name, std::string value);
    Message& set(std::string_view name, int value);
    Message& set(std::string_view name, double value);

    // Format the message
    std::string format(std::string_view template_msg) const;

    // Format without color processing (for plain text)
    std::string format_plain(std::string_view template_msg) const;

private:
    std::optional<std::string> resolve(std::string_view var_name) const;
    std::optional<std::string> resolve_actor(std::string_view property) const;
    std::optional<std::string> resolve_target(std::string_view property) const;
    std::optional<std::string> resolve_object(std::string_view property) const;
    std::optional<std::string> resolve_entity(const Actor* entity, std::string_view property) const;

    const Actor* actor_ = nullptr;
    const Actor* target_ = nullptr;
    const Object* object_ = nullptr;
    std::unordered_map<std::string, std::string> custom_vars_;
};

// =============================================================================
// Convenience Functions
// =============================================================================

/**
 * Format a message with actor and optional target.
 * Shorthand for Message().actor(a).target(t).format(msg)
 */
std::string format(std::string_view template_msg,
                   const Actor* actor,
                   const Actor* target = nullptr);

/**
 * Format a message with just color processing (no variables).
 */
std::string format_colors(std::string_view message);

} // namespace TextFormat
