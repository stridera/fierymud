#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "core/object.hpp"
#include "triggers/trigger_data.hpp"
#include "world/room.hpp"

class Actor;

namespace FieryMUD {

/**
 * ScriptContext - Provides execution context for trigger scripts
 *
 * Contains references to relevant game objects and contextual data
 * that scripts can access during execution. This is passed to the
 * Lua environment when a trigger fires.
 *
 * Contexts are short-lived and created fresh for each trigger execution.
 */
class ScriptContext {
  public:
    /// The entity that owns the trigger (mob, object, or room for world triggers)
    using OwnerVariant = std::variant<std::shared_ptr<Actor>, std::shared_ptr<Object>, std::shared_ptr<Room>>;

    /// Builder pattern for creating contexts
    class Builder {
      public:
        Builder &set_trigger(TriggerDataPtr trigger) {
            trigger_ = std::move(trigger);
            return *this;
        }

        Builder &set_owner(std::shared_ptr<Actor> owner) {
            owner_ = std::move(owner);
            return *this;
        }

        Builder &set_owner(std::shared_ptr<Object> owner) {
            owner_ = std::move(owner);
            return *this;
        }

        Builder &set_owner(std::shared_ptr<Room> owner) {
            owner_ = std::move(owner);
            return *this;
        }

        Builder &set_actor(std::shared_ptr<Actor> actor) {
            actor_ = std::move(actor);
            return *this;
        }

        Builder &set_target(std::shared_ptr<Actor> target) {
            target_ = std::move(target);
            return *this;
        }

        Builder &set_object(std::shared_ptr<Object> object) {
            object_ = std::move(object);
            return *this;
        }

        Builder &set_room(std::shared_ptr<Room> room) {
            room_ = std::move(room);
            return *this;
        }

        Builder &set_command(std::string_view command) {
            command_ = std::string{command};
            return *this;
        }

        Builder &set_argument(std::string_view argument) {
            argument_ = std::string{argument};
            return *this;
        }

        Builder &set_speech(std::string_view speech) {
            speech_ = std::string{speech};
            return *this;
        }

        Builder &set_direction(Direction dir) {
            direction_ = dir;
            return *this;
        }

        Builder &set_amount(int amount) {
            amount_ = amount;
            return *this;
        }

        [[nodiscard]] ScriptContext build() const;

      private:
        TriggerDataPtr trigger_;
        std::optional<OwnerVariant> owner_;
        std::shared_ptr<Actor> actor_;
        std::shared_ptr<Actor> target_;
        std::shared_ptr<Object> object_;
        std::shared_ptr<Room> room_;
        std::string command_;
        std::string argument_;
        std::string speech_;
        std::optional<Direction> direction_;
        std::optional<int> amount_;
    };

    // Accessors

    /// The trigger being executed
    [[nodiscard]] TriggerDataPtr trigger() const { return trigger_; }

    /// The owner of the trigger (self in scripts)
    [[nodiscard]] const std::optional<OwnerVariant> &owner() const { return owner_; }

    /// The actor that triggered the event (player/mob who did something)
    [[nodiscard]] std::shared_ptr<Actor> actor() const { return actor_; }

    /// Target of an action (optional)
    [[nodiscard]] std::shared_ptr<Actor> target() const { return target_; }

    /// Object involved in the trigger (for RECEIVE, DROP, etc.)
    [[nodiscard]] std::shared_ptr<Object> object() const { return object_; }

    /// Room context (where the trigger fired)
    [[nodiscard]] std::shared_ptr<Room> room() const { return room_; }

    /// Command that was typed (for COMMAND triggers)
    [[nodiscard]] std::string_view command() const { return command_; }

    /// Arguments to the command
    [[nodiscard]] std::string_view argument() const { return argument_; }

    /// Speech text (for SPEECH triggers)
    [[nodiscard]] std::string_view speech() const { return speech_; }

    /// Direction (for LEAVE, ENTRY triggers)
    [[nodiscard]] std::optional<Direction> direction() const { return direction_; }

    /// Numeric amount (for BRIBE, HIT_PERCENT)
    [[nodiscard]] std::optional<int> amount() const { return amount_; }

    // Helper methods

    /// Get owner as Actor (if applicable)
    [[nodiscard]] std::shared_ptr<Actor> owner_as_actor() const {
        if (owner_ && std::holds_alternative<std::shared_ptr<Actor>>(*owner_)) {
            return std::get<std::shared_ptr<Actor>>(*owner_);
        }
        return nullptr;
    }

    /// Get owner as Object (if applicable)
    [[nodiscard]] std::shared_ptr<Object> owner_as_object() const {
        if (owner_ && std::holds_alternative<std::shared_ptr<Object>>(*owner_)) {
            return std::get<std::shared_ptr<Object>>(*owner_);
        }
        return nullptr;
    }

    /// Get owner as Room (if applicable)
    [[nodiscard]] std::shared_ptr<Room> owner_as_room() const {
        if (owner_ && std::holds_alternative<std::shared_ptr<Room>>(*owner_)) {
            return std::get<std::shared_ptr<Room>>(*owner_);
        }
        return nullptr;
    }

  private:
    friend class Builder;

    TriggerDataPtr trigger_;
    std::optional<OwnerVariant> owner_;
    std::shared_ptr<Actor> actor_;
    std::shared_ptr<Actor> target_;
    std::shared_ptr<Object> object_;
    std::shared_ptr<Room> room_;
    std::string command_;
    std::string argument_;
    std::string speech_;
    std::optional<Direction> direction_;
    std::optional<int> amount_;
};

// Builder implementation
inline ScriptContext ScriptContext::Builder::build() const {
    ScriptContext ctx;
    ctx.trigger_ = trigger_;
    ctx.owner_ = owner_;
    ctx.actor_ = actor_;
    ctx.target_ = target_;
    ctx.object_ = object_;
    ctx.room_ = room_;
    ctx.command_ = command_;
    ctx.argument_ = argument_;
    ctx.speech_ = speech_;
    ctx.direction_ = direction_;
    ctx.amount_ = amount_;
    return ctx;
}

} // namespace FieryMUD
