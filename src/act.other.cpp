/***************************************************************************
 *   File: act.other.c                                    Part of FieryMUD *
 *  Usage: Miscellaneous player-level commands                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "casting.hpp"
#include "chars.hpp"
#include "clan.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "cooldowns.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "events.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "house.hpp"
#include "interpreter.hpp"
#include "lifeforce.hpp"
#include "limits.hpp"
#include "magic.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "money.hpp"
#include "movement.hpp"
#include "pfiles.hpp"
#include "players.hpp"
#include "quest.hpp"
#include "races.hpp"
#include "regen.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "specprocs.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"
#include "version.hpp"

#include <fmt/format.h>
#include <math.h>
#include <sys/stat.h>

EVENTFUNC(camp_event);
void rem_memming(CharData *ch);
void summon_mount(CharData *ch, int mob_vnum, int base_hp, int base_mv);
void appear(CharData *ch);
void check_new_surroundings(CharData *ch, bool old_room_was_dark, bool tx_obvious);
void get_check_money(CharData *ch, ObjData *obj);
int roll_skill(CharData *ch, int skill);

/* extern procedures */
SPECIAL(shop_keeper);

void appear(CharData *ch) {
    bool was_hidden;

    active_effect_from_char(ch, SPELL_INVISIBLE);
    active_effect_from_char(ch, SPELL_NATURES_EMBRACE);

    was_hidden = IS_HIDDEN(ch);

    REMOVE_FLAG(EFF_FLAGS(ch), EFF_INVISIBLE);
    REMOVE_FLAG(EFF_FLAGS(ch), EFF_CAMOUFLAGED);
    GET_HIDDENNESS(ch) = 0;

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (was_hidden) {
            act("$n steps out of the shadows.", false, ch, 0, 0, TO_ROOM);
            send_to_char("You step out of the shadows.\n", ch);
        } else {
            act("$n snaps into visibility.", false, ch, 0, 0, TO_ROOM);
            send_to_char("You fade back into view.\n", ch);
        }
    } else
        act("You feel a strange presence as $n appears, seemingly from nowhere.", false, ch, 0, 0, TO_ROOM);
}

void stop_guarding(CharData *ch) {

    if (ch->guarding) {
        act("You stop guarding $N.", false, ch, 0, ch->guarding, TO_CHAR);
        if (ch->guarding->guarded_by == ch) {
            act("$n stops guarding you.", true, ch, 0, ch->guarding, TO_VICT);
            ch->guarding->guarded_by = nullptr;
        }
        ch->guarding = nullptr;
    }
    if (ch->guarded_by)
        stop_guarding(ch->guarded_by);
}

ACMD(do_guard) {
    CharData *vict;

    one_argument(argument, arg);

    if (!GET_SKILL(ch, SKILL_GUARD)) {
        send_to_char("You don't have the protective skill required to guard.\n", ch);
        return;
    }

    if (!*arg) {
        if (ch->guarding)
            act("You are guarding $N.", false, ch, 0, ch->guarding, TO_CHAR);
        else
            send_to_char("You are not guarding anyone.\n", ch);
        return;
    }

    if (!strcasecmp(arg, "off"))
        vict = ch;
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        send_to_char("That person is not here.\n", ch);
        return;
    }

    if (ch == vict) {
        if (ch->guarding)
            stop_guarding(ch);
        else
            send_to_char("You are not guarding anyone.\n", ch);
        return;
    }

    if (vict->guarded_by) {
        if (vict->guarded_by == ch)
            send_to_char("You are already guarding that person.\n", ch);
        else
            send_to_char("Someone else is already guarding that person.\n", ch);
        return;
    }

    if (ch->guarding) {
        if (ch->guarding == vict) {
            send_to_char("You are already guarding that person.\n", ch);
            return;
        } else
            stop_guarding(ch);
    }
    act("You start guarding $N.", false, ch, 0, vict, TO_CHAR);
    act("$n starts guarding you.", true, ch, 0, vict, TO_VICT);
    act("$n lays a protective eye on $N, guarding $M.", true, ch, 0, vict, TO_NOTVICT);
    ch->guarding = vict;
    vict->guarded_by = ch;
}

ACMD(do_subclass) {
    int rem_spell(CharData * ch, int spell);
    int subclass, anyvalid;
    QuestList *quest = nullptr;
    MemorizedList *memorized, *last_mem;
    float old_exp;
    char *s;
    ClassDef *c;

    /* Ew */
    if (IS_NPC(ch)) {
        send_to_char("NPCs don't subclass.  Ha.\n", ch);
        return;
    }

    c = &(classes[(int)GET_CLASS(ch)]);

    /* If not a base class, then bail */
    if (c->is_subclass) {
        send_to_char("You can only subclass once!\n", ch);
        return;
    }

    /* If below minimum quest level, bail */
    if (GET_LEVEL(ch) < 10) {
        send_to_char("You need to be level 10 before you can subclass!\n", ch);
        return;
    }

    /* If above maximum quest level, bail */
    if (GET_LEVEL(ch) > c->max_subclass_level) {
        sprintf(buf, "You can no longer subclass, because you are over level %d.\n", c->max_subclass_level);
        send_to_char(buf, ch);
        return;
    }

    /* Figure out whether the player is on a subclass quest */
    if (ch->quests) {
        quest = ch->quests;
        while (quest && !(quest->quest_id & SUBCLASS_BIT))
            quest = quest->next;
        /* quest should now point to the player's subclass quest, if it exists */
    }

    /* If not on subclass quest, show the list of possible subclasses */
    if (!quest) {
        anyvalid = 0;
        for (subclass = 0; subclass < NUM_CLASSES; subclass++) {
            if (classes[subclass].active && classes[subclass].is_subclass &&
                classes[subclass].subclass_of == GET_CLASS(ch) && class_ok_race[(int)GET_RACE(ch)][subclass]) {
                sprintf(buf, "  %s\n", classes[subclass].fmtname);
                if (!anyvalid) {
                    send_to_char("You may choose from the following classes for your race:\n", ch);
                    anyvalid = 1;
                }
                send_to_char(buf, ch);
            }
        }

        if (anyvalid) {
            sprintf(buf,
                    "You have until level %d to subclass. See HELP SUBCLASS_%s for "
                    "more information.\n",
                    c->max_subclass_level, c->name);
            /* Capitalize the "SUBCLASS_class" bit */
            for (s = buf + 43; *s && *s != ' '; s++)
                *s = toupper(*s);
            send_to_char(buf, ch);
        } else
            send_to_char("There are no subclasses available to you.\n", ch);
        return;
    }

    /* Now we know the player has started the subclass quest. */

    /* Make sure the class --> subclass change is possible */
    subclass =
        parse_class(0, 0, get_quest_variable(ch, all_quests[real_quest(quest->quest_id)].quest_name, "subclass_name"));
    if (subclass == CLASS_UNDEFINED) {
        sprintf(buf, "%s finished subclass quest \"%s\" with unknown target subclass \"%s\"", GET_NAME(ch),
                all_quests[real_quest(quest->quest_id)].quest_name,
                get_quest_variable(ch, all_quests[real_quest(quest->quest_id)].quest_name, "subclass_name"));
        log("%s", buf);
        send_to_char("There is an error in your subclass quest.  Ask a god to reset it.\n", ch);
        return;
    }

    /* If it is completed, subclass them */
    if (quest->stage == QUEST_SUCCESS) {
        /* Leveled percentage */
        old_exp = GET_EXP(ch) / (double)exp_next_level(GET_LEVEL(ch), GET_CLASS(ch));

        /* Clear spells the new class doesn't have */
        memorized = GET_SPELL_MEM(ch).list_head;
        while (memorized) {
            last_mem = memorized;
            memorized = memorized->next;
            if (skills[last_mem->spell].min_level[subclass] > GET_LEVEL(ch))
                rem_spell(ch, last_mem->spell);
        }

        /* Change class and exp */
        GET_CLASS(ch) = subclass;
        GET_EXP(ch) = (long)exp_next_level(GET_LEVEL(ch), GET_CLASS(ch)) * old_exp;

        /* Update class and race-related things */
        update_char(ch);

        /* Hubis crap */
        check_regen_rates(ch);

        char_printf(ch, "You have successfully subclassed as %s!\n", with_indefinite_article(CLASS_FULL(ch)));
        all_except_printf(ch, "%s has subclassed to %s!\n", GET_NAME(ch), CLASS_FULL(ch));
        mprintf(L_STAT, LVL_GOD, "%s has subclassed to %s", GET_NAME(ch), CLASS_FULL(ch));
        return;
    }

    /* Now we know the player is on a subclass quest, but not completed */
    sprintf(buf, "You are on the way to becoming a %s\n", classes[(int)subclass].fmtname);
    send_to_char(buf, ch);
    sprintf(buf, "You have until level %d to complete your quest.\n", c->max_subclass_level);
    send_to_char(buf, ch);
}

ACMD(do_quit) {
    int i;
    ObjData *money, *obj;
    one_argument(argument, arg);

    if (IS_NPC(ch) || !ch->desc) {
        send_to_char("You can't quit while shapechanged!\n", ch);
        return;
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        act("$n has left the game.", true, ch, 0, 0, TO_ROOM);
        send_to_char("Goodbye, friend.  Come back soon!\n", ch);
        remove_player_from_game(ch, QUIT_QUITIMM);
        return;
    }

    if (FIGHTING(ch)) {
        send_to_char("No way!  You're fighting for your life!\n", ch);
        return;
    }

    if (subcmd != SCMD_QUIT) {
        send_to_char("For safety purposes, you must type out 'quit yes'.\n", ch);
        send_to_char(
            "Note: You will lose &1&beverything&0 if you quit!  Camping "
            "or renting will save everything.\n",
            ch);
        return;
    }

    if (!*arg || strcasecmp(arg, "yes")) {
        send_to_char("You must type 'quit yes' to leave this world.\n", ch);
        send_to_char(
            "Note: You will lose &1&beverything&0 if you quit!  Camping "
            "or renting will save everything.\n",
            ch);
        return;
    }

    /* Ok, if we've made it this far it's ok to quit */

    if (GET_STANCE(ch) < STANCE_STUNNED) {
        send_to_char("You die before your time...\n", ch);
        act("$n quits the game, but is unable to fend off death...", true, ch, 0, 0, TO_ROOM);
        act("$n is dead!  R.I.P.", true, ch, 0, 0, TO_ROOM);
        die(ch, nullptr);
        return;
    }

    act("$n has left the game.", true, ch, 0, 0, TO_ROOM);

    sprintf(buf, "%s has quit the game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), true);
    send_to_char("Goodbye, friend.  Come back soon!\n", ch);

    /* transfer objects to room */
    while (ch->carrying) {
        obj = ch->carrying;
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
    }

    /* transfer equipment to room */
    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i))
            obj_to_room(unequip_char(ch, i), ch->in_room);

    /* And money */
    money = create_money(GET_COINS(ch));
    obj_to_room(money, ch->in_room);
    GET_GOLD(ch) = 0;
    GET_PLATINUM(ch) = 0;
    GET_COPPER(ch) = 0;
    GET_SILVER(ch) = 0;

    remove_player_from_game(ch, QUIT_QUITMORT);
}

#define MAMMAL (1 << 0)
#define BIRD (1 << 1)
#define REPTILE (1 << 2)
#define FISH (1 << 3)

#define TANK (1 << 4)
#define DAMAGER (1 << 5)
#define TRAVEL (1 << 6)
#define TRACKER (1 << 7)

#define AQUATIC (1 << 8)

#define MAX_SHAPECHANGE_SKILLS 5
#define SHAPE_VNUM_MIN 1000
#define SHAPE_VNUM_MAX 1038

