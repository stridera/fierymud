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

#include "races.hpp"

#include "casting.hpp"
#include "charsize.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "lifeforce.hpp"
#include "math.hpp"
#include "regen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <math.h>

/* Prototypes */
void set_init_height_weight(CharData *ch);
static flagvector race_effects_mask[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];

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

void init_races(void) {
#define PERM_EFF(r, f) SET_FLAG(races[(r)].effect_flags, (f))
#define ADD_SKILL(s, p)                                                                                                \
    do {                                                                                                               \
        Races[race].skills[pos].skill = (s);                                                                           \
        Races[race].skills[pos].proficiency = (p);                                                                     \
        ++pos;                                                                                                         \
    } while (0)

    int race, pos;

    /*
     * Add permanent effects to races here.
     */
    PERM_EFF(RACE_DROW, EFF_INFRAVISION);
    PERM_EFF(RACE_DROW, EFF_ULTRAVISION);
    PERM_EFF(RACE_ELF, EFF_INFRAVISION);
    PERM_EFF(RACE_DWARF, EFF_DETECT_POISON);
    PERM_EFF(RACE_DWARF, EFF_INFRAVISION);
    PERM_EFF(RACE_DWARF, EFF_ULTRAVISION);
    PERM_EFF(RACE_DUERGAR, EFF_INFRAVISION);
    PERM_EFF(RACE_DUERGAR, EFF_ULTRAVISION);
    PERM_EFF(RACE_FAERIE_SEELIE, EFF_FLY);
    PERM_EFF(RACE_FAERIE_UNSEELIE, EFF_FLY);
    PERM_EFF(RACE_HALFLING, EFF_INFRAVISION);
    PERM_EFF(RACE_HALFLING, EFF_SENSE_LIFE);
    PERM_EFF(RACE_TROLL, EFF_INFRAVISION);
    PERM_EFF(RACE_TROLL, EFF_ULTRAVISION);
    PERM_EFF(RACE_OGRE, EFF_INFRAVISION);
    PERM_EFF(RACE_OGRE, EFF_ULTRAVISION);
    PERM_EFF(RACE_HALF_ELF, EFF_INFRAVISION);
    PERM_EFF(RACE_GNOME, EFF_INFRAVISION);
    PERM_EFF(RACE_SVERFNEBLIN, EFF_INFRAVISION);
    PERM_EFF(RACE_SVERFNEBLIN, EFF_ULTRAVISION);
    PERM_EFF(RACE_BROWNIE, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_FIRE, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_FROST, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_ACID, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_LIGHTNING, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_GAS, EFF_INFRAVISION);

    /*
     * Add race skills to the switch below.
     * If a constant value is declared, the skill will always reset back to that value.
     * Use 'proficiency' or 'ROLL_SKILL_PROF' instead.
     */
    for (race = 0; race < NUM_RACES; ++race) {
        memset(Races[race].skills, 0, sizeof(Races[race].skills));
        pos = 0;
        switch (race) {
        case RACE_ELF:
            ADD_SKILL(SKILL_SLASHING, ROLL_SKILL_PROF);
            break;
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
        case RACE_DRAGON_GENERAL:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_ACID_BREATH, 1000);
            ADD_SKILL(SPELL_FROST_BREATH, 1000);
            ADD_SKILL(SPELL_GAS_BREATH, 1000);
            ADD_SKILL(SPELL_FIRE_BREATH, 1000);
            ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
            break;
        case RACE_DRAGON_FIRE:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FIRE_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_FIRE:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FIRE_BREATH, 1000);
            break;
        case RACE_DRAGON_FROST:
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FROST_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_FROST:
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FROST_BREATH, 1000);
            break;
        case RACE_DRAGON_ACID:
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_ACID_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_ACID:
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_ACID_BREATH, 1000);
            break;
        case RACE_DRAGON_LIGHTNING:
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_LIGHTNING:
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
            break;
        case RACE_DRAGON_GAS:
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_GAS_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_GAS:
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_GAS_BREATH, 1000);
            break;
        case RACE_DEMON:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);
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
            sprintf(buf,
                    "init_races: Too many skills assigned to race %s.  "
                    "Increase NUM_RACE_SKILLS in races.h to at least %d",
                    Races[race].name, pos);
            log(buf);
            exit(1);
        }
    }

    CLEAR_FLAGS(race_effects_mask, NUM_EFF_FLAGS);
    for (race = 0; race < NUM_RACES; ++race)
        SET_FLAGS(race_effects_mask, Races[race].effect_flags, NUM_EFF_FLAGS);

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
int parse_race(CharData *ch, CharData *vict, char *arg) {
    int i, race = RACE_UNDEFINED, altname = RACE_UNDEFINED, best = RACE_UNDEFINED;

    if (!*arg) {
        if (ch)
            send_to_char("What race?\r\n", ch);
        return RACE_UNDEFINED;
    }

    for (i = 0; i < NUM_RACES; i++) {
        if (!strncasecmp(arg, Races[i].name, strlen(arg))) {
            if (!strcasecmp(arg, Races[i].name)) {
                race = i;
                break;
            }
            if (best == RACE_UNDEFINED)
                best = i;
        } else if (isname(arg, Races[i].names)) {
            if (altname == RACE_UNDEFINED)
                altname = i;
        } else if (is_abbrev(arg, Races[i].name)) {
            if (best == RACE_UNDEFINED)
                best = i;
        }
    }

    if (race == RACE_UNDEFINED)
        race = altname;
    if (race == RACE_UNDEFINED)
        race = best;
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

    if (!Races[race].playable) {
        if (ch) {
            sprintf(buf, "The %s race is not available to mortals.\r\n", Races[race].name);
            send_to_char(buf, ch);
        }
        return RACE_UNDEFINED;
    }

    if (!class_ok_race[race][(int)GET_CLASS(vict)]) {
        if (ch) {
            sprintf(buf, "As %s, $n can't be %s.", with_indefinite_article(classes[(int)GET_CLASS(vict)].displayname),
                    with_indefinite_article(Races[race].displayname));
            act(buf, false, vict, 0, ch, TO_VICT);
        }
        return RACE_UNDEFINED;
    }

    return race;
}

