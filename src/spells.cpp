/***************************************************************************
 *   File: spells.c                                       Part of FieryMUD *
 *  Usage: Implementation of "manual spells".                              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000, 2001 by the Fiery Consortium. *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University.                                       *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "spells.hpp"

#include "act.hpp"
#include "casting.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "damage.hpp"
#include "db.hpp"
#include "events.hpp"
#include "exits.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "lifeforce.hpp"
#include "limits.hpp"
#include "logging.hpp"
#include "magic.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "movement.hpp"
#include "objects.hpp"
#include "pfiles.hpp"
#include "races.hpp"
#include "rooms.hpp"
#include "skills.hpp"
#include "spell_parser.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

void clear_memory(CharData *ch);
void perform_wear(CharData *ch, ObjData *obj, int where, bool collective = false);
void half_chop(char *string, char *arg1, char *arg2);
void command_interpreter(CharData *ch, char *argument);
void check_new_surroundings(CharData *ch, bool old_room_was_dark, bool tx_obvious);

ASPELL(spell_acid_fog) {
    DelayedCastEventObj *event_obj;

    if (!ch)
        return 0;

    act("&2&bYou conjure a thick corrosive fog!&0", false, ch, 0, victim, TO_CHAR);
    act("&2&b$n conjures a thick corrosive fog!&0", false, ch, 0, victim, TO_ROOM);

    event_obj = construct_delayed_cast(ch, victim, SPELL_ACID_FOG, MAG_AREA, 4, 4 RL_SEC, skill, savetype, false);
    event_create(EVENT_SPELL, delayed_cast_event, event_obj, true, &(ch->events), 4 RL_SEC);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(chant_apocalyptic_anthem) {
    CharData *tch, *next_tch;
    bool found = false;

    if (ch->in_room == NOWHERE)
        return 0;

    char_printf(ch, "You let the anthem of the apocalypse ring!\n");
    act("$n chants an anthem of demise and fatality!", false, ch, 0, 0, TO_ROOM);

    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;
        if (tch == ch)
            continue;
        if (!attack_ok(ch, tch, false))
            continue;

        found = true;

        if (FIGHTING(tch) == ch)
            continue;
        if (!AWAKE(tch))
            continue;
        if (mag_savingthrow(tch, SAVING_PARA))
            continue;
        if (is_grouped(ch, tch))
            continue;

        if (FIGHTING(tch))
            stop_fighting(tch);
        set_fighting(tch, ch, false);

        act("$N turns on you in hatred and confusion!", false, ch, 0, tch, TO_CHAR);
        act("You turn on $n in hatred and confusion!", false, ch, 0, tch, TO_VICT);
        act("$N turns on $n in hatred and confusion!", false, ch, 0, tch, TO_ROOM);
    }

    if (found)
        return CAST_RESULT_IMPROVE | CAST_RESULT_CHARGE;
    else
        return CAST_RESULT_CHARGE;
}

ASPELL(spell_armor_of_gaia) {
    if (!victim)
        return 0;

    if (GET_EQ(victim, WEAR_BODY) && GET_EQ(victim, WEAR_LEGS) && GET_EQ(victim, WEAR_ARMS)) {
        char_printf(victim, "Your body is too encumbered to don the armor.\n");
        return CAST_RESULT_CHARGE;
    }
    if (ch == victim) {
        act("&6$n&6 calls upon &2&bGaia&0&6 to guard $s body...&0", true, ch, 0, 0, TO_ROOM);
        act("&6You call upon &2&bGaia&0&6 to guard your body...&0", false, ch, 0, 0, TO_CHAR);
    } else {
        act("&6$n&6 calls upon &2&bGaia&0&6 to guard $N...&0", true, ch, 0, victim, TO_ROOM);
        act("&6You call upon &2&bGaia&0&6 to guard $S...&0", false, ch, 0, victim, TO_CHAR);
    }
    if (!GET_EQ(victim, WEAR_BODY)) {
        if ((obj = read_object(1040, VIRTUAL))) {
            obj_to_char(obj, victim);
            perform_wear(victim, obj, WEAR_BODY);
        } else {
            log("SYSERR: Armor object not found in spell Armor of Gaia");
            return 0;
        }
    }
    if (!GET_EQ(victim, WEAR_ARMS)) {
        if ((obj = read_object(1041, VIRTUAL))) {
            obj_to_char(obj, victim);
            perform_wear(victim, obj, WEAR_ARMS);
        } else {
            log("SYSERR: Armplates object not found in spell Armor of Gaia");
            return 0;
        }
    }
    if (!GET_EQ(victim, WEAR_LEGS)) {
        if ((obj = read_object(1042, VIRTUAL))) {
            obj_to_char(obj, victim);
            perform_wear(victim, obj, WEAR_LEGS);
        } else {
            log("SYSERR: Legging object not found in spell Armor of Gaia");
            return 0;
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_banish) {
    int roll;
    int location, i, to_room;
    bool wasdark;

    if (!ch || !victim)
        return 0;

    /* Is this attack allowed? */
    if (!attack_ok(ch, victim, true))
        return 0;

    if (ch == victim)
        return CAST_RESULT_CHARGE;

    act("You look at $N&0 and shout, '&1&bI banish thee!&0'", false, ch, 0, victim, TO_CHAR);
    act("$n&0 looks at $N&0 and shouts, '&1&bI banish thee!&0'", false, ch, 0, victim, TO_NOTVICT);
    act("$n&0 looks at you and shouts, '&1&bI banish thee!&0'", false, ch, 0, victim, TO_VICT);

    if (MOB_FLAGGED(victim, MOB_NOSUMMON) || MOB_FLAGGED(victim, MOB_NOCHARM) ||
        (EFF_FLAGGED(victim, EFF_CHARM) && victim->master && victim->master->in_room == victim->in_room) ||
        ROOM_FLAGGED(victim->in_room, ROOM_NOSUMMON) || mag_savingthrow(victim, SAVING_SPELL)) {
        act("$N resists you.", false, ch, 0, victim, TO_CHAR);
        act("You resist.", false, ch, 0, victim, TO_VICT);
        if (IS_NPC(victim) && !FIGHTING(victim))
            attack(victim, ch);
    }

    /* min val -99, max val 207; at max skill and max roll and max charisma against a max level victim gives a value of
     * 108 */
    roll = random_number(0, 100) + skill + stat_bonus[GET_CHA(ch)].magic - GET_LEVEL(victim);

    /* Failure */
    if (roll < 50) {
        act("Nothing happens.", false, ch, 0, victim, TO_ROOM);
        char_printf(ch, "Nothing happens.\n");
    }

    act("$N disappears in a flash of light!", false, ch, 0, victim, TO_CHAR);
    act("$N disappears in a flash of light!", false, ch, 0, victim, TO_NOTVICT);
    act("&9&bYou are banished!&0", false, ch, 0, victim, TO_VICT);

    wasdark = IS_DARK(victim->in_room) && !CAN_SEE_IN_DARK(victim);
    dismount_char(victim);

    /* Success */
    if (roll > 100) {
        if (IS_NPC(victim)) {
            roll = random_number(0, 100) + (stat_bonus[GET_WIS(ch)].magic * 2); /* min: 0, max: 114 */
            if (roll > 66) /* 66% chance to wipe victim eq, nears 50% at max wis */
                extract_objects(victim);
            extract_char(victim);
        } else { /* Is a player */
            if ((to_room = real_room(GET_HOMEROOM(victim))) < 0) {
                log("SYSERR: Could not find homeroom for victim of spell 'banish'.");
                to_room = 0;
            }
            char_from_room(victim);
            char_to_room(victim, to_room);
            act("&7&b$n&7&b appears in a flash of light!&0", true, victim, 0, 0, TO_ROOM);
            check_new_surroundings(victim, wasdark, true);
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_charm) {
    effect eff;

    if (victim == nullptr || ch == nullptr)
        return 0;

    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        char_printf(ch,
                    "A flash of white light fills the room, dispelling your "
                    "violent magic!\n");
        act("White light from no particular source suddenly fills the room, "
            "then vanishes.",
            false, ch, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE;
    }

    if (victim == ch)
        char_printf(ch, "You like yourself even better!\n");
    else if (!IS_NPC(victim)) {
        if (charm_allowed == 0)
            if (!PRF_FLAGGED(victim, PRF_SUMMONABLE))
                char_printf(ch, "You fail because SUMMON protection is on!\n");
    } else if (EFF_FLAGGED(victim, EFF_SANCTUARY))
        char_printf(ch, "Your victim is protected by sanctuary!\n");
    else if (MOB_FLAGGED(victim, MOB_NOCHARM))
        char_printf(ch, "Your victim resists!\n");
    /* modded so animateds CAN cast charm *grin* - 321 */
    else if (EFF_FLAGGED(ch, EFF_CHARM) && !MOB_FLAGGED(ch, MOB_ANIMATED))
        char_printf(ch, "You can't have any followers of your own!\n");
    else if (EFF_FLAGGED(victim, EFF_CHARM) || skill < GET_LEVEL(victim) + 10 || MOB_FLAGGED(victim, MOB_ILLUSORY))
        char_printf(ch, "You fail.\n");
    /* player charming another player - no legal reason for this */
    else if (!charm_allowed && !IS_NPC(victim))
        char_printf(ch, "You fail - shouldn't be doing it anyway.\n");
    else if (!attack_ok(ch, victim, false))
        char_printf(ch, "You fail - shouldn't be doing it anyway.\n");
    else if (random_number(1, 100) > susceptibility(victim, DAM_MENTAL)) {
        act("$N resists your charming magic.", false, ch, 0, victim, TO_CHAR);
        act("You resist $n's attempt to charm you.", false, ch, 0, victim, TO_VICT);
    } else if (mag_savingthrow(victim, SAVING_SPELL) || skill - GET_LEVEL(victim) < random_number(0, 200))
        char_printf(ch, "Your victim resists!\n");
    else {
        if (victim->master)
            stop_follower(victim, 0);

        add_follower(victim, ch);

        memset(&eff, 0, sizeof(eff));
        eff.type = SPELL_CHARM;

        if (GET_INT(victim))
            eff.duration = 24 * 18 / GET_INT(victim);
        else
            eff.duration = 24 * 18;

        eff.modifier = 0;
        eff.location = 0;
        SET_FLAG(eff.flags, EFF_CHARM);
        effect_to_char(victim, &eff);

        act("Isn't $n just such a nice fellow?", false, ch, 0, victim, TO_VICT);
        if (IS_NPC(victim)) {
            REMOVE_FLAG(MOB_FLAGS(victim), MOB_AGGRESSIVE);
            REMOVE_FLAG(MOB_FLAGS(victim), MOB_SPEC);
        }

        /* success! skip the mob attacking */
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }
    /* failure.   make the mob attack */
    if (!FIGHTING(victim)) {
        attack(victim, ch);
        remember(victim, ch);
    }

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_cloud_of_daggers) {
    DelayedCastEventObj *event_obj;

    if (!ch)
        return 0;

    act("You animate a handful of daggers into a &9&bswirling cloud of blades!&0", false, ch, 0, victim, TO_CHAR);
    act("$n animates a handful of daggers into a &9&bswirling cloud of blades!&0", false, ch, 0, victim, TO_ROOM);

    event_obj =
        construct_delayed_cast(ch, victim, SPELL_CLOUD_OF_DAGGERS, MAG_AREA, 4, 4 RL_SEC, skill, savetype, false);
    event_create(EVENT_SPELL, delayed_cast_event, event_obj, true, &(ch->events), 4 RL_SEC);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_color_spray) {
    CharData *vict, *next_vict;
    int effect, required;
    char *color;

    if (!ch)
        return 0;

    act("A rainbow of &1c&3o&2l&4o&5r&7 bursts from $n's hands!&0", false, ch, 0, 0, TO_ROOM);
    char_printf(ch, "A rainbow of &1c&3o&2l&4o&5r&7 bursts from your hands!&0\n");
    for (vict = world[ch->in_room].people; vict; vict = next_vict) {
        next_vict = vict->next_in_room;
        if (GET_LEVEL(vict) >= LVL_IMMORT && !IS_NPC(vict))
            continue;
        if (ch == vict)
            continue;
        if (IS_NPC(ch) && IS_NPC(vict))
            continue;
        if (!mass_attack_ok(ch, vict, false))
            continue;

        /* Blind folks immune to this spell. */
        if (EFF_FLAGGED(vict, EFF_BLIND)) {
            act("&7A searing beam&0&7 falls upon $n's face, but $e appears not to "
                "notice.&0",
                true, vict, 0, 0, TO_ROOM);
            continue;
        }

        if (damage_evasion(vict, 0, 0, skills[spellnum].damage_type)) {
            act("$n is unaffected!", false, vict, 0, 0, TO_ROOM);
            act("You are unaffected!", false, vict, 0, 0, TO_CHAR);
            continue;
        }

        mag_damage(skill, ch, vict, SPELL_COLOR_SPRAY, SAVING_SPELL);
        /* Don't hit with an effect if they're already dead. */
        if (DECEASED(vict))
            continue;

        required = random_number(0, 25);
        switch (required) { /* choose color */
        case 1:
            effect = SPELL_MINOR_PARALYSIS;
            required = 17;
            color = "&3yellow";
            break;
        case 2:
            effect = SPELL_CURSE;
            required = 25;
            color = "&1red";
            break;
        case 3:
            effect = SPELL_POISON;
            required = 33;
            color = "&5violet";
            break;
        case 4:
            effect = SPELL_RAY_OF_ENFEEB;
            required = 41;
            color = "&1&bscreaming red";
            break;
        case 5:
            effect = SPELL_SILENCE;
            required = 49;
            color = "&6cyan";
            break;
        case 6:
            effect = SPELL_BLINDNESS;
            required = 56;
            color = "&9&bblack";
            break;
        case 7:
            effect = SPELL_DISEASE;
            required = 65;
            color = "&2green";
            break;
        case 8:
            effect = SPELL_INSANITY;
            required = 73;
            color = "&5&brich violet";
            break;
        case 9:
            effect = SPELL_DISPEL_MAGIC;
            required = 81;
            color = "&4&bblue";
            break;
        case 10:
            effect = -1; /* knock down */
            required = 89;
            color = "&3&bbrilliant yellow";
            break;
        default:
            /* always at least 64% chance to do nothing */
            effect = 0;
            required = 0;
        }

        if (!effect || required > skill)
            continue;

        sprintf(buf, "A dazzling %s beam&0 shines fully in $n's face!&0", color);
        act(buf, false, vict, 0, 0, TO_ROOM);

        if (evades_spell(ch, vict, effect, skill))
            continue;

        /* Special case for knock down. */
        if (effect < 0) {
            switch (random_number(0, 3)) {
            case 1:
                act("$n falls over, dazed!&0", false, vict, 0, 0, TO_ROOM);
                break;
            case 2:
                act("The beam knocks $n off $s feet!&0", false, vict, 0, 0, TO_ROOM);
                break;
            case 3:
                act("$n is crumples to the ground in a daze.&0", false, vict, 0, 0, TO_ROOM);
                break;
            default:
                act("$n falls to the ground, momentarily stunned.&0", false, vict, 0, 0, TO_ROOM);
            }
            sprintf(buf, "A &3shocking&0 %s FLASH&0 makes you lose your balance!&0\n", color);
            char_printf(vict, buf);
            WAIT_STATE(vict, PULSE_VIOLENCE * 3);
            GET_POS(vict) = POS_SITTING;
            GET_STANCE(vict) = STANCE_ALERT;
            continue;
        } else {
            sprintf(buf, "You are dazzled by a %s&0 beam of light!&0\n", color);
            char_printf(vict, buf);

            /* cast the spell effect with half power */
            mag_affect(skill >> 1, ch, vict, effect, SAVING_SPELL, CAST_SPELL);
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_create_water) {
    int amount, liquid_type;

    void setup_drinkcon(ObjData * obj, int newliq);

    if (ch == nullptr || obj == nullptr)
        return 0;

    if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
        amount = std::min(GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY) - GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING),
                          1 + 15 * skill / 2);
        if (amount <= 0) {
            act("$o seems to be full already.", false, ch, obj, 0, TO_CHAR);
        } else {
            if ((GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID) != LIQ_WATER) && (GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING) != 0))
                liquid_type = LIQ_SLIME;
            else
                liquid_type = LIQ_WATER;
            liquid_to_container(obj, amount, liquid_type, false);
            act("Water wells up within $p.", false, ch, obj, 0, TO_CHAR);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }
    }
    return CAST_RESULT_CHARGE;
}

