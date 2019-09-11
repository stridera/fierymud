/***************************************************************************
 * $Id: skills.c,v 1.68 2010/06/05 14:56:27 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: skills.c                                       Part of FieryMUD  *
 *  Usage: Skill-management functions and data                             *
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
#include "math.h"
#include "interpreter.h"
#include "comm.h"
#include "skills.h"
#include "casting.h"
#include "races.h"
#include "events.h"
#include "chars.h"
#include "fight.h"
#include "damage.h"

struct skilldef skills[TOP_SKILL_DEFINE + 1];
int skill_sort_info[TOP_SKILL + 1];

const char *talent_types[5] = {
  "talent",
  "spell",
  "skill",
  "chant",
  "song",
};

const char *targets[NUM_TAR_FLAGS + 1] = {
  "IGNORE",
  "CHAR_ROOM",
  "CHAR_WORLD",
  "FIGHT_SELF",
  "FIGHT_VICT",
  "SELF_ONLY",
  "NOT_SELF",
  "OBJ_INV",
  "OBJ_ROOM",
  "OBJ_WORLD",
  "OBJ_EQUIP",
  "STRING",
  "NIGHT_ONLY",
  "DAY_ONLY",
  "OUTDOORS",
  "GROUND",
  "CONTACT",
  "DIRECT",
  "\n"
};

const char *routines[NUM_ROUTINE_TYPES + 1] = {
  "DAMAGE",
  "AFFECT",
  "UNAFFECT",
  "POINT",
  "ALTER_OBJ",
  "GROUP",
  "MASS",
  "AREA",
  "SUMMON",
  "CREATION",
  "MANUAL",
  "ROOM",
  "BULK_OBJS",
  "\n",
};

int talent_type(int skill_num)
{
  if (IS_SKILL(skill_num))
    return SKILL;
  if (IS_SPELL(skill_num))
    return SPELL;
  if (IS_CHANT(skill_num))
    return CHANT;
  if (IS_SONG(skill_num))
    return SONG;
  return TALENT;
}


const char *skill_name(int num)
{
  if (num <= 0 || num >= TOP_SKILL_DEFINE) {
    if (num == -1)
      return "UNUSED";
    else
      return "UNDEFINED";
  }

  return skills[num].name;
}

int find_talent_num(char *name, int should_restrict)
{
  int index = 0, abbrevmatch = -1, ok;
  char *temp, *temp2;
  char first[256], first2[256], temp3[256];

  skip_spaces(&name);

  /* Loop through the skills to find a match. */
  while (++index <= TOP_SKILL_DEFINE) {

     if ((should_restrict == SPELL && !IS_SPELL(index)) ||
         (should_restrict == SKILL && !IS_SKILL(index)) ||
         (should_restrict == SONG && !IS_SONG(index)) ||
         (should_restrict == CHANT && !IS_CHANT(index)))
        continue;

    /* Exact match.  This is the skill we're looking for. */
    if (!strcmp(name, skills[index].name))
      return index;

    /*
     * If we found an abbreviated match, we don't want to return its
     * index immediately, in case we find a better match later.
     */
    if (is_abbrev(name, skills[index].name)) {
       if (abbrevmatch == -1 || strcmp(skills[index].name, skills[abbrevmatch].name) == -1) {
         abbrevmatch = index;
         continue;
       }
    }

    /*
     * Check for multiple-word abbreviations.
     */
    ok = 1;
    strcpy(temp3, skills[index].name);
    temp = any_one_arg(temp3, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first2, first))
        ok = 0;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2 && abbrevmatch == -1) {
      abbrevmatch = index;
    }
  }

  return abbrevmatch;
}

int find_skill_num(char *name)
{
   return find_talent_num(name, SKILL);
}

int find_spell_num(char *name)
{
   return find_talent_num(name, SPELL);
}

int find_chant_num(char *name)
{
   return find_talent_num(name, CHANT);
}

int find_song_num(char *name)
{
   return find_talent_num(name, SONG);
}

void improve_skill (struct char_data *ch, int skill)
{
   int percent, maxpercent;
   char skillbuf[MAX_STRING_LENGTH];

   if (IS_NPC(ch))
      return;

   /* Skills don't improve in the PK arena. */
   if (ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
      return;

   percent = GET_ISKILL(ch, skill);
   maxpercent = return_max_skill(ch, skill);
   if (percent <= 0 || percent >= maxpercent)
      return;
   if (number (0, 140) > GET_WIS (ch) + GET_INT (ch))
      return;

   /* These skills improve a bit faster than most. */
   if (skill == SKILL_FIRST_AID ||
         skill == SKILL_BANDAGE ||
         skill == SKILL_DOUSE ||
         skill == SKILL_CHANT)
      percent += number(4, 7);
   else if (skill == SKILL_TAME ||
                skill == SKILL_BASH ||
                skill == SKILL_DISARM ||
                skill == SKILL_SCRIBE ||
                skill == SKILL_SWITCH)
      percent += 2;
   else
      percent++;

   /* returns 1000 for most skills, but caps some others lower */
   if (percent >= maxpercent) {
      SET_SKILL(ch, skill, maxpercent);
      sprintf(skillbuf, "&8You feel about as skilled in %s as possible!&0\r\n",
            skills[skill].name);
   } else {
      SET_SKILL (ch, skill, percent);
      sprintf (skillbuf, "&8You feel your skill in %s improving.\r\n&0",
            skills[skill].name);
   }
   send_to_char (skillbuf, ch);
}

/* Improve a skill as a result of an offensive act against someone.
 *
 * The primary purpose of this function is to avoid improving a skill when
 * it's used against an illusory creature.
 *
 * In some sense it could be allowed, but I wouldn't want Illusionists to
 * be able to summon safe, harmless punching bags to practice on.  Practicing
 * an offensive skill against someone who you *know* cannot harm you is
 * unlikely to be very effective.  You will lack fear and won't incorporate
 * defensive measures into your actions.
 */

void improve_skill_offensively(struct char_data *ch, struct char_data *victim,
      int skill)
{
   /* Note that for some skills, such as switch, there may be no victim. */
   if (victim && MOB_FLAGGED(victim, MOB_ILLUSORY))
      return;
   improve_skill(ch, skill);
}

/* UPDATE_SKILLS
 *
 * Makes sure that the character has the proper skills, spells,
 * songs, and chants.  Should be called whenever someone is created,
 * or logs in, or gains a level, or changes class/race for any reason.
 *
 * It will automatically handle the spell sphere skills, such as
 * SKILL_SPHERE_GENERIC.  If any spell is assigned to a class, the
 * related sphere skill will be assigned.  Conversely, if there are no
 * spells of a given sphere, players in that class will have the
 * sphere-skill removed.
 *
 * The above will work even if additional spheres are introduced,
 * and even if they are non-contiguous with the current ones.
 * In other words, SKILL_SPHERE_NEXUS could be added as number 500.
 * However, the defines IS_SKILL_SPHERE and NUM_SPHERE_SKILLS must
 * correctly take such changes into account.
 *
 * You have the following opportunities to be allowed any skill:
 *
 * 1. Class allows it.
 * 2. Race allows it.
 * 3. Assigned to a mob by vnum: creatures[] in act.other.c (mainly for shapechange).
 *
 * In addition, if a skill is for humanoids only, you will not be allowed
 * to have it even if class would allow it; but the creatures[] array
 * overrides this rule.
 */

void update_skills(struct char_data *ch)
{
   int skill, i, spherecheck[NUM_SPHERE_SKILLS], prof;
   bool spherepresent;

   /* act.other.c: shapechange creatures */
   extern bool creature_allowed_skill(struct char_data *ch, int skill);

   if (!VALID_CLASS(ch))
      return;

   memset(&spherecheck, 0, sizeof(spherecheck));

   for (skill = 0; skill <= TOP_SKILL; ++skill) {

      /* Skills and spells can be divided into three categories here:
       *
       * 1. That you're guaranteed to have - you meet the level requirement
       *    and it isn't a quest spell.
       * 2. That you're guaranteed NOT to have - which is anything for which you
       *    don't meet the level requirement, and there's no special excuse which
       *    would give it to.
       * 3. That you normally wouldn't have, except that a quest or race
       *    allowance might have qualified you for it.
       *
       * For items in category 1, we make sure you have the skill.
       * Category 2 items we make sure you don't have.
       *
       * For spells in category 1 or 3, we make sure you have the related magical
       * sphere skill. For example, the magic missile spell is in the sphere of
       * generic. Any character with that spell, we ensure that he or she has the
       * skill of SKILL_SPHERE_GENERIC.
       */

      if (skills[skill].min_level[(int)GET_CLASS(ch)] <= GET_LEVEL(ch)
            && !(
                  skills[skill].humanoid &&
                     !(IS_HUMANOID(ch) || creature_allowed_skill(ch, skill))
                )) {
         /* This skill is available because of your class, and you meet
          * the humanoid requirement, if any. */

         /* This is a talent that you do have, or could have. */
         if (skills[skill].quest == FALSE) {
            /* This skill/spell you get because your level is high enough.
             * So: ensure that you have it. */
            if (GET_SKILL(ch, skill) <= 0) {
               /* You don't have it, so set the starting value.  Individual
                * spells and languages don't actually improve, so the value
                * is 1000. */
               if (IS_SPELL(skill))
                  SET_SKILL(ch, skill, 1000);
               /* Barehand and safe fall don't improve either, though with
                * some improvements to the mud, they could. */
               else if (skill == SKILL_BAREHAND || skill == SKILL_SAFEFALL)
                  SET_SKILL(ch, skill, 1000);
               else
               /* Skills, chants, and songs do improve. You get the low
                * starting value. */
                  SET_SKILL(ch, skill, !IS_NPC(ch) ? 50 : roll_mob_skill(GET_LEVEL(ch)));
            }
         }

         /* Remember all spells' related sphere skills. */
         if (IS_SPELL(skill)) {
            for (i = 0; i < NUM_SPHERE_SKILLS; i++) {
               if (spherecheck[i] == skills[skill].sphere || !spherecheck[i]) {
                  spherecheck[i] = skills[skill].sphere;
                  break;
               }
            }
         }

      } else if ((prof = racial_skill_proficiency(skill, GET_RACE(ch), GET_LEVEL(ch)))) {
         /* This skill is available because of your race. */
         if (prof == ROLL_SKILL_PROF) {
            /* This skill improves as you gain levels. So we only want to give
             * you a "pre-improved" value if you're just now gaining it -
             * such as a spawned mob, or a person being switched to the race. */
            if (GET_SKILL(ch, skill) == 0)
               SET_SKILL(ch, skill, roll_mob_skill(GET_LEVEL(ch)));
         } else {
            /* This skill has been given a static amount. Probably 1000. */
            SET_SKILL(ch, skill, prof);
         }

         /* Again, remember the spell-sphere skills. */
         if (IS_SPELL(skill)) {
            for (i = 0; i < NUM_SPHERE_SKILLS; i++) {
               if (spherecheck[i] == skills[skill].sphere || !spherecheck[i]) {
                  spherecheck[i] = skills[skill].sphere;
                  break;
               }
            }
         }

      } else if (!IS_SPHERE_SKILL(skill) && skills[skill].quest == FALSE) {
         /* You don't meet the level requirement. Neither race nor questage
          * could give it to you, so you CAN'T have it. We take it away! */
         SET_SKILL(ch, skill, 0);
      }
   }

   /* Now set the sphere skills according to the spells we saw in the previous loop.
    * I've looped over the entire skill set again under the assumption that the
    * sphere-skills might NOT be contiguous in the future. */
   for (skill = 0; skill <= TOP_SKILL; ++skill) {
      if (IS_SPHERE_SKILL(skill)) {
         spherepresent = FALSE;
         for (i = 0; i < NUM_SPHERE_SKILLS; i++) {
            if (spherecheck[i] == skill) {
               spherepresent = TRUE;
               break;
            }
         }
         if (!spherepresent)
            SET_SKILL(ch, skill, 0);
         else if (GET_SKILL(ch, skill) <= 0)
            SET_SKILL(ch, skill, !IS_NPC(ch) ? 50 : roll_mob_skill(GET_LEVEL(ch)));
      }
   }
}

/* Define the skills on boot up */
void dskill(int skill, char *name, int max_mana, int min_mana, int mana_change,
            int minpos, int ok_fighting, int targets, byte violent, bool humanoid,
            int routines, int mem_time, int cast_time, int damage_type, int sphere,
            int pages, int quest, char *wearoff)
{
  int i;

  skills[skill].name = name;

  for (i = 0; i < NUM_CLASSES; i++)
    skills[skill].min_level[i] = LVL_IMMORT;
  skills[skill].lowest_level = LVL_IMMORT;

  skills[skill].mana_max = max_mana;
  skills[skill].mana_min = min_mana;
  skills[skill].mana_change = mana_change;
  skills[skill].fighting_ok = ok_fighting;
  skills[skill].minpos = minpos;
  skills[skill].targets = targets;
  skills[skill].violent = violent || IS_SET(routines, MAG_DAMAGE);
  skills[skill].humanoid = humanoid;
  skills[skill].routines = routines;
  skills[skill].damage_type = damage_type;
  skills[skill].sphere = sphere;
  skills[skill].pages  = pages;
  skills[skill].quest  = quest;
  skills[skill].wearoff = wearoff;

  skills[skill].mem_time = mem_time;
  skills[skill].cast_time = cast_time;
}


void clear_skill(int skill)
{
  int i;

  skills[skill].name = "!UNUSED!";

  for (i = 0; i < NUM_CLASSES; i++)
    skills[skill].min_level[i] = LVL_IMPL + 1;
  skills[skill].lowest_level = LVL_IMPL + 1;

  skills[skill].mana_max = 0;
  skills[skill].mana_min = 0;
  skills[skill].mana_change = 0;
  skills[skill].minpos = 0;
  skills[skill].fighting_ok = FALSE;
  skills[skill].targets = 0;
  skills[skill].violent = FALSE;
  skills[skill].humanoid = FALSE;
  skills[skill].routines = 0;
  skills[skill].damage_type = DAM_UNDEFINED;
  skills[skill].sphere = 0;
  skills[skill].pages = 0;
  skills[skill].quest = FALSE;
  skills[skill].wearoff = NULL;

  skills[skill].mem_time = 0;
  skills[skill].cast_time = 0;

}

#define skillo(skill, name, humanoid, targets) dskill(skill, name, 0, 0, 0, 0, \
      TRUE, targets, 0, humanoid, 0, 0, 0, 0, 0, 0, 0, NULL);
