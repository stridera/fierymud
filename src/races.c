/***************************************************************************
 * $Id: races.c,v 1.49 2009/07/04 16:23:36 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: races.c                                        Part of FieryMUD *
 *  Usage: Aligns race situations                                          *
 * Author: Brian Williams <bmw@efn.org>                                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 98, 99               *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include <math.h>

#include "structs.h"
#include "interpreter.h"
#include "utils.h"
#include "races.h"
#include "class.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "casting.h"
#include "skills.h"
#include "math.h"
#include "regen.h"
#include "composition.h"
#include "lifeforce.h"
#include "charsize.h"

/* Prototypes */
void set_init_height_weight(struct char_data *ch);


/* races[]
 *
 * The individual members of these struct definitions have been arranged
 * in a regular fashion, so that they can easily be located.  Please maintain
 * this arrangement:
 *
 *   name, names, displayname, fullname, plainname,
 *   playable, humanoid, racealign, def_size, def_align, \
 *     bonus_damroll, bonus_hitroll,
 *   def_lifeforce, def_composition,
 *   mweight_lo, mweight_hi, mheight_lo, mheight_hi, \
 *     fweight_lo, fweight_hi, fheight_lo, fheight_hi
 *   attrib_scales[], (str, dex, int, wis, con, cha)
 *   exp_factor, hit_factor, hd_factor, dice_factor, copper_factor, ac_factor,
 *   move_verb, leave_verb
 */

struct racedef races[NUM_RACES] = {
   /* HUMAN */
   { "human", "human", "&6Human&0", "&6Human&0", "Human",
      TRUE, TRUE, RACE_ALIGN_GOOD, SIZE_MEDIUM, 0, 3, 3,
      LIFE_LIFE, COMP_FLESH,
      120, 180, 60, 76, 95, 150, 60, 70,
      { 72, 72, 72, 72, 72, 72 },
      100, 100, 100, 100, 75, 100,
      NULL, NULL,
      0
   },

   /* GREY ELF */
   { "elf", "grey gray elf", "&8Grey Elf&0", "&8Grey Elf&0", "Elf",
      FALSE, TRUE, RACE_ALIGN_GOOD, SIZE_MEDIUM, 1000, 2, 3,
      LIFE_LIFE, COMP_FLESH,
      90, 160, 60, 70, 90, 160, 59, 68,
      { 64, 80, 88, 86, 64, 78 },
      100, 100, 100, 100, 75, 100,
      NULL, NULL,
      0
   },

   /* GNOME */
   { "gnome", "gnome", "&1&d&bGnome&0", "&1&d&bGnome&0", "Gnome",
      TRUE, TRUE, RACE_ALIGN_GOOD, SIZE_SMALL, 1000, 2, 4,
      LIFE_LIFE, COMP_FLESH,
      40, 90, 26, 38, 40, 90, 26, 38,
      { 64, 76, 88, 84, 68, 76 },
      100, 100, 100, 100, 75, 100,
      NULL, NULL,
      0
   },

   /* DWARF */
   { "dwarf", "dwarf mountain", "&3Dwarf&0", "&3Mountain Dwarf&0", "Dwarf",
      TRUE, TRUE, RACE_ALIGN_GOOD, SIZE_MEDIUM, 1000, 4, 5,
      LIFE_LIFE, COMP_FLESH,
      170, 200, 38, 50, 150, 190, 38, 50,
      { 84, 76, 64, 86, 84, 68 },
      100, 100, 100, 100, 75, 100,
      NULL, NULL,
      0
   },

   /* TROLL */
   { "troll", "swamp troll", "&2&dTroll&0", "&2&dSwamp Troll&0", "Troll",
      TRUE, TRUE, RACE_ALIGN_EVIL, SIZE_LARGE, -1000, 5, 4,
      LIFE_LIFE, COMP_FLESH,
      130, 290, 72, 90, 130, 290, 72, 90,
      { 92, 72, 56, 56, 92, 64 },
      100, 120, 110, 110, 75, 100,
      "prowls in", "prowls",
      0
   },

