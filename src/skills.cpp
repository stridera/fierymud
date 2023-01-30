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

#include "skills.hpp"

#include "casting.hpp"
#include "chars.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "damage.hpp"
#include "db.hpp"
#include "events.hpp"
#include "fight.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "races.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

SkillDef skills[TOP_SKILL_DEFINE + 1];

const char *talent_types[5] = {
    "talent", "spell", "skill", "chant", "song",
};

const char *targets[NUM_TAR_FLAGS + 1] = {"IGNORE",    "CHAR_ROOM", "CHAR_WORLD", "FIGHT_SELF", "FIGHT_VICT",
                                          "SELF_ONLY", "NOT_SELF",  "OBJ_INV",    "OBJ_ROOM",   "OBJ_WORLD",
                                          "OBJ_EQUIP", "STRING",    "NIGHT_ONLY", "DAY_ONLY",   "OUTDOORS",
                                          "GROUND",    "CONTACT",   "DIRECT",     "\n"};

const char *routines[NUM_ROUTINE_TYPES + 1] = {
    "DAMAGE", "AFFECT", "UNAFFECT", "POINT",  "ALTER_OBJ", "GROUP",     "MASS",
    "AREA",   "SUMMON", "CREATION", "MANUAL", "ROOM",      "BULK_OBJS", "\n",
};

int skill_sort_info[TOP_SKILL + 1];

