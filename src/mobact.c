/***************************************************************************
 * $Id: mobact.c,v 1.107 2009/03/19 23:16:23 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: mobact.c                                      Part of FieryMUD  *
 *  Usage: Functions for generating intelligent behavior in mobiles.       *
 *         Memory procedures are here, but combat AI is outsourced.        *
 *  All rights reserved.  See license.doc for complete information.        *
 *  Redesigned by: Ben Horner (Proky of HubisMUD)                          *
 *  Redesigned again by: Laoris of FieryMUD                                *
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
#include "interpreter.h"
#include "handler.h"
#include "casting.h"
#include "ai.h"
#include "races.h"
#include "skills.h"
#include "math.h"
#include "events.h"
#include "rooms.h"
#include "fight.h"
#include "movement.h"
#include "composition.h"
#include "directions.h"
#include "act.h"

/* external structs */
extern struct char_data *combat_list;   /* head of l-list of fighting chars */
extern struct char_data *next_combat_list;

/* External functions */
ACMD(do_stand);
ACMD(do_recline);
ACMD(do_sit);
ACMD(do_kneel);
ACMD(do_fly);
ACMD(do_sweep);
ACMD(do_breathe);
ACMD(do_roar);
ACMD(do_track);
ACMD(do_hide);
ACMD(do_sneak);
ACMD(do_douse);
bool update_inventory(struct char_data *ch, struct obj_data *obj, int where);
struct char_data* check_guard(struct char_data *ch, struct char_data *victim, int gag_output);
void get_check_money(struct char_data * ch, struct obj_data * obj);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
bool mob_cast(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int spellnum);
void hunt_victim (struct char_data *ch);
void start_memming(struct char_data *ch);

/* Local functions */
bool check_mob_status (struct char_data *ch);
void remember (struct char_data *ch, struct char_data *victim);
void mob_attempt_equip(struct char_data *ch);
bool mob_memory_action(struct char_data *ch, bool allow_spells);
void memory_attack_announce(struct char_data *ch, struct char_data *victim);
void memory_attack(struct char_data *ch, struct char_data *vict);
void track_victim_check(struct char_data *ch, struct char_data *vict);
bool mob_memory_check(struct char_data *ch);
/* mobile_activity subfunctions */
void mob_scavenge(struct char_data *ch);
bool mob_movement(struct char_data *ch);
bool mob_assist(struct char_data *ch);
void mob_attack(struct char_data *ch, struct char_data *victim);
bool check_spellbank(struct char_data *ch);

void mobile_activity(void) {
  register struct char_data *ch, *next_ch;
  extern int no_specials;

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    /* Players, sleeping mobs, and paralyzed, and casting mobs don't do
     * anything. */
    if (!IS_MOB(ch) || CASTING(ch) || !AWAKE(ch) ||
            EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) ||
            EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS) ||
            EFF_FLAGGED(ch, EFF_MESMERIZED))
      continue;

    /* Don't execute procs when someone is switched in. */
    if (POSSESSED(ch) || MEMMING(ch))
       continue;

    /* If lower than default position, get up. */
    if (GET_MOB_WAIT(ch) <= 0 && GET_DEFAULT_POS(ch) > GET_POS(ch) &&
        GET_STANCE(ch) >= STANCE_RESTING) {
      switch (GET_DEFAULT_POS(ch)) {
      case POS_PRONE:
        do_recline(ch, "", 0, 0);
        break;
      case POS_SITTING:
        do_sit(ch, "", 0, 0);
        break;
      case POS_KNEELING:
        do_kneel(ch, "", 0, 0);
        break;
      case POS_STANDING:
        do_stand(ch, "", 0, 0);
        break;
      case POS_FLYING:
        do_fly(ch, "", 0, 0);
        break;
      }
    }

    /* Execute any special procs. */
    if (MOB_PERFORMS_SCRIPTS(ch) && MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
      if (mob_index[GET_MOB_RNUM(ch)].func == NULL) {
        sprintf(buf, "%s (#%d): Attempting to call non-existing mob func",
                GET_NAME(ch), GET_MOB_VNUM(ch));
        log(buf);
        REMOVE_FLAG(MOB_FLAGS(ch), MOB_SPEC);
      }
      else if ((mob_index[GET_MOB_RNUM(ch)].func)(ch, ch, 0, ""))
        /* If it executes okay, go on to the next mob. */
        continue;
    }

    /* And now scavenger mobs pick up objects and wear them. */
    if (MOB_FLAGGED(ch, MOB_SCAVENGER) && !MOB_FLAGGED(ch, MOB_ILLUSORY)) {
      mob_scavenge(ch);
      mob_attempt_equip(ch);
    }

    /*
     * All AI activity past this point in the function should only occur
     * when the mobile is not in combat.
     */
    if (FIGHTING(ch))
      continue;

    /*
     * Mobs on fire attempt to douse themselves (50% chance to attempt).
     */
    if (EFF_FLAGGED(ch, EFF_ON_FIRE) && number(0, 1)) {
      /* If the mob is flagged !CLASS_AI, then just douse, don't try to cast. */
      if (MOB_FLAGGED(ch, MOB_NO_CLASS_AI) ? TRUE :
          !mob_cast(ch, ch, NULL, SPELL_EXTINGUISH))
        do_douse(ch, "", 0, 0);
      continue;
    }

    /*
     * None of the following actions should be taken by charmies.
     */
    if (EFF_FLAGGED(ch, EFF_CHARM) && ch->master &&
        ch->master->in_room != ch->in_room)
      continue;

    /*
     * These actions are special AI.  They're mainly class specific.
     */
    if (!MOB_FLAGGED(ch, MOB_NO_CLASS_AI)) {

      /*
       * This calls check_sorcerer_status() and check_cleric_status()
       * which cast spells like remove curse, etc.
       */
      if (check_mob_status(ch))
        continue;

      /*
       * Check the mob's spellbank and attempt to remem spells.
       */
      if (check_spellbank(ch))
        continue;

      /*
       * Only do these actions if not immortal, since immortals get
       * all skills, but we don't necessarily want them running
       * around stealing and making zombies.
       */
      if (GET_LEVEL(ch) < LVL_IMMORT) {

        /*
         * Attempt to hide/sneak for those mobs who can do it.
         */
        if (GET_SKILL(ch, SKILL_HIDE) && GET_HIDDENNESS(ch) == 0)
          do_hide(ch, "", 0, 0);

        /* Attempt to steal something from a player in the room. */
        if (GET_SKILL(ch, SKILL_STEAL) && number(0, 101) < GET_SKILL(ch, SKILL_STEAL))
          if (mob_steal(ch))
            continue;

        /* Attempt to make a zombie! */
        if (GET_SKILL(ch, SPELL_ANIMATE_DEAD) && !ch->followers &&
            !MOB_FLAGGED(ch, MOB_ANIMATED))
          if (mob_animate(ch))
            continue;
      }

    }

    /*
     * Mob movement
     *
     * Only move if the mob is standing or flying.
     */
    if (!MOB_FLAGGED(ch, MOB_SENTINEL) && GET_POS(ch) >= POS_STANDING)
      if (mob_movement(ch))
        continue;

    /*
     * Helper mobs
     *
     * This is where we assist.  A helper mob will assist any other mob,
     * unless it is fighting that mob, that mob is fighting a player
     * level 20 or less.
     */
    if (MOB_ASSISTER(ch) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
      if (mob_assist(ch))
        continue;

    /*
     * Slow track
     *
     * This is where slow track is taken care of, because mobile_activity
     * is called once every PULSE_MOBILE.  Tracking only occurs when the
     * mob isn't sentinel, isn't fighting, is marked slow track, is
     * marked memory, has someone in its memory.
     */
    if (!MOB_FLAGGED(ch, MOB_SENTINEL) && MOB_FLAGGED(ch, MOB_SLOW_TRACK) &&
        MOB_FLAGGED(ch, MOB_MEMORY))
      if (mob_memory_action(ch, FALSE))
        continue;
  }
}


