/***************************************************************************
 * $Id: sorcerer.c,v 1.36 2009/03/20 06:15:17 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: sorcerer.c                                     Part of FieryMUD *
 *  Usage: AI actions for sorcerer-type mobiles.                           *
 *     By: Laoris of FieryMUD                                              *
 *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#include "ai.h"
#include "casting.h"
#include "comm.h"
#include "composition.h"
#include "conf.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "lifeforce.h"
#include "magic.h"
#include "math.h"
#include "races.h"
#include "skills.h"
#include "spell_parser.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* Local functions */
bool check_sorcerer_status(struct char_data *ch);

/* External functions */
int mob_cast(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int spellnum);
bool affected_by_armor_spells(struct char_data *ch);

/* Sorcerer spell lists */
const struct spell_pair mob_sorcerer_buffs[] = {{SPELL_STONE_SKIN, 0, EFF_STONE_SKIN},
                                                {SPELL_HASTE, 0, EFF_HASTE},
                                                {SPELL_COLDSHIELD, 0, EFF_COLDSHIELD},
                                                {SPELL_FIRESHIELD, 0, EFF_FIRESHIELD},
                                                {SPELL_WATERFORM, 0, 0},
                                                {SPELL_VAPORFORM, 0, 0},
                                                {SPELL_NEGATE_HEAT, 0, EFF_NEGATE_HEAT},
                                                {SPELL_NEGATE_COLD, 0, EFF_NEGATE_COLD},
                                                {SPELL_MAJOR_GLOBE, 0, EFF_MAJOR_GLOBE},
                                                {SPELL_MINOR_GLOBE, 0, EFF_MINOR_GLOBE},
                                                {SPELL_DETECT_INVIS, 0, EFF_DETECT_INVIS},
                                                {SPELL_INVISIBLE, 0, EFF_INVISIBLE},
                                                {SPELL_FLY, 0, EFF_FLY},
                                                {SPELL_MIRAGE, 0, 0},
                                                {SPELL_MISDIRECTION, 0, EFF_MISDIRECTION},
                                                {SPELL_ICE_ARMOR, 0, 0},
                                                {SPELL_BONE_ARMOR, 0, 0},
                                                {0, 0, 0}};

/* In order of spells to attempt */
const int mob_sorcerer_offensives[] = {
    SPELL_DISINTEGRATE,   SPELL_BIGBYS_CLENCHED_FIST, SPELL_ICEBALL,    SPELL_MELT,         SPELL_ACID_BURST,
    SPELL_POSITIVE_FIELD, SPELL_DEGENERATION,         SPELL_SOUL_TAP,   SPELL_CONE_OF_COLD, SPELL_ENERGY_DRAIN,
    SPELL_CONFUSION,      SPELL_DISINTEGRATE,         SPELL_NIGHTMARE,  SPELL_MESMERIZE,    SPELL_FEAR,
    SPELL_FIREBALL,       SPELL_PHOSPHORIC_EMBERS,    SPELL_FREEZE,     SPELL_HYSTERIA,     SPELL_LIGHTNING_BOLT,
    SPELL_SHOCKING_GRASP, SPELL_CHILL_TOUCH,          SPELL_DETONATION, SPELL_FIRE_DARTS,   SPELL_BURNING_HANDS,
    SPELL_ICE_DARTS,      SPELL_MAGIC_MISSILE,        SPELL_SMOKE,      SPELL_SOUL_TAP,     0};

const int mob_sorcerer_area_spells[] = {SPELL_CHAIN_LIGHTNING,
                                        SPELL_COLOR_SPRAY,
                                        SPELL_CREMATE,
                                        SPELL_FIRESTORM,
                                        SPELL_FLOOD,
                                        SPELL_FREEZING_WIND,
                                        SPELL_ICE_SHARDS,
                                        SPELL_ICE_STORM,
                                        SPELL_METEORSWARM,
                                        SPELL_SEVERANCE,
                                        SPELL_SOUL_REAVER,
                                        SPELL_SUPERNOVA,
                                        0};

