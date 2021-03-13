/***************************************************************************
 * $Id: act.other.c,v 1.305 2011/03/16 13:39:58 myc Exp $
 ***************************************************************************/
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

#include "casting.h"
#include "chars.h"
#include "clan.h"
#include "class.h"
#include "comm.h"
#include "composition.h"
#include "conf.h"
#include "constants.h"
#include "cooldowns.h"
#include "db.h"
#include "dg_scripts.h"
#include "events.h"
#include "fight.h"
#include "handler.h"
#include "house.h"
#include "interpreter.h"
#include "lifeforce.h"
#include "limits.h"
#include "magic.h"
#include "math.h"
#include "money.h"
#include "movement.h"
#include "pfiles.h"
#include "players.h"
#include "quest.h"
#include "races.h"
#include "regen.h"
#include "screen.h"
#include "skills.h"
#include "specprocs.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

#include <math.h>
#include <sys/stat.h>

/* extern variables */
extern int max_group_difference;
extern int damage_amounts;

int remove_var(struct trig_var_data **var_list, char *name);
EVENTFUNC(camp_event);
void rem_memming(struct char_data *ch);
void summon_mount(struct char_data *ch, int mob_vnum, int base_hp, int base_mv);
void appear(struct char_data *ch);
void check_new_surroundings(struct char_data *ch, bool old_room_was_dark, bool tx_obvious);
void get_check_money(struct char_data *ch, struct obj_data *obj);
int roll_skill(struct char_data *ch, int skill);

/* extern procedures */
SPECIAL(shop_keeper);

void appear(struct char_data *ch) {
    bool was_hidden;

    active_effect_from_char(ch, SPELL_INVISIBLE);
    active_effect_from_char(ch, SPELL_NATURES_EMBRACE);

    was_hidden = IS_HIDDEN(ch);

    REMOVE_FLAG(EFF_FLAGS(ch), EFF_INVISIBLE);
    REMOVE_FLAG(EFF_FLAGS(ch), EFF_CAMOUFLAGED);
    GET_HIDDENNESS(ch) = 0;

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (was_hidden) {
            act("$n steps out of the shadows.", FALSE, ch, 0, 0, TO_ROOM);
            send_to_char("You step out of the shadows.\r\n", ch);
        } else {
            act("$n snaps into visibility.", FALSE, ch, 0, 0, TO_ROOM);
            send_to_char("You fade back into view.\r\n", ch);
        }
    } else
        act("You feel a strange presence as $n appears, seemingly from nowhere.", FALSE, ch, 0, 0, TO_ROOM);
}

void stop_guarding(struct char_data *ch) {

    if (ch->guarding) {
        act("You stop guarding $N.", FALSE, ch, 0, ch->guarding, TO_CHAR);
        if (ch->guarding->guarded_by == ch) {
            act("$n stops guarding you.", TRUE, ch, 0, ch->guarding, TO_VICT);
            ch->guarding->guarded_by = NULL;
        }
        ch->guarding = NULL;
    }
    if (ch->guarded_by)
        stop_guarding(ch->guarded_by);
}

ACMD(do_guard) {
    struct char_data *vict;

    one_argument(argument, arg);

    if (!GET_SKILL(ch, SKILL_GUARD)) {
        send_to_char("You don't have the protective skill required to guard.\r\n", ch);
        return;
    }

    if (!*arg) {
        if (ch->guarding)
            act("You are guarding $N.", FALSE, ch, 0, ch->guarding, TO_CHAR);
        else
            send_to_char("You are not guarding anyone.\r\n", ch);
        return;
    }

    if (!str_cmp(arg, "off"))
        vict = ch;
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        send_to_char("That person is not here.\r\n", ch);
        return;
    }

    if (ch == vict) {
        if (ch->guarding)
            stop_guarding(ch);
        else
            send_to_char("You are not guarding anyone.\r\n", ch);
        return;
    }

    if (vict->guarded_by) {
        if (vict->guarded_by == ch)
            send_to_char("You are already guarding that person.\r\n", ch);
        else
            send_to_char("Someone else is already guarding that person.\r\n", ch);
        return;
    }

    if (ch->guarding) {
        if (ch->guarding == vict) {
            send_to_char("You are already guarding that person.\r\n", ch);
            return;
        } else
            stop_guarding(ch);
    }
    act("You start guarding $N.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n starts guarding you.", TRUE, ch, 0, vict, TO_VICT);
    act("$n lays a protective eye on $N, guarding $M.", TRUE, ch, 0, vict, TO_NOTVICT);
    ch->guarding = vict;
    vict->guarded_by = ch;
}

ACMD(do_subclass) {
    int rem_spell(struct char_data * ch, int spell);
    int subclass, anyvalid;
    struct quest_list *quest = NULL;
    struct mem_list *memorized, *last_mem;
    float old_exp;
    char *s;
    struct classdef *c;

    /* Ew */
    if (IS_NPC(ch)) {
        send_to_char("NPCs don't subclass.  Ha.\r\n", ch);
        return;
    }

    c = &(classes[(int)GET_CLASS(ch)]);

    /* If not a base class, then bail */
    if (c->is_subclass) {
        send_to_char("You can only subclass once!\r\n", ch);
        return;
    }

    /* If below minimum quest level, bail */
    if (GET_LEVEL(ch) < 10) {
        send_to_char("You need to be level 10 before you can subclass!\r\n", ch);
        return;
    }

    /* If above maximum quest level, bail */
    if (GET_LEVEL(ch) > c->max_subclass_level) {
        sprintf(buf, "You can no longer subclass, because you are over level %d.\r\n", c->max_subclass_level);
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
                sprintf(buf, "  %s\r\n", classes[subclass].fmtname);
                if (!anyvalid) {
                    send_to_char("You may choose from the following classes for your race:\r\n", ch);
                    anyvalid = 1;
                }
                send_to_char(buf, ch);
            }
        }

        if (anyvalid) {
            sprintf(buf,
                    "You have until level %d to subclass. See HELP SUBCLASS_%s for "
                    "more information.\r\n",
                    c->max_subclass_level, c->name);
            /* Capitalize the "SUBCLASS_class" bit */
            for (s = buf + 43; *s && *s != ' '; s++)
                *s = toupper(*s);
            send_to_char(buf, ch);
        } else
            send_to_char("There are no subclasses available to you.\r\n", ch);
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
        log(buf);
        send_to_char("There is an error in your subclass quest.  Ask a god to reset it.\r\n", ch);
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

        cprintf(ch, "You have successfully subclassed as %s!\r\n", with_indefinite_article(CLASS_FULL(ch)));
        all_except_printf(ch, "%s has subclassed to %s!\r\n", GET_NAME(ch), CLASS_FULL(ch));
        mprintf(L_STAT, LVL_GOD, "%s has subclassed to %s", GET_NAME(ch), CLASS_FULL(ch));
        return;
    }

    /* Now we know the player is on a subclass quest, but not completed */
    sprintf(buf, "You are on the way to becoming a %s\r\n", classes[(int)subclass].fmtname);
    send_to_char(buf, ch);
    sprintf(buf, "You have until level %d to complete your quest.\r\n", c->max_subclass_level);
    send_to_char(buf, ch);
}

ACMD(do_quit) {
    int i;
    struct obj_data *money, *obj;
    one_argument(argument, arg);

    if (IS_NPC(ch) || !ch->desc) {
        send_to_char("You can't quit while shapechanged!\r\n", ch);
        return;
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("Goodbye, friend.  Come back soon!\r\n", ch);
        remove_player_from_game(ch, QUIT_QUITIMM);
        return;
    }

    if (FIGHTING(ch)) {
        send_to_char("No way!  You're fighting for your life!\r\n", ch);
        return;
    }

    if (subcmd != SCMD_QUIT) {
        send_to_char("For safety purposes, you must type out 'quit yes'.\r\n", ch);
        send_to_char(
            "Note: You will lose &1&beverything&0 if you quit!  Camping "
            "or renting will save everything.\r\n",
            ch);
        return;
    }

    if (!*arg || str_cmp(arg, "yes")) {
        send_to_char("You must type 'quit yes' to leave this world.\r\n", ch);
        send_to_char(
            "Note: You will lose &1&beverything&0 if you quit!  Camping "
            "or renting will save everything.\r\n",
            ch);
        return;
    }

    /* Ok, if we've made it this far it's ok to quit */

    if (GET_STANCE(ch) < STANCE_STUNNED) {
        send_to_char("You die before your time...\r\n", ch);
        act("$n quits the game, but is unable to fend off death...", TRUE, ch, 0, 0, TO_ROOM);
        act("$n is dead!  R.I.P.", TRUE, ch, 0, 0, TO_ROOM);
        die(ch, NULL);
        return;
    }

    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);

    sprintf(buf, "%s has quit the game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    send_to_char("Goodbye, friend.  Come back soon!\r\n", ch);

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
    char *name;
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
    int index, type, class, desired_index = -1, i;
    struct char_data *mob, *player;
    struct obj_data *obj;

    if (IS_NPC(ch) ? (!ch->desc || !POSSESSOR(ch)) : !GET_SKILL(REAL_CHAR(ch), SKILL_SHAPECHANGE)) {
        send_to_char("You have no idea how to do that!\r\n", ch);
        return;
    }

    argument = any_one_arg(argument, arg);

    /* If already shapechanged, other rules apply. */
    if (POSSESSED(ch)) {
        if (!*arg) {
            if (ch->char_specials.timer == 0)
                sprintf(buf, "You have just recently taken the form of %s.\r\n", GET_NAME(ch));
            else if (ch->char_specials.timer == 1)
                sprintf(buf, "You have been in the form of %s for 1 hour.\r\n", GET_NAME(ch));
            else
                sprintf(buf, "You have been in the form of %s for %d hours.\r\n", GET_NAME(ch),
                        ch->char_specials.timer);
            send_to_char(buf, ch);
        } else if (!is_abbrev(arg, "me"))
            send_to_char("You cannot shapechange to another animal from this form.\r\n", ch);
        else {
            if (POSSESSOR(ch)->desc)
                close_socket(POSSESSOR(ch)->desc);

            player = POSSESSOR(ch);

            send_to_char("You quickly morph back to your original self.\r\n", ch);
            act("$n&0 contorts wildly as it reforms into $N.", TRUE, ch, 0, player, TO_ROOM);

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
            player->desc->original = NULL;
            player->desc->character = player;
            ch->desc = NULL;
            player->forward = NULL;

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
        sprintf(buf,
                "You are still drained from your last shapechange.\r\n"
                "It will be another %s before you can change again.\r\n",
                buf1);
        send_to_char(buf, ch);
        return;
    }

    if (!*arg) {
        send_to_char("Shapechange to what?\r\n", ch);
        return;
    }

    if (!str_cmp(arg, "me")) {
        send_to_char("You are already in your normal form.\r\n", ch);
        return;
    }

    /* Check alignment. */
    if (GET_LEVEL(ch) < LVL_GOD) {
        if (GET_ALIGNMENT(ch) >= 350) {
            send_to_char("Your good loyalties betray your nature, inhibiting a transformation.\r\n", ch);
            return;
        } else if (GET_ALIGNMENT(ch) <= -350) {
            send_to_char("Your evil loyalties betray your nature, inhibiting a transformation.\r\n", ch);
            return;
        }
    }

    /* Determine the desired shapechange type.  You can supply as many
     * keywords as you like. */
    i = type = class = 0;
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
            class |= TANK;
        else if (is_abbrev(arg, "damager"))
            class |= DAMAGER;
        else if (is_abbrev(arg, "traveler"))
            class |= TRAVEL;
        else if (is_abbrev(arg, "tracker"))
            class |= TRACKER;
        else if (is_abbrev(arg, "aquatic"))
            type |= AQUATIC;
        else {
            for (desired_index = 0; *creatures[desired_index].name != '\n'; ++desired_index)
                if (is_abbrev(arg, creatures[desired_index].name)) {
                    type = creatures[desired_index].type & (MAMMAL | REPTILE | BIRD | AQUATIC | FISH);
                    class = creatures[desired_index].type & (TANK | DAMAGER | TRAVEL | TRACKER);
                    i = 1;
                    break;
                }
            if (*creatures[desired_index].name == '\n') {
                send_to_char("What kind of animal is that?\r\n", ch);
                return;
            }
        }
        argument = any_one_arg(argument, arg);
    } while (*argument && !i);

    if (IS_SET(type, AQUATIC | FISH) && !IS_WATER(ch->in_room) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("You won't be able to turn into that here!\r\n", ch);
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
        if (class && !IS_SET(creatures[i].type, class))
            continue;
        /* This is a match. */
        index = i;
        /* This creature was specifically requested.  Stop looking. */
        if (desired_index == i)
            break;
    }

    if (index < 0) {
        send_to_char("You don't know how to shapechange into that.\r\n", ch);
        return;
    }

    if (desired_index >= 0 && desired_index != index) {
        sprintf(buf, "You didn't feel quite up to changing into %s.\r\n",
                with_indefinite_article(creatures[desired_index].name));
        send_to_char(buf, ch);
    }

    /* Attempt to create the mobile. */
    if (!(mob = read_mobile(creatures[index].vnum, VIRTUAL))) {
        send_to_char("You start to change, then feel ill, and slump back to your normal form.\r\n", ch);
        sprintf(buf,
                "SYSERR: %s tried to shapechange into nonexistent "
                "mob prototype V#%d",
                GET_NAME(ch), creatures[index].vnum);
        mudlog(buf, BRF, LVL_GOD, TRUE);
        return;
    }

    act("The snap of bones reforming can be heard as $n takes the shape of $N&0!", FALSE, ch, 0, mob, TO_ROOM);
    act("You transform into $N!", FALSE, ch, 0, mob, TO_CHAR);

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
    hurt_char(mob, NULL, 0, TRUE);

    /* Add the player's name to the mob's namelist */
    GET_NAMELIST(mob) = strdupf("%s %s", GET_NAMELIST(mob), GET_NAME(ch));

    /* Set gender */
    GET_SEX(mob) = GET_SEX(ch);

    /* Move the descriptor. */
    ch->desc->character = mob;
    ch->desc->original = ch;
    mob->desc = ch->desc;
    ch->desc = NULL;
    ch->forward = mob;
}

