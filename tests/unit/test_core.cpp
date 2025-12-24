/***************************************************************************
 *   File: tests/test_core.cpp                                Part of FieryMUD *
 *  Usage: Unit tests for core data structures and utilities               *
 ***************************************************************************/

#include "../src/core/ids.hpp"
#include "../src/core/result.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Core: EntityId", "[core][ids]") {
    SECTION("Default construction creates invalid ID") {
        EntityId id;
        REQUIRE_FALSE(id.is_valid());
        // Default constructor creates an invalid ID
        REQUIRE(id.zone_id() == 0);
        REQUIRE(id.local_id() == 0);
        REQUIRE(id == INVALID_ENTITY_ID);
    }

    SECTION("Explicit construction") {
        EntityId id{42};
        REQUIRE(id.is_valid());
        REQUIRE(id.value() == 42);
        REQUIRE(id != INVALID_ENTITY_ID);
    }

    SECTION("Zero-valued entity is valid") {
        // EntityId(0,0) is valid - it's a real entity ("a bug" object in zone 0)
        EntityId zero_entity{0, 0};
        REQUIRE(zero_entity.is_valid());
        REQUIRE(zero_entity.zone_id() == 0);
        REQUIRE(zero_entity.local_id() == 0);
        REQUIRE(zero_entity != INVALID_ENTITY_ID);
    }

    SECTION("Comparison operators") {
        EntityId id1{100};
        EntityId id2{100};
        EntityId id3{200};

        REQUIRE(id1 == id2);
        REQUIRE(id1 != id3);
        REQUIRE(id1 < id3);
        REQUIRE(id3 > id1);
    }

    SECTION("Hash functionality") {
        EntityId id1{123};
        EntityId id2{123};
        EntityId id3{456};

        EntityId::Hash hasher;
        REQUIRE(hasher(id1) == hasher(id2));
        REQUIRE(hasher(id1) != hasher(id3));

        // Test std::hash specialization
        std::hash<EntityId> std_hasher;
        REQUIRE(std_hasher(id1) == std_hasher(id2));
        REQUIRE(std_hasher(id1) != std_hasher(id3));
    }
}

TEST_CASE("Core: EntityRegistry", "[core][ids]") {
    struct TestEntity {
        int value;
        explicit TestEntity(int v) : value(v) {}
    };

    EntityRegistry<TestEntity> registry;

    SECTION("Empty registry") {
        REQUIRE(registry.empty());
        REQUIRE(registry.size() == 0);
        REQUIRE(registry.find(EntityId{1}) == nullptr);
        REQUIRE_FALSE(registry.get(EntityId{1}).has_value());
    }

    SECTION("Register and find entities") {
        auto entity1 = std::make_shared<TestEntity>(100);
        auto entity2 = std::make_shared<TestEntity>(200);

        auto id1 = registry.register_entity(entity1);
        auto id2 = registry.register_entity(entity2);

        REQUIRE(id1.is_valid());
        REQUIRE(id2.is_valid());
        REQUIRE(id1 != id2);

        REQUIRE_FALSE(registry.empty());
        REQUIRE(registry.size() == 2);

        auto found1 = registry.find(id1);
        auto found2 = registry.find(id2);

        REQUIRE(found1 != nullptr);
        REQUIRE(found2 != nullptr);
        REQUIRE(found1->value == 100);
        REQUIRE(found2->value == 200);

        auto opt1 = registry.get(id1);
        REQUIRE(opt1.has_value());
        REQUIRE(opt1.value()->value == 100);
    }

    SECTION("Register with specific ID") {
        auto entity = std::make_shared<TestEntity>(300);
        EntityId specific_id{999};

        registry.register_entity(specific_id, entity);

        auto found = registry.find(specific_id);
        REQUIRE(found != nullptr);
        REQUIRE(found->value == 300);

        // Next auto-generated ID should be 1000 or higher
        auto entity2 = std::make_shared<TestEntity>(400);
        auto auto_id = registry.register_entity(entity2);
        REQUIRE(auto_id.value() >= 1000);
    }

    SECTION("Unregister entities") {
        auto entity = std::make_shared<TestEntity>(500);
        auto id = registry.register_entity(entity);

        REQUIRE(registry.size() == 1);
        REQUIRE(registry.unregister_entity(id));
        REQUIRE(registry.size() == 0);
        REQUIRE_FALSE(registry.unregister_entity(id)); // Already removed
    }

    SECTION("Get all IDs and entities") {
        auto entity1 = std::make_shared<TestEntity>(100);
        auto entity2 = std::make_shared<TestEntity>(200);

        auto id1 = registry.register_entity(entity1);
        auto id2 = registry.register_entity(entity2);

        auto all_ids = registry.get_all_ids();
        auto all_entities = registry.get_all_entities();

        REQUIRE(all_ids.size() == 2);
        REQUIRE(all_entities.size() == 2);

        REQUIRE(std::find(all_ids.begin(), all_ids.end(), id1) != all_ids.end());
        REQUIRE(std::find(all_ids.begin(), all_ids.end(), id2) != all_ids.end());
    }

    SECTION("Weak pointer cleanup") {
        EntityId id;
        {
            auto entity = std::make_shared<TestEntity>(600);
            id = registry.register_entity(entity);
            REQUIRE(registry.size() == 1);
        }
        // Entity should be destroyed here

        // Registry still thinks it has the entity until cleanup
        REQUIRE(registry.find(id) == nullptr); // But find returns null

        registry.cleanup_expired();
        REQUIRE(registry.size() == 0); // Now it's cleaned up
    }
}

