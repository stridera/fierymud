/***************************************************************************
 * $Id: warrior.c,v 1.20 2009/03/08 23:34:14 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: warrior.c                                      Part of FieryMUD *
 *  Usage: controls Warrior type mobs, it is closly related to ai.h,       *
 *         and ai_util.c                                                   *
 *     By: Proky of HubisMUD                                               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
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
#include "events.h"
#include "movement.h"

/* Commands */
ACMD(do_backstab);
ACMD(do_bash);
ACMD(do_hitall);
ACMD(do_kick);
ACMD(do_rescue);

/* External functions */
bool has_piercing_weapon(struct char_data *ch);

/*
 * warrior_ai_action
 *
 * Basic warrior mob AI.  Returns TRUE if an action is taken, FALSE
 * otherwise.
 */
bool warrior_ai_action(struct char_data * ch, struct char_data * victim)
{
  int roll, i;
  struct char_data *tch;

  if (!victim) {
    mudlog("No victim in warrior AI action.", NRM, LVL_GOD, FALSE);
    return FALSE;
  }

  /*
   * BASH
   *
   * If someone is
   *   + in the room,
   *   + grouped with the tank,
   *   + casting
   *   + a player
   *   + visible to the mob
   *   + within bashing size
   * then attempt to bash them first.
   */
  if (!EFF_FLAGGED(ch, EFF_BLIND) && GET_SKILL(ch, SKILL_BASH) &&
      FIGHTING(ch) && GET_EQ(ch, WEAR_SHIELD) &&
      (number(25, 60) - GET_SKILL(ch, SKILL_BASH)) <= 0)
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
      i = GET_SIZE(ch) - GET_SIZE(tch);
      if (is_grouped(FIGHTING(ch), tch) && !IS_NPC(tch) &&
             CAN_SEE(ch, tch) && CASTING(tch) && i <= 2 && i >= -1) {
        do_bash(ch, GET_NAME(tch), 0, 0);
        return TRUE;
      }
    }

  /*
   * BREATHE / SWEEP / ROAR
   *
   * If the mob is a dragon/demon try to use dragon/demon skills.
   * There is a 15% chance of something occuring in this function,
   * if the mobile has the skills.
   */
  if (dragonlike_attack(ch))
    return TRUE;

  /*
   * KICK / BASH / HITALL / BODYSLAM / BACKSTAB (for mercs) / RESCUE
   *
   * Random chance to bash and kick proportional to level.
   * Increased chance to bash mages and clerics.
   */
  roll = number(0, 101);
  if (roll < GET_LEVEL(ch)) {

    roll *= 100 / GET_LEVEL(ch);
    i = GET_SIZE(ch) - GET_SIZE(victim);
    if (CAN_SEE(ch, victim) && roll < GET_LEVEL(ch) &&
        GET_SKILL(ch, SKILL_BODYSLAM) &&
        !FIGHTING(ch) && GET_POS(victim) >= POS_STANDING) {
      do_bash(ch, GET_NAME(victim), 0, SCMD_BODYSLAM);
      return TRUE;
    }
    if (roll > 75 && GET_SKILL(ch, SKILL_HITALL) && FIGHTING(ch)) {
      do_hitall(ch, "", 0, SCMD_HITALL);
      return TRUE;
    }
    /*
     * BACKSTAB
     *
     * Mercenaries share a lot of skills with warriors, but they have
     * backstab too.  Attempt to start combat using backstab.
     */
    if (CAN_SEE(ch, victim) && roll > 95 && GET_SKILL(ch, SKILL_BACKSTAB) &&
        has_piercing_weapon(ch) && !is_tanking(ch)) {
      do_backstab(ch, GET_NAME(victim), 0, 0);
      return TRUE;
    }
    /*
     * To attempt a bash we need a) skill in bash, b) a shield, c)
     * opponent standing, d) realistic size, and e) opponent not !bash.
     * If the opponent is a caster, increased chance to attempt a bash.
     * Higher level mobs get a better chance of attempting a bash.
     * (A level 99 mob has about a 50% chance of attempting to bash a
     * warrior opponent.)
     */
    if (CAN_SEE(ch, victim) && roll > (80 - GET_SKILL(ch, SKILL_BASH) / 2 -
        (classes[(int) GET_CLASS(victim)].magical ? 20 : 0)) &&
        GET_SKILL(ch, SKILL_BASH) && GET_EQ(ch, WEAR_SHIELD) &&
        GET_POS(victim) >= POS_STANDING && i <= 2 && i > -1 &&
        !MOB_FLAGGED(victim, MOB_NOBASH)) {
      do_bash(ch, GET_NAME(victim), 0, SCMD_BASH);
      return TRUE;
    }
/* This would probably not work very well, because the GET_NAME refers to
   a mob, and we don't know if it's 1.guard or 2.guard etc.  Skills need
   to be re-thought-out for use with mobs.  Maybe this is v3 territory.
    else if (GET_SKILL(ch, SKILL_RESCUE) &&
            FIGHTING(ch) && FIGHTING(victim) &&
            roll > 65 &&
            (tch = FIGHTING(victim)) != ch &&
            GET_LEVEL(tch) < GET_LEVEL(ch) + 5 &&
            GET_HIT(tch) < GET_MAX_HIT(tch) / 4 &&
            GET_HIT(ch) > GET_MAX_HIT(ch) / 2 &&
            GET_HIT(ch) > GET_HIT(tch)) {
      do_rescue(ch, GET_NAME(tch), 0, 0);
      return TRUE;
    }
*/
    if (GET_SKILL(ch, SKILL_KICK)) {
      do_kick(ch, GET_NAME(victim), 0, 0);
      return TRUE;
    }
  }

  return FALSE;
}

/***************************************************************************
 * $Log: warrior.c,v $
 * Revision 1.20  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.19  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.18  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.17  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.16  2008/03/10 18:01:17  myc
 * Bash, hitall, and bodyslam now use subcommands.
 *
 * Revision 1.15  2008/02/09 21:07:50  myc
 * Need access to event flags to check casting status.
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
 * Fixed formatting a bit and fixed the check for mercenary's backstab.
 *
 * Revision 1.10  2008/01/12 19:08:14  myc
 * Rerowte a lot of mob AI functionality.
 *
 * Revision 1.9  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.8  2007/04/19 00:53:54  jps
 * Create macros for stopping spellcasting.
 *
 * Revision 1.7  2006/11/08 07:55:17  jps
 * Change verbal instances of "breath" to "breathe"
 *
 * Revision 1.6  2002/09/30 01:11:34  jjl
 * Put in checks so guard actually does something.
 *
 * Revision 1.5  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.4  2000/11/25 02:33:15  rsd
 * Altered comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/02/04 18:18:16  mud
 * indented file
 * added standard comment header
 * dos2unix
 *
 * Revision 1.1  1999/01/29 01:23:32  mud
 * Initial revision
 *
 ***************************************************************************/
