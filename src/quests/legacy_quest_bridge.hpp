#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "core/ids.hpp"

namespace FieryMUD {

/**
 * Maps string-based quest names (from legacy DG-to-Lua triggers) to real Quest
 * records in zone 9998. Auto-creates Quest rows on first use so legacy triggers
 * integrate with the modern quest system.
 */
class LegacyQuestBridge {
  public:
    static LegacyQuestBridge &instance();

    /// Load all zone 9998 quests into cache on startup.
    void initialize();

    /// Find or auto-create a Quest record for the given name.
    /// Returns EntityId(9998, N) or invalid ID on DB error.
    EntityId get_or_create(const std::string &name);

    /// Lookup only (no auto-create). Returns invalid ID if not found.
    EntityId find(const std::string &name) const;

    static constexpr int LEGACY_ZONE_ID = 9998;

  private:
    LegacyQuestBridge() = default;

    /// Ensure zone 9998 exists in the Zones table.
    bool ensure_zone_exists();

    /// Insert a new Quest record in zone 9998 and return its EntityId.
    EntityId create_quest(const std::string &name);

    std::unordered_map<std::string, EntityId> name_cache_;
    int next_local_id_ = 0;
    mutable std::mutex mutex_;
};

} // namespace FieryMUD
