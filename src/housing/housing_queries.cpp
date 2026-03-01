#include "housing/housing_queries.hpp"

#include <fmt/format.h>

namespace HousingQueries {

// ============================================================================
// Load operations
// ============================================================================

Result<std::optional<HouseData>> load_house(pqxx::work &txn, const std::string &character_id) {
    try {
        auto result = txn.exec_params(R"(
            SELECT id, character_id, return_room_zone_id, return_room_id,
                   entrance_room_zone_id, entrance_room_id
            FROM player_houses
            WHERE character_id = $1
        )",
                                      character_id);

        if (result.empty()) {
            return std::optional<HouseData>(std::nullopt);
        }

        const auto &row = result[0];
        HouseData data;
        data.id = row["id"].as<int>();
        data.character_id = row["character_id"].as<std::string>();
        if (!row["return_room_zone_id"].is_null()) {
            data.return_room_zone_id = row["return_room_zone_id"].as<int>();
        }
        if (!row["return_room_id"].is_null()) {
            data.return_room_id = row["return_room_id"].as<int>();
        }
        data.entrance_room_zone_id = row["entrance_room_zone_id"].as<int>();
        data.entrance_room_id = row["entrance_room_id"].as<int>();

        return std::optional<HouseData>(std::move(data));
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("load_house: {}", e.what())));
    }
}

Result<std::vector<HouseRoomData>> load_house_rooms(pqxx::work &txn, int house_id) {
    try {
        auto result = txn.exec_params(R"(
            SELECT id, house_id, local_index, name, description,
                   sector, is_peaceful, base_light_level, capacity
            FROM player_house_rooms
            WHERE house_id = $1
            ORDER BY local_index
        )",
                                      house_id);

        std::vector<HouseRoomData> rooms;
        rooms.reserve(result.size());

        for (const auto &row : result) {
            HouseRoomData data;
            data.id = row["id"].as<int>();
            data.house_id = row["house_id"].as<int>();
            data.local_index = row["local_index"].as<int>();
            data.name = row["name"].as<std::string>();
            data.description = row["description"].as<std::string>();
            data.sector = row["sector"].as<std::string>();
            data.is_peaceful = row["is_peaceful"].as<bool>();
            data.base_light_level = row["base_light_level"].as<int>();
            data.capacity = row["capacity"].as<int>();
            rooms.push_back(std::move(data));
        }

        return rooms;
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("load_house_rooms: {}", e.what())));
    }
}

Result<std::vector<HouseExitData>> load_house_exits(pqxx::work &txn, int house_id) {
    try {
        auto result = txn.exec_params(R"(
            SELECT e.from_room_id, e.to_room_id, e.direction
            FROM player_house_exits e
            JOIN player_house_rooms r ON e.from_room_id = r.id
            WHERE r.house_id = $1
        )",
                                      house_id);

        std::vector<HouseExitData> exits;
        exits.reserve(result.size());

        for (const auto &row : result) {
            HouseExitData data;
            data.from_room_id = row["from_room_id"].as<int>();
            data.to_room_id = row["to_room_id"].as<int>();
            data.direction = row["direction"].as<std::string>();
            exits.push_back(std::move(data));
        }

        return exits;
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("load_house_exits: {}", e.what())));
    }
}

Result<std::vector<HouseItemData>> load_room_items(pqxx::work &txn, int room_id) {
    try {
        auto result = txn.exec_params(R"(
            SELECT id, room_id, object_zone_id, object_id, condition, custom_values::text
            FROM player_house_items
            WHERE room_id = $1
            ORDER BY placed_at
        )",
                                      room_id);

        std::vector<HouseItemData> items;
        items.reserve(result.size());

        for (const auto &row : result) {
            HouseItemData data;
            data.id = row["id"].as<int>();
            data.room_id = row["room_id"].as<int>();
            data.object_zone_id = row["object_zone_id"].as<int>();
            data.object_id = row["object_id"].as<int>();
            data.condition = row["condition"].as<int>();
            data.custom_values = row[5].as<std::string>();
            items.push_back(std::move(data));
        }

        return items;
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("load_room_items: {}", e.what())));
    }
}

