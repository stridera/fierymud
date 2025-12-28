#include "core/ability_executor.hpp"

#include "core/actor.hpp"
#include "core/combat.hpp"
#include "core/logging.hpp"
#include "database/connection_pool.hpp"
#include "database/world_queries.hpp"
#include "scripting/trigger_manager.hpp"
#include "world/room.hpp"

#include <algorithm>
#include <cctype>

#include <fmt/format.h>

namespace FieryMUD {

// =============================================================================
// Message Template Processing
// =============================================================================

/**
 * Format message template with named arguments using fmt::format style.
 * Supported placeholders:
 *   {actor}  - actor's display name
 *   {target} - target's display name
 *   {damage} - damage value
 *   {healing} - healing value (alias for damage in healing context)
 *
 * Example: "You strike {target} for {damage} damage!"
 */
static std::string process_message_template(
    std::string_view template_str,
    const std::shared_ptr<Actor>& actor,
    const std::shared_ptr<Actor>& target,
    int damage = 0) {

    if (template_str.empty()) return "";

    try {
        return fmt::format(
            fmt::runtime(template_str),
            fmt::arg("actor", actor ? actor->display_name() : "someone"),
            fmt::arg("target", target ? target->display_name() : "someone"),
            fmt::arg("damage", damage),
            fmt::arg("healing", damage)
        );
    } catch (const fmt::format_error& e) {
        // If template is malformed, return it as-is with a warning
        Log::warn("Invalid message template '{}': {}", template_str, e.what());
        return std::string(template_str);
    }
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
                result.attacker_message = process_message_template(
                    custom_msgs->wearoff_to_target, ctx.actor, nullptr, 0);
                result.room_message = process_message_template(
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
        auto room = ctx.actor->current_room();
        if (room) {
            for (const auto& room_actor : room->contents().actors) {
                // Skip self, skip non-visible, skip allies (for violent AOE)
                if (room_actor && room_actor != ctx.actor &&
                    room_actor->is_visible_to(*ctx.actor)) {
                    // For violent AOE spells, only target enemies
                    // For now, we consider all other actors as valid targets
                    // TODO: Add faction/group checking for proper ally detection
                    if (ability->violent) {
                        // Skip players for NPC-cast violent spells, skip NPCs if in same group, etc.
                        // For simplicity, just target all visible actors that aren't the caster
                        targets.push_back(room_actor);
                    } else {
                        // Non-violent AOE (like mass heal) - target everyone including self
                        targets.push_back(room_actor);
                    }
                }
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
        }
    }

    // Execute effects against all targets
    AbilityExecutionResult result;
    std::vector<std::string> all_attacker_msgs;
    std::vector<std::string> all_room_msgs;
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

        // Aggregate results
        int target_damage = 0;
        for (const auto& effect_result : *effects_result) {
            result.effect_results.push_back(effect_result);
            if (effect_result.success) {
                any_effect_succeeded = true;
                target_damage += effect_result.value;
                result.total_damage += effect_result.value;
                if (effect_result.value < 0) {
                    result.total_healing += -effect_result.value;
                }
            }
        }

        // Build per-target messages
        if (target_damage > 0) {
            all_attacker_msgs.push_back(fmt::format("{} takes {} damage!",
                current_target->display_name(), target_damage));
        }

        // Send damage message to target
        if (current_target != ctx.actor && target_damage > 0) {
            current_target->send_message(fmt::format("{} hits you for {} damage!",
                ctx.actor->display_name(), target_damage));
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
                // For AOE, use a generic message about the area effect
                result.attacker_message = process_message_template(
                    custom_msgs->success_to_caster, ctx.actor, nullptr, result.total_damage);
                result.room_message = process_message_template(
                    custom_msgs->success_to_room, ctx.actor, nullptr, result.total_damage);
                // Append individual damage if any
                for (const auto& msg : all_attacker_msgs) {
                    result.attacker_message += " " + msg;
                }
            } else {
                // Check if this is a self-cast (target is the caster, or null for non-violent self-target)
                bool self_cast = (target == ctx.actor) ||
                                 (target == nullptr && !ability->violent);

                // For message processing, use actual target (self if null for non-violent)
                auto msg_target = target ? target : ctx.actor;

                if (self_cast && !custom_msgs->success_to_self.empty()) {
                    // Use self-specific messages when casting on self
                    result.attacker_message = process_message_template(
                        custom_msgs->success_to_self, ctx.actor, msg_target, result.total_damage);
                    // No target message for self-cast (caster IS the target)
                    result.room_message = process_message_template(
                        !custom_msgs->success_self_room.empty()
                            ? custom_msgs->success_self_room
                            : custom_msgs->success_to_room,
                        ctx.actor, msg_target, result.total_damage);
                } else {
                    // Normal caster/target messages
                    result.attacker_message = process_message_template(
                        custom_msgs->success_to_caster, ctx.actor, msg_target, result.total_damage);
                    result.target_message = process_message_template(
                        custom_msgs->success_to_victim, ctx.actor, msg_target, result.total_damage);
                    result.room_message = process_message_template(
                        custom_msgs->success_to_room, ctx.actor, msg_target, result.total_damage);
                }
            }
        } else {
            result.attacker_message = process_message_template(
                custom_msgs->fail_to_caster, ctx.actor, target, 0);
            result.target_message = process_message_template(
                custom_msgs->fail_to_victim, ctx.actor, target, 0);
            result.room_message = process_message_template(
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

    // Note: FieryMUD uses spell circles (memorization), not mana costs
    // TODO: Add spell circle validation when spell memorization system is implemented

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