const struct shapechange_data {
    const char *name;
    int vnum;
    int type;
    int midlevel;
    int minhp;
    int maxhp;
    int minmv;
    int maxmv;
    int skills[MAX_SHAPECHANGE_SKILLS];
} creatures[] = {
    /*   NAME      VNUM            TYPE            LVL  -HP   +HP  -MV  +MV
       SKILLS */
    /* ----------  ----  -----------------------   ---  ---  ----  ---  ---
       ------------- */
    {"rabbit", 1000, MAMMAL | DAMAGER, 30, 80, 140, 200, 250, {SKILL_SWITCH}},
    {"rat", 1001, MAMMAL | DAMAGER, 8, 40, 60, 80, 120, {SKILL_HIDE}},
    {"bat", 1002, MAMMAL | TRAVEL, 8, 50, 75, 140, 180, {SKILL_VAMP_TOUCH}},
    {"horse", 1003, MAMMAL | TRAVEL, 25, 220, 260, 180, 300, {0}},
    {"badger", 1004, MAMMAL | TANK, 30, 120, 160, 140, 170, {SKILL_CLAW, SKILL_SWITCH}},
    {"wolf", 1005, MAMMAL | TRACKER, 35, 400, 480, 270, 320, {SKILL_CLAW, SKILL_TRACK}},
    {"panther", 1006, MAMMAL | DAMAGER, 40, 380, 450, 300, 350, {SKILL_SWITCH, SKILL_SNEAK, SKILL_HIDE}},
    {"bear", 1007, MAMMAL | TANK, 45, 640, 740, 140, 160, {SKILL_ROAR, SKILL_RESCUE, SKILL_CORNER}},
    {"cheetah", 1008, MAMMAL | DAMAGER, 45, 420, 500, 400, 480, {SKILL_SWITCH, SKILL_RETREAT, SKILL_SPRINGLEAP}},
    {"rhino",
     1009,
     MAMMAL | TANK,
     50,
     860,
     1040,
     120,
     150,
     {SKILL_BODYSLAM, SKILL_RESCUE, SKILL_SWITCH, SKILL_RETREAT, SKILL_DOORBASH}},
    {"sparrow", 1010, BIRD | TRAVEL, 4, 40, 60, 140, 180, {SKILL_PECK}},
    {"bluebird", 1011, BIRD | TRAVEL, 6, 40, 70, 150, 200, {SKILL_PECK}},
    {"robin", 1012, BIRD | TRAVEL, 10, 60, 80, 125, 160, {SKILL_PECK}},
    {"owl", 1013, BIRD | TRAVEL, 15, 80, 100, 140, 160, {SKILL_SNEAK}},
    {"raven", 1014, BIRD | TRAVEL, 20, 100, 120, 160, 180, {0}},
    {"hawk", 1015, BIRD | TRAVEL, 25, 160, 190, 190, 220, {SKILL_PECK}},
    {"falcon", 1016, BIRD | DAMAGER, 30, 150, 180, 200, 240, {SKILL_CLAW, SKILL_PECK}},
    {"buzzard", 1017, BIRD | TRACKER, 35, 240, 280, 140, 160, {SKILL_PECK, SKILL_TRACK}},
    {"eagle", 1018, BIRD | DAMAGER, 40, 320, 380, 260, 300, {SKILL_CLAW, SKILL_PECK}},
    {"ostrich", 1019, BIRD | DAMAGER, 45, 360, 460, 500, 650, {SKILL_PECK, SKILL_KICK}},
    {"snake", 1020, REPTILE | TRACKER, 2, 25, 35, 80, 100, {SKILL_TRACK}},
    {"lizard", 1021, REPTILE | DAMAGER, 6, 45, 55, 80, 120, {0}},
    {"chameleon", 1022, REPTILE | TANK, 8, 60, 75, 80, 140, {SKILL_HIDE, SKILL_SNEAK, SKILL_STEALTH}},
    {"asp", 1023, REPTILE | TRACKER, 20, 120, 160, 120, 160, {SKILL_TRACK}},
    {"tortoise", 1024, REPTILE | TANK, 30, 320, 360, 40, 70, {0}},
    {"cobra", 1025, REPTILE | DAMAGER, 40, 280, 320, 140, 160, {SKILL_HIDE, SKILL_SNEAK}},
    {"turtle", 1026, REPTILE | TANK | AQUATIC, 45, 440, 480, 60, 100, {0}},
    {"viper", 1027, REPTILE | DAMAGER, 50, 340, 420, 140, 180, {SKILL_HIDE, SKILL_SNEAK}},
    {"crocodile", 1028, REPTILE | TANK, 55, 720, 830, 60, 120, {SKILL_RESCUE, SKILL_GUARD, SKILL_SWITCH}},
    {"anaconda",
     1029,
     REPTILE | TANK,
     60,
     945,
     1025,
     140,
     160,
     {SKILL_RESCUE, SKILL_GUARD, SKILL_SWITCH, SKILL_CORNER}},
    {"piranah", 1030, FISH | DAMAGER | AQUATIC, 4, 25, 35, 100, 150, {0}},
    {"eel", 1031, FISH | DAMAGER | AQUATIC, 9, 90, 110, 80, 120, {SKILL_CORNER, SKILL_SNEAK, SKILL_ELECTRIFY}},
    {"swordfish", 1032, FISH | TRAVEL | AQUATIC, 15, 100, 120, 200, 250, {0}},
    {"stingray",
     1033,
     FISH | TRACKER | AQUATIC,
     20,
     200,
     230,
     150,
     180,
     {SKILL_TRACK, SKILL_CORNER, SKILL_SNEAK, SKILL_STEALTH}},
    {"manatee", 1034, MAMMAL | TANK | AQUATIC, 30, 400, 450, 80, 100, {SKILL_GUARD, SKILL_RESCUE}},
    {"lamprey", 1035, FISH | DAMAGER | AQUATIC, 30, 300, 320, 120, 200, {SKILL_HIDE, SKILL_SNEAK}},
    {"dolphin", 1036, MAMMAL | TRAVEL | AQUATIC, 42, 400, 430, 400, 500, {SKILL_SWITCH}},
    {"shark", 1037, FISH | TRACKER | AQUATIC, 50, 480, 500, 240, 290, {SKILL_TRACK, SKILL_BODYSLAM, SKILL_SWITCH}},
    {"orca",
     1038,
     MAMMAL | TANK | AQUATIC,
     60,
     950,
     1100,
     150,
     200,
     {SKILL_CORNER, SKILL_BODYSLAM, SKILL_GUARD, SKILL_RESCUE}},
    {"\n", 0, 0, 0, 0, 0, 0, 0, {0}}};

ACMD(do_shapechange) {
    int index, type, class_num, desired_index = -1, i;
    CharData *mob, *player;
    ObjData *obj;

    if (IS_NPC(ch) ? (!ch->desc || !POSSESSOR(ch)) : !GET_SKILL(REAL_CHAR(ch), SKILL_SHAPECHANGE)) {
        send_to_char("You have no idea how to do that!\n", ch);
        return;
    }

    argument = any_one_arg(argument, arg);

    /* If already shapechanged, other rules apply. */
    if (POSSESSED(ch)) {
        if (!*arg) {
            if (ch->char_specials.timer == 0)
                sprintf(buf, "You have just recently taken the form of %s.\n", GET_NAME(ch));
            else if (ch->char_specials.timer == 1)
                sprintf(buf, "You have been in the form of %s for 1 hour.\n", GET_NAME(ch));
            else
                sprintf(buf, "You have been in the form of %s for %d hours.\n", GET_NAME(ch), ch->char_specials.timer);
            send_to_char(buf, ch);
        } else if (!is_abbrev(arg, "me"))
            send_to_char("You cannot shapechange to another animal from this form.\n", ch);
        else {
            if (POSSESSOR(ch)->desc)
                close_socket(POSSESSOR(ch)->desc);

            player = POSSESSOR(ch);

            send_to_char("You quickly morph back to your original self.\n", ch);
            act("$n&0 contorts wildly as it reforms into $N.", true, ch, 0, player, TO_ROOM);

            /* Set the player's hit/maxhit ratio to the same as the mob's. */
            /* Avoid division by zero by just setting to maximum */
            if (GET_MAX_HIT(ch) == 0)
                GET_HIT(player) = GET_MAX_HIT(player);
            else
                GET_HIT(player) = (GET_HIT(ch) * GET_MAX_HIT(player)) / GET_MAX_HIT(ch);
            if (GET_MAX_MOVE(ch) == 0)
                GET_MOVE(player) = GET_MAX_MOVE(player);
            else
                GET_MOVE(player) = (GET_MOVE(ch) * GET_MAX_MOVE(player)) / GET_MAX_MOVE(ch);
            GET_ALIGNMENT(player) = GET_ALIGNMENT(ch);

            if (GET_LEVEL(player) < LVL_IMMORT) {
                i = GET_COOLDOWN(player, CD_SHAPECHANGE) / (1 MUD_HR);
                SET_COOLDOWN(player, CD_SHAPECHANGE, MAX(1, MIN(i, 5)) MUD_HR);
            }

            player->desc = ch->desc;
            player->desc->original = nullptr;
            player->desc->character = player;
            ch->desc = nullptr;
            player->forward = nullptr;

            char_from_room(player);
            char_to_room(player, ch->in_room);
            transfer_battle(ch, player);

            /* Transfer any objects or money the mob had to the player. */
            while (ch->carrying) {
                obj = ch->carrying;
                obj_from_char(obj);
                obj_to_char(obj, player);
            }
            for (i = 0; i < NUM_WEARS; ++i)
                if (GET_EQ(ch, i))
                    obj_to_char(unequip_char(ch, i), player);
            GET_PLATINUM(player) += GET_PLATINUM(ch);
            GET_GOLD(player) += GET_GOLD(ch);
            GET_SILVER(player) += GET_SILVER(ch);
            GET_COPPER(player) += GET_COPPER(ch);

            extract_char(ch);
        }
        return;
    }

    if (GET_COOLDOWN(ch, CD_SHAPECHANGE) && GET_LEVEL(ch) < LVL_IMMORT) {
        i = GET_COOLDOWN(ch, CD_SHAPECHANGE) / (1 MUD_HR) + 1;
        if (i == 1)
            strcpy(buf1, "hour");
        else
            sprintf(buf1, "%d hours", i);
        snprintf(buf, sizeof(buf),
                 "You are still drained from your last shapechange.\n"
                 "It will be another %s before you can change again.\n",
                 buf1);
        send_to_char(buf, ch);
        return;
    }

    if (!*arg) {
        send_to_char("Shapechange to what?\n", ch);
        return;
    }

    if (!strcasecmp(arg, "me")) {
        send_to_char("You are already in your normal form.\n", ch);
        return;
    }

    /* Check alignment. */
    if (GET_LEVEL(ch) < LVL_GOD) {
        if (GET_ALIGNMENT(ch) >= 350) {
            send_to_char("Your good loyalties betray your nature, inhibiting a transformation.\n", ch);
            return;
        } else if (GET_ALIGNMENT(ch) <= -350) {
            send_to_char("Your evil loyalties betray your nature, inhibiting a transformation.\n", ch);
            return;
        }
    }

    /* Determine the desired shapechange type.  You can supply as many
     * keywords as you like. */
    i = type = class_num = 0;
    do {
        if (is_abbrev(arg, "mammal"))
            type |= MAMMAL;
        else if (is_abbrev(arg, "reptile"))
            type |= REPTILE;
        else if (is_abbrev(arg, "bird"))
            type |= BIRD;
        else if (is_abbrev(arg, "fish"))
            type |= FISH;
        else if (is_abbrev(arg, "tank"))
            class_num |= TANK;
        else if (is_abbrev(arg, "damager"))
            class_num |= DAMAGER;
        else if (is_abbrev(arg, "traveler"))
            class_num |= TRAVEL;
        else if (is_abbrev(arg, "tracker"))
            class_num |= TRACKER;
        else if (is_abbrev(arg, "aquatic"))
            type |= AQUATIC;
        else {
            for (desired_index = 0; *creatures[desired_index].name != '\n'; ++desired_index)
                if (is_abbrev(arg, creatures[desired_index].name)) {
                    type = creatures[desired_index].type & (MAMMAL | REPTILE | BIRD | AQUATIC | FISH);
                    class_num = creatures[desired_index].type & (TANK | DAMAGER | TRAVEL | TRACKER);
                    i = 1;
                    break;
                }
            if (*creatures[desired_index].name == '\n') {
                send_to_char("What kind of animal is that?\n", ch);
                return;
            }
        }
        argument = any_one_arg(argument, arg);
    } while (*argument && !i);

    if (IS_SET(type, AQUATIC | FISH) && !IS_WATER(ch->in_room) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("You won't be able to turn into that here!\n", ch);
        return;
    }

    /* Determine which mob this player can actually shapechange into. */
    for (i = 0, index = -1; *creatures[i].name != '\n'; ++i) {
        /* Skip creatures of lower level than already picked creature. */
        if (index >= 0 && creatures[i].midlevel < creatures[index].midlevel)
            continue;
        /* Skip aquatic creatures if not in water room. */
        if (IS_SET(creatures[i].type, AQUATIC) && !IS_WATER(ch->in_room))
            continue;
        /* Skip creatures of the wrong type. */
        if (type && !IS_SET(creatures[i].type, type))
            continue;
        /* This is where the dice roll occurs.  It's some complicated
         * alignment and level calculation.  I dunno. */
        if (GET_LEVEL(ch) < LVL_GOD && index >= 0 &&
            creatures[i].midlevel - 5 + number(0, abs(GET_ALIGNMENT(ch))) > GET_LEVEL(ch))
            continue;
        /* Skip creatures of the wrong class. */
        if (class_num && !IS_SET(creatures[i].type, class_num))
            continue;
        /* This is a match. */
        index = i;
        /* This creature was specifically requested.  Stop looking. */
        if (desired_index == i)
            break;
    }

    if (index < 0) {
        send_to_char("You don't know how to shapechange into that.\n", ch);
        return;
    }

    if (desired_index >= 0 && desired_index != index) {
        sprintf(buf, "You didn't feel quite up to changing into %s.\n",
                with_indefinite_article(creatures[desired_index].name));
        send_to_char(buf, ch);
    }

    /* Attempt to create the mobile. */
    if (!(mob = read_mobile(creatures[index].vnum, VIRTUAL))) {
        send_to_char("You start to change, then feel ill, and slump back to your normal form.\n", ch);
        sprintf(buf,
                "SYSERR: %s tried to shapechange into nonexistent "
                "mob prototype V#%d",
                GET_NAME(ch), creatures[index].vnum);
        mudlog(buf, BRF, LVL_GOD, true);
        return;
    }

    act("The snap of bones reforming can be heard as $n takes the shape of $N&0!", false, ch, 0, mob, TO_ROOM);
    act("You transform into $N!", false, ch, 0, mob, TO_CHAR);

    /* This must be done before ch is removed from a room, because that would
     * clear ch's battle status. */
    transfer_battle(ch, mob);

    /* Shuffle characters around. */
    char_to_room(mob, ch->in_room);
    char_from_room(ch);
    char_to_room(ch, 0);

    /* Copy some preferences from the player to the mob. */
    GET_PROMPT(mob) = strdup(GET_PROMPT(ch));

    /* Set up level based on player level and alignment. */
    GET_LEVEL(mob) = creatures[index].midlevel + MAX(MAX(-5, MIN(5, GET_LEVEL(ch) - creatures[index].midlevel)),
                                                     ((350 - abs(GET_ALIGNMENT(ch))) * 5) / 350);

    /* Set up mob's skills. First, turn off default mob skills that animals
     * shouldn't have.  Yes, it's a shame, since the mob classes set on
     * the mob prototypes won't mean much, but it's too much to manually
     * list all the skills we want to remove.  After clearing skills, turn
     * on animal skills. */
    memset(&mob->char_specials.skills, 0, sizeof(mob->char_specials.skills));
    SET_SKILL(mob, SKILL_DODGE, roll_skill(mob, SKILL_DODGE));
    SET_SKILL(mob, SKILL_DOUSE, roll_skill(mob, SKILL_DOUSE));
    for (i = 0; i < MAX_SHAPECHANGE_SKILLS; ++i)
        if (creatures[index].skills[i])
            SET_SKILL(mob, creatures[index].skills[i], roll_skill(mob, creatures[index].skills[i]));

    /* Scale hp/mv based on player's current/max ratios. */
    GET_MAX_HIT(mob) = number(creatures[index].minhp, creatures[index].maxhp);
    GET_MAX_MOVE(mob) = number(creatures[index].minmv, creatures[index].maxmv);
    if (GET_MAX_HIT(ch) == 0)
        GET_HIT(mob) = GET_MAX_HIT(mob);
    else
        GET_HIT(mob) = (GET_HIT(ch) * GET_MAX_HIT(mob)) / GET_MAX_HIT(ch);
    if (GET_MAX_MOVE(ch) == 0)
        GET_MOVE(mob) = GET_MAX_MOVE(mob);
    else
        GET_MOVE(mob) = (GET_MOVE(ch) * GET_MAX_MOVE(mob)) / GET_MAX_MOVE(ch);
    GET_ALIGNMENT(mob) = GET_ALIGNMENT(ch);
    hurt_char(mob, nullptr, 0, true);

    /* Add the player's name to the mob's namelist */
    GET_NAMELIST(mob) = strdup(fmt::format("{} {}", GET_NAMELIST(mob), GET_NAME(ch)).c_str());

    /* Set gender */
    GET_SEX(mob) = GET_SEX(ch);

    /* Move the descriptor. */
    ch->desc->character = mob;
    ch->desc->original = ch;
    mob->desc = ch->desc;
    ch->desc = nullptr;
    ch->forward = mob;
}

