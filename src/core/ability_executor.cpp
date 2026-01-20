#include "core/ability_executor.hpp"

#include "core/actor.hpp"
#include "core/combat.hpp"
#include "core/logging.hpp"
#include "database/connection_pool.hpp"
#include "database/world_queries.hpp"
#include "scripting/trigger_manager.hpp"
#include "text/text_format.hpp"
#include "world/room.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <random>

#include <fmt/format.h>

namespace FieryMUD {

// =============================================================================
// Casting Time Constants
// =============================================================================

// Combat rounds are 4 seconds (COMBAT_ROUND_MS = 4000 in combat.hpp)
// CASTING_TICK_INTERVAL = 500ms, so 8 ticks = 1 combat round (4 seconds)
// Circle 1 spell: (1 + 1) * 8 = 16 ticks = 8 seconds = 2 combat rounds
constexpr int CAST_SPEED_PER_ROUND = 8;  // Ticks per combat round (500ms each)

// Convert cast_time_rounds (from database) to pulses
inline int rounds_to_pulses(int rounds) {
    return rounds * CAST_SPEED_PER_ROUND;
}

// =============================================================================
// Dice Roll Display Helpers
// =============================================================================

/**
 * Check if an actor wants to see dice roll details.
 * Only players can have this preference enabled.
 */
static bool wants_dice_details(const std::shared_ptr<Actor>& actor) {
    if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
        return player->is_show_dice_rolls();
    }
    return false;
}


// =============================================================================
// Quickcast Skill - Reduces casting time
// =============================================================================

/**
 * Calculate reduced casting time based on Quick Chant skill.
 * Based on legacy SKILL_QUICK_CHANT implementation:
 * - Skill check: roll d100 < proficiency + INT/WIS bonuses
 * - On success: halve casting time
 * - Additional reduction for high-level casters casting low-circle spells
 * - Never reduces below minimum threshold
 *
 * @param base_ticks Original casting time in ticks
 * @param caster The actor casting the spell
 * @param spell_circle The circle of the spell being cast
 * @return Reduced casting time in ticks, and whether quickcast was applied
 */
std::pair<int, bool> calculate_quickcast_reduction(
    int base_ticks,
    const std::shared_ptr<Actor>& caster,
    int spell_circle) {

    auto player = std::dynamic_pointer_cast<Player>(caster);
    if (!player) {
        return {base_ticks, false};  // Only players can use quickcast
    }

    // Look up Quick Chant skill from ability cache
    auto& cache = AbilityCache::instance();
    const auto* quick_chant = cache.get_ability_by_name("quick chant");
    if (!quick_chant) {
        return {base_ticks, false};  // Skill not in database
    }

    // Get player's proficiency in Quick Chant (0-100)
    int quickcast_proficiency = player->get_proficiency(quick_chant->id);
    if (quickcast_proficiency == 0) {
        return {base_ticks, false};  // Player doesn't know the skill
    }

    // Skill check: roll d100 < proficiency + stat bonuses
    // INT and WIS both help with casting speed
    const auto& stats = player->stats();
    int int_bonus = (stats.intelligence - 10) / 2;  // D&D style stat bonus
    int wis_bonus = (stats.wisdom - 10) / 2;
    int skill_check_target = quickcast_proficiency + int_bonus + wis_bonus;

    // Roll 1-100
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(1, 100);
    int roll = dist(rng);

    if (roll > skill_check_target) {
        return {base_ticks, false};  // Skill check failed
    }

    // Skill check passed - halve casting time
    int reduced_ticks = base_ticks / 2;

    // Additional reduction for high-level casters casting low-circle spells
    if (spell_circle > 0) {
        // Get max circle the caster can access
        int max_circle = 0;
        for (int c = 9; c >= 1; --c) {
            if (player->has_spell_slots(c)) {
                max_circle = c;
                break;
            }
        }

        // Bonus reduction based on circle difference
        // Casting circle 1 when you have circle 5 slots = faster
        if (max_circle > spell_circle) {
            int circle_diff = max_circle - spell_circle;
            // Each circle difference reduces by 1 tick (capped at proficiency/25)
            int max_reduction = quickcast_proficiency / 25;
            int circle_reduction = std::min(circle_diff, max_reduction);
            reduced_ticks -= circle_reduction;
        }
    }

    // Enforce minimum: at least 25% of base time, minimum 1 tick
    int min_ticks = std::max(1, base_ticks / 4);
    reduced_ticks = std::max(reduced_ticks, min_ticks);

    Log::game()->info("Quick Chant: {} proficiency, roll {} vs {}, reduced {} -> {} ticks",
                      quickcast_proficiency, roll, skill_check_target, base_ticks, reduced_ticks);

    return {reduced_ticks, true};
}

