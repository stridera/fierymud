#include "database/trigger_queries.hpp"
#include "database/generated/db_tables.hpp"
#include "database/generated/db_enums.hpp"
#include "database/db_parsing_utils.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

using DbParsingUtils::parse_pg_array;

namespace TriggerQueries {

namespace {

// Convert db::TriggerFlag to scripting TriggerFlag
FieryMUD::TriggerFlag convert_flag(db::TriggerFlag db_flag) {
    switch (db_flag) {
        // Common flags
        case db::TriggerFlag::Global:    return FieryMUD::TriggerFlag::GLOBAL;
        case db::TriggerFlag::Random:    return FieryMUD::TriggerFlag::RANDOM;
        case db::TriggerFlag::Command:   return FieryMUD::TriggerFlag::COMMAND;
        case db::TriggerFlag::Load:      return FieryMUD::TriggerFlag::LOAD;
        case db::TriggerFlag::Cast:      return FieryMUD::TriggerFlag::CAST;
        case db::TriggerFlag::Leave:     return FieryMUD::TriggerFlag::LEAVE;
        case db::TriggerFlag::Time:      return FieryMUD::TriggerFlag::TIME;

        // MOB-specific flags
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
        case db::TriggerFlag::Memory:    return FieryMUD::TriggerFlag::MEMORY;
        case db::TriggerFlag::Door:      return FieryMUD::TriggerFlag::DOOR;
        case db::TriggerFlag::SpeechTo:  return FieryMUD::TriggerFlag::SPEECH_TO;
        case db::TriggerFlag::Look:      return FieryMUD::TriggerFlag::LOOK;
        case db::TriggerFlag::Auto:      return FieryMUD::TriggerFlag::AUTO;

        // OBJECT-specific flags
        case db::TriggerFlag::Attack:    return FieryMUD::TriggerFlag::ATTACK;
        case db::TriggerFlag::Defend:    return FieryMUD::TriggerFlag::DEFEND;
        case db::TriggerFlag::Timer:     return FieryMUD::TriggerFlag::TIMER;
        case db::TriggerFlag::Get:       return FieryMUD::TriggerFlag::GET;
        case db::TriggerFlag::Drop:      return FieryMUD::TriggerFlag::DROP;
        case db::TriggerFlag::Give:      return FieryMUD::TriggerFlag::GIVE;
        case db::TriggerFlag::Wear:      return FieryMUD::TriggerFlag::WEAR;
        case db::TriggerFlag::Remove:    return FieryMUD::TriggerFlag::REMOVE;
        case db::TriggerFlag::Use:       return FieryMUD::TriggerFlag::USE;
        case db::TriggerFlag::Consume:   return FieryMUD::TriggerFlag::CONSUME;

        // WORLD-specific flags
        case db::TriggerFlag::Reset:     return FieryMUD::TriggerFlag::RESET;
        case db::TriggerFlag::Preentry:  return FieryMUD::TriggerFlag::PREENTRY;
        case db::TriggerFlag::Postentry: return FieryMUD::TriggerFlag::POSTENTRY;
    }
    return FieryMUD::TriggerFlag::GLOBAL; // Default fallback
}

// Convert string flags array to combined TriggerFlag bitfield
FieryMUD::TriggerFlag parse_trigger_flags(const std::vector<std::string>& flag_strings) {
    FieryMUD::TriggerFlag result{};
    for (const auto& str : flag_strings) {
        if (auto db_flag = db::trigger_flag_from_db(str)) {
            auto converted = convert_flag(*db_flag);
            result |= converted;
            spdlog::debug("Parsed trigger flag '{}' -> db:{} -> fiery:{:#x}",
                str, static_cast<int>(*db_flag), static_cast<std::uint32_t>(converted));
        } else {
            spdlog::warn("Unknown trigger flag: {}", str);
        }
    }
    spdlog::debug("Final trigger flags: {:#x}", static_cast<std::uint32_t>(result));
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

// Column indices for trigger SELECT query (must match build_trigger_select order)
// NOTE: Triggers table does NOT have mob_zone_id/mob_id/object_zone_id/object_id columns
// Entity associations are handled via junction tables (MobTriggers, ObjectTriggers)
namespace TriggerColumn {
    constexpr int ID = 0;
    constexpr int NAME = 1;
    constexpr int ATTACH_TYPE = 2;
    constexpr int FLAGS = 3;
    constexpr int COMMANDS = 4;
    constexpr int NUM_ARGS = 5;
    constexpr int ARG_LIST = 6;
    constexpr int VARIABLES = 7;
    constexpr int ZONE_ID = 8;
}

// Parse a database row into TriggerData (base columns only)
// NOTE: Entity associations (mob_id, object_id) are set separately from junction tables
FieryMUD::TriggerDataPtr parse_trigger_row(const pqxx::row& row) {
    auto trigger = std::make_shared<FieryMUD::TriggerData>();

    trigger->id = row[TriggerColumn::ID].as<int>();
    trigger->name = row[TriggerColumn::NAME].as<std::string>();
    trigger->attach_type = parse_script_type(row[TriggerColumn::ATTACH_TYPE].as<std::string>());

    // Parse flags array
    std::string flags_str = row[TriggerColumn::FLAGS].as<std::string>("");
    auto flag_strings = parse_pg_array(flags_str);
    trigger->flags = parse_trigger_flags(flag_strings);

    // Script code
    trigger->commands = row[TriggerColumn::COMMANDS].as<std::string>("");

    // Arguments
    trigger->num_args = row[TriggerColumn::NUM_ARGS].as<int>(0);
    std::string arg_list_str = row[TriggerColumn::ARG_LIST].as<std::string>("");
    trigger->arg_list = parse_pg_array(arg_list_str);

    // Variables (JSON field)
    std::string variables_str = row[TriggerColumn::VARIABLES].as<std::string>("{}");
    try {
        trigger->variables = nlohmann::json::parse(variables_str);
    } catch (const nlohmann::json::exception& e) {
        spdlog::warn("Failed to parse trigger {} variables: {}", trigger->name, e.what());
        trigger->variables = nlohmann::json::object();
    }

    // Zone reference (for WORLD triggers)
    if (!row[TriggerColumn::ZONE_ID].is_null()) {
        trigger->zone_id = row[TriggerColumn::ZONE_ID].as<int>();
    }

    return trigger;
}

// Build the common SELECT query for triggers (base columns only)
std::string build_trigger_select() {
    return fmt::format(
        R"(SELECT "{}", "{}", "{}", "{}", "{}", "{}", "{}", "{}", "{}"
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
        db::Triggers::TABLE
    );
}

} // anonymous namespace

Result<std::vector<FieryMUD::TriggerDataPtr>> load_triggers_for_zone(
    pqxx::work& txn, int zone_id)
{
    try {
        // Use UNION to get triggers from three sources:
        // 1. WORLD triggers via zone_id on Triggers table
        // 2. MOB triggers via MobTriggers junction table
        // 3. OBJECT triggers via ObjectTriggers junction table
        //
        // The last two columns (jt_zone_id, jt_id) contain the entity IDs
        // from junction tables, which we use to set mob_id/object_id.
        std::string query = fmt::format(
            R"(
            -- WORLD triggers for this zone
            SELECT t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}",
                   $1::int as jt_zone_id, 0::int as jt_id
            FROM "{}" t
            WHERE t."{}" = $1 AND t."{}" = 'WORLD'

            UNION ALL

            -- MOB triggers for mobs in this zone (via junction table)
            SELECT t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}",
                   mt."{}" as jt_zone_id, mt."{}" as jt_id
            FROM "{}" t
            INNER JOIN "{}" mt ON t."{}" = mt."{}" AND t."{}" = mt."{}"
            WHERE mt."{}" = $1

            UNION ALL

            -- OBJECT triggers for objects in this zone (via junction table)
            SELECT t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}",
                   ot."{}" as jt_zone_id, ot."{}" as jt_id
            FROM "{}" t
            INNER JOIN "{}" ot ON t."{}" = ot."{}" AND t."{}" = ot."{}"
            WHERE ot."{}" = $1
            )",
            // WORLD triggers columns (9 base columns)
            db::Triggers::ID, db::Triggers::NAME, db::Triggers::ATTACH_TYPE,
            db::Triggers::FLAGS, db::Triggers::COMMANDS, db::Triggers::NUM_ARGS,
            db::Triggers::ARG_LIST, db::Triggers::VARIABLES, db::Triggers::ZONE_ID,
            db::Triggers::TABLE, db::Triggers::ZONE_ID, db::Triggers::ATTACH_TYPE,
            // MOB triggers columns (9 base columns + junction table columns)
            db::Triggers::ID, db::Triggers::NAME, db::Triggers::ATTACH_TYPE,
            db::Triggers::FLAGS, db::Triggers::COMMANDS, db::Triggers::NUM_ARGS,
            db::Triggers::ARG_LIST, db::Triggers::VARIABLES, db::Triggers::ZONE_ID,
            db::MobTriggers::MOB_ZONE_ID, db::MobTriggers::MOB_ID,
            db::Triggers::TABLE, db::MobTriggers::TABLE,
            db::Triggers::ZONE_ID, db::MobTriggers::TRIGGER_ZONE_ID,
            db::Triggers::ID, db::MobTriggers::TRIGGER_ID,
            db::MobTriggers::MOB_ZONE_ID,
            // OBJECT triggers columns (9 base columns + junction table columns)
            db::Triggers::ID, db::Triggers::NAME, db::Triggers::ATTACH_TYPE,
            db::Triggers::FLAGS, db::Triggers::COMMANDS, db::Triggers::NUM_ARGS,
            db::Triggers::ARG_LIST, db::Triggers::VARIABLES, db::Triggers::ZONE_ID,
            db::ObjectTriggers::OBJECT_ZONE_ID, db::ObjectTriggers::OBJECT_ID,
            db::Triggers::TABLE, db::ObjectTriggers::TABLE,
            db::Triggers::ZONE_ID, db::ObjectTriggers::TRIGGER_ZONE_ID,
            db::Triggers::ID, db::ObjectTriggers::TRIGGER_ID,
            db::ObjectTriggers::OBJECT_ZONE_ID
        );

        auto result = txn.exec_params(query, zone_id);

        std::vector<FieryMUD::TriggerDataPtr> triggers;
        triggers.reserve(result.size());

        // Column indices for junction table entity IDs (after 9 base columns)
        constexpr int JT_ZONE_ID = 9;
        constexpr int JT_ID = 10;

        for (const auto& row : result) {
            auto trigger = parse_trigger_row(row);

            // Override entity ID from junction table columns
            int jt_zone = row[JT_ZONE_ID].as<int>();
            int jt_id = row[JT_ID].as<int>();

            if (trigger->attach_type == FieryMUD::ScriptType::MOB) {
                trigger->mob_id = EntityId(
                    static_cast<std::uint32_t>(jt_zone),
                    static_cast<std::uint32_t>(jt_id)
                );
            } else if (trigger->attach_type == FieryMUD::ScriptType::OBJECT) {
                trigger->object_id = EntityId(
                    static_cast<std::uint32_t>(jt_zone),
                    static_cast<std::uint32_t>(jt_id)
                );
            } else if (trigger->attach_type == FieryMUD::ScriptType::WORLD) {
                trigger->zone_id = jt_zone;
            }

            triggers.push_back(trigger);
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
        // Query via MobTriggers junction table (many-to-many relationship)
        // Join on composite key: (zone_id, id) = (trigger_zone_id, trigger_id)
        std::string query = fmt::format(
            R"(SELECT t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}"
               FROM "{}" t
               INNER JOIN "{}" mt ON t."{}" = mt."{}" AND t."{}" = mt."{}"
               WHERE mt."{}" = $1 AND mt."{}" = $2)",
            db::Triggers::ID,
            db::Triggers::NAME,
            db::Triggers::ATTACH_TYPE,
            db::Triggers::FLAGS,
            db::Triggers::COMMANDS,
            db::Triggers::NUM_ARGS,
            db::Triggers::ARG_LIST,
            db::Triggers::VARIABLES,
            db::Triggers::ZONE_ID,
            db::Triggers::TABLE,
            db::MobTriggers::TABLE,
            db::Triggers::ZONE_ID, db::MobTriggers::TRIGGER_ZONE_ID,
            db::Triggers::ID, db::MobTriggers::TRIGGER_ID,
            db::MobTriggers::MOB_ZONE_ID,
            db::MobTriggers::MOB_ID
        );

        auto result = txn.exec_params(query, zone_id, mob_id);

        std::vector<FieryMUD::TriggerDataPtr> triggers;
        triggers.reserve(result.size());

        for (const auto& row : result) {
            auto trigger = parse_trigger_row(row);
            // Set mob_id from the query parameters
            trigger->mob_id = EntityId(
                static_cast<std::uint32_t>(zone_id),
                static_cast<std::uint32_t>(mob_id)
            );
            triggers.push_back(trigger);
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
        // Query via ObjectTriggers junction table (many-to-many relationship)
        // Join on composite key: (zone_id, id) = (trigger_zone_id, trigger_id)
        std::string query = fmt::format(
            R"(SELECT t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}"
               FROM "{}" t
               INNER JOIN "{}" ot ON t."{}" = ot."{}" AND t."{}" = ot."{}"
               WHERE ot."{}" = $1 AND ot."{}" = $2)",
            db::Triggers::ID,
            db::Triggers::NAME,
            db::Triggers::ATTACH_TYPE,
            db::Triggers::FLAGS,
            db::Triggers::COMMANDS,
            db::Triggers::NUM_ARGS,
            db::Triggers::ARG_LIST,
            db::Triggers::VARIABLES,
            db::Triggers::ZONE_ID,
            db::Triggers::TABLE,
            db::ObjectTriggers::TABLE,
            db::Triggers::ZONE_ID, db::ObjectTriggers::TRIGGER_ZONE_ID,
            db::Triggers::ID, db::ObjectTriggers::TRIGGER_ID,
            db::ObjectTriggers::OBJECT_ZONE_ID,
            db::ObjectTriggers::OBJECT_ID
        );

        auto result = txn.exec_params(query, zone_id, object_id);

        std::vector<FieryMUD::TriggerDataPtr> triggers;
        triggers.reserve(result.size());

        for (const auto& row : result) {
            auto trigger = parse_trigger_row(row);
            // Set object_id from the query parameters
            trigger->object_id = EntityId(
                static_cast<std::uint32_t>(zone_id),
                static_cast<std::uint32_t>(object_id)
            );
            triggers.push_back(trigger);
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

Result<FieryMUD::TriggerDataPtr> load_trigger_by_id(pqxx::work& txn, int zone_id, int trigger_id)
{
    try {
        std::string query = fmt::format(
            R"({} WHERE "{}" = $1 AND "{}" = $2)",
            build_trigger_select(),
            db::Triggers::ZONE_ID,
            db::Triggers::ID
        );

        auto result = txn.exec_params(query, zone_id, trigger_id);

        if (result.empty()) {
            return std::unexpected(Errors::NotFound(
                fmt::format("Trigger {}:{}", zone_id, trigger_id)));
        }

        return parse_trigger_row(result[0]);

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error loading trigger {}:{}: {}", zone_id, trigger_id, e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error loading trigger {}:{}: {}", zone_id, trigger_id, e.what())));
    }
}