bool creature_allowed_skill(CharData *ch, int skill) {
    int i, j;

    if (!IS_NPC(ch) || GET_MOB_VNUM(ch) < SHAPE_VNUM_MIN || GET_MOB_VNUM(ch) > SHAPE_VNUM_MAX)
        return false;

    for (i = 0; creatures[i].vnum > 0; i++) {
        if (creatures[i].vnum == GET_MOB_VNUM(ch)) {
            for (j = 0; j < MAX_SHAPECHANGE_SKILLS; ++j)
                if (creatures[i].skills[j] == skill)
                    return true;
        }
    }

    return false;
}

#undef MAMMAL
#undef BIRD
#undef REPTILE
#undef FISH

#undef TANK
#undef DAMAGER
#undef TRAVEL
#undef TRACKER

#undef AQUATIC

#undef MAX_SHAPECHANGE_SKILLS

ACMD(do_save) {
    CharData *target = nullptr;

    /*  Player save functionality for god types. */
    /*  The following section allows for gods to save players using the */
    /*  syntax: save <playername>, where <playername> is the player to */
    /*  be saved. This works on any character which has been brought */
    /*  online either by the player logging in or a god linkloading. */
    if (GET_LEVEL(ch) >= LVL_GOD) {
        one_argument(argument, arg);

        if (!strcasecmp(arg, "all")) {
            auto_save_all();
            char_printf(ch, "You have saved all players in the realm.\n");
            mprintf(L_STAT, MAX(GET_LEVEL(ch), GET_INVIS_LEV(ch)), "(GC) %s has saved all players in the realm.",
                    GET_NAME(ch));
            return;
        } else if (*arg)
            /*  try to locate this player within the realm */
            target = find_char_around_char(ch, find_by_name(arg));
        else
            target = ch;
    } else
        target = ch;

    if (IS_NPC(ch) || !ch->desc)
        return;

    if (!target) {
        char_printf(ch, "No player by the name of %s is currently in the game.\n", arg);
        return;
    }

    if (IS_NPC(target)) {
        send_to_char("You can't save an NPC!\n", ch);
        return;
    }

    if (cmd) {
        if (ch == target)
            char_printf(ch, "Saving %s.\n", GET_NAME(ch));
        else
            char_printf(ch, "You have force-saved %s.\n", GET_NAME(target));
    }
    save_player(target);

    if (ch != target)
        mprintf(L_STAT, GET_LEVEL(ch), "(GC) %s has saved %s to file.", GET_NAME(ch), GET_NAME(target));
}

/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here) {
    if (CMD_IS("balance") || CMD_IS("deposit") || CMD_IS("withdraw") || CMD_IS("dump") || CMD_IS("exchange"))
        send_to_char("Sorry, you can only do that in a bank!\n", ch);
    else if (CMD_IS("appear") || CMD_IS("disappear"))
        send_to_char(HUH, ch);
    else if (CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive"))
        send_to_char("Sorry, you can only do that in a post office!\n", ch);
    else if (CMD_IS("list") || CMD_IS("value") || CMD_IS("buy") || CMD_IS("sell"))
        send_to_char("Sorry, you can only do that in a shop!\n", ch);
    else if (CMD_IS("rent"))
        send_to_char("Sorry, you can only do that in an inn!\n", ch);
    else
        send_to_char("Sorry, but you cannot do that here!\n", ch);
}

ACMD(do_camp) {
    CampEvent *ce;

    if (FIGHTING(ch) || EVENT_FLAGGED(ch, EVENT_CAMP)) {
        send_to_char("You are too busy to do this!\n", ch);
        return;
    }

    if (IS_NPC(ch) || !ch->desc) {
        send_to_char("You can't camp while shapechanged!\n", ch);
        return;
    }

    /* Restrictions: can't camp inside, in a city, or in water. */
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (CH_INDOORS(ch)) {
            send_to_char("&7You always pitch a tent indoors?&0\n", ch);
            return;
        }

        if (SECT(ch->in_room) == SECT_CITY) {
            send_to_char("&7Ye can't pitch a tent on the sidewalk fool.&0\n", ch);
            return;
        }

        if ((SECT(ch->in_room) == SECT_SHALLOWS) || (SECT(ch->in_room) == SECT_WATER) ||
            (SECT(ch->in_room) == SECT_UNDERWATER)) {
            send_to_char("&7Go buy a floating tent and try again.&0\n", ch);
            return;
            if (SECT(ch->in_room) == SECT_AIR) {
                send_to_char("&7You can't camp in mid-air.&0\n", ch);
                return;
            }
        }
        if (RIDING(ch)) {
            send_to_char("You'd better dismount first.\n", ch);
            return;
        }
    }

    if (GET_STANCE(ch) == STANCE_FIGHTING)
        send_to_char("No way!  You're fighting for your life!\n", ch);
    else if (GET_STANCE(ch) < STANCE_STUNNED)
        send_to_char("It's hard to set your tent up while dying...\n", ch);
    else {
        /* create and initialize the camp event */
        CREATE(ce, CampEvent, 1);
        ce->ch = ch;
        ce->was_in = ch->in_room;
        event_create(EVENT_CAMP, camp_event, ce, true, &(ch->events), GET_LEVEL(ch) >= LVL_IMMORT ? 5 : 350);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_CAMP);
        act("You start setting up camp.", false, ch, nullptr, nullptr, TO_CHAR);
        act("$n starts setting up camp.", true, ch, 0, 0, TO_ROOM);
    }
}

EVENTFUNC(recall_event) {
    RecallEventObj *re = (RecallEventObj *)event_obj;
    CharData *ch;
    bool wasdark;

    ch = re->ch;

    if (ch->in_room != re->from_room) {
        send_to_room("The magic of the scroll fizzles as its target has left.\n", re->from_room);
        send_to_char("The magic of the scroll fizzles, as you left the area.\n", ch);
        return EVENT_FINISHED;
    };

    if (IS_NPC(ch) || !ch->desc)
        return EVENT_FINISHED;

    send_to_char("You feel the scroll's energy start to envelop you.\n", ch);
    act("$N disappears in a bright flash.\n", false, ch, 0, ch, TO_NOTVICT);
    wasdark = IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch);

    dismount_char(ch);
    char_from_room(ch);
    char_to_room(ch, re->room);
    act("$N appears in a bright flash of light.\n", false, ch, 0, ch, TO_NOTVICT);

    check_new_surroundings(ch, wasdark, true);

    return EVENT_FINISHED;
}

EVENTFUNC(camp_event) {
    CampEvent *ce = (CampEvent *)event_obj;
    CharData *ch = nullptr;
    int was_in, now_in;

    /* extract all the info from ce */
    ch = ce->ch;
    was_in = ce->was_in;
    now_in = ch->in_room;

    if (IS_NPC(ch) || !ch->desc) {
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_CAMP);
        return EVENT_FINISHED;
    }

    if (RIDING(ch)) {
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_CAMP);
        send_to_char("You can't camp while mounted!\n", ch);
        return EVENT_FINISHED;
    }

    if (FIGHTING(ch)) {
        act("You decide now is not the best time for camping.", false, ch, nullptr, nullptr, TO_CHAR);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_CAMP);
        return EVENT_FINISHED;
    }

    if (now_in != was_in) {
        act("You are no longer near where you began the campsite.", false, ch, nullptr, nullptr, TO_CHAR);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_CAMP);
        return EVENT_FINISHED;
    }

    /* Yeah, let's not try to update characters who are about to be free'd, eh */
    rem_memming(ch);

    /* So players don't get saved with the meditate flag and cause syserrs
       when they log back on. */
    if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
        act("$N ceases $s meditative trance.", true, ch, 0, 0, TO_ROOM);
        send_to_char("&8You stop meditating.\n&0", ch);
        REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
    }

    act("You complete your campsite, and leave this world for a while.", false, ch, nullptr, nullptr, TO_CHAR);
    if (!GET_INVIS_LEV(ch))
        act("$n rolls up $s bedroll and tunes out the world.", true, ch, 0, 0, TO_ROOM);

    sprintf(buf, "%s has camped in %s (%d).", GET_NAME(ch), world[ch->in_room].name, world[ch->in_room].vnum);

    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), true);
    REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_CAMP);
    remove_player_from_game(ch, QUIT_CAMP);
    return EVENT_FINISHED;
}

