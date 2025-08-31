/***************************************************************************
 *   File: src/game/login_system.hpp            Part of FieryMUD *
 *  Usage: Modern login and character creation system                     *
 ***************************************************************************/

#pragma once

#include "../core/actor.hpp"
#include "../core/ids.hpp"
#include "../core/result.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

// Forward declarations
class PlayerConnection;

/**
 * @brief Login states for the connection state machine
 */
enum class LoginState {
    GetName,         // Asking for character name
    GetPassword,     // Asking for password (existing character)
    ConfirmNewName,  // Confirming new character name
    GetNewPassword,  // Getting password for new character
    ConfirmPassword, // Confirming new password
    SelectClass,     // Choosing character class
    SelectRace,      // Choosing character race
    ConfirmCreation, // Final confirmation before creation
    Playing,         // Successfully logged in and playing
    Disconnected     // Connection closed
};

/**
 * @brief Character class options for creation
 */
enum class CharacterClass { Warrior = 1, Cleric = 2, Sorcerer = 3, Rogue = 4 };

/**
 * @brief Character race options for creation
 */
enum class CharacterRace { Human = 1, Elf = 2, Dwarf = 3, Halfling = 4 };

/**
 * @brief Character creation data collected during login
 */
struct CharacterCreationData {
    std::string name;
    std::string password;
    CharacterClass character_class{CharacterClass::Warrior};
    CharacterRace race{CharacterRace::Human};

    bool is_complete() const;
    std::string get_class_name() const;
    std::string get_race_name() const;
};

/**
 * @brief Login system manages the connection state machine
 *
 * Handles the complete login flow:
 * 1. Get character name
 * 2. Check if character exists
 * 3. If exists: authenticate with password
 * 4. If new: guide through character creation
 * 5. Create/load player and enter game
 */
class LoginSystem {
  public:
    using PlayerLoadedCallback = std::function<void(std::shared_ptr<Player>)>;

    explicit LoginSystem(std::shared_ptr<PlayerConnection> connection);
    ~LoginSystem() = default;

    // State machine control
    void start_login();
    void process_input(std::string_view input);

    // Callbacks
    void set_player_loaded_callback(PlayerLoadedCallback callback) { player_loaded_callback_ = std::move(callback); }

  private:
    // State machine handlers
    void handle_get_name(std::string_view input);
    void handle_get_password(std::string_view input);
    void handle_confirm_new_name(std::string_view input);
    void handle_get_new_password(std::string_view input);
    void handle_confirm_password(std::string_view input);
    void handle_select_class(std::string_view input);
    void handle_select_race(std::string_view input);
    void handle_confirm_creation(std::string_view input);

    // Helper methods
    void transition_to(LoginState new_state);
    void send_prompt();
    void send_message(std::string_view message);
    void send_welcome_message();
    void send_class_menu();
    void send_race_menu();
    void send_creation_summary();
    void disconnect_with_message(std::string_view message);

    // Character management
    Result<bool> character_exists(std::string_view name);
    Result<std::shared_ptr<Player>> load_character(std::string_view name, std::string_view password);
    Result<std::shared_ptr<Player>> create_character();
    Result<void> save_character(std::shared_ptr<Player> player);

    // Validation
    bool is_valid_name(std::string_view name) const;
    bool is_valid_password(std::string_view password) const;
    std::string normalize_name(std::string_view name) const;
    
    // Player file validation and migration
    struct PlayerFileValidation {
        bool is_valid{false};
        bool needs_migration{false};
        bool has_corruption{false};
        std::string error_message;
        std::string suggested_fix;
    };
    
    PlayerFileValidation validate_player_file(std::string_view name) const;

    // State
    LoginState state_{LoginState::GetName};
    std::shared_ptr<PlayerConnection> connection_;
    CharacterCreationData creation_data_;
    std::shared_ptr<Player> player_;

    // Callbacks
    PlayerLoadedCallback player_loaded_callback_;

    // Attempt tracking
    int login_attempts_{0};
    static constexpr int MAX_LOGIN_ATTEMPTS = 3;
};