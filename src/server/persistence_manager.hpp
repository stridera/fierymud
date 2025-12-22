#pragma once

#include "../core/result.hpp"
#include <memory>

// Forward declarations
struct ServerConfig;
class Player;

/**
 * Modern Persistence Manager - JSON-based data storage
 */
class PersistenceManager {
public:
    /** Get singleton instance */
    static PersistenceManager& instance();
    
    Result<void> initialize(const ServerConfig& config);
    
    Result<void> save_player(const Player& player);
    Result<void> save_all_players();
    Result<std::shared_ptr<Player>> load_player(std::string_view name);
    
    Result<void> backup_data();
    
private:
    const ServerConfig* config_ = nullptr;
    
    // Singleton pattern
    PersistenceManager() = default;
    ~PersistenceManager() = default;
    PersistenceManager(const PersistenceManager&) = delete;
    PersistenceManager& operator=(const PersistenceManager&) = delete;
};