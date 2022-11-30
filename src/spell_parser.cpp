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

#include "spell_parser.hpp"

#include "casting.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "cooldowns.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "events.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "magic.hpp"
#include "math.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "spells.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

void charge_mem(CharData *ch, int spellnum);
int check_spell_memory(CharData *ch, int spellnum);
void complete_spell(CharData *ch);
void start_chant(CharData *ch);
void end_chant(CharData *ch, CharData *tch, ObjData *tobj, int spellnum);
int bad_guess(CharData *ch);
bool find_spell_target(int spellnum, CharData *ch, char *t, int *target_status, CharData **tch, ObjData **tobj);
bool check_spell_target(int spellnum, CharData *ch, CharData *tch, ObjData *tobj);
void abort_casting(CharData *ch);
void aggro_lose_spells(CharData *ch);
EVENTFUNC(casting_handler);

int bogus_mage_spells[] = {0,
                           /* first 10 are defensive */
                           SPELL_ENHANCE_ABILITY, SPELL_DETECT_INVIS, SPELL_INFRAVISION, SPELL_MINOR_GLOBE,
                           SPELL_FIRESHIELD, SPELL_COLDSHIELD, SPELL_FARSEE, SPELL_FLY, SPELL_HASTE, SPELL_MAJOR_GLOBE,
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
    const char *org;
    const char *new_char;
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

void end_chant(CharData *ch, CharData *tch, ObjData *tobj, int spellnum) {
    CharData *gch;
    KnowSpell *tmp;
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
        found = false;

        /* gods see all */
        if (GET_LEVEL(gch) >= LVL_GOD)
            found = true;

        /* see if anyone recognized the spell */
        for (tmp = ch->see_spell; tmp && !found; tmp = tmp->next)
            if (gch == tmp->sch)
                found = true; /* ok he recognized it */

        if (!found) {
            /* change the syllables of the spoken spell */
            memset(lbuf, 0x0, 256);
            ofs = 0;
            strcpy(lbuf, skills[spellnum].name);
            while (*(lbuf + ofs))
                for (j = 0; *(syls[j].org); j++)
                    if (!strncasecmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
                        strcat(spellbuf, syls[j].new_char);
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
                    snprintf(saybuf, sizeof(saybuf), "$n closes $s eyes and utters the words, '%s'.", spellbuf);
                /* Is the target the receiver of the message? */
                else if (tch == gch)
                    snprintf(saybuf, sizeof(saybuf), "$n stares at you and utters the words, '%s'.", spellbuf);
                /* Ok, just a bystander.  But can they see the target? */
                else if (CAN_SEE(gch, tch))
                    snprintf(saybuf, sizeof(saybuf), "$n stares at $N and utters the words, '%s'.", spellbuf);
                else
                    snprintf(saybuf, sizeof(saybuf), "$n stares off at nothing and utters the words, '%s'.", spellbuf);
            }
            /* Is there an object target in the room? */
            else if (tobj && ((tobj->in_room == ch->in_room) || (tobj->carried_by == ch))) {
                if (CAN_SEE_OBJ(gch, tobj))
                    snprintf(saybuf, sizeof(saybuf), "$n stares at $p and utters the words, '%s'.", spellbuf);
                else
                    snprintf(saybuf, sizeof(saybuf), "$n stares at something and utters the words, '%s'.", spellbuf);
            }
            /* No target. */
            else
                snprintf(saybuf, sizeof(saybuf), "$n utters the words, '%s'.", spellbuf);
        }
        /* The bystander cannot see the caster. */
        else
            snprintf(saybuf, sizeof(saybuf), "Someone utters the words, '%s'.", spellbuf);

        /* Sending the message to the bystander or target. */
        format_act(buf, saybuf, ch, tobj, tch, gch);
        char_printf(gch, "%s", buf);
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
int call_magic(CharData *caster, CharData *cvict, ObjData *ovict, int spellnum, int skill, int casttype) {
    int savetype, imp_skill = 0;
    CharData *newvict;

    CharData *random_attack_target(CharData * ch, CharData * target, bool verbose);

    if (IS_SKILL(spellnum))
        return false;

    if (!cast_wtrigger(caster, cvict, ovict, spellnum) || !cast_otrigger(caster, ovict, spellnum) ||
        !cast_mtrigger(caster, cvict, spellnum))
        return 0;

    /* Don't check PEACEFUL and NOMAGIC flags for gods */
    if (GET_LEVEL(caster) < LVL_ATTENDANT) {
        if (IS_SPELL(spellnum) && ROOM_FLAGGED(caster->in_room, ROOM_NOMAGIC)) {
            send_to_char("Your magic fizzles out and dies.\n", caster);
            act("$n's magic fizzles out and dies.", false, caster, 0, 0, TO_ROOM);
            return false;
        }

        if (ROOM_FLAGGED(caster->in_room, ROOM_PEACEFUL) && SINFO.violent) {
            if (IS_SPELL(spellnum)) {
                send_to_char("A flash of white light fills the room, dispelling your violent magic!\n", caster);
                act("White light from no particular source suddenly fills the room, then vanishes.", false, caster, 0,
                    0, TO_ROOM);
            } else { /* song/chant */
                send_to_char("Your words dissolve into peaceful nothingness...\n", caster);
                act("$n's words fade away into peaceful nothingness...\n", false, caster, 0, 0, TO_ROOM);
            }
            return false;
        }
    }

    /* A confused caster - spell could go to the wrong target */
    if (SINFO.violent && CONFUSED(caster) && cvict) {
        newvict = random_attack_target(caster, cvict, false);
        if (newvict && newvict != cvict) {
            act("&5You fumbled the spell and aimed it at $N&0&5 instead!&0", false, caster, 0, newvict, TO_CHAR);
            act("&5$n&0&5 fumbled the spell and aimed it at $N&0&5 instead!&0", true, caster, 0, newvict, TO_NOTVICT);
            act("&5$n&0&5 fumbled the spell and aimed it at YOU instead!&0", false, caster, 0, newvict, TO_VICT);
        }
        cvict = newvict;
    }

    /* A victim attacks back immediately, 80% of the time */
    if (SINFO.violent && cvict && IS_NPC(cvict) && !FIGHTING(cvict) && GET_STANCE(cvict) >= STANCE_RESTING &&
        number(0, 4)) {
        event_create(EVENT_QUICK_AGGRO, quick_aggro_event, mkgenericevent(cvict, caster, 0), true, &(cvict->events), 0);
        remember(cvict, caster);
    }

    if (IS_SPELL(spellnum) && cvict && evades_spell(caster, cvict, spellnum, skill))
        return false;

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
        case SPELL_ACID_FOG:
            MANUAL_SPELL(spell_acid_fog);
            break;
        case SPELL_ARMOR_OF_GAIA:
            MANUAL_SPELL(spell_armor_of_gaia);
            break;
        case SPELL_BANISH:
            MANUAL_SPELL(spell_banish);
            break;
        case SPELL_CHARM:
            MANUAL_SPELL(spell_charm);
            break;
        case SPELL_CLOUD_OF_DAGGERS:
            MANUAL_SPELL(spell_cloud_of_daggers);
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
        case SPELL_REVEAL_HIDDEN:
            MANUAL_SPELL(spell_reveal_hidden);
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
    if (SINFO.violent && caster && cvict && attack_ok(cvict, caster, false))
        set_fighting(cvict, caster, false);

    return imp_skill;
}

/* Random messages to be sent to the room when a scroll is recited at a mob. */
/* These are used when the spheres of the spells don't tell us what message
 * to send. */
char *scroll_mob_msg[] = {"The writing on $p glows faintly as $n recites it at $N.",
                          "$n turns toward $N and recites $p.", "$n faces $N and recites $p.", nullptr};

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

void mag_objectmagic(CharData *ch, ObjData *obj, char *argument) {
    int i, target_status, spellnum;
    CharData *tch = nullptr, *next_tch;
    ObjData *tobj = nullptr;
    int scroll_failure, scroll_success;
    char *misc = nullptr;
    char *actmsg;

    one_argument(argument, arg);

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_STAFF:
        act("You tap $p three times on the ground.", false, ch, obj, 0, TO_CHAR);
        if (obj->action_description && strncasecmp(obj->action_description, "Nothing.", 8))
            act(obj->action_description, false, ch, obj, 0, TO_ROOM);
        else
            act("$n taps $p three times on the ground.", false, ch, obj, 0, TO_ROOM);

        if (GET_OBJ_VAL(obj, VAL_STAFF_CHARGES_LEFT) <= 0) {
            act("It seems powerless.", false, ch, obj, 0, TO_CHAR);
            act("Nothing seems to happen.", false, ch, obj, 0, TO_ROOM);
        } else {
            GET_OBJ_VAL(obj, VAL_STAFF_CHARGES_LEFT)--;
            WAIT_STATE(ch, PULSE_VIOLENCE);
            if (!check_spell_target(GET_OBJ_VAL(obj, VAL_STAFF_SPELL), ch, nullptr, nullptr))
                return;
            for (tch = world[ch->in_room].people; tch; tch = next_tch) {
                next_tch = tch->next_in_room;
                if (ch == tch)
                    continue;
                ch->casting.spell = 0;
                if (GET_OBJ_VAL(obj, VAL_STAFF_LEVEL))
                    call_magic(ch, tch, nullptr, GET_OBJ_VAL(obj, VAL_STAFF_SPELL), GET_OBJ_VAL(obj, VAL_STAFF_LEVEL),
                               CAST_STAFF);
                else
                    call_magic(ch, tch, nullptr, GET_OBJ_VAL(obj, VAL_STAFF_SPELL), DEFAULT_STAFF_LVL, CAST_STAFF);
            }
        }
        break;
    case ITEM_WAND:
        spellnum = GET_OBJ_VAL(obj, VAL_WAND_SPELL);

        if (find_spell_target(spellnum, ch, arg, &target_status, &tch, &tobj)) {

            if (tch) {
                if (tch == ch) {
                    if (SINFO.targets & TAR_NOT_SELF) {
                        send_to_char("You cannot cast this spell upon yourself.\n", ch);
                        return;
                    } else {
                        act("You point $p at yourself.", false, ch, obj, 0, TO_CHAR);
                        if (obj->action_description && strncasecmp(obj->action_description, "Nothing.", 8))
                            act(obj->action_description, false, ch, obj, tobj, TO_ROOM);
                        else
                            act("$n points $p at $mself.", false, ch, obj, 0, TO_ROOM);
                    }
                } else {
                    act("You point $p at $N.", false, ch, obj, tch, TO_CHAR);

                    /* For the sake of distance spells (e.g., moonwell) where
                     * the target may not be in the same room, there is no
                     * reason to give the name of the target to observers... */

                    if (ch->in_room == tch->in_room) {
                        if (obj->action_description && strncasecmp(obj->action_description, "Nothing.", 8))
                            act(obj->action_description, false, ch, obj, tch, TO_ROOM);
                        else {
                            act("$n points $p at $N.", true, ch, obj, tch, TO_NOTVICT);
                            act("$n points $p at you.", true, ch, obj, tch, TO_VICT);
                        }
                    } else {
                        act("$n points $p at nothing in particular.", true, ch, obj, 0, TO_ROOM);
                    }
                }
            } else if (tobj) {
                act("You point $p at $P.", false, ch, obj, tobj, TO_CHAR);
                if (ch->in_room == tobj->in_room) {
                    if (obj->action_description && strncasecmp(obj->action_description, "Nothing.", 8))
                        act(obj->action_description, false, ch, obj, tobj, TO_ROOM);
                    else
                        act("$n points $p at $P.", true, ch, obj, tobj, TO_ROOM);
                } else {
                    act("$n points $p at nothing in particular.", true, ch, obj, 0, TO_ROOM);
                }
            } else {
                act("You wave $p in the air.", true, ch, obj, 0, TO_CHAR);
                act("$n waves $p in the air.", true, ch, obj, 0, TO_ROOM);
            }
        } else if (*arg) {
            sprintf(buf, "You can't see any %s here.\n", arg);
            send_to_char(buf, ch);
            return;
        } else {
            act("At what should $p be pointed?", false, ch, obj, nullptr, TO_CHAR);
            return;
        }

        if (GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT) <= 0) {
            act("It seems powerless.", false, ch, obj, 0, TO_CHAR);
            act("Nothing seems to happen.", false, ch, obj, 0, TO_ROOM);
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
        scroll_success = false;

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
                        scroll_success = true;
                        act("You recite $p which dissolves.", true, ch, obj, 0, TO_CHAR);
                        /* Decide what message to send to the room. */
                        if (obj->action_description && strncasecmp(obj->action_description, "Nothing.", 8))
                            /* The scroll has a specific action desription message. */
                            act(obj->action_description, false, ch, obj, nullptr, TO_ROOM);
                        else if (tch) {
                            /* A mobile target */
                            actmsg = get_scroll_mob_msg(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1),
                                                        GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_2),
                                                        GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_3));
                            act(actmsg, false, ch, obj, tch, TO_ROOM);
                        } else if (tobj) {
                            /* An object target */
                            act("$n places $s hand on $P and recites $p.", false, ch, obj, tobj, TO_ROOM);
                        } else {
                            /* No target */
                            act("$n recites $p.", false, ch, obj, nullptr, TO_ROOM);
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
            if (obj != nullptr)
                extract_obj(obj);
        } else if (scroll_failure == SCROLL_FAILURE_NOTSELF) {
            send_to_char("You cannot cast this spell on yourself.\n", ch);
            return;
        } else if (scroll_failure == SCROLL_FAILURE_ONLYSELF) {
            send_to_char("You can only cast this spell on yourself.\n", ch);
            return;
        } else if (!(*arg)) {
            act("What do you want to recite $p at?", false, ch, obj, nullptr, TO_CHAR);
            return;
        } else {
            sprintf(buf, "You can't see any %s here.\n", arg);
            send_to_char(buf, ch);
            return;
        }
        break;
    case ITEM_POTION:
        tch = ch;
        act("You quaff $p.", false, ch, obj, nullptr, TO_CHAR);
        if (obj->action_description && strncasecmp(obj->action_description, "Nothing.", 8))
            act(obj->action_description, false, ch, obj, nullptr, TO_ROOM);
        else
            act("$n quaffs $p.", true, ch, obj, nullptr, TO_ROOM);

        WAIT_STATE(ch, PULSE_VIOLENCE);
        ch->casting.spell = 0;
        for (i = VAL_POTION_SPELL_1; i <= VAL_POTION_SPELL_3; i++)
            if (!(call_magic(ch, ch, nullptr, GET_OBJ_VAL(obj, i), GET_OBJ_VAL(obj, VAL_POTION_LEVEL), CAST_POTION)))
                break;

        if (obj != nullptr)
            extract_obj(obj);
        break;
    default:
        log("SYSERR: Unknown object_type in mag_objectmagic");
        break;
    }
}

