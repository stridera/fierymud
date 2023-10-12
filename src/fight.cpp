/***************************************************************************
 *   File: fight.c                                        Part of FieryMUD *
 *  Usage: Combat system                                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "fight.hpp"

#include "act.hpp"
#include "casting.hpp"
#include "clan.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "cooldowns.hpp"
#include "corpse_save.hpp"
#include "damage.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "directions.hpp"
#include "events.hpp"
#include "exits.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "lifeforce.hpp"
#include "limits.hpp"
#include "logging.hpp"
#include "magic.hpp"
#include "math.hpp"
#include "movement.hpp"
#include "pfiles.hpp"
#include "players.hpp"
#include "races.hpp"
#include "regen.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "specprocs.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "trophy.hpp"
#include "utils.hpp"

/* Globals */

AttackHitType attack_hit_text[] = {{"hit", "hits"}, /* 0 */
                                   {"sting", "stings"},   {"whip", "whips"},         {"slash", "slashes"},
                                   {"bite", "bites"},     {"bludgeon", "bludgeons"}, /* 5 */
                                   {"crush", "crushes"},  {"pound", "pounds"},       {"claw", "claws"},
                                   {"maul", "mauls"},     {"thrash", "thrashes"}, /* 10 */
                                   {"pierce", "pierces"}, {"blast", "blasts"},       {"punch", "punches"},
                                   {"stab", "stabs"},     {"burn", "burns"}, /* 15 */
                                   {"freeze", "freezes"}, {"corrode", "corrodes"},   {"shock", "shocks"},
                                   {"poison", "poisons"}, {"smite", "smites"}}; /* 20 */

CharData *combat_list = nullptr; /* head of l-list of fighting chars */
CharData *next_combat_list = nullptr;

/* External procedures */
char *fread_action(FILE *fl, int nr);
char *fread_string(FILE *fl, char *error);
void decrease_modifier(CharData *i, int spell);
void check_new_surroundings(CharData *ch, bool old_room_was_dark, bool tx_obvious);
bool in_memory(CharData *ch, CharData *tch);
long exp_death_loss(CharData *ch, int level);
EVENTFUNC(die_event);

ACMD(do_get);
ACMD(do_return);
CharData *check_guard(CharData *ch, CharData *victim, int gag_output);
void remove_from_all_memories(CharData *ch);
void abort_casting(CharData *ch);
void aggro_lose_spells(CharData *ch);

/****************************/
/*  General target linking  */
/****************************/

/* The following functions - set_battling and stop_battling - are utility
 * functions that simply link or unlink attackers and their targets, and make
 * sure that people who are fighting are in the mudwide combat list.
 *
 * Any and all sanity checks, permissions, and so forth that govern whether
 * the battle may take place must be performed before calling these functions.
 *
 * These are the only functions that should modify ch->target.  Call these
 * functions instead of setting the value elsewhere. */

/* Cause ch to be battling target */
void set_battling(CharData *ch, CharData *target) {
    CharData *c; /* Error check/debug */
    static int set_battling_corecount_1 = 0;
    static int set_battling_corecount_2 = 0;
    static int set_battling_corecount_3 = 0;

    /* Caller should not blindly set one person fighting another, when the
     * attacker is already in battle.  That's a special circumstance, like the
     * switch skill, and the caller should be aware of the situation and call
     * stop_battling() if it decides that ch will change its target */
    if (ch->target) {
        log(LogSeverity::Error, LVL_GOD, "set_battling: {} is already battling {}", GET_NAME(ch), GET_NAME(ch->target));
        if (set_battling_corecount_1 < 3) {
            drop_core(nullptr, "set_battling_I");
            log(LogSeverity::Error, LVL_GOD, "set_battling: core dropped (I)");
            set_battling_corecount_1++;
        }
        stop_battling(ch);
    }

    ch->target = target;

    /* ch will be placed in the target's attacker list.
     * It should not be there already.  Check it's there - it would be an error.
     */
    for (c = target->attackers; c; c = c->next_attacker)
        if (c == ch) {
            log(LogSeverity::Error, LVL_GOD, "set_battling: {} is already in attacker list for {}", GET_NAME(ch),
                GET_NAME(target));
            if (set_battling_corecount_2 < 3) {
                drop_core(nullptr, "set_battling_II");
                log(LogSeverity::Error, LVL_GOD, "set_battling: core dropped (II)");
                set_battling_corecount_2++;
            }
            return;
        }

    ch->next_attacker = target->attackers;
    target->attackers = ch;

    /* ch will be placed in the mudwide combat list.
     * It should not be there already.  Check it's there - it would be an error.
     */
    for (c = combat_list; c; c = c->next_fighting)
        if (c == ch) {
            log(LogSeverity::Error, LVL_GOD, "set_battling: {} is already in the combat list", GET_NAME(ch));
            if (set_battling_corecount_3 < 3) {
                drop_core(nullptr, "set_battling_III");
                log(LogSeverity::Error, LVL_GOD, "set_battling: core dropped (III)");
                set_battling_corecount_3++;
            }
            return;
        }

    ch->next_fighting = combat_list;
    combat_list = ch;
    GET_STANCE(ch) = STANCE_FIGHTING;
    if (!pk_allowed)
        check_killer(ch, target);
}

/* Stop ch from battling its target */
void stop_battling(CharData *ch) {
    static int stop_battling_corecount = 0;

    CharData *temp;

    if (ch->target) {
        REMOVE_FROM_LIST(ch, ch->target->attackers, next_attacker);
        ch->target = nullptr;
    } else {
        log(LogSeverity::Error, LVL_GOD, "SYSERR: stop_battling: {} has no target", GET_NAME(ch));
        if (stop_battling_corecount < 3) {
            drop_core(nullptr, "stop_battling_no_target");
            log(LogSeverity::Error, LVL_GOD, "stop_battling: core dropped");
            stop_battling_corecount++;
        }
    }

    /* Remove ch from the mudwide combat list. */

    /* If we're in the middle of another loop, and ch is the one that it's
     * processing, we need to advance this pointer. */
    if (ch == next_combat_list)
        next_combat_list = ch->next_fighting;
    REMOVE_FROM_LIST(ch, combat_list, next_fighting);
    ch->next_fighting = nullptr;

    /* This function is called from alter_pos(), so in the interest of avoiding
     * infinite loops -- and because this is a pretty safe stance change --
     * we'll directly modify the stance. */
    if (GET_STANCE(ch) == STANCE_FIGHTING)
        GET_STANCE(ch) = STANCE_ALERT;
}

/* Stop everyone from fighting this character */
void stop_attackers(CharData *ch) {
    CharData *a, *next;

    if (ch->attackers) {
        for (a = ch->attackers; a;) {
            next = a->next_attacker;
            stop_battling(a);
            a = next;
        }
    }
}

/* Stop non-VICIOUS folks from fighting this character */
void stop_merciful_attackers(CharData *ch) {
    CharData *a, *next;

    if (ch->attackers) {
        for (a = ch->attackers; a;) {
            next = a->next_attacker;
            if (!IS_VICIOUS(a))
                stop_battling(a);
            a = next;
        }
    }
}

/* This switches ch to attacking someone new.
 * It does not involve an immediate hit - it just switches who ch is battling.
 * For that reason, it isn't called when the "switch" skill is invoked --
 * that does involve an extra hit. */
void switch_target(CharData *ch, CharData *newvict) {
    if (ch->target)
        stop_battling(ch);
    set_battling(ch, newvict);
}

/* start one char fighting another (yes, it is horrible, I know... )   */
void set_fighting(CharData *ch, CharData *vict, bool reciprocate) {
    if (ch != vict) {
        /* Not sure how the attacker could be stunned or sleeping, but whatever...
         */
        if (GET_STANCE(ch) > STANCE_STUNNED && !FIGHTING(ch)) {
            if (EFF_FLAGGED(ch, EFF_SLEEP))
                effect_from_char(ch, SPELL_SLEEP);
            set_battling(ch, vict);
        }

        if (reciprocate && AWAKE(vict) && attack_ok(vict, ch, false) && !FIGHTING(vict)) {
            set_battling(vict, ch);
        }
    }
}

/* Move battle targets and such from one character to another.
 * Useful for shapechange. */
void transfer_battle(CharData *ch, CharData *tch) {
    CharData *c;
    CharData *temp;

    /* Replacement will fight whoever the original was fighting */
    tch->target = ch->target;
    ch->target = nullptr;

    /* Those fighting the original will fight the replacement */
    tch->attackers = ch->attackers;
    ch->attackers = nullptr;
    for (c = tch->attackers; c; c = c->next_attacker)
        c->target = tch;

    /* Replacement will take the place of the original in a list of attackers */
    if (FIGHTING(tch)) {
        for (c = FIGHTING(tch)->attackers; c; c = c->next_attacker)
            if (c->next_attacker == ch)
                c->next_attacker = tch;
        if (FIGHTING(tch)->attackers == ch)
            FIGHTING(tch)->attackers = tch;
    }
    tch->next_attacker = ch->next_attacker;
    ch->next_attacker = nullptr;

    /* Swap them in the main combat list */
    if (FIGHTING(tch)) {
        REMOVE_FROM_LIST(ch, combat_list, next_fighting);
        tch->next_fighting = combat_list;
        combat_list = tch;
    }
}

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

EVENTFUNC(quick_aggro_event) {
    GenericEventData *data = (GenericEventData *)event_obj;
    CharData *ch = data->ch;
    CharData *vict = data->vict;

    if (ch && !FIGHTING(ch) && GET_STANCE(ch) >= STANCE_ALERT && vict && room_contains_char(IN_ROOM(ch), vict) &&
        attack_ok(ch, vict, false))
        attack(ch, vict);

    return EVENT_FINISHED;
}

/*
 * This function determines whether ch attacking victim should be
 * allowed.   Returns true if this is an unallowed action, and false
 * if the action should go forward.
 */
bool attack_ok(CharData *ch, CharData *victim, bool verbose) {
    bool pet = false;

    if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MESMERIZED))
        return false;

    if (ch != victim && (ROOM_FLAGGED(victim->in_room, ROOM_PEACEFUL) || ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))) {
        if (verbose)
            char_printf(ch, "You feel ashamed trying to disturb the peace of this room.\n");
        return false;
    }

    if (MOB_FLAGGED(victim, MOB_PEACEFUL)) {
        if (verbose)
            act("But $N just has such a calm, peaceful feeling about $Mself!", false, ch, 0, victim, TO_CHAR);
        return false;
    }

    if (DECEASED(victim)) {
        if (verbose)
            act("$N is already dead.", true, ch, 0, victim, TO_CHAR);
        return false;
    }

    /* From here on, we consider PK, so if PK is on, the attack is ok */
    if (pk_allowed)
        return true;

    /* Also, PK is ok if we're in an arena room */
    if (ROOM_FLAGGED(victim->in_room, ROOM_ARENA) && ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
        return true;

    /* Is one of the parties a pet?   Use the master for calculations. */
    if (ch->master && EFF_FLAGGED(ch, EFF_CHARM))
        ch = ch->master;
    if (victim->master && EFF_FLAGGED(victim, EFF_CHARM)) {
        victim = victim->master;
        pet = true;
    }

    /* Feel free to hit yourself as much as you please.
     * Note that this test, coming after pet detection, will also allow
     * you to attack your own pet. */
    if (ch == victim)
        return true;

    /* Unallowed PK if both attacker and victim are players */
    if (IS_PC(ch) && IS_PC(victim)) {
        /* But allow it if one of them is a switched god. */
        if (IS_NPC(ch) && GET_LEVEL(REAL_CHAR(ch)) >= LVL_GOD)
            return true;
        if (IS_NPC(victim) && GET_LEVEL(REAL_CHAR(victim)) >= LVL_GOD)
            return true;
        if (verbose) {
            if (pet)
                char_printf(ch, "Sorry, you can't attack someone else's pet!\n");
            else
                char_printf(ch, "Sorry, player killing isn't allowed.\n");
        }
        return false;
    }

    return true;
}

/* mass_attack_ok - like attack_ok, but for widespread violence like area spells
 * and "hitall all".  Avoids your pets - where attack_ok wouldn't. */

bool mass_attack_ok(CharData *ch, CharData *victim, bool verbose) {

    if (!attack_ok(ch, victim, verbose))
        return false;

    if (IS_PC(ch) && PLAYERALLY(victim) && !ROOM_FLAGGED(victim->in_room, ROOM_ARENA)) {
        if (verbose)
            char_printf(ch, "You can't attack someone's pet!\n");
        return false;
    }

    return true;
}

int blessed_blow(CharData *ch, ObjData *weapon) {
    if (EFF_FLAGGED(ch, EFF_BLESS)) {
        return true;
    }
    return false;
}

/* Whether tch will be selected in some sort of area attack by ch.
 * The types of area attacks include:
 * -- mass attack skills like hitall and stomp
 * -- area spells like chain lighting and earthquake
 */
