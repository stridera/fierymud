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

#pragma once

#include "spell_mem.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

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
#define CAST_PERFORM 7
#define CAST_INSTRUMENT 8

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
typedef std::bitset<NUM_ROUTINE_TYPES> RoutineFlags;

#define TYPE_UNDEFINED -1

/*
 * spell casting time defines, ordinary ints can be used for other values, but should be declared here...
 * NOTE: These integers represent pulses, not seconds. if PULSE_VIOLENCE is used, then each of the following is (n*2)
 * seconds.
 */
/* This is based on 4 second rounds. */

#define CAST_SPEED1 1   /* 1*   .5 rounds -> reduces to 0 */
#define CAST_SPEED2 2   /* 1*   .5 rounds   */
#define CAST_SPEED4 4   /* 2*   1 round    */
#define CAST_SPEED6 6   /* 3*   1.5 rounds */
#define CAST_SPEED8 8   /* 4*   2 rounds   */
#define CAST_SPEED10 10 /* 5*   2.5 rounds */
#define CAST_SPEED12 12 /* 6*   3 rounds   */
#define CAST_SPEED14 14 /* 7*   3.5 rounds */
#define CAST_SPEED16 16 /* 8*   4 rounds   */
#define CAST_SPEED18 18 /* 9*   4.5 rounds */

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
void targets_remember_caster(CharData *caster);
void obj_forget_caster(ObjData *obj, CharData *caster);
void char_forget_caster(CharData *ch, CharData *caster);
void obj_forget_casters(ObjData *obj);
void char_forget_casters(CharData *ch);

#define MANUAL_SPELL(spellname) imp_skill |= spellname(spellnum, skill, caster, cvict, ovict, savetype);

#define CASTING(ch) (EVENT_FLAGGED(ch, EVENT_CASTING))

#define STOP_CASTING(ch)                                                                                               \
    GET_EVENT_FLAGS(ch).reset(EVENT_CASTING);                                                                          \
    if ((ch)->casting.obj)                                                                                             \
        obj_forget_caster((ch)->casting.obj, ch);                                                                      \
    if ((ch)->casting.tch)                                                                                             \
        char_forget_caster((ch)->casting.tch, ch);                                                                     \
    WAIT_STATE(ch, PULSE_VIOLENCE / 2);

int mag_damage(int skill, CharData *ch, CharData *victim, int spellnum, int savetype);
int mag_affect(int skill, CharData *ch, CharData *victim, int spellnum, int savetype, int casttype);
void perform_mag_group(int skill, CharData *ch, CharData *tch, int spellnum, int savetype);
int mag_group(int skill, CharData *ch, int spellnum, int savetype);
int mag_mass(int skill, CharData *ch, int spellnum, int savetype);
int mag_area(int skill, CharData *ch, int spellnum, int savetype);
int mag_summon(int skill, CharData *ch, CharData *vict, ObjData *obj, int spellnum, int savetype);
int mag_point(int skill, CharData *ch, CharData *victim, int spellnum, int savetype);
int mag_unaffect(int skill, CharData *ch, CharData *victim, int spellnum, int type);
int mag_alter_obj(int skill, CharData *ch, ObjData *obj, int spellnum, int type);
int mag_bulk_objs(int skill, CharData *ch, int spellnum, int type);
int mag_creation(int skill, CharData *ch, int spellnum);
int call_magic(CharData *caster, CharData *cvict, ObjData *ovict, int spellnum, int skill, int casttype);
int mag_room(int skill, CharData *ch, int spellnum);
void mag_objectmagic(CharData *ch, ObjData *obj, char *argument);
int cast_spell(CharData *ch, CharData *tch, ObjData *tobj, int spellnum);

/* other prototypes */
void free_mem_list(CharData *ch);
void free_scribe_list(CharData *ch);
void init_mem_list(CharData *ch);
void save_mem_list(CharData *ch);
int add_spell(CharData *ch, int spell, int can_cast, int addl_mem_time, bool verbose);

#include "events.hpp"
EVENTFUNC(delayed_cast_event);
EVENTFUNC(room_undo_event);
DelayedCastEventObj *construct_delayed_cast(CharData *ch, CharData *victim, int spellnum, int routines, int rounds,
                                            int wait, int skill, int savetype, bool sustained);
