/***************************************************************************
 *   File: s../core/entity.cpp                          Part of FieryMUD *
 *  Usage: Base Entity class implementation                                 *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "entity.hpp"
#include "../core/logging.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

// Entity Implementation

Entity::Entity(EntityId id, std::string_view name) 
    : id_(id), name_(name) {
    if (!name.empty()) {
        ensure_name_in_keywords();
    }
}

Entity::Entity(EntityId id, std::string_view name, 
               std::span<const std::string> keywords,
               std::string_view description,
               std::string_view short_description)
    : id_(id), name_(name), description_(description), short_description_(short_description) {
    
    keywords_.reserve(keywords.size() + 1);
    for (const auto& keyword : keywords) {
        if (EntityUtils::is_valid_keyword(keyword)) {
            keywords_.push_back(EntityUtils::normalize_keyword(keyword));
        }
    }
    
    ensure_name_in_keywords();
}

bool Entity::matches_keyword(std::string_view keyword) const {
    std::string normalized = EntityUtils::normalize_keyword(keyword);
    
    return std::any_of(keywords_.begin(), keywords_.end(),
        [&normalized](const std::string& kw) {
            return kw == normalized;
        });
}

bool Entity::matches_any_keyword(std::span<const std::string> keywords) const {
    return std::any_of(keywords.begin(), keywords.end(),
        [this](const std::string& kw) {
            return matches_keyword(kw);
        });
}

void Entity::set_keywords(std::span<const std::string> new_keywords) {
    keywords_.clear();
    keywords_.reserve(new_keywords.size() + 1);
    
    for (const auto& keyword : new_keywords) {
        if (EntityUtils::is_valid_keyword(keyword)) {
            std::string normalized = EntityUtils::normalize_keyword(keyword);
            
            // Avoid duplicates
            if (std::find(keywords_.begin(), keywords_.end(), normalized) == keywords_.end()) {
                keywords_.push_back(std::move(normalized));
            }
        }
    }
    
    ensure_name_in_keywords();
}

void Entity::add_keyword(std::string_view keyword) {
    if (!EntityUtils::is_valid_keyword(keyword)) {
        return;
    }
    
    std::string normalized = EntityUtils::normalize_keyword(keyword);
    
    // Check for duplicates
    if (std::find(keywords_.begin(), keywords_.end(), normalized) == keywords_.end()) {
        keywords_.push_back(std::move(normalized));
    }
}

void Entity::remove_keyword(std::string_view keyword) {
    std::string normalized = EntityUtils::normalize_keyword(keyword);
    
    // Don't allow removing the primary name
    std::string normalized_name = EntityUtils::normalize_keyword(name_);
    if (normalized == normalized_name) {
        return;
    }
    
    keywords_.erase(
        std::remove(keywords_.begin(), keywords_.end(), normalized),
        keywords_.end());
}

nlohmann::json Entity::to_json() const {
    nlohmann::json json;
    
    json["type"] = type_name();
    json["id"] = id_.value();
    json["name"] = name_;
    json["keywords"] = keywords_;
    json["description"] = description_;
    
    if (!short_description_.empty()) {
        json["short_description"] = short_description_;
    }
    
    return json;
}

Result<std::unique_ptr<Entity>> Entity::from_json(const nlohmann::json& json) {
    try {
        // Parse required fields
        if (!json.contains("id")) {
            return std::unexpected(Errors::ParseError("Entity JSON missing 'id' field"));
        }
        
        // Handle name field mapping: name_list -> name
        std::string name;
        if (json.contains("name")) {
            name = json["name"].get<std::string>();
        } else if (json.contains("name_list")) {
            name = json["name_list"].get<std::string>();
        } else {
            return std::unexpected(Errors::ParseError("Entity JSON missing 'name' or 'name_list' field"));
        }
        
        // Handle ID conversion
        EntityId id;
        if (json["id"].is_string()) {
            auto id_value = std::stoull(json["id"].get<std::string>());
            if (id_value == 0) id_value = 1000; // Convert zone 0 to ID 1000
            id = EntityId{id_value};
        } else {
            id = EntityId{json["id"].get<std::uint64_t>()};
        }
        
        auto entity = std::unique_ptr<Entity>(new Entity(id, name));
        
        // Parse optional fields with field mapping
        if (json.contains("description")) {
            entity->set_description(json["description"].get<std::string>());
        } else if (json.contains("long_description")) {
            entity->set_description(json["long_description"].get<std::string>());
        }
        
        if (json.contains("short_description")) {
            entity->set_short_description(json["short_description"].get<std::string>());
        }
        
        // Parse keywords from array or space-separated name_list
        std::vector<std::string> keywords;
        if (json.contains("keywords") && json["keywords"].is_array()) {
            for (const auto& keyword : json["keywords"]) {
                if (keyword.is_string()) {
                    keywords.push_back(keyword.get<std::string>());
                }
            }
        } else if (json.contains("name_list") && json["name_list"].is_string()) {
            // Parse space-separated keywords from name_list
            std::string name_list = json["name_list"].get<std::string>();
            std::istringstream iss(name_list);
            std::string keyword;
            while (iss >> keyword) {
                keywords.push_back(keyword);
            }
        }
        
        if (!keywords.empty()) {
            entity->set_keywords(keywords);
        }
        
        TRY(entity->validate());
        
        return entity;
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Entity JSON parsing", e.what()));
    }
}

std::string Entity::display_name(bool with_article) const {
    return EntityUtils::format_entity_name(name_, short_description_, with_article);
}

void Entity::ensure_name_in_keywords() {
    if (name_.empty()) {
        return;
    }
    
    std::string normalized_name = EntityUtils::normalize_keyword(name_);
    
    // Check if name is already in keywords
    if (std::find(keywords_.begin(), keywords_.end(), normalized_name) == keywords_.end()) {
        keywords_.insert(keywords_.begin(), normalized_name);
    }
}

Result<void> Entity::validate() const {
    if (!id_.is_valid()) {
        return std::unexpected(Errors::InvalidState("Entity has invalid ID"));
    }
    
    if (name_.empty()) {
        return std::unexpected(Errors::InvalidState("Entity name cannot be empty"));
    }
    
    if (keywords_.empty()) {
        return std::unexpected(Errors::InvalidState("Entity must have at least one keyword"));
    }
    
    return Success();
}

// EntityFactory Implementation

std::unordered_map<std::string, EntityFactory::CreateFunction> EntityFactory::creators_;

void EntityFactory::register_type(std::string_view type_name, CreateFunction creator) {
    creators_[std::string(type_name)] = std::move(creator);
}

Result<std::unique_ptr<Entity>> EntityFactory::create_from_json(const nlohmann::json& json) {
    try {
        if (!json.contains("type")) {
            return std::unexpected(Errors::ParseError("JSON missing 'type' field"));
        }
        
        std::string type = json["type"].get<std::string>();
        
        auto it = creators_.find(type);
        if (it == creators_.end()) {
            // Fallback to base Entity if specific type not registered
            Log::debug("Unknown entity type '{}', using base Entity", type);
            return create_base_entity_from_json(json);
        }
        
        return it->second(json);
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("JSON parsing error", e.what()));
    }
}

std::vector<std::string> EntityFactory::get_registered_types() {
    std::vector<std::string> types;
    types.reserve(creators_.size());
    
    for (const auto& [type_name, creator] : creators_) {
        types.push_back(type_name);
    }
    
    std::sort(types.begin(), types.end());
    return types;
}

Result<std::unique_ptr<Entity>> EntityFactory::create_base_entity_from_json(const nlohmann::json& json) {
    try {
        // Parse required fields
        if (!json.contains("id")) {
            return std::unexpected(Errors::ParseError("Entity JSON missing 'id' field"));
        }
        
        if (!json.contains("name")) {
            return std::unexpected(Errors::ParseError("Entity JSON missing 'name' field"));
        }
        
        EntityId id{json["id"].get<std::uint64_t>()};
        std::string name = json["name"].get<std::string>();
        
        auto entity = std::unique_ptr<Entity>(new Entity(id, name));
        
        // Parse optional fields
        if (json.contains("description")) {
            entity->set_description(json["description"].get<std::string>());
        }
        
        if (json.contains("short_description")) {
            entity->set_short_description(json["short_description"].get<std::string>());
        }
        
        if (json.contains("keywords") && json["keywords"].is_array()) {
            std::vector<std::string> keywords;
            for (const auto& keyword : json["keywords"]) {
                if (keyword.is_string()) {
                    keywords.push_back(keyword.get<std::string>());
                }
            }
            if (!keywords.empty()) {
                entity->set_keywords(keywords);
            }
        }
        
        TRY(entity->validate());
        
        return entity;
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Entity JSON parsing", e.what()));
    }
}

// EntityUtils Implementation

namespace EntityUtils {
    bool is_valid_keyword(std::string_view keyword) {
        if (keyword.empty() || keyword.length() > 50) {
            return false;
        }
        
        // Check for printable ASCII characters only
        return std::all_of(keyword.begin(), keyword.end(), [](char c) {
            return std::isprint(c) && c != '"' && c != '\'';
        });
    }
    
    std::string normalize_keyword(std::string_view keyword) {
        std::string normalized;
        normalized.reserve(keyword.length());
        
        // Convert to lowercase and trim spaces
        bool in_word = false;
        for (char c : keyword) {
            if (std::isspace(c)) {
                if (in_word) {
                    normalized.push_back(' ');
                    in_word = false;
                }
            } else {
                normalized.push_back(std::tolower(c));
                in_word = true;
            }
        }
        
        // Remove trailing space if any
        if (!normalized.empty() && normalized.back() == ' ') {
            normalized.pop_back();
        }
        
        return normalized;
    }
    
    std::vector<std::string> parse_keyword_list(std::string_view keyword_string) {
        std::vector<std::string> keywords;
        
        // Split on whitespace and filter valid keywords
        std::string current;
        for (char c : keyword_string) {
            if (std::isspace(c)) {
                if (!current.empty()) {
                    if (EntityUtils::is_valid_keyword(current)) {
                        keywords.push_back(EntityUtils::normalize_keyword(current));
                    }
                    current.clear();
                }
            } else {
                current.push_back(c);
            }
        }
        
        // Handle last keyword
        if (!current.empty() && EntityUtils::is_valid_keyword(current)) {
            keywords.push_back(EntityUtils::normalize_keyword(current));
        }
        
        // Remove duplicates
        std::sort(keywords.begin(), keywords.end());
        keywords.erase(std::unique(keywords.begin(), keywords.end()), keywords.end());
        
        return keywords;
    }
    
    std::string create_keyword_string(std::span<const std::string> keywords) {
        if (keywords.empty()) {
            return "";
        }
        
        std::string result;
        for (size_t i = 0; i < keywords.size(); ++i) {
            if (i > 0) {
                result += " ";
            }
            result += keywords[i];
        }
        
        return result;
    }
    
    std::string_view get_article(std::string_view name, bool definite) {
        if (definite) {
            return "the";
        }
        
        if (name.empty()) {
            return "a";
        }
        
        char first = std::tolower(name[0]);
        if (first == 'a' || first == 'e' || first == 'i' || first == 'o' || first == 'u') {
            return "an";
        }
        
        return "a";
    }
    
    std::string format_entity_name(std::string_view name, std::string_view short_desc, 
                                 bool with_article, bool definite_article) {
        std::string display_name = short_desc.empty() ? std::string(name) : std::string(short_desc);
        
        if (with_article) {
            std::string_view article = EntityUtils::get_article(display_name, definite_article);
            return fmt::format("{} {}", article, display_name);
        }
        
        return display_name;
    }
}

// JSON Support Functions

void to_json(nlohmann::json& json, const Entity& entity) {
    json = entity.to_json();
}

void from_json(const nlohmann::json& json, std::unique_ptr<Entity>& entity) {
    auto result = EntityFactory::create_from_json(json);
    if (result) {
        entity = std::move(result.value());
    } else {
        throw std::runtime_error(fmt::format("Failed to create entity from JSON: {}", result.error().message));
    }
}