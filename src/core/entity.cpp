#include "entity.hpp"
#include "../core/logging.hpp"
#include "../text/string_utils.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace {
    // Entity validation constants
    constexpr size_t MAX_KEYWORD_LENGTH = 50;
}

// Entity Implementation

Entity::Entity(EntityId id, std::string_view name) 
    : id_(id), name_(name) {
    if (!name.empty()) {
        ensure_name_in_keywords();
    }
}

Entity::Entity(EntityId id, std::string_view name,
               std::span<const std::string> keywords,
               std::string_view ground,
               std::string_view short_desc)
    : id_(id), name_(name), ground_(ground), short_(short_desc) {

    keywords_.reserve(keywords.size() + 1);
    for (const auto& keyword : keywords) {
        if (EntityUtils::is_valid_keyword(keyword)) {
            std::string normalized = EntityUtils::normalize_keyword(keyword);
            if (keyword_set_.find(normalized) == keyword_set_.end()) {
                keywords_.push_back(normalized);
                keyword_set_.insert(normalized);
            }
        }
    }

    ensure_name_in_keywords();
}

bool Entity::matches_keyword(std::string_view keyword) const {
    std::string normalized = EntityUtils::normalize_keyword(keyword);
    // O(1) lookup using unordered_set instead of O(n) linear search
    return keyword_set_.find(normalized) != keyword_set_.end();
}

bool Entity::matches_any_keyword(std::span<const std::string> keywords) const {
    return std::any_of(keywords.begin(), keywords.end(),
        [this](const std::string& kw) {
            return matches_keyword(kw);
        });
}

