# FieryMUD Modernization Plan

## Executive Summary

This document provides a concrete, incremental roadmap for modernizing FieryMUD from legacy C-style code to modern C++23 architecture. The plan emphasizes safety, testability, and maintainability while preserving gameplay functionality during the transition.

**Core Objectives**: Transform monolithic global-state architecture into modular, testable components with clear domain boundaries, modern C++23 features, and comprehensive testing.

---

# 📋 Project Scope & Goals

## Objectives

✅ **Modernize Architecture**
- Transform legacy global-state design into modular, domain-driven architecture
- Establish clear boundaries between networking, game logic, and persistence layers
- Implement single-threaded world simulation with message-passing I/O

✅ **Improve Code Quality**
- Replace C-style structs and globals with modern C++23 classes
- Add comprehensive test coverage using Catch2
- Implement RAII, smart pointers, and modern standard library features

✅ **Enable Future Development**
- Create stable, testable APIs for World, Entity/Object/Actor systems
- Establish patterns for safe, incremental feature development
- Provide foundation for advanced features like scripting modernization

## Non-Goals (This Phase)

❌ **New Game Features** - Focus on architecture migration, not gameplay additions
❌ **Complete Scripting Overhaul** - Basic adapters only, full Lua migration is separate
❌ **Protocol Changes** - Network layer isolation only, no protocol modifications

# 🏗️ Target Architecture

## Module Organization

The new architecture organizes code into logical modules with clear responsibilities:

| Module | Purpose | Key Components |
|--------|---------|----------------|
| **Core** | Foundation utilities | IDs, time, logging, error handling, constants |
| **Domain** | Game entities | Entity base, Object/Actor hierarchies, stats, inventory |
| **World** | Game world management | World, Zone, Room, weather, registries, spawning |
| **GameLoop** | Simulation engine | World tick, scheduling, event dispatch, combat |
| **Net** | Network layer | Sessions, telnet, input parsing, command requests |
| **Commands** | Command processing | Parsing, routing, execution in world thread |
| **Persistence** | Data storage | JSON load/save for world data and players |
| **Logging** | Diagnostics | Structured logging, metrics, debugging |

## Concurrency Model

**Single-Threaded Game World**
- One world strand owns ALL world state mutations
- Eliminates data races and synchronization complexity
- Deterministic execution for reliable testing

**Multi-Threaded I/O**
- Separate I/O strands handle networking
- I/O threads NEVER mutate world state directly
- Message passing between I/O and world threads

**Message Flow**
```
Network Input → Command Request → World Thread → Output Event → Network Output
```

# 🎯 Domain Model

## Core Entity Hierarchy

```cpp
// Base entity for all game objects
class Entity {
public:
    EntityId id() const;
    std::string_view name() const;
    std::span<const std::string> keywords() const;
    std::string_view description() const;
    
    // JSON serialization
    nlohmann::json to_json() const;
    static std::expected<Entity, Error> from_json(const nlohmann::json&);
};

// Game objects (items, weapons, armor, etc.)
class Object : public Entity {
public:
    ObjectType type() const;        // weapon, armor, container, etc.
    bool is_container() const;
    int capacity() const;
    bool is_weapon() const;
    DamageProfile damage_profile() const;
    EquipSlot equip_slot() const;
};

// Living beings (players and NPCs)
class Actor : public Entity {
public:
    std::expected<void, Error> move_to(std::shared_ptr<Room> room);
    void receive_message(std::string_view msg);
    void enqueue_output(std::string_view msg);
    bool is_visible_to(const Actor& other) const;
    
    Stats& stats();
    Inventory& inventory();
    Equipment& equipment();
};

class Mob : public Actor {
    // AI behavior, respawn rules
};

class Player : public Actor {
    // Session handle, account data, permissions
};
```

## World Management

```cpp
class Room {
public:
    EntityId id() const;
    void add_actor(std::shared_ptr<Actor> actor);
    void remove_actor(EntityId id);
    void add_object(std::shared_ptr<Object> obj);
    std::optional<EntityId> find_exit(Direction dir) const;
    
    template<typename Func>
    void for_each_occupant(Func&& func);
    
    void broadcast(std::string_view msg, const Actor* exclude = nullptr);
};

class World {
public:
    std::optional<std::shared_ptr<Room>> get_room(EntityId id);
    void post(CommandRequest request);  // Thread-safe command queuing
    void schedule(std::chrono::milliseconds delay, std::function<void()> func);
    
    std::shared_ptr<Object> spawn_object(EntityId id, std::shared_ptr<Room> room);
    std::shared_ptr<Mob> spawn_mob(EntityId id, std::shared_ptr<Room> room);
};
```

# ⚡ Modern C++ Standards

## Error Handling
```cpp
// Use std::expected for operations that can fail
std::expected<Player, Error> load_player(std::string_view name);

// Use std::optional for nullable values  
std::optional<Room> find_room(Vnum vnum);

// Error types with context
struct Error {
    enum class Code { NotFound, InvalidState, Permission, Parse };
    Code code;
    std::string message;
};
```

## Logging & Debugging
```cpp
// Structured logging with spdlog
spdlog::info("Player {} entered room {}", player.name(), room.vnum());

// Context-rich error messages
log_error("Command failed", {
    {"player", player.name()},
    {"command", cmd.verb},
    {"room", player.room()->vnum()},
    {"reason", error.message}
});
```

## Modern Features Required
- **Strings**: `fmt::format()`, `std::string_view` parameters
- **Containers**: `std::vector`, `std::span`, `std::unordered_map`
- **Memory**: Smart pointers, RAII, no raw `new`/`delete`
- **Algorithms**: `std::ranges`, range-based for loops
- **JSON**: `nlohmann/json` for all data serialization
- **Testing**: `Catch2` with comprehensive coverage

## 6) File/Folder Layout (incremental)

We will introduce new headers/impl into subfolders without breaking existing includes, then migrate call sites:
- src/core/… (ids.hpp, result.hpp, time.hpp, logging.hpp)
- src/domain/… (entity.hpp, object.hpp, actor.hpp, mob.hpp, player.hpp, stats.hpp, inventory.hpp)
- src/world/… (room.hpp, zone.hpp, world.hpp)
- src/game/… (loop.hpp, scheduler.hpp, commands.hpp)
- src/net/… (session.hpp, server.hpp)
- tests/… (mirror structure; Catch2)

Note: During early phases we may place thin adapters in existing files (e.g., keep `interpreter.cpp` but forward into Commands).

## 7) APIs and Contracts (LLM reference)

