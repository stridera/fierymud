#pragma once

#include "core/ids.hpp"
#include "core/result.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

// Forward declarations
class PlayerConnection;
class Player;

/**
 * @brief Login states for the connection state machine
 *
 * Supports both legacy character-based login and modern account-based login:
 * - Legacy: GetName -> GetPassword -> Playing
 * - Account: GetAccount -> GetAccountPassword -> SelectCharacter -> Playing
 */
enum class LoginState {
    // Initial state - waiting for login system to initialize
    Connecting,         // Connection established, waiting for start_login()

    // Account-based login flow
    GetAccount,         // Asking for account username or email
    GetAccountPassword, // Asking for account password
    SelectCharacter,    // Showing character selection menu

    // Legacy character-based login (also used for character creation)
    GetName,         // Asking for character name
    GetPassword,     // Asking for password (existing character)
    ConfirmNewName,  // Confirming new character name
    GetNewPassword,  // Getting password for new character
    ConfirmPassword, // Confirming new password
    SelectClass,     // Choosing character class
    SelectRace,      // Choosing character race
    ConfirmCreation, // Final confirmation before creation

    // Terminal states
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
    // Account-based login handlers
    void handle_get_account(std::string_view input);
    void handle_get_account_password(std::string_view input);
    void handle_select_character(std::string_view input);

    // Legacy/character creation handlers
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
    void send_logo_full();           // Full dragon logo (256-color + Unicode)
    void send_logo_extended();       // Extended logo (256-color, simpler art)
    void send_logo_basic();          // Basic ASCII fallback
    void send_login_instructions();  // Common footer with login info
    void send_class_menu();
    void send_race_menu();
    void send_creation_summary();
    void send_character_menu();
    void disconnect_with_message(std::string_view message);

    // User/account management
    Result<bool> user_exists(std::string_view username_or_email);
    Result<bool> verify_user(std::string_view username_or_email, std::string_view password);
    void load_user_characters();

    // Character management
    Result<bool> character_exists(std::string_view name);
    Result<std::shared_ptr<Player>> load_character(std::string_view name, std::string_view password);
    Result<std::shared_ptr<Player>> create_character();
    Result<void> save_character(std::shared_ptr<Player> player);
    void load_player_abilities(std::shared_ptr<Player>& player);
    void load_player_items(std::shared_ptr<Player>& player);
    void load_player_effects(std::shared_ptr<Player>& player);

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
    LoginState state_{LoginState::Connecting};  // Start in connecting state until start_login()
    std::vector<std::string> buffered_input_;   // Input received before login started
    std::shared_ptr<PlayerConnection> connection_;
    CharacterCreationData creation_data_;
    std::shared_ptr<Player> player_;

    // User/account state
    std::string user_id_;           // UUID of logged-in user
    std::string account_name_;      // Username or email used to login
    std::vector<std::tuple<std::string, std::string, int, std::string>> user_characters_;
    // Characters: (id, name, level, class)

    // Callbacks
    PlayerLoadedCallback player_loaded_callback_;

    // Attempt tracking and rate limiting
    int login_attempts_{0};
    static constexpr int MAX_LOGIN_ATTEMPTS = 3;
    std::chrono::steady_clock::time_point last_failed_attempt_{};

    // Rate limiting: delay in seconds = BASE_DELAY * 2^(attempts-1)
    // 1st failure: 1s, 2nd: 2s, 3rd: 4s (then disconnect)
    static constexpr int RATE_LIMIT_BASE_DELAY_MS = 1000;

    // Check if enough time has passed since last failed attempt
    // Returns true if request should be processed, false if rate limited
    bool check_rate_limit();

    // Record a failed login attempt and log it
    void record_failed_attempt(std::string_view attempted_name);
};