Result<std::vector<FieryMUD::TriggerDataPtr>> load_all_triggers(pqxx::work& txn)
{
    try {
        // Use UNION to get triggers from three sources:
        // 1. WORLD triggers (attached to zones via zone_id)
        // 2. MOB triggers (attached via MobTriggers junction table)
        // 3. OBJECT triggers (attached via ObjectTriggers junction table)
        //
        // A trigger attached to multiple mobs will result in multiple rows.
        // The last two columns (jt_zone_id, jt_id) contain the entity IDs.
        // NOTE: Triggers table only has 9 real columns - entity associations
        // are handled via junction tables (MobTriggers, ObjectTriggers)
        std::string query = fmt::format(
            R"(
            -- WORLD triggers (zone_id is the entity reference)
            SELECT t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}",
                   t."{}" as jt_zone_id, 0 as jt_id
            FROM "{}" t
            WHERE t."{}" = 'WORLD' AND t."{}" IS NOT NULL

            UNION ALL

            -- MOB triggers via junction table (composite key join)
            SELECT t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}",
                   mt."{}" as jt_zone_id, mt."{}" as jt_id
            FROM "{}" t
            INNER JOIN "{}" mt ON t."{}" = mt."{}" AND t."{}" = mt."{}"

            UNION ALL

            -- OBJECT triggers via junction table (composite key join)
            SELECT t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}", t."{}",
                   ot."{}" as jt_zone_id, ot."{}" as jt_id
            FROM "{}" t
            INNER JOIN "{}" ot ON t."{}" = ot."{}" AND t."{}" = ot."{}"
            )",
            // WORLD triggers columns (9 base columns + 2 placeholder entity columns)
            db::Triggers::ID, db::Triggers::NAME, db::Triggers::ATTACH_TYPE,
            db::Triggers::FLAGS, db::Triggers::COMMANDS, db::Triggers::NUM_ARGS,
            db::Triggers::ARG_LIST, db::Triggers::VARIABLES, db::Triggers::ZONE_ID,
            db::Triggers::ZONE_ID,  // jt_zone_id for WORLD is the zone_id
            db::Triggers::TABLE, db::Triggers::ATTACH_TYPE, db::Triggers::ZONE_ID,
            // MOB triggers columns (9 base columns + 2 junction table entity columns)
            db::Triggers::ID, db::Triggers::NAME, db::Triggers::ATTACH_TYPE,
            db::Triggers::FLAGS, db::Triggers::COMMANDS, db::Triggers::NUM_ARGS,
            db::Triggers::ARG_LIST, db::Triggers::VARIABLES, db::Triggers::ZONE_ID,
            db::MobTriggers::MOB_ZONE_ID, db::MobTriggers::MOB_ID,
            db::Triggers::TABLE, db::MobTriggers::TABLE,
            db::Triggers::ZONE_ID, db::MobTriggers::TRIGGER_ZONE_ID,
            db::Triggers::ID, db::MobTriggers::TRIGGER_ID,
            // OBJECT triggers columns (9 base columns + 2 junction table entity columns)
            db::Triggers::ID, db::Triggers::NAME, db::Triggers::ATTACH_TYPE,
            db::Triggers::FLAGS, db::Triggers::COMMANDS, db::Triggers::NUM_ARGS,
            db::Triggers::ARG_LIST, db::Triggers::VARIABLES, db::Triggers::ZONE_ID,
            db::ObjectTriggers::OBJECT_ZONE_ID, db::ObjectTriggers::OBJECT_ID,
            db::Triggers::TABLE, db::ObjectTriggers::TABLE,
            db::Triggers::ZONE_ID, db::ObjectTriggers::TRIGGER_ZONE_ID,
            db::Triggers::ID, db::ObjectTriggers::TRIGGER_ID
        );

        auto result = txn.exec(query);

        std::vector<FieryMUD::TriggerDataPtr> triggers;
        triggers.reserve(result.size());

        // Column indices for junction table entity IDs (after 9 base columns)
        constexpr int JT_ZONE_ID = 9;
        constexpr int JT_ID = 10;

        for (const auto& row : result) {
            auto trigger = parse_trigger_row(row);

            // Override entity ID from junction table columns
            int jt_zone = row[JT_ZONE_ID].as<int>();
            int jt_id = row[JT_ID].as<int>();

            if (trigger->attach_type == FieryMUD::ScriptType::MOB) {
                trigger->mob_id = EntityId(
                    static_cast<std::uint32_t>(jt_zone),
                    static_cast<std::uint32_t>(jt_id)
                );
            } else if (trigger->attach_type == FieryMUD::ScriptType::OBJECT) {
                trigger->object_id = EntityId(
                    static_cast<std::uint32_t>(jt_zone),
                    static_cast<std::uint32_t>(jt_id)
                );
            } else if (trigger->attach_type == FieryMUD::ScriptType::WORLD) {
                trigger->zone_id = jt_zone;
            }

            triggers.push_back(trigger);
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

// ============================================================================
// Error Logging Functions
// ============================================================================

Result<void> log_script_error(
    pqxx::work& txn,
    int zone_id,
    int trigger_id,
    std::string_view error_type,
    std::string_view error_message,
    std::optional<int> script_line,
    std::optional<std::string> context_info)
{
    try {
        // Insert into script_error_log table
        std::string insert_query = R"(
            INSERT INTO "script_error_log"
                ("trigger_zone_id", "trigger_id", "error_type", "error_message", "script_line", "context_info", "occurred_at")
            VALUES ($1, $2, $3, $4, $5, $6::jsonb, NOW())
        )";

        if (script_line && context_info) {
            txn.exec_params(insert_query, zone_id, trigger_id,
                            std::string(error_type), std::string(error_message),
                            *script_line, *context_info);
        } else if (script_line) {
            txn.exec_params(insert_query, zone_id, trigger_id,
                            std::string(error_type), std::string(error_message),
                            *script_line, nullptr);
        } else if (context_info) {
            txn.exec_params(insert_query, zone_id, trigger_id,
                            std::string(error_type), std::string(error_message),
                            nullptr, *context_info);
        } else {
            txn.exec_params(insert_query, zone_id, trigger_id,
                            std::string(error_type), std::string(error_message),
                            nullptr, nullptr);
        }

        // Update the trigger's needsReview flag and syntaxError field
        std::string update_query = fmt::format(R"(
            UPDATE "{}"
            SET "{}" = true, "{}" = $3
            WHERE "{}" = $1 AND "{}" = $2
        )",
            db::Triggers::TABLE,
            db::Triggers::NEEDS_REVIEW,
            db::Triggers::SYNTAX_ERROR,
            db::Triggers::ZONE_ID,
            db::Triggers::ID
        );

        txn.exec_params(update_query, zone_id, trigger_id, std::string(error_message));

        spdlog::debug("Logged script error for trigger {}:{}: {}", zone_id, trigger_id, error_type);
        return {};

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error logging script error for {}:{}: {}", zone_id, trigger_id, e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error logging script error for {}:{}: {}", zone_id, trigger_id, e.what())));
    }
}