bool creature_allowed_skill(struct char_data *ch, int skill) {
    int i, j;

    if (!IS_NPC(ch) || GET_MOB_VNUM(ch) < SHAPE_VNUM_MIN || GET_MOB_VNUM(ch) > SHAPE_VNUM_MAX)
        return FALSE;

    for (i = 0; creatures[i].vnum > 0; i++) {
        if (creatures[i].vnum == GET_MOB_VNUM(ch)) {
            for (j = 0; j < MAX_SHAPECHANGE_SKILLS; ++j)
                if (creatures[i].skills[j] == skill)
                    return TRUE;
        }
    }

    return FALSE;
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
    struct char_data *target = NULL;

    /*  Player save functionality for god types. */
    /*  The following section allows for gods to save players using the */
    /*  syntax: save <playername>, where <playername> is the player to */
    /*  be saved. This works on any character which has been brought */
    /*  online either by the player logging in or a god linkloading. */
    if (GET_LEVEL(ch) >= LVL_GOD) {
        one_argument(argument, arg);

        if (!strcmp(arg, "all")) {
            auto_save_all();
            cprintf(ch, "You have saved all players in the realm.\r\n");
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
        cprintf(ch, "No player by the name of %s is currently in the game.\r\n", arg);
        return;
    }

    if (IS_NPC(target)) {
        send_to_char("You can't save an NPC!\r\n", ch);
        return;
    }

    if (cmd) {
        if (ch == target)
            cprintf(ch, "Saving %s.\r\n", GET_NAME(ch));
        else
            cprintf(ch, "You have force-saved %s.\r\n", GET_NAME(target));
    }
    save_player(target);

    if (ch != target)
        mprintf(L_STAT, GET_LEVEL(ch), "(GC) %s has saved %s to file.", GET_NAME(ch), GET_NAME(target));
}

/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here) {
    if (CMD_IS("balance") || CMD_IS("deposit") || CMD_IS("withdraw") || CMD_IS("dump") || CMD_IS("exchange"))
        send_to_char("Sorry, you can only do that in a bank!\r\n", ch);
    else if (CMD_IS("appear") || CMD_IS("disappear"))
        send_to_char(HUH, ch);
    else if (CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive"))
        send_to_char("Sorry, you can only do that in a post office!\r\n", ch);
    else if (CMD_IS("list") || CMD_IS("value") || CMD_IS("buy") || CMD_IS("sell"))
        send_to_char("Sorry, you can only do that in a shop!\r\n", ch);
    else if (CMD_IS("rent"))
        send_to_char("Sorry, you can only do that in an inn!\r\n", ch);
    else
        send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}

ACMD(do_camp) {
    struct camp_event *ce;

    if (FIGHTING(ch) || EVENT_FLAGGED(ch, EVENT_CAMP)) {
        send_to_char("You are too busy to do this!\r\n", ch);
        return;
    }

    if (IS_NPC(ch) || !ch->desc) {
        send_to_char("You can't camp while shapechanged!\r\n", ch);
        return;
    }

    /* Restrictions: can't camp inside, in a city, or in water. */
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (CH_INDOORS(ch)) {
            send_to_char("&7You always pitch a tent indoors?&0\r\n", ch);
            return;
        }

        if (SECT(ch->in_room) == SECT_CITY) {
            send_to_char("&7Ye can't pitch a tent on the sidewalk fool.&0\r\n", ch);
            return;
        }

        if ((SECT(ch->in_room) == SECT_SHALLOWS) || (SECT(ch->in_room) == SECT_WATER) ||
            (SECT(ch->in_room) == SECT_UNDERWATER)) {
            send_to_char("&7Go buy a floating tent and try again.&0\r\n", ch);
            return;
            if (SECT(ch->in_room) == SECT_AIR) {
                send_to_char("&7You can't camp in mid-air.&0\r\n", ch);
                return;
            }
        }
        if (RIDING(ch)) {
            send_to_char("You'd better dismount first.\r\n", ch);
            return;
        }
    }

    if (GET_STANCE(ch) == STANCE_FIGHTING)
        send_to_char("No way!  You're fighting for your life!\r\n", ch);
    else if (GET_STANCE(ch) < STANCE_STUNNED)
        send_to_char("It's hard to set your tent up while dying...\r\n", ch);
    else {
        /* create and initialize the camp event */
        CREATE(ce, struct camp_event, 1);
        ce->ch = ch;
        ce->was_in = ch->in_room;
        event_create(EVENT_CAMP, camp_event, ce, TRUE, &(ch->events), GET_LEVEL(ch) >= LVL_IMMORT ? 5 : 350);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_CAMP);
        act("You start setting up camp.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n starts setting up camp.", TRUE, ch, 0, 0, TO_ROOM);
    }
}

EVENTFUNC(recall_event) {
    struct recall_event_obj *re = (struct recall_event_obj *)event_obj;
    struct char_data *ch;
    bool wasdark;

    ch = re->ch;

    if (ch->in_room != re->from_room) {
        send_to_room("The magic of the scroll fizzles as its target has left.\r\n", re->from_room);
        send_to_char("The magic of the scroll fizzles, as you left the area.\r\n", ch);
        return EVENT_FINISHED;
    };

    if (IS_NPC(ch) || !ch->desc)
        return EVENT_FINISHED;

    send_to_char("You feel the scroll's energy start to envelop you.\r\n", ch);
    act("$N disappears in a bright flash.\r\n", FALSE, ch, 0, ch, TO_NOTVICT);
    wasdark = IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch);

    dismount_char(ch);
    char_from_room(ch);
    char_to_room(ch, re->room);
    act("$N appears in a bright flash of light.\r\n", FALSE, ch, 0, ch, TO_NOTVICT);

    check_new_surroundings(ch, wasdark, TRUE);

    return EVENT_FINISHED;
}

EVENTFUNC(camp_event) {
    struct camp_event *ce = (struct camp_event *)event_obj;
    struct char_data *ch = NULL;
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
        send_to_char("You can't camp while mounted!\r\n", ch);
        return EVENT_FINISHED;
    }

    if (FIGHTING(ch)) {
        act("You decide now is not the best time for camping.", FALSE, ch, NULL, NULL, TO_CHAR);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_CAMP);
        return EVENT_FINISHED;
    }

    if (now_in != was_in) {
        act("You are no longer near where you began the campsite.", FALSE, ch, NULL, NULL, TO_CHAR);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_CAMP);
        return EVENT_FINISHED;
    }

    /* Yeah, let's not try to update characters who are about to be free'd, eh */
    rem_memming(ch);

    /* So players don't get saved with the meditate flag and cause syserrs
       when they log back on. */
    if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
        act("$N ceases $s meditative trance.", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("&8You stop meditating.\r\n&0", ch);
        REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
    }

    act("You complete your campsite, and leave this world for a while.", FALSE, ch, NULL, NULL, TO_CHAR);
    if (!GET_INVIS_LEV(ch))
        act("$n rolls up $s bedroll and tunes out the world.", TRUE, ch, 0, 0, TO_ROOM);

    sprintf(buf, "%s has camped in %s (%d).", GET_NAME(ch), world[ch->in_room].name, world[ch->in_room].vnum);

    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_CAMP);
    remove_player_from_game(ch, QUIT_CAMP);
    return EVENT_FINISHED;
}

ACMD(do_unbind) {
    int prob, percent;
    struct char_data *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg) {
        if (!PLR_FLAGGED(ch, PLR_BOUND)) {
            send_to_char("You are free as a bird!\r\n", ch);
            return;
        }
        prob = number(1, 70);
        percent = number(20, 101);
        if (prob > percent) {
            send_to_char("You break free from your binds!\r\n", ch);
            act("$n breaks free from his binds", FALSE, ch, 0, 0, TO_ROOM);
            REMOVE_FLAG(PLR_FLAGS(ch), PLR_BOUND);
            WAIT_STATE(ch, PULSE_VIOLENCE);
            return;
        } else
            WAIT_STATE(ch, PULSE_VIOLENCE * 3);
        return;
    } else {
        if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
            send_to_char("Unbind who?\r\n", ch);
            return;
        }
        if (vict == ch) {
            prob = number(20, 70);
            percent = number(1, 101);
            if (prob > percent) {
                send_to_char("You break free from your binds!\r\n", ch);
                act("$n breaks free from his binds", FALSE, ch, 0, 0, TO_ROOM);
                REMOVE_FLAG(PLR_FLAGS(ch), PLR_BOUND);
                WAIT_STATE(ch, PULSE_VIOLENCE);
                return;
            } else
                WAIT_STATE(ch, PULSE_VIOLENCE * 3);
            return;
        }
        REMOVE_FLAG(PLR_FLAGS(vict), PLR_BOUND);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        send_to_char("You are free of your binds.\r\n", vict);
    }
}

ACMD(do_bind) {
    struct char_data *vict;
    struct obj_data *held = GET_EQ(ch, WEAR_HOLD);
    int prob, percent;

    /* disable this command it's broken and is being used to
       ruin the game for players RSD 2/11/2001 */
    send_to_char("Huh?!?\r\n", ch);
    return;

    if (FIGHTING(ch)) {
        send_to_char("You are too busy fighting to think about that right now!\r\n", ch);
        return;
    }

    one_argument(argument, arg);

    if (!*arg)
        send_to_char("Bind who?\r\n", ch);
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))))
        send_to_char("There is no such player.\r\n", ch);
    else if (IS_NPC(vict))
        send_to_char("You can't do that to a mob!\r\n", ch);
    else if (GET_LEVEL(vict) > LVL_GOD)
        send_to_char("Hmmm...you'd better not.\r\n", ch);
    else if (!held)
        send_to_char("You must be holding a rope to tie someone up!\r\n", ch);
    else {

        if (ch == vict) {
            send_to_char("Oh, yeah, THAT'S real smart...\r\n", ch);
            return;
        }
        if (PLR_FLAGGED(vict, PLR_BOUND)) {
            send_to_char("Your victim is already tied up.\r\n", ch);
            return;
        }
        if (GET_OBJ_TYPE(held) != ITEM_ROPE) {
            send_to_char("You must be holding a rope to tie someone up!\r\n", ch);
            return;
        }
        if (GET_SKILL(ch, SKILL_BIND) == 0) {
            if (GET_STANCE(vict) > STANCE_STUNNED) {
                send_to_char("You aren't skilled enough to tie a conscious person\r\n", ch);
                return;
            } else {
                act("You tie $N up.... What next?", FALSE, ch, 0, vict, TO_CHAR);
                act("$n ties you up.... Hope he isnt the kinky type", FALSE, ch, 0, vict, TO_VICT);
                act("$n ties up $N.", FALSE, ch, 0, vict, TO_NOTVICT);
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
                act("You tie $N up.... What next?", FALSE, ch, 0, vict, TO_CHAR);
                act("$n ties you up.... Hope he isnt the kinky type", FALSE, ch, 0, vict, TO_VICT);
                act("$n ties up $N.", FALSE, ch, 0, vict, TO_NOTVICT);
                SET_FLAG(PLR_FLAGS(vict), PLR_BOUND);
                extract_obj(held);
                improve_skill(ch, SKILL_BIND);
                WAIT_STATE(ch, PULSE_VIOLENCE * 3);
                return;
            } else {
                act("You tries to tie $N up.... What next?", FALSE, ch, 0, vict, TO_CHAR);
                act("$n tries to tie you up.... Hope he isnt the kinky type", FALSE, ch, 0, vict, TO_VICT);
                act("$n tries to tie up $N.", FALSE, ch, 0, vict, TO_ROOM);
                improve_skill(ch, SKILL_BIND);
                WAIT_STATE(ch, PULSE_VIOLENCE * 3);
                return;
            }
        }
    }
}

ACMD(do_abort) {
    void abort_casting(struct char_data * ch);
    void flush_queues(struct descriptor_data * d);

    if (CASTING(ch)) {
        send_to_char("&8You abort your spell!&0\r\n", ch);
        abort_casting(ch);
        if (ch->desc)
            flush_queues(ch->desc);
    } else
        send_to_char("You're not even casting a spell right now.\r\n", ch);
}