ASPELL(spell_creeping_doom) {
    DelayedCastEventObj *event_obj;

    if (!ch)
        return 0;

    act("&9&bYou send out an endless wave of crawling&0 &1arachnids&0 &9&band&0 "
        "&2insects&0...",
        false, ch, 0, 0, TO_CHAR);
    act("&9&b$n&9&b sends out an endless wave of crawling&0 &1arachnids&0 "
        "&9&band&0 &2insects&0...",
        false, ch, 0, 0, TO_ROOM);

    event_obj = construct_delayed_cast(ch, nullptr, SPELL_DOOM, MAG_AREA, 3, 3 RL_SEC, skill, savetype, false);
    event_create(EVENT_SPELL, delayed_cast_event, event_obj, true, &(ch->events), 3 RL_SEC);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_dark_feast) {
    if (!ch || !obj)
        return 0;

    if (!IS_CORPSE(obj)) {
        char_printf(ch, "That is not a corpse!\n");
        return CAST_RESULT_CHARGE;
    }

    act("&9&bYou gorge yourself on $p.&0", false, ch, obj, 0, TO_CHAR);
    act("&9&b$n&9&b gorges $mself on $p.&0", true, ch, obj, 0, TO_ROOM);
    gain_condition(ch, FULL, 24);
    gain_condition(ch, THIRST, 24);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_darkness) {
    RoomEffectNode *reff;

    int ticks;
    long eff;

    eff = ticks = 0;

    if (!ch)
        return 0;

    if (obj != nullptr) { /* De-Light an obj */
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING)) {

            /* Normally, val 1 would already have val 2 in there.
             * But this might be a saved item from before we did that.
             * So copy light-time into value 1 - that means this item
             * can be returned to perma-light status, if it had it. */
            if (!GET_OBJ_VAL(obj, VAL_LIGHT_CAPACITY))
                GET_OBJ_VAL(obj, VAL_LIGHT_CAPACITY) = GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING);

            /* Now kill its potential: */
            GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) = 0;

            act("You drain $p's energy.", false, ch, obj, 0, TO_CHAR);

            /* Now make sure it's off */
            if (GET_OBJ_VAL(obj, VAL_LIGHT_LIT)) {
                GET_OBJ_VAL(obj, VAL_LIGHT_LIT) = false;
                /* lost light to the room */
                world[ch->in_room].light--;
                act("$p quickly &bsputters &9&bout.&0.", false, ch, obj, 0, TO_CHAR);
                act("$p quickly &bsputters &9&bout.&0.", false, ch, obj, 0, TO_ROOM);
            }
        } else {
            char_printf(ch, NOEFFECT);
            return CAST_RESULT_CHARGE;
        }
    } else { /* De-Light the room */
        if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_ILLUMINATION)) {
            act("You dispel the magical light.", true, ch, 0, 0, TO_CHAR);
            act("$n dispels the magical light.", true, ch, 0, 0, TO_ROOM);
            REMOVE_FLAG(ROOM_EFFECTS(ch->in_room), ROOM_EFF_ILLUMINATION);
            eff = 0;
            world[ch->in_room].light--;
        } else if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS)) {
            char_printf(ch, NOEFFECT);
            return CAST_RESULT_CHARGE;
        } else {
            act("You engulf the area in a magical darkness!", true, ch, 0, 0, TO_CHAR);
            act("$n engulfs the area in a magical darkness!", true, ch, 0, 0, TO_ROOM);
            eff = ROOM_EFF_DARKNESS;
            world[ch->in_room].light--;
            ticks = 4 + 20 * skill / 100; /* Lasts 4-24 hours */

            /* create, initialize, and link a room-effect node */
            CREATE(reff, RoomEffectNode, 1);
            reff->room = ch->in_room;
            reff->timer = ticks;
            reff->effect = eff;
            reff->spell = SPELL_DARKNESS;
            reff->next = room_effect_list;
            room_effect_list = reff;

            /* set the affection */
            if (eff != 0)
                SET_FLAG(ROOM_EFFECTS(reff->room), eff);
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_degeneration) {

    if (!ch || !victim)
        return 0;

    if (damage_evasion(victim, ch, 0, skills[spellnum].damage_type)) {
        act("$n is unaffected!", false, victim, 0, 0, TO_ROOM);
        act("You are unaffected!", false, victim, 0, 0, TO_CHAR);
        return CAST_RESULT_CHARGE;
    }

    if (GET_LIFEFORCE(victim) == LIFE_UNDEAD) {
        mag_point(skill, ch, victim, SPELL_HEAL, savetype);
        act("You cause magical degeneration in $N.", false, ch, 0, victim, TO_CHAR);
        act("$n causes magical degeneration in $N.", true, ch, 0, victim, TO_ROOM);
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }

    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        char_printf(ch, "A flash of white light fills the room, dispelling your violent magic!\n");
        act("White light from no particular source suddenly fills the room, then vanishes.", false, ch, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE;
    }

    return mag_damage(skill, ch, victim, SPELL_DEGENERATION, savetype);
}

ASPELL(spell_dimension_door) {
    int czone, tzone;
    bool wasdark;

    if (ch == nullptr || victim == nullptr)
        return 0;

    /* Dim Door should only work to players in your zone */
    if (IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOFOLLOW) || GET_LEVEL(victim) >= LVL_IMMORT) {
        char_printf(ch, "You failed.\n");
        return CAST_RESULT_CHARGE;
    }
    if (victim->in_room == -1) {
        char_printf(ch, "You failed.\n");
        WAIT_STATE(ch, PULSE_VIOLENCE);
        log("SYSERR: {} tried to dimension door to {} in room -1.", GET_NAME(ch), GET_NAME(victim));
        return CAST_RESULT_CHARGE;
    }
    czone = world[ch->in_room].zone;
    tzone = world[victim->in_room].zone;
    if (czone == tzone) {
        dismount_char(ch);
        act("&9&bYou quickly enter the black rift that tears open at your "
            "command...&0",
            false, ch, 0, victim, TO_CHAR);
        act("&9&bA black rift in space tears open and $n&9&b steps inside.&0", true, ch, 0, victim, TO_ROOM);
        wasdark = IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch);

        char_from_room(ch);
        char_to_room(ch, victim->in_room);

        act("&9&bA black rift in space tears open and $n&9&b steps out grinning.&0", true, ch, 0, 0, TO_ROOM);
        check_new_surroundings(ch, wasdark, true);
    } else {
        act("&9&bYour magics are not strong enough for such a great journey.&0", false, ch, 0, 0, TO_CHAR);
        act("&9&bA rift in space opens up, wavers, then dissipates into thin "
            "air.&0",
            false, ch, 0, 0, TO_ROOM);
    }
    if (GET_LEVEL(ch) >= LVL_IMMORT)
        WAIT_STATE(ch, PULSE_VIOLENCE * 4);
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_enchant_weapon) {
    int i;

    if (ch == nullptr || obj == nullptr)
        return 0;

    if ((GET_OBJ_TYPE(obj) == ITEM_WEAPON) && !OBJ_FLAGGED(obj, ITEM_MAGIC)) {

        for (i = 0; i < MAX_OBJ_APPLIES; i++)
            if (obj->applies[i].location != APPLY_NONE)
                return CAST_RESULT_CHARGE;

        SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_MAGIC);
        SET_FLAG(GET_OBJ_EFF_FLAGS(obj), EFF_BLESS);

        obj->applies[0].location = APPLY_HITROLL;
        obj->applies[0].modifier = 1 + (skill >= 18);

        obj->applies[1].location = APPLY_DAMROLL;
        obj->applies[1].modifier = 1 + (skill >= 20);

        if (IS_GOOD(ch)) {
            SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_ANTI_EVIL);
            act("$p glows blue.", false, ch, obj, 0, TO_CHAR);
        } else if (IS_EVIL(ch)) {
            SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_ANTI_GOOD);
            act("$p glows red.", false, ch, obj, 0, TO_CHAR);
        } else {
            act("$p glows yellow.", false, ch, obj, 0, TO_CHAR);
        }
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }
    return CAST_RESULT_CHARGE;
}