Core types
- Unified EntityId replaces all legacy ID types (vnum/rnum/obj_num/room_num/zone_vnum)
- using EntityId = std::uint64_t; constexpr auto INVALID_ENTITY_ID = 0;
- struct Error { enum class Code { NotFound, InvalidState, Permission, Parse }; Code code; std::string msg; };

World
- class World {
  - get_room(Vnum) -> std::optional<std::shared_ptr<Room>>
  - post(CommandRequest) -> void  // thread-safe; queues to world strand
  - schedule(std::chrono::milliseconds, Func) -> void
  - broadcast(std::string_view msg) -> void
}

Command pipeline
- struct CommandRequest { EntityId player_id; std::string verb; std::vector<std::string> args; };
- execute_command(World&, CommandRequest) -> std::expected<void, Error>

Room
- class Room {
  - vnum() -> Vnum
  - add_actor(std::shared_ptr<Actor>) -> void
  - remove_actor(EntityId) -> void
  - add_object(std::shared_ptr<Object>) -> void
  - for_each_occupant(Func) -> void
  - find_exit(Direction) -> std::optional<Vnum>
}

Actor
- class Actor {
  - id() -> EntityId; room() -> std::weak_ptr<Room>
  - move_to(std::shared_ptr<Room>) -> std::expected<void, Error>
  - enqueue_output(std::string_view) -> void
}

Notes
- Use fmt::format for all user-visible strings.
- Use ranges for iteration over containers where clear.

# 🛣️ Migration Strategy

## Core Principles

**🔄 Strangler Fig Pattern**
- Build new modules alongside legacy code
- Add adapter layers between old and new systems  
- Gradually migrate call sites without breaking functionality
- Remove legacy code only after full replacement

**📈 Incremental & Safe**
- Small, reviewable changes with comprehensive tests
- Preserve all existing gameplay functionality during migration
- Each milestone can be deployed independently
- Rollback capability at every step

## Implementation Phases

### Phase 1: Foundation (Weeks 1-2)
**Goal**: Modern C++ infrastructure

1. 🆕 **ID System Consolidation**: Replace multiple ID types (vnum/rnum/obj_num/room_num/zone_vnum) with unified EntityId system
2. ✅ **Core Utilities**: ID types, error handling, logging with spdlog
3. ✅ **Entity Skeletons**: Base classes with JSON serialization
4. ✅ **Testing Framework**: Catch2 setup with CI integration

**Success Criteria**: Foundation compiles, basic tests pass

#### 🔑 ID System Consolidation Details

**Current Problem**: Multiple overlapping ID systems create confusion and bugs:
- `typedef int room_num` - Real room numbers (array indices)  
- `typedef int obj_num` - Object prototype numbers
- `typedef int zone_vnum` - Zone virtual numbers
- `long id` - DG trigger global IDs
- Various VNUM/RNUM conversion macros (`GET_MOB_VNUM`, `IN_ROOM_VNUM`, etc.)

**Root Cause**: Legacy CircleMUD distinction between:
- **VNUM** (Virtual Number): World builder's ID in zone files  
- **RNUM** (Real Number): Runtime array index for fast lookup
- **ID**: Global unique identifier for scripting system

**Modern Solution**: Unified `EntityId` system with clear semantics:

```cpp
// Replace all ID types with this modern approach
class EntityId {
public:
    constexpr EntityId() : value_(0) {}
    constexpr explicit EntityId(std::uint64_t id) : value_(id) {}
    
    constexpr std::uint64_t value() const { return value_; }
    constexpr bool is_valid() const { return value_ != 0; }
    
    // Enable use in std::unordered_map
    friend constexpr bool operator==(EntityId a, EntityId b) = default;
    
private:
    std::uint64_t value_;
};

// Type-safe entity lookups
template<typename T>
class EntityRegistry {
public:
    std::optional<T*> find(EntityId id) const;
    std::expected<T*, RegistryError> get(EntityId id) const;
    EntityId register_entity(std::unique_ptr<T> entity);
};
```

**Migration Approach**:
1. **Phase 1**: Add `EntityId` alongside existing ID types - no breaking changes
2. **Phase 2**: Update APIs to accept both old and new ID types using adapter functions  
3. **Phase 3**: Migrate internal storage to use `EntityId` with backward compatibility
4. **Phase 4**: Remove legacy ID types once all references are updated

**Benefits**:
- ✅ **Type Safety**: Compiler prevents mixing room IDs with object IDs
- ✅ **Clarity**: Single concept instead of vnum/rnum/id confusion
- ✅ **Performance**: Direct hash table lookups replace array indexing
- ✅ **Scalability**: 64-bit IDs support massive worlds without wraparound

### Phase 2: Domain Models (Weeks 3-4)  
**Goal**: Modern game entity classes

5. ✅ **Object Hierarchy**: Weapons, armor, containers with modern APIs
6. ✅ **Actor System**: Player/Mob classes with stats and inventory
7. ✅ **Room Management**: Room class with occupancy and messaging

**Success Criteria**: Can create and manipulate game entities via modern APIs

### Phase 3: Game Loop (Weeks 5-6)
**Goal**: Multi-threaded architecture foundation

8. ✅ **World Class**: Registry, scheduling, command queuing
9. ✅ **GameLoop**: Single-threaded world updates with message passing
10. ✅ **Command Pipeline**: Parse input → queue → execute → output

**Success Criteria**: Simple commands work end-to-end through new architecture

### Phase 4: Core Gameplay (Weeks 7-10)
**Goal**: Essential game systems working

10. ✅ **Movement System**: Look, move, room transitions
11. ✅ **Inventory System**: Get, drop, equip, unequip  
12. ✅ **Communication**: Say, tell, channels
13. ✅ **Basic Combat**: Attack, defend, death

**Success Criteria**: Core gameplay loop functional through modern APIs

### Phase 5: Data & Persistence (Weeks 11-12)
**Goal**: Modern data handling

14. ✅ **JSON Persistence**: Save/load world data and players
15. ✅ **Migration Tools**: Convert legacy data formats
16. ✅ **Atomic Saves**: Crash-safe data persistence

**Success Criteria**: Game state persists reliably, data migrations work

# 🧪 Testing Strategy

## Test Categories

**Unit Tests (Catch2)**
```cpp
TEST_CASE("Entity creation and basic properties") {
    auto entity = Entity::create("sword", 1001);
    REQUIRE(entity.name() == "sword");
    REQUIRE(entity.vnum() == 1001);
}
```