ACMD(do_hide) {
    long lower_bound, upper_bound;
    int skill;

    if (!GET_SKILL(ch, SKILL_HIDE)) {
        send_to_char("You'd better leave that art to the rogues.\r\n", ch);
        return;
    }

    if (RIDING(ch)) {
        send_to_char("While mounted? I don't think so...\r\n", ch);
        return;
    }

    if (IS_HIDDEN(ch))
        send_to_char("You try to find a better hiding spot.\r\n", ch);
    else
        send_to_char("You attempt to hide yourself.\r\n", ch);

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
    struct char_data *vict;
    struct obj_data *obj;
    char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
    int percent, eq_pos, caught = 0, coins[NUM_COIN_TYPES];

    ACMD(do_gen_comm);

    if (FIGHTING(ch)) {
        send_to_char("You can't steal while you are fighting!\r\n", ch);
        return;
    }
    if (GET_SKILL(ch, SKILL_STEAL) <= 0) {
        send_to_char("You don't know how to steal!\r\n", ch);
        return;
    }
    if (MOB_FLAGGED(ch, MOB_ILLUSORY)) {
        send_to_char("Being an illusion, you can't steal things.\r\n", ch);
        return;
    }
    if (!RIGID(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("You can't handle objects in your condition.\r\n", ch);
        return;
    }

    argument = one_argument(argument, obj_name);
    one_argument(argument, vict_name);

    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, vict_name)))) {
        send_to_char("Steal what from who?\r\n", ch);
        return;
    } else if (vict == ch) {
        send_to_char("Come on now, that's rather stupid!\r\n", ch);
        return;
    }

    /* Player-stealing is only allowed during PK. */
    if (!attack_ok(ch, vict, FALSE)) {
        send_to_char("You can't steal from them!\r\n", ch);
        return;
    }
    if (ROOM_FLAGGED(vict->in_room, ROOM_ARENA)) {
        send_to_char("You can't steal in the arena!\r\n", ch);
        return;
    }

    /* 101% is a complete failure */
    percent = number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;
    if (!CAN_SEE(vict, ch))
        percent -= dex_app_skill[GET_DEX(ch)].p_pocket;

    /* Stealing from unconscious folks is always successful. */
    if (!AWAKE(vict))
        percent = -1;

    /* ... except that you cannot steal from immortals, shopkeepers or those who are aware. */
    if (GET_LEVEL(vict) >= LVL_IMMORT || GET_MOB_SPEC(vict) == shop_keeper || MOB_FLAGGED(vict, MOB_AWARE))
        percent = 101 + 50;

    /* First check whether the thief wants to steal money */
    if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

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
                act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
                return;
            } else {
                /* You cannot steal equipped items unless the victim is knocked out. */
                if (GET_STANCE(vict) > STANCE_STUNNED) {
                    send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
                    return;
                } else if (GET_OBJ_LEVEL(obj) > GET_LEVEL(ch)) {
                    act("$p is too powerful for you to steal.", FALSE, ch, obj, 0, TO_CHAR);
                    return;
                } else {
                    /* You stole an equipped item from a helpless mob. */
                    act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
                    act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
                    obj_to_char(unequip_char(vict, eq_pos), ch);
                }
            }
        } else {
            /* Steal an item from inventory */
            percent += GET_OBJ_WEIGHT(obj); /* Make heavy harder */
            if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
                /* You failed. */
                caught = TRUE;
                act("Oops...", FALSE, ch, 0, 0, TO_CHAR);
                act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
                act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
            } else if (GET_OBJ_LEVEL(obj) > GET_LEVEL(ch)) {
                act("$p is too powerful for you to steal.", FALSE, ch, obj, 0, TO_CHAR);
                return;
            } else {
                /* You succeeded. */
                if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
                    if (ADDED_WEIGHT_OK(ch, obj)) {
                        obj_from_char(obj);
                        obj_to_char(obj, ch);
                        send_to_char("Got it!\r\n", ch);
                        if (AWAKE(vict))
                            improve_skill(ch, SKILL_STEAL);
                        get_check_money(ch, obj);
                    }
                } else
                    send_to_char("You cannot carry that much.\r\n", ch);
            }
        }
    } else {
        /* Steal some coins */
        if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
            /* Failed attempt to steal some coins */
            caught = TRUE;
            act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
            act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
            act("$n tries to steal coins from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
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
                cprintf(ch, "Woohoo! You stole %s.\r\n", buf);
            } else {
                send_to_char("You couldn't get any coins...\r\n", ch);
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
    extern char *exp_message(struct char_data * ch);
    extern ACMD(do_experience);

    one_argument(argument, arg);

    if (!*arg)
        do_experience(ch, argument, 0, 0);
    else if (!str_cmp(arg, "gain"))
        send_to_char("You can only do that in your guild.\r\n", ch);
    else
        send_to_char("Huh?!?\r\n", ch);
}

ACMD(do_visible) {
    void perform_immort_vis(struct char_data * ch);

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        perform_immort_vis(ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_INVISIBLE) || IS_HIDDEN(ch) || EFF_FLAGGED(ch, EFF_CAMOUFLAGED)) {
        appear(ch);
    } else {
        send_to_char("You are already visible.\r\n", ch);
    }
}

ACMD(do_title) {
    int titles, which;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (IS_NPC(ch))
        send_to_char("Your title is fine... go away.\r\n", ch);
    else if (PLR_FLAGGED(ch, PLR_NOTITLE))
        send_to_char("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
    else if (GET_LEVEL(ch) >= LVL_IMMORT) {
        if (strlen(argument) > MAX_TITLE_LENGTH)
            cprintf(ch, "Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
        else {
            set_title(ch, argument);
            cprintf(ch, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
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
            send_to_char("You haven't earned any permanent titles!\r\n", ch);
            if (ch->player.title && *ch->player.title)
                send_to_char("Use 'title 0' to clear your current title.\r\n", ch);
        } else {
            titles = 0;
            cprintf(ch,
                    "You have earned the following titles:\r\n"
                    "  0) <no title>\r\n");
            if (GET_PERM_TITLES(ch))
                while (GET_PERM_TITLES(ch)[titles]) {
                    cprintf(ch, "  %d) %s\r\n", titles + 1, GET_PERM_TITLES(ch)[titles]);
                    ++titles;
                }
            if (GET_CLAN(ch) && IS_CLAN_MEMBER(ch))
                cprintf(ch, "  %d) %s %s\r\n", ++titles, GET_CLAN_TITLE(ch), GET_CLAN(ch)->abbreviation);
            cprintf(ch, "Use 'title <number>' to switch your title.\r\n");
        }
    } else if (!is_positive_integer(argument))
        send_to_char(
            "Usage: title\r\n"
            "       title <number>\r\n",
            ch);
    else {
        int i;
        which = atoi(argument);
        titles = 0;
        if (which == 0)
            set_title(ch, NULL);
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
            send_to_char("You don't have that many titles!\r\n", ch);
            return;
        }
        if (GET_TITLE(ch)) {
            sprintf(buf, "Okay, set your title to: %s\r\n", GET_TITLE(ch));
            send_to_char(buf, ch);
        } else
            send_to_char("Okay, cleared your title.\r\n", ch);
    }
}

ACMD(do_douse) {
    bool success = FALSE;
    struct char_data *vict;
    struct obj_data *obj;

    if (!ch)
        return;

    if (argument && *argument) {
        one_argument(argument, arg);
        vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg));
    } else
        vict = ch;

    if (!vict) {
        send_to_char("You don't see that person here.\r\n", ch);
        return;
    } else if (!EFF_FLAGGED(vict, EFF_ON_FIRE)) {
        send_to_char("Where's the fire?\r\n", ch);
        return;
    }

    /* A fountain in the room guarantees success. */
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
        if (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) {
            success = TRUE;
            break;
        }

    /* There is a fountain here */
    if (success) {

        /* Dousing yourself with a fountain */
        if (ch == vict) {
            act("*SPLASH* $n leaps into $o, putting out the flames that were "
                "consuming $m.",
                FALSE, ch, obj, 0, TO_ROOM);
            act("*SPLASH* You leap into $o, dousing your flames!", FALSE, ch, obj, 0, TO_CHAR);

            /* Dousing someone else with a fountain */
        } else {
            act("You dunk $N into $o, putting $M out! *SPLASH* *GURGLE*", FALSE, ch, obj, vict, TO_CHAR);
            act("$n dunks you into $o, putting your flames out! *GURGLE*", FALSE, ch, obj, vict, TO_VICT);
            act("$n dunks $N into $o, dousing $S flames! *SPLASH* *GURGLE*", FALSE, ch, obj, vict, TO_NOTVICT);
        }
    }

    /* No fountain */

    /* Water room? */

    else if (IS_WATER(IN_ROOM(vict))) {
        if (ch == vict) {
            act("$n ducks under the surface of the water, putting out $s flames.", FALSE, ch, 0, 0, TO_ROOM);
            act("You duck under the surface of the water, dousing your flames.", FALSE, ch, obj, 0, TO_CHAR);
        } else {
            act("You push $N under the water, putting $M out! *SPLASH* *GURGLE*", FALSE, ch, 0, vict, TO_CHAR);
            act("$n pushes you under the water, putting your flames out! *GURGLE*", FALSE, ch, 0, vict, TO_VICT);
            act("$n pushes $N under the water, dousing $S flames! *SPLASH* *GURGLE*", FALSE, ch, 0, vict, TO_NOTVICT);
        }
        success = TRUE;
    }

    /* Splashy room?  E.g., swamp, beach */

    else if (IS_SPLASHY(IN_ROOM(vict)) || SECT(vict->in_room) == SECT_BEACH) {
        if (ch == vict) {
            act("$n rolls around in the water, quickly putting out $s flames.", FALSE, ch, 0, 0, TO_ROOM);
            act("You roll around in the water, quickly dousing your flames.", FALSE, ch, obj, 0, TO_CHAR);
        } else {
            act("You push $N down into the shallow water, putting $M out! *SPLASH* "
                "*GURGLE*",
                FALSE, ch, 0, vict, TO_CHAR);
            act("$n pushes you into the shallow water, putting your flames out! "
                "*GURGLE*",
                FALSE, ch, 0, vict, TO_VICT);
            act("$n pushes $N into the shallow water, dousing $S flames! *SPLASH* "
                "*GURGLE*",
                FALSE, ch, 0, vict, TO_NOTVICT);
        }
        success = TRUE;
    }

    /* No water available! */

    /* No water, trying to douse yourself */
    else if (ch == vict) {
        if (GET_SKILL(ch, SKILL_DOUSE) < number(0, 100)) {
            act("$n&0 frantically rolls around on the ground, attempting to douse "
                "the flames consuming $s body.",
                TRUE, ch, 0, 0, TO_ROOM);
            send_to_char("You roll around on the ground, trying to douse the flames engulfing your body!\r\n", ch);
        } else {
            act("$n&0 rolls on the ground frantically, finally smothering the fire that was consuming $m.", TRUE, ch, 0,
                0, TO_ROOM);
            send_to_char("You roll around on the ground, finally smothering your flames.\r\n", ch);
            success = TRUE;
        }
    }

    /* No water, trying to douse someone else */
    else if (GET_SKILL(ch, SKILL_DOUSE) - 40 < number(0, 100)) {
        act("You frantically try to brush the flames from $N&0.", FALSE, ch, 0, vict, TO_CHAR);
        act("$n&0 aids you, attempting to douse your flames.", FALSE, ch, 0, vict, TO_VICT);
        act("$n&0 frantically attempts to brush the flames off $N&0.", FALSE, ch, 0, vict, TO_NOTVICT);
    } else {
        act("You frantically brush the flames from $N&0, finally extinguishing $M!", TRUE, ch, 0, vict, TO_CHAR);
        act("$n&0 aids you, finally putting your flames out!", FALSE, ch, 0, vict, TO_VICT);
        act("$n&0 finally douses the flames that were consuming $N&0!", FALSE, ch, 0, vict, TO_NOTVICT);
        success = TRUE;
    }

    if (success)
        REMOVE_FLAG(EFF_FLAGS(vict), EFF_ON_FIRE);
    improve_skill(ch, SKILL_DOUSE);
    WAIT_STATE(ch, PULSE_VIOLENCE);
}

ACMD(do_disband) {
    if (!ch->groupees) {
        send_to_char("&0&7&bBut you lead no group!&0\r\n", ch);
        return;
    }

    disband_group(ch, TRUE, FALSE);
}