   /* DROW */
   { "drow", "drow", "&5Drow&0", "&5Drow&0", "Drow",
      FALSE, TRUE, RACE_ALIGN_EVIL, SIZE_MEDIUM, -1000, 2, 3,
      LIFE_LIFE, COMP_FLESH,
      90, 160, 60, 70, 90, 160, 60, 70,
      { 64, 80, 88, 80, 64, 72 },
      100, 100, 100, 100, 75, 100,
      NULL, NULL,
      0
   },

   /* DUERGAR */
   { "duergar", "duergar", "&1Duergar&0", "&1Duergar&0", "Duergar",
      TRUE, TRUE, RACE_ALIGN_EVIL, SIZE_MEDIUM, -1000, 4, 5,
      LIFE_LIFE, COMP_FLESH,
      170, 200, 38, 50, 150, 190, 38, 50,
      { 84, 76, 68, 72, 84, 64 },
      100, 100, 100, 100, 75, 100,
      "skulks in", "skulks",
      0
   },

   /* OGRE */
   { "ogre", "ogre", "&4Ogre&0", "&4Ogre&0", "Ogre",
      TRUE, TRUE, RACE_ALIGN_EVIL, SIZE_LARGE, -1000, 5, 4,
      LIFE_LIFE, COMP_FLESH,
      390, 530, 93, 119, 390, 530, 93, 119,
      { 96, 64, 52, 60,100, 60 },
      100, 130, 100, 120, 75, 90,
      "lumbers in", "lumbers",
      0
   },

   /* ORC */
   { "orc", "orc", "&9&bOrc&0", "&9&bOrc&0", "Orc",
      TRUE, TRUE, RACE_ALIGN_EVIL, SIZE_MEDIUM, -1000, 3, 3,
      LIFE_LIFE, COMP_FLESH,
      90, 150, 58, 68, 90, 150, 58, 68,
      { 72, 72, 72, 72, 72, 72 },
      100, 100, 100, 100, 75, 100,
      NULL, NULL,
      0
   },

   /* HALF-ELF */
   { "half-elf", "half-elf half elf", "&6&bHalf-&0&6&dElf&0", "&6&bHalf-&0&6&dElf&0", "Half-Elf",
      TRUE, TRUE, RACE_ALIGN_GOOD, SIZE_MEDIUM, 1000, 3, 3,
      LIFE_LIFE, COMP_FLESH,
      100, 170, 60, 76, 94, 155, 60, 70,
      { 68, 76, 76, 76, 68, 78 },
      100, 100, 100, 100, 75, 100,
      NULL, NULL,
      0
   },

   /* BARBARIAN */
   { "barbarian", "barbarian", "&4Barbarian&0", "&4Barbarian&0", "Barbarian",
      TRUE, TRUE, RACE_ALIGN_GOOD, SIZE_LARGE, 0, 5, 4,
      LIFE_LIFE, COMP_FLESH,
      170, 260, 69, 88, 130, 210, 69, 80,
      { 88, 68, 60, 60, 88, 64 },
      100, 100, 100, 100, 75, 100,
      NULL, NULL,
      0
   },

   /* HALFLING */
   { "halfling", "halfling", "&3&dHalfling&0", "&3&dHalfling&0", "Halfling",
      TRUE, TRUE, RACE_ALIGN_GOOD, SIZE_SMALL, 1000, 3, 6,
      LIFE_LIFE, COMP_FLESH,
      90, 160, 35, 42, 90, 160, 35, 42,
      { 68, 96, 80, 80, 64, 76 },
      100, 100, 100, 100, 75, 100,
      NULL, NULL,
      0
   },

   /* PLANT */
   { "plant", "plant", "&2Plant&0", "&2Plant&0", "Plant",
      FALSE, FALSE, RACE_ALIGN_GOOD, SIZE_MEDIUM, 0, 3, 3,
      LIFE_LIFE, COMP_PLANT,
      80, 180, 40, 96, 80, 180, 40, 96,
      { 72, 52, 32, 72, 100, 72 },
      100, 100, 100, 100, 0, 120,
      NULL, NULL,
      0
   },