#define chanto(chant, name, minpos, ok_fighting, targets, violent, routines, damage, quest, wearoff) \
     dskill(chant, name, 0, 0, 0, minpos, ok_fighting, targets, violent, FALSE, \
            routines, 0, 0, damage, 0, 0, quest, wearoff)
#define songo(song, name, minpos, ok_fighting, targets, violent, routines, damage, quest, wearoff) \
     dskill(song, name, 0, 0, 0, minpos, ok_fighting, targets, violent, FALSE, \
            routines, 0, 0, damage, 0, 0, quest, wearoff)
#define spello(spl, name, max_mana, min_mana, mana_change, minpos, ok_fighting, \
            targets, violent, routines, mem_time, \
            cast_time, damage_type, sphere, pages, quest, \
            wearoff) \
     dskill(spl, name, max_mana, min_mana, mana_change, minpos, ok_fighting, \
            targets, violent, FALSE, routines, mem_time, \
            cast_time, damage_type, sphere, pages, quest, \
            wearoff)
#define effecto(eff, name, wearoff) \
     dskill(eff, name, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, wearoff)



/* **** Initialization of skills **** */

void init_skills(void)
{
  int i;

  for (i = 0; i <= TOP_SKILL_DEFINE; i++)
    clear_skill(i);

  skills[0].name = "!RESERVED!";

/*
 * Arguments for spello calls:
 *
 * spellnum, name, maxmana, minmana, manachng, minpos, targets, violent?, routines,
 * mem_time, cast_time, damage_type, sphere, pages, quest, wearoff
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL).
 *
 * name    :  Name of the spell
 *
 * maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell).
 *
 * minmana :  The minimum mana this spell will take, no matter how high
 * level the caster is.
 *
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|').
 *
 * violent :  TRUE or FALSE, depending on if this is considered a violent
 * spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 * set on any spell that inflicts damage, is considered aggressive (i.e.
 * charm, curse), or is otherwise nasty.
 *
 * routines:  A list of magic routines which are associated with this spell
 * if the spell uses spell templates.  Also joined with bitwise OR ('|').
 *
 * mem_time:
 *
 * cast_time:
 *
 * damage_type:
 *
 * sphere:
 *
 * pages:  Base Number of pages spell takes to write in a spellbook.
 *
 * quest:  TRUE if spell is a quest spell FALSE if not.
 *
 * wearoff: The message seen when the spell wears off.  NULL if none.
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

  spello(SPELL_BARKSKIN, "barkskin", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_PROT, 17, FALSE,
         "Your skin softens back to its original texture.");

  spello(SPELL_CIRCLE_OF_FIRE, "circle of fire", 0, 0, 0, POS_STANDING, FALSE,
         TAR_IGNORE, TRUE, MAG_ROOM, C1,
         CAST_SPEED6, DAM_FIRE, SKILL_SPHERE_FIRE, 25, FALSE,
         "&1&bThe &1&bfl&3am&1es&0 &1surrounding &1the area &9&bdie out&0.");

  spello(SPELL_GAIAS_CLOAK, "cloak of gaia", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_OUTDOORS, FALSE, MAG_AFFECT, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, FALSE,
         "Your shroud of nature dissolves.");

  spello(SPELL_ACID_BURST, "acid burst", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDG, DAM_ACID, SKILL_SPHERE_GENERIC, 23, FALSE, NULL);

  spello(SPELL_DETONATION, "detonation", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_GROUND | TAR_OUTDOORS,
         TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDE, DAM_CRUSH, SKILL_SPHERE_EARTH, 10, FALSE, NULL);

  spello(SPELL_CONFUSION, "confusion", 0, 0, 0, POS_STANDING, TRUE,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECT, C1,
         CAST_SPEED4, DAM_MENTAL, SKILL_SPHERE_ENCHANT, 19, FALSE,
         "You no longer feel so confused.");

  spello(SPELL_CREATE_SPRING, "create spring", 0, 0, 0, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_CREATION, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, FALSE, NULL);

  spello(SPELL_CREMATE, "cremate", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C1,
         CAST_SPEED7, DAM_FIRE, SKILL_SPHERE_FIRE, 35, FALSE, NULL);

  spello(SPELL_DARK_FEAST, "dark feast", 0, 0, 0, POS_STANDING, FALSE,
         TAR_OBJ_ROOM | TAR_CONTACT, FALSE, MAG_MANUAL, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_GENERIC, 5, FALSE, NULL);

  spello(SPELL_DARK_PRESENCE, "dark presence", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_AFFECT | MAG_ALTER_OBJ, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "You feel the dark presence leave you.");

  spello(SPELL_DEMONIC_ASPECT, "demonic aspect", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "The demon within you fades away.");

  spello(SPELL_DEMONIC_MUTATION, "demonic mutation", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "You mutate back to your original form.");

  spello(SPELL_DEMONSKIN, "demonskin", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, FALSE,
         "Your skin reverts back to normal.");

  spello(SPELL_DISCORPORATE, "discorporate", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDE, DAM_DISCORPORATE, SKILL_SPHERE_GENERIC, 14, FALSE, NULL);

  spello(SPELL_DISEASE, "disease", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "You are cured of your disease!");

  spello(SPELL_DIVINE_ESSENCE, "divine essence", 0, 0, 0, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_GROUP, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE, NULL);

  spello(SPELL_ENTANGLE, "entangle", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OUTDOORS, FALSE, MAG_AFFECT, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "You break free of the vines.");

  spello(SPELL_EXTINGUISH, "extinguish", 0, 0, 0, POS_SITTING, TRUE,
         TAR_CHAR_ROOM, FALSE, MAG_UNAFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_WATER, 11, FALSE, NULL);

  spello(SPELL_FAMILIARITY, "familiarity", 0, 0, 0, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 10, FALSE,
         "Your familiar disguise melts away.");

  spello(SPELL_FIRE_DARTS, "fire darts", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_MANUAL, C1,
         CAST_SPEEDD, DAM_FIRE, SKILL_SPHERE_FIRE, 9, FALSE, NULL);

  spello(SPELL_FIRESHIELD, "fireshield", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_FIRE, SKILL_SPHERE_FIRE, 17, FALSE,
         "The flames around your body dissipate.");

  spello(SPELL_FIRESTORM, "firestorm", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C1,
         CAST_SPEED6, DAM_FIRE, SKILL_SPHERE_FIRE, 25, FALSE, NULL);

  spello(SPELL_FLAME_BLADE, "flame blade", 0, 0, 0, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_MANUAL, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 19, FALSE, NULL);

  spello(SPELL_FLOOD, "flood", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_MANUAL, C1,
         18, DAM_WATER, SKILL_SPHERE_WATER, 35, TRUE, NULL);

  spello(SPELL_FREEZE, "freeze", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDE, DAM_COLD, SKILL_SPHERE_WATER, 25, FALSE, NULL);

  spello(SPELL_FREEZING_WIND, "freezing wind", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C1,
         CAST_SPEED4, DAM_COLD, SKILL_SPHERE_AIR, 21, FALSE, NULL);

  spello(SPELL_ARMOR_OF_GAIA, "armor of gaia", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, FALSE, NULL);

  spello(SPELL_GLORY, "glory", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 17, FALSE,
         "Your visage becomes plain once again.");

  spello(SPELL_GREATER_VITALITY, "greater vitality", 50, 30, 5, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 11, FALSE,
         "Your magical vitality drains away.");

  spello(SPELL_POSITIVE_FIELD, "positive field", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_GROUND, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDF, DAM_SHOCK, SKILL_SPHERE_EARTH, 18, FALSE, NULL);

  spello(SPELL_PHOSPHORIC_EMBERS, "phosphoric embers", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_MANUAL, C1,
         CAST_SPEEDE, DAM_FIRE, SKILL_SPHERE_FIRE, 18, FALSE, NULL);

  spello(SPELL_HEAVENS_GATE, "heavens gate", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_WORLD, FALSE, MAG_MANUAL, C1,
         16, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, TRUE, NULL);

  spello(SPELL_HELL_BOLT, "hell bolt", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED2, DAM_FIRE, SKILL_SPHERE_FIRE, 5, FALSE, NULL);

  spello(SPELL_HELLS_GATE, "hell gate", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_WORLD, FALSE, MAG_MANUAL, C1,
         18, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, TRUE, NULL);

  spello(SPELL_HELLFIRE_BRIMSTONE, "hellfire and brimstone", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, FALSE, MAG_AREA, C1,
         16, DAM_FIRE, SKILL_SPHERE_FIRE, 5, TRUE, NULL);

  spello(SPELL_HYSTERIA, "hysteria", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_MANUAL, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 16, FALSE,
         "Your courage returns to you.");

  spello(SPELL_ICE_ARMOR, "ice armor", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_PROT, 13, FALSE,
         "Your iced encasing melts away, leaving you vulnerable again.");

  spello(SPELL_ICE_DAGGER, "ice dagger", 0, 0, 0, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_MANUAL, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 17, FALSE, NULL);

  spello(SPELL_ICE_DARTS, "ice darts", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_MANUAL, C1,
         CAST_SPEEDD, DAM_COLD, SKILL_SPHERE_WATER, 7, FALSE, NULL);

  spello(SPELL_ICEBALL, "iceball", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDF, DAM_COLD, SKILL_SPHERE_WATER, 29, FALSE, NULL);

  spello(SPELL_ILLUMINATION, "illumination", 50, 25, 5, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE,
         MAG_MANUAL, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "&8The magical light fades away.&0");

  spello(SPELL_ILLUSORY_WALL, "illusory wall", 0, 0, 0, POS_STANDING, FALSE,
         TAR_STRING, FALSE, MAG_MANUAL, C1,
         18, DAM_UNDEFINED, SKILL_SPHERE_GENERIC, 27, TRUE,
         "The wall dissolves into tiny motes of light...");

  spello(SPELL_IMMOLATE, "immolate", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_MANUAL, C1,
         CAST_SPEEDF, DAM_FIRE, SKILL_SPHERE_FIRE, 25, FALSE, NULL);

  spello(SPELL_INSANITY, "insanity", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECT, C1,
         CAST_SPEED5, DAM_MENTAL, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "Your mind returns to reality.");

  spello(SPELL_INVIGORATE, "invigorate", 0, 0, 0, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_GROUP, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_ISOLATION, "isolation", 0, 0, 0, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_MANUAL, C1,
         10, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 8, FALSE,
         "It is as if a veil has lifted from the surrounding area.");

  spello(SPELL_NATURES_EMBRACE, "natures embrace", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_OUTDOORS, FALSE, MAG_AFFECT | MAG_POINT, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "Nature releases you from her embrace.");

  spello(SPELL_NIGHTMARE, "nightmare", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDD, DAM_MENTAL, SKILL_SPHERE_DEATH, 12, FALSE, NULL);

  spello(SPELL_MAGIC_TORCH, "magic torch", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 7, FALSE,
         "Your magic torch peters out.");

  spello(SPELL_MELT, "melt", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         C1, CAST_SPEEDE, DAM_FIRE, SKILL_SPHERE_FIRE, 29, FALSE, NULL);

  spello(SPELL_MESMERIZE, "mesmerize", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, TRUE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_MENTAL, SKILL_SPHERE_ENCHANT, 8, FALSE,
         "You regain your senses.");

  spello(SPELL_METEORSWARM, "meteorswarm", 100, 50, 3, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C9,
         CAST_SPEED7, DAM_CRUSH, SKILL_SPHERE_EARTH, 37, TRUE, NULL);

  spello(SPELL_MIRAGE, "mirage", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_FIRE, 17, FALSE,
         "You become more visible as the heat around your body dies out.");

  spello(SPELL_NEGATE_COLD, "negate cold", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_PROT, 29, FALSE,
         "You feel vulnerable to the cold again.");

  spello(SPELL_NEGATE_HEAT, "negate heat", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_PROT, 29, FALSE,
         "Your immunity to heat has passed.");

  spello(SPELL_NIGHT_VISION, "night vision", 0, 0, 0, POS_SITTING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "Your night vision fades out.");

  spello(SPELL_NOURISHMENT, "nourishment", 0, 0, 0, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_POINT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_PHANTASM, "phantasm", 0, 0, 0, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_SUMMON, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 12, FALSE, NULL);

  spello(SPELL_RAIN, "rain", 0, 0, 0, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_MANUAL, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_WATER, 23, FALSE, NULL);

  spello(SPELL_RECALL, "recall", 20, 10, 2, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL, C1,
         1, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, FALSE, NULL);

  spello(SPELL_SANE_MIND, "sane mind", 0, 0, 0, POS_SITTING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_UNAFFECT, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_SEVERANCE, "severance", 100, 50, 3, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C9,
         CAST_SPEED7, DAM_DISCORPORATE, SKILL_SPHERE_GENERIC, 37, FALSE, NULL);

  spello(SPELL_SIMULACRUM, "simulacrum", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_WORLD, FALSE, MAG_SUMMON, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 15, FALSE, NULL);

  spello(SPELL_SMOKE, "smoke", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_FIRE, 11, FALSE,
         "As the smoke clears, your vision returns.");

  spello(SPELL_SOUL_REAVER, "soul reaver", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C9,
         CAST_SPEED7, DAM_MENTAL, SKILL_SPHERE_DEATH, 30, FALSE, NULL);

  spello(SPELL_STYGIAN_ERUPTION, "stygian eruption", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED4, DAM_FIRE, SKILL_SPHERE_FIRE, 5, FALSE, NULL);

  spello(SPELL_SUNRAY, "sunray", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE | MAG_AFFECT,
         C1, CAST_SPEED6, DAM_FIRE, SKILL_SPHERE_FIRE, 5, FALSE,
         "Your vision has returned.");

  spello(SPELL_SUPERNOVA, "supernova", 100, 50, 3, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C1,
         16, DAM_FIRE, SKILL_SPHERE_FIRE, 31, TRUE, NULL);

  spello(SPELL_URBAN_RENEWAL, "urban renewal", 0, 0, 0, POS_SITTING, FALSE,
         TAR_IGNORE | TAR_OUTDOORS, FALSE, MAG_ROOM, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "&2The woods in the surrounding area break apart and crumble.&0");

  spello(SPELL_VAPORFORM, "vaporform", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_PROT, 35, TRUE,
         "Your form condenses into flesh once again.");

  spello(SPELL_VENTRILOQUATE, "ventriloquate", 0, 0, 0, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_GENERIC, 6, FALSE, NULL);

  spello(SPELL_WALL_OF_ICE, "wall of ice", 0, 0, 0, POS_STANDING, FALSE,
         TAR_STRING, FALSE, MAG_MANUAL, C1,
         18, DAM_COLD, SKILL_SPHERE_WATER, 27, TRUE,
         "The wall of ice melts away...");

  spello(SPELL_WALL_OF_STONE, "wall of stone", 0, 0, 0, POS_STANDING, FALSE,
         TAR_STRING, FALSE, MAG_MANUAL, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 25, FALSE,
         "The wall of stone crumbles into dust.");

  spello(SPELL_WANDERING_WOODS, "wandering woods", 0, 0, 0, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_MANUAL, C1,
         16, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "The woods around you shift back to their proper form.");

  spello(SPELL_WATERFORM, "waterform", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_WATER, 27, TRUE,
         "Your form solidifies into flesh once again.");

  spello(SPELL_WINGS_OF_HELL, "wings of hell", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "Your giant bat-like wings fold up and vanish.");

  spello(SPELL_WRITHING_WEEDS, "writhing weeds", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OUTDOORS, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED3, DAM_CRUSH, SKILL_SPHERE_GENERIC, 5, FALSE, NULL);

  /* SORTED */

  spello(SPELL_WORD_OF_COMMAND, "word of command", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_MANUAL, C1,
         CAST_SPEED7, DAM_MENTAL, SKILL_SPHERE_GENERIC, 5, TRUE, NULL);

  spello(SPELL_WINGS_OF_HEAVEN, "wings of heaven", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "Your wings gently fold back and fade away.");

  spello(SPELL_BANISH, "banish", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_MANUAL,
         C1, CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, TRUE, NULL);

  spello(SPELL_ENLIGHTENMENT, "enlightenment", 0, 0, 0, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 5, FALSE, NULL);

  spello(SPELL_SPEAK_IN_TONGUES, "speak in tongues", 0, 0, 0, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 5, FALSE,
         "Your vocabulary diminishes drastically.");

  spello(SPELL_EXORCISM, "exorcism", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,C1,
         18, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, FALSE, NULL);

  spello(SPELL_LESSER_EXORCISM, "lesser exorcism", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED5, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, FALSE, NULL);

  spello(SPELL_DIVINE_RAY, "divine ray", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED5, DAM_ALIGN, SKILL_SPHERE_GENERIC, 5, FALSE, NULL);

  spello(SPELL_DIVINE_BOLT, "divine bolt", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED3, DAM_ALIGN, SKILL_SPHERE_GENERIC, 5, FALSE, NULL);

  spello(SPELL_PRAYER, "prayer", 0, 0, 0, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "Your holy prayer fades.");

  spello(SPELL_ELEMENTAL_WARDING, "elemental warding", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, FALSE,
         "You feel less safe from the elements.");

  spello(SPELL_CIRCLE_OF_LIGHT, "circle of light", 0, 0, 0, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "The circle of light above you fades out.");

  spello(SPELL_FIRE_BREATH, "fire breath", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED1, DAM_FIRE, SKILL_SPHERE_FIRE, 5, FALSE, NULL);

  spello(SPELL_GAS_BREATH, "gas breath", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, TRUE, MAG_DAMAGE | MAG_AFFECT, C1,
         CAST_SPEED1, DAM_POISON, SKILL_SPHERE_AIR, 5, FALSE, NULL);

  spello(SPELL_FROST_BREATH, "frost breath", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED1, DAM_COLD, SKILL_SPHERE_WATER, 5, FALSE, NULL);

  spello(SPELL_LIGHTNING_BREATH, "lightning breath", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, TRUE, MAG_DAMAGE | MAG_MANUAL, C1,
         CAST_SPEED1, DAM_SHOCK, SKILL_SPHERE_AIR, 5, FALSE, NULL);

  spello(SPELL_ACID_BREATH, "acid breath", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED1, DAM_ACID, SKILL_SPHERE_FIRE, 5, FALSE, NULL);

  spello(SPELL_FEAR, "fear", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "Your courage returns to you.");

  spello(SPELL_MISDIRECTION, "misdirection", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 12, FALSE,
         "You no longer feel like you're going every which way at once.");

  spello(SPELL_CHAIN_LIGHTNING, "chain lightning", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE | TAR_DIRECT, TRUE, MAG_AREA, C1,
         CAST_SPEED5, DAM_SHOCK, SKILL_SPHERE_AIR, 27, FALSE, NULL);

  spello(SPELL_MASS_INVIS, "mass invisibility", 0, 0, 0, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_BULK_OBJS | MAG_MASS, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 23, FALSE, NULL);

  spello(SPELL_RELOCATE, "relocate", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, C14,
         20, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 35, TRUE, NULL);

  spello(SPELL_DISINTEGRATE, "disintegrate", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE | MAG_MANUAL,
         C1, CAST_SPEEDF, DAM_ACID, SKILL_SPHERE_GENERIC, 27, FALSE, NULL);

  spello(SPELL_HARNESS, "harness", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         2, DAM_UNDEFINED, SKILL_SPHERE_GENERIC, 21, FALSE,
         "&4The harnessed power in your body fades.&0");

  spello(SPELL_MINOR_GLOBE, "minor globe", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_PROT, 23, FALSE,
         "The globe around your body fades out.");

  spello(SPELL_MAJOR_GLOBE, "major globe", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_PROT, 35, TRUE,
         "The globe of force surrounding you dissipates.");

  spello(SPELL_COLDSHIELD, "coldshield", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_COLD, SKILL_SPHERE_WATER, 17, FALSE,
         "The ice formation around your body melts.");

  spello(SPELL_MINOR_PARALYSIS, "minor paralysis", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 21, FALSE,
         "Your muscles regain feeling.");

  spello(SPELL_TELEPORT, "teleport", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 11, FALSE, NULL);

  spello(SPELL_RAY_OF_ENFEEB, "ray of enfeeblement", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 21, FALSE,
         "Your strength returns to you.");

  spello(SPELL_LEVITATE, "levitate", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 13, FALSE,
         "You float back to the ground.");

  spello(SPELL_WIZARD_EYE, "wizard eye", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_WORLD, FALSE, MAG_MANUAL, C1,
         16, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 17, TRUE, NULL);

  spello(SPELL_MINOR_CREATION, "minor creation", 0, 0, 0,POS_SITTING, FALSE,
         TAR_STRING, FALSE, MAG_MANUAL, C1,
         CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, FALSE, NULL);

  spello(SPELL_DISPEL_MAGIC, "dispel magic", 0, 0, 0, POS_SITTING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_ROOM | TAR_OBJ_INV,
         TRUE, MAG_MANUAL, C1, CAST_SPEED4, DAM_DISPEL,
         SKILL_SPHERE_GENERIC, 15, FALSE, NULL);

  spello(SPELL_UNHOLY_WORD, "unholy word", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C1,
         1, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, FALSE, NULL);

  spello(SPELL_HOLY_WORD, "holy word", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C1,
         1, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, FALSE, NULL);

  spello(SPELL_ARMOR, "armor", 30, 15, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, FALSE,
         "You feel less protected.");

  spello(SPELL_REDUCE, "reduce", 35, 5, 3, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT | MAG_UNAFFECT, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 29, FALSE,
         "&8You return to your normal size.&0");

  spello(SPELL_ENLARGE, "enlarge", 35, 5, 3, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT | MAG_UNAFFECT, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 29, FALSE,
         "&8You return to your normal size.&0");

  spello(SPELL_BLESS, "bless", 35, 5, 3, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_AFFECT | MAG_ALTER_OBJ,
         C1, CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "You feel less righteous.");

  spello(SPELL_BLINDNESS, "blindness", 35, 25, 1, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, TRUE,
         MAG_AFFECT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "You feel a cloak of blindness dissolve.");

  spello(SPELL_SILENCE, "silence", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, TRUE, MAG_AFFECT,
         C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "You can speak again.");

  spello(SPELL_FLAMESTRIKE, "flamestrike", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED3, DAM_FIRE, SKILL_SPHERE_FIRE, 5, FALSE, NULL);

  spello(SPELL_BURNING_HANDS, "burning hands", 30, 10, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDD, DAM_FIRE, SKILL_SPHERE_FIRE, 5, FALSE, NULL);

  spello(SPELL_CONE_OF_COLD, "cone of cold", 35, 15, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDE, DAM_COLD, SKILL_SPHERE_WATER, 19, FALSE, NULL);

  spello(SPELL_BIGBYS_CLENCHED_FIST, "bigbys clenched fist", 90, 35, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDE, DAM_CRUSH, SKILL_SPHERE_GENERIC, 25, FALSE, NULL);

  spello(SPELL_WATERWALK, "waterwalk", 35, 5, 3, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 29, FALSE,
         "Your feet seem less buoyant.");

  spello(SPELL_ICE_SHARDS, "ice shards", 100, 50, 3, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C1,
         16, DAM_SLASH, SKILL_SPHERE_WATER, 31, TRUE, NULL);

  spello(SPELL_ICE_STORM, "ice storm", 100, 50, 3, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C1,
         CAST_SPEED5, DAM_COLD, SKILL_SPHERE_WATER, 23, FALSE, NULL);

  spello(SPELL_CALL_LIGHTNING, "call lightning", 40, 25, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OUTDOORS, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED4, DAM_SHOCK, SKILL_SPHERE_AIR, 5, FALSE, NULL);

  spello(SPELL_CHARM, "charm person", 75, 50, 2, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, C1,
         CAST_SPEED5, DAM_MENTAL, SKILL_SPHERE_ENCHANT, 35, TRUE,
         "You feel more self-confident.");

  spello(SPELL_PLANE_SHIFT, "plane shift", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, TRUE, NULL);

  spello(SPELL_CHILL_TOUCH, "chill touch", 30, 10, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, TRUE, MAG_DAMAGE | MAG_AFFECT,
         C1, CAST_SPEEDD, DAM_COLD, SKILL_SPHERE_WATER, 7, FALSE,
         "You feel your strength return.");

  spello(SPELL_CLONE, "clone", 80, 65, 5, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_SUMMON, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 35, FALSE, NULL);

  spello(SPELL_DARKNESS, "darkness", 50, 25, 5, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_MANUAL, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "The magical darkness lifts.");

  /* innate strength */
  spello(SPELL_INN_STRENGTH, "innate strength", 35, 30, 1, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, 0, 0, 7, FALSE,
         "You feel weaker.");

  spello(SPELL_MAJOR_PARALYSIS, "major paralysis", 35, 30, 1, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, TRUE, MAG_MANUAL, C6,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 35, FALSE,
         "You can move again.");

  spello(SPELL_COLOR_SPRAY, "color spray", 30, 15, 3, POS_STANDING, TRUE,
         TAR_IGNORE | TAR_DIRECT, TRUE, MAG_MANUAL, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_GENERIC, 21, FALSE, NULL);

  spello(SPELL_CONTROL_WEATHER, "control weather", 75, 25, 5, POS_STANDING, FALSE,
         TAR_STRING, FALSE, MAG_MANUAL, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE, NULL);

  spello(SPELL_CREATE_FOOD, "create food", 30, 5, 4, POS_SITTING, FALSE,
         TAR_IGNORE, FALSE, MAG_CREATION, C1,
         CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, FALSE, NULL);

  spello(SPELL_CREATE_WATER, "create water", 30, 5, 4, POS_SITTING, FALSE,
         TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL, C1,
         CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, FALSE, NULL);

  spello(SPELL_CURE_BLIND, "cure blind", 30, 5, 2, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_UNAFFECT, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_CURE_CRITIC, "cure critic", 30, 10, 2, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, FALSE, MAG_POINT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 15, FALSE, NULL);

  spello(SPELL_CURE_SERIOUS, "cure serious", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, FALSE, MAG_POINT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_CURE_LIGHT, "cure light", 30, 10, 2, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, FALSE, MAG_POINT, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 9, FALSE, NULL);

  spello(SPELL_VIGORIZE_LIGHT, "vigorize light", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_POINT, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_VIGORIZE_SERIOUS, "vigorize serious", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_POINT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_VIGORIZE_CRITIC, "vigorize critic", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_POINT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_CURSE, "curse", 80, 50, 2, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, TRUE,
         MAG_AFFECT | MAG_ALTER_OBJ, C1, CAST_SPEED5, 0,
         SKILL_SPHERE_ENCHANT, 5, FALSE,
         "You feel more optimistic.");

  spello(SPELL_DETECT_ALIGN, "detect alignment", 20, 10, 2, POS_SITTING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 9, FALSE,
         "You feel less aware.");

  spello(SPELL_DETECT_INVIS, "detect invisibility", 20, 10, 2, POS_SITTING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 19, FALSE,
         "Your eyes stop tingling.");

  spello(SPELL_DETECT_MAGIC, "detect magic", 20, 10, 2, POS_SITTING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 5, FALSE,
         "The detect magic wears off.");

  spello(SPELL_FLY, "fly", 50, 5, 3, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 17, FALSE,
         "You feel the weight of your body return.");

  spello(SPELL_DETECT_POISON, "detect poison", 15, 5, 1, POS_SITTING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 13, FALSE,
         "You feel slightly less aware.");

  spello(SPELL_DISPEL_EVIL, "dispel evil", 40, 25, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED3, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, FALSE, NULL);

  spello(SPELL_DISPEL_GOOD, "dispel good", 40, 25, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED5, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, FALSE, NULL);

  spello(SPELL_EARTHQUAKE, "earthquake", 40, 25, 3, POS_STANDING, TRUE,
         TAR_IGNORE | TAR_OUTDOORS, TRUE, MAG_AREA, C1,
         CAST_SPEED5, DAM_CRUSH, SKILL_SPHERE_EARTH, 5, FALSE, NULL);

  spello(SPELL_DOOM, "doom", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C1,
         CAST_SPEED6, DAM_CRUSH, SKILL_SPHERE_EARTH, 35, FALSE, NULL);

  spello(SPELL_CREEPING_DOOM, "creeping doom", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_MANUAL, C1,
         CAST_SPEED7, DAM_CRUSH, SKILL_SPHERE_EARTH, 35, TRUE, NULL);

  spello(SPELL_ENCHANT_WEAPON, "enchant weapon", 150, 100, 10, POS_STANDING, FALSE,
         TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 23, FALSE, NULL);

  spello(SPELL_ENERGY_DRAIN, "energy drain", 40, 25, 1, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL, C1,
         CAST_SPEED5, DAM_HEAL, SKILL_SPHERE_DEATH, 19, FALSE, NULL);

  spello(SPELL_VAMPIRIC_BREATH, "vampiric breath", 40, 25, 1, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE | MAG_MANUAL, C1,
         CAST_SPEED1, DAM_HEAL, SKILL_SPHERE_DEATH, 5, FALSE, NULL);

  spello(SPELL_GROUP_ARMOR, "group armor", 50, 30, 2, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_GROUP, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_PROT, 9, TRUE, NULL);

  spello(SPELL_GROUP_RECALL, "group recall", 50, 30, 2, POS_STANDING, TRUE,
         TAR_IGNORE, FALSE, MAG_GROUP, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, TRUE, NULL);

  spello(SPELL_FIREBALL, "fireball", 40, 30, 2, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDE, DAM_FIRE, SKILL_SPHERE_FIRE, 21, FALSE, NULL);

  spello(SPELL_FULL_HARM, "full harm", 75, 45, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED6, DAM_HEAL, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_FULL_HEAL, "full heal", 75, 50, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, FALSE, MAG_POINT | MAG_UNAFFECT, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_GROUP_HEAL, "group heal", 80, 60, 5, POS_STANDING, TRUE,
         TAR_IGNORE, FALSE, MAG_GROUP, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, TRUE, NULL);

  spello(SPELL_HARM, "harm", 45, 15, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED5, DAM_HEAL, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_HEAL, "heal", 60, 40, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, FALSE, MAG_POINT | MAG_UNAFFECT, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_INFRAVISION, "infravision", 25, 10, 1, POS_SITTING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 9, FALSE,
         "Your night vision seems to fade.");

  spello(SPELL_INVISIBLE, "invisibility", 35, 25, 1, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_AFFECT |
         MAG_ALTER_OBJ, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 17, FALSE,
         "You fade back into view.");

  spello(SPELL_CONCEALMENT, "concealment", 0, 0, 0, POS_SITTING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_POINT, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 13, FALSE,
         "You fade back into view.");

  spello(SPELL_LIGHTNING_BOLT, "lightning bolt", 30, 15, 1, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDD, DAM_SHOCK, SKILL_SPHERE_AIR, 17, FALSE, NULL);

  spello(SPELL_LOCATE_OBJECT, "locate object", 25, 20, 1, POS_SITTING, FALSE,
         TAR_STRING, FALSE, MAG_MANUAL, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 12, FALSE, NULL);

  spello(SPELL_SUMMON_CORPSE, "summon corpse", 25, 20, 1, POS_STANDING, FALSE,
         TAR_STRING, FALSE, MAG_MANUAL, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 17, FALSE, NULL);

  spello(SPELL_SHIFT_CORPSE, "shift corpse", 25, 20, 1, POS_STANDING, FALSE,
         TAR_STRING, FALSE, MAG_MANUAL, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 17, TRUE, NULL);

  spello(SPELL_MAGIC_MISSILE, "magic missile", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_MANUAL, C1,
         CAST_SPEEDD, DAM_PIERCE, SKILL_SPHERE_GENERIC, 5, FALSE, NULL);

  spello(SPELL_POISON, "poison", 50, 20, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV | TAR_FIGHT_VICT, TRUE,
         MAG_AFFECT | MAG_ALTER_OBJ, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "You feel less sick.");

  spello(SPELL_PROT_FROM_EVIL, "protection from evil", 40, 10, 3, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, FALSE,
         "You feel less protected.");

  spello(SPELL_REMOVE_CURSE, "remove curse", 45, 25, 5, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
         C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE, NULL);

  spello(SPELL_SANCTUARY, "sanctuary", 110, 85, 5, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_PROT, 35, FALSE,
         "The white aura around your body fades.");

  spello(SPELL_SHOCKING_GRASP, "shocking grasp", 30, 15, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDD, DAM_SHOCK, SKILL_SPHERE_AIR, 9, FALSE, NULL);

  spello(SPELL_SLEEP, "sleep", 40, 25, 5, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED4, DAM_MENTAL, SKILL_SPHERE_ENCHANT, 19, FALSE,
         "You feel less tired.");

  spello(SPELL_STRENGTH, "strength", 35, 30, 1, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "You feel weaker.");

  spello(SPELL_SUMMON, "summon", 75, 50, 3, POS_STANDING, FALSE,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, FALSE, NULL);

  spello(SPELL_RESURRECT, "resurrect", 75, 50, 3, POS_STANDING, FALSE,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, C1,
         26, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, TRUE, NULL);

  spello(SPELL_PRESERVE, "preserve", 0, 0, 0, POS_SITTING, FALSE,
         TAR_OBJ_ROOM, FALSE, MAG_MANUAL, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 11, FALSE, NULL);

  spello(SPELL_DIMENSION_DOOR, "dimension door", 75, 45, 3, POS_STANDING, FALSE,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 15, FALSE, NULL);

  spello(SPELL_SUMMON_DRACOLICH, "summon dracolich", 100, 75, 3, POS_STANDING, FALSE,
         TAR_OBJ_ROOM, FALSE, MAG_SUMMON, C1,
         20, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 33, TRUE, NULL);

  spello(SPELL_ANIMATE_DEAD, "animate dead", 75, 15, 3, POS_STANDING, FALSE,
         TAR_OBJ_ROOM, FALSE, MAG_SUMMON, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DEATH, 21, FALSE, NULL);

  spello(SPELL_SUMMON_ELEMENTAL, "summon elemental", 75, 15, 3, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_SUMMON, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 19, FALSE, NULL);

  spello(SPELL_SUMMON_DEMON, "summon demon", 100, 75, 3, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_SUMMON, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 31, FALSE, NULL);

  spello(SPELL_SUMMON_GREATER_DEMON, "summon greater demon", 130, 75, 3, POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_SUMMON, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 35, FALSE, NULL);

  spello(SPELL_WORD_OF_RECALL, "word of recall", 20, 10, 2, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL, C1,
         1, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, FALSE, NULL);

  spello(SPELL_REMOVE_POISON, "remove poison", 40, 8, 4, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_UNAFFECT
         | MAG_ALTER_OBJ, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_INCENDIARY_NEBULA, "incendiary nebula", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_AREA, C1, 15, DAM_FIRE,
         SKILL_SPHERE_FIRE, 35, FALSE, NULL);

  spello(SPELL_MOONWELL, "moonwell", 50, 50, 0, POS_STANDING, FALSE,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, C1,
         18, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, TRUE, NULL);

  spello(SPELL_SENSE_LIFE, "sense life", 20, 10, 2, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 19, FALSE,
         "You feel less aware of your surroundings.");

  spello(SPELL_SOULSHIELD, "soulshield", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, FALSE,
         "The aura guarding your body fades away.");

  spello(SPELL_DESTROY_UNDEAD, "destroy undead", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED3, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, FALSE, NULL);

  spello(SPELL_STONE_SKIN, "stone skin", 50, 25, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_PROT, 19, FALSE,
         "&3&dYour skin softens and returns to normal.&0");

  spello(SPELL_HASTE, "haste", 50, 25, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 19, FALSE,
         "Your pulse returns to normal.");

  spello(SPELL_BLUR, "blur", 90, 60, 3, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 25, TRUE,
         "The world solidifies as your vision unblurs, and you return to normal speed.");

  spello(SPELL_LESSER_ENDURANCE, "lesser endurance", 50, 30, 5, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, FALSE,
         "Your endurance returns to normal.");

  spello(SPELL_ENDURANCE, "endurance", 50, 30, 5, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 7, FALSE,
         "Your endurance returns to normal.");

  spello(SPELL_GREATER_ENDURANCE, "greater endurance", 50, 30, 5, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 11, FALSE,
         "Your endurance returns to normal.");

  spello(SPELL_VITALITY, "vitality", 50, 30, 5, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 7, FALSE,
         "Your magical vitality drains away.");

  spello(SPELL_DRAGONS_HEALTH, "dragons health", 50, 30, 5, POS_STANDING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 21, TRUE,
         "Your health returns to normal.");

  spello(SPELL_WALL_OF_FOG, "wall of fog", 50, 25, 5, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_ROOM, C1,
         CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 21, FALSE,
         "The fog seems to clear out.");

  spello(SPELL_FARSEE, "farsee", 25, 10, 1, POS_SITTING, FALSE,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECT, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 9, FALSE,
         "Your pupils dilate as your vision returns to normal.");

  spello(SPELL_CAUSE_LIGHT, "cause light", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED2, DAM_HEAL, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_CAUSE_SERIOUS, "cause serious", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED3, DAM_HEAL, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_CAUSE_CRITIC, "cause critical", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEED4, DAM_HEAL, SKILL_SPHERE_HEALING, 5, FALSE, NULL);

  spello(SPELL_IDENTIFY, "identify", 0, 0, 0, POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_CONTACT, FALSE, MAG_MANUAL, C1,
         CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 5, FALSE, NULL);

 spello(SPELL_BONE_ARMOR, "bone armor", 0, 0, 0, POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_PROT, 12, FALSE,
         "&3Your skin returns to normal.&0");

  spello(SPELL_DEGENERATION, "degeneration", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, MAG_MANUAL, C1,
         CAST_SPEED4, DAM_HEAL, SKILL_SPHERE_DEATH, 12, TRUE, NULL);

  spello(SPELL_SOUL_TAP, "soul tap", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, TRUE, MAG_MANUAL, C1,
         CAST_SPEEDF, DAM_HEAL, SKILL_SPHERE_DEATH, 22, FALSE, NULL);

  spello(SPELL_REBUKE_UNDEAD, "rebuke undead", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECT, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 8, FALSE, NULL);

  spello(SPELL_DECAY, "decay", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDD, DAM_HEAL, SKILL_SPHERE_DEATH, 5, FALSE, NULL);

  spello(SPELL_SPINECHILLER, "spinechiller", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, TRUE, MAG_AFFECT, C1,
         CAST_SPEEDE, DAM_UNDEFINED, SKILL_SPHERE_DEATH, 10, FALSE,
         "The tingling in your spine subsides.");

  spello(SPELL_PYRE, "pyre", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, FALSE, MAG_MANUAL, C1,
         CAST_SPEEDD, DAM_FIRE, SKILL_SPHERE_FIRE, 15, FALSE,
         "The flames enveloping you die down.");

  spello(SPELL_IRON_MAIDEN, "iron maiden", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, C1,
         CAST_SPEEDE, DAM_PIERCE, SKILL_SPHERE_DEATH, 14, FALSE, NULL);

  spello(SPELL_FRACTURE, "fracture", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE, FALSE, MAG_MANUAL, C1,
         CAST_SPEEDE, DAM_SLASH, SKILL_SPHERE_GENERIC, 17, FALSE, NULL);

  spello(SPELL_BONE_DRAW, "bone draw", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECT, C1,
         CAST_SPEED2, DAM_SLASH, SKILL_SPHERE_SUMMON, 16, FALSE,
         "The bones holding you down crumble to dust.");

  spello(SPELL_NATURES_GUIDANCE, "natures guidance", 0, 0, 0, POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, C1,
         CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 8, FALSE,
         "You suddenly feel a little unguided.");

  spello(SPELL_MOONBEAM, "moonbeam", 0, 0, 0, POS_STANDING, TRUE,
         TAR_IGNORE | TAR_OUTDOORS | TAR_NIGHT_ONLY, TRUE, MAG_MANUAL, C1,
         CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_WATER, 10, FALSE, NULL);

  /* Declaration of skills - sets skills up so that immortals can use
   * them by default. Determines whether a skill is considered
   * "humanoid only". */

  /* skillo(skill, name, humanoid, target mode) */
  /* target mode for skills is designed for use with TAR_CONTACT and TAR_DIRECT,
     which allow you to limit whether an attack may be caught by guard or not. */
  skillo(SKILL_BACKSTAB, "backstab", TRUE, TAR_CONTACT);
  skillo(SKILL_INSTANT_KILL, "instant kill", TRUE, TAR_CONTACT);
  skillo(SKILL_BIND, "bind", TRUE, TAR_CONTACT);
  skillo(SKILL_CIRCLE, "circle", FALSE, TAR_CONTACT);
  skillo(SKILL_BASH, "bash", FALSE, TAR_CONTACT);
  skillo(SKILL_DISARM, "disarm", TRUE, TAR_CONTACT);
  skillo(SKILL_HIDE, "hide", FALSE, 0);
  skillo(SKILL_KICK, "kick", TRUE, TAR_CONTACT);
  skillo(SKILL_PICK_LOCK, "pick lock", TRUE, 0);
  skillo(SKILL_PUNCH, "punch", TRUE, TAR_CONTACT);
  skillo(SKILL_RESCUE, "rescue", FALSE, 0);
  skillo(SKILL_SNEAK, "sneak", FALSE, 0);
  skillo(SKILL_STEAL, "steal", TRUE, TAR_CONTACT);
  skillo(SKILL_TRACK, "track", FALSE, 0);
  skillo(SKILL_HUNT, "hunt", FALSE, 0);
  skillo(SKILL_DUAL_WIELD, "dual wield", TRUE, 0);
  skillo(SKILL_DOUBLE_ATTACK, "double attack", FALSE, 0);
  skillo(SKILL_BERSERK, "berserk", FALSE, 0);
  skillo(SKILL_SPRINGLEAP, "springleap", FALSE, TAR_CONTACT);
  skillo(SKILL_MOUNT, "mount", FALSE, 0);
  skillo(SKILL_RIDING, "riding", FALSE, 0);
  skillo(SKILL_TAME, "tame", FALSE, 0);
  skillo(SKILL_THROATCUT, "throatcut", TRUE, TAR_CONTACT);
  skillo(SKILL_DOORBASH, "doorbash", FALSE, 0);
  skillo(SKILL_PARRY, "parry", TRUE, 0);
  skillo(SKILL_DODGE, "dodge", FALSE, 0);
  skillo(SKILL_RIPOSTE, "riposte", TRUE, 0);
  skillo(SKILL_MEDITATE, "meditate", FALSE, 0);
  skillo(SKILL_QUICK_CHANT, "quick chant", FALSE, 0);
  skillo(SKILL_CHANT, "chant", FALSE, 0);
  skillo(SKILL_SAFEFALL, "safefall", FALSE, 0);
  skillo(SKILL_BAREHAND, "barehand", FALSE, TAR_CONTACT);
  skillo(SKILL_SHAPECHANGE, "shapechange", FALSE, 0);
  skillo(SKILL_SWITCH, "switch", FALSE, TAR_CONTACT);
  skillo(SKILL_GUARD, "guard", FALSE, 0);
  skillo(SKILL_BREATHE, "breathe", FALSE, 0);
  skillo(SKILL_SWEEP, "sweep", FALSE, TAR_CONTACT);
  skillo(SKILL_ROAR, "roar", FALSE, 0);
  skillo(SKILL_DOUSE, "douse", FALSE, 0);
  skillo(SKILL_SCRIBE, "scribe", TRUE, 0);
  skillo(SKILL_HITALL, "hitall", FALSE, TAR_CONTACT);
  skillo(SKILL_BANDAGE, "bandage", TRUE, 0);
  skillo(SKILL_FIRST_AID, "first aid", TRUE, 0);
  skillo(SKILL_CHANT, "chant", FALSE, 0);
  skillo(SKILL_SUMMON_MOUNT, "summon mount", TRUE, 0);
  skillo(SKILL_KNOW_SPELL, "spell knowledge", FALSE, 0);
  skillo(SKILL_SPHERE_GENERIC, "sphere of generic", FALSE, 0);
  skillo(SKILL_SPHERE_FIRE, "sphere of fire", FALSE, 0);
  skillo(SKILL_SPHERE_WATER, "sphere of water", FALSE, 0);
  skillo(SKILL_SPHERE_EARTH, "sphere of earth", FALSE, 0);
  skillo(SKILL_SPHERE_AIR, "sphere of air", FALSE, 0);
  skillo(SKILL_SPHERE_HEALING, "sphere of healing", FALSE, 0);
  skillo(SKILL_SPHERE_PROT, "sphere of protection", FALSE, 0);
  skillo(SKILL_SPHERE_ENCHANT, "sphere of enchantment", FALSE, 0);
  skillo(SKILL_SPHERE_SUMMON, "sphere of summoning", FALSE, 0);
  skillo(SKILL_SPHERE_DEATH, "sphere of death", FALSE, 0);
  skillo(SKILL_SPHERE_DIVIN, "sphere of divination", FALSE, 0);
  skillo(SKILL_BLUDGEONING, "bludgeoning weapons", TRUE, TAR_CONTACT);
  skillo(SKILL_PIERCING, "piercing weapons", TRUE, TAR_CONTACT);
  skillo(SKILL_SLASHING, "slashing weapons", TRUE, TAR_CONTACT);
  skillo(SKILL_2H_BLUDGEONING, "2H bludgeoning weapons", TRUE, TAR_CONTACT);
  skillo(SKILL_2H_PIERCING, "2H piercing weapons", TRUE, TAR_CONTACT);
  skillo(SKILL_2H_SLASHING, "2H slashing weapons", TRUE, TAR_CONTACT);
  skillo(SKILL_MISSILE, "missile weapons", TRUE, TAR_DIRECT);
  skillo(SKILL_EYE_GOUGE, "eye gouge", TRUE, TAR_CONTACT);
  skillo(SKILL_RETREAT, "retreat", FALSE, 0);
  skillo(SKILL_GROUP_RETREAT, "group retreat", FALSE, 0);
  skillo(SKILL_CORNER, "corner", FALSE, 0);
  skillo(SKILL_STEALTH, "stealth", FALSE, 0);
  skillo(SKILL_SHADOW, "shadow", FALSE, 0);
  skillo(SKILL_CONCEAL, "conceal", FALSE, 0);
  skillo(SKILL_PECK, "peck", FALSE, TAR_CONTACT);
  skillo(SKILL_CLAW, "claw", FALSE, TAR_CONTACT);
  skillo(SKILL_ELECTRIFY, "electrify", FALSE, TAR_DIRECT);
  skillo(SKILL_TANTRUM, "tantrum", FALSE, TAR_CONTACT);
  skillo(SKILL_GROUND_SHAKER, "ground shaker", FALSE, 0);
  skillo(SKILL_BATTLE_HOWL, "battle howl", FALSE, 0);
  skillo(SKILL_MAUL, "maul", FALSE, TAR_CONTACT);
  skillo(SKILL_BODYSLAM, "bodyslam", FALSE, TAR_CONTACT);
  skillo(SKILL_VAMP_TOUCH, "vampiric touch", FALSE, TAR_CONTACT);

  /* Set up monk/berserker chants */
  chanto(CHANT_REGENERATION, "regeneration", POS_SITTING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, 0, FALSE,
         "Your healthy feeling subsides.");

  chanto(CHANT_BATTLE_HYMN, "battle hymn", POS_STANDING, FALSE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, 0, FALSE,
         "Your rage fades away.");

  chanto(CHANT_WAR_CRY, "war cry", POS_STANDING, FALSE,
         TAR_IGNORE, FALSE, MAG_GROUP, 0, FALSE,
         "Your determination level returns to normal.");

  chanto(CHANT_PEACE, "peace", POS_STANDING, TRUE,
         TAR_IGNORE, FALSE, MAG_MANUAL, 0, FALSE, NULL);

  chanto(CHANT_SHADOWS_SORROW_SONG, "shadows sorrow song", POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_MASS, 0, FALSE,
         "The shadows in your mind clear up.");

  chanto(CHANT_IVORY_SYMPHONY, "ivory symphony", POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_MANUAL, 0, FALSE,
         "Feeling returns to your limbs.");

  chanto(CHANT_ARIA_OF_DISSONANCE, "aria of dissonance", POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECT, 0, TRUE,
         "The dissonance stops ringing in your ears.");

  chanto(CHANT_SONATA_OF_MALAISE, "sonata of malaise", POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_MASS, 0, FALSE,
         "The sonata of malaise stops echoing in your ears.");

  chanto(CHANT_APOCALYPTIC_ANTHEM, "apocalyptic anthem", POS_STANDING, TRUE,
         TAR_IGNORE, TRUE, MAG_MANUAL, 0, TRUE, NULL);

  chanto(CHANT_SEED_OF_DESTRUCTION, "seed of destruction", POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECT, 0, TRUE,
         "The disease leaves you.");

  chanto(CHANT_SPIRIT_WOLF, "spirit of the wolf", POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, 0, FALSE,
         "Your fangs recede and you lose your wolf-like spirit.");

  chanto(CHANT_SPIRIT_BEAR, "spirit of the bear", POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECT, 0, FALSE,
         "Your claws become decidedly less bear-like.");

  chanto(CHANT_INTERMINABLE_WRATH, "interminable wrath", POS_STANDING, TRUE,
         TAR_CHAR_ROOM | TAR_SELF_ONLY,
         FALSE, MAG_AFFECT, 0, FALSE, NULL);

  /* Set up bard songs. */
/*  songo(SONG_CHORUS_OF_COURAGE, "chorus of courage", POS_STANDING, FALSE,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECT, 0, FALSE,
        "Your courage leaves you.");
*/

  /* Set up non-skill effects */
  effecto(SKILL_AWARE, "aware", "");
}

