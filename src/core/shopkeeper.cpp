#include "shopkeeper.hpp"

#include <algorithm>

#include <fmt/format.h>

#include "actor.hpp"
#include "logging.hpp"
#include "money.hpp"
#include "object.hpp"
#include "player.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"

// =============================================================================
// Shopkeeper Implementation
// =============================================================================

Shopkeeper::Shopkeeper(EntityId shop_id) : shop_id_(shop_id), shop_name_("Generic Shop") {
    Log::debug("Created shopkeeper with ID {}", shop_id_);
}

void Shopkeeper::add_item(const ShopItem &item) {
    items_[item.prototype_id] = item;
    Log::debug("Added item '{}' (ID: {}) to shop {} with price {}", item.name, item.prototype_id, shop_id_, item.cost);
}

void Shopkeeper::remove_item(EntityId prototype_id) {
    if (auto it = items_.find(prototype_id); it != items_.end()) {
        Log::debug("Removed item '{}' from shop {}", it->second.name, shop_id_);
        items_.erase(it);
    }
}

bool Shopkeeper::has_item(EntityId prototype_id) const { return items_.find(prototype_id) != items_.end(); }

const ShopItem *Shopkeeper::get_item(EntityId prototype_id) const {
    if (auto it = items_.find(prototype_id); it != items_.end()) {
        return &it->second;
    }
    return nullptr;
}

void Shopkeeper::restock_item(EntityId prototype_id, int quantity) {
    if (auto it = items_.find(prototype_id); it != items_.end()) {
        auto &item = it->second;
        if (item.max_stock >= 0) {
            item.stock = std::min(item.stock + quantity, item.max_stock);
            Log::debug("Restocked item '{}' in shop {}, now has {} items", item.name, shop_id_, item.stock);
        }
    }
}

void Shopkeeper::restock_all() {
    for (auto &[id, item] : items_) {
        if (item.max_stock >= 0 && item.stock < item.max_stock) {
            item.stock = item.max_stock;
        }
    }
    for (auto &[id, mob] : mobs_) {
        if (mob.max_stock >= 0 && mob.stock < mob.max_stock) {
            mob.stock = mob.max_stock;
        }
    }
    Log::debug("Fully restocked shop {}", shop_id_);
}

bool Shopkeeper::reduce_stock(EntityId prototype_id, int quantity) {
    if (auto it = items_.find(prototype_id); it != items_.end()) {
        auto &item = it->second;
        if (item.stock == -1) {
            return true; // Unlimited stock
        }
        if (item.stock >= quantity) {
            item.stock -= quantity;
            Log::debug("Reduced stock for item '{}' in shop {}, {} remaining", item.name, shop_id_, item.stock);
            return true;
        }
    }
    return false;
}

// =============================================================================
// Mob Management (Pet/Mount Shops)
// =============================================================================

void Shopkeeper::add_mob(const ShopMob &mob) {
    mobs_[mob.prototype_id] = mob;
    sells_mobs_ = true;
    Log::debug("Added mob '{}' (ID: {}) to shop {} with price {}", mob.name, mob.prototype_id, shop_id_, mob.cost);
}

void Shopkeeper::remove_mob(EntityId prototype_id) {
    if (auto it = mobs_.find(prototype_id); it != mobs_.end()) {
        Log::debug("Removed mob '{}' from shop {}", it->second.name, shop_id_);
        mobs_.erase(it);
    }
    if (mobs_.empty()) {
        sells_mobs_ = false;
    }
}

bool Shopkeeper::has_mob(EntityId prototype_id) const { return mobs_.find(prototype_id) != mobs_.end(); }

const ShopMob *Shopkeeper::get_mob(EntityId prototype_id) const {
    if (auto it = mobs_.find(prototype_id); it != mobs_.end()) {
        return &it->second;
    }
    return nullptr;
}