ASPELL(spell_energy_drain) {
    int dam, hp;

    if (!ch || !victim)
        return 0;

    if (ch == victim) {
        act("Draining yourself?   My, aren't we funny today...", false, ch, 0, 0, TO_CHAR);
        act("$n grabs $mself by the skull and wiggles in pain!", true, ch, 0, 0, TO_ROOM);
        GET_HIT(ch) *= 0.75;
        return CAST_RESULT_CHARGE;
    }

    if (GET_LIFEFORCE(victim) == LIFE_UNDEAD) {
        act("$N seems far too lifeless to drain any energy from $M.", false, ch, 0, victim, TO_CHAR);
        return CAST_RESULT_CHARGE;
    }

    if (damage_evasion(victim, 0, 0, skills[spellnum].damage_type)) {
        act("$n is unaffected!", false, victim, 0, 0, TO_ROOM);
        act("You are unaffected!", false, victim, 0, 0, TO_CHAR);
        return CAST_RESULT_CHARGE;
    }

    dam = normal_random(3 * skill, skill / 3);
    dam = dam * susceptibility(victim, skills[spellnum].damage_type) / 100;

    hp = GET_HIT(victim);
    damage(ch, victim, dam, SPELL_ENERGY_DRAIN);

    /* Actual damage done: */
    dam = hp - GET_HIT(victim);

    if (GET_HIT(ch) < GET_MAX_HIT(ch))
        GET_HIT(ch) += dam;
    else if (dam) {
        float ratio = GET_HIT(ch) / GET_MAX_HIT(ch);
        /* Polynomial that approaches zero at a max hp / hp ratio of about 5 */
        hp = dam * (-0.0457 * ratio * ratio - 0.0171 * ratio + 1.066);
        if (hp > 10)
            GET_HIT(ch) += hp;
        else
            GET_HIT(ch) += random_number(5, 10);
    }

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_enlightenment) {

    if (!ch || !victim)
        return 0;

    act("You attempt to glean information about $N...", false, ch, 0, victim, TO_CHAR);
    act("$n&0 attempts to stare into your mind.", true, ch, 0, victim, TO_VICT);
    if (mag_savingthrow(victim, SAVING_SPELL) || GET_LEVEL(victim) > LVL_IMMORT ||
        GET_LEVEL(victim) > GET_LEVEL(ch) + 5) {
        act("$N&0 resists your attempt!", false, ch, 0, victim, TO_CHAR);
        if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT)
            WAIT_STATE(ch, PULSE_VIOLENCE * 4);
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }
    act("$N&0's mental defenses fail, letting you peer into $S mind.", false, ch, 0, victim, TO_CHAR);
    if (IS_NPC(victim)) {
        char_printf(ch, "Race: {}\nClass: {}\nLevel: {}\n", RACE_PLAINNAME(victim), CLASS_PLAINNAME(victim),
                    GET_LEVEL(victim));
    } else {
        char_printf(ch, "Race: {}\nClass: {}\nLevel: {}\n", RACE_ABBR(victim), CLASS_WIDE(victim), GET_LEVEL(victim));
    }
    if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT)
        WAIT_STATE(ch, PULSE_VIOLENCE * 4);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_flame_blade) {
    ObjData *sword;
    int hands, weapons;

    if (!ch)
        return 0;

    count_hand_eq(ch, &hands, &weapons);
    if (hands > 1 || weapons) {
        char_printf(ch, "Your hands are not free to wield the blade.\n");
        return CAST_RESULT_CHARGE;
    }
    act("&1&b$n&1&b summons a blade of &0&1pure flame&b to aid $m.&0", true, ch, 0, 0, TO_ROOM);
    act("&1&bYou summon a blade of &0&1pure flame&b to aid you.&0", false, ch, 0, 0, TO_CHAR);
    if ((sword = read_object(1043, VIRTUAL)) == nullptr) {
        log("SYSERR: Blade object not found in spell flame blade.");
        return 0;
    }
    obj_to_char(sword, ch);
    perform_wear(ch, sword, WEAR_WIELD);
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_flood) {
    ObjData *tobj, *next_tobj;
    CharData *vict, *next_vict;

    if (!ch)
        return 0;

    act("&7&b$n&7&b calls forth a mighty &4FLOOD...&0", false, ch, 0, 0, TO_ROOM);
    act("&7&bYou release a mighty &4FLOOD...&0", false, ch, 0, 0, TO_CHAR);
    if (!MOB_FLAGGED(ch, MOB_ILLUSORY))
        for (tobj = world[ch->in_room].contents; tobj; tobj = next_tobj) {
            next_tobj = tobj->next_content;
            if (!OBJ_FLAGGED(tobj, ITEM_FLOAT) && !OBJ_FLAGGED(tobj, ITEM_MAGIC) && CAN_WEAR(tobj, ITEM_WEAR_TAKE) &&
                GET_OBJ_TYPE(tobj) != ITEM_WALL && !IS_CORPSE(tobj)) {
                act("$p &4is washed away by the huge current of water.&0", false, ch, tobj, 0, TO_CHAR);
                act("$p &4is washed away by the huge current of water.&0", false, ch, tobj, 0, TO_ROOM);
                extract_obj(tobj);
            }
        }
    for (vict = world[ch->in_room].people; vict; vict = next_vict) {
        next_vict = vict->next_in_room;
        if (GET_LEVEL(vict) >= LVL_IMMORT && !IS_NPC(vict))
            continue;
        if (ch == vict)
            continue;
        if (is_grouped(ch, vict))
            continue;
        mag_damage(skill, ch, vict, SPELL_FLOOD, SAVING_SPELL);
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_fracture) {
    FollowType *fol;
    char *argument;

    if (!ch)
        return 0;

    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        char_printf(ch, "A flash of white light fills the room, dispelling your violent magic!\n");
        act("White from no particular source suddenly fills the room, then vanishes.", false, victim, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE;
    }

    if (ch->casting.misc && *ch->casting.misc) {
        argument = ch->casting.misc;
        skip_spaces(&argument);
        one_argument(argument, arg);
        victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg));
    }

    if (victim) {
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        if (!RIGID(victim) || !(IS_HUMANOID(victim) || GET_RACE(victim) == RACE_ANIMAL ||
                                GET_RACE(victim) == RACE_DRAGON_GAS || GET_RACE(victim) == RACE_DRAGON_GENERAL ||
                                GET_RACE(victim) == RACE_DRAGON_FIRE || GET_RACE(victim) == RACE_DRAGON_ACID ||
                                GET_RACE(victim) == RACE_DRAGON_FROST || GET_RACE(victim) == RACE_DRAGON_LIGHTNING)) {
            act("You can't shatter $N!", false, ch, 0, victim, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }
    } else {
        /* Find a solidish zombie/pet */
        for (fol = ch->followers; fol; fol = fol->next)
            if (RIGID(fol->follower) &&
                (IS_HUMANOID(fol->follower) || GET_RACE(fol->follower) == RACE_ANIMAL ||
                 GET_RACE(fol->follower) == RACE_DRAGON_GAS || GET_RACE(fol->follower) == RACE_DRAGON_GENERAL ||
                 GET_RACE(fol->follower) == RACE_DRAGON_FIRE || GET_RACE(fol->follower) == RACE_DRAGON_ACID ||
                 GET_RACE(fol->follower) == RACE_DRAGON_FROST || GET_RACE(fol->follower) == RACE_DRAGON_LIGHTNING) &&
                EFF_FLAGGED(fol->follower, EFF_CHARM) && IN_ROOM(fol->follower) == IN_ROOM(ch)) {
                victim = fol->follower;
                break;
            }
        if (!victim) {
            char_printf(ch, "You look around, but can't find an acceptable body.\n");
            return CAST_RESULT_CHARGE;
        }
    }

    ch = EFF_FLAGGED(victim, EFF_CHARM) && victim->master == ch ? victim : ch;

    mag_damage(skill, ch, victim, SPELL_FRACTURE, savetype);

    mag_area(skill, victim, SPELL_FRACTURE_SHRAPNEL, savetype);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_heavens_gate) {
    ObjData *portal, *tportal;
    ExtraDescriptionData *new_descr, *new_tdescr;

    if (ch == nullptr || victim == nullptr)
        return 0;
    if (IS_NPC(victim)) {
        char_printf(ch, "You cannot target NPCs with this spell.\n");
        return 0;
    }
    if (victim->in_room == 0)
        return 0;
    if (victim->in_room == -1) {
        char_printf(ch, "You failed.\n");
        WAIT_STATE(ch, PULSE_VIOLENCE);
        log("SYSERR: {} tried to heavens gate to {} in room -1.", GET_NAME(ch), GET_NAME(victim));
        return CAST_RESULT_CHARGE;
    }
    if ((ROOM_FLAGGED(ch->in_room, ROOM_NOWELL) || ROOM_FLAGGED(victim->in_room, ROOM_NOWELL)) ||
        PRF_FLAGGED(victim, PRF_NOFOLLOW)) {
        act("&7A&0 &6&btunnel of &7light&0 &7 appears briefly, then vanishes.&0", true, ch, 0, 0, TO_ROOM);
        act("&7A&0 &6&btunnel of &7light&0 &7 appears briefly, then vanishes.&0", true, ch, 0, 0, TO_CHAR);
        act("&7A&0 &6&btunnel of &7light&0 &7 appears briefly, then vanishes.&0", true, ch, 0, 0, TO_VICT);
        return CAST_RESULT_CHARGE;
    }
    if ((portal = read_object(OBJ_VNUM_HEAVENSGATE, VIRTUAL)) == nullptr) {
        log("SYSERR: Heaven's gate portal object not valid!");
        return 0;
    }
    GET_OBJ_VAL(portal, VAL_PORTAL_DESTINATION) = world[victim->in_room].vnum;
    GET_OBJ_DECOMP(portal) = 2;
    CREATE(new_descr, ExtraDescriptionData, 1);
    new_descr->keyword = strdup("tunnel light portal");
    sprintf(buf, "You can barely make out %s.\n", world[victim->in_room].name.c_str());
    new_descr->description = strdup(buf);
    new_descr->next = portal->ex_description;
    portal->ex_description = new_descr;
    obj_to_room(portal, ch->in_room);
    act("&7&b$n&7&b calls upon his deity...&0\n"
        "&6A soft &btunnel of &7light&6 opens up near you, &0&6beckoning you to enter.",
        true, ch, 0, 0, TO_ROOM);
    act("&7&bYou call upon your deity...&0\n"
        "&6A soft &btunnel of &7light &6opens up near you, &6beckoning you to enter.&0",
        true, ch, 0, 0, TO_CHAR);
    /* create the portal at the other end */
    tportal = read_object(OBJ_VNUM_HEAVENSGATE, VIRTUAL);
    GET_OBJ_VAL(tportal, VAL_PORTAL_DESTINATION) = world[ch->in_room].vnum;
    GET_OBJ_DECOMP(tportal) = 2;
    CREATE(new_tdescr, ExtraDescriptionData, 1);
    new_tdescr->keyword = strdup("tunnel light portal");
    sprintf(buf, "You can barely make out %s.\n", world[ch->in_room].name.c_str());
    new_tdescr->description = strdup(buf);
    new_tdescr->next = tportal->ex_description;
    tportal->ex_description = new_tdescr;
    obj_to_room(tportal, victim->in_room);
    act("&6A soft &btunnel of &7light &6opens up near you, &6beckoning you to enter.&0", true, victim, 0, 0, TO_ROOM);
    act("&6A soft &btunnel of &7light &6opens up near you, &6beckoning you to enter.&0", true, victim, 0, 0, TO_CHAR);
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_hells_gate) {
    ObjData *portal, *tportal;
    ExtraDescriptionData *new_descr, *new_tdescr;

    if (ch == nullptr || victim == nullptr)
        return 0;
    if (IS_NPC(victim)) {
        char_printf(ch, "You cannot target NPCs with this spell.\n");
        return 0;
    }
    if (victim->in_room == 0)
        return 0;
    if (victim->in_room == -1) {
        char_printf(ch, "You failed.\n");
        WAIT_STATE(ch, PULSE_VIOLENCE);
        log("SYSERR: {} tried to hell gate to {} in room -1.", GET_NAME(ch), GET_NAME(victim));
        return CAST_RESULT_CHARGE;
    }
    if ((ROOM_FLAGGED(ch->in_room, ROOM_NOWELL) || ROOM_FLAGGED(victim->in_room, ROOM_NOWELL)) ||
        PRF_FLAGGED(victim, PRF_NOFOLLOW)) {
        act("&9&bThe ground begins to quake and open up, briefly revealing &1hell&9, then closes.&0", true, ch, 0, 0,
            TO_ROOM);
        act("&9&bThe ground begins to quake and open up, briefly revealing &1hell&9, then closes.&0", true, ch, 0, 0,
            TO_CHAR);
        act("&9&bThe ground begins to quake and open up, briefly revealing &1hell&9, then closes.&0", true, ch, 0, 0,
            TO_VICT);
        return CAST_RESULT_CHARGE;
    }
    if ((portal = read_object(OBJ_VNUM_HELLGATE, VIRTUAL)) == nullptr) {
        log("SYSERR: Hell's gate portal object not valid!");
        return 0;
    }
    GET_OBJ_VAL(portal, VAL_PORTAL_DESTINATION) = world[victim->in_room].vnum;
    GET_OBJ_DECOMP(portal) = 2;
    CREATE(new_descr, ExtraDescriptionData, 1);
    new_descr->keyword = strdup("portal hole gate");
    sprintf(buf, "You can barely make out %s.\n", world[victim->in_room].name.c_str());
    new_descr->description = strdup(buf);
    new_descr->next = portal->ex_description;
    portal->ex_description = new_descr;
    obj_to_room(portal, ch->in_room);
    act("&1A &9&bdark hole in the &0&3earth&1 opens up to &bhell.&0", true, ch, 0, 0, TO_ROOM);
    act("&1A &9&bdark hole in the &0&3earth&1 opens up to &bhell.&0", true, ch, 0, 0, TO_CHAR);
    /* create the portal at the other end */
    tportal = read_object(OBJ_VNUM_HELLGATE, VIRTUAL);
    GET_OBJ_VAL(tportal, VAL_PORTAL_DESTINATION) = world[ch->in_room].vnum;
    GET_OBJ_DECOMP(tportal) = 2;
    CREATE(new_tdescr, ExtraDescriptionData, 1);
    new_tdescr->keyword = strdup("portal hole gate");
    sprintf(buf, "You can barely make out %s.\n", world[ch->in_room].name.c_str());
    new_tdescr->description = strdup(buf);
    new_tdescr->next = tportal->ex_description;
    tportal->ex_description = new_tdescr;
    obj_to_room(tportal, victim->in_room);
    act("&1A &9&bdark hole in the &0&3earth&1 opens up to &bhell.&0", true, victim, 0, 0, TO_ROOM);
    act("&1A &9&bdark hole in the &0&3earth&1 opens up to &bhell.&0", true, victim, 0, 0, TO_CHAR);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_ice_dagger) {
    ObjData *dagger;
    int hands, weapons;

    if (!ch)
        return 0;
    count_hand_eq(ch, &hands, &weapons);
    if (hands > 1 || weapons) {
        char_printf(ch, "Your hands are not free to wield the dagger.\n");
        return CAST_RESULT_CHARGE;
    }
    act("&7&b$n&7&b summons a dagger of &0&4glimmering ice&7&b to aid $m.&0", true, ch, 0, 0, TO_ROOM);
    act("&7&bYou summon a blade of &0&4glimmering ice&7&b to aid you.&0", false, ch, 0, 0, TO_CHAR);
    if ((dagger = read_object(1047, VIRTUAL)) == nullptr) {
        log("SYSERR: Dagger object not found in spell ice dagger.");
        return 0;
    }
    obj_to_char(dagger, ch);
    perform_wear(ch, dagger, WEAR_WIELD);
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_identify) {
    void identify_obj(ObjData * obj, CharData * ch, int location);

    if (obj) {
        char_printf(ch, "You feel informed:\n");
        identify_obj(obj, ch, 0);
    } else if (victim) { /* victim */
        char_printf(ch, "Name: {}\n", GET_NAME(victim));
        if (!IS_NPC(victim)) {
            char_printf(ch, "{} is {:d} years, {:d} months, {:d} days and {:d} hours old.\n", GET_NAME(victim),
                        age(victim).year, age(victim).month, age(victim).day, age(victim).hours);
        }
        char_printf(ch, "Height {}; Weight {}\n", statelength(GET_HEIGHT(victim)), stateweight(GET_WEIGHT(victim)));
        /*      sprintf(buf, "%sLevel: %d, Hits: %d, Mana: %d\n", buf,
           GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
           sprintf(buf, "%sAC: %d, Hitroll: %d, Damroll: %d\n", buf,
           GET_AC(victim), GET_HITROLL(victim), GET_DAMROLL(victim));
           sprintf(buf, "%sStr: %d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha:
           %d\n", buf, GET_STR(victim), GET_INT(victim), GET_WIS(victim),
           GET_DEX(victim), GET_CON(victim), GET_CHA(victim));
         */
        sprintf(buf, "$E is composed of %s%s&0, and $S nature is %s%s.", COMPOSITION_COLOR(victim),
                COMPOSITION_NAME(victim), LIFEFORCE_COLOR(victim), LIFEFORCE_NAME(victim));
        act(buf, false, ch, 0, victim, TO_CHAR);
    } else {
        return CAST_RESULT_CHARGE;
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_illumination) {
    RoomEffectNode *reff;

    int ticks;
    long eff;

    eff = ticks = 0;

    if (!ch)
        return CAST_RESULT_CHARGE;

    if (obj != nullptr) { /* Light an obj */
        if (GET_OBJ_TYPE(obj) != ITEM_LIGHT) {
            /* The item isn't a LIGHT item */
            char_printf(ch, NOEFFECT);
            return CAST_RESULT_CHARGE;
        }
        if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT) {
            /* The item is a permanent light. */
            act("The power of your magic washes uselessly over the surface of $o.", false, ch, obj, 0, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }

        /* We know that it is a LIGHT and that its time left is 0 -
         * it's a normal, burnt out light. */

        /* Make use of the stored maximum-light-time of this object, if available */
        ticks = GET_OBJ_VAL(obj, VAL_LIGHT_CAPACITY) * 2;

        if (ticks < 1)
            ticks = 50;

        /* Ah, but the value we just put in ticks is for one who has
         * mastered the proficiency associated with this spell... */

        ticks = (ticks / 2) + (ticks * skill / 100);

        /* rejuvenate light */
        GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) = ticks;

        /* But... if it was permanent once, it's fixed again!
         * Note: the spell of darkness could have extinguished it. */
        if (GET_OBJ_VAL(obj, VAL_LIGHT_CAPACITY) == LIGHT_PERMANENT)
            GET_OBJ_VAL(obj, 2) = LIGHT_PERMANENT;

        /* Make it lit */

        /* Only if the light isn't lit */
        if (!GET_OBJ_VAL(obj, VAL_LIGHT_LIT)) {
            /* make it lit now */
            GET_OBJ_VAL(obj, VAL_LIGHT_LIT) = true;
            /* light in the room */
            world[ch->in_room].light++;
        }

        act("$p begins glowing with a &3&bbright yellow light&0.", false, ch, obj, 0, TO_CHAR);
        act("$p begins glowing with a &3&bbright yellow light&0.", false, ch, obj, 0, TO_ROOM);
    } else { /* Light the room */
        if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS)) {
            act("Your magical light dispels the darkness.&0", false, ch, 0, 0, TO_CHAR);
            act("$n's magical light dispels the darkness.&0", false, ch, 0, 0, TO_ROOM);
            REMOVE_FLAG(ROOM_EFFECTS(ch->in_room), ROOM_EFF_DARKNESS);
            eff = 0;
            world[ch->in_room].light++;

            /* This will prevent the spell from stacking, for now.   It might be
             * preferable for the new spell to displace one that had less time
             * left on it than this new one will provide.
             *
             * Without this check, if two spells are cast in quick succession,
             * world[ch->in_room].light++ gets called twice, but the effect
             * will only wear off once -- leading to a permanently lit room!
             */
        } else if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_ILLUMINATION)) {
            char_printf(ch, NOEFFECT);
            return CAST_RESULT_CHARGE;
        } else {
            act("The room magically lights up!&0", false, ch, 0, 0, TO_CHAR);
            act("The room magically lights up!&0", false, ch, 0, 0, TO_ROOM);
            eff = ROOM_EFF_ILLUMINATION;
            world[ch->in_room].light++;
            ticks = 4 + 20 * skill / 100; /* Lasts 4-24 hours */

            /* create, initialize, and link a room-effect node */
            CREATE(reff, RoomEffectNode, 1);
            reff->room = ch->in_room;
            reff->timer = ticks;
            reff->effect = eff;
            reff->spell = SPELL_ILLUMINATION;
            reff->next = room_effect_list;
            room_effect_list = reff;

            /* set the affection */
            if (eff != 0)
                SET_FLAG(ROOM_EFFECTS(reff->room), eff);
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_immolate) {
    DelayedCastEventObj *event_obj;

    if (!ch || !victim)
        return 0;

    act("&1You spew a &bconflagration of flame&0&1 at $N.&0", false, ch, 0, victim, TO_CHAR);
    act("&1$n&1 calls down a &bconflagration of flame&0&1 on $N.&0", false, ch, 0, victim, TO_NOTVICT);
    act("&1$n&1 calls down a &bconflagration of flame&0&1 on you.&0", false, ch, 0, victim, TO_VICT);

    event_obj = construct_delayed_cast(ch, victim, SPELL_IMMOLATE, MAG_DAMAGE, 5, 5 RL_SEC, skill, savetype, false);
    event_create(EVENT_SPELL, delayed_cast_event, event_obj, true, &(ch->events), 5 RL_SEC);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_isolation) {
    RoomEffectNode *reff;

    if (!ch)
        return CAST_RESULT_CHARGE;

    /* You can't cast this spell in peaceful rooms. */

    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        char_printf(ch, "A flash of light appears, dispelling your spell!\n");
        act("Bright light suddenly fills the room, "
            "then vanishes.",
            false, ch, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE;
    }

    if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_ISOLATION)) {
        char_printf(ch, NOEFFECT);
        return CAST_RESULT_CHARGE;
    } else {
        room_printf(ch->in_room, "&bA wide and diffuse veil of sorts descends upon the area.&0\n");
        CREATE(reff, RoomEffectNode, 1);

        reff->room = ch->in_room;
        reff->timer = 3 + skill / 30; /* Lasts 3-6 hours */
        reff->effect = ROOM_EFF_ISOLATION;
        reff->spell = SPELL_ISOLATION;
        reff->next = room_effect_list;

        room_effect_list = reff;

        SET_FLAG(ROOM_EFFECTS(reff->room), ROOM_EFF_ISOLATION);
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(chant_ivory_symphony) {
    CharData *tch, *next_tch;
    bool found = false;
    bool is_dark;

    if (ch->in_room == NOWHERE)
        return 0;

    char_printf(ch, "You sing a flowing ivory song...\n");
    act("$n sings a strangely alarming tune...", false, ch, 0, 0, TO_ROOM);

    is_dark = IS_DARK(ch->in_room);

    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;
        if (tch == ch)
            continue;
        if (!attack_ok(ch, tch, false))
            continue;

        found = true;

        if (!AWAKE(tch))
            continue;
        if (is_grouped(ch, tch))
            continue;
        if (!is_dark && mag_savingthrow(tch, SAVING_PARA))
            continue;
        if (MOB_FLAGGED(tch, MOB_SENTINEL) && mag_savingthrow(tch, SAVING_PARA))
            continue;
        if (random_number(0, 100) > skill)
            continue;

        delayed_command(tch, "flee", 0, false);
    }

    if (found)
        return CAST_RESULT_IMPROVE | CAST_RESULT_CHARGE;
    else
        return CAST_RESULT_CHARGE;
}