ACMD(do_unbind) {
    int prob, percent;
    CharData *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg) {
        if (!PLR_FLAGGED(ch, PLR_BOUND)) {
            send_to_char("You are free as a bird!\n", ch);
            return;
        }
        prob = number(1, 70);
        percent = number(20, 101);
        if (prob > percent) {
            send_to_char("You break free from your binds!\n", ch);
            act("$n breaks free from his binds", false, ch, 0, 0, TO_ROOM);
            REMOVE_FLAG(PLR_FLAGS(ch), PLR_BOUND);
            WAIT_STATE(ch, PULSE_VIOLENCE);
            return;
        } else
            WAIT_STATE(ch, PULSE_VIOLENCE * 3);
        return;
    } else {
        if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
            send_to_char("Unbind who?\n", ch);
            return;
        }
        if (vict == ch) {
            prob = number(20, 70);
            percent = number(1, 101);
            if (prob > percent) {
                send_to_char("You break free from your binds!\n", ch);
                act("$n breaks free from his binds", false, ch, 0, 0, TO_ROOM);
                REMOVE_FLAG(PLR_FLAGS(ch), PLR_BOUND);
                WAIT_STATE(ch, PULSE_VIOLENCE);
                return;
            } else
                WAIT_STATE(ch, PULSE_VIOLENCE * 3);
            return;
        }
        REMOVE_FLAG(PLR_FLAGS(vict), PLR_BOUND);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        send_to_char("You are free of your binds.\n", vict);
    }
}

ACMD(do_bind) {
    CharData *vict;
    ObjData *held = GET_EQ(ch, WEAR_HOLD);
    int prob, percent;

    /* disable this command it's broken and is being used to
       ruin the game for players RSD 2/11/2001 */
    send_to_char("Huh?!?\n", ch);
    return;

    if (FIGHTING(ch)) {
        send_to_char("You are too busy fighting to think about that right now!\n", ch);
        return;
    }

    one_argument(argument, arg);

    if (!*arg)
        send_to_char("Bind who?\n", ch);
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))))
        send_to_char("There is no such player.\n", ch);
    else if (IS_NPC(vict))
        send_to_char("You can't do that to a mob!\n", ch);
    else if (GET_LEVEL(vict) > LVL_GOD)
        send_to_char("Hmmm...you'd better not.\n", ch);
    else if (!held)
        send_to_char("You must be holding a rope to tie someone up!\n", ch);
    else {

        if (ch == vict) {
            send_to_char("Oh, yeah, THAT'S real smart...\n", ch);
            return;
        }
        if (PLR_FLAGGED(vict, PLR_BOUND)) {
            send_to_char("Your victim is already tied up.\n", ch);
            return;
        }
        if (GET_OBJ_TYPE(held) != ITEM_ROPE) {
            send_to_char("You must be holding a rope to tie someone up!\n", ch);
            return;
        }
        if (GET_SKILL(ch, SKILL_BIND) == 0) {
            if (GET_STANCE(vict) > STANCE_STUNNED) {
                send_to_char("You aren't skilled enough to tie a conscious person\n", ch);
                return;
            } else {
                act("You tie $N up.... What next?", false, ch, 0, vict, TO_CHAR);
                act("$n ties you up.... Hope he isnt the kinky type", false, ch, 0, vict, TO_VICT);
                act("$n ties up $N.", false, ch, 0, vict, TO_NOTVICT);
                SET_FLAG(PLR_FLAGS(vict), PLR_BOUND);
                extract_obj(held);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                return;
            }
        } else {
            prob = number(1, 50);
            prob += GET_SKILL(ch, SKILL_BIND);
            prob += GET_LEVEL(ch);
            percent = number(50, 200);
            percent += dex_app[GET_DEX(vict)].defensive;
            percent += GET_LEVEL(vict);

            if (GET_STANCE(vict) < STANCE_SLEEPING)
                prob = percent + 1;

            if (prob > percent) {
                act("You tie $N up.... What next?", false, ch, 0, vict, TO_CHAR);
                act("$n ties you up.... Hope he isnt the kinky type", false, ch, 0, vict, TO_VICT);
                act("$n ties up $N.", false, ch, 0, vict, TO_NOTVICT);
                SET_FLAG(PLR_FLAGS(vict), PLR_BOUND);
                extract_obj(held);
                improve_skill(ch, SKILL_BIND);
                WAIT_STATE(ch, PULSE_VIOLENCE * 3);
                return;
            } else {
                act("You tries to tie $N up.... What next?", false, ch, 0, vict, TO_CHAR);
                act("$n tries to tie you up.... Hope he isnt the kinky type", false, ch, 0, vict, TO_VICT);
                act("$n tries to tie up $N.", false, ch, 0, vict, TO_ROOM);
                improve_skill(ch, SKILL_BIND);
                WAIT_STATE(ch, PULSE_VIOLENCE * 3);
                return;
            }
        }
    }
}

ACMD(do_abort) {
    void abort_casting(CharData * ch);
    void flush_queues(DescriptorData * d);

    if (CASTING(ch)) {
        send_to_char("&8You abort your spell!&0\n", ch);
        abort_casting(ch);
        if (ch->desc)
            flush_queues(ch->desc);
    } else
        send_to_char("You're not even casting a spell right now.\n", ch);
}

ACMD(do_hide) {
    long lower_bound, upper_bound;
    int skill;

    if (!GET_SKILL(ch, SKILL_HIDE)) {
        send_to_char("You'd better leave that art to the rogues.\n", ch);
        return;
    }

    if (RIDING(ch)) {
        send_to_char("While mounted? I don't think so...\n", ch);
        return;
    }

    if (IS_HIDDEN(ch))
        send_to_char("You try to find a better hiding spot.\n", ch);
    else
        send_to_char("You attempt to hide yourself.\n", ch);

    skill = GET_SKILL(ch, SKILL_HIDE);
    lower_bound = -0.0008 * pow(skill, 3) + 0.1668 * pow(skill, 2) - 3.225 * skill;
    upper_bound = skill * (3 * GET_DEX(ch) + GET_INT(ch)) / 40;
    GET_HIDDENNESS(ch) = number(lower_bound, upper_bound) + dex_app_skill[GET_DEX(ch)].hide;

    GET_HIDDENNESS(ch) = MAX(GET_HIDDENNESS(ch), 0);

    WAIT_STATE(ch, PULSE_VIOLENCE);

    improve_skill(ch, SKILL_HIDE);

    REMOVE_FLAG(EFF_FLAGS(ch), EFF_STEALTH);
    if (GET_SKILL(ch, SKILL_STEALTH)) {
        if (GET_HIDDENNESS(ch) && GET_SKILL(ch, SKILL_STEALTH) > number(0, 101))
            SET_FLAG(EFF_FLAGS(ch), EFF_STEALTH);
        improve_skill(ch, SKILL_STEALTH);
    }
}

ACMD(do_steal) {
    CharData *vict;
    ObjData *obj;
    char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
    int percent, eq_pos, caught = 0, coins[NUM_COIN_TYPES];

    ACMD(do_gen_comm);

    if (FIGHTING(ch)) {
        send_to_char("You can't steal while you are fighting!\n", ch);
        return;
    }
    if (GET_SKILL(ch, SKILL_STEAL) <= 0) {
        send_to_char("You don't know how to steal!\n", ch);
        return;
    }
    if (MOB_FLAGGED(ch, MOB_ILLUSORY)) {
        send_to_char("Being an illusion, you can't steal things.\n", ch);
        return;
    }
    if (!RIGID(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("You can't handle objects in your condition.\n", ch);
        return;
    }

    argument = one_argument(argument, obj_name);
    one_argument(argument, vict_name);

    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, vict_name)))) {
        send_to_char("Steal what from who?\n", ch);
        return;
    } else if (vict == ch) {
        send_to_char("Come on now, that's rather stupid!\n", ch);
        return;
    }

    /* Player-stealing is only allowed during PK. */
    if (!attack_ok(ch, vict, false)) {
        send_to_char("You can't steal from them!\n", ch);
        return;
    }
    if (ROOM_FLAGGED(vict->in_room, ROOM_ARENA)) {
        send_to_char("You can't steal in the arena!\n", ch);
        return;
    }

    /* 101% is a complete failure */
    percent = number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;
    if (!CAN_SEE(vict, ch))
        percent -= dex_app_skill[GET_DEX(ch)].p_pocket;

    /* Stealing from unconscious folks is always successful. */
    if (!AWAKE(vict))
        percent = -1;

    /* ... except that you cannot steal from immortals, shopkeepers, or those who are aware. */
    if (GET_LEVEL(vict) >= LVL_IMMORT || GET_MOB_SPEC(vict) == shop_keeper || MOB_FLAGGED(vict, MOB_AWARE))
        percent = 101 + 50;

    /* First check whether the thief wants to steal money */
    if (strcasecmp(obj_name, "coins") && strcasecmp(obj_name, "gold")) {

        /* If not money, look for an item in the victim's inventory */
        if (!(obj = find_obj_in_list(vict->carrying, find_vis_by_name(ch, obj_name)))) {

            /* If not an item in inventory, look for an equipped item */
            for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
                if (GET_EQ(vict, eq_pos) && (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
                    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
                    obj = GET_EQ(vict, eq_pos);
                    break;
                }

            /* Thief is attempting to steal an equipped item. */

            if (!obj) {
                act("$E hasn't got that item.", false, ch, 0, vict, TO_CHAR);
                return;
            } else {
                /* You cannot steal equipped items unless the victim is knocked out. */
                if (GET_STANCE(vict) > STANCE_STUNNED) {
                    send_to_char("Steal the equipment now?  Impossible!\n", ch);
                    return;
                } else if (GET_OBJ_LEVEL(obj) > GET_LEVEL(ch)) {
                    act("$p is too powerful for you to steal.", false, ch, obj, 0, TO_CHAR);
                    return;
                } else {
                    /* You stole an equipped item from a helpless mob. */
                    act("You unequip $p and steal it.", false, ch, obj, 0, TO_CHAR);
                    act("$n steals $p from $N.", false, ch, obj, vict, TO_NOTVICT);
                    obj_to_char(unequip_char(vict, eq_pos), ch);
                }
            }
        } else {
            /* Steal an item from inventory */
            percent += GET_OBJ_EFFECTIVE_WEIGHT(obj); /* Make heavy harder */
            if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
                /* You failed. */
                caught = true;
                act("Oops...", false, ch, 0, 0, TO_CHAR);
                act("$n tried to steal something from you!", false, ch, 0, vict, TO_VICT);
                act("$n tries to steal something from $N.", true, ch, 0, vict, TO_NOTVICT);
            } else if (GET_OBJ_LEVEL(obj) > GET_LEVEL(ch)) {
                act("$p is too powerful for you to steal.", false, ch, obj, 0, TO_CHAR);
                return;
            } else {
                /* You succeeded. */
                if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
                    if (ADDED_WEIGHT_OK(ch, obj)) {
                        obj_from_char(obj);
                        obj_to_char(obj, ch);
                        send_to_char("Got it!\n", ch);
                        if (AWAKE(vict))
                            improve_skill(ch, SKILL_STEAL);
                        get_check_money(ch, obj);
                    }
                } else
                    send_to_char("You cannot carry that much.\n", ch);
            }
        }
    } else {
        /* Steal some coins */
        if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
            /* Failed attempt to steal some coins */
            caught = true;
            act("Oops..", false, ch, 0, 0, TO_CHAR);
            act("You discover that $n has $s hands in your wallet.", false, ch, 0, vict, TO_VICT);
            act("$n tries to steal coins from $N.", true, ch, 0, vict, TO_NOTVICT);
            improve_skill(ch, SKILL_STEAL);
        } else {
            /* Successful theft of coins */
            coins[PLATINUM] = (GET_PLATINUM(vict) * number(1, 10)) / 100;
            coins[GOLD] = (GET_GOLD(vict) * number(1, 10)) / 100;
            coins[SILVER] = (GET_SILVER(vict) * number(1, 10)) / 100;
            coins[COPPER] = (GET_COPPER(vict) * number(1, 10)) / 100;

            if (CASH_VALUE(coins) > 0) {
                GET_COPPER(ch) += coins[COPPER];
                GET_COPPER(vict) -= coins[COPPER];
                GET_SILVER(ch) += coins[SILVER];
                GET_SILVER(vict) -= coins[SILVER];
                GET_GOLD(ch) += coins[GOLD];
                GET_GOLD(vict) -= coins[GOLD];
                GET_PLATINUM(ch) += coins[PLATINUM];
                GET_PLATINUM(vict) -= coins[PLATINUM];
                statemoney(buf, coins);
                char_printf(ch, "Woohoo! You stole %s.\n", buf);
            } else {
                send_to_char("You couldn't get any coins...\n", ch);
            }
            if (AWAKE(vict))
                improve_skill(ch, SKILL_STEAL);
        }
    }

    if (caught && IS_NPC(vict) && AWAKE(vict))
        attack(vict, ch);
    WAIT_STATE(ch, PULSE_VIOLENCE / 2);
}