**Integration Tests**
```cpp  
TEST_CASE("Command pipeline end-to-end") {
    auto world = create_test_world();
    auto player = create_test_player();
    
    auto result = execute_command(world, {"look", player.id()});
    REQUIRE(result.has_value());
    REQUIRE(!player.output_queue().empty());
}
```

**Game System Tests**
- Movement between rooms with proper messaging
- Inventory operations (get/drop/equip)
- Combat mechanics and damage calculation
- Player persistence and loading

## Test Infrastructure

**Fixtures & Test Data**
- Small JSON world (2-3 rooms, basic objects, test NPCs)
- Deterministic random number generation for reproducible tests
- Controllable game clock for time-dependent tests
- Memory leak detection and performance benchmarks

**Test Coverage Goals**
- 90%+ line coverage for new code
- 100% coverage of error handling paths
- Integration tests for all major game systems
- Performance regression testing

# 🎯 Detailed Milestones

## M1: Foundation (Week 1)
**Deliverables**
- `src/core/ids.hpp` - Unified EntityId system replacing vnum/rnum/obj_num/room_num/zone_vnum
- `src/core/result.hpp` - Error struct and Result<T> type alias  
- `src/core/logging.hpp` - spdlog integration with structured logging
- `src/core/legacy_id_adapters.hpp` - Compatibility layer for existing ID types
- Unit tests for all core utilities including ID conversion/compatibility

**Definition of Done**
- All code compiles without warnings
- Tests pass with 100% coverage
- spdlog logger works across modules
- Error types are well-documented
- EntityId system works alongside legacy ID types without breaking existing code
- Conversion functions between old and new ID systems tested and documented

## M2: Entity System (Week 2)  
**Deliverables**
- `src/domain/entity.hpp|.cpp` - Base Entity class with JSON support
- `src/domain/object.hpp|.cpp` - Object hierarchy with modern APIs
- `src/domain/actor.hpp|.cpp` - Actor base with stats/inventory stubs
- Comprehensive unit tests for all entity types

**Definition of Done**
- JSON serialization round-trip works perfectly
- Entity relationships properly modeled
- No dependencies on legacy global state
- Memory-safe with smart pointers

## M3: World Management (Week 3)
**Deliverables**  
- `src/world/room.hpp|.cpp` - Room class with occupancy management
- `src/world/world.hpp|.cpp` - World registries and basic API
- Unit tests for room operations and world indexing

**Definition of Done**
- Room add/remove operations work correctly
- World registries properly index entities by vnum/id
- Thread-safety considerations documented
- No threading implementation yet (single-threaded)

## M4: Game Loop Foundation (Week 4)
**Deliverables**
- `src/game/loop.hpp|.cpp` - GameLoop with command queue
- `src/game/commands.hpp|.cpp` - CommandRequest processing
- Integration test for echo command end-to-end

**Definition of Done**
- Commands can be posted to world thread
- Simple echo command works via new pipeline  
- Output properly routed back to players
- No gameplay functionality yet

## M5: Command Pipeline (Week 5)
**Deliverables**
- Modified `src/interpreter.cpp` - Bridge to new command system
- Adapter layer routing legacy commands through new pipeline
- All existing commands still work unchanged

**Definition of Done**
- Legacy command parsing uses new CommandRequest format
- All commands route through World::post()  
- Player behavior remains identical
- Performance unchanged or improved

# 🚀 Quick Reference

## Key API Patterns
```cpp
// Modern error handling
auto result = load_player("gandalf");
if (!result) {
    log_error("Failed to load player: {}", result.error().message);
    return;
}
auto& player = result.value();

// Range-based operations  
for (const auto& occupant : room.occupants()) {
    occupant.receive_message("Someone entered the room.");
}

// Smart pointer usage
auto sword = std::make_shared<Object>("rusty sword", 1001);
player.inventory().add_item(sword);

// JSON serialization
nlohmann::json j = player.to_json();
auto loaded_player = Player::from_json(j);
```

## File Organization
```
src/
├── core/           # Foundation: IDs, errors, logging
├── domain/         # Game entities: Entity, Object, Actor
├── world/          # World management: Room, Zone, World  
├── game/           # Game loop: commands, scheduling, events
├── net/            # Networking: sessions, I/O handling
├── persist/        # Data storage: JSON load/save
└── tests/          # Comprehensive test suite
```