void Shopkeeper::restock_mob(EntityId prototype_id, int quantity) {
    if (auto it = mobs_.find(prototype_id); it != mobs_.end()) {
        auto &mob = it->second;
        if (mob.max_stock >= 0) {
            mob.stock = std::min(mob.stock + quantity, mob.max_stock);
            Log::debug("Restocked mob '{}' in shop {}, now has {} available", mob.name, shop_id_, mob.stock);
        }
    }
}

bool Shopkeeper::reduce_mob_stock(EntityId prototype_id, int quantity) {
    if (auto it = mobs_.find(prototype_id); it != mobs_.end()) {
        auto &mob = it->second;
        if (mob.stock == -1) {
            return true; // Unlimited stock
        }
        if (mob.stock >= quantity) {
            mob.stock -= quantity;
            Log::debug("Reduced stock for mob '{}' in shop {}, {} remaining", mob.name, shop_id_, mob.stock);
            return true;
        }
    }
    return false;
}

std::vector<ShopMob> Shopkeeper::get_available_mobs() const {
    std::vector<ShopMob> available;
    for (const auto &[id, mob] : mobs_) {
        if (mob.stock != 0) {
            available.push_back(mob);
        }
    }
    return available;
}

std::vector<ShopItem> Shopkeeper::get_available_items() const {
    std::vector<ShopItem> available;
    for (const auto &[id, item] : items_) {
        if (item.stock != 0) {
            available.push_back(item);
        }
    }
    return available;
}

std::vector<std::string> Shopkeeper::get_shop_listing() const {
    std::vector<std::string> listing;

    // Show items if available
    auto available_items = get_available_items();
    if (!available_items.empty()) {
        listing.push_back(fmt::format("=== {} ===", shop_name_));
        listing.push_back("");

        // Table header
        listing.push_back(" ##  Lvl  Qty  Item                                          Cost");
        listing.push_back("---  ---  ---  --------------------------------------------  -------------");

        // Sort by level, then by price for consistent display
        std::sort(available_items.begin(), available_items.end(), [](const ShopItem &a, const ShopItem &b) {
            if (a.level != b.level)
                return a.level < b.level;
            return a.cost < b.cost;
        });

        int row = 1;
        for (const auto &item : available_items) {
            int actual_price = calculate_buy_price(item);
            auto price_money = fiery::Money::from_copper(actual_price);
            std::string price_str = price_money.to_shop_format(true);
            std::string qty_str = (item.stock == -1) ? " - " : fmt::format("{:>3}", item.stock);

            listing.push_back(
                fmt::format("{:>2})  {:>3}  {}  {:<44}  {}", row, item.level, qty_str, item.name, price_str));
            ++row;
        }
    }

    // Show mobs if this is a pet/mount shop
    if (sells_mobs_) {
        auto available_mobs = get_available_mobs();
        if (!available_mobs.empty()) {
            if (!available_items.empty()) {
                listing.push_back("");
            }
            listing.push_back("Pets/Mounts for sale:");
            listing.push_back("");

            // Table header
            listing.push_back(" ##  Lvl  Qty  Pet/Mount                                     Cost");
            listing.push_back("---  ---  ---  --------------------------------------------  -------------");

            // Sort by level, then by price for consistent display
            std::sort(available_mobs.begin(), available_mobs.end(), [](const ShopMob &a, const ShopMob &b) {
                if (a.level != b.level)
                    return a.level < b.level;
                return a.cost < b.cost;
            });

            int row = 1;
            for (const auto &mob : available_mobs) {
                int actual_price = calculate_buy_price(mob);
                auto price_money = fiery::Money::from_copper(actual_price);
                std::string price_str = price_money.to_shop_format(true);
                std::string qty_str = (mob.stock == -1) ? " - " : fmt::format("{:>3}", mob.stock);

                listing.push_back(
                    fmt::format("{:>2})  {:>3}  {}  {:<44}  {}", row, mob.level, qty_str, mob.name, price_str));
                ++row;
            }
        }
    }

    // Handle case where nothing is available
    if (available_items.empty() && (!sells_mobs_ || get_available_mobs().empty())) {
        listing.push_back(fmt::format("=== {} ===", shop_name_));
        listing.push_back("");
        listing.push_back("  Nothing currently available.");
    }

    return listing;
}