bool area_attack_target(CharData *ch, CharData *tch) {
    extern int roomeffect_allowed;

    /* Not yourself */
    if (ch == tch)
        return false;

    /* Not your group members */
    if (is_grouped(ch, tch))
        return false;

    /* Not your followers, or leader */
    if (tch->master == ch || ch->master == tch)
        return false;

    /* Players can't attack players unless... this is set or something */
    if (!roomeffect_allowed && !IS_NPC(ch) && !IS_NPC(tch))
        return false;

    /* Check whether you're ALLOWED to attack this person */
    if (!mass_attack_ok(ch, tch, false))
        return false;

    /* The following checks will be overridden if they're already in battle */
    if (!battling_my_group(ch, tch)) {
        /* Mobs don't include mobs, except player allies */
        if (!PLAYERALLY(ch) && !PLAYERALLY(tch))
            return false;
        /* Yeah, no hassle! */
        if (PRF_FLAGGED(tch, PRF_NOHASSLE))
            return false;
    }
    return true;
}

char *fread_message(FILE *fl, int nr) {
    char *action = fread_action(fl, nr);
    if (action)
        return action;
    sprintf(buf, "ERROR!   Message #%d missing.", nr);
    return strdup(buf);
}

void load_messages(void) {
    FILE *fl;
    int i, type;
    message_type *messages;
    char chk[256];

    if (!(fl = fopen(MESS_FILE, "r"))) {
        sprintf(buf2, "Error reading combat message file %s", MESS_FILE);
        perror(buf2);
        exit(1);
    }
    for (i = 0; i < MAX_MESSAGES; i++) {
        fight_messages[i].a_type = 0;
        fight_messages[i].number_of_attacks = 0;
        fight_messages[i].msg = 0;
    }

    fgets(chk, 256, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
        fgets(chk, 256, fl);

    while (*chk == 'M') {
        fgets(chk, 256, fl);
        sscanf(chk, " %d\n", &type);
        for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) && (fight_messages[i].a_type); i++)
            ;
        if (i >= MAX_MESSAGES) {
            fprintf(stderr, "Too many combat messages.   Increase MAX_MESSAGES and recompile.");
            exit(1);
        }
        CREATE(messages, message_type, 1);

        fight_messages[i].number_of_attacks++;
        fight_messages[i].a_type = type;
        messages->next = fight_messages[i].msg;
        fight_messages[i].msg = messages;

        messages->die_msg.attacker_msg = fread_message(fl, i);
        messages->die_msg.victim_msg = fread_message(fl, i);
        messages->die_msg.room_msg = fread_message(fl, i);
        messages->miss_msg.attacker_msg = fread_message(fl, i);
        messages->miss_msg.victim_msg = fread_message(fl, i);
        messages->miss_msg.room_msg = fread_message(fl, i);
        messages->hit_msg.attacker_msg = fread_message(fl, i);
        messages->hit_msg.victim_msg = fread_message(fl, i);
        messages->hit_msg.room_msg = fread_message(fl, i);
        messages->god_msg.attacker_msg = fread_message(fl, i);
        messages->god_msg.victim_msg = fread_message(fl, i);
        messages->god_msg.room_msg = fread_message(fl, i);
        messages->heal_msg.attacker_msg = fread_message(fl, i);
        messages->heal_msg.victim_msg = fread_message(fl, i);
        messages->heal_msg.room_msg = fread_message(fl, i);
        fgets(chk, 256, fl);
        while (!feof(fl) && (*chk == '\n' || *chk == '*'))
            fgets(chk, 256, fl);
    }

    fclose(fl);
}

void free_messages_type(msg_type *msg) {
    if (msg->attacker_msg)
        free(msg->attacker_msg);
    if (msg->victim_msg)
        free(msg->victim_msg);
    if (msg->room_msg)
        free(msg->room_msg);
}

void free_messages() {
    int i;

    for (i = 0; i < MAX_MESSAGES; i++)
        while (fight_messages[i].msg) {
            message_type *former = fight_messages[i].msg;

            free_messages_type(&former->die_msg);
            free_messages_type(&former->miss_msg);
            free_messages_type(&former->hit_msg);
            free_messages_type(&former->god_msg);
            free_messages_type(&former->heal_msg);

            fight_messages[i].msg = fight_messages[i].msg->next;
            free(former);
        }
}

void check_killer(CharData *ch, CharData *vict) {
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA))
        return;

    if (!PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(ch, PLR_KILLER) &&
        !IS_NPC(ch) && !IS_NPC(vict) && (ch != vict)) {
        SET_FLAG(PLR_FLAGS(ch), PLR_KILLER);
        log(LogSeverity::Warn, LVL_IMMORT, "PC Killer bit set on {} for initiating attack on {} at {}.", GET_NAME(ch),
            GET_NAME(vict), world[vict->in_room].name);
        char_printf(ch, "If you want to be a PLAYER KILLER, so be it...\n");
    }
}

void fluid_death(CharData *ch, CharData *killer) {
    ObjData *obj;
    ObjData *money;
    int i, numcoins, numitems;
    ObjData *next_obj;

    /* FIXME: Dumping all your crap on the ground should be centralized.
     *        This also occurs in do_quit. */

    numitems = 0;

    /* transfer objects to room */
    while (ch->carrying) {
        obj = ch->carrying;
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
        start_decomposing(obj);
        numitems++;
    }

    /* transfer equipment to room */
    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i)) {
            obj = unequip_char(ch, i);
            obj_to_room(obj, ch->in_room);
            start_decomposing(obj);
            numitems++;
        }

    /* And money */
    numcoins = GET_GOLD(ch) + GET_PLATINUM(ch) + GET_COPPER(ch) + GET_SILVER(ch);
    if (numcoins > 0) {
        money = create_money(GET_COINS(ch));
        obj_to_room(money, ch->in_room);
        start_decomposing(money);
        GET_GOLD(ch) = 0;
        GET_PLATINUM(ch) = 0;
        GET_COPPER(ch) = 0;
        GET_SILVER(ch) = 0;
    }

    /* Describe what happens to the creature's body, and state whether anything
     * is dropped. */
    if (numitems + numcoins > 1) {
        if (PHASE(ch) == PHASE_LIQUID)
            act("$n's remains fall with a splatter, dropping a few items.", true, ch, 0, 0, TO_ROOM);
        else
            act("The remnants of $n dissipate, dropping a few things.", true, ch, 0, 0, TO_ROOM);
    } else if (numitems + numcoins == 1) {
        if (PHASE(ch) == PHASE_LIQUID)
            act("$n falls with a splatter, dropping something.", true, ch, 0, 0, TO_ROOM);
        else
            act("The remnants of $n dissipate, dropping something.", true, ch, 0, 0, TO_ROOM);
    } else {
        if (PHASE(ch) == PHASE_LIQUID)
            act("$n's body loses cohesion and falls with a splatter.", true, ch, 0, 0, TO_ROOM);
        else
            act("The remnants of $n dissipate.", true, ch, 0, 0, TO_ROOM);
    }

    if (numitems + numcoins > 0 && killer && IN_ROOM(killer) == IN_ROOM(ch) && AWAKE(killer)) {
        if (PRF_FLAGGED(killer, PRF_AUTOLOOT)) {
            GetContext *context = begin_get_transaction(killer);
            if (numcoins)
                perform_get_from_room(context, CH_ROOM(ch)->contents);
            for (obj = CH_ROOM(ch)->contents; obj; obj = next_obj) {
                next_obj = obj->next_content;
                if (numitems-- > 0)
                    perform_get_from_room(context, obj);
                else
                    break;
            }
            end_get_transaction(context, nullptr);
        } else if (PRF_FLAGGED(killer, PRF_AUTOTREAS)) {
            GetContext *context = begin_get_transaction(killer);
            for (obj = CH_ROOM(ch)->contents; obj; obj = next_obj) {
                next_obj = obj->next_content;
                if (GET_OBJ_TYPE(obj) == ITEM_MONEY || GET_OBJ_TYPE(obj) == ITEM_TREASURE)
                    perform_get_from_room(context, obj);
            }
            end_get_transaction(context, nullptr);
        }
    }

    return;
}

ObjData *make_corpse(CharData *ch, CharData *killer) {
    ObjData *corpse, *o;
    ObjData *money;
    int i;
    extern int max_npc_corpse_time, max_pc_corpse_time, short_pc_corpse_time;

    /* Some folks don't make corpses */
    if (MOB_FLAGGED(ch, MOB_ILLUSORY)) {
        act("$n seems to have vanished entirely.", true, ch, 0, 0, TO_ROOM);
        return nullptr;
    } else if (PHASE(ch) != PHASE_SOLID) {
        fluid_death(ch, killer);
        return nullptr;
    }

    corpse = create_obj();

    corpse->item_number = NOTHING;
    corpse->in_room = NOWHERE;
    sprintf(buf2, "corpse %s", strip_ansi((ch)->player.namelist).c_str());
    corpse->name = strdup(buf2);

    sprintf(buf2, "&0The corpse of %s&0 is lying here.", GET_NAME(ch));
    corpse->description = strdup(buf2);

    sprintf(buf2, "the corpse of %s", GET_NAME(ch));
    corpse->short_description = strdup(buf2);

    GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
    SET_FLAG(GET_OBJ_FLAGS(corpse), ITEM_FLOAT);
    /* You can't store stuff in a corpse */
    GET_OBJ_VAL(corpse, VAL_CONTAINER_CAPACITY) = 0;

    if (!IS_NPC(ch))
        GET_OBJ_VAL(corpse, VAL_CONTAINER_CORPSE) = CORPSE_PC;
    else if (GET_LIFEFORCE(ch) == LIFE_UNDEAD)
        GET_OBJ_VAL(corpse, VAL_CONTAINER_CORPSE) = CORPSE_NPC_NORAISE;
    else
        GET_OBJ_VAL(corpse, VAL_CONTAINER_CORPSE) = CORPSE_NPC;

    GET_OBJ_VAL(corpse, VAL_CONTAINER_BITS) = 0; /* not closable */
    GET_OBJ_EFFECTIVE_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch) + 100;
    if (IS_NPC(ch))
        GET_OBJ_MOB_FROM(corpse) = GET_MOB_RNUM(ch);
    else
        GET_OBJ_MOB_FROM(corpse) = NOBODY;

    GET_OBJ_LEVEL(corpse) = GET_LEVEL(ch);

    /* transfer character's inventory to the corpse */
    corpse->contains = ch->carrying;
    for (o = corpse->contains; o != nullptr; o = o->next_content) {
        o->in_obj = corpse;
        o->carried_by = nullptr;
    }

    /* transfer character's equipment to the corpse */
    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i))
            obj_to_obj(unequip_char(ch, i), corpse);

    /* transfer coins */
    if (GET_CASH(ch) > 0) {
        /* following 'if' clause added to fix gold duplication loophole */
        if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
            money = create_money(GET_COINS(ch));
            obj_to_obj(money, corpse);
        }
        GET_GOLD(ch) = 0;
        GET_PLATINUM(ch) = 0;
        GET_COPPER(ch) = 0;
        GET_SILVER(ch) = 0;
    }
    ch->carrying = nullptr;
    IS_CARRYING_N(ch) = 0;
    IS_CARRYING_W(ch) = 0;

    /* DECOMPOSITION */
    if (IS_NPC(ch))
        GET_OBJ_DECOMP(corpse) = max_npc_corpse_time;
    else if (
        /* Corpse is empty */
        !corpse->contains ||
        /* Corpse contains just one object, and it's cash */
        (corpse->contains && !corpse->contains->next_content && GET_OBJ_TYPE(corpse->contains) == ITEM_MONEY))
        /* Therefore, a fairly short decomp time for a player corpse */
        GET_OBJ_DECOMP(corpse) = short_pc_corpse_time;
    else
        /* Corpse contains other objects - long decomp time */
        GET_OBJ_DECOMP(corpse) = max_pc_corpse_time;

    /* Ensure that the items inside aren't marked for decomposition. */
    stop_decomposing(corpse);
    /* And mark the corpse itself as decomposing. */
    SET_FLAG(GET_OBJ_FLAGS(corpse), ITEM_DECOMP);

    obj_to_room(corpse, ch->in_room);
    if (!IS_NPC(ch)) {
        register_corpse(corpse);
        delete_player_obj_file(ch);
    }

    return corpse;
}