ACMD(do_level) {
    extern char *exp_message(CharData * ch);
    extern ACMD(do_experience);

    one_argument(argument, arg);

    if (!*arg)
        do_experience(ch, argument, 0, 0);
    else if (!strcasecmp(arg, "gain"))
        send_to_char("You can only do that in your guild.\n", ch);
    else
        send_to_char("Huh?!?\n", ch);
}

ACMD(do_visible) {
    void perform_immort_vis(CharData * ch);

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        perform_immort_vis(ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_INVISIBLE) || IS_HIDDEN(ch) || EFF_FLAGGED(ch, EFF_CAMOUFLAGED)) {
        appear(ch);
    } else {
        send_to_char("You are already visible.\n", ch);
    }
}

void set_title(CharData *ch, char *title);
ACMD(do_title) {
    int titles, which;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (IS_NPC(ch))
        send_to_char("Your title is fine... go away.\n", ch);
    else if (PLR_FLAGGED(ch, PLR_NOTITLE))
        send_to_char("You can't title yourself -- you shouldn't have abused it!\n", ch);
    else if (GET_LEVEL(ch) >= LVL_IMMORT) {
        if (strlen(argument) > MAX_TITLE_LENGTH)
            char_printf(ch, "Sorry, titles can't be longer than %d characters.\n", MAX_TITLE_LENGTH);
        else {
            set_title(ch, argument);
            char_printf(ch, "Okay, you're now %s %s.\n", GET_NAME(ch), GET_TITLE(ch));
            save_player_char(ch);
        }
    } else if (!*argument) {
        titles = 0;
        if (GET_PERM_TITLES(ch))
            while (GET_PERM_TITLES(ch)[titles])
                ++titles;
        if (GET_CLAN(ch) && IS_CLAN_MEMBER(ch))
            ++titles;
        if (titles == 0) {
            send_to_char("You haven't earned any permanent titles!\n", ch);
            if (ch->player.title && *ch->player.title)
                send_to_char("Use 'title 0' to clear your current title.\n", ch);
        } else {
            titles = 0;
            char_printf(ch,
                        "You have earned the following titles:\n"
                        "  0) <no title>\n");
            if (GET_PERM_TITLES(ch))
                while (GET_PERM_TITLES(ch)[titles]) {
                    char_printf(ch, "  %d) %s\n", titles + 1, GET_PERM_TITLES(ch)[titles]);
                    ++titles;
                }
            if (GET_CLAN(ch) && IS_CLAN_MEMBER(ch))
                char_printf(ch, "  %d) %s %s\n", ++titles, GET_CLAN_TITLE(ch), GET_CLAN(ch)->abbreviation);
            char_printf(ch, "Use 'title <number>' to switch your title.\n");
        }
    } else if (!is_positive_integer(argument))
        send_to_char(
            "Usage: title\n"
            "       title <number>\n",
            ch);
    else {
        int i;
        which = atoi(argument);
        titles = 0;
        if (which == 0)
            set_title(ch, nullptr);
        if (GET_PERM_TITLES(ch))
            for (i = 0; GET_PERM_TITLES(ch)[i]; ++i)
                if (++titles == which) {
                    set_title(ch, GET_PERM_TITLES(ch)[i]);
                    break;
                }
        if (GET_CLAN(ch) && IS_CLAN_MEMBER(ch)) {
            if (++titles == which)
                clan_set_title(ch);
        }
        if (which > titles) {
            send_to_char("You don't have that many titles!\n", ch);
            return;
        }
        if (GET_TITLE(ch)) {
            sprintf(buf, "Okay, set your title to: %s\n", GET_TITLE(ch));
            send_to_char(buf, ch);
        } else
            send_to_char("Okay, cleared your title.\n", ch);
    }
}

ACMD(do_douse) {
    bool success = false;
    CharData *vict;
    ObjData *obj;

    if (!ch)
        return;

    if (argument && *argument) {
        one_argument(argument, arg);
        vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg));
    } else
        vict = ch;

    if (!vict) {
        send_to_char("You don't see that person here.\n", ch);
        return;
    } else if (!EFF_FLAGGED(vict, EFF_ON_FIRE)) {
        send_to_char("Where's the fire?\n", ch);
        return;
    }

    /* A fountain in the room guarantees success. */
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
        if (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) {
            success = true;
            break;
        }

    /* There is a fountain here */
    if (success) {

        /* Dousing yourself with a fountain */
        if (ch == vict) {
            act("*SPLASH* $n leaps into $o, putting out the flames that were "
                "consuming $m.",
                false, ch, obj, 0, TO_ROOM);
            act("*SPLASH* You leap into $o, dousing your flames!", false, ch, obj, 0, TO_CHAR);

            /* Dousing someone else with a fountain */
        } else {
            act("You dunk $N into $o, putting $M out! *SPLASH* *GURGLE*", false, ch, obj, vict, TO_CHAR);
            act("$n dunks you into $o, putting your flames out! *GURGLE*", false, ch, obj, vict, TO_VICT);
            act("$n dunks $N into $o, dousing $S flames! *SPLASH* *GURGLE*", false, ch, obj, vict, TO_NOTVICT);
        }
    }

    /* No fountain */

    /* Water room? */

    else if (IS_WATER(IN_ROOM(vict))) {
        if (ch == vict) {
            act("$n ducks under the surface of the water, putting out $s flames.", false, ch, 0, 0, TO_ROOM);
            act("You duck under the surface of the water, dousing your flames.", false, ch, obj, 0, TO_CHAR);
        } else {
            act("You push $N under the water, putting $M out! *SPLASH* *GURGLE*", false, ch, 0, vict, TO_CHAR);
            act("$n pushes you under the water, putting your flames out! *GURGLE*", false, ch, 0, vict, TO_VICT);
            act("$n pushes $N under the water, dousing $S flames! *SPLASH* *GURGLE*", false, ch, 0, vict, TO_NOTVICT);
        }
        success = true;
    }

    /* Splashy room?  E.g., swamp, beach */

    else if (IS_SPLASHY(IN_ROOM(vict)) || SECT(vict->in_room) == SECT_BEACH) {
        if (ch == vict) {
            act("$n rolls around in the water, quickly putting out $s flames.", false, ch, 0, 0, TO_ROOM);
            act("You roll around in the water, quickly dousing your flames.", false, ch, obj, 0, TO_CHAR);
        } else {
            act("You push $N down into the shallow water, putting $M out! *SPLASH* "
                "*GURGLE*",
                false, ch, 0, vict, TO_CHAR);
            act("$n pushes you into the shallow water, putting your flames out! "
                "*GURGLE*",
                false, ch, 0, vict, TO_VICT);
            act("$n pushes $N into the shallow water, dousing $S flames! *SPLASH* "
                "*GURGLE*",
                false, ch, 0, vict, TO_NOTVICT);
        }
        success = true;
    }

    /* No water available! */

    /* No water, trying to douse yourself */
    else if (ch == vict) {
        if (GET_SKILL(ch, SKILL_DOUSE) < number(0, 100)) {
            act("$n&0 frantically rolls around on the ground, attempting to douse "
                "the flames consuming $s body.",
                true, ch, 0, 0, TO_ROOM);
            send_to_char("You roll around on the ground, trying to douse the flames engulfing your body!\n", ch);
        } else {
            act("$n&0 rolls on the ground frantically, finally smothering the fire that was consuming $m.", true, ch, 0,
                0, TO_ROOM);
            send_to_char("You roll around on the ground, finally smothering your flames.\n", ch);
            success = true;
        }
    }

    /* No water, trying to douse someone else */
    else if (GET_SKILL(ch, SKILL_DOUSE) - 40 < number(0, 100)) {
        act("You frantically try to brush the flames from $N&0.", false, ch, 0, vict, TO_CHAR);
        act("$n&0 aids you, attempting to douse your flames.", false, ch, 0, vict, TO_VICT);
        act("$n&0 frantically attempts to brush the flames off $N&0.", false, ch, 0, vict, TO_NOTVICT);
    } else {
        act("You frantically brush the flames from $N&0, finally extinguishing $M!", true, ch, 0, vict, TO_CHAR);
        act("$n&0 aids you, finally putting your flames out!", false, ch, 0, vict, TO_VICT);
        act("$n&0 finally douses the flames that were consuming $N&0!", false, ch, 0, vict, TO_NOTVICT);
        success = true;
    }

    if (success)
        REMOVE_FLAG(EFF_FLAGS(vict), EFF_ON_FIRE);
    improve_skill(ch, SKILL_DOUSE);
    WAIT_STATE(ch, PULSE_VIOLENCE);
}

ACMD(do_disband) {
    if (!ch->groupees) {
        send_to_char("&0&7&bBut you lead no group!&0\n", ch);
        return;
    }

    disband_group(ch, true, false);
}