ACMD(do_consent) {
    struct char_data *target;

    one_argument(argument, arg);

    if (!*arg) {
        if (!CONSENT(ch))
            send_to_char("You are not consented to anyone!\r\n", ch);
        else
            act("You are consented to $N.", TRUE, ch, 0, CONSENT(ch), TO_CHAR | TO_SLEEP);
        return;
    }

    if (!str_cmp(arg, "off"))
        target = ch; /* consent self to turn it off */
    else if (!(target = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
        send_to_char(NOPERSON, ch);
        return;
    }

    if (target == ch) {
        if (CONSENT(ch)) {
            act("&8$n&0&8 has revoked $s consent.&0", FALSE, ch, 0, CONSENT(ch), TO_VICT | TO_SLEEP);
            send_to_char("&8You revoke your consent.&0\r\n", ch);
            CONSENT(ch) = NULL;
        } else
            send_to_char("You haven't given your consent to anyone.\r\n", ch);
        return;
    }

    if (CONSENT(ch) == target) {
        act("$N already has your consent.", FALSE, ch, 0, target, TO_CHAR | TO_SLEEP);
        return;
    }

    if (!speech_ok(ch, 1)) {
        send_to_char("Your sore throat somehow prevents you from doing this.\r\n", ch);
        return;
    }

    if (CONSENT(ch))
        act("&8$n&0&8 has removed $s consent.&0", FALSE, ch, 0, CONSENT(ch), TO_VICT | TO_SLEEP);
    CONSENT(ch) = target;
    act("&7&bYou give your consent to $N.&0", FALSE, ch, 0, target, TO_CHAR | TO_SLEEP);
    act("&7&b$n has given you $s consent.&0", FALSE, ch, 0, target, TO_VICT | TO_SLEEP);
}

ACMD(do_bandage) {
    struct char_data *victim;

    one_argument(argument, arg);

    /* If no arg, bandage the first person in the room who needs it. */
    if (!*arg) {
        for (victim = world[ch->in_room].people; victim; victim = victim->next_in_room)
            if (CAN_SEE(ch, victim))
                if (GET_HIT(victim) < 0 || GET_STANCE(victim) < STANCE_STUNNED)
                    break;
        if (!victim) {
            send_to_char("Nobody here looks like they need bandaging!\r\n", ch);
            return;
        }
    } else if (!(victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        send_to_char(NOPERSON, ch);
        return;
    }
    if (GET_HIT(victim) >= 0 && GET_STANCE(victim) >= STANCE_STUNNED) {
        act("&8$N looks in pretty good shape already!&0", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    if (GET_SKILL(ch, SKILL_BANDAGE) > number(1, 80)) {
        act("&0&8You bandage $N.&0", FALSE, ch, 0, victim, TO_CHAR);
        act("&8$n&0&8 bandages $N&8's wounds.&0", FALSE, ch, 0, victim, TO_NOTVICT);
        hurt_char(victim, NULL, MAX(-3, GET_SKILL(ch, SKILL_BANDAGE) / -10), TRUE);
    } else {
        act("You fail to bandage $N properly.", FALSE, ch, 0, victim, TO_CHAR);
        act("&8$n fails an attempt to bandage $N&8's wounds.&0", FALSE, ch, 0, victim, TO_NOTVICT);
        if (DAMAGE_WILL_KILL(victim, 1)) {
            act("Your bandaging was so appalling that $N died!&0", FALSE, ch, 0, victim, TO_CHAR);
            act("&8$n kills $N with some dismal bandaging.&0", FALSE, ch, 0, victim, TO_NOTVICT);
        }
        hurt_char(victim, NULL, 1, TRUE);
    }

    improve_skill(ch, SKILL_BANDAGE);
    if (GET_LEVEL(ch) < LVL_IMMORT)
        WAIT_STATE(ch, PULSE_VIOLENCE);
}

void make_group_report_line(struct char_data *ch, char *buffer) {
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

void print_group(struct char_data *ch) {
    struct char_data *k;
    struct group_type *f;

    if (!ch->group_master && !ch->groupees)
        send_to_char("&2&8But you are not the member of a group!&0\r\n", ch);
    else {
        sprintf(buf, "%sYour group consists of:&0\r\n", CLR(ch, AUND));
        send_to_char(buf, ch);

        k = (ch->group_master ? ch->group_master : ch);
        if (CAN_SEE(ch, k)) {
            make_group_report_line(k, buf);
            strcat(buf, " (&0&2&bHead of group&0)\r\n");
            send_to_char(buf, ch);
        }

        for (f = k->groupees; f; f = f->next) {
            if (!CAN_SEE(ch, f->groupee))
                continue;

            make_group_report_line(f->groupee, buf);
            strcat(buf, "\r\n");
            send_to_char(buf, ch);
        }
    }
}

ACMD(do_group) {
    struct char_data *tch;
    int level_diff;
    bool group_all = FALSE;

    one_argument(argument, arg);

    /* No argument, just asking for group info. */
    if (!*arg) {
        print_group(ch);
        return;
    }

    /* Only info is allowed while sleeping, not actual grouping. */
    if (!AWAKE(ch)) {
        send_to_char("In your dreams, or what?\r\n", ch);
        return;
    }

    if (!str_cmp("all", arg)) {
        group_all = TRUE;
        tch = NULL;
    } else if (!(tch = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
        send_to_char(NOPERSON, ch);
        return;
    }

    /* Attempt to remove yourself from your group. */
    if (ch == tch) {
        if (ch->group_master || ch->groupees)
            ungroup(ch, TRUE, FALSE);
        else
            send_to_char("&2&8You're not in a group!&0\r\n", ch);
        return;
    }

    /* You can't enroll someone if you're in a group and not the leader. */
    if (ch->group_master) {
        send_to_char("&2&8You cannot enroll group members without being head of a group.&0\r\n", ch);
        return;
    }

    if (group_all) {
        /* group all followers */
        bool found = FALSE;
        struct descriptor_data *d;
        struct char_data *gch;

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
                        "great.\r\n"
                        "(The maximum allowable difference is currently %d.)&0\r\n",
                        GET_NAME(gch), max_group_difference);
                send_to_char(buf, ch);
                continue;
            }

            add_groupee(ch, gch);
            found = TRUE;
        }

        if (!found)
            send_to_char("&2&8No one has consented you that is not already in a group.&0\r\n", ch);
        return;
    }

    if (tch->group_master && tch->group_master != ch) {
        send_to_char("&2&8That person is already in a group.&0\r\n", ch);
        return;
    }

    if (tch->groupees) {
        send_to_char("&2&8That person is leading a group.&0\r\n", ch);
        return;
    }

    /* Ok, if the target is in your group, remove them. */
    if (tch->group_master == ch) {
        ungroup(tch, TRUE, TRUE);
        return;
    }

    /* Check for consent unless it's a charmed follower */
    if (CONSENT(tch) != ch && GET_LEVEL(ch) < LVL_IMMORT) {
        if (!(IS_NPC(tch) && tch->master == ch && EFF_FLAGGED(tch, EFF_CHARM))) {
            act("&2&8You do not have $S consent.&0", TRUE, ch, NULL, tch, TO_CHAR);
            return;
        }
    }

    level_diff = GET_LEVEL(ch) - GET_LEVEL(tch);

    if (max_group_difference && (level_diff > max_group_difference || level_diff < -max_group_difference)) {
        sprintf(buf,
                "&2&8You cannot group %s, because the level difference is too "
                "great.\r\n"
                "(The maximum allowable difference is currently %d.)&0\r\n",
                GET_NAME(tch), max_group_difference);
        send_to_char(buf, ch);
        return;
    }

    add_groupee(ch, tch);
}

static void split_share(struct char_data *giver, struct char_data *receiver, int coins[]) {
    if (coins[PLATINUM] || coins[GOLD] || coins[SILVER] || coins[COPPER]) {
        statemoney(buf, coins);
        cprintf(receiver, "You %s %s.\r\n", giver == receiver ? "keep" : "receive", buf);
    } else
        send_to_char("You forego your share.\r\n", receiver);
    GET_PLATINUM(receiver) += coins[PLATINUM];
    GET_GOLD(receiver) += coins[GOLD];
    GET_SILVER(receiver) += coins[SILVER];
    GET_COPPER(receiver) += coins[COPPER];
    GET_PLATINUM(giver) -= coins[PLATINUM];
    GET_GOLD(giver) -= coins[GOLD];
    GET_SILVER(giver) -= coins[SILVER];
    GET_COPPER(giver) -= coins[COPPER];
}

void split_coins(struct char_data *ch, int coins[], unsigned int mode) {
    int i, j, count, share[NUM_COIN_TYPES], remainder_start[NUM_COIN_TYPES];
    struct group_type *g;
    struct char_data *master;

    static struct char_data **members;
    static int max_members = 0;

    if (!max_members) {
        max_members = 16;
        CREATE(members, struct char_data *, max_members);
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
                RECREATE(members, struct char_data *, max_members);
            }
            members[count++] = g->groupee;
        }

    if (count == 1) {
        if (!IS_SET(mode, FAIL_SILENTLY))
            send_to_char("But you're the only one in your group here!\r\n", ch);
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

    send_to_char("&7&bYou split some coins with your group.&0\r\n", ch);
    act("&7&8$n splits some coins.&0", TRUE, ch, 0, 0, TO_ROOM);
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
        send_to_char("&2&8But you are not a member of any group!&0\r\n", ch);
        return;
    }

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char("Split what?\r\n", ch);
        return;
    }

    if (!parse_money(&argument, coins)) {
        send_to_char("That's not a coin type.\r\n", ch);
        return;
    }

    if (!coins[PLATINUM] && !coins[GOLD] && !coins[SILVER] && !coins[COPPER]) {
        send_to_char("Split zero coins?  Done.\r\n", ch);
        return;
    }

    for (i = 0; i < NUM_COIN_TYPES; ++i)
        if (coins[i] > GET_COINS(ch)[i]) {
            cprintf(ch, "You don't have enough %s!\r\n", COIN_NAME(i));
            return;
        }

    split_coins(ch, coins, 0);
}

ACMD(do_use) {
    struct obj_data *mag_item;

    half_chop(argument, arg, buf);
    if (!*arg) {
        sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
        send_to_char(buf2, ch);
        return;
    }
    mag_item = GET_EQ(ch, WEAR_HOLD);

    if (!mag_item || !isname(arg, mag_item->name)) {
        switch (subcmd) {
        case SCMD_RECITE:
        case SCMD_QUAFF:
            if (!(mag_item = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg)))) {
                cprintf(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
                return;
            }
            break;
        case SCMD_USE:
            /* Item isn't in first hand, now check the second. */
            mag_item = GET_EQ(ch, WEAR_HOLD2);
            if (!mag_item || !isname(arg, mag_item->name)) {
                cprintf(ch, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
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
        send_to_char("That item is too powerful for you to use.\r\n", ch);
        return;
    }
    switch (subcmd) {
    case SCMD_QUAFF:
        if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
            send_to_char("You can only quaff potions.\r\n", ch);
            return;
        }
        break;
    case SCMD_RECITE:
        if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
            send_to_char("You can only recite scrolls.\r\n", ch);
            return;
        }
        break;
    case SCMD_USE:
        if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) && (GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
            send_to_char("You can't seem to figure out how to use it.\r\n", ch);
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
            sprintf(buf, "Your current wimp level is %d hit points.\r\n", GET_WIMP_LEV(ch));
            send_to_char(buf, ch);
            return;
        } else {
            send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
            return;
        }
    }
    if (isdigit(*arg)) {
        if ((wimp_lev = atoi(arg))) {
            if (wimp_lev < 0)
                send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
            else if (wimp_lev > GET_MAX_HIT(ch))
                send_to_char("That doesn't make much sense, now does it?\r\n", ch);
            else if (wimp_lev > (GET_MAX_HIT(ch) >> 1))
                send_to_char("You can't set your wimp level above half your hit points.\r\n", ch);
            else {
                sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n", wimp_lev);
                send_to_char(buf, ch);
                GET_WIMP_LEV(ch) = wimp_lev;
            }
        } else {
            send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
            GET_WIMP_LEV(ch) = 0;
        }
    } else
        send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);

    return;
}

ACMD(do_display) {
    int i, x;

    one_argument(argument, arg);

    if (!*arg || !is_number(arg)) {
        send_to_char("The following pre-set prompts are availible...\r\n", ch);
        for (i = 0; default_prompts[i][0]; i++) {
            sprintf(buf, "%2d. %-20s %s\r\n", i, default_prompts[i][0], default_prompts[i][1]);
            send_to_char(buf, ch);
        }
        send_to_char("Usage: display <number>\r\n", ch);
        return;
    }

    i = atoi(arg);

    if (i < 0) {
        send_to_char("The number cannot be negative.\r\n", ch);
        return;
    }

    for (x = 0; default_prompts[x][0]; ++x)
        ;

    if (i >= x) {
        sprintf(buf, "The range for the prompt number is 0-%d.\r\n", x - 1);
        send_to_char(buf, ch);
        return;
    }

    if (GET_PROMPT(ch))
        free(GET_PROMPT(ch));

    GET_PROMPT(ch) = strdup(default_prompts[i][1]);
    sprintf(buf, "Set your prompt to the %s preset prompt.\r\n", default_prompts[i][0]);
    send_to_char(buf, ch);
}

ACMD(do_prompt) {
    skip_spaces(&argument);

    if (!*argument) {
        sprintf(buf, "Your prompt is currently: %s\r\n", (GET_PROMPT(ch) ? escape_ansi(GET_PROMPT(ch)) : "n/a"));
        send_to_char(buf, ch);
        return;
    }

    delete_doubledollar(argument);

    if (GET_PROMPT(ch))
        free(GET_PROMPT(ch));

    GET_PROMPT(ch) = strdup(argument);

    sprintf(buf, "Okay, set your prompt to: %s\r\n", escape_ansi(argument));
    send_to_char(buf, ch);
}

const char *idea_types[] = {"bug", "typo", "idea", "note"};

void send_to_mantis(struct char_data *ch, int category, const char *string) {
    const char *cat;
    extern int make_count;

    if (!ch || !string || !*string)
        return;

    str_start(buf, sizeof(buf));

    str_catf(buf,
             "curl -s \"http://bug.fierymud.org/fiery_report.php?"
             "plr=%s&room=%d&cat=%s&build=%d&msg=",
             GET_NAME(ch), CH_RVNUM(ch), idea_types[category], make_count);

    for (cat = string; *cat; ++cat)
        str_catf(buf, isalpha(*cat) ? "%c" : "%%%x", *cat);

    str_catf(buf, "\"");

    system(buf);
}

ACMD(do_gen_write) {
    FILE *fl;
    char *filename, buf[MAX_STRING_LENGTH];
    struct stat fbuf;
    extern int max_filesize;
    time_t ct;

    ch = REAL_CHAR(ch);

    if (IS_NPC(ch)) {
        send_to_char("Monsters can't have ideas - go away.\r\n", ch);
        return;
    }

    skip_spaces(&argument);
    delete_doubledollar(argument);

    switch (subcmd) {
    case SCMD_NOTE:
        argument = one_argument(argument, arg);
        if (!*argument) {
            send_to_char("Usage: note <player> <text>\r\n", ch);
            return;
        }
        get_pfilename(arg, buf, NOTES_FILE);
        filename = buf;
        cprintf(ch, "%s\r\n", buf);
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

    if (!speech_ok(ch, TRUE)) {
        cprintf(ch,
                "You have been communicating too frequently recently.\r\n"
                "Please idle momentarily and try to submit your %s again.\r\n",
                idea_types[subcmd]);
        return;
    }

    if (!*argument) {
        send_to_char("Please enter a message to go with that bug, idea, or typo.\r\n", ch);
        return;
    }

    mprintf(L_STAT, LVL_IMMORT, "%s by %s [%d]: %s", idea_types[subcmd], GET_NAME(ch), CH_RVNUM(ch), argument);

    if (stat(filename, &fbuf) >= 0 && fbuf.st_size >= max_filesize) {
        send_to_char("Sorry, the file is full right now.. try again later.\r\n", ch);
        return;
    }

    if (!(fl = fopen(filename, "a"))) {
        perror("do_gen_write");
        send_to_char("Could not open the file.  Sorry.\r\n", ch);
        return;
    }

    ct = time(0);
    strftime(buf1, 15, TIMEFMT_DATE, localtime(&ct));

    fprintf(fl, "%-8s (%11.11s) [%5d] %s\n", GET_NAME(ch), buf1, world[ch->in_room].vnum, argument);
    fclose(fl);
    send_to_char("Thanks for the bug, idea, or typo comment!\r\n", ch);

/*
 * If this is the production mud, send the bug/typo/idea to mantis
 */
#ifdef PRODUCTION
    switch (subcmd) {
    case SCMD_BUG:
    case SCMD_TYPO:
    case SCMD_IDEA:
        send_to_mantis(ch, subcmd, argument);
    }
#endif
}

ACMD(do_peace) {
    struct char_data *vict, *next_v;
    one_argument(argument, arg);
    if (!is_abbrev(arg, "off")) {
        act("&7$n &4&bglows&0&7 with a &bbright white aura&0&7 as $e waves $s "
            "mighty hand!&0",
            FALSE, ch, 0, 0, TO_ROOM);
        send_to_room(
            "&7&bA peaceful feeling washes into the room, dousing all "
            "violence!&0\r\n",
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
            FALSE, ch, 0, 0, TO_ROOM);
        send_to_room(
            "&1&bThe peaceful feeling in the room subsides... You don't "
            "feel quite as safe anymore.&0\r\n",
            ch->in_room);
        REMOVE_FLAG(ROOM_FLAGS(ch->in_room), ROOM_PEACEFUL);
    }
}

ACMD(do_petition) {
    struct descriptor_data *d;
    struct char_data *tch;

    if (!ch->desc)
        return;

    skip_spaces(&argument);
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        send_to_char("Petition is for those wimpy mortals!\r\n", ch);
        return;
    }
    if (!*argument) {
        send_to_char("Yes, but WHAT do you want to petition?\r\n", ch);
        return;
    }

    if (!speech_ok(ch, 0))
        return;

    sprintf(buf, "&0&6You petition, '&b%s&0&6'&0\r\n", argument);
    send_to_char(buf, ch);

    sprintf(buf, "&0&6%s&0&6 petitions, '&b%s&0&6'&0\r\n", GET_NAME(REAL_CHAR(ch)), argument);

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
    struct follow_type *fol;

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
        send_to_char("You can't concentrate enough while you are fighting.\r\n", ch);
        return;
    }

    if ((GET_CLASS(ch) != CLASS_PALADIN) && (GET_CLASS(ch) != CLASS_ANTI_PALADIN)) {
        send_to_char("You have no idea what you are trying to accomplish.\r\n", ch);
        return;
    }

    if (GET_LEVEL(ch) < LVL_GOD) {
        for (fol = ch->followers; fol; fol = fol->next)
            if (IS_NPC(fol->follower) && MOB_FLAGGED(fol->follower, MOB_MOUNTABLE)) {
                send_to_char("You already have a mount!\r\n", ch);
                return;
            }
        if (GET_LEVEL(ch) < 15) {
            send_to_char("You are not yet deemed worthy of a mount (try gaining some more experience)\r\n", ch);
            return;
        }
        if (!IS_GOOD(ch) && !IS_NPC(ch) && (GET_CLASS(ch) == CLASS_PALADIN)) {
            send_to_char("Not even horses can stand your offensive presence!\r\n", ch);
            return;
        }
        if (CH_INDOORS(ch)) {
            send_to_char("Try again, OUTDOORS THIS TIME!\r\n", ch);
            return;
        }
        if (GET_COOLDOWN(ch, CD_SUMMON_MOUNT)) {
            i = GET_COOLDOWN(ch, CD_SUMMON_MOUNT) / (1 MUD_HR) + 1;
            if (i == 1)
                strcpy(buf1, "hour");
            else
                sprintf(buf1, "%d hours", i);
            cprintf(ch, "You must wait another %s before you can summon your mount.\r\n", buf1);
            return;
        }
    }

    send_to_char("You begin calling for a mount..\r\n", ch);

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