Result<std::vector<HouseGuestData>> load_house_guests(pqxx::work &txn, int house_id) {
    try {
        auto result = txn.exec_params(R"(
            SELECT g.character_id, c.name AS character_name, g.can_place
            FROM player_house_guests g
            JOIN "Characters" c ON g.character_id = c.id
            WHERE g.house_id = $1
            ORDER BY c.name
        )",
                                      house_id);

        std::vector<HouseGuestData> guests;
        guests.reserve(result.size());

        for (const auto &row : result) {
            HouseGuestData data;
            data.character_id = row["character_id"].as<std::string>();
            data.character_name = row["character_name"].as<std::string>();
            data.can_place = row["can_place"].as<bool>();
            guests.push_back(std::move(data));
        }

        return guests;
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("load_house_guests: {}", e.what())));
    }
}

Result<bool> has_house(pqxx::work &txn, const std::string &character_id) {
    try {
        auto result = txn.exec_params(R"(
            SELECT EXISTS(SELECT 1 FROM player_houses WHERE character_id = $1)
        )",
                                      character_id);
        return result[0][0].as<bool>();
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("has_house: {}", e.what())));
    }
}

Result<bool> is_guest_of(pqxx::work &txn, int house_id, const std::string &character_id) {
    try {
        auto result = txn.exec_params(R"(
            SELECT EXISTS(SELECT 1 FROM player_house_guests WHERE house_id = $1 AND character_id = $2)
        )",
                                      house_id, character_id);
        return result[0][0].as<bool>();
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("is_guest_of: {}", e.what())));
    }
}

// ============================================================================
// Mutation operations
// ============================================================================

Result<int> create_house(pqxx::work &txn, const std::string &character_id, int entrance_zone_id, int entrance_room_id) {
    try {
        // Create the house
        auto house_result = txn.exec_params(R"(
            INSERT INTO player_houses (character_id, entrance_room_zone_id, entrance_room_id, updated_at)
            VALUES ($1, $2, $3, NOW())
            RETURNING id
        )",
                                            character_id, entrance_zone_id, entrance_room_id);

        int house_id = house_result[0]["id"].as<int>();

        // Create the foyer (room index 0)
        txn.exec_params(R"(
            INSERT INTO player_house_rooms (house_id, local_index, name, description, updated_at)
            VALUES ($1, 0, 'Your Foyer', 'A cozy entrance room. A sturdy chest sits against the wall, ready to hold your belongings.', NOW())
        )",
                        house_id);

        return house_id;
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("create_house: {}", e.what())));
    }
}

Result<int> add_room(pqxx::work &txn, int house_id, int local_index, const std::string &name,
                     const std::string &description) {
    try {
        auto result = txn.exec_params(R"(
            INSERT INTO player_house_rooms (house_id, local_index, name, description, updated_at)
            VALUES ($1, $2, $3, $4, NOW())
            RETURNING id
        )",
                                      house_id, local_index, name, description);
        return result[0]["id"].as<int>();
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("add_room: {}", e.what())));
    }
}

Result<void> add_exit(pqxx::work &txn, int from_room_id, int to_room_id, const std::string &direction) {
    try {
        txn.exec_params(R"(
            INSERT INTO player_house_exits (from_room_id, to_room_id, direction)
            VALUES ($1, $2, $3)
        )",
                        from_room_id, to_room_id, direction);
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("add_exit: {}", e.what())));
    }
}

Result<void> update_room_description(pqxx::work &txn, int room_id, const std::string &name,
                                     const std::string &description) {
    try {
        txn.exec_params(R"(
            UPDATE player_house_rooms SET name = $2, description = $3, updated_at = NOW() WHERE id = $1
        )",
                        room_id, name, description);
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("update_room_description: {}", e.what())));
    }
}

Result<int> place_item(pqxx::work &txn, int room_id, int object_zone_id, int object_id, int condition) {
    try {
        auto result = txn.exec_params(R"(
            INSERT INTO player_house_items (room_id, object_zone_id, object_id, condition)
            VALUES ($1, $2, $3, $4)
            RETURNING id
        )",
                                      room_id, object_zone_id, object_id, condition);
        return result[0]["id"].as<int>();
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("place_item: {}", e.what())));
    }
}

