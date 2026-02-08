#include "quest_manager.hpp"

#include "core/actor.hpp"
#include "core/logging.hpp"
#include "core/object.hpp"
#include "database/connection_pool.hpp"
#include "database/quest_queries.hpp"
#include "world/room.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace FieryMUD {

// ============================================================================
// Helper Functions
// ============================================================================

namespace {
// Helper to get character_id from an Actor (returns empty if not a Player)
std::string_view get_character_id(const std::shared_ptr<Actor> &actor) {
    if (!actor)
        return {};
    auto player = std::dynamic_pointer_cast<Player>(actor);
    if (!player)
        return {};
    return player->database_id();
}
} // namespace

// ============================================================================
// Singleton
// ============================================================================

QuestManager &QuestManager::instance() {
    static QuestManager instance;
    return instance;
}

// ============================================================================
// Initialization
// ============================================================================

bool QuestManager::initialize() {
    if (initialized_) {
        spdlog::warn("QuestManager::initialize() called when already initialized");
        return true;
    }

    auto logger = Log::database();
    logger->info("QuestManager initializing...");

    // Clear any stale state
    clear_all_quests();
    reset_stats();

    initialized_ = true;
    logger->info("QuestManager initialized");
    return true;
}

void QuestManager::shutdown() {
    if (!initialized_) {
        return;
    }

    auto logger = Log::database();
    logger->info("QuestManager shutting down...");

    clear_all_quests();
    initialized_ = false;

    logger->info("QuestManager shutdown complete (started={}, completed={}, abandoned={})", stats_.quests_started,
                 stats_.quests_completed, stats_.quests_abandoned);
}

// ============================================================================
// Quest Loading
// ============================================================================

std::expected<std::size_t, std::string> QuestManager::load_zone_quests(int zone_id) {
    if (!initialized_) {
        return std::unexpected("QuestManager not initialized");
    }

    // Check if zone already loaded
    if (loaded_zones_.contains(zone_id)) {
        spdlog::debug("Zone {} quests already loaded", zone_id);
        return 0;
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        spdlog::debug("Database not initialized, skipping quest load for zone {}", zone_id);
        return 0;
    }

    auto result = pool.execute([zone_id](pqxx::work &txn) -> Result<std::vector<QuestQueries::QuestData>> {
        return QuestQueries::load_quests_in_zone(txn, zone_id);
    });

    if (!result) {
        last_error_ = fmt::format("Failed to load quests for zone {}: {}", zone_id, result.error().message);
        return std::unexpected(last_error_);
    }

    // Cache all loaded quests
    std::size_t count = 0;
    for (auto &quest : *result) {
        QuestKey key{static_cast<int>(quest.id.zone_id()), static_cast<int>(quest.id.local_id())};

        // Index triggers before caching
        index_quest_triggers(quest);

        // Store in cache
        quest_cache_[key] = std::move(quest);
        zone_quests_[zone_id].push_back(key);
        count++;
    }

    loaded_zones_.insert(zone_id);
    spdlog::info("Loaded {} quests for zone {}", count, zone_id);
    return count;
}

std::expected<std::size_t, std::string> QuestManager::reload_zone_quests(int zone_id) {
    clear_zone_quests(zone_id);
    return load_zone_quests(zone_id);
}

void QuestManager::clear_zone_quests(int zone_id) {
    // Unindex triggers for quests in this zone
    auto it = zone_quests_.find(zone_id);
    if (it != zone_quests_.end()) {
        for (const auto &key : it->second) {
            auto quest_it = quest_cache_.find(key);
            if (quest_it != quest_cache_.end()) {
                unindex_quest_triggers(quest_it->second);
                quest_cache_.erase(quest_it);
            }
        }
        zone_quests_.erase(it);
    }

    loaded_zones_.erase(zone_id);
    spdlog::debug("Cleared quests for zone {}", zone_id);
}

void QuestManager::clear_all_quests() {
    quest_cache_.clear();
    zone_quests_.clear();
    level_triggered_quests_.clear();
    item_triggered_quests_.clear();
    room_triggered_quests_.clear();
    skill_triggered_quests_.clear();
    event_triggered_quests_.clear();
    auto_triggered_quests_.clear();
    loaded_zones_.clear();
    spdlog::debug("Cleared all quests");
}

// ============================================================================
// Quest Lookup
// ============================================================================

const QuestQueries::QuestData *QuestManager::get_quest(int zone_id, int quest_id) const {
    QuestKey key{zone_id, quest_id};
    auto it = quest_cache_.find(key);
    if (it != quest_cache_.end()) {
        return &it->second;
    }
    return nullptr;
}

const QuestQueries::QuestData *QuestManager::get_quest(const EntityId &id) const {
    return get_quest(id.zone_id(), id.local_id());
}

