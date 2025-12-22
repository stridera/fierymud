#pragma once

#include "../core/result.hpp"
#include <pqxx/pqxx>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <unordered_map>

/**
 * SocialPosition enum matching the database Position enum.
 * Represents the minimum position a target must be in to receive a social.
 * Named SocialPosition to avoid conflict with the core Position enum.
 */
enum class SocialPosition {
    Dead = 0,
    MortallyWounded = 1,
    Incapacitated = 2,
    Stunned = 3,
    Sleeping = 4,
    Resting = 5,
    Sitting = 6,
    Fighting = 7,
    Standing = 8,
    Flying = 9
};

/**
 * Social data structure matching the database Social table.
 *
 * Messages use the new template syntax:
 *   {actor.name} - Actor's display name
 *   {target.name} - Target's display name
 *   {actor.pronoun.subjective} - he/she/they
 *   {actor.pronoun.objective} - him/her/them
 *   {actor.pronoun.possessive} - his/her/their
 *   {actor.pronoun.reflexive} - himself/herself/themselves
 *   {target.pronoun.*} - Same for target
 *
 * Color codes use XML-like tags:
 *   <red>text</red> - Standard colors
 *   <b:red>text</b:red> - Bright colors
 *   <#FF0000>text</#FF0000> - Hex colors
 */
struct Social {
    int id;
    std::string name;                      // Command name (lowercase)
    bool hide;                             // Hide who initiated the action
    SocialPosition min_victim_position;    // Minimum position for target

    // No argument provided - actor alone
    std::optional<std::string> char_no_arg;    // Message to actor
    std::optional<std::string> others_no_arg;  // Message to room

    // Target found
    std::optional<std::string> char_found;     // Message to actor
    std::optional<std::string> others_found;   // Message to room (excluding target)
    std::optional<std::string> vict_found;     // Message to target

    // Target not found
    std::optional<std::string> not_found;      // "Can't find that person"

    // Self-targeting (actor targets themselves)
    std::optional<std::string> char_auto;      // Message to actor
    std::optional<std::string> others_auto;    // Message to room

    /**
     * Check if this social supports targeting.
     * A social supports targeting if it has char_found defined.
     */
    bool supports_target() const {
        return char_found.has_value();
    }

    /**
     * Check if this social supports self-targeting.
     */
    bool supports_self_target() const {
        return char_auto.has_value();
    }
};

/**
 * Social query layer for PostgreSQL database.
 *
 * Provides SQL queries for loading social commands from the
 * Social table. Socials define emote/action commands with
 * message templates that support variable substitution.
 *
 * All queries use prepared statements for security and performance.
 */
namespace SocialQueries {

/**
 * Load all socials from the database.
 *
 * @param txn Database transaction
 * @return Result containing vector of Social objects or error
 */
Result<std::vector<Social>> load_all_socials(pqxx::work& txn);

/**
 * Load all socials using a read-only connection.
 *
 * @param txn Non-transaction connection
 * @return Result containing vector of Social objects or error
 */
Result<std::vector<Social>> load_all_socials(pqxx::nontransaction& txn);

/**
 * Load a single social by name (case-insensitive).
 *
 * @param txn Database transaction
 * @param name Social command name
 * @return Result containing optional Social or error
 */
Result<std::optional<Social>> load_social_by_name(
    pqxx::work& txn,
    std::string_view name);

/**
 * Load a single social by ID.
 *
 * @param txn Database transaction
 * @param id Social ID
 * @return Result containing optional Social or error
 */
Result<std::optional<Social>> load_social_by_id(
    pqxx::work& txn,
    int id);

/**
 * Get count of all socials in database.
 *
 * @param txn Database transaction
 * @return Number of socials
 */
int get_social_count(pqxx::work& txn);

/**
 * Search socials by name pattern.
 *
 * @param txn Database transaction
 * @param pattern Search pattern (supports % wildcards)
 * @return Result containing matching socials
 */
Result<std::vector<Social>> search_socials(
    pqxx::work& txn,
    std::string_view pattern);

/**
 * Parse a SocialPosition enum from database string.
 *
 * @param str Position string from database (e.g., "STANDING", "SITTING")
 * @return Parsed SocialPosition enum value
 */
SocialPosition parse_position(std::string_view str);

/**
 * Convert SocialPosition enum to string.
 *
 * @param pos SocialPosition enum value
 * @return String representation
 */
std::string_view position_to_string(SocialPosition pos);

} // namespace SocialQueries

/**
 * Social cache for runtime access.
 *
 * Stores loaded socials in a map for O(1) lookup by name.
 * This is populated at server startup and refreshed as needed.
 */
class SocialCache {
public:
    /**
     * Get the singleton instance.
     */
    static SocialCache& instance();

    /**
     * Load or reload all socials from database.
     *
     * @return Result indicating success or failure
     */
    Result<void> load_from_database();

    /**
     * Get a social by name (case-insensitive).
     *
     * @param name Social command name
     * @return Pointer to social or nullptr if not found
     */
    const Social* get(std::string_view name) const;

    /**
     * Get all loaded socials.
     *
     * @return Reference to the social map
     */
    const std::unordered_map<std::string, Social>& all() const {
        return socials_;
    }

    /**
     * Get number of loaded socials.
     */
    std::size_t count() const {
        return socials_.size();
    }

    /**
     * Check if socials have been loaded.
     */
    bool is_loaded() const {
        return loaded_;
    }

    /**
     * Find socials matching a prefix (for command abbreviation).
     *
     * @param prefix Prefix to search for
     * @return Vector of matching social names
     */
    std::vector<std::string_view> find_by_prefix(std::string_view prefix) const;

private:
    SocialCache() = default;
    ~SocialCache() = default;
    SocialCache(const SocialCache&) = delete;
    SocialCache& operator=(const SocialCache&) = delete;

    std::unordered_map<std::string, Social> socials_;
    bool loaded_ = false;
};
