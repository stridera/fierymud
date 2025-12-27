#include "database/trigger_queries.hpp"
#include "database/generated/db_tables.hpp"
#include "database/generated/db_enums.hpp"
#include "core/logging.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace TriggerQueries {

namespace {

// Helper to parse PostgreSQL array format: {elem1,elem2,elem3}
std::vector<std::string> parse_pg_array(const std::string& pg_array) {
    std::vector<std::string> result;

    if (pg_array.empty() || pg_array == "{}") {
        return result;
    }

    std::string content = pg_array;
    if (content.front() == '{' && content.back() == '}') {
        content = content.substr(1, content.length() - 2);
    }

    if (content.empty()) {
        return result;
    }

    std::string current;
    bool in_quotes = false;
    bool escape_next = false;

    for (char c : content) {
        if (escape_next) {
            current += c;
            escape_next = false;
            continue;
        }

        if (c == '\\') {
            escape_next = true;
            continue;
        }

        if (c == '"') {
            in_quotes = !in_quotes;
            continue;
        }

        if (c == ',' && !in_quotes) {
            if (!current.empty()) {
                result.push_back(current);
                current.clear();
            }
            continue;
        }

        current += c;
    }

    if (!current.empty()) {
        result.push_back(current);
    }

    return result;
}

// Convert db::TriggerFlag to scripting TriggerFlag
FieryMUD::TriggerFlag convert_flag(db::TriggerFlag db_flag) {
    switch (db_flag) {
        case db::TriggerFlag::Global:    return FieryMUD::TriggerFlag::GLOBAL;
        case db::TriggerFlag::Random:    return FieryMUD::TriggerFlag::RANDOM;
        case db::TriggerFlag::Command:   return FieryMUD::TriggerFlag::COMMAND;
        case db::TriggerFlag::Speech:    return FieryMUD::TriggerFlag::SPEECH;
        case db::TriggerFlag::Act:       return FieryMUD::TriggerFlag::ACT;
        case db::TriggerFlag::Death:     return FieryMUD::TriggerFlag::DEATH;
        case db::TriggerFlag::Greet:     return FieryMUD::TriggerFlag::GREET;
        case db::TriggerFlag::GreetAll:  return FieryMUD::TriggerFlag::GREET_ALL;
        case db::TriggerFlag::Entry:     return FieryMUD::TriggerFlag::ENTRY;
        case db::TriggerFlag::Receive:   return FieryMUD::TriggerFlag::RECEIVE;
        case db::TriggerFlag::Fight:     return FieryMUD::TriggerFlag::FIGHT;
        case db::TriggerFlag::HitPercent:return FieryMUD::TriggerFlag::HIT_PERCENT;
        case db::TriggerFlag::Bribe:     return FieryMUD::TriggerFlag::BRIBE;
        case db::TriggerFlag::Load:      return FieryMUD::TriggerFlag::LOAD;
        case db::TriggerFlag::Memory:    return FieryMUD::TriggerFlag::MEMORY;
        case db::TriggerFlag::Cast:      return FieryMUD::TriggerFlag::CAST;
        case db::TriggerFlag::Leave:     return FieryMUD::TriggerFlag::LEAVE;
        case db::TriggerFlag::Door:      return FieryMUD::TriggerFlag::DOOR;
        case db::TriggerFlag::Time:      return FieryMUD::TriggerFlag::TIME;
        case db::TriggerFlag::Auto:      return FieryMUD::TriggerFlag::AUTO;
        case db::TriggerFlag::SpeechTo:  return FieryMUD::TriggerFlag::SPEECH_TO;
        case db::TriggerFlag::Look:      return FieryMUD::TriggerFlag::LOOK;
    }
    return FieryMUD::TriggerFlag::GLOBAL; // Default fallback
}

// Convert string flags array to combined TriggerFlag bitfield
FieryMUD::TriggerFlag parse_trigger_flags(const std::vector<std::string>& flag_strings) {
    FieryMUD::TriggerFlag result{};
    for (const auto& str : flag_strings) {
        if (auto db_flag = db::trigger_flag_from_db(str)) {
            result |= convert_flag(*db_flag);
        } else {
            spdlog::warn("Unknown trigger flag: {}", str);
        }
    }
    return result;
}

// Convert attach_type string to ScriptType enum
FieryMUD::ScriptType parse_script_type(const std::string& type_str) {
    if (type_str == "MOB") return FieryMUD::ScriptType::MOB;
    if (type_str == "OBJECT") return FieryMUD::ScriptType::OBJECT;
    if (type_str == "WORLD") return FieryMUD::ScriptType::WORLD;

    spdlog::warn("Unknown script type: {}, defaulting to MOB", type_str);
    return FieryMUD::ScriptType::MOB;
}

// Parse a database row into TriggerData
FieryMUD::TriggerDataPtr parse_trigger_row(const pqxx::row& row) {
    auto trigger = std::make_shared<FieryMUD::TriggerData>();

    trigger->id = row[db::Triggers::ID.data()].as<int>();
    trigger->name = row[db::Triggers::NAME.data()].as<std::string>();
    trigger->attach_type = parse_script_type(row[db::Triggers::ATTACH_TYPE.data()].as<std::string>());

    // Parse flags array
    std::string flags_str = row[db::Triggers::FLAGS.data()].as<std::string>("");
    auto flag_strings = parse_pg_array(flags_str);
    trigger->flags = parse_trigger_flags(flag_strings);

    // Script code
    trigger->commands = row[db::Triggers::COMMANDS.data()].as<std::string>("");

    // Arguments
    trigger->num_args = row[db::Triggers::NUM_ARGS.data()].as<int>(0);
    std::string arg_list_str = row[db::Triggers::ARG_LIST.data()].as<std::string>("");
    trigger->arg_list = parse_pg_array(arg_list_str);

    // Variables (JSON field)
    std::string variables_str = row[db::Triggers::VARIABLES.data()].as<std::string>("{}");
    try {
        trigger->variables = nlohmann::json::parse(variables_str);
    } catch (const nlohmann::json::exception& e) {
        spdlog::warn("Failed to parse trigger {} variables: {}", trigger->name, e.what());
        trigger->variables = nlohmann::json::object();
    }

    // Attachment references
    if (!row[db::Triggers::ZONE_ID.data()].is_null()) {
        trigger->zone_id = row[db::Triggers::ZONE_ID.data()].as<int>();
    }

    if (!row[db::Triggers::MOB_ZONE_ID.data()].is_null() && !row[db::Triggers::MOB_ID.data()].is_null()) {
        trigger->mob_id = EntityId(
            static_cast<std::uint32_t>(row[db::Triggers::MOB_ZONE_ID.data()].as<int>()),
            static_cast<std::uint32_t>(row[db::Triggers::MOB_ID.data()].as<int>())
        );
    }

    if (!row[db::Triggers::OBJECT_ZONE_ID.data()].is_null() && !row[db::Triggers::OBJECT_ID.data()].is_null()) {
        trigger->object_id = EntityId(
            static_cast<std::uint32_t>(row[db::Triggers::OBJECT_ZONE_ID.data()].as<int>()),
            static_cast<std::uint32_t>(row[db::Triggers::OBJECT_ID.data()].as<int>())
        );
    }

    return trigger;
}

// Build the common SELECT query for triggers
std::string build_trigger_select() {
    return fmt::format(
        R"(SELECT "{}", "{}", "{}", "{}", "{}", "{}", "{}", "{}", "{}", "{}", "{}", "{}"
           FROM "{}")",
        db::Triggers::ID,
        db::Triggers::NAME,
        db::Triggers::ATTACH_TYPE,
        db::Triggers::FLAGS,
        db::Triggers::COMMANDS,
        db::Triggers::NUM_ARGS,
        db::Triggers::ARG_LIST,
        db::Triggers::VARIABLES,
        db::Triggers::ZONE_ID,
        db::Triggers::MOB_ZONE_ID,
        db::Triggers::MOB_ID,
        db::Triggers::OBJECT_ZONE_ID,
        db::Triggers::OBJECT_ID,
        db::Triggers::TABLE
    );
}

} // anonymous namespace

