/***************************************************************************
 *   File: magic.c                                        Part of FieryMUD *
 *  Usage: low-level functions for magic; spell template code              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "magic.hpp"

#include "casting.hpp"
#include "chars.hpp"
#include "charsize.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "damage.hpp"
#include "db.hpp"
#include "directions.hpp"
#include "events.hpp"
#include "exits.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "lifeforce.hpp"
#include "limits.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "movement.hpp"
#include "races.hpp"
#include "regen.hpp"
#include "skills.hpp"
#include "spells.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <math.h>

int real_mobile(int);

void half_chop(char *string, char *arg1, char *arg2);
int is_abbrev(char *arg1, char *arg2);
bool is_grouped(CharData *ch, CharData *tch);
void add_follower(CharData *ch, CharData *leader);
int dice(int number, int size);
int get_spell_duration(CharData *ch, int spellnum);
int get_vitality_hp_gain(CharData *ch, int spellnum);
char *get_vitality_vict_message(int spellnum);
bool check_armor_spells(CharData *ch, CharData *victim, int spellnum);

CharData *read_mobile(int, int);

/* See whether someone evades a spell entirely, for the following reasons:
 *
 * -- major/minor globe
 * -- elemental immunity
 * -- immortal victim
 */

bool evades_spell(CharData *caster, CharData *vict, int spellnum, int power) {
    int sus;

    /* Non-violent spells don't need to be evaded. */
    if (!SINFO.violent)
        return false;

    /* Dispel magic is a special case */
    if (spellnum == SPELL_DISPEL_MAGIC)
        return false;

    /* Major/minor globe */
    if (EFF_FLAGGED(vict, EFF_MINOR_GLOBE) || EFF_FLAGGED(vict, EFF_MAJOR_GLOBE)) {

        /* Minor globe blocks circle 3 and below.  Major globe blocks 6 and down. */
        if ((EFF_FLAGGED(vict, EFF_MINOR_GLOBE) && SINFO.lowest_level <= CIRCLE_3) ||
            (EFF_FLAGGED(vict, EFF_MAJOR_GLOBE) && SINFO.lowest_level <= CIRCLE_6)) {
            act("&1&bThe shimmering globe around your body flares as the spell flows "
                "around it.&0",
                false, caster, 0, vict, TO_VICT);
            act("&1&bThe shimmering globe around $N&1&b's body flares as your spell "
                "flows around it.&0",
                false, caster, 0, vict, TO_CHAR);
            act("&1&bThe shimmering globe around $N&1&b's body flares as $n&1&b's "
                "spell flows around it.&0",
                false, caster, 0, vict, TO_NOTVICT);
            return true;
        }
    }

    if (skills[spellnum].damage_type == DAM_UNDEFINED)
        return false;

    sus = susceptibility(vict, skills[spellnum].damage_type);

    /* If your susceptibility is Zero, we'll stop this thing right now, so
     * that immunity can block effects. */
    if (sus == 0) {
        if (caster == vict) {
            act("&6$n's&6 spell has no effect on $m.&0", false, caster, 0, vict, TO_NOTVICT);
            act("&6Your spell has no effect on you.&0", false, caster, 0, vict, TO_CHAR);
        } else {
            act("&6$n's&6 spell has no effect on $N.&0", false, caster, 0, vict, TO_NOTVICT);
            act("&6Your spell has no effect on $N!&0", false, caster, 0, vict, TO_CHAR);
            act("&6$n's&6 spell has no effect on you!&0", false, caster, 0, vict, TO_VICT);
        }
        return true;
    }

    /* Are you trying to harm or disable an immortal? */
    if (!IS_NPC(vict) && GET_LEVEL(vict) >= LVL_IMMORT) {
        /* This will cause the "You're trying to silence a god? Ha!" message
         * to be sent */
        if (!skill_message(0, caster, vict, spellnum, false)) {
            /* There's no specific message for this spell - send generic
             * messages instead */
            /* to caster */
            act("$N ignores your feeble spell.", false, caster, 0, vict, TO_CHAR);
            /* to victim */
            act("You ignore $n's feeble spell.", false, caster, 0, vict, TO_VICT);
            /* to room */
            act("$N ignores $n's feeble spell.", false, caster, 0, vict, TO_NOTVICT);
        }
        return true;
    }

    if (skills[spellnum].routines & (MAG_DAMAGE | MAG_MANUAL))
        /* For spells that do actual damage, evasion will be checked during
         * the actual damage() call. */
        return false;

    /* Stuff like word of command would fall into this category:
     *   has a damage type, but doesn't do physical damage */
    if (boolean_attack_evasion(vict, power, skills[spellnum].damage_type)) {
        act("&6$n's&6 spell passes over $N harmlessly.&0", false, caster, 0, vict, TO_NOTVICT);
        act("&6Your spell passes over $N harmlessly!&0", false, caster, 0, vict, TO_CHAR);
        act("&6$n's&6 spell passes over you harmlessly!&0", false, caster, 0, vict, TO_VICT);
        set_fighting(vict, caster, false);
        return true;
    }

    return false;
}

void abort_casting(CharData *ch) {
    if (CASTING(ch)) {
        STOP_CASTING(ch);
        /* Don't say they stop chanting if they've been knocked
         * out or killed - it looks funny. */
        if (!AWAKE(ch))
            return;
        act("You stop chanting abruptly!", false, ch, 0, 0, TO_CHAR);
        act("$n stops chanting abruptly!", false, ch, 0, 0, TO_ROOM);
    }
}

CharData *check_guard(CharData *ch, CharData *victim, int gag_output) {
    if (!ch || !victim)
        return nullptr;
    if (ch->casting.target_status != TARGET_ALL_ROOM && victim->guarded_by &&
        victim->guarded_by->in_room == victim->in_room && CAN_SEE(victim->guarded_by, victim) &&
        GET_SKILL(victim->guarded_by, SKILL_GUARD) && !CHECK_WAIT(victim->guarded_by) &&
        GET_POS(victim->guarded_by) >= POS_STANDING && GET_STANCE(victim->guarded_by) >= STANCE_ALERT &&
        attack_ok(ch, victim->guarded_by, false)) {
        improve_skill(victim->guarded_by, SKILL_GUARD);
        if (GET_ISKILL(victim->guarded_by, SKILL_GUARD) > number(1, 1100)) {
            if (!gag_output) {
                act("$n jumps in front of $N, shielding $M from the assault.", false, victim->guarded_by, 0, victim,
                    TO_NOTVICT);
                act("$n jumps in front of you, shielding you from the assault.", false, victim->guarded_by, 0, victim,
                    TO_VICT);
                act("You jump in front of $N, shielding $M from the assault.", false, victim->guarded_by, 0, victim,
                    TO_CHAR);
            }
            return victim->guarded_by;
        } else if (!gag_output) {
            act("$n tries to intercept the attack on $N, but isn't quick enough.", false, victim->guarded_by, 0, victim,
                TO_NOTVICT);
            act("$n tries to shield you from the attack, but can't move fast enough.", false, victim->guarded_by, 0,
                victim, TO_VICT);
            act("You try to block the attack on $N, but aren't quick enough.", false, victim->guarded_by, 0, victim,
                TO_CHAR);
        }
    }
    return victim;
}

int mag_savingthrow(CharData *ch, int type) {
    int get_base_saves(CharData * ch, int type);
    int save;

    /* negative save numbers is better! */

    /* get the base save */
    save = get_base_saves(ch, type);

    /* apply character's save modifiers */
    switch (type) {
    case SAVING_SPELL:
        save += GET_SAVE(ch, SAVING_SPELL);
        break;
    case SAVING_PARA:
        save += GET_SAVE(ch, SAVING_PARA);
        break;
    case SAVING_BREATH:
        save += GET_SAVE(ch, SAVING_BREATH);
        break;
    case SAVING_ROD:
        save += GET_SAVE(ch, SAVING_ROD);
        break;
    case SAVING_PETRI:
        save += GET_SAVE(ch, SAVING_PETRI);
    }

    /* throwing a 0 is always a failure */
    if (MAX(1, save) < number(0, 99))
        return true;

    return false;
}

/* Decrease modifier is used to decrease the modifiers for
 * stone skin and bone draw by 1
 *  Can be used by any effect really with minor code change
 */
void decrease_modifier(CharData *i, int spell) {
    effect *eff, *tmp;

    for (eff = i->effects; eff; eff = tmp) {
        tmp = eff->next;
        if (eff->type == spell) {
            if (spell == SPELL_BONE_DRAW) {
                act("One of the bones locking you in place shatters under the attack!", false, i, 0, 0, TO_CHAR);
                act("One of the bones locking $n in place shatters under the attack!", false, i, 0, 0, TO_ROOM);
            }
            if (eff->modifier > 1)
                eff->modifier--;
            else
                active_effect_remove(i, eff); /*remove bit */
            return;
        }
    }
}

/* effect_update: called from comm.c (causes spells to wear off) */
void effect_update(void) {
    RoomEffectNode *reff, *next_reff, *temp;
    static effect *eff, *next;
    static CharData *i;

    for (i = character_list; i; i = i->next) {
        for (eff = i->effects; eff; eff = next) {
            next = eff->next;
            if (eff->duration >= 1)
                eff->duration--;
            else if (eff->duration == -1) /* No action */
                eff->duration = -1;       /* GODs only! unlimited */
            else
                active_effect_remove(i, eff);
        }
        /* if the mob was animated and now isn't, kill 'im. */
        if (MOB_FLAGGED(i, MOB_ANIMATED) && !EFF_FLAGGED(i, EFF_ANIMATED)) {
            act("$n freezes and falls twitching to the ground.", false, i, 0, 0, TO_ROOM);
            die(i, nullptr);
        }
        /* if the mob was an illusion and its magic ran out, get rid of it */
        if (MOB_FLAGGED(i, MOB_ILLUSORY) && !EFF_FLAGGED(i, EFF_ANIMATED)) {
            act("$n dissolves into tiny multicolored lights that float away.", true, i, 0, 0, TO_ROOM);
            extract_char(i);
        }
    }
    for (reff = room_effect_list; reff; reff = next_reff) {
        next_reff = reff->next;

        reff->timer--;

        if (reff->timer <= 0) {
            /* this room effect has expired */
            if (ROOM_EFF_FLAGGED(reff->room, reff->effect) && skills[reff->spell].wearoff) {
                send_to_room(skills[reff->spell].wearoff, reff->room);
                send_to_room("\r\n", reff->room);
            }

            /* remove the effect */
            if (ROOM_EFF_FLAGGED(reff->room, ROOM_EFF_DARKNESS))
                world[(int)reff->room].light++;
            if (ROOM_EFF_FLAGGED(reff->room, ROOM_EFF_ILLUMINATION))
                world[(int)reff->room].light--;
            REMOVE_FLAG(world[(int)reff->room].room_effects, reff->effect);
            REMOVE_FROM_LIST(reff, room_effect_list, next);
            free(reff);
        }
    }
}

void remove_char_spell(CharData *ch, int spellnum) {
    static effect *eff, *next;

    for (eff = ch->effects; eff; eff = next) {
        next = eff->next;
        if (eff->type == spellnum)
            active_effect_remove(ch, eff);
    }
}

/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int mag_material(CharData *ch, int item0, int item1, int item2, int extract, int verbose) {
    ObjData *tobj;
    ObjData *obj0 = nullptr, *obj1 = nullptr, *obj2 = nullptr;

    for (tobj = ch->carrying; tobj; tobj = tobj->next_content) {
        if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0)) {
            obj0 = tobj;
            item0 = -1;
        } else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1)) {
            obj1 = tobj;
            item1 = -1;
        } else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2)) {
            obj2 = tobj;
            item2 = -1;
        }
    }
    if ((item0 > 0) || (item1 > 0) || (item2 > 0)) {
        if (verbose) {
            switch (number(0, 2)) {
            case 0:
                send_to_char("A wart sprouts on your nose.\r\n", ch);
                break;
            case 1:
                send_to_char("Your hair falls out in clumps.\r\n", ch);
                break;
            case 2:
                send_to_char("A huge corn develops on your big toe.\r\n", ch);
                break;
            }
        }
        return (false);
    }
    if (extract) {
        if (item0 < 0) {
            obj_from_char(obj0);
            extract_obj(obj0);
        }
        if (item1 < 0) {
            obj_from_char(obj1);
            extract_obj(obj1);
        }
        if (item2 < 0) {
            obj_from_char(obj2);
            extract_obj(obj2);
        }
    }
    if (verbose) {
        send_to_char("A puff of smoke rises from your pack.\r\n", ch);
        act("A puff of smoke rises from $n's pack.", true, ch, nullptr, nullptr, TO_ROOM);
    }
    return (true);
}

/* A standardized calculation for single-target sorcerer spells.
 *
 * This is calibrated such that sorcerer damage will be 120-110%
 * as much as warrior damage at any given level.
 * It also depends on the casting times of these spells
 * being specific values. */
int sorcerer_single_target(CharData *ch, int spell, int power) {
    int circle, minlevel;
    double exponent;

    if (!get_spell_assignment_circle(ch, spell, &circle, &minlevel)) {
        log("SYSERR: Cannot get circle/level of spell %d for %s", spell, GET_NAME(ch));
        return 1;
    }

    /*
       This makes the exponent 1.3 when a spell is first introduced, so its power
       rises along with levels pretty steadily.

       As the level gets farther and farther from the spell, however, the exponent
       shrinks, eventually getting down to 1.1.

       The results of this calculation are:

       * A newly-introduced spell will gain significant power as the caster's
       power (proficiency in that skill) increases.
       * Spells will always increase in power.  Even when you go from level 90
       to level 91, your power in 1st level "burning hands" will increase.
       * Low-level spells increase only minutely at higher levels. So you
       don't end up with "burning hands" doing 400 damage.

     */
    exponent = 1.2 + 0.3 * minlevel / 100.0 + (power - minlevel) * (0.004 * minlevel - 0.2) / 100.0;

    log("sorcerer_single_target: exponent =%0.2f power=%d minlevel=%d\n", exponent, power, minlevel);

    switch (circle) {
    case 1:
        return dice(4, 19) + pow(power, exponent);
    case 2:
        return dice(5, 16) + pow(power, exponent);
    case 3:
        return dice(4, 24) + pow(power, exponent);
    case 4:
        return dice(6, 20) + pow(power, exponent);
    case 5:
        return dice(8, 25) + pow(power, exponent);
    case 6:
        return dice(10, 24) + pow(power, exponent);
    case 7:
        return dice(15, 17) + pow(power, exponent);
    case 8:
        return dice(15, 18) + pow(power, exponent);
    default:
        /* Circle 9 */
        return dice(10, 35) + pow(power, exponent);
    }
}

/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * Return value: CAST_RESULT_ flags
 */