std::vector<const QuestQueries::QuestData *> QuestManager::get_zone_quests(int zone_id) const {
    std::vector<const QuestQueries::QuestData *> result;

    auto it = zone_quests_.find(zone_id);
    if (it != zone_quests_.end()) {
        result.reserve(it->second.size());
        for (const auto &key : it->second) {
            auto quest_it = quest_cache_.find(key);
            if (quest_it != quest_cache_.end()) {
                result.push_back(&quest_it->second);
            }
        }
    }

    return result;
}

std::vector<const QuestQueries::QuestData *> QuestManager::get_available_quests(std::string_view character_id,
                                                                                int level) const {

    std::vector<const QuestQueries::QuestData *> result;

    // Filter cached quests by level, visibility, and character eligibility
    for (const auto &[key, quest] : quest_cache_) {
        if (quest.hidden)
            continue;
        if (level < quest.min_level || level > quest.max_level)
            continue;

        // Skip quests already in progress
        if (has_quest_in_progress(character_id, key.zone_id, key.quest_id))
            continue;

        // Skip non-repeatable quests already completed
        if (!quest.repeatable && has_completed_quest(character_id, key.zone_id, key.quest_id))
            continue;

        result.push_back(&quest);
    }

    return result;
}

// ============================================================================
// Character Quest Progress
// ============================================================================

Result<void> QuestManager::start_quest(std::string_view character_id, int zone_id, int quest_id) {

    if (!initialized_) {
        return std::unexpected(Errors::InvalidState("QuestManager not initialized"));
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return std::unexpected(Errors::DatabaseError("Database not available"));
    }

    auto result = pool.execute([&](pqxx::work &txn) -> Result<void> {
        return QuestQueries::start_quest(txn, character_id, zone_id, quest_id);
    });

    if (result) {
        stats_.quests_started++;
        spdlog::info("Character {} started quest {}:{}", character_id, zone_id, quest_id);
    }

    return result;
}

Result<void> QuestManager::update_objective(std::string_view character_id, int zone_id, int quest_id, int phase_id,
                                            int objective_id, int new_count) {

    if (!initialized_) {
        return std::unexpected(Errors::InvalidState("QuestManager not initialized"));
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return std::unexpected(Errors::DatabaseError("Database not available"));
    }

    return pool.execute([&](pqxx::work &txn) -> Result<void> {
        return QuestQueries::update_objective_progress(txn, character_id, zone_id, quest_id, phase_id, objective_id,
                                                       new_count);
    });
}

Result<void> QuestManager::complete_objective(std::string_view character_id, int zone_id, int quest_id, int phase_id,
                                              int objective_id) {

    if (!initialized_) {
        return std::unexpected(Errors::InvalidState("QuestManager not initialized"));
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return std::unexpected(Errors::DatabaseError("Database not available"));
    }

    auto result = pool.execute([&](pqxx::work &txn) -> Result<void> {
        return QuestQueries::complete_objective(txn, character_id, zone_id, quest_id, phase_id, objective_id);
    });

    if (result) {
        stats_.objectives_completed++;
    }

    return result;
}

Result<void> QuestManager::advance_phase(std::string_view character_id, int zone_id, int quest_id, int next_phase_id) {

    if (!initialized_) {
        return std::unexpected(Errors::InvalidState("QuestManager not initialized"));
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return std::unexpected(Errors::DatabaseError("Database not available"));
    }

    return pool.execute([&](pqxx::work &txn) -> Result<void> {
        return QuestQueries::advance_phase(txn, character_id, zone_id, quest_id, next_phase_id);
    });
}

Result<void> QuestManager::complete_quest(std::string_view character_id, int zone_id, int quest_id) {

    if (!initialized_) {
        return std::unexpected(Errors::InvalidState("QuestManager not initialized"));
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return std::unexpected(Errors::DatabaseError("Database not available"));
    }

    auto result = pool.execute([&](pqxx::work &txn) -> Result<void> {
        return QuestQueries::complete_quest(txn, character_id, zone_id, quest_id);
    });

    if (result) {
        stats_.quests_completed++;
        spdlog::info("Character {} completed quest {}:{}", character_id, zone_id, quest_id);
    }

    return result;
}

Result<void> QuestManager::abandon_quest(std::string_view character_id, int zone_id, int quest_id) {

    if (!initialized_) {
        return std::unexpected(Errors::InvalidState("QuestManager not initialized"));
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return std::unexpected(Errors::DatabaseError("Database not available"));
    }

    auto result = pool.execute([&](pqxx::work &txn) -> Result<void> {
        return QuestQueries::abandon_quest(txn, character_id, zone_id, quest_id);
    });

    if (result) {
        stats_.quests_abandoned++;
        spdlog::info("Character {} abandoned quest {}:{}", character_id, zone_id, quest_id);
    }

    return result;
}

Result<std::vector<QuestQueries::CharacterQuestProgress>>
QuestManager::get_active_quests(std::string_view character_id) const {

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return std::unexpected(Errors::DatabaseError("Database not available"));
    }

    return pool.execute([&](pqxx::work &txn) -> Result<std::vector<QuestQueries::CharacterQuestProgress>> {
        return QuestQueries::load_character_active_quests(txn, character_id);
    });
}