Result<std::vector<FieryMUD::TriggerDataPtr>> load_triggers_for_zone(
    pqxx::work& txn, int zone_id)
{
    try {
        std::string query = fmt::format(
            R"({} WHERE "{}" = $1 OR "{}" = $1 OR "{}" = $1)",
            build_trigger_select(),
            db::Triggers::ZONE_ID,
            db::Triggers::MOB_ZONE_ID,
            db::Triggers::OBJECT_ZONE_ID
        );

        auto result = txn.exec_params(query, zone_id);

        std::vector<FieryMUD::TriggerDataPtr> triggers;
        triggers.reserve(result.size());

        for (const auto& row : result) {
            triggers.push_back(parse_trigger_row(row));
        }

        spdlog::debug("Loaded {} triggers for zone {}", triggers.size(), zone_id);
        return triggers;

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error loading triggers for zone {}: {}", zone_id, e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error loading triggers for zone {}: {}", zone_id, e.what())));
    }
}

Result<std::vector<FieryMUD::TriggerDataPtr>> load_mob_triggers(
    pqxx::work& txn, int zone_id, int mob_id)
{
    try {
        std::string query = fmt::format(
            R"({} WHERE "{}" = 'MOB' AND "{}" = $1 AND "{}" = $2)",
            build_trigger_select(),
            db::Triggers::ATTACH_TYPE,
            db::Triggers::MOB_ZONE_ID,
            db::Triggers::MOB_ID
        );

        auto result = txn.exec_params(query, zone_id, mob_id);

        std::vector<FieryMUD::TriggerDataPtr> triggers;
        triggers.reserve(result.size());

        for (const auto& row : result) {
            triggers.push_back(parse_trigger_row(row));
        }

        spdlog::debug("Loaded {} triggers for mob {}:{}", triggers.size(), zone_id, mob_id);
        return triggers;

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error loading mob triggers {}:{}: {}", zone_id, mob_id, e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error loading mob triggers {}:{}: {}", zone_id, mob_id, e.what())));
    }
}

