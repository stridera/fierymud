#include <algorithm>
#include <cstdint>
#include <format>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <vector>

// Simple game-like structures
struct Chardata {
    std::string name;
    int health;
    int mana;
    int level;
    uint64_t classFlags;

    void printStatus() const { std::cout << std::format("{} - Health: {}, Mana: {}\n", name, health, mana); }
};

struct Arguments {
    std::vector<std::string> args;

    Arguments(const std::vector<std::string> &_args) : args(_args) {}
    Arguments(std::initializer_list<std::string> _args) : args(_args) {}

    std::string getArg(size_t index, const std::string &defaultValue = "") const {
        return index < args.size() ? args[index] : defaultValue;
    }

    int getIntArg(size_t index, int defaultValue = 0) const {
        if (index >= args.size())
            return defaultValue;
        try {
            return std::stoi(args[index]);
        } catch (...) {
            return defaultValue;
        }
    }
};

// Type alias for the function signature
using FunctionType = void (*)(Chardata *, Arguments);

// Function registry that manages registered functions with priorities and bitflags
class FunctionRegistry {
  public:
    // Register a function with a key derived from the function name, a priority, and bitflags
    template <auto Function> static bool registerFunction(int priority = 0, uint64_t bitflags = 0) {
        auto &registry = getInstance();
        constexpr auto functionName = getFunctionName<Function>();
        std::string key = convertToKey(functionName);

        registry.m_functions.emplace(
            key, FunctionInfo{.function = Function, .name = functionName, .priority = priority, .bitflags = bitflags});

        // Add to the available keys for abbreviation matching
        if constexpr (std::is_same_v<std::string, std::string>) {
            registry.m_availableKeys.insert(key);
        }

        return true;
    }

    // Explicitly register a function with a specified key
    template <auto Function> static bool registerFunction(std::string key, int priority = 0, uint64_t bitflags = 0) {
        auto &registry = getInstance();
        constexpr auto functionName = getFunctionName<Function>();

        registry.m_functions.emplace(
            key, FunctionInfo{.function = Function, .name = functionName, .priority = priority, .bitflags = bitflags});

        // Add to the available keys for abbreviation matching
        if constexpr (std::is_same_v<std::string, std::string>) {
            registry.m_availableKeys.insert(key);
        }

        return true;
    }

    // Call functions with a specific key, ordered by priority, and filtered by bitflags
    void callFunctions(const std::string &key, Chardata *chardata, Arguments args, uint64_t classFlags = ~0ULL) const {
        if constexpr (std::is_same_v<std::string, std::string>) {
            // For string keys, handle abbreviations
            std::string actualKey = findBestMatch(key);
            if (actualKey.empty()) {
                std::cout << "Unknown command: " << key << "\n";
                return;
            }

            if (actualKey != key) {
                std::cout << "Using command: " << actualKey << "\n";
            }

            callExactFunctions(actualKey, chardata, args, classFlags);
        }
    }

    // Get all registered functions
    auto getAllFunctions() const {
        std::vector<std::tuple<std::string, std::string_view, int, uint64_t>> result;
        result.reserve(m_functions.size());

        // Create a set of unique keys to avoid duplicates from the multimap
        std::set<std::string> uniqueKeys;
        for (const auto &[key, _] : m_functions) {
            uniqueKeys.insert(key);
        }

        // For each unique key, find the function with the highest priority
        for (const auto &key : uniqueKeys) {
            auto range = m_functions.equal_range(key);

            FunctionInfo highestPriorityFunc = range.first->second;
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second.priority > highestPriorityFunc.priority) {
                    highestPriorityFunc = it->second;
                }
            }