   /* HUMANOID */
   { "humanoid", "humanoid", "&7Humanoid&0", "&7Humanoid&0", "Humanoid",
      FALSE, TRUE, RACE_ALIGN_GOOD, SIZE_MEDIUM, 0, 3, 3,
      LIFE_LIFE, COMP_FLESH,
      120, 180, 60, 76, 95, 150, 60, 70,
      { 72, 72, 72, 72, 72, 72 },
      100, 100, 100, 100, 100, 60,
      NULL, NULL,
      0
   },

   /* ANIMAL */
   { "animal", "animal", "&2Animal&0", "&2Animal&0", "Animal",
      FALSE, FALSE, RACE_ALIGN_GOOD, SIZE_MEDIUM, 0, 3, 3,
      LIFE_LIFE, COMP_FLESH,
      120, 180, 60, 76, 95, 150, 60, 70,
      { 72, 72, 72, 72, 72, 72 },
      100, 100, 100, 100, 0,65,
      NULL, NULL,
      0
   },

   /* DRAGON */
   { "dragon", "dragon", "&1&bDragon&0", "&1&bDragon&0", "Dragon",
      FALSE, FALSE, RACE_ALIGN_GOOD, SIZE_GARGANTUAN, 0, 10, 4,
      LIFE_LIFE, COMP_FLESH,
      16000, 64000, 768, 1536, 16000, 64000, 768, 1536,
      {100, 72,100, 72, 72,100 },
      130, 130, 140, 140, 500, 140,
      "stomps in", "stomps",
      0
   },

   /* GIANT */
   { "giant", "giant", "&2&bGiant&0", "&2&bGiant&0", "Giant",
      FALSE, TRUE, RACE_ALIGN_GOOD, SIZE_HUGE, 0, 7, 3,
      LIFE_LIFE, COMP_FLESH,
      1000, 4000, 196, 384, 1000, 4000, 196, 384,
      {100, 72, 44, 64, 80, 72 },
      110, 120, 120, 100, 125, 120,
      "lumbers in", "lumbers",
      0
   },

   /* OTHER */
   { "other", "other", "&4&bOther&0", "&4&bOther&0", "Other",
      FALSE, FALSE, RACE_ALIGN_GOOD, SIZE_MEDIUM, 0, 3, 3,
      LIFE_LIFE, COMP_FLESH,
      120, 180, 60, 76, 95, 150, 60, 70,
      { 72, 72, 72, 72, 72, 72 },
      80, 110, 120, 80, 75, 105,
      NULL, NULL,
      0
   },

   /* GOBLIN */
   { "goblin", "goblin", "&4&bGoblin&0", "&4&bGoblin&0", "Goblin",
      FALSE, TRUE, RACE_ALIGN_EVIL, SIZE_SMALL, -500, 3, 3,
      LIFE_LIFE, COMP_FLESH,
      60, 90, 30, 38, 55, 80, 30, 35,
      { 76, 72, 64, 72, 84, 64 },
      60, 60, 60, 60, 75, 90,
      NULL, NULL,
      0
   },

   /* DEMON */
   { "demon", "demon", "&1&bDemon&0", "&1&bDemon&0", "Demon",
      FALSE, TRUE, RACE_ALIGN_EVIL, SIZE_LARGE, -1000, 6, 4,
      LIFE_DEMONIC, COMP_FLESH,
      130, 290, 72, 90, 130, 290, 72, 90,
      { 80,100, 68, 68, 58, 58 },
      120, 120, 120, 120, 150, 120,
      "stalks in", "stalks",
      0
   },

   /* BROWNIE */
   { "brownie", "brownie", "&3Brownie&0", "&3Brownie&0", "Brownie",
      FALSE, TRUE, RACE_ALIGN_GOOD, SIZE_SMALL, 500, 1, 3,
      LIFE_LIFE, COMP_FLESH,
      20, 30, 20, 30, 20, 30, 20, 30,
      { 60, 80, 60, 78, 70, 72 },
      100, 100, 100, 100, 75, 100,
      NULL, NULL,
      0
   }
};

const char *race_align_abbrevs[] = {
  "&0&3&bGOOD&0",
  "&0&1&bEVIL&0"
};