## Command Implementation Template
```cpp
// In src/game/commands/movement.cpp
# FieryMUD Modernization Plan

## Executive Summary

This document provides a concrete, incremental roadmap for modernizing FieryMUD from a mixed-architecture C++ codebase to a more consistent and robust modern C++23 architecture. The plan emphasizes safety, testability, and maintainability while preserving all existing gameplay functionality during the transition.

**Core Objectives**: To evolve the existing codebase into a modular, testable, and maintainable system by leveraging and extending the modern C++ patterns already present in the code.

---

# 📋 Project Scope & Goals

## Objectives

✅ **Modernize Architecture**
- Evolve the legacy global-state design into a more modular, domain-driven architecture.
- Establish clear boundaries between networking, game logic, and persistence layers.
- Solidify the single-threaded world simulation with message-passing I/O.

✅ **Improve Code Quality**
- Replace remaining C-style patterns with modern C++23 equivalents.
- Expand test coverage using the existing Catch2 framework.
- Consistently apply RAII, smart pointers, and other modern C++ features.

✅ **Enable Future Development**
- Create stable, testable APIs for all core game systems.
- Establish clear patterns for safe, incremental feature development.
- Provide a solid foundation for future work, such as a full scripting overhaul.

## Non-Goals (This Phase)

❌ **New Game Features**: The focus is strictly on architectural modernization, not on adding new gameplay.
❌ **Complete Scripting Overhaul**: While adapters will be built for the existing DG Scripts, a full migration to a new system (like Lua) is out of scope for this phase.
❌ **Protocol Changes**: The network layer will be isolated, but the underlying Telnet protocol will not be changed.

---

# 🏗️ Target Architecture

The target architecture is already partially realized in the existing codebase. This plan aims to complete the transition to a fully modular system.

## Module Organization

| Module      | Purpose                | Key Components                                      |
|-------------|------------------------|-----------------------------------------------------|
| **Core**    | Foundation utilities   | IDs, time, logging, error handling, constants       |
| **Domain**  | Game entities          | Entity base, Object/Actor hierarchies, stats, inventory |
| **World**     | Game world management  | World, Zone, Room, weather, registries, spawning    |
| **GameLoop**  | Simulation engine      | World tick, scheduling, event dispatch, combat      |
| **Net**       | Network layer          | Sessions, telnet, input parsing, command requests   |
| **Commands**  | Command processing     | Parsing, routing, execution in the world thread     |
| **Persistence**| Data storage           | JSON load/save for world data and players           |
| **Logging**   | Diagnostics            | Structured logging, metrics, debugging              |

## Concurrency Model

The plan will enforce the existing concurrency model:

-   **Single-Threaded Game World**: A single thread (the "world strand") will be responsible for all mutations to the game state. This eliminates a large class of concurrency bugs and makes testing more reliable.
-   **Multi-Threaded I/O**: I/O operations, primarily networking, will be handled by a separate pool of threads. These threads will communicate with the world strand via a message queue and will never mutate the world state directly.

---

# 🛣️ Migration and Testing Strategy

This project will follow an **evolutionary modernization** strategy. The codebase is not a pure legacy C application, but rather a mixed-architecture system with significant modern C++17/20/23 features. The clan system, for example, is a perfect implementation of modern C++ patterns.

Our strategy is to **extend, not replace**. We will leverage the existing modern patterns and gradually refactor the legacy parts of the codebase to match.

## Core Principles

-   **The Strangler Fig Pattern**: We will build new, modern modules alongside the legacy code. Adapters will bridge the old and new systems, allowing us to migrate call sites incrementally. Legacy code will only be removed once it is no longer referenced.
-   **Incremental and Safe**: All changes will be small, reviewable, and covered by tests. We will preserve all existing gameplay functionality during the migration. Each milestone will be independently deployable, and we will have the ability to roll back at every step.

## The Testing Safety Net

Before we begin, we will create a robust "testing safety net" to ensure that we don't introduce regressions.

-   **Characterization Tests**: We will write a suite of tests that capture the current behavior of the system, *including its bugs*. These tests will not make assertions about what the code *should* do, but rather what it *does*. This will give us a baseline against which to compare our changes.
-   **Golden Master Testing**: For a text-based game like a MUD, we can create "golden master" logs of game sessions. These logs will be generated by running a series of commands through the existing system. We will then run the same series of commands through the modernized system and compare the output. Any differences will be flagged as potential regressions.

## DG Script Migration

The existing DG Scripts are a critical part of the game's functionality. A full migration to a new scripting system is out of scope for this phase, but we will ensure that the existing scripts continue to function.

-   **Adapters**: We will create an adapter layer that allows the existing DG Scripts to interact with the modernized game systems. This will involve creating shims that translate between the old and new APIs.
-   **Testing**: The "golden master" tests will be crucial for verifying that the DG Scripts continue to behave as expected.

## Performance Benchmarking

Performance is a critical consideration. We will establish a suite of performance benchmarks that measure key metrics, suchs as server startup time, command processing latency, and memory usage. These benchmarks will be run as part of our continuous integration process to ensure that we don't introduce performance regressions.

---

# 🗺️ Implementation Roadmap

The project will be broken down into a series of tasks. Each task is designed to be a small, incremental change that can be completed and tested independently.

**Task T1: Establish Testing Safety Net**
-   **Description**: Create the characterization and golden master tests that will form our testing safety net.
-   **DoD**: A suite of tests exists that captures the current behavior of the system.

**Task T2: Core Utilities Consolidation**
-   **Description**: Consolidate the various ID types (vnum, rnum, etc.) into a single, unified `EntityId` system. Refine the core error handling and logging utilities.
-   **DoD**: The `EntityId` system is in place, and legacy ID types are handled via adapters.

**Task T3: Create Entity Adapters**
-   **Description**: Create `CharacterAdapter` and `ObjectAdapter` classes that wrap the legacy `CharData` and `ObjData` structs with a modern C++ interface.
-   **DoD**: The adapter classes are implemented and tested.

**Task T4: String Modernization in Core Structures**
-   **Description**: Replace `char*` with `std::string` in the core data structures, starting with `ObjData`.
-   **DoD**: The core data structures use `std::string` for all text fields.

**Task T5: Smart Pointer Integration**
-   **Description**: Add `std::shared_ptr` equivalents alongside the raw pointers in the core data structures.
-   **DoD**: The core data structures support both raw and smart pointers, with a clear path for migrating to smart pointers exclusively.

**Task T6: Game Loop and Command Pipeline**
-   **Description**: Implement the single-threaded game loop with a command queue. Bridge the legacy command interpreter to the new command pipeline.
-   **DoD**: The new game loop is in place, and legacy commands are routed through the new pipeline.

**Task T7: Movement System Migration**
-   **Description**: Migrate the core movement commands (`look`, `move`, etc.) to the new command pipeline.
-   **DoD**: The movement system is fully migrated and tested.

**Task T8: Inventory System Migration**
-   **Description**: Migrate the core inventory commands (`get`, `drop`, `equip`, etc.) to the new command pipeline.
-   **DoD**: The inventory system is fully migrated and tested.

**Task T9: Communication System Migration**
-   **Description**: Migrate the core communication commands (`say`, `tell`, etc.) to the new command pipeline.
-   **DoD**: The communication system is fully migrated and tested.

**Task T10: Basic Combat Migration**
-   **Description**: Migrate the basic combat system to the new command pipeline.
--   **DoD**: The basic combat system is fully migrated and tested.

**Task T11: JSON Persistence**
-   **Description**: Implement the JSON-based persistence layer for world data and players.
-   **DoD**: The new persistence layer is in place and tested.

---

# 📚 Appendices

The following sections contain detailed technical specifications, future plans, and other reference material.

## Appendix A: Quick Reference

### Key API Patterns
```cpp
// Modern error handling
auto result = load_player("gandalf");
if (!result) {
    log_error("Failed to load player: {}", result.error().message);
    return;
}
auto& player = result.value();

// Range-based operations  
for (const auto& occupant : room.occupants()) {
    occupant.receive_message("Someone entered the room.");
}

// Smart pointer usage
auto sword = std::make_shared<Object>("rusty sword", 1001);
player.inventory().add_item(sword);

