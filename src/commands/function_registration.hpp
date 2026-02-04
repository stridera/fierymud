#include "arguments.hpp"
#include "logging.hpp"
#include "structs.hpp"
#include "utils.hpp"

#include <algorithm>
#include <format>
#include <iostream>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

// Silence spurious warnings in <functional> header
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <functional>
#pragma GCC diagnostic pop

// All of our functions should include these headers
// Enhanced command categorization for intelligent disambiguation
enum class CommandCategory : uint8_t {
    MOVEMENT = 0,            // north, south, east, west, up, down
    COMBAT = 1,              // kill, flee, bash, kick, backstab
    COMMUNICATION = 2,       // say, tell, gossip, clan
    OBJECT_MANIPULATION = 3, // get, drop, put, give, wear, remove
    INFORMATION = 4,         // look, examine, score, inventory, who
    SOCIAL = 5,              // smile, nod, nap, sleep, wake
    ADMINISTRATIVE = 6,      // shutdown, advance, set, restore
    SYSTEM = 7,              // save, quit, time, weather
    CLAN = 8,                // clan-specific commands
    MAGIC = 9,               // cast, memorize, pray
    SKILLS = 10,             // practice, train, use
    UNKNOWN = 255            // Default/unspecified category
};

// Context flags for situational command priority
using ContextFlags = uint32_t;
constexpr ContextFlags CONTEXT_COMBAT = 1 << 0;    // Higher priority in combat
constexpr ContextFlags CONTEXT_PEACEFUL = 1 << 1;  // Higher priority when peaceful
constexpr ContextFlags CONTEXT_INDOOR = 1 << 2;    // Indoor environments
constexpr ContextFlags CONTEXT_OUTDOOR = 1 << 3;   // Outdoor environments
constexpr ContextFlags CONTEXT_WATER = 1 << 4;     // Water rooms
constexpr ContextFlags CONTEXT_CLAN_ROOM = 1 << 5; // Clan-specific rooms
constexpr ContextFlags CONTEXT_ALWAYS = 0;         // No context restrictions

// User preference system for command disambiguation
struct UserCommandPreferences {
    std::unordered_map<std::string, std::string> abbreviation_overrides; // "n" -> "north"
    std::unordered_map<CommandCategory, int> category_priorities;        // Custom category weights
    bool prefer_combat_commands = false;                                 // Prefer combat in ambiguous cases
    bool prefer_movement_commands = true;                                // Prefer movement (default true)
};

using UniformFunction = void (*)(CharData *, Arguments);
using PermissionFlags = uint32_t;

// Lambda handler - wrapper for runtime functions with any capture
class LambdaHandler {
  public:
    virtual ~LambdaHandler() = default;
    virtual void invoke(CharData *data, Arguments args) = 0;
};

// Template implementation that wraps any lambda/functor
template <typename F> class LambdaHandlerImpl : public LambdaHandler {
  private:
    F func_;

  public:
    LambdaHandlerImpl(F &&func) : func_(std::forward<F>(func)) {}

    void invoke(CharData *data, Arguments args) override { func_(data, args); }
};

// Function registry with runtime registration support
class FunctionRegistry {
  private:
    // Enhanced function info structure with category-based disambiguation
    struct FunctionInfo {
        std::string name;
        UniformFunction func;
        size_t priority;
        PermissionFlags permissions;
        std::string description;
        CommandCategory category;
        ContextFlags context_flags;

        // Enhanced comparison for sophisticated disambiguation
        bool operator<(const FunctionInfo &other) const {
            // 1. Category-based priority (movement commands beat social commands)
            if (category != other.category) {
                return get_category_priority(category) > get_category_priority(other.category);
            }

            // 2. Explicit priority within category
            if (priority != other.priority) {
                return priority > other.priority; // Higher priority first
            }

            // 3. Context-specific priority (combat vs peaceful)
            auto context_priority = get_context_priority(context_flags);
            auto other_context_priority = get_context_priority(other.context_flags);
            if (context_priority != other_context_priority) {
                return context_priority > other_context_priority;
            }

            // 4. Alphabetical for final tie-breaking
            return name < other.name;
        }

