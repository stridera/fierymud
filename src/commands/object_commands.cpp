#include "object_commands.hpp"
#include "../core/ability_executor.hpp"
#include "../core/actor.hpp"
#include "../core/object.hpp"
#include "../core/logging.hpp"
#include "../core/money.hpp"
#include "../core/shopkeeper.hpp"
#include "../scripting/trigger_manager.hpp"
#include "../world/room.hpp"
#include "../world/world_manager.hpp"

#include <algorithm>

namespace ObjectCommands {

namespace {

/**
 * Handle picking up an object - if it's money (coins), add to player wealth
 * and destroy the object. Otherwise, add to inventory normally.
 *
 * @param ctx Command context
 * @param obj The object being picked up
 * @param gotten_names Output vector for item names (for display)
 * @return true if pickup was successful, false otherwise
 */
bool handle_pickup(const CommandContext &ctx, std::shared_ptr<Object> obj,
                   std::vector<std::string>* gotten_names = nullptr) {
    if (!obj) return false;

    // Check if this is a money/coins object
    if (obj->type() == ObjectType::Money) {
        // Get the coin value
        long coin_value = obj->value();
        if (coin_value > 0) {
            // Add to player's wealth
            ctx.actor->give_wealth(coin_value);

            // Format coin description
            auto coins = fiery::Money::from_copper(coin_value);
            std::string coin_desc = coins.to_string();

            if (gotten_names) {
                gotten_names->push_back(coin_desc);
            } else {
                ctx.send_success(fmt::format("You get {}.", coin_desc));
                ctx.send_to_room(fmt::format("{} picks up some coins.", ctx.actor->display_name()), true);
            }
        }
        // Object is consumed (not added to inventory)
        return true;
    }

    // Normal item - add to inventory
    auto add_result = ctx.actor->inventory().add_item(obj);
    if (add_result) {
        if (gotten_names) {
            gotten_names->push_back(ctx.format_object_name(obj));
        }
        return true;
    }
    return false;
}

} // anonymous namespace

// =============================================================================
// Object Commands
// =============================================================================

Result<CommandResult> cmd_get(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("get <object>|all [from <container>]");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    std::shared_ptr<Object> target_object = nullptr;
    std::shared_ptr<Object> source_container = nullptr;

    // Determine container name - supports both "get X from Y" and "get all Y" syntax
    // "get all corpse" should get all items FROM the corpse (same as "get all from corpse")
    // "get all" with no second arg gets from room
    // "get all.corpse" gets all corpses from room (handled in room section via dot-notation)
    std::string_view container_name;
    bool is_container_syntax = false;

    if (ctx.arg_count() >= 3 && ctx.arg(1) == "from") {
        // "get X from container" syntax
        container_name = ctx.arg(2);
        is_container_syntax = true;
    } else if (ctx.arg_count() == 2 && ctx.arg(0) == "all") {
        // "get all container" syntax - treat as "get all from container"
        container_name = ctx.arg(1);
        is_container_syntax = true;
    }

    // Check if this is "get <object> from <container>" syntax
    if (is_container_syntax) {
        // Get from container
        auto inventory_items = ctx.actor->inventory().get_all_items();

        // First check inventory for the object
        std::shared_ptr<Object> potential_container = nullptr;
        for (const auto &obj : inventory_items) {
            if (obj && obj->matches_keyword(container_name)) {
                potential_container = obj;
                if (obj->is_container()) {
                    source_container = obj;
                }
                break;
            }
        }

        // If not found in inventory, check room
        if (!potential_container) {
            auto &room_objects = ctx.room->contents_mutable().objects;
            for (auto &obj : room_objects) {
                if (obj && obj->matches_keyword(container_name)) {
                    potential_container = obj;
                    if (obj->is_container()) {
                        source_container = obj;
                    }
                    break;
                }
            }
        }

        if (!potential_container) {
            ctx.send_error(fmt::format("You don't see a container called '{}' here.", container_name));
            return CommandResult::InvalidTarget;
        }

        if (!source_container) {
            ctx.send_error(fmt::format("You can't get anything from {} - it's not a container.", ctx.format_object_name(potential_container)));
            return CommandResult::InvalidTarget;
        }

        // Check container properties
        const auto &container_info = source_container->container_info();

        // Check if container is closed
        if (container_info.closeable && container_info.closed) {
            ctx.send_error(fmt::format("The {} is closed.", ctx.format_object_name(source_container)));
            return CommandResult::InvalidState;
        }

        // Check if container is locked
        if (container_info.lockable && container_info.locked) {
            ctx.send_error(fmt::format("The {} is locked.", ctx.format_object_name(source_container)));
            return CommandResult::InvalidState;
        }

        // Cast to Container to access container functionality
        auto container_obj = std::dynamic_pointer_cast<Container>(source_container);
        if (!container_obj) {
            ctx.send_error(fmt::format("You can't get anything from {}.", ctx.format_object_name(source_container)));
            return CommandResult::InvalidState;
        }

        // Handle "get all from container"
        if (ctx.arg(0) == "all") {
            auto all_items = container_obj->get_contents();
            if (all_items.empty()) {
                ctx.send_error(fmt::format("There's nothing in {}.", ctx.format_object_name(source_container)));
                return CommandResult::InvalidTarget;
            }

            int gotten_count = 0;
            std::vector<std::string> gotten_names;
            // Make a proper copy - span is just a view, copying it doesn't copy the data
            std::vector<std::shared_ptr<Object>> items_to_get(all_items.begin(), all_items.end());

            for (const auto &item : items_to_get) {
                if (!item) continue;

                // Check if actor can carry more weight
                int object_weight = item->weight();
                if (!ctx.actor->inventory().can_carry(object_weight, ctx.actor->max_carry_weight())) {
                    if (gotten_count == 0) {
                        ctx.send_error("You can't carry any more.");
                        return CommandResult::ResourceError;
                    }
                    break; // Stop getting more items
                }

                // Remove from container and add to inventory (or wealth if coins)
                if (container_obj->remove_item(item)) {
                    // Money objects skip weight check and add directly to wealth
                    if (item->type() == ObjectType::Money) {
                        if (handle_pickup(ctx, item, &gotten_names)) {
                            gotten_count++;
                        }
                    } else {
                        if (handle_pickup(ctx, item, &gotten_names)) {
                            gotten_count++;
                        } else {
                            // Return item to container if inventory add failed
                            container_obj->add_item(item);
                        }
                    }
                }
            }

            if (gotten_count > 0) {
                if (gotten_count == 1) {
                    ctx.send_success(fmt::format("You get {} from {}.", gotten_names[0], ctx.format_object_name(source_container)));
                    ctx.send_to_room(fmt::format("{} gets {} from {}.", ctx.actor->display_name(), gotten_names[0], ctx.format_object_name(source_container)), true);
                } else {
                    ctx.send_success(fmt::format("You get {} items from {}.", gotten_count, ctx.format_object_name(source_container)));
                    ctx.send_to_room(fmt::format("{} gets several items from {}.", ctx.actor->display_name(), ctx.format_object_name(source_container)), true);
                }
                return CommandResult::Success;
            } else {
                ctx.send_error("You couldn't get anything.");
                return CommandResult::ResourceError;
            }
        }

        // Search for specific object in the container
        auto items_found = container_obj->find_items_by_keyword(ctx.arg(0));
        if (items_found.empty()) {
            ctx.send_error(fmt::format("There's no '{}' in {}.", ctx.arg(0), ctx.format_object_name(source_container)));
            return CommandResult::InvalidTarget;
        }

        // Get the first matching item
        auto item_to_get = items_found[0];

        // Check if actor can carry more weight (money skips this check)
        if (item_to_get->type() != ObjectType::Money) {
            int object_weight = item_to_get->weight();
            if (!ctx.actor->inventory().can_carry(object_weight, ctx.actor->max_carry_weight())) {
                ctx.send_error("You can't carry any more.");
                return CommandResult::ResourceError;
            }
        }

        // Remove from container and add to inventory (or wealth if coins)
        if (!container_obj->remove_item(item_to_get)) {
            ctx.send_error("Failed to get item from container.");
            return CommandResult::ResourceError;
        }

        // Handle money specially - add to wealth, don't add to inventory
        if (item_to_get->type() == ObjectType::Money) {
            long coin_value = item_to_get->value();
            if (coin_value > 0) {
                ctx.actor->give_wealth(coin_value);
                auto coins = fiery::Money::from_copper(coin_value);
                ctx.send_success(fmt::format("You get {} from {}.", coins.to_string(),
                                             ctx.format_object_name(source_container)));
                ctx.send_to_room(fmt::format("{} gets some coins from {}.", ctx.actor->display_name(),
                                             ctx.format_object_name(source_container)), true);
            }
            return CommandResult::Success;
        }

        auto add_result = ctx.actor->inventory().add_item(item_to_get);
        if (!add_result) {
            // Return item to container if inventory add failed
            container_obj->add_item(item_to_get);
            ctx.send_error(add_result.error().message);
            return CommandResult::ResourceError;
        }

        ctx.send_success(fmt::format("You get {} from {}.", ctx.format_object_name(item_to_get),
                                     ctx.format_object_name(source_container)));
        ctx.send_to_room(fmt::format("{} gets {} from {}.", ctx.actor->display_name(), ctx.format_object_name(item_to_get),
                                     ctx.format_object_name(source_container)),
                         true);

        return CommandResult::Success;
    } else {
        // Get from room
        auto &room_objects = ctx.room->contents_mutable().objects;

        // Handle "get all"
        if (ctx.arg(0) == "all") {
            if (room_objects.empty()) {
                ctx.send("There's nothing here to get.");
                return CommandResult::Success;
            }

            int gotten_count = 0;
            std::vector<std::string> gotten_names;
            auto objects_to_get = room_objects; // Copy to avoid iterator invalidation

            // Check if actor is a god (cached for "get all")
            auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
            bool is_god = player && player->is_god();

            for (const auto &obj : objects_to_get) {
                if (!obj) continue;

                // Check if object can be picked up (gods can pick up anything)
                if (!obj->can_take() && !is_god) {
                    continue; // Skip items without TAKE flag silently for "get all"
                }

                // Check if actor can carry more weight (money skips this check)
                if (obj->type() != ObjectType::Money) {
                    int object_weight = obj->weight();
                    if (!ctx.actor->inventory().can_carry(object_weight, ctx.actor->max_carry_weight())) {
                        ctx.send_error(fmt::format("The {} is too heavy to pick up.", ctx.format_object_name(obj)));
                        continue; // Skip this item and try the next one
                    }
                }

                // Remove from room and add to inventory (or wealth if coins)
                auto it = std::find(room_objects.begin(), room_objects.end(), obj);
                if (it != room_objects.end()) {
                    room_objects.erase(it);

                    // Money objects add to wealth directly instead of inventory
                    if (obj->type() == ObjectType::Money) {
                        if (handle_pickup(ctx, obj, &gotten_names)) {
                            gotten_count++;
                        }
                    } else {
                        if (handle_pickup(ctx, obj, &gotten_names)) {
                            gotten_count++;
                        } else {
                            // Return object to room if inventory add failed
                            room_objects.push_back(obj);
                        }
                    }
                }
            }

            if (gotten_count > 0) {
                if (gotten_count == 1) {
                    ctx.send_success(fmt::format("You get {}.", gotten_names[0]));
                    ctx.send_to_room(fmt::format("{} gets {}.", ctx.actor->display_name(), gotten_names[0]), true);
                } else {
                    ctx.send_success(fmt::format("You get {} items.", gotten_count));
                    ctx.send_to_room(fmt::format("{} gets several items.", ctx.actor->display_name()), true);
                }
                return CommandResult::Success;
            } else {
                ctx.send_error("You couldn't get anything.");
                return CommandResult::ResourceError;
            }
        }

        // Handle single object get
        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(ctx.arg(0))) {
                target_object = obj;
                break;
            }
        }