Result<std::vector<QuestQueries::CharacterQuestProgress>>
QuestManager::get_completed_quests(std::string_view character_id) const {

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return std::unexpected(Errors::DatabaseError("Database not available"));
    }

    return pool.execute([&](pqxx::work &txn) -> Result<std::vector<QuestQueries::CharacterQuestProgress>> {
        return QuestQueries::load_character_completed_quests(txn, character_id);
    });
}

bool QuestManager::has_quest_in_progress(std::string_view character_id, int zone_id, int quest_id) const {

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return false;
    }

    auto result = pool.execute([&](pqxx::work &txn) -> Result<QuestQueries::CharacterQuestProgress> {
        return QuestQueries::load_character_quest(txn, character_id, zone_id, quest_id);
    });

    if (!result) {
        return false;
    }

    return result->status == QuestQueries::QuestStatus::IN_PROGRESS;
}

bool QuestManager::has_completed_quest(std::string_view character_id, int zone_id, int quest_id) const {

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return false;
    }

    auto result = pool.execute([&](pqxx::work &txn) -> Result<QuestQueries::CharacterQuestProgress> {
        return QuestQueries::load_character_quest(txn, character_id, zone_id, quest_id);
    });

    if (!result) {
        return false;
    }

    return result->status == QuestQueries::QuestStatus::COMPLETED;
}

// ============================================================================
// Actor-Based Convenience Methods (for Lua bindings)
// ============================================================================

Result<void> QuestManager::start_quest(Actor &actor, int zone_id, int quest_id) {
    // Only players can have quests - check if this actor is a Player
    const auto *player = dynamic_cast<const Player *>(&actor);
    if (!player) {
        return std::unexpected(Errors::InvalidState("Only players can have quests"));
    }
    auto character_id = player->database_id();
    if (character_id.empty()) {
        return std::unexpected(Errors::InvalidState("Player has no database ID"));
    }
    return start_quest(character_id, zone_id, quest_id);
}

Result<void> QuestManager::complete_quest(Actor &actor, int zone_id, int quest_id) {
    const auto *player = dynamic_cast<const Player *>(&actor);
    if (!player) {
        return std::unexpected(Errors::InvalidState("Only players can have quests"));
    }
    auto character_id = player->database_id();
    if (character_id.empty()) {
        return std::unexpected(Errors::InvalidState("Player has no database ID"));
    }
    return complete_quest(character_id, zone_id, quest_id);
}

Result<void> QuestManager::abandon_quest(Actor &actor, int zone_id, int quest_id) {
    const auto *player = dynamic_cast<const Player *>(&actor);
    if (!player) {
        return std::unexpected(Errors::InvalidState("Only players can have quests"));
    }
    auto character_id = player->database_id();
    if (character_id.empty()) {
        return std::unexpected(Errors::InvalidState("Player has no database ID"));
    }
    return abandon_quest(character_id, zone_id, quest_id);
}

QuestStatus QuestManager::get_quest_status(const Actor &actor, int zone_id, int quest_id) const {
    const auto *player = dynamic_cast<const Player *>(&actor);
    if (!player) {
        return QuestStatus::None; // Only players can have quests
    }
    auto character_id = player->database_id();
    if (character_id.empty()) {
        return QuestStatus::None;
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return QuestStatus::None;
    }

    auto result = pool.execute([&](pqxx::work &txn) -> Result<QuestQueries::CharacterQuestProgress> {
        return QuestQueries::load_character_quest(txn, character_id, zone_id, quest_id);
    });

    if (!result) {
        // Check if quest exists and is available
        if (is_quest_available(actor, zone_id, quest_id)) {
            return QuestStatus::Available;
        }
        return QuestStatus::None;
    }

    // Convert from QuestQueries::QuestStatus to QuestStatus
    switch (result->status) {
    case QuestQueries::QuestStatus::IN_PROGRESS:
        return QuestStatus::InProgress;
    case QuestQueries::QuestStatus::COMPLETED:
        return QuestStatus::Completed;
    case QuestQueries::QuestStatus::FAILED:
        return QuestStatus::Failed;
    case QuestQueries::QuestStatus::ABANDONED:
        return QuestStatus::Abandoned;
    default:
        return QuestStatus::None;
    }
}

bool QuestManager::is_quest_available(const Actor &actor, int zone_id, int quest_id) const {
    const auto *player = dynamic_cast<const Player *>(&actor);
    if (!player) {
        return false; // Only players can have quests
    }
    auto character_id = player->database_id();
    if (character_id.empty()) {
        return false;
    }

    auto quest = get_quest(zone_id, quest_id);
    if (!quest) {
        return false;
    }

    // Check level requirements (use stats().level available on all Actors)
    int level = actor.stats().level;
    if (level < quest->min_level || level > quest->max_level) {
        return false;
    }

    // Check if already in progress or completed
    return !has_quest_in_progress(character_id, zone_id, quest_id) &&
           (!has_completed_quest(character_id, zone_id, quest_id) || quest->repeatable);
}

