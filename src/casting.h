/***************************************************************************
 *   File: casting.h                                      Part of FieryMUD *
 *  Usage: header file: constants and fn prototypes for spell system       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium.       *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University.                                       *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_CASTING_H
#define __FIERY_CASTING_H

#include "structs.h"
#include "sysdep.h"
#include "spell_mem.h"

#define DEFAULT_STAFF_LVL 12
#define DEFAULT_WAND_LVL 12

#define CAST_UNDEFINED -1
#define CAST_SPELL 0
#define CAST_POTION 1
#define CAST_WAND 2
#define CAST_STAFF 3
#define CAST_SCROLL 4
#define CAST_BREATH 5
#define CAST_CHANT 6
#define CAST_SINGN 7

#define CAST_RESULT_CHARGE (1 << 0)
#define CAST_RESULT_IMPROVE (1 << 1)

#define MAG_DAMAGE (1 << 0)
#define MAG_AFFECT (1 << 1)
#define MAG_UNAFFECT (1 << 2)
#define MAG_POINT (1 << 3)
#define MAG_ALTER_OBJ (1 << 4)
#define MAG_GROUP (1 << 5)
#define MAG_MASS (1 << 6)
#define MAG_AREA (1 << 7)
#define MAG_SUMMON (1 << 8)
#define MAG_CREATION (1 << 9)
#define MAG_MANUAL (1 << 10)
#define MAG_ROOM (1 << 11)
#define MAG_BULK_OBJS (1 << 12)
#define MAG_REPEAT (1 << 13)
#define NUM_ROUTINE_TYPES 13

#define TYPE_UNDEFINED -1

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */
/* enumerate the base mem time for each circle. PLEASE feel free to alter based
 * on practicality */
#define C1 30
#define C2 33
#define C3 36
#define C4 39
#define C5 42
#define C6 45
#define C7 48
#define C8 51
#define C9 54
#define C10 57
#define C11 60
#define C12 60
#define C13 60
#define C14 60

/*
 * spell casting time defines , ordinary ints can be used
 * for other values, but should be declared here...
 * NOTE: These integers represent pulses, not seconds. if PULSE_VIOLENCE is
 * used, then each of the following is (n*2) seconds.
 */
/* This is based on 4 second rounds. */
#define CAST_SPEED1 6  /* 1.5  rounds */
#define CAST_SPEED2 7  /* 1.75 rounds */
#define CAST_SPEED3 8  /* 2.0  rounds */
#define CAST_SPEED4 9  /* 2.25 rounds */
#define CAST_SPEED5 10 /* 2.5  rounds */
#define CAST_SPEED6 12 /* 3.0  rounds */
#define CAST_SPEED7 14 /* 3.5  rounds */

/* The following casting times are for offensive spells. */

#define CAST_SPEEDA 1
#define CAST_SPEEDB 2
#define CAST_SPEEDC 3
#define CAST_SPEEDD 4
#define CAST_SPEEDE 5
#define CAST_SPEEDF 6
#define CAST_SPEEDG 7
#define CAST_SPEEDH 8
#define CAST_SPEEDI 9
#define CAST_SPEEDJ 10
#define CAST_SPEEDK 11

/* This is a target status to determine after do_cast what exactly the
   target is supposed to be. This fixes casting abort and crash bugs
   Nechtrous
*/
#define TARGET_NULL 0
#define TARGET_ALL_ROOM 1
#define TARGET_IN_ROOM 2
#define TARGET_IN_WORLD 3
#define TARGET_IN_INV 4
#define TARGET_EQUIP 5
#define TARGET_SELF 6
#define TARGET_FIGHTING 7
#define TARGET_STRING 8

/*
 * TAR_SELF_ONLY, TAR_NOT_SELF, TAR_NIGHT_ONLY, TAR_DAY_ONLY,
 * and TAR_OUTDOORS are only checks.  They must be used with one of the
 * other target values to make the cast command actually look for a
 * target.  For instance, TAR_CHAR_ROOM | TAR_NIGHT_ONLY would hit
 * a target in the room, but would only work at night.
 *
 * Possible Targets:
 *
 * bit 0 : IGNORE TARGET
 * bit 1 : PC/NPC in room
 * bit 2 : PC/NPC in world
 * bit 3 : If fighting, and no argument, select tar_char as self
 * bit 4 : If fighting, and no argument, select tar_char as victim (fighting)
 * bit 5 : If no argument, select self, if argument check that it IS self.
 * bit 6 : Don't allow spell to be targeted at caster
 * bit 7 : Object in inventory
 * bit 8 : Object in room
 * bit 9 : Object in world
 * bit 10: Object held / in equipment
 * bit 11: The argument is used as a string (for spells like locate object)
 * bit 12: Only allow casting at night
 * bit 13: Only allow casting during the day
 * bit 14: Only allow casting outdoors
 * bit 15: Only allow casting on ground (no structure/air/water sector)
 * bit 16: Skill or spell is contact-based
 * bit 17: Skill or spell is ranged and requires direct line-of-sight
 */

