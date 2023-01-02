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

#include "act.hpp"
#include "casting.hpp"
#include "charsize.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "damage.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "events.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "lifeforce.hpp"
#include "logging.hpp"
#include "magic.hpp"
#include "math.hpp"
#include "movement.hpp"
#include "races.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "spell_parser.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

const char *stats_display =
    "&0&7&b[s]&0 Strength      &0&7&b[i]&0 Intelligence\n"
    "&0&7&b[w]&0 Wisdom        &0&7&b[c]&0 Constitution\n"
    "&0&7&b[d]&0 Dexterity     &0&7&b[m]&0 Charisma\n\n";

constexpr bool Y = true;
constexpr bool N = false;

int class_ok_race[NUM_RACES][NUM_CLASSES] = {
    /* RACE   So Cl Th Wa Pa An Ra Dr Sh As Me Ne Co Mo Be Pr Di My Ro Ba Py Cr Il Hu */
    /* Hu */ {Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, N},
    /* El */ {Y, Y, Y, Y, Y, N, Y, Y, N, N, N, N, Y, N, N, Y, N, Y, Y, Y, Y, Y, Y, N},
    /* Gn */ {Y, Y, Y, N, N, N, N, Y, Y, N, N, N, Y, N, N, Y, N, N, Y, Y, Y, Y, Y, N},
    /* Dw */ {N, Y, Y, Y, Y, N, N, N, N, N, Y, N, Y, N, Y, Y, N, N, Y, Y, N, N, N, N},
    /* Tr */ {N, N, N, Y, N, N, N, N, Y, N, Y, N, N, N, Y, N, N, N, Y, N, N, N, N, Y},
    /* Dr */ {Y, Y, N, Y, N, Y, N, N, Y, Y, Y, Y, Y, N, N, N, Y, N, Y, N, Y, Y, Y, Y},
    /* Du */ {N, Y, Y, Y, N, Y, N, N, N, Y, Y, N, N, N, Y, N, Y, N, Y, N, N, N, N, Y},
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
    /*DbFi*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, N, Y, N},
    /*DbFr*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, N, Y, Y, N},
    /*DbAc*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, Y, Y, N},
    /*DbLi*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, Y, Y, N},
    /*DbGa*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, Y, Y, N},
    /*svrf*/ {Y, Y, Y, N, N, N, N, N, Y, Y, N, Y, Y, N, N, N, Y, Y, Y, Y, Y, Y, Y, N},
    /*SFae*/ {Y, Y, Y, Y, N, N, Y, Y, N, N, N, N, Y, N, N, N, N, Y, Y, Y, Y, Y, Y, N},
    /*UFae*/ {Y, Y, Y, N, N, N, N, Y, N, N, N, Y, Y, N, N, N, N, Y, Y, Y, Y, Y, Y, N},
    /*nmph*/ {Y, Y, N, Y, N, N, Y, Y, Y, N, N, N, Y, N, N, N, N, Y, Y, Y, Y, Y, Y, N},
};

ACMD(do_flee); /* act.offensive.c */

