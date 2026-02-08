#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fierymud {

// Spell progression type determines how a class gains spell slots
enum class SpellProgression {
    None,  // No spellcasting (Warrior, Rogue, Monk)
    Third, // Third caster progression (Arcane Trickster)
    Half,  // Half caster progression (Paladin, Ranger)
    Full   // Full caster progression (Cleric, Sorcerer)
};

// Circle access configuration - when a class gains access to a spell circle
struct CircleAccess {
    int circle;    // Spell circle (1-9)
    int min_level; // Minimum level to access this circle
    int max_slots; // Maximum slots at this circle
};

// Class spellcasting configuration
struct ClassSpellConfig {
    std::string class_name;
    SpellProgression progression = SpellProgression::None;
    std::vector<CircleAccess> circles;

    [[nodiscard]] bool is_caster() const { return progression != SpellProgression::None; }
    [[nodiscard]] std::optional<CircleAccess> get_circle(int circle) const;
};

// Registry for class configurations
class ClassConfigRegistry {
  public:
    static ClassConfigRegistry &instance();

    // Initialize with default configurations
    void initialize_defaults();

    // Load configurations from JSON file
    bool load_from_file(const std::string &path);

    // Get configuration for a class (by lowercase name)
    [[nodiscard]] const ClassSpellConfig *get_config(std::string_view class_name) const;

    // Check if a class is a caster
    [[nodiscard]] bool is_caster(std::string_view class_name) const;

    // Get spell progression for a class
    [[nodiscard]] SpellProgression get_progression(std::string_view class_name) const;

  private:
    ClassConfigRegistry() = default;

    // Map of lowercase class name to config
    std::unordered_map<std::string, ClassSpellConfig> configs_;

    // Helper to add a full caster class
    void add_full_caster(std::string_view name);

    // Helper to add a half caster class
    void add_half_caster(std::string_view name);
};

} // namespace fierymud