Result<void> QuestManager::advance_objective(Actor &actor, int zone_id, int quest_id, int objective_id, int count) {
    const auto *player = dynamic_cast<const Player *>(&actor);
    if (!player) {
        return std::unexpected(Errors::InvalidState("Only players can have quests"));
    }
    auto character_id = player->database_id();
    if (character_id.empty()) {
        return std::unexpected(Errors::InvalidState("Player has no database ID"));
    }

    if (!initialized_) {
        return std::unexpected(Errors::InvalidState("QuestManager not initialized"));
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return std::unexpected(Errors::DatabaseError("Database not available"));
    }

    // Get current objective progress
    auto progress_result = pool.execute([&](pqxx::work &txn) -> Result<QuestQueries::CharacterQuestProgress> {
        return QuestQueries::load_character_quest(txn, character_id, zone_id, quest_id);
    });

    if (!progress_result) {
        return std::unexpected(Errors::NotFound("Quest not active for character"));
    }

    // Find the objective and get current count
    for (const auto &obj_progress : progress_result->objective_progress) {
        if (obj_progress.objective_id == objective_id) {
            int new_count = obj_progress.current_count + count;
            return update_objective(character_id, zone_id, quest_id, obj_progress.phase_id, objective_id, new_count);
        }
    }

    return std::unexpected(Errors::NotFound("Objective not found"));
}

Result<void> QuestManager::set_quest_variable(Actor &actor, int zone_id, int quest_id, const std::string &name,
                                              const nlohmann::json &value) {
    const auto *player = dynamic_cast<const Player *>(&actor);
    if (!player) {
        return std::unexpected(Errors::InvalidState("Only players can have quests"));
    }
    auto character_id = player->database_id();
    if (character_id.empty()) {
        return std::unexpected(Errors::InvalidState("Player has no database ID"));
    }

    if (!initialized_) {
        return std::unexpected(Errors::InvalidState("QuestManager not initialized"));
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return std::unexpected(Errors::DatabaseError("Database not available"));
    }

    return pool.execute([&](pqxx::work &txn) -> Result<void> {
        return QuestQueries::set_quest_variable(txn, character_id, zone_id, quest_id, name, value);
    });
}

std::expected<nlohmann::json, Error> QuestManager::get_quest_variable(const Actor &actor, int zone_id, int quest_id,
                                                                      const std::string &name) const {

    const auto *player = dynamic_cast<const Player *>(&actor);
    if (!player) {
        return std::unexpected(Errors::InvalidState("Only players can have quests"));
    }
    auto character_id = player->database_id();
    if (character_id.empty()) {
        return std::unexpected(Errors::InvalidState("Player has no database ID"));
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return std::unexpected(Errors::DatabaseError("Database not available"));
    }

    return pool.execute([&](pqxx::work &txn) -> std::expected<nlohmann::json, Error> {
        return QuestQueries::get_quest_variable(txn, character_id, zone_id, quest_id, name);
    });
}

// ============================================================================
// Trigger-Based Quest Activation
// ============================================================================

std::vector<const QuestQueries::QuestData *> QuestManager::check_level_triggers(std::shared_ptr<Actor> actor,
                                                                                int new_level) {

    std::vector<const QuestQueries::QuestData *> started;

    auto character_id = get_character_id(actor);
    if (character_id.empty()) {
        return started; // Only players have quests
    }

    // Check cached level-triggered quests
    auto it = level_triggered_quests_.find(new_level);
    if (it == level_triggered_quests_.end()) {
        return started;
    }

    for (const auto &key : it->second) {
        auto quest_it = quest_cache_.find(key);
        if (quest_it != quest_cache_.end()) {
            // Check if player can accept this quest
            if (!has_quest_in_progress(character_id, key.zone_id, key.quest_id) &&
                (!quest_it->second.repeatable || !has_completed_quest(character_id, key.zone_id, key.quest_id))) {
                started.push_back(&quest_it->second);
                stats_.trigger_activations++;
            }
        }
    }

    if (!started.empty()) {
        spdlog::debug("Level {} triggered {} quests for player {}", new_level, started.size(), character_id);
    }

    return started;
}

