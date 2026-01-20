#include "login_system.hpp"

#include "../core/actor.hpp"
#include "../core/config.hpp"
#include "../core/logging.hpp"
#include "../core/money.hpp"
#include "../core/object.hpp"
#include "../database/connection_pool.hpp"
#include "../database/game_data_cache.hpp"
#include "../database/player_queries.hpp"
#include "../database/world_queries.hpp"
#include "../net/player_connection.hpp"
#include "../server/network_manager.hpp"
#include "../text/string_utils.hpp"
#include "../world/world_manager.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <magic_enum/magic_enum.hpp>

// Immortal level threshold - characters at or above this level are considered gods
// and bypass zone level restrictions. Legacy: LVL_IMMORT = 100
constexpr int kImmortalLevel = 100;

// CharacterCreationData implementation
bool CharacterCreationData::is_complete() const { return !name.empty() && !password.empty(); }

std::string CharacterCreationData::get_class_name() const {
    switch (character_class) {
    case CharacterClass::Warrior:
        return "Warrior";
    case CharacterClass::Cleric:
        return "Cleric";
    case CharacterClass::Sorcerer:
        return "Sorcerer";
    case CharacterClass::Rogue:
        return "Rogue";
    default:
        return "Unknown";
    }
}

std::string CharacterCreationData::get_race_name() const {
    switch (race) {
    case CharacterRace::Human:
        return "Human";
    case CharacterRace::Elf:
        return "Elf";
    case CharacterRace::Dwarf:
        return "Dwarf";
    case CharacterRace::Halfling:
        return "Halfling";
    default:
        return "Unknown";
    }
}

// LoginSystem implementation
LoginSystem::LoginSystem(std::shared_ptr<PlayerConnection> connection) : connection_(std::move(connection)) {}

// Helper function to load character abilities from database
void LoginSystem::load_player_abilities(std::shared_ptr<Player> &player) {
    // Gods get ALL abilities at max proficiency
    if (player->is_god()) {
        auto all_abilities_result =
            ConnectionPool::instance().execute([](pqxx::work &txn) -> Result<std::vector<WorldQueries::AbilityData>> {
                return WorldQueries::load_all_abilities(txn);
            });

        if (!all_abilities_result) {
            Log::warn("Failed to load all abilities for god {}: {}", player->name(),
                      all_abilities_result.error().message);
            return;
        }

        for (const auto &ability_data : *all_abilities_result) {
            LearnedAbility learned;
            learned.ability_id = ability_data.id;
            learned.name = ability_data.name;
            learned.plain_name = ability_data.plain_name;
            learned.description = ability_data.description;
            learned.known = true;
            learned.proficiency = 1000; // Max proficiency for gods
            learned.violent = ability_data.violent;
            learned.min_level = 1; // Gods can use everything

            // Convert enum to string
            switch (ability_data.type) {
            case WorldQueries::AbilityType::Spell:
                learned.type = "SPELL";
                break;
            case WorldQueries::AbilityType::Skill:
                learned.type = "SKILL";
                break;
            case WorldQueries::AbilityType::Chant:
                learned.type = "CHANT";
                break;
            case WorldQueries::AbilityType::Song:
                learned.type = "SONG";
                break;
            }

            player->set_ability(learned);
        }

        Log::info("Granted {} abilities to god '{}'", all_abilities_result->size(), player->name());
        return;
    }

    // Regular players load from CharacterAbilities
    if (player->database_id().empty()) {
        Log::warn("Cannot load abilities: player {} has no database_id", player->name());
        return;
    }

    std::string char_id{player->database_id()};
    auto result = ConnectionPool::instance().execute(
        [&char_id](pqxx::work &txn) -> Result<std::vector<WorldQueries::CharacterAbilityWithMetadata>> {
            return WorldQueries::load_character_abilities_with_metadata(txn, char_id);
        });

    if (!result) {
        Log::warn("Failed to load abilities for player {}: {}", player->name(), result.error().message);
        return;
    }

    // Apply abilities to player
    for (const auto &ability_data : *result) {
        LearnedAbility learned;
        learned.ability_id = ability_data.ability_id;
        learned.name = ability_data.name;
        learned.plain_name = ability_data.plain_name;
        learned.description = ability_data.description;
        learned.known = ability_data.known;
        learned.proficiency = ability_data.proficiency;
        learned.violent = ability_data.violent;
        learned.circle = ability_data.circle;
        learned.sphere = ability_data.sphere;

        // Convert enum to string
        switch (ability_data.type) {
        case WorldQueries::AbilityType::Spell:
            learned.type = "SPELL";
            break;
        case WorldQueries::AbilityType::Skill:
            learned.type = "SKILL";
            break;
        case WorldQueries::AbilityType::Chant:
            learned.type = "CHANT";
            break;
        case WorldQueries::AbilityType::Song:
            learned.type = "SONG";
            break;
        }

        player->set_ability(learned);
    }

    Log::info("Loaded {} abilities for player '{}'", result->size(), player->name());
}

void LoginSystem::load_player_items(std::shared_ptr<Player> &player) {
    if (player->database_id().empty()) {
        Log::warn("Cannot load items: player {} has no database_id", player->name());
        return;
    }

    std::string char_id{player->database_id()};
    auto result = ConnectionPool::instance().execute(
        [&char_id](pqxx::work &txn) -> Result<std::vector<WorldQueries::CharacterItemData>> {
            return WorldQueries::load_character_items(txn, char_id);
        });

    if (!result) {
        Log::warn("Failed to load items for player {}: {}", player->name(), result.error().message);
        return;
    }

    const auto &items_data = *result;
    if (items_data.empty()) {
        Log::debug("No items to load for player '{}'", player->name());
        return;
    }

    auto &world = WorldManager::instance();

    // Map from CharacterItems.id to created object (for container nesting)
    std::unordered_map<int, std::shared_ptr<Object>> created_objects;

    // First pass: create all objects
    for (const auto &item_data : items_data) {
        auto object = world.create_object_instance(item_data.object_id);
        if (!object) {
            Log::warn("Failed to create object instance for prototype {} for player {}", item_data.object_id,
                      player->name());
            continue;
        }

        // Apply custom name if set
        if (!item_data.custom_name.empty()) {
            object->set_name(item_data.custom_name);
        }

        // Apply custom description if set
        if (!item_data.custom_description.empty()) {
            object->set_description(item_data.custom_description);
        }

        // Store for second pass
        created_objects[item_data.id] = object;
    }

    // Second pass: place items (handle containers, equipment, inventory)
    int equipped_count = 0;
    int inventory_count = 0;
    int container_count = 0;

    for (const auto &item_data : items_data) {
        auto it = created_objects.find(item_data.id);
        if (it == created_objects.end()) {
            continue; // Object creation failed in first pass
        }

        auto object = it->second;

        // If this item is inside a container
        if (item_data.container_id.has_value()) {
            auto container_it = created_objects.find(*item_data.container_id);
            if (container_it != created_objects.end() && container_it->second->is_container()) {
                auto *container_ptr = dynamic_cast<Container *>(container_it->second.get());
                if (container_ptr) {
                    auto add_result = container_ptr->add_item(object);
                    if (add_result) {
                        container_count++;
                        continue;
                    }
                    Log::warn("Failed to add item {} to container for player {}", object->name(), player->name());
                }
            }
            // Fallback: put in inventory
            player->inventory().add_item(object);
            inventory_count++;
            continue;
        }

        // If this item is equipped
        if (!item_data.equipped_location.empty()) {
            auto slot = ObjectUtils::parse_equip_slot(item_data.equipped_location);
            if (slot.has_value()) {
                auto equip_result = player->equipment().equip_to_slot(object, *slot);
                if (equip_result) {
                    equipped_count++;
                    continue;
                }
                Log::warn("Failed to equip item {} in slot {} for player {}", object->name(),
                          item_data.equipped_location, player->name());
            } else {
                Log::warn("Unknown equipment slot '{}' for item {} for player {}", item_data.equipped_location,
                          object->name(), player->name());
            }
            // Fallback: put in inventory
            player->inventory().add_item(object);
            inventory_count++;
            continue;
        }

        // Regular inventory item
        player->inventory().add_item(object);
        inventory_count++;
    }

    Log::info("Loaded {} items for player '{}' ({} equipped, {} inventory, {} in containers)", created_objects.size(),
              player->name(), equipped_count, inventory_count, container_count);
}

