/***************************************************************************
 *  File: chars.c                                        Part of FieryMUD  *
 *  Usage: Source file for characters (mobs and players)                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "act.h"
#include "casting.h"
#include "charsize.h"
#include "class.h"
#include "comm.h"
#include "composition.h"
#include "conf.h"
#include "constants.h"
#include "damage.h"
#include "db.h"
#include "dg_scripts.h"
#include "events.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "lifeforce.h"
#include "magic.h"
#include "math.h"
#include "movement.h"
#include "races.h"
#include "screen.h"
#include "skills.h"
#include "spell_parser.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

ACMD(do_flee); /* act.offensive.c */

const char *stats_display = "&0&7&b[s]&0 Strength      &0&7&b[i]&0 Intelligence\r\n"
                            "&0&7&b[w]&0 Wisdom        &0&7&b[c]&0 Constitution\r\n"
                            "&0&7&b[d]&0 Dexterity     &0&7&b[m]&0 Charisma\r\n\r\n";

#define Y TRUE
#define N FALSE

int class_ok_race[NUM_RACES][NUM_CLASSES] = {
    /* RACE   So Cl Th Wa Pa An Ra Dr Sh As Me Ne Co Mo Be Pr Di My Ro Ba Py Cr Il Hu */
    /* Hu */ {Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, N},
    /* El */ {Y, Y, Y, Y, N, N, Y, Y, N, N, N, N, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, N},
    /* Gn */ {Y, Y, N, N, N, N, N, N, Y, N, N, N, Y, N, N, Y, Y, N, N, N, Y, Y, Y, N},
    /* Dw */ {N, Y, Y, Y, Y, N, N, N, N, N, Y, N, Y, N, Y, Y, N, N, Y, Y, N, N, N, N},
    /* Tr */ {N, N, N, Y, N, N, N, N, Y, N, Y, N, N, N, Y, N, N, N, Y, N, N, N, N, Y},
    /* Dr */ {Y, Y, N, Y, N, Y, N, N, Y, Y, Y, Y, Y, N, N, N, Y, N, Y, N, Y, Y, Y, Y},
    /* Du */ {N, Y, Y, Y, N, N, N, N, N, Y, Y, N, N, N, Y, N, Y, N, Y, N, N, N, N, Y},
    /* Og */ {N, N, N, Y, N, N, N, N, Y, N, Y, N, N, N, Y, N, N, N, Y, N, N, N, N, Y},
    /* Or */ {Y, Y, Y, Y, N, Y, N, N, Y, Y, Y, Y, Y, N, Y, N, Y, N, Y, N, Y, Y, Y, Y},
    /* HE */ {Y, Y, Y, Y, N, N, Y, Y, N, N, N, N, Y, Y, N, Y, N, N, Y, Y, Y, Y, Y, N},
    /* Ba */ {N, N, N, Y, N, N, N, N, Y, N, Y, N, N, N, Y, N, N, N, Y, N, N, N, N, N},
    /* Ha */ {Y, Y, Y, Y, N, N, N, N, N, N, N, N, Y, N, N, Y, N, N, Y, Y, Y, Y, Y, N},
    /*plnt*/ {},
    /*hmnd*/ {},
    /*anml*/ {},
    /*drgn*/ {},
    /*gint*/ {},
    /*othr*/ {},
    /*gbln*/ {},
    /*demn*/ {},
    /*brwn*/ {},
    /*fire*/ {},
    /*frst*/ {},
    /*acid*/ {},
    /*ligh*/ {},
    /*gas */ {},
    /*DbFi*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, Y, Y, N},
    /*DbFr*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, Y, Y, N},
    /*DbAc*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, Y, Y, N},
    /*DbLi*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, Y, Y, N},
    /*DbGa*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, Y, Y, N},
};

int get_base_saves(struct char_data *ch, int type) {
    /* Here are default values for saving throws: */
    int saves[NUM_SAVES] = {105, 115, 105, 110, 110};
    int i;

    if (type < 0 || type >= NUM_SAVES) {
        sprintf(buf, "SYSERR: get_base_saves: invalid type %d", type);
        log(buf);
        return 100;
    }

    if (VALID_CLASS(ch)) {
        for (i = 0; i < NUM_SAVES; i++)
            saves[i] = classes[(int)GET_CLASS(ch)].saves[i];
    } /* else leave the default values */

    /* decrease by 1 point per 2 levels */
    for (i = 0; i < NUM_SAVES; i++)
        saves[i] -= (int)(GET_LEVEL(ch) / 2);

    /* dwarves, gnomes, and halflings get better saves */
    /* FIXME: handle in races.c */
    switch (GET_RACE(ch)) {
    case RACE_DUERGAR:
    case RACE_DWARF:
    case RACE_DRAGONBORN_FIRE:
    case RACE_DRAGONBORN_FROST:
    case RACE_DRAGONBORN_ACID:
    case RACE_DRAGONBORN_LIGHTNING:
    case RACE_DRAGONBORN_GAS:
        saves[SAVING_PARA] -= (int)(0.125 * GET_VIEWED_CON(ch));
        saves[SAVING_ROD] -= (int)(0.1 * GET_VIEWED_CON(ch));
        saves[SAVING_SPELL] -= (int)(0.1 * GET_VIEWED_CON(ch));
        break;
    case RACE_GNOME:
        saves[SAVING_ROD] -= (int)(0.1 * GET_VIEWED_CON(ch));
        saves[SAVING_SPELL] -= (int)(0.1 * GET_VIEWED_CON(ch));
        break;
    case RACE_HALFLING:
        saves[SAVING_PARA] -= (int)(0.1 * GET_VIEWED_CON(ch));
        saves[SAVING_ROD] -= (int)(0.1 * GET_VIEWED_CON(ch));
        saves[SAVING_SPELL] -= (int)(0.1 * GET_VIEWED_CON(ch));
        break;
    }

    /* stats affect saves */
    saves[SAVING_PARA] -= (int)(0.5 * (GET_VIEWED_CON(ch) - 90));
    saves[SAVING_SPELL] -= (int)(0.5 * (GET_VIEWED_WIS(ch) - 90));
    saves[SAVING_ROD] -= (int)(0.5 * (GET_VIEWED_DEX(ch) - 90));
    saves[SAVING_BREATH] -= (int)(0.5 * (GET_VIEWED_DEX(ch) - 90));

    return saves[type];
}

