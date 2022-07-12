/***************************************************************************
 *   File: spell_parser.c                                 Part of FieryMUD *
 *  Usage: top-level magic routines; outside points of entry to magic sys. *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "spell_parser.h"

#include "casting.h"
#include "class.h"
#include "comm.h"
#include "conf.h"
#include "constants.h"
#include "cooldowns.h"
#include "db.h"
#include "dg_scripts.h"
#include "events.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "magic.h"
#include "math.h"
#include "screen.h"
#include "skills.h"
#include "spells.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

extern void charge_mem(struct char_data *ch, int spellnum);
extern int check_spell_memory(struct char_data *ch, int spellnum);
void complete_spell(struct char_data *ch);
void start_chant(struct char_data *ch);
void end_chant(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int spellnum);
int bad_guess(struct char_data *ch);
bool find_spell_target(int spellnum, struct char_data *ch, char *t, int *target_status, struct char_data **tch,
                       struct obj_data **tobj);
bool check_spell_target(int spellnum, struct char_data *ch, struct char_data *tch, struct obj_data *tobj);
void abort_casting(struct char_data *ch);
void aggro_lose_spells(struct char_data *ch);
EVENTFUNC(casting_handler);

int bogus_mage_spells[] = {0,
                           /* first 10 are defensive */
                           SPELL_ENHANCE_ABILITY, SPELL_DETECT_INVIS, SPELL_INFRAVISION, SPELL_MINOR_GLOBE, SPELL_FIRESHIELD,
                           SPELL_COLDSHIELD, SPELL_FARSEE, SPELL_FLY, SPELL_HASTE, SPELL_MAJOR_GLOBE,
                           /* next 10 are offensive */
                           SPELL_BURNING_HANDS, SPELL_CHILL_TOUCH, SPELL_MINOR_PARALYSIS, SPELL_LIGHTNING_BOLT,
                           SPELL_RAY_OF_ENFEEB, SPELL_SLEEP, SPELL_CONE_OF_COLD, SPELL_FIREBALL,
                           SPELL_BIGBYS_CLENCHED_FIST, SPELL_CHAIN_LIGHTNING};

int bogus_priest_spells[] = {0,
                             /* first 10 are defensive */
                             SPELL_CURE_LIGHT, SPELL_BLESS, SPELL_CURE_SERIOUS, SPELL_PROT_FROM_EVIL, SPELL_CURE_CRITIC,
                             SPELL_SENSE_LIFE, SPELL_HEAL, SPELL_VITALITY, SPELL_WATERWALK, SPELL_SOULSHIELD,
                             /* next 10 are offensive */
                             SPELL_CAUSE_LIGHT, SPELL_CAUSE_SERIOUS, SPELL_CAUSE_CRITIC, SPELL_EARTHQUAKE, SPELL_HARM,
                             SPELL_FLAMESTRIKE, SPELL_FULL_HARM, SPELL_BLINDNESS, SPELL_SILENCE, SPELL_DARKNESS};

struct syllable {
    char *org;
    char *new;
};

struct syllable syls[] = {{" ", " "},      {"ar", "abra"},  {"ate", "i"},         {"cau", "kada"},   {"blind", "nose"},
                          {"bur", "mosa"}, {"cu", "judi"},  {"de", "oculo"},      {"dis", "mar"},    {"ect", "kamina"},
                          {"en", "uns"},   {"gro", "cra"},  {"light", "dies"},    {"lo", "hi"},      {"magi", "kari"},
                          {"mon", "bar"},  {"mor", "zak"},  {"move", "sido"},     {"ness", "lacri"}, {"ning", "illa"},
                          {"per", "duda"}, {"ra", "gru"},   {"re", "candus"},     {"son", "sabru"},  {"tect", "infra"},
                          {"tri", "cula"}, {"ven", "nofo"}, {"word of", "inset"}, {"a", "i"},        {"b", "v"},
                          {"c", "q"},      {"d", "m"},      {"e", "o"},           {"f", "y"},        {"g", "t"},
                          {"h", "p"},      {"i", "u"},      {"j", "y"},           {"k", "t"},        {"l", "r"},
                          {"m", "w"},      {"n", "b"},      {"o", "a"},           {"p", "s"},        {"q", "d"},
                          {"r", "f"},      {"s", "g"},      {"t", "h"},           {"u", "e"},        {"v", "z"},
                          {"w", "x"},      {"x", "n"},      {"y", "l"},           {"z", "k"},        {"", ""}};

void end_chant(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int spellnum) {
    struct char_data *gch;
    struct know_spell *tmp;
    char lbuf[256];
    char spellbuf[256];
    char saybuf[256];
    int ofs, j;
    bool found;

    /* There's no end-of-cast message for ventriloquate. */
    if (spellnum == SPELL_VENTRILOQUATE)
        return;

    /*
     * Loop through bystanders in the room and see if they recognize the
     * spell and/or target.
     */
    for (gch = world[ch->in_room].people; gch; gch = gch->next_in_room) {
        /* either the caster or sleeping bystander */
        if (ch == gch || !AWAKE(gch) || !gch->desc || PLR_FLAGGED(gch, PLR_WRITING) || EDITING(gch->desc))
            continue;

        memset(spellbuf, 0x0, 256);
        memset(saybuf, 0x0, 256);
        found = FALSE;

        /* gods see all */
        if (GET_LEVEL(gch) >= LVL_GOD)
            found = TRUE;

        /* see if anyone recognized the spell */
        for (tmp = ch->see_spell; tmp && !found; tmp = tmp->next)
            if (gch == tmp->sch)
                found = TRUE; /* ok he recognized it */

        if (!found) {
            /* change the syllables of the spoken spell */
            memset(lbuf, 0x0, 256);
            ofs = 0;
            strcpy(lbuf, skills[spellnum].name);
            while (*(lbuf + ofs))
                for (j = 0; *(syls[j].org); j++)
                    if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
                        strcat(spellbuf, syls[j].new);
                        ofs += strlen(syls[j].org);
                    }
        } else
            /* on the list. recognize the spell */
            strcpy(spellbuf, skills[spellnum].name);

        /* Can the bystander see the caster? */
        if (CAN_SEE(gch, ch)) {
            /* Is there a PC/NPC Target?  Is it in the room? */
            if (tch && tch->in_room == ch->in_room) {
                /* Is the caster targeting himself? */
                if (ch == tch)
                    sprintf(saybuf, "$n closes $s eyes and utters the words, '%s'.", spellbuf);
                /* Is the target the receiver of the message? */
                else if (tch == gch)
                    sprintf(saybuf, "$n stares at you and utters the words, '%s'.", spellbuf);
                /* Ok, just a bystander.  But can they see the target? */
                else if (CAN_SEE(gch, tch))
                    sprintf(saybuf, "$n stares at $N and utters the words, '%s'.", spellbuf);
                else
                    sprintf(saybuf, "$n stares off at nothing and utters the words, '%s'.", spellbuf);
            }
            /* Is there an object target in the room? */
            else if (tobj && ((tobj->in_room == ch->in_room) || (tobj->carried_by == ch))) {
                if (CAN_SEE_OBJ(gch, tobj))
                    sprintf(saybuf, "$n stares at $p and utters the words, '%s'.", spellbuf);
                else
                    sprintf(saybuf, "$n stares at something and utters the words, '%s'.", spellbuf);
            }
            /* No target. */
            else
                sprintf(saybuf, "$n utters the words, '%s'.", spellbuf);
        }
        /* The bystander cannot see the caster. */
        else
            sprintf(saybuf, "Someone utters the words, '%s'.", spellbuf);

        /* Sending the message to the bystander or target. */
        format_act(buf, saybuf, ch, tobj, tch, gch);
        cprintf(gch, "%s", buf);
    }
}

/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 *
 * Return value: CAST_RESULT_ flags.
 */
