/**
 * @file mock_player_repository.hpp
 * @brief In-memory mock implementation of IPlayerRepository for testing
 *
 * Stores all data in memory, allowing tests to:
 * - Test save/load round-trips without a database
 * - Verify data integrity after persistence operations
 * - Inspect stored data for assertions
 */

#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <core/actor.hpp>
#include <database/player_repository.hpp>
#include <database/world_queries.hpp>

/**
 * @class MockPlayerRepository
 * @brief In-memory storage for player persistence testing
 */
class MockPlayerRepository : public IPlayerRepository {
  public:
    MockPlayerRepository() = default;

    // =========================================================================
    // IPlayerRepository Implementation
    // =========================================================================

    Result<std::unique_ptr<Player>> load_player_by_name(std::string_view name) override {
        std::string lower_name = to_lower(name);
        auto it = characters_by_name_.find(lower_name);
        if (it == characters_by_name_.end()) {
            return std::unexpected(Error{ErrorCode::NotFound, fmt::format("Player '{}' not found", name)});
        }

        // Create player from stored character data
        return create_player_from_data(it->second);
    }

    Result<std::unique_ptr<Player>> load_player_by_id(std::string_view player_id) override {
        std::string id_str{player_id};
        auto it = characters_.find(id_str);
        if (it == characters_.end()) {
            return std::unexpected(Error{ErrorCode::NotFound, fmt::format("Player ID '{}' not found", player_id)});
        }

        return create_player_from_data(it->second);
    }

    bool player_exists(std::string_view name) override {
        std::string lower_name = to_lower(name);
        return characters_by_name_.contains(lower_name);
    }

    int get_player_count() override { return static_cast<int>(characters_.size()); }

    Result<void> save_character(const WorldQueries::CharacterData &char_data) override {
        characters_[char_data.id] = char_data;
        characters_by_name_[to_lower(char_data.name)] = char_data;
        return Success();
    }

    Result<void> save_character_items(std::string_view character_id,
                                      const std::vector<WorldQueries::CharacterItemData> &items) override {

        std::string id_str{character_id};
        character_items_[id_str] = items;
        return Success();
    }

    Result<void> save_character_effects(std::string_view character_id,
                                        const std::vector<WorldQueries::CharacterEffectData> &effects) override {

        std::string id_str{character_id};
        character_effects_[id_str] = effects;
        return Success();
    }

    Result<void> save_account_wealth(std::string_view user_id, long wealth_copper) override {
        std::string id_str{user_id};
        account_wealth_[id_str] = wealth_copper;
        return Success();
    }

    Result<std::vector<WorldQueries::CharacterEffectData>>
    load_character_effects(std::string_view character_id) override {

        std::string id_str{character_id};
        auto it = character_effects_.find(id_str);
        if (it == character_effects_.end()) {
            return std::vector<WorldQueries::CharacterEffectData>{};
        }
        return it->second;
    }

    // =========================================================================
    // Test Helper Methods
    // =========================================================================

    /** Clear all stored data */
    void clear() {
        characters_.clear();
        characters_by_name_.clear();
        character_items_.clear();
        character_effects_.clear();
        account_wealth_.clear();
    }

    /** Get stored character data (for assertions) */
    const WorldQueries::CharacterData *get_character_data(std::string_view id) const {
        auto it = characters_.find(std::string{id});
        return it != characters_.end() ? &it->second : nullptr;
    }

    /** Get stored items (for assertions) */
    const std::vector<WorldQueries::CharacterItemData> *get_items(std::string_view character_id) const {
        auto it = character_items_.find(std::string{character_id});
        return it != character_items_.end() ? &it->second : nullptr;
    }

    /** Get stored effects (for assertions) */
    const std::vector<WorldQueries::CharacterEffectData> *get_effects(std::string_view character_id) const {
        auto it = character_effects_.find(std::string{character_id});
        return it != character_effects_.end() ? &it->second : nullptr;
    }