        // Helper functions for priority calculation
        static constexpr size_t get_category_priority(CommandCategory cat) {
            using enum CommandCategory;
            switch (cat) {
            case MOVEMENT:
                return 100; // Highest: n = north
            case COMBAT:
                return 90; // High: k = kill
            case COMMUNICATION:
                return 80; // Medium-high: s = say
            case OBJECT_MANIPULATION:
                return 70; // Medium: g = get
            case INFORMATION:
                return 60; // Medium-low: l = look
            case SOCIAL:
                return 50; // Low: n = nap
            case ADMINISTRATIVE:
                return 40; // Lower
            case SYSTEM:
                return 30; // Lowest
            default:
                return 0;
            }
        }

        static constexpr size_t get_context_priority(ContextFlags flags) {
            size_t priority = 0;
            if (flags & CONTEXT_COMBAT)
                priority += 20;
            if (flags & CONTEXT_PEACEFUL)
                priority += 10;
            if (flags & CONTEXT_INDOOR)
                priority += 5;
            if (flags & CONTEXT_OUTDOOR)
                priority += 3;
            return priority;
        }
    };

    // Primary storage: map for O(1) direct lookups by name
    inline static std::unordered_map<std::string, UniformFunction> function_map_;

    // Secondary storage: sorted vector for prefix matching and priority ordering
    inline static std::vector<FunctionInfo> sorted_functions_;

    // Storage for lambda handlers (owns the lambdas)
    inline static std::vector<std::unique_ptr<LambdaHandler>> lambda_handlers_;

    // Abbreviation cache
    inline static std::unordered_map<std::string, std::string> abbreviation_cache_;
    inline static bool abbreviation_cache_valid_ = false;

    // Update the abbreviation cache based on the sorted functions list
    static void update_abbreviation_cache() {
        abbreviation_cache_.clear();

        // For each prefix length, starting with shortest
        for (size_t prefix_len = 1; prefix_len <= 10; prefix_len++) {
            // Group functions by their prefix of this length
            std::unordered_map<std::string, std::vector<FunctionInfo *>> prefix_groups;

            for (auto &func_info : sorted_functions_) {
                if (func_info.name.length() >= prefix_len) {
                    std::string prefix = func_info.name.substr(0, prefix_len);
                    prefix_groups[prefix].push_back(&func_info);
                }
            }

            // For each prefix group, if there's only one function, it's unique
            // Otherwise, select by priority and then alphabetically
            for (auto &[prefix, funcs] : prefix_groups) {
                if (funcs.size() == 1) {
                    // Unique prefix
                    abbreviation_cache_[prefix] = funcs[0]->name;
                } else {
                    // Multiple functions with this prefix - sorted_functions_ is already
                    // sorted by priority and then name, so the first match is correct
                    abbreviation_cache_[prefix] = funcs[0]->name;
                }
            }
        }

        // Also add full function names for completeness
        for (const auto &func_info : sorted_functions_) {
            abbreviation_cache_[func_info.name] = func_info.name;
        }

        abbreviation_cache_valid_ = true;
    }

  public:
    // Enhanced registration with category and context support
    static void register_function(std::string_view name, UniformFunction func, size_t priority,
                                  CommandCategory category = CommandCategory::UNKNOWN,
                                  ContextFlags context = CONTEXT_ALWAYS,
                                  std::optional<PermissionFlags> required_permissions = std::nullopt,
                                  std::string description = "") {
        std::string name_str(name);

        // Add to the map
        function_map_[name_str] = func;

        // Add to the sorted vector with enhanced metadata
        sorted_functions_.push_back(
            {name_str, func, priority, required_permissions.value_or(0), std::move(description), category, context});

        // Resort the vector to maintain the priority ordering
        std::sort(sorted_functions_.begin(), sorted_functions_.end());

        // Invalidate the abbreviation cache
        abbreviation_cache_valid_ = false;
    }

    // Runtime function registration with proper lambda support
    template <typename F> static void register_runtime_function(std::string_view name, F &&func, size_t priority) {
        // Create a handler for the lambda (preserves captures and state)
        auto handler = std::make_unique<LambdaHandlerImpl<F>>(std::forward<F>(func));

        // Add the handler to our storage (we'll own it now)
        lambda_handlers_.push_back(std::move(handler));

        // Create a trampoline function that calls the handler
        auto trampoline = [](CharData *data, Arguments args) {
            // Find the last handler (most recently added)
            auto &last_handler = lambda_handlers_.back();
            last_handler->invoke(data, args);
        };

        // Register the trampoline function
        register_function(name, trampoline, priority);
    }