int call_magic(struct char_data *caster, struct char_data *cvict, struct obj_data *ovict, int spellnum, int skill,
               int casttype) {
    int savetype, imp_skill = 0;
    struct char_data *newvict;

    struct char_data *random_attack_target(struct char_data * ch, struct char_data * target, bool verbose);

    if (IS_SKILL(spellnum))
        return FALSE;

    if (!cast_wtrigger(caster, cvict, ovict, spellnum) || !cast_otrigger(caster, ovict, spellnum) ||
        !cast_mtrigger(caster, cvict, spellnum))
        return 0;

    /* Don't check PEACEFUL and NOMAGIC flags for gods */
    if (GET_LEVEL(caster) < LVL_ATTENDANT) {
        if (IS_SPELL(spellnum) && ROOM_FLAGGED(caster->in_room, ROOM_NOMAGIC)) {
            send_to_char("Your magic fizzles out and dies.\r\n", caster);
            act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
            return FALSE;
        }

        if (ROOM_FLAGGED(caster->in_room, ROOM_PEACEFUL) && SINFO.violent) {
            if (IS_SPELL(spellnum)) {
                send_to_char("A flash of white light fills the room, dispelling your violent magic!\r\n", caster);
                act("White light from no particular source suddenly fills the room, then vanishes.", FALSE, caster, 0,
                    0, TO_ROOM);
            } else { /* song/chant */
                send_to_char("Your words dissolve into peaceful nothingness...\r\n", caster);
                act("$n's words fade away into peaceful nothingness...\r\n", FALSE, caster, 0, 0, TO_ROOM);
            }
            return FALSE;
        }
    }

    /* A confused caster - spell could go to the wrong target */
    if (SINFO.violent && CONFUSED(caster) && cvict) {
        newvict = random_attack_target(caster, cvict, FALSE);
        if (newvict && newvict != cvict) {
            act("&5You fumbled the spell and aimed it at $N&0&5 instead!&0", FALSE, caster, 0, newvict, TO_CHAR);
            act("&5$n&0&5 fumbled the spell and aimed it at $N&0&5 instead!&0", TRUE, caster, 0, newvict, TO_NOTVICT);
            act("&5$n&0&5 fumbled the spell and aimed it at YOU instead!&0", FALSE, caster, 0, newvict, TO_VICT);
        }
        cvict = newvict;
    }

    /* A victim attacks back immediately, 80% of the time */
    if (SINFO.violent && cvict && IS_NPC(cvict) && !FIGHTING(cvict) && GET_STANCE(cvict) >= STANCE_RESTING &&
        number(0, 4)) {
        event_create(EVENT_QUICK_AGGRO, quick_aggro_event, mkgenericevent(cvict, caster, 0), TRUE, &(cvict->events), 0);
        remember(cvict, caster);
    }

    if (IS_SPELL(spellnum) && cvict && evades_spell(caster, cvict, spellnum, skill))
        return FALSE;

    /* determine the type of saving throw */
    switch (casttype) {
    case CAST_STAFF:
    case CAST_SCROLL:
    case CAST_POTION:
    case CAST_WAND:
        savetype = SAVING_ROD;
        break;
    case CAST_SPELL:
        savetype = SAVING_SPELL;
        break;
    case CAST_BREATH:
        savetype = SAVING_BREATH;
        break;
    default:
        savetype = SAVING_BREATH;
        break;
    }

    if (IS_SET(SINFO.routines, MAG_DAMAGE))
        imp_skill |= mag_damage(skill, caster, cvict, spellnum, savetype);

    /* Don't hit with any effects if they're already dead. */
    if (cvict && DECEASED(cvict))
        return imp_skill;

    /* We only allow a subset of illusory spellcasting */
    if (MOB_FLAGGED(caster, MOB_ILLUSORY)) {
        /* For beneficial and effect-causing spells, only allow casting upon self */
        if (caster == cvict) {
            if (IS_SET(SINFO.routines, MAG_AFFECT))
                imp_skill |= mag_affect(skill, caster, cvict, spellnum, savetype, casttype);

            if (IS_SET(SINFO.routines, MAG_UNAFFECT))
                imp_skill |= mag_unaffect(skill, caster, cvict, spellnum, savetype);

            if (IS_SET(SINFO.routines, MAG_POINT))
                imp_skill |= mag_point(skill, caster, cvict, spellnum, savetype);
        }

        /* Allow mag_areas because it will look like the caster is hurting folks */
        if (IS_SET(SINFO.routines, MAG_AREA))
            imp_skill |= mag_area(skill, caster, spellnum, savetype);

        /* And a few manual spells... */
        if (IS_SET(SINFO.routines, MAG_MANUAL))
            switch (spellnum) {
            case SPELL_COLOR_SPRAY:
                MANUAL_SPELL(spell_color_spray);
                break;
            case SPELL_DEGENERATION:
                MANUAL_SPELL(spell_degeneration);
                break;
            case SPELL_DIMENSION_DOOR:
                MANUAL_SPELL(spell_dimension_door);
                break;
            case SPELL_ENERGY_DRAIN:
                MANUAL_SPELL(spell_energy_drain);
                break;
            case SPELL_FEAR:
                MANUAL_SPELL(spell_fear);
                break;
            case SPELL_FIRE_DARTS:
                MANUAL_SPELL(spell_fire_darts);
                break;
            case SPELL_FLOOD:
                MANUAL_SPELL(spell_flood);
                break;
            case SPELL_HYSTERIA:
                MANUAL_SPELL(spell_fear);
                break;
            case SPELL_ICE_DAGGER:
                MANUAL_SPELL(spell_ice_dagger);
                break;
            case SPELL_ICE_DARTS:
                MANUAL_SPELL(spell_ice_darts);
                break;
            case SPELL_IMMOLATE:
                MANUAL_SPELL(spell_immolate);
                break;
            case SPELL_ISOLATION:
                MANUAL_SPELL(spell_isolation);
                break;
            case SPELL_LIGHTNING_BREATH:
                MANUAL_SPELL(spell_lightning_breath);
                break;
            case SPELL_MAGIC_MISSILE:
                MANUAL_SPELL(spell_magic_missile);
                break;
            case SPELL_MOONBEAM:
                MANUAL_SPELL(spell_moonbeam);
                break;
            case SPELL_PHOSPHORIC_EMBERS:
                MANUAL_SPELL(spell_phosphoric_embers);
                break;
            case SPELL_SOUL_TAP:
                MANUAL_SPELL(spell_soul_tap);
                break;
            case SPELL_SPIRIT_ARROWS:
                MANUAL_SPELL(spell_spirit_arrows);
                break;
            case SPELL_TELEPORT:
                MANUAL_SPELL(spell_teleport);
                break;
            case SPELL_VENTRILOQUATE:
                MANUAL_SPELL(spell_ventriloquate);
                break;
            case SPELL_WORD_OF_COMMAND:
                MANUAL_SPELL(spell_word_of_command);
                break;
            }
        return imp_skill;
    } /* END of illusory spellcasting section */

    if (IS_SET(SINFO.routines, MAG_AFFECT))
        imp_skill |= mag_affect(skill, caster, cvict, spellnum, savetype, casttype);

    if (IS_SET(SINFO.routines, MAG_UNAFFECT))
        imp_skill |= mag_unaffect(skill, caster, cvict, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_POINT))
        imp_skill |= mag_point(skill, caster, cvict, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_ALTER_OBJ))
        imp_skill |= mag_alter_obj(skill, caster, ovict, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_GROUP))
        imp_skill |= mag_group(skill, caster, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_MASS))
        imp_skill |= mag_mass(skill, caster, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_BULK_OBJS))
        imp_skill |= mag_bulk_objs(skill, caster, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_AREA))
        imp_skill |= mag_area(skill, caster, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_SUMMON))
        imp_skill |= mag_summon(skill, caster, cvict, ovict, spellnum, savetype);

    if (IS_SET(SINFO.routines, MAG_CREATION))
        imp_skill |= mag_creation(skill, caster, spellnum);

    if (IS_SET(SINFO.routines, MAG_ROOM))
        imp_skill |= mag_room(skill, caster, spellnum);

    if (IS_SET(SINFO.routines, MAG_MANUAL)) {
        switch (spellnum) {
        case SPELL_ARMOR_OF_GAIA:
            MANUAL_SPELL(spell_armor_of_gaia);
            break;
        case SPELL_BANISH:
            MANUAL_SPELL(spell_banish);
            break;
        case SPELL_CHARM:
            MANUAL_SPELL(spell_charm);
            break;
        case SPELL_COLOR_SPRAY:
            MANUAL_SPELL(spell_color_spray);
            break;
        case SPELL_CREATE_WATER:
            MANUAL_SPELL(spell_create_water);
            break;
        case SPELL_CREEPING_DOOM:
            MANUAL_SPELL(spell_creeping_doom);
            break;
        case SPELL_DARK_FEAST:
            MANUAL_SPELL(spell_dark_feast);
            break;
        case SPELL_DARKNESS:
            MANUAL_SPELL(spell_darkness);
            break;
        case SPELL_DEGENERATION:
            MANUAL_SPELL(spell_degeneration);
            break;
        case SPELL_DIMENSION_DOOR:
            MANUAL_SPELL(spell_dimension_door);
            break;
        case SPELL_DISPEL_MAGIC:
            MANUAL_SPELL(spell_dispel_magic);
            break;
        case SPELL_ENCHANT_WEAPON:
            MANUAL_SPELL(spell_enchant_weapon);
            break;
        case SPELL_ENERGY_DRAIN:
            MANUAL_SPELL(spell_energy_drain);
            break;
        case SPELL_ENLIGHTENMENT:
            MANUAL_SPELL(spell_enlightenment);
            break;
        case SPELL_FEAR:
            MANUAL_SPELL(spell_fear);
            break;
        case SPELL_FIRE_DARTS:
            MANUAL_SPELL(spell_fire_darts);
            break;
        case SPELL_FLAME_BLADE:
            MANUAL_SPELL(spell_flame_blade);
            break;
        case SPELL_FLOOD:
            MANUAL_SPELL(spell_flood);
            break;
        case SPELL_FRACTURE:
            MANUAL_SPELL(spell_fracture);
            break;
        case SPELL_HEAVENS_GATE:
            MANUAL_SPELL(spell_heavens_gate);
            break;
        case SPELL_HELLS_GATE:
            MANUAL_SPELL(spell_hells_gate);
            break;
        case SPELL_HYSTERIA:
            MANUAL_SPELL(spell_fear);
            break;
        case SPELL_ICE_DAGGER:
            MANUAL_SPELL(spell_ice_dagger);
            break;
        case SPELL_ICE_DARTS:
            MANUAL_SPELL(spell_ice_darts);
            break;
        case SPELL_IDENTIFY:
            MANUAL_SPELL(spell_identify);
            break;
        case SPELL_ILLUMINATION:
            MANUAL_SPELL(spell_illumination);
            break;
        case SPELL_ILLUSORY_WALL:
            MANUAL_SPELL(spell_magical_wall);
            break;
        case SPELL_IMMOLATE:
            MANUAL_SPELL(spell_immolate);
            break;
        case SPELL_ISOLATION:
            MANUAL_SPELL(spell_isolation);
            break;
        case SPELL_LIGHTNING_BREATH:
            MANUAL_SPELL(spell_lightning_breath);
            break;
        case SPELL_LOCATE_OBJECT:
            MANUAL_SPELL(spell_locate_object);
            break;
        case SPELL_MAGIC_MISSILE:
            MANUAL_SPELL(spell_magic_missile);
            break;
        case SPELL_MAJOR_PARALYSIS:
            MANUAL_SPELL(spell_major_paralysis);
            break;
        case SPELL_MINOR_CREATION:
            MANUAL_SPELL(spell_minor_creation);
            break;
        case SPELL_MOONBEAM:
            MANUAL_SPELL(spell_moonbeam);
            break;
        case SPELL_MOONWELL:
            MANUAL_SPELL(spell_moonwell);
            break;
        case SPELL_PHOSPHORIC_EMBERS:
            MANUAL_SPELL(spell_phosphoric_embers);
            break;
        case SPELL_PLANE_SHIFT:
            MANUAL_SPELL(spell_plane_shift);
            break;
        case SPELL_PRESERVE:
            MANUAL_SPELL(spell_preserve);
            break;
        case SPELL_PYRE:
            MANUAL_SPELL(spell_pyre);
            break;
        case SPELL_RAIN:
            MANUAL_SPELL(spell_rain);
            break;
        case SPELL_RELOCATE:
            MANUAL_SPELL(spell_relocate);
            break;
        case SPELL_REMOVE_CURSE:
            MANUAL_SPELL(spell_remove_curse);
            break;
        case SPELL_REMOVE_PARALYSIS:
            MANUAL_SPELL(spell_remove_paralysis);
            break;
        case SPELL_RESURRECT:
            MANUAL_SPELL(spell_resurrect);
            break;
        case SPELL_SHIFT_CORPSE:
            MANUAL_SPELL(spell_shift_corpse);
            break;
        case SPELL_SOUL_TAP:
            MANUAL_SPELL(spell_soul_tap);
            break;
        case SPELL_SPIRIT_ARROWS:
            MANUAL_SPELL(spell_spirit_arrows);
            break;
        case SPELL_SUMMON:
            MANUAL_SPELL(spell_summon);
            break;
        case SPELL_SUMMON_CORPSE:
            MANUAL_SPELL(spell_summon_corpse);
            break;
        case SPELL_TELEPORT:
            MANUAL_SPELL(spell_teleport);
            break;
        case SPELL_VENTRILOQUATE:
            MANUAL_SPELL(spell_ventriloquate);
            break;
        case SPELL_WIZARD_EYE:
            MANUAL_SPELL(spell_wizard_eye);
            break;
        case SPELL_WALL_OF_ICE:
            MANUAL_SPELL(spell_magical_wall);
            break;
        case SPELL_WALL_OF_STONE:
            MANUAL_SPELL(spell_magical_wall);
            break;
        case SPELL_WANDERING_WOODS:
            MANUAL_SPELL(spell_wandering_woods);
            break;
        case SPELL_WORD_OF_COMMAND:
            MANUAL_SPELL(spell_word_of_command);
            break;
        case SPELL_WORD_OF_RECALL:
            MANUAL_SPELL(spell_recall);
            break;
        case SPELL_WORLD_TELEPORT:
            MANUAL_SPELL(spell_world_teleport);
            break;

        case CHANT_APOCALYPTIC_ANTHEM:
            MANUAL_SPELL(chant_apocalyptic_anthem);
            break;
        case CHANT_IVORY_SYMPHONY:
            MANUAL_SPELL(chant_ivory_symphony);
            break;
        case CHANT_PEACE:
            MANUAL_SPELL(chant_peace);
            break;
        }
        /* Check for death caused during manual spells */
        if (cvict && DECEASED(cvict))
            return imp_skill;
    }

    /* Violent spells cause fights. */
    if (SINFO.violent && caster && cvict && attack_ok(cvict, caster, FALSE))
        set_fighting(cvict, caster, FALSE);

    return imp_skill;
}

