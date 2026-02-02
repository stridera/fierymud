/**
 * @file player_repository.cpp
 * @brief Implementation of player repository interfaces
 */

#include "player_repository.hpp"
#include "connection_pool.hpp"
#include "player_queries.hpp"
#include "world_queries.hpp"
#include "core/logging.hpp"
#include "core/player.hpp"

// =============================================================================
// PostgresPlayerRepository Implementation
// =============================================================================

Result<std::unique_ptr<Player>> PostgresPlayerRepository::load_player_by_name(std::string_view name) {
    return ConnectionPool::instance().execute([name](pqxx::work& txn) {
        return PlayerQueries::load_player_by_name(txn, name);
    });
}

Result<std::unique_ptr<Player>> PostgresPlayerRepository::load_player_by_id(std::string_view player_id) {
    return ConnectionPool::instance().execute([player_id](pqxx::work& txn) {
        return PlayerQueries::load_player_by_id(txn, player_id);
    });
}

bool PostgresPlayerRepository::player_exists(std::string_view name) {
    auto result = ConnectionPool::instance().execute([name](pqxx::work& txn) -> Result<bool> {
        return PlayerQueries::player_exists(txn, name);
    });
    return result.has_value() && result.value();
}

int PostgresPlayerRepository::get_player_count() {
    auto result = ConnectionPool::instance().execute([](pqxx::work& txn) -> Result<int> {
        return PlayerQueries::get_player_count(txn);
    });
    return result.has_value() ? result.value() : 0;
}

Result<void> PostgresPlayerRepository::save_character(const WorldQueries::CharacterData& char_data) {
    return ConnectionPool::instance().execute([&char_data](pqxx::work& txn) {
        return WorldQueries::save_character(txn, char_data);
    });
}

Result<void> PostgresPlayerRepository::save_character_items(
    std::string_view character_id,
    const std::vector<WorldQueries::CharacterItemData>& items) {

    return ConnectionPool::instance().execute([character_id, &items](pqxx::work& txn) {
        return WorldQueries::save_character_items(txn, std::string(character_id), items);
    });
}

Result<void> PostgresPlayerRepository::save_character_effects(
    std::string_view character_id,
    const std::vector<WorldQueries::CharacterEffectData>& effects) {

    return ConnectionPool::instance().execute([character_id, &effects](pqxx::work& txn) {
        return WorldQueries::save_character_effects(txn, std::string(character_id), effects);
    });
}

Result<void> PostgresPlayerRepository::save_account_wealth(std::string_view user_id, long wealth_copper) {
    return ConnectionPool::instance().execute([user_id, wealth_copper](pqxx::work& txn) {
        return WorldQueries::save_account_wealth(txn, std::string(user_id), wealth_copper);
    });
}

Result<std::vector<WorldQueries::CharacterEffectData>> PostgresPlayerRepository::load_character_effects(
    std::string_view character_id) {

    return ConnectionPool::instance().execute([character_id](pqxx::work& txn) {
        return WorldQueries::load_character_effects(txn, std::string(character_id));
    });
}

// =============================================================================
// PlayerRepositoryProvider Implementation
// =============================================================================

PlayerRepositoryProvider& PlayerRepositoryProvider::instance() {
    static PlayerRepositoryProvider provider;
    return provider;
}

PlayerRepositoryProvider::PlayerRepositoryProvider()
    : repository_(std::make_unique<PostgresPlayerRepository>())
    , is_mock_(false) {
}

IPlayerRepository& PlayerRepositoryProvider::get() {
    return *repository_;
}

void PlayerRepositoryProvider::set(std::unique_ptr<IPlayerRepository> repo) {
    repository_ = std::move(repo);
    is_mock_ = true;
    Log::debug("PlayerRepository: Switched to mock implementation");
}

void PlayerRepositoryProvider::reset() {
    repository_ = std::make_unique<PostgresPlayerRepository>();
    is_mock_ = false;
    Log::debug("PlayerRepository: Reset to PostgreSQL implementation");
}