#define TAR_IGNORE (1 << 0)
#define TAR_CHAR_ROOM (1 << 1)
#define TAR_CHAR_WORLD (1 << 2)
#define TAR_FIGHT_SELF (1 << 3)
#define TAR_FIGHT_VICT (1 << 4)
#define TAR_SELF_ONLY (1 << 5) /* Only a check */
#define TAR_NOT_SELF (1 << 6)  /* Only a check */
#define TAR_OBJ_INV (1 << 7)
#define TAR_OBJ_ROOM (1 << 8)
#define TAR_OBJ_WORLD (1 << 9)
#define TAR_OBJ_EQUIP (1 << 10)
#define TAR_STRING (1 << 11)
#define TAR_NIGHT_ONLY (1 << 12) /* Only a check */
#define TAR_DAY_ONLY (1 << 13)   /* Only a check */
#define TAR_OUTDOORS (1 << 14)   /* Only a check */
#define TAR_GROUND (1 << 15)     /* Only a check */
#define TAR_CONTACT (1 << 16)
#define TAR_DIRECT (1 << 17)
#define NUM_TAR_FLAGS 18

/* Keeping track of casters, so destroyed objects can abort spells cast on them.
 * These functions are defined in spell_parser.c. */
void targets_remember_caster(struct char_data *caster);
void obj_forget_caster(struct obj_data *obj, struct char_data *caster);
void char_forget_caster(struct char_data *ch, struct char_data *caster);
void obj_forget_casters(struct obj_data *obj);
void char_forget_casters(struct char_data *ch);

#define MANUAL_SPELL(spellname) imp_skill |= spellname(spellnum, skill, caster, cvict, ovict, savetype);

#define CASTING(ch) (EVENT_FLAGGED(ch, EVENT_CASTING))

#define STOP_CASTING(ch)                                                                                               \
    REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_CASTING);                                                                   \
    if ((ch)->casting.obj)                                                                                             \
        obj_forget_caster((ch)->casting.obj, ch);                                                                      \
    if ((ch)->casting.tch)                                                                                             \
        char_forget_caster((ch)->casting.tch, ch);                                                                     \
    WAIT_STATE(ch, PULSE_VIOLENCE / 2);

int mag_damage(int skill, struct char_data *ch, struct char_data *victim, int spellnum, int savetype);

int mag_affect(int skill, struct char_data *ch, struct char_data *victim, int spellnum, int savetype, int casttype);

void perform_mag_group(int skill, struct char_data *ch, struct char_data *tch, int spellnum, int savetype);

int mag_group(int skill, struct char_data *ch, int spellnum, int savetype);

int mag_mass(int skill, struct char_data *ch, int spellnum, int savetype);

int mag_area(int skill, struct char_data *ch, int spellnum, int savetype);

int mag_summon(int skill, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int spellnum,
               int savetype);

int mag_point(int skill, struct char_data *ch, struct char_data *victim, int spellnum, int savetype);

int mag_unaffect(int skill, struct char_data *ch, struct char_data *victim, int spellnum, int type);

int mag_alter_obj(int skill, struct char_data *ch, struct obj_data *obj, int spellnum, int type);

int mag_bulk_objs(int skill, struct char_data *ch, int spellnum, int type);

int mag_creation(int skill, struct char_data *ch, int spellnum);

int call_magic(struct char_data *caster, struct char_data *cvict, struct obj_data *ovict, int spellnum, int skill,
               int casttype);

int mag_room(int skill, struct char_data *ch, int spellnum);

void mag_objectmagic(struct char_data *ch, struct obj_data *obj, char *argument);

int cast_spell(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int spellnum);

/* other prototypes */
void free_mem_list(struct char_data *ch);
void free_scribe_list(struct char_data *ch);
void init_mem_list(struct char_data *ch);
void save_mem_list(struct char_data *ch);
int add_spell(struct char_data *ch, int spell, int can_cast, int mem_time, bool verbose);

#include "events.h"
EVENTFUNC(delayed_cast_event);
EVENTFUNC(room_undo_event);
struct delayed_cast_event_obj *construct_delayed_cast(struct char_data *ch, struct char_data *victim, int spellnum,
                                                      int routines, int rounds, int wait, int skill, int savetype,
                                                      bool sustained);

extern int spells_of_circle[LVL_IMPL + 1][NUM_SPELL_CIRCLES + 1];
extern const char *targets[NUM_TAR_FLAGS + 1];
extern const char *routines[NUM_ROUTINE_TYPES + 1];

#endif
