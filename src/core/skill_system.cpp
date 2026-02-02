#include "skill_system.hpp"
#include "actor.hpp"
#include "player.hpp"
#include "ability_executor.hpp"
#include "../database/world_queries.hpp"
#include "../world/room.hpp"
#include <memory>

namespace FieryMUD {

SkillSystem& SkillSystem::instance() {
    static SkillSystem instance;
    return instance;
}

std::expected<void, SkillError> SkillSystem::execute_skill(Actor& actor, std::string_view skill_name,
                                                           Actor* target) {
    // Look up the ability in the cache
    const auto& cache = AbilityCache::instance();
    const auto* ability = cache.get_ability_by_name(skill_name);

    if (!ability) {
        return std::unexpected(SkillError{fmt::format("Unknown skill: {}", skill_name)});
    }

    // Check if this is a SKILL type ability (not SPELL, CHANT, or SONG)
    if (ability->type != WorldQueries::AbilityType::Skill) {
        return std::unexpected(SkillError{fmt::format("{} is not a skill", skill_name)});
    }

    // Get the room the actor is in
    auto room = actor.current_room();
    if (!room) {
        return std::unexpected(SkillError{"Actor is not in a room"});
    }

    // Get skill proficiency
    int skill_level = 50;  // Default
    if (auto* player = dynamic_cast<Player*>(&actor)) {
        if (player->is_god()) {
            skill_level = 100;
        } else {
            skill_level = player->get_proficiency(ability->id);
        }
    }

    // Convert Actor* to shared_ptr if we have a target
    std::shared_ptr<Actor> target_ptr;
    if (target) {
        // Find the target in the room's actor list
        for (const auto& room_actor : room->contents().actors) {
            if (room_actor.get() == target) {
                target_ptr = room_actor;
                break;
            }
        }
        if (!target_ptr) {
            return std::unexpected(SkillError{"Target not found in room"});
        }
    }

    // Get a shared_ptr to the actor
    std::shared_ptr<Actor> actor_ptr;
    for (const auto& room_actor : room->contents().actors) {
        if (room_actor.get() == &actor) {
            actor_ptr = room_actor;
            break;
        }
    }
    if (!actor_ptr) {
        return std::unexpected(SkillError{"Actor not found in room"});
    }

    // Build a minimal command context for execution
    CommandContext ctx;
    ctx.actor = actor_ptr;
    ctx.room = room;
    ctx.command.command = std::string(skill_name);
    ctx.start_time = std::chrono::steady_clock::now();

    // Execute the ability
    auto result = AbilityExecutor::execute_by_id(ctx, ability->id, target_ptr, skill_level);
    if (!result) {
        return std::unexpected(SkillError{result.error().message});
    }

    return {};
}

bool SkillSystem::skill_exists(std::string_view skill_name) const {
    const auto& cache = AbilityCache::instance();
    const auto* ability = cache.get_ability_by_name(skill_name);

    // Only return true if it exists and is a SKILL type
    return ability != nullptr && ability->type == WorldQueries::AbilityType::Skill;
}

int SkillSystem::get_skill_level(const Actor& actor, std::string_view skill_name) const {
    const auto& cache = AbilityCache::instance();
    const auto* ability = cache.get_ability_by_name(skill_name);

    if (!ability) {
        return 0;
    }

    // Players have proficiency tracking
    if (const auto* player = dynamic_cast<const Player*>(&actor)) {
        return player->get_proficiency(ability->id);
    }

    // Non-players (mobs) - check if they have the ability
    // For now, return 50 (average proficiency) for mobs
    // TODO: Add mob ability tracking if needed
    return 50;
}

void SkillSystem::set_skill_level(Actor& actor, std::string_view skill_name, int level) {
    const auto& cache = AbilityCache::instance();
    const auto* ability = cache.get_ability_by_name(skill_name);

    if (!ability) {
        return;
    }

    // Only players can have their proficiency set
    if (auto* player = dynamic_cast<Player*>(&actor)) {
        player->set_proficiency(ability->id, std::clamp(level, 0, 100));
    }
}

} // namespace FieryMUD