/* Random messages to be sent to the room when a scroll is recited at a mob. */
/* These are used when the spheres of the spells don't tell us what message
 * to send. */
char *scroll_mob_msg[] = {"The writing on $p glows faintly as $n recites it at $N.",
                          "$n turns toward $N and recites $p.", "$n faces $N and recites $p.", NULL};

/* Choose randomly among the strings in scroll_mob_msg */
char *random_scroll_mob_msg(void) {
    char *msg = "$n recites $p at $N."; /* Default message */
    int i = 0;
    for (; scroll_mob_msg[i]; i++) {
        if (number(1, i) == 1)
            msg = scroll_mob_msg[i];
    }
    return msg;
}

/* Choose a message according to the sphere of spell(s) cast. */
char *get_scroll_mob_msg(int spell1, int spell2, int spell3) {

    /* Select one of the passed spells at random. Its sphere will be used
     * to determine the message. */

    int chosenspell = spell1;

    if (spell2 >= 1 && spell2 <= TOP_SKILL_DEFINE && number(1, 2) == 1)
        chosenspell = spell2;

    if (spell3 >= 1 && spell3 <= TOP_SKILL_DEFINE && number(1, 3) == 1)
        chosenspell = spell3;

    switch (skills[chosenspell].sphere) {
    case SKILL_SPHERE_FIRE:
        return "$n faces $N and recites $p, which is consumed in a burst of flames.";
    case SKILL_SPHERE_WATER:
        return "$n faces $N and recites $p, which dissolves into paste.";
    case SKILL_SPHERE_EARTH:
        return "$n faces $N and recites $p, which crumbles to dust.";
    default:
        return random_scroll_mob_msg();
    }
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * For reference, object values 0-3:
 * staff  - [0] skill [1] max charges [2] num charges [3] spell num
 * wand   - [0] skill [1] max charges [2] num charges [3] spell num
 * scroll - [0] skill [1] spell num   [2] spell num   [3] spell num
 * potion - [0] skill [1] spell num   [2] spell num   [3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */

#define SCROLL_FAILURE_NONE 0
#define SCROLL_FAILURE_NOTSELF 1
#define SCROLL_FAILURE_ONLYSELF 2

void mag_objectmagic(struct char_data *ch, struct obj_data *obj, char *argument) {
    int i, target_status, spellnum;
    struct char_data *tch = NULL, *next_tch;
    struct obj_data *tobj = NULL;
    int scroll_failure, scroll_success;
    char *misc = NULL;
    char *actmsg;

    one_argument(argument, arg);

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_STAFF:
        act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
        if (obj->action_description && strncmp(obj->action_description, "Nothing.", 8))
            act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
        else
            act("$n taps $p three times on the ground.", FALSE, ch, obj, 0, TO_ROOM);

        if (GET_OBJ_VAL(obj, VAL_STAFF_CHARGES_LEFT) <= 0) {
            act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
            act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
        } else {
            GET_OBJ_VAL(obj, VAL_STAFF_CHARGES_LEFT)--;
            WAIT_STATE(ch, PULSE_VIOLENCE);
            if (!check_spell_target(GET_OBJ_VAL(obj, VAL_STAFF_SPELL), ch, NULL, NULL))
                return;
            for (tch = world[ch->in_room].people; tch; tch = next_tch) {
                next_tch = tch->next_in_room;
                if (ch == tch)
                    continue;
                ch->casting.spell = 0;
                if (GET_OBJ_VAL(obj, VAL_STAFF_LEVEL))
                    call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, VAL_STAFF_SPELL), GET_OBJ_VAL(obj, VAL_STAFF_LEVEL),
                               CAST_STAFF);
                else
                    call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, VAL_STAFF_SPELL), DEFAULT_STAFF_LVL, CAST_STAFF);
            }
        }
        break;
    case ITEM_WAND:
        spellnum = GET_OBJ_VAL(obj, VAL_WAND_SPELL);

        if (find_spell_target(spellnum, ch, arg, &target_status, &tch, &tobj)) {

            if (tch) {
                if (tch == ch) {
                    if (SINFO.targets & TAR_NOT_SELF) {
                        send_to_char("You cannot cast this spell upon yourself.\r\n", ch);
                        return;
                    } else {
                        act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
                        if (obj->action_description && strncmp(obj->action_description, "Nothing.", 8))
                            act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
                        else
                            act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
                    }
                } else {
                    act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);

                    /* For the sake of distance spells (e.g., moonwell) where
                     * the target may not be in the same room, there is no
                     * reason to give the name of the target to observers... */

                    if (ch->in_room == tch->in_room) {
                        if (obj->action_description && strncmp(obj->action_description, "Nothing.", 8))
                            act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
                        else {
                            act("$n points $p at $N.", TRUE, ch, obj, tch, TO_NOTVICT);
                            act("$n points $p at you.", TRUE, ch, obj, tch, TO_VICT);
                        }
                    } else {
                        act("$n points $p at nothing in particular.", TRUE, ch, obj, 0, TO_ROOM);
                    }
                }
            } else if (tobj) {
                act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
                if (ch->in_room == tobj->in_room) {
                    if (obj->action_description && strncmp(obj->action_description, "Nothing.", 8))
                        act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
                    else
                        act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
                } else {
                    act("$n points $p at nothing in particular.", TRUE, ch, obj, 0, TO_ROOM);
                }
            } else {
                act("You wave $p in the air.", TRUE, ch, obj, 0, TO_CHAR);
                act("$n waves $p in the air.", TRUE, ch, obj, 0, TO_ROOM);
            }
        } else if (*arg) {
            sprintf(buf, "You can't see any %s here.\r\n", arg);
            send_to_char(buf, ch);
            return;
        } else {
            act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
            return;
        }

        if (GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT) <= 0) {
            act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
            act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
            return;
        }

        if (!check_spell_target(spellnum, ch, tch, tobj))
            return;

        GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT)--;
        WAIT_STATE(ch, PULSE_VIOLENCE);

        misc = strdup(arg);
        if (ch->casting.misc)
            free(ch->casting.misc);
        ch->casting.misc = misc;
        ch->casting.spell = 0;
        if (GET_OBJ_VAL(obj, 0))
            call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, VAL_WAND_SPELL), GET_OBJ_VAL(obj, VAL_WAND_LEVEL), CAST_WAND);
        else
            call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, VAL_WAND_SPELL), DEFAULT_WAND_LVL, CAST_WAND);
        break;
    case ITEM_SCROLL:
        /* A scroll has multiple spells, and therefore multiple
         * target opportunities.
         *
         * If any of the spells can be legally cast, given the
         * caster's target choice, the scroll will be used up and
         * no error message will be sent.
         *
         * However, if none can be cast, an error message about
         * improper targeting will be sent based on the first
         * spell in the scroll.
         */

        scroll_failure = SCROLL_FAILURE_NONE;
        scroll_success = FALSE;

        /* Attempt to cast each spell. */
        for (i = VAL_SCROLL_SPELL_1; i <= VAL_SCROLL_SPELL_3; i++) {
            spellnum = GET_OBJ_VAL(obj, i);
            if (spellnum >= 1 && spellnum <= TOP_SKILL_DEFINE &&
                find_spell_target(spellnum, ch, arg, &target_status, &tch, &tobj)) {
                if (SINFO.targets & TAR_SELF_ONLY && ch != tch && GET_LEVEL(ch) < LVL_GOD) {
                    if (scroll_failure == SCROLL_FAILURE_NONE)
                        scroll_failure = SCROLL_FAILURE_ONLYSELF;
                } else if (SINFO.targets & TAR_NOT_SELF && ch == tch) {
                    if (scroll_failure == SCROLL_FAILURE_NONE)
                        scroll_failure = SCROLL_FAILURE_NOTSELF;
                } else {

                    if (!scroll_success) {
                        scroll_success = TRUE;
                        act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
                        /* Decide what message to send to the room. */
                        if (obj->action_description && strncmp(obj->action_description, "Nothing.", 8))
                            /* The scroll has a specific action desription message. */
                            act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
                        else if (tch) {
                            /* A mobile target */
                            actmsg = get_scroll_mob_msg(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1),
                                                        GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_2),
                                                        GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_3));
                            act(actmsg, FALSE, ch, obj, tch, TO_ROOM);
                        } else if (tobj) {
                            /* An object target */
                            act("$n places $s hand on $P and recites $p.", FALSE, ch, obj, tobj, TO_ROOM);
                        } else {
                            /* No target */
                            act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);
                        }
                        WAIT_STATE(ch, PULSE_VIOLENCE);
                        misc = strdup(arg);
                        if (ch->casting.misc)
                            free(ch->casting.misc);
                        ch->casting.misc = misc;
                        ch->casting.spell = 0;
                    }
                    call_magic(ch, tch, tobj, spellnum, GET_OBJ_VAL(obj, VAL_SCROLL_LEVEL), CAST_SCROLL);
                }
            }
        }

        if (scroll_success) {
            /* If any spells were cast, we're done with the scroll. */
            if (obj != NULL)
                extract_obj(obj);
        } else if (scroll_failure == SCROLL_FAILURE_NOTSELF) {
            send_to_char("You cannot cast this spell on yourself.\r\n", ch);
            return;
        } else if (scroll_failure == SCROLL_FAILURE_ONLYSELF) {
            send_to_char("You can only cast this spell on yourself.\r\n", ch);
            return;
        } else if (!(*arg)) {
            act("What do you want to recite $p at?", FALSE, ch, obj, NULL, TO_CHAR);
            return;
        } else {
            sprintf(buf, "You can't see any %s here.\r\n", arg);
            send_to_char(buf, ch);
            return;
        }
        break;
    case ITEM_POTION:
        tch = ch;
        act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
        if (obj->action_description && strncmp(obj->action_description, "Nothing.", 8))
            act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
        else
            act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);

        WAIT_STATE(ch, PULSE_VIOLENCE);
        ch->casting.spell = 0;
        for (i = VAL_POTION_SPELL_1; i <= VAL_POTION_SPELL_3; i++)
            if (!(call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i), GET_OBJ_VAL(obj, VAL_POTION_LEVEL), CAST_POTION)))
                break;

        if (obj != NULL)
            extract_obj(obj);
        break;
    default:
        log("SYSERR: Unknown object_type in mag_objectmagic");
        break;
    }
}