ASPELL(spell_lightning_breath) {
    if (!ch || !victim)
        return 0;

    if (attack_ok(ch, victim, false)) {
        if (damage_evasion(victim, 0, 0, skills[spellnum].damage_type)) {
            act("$n is unaffected!", false, victim, 0, 0, TO_ROOM);
            act("You are unaffected!", false, victim, 0, 0, TO_CHAR);
        } else if (random_number(0, 100) < GET_LEVEL(ch) && !mag_savingthrow(victim, SAVING_PARA)) {
            act("$n is stunned!", false, victim, 0, 0, TO_ROOM);
            act("You are stunned!", false, victim, 0, 0, TO_CHAR);
            GET_POS(victim) = POS_PRONE;
            GET_STANCE(victim) = STANCE_STUNNED;
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/* Quite possibly the most elaborate magic missile spell on the net */

ASPELL(spell_magic_missile) {
    int i;
    int missiles = 0;
    int min_missiles = 1;

    if (!ch || !victim)
        return 0;

    /*
     * This graduates the number of missiles cast based on prof of the sphere *
     */

    if (skill >= 5 && random_number(1, 100) > 80)
        missiles++;
    if (skill >= 14 && random_number(1, 100) > 75)
        missiles++;
    if (skill >= 24 && random_number(1, 100) > 60)
        missiles++;
    if (skill >= 34 && random_number(1, 100) > 55)
        missiles++;
    if (skill >= 44 && random_number(1, 100) > 50)
        missiles++;
    if (skill >= 74 && random_number(1, 100) > 25)
        missiles++;

    missiles += min_missiles;

    /* this casts the spell once for each missile */

    for (i = 1; i <= missiles; i++) {

        if (ALIVE(victim))
            mag_damage(skill, ch, victim, SPELL_MAGIC_MISSILE, SAVING_SPELL);
        else {
            act("$n pelts $N's dead body with magical bolts.", true, ch, 0, victim, TO_ROOM);
            act("You're pelting $M with missiles but $E's dead already!", false, ch, 0, victim, TO_CHAR);
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_fire_darts) {
    int i;
    int missiles = 0;
    int min_missiles = 1;

    if (!ch || !victim)
        return 0;

    /*
     * This graduates the number of missiles cast based on prof of the sphere
     */
    if (skill >= 5 && random_number(1, 100) > 80)
        missiles++;
    if (skill >= 12 && random_number(1, 100) > 75)
        missiles++;
    if (skill >= 24 && random_number(1, 100) > 60)
        missiles++;
    if (skill >= 34 && random_number(1, 100) > 55)
        missiles++;
    if (skill >= 44 && random_number(1, 100) > 50)
        missiles++;
    if (skill >= 74 && random_number(1, 100) > 25)
        missiles++;

    missiles += min_missiles;

    /* this casts the spell once for each missile */

    for (i = 1; i <= missiles; i++) {

        /* Check for live victim RSD 3/18/00 */

        if (ALIVE(victim))
            mag_damage(skill, ch, victim, SPELL_FIRE_DARTS, SAVING_SPELL);
        else {
            act("$n pelts $N's dead body with magical bolts.", true, ch, 0, victim, TO_ROOM);
            act("You're pelting $M with missiles but $E's dead already!", false, ch, 0, victim, TO_CHAR);
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_ice_darts) {
    int i;
    int missiles = 0;
    int min_missiles = 1;

    if (!ch || !victim)
        return 0;

    /*
     * This graduates the number of missiles cast based on prof of the sphere *
     */
    if (skill >= 5 && random_number(1, 100) > 80)
        missiles++;
    if (skill >= 14 && random_number(1, 100) > 75)
        missiles++;
    if (skill >= 24 && random_number(1, 100) > 60)
        missiles++;
    if (skill >= 34 && random_number(1, 100) > 55)
        missiles++;
    if (skill >= 44 && random_number(1, 100) > 50)
        missiles++;
    if (skill >= 74 && random_number(1, 100) > 25)
        missiles++;

    missiles += min_missiles;

    /* this casts the spell once for each missile */

    for (i = 1; i <= missiles; i++) {
        if (ALIVE(victim))
            mag_damage(skill, ch, victim, SPELL_ICE_DARTS, SAVING_SPELL);
        else {
            act("$n pelts $N's dead body with magical bolts.", true, ch, 0, victim, TO_ROOM);
            act("You're pelting $M with missiles but $E's dead already!", false, ch, 0, victim, TO_CHAR);
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_spirit_arrows) {
    int i;
    int missiles = 0;
    int min_missiles = 1;

    if (!ch || !victim)
        return 0;

    /*
     * This graduates the number of missiles cast based on prof of the sphere *
     */

    if (skill >= 5 && random_number(1, 100) > 80)
        missiles++;
    if (skill >= 14 && random_number(1, 100) > 75)
        missiles++;
    if (skill >= 24 && random_number(1, 100) > 60)
        missiles++;
    if (skill >= 34 && random_number(1, 100) > 55)
        missiles++;
    if (skill >= 44 && random_number(1, 100) > 50)
        missiles++;
    if (skill >= 74 && random_number(1, 100) > 25)
        missiles++;

    missiles += min_missiles;

    /* this casts the spell once for each missile */

    for (i = 1; i <= missiles; i++) {

        if (ALIVE(victim))
            mag_damage(skill, ch, victim, SPELL_SPIRIT_ARROWS, SAVING_SPELL);
        else {
            act("$n pelts $N's dead body with magical arrows.", true, ch, 0, victim, TO_ROOM);
            act("You're pelting $M with arrows but $E's dead already!", false, ch, 0, victim, TO_CHAR);
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_major_paralysis) {
    effect eff;

    if (victim == nullptr || ch == nullptr)
        return 0;
    if (!attack_ok(ch, victim, true))
        return CAST_RESULT_CHARGE;
    if (mag_savingthrow(victim, SAVING_PARA) || skill - GET_LEVEL(victim) > random_number(0, 70) ||
        MOB_FLAGGED(victim, MOB_NOCHARM)) {

        if (MOB_FLAGGED(victim, MOB_NOCHARM))
            act("&7&b$N cannot be paralyzed!&0", false, ch, 0, victim, TO_CHAR);
        else
            act("&7&b$N resists your attempt to paralyze $M.&0", false, ch, 0, victim, TO_CHAR);

        act("&7&b$n tries to paralize you but fails!&0", false, ch, 0, victim, TO_VICT);
        act("&7&b$n squints at $N but nothing happens.&0", true, ch, 0, victim, TO_NOTVICT);

        /* start combat for failure */
        if (!FIGHTING(victim)) {
            attack(victim, ch);
            remember(victim, ch);
        }

        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }

    memset(&eff, 0, sizeof(eff));
    eff.type = SPELL_MAJOR_PARALYSIS;
    eff.duration = skill / 20;
    SET_FLAG(eff.flags, EFF_MAJOR_PARALYSIS);
    effect_to_char(victim, &eff);
    if (victim == ch)
        return CAST_RESULT_CHARGE;
    act("&6You totally paralyze $N's nervous system!&0", false, ch, 0, victim, TO_CHAR);
    act("&6$n totally paralyzes your nervous system!&0", false, ch, 0, victim, TO_VICT);
    act("&6$n totally paralyzes $N's nervous system!&0", true, ch, 0, victim, TO_ROOM);
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_minor_creation) {
    int r_num, i = 0, found = 0;
    ObjData *cobj;

    if (!ch)
        return 0;

    if (!(ch->casting.misc) || !*(ch->casting.misc)) {
        char_printf(ch, "What are you trying to create?\n");
        return 0;
    }
    half_chop(ch->casting.misc, buf, buf2);
    while (minor_creation_items[i].front() != '\n') {
        if (is_abbrev(buf, minor_creation_items[i].data())) {
            found = 1;
            break;
        } else
            i++;
    }
    if (found) {
        if ((r_num = real_object(1000 + i)) < 0) {
            log("SYSERR: Error in function spell_minor_create: target item not "
                "found.");
            char_printf(ch, "Something is wrong with minor create.   Please tell a god.\n");
            return 0;
        }
        cobj = read_object(r_num, REAL);
        if (GET_OBJ_TYPE(cobj) == ITEM_LIGHT) {
            GET_OBJ_VAL(cobj, VAL_LIGHT_LIT) = true;
        }
        obj_to_room(cobj, ch->in_room);
        act("$n &0&5invokes the powers of creation...&0", true, ch, 0, 0, TO_ROOM);
        act("$p &0&7appears in a &bbright flash of light!&0", false, ch, cobj, 0, TO_ROOM);
        act("&7You have created $p.&0", false, ch, cobj, 0, TO_CHAR);

        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    } else {
        char_printf(ch, "You have no idea how to create such an item.\n");
        return CAST_RESULT_CHARGE;
    }
}

ASPELL(spell_moonwell) {
    ObjData *portal, *tportal;
    ExtraDescriptionData *new_descr, *new_tdescr;

    if (ch == nullptr || victim == nullptr)
        return 0;
    if (IS_NPC(victim)) {
        char_printf(ch, "You cannot target NPCs with this spell.\n");
        return 0;
    }
    if (victim->in_room == 0)
        return 0;
    if (victim->in_room == -1) {
        char_printf(ch, "You failed.\n");
        WAIT_STATE(ch, PULSE_VIOLENCE);
        log("SYSERR: {} tried to moonwell to {} in room -1.", GET_NAME(ch), GET_NAME(victim));
        return CAST_RESULT_CHARGE;
    }
    if (GET_LEVEL(ch) < LVL_IMMORT &&
        (ROOM_FLAGGED(ch->in_room, ROOM_NOWELL) || ROOM_FLAGGED(victim->in_room, ROOM_NOWELL) ||
         PRF_FLAGGED(victim, PRF_NOFOLLOW))) {
        act("A&0 &5moonwell&0 &7 appears briefly, then dissolves.&0", true, ch, 0, 0, TO_ROOM);
        act("A&0 &5moonwell&0 &7 appears briefly, then dissolves.&0", true, ch, 0, 0, TO_CHAR);
        act("A&0 &5moonwell&0 &7 appears briefly, then dissolves.&0", true, ch, 0, victim, TO_VICT);
        return CAST_RESULT_CHARGE;
    }
    if ((portal = read_object(OBJ_VNUM_MOONWELL, VIRTUAL)) == nullptr) {
        log("SYSERR: Moonwell object not found.");
        return 0;
    }
    GET_OBJ_VAL(portal, VAL_PORTAL_DESTINATION) = world[victim->in_room].vnum;
    GET_OBJ_DECOMP(portal) = 2;
    CREATE(new_descr, ExtraDescriptionData, 1);
    new_descr->keyword = strdup("well gate moonwell");
    sprintf(buf, "You can barely make out %s.\n", world[victim->in_room].name.c_str());
    new_descr->description = strdup(buf);
    new_descr->next = portal->ex_description;
    portal->ex_description = new_descr;
    obj_to_room(portal, ch->in_room);
    /* Ideally, the message would differ depending on whether you can see the
     * caster or not: if you can, you'd see it appear "at $n's feet", and if not,
     * it would just appear. But since act() doesn't support such shenanigans,
     * we'll just have it appear. */
    act("&5In a gust of wind a&0 &7moonwell&0 &5appears.&0", false, ch, 0, 0, TO_ROOM);
    if (!EFF_FLAGGED(ch, EFF_BLIND))
        act("&5In a gust of wind a&0 &7moonwell&0 &5appears at your feet.&0", false, ch, 0, 0, TO_CHAR);
    /* create the portal at the other end */
    tportal = read_object(OBJ_VNUM_MOONWELL, VIRTUAL);
    GET_OBJ_VAL(tportal, VAL_PORTAL_DESTINATION) = world[ch->in_room].vnum;
    GET_OBJ_DECOMP(tportal) = 2;
    CREATE(new_tdescr, ExtraDescriptionData, 1);
    new_tdescr->keyword = strdup("well gate moonwell");
    sprintf(buf, "You can barely make out %s.\n", world[ch->in_room].name.c_str());
    new_tdescr->description = strdup(buf);
    new_tdescr->next = tportal->ex_description;
    tportal->ex_description = new_tdescr;
    obj_to_room(tportal, victim->in_room);
    act("&5In a gust of wind a&0 &7moonwell&0 &5appears.&0", false, victim, 0, 0, TO_ROOM);
    if (!EFF_FLAGGED(victim, EFF_BLIND))
        act("&5In a gust of wind a&0 &7moonwell&0 &5appears at your feet.&0", true, victim, 0, 0, TO_CHAR);
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(chant_peace) {
    CharData *i;

    if (ch->in_room == NOWHERE)
        return 0;

    for (i = world[ch->in_room].people; i; i = i->next_in_room) {
        if (FIGHTING(i))
            stop_fighting(i);
        if (IS_NPC(i))
            clear_memory(i);
    }

    act("A peaceful feeling washes into the room, dousing all violence!", false, ch, 0, 0, TO_ROOM);
    char_printf(ch, "You douse all violence!\n");
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_phosphoric_embers) {
    DelayedCastEventObj *event_obj;

    if (!ch || !victim)
        return 0;

    act("@yYou hurl a handful of @Yburning phosphoric embers@y at $N.@0", false, ch, 0, victim, TO_CHAR);
    act("@y$n@y hurls a handful of @Yflaring phosphoric embers@y at $N.@0", false, ch, 0, victim, TO_NOTVICT);
    act("@y$n@y hurls a a handful of @Yburning phosphoric embers@y on you!@0", false, ch, 0, victim, TO_VICT);

    event_obj =
        construct_delayed_cast(ch, victim, SPELL_PHOSPHORIC_EMBERS, MAG_DAMAGE, 4, 4 RL_SEC, skill, savetype, false);
    event_create(EVENT_SPELL, delayed_cast_event, event_obj, true, &(ch->events), 4 RL_SEC);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_plane_shift) {

    /* When planes are put in, this spell will randomly place them in a plane */
    if (!ch)
        return 0;
    act("You begin to fade, then reappear.", true, ch, 0, 0, TO_CHAR);
    act("$n begins to fade, then reappears.", true, ch, 0, 0, TO_ROOM);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_preserve) {
    if (!ch || !obj)
        return 0;

    if (!IS_CORPSE(obj)) {
        char_printf(ch, "You can only preserve a corpse!\n");
        return CAST_RESULT_CHARGE;
    }
    sprintf(buf, "$p is preserved for %d more hours.", GET_LEVEL(ch));

    GET_OBJ_DECOMP(obj) += GET_LEVEL(ch);
    act("$p glows &4&bblue&0 briefly.", false, ch, obj, 0, TO_ROOM);
    act("$p glows &4&bblue&0 briefly.", false, ch, obj, 0, TO_CHAR);
    act(buf, false, ch, obj, 0, TO_CHAR);
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/*
 * flesh, bone, plant
 */

ASPELL(spell_pyre) {
    const int PYRE_DELAY = PULSE_VIOLENCE;
    FollowType *fol;
    DelayedCastEventObj *event_obj;
    char *argument;

#define CAN_BURN(ch)                                                                                                   \
    (GET_COMPOSITION(ch) == COMP_FLESH || GET_COMPOSITION(ch) == COMP_PLANT || GET_COMPOSITION(ch) == COMP_BONE)

    if (!ch)
        return 0;

    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        char_printf(ch, "A flash of white light fills the room, dispelling your violent magic!\n");
        act("White from no particular source suddenly fills the room, then vanishes.", false, victim, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE;
    }

    /* If there was a named target, try to use that one */
    if (ch->casting.misc && *ch->casting.misc) {
        argument = ch->casting.misc;
        skip_spaces(&argument);
        one_argument(argument, arg);
        victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg));
    }

    /* If there was a named target, check if attacking is okay */
    if (victim) {
        if (!attack_ok(ch, victim, true))
            return CAST_RESULT_CHARGE;

        /* Make sure it's flammable */
        if (!CAN_BURN(victim)) {
            act("You can't burn $N!", false, ch, 0, victim, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }
    } else {
        /* No target was specified, find a zombie target */
        for (fol = ch->followers; fol; fol = fol->next)
            if (CAN_BURN(fol->follower) && EFF_FLAGGED(fol->follower, EFF_CHARM) &&
                IN_ROOM(fol->follower) == IN_ROOM(ch)) {
                victim = fol->follower;
                break;
            }

        /* If caster is sacrificial, target self */
        if (PRF_FLAGGED(ch, PRF_SACRIFICIAL))
            victim = ch;

        if (!victim) {
            char_printf(ch, "You look around, but can't find an acceptable body.\n");
            return CAST_RESULT_CHARGE;
        }
    }

    if (ch == victim) {
        act("@yYou snap a @Rconflagration @yinto existence, lighting yourself up "
            "like a torch.&0",
            false, ch, 0, 0, TO_CHAR);
        act("@y$n@y snaps a @Rconflagration @yinto existence, lighting $mself up "
            "like a torch!&0",
            false, ch, 0, 0, TO_ROOM);
    } else {
        act("@yYou snap a @Rconflagration @yinto existence, lighting $N up like a "
            "torch.&0",
            false, ch, 0, victim, TO_CHAR);
        act("@y$n@y snaps a @Rconflagration @yinto existence, lighting $N up like "
            "a torch!&0",
            false, ch, 0, victim, TO_ROOM);
        act("@y$n@y snaps a @Rconflagration @yinto existence, enveloping you in "
            "flames!&0",
            false, ch, 0, victim, TO_VICT);
    }

    event_obj = construct_delayed_cast(EFF_FLAGGED(victim, EFF_CHARM) && victim->master == ch ? victim : ch, victim,
                                       SPELL_PYRE, MAG_MANUAL, 4, PYRE_DELAY, skill, savetype, false);
    event_create(EVENT_SPELL, delayed_cast_event, event_obj, true, &(victim->events), PYRE_DELAY);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_pyre_recur) {
    CharData *tch, *next_tch;
    room_num room = victim->in_room;

    if (ch->in_room == NOWHERE)
        return CAST_RESULT_CHARGE;

    mag_damage(skill, ch, victim, SPELL_PYRE_RECOIL, savetype);

    for (tch = world[room].people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;
        if (victim == tch)
            continue;
        /* Standard room attack check */
        if (!area_attack_target(victim, tch))
            continue;
        if (FIGHTING(tch) != victim && FIGHTING(victim) != tch)
            continue;
        mag_damage(skill, victim, tch, SPELL_PYRE, savetype);
    }

    return CAST_RESULT_CHARGE;
}

ASPELL(spell_rain) {
    CharData *vict, *next_vict;

    if (!ch)
        return 0;

    act("&4$n&4 conjures a mighty rainstorm, dousing everything in the area.&0", false, ch, 0, 0, TO_ROOM);
    act("&4You conjure a mighty rainstorm, dousing everything in the area.&0", false, ch, 0, 0, TO_CHAR);

    /* Douse circle of fire in room */
    if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_CIRCLE_FIRE))
        REMOVE_FLAG(ROOM_EFFECTS(ch->in_room), ROOM_EFF_CIRCLE_FIRE);

    /* Douse all people in room */
    for (vict = world[ch->in_room].people; vict; vict = next_vict) {
        next_vict = vict->next_in_room;
        if (GET_LEVEL(vict) >= LVL_IMMORT && !IS_NPC(vict))
            continue;
        REMOVE_FLAG(EFF_FLAGS(vict), EFF_ON_FIRE);
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_relocate) {
    bool wasdark;

    if (!ch || !victim)
        return 0;
    if (IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOFOLLOW)) {
        char_printf(ch, "You failed.\n");
        WAIT_STATE(ch, PULSE_VIOLENCE * 10);
        return CAST_RESULT_CHARGE;
    }
    if (victim->in_room == -1) {
        char_printf(ch, "You failed.\n");
        WAIT_STATE(ch, PULSE_VIOLENCE);
        log("SYSERR: {} tried to relocate to {} in room -1.", GET_NAME(ch), GET_NAME(victim));
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }
    if (GET_LEVEL(victim) >= LVL_IMMORT && GET_LEVEL(ch) < GET_LEVEL(victim)) {
        char_printf(ch, "Your magics are stamped out by the gods.\n");
        return CAST_RESULT_CHARGE;
    } else {
        if (victim->in_room == ch->in_room) {
            char_printf(ch, "You are already here.\n");
            return CAST_RESULT_CHARGE;
        }
        dismount_char(ch);
        act("&7&bYour body begins to fade from existence, &9then you black "
            "out...&0",
            false, ch, 0, 0, TO_CHAR);
        act("&7&b$n&7&b's molecules loosen and eventually dissipate into thin "
            "air.&0",
            true, ch, 0, 0, TO_ROOM);
        wasdark = IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch);
        char_from_room(ch);
        char_to_room(ch, victim->in_room);
        act("&6&bThe air begins to thicken, slowly revealing a living form...&0\n\
         &7&b$n&7&b's molecules condense and finally take hold... "
            "$n&7&b appears quite tired.&0",
            true, ch, 0, 0, TO_ROOM);
        check_new_surroundings(ch, wasdark, true);
        GET_POS(ch) = POS_SITTING;
        GET_STANCE(ch) = STANCE_RESTING;
    }
    WAIT_STATE(ch, PULSE_VIOLENCE * 10);
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_remove_curse) {
    ObjData *object;
    bool found = false;

    if (victim) {
        if (EFF_FLAGGED(victim, EFF_CURSE)) {
            if (victim == ch) {
                act("You place your hand over your heart and wince slightly.", false, ch, 0, victim, TO_CHAR);
                act("You feel less unlucky.", false, ch, 0, victim, TO_CHAR);
                act("$n places $s hand over $s heart and winces slightly.", false, ch, 0, victim, TO_NOTVICT);
            } else {
                act("You place your hand over $S heart and wince slightly.", false, ch, 0, victim, TO_CHAR);
                act("$n places $s hand over your heart and winces slightly.", false, ch, 0, victim, TO_VICT);
                act("You feel less unlucky.", false, ch, 0, victim, TO_VICT);
                act("$n places $s hand over the heart of $N and winces slightly.", false, ch, 0, victim, TO_NOTVICT);
            }

            effect_from_char(victim, SPELL_CURSE);
        } else {
            if (victim->carrying)
                for (object = victim->carrying; object; object = object->next_content) {
                    if (!found && OBJ_FLAGGED(object, ITEM_NODROP)) {
                        REMOVE_FLAG(GET_OBJ_FLAGS(object), ITEM_NODROP);
                        if (GET_OBJ_TYPE(object) == ITEM_WEAPON)
                            GET_OBJ_VAL(object, VAL_WEAPON_DICE_SIZE)++;
                        act("$p glows blue momentarily.", false, ch, object, victim, TO_ROOM);
                        act("$p glows blue momentarily.", false, ch, object, victim, TO_CHAR);
                        found = true;
                    }
                }
            if (!found) {
                if (victim == ch)
                    act("You do not sense any foul magicks upon your being.", false, ch, 0, victim, TO_CHAR);
                else
                    act("You do not sense any foul magicks upon $M.", false, ch, 0, victim, TO_CHAR);
                return CAST_RESULT_CHARGE;
            }
        }
    }

    if (obj) {
        if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
            REMOVE_FLAG(GET_OBJ_FLAGS(obj), ITEM_NODROP);
            if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
                GET_OBJ_VAL(obj, VAL_WEAPON_DICE_SIZE)++;
            }
            act("$p glows blue momentarily.", false, ch, obj, victim, TO_ROOM);
            act("$p glows blue momentarily.", false, ch, obj, victim, TO_CHAR);
        } else {
            act("You do not sense any foul magicks upon $p.", false, ch, obj, victim, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_remove_paralysis) {
    ObjData *object;
    bool found = false;

    if (victim) {
        if ((EFF_FLAGGED(victim, EFF_MINOR_PARALYSIS)) || (EFF_FLAGGED(victim, EFF_MAJOR_PARALYSIS))) {
            act("&3&b$N begins to move again.", false, ch, 0, victim, TO_CHAR);
            act("&3&bYou begin to move again.", false, ch, 0, victim, TO_VICT);
            act("&3&b$N begins to move again.", false, ch, 0, victim, TO_NOTVICT);

            if (EFF_FLAGGED(victim, EFF_MINOR_PARALYSIS))
                effect_from_char(victim, SPELL_MINOR_PARALYSIS);
            if (EFF_FLAGGED(victim, EFF_MAJOR_PARALYSIS))
                effect_from_char(victim, SPELL_MAJOR_PARALYSIS);

        } else {
            if (!found) {
                if (victim == ch)
                    act("You can already move just fine.", false, ch, 0, victim, TO_CHAR);
                else
                    act("$N can already move just fine.", false, ch, 0, victim, TO_CHAR);
                return CAST_RESULT_CHARGE;
            }
        }

        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    } else {
        return CAST_RESULT_CHARGE;
    }
}

int reveal_contents(ObjData *container, CharData *ch) {
    int found_something = 0;
    ObjData *k;

    for (k = container; k; k = k->next_content) {
        if (OBJ_FLAGGED(k, ITEM_INVISIBLE) || GET_OBJ_HIDDENNESS(k) > 0) {
            REMOVE_FLAG(GET_OBJ_FLAGS(k), ITEM_INVISIBLE);
            GET_OBJ_HIDDENNESS(k) = 0;
            act("You reveal $p!", false, ch, k, 0, TO_CHAR);
            act("$n reveals $p!", true, ch, k, 0, TO_ROOM);
            found_something += 1;
        }
        if (k) {
            if (GET_OBJ_TYPE(k) == ITEM_CONTAINER) {
                if (IS_SET(GET_OBJ_VAL(k, VAL_CONTAINER_BITS), CONT_CLOSED)) {
                    act("You must open $p before you can reveal its contents.", false, ch, k, 0, TO_CHAR);
                } else {
                    found_something += reveal_contents(k->contains, ch);
                }
            }
        }
    }
    return found_something;
}

/* search_for_doors() - returns true if a door was located. */
static int search_for_doors(CharData *ch) {
    int door;
    int found_something = 0;

    for (door = 0; door < NUM_OF_DIRS; ++door)
        if (CH_EXIT(ch, door) && CH_EXIT(ch, door)->to_room != NOWHERE &&
            IS_SET(CH_EXIT(ch, door)->exit_info, EX_HIDDEN)) {
            sprintf(buf, "You have found%s hidden %s %s.&0",
                    CH_EXIT(ch, door)->keyword && isplural(CH_EXIT(ch, door)->keyword) ? "" : " a",
                    CH_EXIT(ch, door)->keyword ? "$F" : "door", dirpreposition[door].data());
            act(buf, false, ch, 0, CH_EXIT(ch, door)->keyword, TO_CHAR);
            sprintf(buf, "$n has found%s hidden %s %s.",
                    CH_EXIT(ch, door)->keyword && isplural(CH_EXIT(ch, door)->keyword) ? "" : " a",
                    CH_EXIT(ch, door)->keyword ? "$F" : "door", dirpreposition[door].data());
            act(buf, false, ch, 0, CH_EXIT(ch, door)->keyword, TO_ROOM);
            REMOVE_BIT(CH_EXIT(ch, door)->exit_info, EX_HIDDEN);
            send_gmcp_room(ch);
            found_something += 1;
        }

    return found_something;
}

ASPELL(spell_reveal_hidden) {
    CharData *vict;
    int found_something = 0;

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (EFF_FLAGGED(ch, EFF_BLIND)) {
            char_printf(ch, "You're blind and can't see a thing!\n");
            return CAST_RESULT_CHARGE;
        }
        if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_FOG)) {
            act("&3&bYou dispel the fog!&0", false, ch, 0, victim, TO_CHAR);
            act("&3&b$n dispels the fog!&0", false, ch, 0, victim, TO_NOTVICT);
            REMOVE_FLAG(ROOM_EFFECTS(ch->in_room), ROOM_EFF_FOG);
        }
    }

    found_something += reveal_contents(world[ch->in_room].contents, ch);

    found_something += reveal_contents(ch->carrying, ch);

    found_something += search_for_doors(ch);

    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
        if (EFF_FLAGGED(vict, EFF_INVISIBLE) || IS_HIDDEN(vict)) {
            if (IS_NPC(vict) && !IS_IN_GROUP(ch, vict)) {
                REMOVE_FLAG(EFF_FLAGS(vict), EFF_INVISIBLE);
                GET_HIDDENNESS(vict) = 0;
                act("You reavel $N lurking here!", false, ch, 0, vict, TO_CHAR);
                act("$n reveals $N lurking here!", true, ch, 0, vict, TO_NOTVICT);
                found_something += 1;
            }
        }

    if (found_something > 0) {
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    } else {
        char_printf(ch, "You don't find anything you didn't see before.\n");
        return CAST_RESULT_CHARGE;
    }
}

ASPELL(spell_soul_tap) {
    DelayedCastEventObj *event_obj;

    if (!ch || !victim)
        return 0;

    if (ch == victim) {
        char_printf(ch, "Tapping our own soul?  Not very effective...\n");
        return CAST_RESULT_CHARGE;
    }

    if (GET_LIFEFORCE(victim) == LIFE_UNDEAD || GET_LIFEFORCE(victim) == LIFE_MAGIC ||
        GET_LIFEFORCE(victim) == LIFE_ELEMENTAL) {
        act("$N has no soul to tap!", false, ch, 0, victim, TO_CHAR);
        return CAST_RESULT_CHARGE;
    }

    act("@LYou reach a shadowy hand into the depths of $N@L's soul...@0", false, ch, 0, victim, TO_CHAR);
    act("@L$n@L reaches a shadowy hand into the depths of $N@L's soul...@0", false, ch, 0, victim, TO_NOTVICT);
    act("@L$n@L reaches a shadowy hand into the depths of your soul...@0", false, ch, 0, victim, TO_VICT);

    event_obj = construct_delayed_cast(ch, victim, SPELL_SOUL_TAP, MAG_MANUAL, 5, 5 RL_SEC, skill, savetype, true);
    event_create(EVENT_SPELL, delayed_cast_event, event_obj, true, &(ch->events), 5 RL_SEC);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_soul_tap_recur) {
    int dam, hp;

    if (damage_evasion(victim, 0, 0, skills[spellnum].damage_type)) {
        act("$n is unaffected!", false, victim, 0, 0, TO_ROOM);
        act("You are unaffected!", false, victim, 0, 0, TO_CHAR);
        return CAST_RESULT_CHARGE;
    }

    dam = normal_random(skill + 10, skill / 3);
    dam *= susceptibility(victim, skills[spellnum].damage_type) / 100;

    hp = GET_HIT(victim);
    damage(ch, victim, dam, SPELL_SOUL_TAP);

    /* Actual damage done: */
    dam = hp - GET_HIT(victim);

    if (GET_HIT(ch) < GET_MAX_HIT(ch))
        GET_HIT(ch) += dam * 3 / 2;
    else if (dam) {
        float ratio = GET_HIT(ch) / GET_MAX_HIT(ch);
        /* Polynomial that approaches zero at a max hp / hp ratio of about 5 */
        hp = dam * (-0.0457 * ratio * ratio - 0.0171 * ratio + 1.066);
        if (hp > 10)
            GET_HIT(ch) += hp;
        else
            GET_HIT(ch) += random_number(5, 10);
    }

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_ventriloquate) {
    char *msg;
    CharData *tch;

    msg = ch->casting.misc;

    /* Skip over the first word, which is the target's name */
    while (*msg && *msg != ' ')
        msg++;
    while (*msg && *msg == ' ')
        msg++;

    if (!*msg) {
        act("You seem to have forgotten what you wanted $M to say!", false, ch, 0, victim, TO_CHAR);
        return CAST_RESULT_CHARGE;
    }

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
        if (tch == ch)
            continue;
        if (IS_PC(victim) || random_number(1, 8 * (skill + 24)) < GET_LEVEL(tch) + GET_VIEWED_WIS(tch) / 2) {
            if (tch == victim) {
                act("Someone tried to make it sound like you said '$T'!", false, tch, 0, msg, TO_CHAR);
            } else {
                sprintf(buf, "Someone tried to make it sound like $N said, '%s'!", msg);
                act(buf, false, tch, 0, victim, TO_CHAR);
            }
        } else if (random_number(1, 8 * (skill + 24)) < (GET_LEVEL(tch) + GET_VIEWED_WIS(tch)) / 4) {
            if (tch == victim) {
                sprintf(buf, "$n tried to make it sound like you said '%s'!", msg);
                act(buf, false, ch, 0, victim, TO_VICT);
            } else {
                sprintf(buf, "%s tries to make it sound like $N says, '%s'!", PERS(ch, tch), msg);
                act(buf, false, tch, 0, victim, TO_CHAR);
            }
        } else {
            if (tch == victim) {
                act("Someone says, '$T'", false, tch, 0, msg, TO_CHAR);
            } else {
                sprintf(buf, "$n says, '%s'", msg);
                act(buf, false, victim, 0, tch, TO_VICT);
            }
        }
    }

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_wandering_woods) {
    RoomUndoEventObj *room_undo;
    int dir, dir2, next_room, i, changed = false;

    if (!ch)
        return 0;

    if (SECT(ch->in_room) != SECT_FOREST && !ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_FOREST)) {
        char_printf(ch, "You are not in a forest.\n");
        return CAST_RESULT_CHARGE;
    }

    for (dir = 0; dir < NUM_OF_DIRS; dir++) {
        if (!CAN_GO(ch, dir))
            continue;
        next_room = CH_NDEST(ch, dir);
        if (SECT(next_room) != SECT_FOREST && !ROOM_EFF_FLAGGED(next_room, ROOM_EFF_FOREST))
            /* is next room forest? */
            continue;
        if (ROOM_FLAGGED(next_room, ROOM_ALT_EXIT)) /* has next room been altered? */
            continue;
        dir2 = -1;
        for (i = 0; i < NUM_OF_DIRS; i++)
            /* find the exit in next room that leads to this one */
            if (world[next_room].exits[i] && (world[next_room].exits[i]->to_room != NOWHERE) &&
                (world[next_room].exits[i])->to_room == ch->in_room)
                dir2 = i;
        if (dir2 == -1) /* next room does not lead back to this one */
            continue;
        /* queue an event to change the room back to normal */
        CREATE(room_undo, RoomUndoEventObj, 1);
        room_undo->exit = dir2;
        room_undo->room = next_room;
        room_undo->connect_room = ch->in_room;
        event_create(EVENT_ROOM_UNDO, room_undo_event, room_undo, true, nullptr, 120 * PASSES_PER_SEC);
        /* ok, now we know switch exit is ok... time to find the exit to switch to
         */
        if (CAN_GO(ch, rev_dir[dir])) {
            /* hook exit to opposite exit */
            world[next_room].exits[dir2]->to_room = CH_NDEST(ch, rev_dir[dir]);
        } else {
            /* loop exit back to next_room */
            world[next_room].exits[dir2]->to_room = next_room;
        }
        changed = true;
        SET_FLAG(ROOM_FLAGS(next_room), ROOM_ALT_EXIT);
        room_printf(next_room,
                    "&2The forest seems to come alive... Trees and shrubs move about, finally resting in different "
                    "locations.&0\n");
    }
    if (changed) {
        act("&2&b$n&2&b exudes a &0&2green&b glow as $e speaks with "
            "the surrounding forest...&0",
            true, ch, 0, 0, TO_ROOM);
        act("&2&bYou begin to speak with the surrounding forest...&0", false, ch, 0, 0, TO_CHAR);
        room_printf(ch->in_room,
                    "&2The forest seems to come alive... Trees and shrubs move about, finally resting in different "
                    "locations.&0\n");
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    } else {
        char_printf(ch, "&2The surrounding forest resists your command.&0\n");
        return CAST_RESULT_CHARGE;
    }
}

ASPELL(spell_wizard_eye) {
    int room;

    if (!ch || !victim)
        return 0;

    if (GET_LEVEL(victim) >= LVL_IMMORT) {
        char_printf(ch, "You failed.\n");
        return CAST_RESULT_CHARGE;
    }

    if (skill < random_number(1, 100)) {
        char_printf(ch, "You failed.\n");
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }

    char_printf(ch, "&6&bYou close your eyes and let your vision wander.&0\n");
    room = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, victim->in_room);
    look_at_room(ch, true);
    char_from_room(ch);
    char_to_room(ch, room);
    if (!IS_NPC(victim))
        char_printf(victim, "&9&bYou feel like you are being watched...&0\n");

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_word_of_command) {
    if (!ch || !victim)
        return 0;

    if ((ch->casting.misc) && *(ch->casting.misc))
        half_chop(ch->casting.misc, buf, buf2);
    if (!*buf2) {
        char_printf(ch, "What do you want them to do?\n");
        return 0;
    }
    half_chop(buf2, buf2, buf);                               /* only 1 word commands */
    if (GET_LEVEL(victim) >= LVL_IMMORT && !IS_NPC(victim)) { /* no commanding gods */
        char_printf(ch, "You best be careful who you try commanding!\n");
        return CAST_RESULT_CHARGE;
    }
    if (GET_LEVEL(victim) >= GET_LEVEL(ch)) { /* only command chars lower lvl */
        char_printf(ch, "That being is too experienced for you to command.\n");
        return CAST_RESULT_CHARGE;
    }

    /* This check is already done in evades_spell(), but we need some
     * way to get proper messages sent.   Maybe a mag_evades()? :-) */
    /*
       if (boolean_attack_evasion(victim, skill, DAM_MENTAL)) {
       act("$N resists your command.", false, ch, 0, victim, TO_CHAR);
       act("You resist $n's telepathic command.", false, ch, 0, victim, TO_VICT);
       return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
       }
     */

    sprintf(buf, "$n&0 has commanded you to, '%s'", buf2);
    act(buf, false, ch, 0, victim, TO_VICT);
    sprintf(buf, "You command $N&0 to, '%s'", buf2);
    act(buf, false, ch, 0, victim, TO_CHAR);

    if (mag_savingthrow(victim, SAVING_SPELL)) {
        /* start combat if failure */
        if (!FIGHTING(victim)) {
            attack(victim, ch);
            remember(victim, ch);
        }
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }

    act("&7&b$n&7&b uses $s holy power to command $N.&0", true, ch, 0, victim, TO_NOTVICT);
    command_interpreter(victim, buf2);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

void create_magical_wall(int room, int power, int dir, int spell, char *material, char *mcolor) {
    ObjData *wall;

    wall = create_obj();

    wall->item_number = NOTHING;
    wall->in_room = NOWHERE;

    sprintf(buf, "wall %s", material);
    wall->name = strdup(buf);

    if (dir == UP)
        sprintf(buf2, "%sA wall of %s lies upwards.&0", mcolor, material);
    else if (dir == DOWN)
        sprintf(buf2, "%sA wall of %s lies downwards.&0", mcolor, material);
    else
        sprintf(buf2, "%sA wall of %s lies to the %s.&0", mcolor, material, dirs[dir].data());
    wall->description = strdup(buf2);
    sprintf(buf, "%sa wall of %s&0", mcolor, material);
    wall->short_description = strdup(buf);

    GET_OBJ_TYPE(wall) = ITEM_WALL;
    SET_FLAG(GET_OBJ_FLAGS(wall), ITEM_FLOAT);
    SET_FLAG(GET_OBJ_FLAGS(wall), ITEM_DECOMP);
    SET_FLAG(GET_OBJ_FLAGS(wall), ITEM_MAGIC);
    GET_OBJ_VAL(wall, VAL_WALL_DIRECTION) = dir;
    GET_OBJ_VAL(wall, VAL_WALL_DISPELABLE) = 250 + (power * 1750) / 100; /* 250-2000 hit points */
    GET_OBJ_VAL(wall, VAL_WALL_HITPOINTS) = GET_OBJ_VAL(wall, VAL_WALL_DISPELABLE);
    GET_OBJ_VAL(wall, VAL_WALL_SPELL) = spell;
    GET_OBJ_DECOMP(wall) = 3 + power / 30; /* 3-6 hours */
    GET_OBJ_EFFECTIVE_WEIGHT(wall) = 5000;

    obj_to_room(wall, room);

    if (dir == UP)
        room_printf(room, "{}A wall of {} appears above.&0\n", mcolor, material);
    else if (dir == DOWN)
        room_printf(room, "{}A wall of {} appears below.&0\n", mcolor, material);
    else
        room_printf(room, "{}A wall of {} appears to the {}.&0\n", mcolor, material, dirs[dir]);
}

/* General wall-creation function. */
ASPELL(spell_magical_wall) {
    char material[40], mcolor[40];
    int i, next_room, dir = -1;
    ObjData *sobj;

    if (!ch)
        return 0;

    if (!(ch->casting.misc) || !*(ch->casting.misc)) {
        char_printf(ch, "In what direction should the wall be cast?\n");
        return 0;
    }

    /* Determine what direction the wall will be created in. */

    half_chop(ch->casting.misc, buf, buf2);
    for (i = 0; i < NUM_OF_DIRS; i++) {
        if (is_abbrev(buf, dirs[i].data()))
            dir = i;
    }

    if (dir == -1) {
        char_printf(ch, "That is not a proper direction.\n");
        return 0;
    }

    /* You can't cast this spell in peaceful rooms. */

    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        char_printf(ch, "A flash of white light fills the room, dispelling your violent magic!\n");
        act("White light from no particular source suddenly fills the room, then vanishes.", false, ch, 0, 0, TO_ROOM);
        return CAST_RESULT_CHARGE;
    }

    if (!CAN_GO(ch, dir)) {
        char_printf(ch, "There is no exit that direction.\n");
        return CAST_RESULT_CHARGE;
    }

    /* See whether there is already a wall there. */

    for (sobj = world[ch->in_room].contents; sobj; sobj = sobj->next_content) {
        if (GET_OBJ_TYPE(sobj) == ITEM_WALL && GET_OBJ_VAL(sobj, VAL_WALL_DIRECTION) == dir) {
            char_printf(ch, "There is already a wall there!\n");
            return CAST_RESULT_CHARGE;
        }
    }

    /* Establish the composition of the wall.  Most wall-creation spells
     * each have a specific material.  The illusory wall spell allows the
     * caster to specify the material. */

    switch (spellnum) {
    case SPELL_WALL_OF_ICE:
        sprintf(material, "ice");
        sprintf(mcolor, "&6&b");
        break;
    case SPELL_ILLUSORY_WALL:
        half_chop(buf2, buf, arg);
        if (strlen(buf) > 38) {
            char_printf(ch, "That's far too exotic a material!\n");
            return 0;
        }
        strcpy(material, buf);
        if (!*buf || !strcasecmp("stone", buf)) {
            sprintf(material, "stone");
            sprintf(mcolor, "&9&b");
        } else if (!strcasecmp("ice", buf)) {
            sprintf(mcolor, "&6&b");
        } else if (!strcasecmp("brick", buf)) {
            sprintf(mcolor, "&1");
        } else if (!strcasecmp("wood", buf)) {
            sprintf(mcolor, "&3");
        } else {
            char_printf(ch, "The material must be brick, ice, stone, or wood.\n");
            return 0;
        }
        break;
    case SPELL_WALL_OF_STONE:
    default:
        sprintf(material, "stone");
        sprintf(mcolor, "&9&b");
        break;
    }

    /* Create the first wall. */

    create_magical_wall(ch->in_room, skill, dir, spellnum, material, mcolor);

    /* Find the proper exit in the next room for walling.
     * If the next room does not have an exit to the original room,
     * don't make a wall there. */

    next_room = CH_NDEST(ch, dir);

    for (i = 0, dir = -1; i < NUM_OF_DIRS; i++) {
        if (world[next_room].exits[i] && (world[next_room].exits[i]->to_room != NOWHERE) &&
            (world[next_room].exits[i])->to_room == ch->in_room)
            dir = i;
    }

    /* Create the second wall. */

    if (dir != -1)
        create_magical_wall(next_room, skill, dir, spellnum, material, mcolor);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

void dispel_harmful_magic(CharData *ch) {
    effect *effect, *next_effect;

    if ((effect = ch->effects)) {
        while (effect) {
            next_effect = effect->next;
            if ((IS_SPELL(effect->type) || IS_CHANT(effect->type) || IS_SONG(effect->type)) &&
                skills[effect->type].violent) {
                active_effect_remove(ch, effect);
            }
            effect = next_effect;
        }
    }
}

ASPELL(spell_dispel_magic) {
    void decay_object(ObjData * obj);
    void destroy_opposite_wall(ObjData * wall);
    effect *eff = nullptr, *next_eff = nullptr;
    int count = 0;
    bool didanything = false;

    if (!ch || (!victim && !obj))
        return 0;

    if (victim) {
        if (GET_LEVEL(ch) < LVL_IMMORT && !attack_ok(ch, victim, false) && CONSENT(victim) != ch) {
            char_printf(ch, "Not without consent you don't!\n");
            return CAST_RESULT_CHARGE;
        }
        if (ch != victim) {
            act("&4You attempt to dispel $N&0&4's magical nature...&0", false, ch, 0, victim, TO_CHAR);
            act("&4$n&0&4 focuses on you, you feel your magical nature wavering...&0", true, ch, 0, victim, TO_VICT);
            act("&4$n&0&4 focuses on $N&0&4, attempting to dispel $S magical "
                "nature...&0",
                true, ch, 0, victim, TO_NOTVICT);
        } else {
            act("&4You attempt to dispel your magical nature...&0", false, ch, 0, victim, TO_CHAR);
            act("&4$n&0&4 focuses on $mself, attempting to dispel $s magical "
                "nature...&0",
                true, ch, 0, victim, TO_NOTVICT);
        }

        if ((eff = victim->effects)) {
            while (eff) {
                next_eff = eff->next;
                if ((IS_SPELL(eff->type) || IS_CHANT(eff->type) || IS_SONG(eff->type)) &&
                    /* Here are some spells you can't dispel. */
                    (eff->type != SPELL_PHANTASM && eff->type != SPELL_ANIMATE_DEAD && eff->type != SPELL_CHARM) &&
                    (!mag_savingthrow(victim, SAVING_PARA) || (!IS_NPC(ch) && GET_LEVEL(ch) > LVL_IMMORT))) {
                    count++;
                    active_effect_remove(victim, eff);
                    didanything = true;
                }
                eff = next_eff;
            }
        }

        if (attack_ok(ch, victim, false)) {
            mag_damage(skill, ch, victim, SPELL_DISPEL_MAGIC, savetype);
            if (DECEASED(victim))
                return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
            didanything = true;
        }

        if (!didanything) {
            char_printf(ch, "You failed.\n");
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }

        if (count) {
            if (ch != victim) {
                act("&4&bYou have nullified some of $N&4&b's magic!&0", false, ch, 0, victim, TO_CHAR);
                act("&4&b$n&4&b has nullified some of your magic!&0", false, ch, 0, victim, TO_VICT);
                act("&4&b$n&4&b has nullified some of $N&4&b's magic!&0", false, ch, 0, victim, TO_NOTVICT);
            } else {
                act("&4&bYou have nullified some of your magic.&0", false, ch, 0, victim, TO_CHAR);
                act("&4&b$n&4&b has nullified some of $s magic!&0", false, ch, 0, victim, TO_NOTVICT);
            }
        }
    }

    if (obj) {
        switch (GET_OBJ_TYPE(obj)) {
        case ITEM_WALL:
            if (GET_LEVEL(ch) >= LVL_IMMORT || (GET_OBJ_VAL(obj, VAL_WALL_DISPELABLE) && random_number(1, 100) > 60)) {
                destroy_opposite_wall(obj);
                decay_object(obj);
                return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
            }
            break;
        default:
            if (OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
                if (random_number(1, 50) + skill > GET_OBJ_LEVEL(obj)) {
                    REMOVE_FLAG(GET_OBJ_FLAGS(obj), ITEM_INVISIBLE);
                    act("$p&0 fades into existence.", false, ch, obj, 0, TO_CHAR);
                    act("$p&0 fades into existence.", false, ch, obj, 0, TO_ROOM);
                } else
                    char_printf(ch, "You fail.\n");
                return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
            } else {
                char_printf(ch, NOEFFECT);
                return CAST_RESULT_CHARGE;
            }
        }
    }
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/* See if the room permits recalling.  If not, sends a message
 * to the player and the room and returns false.
 * Otherwise returns true. */

int room_recall_check(CharData *ch, CharData *victim, ObjData *obj) {
    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;

    if (ROOM_FLAGGED(ch->in_room, ROOM_NORECALL)) {
        if (obj) {
            act("$n speaks the words of $p, but to no effect!", false, ch, obj, 0, TO_ROOM);
            act("You speak the words on $p, but nothing happens!", false, ch, obj, 0, TO_CHAR);
        } else {
            act("$n's spell dissipates uselessly.", true, ch, 0, 0, TO_ROOM);
            char_printf(victim, "You feel a force drawing you away for a moment, but then the spell falters.\n");
            if (victim != ch)
                act("$N begins to disappear, but then the spell falters.", true, ch, 0, victim, TO_CHAR);
        }
        return false;
    }

    return true;
}

ASPELL(spell_recall) {
    int location;
    bool wasdark;

    if (victim == nullptr || IS_NPC(victim))
        return 0;

    if (!room_recall_check(ch, victim, nullptr))
        return CAST_RESULT_CHARGE;

    if ((location = real_room(GET_HOMEROOM(victim))) < 0) {
        log("SYSERR: Could not find homeroom for victim of spell 'recall'.");
        return 0;
    }
    dismount_char(victim);
    act("$n utters a single word...", true, victim, 0, 0, TO_ROOM);
    act("&7&b$n&7&b disappears in a bright flash of light!&0", true, victim, 0, 0, TO_ROOM);
    wasdark = IS_DARK(victim->in_room) && !CAN_SEE_IN_DARK(victim);

    char_from_room(victim);
    char_to_room(victim, location);
    act("&7&b$n&7&b appears in a bright flash of light.&0", true, victim, 0, 0, TO_ROOM);
    check_new_surroundings(victim, wasdark, false);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_world_teleport) {
    room_num to_room;

    if (victim == nullptr || IS_NPC(victim))
        return 0;

    do {
        to_room = random_number(0, top_of_world);
    } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE) || ROOM_FLAGGED(to_room, ROOM_DEATH) ||
             ROOM_FLAGGED(to_room, ROOM_GODROOM));

    act("$n slowly fades out of existence and is gone.", false, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, to_room);
    act("$n slowly fades into existence.", false, victim, 0, 0, TO_ROOM);
    look_at_room(victim, 0);
    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_teleport) {
    int location, to_room;
    RoomData *rm = &world[ch->in_room];
    int zone = rm->zone, tries = 100;
    bool wasdark;

    /* Check for success. */
    if (random_number(1, 100) > 10 + skill * 2) {
        char_printf(ch, "&7The spell swirls about and dies away.&0\n");
        if (ch == victim)
            act("&7$n tries to teleport $mself, but fails.&0", false, ch, 0, victim, TO_NOTVICT);
        else
            act("&7$n tries to teleport $N, but fails.&0", false, ch, 0, victim, TO_NOTVICT);
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }

    do {
        location = random_number((zone_table[zone].number) * 100, zone_table[zone].top);
        tries--;
    } while (((to_room = real_room(location)) < 0 ||
              (ROOM_FLAGGED(to_room, ROOM_PRIVATE) || ROOM_FLAGGED(to_room, ROOM_DEATH))) &&
             tries);

    /* Here's how the above fails to be a infinite loop, crashing the mud,
     * when the area has no suitable destination rooms. */
    if (!tries) {
        act("$n flickers briefly.", false, victim, 0, 0, TO_ROOM);
        char_printf(victim, "The spell sputters out.\n");
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }

    act("&9&b$n &9&bslowly fades out of existence and is gone.&0", false, victim, 0, 0, TO_ROOM);
    act("&9&bYou feel your body being pulled in all directions, then find "
        "yourself elsewhere.&0",
        false, ch, 0, victim, TO_VICT);
    wasdark = IS_DARK(victim->in_room) && !CAN_SEE_IN_DARK(victim);
    dismount_char(victim);
    char_from_room(victim);
    char_to_room(victim, to_room);
    act("&7&b$n&7&b fades into existence.&0", false, victim, 0, 0, TO_ROOM);
    check_new_surroundings(victim, wasdark, true);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

#define SUMMON_FAIL "You failed.\n"

ASPELL(spell_summon) {
    int track;
    bool wasdark;
    int find_track_victim(CharData * ch, char *name, int maxdist, CharData **victim);

    if (ch == nullptr)
        return 0;

    track = find_track_victim(ch, ch->casting.misc, skill / 5, &victim);

    if (track == BFS_ERROR) {
        char_printf(ch, "Your magic dissipates uselessly.\n");
        return CAST_RESULT_IMPROVE;
    }
    if (track == BFS_NO_PATH && victim) {
        if (world[ch->in_room].zone == world[victim->in_room].zone)
            track = BFS_ALREADY_THERE;
    }
    if (track == BFS_NO_PATH || !victim || world[ch->in_room].zone != world[victim->in_room].zone) {
        char_printf(ch, "That person is too far away.\n");
        return CAST_RESULT_CHARGE;
    }

    if (GET_LEVEL(victim) > std::min(LVL_IMMORT, skill + 3)) {
        char_printf(ch, "You aren't proficient enough to summon such a powerful being.\n");
        return CAST_RESULT_CHARGE;
    }

    if (MOB_FLAGGED(victim, MOB_NOSUMMON) || MOB_FLAGGED(victim, MOB_NOCHARM)) {
        char_printf(ch, "You feel your magic probing {}, but it can't seem to get a grip.\n", PERS(victim, ch));
        return CAST_RESULT_CHARGE;
    }

    if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOSUMMON)) {
        char_printf(ch, "A negating force blocks your spell.\n");
        return CAST_RESULT_CHARGE;
    }

    if ((ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) && !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))) {
        char_printf(ch, "You can't summon someone into an arena room.\n");
        return CAST_RESULT_CHARGE;
    }

    if (!summon_allowed) {
        if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
            act("As the words escape your lips and $N travels\n"
                "through time and space towards you, you realize that $E is\n"
                "aggressive and might harm you, so you wisely send $M back.",
                false, ch, 0, victim, TO_CHAR);
            return CAST_RESULT_CHARGE;
        }
        if (IS_PC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) && !PLR_FLAGGED(victim, PLR_KILLER)) {
            char_printf(victim,
                        "{} just tried to summon you to: {}.\n"
                        "{} failed because you have summon protection on.\n"
                        "Type NOSUMMON to allow other players to summon you.\n",
                        GET_NAME(ch), world[ch->in_room].name.c_str(),
                        (ch->player.sex == SEX_MALE) ? "He" : ((ch->player.sex == SEX_FEMALE) ? "She" : "They"));

            char_printf(ch, "You failed because {} has summon protection on.\n", GET_NAME(victim));

            log(LogSeverity::Warn, LVL_IMMORT, "{} failed summoning {} to {}.", GET_NAME(ch), GET_NAME(victim),
                world[ch->in_room].name);
            return CAST_RESULT_CHARGE;
        }
    }

    if (!IS_PC(victim) && mag_savingthrow(victim, SAVING_SPELL)) {
        char_printf(ch, SUMMON_FAIL);
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }

    dismount_char(victim);
    act("$n disappears suddenly.", true, victim, 0, 0, TO_ROOM);
    wasdark = IS_DARK(victim->in_room) && !CAN_SEE_IN_DARK(victim);

    char_from_room(victim);
    char_to_room(victim, ch->in_room);

    act("$n arrives suddenly.", true, victim, 0, 0, TO_ROOM);
    act("$n has summoned you!", false, ch, 0, victim, TO_VICT);
    check_new_surroundings(victim, wasdark, true);
    WAIT_STATE(ch, PULSE_VIOLENCE * 4);

    /*   if the target of the successful summon is an NPC, make it */
    /*   attack the summoner! muahahahahahaha! (Demolitum) */
    if (!IS_PC(victim)) {
        act("$N turns on $S summoner, $n!", false, ch, 0, victim, TO_ROOM);
        act("Magical bindings crumble and $N turns on you!", false, ch, 0, victim, TO_CHAR);
        set_fighting(victim, ch, false);
    }

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

