#include "core/ability_executor.hpp"

#include "core/actor.hpp"
#include "core/combat.hpp"
#include "core/logging.hpp"
#include "database/connection_pool.hpp"
#include "database/world_queries.hpp"

#include <algorithm>
#include <cctype>

#include <fmt/format.h>

namespace FieryMUD {

// =============================================================================
// AbilityExecutionResult
// =============================================================================

AbilityExecutionResult AbilityExecutionResult::failure(std::string_view error) {
    AbilityExecutionResult result;
    result.success = false;
    result.error_message = error;
    return result;
}

// =============================================================================
// AbilityCache
// =============================================================================

AbilityCache& AbilityCache::instance() {
    static AbilityCache cache;
    return cache;
}

Result<void> AbilityCache::initialize() {
    if (initialized_) {
        return {}; // Already initialized
    }

    return reload();
}

Result<void> AbilityCache::reload() {
    auto logger = Log::game();
    logger->debug("Loading ability cache from database...");

    auto result = ConnectionPool::instance().execute([this](pqxx::work& txn) -> Result<void> {
        // Load all abilities
        auto abilities_result = WorldQueries::load_all_abilities(txn);
        if (!abilities_result) {
            return std::unexpected(abilities_result.error());
        }

        abilities_.clear();
        ability_name_index_.clear();

        for (auto& ability : *abilities_result) {
            int id = ability.id;

            // Build name index (lowercase)
            std::string lower_name = ability.plain_name;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                          [](unsigned char c) { return std::tolower(c); });
            ability_name_index_[lower_name] = id;

            abilities_.emplace(id, std::move(ability));
        }

        // Load all effects
        auto effects_result = WorldQueries::load_all_effects(txn);
        if (!effects_result) {
            return std::unexpected(effects_result.error());
        }

        effects_.clear();
        for (const auto& effect_data : *effects_result) {
            EffectDefinition def;
            def.id = effect_data.id;
            def.name = effect_data.name;
            def.description = effect_data.description;
            def.type = parse_effect_type(effect_data.effect_type);
            def.default_params = EffectParams::from_json_string(effect_data.default_params);
            effects_.emplace(effect_data.id, std::move(def));
        }

        // Load ability effects for all abilities
        std::vector<int> ability_ids;
        ability_ids.reserve(abilities_.size());
        for (const auto& [id, _] : abilities_) {
            ability_ids.push_back(id);
        }

        auto ability_effects_result = WorldQueries::load_abilities_effects(txn, ability_ids);
        if (!ability_effects_result) {
            return std::unexpected(ability_effects_result.error());
        }

        ability_effects_.clear();
        for (const auto& effect_data : *ability_effects_result) {
            AbilityEffect effect;
            effect.ability_id = effect_data.ability_id;
            effect.effect_id = effect_data.effect_id;
            effect.order = effect_data.order;
            effect.trigger = parse_effect_trigger(effect_data.trigger);
            effect.chance_percent = effect_data.chance_percent;
            effect.condition = effect_data.condition;

            // Merge default params with override
            auto effect_it = effects_.find(effect_data.effect_id);
            if (effect_it != effects_.end()) {
                effect.params = effect_it->second.default_params;
            }
            effect.params.merge_override(effect_data.override_params);

            ability_effects_[effect_data.ability_id].push_back(std::move(effect));
        }

        return {};
    });

    if (!result) {
        logger->error("Failed to load ability cache: {}", result.error().message);
        return std::unexpected(result.error());
    }

    initialized_ = true;
    logger->info("Ability cache loaded: {} abilities, {} effects",
                abilities_.size(), effects_.size());
    return {};
}

const WorldQueries::AbilityData* AbilityCache::get_ability(int ability_id) const {
    auto it = abilities_.find(ability_id);
    return it != abilities_.end() ? &it->second : nullptr;
}

const WorldQueries::AbilityData* AbilityCache::get_ability_by_name(std::string_view name) const {
    std::string lower_name(name);
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                  [](unsigned char c) { return std::tolower(c); });

    auto it = ability_name_index_.find(lower_name);
    if (it == ability_name_index_.end()) {
        return nullptr;
    }

    return get_ability(it->second);
}

const EffectDefinition* AbilityCache::get_effect(int effect_id) const {
    auto it = effects_.find(effect_id);
    return it != effects_.end() ? &it->second : nullptr;
}

std::vector<AbilityEffect> AbilityCache::get_ability_effects(int ability_id) const {
    auto it = ability_effects_.find(ability_id);
    return it != ability_effects_.end() ? it->second : std::vector<AbilityEffect>{};
}