std::vector<const QuestQueries::QuestData *> QuestManager::check_item_triggers(std::shared_ptr<Actor> actor,
                                                                               std::shared_ptr<Object> item) {

    std::vector<const QuestQueries::QuestData *> started;

    if (!item)
        return started;

    auto character_id = get_character_id(actor);
    if (character_id.empty()) {
        return started; // Only players have quests
    }

    // Get item EntityId and look up in item_triggered_quests_
    EntityId item_id = item->id();
    QuestKey item_key{static_cast<int>(item_id.zone_id()), static_cast<int>(item_id.local_id())};

    auto it = item_triggered_quests_.find(item_key);
    if (it == item_triggered_quests_.end()) {
        return started;
    }

    for (const auto &quest_key : it->second) {
        auto quest_it = quest_cache_.find(quest_key);
        if (quest_it != quest_cache_.end()) {
            // Check if player can accept this quest
            if (!has_quest_in_progress(character_id, quest_key.zone_id, quest_key.quest_id) &&
                (!quest_it->second.repeatable ||
                 !has_completed_quest(character_id, quest_key.zone_id, quest_key.quest_id))) {
                started.push_back(&quest_it->second);
                stats_.trigger_activations++;
            }
        }
    }

    if (!started.empty()) {
        spdlog::debug("Item {}:{} triggered {} quests for player {}", item_id.zone_id(), item_id.local_id(),
                      started.size(), character_id);
    }

    return started;
}

std::vector<const QuestQueries::QuestData *> QuestManager::check_room_triggers(std::shared_ptr<Actor> actor,
                                                                               std::shared_ptr<Room> room) {

    std::vector<const QuestQueries::QuestData *> started;

    if (!room)
        return started;

    auto character_id = get_character_id(actor);
    if (character_id.empty()) {
        return started; // Only players have quests
    }

    // Get room EntityId and look up in room_triggered_quests_
    EntityId room_id = room->id();
    QuestKey room_key{static_cast<int>(room_id.zone_id()), static_cast<int>(room_id.local_id())};

    auto it = room_triggered_quests_.find(room_key);
    if (it == room_triggered_quests_.end()) {
        return started;
    }

    for (const auto &quest_key : it->second) {
        auto quest_it = quest_cache_.find(quest_key);
        if (quest_it != quest_cache_.end()) {
            // Check if player can accept this quest
            if (!has_quest_in_progress(character_id, quest_key.zone_id, quest_key.quest_id) &&
                (!quest_it->second.repeatable ||
                 !has_completed_quest(character_id, quest_key.zone_id, quest_key.quest_id))) {
                started.push_back(&quest_it->second);
                stats_.trigger_activations++;
            }
        }
    }

    if (!started.empty()) {
        spdlog::debug("Room {}:{} triggered {} quests for player {}", room_id.zone_id(), room_id.local_id(),
                      started.size(), character_id);
    }

    return started;
}

std::vector<const QuestQueries::QuestData *> QuestManager::check_skill_triggers(std::shared_ptr<Actor> actor,
                                                                                int ability_id) {

    std::vector<const QuestQueries::QuestData *> started;

    auto character_id = get_character_id(actor);
    if (character_id.empty()) {
        return started; // Only players have quests
    }

    auto it = skill_triggered_quests_.find(ability_id);
    if (it == skill_triggered_quests_.end()) {
        return started;
    }

    for (const auto &key : it->second) {
        auto quest_it = quest_cache_.find(key);
        if (quest_it != quest_cache_.end()) {
            // Check if player can accept this quest
            if (!has_quest_in_progress(character_id, key.zone_id, key.quest_id) &&
                (!quest_it->second.repeatable || !has_completed_quest(character_id, key.zone_id, key.quest_id))) {
                started.push_back(&quest_it->second);
                stats_.trigger_activations++;
            }
        }
    }

    if (!started.empty()) {
        spdlog::debug("Skill {} triggered {} quests for player {}", ability_id, started.size(), character_id);
    }

    return started;
}

std::vector<const QuestQueries::QuestData *> QuestManager::check_event_triggers(int event_id) {
    std::vector<const QuestQueries::QuestData *> available;

    auto it = event_triggered_quests_.find(event_id);
    if (it == event_triggered_quests_.end()) {
        return available;
    }

    for (const auto &key : it->second) {
        auto quest_it = quest_cache_.find(key);
        if (quest_it != quest_cache_.end()) {
            available.push_back(&quest_it->second);
            stats_.trigger_activations++;
        }
    }

    if (!available.empty()) {
        spdlog::info("Event {} triggered {} quests", event_id, available.size());
    }

    return available;
}

std::vector<const QuestQueries::QuestData *> QuestManager::get_auto_quests() const {
    std::vector<const QuestQueries::QuestData *> result;
    result.reserve(auto_triggered_quests_.size());

    for (const auto &key : auto_triggered_quests_) {
        auto it = quest_cache_.find(key);
        if (it != quest_cache_.end()) {
            result.push_back(&it->second);
        }
    }

    return result;
}

// ============================================================================
// Objective Progress Tracking
// ============================================================================