void change_alignment(CharData *ch, CharData *victim) {
    int k_al, v_al, change;

    k_al = GET_ALIGNMENT(ch);
    v_al = GET_ALIGNMENT(victim);

    /* If victim was !good, and killer was good, make victim seem eviler */
    if (MOB_FLAGGED(victim, MOB_AGGR_GOOD) && IS_GOOD(ch))
        v_al -= 100;
    else if (MOB_FLAGGED(victim, MOB_AGGR_EVIL) && IS_EVIL(ch))
        v_al += 100;

    /* if we are a 'good' class then our effective align is higher, ie
     * even if a paladin of align 800 kills someone its as if they were align
     * 900 compared to a wariror 800 (cos they should know better!)
     */
    switch (GET_CLASS(ch)) {
        /* using drop through cases to accumulate the align */
    case CLASS_PALADIN:
    case CLASS_PRIEST:
        k_al += 50;
    case CLASS_RANGER:
    case CLASS_DRUID:
        k_al += 50;
        break;

        /* same deal for 'bad' classes */
    case CLASS_ANTI_PALADIN:
    case CLASS_DIABOLIST:
    case CLASS_NECROMANCER:
        k_al -= 50;
    case CLASS_THIEF:
    case CLASS_ASSASSIN:
        k_al -= 50;
    }

    change = (v_al / (-75 - (25 * abs((k_al - 1000) / 200)))) - (2 * abs(k_al / 1000));

    /* modifier based on level? */
    if (change < 0 && GET_LEVEL(ch) > GET_LEVEL(victim) + 10)
        change *= (GET_LEVEL(ch) - GET_LEVEL(victim)) / 10;

    GET_ALIGNMENT(ch) += change;
    GET_ALIGNMENT(ch) = std::clamp(GET_ALIGNMENT(ch), MIN_ALIGNMENT, MAX_ALIGNMENT);
}

void death_cry(CharData *ch) {
    int door, was_in;

    act("Your blood freezes as you hear $n's death cry.", false, ch, 0, 0, TO_ROOM);
    was_in = ch->in_room;

    for (door = 0; door < NUM_OF_DIRS; door++) {
        if (CAN_GO(ch, door) &&
            (!ROOM_FLAGGED(was_in, ROOM_ARENA) ||
             !ROOM_FLAGGED(world[was_in].exits[door]->to_room, ROOM_OBSERVATORY)) &&
            (!ROOM_FLAGGED(was_in, ROOM_OBSERVATORY) ||
             !ROOM_FLAGGED(world[was_in].exits[door]->to_room, ROOM_ARENA))) {
            ch->in_room = world[was_in].exits[door]->to_room;
            act("Your blood freezes as you hear someone's death cry.", false, ch, 0, 0, TO_ROOM);
            ch->in_room = was_in;
        }
    }
}

void arena_death(CharData *ch) {
    int door, destination, wasdark;
    int r_mortal_start_room;

    /* Make sure they have positive hit points */

    /* Not a good idea to restore someone to full health here, because
     * they could use that as a way to get healed from wounds acquired
     * on other fields of battle. */
    if (GET_HIT(ch) < 1)
        hurt_char(ch, nullptr, -(abs(GET_HIT(ch)) + 5), true);

    /* Move it out of the room */
    /* Choose the destination */
    destination = NOWHERE;
    for (door = 0; door < NUM_OF_DIRS; door++) {
        if (CAN_GO(ch, door) && ROOM_FLAGGED(world[IN_ROOM(ch)].exits[door]->to_room, ROOM_OBSERVATORY)) {
            destination = world[IN_ROOM(ch)].exits[door]->to_room;
            break;
        }
    }

    if (destination == NOWHERE) {
        destination = real_room(GET_HOMEROOM(ch));
        if (destination < 0) {
            log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
            destination = r_mortal_start_room;
        }
    }
    alter_pos(ch, POS_SITTING, STANCE_RESTING);

    wasdark = IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch);
    char_printf(ch, "An invisible hand removes you from the battlefield.\n");
    act("$n's still body is lifted away by an invisible force.", true, ch, nullptr, nullptr, TO_ROOM);
    dismount_char(ch);
    char_from_room(ch);
    char_to_room(ch, destination);
    act("$n is deposited gently in the middle of the room.", true, ch, nullptr, nullptr, TO_ROOM);
    check_new_surroundings(ch, wasdark, true);
    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

void perform_die(CharData *ch, CharData *killer) {
    CharData *real_char;
    ObjData *corpse = nullptr, *obj, *next_obj;
    MemorizedList *cur;

    /*check for switched victim */
    real_char = REAL_CHAR(ch);
    if (POSSESSED(ch)) {
        char_from_room(real_char);
        char_to_room(real_char, ch->in_room);
    }

    while (ch->effects)
        effect_remove(ch, ch->effects);
    while (real_char->effects)
        effect_remove(real_char, real_char->effects);

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA)) {
        arena_death(ch);
        return;
    }
    /* Everything beyond this point applies to normal, non-arena deaths */

    /* Set all spells in the memory list to unmemorized, but don't remove. */
    for (cur = GET_SPELL_MEM(ch).list_head; cur; cur = cur->next)
        cur->can_cast = 0;
    GET_SPELL_MEM(ch).num_memmed = 0;
    GET_SPELL_MEM(ch).mem_status = false;

    clear_cooldowns(ch);

    /* Stop NPCs from hunting you */
    remove_from_all_memories(ch);

    if (IS_NPC(real_char) && GET_LEVEL(real_char) < 60 && !MOB_FLAGGED(real_char, MOB_ILLUSORY))
        perform_random_gem_drop(ch);

    if (GET_LEVEL(real_char) < LVL_IMMORT) {
        corpse = make_corpse(real_char, killer);
        if (!IS_NPC(real_char)) {
            /* If killed dont save in room he died in */
            GET_LOADROOM(real_char) = GET_HOMEROOM(real_char);
            if (!PRF_FLAGGED(real_char, PRF_QUEST))
                init_trophy(real_char);
        }
        if (ch != real_char)
            extract_char(real_char);
        extract_char(ch);
    } else if (ch != real_char) {
        do_return(real_char, "", 0, 1);
        corpse = make_corpse(ch, killer);
        extract_char(ch);
    }

    if (killer && ch != killer && corpse && corpse->contains) {
        if (PRF_FLAGGED(killer, PRF_AUTOLOOT))
            get_from_container(killer, corpse, "all", nullptr);
        else if (PRF_FLAGGED(killer, PRF_AUTOTREAS)) {
            GetContext *context = begin_get_transaction(killer);
            for (obj = corpse->contains; obj; obj = next_obj) {
                next_obj = obj->next_content;
                if (GET_OBJ_TYPE(obj) == ITEM_MONEY || GET_OBJ_TYPE(obj) == ITEM_TREASURE)
                    perform_get_from_container(context, obj, corpse);
            }
            end_get_transaction(context, corpse);
        }
    }
}

void kill_to_group_trophy(CharData *ch, CharData *victim) {
    int members = 0;
    CharData *group_master;
    GroupType *g;

    group_master = (ch->group_master ? ch->group_master : ch);

    if (group_master->in_room == ch->in_room)
        members = 1;
    for (g = group_master->groupees; g; g = g->next)
        if (g->groupee->in_room == ch->in_room)
            ++members;

    if (group_master->in_room == ch->in_room)
        if (!IS_NPC(group_master))
            kill_to_trophy(victim, group_master, 1.0 / members);
    for (g = group_master->groupees; g; g = g->next)
        if (g->groupee->in_room == ch->in_room)
            if (!IS_NPC(g->groupee))
                kill_to_trophy(victim, g->groupee, 1.0 / members);
}

void die(CharData *ch, CharData *killer) {
    int i;

    GET_HIT(ch) = -15;
    alter_pos(ch, POS_PRONE, STANCE_DEAD);

    /* Record player death in log */
    if (!IS_NPC(ch)) {
        if (killer)
            sprintf(buf2, "%s was killed by %s", GET_NAME(ch), GET_NAME(killer));
        else
            sprintf(buf2, "%s died", GET_NAME(ch));
        clan_notification(GET_CLAN(ch), ch, "%s.", buf2);
        log(LogSeverity::Stat, LVL_IMMORT, "{} in {} [{:d}]", buf2, world[ch->in_room].name, world[ch->in_room].vnum);
    }

    /* Stop the fighting */
    if (FIGHTING(ch))
        stop_fighting(ch);
    stop_attackers(ch);

    /* Victim loses exp */
    /* Only if a player.  Exp for killing a mob has not been disbursed yet. */
    if (!IS_NPC(ch))
        gain_exp(ch, -exp_death_loss(REAL_CHAR(ch), GET_LEVEL(REAL_CHAR(ch))), GAIN_REGULAR);

    /* Trophy for the killer */
    if (killer && !MOB_FLAGGED(ch, MOB_ILLUSORY)) {
        if (IS_GROUPED(killer))
            kill_to_group_trophy(killer, ch);
        else if (!IS_NPC(killer))
            kill_to_trophy(ch, killer, 1.0);
    }

    if (EFF_FLAGGED(ch, EFF_ON_FIRE))
        REMOVE_FLAG(EFF_FLAGS(ch), EFF_ON_FIRE);

    if (!IS_NPC(ch)) {
        REMOVE_FLAG(PLR_FLAGS(ch), PLR_KILLER);
        REMOVE_FLAG(PLR_FLAGS(ch), PLR_THIEF);
        GET_COND(ch, THIRST) = 24;
        GET_COND(ch, FULL) = 24;
        GET_COND(ch, DRUNK) = 0;
    }
    abort_casting(ch);

    /* Cancel events */
    if (ch->events)
        cancel_event_list(&(ch->events));
    for (i = 0; i < EVENT_FLAG_FIELDS; ++i)
        ch->event_flags[i] = 0;

    /* show death cry if there's a killer */
    if (!MOB_PERFORMS_SCRIPTS(ch)) {
        if (killer)
            death_cry(ch);
    }
    /* run the death trigger, but don't death cry if it returns 0
     * or if there's no killer */
    else if (death_mtrigger(ch, killer) && killer)
        death_cry(ch);

    event_create(EVENT_DIE, die_event, mkgenericevent(ch, killer, 0), true, &(ch->events), 0);
    /* perform_die() will be called by this event. */
}

static float exp_level_bonus(int level_diff) {
    /* Can't use std::max() because it returns an int. */
    if (level_diff > 0)
        return 1.0 + (0.015 * level_diff);
    else
        return 1.0;
}

static void receive_kill_credit(CharData *ch, CharData *vict, long exp) {
    /* Calculate level-difference bonus */
    exp *= exp_level_bonus(GET_LEVEL(vict) - GET_LEVEL(ch));

    /* Trophy decrease */
    exp *= exp_trophy_modifier(ch, vict);

    exp = std::max(exp, 1l);

    char_printf(ch, "You receive your share of experience.\n");

    /* Adjust exp for paladins */
    if (GET_CLASS(ch) == CLASS_PALADIN) {
        if (IS_EVIL(vict))
            exp *= 1.25;
        else if (GET_ALIGNMENT(vict) > -350 && GET_ALIGNMENT(ch) < -150)
            exp *= 0.5;
        else if (GET_ALIGNMENT(vict) >= -150 && GET_ALIGNMENT(ch) <= 150)
            exp = 0;
        else if (GET_ALIGNMENT(vict) > 150 && GET_ALIGNMENT(ch) < 350)
            exp *= -0.5;
        else if (IS_GOOD(vict))
            exp *= -1;
    }

    gain_exp(ch, exp, GAIN_REGULAR);
    change_alignment(ch, vict);
}

static void group_gain(CharData *killer, CharData *vict) {
    CharData *group_master;
    GroupType *g;
    long total_exp, exp_share;
    int groupees = 0;

    group_master = killer->group_master ? killer->group_master : killer;

    if (group_master->in_room == killer->in_room)
        ++groupees;
    for (g = group_master->groupees; g; g = g->next)
        if (g->groupee->in_room == killer->in_room)
            ++groupees;

    total_exp = GET_EXP(vict);
    total_exp *= 1.3;
    exp_share = total_exp / groupees;

    if (group_master->in_room == killer->in_room)
        receive_kill_credit(group_master, vict, exp_share);
    for (g = group_master->groupees; g; g = g->next)
        if (g->groupee->in_room == killer->in_room)
            receive_kill_credit(g->groupee, vict, exp_share);
}

void disburse_kill_exp(CharData *killer, CharData *vict) {
    /* No exp for killing a disconnected player */
    if (!IS_NPC(vict) && !vict->desc)
        return;

    if (MOB_FLAGGED(vict, MOB_ILLUSORY))
        return;

    if (ROOM_FLAGGED(killer->in_room, ROOM_ARENA))
        return;

    if (IS_GROUPED(killer))
        group_gain(killer, vict);

    /* Lone killer */
    else
        receive_kill_credit(killer, vict, GET_EXP(vict));

    /* cap for exp is in max_exp_gain() in gain_exp() */
}

char *replace_string(char *str, const char *weapon_singular, const char *weapon_plural) {
    static char buf[1024];
    char *cp;

    cp = buf;

    for (; *str; str++) {
        if (*str == '#') {
            switch (*(++str)) {
            case 'W':
                for (; *weapon_plural; *(cp++) = *(weapon_plural++))
                    ;
                break;
            case 'w':
                for (; *weapon_singular; *(cp++) = *(weapon_singular++))
                    ;
                break;
            default:
                *(cp++) = '#';
                break;
            }
        } else
            *(cp++) = *str;

        *cp = 0;
    } /* For */

    return (buf);
}