            result.emplace_back(key, highestPriorityFunc.name, highestPriorityFunc.priority,
                                highestPriorityFunc.bitflags);
        }

        return result;
    }

    // List available commands
    void listCommands() const {
        auto funcs = getAllFunctions();

        std::cout << "Available commands:\n";
        std::cout << "==================\n";

        for (const auto &[key, name, priority, flags] : funcs) {
            std::cout << std::format("{:<15} Priority: {:<5} Flags: {:#x}\n", key, priority, flags);
        }
    }

    // Clear all registered functions
    void clear() {
        m_functions.clear();
        m_availableKeys.clear();
    }

    // Access singleton instance
    static FunctionRegistry &getInstance() {
        static FunctionRegistry instance;
        return instance;
    }

  private:
    // Information about a registered function
    struct FunctionInfo {
        FunctionType function;
        std::string_view name;
        int priority;
        uint64_t bitflags;
    };

    // Call functions with an exact key match
    void callExactFunctions(const std::string &exactKey, Chardata *chardata, Arguments args,
                            uint64_t classFlags) const {
        auto range = m_functions.equal_range(exactKey);

        // Convert the range into a vector for sorting
        std::vector<FunctionInfo> matchingFunctions;
        for (auto it = range.first; it != range.second; ++it) {
            matchingFunctions.push_back(it->second);
        }

        // Sort by priority (highest first), then by name (alphabetical)
        std::ranges::sort(matchingFunctions, [](const auto &a, const auto &b) {
            if (a.priority != b.priority) {
                return a.priority > b.priority;
            }
            return a.name < b.name;
        });

        // Call functions that match the class flags
        bool foundMatch = false;
        for (const auto &info : matchingFunctions) {
            if ((info.bitflags & classFlags) != 0) {
                info.function(chardata, args);
                foundMatch = true;
            }
        }

        if (!foundMatch) {
            std::cout << "You don't have the required class to use this command.\n";
        }
    }

    // Find the best matching command based on abbreviation
    std::string findBestMatch(const std::string &abbr) const {
        // If the exact key exists, use it
        if (m_availableKeys.contains(abbr)) {
            return abbr;
        }

        // If it's an abbreviation, find commands that start with it
        std::vector<std::string> candidates;
        for (const auto &key : m_availableKeys) {
            // Check if key starts with the abbreviation
            if (key.starts_with(abbr)) {
                candidates.push_back(key);
            }
        }

        // If exactly one match, return it
        if (candidates.size() == 1) {
            return candidates[0];
        }

        // If multiple matches but one is exact length match, prioritize it
        for (const auto &candidate : candidates) {
            if (candidate.length() == abbr.length()) {
                return candidate;
            }
        }

        // If there are candidates but no exact match, return the one with highest priority
        if (!candidates.empty()) {
            // Find the function with highest priority among all candidates
            int highestPriority = std::numeric_limits<int>::min();
            std::string highestPriorityCandidate;

            for (const auto &candidate : candidates) {
                // Find all functions registered with this key
                auto range = m_functions.equal_range(candidate);

                // Find the highest priority among functions with this key
                for (auto it = range.first; it != range.second; ++it) {
                    if (it->second.priority > highestPriority) {
                        highestPriority = it->second.priority;
                        highestPriorityCandidate = candidate;
                    }
                }
            }

            // If we found a candidate with priority, return it
            if (!highestPriorityCandidate.empty()) {
                return highestPriorityCandidate;
            }

            // Fallback to shortest if no priority info (shouldn't happen in practice)
            auto shortest = std::ranges::min_element(
                candidates, [](const auto &a, const auto &b) { return a.length() < b.length(); });
            return *shortest;
        }

        // No match found
        return {};
    }

    // Get function name at compile time
    template <auto Function> static consteval std::string_view getFunctionName() {
        constexpr std::string_view fullName = __PRETTY_FUNCTION__;
        constexpr auto startPos = fullName.find("Function = ") + 11;
        constexpr auto endPos = fullName.rfind(";");
        constexpr auto length = endPos - startPos;

        // Extract just the function name from the full signature
        constexpr auto functionSig = fullName.substr(startPos, length);
        constexpr auto parenPos = functionSig.find('(');
        constexpr auto nameOnly = functionSig.substr(0, parenPos);

        return nameOnly;
    }

    // Convert a function name to a registry key
    static std::string convertToKey(std::string_view functionName) {
        if constexpr (std::is_same_v<std::string, std::string> || std::convertible_to<std::string_view, std::string>) {
            return std::string(functionName);
        } else {
            // For other key types, this would need a specific implementation
            static_assert(std::is_constructible_v<std::string, std::string_view>,
                          "std::string must be constructible from std::string_view");
            return std::string(functionName);
        }
    }

    // Storage for registered functions, allowing multiple functions per key
    std::multimap<std::string, FunctionInfo> m_functions;

    // Set of all available command keys (for string keys only)
    std::set<std::string> m_availableKeys;

    // Private constructor for singleton
    FunctionRegistry() = default;
};

// Macro for convenient function registration
#define REGISTER_FUNCTION(func, priority, bitflags)                                                                    \
    inline static bool func##_registered = FunctionRegistry::registerFunction<func>(priority, bitflags)

// Macro for registering a function with an explicit key
#define REGISTER_FUNCTION_WITH_KEY(func, key, priority, bitflags)                                                      \
    inline static bool func##_registered = FunctionRegistry::registerFunction<func>(key, priority, bitflags)

// Define some class flags
enum ClassFlags {
    CLASS_NONE = 0,
    CLASS_WARRIOR = 1 << 0,
    CLASS_MAGE = 1 << 1,
    CLASS_CLERIC = 1 << 2,
    CLASS_ROGUE = 1 << 3,
    CLASS_PALADIN = 1 << 4,
    CLASS_ALL = ~0ULL
};

