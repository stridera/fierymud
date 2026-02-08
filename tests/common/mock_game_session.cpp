#include "mock_game_session.hpp"

#include <iostream>
#include <thread>

#include "commands/builtin_commands.hpp"
#include "commands/command_system.hpp"
#include "core/actor.hpp"
#include "core/logging.hpp"
#include "core/mobile.hpp"
#include "core/player.hpp"
#include "server/mud_server.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"

// MockPlayerConnection Implementation

MockPlayerConnection::MockPlayerConnection(asio::io_context &io_context, std::shared_ptr<WorldServer> world_server,
                                           NetworkManager *network_manager)
    : PlayerConnection(io_context, world_server, network_manager) {
    // Initialize test-specific state
}

Result<void> MockPlayerConnection::initialize() {
    // Start parent connection
    start();

    // Send welcome message like real connection
    send_message("Welcome to Modern FieryMUD!\r\n");
    send_prompt("What is your name? ");

    return Success();
}

void MockPlayerConnection::close() { disconnect(); }

void MockPlayerConnection::send_message(std::string_view message) {
    if (!is_connected()) {
        return;
    }

    std::lock_guard<std::mutex> lock(queue_mutex_);
    output_queue_.push(std::string(message));
}

void MockPlayerConnection::send_prompt(std::string_view prompt) { send_message(prompt); }

void MockPlayerConnection::simulate_input(std::string_view input) {
    if (!is_connected())
        return;

    std::lock_guard<std::mutex> lock(queue_mutex_);
    input_queue_.push(std::string(input));

    // Process input immediately for testing
    process_input_queue();
}

std::string MockPlayerConnection::get_output() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (output_queue_.empty()) {
        return "";
    }

    std::string output = output_queue_.front();
    output_queue_.pop();
    return output;
}

std::string MockPlayerConnection::get_all_output() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    std::string all_output;

    while (!output_queue_.empty()) {
        all_output += output_queue_.front();
        output_queue_.pop();
    }

    return all_output;
}

void MockPlayerConnection::clear_output() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!output_queue_.empty()) {
        output_queue_.pop();
    }
}

void MockPlayerConnection::process_input_queue() {
    // Note: Called with queue_mutex_ already locked
    while (!input_queue_.empty()) {
        std::string input = input_queue_.front();
        input_queue_.pop();

        // Remove carriage return/newline if present
        if (!input.empty() && input.back() == '\n') {
            input.pop_back();
        }
        if (!input.empty() && input.back() == '\r') {
            input.pop_back();
        }

        if (!input.empty()) {
            handle_input_by_state(input);
        }
    }
}

void MockPlayerConnection::handle_input_by_state(const std::string &input) {
    switch (state_) {
    case ConnectionState::Login:
        // Create the networked player
        transition_to(ConnectionState::Playing);
        if (world_server_) {
            auto player = std::make_shared<NetworkedPlayer>(shared_from_this(), input);
            player->initialize();

            // Special handling for test players that need specific rooms
            if (input == "WorldTester") {
                auto &world_manager = WorldManager::instance();
                auto target_room = world_manager.get_room(EntityId{100});
                if (target_room) {
                    // Remove from current room
                    auto current_room = player->current_room();
                    if (current_room) {
                        current_room->remove_actor(player->id());
                    }
                    // Move to room 100
                    player->set_current_room(target_room);
                    target_room->add_actor(player);
                }
            }

            world_server_->set_actor_for_connection(shared_from_this(), player);
            send_message(fmt::format("Welcome, {}!\r\n", input));
            world_server_->process_command(shared_from_this(), "look");
        }
        break;
    case ConnectionState::Playing:
        if (world_server_) {
            world_server_->process_command(shared_from_this(), input);
        }
        break;
    default:
        break;
    }
}

// MockGameSession Implementation

MockGameSession::MockGameSession(WorldServer &world_server, asio::io_context &io_context)
    : world_server_(std::shared_ptr<WorldServer>(&world_server, [](auto *) {})), io_context_(io_context),
      work_guard_(asio::make_work_guard(io_context)) {
    // Start the I/O thread after initialization to avoid destructor issues
    try {
        connection_ = std::make_shared<MockPlayerConnection>(io_context_, world_server_);

        // Start thread only after successful initialization
        io_thread_ = std::thread([this]() { io_context_.run(); });
    } catch (...) {
        // Ensure proper cleanup if initialization fails
        io_context_.stop();
        throw;
    }
}