Result<std::vector<FieryMUD::TriggerDataPtr>> get_triggers_needing_review(pqxx::work& txn)
{
    try {
        std::string query = fmt::format(
            R"({} WHERE "{}" = true)",
            build_trigger_select(),
            db::Triggers::NEEDS_REVIEW
        );

        auto result = txn.exec(query);

        std::vector<FieryMUD::TriggerDataPtr> triggers;
        triggers.reserve(result.size());

        for (const auto& row : result) {
            triggers.push_back(parse_trigger_row(row));
        }

        spdlog::debug("Found {} triggers needing review", triggers.size());
        return triggers;

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error getting triggers needing review: {}", e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error getting triggers needing review: {}", e.what())));
    }
}

Result<std::vector<FieryMUD::TriggerDataPtr>> get_triggers_needing_review_for_zone(
    pqxx::work& txn, int zone_id)
{
    try {
        std::string query = fmt::format(
            R"({} WHERE "{}" = true AND "{}" = $1)",
            build_trigger_select(),
            db::Triggers::NEEDS_REVIEW,
            db::Triggers::ZONE_ID
        );

        auto result = txn.exec_params(query, zone_id);

        std::vector<FieryMUD::TriggerDataPtr> triggers;
        triggers.reserve(result.size());

        for (const auto& row : result) {
            triggers.push_back(parse_trigger_row(row));
        }

        spdlog::debug("Found {} triggers needing review in zone {}", triggers.size(), zone_id);
        return triggers;

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error getting triggers needing review for zone {}: {}", zone_id, e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error getting triggers needing review for zone {}: {}", zone_id, e.what())));
    }
}