static int skill_comparator(int a, int b)
{
  return strcmp(skills[a].name, skills[b].name);
}

void sort_skills(void)
{
   int i;

   /* initialize array */
   for (i = 0; i <= TOP_SKILL; ++i)
      skill_sort_info[i] = i;

   /* Start at element 1 to skip 'RESERVED' */
   sort(quicksort, &skill_sort_info[1], TOP_SKILL, skill_comparator);
}

void skill_assign(int skillnum, int class, int level)
{
  int okay = TRUE;

  if (skillnum < 0 || skillnum > TOP_SKILL_DEFINE) {
    sprintf(buf, "SYSERR: attempting assign to illegal talent num %d", skillnum);
    log(buf);
    return;
  }

  if (class < 0 || class >= NUM_CLASSES) {
    sprintf(buf, "SYSERR: assigning '%s' to illegal class %d",
            skill_name(skillnum), class);
    log(buf);
    okay = FALSE;
  }

  if (level < 1 || level > LVL_IMPL) {
    sprintf(buf, "SYSERR: assigning '%s' to illegal level %d",
            skill_name(skillnum), level);
    log(buf);
    okay = FALSE;
  }

  if (okay) {
    skills[skillnum].min_level[class] = level;
    skills[skillnum].lowest_level = MIN(skills[skillnum].lowest_level, level);
  }

}