void LoginSystem::load_player_effects(std::shared_ptr<Player> &player) {
    if (player->database_id().empty()) {
        Log::warn("Cannot load effects: player {} has no database_id", player->name());
        return;
    }

    std::string char_id{player->database_id()};
    auto effects_result = ConnectionPool::instance().execute(
        [&char_id](pqxx::work& txn) {
            return WorldQueries::load_character_effects(txn, char_id);
        });

    if (!effects_result) {
        Log::debug("No effects to load for player '{}': {}", player->name(), effects_result.error().message);
        return;
    }

    auto now = std::chrono::system_clock::now();
    int loaded_count = 0;

    for (const auto& effect_data : effects_result.value()) {
        try {
            nlohmann::json modifier_json = nlohmann::json::parse(effect_data.modifier_data);
            std::string effect_type = modifier_json.value("effect_type", "active");

            // Check if effect has expired
            if (effect_data.expires_at.has_value() && *effect_data.expires_at <= now) {
                Log::debug("Skipping expired effect '{}' for player {}", effect_data.effect_name, player->name());
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
                loaded_count++;
                Log::debug("Restored DoT effect for player {}", player->name());

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
                loaded_count++;
                Log::debug("Restored HoT effect for player {}", player->name());

            } else {
                // Restore regular ActiveEffect
                ActiveEffect effect;
                effect.effect_id = effect_data.effect_id;
                // Use saved effect name from modifier_data if available, otherwise fall back to effect type name
                effect.name = modifier_json.value("effect_name", effect_data.effect_name);
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
                loaded_count++;
                Log::debug("Restored active effect '{}' for player {}", effect.name, player->name());
            }
        } catch (const nlohmann::json::exception& e) {
            Log::warn("Failed to parse effect data for player {}: {}", player->name(), e.what());
        }
    }

    if (loaded_count > 0) {
        Log::info("Loaded {} effects for player '{}'", loaded_count, player->name());
    }
}

void LoginSystem::start_login() {
    Log::debug("Starting login process for new connection");
    transition_to(LoginState::GetAccount);
    send_welcome_message();
    send_prompt();

    // Process any input that was buffered during the connection phase
    // This handles cases where clients send input before the welcome message
    if (!buffered_input_.empty()) {
        Log::debug("Processing {} buffered input(s)", buffered_input_.size());
        auto buffered = std::move(buffered_input_);
        buffered_input_.clear();
        for (size_t i = 0; i < buffered.size(); ++i) {
            // If login completed, forward remaining commands to the game
            if (state_ == LoginState::Playing && connection_) {
                Log::debug("Login complete, forwarding {} remaining commands to game", buffered.size() - i);
                for (size_t j = i; j < buffered.size(); ++j) {
                    // Route through connection's process_input which will send to world_server
                    connection_->forward_command_to_game(buffered[j]);
                }
                break;
            }
            process_input(buffered[i]);
        }
    }
}

void LoginSystem::process_input(std::string_view input) {
    if (state_ == LoginState::Disconnected) {
        return;
    }

    // Trim whitespace
    std::string trimmed_input{trim(input)};

    Log::debug("Processing login input in state {}: '{}'", static_cast<int>(state_), trimmed_input);

    switch (state_) {
    // Initial state - buffer input until start_login() is called
    case LoginState::Connecting:
        // Buffer input received before welcome message is displayed
        if (!trimmed_input.empty()) {
            Log::debug("Buffering early input: '{}'", trimmed_input);
            buffered_input_.push_back(trimmed_input);
        }
        break;

    // Account-based login flow
    case LoginState::GetAccount:
        handle_get_account(trimmed_input);
        break;
    case LoginState::GetAccountPassword:
        handle_get_account_password(trimmed_input);
        break;
    case LoginState::SelectCharacter:
        handle_select_character(trimmed_input);
        break;

    // Legacy character-based login / character creation flow
    case LoginState::GetName:
        handle_get_name(trimmed_input);
        break;
    case LoginState::GetPassword:
        handle_get_password(trimmed_input);
        break;
    case LoginState::ConfirmNewName:
        handle_confirm_new_name(trimmed_input);
        break;
    case LoginState::GetNewPassword:
        handle_get_new_password(trimmed_input);
        break;
    case LoginState::ConfirmPassword:
        handle_confirm_password(trimmed_input);
        break;
    case LoginState::SelectClass:
        handle_select_class(trimmed_input);
        break;
    case LoginState::SelectRace:
        handle_select_race(trimmed_input);
        break;
    case LoginState::ConfirmCreation:
        handle_confirm_creation(trimmed_input);
        break;

    // Terminal states
    case LoginState::Playing:
        // Input should be handled by game systems, not login
        Log::warn("Received input in Playing state - should be handled by game");
        break;
    case LoginState::Disconnected:
        // Already handled above
        break;
    }
}

// =============================================================================
// Account-based login handlers
// =============================================================================