        if (!target_object) {
            ctx.send_error(fmt::format("You don't see '{}' here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }

        // Check if object can be picked up (gods can pick up anything)
        if (!target_object->can_take()) {
            auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
            if (!player || !player->is_god()) {
                ctx.send_error(fmt::format("You can't pick up {}.", ctx.format_object_name(target_object)));
                return CommandResult::InvalidState;
            }
        }

        // Check if actor can carry more weight (money skips this check)
        if (target_object->type() != ObjectType::Money) {
            int object_weight = target_object->weight();
            if (!ctx.actor->inventory().can_carry(object_weight, ctx.actor->max_carry_weight())) {
                ctx.send_error("You can't carry any more.");
                return CommandResult::ResourceError;
            }
        }

        // Remove from room and add to inventory (or wealth if coins)
        auto it = std::find(room_objects.begin(), room_objects.end(), target_object);
        if (it != room_objects.end()) {
            room_objects.erase(it);
        }

        // Handle money specially - add to wealth, don't add to inventory
        if (target_object->type() == ObjectType::Money) {
            long coin_value = target_object->value();
            if (coin_value > 0) {
                ctx.actor->give_wealth(coin_value);
                auto coins = fiery::Money::from_copper(coin_value);
                ctx.send_success(fmt::format("You get {}.", coins.to_string()));
                ctx.send_to_room(fmt::format("{} picks up some coins.", ctx.actor->display_name()), true);
            }
            return CommandResult::Success;
        }

        auto add_result = ctx.actor->inventory().add_item(target_object);
        if (!add_result) {
            // Return object to room if inventory add failed
            room_objects.push_back(target_object);
            ctx.send_error(add_result.error().message);
            return CommandResult::ResourceError;
        }

        ctx.send_success(fmt::format("You get {}.", ctx.format_object_name(target_object)));
        ctx.send_to_room(fmt::format("{} gets {}.", ctx.actor->display_name(), ctx.format_object_name(target_object)), true);

        return CommandResult::Success;
    }
}

Result<CommandResult> cmd_drop(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("drop <object>|all");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    auto inventory_items = ctx.actor->inventory().get_all_items();

    // Handle "drop all" 
    if (ctx.arg(0) == "all") {
        if (inventory_items.empty()) {
            ctx.send("You aren't carrying anything.");
            return CommandResult::Success;
        }

        int dropped_count = 0;
        std::vector<std::string> dropped_names;

        // Create a proper copy of the inventory items to avoid iterator invalidation
        std::vector<std::shared_ptr<Object>> items_to_drop(inventory_items.begin(), inventory_items.end());

        for (const auto &obj : items_to_drop) {
            if (obj) {
                // Remove from inventory
                if (ctx.actor->inventory().remove_item(obj)) {
                    // Add to room
                    ctx.room->contents_mutable().objects.push_back(obj);
                    dropped_count++;
                    dropped_names.push_back(ctx.format_object_name(obj));
                    // Check if object falls (in flying sector)
                    World().check_and_handle_object_falling(obj, ctx.room);
                }
            }
        }

        if (dropped_count > 0) {
            if (dropped_count == 1) {
                ctx.send_success(fmt::format("You drop {}.", dropped_names[0]));
                ctx.send_to_room(fmt::format("{} drops {}.", ctx.actor->display_name(), dropped_names[0]), true);
            } else {
                ctx.send_success(fmt::format("You drop {} items.", dropped_count));
                ctx.send_to_room(fmt::format("{} drops several items.", ctx.actor->display_name()), true);
            }
        } else {
            ctx.send_error("You couldn't drop anything.");
            return CommandResult::ResourceError;
        }

        return CommandResult::Success;
    }

    // Handle single object drop
    std::shared_ptr<Object> target_object = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            break;
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Remove from inventory
    if (!ctx.actor->inventory().remove_item(target_object)) {
        ctx.send_error("You can't drop that.");
        return CommandResult::ResourceError;
    }

    // Add to room
    ctx.room->contents_mutable().objects.push_back(target_object);

    ctx.send_success(fmt::format("You drop {}.", ctx.format_object_name(target_object)));
    ctx.send_to_room(fmt::format("{} drops {}.", ctx.actor->display_name(), ctx.format_object_name(target_object)), true);

    // Check if object falls (in flying sector)
    World().check_and_handle_object_falling(target_object, ctx.room);

    return CommandResult::Success;
}

Result<CommandResult> cmd_put(const CommandContext &ctx) {
    if (ctx.arg_count() < 2) {
        ctx.send_usage("put <object> [in] <container>");
        ctx.send_info("  Examples:");
        ctx.send_info("    put bread bag       - put one bread");
        ctx.send_info("    put 3 bread bag     - put 3 pieces of bread");
        ctx.send_info("    put all.bread bag   - put all bread");
        ctx.send_info("    put 3.bread bag     - put the 3rd bread");
        return CommandResult::InvalidSyntax;
    }

    // Parse arguments - handle multiple syntaxes:
    // "put object container"
    // "put object in container"
    // "put N object container" (put N items)
    // "put N object in container"
    std::string_view object_name;
    std::string_view container_name;
    int put_count = 0;  // 0 means use dot-notation logic, >0 means put exactly N items

    // Check if first arg is a pure number (count syntax)
    bool first_arg_is_count = true;
    for (char c : ctx.arg(0)) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            first_arg_is_count = false;
            break;
        }
    }