/*
 * mobile_spec_activity
 *
 * This handles special mobile activity, called every PULSE_VIOLENCE.
 */
void mobile_spec_activity (void) {
  register struct char_data *ch, *next_ch, *vict;
  int found = FALSE;

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    /* Only for mobiles who aren't fighting, asleep, or casting */
    if (!IS_MOB(ch) || FIGHTING(ch) || !AWAKE(ch) || CASTING(ch))
      continue;

    /* Skip mobiles with someone switched in. */
    if (POSSESSED(ch))
      continue;

    /* Skip mobiles who are paralyzed. */
    if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS))
      continue;

    /* If mob isn't fighting, just decrease wait state. */
    if (GET_MOB_WAIT (ch) > 0) {
      WAIT_STATE(ch, MAX(GET_MOB_WAIT(ch) - PULSE_VIOLENCE, 0));
      continue;
    }

    /* Pets, courtesy of Banyal */
    if (PLAYERALLY(ch)) {
      found = FALSE;
      for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
        if (ch != vict && (ch->master == vict) && FIGHTING (vict) && ch != FIGHTING (vict)) {
          act("&7$n assists $s master!&0", FALSE, ch, 0, vict, TO_ROOM);
          attack(ch, check_guard(ch, FIGHTING(vict), FALSE));
          found = TRUE;
          break;
        }
      if (found)
        continue;
    }

    /* Should I start a fight? */

    /* Don't start a fight if you're too scared */
    if (GET_HIT(ch) < (GET_MAX_HIT(ch) >> 2) &&
        MOB_FLAGGED(ch, MOB_WIMPY))
      continue;

    /* Look for people I'd like to attack */
    if (!ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) &&
        !EFF_FLAGGED(ch, EFF_MESMERIZED) &&
        (!EFF_FLAGGED(ch, EFF_CHARM) || (ch->master &&
         ch->master->in_room != ch->in_room))) {
      if ((vict = find_aggr_target(ch))) {
        mob_attack(ch, vict);
        continue;
      }
    }

    /*
     * Simple mob memory
     *
     * Checks room for memory targets.
     */
    if (MOB_FLAGGED(ch, MOB_MEMORY) && mob_memory_check(ch))
        continue;

    /* If cleric-type and hurt, then heal. */
    if (!MOB_FLAGGED(ch, MOB_NO_CLASS_AI))
      if ((GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_DRUID ||
          GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_DIABOLIST ||
          GET_CLASS(ch) == CLASS_PALADIN || GET_CLASS(ch) == CLASS_RANGER) &&
          100 * GET_HIT(ch) / GET_MAX_HIT(ch) < 95)
        if (mob_heal_up(ch))
          continue;

    /*
     * Advanced mob memory
     *
     * Attempt to summon victim, dim door to victim, or just fast track.
     */
    if (GET_SKILL(ch, SPELL_SUMMON) || GET_SKILL(ch, SPELL_DIMENSION_DOOR) ||
        MOB_FLAGGED(ch, MOB_FAST_TRACK))
      if (mob_memory_action(ch, TRUE))
        continue;
  }
}


