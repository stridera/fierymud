/***************************************************************************
 *   File: src/server/persistence_manager.hpp             Part of FieryMUD *
 *  Usage: Modern data persistence and storage management                  *
 ***************************************************************************/

#pragma once

#include "../core/result.hpp"

// Forward declarations
struct ServerConfig;
class Player;

/**
 * Modern Persistence Manager - JSON-based data storage
 */
class PersistenceManager {
public:
    Result<void> initialize(const ServerConfig& config);
    
    Result<void> save_player(const Player& player);
    Result<void> save_all_players();
    Result<Player> load_player(std::string_view name);
    
    Result<void> backup_data();
    
private:
    const ServerConfig* config_ = nullptr;
};