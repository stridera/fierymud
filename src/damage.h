/***************************************************************************
 *  File: damage.h                                        Part of FieryMUD *
 *  Usage: header file for damage types                                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_DAMAGE_H
#define __FIERY_DAMAGE_H

#include "sysdep.h"
#include "structs.h"

/* When adding a damage type, keep in mind that the susceptibility may vary
 * by composition and/or life force.  Consider adding a field to one or
 * both of those structs.
 *
 * Also add a case statement to the susceptibility() function.
 * The default vulnerability for any damage type is 100.
 */
#define DAM_UNDEFINED -1
#define DAM_SLASH 0
#define DAM_PIERCE 1
#define DAM_CRUSH 2
#define DAM_SHOCK 3
#define DAM_FIRE 4
#define DAM_WATER 5
#define DAM_COLD 6
#define DAM_ACID 7
#define DAM_POISON 8
#define DAM_HEAL 9
#define DAM_ALIGN 10
#define DAM_DISPEL 11
#define DAM_DISCORPORATE 12
#define DAM_MENTAL 13
#define NUM_DAMTYPES 14 /* keep updated */

struct damdef {
    char *name;
    char *color;
    char *verb1st;
    char *verb2nd;
    char *action;
};

#define VALID_DAMTYPE(d) (d >= 0 && d < NUM_DAMTYPES)

extern struct damdef damtypes[];
extern int parse_damtype(struct char_data *ch, char *arg);

extern int skill_to_dtype(int skill);
extern void damage_evasion_message(struct char_data *ch, struct char_data *vict, struct obj_data *weapon, int dtype);
extern int weapon_damtype(struct obj_data *obj);
extern int physical_damtype(struct char_data *ch);

#endif

/***************************************************************************
 * $Log: damage.h,v $
 * Revision 1.1  2009/03/08 21:43:13  jps
 * Initial revision
 *
 * Revision 1.31  2008/08/30 01:31:51  myc
 * Changed the way stats are calculated in effect_total; ability
 * stats are saved in a raw form now, and only capped when accessed.
 * Damroll and hitroll are recalculated everytime effect_total
 * is called, using cached base values.
 *
 * Revision 1.30  2008/08/24 19:29:11  jps
 * Apply damage susceptibility reductions to the various physical attack skills.
 *
 * Revision 1.29  2008/08/20 05:03:13  jps
 * Removed the damage type 'magic'.
 *
 * Revision 1.28  2008/06/09 23:00:13  myc
 * Added 'extern' to all the function declarations.
 *
 * Revision 1.27  2008/05/12 02:50:25  jps
 * Add VALID_DAMTYPE
 *
 * Revision 1.26  2008/05/11 05:52:25  jps
 * Added some function prototypes.
 *
 * Revision 1.25  2008/05/10 16:19:50  jps
 * Made EVASIONCLR globally available.
 *
 * Revision 1.24  2008/04/26 23:36:32  myc
 * Added align_color function.
 *
 * Revision 1.23  2008/04/13 01:42:18  jps
 * Adding composition_check() function.
 *
 * Revision 1.22  2008/04/06 19:48:52  jps
 * Add an adjective to compositions.
 *
 * Revision 1.21  2008/04/05 18:06:53  jps
 * Add an action word to damtypes.
 *
 * Revision 1.20  2008/04/05 03:46:10  jps
 * Add a "mass noun" string to composition definitions.
 *
 * Revision 1.19  2008/03/29 16:26:57  jps
 * Add an evasion check exclusively for non-damaging attack spells.
 *
 * Revision 1.18  2008/03/27 00:21:07  jps
 * Add some verbs to damage types.
 *
 * Revision 1.17  2008/03/26 22:57:53  jps
 * Add a rigid field to compositions.
 *
 * Revision 1.16  2008/03/26 18:11:36  jps
 * Passing attacker and weapon into damage_evasion() so that when you
 * attack an ethereal mob with a blessed weapon (or self), it will
 * be vulnerable.
 *
 * Revision 1.15  2008/03/25 21:59:57  jps
 * Removing earth and air damage. Added shock, cold, and mental. Changed
 * iron composition to mental. Added lava and plant.
 * Added a default damage depending on composition.
 *
 * Revision 1.14  2008/03/25 05:30:41  jps
 * Adding weapon_damtype(), equipped_weapon(), and physical_damtype() functions.
 *
 * Revision 1.13  2008/03/24 08:43:29  jps
 * Adding prototypes for damage_evasion(), skill_to_dtype(), and
 * damage_evasion_message().
 *
 * Revision 1.12  2008/03/23 19:46:29  jps
 * Added compositions stone and bone.
 *
 * Revision 1.11  2008/03/23 18:41:20  jps
 * New damage defines (old ones in spells.h are now obsolete). Added damage
 * susceptibilities to struct lifedef and struct compdef.
 *
 * Revision 1.10  2008/03/23 00:25:57  jps
 * Add base composition so it can be temporarily changed with an apply.
 *
 * Revision 1.9  2008/03/22 20:26:37  jps
 * Add functions to convert life force and composition.
 * Rename ethereal to ether - compositions should be nouns.
 *
 * Revision 1.8  2008/03/22 19:57:28  jps
 * Added some macros for lifeforce and compositions.
 *
 * Revision 1.7  2008/03/22 19:10:10  jps
 * Added lists of lifeforce and composition definitions.
 *
 * Revision 1.6  2008/03/11 02:56:32  jps
 * Add size-releated defs and functions.
 *
 * Revision 1.5  2008/01/27 09:39:47  jps
 * Added "Layman" class as the default, unskilled class for mobiles.
 * I did it because the old default class, "Nothin", occupied the -1
 * index position in an array. -1-indexed arrays????? No!
 *
 * Revision 1.4  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.3  2008/01/25 21:05:45  myc
 * Removed the barehand_mult and backstab_mult functions.  This
 * functionality is contained within the hit function now.  Besides,
 * barehand_mult was poorly named, as it isn't multiplied, but rather
 * added.
 *
 * Revision 1.2  2008/01/05 22:01:22  jps
 * Add stat indices
 *
 ***************************************************************************/