/*
   This function is called every other combat round.  It allows NPCs who
   are in battle to attempt special skills, such as bash or spellcasting.
*/
void perform_mob_violence(void)
{
   register struct char_data *ch;

   for (ch = combat_list; ch; ch = next_combat_list) {
      next_combat_list = ch->next_fighting;
      if (IS_NPC(ch) && !ch->desc) {
         if (GET_POS(ch) > POS_KNEELING)
            GET_STANCE(ch) = STANCE_FIGHTING;

         if (GET_MOB_WAIT(ch) > 0)
            /* If the NPC is delayed, continue waiting */
            WAIT_STATE(ch, MAX ((GET_MOB_WAIT(ch) - (PULSE_VIOLENCE / 2)), 0));
         else {
            /* This NPC is ready to perform actions */
            if (GET_POS(ch) < POS_STANDING) {
               GET_POS(ch) = POS_STANDING;
               GET_STANCE(ch) = STANCE_FIGHTING;
               act("&0&3$n scrambles to $s feet!&0", TRUE, ch, 0, 0, TO_ROOM);
            } else
               mob_attack(ch, FIGHTING(ch));
         }
      }
   }
}


bool mob_movement(struct char_data *ch) {
  struct char_data *vict;
  int door = number(0, 18);

  if (door >= NUM_OF_DIRS || !CAN_GO(ch, door))
    return FALSE;
  if (ROOM_FLAGGED(CH_NDEST(ch, door), ROOM_NOMOB) ||
      ROOM_FLAGGED(CH_NDEST(ch, door), ROOM_DEATH))
    return FALSE;
  if (MOB_FLAGGED(ch, MOB_STAY_ZONE) && CH_DEST(ch, door)->zone != CH_ROOM(ch)->zone)
    return FALSE;
  if (EFF_FLAGGED(ch, EFF_CHARM) && !MOB_FLAGGED(ch, MOB_ILLUSORY))
    return FALSE;

  /* Don't wander off while someone is fighting you! */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch)
      return FALSE;

  if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_ISOLATION)) {
    act("$n looks vainly about for an exit.", TRUE, ch, 0, 0, TO_ROOM);
    /* True since it did *something* (looking around) */
    return TRUE;
  }

  /* If we made it through all the checks above, then move! */

  /* Mobs with misdirection like to be sneaky */
  if (EFF_FLAGGED(ch, EFF_MISDIRECTION)) {
     if (number(1, NUM_OF_DIRS) == 1) {
        /* Decided to stay but pretend to move */
        perform_move(ch, door, 1, TRUE);
     } else {
        /* Decided to move */
        perform_move(ch, number(0, NUM_OF_DIRS - 1), 1, TRUE);
        SET_FLAG(EFF_FLAGS(ch), EFF_MISDIRECTING);
        perform_move(ch, door, 1, FALSE);
        REMOVE_FLAG(EFF_FLAGS(ch), EFF_MISDIRECTING);
     }
  } else
    perform_move(ch, door, 1, FALSE);
  return TRUE;
}


bool mob_assist(struct char_data *ch) {
  struct char_data *vict, *cvict = NULL;

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
    if (!will_assist(ch, vict))
      continue;
    /*
    if (ch == vict || !FIGHTING(vict) || ch == FIGHTING(vict))
      continue;
    if (FIGHTING(vict)->in_room != ch->in_room)
      continue;
    if (!IS_NPC(vict) || MOB_FLAGGED(vict, MOB_PLAYER_PHANTASM))
      continue;
    if (IS_NPC(FIGHTING(vict)) && !MOB_FLAGGED(FIGHTING(vict), MOB_PLAYER_PHANTASM))
      continue;
      */
    if (GET_LEVEL(FIGHTING(vict)) <= 20)
      cvict = vict;
    else if (EFF_FLAGGED(FIGHTING(vict), EFF_FAMILIARITY) &&
          number(1, 100 + GET_CHA(FIGHTING(vict)) - GET_CHA(vict)) < 50) {
      act("$n moves to join the fight, but gets a good look at $N and stops, confused.",
            TRUE, ch, 0, FIGHTING(vict), TO_ROOM);
      act("You want to help, but $N looks like your friend too!",
            FALSE, ch, 0, FIGHTING(vict), TO_CHAR);
      return FALSE;
    } else {
      act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
      mob_attack(ch, FIGHTING(vict));
      return TRUE;
    }
  }
  if (cvict) {
    int chuckle = number(1, 10);
    if (chuckle == 1)
      act("$n watches the battle in amusement.", FALSE, ch, 0, cvict, TO_ROOM);
    else if (chuckle == 2)
      act("$n chuckles as $e watches the fight.", FALSE, ch, 0, cvict, TO_ROOM);
    else if (chuckle == 3) {
      act("$n takes note of your battle tactics.", FALSE, ch, 0, FIGHTING(cvict), TO_CHAR);
      act("$n takes note of $N's battle tactics.", FALSE, ch, 0, FIGHTING(cvict), TO_ROOM);
    }
  }

  return FALSE;
}