int Shopkeeper::calculate_buy_price(const ShopItem &item) const { return static_cast<int>(item.cost * buy_rate_); }

int Shopkeeper::calculate_buy_price(const ShopMob &mob) const { return static_cast<int>(mob.cost * buy_rate_); }

int Shopkeeper::calculate_sell_price(const Object &obj) const {
    // Base sell price on object's value (stored in copper)
    int base_value = obj.value();
    if (base_value <= 0) {
        base_value = 1; // Minimum 1 copper for worthless items
    }
    return static_cast<int>(base_value * sell_rate_);
}

// =============================================================================
// ShopManager Implementation
// =============================================================================

ShopManager &ShopManager::instance() {
    static ShopManager instance_;
    return instance_;
}

void ShopManager::register_shopkeeper(EntityId shopkeeper_id, std::unique_ptr<Shopkeeper> shop) {
    shops_[shopkeeper_id] = std::move(shop);
    Log::trace("Registered shopkeeper {} with shop '{}'", shopkeeper_id, shops_[shopkeeper_id]->get_name());
}

void ShopManager::unregister_shopkeeper(EntityId shopkeeper_id) {
    if (shops_.erase(shopkeeper_id)) {
        Log::trace("Unregistered shopkeeper {}", shopkeeper_id);
    }
}