    // Call a function by name - O(1) lookup
    static bool call(std::string_view name, CharData *character, Arguments args) {
        auto it = function_map_.find(std::string(name));
        if (it != function_map_.end()) {
            log("Executing function: {}", name);
            it->second(character, args);
            return true;
        }

        log("Function '{}' not found\n", name);
        return false;
    }

    // Check if a function can be called with given permissions
    static bool can_call_function(std::string_view name, PermissionFlags user_permissions, CharData *character = nullptr) {
        auto it = function_map_.find(std::string(name));
        if (it == function_map_.end()) {
            return false;
        }

        // Immortals bypass all permission checks
        if (character && GET_LEVEL(character) >= LVL_IMMORT) {
            return true;
        }

        // Find the function info to check permissions
        for (const auto &func_info : sorted_functions_) {
            if (func_info.name == name) {
                return (func_info.permissions & user_permissions) == func_info.permissions;
            }
        }
        return false;
    }

    // Call a function by abbreviation - O(1) lookup with cache and fuzzy matching fallback
    static bool call_by_abbrev(std::string_view abbrev, CharData *character, Arguments args) {
        // Ensure abbreviation cache is valid
        if (!abbreviation_cache_valid_) {
            update_abbreviation_cache();
        }

        // Try direct function lookup first
        auto direct_it = function_map_.find(std::string(abbrev));
        if (direct_it != function_map_.end()) {
            log("Executing function: {}", abbrev);
            direct_it->second(character, args);
            return true;
        }

        // Try the abbreviation cache
        auto abbrev_it = abbreviation_cache_.find(std::string(abbrev));
        if (abbrev_it != abbreviation_cache_.end()) {
            const std::string &full_name = abbrev_it->second;
            log("Executing function '{}' using abbreviation '{}'", full_name, abbrev);
            function_map_[full_name](character, args);
            return true;
        }

        // Try fuzzy matching as fallback for longer abbreviations
        if (abbrev.length() >= 4) {
            auto fuzzy_matches = find_fuzzy_matches(abbrev, 2);
            if (!fuzzy_matches.empty()) {
                // Only use fuzzy match if there's a clear best candidate
                if (fuzzy_matches.size() == 1 ||
                    (fuzzy_matches.size() <= 3 && fuzzy_matches[0].second < fuzzy_matches[1].second)) {
                    const auto &best_match = fuzzy_matches[0].first;
                    log("Executing function '{}' using fuzzy match for '{}' (distance: {})", best_match->name, abbrev,
                        fuzzy_matches[0].second);
                    best_match->func(character, args);
                    return true;
                }
            }
        }

        // If we get here, no matching function was found
        log("No function matches abbreviation '{}'\n", abbrev);
        return false;
    }

    // Call a function by abbreviation with permission checking
    static bool call_by_abbrev_with_permissions(std::string_view abbrev, CharData *character, Arguments args,
                                                PermissionFlags user_permissions) {
        // Ensure abbreviation cache is valid
        if (!abbreviation_cache_valid_) {
            update_abbreviation_cache();
        }

        std::string resolved_name;

        // Try direct function lookup first
        auto direct_it = function_map_.find(std::string(abbrev));
        if (direct_it != function_map_.end()) {
            resolved_name = std::string(abbrev);
        } else {
            // Try the abbreviation cache
            auto abbrev_it = abbreviation_cache_.find(std::string(abbrev));
            if (abbrev_it != abbreviation_cache_.end()) {
                resolved_name = abbrev_it->second;
            }
        }

        if (resolved_name.empty()) {
            // Try fuzzy matching as fallback for longer abbreviations
            if (abbrev.length() >= 4) {
                auto fuzzy_matches = find_fuzzy_matches(abbrev, 2);
                if (!fuzzy_matches.empty()) {
                    // Only use fuzzy match if there's a clear best candidate
                    if (fuzzy_matches.size() == 1 ||
                        (fuzzy_matches.size() <= 3 && fuzzy_matches[0].second < fuzzy_matches[1].second)) {

                        const auto &best_match = fuzzy_matches[0].first;

                        // Check permissions for the fuzzy match
                        if (can_call_function(best_match->name, user_permissions, character)) {
                            log("Executing function '{}' using fuzzy match for '{}' (distance: {})",
                                best_match->name, abbrev, fuzzy_matches[0].second);
                            best_match->func(character, args);
                            return true;
                        } else {
                            // Found a fuzzy match but no permission
                            log("Permission denied for fuzzy match function '{}'", best_match->name);
                            return false;
                        }
                    }
                }
            }

            log("No function matches abbreviation '{}'", abbrev);
            return false;
        }

        // Check permissions
        if (!can_call_function(resolved_name, user_permissions, character)) {
            // Find the function info to get the required permissions for logging
            PermissionFlags required_permissions = 0;
            for (const auto &func_info : sorted_functions_) {
                if (func_info.name == resolved_name) {
                    required_permissions = func_info.permissions;
                    break;
                }
            }
            log("Permission denied for function '{}' Required permissions: {}, Current permissions: {}", resolved_name,
                required_permissions, user_permissions);
            return false;
        }

        // Call the function
        log("Executing function '{}' with permission check", resolved_name);
        function_map_[resolved_name](character, args);
        return true;
    }