void mob_attack(struct char_data *ch, struct char_data *victim)
{
   if (CASTING(ch))
      return;

   /* See if anyone is guarding the victim.
    * But guarding doesn't apply if this NPC was already fighting the victim. */
   if (FIGHTING(ch) != victim)
      victim = check_guard(ch, victim, FALSE);

   /* Mob should not execute any special mobile AI procedures. */
   if (!MOB_FLAGGED(ch, MOB_NO_CLASS_AI)) {

      /* Attempt special class actions first. */
      switch (GET_CLASS(ch)) {
         case CLASS_RANGER:
         case CLASS_PALADIN:
         case CLASS_ANTI_PALADIN:
            if (cleric_ai_action(ch, victim))
               return;
            /* Otherwise go on down to warrior ai action. */
         case CLASS_WARRIOR:
         case CLASS_MONK:
         case CLASS_MERCENARY:
         case CLASS_BERSERKER:
            if (warrior_ai_action(ch, victim))
               return;
            break;
         case CLASS_CLERIC:
         case CLASS_DRUID:
         case CLASS_DIABOLIST:
         case CLASS_PRIEST:
         case CLASS_SHAMAN:
            victim = weakest_attacker(ch, victim);
            if (cleric_ai_action(ch, victim))
               return;
            break;
         case CLASS_SORCERER:
         case CLASS_PYROMANCER:
         case CLASS_CRYOMANCER:
         case CLASS_NECROMANCER:
         case CLASS_ILLUSIONIST:
         case CLASS_CONJURER:
            victim = weakest_attacker(ch, victim);
            if (sorcerer_ai_action(ch, victim))
               return;
            break;
         case CLASS_ASSASSIN:
         case CLASS_THIEF:
         case CLASS_ROGUE:
         case CLASS_BARD:
            if (rogue_ai_action(ch, victim))
               return;
            break;
      }

      switch (GET_RACE(ch)) {
         case RACE_DRAGON:
         case RACE_DEMON:
            if (dragonlike_attack(ch))
               return;
            break;
      }
   }

   if (!FIGHTING(ch))
      attack(ch, victim);
}


void mob_scavenge(struct char_data *ch) {
  int best_value, value;
  struct obj_data *best_obj, *obj;

  /* If there's nothing to pick up, return.  50% chance otherwise. */
  if (!world[ch->in_room].contents || !number(0, 1))
    return;

  best_value = 0;
  best_obj = NULL;

  /* Find the most desirable item in the room. */
  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
    if (CAN_GET_OBJ(ch, obj) && (value = appraise_item(ch, obj)) > best_value) {
      best_obj = obj;
      best_value = value;
    }

  /* And then pick it up. */
  if (best_obj != NULL) {
    struct get_context *context = begin_get_transaction(ch);
    perform_get_from_room(context, best_obj);
    end_get_transaction(context, NULL);
  }
}

/*
 * mob_attempt_equip
 *
 * Makes the mob attempt to wear an item in its inventory.
 * Returns 1 as soon as something is worn or removed.  0
 * otherwise.
 */
void mob_attempt_equip(struct char_data *ch) {
  int where;
  struct obj_data *obj;

  if (GET_RACE(ch) == RACE_ANIMAL)
    return;

  for (obj = ch->carrying; obj; obj = obj->next_content) {
    if (!CAN_SEE_OBJ(ch, obj))
      continue;

    where = find_eq_pos(ch, obj, NULL);
    if (where >= 0) {
      if (GET_EQ(ch, where)) {
        if (appraise_item(ch, GET_EQ(ch, where)) >= appraise_item(ch, obj))
          continue;
        perform_remove(ch, where);
      }
      perform_wear(ch, obj, where);
      continue;
    }
    else { /* where < 0 means not wearable */
      /* Maybe handle other types of items here; eat food, I dunno */
    }
  }
}

/*
 * mob_memory_action
 *
 * Expects a mob with the MOB_MEMORY flag.
 */
bool mob_memory_action(struct char_data *ch, bool allow_spells) {
  struct descriptor_data *d, *next_d;
  memory_rec *names;

  if (!MEMORY(ch))
    return FALSE;

  /* Check everyone logged on against the memory. */
  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (STATE(d) != CON_PLAYING)
      continue;

    /* Check all the names in memory against this descriptor. */
    for (names = MEMORY(ch); names; names = names->next) {
      if (names->id != GET_IDNUM(d->character))
        continue;

      if (allow_spells && !MOB_FLAGGED(ch, MOB_NO_CLASS_AI)) {
        /* Now attempt spells that require the tracker and victim to be
           in the same zone. */
        if (world[ch->in_room].zone == world[d->character->in_room].zone) {
          /* First try to summon. */
          if (GET_LEVEL(d->character) <= GET_SKILL(ch, SKILL_SPHERE_SUMMON) + 3)
            if (mob_cast(ch, d->character, NULL, SPELL_SUMMON))
              return TRUE;
          /* Then try to dimension door. */
          if (mob_cast(ch, d->character, NULL, SPELL_DIMENSION_DOOR))
            return TRUE;
        }
      }

      if ((MOB_FLAGGED(ch, MOB_SLOW_TRACK) || MOB_FLAGGED(ch, MOB_FAST_TRACK))
            && !EVENT_FLAGGED(ch, EVENT_TRACK)) {
        do_track(ch, GET_NAMELIST(d->character), 0, 0);
        track_victim_check(ch, d->character);
        return TRUE;
      }
    }
  }
  return FALSE;
}