void summon_mount(struct char_data *ch, int mob_vnum, int base_hp, int base_mv) {
    extern int ideal_mountlevel(struct char_data * ch);
    struct char_data *mount = NULL;

    if (!ch || (ch->in_room == NOWHERE))
        /* The summoner died in the meantime.  Its events should have been
         * pulled, but why trust that */
        return;

    mount = read_mobile(mob_vnum, VIRTUAL);

    if (!mount) {
        send_to_char("No mount could be found, please report this to a god.\r\n", ch);
        log("SYSERR: No mount found in summon_mount.");
        return;
    }

    char_to_room(mount, ch->in_room); /*  was -2 */

    act("$N answers your summons!", TRUE, ch, 0, mount, TO_CHAR);
    act("$N walks in, seemingly from nowhere, and nuzzles $n's face.", TRUE, ch, 0, mount, TO_ROOM);
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
    bool found = FALSE;
    struct follow_type *k;

    for (k = ch->followers; k; k = k->next) {
        if (IS_PET(k->follower)) {
            found = TRUE;
        }
    }

    if (found) {
        send_to_char("You whistle sharply.\r\n", ch);

        for (k = ch->followers; k; k = k->next) {
            if (IS_PET(k->follower) && k->follower->master == ch) {
                if (IN_ROOM(ch) == IN_ROOM(k->follower)) {
                    act("$N looks at you curiously.", FALSE, ch, 0, k->follower, TO_CHAR);
                    act("You look at $n as they whistle for you.", FALSE, ch, 0, k->follower, TO_VICT);
                    act("$n whistles at $N, who looks at $m curiously.", FALSE, ch, 0, k->follower, TO_NOTVICT);
                } else {
                    char_from_room(k->follower);
                    char_to_room(k->follower, ch->in_room);
                    act("$N rushes into the room and looks at you expectantly.", FALSE, ch, 0, k->follower, TO_CHAR);
                    act("You hear $n whistle in the distance and rush to rejoin them.", FALSE, ch, 0, k->follower,
                        TO_VICT);
                    act("$n whistles sharply and $N rushes in to join $m.", FALSE, ch, 0, k->follower, TO_NOTVICT);
                }
            }
        }
    } else {
        send_to_char("You don't have a pet to call out to.\r\n", ch);
    }
}

/***************************************************************************
 * LAY_HANDS
 ***************************************************************************/

