// Unified EntityId system with zone-scoped identifiers

#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <vector>

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
    constexpr EntityId() : zone_id_(0), local_id_(0) {}

    /** Composite key constructor for database integration */
    constexpr explicit EntityId(std::uint32_t zone_id, std::uint32_t local_id)
        : zone_id_(zone_id), local_id_(local_id) {}

    /** Legacy constructor for backward compatibility with JSON files */
    constexpr explicit EntityId(std::uint64_t legacy_id) {
        // Extract zone and local_id from legacy ID (e.g., 3045 → zone 30, local_id 45)
        zone_id_ = static_cast<std::uint32_t>(legacy_id / 100);
        local_id_ = static_cast<std::uint32_t>(legacy_id % 100);
    }

    /** Get zone ID component (for database composite keys) */
    constexpr std::uint32_t zone_id() const { return zone_id_; }

    /** Get local ID component (for database composite keys) */
    constexpr std::uint32_t local_id() const { return local_id_; }

    /** Get legacy combined value for backward compatibility */
    constexpr std::uint64_t value() const {
        return static_cast<std::uint64_t>(zone_id_) * 100 + local_id_;
    }

    /** Check if ID is valid (non-zero) */
    constexpr bool is_valid() const { return zone_id_ != 0 || local_id_ != 0; }

    /** Comparison operators for std::unordered_map compatibility */
    constexpr bool operator==(const EntityId& other) const = default;
    constexpr auto operator<=>(const EntityId& other) const = default;

    /** Hash support for unordered containers */
    struct Hash {
        std::size_t operator()(const EntityId& id) const noexcept {
            // Combine hash of both components for better distribution
            std::size_t h1 = std::hash<std::uint32_t>{}(id.zone_id_);
            std::size_t h2 = std::hash<std::uint32_t>{}(id.local_id_);
            return h1 ^ (h2 << 1);
        }
    };

private:
    std::uint32_t zone_id_;
    std::uint32_t local_id_;
};

/** Invalid entity ID constant */
constexpr EntityId INVALID_ENTITY_ID{};

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
        return fmt::format_to(ctx.out(), "{}", id.value());
    }
};