bool mob_memory_check(struct char_data *ch) {
  struct char_data *vict;
  memory_rec *names;

  if (!MEMORY(ch))
    return FALSE;

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
    if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
      continue;

    for (names = MEMORY(ch); names; names = names->next) {
      if (names->id != GET_IDNUM(vict))
        continue;
      track_victim_check(ch, vict);
      return TRUE;
    }
  }
  return FALSE;
}

void memory_attack_announce(struct char_data *ch, struct char_data *vict)
{
  /* No announcement if the mob is trying to be sneaky. */
  if (EFF_FLAGGED(ch, EFF_SNEAK) || GET_HIDDENNESS(ch) > 0)
    return;

  /* No announcement if the room is peaceful. (Because there's no attack.) */
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
    return;

  if (EFF_FLAGGED(ch, EFF_SILENCE)) {
    act("$n silently moves in for the attack.",
        TRUE, ch, 0, vict, TO_ROOM);
  }
  else if (GET_RACE(ch) == RACE_ANIMAL) {
    act("$n growls angrily at $N, and attacks!",
        FALSE, ch, 0, vict, TO_NOTVICT);
    act("$n growls angrily at you, and attacks!",
        FALSE, ch, 0, vict, TO_VICT);
  }
  else {
    switch(number(1, 10)) {
      case 1:
        act("$n growls, 'Thought you could walk away from a fight, eh?'",
            FALSE, ch, 0, vict, TO_ROOM);
        break;
      case 2:
        /* Using GET_NAME because a mob wouldn't growl "someone" */
        sprintf(buf, "$n growls, 'This ends here and now, %s!'",
              GET_NAME(vict));
        act(buf, FALSE, ch, 0, vict, TO_ROOM);
        break;
      case 10:
      default:
        act("$n snarls, 'You're not getting away that easily!'",
            FALSE, ch, 0, vict, TO_ROOM);
        break;
      /*
       * Add more messages here for variety!
       */
    }
  }
}

void track_victim_check(struct char_data *ch, struct char_data *vict)
{
   if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
     return;
   if (ch->in_room == vict->in_room ||
       ((vict = find_char_around_char(ch, find_vis_by_name(ch, GET_NAME(vict)))) &&
       MOB_FLAGGED(vict, MOB_PLAYER_PHANTASM)))
     if (!CASTING(ch) && !FIGHTING(ch)) {
       memory_attack_announce(ch, vict);
       mob_attack(ch, vict);
     }
}



/* Add victim to ch's memory list. */
void remember(struct char_data *ch, struct char_data *victim) {
  memory_rec *tmp;
  bool present = FALSE;

  if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
    return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present) {
    CREATE(tmp, memory_rec, 1);
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    MEMORY(ch) = tmp;
  }
}


