/**
 * @file player_repository.hpp
 * @brief Repository interface for player persistence operations
 *
 * Provides an abstraction layer over database operations to enable:
 * - Unit testing with mock implementations
 * - Swappable storage backends
 * - Clean separation of concerns
 */

#pragma once

#include "../core/result.hpp"
#include "../core/actor.hpp"
#include "../core/active_effect.hpp"
#include "world_queries.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

/**
 * @interface IPlayerRepository
 * @brief Abstract interface for player data persistence
 *
 * Implementations:
 * - PostgresPlayerRepository: Real database via ConnectionPool
 * - MockPlayerRepository: In-memory storage for testing
 */
class IPlayerRepository {
public:
    virtual ~IPlayerRepository() = default;

    // =========================================================================
    // Player Loading
    // =========================================================================

    /**
     * Load a player by name
     * @param name Player name (case-insensitive)
     * @return Player object or error
     */
    virtual Result<std::unique_ptr<Player>> load_player_by_name(std::string_view name) = 0;

    /**
     * Load a player by database ID
     * @param player_id Database UUID
     * @return Player object or error
     */
    virtual Result<std::unique_ptr<Player>> load_player_by_id(std::string_view player_id) = 0;

    /**
     * Check if a player exists
     * @param name Player name (case-insensitive)
     * @return true if player exists
     */
    virtual bool player_exists(std::string_view name) = 0;

    /**
     * Get total player count
     * @return Number of players in database
     */
    virtual int get_player_count() = 0;

    // =========================================================================
    // Player Saving
    // =========================================================================

    /**
     * Save character core data (stats, location, etc.)
     * @param char_data Character data to save
     * @return Success or error
     */
    virtual Result<void> save_character(const WorldQueries::CharacterData& char_data) = 0;

    /**
     * Save character items (inventory and equipment)
     * @param character_id Database UUID of character
     * @param items Item data to save
     * @return Success or error
     */
    virtual Result<void> save_character_items(
        std::string_view character_id,
        const std::vector<WorldQueries::CharacterItemData>& items) = 0;

    /**
     * Save character active effects
     * @param character_id Database UUID of character
     * @param effects Effect data to save
     * @return Success or error
     */
    virtual Result<void> save_character_effects(
        std::string_view character_id,
        const std::vector<WorldQueries::CharacterEffectData>& effects) = 0;

    /**
     * Save account wealth (shared across characters)
     * @param user_id User account UUID
     * @param wealth_copper Total wealth in copper
     * @return Success or error
     */
    virtual Result<void> save_account_wealth(std::string_view user_id, long wealth_copper) = 0;

    // =========================================================================
    // Effect Loading
    // =========================================================================

    /**
     * Load active effects for a character
     * @param character_id Database UUID of character
     * @return Vector of effect data or error
     */
    virtual Result<std::vector<WorldQueries::CharacterEffectData>> load_character_effects(
        std::string_view character_id) = 0;
};

/**
 * @class PostgresPlayerRepository
 * @brief Real database implementation using ConnectionPool
 */
class PostgresPlayerRepository : public IPlayerRepository {
public:
    Result<std::unique_ptr<Player>> load_player_by_name(std::string_view name) override;
    Result<std::unique_ptr<Player>> load_player_by_id(std::string_view player_id) override;
    bool player_exists(std::string_view name) override;
    int get_player_count() override;

    Result<void> save_character(const WorldQueries::CharacterData& char_data) override;
    Result<void> save_character_items(
        std::string_view character_id,
        const std::vector<WorldQueries::CharacterItemData>& items) override;
    Result<void> save_character_effects(
        std::string_view character_id,
        const std::vector<WorldQueries::CharacterEffectData>& effects) override;
    Result<void> save_account_wealth(std::string_view user_id, long wealth_copper) override;

    Result<std::vector<WorldQueries::CharacterEffectData>> load_character_effects(
        std::string_view character_id) override;
};

/**
 * @class PlayerRepositoryProvider
 * @brief Provides access to the current player repository implementation
 *
 * Allows swapping between real and mock implementations for testing.
 */
class PlayerRepositoryProvider {
public:
    /** Get singleton instance */
    static PlayerRepositoryProvider& instance();

    /** Get current repository */
    IPlayerRepository& get();

    /** Set repository (for testing - injects mock) */
    void set(std::unique_ptr<IPlayerRepository> repo);

    /** Reset to default PostgreSQL implementation */
    void reset();

    /** Check if using mock */
    bool is_mock() const { return is_mock_; }

private:
    PlayerRepositoryProvider();
    std::unique_ptr<IPlayerRepository> repository_;
    bool is_mock_ = false;
};