/*
 * Roll the 6 stats for a PC/NPC... each stat is made of 2*NUMBER.  Where
 * NUMBER is the best 5 out of 6 rolls of a random number from 3 to 10.
 * This gives a range of 2*(18-3)=30 TO 2*(60-10)=100.
 * Each class then decides which priority will be given for the best to worst
 * stats.
 */

void roll_natural_abils(struct char_data *ch) {
    int i, j, k, temp;
    ubyte table[6];
    ubyte rolls[6];

    for (i = 0; i < 6; i++)
        table[i] = 0;
    /* loop through each stat */
    for (i = 0; i < 6; i++) {
        /* roll a number from 3 to 10 */
        for (j = 0; j < 6; j++)
            rolls[j] = number(3, 10);
        /* sum the best 5 out of 6 */
        temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] + rolls[4] + rolls[5] -
               MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], MIN(rolls[3], MIN(rolls[4], rolls[5])))));
        /* multiply by 2 */
        temp = temp * 2;
        /* this arranges the rolls from lowest to highest in table[] */
        for (k = 0; k < 6; k++)
            if (table[k] < temp) {
                temp ^= table[k];
                table[k] ^= temp;
                temp ^= table[k];
            }
    }
    /* At this point we have six stats from lowest to highest in table[] */

    /* time to give a bonus if necessary */

    /* this sums the total stats into i */
    i = 0;
    for (j = 0; j < 6; j++)
        i = i + (int)table[j];
    /* ok lets say average = 450 (75 per) if LESS then this then bonus stats */
    /* random spread of bonus 5 stat points */
    while (i <= 450) {
        j = number(0, 5);
        if ((int)table[j] <= 95) {
            table[j] = (ubyte)((int)table[j] + 5);
            i = i + 5;
        }
    }

    /* Arrange the stats according to the class.  Each class has stats that are
     * most important; the highest rolled values will be assigned to those stats.
     */
    if (VALID_CLASS(ch)) {
        for (i = 0; i < NUM_STATS; i++) {
            switch (classes[(int)GET_CLASS(ch)].statorder[i]) {
            case STAT_STR:
                GET_NATURAL_STR(ch) = table[i];
                break;
            case STAT_DEX:
                GET_NATURAL_DEX(ch) = table[i];
                break;
            case STAT_CON:
                GET_NATURAL_CON(ch) = table[i];
                break;
            case STAT_WIS:
                GET_NATURAL_WIS(ch) = table[i];
                break;
            case STAT_INT:
                GET_NATURAL_INT(ch) = table[i];
                break;
            case STAT_CHA:
                GET_NATURAL_CHA(ch) = table[i];
                break;
            default:
                sprintf(buf, "SYSERR: roll_natural_abils(): class '%s' statorder is broken",
                        classes[(int)GET_CLASS(ch)].name);
                log(buf);
            }
        }
    } else {
        GET_NATURAL_STR(ch) = table[0];
        GET_NATURAL_DEX(ch) = table[1];
        GET_NATURAL_CON(ch) = table[2];
        GET_NATURAL_WIS(ch) = table[3];
        GET_NATURAL_INT(ch) = table[4];
        GET_NATURAL_CHA(ch) = table[5];
    }
}