int talent_type(int skill_num) {
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

const char *skill_name(int num) {
    if (num <= 0 || num >= TOP_SKILL_DEFINE) {
        if (num == 0)
            return "UNUSED";
        else
            return "UNDEFINED";
    }

    return skills[num].name;
}

int find_talent_num(char *name, int should_restrict) {
    int index = 0, abbrevmatch = -1, ok;
    char *temp, *temp2;
    char first[256], first2[256], temp3[256];

    skip_spaces(&name);

    /* Loop through the skills to find a match. */
    while (++index <= TOP_SKILL_DEFINE) {

        if ((should_restrict == SPELL && !IS_SPELL(index)) || (should_restrict == SKILL && !IS_SKILL(index)) ||
            (should_restrict == SONG && !IS_SONG(index)) || (should_restrict == CHANT && !IS_CHANT(index)))
            continue;

        /* Exact match.  This is the skill we're looking for. */
        if (!strcasecmp(name, skills[index].name))
            return index;

        /*
         * If we found an abbreviated match, we don't want to return its
         * index immediately, in case we find a better match later.
         */
        if (is_abbrev(name, skills[index].name)) {
            if (abbrevmatch == -1 || strcasecmp(skills[index].name, skills[abbrevmatch].name) == -1) {
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

int find_skill_num(char *name) { return find_talent_num(name, SKILL); }

int find_spell_num(char *name) { return find_talent_num(name, SPELL); }

int find_chant_num(char *name) { return find_talent_num(name, CHANT); }

int find_song_num(char *name) { return find_talent_num(name, SONG); }

void improve_skill(CharData *ch, int skill) {
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
    if (random_number(0, 140) > GET_WIS(ch) + GET_INT(ch))
        return;

    /* These skills improve a bit faster than most. */
    if (skill == SKILL_FIRST_AID || skill == SKILL_BANDAGE || skill == SKILL_DOUSE || skill == SKILL_CHANT)
        percent += random_number(4, 7);
    else if (skill == SKILL_TAME || skill == SKILL_BASH || skill == SKILL_DISARM || skill == SKILL_SCRIBE ||
             skill == SKILL_SWITCH || skill == SKILL_PERFORM)
        percent += 2;
    else
        percent++;

    /* returns 1000 for most skills, but caps some others lower */
    if (percent >= maxpercent) {
        SET_SKILL(ch, skill, maxpercent);
        sprintf(skillbuf, "You feel about as skilled in %s as possible!&0\n", skills[skill].name);
    } else {
        SET_SKILL(ch, skill, percent);
        sprintf(skillbuf, "You feel your skill in %s improving.\n&0", skills[skill].name);
    }
    char_printf(ch, skillbuf);
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

void improve_skill_offensively(CharData *ch, CharData *victim, int skill) {
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
 * 3. Assigned to a mob by vnum: creatures[] in act.other.c (mainly for
 * shapechange).
 *
 * In addition, if a skill is for humanoids only, you will not be allowed
 * to have it even if class would allow it; but the creatures[] array
 * overrides this rule.
 */

void update_skills(CharData *ch) {
    int skill, i, spherecheck[NUM_SPHERE_SKILLS], prof;
    bool spherepresent;

    /* act.other.c: shapechange creatures */
    extern bool creature_allowed_skill(CharData * ch, int skill);

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

        if (skills[skill].min_level[(int)GET_CLASS(ch)] <= GET_LEVEL(ch) &&
            !(skills[skill].humanoid && !(IS_HUMANOID(ch) || creature_allowed_skill(ch, skill)))) {
            /* This skill is available because of your class, and you meet
             * the humanoid requirement, if any. */

            /* This is a talent that you do have, or could have. */
            if (skills[skill].quest == false) {
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

        } else if (!IS_SPHERE_SKILL(skill) && skills[skill].quest == false) {
            /* You don't meet the level requirement. Neither race nor questage
             * could give it to you, so you CAN'T have it. We take it away! */
            SET_SKILL(ch, skill, 0);
        }
    }

    /* Now set the sphere skills according to the spells we saw in the previous
     * loop. I've looped over the entire skill set again under the assumption that
     * the sphere-skills might NOT be contiguous in the future. */
    for (skill = 0; skill <= TOP_SKILL; ++skill) {
        if (IS_SPHERE_SKILL(skill)) {
            spherepresent = false;
            for (i = 0; i < NUM_SPHERE_SKILLS; i++) {
                if (spherecheck[i] == skill) {
                    spherepresent = true;
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
void dskill(int skill, char *name, int max_mana, int min_mana, int mana_change, int minpos, int ok_fighting,
            int targets, byte violent, bool humanoid, int routines, int mem_time, int cast_time, int damage_type,
            int sphere, int pages, int quest, char *wearoff) {
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
    skills[skill].pages = pages;
    skills[skill].quest = quest;
    skills[skill].wearoff = wearoff;

    skills[skill].mem_time = mem_time;
    skills[skill].cast_time = cast_time;
}

void clear_skill(int skill) {
    int i;

    skills[skill].name = "!UNUSED!";

    for (i = 0; i < NUM_CLASSES; i++)
        skills[skill].min_level[i] = LVL_IMPL + 1;
    skills[skill].lowest_level = LVL_IMPL + 1;

    skills[skill].mana_max = 0;
    skills[skill].mana_min = 0;
    skills[skill].mana_change = 0;
    skills[skill].minpos = 0;
    skills[skill].fighting_ok = false;
    skills[skill].targets = 0;
    skills[skill].violent = false;
    skills[skill].humanoid = false;
    skills[skill].routines = 0;
    skills[skill].damage_type = DAM_UNDEFINED;
    skills[skill].sphere = 0;
    skills[skill].pages = 0;
    skills[skill].quest = false;
    skills[skill].wearoff = nullptr;

    skills[skill].mem_time = 0;
    skills[skill].cast_time = 0;
}

#define skillo(skill, name, humanoid, targets)                                                                         \
    dskill(skill, name, 0, 0, 0, 0, true, targets, 0, humanoid, 0, 0, 0, 0, 0, 0, 0, nullptr);
#define chanto(chant, name, minpos, ok_fighting, targets, violent, routines, damage, quest, wearoff)                   \
    dskill(chant, name, 0, 0, 0, minpos, ok_fighting, targets, violent, false, routines, 0, 0, damage, 0, 0, quest,    \
           wearoff)
#define songo(song, name, minpos, ok_fighting, targets, violent, routines, damage, quest, wearoff)                     \
    dskill(song, name, 0, 0, 0, minpos, ok_fighting, targets, violent, false, routines, 0, 0, damage, 0, 0, quest,     \
           wearoff)
#define spello(spl, name, max_mana, min_mana, mana_change, minpos, ok_fighting, targets, violent, routines, mem_time,  \
               cast_time, damage_type, sphere, pages, quest, wearoff)                                                  \
    dskill(spl, name, max_mana, min_mana, mana_change, minpos, ok_fighting, targets, violent, false, routines,         \
           mem_time, cast_time, damage_type, sphere, pages, quest, wearoff)
#define effecto(eff, name, wearoff) dskill(eff, name, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, wearoff)

/* **** Initialization of skills **** */

void init_skills(void) {
    int i;

    for (i = 0; i <= TOP_SKILL_DEFINE; i++)
        clear_skill(i);

    skills[0].name = "!RESERVED!";

    /*
     * Arguments for spello calls:
     *
     * spellnum, name, maxmana, minmana, manachng, minpos, targets, violent?,
     * routines, mem_time, cast_time, damage_type, sphere, pages, quest, wearoff
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
     * (usually fighting or standing).
     *
     * targets :  A "list" of the valid targets
     * for the spell, joined with bitwise OR ('|').
     *
     * violent :  true or false, depending on if this is considered a violent
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
     * quest:  true if spell is a quest spell false if not.
     *
     * wearoff: The message seen when the spell wears off.  NULL if none.
     *
     * See the CircleMUD documentation for a more detailed description of these
     * fields.
     */

    spello(SPELL_ACID_BREATH, "acid breath", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_DIRECT, true, MAG_DAMAGE,
           C1, CAST_SPEED1, DAM_ACID, SKILL_SPHERE_EARTH, 5, false, nullptr);

    spello(SPELL_ACID_BURST, "acid burst", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT,
           true, MAG_DAMAGE, C1, CAST_SPEEDG, DAM_ACID, SKILL_SPHERE_GENERIC, 23, false, nullptr);

    spello(SPELL_ACID_FOG, "acid fog", 0, 0, 0, POS_STANDING, true, TAR_IGNORE | TAR_DIRECT, true, MAG_MANUAL, C1,
           CAST_SPEED5, DAM_ACID, SKILL_SPHERE_EARTH, 27, false, nullptr);

    spello(SPELL_ANCESTRAL_VENGEANCE, "ancestral vengeance", 40, 30, 2, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true, MAG_DAMAGE, C1, CAST_SPEEDE, DAM_ALIGN,
           SKILL_SPHERE_GENERIC, 21, false, nullptr);

    spello(SPELL_ANIMATE_DEAD, "animate dead", 75, 15, 3, POS_STANDING, false, TAR_OBJ_ROOM, false, MAG_SUMMON, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DEATH, 21, false, nullptr);

    spello(SPELL_ARMOR, "armor", 30, 15, 3, POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_AFFECT, C1, CAST_SPEED2,
           DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false, "You feel less protected.");

    spello(SPELL_ARMOR_OF_GAIA, "armor of gaia", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_MANUAL, C1, CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, false, nullptr);

    spello(SPELL_BALEFUL_POLYMORPH, "baleful polymorph", 90, 35, 3, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true, MAG_DAMAGE, C1, CAST_SPEEDE, DAM_UNDEFINED,
           SKILL_SPHERE_SUMMON, 25, false, nullptr);

    spello(SPELL_BANISH, "banish", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, true,
           MAG_MANUAL, C1, CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, true, nullptr);

    spello(SPELL_BARKSKIN, "barkskin", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_AFFECT, C1, CAST_SPEED2,
           DAM_UNDEFINED, SKILL_SPHERE_PROT, 17, false, "Your skin softens back to its original texture.");

    spello(SPELL_BIGBYS_CLENCHED_FIST, "bigbys clenched fist", 90, 35, 3, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true, MAG_DAMAGE, C1, CAST_SPEEDE, DAM_CRUSH,
           SKILL_SPHERE_GENERIC, 25, false, nullptr);

    spello(SPELL_BLESS, "bless", 35, 5, 3, POS_SITTING, false, TAR_CHAR_ROOM | TAR_OBJ_INV, false,
           MAG_AFFECT | MAG_ALTER_OBJ, C1, CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "You feel less righteous.");

    spello(SPELL_BLINDING_BEAUTY, "blinding beauty", 35, 25, 1, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C1,
           CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "You feel a cloak of blindness dissolve.");

    spello(SPELL_BLINDNESS, "blindness", 35, 25, 1, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF,
           true, MAG_AFFECT, C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "You feel a cloak of blindness dissolve.");

    spello(SPELL_BLUR, "blur", 90, 60, 3, POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECT, C1,
           CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 25, true,
           "The world solidifies as your vision unblurs, and you return to "
           "normal speed.");

    spello(SPELL_BONE_ARMOR, "bone armor", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_PROT, 12, false,
           "&3Your skin returns to normal.&0");

    spello(SPELL_BONE_CAGE, "bone cage", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_AFFECT,
           C1, CAST_SPEED2, DAM_SLASH, SKILL_SPHERE_SUMMON, 16, false, "The bones holding you down crumble to dust.");

    spello(SPELL_BURNING_HANDS, "burning hands", 30, 10, 3, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, true, MAG_DAMAGE, C1, CAST_SPEEDD, DAM_FIRE, SKILL_SPHERE_FIRE,
           5, false, nullptr);

    spello(SPELL_CALL_LIGHTNING, "call lightning", 40, 25, 3, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OUTDOORS, true, MAG_DAMAGE, C1, CAST_SPEED4, DAM_SHOCK,
           SKILL_SPHERE_AIR, 5, false, nullptr);

    spello(SPELL_CAUSE_CRITIC, "cause critical", 0, 0, 0, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, true, MAG_DAMAGE, C1, CAST_SPEED4, DAM_HEAL,
           SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_CAUSE_LIGHT, "cause light", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT,
           true, MAG_DAMAGE, C1, CAST_SPEED2, DAM_HEAL, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_CAUSE_SERIOUS, "cause serious", 0, 0, 0, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, true, MAG_DAMAGE, C1, CAST_SPEED3, DAM_HEAL,
           SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_CHAIN_LIGHTNING, "chain lightning", 0, 0, 0, POS_STANDING, true, TAR_IGNORE | TAR_DIRECT, true,
           MAG_AREA, C1, CAST_SPEED5, DAM_SHOCK, SKILL_SPHERE_AIR, 27, false, nullptr);

    spello(SPELL_CHARM, "charm person", 75, 50, 2, POS_STANDING, false, TAR_CHAR_ROOM | TAR_NOT_SELF, false, MAG_MANUAL,
           C1, CAST_SPEED5, DAM_MENTAL, SKILL_SPHERE_ENCHANT, 35, true, "You feel more self-confident.");

    spello(SPELL_CHILL_TOUCH, "chill touch", 30, 10, 3, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, true, MAG_DAMAGE | MAG_AFFECT, C1, CAST_SPEEDD, DAM_COLD,
           SKILL_SPHERE_WATER, 7, false, "You feel your strength return.");

    spello(SPELL_CIRCLE_OF_DEATH, "circle of death", 0, 0, 0, POS_STANDING, true, TAR_IGNORE | TAR_DIRECT, true,
           MAG_AREA, C1, CAST_SPEED5, DAM_MENTAL, SKILL_SPHERE_DEATH, 27, false, nullptr);

    spello(SPELL_CIRCLE_OF_FIRE, "circle of fire", 0, 0, 0, POS_STANDING, false, TAR_IGNORE, true, MAG_ROOM, C1,
           CAST_SPEED6, DAM_FIRE, SKILL_SPHERE_FIRE, 25, false,
           "&1&bThe &1&bfl&3am&1es&0 &1surrounding &1the area &9&bdie out&0.");

    spello(SPELL_CIRCLE_OF_LIGHT, "circle of light", 0, 0, 0, POS_SITTING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "The circle of light above you fades out.");

    spello(SPELL_CLONE, "clone", 80, 65, 5, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_SUMMON, C1,
           CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 35, false, nullptr);

    spello(SPELL_CLOUD_OF_DAGGERS, "cloud of daggers", 0, 0, 0, POS_STANDING, true, TAR_IGNORE | TAR_DIRECT, true,
           MAG_MANUAL, C1, CAST_SPEED5, DAM_SLASH, SKILL_SPHERE_GENERIC, 27, false, nullptr);

    spello(SPELL_COLDSHIELD, "coldshield", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED3, DAM_COLD, SKILL_SPHERE_WATER, 17, false,
           "The ice formation around your body melts.");

    spello(SPELL_COLOR_SPRAY, "color spray", 30, 15, 3, POS_STANDING, true, TAR_IGNORE | TAR_DIRECT, true, MAG_MANUAL,
           C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_GENERIC, 21, false, nullptr);

    spello(SPELL_CONCEALMENT, "concealment", 0, 0, 0, POS_SITTING, false, TAR_CHAR_ROOM, false, MAG_POINT, C1,
           CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 13, false, "You fade back into view.");

    spello(SPELL_CONE_OF_COLD, "cone of cold", 35, 15, 3, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true, MAG_DAMAGE, C1, CAST_SPEEDE, DAM_COLD, SKILL_SPHERE_WATER,
           19, false, nullptr);

    spello(SPELL_CONFUSION, "confusion", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_AFFECT,
           C1, CAST_SPEED4, DAM_MENTAL, SKILL_SPHERE_ENCHANT, 19, false, "You no longer feel so confused.");

    spello(SPELL_CONTROL_WEATHER, "control weather", 75, 25, 5, POS_STANDING, false, TAR_STRING, false, MAG_MANUAL, C1,
           CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, nullptr);

    spello(SPELL_CREATE_FOOD, "create food", 30, 5, 4, POS_SITTING, false, TAR_IGNORE, false, MAG_CREATION, C1,
           CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, false, nullptr);

    spello(SPELL_CREATE_SPRING, "create spring", 0, 0, 0, POS_STANDING, false, TAR_IGNORE, false, MAG_CREATION, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, false, nullptr);

    spello(SPELL_CREATE_WATER, "create water", 30, 5, 4, POS_SITTING, false, TAR_OBJ_INV | TAR_OBJ_EQUIP, false,
           MAG_MANUAL, C1, CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, false, nullptr);

    spello(SPELL_CREEPING_DOOM, "creeping doom", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, true, MAG_MANUAL, C1,
           CAST_SPEED7, DAM_CRUSH, SKILL_SPHERE_EARTH, 35, true, nullptr);

    spello(SPELL_CREMATE, "cremate", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C1, CAST_SPEED7, DAM_FIRE,
           SKILL_SPHERE_FIRE, 35, false, nullptr);

    spello(SPELL_CURE_BLIND, "cure blind", 30, 5, 2, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_UNAFFECT, C1,
           CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_CURE_CRITIC, "cure critic", 30, 10, 2, POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_POINT, C1,
           CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 15, false, nullptr);

    spello(SPELL_CURE_LIGHT, "cure light", 30, 10, 2, POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_POINT, C1,
           CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 9, false, nullptr);

    spello(SPELL_CURE_SERIOUS, "cure serious", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_POINT, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_CURSE, "curse", 80, 50, 2, POS_STANDING, true, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, true,
           MAG_AFFECT | MAG_ALTER_OBJ, C1, CAST_SPEED5, 0, SKILL_SPHERE_ENCHANT, 5, false, "You feel more optimistic.");

    spello(SPELL_DARK_FEAST, "dark feast", 0, 0, 0, POS_STANDING, false, TAR_OBJ_ROOM | TAR_CONTACT, false, MAG_MANUAL,
           C1, CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_GENERIC, 5, false, nullptr);

    spello(SPELL_DARKNESS, "darkness", 50, 25, 5, POS_SITTING, false, TAR_CHAR_ROOM | TAR_OBJ_INV, false, MAG_MANUAL,
           C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "The magical darkness lifts.");

    spello(SPELL_DARK_PRESENCE, "dark presence", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_OBJ_INV, false,
           MAG_AFFECT | MAG_ALTER_OBJ, C1, CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "You feel the dark presence leave you.");

    spello(SPELL_DECAY, "decay", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_DAMAGE, C1,
           CAST_SPEEDD, DAM_HEAL, SKILL_SPHERE_DEATH, 5, false, nullptr);

    spello(SPELL_DEGENERATION, "degeneration", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, false,
           MAG_MANUAL, C1, CAST_SPEED4, DAM_HEAL, SKILL_SPHERE_DEATH, 12, true, nullptr);

    spello(SPELL_DEMONIC_ASPECT, "demonic aspect", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "The demon within you fades away.");

    spello(SPELL_DEMONIC_MUTATION, "demonic mutation", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY,
           false, MAG_AFFECT, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "You mutate back to your original form.");

    spello(SPELL_DEMONSKIN, "demonskin", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_AFFECT, C1, CAST_SPEED1,
           DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false, "Your skin reverts back to normal.");

    spello(SPELL_DESTROY_UNDEAD, "destroy undead", 0, 0, 0, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true, MAG_DAMAGE, C1, CAST_SPEED3, DAM_ALIGN,
           SKILL_SPHERE_DEATH, 5, false, nullptr);

    spello(SPELL_DETECT_ALIGN, "detect alignment", 20, 10, 2, POS_SITTING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 9, false, "You feel less aware.");

    spello(SPELL_DETECT_INVIS, "detect invisibility", 20, 10, 2, POS_SITTING, false, TAR_CHAR_ROOM, false, MAG_AFFECT,
           C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 19, false, "Your eyes stop tingling.");

    spello(SPELL_DETECT_MAGIC, "detect magic", 20, 10, 2, POS_SITTING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 5, false, "The detect magic wears off.");

    spello(SPELL_DETECT_POISON, "detect poison", 15, 5, 1, POS_SITTING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 13, false, "You feel slightly less aware.");

    spello(SPELL_DETONATION, "detonation", 0, 0, 0, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_GROUND | TAR_OUTDOORS, true, MAG_DAMAGE, C1, CAST_SPEEDE, DAM_CRUSH,
           SKILL_SPHERE_EARTH, 10, false, nullptr);

    spello(SPELL_DIMENSION_DOOR, "dimension door", 75, 45, 3, POS_STANDING, false, TAR_CHAR_WORLD | TAR_NOT_SELF, false,
           MAG_MANUAL, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 15, false, nullptr);

    spello(SPELL_DISCORPORATE, "discorporate", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT,
           true, MAG_DAMAGE, C1, CAST_SPEEDE, DAM_DISCORPORATE, SKILL_SPHERE_GENERIC, 14, false, nullptr);

    spello(SPELL_DISPEL_MAGIC, "dispel magic", 0, 0, 0, POS_SITTING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_ROOM | TAR_OBJ_INV, true, MAG_MANUAL, C1, CAST_SPEED4, DAM_DISPEL,
           SKILL_SPHERE_GENERIC, 15, false, nullptr);

    spello(SPELL_DISPEL_EVIL, "dispel evil", 40, 25, 3, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
           MAG_DAMAGE, C1, CAST_SPEED3, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, false, nullptr);

    spello(SPELL_DISPEL_GOOD, "dispel good", 40, 25, 3, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
           MAG_DAMAGE, C1, CAST_SPEED5, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, false, nullptr);

    spello(SPELL_DISEASE, "disease", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true,
           MAG_AFFECT, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "You are cured of your disease!");

    spello(SPELL_DISINTEGRATE, "disintegrate", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT,
           true, MAG_DAMAGE | MAG_MANUAL, C1, CAST_SPEEDF, DAM_ACID, SKILL_SPHERE_GENERIC, 27, false, nullptr);

    spello(SPELL_DIVINE_BOLT, "divine bolt", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT,
           true, MAG_DAMAGE, C1, CAST_SPEED3, DAM_ALIGN, SKILL_SPHERE_GENERIC, 5, false, nullptr);

    spello(SPELL_DIVINE_ESSENCE, "divine essence", 0, 0, 0, POS_STANDING, false, TAR_IGNORE, false, MAG_GROUP, C1,
           CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, nullptr);

    spello(SPELL_DIVINE_RAY, "divine ray", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT,
           true, MAG_DAMAGE, C1, CAST_SPEED5, DAM_ALIGN, SKILL_SPHERE_GENERIC, 5, false, nullptr);

    spello(SPELL_DOOM, "doom", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C1, CAST_SPEED6, DAM_CRUSH,
           SKILL_SPHERE_EARTH, 35, false, nullptr);

    spello(SPELL_DRAGONS_HEALTH, "dragons health", 50, 30, 5, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 21, true, "Your health returns to normal.");

    spello(SPELL_EARTH_BLESSING, "earth blessing", 35, 5, 3, POS_SITTING, false, TAR_CHAR_ROOM | TAR_OBJ_INV, false,
           MAG_AFFECT | MAG_ALTER_OBJ, C1, CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "You feel less righteous.");

    spello(SPELL_EARTHQUAKE, "earthquake", 40, 25, 3, POS_STANDING, true, TAR_IGNORE | TAR_OUTDOORS, true, MAG_AREA, C1,
           CAST_SPEED5, DAM_CRUSH, SKILL_SPHERE_EARTH, 5, false, nullptr);

    spello(SPELL_ELEMENTAL_WARDING, "elemental warding", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT,
           C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false, "You feel less safe from the elements.");

    spello(SPELL_ENCHANT_WEAPON, "enchant weapon", 150, 100, 10, POS_STANDING, false, TAR_OBJ_INV | TAR_OBJ_EQUIP,
           false, MAG_MANUAL, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 23, false, nullptr);

    spello(SPELL_ENDURANCE, "endurance", 50, 30, 5, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 7, false, "Your endurance returns to normal.");

    spello(SPELL_ENERGY_DRAIN, "energy drain", 40, 25, 1, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
           MAG_MANUAL, C1, CAST_SPEED5, DAM_HEAL, SKILL_SPHERE_DEATH, 19, false, nullptr);

    spello(SPELL_ENLARGE, "enlarge", 35, 5, 3, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT | MAG_UNAFFECT, C1,
           CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 29, false, "You return to your normal size.&0");

    spello(SPELL_ENLIGHTENMENT, "enlightenment", 0, 0, 0, POS_SITTING, false, TAR_CHAR_ROOM | TAR_NOT_SELF, false,
           MAG_MANUAL, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 5, false, nullptr);

    spello(SPELL_ENTANGLE, "entangle", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OUTDOORS,
           false, MAG_AFFECT, C1, CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "You break free of the vines.");

    spello(SPELL_EXORCISM, "exorcism", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_DAMAGE,
           C1, 18, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, false, nullptr);

    spello(SPELL_EXTINGUISH, "extinguish", 0, 0, 0, POS_SITTING, true, TAR_CHAR_ROOM, false, MAG_UNAFFECT, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_WATER, 11, false, nullptr);

    spello(SPELL_FAMILIARITY, "familiarity", 0, 0, 0, POS_SITTING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 10, false,
           "Your familiar disguise melts away.");

    spello(SPELL_FARSEE, "farsee", 25, 10, 1, POS_SITTING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1, CAST_SPEED2,
           DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 9, false, "Your pupils dilate as your vision returns to normal.");

    spello(SPELL_FEAR, "fear", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_MANUAL, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "Your courage returns to you.");

    spello(SPELL_FIREBALL, "fireball", 40, 30, 2, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true,
           MAG_DAMAGE, C1, CAST_SPEEDE, DAM_FIRE, SKILL_SPHERE_FIRE, 21, false, nullptr);

    spello(SPELL_FIRE_BREATH, "fire breath", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM, true, MAG_DAMAGE, C1,
           CAST_SPEED1, DAM_FIRE, SKILL_SPHERE_FIRE, 5, false, nullptr);

    spello(SPELL_FIRE_DARTS, "fire darts", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT,
           true, MAG_MANUAL, C1, CAST_SPEEDD, DAM_FIRE, SKILL_SPHERE_FIRE, 9, false, nullptr);

    spello(SPELL_FIRESHIELD, "fireshield", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED3, DAM_FIRE, SKILL_SPHERE_FIRE, 17, false,
           "The flames around your body dissipate.");

    spello(SPELL_FIRESTORM, "firestorm", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C1, CAST_SPEED6,
           DAM_FIRE, SKILL_SPHERE_FIRE, 25, false, nullptr);

    spello(SPELL_FLAME_BLADE, "flame blade", 0, 0, 0, POS_STANDING, false, TAR_IGNORE, false, MAG_MANUAL, C1,
           CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 19, false, nullptr);

    spello(SPELL_FLAMESTRIKE, "flamestrike", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT,
           true, MAG_DAMAGE, C1, CAST_SPEED3, DAM_FIRE, SKILL_SPHERE_FIRE, 5, false, nullptr);

    spello(SPELL_FLOOD, "flood", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, true, MAG_MANUAL, C1, 18, DAM_WATER,
           SKILL_SPHERE_WATER, 35, true, nullptr);

    spello(SPELL_FLY, "fly", 50, 5, 3, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1, CAST_SPEED3,
           DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 17, false, "You feel the weight of your body return.");

    spello(SPELL_FRACTURE, "fracture", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, false, MAG_MANUAL, C1, CAST_SPEEDE,
           DAM_SLASH, SKILL_SPHERE_GENERIC, 17, false, nullptr);

    spello(SPELL_FREEZE, "freeze", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_DAMAGE, C1,
           CAST_SPEEDE, DAM_COLD, SKILL_SPHERE_WATER, 25, false, nullptr);

    spello(SPELL_FREEZING_WIND, "freezing wind", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C1,
           CAST_SPEED4, DAM_COLD, SKILL_SPHERE_AIR, 21, false, nullptr);

    spello(SPELL_FROST_BREATH, "frost breath", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_DIRECT, true,
           MAG_DAMAGE, C1, CAST_SPEED1, DAM_COLD, SKILL_SPHERE_WATER, 5, false, nullptr);

    spello(SPELL_FULL_HARM, "full harm", 75, 45, 3, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT,
           true, MAG_DAMAGE, C1, CAST_SPEED6, DAM_HEAL, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_FULL_HEAL, "full heal", 75, 50, 3, POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_POINT | MAG_UNAFFECT,
           C1, CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_GAIAS_CLOAK, "cloak of gaia", 0, 0, 0, POS_STANDING, false,
           TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_OUTDOORS, false, MAG_AFFECT, C1, CAST_SPEED6, DAM_UNDEFINED,
           SKILL_SPHERE_PROT, 5, false, "Your shroud of nature dissolves.");

    spello(SPELL_GAS_BREATH, "gas breath", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM, true, MAG_DAMAGE | MAG_AFFECT,
           C1, CAST_SPEED1, DAM_POISON, SKILL_SPHERE_AIR, 5, false, nullptr);

    spello(SPELL_GLORY, "glory", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECT, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 17, false, "Your visage becomes plain once again.");

    spello(SPELL_GREATER_ENDURANCE, "greater endurance", 50, 30, 5, POS_STANDING, false, TAR_CHAR_ROOM, false,
           MAG_AFFECT, C1, CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 11, false,
           "Your endurance returns to normal.");

    spello(SPELL_GREATER_VITALITY, "greater vitality", 50, 30, 5, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT,
           C1, CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 11, false, "Your magical vitality drains away.");

    spello(SPELL_GROUP_ARMOR, "group armor", 50, 30, 2, POS_STANDING, false, TAR_IGNORE, false, MAG_GROUP, C1,
           CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_PROT, 9, true, nullptr);

    spello(SPELL_GROUP_HEAL, "group heal", 80, 60, 5, POS_STANDING, true, TAR_IGNORE, false, MAG_GROUP, C1, CAST_SPEED6,
           DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, true, nullptr);

    spello(SPELL_GROUP_RECALL, "group recall", 50, 30, 2, POS_STANDING, true, TAR_IGNORE, false, MAG_GROUP, C1,
           CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, true, nullptr);

    spello(SPELL_HARM, "harm", 45, 15, 3, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, true,
           MAG_DAMAGE, C1, CAST_SPEED5, DAM_HEAL, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_HARNESS, "harness", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECT, C1,
           2, DAM_UNDEFINED, SKILL_SPHERE_GENERIC, 21, false, "&4The harnessed power in your body fades.&0");

    spello(SPELL_HASTE, "haste", 50, 25, 3, POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_AFFECT, C1, CAST_SPEED3,
           DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 19, false, "Your pulse returns to normal.");

    spello(SPELL_HEAL, "heal", 60, 40, 3, POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_POINT | MAG_UNAFFECT, C1,
           CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_HEAVENS_GATE, "heavens gate", 0, 0, 0, POS_STANDING, false, TAR_CHAR_WORLD, false, MAG_MANUAL, C1, 16,
           DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, true, nullptr);

    spello(SPELL_HELL_BOLT, "hell bolt", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true,
           MAG_DAMAGE, C1, CAST_SPEED2, DAM_FIRE, SKILL_SPHERE_FIRE, 5, false, nullptr);

    spello(SPELL_HELLFIRE_BRIMSTONE, "hellfire and brimstone", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, false, MAG_AREA,
           C1, 16, DAM_FIRE, SKILL_SPHERE_FIRE, 5, true, nullptr);

    spello(SPELL_HELLS_GATE, "hell gate", 0, 0, 0, POS_STANDING, false, TAR_CHAR_WORLD, false, MAG_MANUAL, C1, 18,
           DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, true, nullptr);

    spello(SPELL_HOLY_WORD, "holy word", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C1, 1, DAM_ALIGN,
           SKILL_SPHERE_DEATH, 5, false, nullptr);

    spello(SPELL_HYSTERIA, "hysteria", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, true, MAG_MANUAL, C1, CAST_SPEED4,
           DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 16, false, "Your courage returns to you.");

    spello(SPELL_ICE_ARMOR, "ice armor", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECT,
           C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_PROT, 13, false,
           "Your iced encasing melts away, leaving you vulnerable again.");

    spello(SPELL_ICEBALL, "iceball", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true,
           MAG_DAMAGE, C1, CAST_SPEEDF, DAM_COLD, SKILL_SPHERE_WATER, 29, false, nullptr);

    spello(SPELL_ICE_DAGGER, "ice dagger", 0, 0, 0, POS_STANDING, false, TAR_IGNORE, false, MAG_MANUAL, C1, CAST_SPEED3,
           DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 17, false, nullptr);

    spello(SPELL_ICE_DARTS, "ice darts", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true,
           MAG_MANUAL, C1, CAST_SPEEDD, DAM_COLD, SKILL_SPHERE_WATER, 7, false, nullptr);

    spello(SPELL_ICE_SHARDS, "ice shards", 100, 50, 3, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C1, 16,
           DAM_SLASH, SKILL_SPHERE_WATER, 31, true, nullptr);

    spello(SPELL_ICE_STORM, "ice storm", 100, 50, 3, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C1, CAST_SPEED5,
           DAM_COLD, SKILL_SPHERE_WATER, 23, false, nullptr);

    spello(SPELL_ILLUMINATION, "illumination", 50, 25, 5, POS_SITTING, false, TAR_CHAR_ROOM | TAR_OBJ_INV, false,
           MAG_MANUAL, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "The magical light fades away.&0");

    spello(SPELL_ILLUSORY_WALL, "illusory wall", 0, 0, 0, POS_STANDING, false, TAR_STRING, false, MAG_MANUAL, C1, 18,
           DAM_UNDEFINED, SKILL_SPHERE_GENERIC, 27, true, "The wall dissolves into tiny motes of light...");

    spello(SPELL_IMMOLATE, "immolate", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true,
           MAG_MANUAL, C1, CAST_SPEEDF, DAM_FIRE, SKILL_SPHERE_FIRE, 25, false, nullptr);

    spello(SPELL_INCENDIARY_NEBULA, "incendiary nebula", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C1,
           15, DAM_FIRE, SKILL_SPHERE_FIRE, 35, false, nullptr);

    spello(SPELL_IDENTIFY, "identify", 0, 0, 0, POS_SITTING, false,
           TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_CONTACT, false, MAG_MANUAL, C1, CAST_SPEED3, DAM_UNDEFINED,
           SKILL_SPHERE_DIVIN, 5, false, nullptr);

    spello(SPELL_INFRAVISION, "infravision", 25, 10, 1, POS_SITTING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 9, false, "Your night vision seems to fade.");

    /* innate charisma */
    spello(SPELL_INN_ASCEN, "innate ascen", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, 0, 0, 7, false, "You feel less splendid.");

    /* innate intelligence */
    spello(SPELL_INN_BRILL, "innate brill", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, 0, 0, 7, false, "You feel less intelligent.");

    /* innate strength */
    spello(SPELL_INN_CHAZ, "innate chaz", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, 0, 0, 7, false, "You feel weaker.");

    /* innate dexterity */
    spello(SPELL_INN_SYLL, "innate syll", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, 0, 0, 7, false, "You feel clumsier.");

    /* innate wisdom */
    spello(SPELL_INN_TASS, "innate tass", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, 0, 0, 7, false, "You feel less wise.");

    /* innate constitution */
    spello(SPELL_INN_TREN, "innate tren", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, 0, 0, 7, false, "You feel less healthy.");

    spello(SPELL_INSANITY, "insanity", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_AFFECT,
           C1, CAST_SPEED5, DAM_MENTAL, SKILL_SPHERE_ENCHANT, 5, false, "Your mind returns to reality.");

    spello(SPELL_INVIGORATE, "invigorate", 0, 0, 0, POS_STANDING, false, TAR_IGNORE, false, MAG_GROUP, C1, CAST_SPEED7,
           DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_INVISIBLE, "invisibility", 35, 25, 1, POS_STANDING, false, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM,
           false, MAG_AFFECT | MAG_ALTER_OBJ, C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 17, false,
           "You fade back into view.");

    spello(SPELL_IRON_MAIDEN, "iron maiden", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
           MAG_DAMAGE, C1, CAST_SPEEDE, DAM_PIERCE, SKILL_SPHERE_DEATH, 14, false, nullptr);

    spello(SPELL_ISOLATION, "isolation", 0, 0, 0, POS_STANDING, false, TAR_IGNORE, false, MAG_MANUAL, C1, 10,
           DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 8, false, "It is as if a veil has lifted from the surrounding area.");

    spello(SPELL_LESSER_ENDURANCE, "lesser endurance", 50, 30, 5, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT,
           C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "Your endurance returns to normal.");

    spello(SPELL_LESSER_EXORCISM, "lesser exorcism", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
           MAG_DAMAGE, C1, CAST_SPEED5, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, false, nullptr);

    spello(SPELL_FEATHER_FALL, "feather fall", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1, CAST_SPEED3,
           DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 13, false, "You float back to the ground.");

    spello(SPELL_LIGHTNING_BOLT, "lightning bolt", 30, 15, 1, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true, MAG_DAMAGE, C1, CAST_SPEEDD, DAM_SHOCK, SKILL_SPHERE_AIR,
           17, false, nullptr);

    spello(SPELL_LIGHTNING_BREATH, "lightning breath", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM, true,
           MAG_DAMAGE | MAG_MANUAL, C1, CAST_SPEED1, DAM_SHOCK, SKILL_SPHERE_AIR, 5, false, nullptr);

    spello(SPELL_LOCATE_OBJECT, "locate object", 25, 20, 1, POS_SITTING, false, TAR_STRING, false, MAG_MANUAL, C1,
           CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 12, false, nullptr);

    spello(SPELL_MAGIC_MISSILE, "magic missile", 0, 0, 0, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true, MAG_MANUAL, C1, CAST_SPEEDD, DAM_PIERCE,
           SKILL_SPHERE_GENERIC, 5, false, nullptr);

    spello(SPELL_MINOR_PARALYSIS, "minor paralysis", 0, 0, 0, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, false, MAG_AFFECT, C1, CAST_SPEED3, DAM_UNDEFINED,
           SKILL_SPHERE_ENCHANT, 21, false, "Your muscles regain feeling.");

    spello(SPELL_NATURES_EMBRACE, "natures embrace", 0, 0, 0, POS_STANDING, false,
           TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_OUTDOORS, false, MAG_AFFECT | MAG_POINT, C1, CAST_SPEED7, DAM_UNDEFINED,
           SKILL_SPHERE_ENCHANT, 5, false, "Nature releases you from her embrace.");

    spello(SPELL_NIGHTMARE, "nightmare", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true,
           MAG_DAMAGE, C1, CAST_SPEEDD, DAM_MENTAL, SKILL_SPHERE_DEATH, 12, false, nullptr);

    spello(SPELL_MAGIC_TORCH, "magic torch", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 7, false, "Your magic torch peters out.");

    spello(SPELL_MAJOR_GLOBE, "major globe", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_PROT, 35, true, "The globe of force surrounding you dissipates.");

    spello(SPELL_MAJOR_PARALYSIS, "major paralysis", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, true, MAG_MANUAL,
           C6, CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 35, true, "You can move again.");

    spello(SPELL_MASS_INVIS, "mass invisibility", 0, 0, 0, POS_STANDING, false, TAR_IGNORE, false,
           MAG_BULK_OBJS | MAG_MASS, C1, CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 23, false, nullptr);

    spello(SPELL_MELT, "melt", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_DAMAGE, C1,
           CAST_SPEEDE, DAM_FIRE, SKILL_SPHERE_FIRE, 29, false, nullptr);

    spello(SPELL_MESMERIZE, "mesmerize", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, true, MAG_AFFECT, C1, CAST_SPEED3,
           DAM_MENTAL, SKILL_SPHERE_ENCHANT, 8, false, "You regain your senses.");

    spello(SPELL_METEORSWARM, "meteorswarm", 100, 50, 3, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C9,
           CAST_SPEED7, DAM_CRUSH, SKILL_SPHERE_EARTH, 37, true, nullptr);

    spello(SPELL_MINOR_CREATION, "minor creation", 0, 0, 0, POS_SITTING, false, TAR_STRING, false, MAG_MANUAL, C1,
           CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, false, nullptr);

    spello(SPELL_MINOR_GLOBE, "minor globe", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_PROT, 23, false, "The globe around your body fades out.");

    spello(SPELL_MIRAGE, "mirage", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECT, C1,
           CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_FIRE, 17, false,
           "You become more visible as the heat around your body dies out.");

    spello(SPELL_MISDIRECTION, "misdirection", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 12, false,
           "You no longer feel like you're going every which way at once.");

    spello(SPELL_MOONBEAM, "moonbeam", 0, 0, 0, POS_STANDING, true, TAR_IGNORE | TAR_OUTDOORS | TAR_NIGHT_ONLY, true,
           MAG_MANUAL, C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_WATER, 10, false, nullptr);

    spello(SPELL_MOONWELL, "moonwell", 50, 50, 0, POS_STANDING, false, TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_MANUAL,
           C1, 18, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, true, nullptr);

    spello(SPELL_NATURES_GUIDANCE, "natures guidance", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY,
           false, MAG_AFFECT, C1, CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 8, false,
           "You suddenly feel a little unguided.");

    spello(SPELL_NEGATE_COLD, "negate cold", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_PROT, 29, false,
           "You feel vulnerable to the cold again.");

    spello(SPELL_NEGATE_HEAT, "negate heat", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_PROT, 29, false,
           "Your immunity to heat has passed.");

    spello(SPELL_NIGHT_VISION, "night vision", 0, 0, 0, POS_SITTING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "Your night vision fades out.");

    spello(SPELL_NOURISHMENT, "nourishment", 0, 0, 0, POS_SITTING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_POINT, C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_PHANTASM, "phantasm", 0, 0, 0, POS_STANDING, false, TAR_IGNORE, false, MAG_SUMMON, C1, CAST_SPEED4,
           DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 12, false, nullptr);

    spello(SPELL_PHOSPHORIC_EMBERS, "phosphoric embers", 0, 0, 0, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true, MAG_MANUAL, C1, CAST_SPEEDE, DAM_FIRE, SKILL_SPHERE_FIRE,
           18, false, nullptr);

    spello(SPELL_PLANE_SHIFT, "plane shift", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_MANUAL, C1, CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, true, nullptr);

    spello(SPELL_POISON, "poison", 50, 20, 3, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV | TAR_FIGHT_VICT, true, MAG_AFFECT | MAG_ALTER_OBJ, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "You feel less sick.");

    spello(SPELL_POSITIVE_FIELD, "positive field", 0, 0, 0, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_GROUND, true, MAG_DAMAGE, C1, CAST_SPEEDF, DAM_SHOCK,
           SKILL_SPHERE_EARTH, 18, false, nullptr);

    spello(SPELL_PRAYER, "prayer", 0, 0, 0, POS_SITTING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECT, C1,
           CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "Your holy prayer fades.");

    spello(SPELL_PRESERVE, "preserve", 0, 0, 0, POS_SITTING, false, TAR_OBJ_ROOM, false, MAG_MANUAL, C1, CAST_SPEED3,
           DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 11, false, nullptr);

    spello(SPELL_PROT_FROM_EVIL, "protection from evil", 40, 10, 3, POS_STANDING, false, TAR_CHAR_ROOM, false,
           MAG_AFFECT, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false, "You feel less protected.");

    spello(SPELL_PROT_FROM_GOOD, "protection from good", 40, 10, 3, POS_STANDING, false, TAR_CHAR_ROOM, false,
           MAG_AFFECT, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false, "You feel less protected.");

    spello(SPELL_PYRE, "pyre", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, false, MAG_MANUAL, C1, CAST_SPEEDD, DAM_FIRE,
           SKILL_SPHERE_FIRE, 15, false, "The flames enveloping you die down.");

    spello(SPELL_RAIN, "rain", 0, 0, 0, POS_STANDING, false, TAR_IGNORE, false, MAG_MANUAL, C1, CAST_SPEED6,
           DAM_UNDEFINED, SKILL_SPHERE_WATER, 23, false, nullptr);

    spello(SPELL_RAY_OF_ENFEEB, "ray of enfeeblement", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
           true, MAG_AFFECT, C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 21, false,
           "Your strength returns to you.");

    spello(SPELL_REBUKE_UNDEAD, "rebuke undead", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
           MAG_AFFECT, C1, CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 8, false, nullptr);

    spello(SPELL_RECALL, "recall", 20, 10, 2, POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_MANUAL, C1,
           1, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, false, nullptr);

    spello(SPELL_REDUCE, "reduce", 35, 5, 3, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT | MAG_UNAFFECT, C1,
           CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 29, false, "You return to your normal size.&0");

    spello(SPELL_RELOCATE, "relocate", 0, 0, 0, POS_STANDING, false, TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_MANUAL,
           C14, 20, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 35, true, nullptr);

    spello(SPELL_REMOVE_CURSE, "remove curse", 45, 25, 5, POS_STANDING, false,
           TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, false, MAG_MANUAL, C1, CAST_SPEED4, DAM_UNDEFINED,
           SKILL_SPHERE_ENCHANT, 5, false, nullptr);

    spello(SPELL_REMOVE_PARALYSIS, "remove paralysis", 45, 25, 5, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_MANUAL,
           C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, nullptr);

    spello(SPELL_REMOVE_POISON, "remove poison", 40, 8, 4, POS_SITTING, false,
           TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, false, MAG_UNAFFECT | MAG_ALTER_OBJ, C1, CAST_SPEED3,
           DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_RESURRECT, "resurrect", 75, 50, 3, POS_STANDING, false, TAR_CHAR_WORLD | TAR_NOT_SELF, false,
           MAG_MANUAL, C1, 26, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, true, nullptr);

    spello(SPELL_REVEAL_HIDDEN, "reveal hidden", 20, 10, 2, POS_SITTING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_MANUAL, C1, CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 19, false, nullptr);

    spello(SPELL_SANCTUARY, "sanctuary", 110, 85, 5, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED1, DAM_UNDEFINED, SKILL_SPHERE_PROT, 35, false, "The white aura around your body fades.");

    spello(SPELL_SANE_MIND, "sane mind", 0, 0, 0, POS_SITTING, false, TAR_CHAR_ROOM, false, MAG_UNAFFECT, C1,
           CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_SENSE_LIFE, "sense life", 20, 10, 2, POS_SITTING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 19, false,
           "You feel less aware of your surroundings.");

    spello(SPELL_SEVERANCE, "severance", 100, 50, 3, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C9, CAST_SPEED7,
           DAM_DISCORPORATE, SKILL_SPHERE_GENERIC, 37, false, nullptr);

    spello(SPELL_SHIFT_CORPSE, "shift corpse", 25, 20, 1, POS_STANDING, false, TAR_STRING, false, MAG_MANUAL, C1,
           CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 17, true, nullptr);

    spello(SPELL_SHOCKING_GRASP, "shocking grasp", 30, 15, 3, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, true, MAG_DAMAGE, C1, CAST_SPEEDD, DAM_SHOCK, SKILL_SPHERE_AIR,
           9, false, nullptr);

    spello(SPELL_SILENCE, "silence", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, true,
           MAG_AFFECT, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "You can speak again.");

    spello(SPELL_SIMULACRUM, "simulacrum", 0, 0, 0, POS_STANDING, false, TAR_CHAR_WORLD, false, MAG_SUMMON, C1,
           CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 15, false, nullptr);

    spello(SPELL_SLEEP, "sleep", 40, 25, 5, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1, CAST_SPEED4,
           DAM_MENTAL, SKILL_SPHERE_ENCHANT, 19, false, "You feel less tired.");

    spello(SPELL_SMOKE, "smoke", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true,
           MAG_AFFECT, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_FIRE, 11, false,
           "As the smoke clears, your vision returns.");

    spello(SPELL_SOUL_REAVER, "soul reaver", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C9, CAST_SPEED7,
           DAM_MENTAL, SKILL_SPHERE_DEATH, 30, false, nullptr);

    spello(SPELL_SOULSHIELD, "soulshield", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false,
           "The aura guarding your body fades away.");

    spello(SPELL_SOUL_TAP, "soul tap", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, true,
           MAG_MANUAL, C1, CAST_SPEEDF, DAM_HEAL, SKILL_SPHERE_DEATH, 22, false, nullptr);

    spello(SPELL_SPEAK_IN_TONGUES, "speak in tongues", 0, 0, 0, POS_SITTING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY,
           false, MAG_AFFECT, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 5, false,
           "Your vocabulary diminishes drastically.");

    spello(SPELL_SPINECHILLER, "spinechiller", 0, 0, 0, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CONTACT, true, MAG_AFFECT, C1, CAST_SPEEDE, DAM_UNDEFINED,
           SKILL_SPHERE_DEATH, 10, false, "The tingling in your spine subsides.");

    spello(SPELL_SPIRIT_ARROWS, "spirit arrows", 0, 0, 0, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true, MAG_MANUAL, C1, CAST_SPEEDD, DAM_ALIGN,
           SKILL_SPHERE_GENERIC, 5, false, nullptr);

    spello(SPELL_SPIRIT_RAY, "spirit ray", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT,
           true, MAG_DAMAGE | MAG_AFFECT, C1, CAST_SPEED6, DAM_ALIGN, SKILL_SPHERE_DEATH, 5, false, nullptr);

    spello(SPELL_STATUE, "statue", 0, 0, 0, POS_SITTING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECT, C1,
           CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 10, false, "Your statuesque disguise melts away.");

    spello(SPELL_STONE_SKIN, "stone skin", 50, 25, 3, POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_PROT, 19, false, "&3&dYour skin softens and returns to normal.&0");

    spello(SPELL_ENHANCE_ABILITY, "enhance ability", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT,
           C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "You feel less enhanced.");

    spello(SPELL_STYGIAN_ERUPTION, "stygian eruption", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
           true, MAG_DAMAGE, C1, CAST_SPEED4, DAM_FIRE, SKILL_SPHERE_FIRE, 5, false, nullptr);

    spello(SPELL_SUMMON, "summon", 75, 50, 3, POS_STANDING, false, TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_MANUAL, C1,
           CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, false, nullptr);

    spello(SPELL_SUMMON_CORPSE, "summon corpse", 25, 20, 1, POS_STANDING, false, TAR_STRING, false, MAG_MANUAL, C1,
           CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 17, false, nullptr);

    spello(SPELL_SUMMON_DEMON, "summon demon", 100, 75, 3, POS_STANDING, false, TAR_IGNORE, false, MAG_SUMMON, C1,
           CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 31, false, nullptr);

    spello(SPELL_SUMMON_DRACOLICH, "summon dracolich", 100, 75, 3, POS_STANDING, false, TAR_OBJ_ROOM, false, MAG_SUMMON,
           C1, 20, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 33, true, nullptr);

    spello(SPELL_SUMMON_ELEMENTAL, "summon elemental", 75, 15, 3, POS_STANDING, false, TAR_IGNORE, false, MAG_SUMMON,
           C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 19, false, nullptr);

    spello(SPELL_SUMMON_GREATER_DEMON, "summon greater demon", 130, 75, 3, POS_STANDING, false, TAR_IGNORE, false,
           MAG_SUMMON, C1, CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 35, false, nullptr);

    spello(SPELL_SUNRAY, "sunray", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true,
           MAG_DAMAGE | MAG_AFFECT, C1, CAST_SPEED6, DAM_FIRE, SKILL_SPHERE_FIRE, 5, false,
           "Your vision has returned.");

    spello(SPELL_SUPERNOVA, "supernova", 100, 50, 3, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C1, 16, DAM_FIRE,
           SKILL_SPHERE_FIRE, 31, true, nullptr);

    spello(SPELL_TELEPORT, "teleport", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_MANUAL,
           C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 11, false, nullptr);

    spello(SPELL_UNHOLY_WORD, "unholy word", 0, 0, 0, POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, C1, 1, DAM_ALIGN,
           SKILL_SPHERE_DEATH, 5, false, nullptr);

    spello(SPELL_URBAN_RENEWAL, "urban renewal", 0, 0, 0, POS_SITTING, false, TAR_IGNORE | TAR_OUTDOORS, false,
           MAG_ROOM, C1, CAST_SPEED6, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "&2The woods in the surrounding area break apart and crumble.&0");

    spello(SPELL_VAMPIRIC_BREATH, "vampiric breath", 40, 25, 1, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true, MAG_DAMAGE | MAG_MANUAL, C1, CAST_SPEED1, DAM_HEAL,
           SKILL_SPHERE_DEATH, 5, false, nullptr);

    spello(SPELL_VAPORFORM, "vaporform", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECT,
           C1, CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_PROT, 35, true, "Your form condenses into flesh once again.");

    spello(SPELL_VENTRILOQUATE, "ventriloquate", 0, 0, 0, POS_SITTING, false, TAR_CHAR_ROOM | TAR_NOT_SELF, false,
           MAG_MANUAL, C1, CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_GENERIC, 6, false, nullptr);

    spello(SPELL_VICIOUS_MOCKERY, "vicious mockery", 40, 30, 2, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_DIRECT, true, MAG_DAMAGE, C1, CAST_SPEEDE, DAM_MENTAL,
           SKILL_SPHERE_GENERIC, 21, false, nullptr);

    spello(SPELL_VIGORIZE_CRITIC, "vigorize critic", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_POINT, C1,
           CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_VIGORIZE_LIGHT, "vigorize light", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_POINT, C1,
           CAST_SPEED2, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_VIGORIZE_SERIOUS, "vigorize serious", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_POINT,
           C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_HEALING, 5, false, nullptr);

    spello(SPELL_VITALITY, "vitality", 50, 30, 5, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 7, false, "Your magical vitality drains away.");

    spello(SPELL_WALL_OF_FOG, "wall of fog", 50, 25, 5, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_ROOM, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 21, false, "The fog seems to clear out.");

    spello(SPELL_WALL_OF_ICE, "wall of ice", 0, 0, 0, POS_STANDING, false, TAR_STRING, false, MAG_MANUAL, C1, 18,
           DAM_COLD, SKILL_SPHERE_WATER, 27, true, "The wall of ice melts away...");

    spello(SPELL_WALL_OF_STONE, "wall of stone", 0, 0, 0, POS_STANDING, false, TAR_STRING, false, MAG_MANUAL, C1,
           CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 25, false, "The wall of stone crumbles into dust.");

    spello(SPELL_WANDERING_WOODS, "wandering woods", 0, 0, 0, POS_STANDING, false, TAR_IGNORE, false, MAG_MANUAL, C1,
           16, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "The woods around you shift back to their proper form.");

    spello(SPELL_WATERFORM, "waterform", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECT,
           C1, CAST_SPEED7, DAM_UNDEFINED, SKILL_SPHERE_WATER, 27, true, "Your form solidifies into flesh once again.");

    spello(SPELL_WATERWALK, "waterwalk", 35, 5, 3, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 29, false, "Your feet seem less buoyant.");

    spello(SPELL_WEB, "web", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, false,
           MAG_AFFECT, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 21, false,
           "The webs holding you in place dissolve.");

    spello(SPELL_WINGS_OF_HEAVEN, "wings of heaven", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "Your wings gently fold back and fade away.");

    spello(SPELL_WINGS_OF_HELL, "wings of hell", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false,
           "Your giant bat-like wings fold up and vanish.");

    spello(SPELL_WIZARD_EYE, "wizard eye", 0, 0, 0, POS_STANDING, false, TAR_CHAR_WORLD, false, MAG_MANUAL, C1, 16,
           DAM_UNDEFINED, SKILL_SPHERE_DIVIN, 17, true, nullptr);

    spello(SPELL_WORD_OF_COMMAND, "word of command", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_MANUAL, C1,
           CAST_SPEED7, DAM_MENTAL, SKILL_SPHERE_GENERIC, 5, true, nullptr);

    spello(SPELL_WORD_OF_RECALL, "word of recall", 20, 10, 2, POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_MANUAL, C1, 1, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 5, false, nullptr);

    spello(SPELL_WORLD_TELEPORT, "world teleport", 0, 0, 0, POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_MANUAL, C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_SUMMON, 11, false, nullptr);

    spello(SPELL_WRITHING_WEEDS, "writhing weeds", 0, 0, 0, POS_STANDING, true,
           TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OUTDOORS, true, MAG_DAMAGE, C1, CAST_SPEED3, DAM_CRUSH,
           SKILL_SPHERE_EARTH, 5, false, nullptr);

    /* SORTED */

    /* The following spells are intended only for magic items.
     * Magic items have no way to accept arguments for spells that require making type selections, so they are broken
     * out here. They should not be assigned to classes without VERY good reason. */

    spello(SPELL_ENHANCE_STR, "enhance strength", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "You feel weaker.");

    spello(SPELL_ENHANCE_DEX, "enhance dexterity", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "You feel slower.");

    spello(SPELL_ENHANCE_CON, "enhance constitution", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT,
           C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "You feel less healthy.");

    spello(SPELL_ENHANCE_INT, "enhance intelligence", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT,
           C1, CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "You feel dumber.");

    spello(SPELL_ENHANCE_WIS, "enhance wisdom", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "You feel less witty.");

    spello(SPELL_ENHANCE_CHA, "enhance charisma", 35, 30, 1, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, C1,
           CAST_SPEED3, DAM_UNDEFINED, SKILL_SPHERE_ENCHANT, 5, false, "You feel uglier.");

    spello(SPELL_PROTECT_FIRE, "protection from fire", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT,
           C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false, "You feel less protected from fire.");

    spello(SPELL_PROTECT_COLD, "protection from cold", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT,
           C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false, "You feel less protected from cold.");

    spello(SPELL_PROTECT_ACID, "protection from earth", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT,
           C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false, "You feel less protected from earth.");

    spello(SPELL_PROTECT_SHOCK, "protection from air", 0, 0, 0, POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT,
           C1, CAST_SPEED4, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false, "You feel less protected from air.");

    spello(SPELL_MONK_FIRE, "fires of saint augustine", 0, 0, 0, POS_SITTING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY,
           false, MAG_AFFECT, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false, "Your inner fire subsides.");

    spello(SPELL_MONK_COLD, "blizzards of saint augustine", 0, 0, 0, POS_SITTING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY,
           false, MAG_AFFECT, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false, "Your inner cold subsides.");

    spello(SPELL_MONK_ACID, "tremors of saint augustine", 0, 0, 0, POS_SITTING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY,
           false, MAG_AFFECT, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false,
           "Your inner earth subsides.");

    spello(SPELL_MONK_SHOCK, "tempest of saint augustine", 0, 0, 0, POS_SITTING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY,
           false, MAG_AFFECT, C1, CAST_SPEED5, DAM_UNDEFINED, SKILL_SPHERE_PROT, 5, false,
           "Your inner storm subsides.");

    /* Declaration of skills - sets skills up so that immortals can use
     * them by default. Determines whether a skill is considered
     * "humanoid only". */

    /* skillo(skill, name, humanoid, target mode) */
    /* target mode for skills is designed for use with TAR_CONTACT and TAR_DIRECT,
       which allow you to limit whether an attack may be caught by guard or not.
     */
    skillo(SKILL_2H_BLUDGEONING, "2H bludgeoning weapons", true, TAR_CONTACT);
    skillo(SKILL_2H_PIERCING, "2H piercing weapons", true, TAR_CONTACT);
    skillo(SKILL_2H_SLASHING, "2H slashing weapons", true, TAR_CONTACT);
    skillo(SKILL_BACKSTAB, "backstab", true, TAR_CONTACT);
    skillo(SKILL_BANDAGE, "bandage", true, 0);
    skillo(SKILL_BAREHAND, "barehand", false, TAR_CONTACT);
    skillo(SKILL_BASH, "bash", false, TAR_CONTACT);
    skillo(SKILL_BATTLE_HOWL, "battle howl", false, 0);
    skillo(SKILL_BERSERK, "berserk", false, 0);
    skillo(SKILL_BIND, "bind", true, TAR_CONTACT);
    skillo(SKILL_BLUDGEONING, "bludgeoning weapons", true, TAR_CONTACT);
    skillo(SKILL_BODYSLAM, "bodyslam", false, TAR_CONTACT);
    skillo(SKILL_BREATHE_ACID, "breathe acid", false, 0);
    skillo(SKILL_BREATHE_GAS, "breathe gas", false, 0);
    skillo(SKILL_BREATHE_FIRE, "breathe fire", false, 0);
    skillo(SKILL_BREATHE_FROST, "breathe frost", false, 0);
    skillo(SKILL_BREATHE_LIGHTNING, "breathe lightning", false, 0);
    skillo(SKILL_CARTWHEEL, "cartwheel", false, TAR_CONTACT);
    skillo(SKILL_CHANT, "chant", false, 0);
    skillo(SKILL_DOUBLE_ATTACK, "double attack", false, 0);
    skillo(SKILL_DUAL_WIELD, "dual wield", true, 0);
    skillo(SKILL_CIRCLE, "circle", false, TAR_CONTACT);
    skillo(SKILL_CLAW, "claw", false, TAR_CONTACT);
    skillo(SKILL_CONCEAL, "conceal", false, 0);
    skillo(SKILL_CORNER, "corner", false, 0);
    skillo(SKILL_DISARM, "disarm", true, TAR_CONTACT);
    skillo(SKILL_EYE_GOUGE, "eye gouge", true, TAR_CONTACT);
    skillo(SKILL_DODGE, "dodge", false, 0);
    skillo(SKILL_DOORBASH, "doorbash", false, 0);
    skillo(SKILL_DOUSE, "douse", false, 0);
    skillo(SKILL_ELECTRIFY, "electrify", false, TAR_DIRECT);
    skillo(SKILL_FIRST_AID, "first aid", true, 0);
    skillo(SKILL_GROUND_SHAKER, "ground shaker", false, 0);
    skillo(SKILL_GROUP_RETREAT, "group retreat", false, 0);
    skillo(SKILL_GUARD, "guard", false, 0);
    skillo(SKILL_HIDE, "hide", false, 0);
    skillo(SKILL_HITALL, "hitall", false, TAR_CONTACT);
    skillo(SKILL_HUNT, "hunt", false, 0);
    skillo(SKILL_INSTANT_KILL, "instant kill", true, TAR_CONTACT);
    skillo(SKILL_KICK, "kick", true, TAR_CONTACT);
    skillo(SKILL_MAUL, "maul", false, TAR_CONTACT);
    skillo(SKILL_MEDITATE, "meditate", false, 0);
    skillo(SKILL_MISSILE, "missile weapons", true, TAR_DIRECT);
    skillo(SKILL_MOUNT, "mount", false, 0);
    skillo(SKILL_PARRY, "parry", true, 0);
    skillo(SKILL_PECK, "peck", false, TAR_CONTACT);
    skillo(SKILL_PERFORM, "perform", false, 0);
    skillo(SKILL_PICK_LOCK, "pick lock", true, 0);
    skillo(SKILL_PIERCING, "piercing weapons", true, TAR_CONTACT);
    skillo(SKILL_PUNCH, "punch", true, TAR_CONTACT);
    skillo(SKILL_QUICK_CHANT, "quick chant", false, 0);
    skillo(SKILL_RESCUE, "rescue", false, 0);
    skillo(SKILL_RIDING, "riding", false, 0);
    skillo(SKILL_RIPOSTE, "riposte", true, 0);
    skillo(SKILL_ROAR, "roar", false, 0);
    skillo(SKILL_SAFEFALL, "safefall", false, 0);
    skillo(SKILL_SCRIBE, "scribe", true, 0);
    skillo(SKILL_SHAPECHANGE, "shapechange", false, 0);
    skillo(SKILL_SNEAK, "sneak", false, 0);
    skillo(SKILL_KNOW_SPELL, "spell knowledge", false, 0);
    skillo(SKILL_RETREAT, "retreat", false, 0);
    skillo(SKILL_SHADOW, "shadow", false, 0);
    skillo(SKILL_SLASHING, "slashing weapons", true, TAR_CONTACT);
    skillo(SKILL_SPHERE_AIR, "sphere of air", false, 0);
    skillo(SKILL_SPHERE_DEATH, "sphere of death", false, 0);
    skillo(SKILL_SPHERE_DIVIN, "sphere of divination", false, 0);
    skillo(SKILL_SPHERE_EARTH, "sphere of earth", false, 0);
    skillo(SKILL_SPHERE_ENCHANT, "sphere of enchantment", false, 0);
    skillo(SKILL_SPHERE_FIRE, "sphere of fire", false, 0);
    skillo(SKILL_SPHERE_GENERIC, "sphere of generic", false, 0);
    skillo(SKILL_SPHERE_HEALING, "sphere of healing", false, 0);
    skillo(SKILL_SPHERE_PROT, "sphere of protection", false, 0);
    skillo(SKILL_SPHERE_SUMMON, "sphere of summoning", false, 0);
    skillo(SKILL_SPHERE_WATER, "sphere of water", false, 0);
    skillo(SKILL_SPRINGLEAP, "springleap", false, TAR_CONTACT);
    skillo(SKILL_STEAL, "steal", true, TAR_CONTACT);
    skillo(SKILL_STEALTH, "stealth", false, 0);
    skillo(SKILL_SUMMON_MOUNT, "summon mount", true, 0);
    skillo(SKILL_SWEEP, "sweep", false, TAR_CONTACT);
    skillo(SKILL_SWITCH, "switch", false, TAR_CONTACT);
    skillo(SKILL_TAME, "tame", false, 0);
    skillo(SKILL_TANTRUM, "tantrum", false, TAR_CONTACT);
    skillo(SKILL_THROATCUT, "throatcut", true, TAR_CONTACT);
    skillo(SKILL_TRACK, "track", false, 0);
    skillo(SKILL_VAMP_TOUCH, "vampiric touch", false, TAR_CONTACT);

    /* Set up monk/berserker chants */
    /*
     * Arguments for chanto calls:
     *
     * chant, name, minpos, ok_fighting, targets, violent, routines, damage, quest, wearoff
     */

    chanto(CHANT_APOCALYPTIC_ANTHEM, "apocalyptic anthem", POS_STANDING, true, TAR_IGNORE, true, MAG_MANUAL, 0, true,
           nullptr);

    chanto(CHANT_ARIA_OF_DISSONANCE, "aria of dissonance", POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
           MAG_AFFECT, 0, true, "The dissonance stops ringing in your ears.");

    chanto(CHANT_BATTLE_HYMN, "battle hymn", POS_STANDING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECT, 0,
           false, "Your rage fades away.");

    chanto(CHANT_INTERMINABLE_WRATH, "interminable wrath", POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, 0, false, nullptr);

    chanto(CHANT_IVORY_SYMPHONY, "ivory symphony", POS_STANDING, true, TAR_IGNORE, true, MAG_MANUAL, 0, false,
           "Feeling returns to your limbs.");

    chanto(CHANT_PEACE, "peace", POS_STANDING, true, TAR_IGNORE, false, MAG_MANUAL, 0, false, nullptr);

    chanto(CHANT_REGENERATION, "regeneration", POS_SITTING, false, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECT, 0,
           false, "Your healthy feeling subsides.");

    chanto(CHANT_SHADOWS_SORROW_SONG, "shadows sorrow song", POS_STANDING, true, TAR_IGNORE, true, MAG_MASS, 0, false,
           "The shadows in your mind clear up.");

    chanto(CHANT_SEED_OF_DESTRUCTION, "seed of destruction", POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
           MAG_AFFECT, 0, true, "The disease leaves you.");

    chanto(CHANT_SONATA_OF_MALAISE, "sonata of malaise", POS_STANDING, true, TAR_IGNORE, true, MAG_MASS, 0, false,
           "The sonata of malaise stops echoing in your ears.");

    chanto(CHANT_SPIRIT_BEAR, "spirit of the bear", POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, 0, false, "Your claws become decidedly less bear-like.");

    chanto(CHANT_SPIRIT_WOLF, "spirit of the wolf", POS_STANDING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY, false,
           MAG_AFFECT, 0, false, "Your fangs recede and you lose your wolf-like spirit.");

    chanto(CHANT_WAR_CRY, "war cry", POS_STANDING, false, TAR_IGNORE, false, MAG_GROUP, 0, false,
           "Your determination level returns to normal.");

    chanto(CHANT_HYMN_OF_SAINT_AUGUSTINE, "hymn of saint augustine", POS_SITTING, true, TAR_CHAR_ROOM | TAR_SELF_ONLY,
           false, MAG_AFFECT, 0, true, "Your inner elements subside.");

    /* Set up bard songs. */

    /* Arguments for songo calls
     *
     * song, name, minpos, ok_fighting, targets, violent, routines, damage, quest, wearoff
     */

    songo(SONG_BALLAD_OF_TEARS, "ballad of tears", POS_STANDING, true, TAR_IGNORE, true, MAG_AREA | MAG_UNAFFECT, 0,
          false, "Your nerves settle down as the terror leaves you.");

    songo(SONG_SONG_OF_REST, "song of rest", POS_STANDING, false, TAR_CHAR_ROOM, false, MAG_AFFECT, 0, false,
          "The restful song fades from your memory.");

    songo(SONG_CROWN_OF_MADNESS, "crown of madness", POS_STANDING, true, TAR_IGNORE, true, MAG_AREA, 0, true,
          "Your mind returns to reality.");

    songo(SONG_ENRAPTURE, "enrapture", POS_STANDING, false, TAR_IGNORE, false, MAG_AREA, 0, true,
          "You regain your senses as the illusions subside.");

    songo(SONG_FREEDOM_SONG, "freedom song", POS_STANDING, true, TAR_IGNORE, true, MAG_GROUP, 0, false,
          "Your nerves settle down as the terror leaves you.");

    songo(SONG_HEARTHSONG, "hearthsong", POS_STANDING, false, TAR_IGNORE, false, MAG_GROUP, 0, true,
          "Your familiar disguise melts away.");

    songo(SONG_HEROIC_JOURNEY, "heroic journey", POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_GROUP | MAG_UNAFFECT, 0,
          false, "Your inspiration fades.");

    songo(SONG_INSPIRATION, "inspiration", POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_AFFECT | MAG_UNAFFECT, 0,
          false, "Your inspiration fades.");

    songo(SONG_JOYFUL_NOISE, "joyful noise", POS_STANDING, true, TAR_CHAR_ROOM, false, MAG_UNAFFECT, 0, false, nullptr);

    songo(SONG_TERROR, "terror", POS_STANDING, true, TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_AFFECT | MAG_UNAFFECT, 0,
          false, "Your nerves settle down as the terror leaves you.");

    /* Set up non-skill effects */
    /* effect, name, wearoff */
    effecto(SKILL_AWARE, "aware", "");
}

static int skill_comparator(int a, int b) { return strcasecmp(skills[a].name, skills[b].name); }

void sort_skills(void) {
    int i;

    /* initialize array */
    for (i = 0; i <= TOP_SKILL; ++i)
        skill_sort_info[i] = i;

    /* Start at element 1 to skip 'RESERVED' */
    sort(quicksort, &skill_sort_info[1], TOP_SKILL, skill_comparator);
}

void skill_assign(int skillnum, int class_num, int level) {
    int okay = true;

    if (skillnum < 0 || skillnum > TOP_SKILL_DEFINE) {
        log("SYSERR: attempting assign to illegal talent num {:d}", skillnum);
        return;
    }

    if (class_num < 0 || class_num >= NUM_CLASSES) {
        log("SYSERR: assigning '{}' to illegal class_num {:d}", skill_name(skillnum), class_num);
        okay = false;
    }

    if (level < 1 || level > LVL_IMPL) {
        log("SYSERR: assigning '{}' to illegal level {:d}", skill_name(skillnum), level);
        okay = false;
    }

    if (okay) {
        skills[skillnum].min_level[class_num] = level;
        skills[skillnum].lowest_level = std::min(skills[skillnum].lowest_level, level);
    }
}

int level_to_circle(int level) { return std::clamp((level - 1) / 8 + 1, 1, NUM_SPELL_CIRCLES); }

int circle_to_level(int circle) { return std::clamp((circle - 1) * 8 + 1, 1, LVL_IMPL); }

bool get_spell_assignment_circle(CharData *ch, int spell, int *circle_assignment, int *level_assignment) {
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
            return true;
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
                return true;
            }
        }
    }
    return false;
}