void QuestManager::on_mob_killed(std::shared_ptr<Actor> killer, const EntityId &killed_mob_id) {

    auto character_id = get_character_id(killer);
    if (character_id.empty()) {
        spdlog::trace("Mob killed by non-player: {}:{}", killed_mob_id.zone_id(), killed_mob_id.local_id());
        return;
    }

    spdlog::trace("Player {} killed mob {}:{}", character_id, killed_mob_id.zone_id(), killed_mob_id.local_id());

    // Get active quests for this player
    auto active_result = get_active_quests(character_id);
    if (!active_result) {
        return;
    }

    // Check each active quest for KILL_MOB objectives targeting this mob
    for (const auto &progress : *active_result) {
        const auto *quest = get_quest(progress.quest_id);
        if (!quest || !progress.current_phase_id)
            continue;

        // Find objectives in current phase
        for (const auto &phase : quest->phases) {
            if (phase.id != *progress.current_phase_id)
                continue;

            for (const auto &objective : phase.objectives) {
                if (objective.type != QuestQueries::QuestObjectiveType::KILL_MOB)
                    continue;
                if (!objective.target_mob || *objective.target_mob != killed_mob_id)
                    continue;

                // Found matching objective - increment progress
                auto obj_progress = std::find_if(
                    progress.objective_progress.begin(), progress.objective_progress.end(),
                    [&](const auto &op) { return op.phase_id == phase.id && op.objective_id == objective.id; });

                int current = (obj_progress != progress.objective_progress.end()) ? obj_progress->current_count : 0;
                int new_count = current + 1;

                spdlog::debug("Quest {}:{} objective {} progress: {}/{}", quest->id.zone_id(), quest->id.local_id(),
                              objective.id, new_count, objective.required_count);

                // Update in database
                update_objective(character_id, static_cast<int>(quest->id.zone_id()),
                                 static_cast<int>(quest->id.local_id()), phase.id, objective.id, new_count);

                stats_.objectives_completed++;
            }
        }
    }
}

void QuestManager::on_item_collected(std::shared_ptr<Actor> collector, const EntityId &item_id) {

    auto character_id = get_character_id(collector);
    if (character_id.empty()) {
        spdlog::trace("Item collected by non-player: {}:{}", item_id.zone_id(), item_id.local_id());
        return;
    }

    spdlog::trace("Player {} collected item {}:{}", character_id, item_id.zone_id(), item_id.local_id());

    // Get active quests for this player
    auto active_result = get_active_quests(character_id);
    if (!active_result) {
        return;
    }

    // Check each active quest for COLLECT_ITEM objectives
    for (const auto &progress : *active_result) {
        const auto *quest = get_quest(progress.quest_id);
        if (!quest || !progress.current_phase_id)
            continue;

        for (const auto &phase : quest->phases) {
            if (phase.id != *progress.current_phase_id)
                continue;

            for (const auto &objective : phase.objectives) {
                if (objective.type != QuestQueries::QuestObjectiveType::COLLECT_ITEM)
                    continue;
                if (!objective.target_object || *objective.target_object != item_id)
                    continue;

                auto obj_progress = std::find_if(
                    progress.objective_progress.begin(), progress.objective_progress.end(),
                    [&](const auto &op) { return op.phase_id == phase.id && op.objective_id == objective.id; });

                int current = (obj_progress != progress.objective_progress.end()) ? obj_progress->current_count : 0;
                int new_count = current + 1;

                spdlog::debug("Quest {}:{} collect objective {} progress: {}/{}", quest->id.zone_id(),
                              quest->id.local_id(), objective.id, new_count, objective.required_count);

                update_objective(character_id, static_cast<int>(quest->id.zone_id()),
                                 static_cast<int>(quest->id.local_id()), phase.id, objective.id, new_count);

                stats_.objectives_completed++;
            }
        }
    }
}

void QuestManager::on_item_delivered(std::shared_ptr<Actor> deliverer, const EntityId &recipient_mob_id,
                                     const EntityId &item_id) {

    auto character_id = get_character_id(deliverer);
    if (character_id.empty()) {
        spdlog::trace("Item delivered by non-player: {}:{} to {}:{}", item_id.zone_id(), item_id.local_id(),
                      recipient_mob_id.zone_id(), recipient_mob_id.local_id());
        return;
    }

    spdlog::trace("Player {} delivered item {}:{} to {}:{}", character_id, item_id.zone_id(), item_id.local_id(),
                  recipient_mob_id.zone_id(), recipient_mob_id.local_id());

    auto active_result = get_active_quests(character_id);
    if (!active_result) {
        return;
    }

    for (const auto &progress : *active_result) {
        const auto *quest = get_quest(progress.quest_id);
        if (!quest || !progress.current_phase_id)
            continue;

        for (const auto &phase : quest->phases) {
            if (phase.id != *progress.current_phase_id)
                continue;

            for (const auto &objective : phase.objectives) {
                if (objective.type != QuestQueries::QuestObjectiveType::DELIVER_ITEM)
                    continue;
                if (!objective.target_object || *objective.target_object != item_id)
                    continue;
                if (!objective.deliver_to_mob || *objective.deliver_to_mob != recipient_mob_id)
                    continue;

                spdlog::debug("Quest {}:{} deliver objective {} completed", quest->id.zone_id(), quest->id.local_id(),
                              objective.id);

                complete_objective(character_id, static_cast<int>(quest->id.zone_id()),
                                   static_cast<int>(quest->id.local_id()), phase.id, objective.id);

                stats_.objectives_completed++;
            }
        }
    }
}