Result<void> remove_item(pqxx::work &txn, int item_id) {
    try {
        auto result = txn.exec_params(R"(
            DELETE FROM player_house_items WHERE id = $1
        )",
                                      item_id);
        if (result.affected_rows() == 0) {
            return std::unexpected(Errors::NotFound("house item"));
        }
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("remove_item: {}", e.what())));
    }
}

Result<int> count_room_items(pqxx::work &txn, int room_id) {
    try {
        auto result = txn.exec_params(R"(
            SELECT COUNT(*) FROM player_house_items WHERE room_id = $1
        )",
                                      room_id);
        return result[0][0].as<int>();
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("count_room_items: {}", e.what())));
    }
}

Result<void> save_return_location(pqxx::work &txn, int house_id, int zone_id, int room_id) {
    try {
        txn.exec_params(R"(
            UPDATE player_houses
            SET return_room_zone_id = $2, return_room_id = $3, updated_at = NOW()
            WHERE id = $1
        )",
                        house_id, zone_id, room_id);
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("save_return_location: {}", e.what())));
    }
}

Result<void> clear_return_location(pqxx::work &txn, int house_id) {
    try {
        txn.exec_params(R"(
            UPDATE player_houses
            SET return_room_zone_id = NULL, return_room_id = NULL, updated_at = NOW()
            WHERE id = $1
        )",
                        house_id);
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("clear_return_location: {}", e.what())));
    }
}

Result<void> add_guest(pqxx::work &txn, int house_id, const std::string &character_id, bool can_place) {
    try {
        txn.exec_params(R"(
            INSERT INTO player_house_guests (house_id, character_id, can_place)
            VALUES ($1, $2, $3)
            ON CONFLICT (house_id, character_id) DO UPDATE SET can_place = $3
        )",
                        house_id, character_id, can_place);
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("add_guest: {}", e.what())));
    }
}

Result<void> remove_guest(pqxx::work &txn, int house_id, const std::string &character_id) {
    try {
        auto result = txn.exec_params(R"(
            DELETE FROM player_house_guests WHERE house_id = $1 AND character_id = $2
        )",
                                      house_id, character_id);
        if (result.affected_rows() == 0) {
            return std::unexpected(Errors::NotFound("guest"));
        }
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("remove_guest: {}", e.what())));
    }
}

Result<void> delete_house(pqxx::work &txn, int house_id) {
    try {
        txn.exec_params(R"(DELETE FROM player_houses WHERE id = $1)", house_id);
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("delete_house: {}", e.what())));
    }
}

Result<std::vector<HouseData>> load_stranded_houses(pqxx::work &txn) {
    try {
        auto result = txn.exec(R"(
            SELECT id, character_id, return_room_zone_id, return_room_id,
                   entrance_room_zone_id, entrance_room_id
            FROM player_houses
            WHERE return_room_zone_id IS NOT NULL
        )");

        std::vector<HouseData> houses;
        houses.reserve(result.size());

        for (const auto &row : result) {
            HouseData data;
            data.id = row["id"].as<int>();
            data.character_id = row["character_id"].as<std::string>();
            data.return_room_zone_id = row["return_room_zone_id"].as<int>();
            data.return_room_id = row["return_room_id"].as<int>();
            data.entrance_room_zone_id = row["entrance_room_zone_id"].as<int>();
            data.entrance_room_id = row["entrance_room_id"].as<int>();
            houses.push_back(std::move(data));
        }

        return houses;
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("load_stranded_houses: {}", e.what())));
    }
}

Result<void> update_character_location(pqxx::work &txn, const std::string &character_id, int zone_id, int room_id) {
    try {
        txn.exec_params(R"(
            UPDATE "Characters"
            SET current_room_zone_id = $2, current_room_id = $3
            WHERE id = $1
        )",
                        character_id, zone_id, room_id);
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("update_character_location: {}", e.what())));
    }
}

Result<int> count_house_rooms(pqxx::work &txn, int house_id) {
    try {
        auto result = txn.exec_params(R"(
            SELECT COUNT(*) FROM player_house_rooms WHERE house_id = $1
        )",
                                      house_id);
        return result[0][0].as<int>();
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("count_house_rooms: {}", e.what())));
    }
}

} // namespace HousingQueries
