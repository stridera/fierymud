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

constexpr char circlemud_version[] = "CircleMUD, version 3.00 beta patchlevel 11\n";
constexpr char mudlet_client_version[] = "0.3";
constexpr char mudlet_client_url[] = "http://www.fierymud.org/mudlet/FierymudOfficial.mpackage";
constexpr char mudlet_map_url[] = "http://www.fierymud.org/mudlet/default_map.dat";
constexpr char discord_invite_url[] = "https://discord.gg/aqhapUCgFz";
constexpr char discord_app_id[] = "998826809686765569";
constexpr char fierymud_icon[] = "https://www.fierymud.org/images/fiery64.png";
constexpr char fierymud_url[] = "https://www.fierymud.org";

extern const char *minor_creation_items[];
extern const char *exit_bits[];
extern const char *genders[NUM_SEXES + 1];
extern const char *stance_types[NUM_STANCES + 1];
extern const char *position_types[NUM_POSITIONS + 1];
extern const char *player_bits[NUM_PLR_FLAGS + 1];
extern const char *action_bits[NUM_MOB_FLAGS + 1];
extern const char *preference_bits[NUM_PRF_FLAGS + 1];
extern const char *privilege_bits[NUM_PRV_FLAGS + 1];
extern const char *connected_types[NUM_CON_MODES + 1];
extern const char *where[NUM_WEARS];
extern const char *equipment_types[NUM_WEARS + 1];
extern const char *wear_positions[NUM_WEARS + 1];
extern int wear_order_index[NUM_WEARS];
extern const int wear_flags[NUM_WEARS];
extern const char *wear_bits[NUM_ITEM_WEAR_FLAGS + 1];
extern const char *extra_bits[NUM_ITEM_FLAGS + 1];
extern const char *apply_types[NUM_APPLY_TYPES + 1];
extern const char *apply_abbrevs[NUM_APPLY_TYPES + 1];
extern const char *container_bits[];
extern const char *carry_desc[];
extern const char *weekdays[DAYS_PER_WEEK];
extern const char *rolls_abils_result[];
extern const char *month_name[MONTHS_PER_YEAR];
extern const int sharp[];
extern const char *default_prompts[][2];