int mag_damage(int skill, CharData *ch, CharData *victim, int spellnum, int savetype) {
    EVENTFUNC(battle_paralysis_handler);
    int dam = 0;
    int temp = 0;
    double dmod;
    int reduction = false;
    int sus;
    int damage_spellnum = spellnum;

    if (victim == nullptr || ch == nullptr)
        return 0;

    sus = susceptibility(victim, skills[spellnum].damage_type);

    /* spell damage is now online, and is stored in array "spell_dam_info",
     *******The defines are as such*******:
     *  #define SD_SPELL(i) spell_dam_info[i].spell
     *  #define SD_INTERN_DAM(i) spell_dam_info[i].intern_dam
     *  #define SD_NPC_NO_DICE(i) spell_dam_info[i].npc_no_dice
     *  #define SD_NPC_NO_FACE(i) spell_dam_info[i].npc_no_face
     *  #define SD_PC_NO_DICE(i) spell_dam_info[i].pc_no_dice
     *  #define SD_PC_NO_FACE(i) spell_dam_info[i].pc_no_face
     *  #define SD_NPC_REDUCE_FACTOR(i) spell_dam_info[i].npc_reduce_factor
     *  #define SD_USE_BONUS(i) spell_dam_info[i].use_bonus
     *  #define SD_BONUS(i) spell_dam_info[i].max_bonus
     *  #define SD_NPC_STATIC...
     *  #define SD_PC_STATIC...
     *
     ****General****
     *  - If SD_INTERN_DAM(i) = false then it will look for a internal switch
     *  - Otherwise it will go through a loop using the values in the array
     *spell_dam_info
     *  - If you wish to have Differeng class Affects simply add a class and put
     *in its factor *results** pc vs pc - SD_PC_NO_DICE * SD_PC_NO_FACE + (if
     *using bonus) MIN(SD_USE_BONUS, (int)lvl/4) pc vs npc - SD_NPC_NO_DICE *
     *SD_NPC_NO_FACE + (if using bonus) MIN(SD_USE_BONUS, (int)lvl/2) npc vs npc -
     *SD_NPC_NO_DICE * SD_NPC_NO_FACE + (if using bonus) MIN(SD_USE_BONUS,
     *(int)lvl/2) npc vs pc - SD_NPC_NO_DICE * SD_NPC_NO_FACE + (if using bonus)
     *MIN(SD_USE_BONUS, (int)lvl/2) all npc vs x is also npc *
     *SD_NPC_REDUCE_FACTOR Add Statics onto value Proky
     */

    /* mag_damage spells have their own messages associated with the completion of
       the spell in the messages file in lib
     */

    if (SD_INTERN_DAM(spellnum)) {
        if ((!IS_NPC(ch) && !IS_NPC(victim)) || EFF_FLAGGED(ch, EFF_CHARM)) {
            /* PC vs PC */
            dam = dice(SD_PC_NO_DICE(spellnum), SD_PC_NO_FACE(spellnum)) + SD_PC_STATIC(spellnum);

            if (SD_USE_BONUS(spellnum))
                dam += MIN(SD_BONUS(spellnum), skill / 4);
            else
                dam += MIN(SD_BONUS(spellnum), skill * SD_LVL_MULT(spellnum));
        } else {
            /* Not PC vs PC */
            dam = dice(SD_NPC_NO_DICE(spellnum), SD_NPC_NO_FACE(spellnum)) + SD_NPC_STATIC(spellnum);

            if (SD_USE_BONUS(spellnum))
                dam += MIN(SD_BONUS(spellnum), skill / 2);
            else
                dam += MIN(SD_BONUS(spellnum), skill * SD_LVL_MULT(spellnum));

            if (IS_NPC(ch))
                /* Reduce NPC damage */
                dam = (SD_NPC_REDUCE_FACTOR(spellnum) * dam) / 100;
        }
    }

    /* Spells can pull dam from the online damage table and manipulate it to
       include the spell proficiency system.  Most damage spells will increase
       parabolicly as skill increases so that the base damage will seem to be the
       spells norm thoughout much of the skill range. If the caster eventually
       crosses a certain threshold of the skill then the damage seems to increase
       exponentially.  The algorythms are manipulated in such a way that at a 1000
       skill the spell never does more than (Y) amount of damage. RSD 3/28/00 */

    switch (spellnum) {
        /* Single-target sorcerer spells */

        /* Note that these single-target sorcerer spells are well-balanced.
         * They are calibrated to warrior damage - see sorcerer_single_target().
         * Their damage must also be reduced for lower-level mobs.
         * The low-level missile spells are also balanced, even though they
         * don't use sorcerer_single target (magic missile, fire darts, ice darts).
         *
         * The remainder of the spells here need to be analyzed. */
    case SPELL_DECAY:
    case SPELL_BURNING_HANDS:
    case SPELL_DETONATION:
    case SPELL_CHILL_TOUCH:
    case SPELL_SHOCKING_GRASP:
    case SPELL_LIGHTNING_BOLT:
    case SPELL_NIGHTMARE:
    case SPELL_CONE_OF_COLD:
    case SPELL_POSITIVE_FIELD:
    case SPELL_FIREBALL:
    case SPELL_VICIOUS_MOCKERY:
    case SPELL_ANCESTRAL_VENGEANCE:
    case SPELL_BIGBYS_CLENCHED_FIST:
    case SPELL_SPIRIT_RAY:
    case SPELL_BALEFUL_POLYMORPH:
    case SPELL_IRON_MAIDEN:
    case SPELL_FREEZE:
    case SPELL_ACID_BURST:
    case SPELL_DISINTEGRATE:
    case SPELL_ICEBALL:
        dam = sorcerer_single_target(ch, spellnum, skill);
        reduction = true;
        break; /* <-- End Sorcerer Single Target Switch */

    /* dart and missle spells */
    case SPELL_MAGIC_MISSILE:
    case SPELL_SPIRIT_ARROWS:
    case SPELL_ICE_DARTS:
        dam = dice(4, 21);
        reduction = true;
        break;
    case SPELL_FIRE_DARTS: /* <-- unique because min circle is 2, not 1 */
        dam = dice(5, 18);
        reduction = true;
        break; /* <-- end dart and missle spells */

    /* Cause light etc. series */
    case SPELL_CAUSE_LIGHT:
        /* max dam 50 from max 4d4+2 online */
        dam += (pow(skill, 2) * 2) / 625;
        break;
    case SPELL_CAUSE_SERIOUS:
        /* max dam 80 from max 6d5+5 online */
        dam += (pow(skill, 2) * 9) / 2000;
        break;
    case SPELL_CAUSE_CRITIC:
        /* max dam 110 from max 8d6+9 online */
        dam += (pow(skill, 2) * 53) / 10000;
        break;
    case SPELL_HARM:
        /* max dam 170 from max 9d8+16 online */
        dam += (pow(skill, 2) * 41) / 5000;
        break;
    case SPELL_FULL_HARM:
        /* max dam 400 from max 18d10+20 online */
        dam += (pow(skill, 2) * 1) / 50;
        break; /* <-- end Cause series */

    /* breath spells */
    case SPELL_LIGHTNING_BREATH:
    case SPELL_FIRE_BREATH:
    case SPELL_FROST_BREATH:
    case SPELL_GAS_BREATH:
    case SPELL_ACID_BREATH:
        dam = skill + number(1, skill * 2);
        break;
    case SPELL_ACID_FOG:
        /* spell hits 4 times */
        dam += (pow(skill, 2) * 7) / 1250;
        reduction = true;
        break;
    case SPELL_CALL_LIGHTNING:
        /* There needs to be some code referencing weather to make this spell
           only work when the weather is bad.  If this happens when we can jack
           the damage of the spell since it will be significantly more rare when
           it can be cast. RSD 4/4/00 */
        /* max dam 300 from 15d5+50 online */
        dam += (pow(skill, 2) * 7) / 400;
        break;
    case SPELL_CHAIN_LIGHTNING:
        /* max dam 226 from 12d5+26 online */
        dam += (pow(skill, 2) * 7) / 500;
        break;
    case SPELL_CIRCLE_OF_DEATH:
        /* max dam 226 from 12d5+26 online */
        dam += (pow(skill, 2) * 7) / 500;
    case SPELL_CIRCLE_OF_FIRE:
        dam = (skill / 2) + dice(2, 3);
        break;
    case SPELL_CLOUD_OF_DAGGERS:
        /* spell hits 4 times */
        dam += (pow(skill, 2) * 7) / 1250;
        reduction = true;
        break;
    case SPELL_COLOR_SPRAY:
        /* max dam 190 from 15d5+45 online */
        dam += (pow(skill, 2) * 1) / 200;
        break;
    case SPELL_DEGENERATION:
        dam += dice(3, 40) + 300;
        break;
    case SPELL_DESTROY_UNDEAD:
        if (GET_LIFEFORCE(victim) != LIFE_UNDEAD) {
            act("$N has far too much life for you to harm it!", false, ch, 0, victim, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }
        /* max dam 400 from 18d5+40 online */
        dam += (pow(skill, 2) * 27) / 1000;
        if (GET_CLASS(ch) == CLASS_PRIEST)
            dam *= 1.25;
        break;
    case SPELL_DISCORPORATE:
        if (MOB_FLAGGED(victim, MOB_ILLUSORY))
            /* Discorporate just annihilates illusory mobs */
            dam = 100 + GET_HIT(victim);
        else
            dam = sorcerer_single_target(ch, spellnum, skill) / 5;
        break;
    case SPELL_DISPEL_EVIL:
        if (IS_GOOD(victim)) {
            act("The gods protect $N.", false, ch, 0, victim, TO_CHAR);
            act("You leer at $n as $e attempts to dispel your evilness.", false, ch, 0, victim, TO_VICT);
            act("$n tries to make the evil in Saint $N suffer.", true, victim, 0, ch, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        } else if (IS_NEUTRAL(victim)) {
            act("Yeah, right there fancy pants.  $U$N doesn't seem to care.", false, ch, 0, victim, TO_CHAR);
            act("You don't seem to care that $N is attempting to dispel your "
                "evilness.",
                false, ch, 0, victim, TO_VICT);
            act("$N doesn't care that $n is trying to make $S evilness suffer.", true, victim, 0, ch, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }
        /* max dam 135 from max 10d5+15 online */
        dam += (pow(skill, 2) * 7) / 1000;
        if (GET_CLASS(ch) == CLASS_PRIEST) {
            if (GET_ALIGNMENT(ch) >= 750)
                dam *= 1.25;
            else
                dam *= 1.15;
        } else if (GET_ALIGNMENT(ch) >= 750)
            dam *= 1.1;
        break;
    case SPELL_DISPEL_GOOD:
        if (IS_EVIL(victim)) {
            act("The gods protect $N.", false, ch, 0, victim, TO_CHAR);
            act("You leer at $n as $e attempts to dispel your goodness.", false, ch, 0, victim, TO_VICT);
            act("$N leers at $n as $e tries to rot $S goodness.", true, victim, 0, ch, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        } else if (IS_NEUTRAL(victim)) {
            act("Yeah, right there fancy pants.  $U$N doesn't seem to care.", false, ch, 0, victim, TO_CHAR);
            act("You don't seem to care that $N is attempting to dispel your "
                "goodness.",
                false, ch, 0, victim, TO_VICT);
            act("$N doesn't care that $n is trying to make $S goodness suffer.", true, victim, 0, ch, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }
        /* max dam 135 from max 10d5+15 online */
        dam += (pow(skill, 2) * 7) / 1000;
        if (GET_CLASS(ch) == CLASS_DIABOLIST) {
            if (GET_ALIGNMENT(ch) <= -750)
                dam *= 1.25;
            else
                dam *= 1.15;
        } else if (GET_ALIGNMENT(ch) <= -750)
            dam *= 1.1;
        break;
    case SPELL_DISPEL_MAGIC:
        /* We don't want to hear about folks being immune to dispel magic,
         * because the spell might have been cast to remove spells from them.
         * So get out of here if the victim is immune. */
        if (sus == 0)
            return 0;
        dam = sorcerer_single_target(ch, spellnum, skill);
        reduction = true;
        break;
    case SPELL_DIVINE_BOLT:
        /* max dam 110 from max 8d6+9 online, except alignment bonus! */
        dam += (pow(skill, 2) * 53) / 10000;
        dam *= GET_ALIGNMENT(victim) * -0.0007 + 0.8;
        break;
    case SPELL_DIVINE_RAY:
        /* This spell appears to be the goodie version of stygian eruption */
        /* max dam 176 from 12d5+36 online */
        dam += (pow(skill, 2) * 1) / 125;
        dam *= (GET_ALIGNMENT(ch) * 0.0022) - 0.7;
        break;
    case SPELL_EARTHQUAKE:
        if (!QUAKABLE(CH_NROOM(victim)))
            return CAST_RESULT_CHARGE;
        /* max dam 125 from max 8d7+12 online */
        dam += (pow(skill, 2) * 13) / 2500;
        dam = sectors[CH_SECT(victim)].qdam_mod * dam / 100;
        temp = sectors[CH_SECT(victim)].fall_mod; /* Modifier for likelihood to be knocked down */
        if (GET_POS(victim) == POS_FLYING) {
            act("$N doesn't care that you are making the ground shake.", true, ch, 0, victim, TO_CHAR);
            act("You don't seem to care that $n is attempting to knock you to the "
                "ground.",
                true, ch, 0, victim, TO_VICT);
            act("$N doesn't seem to care that $n is shaking the ground.", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }
        /* Do you fall down?  Levitate and high dex prevent it */
        if (!EFF_FLAGGED(victim, EFF_LEVITATE) && number(1, 100) >= GET_DEX(victim) - temp) {
            GET_POS(victim) = POS_SITTING;
            GET_STANCE(victim) = STANCE_ALERT;
        } else {
            act("$N doesn't care that you are making the ground shake.", true, ch, 0, victim, TO_CHAR);
            act("$n is unable to knock you to the ground.", true, ch, 0, victim, TO_VICT);
            act("$N manages to keep $S footing.", true, ch, 0, victim, TO_NOTVICT);
        }
        /* Levitate - cuts damage in half */
        if (EFF_FLAGGED(victim, EFF_LEVITATE))
            dam /= 2;
        break;
    case SPELL_EXORCISM:
        if (GET_RACE(victim) != RACE_DEMON) {
            send_to_char("That spell only has effect on demonic creatures.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        if (GET_ALIGNMENT(ch) >= 990 && skill - GET_LEVEL(victim) > 30 && !mag_savingthrow(victim, SAVING_SPELL) &&
            number(1, 100) > 50) {
            act("$N &7&blets out a massive &1howl&7 as $E is banished by $n's&7&b "
                "command.&0",
                false, ch, 0, victim, TO_ROOM);
            act("$N &7&blets out a massive &1howl&7 as $E is banished by your holy "
                "might.&0",
                false, ch, 0, victim, TO_CHAR);
            if (!MOB_FLAGGED(ch, MOB_ILLUSORY)) { /* illusions don't really banish */
                event_create(EVENT_EXTRACT, extract_event, victim, false, &(victim->events), 0);
                GET_HIT(victim) = -50;
                GET_POS(victim) = POS_PRONE;
                GET_STANCE(victim) = STANCE_DEAD;
            }
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }
        if (GET_ALIGNMENT(ch) <= 400) {
            send_to_char("You are not holy enough to cast that spell!\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        /* This spell is meant to cause great concern to demonic forces so it'll be
         * quite stout */
        /* max dam 1200 from 50d10+50 online */
        dam = (pow(skill, 2) * 13) / 20;
        dam *= (GET_ALIGNMENT(ch) * 0.001) + 0.2;
        /* thought it was to good? Muahahaha!
           Now you lose some HP and MV for selling your soul to hurt the demons! */
        GET_HIT(ch) -= dam / 50;
        if (GET_MOVE(ch) > 45)
            GET_MOVE(ch) = MAX(45, GET_MOVE(ch) - (dam / 10));
        act("You lose some life in libation of your holy allegiance!", false, ch, 0, victim, TO_CHAR);
        act("$n looks slightly diminished.", false, ch, 0, victim, TO_ROOM);
        break;
    case SPELL_FIRESTORM:
        /* Mirror spell of ice storm */
        /* max dam 235 from 20d5+35 online */
        dam += (pow(skill, 2) * 1) / 100;
        if (GET_CLASS(ch) == CLASS_PYROMANCER)
            dam *= 1.25;
        break;
    case SPELL_FLAMESTRIKE:
        /* max dam 80 for neutral from max 8d9+8 online */
        if (IS_EVIL(ch)) {
            send_to_char("You are not holy enough to cast that spell!\r\n", ch);
            return 0;
        }
        dam *= (GET_ALIGNMENT(ch) * 0.0007) + 0.8;
        break;
    case SPELL_FLOOD:
        /* max dam 1200 from 50d10+50 online */
        dam += (pow(skill, 2) * 13) / 200;
        break;
    case SPELL_FRACTURE:
        dam = sorcerer_single_target(ch, spellnum, skill) / 3;
        if (EFF_FLAGGED(ch, EFF_CHARM) && ch->master && IS_PC(ch->master))
            dam *= 2;
        break;
    case SPELL_FRACTURE_SHRAPNEL:
        dam = sorcerer_single_target(ch, SPELL_FRACTURE, skill) / 2;
        if (EFF_FLAGGED(ch, EFF_CHARM) && ch->master && IS_PC(ch->master))
            dam *= 2;
        break;
    case SPELL_FREEZING_WIND:
        /* As Freezing wind is an area effect spell it's initial base damage will be
           low and slow to increase like the other, however it's top will be low due
           to the existance of other ice based area effect spells. This spell will
           need some affect associated with it so it will be unique and worth
           casting instead of the other more poweful spells. RSD 4/2/00
           max dam 145 from 15d5+5 online */
        dam += (pow(skill, 2) * 3) / 500;
        break;
    case SPELL_HELL_BOLT:
        /* max dam 110 from max 8d6+9 online, except alignment bonus! */
        dam += (pow(skill, 2) * 53) / 10000;
        dam *= GET_ALIGNMENT(victim) * 0.0007 + 0.8;
        break;
    case SPELL_HELLFIRE_BRIMSTONE:
        /* looks like area version of stygian eruption */
        if (GET_ALIGNMENT(ch) >= -400) {
            send_to_char("You are not unholy enough to cast that spell!\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        /* max dam 300 from 30d5+25 online */
        dam += (pow(skill, 2) * 1) / 180;
        dam *= (GET_ALIGNMENT(ch) * -0.0022) - 0.7;
        break;
    case SPELL_HOLY_WORD:
        /* max dam 300 from 20d5+35 online */
        dam += (pow(skill, 2) * 33) / 2000;
        if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_PALADIN)
            dam *= 1.25;
        break;
    case SPELL_ICE_SHARDS:
        /* max dam 2500 from 100d10+500 online */
        dam += (pow(skill, 2) * 1) / 10;
        break;
    case SPELL_ICE_STORM:
        /* max dam 235 from 20d5+35 online */
        dam += (pow(skill, 2) * 1) / 100;
        if (GET_CLASS(ch) == CLASS_CRYOMANCER)
            dam *= 1.25;
        break;
    case SPELL_IMMOLATE:
        /* Immolate hits 5 times. */
        dam = sorcerer_single_target(ch, spellnum, skill) / 4;
        reduction = true;
        break;
    case SPELL_LESSER_EXORCISM:
        if (GET_RACE(victim) != RACE_DEMON) {
            send_to_char("That spell only has effect on demonic creatures.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        if (GET_ALIGNMENT(ch) >= 990 && skill - GET_LEVEL(victim) > 30 && !mag_savingthrow(victim, SAVING_SPELL) &&
            number(1, 100) > 50) {
            act("$N &7&blets out a massive howl as $E is banished by your holy "
                "might.&0",
                false, ch, 0, victim, TO_CHAR);
            act("$N &7&blets out a massive howl as $E is banished by $n's&7&b "
                "command.&0",
                false, ch, 0, victim, TO_ROOM);
            if (!MOB_FLAGGED(ch, MOB_ILLUSORY)) { /* illusions don't really banish */
                event_create(EVENT_EXTRACT, extract_event, victim, false, &(victim->events), 0);
                GET_HIT(victim) = -50;
                GET_POS(victim) = POS_PRONE;
                GET_STANCE(victim) = STANCE_DEAD;
            }
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }
        if (GET_ALIGNMENT(ch) <= 400) {
            send_to_char("You are not holy enough to cast that spell!\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        /* This spell is meant to cause great concern to demonic forces so it'll be
         * quite stout */
        /* max dam 700 from 50d5+50 online */
        dam += (pow(skill, 2) * 1) / 25;
        dam *= (GET_ALIGNMENT(ch) * 0.001) + 0.2;
        /* thought it was to good? Muahahaha!
           Now you lose some HP and MV for selling your soul to hurt the demons! */
        GET_HIT(ch) -= dam / 15;
        if (GET_MOVE(ch) > 25)
            GET_MOVE(ch) = MAX(20, GET_MOVE(ch) - (dam / 50));
        act("$n looks slightly diminished.", false, ch, 0, 0, TO_ROOM);
        act("You lose some life in libation of your holy allegiance!", false, ch, 0, 0, TO_CHAR);
        break;
    case SPELL_MELT:
        /* Increased damage against metal, stone, and ice */
        dam = sorcerer_single_target(ch, spellnum, skill);
        switch (GET_COMPOSITION(ch)) {
        case COMP_METAL:
        case COMP_STONE:
        case COMP_ICE:
            dam *= 2;
            break;
        }
        reduction = true;
        break;
    case SPELL_METEORSWARM:
        /* max dam 1200 from 50d10+50 online */
        dam += (pow(skill, 2) * 13) / 200;
        break;
    case SPELL_MOONBEAM:
        dam += skill * 2 + number(20, 80);
        break;
    case SPELL_PHOSPHORIC_EMBERS:
        /* hits 4 times. */
        dam = sorcerer_single_target(ch, spellnum, skill) / 3;
        reduction = true;
        break;
    case SPELL_PYRE:
        /* hits 4 times, but charmies do half damage */
        dam = sorcerer_single_target(ch, spellnum, skill) / 3;
        if (EFF_FLAGGED(ch, EFF_CHARM) && ch->master && IS_PC(ch->master))
            dam *= 2;
        reduction = true;
        break;
    case SPELL_PYRE_RECOIL:
        dam = sorcerer_single_target(ch, SPELL_PYRE, skill) / 6;
        /* use the SPELL_ON_FIRE damage message */
        damage_spellnum = SPELL_ON_FIRE;
        reduction = true;
        break;
    case SPELL_SEVERANCE:
        dam += (pow(skill, 2) * 13) / 200;
        break;
    case SPELL_SOUL_REAVER:
        dam += (pow(skill, 2) * 13) / 199;
        break;
    case SPELL_STYGIAN_ERUPTION:
        if (GET_ALIGNMENT(ch) >= -400) {
            send_to_char("You are not unholy enough to cast that spell!\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        /* max dam 176 from 12d5+36 online */
        dam += (pow(skill, 2) * 1) / 125;
        dam *= (GET_ALIGNMENT(ch) * -0.0022) - 0.7;
        break;
    case SPELL_SUNRAY:
        dam += (pow(skill, 2) * 7) / 400;
        break;
    case SPELL_SUPERNOVA:
        /* max dam 2500 from 100d10+500 online */
        dam += (pow(skill, 2) * 1) / 10;
        break;
    case SPELL_UNHOLY_WORD:
        /* max dam 300 from 20d5+35 online */
        dam += (pow(skill, 2) * 33) / 2000;
        if (GET_CLASS(ch) == CLASS_DIABOLIST || GET_CLASS(ch) == CLASS_ANTI_PALADIN)
            dam *= 1.25;
        break;
    case SPELL_VAMPIRIC_BREATH:
        dam += dice(2, skill + 10);
        if (skill >= 95)
            dam += number(0, 70);
        GET_HIT(ch) += dam;
        break;
    case SPELL_WRITHING_WEEDS:
        /* max dam 110 from max 8d6+9 online */
        dam += (pow(skill, 2) * 53) / 10000;
        break;

    case SKILL_ELECTRIFY:
        dam = skill - number(0, 3);
        break;

    } /* <--- end mag_damage switch */

    /* For the balanced sorcerer spells, reduce damage from low-level NPCs. */
    if (reduction && IS_NPC(ch)) {
        dmod = 0.3 + pow((skill / 100.0), 2) * 0.7, 1;
        if (dmod < 0.3)
            dmod = 0.3;
        if (dmod > 1)
            dmod = 1;
        dam *= dmod;
        if (dam < 1)
            dam = 1;
    }

    victim = check_guard(ch, victim, false);

    if (!attack_ok(ch, victim, true))
        return CAST_RESULT_CHARGE;

    /* divide damage by two if victim makes his saving throw */
    if (mag_savingthrow(victim, savetype))
        dam >>= 1;

    /* if HARNESS spell is active, do extra 1% per lvl of foe */
    if (EFF_FLAGGED(ch, EFF_HARNESS)) {
        dam += (dam * GET_LEVEL(victim)) / 100;
        act("&5&b$n&5&b executes $s spell with amazing force...&0", false, ch, 0, 0, TO_ROOM);
        act("&5&bYou execute your spell with amazing force...&0", false, ch, 0, 0, TO_CHAR);
        effect_from_char(ch, SPELL_HARNESS);
    }

    /* Adjust the damage according to the susceptibility */
    dam = dam * sus / 100;

    if (sus > 119) {
        /* Cry out if you're highly vulnerable */
        if (number(1, 4) == 1)
            act("$n cries out in pain!", true, victim, 0, 0, TO_ROOM);
    } else if (sus > 104) {
        /* Express pain if you're very vulnerable */
        if (number(1, 4) == 1)
            act("$n cringes with a pained look on $s face.", true, victim, 0, 0, TO_ROOM);
    }

    /* and finally, inflict the damage */
    damage(ch, victim, dam, damage_spellnum);

    if (ALIVE(victim) && EFF_FLAGGED(victim, EFF_IMMOBILIZED) &&
        IS_SET(skills[spellnum].targets, TAR_DIRECT | TAR_CONTACT))
        decrease_modifier(victim, SPELL_BONE_DRAW);

    if (ALIVE(victim) && dam > 0 && GET_LEVEL(victim) < LVL_IMMORT && !MOB_FLAGGED(ch, MOB_ILLUSORY)) {

        /* Secondary effects: catch on fire, or freeze up. */

        /* If not protected from fire, chance to catch on fire. */
        if (skills[spellnum].damage_type == DAM_FIRE && susceptibility(victim, DAM_FIRE) > 60 &&
            !EFF_FLAGGED(victim, EFF_ON_FIRE)) {
            temp = MAX(1, MIN(90, (3 + skill - GET_LEVEL(victim)) * susceptibility(victim, DAM_FIRE) / 100));
            if (temp > number(0, 100)) {
                SET_FLAG(EFF_FLAGS(victim), EFF_ON_FIRE);
                switch (number(1, 3)) {
                case 1:
                    act("&1&8$n bursts into flame!&0", false, victim, 0, 0, TO_ROOM);
                    send_to_char("&1&8Your skin and clothes ignite into flame!&0\r\n", victim);
                    break;
                case 2:
                    sprintf(buf, "%s light%s", skills[spellnum].name,
                            skills[spellnum].name[strlen(skills[spellnum].name) - 1] == 's' ? "" : "s");
                    act("&1&8$n's $t you on fire!&0", false, ch, buf, victim, TO_VICT);
                    act("&1&8$n's $t $N on fire!&0", false, ch, buf, victim, TO_NOTVICT | TO_VICTROOM);
                    act("&1&8Your $t $N on fire!&0", false, ch, buf, victim, TO_CHAR);
                    break;
                case 3:
                    send_to_char("&1&8Flames spread across your body!&0\r\n", victim);
                    act("&1&8Flames envelope $n!&0", false, victim, 0, 0, TO_ROOM);
                    break;
                }
            }
        }
        /* If not protected from cold, chance to freeze up. */
        if (skills[spellnum].damage_type == DAM_COLD && !EFF_FLAGGED(victim, EFF_MINOR_PARALYSIS)) {
            /* The following calculation gives you about a 20/500 chance
             * of freezing a level 100 mob with your best cold spell. */
            temp = skill + skills[spellnum].min_level[(int)GET_CLASS(ch)];
            temp = temp * susceptibility(victim, DAM_COLD) / 100;
            temp = temp - 2 * GET_LEVEL(victim) + 20;
            if (temp > number(0, 500)) {
                WAIT_STATE(victim, MAX(3, PULSE_VIOLENCE * (skill - GET_LEVEL(victim)) / 20));
                act("&4&b$n&4&b freezes up!&0", true, victim, 0, 0, TO_ROOM);
                send_to_char("&4&bYour joints stiffen as the frost penetrates you!&0\r\n", victim);
                STOP_CASTING(victim);
                event_create(EVENT_BATTLE_PARALYSIS, battle_paralysis_handler, mkgenericevent(ch, victim, 0), true,
                             &(victim->events), 0);
            }
        }
    }

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
} /* end mag_damage */

/*
 * Every spell that does an effect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * effect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod, refresh)
 *
 * I added refresh so that spells can be refreshed before they run out.
 * NOTE: If accum_duration is set to true, it will override refresh.
 *
 * Return value: CAST_RESULT_ flags.
 */

#define MAX_SPELL_EFFECTS 9 /* change if more needed */

int mag_affect(int skill, CharData *ch, CharData *victim, int spellnum, int savetype, int casttype) {
    effect eff[MAX_SPELL_EFFECTS];
    effect *effect = nullptr;
    bool accum_effect = false, accum_duration = false, is_innate = false, refresh = true;
    const char *to_vict = nullptr, *to_room = nullptr, *to_char = nullptr;
    int i;

    if (victim == nullptr || ch == nullptr)
        return 0;
    if (MOB_FLAGGED(ch, MOB_ILLUSORY) && ch != victim)
        return 0;
    if (!check_fluid_spell_ok(ch, victim, spellnum, false))
        return CAST_RESULT_CHARGE;
    if (ch->casting.misc && *ch->casting.misc)
        half_chop(ch->casting.misc, buf, buf2);

    memset(eff, 0, sizeof(eff));
    for (i = 0; i < MAX_SPELL_EFFECTS; i++)
        eff[i].type = spellnum;

    if (GET_LEVEL(victim) >= LVL_IMMORT && GET_LEVEL(ch) < GET_LEVEL(victim)) {
        act("Your spell is too weak to affect $N.", false, ch, 0, victim, TO_CHAR);
        act("$n's spell has no effect on $N.", true, ch, 0, victim, TO_NOTVICT);
        act("$n's spell has no effect on you.", false, ch, 0, victim, TO_VICT);
        return CAST_RESULT_CHARGE;
    }

    switch (spellnum) {
    /* Temp HP spells */
    case SPELL_ENDURANCE:
    case SPELL_LESSER_ENDURANCE:
    case SPELL_GREATER_ENDURANCE:
    case SPELL_VITALITY:
    case SPELL_GREATER_VITALITY:
    case SPELL_DRAGONS_HEALTH:

        if (affected_by_spell(victim, SPELL_LESSER_ENDURANCE) || affected_by_spell(victim, SPELL_ENDURANCE) ||
            affected_by_spell(victim, SPELL_GREATER_ENDURANCE) || affected_by_spell(victim, SPELL_VITALITY) ||
            affected_by_spell(victim, SPELL_GREATER_VITALITY) || affected_by_spell(victim, SPELL_DRAGONS_HEALTH)) {
            send_to_char("Nothing happens!\r\n", ch);
            return CAST_RESULT_CHARGE;
        }

        eff[0].location = APPLY_HIT;
        eff[0].modifier = get_vitality_hp_gain(ch, spellnum);
        eff[0].duration = get_spell_duration(ch, spellnum);

        to_vict = get_vitality_vict_message(spellnum);
        to_room = "$N looks healthier than before!";
        to_char = "$N looks healthier than before!";
        break; /* <-- end Temp HP spells */

    case SPELL_ARMOR:

        /* check for exclusion of other armor spells */
        if (check_armor_spells(ch, victim, spellnum))
            return CAST_RESULT_CHARGE;

        eff[0].location = APPLY_AC;
        eff[0].modifier = 10 + (skill / 20); /* max 15 */
        eff[0].duration = 10 + (skill / 50); /* max 12 */
        to_vict = "You feel someone protecting you.";
        to_room = "$n calls upon $s gods to protect $N.";
        break;

    case SPELL_BARKSKIN:
        /* Check for other types of armor spells */
        if (check_armor_spells(ch, victim, spellnum))
            return CAST_RESULT_CHARGE;

        eff[0].location = APPLY_AC;
        eff[0].modifier = 7 + (skill / 9);  /* max 18 */
        eff[0].duration = 5 + (skill / 10); /* max 15 */

        to_char = "&3$N's&3 skin hardens to bark.&0";
        to_vict = "&3Your skin hardens to bark.&0";
        to_room = "&3$N's&3 skin hardens to bark.&0";
        break;

    case SPELL_BLESS:

        if (GET_LEVEL(ch) < LVL_IMMORT && !IS_GOOD(ch) && casttype == CAST_SPELL) {
            send_to_char("The gods have forsaken you in your evilness!\r\n", ch);
            act("There is no effect.  $U$n adopts a dejected look.", true, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        if (affected_by_spell(victim, SPELL_DARK_PRESENCE) || affected_by_spell(victim, SPELL_DEMONSKIN) ||
            affected_by_spell(victim, SPELL_DEMONIC_ASPECT) || affected_by_spell(victim, SPELL_DEMONIC_MUTATION) ||
            affected_by_spell(victim, SPELL_WINGS_OF_HELL)) {
            act("$N is already blessed by some dark gods.", false, ch, 0, victim, TO_CHAR);
            act("$n looks a little overprotective.", true, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        if (affected_by_spell(victim, SPELL_EARTH_BLESSING)) {
            act("$N is already blessed by nature.", false, ch, 0, victim, TO_CHAR);
            act("$n looks a little overprotective.", true, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        /* Alignment Checks! */
        if (IS_EVIL(victim)) {
            act("You can't bless evil people!", false, ch, 0, 0, TO_CHAR);
            act("$n tries to awaken your inner angel.\r\nSilly isn't $e?", false, ch, 0, victim, TO_VICT);
            act("$n fails to awaken $N's inner angel.", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        eff[0].location = APPLY_HITROLL;
        eff[0].modifier = 1 + (skill >= 50);
        eff[0].duration = 10 + (skill / 7); /* 10-24 hrs */
        eff[1].location = APPLY_SAVING_SPELL;
        eff[1].modifier = -2 - (skill / 10);
        eff[1].duration = eff[0].duration;
        SET_FLAG(eff[2].flags, EFF_BLESS);
        eff[2].duration = eff[0].duration;
        to_char = "$N is inspired by your gods.";
        to_vict = "Your inner angel is inspired by $n.\r\nYou feel righteous.";
        to_room = "$N is inspired to do good by $n.";
        break;

    case SPELL_BLINDNESS:
    case SPELL_BLINDING_BEAUTY:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        if (MOB_FLAGGED(victim, MOB_NOBLIND)) {
            send_to_char("You seem unable to blind this creature.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }

        if (mag_savingthrow(victim, savetype)) {
            act("$N resists your pitiful attempt to blind $M.", false, ch, 0, victim, TO_CHAR);
            act("&7&b$n tries to blind you but fails!&0", false, ch, 0, victim, TO_VICT);
            act("&7&b$n tries to blind $N but nothing happens.&0", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        eff[0].location = APPLY_HITROLL;
        eff[0].modifier = -4;
        eff[0].duration = 2;
        SET_FLAG(eff[0].flags, EFF_BLIND);

        eff[1].location = APPLY_AC;
        eff[1].modifier = -40;
        eff[1].duration = 2;
        SET_FLAG(eff[1].flags, EFF_BLIND);

        to_char = "&9&b$N&9&b is blinded by you!&0";
        to_room = "&9&b$N&9&b is blinded by $n!&0";
        to_vict = "&9&bYou have been blinded!&0";
        break;

    case SPELL_BLUR:

        SET_FLAG(eff[0].flags, EFF_BLUR);
        eff[0].duration = 2 + (skill / 21); /* max 6 */
        to_vict = "&7The world seems to slow as you start moving with unnatural speed!&0";
        to_room = "&7$N's image blurs in unnatural speed!&0";
        break;

    case SPELL_BONE_ARMOR:

        if (check_armor_spells(ch, victim, spellnum))
            return CAST_RESULT_CHARGE;

        eff[0].location = APPLY_AC;
        eff[0].modifier = 10 + (skill / 6);    /* max 25 */
        eff[0].duration = 8 + 2 * (skill / 5); /* max 48 */
        to_char = "&3$N's&3 skin hardens into a bone carapace.&0";
        to_vict = "&3Your skin hardens into a bone carapace.&0";
        to_room = "&3$N's&3 skin hardens to a bone carapace.&0";
        break;

    case SPELL_BONE_DRAW:
        if (EFF_FLAGGED(victim, EFF_IMMOBILIZED)) {
            act("$n is already immobilized!", false, ch, 0, victim, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }

        if (evades_spell(ch, victim, spellnum, skill))
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;

        to_char = "You conjure four magical bones to lock $N in place!";
        to_vict = "$n conjures four magical bones which bind your legs!";
        to_room = "$n conjures four magical bones that lock around $N's legs!";

        SET_FLAG(eff[0].flags, EFF_IMMOBILIZED);
        eff[0].location = APPLY_NONE;
        eff[0].modifier = 4;
        eff[0].duration = 1 + skill / 25; /* 1-5 hours */
        if (!IS_NPC(victim))
            eff[0].duration = 1;
        remember(victim, ch);
        break;

    case SPELL_CHILL_TOUCH:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;
        if (mag_savingthrow(victim, savetype)) {
            act("$N resists your withering effect!", false, ch, 0, victim, TO_CHAR);
            act("You resist $n's withering effects!", false, ch, 0, victim, TO_VICT);
            act("$N resists $n's withering effect!", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        eff[0].location = APPLY_STR;
        eff[0].duration = 3 + (skill / 20);  /* max 8 */
        eff[0].modifier = -5 - (skill / 10); /* max -15 */

        to_vict = "You feel your strength wither!";
        to_char = "$N is withered by your cold!";
        to_room = "$N withers slightly from $n's cold!";
        break;

    case SPELL_CIRCLE_OF_LIGHT:

        SET_FLAG(eff[0].flags, EFF_LIGHT);
        eff[0].duration = 5 + (skill / 2); /* max 55 */
        refresh = false;
        to_vict = "&7&bA bright white circle of light begins hovering about your head.&0";
        to_room = "&7&bA bright white circle of light appears over $N's&7&b head.";
        break;

    case SPELL_COLDSHIELD:

        if (EFF_FLAGGED(ch, EFF_FIRESHIELD)) {
            cprintf(ch, "The shield of fire around %s body negates your spell.\r\n",
                    ch == victim ? "your" : HSHR(victim));
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_COLDSHIELD);
        eff[0].duration = skill / 20; /* max 5 */
        refresh = false;
        to_vict = "&4A jagged formation of i&bc&7e sh&4ard&0&4s forms around you.&0";
        to_room = "&4A jagged formation of i&bc&7e sh&4ard&0&4s forms around $N&0&4.&0";
        break;

    case SPELL_CONFUSION:
        /* Check for resistance due to high wis/dex.
         * Up to an additional 10% chance to evade. */
        i = (GET_DEX(victim) + GET_WIS(victim) - 100) / 10;
        if (i > number(1, 100)) {
            act("$n's eyes start to cross, but $e shakes it off.", true, victim, 0, 0, TO_ROOM);
            send_to_char("Your eyes start to &5spin off&0 in different directions, but you manage\r\n", ch);
            send_to_char("to bring them back under control.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        SET_FLAG(eff[0].flags, EFF_CONFUSION);
        eff[0].duration = 2 + skill / 40;
        to_vict = "&5You suddenly find it difficult to focus upon your foes.&0";
        to_room = "$N can't decide which way to cross $S eyes!";
        break;

    case SPELL_CURSE:

        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        if (mag_savingthrow(victim, savetype)) {
            send_to_char(NOEFFECT, ch);
            act("&7&b$n tries to curse you but fails!&0", false, ch, 0, victim, TO_VICT);
            act("&7&b$n squints at $N but nothing happens.&0", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        eff[0].location = APPLY_HITROLL;
        eff[0].duration = 5 + (skill / 14);  /* max 12 */
        eff[0].modifier = -1 - (skill / 50); /* max -3 */
        SET_FLAG(eff[0].flags, EFF_CURSE);
        eff[1].location = APPLY_DAMROLL;
        eff[1].duration = eff[0].duration;
        eff[1].modifier = eff[0].modifier;
        accum_effect = true;
        to_char = "You curse $N! Muahahah!";
        to_room = "$N briefly glows red!";
        to_vict = "You feel very uncomfortable.";
        break;

    case SPELL_DARK_PRESENCE:

        if (GET_LEVEL(ch) < LVL_IMMORT && !IS_EVIL(ch) && casttype == CAST_SPELL) {
            send_to_char("In your goodness, the dark gods have forsaken you!\r\n", ch);
            act("Nothing happens.  $U$n looks rather forlorn.", true, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        if (affected_by_spell(victim, SPELL_BLESS) || affected_by_spell(victim, SPELL_WINGS_OF_HEAVEN)) {
            act("$N is already blessed by some other gods.", false, ch, 0, victim, TO_CHAR);
            act("$n looks a little overprotective.", true, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        if (affected_by_spell(victim, SPELL_EARTH_BLESSING)) {
            act("$N is already blessed by nature.", false, ch, 0, victim, TO_CHAR);
            act("$n looks a little overprotective.", true, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        /* Alignment Checks! */
        if (IS_GOOD(victim)) {
            act("You can't protect an evil ally if they are GOOD!", false, ch, 0, 0, TO_CHAR);
            act("$n tries to enrage your inner demon.\r\nSilly isn't $e?", false, ch, 0, victim, TO_VICT);
            act("$n fails to enrage $N's inner demon.", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        eff[0].location = APPLY_SAVING_SPELL;
        eff[0].modifier = -2 - (skill / 10);
        eff[0].duration = 10 + (skill / 7); /* 10-24 hrs */
        if (skill < 55) {
            to_char = "You summon allegiance from your dark gods to protect $N.";
            to_vict = "&9&bA dark presence fills your mind.&0";
            if (ch == victim)
                to_room =
                    "$n seizes up in pain!\r\n"
                    "$n crosses $s arms on $s chest, and is surrounded by a dark "
                    "presence.";
            else
                to_room =
                    "$n seizes up in pain!\r\n"
                    "$n grabs $N, who is surrounded by a dark presence.";
        } else {
            eff[1].location = APPLY_DAMROLL;
            eff[1].modifier = 1 + (skill > 95);
            eff[1].duration = eff[0].duration;
            to_char = "You summon allegiance from your dark gods to protect and enrage $N.";
            to_vict = "&9&bA dark presence fills your mind and body!&0";
            if (ch == victim)
                to_room =
                    "$n seizes up in pain!\r"
                    "\n$n crosses $s arms on $s chest, and is enraged by a dark "
                    "presence.";
            else
                to_room =
                    "$n seizes up in pain!\r"
                    "\n$n grabs $N, who is enraged by a dark presence.";
        }
        SET_FLAG(eff[2].flags, EFF_HEX);
        eff[2].duration = eff[0].duration;
        break;

    case SPELL_DEMONIC_ASPECT:
    case SPELL_DEMONIC_MUTATION:

        if (affected_by_spell(victim, SPELL_DEMONIC_ASPECT) || affected_by_spell(victim, SPELL_DEMONIC_MUTATION)) {
            send_to_char("You're feeling pretty demonic already.\r\n", victim);
            return CAST_RESULT_CHARGE;
        }

        eff[0].location = APPLY_HIT;
        /* starts at (level / 5) and goes to (level / 1) */
        eff[0].modifier = skill / (1 + ((100 - skill) / 25));
        eff[0].duration = 5 + (skill / 20); /* max 10 */
        eff[1].location = APPLY_STR;
        eff[1].modifier = 5 + (skill / 14); /* max 12 */
        eff[1].duration = eff[0].duration;

        /* Modify hp by alignment */
        eff[0].modifier *= GET_ALIGNMENT(victim) / -1000.0;

        if (spellnum == SPELL_DEMONIC_ASPECT) {
            to_vict = "&1Your body fills with a demonic strength.&0";
            to_room = "&1$n's&1 body &bglows red&0&1 briefly and grows stronger.&0";
        } else if (spellnum == SPELL_DEMONIC_MUTATION) {
            eff[0].modifier *= 2.5;
            to_vict = "&1Your body fills with a demonic strength.&0";
            to_room = "&1$n's eyes flash &8red&0&1 as $e suddenly sprouts horns!&0";
        }
        break;

    case SPELL_DEMONSKIN:

        if (check_armor_spells(ch, victim, spellnum))
            return CAST_RESULT_CHARGE;

        /* Alignement Check! */
        if (IS_GOOD(victim)) {
            act("You can't protect an evil ally if they are GOOD!", false, ch, 0, 0, TO_CHAR);
            act("$n tries to wrap you in demonic skin!\r\nSilly isn't $e?", false, ch, 0, victim, TO_VICT);
            act("$n fails to wrap $N is a demonic skin of protection.", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        } else if (GET_ALIGNMENT(victim) > 0) {
            eff[0].location = APPLY_AC;
            eff[0].modifier = 8;
            eff[0].duration = 5;
            SET_FLAG(eff[0].flags, EFF_PROT_FIRE);
        } else {
            eff[0].location = APPLY_AC;
            eff[0].modifier = 10 + (skill / 20);
            eff[0].duration = 10 + (skill / 50);
            SET_FLAG(eff[0].flags, EFF_PROT_FIRE);
        }
        to_vict = "&1Your skin toughens into a dark red hide.&0";
        to_room = "&1$N's&1 skin toughens into a dark red hide.&0";
        break;

    case SPELL_DETECT_ALIGN:

        SET_FLAG(eff[0].flags, EFF_DETECT_ALIGN);
        eff[0].duration = 5 + (skill / 10); /* max 15 */
        to_char = "$N can determine alignment.";
        to_room = "&7&b$N&7&b glows briefly.&0";
        to_vict = "Your eyes tingle.";
        break;

    case SPELL_DETECT_INVIS:

        SET_FLAG(eff[0].flags, EFF_DETECT_INVIS);
        eff[0].location = APPLY_PERCEPTION;
        eff[0].modifier = 10;
        eff[0].duration = 5 + (skill / 10); /* max 15 */
        to_vict = "Your eyes tingle.";
        break;

    case SPELL_DETECT_MAGIC:

        SET_FLAG(eff[0].flags, EFF_DETECT_MAGIC);
        eff[0].duration = 5 + (skill / 10); /* max 15 */
        to_vict = "Your eyes tingle.";
        break;

    case SPELL_DETECT_POISON:

        SET_FLAG(eff[0].flags, EFF_DETECT_POISON);
        eff[0].duration = 5 + (skill / 10); /* max 15 */
        to_vict = "Your eyes tingle.";
        break;

    case SPELL_DISEASE:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        if (mag_savingthrow(victim, SAVING_SPELL)) {
            act("You resist $n's foul incantation!", false, ch, 0, victim, TO_VICT);
            act("$N holds $S breath avoiding $n's diseased air!", false, ch, 0, victim, TO_NOTVICT);
            act("$N resists your disease!", true, ch, 0, victim, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }

        eff[0].location = APPLY_CON;
        eff[0].modifier = -10 - (skill / 10);
        eff[0].duration = 5 + (skill / 10);
        SET_FLAG(eff[0].flags, EFF_DISEASE);
        eff[1].location = APPLY_STR;
        eff[1].modifier = eff[0].modifier;
        eff[1].duration = eff[0].duration;
        to_char = "Your diseased air infects $N!";
        to_vict =
            "&3You choke and gasp on $n's foul air as a sick feeling "
            "overtakes you.\r\n"
            "You feel seriously ill!&0";
        to_room = "&3$N&3 chokes and gasps on $n's foul air, $E looks seriously ill!";
        break;

    case SPELL_EARTH_BLESSING:

        if (GET_LEVEL(ch) < LVL_IMMORT && !IS_NEUTRAL(ch) && casttype == CAST_SPELL) {
            send_to_char("Nature has forsaken you in your zealousness!\r\n", ch);
            act("There is no effect.  $U$n adopts a dejected look.", true, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        if (affected_by_spell(victim, SPELL_DARK_PRESENCE) || affected_by_spell(victim, SPELL_DEMONSKIN) ||
            affected_by_spell(victim, SPELL_DEMONIC_ASPECT) || affected_by_spell(victim, SPELL_DEMONIC_MUTATION) ||
            affected_by_spell(victim, SPELL_WINGS_OF_HELL)) {
            act("$N is already blessed by some dark gods.", false, ch, 0, victim, TO_CHAR);
            act("$n looks a little overprotective.", true, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        if (affected_by_spell(victim, SPELL_BLESS) || affected_by_spell(victim, SPELL_WINGS_OF_HEAVEN)) {
            act("$N is already blessed by some other gods.", false, ch, 0, victim, TO_CHAR);
            act("$n looks a little overprotective.", true, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        /* Alignment Checks! */
        if (IS_EVIL(victim)) {
            act("You can't bless evil people!", false, ch, 0, 0, TO_CHAR);
            act("$n tries to attune you to nature.\r\nSilly isn't $e?", false, ch, 0, victim, TO_VICT);
            act("$n fails to attune $N to nature.", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        if (IS_GOOD(victim)) {
            act("You can't bless good people!", false, ch, 0, 0, TO_CHAR);
            act("$n tries to attune you to nature.\r\nSilly isn't $e?", false, ch, 0, victim, TO_VICT);
            act("$n fails to attune $N to nature.", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        eff[0].location = APPLY_HITROLL;
        eff[0].modifier = 1 + (skill >= 50);
        eff[0].duration = 10 + (skill / 7); /* 10-24 hrs */
        eff[1].location = APPLY_SAVING_SPELL;
        eff[1].modifier = -2 - (skill / 10);
        eff[1].duration = eff[0].duration;
        SET_FLAG(eff[2].flags, EFF_BLESS);
        eff[2].duration = eff[0].duration;
        to_char = "$N is imbued with the power of nature.";
        to_vict = "$n imbues you with the power of nature.\r\nYou feel righteous.";
        to_room = "$N is imbued with the power of nature by $n.";
        break;

    case SPELL_ELEMENTAL_WARDING:

        if (is_abbrev(buf2, "fire")) {
            SET_FLAG(eff[0].flags, EFF_PROT_FIRE);
            to_vict = "You are warded from &1fire&0.";
            to_char = "You protect $N from &1fire&0.";
        } else if (is_abbrev(buf2, "cold")) {
            SET_FLAG(eff[0].flags, EFF_PROT_COLD);
            to_vict = "You are warded from the &4cold&0.";
            to_char = "You protect $N from the &4cold&0.";
        } else if (is_abbrev(buf2, "air")) {
            SET_FLAG(eff[0].flags, EFF_PROT_AIR);
            to_vict = "You are warded from &6&bair&0.";
            to_char = "You protect $N from &6&bair&0.";
        } else if (is_abbrev(buf2, "earth")) {
            SET_FLAG(eff[0].flags, EFF_PROT_EARTH);
            to_vict = "You are warded from &3earth&0.";
            to_char = "You protect $N from &3earth&0.";
        } else {
            send_to_char("What element do you want to ward against?\r\n", ch);
            return 0;
        }

        eff[0].duration = 5 + (skill / 14); /* max 12 */
        to_room = "&7&b$N&7&b glows briefly.&0";
        break;

    case SPELL_ENHANCE_ABILITY:

        /* This is rather a hack of the intended nature of how the
           spell should work, but this is better than the way it was
           RSD 3/27/00 */

        /* This is a re-work of the spell Strength */

        if (is_abbrev(buf2, "strength")) {
            eff[0].location = APPLY_STR;
            to_vict = "You feel stronger!";
            to_char = "You increase $N's strength!";
            to_room = "$N looks stronger!";
        } else if (is_abbrev(buf2, "dexterity")) {
            eff[0].location = APPLY_DEX;
            to_vict = "You feel more dexterous!";
            to_char = "You increase $N's dexterity!";
            to_room = "$N looks more dexterous!";
        } else if (is_abbrev(buf2, "constitution")) {
            eff[0].location = APPLY_CON;
            to_vict = "You feel healthier!";
            to_char = "You increase $N's constitution!";
            to_room = "$N looks healthier!";
        } else if (is_abbrev(buf2, "intelligence")) {
            eff[0].location = APPLY_INT;
            to_vict = "You feel smarter!";
            to_char = "You increase $N's intelligence!";
            to_room = "$N looks smarter!";
        } else if (is_abbrev(buf2, "wisdom")) {
            eff[0].location = APPLY_WIS;
            to_vict = "You feel wiser!";
            to_char = "You increase $N's wisdom!";
            to_room = "$N looks wiser!";
        } else if (is_abbrev(buf2, "charisma")) {
            eff[0].location = APPLY_CHA;
            to_vict = "You feel more charismatic!";
            to_char = "You increase $N's charisma!";
            to_room = "$N looks more charismatic!";
        } else {
            send_to_char("What abiliy do you want to enhance?\r\n", ch);
            return 0;
        }

        eff[0].modifier = 2 + (skill / 8);  /* max 14 */
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        break;

    case SPELL_ENHANCE_STR:

        /* This is copy-paste from the Enhance Ability spell above */

        /* First check if already affected by an enhancement */
        if (affected_by_spell(victim, SPELL_ENHANCE_ABILITY) || affected_by_spell(victim, SPELL_ENHANCE_STR) ||
            affected_by_spell(victim, SPELL_ENHANCE_DEX) || affected_by_spell(victim, SPELL_ENHANCE_CON) ||
            affected_by_spell(victim, SPELL_ENHANCE_INT) || affected_by_spell(victim, SPELL_ENHANCE_WIS) ||
            affected_by_spell(victim, SPELL_ENHANCE_CHA)) {
            send_to_char("You are already enhanced!\r\n", victim);
            return 0;
        }

        eff[0].location = APPLY_STR;
        to_vict = "You feel stronger!";
        to_char = "You increase $N's strength!";
        to_room = "$N looks stronger!";

        eff[0].modifier = 2 + (skill / 8);  /* max 14 */
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        break;

    case SPELL_ENHANCE_DEX:

        /* This is copy-paste from the Enhance Ability spell above */

        /* First check if already affected by an enhancement */
        if (affected_by_spell(victim, SPELL_ENHANCE_ABILITY) || affected_by_spell(victim, SPELL_ENHANCE_STR) ||
            affected_by_spell(victim, SPELL_ENHANCE_DEX) || affected_by_spell(victim, SPELL_ENHANCE_CON) ||
            affected_by_spell(victim, SPELL_ENHANCE_INT) || affected_by_spell(victim, SPELL_ENHANCE_WIS) ||
            affected_by_spell(victim, SPELL_ENHANCE_CHA)) {
            send_to_char("You are already enhanced!\r\n", victim);
            return 0;
        }

        eff[0].location = APPLY_DEX;
        to_vict = "You feel more dexterous!";
        to_char = "You increase $N's dexterity!";
        to_room = "$N looks more dexterous!";

        eff[0].modifier = 2 + (skill / 8);  /* max 14 */
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        break;

    case SPELL_ENHANCE_CON:

        /* This is copy-paste from the Enhance Ability spell above */

        /* First check if already affected by an enhancement */
        if (affected_by_spell(victim, SPELL_ENHANCE_ABILITY) || affected_by_spell(victim, SPELL_ENHANCE_STR) ||
            affected_by_spell(victim, SPELL_ENHANCE_DEX) || affected_by_spell(victim, SPELL_ENHANCE_CON) ||
            affected_by_spell(victim, SPELL_ENHANCE_INT) || affected_by_spell(victim, SPELL_ENHANCE_WIS) ||
            affected_by_spell(victim, SPELL_ENHANCE_CHA)) {
            send_to_char("You are already enhanced!\r\n", victim);
            return 0;
        }

        eff[0].location = APPLY_CON;
        to_vict = "You feel healthier!";
        to_char = "You increase $N's constitution!";
        to_room = "$N looks healthier!";

        eff[0].modifier = 2 + (skill / 8);  /* max 14 */
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        break;

    case SPELL_ENHANCE_INT:

        /* This is copy-paste from the Enhance Ability spell above */

        /* First check if already affected by an enhancement */
        if (affected_by_spell(victim, SPELL_ENHANCE_ABILITY) || affected_by_spell(victim, SPELL_ENHANCE_STR) ||
            affected_by_spell(victim, SPELL_ENHANCE_DEX) || affected_by_spell(victim, SPELL_ENHANCE_CON) ||
            affected_by_spell(victim, SPELL_ENHANCE_INT) || affected_by_spell(victim, SPELL_ENHANCE_WIS) ||
            affected_by_spell(victim, SPELL_ENHANCE_CHA)) {
            send_to_char("You are already enhanced!\r\n", victim);
            return 0;
        }

        eff[0].location = APPLY_INT;
        to_vict = "You feel smarter!";
        to_char = "You increase $N's intelligence!";
        to_room = "$N looks smarter!";

        eff[0].modifier = 2 + (skill / 8);  /* max 14 */
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        break;

    case SPELL_ENHANCE_WIS:

        /* This is copy-paste from the Enhance Ability spell above */

        /* First check if already affected by an enhancement */
        if (affected_by_spell(victim, SPELL_ENHANCE_ABILITY) || affected_by_spell(victim, SPELL_ENHANCE_STR) ||
            affected_by_spell(victim, SPELL_ENHANCE_DEX) || affected_by_spell(victim, SPELL_ENHANCE_CON) ||
            affected_by_spell(victim, SPELL_ENHANCE_INT) || affected_by_spell(victim, SPELL_ENHANCE_WIS) ||
            affected_by_spell(victim, SPELL_ENHANCE_CHA)) {
            send_to_char("You are already enhanced!\r\n", victim);
            return 0;
        }

        eff[0].location = APPLY_WIS;
        to_vict = "You feel wiser!";
        to_char = "You increase $N's wisdom!";
        to_room = "$N looks wiser!";

        eff[0].modifier = 2 + (skill / 8);  /* max 14 */
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        break;

    case SPELL_ENHANCE_CHA:

        /* This is copy-paste from the Enhance Ability spell above */

        /* First check if already affected by an enhancement */
        if (affected_by_spell(victim, SPELL_ENHANCE_ABILITY) || affected_by_spell(victim, SPELL_ENHANCE_STR) ||
            affected_by_spell(victim, SPELL_ENHANCE_DEX) || affected_by_spell(victim, SPELL_ENHANCE_CON) ||
            affected_by_spell(victim, SPELL_ENHANCE_INT) || affected_by_spell(victim, SPELL_ENHANCE_WIS) ||
            affected_by_spell(victim, SPELL_ENHANCE_CHA)) {
            send_to_char("You are already enhanced!\r\n", victim);
            return 0;
        }

        eff[0].location = APPLY_CHA;
        to_vict = "You feel more charismatic!";
        to_char = "You increase $N's charisma!";
        to_room = "$N looks more charismatic!";

        eff[0].modifier = 2 + (skill / 8);  /* max 14 */
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        break;

    case SPELL_ENLARGE:

        if (IS_NPC(victim))
            return CAST_RESULT_CHARGE;

        eff[0].location = APPLY_SIZE;
        eff[0].modifier = 1;
        SET_FLAG(eff[0].flags, EFF_ENLARGE);
        eff[0].duration = 1 + (skill / 40);

        eff[1].location = APPLY_CON;
        eff[1].modifier = 10;
        eff[1].duration = eff[0].duration;

        eff[2].location = APPLY_STR;
        eff[2].modifier = 10;
        eff[2].duration = eff[0].duration;

        refresh = true;
        to_vict =
            "&9&bYour skin starts to itch as you enlarge to twice your "
            "normal size!&0";
        to_room = "&9&b$N's skin ripples as $E enlarges to twice $S normal size!&0";
        break;

    case SPELL_ENTANGLE:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        /* A difficult spell to land with success */
        if (mag_savingthrow(victim, SAVING_PARA) || skill - GET_LEVEL(victim) < number(0, 80)) {
            act("&2&bYour crop of ripe vines search in vain for $N.&0", false, ch, 0, victim, TO_CHAR);
            act("&2&bA crop of ripe vines snakes along the ground, unable to locate "
                "you!&0",
                false, ch, 0, victim, TO_VICT);
            act("&2&bA crop of ripe vines searches in vain for $N.&0", true, ch, 0, victim, TO_NOTVICT);
            /* start combat for failure */
            if (!FIGHTING(victim)) {
                attack(victim, ch);
                remember(victim, ch);
            }
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        /* Chance for major para increases with higher skill. */
        if (skill >= 40 && number(0, 100) < 2 + (skill / 14)) {
            SET_FLAG(eff[0].flags, EFF_MAJOR_PARALYSIS);
            eff[0].duration = 2 + (skill > 95);
        } else {
            SET_FLAG(eff[0].flags, EFF_MINOR_PARALYSIS);
            eff[0].duration = 2 + (skill / 24);
        }
        to_char =
            "&2&bYour crop of thick branches and vines burst"
            " from the ground, partially entangling $N!&0";
        to_room =
            "&2&bA slew of thick branches and vines burst"
            " from the ground, partially entangling $N!&0";
        to_vict =
            "&2&bA slew of thick branches and vines burst"
            " from the ground, partially entangling you!&0";
        refresh = false;
        break;

    case SPELL_FAMILIARITY:
        SET_FLAG(eff[0].flags, EFF_FAMILIARITY);
        eff[0].duration = skill / 5 + 4;
        to_vict = "&7&bAn aura of comfort and solidarity surrounds you.&0";
        to_room =
            "You know in your heart that $N is a steady friend, to be "
            "depended upon.";
        break;

    case SPELL_FARSEE:

        SET_FLAG(eff[0].flags, EFF_FARSEE);
        eff[0].duration = 5 + (skill / 10); /* max 15 */
        to_vict = "Your sight improves dramatically.";
        to_room = "$N's pupils dilate rapidly for a second.";
        break;

    case CHANT_HYMN_OF_SAINT_AUGUSTINE:

        if (is_abbrev(buf2, "fire")) {
            SET_FLAG(eff[0].flags, EFF_FIREHANDS);
            to_vict = "&1Your fists burn with inner fire.&0";
            to_room = "&1$N's fists burn with inner fire.&0";
        } else if (is_abbrev(buf2, "ice")) {
            SET_FLAG(eff[0].flags, EFF_ICEHANDS);
            to_vict = "&4&bYou unleash the blizzard in your heart.&0";
            to_room = "&4&b$N unleashes the blizzard in $S heart.&0";
        } else if (is_abbrev(buf2, "lightning")) {
            SET_FLAG(eff[0].flags, EFF_LIGHTNINGHANDS);
            to_vict = "&6&bYour knuckles crackle with lightning.&0";
            to_room = "&6&b$N's knuckles crackle with lightning.&0";
        } else if (is_abbrev(buf2, "acid")) {
            SET_FLAG(eff[0].flags, EFF_ACIDHANDS);
            to_vict = "&3&bYou charge your hands with corrosive chi.&0";
            to_room = "&3&b$N charges $S hands with corrosive chi.&0";
        } else {
            send_to_char("What element do you want to imbue?\r\n", ch);
            send_to_char("Fire, ice, lightning, or acid?\r\n", ch);
            return 0;
        }
        eff[0].duration = (skill / 10) + wis_app[GET_WIS(ch)].bonus; /* max 15 */
        break;

    case SPELL_FIRESHIELD:

        if (EFF_FLAGGED(ch, EFF_COLDSHIELD)) {
            cprintf(ch, "The shield of ice around %s body negates your spell.\r\n",
                    ch == victim ? "your" : HSHR(victim));
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_FIRESHIELD);
        eff[0].duration = skill / 20; /* max 5 */
        refresh = false;
        to_vict = "&1A burning shield of f&bi&3r&7e&0&1 explodes from your body!&0";
        to_room = "&1A burning shield of f&bi&3r&7e&0&1 explodes from $N&0&1's body!&0";
        break;

    case SPELL_FLY:

        SET_FLAG(eff[0].flags, EFF_FLY);
        eff[0].duration = 5 + (skill / 10); /* max 15 */
        if (too_heavy_to_fly(victim)) {
            to_vict = "You feel somewhat lighter.";
            to_char = "$N remains earthbound.";
        } else {
            to_vict = "&7You fly through the air, as free as a bird!&0";
            to_room = "&6&b$N lifts into the air.&0";
            to_char = "&6&b$N lifts into the air.&0";
            if (AWAKE(victim)) {
                GET_POS(victim) = POS_FLYING;
                GET_STANCE(victim) = STANCE_ALERT;
            }
        }
        break;

    case SPELL_GAIAS_CLOAK:

        /* check for exclusion of other armor spells */
        if (check_armor_spells(ch, victim, spellnum))
            return CAST_RESULT_CHARGE;

        eff[0].location = APPLY_AC;
        eff[0].modifier = 15 + (skill / 16); /* max 21 */
        eff[0].duration = 5 + (skill / 14);  /* max 12 */

        refresh = false;
        to_room =
            "&2A cyclone of leaves and sticks twirls around $n&2, guarding "
            "$s body.&0";
        to_vict =
            "&2A cyclone of leaves and sticks twirls around you, guarding "
            "your body.&0";
        break;

    case SPELL_GLORY:
        SET_FLAG(eff[0].flags, EFF_GLORY);
        eff[0].duration = 5 + skill / 20;
        eff[1].location = APPLY_CHA;
        eff[1].modifier = 50;
        eff[1].duration = eff[0].duration;
        to_vict = "&7&bYou stand tall in the light, a beacon of greatness.&0";
        to_room = "&7&b$N seems taller in the light, and appears like unto a god.&0";
        break;

    case SPELL_HARNESS:

        if (EFF_FLAGGED(victim, EFF_HARNESS)) {
            send_to_char("You have already harnessed your energy!\r\n", victim);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_HARNESS);
        eff[0].duration = (skill >= 20); /* max 1 */
        refresh = false;
        to_vict = "&4&bYour veins begin to pulse with energy!&0";
        to_room = "&4&b$N&4&b's veins bulge as a surge of energy rushes into $M!&0";
        break;

    case SPELL_HASTE:

        SET_FLAG(eff[0].flags, EFF_HASTE);
        eff[0].duration = 2 + (skill / 21); /* max 6 */
        to_char = "&1$N starts to move with uncanny speed!&0";
        to_vict = "&1You start to move with uncanny speed!&0";
        to_room = "&1$N starts to move with uncanny speed!&0";
        break;

    case SPELL_ICE_ARMOR:

        /* check for exclusion of other armor spells */
        if (check_armor_spells(ch, victim, spellnum))
            return CAST_RESULT_CHARGE;

        eff[0].location = APPLY_AC;
        eff[0].modifier = 5 + (skill / 14); /* max 12 */
        eff[0].duration = 5 + (skill / 20); /* max 10 */
        to_vict = "&4Your body is encased in layer of &7&bsolid ice.&0";
        to_room = "&4$n's&4 body is encased in a layer of &7&bsolid ice.&0";
        break;

    case SPELL_INFRAVISION:

        if (affected_by_spell(victim, SPELL_NIGHT_VISION)) {
            if (victim == ch) {
                send_to_char("You are already enchanted with enhanced vision.\r\n", ch);
                act("$n looks a little overprotective.", true, ch, 0, 0, TO_ROOM);
            } else {
                act("$N seems to be able to sort of see enough already.", false, ch, 0, victim, TO_CHAR);
                act("You are already enchanted with enhanced vision.", false, ch, 0, victim, TO_VICT);
                act("$n looks a little overprotective.", true, ch, 0, victim, TO_NOTVICT);
            }
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_INFRAVISION);
        eff[0].duration = 5 + (skill / 10); /* max 15 */
        to_char = "$N's eyes glow red.";
        to_vict = "Your eyes glow red.";
        to_room = "$N's eyes glow red.";
        break;

    case SPELL_INSANITY:
    case SONG_CROWN_OF_MADNESS:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;
        if (mag_savingthrow(victim, SAVING_SPELL)) {
            act("$N has too strong a will to drive insane!", false, ch, 0, victim, TO_CHAR);
            act("Your strength of will protects you from an insane suggestion from "
                "$n... This time...",
                false, ch, 0, victim, TO_VICT);
            act("$N resists going insane at $n's suggestion.", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_INSANITY);
        eff[0].location = APPLY_WIS;
        eff[0].modifier = -50;
        eff[0].duration = 5;
        to_char = "You cause &5$N to snap... A crazed gleam fills $S eyes.&0";
        to_vict = "&5You go out of your &bMIND!&0";
        to_room = "&5$N's&5 psyche snaps... A crazed gleam fills $S eyes.&0";
        break;

    case SPELL_INVISIBLE:
    case SPELL_MASS_INVIS:
        eff[0].type = SPELL_INVISIBLE;
        eff[0].modifier = 40;
        eff[0].duration = 9 + (skill / 9); /* max 20 */
        eff[0].location = APPLY_AC;
        SET_FLAG(eff[0].flags, EFF_INVISIBLE);
        to_vict = "You vanish.";
        to_room = "$N slowly fades out of existence.";
        break;

    case SPELL_LEVITATE:

        SET_FLAG(eff[0].flags, EFF_LEVITATE);
        eff[0].duration = 5 + (skill / 10);
        to_char = "&6$N&0&6 floats up into the air.&0";
        to_vict = "&6You float up into the air.&0";
        to_room = "&6$N&0&6 floats up into the air.&0";
        break;

    case SPELL_MAGIC_TORCH:

        SET_FLAG(eff[0].flags, EFF_LIGHT);
        eff[0].duration = 5 + (skill / 2); /* max 55 */
        refresh = false;
        to_vict = "&1A magical flame bursts into focus, lighting the area.&0";
        to_room = "&1A magical flame bursts into focus, lighting the area.&0";
        break;

    case SPELL_MAJOR_GLOBE:

        if (EFF_FLAGGED(victim, EFF_MINOR_GLOBE)) {
            act("$N's minor globe of invulnerability resists your spell!", false, ch, 0, victim, TO_CHAR);
            act("Your minor globe of invulnerability resists $n's spell.", false, ch, 0, victim, TO_VICT);
            act("$n tries to add an additional globe of protection to $N.", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_MAJOR_GLOBE);
        eff[0].duration = 4 + (skill / 20); /* max 9 */
        refresh = false;
        to_char = "&1&bYour shimmering globe of force wraps around $N&1&b's body.&0";
        to_vict = "&1&bA shimmering globe of force wraps around your body.&0";
        to_room = "&1&bA shimmering globe of force wraps around $N&1&b's body.&0";
        break;

    case SPELL_MESMERIZE:
    case SONG_ENRAPTURE:
        if (!AWAKE(victim)) {
            act("$n makes colorful illusions before $N's closed eyes.", false, ch, 0, victim, TO_ROOM);
            act("$N is in no condition to notice your illusion.", false, ch, 0, victim, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }

        if (EFF_FLAGGED(victim, EFF_MESMERIZED)) {
            act("$n tries to get $N's attention, but it's no use.", true, ch, 0, victim, TO_ROOM);
            act("$n seems to be trying to show you something, but you're busy.", false, ch, 0, victim, TO_VICT);
            act("You weave a fascinating illusion, but $N is not paying attention.", false, ch, 0, victim, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }

        if (evades_spell(ch, victim, spellnum, skill))
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;

        to_room =
            "$n &0&5w&4e&5av&4es a &5mesme&4rizing pa&5ttern before &0$N's "
            "eyes.\r\n"
            "$U$N appears entranced, as if working out a puzzle of some kind.";
        to_vict =
            "$n shows you a truly fascinating puzzle.  You simply must work "
            "it out.";
        to_char =
            "You weave a mesmerizing pattern before $N, and $E seems\r\n"
            "to be utterly absorbed by it.";

        SET_FLAG(eff[0].flags, EFF_MESMERIZED);
        eff[0].duration = 2 + skill / 16; /* 2-8 hours */
        if (!IS_NPC(victim))
            eff[0].duration = 2; /* Players: just 2 hours. */
        remember(victim, ch);    /* Victim will be angry about this */
        break;

    case SPELL_MINOR_GLOBE:

        if (EFF_FLAGGED(victim, EFF_MAJOR_GLOBE)) {
            act("$N's globe of invulnerability resists your spell!", false, ch, 0, victim, TO_CHAR);
            act("Your globe of invulnerability resists $n's spell.", false, ch, 0, victim, TO_VICT);
            act("$n tries to add an additional globe of protection to $N.", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_MINOR_GLOBE);
        eff[0].duration = skill / 20; /* max 5 */
        refresh = false;
        to_char = "&1Your shimmering globe wraps around $N&0&1's body.&0";
        to_vict = "&1A shimmering globe wraps around your body.&0";
        to_room = "&1A shimmering globe wraps around $N&0&1's body.&0";
        break;

    case SPELL_MINOR_PARALYSIS:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;
        /* Make success based on skill and saving throw -myc 17 Feb 2007 */
        if (mag_savingthrow(victim, SAVING_PARA) || skill - GET_LEVEL(victim) < number(0, 70)) {
            act("&7&b$N resists your weak paralysis.&0", false, ch, 0, victim, TO_CHAR);
            act("&7&b$n tries to paralize you but fails!&0", false, ch, 0, victim, TO_VICT);
            act("&7&b$n squints at $N but nothing happens.&0", true, ch, 0, victim, TO_NOTVICT);

            /* start combat for failure */
            if (!FIGHTING(victim)) {
                attack(victim, ch);
                remember(victim, ch);
            }

            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        SET_FLAG(eff[0].flags, EFF_MINOR_PARALYSIS);
        eff[0].duration = 2 + (skill / 15); /* max 8 */
        refresh = false;
        to_char = "You paralyze $N! WooHoo!";
        to_vict = "&7&bAll motion in your body grinds to a halt.&0";
        to_room = "&7&bAll motion in $N&7&b's body grinds to a halt.&0";
        break;

    case SPELL_MIRAGE:

        /* Need a check to make sure no other conflicting armor spells can help as
         * well */
        if (check_armor_spells(ch, victim, spellnum))
            return CAST_RESULT_CHARGE;

        eff[0].location = APPLY_AC;
        eff[0].modifier = 5 + (skill / 16); /* max 11 */
        eff[0].duration = 5 + (skill / 25); /* max 9 */
        to_room =
            "&1$n's&1 form begins to waver as a wave of heat blurs the air "
            "around $m.&0";
        to_vict =
            "&1Your form begins to waver as a wave of heat blurs the air "
            "around you.&0";
        break;

    case SPELL_MISDIRECTION:
        SET_FLAG(eff[0].flags, EFF_MISDIRECTION);
        eff[0].duration = 2 + skill / 4;
        to_vict =
            "You feel like a stack of little illusions all pointing in "
            "different directions.";
        break;

    case SPELL_MONK_ACID:
        /* for potions and scrolls to turn monk punches to acid */
        /* check if already affected by one of the enhancement types */

        if (affected_by_spell(victim, SPELL_MONK_ACID) || affected_by_spell(victim, SPELL_MONK_COLD) ||
            affected_by_spell(victim, SPELL_MONK_FIRE) || affected_by_spell(victim, SPELL_MONK_SHOCK)) {
            send_to_char("You can only channel one element at a time.\r\n", victim);
            return 0;
        }

        SET_FLAG(eff[0].flags, EFF_ACIDHANDS);
        to_vict = "&3&bYou charge your hands with corrosive chi.&0";
        to_room = "&3&b$N charges $S hands with corrosive chi.&0";
        eff[0].duration = (skill / 10); /* max 10 */
        break;

    case SPELL_MONK_COLD:
        /* for potions and scrolls to turn monk punches to ice */
        /* check if already affected by one of the enhancement types */

        if (affected_by_spell(victim, SPELL_MONK_ACID) || affected_by_spell(victim, SPELL_MONK_COLD) ||
            affected_by_spell(victim, SPELL_MONK_FIRE) || affected_by_spell(victim, SPELL_MONK_SHOCK)) {
            send_to_char("You can only channel one element at a time.\r\n", victim);
            return 0;
        }

        SET_FLAG(eff[0].flags, EFF_ICEHANDS);
        to_vict = "&4&bYou unleash the blizzard in your heart.&0";
        to_room = "&4&b$N unleashes the blizzard in $S heart.&0";
        eff[0].duration = (skill / 10); /* max 10 */
        break;

    case SPELL_MONK_FIRE:
        /* for potions and scrolls to turn monk punches to fire */
        /* check if already affected by one of the enhancement types */

        if (affected_by_spell(victim, SPELL_MONK_ACID) || affected_by_spell(victim, SPELL_MONK_COLD) ||
            affected_by_spell(victim, SPELL_MONK_FIRE) || affected_by_spell(victim, SPELL_MONK_SHOCK)) {
            send_to_char("You can only channel one element at a time.\r\n", victim);
            return 0;
        }

        SET_FLAG(eff[0].flags, EFF_FIREHANDS);
        to_vict = "&1Your fists burn with inner fire.&0";
        to_room = "&1$N's fists burn with inner fire.&0";
        eff[0].duration = (skill / 10); /* max 10 */
        break;

    case SPELL_MONK_SHOCK:
        /* for potions and scrolls to turn monk punches to lightning */
        /* check if already affected by one of the enhancement types */

        if (affected_by_spell(victim, SPELL_MONK_ACID) || affected_by_spell(victim, SPELL_MONK_COLD) ||
            affected_by_spell(victim, SPELL_MONK_FIRE) || affected_by_spell(victim, SPELL_MONK_SHOCK)) {
            send_to_char("You can only channel one element at a time.\r\n", victim);
            return 0;
        }

        SET_FLAG(eff[0].flags, EFF_LIGHTNINGHANDS);
        to_vict = "&6&bYour knuckles crackle with lightning.&0";
        to_room = "&6&b$N's knuckles crackle with lightning.&0";
        eff[0].duration = (skill / 10); /* max 10 */
        break;

    case SPELL_NEGATE_COLD:

        SET_FLAG(eff[0].flags, EFF_NEGATE_COLD);
        eff[0].duration = 2 + (skill / 20); /* max 7 */
        refresh = false;
        to_vict = "&4&bYour body becomes impervious to the cold!&0";
        to_room = "&4$n&4's is protected by a &3&bwarm&0&4-looking magical field.&0";
        break;

    case SPELL_NEGATE_HEAT:

        SET_FLAG(eff[0].flags, EFF_NEGATE_HEAT);
        eff[0].duration = 2 + (skill / 20); /* max 7 */
        refresh = false;
        to_vict = "&6Your body becomes impervious to all forms of heat!&0";
        to_room = "&6$n&6 is surrounded by a frigid crystalline field.&0";
        break;

    case SPELL_NATURES_EMBRACE:
        SET_FLAG(eff[0].flags, EFF_CAMOUFLAGED);
        eff[0].duration = (skill / 3) + 1; /* range (1, 34) */
        to_vict = "&9&8You phase into the landscape.&0";
        to_room = "&9&8$n&9&8 phases into the landscape.&0";
        break;

    case SPELL_NATURES_GUIDANCE:
        eff[0].location = APPLY_HITROLL;
        eff[0].modifier = eff[0].duration = (skill / 20) + 1; /* range (1, 6) */
        /* Self only */
        to_vict = "You feel a higher power guiding your hands.";
        to_room = "$N calls on guidance from a higher power.";
        break;

    case SPELL_NIGHT_VISION:

        if (affected_by_spell(victim, SPELL_INFRAVISION)) {
            if (ch == victim)
                act("You are already enchanted with enhanced vision.", false, ch, 0, 0, TO_CHAR);
            else
                act("$N is already enchanted with enhanced vision.", false, ch, 0, victim, TO_CHAR);
            act("$n looks a little overprotective.", true, ch, 0, victim, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_ULTRAVISION);
        eff[0].duration = (skill / 21); /* max 4 */
        to_room = "$N's eyes glow a dim neon green.";
        to_vict = "&9&bYour vision sharpens a bit.";
        break;

    case SPELL_POISON:
    case SPELL_GAS_BREATH:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;
        if (GET_LEVEL(victim) >= LVL_IMMORT || damage_evasion(victim, 0, 0, DAM_POISON)) {
            act("$n is unaffected!", false, victim, 0, 0, TO_ROOM);
            act("You are unaffected!", false, victim, 0, 0, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }
        if (mag_savingthrow(victim, SAVING_PARA)) {
            if (spellnum != SPELL_GAS_BREATH) {
                /* No message is sent for dodging this part of a gas attack
                 * because relevant messages have already been sent in the
                 * mag_damage portion of the attack. */
                act("You dodge $n's attempt to prick you!", false, ch, 0, victim, TO_VICT);
                act("$N dodges $n's attempt to prick $M.", true, ch, 0, victim, TO_NOTVICT);
            }
            send_to_char(NOEFFECT, ch);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }
        eff[0].location = APPLY_STR;
        eff[0].modifier = (-2 - (skill / 4) - (skill / 20)) * susceptibility(victim, DAM_POISON) / 100; /* max -32 */
        SET_FLAG(eff[0].flags, EFF_POISON);
        eff[0].duration = 4 + (skill / 10); /* max 14 */
        eff[0].type = SPELL_POISON;
        to_vict = "You feel very sick.";
        to_room = "$N gets violently ill!";
        to_char = "$N gets violently ill!";
        break;

    case SPELL_PRAYER:

        /* I think this may be demonic aspects counter part */
        eff[0].location = APPLY_HIT;
        /* starts at (level / 3) and goes to (level / 1) */
        eff[0].modifier = skill / (1 + ((100 - skill) / 30));
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        eff[1].location = APPLY_SAVING_SPELL;
        eff[1].modifier = -5 - (skill / 14); /* max -12 */
        eff[1].duration = eff[0].duration;

        /* Modify hp by alignment */
        eff[0].modifier *= GET_ALIGNMENT(victim) / 1000.0;

        to_vict = "Your prayer is answered...\r\nYou feel full of life!";
        to_room = "$N perks up, looking full of life.";
        break;

    case SPELL_PROT_FROM_EVIL:

        /* Alignment Check! */
        if (IS_EVIL(victim)) {
            act("You can't protect an ally if they are EVIL!", false, ch, 0, victim, TO_CHAR);
            act("$n tries to protect you from evil!\r\nSilly isn't $e.", false, ch, 0, victim, TO_VICT);
            act("$n fails to protect $N from evil.  DUH!", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_PROTECT_EVIL);
        eff[0].duration = 9 + (skill / 9); /* max 20 */
        to_vict = "You feel invulnerable!";
        to_char = "You surround $N with glyphs of holy warding.";
        break;

    case SPELL_PROT_FROM_GOOD:

        /* Alignment Check! */
        if (IS_GOOD(victim)) {
            act("You can't protect an ally if they are GOOD!", false, ch, 0, victim, TO_CHAR);
            act("$n tries to protect you from evil!\r\nSilly isn't $e.", false, ch, 0, victim, TO_VICT);
            act("$n fails to protect $N from good.  DUH!", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_PROTECT_GOOD);
        eff[0].duration = 9 + (skill / 9); /* max 20 */
        to_vict = "You feel invulnerable!";
        to_char = "You surround $N with glyphs of unholy warding.";
        break;

    case SPELL_PROTECT_ACID:

        /* is the target already protected? */
        if (affected_by_spell(victim, SPELL_ELEMENTAL_WARDING) || affected_by_spell(victim, SPELL_PROTECT_ACID) ||
            affected_by_spell(victim, SPELL_PROTECT_COLD) || affected_by_spell(victim, SPELL_PROTECT_FIRE) ||
            affected_by_spell(victim, SPELL_PROTECT_SHOCK)) {
            send_to_char("You can only be protected from one element at a time.\r\n", victim);
            return 0;
        }

        SET_FLAG(eff[0].flags, EFF_PROT_EARTH);
        to_vict = "You are warded from &3earth&0.";
        to_char = "You protect $N from &3earth&0.";
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        to_room = "&7&b$N&7&b glows briefly.&0";
        break;

    case SPELL_PROTECT_COLD:

        /* is the target already protected? */
        if (affected_by_spell(victim, SPELL_ELEMENTAL_WARDING) || affected_by_spell(victim, SPELL_PROTECT_ACID) ||
            affected_by_spell(victim, SPELL_PROTECT_COLD) || affected_by_spell(victim, SPELL_PROTECT_FIRE) ||
            affected_by_spell(victim, SPELL_PROTECT_SHOCK)) {
            send_to_char("You can only be protected from one element at a time.\r\n", victim);
            return 0;
        }

        SET_FLAG(eff[0].flags, EFF_PROT_COLD);
        to_vict = "You are warded from the &4cold&0.";
        to_char = "You protect $N from the &4cold&0.";
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        to_room = "&7&b$N&7&b glows briefly.&0";
        break;

    case SPELL_PROTECT_FIRE:

        /* is the target already protected? */
        if (affected_by_spell(victim, SPELL_ELEMENTAL_WARDING) || affected_by_spell(victim, SPELL_PROTECT_ACID) ||
            affected_by_spell(victim, SPELL_PROTECT_COLD) || affected_by_spell(victim, SPELL_PROTECT_FIRE) ||
            affected_by_spell(victim, SPELL_PROTECT_SHOCK)) {
            send_to_char("You can only be protected from one element at a time.\r\n", victim);
            return 0;
        }

        SET_FLAG(eff[0].flags, EFF_PROT_FIRE);
        to_vict = "You are warded from &1fire&0.";
        to_char = "You protect $N from &1fire&0.";
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        to_room = "&7&b$N&7&b glows briefly.&0";
        break;

    case SPELL_PROTECT_SHOCK:

        /* is the target already protected? */
        if (affected_by_spell(victim, SPELL_ELEMENTAL_WARDING) || affected_by_spell(victim, SPELL_PROTECT_ACID) ||
            affected_by_spell(victim, SPELL_PROTECT_COLD) || affected_by_spell(victim, SPELL_PROTECT_FIRE) ||
            affected_by_spell(victim, SPELL_PROTECT_SHOCK)) {
            send_to_char("You can only be protected from one element at a time.\r\n", victim);
            return 0;
        }

        SET_FLAG(eff[0].flags, EFF_PROT_AIR);
        to_vict = "You are warded from &6&bair&0.";
        to_char = "You protect $N from &6&bair&0.";
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        to_room = "&7&b$N&7&b glows briefly.&0";
        break;

    case SPELL_RAY_OF_ENFEEB:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;
        if (EFF_FLAGGED(victim, EFF_RAY_OF_ENFEEB)) {
            act("$N is already feeble enough dammit.", false, ch, 0, victim, TO_CHAR);
            act("$n seems to be looking at you funny.", false, ch, 0, victim, TO_VICT);
            act("&7&b$n squints at $N but nothing happens.&0", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }
        if (mag_savingthrow(victim, savetype)) {
            act("$N resists your feeble attempt!", false, ch, 0, victim, TO_CHAR);
            act("$n tries to drain your strength, but you resist!", false, ch, 0, victim, TO_VICT);
            act("&7&b$n squints at $N but nothing happens.&0", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        eff[0].location = APPLY_STR;
        eff[0].duration = 8 + (skill / 20);   /* max 13 */
        eff[0].modifier = -15 - (skill / 10); /* max -25 */
        SET_FLAG(eff[0].flags, EFF_RAY_OF_ENFEEB);
        to_vict = "You feel the strength flow out of your body.";
        to_room = "$N turns pale and starts to sag.";
        to_char = "$N turns pale and starts to sag.";
        break;

    case SPELL_REBUKE_UNDEAD:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        if (GET_LIFEFORCE(victim) != LIFE_UNDEAD) {
            act("$n seems confused as to your state of mortality.", false, ch, 0, victim, TO_VICT);
            act("$n tries to rebuke $N's buried undead nature.  Must be buried too deep.", true, ch, 0, victim,
                TO_ROOM);
            send_to_char("Your rebuke elicits nothing but a raised eyebrow.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        if (mag_savingthrow(victim, SAVING_SPELL)) {
            act("You stare blankly at $n as $e attempts to rebuke you.", false, ch, 0, victim, TO_VICT);

            act("$N looks at $n blankly as $e calls down a spell of condemnation.", true, ch, 0, victim, TO_NOTVICT);
            send_to_char("Your rebuke elicits nothing but a blank stare.\r\n", ch);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }
        eff[0].location = APPLY_STR;
        eff[1].location = APPLY_CON;
        eff[2].location = APPLY_AC;
        eff[0].modifier = eff[1].modifier = -skill / 5;                   /* Max -20 */
        eff[2].modifier = -skill / 5 * 2;                                 /* Max 40 */
        eff[0].duration = eff[1].duration = eff[2].duration = skill / 10; /* Max 10 */
        to_char = "&5You shout a powerful rebuke at $N, forcing $M to cower in fear!&0";
        to_vict = "&5You catch a glimpse of $n's true power and cower in fear!&0";
        to_room = "&5$N cowers in fear as $n rebukes $M.&0";
        break;

    case SPELL_REDUCE:

        if (IS_NPC(victim))
            return CAST_RESULT_CHARGE;

        eff[0].location = APPLY_SIZE;
        eff[0].modifier = -1;
        SET_FLAG(eff[0].flags, EFF_REDUCE);
        eff[0].duration = 1 + (skill / 40);

        eff[1].location = APPLY_CON;
        eff[1].modifier = -10;
        eff[1].duration = eff[0].duration;

        eff[2].location = APPLY_STR;
        eff[2].modifier = -10;
        eff[2].duration = eff[0].duration;

        refresh = true;
        to_vict =
            "&1&bYour skin starts to itch as you reduce to half your normal "
            "size.&0";
        to_room = "&1&b$N's skin ripples as $E shrinks to half $S normal size!&0";
        break;

    case SPELL_SANCTUARY:

        /* reimplemented by RLS back in 2k2.  While no class actually has this
           spell, there are god granted or quest items that do.
         */

        eff[0].duration = 4;
        SET_FLAG(eff[0].flags, EFF_SANCTUARY);
        to_vict = "This spell doesn't exist.  Ask no questions.";
        to_room = "Absolutely nothing happens to $N.";
        break;

    case SPELL_SENSE_LIFE:

        SET_FLAG(eff[0].flags, EFF_SENSE_LIFE);
        eff[0].duration = 17 + (skill / 3); /* max 50 */
        to_vict = "Your feel your awareness improve.";
        to_room = "$N seems more aware of $S surroundings.";
        break;

    case SPELL_SILENCE:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        if (MOB_FLAGGED(victim, MOB_NOSILENCE)) {
            send_to_char("You seem unable to silence this one.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }

        if (mag_savingthrow(victim, savetype)) {
            act("$N resists your pitiful attempt to silence $M.", false, ch, 0, victim, TO_CHAR);
            act("&7&b$n tries to silence you but fails!&0", false, ch, 0, victim, TO_VICT);
            act("&7&b$n squints at $N but nothing happens.&0", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        SET_FLAG(eff[0].flags, EFF_SILENCE);
        eff[0].duration = 2 + (skill / 15); /* max 8 */
        to_char = "You silence $N!";
        to_vict =
            "&9&bYour throat begins to close, sealing off all chance of "
            "communication.&0";
        to_room = "&0$N&7 squeaks as all sound is squelched from $S throat.&0";
        break;

    case SPELL_SLEEP:
        if (!sleep_allowed && ((!IS_NPC(ch) && ((!IS_NPC(victim) && ch != victim) ||
                                                /* victim is shapechanged? */
                                                (POSSESSED(victim) && GET_LEVEL(POSSESSOR(victim)) < 100))) ||
                               /* ch is shapechanged? */
                               (POSSESSED(ch) && GET_LEVEL(POSSESSOR(ch)) < 100))) {
            send_to_char("Use the 'nap' command instead!\r\n", ch);
            return CAST_RESULT_CHARGE;
        }

        /* Make it based on skill/saving throw -myc 17 Feb 2007 */
        if ((IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOSLEEP)) || mag_savingthrow(victim, SAVING_PARA) ||
            skill - GET_LEVEL(victim) < number(0, 100)) {
            act("$n can sing all $e wants, you aren't going to sleep.", false, ch, 0, victim, TO_VICT);
            act("$n tries to sing $N to sleep, but to no avail, uh oh.", true, ch, 0, victim, TO_NOTVICT);
            send_to_char(NOEFFECT, ch);
            if (!FIGHTING(victim)) {
                attack(victim, ch);
                if (IS_NPC(victim))
                    remember(victim, ch);
            }
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        SET_FLAG(eff[0].flags, EFF_SLEEP);
        eff[0].duration = 9 + (skill / 9); /* max 20 */
        refresh = false;

        if (GET_STANCE(victim) > STANCE_SLEEPING) {
            act("You feel very sleepy...  Zzzz......", false, victim, 0, 0, TO_CHAR);
            act("$n goes to sleep.", true, victim, 0, 0, TO_ROOM);
            GET_STANCE(victim) = STANCE_SLEEPING;
            GET_POS(victim) = POS_PRONE;
        }
        break;

    case SPELL_SMOKE:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        if (MOB_FLAGGED(victim, MOB_NOBLIND)) {
            act("&9&b$N&9&b resists your&9&b column of smoke!&0", false, ch, 0, 0, TO_CHAR);
            act("&9&bYou&9&b resist $n's&9&b column of smoke!&0", false, ch, 0, victim, TO_VICT);
            act("&9&b$N&9&b resists $n's&9&b column of smoke!&0", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        } else if (mag_savingthrow(victim, savetype)) {
            eff[0].location = APPLY_HITROLL;
            eff[0].modifier = -1;
            eff[0].duration = 0;
            eff[1].location = APPLY_AC;
            eff[1].modifier = -10;
            eff[1].duration = 0;

            to_char = "You temporarily choke $N with your column of smoke.";
            to_vict = "&9You have been temporarily choked by $n's&9&b column of smoke!&0";
            to_room = "&9&b$N&9&b is slightly choked by $n's&9&b column of smoke!&0";
        } else {
            eff[0].location = APPLY_HITROLL;
            eff[0].modifier = -4;
            eff[0].duration = 2;
            SET_FLAG(eff[0].flags, EFF_BLIND);
            eff[1].location = APPLY_AC;
            eff[1].modifier = -40;
            eff[1].duration = 2;
            SET_FLAG(eff[1].flags, EFF_BLIND);

            to_room = "&9&b$N&9&b is blinded by $n's&9&b column of smoke!&0";
            to_vict = "&9You have been blinded by $n's&9&b column of smoke&0";
            to_char = "&9&b$N&9&b is blinded by your column of smoke!&0";
        }
        break;

    case SPELL_SOULSHIELD:

        SET_FLAG(eff[0].flags, EFF_SOULSHIELD);
        eff[0].duration = 2 + (skill / 10); /* max 12 */
        refresh = false;

        if (IS_GOOD(victim)) {
            to_vict = "&3&bA bright golden aura surrounds your body!&0";
            to_room = "&3&bA bright golden aura surrounds $N's body!&0";
        } else if (IS_EVIL(victim)) {
            to_vict = "&1&bA &0&1dark red&b aura engulfs you!&0";
            to_room = "&1&bA &0&1dark red&b aura engulfs $N's body!&0";
        } else {
            act("A brief aura surrounds you, then fades.", false, ch, 0, victim, TO_VICT);
            act("A brief aura surrounds $n, then fades.", true, victim, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }
        break;

    case SPELL_STATUE:
        SET_FLAG(eff[0].flags, EFF_FAMILIARITY);
        eff[0].duration = (skill / 5) + (GET_INT(ch) / 4);
        to_vict = "&9&bYou disguise yourself as a little statue.&0";
        to_room = "You realize $N has disappeared and been replaced by a statue!";
        break;

    case SPELL_STONE_SKIN:

        /* what the hell does the modifier/location for this spell do? */
        /* It acts as a hit counter; it's decremented by decrease_modifier()
           everytime the victim is hit, and when it reaches 0, stone skin
           "wears off" before the duration is up. */

        eff[0].location = APPLY_NONE;
        eff[0].modifier = 7 + (skill / 16); /* max 13 */
        eff[0].duration = 2;
        SET_FLAG(eff[0].flags, EFF_STONE_SKIN);

        refresh = false;
        to_char = "&9&b$N's skin hardens and turns to stone!&0";
        to_vict = "&9&bYour skin hardens and turns to stone!&0";
        to_room = "&9&b$N's skin hardens and turns to stone!&0";
        break;

    case SPELL_SUNRAY:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        if (MOB_FLAGGED(victim, MOB_NOBLIND)) {
            send_to_char("You seem unable to blind this creature.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        if (mag_savingthrow(victim, savetype)) {
            act("$N resists your pitiful attempt to blind $M.", false, ch, 0, victim, TO_CHAR);
            act("&7&b$n tries to blind you but fails!&0", false, ch, 0, victim, TO_VICT);
            act("&7&b$n directs the rays of the sun at $N but nothing happens.&0", true, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        eff[0].location = APPLY_HITROLL;
        eff[0].modifier = -4;
        eff[0].duration = 2;
        SET_FLAG(eff[0].flags, EFF_BLIND);

        eff[1].location = APPLY_AC;
        eff[1].modifier = -40;
        eff[1].duration = 2;
        SET_FLAG(eff[1].flags, EFF_BLIND);

        to_char = "&9&bYou have blinded $N with your sunray!&0";
        to_room = "&9&b$N&9&b seems to be blinded!&0";
        to_vict = "&9&bYou have been blinded!&0";
        break;

    case SPELL_VAPORFORM:

        if (GET_COMPOSITION(victim) != COMP_FLESH) {
            send_to_char("Your body cannot sustain this change.\r\n", victim);
            return CAST_RESULT_CHARGE;
        }

        eff[0].location = APPLY_COMPOSITION;
        eff[0].modifier = COMP_MIST;
        eff[0].duration = 2 + (skill / 25); /* max 6 */
        refresh = false;
        to_vict = "&6&bYour body sublimates into a &7cloud &6of &7vapor&6.&0";
        to_room =
            "&6&b$N's body dematerializes into a translucent &7cloud &6of "
            "&7vapor&6!&0";
        break;

    case SPELL_WATERFORM:

        if (GET_COMPOSITION(victim) != COMP_FLESH) {
            send_to_char("Your body cannot sustain this change.\r\n", victim);
            return CAST_RESULT_CHARGE;
        }

        eff[0].location = APPLY_COMPOSITION;
        eff[0].modifier = COMP_WATER;
        eff[0].duration = 2 + (skill / 20); /* max 7 */
        refresh = false;
        to_vict = "&4&bYour body liquifies.&0";
        to_room =
            "&4&b$N&4&b's body wavers a bit, slowly changing into a "
            "&0&4liquid&b state!&0";
        break;

    case SPELL_WATERWALK:

        SET_FLAG(eff[0].flags, EFF_WATERWALK);
        eff[0].duration = 35 + (skill / 4); /* max 60 */
        to_room = "$N sprouts webbing between $S toes!";
        to_vict = "You feel webbing between your toes.";
        break;

    case SPELL_WEB:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;
        if ((skill + mag_savingthrow(victim, SAVING_PARA) - GET_LEVEL(victim)) <= number(0, 40)) {
            act("&2&bYou miss $N with a glowing &3&bweb&2&b!&0", false, ch, 0, victim, TO_CHAR);
            act("&2&b$n tries to tangle you in a glowing &3&bweb&2&b but misses!&0", false, ch, 0, victim, TO_VICT);
            act("&2&b$n throws a glowing &3&bweb&2&b at $N but misses.&0", true, ch, 0, victim, TO_NOTVICT);

            /* start combat for failure */
            if (!FIGHTING(victim)) {
                attack(victim, ch);
                remember(victim, ch);
            }

            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        SET_FLAG(eff[0].flags, EFF_IMMOBILIZED);
        eff[0].duration = 2 + (skill / 50); /* max 4 */
        refresh = false;
        to_char = "&2&bYou tangle $N in a glowing &3&bweb&2&b!&0";
        to_vict = "&2&b$n tangles you in a glowing &3&bweb&2&b!&0";
        to_room = "&2&b$n tangles $N in a glowing &3&bweb&2&b!&0";
        break;

    case SPELL_WINGS_OF_HEAVEN:

        if (affected_by_spell(victim, SPELL_WINGS_OF_HELL)) {
            send_to_char("You already have something sticking out of your back.\r\n", victim);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_FLY);
        eff[0].duration = 10 + (skill / 5); /* max 30 */

        to_vict =
            "&7&bBeautiful bright white wings unfurl behind you as you lift "
            "into the air.&0";
        to_room =
            "&7&bBeautiful bright white wings unfurl from $n's&7&b back, "
            "lifting $m into the air.&0";
        if (AWAKE(victim)) {
            GET_STANCE(victim) = STANCE_ALERT;
            GET_POS(victim) = POS_FLYING;
        }
        break;

    case SPELL_WINGS_OF_HELL:

        if (affected_by_spell(victim, SPELL_WINGS_OF_HEAVEN)) {
            send_to_char("You already have something sticking out of your back.\r\n", victim);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_FLY);
        eff[0].duration = 10 + (skill / 5);

        to_vict = "&1&bHuge leathery &9bat-like&1 wings sprout from your back.&0";
        to_room = "&1&bHuge leathery &9bat-like&1 wings sprout out of $n's&1&b back.&0";
        if (AWAKE(victim)) {
            GET_STANCE(victim) = STANCE_ALERT;
            GET_POS(victim) = POS_FLYING;
        }
        break;

        /* --- INNATES START HERE --- */

    case SPELL_INN_CHAZ:
        eff[0].location = APPLY_STR;
        eff[0].duration = (skill >> 1) + 4;
        eff[0].modifier = 1 + (skill / 18); /* max 6 */
        accum_effect = false;
        to_vict = "You feel stronger!";
        /* Innate chaz usage shouldn't call for a skill improvement in a spell
         * sphere */
        break;

    case SPELL_INN_SYLL:
        eff[0].location = APPLY_DEX;
        eff[0].duration = (skill >> 1) + 4;
        eff[0].modifier = 1 + (skill / 18); /* max 6 */
        accum_effect = false;
        to_vict = "You feel nimbler!";
        /* Innate syll usage shouldn't call for a skill improvement in a spell
         * sphere */
        break;

    case SPELL_INN_TASS:
        eff[0].location = APPLY_WIS;
        eff[0].duration = (skill >> 1) + 4;
        eff[0].modifier = 1 + (skill / 18); /* max 6 */
        accum_effect = false;
        to_vict = "You feel wiser!";
        /* Innate tass usage shouldn't call for a skill improvement in a spell
         * sphere */
        break;

    case SPELL_INN_BRILL:
        eff[0].location = APPLY_INT;
        eff[0].duration = (skill >> 1) + 4;
        eff[0].modifier = 1 + (skill / 18); /* max 6 */
        accum_effect = false;
        to_vict = "You feel smarter!";
        /* Innate brill usage shouldn't call for a skill improvement in a spell
         * sphere */
        break;

    case SPELL_INN_TREN:
        eff[0].location = APPLY_CON;
        eff[0].duration = (skill >> 1) + 4;
        eff[0].modifier = 1 + (skill / 18); /* max 6 */
        accum_effect = false;
        to_vict = "You feel healthier!";
        /* Innate tren usage shouldn't call for a skill improvement in a spell
         * sphere */
        break;

    case SPELL_INN_ASCEN:
        eff[0].location = APPLY_CHA;
        eff[0].duration = (skill >> 1) + 4;
        eff[0].modifier = 1 + (skill / 18); /* max 6 */
        accum_effect = false;
        to_vict = "You feel more resplendent!";
        /* Innate ascen usage shouldn't call for a skill improvement in a spell
         * sphere */
        break;

        /* --- CHANTS START HERE --- */

    case CHANT_ARIA_OF_DISSONANCE:

        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        eff[0].location = APPLY_AC;
        eff[0].duration = 5 + (skill / 30);  /* max 8 */
        eff[0].modifier = -10 - (skill / 2); /* max -60 */
        to_char = "Your song of dissonance confuses $N!";
        to_room = "$N winces as $n's dissonant song fills $S ears.";
        to_vict = "$n fills your ears with an aria of dissonance, causing confusion!";
        break;

    case CHANT_BATTLE_HYMN:
        eff[0].location = APPLY_HITROLL;
        eff[0].modifier = skill / 25 + 1;
        eff[0].duration = skill / 25 + 1;
        eff[1].location = APPLY_DAMROLL;
        eff[1].modifier = skill / 25 + 1;
        eff[1].duration = skill / 25 + 1;
        if (number(0, 1))
            to_vict = "Your heart beats with the rage of your fallen brothers.";
        else
            to_vict = "Your heart beats with the rage of your fallen sisters.";
        to_room = "$n's chest swells with courage!";
        break;

    case CHANT_INTERMINABLE_WRATH:
        SET_FLAG(eff[0].flags, EFF_WRATH);
        eff[0].duration = (skill / 20); /* max 5 */
        to_vict = "A feeling of unforgiving wrath fills you.";
        to_room = "$n bristles with anger.";
        break;

    case CHANT_SEED_OF_DESTRUCTION:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        /*
         * The modifier here makes the victim lose a percentage of
         * half their str and con based on the chanter's skill.
         */

        SET_FLAG(eff[0].flags, EFF_DISEASE);
        eff[0].location = APPLY_CON;
        eff[0].modifier = -(skill * GET_VIEWED_CON(victim) / 2) / 100;
        eff[0].duration = (skill / 20); /* max 5 */
        eff[1].location = APPLY_STR;
        eff[1].modifier = -(skill * GET_VIEWED_STR(victim) / 2) / 100;
        eff[1].duration = (skill / 20); /* max 5 */
        to_char = "You force $N down the path to destruction...";
        to_vict = "You feel your time in this world growing short...";
        to_room = "$n plants the seed of destruction in $N's mind.";
        break;

    case CHANT_REGENERATION:
        eff[0].location = APPLY_HIT_REGEN;
        eff[0].duration = skill / 2 + 3;
        eff[0].modifier = skill / 2 + 10;
        to_vict = "You feel your health improve.";
        to_room = "$n looks a little healthier.";
        break;

    case CHANT_SHADOWS_SORROW_SONG:
        /*
         * no attack_ok check, since this is a mag_masses spell, and
         * mag_masses won't pass to mag_affect if it doesn't pass attack_ok
         */

        /* No chance to fail, since chanting already has a built-in failure.
           if (mag_savingthrow(victim, savetype)) {
           send_to_char(NOEFFECT, ch);
           send_to_char("&7&bShadows start to creep across your vision, but you
           resist them.&0", victim); act("&7&b$n sings $N a song of depression, but
           $E ignores it.&0", true, ch, 0, victim, TO_NOTVICT); return
           CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
           }
         */

        eff[0].location = APPLY_HITROLL;
        eff[0].duration = 5 + (skill / 14);  /* max 12 */
        eff[0].modifier = -1 - (skill / 10); /* max -11 */
        accum_effect = true;
        to_char = "You depress $N with a song of darkness and sorrow!";
        to_room = "A dark look clouds $N's visage as $n sings $M a song of sorrow.";
        to_vict = "$n's song of sorrow fills your mind with darkness and shadows.";
        break;

    case CHANT_SONATA_OF_MALAISE:
        /*
         * no attack_ok check, since this is a MAG_MASSES spell, and
         * mag_masses checks attack_ok before calling mag_affect
         */
        eff[0].location = APPLY_SAVING_PARA;
        eff[1].location = APPLY_SAVING_ROD;
        eff[2].location = APPLY_SAVING_PETRI;
        eff[3].location = APPLY_SAVING_BREATH;
        eff[4].location = APPLY_SAVING_SPELL;
        /* same duration */
        eff[0].duration = eff[1].duration = eff[2].duration = eff[3].duration = eff[4].duration =
            1 + (skill / 20);                         /* max 6 */
        eff[0].modifier = (skill / 7) + number(0, 6); /* max 20 */
        eff[1].modifier = (skill / 7) + number(0, 6); /* max 20 */
        eff[2].modifier = (skill / 7) + number(0, 6); /* max 20 */
        eff[3].modifier = (skill / 7) + number(0, 6); /* max 20 */
        eff[4].modifier = (skill / 7) + number(0, 6); /* max 20 */
        to_char = "You fill $N with a sense of malaise!";
        to_vict = "Malaise fills the air, hampering your movements!";
        to_room = "$N contorts $S face briefly in anger and fear.";
        break;

    case SPELL_SPINECHILLER:
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;
        if (mag_savingthrow(victim, SAVING_PARA) || skill - GET_LEVEL(victim) < number(0, 70)) {
            act("$N resists your neuroparalysis.", false, ch, 0, victim, TO_CHAR);
            act("$n tries to scramble your nerves, but fails!", false, ch, 0, victim, TO_VICT);
            act("$n grabs onto $N and squeezes.", true, ch, 0, victim, TO_ROOM);

            /* start combat for failure */
            if (!FIGHTING(victim)) {
                attack(victim, ch);
                remember(victim, ch);
            }

            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        SET_FLAG(eff[0].flags, EFF_MINOR_PARALYSIS);
        eff[0].duration = 2 + (skill / 15); /* max 8 */
        refresh = false;
        to_char = "You grab $N and scramble $S nerves!";
        to_vict = "Tingles run up and down your spine as $n scrambles your nerves!";
        to_room = "$N gasps for breath as $n scrambles $S nerves!";
        break;

    case CHANT_SPIRIT_BEAR:
        SET_FLAG(eff[0].flags, EFF_SPIRIT_BEAR);
        eff[0].duration = (skill / 20); /* max 5 */
        to_vict = "The spirit of the bear consumes your body.";
        to_room = "$n shifts $s weight, seeming heavier and more dangerous.";
        break;

    case CHANT_SPIRIT_WOLF:
        SET_FLAG(eff[0].flags, EFF_SPIRIT_WOLF);
        eff[0].duration = (skill / 20); /* max 5 */
        to_vict = "You feel a wolf-like fury come over you.";
        to_room = "$n seems to take on a fearsome, wolf-like demeanor.";
        break;

    case CHANT_WAR_CRY:
        eff[0].location = APPLY_HITROLL;
        eff[0].modifier = skill / 25 + 1;
        eff[0].duration = skill / 25 + 1;
        eff[1].location = APPLY_DAMROLL;
        eff[1].modifier = skill / 25 + 1;
        eff[1].duration = skill / 25 + 1;
        to_vict = "You feel more determined than ever!";
        to_room = "$N looks more determined than ever!";
        break;

        /* --- SONGS START HERE --- */

    case SONG_INSPIRATION:
        eff[0].location = APPLY_HITROLL;
        eff[0].modifier = skill / (25 - (GET_CHA(ch) / 20)); /* Hitroll max 5 */
        eff[0].duration = skill / (15 - (GET_CHA(ch) / 20)); /* max 10 */
        eff[1].location = APPLY_DAMROLL;
        eff[1].modifier = skill / (25 - (GET_CHA(ch) / 20)); /* Damroll max 5 */
        eff[1].duration = skill / (15 - (GET_CHA(ch) / 20)); /* max 10 */
        if (GET_LEVEL(ch) >= 20) {
            eff[2].location = APPLY_SAVING_PARA;
            eff[3].location = APPLY_SAVING_ROD;
            eff[4].location = APPLY_SAVING_PETRI;
            eff[5].location = APPLY_SAVING_BREATH;
            eff[6].location = APPLY_SAVING_SPELL;
            eff[7].location = APPLY_AC;
            eff[2].duration = eff[3].duration = eff[4].duration = eff[5].duration = eff[6].duration = eff[7].duration =
                skill / (15 - (GET_CHA(ch) / 20));                          /* same duration, max 10 */
            eff[2].modifier = -(skill / 7) + number(0, (GET_CHA(ch) / 20)); /* Paralysis max -19 */
            eff[3].modifier = -(skill / 7) + number(0, (GET_CHA(ch) / 20)); /* Wand max -19 */
            eff[4].modifier = -(skill / 7) + number(0, (GET_CHA(ch) / 20)); /* Petrification max -19 */
            eff[5].modifier = -(skill / 7) + number(0, (GET_CHA(ch) / 20)); /* Breath max -19 */
            eff[6].modifier = -(skill / 7) + number(0, (GET_CHA(ch) / 20)); /* Spell max -19 */
            eff[7].modifier = (skill / 20) + (GET_CHA(ch) / 20);            /* AC max 10 */
            if (GET_LEVEL(ch) >= 40) {
                eff[8].location = APPLY_HIT;
                eff[8].modifier = (skill + (GET_CHA(ch)) * 2);       /* HP max 400 */
                eff[8].duration = skill / (15 - (GET_CHA(ch) / 20)); /* max 10 */
            }
        }
        act("You begin an inspiring performance!", false, ch, 0, victim, TO_CHAR);
        act("$n begins an inspiring performance!", false, ch, 0, victim, TO_NOTVICT);
        to_vict = "Your spirit swells with inspiration!";
        to_room = "$N's spirit stirs with inspiration!";
        break;

    case SONG_SONG_OF_REST: /* increases HP and MV recovery while sleeping, see limits.c */
        SET_FLAG(eff[0].flags, EFF_SONG_OF_REST);
        eff[0].duration = skill / (15 - (GET_CHA(ch) / 20)); /* max 10 */
        if (ch != victim) {
            to_char = "You sing $N a gentle lullaby to help $M rest.\r\n";
            to_room = "$n sings $N a gentle lullaby to help $M rest.";
            to_vict = "$n sings you a gentle lullaby to help you rest.";
        } else {
            to_vict = "You sing yourself a lullaby.\r\n";
            to_room = "$n sings a lullaby to $Mself.";
        }
        break;

    case SONG_TERROR:
        act("You perform a haunting melody!", false, ch, 0, victim, TO_CHAR);
        act("$n performs a haunting melody!", false, ch, 0, victim, TO_NOTVICT);
    case SONG_BALLAD_OF_TEARS:
        eff[0].location = APPLY_SAVING_PARA;
        eff[1].location = APPLY_SAVING_ROD;
        eff[2].location = APPLY_SAVING_SPELL;
        eff[3].location = APPLY_AC;
        eff[0].duration = eff[1].duration = eff[2].duration = eff[3].duration =
            skill / (15 - (GET_CHA(ch) / 10));                           /* same duration, max 20 */
        eff[0].modifier = ((skill / 7) + number(0, (GET_CHA(ch) / 20))); /* Paralysis max 19 */
        eff[1].modifier = ((skill / 7) + number(0, (GET_CHA(ch) / 20))); /* Rod max 19 */
        eff[2].modifier = ((skill / 7) + number(0, (GET_CHA(ch) / 20))); /* Spell max 19 */
        eff[3].modifier = -(5 + ((skill / 10) - (GET_CHA(ch) / 20)));    /* AC max -10 */
        if (GET_LEVEL(ch) >= 30) {
            eff[4].location = APPLY_CON;
            eff[4].modifier = -(((skill + GET_CHA(ch)) / 4) * (GET_VIEWED_CON(victim) / 2)) / 100; /* Con max -25 */
            eff[4].duration = skill / (15 - (GET_CHA(ch) / 10));                                   /* max 20 */
            eff[5].location = APPLY_STR;
            eff[5].modifier = -(((skill + GET_CHA(ch)) / 4) * (GET_VIEWED_STR(victim) / 2)) / 100; /* Str max -25 */
            eff[5].duration = skill / (15 - (GET_CHA(ch) / 10));                                   /* max 20 */
            if (GET_LEVEL(ch) >= 50) {
                eff[6].location = APPLY_HITROLL;
                eff[6].modifier = -(skill / (15 - (GET_CHA(ch) / 20))); /* Hitroll max -10 */
                eff[6].duration = skill / (15 - (GET_CHA(ch) / 10));    /* max 20 */
                eff[7].location = APPLY_DAMROLL;
                eff[7].modifier = -(skill / (15 - (GET_CHA(ch) / 20))); /* Damroll max -10 */
                eff[7].duration = skill / (15 - (GET_CHA(ch) / 10));    /* max 20 */
            }
        }
        to_vict = "Your spirit withers in terror and sorrow!";
        to_room = "$N's spirit withers in terror and sorrow!";
        break;

    } /* <--- end of switch of spells */

    /*
     * If this is a mob that has this effect set in its mob file, do not
     * perform the affect.  This prevents people from un-sancting mobs
     * by sancting them and waiting for it to fade, for example.
     */
    if (IS_NPC(victim) && !affected_by_spell(victim, spellnum))
        for (i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (ANY_FLAGGED(EFF_FLAGS(victim), eff[i].flags, NUM_EFF_FLAGS)) {
                send_to_char(NOEFFECT, ch);
                return CAST_RESULT_CHARGE;
            }

    /*
     * If the victim is already affected by this spell, and the spell does
     * not have a cumulative effect, then fail the spell.
     */
    if (affected_by_spell(victim, spellnum) && !(accum_duration || accum_effect || refresh)) {
        send_to_char(NOEFFECT, ch);
        return CAST_RESULT_CHARGE;
    }

    for (effect = victim->effects; effect && !is_innate; effect = effect->next) {
        if (spellnum == effect->type && effect->duration == -1)
            is_innate = true;
    }

    if (affected_by_spell(victim, spellnum) && is_innate) {
        send_to_char(NOEFFECT, ch);
        return CAST_RESULT_CHARGE;
    }

    /* act() for the message buffers in mag_affect() */

    if (!affected_by_spell(victim, spellnum)) {
        /* Suppress this message when you are casting the spell on yourself. */
        if (to_char != nullptr && ch != victim)
            act(to_char, false, ch, 0, victim, TO_CHAR);
        if (to_vict != nullptr)
            act(to_vict, false, ch, 0, victim, TO_VICT);
        if (to_room != nullptr) {
            act(to_room, true, ch, 0, victim, TO_NOTVICT);
            if (to_char == nullptr && ch != victim)
                act(to_room, false, ch, 0, victim, TO_CHAR);
        }
    }

    for (i = 0; i < MAX_SPELL_EFFECTS; i++)
        if (HAS_FLAGS(eff[i].flags, NUM_EFF_FLAGS) || eff[i].location != APPLY_NONE) {
            effect_join(victim, eff + i, accum_duration, false, accum_effect, false, refresh);
            if (CASTING(victim)) {
                if (IS_FLAGGED(eff[i].flags, EFF_SILENCE)) {
                    STOP_CASTING(victim);
                    act("Your spell collapses.", false, victim, 0, 0, TO_CHAR);
                    act("$n continues silently moving $s lips for a moment before giving "
                        "up.",
                        false, victim, 0, 0, TO_ROOM);
                } else if (IS_FLAGGED(eff[i].flags, EFF_MINOR_PARALYSIS) ||
                           IS_FLAGGED(eff[i].flags, EFF_MAJOR_PARALYSIS) || IS_FLAGGED(eff[i].flags, EFF_MESMERIZED))
                    /* Just silently stop the casting for paralysis */
                    STOP_CASTING(victim);
            }
            if ((IS_FLAGGED(eff[i].flags, EFF_MINOR_PARALYSIS) || IS_FLAGGED(eff[i].flags, EFF_MAJOR_PARALYSIS) ||
                 IS_FLAGGED(eff[i].flags, EFF_MESMERIZED))) {
                if (FIGHTING(victim))
                    stop_fighting(victim);
                stop_attackers(victim);
            }
        }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/*
 * This function is used to provide services to mag_group.  This function
 * is the one you should change to add new group spells.
 */

void perform_mag_group(int skill, CharData *ch, CharData *tch, int spellnum, int savetype) {
    switch (spellnum) {
    case SPELL_DIVINE_ESSENCE:
        mag_affect(skill, ch, tch, SPELL_GREATER_ENDURANCE, savetype, CAST_SPELL);
        mag_affect(skill, ch, tch, SPELL_BLESS, savetype, CAST_SPELL);
        break;
    case SONG_FREEDOM_SONG:
        mag_unaffect(skill, ch, tch, SONG_FREEDOM_SONG, savetype);
        break;
    case SPELL_GROUP_ARMOR:
        mag_affect(skill, ch, tch, SPELL_ARMOR, savetype, CAST_SPELL);
        break;
    case SPELL_GROUP_HEAL:
        mag_point(skill, ch, tch, SPELL_HEAL, savetype);
        mag_unaffect(skill, ch, tch, SPELL_HEAL, savetype);
        break;
    case SPELL_GROUP_RECALL:
        spell_recall(spellnum, skill, ch, tch, nullptr, savetype);
        break;
    case SONG_HEARTHSONG:
        mag_affect(skill, ch, tch, SPELL_FAMILIARITY, savetype, CAST_PERFORM);
        break;
    case SONG_HEROIC_JOURNEY:
        mag_affect(skill, ch, tch, SONG_INSPIRATION, savetype, CAST_SPELL);
        break;
    case SPELL_INVIGORATE:
        mag_point(skill, ch, tch, SPELL_INVIGORATE, savetype);
        break;
    case CHANT_WAR_CRY:
        mag_affect(skill, ch, tch, CHANT_WAR_CRY, savetype, CAST_SPELL);
        break;
    }
}

/*
 * Every spell that affects the group should run through here
 * perform_mag_group contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_group.
 * You can add a message to the switch in mag_groups though.
 *
 * Return value: CAST_RESULT_ flags
 */

int mag_group(int skill, CharData *ch, int spellnum, int savetype) {
    CharData *tch, *next_tch;
    char *to_room, *to_char;

    if (ch == nullptr)
        return 0;

    if (!IS_GROUPED(ch))
        return CAST_RESULT_CHARGE;

    switch (spellnum) {
    case SPELL_DIVINE_ESSENCE:
        to_room = "&3&b$n&3&b invokes $s deity's divine essence to fill the area!&0";
        to_char = "&3&bYou invoke your deity's divine essence!&0\r\n";
        break;
    case SONG_FREEDOM_SONG:
        to_room = "&7&b$n&7&b performs a song to break the chains that bind!&0";
        to_char = "&7&bYou perform a song to break the chains that bind!&0\r\n";
        break;
    case SONG_HEARTHSONG:
        to_room = "&3&b$n&3&b deepens the bonds of community and fellowship amongst you.&0";
        to_char = "&3&bYou deepen the bonds of community and fellowship with your group.&0\r\n";
        break;
    case SONG_HEROIC_JOURNEY:
        to_room = "&6&b$n&6&b spins a tale of great triumph and adventure for your party!&0";
        to_char = "&6&bYou spin a tale of great triumph and adventure for your party!&0\r\n";
        break;
    default:
        to_room = nullptr;
        to_char = nullptr;
    }
    if (to_room)
        act(to_room, true, ch, 0, 0, TO_ROOM);
    if (to_char)
        send_to_char(to_char, ch);

    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
        /* I suppose it's possible a heal could kill someone. */
        next_tch = tch->next_in_room;

        if (tch->in_room != ch->in_room)
            continue;

        if (ch != tch && is_grouped(ch, tch))
            perform_mag_group(skill, ch, tch, spellnum, savetype);
    }

    /* Always hit the caster last for spells like group recall, which
       will otherwise fail if the caster leaves the room partway
       through the spell */
    perform_mag_group(skill, ch, ch, spellnum, savetype);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/*
 * Mass spells affect every creature in the room except the caster.
 * These are often offensive spells.  This calls mag_affect to do the
 * actual affection -- all spells listed here must also have a case in
 * mag_affect() in order for them to work.
 */

int mag_mass(int skill, CharData *ch, int spellnum, int savetype) {
    CharData *tch, *tch_next;
    bool found = false;

    if (ch == nullptr)
        return 0;

    if (ch->in_room == NOWHERE)
        return CAST_RESULT_CHARGE;

    for (tch = world[ch->in_room].people; tch; tch = tch_next) {
        tch_next = tch->next_in_room;

        if (SINFO.violent) {
            if (tch == ch)
                continue;
            if (!mass_attack_ok(ch, tch, false))
                continue;
            if (is_grouped(ch, tch))
                continue;
        }

        found = true;
        mag_affect(skill, ch, tch, spellnum, savetype, CAST_SPELL);

        /*
         * If this is a violent spell, and the victim isn't already fighting
         * someone, then make it attack the caster.
         */
        if (SINFO.violent && !FIGHTING(tch))
            set_fighting(tch, ch, false);
    }

    /* No skill improvement if there weren't any valid targets in the room. */
    if (!found)
        return CAST_RESULT_CHARGE;

    return CAST_RESULT_IMPROVE | CAST_RESULT_CHARGE;
}

int mag_bulk_objs(int skill, CharData *ch, int spellnum, int savetype) {
    ObjData *tobj, *tobj_next;
    bool found = false;

    if (ch == nullptr)
        return 0;

    if (ch->in_room == NOWHERE)
        return CAST_RESULT_CHARGE;

    for (tobj = world[ch->in_room].contents; tobj; tobj = tobj_next) {
        tobj_next = tobj->next_content;

        found = true;
        mag_alter_obj(skill, ch, tobj, spellnum, savetype);
    }

    /* No skill improvement if there weren't any valid targets. */
    if (!found)
        return CAST_RESULT_CHARGE;
    return CAST_RESULT_IMPROVE | CAST_RESULT_CHARGE;
}

/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
 *
 * Return value: CAST_RESULT_ flags
 */

int mag_area(int skill, CharData *ch, int spellnum, int savetype) {
    CharData *tch, *next_tch;
    char *to_char = nullptr;
    char *to_room = nullptr;
    int casttype = nullptr;
    bool found = false;
    bool damage = true;

    if (ch == nullptr)
        return 0;

    if (ch->in_room == NOWHERE)
        return CAST_RESULT_CHARGE;

    /*
     * to add spells to this fn, just add the message here plus an entry
     * in mag_damage for the damaging part of the spell.
     */
    switch (spellnum) {
    case SONG_BALLAD_OF_TEARS:
        to_char = "&9&bYou weave a tale of suffering and misery!&0";
        to_room = "&9&b$n&9&b weaves a tale of suffering and misery!&0";
        damage = false;
        break;
    case SPELL_BLINDING_BEAUTY:
        to_char = "&3&bThe splendor of your beauty sears the eyes of everything around you!&0";
        to_room = "&3&bThe splendor of $s's beauty sears the eyes of everything around $s!&0";
        damage = false;
        break;
    case SPELL_CHAIN_LIGHTNING:
        to_char = "&4&bYou send powerful bolts of lightning from your body...&0";
        to_room = "&4&b$n&4&b sends powerful bolts of lightning into $s foes...&0";
        break;
    case SPELL_CIRCLE_OF_DEATH:
        to_char = "&9&bWaves of death energy wash outward from you in a violent circle.&0";
        to_room = "&9&b$n&9&b sends waves of death energy outward in a violent circle.&0";
        break;
    case SPELL_CREMATE:
        to_char = "&1&8You raise up a huge conflaguration in the area.&0";
        to_room = "&1&8$n summons a huge conflagration burning through the area.&0";
        break;
    case SONG_CROWN_OF_MADNESS:
        to_char = "&2&bYou rend the sanity of those observing you with a tale of the Old Gods!&0";
        to_room = "&2&b$n&2&b rends the sanity of those watching with a tale of the Old Gods!&0";
        damage = false;
        break;
    case SPELL_EARTHQUAKE:
        switch (SECT(IN_ROOM(ch))) {
        case SECT_SHALLOWS:
        case SECT_WATER:
        case SECT_UNDERWATER:
        case SECT_AIR:
        case SECT_AIRPLANE:
        case SECT_ASTRALPLANE:
        case SECT_AVERNUS:
            act("Quake the earth? What earth? There's no ground here anywhere.", false, ch, 0, 0, TO_CHAR);
            act("$n hunches over and grunts loudly!", true, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }
        to_char = "&3You gesture and the earth begins to shake all around you!&0";
        to_room = "$n&3 gracefully gestures and the earth begins to shake violently!&0";
        send_to_zone("&3The ground &1rumbles&3 and shakes!&0\r\n", IN_ZONE_VNUM(ch), IN_ROOM(ch), STANCE_SLEEPING);
        break;
    case SKILL_ELECTRIFY:
        to_char = "&4&8You send out electricity in all directions...&0";
        to_room = "&4&8$n&4&8 sends out electricity in all directions...&0";
        break;
    case SONG_ENRAPTURE:
        to_char = "&5&bYou unleash a grand illusory performance!&0";
        to_room = "&5&b$n unleashes a grand illusory performance!&0";
        damage = false;
        break;
    case SPELL_FIRESTORM:
        to_char = "You conjure a gout of flame to sweep through the area.";
        to_room = "$n waves his hands as a gout of flame floods into the area.";
        break;
    case SPELL_FREEZING_WIND:
        to_char = "&6&bYou release a &0&4chilling&6&b stream of air at your foes.&0";
        to_room = "&6&b$n&6&b releases a &0&4chilling&6&b stream of air.&0";
        break;
    case SPELL_ICE_SHARDS:
        to_char =
            "&6&bYou conjure thousands of razor sharp ice shards to massacre "
            "your foe!&0";
        to_room =
            "$n &6&bconjures thousands of razor sharp ice shards to massacre "
            "$s foe!&0";
        break;
    case SPELL_HELLFIRE_BRIMSTONE:
        to_char =
            "&1A large hole opens in the &3earth&1 nearby spouting a gout of "
            "&bhellfire&0&1 and &9&bbrimstone!&0";
        to_room =
            "&1A large hole opens in the &3earth&1 nearby spouting a gout of "
            "&bhellfire&0&1 and &9&bbrimstone!&0";
        break;
    case SPELL_HOLY_WORD:
        to_char = "&7&bYou invoke a word of holy power!&0";
        to_room = "&7&b$n &7&butters a word of holy power!&0";
        break;
    case SPELL_ICE_STORM:
        to_char = "&6&bYou crush your foes under a relentless ice storm!&0";
        to_room = "$n&6&b crushes $s foes under a relentless ice storm!&0";
        break;
    case SPELL_INCENDIARY_NEBULA:
        to_char = "&0&2You cackle as your &bwave of gases&0 &2torches your enemies!&0";
        to_room = "&0$n&2 cackles as his &bwave of gases&0&2 torches his enemies!&0";
        break;
    case SPELL_METEORSWARM:
        to_char = "&1You conjure up a controlled shower of meteors to crush your foes!&0";
        to_room = "$n&1 conjures a controlled shower of flaming meteors!&0";
        break;
    case SPELL_SEVERANCE:
        to_char =
            "&7You lay a fell glow upon the surroundings, revealing the "
            "&bsilver cords&0&7 of all present.&0";
        to_room =
            "&7$n&0&7 spreads a &6fell glow&7 upon the surroundsings, "
            "revealing the &bsilver cords&0 of all present.&0";
        break;
    case SPELL_SOUL_REAVER:
        to_char = "&5You shape an awesome vision of calamity for all...&0";
        to_room = "&5$n&0&5 warps all reality into something deadly sinister...&0";
        break;
    case SPELL_SUPERNOVA:
        to_char =
            "&1You release a &bconflagration&0&1 of f&3&bi&1r&0&1e, laying "
            "waste to all that surrounds you.&0";
        to_room =
            "&1&b$n&1&b EXPLODES,&0&1 releasing a &bconflagration&0&1 of "
            "f&3&bi&1r&0&1e that lays waste to the entire area!&0";
        break;
    case SPELL_UNHOLY_WORD:
        to_char = "&9&bYou invoke a word of unholy power!&0";
        to_room = "&9&b$n &9&butters a word of unholy power!&0";
        break;
    }

    if (to_char != nullptr)
        act(to_char, false, ch, 0, 0, TO_CHAR);
    if (to_room != nullptr)
        act(to_room, false, ch, 0, 0, TO_ROOM);

    if ((spellnum == SPELL_UNHOLY_WORD && IS_GOOD(ch)) || (spellnum == SPELL_HOLY_WORD && IS_EVIL(ch))) {
        act("&9&bYour word of power is the last thing you hear as your soul is "
            "ripped apart!&0",
            false, ch, 0, 0, TO_CHAR);
        die(ch, ch);
        return CAST_RESULT_CHARGE;
    }

    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;
        /* Standard room attack check */
        if (!area_attack_target(ch, tch))
            continue;
        if (spellnum == SPELL_UNHOLY_WORD && !IS_GOOD(tch))
            continue;
        if (spellnum == SPELL_HOLY_WORD && !IS_EVIL(tch))
            continue;

        found = true;
        if (damage == true)
            mag_damage(skill, ch, tch, spellnum, savetype);
        else
            mag_affect(skill, ch, tch, spellnum, savetype, casttype);
    }
    /* No skill improvement if there weren't any valid targets. */
    if (!found)
        return CAST_RESULT_CHARGE;
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/* a few helpers for mag_summons */

enum undead_type { MOB_ZOMBIE, MOB_SKELETON, MOB_SPECTRE, MOB_WRAITH, MOB_LICH };

/* mod_for_undead_type takes a mob and modifies its stats/skills/etc as
 * as appropriate for an undead of the specified type */
void mod_for_undead_type(CharData *mob, enum undead_type type) {
    if (!mob)
        return;

    GET_LIFEFORCE(mob) = LIFE_UNDEAD;
    GET_ALIGNMENT(mob) = -1000;
    GET_RACE_ALIGN(mob) = RACE_ALIGN_EVIL;
    REMOVE_FLAG(MOB_FLAGS(mob), MOB_AGGRESSIVE);
    REMOVE_FLAG(MOB_FLAGS(mob), MOB_AGGR_EVIL);
    REMOVE_FLAG(MOB_FLAGS(mob), MOB_AGGR_GOOD);
    REMOVE_FLAG(MOB_FLAGS(mob), MOB_AGGR_NEUTRAL);
    REMOVE_FLAG(MOB_FLAGS(mob), MOB_AGGR_EVIL_RACE);
    REMOVE_FLAG(MOB_FLAGS(mob), MOB_AGGR_GOOD_RACE);
    SET_FLAG(MOB_FLAGS(mob), MOB_ANIMATED);

    switch (type) {
    case MOB_ZOMBIE:
        GET_COMPOSITION(mob) = COMP_FLESH;
        GET_CLASS(mob) = CLASS_WARRIOR;
        GET_HITROLL(mob) *= 1.1;
        GET_DAMROLL(mob) *= 1.1;
        GET_MAX_HIT(mob) *= 1.1;
        GET_MAX_MOVE(mob) *= 1.1;
        GET_AC(mob) -= 10;
        GET_NATURAL_STR(mob) *= 1.2;
        GET_NATURAL_INT(mob) *= 0.5;
        GET_NATURAL_WIS(mob) *= 0.75;
        GET_NATURAL_DEX(mob) *= 0.75;
        GET_NATURAL_CON(mob) *= 1.2;
        GET_NATURAL_CHA(mob) *= 0.5;
        break;
    case MOB_SKELETON:
        GET_COMPOSITION(mob) = COMP_BONE;
        GET_CLASS(mob) = CLASS_WARRIOR;
        GET_HITROLL(mob) *= 1.2;
        GET_DAMROLL(mob) *= 1.2;
        GET_MAX_HIT(mob) *= 1.2;
        GET_MAX_MOVE(mob) *= 1.2;
        GET_AC(mob) -= 15;
        GET_NATURAL_STR(mob) *= 1.2;
        GET_NATURAL_INT(mob) *= 0.75;
        GET_NATURAL_WIS(mob) *= 0.9;
        GET_NATURAL_DEX(mob) *= 1.0;
        GET_NATURAL_CON(mob) *= 1.2;
        GET_NATURAL_CHA(mob) *= 0.75;
        break;
    case MOB_SPECTRE:
        GET_COMPOSITION(mob) = COMP_ETHER;
        GET_CLASS(mob) = CLASS_ASSASSIN;
        GET_HITROLL(mob) *= 1.4;
        GET_DAMROLL(mob) *= 1.4;
        GET_MAX_HIT(mob) *= 1.4;
        GET_MAX_MOVE(mob) *= 1.4;
        GET_AC(mob) -= 20;
        GET_NATURAL_STR(mob) *= 1.2;
        GET_NATURAL_INT(mob) *= 1.0;
        GET_NATURAL_WIS(mob) *= 1.0;
        GET_NATURAL_DEX(mob) *= 1.2;
        GET_NATURAL_CON(mob) *= 1.2;
        GET_NATURAL_CHA(mob) *= 1.0;
        break;
    case MOB_WRAITH:
        GET_COMPOSITION(mob) = COMP_ETHER;
        GET_CLASS(mob) = CLASS_SORCERER;
        GET_HITROLL(mob) *= 1.6;
        GET_DAMROLL(mob) *= 1.6;
        GET_MAX_HIT(mob) *= 1.6;
        GET_MAX_MOVE(mob) *= 1.6;
        GET_AC(mob) -= 30;
        GET_NATURAL_STR(mob) *= 1.3;
        GET_NATURAL_INT(mob) *= 1.3;
        GET_NATURAL_WIS(mob) *= 1.3;
        GET_NATURAL_DEX(mob) *= 1.3;
        GET_NATURAL_CON(mob) *= 1.3;
        GET_NATURAL_CHA(mob) *= 1.3;
        break;
    case MOB_LICH:
        GET_COMPOSITION(mob) = COMP_FLESH;
        GET_CLASS(mob) = CLASS_NECROMANCER;
        GET_HITROLL(mob) *= 2;
        GET_DAMROLL(mob) *= 2;
        GET_MAX_HIT(mob) *= 2;
        GET_MAX_MOVE(mob) *= 2;
        GET_AC(mob) -= 50;
        GET_NATURAL_STR(mob) *= 1.5;
        GET_NATURAL_INT(mob) *= 1.5;
        GET_NATURAL_WIS(mob) *= 1.5;
        GET_NATURAL_DEX(mob) *= 1.5;
        GET_NATURAL_CON(mob) *= 1.5;
        GET_NATURAL_CHA(mob) *= 1.5;
        break;
    }
    if (GET_NATURAL_STR(mob) > 100)
        GET_NATURAL_STR(mob) = 100;
    if (GET_NATURAL_INT(mob) > 100)
        GET_NATURAL_INT(mob) = 100;
    if (GET_NATURAL_WIS(mob) > 100)
        GET_NATURAL_WIS(mob) = 100;
    if (GET_NATURAL_DEX(mob) > 100)
        GET_NATURAL_DEX(mob) = 100;
    if (GET_NATURAL_CON(mob) > 100)
        GET_NATURAL_CON(mob) = 100;
    if (GET_NATURAL_CHA(mob) > 100)
        GET_NATURAL_CHA(mob) = 100;
}

/* mod_for_lvldiff takes a mob that is being summoned/raised/etc by ch and
 * modifies its stats appropriately (up for higher-level ch, down for
 * lower-level ch). if ch or mob is NULL, it returns without effect. */
void mod_for_lvldiff(CharData *mob, CharData *ch) {
    float mult;

    if (!mob || !ch)
        return;

    mult = (float)GET_LEVEL(ch) / (float)GET_LEVEL(mob);
    if (mult > 2.0)
        mult = 2.0;

    GET_MAX_HIT(mob) *= mult;
    GET_MAX_MOVE(mob) *= mult;
}

/* new fn create_undead() added to help separate the logic of creating the
 * critter from that of actually summoning the thing -- 321 */
/* orig is the original (living) mob.
 * caster is the caster of the animate spell to tell us how to modify the
 *   new mob's stats, or NULL to skip modifying undead stats */
CharData *create_undead(CharData *orig, CharData *caster, bool ISPC) {
    char short_buf[160], long_buf[160], alias_buf[160];
    CharData *new_mob, *next_mob;
    enum undead_type new_mob_type;

    extern PlayerSpecialData dummy_mob;
    extern void roll_natural_abils(CharData *);
    extern void assign_triggers(void *, int);

    if ((GET_LEVEL(orig) > 94) && IS_MAGIC_USER(orig) && !number(0, 250))
        new_mob_type = MOB_LICH;
    else if ((GET_LEVEL(orig) > 40) && IS_MAGIC_USER(orig) && !number(0, 6))
        new_mob_type = MOB_WRAITH;
    else if ((GET_LEVEL(orig) > 25) && IS_ROGUE(orig) && !number(0, 4))
        new_mob_type = MOB_SPECTRE;
    else if (!number(0, 2))
        new_mob_type = MOB_SKELETON;
    else
        new_mob_type = MOB_ZOMBIE;

    new_mob = create_char();

    next_mob = new_mob->next; /* it's about to get overwritten */
    *new_mob = *orig;
    new_mob->next = next_mob; /* put it back */
    new_mob->player_specials = &dummy_mob;

    /* make sure it has no money in case the proto does */
    GET_PLATINUM(new_mob) = 0;
    GET_GOLD(new_mob) = 0;
    GET_SILVER(new_mob) = 0;
    GET_COPPER(new_mob) = 0;

    if (ISPC) {
        sprintf(short_buf, "a rotting, fetid zombie");
        sprintf(long_buf, "A rotting, fetid zombie lurches here.");
        sprintf(alias_buf, "zombie animate dead");
    } else {
        switch (new_mob_type) {
        case MOB_ZOMBIE:
            sprintf(short_buf, "the zombie of %s", GET_NAME(orig));
            sprintf(long_buf, "The zombie of %s lurches here.", GET_NAME(orig));
            sprintf(alias_buf, "zombie %s", GET_NAMELIST(orig));
            break;
        case MOB_SKELETON:
            sprintf(short_buf, "the skeleton of %s", GET_NAME(orig));
            sprintf(long_buf, "The skeleton of %s stands creaking at attention.", GET_NAME(orig));
            sprintf(alias_buf, "skeleton %s", GET_NAMELIST(orig));
            break;
        case MOB_SPECTRE:
            strcpy(short_buf, "a spectre");
            strcpy(long_buf, "A spectre lurks in the corner here.");
            strcpy(alias_buf, "spectre");
            break;
        case MOB_WRAITH:
            strcpy(short_buf, "a wraith");
            strcpy(long_buf, "A wraith floats above the ground moaning.");
            strcpy(alias_buf, "wraith");
            break;
        case MOB_LICH:
            strcpy(short_buf, "a lich");
            strcpy(long_buf, "A lich glares around the room sneering maliciously.");
            strcpy(alias_buf, "lich");
            break;
        }
    }

    strcat(long_buf, "\r\n");
    GET_NAME(new_mob) = strdup(short_buf);
    new_mob->player.long_descr = strdup(long_buf);
    GET_NAMELIST(new_mob) = strdup(alias_buf);

    roll_natural_abils(new_mob);

    /* this bit's taken straight out of read_mobile. i'm assuming it's good
     * *gulp* */
    new_mob->actual_abils = new_mob->natural_abils;

    scale_attribs(new_mob);

    /* Set up skills, innates, etc. */
    update_char(new_mob);

    /* New Points Gen to make for lower values = to around 2x the caster's */
    new_mob->points.max_hit = ((caster->points.max_hit * 21) / 10);

    mod_for_undead_type(new_mob, new_mob_type);
    mod_for_lvldiff(new_mob, caster);

    new_mob->points.move = new_mob->points.max_move;
    new_mob->points.hit = new_mob->points.max_hit;

    new_mob->player.time.birth = time(0);
    new_mob->player.time.played = 0;
    new_mob->player.time.logon = time(0);

    assign_triggers(new_mob, 0);
    /* no more exp for killing your raised dead */
    GET_EXP(new_mob) = 0;
    SET_FLAG(MOB_FLAGS(new_mob), MOB_NOSCRIPT); /* Prevent specprocs and triggers */

    return new_mob;
}

/* new fn ch_can_control_mob added as a helper to mag_summons to determine if
 * ch can successfully summon and control a mob. returns CONTROL_YES if ch
 * can summon and control mob, CONTROL_NO if ch can't control it, and
 * CONTROL_HELLNO if ch couldn't even control it under the /best/ of
 * circumstances - 321 */

enum { CONTROL_HELLNO, CONTROL_NO, CONTROL_YES };

#define ATTRIB_MULT(attr) ((float)(attr) / 250.0 + 0.8)
#define GET_CONTROL_VAL(ch)                                                                                            \
    ((float)(GET_LEVEL(ch)) * (ATTRIB_MULT(GET_INT(ch))) * (ATTRIB_MULT(GET_WIS(ch))) * (ATTRIB_MULT(GET_CHA(ch))))

#define MAX_PETS 8

int ch_can_control_mob(CharData *ch, CharData *mob) {
    int max_single_control, max_total_control, current_control = 0;
    FollowType *foll;
    int num_pets = 0;

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return CONTROL_YES;

    max_single_control = (LVL_IMMORT - GET_LEVEL(ch)) / 2 + GET_LEVEL(ch);
    if (GET_LEVEL(ch) * 2 < max_single_control)
        max_single_control = GET_LEVEL(ch) * 2;
    if (GET_CONTROL_VAL(mob) > max_single_control)
        return CONTROL_HELLNO;

    max_total_control = GET_CONTROL_VAL(ch);
    for (foll = ch->followers; foll; foll = foll->next)
        if (EFF_FLAGGED(foll->follower, EFF_CHARM)) {
            current_control += GET_CONTROL_VAL(foll->follower);
            num_pets++;
        }
    if ((current_control + GET_CONTROL_VAL(mob) > max_total_control) || (num_pets >= MAX_PETS))
        return CONTROL_NO;

    return CONTROL_YES;
}

CharData *load_summoned_mob(int vnum, int destroom) {
    CharData *mob;
    int r_num;
    if ((r_num = real_mobile(vnum)) < 0) {
        sprintf(buf, "SYSERR: tried to summon mob with nonexistent vnum %d", vnum);
        log(buf);
        return nullptr;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, destroom);
    GET_EXP(mob) = 0;
    GET_PLATINUM(mob) = 0;
    GET_GOLD(mob) = 0;
    GET_SILVER(mob) = 0;
    GET_COPPER(mob) = 0;

    REMOVE_FLAG(MOB_FLAGS(mob), MOB_HELPER);
    REMOVE_FLAG(MOB_FLAGS(mob), MOB_NOSUMMON);
    REMOVE_FLAG(MOB_FLAGS(mob), MOB_PEACEFUL);
    REMOVE_FLAG(MOB_FLAGS(mob), MOB_PEACEKEEPER);
    REMOVE_FLAG(MOB_FLAGS(mob), MOB_PROTECTOR);
    return mob;
}

CharData *duplicate_char(CharData *model, int destroom) {
    CharData *new_mob;

    if (GET_MOB_VNUM(model) > 0)
        new_mob = load_summoned_mob(GET_MOB_VNUM(model), destroom);
    else
        new_mob = load_summoned_mob(1, destroom);
    if (!new_mob)
        return 0;

    new_mob->natural_abils = model->natural_abils;
    new_mob->actual_abils = model->actual_abils;
    new_mob->affected_abils = model->affected_abils;
    new_mob->points = model->points;

    /*  char_player_data */
    if (model->player.namelist)
        new_mob->player.namelist = strdup(model->player.namelist);

    if (model->player.short_descr)
        new_mob->player.short_descr = strdup(model->player.short_descr);
    if (model->player.long_descr)
        new_mob->player.long_descr = strdup(model->player.long_descr);

    if (!IS_NPC(model)) {
        new_mob->mob_specials.default_pos = 127;
    }
    if (model->player.description)
        new_mob->player.description = strdup(model->player.description);
    if (model->player.title) {
        new_mob->player.title = strdup(model->player.title);
    }
    new_mob->player.sex = model->player.sex;
    new_mob->player.class_num = model->player.class_num;
    new_mob->player.race = model->player.race;
    new_mob->player.race_align = model->player.race_align;
    new_mob->player.level = model->player.level;
    GET_HOMEROOM(new_mob) = GET_HOMEROOM(model);
    new_mob->player.time = model->player.time;
    new_mob->player.composition = model->player.composition;
    new_mob->player.lifeforce = model->player.lifeforce;

    /* mod_size and affected_size are set like this because the duplicate
     * will not have the spells or equipment that would place a value in mod_size.
     */
    new_mob->player.affected_size = model->player.natural_size;
    new_mob->player.mod_size = 0;
    new_mob->player.natural_size = model->player.natural_size;
    new_mob->player.base_size = model->player.base_size;
    new_mob->player.base_weight = model->player.base_weight;
    new_mob->player.base_height = model->player.base_height;
    reset_height_weight(new_mob);
    /* END  char_player_data */

    /*  char_special_data */
    GET_POS(new_mob) = GET_POS(model);
    GET_STANCE(new_mob) = GET_STANCE(model);
    /* Set default pos to something impossible so that the position-description
     * will always be tacked on */
    GET_DEFAULT_POS(new_mob) = -1;
    new_mob->char_specials.alignment = model->char_specials.alignment;
    COPY_FLAGS(MOB_FLAGS(new_mob), MOB_FLAGS(model), NUM_MOB_FLAGS);
    new_mob->char_specials.perception = model->char_specials.perception;
    new_mob->char_specials.hiddenness = model->char_specials.hiddenness;
    /* TODO: saving throw and skills */
    /* END  char_special_data */

    /*  mob_special_data */
    new_mob->mob_specials = model->mob_specials;
    new_mob->mob_specials.memory = nullptr;
    /* END  mob_special_data */

    REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_SPEC);

    return new_mob;
}

CharData *copyplayer(CharData *ch, CharData *model) {
    CharData *new_mob;

    new_mob = duplicate_char(model, ch->in_room);
    if (!new_mob)
        return 0;

    CLEAR_FLAGS(MOB_FLAGS(new_mob), NUM_MOB_FLAGS);
    SET_FLAG(MOB_FLAGS(new_mob), MOB_ISNPC);
    SET_FLAG(MOB_FLAGS(new_mob), MOB_SCAVENGER);
    SET_FLAG(MOB_FLAGS(new_mob), MOB_MEMORY);

    return new_mob;
}

/* Transform a character into a phantasm.
 * This is intended for a mob that's being created as a phantasm.
 *
 * In other words, NOT to take some physical creature and suddenly make it
 * illusory - that would entail its objects falling to the ground and stuff.
 * And it really wouldn't fit any scenarios very well. */
void phantasm_transform(CharData *ch, CharData *model, int life_hours) {
    char short_buf[160], long_buf[160], alias_buf[160];
    effect effect;

    SET_FLAG(MOB_FLAGS(ch), MOB_ILLUSORY); /* Make it an illusion */

    /* Make it expire */
    if (life_hours > 0) {
        memset(&effect, 0, sizeof(effect));
        effect.type = SPELL_PHANTASM;
        effect.duration = life_hours;
        SET_FLAG(effect.flags, EFF_ANIMATED);
        effect_to_char(ch, &effect);
    }

    /* Phantasms have no objects and thus no money. */
    GET_PLATINUM(ch) = 0;
    GET_GOLD(ch) = 0;
    GET_SILVER(ch) = 0;
    GET_COPPER(ch) = 0;

    /* Having no soul, phantasms cannot exude auras of good or evil. */
    GET_ALIGNMENT(ch) = 0;

    /* The phantasm copies its model's current state, even if that state was
     * modified by magic.  Illusion-creating spells don't worry about the
     * underlying cause - they just make a copy. */
    if (model) {
        /* So the copy's height, weight, and size all come from the
         * model's affected values. */
        sprintf(short_buf, "the illusion of %s", GET_NAME(model));
        sprintf(long_buf, "The illusion of %s waits here.\r\n", GET_NAME(model));
        sprintf(alias_buf, "illusion %s", GET_NAMELIST(model));

        ch->player.weight = model->player.weight;
        ch->player.height = model->player.height;
        ch->player.affected_size = model->player.affected_size;
        ch->player.mod_size = 0;
        ch->player.natural_size = model->player.affected_size;
        ch->player.base_size = model->player.base_size;
        ch->player.base_weight = model->player.base_weight;
        ch->player.base_height = model->player.base_height;
    } else {
        sprintf(short_buf, "the illusion of %s", GET_NAME(ch));
        sprintf(long_buf, "The illusion of %s waits here.\r\n", GET_NAME(ch));
        sprintf(alias_buf, "illusion %s", GET_NAMELIST(ch));
    }
    GET_NAME(ch) = strdup(short_buf);
    GET_LDESC(ch) = strdup(long_buf);
    GET_NAMELIST(ch) = strdup(alias_buf);
}

CharData *summon_phantasm(CharData *ch, int vnum, int life_hours) {
    CharData *new_mob;

    if (!(new_mob = load_summoned_mob(vnum, ch->in_room)))
        return nullptr;

    phantasm_transform(new_mob, nullptr, life_hours);
    SET_FLAG(MOB_FLAGS(new_mob), MOB_NOSCRIPT); /* Prevent specprocs and triggers */

    return new_mob;
}

/*
 * Every spell which summons/gates/conjours a mob comes through here.
 *
 * Return value: CAST_RESULT_ flags.
 */

int mag_summon(int skill, CharData *ch, CharData *vict, ObjData *obj, int spellnum, int savetype) {
    int orig_mob_rnum;
    CharData *new_mob;
    int success, duration;
    effect eff;
    ObjData *temp_obj, *next_obj;
    float base_duration, preserve_mult;

    /* PROJECT */
    int pvnum;
    int phantasm_mobs[] = {
        8016,  /*  0. ant              */
        3506,  /*  1. mouse            */
        1020,  /*  2. garter snake     */
        5414,  /*  3. lesser shade     */
        8605,  /*  4. sparrow          */
        1688,  /*  5. rabbit           */
        8807,  /*  6. cat              */
        17205, /*  7. the familiar     */
        8803,  /*  8. cow              */
        4308,  /*  9. ceiling monkey   */
        30215, /* 10. snow troll       */
        12504, /* 11. gnome            */
        23732  /* 12. tiny mist beast  */
    };
    int num_phantasm_mobs = 13;

    if (!ch)
        return 0;

    if (EFF_FLAGGED(ch, EFF_CHARM) && !MOB_FLAGGED(ch, MOB_ANIMATED)) {
        send_to_char("You are too giddy to have any followers!\r\n", ch);
        return CAST_RESULT_CHARGE;
    }

    switch (spellnum) {
    case SPELL_ANIMATE_DEAD:
        /* first make sure it's a corpse */
        if (!obj || !IS_CORPSE(obj)) {
            send_to_char("A corpse would help, don't you think?\r\n", ch);
            return CAST_RESULT_CHARGE;
        }

        /* if it was a pc corpse, fail automatically (for now at least) */
        if (IS_PLR_CORPSE(obj)) {
            if (!pk_allowed) {
                send_to_char("Raising PC corpses is not currently allowed.\r\n", ch);
                return CAST_RESULT_CHARGE;
            }
        } else if ((GET_OBJ_VAL(obj, VAL_CONTAINER_CORPSE) == CORPSE_NPC_NORAISE) || /* unraisable */
                   ((orig_mob_rnum = GET_OBJ_MOB_FROM(obj)) == NOBODY)) {
            send_to_char("That corpse is much too decayed to raise.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }

        /* make the mob. */
        if (!IS_PLR_CORPSE(obj))
            new_mob = create_undead(mob_proto + orig_mob_rnum, ch, false);
        else
            new_mob = create_undead(mob_proto, ch, true);

        char_to_room(new_mob, ch->in_room);

        /* can we control it? */
        success = ch_can_control_mob(ch, new_mob);
        if (success == CONTROL_HELLNO) {
            act("You begin to raise $N beyond your power, but you stop the spell "
                "in time.",
                false, ch, 0, new_mob, TO_CHAR);
            act("$n begins to raise $N beyond $s power, but $e stops the spell "
                "in time.",
                false, ch, 0, new_mob, TO_ROOM);
            extract_char(new_mob);
            GET_OBJ_VAL(obj, VAL_CONTAINER_CORPSE) = CORPSE_NPC_NORAISE; /* mark it unraisable */
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        /* anim duration will depend on success later */
        base_duration = (((float)skill - 20.0) / 5.0 + 5.0) * 3.0;
        preserve_mult = (float)GET_OBJ_DECOMP(obj) / 300.0 + 1.0;
        if (preserve_mult > 2.0)
            preserve_mult = 2.0;

        /* make perma-stoned mobs twitch faster */
        if (EFF_FLAGGED(new_mob, EFF_STONE_SKIN))
            base_duration /= 6.0;

        /* set it as animated */
        memset(&eff, 0, sizeof(eff));
        eff.type = SPELL_ANIMATE_DEAD;
        eff.duration = (int)(base_duration * preserve_mult);
        SET_FLAG(eff.flags, EFF_ANIMATED);
        eff.modifier = 0;
        eff.location = APPLY_NONE;
        effect_to_char(new_mob, &eff);

        if (success == CONTROL_YES) { /* controlled it */
            act("You raise $N.", false, ch, 0, new_mob, TO_CHAR);
            act("$n raises $N.", false, ch, 0, new_mob, TO_ROOM);
            eff.type = SPELL_CHARM;
            eff.duration = (int)(base_duration * preserve_mult) + 1;
            SET_FLAG(eff.flags, EFF_CHARM);
            eff.modifier = 0;
            eff.location = APPLY_NONE;
            effect_to_char(new_mob, &eff);
            add_follower(new_mob, ch);
            REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGRESSIVE);
            REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_SPEC);
        } else { /* not able to control it */
            act("You raise $N, and $E doesn't seem too happy about it.", false, ch, 0, new_mob, TO_CHAR);
            act("$n raises $N, and $E doesn't seem too happy about it.", false, ch, 0, new_mob, TO_ROOM);
            /* we want it aggressive, but not against anything in particular */
            SET_FLAG(MOB_FLAGS(new_mob), MOB_AGGRESSIVE);
            REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_EVIL);
            REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_GOOD);
            REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_NEUTRAL);
            REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_EVIL_RACE);
            REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_GOOD_RACE);

            attack(new_mob, ch);
        }

        /* move corpse possessions to mob */
        for (temp_obj = obj->contains; temp_obj; temp_obj = next_obj) {
            next_obj = temp_obj->next_content;
            obj_from_obj(temp_obj);
            if (GET_OBJ_TYPE(temp_obj) == ITEM_MONEY) {
                GET_PLATINUM(new_mob) += GET_OBJ_VAL(temp_obj, VAL_MONEY_PLATINUM);
                GET_GOLD(new_mob) += GET_OBJ_VAL(temp_obj, VAL_MONEY_GOLD);
                GET_SILVER(new_mob) += GET_OBJ_VAL(temp_obj, VAL_MONEY_SILVER);
                GET_COPPER(new_mob) += GET_OBJ_VAL(temp_obj, VAL_MONEY_COPPER);
                extract_obj(temp_obj);
            } else
                obj_to_char(temp_obj, new_mob);
        }

        /* destroy the corpse */
        extract_obj(obj);

        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    case SPELL_PHANTASM:

        /* Decide how long it will live */
        duration = 2 + skill / 5;

        /* Choose one of our available mobs */
        pvnum = phantasm_mobs[number(0, num_phantasm_mobs - 1)];
        if (real_mobile(pvnum) < 0)
            pvnum = 9001; /* Test mob = earle's doppelganger */

        /* Load it up */
        new_mob = summon_phantasm(ch, pvnum, duration);
        if (new_mob == nullptr) {
            act("The spell fizzles.", false, 0, 0, 0, TO_ROOM);
            send_to_char("The spell fizzles.\r\n", ch);
            return 0;
        }

        /* need to add charm flag */
        eff.type = SPELL_CHARM;
        eff.duration = 1000;
        SET_FLAG(eff.flags, EFF_CHARM);
        eff.modifier = 0;
        eff.location = APPLY_NONE;
        effect_to_char(new_mob, &eff);
        add_follower(new_mob, ch);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_EVIL);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_GOOD);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_NEUTRAL);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_EVIL_RACE);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_GOOD_RACE);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGRESSIVE);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_STAY_ZONE);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_PROTECTOR);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_PEACEKEEPER);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_HELPER);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_MEMORY);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_WIMPY);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_NOSUMMON);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_SPEC);
        GET_LIFEFORCE(new_mob) = LIFE_MAGIC;

        /* Feedback */
        act("From scattered motes of light, $n coalesces.", true, new_mob, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;

    case SPELL_SIMULACRUM:
        /* Simulacrum is like the phantasm spell, except it duplicates an
         * existing creature. */
        if (!vict) {
            send_to_char("Who did you want to duplicate?\r\n", ch);
            return 0;
        }
        if (GET_LEVEL(vict) > skill) {
            act("$N is far too powerful!", false, ch, 0, vict, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }
        if (MOB_FLAGGED(vict, MOB_ILLUSORY)) {
            act("You cannot copy another illusion!", false, ch, 0, vict, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }

        /* Decide how long the phantasm will last */
        duration = 3 + skill / 4;
        pvnum = GET_MOB_VNUM(vict);

        /* Load it up */
        if (pvnum < 0 || real_mobile(pvnum) < 0) {
            /* Apparently we're making an illusory copy of a player. */
            if (!IS_NPC(vict)) {
                new_mob = copyplayer(ch, vict);
                if (new_mob == nullptr) {
                    act("The spell fizzles.", false, 0, 0, 0, TO_ROOM);
                    send_to_char("The spell fizzles.\r\n", ch);
                    return 0;
                }
            } else {
                send_to_char("Sadly, that's impossible.\r\n", ch);
                return 0;
            }
            phantasm_transform(new_mob, vict, duration);

            /* Set this so aggro mobs will still attack it, even though
             * it technically isn't a player. */
            SET_FLAG(MOB_FLAGS(new_mob), MOB_PLAYER_PHANTASM);
        } else {
            /* Making an illusory copy of an NPC. */
            new_mob = duplicate_char(vict, ch->in_room);
            if (!new_mob) {
                act("The spell fizzles.", false, 0, 0, 0, TO_ROOM);
                send_to_char("The spell fizzles.\r\n", ch);
                return 0;
            }
            phantasm_transform(new_mob, vict, duration);
        }
        SET_FLAG(MOB_FLAGS(new_mob), MOB_NOSCRIPT); /* Prevent specprocs and triggers */

        /* need to add charm flag */
        eff.type = SPELL_CHARM;
        eff.duration = 1000;
        SET_FLAG(eff.flags, EFF_CHARM);
        eff.modifier = 0;
        eff.location = APPLY_NONE;
        effect_to_char(new_mob, &eff);
        add_follower(new_mob, ch);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_EVIL);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_GOOD);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_NEUTRAL);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_EVIL_RACE);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGR_GOOD_RACE);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_AGGRESSIVE);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_STAY_ZONE);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_PROTECTOR);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_PEACEKEEPER);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_HELPER);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_WIMPY);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_MEMORY);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_NOSUMMON);
        REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_SPEC);
        GET_LIFEFORCE(new_mob) = LIFE_MAGIC;

        /* Feedback */
        act("From scattered motes of light, $n coalesces.", true, new_mob, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    case SPELL_SUMMON_ELEMENTAL:
    default:
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/* mag_point
 *
 * Performs healing magic on characters.
 *
 * Return value: CAST_RESULT_ flags
 */

int mag_point(int skill, CharData *ch, CharData *victim, int spellnum, int savetype) {
    int hit = 0;
    int move = 0;
    int hunger = 0;
    int thirst = 0;
    int hide = 0;
    int multiplier = 0;
    int sus;

    if (victim == nullptr)
        return 0;

    sus = susceptibility(victim, skills[spellnum].damage_type);

    multiplier = 2 + (skill / 24); /* max 6 */

    switch (spellnum) {
    case SPELL_CURE_LIGHT:
        hit = dice(2, 8) + 1;
        send_to_char("You feel better.\r\n", victim);
        break;
    case SPELL_CURE_SERIOUS:
        hit = dice(4, 8) + 2;
        send_to_char("You feel much better!\r\n", victim);
        break;
    case SPELL_CURE_CRITIC:
        hit = dice(6, 8) + 3;
        send_to_char("You feel a lot better!\r\n", victim);
        break;
    case SPELL_HEAL:
        hit = dice(50, 2);
        send_to_char("A warm feeling floods your body.\r\n", victim);
        break;
    case SPELL_FULL_HEAL:
        hit = dice(100, 2) + 30;
        send_to_char("&7You have been FULLY healed!&0\r\n", victim);
        break;
    case SPELL_VIGORIZE_LIGHT:
        move = dice(3, 8) + 5;
        send_to_char("You feel vigorized!\r\n", victim);
        break;
    case SPELL_VIGORIZE_SERIOUS:
        move = dice(5, 8) + 5;
        send_to_char("You feel vigorized!\r\n", victim);
        break;
    case SPELL_VIGORIZE_CRITIC:
        move = dice(7, 8) + 5;
        send_to_char("You feel vigorized!\r\n", victim);
        break;
    case SPELL_NOURISHMENT:
        switch (SECT(victim->in_room)) {
        case SECT_FIELD:
        case SECT_FOREST:
        case SECT_HILLS:
        case SECT_MOUNTAIN:
        case SECT_ROAD:
        case SECT_GRASSLANDS:
        case SECT_SWAMP:
        case SECT_BEACH:
            hunger = 24;
            thirst = 24;
            act("&2&b$n&2&b sprouts roots that dig deep beneath the soil, drawing "
                "sustenence.&0",
                true, victim, 0, 0, TO_ROOM);
            act("&2&bYou sprout roots that dig deep beneath the soil, drawing "
                "sustenence.&0",
                false, victim, 0, 0, TO_CHAR);
            break;
        default:
            act("&2$n&2 sends forth roots in a vain attempt to gain sustenance.&0", true, victim, 0, 0, TO_ROOM);
            switch (SECT(victim->in_room)) {
            case SECT_SHALLOWS:
            case SECT_WATER:
            case SECT_UNDERWATER:
                thirst = 24;
                act("&2You send out roots which quickly become waterlogged.  They draw "
                    "moisture but no nutrients.&0",
                    false, victim, 0, 0, TO_CHAR);
                break;
            case SECT_AIR:
                act("&2Your roots flail briefly in the air, unable to grow without "
                    "soil.&0",
                    false, victim, 0, 0, TO_CHAR);
                break;
            default:
                act("&2You send out roots, but they are unable to penetrate to the "
                    "life-giving soil.&0",
                    false, victim, 0, 0, TO_CHAR);
            }
        }
        break;
    case SPELL_INVIGORATE:
        move = GET_MAX_MOVE(victim);
        send_to_char("You feel fully vigorized!\r\n", victim);
        break;
    case SPELL_NATURES_EMBRACE:
        hide = skill * 5;
        break;
    case SPELL_CONCEALMENT:
        send_to_char("&9&bYou vanish.&0\r\n", victim);
        act("&9&b$N&9&b slowly fades out of existence.&0", true, ch, 0, victim, TO_ROOM);
        hide = skill * 4;
        break;
    default:
        log("SYSERR:magic.c:mag_point():invalid spell");
        return CAST_RESULT_CHARGE;
    }

    hit = hit * sus / 100;
    move = move * sus / 100;
    hunger = hunger * sus / 100;
    thirst = thirst * sus / 100;
    hide = hide * sus / 100;

    if (hit)
        hurt_char(victim, ch, -hit * multiplier, true);
    if (move)
        alter_move(victim, -move * multiplier);
    if (hunger)
        gain_condition(victim, FULL, hunger);
    if (thirst)
        gain_condition(victim, THIRST, thirst);
    if (hide)
        GET_HIDDENNESS(victim) = MIN(GET_HIDDENNESS(victim) + hide, 1000);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/* mag_unaffect
 *
 * Return value: CAST_RESULT_ flags.
 */

int mag_unaffect(int skill, CharData *ch, CharData *victim, int spellnum, int type) {
    int spell = 0;
    char *to_vict = nullptr, *to_room = nullptr;

    if (victim == nullptr)
        return 0;

    switch (spellnum) {
    case SPELL_CURE_BLIND:
    case SPELL_HEAL:
    case SPELL_FULL_HEAL:
        if (EFF_FLAGGED(victim, EFF_BLIND)) {
            to_vict = "Your vision returns!";
            to_room = "There's a momentary gleam in $n's eyes.";
            if (affected_by_spell(victim, SPELL_BLINDNESS))
                spell = SPELL_BLINDNESS; /* Remove blindness below */
            if (affected_by_spell(victim, SKILL_EYE_GOUGE)) {
                if (spell) /* If already removing a spell, remove it and eye gouge now */
                    effect_from_char(victim, spell);
                spell = SKILL_EYE_GOUGE;
            }
            if (affected_by_spell(victim, SPELL_BLINDING_BEAUTY)) {
                if (spell)
                    effect_from_char(victim, spell);
                spell = SPELL_BLINDING_BEAUTY;
            }
            if (affected_by_spell(victim, SPELL_SUNRAY)) {
                if (spell) /* If already removing a spell, remove it and set sunray now */
                    effect_from_char(victim, spell);
                spell = SPELL_SUNRAY;
            }
        }
        if ((spellnum == SPELL_HEAL || spellnum == SPELL_FULL_HEAL) && affected_by_spell(victim, SPELL_DISEASE)) {
            effect_from_char(victim, SPELL_DISEASE);
            send_to_char("Your disease has been cured.\r\n", victim);
        }
        if ((spellnum == SPELL_HEAL || spellnum == SPELL_FULL_HEAL) && affected_by_spell(victim, SPELL_POISON)) {
            effect_from_char(victim, SPELL_POISON);
            send_to_char("The poison in your system has been cleansed.\r\n", victim);
            check_regen_rates(victim); /* speed up regen rate immediately */
        }
        break;
    case SPELL_ENLARGE:
        if (!EFF_FLAGGED(victim, EFF_REDUCE))
            return CAST_RESULT_CHARGE;
        spell = SPELL_REDUCE;
        to_vict = "&8You return to your normal size.&0";
        break;
    case SPELL_EXTINGUISH:
        REMOVE_FLAG(EFF_FLAGS(victim), EFF_ON_FIRE);
        send_to_char("You are doused with a magical liquid.\r\n", victim);
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    case SONG_JOYFUL_NOISE:
        spell = SPELL_SILENCE;
        send_to_char("&3&bYou make such a racket it pierces the silence!&0\r\n", ch);
        act("&3&b$N makes such a racket it pierces the silence!&0", true, victim, 0, ch, TO_NOTVICT);
        to_vict = "The noise shatters the silence about you!";
        to_room = "$n begins to make sound again!";
        break;
    case SONG_FREEDOM_SONG:
        if (EFF_FLAGGED(victim, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(victim, EFF_MAJOR_PARALYSIS) ||
            affected_by_spell(victim, SPELL_ENTANGLE) || EFF_FLAGGED(victim, EFF_IMMOBILIZED)) {
            act("&6&b$N's music shatters the magic paralyzing you!&0", false, victim, 0, ch, TO_CHAR);
            act("&6&bYour music disrupts the magic keeping $n frozen.&0", false, victim, 0, ch, TO_VICT);
            act("&6&b$N's music frees $n from magic which held $m motionless.&0", true, victim, 0, ch, TO_NOTVICT);
            if (affected_by_spell(victim, SPELL_MINOR_PARALYSIS))
                spell = SPELL_MINOR_PARALYSIS;
            if (affected_by_spell(victim, SPELL_MAJOR_PARALYSIS)) {
                if (spell) /* If already removing a spell, remove it and set major paralysis now */
                    effect_from_char(victim, spell);
                spell = SPELL_MAJOR_PARALYSIS;
            }
            if (affected_by_spell(victim, SPELL_ENTANGLE)) {
                if (spell) /* If already removing a spell, remove it and set entangle now */
                    effect_from_char(victim, spell);
                spell = SPELL_ENTANGLE;
            }
            if (affected_by_spell(victim, SPELL_SPINECHILLER)) {
                if (spell) /* If already removing a spell, remove it and set spinechiller now */
                    effect_from_char(victim, spell);
                spell = SPELL_SPINECHILLER;
            }
            if (affected_by_spell(victim, SPELL_WEB)) {
                if (spell) /*If already removing a spell, remove it and set web now */
                    effect_from_char(victim, spell);
                spell = SPELL_WEB;
            }
        }
        if (EFF_FLAGGED(victim, EFF_MESMERIZED) || EFF_FLAGGED(victim, EFF_CONFUSION) ||
            EFF_FLAGGED(victim, EFF_MISDIRECTION) || affected_by_spell(victim, SONG_ENRAPTURE)) {
            act("&5&bYou draw $n's attention from whatever $e was pondering.&0", false, victim, 0, ch, TO_VICT);
            act("&5&b$N jolts you out of your reverie!&0", false, victim, 0, ch, TO_CHAR);
            act("&5&b$N's music distracts $n from whatever was fascinating $m.&0", true, victim, 0, ch, TO_NOTVICT);
            if (affected_by_spell(victim, SPELL_MESMERIZE)) {
                if (spell) /* If already removing a spell, remove it and set mesmerize now */
                    effect_from_char(victim, spell);
                spell = SPELL_SPINECHILLER;
            }
            if (affected_by_spell(victim, SPELL_CONFUSION)) {
                if (spell) /* If already removing a spell, remove it and set confusion now */
                    effect_from_char(victim, spell);
                spell = SPELL_CONFUSION;
            }
            if (affected_by_spell(victim, SPELL_MISDIRECTION)) {
                if (spell) /* If already removing a spell, remove it and set misdirection now */
                    effect_from_char(victim, spell);
                spell = SPELL_MISDIRECTION;
            }
            if (affected_by_spell(victim, SONG_ENRAPTURE)) {
                if (spell) /* If already removing a spell, remove it and set enrapture now */
                    effect_from_char(victim, spell);
                spell = SONG_ENRAPTURE;
            }
        }
        if (EFF_FLAGGED(victim, EFF_CHARM) || affected_by_spell(victim, SPELL_CHARM)) {
            act("&3&bYour music breaks the hold over $n!&0", false, ch, 0, ch->master, TO_VICT);
            act("&3&b$N's music breaks the hold over you!&0", false, ch, 0, ch->master, TO_CHAR);
            act("&3&b$N's music breaks the hold over $n!&0", true, ch, 0, ch->master, TO_NOTVICT);
            if (affected_by_spell(victim, SPELL_CHARM)) {
                if (spell) /* If already removing a spell, remove it and set charm now */
                    effect_from_char(victim, spell);
                spell = SPELL_CHARM;
            }
        }
        if (EFF_FLAGGED(victim, EFF_RAY_OF_ENFEEB) || affected_by_spell(victim, SPELL_CHILL_TOUCH)) {
            act("&1Your music rejuvenates $n's body!&0", false, victim, 0, ch, TO_VICT);
            act("&1$N's music rejuvenates your body!&0", false, victim, 0, ch, TO_CHAR);
            act("&1$N's music rejuvenates $n's body!&0", true, victim, 0, ch, TO_NOTVICT);
            if (affected_by_spell(victim, SPELL_RAY_OF_ENFEEB)) {
                if (spell) /* If already removing a spell, remove it and set ray of enfeeblement now */
                    effect_from_char(victim, spell);
                spell = SPELL_RAY_OF_ENFEEB;
            }
            if (affected_by_spell(victim, SPELL_CHILL_TOUCH)) {
                if (spell) /* If already removing a spell, remove it and set chill touch now */
                    effect_from_char(victim, spell);
                spell = SPELL_CHILL_TOUCH;
            }
        }
        break;
    case SONG_INSPIRATION:
    case SONG_HEROIC_JOURNEY:
        if (affected_by_spell(victim, SONG_TERROR) || affected_by_spell(victim, SONG_BALLAD_OF_TEARS)) {
            if (affected_by_spell(victim, SONG_TERROR))
                spell = SONG_TERROR;
            if (affected_by_spell(victim, SONG_BALLAD_OF_TEARS)) {
                if (spell) /* If already removing a spell, remove it and set ballad of tears now */
                    effect_from_char(victim, spell);
                spell = SONG_BALLAD_OF_TEARS;
            }
        }
        to_vict = "$N's music soothes your fears.";
        to_room = "$N's music soothes $n's fears.";
        break;
    case SPELL_SANE_MIND:
        if (!EFF_FLAGGED(victim, EFF_INSANITY))
            return CAST_RESULT_CHARGE;
        if (affected_by_spell(victim, SPELL_INSANITY))
            spell = SPELL_INSANITY;
        if (affected_by_spell(victim, SONG_CROWN_OF_MADNESS))
            spell = SONG_CROWN_OF_MADNESS;
        to_vict = "Your mind comes back to reality.";
        to_room = "$n regains $s senses.";
        break;
    case SPELL_REDUCE:
        if (!EFF_FLAGGED(victim, EFF_ENLARGE))
            return CAST_RESULT_CHARGE;
        spell = SPELL_ENLARGE;
        to_vict = "&8You return to your normal size.&0";
        break;
    case SPELL_REMOVE_CURSE:
        spell = SPELL_CURSE;
        to_vict = "You don't feel so unlucky.";
        break;
    case SPELL_REMOVE_PARALYSIS:
        if ((affected_by_spell(victim, SPELL_MINOR_PARALYSIS)) || (affected_by_spell(victim, SPELL_MAJOR_PARALYSIS)) ||
            (affected_by_spell(victim, SPELL_ENTANGLE))) {
            if (affected_by_spell(victim, SPELL_MINOR_PARALYSIS)) {
                spell = SPELL_MINOR_PARALYSIS;
            }
            if (affected_by_spell(victim, SPELL_MAJOR_PARALYSIS)) {
                if (spell) /* If already removing a spell, remove it and set major paralysis now */
                    effect_from_char(victim, spell);
                spell = SPELL_MAJOR_PARALYSIS;
            }
            if (affected_by_spell(victim, SPELL_ENTANGLE)) {
                if (spell) /* If already removing a spell, remove it and set entangle now */
                    effect_from_char(victim, spell);
                spell = SPELL_ENTANGLE;
            }
            if (affected_by_spell(victim, SPELL_MESMERIZE)) {
                if (spell) /* If already removing a spell, remove it and set mesmerize now */
                    effect_from_char(victim, spell);
                spell = SPELL_MESMERIZE;
            }
            to_vict = "&3&bYour body begins to move again.&0";
            to_room = "&3&b$n begins to move again.&0";
        }
        break;
    case SPELL_REMOVE_POISON:
        spell = SPELL_POISON;
        to_vict = "A warm feeling runs through your body!";
        to_room = "$n looks better.";
        break;
    case SONG_TERROR:
    case SONG_BALLAD_OF_TEARS:
        if (affected_by_spell(victim, SONG_INSPIRATION) || affected_by_spell(victim, SONG_HEROIC_JOURNEY)) {
            if (affected_by_spell(victim, SONG_INSPIRATION))
                spell = SONG_INSPIRATION;
            if (affected_by_spell(victim, SONG_HEROIC_JOURNEY)) {
                if (spell) /* If already removing a spell, remove it and set heroic journey now */
                    effect_from_char(victim, spell);
                spell = SONG_HEROIC_JOURNEY;
            }
        }
        to_vict = "Your inspiration chills suddenly.";
        to_room = "$n's inspiration chills suddenly.";
        break;
    default:
        sprintf(buf, "SYSERR: unknown spellnum %d passed to mag_unaffect", spellnum);
        log(buf);
        return CAST_RESULT_CHARGE;
    }

    if (!affected_by_spell(victim, spell) && spellnum != SPELL_HEAL && spellnum != SPELL_FULL_HEAL) {
        send_to_char(NOEFFECT, ch);
        return CAST_RESULT_CHARGE;
    }

    effect_from_char(victim, spell);
    if (to_vict != nullptr)
        act(to_vict, false, victim, 0, ch, TO_CHAR);
    if (to_room != nullptr)
        act(to_room, true, victim, 0, ch, TO_ROOM);

    if (spellnum == SPELL_REMOVE_POISON)
        check_regen_rates(victim); /* speed up regen rate immediately */

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/* Return value: CAST_RESULT_ flags.
 */
int mag_alter_obj(int skill, CharData *ch, ObjData *obj, int spellnum, int savetype) {
    char *to_char = nullptr;
    char *to_room = nullptr;
    int i;
    int result = CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;

    /* Set post_modify to true if you want to send messages first, then
     * modify the object. You must add a case to the second switch
     * statement to actualy make the modification(s). */
    bool post_modify = false;

    if (obj == nullptr)
        return 0;

    switch (spellnum) {
    case SPELL_BLESS:
        /* Skip all checks if godly */
        if (GET_LEVEL(ch) > LVL_IMMORT) {
            if (OBJ_EFF_FLAGGED(obj, EFF_BLESS)) {
                to_char = "It's already blessed.";
            } else {
                SET_FLAG(GET_OBJ_EFF_FLAGS(obj), EFF_BLESS);
                SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_ANTI_EVIL);
                to_char = "$p glows briefly.";
            }
            break;
        }

        result = CAST_RESULT_CHARGE;

        for (i = 0; i < MAX_OBJ_APPLIES; i++) {
            if (obj->applies[i].location) {
                /* This signifies that an effect was found - see below */
                i = -1;
                break;
            }
        }

        if (GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
            to_char = "This spell is only effective on weapons.";
            result = 0;

            /* Too high level? */
        } else if (GET_OBJ_LEVEL(obj) > skill)
            to_char = "$p is too powerful for you to bless.";

        /* Too heavy */
        else if (GET_OBJ_WEIGHT(obj) > 5 * skill)
            to_char = "$p is too large for you to bless.";

        /* Some sort of impurity (or already blessed) */
        else if (OBJ_FLAGGED(obj, ITEM_GLOW) || OBJ_FLAGGED(obj, ITEM_HUM) || OBJ_FLAGGED(obj, ITEM_INVISIBLE) ||
                 OBJ_FLAGGED(obj, ITEM_MAGIC) || OBJ_FLAGGED(obj, ITEM_NODROP) || OBJ_FLAGGED(obj, ITEM_ANTI_GOOD))
            to_char = "$p doesn't seem receptive to the blessing.";

        /* Already has magical effects */
        else if (HAS_FLAGS(GET_OBJ_EFF_FLAGS(obj), NUM_EFF_FLAGS) || i < 0 /* see obj->affected check, above */
        )
            to_char = "The blessing is repelled from $p.";
        else {
            SET_FLAG(GET_OBJ_EFF_FLAGS(obj), EFF_BLESS);
            SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_ANTI_EVIL);
            to_char = "$p glows briefly.";
            result = CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }
        break;
    case SPELL_CURSE:
        if (!OBJ_FLAGGED(obj, ITEM_NODROP)) {
            SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_NODROP);
            if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
                GET_OBJ_VAL(obj, VAL_WEAPON_DICE_SIZE)--;
            to_char = "$p briefly glows red.";
        }
        break;
    case SPELL_DARK_PRESENCE:
        /* Skip all checks if godly */
        if (GET_LEVEL(ch) > LVL_IMMORT) {
            if (OBJ_EFF_FLAGGED(obj, EFF_HEX)) {
                to_char = "It's already hexed.";
            } else {
                SET_FLAG(GET_OBJ_EFF_FLAGS(obj), EFF_HEX);
                SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_ANTI_GOOD);
                to_char = "$p is imbued with a dark aura.";
            }
            break;
        }

        for (i = 0; i < MAX_OBJ_APPLIES; i++) {
            if (obj->applies[i].location) {
                /* This signifies that an effect was found - see below */
                i = -1;
                break;
            }
        }

        result = CAST_RESULT_CHARGE;

        if (GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
            to_char = "This spell is only effective on weapons.";
            result = 0;

            /* Too high level? */
        } else if (GET_OBJ_LEVEL(obj) > skill)
            to_char = "$p is too powerful for you to hex.";

        /* Too heavy */
        else if (GET_OBJ_WEIGHT(obj) > 5 * skill)
            to_char = "$p is too large for you to hex.";

        /* Some sort of impurity (or already hexed) */
        else if (OBJ_FLAGGED(obj, ITEM_GLOW) || OBJ_FLAGGED(obj, ITEM_HUM) || OBJ_FLAGGED(obj, ITEM_INVISIBLE) ||
                 OBJ_FLAGGED(obj, ITEM_MAGIC) || OBJ_FLAGGED(obj, ITEM_NODROP) || OBJ_FLAGGED(obj, ITEM_ANTI_EVIL))
            to_char = "$p doesn't seem receptive to the malediction.";

        /* Already has magical effects */
        else if (HAS_FLAGS(GET_OBJ_EFF_FLAGS(obj), NUM_EFF_FLAGS) || i < 0 /* see obj->affected check, above */
        )
            to_char = "The hex is repelled from $p.";
        else {
            SET_FLAG(GET_OBJ_EFF_FLAGS(obj), EFF_HEX);
            SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_ANTI_GOOD);
            to_char = "$p is imbued with a dark aura.";
            result = CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }
        break;
    case SPELL_INVISIBLE:
    case SPELL_MASS_INVIS:
        if (!OBJ_FLAGGED(obj, ITEM_NOINVIS) && !OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
            post_modify = true;
            to_char = "$p vanishes.";
        }
        break;
    case SPELL_POISON:
        if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) || (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
             (GET_OBJ_TYPE(obj) == ITEM_FOOD)) &&
            !IS_POISONED(obj)) {
            GET_OBJ_VAL(obj, VAL_FOOD_POISONED) = true;
            to_char = "$p steams briefly.";
        }
        break;
    case SPELL_REMOVE_CURSE:
        if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
            REMOVE_FLAG(GET_OBJ_FLAGS(obj), ITEM_NODROP);
            if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
                GET_OBJ_VAL(obj, VAL_WEAPON_DICE_SIZE)++;
            to_char = "$p briefly glows blue.";
        }
        break;
    case SPELL_REMOVE_POISON:
        if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) || (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
             (GET_OBJ_TYPE(obj) == ITEM_FOOD)) &&
            IS_POISONED(obj)) {
            GET_OBJ_VAL(obj, VAL_FOOD_POISONED) = false;
            to_char = "$p steams briefly.";
        }
        break;
    }

    if (to_char == nullptr)
        send_to_char(NOEFFECT, ch);
    else
        act(to_char, true, ch, obj, 0, TO_CHAR);

    if (to_room != nullptr)
        act(to_room, true, ch, obj, 0, TO_ROOM);
    else if (to_char != nullptr)
        act(to_char, true, ch, obj, 0, TO_ROOM);

    if (post_modify)
        switch (spellnum) {
        case SPELL_INVISIBLE:
        case SPELL_MASS_INVIS:
            SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_INVISIBLE);
            break;
        }

    return result;
}

#define WAYBREAD_OBJ_1 18508
#define WAYBREAD_OBJ_2 10

int mag_creation(int skill, CharData *ch, int spellnum) {
    char *to_char = nullptr, *to_room = nullptr;
    ObjData *tobj;
    int z, zplus;
    int give_char = 0;
    if (ch == nullptr)
        return 0;

    switch (spellnum) {
    case SPELL_CREATE_SPRING:
        switch (SECT(IN_ROOM(ch))) {
        case SECT_SHALLOWS:
        case SECT_WATER:
        case SECT_UNDERWATER:
        case SECT_AIR:
            cprintf(ch, "Nothing happens.\r\n");
            act("$n completes $s spell, but nothing happens.", true, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
            break;
        }
        z = 75;
        to_room = "&4A fresh clear spring of water bursts through the ground here.&0";
        to_char = "&4A fresh clear spring of water bursts through the ground here.&0";
        break;
    case SPELL_CREATE_FOOD:
        /* Select an item from 100.obj. There are 50 items, 10 for each
         * create-fooding class. */

        /* First determine the base vnum by class */
        z = 120; /* For clerics, also for default */
        if (GET_CLASS(ch) == CLASS_PALADIN)
            z = 110;
        else if (GET_CLASS(ch) == CLASS_PRIEST)
            z = 100;
        else if (GET_CLASS(ch) == CLASS_ANTI_PALADIN)
            z = 130;
        else if (GET_CLASS(ch) == CLASS_DRUID)
            z = 140;

        zplus = skill / 16 + ((random() % 1000) * (random() % 1000) * 5 * ((random() % 10) < 5 ? 1 : -1)) / 1000000;

        if (zplus < 0)
            zplus = 0;
        if (zplus > 9)
            zplus = 9;
        z += zplus;

        /* Do our best to ensure we have an actual object */

        if (real_object(z) < 0) {
            if (real_object(WAYBREAD_OBJ_1) >= 0)
                z = WAYBREAD_OBJ_1;
            else
                z = WAYBREAD_OBJ_2;
        }

        give_char = 1;
        to_room = "$n creates $p.";
        to_char = "You create $p.";
        break;
    default:
        send_to_char("Spell unimplemented, it would seem.\r\n", ch);
        return 0;
    }

    if (!(tobj = read_object(z, VIRTUAL))) {
        send_to_char("I seem to have goofed.\r\n", ch);
        sprintf(buf, "SYSERR: spell_creations, spell %d, obj %d: obj not found", spellnum, z);
        log(buf);
        return 0;
    }
    if (give_char)
        obj_to_char(tobj, ch);
    else
        obj_to_room(tobj, ch->in_room);
    if (to_room)
        act(to_room, false, ch, tobj, 0, TO_ROOM);
    if (to_char)
        act(to_char, false, ch, tobj, 0, TO_CHAR);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

int mag_room(int skill, CharData *ch, int spellnum) {
    int eff;   /* what effect */
    int ticks; /* how many ticks this spell lasts */
    char *to_char = nullptr;
    char *to_room = nullptr;
    RoomEffectNode *reff;

    ticks = 0;
    eff = -1;

    if (ch == nullptr)
        return 0;

    switch (spellnum) {
    case SPELL_CIRCLE_OF_FIRE:
        if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_CIRCLE_FIRE)) {
            send_to_char("The room is already on fire.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        if (SECT(ch->in_room) == SECT_SHALLOWS || SECT(ch->in_room) == SECT_WATER ||
            SECT(ch->in_room) == SECT_UNDERWATER) {
            send_to_char("Impossible. There is too much water here.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        eff = ROOM_EFF_CIRCLE_FIRE;
        ticks = 2;
        to_char = "&1A ring of fire encircles the area.&0";
        to_room = "&1A ring of fire encircles the area.&0";
        improve_skill(ch, skills[SPELL_CIRCLE_OF_FIRE].sphere);
        break;
    case SPELL_URBAN_RENEWAL:
        if (SECT(ch->in_room) == SECT_FOREST || ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_FOREST)) {
            send_to_char("There is already enough foliage here to constitute a forest.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        eff = ROOM_EFF_FOREST;
        ticks = skill / 10;
        to_char =
            "&2&bThick vines and shrubs sprout out of the ground and cover "
            "the whole area in thick foliage.&0";
        to_room =
            "&2&bThick vines and shrubs sprout out of the ground and cover "
            "the whole area in thick foliage.&0";
        break;
    case SPELL_WALL_OF_FOG:
        to_char = "You create a fog out of nowhere.";
        to_room = "$n creates a fog out of nowhere.";
        eff = ROOM_EFF_FOG;
        ticks = 1; /* this spell lasts one tick */
        break;

        /* add more room spells continual here */

    default:
        sprintf(buf,
                "SYSERR: unknown spellnum %d "
                "passed to mag_unaffect",
                spellnum);
        log(buf);
        return CAST_RESULT_CHARGE;
    }

    /* create, initialize, and link a room-affection node */
    CREATE(reff, RoomEffectNode, 1);
    reff->room = ch->in_room;
    reff->timer = ticks;
    reff->effect = eff;
    reff->spell = spellnum;
    reff->next = room_effect_list;
    room_effect_list = reff;

    /* set the affection */
    if (eff != -1)
        SET_FLAG(ROOM_EFFECTS(reff->room), eff);

    if (to_char == nullptr)
        send_to_char(NOEFFECT, ch);
    else
        act(to_char, true, ch, 0, 0, TO_CHAR);

    if (to_room != nullptr)
        act(to_room, true, ch, 0, 0, TO_ROOM);
    else if (to_char != nullptr)
        act(to_char, true, ch, 0, 0, TO_ROOM);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/* Add spells to the case statement here to make a spell's duration skill
   related the spell system automagically adds 1 to this so values start at 0
   Note that adding the default 1 added to a 0 will make the spell wear off
   when the next TICK (currently 75 secs) expires no matter how close it is.
   This gives the affect of nearly instantaneous expiration of the spell.
   */
int get_spell_duration(CharData *ch, int spellnum) {
    int skill;
    int duration;

    skill = GET_SKILL(ch, skills[spellnum].sphere);
    duration = 0;

    switch (spellnum) {
    case (SPELL_LESSER_ENDURANCE):
        duration = skill / 30;
        if (duration == 0)
            duration = 2;
        break;

    case (SPELL_ENDURANCE):
    case (SPELL_VITALITY):
        duration = skill / 20;
        if (duration == 0 && skill > 0)
            duration = 2;
        if (skill > 95)
            duration = 5;
        break;

    case (SPELL_DRAGONS_HEALTH):
        duration = skill / 10;
        break;

    case (SPELL_GREATER_ENDURANCE):
    case (SPELL_GREATER_VITALITY):
        duration = skill / 15;
        if (skill > 95)
            duration = 7;
        break;

    default:
        log("SYSERR:magic.c:get_spell_duration(): Unknown spell");
    }

    return duration;
}

char *get_vitality_vict_message(int spellnum) {
    int index;
    char *victim_messages[] = {"&4&bYou feel you can endure significant pain!&0",
                               "&4&bYou feel you can endure a bit more pain!&0",
                               "&4&bYou feel you can endure more pain!&0",
                               "&4&bYou feel vitalized!&0",
                               "&4&bYou feel greatly vitalized!&0",
                               "&4&bYou feel the blood of dragons surge through your veins!&0"};

    index = 0;

    if (spellnum == 70) {
        index = 0;
    } else {
        index = spellnum - SPELL_LESSER_ENDURANCE + 1;
    };

    /* Greater Endurance, Lesser E, E, Vit, Greater Vit, Dragon's health */

    return victim_messages[index];
}

int get_vitality_hp_gain(CharData *ch, int spellnum) {
    int skill;
    int hp;

    skill = GET_SKILL(ch, skills[spellnum].sphere);
    hp = 0;

    switch (spellnum) {
    case (SPELL_LESSER_ENDURANCE):
        hp = (pow(skill, 2) / 80) + 25;
        break;

    case (SPELL_ENDURANCE):
        hp = (pow(skill, 2) / 60) + 40;
        break;

    case (SPELL_GREATER_ENDURANCE):
        hp = (pow(skill, 2) / 40) + 50;
        break;

    case (SPELL_VITALITY):
        hp = (pow(skill, 2) / 35) + 75;
        break;

    case (SPELL_GREATER_VITALITY):
        hp = (pow(skill, 2) / 30) + 75;
        break;

    case (SPELL_DRAGONS_HEALTH):
        hp = (pow(skill, 2) / 20) + 90;
        break;

    default:
        log("SYSERR:magic.c:get_vitality_hp_gain(): Unknown spell");
    }

    /* I don't like nice even numbers.  Add some randomness into it. */
    hp -= number(2, 10);

    return hp;
}

/*
 * Function to standardize checking for armor spells.
 * Check it at the beginning of any armor spell and immediately return
 * if it returns 1.
 */
bool affected_by_armor_spells(CharData *victim) {
    return affected_by_spell(victim, SPELL_ARMOR) || affected_by_spell(victim, SPELL_BARKSKIN) ||
           affected_by_spell(victim, SPELL_BONE_ARMOR) || affected_by_spell(victim, SPELL_DEMONSKIN) ||
           affected_by_spell(victim, SPELL_GAIAS_CLOAK) || affected_by_spell(victim, SPELL_ICE_ARMOR) ||
           affected_by_spell(victim, SPELL_MIRAGE);
}

bool spell_suitable_for_fluid_characters(int spellnum) {
    switch (spellnum) {
    case SPELL_BARKSKIN:
    case SPELL_BONE_ARMOR:
    case SPELL_DEMONSKIN:
    case SPELL_GAIAS_CLOAK:
    case SPELL_ICE_ARMOR:
    case SPELL_STONE_SKIN:
    case SPELL_COLDSHIELD:
        return false;
    }
    return true;
}

bool check_fluid_spell_ok(CharData *ch, CharData *victim, int spellnum, bool quiet) {
    if (!victim)
        return false;
    if (RIGID(victim) || GET_LEVEL(victim) >= LVL_IMMORT)
        return true;
    if (!spell_suitable_for_fluid_characters(spellnum)) {
        if (!quiet) {
            cprintf(victim, "The spell is unable to take hold in your substance.\r\n");
            if (ch && ch != victim)
                act("The spell is unable to alter $N's substance.", false, ch, 0, victim, TO_CHAR);
        }
        return false;
    }
    return true;
}

void remove_unsuitable_spells(CharData *ch) {
    effect *eff, *next;

    if (!RIGID(ch)) {
        for (eff = ch->effects; eff; eff = next) {
            next = eff->next;
            if (!spell_suitable_for_fluid_characters(eff->type))
                active_effect_remove(ch, eff);
        }
    }
}

bool check_armor_spells(CharData *ch, CharData *victim, int spellnum) {
    if (affected_by_armor_spells(victim)) {
        if (ch == victim) {
            act("You seem to be protected enough already!", false, ch, 0, 0, TO_CHAR);
            act("$n looks a little overprotective.", true, ch, 0, 0, TO_ROOM);
        } else {
            act("$N seem to be protected enough already!", false, ch, 0, victim, TO_CHAR);
            act("You seem to be protected enough already!", false, ch, 0, victim, TO_VICT);
            act("$n looks a little overprotective.", true, ch, 0, victim, TO_NOTVICT);
        }
        return true;
    }
    return false;
}

void destroy_opposite_wall(ObjData *wall) {
    void decay_object(ObjData * obj);
    int room, dir;
    ObjData *next;

    /* Not in a room? */
    if ((room = wall->in_room) == NOWHERE)
        return;

    dir = GET_OBJ_VAL(wall, VAL_WALL_DIRECTION);

    /* Invalid direction? */
    if (dir < 0 || dir >= NUM_OF_DIRS)
        return;

    /* No exit in that direction? */
    if (!world[room].exits[dir] || EXIT_NDEST(world[room].exits[dir]) == NOWHERE)
        return;

    /* Go through the objects in the other room. */
    for (wall = world[world[room].exits[dir]->to_room].contents; wall; wall = next) {
        next = wall->next_content;
        dir = GET_OBJ_VAL(wall, VAL_WALL_DIRECTION);
        /* If the object is a wall, has a valid direction, and points back to
         * the original room, kill it. */
        if (GET_OBJ_TYPE(wall) == ITEM_WALL && dir >= 0 && dir < NUM_OF_DIRS && world[wall->in_room].exits[dir] &&
            world[wall->in_room].exits[dir]->to_room == room)
            decay_object(wall);
    }
}

/* Returns true if something was seen. */
bool look_at_magic_wall(CharData *ch, int dir, bool sees_next_room) {
    ObjData *wall;
    room_num next_room;

    for (wall = world[ch->in_room].contents; wall; wall = wall->next_content) {
        if (GET_OBJ_TYPE(wall) == ITEM_WALL && GET_OBJ_VAL(wall, VAL_WALL_DIRECTION) == dir) {
            sprintf(buf, "%s &0is standing here.\r\n", wall->short_description);
            CAP(buf);
            send_to_char(buf, ch);
            return true;
        }
    }

    if (sees_next_room) {
        next_room = world[ch->in_room].exits[dir]->to_room;

        if (next_room != NOWHERE) {
            for (wall = world[next_room].contents; wall; wall = wall->next_content)
                if (GET_OBJ_TYPE(wall) == ITEM_WALL &&
                    world[next_room].exits[GET_OBJ_VAL(wall, VAL_WALL_DIRECTION)]->to_room == ch->in_room) {
                    sprintf(buf, "You see %s&0 a short distance away.\r\n", wall->short_description);
                    send_to_char(buf, ch);
                    return true;
                }
        }
    }

    return false;
}

ObjData *find_wall_dir(int rnum, int dir) {
    ObjData *wall;

    for (wall = world[rnum].contents; wall; wall = wall->next_content)
        if (GET_OBJ_TYPE(wall) == ITEM_WALL && GET_OBJ_VAL(wall, VAL_WALL_DIRECTION) == dir)
            return wall;
    return nullptr;
}

/* Returns true if you were stopped by a magic wall. */
bool wall_block_check(CharData *actor, CharData *motivator, int dir) {
    ObjData *wall;

    if (GET_LEVEL(actor) >= LVL_GOD)
        return false;
    if ((wall = find_wall_dir(actor->in_room, dir)) != nullptr) {
        /* Found a wall; you'll be blocked. */

        /* See if a wall of ice will put out anyone's flames. */
        if (GET_OBJ_VAL(wall, VAL_WALL_SPELL) == SPELL_WALL_OF_ICE && EFF_FLAGGED(motivator, EFF_ON_FIRE)) {
            act("$n&0 spreads $mself out on $p&0 and with a &8&bsizzle&0, $s flames "
                "are put out.",
                false, motivator, wall, 0, TO_ROOM);
            act("You spread yourself out on $p&0 and your flames go out in a "
                "&8&bsizzle of steam&0.",
                false, motivator, wall, 0, TO_CHAR);
            REMOVE_FLAG(EFF_FLAGS(motivator), EFF_ON_FIRE);
        } else if (GET_OBJ_VAL(wall, VAL_WALL_SPELL) == SPELL_WALL_OF_ICE && EFF_FLAGGED(actor, EFF_ON_FIRE)) {
            act("$n&0 spreads $mself out on $p&0 and with a &8&bsizzle&0, $s flames "
                "are put out.",
                false, actor, wall, 0, TO_ROOM);
            act("You spread yourself out on $p&0 and your flames go out in a "
                "&8&bsizzle of steam&0.",
                false, actor, wall, 0, TO_CHAR);
            REMOVE_FLAG(EFF_FLAGS(actor), EFF_ON_FIRE);

            /* No flames being put out.  Is this a mounted situation? */
        } else if (actor && motivator && actor != motivator && RIDING(actor) == motivator) {
            act("You rode $N right into $p!", false, actor, wall, motivator, TO_CHAR);
            act("Bump!  $n rides $N into $p.", false, actor, wall, motivator, TO_ROOM);

            /* Just your standard walk-into-a-wall. */
        } else {
            act("Oof.  You bump into $p.", false, actor, wall, 0, TO_CHAR);
            act("$n bumps into $p.", false, actor, wall, 0, TO_ROOM);
        }
        return true;
    }
    return false;
}

/* Returns true if you ran into a wall. */
bool wall_charge_check(CharData *ch, int dir) {
    ObjData *wall;
    int dam, chance;

    if ((wall = find_wall_dir(ch->in_room, dir)) == nullptr)
        return false;

    /* Found a wall for you to run into! */

    /* If walls were being damaged, this one should get hurt now.
     * It might also show visible cracking or even be destroyed. */

    act("You CHARGE at $p&0 and crash right into it!", false, ch, wall, 0, TO_CHAR);
    act("$n &0CHARGES at $p&0 and crashes into it headfirst!", false, ch, wall, 0, TO_ROOM);

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        /* You're going to get hurt... */

        chance = number(0, 101);
        dam = ((chance / 10) * (GET_LEVEL(ch) / 10)) + GET_LEVEL(ch);
        /* But you won't die... */
        if (GET_HIT(ch) - dam < -5)
            dam = GET_HIT(ch) + 5;
        hurt_char(ch, nullptr, dam, true);
        /* You fell to a sitting position (unless you were knocked out) */
        if (GET_POS(ch) >= POS_STANDING)
            alter_pos(ch, POS_SITTING, STANCE_ALERT);
        WAIT_STATE(ch, PULSE_VIOLENCE * 3);
    }

    /* Would like to send messages to the other room that the wall's in, but
     * we'd have to check each person to see if they were awake, can see the
     * wall, ... maybe send a message about sound if they can't see but are
     * awake, etc. */

    return true;
}

int get_fireshield_damage(CharData *attacker, CharData *victim, int dam) {
    if (EFF_FLAGGED(attacker, EFF_MAJOR_GLOBE))
        act("&1&bThe globe around your body absorbs the burning flames!&0", false, attacker, 0, 0, TO_CHAR);
    else {
        int amount = MIN((GET_LEVEL(victim) / 2 + number(1, GET_LEVEL(victim) / 10)),
                         dam / 3 + number(1, 1 + GET_LEVEL(victim) / 10));
        amount = dam_suscept_adjust(victim, attacker, nullptr, amount, DAM_FIRE);
        if (amount > 0) {
            act("&1Your limbs are seared by $N&0&1's shield of flames.&0 (&1&8$i&0)", false, attacker, amount, victim,
                TO_CHAR);
            act("&1$n&0&1's limbs are seared by your shield of flames.&0 (&3$i&0)", false, attacker, amount, victim,
                TO_VICT);
            act("&1$n&0&1's limbs are seared by $N&0&1's shield of flames.&0 "
                "(&4$i&0)",
                false, attacker, amount, victim, TO_NOTVICT);
        }
        return amount;
    }

    return 0;
}

int get_coldshield_damage(CharData *attacker, CharData *victim, int dam) {
    if (EFF_FLAGGED(attacker, EFF_MAJOR_GLOBE))
        act("&4&bThe globe around your body absorbs the killing ice!&0", false, attacker, 0, 0, TO_CHAR);
    else {
        int amount = MIN((GET_LEVEL(victim) / 2 + number(1, 1 + GET_LEVEL(victim) / 10)),
                         dam / 3 + number(1, 1 + GET_LEVEL(victim) / 10));
        amount = dam_suscept_adjust(victim, attacker, nullptr, amount, DAM_COLD);
        if (amount > 0) {
            act("&4You are impaled on $N&0&4's shield of ice.&0 (&1&8$i&0)", false, attacker, amount, victim, TO_CHAR);
            act("&4$n&0&4 is impaled on your shield of ice.&0 (&3$i&0)", false, attacker, amount, victim, TO_VICT);
            act("&4$n&0&4 is impaled on $N&0&4's shield of ice.&0 (&4$i&0)", false, attacker, amount, victim,
                TO_NOTVICT);
        }
        return amount;
    }

    return 0;
}

int get_soulshield_damage(CharData *attacker, CharData *victim, int dam) {
    if ((IS_GOOD(attacker) && IS_EVIL(victim)) || (IS_EVIL(attacker) && IS_GOOD(victim))) {
        int amount = MIN(2 * GET_LEVEL(victim) / 5 + number(1, 1 + GET_LEVEL(victim) / 10),
                         3 * dam / 16 + number(1, 1 + GET_LEVEL(victim) / 10));
        amount = dam_suscept_adjust(victim, attacker, nullptr, amount, DAM_ALIGN);
        if (amount > 0) {
            act("&7&b$n's soul suffers upon contact with your aura.&0 (&3$i&0)", true, attacker, amount, victim,
                TO_VICT);
            act("&7&bYour soul suffers upon contact with $N's aura.&0 (&1&8$i&0)", true, attacker, amount, victim,
                TO_CHAR);
            act("&7&b$n's soul suffers upon contact with $N's aura.&0 (&4$i&0)", true, attacker, amount, victim,
                TO_NOTVICT);
        }
        return amount;
    }

    return 0;
}

int defensive_spell_damage(CharData *attacker, CharData *victim, int dam) {
    int shdam = 0;

    if (EFF_FLAGGED(victim, EFF_FIRESHIELD))
        shdam += get_fireshield_damage(attacker, victim, dam);
    if (EFF_FLAGGED(victim, EFF_COLDSHIELD))
        shdam += get_coldshield_damage(attacker, victim, dam);
    if (EFF_FLAGGED(victim, EFF_SOULSHIELD))
        shdam += get_soulshield_damage(attacker, victim, dam);

    return shdam;
}