/* this rolls a random skill based on the level of the mob */
/* Values range from 55 to 1000. --gurlaek 7/3/1999         */

int roll_mob_skill(int level) {
    int x, tmp, value = number(50, 100); /* ok give him 50-100 to start with */

    for (x = 1; x < level; x++) {
        tmp = number(5, 15); /* add an additional 5-15 per level */
        if ((value + tmp) <= 1000) {
            value += tmp;
        } else {
            value = 1000;
        }
    }
    return value;
}

int roll_skill(struct char_data *ch, int skill) {
    int value = number(50 + 5 * GET_LEVEL(ch), 100 + 15 * GET_LEVEL(ch));
    int max = return_max_skill(ch, skill);
    return MIN(value, max);
}

void rider_flowoff(struct char_data *rider, struct char_data *mount) {
    act("You suddenly find yourself flowing down off $N's back.", FALSE, rider, 0, mount, TO_CHAR);
    act("$n loses cohesion and flows down off your back.", FALSE, rider, 0, mount, TO_VICT);
    act("Unable to keep $s seat, $n suddenly flows down from $N's back.", TRUE, rider, 0, mount, TO_NOTVICT);
    dismount_char(rider);
}

void rider_fallthrough(struct char_data *rider, struct char_data *mount) {
    act("$N is no longer able to support you, and you fall through $M to the "
        "ground.",
        FALSE, rider, 0, mount, TO_CHAR);
    act("$n falls through your fluid body and ends up on the ground.", FALSE, rider, 0, mount, TO_VICT);
    act("$n suddenly finds $mself falling through $N, and ends up on the ground.", FALSE, rider, 0, mount, TO_NOTVICT);
    dismount_char(rider);
}

void composition_check(struct char_data *ch) {
    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return;
    remove_unsuitable_spells(ch);

    /* See if you're changing your rigidity, and mounted */
    if (RIDING(ch)) {
        if (RIGID(ch) && !RIGID(RIDING(ch))) {
            rider_fallthrough(ch, RIDING(ch));
        } else if (!RIGID(ch) && RIGID(RIDING(ch))) {
            rider_flowoff(ch, RIDING(ch));
        }
    } else if (RIDDEN_BY(ch) && GET_LEVEL(RIDDEN_BY(ch)) < LVL_IMMORT) {
        if (RIGID(ch) && !RIGID(RIDDEN_BY(ch))) {
            rider_flowoff(RIDDEN_BY(ch), ch);
        } else if (!RIGID(ch) && RIGID(RIDDEN_BY(ch))) {
            rider_fallthrough(RIDDEN_BY(ch), ch);
        }
    }
}