static void append_damage_amount(char *b, const char *msg, int dam, int type) {
    /*
     * In comm.h, TO_ROOM is 1, TO_VICT is 2, TO_NOTVICT is 3, TO_CHAR is 4
     */
    const char *colors[6] = {
        "", "&4", "&1", "&4", "&3", "&2", /* healing */
    };
    if (damage_amounts) {
        if (type != TO_VICT && type != TO_NOTVICT && type != TO_CHAR && type != TO_ROOM) {
            log("SYSERR: append_damage_amounts: unrecognized target type {:d}", type);
            type = TO_NOTVICT;
        }
        if (dam < 0) {
            dam *= -1;
            type = 5; /* healing */
        }
        sprintf(b, "%s (%s%d&0)", msg, colors[type], dam);
    } else
        strcpy(b, msg);
}

/* message for doing damage with a weapon */
void dam_message(int dam, CharData *ch, CharData *victim, int w_type) {
    const char *msg;
    char b2[1024];
    int msgnum;
    int percent = 0;

    static struct dam_barehand {
        char *to_room;
        char *to_char;
        char *to_victim;
    } bare_attack[] = {{"$n threw $s punch just a little wide, missing $N completely.&0", /* 0: 0 */
                        "You thought you saw $N somewhere where $E wasn't.&0",
                        "$n takes aim at you, but loses communication with $s fists!&0"},

                       {"$n slaps $N before tweaking $S nose.&0", /* 1: 1..2   */
                        "You slap $N, and while $E is distracted, make a grab for $S nose.&0",
                        "$n slaps your cheek, then tries to twist your nose off!&0"},

                       {"$n delivers a swift kick to $N's shin, causing a yelp of pain.&0", /* 2: 3..4 */
                        "You send a swift kick to $N's shin.&0", "You yelp in pain as $n kicks you in the shin!&0"},

                       {"$N howls in pain as $n nearly rips $S ear off!&0", /* 3: 5..6 */
                        "You grab $N's ears and almost pull them off!&0",
                        "You cannot help but scream in pain as $n gets a good grasp on your ears.&0"},

                       {"$N nearly doubles over choking as $n collapses $S trachea!&0", /* 4: 7..10 */
                        "You sink your stiffened fingers into $N's throat.&0",
                        "$n strikes you in the throat, making you cough involuntarily.&0"},

                       {"$n dances up to $N and throws $s elbow into $S gut!&0", /* 5: 11..14 */
                        "You spin around so your back is against $N and throw your elbow into $S gut!&0",
                        "$n dances up to you, and throws $s elbow into your stomach!   OUCH!&0"},

                       {"Cupping $s hands, $n slaps $N's ears, bursting $S eardrums.&0", /* 6: 15..19 */
                        "You cup your hands and slap $N's ears, hoping to cause deafness.&0",
                        "You feel as if $n inserted long daggers into your ears, piercing your brain!&0"},

                       {"$n punches $N repeatedly in the kidneys!&0", /* 7: 19..23 */
                        "You wallop $N in the kidneys!&0",
                        "$n repeatedly punches you in the kidneys, and you feel like hurling!&0"},

                       {"$n punches $N in the throat, causing $M to choke!&0", /* 8: > 23 */
                        "You punch $N in the throat causing $M to nearly choke to death!&0",
                        "$n nails you in the throat, and you barely avoid choking to death!&0"}};

    static struct dam_fire_barehand {
        char *to_room;
        char *to_char;
        char *to_victim;
    } bare_fire_attack[] = {
        {"$n threw $s burning punch just a little wide, missing $N completely.&0", /* 0: 0 */
         "You thought you saw $N somewhere where $E wasn't.&0",
         "$n takes aim at you, but loses communication with $s burning fists!&0"},

        {"$n slaps $N before tweaking $S nose with $s burning fists.&0", /* 1: 1..2   */
         "You slap $N with your burning fists, and make a grab for $S nose.&0",
         "$n slaps your cheek with $s burning fists, then tries to twist your nose off!&0"},

        {"$n delivers a swift burning kick to $N's shin, causing a yelp of pain.&0", /* 2: 3..4 */
         "You send a swift burning kick to $N's shin.&0", "You yelp in pain as $n kicks you in the shin!&0"},

        {"$N howls in pain as $n nearly burns $S ear off!&0", /* 3: 5..6 */
         "You geab $N's ears and almost burn them off!&0",
         "You cannot help but scream in pain as $n gets a burning grasp on your ears.&0"},

        {"$N nearly doubles over choking as $n collapses $S trachea with a fiery punch!&0", /* 4: 7..10 */
         "You sink your fiery stiffened fingers into $N's throat.&0",
         "$n strikes you in the throat with a fiery punch.&0"},

        {"$n dances up to $N and throws $s fiery elbow into $S gut!&0", /* 5: 11..14 */
         "You spin around $N and throw your fiery elbow into $S gut!&0",
         "$n dances up to you, and throws $s fiery elbow into your stomach!   OUCH!&0"},

        {"Cupping $s flaming hands, $n slaps $N's ears, bursting $S eardrums.&0", /* 6: 15..19 */
         "You cup your flaming hands and slap $N's ears, hoping to cause deafness.&0",
         "You feel as if $n inserted flaming daggers into your ears, burning your brain!&0"},

        {"$n punches $N repeatedly in the kidneys with flaming fists!&0", /* 7: 19..23 */
         "You wallop $N in the kidneys with flaming fists!&0",
         "$n repeatedly punches you in the kidneys with flaming fists!&0"},

        {"$n punches $N in the throat, causing $M to burn!&0", /* 8: > 23 */
         "You punch $N in the throat causing $M to nearly burn to death!&0",
         "$n nails you in the throat, and you barely avoid burning to death!&0"}};

    static struct dam_ice_barehand {
        char *to_room;
        char *to_char;
        char *to_victim;
    } bare_ice_attack[] = {
        {"$n threw $s burning punch just a little wide, missing $N completely.&0", /* 0: 0 */
         "You thought you saw $N somewhere where $E wasn't.&0",
         "$n takes aim at you, but loses communication with $s icy fists!&0"},

        {"$n slaps $N before tweaking $S nose with $s chilling fists.&0", /* 1: 1..2   */
         "You slap $N with your chilling fists, and make a grab for $S nose.&0",
         "$n slaps your cheek with $s chilling fists, then tries to twist your nose off!&0"},

        {"$n delivers a swift chilling kick to $N's shin, causing a yelp of pain.&0", /* 2: 3..4 */
         "You send a swift chilling kick to $N's shin.&0", "You yelp in pain as $n kicks you in the shin!&0"},

        {"$N howls in pain as $n nearly freezes $S ear off!&0", /* 3: 5..6 */
         "You geab $N's ears and almost freeze them off!&0",
         "You cannot help but scream in pain as $n gets a freezeing grasp on your ears.&0"},

        {"$N nearly doubles over choking as $n collapses $S trachea with an icy punch!&0", /* 4: 7..10 */
         "You sink your icy stiffened fingers into $N's throat.&0",
         "$n strikes you in the throat with an icy punch.&0"},

        {"$n dances up to $N and throws $s icy elbow into $S gut!&0", /* 5: 11..14 */
         "You spin around $N and throw your icy elbow into $S gut!&0",
         "$n dances up to you, and throws $s icy elbow into your stomach!   OUCH!&0"},

        {"Cupping $s freezing hands, $n slaps $N's ears, bursting $S eardrums.&0", /* 6: 15..19 */
         "You cup your freezing hands and slap $N's ears, hoping to cause deafness.&0",
         "You feel as if $n inserted icy daggers into your ears, freezing your brain!&0"},

        {"$n punches $N repeatedly in the kidneys with icy fists!&0", /* 7: 19..23 */
         "You wallop $N in the kidneys with icy fists!&0",
         "$n repeatedly punches you in the kidneys with icy fists!&0"},

        {"$n punches $N in the throat, causing $M to freeze!&0", /* 8: > 23 */
         "You punch $N in the throat causing $M to nearly freeze to death!&0",
         "$n nails you in the throat, and you barely avoid freezing to death!&0"}};

    static struct dam_lightning_barehand {
        char *to_room;
        char *to_char;
        char *to_victim;
    } bare_lightning_attack[] = {
        {"$n threw $s burning punch just a little wide, missing $N completely.&0", /* 0: 0 */
         "You thought you saw $N somewhere where $E wasn't.&0",
         "$n takes aim at you, but loses communication with $s electric fists!&0"},

        {"$n slaps $N before tweaking $S nose with $s electric fists.&0", /* 1: 1..2   */
         "You slap $N with your electric fists, and make a grab for $S nose.&0",
         "$n slaps your cheek with $s electric fists, then tries to twist your nose off!&0"},

        {"$n delivers a swift electrifying kick to $N's shin, causing a yelp of pain.&0", /* 2: 3..4 */
         "You send a swift electrifying kick to $N's shin.&0", "You yelp in pain as $n kicks you in the shin!&0"},

        {"$N howls in pain as $n nearly shock $S ear off!&0", /* 3: 5..6 */
         "You geab $N's ears and almost shock them off!&0",
         "You cannot help but scream in pain as $n gets a shocking grasp on your ears.&0"},

        {"$N nearly doubles over choking as $n collapses $S trachea with an electrifying punch!&0", /* 4: 7..10 */
         "You sink your electrified stiffened fingers into $N's throat.&0",
         "$n strikes you in the throat with an electrifying punch.&0"},

        {"$n dances up to $N and throws $s electrified elbow into $S gut!&0", /* 5: 11..14 */
         "You spin around $N and throw your electrified elbow into $S gut!&0",
         "$n dances up to you, and throws $s electrified elbow into your stomach!   OUCH!&0"},

        {"Cupping $s shocking hands, $n slaps $N's ears, bursting $S eardrums.&0", /* 6: 15..19 */
         "You cup your shocking hands and slap $N's ears, hoping to cause deafness.&0",
         "You feel as if $n inserted electrified daggers into your ears, shocking your brain!&0"},

        {"$n punches $N repeatedly in the kidneys with jolting fists!&0", /* 7: 19..23 */
         "You wallop $N in the kidneys with jolting fists!&0",
         "$n repeatedly punches you in the kidneys with jolting fists!&0"},

        {"$n punches $N in the throat, causing $M to be shocked!&0", /* 8: > 23 */
         "You punch $N in the throat causing $M to be nearly shocking to death!&0",
         "$n nails you in the throat, and you barely avoid being shocked to death!&0"}};

    static struct dam_acid_barehand {
        char *to_room;
        char *to_char;
        char *to_victim;
    } bare_acid_attack[] = {
        {"$n threw $s burning punch just a little wide, missing $N completely.&0", /* 0: 0 */
         "You thought you saw $N somewhere where $E wasn't.&0",
         "$n takes aim at you, but loses communication with $s corrosive fists!&0"},

        {"$n slaps $N before tweaking $S nose with $s acidic fists.&0", /* 1: 1..2   */
         "You slap $N with your acidic fists, and make a grab for $S nose.&0",
         "$n slaps your cheek with $s acidic fists, then tries to twist your nose off!&0"},

        {"$n delivers a swift corrosive kick to $N's shin, causing a yelp of pain.&0", /* 2: 3..4 */
         "You send a swift corrosive kick to $N's shin.&0", "You yelp in pain as $n kicks you in the shin!&0"},

        {"$N howls in pain as $n nearly melt $S ear off!&0", /* 3: 5..6 */
         "You geab $N's ears and almost melt them off!&0",
         "You cannot help but scream in pain as $n gets an acidic grasp on your ears.&0"},

        {"$N nearly doubles over choking as $n collapses $S trachea with a corrosive punch!&0", /* 4: 7..10 */
         "You sink your corrosive stiffened fingers into $N's throat.&0",
         "$n strikes you in the throat with a corrosive punch.&0"},

        {"$n dances up to $N and throws $s acidic elbow into $S gut!&0", /* 5: 11..14 */
         "You spin around $N and throw your acidic elbow into $S gut!&0",
         "$n dances up to you, and throws $s acidic elbow into your stomach!   OUCH!&0"},

        {"Cupping $s corrosive hands, $n slaps $N's ears, melting $S eardrums.&0", /* 6: 15..19 */
         "You cup your corrosive hands and slap $N's ears, hoping to cause deafness.&0",
         "You feel as if $n inserted acid daggers into your ears, melting your brain!&0"},

        {"$n punches $N repeatedly in the kidneys with dissolving fists!&0", /* 7: 19..23 */
         "You wallop $N in the kidneys with dissolving fists!&0",
         "$n repeatedly punches you in the kidneys with dissolving fists!&0"},

        {"$n punches $N in the throat, causing $M to melt!&0", /* 8: > 23 */
         "You punch $N in the throat causing $M to nearly melt to death!&0",
         "$n nails you in the throat, and you barely avoid melting to death!&0"}};

    static struct dam_weapon_type {
        char *to_room;
        char *to_char;
        char *to_victim;
    } dam_weapons[] = {

        /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

        {"$n tries to #w $N, but misses.&0", /* 0: 0       */
         "You try to #w $N, but miss.&0", "$n tries to #w you, but misses.&0"},

        {"$n grazes $N as $e #W $M.&0", /* 1: 1..2   */
         "You graze $N as you #w $M.&0", "$n grazes you as $e #W you.&0"},

        {"$n barely #W $N.&0", /* 2: 3..4   */
         "You barely #w $N.&0", "$n barely #W you.&0"},

        {"$n #W $N.&0", /* 3: 5..6   */
         "You #w $N.&0", "$n #W you.&0"},

        {"$n #W $N hard.&0", /* 4: 7..10   */
         "You #w $N hard.&0", "$n #W you hard.&0"},

        {"$n #W $N very hard.&0", /* 5: 11..14   */
         "You #w $N very hard.&0", "$n #W you very hard.&0"},

        {"$n #W $N extremely hard.&0", /* 6: 15..19   */
         "You #w $N extremely hard.&0", "$n #W you extremely hard.&0"},

        {"$n massacres $N to small fragments with $s #w.&0", /* 7: 19..23 */
         "You massacre $N to small fragments with your #w.&0", "$n massacres you to small fragments with $s #w.&0"},

        {"$n nearly rips $N in two with $s deadly #w!!&0", /* 8: > 23    */
         "You nearly rip $N in two with your deadly #w!!&0", "$n nearly rips you in two with $s deadly #w!!&0"}};

    memset(b2, 0x0, 1024);

    if (w_type != SKILL_BAREHAND)
        w_type -= TYPE_HIT; /* Change to base of table with text */

    if (GET_HIT(victim) > 0)
        percent = (int)(dam * 100 / GET_HIT(victim));
    else
        percent = 100;

    if ((percent <= 1) && (dam >= 20))
        percent += 1;
    if ((percent <= 1) && (dam >= 50))
        percent += 1;

    /* this fixes the false message due to the rounding error --gurlaek 11/22/1999
     */
    percent = std::max(percent, 1);

    if (dam == 0)
        percent = 0;

    if (percent == 0)
        msgnum = 0;
    else if (percent <= 2)
        msgnum = 1;
    else if (percent <= 4)
        msgnum = 2;
    else if (percent <= 6)
        msgnum = 3;
    else if (percent <= 10)
        msgnum = 4;
    else if (percent <= 15)
        msgnum = 5;
    else if (percent <= 19)
        msgnum = 6;
    else if (percent <= 35)
        msgnum = 7;
    else if (percent <= 55)
        msgnum = 8;
    else
        msgnum = 8;

    /* damage message to onlookers */
    if (w_type == SKILL_BAREHAND)
        msg = bare_attack[msgnum].to_room;
    else if (EFF_FLAGGED(ch, EFF_FIREHANDS))
        msg = bare_fire_attack[msgnum].to_room;
    else if (EFF_FLAGGED(ch, EFF_ICEHANDS))
        msg = bare_ice_attack[msgnum].to_room;
    else if (EFF_FLAGGED(ch, EFF_LIGHTNINGHANDS))
        msg = bare_lightning_attack[msgnum].to_room;
    else if (EFF_FLAGGED(ch, EFF_ACIDHANDS))
        msg = bare_acid_attack[msgnum].to_room;
    else
        msg = replace_string(dam_weapons[msgnum].to_room, attack_hit_text[w_type].singular,
                             attack_hit_text[w_type].plural);
    append_damage_amount(b2, msg, dam, TO_NOTVICT);
    act(b2, false, ch, nullptr, victim, TO_NOTVICT);

    /* damage message to damager */
    if (w_type == SKILL_BAREHAND)
        msg = bare_attack[msgnum].to_char;
    else if (EFF_FLAGGED(ch, EFF_FIREHANDS))
        msg = bare_fire_attack[msgnum].to_char;
    else if (EFF_FLAGGED(ch, EFF_ICEHANDS))
        msg = bare_ice_attack[msgnum].to_char;
    else if (EFF_FLAGGED(ch, EFF_LIGHTNINGHANDS))
        msg = bare_lightning_attack[msgnum].to_char;
    else if (EFF_FLAGGED(ch, EFF_ACIDHANDS))
        msg = bare_acid_attack[msgnum].to_char;
    else
        msg = replace_string(dam_weapons[msgnum].to_char, attack_hit_text[w_type].singular,
                             attack_hit_text[w_type].plural);
    append_damage_amount(b2, msg, dam, TO_CHAR);
    act(b2, false, ch, nullptr, victim, TO_CHAR);

    /* damage message to damagee */
    if (w_type == SKILL_BAREHAND)
        msg = bare_attack[msgnum].to_victim;
    else if (EFF_FLAGGED(ch, EFF_FIREHANDS))
        msg = bare_fire_attack[msgnum].to_victim;
    else if (EFF_FLAGGED(ch, EFF_ICEHANDS))
        msg = bare_ice_attack[msgnum].to_victim;
    else if (EFF_FLAGGED(ch, EFF_LIGHTNINGHANDS))
        msg = bare_lightning_attack[msgnum].to_victim;
    else if (EFF_FLAGGED(ch, EFF_ACIDHANDS))
        msg = bare_acid_attack[msgnum].to_victim;
    else
        msg = replace_string(dam_weapons[msgnum].to_victim, attack_hit_text[w_type].singular,
                             attack_hit_text[w_type].plural);
    append_damage_amount(b2, msg, dam, TO_VICT);
    act(b2, false, ch, nullptr, victim, TO_VICT | TO_SLEEP);
}

