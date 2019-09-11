/***************************************************************************
 * $Id: constants.h,v 1.22 2009/06/09 05:36:54 myc Exp $
 ***************************************************************************/
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
#define DEFAULT_PROMPT	4
extern const char *log_severities[];

/***************************************************************************
 * $Log: constants.h,v $
 * Revision 1.22  2009/06/09 05:36:54  myc
 * Added privilege_bits for PRV flags.
 *
 * Revision 1.21  2009/03/09 05:41:31  jps
 * Moved money stuff into money.h, money.c
 *
 * Revision 1.20  2009/03/09 05:09:22  jps
 * Moved effect flags and strings into effects.h and effects.c.
 *
 * Revision 1.19  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.18  2009/03/09 03:45:17  jps
 * Extract some spell-mem related stuff from structs.h and put it in spell_mem.h
 *
 * Revision 1.17  2008/09/03 17:34:08  myc
 * Moved liquid information into a def struct array.
 *
 * Revision 1.16  2008/09/01 18:29:38  jps
 * consolidating cooldown code in skills.c/h
 *
 * Revision 1.15  2008/08/29 04:16:26  myc
 * Added toggles for stacking objects and stacking mobiles.
 * Removed global fullness description array.
 *
 * Revision 1.14  2008/08/14 23:02:11  myc
 * Added an array of graduated log severity names.
 *
 * Revision 1.13  2008/06/19 18:53:12  myc
 * Replaced the item_types and item_type_desc arrays with a
 * struct array in objects.c
 *
 * Revision 1.12  2008/06/05 02:07:43  myc
 * Added a list of mappings from wear positions to item wear flags.
 *
 * Revision 1.11  2008/05/18 02:01:06  jps
 * Moved some room-related constants into rooms.c and rooms.h.
 *
 * Revision 1.10  2008/04/02 04:55:59  myc
 * Added coin names array.
 *
 * Revision 1.9  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.8  2008/03/21 15:01:17  myc
 * Removed languages.
 *
 * Revision 1.7  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.
 *
 * Revision 1.6  2008/03/10 19:55:37  jps
 * Made a struct for sizes with name, height, and weight.  Save base height
 * weight and size so they stay the same over size changes.
 *
 * Revision 1.5  2008/03/10 18:01:17  myc
 * Added a lookup list for posture types, and added a couple more
 * default prompts.
 *
 * Revision 1.4  2008/03/08 22:29:06  myc
 * Cooldowns are now listed on stat.
 *
 * Revision 1.3  2008/03/05 03:03:54  myc
 * Moved default prompts here from the do_display command.
 *
 * Revision 1.2  2008/02/06 21:53:53  myc
 * Adding an apply_abbrevs array.
 *
 * Revision 1.1  2008/01/29 21:02:31  myc
 * Initial revision
 *
 *
 ***************************************************************************/