void LoginSystem::handle_get_account(std::string_view input) {
    if (input.empty()) {
        send_message("Please enter your account username or email.");
        send_prompt();
        return;
    }

    // Check for user:password[:character] format for quick login
    auto colon_pos = input.find(':');
    if (colon_pos != std::string::npos) {
        std::string username = std::string(input.substr(0, colon_pos));
        std::string rest = std::string(input.substr(colon_pos + 1));
        std::string password;
        std::string quick_character;  // Optional character name for direct login

        // Check for second colon (username:password:character format)
        auto second_colon = rest.find(':');
        if (second_colon != std::string::npos) {
            password = rest.substr(0, second_colon);
            quick_character = rest.substr(second_colon + 1);
        } else {
            password = rest;
        }

        // Trim whitespace
        username = std::string(trim(username));
        password = std::string(trim(password));
        quick_character = std::string(trim(quick_character));

        if (username.empty() || password.empty()) {
            send_message("Invalid format. Use 'username:password' or just 'username'.");
            send_prompt();
            return;
        }

        account_name_ = username;

        // Try to verify user with password
        auto verify_result = verify_user(username, password);
        if (!verify_result || !verify_result.value()) {
            // User account verification failed - try legacy character login
            auto char_exists = character_exists(username);
            if (char_exists && char_exists.value()) {
                // It's a legacy character - try to load with password
                auto load_result = load_character(username, password);
                if (load_result) {
                    // Successfully loaded legacy character
                    player_ = load_result.value();

                    // Check for existing connection and handle reconnection
                    if (auto *network_manager = connection_->get_network_manager()) {
                        std::string player_name = std::string(player_->name());
                        if (network_manager->handle_reconnection(connection_, player_name)) {
                            player_.reset();                    // Clear freshly-loaded player, using transferred one
                            transition_to(LoginState::Playing); // Must transition so buffered commands are forwarded
                            Log::info("Player '{}' reconnected successfully", player_name);
                            return;
                        }
                    }

                    transition_to(LoginState::Playing);
                    send_message(fmt::format("Welcome to FieryMUD, {}!", player_->name()));

                    if (player_loaded_callback_) {
                        player_loaded_callback_(player_);
                    }

                    Log::info("Player '{}' logged in via quick login (legacy character)", player_->name());
                    return;
                }
                // Password was wrong for legacy character
            }

            // Neither account nor legacy character login worked
            login_attempts_++;
            if (login_attempts_ >= MAX_LOGIN_ATTEMPTS) {
                disconnect_with_message("Too many failed login attempts. Goodbye!");
                return;
            }
            send_message("Invalid username or password. Please try again.");
            send_prompt();
            return;
        }

        // User verified - load characters and show selection
        load_user_characters();
        if (user_characters_.empty()) {
            // No characters - go to character creation
            send_message(fmt::format("Welcome, {}! You have no characters.", account_name_));
            send_message("Let's create your first character.");
            transition_to(LoginState::GetName);
            send_message("Please enter a name for your new character:");
            send_prompt();
        } else if (!quick_character.empty()) {
            // Character specified in quick login - try to select it directly
            bool found = false;
            std::string lower_quick = to_lowercase(quick_character);
            for (size_t i = 0; i < user_characters_.size(); ++i) {
                std::string char_id = std::get<0>(user_characters_[i]);
                std::string char_name = std::get<1>(user_characters_[i]);
                std::string lower_char_name = to_lowercase(char_name);

                if (lower_char_name == lower_quick || lower_char_name.starts_with(lower_quick)) {
                    // Found the character - load it via connection pool
                    auto load_result = ConnectionPool::instance().execute(
                        [&char_id, &char_name, this](pqxx::work &txn) -> Result<std::shared_ptr<Player>> {
                            auto char_result = WorldQueries::load_character_by_id(txn, char_id);
                            if (!char_result) {
                                return std::unexpected(char_result.error());
                            }

                            auto player_result = PlayerQueries::load_player_by_name(txn, char_name);
                            if (!player_result) {
                                return std::unexpected(player_result.error());
                            }

                            return std::move(*player_result);
                        });

                    if (load_result && *load_result) {
                        player_ = *load_result;

                        // Link player to their account
                        if (!user_id_.empty()) {
                            player_->set_account(user_id_);
                        }

                        // Load character abilities from database
                        load_player_abilities(player_);

                        // Load saved effects from database
                        load_player_effects(player_);

                        // Check for existing connection and handle reconnection
                        if (auto *network_manager = connection_->get_network_manager()) {
                            std::string player_name = std::string(player_->name());
                            if (network_manager->handle_reconnection(connection_, player_name)) {
                                player_.reset();
                                transition_to(LoginState::Playing);
                                Log::info("Player '{}' reconnected via quick login", player_name);
                                return;
                            }
                        }

                        transition_to(LoginState::Playing);
                        send_message(fmt::format("Welcome to FieryMUD, {}!", player_->name()));

                        if (player_loaded_callback_) {
                            player_loaded_callback_(player_);
                        }

                        Log::info("Player '{}' logged in via quick login (account:password:character)", player_->name());
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                send_message(fmt::format("Character '{}' not found on this account.", quick_character));
                transition_to(LoginState::SelectCharacter);
                send_character_menu();
                send_prompt();
            }
        } else {
            transition_to(LoginState::SelectCharacter);
            send_character_menu();
            send_prompt();
        }
        return;
    }

    // Just username/email provided - check if exists
    account_name_ = std::string(input);

    auto exists_result = user_exists(account_name_);
    if (!exists_result) {
        send_message("Error checking account database. Please try again.");
        send_prompt();
        return;
    }

    if (!exists_result.value()) {
        // Account doesn't exist - check if it's a legacy character name
        auto char_exists = character_exists(input);
        if (char_exists && char_exists.value()) {
            // It's a legacy character - switch to legacy login flow
            send_message(fmt::format("Character '{}' found. Using legacy login.", input));
            creation_data_.name = normalize_name(input);
            transition_to(LoginState::GetPassword);
            send_prompt();
            return;
        }

        // Neither account nor character exists
        send_message(fmt::format("Account '{}' not found.", account_name_));
        send_message("Please register at https://fierymud.org or enter a character name for legacy login.");
        send_prompt();
        return;
    }

    // Account exists - ask for password
    transition_to(LoginState::GetAccountPassword);
    send_message(fmt::format("Welcome back, {}!", account_name_));
    send_prompt();
}

void LoginSystem::handle_get_account_password(std::string_view input) {
    if (input.empty()) {
        send_message("Please enter your password.");
        send_prompt();
        return;
    }

    auto verify_result = verify_user(account_name_, input);
    if (!verify_result) {
        send_message("Error verifying password. Please try again.");
        send_prompt();
        return;
    }

    if (!verify_result.value()) {
        login_attempts_++;
        if (login_attempts_ >= MAX_LOGIN_ATTEMPTS) {
            disconnect_with_message("Too many failed login attempts. Goodbye!");
            return;
        }
        send_message("Invalid password. Please try again.");
        send_prompt();
        return;
    }

    // Password verified - load characters
    load_user_characters();

    if (user_characters_.empty()) {
        // No characters yet - go to character creation
        send_message("You have no characters yet. Let's create one!");
        transition_to(LoginState::GetName);
        send_message("Please enter a name for your new character:");
        send_prompt();
    } else {
        transition_to(LoginState::SelectCharacter);
        send_character_menu();
        send_prompt();
    }
}

void LoginSystem::handle_select_character(std::string_view input) {
    if (input.empty()) {
        send_message("Please select a character or 'new' to create one.");
        send_prompt();
        return;
    }

    std::string lower_input = to_lowercase(input);

    // Check for 'new' to create a new character
    if (lower_input == "new" || lower_input == "n") {
        transition_to(LoginState::GetName);
        send_message("Please enter a name for your new character:");
        send_prompt();
        return;
    }

    // Try to parse as a number
    int choice = 0;
    try {
        choice = std::stoi(std::string(input));
    } catch (...) {
        // Try to match by name
        for (size_t i = 0; i < user_characters_.size(); ++i) {
            std::string char_name = std::get<1>(user_characters_[i]);
            std::string lower_char_name = to_lowercase(char_name);
            if (lower_char_name == lower_input || lower_char_name.starts_with(lower_input)) {
                choice = static_cast<int>(i + 1);
                break;
            }
        }

        if (choice == 0) {
            send_message("Invalid choice. Enter a number, character name, or 'new'.");
            send_prompt();
            return;
        }
    }

    if (choice < 1 || choice > static_cast<int>(user_characters_.size())) {
        send_message(fmt::format("Invalid choice. Select 1-{} or 'new'.", user_characters_.size()));
        send_prompt();
        return;
    }

    // Load the selected character
    const auto &selected = user_characters_[choice - 1];
    std::string char_id = std::get<0>(selected);
    std::string char_name = std::get<1>(selected);

    Log::debug("Loading character '{}' (id: {}) for user {}", char_name, char_id, user_id_);

    // Load character by ID (without password verification since user is already authenticated)
    auto result = ConnectionPool::instance().execute(
        [&char_id, &char_name, this](pqxx::work &txn) -> Result<std::shared_ptr<Player>> {
            auto char_result = WorldQueries::load_character_by_id(txn, char_id);
            if (!char_result) {
                return std::unexpected(char_result.error());
            }

            const auto &char_data = char_result.value();

            // Create Player from database data
            std::size_t hash = std::hash<std::string>{}(char_data.id);
            EntityId player_id(static_cast<int>(hash & 0x7FFFFFFF));

            auto player_result = Player::create(player_id, char_data.name);
            if (!player_result) {
                return std::unexpected(player_result.error());
            }

            auto player = std::shared_ptr<Player>(player_result.value().release());

            // Set character data from database
            player->set_level(char_data.level);
            player->set_class(char_data.player_class);
            player->set_race(char_data.race_type);
            player->set_gender(char_data.gender);

            // Set god level for immortals (level 100+)
            // god_level = level - 99, so level 100 = god_level 1, level 105 = god_level 6
            if (char_data.level >= kImmortalLevel) {
                player->set_god_level(char_data.level - (kImmortalLevel - 1));
            }

            // Set stats
            auto &stats = player->stats();
            stats.strength = char_data.strength;
            stats.intelligence = char_data.intelligence;
            stats.wisdom = char_data.wisdom;
            stats.dexterity = char_data.dexterity;
            stats.constitution = char_data.constitution;
            stats.charisma = char_data.charisma;

            // Set vitals
            stats.hit_points = char_data.hit_points;
            stats.max_hit_points = char_data.hit_points_max;
            stats.stamina = char_data.stamina;
            stats.max_stamina = char_data.stamina_max;

            // Convert legacy DB combat stats to new system
            stats.accuracy = char_data.hit_roll;
            stats.attack_power = char_data.damage_roll;
            stats.armor_rating = std::max(0, 100 - char_data.armor_class);

            // Set experience
            stats.experience = char_data.experience;

            // Set player preferences
            player->set_prompt(char_data.prompt);

            // Store the database character ID
            player->set_database_id(char_data.id);

            // Update last login and online status
            WorldQueries::update_last_login(txn, char_data.id);
            WorldQueries::set_character_online(txn, char_data.id, true);

            // Also update user's last login
            if (!user_id_.empty()) {
                WorldQueries::update_user_last_login(txn, user_id_);
            }

            return player;
        });

    if (!result) {
        send_message(fmt::format("Error loading character '{}'. Please try again.", char_name));
        send_prompt();
        return;
    }

    player_ = *result;

    // Link player to their account
    if (!user_id_.empty()) {
        player_->set_account(user_id_);
    }

    // Load character abilities from database
    load_player_abilities(player_);

    // Load saved effects from database
    load_player_effects(player_);

    // Check for existing connection and handle reconnection
    if (auto *network_manager = connection_->get_network_manager()) {
        std::string player_name = std::string(player_->name());
        if (network_manager->handle_reconnection(connection_, player_name)) {
            player_.reset();                    // Clear freshly-loaded player, using transferred one
            transition_to(LoginState::Playing); // Must transition so buffered commands are forwarded
            Log::info("Player '{}' reconnected successfully", player_name);
            return;
        }
    }

    // Use player's saved start room if valid, otherwise world default
    // The saved room was already set during load_character()
    if (!player_->start_room().is_valid()) {
        auto world_start_room = WorldManager::instance().get_start_room();
        if (world_start_room.is_valid()) {
            player_->set_start_room(world_start_room);
        }
    }

    transition_to(LoginState::Playing);
    send_message(fmt::format("Welcome to FieryMUD, {}!", player_->name()));

    if (player_loaded_callback_) {
        player_loaded_callback_(player_);
    }

    Log::info("Player '{}' logged in via account '{}'", player_->name(), account_name_);
}

// =============================================================================
// Legacy character-based login handlers
// =============================================================================

void LoginSystem::handle_get_name(std::string_view input) {
    if (input.empty()) {
        send_message("Please enter a character name.");
        send_prompt();
        return;
    }

    // Check if input contains colon (user:password format)
    auto colon_pos = input.find(':');
    if (colon_pos != std::string::npos) {
        // Parse user:password format
        std::string name_part = std::string(input.substr(0, colon_pos));
        std::string password_part = std::string(input.substr(colon_pos + 1));

        // Trim whitespace from both parts
        name_part = std::string(trim(name_part));
        password_part = std::string(trim(password_part));

        if (name_part.empty() || password_part.empty()) {
            send_message("Invalid format. Use 'name:password' or just 'name'.");
            send_prompt();
            return;
        }

        std::string name = normalize_name(name_part);

        if (!is_valid_name(name)) {
            send_message("Invalid name. Names must be 3-20 characters, letters only, and start with a capital letter.");
            send_prompt();
            return;
        }

        creation_data_.name = name;

        // Check if character exists
        auto exists_result = character_exists(name);
        if (!exists_result) {
            send_message("Error checking character database. Please try again.");
            send_prompt();
            return;
        }

        if (exists_result.value()) {
            // Existing character - try to login directly with provided password
            auto load_result = load_character(name, password_part);
            if (!load_result) {
                login_attempts_++;
                if (login_attempts_ >= MAX_LOGIN_ATTEMPTS) {
                    disconnect_with_message("Too many failed login attempts. Goodbye!");
                    return;
                }

                send_message("Invalid password. Please try again with 'name:password' or just the name.");
                send_prompt();
                return;
            }

            // Successfully loaded character
            player_ = load_result.value();

            // Check for existing connection and handle reconnection
            if (auto *network_manager = connection_->get_network_manager()) {
                std::string player_name = std::string(player_->name());
                if (network_manager->handle_reconnection(connection_, player_name)) {
                    player_.reset();                    // Clear freshly-loaded player, using transferred one
                    transition_to(LoginState::Playing); // Must transition so buffered commands are forwarded
                    Log::info("Player '{}' reconnected successfully", player_name);
                    return;
                }
            }

            transition_to(LoginState::Playing);
            send_message(fmt::format("Welcome to FieryMUD, {}!", player_->name()));

            if (player_loaded_callback_) {
                player_loaded_callback_(player_);
            }

            Log::info("Player '{}' logged in successfully via name:password format", player_->name());
            return;
        } else {
            send_message(
                fmt::format("Character '{}' does not exist. Create it first by entering just the name.", name));
            send_prompt();
            return;
        }
    }

    std::string name = normalize_name(input);

    if (!is_valid_name(name)) {
        send_message("Invalid name. Names must be 3-20 characters, letters only, and start with a capital letter.");
        send_prompt();
        return;
    }

    creation_data_.name = name;

    // Check if character exists
    auto exists_result = character_exists(name);
    if (!exists_result) {
        send_message("Error checking character database. Please try again.");
        send_prompt();
        return;
    }

    if (exists_result.value()) {
        // Existing character - ask for password
        transition_to(LoginState::GetPassword);
        send_message(fmt::format("Welcome back, {}!", name));
        send_prompt();
    } else {
        // New character - confirm creation
        transition_to(LoginState::ConfirmNewName);
        send_message(fmt::format("'{}' is a new character name.", name));
        send_message("Are you sure you want to create this character? (yes/no)");
        send_prompt();
    }
}

void LoginSystem::handle_get_password(std::string_view input) {
    if (input.empty()) {
        send_message("Please enter your password.");
        send_prompt();
        return;
    }

    auto load_result = load_character(creation_data_.name, input);
    if (!load_result) {
        login_attempts_++;
        if (login_attempts_ >= MAX_LOGIN_ATTEMPTS) {
            disconnect_with_message("Too many failed login attempts. Goodbye!");
            return;
        }

        send_message("Invalid password. Please try again.");
        send_prompt();
        return;
    }

    // Successfully loaded character
    player_ = load_result.value();

    // Check for existing connection and handle reconnection
    if (auto *network_manager = connection_->get_network_manager()) {
        std::string player_name = std::string(player_->name()); // Copy name before potential reset
        if (network_manager->handle_reconnection(connection_, player_name)) {
            // Existing connection was found and player transferred to this connection
            // The reconnection handler already sent appropriate messages
            // Clear our freshly-loaded player since we're using the transferred player instead
            player_.reset();
            transition_to(LoginState::Playing); // Must transition so buffered commands are forwarded
            Log::info("Player '{}' reconnected successfully", player_name);
            return; // Early return since reconnection handled everything
        }
    }

    transition_to(LoginState::Playing);

    send_message(fmt::format("Welcome to FieryMUD, {}!", player_->name()));

    if (player_loaded_callback_) {
        player_loaded_callback_(player_);
    }

    Log::info("Player '{}' logged in successfully", player_->name());
}

void LoginSystem::handle_confirm_new_name(std::string_view input) {
    std::string lower_input = to_lowercase(input);

    if (lower_input == "yes" || lower_input == "y") {
        transition_to(LoginState::GetNewPassword);
        send_message("Please choose a password (6-20 characters):");
        send_prompt();
    } else if (lower_input == "no" || lower_input == "n") {
        creation_data_ = CharacterCreationData{}; // Reset
        transition_to(LoginState::GetName);
        send_message("Please enter a different character name:");
        send_prompt();
    } else {
        send_message("Please answer yes or no.");
        send_prompt();
    }
}

void LoginSystem::handle_get_new_password(std::string_view input) {
    if (!is_valid_password(input)) {
        send_message("Invalid password. Must be 6-20 characters with no spaces.");
        send_prompt();
        return;
    }

    creation_data_.password = std::string(input);
    transition_to(LoginState::ConfirmPassword);
    send_message("Please confirm your password:");
    send_prompt();
}

void LoginSystem::handle_confirm_password(std::string_view input) {
    if (std::string(input) != creation_data_.password) {
        send_message("Passwords don't match. Please try again.");
        transition_to(LoginState::GetNewPassword);
        send_message("Please choose a password (6-20 characters):");
        send_prompt();
        return;
    }

    transition_to(LoginState::SelectClass);
    send_class_menu();
    send_prompt();
}

void LoginSystem::handle_select_class(std::string_view input) {
    int choice = 0;
    try {
        choice = std::stoi(std::string(input));
    } catch (...) {
        send_message("Please enter a number (1-4).");
        send_prompt();
        return;
    }

    if (choice < 1 || choice > 4) {
        send_message("Invalid choice. Please select 1-4.");
        send_prompt();
        return;
    }

    creation_data_.character_class = static_cast<CharacterClass>(choice);
    transition_to(LoginState::SelectRace);
    send_race_menu();
    send_prompt();
}

void LoginSystem::handle_select_race(std::string_view input) {
    int choice = 0;
    try {
        choice = std::stoi(std::string(input));
    } catch (...) {
        send_message("Please enter a number (1-4).");
        send_prompt();
        return;
    }

    if (choice < 1 || choice > 4) {
        send_message("Invalid choice. Please select 1-4.");
        send_prompt();
        return;
    }

    creation_data_.race = static_cast<CharacterRace>(choice);
    transition_to(LoginState::ConfirmCreation);
    send_creation_summary();
    send_message("Is this correct? (yes/no)");
    send_prompt();
}

void LoginSystem::handle_confirm_creation(std::string_view input) {
    std::string lower_input = to_lowercase(input);

    if (lower_input == "yes" || lower_input == "y") {
        auto create_result = create_character();
        if (!create_result) {
            send_message("Error creating character. Please try again later.");
            disconnect_with_message("Character creation failed.");
            return;
        }

        player_ = create_result.value();

        // Link player to their account
        if (!user_id_.empty()) {
            player_->set_account(user_id_);
        }

        auto save_result = save_character(player_);
        if (!save_result) {
            Log::error("Failed to save new character '{}': {}", player_->name(), save_result.error().message);
            send_message("Warning: Character created but save failed. Contact an administrator.");
        }

        transition_to(LoginState::Playing);
        send_message(fmt::format("Welcome to FieryMUD, {}! Your character has been created.", player_->name()));
        send_message("Type 'help' for basic commands or 'newbie' for new player guidance.");

        if (player_loaded_callback_) {
            player_loaded_callback_(player_);
        }

        Log::info("New character '{}' created and logged in", player_->name());
    } else if (lower_input == "no" || lower_input == "n") {
        transition_to(LoginState::SelectClass);
        send_message("Let's start over with character creation.");
        send_class_menu();
        send_prompt();
    } else {
        send_message("Please answer yes or no.");
        send_prompt();
    }
}

// Helper methods
void LoginSystem::transition_to(LoginState new_state) {
    Log::debug("Login state transition: {} -> {}", static_cast<int>(state_), static_cast<int>(new_state));
    state_ = new_state;
}

void LoginSystem::send_prompt() {
    std::string prompt_text;

    switch (state_) {
    // Account-based login states
    case LoginState::GetAccount:
        prompt_text = "Enter account username or email: ";
        break;
    case LoginState::GetAccountPassword:
        prompt_text = "Account password: ";
        break;
    case LoginState::SelectCharacter:
        prompt_text = "Select character: ";
        break;

    // Legacy/character creation states
    case LoginState::GetName:
        prompt_text = "Enter character name: ";
        break;
    case LoginState::GetPassword:
        prompt_text = "Password: ";
        break;
    case LoginState::ConfirmNewName:
        prompt_text = "Create this character? ";
        break;
    case LoginState::GetNewPassword:
        prompt_text = "Choose password: ";
        break;
    case LoginState::ConfirmPassword:
        prompt_text = "Confirm password: ";
        break;
    case LoginState::SelectClass:
        prompt_text = "Select class: ";
        break;
    case LoginState::SelectRace:
        prompt_text = "Select race: ";
        break;
    case LoginState::ConfirmCreation:
        prompt_text = "Confirm creation? ";
        break;
    default:
        break;
    }

    if (connection_) {
        connection_->send_prompt(prompt_text);
    }
}

void LoginSystem::send_message(std::string_view message) {
    if (connection_) {
        // PlayerConnection::send_message() handles \r\n and color processing
        connection_->send_message(message);
    }
}

void LoginSystem::send_welcome_message() {
    // Select logo based on terminal capabilities detected via GMCP/MTTS/NEW-ENVIRON.
    // The connection delays login by 200ms to allow capability negotiation, so
    // by the time we get here, GMCP clients should have reported their capabilities.
    using TerminalCapabilities::DetectionMethod;
    using TerminalCapabilities::SupportLevel;

    auto level = SupportLevel::Full; // Default to full for modern MUD clients
    if (connection_) {
        auto caps = connection_->get_terminal_capabilities();

        // Use detected level if we have actual client detection
        // Environment detection means "basic telnet client" - still default to Full
        // since the 200ms delay should have allowed GMCP detection if supported
        if (caps.detection_method == DetectionMethod::GMCP || caps.detection_method == DetectionMethod::MTTS ||
            caps.detection_method == DetectionMethod::NewEnviron) {
            level = caps.overall_level;
            Log::info("Logo selection: Using detected level {} from {} (client: {} {})", static_cast<int>(level),
                      caps.detection_method == DetectionMethod::GMCP   ? "GMCP"
                      : caps.detection_method == DetectionMethod::MTTS ? "MTTS"
                                                                       : "NEW-ENVIRON",
                      caps.client_name, caps.client_version);
        } else {
            Log::info("Logo selection: No client detection, defaulting to Full");
        }
    }

    switch (level) {
    case SupportLevel::Basic:
        send_logo_basic();
        break;
    case SupportLevel::Standard:
        send_logo_extended();
        break;
    default:
        // Full, Extended, or undetected - use full logo
        send_logo_full();
        break;
    }

    // Common footer with login instructions
    send_login_instructions();
}

void LoginSystem::send_logo_full() {
    // Dragon perched on FIERYMUD text - 256-color
    // Dragon design inspired by Jeff Ferris ASCII art
    send_message("");

    send_message(
        "<c196>  ███████╗<c202>██╗<c208>███████╗<c214>██████╗ <c220>██╗   ██╗<c214>███╗   ███╗<c208>██╗   "
        "██╗<c202>██████╗");
    send_message(
        "<c196>  ██╔════╝<c202>██║<c208>██╔════╝<c214>██╔══██╗<c220>╚██╗ ██╔╝<c214>████╗ ████║<c208>██║   "
        "██║<c202>██╔══██╗");
    send_message(
        "<c196>  █████╗  <c202>██║<c208>█████╗  <c214>██████╔╝<c220> ╚████╔╝ <c214>██╔████╔██║<c208>██║   ██║<c202>██║ "
        " ██║");
    send_message(
        "<c196>  ██╔══╝  <c202>██║<c208>██╔══╝  <c214>██╔══██╗<c220>  ╚██╔╝  <c214>██║╚██╔╝██║<c208>██║   ██║<c202>██║ "
        " ██║");
    send_message(
        "<c196>  ██║     <c202>██║<c208>███████╗<c214>██║  ██║<c220>   ██║   <c214>██║ ╚═╝ "
        "██║<c208>╚██████╔╝<c202>██████╔╝");
    send_message(
        "<c196>  ╚═╝     <c202>╚═╝<c208>╚══════╝<c214>╚═╝  ╚═╝<c220>   ╚═╝   <c214>╚═╝     ╚═╝<c208> ╚═════╝ "
        "<c202>╚═════╝");
    send_message("");
}

void LoginSystem::send_logo_extended() {
    // Simplified logo with 256-color but less complex dragon
    send_message("");

    // Simple flame header
    send_message("<c196>            )  (  (     <c202>)<c208>)<c214>)</c214>");
    send_message("<c202>           (   ) )<c208>)   <c214>(<c220>(<c214>(</c214>");
    send_message("<c208>            )_(_<c214>)_<c220>(<c214>_<c208>)<c202>_</c202>");
    send_message("");

    // "FIERY" text with fire gradient
    send_message("<c196>       ███████╗<c202>██╗<c208>███████╗<c214>██████╗ <c220>██╗   ██╗</c220>");
    send_message("<c196>       ██╔════╝<c202>██║<c208>██╔════╝<c214>██╔══██╗<c220>╚██╗ ██╔╝</c220>");
    send_message("<c196>       █████╗  <c202>██║<c208>█████╗  <c214>██████╔╝<c220> ╚████╔╝</c220>");
    send_message("<c196>       ██╔══╝  <c202>██║<c208>██╔══╝  <c214>██╔══██╗<c220>  ╚██╔╝</c220>");
    send_message("<c196>       ██║     <c202>██║<c208>███████╗<c214>██║  ██║<c220>   ██║</c220>");
    send_message("<c196>       ╚═╝     <c202>╚═╝<c208>╚══════╝<c214>╚═╝  ╚═╝<c220>   ╚═╝</c220>");
    send_message("");

    // "MUD" text
    send_message("<c208>            ███╗   ███╗<c214>██╗   ██╗<c220>██████╗</c220>");
    send_message("<c208>            ████╗ ████║<c214>██║   ██║<c220>██╔══██╗</c220>");
    send_message("<c208>            ██╔████╔██║<c214>██║   ██║<c220>██║  ██║</c220>");
    send_message("<c208>            ██║╚██╔╝██║<c214>██║   ██║<c220>██║  ██║</c220>");
    send_message("<c208>            ██║ ╚═╝ ██║<c214>╚██████╔╝<c220>██████╔╝</c220>");
    send_message("<c208>            ╚═╝     ╚═╝<c214> ╚═════╝ <c220>╚═════╝</c220>");
    send_message("");
}

void LoginSystem::send_logo_basic() {
    // ASCII-only fallback for basic terminals
    send_message("");
    send_message("            )  (  (       )  )");
    send_message("           (   ) ))     ( ( (");
    send_message("            )_(_)_(     _)_)_)");
    send_message("       .'~'`~.'~'`~.'~'`~.'~'`~.'");
    send_message("");
    send_message("       FFFFF III EEEEE RRRR  Y   Y");
    send_message("       F      I  E     R   R  Y Y");
    send_message("       FFFF   I  EEEE  RRRR    Y");
    send_message("       F      I  E     R  R    Y");
    send_message("       F     III EEEEE R   R   Y");
    send_message("");
    send_message("             M   M U   U DDDD");
    send_message("             MM MM U   U D   D");
    send_message("             M M M U   U D   D");
    send_message("             M   M  UUU  DDDD");
    send_message("");
}

void LoginSystem::send_login_instructions() {
    send_message("<dim>━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━</dim>");
    send_message("<c245>         A classic fantasy MUD, forged in fire.</c245>");
    send_message("<c245>                  www.fierymud.org</c245>");
    send_message("<dim>━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━</dim>");
    send_message("");
    send_message("Login with your account username, email, or character name.");
    send_message("Quick login: <b>username:password[:character]</b>, or <b>character:password</b>");
    send_message("");
}

void LoginSystem::send_class_menu() {
    send_message("");
    send_message("Choose your character class:");
    send_message("1) Warrior  - Masters of combat and rage");
    send_message("2) Cleric   - Divine magic and healing");
    send_message("3) Sorcerer - Arcane magic and spell mastery");
    send_message("4) Rogue    - Stealth, speed, and cunning");
    send_message("");
}

void LoginSystem::send_race_menu() {
    send_message("");
    send_message("Choose your character race:");
    send_message("1) Human    - Versatile and adaptable");
    send_message("2) Elf      - Graceful and magical");
    send_message("3) Dwarf    - Sturdy and resilient");
    send_message("4) Halfling - Small but brave");
    send_message("");
}

void LoginSystem::send_creation_summary() {
    send_message("");
    send_message("=== Character Summary ===");
    send_message(fmt::format("Name:  {}", creation_data_.name));
    send_message(fmt::format("Class: {}", creation_data_.get_class_name()));
    send_message(fmt::format("Race:  {}", creation_data_.get_race_name()));
    send_message("");
}

void LoginSystem::send_character_menu() {
    send_message("");
    send_message(fmt::format("=== Characters for {} ===", account_name_));
    send_message("");

    if (user_characters_.empty()) {
        send_message("No characters found.");
    } else {
        for (size_t i = 0; i < user_characters_.size(); ++i) {
            const auto &[id, name, level, char_class] = user_characters_[i];
            send_message(fmt::format("  {}) {} - Level {} {}", i + 1, name, level, char_class));
        }
    }

    send_message("");
    send_message("Enter a number to select, character name, or 'new' to create.");
    send_message("");
}

// =============================================================================
// User/Account management helpers
// =============================================================================

Result<bool> LoginSystem::user_exists(std::string_view username_or_email) {
    auto result = ConnectionPool::instance().execute([&username_or_email](pqxx::work &txn) {
        return WorldQueries::user_exists(txn, std::string(username_or_email));
    });

    if (!result) {
        Log::error("Failed to check user existence: {}", result.error().message);
        return std::unexpected(result.error());
    }
    return *result;
}

Result<bool> LoginSystem::verify_user(std::string_view username_or_email, std::string_view password) {
    auto result =
        ConnectionPool::instance().execute([&username_or_email, &password, this](pqxx::work &txn) -> Result<bool> {
            // First, try to load the user to get their ID
            auto user_result = WorldQueries::load_user_by_username(txn, std::string(username_or_email));
            if (!user_result) {
                // Try by email
                user_result = WorldQueries::load_user_by_email(txn, std::string(username_or_email));
            }

            if (!user_result) {
                return false; // User not found
            }

            const auto &user_data = user_result.value();

            // Check if account is locked
            if (user_data.locked_until) {
                auto now = std::chrono::system_clock::now();
                if (user_data.locked_until.value() > now) {
                    Log::warn("User '{}' account is locked until {}", username_or_email,
                              std::chrono::system_clock::to_time_t(user_data.locked_until.value()));
                    return std::unexpected(Error{ErrorCode::PermissionDenied, "Account is temporarily locked"});
                }
            }

            // Verify password
            auto verify_result =
                WorldQueries::verify_user_password(txn, std::string(username_or_email), std::string(password));
            if (!verify_result || !verify_result.value()) {
                // Increment failed login attempts
                WorldQueries::increment_failed_login(txn, user_data.id);

                // Lock account after too many attempts
                if (user_data.failed_login_attempts + 1 >= 5) {
                    auto lock_until = std::chrono::system_clock::now() + std::chrono::minutes(15);
                    WorldQueries::lock_user_account(txn, user_data.id, lock_until);
                    Log::warn("User '{}' locked for 15 minutes after {} failed attempts", username_or_email,
                              user_data.failed_login_attempts + 1);
                }

                return false;
            }

            // Password verified - store user ID and update last login
            user_id_ = user_data.id;
            WorldQueries::update_user_last_login(txn, user_data.id);

            return true;
        });

    if (!result) {
        Log::error("Failed to verify user: {}", result.error().message);
        return std::unexpected(result.error());
    }
    return *result;
}

void LoginSystem::load_user_characters() {
    user_characters_.clear();

    if (user_id_.empty()) {
        Log::warn("Cannot load characters - no user ID set");
        return;
    }

    auto result = ConnectionPool::instance().execute(
        [this](pqxx::work &txn) -> Result<std::vector<WorldQueries::CharacterSummary>> {
            return WorldQueries::get_user_characters(txn, user_id_);
        });

    if (!result) {
        Log::error("Failed to load characters for user {}: {}", user_id_, result.error().message);
        return;
    }

    const auto &characters = result.value();
    for (const auto &char_summary : characters) {
        user_characters_.emplace_back(char_summary.id, char_summary.name, char_summary.level,
                                      char_summary.player_class);
    }

    Log::debug("Loaded {} characters for user {}", user_characters_.size(), user_id_);
}

void LoginSystem::disconnect_with_message(std::string_view message) {
    send_message(message);
    transition_to(LoginState::Disconnected);
    if (connection_) {
        connection_->disconnect(message);
    }
}

// Character management - Database-backed implementation
Result<bool> LoginSystem::character_exists(std::string_view name) {
    auto result = ConnectionPool::instance().execute(
        [&name](pqxx::work &txn) { return WorldQueries::character_exists(txn, std::string(name)); });

    if (!result) {
        Log::error("Failed to check character existence: {}", result.error().message);
        return std::unexpected(result.error());
    }
    return *result;
}

Result<std::shared_ptr<Player>> LoginSystem::load_character(std::string_view name, std::string_view password) {
    // Database-backed character loading
    auto result =
        ConnectionPool::instance().execute([&name, &password](pqxx::work &txn) -> Result<std::shared_ptr<Player>> {
            // Verify password
            auto verify_result = WorldQueries::verify_character_password(txn, std::string(name), std::string(password));
            if (!verify_result) {
                return std::unexpected(verify_result.error());
            }
            if (!verify_result.value()) {
                return std::unexpected(Error{ErrorCode::PermissionDenied, "Invalid password"});
            }

            // Load character data
            auto char_result = WorldQueries::load_character_by_name(txn, std::string(name));
            if (!char_result) {
                return std::unexpected(char_result.error());
            }

            const auto &char_data = char_result.value();

            // Create Player from database data
            // Generate an EntityId for the player from the database UUID
            // We'll use a hash of the UUID to get a numeric ID
            std::size_t hash = std::hash<std::string>{}(char_data.id);
            EntityId player_id(static_cast<int>(hash & 0x7FFFFFFF));

            auto player_result = Player::create(player_id, char_data.name);
            if (!player_result) {
                return std::unexpected(player_result.error());
            }

            auto player = std::shared_ptr<Player>(player_result.value().release());

            // Set character data from database
            player->set_level(char_data.level);
            player->set_class(char_data.player_class);
            player->set_race(char_data.race_type);
            player->set_gender(char_data.gender);

            // Set god level for immortals (level 100+)
            if (char_data.level >= kImmortalLevel) {
                player->set_god_level(char_data.level - (kImmortalLevel - 1));
            }

            // Set stats
            auto &stats = player->stats();
            stats.strength = char_data.strength;
            stats.intelligence = char_data.intelligence;
            stats.wisdom = char_data.wisdom;
            stats.dexterity = char_data.dexterity;
            stats.constitution = char_data.constitution;
            stats.charisma = char_data.charisma;

            // Set vitals
            stats.hit_points = char_data.hit_points;
            stats.max_hit_points = char_data.hit_points_max;
            stats.stamina = char_data.stamina;
            stats.max_stamina = char_data.stamina_max;

            // Convert legacy DB combat stats to new system
            stats.accuracy = char_data.hit_roll;
            stats.attack_power = char_data.damage_roll;
            stats.armor_rating = std::max(0, 100 - char_data.armor_class);

            // Set experience
            stats.experience = char_data.experience;

            // Set position from database (persists ghost state, etc.)
            if (auto pos = ActorUtils::parse_position(char_data.position)) {
                player->set_position(*pos);
                Log::debug("Player {} position restored to {}", char_data.name, char_data.position);
            }

            // Store the database character ID for saving later
            player->set_database_id(char_data.id);

            // Load player preferences
            player->set_title(char_data.title);
            player->set_description(char_data.description);
            player->set_prompt(char_data.prompt);
            player->set_wimpy_threshold(char_data.wimpy_threshold);
            player->set_player_flags_from_strings(char_data.player_flags);

            // Load currency (stored as copper)
            player->set_wallet(fiery::Money(char_data.wealth));
            player->set_bank(fiery::Money(char_data.bank_wealth));
            player->set_account_bank(fiery::Money(char_data.account_wealth));

            // Link to user account for persistence
            if (char_data.user_id) {
                player->set_user_id(*char_data.user_id);
            }

            // Set start room (login location) from saved current_room
            // Room locations are stored as composite keys (zone_id, room_id)
            if (char_data.current_room_zone_id && char_data.current_room_id) {
                player->set_start_room(EntityId(static_cast<std::uint32_t>(*char_data.current_room_zone_id),
                                                static_cast<std::uint32_t>(*char_data.current_room_id)));
                Log::debug("Player {} has current_room ({}, {}) -> start_room {}", char_data.name,
                           *char_data.current_room_zone_id, *char_data.current_room_id, player->start_room());
            }

            // Set recall room (touchstone) separately - this is where recall command takes you
            if (char_data.recall_room_zone_id && char_data.recall_room_id) {
                player->set_recall_room(EntityId(static_cast<std::uint32_t>(*char_data.recall_room_zone_id),
                                                 static_cast<std::uint32_t>(*char_data.recall_room_id)));
                Log::debug("Player {} has recall_room ({}, {}) -> recall_room {}", char_data.name,
                           *char_data.recall_room_zone_id, *char_data.recall_room_id, player->recall_room());
            }

            // Update last login and online status
            WorldQueries::update_last_login(txn, char_data.id);
            WorldQueries::set_character_online(txn, char_data.id, true);

            return player;
        });

    if (!result) {
        Log::error("Failed to load character '{}': {}", name, result.error().message);
        return std::unexpected(result.error());
    }

    auto player = *result;

    // Load character abilities from database
    load_player_abilities(player);

    // Load saved effects from database
    load_player_effects(player);

    // Load character items (equipment and inventory) from database
    load_player_items(player);

    // Determine start room with priority:
    // For mortals: 1. Saved location (only if rented) -> 2. Recall -> 3. Race default -> 4. MUD default
    // For gods: Always use saved location if valid
    auto &world_manager = WorldManager::instance();
    auto &game_data = GameDataCache::instance();
    auto saved_room = player->start_room();
    bool use_saved_room = false;

    // Gods can return to their saved location regardless of how they logged out
    if (player->is_god() && saved_room.is_valid()) {
        if (world_manager.get_room(saved_room)) {
            Log::debug("God {} returning to saved room {}", player->name(), saved_room);
            use_saved_room = true;
        } else {
            Log::info("God {} saved room {} doesn't exist, trying alternatives", player->name(), saved_room);
        }
    }
    // Mortals use saved room if valid (only exists if they rented/camped - quit clears it)
    else if (saved_room.is_valid()) {
        if (world_manager.get_room(saved_room)) {
            Log::debug("Player {} returning to saved room {}", player->name(), saved_room);
            use_saved_room = true;
        } else {
            Log::info("Player {} saved room {} doesn't exist, trying alternatives", player->name(), saved_room);
        }
    }

    if (!use_saved_room) {
        // Clear the saved room so fallbacks can take over
        player->set_start_room(EntityId{});

        // Try recall room (home/touchstone/savepoint)
        auto recall_room = player->recall_room();
        if (recall_room.is_valid() && world_manager.get_room(recall_room)) {
            player->set_start_room(recall_room);
            Log::debug("Player {} using recall room (savepoint): {}", player->name(), recall_room);
        }
    }

    if (!player->start_room().is_valid()) {
        // No valid recall room - try race-specific start room from cache
        std::string race_key(player->race());
        if (auto race_data = game_data.find_race_by_key(race_key)) {
            if (race_data->has_start_room()) {
                EntityId race_room(static_cast<std::uint32_t>(*race_data->start_room_zone_id),
                                   static_cast<std::uint32_t>(*race_data->start_room_id));

                if (world_manager.get_room(race_room)) {
                    player->set_start_room(race_room);
                    Log::debug("Player {} using race '{}' start room: {}", player->name(), race_key, race_room);
                }
            }
        }
    }

    if (!player->start_room().is_valid()) {
        // Fall back to world default
        auto world_start_room = world_manager.get_start_room();
        if (world_start_room.is_valid()) {
            player->set_start_room(world_start_room);
            Log::debug("Player {} using world default start room: {}", player->name(), world_start_room);
        } else {
            Log::warn("World start room is invalid for loaded player {}", player->name());
        }
    }

    Log::info("Player '{}' loaded from database", player->name());
    return player;
}

Result<std::shared_ptr<Player>> LoginSystem::create_character() {
    if (!creation_data_.is_complete()) {
        return std::unexpected(Error{ErrorCode::InvalidState, "Character creation data incomplete"});
    }

    std::string class_name = std::string(magic_enum::enum_name(creation_data_.character_class));
    std::string race_name = std::string(magic_enum::enum_name(creation_data_.race));

    // Create character in database
    auto result = ConnectionPool::instance().execute([this, &class_name,
                                                      &race_name](pqxx::work &txn) -> Result<std::shared_ptr<Player>> {
        auto create_result =
            WorldQueries::create_character(txn, creation_data_.name, creation_data_.password, class_name, race_name);

        if (!create_result) {
            return std::unexpected(create_result.error());
        }

        const auto &char_data = create_result.value();

        // Generate EntityId from database UUID
        std::size_t hash = std::hash<std::string>{}(char_data.id);
        EntityId player_id(static_cast<int>(hash & 0x7FFFFFFF));

        auto player_result = Player::create(player_id, char_data.name);
        if (!player_result) {
            return std::unexpected(player_result.error());
        }

        auto player = std::shared_ptr<Player>(player_result.value().release());

        // Set character data from database
        player->set_database_id(char_data.id);
        player->set_level(char_data.level);
        player->set_class(char_data.player_class);
        player->set_race(char_data.race_type);
        player->set_gender(char_data.gender);

        // Set god level for immortals (level 100+)
        if (char_data.level >= kImmortalLevel) {
            player->set_god_level(char_data.level - (kImmortalLevel - 1));
        }

        // Set stats
        auto &stats = player->stats();
        stats.strength = char_data.strength;
        stats.intelligence = char_data.intelligence;
        stats.wisdom = char_data.wisdom;
        stats.dexterity = char_data.dexterity;
        stats.constitution = char_data.constitution;
        stats.charisma = char_data.charisma;

        // Set vitals
        stats.hit_points = char_data.hit_points;
        stats.max_hit_points = char_data.hit_points_max;
        stats.stamina = char_data.stamina;
        stats.max_stamina = char_data.stamina_max;

        // Mark as online
        WorldQueries::set_character_online(txn, char_data.id, true);

        return player;
    });

    if (!result) {
        Log::error("Failed to create character '{}': {}", creation_data_.name, result.error().message);
        return std::unexpected(result.error());
    }

    auto player = *result;
    auto &world_manager = WorldManager::instance();
    auto &game_data = GameDataCache::instance();

    // Determine start room with priority: raceRoom > mudDefault
    // (New characters don't have a saved room)
    std::string race_key(player->race());
    if (auto race_data = game_data.find_race_by_key(race_key)) {
        if (race_data->has_start_room()) {
            EntityId race_room(static_cast<std::uint32_t>(*race_data->start_room_zone_id),
                               static_cast<std::uint32_t>(*race_data->start_room_id));

            if (world_manager.get_room(race_room)) {
                player->set_start_room(race_room);
                Log::debug("New player {} using race '{}' start room: {}", player->name(), race_key, race_room);
            }
        }
    }

    if (!player->start_room().is_valid()) {
        // Fall back to world default
        auto world_start_room = world_manager.get_start_room();
        if (world_start_room.is_valid()) {
            player->set_start_room(world_start_room);
            Log::debug("New player {} using world default start room: {}", player->name(), world_start_room);
        } else {
            Log::warn("World start room is invalid for new player {}", player->name());
        }
    }

    Log::info("Created new character '{}' in database", player->name());
    return player;
}

Result<void> LoginSystem::save_character(std::shared_ptr<Player> player) {
    if (!player) {
        return std::unexpected(Error{ErrorCode::InvalidArgument, "Null player"});
    }

    // Check if player has a database ID
    if (player->database_id().empty()) {
        Log::warn("Cannot save character '{}' - no database ID", player->name());
        return std::unexpected(Error{ErrorCode::InvalidState, "Character has no database ID"});
    }

    // Build CharacterData from Player
    WorldQueries::CharacterData char_data;
    char_data.id = std::string(player->database_id());
    char_data.name = std::string(player->name());
    char_data.level = player->level();
    char_data.player_class = player->player_class();
    char_data.race_type = player->race();

    // Stats
    const auto &stats = player->stats();
    char_data.strength = stats.strength;
    char_data.intelligence = stats.intelligence;
    char_data.wisdom = stats.wisdom;
    char_data.dexterity = stats.dexterity;
    char_data.constitution = stats.constitution;
    char_data.charisma = stats.charisma;

    // Vitals
    char_data.hit_points = stats.hit_points;
    char_data.hit_points_max = stats.max_hit_points;
    char_data.stamina = stats.stamina;
    char_data.stamina_max = stats.max_stamina;

    // Convert new combat stats back to legacy DB format for saving
    char_data.armor_class = std::max(0, 100 - stats.armor_rating);
    char_data.hit_roll = stats.accuracy;
    char_data.damage_roll = stats.attack_power;

    // Experience
    char_data.experience = stats.experience;

    // Alignment
    char_data.alignment = stats.alignment;

    // Currency (stats.gold is stored in copper)
    char_data.wealth = stats.gold;

    // Save to database
    auto result = ConnectionPool::instance().execute(
        [&char_data](pqxx::work &txn) { return WorldQueries::save_character(txn, char_data); });

    if (!result) {
        Log::error("Failed to save character '{}': {}", player->name(), result.error().message);
        return std::unexpected(result.error());
    }

    Log::info("Saved character '{}' to database", player->name());
    return Success();
}

// Validation
bool LoginSystem::is_valid_name(std::string_view name) const {
    if (name.length() < 3 || name.length() > 20) {
        return false;
    }

    // Must start with capital letter
    if (!std::isupper(name[0])) {
        return false;
    }

    // Rest must be lowercase letters
    for (size_t i = 1; i < name.length(); ++i) {
        if (!std::islower(name[i])) {
            return false;
        }
    }

    return true;
}

bool LoginSystem::is_valid_password(std::string_view password) const {
    if (password.length() < 6 || password.length() > 20) {
        return false;
    }

    // No spaces allowed
    return password.find(' ') == std::string::npos;
}

std::string LoginSystem::normalize_name(std::string_view name) const {
    std::string result;
    result.reserve(name.length());

    // First character uppercase
    if (!name.empty()) {
        result += std::toupper(name[0]);

        // Rest lowercase
        for (size_t i = 1; i < name.length(); ++i) {
            result += std::tolower(name[i]);
        }
    }

    return result;
}

// Player file validation and migration implementations

LoginSystem::PlayerFileValidation LoginSystem::validate_player_file(std::string_view name) const {
    PlayerFileValidation validation;

    const auto &dir = Config::instance().player_directory();
    std::filesystem::path path{dir};
    path /= fmt::format("{}.json", name);
    auto filename = path.string();

    // Check if file exists
    if (!std::filesystem::exists(path)) {
        validation.error_message = "Player file does not exist";
        return validation;
    }

    // Check file permissions and size
    std::error_code ec;
    auto file_size = std::filesystem::file_size(path, ec);
    if (ec) {
        validation.error_message = fmt::format("Cannot access file: {}", ec.message());
        return validation;
    }

    if (file_size == 0) {
        validation.has_corruption = true;
        validation.error_message = "Player file is empty";
        validation.suggested_fix = "Restore from backup or recreate character";
        return validation;
    }

    if (file_size > 1024 * 1024) { // 1MB limit
        validation.has_corruption = true;
        validation.error_message = "Player file unusually large (possible corruption)";
        validation.suggested_fix = "Check file content for corruption";
        return validation;
    }

    // Validate JSON structure
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            validation.error_message = "Cannot open player file";
            return validation;
        }

        nlohmann::json data;
        file >> data;

        // Check required top-level fields
        if (!data.contains("password")) {
            validation.has_corruption = true;
            validation.error_message = "Missing password field";
            validation.suggested_fix = "File structure corruption - restore from backup";
            return validation;
        }

        if (!data.contains("player_data")) {
            validation.has_corruption = true;
            validation.error_message = "Missing player_data field";
            validation.suggested_fix = "File structure corruption - restore from backup";
            return validation;
        }

        auto player_data = data["player_data"];

        // Check critical player fields
        std::vector<std::string> required_fields = {"name", "id", "level", "class", "race", "attributes", "vitals"};

        for (const auto &field : required_fields) {
            if (!player_data.contains(field)) {
                validation.has_corruption = true;
                validation.error_message = fmt::format("Missing required field: {}", field);
                validation.suggested_fix = "Critical data missing - restore from backup";
                return validation;
            }
        }

        // Check for migration needs (version compatibility)
        if (!data.contains("created")) {
            validation.needs_migration = true;
            validation.error_message = "Legacy format detected";
            validation.suggested_fix = "File will be migrated on load";
        }

        // Validate data integrity
        if (player_data["name"] != name) {
            validation.has_corruption = true;
            validation.error_message = "Name mismatch in player data";
            validation.suggested_fix = "File name and data inconsistent";
            return validation;
        }

        validation.is_valid = true;

    } catch (const nlohmann::json::exception &e) {
        validation.has_corruption = true;
        validation.error_message = fmt::format("JSON parsing error: {}", e.what());
        validation.suggested_fix = "File corrupted - restore from backup";
        return validation;
    } catch (const std::exception &e) {
        validation.has_corruption = true;
        validation.error_message = fmt::format("File validation error: {}", e.what());
        validation.suggested_fix = "File access issue - check permissions";
        return validation;
    }

    return validation;
}