// =============================================================================
// Message Template Processing
// =============================================================================

/**
 * Format message template using the canonical TextFormat::Message system.
 * Supported placeholders:
 *   {actor}, {actor.name}    - Actor's display name
 *   {target}, {target.name}  - Target's display name
 *   {actor.pronoun.*}        - Actor's pronouns (subjective, objective, possessive, reflexive)
 *   {actor.he}, {actor.him}  - Pronoun shortcuts
 *   {target.pronoun.*}       - Target's pronouns
 *   {damage}                 - Damage value
 *   {healing}                - Healing value (alias for damage in healing context)
 *
 * Example: "You strike {target.name} for {damage} damage!"
 */
static std::string format_ability_message(
    std::string_view template_str,
    const std::shared_ptr<Actor>& actor,
    const std::shared_ptr<Actor>& target,
    int damage = 0) {

    if (template_str.empty()) return "";

    return TextFormat::Message()
        .actor(actor.get())
        .target(target.get())
        .set("damage", damage)
        .set("healing", damage)
        .format(template_str);
}

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

            // Build name index (lowercase) - index by both plain_name and display name
            std::string lower_plain_name = ability.plain_name;
            std::transform(lower_plain_name.begin(), lower_plain_name.end(), lower_plain_name.begin(),
                          [](unsigned char c) { return std::tolower(c); });
            ability_name_index_[lower_plain_name] = id;

            // Also index by display name for lookups from effect.source
            std::string lower_display_name = ability.name;
            std::transform(lower_display_name.begin(), lower_display_name.end(), lower_display_name.begin(),
                          [](unsigned char c) { return std::tolower(c); });
            ability_name_index_[lower_display_name] = id;

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

        // Load ability messages
        auto messages_result = WorldQueries::load_all_ability_messages(txn);
        if (!messages_result) {
            Log::warn("Failed to load ability messages: {}", messages_result.error().message);
            // Non-fatal, continue without messages
        } else {
            ability_messages_.clear();
            for (const auto& msg_data : *messages_result) {
                CachedAbilityMessages msg;
                msg.start_to_caster = msg_data.start_to_caster;
                msg.start_to_victim = msg_data.start_to_victim;
                msg.start_to_room = msg_data.start_to_room;
                msg.success_to_caster = msg_data.success_to_caster;
                msg.success_to_victim = msg_data.success_to_victim;
                msg.success_to_room = msg_data.success_to_room;
                msg.success_to_self = msg_data.success_to_self;
                msg.success_self_room = msg_data.success_self_room;
                msg.fail_to_caster = msg_data.fail_to_caster;
                msg.fail_to_victim = msg_data.fail_to_victim;
                msg.fail_to_room = msg_data.fail_to_room;
                msg.wearoff_to_target = msg_data.wearoff_to_target;
                msg.wearoff_to_room = msg_data.wearoff_to_room;
                msg.look_message = msg_data.look_message;
                ability_messages_[msg_data.ability_id] = std::move(msg);
            }
        }

        // Load ability restrictions
        auto restrictions_result = WorldQueries::load_all_ability_restrictions(txn);
        if (!restrictions_result) {
            Log::warn("Failed to load ability restrictions: {}", restrictions_result.error().message);
            // Non-fatal, continue without restrictions
        } else {
            ability_restrictions_.clear();
            for (const auto& restr_data : *restrictions_result) {
                CachedAbilityRestrictions restr;
                restr.requirements = WorldQueries::parse_ability_requirements(
                    restr_data.requirements_json, restr_data.ability_id);
                restr.custom_lua = restr_data.custom_requirement_lua;
                ability_restrictions_[restr_data.ability_id] = std::move(restr);
            }
        }

        // Load damage components
        auto damage_result = WorldQueries::load_all_ability_damage_components(txn);
        if (!damage_result) {
            Log::warn("Failed to load ability damage components: {}", damage_result.error().message);
            // Non-fatal, continue without damage components
        } else {
            damage_components_.clear();
            for (const auto& comp_data : *damage_result) {
                CachedDamageComponent comp;
                comp.element = comp_data.element;
                comp.damage_formula = comp_data.damage_formula;
                comp.percentage = comp_data.percentage;
                comp.sequence = comp_data.sequence;
                damage_components_[comp_data.ability_id].push_back(std::move(comp));
            }
        }

        // Load class abilities (which classes can learn each ability)
        auto classes_result = WorldQueries::load_all_ability_classes(txn);
        if (!classes_result) {
            Log::warn("Failed to load ability class data: {}", classes_result.error().message);
            // Non-fatal, continue without class data
        } else {
            ability_classes_.clear();
            for (const auto& class_data : *classes_result) {
                CachedAbilityClass cls;
                cls.class_id = class_data.class_id;
                cls.class_name = class_data.class_name;
                cls.circle = class_data.circle;
                ability_classes_[class_data.ability_id].push_back(std::move(cls));
            }
        }

        return {};
    });

    if (!result) {
        logger->error("Failed to load ability cache: {}", result.error().message);
        return std::unexpected(result.error());
    }

    initialized_ = true;
    logger->info("Ability cache loaded: {} abilities, {} effect types, {} ability-effects, {} messages, {} restrictions",
                abilities_.size(), effects_.size(), ability_effects_.size(),
                ability_messages_.size(), ability_restrictions_.size());
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