static flagvector race_effects_mask[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];
void init_races(void)
{
  #define PERM_EFF(r, f) SET_FLAG(races[(r)].effect_flags, (f))
  #define ADD_SKILL(s, p) do { \
    races[race].skills[pos].skill = (s); \
    races[race].skills[pos].proficiency = (p); \
    ++pos; \
  } while (0)

  int race, pos;

  /*
   * Add permanent effects to races here.
   */
  PERM_EFF(RACE_DROW,     EFF_INFRAVISION);
  PERM_EFF(RACE_DROW,     EFF_ULTRAVISION);
  PERM_EFF(RACE_ELF,      EFF_INFRAVISION);
  PERM_EFF(RACE_DWARF,    EFF_DETECT_POISON);
  PERM_EFF(RACE_DWARF,    EFF_INFRAVISION);
  PERM_EFF(RACE_DUERGAR,  EFF_INFRAVISION);
  PERM_EFF(RACE_DUERGAR,  EFF_ULTRAVISION);
  PERM_EFF(RACE_HALFLING, EFF_INFRAVISION);
  PERM_EFF(RACE_HALFLING, EFF_SENSE_LIFE);
  PERM_EFF(RACE_TROLL,    EFF_INFRAVISION);
  PERM_EFF(RACE_TROLL,    EFF_ULTRAVISION);
  PERM_EFF(RACE_OGRE,     EFF_INFRAVISION);
  PERM_EFF(RACE_OGRE,     EFF_ULTRAVISION);
  PERM_EFF(RACE_HALF_ELF, EFF_INFRAVISION);
  PERM_EFF(RACE_GNOME,    EFF_INFRAVISION);
  PERM_EFF(RACE_BROWNIE,  EFF_INFRAVISION);

  /*
   * Add race skills to the switch below.
   */
  for (race = 0; race < NUM_RACES; ++race) {
    memset(races[race].skills, 0, sizeof(races[race].skills));
    pos = 0;
    switch (race) {
    case RACE_TROLL:
      ADD_SKILL(SKILL_DOORBASH, 1000);
      ADD_SKILL(SKILL_BODYSLAM, 1000);
      break;
    case RACE_OGRE:
      ADD_SKILL(SKILL_DOORBASH, 1000);
      ADD_SKILL(SKILL_BODYSLAM, 1000);
      break;
    case RACE_BARBARIAN:
      ADD_SKILL(SKILL_DOORBASH, 1000);
      ADD_SKILL(SKILL_BODYSLAM, 1000);
      break;
    case RACE_DRAGON:
      ADD_SKILL(SKILL_BREATHE, ROLL_SKILL_PROF);
      ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
      ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
      ADD_SKILL(SPELL_ACID_BREATH, 1000);
      ADD_SKILL(SPELL_FROST_BREATH, 1000);
      ADD_SKILL(SPELL_GAS_BREATH, 1000);
      ADD_SKILL(SPELL_FIRE_BREATH, 1000);
      ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
      break;
    case RACE_DEMON:
      ADD_SKILL(SKILL_BREATHE, ROLL_SKILL_PROF);
      ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
      ADD_SKILL(SPELL_ACID_BREATH, 1000);
      ADD_SKILL(SPELL_FROST_BREATH, 1000);
      ADD_SKILL(SPELL_GAS_BREATH, 1000);
      ADD_SKILL(SPELL_FIRE_BREATH, 1000);
      ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
      break;
    case RACE_BROWNIE:
      ADD_SKILL(SKILL_SNEAK, ROLL_SKILL_PROF);
      ADD_SKILL(SKILL_HIDE, ROLL_SKILL_PROF);
      break;
    }
    if (pos > NUM_RACE_SKILLS) {
      sprintf(buf, "init_races: Too many skills assigned to race %s.  "
                   "Increase NUM_RACE_SKILLS in races.h to at least %d",
              races[race].name, pos);
      log(buf);
      exit(1);
    }
  }

  CLEAR_FLAGS(race_effects_mask, NUM_EFF_FLAGS);
  for (race = 0; race < NUM_RACES; ++race)
    SET_FLAGS(race_effects_mask, races[race].effect_flags, NUM_EFF_FLAGS);

  #undef ADD_SKILL
  #undef PERM_EFF
}