/*
 * Send messages for doing damage with a spell or skill, or
 * for weapon damage on miss and death blows.
 *
 * Returns true if any messages were sent, false otherwise.
 */
bool skill_message(int dam, CharData *ch, CharData *vict, int attacktype, bool death) {
    int i, j, nr;
    char b2[1024];
    message_type *msg;
    msg_type *type;

    ObjData *weap = GET_EQ(ch, WEAR_WIELD);

    memset(b2, 0x0, 1024);

    /* Try the 2H weapon */
    if (!weap)
        weap = GET_EQ(ch, WEAR_2HWIELD);

    /* Secondary hand uses second weapon */
    if (attacktype == SKILL_2BACK)
        weap = GET_EQ(ch, WEAR_WIELD2);

    for (i = 0; i < MAX_MESSAGES; i++) {
        if (fight_messages[i].a_type == attacktype) {
            nr = roll_dice(1, fight_messages[i].number_of_attacks);

            for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
                msg = msg->next;

            if (dam < 0)
                type = &msg->heal_msg;
            else if (!IS_NPC(vict) && GET_LEVEL(vict) >= LVL_IMMORT)
                type = &msg->god_msg;
            else if (dam != 0) {
                if (death)
                    type = &msg->die_msg;
                else
                    type = &msg->hit_msg;
            } else if (ch != vict)
                type = &msg->miss_msg;
            else
                return true; /* don't show a message */

            /* to damager */
            if (ch != vict) {
                append_damage_amount(b2, type->attacker_msg, dam, TO_CHAR);
                act(b2, false, ch, weap, vict, TO_CHAR);
            }

            /* to target */
            append_damage_amount(b2, type->victim_msg, dam, TO_VICT);
            act(b2, false, ch, weap, vict, TO_VICT | TO_SLEEP);

            /* to room */
            append_damage_amount(b2, type->room_msg, dam, TO_ROOM);
            act(b2, false, ch, weap, vict, TO_NOTVICT | TO_VICTROOM);

            return true;
        }
    }
    return false;
}

