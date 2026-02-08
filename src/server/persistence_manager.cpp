#include "persistence_manager.hpp"

#include "core/logging.hpp"
#include "core/actor.hpp"
#include "core/player.hpp"
#include "core/active_effect.hpp"
#include "core/config.hpp"
#include "core/object.hpp"
#include "database/connection_pool.hpp"
#include "database/player_queries.hpp"
#include "database/world_queries.hpp"

#include <algorithm>
#include <chrono>
#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>

PersistenceManager& PersistenceManager::instance() {
    static PersistenceManager instance;
    return instance;
}

Result<void> PersistenceManager::initialize(const ServerConfig &config) {
    config_ = &config;
    Log::info("PersistenceManager initialized (placeholder)");
    return Success();
}

Result<void> PersistenceManager::save_all_players() {
    Log::debug("PersistenceManager save_all_players (placeholder)");
    return Success();
}

Result<void> PersistenceManager::backup_data() {
    Log::info("PersistenceManager backup_data (placeholder)");
    return Success();
}

Result<void> PersistenceManager::save_player(const Player& player) {
    if (!config_) {
        return std::unexpected(Errors::InvalidState("PersistenceManager not initialized"));
    }

    try {
        // Save character core data to database
        std::string char_id{player.database_id()};
        if (!char_id.empty()) {
            // Build CharacterData from player for database save
            WorldQueries::CharacterData char_data;
            char_data.id = char_id;
            char_data.name = player.name();
            char_data.level = player.stats().level;
            char_data.alignment = player.stats().alignment;

            // Stats
            char_data.strength = player.stats().strength;
            char_data.intelligence = player.stats().intelligence;
            char_data.wisdom = player.stats().wisdom;
            char_data.dexterity = player.stats().dexterity;
            char_data.constitution = player.stats().constitution;
            char_data.charisma = player.stats().charisma;

            // Vitals
            char_data.hit_points = player.stats().hit_points;
            char_data.hit_points_max = player.stats().max_hit_points;
            char_data.stamina = player.stats().stamina;
            char_data.stamina_max = player.stats().max_stamina;

            // Currency - use player.wealth() which gets the wallet value, not stats.gold
            char_data.wealth = player.wealth();

            // Location (from current room)
            if (auto room = player.current_room()) {
                auto room_id = room->id();
                char_data.current_room_zone_id = static_cast<int>(room_id.zone_id());
                char_data.current_room_id = static_cast<int>(room_id.local_id());
            }

            // Recall room
            auto recall = player.recall_room();
            if (recall != INVALID_ENTITY_ID) {
                char_data.recall_room_zone_id = static_cast<int>(recall.zone_id());
                char_data.recall_room_id = static_cast<int>(recall.local_id());
            }

            // Combat stats
            char_data.hit_roll = player.stats().accuracy;
            char_data.damage_roll = player.stats().attack_power;
            char_data.armor_class = std::max(0, 100 - player.stats().armor_rating);

            // Position - database expects uppercase enum names
            std::string pos_name{magic_enum::enum_name(player.position())};
            std::transform(pos_name.begin(), pos_name.end(), pos_name.begin(), ::toupper);
            char_data.position = pos_name;

            // Description and title
            char_data.description = std::string(player.description());
            char_data.title = std::string(player.title());

            // Prompt format string
            char_data.prompt = std::string(player.prompt());

            // Save to database
            auto char_save_result = ConnectionPool::instance().execute(
                [&char_data](pqxx::work& txn) {
                    return WorldQueries::save_character(txn, char_data);
                });

            if (!char_save_result) {
                Log::error("Failed to save character data for player {}: {}",
                          player.name(), char_save_result.error().message);
                // Don't fail entire save - JSON backup was successful
            } else {
                Log::debug("Saved character data to database for player {}", player.name());
            }
        }

        // Save items to database
        if (!char_id.empty()) {
            // Build CharacterItemData from inventory and equipment
            std::vector<WorldQueries::CharacterItemData> items_data;

            // Add inventory items (skip Temporary items - they're dynamic and not saved)
            for (const auto& item : player.inventory().get_all_items()) {
                if (!item) continue;
                if (item->has_flag(ObjectFlag::Temporary)) continue;

                WorldQueries::CharacterItemData item_data;
                item_data.character_id = char_id;
                item_data.object_id = item->id();
                item_data.equipped_location = "";  // Not equipped
                item_data.condition = item->condition();
                item_data.charges = item->charges();
                // Container nesting handled through database relationships

                items_data.push_back(std::move(item_data));
            }

            // Add equipped items (skip Temporary items - they're dynamic and not saved)
            for (const auto& [slot, item] : player.equipment().get_all_equipped_with_slots()) {
                if (!item) continue;
                if (item->has_flag(ObjectFlag::Temporary)) continue;

                WorldQueries::CharacterItemData item_data;
                item_data.character_id = char_id;
                item_data.object_id = item->id();
                item_data.equipped_location = std::string(magic_enum::enum_name(slot));
                item_data.condition = item->condition();
                item_data.charges = item->charges();

                items_data.push_back(std::move(item_data));
            }

            // Save to database
            auto save_result = ConnectionPool::instance().execute(
                [&char_id, &items_data](pqxx::work& txn) {
                    return WorldQueries::save_character_items(txn, char_id, items_data);
                });

            if (!save_result) {
                Log::error("Failed to save items for player {}: {}",
                          player.name(), save_result.error().message);
                // Don't fail the entire save - JSON backup was successful
            } else {
                Log::info("Saved {} items to database for player {}",
                         items_data.size(), player.name());
            }

            // Save account wealth if player has a linked user account
            if (player.has_user_account()) {
                std::string user_id{player.user_id()};
                long account_wealth = player.account_bank().value();

                auto account_save_result = ConnectionPool::instance().execute(
                    [&user_id, account_wealth](pqxx::work& txn) {
                        return WorldQueries::save_account_wealth(txn, user_id, account_wealth);
                    });

                if (!account_save_result) {
                    Log::error("Failed to save account wealth for player {}: {}",
                              player.name(), account_save_result.error().message);
                } else {
                    Log::debug("Saved account wealth {} for player {}",
                              account_wealth, player.name());
                }
            }

            // Save active effects (buffs, debuffs, DoTs, HoTs)
            std::vector<WorldQueries::CharacterEffectData> effects_data;
            auto now = std::chrono::system_clock::now();

            // Convert ActiveEffects to CharacterEffectData
            for (const auto& effect : player.active_effects()) {
                // Skip effects without a valid effect_id (shouldn't happen, but be safe)
                if (effect.effect_id <= 0) {
                    Log::warn("Effect '{}' has no effect_id ({}), skipping save", effect.name, effect.effect_id);
                    continue;
                }

                WorldQueries::CharacterEffectData effect_data;
                effect_data.character_id = char_id;
                effect_data.effect_id = effect.effect_id;
                effect_data.strength = 1;
                effect_data.source_type = effect.source.empty() ? "ability" : effect.source;
                effect_data.applied_at = now;

                // Convert duration from hours to seconds
                if (effect.duration_hours >= 0) {
                    int duration_secs = static_cast<int>(effect.duration_hours * 3600.0);
                    effect_data.duration_seconds = duration_secs;
                    effect_data.expires_at = now + std::chrono::seconds(duration_secs);
                }

                // Store modifier info in JSON
                nlohmann::json modifier_json;
                modifier_json["effect_name"] = effect.name;  // Save original spell/ability name
                modifier_json["flag"] = std::string(magic_enum::enum_name(effect.flag));
                modifier_json["modifier_value"] = effect.modifier_value;
                modifier_json["modifier_stat"] = effect.modifier_stat;
                modifier_json["effect_type"] = "active";
                effect_data.modifier_data = modifier_json.dump();

                effects_data.push_back(std::move(effect_data));
            }

            // Convert DoT effects to CharacterEffectData
            for (const auto& dot : player.dot_effects()) {
                WorldQueries::CharacterEffectData effect_data;
                effect_data.character_id = char_id;
                effect_data.effect_id = dot.effect_id;
                effect_data.strength = dot.stack_count;
                effect_data.source_type = "ability";
                effect_data.source_id = dot.ability_id;
                effect_data.applied_at = now;

                // Convert remaining ticks to seconds (assuming 1 tick = 2 seconds game time)
                constexpr int SECONDS_PER_TICK = 2;
                if (dot.remaining_ticks >= 0) {
                    int duration_secs = dot.remaining_ticks * SECONDS_PER_TICK;
                    effect_data.duration_seconds = duration_secs;
                    effect_data.expires_at = now + std::chrono::seconds(duration_secs);
                }

                // Store DoT-specific data in JSON
                nlohmann::json modifier_json;
                modifier_json["effect_type"] = "dot";
                modifier_json["damage_type"] = dot.damage_type;
                modifier_json["cure_category"] = dot.cure_category;
                modifier_json["potency"] = dot.potency;
                modifier_json["flat_damage"] = dot.flat_damage;
                modifier_json["percent_damage"] = dot.percent_damage;
                modifier_json["blocks_regen"] = dot.blocks_regen;
                modifier_json["reduces_regen"] = dot.reduces_regen;
                modifier_json["tick_interval"] = dot.tick_interval;
                modifier_json["ticks_since_last"] = dot.ticks_since_last;
                modifier_json["remaining_ticks"] = dot.remaining_ticks;
                modifier_json["source_actor_id"] = dot.source_actor_id;
                modifier_json["source_level"] = dot.source_level;
                modifier_json["max_stacks"] = dot.max_stacks;
                modifier_json["stackable"] = dot.stackable;
                effect_data.modifier_data = modifier_json.dump();

                effects_data.push_back(std::move(effect_data));
            }

            // Convert HoT effects to CharacterEffectData
            for (const auto& hot : player.hot_effects()) {
                WorldQueries::CharacterEffectData effect_data;
                effect_data.character_id = char_id;
                effect_data.effect_id = hot.effect_id;
                effect_data.strength = hot.stack_count;
                effect_data.source_type = "ability";
                effect_data.source_id = hot.ability_id;
                effect_data.applied_at = now;

                // Convert remaining ticks to seconds
                constexpr int SECONDS_PER_TICK = 2;
                if (hot.remaining_ticks >= 0) {
                    int duration_secs = hot.remaining_ticks * SECONDS_PER_TICK;
                    effect_data.duration_seconds = duration_secs;
                    effect_data.expires_at = now + std::chrono::seconds(duration_secs);
                }

                // Store HoT-specific data in JSON
                nlohmann::json modifier_json;
                modifier_json["effect_type"] = "hot";
                modifier_json["heal_type"] = hot.heal_type;
                modifier_json["hot_category"] = hot.hot_category;
                modifier_json["flat_heal"] = hot.flat_heal;
                modifier_json["percent_heal"] = hot.percent_heal;
                modifier_json["boosts_regen"] = hot.boosts_regen;
                modifier_json["regen_boost"] = hot.regen_boost;
                modifier_json["tick_interval"] = hot.tick_interval;
                modifier_json["ticks_since_last"] = hot.ticks_since_last;
                modifier_json["remaining_ticks"] = hot.remaining_ticks;
                modifier_json["source_actor_id"] = hot.source_actor_id;
                modifier_json["source_level"] = hot.source_level;
                modifier_json["max_stacks"] = hot.max_stacks;
                modifier_json["stackable"] = hot.stackable;
                effect_data.modifier_data = modifier_json.dump();

                effects_data.push_back(std::move(effect_data));
            }

            // Save effects to database
            if (!effects_data.empty()) {
                auto effects_save_result = ConnectionPool::instance().execute(
                    [&char_id, &effects_data](pqxx::work& txn) {
                        return WorldQueries::save_character_effects(txn, char_id, effects_data);
                    });

                if (!effects_save_result) {
                    Log::error("Failed to save effects for player {}: {}",
                              player.name(), effects_save_result.error().message);
                } else {
                    Log::info("Saved {} effects to database for player {}",
                             effects_data.size(), player.name());
                }
            } else {
                // No effects to save, clear any existing effects in database
                auto clear_result = ConnectionPool::instance().execute(
                    [&char_id](pqxx::work& txn) {
                        return WorldQueries::delete_character_effects(txn, char_id);
                    });
                if (clear_result) {
                    Log::debug("Cleared effects from database for player {}", player.name());
                }
            }

            // Save character aliases
            const auto& player_aliases = player.get_aliases();
            if (!player_aliases.empty()) {
                std::vector<WorldQueries::CharacterAliasData> aliases_data;
                aliases_data.reserve(player_aliases.size());

                for (const auto& [alias, command] : player_aliases) {
                    WorldQueries::CharacterAliasData alias_data;
                    alias_data.character_id = char_id;
                    alias_data.alias = alias;
                    alias_data.command = command;
                    aliases_data.push_back(std::move(alias_data));
                }

                auto aliases_save_result = ConnectionPool::instance().execute(
                    [&char_id, &aliases_data](pqxx::work& txn) {
                        return WorldQueries::save_character_aliases(txn, char_id, aliases_data);
                    });

                if (!aliases_save_result) {
                    Log::error("Failed to save aliases for player {}: {}",
                              player.name(), aliases_save_result.error().message);
                } else {
                    Log::debug("Saved {} aliases to database for player {}",
                             aliases_data.size(), player.name());
                }
            } else {
                // No aliases to save, clear any existing aliases in database
                auto clear_result = ConnectionPool::instance().execute(
                    [&char_id](pqxx::work& txn) {
                        return WorldQueries::delete_character_aliases(txn, char_id);
                    });
                if (clear_result) {
                    Log::debug("Cleared aliases from database for player {}", player.name());
                }
            }
        } else {
            Log::warn("Cannot save items to database: player {} has no database_id", player.name());
        }

        Log::info("Saved player {}", player.name());
        return Success();

    } catch (const std::exception& e) {
        return std::unexpected(Errors::FileSystem("Failed to save player: " + std::string(e.what())));
    }
}

