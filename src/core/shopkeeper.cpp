#include "shopkeeper.hpp"
#include "actor.hpp"
#include "object.hpp"
#include "logging.hpp"
#include "../world/world_manager.hpp"
#include <algorithm>
#include <fmt/format.h>

// =============================================================================
// Shopkeeper Implementation
// =============================================================================

Shopkeeper::Shopkeeper(EntityId shop_id) : shop_id_(shop_id), shop_name_("Generic Shop") {
    Log::debug("Created shopkeeper with ID {}", shop_id_.value());
}

void Shopkeeper::add_item(const ShopItem& item) {
    items_[item.prototype_id] = item;
    Log::debug("Added item '{}' (ID: {}) to shop {} with price {}", 
               item.name, item.prototype_id.value(), shop_id_.value(), item.cost);
}

void Shopkeeper::remove_item(EntityId prototype_id) {
    if (auto it = items_.find(prototype_id); it != items_.end()) {
        Log::debug("Removed item '{}' from shop {}", it->second.name, shop_id_.value());
        items_.erase(it);
    }
}

bool Shopkeeper::has_item(EntityId prototype_id) const {
    return items_.find(prototype_id) != items_.end();
}

const ShopItem* Shopkeeper::get_item(EntityId prototype_id) const {
    if (auto it = items_.find(prototype_id); it != items_.end()) {
        return &it->second;
    }
    return nullptr;
}

void Shopkeeper::restock_item(EntityId prototype_id, int quantity) {
    if (auto it = items_.find(prototype_id); it != items_.end()) {
        auto& item = it->second;
        if (item.max_stock >= 0) {
            item.stock = std::min(item.stock + quantity, item.max_stock);
            Log::debug("Restocked item '{}' in shop {}, now has {} items", 
                      item.name, shop_id_.value(), item.stock);
        }
    }
}

void Shopkeeper::restock_all() {
    for (auto& [id, item] : items_) {
        if (item.max_stock >= 0 && item.stock < item.max_stock) {
            item.stock = item.max_stock;
        }
    }
    Log::debug("Fully restocked shop {}", shop_id_.value());
}

bool Shopkeeper::reduce_stock(EntityId prototype_id, int quantity) {
    if (auto it = items_.find(prototype_id); it != items_.end()) {
        auto& item = it->second;
        if (item.stock == -1) {
            return true; // Unlimited stock
        }
        if (item.stock >= quantity) {
            item.stock -= quantity;
            Log::debug("Reduced stock for item '{}' in shop {}, {} remaining", 
                      item.name, shop_id_.value(), item.stock);
            return true;
        }
    }
    return false;
}

std::vector<ShopItem> Shopkeeper::get_available_items() const {
    std::vector<ShopItem> available;
    for (const auto& [id, item] : items_) {
        if (item.stock != 0) {
            available.push_back(item);
        }
    }
    return available;
}

std::vector<std::string> Shopkeeper::get_shop_listing() const {
    std::vector<std::string> listing;
    listing.push_back(fmt::format("=== {} ===", shop_name_));
    listing.push_back(fmt::format("Items for sale:"));
    listing.push_back("");
    
    auto available = get_available_items();
    if (available.empty()) {
        listing.push_back("  No items currently available.");
        return listing;
    }
    
    // Sort by price for consistent display
    std::sort(available.begin(), available.end(), 
              [](const ShopItem& a, const ShopItem& b) { return a.cost < b.cost; });
    
    for (const auto& item : available) {
        std::string stock_info = (item.stock == -1) ? "unlimited" : std::to_string(item.stock);
        std::string price_str = fmt::format("{} copper", item.cost);
        
        listing.push_back(fmt::format("  {:<30} {:<15} ({})", 
                                    item.name, price_str, stock_info));
        if (!item.description.empty()) {
            listing.push_back(fmt::format("    {}", item.description));
        }
    }
    
    return listing;
}

int Shopkeeper::calculate_buy_price(const ShopItem& item) const {
    return static_cast<int>(item.cost * buy_rate_);
}

int Shopkeeper::calculate_sell_price(const Object& /* obj */) const {
    // Base sell price on object value or type-specific pricing
    int base_value = 100; // Default value - could be object property
    return static_cast<int>(base_value * sell_rate_);
}

// =============================================================================
// ShopManager Implementation
// =============================================================================

ShopManager& ShopManager::instance() {
    static ShopManager instance_;
    return instance_;
}

void ShopManager::register_shopkeeper(EntityId shopkeeper_id, std::unique_ptr<Shopkeeper> shop) {
    shops_[shopkeeper_id] = std::move(shop);
    Log::info("Registered shopkeeper {} with shop '{}'", 
              shopkeeper_id.value(), shops_[shopkeeper_id]->get_name());
}

void ShopManager::unregister_shopkeeper(EntityId shopkeeper_id) {
    if (shops_.erase(shopkeeper_id)) {
        Log::info("Unregistered shopkeeper {}", shopkeeper_id.value());
    }
}

Shopkeeper* ShopManager::get_shopkeeper(EntityId shopkeeper_id) {
    if (auto it = shops_.find(shopkeeper_id); it != shops_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const Shopkeeper* ShopManager::get_shopkeeper(EntityId shopkeeper_id) const {
    if (auto it = shops_.find(shopkeeper_id); it != shops_.end()) {
        return it->second.get();
    }
    return nullptr;
}

ShopResult ShopManager::buy_item(std::shared_ptr<Actor> buyer, EntityId shopkeeper_id, EntityId item_id) {
    if (!buyer) {
        return ShopResult::InvalidItem;
    }
    
    auto* shop = get_shopkeeper(shopkeeper_id);
    if (!shop) {
        return ShopResult::NoShopkeeper;
    }
    
    const auto* shop_item = shop->get_item(item_id);
    if (!shop_item) {
        return ShopResult::ItemNotFound;
    }
    
    if (shop_item->stock == 0) {
        return ShopResult::InsufficientStock;
    }
    
    int price = shop->calculate_buy_price(*shop_item);
    
    // Check if player has enough money (in copper coins)
    if (buyer->stats().gold < price) {
        return ShopResult::InsufficientFunds;
    }
    
    // Reduce stock and deduct payment
    if (!shop->reduce_stock(item_id, 1)) {
        return ShopResult::InsufficientStock;
    }
    
    // Deduct money from buyer
    buyer->stats().gold -= price;
    
    Log::info("Player {} bought item '{}' from shopkeeper {} for {} copper", 
              buyer->name(), shop_item->name, shopkeeper_id.value(), price);
    
    return ShopResult::Success;
}

ShopResult ShopManager::sell_item(std::shared_ptr<Actor> seller, EntityId shopkeeper_id, std::shared_ptr<Object> item) {
    if (!seller || !item) {
        return ShopResult::InvalidItem;
    }
    
    auto* shop = get_shopkeeper(shopkeeper_id);
    if (!shop) {
        return ShopResult::NoShopkeeper;
    }
    
    int price = shop->calculate_sell_price(*item);
    
    // Remove item from player inventory and give money
    if (!seller->inventory().remove_item(item->id())) {
        return ShopResult::InvalidItem; // Player doesn't have the item
    }
    
    // Give money to seller  
    seller->stats().gold += price;
    
    Log::info("Player {} sold item '{}' to shopkeeper {} for {} copper", 
              seller->name(), item->name(), shopkeeper_id.value(), price);
    
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
    for (const auto& actor : room->contents().actors) {
        if (actor && shops_.find(actor->id()) != shops_.end()) {
            shops_in_room.push_back(actor->id());
        }
    }
    
    return shops_in_room;
}