/* Send a menu to someone who's creating a character, listing the available
 * races.  We assume that this function would not have been called if
 * "races_allowed" were false. */
void send_race_menu(DescriptorData *d) {
    extern int evil_races_allowed;
    char idx;
    int i;

    write_to_output("\r\nThe following races are available:\r\n", d);
    for (i = 0, idx = 'a'; i < NUM_RACES; i++) {
        if (Races[i].playable && (evil_races_allowed || Races[i].racealign == RACE_ALIGN_GOOD)) {
            sprintf(buf, "  &7%c)&0 %s\r\n", idx, Races[i].fullname);
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
        if (Races[i].playable && (evil_races_allowed || Races[i].racealign == RACE_ALIGN_GOOD)) {
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
int natural_move(CharData *ch) {
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
                return MOUNT_MAXMOVE + 2 * (GET_LEVEL(ch) - MAX_MOUNT_LEVEL) + number(0, 9);
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

void init_proto_race(CharData *ch) {
    set_base_size(ch, Races[(int)GET_RACE(ch)].def_size);
    GET_LIFEFORCE(ch) = Races[(int)GET_RACE(ch)].def_lifeforce;
    BASE_COMPOSITION(ch) = Races[(int)GET_RACE(ch)].def_composition;
    GET_COMPOSITION(ch) = BASE_COMPOSITION(ch);
}

/* init_char_race()
 *
 * Sets beginning values that are appropriate for a brand-new character,
 * according to race. */

void init_char_race(CharData *ch) {
    if (!IS_NPC(ch) && VALID_RACE(ch)) {
        GET_BASE_DAMROLL(ch) = Races[(int)GET_RACE(ch)].bonus_damroll;
        GET_BASE_HITROLL(ch) = Races[(int)GET_RACE(ch)].bonus_hitroll;
    }

    /* NPCs will have their own align defined at build time,
     * and it might have been adjusted by the builder, too. */
    if (!IS_NPC(ch) && VALID_RACE(ch))
        GET_ALIGNMENT(ch) = Races[(int)GET_RACE(ch)].def_align;
    set_init_height_weight(ch);

    GET_MAX_MOVE(ch) = natural_move(ch);
}

void update_char_race(CharData *ch) {
    if (!VALID_RACE(ch)) {
        log("update_char_race: %s doesn't have a valid race (%d).", GET_NAME(ch), GET_RACE(ch));
        return;
    }

    GET_RACE_ALIGN(ch) = Races[(int)GET_RACE(ch)].racealign;

    /* Any bits that might get set below should be cleared here first. */
    REMOVE_FLAGS(EFF_FLAGS(ch), race_effects_mask, NUM_EFF_FLAGS);

    /* Reset effect flags for this race */
    SET_FLAGS(EFF_FLAGS(ch), Races[(int)GET_RACE(ch)].effect_flags, NUM_EFF_FLAGS);
}

/*
 * Returns a positive value for skills that this race has.
 *
 * Doesn't disqualify any skills! Only enables them.
 */

int racial_skill_proficiency(int skill, int race, int level) {
    int i;

    for (i = 0; Races[race].skills[i].skill > 0 && i < NUM_RACE_SKILLS; ++i)
        if (Races[race].skills[i].skill == skill) {
            return Races[race].skills[i].proficiency;
        }

    return 0;
}

/* convert_race does no checking.  It expects a valid race and ch.
 * This function changes a player's race and converts the skills/spells
 * accordingly, keeping the old values if they are better.
 * It also transfers quest spells. */
void convert_race(CharData *ch, int newrace) {
    int skill;
    sh_int old_skills[TOP_SKILL + 1];
    sh_int new_skills[TOP_SKILL + 1];

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
        if (new_skills[skill]) {
            /* keep the value of the old skill if you still have the skill */
            if (old_skills[skill] > new_skills[skill]) {
                SET_SKILL(ch, skill, old_skills[skill]);
            }
        }

        /* keep any quest spells you might have earned */
        if ((old_skills[skill]) && (skills[skill].quest)) {
            SET_SKILL(ch, skill, old_skills[skill]);
        }
    }
    check_regen_rates(ch);
}

void scale_attribs(CharData *ch) {
    if (VALID_RACE(ch)) {
        GET_AFFECTED_STR(ch) = (GET_VIEWED_STR(ch) * Races[(int)GET_RACE(ch)].attrib_scales[APPLY_STR - 1]) / 100;
        GET_AFFECTED_DEX(ch) = (GET_VIEWED_DEX(ch) * Races[(int)GET_RACE(ch)].attrib_scales[APPLY_DEX - 1]) / 100;
        GET_AFFECTED_INT(ch) = (GET_VIEWED_INT(ch) * Races[(int)GET_RACE(ch)].attrib_scales[APPLY_INT - 1]) / 100;
        GET_AFFECTED_WIS(ch) = (GET_VIEWED_WIS(ch) * Races[(int)GET_RACE(ch)].attrib_scales[APPLY_WIS - 1]) / 100;
        GET_AFFECTED_CON(ch) = (GET_VIEWED_CON(ch) * Races[(int)GET_RACE(ch)].attrib_scales[APPLY_CON - 1]) / 100;
        GET_AFFECTED_CHA(ch) = (GET_VIEWED_CHA(ch) * Races[(int)GET_RACE(ch)].attrib_scales[APPLY_CHA - 1]) / 100;
    } else {
        GET_AFFECTED_STR(ch) = GET_VIEWED_STR(ch) * 72 / 100;
        GET_AFFECTED_DEX(ch) = GET_VIEWED_DEX(ch) * 72 / 100;
        GET_AFFECTED_INT(ch) = GET_VIEWED_INT(ch) * 72 / 100;
        GET_AFFECTED_WIS(ch) = GET_VIEWED_WIS(ch) * 72 / 100;
        GET_AFFECTED_CON(ch) = GET_VIEWED_CON(ch) * 72 / 100;
        GET_AFFECTED_CHA(ch) = GET_VIEWED_CHA(ch) * 72 / 100;
    }
}