ACMD(do_layhand) {
    struct char_data *vict;
    int dam = GET_DEX(ch) * GET_LEVEL(ch) / 10; /* Base damage/healing */

    /* Check for appropriate class */
    if (GET_CLASS(ch) != CLASS_PALADIN && GET_CLASS(ch) != CLASS_ANTI_PALADIN) {
        send_to_char("You don't have the ability to lay hands.\r\n", ch);
        return;
    }

    /* Make sure we haven't already used it for the day */
    if (GET_COOLDOWN(ch, CD_LAY_HANDS)) {
        send_to_char("You need more rest before laying hands again.\r\n", ch);
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
            send_to_char("How about a target this time!?\r\n", ch);
            return;
        }
    }

    if (GET_CLASS(ch) == CLASS_PALADIN) {
        if (!IS_GOOD(ch)) {
            send_to_char("For your evil ways, your god has forsaken you!\r\n", ch);
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
            send_to_char("For your benevolent ways, your god has forsaken you!\r\n", ch);
            return;
        }

        /* Anti-paladins heal the undead, regardless of alignment. */
        if (GET_LIFEFORCE(vict) == LIFE_UNDEAD)
            dam = -1.5 * dam + number(1, 50);

        /* Anti-paladins have no effect on evils. */
        else if (IS_EVIL(vict)) {
            send_to_char("Your harmful touch has no affect on other evils.\r\n", ch);
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
  struct char_data *vict = NULL;
  char *to_vict = NULL, *to_room = NULL, *to_char = NULL;
  struct affected_type af;
  float control_duration = 0;

  if (GET_SKILL(ch, SKILL_CONTROL_UNDEAD) <= 0) {
    send_to_char("You don't know how to control undead!\r\n", ch);
    return;
  }

  if (FIGHTING(ch)) {
    send_to_char("You can't gain control the dead while fighting!\r\n",ch);
    return;
  }

  act("You attempt to gain control over the undead", FALSE, ch, 0, vict,
TO_CHAR);

  for(vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
        if (!IS_NPC(vict)) {
                continue;
        } else {
                if (GET_LIFEFORCE(vict) == LIFE_UNDEAD) {
                        act("You focus your powers on controlling
$N.",FALSE,ch,0,vict,TO_CHAR);

                                                                                                     if
(mag_savingthrow(vict, SAVING_SPELL)) {*//*test for not falling prey */
/*to_char="$N resists your pitiful attempt to control $M.";
   to_vict="&7&b$n tries to control you but fails!&0";
   to_room="&7&b$n tries to control $N but nothing happens.&0";
   act(to_char, FALSE, ch, 0, vict, TO_CHAR);
   act(to_vict, FALSE, ch, 0, vict, TO_VICT);
   act(to_room, TRUE, ch, 0, vict, TO_NOTVICT);
   } else {
                                              if (mag_savingthrow(vict,
   SAVING_SPELL)) { *//*test for getting scared */
/*to_char="$N starts to shake and gets very angry!";
   to_vict="&7&b$n tries to control you so you attack him!&0";
   to_room="&7&b$n tries to control $N, but $N becomes very angry!&0";
   act(to_char, FALSE, ch, 0, vict, TO_CHAR);
   act(to_vict, FALSE, ch, 0, vict, TO_VICT);
   act(to_room, TRUE, ch, 0, vict, TO_NOTVICT);
   SET_FLAG(MOB_FLAGS(vict), MOB_AGGRESSIVE);
   attack(vict, ch);
   } else {
   to_char="$N starts to wither and falls under your control!";
   to_vict="&7&b$n gestures towards you and you feel your powers wither
   away!&0"; to_room="&7&b$n gestures at $N and gains complete control over
   $M!&0"; act(to_char, FALSE, ch, 0, vict, TO_CHAR); act(to_vict, FALSE, ch, 0,
   vict, TO_VICT); act(to_room, TRUE, ch, 0, vict, TO_NOTVICT);
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
        send_to_char("You are too busy fighting to attend to yourself!\r\n", ch);
        return;
    }

    if (GET_COOLDOWN(ch, CD_FIRST_AID)) {
        send_to_char("You can only do this once per day.\r\n", ch);
        return;
    }

    if (GET_HIT(ch) >= GET_MAX_HIT(ch)) {
        send_to_char("You're already in pristine health!\r\n", ch);
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

    cprintf(ch,
            "You attempt to render first aid unto yourself. "
            "(" AHGRN "%d" ANRM ")\r\n",
            GET_HIT(ch) - orig_hp);

    improve_skill(ch, SKILL_FIRST_AID);

    if (GET_LEVEL(ch) < LVL_IMMORT)
        SET_COOLDOWN(ch, CD_FIRST_AID, 24 MUD_HR);
}

/***************************************************************************
 * end FIRST_AID
 ***************************************************************************/

ACMD(do_ignore) {
    struct char_data *target, *tch;
    char arg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];

    tch = REAL_CHAR(ch);

    if (IS_NPC(tch))
        return;

    one_argument(argument, arg);

    if (!*arg || is_abbrev(arg, "off")) {
        send_to_char("You feel sociable and stop ignoring anyone.\r\n", ch);
        tch->player_specials->ignored = NULL;
        return;
    }
    if (!(target = find_char_around_char(ch, find_vis_by_name(ch, arg))) || IS_NPC(target)) {
        send_to_char(NOPERSON, ch);
        return;
    }
    sprintf(buf, "You now ignore %s.\r\n", GET_NAME(target));
    send_to_char(buf, ch);
    tch->player_specials->ignored = target;
}

ACMD(do_point) {
    int found;
    struct char_data *tch = NULL;
    struct obj_data *tobj = NULL;

    argument = one_argument(argument, arg);
    skip_spaces(&argument);

    if (!*arg) {
        send_to_char("Point at what?  Or whom?\r\n", ch);
        return;
    }

    if (!(found = generic_find(arg, FIND_OBJ_ROOM | FIND_CHAR_ROOM, ch, &tch, &tobj))) {
        send_to_char("Can't find that!\r\n", ch);
        return;
    }

    if (tobj) {
        act("You point at $p.", FALSE, ch, tobj, 0, TO_CHAR);
        act("$n points at $p.", TRUE, ch, tobj, 0, TO_ROOM);
        return;
    }

    if (!tch) {
        log("SYSERR: do_point had neither tch nor tobj");
        return;
    }

    if (tch == ch) {
        send_to_char("You point at yourself.\r\n", ch);
        act("$n points at $mself.", TRUE, ch, 0, 0, TO_ROOM);
        return;
    }

    if (GET_HIDDENNESS(tch) == 0) {
        act("You point at $N.", FALSE, ch, 0, tch, TO_CHAR);
        act("$n points at $N.", TRUE, ch, 0, tch, TO_NOTVICT);
        act("$n points at you.", FALSE, ch, 0, tch, TO_VICT);
    } else {
        GET_HIDDENNESS(tch) = 0;
        act("You point out $N's hiding place.", FALSE, ch, 0, tch, TO_CHAR);
        act("$n points out $N who was hiding here!", TRUE, ch, 0, tch, TO_NOTVICT);
        act("$n points out your hiding place!", TRUE, ch, 0, tch, TO_VICT);
    }
}

/***************************************************************************
 * $Log: act.other.c,v $
 * Revision 1.305  2011/03/16 13:39:58  myc
 * Fix all warnings for "the address of X will always evaluate to 'true'",
 * where X is a variable.
 *
 * Revision 1.304  2011/03/11 04:47:55  mud
 * Fix crash bug in shapechange that happens when a mob has
 * zero mv or hp and the player inhabiting the mob uses
 * 'shapechange me'.
 *
 * Revision 1.303  2010/06/09 22:32:01  mud
 * Moving toggle command and prf flags into prefs.[ch]
 *
 * Revision 1.302  2010/06/05 18:35:47  mud
 * Make pyre auto-target caster if sacrificial preference is
 * toggled on.
 *
 * Revision 1.301  2010/06/05 14:56:27  mud
 * Moving cooldowns to their own file.
 *
 * Revision 1.300  2009/07/17 01:19:01  myc
 * Autosplit no longer gives an error message if no one else
 * is present.
 *
 * Revision 1.299  2009/07/17 00:48:17  myc
 * Implemented anon toggle privilege.
 *
 * Revision 1.298  2009/06/10 18:27:43  myc
 * When a druid shapechanges, add the druid's name as a keyword
 * to the shapechange's namelist.
 *
 * Revision 1.297  2009/06/09 21:48:21  myc
 * Broadcast a message to the clan when someone subclasses.
 *
 * Revision 1.296  2009/06/09 05:30:58  myc
 * The way clan titles are implemented has changed; title command
 * has been adjusted accordingly.  Also renamed the NoClanTell
 * toggle to NoClanComm to cover other clan communication.
 *
 * Revision 1.295  2009/03/20 15:38:53  jps
 * Fix message for stop guarding.
 *
 * Revision 1.294  2009/03/20 13:56:22  jps
 * Moved coin info into an array of struct coindef.
 *
 * Revision 1.293  2009/03/19 23:16:23  myc
 * parse_money now takes a char** and moves the pointer up to
 * just past any money phrase it parses.
 *
 * Revision 1.292  2009/03/16 22:15:05  jps
 * Don't let imms break up groups with a simple 'group <other-group-leader>'
 *
 * Revision 1.291  2009/03/09 21:43:50  myc
 * Make do_steal use statemoney.
 *
 * Revision 1.290  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.289  2009/03/09 05:41:31  jps
 * Moved money stuff into money.h, money.c
 *
 * Revision 1.288  2009/03/09 04:50:38  myc
 * First aid now does a percentage of the max hit points, and is
 * always successful to some degree; uses a capped normal distribution.
 *
 * Revision 1.287  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.286  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.285  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.284  2009/03/07 22:29:13  jps
 * Use active_effect_from_char in aggro_remove_spells, so you get feedback
 * about those things being removed from you.
 *
 * Revision 1.283  2009/03/03 19:41:50  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.282  2009/02/21 03:30:16  myc
 * Removed L_FILE flag--mprintf now logs to file by default; assert
 * L_NOFILE to prevent that.
 *
 * Revision 1.281  2009/02/05 17:23:39  myc
 * Make bug/typo/idea cause laryngitis.
 *
 * Revision 1.280  2009/01/19 09:25:23  myc
 * Changed summoned mount cooldown to count 12 hours from time
 * mount is lost.
 *
 * Revision 1.279  2009/01/19 03:03:39  myc
 * Allow guarding of NPCs.
 *
 * Revision 1.278  2009/01/18 07:23:03  myc
 * level command will show exp bar now too.
 *
 * Revision 1.277  2009/01/16 23:36:34  myc
 * Fix use of uninitialized variable in do_group().
 *
 * Revision 1.276  2008/09/28 19:13:25  jps
 * Send idea log messages to file.
 *
 * Revision 1.275  2008/09/28 19:07:14  jps
 * Adding -s to curl so its progress info doesn't go in syslog.
 * Changing the log message for gen_write.
 *
 * Revision 1.274  2008/09/21 21:50:22  jps
 * Call transfer_battle when shapechanging, so the "replacement" character
 * is embroiled in the conflict the same way the original was.
 *
 * Revision 1.273  2008/09/20 08:24:44  jps
 * Don't store potentially negative values in unsigned variables!!!!!!!!!1
 *
 * Revision 1.272  2008/09/20 06:05:06  jps
 * Add macros POSSESSED and POSSESSOR.
 *
 * Revision 1.271  2008/09/09 08:23:37  jps
 * Placed sector info into a struct and moved its macros into rooms.h.
 *
 * Revision 1.270  2008/09/07 20:05:27  jps
 * Renamed exp_to_level to exp_next_level to make it clearer what it means.
 *
 * Revision 1.269  2008/09/07 01:32:32  jps
 * Don't send a message to all when "save all" is entered.
 *
 * Revision 1.268  2008/09/06 22:06:21  myc
 * Fixed the naming of NoTell in toggle.
 *
 * Revision 1.267  2008/09/04 06:47:36  jps
 * Changed sector constants to match their strings
 *
 * Revision 1.266  2008/09/02 06:52:30  jps
 * Using limits.h.
 *
 * Revision 1.265  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.264  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.263  2008/09/01 00:48:42  mud
 * Remove prototype which is imported from skills.h.
 *
 * Revision 1.262  2008/08/31 21:44:03  jps
 * Renamed StackObjs and StackMobs prefs to ExpandObjs and ExpandMobs.
 *
 * Revision 1.261  2008/08/31 21:05:38  myc
 * Abort command gives a useful message when not casting.
 *
 * Revision 1.260  2008/08/31 20:40:41  rbr
 * Changed do_group to be consent based instead of follow based
 *
 * Revision 1.259  2008/08/29 04:16:26  myc
 * Added toggles for stacking objects and stacking mobiles.
 *
 * Revision 1.258  2008/08/19 02:11:14  jps
 * Don't apply fluid/rigidity restrictions to immortals.
 *
 * Revision 1.257  2008/08/18 01:35:38  jps
 * Replaced all \\n\\r with \\r\\n, not that it was really necessary...
 *
 * Revision 1.256  2008/08/16 23:04:03  jps
 * Added speech_ok() to comm.h.
 *
 * Revision 1.255  2008/08/15 03:51:35  myc
 * Bugs/typos/ideas reported by players on the production MUD
 * get automatically sent to Mantis now.
 *
 * Revision 1.254  2008/07/27 05:29:43  jps
 * Using remove_player_from_game and save_player_char functions.
 *
 * Revision 1.253  2008/07/18 17:25:34  jps
 * Add room identification to camping syslog messages.
 *
 * Revision 1.252  2008/06/21 06:59:38  jps
 * In split_coins(), don't assume that the group leader is in the room.
 *
 * Revision 1.251  2008/06/05 02:07:43  myc
 * Rewrote rent-saving code to use ascii-format files.
 *
 * Revision 1.250  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.249  2008/05/18 05:39:59  jps
 * Changed room_data member number to "vnum".
 *
 * Revision 1.248  2008/05/14 05:10:44  jps
 * Using hurt_char for play-time harm, while alter_hit is for changing hp only.
 *
 * Revision 1.247  2008/05/11 05:45:26  jps
 * alter_hit now takes the killer.  update_pos is removed.
 *
 * Revision 1.246  2008/04/19 21:10:13  myc
 * Removed some old unused sorting code.
 *
 * Revision 1.245  2008/04/13 00:57:34  jps
 * Added a toggle for auto-treasure looting.
 *
 * Revision 1.244  2008/04/12 21:13:18  jps
 * Using new header file magic.h.
 *
 * Revision 1.243  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.242  2008/04/05 18:07:09  myc
 * Re-implementing stealth for hide points.
 *
 * Revision 1.241  2008/04/05 16:49:07  myc
 * Oops, fixing autosplit/autoloot messages again?
 *
 * Revision 1.240  2008/04/05 15:30:38  myc
 * Slightly changing the split algorithm to more evenly distribute the
 * remainder coins.
 *
 * Revision 1.239  2008/04/05 05:04:24  myc
 * Fixed messages for autoloot/autosplit toggles.
 *
 * Revision 1.238  2008/04/05 03:30:55  jps
 * Remove the ability to see others' toggles from mortals.
 *
 * Revision 1.237  2008/04/04 22:19:06  jps
 * Put the right feedback messages with tog autoloot and tog autosplit.
 *
 * Revision 1.236  2008/04/04 20:37:42  myc
 * Made autoloot and autosplit level 0.
 *
 * Revision 1.235  2008/04/03 17:37:21  jps
 * Added autoinvis toggle for deities.
 *
 * Revision 1.234  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.233  2008/04/02 05:36:19  myc
 * Added the autosplit toggle and functionalized the split command.
 * Also added the autoloot toggle and removed the noname toggle.
 *
 * Revision 1.232  2008/04/02 04:55:59  myc
 * Redesigned the split command to randomly distribute extra monies.
 *
 * Revision 1.231  2008/04/02 03:24:44  myc
 * Rewrote group code and removed all major group code.
 *
 * Revision 1.230  2008/03/30 17:30:38  jps
 * Renamed objsave.c to pfiles.c and introduced pfiles.h. Files using functions
 * from pfiles.c now include pfiles.h and depend on it in the makefile.
 *
 * Revision 1.229  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.228  2008/03/26 23:32:00  jps
 * Prevent stealing things when in a fluid state.
 *
 * Revision 1.227  2008/03/26 16:44:36  jps
 * Replaced all checks for undead race with checks for undead lifeforce.
 * Replaced the undead race with the plant race.
 *
 * Revision 1.226  2008/03/22 03:22:38  myc
 * All invocations of the string editor now go through string_write()
 * instead of messing with the descriptor variables itself.  Also added
 * a toggle, LineNums, to decide whether to do /l or /n when entering
 * the string editor.
 *
 * Revision 1.225  2008/03/19 18:29:59  myc
 * Shapeshifted creatures inherit the sex from their original players.
 *
 * Revision 1.224  2008/03/19 04:32:14  myc
 * Fixed typo in do_quit.
 *
 * Revision 1.223  2008/03/17 15:31:27  myc
 * Remove the camp event flag when leaving the game for proper clean-up.
 *
 * Revision 1.222  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.
 *
 * Revision 1.221  2008/03/10 18:01:17  myc
 * Instead of taking off the berserk effect in multiple places, like
 * when camping, we'll do it when saving the player.
 *
 * Revision 1.220  2008/03/08 22:29:06  myc
 * Moving shapechange and chant to the cooldown system.
 *
 * Revision 1.219  2008/03/07 21:21:57  myc
 * Replaced action delays and skill delays with a single list of
 * 'cooldowns', which are decremented by a recurring event and
 * also save to the player file.
 *
 * Revision 1.218  2008/03/06 05:11:51  myc
 * Combined the 'saved' and 'unsaved' portions of the char_specials and
 * player_specials structures by moving all fields of each saved structure
 * to its parent structure.  Also combined the skills array from the
 * player and mob structures since they are identical.
 *
 * Revision 1.217  2008/03/05 05:21:56  myc
 * Got rid of old save_char_file_u declaration.
 *
 * Revision 1.216  2008/03/05 03:03:54  myc
 * Changed the spell memory structures.  Also new ascii pfiles are in,
 * so the do_save and do_title commands have been updated.
 * Also made the default prompts (that were in the do_display command)
 * constants in constants.c.
 *
 * Revision 1.215  2008/02/24 17:31:13  myc
 * Added an OLCComm toggle which allows you to receive communication
 * while in OLC.  Also a NoClanTell toggle for members of a clan.
 *
 * Revision 1.214  2008/02/11 21:04:01  myc
 * Make the do_not_here command placeholder for spec-procs give a little
 * more information on /why/ you cannot do that here.
 *
 * Revision 1.213  2008/02/10 20:28:42  jps
 * Use correct method of getting quest name
 *
 * Revision 1.212  2008/02/10 20:19:19  jps
 * Further quest numbering tweaks/fixes.
 *
 * Revision 1.211  2008/02/10 19:43:38  jps
 * Subclass quests now store the target subclass as a quest variable rather
 * than as 3 bits in the quest id.
 *
 * Revision 1.210  2008/02/09 21:57:13  myc
 * The hide command won't cause hide points to go negative anymore.
 *
 * Revision 1.209  2008/02/09 21:07:50  myc
 * Must provide a boolean to event_create saying whether to
 * free the event obj when done or not.
 *
 * Revision 1.208  2008/02/09 18:29:11  myc
 * The event code now handles freeing of event objects.  The camp
 * event now uses an event flag instead of storing the camp event
 * in a special field.
 *
 * Revision 1.207  2008/02/09 06:19:44  jps
 * Add "nohints" toggle for whether you receive command suggestions
 * after entering a typo.
 *
 * Revision 1.206  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.205  2008/02/02 19:38:20  myc
 * Adding 'permanent titles' to players, so they can switch between
 * any of the titles they've earned.
 *
 * Revision 1.204  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.203  2008/01/27 21:09:12  myc
 * Quitting or camping removes berserk status and all rage from a
 * character.  Replaced hit() references with TYPE_UNDEFINED with
 * attack().
 *
 * Revision 1.202  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species
 *into races.
 *
 * Revision 1.201  2008/01/26 20:44:39  myc
 * Fix crash bug because of uninitialized memory in do_shapechange.
 *
 * Revision 1.200  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.199  2008/01/26 12:30:19  jps
 * Use skills.h to import improve_skill().
 *
 * Revision 1.198  2008/01/23 14:13:54  jps
 * Added function creature_allowed_skill(), so the shapechanged critters
 * can have certain skills that override the question of whether a
 * non-humanoid should be allowed to have it.
 *
 * Revision 1.197  2008/01/23 05:13:37  jps
 * Add the point command, which is used to unhide someone you've spotted.
 *
 * Revision 1.196  2008/01/10 06:46:53  myc
 * Moved the messages for the lay hands skill into the messages file.
 * The new healing message in the message struct allows lay hands to
 * simply use damage() instead of copying its functionality.
 *
 * Revision 1.195  2008/01/10 05:39:43  myc
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.194  2008/01/06 23:50:47  jps
 * Added spells project and simulacrum, and MOB2_ILLUSORY flag.
 *
 * Revision 1.193  2008/01/05 06:17:36  myc
 * Fixing the valid prompt range for do_display.
 *
 * Revision 1.192  2008/01/05 05:42:30  jps
 * Changed name of save_char() to save_player().
 *
 * Revision 1.191  2008/01/04 05:17:52  jps
 * Provide some feedback when you don't get the shapechange
 * creature you requested.
 *
 * Revision 1.190  2008/01/04 05:14:52  jps
 * Exempt gods from align restrictions when shapechanging.
 *
 * Revision 1.189  2008/01/04 01:43:10  jps
 * Removed unused function do_practice.
 *
 * Revision 1.188  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.187  2008/01/01 03:05:20  jps
 * Add the year to timestamps on entries in ideas, bugs, typos, and notes.
 *
 * Revision 1.186  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.185  2007/12/21 07:52:11  myc
 * It is no longer possible to damage an immortal with lay hands.
 *
 * Revision 1.184  2007/12/20 23:09:03  myc
 * Updated do_level to use the new exp_message function that replaced
 * exp_mesg.  do_display now offers two new prompts; the old 'Complete'
 * is now 'Standard', and there are two new 'Complete' prompts.  Also
 * fixed a memory leak in do_display.
 *
 * Revision 1.183  2007/12/20 17:18:21  myc
 * You can no longer steal in the PK arena.
 *
 * Revision 1.182  2007/12/19 20:36:16  myc
 * save_char() no longer requires you to specify the save room (which
 * wasn't being used anyway).  Renamed the 'Cloaked' toggle to 'RoomVis'.
 *
 * Revision 1.181  2007/11/18 16:51:55  myc
 * Improve steal chance when hidden.
 *
 * Revision 1.180  2007/10/27 18:15:10  myc
 * Fixing hide calculation to use dex and int right.
 *
 * Revision 1.179  2007/10/12 20:32:22  jps
 * Apply laryngitis to petitioning.
 *
 * Revision 1.178  2007/10/11 20:14:48  myc
 * Removed all chant commands from here, and merged its functionality with
 * the magic system.  do_cast now covers the chant command via SCMD_CHANT.
 * The songs command is now in act.informative.c.  The actual chants are
 * in magic.c, except peace which is now a manual "spell".
 *
 * Revision 1.177  2007/10/09 02:38:00  myc
 * When a druid shapechanges back to their player form, any objects or
 * money the druid had is transfered to the player.
 *
 * Revision 1.176  2007/10/02 02:52:27  myc
 * Added a forward pointer on the char_data struct so we can check to see
 * who someone is shapeshifted/switched into.  Shapechanged druids will
 * be able to see how long they have been shapechanged/how long until they
 * can shapechange again.  The timer isn't a static 5 tics anymore;
 * instead it's variable based on the time spent shapechanged.  Fixed some
 * typos.  Added lag to steal command.  Moved do_grep and do_report to
 * act.comm.c.  Toggle command can now be used while shapechanged, and it
 * applies to the original player character.  Petition now goes to the
 * player's descriptor even when they are shapechanged.  Cleaned up
 * various other commands.
 *
 * Revision 1.175  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  AFF_HIDE, AFF_SNEAK, and ITEM_HIDDEN
 * are now unused.  Sneak chance is now calculated based on hiddenness.
 * do_sneak is gone, and do_hide has changed.  Also got rid of toggle
 * nonamebroadcast, and replaced it with toggle cloaked for imms, which
 * lets you appear visible to people in the room, but not everyone else.
 *
 * Revision 1.174  2007/09/15 15:36:48  myc
 * Natures embrace now sets camouflage bit, which lets you be hidden as long
 * as you are outside.
 *
 * Revision 1.173  2007/09/12 22:23:04  myc
 * Removed autoexit set from shapechange since it's set in db.c now.
 *
 * Revision 1.172  2007/09/11 19:44:48  myc
 * Gods can turn into aquatic creatures whereever they please.
 *
 * Revision 1.171  2007/09/11 19:41:51  myc
 * Renamed 'sting ray' as 'stingray'
 * /s
 *
 * Revision 1.170  2007/09/11 16:34:24  myc
 * Rewrote shapechange command, added 9 new aquatic mobs, and animal
 * types (damager, tank, travel, tracker).  Certain shapechange mobs now
 * also have special skills.
 *
 * Revision 1.169  2007/09/03 23:59:43  jps
 * Added macro ADDED_WEIGHT_OK for testing whether a char can have an
 * object added to its inventory.  Avoids an integer overflow problem
 * that could occur if an object's weight was near maxint.
 *
 * Revision 1.168  2007/09/03 22:44:09  jps
 * Fix douse message when a fountain is used.  Make douse automatically
 * successful when in a water room of any kind, or a swamp or beach room.
 *
 * Revision 1.167  2007/08/27 21:18:00  myc
 * You can now queue up commands while casting as well as abort midcast.
 * Casting commands such as look and abort are caught and interpreted
 * before the input is normally queued up by the game loop.
 *
 * Revision 1.166  2007/08/26 01:55:41  myc
 * Fire now does real damage.  All fire spells have a chance to catch the
 * victim on fire.  Mobs attempt to douse themselves.
 *
 * Revision 1.165  2007/08/18 21:23:40  jps
 * Remove a lot of redundant code from do_shapechange. Changed syntax so that
 * shapechange only takes one argument: class (mammal, bird, reptile) or
 * the name of the creature.
 *
 * Revision 1.164  2007/08/16 11:52:47  jps
 * Remove defunct do_old_subclass. The magic of RCS means you can find it
 * again if you REALLY want to.
 *
 * Revision 1.163  2007/08/14 22:43:07  myc
 * Adding 'stealth' skill as secondary effect for sneak.  Stealth
 * makes a person untrackable and invisible on 'who -z'.
 * Cleaned up hide code.
 *
 * Revision 1.162  2007/08/14 10:43:11  jps
 * Laryngitis will prevent you from consenting, because the consent command
 * is one way to pester people at a distance.
 *
 * Revision 1.161  2007/08/05 20:21:51  myc
 * Cleaned up bandage code.
 *
 * Revision 1.160  2007/08/05 01:50:16  myc
 * Follow, guard, and consent all follow similar usage patterns now.
 * <action> off/self/me stops whatever it is, <action> target starts
 * on the target, and <action> shows who is currently targeted.
 *
 * Revision 1.159  2007/08/04 21:13:10  myc
 * Cleaned up layhands code.
 *
 * Revision 1.158  2007/08/03 22:00:11  myc
 * Fixed several \r\n typos in send_to_chars.
 *
 * Revision 1.157  2007/08/03 03:51:44  myc
 * check_pk is now attack_ok, and covers many more cases than before,
 * including peaced rooms, shapeshifted pk, and arena rooms.  Almost all
 * offensive attacks now use attack_ok to determine whether an attack is
 * allowed.
 *
 * Revision 1.156  2007/07/18 23:07:10  jps
 * do_consent will inform you when you're trying to consent the person
 * you've already consented, rather than un-consenting and reconsenting.
 *
 * Revision 1.155  2007/07/18 16:50:09  jps
 * Oops... forgot to free the event when someone camped, then
 * mounted something, then was stopped from camping. Ok, fixed.
 *
 * Revision 1.154  2007/07/14 02:16:22  jps
 * Make certain that the level of a summoned mount is low enough
 * that the summoner may easily ride it.
 *
 * Revision 1.153  2007/07/14 00:37:56  jps
 * Make characters save after changing their title.
 *
 * Revision 1.152  2007/07/14 00:05:49  jps
 * Allow grouping of your pets.
 *
 * Revision 1.151  2007/07/02 04:58:22  jps
 * Stop counting items in inventory at the *end* of the camp event.
 *
 * Revision 1.150  2007/07/02 04:55:03  jps
 * Made the steal skill only improve when the victim is awake.
 *
 * Revision 1.149  2007/06/25 06:23:08  jps
 * Prevent camping while mounted.
 *
 * Revision 1.148  2007/06/24 01:16:55  jps
 * Add spaces to the end of the predetermined prompts, now that the code
 * doesn't automatically add one.
 *
 * Revision 1.147  2007/06/03 03:28:21  jps
 * Fixed typo in "first aid".
 *
 * Revision 1.146  2007/05/28 07:03:17  jps
 * Cause prompt-set feedback to escape color codes.
 *
 * Revision 1.145  2007/05/28 05:38:25  jps
 * Setting up camp sends message to the room.
 *
 * Revision 1.144  2007/04/25 08:04:41  jps
 * Tell little rogues they'll need more experience to sneak.
 *
 * Revision 1.143  2007/04/25 07:53:01  jps
 * Allow 'visible' to properly terminate hiding.
 *
 * Revision 1.142  2007/04/19 00:53:54  jps
 * Create macros for stopping spellcasting.
 *
 * Revision 1.141  2007/04/18 00:32:01  myc
 * Camping now removes the meditate flag to prevent syserrs when chars
 * log back on.  Also, changed the way first aid rolls dice to save some
 * clock cycles - the average values are still the same.
 *
 * Revision 1.140  2007/04/18 00:18:48  myc
 * Rewrote 'subclass' command to hopefully be less buggy.  It at least has
 * better messages now.
 *
 * Revision 1.139  2007/04/18 00:05:59  myc
 * Prompt parser has been totally rewritten so it won't print garbage
 * characters anymore.  Also, some new features were added.  Giving the
 * prompt command back to mortals.
 *
 * Revision 1.138  2007/04/11 14:24:00  jps
 * Fix warning about function get_check_money.
 *
 * Revision 1.137  2007/04/11 14:15:28  jps
 * Give money piles proper keywords and make them dissolve when stolen.
 *
 * Revision 1.136  2007/04/11 09:57:55  jps
 * Fix formatting of stolen money string.
 *
 * Revision 1.135  2007/03/27 04:27:05  myc
 * Fixed typos in hide, steal, and toggle.  Revamped summon mount to offer
 * several different mounts based on alignment and level.  Lay hands will
 * heal an anti-paladin with vamp touch now.
 *
 * Revision 1.134  2007/02/04 18:12:31  myc
 * Page length now saves as a part of player specials.
 *
 * Revision 1.133  2006/12/08 05:06:58  myc
 * Removed coins enum from do_steal, now constants in structs.h.
 *
 * Revision 1.132  2006/11/27 02:26:45  jps
 * Let people see consent messages even when asleep.
 *
 * Revision 1.131  2006/11/27 02:18:42  jps
 * Let imms camp in air rooms.
 *
 * Revision 1.130  2006/11/21 20:53:52  jps
 * Align values in group printout
 *
 * Revision 1.129  2006/11/20 22:24:17  jps
 * End the difficulties in interaction between evil and good player races.
 *
 * Revision 1.128  2006/11/18 06:41:16  jps
 * Fixed typos when toggling anonymous.
 *
 * Revision 1.127  2006/11/16 18:42:45  jps
 * Awareness of new surroundings when magically tranported is related to
 * being asleep, blindness, etc.
 *
 * Revision 1.126  2006/11/14 21:30:44  jps
 * Stop invis'd gods from being seen by lower level imms as they camp
 *
 * Revision 1.125  2006/11/13 16:34:43  jps
 * You can't steal items over your level any more.
 * Blind mobs are no longer immune from stealing.
 *
 * Revision 1.124  2006/11/12 02:31:01  jps
 * You become unmounted when magically moved to another room.
 *
 * Revision 1.123  2006/11/08 08:34:03  jps
 * Fix gender of pronoun when trying to group someone without consent.
 *
 * Revision 1.122  2006/11/08 08:03:47  jps
 * Typo fix "You better leave art to the thieves." ->
 * "You'd better leave that art to the rogues."
 *
 * Revision 1.121  2006/11/07 09:51:48  jps
 * Allow wands and staves held in second hand to be used.
 *
 * Revision 1.120  2006/11/07 09:35:38  jps
 * Stop sending spurious "You petition, ''" when empty petition sent.
 *
 * Revision 1.119  2006/07/20 07:41:07  cjd
 * Typo fixes.
 *
 * Revision 1.118  2006/04/28 21:14:02  mud
 * Layhands patch, with silent damage so it isn't pulling from
 * messages file and shorter colorized messages.
 *
 * Revision 1.117  2006/04/28 08:37:02  mud
 * Made some quick changes for testing purposes but still can't
 * seem to find where layhands is going awry. - RLS
 *
 * Revision 1.116  2006/04/28 01:41:12  rls
 * Frog!  evil and !undead check for layhands... whee!
 *
 * Revision 1.115  2006/04/28 01:26:58  rls
 * More todo with layhands.
 *
 * Revision 1.114  2006/04/28 00:27:33  rls
 * Fix to layhands, bad argument checking and added
 * healing effect for antis laying undead... fun!
 *
 * Revision 1.113  2005/06/16 02:07:07  cjd
 * adjusted the minimum level to subclass from 5 to 10
 *
 * Revision 1.112  2005/06/10 18:07:16  cjd
 * oops, had to fix a PK check for pally's. also
 * included a fix for damage amounts shown in the
 * case of healing.
 *
 * Revision 1.111  2005/06/09 21:59:17  cjd
 * Fixed error where vict was not being defined before usage
 *
 * Revision 1.110  2005/03/30 18:34:16  rls
 * Added missing external declaration for pk_allowed
 *
 * Revision 1.109  2005/03/24 02:54:14  djb
 * Added PK checks for layhands.
 *
 * Revision 1.108  2005/02/14 10:17:28  rls
 * Fixed missing damage messages in layhands function, as well
 * made it to where anti's don't heal anything.  Still needs
 * to have pk / charmed mobs by PC safeguards in place.
 *
 * Revision 1.107  2005/02/14 02:35:29  djb
 * Changed the consent/group/mgroup functions to allow for good/evil race
 *groups. Changed layhands around, and added layhands for anti-paladins. Also
 *added code for a new spell control_undead, but left it commented out for now
 *so that I can finish it up later.
 *
 * Revision 1.106  2004/11/15 01:03:37  rsd
 * Added code from Acerite to add a save all option to save
 * every player online without a force all save.
 *
 * Revision 1.105  2003/07/27 01:22:02  jjl
 * A fix to prevent crashes caused by camping and then memorizing.
 *
 * Revision 1.104  2003/07/15 02:32:04  jjl
 * Fixed lay hands.
 *
 * Revision 1.103  2003/06/28 03:08:09  jjl
 * Added back messages for paladins, since they don't go through damage.  Added
 *a no-argument check .  Both for layhands that is
 *
 * Revision 1.102  2003/06/25 04:52:22  jjl
 * Updated lay hands to use skill messages.
 *
 * Revision 1.101  2003/06/25 03:32:34  jjl
 * *** empty log message ***
 *
 * Revision 1.100  2003/06/25 03:28:16  jjl
 * Fixed the delay on lay hands.  Got that was dumb.
 *
 * Revision 1.99  2003/06/25 02:21:03  jjl
 * Revised lay hands to not suck.
 *
 * Revision 1.98  2003/06/23 02:55:02  jjl
 * Added failure message to guard, moved the skill improvement.
 *
 * Revision 1.97  2003/06/23 01:47:09  jjl
 * Added a NOFOLLOW flag, and the "note" command, and show notes <player>
 *
 * Revision 1.96  2003/06/21 03:18:29  jjl
 * *** empty log message ***
 *
 * Revision 1.95  2003/06/21 01:01:08  jjl
 * Modified rogues.  Removed circle - backstab is now circlicious.  Updated
 * damage on backstab to give a little more pop.  Throatcut is now a once a day.
 *
 * Revision 1.94  2003/04/03 01:05:17  jjl
 * Changed the stun on hide, because it was like 16 seconds before.
 *
 * Revision 1.93  2002/12/04 09:03:55  rls
 * changed max_group_diff to an external call for max group level diff code
 *
 * Revision 1.92  2002/12/04 06:11:09  jjl
 * Well, that was dumb.  In group "all", it was checking to see if there was a
 * level difference as a prerequisite for checking the range.  It should have
 * been checking if the max range was nonzero instead.
 *
 * Revision 1.91  2002/11/30 19:39:38  jjl
 * Added the ability for GAME commands to have integer values, and added a
 * GROUPING game command, that allows you to set a maximum level difference
 * between group masters and potential groupees..
 *
 * Revision 1.90  2002/10/19 18:29:21  jjl
 * Recall spec procs.  This file has the event handler for it
 *
 * Revision 1.89  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.88  2002/05/18 12:57:55  dce
 * Fixed it so rhino's are a lower level...same problem
 * as anaconda's
 *
 * Revision 1.87  2002/05/13 21:20:26  dce
 * Fixed that anaconda problem where it would make you level 100.
 *
 * Revision 1.86  2002/05/05 22:04:28  mpg
 * modified "report" so you can no longer see (wiz)invizies names when they
 *report
 *
 * Revision 1.85  2002/04/26 21:36:30  mpg
 * when grouped with wizesies, you no longer get info typing "group"
 * unless you've got their consent.
 *
 * Revision 1.84  2002/04/26 20:22:21  mpg
 * modified "chant" to work show correct gender in messages.
 *
 * Revision 1.83  2002/04/26 18:54:28  mpg
 * modified "group" so group info can be accessed while sleeping.
 * forming of groups etc. still restricted to POS_RESTING and up.
 * NB: while .
 *
 * Revision 1.82  2002/02/25 10:59:41  rls
 * Added apostrophe and punctuation to display string in do_report
 *
 * Revision 1.81  2001/12/07 02:09:56  dce
 * Linkdead players will now lose exp when they die.
 * Linkdead shapechanged players will now shapechange
 * to their original form before linking out.
 *
 * Revision 1.80  2001/11/15 19:52:48  rjd
 * Plural rules fixed for splitting coins.
 *
 * Revision 1.79  2001/11/15 18:04:38  rjd
 * Coin splitter message for the splitter corrected.
 *
 * Revision 1.78  2001/10/17 21:04:05  rjd
 * Tweaked "split" command so that splitter (giver) now sees how much of the
 * split currency (s)he kept. Also cleaned up the message to the receivers,
 * which had a spelling error (was "recieve", now is correctly "receive").
 *
 * Revision 1.77  2001/10/12 21:21:12  rjd
 * Extension of the "save" command: Gods can now type "save <playername>" to
 *save a player to file. This works on any player loaded into the game, whether
 *by a player logging in normally or a god linkloading the player.
 *
 * Revision 1.76  2001/04/06 00:58:41  dce
 * Subclassing should show in the syslog.
 *
 * Revision 1.75  2001/04/02 23:31:21  dce
 * Put a level restriction on potions and wands, etc...
 *
 * Revision 1.74  2001/04/01 21:59:31  mtp
 * changed CLASS_THIEF to CLASS_ROGUE, since rogue is teh base not thief
 *
 * Revision 1.73  2001/03/25 00:44:46  dce
 * Bug/Idea/Typo gives better output
 *
 * Revision 1.72  2001/03/13 20:52:09  dce
 * Made the summonable mounts more sane. No longer a level 70+
 * horse with 1500+ hps.
 *
 * Revision 1.71  2001/03/07 01:45:18  dce
 * Added checks so that players can not kill shapechanged players and
 * vise versa. Hopefully I didn't miss any...
 *
 * Revision 1.70  2001/03/04 17:33:19  dce
 * Shapechange does not ding you a hp.
 *
 * Revision 1.69  2001/03/03 18:07:10  dce
 * Gods should not fail specific shapechanges.
 *
 * Revision 1.68  2001/02/24 16:47:57  dce
 * Phase 3 of shapechange.
 *
 * Revision 1.67  2001/02/24 04:04:05  dce
 * Phase 2 of shapechange
 *
 * Revision 1.66  2001/02/21 01:06:19  dce
 * Phase 1 of the shapechange re-write
 *
 * Revision 1.65  2001/02/12 23:22:42  mtp
 * min level for subclass changed to 5
 *
 * Revision 1.64  2001/02/11 22:29:39  rsd
 * disabled the bind command
 *
 * Revision 1.63  2001/02/03 00:56:51  mtp
 * do a race check before starting subclass quest
 * also returing different codes so that calling procs can do something sensible
 * on failure
 *
 * Revision 1.62  2001/02/01 02:36:21  dce
 * Somone was sending a buf to a char without ever putting anything
 * in the buf...in the disband command. It is now fixed.
 *
 * Revision 1.61  2001/01/16 00:33:56  mtp
 * make sure spell/skill list is clean after subclass
 *
 * Revision 1.60  2001/01/10 23:30:24  mtp
 * cant camp with more than 50 items
 *
 * Revision 1.59  2000/11/26 10:03:57  jimmy
 * Fixed do_sneak.  The affectation was not being properly initilized.
 * Added init to 0 for aff2/aff3.  This fixes the do_stat_character
 * bug of a ch who's sneaking.
 *
 * Revision 1.58  2000/11/20 04:21:41  rsd
 * Added all i freaking zillion back rlog messages from
 * prior to the addition of the $log$ string. phew.
 *
 * Revision 1.57  2000/11/07 01:42:04  mtp
 * changes to do_subclass to use the subclass quest style
 * old method (using can_subclass_plyr saved as do_old_subclass in case it
 *doesnt work)
 *
 * Revision 1.56  2000/10/25 23:43:54  rsd
 * Fixed the if_pkill check on psteal to allow
 * mobiles to steal from players, DOH.
 *
 * Revision 1.55  2000/10/21 12:05:57  mtp
 * improved subclass code, now looks for cvariable can_subclass_<name> =
 *<subclass> to allow a QM to be waiting for multiple potential finishers
 *
 * Revision 1.54  2000/10/19 01:54:08  mtp
 * added new subclass code, dependent on two variable (global set on mob)
 *use_subclass and can_subclass, if the player with name == can_subclass types
 *subclass in the room with a teacher mob, and the value of use_subclass is
 *valid for that user, then they are subclassed.
 *
 * Revision 1.53  2000/10/13 17:50:39  cmc
 * re-instituted modified level command
 * required to re-implement "level gain"
 *
 * Revision 1.52  2000/09/28 03:22:09  rsd
 * made no gossip level 0
 *
 * Revision 1.51  2000/06/04 03:59:07  rsd
 * altered summon mount so the mounts increase in strength as
 * the pally's increase in level and align.
 * The algorythm could use some tweaking, I'm open to suggestion
 *
 * Revision 1.50  2000/05/21 23:55:59  rsd
 * Altered do_prompt to point mortals at the display command.
 *
 * Revision 1.49  2000/05/10 22:09:24  rsd
 * added a check to do_douse to check for fountains
 * in the room.  If there is a fountain in the room
 * the player automatically gets doused w/o any checks.
 * Cool eh? :)
 *
 * Revision 1.48  2000/04/22 22:30:04  rsd
 * Changed the comment header, also retabbed and braced sections of the code.
 * Altered toggle to show the numbers associated with wimpy and pagelength.
 * Also, toggle will return a message to the player if the toggle with a
 * bogus argument.
 *
 * Revision 1.47  2000/04/17 00:52:33  rsd
 * removed mana from do_display and made do prompt level 101
 * or higher.
 *
 * Revision 1.46  2000/03/08 11:14:42  cso
 * do_steal only checked for gold. made it check all 4 coin types.
 *
 * Revision 1.45  1999/12/13 05:16:49  cso
 * made do_steal check the pk_allowed game setting
 *
 * Revision 1.44  1999/11/28 22:44:48  cso
 * do_prompt: commented out a CREATE that was causing a mem leak
 *
 * Revision 1.43  1999/10/06 17:55:24  rsd
 * Removed the AFK code from the end of the prompt and moved
 * it to comm.c
 *
 * Revision 1.42  1999/09/16 01:47:17  dce
 * Song levels moved up.
 *
 * Revision 1.41  1999/09/14 00:27:17  dce
 * Monks hit and dam toned down.
 *
 * Revision 1.40  1999/09/10 00:49:52  mtp
 * an[4~[4~bandage can KILL now if done badly for too long
 *
 * Revision 1.39  1999/09/10 00:01:53  mtp
 * added a delay (for mortals) to bandage
 *
 * Revision 1.38  1999/09/07 23:26:53  mtp
 * removed mana from 'print_group' output
 *
 * Revision 1.37  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.36  1999/09/05 05:04:43  mud
 * reverted to previous version to remove compile errors until they can be fixed
 *
 * Revision 1.35  1999/09/03 23:22:50  mtp
 * remove mana from group output
 *
 * Revision 1.34  1999/09/03 23:06:54  mtp
 * added some IS_FIGHTING checks
 *
 * Revision 1.33  1999/08/13 15:31:01  dce
 * Allow camping to be seen in normal syslog.
 *
 * Revision 1.32  1999/08/12 17:54:46  dce
 * Fixed experience so that there are no overflows of integers that are placed
 *into longs. Main problem was max_exp_gain and max_exp_loss. Both were
 *overflowing due to poor Hubis coding.
 *
 * Revision 1.31  1999/08/10 20:51:42  dce
 * Qucik
 * Quick fix to a bug where players can get a free rippost
 * if they don't even have the skill.
 *
 * Revision 1.30  1999/08/09 22:37:11  mtp
 * Added <AFK> to players prompt if toggled AFK (includes changes of prompt)
 *
 * Revision 1.29  1999/07/24 21:01:37  dce
 * Sublass function
 *
 * Revision 1.28  1999/07/23 01:57:18  mud
 * fixed do gen toggle, thanks
 * fixed from printing double line at end, removed ooc and slowns
 * as well as case sensitivity. I'm so insensitive.
 *
 * Revision 1.27  1999/07/10 05:51:23  mud
 * Ok, removed the OOC from being a toggle, commented it out of
 * certain sections, and just cut it out of other parts of do_toggle.
 *
 * Revision 1.26  1999/07/09 21:00:06  mud
 * moved the ascii terminator in the toggle display and
 * added a header line above the cute format characters.
 *
 * Revision 1.25  1999/07/07 23:21:45  jimmy
 * Fixed do_group to check for consent of following npc's.  This was an
 * oversight on my part when fixing the command in the last ci.
 * gurlaek
 *
 * Revision 1.24  1999/07/07 15:46:44  jimmy
 * fixed the problems with trophys not being updated when in a group.
 * This was A bug created by my cut and pasting when i fixed the
 * void * function pointer warnings. All better now :)
 * --gurlaek 7/7/1999
 *
 * Revision 1.23  1999/07/06 02:53:50  mud
 * Completely reworked the toggle command to make it easier to
 * read.  It's now 3 columns of stuff instead of that blue
 * one line stuff that scrolled off the screen.
 * Gurlaek showed me how to do it.
 *
 * Revision 1.22  1999/06/11 16:56:55  jimmy
 * Ok, fixed do_quit to check for fighting and also not crash when mortally
 * wounded.  This was done in die() by checking for killer=NULL.
 * since no one killed you if you quit while morted the die code
 * didn't know how to deal with a NULL killer.
 * --Gurlaek 6/11/1999
 *
 * Revision 1.21  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list.  This code compiles fine under both gcc RH5.2 and egcs RH6.0
 *
 * Revision 1.20  1999/05/26 01:50:28  mud
 * made summon mount level 15 instead of 25 in the
 * if (GET_LEVEL(ch) < XX) statement
 *
 * Revision 1.19  1999/05/04 19:41:51  dce
 * Fixed some typos
 *
 * Revision 1.18  1999/05/02 17:10:17  mud
 * made toggeling of autoexits level 0 as opposed to
 * LVL_IMMORT or whatever it was before.
 *
 * Revision 1.17  1999/05/01 18:01:21  dce
 * Allow players to drop all eq and quit.
 *
 * Revision 1.16  1999/04/29 19:02:04  mud
 * made anonymous level 50
 *
 * Revision 1.15  1999/04/23 01:51:22  jimmy
 * fixed camp crashbug.  This occured when someone type camp twice.
 * I added checks to EVENTFUNC(camp_event) to determine if someone was
 * already camping, and also NULLed freed pointers SCREAM!.  Seems to work
 * fine.  --Gurlaek.
 *
 * Revision 1.14  1999/04/16 03:55:09  dce
 * Removed some things temporarly until they can be fixed.
 *
 * Revision 1.13  1999/03/30 18:59:31  jen
 * Fixed a toggle bug... JEN II
 * Was allowing proper lvl players to toggle, but not displaying
 * anything except stuff available to ppl BELOW your lvl
 *
 * Revision 1.12  1999/03/26 19:44:35  jen
 * Added a mortal gossip channel with 103+ godly control
 *
 * Revision 1.11  1999/03/08 21:09:52  dce
 * Adjusts chant semantics
 *
 * Revision 1.10  1999/03/08 04:47:16 dce
 * Chant semantics added.
 *
 * Revision 1.9  1999/03/07 05:01:09  dce
 * Chant finishes and wearoff messages.
 *
 * Revision 1.8  1999/03/06 23:51:54  dce
 * Add's chant songs, and can only chant once every four hours
 *
 * Revision 1.7  1999/03/05 20:02:36  dce
 * Chant added to, and songs craeted
 *
 * Revision 1.6  1999/02/26 22:30:30  dce
 * Added skeleton for chant skill
 *
 * Revision 1.5  1999/02/20 18:41:36  dce
 * Adds improve_skill calls so that players can imprve their skills.
 *
 * Revision 1.4  1999/02/10 05:57:14  jimmy
 * Added long description to player file.  Added AFK toggle.
 * removed NOAUCTION toggle.
 * fingon
 *
 * Revision 1.3  1999/02/06 17:27:25  jimmy
 * God loadrooms now set permanently until changed.
 * if you drop link however you come back where you
 * dropped.
 *
 * Revision 1.2  1999/02/03 22:54:08  jimmy
 * removed some toggles from mortal chars
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial Revision
 *
 ***************************************************************************/
