#include "arguments.hpp"
#include "logging.hpp"
#include "structs.hpp"

#include <algorithm>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

// All of our functions should include these headers
using UniformFunction = void (*)(CharData *, Arguments);
using PermissionFlags = uint8_t;

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
    // Function info structure
    struct FunctionInfo {
        std::string name;
        UniformFunction func;
        size_t priority;
        PermissionFlags bitflags;
        std::string description;

        // Comparison for sorting (by priority, then name)
        bool operator<(const FunctionInfo &other) const {
            if (priority != other.priority) {
                return priority > other.priority; // Higher priority first
            }
            return name < other.name; // Alphabetical for tie-breaking
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
    // Register a function (static version for compile-time registration)
    static void register_function(std::string_view name, UniformFunction func, size_t priority,
                                  PermissionFlags bitflags = 0, std::string description = "") {
        std::string name_str(name);

        // Add to the map
        function_map_[name_str] = func;

        // Add to the sorted vector
        sorted_functions_.push_back({name_str, func, priority, bitflags, description});

        // Resort the vector to maintain the priority/name ordering
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
            log("Executing function: {}\n", name);
            it->second(character, args);
            return true;
        }

        log("Function '{}' not found\n", name);
        return false;
    }

    // Check if a function can be called with given permissions
    static bool can_call_function(std::string_view name, PermissionFlags user_permissions) {
        auto it = function_map_.find(std::string(name));
        if (it == function_map_.end()) {
            return false;
        }
        
        // Find the function info to check permissions
        for (const auto &func_info : sorted_functions_) {
            if (func_info.name == name) {
                return (func_info.bitflags & user_permissions) == func_info.bitflags;
            }
        }
        return false;
    }

    // Call a function by abbreviation - O(1) lookup with cache
    static bool call_by_abbrev(std::string_view abbrev, CharData *character, Arguments args) {
        // Ensure abbreviation cache is valid
        if (!abbreviation_cache_valid_) {
            update_abbreviation_cache();
        }

        // Try direct function lookup first
        auto direct_it = function_map_.find(std::string(abbrev));
        if (direct_it != function_map_.end()) {
            log("Executing function: {}\n", abbrev);
            direct_it->second(character, args);
            return true;
        }

        // Try the abbreviation cache
        auto abbrev_it = abbreviation_cache_.find(std::string(abbrev));
        if (abbrev_it != abbreviation_cache_.end()) {
            const std::string &full_name = abbrev_it->second;
            log("Executing function '{}' using abbreviation '{}'\n", full_name, abbrev);
            function_map_[full_name](character, args);
            return true;
        }

        // If we get here, no matching function was found
        log("No function matches abbreviation '{}'\n", abbrev);
        return false;
    }

    // Call a function by abbreviation with permission checking
    static bool call_by_abbrev_with_permissions(std::string_view abbrev, CharData *character, Arguments args, PermissionFlags user_permissions) {
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
            log("No function matches abbreviation '{}'\n", abbrev);
            return false;
        }
        
        // Check permissions
        if (!can_call_function(resolved_name, user_permissions)) {
            log("Permission denied for function '{}'\n", resolved_name);
            return false;
        }
        
        // Call the function
        log("Executing function '{}' with permission check\n", resolved_name);
        function_map_[resolved_name](character, args);
        return true;
    }

    // Execute all functions in priority order
    static void call_all(CharData *character, Arguments args) {
        log("Calling all functions in priority order:\n");
        for (const auto &func_info : sorted_functions_) {
            log("  {} (priority: {})\n", func_info.name, func_info.priority);
            func_info.func(character, args);
        }
    }

    static std::string print_available(PermissionFlags bitflags) {
        if (!abbreviation_cache_valid_) {
            update_abbreviation_cache();
        }
        std::string result;
        result += "Available functions:\n";
        for (const auto &func_info : sorted_functions_) {
            if ((func_info.bitflags & bitflags) == bitflags) {
                result += fmt::format("  {} (priority: {}) - {}\n", func_info.name, func_info.priority,
                                      func_info.description);
            }
        }
        return result;
    }

    // Print available functions with a specific prefix, formatting them nicely for user commands
    static std::string print_available_with_prefix(std::string_view prefix, PermissionFlags bitflags, 
                                                   std::function<std::string(std::string_view)> name_formatter = nullptr) {
        if (!abbreviation_cache_valid_) {
            update_abbreviation_cache();
        }
        std::string result;
        
        for (const auto &func_info : sorted_functions_) {
            if ((func_info.bitflags & bitflags) == func_info.bitflags && 
                func_info.name.starts_with(prefix)) {
                
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

// Macro for static function registration
#define REGISTER_FUNCTION(func, name, priority, bitflag, description)                                                  \
    inline static const auto func##_reg = []() {                                                                       \
        FunctionRegistry::register_function(name, func, priority, bitflag, description);                               \
        return true;                                                                                                   \
    }();