/* Find the character's susceptibility to a particular type of damage.
 * The standard value is 100, meaning 100%.  Resistance will result in
 * a lower value, while vulnerability will result in a higher value.
 *
 * Zero is the lowest possible susceptibility, and it means immunity.
 */
int susceptibility(struct char_data *ch, int dtype) {
    int sus;

    if (!VALID_COMPOSITION(ch) || !VALID_LIFEFORCE(ch))
        return 100;

    switch (dtype) {
    case DAM_SLASH:
        return compositions[GET_COMPOSITION(ch)].sus_slash;
    case DAM_PIERCE:
        return compositions[GET_COMPOSITION(ch)].sus_pierce;
    case DAM_CRUSH:
        if (EFF_FLAGGED(ch, EFF_NEGATE_EARTH))
            return 0;
        if (EFF_FLAGGED(ch, EFF_PROT_EARTH))
            return compositions[GET_COMPOSITION(ch)].sus_crush * 75 / 100;
        return compositions[GET_COMPOSITION(ch)].sus_crush;
    case DAM_SHOCK:
        if (EFF_FLAGGED(ch, EFF_NEGATE_AIR))
            return 0;
        if (EFF_FLAGGED(ch, EFF_PROT_AIR))
            return compositions[GET_COMPOSITION(ch)].sus_shock * 75 / 100;
        return compositions[GET_COMPOSITION(ch)].sus_shock;
    case DAM_FIRE:
        /* Negate heat: immune */
        if (EFF_FLAGGED(ch, EFF_NEGATE_HEAT))
            return 0;
        sus = compositions[GET_COMPOSITION(ch)].sus_fire;
        /* Coldshield: reduce by 25% */
        if (EFF_FLAGGED(ch, EFF_COLDSHIELD))
            sus = sus * 75 / 100;
        /* Protection from fire: reduce by 25% */
        if (EFF_FLAGGED(ch, EFF_PROT_FIRE))
            sus = sus * 75 / 100;
        return sus;
    case DAM_WATER:
        return compositions[GET_COMPOSITION(ch)].sus_water;
    case DAM_COLD:
        /* Negate cold: immune */
        if (EFF_FLAGGED(ch, EFF_NEGATE_COLD))
            return 0;
        sus = compositions[GET_COMPOSITION(ch)].sus_cold;
        /* Fireshield: reduce by 25% */
        if (EFF_FLAGGED(ch, EFF_FIRESHIELD))
            sus = sus * 75 / 100;
        /* Protection from cold: reduce by 25% */
        if (EFF_FLAGGED(ch, EFF_PROT_COLD))
            sus = sus * 75 / 100;
        return sus;
    case DAM_ACID:
        if (EFF_FLAGGED(ch, EFF_NEGATE_EARTH))
            return 0;
        if (EFF_FLAGGED(ch, EFF_PROT_EARTH))
            return compositions[GET_COMPOSITION(ch)].sus_acid * 75 / 100;
        return compositions[GET_COMPOSITION(ch)].sus_acid;
    case DAM_POISON:
        if (MOB_FLAGGED(ch, MOB_NOPOISON))
            return 0;
        return compositions[GET_COMPOSITION(ch)].sus_poison;
    case DAM_HEAL:
        return lifeforces[GET_LIFEFORCE(ch)].sus_heal;
    case DAM_ALIGN:
        return 100;
    case DAM_DISPEL:
        return lifeforces[GET_LIFEFORCE(ch)].sus_dispel;
    case DAM_DISCORPORATE:
        return lifeforces[GET_LIFEFORCE(ch)].sus_discorporate;
    default:
        return 100;
    }
}

/* Use this to determine whether a victim evades some attack that's
 * all or nothing, like sleep. */
bool boolean_attack_evasion(struct char_data *ch, int power, int dtype) {
    return number(1, 100) < MAX(3, 110 + GET_LEVEL(ch) - susceptibility(ch, dtype) - power);
}