    // Calculate simple edit distance for fuzzy matching
    static int edit_distance(std::string_view s1, std::string_view s2, int max_distance = 3) {
        if (s1.empty())
            return static_cast<int>(s2.length());
        if (s2.empty())
            return static_cast<int>(s1.length());

        // Early exit if length difference exceeds max_distance
        int len_diff = std::abs(static_cast<int>(s1.length()) - static_cast<int>(s2.length()));
        if (len_diff > max_distance)
            return max_distance + 1;

        std::vector<int> prev(s2.length() + 1);
        std::vector<int> curr(s2.length() + 1);

        // Initialize first row
        for (size_t j = 0; j <= s2.length(); ++j) {
            prev[j] = static_cast<int>(j);
        }

        for (size_t i = 1; i <= s1.length(); ++i) {
            curr[0] = static_cast<int>(i);

            int min_in_row = curr[0];
            for (size_t j = 1; j <= s2.length(); ++j) {
                int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                curr[j] = std::min({
                    curr[j - 1] + 1,   // insertion
                    prev[j] + 1,       // deletion
                    prev[j - 1] + cost // substitution
                });
                min_in_row = std::min(min_in_row, curr[j]);
            }

            // Early exit if minimum distance in this row exceeds threshold
            if (min_in_row > max_distance) {
                return max_distance + 1;
            }

            prev = curr;
        }

        return curr[s2.length()];
    }

    // Find fuzzy matches when exact/prefix matching fails
    static std::vector<std::pair<const FunctionInfo *, int>> find_fuzzy_matches(std::string_view abbrev,
                                                                                int max_distance = 2) {
        std::vector<std::pair<const FunctionInfo *, int>> fuzzy_candidates;

        for (const auto &func : sorted_functions_) {
            int distance = edit_distance(abbrev, func.name, max_distance);
            if (distance <= max_distance) {
                fuzzy_candidates.emplace_back(&func, distance);
            }
        }

        // Sort by distance (closest first), then by priority
        std::sort(fuzzy_candidates.begin(), fuzzy_candidates.end(), [](const auto &a, const auto &b) {
            if (a.second != b.second)
                return a.second < b.second;
            return *a.first < *b.first;
        });

        return fuzzy_candidates;
    }