int damage(CharData *ch, CharData *victim, int dam, int attacktype) {
    int attacker_weapons, dummy, shdam;
    CharData *was_fighting = FIGHTING(victim);
    bool death = false;

    if (DECEASED(victim)) {
        log("SYSERR: Attempt to damage a corpse in room num {:d}", world[victim->in_room].vnum);
        return VICTIM_DEAD;
    }

    /*
     * Eventually, I hope to implement V3's resistances here.
     * The resistance check needs to come before healing in case
     * the victim has inverse resistance to the damage.
     */

    /*
     * If damage is negative and healing should occur, take care of it
     * up here so as not to confuse the real damage code below.
     */
    if (dam < 0) {
        int orig_hp = GET_HIT(victim);

        hurt_char(victim, ch, dam, true);

        if (!skill_message(dam, ch, victim, attacktype, false)) {
            /* Default message */
        }

        /* Return the actual amount of healing that occured. */
        return orig_hp - GET_HIT(victim);
    }

    if (!attack_ok(ch, victim, true))
        return 0;

    if (victim != ch) {
        if (GET_STANCE(ch) > STANCE_STUNNED) {

            /* Start the attacker fighting the victim. */
            if (!IS_SPELL(attacktype))
                /* Caller can decide whether vict should fight back */
                set_fighting(ch, victim, false);

            /*
             * If this is a pet attacking, there is a chance the victim will
             * notice the master and switch targets.
             */
            if (IS_NPC(ch) && IS_NPC(victim) && victim->master && !random_number(0, 100) &&
                EFF_FLAGGED(victim, EFF_CHARM) && (victim->master->in_room == ch->in_room) &&
                CAN_SEE(ch, victim->master)) {
                if (FIGHTING(ch))
                    stop_fighting(ch);

                sprintf(buf, "&6&b$n observes your command over %s and attacks YOU instead!&0", GET_NAME(victim));
                act(buf, false, ch, 0, victim->master, TO_VICT);

                sprintf(buf, "&6&b$n observes $N commanding %s and attacks $M instead!&0", GET_NAME(victim));
                act(buf, false, ch, 0, victim->master, TO_NOTVICT);
                sprintf(buf, "&6&bYou observes $N commanding %s and attacks $M instead!&0", GET_NAME(victim));
                act(buf, false, ch, 0, victim->master, TO_CHAR);

                attack(ch, victim->master);
                return 0;
            }
        }

        /* Start the attacker fighting the victim. */
        if (GET_STANCE(victim) > STANCE_STUNNED && !FIGHTING(victim)) {
            /*
             * NPC's always start fighting, but PC's can ignore spells via
             * the PASSIVE flag.
             */
            if (IS_NPC(victim) || (!IS_SPELL(attacktype) || !PRF_FLAGGED(victim, PRF_PASSIVE)))
                /* Caller can decide whether vict should fight back */
                set_fighting(victim, ch, false);
        }

        /*
         * If the mob has memory, make it remember this player.
         */
        if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
            remember(victim, ch);
    }

    /* If ch is attacking his/her pet, cancel charming */
    if (victim->master == ch)
        stop_follower(victim, true);

    /* Appear, lose glory */
    aggro_lose_spells(ch);

    if (attacktype != SPELL_ON_FIRE && attacktype != SPELL_POISON) {

        /* Reduce damage for PVP for everything except spells */
        if (!IS_NPC(ch) && !IS_NPC(victim) && attacktype > MAX_SPELLS)
            dam /= 3;

        /* Charmies do half damage. */
        if (EFF_FLAGGED(ch, EFF_CHARM) && ch->master && IS_PC(ch->master))
            dam >>= 1;

        /* Sanctuary and stone skin take half damage. */
        if (EFF_FLAGGED(victim, EFF_SANCTUARY) || EFF_FLAGGED(victim, EFF_STONE_SKIN))
            dam >>= 1;

        /* Ranger players deal bonus damage for fighting with two weapons */
        if (GET_CLASS(ch) == CLASS_RANGER && !IS_NPC(ch) && (GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON))
            dam *= 1.2;

        /* Protection from evil/good takes 80% damage */
        if (EFF_FLAGGED(victim, EFF_PROTECT_EVIL) && GET_ALIGNMENT(ch) <= -500 && GET_ALIGNMENT(victim) >= 500)
            dam *= 0.8;

        else if (EFF_FLAGGED(victim, EFF_PROTECT_GOOD) && GET_ALIGNMENT(ch) >= 500 && GET_ALIGNMENT(victim) <= -500)
            dam *= 0.8;
    }

    /* You can't damage an immortal! */
    if (!IS_NPC(victim) && GET_LEVEL(victim) >= LVL_IMMORT)
        dam = 0;

    if (EFF_FLAGGED(victim, EFF_MINOR_PARALYSIS)) {
        act("$n's attack frees $N from magic which held $M motionless.", false, ch, 0, victim, TO_NOTVICT);
        act("$n's blow shatters the magic paralyzing you!", false, ch, 0, victim, TO_VICT);
        act("Your blow disrupts the magic keeping $N frozen.", false, ch, 0, victim, TO_CHAR);
        effect_from_char(victim, SPELL_MINOR_PARALYSIS);
        effect_from_char(victim, SPELL_ENTANGLE);
        REMOVE_FLAG(EFF_FLAGS(victim), EFF_MINOR_PARALYSIS);
    }

    if (EFF_FLAGGED(victim, EFF_MESMERIZED)) {
        act("$n's attack distracts $N from whatever was fascinating $M.", false, ch, 0, victim, TO_NOTVICT);
        act("$n attacks, jolting you out of your reverie!", false, ch, 0, victim, TO_VICT);
        act("You drew $N's attention from whatever $E was pondering.", false, ch, 0, victim, TO_CHAR);
        effect_from_char(victim, SPELL_MESMERIZE);
        REMOVE_FLAG(EFF_FLAGS(victim), EFF_MESMERIZED);
    }

    if (!pk_allowed)
        check_killer(ch, victim);

    /* Cap damage */
    dam = std::clamp(dam, 0, MAX_DAMAGE);

    /* illusory mobs still seem to be dealing damage below... */
    if (MOB_FLAGGED(ch, MOB_ILLUSORY))
        dam = 0;

    if (!MOB_FLAGGED(ch, MOB_ILLUSORY)) {
        /* Will the victim die? */
        if DAMAGE_WILL_KILL (victim, dam) {
            /* Give a chance for item triggers to save the victim */
            death_otrigger(victim);
            death = DAMAGE_WILL_KILL(victim, dam);
        }

        /* Vampiric touch */
        if (IS_WEAPON(attacktype) || SKILL_IS_TARGET(attacktype, TAR_CONTACT)) {
            count_hand_eq(ch, &dummy, &attacker_weapons);
            if (ch != victim && attacker_weapons == 0) {
                if (EFF_FLAGGED(ch, EFF_VAMP_TOUCH)) {
                    hurt_char(ch, nullptr, -dam, false);
                } else if (GET_SKILL(ch, SKILL_VAMP_TOUCH) > 0) {
                    if (GET_SKILL(ch, SKILL_VAMP_TOUCH) > random_number(0, 101))
                        hurt_char(ch, nullptr, -dam / 2, true);
                    if (random_number(0, 2))
                        improve_skill(ch, SKILL_VAMP_TOUCH);
                }
            }
        }
    }

    /* You get some exp for doing damage (but not to players or illusions) */
    if (ch != victim && !IS_NPC(ch) && !MOB_FLAGGED(victim, MOB_ILLUSORY))
        gain_exp(ch,
                 (GET_LEVEL(victim) * dam) /
                     std::max(GET_LEVEL(ch) - GET_LEVEL(victim) > 10 ? 30 : 15, 50 - GET_LEVEL(ch)),
                 GAIN_REGULAR);

    /*
     * skill_message sends a message from the messages file in lib/misc.
     * dam_message just sends a generic "You hit $n extremely hard.".
     * skill_message is preferable to dam_message because it is more
     * descriptive.
     *
     * If we are _not_ attacking with a weapon (i.e. a spell), always use
     * skill_message. If we are attacking with a weapon: If this is a miss or a
     * death blow, send a skill_message if one exists; if not, default to a
     * dam_message. Otherwise, always send a dam_message.
     */
    if (attacktype == SKILL_BAREHAND) {
        if (death)
            skill_message(dam, ch, victim, attacktype, death);
        else
            dam_message(dam, ch, victim, attacktype);
    } else if (!IS_WEAPON(attacktype))
        skill_message(dam, ch, victim, attacktype, death);
    else {
        if (death || dam == 0) {
            if (!skill_message(dam, ch, victim, attacktype, death))
                dam_message(dam, ch, victim, attacktype);
        } else
            dam_message(dam, ch, victim, attacktype);
    }

    /* Defensive spells bite back */
    if (ch != victim && GET_LEVEL(ch) < LVL_IMMORT &&
        (IS_WEAPON(attacktype) || SKILL_IS_TARGET(attacktype, TAR_CONTACT))) {
        shdam = defensive_spell_damage(ch, victim, dam);
        if (shdam > 0 && !MOB_FLAGGED(victim, MOB_ILLUSORY))
            hurt_char(ch, victim, shdam, true);
    }

    /* Do the damage. */
    if (!MOB_FLAGGED(ch, MOB_ILLUSORY)) {
        hurt_char(victim, ch, dam, true);
        /* Sanity check */
        if (death && !DECEASED(victim)) {
            log(LogSeverity::Warn, LVL_GOD,
                "Error: Damage of {:d} was predicted to kill {}, but victim survived with {:d} hp", dam,
                GET_NAME(victim), GET_HIT(victim));
        }
    }

    /* If the victim is somehow sleeping after all this, then they can't fight. */
    if (!AWAKE(victim)) {
        if (FIGHTING(victim))
            stop_fighting(victim);
        abort_casting(victim);
    }

    /* If the victim died, award exp and quit. */
    if (DECEASED(victim)) {
        /* If the victim is about to die by something like fire or poison,
         * AND they are in combat with someone, let their opponent get the
         * exp for killing it. */
        if (ch == victim && was_fighting) {
            switch (attacktype) {
            case SPELL_ON_FIRE:
            case SPELL_POISON:
            case TYPE_SUFFERING:
                ch = was_fighting;
                break;
            }
        }

        disburse_kill_exp(ch, victim);
        return VICTIM_DEAD;
    }

    /* Response: stand or flee */
    if (AWAKE(victim)) {
        if (GET_POS(victim) < POS_STANDING) {
            delayed_command(victim, "stand", 4 + (100 - GET_DEX(victim)) * 3 / 10, false);
        } else if (IS_NPC(victim) && GET_HIT(victim) < (GET_MAX_HIT(victim) >> 2)) {
            if (MOB_FLAGGED(victim, MOB_WIMPY) && (ch != victim) && !EFF_FLAGGED(victim, EFF_CHARM))
                delayed_command(victim, "flee", 0, false);
        } else if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
                   GET_HIT(victim) < GET_WIMP_LEV(victim)) {
            char_printf(victim, "You attempt to flee!\n");
            delayed_command(victim, "flee", 0, false);
        }
    }

    return dam;
}

bool riposte(CharData *ch, CharData *victim) {
    int ch_hit, vict_riposte;

    if (!GET_SKILL(victim, SKILL_RIPOSTE))
        return false;

    if (GET_POS(victim) <= POS_SITTING)
        return false;

    if (!GET_EQ(victim, WEAR_WIELD) && !GET_EQ(victim, WEAR_2HWIELD) && !GET_SKILL(victim, SKILL_BAREHAND))
        return false;

    ch_hit = random_number(55, 200);
    ch_hit += GET_HITROLL(ch);
    ch_hit -= monk_weight_penalty(ch);
    vict_riposte = random_number(20, 50);
    vict_riposte += GET_LEVEL(victim) - GET_LEVEL(ch);
    vict_riposte -= dex_app[GET_DEX(victim)].defensive;
    vict_riposte += GET_SKILL(victim, SKILL_RIPOSTE) * 0.085;

    if (random_number(1, 10) < 5)
        improve_skill_offensively(victim, ch, SKILL_RIPOSTE);

    if (vict_riposte <= ch_hit)
        return false;

    if (GET_SKILL(ch, SKILL_BAREHAND) > 20) {
        act("You grab $n's arm and twist it, causing $m to strike $mself!&0", false, ch, 0, victim, TO_VICT);
        act("$N grabs your arm and twists it, causing you to damage to "
            "yourself!&0",
            false, ch, 0, victim, TO_CHAR);
        act("&7$N grabs and twists $n's arm, causing $n to hurt $mself!&0", false, ch, 0, victim, TO_NOTVICT);
    } else {
        act("You block $n's attack, and strike back!&0", false, ch, 0, victim, TO_VICT);
        act("$N blocks your attack, and strikes back!&0", false, ch, 0, victim, TO_CHAR);
        act("&7$N blocks $n's attack, and strikes back at $m!&0", false, ch, 0, victim, TO_NOTVICT);
    }

    hit(victim, ch, SKILL_RIPOSTE);
    return true;
}

bool parry(CharData *ch, CharData *victim) {
    int ch_hit, vict_parry;

    if (!GET_SKILL(victim, SKILL_PARRY))
        return false;

    if (GET_POS(victim) <= POS_SITTING)
        return false;

    if (!GET_EQ(victim, WEAR_WIELD) && !GET_EQ(victim, WEAR_2HWIELD) && !GET_SKILL(victim, SKILL_BAREHAND))
        return false;

    ch_hit = random_number(45, 181);
    ch_hit += GET_HITROLL(ch);
    ch_hit -= monk_weight_penalty(ch);
    vict_parry = random_number(20, 50);
    vict_parry += GET_LEVEL(victim) - GET_LEVEL(ch);
    vict_parry -= dex_app[GET_DEX(victim)].defensive;
    vict_parry += GET_SKILL(victim, SKILL_PARRY) / 10;
    if (random_number(1, 10) < 5)
        improve_skill_offensively(victim, ch, SKILL_PARRY);

    if (vict_parry <= ch_hit)
        return false;

    if (GET_SKILL(ch, SKILL_BAREHAND) > 20) {
        act("&7You smirk and slap $n's attack away with ease.&0", false, ch, 0, victim, TO_VICT);
        act("&7$N smirks and slaps your attack away.&0", false, ch, 0, victim, TO_CHAR);
        act("&7$N smirks as $E slaps $n's attack away.&0", false, ch, 0, victim, TO_NOTVICT);
    } else {
        act("&7You parry $n's attack.&0", false, ch, 0, victim, TO_VICT);
        act("&7$N parries your attack.&0", false, ch, 0, victim, TO_CHAR);
        act("&7$N parries $n's attack.&0", false, ch, 0, victim, TO_NOTVICT);
    }

    set_fighting(ch, victim, true);

    return true;
}

bool dodge(CharData *ch, CharData *victim) {
    int ch_hit, vict_dodge;

    if (!GET_SKILL(victim, SKILL_DODGE))
        return false;
    if (GET_POS(victim) <= POS_SITTING)
        return false;

    ch_hit = random_number(35, 171);
    ch_hit += GET_HITROLL(ch);
    ch_hit -= monk_weight_penalty(ch);
    vict_dodge = random_number(20, 50);
    vict_dodge += GET_LEVEL(victim) - GET_LEVEL(ch);
    vict_dodge -= dex_app[GET_DEX(victim)].defensive;
    vict_dodge += GET_SKILL(victim, SKILL_DODGE) / 10;
    if (random_number(1, 10) < 5)
        improve_skill_offensively(victim, ch, SKILL_DODGE);

    if (vict_dodge <= ch_hit)
        return false;

    if (GET_SKILL(ch, SKILL_BAREHAND) > 20) {
        if (CAN_SEE(ch, victim))
            act("&7You twist in place and watch $n attack the air nearby.&0", false, ch, 0, victim, TO_VICT);
        else
            act("&7You twist in place, avoiding $n's attack.&0", false, ch, 0, victim, TO_VICT);
        act("&7$N twists in place causing you to strike nothing but air!&0", true, ch, 0, victim, TO_CHAR);
        act("&7$N twists in place as $n attacks the air nearby $M.&0", true, ch, 0, victim, TO_NOTVICT);
    } else {
        act("&7You dodge $n's attack.&0", false, ch, 0, victim, TO_VICT);
        act("&7$N dodges your attack.&0", false, ch, 0, victim, TO_CHAR);
        act("&7$N dodges $n's attack.&0", false, ch, 0, victim, TO_NOTVICT);
    }

    set_fighting(ch, victim, true);

    return true;
}

int weapon_special(ObjData *wpn, CharData *ch) {
    int (*name)(CharData * ch, void *me, int cmd, char *argument);

    SPECIAL(lightning_weapon);
    SPECIAL(frost_weapon);
    SPECIAL(fire_weapon);
    SPECIAL(vampiric_weapon);
    SPECIAL(holyw_weapon);

    if (GET_OBJ_RNUM(wpn) == NOTHING)
        return 0;
    name = obj_index[GET_OBJ_RNUM(wpn)].func;
    if (name != vampiric_weapon && name != lightning_weapon && name != holyw_weapon && name != fire_weapon &&
        name != frost_weapon)
        return 0;
    return (name)(ch, wpn, 0, "");
}

