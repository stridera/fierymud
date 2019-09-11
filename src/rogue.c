/***************************************************************************
 * $Id: rogue.c,v 1.15 2009/03/08 23:34:14 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: rogue.c                                        Part of FieryMUD *
 *  Usage: Control of rogue type mobs, It is closly related to ai.h,       *
 *         and ai_util.c.                                                  *
 *     By: Proky of HubisMUD                                               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "casting.h"
#include "ai.h"
#include "skills.h"
#include "math.h"

/* External functions */
ACMD(do_backstab);
ACMD(do_corner);
ACMD(do_eye_gouge);
ACMD(do_kick);
ACMD(do_steal);
ACMD(do_throatcut);

bool has_piercing_weapon(struct char_data *ch) {
  if (GET_EQ(ch, WEAR_WIELD) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_WIELD)))
    return TRUE;
  if (GET_EQ(ch, WEAR_WIELD2) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_WIELD2)))
    return TRUE;
  if (GET_EQ(ch, WEAR_2HWIELD) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_2HWIELD)))
    return TRUE;
  return FALSE;
}

bool rogue_ai_action(struct char_data *ch, struct char_data *victim) {
  int roll;

  if (!victim) {
    mudlog("No victim in rogue AI action.", NRM, LVL_GOD, FALSE);
    return FALSE;
  }

  /* Success in doing an action? */
  roll = number(0, 101);
  if (roll >= GET_LEVEL(ch))
    return FALSE;
  roll *= 100 / GET_LEVEL(ch);

  /*
   * Backstab requires a piercing weapon.
   */
  if (CAN_SEE(ch, victim) && roll > 95 && GET_SKILL(ch, SKILL_BACKSTAB) &&
      has_piercing_weapon(ch) && !is_tanking(ch)) {
    do_backstab(ch, GET_NAME(victim), 0, 0);
    return TRUE;
  }

  if (CAN_SEE(ch, victim) && roll > 94 && GET_SKILL(ch, SKILL_THROATCUT) &&
      has_piercing_weapon(ch) && !FIGHTING(ch)) {
    do_throatcut(ch, GET_NAME(victim), 0, 0);
    return TRUE;
  }

  if (CAN_SEE(ch, victim) && roll > 70 && GET_SKILL(ch, SKILL_CORNER) &&
      FIGHTING(ch) == victim && !ch->cornering) {
    do_corner(ch, GET_NAME(victim), 0, 0);
    return TRUE;
  }

  if (CAN_SEE(ch, victim) && roll > 50 && GET_SKILL(ch, SKILL_EYE_GOUGE)) {
    do_eye_gouge(ch, GET_NAME(victim), 0, 0);
    return TRUE;
  }

  if (GET_SKILL(ch, SKILL_KICK)) {
    do_kick(ch, GET_NAME(victim), 0, 0);
    return TRUE;
  }

  return FALSE;  
}

bool mob_steal(struct char_data *ch) {
  struct char_data *vict = get_random_char_around(ch, RAND_AGGRO | RAND_PLAYERS);

  if (vict && GET_LEVEL(ch) + 5 > GET_LEVEL(vict)) {
    if (vict->carrying && CAN_SEE_OBJ(ch, vict->carrying) && number(0, 1))
      sprintf(buf1, "%s %s", fname(vict->carrying->short_description), GET_NAME(vict));
    else
      sprintf(buf1, "%s %s", "coins", GET_NAME(vict));
    do_steal(ch, buf1, 0, 0);
    return TRUE;
  }

  return FALSE;
}

/***************************************************************************
 * $Log: rogue.c,v $
 * Revision 1.15  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.14  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.13  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.12  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.11  2008/01/13 03:19:53  myc
 * Fixed checks for rogue skills.  Also changed mob_steal to
 * try and steal the first thing in a player's inventory.
 *
 * Revision 1.10  2008/01/12 23:13:20  myc
 * Moved mob steal code here.
 *
 * Revision 1.9  2008/01/12 19:08:14  myc
 * Rerowte a lot of mob AI functionality.
 *
 * Revision 1.8  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.7  2002/09/30 01:39:58  jjl
 * Urgh. Forgot the function prototype for check_guard.
 *
 * Revision 1.6  2002/09/30 01:11:09  jjl
 * Put in checks for guard, so it actually does something.
 *
 * Revision 1.5  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.4  2000/11/24 21:17:12  rsd
 * Altered comment header and added back rlog messgaes from
 * prior to the addition of the $log$ string.
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/02/02 00:08:11  mud
 * Indented file
 * added standard and corrected comment header, I don't think
 * this file controls cleric AI like it claimed.
 * dos2unix
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/