/* parse_race
 *
 * Identifies a race from a string.  Will do partial matches.
 *
 * Code is present to prohibit a player from being set to the wrong
 * race, but it's disabled.  If it were enabled, it would only take
 * effect if "vict" were not null.
 *
 * ch is someone who's trying to change vict's race (e.g., a wizard
 * manually setting someone to a race due to a quest).
 *
 * If RACE_UNDEFINED is returned, this function will already have provided
 * feedback to ch (if specified) as to the reason for the failure.  Otherwise,
 * it does not provide feedback.
 */
int parse_race(struct char_data *ch, struct char_data *vict, char *arg) {
   int i, race = RACE_UNDEFINED, altname = RACE_UNDEFINED, best = RACE_UNDEFINED;

   if (!*arg){
      if (ch)
         send_to_char("What race?\r\n", ch);
      return RACE_UNDEFINED;
   }

   for (i = 0; i < NUM_RACES; i++) {
      if (!strncasecmp(arg, races[i].name, strlen(arg))) {
         if (!strcasecmp(arg, races[i].name)) {
            race = i;
            break;
         }
         if (best == RACE_UNDEFINED)
            best = i;
      } else if (isname(arg, races[i].names)) {
         if (altname == RACE_UNDEFINED)
            altname = i;
      } else if (is_abbrev(arg, races[i].name)) {
         if (best == RACE_UNDEFINED)
            best = i;
      }
   }

   if (race == RACE_UNDEFINED) race = altname;
   if (race == RACE_UNDEFINED) race = best;
   if (race == RACE_UNDEFINED) {
      if (ch)
         send_to_char("There is no such race.\r\n", ch);
   }

   /* There are no validity checks. */
   return race;

   /* The following code could be used to prevent deities from assigning
    * a race racee to a player if:
    *
    *  - The race is not "playable"
    *  - The player's race does not allow the race
    *
    * It's currently not used. */

   /* Bypass validity checks for immortal victims (or no specified victim). */
   if (!vict || GET_LEVEL(vict) > LVL_MAX_MORT)
      return race;

   /* The race has been identified, and there is a mortal victim.
    * Make sure this race is available to the victim. */

   if (!races[race].playable) {
      if (ch) {
         sprintf(buf, "The %s race is not available to mortals.\r\n",
               races[race].name);
         send_to_char(buf, ch);
      }
      return RACE_UNDEFINED;
   }

   if (!class_ok_race[race][(int)GET_CLASS(vict)]) {
      if (ch) {
         sprintf(buf, "As %s, $n can't be %s.",
               with_indefinite_article(classes[(int)GET_CLASS(vict)].displayname),
               with_indefinite_article(races[race].displayname));
         act(buf, FALSE, vict, 0, ch, TO_VICT);
       }
       return RACE_UNDEFINED;
   }

   return race;
}

/* Send a menu to someone who's creating a character, listing the available
 * races.  We assume that this function would not have been called if
 * "races_allowed" were false. */
void send_race_menu(struct descriptor_data *d) {
   extern int evil_races_allowed;
   char idx;
   int i;

   write_to_output("\r\nThe following races are available:\r\n", d);
   for (i = 0, idx = 'a'; i < NUM_RACES; i++) {
      if (races[i].playable &&
            (evil_races_allowed || races[i].racealign == RACE_ALIGN_GOOD)) {
         sprintf(buf, "  &7%c)&0 %s\r\n", idx, races[i].fullname);
         write_to_output(buf, d);
         idx++;
      }
   }
}

/* Someone who's creating a character typed a letter to indicate which
 * race they wanted.  Determine which race they indicated, using the same
 * rules as send_race_menu() -- skip over inactive/unavailable races. */
int interpret_race_selection(char arg) {
   extern int evil_races_allowed;
   char idx;
   int i;

   for (i = 0, idx = 'a'; i < NUM_RACES; i++) {
      if (races[i].playable &&
            (evil_races_allowed || races[i].racealign == RACE_ALIGN_GOOD)) {
         if (arg == idx)
            return i;
         idx++;
      }
   }
   return RACE_UNDEFINED;
}

/* Oddly enough, the base value for movement points is not stored
 * anywhere.  Thus, it would be impossible to increase the value
 * as a player advances in level.  Anyway, the same value gets set,
 * based on CON, whenever a player logs in.
 *
 * There are times when you want to see this unaffected value, so
 * here's the function to find it.  This is also used at character
 * creation time and when logging. */