Shopkeeper *ShopManager::get_shopkeeper(EntityId shopkeeper_id) {
    if (auto it = shops_.find(shopkeeper_id); it != shops_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const Shopkeeper *ShopManager::get_shopkeeper(EntityId shopkeeper_id) const {
    if (auto it = shops_.find(shopkeeper_id); it != shops_.end()) {
        return it->second.get();
    }
    return nullptr;
}

ShopResult ShopManager::buy_item(std::shared_ptr<Actor> buyer, EntityId shopkeeper_id, EntityId item_id) {
    if (!buyer) {
        return ShopResult::InvalidItem;
    }

    // Only players can buy items
    auto player = std::dynamic_pointer_cast<Player>(buyer);
    if (!player) {
        return ShopResult::InvalidItem;
    }

    auto *shop = get_shopkeeper(shopkeeper_id);
    if (!shop) {
        return ShopResult::NoShopkeeper;
    }

    const auto *shop_item = shop->get_item(item_id);
    if (!shop_item) {
        return ShopResult::ItemNotFound;
    }

    if (shop_item->stock == 0) {
        return ShopResult::InsufficientStock;
    }

    int price = shop->calculate_buy_price(*shop_item);

    // Gods get items for free
    bool is_god = player->is_god();

    Log::info("buy_item: player={}, price={}, wallet={}, is_god={}, can_afford={}", player->name(), price,
              player->wealth(), is_god, player->can_afford(price));

    // Check if player has enough money (price is in copper) - gods bypass this
    if (!is_god && !player->can_afford(price)) {
        return ShopResult::InsufficientFunds;
    }

    // Reduce stock and deduct payment
    if (!shop->reduce_stock(item_id, 1)) {
        return ShopResult::InsufficientStock;
    }

    // Deduct money from buyer's wallet (gods don't pay)
    if (!is_god) {
        if (!player->spend(price)) {
            // Shouldn't happen since we already checked can_afford()
            return ShopResult::InsufficientFunds;
        }
        Log::info("Player {} bought item '{}' from shopkeeper {} for {} copper", player->name(), shop_item->name,
                  shopkeeper_id, price);
    } else {
        Log::info("God {} took item '{}' from shopkeeper {} (free)", player->name(), shop_item->name, shopkeeper_id);
    }

    return ShopResult::Success;
}

ShopResult ShopManager::buy_mob(std::shared_ptr<Actor> buyer, EntityId shopkeeper_id, EntityId mob_id) {
    if (!buyer) {
        return ShopResult::InvalidItem;
    }

    // Only players can buy mobs
    auto player = std::dynamic_pointer_cast<Player>(buyer);
    if (!player) {
        return ShopResult::InvalidItem;
    }

    auto *shop = get_shopkeeper(shopkeeper_id);
    if (!shop) {
        return ShopResult::NoShopkeeper;
    }

    if (!shop->sells_mobs()) {
        return ShopResult::ItemNotFound;
    }

    const auto *shop_mob = shop->get_mob(mob_id);
    if (!shop_mob) {
        return ShopResult::ItemNotFound;
    }

    if (shop_mob->stock == 0) {
        return ShopResult::InsufficientStock;
    }

    int price = shop->calculate_buy_price(*shop_mob);

    // Gods get pets for free
    bool is_god = player->is_god();

    Log::info("buy_mob: player={}, price={}, wallet={}, is_god={}, can_afford={}", player->name(), price,
              player->wealth(), is_god, player->can_afford(price));

    // Check if player has enough money (price is in copper) - gods bypass this
    if (!is_god && !player->can_afford(price)) {
        return ShopResult::InsufficientFunds;
    }

    // Reduce stock and deduct payment
    if (!shop->reduce_mob_stock(mob_id, 1)) {
        return ShopResult::InsufficientStock;
    }

    // Deduct money from buyer's wallet (gods don't pay)
    if (!is_god) {
        if (!player->spend(price)) {
            // Shouldn't happen since we already checked can_afford()
            return ShopResult::InsufficientFunds;
        }
        Log::info("Player {} bought pet '{}' from shopkeeper {} for {} copper", player->name(), shop_mob->name,
                  shopkeeper_id, price);
    } else {
        Log::info("God {} took pet '{}' from shopkeeper {} (free)", player->name(), shop_mob->name, shopkeeper_id);
    }

    // TODO: Create the actual pet mob instance in the room
    // The pet naming will be handled by the calling command, which prompts:
    // "What would you like to name your new pet?"
    // Then sets pet->set_custom_name(name) which displays as:
    // "A kitten named 'Meow' is here."

    return ShopResult::Success;
}

ShopResult ShopManager::sell_item(std::shared_ptr<Actor> seller, EntityId shopkeeper_id, std::shared_ptr<Object> item) {
    if (!seller || !item) {
        return ShopResult::InvalidItem;
    }

    // Only players can sell items
    auto player = std::dynamic_pointer_cast<Player>(seller);
    if (!player) {
        return ShopResult::InvalidItem;
    }

    auto *shop = get_shopkeeper(shopkeeper_id);
    if (!shop) {
        return ShopResult::NoShopkeeper;
    }

    int price = shop->calculate_sell_price(*item);

    // Remove item from player inventory and give money
    if (!player->inventory().remove_item(item->id())) {
        return ShopResult::InvalidItem; // Player doesn't have the item
    }

    // Give money to seller (auto-converts to optimal denominations)
    player->receive(price);

    Log::info("Player {} sold item '{}' to shopkeeper {} for {} copper", player->name(), item->name(), shopkeeper_id,
              price);

    return ShopResult::Success;
}

std::vector<EntityId> ShopManager::find_shops_in_room(EntityId room_id) const {
    std::vector<EntityId> shops_in_room;

    // Get the room from WorldManager to check its contents
    auto room = WorldManager::instance().get_room(room_id);
    if (!room) {
        return shops_in_room; // Room doesn't exist
    }

    // Check each actor in the room to see if they're a registered shopkeeper
    for (const auto &actor : room->contents().actors) {
        if (actor && shops_.find(actor->id()) != shops_.end()) {
            shops_in_room.push_back(actor->id());
        }
    }

    return shops_in_room;
}
