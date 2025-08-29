/***************************************************************************
 *   File: src/commands/object_commands.cpp                Part of FieryMUD *
 *  Usage: Object interaction command implementations                        *
 *                                                                           *
 *  All rights reserved.  See license.doc for complete information.         *
 *                                                                           *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "object_commands.hpp"
#include "../core/actor.hpp"
#include "../core/object.hpp"
#include "../core/logging.hpp"
#include "../world/room.hpp"

#include <algorithm>

namespace ObjectCommands {

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

    // Check if this is "get <object> from <container>" syntax
    if (ctx.arg_count() >= 3 && ctx.arg(1) == "from") {
        // Get from container
        auto inventory_items = ctx.actor->inventory().get_all_items();

        // First check inventory for the object
        std::shared_ptr<Object> potential_container = nullptr;
        for (const auto &obj : inventory_items) {
            if (obj && obj->matches_keyword(ctx.arg(2))) {
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
                if (obj && obj->matches_keyword(ctx.arg(2))) {
                    potential_container = obj;
                    if (obj->is_container()) {
                        source_container = obj;
                    }
                    break;
                }
            }
        }

        if (!potential_container) {
            ctx.send_error(fmt::format("You don't see a container called '{}' here.", ctx.arg(2)));
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
            auto items_to_get = all_items; // Copy to avoid iterator invalidation

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

                // Remove from container and add to inventory
                if (container_obj->remove_item(item)) {
                    auto add_result = ctx.actor->inventory().add_item(item);
                    if (add_result) {
                        gotten_count++;
                        gotten_names.push_back(ctx.format_object_name(item));
                    } else {
                        // Return item to container if inventory add failed
                        container_obj->add_item(item);
                    }
                }
            }

            if (gotten_count > 0) {
                if (gotten_count == 1) {
                    ctx.send_success(fmt::format("You get {} from {}.", gotten_names[0], ctx.format_object_name(source_container)));
                    ctx.send_to_room(fmt::format("{} gets {} from {}.", ctx.actor->name(), gotten_names[0], ctx.format_object_name(source_container)), true);
                } else {
                    ctx.send_success(fmt::format("You get {} items from {}.", gotten_count, ctx.format_object_name(source_container)));
                    ctx.send_to_room(fmt::format("{} gets several items from {}.", ctx.actor->name(), ctx.format_object_name(source_container)), true);
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

        // Check if actor can carry more weight
        int object_weight = item_to_get->weight();
        if (!ctx.actor->inventory().can_carry(object_weight, ctx.actor->max_carry_weight())) {
            ctx.send_error("You can't carry any more.");
            return CommandResult::ResourceError;
        }

        // Remove from container and add to inventory
        if (!container_obj->remove_item(item_to_get)) {
            ctx.send_error("Failed to get item from container.");
            return CommandResult::ResourceError;
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
        ctx.send_to_room(fmt::format("{} gets {} from {}.", ctx.actor->name(), ctx.format_object_name(item_to_get),
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

            for (const auto &obj : objects_to_get) {
                if (!obj) continue;

                // Check if actor can carry more weight
                int object_weight = obj->weight();
                if (!ctx.actor->inventory().can_carry(object_weight, ctx.actor->max_carry_weight())) {
                    ctx.send_error(fmt::format("The {} is too heavy to pick up.", ctx.format_object_name(obj)));
                    continue; // Skip this item and try the next one
                }

                // Remove from room and add to inventory
                auto it = std::find(room_objects.begin(), room_objects.end(), obj);
                if (it != room_objects.end()) {
                    room_objects.erase(it);
                    
                    auto add_result = ctx.actor->inventory().add_item(obj);
                    if (add_result) {
                        gotten_count++;
                        gotten_names.push_back(ctx.format_object_name(obj));
                    } else {
                        // Return object to room if inventory add failed
                        room_objects.push_back(obj);
                    }
                }
            }

            if (gotten_count > 0) {
                if (gotten_count == 1) {
                    ctx.send_success(fmt::format("You get {}.", gotten_names[0]));
                    ctx.send_to_room(fmt::format("{} gets {}.", ctx.actor->name(), gotten_names[0]), true);
                } else {
                    ctx.send_success(fmt::format("You get {} items.", gotten_count));
                    ctx.send_to_room(fmt::format("{} gets several items.", ctx.actor->name()), true);
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
            // Return object to room if inventory add failed
            room_objects.push_back(target_object);
            ctx.send_error(add_result.error().message);
            return CommandResult::ResourceError;
        }

        ctx.send_success(fmt::format("You get {}.", ctx.format_object_name(target_object)));
        ctx.send_to_room(fmt::format("{} gets {}.", ctx.actor->name(), ctx.format_object_name(target_object)), true);

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
                }
            }
        }

        if (dropped_count > 0) {
            if (dropped_count == 1) {
                ctx.send_success(fmt::format("You drop {}.", dropped_names[0]));
                ctx.send_to_room(fmt::format("{} drops {}.", ctx.actor->name(), dropped_names[0]), true);
            } else {
                ctx.send_success(fmt::format("You drop {} items.", dropped_count));
                ctx.send_to_room(fmt::format("{} drops several items.", ctx.actor->name()), true);
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
    ctx.send_to_room(fmt::format("{} drops {}.", ctx.actor->name(), ctx.format_object_name(target_object)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_put(const CommandContext &ctx) {
    if (ctx.arg_count() < 2) {
        ctx.send_usage("put <object> [in] <container>");
        return CommandResult::InvalidSyntax;
    }
    
    // Handle both "put object container" and "put object in container" syntax
    std::string_view object_name = ctx.arg(0);
    std::string_view container_name;
    
    if (ctx.arg_count() >= 3 && ctx.arg(1) == "in") {
        // "put object in container" syntax
        container_name = ctx.arg(2);
    } else {
        // "put object container" syntax  
        container_name = ctx.arg(1);
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    // Find object in actor's inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> item_to_put = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(object_name)) {
            item_to_put = obj;
            break;
        }
    }

    if (!item_to_put) {
        ctx.send_error(fmt::format("You don't have '{}'.", object_name));
        return CommandResult::InvalidTarget;
    }

    // Find container in the room or inventory
    std::shared_ptr<Object> container = nullptr;
    std::shared_ptr<Object> potential_container = nullptr;

    // First check inventory 
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(container_name) && obj != item_to_put) {
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
        ctx.send_error(fmt::format("You can't put anything in {} - it's not a container.", ctx.format_object_name(potential_container)));
        return CommandResult::InvalidTarget;
    }

    // Check container properties
    const auto &container_info = container->container_info();

    // Check if container is closed
    if (container_info.closeable && container_info.closed) {
        ctx.send_error(fmt::format("The {} is closed.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // DEBUG: Add comprehensive container debugging information
    spdlog::info("=== PUT COMMAND DEBUG ===");
    spdlog::info("Container name: {}", container->name());
    spdlog::info("Container type_name(): {}", container->type_name());
    spdlog::info("Container is_container(): {}", container->is_container());
    spdlog::info("Container object type: {}", static_cast<int>(container->type()));
    spdlog::info("Container ptr address: {}", static_cast<void*>(container.get()));
    
    // Try to cast to Container to access container functionality
    auto container_obj = std::dynamic_pointer_cast<Container>(container);
    spdlog::info("dynamic_pointer_cast<Container> result: {}", static_cast<void*>(container_obj.get()));
    
    if (!container_obj) {
        spdlog::error("CONTAINER CAST FAILED - Object claims to be container but cast failed!");
        spdlog::error("This indicates object was created as base Object, not Container subclass");
        ctx.send_error(fmt::format("You can't put anything in {} [DEBUG: cast failed].", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }
    
    spdlog::info("Container cast successful! Capacity: {}", container_obj->container_info().capacity);
    spdlog::info("=========================");

    // Try to add item to container
    auto add_result = container_obj->add_item(item_to_put);
    if (!add_result) {
        ctx.send_error(fmt::format("The {} is full.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Remove from actor's inventory
    if (!ctx.actor->inventory().remove_item(item_to_put)) {
        // If we can't remove from inventory, remove from container to maintain consistency
        container_obj->remove_item(item_to_put);
        ctx.send_error("Failed to remove item from inventory.");
        return CommandResult::ResourceError;
    }

    ctx.send_success(
        fmt::format("You put {} in {}.", ctx.format_object_name(item_to_put), ctx.format_object_name(container)));
    ctx.send_to_room(fmt::format("{} puts {} in {}.", ctx.actor->name(), ctx.format_object_name(item_to_put),
                                 ctx.format_object_name(container)),
                     true);

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

    // TODO: Implement actual give logic
    ctx.send_success(fmt::format("You give {} to {}.", ctx.format_object_name(obj), target->name()));

    ctx.send_to_actor(target, fmt::format("{} gives you {}.", ctx.actor->name(), ctx.format_object_name(obj)));

    ctx.send_to_room(fmt::format("{} gives {} to {}.", ctx.actor->name(), ctx.format_object_name(obj), target->name()),
                     true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_wear(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("wear <object>");
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
    ctx.send_to_room(fmt::format("{} wears {}.", ctx.actor->name(), ctx.format_object_name(target_object)), true);

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
    ctx.send_to_room(fmt::format("{} wields {}.", ctx.actor->name(), ctx.format_object_name(target_object)), true);

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
    ctx.send_to_room(fmt::format("{} removes {}.", ctx.actor->name(), ctx.format_object_name(target_object)), true);

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
    ctx.send_to_room(fmt::format("{} opens {}.", ctx.actor->name(), ctx.format_object_name(target_obj)), true);

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
    ctx.send_to_room(fmt::format("{} closes {}.", ctx.actor->name(), ctx.format_object_name(target_obj)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_lock(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("lock <container|door>");
        return CommandResult::InvalidSyntax;
    }

    // TODO: Implement container and door locking
    ctx.send_error("Locking containers and doors is not yet implemented.");
    return CommandResult::SystemError;
}

Result<CommandResult> cmd_unlock(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("unlock <container|door>");
        return CommandResult::InvalidSyntax;
    }

    // TODO: Implement container and door unlocking
    ctx.send_error("Unlocking containers and doors is not yet implemented.");
    return CommandResult::SystemError;
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

    // Find light source in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> light_source = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_light_source()) {
            light_source = obj;
            break;
        }
    }

    if (!light_source) {
        ctx.send_error(fmt::format("You don't have a light source called '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if already lit
    const auto &light_info = light_source->light_info();
    if (light_info.lit) {
        ctx.send_error(fmt::format("The {} is already lit.", ctx.format_object_name(light_source)));
        return CommandResult::InvalidState;
    }

    // Check if light source has duration remaining
    if (light_info.duration <= 0) {
        ctx.send_error(fmt::format("The {} is burnt out and cannot be lit.", ctx.format_object_name(light_source)));
        return CommandResult::InvalidState;
    }

    // Light the object
    auto new_light_info = light_info;
    new_light_info.lit = true;
    light_source->set_light_info(new_light_info);

    ctx.send_success(fmt::format("You light the {}. It glows brightly.", ctx.format_object_name(light_source)));
    ctx.send_to_room(fmt::format("{} lights {}.", ctx.actor->name(), ctx.format_object_name(light_source)), true);

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
        ctx.send_to_room(fmt::format("{} eats {}.", ctx.actor->name(), ctx.format_object_name(food_item)), true);
    } else {
        ctx.send_success(fmt::format("You drink the {}. You feel refreshed.", ctx.format_object_name(food_item)));
        ctx.send_to_room(fmt::format("{} drinks {}.", ctx.actor->name(), ctx.format_object_name(food_item)), true);
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

    // Find drinkable item in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> drink_item = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(target_name)) {
            if (obj->type() == ObjectType::Liquid_Container || 
                obj->type() == ObjectType::Potion || 
                obj->type() == ObjectType::Food) {
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
        ctx.send_to_room(fmt::format("{} drinks {}.", ctx.actor->name(), ctx.format_object_name(drink_item)), true);
    } else if (drink_item->type() == ObjectType::Liquid_Container) {
        // Liquid containers can be drunk from multiple times
        ctx.send_success(fmt::format("You drink from the {}. The liquid is refreshing.", 
                                   ctx.format_object_name(drink_item)));
        ctx.send_to_room(fmt::format("{} drinks from {}.", ctx.actor->name(), ctx.format_object_name(drink_item)), true);
    } else {
        // Food items (like fruits with juice)
        ctx.send_success(fmt::format("You drink from the {}. It quenches your thirst.", 
                                   ctx.format_object_name(drink_item)));
        ctx.send_to_room(fmt::format("{} drinks from {}.", ctx.actor->name(), ctx.format_object_name(drink_item)), true);
    }

    return CommandResult::Success;
}

} // namespace ObjectCommands