MockGameSession::~MockGameSession() {
    try {
        disconnect();
        io_context_.stop();
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
        world_server_.reset(); // Explicitly reset WorldServer
    } catch (...) {
        // Suppress exceptions in destructor to avoid std::terminate
    }
}

Result<void> MockGameSession::connect(std::string_view player_name) {
    if (is_connected()) {
        return std::unexpected(Error{ErrorCode::InvalidState, "Session already connected"});
    }

    auto init_result = connection_->initialize();
    if (!init_result) {
        return init_result;
    }

    // Add connection to world server
    world_server_->add_player_connection(connection_);

    // If player name provided, simulate the name entry
    if (!player_name.empty()) {
        player_name_ = player_name;
        // Small delay to let initialization complete
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        send_input(player_name);
    }

    return Success();
}

void MockGameSession::disconnect() {
    if (is_connected()) {
        if (world_server_) {
            world_server_->remove_player_connection(connection_);
        }
        connection_->close();
        player_.reset();
    }
}

bool MockGameSession::is_connected() const {
    if (!connection_)
        return false;
    const auto state = connection_->state();
    // Consider the session "connected" only after transitioning out of the
    // initial Connected state (i.e., during Login, Playing, or Disconnecting).
    return state == ConnectionState::Login || state == ConnectionState::Playing ||
           state == ConnectionState::Disconnecting;
}

void MockGameSession::send_input(std::string_view command) {
    if (!is_connected())
        return;
    connection_->simulate_input(command);
}

std::string MockGameSession::get_output() {
    if (!connection_)
        return "";
    return connection_->get_output();
}

std::string MockGameSession::get_all_output() {
    if (!connection_)
        return "";
    return connection_->get_all_output();
}

void MockGameSession::clear_output() {
    if (connection_) {
        connection_->clear_output();
    }
}

bool MockGameSession::wait_for_output_containing(std::string_view text, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < timeout) {
        std::string output = get_all_output();
        if (output.find(text) != std::string::npos) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return false;
}

bool MockGameSession::wait_for_prompt(std::string_view prompt_text, std::chrono::milliseconds timeout) {
    return wait_for_output_containing(prompt_text, timeout);
}

EntityId MockGameSession::current_room() const {
    if (player_) {
        auto room = player_->current_room();
        return room ? room->id() : INVALID_ENTITY_ID;
    }
    return INVALID_ENTITY_ID;
}

std::string MockGameSession::player_name() const { return player_name_; }

ConnectionState MockGameSession::connection_state() const {
    return connection_ ? connection_->state() : ConnectionState::Connected;
}

std::shared_ptr<Room> MockGameSession::get_room(EntityId room_id) const {
    if (world_server_) {
        auto &world_manager = WorldManager::instance();
        return world_manager.get_room(room_id);
    }
    return nullptr;
}

std::vector<std::shared_ptr<Actor>> MockGameSession::get_actors_in_room(EntityId room_id) const {
    auto room = get_room(room_id);
    if (room) {
        return room->contents().actors;
    }
    return {};
}

// RAII helper to ensure test environment cleanup
class TestEnvironmentGuard {
  public:
    ~TestEnvironmentGuard() { UnifiedTestHarness::cleanup_test_environment(); }
};

std::unique_ptr<UnifiedTestHarness> UnifiedTestHarness::instance_ = nullptr;
bool UnifiedTestHarness::builtin_commands_registered_ = false;

UnifiedTestHarness &UnifiedTestHarness::instance() {
    if (!instance_) {
        instance_ = std::unique_ptr<UnifiedTestHarness>(new UnifiedTestHarness());
    }
    return *instance_;
}

UnifiedTestHarness::UnifiedTestHarness() : io_context_(), work_guard_(asio::make_work_guard(io_context_)) {
    // Create test configuration
    ServerConfig config;
    config.world_directory = "data/world";
    config.port = 0; // Not used for testing

    // Create and initialize world server
    world_server_ = std::make_unique<WorldServer>(io_context_, config);

    auto init_result = world_server_->initialize(true); // Test mode
    if (!init_result) {
        throw std::runtime_error("Failed to initialize test WorldServer: " + init_result.error().message);
    }

    // Initialize command system
    auto &command_system = CommandSystem::instance();
    auto cmd_result = command_system.initialize();
    if (!cmd_result) {
        throw std::runtime_error("Failed to initialize command system: " + cmd_result.error().message);
    }

    if (!builtin_commands_registered_) {
        auto builtin_result = BuiltinCommands::register_all_commands();
        if (!builtin_result) {
            throw std::runtime_error("Failed to register builtin commands: " + builtin_result.error().message);
        }
        builtin_commands_registered_ = true;
    }

    // Start world server
    auto start_result = world_server_->start();
    if (!start_result) {
        throw std::runtime_error("Failed to start test WorldServer: " + start_result.error().message);
    }

    // Start I/O thread
    io_thread_ = std::thread([this]() { io_context_.run(); });
}

