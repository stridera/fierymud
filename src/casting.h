/***************************************************************************
 * $Id: casting.h,v 1.93 2009/03/20 06:15:17 myc Exp $
 ***************************************************************************/
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

#include "sysdep.h"
#include "structs.h"
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

/***************************************************************************
 * $Log: casting.h,v $
 * Revision 1.93  2009/03/20 06:15:17  myc
 * Adding a TAR_GROUND cast requirement.  Added detonation,
 * phosphoric embers, positive field, and acid burst spells.
 * Removed combust and heatwave.  Made soul tap a manual spell.
 *
 * Revision 1.92  2009/03/09 03:45:17  jps
 * Extract some spell-mem related stuff from structs.h and put it in spell_mem.h
 *
 * Revision 1.91  2009/03/09 03:26:34  jps
 * Moved individual spell definitions to spell.c and spell.h.
 *
 * Revision 1.90  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.89  2008/09/21 21:04:20  jps
 * Passing cast type to mag_affect so that potions of bless/dark presence can be
 *quaffed by neutral people.
 *
 * Revision 1.88  2008/09/11 02:50:02  jps
 * Changed skills so you have a minimum position, and fighting_ok fields.
 *
 * Revision 1.87  2008/08/25 00:20:33  myc
 * Changed the way mobs memorize spells.
 *
 * Revision 1.86  2008/08/01 05:20:20  jps
 * Rolled "show spell" into "show skill".
 *
 * Revision 1.85  2008/05/19 05:46:48  jps
 * Remove unused ASPELL def.
 *
 * Revision 1.84  2008/05/18 02:02:17  jps
 * Adding prototype for manual spell for isolation.
 *
 * Revision 1.83  2008/05/12 03:36:42  jps
 * Added valid_cast_stance
 *
 * Revision 1.82  2008/05/12 02:50:05  jps
 * Add do_show_spell and level_to_circle, which help with
 * "show spell <spell>".
 *
 * Revision 1.81  2008/04/19 21:10:31  myc
 * Added a 'show skill' subcommand, which required a list of
 * target flag and routine type strings.
 *
 * Revision 1.80  2008/04/14 08:37:22  jps
 * Replaced calls to spell_wall_of_stone and spell_wall_of_ice
 * with a single ASPELL for spell_magical_wall.  It will also
 * handle illusory wall.  Changed ASPELL and MANUAL_SPELL to
 * include the spell number.
 *
 * Revision 1.79  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.78  2008/03/23 18:42:53  jps
 * Removing obsolete damage types.  Damage types are now defined in chars.h.
 *
 * Revision 1.77  2008/03/09 08:58:28  jps
 * Add definition for manual spell of fear.
 *
 * Revision 1.76  2008/03/05 03:03:54  myc
 * Exporting add_spell function from spell_mem.c
 *
 * Revision 1.75  2008/02/23 01:03:54  myc
 * Renamed and publicizing a bunch of spell memorization functions.
 *
 * Revision 1.74  2008/02/09 21:07:50  myc
 * Casting uses event flags instead of plr/mob flags now.
 *
 * Revision 1.73  2008/01/29 16:51:12  myc
 * Adding the contact and direct (LoS) skills and spells.
 *
 * Revision 1.72  2008/01/26 23:19:28  jps
 * Remove the equipment-destroying manual spell for acid breath.
 *
 * Revision 1.71  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.70  2008/01/23 14:15:34  jps
 * Added the "humanoid" field to skill definitions.
 *
 * Revision 1.69  2008/01/13 23:06:04  myc
 * Changed the spell_info struct to store lowest_level and lowest_circle
 * data for each spell, thus making it unnecessary to calculate the
 * minimum spell level for major globe over and over.
 *
 * Revision 1.68  2008/01/12 23:13:20  myc
 * Declaring spells_of_circle in header file.
 *
 * Revision 1.67  2008/01/07 10:38:56  jps
 * Rename project to phantasm.
 *
 * Revision 1.66  2008/01/06 23:50:47  jps
 * Added spells project and simulacrum, and MOB2_ILLUSORY flag.
 *
 * Revision 1.65  2008/01/06 20:38:25  jps
 * Add function for ventriloquate.
 *
 * Revision 1.64  2008/01/05 21:54:43  jps
 * Saving throw defs moved to chars.h.
 *
 * Revision 1.63  2008/01/05 05:42:09  jps
 * Explicitly exporting update_skills() and spell_info[].
 *
 * Revision 1.62  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.61  2008/01/02 07:11:58  jps
 * Using class.h.
 *
 * Revision 1.60  2008/01/02 01:26:32  jps
 * Added defines for the sphere-related skills.
 *
 * Revision 1.59  2007/12/31 02:00:57  jps
 * Made the general term for spells, skills, chants, and songs 'talent'.
 * Fixed mskillset to handle all talents.
 *
 * Revision 1.58  2007/11/25 00:04:59  jps
 * Spell targets will keep close track of whoever's casting a spell
 * at them.  This allows spells to be safely aborted if the target
 * is removed from the game before the spell is completed.
 *
 * Revision 1.57  2007/10/13 20:15:09  myc
 * Added functions to find spell/skill/chant nums to spells.h
 *
 * Revision 1.56  2007/10/13 05:07:24  myc
 * Added new monk chants.
 *
 * Revision 1.55  2007/10/11 20:14:48  myc
 * Changed skill defines to support chants and songs as skills, but
 * slightly distinguished from spells and skills.  TOP_SKILL is the
 * old MAX_SKILLS.  Chants and songs now each have a block of 50
 * defines above the new MAX_SKILLS (550).  This is important
 * because MAX_SKILLS (now TOP_SKILL) is used in the pfile.
 *
 * Revision 1.54  2007/10/02 02:52:27  myc
 * Energy drain is now a manual spell.
 *
 * Revision 1.53  2007/09/15 15:36:48  myc
 * Natures embrace is now in mag_affects instead of a manual spell.
 *
 * Revision 1.52  2007/09/11 16:34:24  myc
 * Added peck, claw, and electrify skills for use by druid shapechanges.
 *
 * Revision 1.51  2007/09/09 01:20:14  jps
 * The result of casting a spell is no longer just TRUE or FALSE,
 * but two possible bits combined: charge and/or improve. If
 * CAST_RESULT_CHARGE is returned, the spell was used and the caster
 * will be charged (have the spell erased from memory).  If
 * CAST_RESULT_IMPROVE is returned, the caster may improve in that
 * sphere of magic.
 * At the same time, casters will now correctly be charged for
 * spells that are cast on objects.
 *
 * Revision 1.50  2007/09/07 01:37:34  jps
 * Add shorter casting times for attack spells.
 *
 * Revision 1.49  2007/09/04 06:49:19  myc
 * Added new TAR_OUTDOORS, TAR_NIGHT_ONLY, and TAR_DAY_ONLY checks for
 * spells to limit when and where they may be cast (like TAR_SELF_ONLY or
 * TAR_NOT_SELF).  This replaces checks within spells, and the checks are
 * made before the caster's spell memory is charged.
 *
 * Revision 1.48  2007/08/30 08:51:25  jps
 * Add define for spell_info.
 *
 * Revision 1.47  2007/08/27 21:18:00  myc
 * You can now queue up commands while casting as well as abort midcast.
 * Casting commands such as look and abort are caught and interpreted
 * before the input is normally queued up by the game loop.
 *
 * Revision 1.46  2007/08/14 22:43:07  myc
 * Adding conceal, corner, shadow, and stealth skills.
 *
 * Revision 1.45  2007/08/05 20:21:51  myc
 * Added retreat and group retreat skills.
 *
 * Revision 1.44  2007/08/02 04:19:04  jps
 * Added "moonbeam" spell for Druids.
 *
 * Revision 1.43  2007/08/02 00:23:53  myc
 * Standardized magic check-PK function.  Cut out a LOT of unnecessary magic
 * code and cleaned up the whole system in general.  Magic casts are now
 * guaranteed to use sphere skills rather than level.  Almost all magic
 * functions like mag_damage or even manual spells return a boolean now:
 * TRUE if the cast deserves a skill improvement, FALSE if it doesn't.
 * This return value is ignored for object magic (wands, etc.).
 *
 * Revision 1.42  2007/07/31 07:39:04  jps
 * Added macros IS_SKILL, IS_SPELL, and IS_LANGUAGE for classifying
 * skill numbers.
 *
 * Revision 1.41  2007/07/04 02:21:58  myc
 * Renamed douse spell to extinguish.
 *
 * Revision 1.40  2007/06/16 00:15:49  myc
 * Three spells for necromancers: soul tap, rebuke undead,
 * and degeneration.  One spell for rangers: natures guidance.
 *
 * Revision 1.39  2007/05/11 21:03:12  myc
 * New rogue skill, eye gouge, allows rogues to gouge out eyes.  A very
 * complicated skill.  :P  Fixed cure blind's logic, and made it support
 * eye gouge too.
 *
 * Revision 1.38  2007/05/11 20:13:28  myc
 * Vaporform is a new circle 13 spell for cryomancers.  It significantly
 * increases the caster's chance of dodging a hit.  It is a quest spell.
 *
 * Revision 1.37  2007/04/19 00:53:54  jps
 * Create macros for stopping spellcasting.
 *
 * Revision 1.36  2007/04/17 23:38:03  myc
 * Introducing the new improved color spray!  It's now an area spell that
 * causes various effects based on caster skill.
 *
 * Revision 1.35  2007/02/14 03:54:53  myc
 * Replaced firewalk and greater firewalk with combust and cremate.
 *
 * Revision 1.34  2006/11/18 21:04:29  jps
 * Make casting items use the same targeting code as regular spellcasting.
 *
 * Revision 1.33  2006/11/18 04:26:32  jps
 * Renamed continual light spell to illumination, and it only works on
 * LIGHT items (still rooms too).
 *
 * Revision 1.32  2006/11/08 07:55:17  jps
 * Change verbal instances of "breath" to "breathe"
 *
 * Revision 1.31  2003/09/02 03:07:14  jjl
 * Fixed On Fire / Lay Hands, which both tried to claim skill ID 470
 *
 * Revision 1.30  2003/06/25 02:21:03  jjl
 * Revised lay hands to not suck.
 *
 * Revision 1.29  2003/04/16 02:00:22  jjl
 * Added skill timers for Zzur.  They don't save to file, so they were a
 * quickie.
 *
 * Revision 1.28  2002/12/19 07:42:02  rls
 * define for spell_remove_curse
 *
 * Revision 1.27  2002/10/14 02:16:08  jjl
 * An update to turn vitality into a set of 6 spells, lesser endurance,
 * endurance, greater endurance, vitality, greater vitality, and dragon's
 * health.  Greater endurance is what vitality was.  The rest are scaled
 * appropriately.    The higher end may need scaled down, or may not.
 *
 * Revision 1.26  2002/09/15 04:17:02  jjl
 * D'oh.  Mistyped the headers. . .  Fixed that.
 *
 * Revision 1.24  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.23  2002/07/16 23:25:17  rls
 * added in necro spell, bone armor
 *
 * Revision 1.22  2000/11/25 02:33:15  rsd
 * Altered comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.21  2000/03/18 06:20:12  rsd
 * removed the SPELL_SPHERE references since they essentially
 * do the same thing as SKILL_SPHERE_X and it seemed as though
 * it was a nightmare to cross use them. spell parser.c was
 * way changed due to this, check it out.
 *
 * Revision 1.20  1999/11/23 15:48:23  jimmy
 * Fixed the slashing weapon skill.  I had it erroneously as stabbing. Doh.
 * Reinstated dual wield.
 * Allowed mobs/players to pick up items while fighting.
 * Fixed a bug in the damage message that wrongfully indicated a miss
 * due to a rounding error in the math.
 * This was all done in order to facilitate the chance to sling your
 * weapon in combat.  Dex and proficiency checks are now made on any missed
 * attact and a failure of both causes the weapon to be slung.
 *
 * Revision 1.18  1999/11/17 20:42:17  jimmy
 * Added sphere of divination, and sphere skills.
 *
 * Revision 1.16  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.15  1999/08/29 07:06:04  jimmy
 * Many many small but ver significant bug fixes found using insure.  The
 * code now compiles cleanly and boots cleanly with insure.  The most
 *significant changes were moving all the BREATH's to within normal spell range,
 *and fixing the way socials were allocated.  Too many small fixes to list them
 * all. --gurlaek (now for the runtime debugging :( )
 *
 * Revision 1.14  1999/07/22 17:43:59  jimmy
 * Moved the identify spell back into the normal castable spells range.
 * moved all the breath spells up by one to compensate for moveing identify.
 *
 * Revision 1.13  1999/07/20 19:45:51  jimmy
 * This is the spanky New Spell recognition code.
 * This code allows mobs/players that have the KNOW_SPELL skill
 * to make a skill check to guess the spell.  A good roll will show both
 * the spell and the target.  A bad roll will show the spell garbled and
 * then an INT check for the target.  If a really bad roll is made, the spell
 * will be replaced by an incorrect one.  the heart of this system is
 * start_chant(), end_chant, and bad_guess().
 * --gurlaek 7/20/1999
 *
 * Revision 1.12  1999/07/15 03:15:05  mud
 * change cast speed defines to make more sense and have a
 * wider range of time.. see CAST_SPEEDx
 *
 * Revision 1.11  jimmy
 * This is a Mass check-in of the new skill/spell/language assignment system.
 * This New system combines the assignment of skill/spell/language for
 * both mobs and PCs.  LOts of code was touched and many errors were fixed.
 * MCLASS_VOID was moved from 13 to -1 to match CLASS_UNDEFINED for PC's.
 * MObs now get random skill/spell/language levels baseed on their
 *race/class/level that exactly align with PC's.  PC's no longer have to rent to
 *use skills gained by leveling or when first creating a char.  Languages no
 *longer reset to defaults when a PC levels.  Discovered that languages have
 *been defined right in the middle of the spell area.  This needs to be fixed.
 *A conversion util neeDs to be run on the mob files to compensate for the 13 to
 *-1 class change.
 * --gurlaek 7/6/1999
 *
 * Revision 1.10  1999/05/26 02:00:04  mud
 * added #define SKILL_SUMMON_MOUNT          450
 *
 * Revision 1.9  1999/04/18 20:12:54  dce
 * Magic missile works like Fire Dart, Ice Dart works.
 *
 * Revision 1.8  1999/03/24 23:43:16  jimmy
 * Working on quest spells.  Still in progress.  HOwever, spell_info[] array now
 *has a flag quest.  If it's true then it's considerd a quest spell.  Also,
 *allowed pyro/cryo's to learn from any sorcerer type teacher fingon
 *
 * Revision 1.7  1999/03/10 00:03:37  dce
 * Monk semantics for dodge/parry/ripost/attack
 *
 * Revision 1.6  1999/03/08 20:22:35  dce
 * Adds the skill safefall for monks.
 *
 * Revision 1.5  1999/03/07 05:01:09  dce
 * Chant finishes and wearoff messages.
 *
 * Revision 1.4  1999/03/03 20:11:02  jimmy
 * Many enhancements to scribe and spellbooks.  Lots of checks added.  Scribe is
 *now a skill. Spellbooks now have to be held to scribe as well as a quill in
 *the other hand.
 *
 * -fingon
 *
 * Revision 1.3  1999/02/26 22:30:30  dce
 * Monk additions/fixes
 *
 * Revision 1.2  1999/02/13 19:37:12  dce
 * Rewrote Continual Light and Darkness to be manual spells to meet our needs.
 *
 * Revision 1.1  1999/01/29 01:23:32  mud
 * Initial revision
 *
 ***************************************************************************/
