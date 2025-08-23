#pragma once

#include "commands/builtin_commands.hpp"
#include "commands/command_system.hpp"
#include "core/actor.hpp"
#include "server/mud_server.hpp"
#include "server/world_server.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"

#include <asio.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class TestablePlayer : public Player {
  public:
    TestablePlayer(EntityId id, std::string_view name) : Player(id, name) {}

    void send_message(std::string_view message) override {
        output.push_back(std::string(message));
        if (output_promise && !promise_fulfilled) {
            output_promise->set_value();
            promise_fulfilled = true;
        }
    }

    void prepare_for_output() {
        output_promise = std::make_unique<std::promise<void>>();
        promise_fulfilled = false;
    }

    std::future<void> get_output_future() { return output_promise->get_future(); }

    std::vector<std::string> output;

  private:
    std::unique_ptr<std::promise<void>> output_promise;
    bool promise_fulfilled = false;
};

class TestableNPC : public Actor {
  public:
    TestableNPC(EntityId id, std::string_view name) : Actor(id, name) {}

    void send_message(std::string_view message) override {}
    void receive_message(std::string_view message) override {}
};

class TestHarness {
  public:
    class TestFixtures {
      public:
        TestFixtures(TestHarness &harness) : harness_(harness) {}

        Room &create_room(std::string_view name) {
            auto room_result = Room::create(EntityId{next_entity_id++}, name, SectorType::Inside);
            auto room = std::shared_ptr<Room>(room_result.value().release());
            WorldManager::instance().add_room(room);
            return *room;
        }

        Actor &create_npc(std::string_view name, Room &room) {
            auto npc = std::make_shared<TestableNPC>(EntityId{next_entity_id++}, name);
            npc->move_to(WorldManager::instance().get_room(room.id()));
            return *npc;
        }

      private:
        TestHarness &harness_;
        static inline int next_entity_id = 100;
    };

    asio::io_context io_context;
    ServerConfig config;
    WorldServer world_server;
    std::shared_ptr<TestablePlayer> player;
    std::shared_ptr<Room> start_room;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    std::thread io_thread;
    TestFixtures fixtures_;

    TestHarness() : world_server(io_context, config), work_guard(asio::make_work_guard(io_context)), fixtures_(*this) {
        // Ensure WorldManager is clean before adding test data
        WorldManager::instance().clear_state();
        WorldManager::instance().initialize("lib/world", false); // Initialize without loading files

        io_thread = std::thread([&]() { io_context.run(); });

        // Create a starting room and add it to WorldManager
        auto room_result = Room::create(EntityId{1}, "Test Room", SectorType::Inside);
        start_room = std::shared_ptr<Room>(room_result.value().release());
        start_room->set_description("This is a test room.");
        WorldManager::instance().add_room(start_room);
        WorldManager::instance().set_start_room(EntityId{1});

        // Initialize the world server (without loading default world data)
        world_server.initialize(true); // Pass true for test mode
        world_server.start();
        // No wait for world_loaded_future, as we manually manage rooms

        // Register builtin commands (similar to MockGameSession)
        auto builtin_result = BuiltinCommands::register_all_commands();
        if (!builtin_result) {
            throw std::runtime_error("Failed to register builtin commands: " + builtin_result.error().message);
        }

        player = std::make_shared<TestablePlayer>(EntityId{1}, "TestPlayer");
        player->move_to(start_room);
    }

    ~TestHarness() {
        asio::post(io_context, [&]() {
            world_server.stop();
            io_context.stop();
        });
        if (io_thread.joinable()) {
            io_thread.join();
        }
        // WorldManager state is cleared at the beginning of the next test
    }

    TestHarness &execute_command(const std::string &command) {
        player->prepare_for_output();
        asio::post(world_server.get_strand(), [&]() { world_server.process_command(player, command); });
        return *this;
    }

    TestHarness &and_wait_for_output() {
        auto future = player->get_output_future();
        auto status = future.wait_for(std::chrono::seconds(5)); // 5 second timeout
        if (status == std::future_status::timeout) {
            throw std::runtime_error("Timeout waiting for command output");
        }
        return *this;
    }

    TestHarness &then_assert_output_contains(std::string_view text) {
        const auto &output = get_output();
        bool found = false;
        for (const auto &line : output) {
            if (line.find(text) != std::string::npos) {
                found = true;
                break;
            }
        }
        REQUIRE(found);
        return *this;
    }

    TestHarness &then_assert_output_size(size_t size) {
        REQUIRE(get_output().size() == size);
        return *this;
    }

    const std::vector<std::string> &get_output() const { return player->output; }

    void clear_output() { player->output.clear(); }

    TestFixtures &fixtures() { return fixtures_; }
};