int level_to_circle(int level)
{
   return LIMIT(1, (level - 1) / 8 + 1, NUM_SPELL_CIRCLES);
}

int circle_to_level(int circle)
{
   return LIMIT(1, (circle - 1) * 8 + 1, LVL_IMPL);
}

bool get_spell_assignment_circle(struct char_data *ch, int spell,
      int *circle_assignment, int *level_assignment)
{
   int i, tmp_level;

   if (IS_SPELL(spell)) {
      if (ch && skills[spell].min_level[GET_CLASS(ch)] <= GET_LEVEL(ch) &&
            /* < LVL_IMMORT tests whether the spell is actually assigned
             * to this class (unassigned ones get set to LVL_IMMORT by
             * dskill()). This only matters when imms cast spells that
             * aren't assigned to their class... */
            skills[spell].min_level[GET_CLASS(ch)] < LVL_IMMORT) {
         *level_assignment = skills[spell].min_level[GET_CLASS(ch)];
         *circle_assignment = level_to_circle(*level_assignment);
         return TRUE;
      } else {
         /* No character: may have been cast from an object.
          * Use the lowest class-assignment level. */
         tmp_level = 100;
         for (i = 0; i < NUM_CLASSES; i++) {
            if (skills[spell].min_level[i] < tmp_level) {
               tmp_level = skills[spell].min_level[i];
            }
         }
         if (tmp_level < 100) {
            *level_assignment = tmp_level;
            *circle_assignment = level_to_circle(*level_assignment);
            return TRUE;
         }
      }
   }
   return FALSE;
}