Result<std::vector<ScriptErrorEntry>> get_error_log_for_trigger(
    pqxx::work& txn, int zone_id, int trigger_id, int limit)
{
    try {
        std::string query = fmt::format(R"(
            SELECT e."id", e."trigger_zone_id", e."trigger_id", t."{}",
                   e."error_type", e."error_message", e."script_line",
                   to_char(e."occurred_at", 'YYYY-MM-DD HH24:MI:SS') as occurred_at
            FROM "script_error_log" e
            JOIN "{}" t ON e."trigger_zone_id" = t."{}" AND e."trigger_id" = t."{}"
            WHERE e."trigger_zone_id" = $1 AND e."trigger_id" = $2
            ORDER BY e."occurred_at" DESC
            LIMIT $3
        )",
            db::Triggers::NAME,
            db::Triggers::TABLE,
            db::Triggers::ZONE_ID,
            db::Triggers::ID
        );

        auto result = txn.exec_params(query, zone_id, trigger_id, limit);

        std::vector<ScriptErrorEntry> entries;
        entries.reserve(result.size());

        for (const auto& row : result) {
            ScriptErrorEntry entry;
            entry.id = row[0].as<int>();
            entry.zone_id = row[1].as<int>();
            entry.trigger_id = row[2].as<int>();
            entry.trigger_name = row[3].as<std::string>();
            entry.error_type = row[4].as<std::string>();
            entry.error_message = row[5].as<std::string>();
            if (!row[6].is_null()) {
                entry.script_line = row[6].as<int>();
            }
            entry.occurred_at = row[7].as<std::string>();
            entries.push_back(std::move(entry));
        }

        return entries;

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error getting error log for trigger {}:{}: {}", zone_id, trigger_id, e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error getting error log for trigger {}:{}: {}", zone_id, trigger_id, e.what())));
    }
}