int natural_move(struct char_data *ch)
{
   if (IS_NPC(ch) && GET_MOB_RNUM(ch) >= 0) {

     /* Mountable mobs will have their mv points set according to level.
      * The second parameter to pow (now 0.8) controls how the points
      * increase as the level increases. If it were 1, the points would
      * increase in a straight line as the level increases. If it were
      * greater than 1, the points would increase slowly at first, and
      * then sharply curve up to their maximum as the level got close
      * to the maximum level. When it's below 1, the points increase
      * quickly at first, and then slowly reach their maximum. */

      if (MOB_FLAGGED(ch, MOB_MOUNTABLE)) {
         if (GET_LEVEL(ch) > MAX_MOUNT_LEVEL)
            return MOUNT_MAXMOVE + 2 * (GET_LEVEL(ch) - MAX_MOUNT_LEVEL) +
                  number(0, 9);
         else
            return (int)(MOUNT_MINMOVE + (MOUNT_MAXMOVE - MOUNT_MINMOVE) *
                  pow((GET_LEVEL(ch) - 1) / (double)(MAX_MOUNT_LEVEL - 1), 0.8)) +
                  number(0, 9);
      } else
         return mob_proto[GET_MOB_RNUM(ch)].points.max_move;
   } else {
      return MAX(100, GET_CON(ch) * 2);
   }
}

/* init_proto_race()
 *
 * Sets beginning values on a mob prototype, according to race.
 */

void init_proto_race(struct char_data *ch)
{
   set_base_size(ch, races[(int)GET_RACE(ch)].def_size);
   GET_LIFEFORCE(ch) = races[(int)GET_RACE(ch)].def_lifeforce;
   BASE_COMPOSITION(ch) = races[(int)GET_RACE(ch)].def_composition;
   GET_COMPOSITION(ch) = BASE_COMPOSITION(ch);
}

/* init_char_race()
 *
 * Sets beginning values that are appropriate for a brand-new character,
 * according to race. */

void init_char_race(struct char_data *ch)
{
   if (!IS_NPC(ch) && VALID_RACE(ch)) {
      GET_BASE_DAMROLL(ch) = races[(int) GET_RACE(ch)].bonus_damroll;
      GET_BASE_HITROLL(ch) = races[(int) GET_RACE(ch)].bonus_hitroll;
   }

   /* NPCs will have their own align defined at build time,
    * and it might have been adjusted by the builder, too. */
   if (!IS_NPC(ch) && VALID_RACE(ch))
      GET_ALIGNMENT(ch) = races[(int)GET_RACE(ch)].def_align;
   set_init_height_weight(ch);

   GET_MAX_MOVE(ch) = natural_move(ch);
}

void update_char_race(struct char_data *ch)
{
   if (!VALID_RACE(ch)) {
     log("update_char_race: %s doesn't have a valid race (%d).",
         GET_NAME(ch), GET_RACE(ch));
     return;
   }

   GET_RACE_ALIGN(ch) = races[(int)GET_RACE(ch)].racealign;

   /* Any bits that might get set below should be cleared here first. */
   REMOVE_FLAGS(EFF_FLAGS(ch), race_effects_mask, NUM_EFF_FLAGS);

   /* Reset effect flags for this race */
   SET_FLAGS(EFF_FLAGS(ch), races[(int) GET_RACE(ch)].effect_flags, NUM_EFF_FLAGS);
}

/*
 * Returns a positive value for skills that this race has.
 *
 * Doesn't disqualify any skills! Only enables them.
 */

int racial_skill_proficiency(int skill, int race, int level)
{
   int i;

   for (i = 0; races[race].skills[i].skill > 0 && i < NUM_RACE_SKILLS; ++i)
      if (races[race].skills[i].skill == skill) {
         return races[race].skills[i].proficiency;
      }

  return 0;
}

/* convert_race does no checking.  It expects a valid race and ch.
 * This function changes a player's race and converts the skills/spells
 * accordingly, keeping the old values if they are better.
 * It also transfers quest spells. */
