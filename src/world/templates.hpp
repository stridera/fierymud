#pragma once

#include "../core/ids.hpp"

#include <string>
#include <string_view>

// Forward declarations
class Object;
class Mobile;

namespace FieryMUD {

/**
 * ObjectTemplate provides read-only access to object prototype data.
 *
 * Used by scripts to inspect object properties without modifying them.
 * Actual object instances are created via room:spawn_object().
 */
class ObjectTemplate {
  public:
    ObjectTemplate(EntityId id, std::string_view name) : id_(id), name_(name) {}

    // Construct from an Object prototype
    explicit ObjectTemplate(const Object *obj);

    EntityId id() const { return id_; }
    std::string_view name() const { return name_; }
    std::string_view keywords() const { return keywords_; }
    std::string_view description() const { return description_; }
    int weight() const { return weight_; }
    int value() const { return value_; }
    int level() const { return level_; }
    std::string_view type() const { return type_; }

    // Setters for initialization
    void set_keywords(std::string_view kw) { keywords_ = std::string(kw); }
    void set_description(std::string_view desc) { description_ = std::string(desc); }
    void set_weight(int w) { weight_ = w; }
    void set_value(int v) { value_ = v; }
    void set_level(int l) { level_ = l; }
    void set_type(std::string_view t) { type_ = std::string(t); }

  private:
    EntityId id_;
    std::string name_;
    std::string keywords_;
    std::string description_;
    int weight_ = 0;
    int value_ = 0;
    int level_ = 0;
    std::string type_;
};

/**
 * MobileTemplate provides read-only access to mobile prototype data.
 *
 * Used by scripts to inspect mobile properties without modifying them.
 * Actual mobile instances are spawned via room:spawn_mobile().
 */
class MobileTemplate {
  public:
    MobileTemplate(EntityId id, std::string_view name) : id_(id), name_(name) {}

    // Construct from a Mobile prototype
    explicit MobileTemplate(const Mobile *mob);

    EntityId id() const { return id_; }
    std::string_view name() const { return name_; }
    std::string_view keywords() const { return keywords_; }
    std::string_view description() const { return description_; }
    int level() const { return level_; }
    int alignment() const { return alignment_; }
    int max_hp() const { return max_hp_; }
    int experience() const { return experience_; }
    int gold() const { return gold_; }

    // Setters for initialization
    void set_keywords(std::string_view kw) { keywords_ = std::string(kw); }
    void set_description(std::string_view desc) { description_ = std::string(desc); }
    void set_level(int l) { level_ = l; }
    void set_alignment(int a) { alignment_ = a; }
    void set_max_hp(int hp) { max_hp_ = hp; }
    void set_experience(int exp) { experience_ = exp; }
    void set_gold(int g) { gold_ = g; }

  private:
    EntityId id_;
    std::string name_;
    std::string keywords_;
    std::string description_;
    int level_ = 1;
    int alignment_ = 0;
    int max_hp_ = 10;
    int experience_ = 0;
    int gold_ = 0;
};

} // namespace FieryMUD