    // Enhanced abbreviation resolution with context awareness and fuzzy matching
    static bool call_by_abbrev_contextual(std::string_view abbrev, CharData *character, Arguments args,
                                          PermissionFlags user_permissions = 0,
                                          const UserCommandPreferences *user_prefs = nullptr) {
        if (!abbreviation_cache_valid_) {
            update_abbreviation_cache();
        }

        // Collect all matching functions (exact and prefix matches)
        std::vector<const FunctionInfo *> candidates;

        // Check for exact match first
        auto exact_it = function_map_.find(std::string(abbrev));
        if (exact_it != function_map_.end()) {
            for (const auto &func : sorted_functions_) {
                if (func.name == abbrev) {
                    candidates.push_back(&func);
                    break;
                }
            }
        }

        // Find prefix matches if no exact match
        if (candidates.empty()) {
            for (const auto &func : sorted_functions_) {
                if (func.name.starts_with(abbrev)) {
                    candidates.push_back(&func);
                }
            }
        }

        // Try fuzzy matching if no prefix matches found
        if (candidates.empty() && abbrev.length() >= 4) { // Only for longer abbreviations
            auto fuzzy_matches = find_fuzzy_matches(abbrev, 2);
            if (!fuzzy_matches.empty()) {
                // Only use fuzzy match if there's a clear best candidate or very few options
                if (fuzzy_matches.size() == 1 ||
                    (fuzzy_matches.size() <= 3 && fuzzy_matches[0].second < fuzzy_matches[1].second)) {
                    candidates.push_back(fuzzy_matches[0].first);
                    log("Using fuzzy match '{}' for abbreviation '{}' (distance: {})", fuzzy_matches[0].first->name,
                        abbrev, fuzzy_matches[0].second);
                }
            }
        }

        if (candidates.empty()) {
            log("No function matches abbreviation '{}'", abbrev);
            return false;
        }

        // Apply contextual filtering and user preferences
        auto best_match = select_best_command(candidates, character, user_prefs);
        if (!best_match) {
            log("No suitable function found for abbreviation '{}'", abbrev);
            return false;
        }

        // Check permissions if specified
        if (user_permissions != 0) {
            if ((best_match->permissions & user_permissions) != best_match->permissions) {
                log("Permission denied for function '{}'", best_match->name);
                return false;
            }
        }

        // Execute the selected command
        log("Executing contextual function '{}' using abbreviation '{}'", best_match->name, abbrev);
        best_match->func(character, args);
        return true;
    }

  private:
    // Select the best command from candidates based on context and user preferences
    static const FunctionInfo *select_best_command(const std::vector<const FunctionInfo *> &candidates,
                                                   CharData *character, const UserCommandPreferences *user_prefs) {
        if (candidates.empty())
            return nullptr;
        if (candidates.size() == 1)
            return candidates[0];

        // Apply user preference overrides first
        if (user_prefs && !user_prefs->abbreviation_overrides.empty()) {
            // Implementation would check user overrides here
        }

        // Apply contextual scoring
        std::vector<std::pair<const FunctionInfo *, int>> scored_candidates;
        for (const auto *candidate : candidates) {
            int score = calculate_contextual_score(*candidate, character, user_prefs);
            scored_candidates.emplace_back(candidate, score);
        }

        // Sort by score (highest first), then by the function's natural ordering
        std::sort(scored_candidates.begin(), scored_candidates.end(), [](const auto &a, const auto &b) {
            if (a.second != b.second) {
                return a.second > b.second; // Higher score first
            }
            return *a.first < *b.first; // Natural function ordering
        });

        return scored_candidates[0].first;
    }

    // Calculate contextual score for command selection
    static int calculate_contextual_score(const FunctionInfo &func, CharData *character,
                                          const UserCommandPreferences *user_prefs) {
        int score = 0;

        // Base category priority (from the comparison function)
        score += FunctionInfo::get_category_priority(func.category);

        // Context-specific bonuses
        if (character) {
            // Check if character is in combat
            bool in_combat = false; // You'd implement this check
            if (in_combat && (func.context_flags & CONTEXT_COMBAT)) {
                score += 25;
            } else if (!in_combat && (func.context_flags & CONTEXT_PEACEFUL)) {
                score += 15;
            }

            // Room-specific context bonuses
            // You'd add room type checks here based on character's location
        }

        // User preference bonuses
        if (user_prefs) {
            if (user_prefs->prefer_combat_commands && func.category == CommandCategory::COMBAT) {
                score += 10;
            }
            if (user_prefs->prefer_movement_commands && func.category == CommandCategory::MOVEMENT) {
                score += 10;
            }

            // Custom category priorities
            auto cat_prio = user_prefs->category_priorities.find(func.category);
            if (cat_prio != user_prefs->category_priorities.end()) {
                score += cat_prio->second;
            }
        }

        // Explicit priority bonus
        score += static_cast<int>(func.priority);

        return score;
    }

  public:
    // Execute all functions in priority order
    static void call_all(CharData *character, Arguments args) {
        log("Calling all functions in priority order:\n");
        for (const auto &func_info : sorted_functions_) {
            log("  {} (priority: {})\n", func_info.name, func_info.priority);
            func_info.func(character, args);
        }
    }

