#include "class_config.hpp"

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

#include "core/logging.hpp"
#include "text/string_utils.hpp"

namespace fierymud {

std::optional<CircleAccess> ClassSpellConfig::get_circle(int circle) const {
    auto it =
        std::find_if(circles.begin(), circles.end(), [circle](const CircleAccess &ca) { return ca.circle == circle; });
    if (it != circles.end()) {
        return *it;
    }
    return std::nullopt;
}

ClassConfigRegistry &ClassConfigRegistry::instance() {
    static ClassConfigRegistry registry;
    return registry;
}

void ClassConfigRegistry::add_full_caster(std::string_view name) {
    ClassSpellConfig config;
    config.class_name = std::string{name};
    config.progression = SpellProgression::Full;

    // Full casters get access to all 9 circles
    // Standard D&D-style progression: Circle N available at level (N-1)*2+1
    // Max 4 slots per circle
    for (int circle = 1; circle <= 9; ++circle) {
        config.circles.push_back({.circle = circle, .min_level = (circle - 1) * 2 + 1, .max_slots = 4});
    }

    configs_[to_lowercase(name)] = std::move(config);
}

void ClassConfigRegistry::add_half_caster(std::string_view name) {
    ClassSpellConfig config;
    config.class_name = std::string{name};
    config.progression = SpellProgression::Half;

    // Half casters get circles 1-5 at higher level requirements
    // Circle N available at level (N-1)*4+2, max 3 slots
    for (int circle = 1; circle <= 5; ++circle) {
        config.circles.push_back({.circle = circle, .min_level = (circle - 1) * 4 + 2, .max_slots = 3});
    }

    configs_[to_lowercase(name)] = std::move(config);
}

void ClassConfigRegistry::initialize_defaults() {
    configs_.clear();

    // Full casters - all 9 circles, standard progression
    add_full_caster("cleric");
    add_full_caster("sorcerer");
    add_full_caster("druid");
    add_full_caster("necromancer");
    add_full_caster("conjurer");
    add_full_caster("priest");
    add_full_caster("diabolist");
    add_full_caster("shaman");
    add_full_caster("wizard");
    add_full_caster("mage");

    // Half casters - circles 1-5, slower progression
    add_half_caster("paladin");
    add_half_caster("ranger");
    add_half_caster("anti-paladin");
    add_half_caster("bard");

    // Non-casters get no entries - they'll return SpellProgression::None

    Log::info("ClassConfigRegistry: Initialized {} class configurations", configs_.size());
}

bool ClassConfigRegistry::load_from_file(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Log::warn("ClassConfigRegistry: Could not open config file: {}", path);
        return false;
    }

    try {
        nlohmann::json json;
        file >> json;

        for (const auto &[class_name, class_json] : json.items()) {
            ClassSpellConfig config;
            config.class_name = class_name;

            // Parse progression type
            std::string prog = class_json.value("progression", "none");
            if (prog == "full") {
                config.progression = SpellProgression::Full;
            } else if (prog == "half") {
                config.progression = SpellProgression::Half;
            } else if (prog == "third") {
                config.progression = SpellProgression::Third;
            } else {
                config.progression = SpellProgression::None;
            }

            // Parse circles
            if (class_json.contains("circles")) {
                for (const auto &circle_json : class_json["circles"]) {
                    config.circles.push_back({.circle = circle_json.at("circle").get<int>(),
                                              .min_level = circle_json.at("min_level").get<int>(),
                                              .max_slots = circle_json.value("max_slots", 4)});
                }
            }

            configs_[to_lowercase(class_name)] = std::move(config);
        }

        Log::info("ClassConfigRegistry: Loaded {} class configurations from {}", configs_.size(), path);
        return true;

    } catch (const std::exception &e) {
        Log::error("ClassConfigRegistry: Failed to parse config file: {}", e.what());
        return false;
    }
}

const ClassSpellConfig *ClassConfigRegistry::get_config(std::string_view class_name) const {
    std::string lower = to_lowercase(class_name);
    auto it = configs_.find(lower);
    if (it != configs_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool ClassConfigRegistry::is_caster(std::string_view class_name) const {
    const auto *config = get_config(class_name);
    return config && config->is_caster();
}

SpellProgression ClassConfigRegistry::get_progression(std::string_view class_name) const {
    const auto *config = get_config(class_name);
    if (config) {
        return config->progression;
    }
    return SpellProgression::None;
}

} // namespace fierymud
