/***************************************************************************
 *  File: constats.h                                      Part of FieryMUD *
 *  Usage: header file for constants                                       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#pragma once

#include "defines.hpp"
#include "logging.hpp"
#include "utils.hpp"

#define DEFAULT_PROMPT 4

constexpr char version[] = "FieryMUD.  Originally based on CircleMUD 3.0\n";
constexpr char mudlet_client_version[] = "0.5";
constexpr char mudlet_client_url[] = "http://www.fierymud.org/mudlet/FierymudOfficial.mpackage";
constexpr char mudlet_map_url[] = "http://www.fierymud.org/mudlet/default_map.dat";
constexpr char discord_invite_url[] = "https://discord.gg/aqhapUCgFz";
constexpr char discord_app_id[] = "998826809686765569";
constexpr char fierymud_icon[] = "https://www.fierymud.org/images/fiery64.png";
constexpr char fierymud_url[] = "https://www.fierymud.org";

extern const std::string_view number_words[];
extern const std::string_view minor_creation_items[];
extern const std::string_view exit_bits[];
extern const std::string_view genders[NUM_SEXES + 1];
extern const std::string_view stance_types[NUM_STANCES + 1];
extern const std::string_view position_types[NUM_POSITIONS + 1];
extern const std::string_view player_bits[NUM_PLR_FLAGS + 1];
extern const std::string_view action_bits[NUM_MOB_FLAGS + 1];
extern const std::string_view preference_bits[NUM_PRF_FLAGS + 1];
extern const std::string_view privilege_bits[NUM_PRV_FLAGS + 1];
extern const std::string_view connected_types[NUM_CON_MODES + 1];
extern const std::string_view where[NUM_WEARS];
extern const std::string_view equipment_types[NUM_WEARS + 1];
extern const std::string_view wear_positions[NUM_WEARS + 1];
extern int wear_order_index[NUM_WEARS];
extern const int wear_flags[NUM_WEARS];
extern const std::string_view wear_bits[NUM_ITEM_WEAR_FLAGS + 1];
extern const std::string_view extra_bits[NUM_ITEM_FLAGS + 1];
extern const std::string_view apply_types[NUM_APPLY_TYPES + 1];
extern const std::string_view apply_abbrevs[NUM_APPLY_TYPES + 1];
extern const std::string_view container_bits[];
extern const std::string_view carry_desc[];
extern const std::string_view weekdays[DAYS_PER_WEEK];
extern const std::string_view rolls_abils_result[];
extern const std::string_view month_name[MONTHS_PER_YEAR];
extern const int sharp[];

constexpr std::array<std::array<std::string_view, 2>, 11> default_prompts = {{
    {"Basic", "&0%hhp %vmv>&0 "},
    {"Colorized Basic", "&1&b%h&0&1hp &2&b%v&0&2mv&0> "},
    {"Basic Percentages", "&1&b%ph&0&1hp &2&b%pv&0&2mv&0> "},
    {"Full-Featured",
     "&6Opponent&0: &4&b%o &7&b/ &0&6Tank&0: &4&b%t%_&0&1%h&0(&1&b%H&0)"
     "hitp &2%v&0(&2&b%V&0)&7move&0> "},
    {"Standard",
     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b>%_"
     "&0<%t&0>:<&0%o&0> "},
    {"Complete w/ Spells",
     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
     "<&0&2%aA&2&b> <&0%l&2&b>%_&0<%t&0>:<&0%o&0> "},
    {"Complete w/ Exp",
     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
     "<&0&2%aA&2&b> <&0%e&2&b>%_&0<%t&0>:<&0%o&0> "},
    {"Complete w/ Hide Pts",
     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
     "<&0&2%aA&2&b> <&0&2%ih&2&b>%_&0<%t&0>:<&0%o&0> "},
    {"Complete w/ Rage",
     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
     "<&0&2%aA&2&b> <&0&2%rr&2&b>%_&0<%t&0>:<&0%o&0> "},
    {"Complete w/ 1st Aid",
     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
     "<&0&2%aA&2&b> <&0&2%df&2&b>%_&0<%t&0>:<&0%o&0> "},
}};
