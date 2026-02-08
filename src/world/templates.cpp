#include "templates.hpp"
#include "core/object.hpp"
#include "core/mobile.hpp"
#include <magic_enum/magic_enum.hpp>
#include <sstream>

namespace FieryMUD {

namespace {
    // Helper to join keywords span into a single string
    std::string join_keywords(std::span<const std::string> keywords) {
        if (keywords.empty()) return "";
        std::ostringstream oss;
        for (size_t i = 0; i < keywords.size(); ++i) {
            if (i > 0) oss << " ";
            oss << keywords[i];
        }
        return oss.str();
    }
}

ObjectTemplate::ObjectTemplate(const Object* obj)
    : id_(obj ? obj->id() : INVALID_ENTITY_ID)
    , name_(obj ? std::string(obj->name()) : "")
    , keywords_(obj ? join_keywords(obj->keywords()) : "")
    , description_(obj ? std::string(obj->description()) : "")
    , weight_(obj ? obj->weight() : 0)
    , value_(obj ? obj->value() : 0)
    , level_(obj ? obj->level() : 0)
    , type_(obj ? std::string(magic_enum::enum_name(obj->type())) : "")
{
}

MobileTemplate::MobileTemplate(const Mobile* mob)
    : id_(mob ? mob->id() : INVALID_ENTITY_ID)
    , name_(mob ? std::string(mob->name()) : "")
    , keywords_(mob ? join_keywords(mob->keywords()) : "")
    , description_(mob ? std::string(mob->description()) : "")
    , level_(mob ? mob->stats().level : 1)
    , alignment_(mob ? mob->stats().alignment : 0)
    , max_hp_(mob ? mob->stats().max_hit_points : 10)
    , experience_(mob ? mob->stats().experience : 0)
    , gold_(mob ? mob->stats().gold : 0)
{
}

} // namespace FieryMUD