bool check_spell_stance_position(struct char_data *ch, int spellnum) {
    if (GET_STANCE(ch) < STANCE_ALERT) {
        switch (GET_STANCE(ch)) {
        case STANCE_SLEEPING:
            send_to_char("You dream about great magical powers.\r\n", ch);
            break;
        case STANCE_RESTING:
            send_to_char("You cannot concentrate while resting.\r\n", ch);
            break;
        default:
            send_to_char("You can't do much of anything like this!\r\n", ch);
            break;
        }
        return 0;
    }

    if (GET_POS(ch) < SINFO.minpos) {
        switch (SINFO.minpos) {
        case POS_FLYING:
            cprintf(ch, "You must be flying to cast this spell.\r\n");
            break;
        case POS_STANDING:
            cprintf(ch, "You can only cast this spell while standing.\r\n");
            break;
        case POS_KNEELING:
            cprintf(ch, "You must at least be kneeling to cast this spell.\r\n");
            break;
        case POS_SITTING:
            cprintf(ch, "You'll have to at least sit up to cast this spell.\r\n");
            break;
        default:
            cprintf(ch, "Sorry, you can't cast this spell.  Who knows why.\r\n");
            break;
        }
        return 0;
    }

    return 1;
}

/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 *
 * Return value: CAST_RESULT_ flags.
 */