#define MAX_LOCATE_ITEMS 50

ASPELL(spell_locate_object) {
    ObjData *o, *tmp;
    char name[MAX_INPUT_LENGTH];
    int j,         /* Maximum of items to find, according to the power of the casting */
        found = 0, /* the number of items actually found */
        t = 0,     /* Total number of eligible objects detected */
        i, k;
    CharData *holder;

    ObjData *items[MAX_LOCATE_ITEMS];

    strcpy(name, ch->casting.misc);

    /* How many items can you locate with this casting strength? */
    j = std::min(MAX_LOCATE_ITEMS, std::max(MAX_LOCATE_ITEMS * skill / 100, 1));

    /* Loop through every object in the game */
    for (o = object_list; o; o = o->next) {

        /* Determine whether this object meets the search criteria:
         *
         *  - it is not flagged !locate
         *  - it has the requested name
         *  - the caster can see it
         *  - the object is not held by an invisible god
         *  - the power of the spell is sufficient:
         *    -- obj_level <= power: OK
         *    -- obj_level - power <= 10: probability declines by 10% per level
         *    -- obj_level - power > 10: No, you cannot locate this object
         */

        if (OBJ_FLAGGED(o, ITEM_NOLOCATE))
            continue;

        if (!isname(name, o->name) || !CAN_SEE_OBJ((ch), (o)) || GET_OBJ_LEVEL(o) - skill > 10)
            continue;

        for (tmp = o; tmp->in_obj; tmp = tmp->in_obj)
            ;
        if (tmp->worn_by)
            holder = tmp->worn_by;
        else if (tmp->carried_by)
            holder = o->carried_by;
        else
            holder = nullptr;
        if (holder && GET_INVIS_LEV(holder) > GET_LEVEL(ch))
            continue;

        /* If obj is 10 levels above power, you have 10% chance of locating
         * If 4 levels above, 70% chance
         * Else 80% chance */
        if (GET_OBJ_LEVEL(o) - skill > 3) {
            if (random_number(1, 10) >= 12 - GET_OBJ_LEVEL(o) + skill)
                continue;
        } else if (random_number(1, 10) < 3)
            continue;

        /* This object can be detected */
        t++;

        if (found < j)
            /* We are still gathering up the first j number of items, so
             * go ahead and stash it in the list */
            items[found++] = o;
        else
            /* Already got j items.  Probability of storing this one is j/t */
            if (random_number(1, t) <= j)
                /* Overwrite a random one of the objects already in the list */
                items[random_number(0, j - 1)] = o;
    }

    if (!found) {
        char_printf(ch, "You sense nothing.\n");
        return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
    }

    /* Shuffle these around a bit!  */
    for (i = 0; i < (100 - skill) / 3; i++) {
        t = random_number(0, found - 1);
        k = random_number(0, found - 1);
        if (t != k) {
            tmp = items[t];
            items[t] = items[k];
            items[k] = tmp;
        }
    }

    /* List located objects to the caster */
    for (i = 0; i < found; i++) {
        o = items[i];

        if (o->carried_by)
            char_printf(ch, "{} is being carried by {}.\n", capitalize(o->short_description), PERS(o->carried_by, ch));
        else if (o->in_room != NOWHERE)
            char_printf(ch, "{} is in {}.\n", capitalize(o->short_description), world[o->in_room].name.c_str());
        else if (o->in_obj)
            char_printf(ch, "{} is in {}.\n", capitalize(o->short_description), o->in_obj->short_description);
        else if (o->worn_by)
            char_printf(ch, "{} is being worn by {}.\n", capitalize(o->short_description), PERS(o->worn_by, ch));
        else
            char_printf(ch, "{}'s location is uncertain.\n", capitalize(o->short_description));
    }

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

static MATCH_OBJ_FUNC(match_corpse_by_name) {
    if (obj->in_room != NOWHERE)
        if (IS_PLR_CORPSE(obj))
            if (isname(context->string, obj->name))
                if (CAN_SEE_OBJ(context->ch, obj))
                    if (--context->number <= 0)
                        return true;
    return false;
}

/* Necromancer / Anti-paladin spell, by Zzur's request (9/14/02) jjl */
ASPELL(spell_summon_corpse) {
    int o_zone, c_zone;
    FindContext find_corpse = find_vis_by_name(ch, ch->casting.misc);
    find_corpse.obj_func = match_corpse_by_name;
    obj = find_obj_in_world(find_corpse);

    /* Make sure the obj is valid */
    if (!obj) {
        char_printf(ch, "Couldn't find any such corpse!\n");
        return CAST_RESULT_CHARGE;
    }

    /* This spell is zone restricted.   Make sure the char and obj are in
       the same zone. */
    o_zone = world[ch->in_room].zone;
    c_zone = world[obj->in_room].zone;
    if (o_zone != c_zone) {
        char_printf(ch, "{} is much too far away for your magic to reach it.\n", obj->short_description);
        return CAST_RESULT_CHARGE;
    }

    /* The spell also requires consent. */
    if (!has_corpse_consent(ch, obj)) {
        char_printf(ch, "Perhaps with consent. . . \n");
        log(LogSeverity::Stat, LVL_IMMORT, "{} tried to summon {} without consent!", GET_NAME(ch),
            obj->short_description);
        return CAST_RESULT_CHARGE;
    }

    /* If they're in the same room, don't bother. */
    if (obj->in_room == ch->in_room) {
        char_printf(ch, "{} is already here!\n", obj->short_description);
        return CAST_RESULT_CHARGE;
    }

    /* Print success to the original room. */
    act("A cloud of &9&bdarkness&0 envelopes $p, which fades out of existence.", false, 0, obj, 0, TO_ROOM);

    /* Perform the actual move. */
    obj_from_room(obj);
    obj_to_room(obj, ch->in_room);

    /* Print success to the new room. */
    room_printf(ch->in_room,
                "From within a cloud of &9&bdarkness&0, a corpse "
                "materializes.\n");

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

ASPELL(spell_shift_corpse) {
    FindContext find_corpse = find_vis_by_name(ch, ch->casting.misc);
    find_corpse.obj_func = match_corpse_by_name;
    obj = find_obj_in_world(find_corpse);

    /* Make sure the obj is valid */
    if (!obj) {
        char_printf(ch, "Couldn't find any such corpse!\n");
        return CAST_RESULT_CHARGE;
    }

    /* And we have permission */
    if (!has_corpse_consent(ch, obj)) {
        char_printf(ch, "Perhaps with consent. . . \n");
        log(LogSeverity::Stat, LVL_IMMORT, "{} tried to summon {} without consent!", GET_NAME(ch),
            obj->short_description);
        return CAST_RESULT_CHARGE;
    }

    /* Make sure it's not already there */
    if (obj->in_room == ch->in_room) {
        char_printf(ch, "{} is already here!\n", obj->short_description);
        return CAST_RESULT_CHARGE;
    }

    /* Print success to old room */
    act("The hand of &9&bdeath&0 lifts $p into the air, and disappears.", false, 0, obj, 0, TO_ROOM);

    /* Do the move */
    obj_from_room(obj);
    obj_to_room(obj, ch->in_room);

    /* Success to the new room */
    char_printf(ch, "The hand of &9&bdeath&0 delivers {} to your feet.\n", obj->short_description);

    act("The hand of &9&bdeath&0 delivers $p to $n's feet.", false, ch, obj, 0, TO_ROOM);

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

/*
   >   Upon entering the spell_ressurect function, it is given that the
   >   player being ressed is actively in the game. If the player being
   >   ressed is not found, that is handled above this function
*/
ASPELL(spell_resurrect) {
    extern long exp_death_loss(CharData * ch, int level);
    ObjData *corpse, *object, *next_obj;
    int i;
    long exp = 0;

    /* check for any linkdeads */
    if (ch == nullptr || victim == nullptr)
        return 0;
    /* no ressing imms+ */
    if (GET_LEVEL(victim) > (LVL_IMMORT - 1)) {
        char_printf(ch, "No need to resurrect a god.\n");
        return CAST_RESULT_CHARGE;
    }
    /* no ressing mobs */
    if (IS_NPC(victim)) {
        char_printf(ch, "You can not resurrect creatures.\n");
        return CAST_RESULT_CHARGE;
    }
    /* look for the proper corpse within the room */
    for (corpse = world[ch->in_room].contents; corpse; corpse = corpse->next_content) {
        if (CAN_SEE_OBJ(ch, corpse) && isname(GET_NAME(victim), corpse->name) && IS_CORPSE(corpse)) {
            /* found the corpse */
            act("$n howls in pain as $s body crumbles to dust.", true, victim, 0, 0, TO_ROOM);
            /* spill all items on the old body into the room */
            for (object = victim->carrying; object; object = next_obj) {
                next_obj = object->next_content;
                obj_from_char(object);
                obj_to_obj(object, corpse);
            }
            for (i = 0; i < NUM_WEARS; i++)
                if (GET_EQ(victim, i))
                    obj_to_obj(unequip_char(victim, i), corpse);

            /* move victim to corpse location */
            char_from_room(victim);
            char_to_room(victim, ch->in_room);

            /* transfer eq from corpse to players inventory */
            for (object = corpse->contains; object; object = next_obj) {
                next_obj = object->next_content;
                obj_from_obj(object);
                obj_to_char(object, victim);

                if (GET_OBJ_TYPE(object) == ITEM_MONEY) {
                    if (MONEY_VALUE(object) > 0) {
                        obj_from_char(object);
                        GET_PLATINUM(victim) += GET_OBJ_VAL(object, VAL_MONEY_PLATINUM);
                        GET_GOLD(victim) += GET_OBJ_VAL(object, VAL_MONEY_GOLD);
                        GET_SILVER(victim) += GET_OBJ_VAL(object, VAL_MONEY_SILVER);
                        GET_COPPER(victim) += GET_OBJ_VAL(object, VAL_MONEY_COPPER);
                        extract_obj(object);
                    }
                }
            }

            /* -->Add any exp modifiers here<-- */

            /*return exp to make life easy just return the exp of the level he at
               now */
            /*GET_OBJ_LEVEL(obj) can be used to find the level of the corpse and
               thus return
               the correct exp */
            exp = exp_death_loss(victim, GET_OBJ_LEVEL(corpse));
            exp = 0.60 * exp;
            gain_exp(victim, exp, GAIN_IGNORE_LEVEL_BOUNDARY | GAIN_IGNORE_LOCATION);

            /* ditch the corpse */
            extract_obj(corpse);

            act("$n's body gasps for breath and $s eyes slowly open.\n$n has been "
                "resurrected!",
                true, victim, 0, 0, TO_ROOM);
            act("$n has summoned you from the land of the dead!", false, ch, 0, victim, TO_VICT);
            act("You feel tired after being resurrected.", false, ch, 0, victim, TO_VICT);
            return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
        }
    }
    /* corpse was not found in room */
    char_printf(ch, "There is no corpse here with that description.\n");
    return CAST_RESULT_CHARGE;
}

#define MAX_MOONBEAM_TARGETS 15
ASPELL(spell_moonbeam) {
    CharData *victims[MAX_MOONBEAM_TARGETS], *vict;
    int numvicts = 0, numconsidered = 0, numallowed, i;

    /* Todo: establish the movements of the moon, and use that instead of just
     * assuming you can't have moonbeams when the sun is out. */

    act("$n floods the area with cool, soothing &bmoon&7beams&0.", false, ch, 0, 0, TO_ROOM);
    act("You flood the area with cool, soothing &bmoon&7beams&0.", false, ch, 0, 0, TO_CHAR);

    /* Determine how many victims the spell can affect, given the
     * power of the casting */
    numallowed = std::min(std::max(1, skill / 10), MAX_MOONBEAM_TARGETS);

    /* Select victims. */
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
        if (GET_LEVEL(vict) >= LVL_IMMORT && !IS_NPC(vict))
            continue;
        if (ch == vict || is_grouped(ch, vict))
            continue;
        if (!mass_attack_ok(ch, vict, false))
            continue;

        /* Blind folks immune to this spell. */
        if (EFF_FLAGGED(vict, EFF_BLIND))
            continue;

        if (damage_evasion(vict, 0, 0, skills[spellnum].damage_type)) {
            act("$n is unaffected!", false, vict, 0, 0, TO_ROOM);
            act("You are unaffected!", false, vict, 0, 0, TO_CHAR);
            continue;
        }

        /* This is a valid target. */
        numconsidered++;
        if (numvicts < numallowed)
            victims[numvicts++] = vict;
        else if (random_number(1, numconsidered) == 1)
            victims[random_number(0, numallowed - 1)] = vict;
    }

    if (numvicts == 0) {
        char_printf(ch, "No one seems to care.\n");
        return CAST_RESULT_CHARGE;
    }

    /* Rain down cool, soothing terror upon them */
    for (i = 0; i < numvicts; i++) {
        vict = victims[i];

        /* Hurt it */
        mag_damage(skill, ch, vict, SPELL_MOONBEAM, SAVING_SPELL);
        if (DECEASED(vict))
            continue;

        /* Paralysis/insanity? */
        if (random_number(1, 100) > 4 + skill - GET_LEVEL(vict))
            continue;

        if (random_number(1, 2) == 1) {
            mag_affect(skill, ch, vict, SPELL_MINOR_PARALYSIS, SAVING_SPELL, CAST_SPELL);
        } else {
            mag_affect(skill, ch, vict, SPELL_INSANITY, SAVING_SPELL, CAST_SPELL);
        }
    }

    return CAST_RESULT_CHARGE | CAST_RESULT_IMPROVE;
}

int inflict_fear(CharData *ch, CharData *victim, int power, bool multi) {
    effect effect;
    ObjData *weap;
    int opos;
    bool fightback = false;

    if (!attack_ok(ch, victim, true))
        return 0;

    if (!AWAKE(victim)) {
        if (!multi)
            act("$N is in no condition to notice your illusion.", false, ch, 0, victim, TO_CHAR);
        return 0;
    }

    if (EFF_FLAGGED(victim, EFF_MINOR_PARALYSIS)) {
        if (!multi) {
            act("$N doesn't even move.", false, ch, 0, victim, TO_CHAR);
            act("$N doesn't doesn't appear to notice.", false, ch, 0, victim, TO_NOTVICT);
            act("$n shows you visions of great horror, but you can't even move!", false, ch, 0, victim, TO_VICT);
        }
        return 0;
    }

    weap = GET_EQ(victim, WEAR_WIELD);
    opos = WEAR_WIELD;
    if (!weap) {
        weap = GET_EQ(victim, WEAR_WIELD2);
        opos = WEAR_WIELD2;
    }
    if (!weap) {
        weap = GET_EQ(victim, WEAR_2HWIELD);
        opos = WEAR_2HWIELD;
    }

    if (random_number(0, 100) < std::min(80, 17 * (power - GET_LEVEL(victim)) / 10)) {
        /* Minor paralysis */
        memset(&effect, 0, sizeof(effect));
        effect.type = SPELL_FEAR;
        effect.duration = 2 + (power / 30); /* max 5 */
        SET_FLAG(effect.flags, EFF_MINOR_PARALYSIS);
        effect.modifier = 0;
        effect.location = APPLY_NONE;
        act("You frighten $N so bad that $E is frozen in terror!", false, ch, 0, victim, TO_CHAR);
        act("&5$n shows you a vision so &bterrifying&0&5 that you freeze in "
            "horror!&0",
            false, ch, 0, victim, TO_VICT);
        act("&5$N is frozen in shock at $n's vision of &bterror!&0", false, ch, 0, victim, TO_NOTVICT);
        stop_fighting(victim);
        STOP_CASTING(victim);
        effect_to_char(victim, &effect);
        if (FIGHTING(ch) == victim)
            stop_fighting(ch);
        remember(victim, ch);
    } else if (weap && random_number(0, 100) < std::min(85, 1 + 18 * (power - GET_LEVEL(victim)) / 10)) {
        /* Drop weapon */
        act("You made $N drop $S $o!", false, ch, weap, victim, TO_CHAR);
        act("$n frightens you so badly that you forget to hold on to your $o!", false, ch, weap, victim, TO_VICT);
        act("$N is so terrified that $E drops $o!", false, ch, weap, victim, TO_NOTVICT);
        obj_to_room(unequip_char(victim, opos), victim->in_room);
        stop_fighting(victim);
        STOP_CASTING(victim);
        WAIT_STATE(victim, (PULSE_VIOLENCE));
        fightback = true;
    } else if (random_number(0, 100) < std::min(90, 3 + 19 * (power - GET_LEVEL(victim)) / 10)) {
        /* Flee */
        stop_fighting(victim);
        STOP_CASTING(victim);
        act("$N shrieks madly at your vision of terror!", false, ch, 0, victim, TO_CHAR);
        act("$n fills you with such horror that you panic!", false, ch, 0, victim, TO_VICT);
        act("$N shrieks uncontrollably!", false, ch, 0, victim, TO_NOTVICT);
        /* Turn off wait states so they can flee. */
        if (IS_NPC(victim))
            GET_MOB_WAIT(victim) = 0;
        else if (victim->desc)
            victim->desc->wait = 0;
        delayed_command(victim, "flee", 0, false);
        WAIT_STATE(victim, (PULSE_VIOLENCE));
        remember(victim, ch);
    } else if (random_number(0, 100) < std::min(95, 5 + 20 * (power - GET_LEVEL(victim)) / 10)) {
        /* Falter */
        act("$N gets a scared look, but soldiers on.", false, ch, 0, victim, TO_CHAR);
        act("$n frightens you, but you recover.", false, ch, 0, victim, TO_VICT);
        act("$N looks frightened at $n's fearful illusion, but recovers.", false, ch, 0, victim, TO_NOTVICT);
        stop_fighting(victim);
        STOP_CASTING(victim);
        WAIT_STATE(victim, PULSE_VIOLENCE / 2);
        fightback = true;
    } else {
        act("$N barely raises an eyebrow at your fearful illusion.", false, ch, 0, victim, TO_CHAR);
        act("$n tries to frighten you with a pitiful illusion.  Yawn.", false, ch, 0, victim, TO_VICT);
        act("$N barely notices when $n tries to frighten $M.", false, ch, 0, victim, TO_NOTVICT);
        fightback = true;
    }

    if (fightback && attack_ok(victim, ch, false))
        set_fighting(victim, ch, false);

    return CAST_RESULT_IMPROVE;
}

ASPELL(spell_fear) {
    int ret = 0;
    CharData *tch, *uch;
    extern int roomeffect_allowed;

    if (spellnum == SPELL_FEAR) {
        ret = inflict_fear(ch, victim, skill, false);
    } else if (spellnum == SPELL_HYSTERIA) {
        for (tch = CH_ROOM(ch)->people; tch; tch = uch) {
            uch = tch->next_in_room;
            if (tch == ch)
                continue;
            if (is_grouped(ch, tch))
                continue;
            if (!roomeffect_allowed && !IS_NPC(ch) && !IS_NPC(tch))
                continue;
            if (!mass_attack_ok(ch, tch, false))
                continue;
            /* Mobs don't hit other mobs, unless they're pets */
            if (!IS_PC(ch) && !IS_PC(tch) && !PLAYERALLY(ch) && !PLAYERALLY(tch))
                continue;
            ret |= inflict_fear(ch, tch, skill, true);
        }
    } else {
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: spell_fear() got invalid spellnum {}", spellnum);
        return 0;
    }

    return CAST_RESULT_CHARGE | ret;
}