bool check_spell_stance_position(CharData *ch, int spellnum) {
    if (GET_STANCE(ch) < STANCE_ALERT) {
        switch (GET_STANCE(ch)) {
        case STANCE_SLEEPING:
            send_to_char("You dream about great magical powers.\n", ch);
            break;
        case STANCE_RESTING:
            send_to_char("You cannot concentrate while resting.\n", ch);
            break;
        default:
            send_to_char("You can't do much of anything like this!\n", ch);
            break;
        }
        return 0;
    }

    if (GET_POS(ch) < SINFO.minpos) {
        switch (SINFO.minpos) {
        case POS_FLYING:
            char_printf(ch, "You must be flying to cast this spell.\n");
            break;
        case POS_STANDING:
            char_printf(ch, "You can only cast this spell while standing.\n");
            break;
        case POS_KNEELING:
            char_printf(ch, "You must at least be kneeling to cast this spell.\n");
            break;
        case POS_SITTING:
            char_printf(ch, "You'll have to at least sit up to cast this spell.\n");
            break;
        default:
            char_printf(ch, "Sorry, you can't cast this spell.  Who knows why.\n");
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

int cast_spell(CharData *ch, CharData *tch, ObjData *tobj, int spellnum) {
    char buf[256];
    int sphere, cresult = 0;

    if (spellnum < 0 || spellnum > TOP_SKILL_DEFINE) {
        sprintf(buf, "SYSERR: cast_spell trying to call spellnum %d\n", spellnum);
        log("%s", buf);
        ;
        return 0;
    }

    if (GET_STANCE(ch) == STANCE_FIGHTING && !SINFO.fighting_ok && GET_LEVEL(ch) <= LVL_IMMORT) {
        send_to_char("Impossible!  You can't concentrate enough!\n", ch);
        return 0;
    }

    if (!check_spell_stance_position(ch, spellnum))
        return 0;

    if (EFF_FLAGGED(ch, EFF_CHARM) && (ch->master == tch) && SINFO.violent) {
        send_to_char("You are afraid you might hurt your master!\n", ch);
        return 0;
    }
    if (!check_spell_target(spellnum, ch, tch, tobj))
        return 0;
    if (IS_SET(SINFO.routines, MAG_GROUP) && !IS_GROUPED(ch)) {
        send_to_char("You can't cast this spell if you're not in a group!\n", ch);
        return 0;
    }
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        switch (GET_CLASS(ch)) {
        case CLASS_PRIEST:
        case CLASS_PALADIN:
        case CLASS_RANGER:
            if (!IS_GOOD(ch)) {
                send_to_char("Your deity has removed your holy powers!\n", ch);
                return 0;
            }
            break;
        case CLASS_DIABOLIST:
        case CLASS_ANTI_PALADIN:
            if (!IS_EVIL(ch)) {
                send_to_char("Your deity has removed your unholy powers!\n", ch);
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

/* entry point for all Monk chants */

int chant(CharData *ch, CharData *tch, ObjData *obj, int chantnum) {
    int cresult;

    if (chantnum < 0 || chantnum > TOP_SKILL_DEFINE) {
        sprintf(buf, "SYSERR: chant trying to call chantnum %d", chantnum);
        log("%s", buf);
        ;
        return 0;
    }

    if (GET_STANCE(ch) == STANCE_FIGHTING && !skills[chantnum].fighting_ok && GET_LEVEL(ch) <= LVL_IMMORT) {
        send_to_char("Impossible!  You can't concentrate enough!\n", ch);
        return 0;
    }

    if (GET_STANCE(ch) < STANCE_ALERT) {
        switch (GET_STANCE(ch)) {
        case STANCE_SLEEPING:
            send_to_char("You dream about great magical powers.\n", ch);
            break;
        case STANCE_RESTING:
            send_to_char("You cannot concentrate while resting.\n", ch);
            break;
        default:
            send_to_char("You can't do much of anything like this!\n", ch);
            break;
        }
        return 0;
    }

    if (GET_POS(ch) < skills[chantnum].minpos) {
        switch (skills[chantnum].minpos) {
        case POS_FLYING:
            char_printf(ch, "You must be flying to chant this song.\n");
            break;
        case POS_STANDING:
            char_printf(ch, "You can only chant this song while standing.\n");
            break;
        case POS_KNEELING:
            char_printf(ch, "You must at least be kneeling to chant this song.\n");
            break;
        case POS_SITTING:
            char_printf(ch, "You'll have to at least sit up to chant this song.\n");
            break;
        default:
            char_printf(ch, "Sorry, you can't chant this song.  Who knows why.\n");
            break;
        }
        return 0;
    }

    if (!check_spell_target(chantnum, ch, tch, obj))
        return 0;
    if (EFF_FLAGGED(ch, EFF_CHARM) && (ch->master == tch) && skills[chantnum].violent) {
        send_to_char("You are afraid you might hurt your master!\n", ch);
        return 0;
    }
    if (IS_SET(skills[chantnum].routines, MAG_GROUP) && !IS_GROUPED(ch)) {
        send_to_char("You can't chant this song if you're not in a group!\n", ch);
        return 0;
    }

    act("$n begins chanting in a deep voice.", false, ch, 0, 0, TO_ROOM);
    send_to_char("You begin chanting in a deep voice.\n", ch);

    if (number(0, 101) > 50 + GET_SKILL(ch, SKILL_CHANT)) {
        send_to_char("You choke and grunt a raspy wail of pain.\n", ch);
        act("$n chokes on $s tears and coughs a raspy grunt.", true, ch, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE;
    }

    cresult = call_magic(ch, tch, obj, chantnum, GET_SKILL(ch, SKILL_CHANT), CAST_CHANT);

    /* Prevent skill improvement for chants against illusions */
    if (IS_SET(cresult, CAST_RESULT_IMPROVE) && tch && MOB_FLAGGED(tch, MOB_ILLUSORY))
        return cresult & ~CAST_RESULT_IMPROVE;

    return cresult;
}

/* Entry point for Bard music */

/*
bool music( CharData *ch, int music) {
    if (GET_LEVEL(ch) < LVL_GOD && GET_COOLDOWN(ch, music)) {
            int hours = GET_COOLDOWN(ch, music) / (1 MUD_HR);
            if (hours == 1)
                strcpy(buf1, "hour");
            else
                sprintf(buf1, "%d hours", hours);
            sprintf(buf,
                    "You're still drained from performing recently!\n"
                    "You'll be able to perform again in another %s.\n",
                    buf1);
            send_to_char(buf, ch);
            return true;
    }
    return false;
}
*/

int perform(CharData *ch, CharData *tch, ObjData *obj, int songnum) {
    int cresult;

    if (songnum < 0 || songnum > TOP_SKILL_DEFINE) {
        sprintf(buf, "SYSERR: perform trying to call songnum %d", songnum);
        log("%s", buf);
        ;
        return 0;
    }

    if (GET_STANCE(ch) == STANCE_FIGHTING && !skills[songnum].fighting_ok && GET_LEVEL(ch) <= LVL_IMMORT) {
        send_to_char("Impossible!  You can't concentrate enough!\n", ch);
        return 0;
    }

    if (GET_STANCE(ch) < STANCE_ALERT) {
        switch (GET_STANCE(ch)) {
        case STANCE_SLEEPING:
            send_to_char("You dream about heart-stopping performances.\n", ch);
            break;
        case STANCE_RESTING:
            send_to_char("You cannot concentrate while resting.\n", ch);
            break;
        default:
            send_to_char("You can't do much of anything like this!\n", ch);
            break;
        }
        return 0;
    }

    if (GET_POS(ch) < skills[songnum].minpos) {
        switch (skills[songnum].minpos) {
        case POS_FLYING:
            char_printf(ch, "You must be flying to perform this.\n");
            break;
        case POS_STANDING:
            char_printf(ch, "You can only perform this while standing.\n");
            break;
        case POS_KNEELING:
            char_printf(ch, "You must at least be kneeling to perform this.\n");
            break;
        case POS_SITTING:
            char_printf(ch, "You'll have to at least sit up to perform this.\n");
            break;
        default:
            char_printf(ch, "Sorry, you can't perform this.  Who knows why.\n");
            break;
        }
        return 0;
    }

    if (!check_spell_target(songnum, ch, tch, obj))
        return 0;
    if (EFF_FLAGGED(ch, EFF_CHARM) && (ch->master == tch) && skills[songnum].violent) {
        send_to_char("You are afraid you might hurt your master!\n", ch);
        return 0;
    }
    if (IS_SET(skills[songnum].routines, MAG_GROUP) && !IS_GROUPED(ch)) {
        send_to_char("You can't perform this if you're not in a group!\n", ch);
        return 0;
    }

    act("$n begins playing beautiful music.", false, ch, 0, 0, TO_ROOM);
    send_to_char("You begin a virtuosic performance.\n", ch);

    if (number(0, 101) > 50 + GET_SKILL(ch, SKILL_PERFORM)) {
        send_to_char("You choke and grunt a raspy wail of pain.\n", ch);
        act("$n chokes on $s tears and coughs a raspy grunt.", true, ch, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE;
    }

    cresult = call_magic(ch, tch, obj, songnum, GET_SKILL(ch, SKILL_PERFORM), CAST_PERFORM);

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
    CharData *tch = nullptr;
    ObjData *tobj = nullptr;
    int spellnum, target = 0;
    int target_status = TARGET_NULL;

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        send_to_char("Your lips move, but no sound forms.\n", ch);
        return;
    }

    if (CASTING(ch)) {
        send_to_char("But you are already casting a spell!\n", ch);
        return;
    }

    if (GET_STANCE(ch) < STANCE_ALERT) {
        send_to_char("You are too relaxed.\n", ch);
        return;
    }

    if (subcmd == SCMD_CHANT) {
        if (IS_NPC(ch)) {
            send_to_char("NPC's can't chant!\n", ch);
            return;
        }
        if (!GET_SKILL(ch, SKILL_CHANT)) {
            send_to_char("You imitate a monk chanting...Monkey see, monkey do?\n", ch);
            return;
        }
        if (GET_LEVEL(ch) < LVL_GOD && GET_COOLDOWN(ch, CD_CHANT)) {
            int hours = GET_COOLDOWN(ch, CD_CHANT) / (1 MUD_HR);
            if (hours == 1)
                strcpy(buf1, "hour");
            else
                sprintf(buf1, "%d hours", hours);
            sprintf(buf,
                    "You're still out of breath from chanting recently!\n"
                    "You'll be able to chant again in another %s.\n",
                    buf1);
            send_to_char(buf, ch);
            return;
        }
    }

    if (subcmd == SCMD_PERFORM) {
        if (IS_NPC(ch)) {
            send_to_char("NPC's can't perform!\n", ch);
            return;
        }
        if (!GET_SKILL(ch, SKILL_PERFORM)) {
            send_to_char("You have no idea how to perform anything good.\n", ch);
            return;
        }
        if (GET_LEVEL(ch) < LVL_GOD) {
            switch (cha_app[GET_CHA(ch)].music) {
            case 7:
                if (!GET_COOLDOWN(ch, CD_MUSIC_7))
                    break;
            case 6:
                if (!GET_COOLDOWN(ch, CD_MUSIC_6))
                    break;
            case 5:
                if (!GET_COOLDOWN(ch, CD_MUSIC_5))
                    break;
            case 4:
                if (!GET_COOLDOWN(ch, CD_MUSIC_4))
                    break;
            case 3:
                if (!GET_COOLDOWN(ch, CD_MUSIC_3))
                    break;
            case 2:
                if (!GET_COOLDOWN(ch, CD_MUSIC_2))
                    break;
            case 1:
                if (!GET_COOLDOWN(ch, CD_MUSIC_1))
                    break;
            default:
                sprintf(buf, "You're still drained from performing recently!\n");
                send_to_char(buf, ch);
                if GET_COOLDOWN (ch, CD_MUSIC_1) {
                    int hours = GET_COOLDOWN(ch, CD_MUSIC_1) / (1 MUD_HR);
                    if (hours == 1)
                        strcpy(buf1, "hour");
                    else {
                        sprintf(buf1, "%d hours", hours);
                        sprintf(buf, "Performance one will refresh in %s.\n", buf1);
                        send_to_char(buf, ch);
                    }
                }
                if GET_COOLDOWN (ch, CD_MUSIC_2) {
                    int hours = GET_COOLDOWN(ch, CD_MUSIC_2) / (1 MUD_HR);
                    if (hours == 1)
                        strcpy(buf1, "hour");
                    else {
                        sprintf(buf1, "%d hours", hours);
                        sprintf(buf, "Performance two will refresh in %s.\n", buf1);
                        send_to_char(buf, ch);
                    }
                }
                if GET_COOLDOWN (ch, CD_MUSIC_3) {
                    int hours = GET_COOLDOWN(ch, CD_MUSIC_3) / (1 MUD_HR);
                    if (hours == 1)
                        strcpy(buf1, "hour");
                    else {
                        sprintf(buf1, "%d hours", hours);
                        sprintf(buf, "Performance three will refresh in %s.\n", buf1);
                        send_to_char(buf, ch);
                    }
                }
                if GET_COOLDOWN (ch, CD_MUSIC_4) {
                    int hours = GET_COOLDOWN(ch, CD_MUSIC_4) / (1 MUD_HR);
                    if (hours == 1)
                        strcpy(buf1, "hour");
                    else {
                        sprintf(buf1, "%d hours", hours);
                        sprintf(buf, "Performance four will refresh in %s.\n", buf1);
                        send_to_char(buf, ch);
                    }
                }
                if GET_COOLDOWN (ch, CD_MUSIC_5) {
                    int hours = GET_COOLDOWN(ch, CD_MUSIC_5) / (1 MUD_HR);
                    if (hours == 1)
                        strcpy(buf1, "hour");
                    else {
                        sprintf(buf1, "%d hours", hours);
                        sprintf(buf, "Performance five will refresh in %s.\n", buf1);
                        send_to_char(buf, ch);
                    }
                }
                if GET_COOLDOWN (ch, CD_MUSIC_6) {
                    int hours = GET_COOLDOWN(ch, CD_MUSIC_6) / (1 MUD_HR);
                    if (hours == 1)
                        strcpy(buf1, "hour");
                    else {
                        sprintf(buf1, "%d hours", hours);
                        sprintf(buf, "Performance six will refresh in %s.\n", buf1);
                        send_to_char(buf, ch);
                    }
                }
                if GET_COOLDOWN (ch, CD_MUSIC_7) {
                    int hours = GET_COOLDOWN(ch, CD_MUSIC_7) / (1 MUD_HR);
                    if (hours == 1)
                        strcpy(buf1, "hour");
                    else {
                        sprintf(buf1, "%d hours", hours);
                        sprintf(buf, "Performance seven will refresh in %s.\n", buf1);
                        send_to_char(buf, ch);
                    }
                }
                return;
            }
        }
    }

    argument = delimited_arg(argument, arg, '\'');

    if (!*arg) {
        if (subcmd == SCMD_CHANT)
            send_to_char("What do you want to chant?\n", ch);
        else if (subcmd == SCMD_PERFORM)
            send_to_char("What do you want to perform?\n", ch);
        else
            send_to_char("Cast what where?\n", ch);
        return;
    }

    if (subcmd == SCMD_CHANT) {
        spellnum = find_chant_num(arg);
        if (!IS_CHANT(spellnum)) {
            send_to_char("Chant what?!?\n", ch);
            return;
        }
    } else if (subcmd == SCMD_PERFORM) {
        spellnum = find_song_num(arg);
        if (!IS_SONG(spellnum)) {
            send_to_char("Perform what?!?\n", ch);
            return;
        }
    } else {
        spellnum = find_spell_num(arg);
        if (!IS_SPELL(spellnum)) {
            send_to_char("Cast what?!?\n", ch);
            return;
        }
    }

    /* Can the caster actually cast this spell? */
    if (GET_LEVEL(ch) < SINFO.min_level[(int)GET_CLASS(ch)] || !GET_SKILL(ch, spellnum)) {
        if (subcmd == SCMD_CHANT)
            send_to_char("You do not know that chant!\n", ch);
        else if (subcmd == SCMD_PERFORM)
            send_to_char("You do not know that music!\n", ch);
        else
            send_to_char("You do not know that spell!\n", ch);
        return;
    }

    /* Is the spell memorized?  PC's only. */
    if (subcmd == SCMD_CAST && !IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT && !check_spell_memory(ch, spellnum)) {
        send_to_char("You do not have that spell memorized!\n", ch);
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
                send_to_char("Cannot find the target of your chant!\n", ch);
            else if (subcmd == SCMD_PERFORM)
                send_to_char("Cannot find the target of your music!\n", ch);
            else
                send_to_char("Cannot find the target of your spell!\n", ch);
        } else {
            if (subcmd == SCMD_CHANT)
                send_to_char("To whom should the chant be sung?\n", ch);
            else if (subcmd == SCMD_PERFORM)
                send_to_char("To whom should the music be played?\n", ch);
            else {
                sprintf(buf, "Upon %s should the spell be cast?\n",
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
            act("$n starts chanting, but stops abruptly, coughing up blood!", false, ch, 0, 0, TO_ROOM);
            char_printf(ch,
                        "You begin chanting, but your throat causes you to "
                        "cough up blood!\n");
        } else if (subcmd == SCMD_PERFORM) {
            act("$n starts playing, but stops abruptly, coughing up blood!", false, ch, 0, 0, TO_ROOM);
            char_printf(ch,
                        "You begin playing, but your throat causes you to "
                        "cough up blood!\n");
        } else {
            act("$n starts casting, but stops abruptly, coughing up blood!", false, ch, 0, 0, TO_ROOM);
            char_printf(ch,
                        "You begin casting, but your throat causes you to "
                        "cough up blood!\n");
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
            SET_COOLDOWN(ch, CD_CHANT,
                         (7 - (((wis_app[GET_WIS(ch)].bonus) * 3) / 4) + (((int_app[GET_INT(ch)].bonus) * 1) / 4))
                             MUD_HR);
            WAIT_STATE(ch, PULSE_VIOLENCE * 1.5);
        }

    } else if (subcmd == SCMD_PERFORM) {
        if (cha_app[GET_CHA(ch)].music == 0) {
            send_to_char("Your Charisma is too low to perform!\n", ch);
        }
        for (int i = 1; i <= cha_app[GET_CHA(ch)].music; i++) {
            switch (i) {
            case 1:
                if (!GET_COOLDOWN(ch, CD_MUSIC_1)) {
                    int cresult = perform(ch, tch, tobj, spellnum);
                    if (IS_SET(cresult, CAST_RESULT_IMPROVE))
                        improve_skill(ch, SKILL_PERFORM);
                    if (IS_SET(cresult, CAST_RESULT_CHARGE)) {
                        SET_COOLDOWN(ch, CD_MUSIC_1, (8 - cha_app[GET_CHA(ch)].music) MUD_HR);
                        WAIT_STATE(ch, PULSE_VIOLENCE * 1.5);
                    }
                    i = 8;
                }
                break;
            case 2:
                if (!GET_COOLDOWN(ch, CD_MUSIC_2)) {
                    int cresult = perform(ch, tch, tobj, spellnum);
                    if (IS_SET(cresult, CAST_RESULT_IMPROVE))
                        improve_skill(ch, SKILL_PERFORM);
                    if (IS_SET(cresult, CAST_RESULT_CHARGE)) {
                        SET_COOLDOWN(ch, CD_MUSIC_2, (8 - cha_app[GET_CHA(ch)].music) MUD_HR);
                        WAIT_STATE(ch, PULSE_VIOLENCE * 1.5);
                    }
                    i = 8;
                }
                break;
            case 3:
                if (!GET_COOLDOWN(ch, CD_MUSIC_3)) {
                    int cresult = perform(ch, tch, tobj, spellnum);
                    if (IS_SET(cresult, CAST_RESULT_IMPROVE))
                        improve_skill(ch, SKILL_PERFORM);
                    if (IS_SET(cresult, CAST_RESULT_CHARGE)) {
                        SET_COOLDOWN(ch, CD_MUSIC_3, (8 - cha_app[GET_CHA(ch)].music) MUD_HR);
                        WAIT_STATE(ch, PULSE_VIOLENCE * 1.5);
                    }
                    i = 8;
                }
                break;
            case 4:
                if (!GET_COOLDOWN(ch, CD_MUSIC_4)) {
                    int cresult = perform(ch, tch, tobj, spellnum);
                    if (IS_SET(cresult, CAST_RESULT_IMPROVE))
                        improve_skill(ch, SKILL_PERFORM);
                    if (IS_SET(cresult, CAST_RESULT_CHARGE)) {
                        SET_COOLDOWN(ch, CD_MUSIC_4, (8 - cha_app[GET_CHA(ch)].music) MUD_HR);
                        WAIT_STATE(ch, PULSE_VIOLENCE * 1.5);
                    }
                    i = 8;
                }
                break;
            case 5:
                if (!GET_COOLDOWN(ch, CD_MUSIC_5)) {
                    int cresult = perform(ch, tch, tobj, spellnum);
                    if (IS_SET(cresult, CAST_RESULT_IMPROVE))
                        improve_skill(ch, SKILL_PERFORM);
                    if (IS_SET(cresult, CAST_RESULT_CHARGE)) {
                        SET_COOLDOWN(ch, CD_MUSIC_5, (8 - cha_app[GET_CHA(ch)].music) MUD_HR);
                        WAIT_STATE(ch, PULSE_VIOLENCE * 1.5);
                    }
                    i = 8;
                }
                break;
            case 6:
                if (!GET_COOLDOWN(ch, CD_MUSIC_6)) {
                    int cresult = perform(ch, tch, tobj, spellnum);
                    if (IS_SET(cresult, CAST_RESULT_IMPROVE))
                        improve_skill(ch, SKILL_PERFORM);
                    if (IS_SET(cresult, CAST_RESULT_CHARGE)) {
                        SET_COOLDOWN(ch, CD_MUSIC_6, (8 - cha_app[GET_CHA(ch)].music) MUD_HR);
                        WAIT_STATE(ch, PULSE_VIOLENCE * 1.5);
                    }
                    i = 8;
                }
                break;
            case 7:
                if (!GET_COOLDOWN(ch, CD_MUSIC_7)) {
                    int cresult = perform(ch, tch, tobj, spellnum);
                    if (IS_SET(cresult, CAST_RESULT_IMPROVE))
                        improve_skill(ch, SKILL_PERFORM);
                    if (IS_SET(cresult, CAST_RESULT_CHARGE)) {
                        SET_COOLDOWN(ch, CD_MUSIC_7, (8 - cha_app[GET_CHA(ch)].music) MUD_HR);
                        WAIT_STATE(ch, PULSE_VIOLENCE * 1.5);
                    }
                    i = 8;
                }
                break;
            default:
                break;
            }
        }

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
            event_create(EVENT_CASTING, casting_handler, ch, false, &(ch->events), PULSE_VIOLENCE / 2);
        }
    }
}

bool check_spell_target(int spellnum, CharData *ch, CharData *tch, ObjData *tobj) {
    const char *verb = IS_CHANT(spellnum) ? "chant" : "cast";
    const char *noun = IS_CHANT(spellnum) ? "song" : "spell";

    if (IS_SET(SINFO.targets, TAR_SELF_ONLY) && tch != ch && GET_LEVEL(ch) < LVL_GOD) {
        char_printf(ch, "You can only %s this %s %s yourself!\n", verb, noun, IS_CHANT(spellnum) ? "to" : "upon");
        return false;
    }
    if (IS_SET(SINFO.targets, TAR_NOT_SELF) && tch == ch) {
        char_printf(ch, "You cannot %s this %s %s yourself!\n", verb, noun, IS_CHANT(spellnum) ? "to" : "upon");
        return false;
    }
    if (IS_SET(SINFO.targets, TAR_OUTDOORS) && CH_INDOORS(ch)) {
        char_printf(ch, "This area is too enclosed to %s that %s!\n", verb, noun);
        return false;
    }
    if (IS_SET(SINFO.targets, TAR_GROUND) && !CH_INDOORS(ch) && !QUAKABLE(IN_ROOM(ch))) {
        char_printf(ch, "You must be on solid ground to %s that %s!\n", verb, noun);
        return false;
    }
    if (IS_SET(SINFO.targets, TAR_NIGHT_ONLY) && SUN(IN_ROOM(ch)) == SUN_LIGHT) {
        char_printf(ch, "You cannot %s this %s during the day!\n", verb, noun);
        return false;
    }
    if (IS_SET(SINFO.targets, TAR_DAY_ONLY) && SUN(IN_ROOM(ch)) == SUN_DARK) {
        char_printf(ch, "You can only %s this %s during the day!\n", verb, noun);
        return false;
    }
    return true;
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
 *   tch              A CharData target.
 *   tobj             An object target.
 */

bool find_spell_target(int spellnum, CharData *ch, char *t, int *target_status, CharData **tch, ObjData **tobj) {

    FindContext context = find_vis_by_name(ch, t);

    *tch = nullptr;
    *tobj = nullptr;
    *target_status = TARGET_NULL;

    if (IS_SET(SINFO.targets, TAR_IGNORE)) {
        *target_status = TARGET_ALL_ROOM;
        return true;
    } else if (*t) {
        if ((IS_SET(SINFO.targets, TAR_CHAR_ROOM))) {
            if ((*tch = find_char_in_room(&world[ch->in_room], context)) != nullptr) {
                *target_status = TARGET_IN_ROOM;
                return true;
            }
        }
        if (IS_SET(SINFO.targets, TAR_CHAR_WORLD))
            if ((*tch = find_char_around_char(ch, context))) {
                *target_status = TARGET_IN_WORLD;
                return true;
            }
        if (IS_SET(SINFO.targets, TAR_OBJ_INV))
            if ((*tobj = find_obj_in_list(ch->carrying, context))) {
                *target_status = TARGET_IN_INV;
                return true;
            }
        if (IS_SET(SINFO.targets, TAR_OBJ_EQUIP)) {
            if ((*tobj = find_obj_in_eq(ch, nullptr, context))) {
                *target_status = TARGET_EQUIP;
                return true;
            }
        }
        if (IS_SET(SINFO.targets, TAR_OBJ_ROOM))
            if ((*tobj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, t)))) {
                *target_status = TARGET_IN_ROOM;
                return true;
            }
        if (IS_SET(SINFO.targets, TAR_OBJ_WORLD))
            if ((*tobj = find_obj_around_char(ch, find_vis_by_name(ch, t)))) {
                *target_status = TARGET_IN_WORLD;
                return true;
            }
        if (IS_SET(SINFO.targets, TAR_STRING)) {
            *target_status = TARGET_STRING;
            return true;
        }

    } else { /* if target string is empty */
        if (IS_SET(SINFO.targets, TAR_FIGHT_SELF))
            if (FIGHTING(ch) != nullptr) {
                *tch = ch;
                *target_status = TARGET_SELF;
                return true;
            }
        if (IS_SET(SINFO.targets, TAR_FIGHT_VICT))
            if (FIGHTING(ch) != nullptr) {
                *tch = FIGHTING(ch);
                *target_status = TARGET_FIGHTING;
                return true;
            }
        /* if no target specified, and the spell isn't violent, default to self */
        if (IS_SET(SINFO.targets, TAR_CHAR_ROOM) && !SINFO.violent && !IS_SET(SINFO.targets, TAR_NOT_SELF)) {
            *tch = ch;
            *target_status = TARGET_SELF;
            return true;
        }
    }
    return false;
}

void complete_spell(CharData *ch) {

    /* Remove links from target to caster */
    if (ch->casting.obj)
        obj_forget_caster(ch->casting.obj, ch);
    if (ch->casting.tch)
        char_forget_caster(ch->casting.tch, ch);

    /* Insanity really makes it hard to cast spells. */
    if (EFF_FLAGGED(ch, EFF_INSANITY) && !mag_savingthrow(ch, SAVING_PARA)) {
        act("$n babbles a bit as a strand of drool drips down $s chin.", true, ch, 0, 0, TO_ROOM);
        act("Your mind is not in any condition to cast spells.", false, ch, 0, 0, TO_CHAR);
        act("&1$n&1 stops chanting abruptly!&0", true, ch, 0, 0, TO_ROOM);
        return;
    }

    if (GET_LEVEL(ch) < LVL_GOD && ch->casting.spell != SPELL_VENTRILOQUATE) {
        send_to_char("You complete your spell.\n", ch);
        act("$n completes $s spell...", false, ch, 0, 0, TO_ROOM);
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
 * Returns true if the mob started casting and false otherwise.
 */
bool mob_cast(CharData *ch, CharData *tch, ObjData *tobj, int spellnum) {
    int target_status = TARGET_NULL;
    int circle = SPELL_CIRCLE(ch, spellnum);
    ACMD(do_stand);
    CharData *targ_ch = nullptr;
    ObjData *targ_obj = nullptr;

    if (!IS_NPC(ch))
        return false;

    if (EFF_FLAGGED(ch, EFF_SILENCE))
        return false;

    if (CASTING(ch))
        return false;

    if (!AWAKE(ch))
        return false;

    if (GET_MOB_WAIT(ch) <= 0 && GET_POS(ch) < POS_STANDING)
        do_stand(ch, "", 0, 0);

    if (GET_STANCE(ch) < STANCE_ALERT)
        return false;

    if (!IS_SPELL(spellnum))
        return false;

    if (GET_LEVEL(ch) < SINFO.min_level[(int)GET_CLASS(ch)])
        return false;

    if (!GET_SKILL(ch, spellnum))
        return false;

    if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC))
        return false;

    /* Check alignment restrictions for clerical classes. */
    if ((GET_CLASS(ch) == CLASS_DIABOLIST && !IS_EVIL(ch)) || (GET_CLASS(ch) == CLASS_PRIEST && !IS_GOOD(ch)) ||
        (GET_CLASS(ch) == CLASS_PALADIN && !IS_GOOD(ch)) || (GET_CLASS(ch) == CLASS_RANGER && !IS_GOOD(ch)) ||
        (GET_CLASS(ch) == CLASS_ANTI_PALADIN && !IS_EVIL(ch)))
        return false;

    /* Check mob's position and stance. */
    if (SINFO.minpos > GET_POS(ch) || GET_STANCE(ch) < STANCE_ALERT ||
        (!SINFO.fighting_ok && GET_STANCE(ch) == STANCE_FIGHTING))
        return false;

    /* Check if mob has slots in this spell's circle in its spell bank */
    if (GET_MOB_SPLBANK(ch, circle) <= 0)
        return false;

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
        return false;

    if (!check_spell_target(spellnum, ch, tch, tobj))
        return false;

    /* An injured throat makes it difficult to cast. */
    if (EFF_FLAGGED(ch, EFF_HURT_THROAT) && number(0, MAX_ABILITY_VALUE) > GET_VIEWED_CON(ch)) {
        act("$n starts casting, but stops abruptly, coughing up blood!", false, ch, 0, 0, TO_ROOM);
        char_printf(ch,
                    "You begin casting, but your throat causes you to "
                    "cough up blood!\n");
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return true; /* makes caller think we cast a spell so they don't try again
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
    event_create(EVENT_CASTING, casting_handler, ch, false, &(ch->events), 0);
    return true;
}

void start_chant(CharData *ch) {
    void garble_text(char *string, int percent);
    CharData *gch;
    KnowSpell *tmp, *tmp2;
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
    ch->see_spell = nullptr;

    /* Randomly garble the letters. */
    strcpy(lbuf, skills[ch->casting.spell].name);
    garble_text(lbuf, -1);

    /* Change the syllables of the spoken spell */
    while (*(lbuf + ofs))
        for (j = 0; *(syls[j].org); j++)
            if (!strncasecmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
                strcat(garblebuf, syls[j].new_char);
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

        bad = false;
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
                bad = true;
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
            CREATE(tmp, KnowSpell, 1);
            tmp->sch = gch;
            tmp->next = ch->see_spell;
            ch->see_spell = tmp;
        }

        /* Is there a Target?  Is it in the room?  Is it not the caster? */
        if (ch->casting.tch && ch->casting.tch->in_room == ch->in_room && ch != ch->casting.tch) {

            /* Intelligence check to determine the target.  If we know the spell we
             * know the target */
            if (number(0, 101) < GET_INT(gch) || !strcasecmp(spellbuf, skills[ch->casting.spell].name) || (bad) ||
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
        act(buf, false, ch, 0, gch, TO_VICT);
    }

    /* Message to caster */
    if (GET_LEVEL(ch) < LVL_GOD)
        send_to_char("You start chanting...\n", ch);
    else
        send_to_char("You cast your spell...\n", ch);
}

/*
 * Display incorrect spell on a very bad KNOW_SPELL roll.
 */
int bad_guess(CharData *ch) {

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
void targets_remember_caster(CharData *caster) {
    CharData *temp;

    if (caster->casting.obj) {
        /* This should never happen. If this code catches a circular list,
         * it will prevent a crash. The execution time cost of this code should
         * be low, since the number of spells being cast at any person or object
         * should also be low. */
        for (temp = caster->casting.obj->casters; temp; temp = temp->next_caster)
            if (temp == caster) {
                sprintf(buf, "1. CIRCULAR CAST LIST ERROR: %s was already casting at %s!", GET_NAME(caster),
                        caster->casting.obj->short_description);
                mudlog(buf, NRM, LVL_GOD, true);
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
                mudlog(buf, NRM, LVL_GOD, true);
                return;
            }
        caster->next_caster = caster->casting.tch->casters;
        caster->casting.tch->casters = caster;
    }
}

/* Forget someone who's casting a spell at you.  This can happen when
 * the spell is complete or has been aborted for any reason. */
void obj_forget_caster(ObjData *obj, CharData *caster) {
    CharData *temp;

    REMOVE_FROM_LIST(caster, obj->casters, next_caster);
}

void char_forget_caster(CharData *ch, CharData *caster) {
    CharData *temp;

    REMOVE_FROM_LIST(caster, ch->casters, next_caster);
}

/* Forget everyone who is casting a spell at you.  This tends to happen
 * when you're removed from the game. */
void obj_forget_casters(ObjData *obj) {
    CharData *caster, *c;

    for (caster = obj->casters; caster;) {
        /* First make the caster forget what they it was casting its
         * spell upon.  This prevents it from coming back and calling
         * obj_forget_caster() in STOP_CASTING() and messing up our list. */
        caster->casting.obj = nullptr;
        STOP_CASTING(caster);
        if (AWAKE(caster)) {
            act("You stop chanting abruptly!", false, caster, 0, 0, TO_CHAR);
            act("$n stops chanting abruptly!", false, caster, 0, 0, TO_ROOM);
        }
        c = caster;
        caster = caster->next_caster;
        c->next_caster = nullptr;
    }
    obj->casters = nullptr;
}

void char_forget_casters(CharData *ch) {
    CharData *caster, *c;

    for (caster = ch->casters; caster;) {
        /* First make the caster forget what it was casting its
         * spell upon.  This prevents it from coming back and calling
         * char_forget_caster() in STOP_CASTING() and messing up our list. */
        caster->casting.tch = nullptr;
        STOP_CASTING(caster);
        if (AWAKE(caster)) {
            act("You stop chanting abruptly!", false, caster, 0, 0, TO_CHAR);
            act("$n stops chanting abruptly!", false, caster, 0, 0, TO_ROOM);
        }
        c = caster;
        caster = caster->next_caster;
        c->next_caster = nullptr;
    }
    ch->casters = nullptr;
}

bool valid_cast_stance(CharData *ch, int spellnum) {
    if (!IS_SPELL(spellnum))
        return false;

    if (!skills[spellnum].fighting_ok && GET_STANCE(ch) == STANCE_FIGHTING)
        return false;

    return GET_POS(ch) >= skills[spellnum].minpos;
}