void QuestManager::on_npc_talked(std::shared_ptr<Actor> talker, const EntityId &npc_id) {

    auto character_id = get_character_id(talker);
    if (character_id.empty()) {
        spdlog::trace("NPC talked to by non-player: {}:{}", npc_id.zone_id(), npc_id.local_id());
        return;
    }

    spdlog::trace("Player {} talked to NPC {}:{}", character_id, npc_id.zone_id(), npc_id.local_id());

    auto active_result = get_active_quests(character_id);
    if (!active_result) {
        return;
    }

    for (const auto &progress : *active_result) {
        const auto *quest = get_quest(progress.quest_id);
        if (!quest || !progress.current_phase_id)
            continue;

        for (const auto &phase : quest->phases) {
            if (phase.id != *progress.current_phase_id)
                continue;

            for (const auto &objective : phase.objectives) {
                if (objective.type != QuestQueries::QuestObjectiveType::TALK_TO_NPC)
                    continue;
                if (!objective.target_mob || *objective.target_mob != npc_id)
                    continue;

                spdlog::debug("Quest {}:{} talk objective {} completed", quest->id.zone_id(), quest->id.local_id(),
                              objective.id);

                complete_objective(character_id, static_cast<int>(quest->id.zone_id()),
                                   static_cast<int>(quest->id.local_id()), phase.id, objective.id);

                stats_.objectives_completed++;
            }
        }
    }
}

void QuestManager::on_room_visited(std::shared_ptr<Actor> visitor, const EntityId &room_id) {

    auto character_id = get_character_id(visitor);
    if (character_id.empty()) {
        spdlog::trace("Room visited by non-player: {}:{}", room_id.zone_id(), room_id.local_id());
        return;
    }

    spdlog::trace("Player {} visited room {}:{}", character_id, room_id.zone_id(), room_id.local_id());

    auto active_result = get_active_quests(character_id);
    if (!active_result) {
        return;
    }

    for (const auto &progress : *active_result) {
        const auto *quest = get_quest(progress.quest_id);
        if (!quest || !progress.current_phase_id)
            continue;

        for (const auto &phase : quest->phases) {
            if (phase.id != *progress.current_phase_id)
                continue;

            for (const auto &objective : phase.objectives) {
                if (objective.type != QuestQueries::QuestObjectiveType::VISIT_ROOM)
                    continue;
                if (!objective.target_room || *objective.target_room != room_id)
                    continue;

                spdlog::debug("Quest {}:{} visit objective {} completed", quest->id.zone_id(), quest->id.local_id(),
                              objective.id);

                complete_objective(character_id, static_cast<int>(quest->id.zone_id()),
                                   static_cast<int>(quest->id.local_id()), phase.id, objective.id);

                stats_.objectives_completed++;
            }
        }
    }
}

void QuestManager::on_skill_used(std::shared_ptr<Actor> user, int ability_id) {

    auto character_id = get_character_id(user);
    if (character_id.empty()) {
        spdlog::trace("Skill used by non-player: {}", ability_id);
        return;
    }

    spdlog::trace("Player {} used skill {}", character_id, ability_id);

    auto active_result = get_active_quests(character_id);
    if (!active_result) {
        return;
    }

    for (const auto &progress : *active_result) {
        const auto *quest = get_quest(progress.quest_id);
        if (!quest || !progress.current_phase_id)
            continue;

        for (const auto &phase : quest->phases) {
            if (phase.id != *progress.current_phase_id)
                continue;

            for (const auto &objective : phase.objectives) {
                if (objective.type != QuestQueries::QuestObjectiveType::USE_SKILL)
                    continue;
                if (!objective.target_ability_id || *objective.target_ability_id != ability_id)
                    continue;

                auto obj_progress = std::find_if(
                    progress.objective_progress.begin(), progress.objective_progress.end(),
                    [&](const auto &op) { return op.phase_id == phase.id && op.objective_id == objective.id; });

                int current = (obj_progress != progress.objective_progress.end()) ? obj_progress->current_count : 0;
                int new_count = current + 1;

                spdlog::debug("Quest {}:{} skill objective {} progress: {}/{}", quest->id.zone_id(),
                              quest->id.local_id(), objective.id, new_count, objective.required_count);

                update_objective(character_id, static_cast<int>(quest->id.zone_id()),
                                 static_cast<int>(quest->id.local_id()), phase.id, objective.id, new_count);

                stats_.objectives_completed++;
            }
        }
    }
}

// ============================================================================
// Diagnostics
// ============================================================================