ACMD(do_consent) {
    CharData *target;

    one_argument(argument, arg);

    if (!*arg) {
        if (!CONSENT(ch))
            send_to_char("You are not consented to anyone!\n", ch);
        else
            act("You are consented to $N.", true, ch, 0, CONSENT(ch), TO_CHAR | TO_SLEEP);
        return;
    }

    if (!strcasecmp(arg, "off"))
        target = ch; /* consent self to turn it off */
    else if (!(target = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
        send_to_char(NOPERSON, ch);
        return;
    }

    if (target == ch) {
        if (CONSENT(ch)) {
            act("&8$n&0&8 has revoked $s consent.&0", false, ch, 0, CONSENT(ch), TO_VICT | TO_SLEEP);
            send_to_char("&8You revoke your consent.&0\n", ch);
            CONSENT(ch) = nullptr;
        } else
            send_to_char("You haven't given your consent to anyone.\n", ch);
        return;
    }

    if (CONSENT(ch) == target) {
        act("$N already has your consent.", false, ch, 0, target, TO_CHAR | TO_SLEEP);
        return;
    }

    if (!speech_ok(ch, 1)) {
        send_to_char("Your sore throat somehow prevents you from doing this.\n", ch);
        return;
    }

    if (CONSENT(ch))
        act("&8$n&0&8 has removed $s consent.&0", false, ch, 0, CONSENT(ch), TO_VICT | TO_SLEEP);
    CONSENT(ch) = target;
    act("&7&bYou give your consent to $N.&0", false, ch, 0, target, TO_CHAR | TO_SLEEP);
    act("&7&b$n has given you $s consent.&0", false, ch, 0, target, TO_VICT | TO_SLEEP);
}

ACMD(do_bandage) {
    CharData *victim;

    one_argument(argument, arg);

    /* If no arg, bandage the first person in the room who needs it. */
    if (!*arg) {
        for (victim = world[ch->in_room].people; victim; victim = victim->next_in_room)
            if (CAN_SEE(ch, victim))
                if (GET_HIT(victim) < 0 || GET_STANCE(victim) < STANCE_STUNNED)
                    break;
        if (!victim) {
            send_to_char("Nobody here looks like they need bandaging!\n", ch);
            return;
        }
    } else if (!(victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        send_to_char(NOPERSON, ch);
        return;
    }
    if (GET_HIT(victim) >= 0 && GET_STANCE(victim) >= STANCE_STUNNED) {
        act("&8$N looks in pretty good shape already!&0", false, ch, 0, victim, TO_CHAR);
        return;
    }

    if (GET_SKILL(ch, SKILL_BANDAGE) > number(1, 80)) {
        act("&0&8You bandage $N.&0", false, ch, 0, victim, TO_CHAR);
        act("&8$n&0&8 bandages $N&8's wounds.&0", false, ch, 0, victim, TO_NOTVICT);
        hurt_char(victim, nullptr, MAX(-3, GET_SKILL(ch, SKILL_BANDAGE) / -10), true);
    } else {
        act("You fail to bandage $N properly.", false, ch, 0, victim, TO_CHAR);
        act("&8$n fails an attempt to bandage $N&8's wounds.&0", false, ch, 0, victim, TO_NOTVICT);
        if (DAMAGE_WILL_KILL(victim, 1)) {
            act("Your bandaging was so appalling that $N died!&0", false, ch, 0, victim, TO_CHAR);
            act("&8$n kills $N with some dismal bandaging.&0", false, ch, 0, victim, TO_NOTVICT);
        }
        hurt_char(victim, nullptr, 1, true);
    }

    improve_skill(ch, SKILL_BANDAGE);
    if (GET_LEVEL(ch) < LVL_IMMORT)
        WAIT_STATE(ch, PULSE_VIOLENCE);
}

void make_group_report_line(CharData *ch, char *buffer) {
    int perc;
    char harm_color[20];

    perc = (100 * GET_HIT(ch) / GET_MAX_HIT(ch));

    strcpy(harm_color, perc >= 100  ? CLR(ch, ANRM)
                       : perc >= 88 ? CLR(ch, AFYEL)
                       : perc >= 70 ? CLR(ch, AHYEL)
                       : perc >= 45 ? CLR(ch, AFMAG)
                       : perc >= 20 ? CLR(ch, AFRED)
                       : perc >= 0  ? CLR(ch, AFRED)
                                    : CLR(ch, AFRED));

    sprintf(buffer, "%s%-15s &0[", harm_color, GET_NAME(ch));
    if (GET_HIT(ch) < 10)
        strcat(buffer, "   ");
    else if (GET_HIT(ch) < 100)
        strcat(buffer, "  ");
    else if (GET_HIT(ch) < 1000)
        strcat(buffer, " ");

    sprintf(buf2, "%s%d&0&8h&0/", harm_color, GET_HIT(ch));
    strcat(buffer, buf2);
    if (GET_MAX_HIT(ch) < 10)
        strcat(buffer, "   ");
    else if (GET_MAX_HIT(ch) < 100)
        strcat(buffer, "  ");
    else if (GET_MAX_HIT(ch) < 1000)
        strcat(buffer, " ");

    sprintf(buf2, "%s%d&0&8H&0  %3dv/%3dV] [%s]", CLR(ch, AUND), GET_MAX_HIT(ch), GET_MOVE(ch), GET_MAX_MOVE(ch),
            CLASS_ABBR(ch));
    strcat(buffer, buf2);
}

void print_group(CharData *ch) {
    CharData *k;
    GroupType *f;

    if (!ch->group_master && !ch->groupees)
        send_to_char("&2&8But you are not the member of a group!&0\n", ch);
    else {
        sprintf(buf, "%sYour group consists of:&0\n", CLR(ch, AUND));
        send_to_char(buf, ch);

        k = (ch->group_master ? ch->group_master : ch);
        if (CAN_SEE(ch, k)) {
            make_group_report_line(k, buf);
            strcat(buf, " (&0&2&bHead of group&0)\n");
            send_to_char(buf, ch);
        }

        for (f = k->groupees; f; f = f->next) {
            if (!CAN_SEE(ch, f->groupee))
                continue;

            make_group_report_line(f->groupee, buf);
            strcat(buf, "\n");
            send_to_char(buf, ch);
        }
    }
}

ACMD(do_group) {
    CharData *tch;
    int level_diff;
    bool group_all = false;

    one_argument(argument, arg);

    /* No argument, just asking for group info. */
    if (!*arg) {
        print_group(ch);
        return;
    }

    /* Only info is allowed while sleeping, not actual grouping. */
    if (!AWAKE(ch)) {
        send_to_char("In your dreams, or what?\n", ch);
        return;
    }

    if (!strcasecmp("all", arg)) {
        group_all = true;
        tch = nullptr;
    } else if (!(tch = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
        send_to_char(NOPERSON, ch);
        return;
    }

    /* Attempt to remove yourself from your group. */
    if (ch == tch) {
        if (ch->group_master || ch->groupees)
            ungroup(ch, true, false);
        else
            send_to_char("&2&8You're not in a group!&0\n", ch);
        return;
    }

    /* You can't enroll someone if you're in a group and not the leader. */
    if (ch->group_master) {
        send_to_char("&2&8You cannot enroll group members without being head of a group.&0\n", ch);
        return;
    }

    if (group_all) {
        /* group all followers */
        bool found = false;
        DescriptorData *d;
        CharData *gch;

        for (d = descriptor_list; d; d = d->next) {
            /* Check various reasons we should skip this player... */
            if (!IS_PLAYING(d)) {
                continue;
            }
            if (d->original) {
                gch = d->original;
            } else if (!(gch = d->character)) {
                continue;
            }
            if (!CAN_SEE(ch, gch)) {
                continue;
            }
            if (IS_GROUPED(gch)) {
                continue;
            }

            if (CONSENT(gch) != ch) {
                if (!(IS_NPC(gch) && gch->master == ch && EFF_FLAGGED(gch, EFF_CHARM))) {
                    continue;
                }
            }

            level_diff = GET_LEVEL(ch) - GET_LEVEL(gch);

            if (max_group_difference && (level_diff > max_group_difference || level_diff < -max_group_difference)) {
                sprintf(buf,
                        "&2&8You cannot group %s, because the level difference is too "
                        "great.\n"
                        "(The maximum allowable difference is currently %d.)&0\n",
                        GET_NAME(gch), max_group_difference);
                send_to_char(buf, ch);
                continue;
            }

            add_groupee(ch, gch);
            found = true;
        }

        if (!found)
            send_to_char("&2&8No one has consented you that is not already in a group.&0\n", ch);
        return;
    }

    if (tch->group_master && tch->group_master != ch) {
        send_to_char("&2&8That person is already in a group.&0\n", ch);
        return;
    }

    if (tch->groupees) {
        send_to_char("&2&8That person is leading a group.&0\n", ch);
        return;
    }

    /* Ok, if the target is in your group, remove them. */
    if (tch->group_master == ch) {
        ungroup(tch, true, true);
        return;
    }

    /* Check for consent unless it's a charmed follower */
    if (CONSENT(tch) != ch && GET_LEVEL(ch) < LVL_IMMORT) {
        if (!(IS_NPC(tch) && tch->master == ch && EFF_FLAGGED(tch, EFF_CHARM))) {
            act("&2&8You do not have $S consent.&0", true, ch, nullptr, tch, TO_CHAR);
            return;
        }
    }

    level_diff = GET_LEVEL(ch) - GET_LEVEL(tch);

    if (max_group_difference && (level_diff > max_group_difference || level_diff < -max_group_difference)) {
        sprintf(buf,
                "&2&8You cannot group %s, because the level difference is too "
                "great.\n"
                "(The maximum allowable difference is currently %d.)&0\n",
                GET_NAME(tch), max_group_difference);
        send_to_char(buf, ch);
        return;
    }

    add_groupee(ch, tch);
}

static void split_share(CharData *giver, CharData *receiver, int coins[]) {
    if (coins[PLATINUM] || coins[GOLD] || coins[SILVER] || coins[COPPER]) {
        statemoney(buf, coins);
        char_printf(receiver, "You %s %s.\n", giver == receiver ? "keep" : "receive", buf);
    } else
        send_to_char("You forego your share.\n", receiver);
    GET_PLATINUM(receiver) += coins[PLATINUM];
    GET_GOLD(receiver) += coins[GOLD];
    GET_SILVER(receiver) += coins[SILVER];
    GET_COPPER(receiver) += coins[COPPER];
    GET_PLATINUM(giver) -= coins[PLATINUM];
    GET_GOLD(giver) -= coins[GOLD];
    GET_SILVER(giver) -= coins[SILVER];
    GET_COPPER(giver) -= coins[COPPER];
}

void split_coins(CharData *ch, int coins[], unsigned int mode) {
    int i, j, count, share[NUM_COIN_TYPES], remainder_start[NUM_COIN_TYPES];
    GroupType *g;
    CharData *master;

    static CharData **members;
    static int max_members = 0;

    if (!max_members) {
        max_members = 16;
        CREATE(members, CharData *, max_members);
    }

    master = ch->group_master ? ch->group_master : ch;

    if (CAN_SEE(ch, master) && ch->in_room == master->in_room) {
        count = 1;
        members[0] = master;
    } else {
        count = 0;
    }

    for (g = master->groupees; g; g = g->next)
        if (CAN_SEE(ch, g->groupee) && ch->in_room == g->groupee->in_room) {
            if (count >= max_members) {
                max_members *= 2;
                RECREATE(members, CharData *, max_members);
            }
            members[count++] = g->groupee;
        }

    if (count == 1) {
        if (!IS_SET(mode, FAIL_SILENTLY))
            send_to_char("But you're the only one in your group here!\n", ch);
        return;
    }

    for (i = 0; i < NUM_COIN_TYPES; ++i) {
        share[i] = coins[i] / count;
        coins[i] -= share[i] * count;
        /* coins[i] now contains the remainder...but who gets it? */
        remainder_start[i] = number(0, count - 1);
    }

    /*
     * remainder_start[i] is the index of the 'first' group member who
     * gets an extra ith coin type.  If n = coins[i] is nonzero for a
     * particular i, then n group members, starting at the
     * remainder_start[i]th group member, will receive an extra coin.
     */

    send_to_char("&7&bYou split some coins with your group.&0\n", ch);
    act("&7&8$n splits some coins.&0", true, ch, 0, 0, TO_ROOM);
    for (j = 0; j < count; ++j) {
        /*
         * A slight hack: for each coin type, if the current group
         * member i gets an extra coin, then add it to the share array,
         * split the coins, then remove it from the share array.  This
         * reduces split_share()'s complexity significantly.
         */
        for (i = 0; i < NUM_COIN_TYPES; ++i)
            if (coins[i] && ((j + count - remainder_start[i]) % count) < coins[i])
                ++share[i];

        split_share(ch, members[j], share);

        for (i = 0; i < NUM_COIN_TYPES; ++i)
            if (coins[i] && ((j + count - remainder_start[i]) % count) < coins[i])
                --share[i];
    }
}

ACMD(do_split) {
    int coins[NUM_COIN_TYPES], i;

    if (IS_NPC(ch))
        return;

    if (!IS_GROUPED(ch)) {
        send_to_char("&2&8But you are not a member of any group!&0\n", ch);
        return;
    }

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char("Split what?\n", ch);
        return;
    }

    if (!parse_money(&argument, coins)) {
        send_to_char("That's not a coin type.\n", ch);
        return;
    }

    if (!coins[PLATINUM] && !coins[GOLD] && !coins[SILVER] && !coins[COPPER]) {
        send_to_char("Split zero coins?  Done.\n", ch);
        return;
    }

    for (i = 0; i < NUM_COIN_TYPES; ++i)
        if (coins[i] > GET_COINS(ch)[i]) {
            char_printf(ch, "You don't have enough %s!\n", COIN_NAME(i));
            return;
        }

    split_coins(ch, coins, 0);
}

ACMD(do_use) {
    ObjData *mag_item;

    half_chop(argument, arg, buf);
    if (!*arg) {
        sprintf(buf2, "What do you want to %s?\n", CMD_NAME);
        send_to_char(buf2, ch);
        return;
    }
    mag_item = GET_EQ(ch, WEAR_HOLD);

    if (!mag_item || !isname(arg, mag_item->name)) {
        switch (subcmd) {
        case SCMD_RECITE:
        case SCMD_QUAFF:
            if (!(mag_item = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg)))) {
                char_printf(ch, "You don't seem to have %s %s.\n", AN(arg), arg);
                return;
            }
            break;
        case SCMD_USE:
            /* Item isn't in first hand, now check the second. */
            mag_item = GET_EQ(ch, WEAR_HOLD2);
            if (!mag_item || !isname(arg, mag_item->name)) {
                char_printf(ch, "You don't seem to be holding %s %s.\n", AN(arg), arg);
                return;
            }
            break;
        default:
            log("SYSERR: Unknown subcmd passed to do_use");
            return;
            break;
        }
    }
    /* Do a level check -- Zantir 4/2/01 */
    if (GET_OBJ_LEVEL(mag_item) > GET_LEVEL(ch)) {
        send_to_char("That item is too powerful for you to use.\n", ch);
        return;
    }
    switch (subcmd) {
    case SCMD_QUAFF:
        if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
            send_to_char("You can only quaff potions.\n", ch);
            return;
        }
        break;
    case SCMD_RECITE:
        if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
            send_to_char("You can only recite scrolls.\n", ch);
            return;
        }
        break;
    case SCMD_USE:
        if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) && (GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
            send_to_char("You can't seem to figure out how to use it.\n", ch);
            return;
        }
        break;
    }

    mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_wimpy) {
    int wimp_lev;

    one_argument(argument, arg);

    if (!*arg) {
        if (GET_WIMP_LEV(ch)) {
            sprintf(buf, "Your current wimp level is %d hit points.\n", GET_WIMP_LEV(ch));
            send_to_char(buf, ch);
            return;
        } else {
            send_to_char("At the moment, you're not a wimp.  (sure, sure...)\n", ch);
            return;
        }
    }
    if (isdigit(*arg)) {
        if ((wimp_lev = atoi(arg))) {
            if (wimp_lev < 0)
                send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\n", ch);
            else if (wimp_lev > GET_MAX_HIT(ch))
                send_to_char("That doesn't make much sense, now does it?\n", ch);
            else if (wimp_lev > (GET_MAX_HIT(ch) >> 1))
                send_to_char("You can't set your wimp level above half your hit points.\n", ch);
            else {
                sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\n", wimp_lev);
                send_to_char(buf, ch);
                GET_WIMP_LEV(ch) = wimp_lev;
            }
        } else {
            send_to_char("Okay, you'll now tough out fights to the bitter end.\n", ch);
            GET_WIMP_LEV(ch) = 0;
        }
    } else
        send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\n", ch);

    return;
}