bool sorcerer_ai_action(struct char_data *ch, struct char_data *victim) {
    int my_health, victim_health, i, counter;

    if (!victim) {
        mudlog("No victim in sorcerer AI action.", NRM, LVL_GOD, FALSE);
        return FALSE;
    }

    my_health = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
    victim_health = (100 * GET_HIT(victim)) / GET_MAX_HIT(victim);

    /* If mob has low health, maybe try to teleport. */
    if (my_health < 10 && !PLAYERALLY(ch) && !number(0, 3) && mob_cast(ch, ch, NULL, SPELL_TELEPORT))
        return TRUE;

    /* Check buff spells */
    if (victim_health > 15 && check_sorcerer_status(ch))
        return TRUE;

    /* If the victim is grouped, try an area spell first. */
    if (group_size(victim) > 1) {
        counter = 0;
        for (i = 0; mob_sorcerer_area_spells[i]; i++) {
            if (!GET_SKILL(ch, mob_sorcerer_area_spells[i]))
                continue;
            if (mob_cast(ch, victim, NULL, mob_sorcerer_area_spells[i]))
                return TRUE;
            /* Only try the mob's best two spells. */
            if (++counter >= 2)
                break;
        }
    }

    /* Sorcerers get 13% chance to cast harness. */
    if (GET_SKILL(ch, SPELL_HARNESS) && !EFF_FLAGGED(ch, EFF_HARNESS) && FIGHTING(ch) && victim_health > 75 &&
        !number(0, 6) && mob_cast(ch, ch, NULL, SPELL_HARNESS))
        return TRUE;

    /* Necromancers get 10% chance to cast poison. */
    if (GET_SKILL(ch, SPELL_POISON) && !EFF_FLAGGED(ch, EFF_POISON) && victim_health > 75 && !number(0, 9) &&
        mob_cast(ch, victim, NULL, SPELL_POISON))
        return TRUE;

    /* Try and cast ray of enfeeblement! */
    if (GET_SKILL(ch, SPELL_RAY_OF_ENFEEB) && !EFF_FLAGGED(victim, EFF_RAY_OF_ENFEEB) &&
        /* Better chance to attempt if victim in good condition. */
        !number(0, (victim_health > 80) ? 2 : 7) && mob_cast(ch, victim, NULL, SPELL_RAY_OF_ENFEEB))
        return TRUE;

    counter = 0;
    /* Single-target offensive spell */
    for (i = 0; mob_sorcerer_offensives[i]; i++) {
        if (!GET_SKILL(ch, mob_sorcerer_offensives[i]))
            continue;

        switch (mob_sorcerer_offensives[i]) {
        case SPELL_DEGENERATION:
        case SPELL_SOUL_TAP:
        case SPELL_ENERGY_DRAIN:
            if (GET_LIFEFORCE(victim) == LIFE_UNDEAD || GET_LIFEFORCE(victim) == LIFE_MAGIC)
                continue;
            break;
        case SPELL_REBUKE_UNDEAD:
            if (GET_LIFEFORCE(victim) != LIFE_UNDEAD)
                continue;
            break;
        }

        if (mob_cast(ch, victim, NULL, mob_sorcerer_offensives[i]))
            return TRUE;

        /* Only attempt to cast this mob's best 3 spells. */
        if (++counter >= 3)
            break;
    }

    return FALSE;
}

/* This checks mob buffs. */
bool check_sorcerer_status(struct char_data *ch) {
    int i;

    for (i = 0; mob_sorcerer_buffs[i].spell; i++) {
        if (!GET_SKILL(ch, mob_sorcerer_buffs[i].spell) ||
            !check_fluid_spell_ok(ch, ch, mob_sorcerer_buffs[i].spell, TRUE))
            continue;

        if (!valid_cast_stance(ch, i))
            continue;
        /* 20% chance to cancel if in combat. */
        if (FIGHTING(ch) && number(0, 9) < 2)
            return FALSE;
        switch (mob_sorcerer_buffs[i].spell) {
        case SPELL_COLDSHIELD:
        case SPELL_FIRESHIELD:
            /* Coldshield and fireshield don't mix. */
            if (EFF_FLAGGED(ch, EFF_COLDSHIELD) || EFF_FLAGGED(ch, EFF_FIRESHIELD))
                continue;
            break;
        case SPELL_MINOR_GLOBE:
        case SPELL_MAJOR_GLOBE:
            /* Major globe and minor globe don't mix either. */
            if (EFF_FLAGGED(ch, EFF_MINOR_GLOBE) || EFF_FLAGGED(ch, EFF_MAJOR_GLOBE))
                continue;
            break;
        case SPELL_MIRAGE:
        case SPELL_ICE_ARMOR:
        case SPELL_BONE_ARMOR:
            /* Armor spells don't mix. */
            if (affected_by_armor_spells(ch))
                continue;
            break;
        case SPELL_WATERFORM:
            if (GET_COMPOSITION(ch) != COMP_FLESH)
                continue;
            break;
        case SPELL_VAPORFORM:
            if (GET_COMPOSITION(ch) != COMP_FLESH)
                continue;
            break;
        default:
            /* For other spells, just check to see if they have it. */
            if (has_effect(ch, &mob_sorcerer_buffs[i]))
                continue;
        }
        if (mob_cast(ch, ch, NULL, mob_sorcerer_buffs[i].spell))
            return TRUE;
    }

    return FALSE;
}