void convert_race(struct char_data *ch, int newrace)
{
  int skill;
  sh_int old_skills[TOP_SKILL+1];
  sh_int new_skills[TOP_SKILL+1];

  /* read in the player's old skills */
  for (skill = 0; skill <= TOP_SKILL; skill++) {
    old_skills[skill] = GET_ISKILL(ch, skill);
  }

  /* set race/align */
  GET_RACE(ch) = newrace;

  /* Big changes occur here: */
  update_char(ch);

  /* read the new skills */
  for (skill = 0; skill <= TOP_SKILL; skill++) {
    new_skills[skill] = GET_ISKILL(ch, skill);
  }

  /* compare old and new */
  for (skill = 0; skill <= TOP_SKILL; skill++) {
    if(new_skills[skill]) {
      /* keep the value of the old skill if you still have the skill */
      if(old_skills[skill] > new_skills[skill]) {
        SET_SKILL(ch, skill, old_skills[skill]);
      }
    }

    /* keep any quest spells you might have earned */
    if((old_skills[skill]) && (skills[skill].quest)) {
      SET_SKILL(ch, skill, old_skills[skill]);
    }
  }
  check_regen_rates(ch);
}

void scale_attribs(struct char_data *ch) {
   if (VALID_RACE(ch)) {
      GET_AFFECTED_STR(ch) = (GET_VIEWED_STR(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_STR - 1]) / 100;
      GET_AFFECTED_DEX(ch) = (GET_VIEWED_DEX(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_DEX - 1]) / 100;
      GET_AFFECTED_INT(ch) = (GET_VIEWED_INT(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_INT - 1]) / 100;
      GET_AFFECTED_WIS(ch) = (GET_VIEWED_WIS(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_WIS - 1]) / 100;
      GET_AFFECTED_CON(ch) = (GET_VIEWED_CON(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_CON - 1]) / 100;
      GET_AFFECTED_CHA(ch) = (GET_VIEWED_CHA(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_CHA - 1]) / 100;
   } else {
      GET_AFFECTED_STR(ch) = GET_VIEWED_STR(ch) * 72 / 100;
      GET_AFFECTED_DEX(ch) = GET_VIEWED_DEX(ch) * 72 / 100;
      GET_AFFECTED_INT(ch) = GET_VIEWED_INT(ch) * 72 / 100;
      GET_AFFECTED_WIS(ch) = GET_VIEWED_WIS(ch) * 72 / 100;
      GET_AFFECTED_CON(ch) = GET_VIEWED_CON(ch) * 72 / 100;
      GET_AFFECTED_CHA(ch) = GET_VIEWED_CHA(ch) * 72 / 100;
   }
}