ACMD(do_display) {
    int i, x;

    one_argument(argument, arg);

    if (!*arg || !is_number(arg)) {
        send_to_char("The following pre-set prompts are availible...\n", ch);
        for (i = 0; default_prompts[i][0]; i++) {
            sprintf(buf, "%2d. %-20s %s\n", i, default_prompts[i][0], default_prompts[i][1]);
            send_to_char(buf, ch);
        }
        send_to_char("Usage: display <number>\n", ch);
        return;
    }

    i = atoi(arg);

    if (i < 0) {
        send_to_char("The number cannot be negative.\n", ch);
        return;
    }

    for (x = 0; default_prompts[x][0]; ++x)
        ;

    if (i >= x) {
        sprintf(buf, "The range for the prompt number is 0-%d.\n", x - 1);
        send_to_char(buf, ch);
        return;
    }

    if (GET_PROMPT(ch))
        free(GET_PROMPT(ch));

    GET_PROMPT(ch) = strdup(default_prompts[i][1]);
    sprintf(buf, "Set your prompt to the %s preset prompt.\n", default_prompts[i][0]);
    send_to_char(buf, ch);
}

ACMD(do_prompt) {
    skip_spaces(&argument);

    if (!*argument) {
        sprintf(buf, "Your prompt is currently: %s\n", (GET_PROMPT(ch) ? escape_ansi(GET_PROMPT(ch)) : "n/a"));
        send_to_char(buf, ch);
        return;
    }

    delete_doubledollar(argument);

    if (GET_PROMPT(ch))
        free(GET_PROMPT(ch));

    GET_PROMPT(ch) = strdup(argument);

    sprintf(buf, "Okay, set your prompt to: %s\n", escape_ansi(argument));
    send_to_char(buf, ch);
}

const char *idea_types[] = {"bug", "typo", "idea", "note"};

void send_to_mantis(CharData *ch, int category, const char *str) {
    std::string url;

    if (!ch || !str || !*str)
        return;

    url = fmt::format("curl -s \"http://bug.fierymud.org/fiery_report.php?plr={}&room={}&cat={}&build={}&msg=",
                      GET_NAME(ch), CH_RVNUM(ch), idea_types[category], get_build_number());

    for (const char *p = str; *p; ++p)
        if (isalpha(*p) || isdigit(*p))
            url += *p;
        else
            url += fmt::format("{:02x}", *p);

    url += "\"";

    system(url.c_str());
}

ACMD(do_gen_write) {
    FILE *fl;
    const char *filename;
    char buf[MAX_STRING_LENGTH];
    struct stat fbuf;
    extern int max_filesize;
    time_t ct;

    ch = REAL_CHAR(ch);

    if (IS_NPC(ch)) {
        send_to_char("Monsters can't have ideas - go away.\n", ch);
        return;
    }

    skip_spaces(&argument);
    delete_doubledollar(argument);

    switch (subcmd) {
    case SCMD_NOTE:
        argument = one_argument(argument, arg);
        if (!*argument) {
            send_to_char("Usage: note <player> <text>\n", ch);
            return;
        }
        get_pfilename(arg, buf, NOTES_FILE);
        filename = buf;
        char_printf(ch, "%s\n", buf);
        break;
    case SCMD_BUG:
        filename = BUG_FILE;
        break;
    case SCMD_TYPO:
        filename = TYPO_FILE;
        break;
    case SCMD_IDEA:
        filename = IDEA_FILE;
        break;
    default:
        log("SYSERR: do_gen_write() received invalid subcmd");
        return;
    }

    if (!speech_ok(ch, true)) {
        char_printf(ch,
                    "You have been communicating too frequently recently.\n"
                    "Please idle momentarily and try to submit your %s again.\n",
                    idea_types[subcmd]);
        return;
    }

    if (!*argument) {
        send_to_char("Please enter a message to go with that bug, idea, or typo.\n", ch);
        return;
    }

    mprintf(L_STAT, LVL_IMMORT, "%s by %s [%d]: %s", idea_types[subcmd], GET_NAME(ch), CH_RVNUM(ch), argument);

    if (stat(filename, &fbuf) >= 0 && fbuf.st_size >= max_filesize) {
        send_to_char("Sorry, the file is full right now.. try again later.\n", ch);
        return;
    }

    if (!(fl = fopen(filename, "a"))) {
        perror("do_gen_write");
        send_to_char("Could not open the file.  Sorry.\n", ch);
        return;
    }

    ct = time(0);
    strftime(buf1, 15, TIMEFMT_DATE, localtime(&ct));

    fprintf(fl, "%-8s (%11.11s) [%5d] %s\n", GET_NAME(ch), buf1, world[ch->in_room].vnum, argument);
    fclose(fl);
    send_to_char("Thanks for the bug, idea, or typo comment!\n", ch);

    /*
     * If this is the production mud, send the bug/typo/idea to mantis
     */
    if (environment == ENV_PROD) {
        switch (subcmd) {
        case SCMD_BUG:
        case SCMD_TYPO:
        case SCMD_IDEA:
            send_to_mantis(ch, subcmd, argument);
        }
    }
}

ACMD(do_peace) {
    CharData *vict, *next_v;
    one_argument(argument, arg);
    if (!is_abbrev(arg, "off")) {
        act("&7$n &4&bglows&0&7 with a &bbright white aura&0&7 as $e waves $s "
            "mighty hand!&0",
            false, ch, 0, 0, TO_ROOM);
        send_to_room(
            "&7&bA peaceful feeling washes into the room, dousing all "
            "violence!&0\n",
            ch->in_room);
        for (vict = world[ch->in_room].people; vict; vict = next_v) {
            next_v = vict->next_in_room;
            if (FIGHTING(vict))
                stop_fighting(vict);
        }
        SET_FLAG(ROOM_FLAGS(ch->in_room), ROOM_PEACEFUL);
    } else {
        act("&7$n &4&bglows&0&7 with a &1&bbright red aura&0&7 as $e waves $s "
            "mighty hand!&0",
            false, ch, 0, 0, TO_ROOM);
        send_to_room(
            "&1&bThe peaceful feeling in the room subsides... You don't "
            "feel quite as safe anymore.&0\n",
            ch->in_room);
        REMOVE_FLAG(ROOM_FLAGS(ch->in_room), ROOM_PEACEFUL);
    }
}

ACMD(do_petition) {
    DescriptorData *d;
    CharData *tch;

    if (!ch->desc)
        return;

    skip_spaces(&argument);
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        send_to_char("Petition is for those wimpy mortals!\n", ch);
        return;
    }
    if (!*argument) {
        send_to_char("Yes, but WHAT do you want to petition?\n", ch);
        return;
    }

    if (!speech_ok(ch, 0))
        return;

    sprintf(buf, "&0&6You petition, '&b%s&0&6'&0\n", argument);
    send_to_char(buf, ch);

    sprintf(buf, "&0&6%s&0&6 petitions, '&b%s&0&6'&0\n", GET_NAME(REAL_CHAR(ch)), argument);

    for (d = descriptor_list; d; d = d->next) {
        if (!d->character)
            continue;
        tch = d->original ? d->original : d->character;
        if (PRF_FLAGGED(tch, PRF_NOPETI))
            continue;
        if (GET_LEVEL(tch) < LVL_IMMORT)
            continue;
        send_to_char(buf, d->character);
    }
}

/***************************************************************************
 * SUMMON_MOUNT
 ***************************************************************************/

ACMD(do_summon_mount) {
    int i;
    FollowType *fol;

    struct mount_type {
        int min_level;
        int min_align; /* absolute value */
        int hp_bonus;
        int mv_bonus;
        int pal_mob_vnum;
        int ant_mob_vnum;
    } mount_types[] = {                              /* lvl  align   hp   mv  pal  ant */
                       {0, 0, 0, 0, 48, 47},         /* gelding  / mare       */
                       {30, 700, 0, 0, 49, 50},      /* rouncy   / destrier   */
                       {60, 900, 50, 50, 45, 46},    /* warhorse / nightmare  */
                       {90, 1000, 100, 100, 51, 52}, /* courser  / shadowmare */
                       {-1, -1, -1, -1, -1, -1}};

    if (FIGHTING(ch)) {
        send_to_char("You can't concentrate enough while you are fighting.\n", ch);
        return;
    }

    if ((GET_CLASS(ch) != CLASS_PALADIN) && (GET_CLASS(ch) != CLASS_ANTI_PALADIN)) {
        send_to_char("You have no idea what you are trying to accomplish.\n", ch);
        return;
    }

    if (GET_LEVEL(ch) < LVL_GOD) {
        for (fol = ch->followers; fol; fol = fol->next)
            if (IS_NPC(fol->follower) && MOB_FLAGGED(fol->follower, MOB_MOUNTABLE)) {
                send_to_char("You already have a mount!\n", ch);
                return;
            }
        if (GET_LEVEL(ch) < 15) {
            send_to_char("You are not yet deemed worthy of a mount (try gaining some more experience)\n", ch);
            return;
        }
        if (!IS_GOOD(ch) && !IS_NPC(ch) && (GET_CLASS(ch) == CLASS_PALADIN)) {
            send_to_char("Not even horses can stand your offensive presence!\n", ch);
            return;
        }
        if (CH_INDOORS(ch)) {
            send_to_char("Try again, OUTDOORS THIS TIME!\n", ch);
            return;
        }
        if (GET_COOLDOWN(ch, CD_SUMMON_MOUNT)) {
            i = GET_COOLDOWN(ch, CD_SUMMON_MOUNT) / (1 MUD_HR) + 1;
            if (i == 1)
                strcpy(buf1, "hour");
            else
                sprintf(buf1, "%d hours", i);
            char_printf(ch, "You must wait another %s before you can summon your mount.\n", buf1);
            return;
        }
    }

    send_to_char("You begin calling for a mount..\n", ch);

    for (i = 1; mount_types[i].min_level <= GET_LEVEL(ch) &&
                ((GET_CLASS(ch) == CLASS_PALADIN) ? (mount_types[i].min_align <= GET_ALIGNMENT(ch)) : /* Paladin */
                     (-mount_types[i].min_align >= GET_ALIGNMENT(ch))) &&                             /* Anti */
                mount_types[i].min_level > 0;
         i++)
        ;
    i--;
    if (GET_CLASS(ch) == CLASS_PALADIN)
        summon_mount(ch, mount_types[i].pal_mob_vnum, mount_types[i].hp_bonus + 2 * GET_LEVEL(ch),
                     mount_types[i].mv_bonus + (GET_ALIGNMENT(ch) / 2) - 100);
    else
        summon_mount(ch, mount_types[i].ant_mob_vnum, mount_types[i].hp_bonus + 2 * GET_LEVEL(ch),
                     mount_types[i].mv_bonus + 150 - (GET_ALIGNMENT(ch) / 4));
}

void summon_mount(CharData *ch, int mob_vnum, int base_hp, int base_mv) {
    extern int ideal_mountlevel(CharData * ch);
    CharData *mount = nullptr;

    if (!ch || (ch->in_room == NOWHERE))
        /* The summoner died in the meantime.  Its events should have been
         * pulled, but why trust that */
        return;

    mount = read_mobile(mob_vnum, VIRTUAL);

    if (!mount) {
        send_to_char("No mount could be found, please report this to a god.\n", ch);
        log("SYSERR: No mount found in summon_mount.");
        return;
    }

    char_to_room(mount, ch->in_room); /*  was -2 */

    act("$N answers your summons!", true, ch, 0, mount, TO_CHAR);
    act("$N walks in, seemingly from nowhere, and nuzzles $n's face.", true, ch, 0, mount, TO_ROOM);
    SET_FLAG(EFF_FLAGS(mount), EFF_CHARM);
    SET_FLAG(EFF_FLAGS(mount), EFF_TAMED);
    SET_FLAG(MOB_FLAGS(mount), MOB_SUMMONED_MOUNT);
    add_follower(mount, ch);

    GET_LEVEL(mount) = 5;
    if (ideal_mountlevel(ch) < GET_LEVEL(mount))
        GET_LEVEL(mount) = MAX(0, ideal_mountlevel(ch));
    GET_MAX_HIT(mount) = MAX(10, base_hp + number(50, 100));
    GET_HIT(mount) = GET_MAX_HIT(mount);
    GET_MAX_MOVE(mount) = MAX(10, base_mv + number(0, 50));
    GET_MOVE(mount) = GET_MAX_MOVE(mount);
}

