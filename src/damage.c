/***************************************************************************
 *  File: damage.c                                       Part of FieryMUD  *
 *  Usage: Source file for damage types                                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "skills.h"
#include "math.h"
#include "damage.h"
#include "composition.h"

/* name, color, verb1st, verb2nd, action */

struct damdef damtypes[NUM_DAMTYPES] = {
   { "slash", "&3", "slash", "slashes", "slash" },
   { "pierce", "&3", "pierce", "pierces", "slash" },
   { "crush", "&3", "crush", "crushes", "crush" },
   { "shock", "&4&b", "shock", "shocks", "shock" },
   { "fire", "&1&b", "burn", "burns", "flame" },
   { "water", "&4", "drown", "drowns", "flood" },
   { "cold", "&4", "freeze", "freezes", "freeze" },
   { "acid", "&2", "corrode", "corrodes", "spray" },
   { "poison", "&2&b", "poison", "poisons", "poison" },
   { "heal", "&6", "harm", "harms", "harm" },
   { "align", "&6&b", "rebuke", "rebukes", "retribution" },
   { "dispel", "&5&b", "dispel", "dispels", "dispersion" },
   { "discorporate", "&5", "discorporate", "discorporates", "discorporation" },
   { "mental", "", "punish", "punishes", "punishment" }
};

int parse_damtype(struct char_data *ch, char *arg)
{
   return parse_obj_name(ch, arg, "damage type", NUM_DAMTYPES,
         damtypes, sizeof(struct damdef));
}


/* damage_evasion()
 *
 * Whether a character avoids some negative effect entirely.
 * It's based on susceptibility.  If susceptibility to the given damage type is
 * 100 or more, the character will not evade.  If susceptibility is 0, the
 * character is guaranteed to evade.  Between that, it curves so that if you're
 * closer to 100, the probability of evasion is very low.  You have to really
 * start getting toward 0 susceptibility before evasion becomes very
 * probable. */

bool damage_evasion(struct char_data *ch, struct char_data *attacker,
      struct obj_data *weapon, int dtype)
{
   int s;

   /* Ether mobs are not immune at all to blessed physical attacks. */
   if (attacker && ((dtype == DAM_PIERCE || dtype == DAM_SLASH || dtype == DAM_CRUSH) &&
         GET_COMPOSITION(ch) == COMP_ETHER)) {
      return !(EFF_FLAGGED(attacker, EFF_BLESS) || EFF_FLAGGED(attacker, EFF_HEX));
   }

   s = susceptibility(ch, dtype);
   return number(1, 1000000) > 1000000 - (100 - s) * (100 - s) * (100 - s);
}

int skill_to_dtype(int skill)
{
   switch(skill) {
      case TYPE_HIT:       return DAM_CRUSH;
      case TYPE_STING:     return DAM_PIERCE;
      case TYPE_WHIP:      return DAM_SLASH;
      case TYPE_SLASH:     return DAM_SLASH;
      case TYPE_BITE:      return DAM_PIERCE;
      case TYPE_BLUDGEON:  return DAM_CRUSH;
      case TYPE_CRUSH:     return DAM_CRUSH;
      case TYPE_POUND:     return DAM_CRUSH;
      case TYPE_CLAW:      return DAM_SLASH;
      case TYPE_MAUL:      return DAM_CRUSH;
      case TYPE_THRASH:    return DAM_SLASH;
      case TYPE_PIERCE:    return DAM_PIERCE;
      case TYPE_BLAST:     return DAM_CRUSH;
      case TYPE_PUNCH:     return DAM_CRUSH;
      case TYPE_STAB:      return DAM_PIERCE;
      default:
                           return DAM_CRUSH;
   }
}

void damage_evasion_message(struct char_data *ch, struct char_data *vict,
      struct obj_data *weapon, int dtype)
{
   int damtype = dtype;

   if (weapon) {
      damtype = skill_to_dtype(GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT);
   }

   /* Check for physical attacks against not-so-solid opponents. */
   if ((damtype == DAM_SLASH || damtype == DAM_PIERCE || damtype == DAM_CRUSH) &&
         !RIGID(vict)) {
      if (weapon) {
         act(EVASIONCLR "Your $o passes harmlessly through $N" EVASIONCLR
               "!&0", FALSE, ch, weapon, vict, TO_CHAR);
         act(EVASIONCLR "$n" EVASIONCLR "'s $o passes harmlessly through $N"
               EVASIONCLR "!&0", FALSE, ch, weapon, vict, TO_NOTVICT);
         act(EVASIONCLR "$n" EVASIONCLR "'s $o passes harmlessly through you.&0",
               FALSE, ch, weapon, vict, TO_VICT);
      } else {
         act(EVASIONCLR "Your fist passes harmlessly through $N" EVASIONCLR
               "!&0", FALSE, ch, 0, vict, TO_CHAR);
         act(EVASIONCLR "$n" EVASIONCLR "'s fist passes harmlessly through $N"
               EVASIONCLR "!&0", FALSE, ch, 0, vict, TO_NOTVICT);
         act(EVASIONCLR "$n" EVASIONCLR "'s fist passes harmlessly through you.&0",
               FALSE, ch, 0, vict, TO_VICT);
      }
      return;
   }