int cast_spell(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int spellnum) {
    char buf[256];
    int sphere, cresult = 0;

    if (spellnum < 0 || spellnum > TOP_SKILL_DEFINE) {
        sprintf(buf, "SYSERR: cast_spell trying to call spellnum %d\n", spellnum);
        log(buf);
        return 0;
    }

    if (GET_STANCE(ch) == STANCE_FIGHTING && !SINFO.fighting_ok && GET_LEVEL(ch) <= LVL_IMMORT) {
        send_to_char("Impossible!  You can't concentrate enough!\r\n", ch);
        return 0;
    }

    if (!check_spell_stance_position(ch, spellnum))
        return 0;

    if (EFF_FLAGGED(ch, EFF_CHARM) && (ch->master == tch) && SINFO.violent) {
        send_to_char("You are afraid you might hurt your master!\r\n", ch);
        return 0;
    }
    if (!check_spell_target(spellnum, ch, tch, tobj))
        return 0;
    if (IS_SET(SINFO.routines, MAG_GROUP) && !IS_GROUPED(ch)) {
        send_to_char("You can't cast this spell if you're not in a group!\r\n", ch);
        return 0;
    }
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        switch (GET_CLASS(ch)) {
        case CLASS_PRIEST:
        case CLASS_PALADIN:
        case CLASS_RANGER:
            if (!IS_GOOD(ch)) {
                send_to_char("Your deity has removed your holy powers!\r\n", ch);
                return 0;
            }
            break;
        case CLASS_DIABOLIST:
        case CLASS_ANTI_PALADIN:
            if (!IS_EVIL(ch)) {
                send_to_char("Your deity has removed your unholy powers!\r\n", ch);
                return 0;
            }
            break;
        }
    }
    end_chant(ch, tch, tobj, spellnum);

    sphere = skills[spellnum].sphere;
    cresult = call_magic(ch, tch, tobj, spellnum, GET_SKILL(ch, sphere), CAST_SPELL);

    /* Prevent skill improvement for spells cast upon illusions */
    if (tch && MOB_FLAGGED(tch, MOB_ILLUSORY))
        cresult &= ~CAST_RESULT_IMPROVE;

    if (cresult & CAST_RESULT_IMPROVE)
        improve_skill(ch, sphere);
    return cresult;
}

int chant(struct char_data *ch, struct char_data *tch, struct obj_data *obj, int chantnum) {
    int cresult;

    if (chantnum < 0 || chantnum > TOP_SKILL_DEFINE) {
        sprintf(buf, "SYSERR: chant trying to call chantnum %d", chantnum);
        log(buf);
        return 0;
    }

    if (GET_STANCE(ch) == STANCE_FIGHTING && !skills[chantnum].fighting_ok && GET_LEVEL(ch) <= LVL_IMMORT) {
        send_to_char("Impossible!  You can't concentrate enough!\r\n", ch);
        return 0;
    }

    if (GET_STANCE(ch) < STANCE_ALERT) {
        switch (GET_STANCE(ch)) {
        case STANCE_SLEEPING:
            send_to_char("You dream about great magical powers.\r\n", ch);
            break;
        case STANCE_RESTING:
            send_to_char("You cannot concentrate while resting.\r\n", ch);
            break;
        default:
            send_to_char("You can't do much of anything like this!\r\n", ch);
            break;
        }
        return 0;
    }

    if (GET_POS(ch) < skills[chantnum].minpos) {
        switch (skills[chantnum].minpos) {
        case POS_FLYING:
            cprintf(ch, "You must be flying to chant this song.\r\n");
            break;
        case POS_STANDING:
            cprintf(ch, "You can only chant this song while standing.\r\n");
            break;
        case POS_KNEELING:
            cprintf(ch, "You must at least be kneeling to chant this song.\r\n");
            break;
        case POS_SITTING:
            cprintf(ch, "You'll have to at least sit up to chant this song.\r\n");
            break;
        default:
            cprintf(ch, "Sorry, you can't chant this song.  Who knows why.\r\n");
            break;
        }
        return 0;
    }

    if (!check_spell_target(chantnum, ch, tch, obj))
        return 0;
    if (EFF_FLAGGED(ch, EFF_CHARM) && (ch->master == tch) && skills[chantnum].violent) {
        send_to_char("You are afraid you might hurt your master!\r\n", ch);
        return 0;
    }
    if (IS_SET(skills[chantnum].routines, MAG_GROUP) && !IS_GROUPED(ch)) {
        send_to_char("You can't chant this song if you're not in a group!\r\n", ch);
        return 0;
    }

    act("$n begins chanting in a deep voice.", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("You begin chanting in a deep voice.\r\n", ch);

    if (number(0, 101) > 50 + GET_SKILL(ch, SKILL_CHANT)) {
        send_to_char("You choke and grunt a raspy wail of pain.\r\n", ch);
        act("$n chokes on $s tears and coughs a raspy grunt.", TRUE, ch, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE;
    }

    cresult = call_magic(ch, tch, obj, chantnum, GET_SKILL(ch, SKILL_CHANT), CAST_CHANT);

    /* Prevent skill improvement for chants against illusions */
    if (IS_SET(cresult, CAST_RESULT_IMPROVE) && tch && MOB_FLAGGED(tch, MOB_ILLUSORY))
        return cresult & ~CAST_RESULT_IMPROVE;

    return cresult;
}

/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, and passes control to cast_spell().
 *
 * this function is a mirror of mob_cast for npc's.  These two could easily
 * be combined once the target issue is resolved.
 * if you change this, please reflect any changes in mob_cast
 */
ACMD(do_cast) {
    struct char_data *tch = NULL;
    struct obj_data *tobj = NULL;
    int spellnum, target = 0;
    int target_status = TARGET_NULL;

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        send_to_char("Your lips move, but no sound forms.\r\n", ch);
        return;
    }

    if (CASTING(ch)) {
        send_to_char("But you are already casting a spell!\r\n", ch);
        return;
    }

    if (GET_STANCE(ch) < STANCE_ALERT) {
        send_to_char("You are too relaxed.\r\n", ch);
        return;
    }

    if (subcmd == SCMD_CHANT) {
        if (IS_NPC(ch)) {
            send_to_char("NPC's can't chant!\r\n", ch);
            return;
        }
        if (!GET_SKILL(ch, SKILL_CHANT)) {
            send_to_char("You imitate a monk chanting...Monkey see, monkey do?\r\n", ch);
            return;
        }
        if (GET_LEVEL(ch) < LVL_GOD && GET_COOLDOWN(ch, CD_CHANT)) {
            int hours = GET_COOLDOWN(ch, CD_CHANT) / (1 MUD_HR);
            if (hours == 1)
                strcpy(buf1, "hour");
            else
                sprintf(buf1, "%d hours", hours);
            sprintf(buf,
                    "You're still out of breath from chanting recently!\r\n"
                    "You'll be able to chant again in another %s.\r\n",
                    buf1);
            send_to_char(buf, ch);
            return;
        }
    }

    argument = delimited_arg(argument, arg, '\'');

    if (!*arg) {
        if (subcmd == SCMD_CHANT)
            send_to_char("What do you want to chant?\r\n", ch);
        else if (subcmd == SCMD_SING)
            send_to_char("What song do you want to sing?\r\n", ch);
        else
            send_to_char("Cast what where?\r\n", ch);
        return;
    }

    if (subcmd == SCMD_CHANT) {
        spellnum = find_chant_num(arg);
        if (!IS_CHANT(spellnum)) {
            send_to_char("Chant what?!?\r\n", ch);
            return;
        }
    } else if (subcmd == SCMD_SING) {
        spellnum = find_song_num(arg);
        if (!IS_SONG(spellnum)) {
            send_to_char("Sing what?!?\r\n", ch);
            return;
        }
    } else {
        spellnum = find_spell_num(arg);
        if (!IS_SPELL(spellnum)) {
            send_to_char("Cast what?!?\r\n", ch);
            return;
        }
    }

    /* Can the caster actually cast this spell? */
    if (GET_LEVEL(ch) < SINFO.min_level[(int)GET_CLASS(ch)] || !GET_SKILL(ch, spellnum)) {
        if (subcmd == SCMD_CHANT)
            send_to_char("You do not know that chant!\r\n", ch);
        else if (subcmd == SCMD_SING)
            send_to_char("You do not know that song!\r\n", ch);
        else
            send_to_char("You do not know that spell!\r\n", ch);
        return;
    }

    /* Is the spell memorized?  PC's only. */
    if (subcmd == SCMD_CAST && !IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT && !check_spell_memory(ch, spellnum)) {
        send_to_char("You do not have that spell memorized!\r\n", ch);
        return;
    }

    if (!check_spell_stance_position(ch, spellnum))
        return;

    /* Find the target */
    skip_spaces(&argument);
    one_argument(argument, arg);

    target = find_spell_target(spellnum, ch, arg, &target_status, &tch, &tobj);

    if (!target) {
        if (*arg) {
            if (subcmd == SCMD_CHANT)
                send_to_char("Cannot find the target of your chant!\r\n", ch);
            else if (subcmd == SCMD_SING)
                send_to_char("Cannot find the target of your song!\r\n", ch);
            else
                send_to_char("Cannot find the target of your spell!\r\n", ch);
        } else {
            if (subcmd == SCMD_CHANT)
                send_to_char("To whom should the chant be sung?\r\n", ch);
            else if (subcmd == SCMD_SING)
                send_to_char("To whom should the song be sung?\r\n", ch);
            else {
                sprintf(buf, "Upon %s should the spell be cast?\r\n",
                        IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_STRING) ? "what"
                                                                                                       : "whom");
                send_to_char(buf, ch);
            }
        }
        return;
    }

    if (!check_spell_target(spellnum, ch, tch, tobj))
        return;

    /* An injured throat makes it difficult to cast. */
    if (EFF_FLAGGED(ch, EFF_HURT_THROAT) && number(0, MAX_ABILITY_VALUE) > GET_VIEWED_CON(ch)) {
        if (subcmd == SCMD_CHANT) {
            act("$n starts chanting, but stops abruptly, coughing up blood!", FALSE, ch, 0, 0, TO_ROOM);
            cprintf(ch,
                    "You begin chanting, but your throat causes you to "
                    "cough up blood!\r\n");
        } else if (subcmd == SCMD_SING) {
            act("$n starts singing, but stops abruptly, coughing up blood!", FALSE, ch, 0, 0, TO_ROOM);
            cprintf(ch,
                    "You begin singing, but your throat causes you to "
                    "cough up blood!\r\n");
        } else {
            act("$n starts casting, but stops abruptly, coughing up blood!", FALSE, ch, 0, 0, TO_ROOM);
            cprintf(ch,
                    "You begin casting, but your throat causes you to "
                    "cough up blood!\r\n");
        }
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return;
    }

    /* If this is an aggro cast, make the caster become visible. */
    if (target && SINFO.violent)
        aggro_lose_spells(ch);

    /*
     * Set the character to casting and setup the casting structure for
     * complete_spell().
     */
    ch->casting.spell = spellnum;
    ch->casting.tch = tch;
    ch->casting.obj = tobj;

    /* Targets should only remember the caster for cast-type actions
     * that take time, like spells. If/when singing is implemented,
     * its targets should remember the caster if and only if the
     * singing is an event, at whose completion the targets will be
     * told to forget. Chanting is instantaneous, so its targets should
     * not remember the caster. */
    if (subcmd == SCMD_CAST)
        targets_remember_caster(ch);

    if (ch->casting.misc)
        free(ch->casting.misc);
    ch->casting.misc = strdup(argument);
    ch->casting.target_status = target_status;

    ch->casting.casting_time = SINFO.cast_time;

    if (subcmd == SCMD_CHANT) {
        int cresult = chant(ch, tch, tobj, spellnum);
        if (IS_SET(cresult, CAST_RESULT_IMPROVE))
            improve_skill(ch, SKILL_CHANT);
        if (IS_SET(cresult, CAST_RESULT_CHARGE)) {
            SET_COOLDOWN(ch, CD_CHANT, (7 - wis_app[GET_WIS(ch)].bonus) MUD_HR);
            WAIT_STATE(ch, PULSE_VIOLENCE * 1.5);
        }
    } else if (subcmd == SCMD_SING) {

    } else {
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_CASTING);

        /* Chance to quick chant. */
        if (number(1, 102) < GET_SKILL(ch, SKILL_QUICK_CHANT))
            ch->casting.casting_time /= 2;
        improve_skill(ch, SKILL_QUICK_CHANT);

        /* Show chant messages. */
        start_chant(ch);
        WAIT_STATE(ch, ch->casting.casting_time * PULSE_VIOLENCE / 2);

        /* Gods instacast.  Start chant and then stop casting in order to
         * display correct message. */
        if (GET_LEVEL(ch) >= LVL_GOD) {
            STOP_CASTING(ch);
            complete_spell(ch);
        } else {
            event_create(EVENT_CASTING, casting_handler, ch, FALSE, &(ch->events), PULSE_VIOLENCE / 2);
        }
    }
}