// JSON serialization
nlohmann::json j = player.to_json();
auto loaded_player = Player::from_json(j);
```

### File Organization
```
src/
├── core/           # Foundation: IDs, errors, logging
├── domain/         # Game entities: Entity, Object, Actor
├── world/          # World management: Room, Zone, World  
├── game/           # Game loop: commands, scheduling, events
├── net/            # Networking: sessions, I/O handling
├── persist/        # Data storage: JSON load/save
└── tests/          # Comprehensive test suite
```

## Appendix B: Coding Standards

-   **Strings**: `std::string_view` for parameters, `std::string` for owned strings; `fmt::format` for all string formatting.
-   **Containers**: `std::vector`, `std::array`, `std::span`; `std::unordered_map` for registries.
-   **Memory**: Smart pointers (`std::unique_ptr`, `std::shared_ptr`); RAII for all resources; no raw `new` or `delete`.
-   **Algorithms**: `std::ranges` and range-based for loops should be preferred over manual loops.
-   **Error Handling**: `std::expected` for operations that can fail; `std::optional` for nullable values.
-   **Enums**: `magic_enum` for all enum-to-string conversions.
-   **JSON**: `nlohmann/json` for all JSON processing.

## Appendix C: Risks and Mitigations

-   **Risk**: Hidden global state interfering with world invariants.
    -   **Mitigation**: Isolate mutations behind the `World` API. Flag and replace global variables on a case-by-case basis.
-   **Risk**: Threading bugs.
    -   **Mitigation**: Strictly enforce the single-threaded world strand model. Use thread-safe queues for all inter-thread communication.
-   **Risk**: Migration scope creep.
    -   **Mitigation**: Adhere strictly to the "Non-Goals" for this phase. Keep pull requests small and focused.

## Appendix D: Future Work

The following sections are provided for future reference and are not part of the core modernization phase.

### JSON Persistence Plan
(The detailed JSON persistence plan from the original document would go here.)

### Scripting Modernization: Lua
(The detailed Lua scripting plan from the original document would go here.)

```

---

# 💡 Recommendations Based on Codebase Analysis

## Critical Observations

**Existing Structure Analysis** (Updated after deep analysis):
- ~150 source files with **extensive modern C++17/20/23 usage**
- **Mixed architecture**: Legacy C structs + modern C++ patterns coexisting
- **Clan system demonstrates perfect modern C++23**: `std::expected`, smart pointers, type-safe enums
- **String utilities are fully modern**: `std::string_view`, templates, perfect forwarding
- **Function registration system is exemplary**: variadic templates, type erasure
- Global state management **but** with modern containers (`std::unordered_map`)

**Key Discovery**: This is **evolutionary modernization**, not revolutionary rewrite

## Revised Strategy: Evolutionary Not Revolutionary

### E1: Leverage Existing Modern Patterns ⚡ **IMMEDIATE**
**Why**: Clan system, Arguments, and Function Registration already demonstrate perfect C++23

**Action Items**:
- **Copy clan system patterns**: `std::expected` error handling throughout codebase
- **Extend Arguments class**: Add entity parsing (Player, Object lookups)  
- **Enhance Function Registration**: Add entity lifecycle commands using existing system
- **Adopt smart pointer patterns**: Follow clan system's shared_ptr usage

### E2: Field-by-Field Structure Evolution ⚡ **IMMEDIATE**  
**Why**: CharData/ObjData are mixed modern/legacy, not pure legacy

**Strategy - Don't Replace, Evolve**:
```cpp
struct CharData {
    // Keep existing fields for compatibility
    long id;
    char *namelist;  // Legacy
    
    // Add modern equivalents alongside  
    std::string name_modern;        // New
    std::shared_ptr<CharData> next_shared;  // Alongside CharData* next
    
    std::vector<SpellCast> spellcasts;  // ✅ Already modern!
};
```

### E3: Adapter Pattern Implementation
**Why**: Bridge existing code with modern interfaces gradually

**Create Adapter Classes**:
- `CharacterAdapter` wrapping `CharData*` with modern interface
- `ObjectAdapter` wrapping `ObjData*` with modern interface  
- `RoomAdapter` wrapping `RoomData*` (already mostly modern)
- **Benefit**: Use modern APIs while legacy code continues working

### E4: String Modernization Priority
**Why**: String handling is partially modern already

**Target Areas**:
- Replace `char* name` with `std::string` in ObjData (easy win)
- Leverage existing `std::string_view` utilities from string_utils.hpp
- Extend existing string parsing for command processing

### E5: Smart Pointer Migration  
**Why**: Clan system shows the pattern, just extend it

**Migration Path**:
- Add `std::shared_ptr` equivalents alongside raw pointers
- Gradually migrate relationship management (following, group, etc.)
- Use RAII patterns from existing modern code

---

# 📋 Ready-to-Implement Tasks (Revised Evolutionary Approach)

## Task E1: Extend Existing Modern Patterns ⚡
**Context**: Leverage clan system's perfect C++23 patterns for entity management
**Files**: `src/entities/{character_adapter.hpp, object_adapter.hpp}`, extend `src/clan.hpp` patterns
**Dependencies**: None - builds on existing code
**Estimated Time**: 6-8 hours  

**Implementation Steps**:
1. Copy `std::expected` error patterns from clan.hpp to new entity utilities
2. Create `CharacterAdapter` class wrapping `CharData*` with modern interface
3. Create `ObjectAdapter` class wrapping `ObjData*` with modern interface  
4. Add entity lookup functions using existing smart pointer patterns
5. Extend Arguments class to parse entity references (players, objects)

**Definition of Done**:
✅ Adapters provide clean modern interface to legacy structures
✅ Error handling follows clan system patterns (`std::expected`)
✅ Can look up players/objects using modern APIs
✅ Arguments class can parse "get sword" → ObjectAdapter
✅ Zero changes to existing legacy code

## Task E2: String Modernization in ObjData ⚡
**Context**: Replace char* with std::string in ObjData structure  
**Files**: `src/objects.hpp`, `src/objects.cpp`, related object functions
**Dependencies**: None - direct improvement
**Estimated Time**: 4-6 hours

**Implementation Steps**:
1. Replace `char *name, *description, *short_description` with `std::string`
2. Update all object creation/loading functions to use std::string
3. Leverage existing string utilities from `src/string_utils.hpp`
4. Update object save/load to use modern string handling
5. Test with existing object files to ensure compatibility

**Definition of Done**:  
✅ ObjData uses std::string for all text fields
✅ No memory leaks from old char* management  
✅ Existing object files load correctly
✅ String utilities integrated throughout object code
✅ Performance maintained or improved

## Task E3: Smart Pointer Integration ⚡  
**Context**: Add smart pointers alongside existing raw pointers for gradual migration
**Files**: `src/structs.hpp`, `src/characters.hpp`, related character functions
**Dependencies**: Task E1 (Adapter patterns established)
**Estimated Time**: 8-12 hours

**Implementation Steps**:
1. Add `std::shared_ptr<CharData> next_shared` alongside `CharData *next`
2. Add smart pointer relationship management (following, group, etc.)
3. Create utility functions for converting raw→smart and smart→raw pointers
4. Update relationship creation to use both systems during transition
5. Add RAII cleanup patterns following clan system examples