struct obj_data *equipped_weapon(struct char_data *ch) {
    int weapon_position = -1;

    if (GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON)
        weapon_position = WEAR_WIELD2;
    else if (GET_EQ(ch, WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == ITEM_WEAPON)
        weapon_position = WEAR_WIELD;
    else if (GET_EQ(ch, WEAR_2HWIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_2HWIELD)) == ITEM_WEAPON)
        weapon_position = WEAR_2HWIELD;

    return weapon_position >= 0 ? GET_EQ(ch, weapon_position) : NULL;
}

int dam_suscept_adjust(struct char_data *ch, struct char_data *victim, struct obj_data *weapon, int dam, int dtype) {
    if (!victim)
        return dam;
    /* Adjust damage for susceptibility */
    if (ch && !(GET_COMPOSITION(victim) == COMP_ETHER && blessed_blow(ch, weapon)))
        dam = dam * susceptibility(victim, dtype) / 100;
    return dam;
}

const char *align_color(int align) {
    if (align >= 500)
        return "@Y";
    else if (align >= 350)
        return "@y";
    else if (align > -350)
        return "@G";
    else if (align > 500)
        return "@r";
    else
        return "@R";
}

/* ALTER_POS
 *
 * This is called to modify a character's position and stance, based
 * on miscellaneous causes.  For example, if a player types "sit",
 * or if a dragon knocks someone down with its tail, alter_pos() would
 * be called.  The caller requests a specific stance and/or position
 * that the character should adopt.
 *
 * (This is in contrast to hp_pos_check(), which modifies the stance
 * and/or position based on health.)
 *
 * It's merely a request, however - the function won't make changes
 * that are inconsistent with a character's condition.  For example,
 * low hit points or a SLEEP spell would prevent a character
 * from becoming alert.
 *
 * It does not send messages.  That's the responsibility of
 * the caller.
 */

void alter_pos(struct char_data *ch, int newpos, int newstance) {
    /* Make stance changes first since they will restrict the available positions.
     */
    if (newstance >= 0 && newstance < NUM_STANCES) {
        if (GET_HIT(ch) > 0) {

            /* When your HP are positive, you have the opportunity to be
             * awake, sleeping, or stunned. */

            if (newstance > STANCE_SLEEPING && !EFF_FLAGGED(ch, EFF_SLEEP))
                /* So you wanna be awake?  Don't have a sleep spell on you... */
                GET_STANCE(ch) = newstance;
            else if (newstance == STANCE_SLEEPING || newstance == STANCE_STUNNED)
                GET_STANCE(ch) = newstance;

        } else if (newstance <= STANCE_STUNNED)
            /* When your HP are <= 0, you can only be stunned or worse. */
            GET_STANCE(ch) = newstance;
    }

    if (newpos >= 0 && newpos < NUM_POSITIONS) {
        switch (newpos) {
        case POS_PRONE:
            /* prone position is always available */
            GET_POS(ch) = newpos;
            break;
        case POS_FLYING:
            /* flying - you need the flag and must be alert or fighting */
            if (EFF_FLAGGED(ch, EFF_FLY) && GET_STANCE(ch) > STANCE_RESTING)
                GET_POS(ch) = newpos;
            break;
        default:
            /* if you're awake, you can get any position (except flying) */
            if (GET_STANCE(ch) > STANCE_SLEEPING)
                GET_POS(ch) = newpos;
        }
    }

    if (CASTING(ch) && !valid_cast_stance(ch, ch->casting.spell)) {
        STOP_CASTING(ch);
        act("$n ceases casting $s spell.", FALSE, ch, 0, 0, TO_ROOM);
        act("You stop casting your spell.", FALSE, ch, 0, 0, TO_CHAR);
    }

    if (FIGHTING(ch) && GET_STANCE(ch) < STANCE_RESTING)
        stop_battling(ch);
    falling_check(ch);
    mount_pos_check(ch);
}