bool check_spell_target(int spellnum, struct char_data *ch, struct char_data *tch, struct obj_data *tobj) {
    const char *verb = IS_CHANT(spellnum) ? "chant" : "cast";
    const char *noun = IS_CHANT(spellnum) ? "song" : "spell";

    if (IS_SET(SINFO.targets, TAR_SELF_ONLY) && tch != ch && GET_LEVEL(ch) < LVL_GOD) {
        cprintf(ch, "You can only %s this %s %s yourself!\r\n", verb, noun, IS_CHANT(spellnum) ? "to" : "upon");
        return FALSE;
    }
    if (IS_SET(SINFO.targets, TAR_NOT_SELF) && tch == ch) {
        cprintf(ch, "You cannot %s this %s %s yourself!\r\n", verb, noun, IS_CHANT(spellnum) ? "to" : "upon");
        return FALSE;
    }
    if (IS_SET(SINFO.targets, TAR_OUTDOORS) && CH_INDOORS(ch)) {
        cprintf(ch, "This area is too enclosed to %s that %s!\r\n", verb, noun);
        return FALSE;
    }
    if (IS_SET(SINFO.targets, TAR_GROUND) && !CH_INDOORS(ch) && !QUAKABLE(IN_ROOM(ch))) {
        cprintf(ch, "You must be on solid ground to %s that %s!\r\n", verb, noun);
        return FALSE;
    }
    if (IS_SET(SINFO.targets, TAR_NIGHT_ONLY) && SUN(IN_ROOM(ch)) == SUN_LIGHT) {
        cprintf(ch, "You cannot %s this %s during the day!\r\n", verb, noun);
        return FALSE;
    }
    if (IS_SET(SINFO.targets, TAR_DAY_ONLY) && SUN(IN_ROOM(ch)) == SUN_DARK) {
        cprintf(ch, "You can only %s this %s during the day!\r\n", verb, noun);
        return FALSE;
    }
    return TRUE;
}

/* FIND_SPELL_TARGET
 *
 * Given spell, caster, and target string, locate the actual targetted object,
 * based on the spell's allowable target locations.
 *
 * INPUTS
 *
 *   spellnum         Spell number.
 *   ch               The caster.
 *   t                String provided by the caster to indicate the target.
 *
 * OUTPUTS
 *
 *   (return value)   Whether a target was found.
 *   target_status    Type of location where the target was found.
 *   tch              A char_data target.
 *   tobj             An object target.
 */

bool find_spell_target(int spellnum, struct char_data *ch, char *t, int *target_status, struct char_data **tch,
                       struct obj_data **tobj) {

    struct find_context context = find_vis_by_name(ch, t);

    *tch = NULL;
    *tobj = NULL;
    *target_status = TARGET_NULL;

    if (IS_SET(SINFO.targets, TAR_IGNORE)) {
        *target_status = TARGET_ALL_ROOM;
        return TRUE;
    } else if (*t) {
        if ((IS_SET(SINFO.targets, TAR_CHAR_ROOM))) {
            if ((*tch = find_char_in_room(&world[ch->in_room], context)) != NULL) {
                *target_status = TARGET_IN_ROOM;
                return TRUE;
            }
        }
        if (IS_SET(SINFO.targets, TAR_CHAR_WORLD))
            if ((*tch = find_char_around_char(ch, context))) {
                *target_status = TARGET_IN_WORLD;
                return TRUE;
            }
        if (IS_SET(SINFO.targets, TAR_OBJ_INV))
            if ((*tobj = find_obj_in_list(ch->carrying, context))) {
                *target_status = TARGET_IN_INV;
                return TRUE;
            }
        if (IS_SET(SINFO.targets, TAR_OBJ_EQUIP)) {
            if ((*tobj = find_obj_in_eq(ch, NULL, context))) {
                *target_status = TARGET_EQUIP;
                return TRUE;
            }
        }
        if (IS_SET(SINFO.targets, TAR_OBJ_ROOM))
            if ((*tobj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, t)))) {
                *target_status = TARGET_IN_ROOM;
                return TRUE;
            }
        if (IS_SET(SINFO.targets, TAR_OBJ_WORLD))
            if ((*tobj = find_obj_around_char(ch, find_vis_by_name(ch, t)))) {
                *target_status = TARGET_IN_WORLD;
                return TRUE;
            }
        if (IS_SET(SINFO.targets, TAR_STRING)) {
            *target_status = TARGET_STRING;
            return TRUE;
        }

    } else { /* if target string is empty */
        if (IS_SET(SINFO.targets, TAR_FIGHT_SELF))
            if (FIGHTING(ch) != NULL) {
                *tch = ch;
                *target_status = TARGET_SELF;
                return TRUE;
            }
        if (IS_SET(SINFO.targets, TAR_FIGHT_VICT))
            if (FIGHTING(ch) != NULL) {
                *tch = FIGHTING(ch);
                *target_status = TARGET_FIGHTING;
                return TRUE;
            }
        /* if no target specified, and the spell isn't violent, default to self */
        if (IS_SET(SINFO.targets, TAR_CHAR_ROOM) && !SINFO.violent && !IS_SET(SINFO.targets, TAR_NOT_SELF)) {
            *tch = ch;
            *target_status = TARGET_SELF;
            return TRUE;
        }
    }
    return FALSE;
}