// =============================================================================
// AbilityExecutor
// =============================================================================

std::expected<AbilityExecutionResult, Error> AbilityExecutor::execute(
    const CommandContext& ctx,
    std::string_view ability_name,
    std::shared_ptr<Actor> target,
    int skill_level) {

    // Ensure cache is initialized
    auto& cache = AbilityCache::instance();
    if (!cache.is_initialized()) {
        auto init_result = cache.initialize();
        if (!init_result) {
            return std::unexpected(init_result.error());
        }
    }

    // Find ability by name
    const auto* ability = cache.get_ability_by_name(ability_name);
    if (!ability) {
        return std::unexpected(Errors::NotFound(fmt::format("ability '{}'", ability_name)));
    }

    return execute_by_id(ctx, ability->id, target, skill_level);
}

std::expected<AbilityExecutionResult, Error> AbilityExecutor::execute_by_id(
    const CommandContext& ctx,
    int ability_id,
    std::shared_ptr<Actor> target,
    int skill_level) {

    auto& cache = AbilityCache::instance();

    const auto* ability = cache.get_ability(ability_id);
    if (!ability) {
        return std::unexpected(Errors::NotFound(fmt::format("ability with ID {}", ability_id)));
    }

    // Check prerequisites
    auto prereq_check = check_prerequisites(ctx, *ability, target);
    if (!prereq_check) {
        return std::unexpected(prereq_check.error());
    }

    // Build effect context
    EffectContext effect_ctx;
    effect_ctx.actor = ctx.actor;
    effect_ctx.target = target;
    effect_ctx.skill_level = skill_level;
    effect_ctx.ability_name = ability->name;  // Display name for effect tracking
    effect_ctx.build_formula_context();

    // Execute effects - spells use OnCast trigger
    auto effects_result = execute_effects(*ability, effect_ctx, EffectTrigger::OnCast);
    if (!effects_result) {
        return std::unexpected(effects_result.error());
    }

    // Record cooldown after successful execution
    // Note: FieryMUD uses spell circles (memorization), not mana costs
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (player) {
        // Record ability use for cooldown tracking
        player->record_ability_use(ability->id);
    }

    // Build result
    AbilityExecutionResult result;
    result.success = true;
    result.effect_results = std::move(*effects_result);

    // Consolidate messages and calculate totals
    std::vector<std::string> attacker_msgs, target_msgs, room_msgs;

    for (const auto& effect_result : result.effect_results) {
        if (!effect_result.success) continue;

        result.total_damage += effect_result.value;

        if (!effect_result.attacker_message.empty()) {
            attacker_msgs.push_back(effect_result.attacker_message);
        }
        if (!effect_result.target_message.empty()) {
            target_msgs.push_back(effect_result.target_message);
        }
        if (!effect_result.room_message.empty()) {
            room_msgs.push_back(effect_result.room_message);
        }
    }

    // Join messages
    for (size_t i = 0; i < attacker_msgs.size(); ++i) {
        if (i > 0) result.attacker_message += " ";
        result.attacker_message += attacker_msgs[i];
    }
    for (size_t i = 0; i < target_msgs.size(); ++i) {
        if (i > 0) result.target_message += " ";
        result.target_message += target_msgs[i];
    }
    for (size_t i = 0; i < room_msgs.size(); ++i) {
        if (i > 0) result.room_message += " ";
        result.room_message += room_msgs[i];
    }

    return result;
}

std::expected<void, Error> AbilityExecutor::check_prerequisites(
    const CommandContext& ctx,
    const WorldQueries::AbilityData& ability,
    std::shared_ptr<Actor> target) {

    // Check position
    int actor_position = static_cast<int>(ctx.actor->position());
    if (actor_position < ability.min_position) {
        return std::unexpected(Errors::InvalidState(
            fmt::format("You must be {} to use {}",
                       ActorUtils::get_position_name(static_cast<Position>(ability.min_position)),
                       ability.plain_name)));
    }

    // Check if violent ability requires target
    if (ability.violent && !target) {
        return std::unexpected(Errors::InvalidArgument("target",
            fmt::format("{} requires a target", ability.plain_name)));
    }

    // Cooldown check (for players only)
    if (ability.cooldown_ms > 0) {
        auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
        if (player) {
            const auto* learned = player->get_ability(ability.id);
            if (learned) {
                auto now = std::chrono::system_clock::now();
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - learned->last_used).count();
                if (elapsed_ms < ability.cooldown_ms) {
                    int remaining_sec = static_cast<int>((ability.cooldown_ms - elapsed_ms) / 1000) + 1;
                    return std::unexpected(Errors::InvalidState(
                        fmt::format("You must wait {} more second{} to use {}.",
                                   remaining_sec, remaining_sec == 1 ? "" : "s",
                                   ability.plain_name)));
                }
            }
        }
    }

    // Note: FieryMUD uses spell circles (memorization), not mana costs
    // TODO: Add spell circle validation when spell memorization system is implemented

    return {};
}

