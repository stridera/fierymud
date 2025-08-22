/***************************************************************************
 *   File: tests/mock_game_session.hpp                Part of FieryMUD *
 *  Usage: Mock game session for testing NetworkedPlayer connection flows  *
 ***************************************************************************/

#pragma once

#include "../src/modern/server/world_server.hpp"
#include "../src/modern/server/player_connection.hpp"
#include "../src/modern/server/networked_actor.hpp"
#include "../src/modern/core/result.hpp"
#include "../src/modern/core/ids.hpp"
#include "test_harness.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <queue>
#include <chrono>
#include <future>
#include <atomic>

/**
 * MockPlayerConnection - Simulates network connection for testing
 */
class MockPlayerConnection : public PlayerConnection {
public:
    MockPlayerConnection(asio::io_context& io_context);
    ~MockPlayerConnection() = default;

    // Override parent methods for testing
    Result<void> initialize() override;
    void close() override;
    Result<void> send(std::string_view message) override;
    Result<void> send_prompt(std::string_view prompt) override;
    void start_read() override {} // No-op for testing

    // Mock input/output for testing
    void simulate_input(std::string_view input);
    std::string get_output();
    std::string get_all_output();
    void clear_output();

private:
    // Mock-specific state
    std::queue<std::string> output_queue_;
    std::queue<std::string> input_queue_;
    mutable std::mutex queue_mutex_;

    void process_input_queue();
    void handle_input_by_state(const std::string& input);
};

/**
 * MockGameSession - High-level interface for testing complete player sessions
 */
class MockGameSession {
public:
    MockGameSession();
    ~MockGameSession();

    // Session lifecycle
    Result<void> connect(std::string_view player_name = "");
    void disconnect();
    bool is_connected() const;

    // Input/Output simulation
    void send_input(std::string_view command);
    std::string get_output();
    std::string get_all_output();
    void clear_output();

    // Wait for specific output (with timeout)
    bool wait_for_output_containing(std::string_view text, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    bool wait_for_prompt(std::string_view prompt_text, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

    // State verification
    EntityId current_room() const;
    std::string player_name() const;
    ConnectionState connection_state() const;

    // Game state queries
    std::shared_ptr<Room> get_room(EntityId room_id) const;
    std::vector<std::shared_ptr<Actor>> get_actors_in_room(EntityId room_id) const;

    // Utility methods
    std::shared_ptr<NetworkedPlayer> get_player() const { return player_; }
    std::shared_ptr<MockPlayerConnection> get_connection() const { return connection_; }

private:
    std::shared_ptr<WorldServer> world_server_;
    std::shared_ptr<MockPlayerConnection> connection_;
    std::shared_ptr<NetworkedPlayer> player_;
    std::string player_name_;

    private:
    asio::io_context io_context_;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
    std::thread io_thread_;

    void initialize_world_server();
};

/**
 * UnifiedTestHarness - Unified testing framework supporting multiple test types
 */
class UnifiedTestHarness {
public:
    enum class TestMode {
        Unit,        // Isolated component testing
        Integration, // Multi-component testing  
        Session      // Full NetworkedPlayer session testing
    };

    static UnifiedTestHarness& instance();

    // Test execution modes
    static Result<void> run_unit_test(std::function<void()> test);
    static Result<void> run_integration_test(std::function<void()> test);
    static Result<void> run_session_test(std::function<void(MockGameSession&)> test);

    // Session factory methods
    static std::unique_ptr<MockGameSession> create_session();
    static std::vector<std::unique_ptr<MockGameSession>> create_multiple_sessions(size_t count);

    // Test environment management
    static void setup_test_environment();
    static void cleanup_test_environment();
    static void reset_world_state();

private:
    UnifiedTestHarness() = default;
    static std::unique_ptr<WorldServer> test_world_server_;
    static bool environment_initialized_;
    public:
    static bool builtin_commands_registered_;
};

// Note: Use standard Catch2 TEST_CASE syntax with UnifiedTestHarness::run_*_test() methods