/* Make character forget a victim. */
void forget(struct char_data *ch, struct char_data *victim)
{
  memory_rec *curr, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while (curr && curr->id != GET_IDNUM(victim)) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return;   /* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}

/* erase ch's memory */
void clear_memory(struct char_data *ch) {
  memory_rec *curr, *next;

  curr = MEMORY(ch);

  while (curr) {
    next = curr->next;
    free (curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}


/* Returns true if victim is in the mob's memory, false otherwise */
bool in_memory(struct char_data *ch, struct char_data *vict) {
  memory_rec *memory;

  for (memory = MEMORY(ch); memory; memory = memory->next)
    if (memory->id == GET_IDNUM(vict))
      return TRUE;

  return FALSE;
}


/* This function checks mobs for spell ups */
bool check_mob_status (struct char_data *ch) {
  switch (GET_CLASS(ch)) {
    case CLASS_SORCERER:
    case CLASS_PYROMANCER:
    case CLASS_CRYOMANCER:
    case CLASS_NECROMANCER:
    case CLASS_ILLUSIONIST:
      if (check_sorcerer_status(ch))
        return TRUE;
      break;
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_PRIEST:
    case CLASS_DIABOLIST:
    case CLASS_PALADIN:
    case CLASS_ANTI_PALADIN:
      /* check_cleric_status is useless in combat */
      if (!FIGHTING(ch) && check_cleric_status(ch))
        return TRUE;
      break;
  }
  return FALSE;
}

bool check_spellbank(struct char_data *ch)
{
  int i;
  /* If any spell circles are not fully charged, start memming and 
   * return TRUE */
  for (i = 1; i < NUM_SPELL_CIRCLES; ++i)
    if (GET_MOB_SPLBANK(ch, i) < spells_of_circle[(int) GET_LEVEL(ch)][i]) {
      do_sit(ch, NULL, 0, 0);
      if (GET_POS(ch) == POS_SITTING &&
          GET_STANCE(ch) >= STANCE_RESTING &&
          GET_STANCE(ch) <= STANCE_ALERT) {
        act(MEM_MODE(ch) == PRAY ? "$n begins praying to $s deity." :
            "$n takes out $s books and begins to study.",
            TRUE, ch, 0, 0, TO_ROOM);
        start_memming(ch);
        return TRUE;
      }
    }
  return FALSE;
}

void remove_from_all_memories(struct char_data *ch) {
  register struct char_data *tch, *next_tch;

  for (tch = character_list; tch; tch = next_tch) {
    next_tch = tch->next;

    if (!MOB_FLAGGED(tch, MOB_MEMORY) || !MEMORY(tch))
      continue;
    if (tch == ch)
      continue;
    if (POSSESSED(tch))
      continue;
    else
      forget(tch, ch);
  }
}

bool dragonlike_attack(struct char_data *ch) {
  int roll = number(0, 125 - GET_LEVEL(ch));

  /* At level 100, 10% chance to breath, 2% chance for each different type */
  if (roll < 5 && GET_SKILL(ch, SKILL_BREATHE)) {
    switch (GET_COMPOSITION(ch)) {
    case COMP_EARTH:
    case COMP_STONE:
      do_breathe(ch, "acid", 0, 0);
      break;
    case COMP_AIR:
    case COMP_ETHER:
      do_breathe(ch, "lightning", 0, 0);
      break;
    case COMP_FIRE:
    case COMP_LAVA:
      do_breathe(ch, "fire", 0, 0);
      break;
    case COMP_WATER:
    case COMP_ICE:
    case COMP_MIST:
      do_breathe(ch, "frost", 0, 0);
      break;
    case COMP_METAL:
    case COMP_BONE:
    case COMP_PLANT:
      do_breathe(ch, "gas", 0, 0);
      break;
    default:
      switch (roll) {
      case 0:
        do_breathe(ch, "fire", 0, 0);
        break;
      case 1:
        do_breathe(ch, "gas", 0, 0);
        break;
      case 2:
        do_breathe(ch, "frost", 0, 0);
        break;
      case 3:
        do_breathe(ch, "acid", 0, 0);
        break;
      case 4:
      default:
        do_breathe(ch, "lightning", 0, 0);
        break;
      }
    }
    return TRUE;
  }

  /* At level 100, 10% chance to sweep */
  else if (roll < 10 && GET_SKILL(ch, SKILL_SWEEP)) {
    do_sweep(ch, "", 0, 0);
    return TRUE;
  }

  /* At level 100, 10% chance to roar */
  else if (roll < 15 && GET_SKILL(ch, SKILL_ROAR)) {
    int cmd_roar = find_command("roar");
    /*
     * Since do_roar can map to do_action, we MUST find the correct
     * cmd value for roar here.  If we do not and the roar fails, it
     * may get passed to do_action, which will cause the game to
     * crash if passed an invalid cmd value.
     */
    do_roar(ch, "", cmd_roar, 0);
    return TRUE;
  }
  return FALSE;
}

/***************************************************************************
 * $Log: mobact.c,v $
 * Revision 1.107  2009/03/19 23:16:23  myc
 * Moved get command to its own file.
 *
 * Revision 1.106  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.105  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.104  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.103  2009/03/03 19:43:44  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.102  2009/02/18 19:25:57  myc
 * Paladins and anti-paladins will try to cast soulshield etc now.
 *
 * Revision 1.101  2009/02/04 21:07:25  myc
 * Mobs using dragonbreath will attempt to match type to composition.
 *
 * Revision 1.100  2009/01/19 09:25:23  myc
 * Removed MOB_PET flag.
 *
 * Revision 1.99  2009/01/17 00:28:02  myc
 * Fix possible use of uninitialized variable.
 *
 * Revision 1.98  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.97  2008/09/20 06:05:06  jps
 * Add macros POSSESSED and POSSESSOR.
 *
 * Revision 1.96  2008/09/13 18:52:51  jps
 * Cause mobs to scan for victims even if they aren't aggressive to players.
 * They could be aggressive for other reasons.
 *
 * Revision 1.95  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.94  2008/08/31 20:54:08  jps
 * Using will_assist() function which is in ai_utils.c.
 *
 * Revision 1.93  2008/08/31 17:04:19  myc
 * Remove debug message from mob tracking.
 *
 * Revision 1.92  2008/08/25 00:20:33  myc
 * Changed the way mobs memorize spells.
 *
 * Revision 1.91  2008/08/15 04:07:08  jps
 * Fix isolation check
 *
 * Revision 1.90  2008/08/10 16:46:49  jps
 * Stop mobs from wandering out of rooms with ISOLATION.
 *
 * Revision 1.89  2008/05/20 02:38:17  jps
 * Fix mob attack declaration.
 *
 * Revision 1.88  2008/05/19 06:51:39  jps
 * Cause illusionists to cast sorcerer spells.
 *
 * Revision 1.87  2008/05/19 06:33:53  jps
 * Cause wandering mobs to make use of misdirection.
 *
 * Revision 1.86  2008/05/19 05:47:50  jps
 * Don't allow actions when mesmerized.
 *
 * Revision 1.85  2008/05/18 20:46:19  jps
 * Helper mobs may hesitate when the one they would attack has FAMILIARITY.
 *
 * Revision 1.84  2008/05/18 20:15:57  jps
 * Moved glory checking to find_aggr_target.
 *
 * Revision 1.83  2008/05/18 03:49:13  jps
 * Try to stop wimpy mobs from attacking when they're hurt.
 *
 * Revision 1.82  2008/05/17 22:03:01  jps
 * Moving room-related code into rooms.h and rooms.c.
 *
 * Revision 1.81  2008/04/20 03:54:28  jps
 * Make mobs use the same tracking as players.
 *
 * Revision 1.80  2008/04/19 20:17:27  jps
 * Stop intra-fight mob attacks from invoking the guard response.
 *
 * Revision 1.79  2008/04/18 16:13:05  mud
 * Make illusionists do sorcerer AI.
 *
 * Revision 1.78  2008/04/14 02:19:18  jps
 * Implementing EFF_GLORY.  Aggressive mobs are distracted and probably won't
 * attack while someone with GLORY is in their presence.
 *
 * Revision 1.77  2008/04/10 01:38:38  myc
 * Fix handling of default position handling in mob ai so they'll
 * stop standing when flying.
 *
 * Revision 1.76  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.75  2008/04/04 06:12:52  myc
 * Removed justice code.
 *
 * Revision 1.74  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.73  2008/03/17 15:31:27  myc
 * WAIT_STATE macro changed internally.
 *
 * Revision 1.72  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.
 *
 * Revision 1.71  2008/03/10 18:01:17  myc
 * Bug: was setting POS1 to POS_STANDING.
 *
 * Revision 1.70  2008/03/09 18:10:32  jps
 * perform_move may be misdirected now.
 *
 * Revision 1.69  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.68  2008/03/08 23:54:54  jps
 * Check whether a mob performs scripts when initiating specprocs.
 *
 * Revision 1.67  2008/02/11 21:04:01  myc
 * Mobs won't try to autocast summon if the target is too high level.
 *
 * Revision 1.66  2008/02/09 21:07:50  myc
 * Casting macro now checks event flags instead of mob flags.
 *
 * Revision 1.65  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.64  2008/02/01 17:55:09  myc
 * Fixed a crash bug where do_roar was being called with an invalid
 * cmd value.
 *
 * Revision 1.63  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.62  2008/01/28 00:14:32  jps
 * Fix rogue NPC attacking.
 *
 * Revision 1.61  2008/01/27 21:14:59  myc
 * Replace hit() with attack().
 *
 * Revision 1.60  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species into races.
 *
 * Revision 1.59  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.58  2008/01/22 14:59:23  jps
 * Fix chuckles-at-fight bug.
 *
 * Revision 1.57  2008/01/21 02:44:26  myc
 * Make peaceful mobs not assist.
 *
 * Revision 1.56  2008/01/20 23:18:52  myc
 * Fixed mob AI to only leave out class actions.
 *
 * Revision 1.55  2008/01/16 04:12:00  myc
 * Moving pick_target from here to fight.c (renamed find_aggr_target).
 *
 * Revision 1.54  2008/01/14 21:28:32  myc
 * Moved is_aggr_to to fight.c.
 *
 * Revision 1.53  2008/01/14 20:38:42  myc
 * Fixed a crash bug (infinite recursion) in is_aggr_to.
 *
 * Revision 1.52  2008/01/13 23:06:04  myc
 * Removed the NumAttackers function.  Changed mob_scavenge to use
 * appraise_item in ai_utils.
 *
 * Revision 1.51  2008/01/13 03:19:53  myc
 * Made immortal mobs not use hide, steal, and animate dead.
 * Added a check_guard call.
 *
 * Revision 1.50  2008/01/12 23:13:20  myc
 * Cleaned up mobact a bunch.  Subfunctionalized a bunch of actions, added a few,
 * and replaced try_cast with direct calls to mob_cast, which now supports
 * target objects.
 *
 * Revision 1.49  2008/01/12 19:12:05  myc
 * Oops, accidentally erased some of the log entries.
 *
 * Revision 1.48  2008/01/12 19:08:14  myc
 * Rerowte a lot of mob AI functionality.
 *
 * Revision 1.47  2008/01/07 11:57:57  jps
 * Allow illusory mobs to wander.
 *
 * Revision 1.46  2008/01/07 10:34:53  jps
 * Allow mobs to be aggressive to player phantasms.  Mobs will also pause
 * in their tracking to kill such, but won't be fooled once the phantasm dies.
 *
 * Revision 1.45  2008/01/06 23:50:47  jps
 * Added spells project and simulacrum, and MOB2_ILLUSORY flag.
 *
 * Revision 1.44  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.43  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.42  2007/10/27 03:18:58  myc
 * Fixed bug in CAN_SEE so mobs can see without lights.  Removed MCAN_SEE
 * since it does the same thing as CAN_SEE.
 *
 * Revision 1.41  2007/09/08 23:19:23  jps
 * Stop memory mobs from blabbering when they try to start fights
 * in peaceful rooms.
 *
 * Revision 1.40  2007/09/07 19:51:30  jps
 * Added more checks for death after special initial attack, in
 * response to another aggressive-mob-killing-player crash bug.
 *
 * Revision 1.39  2007/08/29 01:22:18  jps
 * An assisting mob's initial special attack might well kill its
 * target.  Made the code check for that, and not press the
 * attack further if so.
 *
 * Revision 1.38  2007/08/26 01:55:41  myc
 * Fire now does real damage.  All fire spells have a chance to catch the
 * victim on fire.  Mobs attempt to douse themselves.
 *
 * Revision 1.37  2007/08/16 11:53:58  jps
 * Remove references to defunct specprocs.
 *
 * Revision 1.36  2007/08/05 22:19:17  myc
 * Set POS1 in addition to POS when a mob scrambles back to its feet.
 *
 * Revision 1.35  2007/07/18 21:05:16  jps
 * Make mobs behave mercifully toward other mobs only when
 * they are flagged NOVICIOUS.
 *
 * Revision 1.34  2007/07/18 17:02:25  jps
 * Stop helpers from rushing to assist their buddy if their buddy's
 * attacker has left the room.
 *
 *
 * Revision 1.33  2007/04/11 16:05:27  jps
 * Scavengers who pick up money won't end up with the money pile object in inventory.
 *
 * Revision 1.32  2007/03/27 04:27:05  myc
 * Fixed typo in battle message.
 *
 * Revision 1.31  2007/02/08 01:32:20  myc
 * Hopefully no crashes when mobs are killed by circle of fire now.
 *
 * Revision 1.30  2006/12/08 04:01:13  myc
 * Aggro mobs don't all announce their attack like memory mobs now.
 *
 * Revision 1.29  2006/11/27 00:17:00  jps
 * Fix mob tracking oops
 *
 * Revision 1.28  2006/11/20 09:03:58  jps
 * Mobs with memory attack - not just the ones with track.
 * They also give some announcement or make noise as they do.
 *
 * Revision 1.27  2006/11/20 07:25:34  jps
 * Stop mobs from wandering off in the middle of a battle, such as
 * when they get up after being bashed.
 *
 * Revision 1.26  2006/11/17 22:52:59  jps
 * Change AGGR_GOOD/EVIL_ALIGN to AGGR_GOOD/EVIL_RACE
 *
 * Revision 1.25  2002/09/30 01:12:32  jjl
 * Added checks so guard does something.
 *
 * Revision 1.24  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.23  2001/04/07 03:02:59  dce
 * Mobs will no longer be agrresive to each other with the
 * MOB_AGGRESSIVE flag...a new flag will be coming soon.
 *
 * Revision 1.22  2000/11/28 00:45:28  mtp
 * removed mobprog stuff
 *
 * Revision 1.21  2000/11/24 18:42:20  rsd
 * Added back rlog messages from prior to the addition of
 * the $log$ string.
 *
 * Revision 1.20  2000/04/24 20:00:41  rsd
 * Ok, mobiles really don't wear eq and wield weapons now,
 * I swear!
 *
 * Revision 1.19  2000/04/22 22:38:37  rsd
 * Fixed comment header, fixed logged information about weapon exchange
 * to include ch's name. Also encouraged animals not to try to wield weapons.
 *
 * Revision 1.18  2000/02/25 03:32:11  cso
 * fixing typos.. guard chuckles at you as he watches the fight
 *
 * Revision 1.17  1999/12/06 20:21:04  cso
 * Fixed a typo in line "takes note of your battle tactics."
 *
 * Revision 1.16  1999/11/28 23:51:54  cso
 * added check to mobile_activity to keep charmed mobs from wandering
 *
 * Revision 1.15  1999/11/23 15:48:23  jimmy
 * Fixed the slashing weapon skill.  I had it erroneously as stabbing. Doh.
 * Reinstated dual wield.
 * Allowed mobs/players to pick up items while fighting.
 * Fixed a bug in the damage message that wrongfully indicated a miss
 * due to a rounding error in the math.
 * This was all done in order to facilitate the chance to sling your
 * weapon in combat.  Dex and proficiency checks are now made on any missed
 * attact and a failure of both causes the weapon to be slung.
 *
 * Revision 1.14  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.13  1999/09/04 18:46:52  jimmy
 * More small but important bug fixes found with insure.  These are all runtime fixes.
 *
 * Revision 1.12  1999/07/25 05:40:18  jimmy
 * Fixed the skill advancement in peace rooms bug.
 * --gurlaek
 *
 * Revision 1.11  1999/07/14 19:24:03  jimmy
 * The combat system was enhanced/improved in the following ways:  Mobs
 * can no longer flee while bashed or sitting.  Fleeing causes casters to
 * stop casting.  You can now flee while flying.  pk checks were added to
 * bash, bodyslam, throatcut, etc etc.  Lots of reformatting and little
 * fixes. spellcasting for mobs is now very similar to PC spellcasting.
 * MObs will now unhide/unconceal/univis/ etc when casting offensive spells.
 * Mobs no longer improve skills.  Bash now requires mobs to have a shield
 * just like PC's.  It's aT 25% with no shield and 50% with a 2handed weapon.
 * --gurlaek
 *
 * Revision 1.10  1999/07/12 02:58:46  jimmy
 * Lots of formatting and general cleanup. No code changes.
 * gurlaek.
 *
 * Revision 1.9  1999/07/06 19:57:05  jimmy
 * This is a Mass check-in of the new skill/spell/language assignment system.
 * This New system combines the assignment of skill/spell/language for
 * both mobs and PCs.  LOts of code was touched and many errors were fixed.
 * MCLASS_VOID was moved from 13 to -1 to match CLASS_UNDEFINED for PC's.
 * MObs now get random skill/spell/language levels baseed on their race/class/level
 * that exactly align with PC's.  PC's no longer have to rent to use skills gained
 * by leveling or when first creating a char.  Languages no longer reset to defaults
 * when a PC levels.  Discovered that languages have been defined right in the middle
 * of the spell area.  This needs to be fixed.  A conversion util neeDs to be run on
 * the mob files to compensate for the 13 to -1 class change.
 * --gurlaek 7/6/1999
 *
 * Revision 1.8  1999/05/02 19:32:34  dce
 * Temporarly make fast track = slow track
 *
 * Revision 1.7  1999/04/04 04:05:12  dce
 * Mob fighting fixes
 *
 * Revision 1.6  1999/04/04 03:42:41  dce
 * Mobs don't atack each other...only if AGGR_ALIGN.
 *
 * Revision 1.5  1999/03/31 22:06:06  dce
 * Allows flying mobs to wander.
 *
 * Revision 1.4  1999/03/22 18:57:32  tph
 * mobs nolonger try_cast in NOMAGIC rooms.
 *
 * Revision 1.3  1999/02/04 21:07:27  jimmy
 * Made mobs aggressive to each other.
 * fingon
 *
 * Revision 1.2  1999/01/31 17:19:29  mud
 * Added a standard comment header
 * Indented the entire file
 * took out some ranges of blank space
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/
