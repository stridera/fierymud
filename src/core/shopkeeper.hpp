#pragma once

#include "ids.hpp"
#include "result.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

// Forward declarations
class Object;
class Actor;

/**
 * @brief Item available for purchase in a shop
 */
struct ShopItem {
    EntityId prototype_id;     // Object prototype to create
    std::string name;          // Display name
    std::string description;   // Item description
    int cost;                 // Cost in copper coins
    int level;                // Minimum level to use item
    int stock;                // Available quantity (-1 for unlimited)
    int max_stock;            // Maximum stock level for restocking

    ShopItem() = default;
    ShopItem(EntityId id, std::string_view item_name, std::string_view desc, int price, int item_level = 1, int quantity = -1)
        : prototype_id(id), name(item_name), description(desc), cost(price), level(item_level), stock(quantity), max_stock(quantity) {}
};

/**
 * @brief Mob available for purchase in a pet/mount shop
 */
struct ShopMob {
    EntityId prototype_id;     // Mob prototype to create
    std::string name;          // Display name
    std::string description;   // Mob description
    int cost;                 // Cost in copper coins
    int level;                // Level of the mob
    int stock;                // Available quantity (-1 for unlimited)
    int max_stock;            // Maximum stock level for restocking

    ShopMob() = default;
    ShopMob(EntityId id, std::string_view mob_name, std::string_view desc, int price, int mob_level = 1, int quantity = -1)
        : prototype_id(id), name(mob_name), description(desc), cost(price), level(mob_level), stock(quantity), max_stock(quantity) {}
};

/**
 * @brief Shopkeeper data and functionality
 */
class Shopkeeper {
public:
    explicit Shopkeeper(EntityId shop_id);

    // Item management
    void add_item(const ShopItem& item);
    void remove_item(EntityId prototype_id);
    bool has_item(EntityId prototype_id) const;
    const ShopItem* get_item(EntityId prototype_id) const;

    // Mob management (pet/mount shops)
    void add_mob(const ShopMob& mob);
    void remove_mob(EntityId prototype_id);
    bool has_mob(EntityId prototype_id) const;
    const ShopMob* get_mob(EntityId prototype_id) const;

    // Stock management
    void restock_item(EntityId prototype_id, int quantity);
    void restock_mob(EntityId prototype_id, int quantity);
    void restock_all();
    bool reduce_stock(EntityId prototype_id, int quantity = 1);
    bool reduce_mob_stock(EntityId prototype_id, int quantity = 1);

    // Shop operations
    std::vector<ShopItem> get_available_items() const;
    std::vector<ShopMob> get_available_mobs() const;
    std::vector<std::string> get_shop_listing() const;

    // Shop flags
    void set_sells_mobs(bool sells) { sells_mobs_ = sells; }
    bool sells_mobs() const { return sells_mobs_; }

    // Buy/sell rates
    void set_buy_rate(float rate) { buy_rate_ = rate; }
    void set_sell_rate(float rate) { sell_rate_ = rate; }
    float get_buy_rate() const { return buy_rate_; }
    float get_sell_rate() const { return sell_rate_; }

    // Shop properties
    void set_name(std::string_view name) { shop_name_ = name; }
    const std::string& get_name() const { return shop_name_; }

    EntityId get_shop_id() const { return shop_id_; }

    // Currency handling
    int calculate_buy_price(const ShopItem& item) const;
    int calculate_buy_price(const ShopMob& mob) const;
    int calculate_sell_price(const Object& obj) const;

private:
    EntityId shop_id_;
    std::string shop_name_;
    std::unordered_map<EntityId, ShopItem> items_;
    std::unordered_map<EntityId, ShopMob> mobs_;
    float buy_rate_ = 1.0f;    // Player buys at this rate (1.0 = full price)
    float sell_rate_ = 0.5f;   // Player sells at this rate (0.5 = half price)
    bool sells_mobs_ = false;  // True if this shop sells mobs (pet/mount shop)
};

/**
 * @brief Shop command results for client feedback
 */
enum class ShopResult {
    Success,
    NoShopkeeper,
    ItemNotFound,
    InsufficientFunds,
    InsufficientStock,
    InventoryFull,
    InvalidItem,
    ShopClosed
};

/**
 * @brief Global shop management
 */
class ShopManager {
public:
    static ShopManager& instance();

    // Shop registration
    void register_shopkeeper(EntityId shopkeeper_id, std::unique_ptr<Shopkeeper> shop);
    void unregister_shopkeeper(EntityId shopkeeper_id);

    // Shop access
    Shopkeeper* get_shopkeeper(EntityId shopkeeper_id);
    const Shopkeeper* get_shopkeeper(EntityId shopkeeper_id) const;

    // Shop operations
    ShopResult buy_item(std::shared_ptr<Actor> buyer, EntityId shopkeeper_id, EntityId item_id);
    ShopResult buy_mob(std::shared_ptr<Actor> buyer, EntityId shopkeeper_id, EntityId mob_id);
    ShopResult sell_item(std::shared_ptr<Actor> seller, EntityId shopkeeper_id, std::shared_ptr<Object> item);

    // Shop discovery
    std::vector<EntityId> find_shops_in_room(EntityId room_id) const;

    // Iterate all registered shops
    template<typename Func>
    void for_each_shop(Func&& func) const {
        for (const auto& [keeper_id, shop] : shops_) {
            if (shop) {
                func(keeper_id, *shop);
            }
        }
    }

    // Get count of registered shops
    size_t shop_count() const { return shops_.size(); }

private:
    ShopManager() = default;
    std::unordered_map<EntityId, std::unique_ptr<Shopkeeper>> shops_;
};