void complete_spell(struct char_data *ch) {

    /* Remove links from target to caster */
    if (ch->casting.obj)
        obj_forget_caster(ch->casting.obj, ch);
    if (ch->casting.tch)
        char_forget_caster(ch->casting.tch, ch);

    /* Insanity really makes it hard to cast spells. */
    if (EFF_FLAGGED(ch, EFF_INSANITY) && !mag_savingthrow(ch, SAVING_PARA)) {
        act("$n babbles a bit as a strand of drool drips down $s chin.", TRUE, ch, 0, 0, TO_ROOM);
        act("Your mind is not in any condition to cast spells.", FALSE, ch, 0, 0, TO_CHAR);
        act("&1$n&1 stops chanting abruptly!&0", TRUE, ch, 0, 0, TO_ROOM);
        return;
    }

    if (GET_LEVEL(ch) < LVL_GOD && ch->casting.spell != SPELL_VENTRILOQUATE) {
        send_to_char("You complete your spell.\r\n", ch);
        act("$n completes $s spell...", FALSE, ch, 0, 0, TO_ROOM);
    }

    if (cast_spell(ch, ch->casting.tch, ch->casting.obj, ch->casting.spell) & CAST_RESULT_CHARGE) {

        /* Lag the caster */
        WAIT_STATE(ch, PULSE_VIOLENCE);

        /* Erase memorized spell */
        charge_mem(ch, ch->casting.spell);

        if (IS_NPC(ch) && skills[ch->casting.spell].violent && ch->casting.tch && IS_NPC(ch->casting.tch) &&
            !FIGHTING(ch->casting.tch) && GET_STANCE(ch->casting.tch) >= STANCE_RESTING && number(0, 4)) {
            attack(ch->casting.tch, ch);
            remember(ch->casting.tch, ch);
        }
    }
}

/*
 * This function is a copy of do_cast, but it is for NPCs.  If you make
 * any changes here, make them to do_cast also.
 * Returns TRUE if the mob started casting and FALSE otherwise.
 */
bool mob_cast(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int spellnum) {
    int target_status = TARGET_NULL;
    int circle = SPELL_CIRCLE(ch, spellnum);
    ACMD(do_stand);
    struct char_data *targ_ch = NULL;
    struct obj_data *targ_obj = NULL;

    if (!IS_NPC(ch))
        return FALSE;

    if (EFF_FLAGGED(ch, EFF_SILENCE))
        return FALSE;

    if (CASTING(ch))
        return FALSE;

    if (!AWAKE(ch))
        return FALSE;

    if (GET_MOB_WAIT(ch) <= 0 && GET_POS(ch) < POS_STANDING)
        do_stand(ch, "", 0, 0);

    if (GET_STANCE(ch) < STANCE_ALERT)
        return FALSE;

    if (!IS_SPELL(spellnum))
        return FALSE;

    if (GET_LEVEL(ch) < SINFO.min_level[(int)GET_CLASS(ch)])
        return FALSE;

    if (!GET_SKILL(ch, spellnum))
        return FALSE;

    if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC))
        return FALSE;

    /* Check alignment restrictions for clerical classes. */
    if ((GET_CLASS(ch) == CLASS_DIABOLIST && !IS_EVIL(ch)) || (GET_CLASS(ch) == CLASS_PRIEST && !IS_GOOD(ch)) ||
        (GET_CLASS(ch) == CLASS_PALADIN && !IS_GOOD(ch)) || (GET_CLASS(ch) == CLASS_RANGER && !IS_GOOD(ch)) ||
        (GET_CLASS(ch) == CLASS_ANTI_PALADIN && !IS_EVIL(ch)))
        return FALSE;

    /* Check mob's position and stance. */
    if (SINFO.minpos > GET_POS(ch) || GET_STANCE(ch) < STANCE_ALERT ||
        (!SINFO.fighting_ok && GET_STANCE(ch) == STANCE_FIGHTING))
        return FALSE;

    /* Check if mob has slots in this spell's circle in its spell bank */
    if (GET_MOB_SPLBANK(ch, circle) <= 0)
        return FALSE;

    /* Find the target */
    if (IS_SET(SINFO.targets, TAR_IGNORE))
        target_status = TARGET_ALL_ROOM;
    else if (tch && IS_SET(SINFO.targets, TAR_CHAR_ROOM) && ch->in_room == tch->in_room) {
        target_status = TARGET_IN_ROOM;
        targ_ch = tch;
    } else if (tch && IS_SET(SINFO.targets, TAR_CHAR_WORLD) && tch->in_room != NOWHERE) {
        target_status = TARGET_IN_WORLD;
        targ_ch = tch;
    } else if (tobj && IS_SET(SINFO.targets, TAR_OBJ_INV) && tobj->carried_by) {
        target_status = TARGET_IN_INV;
        targ_obj = tobj;
    } else if (tobj && IS_SET(SINFO.targets, TAR_OBJ_EQUIP) && tobj->worn_by) {
        target_status = TARGET_EQUIP;
        targ_obj = tobj;
    } else if (tobj && IS_SET(SINFO.targets, TAR_OBJ_ROOM) && ch->in_room == tobj->in_room) {
        target_status = TARGET_IN_ROOM;
        targ_obj = tobj;
    } else if (tobj && IS_SET(SINFO.targets, TAR_OBJ_WORLD)) {
        target_status = TARGET_IN_WORLD;
        targ_obj = tobj;
    } else if (FIGHTING(ch) && IS_SET(SINFO.targets, TAR_FIGHT_SELF)) {
        target_status = TARGET_SELF;
        targ_ch = ch;
    } else if (FIGHTING(ch) && IS_SET(SINFO.targets, TAR_FIGHT_VICT)) {
        target_status = TARGET_FIGHTING;
        targ_ch = FIGHTING(ch);
    } else if (IS_SET(SINFO.targets, TAR_CHAR_ROOM) && !SINFO.violent && !IS_SET(SINFO.targets, TAR_NOT_SELF)) {
        target_status = TARGET_SELF;
        targ_ch = ch;
    } else
        return FALSE;

    if (!check_spell_target(spellnum, ch, tch, tobj))
        return FALSE;

    /* An injured throat makes it difficult to cast. */
    if (EFF_FLAGGED(ch, EFF_HURT_THROAT) && number(0, MAX_ABILITY_VALUE) > GET_VIEWED_CON(ch)) {
        act("$n starts casting, but stops abruptly, coughing up blood!", FALSE, ch, 0, 0, TO_ROOM);
        cprintf(ch,
                "You begin casting, but your throat causes you to "
                "cough up blood!\r\n");
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return TRUE; /* makes caller think we cast a spell so they don't try again
                      */
    }

    /* Reveal hidden/invis/concealed attackers. */
    if (SINFO.violent)
        aggro_lose_spells(ch);

    SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_CASTING);
    ch->casting.spell = spellnum;
    ch->casting.tch = targ_ch;
    ch->casting.obj = targ_obj;
    targets_remember_caster(ch);
    if (ch->casting.misc)
        free(ch->casting.misc);
    if (tch)
        ch->casting.misc = strdup(GET_NAME(tch));
    else if (tobj)
        ch->casting.misc = strdup(GET_OBJ_NAME(tobj));
    else
        ch->casting.misc = strdup("");
    ch->casting.target_status = target_status;

    /* Quick chant for mobs --gurlaek 7/13/1999 */
    if (number(1, 102) < GET_SKILL(ch, SKILL_QUICK_CHANT))
        ch->casting.casting_time = (int)(SINFO.cast_time / 2);
    else
        ch->casting.casting_time = SINFO.cast_time;

    /* Show chant messages. */
    start_chant(ch);
    WAIT_STATE(ch, ch->casting.casting_time * PULSE_VIOLENCE / 2);
    event_create(EVENT_CASTING, casting_handler, ch, FALSE, &(ch->events), 0);
    return TRUE;
}

