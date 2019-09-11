/***************************************************************************
 * $Id: cleric.c,v 1.26 2009/07/04 16:23:13 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: cleric.c                                        Part of FieryMUD *
 *  Usage: Control Cleric type mobs, this file is closely related to ai.h  *
 *         and ai_util.c                                                   *
 *  By: Ben Horner (Proky of HubisMUD)                                     *
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
#include "class.h"
#include "skills.h"
#include "races.h"
#include "math.h"
#include "magic.h"
#include "lifeforce.h"

/* External functions */
int mob_cast(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int spellnum);
bool affected_by_armor_spells(struct char_data *victim);

/* Clerical spell lists */
const struct spell_pair mob_cleric_buffs[] = {
  {SPELL_SOULSHIELD, 0, EFF_SOULSHIELD} ,
  {SPELL_PROT_FROM_EVIL, 0, EFF_PROTECT_EVIL},
  {SPELL_ARMOR, 0, 0},
  {SPELL_DEMONSKIN, 0, 0},
  {SPELL_GAIAS_CLOAK, 0, 0},
  {SPELL_BARKSKIN, 0, 0},
  {SPELL_DEMONIC_MUTATION, 0, 0},
  {SPELL_DEMONIC_ASPECT, 0, 0},
  {SPELL_SENSE_LIFE, 0, EFF_SENSE_LIFE},
  {SPELL_PRAYER, 0, 0},
  {SPELL_DARK_PRESENCE, 0, 0},
  {0, 0, 0}
};

/* These spells should all be castable in combat. */
const struct spell_pair mob_cleric_hindrances[] = {
  {SPELL_BLINDNESS, SPELL_CURE_BLIND, EFF_BLIND},
  {SPELL_POISON, SPELL_REMOVE_POISON, EFF_POISON},
  {SPELL_DISEASE, SPELL_HEAL, EFF_DISEASE},
  {SPELL_CURSE, SPELL_REMOVE_CURSE, EFF_CURSE},
  {SPELL_INSANITY, SPELL_SANE_MIND, EFF_INSANITY},
  {SPELL_SILENCE, 0, 0}, /* Try to cast this, but there's no cure */
  {SPELL_ENTANGLE, 0, 0},
  {0, 0, 0}
};

const int mob_cleric_offensives[] = {
  SPELL_FULL_HARM,
  SPELL_SUNRAY,
  SPELL_FLAMESTRIKE,
  SPELL_DIVINE_RAY,
  SPELL_HARM,
  SPELL_DESTROY_UNDEAD,
  SPELL_STYGIAN_ERUPTION,
  SPELL_DISPEL_EVIL,
  SPELL_DISPEL_GOOD,
  SPELL_WRITHING_WEEDS,
  SPELL_HELL_BOLT,
  SPELL_DIVINE_BOLT,
  SPELL_CAUSE_CRITIC,
  SPELL_CAUSE_SERIOUS,
  SPELL_CAUSE_LIGHT,
  0
};

const int mob_cleric_area_spells[] = {
  SPELL_HOLY_WORD,
  SPELL_UNHOLY_WORD,
  SPELL_EARTHQUAKE,
  0
};

const int mob_cleric_heals[] = {
  SPELL_FULL_HEAL,
  SPELL_HEAL,
  SPELL_CURE_CRITIC,
  SPELL_CURE_SERIOUS,
  SPELL_CURE_LIGHT,
  0
};


/*
 * cleric_ai_action
 *
 *
 */