**Definition of Done**:
✅ CharData has both raw and smart pointer versions of relationships
✅ New code can use smart pointers exclusively  
✅ Legacy code continues working unchanged
✅ Memory leaks eliminated in new relationship code
✅ Follows clan system's smart pointer patterns exactly

## Task E4: Enhanced Command Integration
**Context**: Extend existing function registration system with entity commands  
**Files**: `src/function_registration.hpp`, `src/commands/entity_commands.cpp` (new)
**Dependencies**: Tasks E1-E2 (Adapters and string modernization)
**Estimated Time**: 6-10 hours

**Implementation Steps**:
1. Study existing function registration system (already perfect!)
2. Add entity manipulation commands using existing registration patterns
3. Integrate CharacterAdapter/ObjectAdapter with command system
4. Add error handling using `std::expected` patterns from clan system
5. Create entity-specific commands (look, get, drop, etc.) via modern APIs

**Definition of Done**:
✅ New entity commands work through existing function registration
✅ Commands use modern adapters instead of raw legacy structs
✅ Error handling consistent with clan system patterns  
✅ Command abbreviation and fuzzy matching preserved
✅ Legacy commands continue working unchanged

---

## Summary of Revised Evolutionary Approach

### 🎯 **Key Strategy Change**
- **Before**: Complete architectural replacement
- **After**: Evolutionary modernization building on existing patterns

### ⚡ **Immediate Benefits**  
- **Task E1**: Modern APIs available immediately via adapters
- **Task E2**: Memory safety improvements with zero risk
- **Task E3**: Smart pointer benefits with legacy compatibility
- **Task E4**: Enhanced commands through existing proven system

### 🔄 **Migration Philosophy**
1. **Extend, Don't Replace**: Add modern alongside legacy
2. **Copy Proven Patterns**: Clan system shows perfect C++23 usage
3. **Gradual Transition**: Provide both interfaces during migration
4. **Zero Disruption**: Legacy code continues working unchanged

### 📊 **Risk Reduction**  
- **Binary Compatibility**: Maintained throughout transition
- **Rollback Capability**: Can remove modern additions if needed  
- **Incremental Testing**: Each task independently testable
- **Community Continuity**: No disruption to existing development

### 🚀 **Implementation Timeline**
- **Week 1**: Tasks E1-E2 (Adapters + String modernization) 
- **Week 2**: Tasks E3-E4 (Smart pointers + Command integration)
- **Week 3+**: Continue pattern extension to remaining systems

This evolutionary approach leverages the significant modern C++ already present in FieryMUD, providing immediate benefits while maintaining full compatibility with the existing mature codebase.

## Task A4: Game Loop & Command Pipeline
**Context**: Single-threaded game loop with command queuing
**Files**: `src/game/{loop.hpp, commands.hpp}`, modified `src/interpreter.cpp`
**Dependencies**: Task A3 (World management)  
**Estimated Time**: 12-16 hours

**Implementation Steps**:
1. Implement `GameLoop` with command queue and world strand
2. Create `CommandRequest` struct and processing pipeline
3. Add adapter in `interpreter.cpp` to bridge legacy commands
4. Implement simple echo/look commands as proof of concept
5. Write integration tests for end-to-end command flow

**Definition of Done**:
✅ Commands route through new pipeline while preserving behavior
✅ Simple commands (look, echo) work end-to-end  
✅ No gameplay regressions in existing functionality
✅ Performance matches or exceeds current system

---

# 📚 Additional Implementation Guides

The following sections contain detailed technical specifications, extended milestones, and advanced features like Lua scripting and HTTP admin interfaces. These are provided for future reference and can be implemented after the core modernization is complete.

*[Note: The remaining sections of the original plan contain detailed specifications for persistence systems, Lua integration, HTTP admin interfaces, and extended task breakdowns. These remain available for reference but are not part of the core modernization phase.]*

---

## Summary

This modernized PLAN.md provides:
- **Clear executive summary** and scope definition
- **Visual organization** with headers, emojis, and tables
- **Concrete implementation tasks** with time estimates and dependencies  
- **Modern C++ code examples** showing target patterns
- **Prioritized recommendations** based on existing codebase analysis
- **Ready-to-implement task breakdown** for immediate action

The plan maintains all technical depth while significantly improving readability and actionability for both human developers and LLM implementation.

Task F: GameLoop and queues
- Files: src/game/loop.hpp/cpp, src/game/commands.hpp/cpp
- Steps: world strand, post/schedule, tick; CommandRequest and execute_command stub
- DoD: integration test posts a command and receives echo output.

Task G: Interpreter bridge
- Files: src/interpreter.cpp (adapter), src/game/commands.cpp
- Steps: parse verb/args -> CommandRequest -> post to World
- DoD: legacy commands route via new pipeline while preserving behavior.

Task H: Movement slice
- Files: src/game/commands_move.cpp (new), tests/movement_tests.cpp
- Steps: implement look/move; room enter/leave messages
- DoD: tests passing; manual smoke ok.

Task I: Inventory basics
- Files: src/game/commands_inventory.cpp, tests/inventory_tests.cpp
- Steps: get/drop/equip/unequip
- DoD: tests passing; no leaks; ranges used for iteration.

## 12) Coding Standards (must follow)

- Strings: std::string_view for params, std::string for owned; fmt::format.
- Containers: std::vector/array/span; unordered_map for registries.
- Memory: smart pointers; RAII; no raw new/delete.
- Algorithms: std::ranges; prefer transform/filter over manual loops.
- Errors: std::expected/std::optional; no error codes.
- Enums: magic_enum for conversions.
- JSON: nlohmann/json only.
- CLI/tests/logging libs as in repo guidelines.

## 13) Risks and Mitigations

- Risk: hidden global state interfering with world invariants
  - Mitigation: isolate mutations behind World APIs; flag and replace globals milestone-by-milestone.
- Risk: threading bugs
  - Mitigation: strict strand ownership; assert thread affinity in debug builds.
- Risk: migration scope creep
  - Mitigation: ship vertical slices; keep PRs small; protect milestones.

## 14) Acceptance Criteria Summary

- Builds cleanly with CMake; lints where applicable.
- Unit and integration tests for each milestone; deterministic world tests.
- Movement and basic interactions work via new pipeline before removing legacy code.

## 15) Quick Implementation Prompts (for LLM usage)