    /** Get stored account wealth (for assertions) */
    std::optional<long> get_account_wealth(std::string_view user_id) const {
        auto it = account_wealth_.find(std::string{user_id});
        return it != account_wealth_.end() ? std::optional<long>{it->second} : std::nullopt;
    }

    /** Pre-populate with test data */
    void add_test_character(const WorldQueries::CharacterData &char_data) {
        characters_[char_data.id] = char_data;
        characters_by_name_[to_lower(char_data.name)] = char_data;
    }

  private:
    std::unordered_map<std::string, WorldQueries::CharacterData> characters_;
    std::unordered_map<std::string, WorldQueries::CharacterData> characters_by_name_;
    std::unordered_map<std::string, std::vector<WorldQueries::CharacterItemData>> character_items_;
    std::unordered_map<std::string, std::vector<WorldQueries::CharacterEffectData>> character_effects_;
    std::unordered_map<std::string, long> account_wealth_;

    static std::string to_lower(std::string_view str) {
        std::string result{str};
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    Result<std::unique_ptr<Player>> create_player_from_data(const WorldQueries::CharacterData &data) {
        // Create player with stored data
        auto player_result = Player::create(EntityId{1, 0}, data.name);
        if (!player_result) {
            return std::unexpected(player_result.error());
        }

        auto player = std::move(*player_result);
        player->set_database_id(data.id);

        // Restore stats
        auto &stats = player->stats();
        stats.level = data.level;
        stats.alignment = data.alignment;
        stats.strength = data.strength;
        stats.intelligence = data.intelligence;
        stats.wisdom = data.wisdom;
        stats.dexterity = data.dexterity;
        stats.constitution = data.constitution;
        stats.charisma = data.charisma;
        stats.hit_points = data.hit_points;
        stats.max_hit_points = data.hit_points_max;
        stats.stamina = data.stamina;
        stats.max_stamina = data.stamina_max;
        stats.accuracy = data.hit_roll;
        stats.attack_power = data.damage_roll;
        stats.armor_rating = std::max(0, 100 - data.armor_class);

        // Restore wealth
        player->give_wealth(data.wealth);

        // Restore location
        if (data.current_room_zone_id && data.current_room_id) {
            EntityId start_room(static_cast<uint32_t>(*data.current_room_zone_id),
                                static_cast<uint32_t>(*data.current_room_id));
            player->set_start_room(start_room);
        }

        if (data.recall_room_zone_id && data.recall_room_id) {
            EntityId recall_room(static_cast<uint32_t>(*data.recall_room_zone_id),
                                 static_cast<uint32_t>(*data.recall_room_id));
            player->set_recall_room(recall_room);
        }

        // Restore text fields
        if (!data.description.empty()) {
            player->set_description(data.description);
        }
        if (!data.title.empty()) {
            player->set_title(data.title);
        }
        if (!data.prompt.empty()) {
            player->set_prompt(data.prompt);
        }

        // Restore position
        if (!data.position.empty()) {
            auto pos = magic_enum::enum_cast<Position>(data.position, magic_enum::case_insensitive);
            if (pos.has_value()) {
                player->set_position(pos.value());
            }
        }

        return player;
    }
};

/**
 * @class MockRepositoryGuard
 * @brief RAII guard that installs mock repository for test scope
 *
 * Usage:
 *   TEST_CASE("My test") {
 *       MockRepositoryGuard guard;
 *       auto& mock = guard.mock();
 *       // ... test code using mock ...
 *   } // Automatically resets to real repository
 */
class MockRepositoryGuard {
  public:
    MockRepositoryGuard() {
        mock_ = std::make_unique<MockPlayerRepository>();
        mock_ptr_ = mock_.get();
        PlayerRepositoryProvider::instance().set(std::move(mock_));
    }

    ~MockRepositoryGuard() { PlayerRepositoryProvider::instance().reset(); }

    MockPlayerRepository &mock() { return *mock_ptr_; }

  private:
    std::unique_ptr<MockPlayerRepository> mock_;
    MockPlayerRepository *mock_ptr_;
};
