/**
 * @file config_loader.cpp
 * @brief Implementation of database-driven configuration loader
 */

#include "config_loader.hpp"

#include <algorithm>
#include <set>

#include <fmt/format.h>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

#include "connection_pool.hpp"

namespace fierymud::config {

ConfigLoader &ConfigLoader::instance() {
    static ConfigLoader instance;
    return instance;
}

std::string ConfigLoader::make_key(std::string_view category, std::string_view key) {
    return fmt::format("{}.{}", category, key);
}

const ConfigEntry *ConfigLoader::get_entry(std::string_view category, std::string_view key) const {
    auto lookup_key = make_key(category, key);
    auto it = entries_.find(lookup_key);
    if (it != entries_.end()) {
        return &it->second;
    }
    return nullptr;
}

Result<void> ConfigLoader::load_from_database() {
    spdlog::info("Loading game configuration from database...");

    auto result = ConnectionPool::instance().execute([this](pqxx::work &txn) -> Result<void> {
        try {
            // Query all configuration entries
            auto rows = txn.exec(R"(
                SELECT
                    category,
                    key,
                    value,
                    value_type,
                    description,
                    min_value,
                    max_value,
                    is_secret,
                    restart_req
                FROM "GameConfig"
                ORDER BY category, key
            )");

            entries_.clear();

            for (const auto &row : rows) {
                ConfigEntry entry;
                entry.category = row["category"].as<std::string>();
                entry.key = row["key"].as<std::string>();
                entry.value = row["value"].as<std::string>();

                // Parse value type enum
                std::string type_str = row["value_type"].as<std::string>();
                if (type_str == "STRING") {
                    entry.value_type = ConfigValueType::STRING;
                } else if (type_str == "INT") {
                    entry.value_type = ConfigValueType::INT;
                } else if (type_str == "FLOAT") {
                    entry.value_type = ConfigValueType::FLOAT;
                } else if (type_str == "BOOL") {
                    entry.value_type = ConfigValueType::BOOL;
                } else if (type_str == "JSON") {
                    entry.value_type = ConfigValueType::JSON;
                } else {
                    entry.value_type = ConfigValueType::STRING;
                }

                // Optional fields
                if (!row["description"].is_null()) {
                    entry.description = row["description"].as<std::string>();
                }
                if (!row["min_value"].is_null()) {
                    entry.min_value = row["min_value"].as<std::string>();
                }
                if (!row["max_value"].is_null()) {
                    entry.max_value = row["max_value"].as<std::string>();
                }

                entry.is_secret = row["is_secret"].as<bool>();
                entry.restart_required = row["restart_req"].as<bool>();

                // Store in map
                auto lookup_key = make_key(entry.category, entry.key);
                entries_[lookup_key] = std::move(entry);
            }

            loaded_ = true;
            spdlog::info("Loaded {} configuration entries from database", entries_.size());
            return {};

        } catch (const pqxx::sql_error &e) {
            return std::unexpected(Errors::DatabaseError(fmt::format("Failed to load config: {}", e.what())));
        }
    });

    return result;
}

Result<void> ConfigLoader::reload_category(std::string_view category) {
    spdlog::info("Reloading configuration category: {}", category);

    auto result =
        ConnectionPool::instance().execute([this, cat = std::string(category)](pqxx::work &txn) -> Result<void> {
            try {
                // Remove existing entries for this category
                std::erase_if(entries_, [&cat](const auto &pair) { return pair.second.category == cat; });

                // Query entries for this category
                auto rows = txn.exec_params(R"(
                SELECT
                    category,
                    key,
                    value,
                    value_type,
                    description,
                    min_value,
                    max_value,
                    is_secret,
                    restart_req
                FROM "GameConfig"
                WHERE category = $1
                ORDER BY key
            )",
                                            cat);

                for (const auto &row : rows) {
                    ConfigEntry entry;
                    entry.category = row["category"].as<std::string>();
                    entry.key = row["key"].as<std::string>();
                    entry.value = row["value"].as<std::string>();

                    std::string type_str = row["value_type"].as<std::string>();
                    if (type_str == "STRING") {
                        entry.value_type = ConfigValueType::STRING;
                    } else if (type_str == "INT") {
                        entry.value_type = ConfigValueType::INT;
                    } else if (type_str == "FLOAT") {
                        entry.value_type = ConfigValueType::FLOAT;
                    } else if (type_str == "BOOL") {
                        entry.value_type = ConfigValueType::BOOL;
                    } else if (type_str == "JSON") {
                        entry.value_type = ConfigValueType::JSON;
                    } else {
                        entry.value_type = ConfigValueType::STRING;
                    }

                    if (!row["description"].is_null()) {
                        entry.description = row["description"].as<std::string>();
                    }
                    if (!row["min_value"].is_null()) {
                        entry.min_value = row["min_value"].as<std::string>();
                    }
                    if (!row["max_value"].is_null()) {
                        entry.max_value = row["max_value"].as<std::string>();
                    }

                    entry.is_secret = row["is_secret"].as<bool>();
                    entry.restart_required = row["restart_req"].as<bool>();

                    auto lookup_key = make_key(entry.category, entry.key);
                    entries_[lookup_key] = std::move(entry);
                }

                spdlog::info("Reloaded {} entries for category '{}'", rows.size(), cat);
                return {};

            } catch (const pqxx::sql_error &e) {
                return std::unexpected(
                    Errors::DatabaseError(fmt::format("Failed to reload category '{}': {}", cat, e.what())));
            }
        });