void hit(CharData *ch, CharData *victim, int type) {
    int victim_ac, calc_thaco, dam, diceroll, weapon_position, hidden;
    ObjData *weapon;
    int thac0_01 = 25;
    int thac0_00 = classes[(int)GET_CLASS(ch)].thac0;
    bool no_defense_check = false;
    int dtype;

    if (!ch || !victim || !event_target_valid(victim) || !event_target_valid(ch) || ch == victim) {
        if (ch && FIGHTING(ch))
            stop_fighting(ch);
        return;
    }

    /* This should not happen */
    if (IN_ROOM(ch) != IN_ROOM(victim)) {
        log(LogSeverity::Error, LVL_GOD, "hit(): {} [{:d}] is not in the same room as {} [{:d}]", GET_NAME(ch),
            CH_RVNUM(ch), GET_NAME(victim), CH_RVNUM(victim));
        return;
    }

    /* If a Rogue, save hiddenness as a bonus to backstab */
    if (GET_HIDDENNESS(ch) > 0 && GET_CLASS(ch) == CLASS_ROGUE) {        
        hidden = GET_HIDDENNESS(ch);
    } else {
        hidden = 0;
    }

    GET_HIDDENNESS(ch) = 0;

    /* check if the character has a fight trigger */
    fight_mtrigger(ch);

    /* Check for PK, pets, shapechanged...etc. */
    if (!attack_ok(ch, victim, true)) {
        if (FIGHTING(ch))
            stop_fighting(ch);
        return;
    }

    if (FIGHTING(ch) != victim && EFF_FLAGGED(ch, EFF_BLIND)) {
        char_printf(ch, "You cant see a thing!\n");
        log(LogSeverity::Error, LVL_GOD, "hit(): {} is blind but tried to attack new target {} [room {:d}]",
            GET_NAME(ch), GET_NAME(victim), CH_RVNUM(ch));
        return;
    }

    aggro_lose_spells(ch);

    /* See if anyone is guarding the victim.
     * But guarding doesn't apply if this NPC was already fighting the victim. */
    if (FIGHTING(ch) != victim)
        victim = check_guard(ch, victim, false);

    /*
     * This is a hack.   TYPE_UNDEFINED signifies to hit() that
     * we should figure out the damage type based on weapons
     * or barehand.   SKILL_RIPOSTE extends that but signifies
     * that this attack cannot be riposted back (or parried or
     * dodged).   SKILL_DUAL_WIELD signifies that the secondary
     * hand should be used.
     */

    /* Figure out which weapon we want. */
    weapon_position = -1;
    if (type == SKILL_DUAL_WIELD || type == SKILL_2BACK) {
        if (GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON)
            weapon_position = WEAR_WIELD2;
    } else if (GET_EQ(ch, WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == ITEM_WEAPON)
        weapon_position = WEAR_WIELD;
    else if (GET_EQ(ch, WEAR_2HWIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_2HWIELD)) == ITEM_WEAPON)
        weapon_position = WEAR_2HWIELD;

    weapon = weapon_position >= 0 ? GET_EQ(ch, weapon_position) : nullptr;

    /* If riposting, don't allow a defensive skill check. */
    if (type == SKILL_RIPOSTE)
        no_defense_check = true;

    /* Figure out what kind of damage we're doing. */
    if (type == SKILL_BACKSTAB || type == SKILL_2BACK) {
        if (weapon)
            dtype = skill_to_dtype(GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT);
        else
            dtype = DAM_PIERCE;
    } else {
        if (type == TYPE_UNDEFINED || type == SKILL_RIPOSTE || type == SKILL_DUAL_WIELD || !type) {
            if (weapon)
                type = GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT;
            else if (GET_SKILL(ch, SKILL_BAREHAND)) {
                if (EFF_FLAGGED(ch, EFF_FIREHANDS))
                    type = TYPE_FIRE;
                else if (EFF_FLAGGED(ch, EFF_ICEHANDS))
                    type = TYPE_COLD;
                else if (EFF_FLAGGED(ch, EFF_LIGHTNINGHANDS))
                    type = TYPE_SHOCK;
                else if (EFF_FLAGGED(ch, EFF_ACIDHANDS))
                    type = TYPE_ACID;
                else
                    type = SKILL_BAREHAND;
            } else if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
                type = ch->mob_specials.attack_type + TYPE_HIT;
            else if (GET_RACE(ch) == RACE_DRAGONBORN_FIRE || GET_RACE(ch) == RACE_DRAGONBORN_FROST ||
                     GET_RACE(ch) == RACE_DRAGONBORN_LIGHTNING || GET_RACE(ch) == RACE_DRAGONBORN_ACID ||
                     GET_RACE(ch) == RACE_DRAGONBORN_GAS)
                type = TYPE_CLAW;
            else
                type = TYPE_HIT;
        }
        dtype = skill_to_dtype(type);
    }

    /* Common (fleshy) folks will do crushing damage when not wielding a
     * weapon.   That's how fists work.   But other folks, like for example
     * fire elemental's, would do other kinds of damage... like fire. */

    if (!weapon && COMPOSITION_DAM(ch) != DAM_PIERCE && COMPOSITION_DAM(ch) != DAM_SLASH &&
        COMPOSITION_DAM(ch) != DAM_CRUSH) {
        dtype = COMPOSITION_DAM(ch);
    }

    /* Time to see whether it's a hit or a miss */

    /* VALUES: 240 to -50 */
    calc_thaco = calc_thac0(GET_LEVEL(ch), thac0_01, thac0_00) * 10;
    /* VALUES: -50 to 70 */
    calc_thaco -= str_app[GET_STR(ch)].tohit * 10;
    /* VALUES: 0 to 40    (max hitroll is 40) */
    calc_thaco -= GET_HITROLL(ch);
    /* VALUES: 0 to 40 */
    calc_thaco -= (4 * GET_INT(ch)) / 10; /* Int helps! */
    /* VALUES: 0 to 40 */
    calc_thaco -= (4 * GET_WIS(ch)) / 10; /* So does wis */
    /* check for weapon skills */
    if (weapon) {
        if (weapon_proficiency(weapon, weapon_position)) {
            /* VALUES: 0 to 100 */
            calc_thaco -= (GET_SKILL(ch, weapon_proficiency(weapon, weapon_position)) / 2);
        }
        /* check if monk, if so add barehand */
    } else if (GET_CLASS(ch) == CLASS_MONK) {
        /* VALUES: 0 to 100 */
        calc_thaco -= (GET_SKILL(ch, SKILL_BAREHAND) / 2);
    }

    /* check for bless/hex - VALUES: 0 to 20 */
    if (EFF_FLAGGED(ch, EFF_BLESS)) {
        if (IS_GOOD(ch) && IS_EVIL(victim))
            calc_thaco -= GET_LEVEL(ch) / 5; /* good characters get a big bonus to attacking evil characters */
        if (IS_GOOD(ch) && IS_NEUTRAL(victim))
            calc_thaco -= GET_LEVEL(ch) / 10; /* good characters get a smaller bonus to attacking neutral characters */
        if (IS_GOOD(ch) && IS_GOOD(victim))
            calc_thaco += GET_LEVEL(ch) / 10; /* good characters get a small penalty to attacking good characters */
        if (IS_EVIL(ch) && IS_GOOD(victim))
            calc_thaco -= GET_LEVEL(ch) / 5; /* evil characters get a big bonus to attacking good characters */
        if (IS_EVIL(ch) && IS_NEUTRAL(victim))
            calc_thaco -= GET_LEVEL(ch) / 5; /* evil characters get a small bonus to attacking neutral characters */
        if (IS_EVIL(ch) && IS_EVIL(victim))
            calc_thaco += GET_LEVEL(ch) / 10; /* evil characters get a small penalty to attacking evil characters */
        if (IS_NEUTRAL(ch) && !IS_NEUTRAL(victim))
            calc_thaco -=
                GET_LEVEL(ch) / 10; /* neutral characters get a small bonus to attacking good or evil characters */
    }
    /* calc_thaco ranges from 290 to -290 */
    diceroll = random_number(1, 200);

    /* VALUES: 100 to -100 */
    victim_ac = GET_AC(victim);
    /* VALUES: 60 to -60 */
    victim_ac += dex_app[GET_DEX(victim)].defensive * 10;
    victim_ac = std::max(-100, victim_ac); /* -100 is lowest */
    /* victim_ac ranges from 160 to -100 */

    /*
     *    Victim asleep = hit, otherwise:
     *      1..10       = Automatic miss
     *      11..190    = Checked vs. AC
     *      191..200   = Automatic hit
     */
    if (diceroll > 190 || !AWAKE(victim))
        dam = true;
    else if (diceroll < 11)
        dam = false;
    else
        dam = (calc_thaco - diceroll <= victim_ac);

    /* See if the victim evades damage due to non-susceptibility
     * (this applies when asleep, too!)
     */
    if (damage_evasion(victim, ch, weapon, dtype)) {
        damage_evasion_message(ch, victim, weapon, dtype);
        set_fighting(victim, ch, true);

        /* Process Triggers - added here so they still process even if the attack is evaded */
        dam = 0;
        attack_otrigger(ch, victim, dam);
        hitprcnt_mtrigger(victim);
        return;
    }

    /* adjust for additional effects like Displacement */
    if (EFF_FLAGGED(victim, EFF_DISPLACEMENT) || EFF_FLAGGED(victim, EFF_GREATER_DISPLACEMENT)) {
        int mtype;
        bool displaced = false;
        mtype = type - TYPE_HIT; /* get the damage message */

        if (EFF_FLAGGED(victim, EFF_DISPLACEMENT)) {
            if (random_number(1, 5) == 1) /* 20% chance to ignore damage */
                displaced = true;
        }

        if (EFF_FLAGGED(victim, EFF_GREATER_DISPLACEMENT)) {
            if (random_number(1, 3) == 1) /* 33% chance to ignore damage */
                displaced = true;
        }

        if (displaced == true) {
            sprintf(buf, "&9&b$n takes a guess at $N's position but misses with $s %s!&0",
                    attack_hit_text[mtype].singular);
            act(buf, false, ch, 0, victim, TO_NOTVICT);
            sprintf(buf, "&9&bYou take a guess at $N's position but miss with your %s!&0",
                    attack_hit_text[mtype].singular);
            act(buf, false, ch, 0, victim, TO_CHAR);
            sprintf(buf, "&9&b$n takes a guess your position but misses with $s %s!&0",
                    attack_hit_text[mtype].singular);
            act(buf, false, ch, 0, victim, TO_VICT);
            set_fighting(victim, ch, true);
            return;
        }
    }

    /* The attacker missed the victim. */
    if (!dam) {
        damage(ch, victim, 0, type);
    }

    /*
     * Some skills don't get a chance for riposte, parry, and dodge,
     * so short-circuit those function calls here.
     */
    else if (type == SKILL_BACKSTAB || type == SKILL_2BACK || type == SKILL_BAREHAND || no_defense_check ||
             EFF_FLAGGED(ch, EFF_FIREHANDS) || EFF_FLAGGED(ch, EFF_ICEHANDS) || EFF_FLAGGED(ch, EFF_LIGHTNINGHANDS) ||
             EFF_FLAGGED(ch, EFF_ACIDHANDS) ||
             (!riposte(ch, victim) && !parry(ch, victim) && !dodge(ch, victim) &&
              (!weapon || !weapon_special(weapon, ch)))) {
        /*
         * Okay, we know the guy has been hit.   Now calculate damage,
         * starting with the damage bonuses: damroll and strength apply.
         */
        dam = str_app[GET_STR(ch)].todam;
        dam += GET_DAMROLL(ch);

        if (diceroll == 20)
            dam += dam;

        /* Mob barehand damage is always applied */
        if (IS_NPC(ch))
            dam += roll_dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
        /* Apply weapon damage if there is one */
        if (weapon)
            dam += roll_dice(GET_OBJ_VAL(weapon, VAL_WEAPON_DICE_NUM), GET_OBJ_VAL(weapon, VAL_WEAPON_DICE_SIZE));
        /* Apply player barehand damage if no weapon */
        else if (!IS_NPC(ch))
            dam += random_number(0, 2); /* Yeah, you better wield a weapon. */

        /*
         * Include a damage multiplier if the victim isn't ready to fight.
         * alert     x 1.33
         * resting   x 1.66
         * sleeping  x 2.00
         * stunned   x 2.33
         * incap     x 2.66
         * mortally  x 3.00
         * Note, this is a hack because it depends on the particular
         * values of the STANCE_XXX constants.
         */
        if (GET_STANCE(victim) < STANCE_FIGHTING)
            dam *= 1 + (STANCE_FIGHTING - GET_STANCE(victim)) / 3;

        dam = std::max(1, dam); /* at least 1 hp damage min per hit */

        if (type == SKILL_BACKSTAB || type == SKILL_2BACK) {
            dam *= GET_SKILL(ch, SKILL_BACKSTAB) / 10 + 1;

            if (GET_CLASS(ch) == CLASS_ROGUE)
                dam += ((hidden / 2) * (GET_SKILL(ch, SKILL_SNEAK_ATTACK) / 100));

        } else if (type == SKILL_BAREHAND || EFF_FLAGGED(ch, EFF_FIREHANDS) || EFF_FLAGGED(ch, EFF_ICEHANDS) ||
                 EFF_FLAGGED(ch, EFF_LIGHTNINGHANDS) || EFF_FLAGGED(ch, EFF_ACIDHANDS))
            dam += GET_SKILL(ch, SKILL_BAREHAND) / 4 + random_number(1, GET_LEVEL(ch) / 3) + (GET_LEVEL(ch) / 2);

        else {
            /* Spirit of the bear increases the damage you do by up to 10%. */
            if (EFF_FLAGGED(ch, EFF_SPIRIT_BEAR))
                dam *= 1.0 + GET_LEVEL(ch) / 1000.0;

            /* Berserk increases damage done and taken by 10%. */
            if (EFF_FLAGGED(victim, EFF_BERSERK) || EFF_FLAGGED(ch, EFF_BERSERK))
                dam *= 1.1;

            if (EFF_FLAGGED(victim, EFF_STONE_SKIN) && random_number(0, 10) <= 9) {
                decrease_modifier(victim, SPELL_STONE_SKIN);
                dam = random_number(0, 3);
            }
        }

        if (type == TYPE_HIT && !dam)
            type = SKILL_PUNCH;

        /* If the weapon is flagged to do a special kind of energy damage, alter it. */
        if (weapon) {
            dtype = convert_weapon_damage(weapon);
            type = convert_weapon_type(weapon);
        }

        /* Adjust damage for susceptibility */
        dam = dam_suscept_adjust(ch, victim, weapon, dam, dtype);

        /* Do the damage */
        damage(ch, victim, dam, type);

        if (ALIVE(victim) && EFF_FLAGGED(victim, EFF_IMMOBILIZED))
            decrease_modifier(victim, SPELL_BONE_CAGE);

        /* Do after the damage() so we don't send the wrong type. */
        if (ALIVE(ch) && weapon && !random_number(0, 9) && (type = weapon_proficiency(weapon, weapon_position)) != -1) {
            improve_skill_offensively(ch, victim, type);
        }
    }

    /* Process Triggers */
    attack_otrigger(ch, victim, dam);
    hitprcnt_mtrigger(victim);
}