/***************************************************************************
 * $Log: races.c,v $
 * Revision 1.49  2009/07/04 16:23:36  myc
 * Removed an unneeded local buffer.
 *
 * Revision 1.48  2009/03/16 09:44:38  jps
 * Added brownie race
 *
 * Revision 1.47  2009/03/09 16:57:47  myc
 * Added detect poison effect as dwarven innate.
 *
 * Revision 1.46  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.45  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.44  2008/09/27 03:21:28  jps
 * Changed size of ogres to large.
 *
 * Revision 1.43  2008/08/30 01:31:51  myc
 * Changed the way stats are calculated in effect_total; ability
 * stats are saved in a raw form now, and only capped when accessed.
 * Damroll and hitroll are recalculated everytime effect_total
 * is called, using cached base values.
 *
 * Revision 1.42  2008/06/21 17:27:56  jps
 * Added movement strings to race definitions. Made more use of the
 * VALID_RACE macro.
 *
 * Revision 1.41  2008/05/11 05:42:03  jps
 * Using regen.h.
 *
 * Revision 1.40  2008/04/26 23:35:43  myc
 * Info about permanent effects and race skills are stored in the
 * class/race structs now, but need to be initialized at runtime
 * by the init_races and init_classes functions.
 *
 * Revision 1.39  2008/04/19 18:25:02  jps
 * Fixed dragon default size to gargantuan.
 *
 * Revision 1.38  2008/04/05 05:05:42  myc
 * Removed SEND_TO_Q macro, so call write_to_output directly.
 *
 * Revision 1.37  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.36  2008/03/26 16:44:36  jps
 * Replaced all checks for undead race with checks for undead lifeforce.
 * Replaced the undead race with the plant race.
 *
 * Revision 1.35  2008/03/23 00:24:11  jps
 * Use base composition when initializing prototypes.
 *
 * Revision 1.34  2008/03/22 21:44:23  jps
 * Changing the base size, life force, and composition are now
 * done automatically only for mob prototypes.
 *
 * Revision 1.33  2008/03/22 21:26:50  jps
 * Adding default life force and composition to each race definition.
 * Setting a character's base size, life force, and composition
 * according to the default values when created.
 *
 * Revision 1.32  2008/03/21 16:09:01  myc
 * Quick-fix for racial_skill_proficiency log spam.
 *
 * Revision 1.31  2008/03/21 15:01:17  myc
 * Removed languages.
 *
 * Revision 1.30  2008/03/18 06:00:31  jps
 * Remove unused array size_abbrevs.
 *
 * Revision 1.29  2008/03/11 02:13:39  jps
 * Moving size-related functions to chars.c.
 *
 * Revision 1.28  2008/03/10 19:55:37  jps
 * Made a struct for sizes with name, height, and weight.  Save base height
 * weight and size so they stay the same over size changes.
 *
 * Revision 1.27  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.26  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.25  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species into races.
 *
 * Revision 1.24  2008/01/27 01:13:39  jps
 * Stop overriding the mob-prototype alignment when loading mobs.
 *
 * Revision 1.23  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.22  2008/01/23 14:42:53  jps
 * Added a bunch of race definitions.
 *
 * Revision 1.21  2008/01/23 14:15:15  jps
 * Added the "humanoid" field to racedefs.
 *
 * Revision 1.20  2008/01/09 13:04:24  jps
 * Oops, forgot to remove some debugging code.
 *
 * Revision 1.19  2008/01/09 09:19:50  jps
 * Add function natural_move to tell us a mob's natural move points.
 * Set base move points here, rather than db.c.
 *
 * Revision 1.18  2008/01/09 08:32:52  jps
 * Add code to set a character's height and weight when created.
 * If there is a race definition, it uses the information there.
 * Otherwise it produces random values according to the "size".
 *
 * Revision 1.17  2008/01/06 18:17:14  jps
 * Added bonus hit/dam, and height/weight/size/lang/align values
 * into struct racedef.
 *
 * Revision 1.16  2008/01/05 05:43:53  jps
 * Lots of stuff moved from class.c. Which deals with races.
 *
 * Revision 1.15  2008/01/04 02:33:16  jps
 * The race selection menu is now dynamic.
 *
 * Revision 1.14  2008/01/04 01:49:41  jps
 * Added races.h file.  Combined several race-related items into the
 * global array "races".  Removed obsolete hometown stuff.
 *
 * Revision 1.13  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.12  2008/01/02 07:11:32  jps
 * Using class.h.
 *
 * Revision 1.11  2007/03/27 04:27:05  myc
 * Added new size, colossal.
 *
 * Revision 1.10  2006/11/26 08:31:17  jps
 * Typo fixes for character creation process.
 *
 * Revision 1.9  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.8  2000/11/24 21:17:12  rsd
 * Altered comment header and added back rlog messgaes from
 * prior to the addition of the $log$ string.
 *
 * Revision 1.7  2000/11/14 20:23:14  rsd
 * removed some old comments from the file and corrected a typo
 *
 * Revision 1.6  2000/09/13 01:53:49  rsd
 * Ok, I fixed it to where all good races create to Mielikki
 * since we don't have th playerbase to spread newbies out so far.
 * grumble.
 *
 * Revision 1.5  2000/05/14 05:22:37  rsd
 * completely fixed the comment header. Added Kerristone as
 * a hometowne, also added it as a choice for humans to select
 * for the time being. Also retabbed where players can
 * select hometowns.
 *
 * Revision 1.4  1999/12/12 06:47:01  rsd
 * altered const char *race_menu to remove grey and drow elfs.
 * added a const char *good_race_menu for good race only character
 * creation.  Added a int parse_good_race() with case statements
 * for good races only.  Altered int parse_race() to be in ABC
 * choice order, removed grey and drow elf from it's choices.
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/02/01 03:51:29  mud
 * indented entire file except matrix
 * dos2unix
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/