void hp_stance_alteration(struct char_data *ch, struct char_data *attacker, int newpos, int newstance, int dam) {
    alter_pos(ch, newpos, newstance);

    if (GET_STANCE(ch) < STANCE_SLEEPING)
        stop_merciful_attackers(ch);

    /* Sanity check. */
    if (GET_POS(ch) != newpos || GET_STANCE(ch) != newstance) {
        sprintf(buf,
                "ERR: hp_stance_alteration(): Tried to change to %s/%s but "
                "alter_pos() left values at %s/%s (for %s)",
                position_types[newpos], stance_types[newstance], position_types[GET_POS(ch)],
                stance_types[GET_STANCE(ch)], GET_NAME(ch));
        mudlog(buf, BRF, LVL_GOD, TRUE);
    }

    /* Send messages and cause dying. */
    switch (GET_STANCE(ch)) {
    case STANCE_MORT:
        act("$n is mortally wounded, and will die soon if not aided.", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("You are mortally wounded, and will die soon if not aided.\r\n", ch);
        break;
    case STANCE_INCAP:
        act("$n is incapacitated and will slowly die, if not aided.", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("You are incapacitated an will slowly die, if not aided.\r\n", ch);
        break;
    case STANCE_STUNNED:
        act("$n is stunned, but will probably regain consciousness again.", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("You're stunned, but will probably regain consciousness again.\r\n", ch);
        break;
    case STANCE_DEAD:
        die(ch, attacker);
        act("$n is dead!  R.I.P.", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char("You are dead!  Sorry...\r\n", ch);
        break;
    default:
        if (dam < 0) {
            /* Healing */
            if (GET_STANCE(ch) == STANCE_SLEEPING) {
                act("$n appears to be stabilized, but $e remains unconscious.", TRUE, ch, 0, 0, TO_ROOM);
            } else {
                act("$n regains consciousness.", TRUE, ch, 0, 0, TO_ROOM);
                send_to_char("You regain consciousness.\r\n", ch);
                look_at_room(ch, FALSE);
            }
        }
    }
}

/* HP_POS_CHECK
 *
 * You've just had your HP modified.  Maybe some other character was
 * responsible.
 *
 * Messages may be sent about any consequences of this HP change.
 * Your stance and position will be modified if necessary.
 * You may die.  If so, your killer may receive experience, a trophy
 * update, and whatever else die() chooses to do.
 */

void hp_pos_check(struct char_data *ch, struct char_data *attacker, int dam) {
    int newstance = -1, newpos = -1;

    if (DECEASED(ch))
        return;

    /* Give items an ability to heal before death is checked. */
    if (GET_HIT(ch) <= HIT_DEAD)
        death_otrigger(ch);

    if (GET_HIT(ch) <= HIT_DEAD) {
        newpos = POS_PRONE;
        newstance = STANCE_DEAD;
    } else if (GET_HIT(ch) <= HIT_MORTALLYW) {
        newpos = POS_PRONE;
        newstance = STANCE_MORT;
    } else if (GET_HIT(ch) <= HIT_INCAP) {
        newpos = POS_PRONE;
        newstance = STANCE_INCAP;
    } else if (GET_HIT(ch) <= 0) {
        newpos = POS_PRONE;
        newstance = STANCE_STUNNED;
    } else if (GET_STANCE(ch) <= STANCE_STUNNED) {
        /* Char has positive hit points, yet is stunned.
         * Will recover, waking up if possible. */
        if (EFF_FLAGGED(ch, EFF_SLEEP)) {
            newpos = POS_PRONE;
            newstance = STANCE_SLEEPING;
        } else {
            newpos = POS_PRONE;
            newstance = STANCE_RESTING;
        }
    }

    if (newstance != -1 && newpos != -1 && (GET_POS(ch) != newpos || GET_STANCE(ch) != newstance))
        hp_stance_alteration(ch, attacker, newpos, newstance, dam);

    /* Send messages about serious damage */
    if (AWAKE(ch) && dam > 0) {
        if (dam > (GET_MAX_HIT(ch) >> 2))
            act("That really did HURT!", FALSE, ch, 0, 0, TO_CHAR);
        if (GET_HIT(ch) < (GET_MAX_HIT(ch) >> 2)) {
            sprintf(buf2, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n", CLRLV(ch, FRED, C_SPR),
                    CLRLV(ch, ANRM, C_SPR));
            send_to_char(buf2, ch);
        }
    }
}

/***************************************************************************
 * $Log: chars.c,v $
 * Revision 1.63  2010/06/20 19:53:47  mud
 * Log to file errors we might want to see.
 *
 * Revision 1.62  2009/03/22 03:53:53  jps
 * Use correct function to check whether a person's in the right stance
 * and position to continue casting a spell.
 *
 * Revision 1.61  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.60  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.59  2009/03/05 19:00:43  myc
 * Make dwarves unable to be diabolists and condense the
 * class_ok_race array.
 *
 * Revision 1.58  2009/02/04 21:51:41  myc
 * Increase fire vs air, decrease fire vs ice.
 *
 * Revision 1.57  2009/01/19 02:41:41  myc
 * Boots fire damage vs ice.
 *
 * Revision 1.56  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.55  2008/09/14 18:25:35  mud
 * Only check for falling off a mount when the rider is not immort.
 *
 * Revision 1.54  2008/09/14 00:34:34  jps
 * Remove some composition restrictions from imms.
 *
 * Revision 1.53  2008/09/13 18:06:19  jps
 * Removing certain spells from characters when in fluid form.
 *
 * Revision 1.52  2008/09/13 17:21:39  jps
 * Check for falling off a mount when it changes position.
 *
 * Revision 1.51  2008/09/07 01:27:58  jps
 * Check for falling when you change your position in an air room.
 *
 * Revision 1.50  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.49  2008/08/29 04:16:26  myc
 * Added reference to act.h header file.
 *
 * Revision 1.48  2008/08/24 19:29:11  jps
 * Apply damage susceptibility reductions to the various physical attack skills.
 *
 * Revision 1.47  2008/08/20 05:03:13  jps
 * Removed the damage type 'magic'.
 *
 * Revision 1.46  2008/08/19 02:11:14  jps
 * Don't apply fluid/rigidity restrictions to immortals.
 *
 * Revision 1.45  2008/07/21 18:46:13  jps
 * Protect against null deref when no attacker is passed to damage_evasion().
 *
 * Revision 1.44  2008/06/21 17:28:18  jps
 * Made more use of the VALID_CLASS macro.
 *
 * Revision 1.43  2008/06/20 20:42:53  jps
 * Don't run death_mtrigger here - it's called within die().
 *
 * Revision 1.42  2008/06/07 19:06:46  myc
 * Moved all object-related constants and structures to objects.h
 *
 * Revision 1.41  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.40  2008/05/12 01:11:17  jps
 * Split out the hp-pos-changing bit so that fleeing can be
 * made to work again.
 *
 * Revision 1.39  2008/05/12 00:44:38  jps
 * Change susceptibility to discorporate. Magical life force: 120
 * Elemental: 100
 *
 * Revision 1.38  2008/05/11 05:51:35  jps
 * alter_pos() is now the internal way to change a character's position.
 * hp_pos_check() should be called after modifying hp. It will change the
 * position if necessary, for example if the char became unconscious.
 *
 * Revision 1.37  2008/05/10 16:19:50  jps
 * Made EVASIONCLR globally available.
 *
 * Revision 1.36  2008/04/26 23:36:25  myc
 * Added align_color function.
 *
 * Revision 1.35  2008/04/19 18:17:31  jps
 * Give prototyped mobs a height/weight according to the builder's
 * chosen size.
 *
 * Revision 1.34  2008/04/13 18:49:57  jps
 * Fix sign in boolean_attack_evasion.
 *
 * Revision 1.33  2008/04/13 01:41:26  jps
 * Adding composition_check() function.
 *
 * Revision 1.32  2008/04/13 01:29:53  jps
 * If you change form while mounted, you may well fall down.
 * Or if your mount changes form.
 *
 * Revision 1.31  2008/04/10 02:00:42  jps
 * Changed acid's verb to corrode.
 *
 * Revision 1.30  2008/04/10 01:55:34  jps
 * Making metal very susceptible to acid.
 *
 * Revision 1.29  2008/04/06 19:48:52  jps
 * Add an adjective to compositions.
 *
 * Revision 1.28  2008/04/06 04:58:05  jps
 * Use coldshield and fireshield when calculating susceptibility
 * to fire and cold damage.
 *
 * Revision 1.27  2008/04/05 18:17:45  jps
 * More changing of evasion messages.
 *
 * Revision 1.26  2008/04/05 18:06:35  jps
 * Add an action word to damtypes, and use it in damage_evasion_message.
 *
 * Revision 1.25  2008/04/05 17:53:13  jps
 * Add colors to damage types.
 *
 * Revision 1.24  2008/04/05 03:46:10  jps
 * Add a "mass noun" string to composition definitions.
 *
 * Revision 1.23  2008/04/04 21:32:31  jps
 * Change boolean_evasion to incorporate victim level.
 *
 * Revision 1.22  2008/03/29 16:26:39  jps
 * Add an evasion check exclusively for non-damaging attack spells.
 *
 * Revision 1.21  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.20  2008/03/27 17:29:24  jps
 * Ethereal creatures are now attackable when you have the spell
 * effect of bless or hex.
 *
 * Revision 1.19  2008/03/27 00:21:19  jps
 * Add verbs to damage types. Be more verbose with the evasion messages.
 *
 * Revision 1.18  2008/03/26 22:58:11  jps
 * Use the new rigid field for compositions, and make more balancing
 * changes to the composition vulnerabilities.
 *
 * Revision 1.17  2008/03/26 19:09:08  jps
 * Fix bug in damage_evasion.
 *
 * Revision 1.16  2008/03/26 18:15:28  jps
 * damage_evasion() will check whether an ethereal creature is
 * receiving a blessed physical attack, and return false.
 *
 * Revision 1.15  2008/03/25 22:02:32  jps
 * Forgot to rename iron to metal!
 *
 * Revision 1.14  2008/03/25 22:01:00  jps
 * Some resistance based on composition rebalancing.
 *
 * Revision 1.13  2008/03/25 04:49:44  jps
 * Add some functions for seeing what kind of damage people will do
 *
 * Revision 1.12  2008/03/24 08:45:02  jps
 * Adding flag checks to susceptibility().
 * Implemented damage_evasion(), skill_to_dtype(), and
 * damage_evasion_message().  That last one needs more work.
 *
 * Revision 1.11  2008/03/23 19:46:29  jps
 * Added compositions stone and bone.
 *
 * Revision 1.10  2008/03/23 18:42:02  jps
 * New damage defines (old ones in spells.h are now obsolete). Added damage
 * susceptibilities to struct lifedef and struct compdef.
 *
 * Revision 1.9  2008/03/23 00:26:20  jps
 * Add function set_base_composition, which is appropriate for OLC
 * and "set <foo> composition".
 * Add list_olc_compositions() which does just that.
 *
 * Revision 1.8  2008/03/22 20:26:21  jps
 * Add functions to convert life force and composition.
 *
 * Revision 1.7  2008/03/22 19:57:14  jps
 * Added lifeforce and composition definitions.
 *
 * Revision 1.6  2008/03/22 19:09:21  jps
 * Use parse_obj_name() instead of the specific parse_size() function.
 *
 * Revision 1.5  2008/03/18 06:02:32  jps
 * Make minimum height and weight 1.
 *
 * Revision 1.4  2008/03/11 02:56:55  jps
 * Added a lot of size-releated information and functions.
 *
 * Revision 1.3  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.2  2008/01/25 21:05:45  myc
 * Removed the barehand_mult and backstab_mult functions.  This
 * functionality is contained within the hit function now.  Besides,
 * barehand_mult was poorly named, as it isn't multiplied, but rather
 * added.
 *
 * Revision 1.1  2008/01/05 21:53:56  jps
 * Initial revision
 *
 ***************************************************************************/