Result<std::vector<FieryMUD::TriggerDataPtr>> load_object_triggers(
    pqxx::work& txn, int zone_id, int object_id)
{
    try {
        std::string query = fmt::format(
            R"({} WHERE "{}" = 'OBJECT' AND "{}" = $1 AND "{}" = $2)",
            build_trigger_select(),
            db::Triggers::ATTACH_TYPE,
            db::Triggers::OBJECT_ZONE_ID,
            db::Triggers::OBJECT_ID
        );

        auto result = txn.exec_params(query, zone_id, object_id);

        std::vector<FieryMUD::TriggerDataPtr> triggers;
        triggers.reserve(result.size());

        for (const auto& row : result) {
            triggers.push_back(parse_trigger_row(row));
        }

        spdlog::debug("Loaded {} triggers for object {}:{}", triggers.size(), zone_id, object_id);
        return triggers;

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error loading object triggers {}:{}: {}", zone_id, object_id, e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error loading object triggers {}:{}: {}", zone_id, object_id, e.what())));
    }
}

Result<std::vector<FieryMUD::TriggerDataPtr>> load_world_triggers(
    pqxx::work& txn, int zone_id)
{
    try {
        std::string query = fmt::format(
            R"({} WHERE "{}" = 'WORLD' AND "{}" = $1)",
            build_trigger_select(),
            db::Triggers::ATTACH_TYPE,
            db::Triggers::ZONE_ID
        );

        auto result = txn.exec_params(query, zone_id);

        std::vector<FieryMUD::TriggerDataPtr> triggers;
        triggers.reserve(result.size());

        for (const auto& row : result) {
            triggers.push_back(parse_trigger_row(row));
        }

        spdlog::debug("Loaded {} world triggers for zone {}", triggers.size(), zone_id);
        return triggers;

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error loading world triggers for zone {}: {}", zone_id, e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error loading world triggers for zone {}: {}", zone_id, e.what())));
    }
}

Result<FieryMUD::TriggerDataPtr> load_trigger_by_id(pqxx::work& txn, int trigger_id)
{
    try {
        std::string query = fmt::format(
            R"({} WHERE "{}" = $1)",
            build_trigger_select(),
            db::Triggers::ID
        );

        auto result = txn.exec_params(query, trigger_id);

        if (result.empty()) {
            return std::unexpected(Errors::NotFound(
                fmt::format("Trigger with ID {}", trigger_id)));
        }

        return parse_trigger_row(result[0]);

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error loading trigger {}: {}", trigger_id, e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error loading trigger {}: {}", trigger_id, e.what())));
    }
}

Result<std::vector<FieryMUD::TriggerDataPtr>> load_all_triggers(pqxx::work& txn)
{
    try {
        std::string query = build_trigger_select();

        auto result = txn.exec(query);

        std::vector<FieryMUD::TriggerDataPtr> triggers;
        triggers.reserve(result.size());

        for (const auto& row : result) {
            triggers.push_back(parse_trigger_row(row));
        }

        spdlog::info("Loaded {} triggers from database", triggers.size());
        return triggers;

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error loading all triggers: {}", e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error loading all triggers: {}", e.what())));
    }
}

} // namespace TriggerQueries
