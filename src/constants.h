/***************************************************************************
 *  File: constats.h                                      Part of FieryMUD *
 *  Usage: header file for constants                                       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

extern const char circlemud_version[];
extern const char *minor_creation_items[];
extern const char *exit_bits[];
extern const char *genders[NUM_SEXES + 1];
extern const char *position_types[NUM_POSITIONS + 1];
extern const char *stance_types[NUM_STANCES + 1];
extern const char *player_bits[NUM_PLR_FLAGS + 1];
extern const char *action_bits[NUM_MOB_FLAGS + 1];
extern const char *preference_bits[NUM_PRF_FLAGS + 1];
extern const char *privilege_bits[NUM_PRV_FLAGS + 1];
extern const char *connected_types[NUM_CON_MODES + 1];
extern const char *where[NUM_WEARS];
extern const char *equipment_types[NUM_WEARS + 1];
extern const char *wear_positions[NUM_WEARS + 1];
extern const int wear_order_index[NUM_WEARS];
extern const int wear_flags[NUM_WEARS];
extern const char *wear_bits[NUM_ITEM_WEAR_FLAGS + 1];
extern const char *extra_bits[NUM_ITEM_FLAGS + 1];
extern const char *apply_types[NUM_APPLY_TYPES + 1];
extern const char *apply_abbrevs[NUM_APPLY_TYPES + 1];
extern const char *container_bits[];
extern const char *carry_desc[];
extern const char *weekdays[];
extern const char *rolls_abils_result[];
extern const char *month_name[];
extern const int sharp[];
extern const char *default_prompts[][2];
#define DEFAULT_PROMPT 4
extern const char *log_severities[];