    return result;
}

// ========================================
// Typed Getters
// ========================================

std::optional<std::string> ConfigLoader::get_string(std::string_view category, std::string_view key) const {
    if (const auto *entry = get_entry(category, key)) {
        return entry->value;
    }
    return std::nullopt;
}

std::optional<int> ConfigLoader::get_int(std::string_view category, std::string_view key) const {
    if (const auto *entry = get_entry(category, key)) {
        try {
            return std::stoi(entry->value);
        } catch (...) {
            spdlog::warn("Failed to parse int config {}.{}: '{}'", category, key, entry->value);
        }
    }
    return std::nullopt;
}

std::optional<double> ConfigLoader::get_float(std::string_view category, std::string_view key) const {
    if (const auto *entry = get_entry(category, key)) {
        try {
            return std::stod(entry->value);
        } catch (...) {
            spdlog::warn("Failed to parse float config {}.{}: '{}'", category, key, entry->value);
        }
    }
    return std::nullopt;
}

std::optional<bool> ConfigLoader::get_bool(std::string_view category, std::string_view key) const {
    if (const auto *entry = get_entry(category, key)) {
        const auto &val = entry->value;
        if (val == "true" || val == "1" || val == "yes" || val == "on") {
            return true;
        }
        if (val == "false" || val == "0" || val == "no" || val == "off") {
            return false;
        }
        spdlog::warn("Failed to parse bool config {}.{}: '{}'", category, key, val);
    }
    return std::nullopt;
}

std::optional<std::chrono::seconds> ConfigLoader::get_seconds(std::string_view category, std::string_view key) const {
    if (auto val = get_int(category, key)) {
        return std::chrono::seconds(*val);
    }
    return std::nullopt;
}

std::optional<std::chrono::minutes> ConfigLoader::get_minutes(std::string_view category, std::string_view key) const {
    if (auto val = get_int(category, key)) {
        return std::chrono::minutes(*val);
    }
    return std::nullopt;
}

// ========================================
// Typed Getters with Defaults
// ========================================

std::string ConfigLoader::get_string_or(std::string_view category, std::string_view key,
                                        std::string_view default_value) const {
    return get_string(category, key).value_or(std::string(default_value));
}

int ConfigLoader::get_int_or(std::string_view category, std::string_view key, int default_value) const {
    return get_int(category, key).value_or(default_value);
}

double ConfigLoader::get_float_or(std::string_view category, std::string_view key, double default_value) const {
    return get_float(category, key).value_or(default_value);
}

bool ConfigLoader::get_bool_or(std::string_view category, std::string_view key, bool default_value) const {
    return get_bool(category, key).value_or(default_value);
}

// ========================================
// Category Access
// ========================================

std::vector<ConfigEntry> ConfigLoader::get_category(std::string_view category) const {
    std::vector<ConfigEntry> result;
    for (const auto &[key, entry] : entries_) {
        if (entry.category == category) {
            result.push_back(entry);
        }
    }
    // Sort by key for consistent ordering
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) { return a.key < b.key; });
    return result;
}

std::vector<std::string> ConfigLoader::get_categories() const {
    std::set<std::string> categories;
    for (const auto &[key, entry] : entries_) {
        categories.insert(entry.category);
    }
    return {categories.begin(), categories.end()};
}

bool ConfigLoader::has(std::string_view category, std::string_view key) const {
    return get_entry(category, key) != nullptr;
}

} // namespace fierymud::config