/***************************************************************************
 * $Log: skills.c,v $
 * Revision 1.68  2010/06/05 14:56:27  mud
 * Moving cooldowns to their own file.
 *
 * Revision 1.67  2009/08/02 20:20:27  myc
 * Adding pyre and fracture spells.
 *
 * Revision 1.66  2009/07/18 01:17:23  myc
 * Adding decay, iron maiden, spinechiller, and bone draw spells
 * for necromancer.
 *
 * Revision 1.65  2009/06/20 23:14:13  myc
 * Fix typo in wearoff message for speak in tongues.
 *
 * Revision 1.64  2009/03/21 19:11:37  myc
 * Save the duration each cooldown started at.
 *
 * Revision 1.63  2009/03/21 06:33:36  jps
 * Make phosphoric embers mag_manual
 *
 * Revision 1.62  2009/03/21 06:20:43  jps
 * Fix single-target sorcerer damage for spells that imms cast which
 * are not assigned to their class.
 *
 * Revision 1.61  2009/03/20 16:06:04  jps
 * Removed spells of lesser/greater invocation.
 *
 * Revision 1.60  2009/03/20 15:12:04  jps
 * Added a generic 'recall' spell to be assigned to town recall
 * scrolls, so that the object-spell verifier doesn't complain.
 *
 * Revision 1.59  2009/03/20 06:15:17  myc
 * Adding a TAR_GROUND cast requirement.  Added detonation,
 * phosphoric embers, positive field, and acid burst spells.
 * Removed combust and heatwave.  Made soul tap a manual spell.
 *
 * Revision 1.58  2009/03/17 08:59:25  jps
 * Remove leftover debugging statement
 *
 * Revision 1.57  2009/03/16 09:44:13  jps
 * Allow skills to improve normally if assigned by race
 *
 * Revision 1.56  2009/03/09 16:57:47  myc
 * Made detect poison more like detect align.
 *
 * Revision 1.55  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.54  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.53  2009/03/04 05:14:16  myc
 * Change summon corpse/shift corpse to use a custom match function
 * on the new find obj system in order to only target corpses.
 *
 * Revision 1.52  2009/01/16 23:36:34  myc
 * Add "skill" name/wear off message for 'aware'.
 *
 * Revision 1.51  2008/09/20 08:02:27  jps
 * Removed comprehend language spell.
 *
 * Revision 1.50  2008/09/12 20:16:36  jps
 * Made waterform into a quest spell.
 *
 * Revision 1.49  2008/09/11 02:50:02  jps
 * Changed skills so you have a minimum position, and fighting_ok fields.
 *
 * Revision 1.48  2008/09/01 18:29:38  jps
 * consolidating cooldown code in skills.c/h
 *
 * Revision 1.47  2008/09/01 07:31:31  jps
 * Change skill sorter to operate within the range that accessors do
 *
 * Revision 1.46  2008/08/25 00:20:33  myc
 * Changed the way mobs memorize spells.
 *
 * Revision 1.45  2008/08/24 03:15:57  myc
 * Set all skills and spells to default to DAM_UNDEFINED instead
 * of DAM_SLASHING (0).
 *
 * Revision 1.44  2008/08/20 05:03:13  jps
 * Removed the damage type 'magic'.
 *
 * Revision 1.43  2008/08/10 19:33:58  jps
 * Moved level_to_circle to skills.c. Added get_spell_assignment_circle so
 * damage spells can be properly calibrated.
 *
 * Revision 1.42  2008/08/10 17:20:26  jps
 * Mark soul reaver as violent.
 *
 * Revision 1.41  2008/08/10 16:23:11  jps
 * Make severance and soul reaver spells not quest.
 *
 * Revision 1.40  2008/08/10 01:58:49  jps
 * Added spells severance and soul reaver for illusionists.
 *
 * Revision 1.39  2008/06/21 17:28:38  jps
 * Made more use of the VALID_CLASS macro.
 *
 * Revision 1.38  2008/05/19 05:47:00  jps
 * Add mesmerize spell.
 *
 * Revision 1.37  2008/05/18 22:53:22  jps
 * Adding hysteria spell.
 *
 * Revision 1.36  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.35  2008/05/18 17:59:40  jps
 * Adding spello definition for familiarity.
 *
 * Revision 1.34  2008/05/18 03:39:05  jps
 * Gave the "detect invisibility" and "detect alignment" spells their
 * full, unabbreviated names. I'm sure this will cause output
 * misalignments...
 *
 * Revision 1.33  2008/05/18 02:33:28  jps
 * Change improve_skill_offensively so that it doesn't insist on
 * an opponent. Thus, switch can improve.
 *
 * Revision 1.32  2008/05/18 02:04:21  jps
 * Added isolation spell.
 *
 * Revision 1.31  2008/05/12 00:44:21  jps
 * Add spello calls for nightmare and discorporate.
 *
 * Revision 1.30  2008/04/23 19:58:53  myc
 * Made sort_skills use the quicksort.
 *
 * Revision 1.29  2008/04/19 21:10:31  myc
 * Added a 'show skill' subcommand, which required a list of
 * target flag and routine type strings.
 *
 * Revision 1.28  2008/04/14 08:36:09  jps
 * Add spello() call for spell of illusory wall.
 *
 * Revision 1.27  2008/04/14 02:17:16  jps
 * Adding spello() call for "glory" spell.
 *
 * Revision 1.26  2008/04/13 18:29:59  jps
 * Add spello() call for confusion.
 *
 * Revision 1.25  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.24  2008/04/05 16:31:36  jps
 * Forgot to add damage type to dispel magic spell.
 *
 * Revision 1.23  2008/04/02 17:41:56  myc
 * Fixed vampiric touch.
 *
 * Revision 1.22  2008/03/30 15:37:36  jps
 * Fix spelling of riposte.
 *
 * Revision 1.21  2008/03/29 16:28:58  jps
 * Make several spells do "mental" damage.
 *
 * Revision 1.20  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.19  2008/03/27 17:27:26  jps
 * Allow dark presence to operate on objects.
 *
 * Revision 1.18  2008/03/25 21:59:32  jps
 * Doing away with earth/air damage, going to crush, shock, water, cold.
 *
 * Revision 1.17  2008/03/24 08:43:09  jps
 * Change acid breath's damage type to acid.
 *
 * Revision 1.16  2008/03/23 18:42:21  jps
 * Using the new damage types defined in chars.h for spell damage.
 *
 * Revision 1.15  2008/03/21 15:01:17  myc
 * Removed languages.
 *
 * Revision 1.14  2008/03/10 18:01:17  myc
 * Added bodyslam skillo define.
 *
 * Revision 1.13  2008/03/09 18:10:19  jps
 * Added definition of misdirection spell.
 *
 * Revision 1.12  2008/03/09 08:59:55  jps
 * Make fear into a manual spell.
 *
 * Revision 1.11  2008/03/08 22:29:06  myc
 * Moving shapechange and chant to the cooldown system.
 *
 * Revision 1.10  2008/03/07 21:21:57  myc
 * Replaced action delays and skill delays with a single list of
 * 'cooldowns', which are decremented by a recurring event and
 * also save to the player file.
 *
 * Revision 1.9  2008/02/23 01:03:54  myc
 * Removing the min_circle and lowest_circle fields on the skillinfo
 * struct.  They are redundant; use the level fields combined with
 * LEVEL_TO_CIRCLE macro.  Renaming spell_level to skill_assign and
 * moving it here.
 *
 * Revision 1.8  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.7  2008/02/06 21:53:53  myc
 * The first aid skill define was missing.
 *
 * Revision 1.6  2008/01/29 16:51:12  myc
 * Moving skill names to the skilldef struct.
 * Adding the concept of contact and direct skills/spells.
 *
 * Revision 1.5  2008/01/27 21:18:20  myc
 * Adding berserker skill and chant defines.
 *
 * Revision 1.4  2008/01/26 23:19:28  jps
 * Remove the equipment-destroying manual spell for acid breath.
 *
 * Revision 1.3  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.2  2008/01/26 12:32:23  jps
 * Move improve_skills here. Add improve_skills_offensively() so your skills
 * won't improve when used against illusions.
 *
 * Revision 1.1  2008/01/26 10:44:41  jps
 * Initial revision
 *
 ***************************************************************************/