void Entity::set_keywords(std::span<const std::string> new_keywords) {
    keywords_.clear();
    keyword_set_.clear();
    keywords_.reserve(new_keywords.size() + 1);

    for (const auto& keyword : new_keywords) {
        if (EntityUtils::is_valid_keyword(keyword)) {
            std::string normalized = EntityUtils::normalize_keyword(keyword);

            // O(1) duplicate check using set
            if (keyword_set_.find(normalized) == keyword_set_.end()) {
                keywords_.push_back(normalized);
                keyword_set_.insert(std::move(normalized));
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

    // O(1) duplicate check using set
    if (keyword_set_.find(normalized) == keyword_set_.end()) {
        keywords_.push_back(normalized);
        keyword_set_.insert(std::move(normalized));
    }
}

void Entity::remove_keyword(std::string_view keyword) {
    std::string normalized = EntityUtils::normalize_keyword(keyword);

    // Don't allow removing the primary name
    std::string normalized_name = EntityUtils::normalize_keyword(name_);
    if (normalized == normalized_name) {
        return;
    }

    // Remove from both vector and set
    keyword_set_.erase(normalized);
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
    json["ground"] = ground_;
    
    if (!short_.empty()) {
        json["short"] = short_;
    }
    
    return json;
}

Result<std::unique_ptr<Entity>> Entity::from_json(const nlohmann::json& json) {
    try {
        // Parse required fields
        if (!json.contains("id")) {
            return std::unexpected(Errors::ParseError("Entity JSON missing 'id' field"));
        }
        
        // Handle name field mapping: name -> short -> keywords -> name_list (legacy)
        std::string name;
        if (json.contains("name")) {
            name = json["name"].get<std::string>();
        } else if (json.contains("short")) {
            // Use short description as name for display purposes
            name = json["short"].get<std::string>();
        } else if (json.contains("keywords")) {
            if (json["keywords"].is_string()) {
                name = json["keywords"].get<std::string>();
            } else if (json["keywords"].is_array() && !json["keywords"].empty()) {
                // Use first keyword as name fallback
                for (const auto& keyword : json["keywords"]) {
                    if (keyword.is_string()) {
                        name = keyword.get<std::string>();
                        break; // Use only the first keyword, not all joined
                    }
                }
            }
        } else if (json.contains("name_list")) {
            name = json["name_list"].get<std::string>();
        } else {
            return std::unexpected(Errors::ParseError("Entity JSON missing 'name', 'short', 'keywords', or 'name_list' field"));
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
        
        // Parse optional fields with field mapping (new and legacy field names)
        if (json.contains("ground")) {
            entity->set_ground(json["ground"].get<std::string>());
        } else if (json.contains("description")) {
            entity->set_ground(json["description"].get<std::string>());
        } else if (json.contains("long_description")) {
            entity->set_ground(json["long_description"].get<std::string>());
        }
        
        if (json.contains("short")) {
            entity->set_short_desc(json["short"].get<std::string>());
        } else if (json.contains("short_desc")) {
            entity->set_short_desc(json["short_desc"].get<std::string>());
        } else if (json.contains("short_description")) {
            entity->set_short_desc(json["short_description"].get<std::string>());
        }
        
        // Parse keywords from array or space-separated string
        // Priority: keywords (array) -> keywords (string) -> name_list (legacy)
        std::vector<std::string> keywords;
        if (json.contains("keywords") && json["keywords"].is_array()) {
            // Modern array format
            for (const auto& keyword : json["keywords"]) {
                if (keyword.is_string()) {
                    keywords.push_back(keyword.get<std::string>());
                }
            }
        } else if (json.contains("keywords") && json["keywords"].is_string()) {
            // Modern string format (space-separated)
            std::string keyword_string = json["keywords"].get<std::string>();
            std::istringstream iss(keyword_string);
            std::string keyword;
            while (iss >> keyword) {
                keywords.push_back(keyword);
            }
        } else if (json.contains("name_list") && json["name_list"].is_string()) {
            // Legacy format (still supported for compatibility)
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
    return EntityUtils::format_entity_name(name_, short_, with_article);
}

void Entity::ensure_name_in_keywords() {
    if (name_.empty()) {
        return;
    }

    std::string normalized_name = EntityUtils::normalize_keyword(name_);

    // O(1) check if name is already in keywords using set
    if (keyword_set_.find(normalized_name) == keyword_set_.end()) {
        keywords_.insert(keywords_.begin(), normalized_name);
        keyword_set_.insert(std::move(normalized_name));
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
        
        EntityId id{json["id"].get<std::uint64_t>()};
        
        // Handle name field mapping: name -> short -> keywords fallback
        std::string name;
        if (json.contains("name")) {
            name = json["name"].get<std::string>();
        } else if (json.contains("short")) {
            name = json["short"].get<std::string>();
        } else if (json.contains("keywords") && json["keywords"].is_array() && !json["keywords"].empty()) {
            // Use first keyword as fallback
            for (const auto& keyword : json["keywords"]) {
                if (keyword.is_string()) {
                    name = keyword.get<std::string>();
                    break;
                }
            }
        } else {
            return std::unexpected(Errors::ParseError("Entity JSON missing 'name', 'short', or 'keywords' field"));
        }
        
        auto entity = std::unique_ptr<Entity>(new Entity(id, name));
        
        // Parse optional fields (new and legacy field names)
        if (json.contains("ground")) {
            entity->set_ground(json["ground"].get<std::string>());
        } else if (json.contains("description")) {
            entity->set_ground(json["description"].get<std::string>());
        }
        
        if (json.contains("short")) {
            entity->set_short_desc(json["short"].get<std::string>());
        } else if (json.contains("short_desc")) {
            entity->set_short_desc(json["short_desc"].get<std::string>());
        } else if (json.contains("short_description")) {
            entity->set_short_desc(json["short_description"].get<std::string>());
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
    // Common articles that should not be keywords
    static const std::unordered_set<std::string> ARTICLES = {"a", "an", "the", "some"};

    bool is_valid_keyword(std::string_view keyword) {
        if (keyword.empty() || keyword.length() > MAX_KEYWORD_LENGTH) {
            return false;
        }

        // Check for printable ASCII characters only
        if (!std::all_of(keyword.begin(), keyword.end(), [](char c) {
            return std::isprint(c) && c != '"' && c != '\'';
        })) {
            return false;
        }

        // Reject common articles
        std::string lower_kw;
        lower_kw.reserve(keyword.size());
        for (char c : keyword) {
            lower_kw.push_back(std::tolower(c));
        }
        if (ARTICLES.contains(lower_kw)) {
            return false;
        }

        return true;
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
            // Wrap each keyword in single quotes for clear separation
            result += "'";
            result += keywords[i];
            result += "'";
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
        // Check if display_name already starts with an article
        std::string lower_display = to_lowercase(display_name);

        // Common articles and demonstratives
        if (lower_display.starts_with("a ") || 
            lower_display.starts_with("an ") || 
            lower_display.starts_with("the ") ||
            lower_display.starts_with("some ") ||
            lower_display.starts_with("this ") ||
            lower_display.starts_with("that ")) {
            return display_name; // Already has article, don't add another
        }
        
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