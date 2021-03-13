/***************************************************************************
 * $Id: magic.c,v 1.296 2010/06/05 18:58:47 mud Exp $
 ***************************************************************************/
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

#include "magic.h"

#include "casting.h"
#include "chars.h"
#include "charsize.h"
#include "comm.h"
#include "composition.h"
#include "conf.h"
#include "constants.h"
#include "damage.h"
#include "db.h"
#include "directions.h"
#include "events.h"
#include "exits.h"
#include "fight.h"
#include "handler.h"
#include "lifeforce.h"
#include "limits.h"
#include "math.h"
#include "movement.h"
#include "races.h"
#include "regen.h"
#include "skills.h"
#include "spells.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

#include <math.h>

extern int mini_mud;
extern int pk_allowed;
extern int sleep_allowed;
extern int summon_allowed;
extern int charm_allowed;
extern int roomeffect_allowed;

int real_mobile(int);

void half_chop(char *string, char *arg1, char *arg2);
int is_abbrev(char *arg1, char *arg2);
bool is_grouped(struct char_data *ch, struct char_data *tch);
void add_follower(struct char_data *ch, struct char_data *leader);
int dice(int number, int size);
int get_spell_duration(struct char_data *ch, int spellnum);
int get_vitality_hp_gain(struct char_data *ch, int spellnum);
char *get_vitality_vict_message(int spellnum);
bool check_armor_spells(struct char_data *ch, struct char_data *victim, int spellnum);

struct char_data *read_mobile(int, int);

/* See whether someone evades a spell entirely, for the following reasons:
 *
 * -- major/minor globe
 * -- elemental immunity
 * -- immortal victim
 */

bool evades_spell(struct char_data *caster, struct char_data *vict, int spellnum, int power) {
    int sus;

    /* Non-violent spells don't need to be evaded. */
    if (!SINFO.violent)
        return FALSE;

    /* Dispel magic is a special case */
    if (spellnum == SPELL_DISPEL_MAGIC)
        return FALSE;

    /* Major/minor globe */
    if (EFF_FLAGGED(vict, EFF_MINOR_GLOBE) || EFF_FLAGGED(vict, EFF_MAJOR_GLOBE)) {

        /* Minor globe blocks circle 3 and below.  Major globe blocks 6 and down. */
        if ((EFF_FLAGGED(vict, EFF_MINOR_GLOBE) && SINFO.lowest_level <= CIRCLE_3) ||
            (EFF_FLAGGED(vict, EFF_MAJOR_GLOBE) && SINFO.lowest_level <= CIRCLE_6)) {
            act("&1&bThe shimmering globe around your body flares as the spell flows "
                "around it.&0",
                FALSE, caster, 0, vict, TO_VICT);
            act("&1&bThe shimmering globe around $N&1&b's body flares as your spell "
                "flows around it.&0",
                FALSE, caster, 0, vict, TO_CHAR);
            act("&1&bThe shimmering globe around $N&1&b's body flares as $n&1&b's "
                "spell flows around it.&0",
                FALSE, caster, 0, vict, TO_NOTVICT);
            return TRUE;
        }
    }

    if (skills[spellnum].damage_type == DAM_UNDEFINED)
        return FALSE;

    sus = susceptibility(vict, skills[spellnum].damage_type);

    /* If your susceptibility is Zero, we'll stop this thing right now, so
     * that immunity can block effects. */
    if (sus == 0) {
        if (caster == vict) {
            act("&6$n's&6 spell has no effect on $m.&0", FALSE, caster, 0, vict, TO_NOTVICT);
            act("&6Your spell has no effect on you.&0", FALSE, caster, 0, vict, TO_CHAR);
        } else {
            act("&6$n's&6 spell has no effect on $N.&0", FALSE, caster, 0, vict, TO_NOTVICT);
            act("&6Your spell has no effect on $N!&0", FALSE, caster, 0, vict, TO_CHAR);
            act("&6$n's&6 spell has no effect on you!&0", FALSE, caster, 0, vict, TO_VICT);
        }
        return TRUE;
    }

    /* Are you trying to harm or disable an immortal? */
    if (!IS_NPC(vict) && GET_LEVEL(vict) >= LVL_IMMORT) {
        /* This will cause the "You're trying to silence a god? Ha!" message
         * to be sent */
        if (!skill_message(0, caster, vict, spellnum, FALSE)) {
            /* There's no specific message for this spell - send generic
             * messages instead */
            /* to caster */
            act("$N ignores your feeble spell.", FALSE, caster, 0, vict, TO_CHAR);
            /* to victim */
            act("You ignore $n's feeble spell.", FALSE, caster, 0, vict, TO_VICT);
            /* to room */
            act("$N ignores $n's feeble spell.", FALSE, caster, 0, vict, TO_NOTVICT);
        }
        return TRUE;
    }

    if (skills[spellnum].routines & (MAG_DAMAGE | MAG_MANUAL))
        /* For spells that do actual damage, evasion will be checked during
         * the actual damage() call. */
        return FALSE;

    /* Stuff like word of command would fall into this category:
     *   has a damage type, but doesn't do physical damage */
    if (boolean_attack_evasion(vict, power, skills[spellnum].damage_type)) {
        act("&6$n's&6 spell passes over $N harmlessly.&0", FALSE, caster, 0, vict, TO_NOTVICT);
        act("&6Your spell passes over $N harmlessly!&0", FALSE, caster, 0, vict, TO_CHAR);
        act("&6$n's&6 spell passes over you harmlessly!&0", FALSE, caster, 0, vict, TO_VICT);
        set_fighting(vict, caster, FALSE);
        return TRUE;
    }

    return FALSE;
}

void abort_casting(struct char_data *ch) {
    if (CASTING(ch)) {
        STOP_CASTING(ch);
        /* Don't say they stop chanting if they've been knocked
         * out or killed - it looks funny. */
        if (!AWAKE(ch))
            return;
        act("You stop chanting abruptly!", FALSE, ch, 0, 0, TO_CHAR);
        act("$n stops chanting abruptly!", FALSE, ch, 0, 0, TO_ROOM);
    }
}

struct char_data *check_guard(struct char_data *ch, struct char_data *victim, int gag_output) {
    if (!ch || !victim)
        return NULL;
    if (ch->casting.target_status != TARGET_ALL_ROOM && victim->guarded_by &&
        victim->guarded_by->in_room == victim->in_room && CAN_SEE(victim->guarded_by, victim) &&
        GET_SKILL(victim->guarded_by, SKILL_GUARD) && !CHECK_WAIT(victim->guarded_by) &&
        GET_POS(victim->guarded_by) >= POS_STANDING && GET_STANCE(victim->guarded_by) >= STANCE_ALERT &&
        attack_ok(ch, victim->guarded_by, FALSE)) {
        improve_skill(victim->guarded_by, SKILL_GUARD);
        if (GET_ISKILL(victim->guarded_by, SKILL_GUARD) > number(1, 1100)) {
            if (!gag_output) {
                act("$n jumps in front of $N, shielding $M from the assault.", FALSE, victim->guarded_by, 0, victim,
                    TO_NOTVICT);
                act("$n jumps in front of you, shielding you from the assault.", FALSE, victim->guarded_by, 0, victim,
                    TO_VICT);
                act("You jump in front of $N, shielding $M from the assault.", FALSE, victim->guarded_by, 0, victim,
                    TO_CHAR);
            }
            return victim->guarded_by;
        } else if (!gag_output) {
            act("$n tries to intercept the attack on $N, but isn't quick enough.", FALSE, victim->guarded_by, 0, victim,
                TO_NOTVICT);
            act("$n tries to shield you from the attack, but can't move fast enough.", FALSE, victim->guarded_by, 0,
                victim, TO_VICT);
            act("You try to block the attack on $N, but aren't quick enough.", FALSE, victim->guarded_by, 0, victim,
                TO_CHAR);
        }
    }
    return victim;
}

int mag_savingthrow(struct char_data *ch, int type) {
    int get_base_saves(struct char_data * ch, int type);
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
        return TRUE;

    return FALSE;
}

/* Decrease modifier is used to decrease the modifiers for
 * stone skin and bone draw by 1
 *  Can be used by any effect really with minor code change
 */