const CachedAbilityMessages* AbilityCache::get_ability_messages(int ability_id) const {
    auto it = ability_messages_.find(ability_id);
    return it != ability_messages_.end() ? &it->second : nullptr;
}

const CachedAbilityRestrictions* AbilityCache::get_ability_restrictions(int ability_id) const {
    auto it = ability_restrictions_.find(ability_id);
    return it != ability_restrictions_.end() ? &it->second : nullptr;
}

std::vector<CachedDamageComponent> AbilityCache::get_damage_components(int ability_id) const {
    auto it = damage_components_.find(ability_id);
    return it != damage_components_.end() ? it->second : std::vector<CachedDamageComponent>{};
}

std::vector<CachedAbilityClass> AbilityCache::get_ability_classes(int ability_id) const {
    auto it = ability_classes_.find(ability_id);
    return it != ability_classes_.end() ? it->second : std::vector<CachedAbilityClass>{};
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
        Log::game()->debug("Ability '{}' not found in cache", ability_name);
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

    // Determine spell circle for damage calculation
    // This is used by the formula context for base_damage
    int spell_circle = 1; // Default for non-spell abilities
    if (ability->type == WorldQueries::AbilityType::Spell) {
        auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
        if (player) {
            const auto* learned = player->get_ability(ability->id);
            if (learned && learned->circle > 0) {
                spell_circle = learned->circle;
            }
        }
    }

    // Handle toggle abilities
    if (ability->is_toggle) {
        // Check if this toggle is already active
        if (ctx.actor->has_effect(ability->plain_name)) {
            // Toggle off - remove the effect
            ctx.actor->remove_effect(ability->plain_name);

            AbilityExecutionResult result;
            result.success = true;

            // Check for custom wearoff messages
            const auto* custom_msgs = cache.get_ability_messages(ability->id);
            if (custom_msgs && !custom_msgs->wearoff_to_target.empty()) {
                result.attacker_message = format_ability_message(
                    custom_msgs->wearoff_to_target, ctx.actor, nullptr, 0);
                result.room_message = format_ability_message(
                    custom_msgs->wearoff_to_room, ctx.actor, nullptr, 0);
            } else {
                result.attacker_message = fmt::format("You stop {}.", ability->name);
                result.room_message = fmt::format("{} stops {}.",
                    ctx.actor->display_name(), ability->name);
            }

            return result;
        }
        // Otherwise, continue to activate the toggle (apply effects below)
    }

    // Determine targets for this ability
    std::vector<std::shared_ptr<Actor>> targets;

    if (ability->is_area) {
        // AOE ability - target all enemies in the room
        // AOE spells hit everyone regardless of visibility (casting into darkness still works)
        auto room = ctx.actor->current_room();
        if (room) {
            for (const auto& room_actor : room->contents().actors) {
                // Skip self for violent AOE, include self for beneficial AOE
                if (!room_actor || room_actor == ctx.actor) {
                    continue;
                }

                // For violent AOE spells, target all other actors (enemies)
                // For non-violent AOE (like mass heal), target everyone
                // TODO: Add faction/group checking for proper ally detection
                targets.push_back(room_actor);
            }
            // For non-violent AOE, include self
            if (!ability->violent) {
                targets.insert(targets.begin(), ctx.actor);
            }
        }

        if (targets.empty() && ability->violent) {
            return std::unexpected(Errors::InvalidState("There's no one here to target."));
        }

        Log::game()->debug("AOE ability {} targeting {} actors", ability->plain_name, targets.size());
    } else {
        // Single target ability
        if (target) {
            targets.push_back(target);
        } else if (!ability->violent) {
            // Self-targeting for non-violent abilities without explicit target
            targets.push_back(ctx.actor);
        } else {
            // Violent single-target spell with no target
            return std::unexpected(Errors::InvalidState("Your target is no longer here."));
        }
    }

    // Execute effects against all targets
    AbilityExecutionResult result;
    std::vector<std::string> all_attacker_msgs;  // Per-target damage messages
    std::vector<std::string> all_room_msgs;
    std::vector<std::string> all_dice_details;  // Collect dice details from all effects (for single target)
    std::vector<std::shared_ptr<Actor>> damaged_targets;  // Track who was damaged for combat

    // Per-target tracking: (target, damage, dice_details)
    struct TargetResult {
        std::shared_ptr<Actor> target;
        int damage;
        std::string dice_details;
    };
    std::vector<TargetResult> target_results;

    bool any_effect_succeeded = false;

    for (const auto& current_target : targets) {
        // Fire CAST trigger for mobs being targeted by spells
        // This allows mobs to react to or potentially block spells
        auto& trigger_mgr = TriggerManager::instance();
        if (trigger_mgr.is_initialized() && current_target != ctx.actor) {
            auto trigger_result = trigger_mgr.dispatch_cast(
                current_target, ctx.actor, ability->name);
            if (trigger_result == TriggerResult::Halt) {
                // Mob blocked this spell on itself
                Log::game()->debug("Spell {} on {} blocked by CAST trigger",
                    ability->name, current_target->display_name());
                continue;
            }
        }

        // Build effect context for this target
        EffectContext effect_ctx;
        effect_ctx.actor = ctx.actor;
        effect_ctx.target = current_target;
        effect_ctx.skill_level = skill_level;
        effect_ctx.spell_circle = spell_circle;
        effect_ctx.ability_name = ability->name;
        effect_ctx.build_formula_context();

        // Execute effects - spells use OnCast trigger
        auto effects_result = execute_effects(*ability, effect_ctx, EffectTrigger::OnCast);
        if (!effects_result) {
            // Log but continue with other targets
            Log::game()->warn("Effect execution failed for target {}: {}",
                current_target->display_name(), effects_result.error().message);
            continue;
        }

        // Aggregate results for this target
        int target_damage = 0;
        std::string target_dice_details;
        for (const auto& effect_result : *effects_result) {
            result.effect_results.push_back(effect_result);
            if (effect_result.success) {
                any_effect_succeeded = true;
                // Only count damage effects for damage totals
                if (effect_result.type == EffectType::Damage) {
                    target_damage += effect_result.value;
                    result.total_damage += effect_result.value;
                } else if (effect_result.type == EffectType::Heal) {
                    result.total_healing += effect_result.value;
                }
                // Collect dice details from effects (for showdice) - per target
                if (!effect_result.dice_details.empty()) {
                    all_dice_details.push_back(effect_result.dice_details);
                    // Accumulate this target's dice details
                    if (!target_dice_details.empty()) {
                        target_dice_details += " ";
                    }
                    target_dice_details += effect_result.dice_details;
                }
            }
        }

        // Track per-target damage for message generation and combat
        if (target_damage > 0) {
            damaged_targets.push_back(current_target);
            target_results.push_back({current_target, target_damage, target_dice_details});
        }

        // Send damage message to target (for AOE - single target uses result.target_message)
        if (ability->is_area && current_target != ctx.actor && target_damage > 0) {
            std::string target_dmg_msg = fmt::format("{}'s {} hits you for {} damage!",
                ctx.actor->display_name(), ability->name, target_damage);
            // Add this target's specific dice details if they have ShowDiceRolls enabled
            if (wants_dice_details(current_target) && !target_dice_details.empty()) {
                target_dmg_msg += " " + target_dice_details;
            }
            current_target->send_message(target_dmg_msg);
        }
    }

    // Initiate combat with all damaged targets for violent abilities
    if (ability->violent && !damaged_targets.empty()) {
        for (const auto& damaged_target : damaged_targets) {
            if (damaged_target != ctx.actor) {
                // Add combat pair for each damaged target
                // Uses add_combat_pair to allow multiple opponents (for AOE)
                CombatManager::add_combat_pair(ctx.actor, damaged_target);
            }
        }
    }

    result.success = any_effect_succeeded || result.effect_results.empty();

    // Record cooldown after successful execution
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (player) {
        player->record_ability_use(ability->id);
    }

    // Check for custom ability messages from database
    const auto* custom_msgs = cache.get_ability_messages(ability->id);

    if (custom_msgs) {
        // Use custom database messages (processed with template variables)
        if (result.success) {
            if (ability->is_area) {
                // For AOE: First show cast message, then per-target hit messages
                // Generate a "You cast X!" style message first (using success_to_self if available)
                if (!custom_msgs->success_to_self.empty()) {
                    result.attacker_message = format_ability_message(
                        custom_msgs->success_to_self, ctx.actor, ctx.actor, result.total_damage);
                } else {
                    result.attacker_message = fmt::format("You unleash {}!", ability->name);
                }

                // Generate per-target hit messages using the template
                // Each target gets their own dice roll based on their stats
                for (const auto& tr : target_results) {
                    std::string target_hit_msg = format_ability_message(
                        custom_msgs->success_to_caster, ctx.actor, tr.target, tr.damage);
                    target_hit_msg += fmt::format(" ({})", tr.damage);

                    // Add this target's specific dice details if caster wants them
                    if (wants_dice_details(ctx.actor) && !tr.dice_details.empty()) {
                        target_hit_msg += " " + tr.dice_details;
                    }

                    result.attacker_message += "\n" + target_hit_msg;
                }

                // Room message: announce the spell, mention all affected targets
                if (!custom_msgs->success_to_room.empty() && !damaged_targets.empty()) {
                    result.room_message = format_ability_message(
                        custom_msgs->success_to_room, ctx.actor, damaged_targets[0], result.total_damage);
                    if (damaged_targets.size() > 1) {
                        result.room_message += fmt::format(" and {} other{}!",
                            damaged_targets.size() - 1,
                            damaged_targets.size() > 2 ? "s" : "");
                    }
                }
            } else {
                // Check if this is a self-cast (target is the caster, or null for non-violent self-target)
                bool self_cast = (target == ctx.actor) ||
                                 (target == nullptr && !ability->violent);

                // For message processing, use actual target (self if null for non-violent)
                auto msg_target = target ? target : ctx.actor;

                if (self_cast && !custom_msgs->success_to_self.empty()) {
                    // Use self-specific messages when casting on self
                    result.attacker_message = format_ability_message(
                        custom_msgs->success_to_self, ctx.actor, msg_target, result.total_damage);
                    // No target message for self-cast (caster IS the target)
                    result.room_message = format_ability_message(
                        !custom_msgs->success_self_room.empty()
                            ? custom_msgs->success_self_room
                            : custom_msgs->success_to_room,
                        ctx.actor, msg_target, result.total_damage);
                } else {
                    // Normal caster/target messages
                    result.attacker_message = format_ability_message(
                        custom_msgs->success_to_caster, ctx.actor, msg_target, result.total_damage);

                    // Show damage number for violent spells (matching AOE format)
                    if (ability->violent && result.total_damage > 0) {
                        result.attacker_message += fmt::format(" ({})", result.total_damage);
                    }

                    result.target_message = format_ability_message(
                        custom_msgs->success_to_victim, ctx.actor, msg_target, result.total_damage);
                    result.room_message = format_ability_message(
                        custom_msgs->success_to_room, ctx.actor, msg_target, result.total_damage);

                    // Append damage info for single-target spells
                    for (const auto& msg : all_attacker_msgs) {
                        result.attacker_message += " " + msg;
                    }
                }
            }
        } else {
            result.attacker_message = format_ability_message(
                custom_msgs->fail_to_caster, ctx.actor, target, 0);
            result.target_message = format_ability_message(
                custom_msgs->fail_to_victim, ctx.actor, target, 0);
            result.room_message = format_ability_message(
                custom_msgs->fail_to_room, ctx.actor, target, 0);
        }
    } else {
        // Fall back to effect-generated messages
        std::vector<std::string> attacker_msgs, target_msgs, room_msgs;

        for (const auto& effect_result : result.effect_results) {
            if (!effect_result.success) continue;

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
    }

    // Add dice roll details for players who want them
    // This applies to both custom database messages and effect-generated messages
    if (!all_dice_details.empty()) {
        // Build combined dice details string
        std::string combined_dice_details;
        for (const auto& detail : all_dice_details) {
            combined_dice_details += detail;
        }

        // For the attacker
        if (wants_dice_details(ctx.actor)) {
            result.attacker_message += combined_dice_details;
        }
        // For single-target abilities with a target message, add details for the target too
        // (AOE targets already got their messages sent inline above)
        if (target && target != ctx.actor && !ability->is_area && !result.target_message.empty()) {
            if (wants_dice_details(target)) {
                result.target_message += combined_dice_details;
            }
        }
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

    // Check if violent ability requires target (AOE spells don't need explicit targets)
    if (ability.violent && !target && !ability.is_area) {
        return std::unexpected(Errors::InvalidArgument("target",
            fmt::format("{} requires a target", ability.plain_name)));
    }

    // Check combat restrictions
    bool is_fighting = ctx.actor->is_fighting();
    if (ability.in_combat_only && !is_fighting) {
        return std::unexpected(Errors::InvalidState(
            fmt::format("You can only use {} while fighting!", ability.plain_name)));
    }
    if (!ability.combat_ok && is_fighting) {
        return std::unexpected(Errors::InvalidState(
            fmt::format("You can't use {} while fighting!", ability.plain_name)));
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

    // Spell slot validation (FieryMUD uses spell circles, not mana)
    if (ability.type == WorldQueries::AbilityType::Spell) {
        auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
        if (player && !player->is_god()) {
            // Get spell circle from player's learned ability data
            const auto* learned = player->get_ability(ability.id);
            int spell_circle = learned ? learned->circle : 0;

            if (spell_circle > 0) {
                // Check availability - consume_spell_slot handles upcast automatically
                bool has_available_slot = false;
                for (int c = spell_circle; c <= 9; ++c) {
                    if (player->has_spell_slots(c)) {
                        has_available_slot = true;
                        break;
                    }
                }

                if (!has_available_slot) {
                    return std::unexpected(Errors::InvalidState(
                        fmt::format("You don't have any available spell slots for {} (circle {}).",
                                   ability.plain_name, spell_circle)));
                }

                // Consume the slot (will be added to restoration queue)
                if (!player->consume_spell_slot(spell_circle)) {
                    return std::unexpected(Errors::InvalidState(
                        fmt::format("Failed to use spell slot for {} (circle {}).",
                                   ability.plain_name, spell_circle)));
                }
            }
        }
        // Gods and non-players can cast spells without consuming slots
    }

    // Check ability restrictions from database
    auto& cache = AbilityCache::instance();
    const auto* restrictions = cache.get_ability_restrictions(ability.id);
    if (restrictions) {
        for (const auto& req : restrictions->requirements) {
            bool met = false;
            std::string error_msg;

            if (req.type == "hidden") {
                // Must be hidden (sneaking)
                met = ctx.actor->has_flag(ActorFlag::Hide) || ctx.actor->has_flag(ActorFlag::Sneak);
                error_msg = "You must be hidden to use that ability.";
            }
            else if (req.type == "weapon_type") {
                // Check if wielded weapon is of specified type
                auto weapon = ctx.actor->equipment().get_main_weapon();
                if (weapon) {
                    // TODO: Implement proper weapon damage type checking once
                    // weapons have damage_type property (piercing, slashing, etc.)
                    // For now, we just check if a weapon is equipped
                    // and allow the ability if the type matches common patterns
                    std::string req_type = req.value;
                    std::transform(req_type.begin(), req_type.end(), req_type.begin(),
                                  [](unsigned char c) { return std::tolower(c); });

                    // Check by object type for now
                    if (req_type == "ranged" || req_type == "bow" || req_type == "crossbow") {
                        met = weapon->is_weapon() && weapon->type() == ObjectType::Fireweapon;
                    } else if (req_type == "melee") {
                        met = weapon->is_weapon() && weapon->type() == ObjectType::Weapon;
                    } else {
                        // For specific damage types (piercing, slashing, etc.),
                        // assume any weapon works until we implement proper tracking
                        met = weapon->is_weapon();
                    }

                    if (!met) {
                        error_msg = fmt::format("You need a {} weapon for that ability.", req.value);
                    }
                } else {
                    error_msg = "You need a weapon to use that ability.";
                }
            }
            else if (req.type == "position") {
                // Check position requirement
                std::string pos_str = req.value;
                std::transform(pos_str.begin(), pos_str.end(), pos_str.begin(),
                              [](unsigned char c) { return std::tolower(c); });
                auto required_pos = ActorUtils::parse_position(pos_str);
                if (required_pos) {
                    met = (ctx.actor->position() >= *required_pos);
                    error_msg = fmt::format("You must be {} to use that.", req.value);
                } else {
                    met = true; // Unknown position, allow it
                }
            }
            else if (req.type == "has_effect") {
                // Must have a specific effect active
                met = ctx.actor->has_effect(req.value);
                error_msg = fmt::format("You must be affected by {} to use that.", req.value);
            }
            else if (req.type == "in_combat") {
                // Must be in combat
                met = ctx.actor->is_fighting();
                error_msg = "You must be fighting to use that ability.";
            }
            else if (req.type == "not_in_combat") {
                // Must NOT be in combat
                met = !ctx.actor->is_fighting();
                error_msg = "You can't use that while fighting!";
            }
            else {
                // Unknown requirement type - skip (allow by default)
                met = true;
            }

            // Apply negation if specified
            if (req.negated) {
                met = !met;
            }

            if (!met) {
                return std::unexpected(Errors::InvalidState(error_msg));
            }
        }
    }

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
        Log::game()->info("Ability {} (id={}) has no effects defined", ability.plain_name, ability.id);
        return std::vector<EffectResult>{};
    }

    Log::game()->info("Ability {} (id={}) has {} effects, looking for trigger={}",
                       ability.plain_name, ability.id, ability_effects.size(),
                       trigger == EffectTrigger::OnCast ? "on_cast" : "on_hit");

    // Build effect definitions map
    std::unordered_map<int, EffectDefinition> effect_defs;
    for (const auto& ability_effect : ability_effects) {
        const auto* def = cache.get_effect(ability_effect.effect_id);
        if (def) {
            effect_defs[def->id] = *def;
            Log::game()->info("  Effect {} (id={}) type={} trigger={} chance={}%",
                               def->name, def->id,
                               static_cast<int>(def->type),
                               static_cast<int>(ability_effect.trigger),
                               ability_effect.chance_percent);
        } else {
            Log::game()->warn("  Effect definition {} not found in cache!", ability_effect.effect_id);
        }
    }

    // Execute all effects
    return EffectExecutor::execute_ability_effects(
        ability_effects, effect_defs, context, trigger);
}

// =============================================================================
// Casting Time System - Begin and Complete Cast
// =============================================================================

std::expected<void, Error> AbilityExecutor::begin_casting(
    std::shared_ptr<Actor> caster,
    int ability_id,
    std::shared_ptr<Actor> target,
    int spell_circle) {

    auto& cache = AbilityCache::instance();
    const auto* ability = cache.get_ability(ability_id);
    if (!ability) {
        return std::unexpected(Errors::NotFound(
            fmt::format("Ability ID {} not found", ability_id)));
    }

    // Calculate base casting time in pulses
    // Use cast_time_rounds from database if set, otherwise use circle-based default
    int cast_rounds = ability->cast_time_rounds > 0
        ? ability->cast_time_rounds
        : (spell_circle + 1);  // Default: circle 1 = 2 rounds, circle 3 = 4 rounds
    int base_ticks = cast_rounds * CAST_SPEED_PER_ROUND;

    // Apply quickcast reduction
    auto [reduced_ticks, quickcast_applied] = calculate_quickcast_reduction(
        base_ticks, caster, spell_circle);

    // Create casting state
    Actor::CastingState state;
    state.ability_id = ability_id;
    state.ability_name = ability->name;  // Use display name with colors
    state.ticks_remaining = reduced_ticks;
    state.total_ticks = reduced_ticks;
    state.target = target;
    state.target_name = target ? target->name() : "";
    state.circle = spell_circle;
    state.quickcast_applied = quickcast_applied;

    // Start the cast
    caster->start_casting(state);

    // Send start messages (matches legacy format)
    caster->send_message("You start chanting...\n");

    if (target && target != caster) {
        target->send_message(fmt::format("{} begins casting a spell on you!\n",
                                         caster->display_name()));
    }

    // Send room message
    if (auto room = caster->current_room()) {
        std::string room_msg = fmt::format("{} begins casting a spell.\n",
                                           caster->display_name());
        for (const auto& other : room->contents().actors) {
            if (other != caster && other != target) {
                other->send_message(room_msg);
            }
        }
    }

    return {};
}

std::expected<AbilityExecutionResult, Error> AbilityExecutor::execute_completed_cast(
    std::shared_ptr<Actor> caster,
    int ability_id,
    std::shared_ptr<Actor> target) {

    auto& cache = AbilityCache::instance();
    const auto* ability = cache.get_ability(ability_id);
    if (!ability) {
        return std::unexpected(Errors::NotFound(
            fmt::format("Ability ID {} not found", ability_id)));
    }

    // Get skill proficiency
    int skill_level = 50;  // Default
    auto player = std::dynamic_pointer_cast<Player>(caster);
    if (player) {
        if (player->is_god()) {
            skill_level = 100;
        } else {
            skill_level = player->get_proficiency(ability_id);
        }
    }

    // Create a command context for the caster
    auto room = caster->current_room();
    if (!room) {
        return std::unexpected(Errors::InvalidState("Caster is not in a room"));
    }

    // Build a minimal command context
    CommandContext ctx;
    ctx.actor = caster;
    ctx.room = room;
    ctx.command.command = "cast";
    ctx.start_time = std::chrono::steady_clock::now();

    // Execute the ability using existing infrastructure
    return execute_by_id(ctx, ability_id, target, skill_level);
}

// =============================================================================
// Generic Skill Command Handler
// =============================================================================

Result<CommandResult> execute_skill_command(
    const CommandContext& ctx,
    std::string_view skill_name,
    bool requires_target,
    bool can_initiate_combat) {

    // Can't use skills while casting a spell
    if (ctx.actor->is_casting()) {
        auto state_opt = ctx.actor->casting_state();
        if (state_opt.has_value()) {
            ctx.send_error(fmt::format("You can't {} while casting {}!",
                                       skill_name, state_opt->ability_name));
        } else {
            ctx.send_error(fmt::format("You can't {} while casting a spell!", skill_name));
        }
        return CommandResult::Busy;
    }

    // Interrupt concentration-based activities (like meditation) when using abilities
    ctx.actor->interrupt_concentration();

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
            // Gods can use all abilities at full proficiency
            if (player->is_god()) {
                skill_level = 100;
            } else {
                skill_level = player->get_proficiency(ability->id);
                if (skill_level == 0) {
                    // Player doesn't know this ability
                    ctx.send_error(fmt::format("You don't know how to {}.", skill_name));
                    return CommandResult::InvalidState;
                }
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
        // If target received a personalized message, exclude them from room message
        if (target && !result->target_message.empty()) {
            ctx.send_to_room(result->room_message, true, std::array{target});
        } else {
            ctx.send_to_room(result->room_message, true);
        }
    }

    // Check for death
    if (target && target->stats().hit_points <= 0) {
        CombatManager::end_combat(target);
        target->set_position(Position::Dead);
        ctx.send(fmt::format("You have slain {}!", target->display_name()));
        ctx.send_to_actor(target, fmt::format("{} has slain you!", ctx.actor->display_name()));
        // Exclude target from room message since they got their own death message
        ctx.send_to_room(fmt::format("{} has been slain by {}!",
            target->display_name(), ctx.actor->display_name()), true, std::array{target});
    }

    return CommandResult::Success;
}

} // namespace FieryMUD