void start_chant(struct char_data *ch) {
    void garble_text(char *string, int percent);
    struct char_data *gch;
    struct know_spell *tmp, *tmp2;
    char lbuf[256];
    char spellbuf[256];
    char garblebuf[256];
    char namebuf[256];
    int ofs = 0, j, circle, percent;
    bool bad;

    memset(lbuf, 0x0, 256);
    memset(garblebuf, 0x0, 256);

    /* free the caster's old list of people that knew the spell */
    for (tmp = ch->see_spell; tmp; tmp = tmp2) {
        tmp2 = tmp->next;
        free(tmp);
    }
    ch->see_spell = NULL;

    /* Randomly garble the letters. */
    strcpy(lbuf, skills[ch->casting.spell].name);
    garble_text(lbuf, -1);

    /* Change the syllables of the spoken spell */
    while (*(lbuf + ofs))
        for (j = 0; *(syls[j].org); j++)
            if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
                strcat(garblebuf, syls[j].new);
                ofs += strlen(syls[j].org);
            }

    /* Loop through bystanders and see if they recognize the spell */
    for (gch = world[ch->in_room].people; gch; gch = gch->next_in_room) {

        /* Caster gets its own message later, and no message to sleeping. */
        if (gch == ch || !AWAKE(gch))
            continue;

        /* The casting of ventriloquate is generally not heard */
        if (ch->casting.spell == SPELL_VENTRILOQUATE && (GET_LEVEL(gch) < LVL_IMMORT || GET_LEVEL(gch) < GET_LEVEL(ch)))
            continue;

        bad = FALSE;
        memset(namebuf, 0x0, MAX_NAME_LENGTH);
        memset(spellbuf, 0x0, 256);
        percent = -1; /* assume failure at first */

        /* Clerical and arcane magic don't recognize each others spells. */
        if (MEM_MODE(ch) == MEM_MODE(gch)) {
            percent = GET_SKILL(gch, SKILL_KNOW_SPELL); /* now we have a chance */

            /* Find the circle of caster's spell */
            circle = SPELL_CIRCLE(ch, ch->casting.spell);

            /* If bystander does not have that circle, 50% chance to recognize. */
            if (!spells_of_circle[(int)GET_LEVEL(gch)][circle])
                percent /= 2;
            /* If bystander is more than 1 circle down, no chance */
            if (circle > 1 && !spells_of_circle[(int)GET_LEVEL(gch)][circle - 1])
                percent = 0;

            if (number(0, 1))
                improve_skill(gch, SKILL_KNOW_SPELL);
        }

        /* KNOW_SPELL skill check */
        if (number(0, 101) > percent && GET_LEVEL(gch) < LVL_GOD) {
            if (number(0, 100) < 20) {
                /* For really bad rolls, replace the spell with an incorrect one */
                strcpy(spellbuf, skills[bad_guess(ch)].name);
                bad = TRUE;
            } else
                /* Copy the garbled buffer into the spell */
                strcpy(spellbuf, garblebuf);
        } else {
            /* Hey we recognized the spell! */
            strcpy(spellbuf, skills[ch->casting.spell].name);

            /*
             * Create the caster's see_spell list: This is a list of char's who
             * have recognized the spell and will see it clearly when it is cast
             * if they are still in the room.
             */
            CREATE(tmp, struct know_spell, 1);
            tmp->sch = gch;
            tmp->next = ch->see_spell;
            ch->see_spell = tmp;
        }

        /* Is there a Target?  Is it in the room?  Is it not the caster? */
        if (ch->casting.tch && ch->casting.tch->in_room == ch->in_room && ch != ch->casting.tch) {

            /* Intelligence check to determine the target.  If we know the spell we
             * know the target */
            if (number(0, 101) < GET_INT(gch) || !strcmp(spellbuf, skills[ch->casting.spell].name) || (bad) ||
                GET_LEVEL(gch) >= LVL_GOD) {
                if (ch->casting.tch == gch)
                    /* Target is the receiver of the message */
                    sprintf(namebuf, " at &1&bYou&0!!!");
                else
                    /* target is someone else in room */
                    sprintf(namebuf, " at &7&b%s&0", GET_NAME(ch->casting.tch));
            }
        }

        /* Message to bystander */
        sprintf(buf, "$n %s &3&b'%s'&0%s...", (GET_LEVEL(ch) < LVL_GOD) ? "starts casting" : "casts", spellbuf,
                namebuf);
        act(buf, FALSE, ch, 0, gch, TO_VICT);
    }

    /* Message to caster */
    if (GET_LEVEL(ch) < LVL_GOD)
        send_to_char("You start chanting...\r\n", ch);
    else
        send_to_char("You cast your spell...\r\n", ch);
}

/*
 * Display incorrect spell on a very bad KNOW_SPELL roll.
 */
int bad_guess(struct char_data *ch) {

    /*
     * Replace offensive spells with nonoffensive spells,
     * and nonoffensive spells with offensive spells.
     */

    if (MEM_MODE(ch) == MEMORIZE) {
        if (skills[ch->casting.spell].violent)
            return bogus_mage_spells[number(1, 10)];
        else
            return bogus_mage_spells[number(11, 20)];
    } else {
        if (skills[ch->casting.spell].violent)
            return bogus_priest_spells[number(1, 10)];
        else
            return bogus_priest_spells[number(11, 20)];
    }
}

/* Remember someone who's casting a spell at me, so that if I'm
 * destroyed before the spell is completed, the spell can
 * be safely aborted. */
void targets_remember_caster(struct char_data *caster) {
    struct char_data *temp;

    if (caster->casting.obj) {
        /* This should never happen. If this code catches a circular list,
         * it will prevent a crash. The execution time cost of this code should
         * be low, since the number of spells being cast at any person or object
         * should also be low. */
        for (temp = caster->casting.obj->casters; temp; temp = temp->next_caster)
            if (temp == caster) {
                sprintf(buf, "1. CIRCULAR CAST LIST ERROR: %s was already casting at %s!", GET_NAME(caster),
                        caster->casting.obj->short_description);
                mudlog(buf, NRM, LVL_GOD, TRUE);
                return;
            }
        caster->next_caster = caster->casting.obj->casters;
        caster->casting.obj->casters = caster;
    }

    if (caster->casting.tch) {
        for (temp = caster->casting.tch->casters; temp; temp = temp->next_caster)
            if (temp == caster) {
                sprintf(buf, "2. CIRCULAR CAST LIST ERROR: %s was already casting at %s!", GET_NAME(caster),
                        GET_NAME(caster->casting.tch));
                mudlog(buf, NRM, LVL_GOD, TRUE);
                return;
            }
        caster->next_caster = caster->casting.tch->casters;
        caster->casting.tch->casters = caster;
    }
}

/* Forget someone who's casting a spell at you.  This can happen when
 * the spell is complete or has been aborted for any reason. */
void obj_forget_caster(struct obj_data *obj, struct char_data *caster) {
    struct char_data *temp;

    REMOVE_FROM_LIST(caster, obj->casters, next_caster);
}

void char_forget_caster(struct char_data *ch, struct char_data *caster) {
    struct char_data *temp;

    REMOVE_FROM_LIST(caster, ch->casters, next_caster);
}

/* Forget everyone who is casting a spell at you.  This tends to happen
 * when you're removed from the game. */
void obj_forget_casters(struct obj_data *obj) {
    struct char_data *caster, *c;

    for (caster = obj->casters; caster;) {
        /* First make the caster forget what they it was casting its
         * spell upon.  This prevents it from coming back and calling
         * obj_forget_caster() in STOP_CASTING() and messing up our list. */
        caster->casting.obj = NULL;
        STOP_CASTING(caster);
        if (AWAKE(caster)) {
            act("You stop chanting abruptly!", FALSE, caster, 0, 0, TO_CHAR);
            act("$n stops chanting abruptly!", FALSE, caster, 0, 0, TO_ROOM);
        }
        c = caster;
        caster = caster->next_caster;
        c->next_caster = NULL;
    }
    obj->casters = NULL;
}

void char_forget_casters(struct char_data *ch) {
    struct char_data *caster, *c;

    for (caster = ch->casters; caster;) {
        /* First make the caster forget what it was casting its
         * spell upon.  This prevents it from coming back and calling
         * char_forget_caster() in STOP_CASTING() and messing up our list. */
        caster->casting.tch = NULL;
        STOP_CASTING(caster);
        if (AWAKE(caster)) {
            act("You stop chanting abruptly!", FALSE, caster, 0, 0, TO_CHAR);
            act("$n stops chanting abruptly!", FALSE, caster, 0, 0, TO_ROOM);
        }
        c = caster;
        caster = caster->next_caster;
        c->next_caster = NULL;
    }
    ch->casters = NULL;
}

bool valid_cast_stance(struct char_data *ch, int spellnum) {
    if (!IS_SPELL(spellnum))
        return FALSE;

    if (!skills[spellnum].fighting_ok && GET_STANCE(ch) == STANCE_FIGHTING)
        return FALSE;

    return GET_POS(ch) >= skills[spellnum].minpos;
}