//===== ACTUAL GAME FUNCTIONS =====

void heal(Chardata *chardata, Arguments args) {
    int amount = args.getIntArg(0, 10);
    std::cout << std::format("Healing {} for {} points\n", chardata->name, amount);
    chardata->health += amount;
    chardata->mana -= 5;
    chardata->printStatus();
}
REGISTER_FUNCTION(heal, 100, CLASS_CLERIC | CLASS_PALADIN);

void healMassive(Chardata *chardata, Arguments args) {
    int amount = args.getIntArg(0, 50);
    std::cout << std::format("Casting MASSIVE healing on {} for {} points\n", chardata->name, amount);
    chardata->health += amount;
    chardata->mana -= 20;
    chardata->printStatus();
}
REGISTER_FUNCTION(healMassive, 200, CLASS_CLERIC);

void fireball(Chardata *chardata, Arguments args) {
    std::string target = args.getArg(0, "enemy");
    int damage = 15 + chardata->level * 2;
    std::cout << std::format("{} casts Fireball at {}, dealing {} damage!\n", chardata->name, target, damage);
    chardata->mana -= 10;
    chardata->printStatus();
}
REGISTER_FUNCTION(fireball, 150, CLASS_MAGE);

void fireBlast(Chardata *chardata, Arguments args) {
    std::string target = args.getArg(0, "enemy");
    int damage = 25 + chardata->level * 3;
    std::cout << std::format("{} casts Fire Blast at {}, dealing {} damage!\n", chardata->name, target, damage);
    chardata->mana -= 15;
    chardata->printStatus();
}
REGISTER_FUNCTION(fireBlast, 180, CLASS_MAGE);

void slash(Chardata *chardata, Arguments args) {
    std::string target = args.getArg(0, "enemy");
    int damage = 10 + chardata->level;
    std::cout << std::format("{} slashes at {}, dealing {} damage!\n", chardata->name, target, damage);
    chardata->printStatus();
}
REGISTER_FUNCTION(slash, 100, CLASS_WARRIOR | CLASS_ROGUE);

void help(Chardata *chardata, Arguments args) {
    std::cout << "=== GAME HELP ===\n";
    std::cout << "Player: " << chardata->name << " (Level " << chardata->level << ")\n";

    std::cout << "\nClass abilities available:\n";
    FunctionRegistry::getInstance().listCommands();

    std::cout << "\nCommands can be abbreviated (e.g., 'h' for 'heal')\n";
    std::cout << "When abbreviations match multiple commands, the one with highest priority is chosen\n";
}
REGISTER_FUNCTION(help, 999, CLASS_ALL); // Highest priority, available to all classes

void status(Chardata *chardata, Arguments args) {
    std::cout << "=== CHARACTER STATUS ===\n";
    std::cout << "Name: " << chardata->name << "\n";
    std::cout << "Level: " << chardata->level << "\n";
    std::cout << "Health: " << chardata->health << "\n";
    std::cout << "Mana: " << chardata->mana << "\n";

    std::cout << "Classes: ";
    if (chardata->classFlags & CLASS_WARRIOR)
        std::cout << "Warrior ";
    if (chardata->classFlags & CLASS_MAGE)
        std::cout << "Mage ";
    if (chardata->classFlags & CLASS_CLERIC)
        std::cout << "Cleric ";
    if (chardata->classFlags & CLASS_ROGUE)
        std::cout << "Rogue ";
    if (chardata->classFlags & CLASS_PALADIN)
        std::cout << "Paladin ";
    std::cout << "\n";
}
REGISTER_FUNCTION(status, 900, CLASS_ALL); // High priority, available to all classes

int main() {
    // Create a character
    Chardata player{
        .name = "Aetheria",
        .health = 100,
        .mana = 50,
        .level = 5,
        .classFlags = CLASS_MAGE | CLASS_CLERIC // Multi-class character
    };

    std::cout << "=== Command System Demo ===\n";
    std::cout << "Type 'help' for available commands\n";
    std::cout << "Type 'quit' to exit\n\n";

    std::cout << "h\n";
    FunctionRegistry::getInstance().callFunctions("h", &player, {"10"}, player.classFlags);
    std::cout << "he\n";
    FunctionRegistry::getInstance().callFunctions("he", &player, {"10"}, player.classFlags);
    std::cout << "hea\n";
    FunctionRegistry::getInstance().callFunctions("hea", &player, {"10"}, player.classFlags);
    std::cout << "heal\n";
    FunctionRegistry::getInstance().callFunctions("heal", &player, {"10"}, player.classFlags);

    std::cout << "Thanks for playing!\n";
    return 0;
}