UnifiedTestHarness::~UnifiedTestHarness() {
    try {
        // Stop world server
        if (world_server_) {
            world_server_->stop();
        }

        // Stop I/O context and join thread
        io_context_.stop();
        if (io_thread_.joinable()) {
            io_thread_.join();
        }

        // Reset world server
        world_server_.reset();
    } catch (...) {
        // Suppress exceptions in destructor to avoid std::terminate
    }
}

Result<void> UnifiedTestHarness::run_unit_test(std::function<void()> test) {
    TestEnvironmentGuard guard;
    instance().setup_test_environment();

    try {
        test();
        return Success();
    } catch (const std::exception &e) {
        return std::unexpected(Error{ErrorCode::TestFailure, fmt::format("Unit test failed: {}", e.what())});
    }
}

Result<void> UnifiedTestHarness::run_integration_test(std::function<void()> test) {
    TestEnvironmentGuard guard;
    instance().setup_test_environment();
    instance().reset_world_state();

    try {
        test();
        return Success();
    } catch (const std::exception &e) {
        return std::unexpected(Error{ErrorCode::TestFailure, fmt::format("Integration test failed: {}", e.what())});
    }
}

Result<void> UnifiedTestHarness::run_session_test(std::function<void(MockGameSession &)> test) {
    TestEnvironmentGuard guard;
    instance().setup_test_environment();
    instance().reset_world_state();

    try {
        auto session = instance().create_session();
        test(*session);
        return Success();
    } catch (const std::exception &e) {
        return std::unexpected(Error{ErrorCode::TestFailure, fmt::format("Session test failed: {}", e.what())});
    }
}

std::unique_ptr<MockGameSession> UnifiedTestHarness::create_session() {
    return std::make_unique<MockGameSession>(*instance().world_server_, instance().io_context_);
}

std::vector<std::unique_ptr<MockGameSession>> UnifiedTestHarness::create_multiple_sessions(size_t count) {
    std::vector<std::unique_ptr<MockGameSession>> sessions;
    sessions.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        sessions.push_back(UnifiedTestHarness::create_session());
    }

    return sessions;
}

void UnifiedTestHarness::setup_test_environment() {
    if (environment_initialized_) {
        return;
    }

    // Initialize logging for tests
    Logger::initialize("test.log", LogLevel::Debug);

    environment_initialized_ = true;
}

void UnifiedTestHarness::cleanup_test_environment() {
    if (instance_) {
        if (instance_->world_server_) {
            instance_->world_server_.reset();
        }
        instance_->environment_initialized_ = false;
    }
}

void UnifiedTestHarness::reset_world_state() {
    // Clear world manager state for clean tests
    auto &world_manager = WorldManager::instance();
    world_manager.clear_state();

    // Recreate test rooms after clearing state
    // Create room with ID 1 as primary room for player spawning (first available room)
    auto default_room_result = Room::create(EntityId{1}, "Test Room");
    if (default_room_result.has_value()) {
        auto default_room = std::shared_ptr<Room>(default_room_result.value().release());
        default_room->set_description("Default test room for player spawning.");
        default_room->set_short_description("a default test room");
        world_manager.add_room(default_room);
    }

    // Create room with ID 100 (expected by World: Room and Actor Integration test)
    auto starting_room_result = Room::create(EntityId{100}, "Starting Room");
    if (starting_room_result.has_value()) {
        auto starting_room = std::shared_ptr<Room>(starting_room_result.value().release());
        starting_room->set_description("A test room for integration testing.");
        starting_room->set_short_description("a test room");

        // Add a test NPC guard to satisfy the integration test expectations
        auto guard_result = Mobile::create(EntityId{200}, "test guard", 1);
        if (guard_result.has_value()) {
            auto guard = std::shared_ptr<Mobile>(guard_result.value().release());
            guard->set_current_room(starting_room);
            starting_room->add_actor(guard);
        }

        world_manager.add_room(starting_room);
    }
}