- “Create src/core/result.hpp with Error and Result<T>=std::expected<T,Error>, plus minimal unit test.”
- “Add src/domain/entity.{hpp,cpp} with id/vnum/name/keywords/desc and JSON adapters. Write Catch2 tests for round-trip.”
- “Implement src/world/world.{hpp,cpp} with registries and get_room(vnum). Include add/remove actor/object, with tests.”
- “Wire a single-threaded GameLoop with post/schedule and an echo command integration test.”
- “Port the ‘look’ and ‘move’ commands end-to-end using new classes, keeping behavior identical.”

## 16) Next Steps (immediate)

1) Land M1 (Core + Logging).
2) Land M2 (Entity/Object/Actor skeletons) with unit tests and JSON fixtures.
3) Prepare M3 (Room/World) and stub the GameLoop entry points for routing.

## 17) JSON Persistence Plan (files now, DB later)

Goals
- Use JSON files initially for areas/world data and player saves.
- Hide storage behind repository interfaces so a DB backend can be dropped in later without touching game logic.
- Ensure safe writes (atomic, crash-safe) and clear versioning for smooth migrations.

Abstractions (interfaces)
- IWorldStore
  - load_indices() -> Result<WorldIndex>
  - load_zone(vnum) -> Result<ZoneData>
  - load_room(vnum) -> Result<RoomData>
  - load_object_proto(vnum) -> Result<ObjectProto>
  - save_zone(ZoneData) -> Result<void>
  - save_room(RoomData) -> Result<void>
- IPlayerStore
  - load_player(std::string_view name) -> Result<PlayerData>
  - save_player(const PlayerData&) -> Result<void>
  - list_players() -> Result<std::vector<std::string>>
- Implementations: JsonWorldStore, JsonPlayerStore (later: DbWorldStore, DbPlayerStore)

File layout (configurable root, defaults shown)
- data_root: configurable via CLI/env; default to detected repo path `lib/` (or external runtime directory).
- World content (static-ish)
  - {data_root}/json/world/index.json  // global indices
  - {data_root}/json/world/zones/{zone_vnum}.zone.json
  - {data_root}/json/world/rooms/{room_vnum}.room.json
  - {data_root}/json/world/objects/{vnum}.object.json
- Player data (dynamic)
  - {data_root}/json/players/{first_letter}/{player_name}.player.json
- Backups (optional)
  - {data_root}/json/.backup/YYYYMMDD-HHMM/{mirrors of above}

Indices
- world/index.json example
  {
    "version": 1,
    "zones": [ { "vnum": 1, "path": "zones/1.zone.json" }, ...],
    "rooms": [ { "vnum": 3001, "path": "rooms/3001.room.json" }, ...],
    "objects": [ { "vnum": 1001, "path": "objects/1001.object.json" } ]
  }
- At startup, load indices only; lazy-load entities on demand if needed.

Schema (drafts; keep minimal and explicit; evolve via version field)
- Common envelope
  - { "schema": { "name": "room|zone|object|player", "version": 1 }, "data": { ... } }
- Room
  - data: { vnum:int, name:string, desc:string, exits:[{dir:"north", to:int}], flags:["dark","indoors"], contents:[vnum], actors:[] }
- Object
  - data: { vnum:int, name:string, keywords:[string], type:"weapon|armor|container|...", weight:int, values:[int], flags:[string] }
- Player
  - data: { name:string, entity_id:uint64, stats:{hp:int, mana:int, ...}, inventory:[object_instance], room:int, perms:[string], prefs:{...} }
- Note: Separate object prototypes (by vnum) from instances (owned by players/rooms) which add durability, timer, etc.

Versioning and migrations
- Each file carries {schema.version}; breaking changes bump version.
- Provide migration helpers to upgrade older versions at load time into current in-memory model.
- Keep migrations idempotent and covered by tests.

Atomic writes and durability
- Write to a temporary file next to the target (e.g., `.tmp` suffix), flush, fsync, then std::filesystem::rename to final path.
- fsync directory after rename on POSIX to ensure directory entry durability.
- Optionally write timestamped backup before overwrite if configured.

Performance and caching
- Load indices on startup; load full zone files on first access; retain in-memory caches in World.
- Serialize saves on world strand to avoid races; batch or throttle large save operations.

Security and trust
- Treat JSON as untrusted: validate required fields and value ranges; reject unknown schema names/versions.
- Never evaluate scripts embedded in JSON; scripting remains separate.

DB migration path
- Game code talks only to IWorldStore/IPlayerStore.
- Swap Json*Store for Db*Store implementing the same interface.
- Maintain vnum as stable content key; map to DB primary keys internally.
- Keep entity_id generation internal to runtime for now; DB may assign its own ids later; vnums remain external stable references.

Testing
- Provide small JSON fixtures under tests/fixtures/json/... matching schemas above.
- Unit tests for adapters: parse-valid, parse-invalid, round-trip, migration from older schema.
- Integration tests: save/load a mini world and a player; ensure referential integrity (room->objects, exits target rooms).

## 18) Additional Milestones (Persistence)

M8: JSON stores and schemas (read path)
- Deliver: JsonWorldStore/JsonPlayerStore (load-only), indices loader, schema structs, fixtures.
- DoD: can load a tiny world and a test player into memory; tests cover happy + invalid cases.

M9: Safe save path (atomic writes)
- Deliver: save implementations with temp+rename+fsync; backups optional.
- DoD: tests simulate partial writes (via temp files); final files remain valid.

M10: Interpreter save hooks
- Deliver: autosave intervals and explicit save command using IPlayerStore; world snapshot save for admin command.
- DoD: tests verify no cross-thread mutations; saves occur on world strand.

## 19) Additional Tasks (Persistence)

Task J: Define schema structs and adapters
- Files: src/persist/schemas.hpp, src/persist/json_adapters.{hpp,cpp}, tests/persist_schema_tests.cpp
- Steps: define DTOs for RoomData/ObjectProto/PlayerData with nlohmann/json to/from; version field; validations.
- DoD: fixtures parse and round-trip; invalid inputs rejected with informative Error.

Task K: JsonWorldStore
- Files: src/persist/json_world_store.{hpp,cpp}, tests/json_world_store_tests.cpp
- Steps: implement load_indices, load_zone/room/object_proto; configurable data root; path helpers; caching policy.
- DoD: loads tiny world fixture; missing files return Error::NotFound.

Task L: JsonPlayerStore (atomic saves)
- Files: src/persist/json_player_store.{hpp,cpp}, tests/json_player_store_tests.cpp
- Steps: implement load/save player; atomic write with temp+rename+fsync; optional backups; directory sharding by first letter.
- DoD: save then load matches; interrupted write leaves previous version intact.