std::size_t QuestManager::quest_count() const { return quest_cache_.size(); }

std::size_t QuestManager::quest_count(int zone_id) const {
    auto it = zone_quests_.find(zone_id);
    if (it != zone_quests_.end()) {
        return it->second.size();
    }
    return 0;
}

// ============================================================================
// Private Helpers
// ============================================================================

bool QuestManager::can_accept_quest(std::string_view character_id, int zone_id, int quest_id, int level) const {

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        return false;
    }

    auto result = pool.execute([&](pqxx::work &txn) -> Result<bool> {
        return QuestQueries::can_accept_quest(txn, character_id, zone_id, quest_id, level);
    });

    return result.value_or(false);
}

void QuestManager::index_quest_triggers(const QuestQueries::QuestData &quest) {
    QuestKey key{static_cast<int>(quest.id.zone_id()), static_cast<int>(quest.id.local_id())};

    switch (quest.trigger_type) {
    case QuestQueries::QuestTriggerType::LEVEL:
        if (quest.trigger_level) {
            level_triggered_quests_[*quest.trigger_level].push_back(key);
        }
        break;

    case QuestQueries::QuestTriggerType::ITEM:
        if (quest.trigger_item) {
            QuestKey item_key{static_cast<int>(quest.trigger_item->zone_id()),
                              static_cast<int>(quest.trigger_item->local_id())};
            item_triggered_quests_[item_key].push_back(key);
        }
        break;

    case QuestQueries::QuestTriggerType::ROOM:
        if (quest.trigger_room) {
            QuestKey room_key{static_cast<int>(quest.trigger_room->zone_id()),
                              static_cast<int>(quest.trigger_room->local_id())};
            room_triggered_quests_[room_key].push_back(key);
        }
        break;

    case QuestQueries::QuestTriggerType::SKILL:
        if (quest.trigger_ability_id) {
            skill_triggered_quests_[*quest.trigger_ability_id].push_back(key);
        }
        break;

    case QuestQueries::QuestTriggerType::EVENT:
        if (quest.trigger_event_id) {
            event_triggered_quests_[*quest.trigger_event_id].push_back(key);
        }
        break;

    case QuestQueries::QuestTriggerType::AUTO:
        auto_triggered_quests_.push_back(key);
        break;

    case QuestQueries::QuestTriggerType::MOB:
    case QuestQueries::QuestTriggerType::MANUAL:
        // No indexing needed for MOB (handled via dialogue) or MANUAL (GM-only)
        break;
    }
}

void QuestManager::unindex_quest_triggers(const QuestQueries::QuestData &quest) {
    QuestKey key{static_cast<int>(quest.id.zone_id()), static_cast<int>(quest.id.local_id())};

    // Helper to remove key from a vector
    auto remove_key = [&key](std::vector<QuestKey> &vec) {
        std::erase_if(vec,
                      [&key](const QuestKey &k) { return k.zone_id == key.zone_id && k.quest_id == key.quest_id; });
    };

    switch (quest.trigger_type) {
    case QuestQueries::QuestTriggerType::LEVEL:
        if (quest.trigger_level) {
            auto it = level_triggered_quests_.find(*quest.trigger_level);
            if (it != level_triggered_quests_.end()) {
                remove_key(it->second);
            }
        }
        break;

    case QuestQueries::QuestTriggerType::ITEM:
        if (quest.trigger_item) {
            QuestKey item_key{static_cast<int>(quest.trigger_item->zone_id()),
                              static_cast<int>(quest.trigger_item->local_id())};
            auto it = item_triggered_quests_.find(item_key);
            if (it != item_triggered_quests_.end()) {
                remove_key(it->second);
            }
        }
        break;

    case QuestQueries::QuestTriggerType::ROOM:
        if (quest.trigger_room) {
            QuestKey room_key{static_cast<int>(quest.trigger_room->zone_id()),
                              static_cast<int>(quest.trigger_room->local_id())};
            auto it = room_triggered_quests_.find(room_key);
            if (it != room_triggered_quests_.end()) {
                remove_key(it->second);
            }
        }
        break;

    case QuestQueries::QuestTriggerType::SKILL:
        if (quest.trigger_ability_id) {
            auto it = skill_triggered_quests_.find(*quest.trigger_ability_id);
            if (it != skill_triggered_quests_.end()) {
                remove_key(it->second);
            }
        }
        break;

    case QuestQueries::QuestTriggerType::EVENT:
        if (quest.trigger_event_id) {
            auto it = event_triggered_quests_.find(*quest.trigger_event_id);
            if (it != event_triggered_quests_.end()) {
                remove_key(it->second);
            }
        }
        break;

    case QuestQueries::QuestTriggerType::AUTO:
        remove_key(auto_triggered_quests_);
        break;

    case QuestQueries::QuestTriggerType::MOB:
    case QuestQueries::QuestTriggerType::MANUAL:
        break;
    }
}

} // namespace FieryMUD
