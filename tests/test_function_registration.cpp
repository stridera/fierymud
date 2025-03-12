#include <algorithm>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

// Forward declarations
struct CharData;
struct Arguments;

// The uniform function signature for all registered functions
using UniformFunction = void (*)(CharData *, Arguments);

// Character data structure
struct CharData {
    std::string name;
    int level;
    double health;

    void print_status() const {
        std::cout << std::format("Character: {}, Level: {}, Health: {}\n", name, level, health);
    }
};

// Arguments structure for passing parameters
struct Arguments {
    std::vector<std::string> args;

    int get_int(size_t index, int default_value = 0) const {
        if (index < args.size()) {
            try {
                return std::stoi(args[index]);
            } catch (...) {
                return default_value;
            }
        }
        return default_value;
    }

    double get_double(size_t index, double default_value = 0.0) const {
        if (index < args.size()) {
            try {
                return std::stod(args[index]);
            } catch (...) {
                return default_value;
            }
        }
        return default_value;
    }

    std::string_view get_string(size_t index, std::string_view default_value = "") const {
        if (index < args.size()) {
            return args[index];
        }
        return default_value;
    }

    Arguments() = default;
    Arguments(std::initializer_list<std::string> init_args) : args(init_args) {}
};

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
    static void register_function(std::string_view name, UniformFunction func, size_t priority) {
        std::string name_str(name);

        // Add to the map
        function_map_[name_str] = func;

        // Add to the sorted vector
        sorted_functions_.push_back({name_str, func, priority});

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
            std::cout << "boing\n";
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
            std::cout << std::format("Executing function: {}\n", name);
            it->second(character, args);
            return true;
        }

        std::cout << std::format("Function '{}' not found\n", name);
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
            std::cout << std::format("Executing function: {}\n", abbrev);
            direct_it->second(character, args);
            return true;
        }

        // Try the abbreviation cache
        auto abbrev_it = abbreviation_cache_.find(std::string(abbrev));
        if (abbrev_it != abbreviation_cache_.end()) {
            const std::string &full_name = abbrev_it->second;
            std::cout << std::format("Executing function '{}' using abbreviation '{}'\n", full_name, abbrev);
            function_map_[full_name](character, args);
            return true;
        }

        // If we get here, no matching function was found
        std::cout << std::format("No function matches abbreviation '{}'\n", abbrev);
        return false;
    }

    // Execute all functions in priority order
    static void call_all(CharData *character, Arguments args) {
        std::cout << "Calling all functions in priority order:\n";
        for (const auto &func_info : sorted_functions_) {
            std::cout << std::format("  {} (priority: {})\n", func_info.name, func_info.priority);
            func_info.func(character, args);
        }
    }

    // Print information about registered functions
    static void print_info() {
        std::cout << std::format("Registry contains {} functions:\n", sorted_functions_.size());

        for (const auto &func_info : sorted_functions_) {
            std::cout << std::format("  {} (priority: {})\n", func_info.name, func_info.priority);
        }
    }

    // Print available abbreviations
    static void print_abbreviations() {
        if (!abbreviation_cache_valid_) {
            update_abbreviation_cache();
        }

        std::cout << "Available abbreviations:\n";

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

            std::cout << std::format("  {} → {}\n", abbrev_list, name);
        }
    }

    // Remove a function by name
    static bool unregister_function(std::string_view name) {
        std::string name_str(name);

        // Remove from the map
        auto map_it = function_map_.find(name_str);
        if (map_it == function_map_.end()) {
            std::cout << std::format("Function '{}' not found for removal\n", name);
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

        std::cout << std::format("Function '{}' removed from registry\n", name);
        return true;
    }
};

// Macro for static function registration
#define REGISTER_FUNCTION(func, name, priority)                                                                        \
    inline static const auto func##_reg = []() {                                                                       \
        FunctionRegistry::register_function(name, func, priority);                                                     \
        return true;                                                                                                   \
    }()

// Example functions
void happy(CharData *character, Arguments arg) {}
REGISTER_FUNCTION(happy, "happy", 50);

void heal_character(CharData *character, Arguments args) {
    int amount = args.get_int(0, 10);
    character->health += amount;
    std::cout << std::format("Healed {} for {} points. New health: {}\n", character->name, amount, character->health);
}
REGISTER_FUNCTION(heal_character, "heal", 100);

void damage_character(CharData *character, Arguments args) {
    int amount = args.get_int(0, 5);
    character->health -= amount;
    std::cout << std::format("Damaged {} for {} points. New health: {}\n", character->name, amount, character->health);
}
REGISTER_FUNCTION(damage_character, "damage", 90);

// Add functions with the same priority for testing
void add_experience(CharData *character, Arguments args) {
    int amount = args.get_int(0, 100);
    std::cout << std::format("{} gained {} experience points\n", character->name, amount);
}
REGISTER_FUNCTION(add_experience, "addexp", 75);

void add_health(CharData *character, Arguments args) {
    int amount = args.get_int(0, 50);
    character->health += amount;
    std::cout << std::format("{} gained {} health points. New health: {}\n", character->name, amount,
                             character->health);
}
REGISTER_FUNCTION(add_health, "addhealth", 75);

int main() {
    // Print function information before runtime registration
    std::cout << "Initial function registry:\n";
    FunctionRegistry::print_info();
    FunctionRegistry::print_abbreviations();

    // Create a character
    CharData player = {"Hero", 1, 100.0};

    // Register functions at runtime using lambda
    FunctionRegistry::register_runtime_function(
        "buff",
        [](CharData *character, Arguments args) {
            int amount = args.get_int(0, 10);
            character->health += amount;
            character->level += 1;
            std::cout << std::format("{} received a buff! +{} health, +1 level\n", character->name, amount);
            std::cout << std::format("New stats: Level {}, Health {}\n", character->level, character->health);
        },
        95);

    // Register a runtime function with a captured variable
    int counter = 0;
    FunctionRegistry::register_runtime_function(
        "count",
        [counter](CharData *character, Arguments args) mutable {
            counter += args.get_int(0, 1);
            std::cout << std::format("Counter is now: {}\n", counter);
        },
        40);

    // Print updated function information
    std::cout << "\nUpdated function registry after runtime registration:\n";
    FunctionRegistry::print_info();
    FunctionRegistry::print_abbreviations();

    // Call the runtime registered functions
    std::cout << "\nCalling runtime registered functions:\n";
    FunctionRegistry::call("buff", &player, {"15"});
    FunctionRegistry::call("count", &player, {"5"});
    FunctionRegistry::call("count", &player, {"10"});

    // Call regular functions
    std::cout << "\nCalling regular functions:\n";
    FunctionRegistry::call("damage", &player, {"10"});
    FunctionRegistry::call("heal", &player, {"5"});

    // Abbreviation testing
    std::cout << "\nTesting abbreviations for runtime functions:\n";
    FunctionRegistry::call_by_abbrev("b", &player, {"20"}); // buff
    FunctionRegistry::call_by_abbrev("c", &player, {"3"});  // count

    // Test ambiguous abbreviations
    std::cout << "\nTesting ambiguous abbreviations (silent resolution):\n";
    FunctionRegistry::call_by_abbrev("add", &player, {"25"}); // Should select highest priority

    // Test unregistering a function
    std::cout << "\nTesting function removal:\n";
    FunctionRegistry::unregister_function("count");
    FunctionRegistry::print_info();

    // Try to call the removed function
    FunctionRegistry::call("count", &player, {"1"});

    return 0;
}