Result<std::shared_ptr<Player>> PersistenceManager::load_player(std::string_view name) {
    if (!config_) {
        return std::unexpected(Errors::InvalidState("PersistenceManager not initialized"));
    }

    // Load player from database using connection pool
    auto player_result = ConnectionPool::instance().execute([name](pqxx::work& txn) {
        return PlayerQueries::load_player_by_name(txn, name);
    });

    if (!player_result) {
        Log::debug("Failed to load player '{}' from database: {}", name, player_result.error().message);
        return std::unexpected(player_result.error());
    }

    // Convert unique_ptr to shared_ptr
    auto player = std::shared_ptr<Player>(player_result.value().release());

    // Load and restore active effects
    std::string char_id{player->database_id()};
    if (!char_id.empty()) {
        auto effects_result = ConnectionPool::instance().execute(
            [&char_id](pqxx::work& txn) {
                return WorldQueries::load_character_effects(txn, char_id);
            });

        if (effects_result) {
            auto now = std::chrono::system_clock::now();

            for (const auto& effect_data : effects_result.value()) {
                try {
                    nlohmann::json modifier_json = nlohmann::json::parse(effect_data.modifier_data);
                    std::string effect_type = modifier_json.value("effect_type", "active");

                    // Check if effect has expired
                    if (effect_data.expires_at.has_value() && *effect_data.expires_at <= now) {
                        Log::debug("Skipping expired effect '{}' for player {}",
                                  effect_data.effect_name, name);
                        continue;
                    }

                    if (effect_type == "dot") {
                        // Restore DoT effect
                        fiery::DotEffect dot;
                        dot.ability_id = effect_data.source_id.value_or(0);
                        dot.effect_id = effect_data.effect_id;
                        dot.effect_type = "dot";
                        dot.damage_type = modifier_json.value("damage_type", "");
                        dot.cure_category = modifier_json.value("cure_category", "");
                        dot.potency = modifier_json.value("potency", 5);
                        dot.flat_damage = modifier_json.value("flat_damage", 0);
                        dot.percent_damage = modifier_json.value("percent_damage", 0);
                        dot.blocks_regen = modifier_json.value("blocks_regen", false);
                        dot.reduces_regen = modifier_json.value("reduces_regen", 0);
                        dot.tick_interval = modifier_json.value("tick_interval", 1);
                        dot.ticks_since_last = modifier_json.value("ticks_since_last", 0);
                        dot.remaining_ticks = modifier_json.value("remaining_ticks", -1);
                        dot.source_actor_id = modifier_json.value("source_actor_id", "");
                        dot.source_level = modifier_json.value("source_level", 1);
                        dot.stack_count = effect_data.strength;
                        dot.max_stacks = modifier_json.value("max_stacks", 1);
                        dot.stackable = modifier_json.value("stackable", false);

                        player->add_dot_effect(dot);
                        Log::debug("Restored DoT effect for player {}", name);

                    } else if (effect_type == "hot") {
                        // Restore HoT effect
                        fiery::HotEffect hot;
                        hot.ability_id = effect_data.source_id.value_or(0);
                        hot.effect_id = effect_data.effect_id;
                        hot.effect_type = "hot";
                        hot.heal_type = modifier_json.value("heal_type", "");
                        hot.hot_category = modifier_json.value("hot_category", "");
                        hot.flat_heal = modifier_json.value("flat_heal", 0);
                        hot.percent_heal = modifier_json.value("percent_heal", 0);
                        hot.boosts_regen = modifier_json.value("boosts_regen", false);
                        hot.regen_boost = modifier_json.value("regen_boost", 0);
                        hot.tick_interval = modifier_json.value("tick_interval", 1);
                        hot.ticks_since_last = modifier_json.value("ticks_since_last", 0);
                        hot.remaining_ticks = modifier_json.value("remaining_ticks", -1);
                        hot.source_actor_id = modifier_json.value("source_actor_id", "");
                        hot.source_level = modifier_json.value("source_level", 1);
                        hot.stack_count = effect_data.strength;
                        hot.max_stacks = modifier_json.value("max_stacks", 1);
                        hot.stackable = modifier_json.value("stackable", false);

                        player->add_hot_effect(hot);
                        Log::debug("Restored HoT effect for player {}", name);

                    } else {
                        // Restore regular ActiveEffect
                        ActiveEffect effect;
                        effect.effect_id = effect_data.effect_id;
                        effect.name = effect_data.effect_name;
                        effect.source = effect_data.source_type;

                        // Parse flag from modifier data
                        std::string flag_str = modifier_json.value("flag", "None");
                        auto flag_opt = magic_enum::enum_cast<ActorFlag>(flag_str);
                        effect.flag = flag_opt.value_or(ActorFlag::None);

                        effect.modifier_value = modifier_json.value("modifier_value", 0);
                        effect.modifier_stat = modifier_json.value("modifier_stat", "");
                        effect.applied_at = std::chrono::steady_clock::now();

                        // Calculate remaining duration
                        if (effect_data.duration_seconds.has_value()) {
                            // Convert seconds back to hours
                            effect.duration_hours = static_cast<double>(*effect_data.duration_seconds) / 3600.0;

                            // Adjust for time that has passed since saving
                            if (effect_data.expires_at.has_value()) {
                                auto time_left = std::chrono::duration_cast<std::chrono::seconds>(
                                    *effect_data.expires_at - now);
                                if (time_left.count() > 0) {
                                    effect.duration_hours = static_cast<double>(time_left.count()) / 3600.0;
                                } else {
                                    // Effect has expired, skip it
                                    continue;
                                }
                            }
                        } else {
                            effect.duration_hours = -1;  // Permanent
                        }

                        player->add_effect(effect);
                        Log::debug("Restored active effect '{}' for player {}", effect.name, name);
                    }
                } catch (const nlohmann::json::exception& e) {
                    Log::warn("Failed to parse effect data for player {}: {}", name, e.what());
                }
            }

            Log::info("Loaded {} effects for player '{}'",
                     effects_result.value().size(), name);
        } else {
            Log::debug("No effects to load for player '{}': {}",
                      name, effects_result.error().message);
        }

        // Load character aliases
        auto aliases_result = ConnectionPool::instance().execute(
            [&char_id](pqxx::work& txn) {
                return WorldQueries::load_character_aliases(txn, char_id);
            });

        if (aliases_result) {
            for (const auto& alias_data : aliases_result.value()) {
                player->set_alias(alias_data.alias, alias_data.command);
            }
            Log::debug("Loaded {} aliases for player '{}'",
                      aliases_result.value().size(), name);
        } else {
            Log::debug("No aliases to load for player '{}': {}",
                      name, aliases_result.error().message);
        }
    }

    Log::info("Loaded player '{}' from database", name);
    return player;
}