TEST_CASE("Core: Error System", "[core][result]") {
    SECTION("Error construction") {
        Error error1{ErrorCode::NotFound, "Test item not found"};
        REQUIRE(error1.code == ErrorCode::NotFound);
        REQUIRE(error1.message == "Test item not found");
        REQUIRE(error1.context.empty());

        Error error2{ErrorCode::PermissionDenied, "Access denied", "test_function"};
        REQUIRE(error2.code == ErrorCode::PermissionDenied);
        REQUIRE(error2.message == "Access denied");
        REQUIRE(error2.context == "test_function");
    }

    SECTION("Error utilities") {
        Error error{ErrorCode::InvalidArgument, "Bad input"};

        REQUIRE(error.is(ErrorCode::InvalidArgument));
        REQUIRE_FALSE(error.is(ErrorCode::NotFound));
        REQUIRE(error.code_name() == "InvalidArgument");

        std::string str = error.to_string();
        REQUIRE(str.find("InvalidArgument") != std::string::npos);
        REQUIRE(str.find("Bad input") != std::string::npos);
    }

    SECTION("Error helper functions") {
        auto not_found = Errors::NotFound("player");
        REQUIRE(not_found.code == ErrorCode::NotFound);
        REQUIRE(not_found.message.find("player") != std::string::npos);

        auto invalid_arg = Errors::InvalidArgument("name", "too long");
        REQUIRE(invalid_arg.code == ErrorCode::InvalidArgument);
        REQUIRE(invalid_arg.message.find("name") != std::string::npos);
        REQUIRE(invalid_arg.message.find("too long") != std::string::npos);

        auto perm_denied = Errors::PermissionDenied("delete");
        REQUIRE(perm_denied.code == ErrorCode::PermissionDenied);
        REQUIRE(perm_denied.message.find("delete") != std::string::npos);
    }
}

TEST_CASE("Core: Result<T>", "[core][result]") {
    SECTION("Success results") {
        Result<int> success{42};
        REQUIRE(success.has_value());
        REQUIRE(success.value() == 42);
        REQUIRE(*success == 42);
    }

    SECTION("Error results") {
        Result<int> failure{std::unexpected(Errors::NotFound("item"))};
        REQUIRE_FALSE(failure.has_value());
        REQUIRE(failure.error().code == ErrorCode::NotFound);
        REQUIRE(failure.error().message.find("item") != std::string::npos);
    }

    SECTION("VoidResult functionality") {
        VoidResult success = Success();
        REQUIRE(success.has_value());

        VoidResult failure{std::unexpected(Errors::InvalidState("locked"))};
        REQUIRE_FALSE(failure.has_value());
        REQUIRE(failure.error().code == ErrorCode::InvalidState);
    }

    SECTION("Result transformation") {
        auto divide = [](int a, int b) -> Result<int> {
            if (b == 0) {
                return std::unexpected(Errors::InvalidArgument("divisor", "cannot be zero"));
            }
            return a / b;
        };

        auto result1 = divide(10, 2);
        REQUIRE(result1.has_value());
        REQUIRE(result1.value() == 5);

        auto result2 = divide(10, 0);
        REQUIRE_FALSE(result2.has_value());
        REQUIRE(result2.error().code == ErrorCode::InvalidArgument);
    }
}