void decrease_modifier(struct char_data *i, int spell) {
    struct effect *eff, *tmp;

    for (eff = i->effects; eff; eff = tmp) {
        tmp = eff->next;
        if (eff->type == spell) {
            if (spell == SPELL_BONE_DRAW) {
                act("One of the bones locking you in place shatters under the attack!", FALSE, i, 0, 0, TO_CHAR);
                act("One of the bones locking $n in place shatters under the attack!", FALSE, i, 0, 0, TO_ROOM);
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
    struct room_effect_node *reff, *next_reff, *temp;
    static struct effect *eff, *next;
    static struct char_data *i;

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
            act("$n freezes and falls twitching to the ground.", FALSE, i, 0, 0, TO_ROOM);
            die(i, NULL);
        }
        /* if the mob was an illusion and its magic ran out, get rid of it */
        if (MOB_FLAGGED(i, MOB_ILLUSORY) && !EFF_FLAGGED(i, EFF_ANIMATED)) {
            act("$n dissolves into tiny multicolored lights that float away.", TRUE, i, 0, 0, TO_ROOM);
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

void remove_char_spell(struct char_data *ch, int spellnum) {
    static struct effect *eff, *next;

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
int mag_material(struct char_data *ch, int item0, int item1, int item2, int extract, int verbose) {
    struct obj_data *tobj;
    struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;

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
        return (FALSE);
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
        act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
    }
    return (TRUE);
}

/* A standardized calculation for single-target sorcerer spells.
 *
 * This is calibrated such that sorcerer damage will be 120-110%
 * as much as warrior damage at any given level.
 * It also depends on the casting times of these spells
 * being specific values. */
int sorcerer_single_target(struct char_data *ch, int spell, int power) {
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

int mag_damage(int skill, struct char_data *ch, struct char_data *victim, int spellnum, int savetype) {
    EVENTFUNC(battle_paralysis_handler);
    int dam = 0;
    int temp = 0;
    double dmod;
    int reduction = FALSE;
    int sus;
    int damage_spellnum = spellnum;

    if (victim == NULL || ch == NULL)
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
     *  - If SD_INTERN_DAM(i) = FALSE then it will look for a internal switch
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
    case SPELL_BIGBYS_CLENCHED_FIST:
    case SPELL_IRON_MAIDEN:
    case SPELL_FREEZE:
    case SPELL_ACID_BURST:
    case SPELL_DISINTEGRATE:
    case SPELL_ICEBALL:
        dam = sorcerer_single_target(ch, spellnum, skill);
        reduction = TRUE;
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
        reduction = TRUE;
        break;
    case SPELL_DISPEL_MAGIC:
        /* We don't want to hear about folks being immune to dispel magic,
         * because the spell might have been cast to remove spells from them.
         * So get out of here if the victim is immune. */
        if (sus == 0)
            return 0;
        dam = sorcerer_single_target(ch, spellnum, skill);
        reduction = TRUE;
        break;
    case SPELL_DISCORPORATE:
        if (MOB_FLAGGED(victim, MOB_ILLUSORY))
            /* Discorporate just annihilates illusory mobs */
            dam = 100 + GET_HIT(victim);
        else
            dam = sorcerer_single_target(ch, spellnum, skill) / 5;
        break;
    case SPELL_IMMOLATE:
        /* Immolate hits 5 times. */
        dam = sorcerer_single_target(ch, spellnum, skill) / 4;
        reduction = TRUE;
        break;
    case SPELL_PHOSPHORIC_EMBERS:
        /* hits 4 times. */
        dam = sorcerer_single_target(ch, spellnum, skill) / 3;
        reduction = TRUE;
        break;
    case SPELL_PYRE:
        /* hits 4 times, but charmies do half damage */
        dam = sorcerer_single_target(ch, spellnum, skill) / 3;
        if (EFF_FLAGGED(ch, EFF_CHARM) && ch->master && IS_PC(ch->master))
            dam *= 2;
        reduction = TRUE;
        break;
    case SPELL_PYRE_RECOIL:
        dam = sorcerer_single_target(ch, SPELL_PYRE, skill) / 6;
        /* use the SPELL_ON_FIRE damage message */
        damage_spellnum = SPELL_ON_FIRE;
        reduction = TRUE;
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

    case SPELL_MAGIC_MISSILE:
    case SPELL_ICE_DARTS:
        dam = dice(4, 21);
        reduction = TRUE;
        break;
    case SPELL_FIRE_DARTS:
        dam = dice(5, 18);
        reduction = TRUE;
        break;
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
        break;
    case SPELL_HELL_BOLT:
        /* max dam 110 from max 8d6+9 online, except alignment bonus! */
        dam += (pow(skill, 2) * 53) / 10000;
        dam *= GET_ALIGNMENT(victim) * 0.0007 + 0.8;
        break;
    case SPELL_DIVINE_BOLT:
        /* max dam 110 from max 8d6+9 online, except alignment bonus! */
        dam += (pow(skill, 2) * 53) / 10000;
        dam *= GET_ALIGNMENT(victim) * -0.0007 + 0.8;
        break;
    case SPELL_WRITHING_WEEDS:
        /* max dam 110 from max 8d6+9 online */
        dam += (pow(skill, 2) * 53) / 10000;
        break;
    case SPELL_FLAMESTRIKE:
        /* max dam 80 for neutral from max 8d9+8 online */
        if (IS_EVIL(ch)) {
            send_to_char("You are not holy enough to cast that spell!\r\n", ch);
            return 0;
        }
        dam *= (GET_ALIGNMENT(ch) * 0.0007) + 0.8;
        break;
    case SPELL_DISPEL_EVIL:
        if (IS_GOOD(victim)) {
            act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
            act("You leer at $n as $e attempts to dispel your evilness.", FALSE, ch, 0, victim, TO_VICT);
            act("$n tries to make the evil in Saint $N suffer.", TRUE, victim, 0, ch, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        } else if (IS_NEUTRAL(victim)) {
            act("Yeah, right there fancy pants.  $U$N doesn't seem to care.", FALSE, ch, 0, victim, TO_CHAR);
            act("You don't seem to care that $N is attempting to dispel your "
                "evilness.",
                FALSE, ch, 0, victim, TO_VICT);
            act("$N doesn't care that $n is trying to make $S evilness suffer.", TRUE, victim, 0, ch, TO_NOTVICT);
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
            act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
            act("You leer at $n as $e attempts to dispel your goodness.", FALSE, ch, 0, victim, TO_VICT);
            act("$N leers at $n as $e tries to rot $S goodness.", TRUE, victim, 0, ch, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        } else if (IS_NEUTRAL(victim)) {
            act("Yeah, right there fancy pants.  $U$N doesn't seem to care.", FALSE, ch, 0, victim, TO_CHAR);
            act("You don't seem to care that $N is attempting to dispel your "
                "goodness.",
                FALSE, ch, 0, victim, TO_VICT);
            act("$N doesn't care that $n is trying to make $S goodness suffer.", TRUE, victim, 0, ch, TO_NOTVICT);
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
    case SPELL_EARTHQUAKE:
        if (!QUAKABLE(CH_NROOM(victim)))
            return CAST_RESULT_CHARGE;
        /* max dam 125 from max 8d7+12 online */
        dam += (pow(skill, 2) * 13) / 2500;
        dam = sectors[CH_SECT(victim)].qdam_mod * dam / 100;
        temp = sectors[CH_SECT(victim)].fall_mod; /* Modifier for likelihood to be knocked down */
        if (GET_POS(victim) == POS_FLYING) {
            act("$N doesn't care that you are making the ground shake.", TRUE, ch, 0, victim, TO_CHAR);
            act("You don't seem to care that $n is attempting to knock you to the "
                "ground.",
                TRUE, ch, 0, victim, TO_VICT);
            act("$N doesn't seem to care that $n is shaking the ground.", TRUE, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }
        /* Do you fall down?  Levitate and high dex prevent it */
        if (!EFF_FLAGGED(victim, EFF_LEVITATE) && number(1, 100) >= GET_DEX(victim) - temp) {
            GET_POS(victim) = POS_SITTING;
            GET_STANCE(victim) = STANCE_ALERT;
        } else {
            act("$N doesn't care that you are making the ground shake.", TRUE, ch, 0, victim, TO_CHAR);
            act("$n is unable to knock you to the ground.", TRUE, ch, 0, victim, TO_VICT);
            act("$N manages to keep $S footing.", TRUE, ch, 0, victim, TO_NOTVICT);
        }
        /* Levitate - cuts damage in half */
        if (EFF_FLAGGED(victim, EFF_LEVITATE))
            dam /= 2;
        break;
    case SPELL_COLOR_SPRAY:
        /* max dam 190 from 15d5+45 online */
        dam += (pow(skill, 2) * 1) / 200;
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
    case SPELL_DESTROY_UNDEAD:
        if (GET_LIFEFORCE(victim) != LIFE_UNDEAD) {
            act("$N has far too much life for you to harm it!", FALSE, ch, 0, victim, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }
        /* max dam 400 from 18d5+40 online */
        dam += (pow(skill, 2) * 27) / 1000;
        if (GET_CLASS(ch) == CLASS_PRIEST)
            dam *= 1.25;
        break;
    case SPELL_ICE_STORM:
        /* max dam 235 from 20d5+35 online */
        dam += (pow(skill, 2) * 1) / 100;
        if (GET_CLASS(ch) == CLASS_CRYOMANCER)
            dam *= 1.25;
        break;
    case SPELL_FIRESTORM:
        /* Mirror spell of ice storm */
        /* max dam 235 from 20d5+35 online */
        dam += (pow(skill, 2) * 1) / 100;
        if (GET_CLASS(ch) == CLASS_PYROMANCER)
            dam *= 1.25;
        break;
    case SPELL_UNHOLY_WORD:
        /* max dam 300 from 20d5+35 online */
        dam += (pow(skill, 2) * 33) / 2000;
        if (GET_CLASS(ch) == CLASS_DIABOLIST || GET_CLASS(ch) == CLASS_ANTI_PALADIN)
            dam *= 1.25;
        break;
    case SPELL_HOLY_WORD:
        /* max dam 300 from 20d5+35 online */
        dam += (pow(skill, 2) * 33) / 2000;
        if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_PALADIN)
            dam *= 1.25;
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
    case SPELL_DIVINE_RAY:
        /* This spell appears to be the goodie version of stygian eruption */
        /* max dam 176 from 12d5+36 online */
        dam += (pow(skill, 2) * 1) / 125;
        dam *= (GET_ALIGNMENT(ch) * 0.0022) - 0.7;
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
                FALSE, ch, 0, victim, TO_CHAR);
            act("$N &7&blets out a massive howl as $E is banished by $n's&7&b "
                "command.&0",
                FALSE, ch, 0, victim, TO_ROOM);
            if (!MOB_FLAGGED(ch, MOB_ILLUSORY)) { /* illusions don't really banish */
                event_create(EVENT_EXTRACT, extract_event, victim, FALSE, &(victim->events), 0);
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
        act("$n looks slightly diminished.", FALSE, ch, 0, 0, TO_ROOM);
        act("You lose some life in libation of your holy allegiance!", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SPELL_CALL_LIGHTNING:
        /* There needs to be some code referencing weather to make this spell
           only work when the weather is bad.  If this happens when we can jack
           the damage of the spell since it will be significantly more rare when
           it can be cast. RSD 4/4/00 */
        /* max dam 300 from 15d5+50 online */
        dam += (pow(skill, 2) * 7) / 400;
        break;
    case SPELL_SUNRAY:
        dam += (pow(skill, 2) * 7) / 400;
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
    case SPELL_CHAIN_LIGHTNING:
        /* max dam 226 from 12d5+26 online */
        dam += (pow(skill, 2) * 7) / 500;
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
                FALSE, ch, 0, victim, TO_ROOM);
            act("$N &7&blets out a massive &1howl&7 as $E is banished by your holy "
                "might.&0",
                FALSE, ch, 0, victim, TO_CHAR);
            if (!MOB_FLAGGED(ch, MOB_ILLUSORY)) { /* illusions don't really banish */
                event_create(EVENT_EXTRACT, extract_event, victim, FALSE, &(victim->events), 0);
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
        act("You lose some life in libation of your holy allegiance!", FALSE, ch, 0, victim, TO_CHAR);
        act("$n looks slightly diminished.", FALSE, ch, 0, victim, TO_ROOM);
        break;
    case SPELL_METEORSWARM:
        /* max dam 1200 from 50d10+50 online */
        dam += (pow(skill, 2) * 13) / 200;
        break;
    case SPELL_FLOOD:
        /* max dam 1200 from 50d10+50 online */
        dam += (pow(skill, 2) * 13) / 200;
        break;
    case SPELL_SUPERNOVA:
        /* max dam 2500 from 100d10+500 online */
        dam += (pow(skill, 2) * 1) / 10;
        break;
    case SPELL_ICE_SHARDS:
        /* max dam 2500 from 100d10+500 online */
        dam += (pow(skill, 2) * 1) / 10;
        break;
    case SPELL_SEVERANCE:
        dam += (pow(skill, 2) * 13) / 200;
        break;
    case SPELL_SOUL_REAVER:
        dam += (pow(skill, 2) * 13) / 199;
        break;
    case SPELL_VAMPIRIC_BREATH:
        dam += dice(2, skill + 10);
        if (skill >= 95)
            dam += number(0, 70);
        GET_HIT(ch) += dam;
        break;
    case SPELL_LIGHTNING_BREATH:
    case SPELL_FIRE_BREATH:
    case SPELL_FROST_BREATH:
    case SPELL_GAS_BREATH:
    case SPELL_ACID_BREATH:
        dam = skill + number(1, skill * 2);
        break;
    case SPELL_CIRCLE_OF_FIRE:
        dam = (skill / 2) + dice(2, 3);
        break;
    case SPELL_DEGENERATION:
        dam += dice(3, 40) + 300;
        break;
    case SPELL_MOONBEAM:
        dam += skill * 2 + number(20, 80);
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

    victim = check_guard(ch, victim, FALSE);

    if (!attack_ok(ch, victim, TRUE))
        return CAST_RESULT_CHARGE;

    /* divide damage by two if victim makes his saving throw */
    if (mag_savingthrow(victim, savetype))
        dam >>= 1;

    /* if HARNESS spell is active, do extra 1% per lvl of foe */
    if (EFF_FLAGGED(ch, EFF_HARNESS)) {
        dam += (dam * GET_LEVEL(victim)) / 100;
        act("&5&b$n&5&b executes $s spell with amazing force...&0", FALSE, ch, 0, 0, TO_ROOM);
        act("&5&bYou execute your spell with amazing force...&0", FALSE, ch, 0, 0, TO_CHAR);
        effect_from_char(ch, SPELL_HARNESS);
    }

    /* Adjust the damage according to the susceptibility */
    dam = dam * sus / 100;

    if (sus > 119) {
        /* Cry out if you're highly vulnerable */
        if (number(1, 4) == 1)
            act("$n cries out in pain!", TRUE, victim, 0, 0, TO_ROOM);
    } else if (sus > 104) {
        /* Express pain if you're very vulnerable */
        if (number(1, 4) == 1)
            act("$n cringes with a pained look on $s face.", TRUE, victim, 0, 0, TO_ROOM);
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
                    act("&1&8$n bursts into flame!&0", FALSE, victim, 0, 0, TO_ROOM);
                    send_to_char("&1&8Your skin and clothes ignite into flame!&0\r\n", victim);
                    break;
                case 2:
                    sprintf(buf, "%s light%s", skills[spellnum].name,
                            skills[spellnum].name[strlen(skills[spellnum].name) - 1] == 's' ? "" : "s");
                    act("&1&8$n's $t you on fire!&0", FALSE, ch, (void *)buf, victim, TO_VICT);
                    act("&1&8$n's $t $N on fire!&0", FALSE, ch, (void *)buf, victim, TO_NOTVICT | TO_VICTROOM);
                    act("&1&8Your $t $N on fire!&0", FALSE, ch, (void *)buf, victim, TO_CHAR);
                    break;
                case 3:
                    send_to_char("&1&8Flames spread across your body!&0\r\n", victim);
                    act("&1&8Flames envelope $n!&0", FALSE, victim, 0, 0, TO_ROOM);
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
                act("&4&b$n&4&b freezes up!&0", TRUE, victim, 0, 0, TO_ROOM);
                send_to_char("&4&bYour joints stiffen as the frost penetrates you!&0\r\n", victim);
                STOP_CASTING(victim);
                event_create(EVENT_BATTLE_PARALYSIS, battle_paralysis_handler, mkgenericevent(ch, victim, 0), TRUE,
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
 * NOTE: If accum_duration is set to TRUE, it will override refresh.
 *
 * Return value: CAST_RESULT_ flags.
 */

#define MAX_SPELL_EFFECTS 4 /* change if more needed */

int mag_affect(int skill, struct char_data *ch, struct char_data *victim, int spellnum, int savetype, int casttype) {
    struct effect eff[MAX_SPELL_EFFECTS];
    struct effect *effect = NULL;
    bool accum_effect = FALSE, accum_duration = FALSE, is_innate = FALSE, refresh = TRUE;
    char *to_vict = NULL, *to_room = NULL, *to_char = NULL;
    int i;

    if (victim == NULL || ch == NULL)
        return 0;
    if (MOB_FLAGGED(ch, MOB_ILLUSORY) && ch != victim)
        return 0;
    if (!check_fluid_spell_ok(ch, victim, spellnum, FALSE))
        return CAST_RESULT_CHARGE;
    if (ch->casting.misc && *ch->casting.misc)
        half_chop(ch->casting.misc, buf, buf2);

    memset(eff, 0, sizeof(eff));
    for (i = 0; i < MAX_SPELL_EFFECTS; i++)
        eff[i].type = spellnum;

    if (GET_LEVEL(victim) >= LVL_IMMORT && GET_LEVEL(ch) < GET_LEVEL(victim)) {
        act("Your spell is too weak to affect $N.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n's spell has no effect on $N.", TRUE, ch, 0, victim, TO_NOTVICT);
        act("$n's spell has no effect on you.", FALSE, ch, 0, victim, TO_VICT);
        return CAST_RESULT_CHARGE;
    }

    switch (spellnum) {

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

    case SPELL_CONFUSION:
        /* Check for resistance due to high wis/dex.
         * Up to an additional 10% chance to evade. */
        i = (GET_DEX(victim) + GET_WIS(victim) - 100) / 10;
        if (i > number(1, 100)) {
            act("$n's eyes start to cross, but $e shakes it off.", TRUE, victim, 0, 0, TO_ROOM);
            send_to_char("Your eyes start to &5spin off&0 in different directions, but you manage\r\n", ch);
            send_to_char("to bring them back under control.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        SET_FLAG(eff[0].flags, EFF_CONFUSION);
        eff[0].duration = 2 + skill / 40;
        to_vict = "&5You suddenly find it difficult to focus upon your foes.&0";
        to_room = "$N can't decide which way to cross $S eyes!";
        break;

    case SPELL_DARK_PRESENCE:

        if (GET_LEVEL(ch) < LVL_IMMORT && !IS_EVIL(ch) && casttype == CAST_SPELL) {
            send_to_char("In your goodness, the dark gods have forsaken you!\r\n", ch);
            act("Nothing happens.  $U$n looks rather forlorn.", TRUE, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        if (affected_by_spell(victim, SPELL_BLESS)) {
            act("$N is already blessed by some other gods.", FALSE, ch, 0, victim, TO_CHAR);
            act("$n looks a little overprotective.", TRUE, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        /* Alignment Checks! */
        if (IS_GOOD(victim)) {
            act("You can't protect an evil ally if they are GOOD!", FALSE, ch, 0, 0, TO_CHAR);
            act("$n tries to enrage your inner demon.\r\nSilly isn't $e?", FALSE, ch, 0, victim, TO_VICT);
            act("$n fails to enrage $N's inner demon.", TRUE, ch, 0, victim, TO_NOTVICT);
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
            act("You can't protect an evil ally if they are GOOD!", FALSE, ch, 0, 0, TO_CHAR);
            act("$n tries to wrap you in demonic skin!\r\nSilly isn't $e?", FALSE, ch, 0, victim, TO_VICT);
            act("$n fails to wrap $N is a demonic skin of protection.", TRUE, ch, 0, victim, TO_NOTVICT);
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

    case SPELL_DISEASE:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;

        if (mag_savingthrow(victim, SAVING_SPELL)) {
            act("You resist $n's foul incantation!", FALSE, ch, 0, victim, TO_VICT);
            act("$N holds $S breath avoiding $n's diseased air!", FALSE, ch, 0, victim, TO_NOTVICT);
            act("$N resists your disease!", TRUE, ch, 0, victim, TO_CHAR);
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

    case SPELL_ENTANGLE:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;

        /* A difficult spell to land with success */
        if (mag_savingthrow(victim, SAVING_PARA) || skill - GET_LEVEL(victim) < number(0, 80)) {
            act("&2&bYour crop of ripe vines search in vain for $N.&0", FALSE, ch, 0, victim, TO_CHAR);
            act("&2&bA crop of ripe vines snakes along the ground, unable to locate "
                "you!&0",
                FALSE, ch, 0, victim, TO_VICT);
            act("&2&bA crop of ripe vines searches in vain for $N.&0", TRUE, ch, 0, victim, TO_NOTVICT);
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
        refresh = FALSE;
        break;

    case SPELL_FAMILIARITY:
        SET_FLAG(eff[0].flags, EFF_FAMILIARITY);
        eff[0].duration = skill / 5 + 4;
        to_vict = "&7&bAn aura of comfort and solidarity surrounds you.&0";
        to_room =
            "You know in your heart that $N is a steady friend, to be "
            "depended upon.";
        break;

    case SPELL_GAIAS_CLOAK:

        /* check for exclusion of other armor spells */
        if (check_armor_spells(ch, victim, spellnum))
            return CAST_RESULT_CHARGE;

        eff[0].location = APPLY_AC;
        eff[0].modifier = 15 + (skill / 16); /* max 21 */
        eff[0].duration = 5 + (skill / 14);  /* max 12 */

        refresh = FALSE;
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

    case SPELL_INSANITY:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;
        if (mag_savingthrow(victim, SAVING_SPELL)) {
            act("$N has too strong a will to drive insane!", FALSE, ch, 0, victim, TO_CHAR);
            act("Your strength of will protects you from an insane suggestion from "
                "$n... This time...",
                FALSE, ch, 0, victim, TO_VICT);
            act("$N resists going insane at $n's suggestion.", TRUE, ch, 0, victim, TO_NOTVICT);
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

    case SPELL_MESMERIZE:
        if (!AWAKE(victim)) {
            act("$n makes colorful illusions before $N's closed eyes.", FALSE, ch, 0, victim, TO_ROOM);
            act("$N is in no condition to notice your illusion.", FALSE, ch, 0, victim, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }

        if (EFF_FLAGGED(victim, EFF_MESMERIZED)) {
            act("$n tries to get $N's attention, but it's no use.", TRUE, ch, 0, victim, TO_ROOM);
            act("$n seems to be trying to show you something, but you're busy.", FALSE, ch, 0, victim, TO_VICT);
            act("You weave a fascinating illusion, but $N is not paying attention.", FALSE, ch, 0, victim, TO_CHAR);
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

    case SPELL_NEGATE_COLD:

        SET_FLAG(eff[0].flags, EFF_NEGATE_COLD);
        eff[0].duration = 2 + (skill / 20); /* max 7 */
        refresh = FALSE;
        to_vict = "&4&bYour body becomes impervious to the cold!&0";
        to_room = "&4$n&4's is protected by a &3&bwarm&0&4-looking magical field.&0";
        break;

    case SPELL_NEGATE_HEAT:

        SET_FLAG(eff[0].flags, EFF_NEGATE_HEAT);
        eff[0].duration = 2 + (skill / 20); /* max 7 */
        refresh = FALSE;
        to_vict = "&6Your body becomes impervious to all forms of heat!&0";
        to_room = "&6$n&6 is surrounded by a frigid crystalline field.&0";
        break;

    case SPELL_NIGHT_VISION:

        if (affected_by_spell(victim, SPELL_INFRAVISION)) {
            if (ch == victim)
                act("You are already enchanted with enhanced vision.", FALSE, ch, 0, 0, TO_CHAR);
            else
                act("$N is already enchanted with enhanced vision.", FALSE, ch, 0, victim, TO_CHAR);
            act("$n looks a little overprotective.", TRUE, ch, 0, victim, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_ULTRAVISION);
        eff[0].duration = (skill / 21); /* max 4 */
        to_room = "$N's eyes glow a dim neon green.";
        to_vict = "&9&bYour vision sharpens a bit.";
        break;

    case SPELL_SMOKE:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;

        if (MOB_FLAGGED(victim, MOB_NOBLIND)) {
            act("&9&b$n&9&b resists your&9&b column of smoke!&0", FALSE, ch, 0, 0, TO_CHAR);
            act("&9&bYou&9&b resist $n's&9&b column of smoke!&0", FALSE, ch, 0, victim, TO_VICT);
            act("&9&b$N&9&b resists $n's&9&b column of smoke!&0", TRUE, ch, 0, victim, TO_NOTVICT);
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

    case SPELL_VAPORFORM:

        if (GET_COMPOSITION(victim) != COMP_FLESH) {
            send_to_char("Your body cannot sustain this change.\r\n", victim);
            return CAST_RESULT_CHARGE;
        }

        eff[0].location = APPLY_COMPOSITION;
        eff[0].modifier = COMP_MIST;
        eff[0].duration = 2 + (skill / 25); /* max 6 */
        refresh = FALSE;
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
        refresh = FALSE;
        to_vict = "&4&bYour body liquifies.&0";
        to_room =
            "&4&b$N&4&b's body wavers a bit, slowly changing into a "
            "&0&4liquid&b state!&0";
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

        /* --- SORTED --- */

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

    case SPELL_MAGIC_TORCH:

        SET_FLAG(eff[0].flags, EFF_LIGHT);
        eff[0].duration = 5 + (skill / 2); /* max 55 */
        refresh = FALSE;
        to_vict = "&1A magical flame bursts into focus, lighting the area.&0";
        to_room = "&1A magical flame bursts into focus, lighting the area.&0";
        break;

    case SPELL_CIRCLE_OF_LIGHT:

        SET_FLAG(eff[0].flags, EFF_LIGHT);
        eff[0].duration = 5 + (skill / 2); /* max 55 */
        refresh = FALSE;
        to_vict = "&7&bA bright white circle of light begins hovering about your head.&0";
        to_room = "&7&bA bright white circle of light appears over $N's&7&b head.";
        break;

    case SPELL_HARNESS:

        if (EFF_FLAGGED(victim, EFF_HARNESS)) {
            send_to_char("You have already harnessed your energy!\r\n", victim);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_HARNESS);
        eff[0].duration = (skill >= 20); /* max 1 */
        refresh = FALSE;
        to_vict = "&4&bYour veins begin to pulse with energy!&0";
        to_room = "&4&b$N&4&b's veins bulge as a surge of energy rushes into $M!&0";
        break;

    case SPELL_MINOR_GLOBE:

        if (EFF_FLAGGED(victim, EFF_MAJOR_GLOBE)) {
            act("$N's globe of invulnerability resists your spell!", FALSE, ch, 0, victim, TO_CHAR);
            act("Your globe of invulnerability resists $n's spell.", FALSE, ch, 0, victim, TO_VICT);
            act("$n tries to add an additional globe of protection to $N.", TRUE, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_MINOR_GLOBE);
        eff[0].duration = skill / 20; /* max 5 */
        refresh = FALSE;
        to_char = "&1Your shimmering globe wraps around $N&0&1's body.&0";
        to_vict = "&1A shimmering globe wraps around your body.&0";
        to_room = "&1A shimmering globe wraps around $N&0&1's body.&0";
        break;

    case SPELL_MAJOR_GLOBE:

        if (EFF_FLAGGED(victim, EFF_MINOR_GLOBE)) {
            act("$N's minor globe of invulnerability resists your spell!", FALSE, ch, 0, victim, TO_CHAR);
            act("Your minor globe of invulnerability resists $n's spell.", FALSE, ch, 0, victim, TO_VICT);
            act("$n tries to add an additional globe of protection to $N.", TRUE, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_MAJOR_GLOBE);
        eff[0].duration = 4 + (skill / 20); /* max 9 */
        refresh = FALSE;
        to_char = "&1&bYour shimmering globe of force wraps around $N&1&b's body.&0";
        to_vict = "&1&bA shimmering globe of force wraps around your body.&0";
        to_room = "&1&bA shimmering globe of force wraps around $N&1&b's body.&0";
        break;

    case SPELL_COLDSHIELD:

        if (EFF_FLAGGED(ch, EFF_FIRESHIELD)) {
            cprintf(ch, "The shield of fire around %s body negates your spell.\r\n",
                    ch == victim ? "your" : HSHR(victim));
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_COLDSHIELD);
        eff[0].duration = skill / 20; /* max 5 */
        refresh = FALSE;
        to_vict = "&4A jagged formation of i&bc&7e sh&4ard&0&4s forms around you.&0";
        to_room = "&4A jagged formation of i&bc&7e sh&4ard&0&4s forms around $N&0&4.&0";
        break;

    case SPELL_FIRESHIELD:

        if (EFF_FLAGGED(ch, EFF_COLDSHIELD)) {
            cprintf(ch, "The shield of ice around %s body negates your spell.\r\n",
                    ch == victim ? "your" : HSHR(victim));
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_FIRESHIELD);
        eff[0].duration = skill / 20; /* max 5 */
        refresh = FALSE;
        to_vict = "&1A burning shield of f&bi&3r&7e&0&1 explodes from your body!&0";
        to_room = "&1A burning shield of f&bi&3r&7e&0&1 explodes from $N&0&1's body!&0";
        break;

    case SPELL_MINOR_PARALYSIS:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;
        /* Make success based on skill and saving throw -myc 17 Feb 2007 */
        if (mag_savingthrow(victim, SAVING_PARA) || skill - GET_LEVEL(victim) < number(0, 70)) {
            act("&7&b$N resists your weak paralysis.&0", FALSE, ch, 0, victim, TO_CHAR);
            act("&7&b$n tries to paralize you but fails!&0", FALSE, ch, 0, victim, TO_VICT);
            act("&7&b$n squints at $N but nothing happens.&0", TRUE, ch, 0, victim, TO_NOTVICT);

            /* start combat for failure */
            if (!FIGHTING(victim)) {
                attack(victim, ch);
                remember(victim, ch);
            }

            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        SET_FLAG(eff[0].flags, EFF_MINOR_PARALYSIS);
        eff[0].duration = 2 + (skill / 15); /* max 8 */
        refresh = FALSE;
        to_char = "You paralyze $N! WooHoo!";
        to_vict = "&7&bAll motion in your body grinds to a halt.&0";
        to_room = "&7&bAll motion in $N&7&b's body grinds to a halt.&0";
        break;

    case SPELL_RAY_OF_ENFEEB:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;
        if (EFF_FLAGGED(victim, EFF_RAY_OF_ENFEEB)) {
            act("$N is already feeble enough dammit.", FALSE, ch, 0, victim, TO_CHAR);
            act("$n seems to be looking at you funny.", FALSE, ch, 0, victim, TO_VICT);
            act("&7&b$n squints at $N but nothing happens.&0", TRUE, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }
        if (mag_savingthrow(victim, savetype)) {
            act("$N resists your feeble attempt!", FALSE, ch, 0, victim, TO_CHAR);
            act("$n tries to drain your strength, but you resist!", FALSE, ch, 0, victim, TO_VICT);
            act("&7&b$n squints at $N but nothing happens.&0", TRUE, ch, 0, victim, TO_NOTVICT);
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

    case SPELL_LEVITATE:

        SET_FLAG(eff[0].flags, EFF_LEVITATE);
        eff[0].duration = 5 + (skill / 10);
        to_char = "&6$N&0&6 floats up into the air.&0";
        to_vict = "&6You float up into the air.&0";
        to_room = "&6$N&0&6 floats up into the air.&0";
        break;

    case SPELL_CHILL_TOUCH:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;
        if (mag_savingthrow(victim, savetype)) {
            act("$N resists your withering effect!", FALSE, ch, 0, victim, TO_CHAR);
            act("You resist $n's withering effects!", FALSE, ch, 0, victim, TO_VICT);
            act("$N resists $n's withering effect!", TRUE, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        eff[0].location = APPLY_STR;
        eff[0].duration = 3 + (skill / 20);  /* max 8 */
        eff[0].modifier = -5 - (skill / 10); /* max -15 */

        to_vict = "You feel your strength wither!";
        to_char = "$N is withered by your cold!";
        to_room = "$N withers slightly from $n's cold!";
        break;

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

    case SPELL_SILENCE:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;

        if (MOB_FLAGGED(victim, MOB_NOSILENCE)) {
            send_to_char("You seem unable to silence this one.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }

        if (mag_savingthrow(victim, savetype)) {
            act("$N resists your pitiful attempt to silence $M.", FALSE, ch, 0, victim, TO_CHAR);
            act("&7&b$n tries to silence you but fails!&0", FALSE, ch, 0, victim, TO_VICT);
            act("&7&b$n squints at $N but nothing happens.&0", TRUE, ch, 0, victim, TO_NOTVICT);
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

    case SPELL_SOULSHIELD:

        SET_FLAG(eff[0].flags, EFF_SOULSHIELD);
        eff[0].duration = 2 + (skill / 10); /* max 12 */
        refresh = FALSE;

        if (IS_GOOD(victim)) {
            to_vict = "&3&bA bright golden aura surrounds your body!&0";
            to_room = "&3&bA bright golden aura surrounds $N's body!&0";
        } else if (IS_EVIL(victim)) {
            to_vict = "&1&bA &0&1dark red&b aura engulfs you!&0";
            to_room = "&1&bA &0&1dark red&b aura engulfs $N's body!&0";
        } else {
            act("A brief aura surrounds you, then fades.", FALSE, ch, 0, victim, TO_VICT);
            act("A brief aura surrounds $n, then fades.", TRUE, victim, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }
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

        refresh = TRUE;
        to_vict =
            "&1&bYour skin starts to itch as you reduce to half your normal "
            "size.&0";
        to_room = "&1&b$N's skin ripples as $E shrinks to half $S normal size!&0";
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

        refresh = TRUE;
        to_vict =
            "&9&bYour skin starts to itch as you enlarge to twice your "
            "normal size!&0";
        to_room = "&9&b$N's skin ripples as $E enlarges to twice $S normal size!&0";
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

        refresh = FALSE;
        to_char = "&9&b$N's skin hardens and turns to stone!&0";
        to_vict = "&9&bYour skin hardens and turns to stone!&0";
        to_room = "&9&b$N's skin hardens and turns to stone!&0";
        break;

    case SPELL_HASTE:

        SET_FLAG(eff[0].flags, EFF_HASTE);
        eff[0].duration = 2 + (skill / 21); /* max 6 */
        to_char = "&1$N starts to move with uncanny speed!&0";
        to_vict = "&1You start to move with uncanny speed!&0";
        to_room = "&1$N starts to move with uncanny speed!&0";
        break;

    case SPELL_BLUR:

        SET_FLAG(eff[0].flags, EFF_BLUR);
        eff[0].duration = 2 + (skill / 21); /* max 6 */
        to_vict = "&7The world seems to slow as you start moving with unnatural speed!&0";
        to_room = "&7$N's image blurs in unnatural speed!&0";
        break;

    case SPELL_BLESS:

        if (GET_LEVEL(ch) < LVL_IMMORT && !IS_GOOD(ch) && casttype == CAST_SPELL) {
            send_to_char("The gods have forsaken you in your evilness!\r\n", ch);
            act("There is no effect.  $U$n adopts a dejected look.", TRUE, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        if (affected_by_spell(victim, SPELL_DARK_PRESENCE) || affected_by_spell(victim, SPELL_DEMONSKIN) ||
            affected_by_spell(victim, SPELL_DEMONIC_ASPECT) || affected_by_spell(victim, SPELL_DEMONIC_MUTATION) ||
            affected_by_spell(victim, SPELL_WINGS_OF_HELL)) {
            act("$N is already blessed by some dark gods.", FALSE, ch, 0, victim, TO_CHAR);
            act("$n looks a little overprotective.", TRUE, ch, 0, 0, TO_ROOM);
            return CAST_RESULT_CHARGE;
        }

        /* Alignment Checks! */
        if (IS_EVIL(victim)) {
            act("You can't bless evil people!", FALSE, ch, 0, 0, TO_CHAR);
            act("$n tries to awaken your inner angel.\r\nSilly isn't $e?", FALSE, ch, 0, victim, TO_VICT);
            act("$n fails to awaken $N's inner angel.", TRUE, ch, 0, victim, TO_NOTVICT);
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

    case SPELL_SUNRAY:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;

        if (MOB_FLAGGED(victim, MOB_NOBLIND)) {
            send_to_char("You seem unable to blind this creature.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        if (mag_savingthrow(victim, savetype)) {
            act("$N resists your pitiful attempt to blind $M.", FALSE, ch, 0, victim, TO_CHAR);
            act("&7&b$n tries to blind you but fails!&0", FALSE, ch, 0, victim, TO_VICT);
            act("&7&b$n directs the rays of the sun at $N but nothing happens.&0", TRUE, ch, 0, victim, TO_NOTVICT);
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

    case SPELL_BLINDNESS:

        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;

        if (MOB_FLAGGED(victim, MOB_NOBLIND)) {
            send_to_char("You seem unable to blind this creature.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }

        if (mag_savingthrow(victim, savetype)) {
            act("$N resists your pitiful attempt to blind $M.", FALSE, ch, 0, victim, TO_CHAR);
            act("&7&b$n tries to blind you but fails!&0", FALSE, ch, 0, victim, TO_VICT);
            act("&7&b$n tries to blind $N but nothing happens.&0", TRUE, ch, 0, victim, TO_NOTVICT);
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

    case SPELL_CURSE:

        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;

        if (mag_savingthrow(victim, savetype)) {
            send_to_char(NOEFFECT, ch);
            act("&7&b$n tries to curse you but fails!&0", FALSE, ch, 0, victim, TO_VICT);
            act("&7&b$n squints at $N but nothing happens.&0", TRUE, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        eff[0].location = APPLY_HITROLL;
        eff[0].duration = 5 + (skill / 14);  /* max 12 */
        eff[0].modifier = -1 - (skill / 50); /* max -3 */
        SET_FLAG(eff[0].flags, EFF_CURSE);
        eff[1].location = APPLY_DAMROLL;
        eff[1].duration = eff[0].duration;
        eff[1].modifier = eff[0].modifier;
        accum_effect = TRUE;
        to_char = "You curse $N! Muahahah!";
        to_room = "$N briefly glows red!";
        to_vict = "You feel very uncomfortable.";
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

    case SPELL_INFRAVISION:

        if (affected_by_spell(victim, SPELL_NIGHT_VISION)) {
            if (victim == ch) {
                send_to_char("You are already enchanted with enhanced vision.\r\n", ch);
                act("$n looks a little overprotective.", TRUE, ch, 0, 0, TO_ROOM);
            } else {
                act("$N seems to be able to sort of see enough already.", FALSE, ch, 0, victim, TO_CHAR);
                act("You are already enchanted with enhanced vision.", FALSE, ch, 0, victim, TO_VICT);
                act("$n looks a little overprotective.", TRUE, ch, 0, victim, TO_NOTVICT);
            }
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_INFRAVISION);
        eff[0].duration = 5 + (skill / 10); /* max 15 */
        to_char = "$N's eyes glow red.";
        to_vict = "Your eyes glow red.";
        to_room = "$N's eyes glow red.";
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

    case SPELL_POISON:
    case SPELL_GAS_BREATH:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;
        if (GET_LEVEL(victim) >= LVL_IMMORT || damage_evasion(victim, 0, 0, DAM_POISON)) {
            act("$n is unaffected!", FALSE, victim, 0, 0, TO_ROOM);
            act("You are unaffected!", FALSE, victim, 0, 0, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }
        if (mag_savingthrow(victim, SAVING_PARA)) {
            if (spellnum != SPELL_GAS_BREATH) {
                /* No message is sent for dodging this part of a gas attack
                 * because relevant messages have already been sent in the
                 * mag_damage portion of the attack. */
                act("You dodge $n's attempt to prick you!", FALSE, ch, 0, victim, TO_VICT);
                act("$N dodges $n's attempt to prick $M.", TRUE, ch, 0, victim, TO_NOTVICT);
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

    case SPELL_PROT_FROM_EVIL:

        /* Alignment Check! */
        if (IS_EVIL(victim)) {
            act("You can't protect an ally if they are EVIL!", FALSE, ch, 0, victim, TO_CHAR);
            act("$n tries to protect you from evil!\r\nSilly isn't $e.", FALSE, ch, 0, victim, TO_VICT);
            act("$n fails to protect $N from evil.  DUH!", TRUE, ch, 0, victim, TO_NOTVICT);
            return CAST_RESULT_CHARGE;
        }

        SET_FLAG(eff[0].flags, EFF_PROTECT_EVIL);
        eff[0].duration = 9 + (skill / 9); /* max 20 */
        to_vict = "You feel invulnerable!";
        to_char = "You surround $N with glyphs of holy warding.";
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
            act("$n can sing all $e wants, you aren't going to sleep.", FALSE, ch, 0, victim, TO_VICT);
            act("$n tries to sing $N to sleep, but to no avail, uh oh.", TRUE, ch, 0, victim, TO_NOTVICT);
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
        refresh = FALSE;

        if (GET_STANCE(victim) > STANCE_SLEEPING) {
            act("You feel very sleepy...  Zzzz......", FALSE, victim, 0, 0, TO_CHAR);
            act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
            GET_STANCE(victim) = STANCE_SLEEPING;
            GET_POS(victim) = POS_PRONE;
        }
        break;

    case SPELL_STRENGTH:

        /* This is rather a hack of the intended nature of how the
           spell should work, but this is better than the way it was
           RSD 3/27/00 */

        eff[0].location = APPLY_STR;
        eff[0].modifier = 2 + (skill / 8);  /* max 14 */
        eff[0].duration = 5 + (skill / 14); /* max 12 */
        to_room = "$N looks stronger!";
        to_vict = "You feel stronger!";
        break;

    case SPELL_INN_STRENGTH:
        eff[0].location = APPLY_STR;
        eff[0].duration = (skill >> 1) + 4;
        eff[0].modifier = 1 + (skill / 18); /* max 6 */
        accum_effect = FALSE;
        to_vict = "You feel stronger!";
        /* Innate strength usage shouldn't call for a skill improvement in a spell
         * sphere */
        break;

    case SPELL_SENSE_LIFE:

        SET_FLAG(eff[0].flags, EFF_SENSE_LIFE);
        eff[0].duration = 17 + (skill / 3); /* max 50 */
        to_vict = "Your feel your awareness improve.";
        to_room = "$N seems more aware of $S surroundings.";
        break;

    case SPELL_WATERWALK:

        SET_FLAG(eff[0].flags, EFF_WATERWALK);
        eff[0].duration = 35 + (skill / 4); /* max 60 */
        to_room = "$N sprouts webbing between $S toes!";
        to_vict = "You feel webbing between your toes.";
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

    case SPELL_FARSEE:

        SET_FLAG(eff[0].flags, EFF_FARSEE);
        eff[0].duration = 5 + (skill / 10); /* max 15 */
        to_vict = "Your sight improves dramatically.";
        to_room = "$N's pupils dilate rapidly for a second.";
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

    case SPELL_REBUKE_UNDEAD:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;

        if (GET_LIFEFORCE(victim) != LIFE_UNDEAD) {
            act("$n seems confused as to your state of mortality.", FALSE, ch, 0, victim, TO_VICT);
            act("$n tries to rebuke $N's buried undead nature.  Must be buried too deep.", TRUE, ch, 0, victim,
                TO_ROOM);
            send_to_char("Your rebuke elicits nothing but a raised eyebrow.\r\n", ch);
            return CAST_RESULT_CHARGE;
        }
        if (mag_savingthrow(victim, SAVING_SPELL)) {
            act("You stare blankly at $n as $e attempts to rebuke you.", FALSE, ch, 0, victim, TO_VICT);
            act("$N looks at $n blankly as $e calls down a spell of condemnation.", TRUE, ch, 0, victim, TO_NOTVICT);
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

    case SPELL_NATURES_GUIDANCE:
        eff[0].location = APPLY_HITROLL;
        eff[0].modifier = eff[0].duration = (skill / 20) + 1; /* range (1, 6) */
        /* Self only */
        to_vict = "You feel a higher power guiding your hands.";
        to_room = "$N calls on guidance from a higher power.";
        break;

    case SPELL_NATURES_EMBRACE:
        SET_FLAG(eff[0].flags, EFF_CAMOUFLAGED);
        eff[0].duration = (skill / 3) + 1; /* range (1, 34) */
        to_vict = "&9&8You phase into the landscape.&0";
        to_room = "&9&8$n&9&8 phases into the landscape.&0";
        break;

    case SPELL_MISDIRECTION:
        SET_FLAG(eff[0].flags, EFF_MISDIRECTION);
        eff[0].duration = 2 + skill / 4;
        to_vict =
            "You feel like a stack of little illusions all pointing in "
            "different directions.";
        break;

    case CHANT_REGENERATION:
        eff[0].location = APPLY_HIT_REGEN;
        eff[0].duration = skill / 2 + 3;
        eff[0].modifier = skill / 2 + 10;
        to_vict = "You feel your health improve.";
        to_room = "$n looks a little healthier.";
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
           $E ignores it.&0", TRUE, ch, 0, victim, TO_NOTVICT); return
           CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
           }
         */

        eff[0].location = APPLY_HITROLL;
        eff[0].duration = 5 + (skill / 14);  /* max 12 */
        eff[0].modifier = -1 - (skill / 10); /* max -11 */
        accum_effect = TRUE;
        to_char = "You depress $N with a song of darkness and sorrow!";
        to_room = "A dark look clouds $N's visage as $n sings $M a song of sorrow.";
        to_vict = "$n's song of sorrow fills your mind with darkness and shadows.";
        break;

    case CHANT_ARIA_OF_DISSONANCE:

        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;

        eff[0].location = APPLY_AC;
        eff[0].duration = 5 + (skill / 30);  /* max 8 */
        eff[0].modifier = -10 - (skill / 2); /* max -60 */
        to_char = "Your song of dissonance confuses $N!";
        to_room = "$N winces as $n's dissonant song fills $S ears.";
        to_vict = "$n fills your ears with an aria of dissonance, causing confusion!";
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

    case CHANT_SEED_OF_DESTRUCTION:
        if (!attack_ok(ch, victim, TRUE))
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

    case CHANT_SPIRIT_WOLF:
        SET_FLAG(eff[0].flags, EFF_SPIRIT_WOLF);
        eff[0].duration = (skill / 20); /* max 5 */
        to_vict = "You feel a wolf-like fury come over you.";
        to_room = "$n seems to take on a fearsome, wolf-like demeanor.";
        break;

    case CHANT_SPIRIT_BEAR:
        SET_FLAG(eff[0].flags, EFF_SPIRIT_BEAR);
        eff[0].duration = (skill / 20); /* max 5 */
        to_vict = "The spirit of the bear consumes your body.";
        to_room = "$n shifts $s weight, seeming heavier and more dangerous.";
        break;

    case CHANT_INTERMINABLE_WRATH:
        SET_FLAG(eff[0].flags, EFF_WRATH);
        eff[0].duration = (skill / 20); /* max 5 */
        to_vict = "A feeling of unforgiving wrath fills you.";
        to_room = "$n bristles with anger.";
        break;

    case SPELL_SPINECHILLER:
        if (!attack_ok(ch, victim, TRUE))
            return CAST_RESULT_CHARGE;
        if (mag_savingthrow(victim, SAVING_PARA) || skill - GET_LEVEL(victim) < number(0, 70)) {
            act("$N resists your neuroparalysis.", FALSE, ch, 0, victim, TO_CHAR);
            act("$n tries to scramble your nerves, but fails!", FALSE, ch, 0, victim, TO_VICT);
            act("$n grabs onto $N and squeezes.", TRUE, ch, 0, victim, TO_ROOM);

            /* start combat for failure */
            if (!FIGHTING(victim)) {
                attack(victim, ch);
                remember(victim, ch);
            }

            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        SET_FLAG(eff[0].flags, EFF_MINOR_PARALYSIS);
        eff[0].duration = 2 + (skill / 15); /* max 8 */
        refresh = FALSE;
        to_char = "You grab $N and scramble $S nerves!";
        to_vict = "Tingles run up and down your spine as $n scrambles your nerves!";
        to_room = "$N gasps for breath as $n scrambles $S nerves!";
        break;

    case SPELL_BONE_DRAW:
        if (EFF_FLAGGED(victim, EFF_IMMOBILIZED)) {
            act("$n is already immobilized!", FALSE, ch, 0, victim, TO_CHAR);
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
            is_innate = TRUE;
    }

    if (affected_by_spell(victim, spellnum) && is_innate) {
        send_to_char(NOEFFECT, ch);
        return CAST_RESULT_CHARGE;
    }

    /* act() for the message buffers in mag_affect() */

    if (!affected_by_spell(victim, spellnum)) {
        /* Suppress this message when you are casting the spell on yourself. */
        if (to_char != NULL && ch != victim)
            act(to_char, FALSE, ch, 0, victim, TO_CHAR);
        if (to_vict != NULL)
            act(to_vict, FALSE, ch, 0, victim, TO_VICT);
        if (to_room != NULL) {
            act(to_room, TRUE, ch, 0, victim, TO_NOTVICT);
            if (to_char == NULL && ch != victim)
                act(to_room, FALSE, ch, 0, victim, TO_CHAR);
        }
    }

    for (i = 0; i < MAX_SPELL_EFFECTS; i++)
        if (HAS_FLAGS(eff[i].flags, NUM_EFF_FLAGS) || eff[i].location != APPLY_NONE) {
            effect_join(victim, eff + i, accum_duration, FALSE, accum_effect, FALSE, refresh);
            if (CASTING(victim)) {
                if (IS_FLAGGED(eff[i].flags, EFF_SILENCE)) {
                    STOP_CASTING(victim);
                    act("Your spell collapses.", FALSE, victim, 0, 0, TO_CHAR);
                    act("$n continues silently moving $s lips for a moment before giving "
                        "up.",
                        FALSE, victim, 0, 0, TO_ROOM);
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

void perform_mag_group(int skill, struct char_data *ch, struct char_data *tch, int spellnum, int savetype) {
    switch (spellnum) {
    case SPELL_GROUP_HEAL:
        mag_point(skill, ch, tch, SPELL_HEAL, savetype);
        mag_unaffect(skill, ch, tch, SPELL_HEAL, savetype);
        break;
    case SPELL_GROUP_ARMOR:
        mag_affect(skill, ch, tch, SPELL_ARMOR, savetype, CAST_SPELL);
        break;
    case SPELL_GROUP_RECALL:
        spell_recall(spellnum, skill, ch, tch, NULL, savetype);
        break;
    case SPELL_DIVINE_ESSENCE:
        mag_affect(skill, ch, tch, SPELL_GREATER_ENDURANCE, savetype, CAST_SPELL);
        mag_affect(skill, ch, tch, SPELL_BLESS, savetype, CAST_SPELL);
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

int mag_group(int skill, struct char_data *ch, int spellnum, int savetype) {
    struct char_data *tch, *next_tch;
    char *to_room, *to_char;

    if (ch == NULL)
        return 0;

    if (!IS_GROUPED(ch))
        return CAST_RESULT_CHARGE;

    switch (spellnum) {
    case SPELL_DIVINE_ESSENCE:
        to_room = "&3&b$n&3&b invokes $s deity's divine essence to fill the area!&0";
        to_char = "&3&bYou invoke your deity's divine essence!&0\r\n";
        break;
    default:
        to_room = NULL;
        to_char = NULL;
    }
    if (to_room)
        act(to_room, TRUE, ch, 0, 0, TO_ROOM);
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

int mag_mass(int skill, struct char_data *ch, int spellnum, int savetype) {
    struct char_data *tch, *tch_next;
    bool found = FALSE;

    if (ch == NULL)
        return 0;

    if (ch->in_room == NOWHERE)
        return CAST_RESULT_CHARGE;

    for (tch = world[ch->in_room].people; tch; tch = tch_next) {
        tch_next = tch->next_in_room;

        if (SINFO.violent) {
            if (tch == ch)
                continue;
            if (!mass_attack_ok(ch, tch, FALSE))
                continue;
            if (is_grouped(ch, tch))
                continue;
        }

        found = TRUE;
        mag_affect(skill, ch, tch, spellnum, savetype, CAST_SPELL);

        /*
         * If this is a violent spell, and the victim isn't already fighting
         * someone, then make it attack the caster.
         */
        if (SINFO.violent && !FIGHTING(tch))
            set_fighting(tch, ch, FALSE);
    }

    /* No skill improvement if there weren't any valid targets in the room. */
    if (!found)
        return CAST_RESULT_CHARGE;

    return CAST_RESULT_IMPROVE | CAST_RESULT_CHARGE;
}

int mag_bulk_objs(int skill, struct char_data *ch, int spellnum, int savetype) {
    struct obj_data *tobj, *tobj_next;
    bool found = FALSE;

    if (ch == NULL)
        return 0;

    if (ch->in_room == NOWHERE)
        return CAST_RESULT_CHARGE;

    for (tobj = world[ch->in_room].contents; tobj; tobj = tobj_next) {
        tobj_next = tobj->next_content;

        found = TRUE;
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

int mag_area(int skill, struct char_data *ch, int spellnum, int savetype) {
    struct char_data *tch, *next_tch;
    char *to_char = NULL;
    char *to_room = NULL;
    bool found = FALSE;

    if (ch == NULL)
        return 0;

    if (ch->in_room == NOWHERE)
        return CAST_RESULT_CHARGE;

    /*
     * to add spells to this fn, just add the message here plus an entry
     * in mag_damage for the damaging part of the spell.
     */
    switch (spellnum) {
    case SPELL_CHAIN_LIGHTNING:
        to_char = "&4&bYou send powerful bolts of lightning from your body...&0";
        to_room = "&4&b$n&4&b sends powerful bolts of lightning into $s foes...&0";
        break;
    case SPELL_CREMATE:
        to_char = "&1&8You raise up a huge conflaguration in the area.&0";
        to_room = "&1&8$n summons a huge conflagration burning through the area.&0";
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
            act("Quake the earth? What earth? There's no ground here anywhere.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n hunches over and grunts loudly!", TRUE, ch, 0, 0, TO_ROOM);
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

    if (to_char != NULL)
        act(to_char, FALSE, ch, 0, 0, TO_CHAR);
    if (to_room != NULL)
        act(to_room, FALSE, ch, 0, 0, TO_ROOM);

    if ((spellnum == SPELL_UNHOLY_WORD && IS_GOOD(ch)) || (spellnum == SPELL_HOLY_WORD && IS_EVIL(ch))) {
        act("&9&bYour word of power is the last thing you hear as your soul is "
            "ripped apart!&0",
            FALSE, ch, 0, 0, TO_CHAR);
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

        found = TRUE;
        mag_damage(skill, ch, tch, spellnum, savetype);
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
void mod_for_undead_type(struct char_data *mob, enum undead_type type) {
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
void mod_for_lvldiff(struct char_data *mob, struct char_data *ch) {
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
struct char_data *create_undead(struct char_data *orig, struct char_data *caster, bool ISPC) {
    char short_buf[160], long_buf[160], alias_buf[160];
    struct char_data *new_mob, *next_mob;
    enum undead_type new_mob_type;

    extern struct player_special_data dummy_mob;
    extern void roll_natural_abils(struct char_data *);
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

int ch_can_control_mob(struct char_data *ch, struct char_data *mob) {
    int max_single_control, max_total_control, current_control = 0;
    struct follow_type *foll;
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

struct char_data *load_summoned_mob(int vnum, int destroom) {
    struct char_data *mob;
    int r_num;
    if ((r_num = real_mobile(vnum)) < 0) {
        sprintf(buf, "SYSERR: tried to summon mob with nonexistent vnum %d", vnum);
        log(buf);
        return NULL;
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

struct char_data *duplicate_char(struct char_data *model, int destroom) {
    struct char_data *new_mob;

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

    /* struct char_player_data */
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
    new_mob->player.class = model->player.class;
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
    /* END struct char_player_data */

    /* struct char_special_data */
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
    /* END struct char_special_data */

    /* struct mob_special_data */
    new_mob->mob_specials = model->mob_specials;
    new_mob->mob_specials.memory = NULL;
    /* END struct mob_special_data */

    REMOVE_FLAG(MOB_FLAGS(new_mob), MOB_SPEC);

    return new_mob;
}

struct char_data *copyplayer(struct char_data *ch, struct char_data *model) {
    struct char_data *new_mob;

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
void phantasm_transform(struct char_data *ch, struct char_data *model, int life_hours) {
    struct effect effect;

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
        ch->player.weight = model->player.weight;
        ch->player.height = model->player.height;
        ch->player.affected_size = model->player.affected_size;
        ch->player.mod_size = 0;
        ch->player.natural_size = model->player.affected_size;
        ch->player.base_size = model->player.base_size;
        ch->player.base_weight = model->player.base_weight;
        ch->player.base_height = model->player.base_height;
    }
}

struct char_data *summon_phantasm(struct char_data *ch, int vnum, int life_hours) {
    struct char_data *new_mob;

    if (!(new_mob = load_summoned_mob(vnum, ch->in_room)))
        return NULL;

    phantasm_transform(new_mob, NULL, life_hours);
    SET_FLAG(MOB_FLAGS(new_mob), MOB_NOSCRIPT); /* Prevent specprocs and triggers */

    return new_mob;
}

/*
 * Every spell which summons/gates/conjours a mob comes through here.
 *
 * Return value: CAST_RESULT_ flags.
 */

int mag_summon(int skill, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int spellnum,
               int savetype) {
    int orig_mob_rnum;
    struct char_data *new_mob;
    int success, duration;
    struct effect eff;
    struct obj_data *temp_obj, *next_obj;
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
            new_mob = create_undead(mob_proto + orig_mob_rnum, ch, FALSE);
        else
            new_mob = create_undead(mob_proto, ch, TRUE);

        char_to_room(new_mob, ch->in_room);

        /* can we control it? */
        success = ch_can_control_mob(ch, new_mob);
        if (success == CONTROL_HELLNO) {
            act("You begin to raise $N beyond your power, but you stop the spell "
                "in time.",
                FALSE, ch, 0, new_mob, TO_CHAR);
            act("$n begins to raise $N beyond $s power, but $e stops the spell "
                "in time.",
                FALSE, ch, 0, new_mob, TO_ROOM);
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
            act("You raise $N.", FALSE, ch, 0, new_mob, TO_CHAR);
            act("$n raises $N.", FALSE, ch, 0, new_mob, TO_ROOM);
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
            act("You raise $N, and $E doesn't seem too happy about it.", FALSE, ch, 0, new_mob, TO_CHAR);
            act("$n raises $N, and $E doesn't seem too happy about it.", FALSE, ch, 0, new_mob, TO_ROOM);
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
        if (new_mob == NULL) {
            act("The spell fizzles.", FALSE, 0, 0, 0, TO_ROOM);
            send_to_char("The spell fizzles.\r\n", ch);
            return 0;
        }

        /* Feedback */
        act("From scattered motes of light, $n coalesces.", TRUE, new_mob, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;

    case SPELL_SIMULACRUM:
        /* Simulacrum is like the phantasm spell, except it duplicates an
         * existing creature. */
        if (!vict) {
            send_to_char("Who did you want to duplicate?\r\n", ch);
            return 0;
        }
        if (GET_LEVEL(vict) > skill) {
            act("$N is far too powerful!", FALSE, ch, 0, vict, TO_CHAR);
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
                if (new_mob == NULL) {
                    act("The spell fizzles.", FALSE, 0, 0, 0, TO_ROOM);
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
                act("The spell fizzles.", FALSE, 0, 0, 0, TO_ROOM);
                send_to_char("The spell fizzles.\r\n", ch);
                return 0;
            }
            phantasm_transform(new_mob, vict, duration);
        }
        SET_FLAG(MOB_FLAGS(new_mob), MOB_NOSCRIPT); /* Prevent specprocs and triggers */
        /* Feedback */
        act("From scattered motes of light, $n coalesces.", TRUE, new_mob, 0, 0, TO_ROOM);
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

int mag_point(int skill, struct char_data *ch, struct char_data *victim, int spellnum, int savetype) {
    int hit = 0;
    int move = 0;
    int hunger = 0;
    int thirst = 0;
    int hide = 0;
    int multiplier = 0;
    int sus;

    if (victim == NULL)
        return 0;

    sus = susceptibility(victim, skills[spellnum].damage_type);

    multiplier = 2 + (skill / 24); /* max 6 */

    switch (spellnum) {
    case SPELL_CURE_LIGHT:
        hit = dice(2, 8) + 1;
        send_to_char("You feel better.\r\n", victim);
        break;
    case SPELL_CURE_CRITIC:
        hit = dice(6, 8) + 3;
        send_to_char("You feel a lot better!\r\n", victim);
        break;
    case SPELL_CURE_SERIOUS:
        hit = dice(4, 8) + 2;
        send_to_char("You feel much better!\r\n", victim);
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
                TRUE, victim, 0, 0, TO_ROOM);
            act("&2&bYou sprout roots that dig deep beneath the soil, drawing "
                "sustenence.&0",
                FALSE, victim, 0, 0, TO_CHAR);
            break;
        default:
            act("&2$n&2 sends forth roots in a vain attempt to gain sustenance.&0", TRUE, victim, 0, 0, TO_ROOM);
            switch (SECT(victim->in_room)) {
            case SECT_SHALLOWS:
            case SECT_WATER:
            case SECT_UNDERWATER:
                thirst = 24;
                act("&2You send out roots which quickly become waterlogged.  They draw "
                    "moisture but no nutrients.&0",
                    FALSE, victim, 0, 0, TO_CHAR);
                break;
            case SECT_AIR:
                act("&2Your roots flail briefly in the air, unable to grow without "
                    "soil.&0",
                    FALSE, victim, 0, 0, TO_CHAR);
                break;
            default:
                act("&2You send out roots, but they are unable to penetrate to the "
                    "life-giving soil.&0",
                    FALSE, victim, 0, 0, TO_CHAR);
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
        act("&9&b$N&9&b slowly fades out of existence.&0", TRUE, ch, 0, victim, TO_ROOM);
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
        hurt_char(victim, ch, -hit * multiplier, TRUE);
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

int mag_unaffect(int skill, struct char_data *ch, struct char_data *victim, int spellnum, int type) {
    int spell = 0;
    char *to_vict = NULL, *to_room = NULL;

    if (victim == NULL)
        return 0;

    switch (spellnum) {
    case SPELL_EXTINGUISH:
        REMOVE_FLAG(EFF_FLAGS(victim), EFF_ON_FIRE);
        send_to_char("You are doused with a magical liquid.\r\n", victim);
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    case SPELL_SANE_MIND:
        spell = SPELL_INSANITY;
        to_vict = "Your mind comes back to reality.";
        break;
    case SPELL_ENLARGE:
        if (!EFF_FLAGGED(victim, EFF_REDUCE))
            return CAST_RESULT_CHARGE;
        spell = SPELL_REDUCE;
        to_vict = "&8You return to your normal size.&0";
        break;
    case SPELL_REDUCE:
        if (!EFF_FLAGGED(victim, EFF_ENLARGE))
            return CAST_RESULT_CHARGE;
        spell = SPELL_ENLARGE;
        to_vict = "&8You return to your normal size.&0";
        break;
    case SPELL_CURE_BLIND:
    case SPELL_HEAL:
    case SPELL_FULL_HEAL:
        if (EFF_FLAGGED(victim, EFF_BLIND)) {
            to_vict = "Your vision returns!";
            to_room = "There's a momentary gleam in $n's eyes.";
            if (affected_by_spell(victim, SPELL_BLINDNESS))
                spell = SPELL_BLINDNESS; /* Remove blindness below */
            if (affected_by_spell(victim, SKILL_EYE_GOUGE)) {
                if (spell) /* If already removing a spell, remove eye gouge now */
                    effect_from_char(victim, spell);
                spell = SKILL_EYE_GOUGE;
            }
            if (affected_by_spell(victim, SPELL_SUNRAY)) {
                if (spell) /* If already removing a spell, remove sunray now */
                    effect_from_char(victim, spell);
                spell = SPELL_SUNRAY;
            }
        }
        if ((spellnum == SPELL_HEAL || spellnum == SPELL_FULL_HEAL) && affected_by_spell(victim, SPELL_DISEASE)) {
            effect_from_char(victim, SPELL_DISEASE);
            send_to_char("Your disease has been cured.\r\n", victim);
        }
        break;
    case SPELL_REMOVE_POISON:
        spell = SPELL_POISON;
        to_vict = "A warm feeling runs through your body!";
        to_room = "$n looks better.";
        break;
    case SPELL_REMOVE_CURSE:
        spell = SPELL_CURSE;
        to_vict = "You don't feel so unlucky.";
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
    if (to_vict != NULL)
        act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
    if (to_room != NULL)
        act(to_room, TRUE, victim, 0, ch, TO_ROOM);

    if (spellnum == SPELL_REMOVE_POISON)
        check_regen_rates(victim); /* speed up regen rate immediately */

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/* Return value: CAST_RESULT_ flags.
 */
int mag_alter_obj(int skill, struct char_data *ch, struct obj_data *obj, int spellnum, int savetype) {
    char *to_char = NULL;
    char *to_room = NULL;
    int i;
    int result = CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;

    /* Set post_modify to TRUE if you want to send messages first, then
     * modify the object. You must add a case to the second switch
     * statement to actualy make the modification(s). */
    bool post_modify = FALSE;

    if (obj == NULL)
        return 0;

    switch (spellnum) {
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
    case SPELL_INVISIBLE:
    case SPELL_MASS_INVIS:
        if (!OBJ_FLAGGED(obj, ITEM_NOINVIS) && !OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
            post_modify = TRUE;
            to_char = "$p vanishes.";
        }
        break;
    case SPELL_POISON:
        if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) || (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
             (GET_OBJ_TYPE(obj) == ITEM_FOOD)) &&
            !IS_POISONED(obj)) {
            GET_OBJ_VAL(obj, VAL_FOOD_POISONED) = TRUE;
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
            GET_OBJ_VAL(obj, VAL_FOOD_POISONED) = FALSE;
            to_char = "$p steams briefly.";
        }
        break;
    }

    if (to_char == NULL)
        send_to_char(NOEFFECT, ch);
    else
        act(to_char, TRUE, ch, obj, 0, TO_CHAR);

    if (to_room != NULL)
        act(to_room, TRUE, ch, obj, 0, TO_ROOM);
    else if (to_char != NULL)
        act(to_char, TRUE, ch, obj, 0, TO_ROOM);

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

int mag_creation(int skill, struct char_data *ch, int spellnum) {
    char *to_char = NULL, *to_room = NULL;
    struct obj_data *tobj;
    int z, zplus;
    int give_char = 0;
    if (ch == NULL)
        return 0;

    switch (spellnum) {
    case SPELL_CREATE_SPRING:
        switch (SECT(IN_ROOM(ch))) {
        case SECT_SHALLOWS:
        case SECT_WATER:
        case SECT_UNDERWATER:
        case SECT_AIR:
            cprintf(ch, "Nothing happens.\r\n");
            act("$n completes $s spell, but nothing happens.", TRUE, ch, 0, 0, TO_ROOM);
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
        act(to_room, FALSE, ch, tobj, 0, TO_ROOM);
    if (to_char)
        act(to_char, FALSE, ch, tobj, 0, TO_CHAR);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

int mag_room(int skill, struct char_data *ch, int spellnum) {
    int eff;   /* what effect */
    int ticks; /* how many ticks this spell lasts */
    char *to_char = NULL;
    char *to_room = NULL;
    struct room_effect_node *reff;

    ticks = 0;
    eff = -1;

    if (ch == NULL)
        return 0;

    switch (spellnum) {
    case SPELL_WALL_OF_FOG:
        to_char = "You create a fog out of nowhere.";
        to_room = "$n creates a fog out of nowhere.";
        eff = ROOM_EFF_FOG;
        ticks = 1; /* this spell lasts one tick */
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
    CREATE(reff, struct room_effect_node, 1);
    reff->room = ch->in_room;
    reff->timer = ticks;
    reff->effect = eff;
    reff->spell = spellnum;
    reff->next = room_effect_list;
    room_effect_list = reff;

    /* set the affection */
    if (eff != -1)
        SET_FLAG(ROOM_EFFECTS(reff->room), eff);

    if (to_char == NULL)
        send_to_char(NOEFFECT, ch);
    else
        act(to_char, TRUE, ch, 0, 0, TO_CHAR);

    if (to_room != NULL)
        act(to_room, TRUE, ch, 0, 0, TO_ROOM);
    else if (to_char != NULL)
        act(to_char, TRUE, ch, 0, 0, TO_ROOM);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/* Add spells to the case statement here to make a spell's duration skill
   related the spell system automagically adds 1 to this so values start at 0
   Note that adding the default 1 added to a 0 will make the spell wear off
   when the next TICK (currently 75 secs) expires no matter how close it is.
   This gives the affect of nearly instantaneous expiration of the spell.
   */
int get_spell_duration(struct char_data *ch, int spellnum) {
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

int get_vitality_hp_gain(struct char_data *ch, int spellnum) {
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
bool affected_by_armor_spells(struct char_data *victim) {
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
        return FALSE;
    }
    return TRUE;
}

bool check_fluid_spell_ok(struct char_data *ch, struct char_data *victim, int spellnum, bool quiet) {
    if (!victim)
        return FALSE;
    if (RIGID(victim) || GET_LEVEL(victim) >= LVL_IMMORT)
        return TRUE;
    if (!spell_suitable_for_fluid_characters(spellnum)) {
        if (!quiet) {
            cprintf(victim, "The spell is unable to take hold in your substance.\r\n");
            if (ch && ch != victim)
                act("The spell is unable to alter $N's substance.", FALSE, ch, 0, victim, TO_CHAR);
        }
        return FALSE;
    }
    return TRUE;
}

void remove_unsuitable_spells(struct char_data *ch) {
    struct effect *eff, *next;

    if (!RIGID(ch)) {
        for (eff = ch->effects; eff; eff = next) {
            next = eff->next;
            if (!spell_suitable_for_fluid_characters(eff->type))
                active_effect_remove(ch, eff);
        }
    }
}

bool check_armor_spells(struct char_data *ch, struct char_data *victim, int spellnum) {
    if (affected_by_armor_spells(victim)) {
        if (ch == victim) {
            act("You seem to be protected enough already!", FALSE, ch, 0, 0, TO_CHAR);
            act("$n looks a little overprotective.", TRUE, ch, 0, 0, TO_ROOM);
        } else {
            act("$N seem to be protected enough already!", FALSE, ch, 0, victim, TO_CHAR);
            act("You seem to be protected enough already!", FALSE, ch, 0, victim, TO_VICT);
            act("$n looks a little overprotective.", TRUE, ch, 0, victim, TO_NOTVICT);
        }
        return TRUE;
    }
    return FALSE;
}

void destroy_opposite_wall(struct obj_data *wall) {
    void decay_object(struct obj_data * obj);
    int room, dir;
    struct obj_data *next;

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

/* Returns TRUE if something was seen. */
bool look_at_magic_wall(struct char_data *ch, int dir, bool sees_next_room) {
    struct obj_data *wall;
    room_num next_room;

    for (wall = world[ch->in_room].contents; wall; wall = wall->next_content) {
        if (GET_OBJ_TYPE(wall) == ITEM_WALL && GET_OBJ_VAL(wall, VAL_WALL_DIRECTION) == dir) {
            sprintf(buf, "%s &0is standing here.\r\n", wall->short_description);
            CAP(buf);
            send_to_char(buf, ch);
            return TRUE;
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
                    return TRUE;
                }
        }
    }

    return FALSE;
}

struct obj_data *find_wall_dir(int rnum, int dir) {
    struct obj_data *wall;

    for (wall = world[rnum].contents; wall; wall = wall->next_content)
        if (GET_OBJ_TYPE(wall) == ITEM_WALL && GET_OBJ_VAL(wall, VAL_WALL_DIRECTION) == dir)
            return wall;
    return NULL;
}

/* Returns TRUE if you were stopped by a magic wall. */
bool wall_block_check(struct char_data *actor, struct char_data *motivator, int dir) {
    struct obj_data *wall;

    if (GET_LEVEL(actor) >= LVL_GOD)
        return FALSE;
    if ((wall = find_wall_dir(actor->in_room, dir)) != NULL) {
        /* Found a wall; you'll be blocked. */

        /* See if a wall of ice will put out anyone's flames. */
        if (GET_OBJ_VAL(wall, VAL_WALL_SPELL) == SPELL_WALL_OF_ICE && EFF_FLAGGED(motivator, EFF_ON_FIRE)) {
            act("$n&0 spreads $mself out on $p&0 and with a &8&bsizzle&0, $s flames "
                "are put out.",
                FALSE, motivator, wall, 0, TO_ROOM);
            act("You spread yourself out on $p&0 and your flames go out in a "
                "&8&bsizzle of steam&0.",
                FALSE, motivator, wall, 0, TO_CHAR);
            REMOVE_FLAG(EFF_FLAGS(motivator), EFF_ON_FIRE);
        } else if (GET_OBJ_VAL(wall, VAL_WALL_SPELL) == SPELL_WALL_OF_ICE && EFF_FLAGGED(actor, EFF_ON_FIRE)) {
            act("$n&0 spreads $mself out on $p&0 and with a &8&bsizzle&0, $s flames "
                "are put out.",
                FALSE, actor, wall, 0, TO_ROOM);
            act("You spread yourself out on $p&0 and your flames go out in a "
                "&8&bsizzle of steam&0.",
                FALSE, actor, wall, 0, TO_CHAR);
            REMOVE_FLAG(EFF_FLAGS(actor), EFF_ON_FIRE);

            /* No flames being put out.  Is this a mounted situation? */
        } else if (actor && motivator && actor != motivator && RIDING(actor) == motivator) {
            act("You rode $N right into $p!", FALSE, actor, wall, motivator, TO_CHAR);
            act("Bump!  $n rides $N into $p.", FALSE, actor, wall, motivator, TO_ROOM);

            /* Just your standard walk-into-a-wall. */
        } else {
            act("Oof.  You bump into $p.", FALSE, actor, wall, 0, TO_CHAR);
            act("$n bumps into $p.", FALSE, actor, wall, 0, TO_ROOM);
        }
        return TRUE;
    }
    return FALSE;
}

/* Returns TRUE if you ran into a wall. */
bool wall_charge_check(struct char_data *ch, int dir) {
    struct obj_data *wall;
    int dam, chance;

    if ((wall = find_wall_dir(ch->in_room, dir)) == NULL)
        return FALSE;

    /* Found a wall for you to run into! */

    /* If walls were being damaged, this one should get hurt now.
     * It might also show visible cracking or even be destroyed. */

    act("You CHARGE at $p&0 and crash right into it!", FALSE, ch, wall, 0, TO_CHAR);
    act("$n &0CHARGES at $p&0 and crashes into it headfirst!", FALSE, ch, wall, 0, TO_ROOM);

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        /* You're going to get hurt... */

        chance = number(0, 101);
        dam = ((chance / 10) * (GET_LEVEL(ch) / 10)) + GET_LEVEL(ch);
        /* But you won't die... */
        if (GET_HIT(ch) - dam < -5)
            dam = GET_HIT(ch) + 5;
        hurt_char(ch, NULL, dam, TRUE);
        /* You fell to a sitting position (unless you were knocked out) */
        if (GET_POS(ch) >= POS_STANDING)
            alter_pos(ch, POS_SITTING, STANCE_ALERT);
        WAIT_STATE(ch, PULSE_VIOLENCE * 3);
    }

    /* Would like to send messages to the other room that the wall's in, but
     * we'd have to check each person to see if they were awake, can see the
     * wall, ... maybe send a message about sound if they can't see but are
     * awake, etc. */

    return TRUE;
}

int get_fireshield_damage(struct char_data *attacker, struct char_data *victim, int dam) {
    if (EFF_FLAGGED(attacker, EFF_MAJOR_GLOBE))
        act("&1&bThe globe around your body absorbs the burning flames!&0", FALSE, attacker, 0, 0, TO_CHAR);
    else {
        int amount = MIN((GET_LEVEL(victim) / 2 + number(1, GET_LEVEL(victim) / 10)),
                         dam / 3 + number(1, 1 + GET_LEVEL(victim) / 10));
        amount = dam_suscept_adjust(victim, attacker, NULL, amount, DAM_FIRE);
        if (amount > 0) {
            act("&1Your limbs are seared by $N&0&1's shield of flames.&0 (&1&8$i&0)", FALSE, attacker, (void *)amount,
                victim, TO_CHAR);
            act("&1$n&0&1's limbs are seared by your shield of flames.&0 (&3$i&0)", FALSE, attacker, (void *)amount,
                victim, TO_VICT);
            act("&1$n&0&1's limbs are seared by $N&0&1's shield of flames.&0 "
                "(&4$i&0)",
                FALSE, attacker, (void *)amount, victim, TO_NOTVICT);
        }
        return amount;
    }

    return 0;
}

int get_coldshield_damage(struct char_data *attacker, struct char_data *victim, int dam) {
    if (EFF_FLAGGED(attacker, EFF_MAJOR_GLOBE))
        act("&4&bThe globe around your body absorbs the killing ice!&0", FALSE, attacker, 0, 0, TO_CHAR);
    else {
        int amount = MIN((GET_LEVEL(victim) / 2 + number(1, 1 + GET_LEVEL(victim) / 10)),
                         dam / 3 + number(1, 1 + GET_LEVEL(victim) / 10));
        amount = dam_suscept_adjust(victim, attacker, NULL, amount, DAM_COLD);
        if (amount > 0) {
            act("&4You are impaled on $N&0&4's shield of ice.&0 (&1&8$i&0)", FALSE, attacker, (void *)amount, victim,
                TO_CHAR);
            act("&4$n&0&4 is impaled on your shield of ice.&0 (&3$i&0)", FALSE, attacker, (void *)amount, victim,
                TO_VICT);
            act("&4$n&0&4 is impaled on $N&0&4's shield of ice.&0 (&4$i&0)", FALSE, attacker, (void *)amount, victim,
                TO_NOTVICT);
        }
        return amount;
    }

    return 0;
}

int get_soulshield_damage(struct char_data *attacker, struct char_data *victim, int dam) {
    if ((IS_GOOD(attacker) && IS_EVIL(victim)) || (IS_EVIL(attacker) && IS_GOOD(victim))) {
        int amount = MIN(2 * GET_LEVEL(victim) / 5 + number(1, 1 + GET_LEVEL(victim) / 10),
                         3 * dam / 16 + number(1, 1 + GET_LEVEL(victim) / 10));
        amount = dam_suscept_adjust(victim, attacker, NULL, amount, DAM_ALIGN);
        if (amount > 0) {
            act("&7&b$n's soul suffers upon contact with your aura.&0 (&3$i&0)", TRUE, attacker, (void *)amount, victim,
                TO_VICT);
            act("&7&bYour soul suffers upon contact with $N's aura.&0 (&1&8$i&0)", TRUE, attacker, (void *)amount,
                victim, TO_CHAR);
            act("&7&b$n's soul suffers upon contact with $N's aura.&0 (&4$i&0)", TRUE, attacker, (void *)amount, victim,
                TO_NOTVICT);
        }
        return amount;
    }

    return 0;
}

int defensive_spell_damage(struct char_data *attacker, struct char_data *victim, int dam) {
    int shdam = 0;

    if (EFF_FLAGGED(victim, EFF_FIRESHIELD))
        shdam += get_fireshield_damage(attacker, victim, dam);
    if (EFF_FLAGGED(victim, EFF_COLDSHIELD))
        shdam += get_coldshield_damage(attacker, victim, dam);
    if (EFF_FLAGGED(victim, EFF_SOULSHIELD))
        shdam += get_soulshield_damage(attacker, victim, dam);

    return shdam;
}

/***************************************************************************
 * $Log: magic.c,v $
 * Revision 1.296  2010/06/05 18:58:47  mud
 * Rebalance pyre self damage.
 *
 * Revision 1.295  2010/06/05 18:35:47  mud
 * Make pyre auto-target caster if sacrificial preference is
 * toggled on.
 *
 * Revision 1.294  2010/06/05 05:29:00  mud
 * Typo in full heal message.
 *
 * Revision 1.293  2010/06/05 04:43:57  mud
 * Replacing ocean sector type with cave.
 *
 * Revision 1.292  2009/08/02 20:19:58  myc
 * Adding pyre and fracture spells.
 *
 * Revision 1.291  2009/07/18 01:17:23  myc
 * Adding decay, iron maiden, spinechiller, and bone draw spells
 * for necromancer.
 *
 * Revision 1.290  2009/07/17 06:05:35  myc
 * Make waterwalk last a long time.
 *
 * Revision 1.289  2009/07/04 16:23:13  myc
 * Soulshield, holy word, and unholy word now use regular alignment
 * checks instead of arbitrary 500 and -500 values.
 *
 * Revision 1.288  2009/06/10 18:50:40  myc
 * Reduce the amount of damage breath attacks do.
 *
 * Revision 1.287  2009/06/09 05:43:38  myc
 * Changing a printf to a real log call.
 *
 * Revision 1.286  2009/05/19 19:36:52  myc
 * Fix typo in chain lightning.
 *
 * Revision 1.285  2009/03/21 06:32:37  jps
 * Make phosphoric embers manual spell
 *
 * Revision 1.284  2009/03/20 06:08:18  myc
 * Adding detonation, phosphoric embers, positive field, and acid
 * burst and single-target offensive pyromancer spells.  Increased
 * melt's power vs metal, stone, and ice.  Made soul tap a damage-
 * over-time version of energy drain.
 *
 * Revision 1.283  2009/03/16 19:17:52  jps
 * Change macro GET_HOME to GET_HOMEROOM
 *
 * Revision 1.282  2009/03/15 22:31:24  jps
 * Make cold/fire/soulshield damage adjust for susceptibility
 *
 * Revision 1.281  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.280  2009/03/09 16:57:47  myc
 * Added detect poison effect.
 *
 * Revision 1.279  2009/03/09 04:50:38  myc
 * Fix typo (missing newline) in divine essence.
 *
 * Revision 1.278  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.277  2009/03/09 03:26:34  jps
 * Moved individual spell definitions to spell.c and spell.h.
 *
 * Revision 1.276  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.275  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.274  2009/03/07 22:31:20  jps
 * Make the victim's set-on-fire message go to the victim's room with
 * the TO_VICTROOM act flag, since the perpetrator might be in another room.
 *
 * Revision 1.273  2009/03/07 20:46:28  jps
 * Add bold to messages colored with &9 - they can be invisible otherwise
 *
 * Revision 1.272  2009/03/07 11:24:26  jps
 * Fix wall of fog spell.
 *
 * Revision 1.271  2009/03/03 19:43:44  myc
 * Always apply group spells to the caster last so spells like
 * group recall still work.
 *
 * Revision 1.270  2009/02/04 20:03:56  myc
 * Disallow guard skill if it would result in PK.
 *
 * Revision 1.269  2009/01/19 09:25:23  myc
 * Removed MOB_PET flag.
 *
 * Revision 1.268  2009/01/19 08:42:29  myc
 * Add damage numbers to coldshield/fireshield/soulshield.
 *
 * Revision 1.267  2009/01/18 06:52:37  myc
 * Fix typo (capitalize a sentence in act()).
 *
 * Revision 1.266  2008/11/09 03:47:57  myc
 * Cut duration on poison and ray of enfeeblement.
 *
 * Revision 1.265  2008/09/29 00:03:13  jps
 * Moved weight_change_object to objects.c/h.
 *
 * Revision 1.264  2008/09/21 21:04:20  jps
 * Passing cast type to mag_affect so that potions of bless/dark presence can be
 *quaffed by neutral people.
 *
 * Revision 1.263  2008/09/21 20:46:36  jps
 * Removing unused variable
 *
 * Revision 1.262  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.261  2008/09/20 17:39:12  jps
 * Fix typos in bone armor messages.
 *
 * Revision 1.260  2008/09/20 17:23:20  jps
 * Fix rigidity check with certain spells.
 *
 * Revision 1.259  2008/09/20 07:27:45  jps
 * set_fighting takes a 3rd parameter, reciprocate, which will set the attackee
 *fighting the attacker if true.
 *
 * Revision 1.258  2008/09/20 06:05:06  jps
 * Add macros POSSESSED and POSSESSOR.
 *
 * Revision 1.257  2008/09/16 17:06:32  rsd
 * Made the damage bonus based on class for firestorm the
 * class of pyromancer instead of cryomancer..
 *
 * Revision 1.256  2008/09/14 04:41:41  jps
 * Don't allow mortal spells to affect imms.
 *
 * Revision 1.255  2008/09/14 04:34:30  jps
 * Set folks fighting even if the initial attack was completely ineffective.
 *
 * Revision 1.254  2008/09/14 04:09:24  jps
 * Take certain act flags off summoned mobs, such as peaceful and protector.
 *
 * Revision 1.253  2008/09/14 02:22:53  jps
 * Expand the suitability-for-fluid-chars check to be used for all mag_effect
 *spells.
 *
 * Revision 1.252  2008/09/14 02:08:01  jps
 * Use standardized area attack targetting
 *
 * Revision 1.251  2008/09/13 18:52:20  jps
 * Removing unused variables
 *
 * Revision 1.250  2008/09/13 18:05:29  jps
 * Added functions to remove spells from characters when necessary.
 *
 * Revision 1.249  2008/09/13 16:03:53  jps
 * Fix check_armor_spells
 *
 * Revision 1.248  2008/09/12 20:16:49  jps
 * Make it impossible to cast certain armor spells onto characters who are in a
 *fluid state.
 *
 * Revision 1.247  2008/09/09 08:23:37  jps
 * Placed sector info into a struct and moved its macros into rooms.h.
 *
 * Revision 1.246  2008/09/07 01:28:17  jps
 * Don't automatically start flying if your load is too heavy.
 *
 * Revision 1.245  2008/09/05 21:52:56  myc
 * Fix typo in elemental warding.
 *
 * Revision 1.244  2008/09/04 06:47:36  jps
 * Changed sector constants to match their strings
 *
 * Revision 1.243  2008/09/02 07:19:56  jps
 * Using limits.h.
 *
 * Revision 1.242  2008/09/02 07:16:39  jps
 * Changing object TIMER to DECOMP where appropriate.
 *
 * Revision 1.241  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.240  2008/08/31 06:51:00  myc
 * Reverse sign on modifiers for APPLY_AC so they follow the same scheme as
 * AC applies on equipment.
 *
 * Revision 1.239  2008/08/29 19:18:05  myc
 * Fixed abilities so that no information is lost; the caps occur
 * only when the viewed stats are accessed.
 *
 * Revision 1.238  2008/08/29 16:55:00  myc
 * Fixed messages for many self-only spells so they make sense even if
 * cast on someone else.
 *
 * Revision 1.237  2008/08/28 21:50:05  jps
 * Prevent "create spring" from working in watery or air rooms.
 *
 * Revision 1.236  2008/08/26 04:39:21  jps
 * Changed IN_ZONE to IN_ZONE_RNUM or IN_ZONE_VNUM and fixed zone_printf.
 *
 * Revision 1.235  2008/08/26 02:29:11  jps
 * Change phantasm mob list
 *
 * Revision 1.234  2008/08/24 03:15:57  myc
 * Make evades_spell absolutely fail when the skill's damage type
 * is 'undefined', not 'slashing'.
 *
 * Revision 1.233  2008/08/18 01:35:38  jps
 * Replaced all \\n\\r with \\r\\n, not that it was really necessary...
 *
 * Revision 1.232  2008/08/17 20:24:45  jps
 * Add wall_charge_check, for doorbash toward a wall.
 *
 * Revision 1.231  2008/08/17 06:39:47  jps
 * Fix grammar of cloak of gaia.
 *
 * Revision 1.230  2008/08/10 19:34:37  jps
 * Calibrated sorcerer_single_target spells to the level at which the spell
 * was assigned to the caster.
 *
 * Revision 1.229  2008/08/10 01:58:49  jps
 * Added spells severance and soul reaver for illusionists.
 *
 * Revision 1.228  2008/08/09 20:35:57  jps
 * Changed sense life so that it has a chance of detecting the presence and
 *movement of creatures with a "healable" life force. Increased spell duration
 *to 17-50 hrs.
 *
 * Revision 1.227  2008/08/03 19:31:44  myc
 * Fixing heatwave/cone of cold.
 *
 * Revision 1.226  2008/06/21 07:04:21  jps
 * Don't allow setting-on-fire of folks with a low susceptibility to fire
 *damage.
 *
 * Revision 1.225  2008/06/13 18:43:29  jps
 * Handle spell evasion differently for dispel magic, since it has other
 * purposes then simply doing harm.
 *
 * Revision 1.224  2008/06/11 22:24:43  jps
 * Cancel spellcasting when you become "frozen up" due to cold attacks.
 *
 * Revision 1.223  2008/06/07 19:06:46  myc
 * Moved all object-related constants and structures to objects.h
 *
 * Revision 1.222  2008/06/05 02:07:43  myc
 * Changed object flags to use flagvectors.
 *
 * Revision 1.221  2008/05/19 06:16:40  jps
 * Only mesmerize players for 2 hours.
 * Stop people who are fighting someone who gets mesmerized.
 *
 * Revision 1.220  2008/05/19 05:49:08  jps
 * Add mesmerize to mag_affect.
 *
 * Revision 1.219  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.218  2008/05/18 17:58:49  jps
 * Adding familiarity to mag_affect.
 *
 * Revision 1.217  2008/05/18 04:40:59  jps
 * Make is to people without detect invis can identify the
 * object that's being made invisible.
 *
 * Revision 1.216  2008/05/17 22:03:01  jps
 * Moving room-related code into rooms.h and rooms.c.
 *
 * Revision 1.215  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.214  2008/05/14 05:11:10  jps
 * Using hurt_char for play-time harm, while alter_hit is for changing hp only.
 *
 * Revision 1.213  2008/05/12 04:47:36  jps
 * Require evilness to cast dark presence, and goodness to cast bless.
 *
 * Revision 1.212  2008/05/12 04:41:59  jps
 * Make discorporate destroy illusory mobs.
 *
 * Revision 1.211  2008/05/12 00:43:48  jps
 * Add nightmare and discorporate spells to mag_damage.
 *
 * Revision 1.210  2008/05/11 07:11:53  jps
 * Calling active_effect_remove so as to standardize what happens when
 * an effect goes away.
 *
 * Revision 1.209  2008/05/11 06:13:31  jps
 * Adding defensive_spell_damage(), which calculates damage to attackers
 * due to spells like fireshield, and sends messages.
 *
 * Revision 1.208  2008/05/11 05:50:15  jps
 * alter_hit() now takes the killer.
 *
 * Revision 1.207  2008/04/20 04:42:24  jps
 * Don't send dodging messages when you resist the long-term
 * effects of gas breath.
 *
 * Revision 1.206  2008/04/19 21:22:06  myc
 * Missing break in mag_damage.
 *
 * Revision 1.205  2008/04/14 08:36:53  jps
 * Updated call to ASPELL, since it now includes the spell number.
 *
 * Revision 1.204  2008/04/14 05:11:40  jps
 * Renamed EFF_FLYING to EFF_FLY, since it only indicates an ability
 * to fly - not that the characer is actually flying.
 *
 * Revision 1.203  2008/04/14 02:17:45  jps
 * Adding glory to mag_affect().
 *
 * Revision 1.202  2008/04/13 19:38:17  jps
 * Reduced the length of confusion.
 *
 * Revision 1.201  2008/04/13 18:44:42  jps
 * Fix confusion-application message.
 *
 * Revision 1.200  2008/04/13 18:30:14  jps
 * Add confusion spell to mag_affect().
 *
 * Revision 1.199  2008/04/12 21:29:39  jps
 * Adding special case to evades_spell for dispel magic.
 *
 * Revision 1.198  2008/04/12 21:13:18  jps
 * Using new header file magic.h.
 *
 * Revision 1.197  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.196  2008/04/06 04:58:31  jps
 * Change catching-on-fire and freezing-up to use susceptibility.
 *
 * Revision 1.195  2008/04/05 17:10:57  jps
 * Fix message formatting for enlarge/reduce spells.
 *
 * Revision 1.194  2008/04/05 16:32:24  jps
 * Better feedback for spells that fail to inflict any damage, except that
 * dispel magic won't cause such a message.
 *
 * Revision 1.193  2008/04/05 04:50:39  jps
 * Except MAG_MANUAL spells from boolean spell evasion.
 *
 * Revision 1.192  2008/04/04 21:32:42  jps
 * Un-break area attack spells.
 *
 * Revision 1.191  2008/04/02 03:24:44  myc
 * Rewrote group code and removed major group code.
 *
 * Revision 1.190  2008/03/29 16:28:10  jps
 * Update evades_spell() to use power, and to check differently if the spell
 * is one that doesn't do damage.
 *
 * Revision 1.189  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.188  2008/03/27 17:40:52  jps
 * Don't allow blessing evil characters.  Lengthen the duration
 * of bless and dark presence to 10-24 hours.
 *
 * Revision 1.187  2008/03/27 17:30:17  jps
 * Dark presence now sets AFF3_HEX. It also places it on objects.
 * A hexed weapon will become !GOOD and a blessed weapon will
 * become !EVIL.
 *
 * Revision 1.186  2008/03/26 23:12:19  jps
 * Changed the waterform and vaporform spells to change your composition
 * rather than setting flags.
 *
 * Revision 1.185  2008/03/26 22:01:10  jps
 * Adding mag_damage for dispel magic, like a 3rd circle spell.
 *
 * Revision 1.184  2008/03/26 19:16:17  jps
 * Fix susceptibility check - the VICTIM not the attacker!
 * Also move damage reduction past harness check so that all of
 * the damage is subject to reduction.
 *
 * Revision 1.183  2008/03/26 19:09:27  jps
 * Change the bless spell so that it's very picky about what can
 * be blessed. Essentially, it has to be devoid of enchantments
 * and be rather plain. It can't be !GOOD or too high level
 * compared to the casting power.
 *
 * Revision 1.182  2008/03/26 18:15:59  jps
 * Adding a BLESS effect to a creature who gets bless cast on it.
 *
 * Revision 1.181  2008/03/26 16:44:36  jps
 * Replaced all checks for undead race with checks for undead lifeforce.
 * Replaced the undead race with the plant race.
 *
 * Revision 1.180  2008/03/24 08:42:04  jps
 * Removing protected_from_spell() and immune_from_spell().
 * Changing the way undead servants are generated, since the race
 * of undead is soon to be no more.
 * Taking into account composition-susceptibility to magical
 * damage.
 *
 * Revision 1.179  2008/03/23 18:42:39  jps
 * Using the new damage types defined in chars.h.
 *
 * Revision 1.178  2008/03/22 16:42:45  jps
 * Fix to_room messages for dark presence.
 *
 * Revision 1.177  2008/03/22 16:27:57  jps
 * Correctly copy sizes in duplicate_char.
 * Phantasms will copy the *current* size of a model (if any), disregarding
 * the fact that the model might have had its size modified by magic.
 * Phantasms will be neutral, because they have no souls and cannot be
 * good or evil.
 *
 * Revision 1.176  2008/03/11 02:53:09  jps
 * Update duplicate_char for size-tracking variables.
 *
 * Revision 1.175  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.
 *
 * Revision 1.174  2008/03/09 18:14:25  jps
 * Add spell effects for misdirection spell.
 *
 * Revision 1.173  2008/03/09 08:59:25  jps
 * Remove fear from mag_affects.
 *
 * Revision 1.172  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.171  2008/03/09 04:02:08  jps
 * Fix up mob summoning and phantasms a lot. Player phantasms don't work
 * quite right yet.
 *
 * Revision 1.170  2008/02/23 01:03:54  myc
 * Removed the lowest_circle in the skillinfo struct.  Use lowest_level
 * instead.
 *
 * Revision 1.169  2008/02/09 21:07:50  myc
 * Must provide a boolean to event_create saying whether to
 * free the event obj when done or not.  And instead of creating
 * an event obj for extract events, we'll just pass the victim in.
 *
 * Revision 1.168  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.167  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.166  2008/01/29 16:51:12  myc
 * Moving skill names to the skilldef struct.
 *
 * Revision 1.165  2008/01/28 02:39:01  jps
 * Use extract event for exorcisms.
 *
 * Revision 1.164  2008/01/27 21:14:59  myc
 * Replace hit() with attack().  Adding berserker chants.
 *
 * Revision 1.163  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species
 *into races.
 *
 * Revision 1.162  2008/01/27 12:12:55  jps
 * Changed IS_THIEF macro to IS_ROGUE.
 *
 * Revision 1.161  2008/01/27 09:45:41  jps
 * Got rid of the MCLASS_ defines and we now have a single set of classes
 * for both players and mobiles.
 *
 * Revision 1.160  2008/01/27 02:37:03  jps
 * Lower breath damage a bit.
 *
 * Revision 1.159  2008/01/27 00:18:12  jps
 * Make breath damage appropriate to the level of the mob. No more level
 * 3 demons doing 300 damage!!!
 *
 * Revision 1.158  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.157  2008/01/26 12:33:18  jps
 * Use skills.h to import improve_skill().
 *
 * Revision 1.156  2008/01/25 11:54:29  jps
 * Add a newline to the long description of raised undead creatures.
 *
 * Revision 1.155  2008/01/25 11:47:22  jps
 * Fix grammar in negate heat spell message.
 *
 * Revision 1.154  2008/01/24 15:45:09  myc
 * Fixed a typo in magic.c
 *
 * Revision 1.153  2008/01/15 03:26:00  myc
 * Fixing a typo in sleep spell.
 *
 * Revision 1.152  2008/01/14 18:52:52  myc
 * Auto-set the violent flag on a spell if the spell has the
 * MAG_DAMAGE routine.  Fix soul tap to not assign the effect
 * if the attack cannot occur.  Fix evades_spell to only go
 * off for violent spells.
 *
 * Revision 1.151  2008/01/13 23:06:04  myc
 * Changed the skills struct to store lowest_level and lowest_circle
 * data for each spell, thus making it unnecessary to calculate the
 * minimum spell circle for a spell everytime it goes through the
 * evades_spell function.
 *
 * Revision 1.150  2008/01/13 03:19:53  myc
 * Fixed a bug in check_guard.  Split check_armor_spells into
 * check_armor_spells and affected_by_armor_spells.
 *
 * Revision 1.149  2008/01/12 23:13:20  myc
 * Renamed clearMemory clear_memory.
 *
 * Revision 1.148  2008/01/11 17:34:32  myc
 * Fixing a typo in sanctuary spell message.
 *
 * Revision 1.147  2008/01/10 05:39:43  myc
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.146  2008/01/07 11:56:45  jps
 * Make sure illusory mobs have no cash. Don't use puff for player
 * template, as she has a spec proc. Make sure player illusions
 * are not sentinel. Set their long desc (not that it's likely
 * to be seen).
 *
 * Revision 1.145  2008/01/07 10:37:19  jps
 * Change spell name "project" to "phantasm". Allow simulacrom to
 * make copies of players.
 *
 * Revision 1.144  2008/01/06 23:50:47  jps
 * Added spells project and simulacrum, and MOB2_ILLUSORY flag.
 *
 * Revision 1.143  2008/01/06 20:38:00  jps
 * Remove unused saving throw tables.
 *
 * Revision 1.142  2008/01/05 05:44:19  jps
 * Using update_char() for created mobs.
 *
 * Revision 1.141  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.140  2008/01/01 07:32:56  jps
 * Made cold-spell freeze-ups into an event so that it can set
 * paralysis which won't be removed immediately after it is set.
 *
 * Revision 1.139  2008/01/01 05:08:34  jps
 * Stop area spells from harming things it shouldn't.
 *
 * Revision 1.138  2007/12/26 08:08:09  jps
 * Fix formatting of caster's feedback for "conceal".
 *
 * Revision 1.137  2007/11/28 10:19:39  jps
 * Reduce damage from low-level NPC sorcerers.
 *
 * Revision 1.136  2007/11/20 20:08:15  myc
 * Fixing more spells that weren't using online damage.
 *
 * Revision 1.135  2007/11/18 16:51:55  myc
 * Making mag_masses not hit your own pet.  Fixing some damage spells that
 * were doing less damage because they weren't using online damage.
 *
 * Revision 1.134  2007/10/27 21:49:34  myc
 * Fixed a bug in mass invis
 *
 * Revision 1.133  2007/10/27 18:56:18  myc
 * Typo in disease spell.
 *
 * Revision 1.132  2007/10/27 03:19:24  myc
 * Fixed a bug with war cry spewing garbage messages.
 *
 * Revision 1.131  2007/10/13 05:18:11  myc
 * Whoa immolate hits 5 times, and was doing insane damage with the new
 * sorcerer spell damage calculations.
 * Now it's not.
 *
 * Revision 1.130  2007/10/13 05:07:24  myc
 * Added new monk chants.
 *
 * Revision 1.129  2007/10/11 20:14:48  myc
 * Monk chants are now implemented as magic spells, and use spell wearoffs
 * instead of song wearoffs.  Removed the spell wearoff messages array;
 * spell wearoff messages are now defined for each spell in the spello
 * call in spell_parser.c
 *
 * Revision 1.128  2007/10/04 16:20:24  myc
 * Replaced magic_wall_destruction with destroy_opposite_wall, which calls
 * decay_object in limits.c to actually decay the object.
 *
 * Revision 1.127  2007/10/02 02:52:27  myc
 * A player casting mag_damage spells at another player will no longer
 * improve in skill.  Sense life applies the bit instead of a perception
 * bonus again.  (The perception is dynamically computed in db.c in
 * update_stats.)
 *
 * Revision 1.126  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  Concealment now gives you hide
 * points, sense life gives you perception, natures embrance gives
 * you hide points and camo, and made mag_points support hide points.
 *
 * Revision 1.125  2007/09/20 09:48:06  jps
 * Don't allow energy draining of undead.
 *
 * Revision 1.124  2007/09/20 09:16:32  jps
 * Improve grammer in lights-on-fire message
 *
 * Revision 1.123  2007/09/15 15:36:48  myc
 * Nature's embrace now sets camouflage bit, which lets you be hidden as
 * long as you are outside.  Cleaned up affect-handling code a lot.  It
 * didn't properly handle bitvectors 1, 2, and 3.  Now it does.
 * Invigorate is now a group spell.
 *
 * Revision 1.122  2007/09/15 05:03:46  myc
 * Implementing MOB2_NOPOISON flag.
 *
 * Revision 1.121  2007/09/11 16:34:24  myc
 * Added electrify skill for use by druid's electric eel.
 *
 * Revision 1.120  2007/09/09 01:20:14  jps
 * The result of casting a spell is no longer just TRUE or FALSE,
 * but two possible bits combined: charge and/or improve. If
 * CAST_RESULT_CHARGE is returned, the spell was used and the caster
 * will be charged (have the spell erased from memory).  If
 * CAST_RESULT_IMPROVE is returned, the caster may improve in that
 * sphere of magic.
 * At the same time, casters will now correctly be charged for
 * spells that are cast on objects.
 *
 * Revision 1.119  2007/09/07 01:37:09  jps
 * Standardized the damage done by single-target sorcery spells.
 *
 * Revision 1.118  2007/09/04 06:49:19  myc
 * Changed spells that cannot be cast indoors to use the new TAR_OUTDOORS
 * bit, meaning the character's spell memory won't be charged when they
 * cast such a spell indoors.  (The check comes at command-interpretation
 * time.)
 *
 * Revision 1.117  2007/09/03 23:49:40  jps
 * Add mass_attack_ok() so that you could kill your own pet specifically,
 * but your area spells will not harm it.
 *
 * Revision 1.116  2007/09/03 21:21:17  jps
 * Magic wall updates: seen when looking in a direction; will block movement
 * sensibly, taking into account mounts; wall of ice can be used to douse
 * your flames; expiration message varies depending on what kind of
 * magic wall it is.
 *
 * Revision 1.115  2007/09/02 22:54:55  jps
 * Minor typo fixes.
 *
 * Revision 1.114  2007/08/30 08:51:25  jps
 * Generalize spell evasion (globe, elemental immunity, immortal) to area
 *spells.
 *
 * Revision 1.113  2007/08/29 01:22:18  jps
 * Check for null target in check_guard.
 *
 * Revision 1.112  2007/08/28 20:17:29  myc
 * Gods won't catch on fire anymore.  The caster of an affection spell will
 * now see the to_room message if there is no to_char message and the caster
 * is not the victim.
 *
 * Revision 1.111  2007/08/26 21:49:10  jps
 * Don't say that someone stops chanting if they've been knocked out
 * or killed.
 *
 * Revision 1.110  2007/08/26 21:10:27  jps
 * Provide a caster feedback message for the poison spell.
 *
 * Revision 1.109  2007/08/26 19:51:43  jps
 * Fix ray of enfeeblement resisted-spell message.
 *
 * Revision 1.108  2007/08/26 01:55:41  myc
 * Fire now does real damage.  All fire spells have a chance to catch the
 * victim on fire.  Mobs attempt to douse themselves.
 *
 * Revision 1.107  2007/08/23 00:32:24  jps
 * All four elemental immunities are in effect. An elemental
 * resistance (such as PROT-FIRE) will mean 12.5% extra damage
 * from the opposing element (fire-water, air-earth). An elemental
 * immunity will cause 25% extra damage from the opposing element.
 *
 * Revision 1.106  2007/08/15 20:47:06  myc
 * Fly will now only automatically set you to POS_FLYING if you are awake.
 *
 * Revision 1.105  2007/08/03 22:00:11  myc
 * Fixed some \r\n typoes in send_to_chars.
 *
 * Revision 1.104  2007/08/03 03:51:44  myc
 * check_pk is now attack_ok, and covers many more cases than before,
 * including peaced rooms, shapeshifted pk, and arena rooms.  Almost all
 * offensive attacks now use attack_ok to determine whether an attack is
 * allowed.
 *
 * Revision 1.103  2007/08/02 04:19:32  jps
 * Added "moonbeam" spell for Druids.
 * Stop people from fighting when they become paralyzed.
 *
 * Revision 1.102  2007/08/02 01:04:10  myc
 * check_pk() now works for all PK cases.  Moved from magic.c to fight.c
 *
 * Revision 1.101  2007/08/02 00:23:53  myc
 * Standardized magic check-PK function.  Cut out a LOT of unnecessary magic
 * code and cleaned up the whole system in general.  Magic casts are now
 * guaranteed to use sphere skills rather than level.  Almost all magic
 * functions like mag_damage or even manual spells return a boolean now:
 * TRUE if the cast deserves a skill improvement, FALSE if it doesn't.
 * This return value is ignored for object magic (wands, etc.).
 *
 * Revision 1.100  2007/07/25 00:38:03  jps
 * Earthquake will send a message about shaking to the entire zone.
 *
 * Revision 1.99  2007/07/19 15:05:34  jps
 * Minor typo fixes
 *
 * Revision 1.98  2007/07/15 17:16:12  jps
 * Add IS_POISONED macro.
 *
 * Revision 1.97  2007/07/04 02:21:58  myc
 * Removing coldshield affect from ice armor spell.  Increased duration on
 * magic torch and circle of light.  Renamed douse spell to extinguish.
 *
 * Revision 1.96  2007/06/24 22:45:31  myc
 * Making chill touch improve skill always instead of sometimes.
 *
 * Revision 1.95  2007/06/16 00:15:49  myc
 * Three spells for necromancers: soul tap, rebuke undead,
 * and degeneration.  One spell for rangers: natures guidance.
 *
 * Revision 1.94  2007/05/24 06:06:16  jps
 * Stop sending 'Nothing seems to happen.' when full heal is cast.
 *
 * Revision 1.93  2007/05/12 21:59:07  myc
 * Fixed the bug where random constants were sent_to_char when skill
 * affections wore off.
 *
 * Revision 1.92  2007/05/12 20:02:03  myc
 * Demonic aspect and mutation shouldn't stack.
 *
 * Revision 1.91  2007/05/11 21:03:12  myc
 * New rogue skill, eye gouge, allows rogues to gouge out eyes.  A very
 * complicated skill.  :P  Fixed cure blind's logic, and made it support
 * eye gouge too.
 *
 * Revision 1.90  2007/05/11 20:13:28  myc
 * Vaporform is a new circle 13 spell for cryomancers.  It significantly
 * increases the caster's chance of dodging a hit.  It is a quest spell.
 *
 * Revision 1.89  2007/04/26 03:57:09  myc
 * Got rid of sunray's double skill improvement haxness.
 *
 * Revision 1.88  2007/04/25 07:18:05  jps
 * Fix feedback for casting bless/dark presence on one who is already
 * oppositely blessed.  Make bless impossible if you have any of several
 * evil spells on you.  Make batwings/wings of heaven exclusive.
 *
 * Revision 1.87  2007/04/19 07:03:14  myc
 * Renamed RAY_OF_ENFEB as RAY_OF_ENFEEB.  Implemented demonic mutation
 * as a more powerful version of demonic aspect.  Made it so players
 * can cast offensive affection spells on themselves.
 *
 * Revision 1.86  2007/04/19 00:53:54  jps
 * Create macros for stopping spellcasting, and terminate spellcasting
 * when you become paralyzed.
 *
 * Revision 1.85  2007/04/17 23:58:43  jps
 * Stop sending extra message to caster when writhing weeds fails due to being
 *cast outside.
 *
 * Revision 1.84  2007/04/17 23:38:03  myc
 * Introducing the new improved color spray!  It's now an area spell that
 * causes various effects based on caster skill.
 *
 * Revision 1.83  2007/04/11 14:26:06  jps
 * Spell of nourishment will only work in certain terrains.
 *
 * Revision 1.82  2007/03/27 04:27:05  myc
 * Harness from 2% to 1% extra damage per level.  Forgot a check in ice armor
 * for coldshield last time.  Group heal has the same effects as heal (curing
 * blindness).  Permastoned mobs twitch faster.  Cure blind cures sunray.
 *
 * Revision 1.81  2007/02/20 17:16:27  myc
 * Consolidated armor spell checks into check_armor_spells() function, which
 * is called at the beginning of all armor spells.  Changed success rates for
 * entangle, minor paralysis, and sleep.  Cleaned up entangle spell a bit.
 * Sleep spell checks for shapeshifted players.
 *
 * Revision 1.80  2007/02/14 03:54:53  myc
 * Save applies now make a difference.  Reduced damage exorcism causes to
 *caster. Fixed bug with improving skill preventing other unrelated things from
 *occuring. Ice armor now has coldshield affect.  Entangle, minor paralysis, and
 *sleep no longer make the mob attack for successes.  Added combust and cremate
 *spells.
 *
 * Revision 1.79  2007/02/08 01:30:00  myc
 * Circle of fire does damage based on level now.
 *
 * Revision 1.78  2006/12/19 04:36:53  dce
 * Modified Supernova to mimic Ice Shards.
 *
 * Revision 1.77  2006/11/20 22:24:17  jps
 * End the difficulties in interaction between evil and good player races.
 *
 * Revision 1.76  2006/11/20 19:52:04  jps
 * Levitate halves earthquake damage.  Fixed feedback messages when
 * casting levitate and ray of enfeeblement.
 *
 * Revision 1.75  2006/11/18 07:03:30  jps
 * Minor typo fixes
 *
 * Revision 1.74  2006/11/18 04:26:32  jps
 * Renamed continual light spell to illumination, and it only works on
 * LIGHT items (still rooms too).
 *
 * Revision 1.73  2006/11/17 22:52:59  jps
 * Change AGGR_GOOD/EVIL_ALIGN to AGGR_GOOD/EVIL_RACE
 *
 * Revision 1.72  2006/11/14 18:54:02  jps
 * Fly spell now produces feedback to caster when cast on someone else.
 *
 * Revision 1.71  2006/11/13 19:24:00  jps
 * "animate dead" is in sphere of death, and improves.
 *
 * Revision 1.70  2006/11/13 18:33:58  jps
 * Fix major/minor globe interaction, and add feedback for the caster.
 *
 * Revision 1.69  2006/11/13 17:51:31  jps
 * Guard will improve normally.  Also there are messages for failed
 * guard attempts.
 *
 * Revision 1.68  2006/11/11 10:11:04  jps
 * Create food now chooses from 50 food objects (10 for each food
 * creating class), based on caster proficiency and luck.
 *
 * Revision 1.67  2006/11/08 09:59:46  jps
 * Added caster feedback message for "protection from evil".
 *
 * Revision 1.66  2006/11/08 09:16:04  jps
 * Fixed some loose-lose typos.
 *
 * Revision 1.65  2006/11/08 08:49:29  jps
 * Fix missing punctuation in missed poison spell message.
 *
 * Revision 1.64  2006/11/08 08:01:14  jps
 * Typo fix "You sprouts roots" -> "You sprout roots"
 *
 * Revision 1.63  2006/11/07 14:09:46  jps
 * If you are silenced in the middle of casting your own spell, that
 * spell fails.
 *
 * Revision 1.62  2006/11/06 17:38:19  jps
 * affect spells cast on self will no longer send two messages to the caster
 *
 * Revision 1.61  2006/07/20 07:34:53  cjd
 * Typo fixes.
 *
 * Revision 1.60  2006/05/30 00:51:18  rls
 * modified poison affects to be a bit more potent
 *
 * Revision 1.59  2004/11/28 06:38:07  rsd
 * Changed the healing multiplies back to half what they were
 * and doubled the number of dice in each healing spell.
 *
 * Revision 1.58  2004/11/19 03:09:58  rsd
 * Doubled healing output for the heal spells again.
 *
 * Revision 1.57  2004/11/13 10:45:48  rls
 * Fixed crashie bug with remove curse scroll... or so it seems.
 *
 * Revision 1.56  2004/10/15 18:33:30  rsd
 * Ok, something odd happened with this, I added a buncha
 * checks to the creation of undead to remove the aggressive
 * flags to prevent necros from killing off players.
 * Bad necro's the odd thing is that I fixed a typo while
 * having the file checed out in another directory and did
 * a make.  It's almost as though it checked in my changes and
 * updated the file on me automatically.  weirdness... .
 *
 * Revision 1.55  2003/08/21 02:36:16  jjl
 * Zzur said to double the healing power.  So I did.
 *
 * Revision 1.54  2003/06/20 15:04:56  rls
 * Capped energy drain at 6x caster's hp... 1 hp gain thereafter.
 *
 * Revision 1.53  2003/06/18 14:57:37  rls
 * Added a boolean to create_undead to check for PC corpses being raised
 * Allowed for PC corpse raising only when PK is enabled
 * Toned down the HP in create_undead to that of 2x the caster.
 *
 * Revision 1.52  2002/10/23 02:55:47  jjl
 * D'oh.  Fixed some test code I had in that was breaking necros.
 *
 * Revision 1.51  2002/10/14 02:16:08  jjl
 * An update to turn vitality into a set of 6 spells, lesser endurance,
 * endurance, greater endurance, vitality, greater vitality, and dragon's
 * health.  Greater endurance is what vitality was.  The rest are scaled
 * appropriately.    The higher end may need scaled down, or may not.
 *
 * Revision 1.50  2002/09/15 04:27:11  jjl
 * Fixed sundry typos in messages, added stone skin wear off message, wings of
 *heaven/hell make you fly now.
 *
 * Revision 1.49  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.48  2002/07/16 23:22:41  rls
 * added in new necro spell, bone armor
 *
 * Revision 1.47  2002/03/27 00:04:19  dce
 * *** empty log message ***
 *
 * Revision 1.45  2002/02/25 12:30:07  rls
 * *** empty log message ***
 *
 * Revision 1.44  2002/02/25 11:18:40  rls
 * Adj to_char msg for AFF_BLIND to ...blinded by you!
 *
 * Revision 1.43  2002/02/18 22:54:15  dce
 * When casting fly, the caster would be the one who's position
 * was set to fly. I changed it to the vicitim. So from
 * GET_POS(ch) to GET_POS(victim).
 *
 * Revision 1.42  2001/12/07 16:11:04  dce
 * Fix the heal spells so you can not get your hp's above max.
 *
 * Revision 1.41  2001/03/07 01:45:18  dce
 * Added checks so that players can not kill shapechanged players and
 * vise versa. Hopefully I didn't miss any...
 *
 * Revision 1.40  2000/11/22 20:35:18  rsd
 * Added back rlog messages from prior to the addition of
 * the $log$ string.
 *
 * Revision 1.39  2000/11/15 18:52:33  rsd
 * Magic users can no longer cast energy drain upon themselves
 * to gain hitpoints..
 *
 * Revision 1.38  2000/11/14 20:11:39  rsd
 * Added a pk check for the -str affect on chill touch.
 *
 * Revision 1.37  2000/10/13 22:26:52  rsd
 * altered the power of chain lightning
 *
 * Revision 1.36  2000/10/05 03:05:08  rsd
 * Altered almost every spell to redo the way skills were
 * accessed in the spells by objects and not casters to
 * use the level of the object to determine power and
 * not allow an object to cause a players proficiency to increase
 * by using its spell.
 *
 * Revision 1.35  2000/09/29 04:16:24  rsd
 * Altered every spell in mag_affects to use the new skill
 * checking code that checks if a player is casting or uses
 * an item spell.  This should restore potions and scrolls
 * et al to normal use.
 *
 * Revision 1.34  2000/09/28 20:37:15  jimmy
 * added fix to mag_affects that checks to see weather a spell is
 * being cast or if it's an affect from an object.  Objects will
 * get their skill based on the level of the object.  I.E pc/npc's
 * who don't have SPELL_ARMOR skill can quaff an armor potion
 * and get affected by an armor spell of the level of the potion.
 * All other mag_XXX functions can be migrated to join suit.
 * jbk
 *
 * Revision 1.33  2000/04/30 18:14:19  rsd
 * bless no longer has an alignement check, it does have a dark
 * presence check though, and dark presence has a bless check.
 *
 * Revision 1.32  2000/04/15 23:12:03  rsd
 * fixed damage algorythm for flood and meteorswarm added
 * iceshards and supernova...
 *
 * Revision 1.31  2000/04/14 00:55:49  rsd
 * altered a few spells
 *
 * Revision 1.30  2000/04/09 22:32:24  rsd
 * altered a buncha act buffers to try to get the spell messaging
 * done properly.  Also altered the order of ch and victim in
 * the catch all buffers for the order changed in the act()'s
 * for mag_affects().
 *
 * Revision 1.29  2000/04/08 08:41:23  rsd
 * altered prayer to have float math to return expected values.
 * altered shocking grasp to actually reference the proper
 * sphere to calculate damage, also made it to the 2nd power
 * instead of the typoed 7th. Changed about a kabillion act()
 * statements to make more sense I hope.
 *
 * Revision 1.28  2000/04/05 06:35:34  rsd
 * added 1 kabillion more spells into the spell system.
 * lalala.
 *
 * Revision 1.27  2000/04/02 02:39:39  rsd
 * changed the to_room act in most spells to be TO_NOTVICT as
 * I think was the intent in most cases.  Also added and
 * integrated the spell system up through all 4th circle
 * spells.
 *
 * Revision 1.26  2000/03/31 23:46:37  rsd
 * integrated more spells into the skill system with improve
 * skill calls. Also comeplted more spells. in mag_damage().
 *
 * Revision 1.25  2000/03/31 00:16:10  rsd
 * added a few variables to mag_damage() to make the use of the
 * spell system doable. Also changed how damage was assigned
 * to be proper for the intent for the first few spells.
 * Note that spells are becomeing significantly more potent,
 * maybe to much so, ptesting will get it figured out. This should
 * get all spells up through 3rd circle into the spell system.
 *
 * Revision 1.24  2000/03/30 05:45:54  rsd
 * added mag_damage spells, all first and second circle spells of all
 * classes are in the spell system now.
 * sync
 *
 * Revision 1.23  2000/03/29 06:29:50  rsd
 * Many mag affects spells had returns out for various reasons.
 * The messages associated with the returns were buffered but
 * never sent by the act()'s at the end of the function. Each
 * buffer for player messaging before a return now has an act
 * assicated with it.  I hope it's done correctly.
 *
 * Revision 1.22  2000/03/27 08:06:06  rsd
 * completed hosing with all the spells in mag_affectc()
 * except stone skin.
 *
 * Revision 1.21  2000/03/26 23:48:37  rsd
 * completed more of the mag_affects() spells.
 *
 * Revision 1.20  2000/03/26 07:21:20  rsd
 * Added an improve_skill() call to every mag_affects() spell case in
 * the switch, each spell has a base improve_skill() call just before
 * the break in their case.  Some have more than one instance of the
 * call............
 * Added appropriate to_vict, to_room, and to_char messages for the
 * first half of spells in mag_affects().
 * Removed the defines for af[0].element that I had put in mag_affects()
 * a few days ago.  I realized that there was af[i].element going on
 * so to just have the defines for af[0] was a bit silly. I removed the
 * use of the defines I had coded for barkskin, dark presence, armor,
 * bless, and demonskin......
 * Added alignment checks and restrictions on Demonskin, Dark Presence,
 * and bless.........
 * Worked though about the first half of the spells in mag_affects()
 * and integrated the spell proficiency system.  This was rather easy
 * compared to the pther mag() type spells as most of the mag_affects()
 * spells have very simple affects to manipulate.  There were a few
 * instances of spells where we want more special things to occur,
 * and some instances of confusion, but it's all commented. I admit
 * some of the things I did were goat sacrificing HACKS that may not
 * work. More work later....
 *
 * Revision 1.19  2000/03/25 21:36:13  rsd
 * Altered to use spell proficiencies:
 * chill touch
 * Barkskin
 * nightvision
 * dark presence
 * Also added a missing skill check for demonskin.
 *
 * Revision 1.18  2000/03/24 23:49:35  rsd
 * Altered bless and demonskin to use the spell prof system.
 *
 * Revision 1.17  2000/03/24 05:24:39  rsd
 * changed armor to use sphere of prot as skill proficiency.
 * Also declared some variables at the beginning of mag_affects()
 * to facilitate the use of skills in spells.
 *
 * Revision 1.16  2000/03/19 20:38:18  rsd
 * added brackes to the if statements of SD_INTERN_DAM and
 * tabbed it out.
 *
 * Revision 1.15  2000/03/18 06:08:38  rsd
 * Changed the overall duration of vitality as well as the ranges
 * at which the durations increase. Put some comments in about what
 * a default duration of 0 means to the players. Added an extern -
 * extern void improve_skill(struct char_data *ch, int skill);
 * This was done so skill improvemts could be called for spell usage
 * like with the ones added for vitality.
 *
 * Revision 1.14  2000/03/05 00:15:34  rsd
 * Er, had a buncha returns though the functioning parts of
 * Dispel good and evil, so it was returning out of mag_damage
 * after it listed the damage hehe silly me.
 *
 * Revision 1.13  2000/03/04 08:32:25  rsd
 * Altered DISPEL_GOOD DISPEL_EVIL and VAMPIRIC_BREATH
 * to be somewhat functional, more so than they were
 * before.  Although the dispells don't seem to call
 * their proper messages.
 * /s
 *
 * Revision 1.12  2000/02/26 02:17:21  rsd
 * Oooook, fixed healing proficiency cheks to actually include the 96
 * percentile by adding a = after the > in >96 grumbmle.
 *
 * Revision 1.11  1999/12/08 21:21:59  jimmy
 * added checks for spell proficiencies all healing spells and vitality.
 *
 * Revision 1.10  1999/11/29 01:32:51  cso
 * made chill touch decrease strength a little more
 *
 * Revision 1.9  1999/11/28 23:41:42  cso
 * affect_update: added check to kill animated mobs when the animate wears off
 * new fn: mod_for_undead_type: modify a mob's stats based on what undead type
 *  it is. used by create_undead
 * new fn: mod_for_lvldiff: modify a mob's stats based on the level difference
 *  between it and the necro that raised it. used by create_undead
 * new fn: create_undead: create an undead mob. used by mag_summons for
 *
 *  animate dead
 * new fn: ch_can_control_mob: check to see if a necro can control the undead
 *  he just raised. used by mag_summons for animate dead
 * removed unused mag_summons_msgs
 * removed unused mag_summon_fail_msgs
 * removed unused defines form MOB_ZOMBIE, MOB_MONSUM_I, etc
 * rewrote mag_summons from scratch
 *
 * Revision 1.8  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.7  1999/03/07 05:01:09  dce
 * Chant finishes and wearoff messages.
 *
 * Revision 1.6  1999/02/13 19:37:12  dce
 * Rewrote Continual Light and Darkness to be manual spells to meet our needs.
 *
 * Revision 1.5  1999/02/11 16:44:23  dce
 * When casting minor creation, light objects come lit.
 *
 * Revision 1.4  1999/02/10 05:57:14  jimmy
 * Added long description to player file.  Added AFK toggle.
 * removed NOAUCTION toggle.
 * fingon
 *
 * Revision 1.3  1999/02/10 02:38:58  dce
 * Fixes some of continual light.
 *
 * Revision 1.2  1999/01/31 16:35:11  mud
 * Indented entire file
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/
