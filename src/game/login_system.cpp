/***************************************************************************
 *   File: src/game/login_system.cpp            Part of FieryMUD *
 *  Usage: Modern login and character creation implementation             *
 ***************************************************************************/

#include "login_system.hpp"

#include "../core/config.hpp"
#include "../core/logging.hpp"
#include "../net/player_connection.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>

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

void LoginSystem::start_login() {
    Log::debug("Starting login process for new connection");
    transition_to(LoginState::GetName);
    send_welcome_message();
    send_prompt();
}

void LoginSystem::process_input(std::string_view input) {
    if (state_ == LoginState::Disconnected) {
        return;
    }

    // Trim whitespace
    std::string trimmed_input;
    auto start = input.find_first_not_of(" \t\r\n");
    auto end = input.find_last_not_of(" \t\r\n");
    if (start != std::string::npos) {
        trimmed_input = std::string(input.substr(start, end - start + 1));
    }

    Log::debug("Processing login input in state {}: '{}'", static_cast<int>(state_), trimmed_input);

    switch (state_) {
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
    case LoginState::Playing:
        // Input should be handled by game systems, not login
        Log::warn("Received input in Playing state - should be handled by game");
        break;
    case LoginState::Disconnected:
        // Already handled above
        break;
    }
}

void LoginSystem::handle_get_name(std::string_view input) {
    if (input.empty()) {
        send_message("Please enter a character name.");
        send_prompt();
        return;
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
    transition_to(LoginState::Playing);

    send_message(fmt::format("Welcome to FieryMUD, {}!", player_->name()));

    if (player_loaded_callback_) {
        player_loaded_callback_(player_);
    }

    Log::info("Player '{}' logged in successfully", player_->name());
}

void LoginSystem::handle_confirm_new_name(std::string_view input) {
    std::string lower_input;
    std::transform(input.begin(), input.end(), std::back_inserter(lower_input), ::tolower);

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
    std::string lower_input;
    std::transform(input.begin(), input.end(), std::back_inserter(lower_input), ::tolower);

    if (lower_input == "yes" || lower_input == "y") {
        auto create_result = create_character();
        if (!create_result) {
            send_message("Error creating character. Please try again later.");
            disconnect_with_message("Character creation failed.");
            return;
        }

        player_ = create_result.value();

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
    case LoginState::GetName:
        prompt_text = "Enter your character name: ";
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
        prompt_text = "> ";
        break;
    }

    if (connection_) {
        connection_->send_prompt(prompt_text);
    }
}

void LoginSystem::send_message(std::string_view message) {
    if (connection_) {
        connection_->send_message(fmt::format("{}\r\n", message));
    }
}

void LoginSystem::send_welcome_message() {
    send_message("");
    send_message("=== Welcome to Modern FieryMUD ===");
    send_message("");
    send_message("A classic fantasy MUD with modern features:");
    send_message("• Health/Movement vitals (no mana!)");
    send_message("• Circle-based spell slot system");
    send_message("• GMCP support for modern clients");
    send_message("• Four classic classes: Warrior, Cleric, Sorcerer, Rogue");
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

void LoginSystem::disconnect_with_message(std::string_view message) {
    send_message(message);
    transition_to(LoginState::Disconnected);
    if (connection_) {
        connection_->disconnect(message);
    }
}

// Character management
Result<bool> LoginSystem::character_exists(std::string_view name) {
    // Simple file-based character storage for now
    const auto &dir = Config::instance().player_directory();
    std::filesystem::path path{dir};
    path /= fmt::format("{}.json", name);
    return std::filesystem::exists(path);
}

Result<std::shared_ptr<Player>> LoginSystem::load_character(std::string_view name, std::string_view password) {
    // Simple file-based loading for now
    const auto &dir = Config::instance().player_directory();
    std::filesystem::path path{dir};
    path /= fmt::format("{}.json", name);
    auto filename = path.string();

    std::ifstream file(filename);
    if (!file.is_open()) {
        return std::unexpected(Error{ErrorCode::FileNotFound, fmt::format("Character file not found: {}", filename)});
    }

    nlohmann::json data;
    try {
        file >> data;
    } catch (const std::exception &e) {
        return std::unexpected(
            Error{ErrorCode::ParseError, fmt::format("Failed to parse character file: {}", e.what())});
    }

    // Verify password (in real implementation, this would be hashed)
    if (data.at("password") != password) {
        return std::unexpected(Error{ErrorCode::PermissionDenied, "Invalid password"});
    }

    // Load the player
    auto player_result = Player::from_json(data.at("player_data"));
    if (!player_result) {
        return std::unexpected(player_result.error());
    }

    return std::shared_ptr<Player>(player_result.value().release());
}

Result<std::shared_ptr<Player>> LoginSystem::create_character() {
    if (!creation_data_.is_complete()) {
        return std::unexpected(Error{ErrorCode::InvalidState, "Character creation data incomplete"});
    }

    // Generate unique ID (simple counter for now)
    static EntityId next_id{1000};
    EntityId player_id = next_id;
    next_id = EntityId{next_id.value() + 1};

    auto player_result = Player::create(player_id, creation_data_.name);
    if (!player_result) {
        return std::unexpected(player_result.error());
    }

    auto player = std::move(player_result.value());

    // Set character class and race from creation data
    player->set_class(magic_enum::enum_name(creation_data_.character_class));
    player->set_race(magic_enum::enum_name(creation_data_.race));

    return std::shared_ptr<Player>(player.release());
}

Result<void> LoginSystem::save_character(std::shared_ptr<Player> player) {
    if (!player) {
        return std::unexpected(Error{ErrorCode::InvalidArgument, "Null player"});
    }

    // Ensure players directory exists
    const auto &dir = Config::instance().player_directory();
    std::filesystem::create_directories(dir);

    std::filesystem::path path{dir};
    path /= fmt::format("{}.json", player->name());
    auto filename = path.string();

    nlohmann::json data;
    data["password"] = creation_data_.password; // In real implementation, hash this!
    data["player_data"] = player->to_json();
    data["created"] =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    std::ofstream file(filename);
    if (!file.is_open()) {
        return std::unexpected(
            Error{ErrorCode::FileAccessError, fmt::format("Cannot create character file: {}", filename)});
    }

    try {
        file << data.dump(2);
    } catch (const std::exception &e) {
        return std::unexpected(
            Error{ErrorCode::SerializationError, fmt::format("Failed to save character: {}", e.what())});
    }

    Log::info("Saved character '{}' to {}", player->name(), filename);
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
        std::vector<std::string> required_fields = {
            "name", "id", "level", "class", "race", "attributes", "vitals"
        };
        
        for (const auto& field : required_fields) {
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
        
    } catch (const nlohmann::json::exception& e) {
        validation.has_corruption = true;
        validation.error_message = fmt::format("JSON parsing error: {}", e.what());
        validation.suggested_fix = "File corrupted - restore from backup";
        return validation;
    } catch (const std::exception& e) {
        validation.has_corruption = true;
        validation.error_message = fmt::format("File validation error: {}", e.what());
        validation.suggested_fix = "File access issue - check permissions";
        return validation;
    }
    
    return validation;
}

Result<void> LoginSystem::migrate_player_file(std::string_view name) {
    const auto &dir = Config::instance().player_directory();
    std::filesystem::path path{dir};
    path /= fmt::format("{}.json", name);
    auto filename = path.string();
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return std::unexpected(Error{ErrorCode::FileNotFound, fmt::format("Cannot open file: {}", filename)});
        }
        
        nlohmann::json data;
        file >> data;
        file.close();
        
        // Backup original file
        auto backup_path = fmt::format("{}.backup.{}", filename, 
                                     std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::copy_file(path, backup_path);
        
        bool modified = false;
        
        // Add missing created timestamp if needed
        if (!data.contains("created")) {
            data["created"] = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            modified = true;
        }
        
        // Ensure player_data has all required modern fields
        auto& player_data = data["player_data"];
        
        // Add default equipment if missing
        if (!player_data.contains("equipment")) {
            player_data["equipment"] = nlohmann::json::object();
            modified = true;
        }
        
        // Add default inventory if missing  
        if (!player_data.contains("inventory")) {
            player_data["inventory"] = nlohmann::json::array();
            modified = true;
        }
        
        if (modified) {
            // Write migrated file
            std::ofstream output_file(filename);
            if (!output_file.is_open()) {
                return std::unexpected(Error{ErrorCode::FileAccessError, "Cannot write migrated file"});
            }
            
            output_file << data.dump(2);
            output_file.close();
            
            Log::info("Migrated player file for '{}' (backup: {})", name, backup_path);
        }
        
        return Success();
        
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::ParseError, fmt::format("Migration failed: {}", e.what())});
    }
}