    if (first_arg_is_count && ctx.arg_count() >= 3) {
        // "put N object container" or "put N object in container" syntax
        try {
            put_count = std::stoi(std::string{ctx.arg(0)});
            if (put_count < 1) put_count = 1;
        } catch (...) {
            put_count = 1;
        }
        object_name = ctx.arg(1);

        if (ctx.arg_count() >= 4 && ctx.arg(2) == "in") {
            container_name = ctx.arg(3);
        } else {
            container_name = ctx.arg(2);
        }
    } else {
        // Standard syntax
        object_name = ctx.arg(0);

        if (ctx.arg_count() >= 3 && ctx.arg(1) == "in") {
            // "put object in container" syntax
            container_name = ctx.arg(2);
        } else {
            // "put object container" syntax
            container_name = ctx.arg(1);
        }
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    // Find objects matching the pattern
    std::vector<std::shared_ptr<Object>> inventory_items_to_put;

    if (put_count > 0) {
        // Count syntax: "put 3 bread bag" - find up to N matching items
        std::string keyword{object_name};
        for (const auto &obj : ctx.actor->inventory().get_all_items()) {
            if (!obj)
                continue;
            if (obj->matches_keyword(keyword)) {
                inventory_items_to_put.push_back(obj);
                if (static_cast<int>(inventory_items_to_put.size()) >= put_count) {
                    break;
                }
            }
        }
    } else {
        // Dot-notation syntax: supports all.keyword, N.keyword, or keyword
        auto items_to_put = ctx.find_objects_matching(object_name, false);

        // Filter to only inventory items (can't put equipped items without removing first)
        for (const auto &item : items_to_put) {
            if (ctx.actor->inventory().find_item(item->id())) {
                inventory_items_to_put.push_back(item);
            }
        }
    }

    if (inventory_items_to_put.empty()) {
        ctx.send_error(fmt::format("You don't have '{}'.", object_name));
        return CommandResult::InvalidTarget;
    }

    // Find container in the room or inventory
    std::shared_ptr<Object> container = nullptr;
    std::shared_ptr<Object> potential_container = nullptr;

    // First check inventory
    auto all_inventory = ctx.actor->inventory().get_all_items();
    for (const auto &obj : all_inventory) {
        if (obj && obj->matches_keyword(container_name)) {
            // Don't use an item we're trying to put as the container
            bool is_item_to_put = false;
            for (const auto &item : inventory_items_to_put) {
                if (obj == item) {
                    is_item_to_put = true;
                    break;
                }
            }
            if (is_item_to_put) continue;

            potential_container = obj;
            if (obj->is_container()) {
                container = obj;
            }
            break;
        }
    }

    // If not found in inventory, check room
    if (!potential_container) {
        auto &room_objects = ctx.room->contents_mutable().objects;
        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(container_name)) {
                potential_container = obj;
                if (obj->is_container()) {
                    container = obj;
                }
                break;
            }
        }
    }

    if (!potential_container) {
        ctx.send_error(fmt::format("You don't see '{}'.", container_name));
        return CommandResult::InvalidTarget;
    }

    if (!container) {
        ctx.send_error(fmt::format("You can't put anything in {} - it's not a container.",
                                   ctx.format_object_name(potential_container)));
        return CommandResult::InvalidTarget;
    }

    // Check container properties
    const auto &container_info = container->container_info();

    // Check if container is closed
    if (container_info.closeable && container_info.closed) {
        ctx.send_error(fmt::format("The {} is closed.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Try to cast to Container to access container functionality
    auto container_obj = std::dynamic_pointer_cast<Container>(container);

    if (!container_obj) {
        ctx.send_error(fmt::format("You can't put anything in {}.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Don't allow putting items in corpses
    if (container->type() == ObjectType::Corpse) {
        ctx.send_error("You can't put items in a corpse.");
        return CommandResult::InvalidTarget;
    }

    // Put each item in the container
    int success_count = 0;
    int fail_count = 0;

    for (const auto &item_to_put : inventory_items_to_put) {
        // Try to add item to container
        auto add_result = container_obj->add_item(item_to_put);
        if (!add_result) {
            ctx.send_error(fmt::format("The {} is full - couldn't put {}.",
                                       ctx.format_object_name(container),
                                       ctx.format_object_name(item_to_put)));
            fail_count++;
            continue;
        }

        // Remove from actor's inventory
        if (!ctx.actor->inventory().remove_item(item_to_put)) {
            // If we can't remove from inventory, remove from container to maintain consistency
            container_obj->remove_item(item_to_put);
            fail_count++;
            continue;
        }

        success_count++;

        // Send individual messages for each item
        ctx.send_success(fmt::format("You put {} in {}.",
                                     ctx.format_object_name(item_to_put),
                                     ctx.format_object_name(container)));
        ctx.send_to_room(fmt::format("{} puts {} in {}.",
                                     ctx.actor->display_name(),
                                     ctx.format_object_name(item_to_put),
                                     ctx.format_object_name(container)),
                         true);
    }

    // Summary for multiple items
    if (inventory_items_to_put.size() > 1 && success_count > 0) {
        ctx.send_info(fmt::format("Put {} item{} in {}.",
                                  success_count,
                                  success_count == 1 ? "" : "s",
                                  ctx.format_object_name(container)));
    }

    if (success_count == 0) {
        return CommandResult::InvalidState;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_give(const CommandContext &ctx) {
    if (auto result = ctx.require_args(2, "<object> <player>"); !result) {
        ctx.send_usage("give <object> <player>");
        return CommandResult::InvalidSyntax;
    }

    auto obj = ctx.find_object_target(ctx.arg(0));
    if (!obj) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    auto target = ctx.find_actor_target(ctx.arg(1));
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(1)));
        return CommandResult::InvalidTarget;
    }

    if (target == ctx.actor) {
        ctx.send_error("You can't give something to yourself.");
        return CommandResult::InvalidTarget;
    }

    // Transfer the object from giver to receiver
    auto removed_obj = ctx.actor->inventory().remove_item(obj->id());
    if (!removed_obj) {
        ctx.send_error("You no longer have that item.");
        return CommandResult::ResourceError;
    }
    
    // Add to target's inventory
    if (auto add_result = target->inventory().add_item(removed_obj); !add_result) {
        // Failed to add to target - return to giver
        ctx.actor->inventory().add_item(removed_obj);
        ctx.send_error(fmt::format("{} cannot carry any more items.", target->display_name()));
        return CommandResult::ResourceError;
    }

    // Fire RECEIVE trigger for mobs receiving items
    // For money objects, fire BRIBE trigger instead
    auto& trigger_mgr = FieryMUD::TriggerManager::instance();
    if (trigger_mgr.is_initialized()) {
        if (obj->type() == ObjectType::Money && obj->value() > 0) {
            // BRIBE trigger for gold
            trigger_mgr.dispatch_bribe(target, ctx.actor, obj->value());
        } else {
            // RECEIVE trigger for other items
            trigger_mgr.dispatch_receive(target, ctx.actor, removed_obj);
        }
    }

    ctx.send_success(fmt::format("You give {} to {}.", ctx.format_object_name(obj), target->display_name()));

    ctx.send_to_actor(target, fmt::format("{} gives you {}.", ctx.actor->display_name(), ctx.format_object_name(obj)));

    ctx.send_to_room(fmt::format("{} gives {} to {}.", ctx.actor->display_name(), ctx.format_object_name(obj), target->display_name()),
                     true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_wear(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>|all"); !result) {
        ctx.send_usage("wear <object>|all");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    auto inventory_items = ctx.actor->inventory().get_all_items();

    // Handle "wear all" - attempt to wear everything in inventory
    if (ctx.arg(0) == "all") {
        if (inventory_items.empty()) {
            ctx.send("You aren't carrying anything.");
            return CommandResult::Success;
        }

        int worn_count = 0;
        std::vector<std::string> worn_names;

        // Create a copy to avoid iterator invalidation
        std::vector<std::shared_ptr<Object>> items_to_try(inventory_items.begin(), inventory_items.end());

        for (const auto &obj : items_to_try) {
            if (!obj) continue;

            // Skip non-wearable items silently
            if (!obj->is_wearable()) continue;

            // Try to equip the item
            auto equip_result = ctx.actor->equipment().equip_item(obj);
            if (!equip_result) {
                // Slot full or can't wear - skip silently for "wear all"
                continue;
            }

            // Remove from inventory since it's now equipped
            ctx.actor->inventory().remove_item(obj);
            worn_count++;
            worn_names.push_back(ctx.format_object_name(obj));
        }

        if (worn_count > 0) {
            if (worn_count == 1) {
                ctx.send_success(fmt::format("You wear {}.", worn_names[0]));
                ctx.send_to_room(fmt::format("{} wears {}.", ctx.actor->display_name(), worn_names[0]), true);
            } else {
                ctx.send_success(fmt::format("You wear {} items.", worn_count));
                ctx.send_to_room(fmt::format("{} puts on several items.", ctx.actor->display_name()), true);
            }
            return CommandResult::Success;
        } else {
            ctx.send("You couldn't wear anything.");
            return CommandResult::Success;
        }
    }

    // Find object in actor's inventory
    std::shared_ptr<Object> target_object = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            break;
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Try to equip the item
    auto equip_result = ctx.actor->equipment().equip_item(target_object);
    if (!equip_result) {
        ctx.send_error(
            fmt::format("You can't wear {}: {}", ctx.format_object_name(target_object), equip_result.error().message));
        return CommandResult::ResourceError;
    }

    // Remove from inventory since it's now equipped
    ctx.actor->inventory().remove_item(target_object);

    ctx.send_success(fmt::format("You wear {}.", ctx.format_object_name(target_object)));
    ctx.send_to_room(fmt::format("{} wears {}.", ctx.actor->display_name(), ctx.format_object_name(target_object)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_wield(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<weapon>"); !result) {
        ctx.send_usage("wield <weapon>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find object in actor's inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> target_object = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            break;
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Try to equip the item (wield is just equipping)
    auto equip_result = ctx.actor->equipment().equip_item(target_object);
    if (!equip_result) {
        ctx.send_error(
            fmt::format("You can't wield {}: {}", ctx.format_object_name(target_object), equip_result.error().message));
        return CommandResult::ResourceError;
    }

    // Remove from inventory since it's now equipped
    ctx.actor->inventory().remove_item(target_object);

    ctx.send_success(fmt::format("You wield {}.", ctx.format_object_name(target_object)));
    ctx.send_to_room(fmt::format("{} wields {}.", ctx.actor->display_name(), ctx.format_object_name(target_object)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_remove(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("remove <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find object in actor's equipment
    auto equipped_items = ctx.actor->equipment().get_all_equipped();
    std::shared_ptr<Object> target_object = nullptr;
    EntityId target_item_id = INVALID_ENTITY_ID;

    for (const auto &obj : equipped_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            target_item_id = obj->id();
            break;
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You're not wearing '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Unequip the item
    auto unequipped_item = ctx.actor->equipment().unequip_item(target_item_id);
    if (!unequipped_item) {
        ctx.send_error("You can't remove that.");
        return CommandResult::ResourceError;
    }

    // Add back to inventory
    auto add_result = ctx.actor->inventory().add_item(unequipped_item);
    if (!add_result) {
        // If can't add to inventory, put back in equipment
        auto reequip_result = ctx.actor->equipment().equip_item(unequipped_item);
        if (!reequip_result) {
            Log::error("Failed to reequip item after inventory add failure");
        }
        ctx.send_error("You can't carry any more items.");
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("You remove {}.", ctx.format_object_name(target_object)));
    ctx.send_to_room(fmt::format("{} removes {}.", ctx.actor->display_name(), ctx.format_object_name(target_object)), true);

    return CommandResult::Success;
}

// =============================================================================
// Container and Object Interaction Commands
// =============================================================================

Result<CommandResult> cmd_open(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("open <container|door>");
        return CommandResult::InvalidSyntax;
    }

    std::string_view target_name = ctx.arg(0);

    if (!ctx.actor || !ctx.room) {
        return std::unexpected(Errors::InvalidState("No actor or room context"));
    }

    // Find container in room or inventory
    std::shared_ptr<Object> target_obj = nullptr;
    
    // First check inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(target_name)) {
            target_obj = obj;
            break;
        }
    }
    
    // If not found in inventory, check room
    if (!target_obj) {
        auto &room_objects = ctx.room->contents_mutable().objects;
        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(target_name)) {
                target_obj = obj;
                break;
            }
        }
    }

    if (!target_obj) {
        ctx.send_error(fmt::format("You don't see '{}'.", target_name));
        return CommandResult::InvalidTarget;
    }

    if (!target_obj->is_container()) {
        ctx.send_error(fmt::format("You can't open {} - it's not a container.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidTarget;
    }

    auto container_info = target_obj->container_info();
    
    if (!container_info.closeable) {
        ctx.send_error(fmt::format("The {} cannot be opened.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidTarget;
    }
    
    if (!container_info.closed) {
        ctx.send_error(fmt::format("The {} is already open.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidState;
    }
    
    if (container_info.locked) {
        ctx.send_error(fmt::format("The {} is locked.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidState;
    }

    // Open the container
    container_info.closed = false;
    target_obj->set_container_info(container_info);

    ctx.send_success(fmt::format("You open {}.", ctx.format_object_name(target_obj)));
    ctx.send_to_room(fmt::format("{} opens {}.", ctx.actor->display_name(), ctx.format_object_name(target_obj)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_close(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("close <container|door>");
        return CommandResult::InvalidSyntax;
    }

    std::string_view target_name = ctx.arg(0);

    if (!ctx.actor || !ctx.room) {
        return std::unexpected(Errors::InvalidState("No actor or room context"));
    }

    // Find container in room or inventory
    std::shared_ptr<Object> target_obj = nullptr;
    
    // First check inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(target_name)) {
            target_obj = obj;
            break;
        }
    }
    
    // If not found in inventory, check room
    if (!target_obj) {
        auto &room_objects = ctx.room->contents_mutable().objects;
        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(target_name)) {
                target_obj = obj;
                break;
            }
        }
    }

    if (!target_obj) {
        ctx.send_error(fmt::format("You don't see '{}'.", target_name));
        return CommandResult::InvalidTarget;
    }

    if (!target_obj->is_container()) {
        ctx.send_error(fmt::format("You can't close {} - it's not a container.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidTarget;
    }

    auto container_info = target_obj->container_info();
    
    if (!container_info.closeable) {
        ctx.send_error(fmt::format("The {} cannot be closed.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidTarget;
    }
    
    if (container_info.closed) {
        ctx.send_error(fmt::format("The {} is already closed.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidState;
    }

    // Close the container
    container_info.closed = true;
    target_obj->set_container_info(container_info);

    ctx.send_success(fmt::format("{} is now closed.", ctx.format_object_name(target_obj)));
    ctx.send_to_room(fmt::format("{} closes {}.", ctx.actor->display_name(), ctx.format_object_name(target_obj)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_lock(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("lock <container|door>");
        return CommandResult::InvalidSyntax;
    }

    std::string_view target_name = ctx.arg(0);

    if (!ctx.actor || !ctx.room) {
        return std::unexpected(Errors::InvalidState("No actor or room context"));
    }

    // Find container in room or inventory
    std::shared_ptr<Object> target_obj = nullptr;
    
    // First check inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(target_name)) {
            target_obj = obj;
            break;
        }
    }
    
    // If not found in inventory, check room
    if (!target_obj) {
        auto &room_objects = ctx.room->contents_mutable().objects;
        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(target_name)) {
                target_obj = obj;
                break;
            }
        }
    }

    if (!target_obj) {
        ctx.send_error(fmt::format("You don't see '{}'.", target_name));
        return CommandResult::InvalidTarget;
    }

    if (!target_obj->is_container()) {
        ctx.send_error(fmt::format("You can't lock {} - it's not a container.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidTarget;
    }

    auto container_info = target_obj->container_info();
    
    if (!container_info.lockable) {
        ctx.send_error(fmt::format("The {} cannot be locked.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidTarget;
    }
    
    if (!container_info.closed) {
        ctx.send_error(fmt::format("You must close {} first before locking it.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidState;
    }
    
    if (container_info.locked) {
        ctx.send_error(fmt::format("The {} is already locked.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidState;
    }

    // Lock the container
    container_info.locked = true;
    target_obj->set_container_info(container_info);

    ctx.send_success(fmt::format("You lock {}.", ctx.format_object_name(target_obj)));
    ctx.send_to_room(fmt::format("{} locks {}.", ctx.actor->display_name(), ctx.format_object_name(target_obj)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_unlock(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("unlock <container|door>");
        return CommandResult::InvalidSyntax;
    }

    std::string_view target_name = ctx.arg(0);

    if (!ctx.actor || !ctx.room) {
        return std::unexpected(Errors::InvalidState("No actor or room context"));
    }

    // Find container in room or inventory
    std::shared_ptr<Object> target_obj = nullptr;
    
    // First check inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(target_name)) {
            target_obj = obj;
            break;
        }
    }
    
    // If not found in inventory, check room
    if (!target_obj) {
        auto &room_objects = ctx.room->contents_mutable().objects;
        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(target_name)) {
                target_obj = obj;
                break;
            }
        }
    }

    if (!target_obj) {
        ctx.send_error(fmt::format("You don't see '{}'.", target_name));
        return CommandResult::InvalidTarget;
    }

    if (!target_obj->is_container()) {
        ctx.send_error(fmt::format("You can't unlock {} - it's not a container.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidTarget;
    }

    auto container_info = target_obj->container_info();
    
    if (!container_info.lockable) {
        ctx.send_error(fmt::format("The {} cannot be unlocked.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidTarget;
    }
    
    if (!container_info.locked) {
        ctx.send_error(fmt::format("The {} is not locked.", ctx.format_object_name(target_obj)));
        return CommandResult::InvalidState;
    }

    // Unlock the container
    container_info.locked = false;
    target_obj->set_container_info(container_info);

    ctx.send_success(fmt::format("You unlock {}.", ctx.format_object_name(target_obj)));
    ctx.send_to_room(fmt::format("{} unlocks {}.", ctx.actor->display_name(), ctx.format_object_name(target_obj)), true);

    return CommandResult::Success;
}

// =============================================================================
// Consumable and Utility Commands
// =============================================================================

Result<CommandResult> cmd_light(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("light <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find light source in equipped or inventory (prefer held items)
    std::shared_ptr<Object> light_source = nullptr;

    // First check equipped items (prioritize what you're holding)
    for (const auto &obj : ctx.actor->equipment().get_all_equipped()) {
        if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_light_source()) {
            light_source = obj;
            break;
        }
    }

    // If not found in equipped, check inventory
    if (!light_source) {
        for (const auto &obj : ctx.actor->inventory().get_all_items()) {
            if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_light_source()) {
                light_source = obj;
                break;
            }
        }
    }

    if (!light_source) {
        ctx.send_error(fmt::format("You don't have a light source called '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if already lit
    const auto &light_info = light_source->light_info();
    if (light_info.lit) {
        if (light_info.permanent) {
            ctx.send_error(fmt::format("The {} glows with a permanent light.", ctx.format_object_name(light_source)));
        } else {
            ctx.send_error(fmt::format("The {} is already lit.", ctx.format_object_name(light_source)));
        }
        return CommandResult::InvalidState;
    }

    // Check if light source has duration remaining (duration -1 = infinite)
    if (light_info.duration == 0) {
        ctx.send_error(fmt::format("The {} is burnt out and cannot be lit.", ctx.format_object_name(light_source)));
        return CommandResult::InvalidState;
    }

    // Light the object
    auto new_light_info = light_info;
    new_light_info.lit = true;
    light_source->set_light_info(new_light_info);

    ctx.send_success(fmt::format("You light the {}. It glows brightly.", ctx.format_object_name(light_source)));
    ctx.send_to_room(fmt::format("{} lights {}.", ctx.actor->display_name(), ctx.format_object_name(light_source)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_extinguish(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("extinguish <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find light source in equipped or inventory (prefer held items)
    std::shared_ptr<Object> light_source = nullptr;

    // First check equipped items (prioritize what you're holding)
    for (const auto &obj : ctx.actor->equipment().get_all_equipped()) {
        if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_light_source()) {
            light_source = obj;
            break;
        }
    }

    // If not found in equipped, check inventory
    if (!light_source) {
        for (const auto &obj : ctx.actor->inventory().get_all_items()) {
            if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_light_source()) {
                light_source = obj;
                break;
            }
        }
    }

    if (!light_source) {
        ctx.send_error(fmt::format("You don't have a light source called '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    const auto &light_info = light_source->light_info();

    // Check if it's a permanent light
    if (light_info.permanent) {
        ctx.send_error(fmt::format("The {} glows with a permanent light that cannot be extinguished.",
                                   ctx.format_object_name(light_source)));
        return CommandResult::InvalidState;
    }

    // Check if already extinguished
    if (!light_info.lit) {
        ctx.send_error(fmt::format("The {} is not lit.", ctx.format_object_name(light_source)));
        return CommandResult::InvalidState;
    }

    // Extinguish the object
    auto new_light_info = light_info;
    new_light_info.lit = false;
    light_source->set_light_info(new_light_info);

    ctx.send_success(fmt::format("You extinguish the {}.", ctx.format_object_name(light_source)));
    ctx.send_to_room(fmt::format("{} extinguishes {}.", ctx.actor->display_name(), ctx.format_object_name(light_source)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_eat(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("eat <food>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find food item in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> food_item = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            if (obj->type() == ObjectType::Food || obj->type() == ObjectType::Potion) {
                food_item = obj;
                break;
            }
        }
    }

    if (!food_item) {
        ctx.send_error(fmt::format("You don't have any food called '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if food is spoiled (timer expired)
    if (food_item->is_expired()) {
        ctx.send_error(fmt::format("The {} is spoiled and inedible.", ctx.format_object_name(food_item)));
        return CommandResult::InvalidState;
    }

    // Consume the food
    if (!ctx.actor->inventory().remove_item(food_item)) {
        ctx.send_error("Failed to consume the food item.");
        return CommandResult::ResourceError;
    }

    if (food_item->type() == ObjectType::Food) {
        ctx.send_success(fmt::format("You eat the {}. It's delicious!", ctx.format_object_name(food_item)));
        ctx.send_to_room(fmt::format("{} eats {}.", ctx.actor->display_name(), ctx.format_object_name(food_item)), true);
    } else {
        ctx.send_success(fmt::format("You drink the {}. You feel refreshed.", ctx.format_object_name(food_item)));
        ctx.send_to_room(fmt::format("{} drinks {}.", ctx.actor->display_name(), ctx.format_object_name(food_item)), true);
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_drink(const CommandContext &ctx) {
    std::string target_name;
    
    if (ctx.arg_count() >= 2 && ctx.arg(0) == "from") {
        // "drink from <container>" syntax
        target_name = ctx.arg(1);
    } else if (ctx.arg_count() >= 1) {
        // "drink <item>" syntax
        target_name = ctx.arg(0);
    } else {
        ctx.send_usage("drink <item> | drink from <container>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Helper to check if object matches by keyword OR liquid type
    auto matches_drink_target = [&target_name](const std::shared_ptr<Object>& obj) -> bool {
        if (!obj) return false;

        // Must be a drinkable type
        if (obj->type() != ObjectType::Liquid_Container &&
            obj->type() != ObjectType::Potion &&
            obj->type() != ObjectType::Food) {
            return false;
        }

        // Check if it matches by object keyword
        if (obj->matches_keyword(target_name)) {
            return true;
        }

        // For liquid containers, also check if target matches the liquid type
        if (obj->type() == ObjectType::Liquid_Container) {
            const auto& liquid = obj->liquid_info();
            if (!liquid.liquid_type.empty() && liquid.remaining > 0) {
                // Case-insensitive comparison of liquid type
                std::string lower_target;
                lower_target.reserve(target_name.size());
                for (char c : target_name) {
                    lower_target.push_back(std::tolower(static_cast<unsigned char>(c)));
                }
                std::string lower_liquid;
                lower_liquid.reserve(liquid.liquid_type.size());
                for (char c : liquid.liquid_type) {
                    lower_liquid.push_back(std::tolower(static_cast<unsigned char>(c)));
                }
                if (lower_liquid == lower_target) {
                    return true;
                }
            }
        }

        return false;
    };

    // Find drinkable item - check equipped first, then inventory
    std::shared_ptr<Object> drink_item = nullptr;

    // First check equipped items (prioritize what you're holding)
    for (const auto &obj : ctx.actor->equipment().get_all_equipped()) {
        if (matches_drink_target(obj)) {
            drink_item = obj;
            break;
        }
    }

    // Then check inventory
    if (!drink_item) {
        for (const auto &obj : ctx.actor->inventory().get_all_items()) {
            if (matches_drink_target(obj)) {
                drink_item = obj;
                break;
            }
        }
    }

    if (!drink_item) {
        ctx.send_error(fmt::format("You don't have anything to drink called '{}'.", target_name));
        return CommandResult::InvalidTarget;
    }

    // Handle different drinkable types
    if (drink_item->type() == ObjectType::Potion) {
        // Potions are consumed completely
        if (!ctx.actor->inventory().remove_item(drink_item)) {
            ctx.send_error("Failed to drink the potion.");
            return CommandResult::ResourceError;
        }
        
        ctx.send_success(fmt::format("You drink the {}. You feel its effects course through your body.", 
                                   ctx.format_object_name(drink_item)));
        ctx.send_to_room(fmt::format("{} drinks {}.", ctx.actor->display_name(), ctx.format_object_name(drink_item)), true);
    } else if (drink_item->type() == ObjectType::Liquid_Container) {
        // Check if container has liquid
        const auto& liquid = drink_item->liquid_info();
        if (liquid.remaining <= 0) {
            ctx.send_error(fmt::format("The {} is empty.", ctx.format_object_name(drink_item)));
            return CommandResult::InvalidState;
        }

        // Consume some liquid (one drink = about 1 unit)
        LiquidInfo updated_liquid = liquid;
        int drink_amount = std::min(liquid.remaining, 1);
        updated_liquid.remaining -= drink_amount;
        drink_item->set_liquid_info(updated_liquid);

        // Format the liquid type nicely (lowercase)
        std::string liquid_name = liquid.liquid_type;
        std::transform(liquid_name.begin(), liquid_name.end(), liquid_name.begin(),
                      [](unsigned char c) { return std::tolower(c); });

        if (liquid.poisoned) {
            ctx.send_error(fmt::format("You drink some {} from the {}... it tastes strange!",
                                     liquid_name, ctx.format_object_name(drink_item)));
            // Apply poison effect to the drinker
            ctx.actor->set_flag(ActorFlag::Poison, true);
            ctx.send_error("You feel very sick!");
        } else {
            ctx.send_success(fmt::format("You drink some {} from the {}. Refreshing!",
                                       liquid_name, ctx.format_object_name(drink_item)));
        }
        ctx.send_to_room(fmt::format("{} drinks from {}.", ctx.actor->display_name(), ctx.format_object_name(drink_item)), true);
    } else {
        // Food items (like fruits with juice)
        ctx.send_success(fmt::format("You drink from the {}. It quenches your thirst.", 
                                   ctx.format_object_name(drink_item)));
        ctx.send_to_room(fmt::format("{} drinks from {}.", ctx.actor->display_name(), ctx.format_object_name(drink_item)), true);
    }

    return CommandResult::Success;
}

// =============================================================================
// Shop Commands
// =============================================================================

Result<CommandResult> cmd_list(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find shopkeeper in current room
    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    // Look for shopkeepers in the room
    const auto& room_contents = ctx.room->contents();
    std::vector<std::shared_ptr<Actor>> shopkeepers;
    
    for (const auto& actor : room_contents.actors) {
        if (auto mobile = std::dynamic_pointer_cast<Mobile>(actor)) {
            if (mobile->is_shopkeeper()) {
                shopkeepers.push_back(mobile);
            }
        }
    }
    
    if (shopkeepers.empty()) {
        ctx.send_error("There are no shopkeepers here.");
        return CommandResult::ResourceError;
    }
    
    // Use the first shopkeeper found (could be enhanced to let user choose)
    auto shopkeeper_actor = shopkeepers[0];
    auto shopkeeper_mobile = std::dynamic_pointer_cast<Mobile>(shopkeeper_actor);

    // Look up shop by the mob's prototype ID (which is how shops are registered)
    auto& shop_manager = ShopManager::instance();
    EntityId lookup_id = shopkeeper_mobile ? shopkeeper_mobile->prototype_id() : shopkeeper_actor->id();
    const auto* shop = shop_manager.get_shopkeeper(lookup_id);

    if (!shop) {
        ctx.send_error("The shopkeeper doesn't seem to be selling anything right now.");
        return CommandResult::ResourceError;
    }
    
    // Display shop listing
    auto listing = shop->get_shop_listing();
    for (const auto& line : listing) {
        ctx.send(line);
    }
    
    return CommandResult::Success;
}

Result<CommandResult> cmd_buy(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("buy <item>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    std::string item_name{ctx.arg(0)};
    
    // Look for shopkeepers in the room
    const auto& room_contents = ctx.room->contents();
    std::vector<std::shared_ptr<Actor>> shopkeepers;
    
    for (const auto& actor : room_contents.actors) {
        if (auto mobile = std::dynamic_pointer_cast<Mobile>(actor)) {
            if (mobile->is_shopkeeper()) {
                shopkeepers.push_back(mobile);
            }
        }
    }
    
    if (shopkeepers.empty()) {
        ctx.send_error("There are no shopkeepers here.");
        return CommandResult::ResourceError;
    }
    
    // Use the first shopkeeper found
    auto shopkeeper_actor = shopkeepers[0];
    auto shopkeeper_mobile = std::dynamic_pointer_cast<Mobile>(shopkeeper_actor);

    // Look up shop by the mob's prototype ID (which is how shops are registered)
    auto& shop_manager = ShopManager::instance();
    EntityId lookup_id = shopkeeper_mobile ? shopkeeper_mobile->prototype_id() : shopkeeper_actor->id();
    const auto* shop = shop_manager.get_shopkeeper(lookup_id);

    if (!shop) {
        ctx.send_error("The shopkeeper doesn't seem to be selling anything right now.");
        return CommandResult::ResourceError;
    }

    // Find item by name in shop inventory
    auto available_items = shop->get_available_items();
    const ShopItem* target_item = nullptr;

    for (const auto& item : available_items) {
        if (item.name.find(item_name) != std::string::npos ||
            item.name == item_name) {
            target_item = &item;
            break;
        }
    }

    if (!target_item) {
        ctx.send_error(fmt::format("The shopkeeper doesn't have '{}' for sale.", item_name));
        return CommandResult::InvalidTarget;
    }

    // Calculate price and attempt purchase
    int price = shop->calculate_buy_price(*target_item);

    // Attempt to purchase the item
    auto result = ShopManager::instance().buy_item(ctx.actor, lookup_id, target_item->prototype_id);

    switch (result) {
        case ShopResult::Success: {
            // Create an instance of the object from the prototype
            auto new_object = WorldManager::instance().create_object_instance(target_item->prototype_id);
            if (!new_object) {
                ctx.send_error("The shopkeeper hands you something, but it crumbles to dust!");
                Log::error("Failed to create object instance for prototype {} after successful purchase",
                          target_item->prototype_id);
                return CommandResult::InvalidState;
            }

            // Add the object to the player's inventory
            auto add_result = ctx.actor->inventory().add_item(new_object);
            if (!add_result) {
                ctx.send_error("You can't carry any more items!");
                // Refund the player
                if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
                    player->receive(price);
                    ctx.send("The shopkeeper refunds your money.");
                }
                return CommandResult::InvalidState;
            }

            ctx.send_success(fmt::format("You buy {} for {} copper coins.", target_item->name, price));
            break;
        }
        case ShopResult::InsufficientFunds:
            ctx.send_error("You don't have enough money to buy that.");
            return CommandResult::InvalidSyntax;
        case ShopResult::InsufficientStock:
            ctx.send_error("That item is out of stock.");
            return CommandResult::InvalidTarget;
        default:
            ctx.send_error("You cannot buy that item.");
            return CommandResult::InvalidTarget;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_sell(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("sell <item>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    std::string target_name{ctx.arg(0)};

    // Find shopkeeper in current room
    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    // Look for shopkeepers in the room
    const auto& room_contents = ctx.room->contents();
    std::vector<std::shared_ptr<Actor>> shopkeepers;
    
    for (const auto& actor : room_contents.actors) {
        if (auto mobile = std::dynamic_pointer_cast<Mobile>(actor)) {
            if (mobile->is_shopkeeper()) {
                shopkeepers.push_back(mobile);
            }
        }
    }
    
    if (shopkeepers.empty()) {
        ctx.send_error("There are no shopkeepers here.");
        return CommandResult::ResourceError;
    }
    
    // Use the first shopkeeper found
    auto shopkeeper_actor = shopkeepers[0];
    auto shopkeeper_mobile = std::dynamic_pointer_cast<Mobile>(shopkeeper_actor);

    // Look up shop by the mob's prototype ID (which is how shops are registered)
    auto& shop_manager = ShopManager::instance();
    EntityId lookup_id = shopkeeper_mobile ? shopkeeper_mobile->prototype_id() : shopkeeper_actor->id();
    const auto* shop = shop_manager.get_shopkeeper(lookup_id);

    if (!shop) {
        ctx.send_error("The shopkeeper doesn't seem to be selling anything right now.");
        return CommandResult::ResourceError;
    }

    // Find item in player's inventory (simplified search for now)
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> target_object = nullptr;
    
    for (const auto& item : inventory_items) {
        if (item->name().find(target_name) != std::string::npos || 
            item->name() == target_name) {
            target_object = item;
            break;
        }
    }
    
    if (!target_object) {
        ctx.send_error(fmt::format("You don't have '{}' to sell.", target_name));
        return CommandResult::InvalidTarget;
    }
    
    // Calculate sell price and simulate sale
    int price = shop->calculate_sell_price(*target_object);
    
    ctx.send_success(fmt::format("You would sell {} for {} copper coins.", 
                                 target_object->display_name(), price));
    ctx.send(fmt::format("(Note: Actual sale mechanics with currency integration pending)"));
    
    return CommandResult::Success;
}

// =============================================================================
// Additional Object Manipulation Commands (Phase 1.4)
// =============================================================================

Result<CommandResult> cmd_hold(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("hold <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find object in actor's inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> target_object = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            break;
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Try to equip the item in the held slot
    auto equip_result = ctx.actor->equipment().equip_item(target_object);
    if (!equip_result) {
        ctx.send_error(
            fmt::format("You can't hold {}: {}", ctx.format_object_name(target_object), equip_result.error().message));
        return CommandResult::ResourceError;
    }

    // Remove from inventory since it's now equipped
    ctx.actor->inventory().remove_item(target_object);

    ctx.send_success(fmt::format("You hold {}.", ctx.format_object_name(target_object)));
    ctx.send_to_room(fmt::format("{} holds {}.", ctx.actor->display_name(), ctx.format_object_name(target_object)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_grab(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("grab <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor || !ctx.room) {
        return std::unexpected(Errors::InvalidState("No actor or room context"));
    }

    auto &room_objects = ctx.room->contents_mutable().objects;
    std::shared_ptr<Object> target_object = nullptr;

    for (auto &obj : room_objects) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            break;
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You don't see '{}' here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if actor can carry more weight
    int object_weight = target_object->weight();
    if (!ctx.actor->inventory().can_carry(object_weight, ctx.actor->max_carry_weight())) {
        ctx.send_error("You can't carry any more.");
        return CommandResult::ResourceError;
    }

    // Remove from room and add to inventory
    auto it = std::find(room_objects.begin(), room_objects.end(), target_object);
    if (it != room_objects.end()) {
        room_objects.erase(it);
    }

    auto add_result = ctx.actor->inventory().add_item(target_object);
    if (!add_result) {
        room_objects.push_back(target_object);
        ctx.send_error(add_result.error().message);
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("You quickly grab {}.", ctx.format_object_name(target_object)));
    ctx.send_to_room(fmt::format("{} quickly grabs {}.", ctx.actor->display_name(), ctx.format_object_name(target_object)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_quaff(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<potion>"); !result) {
        ctx.send_usage("quaff <potion>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find potion in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> potion = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            if (obj->type() == ObjectType::Potion) {
                potion = obj;
                break;
            }
        }
    }

    if (!potion) {
        ctx.send_error(fmt::format("You don't have a potion called '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Consume the potion
    if (!ctx.actor->inventory().remove_item(potion)) {
        ctx.send_error("Failed to drink the potion.");
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("You quaff {}. Magical energies course through your body!",
                                 ctx.format_object_name(potion)));
    ctx.send_to_room(fmt::format("{} quaffs {} and is surrounded by a brief magical glow.",
                                 ctx.actor->display_name(), ctx.format_object_name(potion)), true);

    // Apply potion spell effects (target is always self for potions)
    for (int spell_id : potion->spell_ids()) {
        if (spell_id > 0) {
            auto result = FieryMUD::AbilityExecutor::execute_by_id(ctx, spell_id, ctx.actor, potion->spell_level());
            if (result) {
                // Send any effect messages
                if (!result->attacker_message.empty()) {
                    ctx.send(result->attacker_message);
                }
            }
        }
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_recite(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<scroll> [target]"); !result) {
        ctx.send_usage("recite <scroll> [target]");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find scroll in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> scroll = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            if (obj->type() == ObjectType::Scroll) {
                scroll = obj;
                break;
            }
        }
    }

    if (!scroll) {
        ctx.send_error(fmt::format("You don't have a scroll called '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Determine target (optional second argument)
    std::shared_ptr<Actor> target = ctx.actor; // Default to self
    if (ctx.arg_count() > 1) {
        target = ctx.find_actor_target(ctx.arg(1));
        if (!target) {
            ctx.send_error(fmt::format("You don't see '{}' here.", ctx.arg(1)));
            return CommandResult::InvalidTarget;
        }
    }

    // Consume the scroll
    if (!ctx.actor->inventory().remove_item(scroll)) {
        ctx.send_error("Failed to use the scroll.");
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("You recite {}. The scroll crumbles to dust as its magic is released!",
                                 ctx.format_object_name(scroll)));
    ctx.send_to_room(fmt::format("{} recites {} and the scroll crumbles to dust.",
                                 ctx.actor->display_name(), ctx.format_object_name(scroll)), true);

    // Apply scroll spell effects on target
    for (int spell_id : scroll->spell_ids()) {
        if (spell_id > 0) {
            auto result = FieryMUD::AbilityExecutor::execute_by_id(ctx, spell_id, target, scroll->spell_level());
            if (result) {
                // Send attacker messages to caster
                if (!result->attacker_message.empty()) {
                    ctx.send(result->attacker_message);
                }
                // Send target messages if target is different from caster
                if (target != ctx.actor && !result->target_message.empty()) {
                    ctx.send_to_actor(target, result->target_message);
                }
            }
        }
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_use(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<item> [target]"); !result) {
        ctx.send_usage("use <item> [target]");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find item in inventory or equipment
    auto inventory_items = ctx.actor->inventory().get_all_items();
    auto equipped_items = ctx.actor->equipment().get_all_equipped();
    std::shared_ptr<Object> target_object = nullptr;

    // Check inventory first
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            break;
        }
    }

    // If not in inventory, check equipment
    if (!target_object) {
        for (const auto &obj : equipped_items) {
            if (obj && obj->matches_keyword(ctx.arg(0))) {
                target_object = obj;
                break;
            }
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Handle different object types
    switch (target_object->type()) {
        case ObjectType::Wand:
        case ObjectType::Staff: {
            // Check if item has charges
            if (target_object->charges() <= 0) {
                ctx.send_error(fmt::format("{} has no charges left.",
                                          ctx.format_object_name(target_object)));
                return CommandResult::ResourceError;
            }

            // Determine target for wand/staff
            std::shared_ptr<Actor> spell_target = ctx.actor; // Default to self
            if (ctx.arg_count() > 1) {
                spell_target = ctx.find_actor_target(ctx.arg(1));
                if (!spell_target) {
                    ctx.send_error(fmt::format("You don't see '{}' here.", ctx.arg(1)));
                    return CommandResult::InvalidTarget;
                }
            }

            // Use a charge
            target_object->set_charges(target_object->charges() - 1);

            ctx.send_success(fmt::format("You wave {} and it flashes with magical energy!",
                                         ctx.format_object_name(target_object)));
            ctx.send_to_room(fmt::format("{} waves {} and it flashes with magical energy.",
                                         ctx.actor->display_name(), ctx.format_object_name(target_object)), true);

            // Apply spell effects from wand/staff (typically just the first spell)
            for (int spell_id : target_object->spell_ids()) {
                if (spell_id > 0) {
                    auto result = FieryMUD::AbilityExecutor::execute_by_id(ctx, spell_id, spell_target,
                                                                 target_object->spell_level());
                    if (result) {
                        if (!result->attacker_message.empty()) {
                            ctx.send(result->attacker_message);
                        }
                        if (spell_target != ctx.actor && !result->target_message.empty()) {
                            ctx.send_to_actor(spell_target, result->target_message);
                        }
                    }
                }
            }

            // Notify when charges run out
            if (target_object->charges() == 0) {
                ctx.send(fmt::format("{} is now depleted.", ctx.format_object_name(target_object)));
            } else if (target_object->charges() <= 3) {
                ctx.send(fmt::format("{} is running low on charges.", ctx.format_object_name(target_object)));
            }
            break;
        }
        case ObjectType::Potion:
            ctx.send("Use 'quaff' to drink potions.");
            return CommandResult::InvalidSyntax;
        case ObjectType::Scroll:
            ctx.send("Use 'recite' to use scrolls.");
            return CommandResult::InvalidSyntax;
        default:
            ctx.send_success(fmt::format("You use {}.", ctx.format_object_name(target_object)));
            ctx.send_to_room(fmt::format("{} uses {}.", ctx.actor->display_name(),
                                         ctx.format_object_name(target_object)), true);
            break;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_junk(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("junk <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find object in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> target_object = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            break;
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Remove and destroy the item
    if (!ctx.actor->inventory().remove_item(target_object)) {
        ctx.send_error("You can't junk that.");
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("You junk {} - it disintegrates into nothing.", ctx.format_object_name(target_object)));
    ctx.send_to_room(fmt::format("{} junks {} which disintegrates.", ctx.actor->display_name(),
                                 ctx.format_object_name(target_object)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_donate(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("donate <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find object in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> target_object = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            break;
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Remove from inventory
    if (!ctx.actor->inventory().remove_item(target_object)) {
        ctx.send_error("You can't donate that.");
        return CommandResult::ResourceError;
    }

    // TODO: Move item to donation room when donation system is implemented
    // For now, the item is simply removed

    ctx.send_success(fmt::format("You donate {}. It vanishes in a puff of smoke to help those in need.",
                                 ctx.format_object_name(target_object)));
    ctx.send_to_room(fmt::format("{} donates {} which vanishes in a puff of smoke.",
                                 ctx.actor->display_name(), ctx.format_object_name(target_object)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_compare(const CommandContext &ctx) {
    if (auto result = ctx.require_args(2, "<item1> <item2>"); !result) {
        ctx.send_usage("compare <item1> <item2>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find both items
    auto inventory_items = ctx.actor->inventory().get_all_items();
    auto equipped_items = ctx.actor->equipment().get_all_equipped();

    std::shared_ptr<Object> item1 = nullptr;
    std::shared_ptr<Object> item2 = nullptr;

    // Search for first item
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            item1 = obj;
            break;
        }
    }
    if (!item1) {
        for (const auto &obj : equipped_items) {
            if (obj && obj->matches_keyword(ctx.arg(0))) {
                item1 = obj;
                break;
            }
        }
    }

    // Search for second item
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(1)) && obj != item1) {
            item2 = obj;
            break;
        }
    }
    if (!item2) {
        for (const auto &obj : equipped_items) {
            if (obj && obj->matches_keyword(ctx.arg(1)) && obj != item1) {
                item2 = obj;
                break;
            }
        }
    }

    if (!item1) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    if (!item2) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(1)));
        return CommandResult::InvalidTarget;
    }

    // Compare the items
    ctx.send(fmt::format("Comparing {} and {}:", ctx.format_object_name(item1), ctx.format_object_name(item2)));

    // Weight comparison
    if (item1->weight() < item2->weight()) {
        ctx.send(fmt::format("  {} is lighter.", ctx.format_object_name(item1)));
    } else if (item1->weight() > item2->weight()) {
        ctx.send(fmt::format("  {} is heavier.", ctx.format_object_name(item1)));
    } else {
        ctx.send("  They weigh about the same.");
    }

    // Value comparison
    if (item1->value() < item2->value()) {
        ctx.send(fmt::format("  {} appears less valuable.", ctx.format_object_name(item1)));
    } else if (item1->value() > item2->value()) {
        ctx.send(fmt::format("  {} appears more valuable.", ctx.format_object_name(item1)));
    } else {
        ctx.send("  They appear to be of similar value.");
    }

    // Type comparison
    if (item1->type() != item2->type()) {
        ctx.send("  They are different types of items.");
    }

    return CommandResult::Success;
}

// =============================================================================
// Liquid Container Commands
// =============================================================================

Result<CommandResult> cmd_fill(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<container> [from <source>]"); !result) {
        ctx.send_usage("fill <container> [from <source>]");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor || !ctx.room) {
        return std::unexpected(Errors::InvalidState("No actor or room context"));
    }

    // Find container in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> container = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            if (obj->type() == ObjectType::Liquid_Container) {
                container = obj;
                break;
            }
        }
    }

    if (!container) {
        ctx.send_error(fmt::format("You don't have a container called '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if container is already full
    const auto& liquid = container->liquid_info();
    if (liquid.remaining >= liquid.capacity) {
        ctx.send_error(fmt::format("{} is already full.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Find water source (fountain) in room
    std::shared_ptr<Object> fountain = nullptr;
    for (const auto& obj : ctx.room->contents().objects) {
        if (obj && obj->type() == ObjectType::Fountain) {
            fountain = obj;
            break;
        }
    }

    if (!fountain) {
        ctx.send_error("There is no water source here to fill from.");
        return CommandResult::InvalidState;
    }

    // Fill the container with water from the fountain
    LiquidInfo new_liquid = liquid;
    new_liquid.liquid_type = "WATER";
    new_liquid.remaining = new_liquid.capacity;
    new_liquid.poisoned = false;
    container->set_liquid_info(new_liquid);

    ctx.send_success(fmt::format("You fill {} with water from {}.",
                                 ctx.format_object_name(container),
                                 ctx.format_object_name(fountain)));
    ctx.send_to_room(fmt::format("{} fills {} with water from {}.",
                                 ctx.actor->display_name(),
                                 ctx.format_object_name(container),
                                 ctx.format_object_name(fountain)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_pour(const CommandContext &ctx) {
    if (auto result = ctx.require_args(2, "<from-container> <to-container|out>"); !result) {
        ctx.send_usage("pour <from-container> <to-container|out>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find source container in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> source = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            if (obj->type() == ObjectType::Liquid_Container) {
                source = obj;
                break;
            }
        }
    }

    if (!source) {
        ctx.send_error(fmt::format("You don't have a container called '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if pouring out
    if (ctx.arg(1) == "out") {
        ctx.send_success(fmt::format("You pour the contents of {} out onto the ground.",
                                     ctx.format_object_name(source)));
        ctx.send_to_room(fmt::format("{} pours the contents of {} onto the ground.",
                                     ctx.actor->display_name(), ctx.format_object_name(source)), true);
        return CommandResult::Success;
    }

    // Find target container
    std::shared_ptr<Object> target = nullptr;
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(1)) && obj != source) {
            if (obj->type() == ObjectType::Liquid_Container) {
                target = obj;
                break;
            }
        }
    }

    if (!target) {
        ctx.send_error(fmt::format("You don't have a container called '{}'.", ctx.arg(1)));
        return CommandResult::InvalidTarget;
    }

    ctx.send_success(fmt::format("You pour the contents of {} into {}.",
                                 ctx.format_object_name(source), ctx.format_object_name(target)));
    ctx.send_to_room(fmt::format("{} pours from {} into {}.",
                                 ctx.actor->display_name(), ctx.format_object_name(source),
                                 ctx.format_object_name(target)), true);

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    // Object interaction commands
    Commands()
        .command("get", cmd_get)
        .alias("g")
        .alias("take")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("drop", cmd_drop)
        .alias("dr")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("put", cmd_put)
        .alias("p")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("give", cmd_give)
        .alias("gi")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("wear", cmd_wear)
        .alias("we")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("wield", cmd_wield)
        .alias("wi")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("remove", cmd_remove)
        .alias("rem")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Container and object interaction commands
    Commands()
        .command("open", cmd_open)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("close", cmd_close)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("lock", cmd_lock)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("unlock", cmd_unlock)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Consumable and utility commands
    Commands()
        .command("light", cmd_light)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("extinguish", cmd_extinguish)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("eat", cmd_eat)
        .alias("taste")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("drink", cmd_drink)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Shop commands
    Commands()
        .command("list", cmd_list)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("buy", cmd_buy)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("sell", cmd_sell)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Phase 1.4 object manipulation commands
    Commands()
        .command("hold", cmd_hold)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("grab", cmd_grab)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("quaff", cmd_quaff)
        .alias("qu")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("recite", cmd_recite)
        .alias("rec")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("use", cmd_use)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("junk", cmd_junk)
        .alias("trash")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("donate", cmd_donate)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("compare", cmd_compare)
        .alias("comp")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Liquid container commands
    Commands()
        .command("fill", cmd_fill)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("pour", cmd_pour)
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    return Success();
}

} // namespace ObjectCommands