    static std::string print_available(PermissionFlags permissions) {
        if (!abbreviation_cache_valid_) {
            update_abbreviation_cache();
        }
        std::string result;
        result += "Available functions:\n";
        for (const auto &func_info : sorted_functions_) {
            if ((func_info.permissions & permissions) == permissions) {
                result += fmt::format("  {} (priority: {}) - {}\n", func_info.name, func_info.priority,
                                      func_info.description);
            }
        }
        return result;
    }

    // Print available functions with a specific prefix, formatting them nicely for user commands
    static std::string
    print_available_with_prefix(std::string_view prefix, PermissionFlags permissions,
                                std::function<std::string(std::string_view)> name_formatter = nullptr) {
        if (!abbreviation_cache_valid_) {
            update_abbreviation_cache();
        }
        std::string result;

        for (const auto &func_info : sorted_functions_) {
            if ((func_info.permissions & permissions) == func_info.permissions && func_info.name.starts_with(prefix)) {

                std::string display_name = func_info.name;
                if (name_formatter) {
                    display_name = name_formatter(func_info.name);
                }

                result += fmt::format("   {} - {}\n", display_name, func_info.description);
            }
        }
        return result;
    }

    // Print information about registered functions
    static void print_info() {
        log("Registry contains {} functions:\n", sorted_functions_.size());

        for (const auto &func_info : sorted_functions_) {
            log("  {} (priority: {})\n", func_info.name, func_info.priority);
        }
    }

    // Print available abbreviations
    static void print_abbreviations() {
        if (!abbreviation_cache_valid_) {
            update_abbreviation_cache();
        }

        log("Available abbreviations:\n");

        // Group by full function name for cleaner output
        std::unordered_map<std::string, std::vector<std::string>> grouped_abbrevs;

        for (const auto &[abbrev, full_name] : abbreviation_cache_) {
            if (abbrev != full_name) { // Don't show full names as abbreviations
                grouped_abbrevs[full_name].push_back(abbrev);
            }
        }

        // Print sorted by function name
        std::vector<std::string> func_names;
        for (const auto &[name, _] : grouped_abbrevs) {
            func_names.push_back(name);
        }
        std::sort(func_names.begin(), func_names.end());

        for (const auto &name : func_names) {
            auto &abbrevs = grouped_abbrevs[name];
            std::sort(abbrevs.begin(), abbrevs.end(), [](const auto &a, const auto &b) {
                return a.length() < b.length(); // Sort by length (shortest first)
            });

            std::string abbrev_list;
            for (const auto &abbrev : abbrevs) {
                if (!abbrev_list.empty())
                    abbrev_list += ", ";
                abbrev_list += abbrev;
            }

            log("  {} → {}\n", abbrev_list, name);
        }
    }

    // Remove a function by name
    static bool unregister_function(std::string_view name) {
        std::string name_str(name);

        // Remove from the map
        auto map_it = function_map_.find(name_str);
        if (map_it == function_map_.end()) {
            log("Function '{}' not found for removal\n", name);
            return false;
        }

        function_map_.erase(map_it);

        // Remove from the sorted vector
        auto vec_it = std::find_if(sorted_functions_.begin(), sorted_functions_.end(),
                                   [&name_str](const auto &info) { return info.name == name_str; });

        if (vec_it != sorted_functions_.end()) {
            sorted_functions_.erase(vec_it);
        }

        // Invalidate abbreviation cache
        abbreviation_cache_valid_ = false;

        log("Function '{}' removed from registry\n", name);
        return true;
    }
};

// Simple registration macro - only required parameters
#define REGISTER_FUNCTION(func, name, required_permissions, description)                                               \
    inline static const auto func##_reg = []() {                                                                       \
        FunctionRegistry::register_function(name, func, 0, CommandCategory::UNKNOWN, CONTEXT_ALWAYS,                   \
                                            required_permissions, description);                                        \
        return true;                                                                                                   \
    }();

// Registration with category for enhanced disambiguation
#define REGISTER_FUNCTION_WITH_CATEGORY(func, name, category, required_permissions, description)                       \
    inline static const auto func##_reg = []() {                                                                       \
        FunctionRegistry::register_function(name, func, 0, category, CONTEXT_ALWAYS, required_permissions,             \
                                            description);                                                              \
        return true;                                                                                                   \
    }();