bool mob_animate(struct char_data *ch) {
    struct obj_data *obj;

    if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC) || EFF_FLAGGED(ch, EFF_SILENCE))
        return FALSE;

    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
        /* There's no IS_NPC_CORPSE macro */
        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && GET_OBJ_VAL(obj, VAL_CONTAINER_CORPSE) == CORPSE_NPC &&
            GET_OBJ_MOB_FROM(obj) != NOBODY && GET_LEVEL(mob_proto + GET_OBJ_MOB_FROM(obj)) + 10 < GET_LEVEL(ch))
            return mob_cast(ch, NULL, obj, SPELL_ANIMATE_DEAD);

    return FALSE;
}

/***************************************************************************
 * $Log: sorcerer.c,v $
 * Revision 1.36  2009/03/20 06:15:17  myc
 * Adding a TAR_GROUND cast requirement.  Added detonation,
 * phosphoric embers, positive field, and acid burst spells.
 * Removed combust and heatwave.  Made soul tap a manual spell.
 *
 * Revision 1.35  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.34  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.33  2009/02/05 16:26:57  myc
 * Modify list of offensive sorcerer spells to include disintegrate
 * and others.
 *
 * Revision 1.32  2009/01/19 09:25:23  myc
 * Removed MOB_PET flag.
 *
 * Revision 1.31  2009/01/17 00:28:02  myc
 * Fix use of uninitialized variable.
 *
 * Revision 1.30  2008/09/14 02:24:11  jps
 * Using magic.h.
 *
 * Revision 1.29  2008/09/14 02:23:19  jps
 * Don't attempt to use buffs that can't be used when you're fluid.
 *
 * Revision 1.28  2008/09/11 02:50:02  jps
 * Changed skills so you have a minimum position, and fighting_ok fields.
 *
 * Revision 1.27  2008/08/10 16:31:52  jps
 * Added severance and soul reaver to sorcerer area attack spells.
 *
 * Revision 1.26  2008/06/07 19:06:46  myc
 * Moved object-related constants and routines to objects.h.
 *
 * Revision 1.25  2008/05/19 06:33:31  jps
 * Add mesmerize to sorcerer attack spells.
 * Add misdirection to sorcerer buff spells.
 *
 * Revision 1.24  2008/05/18 22:55:37  jps
 * Added hysteria to the list of ai sorcerer spells.
 *
 * Revision 1.23  2008/05/18 02:08:20  jps
 * Adding nightmare to sorcerer AI spell list.
 *
 * Revision 1.22  2008/04/18 15:53:21  jps
 * Added some illusionist spells to sorcerer AI.
 *
 * Revision 1.21  2008/04/14 05:11:40  jps
 * Renamed EFF_FLYING to EFF_FLY, since it only indicates an ability
 * to fly - not that the characer is actually flying.
 *
 * Revision 1.20  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.19  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.18  2008/03/26 23:12:42  jps
 * Changed the handling of waterform and vaporform spells.
 *
 * Revision 1.17  2008/03/26 16:44:36  jps
 * Replaced all checks for undead race with checks for undead lifeforce.
 * Replaced the undead race with the plant race.
 *
 * Revision 1.16  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.15  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.14  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species
 *into races.
 *
 * Revision 1.13  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.12  2008/01/13 03:19:53  myc
 * Added all the new sorcerer spells to the AI casting list.  Removed
 * MSKILL references.
 *
 * Revision 1.11  2008/01/12 23:13:20  myc
 * Replaced try_cast with direct calls to mob_cast, which now
 * supports target objects.
 *
 * Revision 1.10  2008/01/12 19:12:05  myc
 * Oops, accidentally erased some of the log entries.
 *
 * Revision 1.9  2008/01/12 19:08:14  myc
 * Rerowte a lot of mob AI functionality.
 *
 * Revision 1.8  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.7  2006/11/08 07:55:17  jps
 * Change verbal instances of "breath" to "breathe"
 *
 * Revision 1.6  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.5  2000/11/25 02:33:15  rsd
 * Altered comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.4  1999/11/28 23:54:25  cso
 * took out the check that kept charmed mobs from casting spells. now they can
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/02/02 15:15:42  mud
 * dos2unix
 * indented file
 * added standard comment header.
 *
 * Revision 1.1  1999/01/29 01:23:32  mud
 * Initial revision
 *
 ***************************************************************************/
