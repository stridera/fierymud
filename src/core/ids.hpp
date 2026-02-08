// Unified EntityId system with zone-scoped identifiers

#pragma once

#include <cstdint>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <vector>

// Silence spurious warnings in <functional> header
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <functional>
#pragma GCC diagnostic pop

/**
 * Unified EntityId system to replace legacy ID types.
 *
 * Uses composite key (zone_id, local_id) where:
 * - zone_id: The zone containing this entity
 * - local_id: The entity's ID within its zone (unrestricted range)
 *
 * Benefits:
 * - Type safety: Compiler prevents mixing different entity types
 * - Clarity: Single concept replaces legacy id confusion
 * - Performance: Direct hash table lookups replace array indexing
 * - Scalability: Composite keys align with database schema
 * - Database Integration: Native support for PostgreSQL composite primary keys
 */
class EntityId {
public:
    /** Default constructor creates invalid ID */
    constexpr EntityId() : zone_id_(0), local_id_(0), valid_(false) {}

    /** Composite key constructor for database integration - always valid */
    constexpr explicit EntityId(std::uint32_t zone_id, std::uint32_t local_id)
        : zone_id_(zone_id), local_id_(local_id), valid_(true) {}

    /** Legacy constructor for backward compatibility with JSON files - always valid */
    constexpr explicit EntityId(std::uint64_t legacy_id)
        : zone_id_(static_cast<std::uint32_t>(legacy_id / 100)),
          local_id_(static_cast<std::uint32_t>(legacy_id % 100)),
          valid_(true) {}

    /**
     * Parse EntityId from string in format "zone:id" or just "id".
     * If only a single number is provided and default_zone is specified, uses that zone.
     * @param str String to parse (e.g., "30:45" or "45")
     * @param default_zone Optional zone to use if only local_id is provided
     * @return Parsed EntityId, or invalid EntityId if parsing fails
     */
    static EntityId parse(std::string_view str, std::optional<std::uint32_t> default_zone = std::nullopt);

    /** Get zone ID component (for database composite keys) */
    constexpr std::uint32_t zone_id() const { return zone_id_; }

    /** Get local ID component (for database composite keys) */
    constexpr std::uint32_t local_id() const { return local_id_; }

    /** Get legacy combined value for backward compatibility */
    constexpr std::uint64_t value() const {
        return static_cast<std::uint64_t>(zone_id_) * 100 + local_id_;
    }

    /** Check if ID is valid */
    constexpr bool is_valid() const { return valid_; }

    /** Convert to string representation "zone:local" */
    std::string to_string() const;

    /** Comparison operators for std::unordered_map compatibility */
    constexpr bool operator==(const EntityId& other) const {
        // Two invalid IDs are considered equal
        if (!valid_ && !other.valid_) return true;
        // An invalid ID is never equal to a valid ID
        if (valid_ != other.valid_) return false;
        // Both valid - compare components
        return zone_id_ == other.zone_id_ && local_id_ == other.local_id_;
    }

    constexpr auto operator<=>(const EntityId& other) const {
        // Invalid IDs compare less than valid IDs
        if (!valid_ && other.valid_) return std::strong_ordering::less;
        if (valid_ && !other.valid_) return std::strong_ordering::greater;
        if (!valid_ && !other.valid_) return std::strong_ordering::equal;
        // Both valid - compare by zone first, then local_id
        if (auto cmp = zone_id_ <=> other.zone_id_; cmp != 0) return cmp;
        return local_id_ <=> other.local_id_;
    }

    /** Hash support for unordered containers */
    struct Hash {
        std::size_t operator()(const EntityId& id) const noexcept;
    };

private:
    std::uint32_t zone_id_;
    std::uint32_t local_id_;
    bool valid_;
};

/** Invalid entity ID constant */
constexpr EntityId INVALID_ENTITY_ID{};

/**
 * Constants for packed EntityId encoding/decoding.
 * Used for efficiently storing zone_id and local_id in a single uint32_t.
 * Format: (zone_id << PACKED_ID_ZONE_SHIFT) | (local_id & PACKED_ID_LOCAL_MASK)
 */
constexpr int PACKED_ID_ZONE_SHIFT = 16;
constexpr uint32_t PACKED_ID_LOCAL_MASK = 0xFFFF;