   /* For the physical attacks (slash, pierce, crush), the victim is known
    * to be rigid at this point. */
   switch (damtype) {
      case DAM_SLASH:
         if (weapon) {
            act(EVASIONCLR "Your $o slides harmlessly off $N" EVASIONCLR
                  "!&0", FALSE, ch, weapon, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o slides harmlessly off $N"
                  EVASIONCLR "!&0", FALSE, ch, weapon, vict, TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o slides harmlessly off you.&0",
                  FALSE, ch, weapon, vict, TO_VICT);
         } else {
            act(EVASIONCLR "Your blade slides harmlessly off $N" EVASIONCLR
                  "!&0", FALSE, ch, 0, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s blade slides harmlessly off $N"
                  EVASIONCLR "!&0", FALSE, ch, 0, vict, TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s blade slides harmlessly off you.&0",
                  FALSE, ch, 0, vict, TO_VICT);
         }
         break;
      case DAM_PIERCE:
         if (weapon) {
            act(EVASIONCLR "Your $o fails to pierce $N" EVASIONCLR
                  " at all!&0", FALSE, ch, weapon, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o fails to pierce $N"
                  EVASIONCLR " at all!&0", FALSE, ch, weapon, vict, TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o fails to pierce you at all.&0",
                  FALSE, ch, weapon, vict, TO_VICT);
         } else {
            act(EVASIONCLR "You fail to pierce $N" EVASIONCLR " at all!&0",
                  FALSE, ch, 0, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR " fails to pierce $N" EVASIONCLR
                  " at all!&0", FALSE, ch, 0, vict, TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR " fails to pierce you at all.&0",
                  FALSE, ch, 0, vict, TO_VICT);
         }
         break;
      case DAM_CRUSH:
         if (weapon) {
            act(EVASIONCLR "Your $o bounces harmlessly off $N" EVASIONCLR
                  "!&0", FALSE, ch, weapon, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o bounces harmlessly off $N"
                  EVASIONCLR "!&0", FALSE, ch, weapon, vict, TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o bounces harmlessly off you.&0",
                  FALSE, ch, weapon, vict, TO_VICT);
         } else {
            act(EVASIONCLR "Your fist bounces harmlessly off $N" EVASIONCLR
                  "!&0", FALSE, ch, 0, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s fist bounces harmlessly off $N"
                  EVASIONCLR "!&0", FALSE, ch, 0, vict, TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s fist bounces harmlessly off you.&0",
                  FALSE, ch, 0, vict, TO_VICT);
         }
         break;
      case DAM_POISON:
         sprintf(buf, EVASIONCLR "$n" EVASIONCLR " tries to %s $N"
               EVASIONCLR ", but $E is immune!&0",
               damtypes[damtype].verb1st);
         act(buf, FALSE, ch, 0, vict, TO_NOTVICT);
         sprintf(buf, EVASIONCLR "You try to %s $N" EVASIONCLR
               ", but $E is immune!&0",
               damtypes[damtype].verb1st);
         act(buf, FALSE, ch, 0, vict, TO_CHAR);
         sprintf(buf, EVASIONCLR "$n" EVASIONCLR
               " tries to %s you, but you are immune!&0",
               damtypes[damtype].verb1st);
         act(buf, FALSE, ch, 0, vict, TO_VICT);
         break;
      case DAM_SHOCK:
      case DAM_FIRE:
      case DAM_WATER:
      case DAM_COLD:
      case DAM_ACID:
      case DAM_HEAL:
      case DAM_ALIGN:
      case DAM_DISPEL:
      case DAM_DISCORPORATE:
      default:
         if (number(1, 3) == 1)
            sprintf(buf, EVASIONCLR "$n" EVASIONCLR " tries to %s $N"
                  EVASIONCLR ", but $E is completely unaffected!&0",
                  damtypes[damtype].verb1st);
         else
            sprintf(buf, EVASIONCLR "$n" EVASIONCLR "'s %s has no effect on $N" EVASIONCLR "!",
                  damtypes[damtype].action);
         act(buf, FALSE, ch, 0, vict, TO_NOTVICT);
         if (number(1, 3) == 1)
            sprintf(buf, EVASIONCLR "You try to %s $N" EVASIONCLR
                  ", but $E is completely unaffected!&0",
                  damtypes[damtype].verb1st);
         else
            sprintf(buf, EVASIONCLR "Your %s has no effect on $N" EVASIONCLR "!",
                  damtypes[damtype].action);
         act(buf, FALSE, ch, 0, vict, TO_CHAR);
         if (number(1, 3) == 1)
            sprintf(buf, EVASIONCLR "$n" EVASIONCLR
                  " tries to %s you, but you are completely unaffected!&0",
                  damtypes[damtype].verb1st);
         else
            sprintf(buf, EVASIONCLR "$n" EVASIONCLR "'s %s has no effect on you!",
                  damtypes[damtype].action);
         act(buf, FALSE, ch, 0, vict, TO_VICT);
         break;
   }
}

int weapon_damtype(struct obj_data *obj)
{
   if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
      return skill_to_dtype(TYPE_HIT + GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE));
   return DAM_CRUSH;
}


int physical_damtype(struct char_data *ch)
{
   if (equipped_weapon(ch))
      return weapon_damtype(equipped_weapon(ch));
   else
      return COMPOSITION_DAM(ch);
}


/***************************************************************************
 * $Log: damage.c,v $
 * Revision 1.1  2009/03/08 21:43:13  jps
 * Initial revision
 *
 ***************************************************************************/