/* Get the skill associated with a weapon. */
int weapon_proficiency(ObjData *weapon, int position) {
    int w_type;

    w_type = GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT;

    switch (w_type) {
        /* These are generic type weapons. */
    case TYPE_HIT:
    case TYPE_BITE:
    case TYPE_THRASH:
    case TYPE_PUNCH:
    case TYPE_BLAST:
    case TYPE_FIRE:
    case TYPE_COLD:
    case TYPE_ACID:
    case TYPE_SHOCK:
    case TYPE_POISON:
    case TYPE_ALIGN:
        return -1;

        /* These are slashing weapons. */
    case TYPE_WHIP:
    case TYPE_SLASH:
    case TYPE_CLAW:
        return position == WEAR_2HWIELD ? SKILL_2H_SLASHING : SKILL_SLASHING;

        /* These are bludgeoning weapons. */
    case TYPE_BLUDGEON:
    case TYPE_CRUSH:
    case TYPE_POUND:
    case TYPE_MAUL:
        return position == WEAR_2HWIELD ? SKILL_2H_BLUDGEONING : SKILL_BLUDGEONING;

        /* These are piercing weapons. */
    case TYPE_STING:
    case TYPE_PIERCE:
    case TYPE_STAB:
        return position == WEAR_2HWIELD ? SKILL_2H_PIERCING : SKILL_PIERCING;

    default:
        log("SYSERR:fight.c:weapon_proficiency: Unknown weapon type");
        return -1;
    }
}

#define MAJORPCODE 1

/* control the fights going on.   Called every 2 seconds from comm.c. */
void perform_violence(void) {
    CharData *ch, *newvict;
    int hits, secondary_hits;

    CharData *random_attack_target(CharData * ch, CharData * target, bool verbose);

    for (ch = combat_list; ch; ch = next_combat_list) {
        next_combat_list = ch->next_fighting;

        if (FIGHTING(ch) == nullptr || DECEASED(FIGHTING(ch)) || ch->in_room != FIGHTING(ch)->in_room ||
            EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS)) {
            stop_fighting(ch);
            continue;
        }
#ifdef MAJORPCODE
        if (EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS)) {
            char_printf(ch, "You remain paralyzed and can't do a thing to defend yourself..\n");
            act("$n strains to respond to $N's attack, but the paralysis is too "
                "overpowering.",
                false, ch, 0, FIGHTING(ch), TO_ROOM);
            if (MOB_FLAGGED(ch, MOB_MEMORY))
                remember(ch, FIGHTING(ch));
            stop_fighting(ch);
            continue;
        }
#endif

        if (check_disarmed(ch))
            continue;

        if (CASTING(ch))
            continue;

        if (GET_POS(ch) < POS_STANDING) {
            char_printf(ch, "You can't fight while sitting!!\n");
            continue;
        }

        if (GET_SKILL(ch, SKILL_BERSERK))
            GET_RAGE(ch) += 10 + random_number(0, GET_SKILL(ch, SKILL_BERSERK) / 10);

        /* Everybody gets at least one hit */
        hits = 1;
        if (GET_SKILL(ch, SKILL_DOUBLE_ATTACK) && GET_SKILL(ch, SKILL_DOUBLE_ATTACK) >= random_number(1, 101))
            hits *= 2;
        if (EFF_FLAGGED(ch, EFF_HASTE))
            hits += 1;
        if (EFF_FLAGGED(ch, EFF_BLUR))
            hits += 1;
        if (EFF_FLAGGED(ch, EFF_SPIRIT_WOLF) && GET_LEVEL(ch) > random_number(0, 100))
            hits += 1;

        secondary_hits = 0;
        if (GET_EQ(ch, WEAR_WIELD2) && GET_SKILL(ch, SKILL_DUAL_WIELD) &&
            GET_SKILL(ch, SKILL_DUAL_WIELD) >= random_number(1, 101)) {
            secondary_hits = 1;
            if (GET_SKILL(ch, SKILL_DOUBLE_ATTACK) && GET_SKILL(ch, SKILL_DOUBLE_ATTACK) >= random_number(1, 101))
                secondary_hits *= 2;
            if (EFF_FLAGGED(ch, EFF_BLUR))
                secondary_hits += 2;
            if (EFF_FLAGGED(ch, EFF_NIMBLE))
                secondary_hits += 1;
        }



        /* Chance for NPCs to switch. */
        if (IS_NPC(ch) && GET_SKILL(ch, SKILL_SWITCH) && !EFF_FLAGGED(ch, EFF_BLIND) &&
            !random_number(0, std::max(10, 30 - GET_LEVEL(ch)))) {
            CharData *victim = nullptr, *tch;

            /* Find the player fighting this NPC who has the lowest hp in the room */
            for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
                if (FIGHTING(tch) == ch && !IS_NPC(tch) && CAN_SEE(ch, tch))
                    if (victim == nullptr || GET_HIT(tch) < GET_HIT(victim))
                        victim = tch;

            if (victim && FIGHTING(ch) != victim) {
                act("$n switches to $N!", false, ch, 0, victim, TO_NOTVICT);
                act("$n switches to YOU!", false, ch, 0, victim, TO_VICT);
                switch_target(ch, victim);
                hits = 1;
                secondary_hits = 0;
            }
        }

        /* If you're confused, you may spontaneously switch.   But only once per
         * round, not once per hit. */
        if (CONFUSED(ch) && random_number(0, 3) == 0) {
            newvict = random_attack_target(ch, FIGHTING(ch), true);
            if (newvict != FIGHTING(ch))
                switch_target(ch, newvict);
        }

        do {
            if (hits) {
                hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
                --hits;
            }
            if (FIGHTING(ch) && ALIVE(ch) && secondary_hits) {
                hit(ch, FIGHTING(ch), SKILL_DUAL_WIELD);
                --secondary_hits;
            }
        } while ((hits > 0 || secondary_hits > 0) && FIGHTING(ch) && ALIVE(ch));

        if (!ALIVE(ch))
            continue;

        if (GET_EQ(ch, WEAR_WIELD2) && GET_SKILL(ch, SKILL_DUAL_WIELD) && random_number(0, 9) < 3)
            improve_skill_offensively(ch, FIGHTING(ch), SKILL_DUAL_WIELD);

        if (GET_SKILL(ch, SKILL_DOUBLE_ATTACK) && random_number(0, 9) < 3)
            improve_skill_offensively(ch, FIGHTING(ch), SKILL_DOUBLE_ATTACK);

        if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != nullptr)
            (mob_index[GET_MOB_RNUM(ch)].func)(ch, ch, 0, "");
    }
}

void pickup_dropped_weapon(CharData *ch) {
    ObjData *obj;
    char arg[MAX_INPUT_LENGTH];

    ACMD(do_wield);

    for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
        if (OBJ_FLAGGED(obj, ITEM_WAS_DISARMED) && obj->last_to_hold == ch) {
            obj_from_room(obj);
            obj_to_char(obj, ch);

            if (FIGHTING(ch))
                act("$n finally gets a steady grip on $p.", false, ch, obj, 0, TO_ROOM);
            else
                act("$n eagerly reaches for $p.", false, ch, obj, 0, TO_ROOM);

            REMOVE_FLAG(GET_OBJ_FLAGS(obj), ITEM_WAS_DISARMED);

            strncpy(arg, obj->name, sizeof(arg) - 1);
            do_wield(ch, arg, 0, 0);
            return;
        }
    }
}

/* This function serves to allow:

 *   1) MOBs/PCs to have their penalties removed upon recovery from a fumble.
 *       If they're fighting no one (ie. vict fleed), they recover immediately.
 *   2) MOBs to pick up and re-wield both weapons when their
 ACT_DELAY_DROPPED_WEAP action_delay[] expires.
 *
 *   PCs don't need automatic weapon recovery, as they can manually do it.
 *
 *   This function is called from perform_violence(), and thus is only called
 *   when the character has valid disarm cooldowns and is combat.   Since disarm
 *   can't be used until after combat has been initiated, and the affects
 *   it produces are really only useful while in combat, when the affected
 *   char's assailant has fled the room, it'll be perfectly fine to just
 *   clear all EFFECT-flags and clear all action_delays[] associated with
 *   'disarm'.   This includes when a weapon is dislodged and laying on the
 *   ground--the disarmed MOB will immediately pick it up, and the disarmed
 *   PC may immediately pick his/hers up.
 *
 *   Note: MOB/PCs can NOT be both FUMBLING_WEAP and DROPPED_WEAP for same weap.
 *
 *   Return false if the PC/MOB can attack.   Otherwise, return true.
 */
bool check_disarmed(CharData *ch) {
    /* if not fumbling nor regaining dropped weap, let em strike back */
    if (!GET_COOLDOWN(ch, CD_FUMBLING_PRIMARY) && !GET_COOLDOWN(ch, CD_FUMBLING_SECONDARY) &&
        !GET_COOLDOWN(ch, CD_DROPPED_PRIMARY) && !GET_COOLDOWN(ch, CD_DROPPED_SECONDARY))
        return false;

    /* if PC/MOB isn't fighting anyone anymore (vict fleed), remove
       all affect-flags and pick up all dropped weapons */
    if (!FIGHTING(ch)) {
        if (GET_COOLDOWN(ch, CD_DROPPED_PRIMARY) || GET_COOLDOWN(ch, CD_DROPPED_SECONDARY))
            pickup_dropped_weapon(ch);
        GET_COOLDOWN(ch, CD_FUMBLING_PRIMARY) = 0;
        GET_COOLDOWN(ch, CD_FUMBLING_SECONDARY) = 0;
        GET_COOLDOWN(ch, CD_DROPPED_PRIMARY) = 0;
        GET_COOLDOWN(ch, CD_DROPPED_SECONDARY) = 0;
        return false;
    } else {
        if (GET_COOLDOWN(ch, CD_FUMBLING_PRIMARY) || GET_COOLDOWN(ch, CD_FUMBLING_SECONDARY)) {
            act("$n is trying to get a steady grip on $s weapon.", false, ch, 0, 0, TO_ROOM);
            char_printf(ch, "You can't seem to get a steady grip on your weapon.\n");
        } else if (IS_NPC(ch))
            act("$n struggles to regain $s weapon.", false, ch, 0, 0, TO_ROOM);
    }

    return true;
}

/*This is command to sets pc's aggressive state. When set the pc will
   attack aggressive monsters. Depending on thier current hitpoints
   Banyal */
ACMD(do_aggr) {
    int hp;

    if (IS_NPC(ch)) {
        char_printf(ch, "NPCs can't set their aggressiveness!\n");
        return;
    }

    one_argument(argument, buf);

    /* No argument?   Check aggressiveness. */
    if (!*buf) {
        if (GET_AGGR_LEV(ch) <= 0) {
            char_printf(ch, "You are not aggressive to monsters.\n");
            return;
        }
        char_printf(ch, "You will be aggressive unless your hitpoints drop below {:d}.\n", GET_AGGR_LEV(ch));
        return;
    }

    hp = atoi(buf);

    if (strcasecmp(buf, "off") == 0 || (!hp && *buf == '0')) {
        GET_AGGR_LEV(ch) = 0;
        char_printf(ch, "You are no longer aggressive to monsters.\n");
        return;
    }

    if (hp < 0) {
        char_printf(ch, "Aggressive while dying?   Not likely!\n");
        return;
    }

    GET_AGGR_LEV(ch) = hp;
    do_aggr(ch, "", 0, 0); /* show aggr status message. */
}

/* new function to calculate thac0 for PCs instead of tabled data */
int calc_thac0(int level, int thac0_01, int thac0_00) { return thac0_01 + level * (thac0_00 - thac0_01) / 100; }