/***************************************************************************
 * Call your pet
 ***************************************************************************/
ACMD(do_call) {
    bool found = false;
    FollowType *k;

    for (k = ch->followers; k; k = k->next) {
        if (IS_PET(k->follower)) {
            found = true;
        }
    }

    if (found) {
        send_to_char("You whistle sharply.\n", ch);

        for (k = ch->followers; k; k = k->next) {
            if (IS_PET(k->follower) && k->follower->master == ch) {
                if (IN_ROOM(ch) == IN_ROOM(k->follower)) {
                    act("$N looks at you curiously.", false, ch, 0, k->follower, TO_CHAR);
                    act("You look at $n as they whistle for you.", false, ch, 0, k->follower, TO_VICT);
                    act("$n whistles at $N, who looks at $m curiously.", false, ch, 0, k->follower, TO_NOTVICT);
                } else {
                    char_from_room(k->follower);
                    char_to_room(k->follower, ch->in_room);
                    act("$N rushes into the room and looks at you expectantly.", false, ch, 0, k->follower, TO_CHAR);
                    act("You hear $n whistle in the distance and rush to rejoin them.", false, ch, 0, k->follower,
                        TO_VICT);
                    act("$n whistles sharply and $N rushes in to join $m.", false, ch, 0, k->follower, TO_NOTVICT);
                }
            }
        }
    } else {
        send_to_char("You don't have a pet to call out to.\n", ch);
    }
}

/***************************************************************************
 * LAY_HANDS
 ***************************************************************************/

ACMD(do_layhand) {
    CharData *vict;
    int dam = GET_CHA(ch) * GET_LEVEL(ch) / 10; /* Base damage/healing */

    /* Check for appropriate class */
    if (GET_CLASS(ch) != CLASS_PALADIN && GET_CLASS(ch) != CLASS_ANTI_PALADIN) {
        send_to_char("You don't have the ability to lay hands.\n", ch);
        return;
    }

    /* Make sure we haven't already used it for the day */
    if (GET_COOLDOWN(ch, CD_LAY_HANDS)) {
        send_to_char("You need more rest before laying hands again.\n", ch);
        return;
    }

    one_argument(argument, arg);

    /*
     * Determine target.
     */
    if (*arg) {
        /* If a target was specified, attempt to use that. */
        if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
            send_to_char(NOPERSON, ch);
            return;
        }
    } else {
        /* If no target was specified, default depending on class. */
        if (GET_CLASS(ch) == CLASS_PALADIN)
            vict = ch;
        else if (FIGHTING(ch)) /* Anti-paladin */
            vict = FIGHTING(ch);
        else {
            send_to_char("How about a target this time!?\n", ch);
            return;
        }
    }

    if (GET_CLASS(ch) == CLASS_PALADIN) {
        if (!IS_GOOD(ch)) {
            send_to_char("For your evil ways, your god has forsaken you!\n", ch);
            return;
        }

        /* Paladins heal all players, and good/neutral mobs, except undead. */
        if (IS_PC(vict) || (!IS_EVIL(vict) && GET_LIFEFORCE(vict) != LIFE_UNDEAD))
            dam = -1.5 * dam + number(0, 50);

        /* Paladins harm all evil mobs and undead, regardless of alignment. */
        else if (GET_LIFEFORCE(vict) == LIFE_UNDEAD)
            dam = 1.5 * dam + number(50, 150);
        else
            dam = 1.2 * dam + number(1, 50);
    } else { /* Anti-paladin */
        if (!IS_EVIL(ch)) {
            send_to_char("For your benevolent ways, your god has forsaken you!\n", ch);
            return;
        }

        /* Anti-paladins heal the undead, regardless of alignment. */
        if (GET_LIFEFORCE(vict) == LIFE_UNDEAD)
            dam = -1.5 * dam + number(1, 50);

        /* Anti-paladins have no effect on evils. */
        else if (IS_EVIL(vict)) {
            send_to_char("Your harmful touch has no affect on other evils.\n", ch);
            return;
        }

        /* Anti-paladins harm good and neutral mobs/players. */
        else
            dam = 1.2 * dam + number(1, 50);
    }

    damage(ch, vict, dam, SKILL_LAY_HANDS);

    if (GET_LEVEL(ch) < LVL_GOD)
        SET_COOLDOWN(ch, CD_LAY_HANDS, 4 MUD_HR);
}

/***************************************************************************
 * end LAY_HANDS
 ***************************************************************************/

/***************************************************************************
 * CONTROL_UNDEAD
 ****************************************************************************/
/*
ACMD(do_control_undead)
{
 CharData *vict = NULL;
  char *to_vict = NULL, *to_room = NULL, *to_char = NULL;
  affected_type af;
  float control_duration = 0;

  if (GET_SKILL(ch, SKILL_CONTROL_UNDEAD) <= 0) {
    send_to_char("You don't know how to control undead!\n", ch);
    return;
  }

  if (FIGHTING(ch)) {
    send_to_char("You can't gain control the dead while fighting!\n",ch);
    return;
  }

  act("You attempt to gain control over the undead", false, ch, 0, vict,
TO_CHAR);

  for(vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
        if (!IS_NPC(vict)) {
                continue;
        } else {
                if (GET_LIFEFORCE(vict) == LIFE_UNDEAD) {
                        act("You focus your powers on controlling
$N.",false,ch,0,vict,TO_CHAR);

                                                                                                     if
(mag_savingthrow(vict, SAVING_SPELL)) {*//*test for not falling prey */
/*to_char="$N resists your pitiful attempt to control $M.";
   to_vict="&7&b$n tries to control you but fails!&0";
   to_room="&7&b$n tries to control $N but nothing happens.&0";
   act(to_char, false, ch, 0, vict, TO_CHAR);
   act(to_vict, false, ch, 0, vict, TO_VICT);
   act(to_room, true, ch, 0, vict, TO_NOTVICT);
   } else {
                                              if (mag_savingthrow(vict,
   SAVING_SPELL)) { *//*test for getting scared */
/*to_char="$N starts to shake and gets very angry!";
   to_vict="&7&b$n tries to control you so you attack him!&0";
   to_room="&7&b$n tries to control $N, but $N becomes very angry!&0";
   act(to_char, false, ch, 0, vict, TO_CHAR);
   act(to_vict, false, ch, 0, vict, TO_VICT);
   act(to_room, true, ch, 0, vict, TO_NOTVICT);
   SET_FLAG(MOB_FLAGS(vict), MOB_AGGRESSIVE);
   attack(vict, ch);
   } else {
   to_char="$N starts to wither and falls under your control!";
   to_vict="&7&b$n gestures towards you and you feel your powers wither
   away!&0"; to_room="&7&b$n gestures at $N and gains complete control over
   $M!&0"; act(to_char, false, ch, 0, vict, TO_CHAR); act(to_vict, false, ch, 0,
   vict, TO_VICT); act(to_room, true, ch, 0, vict, TO_NOTVICT);
*//*set the mob as controlled (charmed) */
/*control_duration = ((GET_LEVEL(ch) * 20) + (GET_SKILL(ch,
   SKILL_CONTROL_UNDEAD) * 2))/60; control_duration = 1; af.type =
   SKILL_CONTROL_UNDEAD; af.duration = control_duration; af.bitvector = 0;
   af.bitvector2 = 0;
   af.bitvector3 = EFF_CONTROLLED;
   af.modifier = 0;
   af.location = APPLY_NONE;
   affect_to_char(vict, &af);
   SET_FLAG(MOB_FLAGS(vict), MOB_CONTROLLED);
   af.type = SPELL_CHARM;
   af.duration = control_duration + 1;
   af.bitvector = EFF_CHARM;
   af.bitvector2 = 0;
   af.bitvector3 = 0;
   af.modifier = 0;
   af.location = APPLY_NONE;
   affect_to_char(vict, &af);
   add_follower(vict, ch);
   REMOVE_FLAG(MOB_FLAGS(vict), MOB_AGGRESSIVE);
   REMOVE_FLAG(MOB_FLAGS(vict), MOB_SPEC);
   }
   }
   }
   }
   }
   improve_skill(ch, SKILL_CONTROL_UNDEAD);

   } */

/***************************************************************************
 * end CONTROL_UNDEAD
 ****************************************************************************/

/***************************************************************************
 * FIRST_AID
 ***************************************************************************/

#define LINTERP(min, pct, max) ((min) + ((((max) - (min)) * pct) / 100.0))

ACMD(do_first_aid) {
    double avg, dev, low, cap, heal;
    int orig_hp;

    if (FIGHTING(ch)) {
        send_to_char("You are too busy fighting to attend to yourself!\n", ch);
        return;
    }

    if (GET_COOLDOWN(ch, CD_FIRST_AID)) {
        send_to_char("You can only do this once per day.\n", ch);
        return;
    }

    if (GET_HIT(ch) >= GET_MAX_HIT(ch)) {
        send_to_char("You're already in pristine health!\n", ch);
        return;
    }

    avg = LINTERP(15.0, GET_SKILL(ch, SKILL_FIRST_AID), 45.0);
    dev = LINTERP(3.0, GET_SKILL(ch, SKILL_FIRST_AID), 5.0);
    low = LINTERP(10.0, GET_SKILL(ch, SKILL_FIRST_AID), 35.0);
    cap = LINTERP(20.0, GET_SKILL(ch, SKILL_FIRST_AID), 50.0);

    heal = normal_random(avg, dev);
    if (heal < low)
        heal = low;
    if (heal > cap)
        heal = cap;

    orig_hp = GET_HIT(ch);
    GET_HIT(ch) += (GET_MAX_HIT(ch) * heal) / 100;
    if (GET_HIT(ch) > GET_MAX_HIT(ch))
        GET_HIT(ch) = GET_MAX_HIT(ch);

    char_printf(ch,
                "You attempt to render first aid unto yourself. "
                "(" AHGRN "%d" ANRM ")\n",
                GET_HIT(ch) - orig_hp);

    improve_skill(ch, SKILL_FIRST_AID);

    if (GET_LEVEL(ch) < LVL_IMMORT)
        SET_COOLDOWN(ch, CD_FIRST_AID, 24 MUD_HR);
}

/***************************************************************************
 * end FIRST_AID
 ***************************************************************************/

ACMD(do_ignore) {
    CharData *target, *tch;
    char arg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];

    tch = REAL_CHAR(ch);

    if (IS_NPC(tch))
        return;

    one_argument(argument, arg);

    if (!*arg || is_abbrev(arg, "off")) {
        send_to_char("You feel sociable and stop ignoring anyone.\n", ch);
        tch->player_specials->ignored = nullptr;
        return;
    }
    if (!(target = find_char_around_char(ch, find_vis_by_name(ch, arg))) || IS_NPC(target)) {
        send_to_char(NOPERSON, ch);
        return;
    }
    sprintf(buf, "You now ignore %s.\n", GET_NAME(target));
    send_to_char(buf, ch);
    tch->player_specials->ignored = target;
}

ACMD(do_point) {
    int found;
    CharData *tch = nullptr;
    ObjData *tobj = nullptr;

    argument = one_argument(argument, arg);
    skip_spaces(&argument);

    if (!*arg) {
        send_to_char("Point at what?  Or whom?\n", ch);
        return;
    }

    if (!(found = generic_find(arg, FIND_OBJ_ROOM | FIND_CHAR_ROOM, ch, &tch, &tobj))) {
        send_to_char("Can't find that!\n", ch);
        return;
    }

    if (tobj) {
        act("You point at $p.", false, ch, tobj, 0, TO_CHAR);
        act("$n points at $p.", true, ch, tobj, 0, TO_ROOM);
        return;
    }

    if (!tch) {
        log("SYSERR: do_point had neither tch nor tobj");
        return;
    }

    if (tch == ch) {
        send_to_char("You point at yourself.\n", ch);
        act("$n points at $mself.", true, ch, 0, 0, TO_ROOM);
        return;
    }

    if (GET_HIDDENNESS(tch) == 0) {
        act("You point at $N.", false, ch, 0, tch, TO_CHAR);
        act("$n points at $N.", true, ch, 0, tch, TO_NOTVICT);
        act("$n points at you.", false, ch, 0, tch, TO_VICT);
    } else {
        GET_HIDDENNESS(tch) = 0;
        act("You point out $N's hiding place.", false, ch, 0, tch, TO_CHAR);
        act("$n points out $N who was hiding here!", true, ch, 0, tch, TO_NOTVICT);
        act("$n points out your hiding place!", true, ch, 0, tch, TO_VICT);
    }
}
