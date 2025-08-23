/***************************************************************************
 *   File: src/core/ids.hpp                               Part of FieryMUD *
 *  Usage: Unified EntityId system replacing legacy ID types               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

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
 * Replaces:
 * - typedef int room_num     - Real room numbers (array indices)
 * - typedef int obj_num      - Object prototype numbers  
 * - typedef int zone_vnum    - Zone virtual numbers
 * - long id                  - DG trigger global IDs
 * - Various VNUM/RNUM macros
 *
 * Benefits:
 * - Type safety: Compiler prevents mixing different entity types
 * - Clarity: Single concept instead of vnum/rnum/id confusion
 * - Performance: Direct hash table lookups replace array indexing
 * - Scalability: 64-bit IDs support massive worlds without wraparound
 */
class EntityId {
public:
    /** Default constructor creates invalid ID */
    constexpr EntityId() : value_(0) {}
    
    /** Explicit construction from uint64_t */
    constexpr explicit EntityId(std::uint64_t id) : value_(id) {}
    
    /** Get the raw ID value */
    constexpr std::uint64_t value() const { return value_; }
    
    /** Check if ID is valid (non-zero) */
    constexpr bool is_valid() const { return value_ != 0; }
    
    /** Comparison operators for std::unordered_map compatibility */
    constexpr bool operator==(const EntityId& other) const = default;
    constexpr auto operator<=>(const EntityId& other) const = default;
    
    /** Hash support for unordered containers */
    struct Hash {
        std::size_t operator()(const EntityId& id) const noexcept {
            return std::hash<std::uint64_t>{}(id.value_);
        }
    };

private:
    std::uint64_t value_;
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