Result<std::vector<ScriptErrorEntry>> get_recent_script_errors(pqxx::work& txn, int limit)
{
    try {
        std::string query = fmt::format(R"(
            SELECT e."id", e."trigger_zone_id", e."trigger_id", t."{}",
                   e."error_type", e."error_message", e."script_line",
                   to_char(e."occurred_at", 'YYYY-MM-DD HH24:MI:SS') as occurred_at
            FROM "script_error_log" e
            JOIN "{}" t ON e."trigger_zone_id" = t."{}" AND e."trigger_id" = t."{}"
            ORDER BY e."occurred_at" DESC
            LIMIT $1
        )",
            db::Triggers::NAME,
            db::Triggers::TABLE,
            db::Triggers::ZONE_ID,
            db::Triggers::ID
        );

        auto result = txn.exec_params(query, limit);

        std::vector<ScriptErrorEntry> entries;
        entries.reserve(result.size());

        for (const auto& row : result) {
            ScriptErrorEntry entry;
            entry.id = row[0].as<int>();
            entry.zone_id = row[1].as<int>();
            entry.trigger_id = row[2].as<int>();
            entry.trigger_name = row[3].as<std::string>();
            entry.error_type = row[4].as<std::string>();
            entry.error_message = row[5].as<std::string>();
            if (!row[6].is_null()) {
                entry.script_line = row[6].as<int>();
            }
            entry.occurred_at = row[7].as<std::string>();
            entries.push_back(std::move(entry));
        }

        return entries;

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error getting recent script errors: {}", e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error getting recent script errors: {}", e.what())));
    }
}

Result<void> clear_trigger_error(pqxx::work& txn, int zone_id, int trigger_id)
{
    try {
        std::string query = fmt::format(R"(
            UPDATE "{}"
            SET "{}" = false, "{}" = NULL
            WHERE "{}" = $1 AND "{}" = $2
        )",
            db::Triggers::TABLE,
            db::Triggers::NEEDS_REVIEW,
            db::Triggers::SYNTAX_ERROR,
            db::Triggers::ZONE_ID,
            db::Triggers::ID
        );

        txn.exec_params(query, zone_id, trigger_id);

        spdlog::debug("Cleared error flag for trigger {}:{}", zone_id, trigger_id);
        return {};

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("SQL error clearing trigger error {}:{}: {}", zone_id, trigger_id, e.what())));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::DatabaseError(
            fmt::format("Error clearing trigger error {}:{}: {}", zone_id, trigger_id, e.what())));
    }
}

} // namespace TriggerQueries
