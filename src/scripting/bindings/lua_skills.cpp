#include "lua_skills.hpp"
#include "../../core/actor.hpp"
#include "../../core/skill_system.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <spdlog/spdlog.h>

namespace FieryMUD {

void register_skill_bindings(sol::state& lua) {
    auto skills_table = lua.create_named_table("skills");

    // skills.execute(actor, skill_name, target?) - Execute a skill
    // Returns: (bool success, string? error)
    skills_table["execute"] = [](Actor* actor, const std::string& skill_name,
                                 sol::optional<Actor*> target)
        -> std::tuple<bool, sol::optional<std::string>> {
        if (!actor) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        if (skill_name.empty()) {
            return std::make_tuple(false, std::string("skill_not_found"));
        }

        auto& skill_system = SkillSystem::instance();

        // Check if skill exists
        if (!skill_system.skill_exists(skill_name)) {
            spdlog::debug("skills.execute: skill '{}' not found", skill_name);
            return std::make_tuple(false, std::string("skill_not_found"));
        }

        // Execute the skill
        Actor* target_actor = target.value_or(nullptr);
        auto result = skill_system.execute_skill(*actor, skill_name, target_actor);

        if (!result) {
            return std::make_tuple(false, result.error().message);
        }

        spdlog::debug("skills.execute: {} used '{}'{}", actor->name(), skill_name,
                      target_actor ? " on " + std::string(target_actor->name()) : "");
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    // skills.exists(skill_name) - Check if skill exists in database
    // Returns: bool
    skills_table["exists"] = [](const std::string& skill_name) -> bool {
        if (skill_name.empty()) {
            return false;
        }
        return SkillSystem::instance().skill_exists(skill_name);
    };

    // skills.get_level(actor, skill_name) - Get actor's skill level
    // Returns: int (0 if not known)
    skills_table["get_level"] = [](Actor* actor, const std::string& skill_name) -> int {
        if (!actor || skill_name.empty()) {
            return 0;
        }
        return SkillSystem::instance().get_skill_level(*actor, skill_name);
    };

    // skills.set_level(actor, skill_name, level) - Set actor's skill level
    // Returns: void
    skills_table["set_level"] = [](Actor* actor, const std::string& skill_name, int level) {
        if (!actor || skill_name.empty()) {
            return;
        }

        level = std::clamp(level, 0, 100);  // Skill levels are 0-100
        SkillSystem::instance().set_skill_level(*actor, skill_name, level);

        spdlog::debug("skills.set_level: set {} skill '{}' to {}", actor->name(), skill_name, level);
    };

    spdlog::debug("Registered skills Lua bindings");
}

} // namespace FieryMUD