bool cleric_ai_action(struct char_data * ch, struct char_data * victim)
{
  int my_health, victim_health, i, counter, action = 0;

  if (!victim) {
    mudlog("No victim in cleric AI action.", NRM, LVL_GOD, FALSE);
    return FALSE;
  }

  /* Well no chance of casting any spells today. */
  if (EFF_FLAGGED(ch, EFF_SILENCE))
    return FALSE;

  /* Most classes using clerical spells have alignment restrictions. */
  if ((GET_CLASS(ch) == CLASS_DIABOLIST && !IS_EVIL(ch)) ||
      (GET_CLASS(ch) == CLASS_PRIEST && !IS_GOOD(ch)) ||
      (GET_CLASS(ch) == CLASS_PALADIN && !IS_GOOD(ch)) ||
      (GET_CLASS(ch) == CLASS_RANGER && !IS_GOOD(ch)) ||
      (GET_CLASS(ch) == CLASS_ANTI_PALADIN && !IS_EVIL(ch)))
    return FALSE;

  /* Calculate mob and victim health as a percentage. */
  my_health = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
  victim_health = (100 * GET_HIT(victim)) / GET_MAX_HIT(victim);
  
  if (my_health > 90)
    action = 10;
  else if (my_health > 60)
    action = 4;
  else if (my_health < 30)
    action = 2;
  if (victim_health < 20)
    action += 3;
  else if (victim_health < 10)
    action += 5;
  
  /* If action < 6 then heal. */
  if (action < 6 && mob_heal_up(ch))
    return TRUE;

  
  /* Otherwise kill or harm in some fashion */

  /* Area effects first if the victim is grouped. */
  if (group_size(victim) > 1) {
    counter = 0;
    for (i = 0; mob_cleric_area_spells[i]; i++) {
      if (!GET_SKILL(ch, mob_cleric_area_spells[i]))
        continue;
      switch (mob_cleric_area_spells[i]) {
        case SPELL_HOLY_WORD:
          /* Don't cast if it's suicidal or useless. */
          if (IS_EVIL(ch) || !evil_in_group(victim))
            continue;
          break;
        case SPELL_UNHOLY_WORD:
          if (IS_GOOD(ch) || !good_in_group(victim))
            continue;
          break;
        case SPELL_EARTHQUAKE:
          /* Only cast earthquake outside and in ground. */
          if (!QUAKABLE(CH_NROOM(ch)))
            continue;
          break;
      }
      if (mob_cast(ch, victim, NULL, mob_cleric_area_spells[i]))
        return TRUE;
      /* Only try the mob's best two spells. */
      if (++counter >= 2)
        break;
    }
  }

  /* Try to cause an offensive affection. Only attempt one. */
  for (i = 0; mob_cleric_hindrances[i].spell; i++) {
    if (!GET_SKILL(ch, mob_cleric_hindrances[i].spell))
      continue;
    if (!has_effect(victim, &mob_cleric_hindrances[i])) {
      if (mob_cast(ch, victim, NULL, mob_cleric_hindrances[i].spell))
        return TRUE;
      else
        break;
    }
  }

  /* Now attempt a harming spell. */
  for (i = 0, counter = 0; mob_cleric_offensives[i]; i++) {
    switch (mob_cleric_offensives[i]) {
      case SPELL_DISPEL_EVIL:
        if (!IS_GOOD(ch) || !IS_EVIL(victim))
          continue;
        break;
      case SPELL_DISPEL_GOOD:
        if (!IS_EVIL(ch) || !IS_GOOD(victim))
          continue;
        break;
      case SPELL_FLAMESTRIKE:
        if (!IS_GOOD(ch))
          continue; /* Not worth casting if alignment is too low */
        break;        
      case SPELL_DIVINE_RAY:
        if (GET_ALIGNMENT(ch) <= 650)
          continue;
        break;
      case SPELL_DESTROY_UNDEAD:
        if (GET_LIFEFORCE(victim) != LIFE_UNDEAD)
          continue;
        break;
      case SPELL_STYGIAN_ERUPTION:
        if (!IS_EVIL(ch))
          continue;
        break;
      case SPELL_HELL_BOLT:
        if (IS_EVIL(victim))
          continue;
        break;
      case SPELL_DIVINE_BOLT:
        if (IS_GOOD(victim))
          continue;
        break;
    }
    if (mob_cast(ch, victim, NULL, mob_cleric_offensives[i]))
      return TRUE;
    else
      counter++;
    /* Only attempt the mob's two best spells.  The rest are worthless. */
    if (counter > 2)
      break;
  }

  return FALSE;
}

/*
 * check_cleric_status
 *
 * Makes the cleric mob check its spells.  Unlike the sorcerer function of 
 * similar name, this one shouldn't be called when the mob is in combat.
 * Cleric spells are all pretty useless in battle.
 */