std::expected<std::vector<EffectResult>, Error> AbilityExecutor::execute_effects(
    const WorldQueries::AbilityData& ability,
    EffectContext& context,
    EffectTrigger trigger) {

    auto& cache = AbilityCache::instance();

    // Get effects for this ability
    auto ability_effects = cache.get_ability_effects(ability.id);
    if (ability_effects.empty()) {
        // No effects defined - return empty result
        Log::game()->debug("Ability {} has no effects defined", ability.plain_name);
        return std::vector<EffectResult>{};
    }

    // Build effect definitions map
    std::unordered_map<int, EffectDefinition> effect_defs;
    for (const auto& ability_effect : ability_effects) {
        const auto* def = cache.get_effect(ability_effect.effect_id);
        if (def) {
            effect_defs[def->id] = *def;
        }
    }

    // Execute all effects
    return EffectExecutor::execute_ability_effects(
        ability_effects, effect_defs, context, trigger);
}

// =============================================================================
// Generic Skill Command Handler
// =============================================================================

Result<CommandResult> execute_skill_command(
    const CommandContext& ctx,
    std::string_view skill_name,
    bool requires_target,
    bool can_initiate_combat) {

    std::shared_ptr<Actor> target;

    // Handle targeting
    if (ctx.actor->position() == Position::Fighting) {
        // Already fighting - get current opponent or specified target
        if (ctx.arg_count() > 0) {
            target = ctx.find_actor_target(ctx.arg(0));
            if (!target) {
                ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(0)));
                return CommandResult::InvalidTarget;
            }
        } else {
            target = CombatManager::get_opponent(*ctx.actor);
        }
    } else {
        // Not fighting
        if (requires_target) {
            if (ctx.arg_count() == 0) {
                ctx.send_error(fmt::format("{} who?",
                    std::string(1, std::toupper(skill_name[0])) + std::string(skill_name.substr(1))));
                return CommandResult::InvalidTarget;
            }

            target = ctx.find_actor_target(ctx.arg(0));
            if (!target) {
                ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(0)));
                return CommandResult::InvalidTarget;
            }

            // Can't target yourself with combat abilities
            if (target == ctx.actor) {
                ctx.send_error(fmt::format("You can't {} yourself!", skill_name));
                return CommandResult::InvalidState;
            }

            // Start combat if allowed
            if (can_initiate_combat) {
                ctx.actor->set_position(Position::Fighting);
                target->set_position(Position::Fighting);
                CombatManager::start_combat(ctx.actor, target);
            }
        }
    }

    if (requires_target && !target) {
        ctx.send_error("You aren't fighting anyone!");
        return CommandResult::InvalidState;
    }

    // Get actual skill proficiency from character
    int skill_level = 50; // Default for non-players or unknown abilities
    auto& cache = AbilityCache::instance();
    const auto* ability = cache.get_ability_by_name(skill_name);
    if (ability) {
        auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
        if (player) {
            skill_level = player->get_proficiency(ability->id);
            if (skill_level == 0) {
                // Player doesn't know this ability
                ctx.send_error(fmt::format("You don't know how to {}.", skill_name));
                return CommandResult::InvalidState;
            }
        }
    }

    // Execute the ability
    auto result = AbilityExecutor::execute(ctx, skill_name, target, skill_level);
    if (!result) {
        ctx.send_error(result.error().message);
        return CommandResult::SystemError;
    }

    // Send messages
    if (!result->attacker_message.empty()) {
        ctx.send(result->attacker_message);
    }
    if (target && !result->target_message.empty()) {
        ctx.send_to_actor(target, result->target_message);
    }
    if (!result->room_message.empty()) {
        ctx.send_to_room(result->room_message, true);
    }

    // Check for death
    if (target && target->stats().hit_points <= 0) {
        CombatManager::end_combat(target);
        target->set_position(Position::Dead);
        ctx.send(fmt::format("You have slain {}!", target->display_name()));
        ctx.send_to_actor(target, fmt::format("{} has slain you!", ctx.actor->display_name()));
        ctx.send_to_room(fmt::format("{} has been slain by {}!",
            target->display_name(), ctx.actor->display_name()), true);
    }

    return CommandResult::Success;
}

} // namespace FieryMUD
