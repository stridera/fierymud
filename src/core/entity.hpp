#pragma once

#include <memory>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>

#include "core/ids.hpp"
#include "core/result.hpp"

/** Utility functions for entity management */
namespace EntityUtils {
/** Check if keyword is valid (not empty, reasonable length, printable chars) */
bool is_valid_keyword(std::string_view keyword);

/** Normalize keyword for consistent matching (lowercase, trim) */
std::string normalize_keyword(std::string_view keyword);

/** Extract keywords from a name list string (legacy compatibility) */
std::vector<std::string> parse_keyword_list(std::string_view keyword_string);

/** Create keyword list string from vector (legacy compatibility) */
std::string create_keyword_string(std::span<const std::string> keywords);

/** Check if name needs an article and which one ("a", "an", "the") */
std::string_view get_article(std::string_view name, bool definite = false);

/** Format entity reference for display (e.g., "a rusty sword", "the ancient tome") */
std::string format_entity_name(std::string_view name, std::string_view short_desc = "", bool with_article = false,
                               bool definite_article = false);

/** Check if a keyword set matches a targeting string, supporting compound keywords.
 *  Compound keywords use hyphens as AND operators: "magician-sleeves" matches
 *  if the entity has keywords matching BOTH "magician" AND "sleeves" (prefix match).
 */
bool matches_target_string(const std::set<std::string> &keywords, std::string_view target);

std::set<std::string> parse_target_string(std::string_view target);
} // namespace EntityUtils

/**
 * Base Entity class for all game objects.
 *
 * Provides common functionality for:
 * - Rooms, Objects, NPCs, Players
 * - Unique identification via EntityId
 * - Name/keyword/description management
 * - JSON serialization for persistence
 * - Type-safe entity relationships
 */
class Entity {
  public:
    /** Virtual destructor for proper inheritance */
    virtual ~Entity() = default;

    /** Get unique entity ID */
    EntityId id() const { return id_; }

    /** Get primary name */
    std::string_view name() const { return name_; }

    /** Get all keywords for matching */
    std::span<const std::string> keywords() const { return keywords_; }

    /** Get ground description text (shown when object is on ground) */
    std::string_view ground() const { return ground_; }

    /** Get short description (for lists, inventory, etc.) */
    std::string_view short_desc() const {
        if (short_.empty()) {
            return name_;
        }
        return short_;
    }

    /** Legacy compatibility - get ground description */
    std::string_view description() const { return ground_; }

    /** Legacy compatibility - get short description */
    std::string_view short_description() const { return short_desc(); }

    /** Check if entity matches a targeting string, supporting compound keywords.
     *  Compound keywords use hyphens as AND operators: "magician-sleeves" matches
     *  if the entity has keywords matching BOTH "magician" AND "sleeves" (prefix match).
     */
    inline bool matches_target_string(std::string_view target) const {
        return EntityUtils::matches_target_string(keyword_set_, target);
    }

    /** Check if entity matches all of the given keywords.
     *  Prefix matches are accepted.
     *  Precondition: Keywords must already be normalized.
     */
    template <typename C>
        requires std::ranges::forward_range<C> && (std::same_as<typename C::value_type, std::string> ||
                                                   std::same_as<typename C::value_type, const std::string>)
    bool matches_all_keywords(const C &keywords) const {
        for (const std::string &word : keywords) {
            auto match = keyword_set_.lower_bound(word);
            if (match == keyword_set_.end()) {
                return false;
            }
            if (!match->starts_with(word)) {
                return false;
            }
        }
        return true;
    }

    /** Set primary name */
    void set_name(std::string_view new_name) {
        name_ = new_name;
        ensure_name_in_keywords();
    }

    /** Set keywords (automatically includes name if not present) */
    void set_keywords(std::span<const std::string> new_keywords);

    /** Add a single keyword */
    void add_keyword(std::string_view keyword);

    /** Remove a keyword (cannot remove primary name) */
    void remove_keyword(std::string_view keyword);

    /** Set ground description */
    void set_ground(std::string_view new_ground) { ground_ = new_ground; }

    /** Set short description */
    void set_short_desc(std::string_view new_short) { short_ = new_short; }

    /** Legacy compatibility - set ground description */
    void set_description(std::string_view new_desc) { ground_ = new_desc; }

    /** Legacy compatibility - set short description */
    void set_short_description(std::string_view new_short_desc) { short_ = new_short_desc; }

    /** JSON serialization */
    virtual nlohmann::json to_json() const;

    /** JSON deserialization */
    static Result<std::unique_ptr<Entity>> from_json(const nlohmann::json &json);

    /** Get entity type name for serialization/debugging */
    virtual std::string_view type_name() const { return "Entity"; }

    /** Create formatted display name with article (a, an, the) */
    virtual std::string display_name(bool with_article = false) const;

    /**
     * Get dynamic room presence description.
     * Returns what you see when looking at this entity in a room.
     * Unlike ground() which is static, this reflects current state
     * (position, fighting status, etc.)
     *
     * @param viewer The actor viewing this entity (for "fighting YOU!" etc.)
     * @return Description like "Strider is standing here." or the static ground() for NPCs
     */
    virtual std::string room_presence(std::shared_ptr<class Actor> viewer = nullptr) const;

  protected:
    friend class EntityFactory;
    /** Constructor for derived classes */
    explicit Entity(EntityId id, std::string_view name = "");

    /** Constructor for loading from JSON */
    Entity(EntityId id, std::string_view name, std::span<const std::string> keywords, std::string_view ground,
           std::string_view short_desc = "");

    /** Ensure name is always in keywords list */
    void ensure_name_in_keywords();

    /** Validate entity state */
    virtual Result<void> validate() const;

    /** Adds keywords that apply to this instance of the entity.
     * Temporary keywords are not serialized.
     * Setting temporary keywords will remove any temporary keywords that
     * had been previously set on the entity.
     * @param temp_keywords The temporary keywords to add
     */
    void set_temporary_keywords(std::span<const std::string> temp_keywords);

  private:
    EntityId id_;
    std::string name_;
    std::vector<std::string> keywords_; // Ordered list for iteration/serialization
    std::set<std::string> keyword_set_; // O(log n) lookup cache
    std::string ground_;
    std::string short_;
};

/** Entity factory for creating entities from JSON */
class EntityFactory {
  public:
    using CreateFunction = std::function<Result<std::unique_ptr<Entity>>(const nlohmann::json &)>;

    /** Register entity type creator */
    static void register_type(std::string_view type_name, CreateFunction creator);

    /** Create entity from JSON based on type field */
    static Result<std::unique_ptr<Entity>> create_from_json(const nlohmann::json &json);

    /** Get all registered type names */
    static std::vector<std::string> get_registered_types();

  private:
    static std::unordered_map<std::string, CreateFunction> creators_;

    /** Create base Entity from JSON (no type dispatch) */
    static Result<std::unique_ptr<Entity>> create_base_entity_from_json(const nlohmann::json &json);
};

/** JSON conversion support */
void to_json(nlohmann::json &json, const Entity &entity);
void from_json(const nlohmann::json &json, std::unique_ptr<Entity> &entity);

/** Formatting support for Entity */
template <> struct fmt::formatter<Entity> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext> auto format(const Entity &entity, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{}({}): {}", entity.type_name(), entity.id(), entity.name());
    }
};

/** Stream output support for Entity */
#include <ostream>
inline std::ostream &operator<<(std::ostream &os, const Entity &entity) { return os << fmt::format("{}", entity); }