## 20) Scripting Modernization: Lua (coexist with current, then migrate)

Goals
- Add Lua scripting alongside the current framework, then migrate incrementally.
- Provide a safe, deterministic, hot-reloadable scripting environment that runs on the world strand.
- Keep script attachment declarative via JSON; minimize game code coupling.

Choices
- Runtime: Lua 5.4
- Binding: sol2 (friendly C++ API; header-only; fits C++23 style). Fallback: Lua C API if needed.
- Execution: always on the world strand to preserve world invariants.

Abstractions
- IScriptEngine
  - load_script(std::string_view name, std::string_view source_or_path) -> Result<void>
  - unload_script(std::string_view name) -> Result<void>
  - call(std::string_view fn, const ScriptContext&, const json& args) -> Result<json>
  - reload_all() -> Result<void>
  - register_native(std::string_view name, NativeFn) -> void
- IScriptHost (provided by game)
  - bind_world_api(IScriptEngine&) -> void  // registers functions/types exposed to scripts
  - make_context(World&, optional<Player>, optional<Room>, optional<Object>) -> ScriptContext
- Implementation: LuaScriptEngine (sol2 + sandbox), later Db-backed or other engines if desired.

Event model and mapping
- Unified events (first wave): on_tick, on_enter(room, actor), on_leave(room, actor), on_speech(actor, text), on_use(actor, object), on_pickup/on_drop, on_combat_tick(actor, target), on_death(actor)
- Map existing DG-style triggers to unified events via an adapter; both systems can fire during migration.

Script packaging & discovery
- Root (configurable): {data_root}/scripts/lua/
  - rooms/{name}.lua, mobs/{name}.lua, objects/{name}.lua, world/{name}.lua
- JSON schema references scripts by logical name, not absolute path (engine resolves):
  - RoomData: { ..., "scripts": [{"event":"on_enter", "name":"rooms/welcome"}] }
  - MobData/ObjectProto similar

Safety and sandboxing
- Only enable safe libs: base, table, string, math; disable io, os, package.loadlib, debug.
- Use a custom allocator to cap memory per VM or per script group.
- Instruction limit: lua_sethook to yield/abort long-running scripts (wall-clock budget per call; e.g., 2–5ms).
- No blocking I/O; provide schedule(ms, fn) to defer via world loop.
- Validate and sanitize all inbound JSON args; never expose raw pointers.

Threading and performance
- All calls executed on the world strand; expose async via schedule() and timers.
- Cache loaded scripts/compiled chunks; reload only on change or via admin command.
- Optional: bytecode cache if needed later.

Error handling and logging
- Wrap errors into Error{Code::ScriptError, msg}; include script name, event, stack trace.
- Log via spdlog with fields: script, event, actor, room, zone.
- Scripts can log with log_info/debug/warn exposed by the host.

Bindings (initial surface)
- log_info(msg), log_warn(msg), log_debug(msg)
- player_send(player, msg), room_broadcast(room, msg)
- get_room(vnum)->RoomRef, move_actor(actor, room)
- schedule(ms, fn)
- random(min,max)
- Accessors for simple fields (name, vnum, tags), with immutable views where possible

Minimal example (rooms/welcome.lua)
```
function on_enter(ctx)
  local player = ctx.actor
  if player and player:is_player() then
    player:send(string.format("Welcome to %s!", ctx.room:name()))
  end
end
```

Testing
- Unit: run Lua functions in isolation with a fake context; assert returned json or side effects on fakes.
- Integration: fire on_enter when a player moves; ensure message delivered.
- Negative: infinite loop/alloc-heavy script must abort within budget and produce an error.

Migration strategy
- Phase 1: Introduce engine and bindings; no production scripts run by default.
- Phase 2: Dual-run for select events (e.g., on_enter); compare outputs in logs; add a feature flag per zone/room.
- Phase 3: Port critical scripts category-by-category; remove DG trigger equivalents once parity is verified.

## 21) Lua Work Milestones

M11: Embed Lua + sol2 and sandbox
- Deliver: LuaScriptEngine with safe libs, instruction limit, basic logging bindings.
- DoD: unit tests run a simple script; bad scripts fail safely; no io/os access.

M12: Event bridge (enter/speech)
- Deliver: map on_enter/on_speech from world events to Lua calls; context plumbing; basic helpers (player_send, room_broadcast).
- DoD: integration tests prove events fire and messages arrive.

M13: JSON attachment
- Deliver: extend schemas to allow scripts array per entity; loader resolves and registers scripts.
- DoD: room JSON with on_enter script runs on player entry.

M14: Hot reload + limits
- Deliver: admin command to reload scripts; file-watcher optional; memory and instruction budgets configurable.
- DoD: reload updates behavior without restart; enforced limits covered by tests.

M15: Migration sample
- Deliver: port one representative DG script to Lua; adapter dual-runs for comparison.
- DoD: outputs match; flag flips to Lua-only for that script.

## 22) Lua Tasks (LLM-ready)

Task M11a: LuaScriptEngine scaffolding
- Files: src/script/{engine.hpp,lua_engine.{hpp,cpp}}, tests/script_engine_tests.cpp
- Steps: create IScriptEngine; implement LuaScriptEngine with sandbox, register log_*; unit tests for safe/unsafe code.
- DoD: runs simple function; unsafe libs are unavailable; errors surfaced as Error::ScriptError.

Task M11b: Host bindings
- Files: src/script/host_bindings.{hpp,cpp}
- Steps: expose log_*, random, schedule; wrap Room/Actor/Player handles with safe proxies; assert world strand.
- DoD: basic functions callable; schedule posts back to loop.

Task M12a: Event adapter
- Files: src/game/script_events.{hpp,cpp}
- Steps: translate world events to script function names; build ScriptContext; call engine->call.
- DoD: on_enter/on_speech integration tests pass.

Task M13a: Schema & loader updates
- Files: src/persist/schemas.hpp, src/persist/json_adapters.{hpp,cpp}
- Steps: add scripts: [ {event:string, name:string} ] to RoomData/MobData/ObjectProto; loader resolves names.
- DoD: fixtures parse; scripts registered with engine on load.

Task M14a: Hot reload & limits
- Files: src/script/lua_engine.cpp, src/commands/admin_scripts.cpp, tests/script_reload_tests.cpp
- Steps: reload_all(), bounded allocator, instruction budget config; admin command `scripts reload`.
- DoD: reload works; limits enforced.