/** Helper to pack zone_id and local_id into a single uint32_t */
constexpr uint32_t pack_entity_id(uint32_t zone_id, uint32_t local_id) {
    return (zone_id << PACKED_ID_ZONE_SHIFT) | (local_id & PACKED_ID_LOCAL_MASK);
}

/** Helper to extract zone_id from a packed uint32_t */
constexpr uint32_t unpack_zone_id(uint32_t packed) {
    return packed >> PACKED_ID_ZONE_SHIFT;
}

/** Helper to extract local_id from a packed uint32_t */
constexpr uint32_t unpack_local_id(uint32_t packed) {
    return packed & PACKED_ID_LOCAL_MASK;
}

/** Create an EntityId from a packed uint32_t */
inline EntityId unpack_entity_id(uint32_t packed) {
    return EntityId(unpack_zone_id(packed), unpack_local_id(packed));
}

/** Type-safe entity registry template for managing entities by ID */
template<typename T>
class EntityRegistry {
public:
    using EntityPtr = std::shared_ptr<T>;
    using EntityWeakPtr = std::weak_ptr<T>;

    /** Find entity by ID, returns nullptr if not found */
    std::shared_ptr<T> find(EntityId id) const {
        auto it = entities_.find(id);
        return it != entities_.end() ? it->second.lock() : nullptr;
    }

    /** Get entity by ID, returns std::nullopt if not found */
    std::optional<std::shared_ptr<T>> get(EntityId id) const {
        if (auto ptr = find(id)) {
            return ptr;
        }
        return std::nullopt;
    }

    /** Register new entity, returns assigned ID */
    EntityId register_entity(std::shared_ptr<T> entity) {
        EntityId id = generate_id();
        entities_[id] = entity;
        return id;
    }

    /** Register entity with specific ID (for loading from disk) */
    void register_entity(EntityId id, std::shared_ptr<T> entity) {
        entities_[id] = entity;
        if (id.value() >= next_id_) {
            next_id_ = id.value() + 1;
        }
    }

    /** Unregister entity by ID */
    bool unregister_entity(EntityId id) {
        return entities_.erase(id) > 0;
    }

    /** Get all registered entity IDs */
    std::vector<EntityId> get_all_ids() const {
        std::vector<EntityId> ids;
        ids.reserve(entities_.size());

        for (const auto& [id, weak_ptr] : entities_) {
            if (!weak_ptr.expired()) {
                ids.push_back(id);
            }
        }
        return ids;
    }

    /** Get all registered entities */
    std::vector<std::shared_ptr<T>> get_all_entities() const {
        std::vector<std::shared_ptr<T>> entities;
        entities.reserve(entities_.size());

        for (const auto& [id, weak_ptr] : entities_) {
            if (auto ptr = weak_ptr.lock()) {
                entities.push_back(ptr);
            }
        }
        return entities;
    }

    /** Clean up expired weak pointers */
    void cleanup_expired() {
        for (auto it = entities_.begin(); it != entities_.end();) {
            if (it->second.expired()) {
                it = entities_.erase(it);
            } else {
                ++it;
            }
        }
    }

    /** Get count of registered entities */
    std::size_t size() const {
        std::size_t count = 0;
        for (const auto& [id, weak_ptr] : entities_) {
            if (!weak_ptr.expired()) {
                ++count;
            }
        }
        return count;
    }

    /** Check if registry is empty */
    bool empty() const {
        return size() == 0;
    }

private:
    std::unordered_map<EntityId, std::weak_ptr<T>, EntityId::Hash> entities_;
    std::uint64_t next_id_ = 1;

    EntityId generate_id() {
        return EntityId{next_id_++};
    }
};

/** Hash specialization for EntityId */
template<>
struct std::hash<EntityId> {
    std::size_t operator()(const EntityId& id) const noexcept {
        return EntityId::Hash{}(id);
    }
};

/** Formatting support for EntityId */
#include <fmt/format.h>

template<>
struct fmt::formatter<EntityId> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const EntityId& id, FormatContext& ctx) const {
        // Format as composite key (zone:local) for clarity
        return fmt::format_to(ctx.out(), "{}:{}", id.zone_id(), id.local_id());
    }
};

/** Stream output support for EntityId */
inline std::ostream& operator<<(std::ostream& os, const EntityId& id) {
    return os << id.zone_id() << ':' << id.local_id();
}