int get_base_saves(CharData *ch, int type) {
    /* Here are default values for saving throws: */
    int saves[NUM_SAVES] = {105, 115, 105, 110, 110};
    int i;

    if (type < 0 || type >= NUM_SAVES) {
        log("SYSERR: get_base_saves: invalid type {:d}", type);
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
        saves[SAVING_PARA] -= (int)(0.2 * GET_VIEWED_CON(ch));
        saves[SAVING_ROD] -= (int)(0.1 * GET_VIEWED_CON(ch));
        saves[SAVING_SPELL] -= (int)(0.15 * GET_VIEWED_CON(ch));
        break;
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
    case RACE_SVERFNEBLIN:
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

void roll_natural_abils(CharData *ch) {
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
                log("SYSERR: roll_natural_abils(): class '{}' statorder is broken", classes[(int)GET_CLASS(ch)].name);
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

int roll_skill(CharData *ch, int skill) {
    int value = number(50 + 5 * GET_LEVEL(ch), 100 + 15 * GET_LEVEL(ch));
    int max = return_max_skill(ch, skill);
    return MIN(value, max);
}

void rider_flowoff(CharData *rider, CharData *mount) {
    act("You suddenly find yourself flowing down off $N's back.", false, rider, 0, mount, TO_CHAR);
    act("$n loses cohesion and flows down off your back.", false, rider, 0, mount, TO_VICT);
    act("Unable to keep $s seat, $n suddenly flows down from $N's back.", true, rider, 0, mount, TO_NOTVICT);
    dismount_char(rider);
}

void rider_fallthrough(CharData *rider, CharData *mount) {
    act("$N is no longer able to support you, and you fall through $M to the "
        "ground.",
        false, rider, 0, mount, TO_CHAR);
    act("$n falls through your fluid body and ends up on the ground.", false, rider, 0, mount, TO_VICT);
    act("$n suddenly finds $mself falling through $N, and ends up on the ground.", false, rider, 0, mount, TO_NOTVICT);
    dismount_char(rider);
}

void composition_check(CharData *ch) {
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
int susceptibility(CharData *ch, int dtype) {
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
        if (MOB_FLAGGED(ch, MOB_NOPOISON) || MOB_FLAGGED(ch, MOB_ILLUSORY))
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
bool boolean_attack_evasion(CharData *ch, int power, int dtype) {
    return number(1, 100) < MAX(3, 110 + GET_LEVEL(ch) - susceptibility(ch, dtype) - power);
}

ObjData *equipped_weapon(CharData *ch) {
    int weapon_position = -1;

    if (GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON)
        weapon_position = WEAR_WIELD2;
    else if (GET_EQ(ch, WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == ITEM_WEAPON)
        weapon_position = WEAR_WIELD;
    else if (GET_EQ(ch, WEAR_2HWIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_2HWIELD)) == ITEM_WEAPON)
        weapon_position = WEAR_2HWIELD;

    return weapon_position >= 0 ? GET_EQ(ch, weapon_position) : nullptr;
}

int dam_suscept_adjust(CharData *ch, CharData *victim, ObjData *weapon, int dam, int dtype) {
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

void alter_pos(CharData *ch, int newpos, int newstance) {
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
        act("$n ceases casting $s spell.", false, ch, 0, 0, TO_ROOM);
        act("You stop casting your spell.", false, ch, 0, 0, TO_CHAR);
    }

    if (FIGHTING(ch) && GET_STANCE(ch) < STANCE_RESTING)
        stop_battling(ch);
    falling_check(ch);
    mount_pos_check(ch);
}

void hp_stance_alteration(CharData *ch, CharData *attacker, int newpos, int newstance, int dam) {
    alter_pos(ch, newpos, newstance);

    if (GET_STANCE(ch) < STANCE_SLEEPING)
        stop_merciful_attackers(ch);

    /* Sanity check. */
    if (GET_POS(ch) != newpos || GET_STANCE(ch) != newstance) {
        log(LogSeverity::Warn, LVL_GOD,
            "ERR: hp_stance_alteration(): Tried to change to {}/{} but alter_pos() left values at {}/{} (for {})",
            position_types[newpos], stance_types[newstance], position_types[GET_POS(ch)], stance_types[GET_STANCE(ch)],
            GET_NAME(ch));
    }

    /* Send messages and cause dying. */
    switch (GET_STANCE(ch)) {
    case STANCE_MORT:
        act("$n is mortally wounded, and will die soon if not aided.", true, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You are mortally wounded, and will die soon if not aided.\n");
        break;
    case STANCE_INCAP:
        act("$n is incapacitated and will slowly die, if not aided.", true, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You are incapacitated an will slowly die, if not aided.\n");
        break;
    case STANCE_STUNNED:
        act("$n is stunned, but will probably regain consciousness again.", true, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You're stunned, but will probably regain consciousness again.\n");
        break;
    case STANCE_DEAD:
        die(ch, attacker);
        act("$n is dead!  R.I.P.", false, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You are dead!  Sorry...\n");
        break;
    default:
        if (dam < 0) {
            /* Healing */
            if (GET_STANCE(ch) == STANCE_SLEEPING) {
                act("$n appears to be stabilized, but $e remains unconscious.", true, ch, 0, 0, TO_ROOM);
            } else {
                act("$n regains consciousness.", true, ch, 0, 0, TO_ROOM);
                char_printf(ch, "You regain consciousness.\n");
                look_at_room(ch, false);
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

void hp_pos_check(CharData *ch, CharData *attacker, int dam) {
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
            act("That really did HURT!", false, ch, 0, 0, TO_CHAR);
        if (GET_HIT(ch) < (GET_MAX_HIT(ch) >> 2)) {
            char_printf(ch, "{}You wish that your wounds would stop BLEEDING so much!{}\n", CLRLV(ch, FRED, C_SPR),
                        CLRLV(ch, ANRM, C_SPR));
        }
    }
}