bool check_cleric_status(struct char_data *ch) {
  int i;

  /* Check bad affects */
  for (i = 0; mob_cleric_hindrances[i].spell; i++) {
    if (!GET_SKILL(ch, mob_cleric_hindrances[i].spell))
      continue;

    /* If the spell can be removed and the mob has it, try to remove it */
    if (mob_cleric_hindrances[i].remover && has_effect(ch, &mob_cleric_hindrances[i]))
      if (mob_cast(ch, ch, NULL, mob_cleric_hindrances[i].remover))
        return TRUE;
    /* 10% chance to cancel if in combat. */
    if (FIGHTING(ch) && !number(0, 9))
      return FALSE;
  }

  /* Check other spells */
  for (i = 0; mob_cleric_buffs[i].spell; i++) {
    if (!GET_SKILL(ch, mob_cleric_buffs[i].spell) ||
          !check_fluid_spell_ok(ch, ch, mob_cleric_buffs[i].spell, TRUE))
      continue;
    switch (mob_cleric_buffs[i].spell) {
      case SPELL_GAIAS_CLOAK:
        if (CH_INDOORS(ch) ||
            SECT(ch->in_room) == SECT_UNDERWATER ||
            SECT(ch->in_room) == SECT_UNDERDARK)
          continue;
        /* The armor spells don't mix. */
        if (affected_by_armor_spells(ch))
          continue;
        break;
      case SPELL_DEMONSKIN:
        if (IS_GOOD(ch))
          continue;
      case SPELL_ARMOR:
      case SPELL_BARKSKIN:
        /* The armor spells don't mix. */
        if (affected_by_armor_spells(ch))
          continue;
        break;
      case SPELL_DEMONIC_ASPECT:
      case SPELL_DEMONIC_MUTATION:
        /* Demonic mutation and demonic aspect don't mix. */
        if (affected_by_spell(ch, SPELL_DEMONIC_ASPECT) ||
            affected_by_spell(ch, SPELL_DEMONIC_MUTATION))
          continue;
        break;
      case SPELL_PROT_FROM_EVIL:
        if (IS_EVIL(ch) || EFF_FLAGGED(ch, EFF_PROTECT_EVIL))
          continue;
        break;
      case SPELL_SOULSHIELD:
        if (IS_NEUTRAL(ch))
          continue;
        if (EFF_FLAGGED(ch, EFF_SOULSHIELD))
          continue;
        break;
      case SPELL_DARK_PRESENCE:
        if (IS_GOOD(ch) || affected_by_spell(ch, SPELL_BLESS) || 
            affected_by_spell(ch, SPELL_DARK_PRESENCE))
          continue;
        break;
      default:
        if (has_effect(ch, &mob_cleric_buffs[i]))
          continue;
    }
    if (mob_cast(ch, ch, NULL, mob_cleric_buffs[i].spell))
      return TRUE;
  }

  return FALSE;
}

/***************************************************************************
 * $Log: cleric.c,v $
 * Revision 1.26  2009/07/04 16:23:13  myc
 * Soulshield, holy word, and unholy word now use regular alignment
 * checks instead of arbitrary 500 and -500 values.
 *
 * Revision 1.25  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.24  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.23  2008/09/14 02:24:11  jps
 * Using magic.h.
 *
 * Revision 1.22  2008/09/14 02:23:19  jps
 * Don't attempt to use buffs that can't be used when you're fluid.
 *
 * Revision 1.21  2008/09/09 08:23:37  jps
 * Placed sector info into a struct and moved its macros into rooms.h.
 *
 * Revision 1.20  2008/09/04 06:47:36  jps
 * Changed sector constants to match their strings
 *
 * Revision 1.19  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.18  2008/03/26 16:44:36  jps
 * Replaced all checks for undead race with checks for undead lifeforce.
 * Replaced the undead race with the plant race.
 *
 * Revision 1.17  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.16  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.15  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species into races.
 *
 * Revision 1.14  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.13  2008/01/24 15:44:37  myc
 * Fixed checks on some cleric spells in check_cleric_status.
 *
 * Revision 1.12  2008/01/13 03:19:53  myc
 * Got rid of MSKILL references.  Fixed some checks with cleric ai
 * spellcasting.
 *
 * Revision 1.11  2008/01/12 23:13:20  myc
 * Replaced try_cast with direct calls to mob_cast, which now supports target
 * objects.
 *
 * Revision 1.10  2008/01/12 19:08:14  myc
 * Rewrote a lot of mob AI functionality.
 *
 * Revision 1.9  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.8  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.7  2000/11/20 19:26:30  rsd
 * Delted some white space, and added back rlog comments
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.6  2000/11/15 03:58:56  rsd
 * Retabbed part of the file so I could figure out where to add
 * a giant conditional to prevent clerics from casting quake
 * inside when fighting a group and target an individual
 * instead *pant*
 *
 * Revision 1.5  2000/04/05 22:58:13  rsd
 * Altered the comment header to standard while browsing the file.
 *
 * Revision 1.4  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.3  1999/04/24 06:46:52  jimmy
 * dos2unix eeek ^M's --gurlaek
 *
 * Revision 1.2  1999/01/30 19:03:24  mud
 * Added standard comment header for credit 
 * Indented the file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/
