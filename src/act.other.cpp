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
#include "logging.hpp"
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
            char_printf(ch, "You step out of the shadows.\n");
        } else {
            act("$n snaps into visibility.", false, ch, 0, 0, TO_ROOM);
            char_printf(ch, "You fade back into view.\n");
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
        char_printf(ch, "You don't have the protective skill required to guard.\n");
        return;
    }

    if (!*arg) {
        if (ch->guarding)
            act("You are guarding $N.", false, ch, 0, ch->guarding, TO_CHAR);
        else
            char_printf(ch, "You are not guarding anyone.\n");
        return;
    }

    if (!strcasecmp(arg, "off"))
        vict = ch;
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        char_printf(ch, "That person is not here.\n");
        return;
    }

    if (ch == vict) {
        if (ch->guarding)
            stop_guarding(ch);
        else
            char_printf(ch, "You are not guarding anyone.\n");
        return;
    }

    if (vict->guarded_by) {
        if (vict->guarded_by == ch)
            char_printf(ch, "You are already guarding that person.\n");
        else
            char_printf(ch, "Someone else is already guarding that person.\n");
        return;
    }

    if (ch->guarding) {
        if (ch->guarding == vict) {
            char_printf(ch, "You are already guarding that person.\n");
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
        char_printf(ch, "NPCs don't subclass.  Ha.\n");
        return;
    }

    c = &(classes[(int)GET_CLASS(ch)]);

    /* If not a base class, then bail */
    if (c->is_subclass) {
        char_printf(ch, "You can only subclass once!\n");
        return;
    }

    /* If below minimum quest level, bail */
    if (GET_LEVEL(ch) < 10) {
        char_printf(ch, "You need to be level 10 before you can subclass!\n");
        return;
    }

    /* If above maximum quest level, bail */
    if (GET_LEVEL(ch) > c->max_subclass_level) {
        char_printf(ch, "You can no longer subclass, because you are over level {}.\n", c->max_subclass_level);
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
                if (!anyvalid) {
                    char_printf(ch, "You may choose from the following classes for your race:\n");
                    anyvalid = 1;
                }
                char_printf(ch, "  {}\n", classes[subclass].fmtname);
            }
        }

        if (anyvalid) {
            char_printf(ch, "You have until level {} to subclass. See HELP SUBCLASS_{} for more information.\n",
                        c->max_subclass_level, capitalize(c->name));
        } else
            char_printf(ch, "There are no subclasses available to you.\n");
        return;
    }

    /* Now we know the player has started the subclass quest. */

    /* Make sure the class --> subclass change is possible */
    subclass =
        parse_class(0, 0, get_quest_variable(ch, all_quests[real_quest(quest->quest_id)].quest_name, "subclass_name"));
    if (subclass == CLASS_UNDEFINED) {
        log("{} finished subclass quest \"{}\" with unknown target subclass \"{}\"", GET_NAME(ch),
            all_quests[real_quest(quest->quest_id)].quest_name,
            get_quest_variable(ch, all_quests[real_quest(quest->quest_id)].quest_name, "subclass_name"));
        char_printf(ch, "There is an error in your subclass quest.  Ask a god to reset it.\n");
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

        char_printf(ch, "You have successfully subclassed as {}!\n", with_indefinite_article(CLASS_FULL(ch)));
        all_except_printf(ch, "{} has subclassed to {}!\n", GET_NAME(ch), CLASS_FULL(ch));
        log(LogSeverity::Stat, LVL_GOD, "{} has subclassed to {}", GET_NAME(ch), CLASS_FULL(ch));
        return;
    }

    /* Now we know the player is on a subclass quest, but not completed */
    char_printf(ch, "You are on the way to becoming a {}\n", classes[(int)subclass].fmtname);
    char_printf(ch, "You have until level {:d} to complete your quest.\n", c->max_subclass_level);
}

ACMD(do_quit) {
    int i;
    ObjData *money, *obj;
    one_argument(argument, arg);

    if (IS_NPC(ch) || !ch->desc) {
        char_printf(ch, "You can't quit while shapechanged!\n");
        return;
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        act("$n has left the game.", true, ch, 0, 0, TO_ROOM);
        char_printf(ch, "Goodbye, friend.  Come back soon!\n");
        remove_player_from_game(ch, QUIT_QUITIMM);
        return;
    }

    if (FIGHTING(ch)) {
        char_printf(ch, "No way!  You're fighting for your life!\n");
        return;
    }

    if (subcmd != SCMD_QUIT) {
        char_printf(ch, "For safety purposes, you must type out 'quit yes'.\n");
        char_printf(ch,
                    "Note: You will lose &1&beverything&0 if you quit!  Camping or renting will save everything.\n");
        return;
    }

    if (!*arg || strcasecmp(arg, "yes")) {
        char_printf(ch, "You must type 'quit yes' to leave this world.\n");
        char_printf(ch,
                    "Note: You will lose &1&beverything&0 if you quit!  Camping or renting will save everything.\n");
        return;
    }

    /* Ok, if we've made it this far it's ok to quit */

    if (GET_STANCE(ch) < STANCE_STUNNED) {
        char_printf(ch, "You die before your time...\n");
        act("$n quits the game, but is unable to fend off death...", true, ch, 0, 0, TO_ROOM);
        act("$n is dead!  R.I.P.", true, ch, 0, 0, TO_ROOM);
        die(ch, nullptr);
        return;
    }

    act("$n has left the game.", true, ch, 0, 0, TO_ROOM);

    log(LogSeverity::Stat, std::max<int>(LVL_IMMORT, GET_INVIS_LEV(ch)), "{} has quit the game.", GET_NAME(ch));
    char_printf(ch, "Goodbye, friend.  Come back soon!\n");

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
        char_printf(ch, "You have no idea how to do that!\n");
        return;
    }

    argument = any_one_arg(argument, arg);

    /* If already shapechanged, other rules apply. */
    if (POSSESSED(ch)) {
        if (!*arg) {
            if (ch->char_specials.timer == 0)
                char_printf(ch, "You have just recently taken the form of {}.\n", GET_NAME(ch));
            else if (ch->char_specials.timer == 1)
                char_printf(ch, "You have been in the form of {} for 1 hour.\n", GET_NAME(ch));
            else
                char_printf(ch, "You have been in the form of {} for {:d} hours.\n", GET_NAME(ch),
                            ch->char_specials.timer);
        } else if (!is_abbrev(arg, "me"))
            char_printf(ch, "You cannot shapechange to another animal from this form.\n");
        else {
            if (POSSESSOR(ch)->desc)
                close_socket(POSSESSOR(ch)->desc);

            player = POSSESSOR(ch);

            char_printf(ch, "You quickly morph back to your original self.\n");
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
                SET_COOLDOWN(player, CD_SHAPECHANGE, std::clamp(i, 1, 5) MUD_HR);
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
        char_printf(ch,
                    "You are still drained from your last shapechange.\n"
                    "It will be another {:d} {} before you can change again.\n",
                    i, i == 1 ? "hour" : "hours");
        return;
    }

    if (!*arg) {
        char_printf(ch, "Shapechange to what?\n");
        return;
    }

    if (!strcasecmp(arg, "me")) {
        char_printf(ch, "You are already in your normal form.\n");
        return;
    }

    /* Check alignment. */
    if (GET_LEVEL(ch) < LVL_GOD) {
        if (GET_ALIGNMENT(ch) >= 350) {
            char_printf(ch, "Your good loyalties betray your nature, inhibiting a transformation.\n");
            return;
        } else if (GET_ALIGNMENT(ch) <= -350) {
            char_printf(ch, "Your evil loyalties betray your nature, inhibiting a transformation.\n");
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
                char_printf(ch, "What kind of animal is that?\n");
                return;
            }
        }
        argument = any_one_arg(argument, arg);
    } while (*argument && !i);

    if (IS_SET(type, AQUATIC | FISH) && !IS_WATER(ch->in_room) && GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You won't be able to turn into that here!\n");
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
            creatures[i].midlevel - 5 + random_number(0, abs(GET_ALIGNMENT(ch))) > GET_LEVEL(ch))
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
        char_printf(ch, "You don't know how to shapechange into that.\n");
        return;
    }

    if (desired_index >= 0 && desired_index != index) {
        char_printf(ch, "You didn't feel quite up to changing into {}.\n",
                    with_indefinite_article(creatures[desired_index].name));
    }

    /* Attempt to create the mobile. */
    if (!(mob = read_mobile(creatures[index].vnum, VIRTUAL))) {
        char_printf(ch, "You start to change, then feel ill, and slump back to your normal form.\n");
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: {} tried to shapechange into nonexistent mob prototype V#{:d}",
            GET_NAME(ch), creatures[index].vnum);
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
    GET_LEVEL(mob) = creatures[index].midlevel + std::max(std::clamp(GET_LEVEL(ch) - creatures[index].midlevel, -5, 5),
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
    GET_MAX_HIT(mob) = random_number(creatures[index].minhp, creatures[index].maxhp);
    GET_MAX_MOVE(mob) = random_number(creatures[index].minmv, creatures[index].maxmv);
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
            log(LogSeverity::Stat, std::max<int>(GET_LEVEL(ch), GET_INVIS_LEV(ch)),
                "(GC) {} has saved all players in the realm.", GET_NAME(ch));
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
        char_printf(ch, "No player by the name of {} is currently in the game.\n", arg);
        return;
    }

    if (IS_NPC(target)) {
        char_printf(ch, "You can't save an NPC!\n");
        return;
    }

    if (cmd) {
        if (ch == target)
            char_printf(ch, "Saving {}.\n", GET_NAME(ch));
        else
            char_printf(ch, "You have force-saved {}.\n", GET_NAME(target));
    }
    save_player(target);

    if (ch != target)
        log(LogSeverity::Stat, GET_LEVEL(ch), "(GC) {} has saved {} to file.", GET_NAME(ch), GET_NAME(target));
}

/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here) {
    if (CMD_IS("balance") || CMD_IS("deposit") || CMD_IS("withdraw") || CMD_IS("dump") || CMD_IS("exchange"))
        char_printf(ch, "Sorry, you can only do that in a bank!\n");
    else if (CMD_IS("appear") || CMD_IS("disappear"))
        char_printf(ch, HUH);
    else if (CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive"))
        char_printf(ch, "Sorry, you can only do that in a post office!\n");
    else if (CMD_IS("list") || CMD_IS("value") || CMD_IS("buy") || CMD_IS("sell"))
        char_printf(ch, "Sorry, you can only do that in a shop!\n");
    else if (CMD_IS("rent"))
        char_printf(ch, "Sorry, you can only do that in an inn!\n");
    else
        char_printf(ch, "Sorry, but you cannot do that here!\n");
}

ACMD(do_camp) {
    CampEvent *ce;

    if (FIGHTING(ch) || EVENT_FLAGGED(ch, EVENT_CAMP)) {
        char_printf(ch, "You are too busy to do this!\n");
        return;
    }

    if (IS_NPC(ch) || !ch->desc) {
        char_printf(ch, "You can't camp while shapechanged!\n");
        return;
    }

    /* Restrictions: can't camp inside, in a city, or in water. */
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (CH_INDOORS(ch)) {
            char_printf(ch, "&7You always pitch a tent indoors?&0\n");
            return;
        }

        if (SECT(ch->in_room) == SECT_CITY) {
            char_printf(ch, "&7Ye can't pitch a tent on the sidewalk fool.&0\n");
            return;
        }

        if ((SECT(ch->in_room) == SECT_SHALLOWS) || (SECT(ch->in_room) == SECT_WATER) ||
            (SECT(ch->in_room) == SECT_UNDERWATER)) {
            char_printf(ch, "&7Go buy a floating tent and try again.&0\n");
            return;
            if (SECT(ch->in_room) == SECT_AIR) {
                char_printf(ch, "&7You can't camp in mid-air.&0\n");
                return;
            }
        }
        if (RIDING(ch)) {
            char_printf(ch, "You'd better dismount first.\n");
            return;
        }
    }

    if (GET_STANCE(ch) == STANCE_FIGHTING)
        char_printf(ch, "No way!  You're fighting for your life!\n");
    else if (GET_STANCE(ch) < STANCE_STUNNED)
        char_printf(ch, "It's hard to set your tent up while dying...\n");
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
        room_printf(re->from_room, "The magic of the scroll fizzles as its target has left.\n");
        char_printf(ch, "The magic of the scroll fizzles, as you left the area.\n");
        return EVENT_FINISHED;
    };

    if (IS_NPC(ch) || !ch->desc)
        return EVENT_FINISHED;

    char_printf(ch, "You feel the scroll's energy start to envelop you.\n");
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
        char_printf(ch, "You can't camp while mounted!\n");
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
        char_printf(ch, "You stop meditating.\n&0");
        REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
    }

    act("You complete your campsite, and leave this world for a while.", false, ch, nullptr, nullptr, TO_CHAR);
    if (!GET_INVIS_LEV(ch))
        act("$n rolls up $s bedroll and tunes out the world.", true, ch, 0, 0, TO_ROOM);

    log(LogSeverity::Stat, std::max<int>(LVL_IMMORT, GET_INVIS_LEV(ch)), "{} has camped in {} ({:d}).", GET_NAME(ch),
        world[ch->in_room].name, world[ch->in_room].vnum);

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
            char_printf(ch, "You are free as a bird!\n");
            return;
        }
        prob = random_number(1, 70);
        percent = random_number(20, 101);
        if (prob > percent) {
            char_printf(ch, "You break free from your binds!\n");
            act("$n breaks free from his binds", false, ch, 0, 0, TO_ROOM);
            REMOVE_FLAG(PLR_FLAGS(ch), PLR_BOUND);
            WAIT_STATE(ch, PULSE_VIOLENCE);
            return;
        } else
            WAIT_STATE(ch, PULSE_VIOLENCE * 3);
        return;
    } else {
        if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
            char_printf(ch, "Unbind who?\n");
            return;
        }
        if (vict == ch) {
            prob = random_number(20, 70);
            percent = random_number(1, 101);
            if (prob > percent) {
                char_printf(ch, "You break free from your binds!\n");
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
        char_printf(vict, "You are free of your binds.\n");
    }
}

ACMD(do_bind) {
    CharData *vict;
    ObjData *held = GET_EQ(ch, WEAR_HOLD);
    int prob, percent;

    /* disable this command it's broken and is being used to
       ruin the game for players RSD 2/11/2001 */
    char_printf(ch, "Huh?!?\n");
    return;

    if (FIGHTING(ch)) {
        char_printf(ch, "You are too busy fighting to think about that right now!\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg)
        char_printf(ch, "Bind who?\n");
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))))
        char_printf(ch, "There is no such player.\n");
    else if (IS_NPC(vict))
        char_printf(ch, "You can't do that to a mob!\n");
    else if (GET_LEVEL(vict) > LVL_GOD)
        char_printf(ch, "Hmmm...you'd better not.\n");
    else if (!held)
        char_printf(ch, "You must be holding a rope to tie someone up!\n");
    else {

        if (ch == vict) {
            char_printf(ch, "Oh, yeah, THAT'S real smart...\n");
            return;
        }
        if (PLR_FLAGGED(vict, PLR_BOUND)) {
            char_printf(ch, "Your victim is already tied up.\n");
            return;
        }
        if (GET_OBJ_TYPE(held) != ITEM_ROPE) {
            char_printf(ch, "You must be holding a rope to tie someone up!\n");
            return;
        }
        if (GET_SKILL(ch, SKILL_BIND) == 0) {
            if (GET_STANCE(vict) > STANCE_STUNNED) {
                char_printf(ch, "You aren't skilled enough to tie a conscious person\n");
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
            prob = random_number(1, 50);
            prob += GET_SKILL(ch, SKILL_BIND);
            prob += GET_LEVEL(ch);
            percent = random_number(50, 200);
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
        char_printf(ch, "You abort your spell!&0\n");
        abort_casting(ch);
        if (ch->desc)
            flush_queues(ch->desc);
    } else
        char_printf(ch, "You're not even casting a spell right now.\n");
}

ACMD(do_hide) {
    long lower_bound, upper_bound;
    int skill;

    if (!GET_SKILL(ch, SKILL_HIDE)) {
        char_printf(ch, "You'd better leave that art to the rogues.\n");
        return;
    }

    if (RIDING(ch)) {
        char_printf(ch, "While mounted? I don't think so...\n");
        return;
    }

    if (IS_HIDDEN(ch))
        char_printf(ch, "You try to find a better hiding spot.\n");
    else
        char_printf(ch, "You attempt to hide yourself.\n");

    skill = GET_SKILL(ch, SKILL_HIDE);
    lower_bound = -0.0008 * pow(skill, 3) + 0.1668 * pow(skill, 2) - 3.225 * skill;
    upper_bound = skill * (3 * GET_DEX(ch) + GET_INT(ch)) / 40;
    GET_HIDDENNESS(ch) = random_number(lower_bound, upper_bound) + dex_app_skill[GET_DEX(ch)].hide;

    GET_HIDDENNESS(ch) = std::max(GET_HIDDENNESS(ch), 0l);

    WAIT_STATE(ch, PULSE_VIOLENCE);

    improve_skill(ch, SKILL_HIDE);

    REMOVE_FLAG(EFF_FLAGS(ch), EFF_STEALTH);
    if (GET_SKILL(ch, SKILL_STEALTH)) {
        if (GET_HIDDENNESS(ch) && GET_SKILL(ch, SKILL_STEALTH) > random_number(0, 101))
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
        char_printf(ch, "You can't steal while you are fighting!\n");
        return;
    }
    if (GET_SKILL(ch, SKILL_STEAL) <= 0) {
        char_printf(ch, "You don't know how to steal!\n");
        return;
    }
    if (MOB_FLAGGED(ch, MOB_ILLUSORY)) {
        char_printf(ch, "Being an illusion, you can't steal things.\n");
        return;
    }
    if (!RIGID(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You can't handle objects in your condition.\n");
        return;
    }

    argument = one_argument(argument, obj_name);
    one_argument(argument, vict_name);

    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, vict_name)))) {
        char_printf(ch, "Steal what from who?\n");
        return;
    } else if (vict == ch) {
        char_printf(ch, "Come on now, that's rather stupid!\n");
        return;
    }

    /* Player-stealing is only allowed during PK. */
    if (!attack_ok(ch, vict, false)) {
        char_printf(ch, "You can't steal from them!\n");
        return;
    }
    if (ROOM_FLAGGED(vict->in_room, ROOM_ARENA)) {
        char_printf(ch, "You can't steal in the arena!\n");
        return;
    }

    /* 101% is a complete failure */
    percent = random_number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;
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
                    char_printf(ch, "Steal the equipment now?  Impossible!\n");
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
                        char_printf(ch, "Got it!\n");
                        if (AWAKE(vict))
                            improve_skill(ch, SKILL_STEAL);
                        get_check_money(ch, obj);
                    }
                } else
                    char_printf(ch, "You cannot carry that much.\n");
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
            coins[PLATINUM] = (GET_PLATINUM(vict) * random_number(1, 10)) / 100;
            coins[GOLD] = (GET_GOLD(vict) * random_number(1, 10)) / 100;
            coins[SILVER] = (GET_SILVER(vict) * random_number(1, 10)) / 100;
            coins[COPPER] = (GET_COPPER(vict) * random_number(1, 10)) / 100;

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
                char_printf(ch, "Woohoo! You stole {}.\n", buf);
            } else {
                char_printf(ch, "You couldn't get any coins...\n");
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
        char_printf(ch, "You can only do that in your guild.\n");
    else
        char_printf(ch, "Huh?!?\n");
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
        char_printf(ch, "You are already visible.\n");
    }
}

void set_title(CharData *ch, char *title);
ACMD(do_title) {
    int titles, which;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (IS_NPC(ch))
        char_printf(ch, "Your title is fine... go away.\n");
    else if (PLR_FLAGGED(ch, PLR_NOTITLE))
        char_printf(ch, "You can't title yourself -- you shouldn't have abused it!\n");
    else if (GET_LEVEL(ch) >= LVL_IMMORT) {
        if (strlen(argument) > MAX_TITLE_LENGTH)
            char_printf(ch, "Sorry, titles can't be longer than {:d} characters.\n", MAX_TITLE_LENGTH);
        else {
            set_title(ch, argument);
            char_printf(ch, "Okay, you're now {} {}.\n", GET_NAME(ch), GET_TITLE(ch));
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
            char_printf(ch, "You haven't earned any permanent titles!\n");
            if (ch->player.title && *ch->player.title)
                char_printf(ch, "Use 'title 0' to clear your current title.\n");
        } else {
            titles = 0;
            char_printf(ch,
                        "You have earned the following titles:\n"
                        "  0) <no title>\n");
            if (GET_PERM_TITLES(ch))
                while (GET_PERM_TITLES(ch)[titles]) {
                    char_printf(ch, "  {:d}) {}\n", titles + 1, GET_PERM_TITLES(ch)[titles]);
                    ++titles;
                }
            if (GET_CLAN(ch) && IS_CLAN_MEMBER(ch))
                char_printf(ch, "  {:d}) {} {}\n", ++titles, GET_CLAN_TITLE(ch), GET_CLAN(ch)->abbreviation);
            char_printf(ch, "Use 'title <number>' to switch your title.\n");
        }
    } else if (!is_positive_integer(argument))
        char_printf(ch,
                    "Usage: title\n"
                    "       title <number>\n");
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
            char_printf(ch, "You don't have that many titles!\n");
            return;
        }
        if (GET_TITLE(ch)) {
            char_printf(ch, "Okay, set your title to: {}\n", GET_TITLE(ch));
        } else
            char_printf(ch, "Okay, cleared your title.\n");
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
        char_printf(ch, "You don't see that person here.\n");
        return;
    } else if (!EFF_FLAGGED(vict, EFF_ON_FIRE)) {
        char_printf(ch, "Where's the fire?\n");
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
        if (GET_SKILL(ch, SKILL_DOUSE) < random_number(0, 100)) {
            act("$n&0 frantically rolls around on the ground, attempting to douse "
                "the flames consuming $s body.",
                true, ch, 0, 0, TO_ROOM);
            char_printf(ch, "You roll around on the ground, trying to douse the flames engulfing your body!\n");
        } else {
            act("$n&0 rolls on the ground frantically, finally smothering the fire that was consuming $m.", true, ch, 0,
                0, TO_ROOM);
            char_printf(ch, "You roll around on the ground, finally smothering your flames.\n");
            success = true;
        }
    }

    /* No water, trying to douse someone else */
    else if (GET_SKILL(ch, SKILL_DOUSE) - 40 < random_number(0, 100)) {
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
        char_printf(ch, "&0&7&bBut you lead no group!&0\n");
        return;
    }

    disband_group(ch, true, false);
}

ACMD(do_consent) {
    CharData *target;

    one_argument(argument, arg);

    if (!*arg) {
        if (!CONSENT(ch))
            char_printf(ch, "You are not consented to anyone!\n");
        else
            act("You are consented to $N.", true, ch, 0, CONSENT(ch), TO_CHAR | TO_SLEEP);
        return;
    }

    if (!strcasecmp(arg, "off"))
        target = ch; /* consent self to turn it off */
    else if (!(target = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
        char_printf(ch, NOPERSON);
        return;
    }

    if (target == ch) {
        if (CONSENT(ch)) {
            act("$n&0 has revoked $s consent.&0", false, ch, 0, CONSENT(ch), TO_VICT | TO_SLEEP);
            char_printf(ch, "You revoke your consent.&0\n");
            CONSENT(ch) = nullptr;
        } else
            char_printf(ch, "You haven't given your consent to anyone.\n");
        return;
    }

    if (CONSENT(ch) == target) {
        act("$N already has your consent.", false, ch, 0, target, TO_CHAR | TO_SLEEP);
        return;
    }

    if (!speech_ok(ch, 1)) {
        char_printf(ch, "Your sore throat somehow prevents you from doing this.\n");
        return;
    }

    if (CONSENT(ch))
        act("$n&0 has removed $s consent.&0", false, ch, 0, CONSENT(ch), TO_VICT | TO_SLEEP);
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
            char_printf(ch, "Nobody here looks like they need bandaging!\n");
            return;
        }
    } else if (!(victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        char_printf(ch, NOPERSON);
        return;
    }
    if (GET_HIT(victim) >= 0 && GET_STANCE(victim) >= STANCE_STUNNED) {
        act("$N looks in pretty good shape already!&0", false, ch, 0, victim, TO_CHAR);
        return;
    }

    if (GET_SKILL(ch, SKILL_BANDAGE) > random_number(1, 80)) {
        act("&0You bandage $N.&0", false, ch, 0, victim, TO_CHAR);
        act("$n&0 bandages $N's wounds.&0", false, ch, 0, victim, TO_NOTVICT);
        hurt_char(victim, nullptr, std::max(-3, GET_SKILL(ch, SKILL_BANDAGE) / -10), true);
    } else {
        act("You fail to bandage $N properly.", false, ch, 0, victim, TO_CHAR);
        act("$n fails an attempt to bandage $N's wounds.&0", false, ch, 0, victim, TO_NOTVICT);
        if (DAMAGE_WILL_KILL(victim, 1)) {
            act("Your bandaging was so appalling that $N died!&0", false, ch, 0, victim, TO_CHAR);
            act("$n kills $N with some dismal bandaging.&0", false, ch, 0, victim, TO_NOTVICT);
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

    sprintf(buf2, "%s%d&0h&0/", harm_color, GET_HIT(ch));
    strcat(buffer, buf2);
    if (GET_MAX_HIT(ch) < 10)
        strcat(buffer, "   ");
    else if (GET_MAX_HIT(ch) < 100)
        strcat(buffer, "  ");
    else if (GET_MAX_HIT(ch) < 1000)
        strcat(buffer, " ");

    sprintf(buf2, "%s%d&0H&0  %3dv/%3dV] [%s]", CLR(ch, AUND), GET_MAX_HIT(ch), GET_MOVE(ch), GET_MAX_MOVE(ch),
            CLASS_ABBR(ch));
    strcat(buffer, buf2);
}

void print_group(CharData *ch) {
    CharData *k;
    GroupType *f;

    if (!ch->group_master && !ch->groupees)
        char_printf(ch, "&2But you are not the member of a group!&0\n");
    else {
        char_printf(ch, "{}Your group consists of:&0\n", CLR(ch, AUND));

        k = (ch->group_master ? ch->group_master : ch);
        if (CAN_SEE(ch, k)) {
            make_group_report_line(k, buf);
            char_printf(ch, "{} (&0&2&bHead of group&0)\n", buf);
        }

        for (f = k->groupees; f; f = f->next) {
            if (!CAN_SEE(ch, f->groupee))
                continue;

            make_group_report_line(f->groupee, buf);
            char_printf(ch, "{}\n", buf);
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
        char_printf(ch, "In your dreams, or what?\n");
        return;
    }

    if (!strcasecmp("all", arg)) {
        group_all = true;
        tch = nullptr;
    } else if (!(tch = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
        char_printf(ch, NOPERSON);
        return;
    }

    /* Attempt to remove yourself from your group. */
    if (ch == tch) {
        if (ch->group_master || ch->groupees)
            ungroup(ch, true, false);
        else
            char_printf(ch, "&2You're not in a group!&0\n");
        return;
    }

    /* You can't enroll someone if you're in a group and not the leader. */
    if (ch->group_master) {
        char_printf(ch, "&2You cannot enroll group members without being head of a group.&0\n");
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
                char_printf(ch,
                            "&2You cannot group {}, because the level difference is too great.\n"
                            "(The maximum allowable difference is currently {:d}.)&0\n",
                            GET_NAME(gch), max_group_difference);
                continue;
            }

            add_groupee(ch, gch);
            found = true;
        }

        if (!found)
            char_printf(ch, "&2No one has consented you that is not already in a group.&0\n");
        return;
    }

    if (tch->group_master && tch->group_master != ch) {
        char_printf(ch, "&2That person is already in a group.&0\n");
        return;
    }

    if (tch->groupees) {
        char_printf(ch, "&2That person is leading a group.&0\n");
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
            act("&2You do not have $S consent.&0", true, ch, nullptr, tch, TO_CHAR);
            return;
        }
    }

    level_diff = GET_LEVEL(ch) - GET_LEVEL(tch);

    if (max_group_difference && (level_diff > max_group_difference || level_diff < -max_group_difference)) {
        char_printf(ch,
                    "&2You cannot group {}, because the level difference is too great.\n"
                    "(The maximum allowable difference is currently {:d}.)&0\n",
                    GET_NAME(tch), max_group_difference);
        return;
    }

    add_groupee(ch, tch);
}

static void split_share(CharData *giver, CharData *receiver, int coins[]) {
    if (coins[PLATINUM] || coins[GOLD] || coins[SILVER] || coins[COPPER]) {
        statemoney(buf, coins);
        char_printf(receiver, "You {} {}.\n", giver == receiver ? "keep" : "receive", buf);
    } else
        char_printf(receiver, "You forego your share.\n");
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
            char_printf(ch, "But you're the only one in your group here!\n");
        return;
    }

    for (i = 0; i < NUM_COIN_TYPES; ++i) {
        share[i] = coins[i] / count;
        coins[i] -= share[i] * count;
        /* coins[i] now contains the remainder...but who gets it? */
        remainder_start[i] = random_number(0, count - 1);
    }

    /*
     * remainder_start[i] is the index of the 'first' group member who
     * gets an extra ith coin type.  If n = coins[i] is nonzero for a
     * particular i, then n group members, starting at the
     * remainder_start[i]th group member, will receive an extra coin.
     */

    char_printf(ch, "&7&bYou split some coins with your group.&0\n");
    act("&7$n splits some coins.&0", true, ch, 0, 0, TO_ROOM);
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
        char_printf(ch, "&2But you are not a member of any group!&0\n");
        return;
    }

    skip_spaces(&argument);

    if (!*argument) {
        char_printf(ch, "Split what?\n");
        return;
    }

    if (!parse_money(&argument, coins)) {
        char_printf(ch, "That's not a coin type.\n");
        return;
    }

    if (!coins[PLATINUM] && !coins[GOLD] && !coins[SILVER] && !coins[COPPER]) {
        char_printf(ch, "Split zero coins?  Done.\n");
        return;
    }

    for (i = 0; i < NUM_COIN_TYPES; ++i)
        if (coins[i] > GET_COINS(ch)[i]) {
            char_printf(ch, "You don't have enough {}!\n", COIN_NAME(i));
            return;
        }

    split_coins(ch, coins, 0);
}

ACMD(do_use) {
    ObjData *mag_item;

    half_chop(argument, arg, buf);
    if (!*arg) {
        char_printf(ch, "What do you want to {}?\n", CMD_NAME);
        return;
    }
    mag_item = GET_EQ(ch, WEAR_HOLD);

    if (!mag_item || !isname(arg, mag_item->name)) {
        switch (subcmd) {
        case SCMD_RECITE:
        case SCMD_QUAFF:
            if (!(mag_item = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg)))) {
                char_printf(ch, "You don't seem to have {} {}.\n", AN(arg), arg);
                return;
            }
            break;
        case SCMD_USE:
            /* Item isn't in first hand, now check the second. */
            mag_item = GET_EQ(ch, WEAR_HOLD2);
            if (!mag_item || !isname(arg, mag_item->name)) {
                char_printf(ch, "You don't seem to be holding {} {}.\n", AN(arg), arg);
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
        char_printf(ch, "That item is too powerful for you to use.\n");
        return;
    }
    switch (subcmd) {
    case SCMD_QUAFF:
        if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
            char_printf(ch, "You can only quaff potions.\n");
            return;
        }
        break;
    case SCMD_RECITE:
        if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
            char_printf(ch, "You can only recite scrolls.\n");
            return;
        }
        break;
    case SCMD_USE:
        if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) && (GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
            char_printf(ch, "You can't seem to figure out how to use it.\n");
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
            char_printf(ch, "Your current wimp level is {:d} hit points.\n", GET_WIMP_LEV(ch));
            return;
        } else {
            char_printf(ch, "At the moment, you're not a wimp.  (sure, sure...)\n");
            return;
        }
    }
    if (isdigit(*arg)) {
        if ((wimp_lev = atoi(arg))) {
            if (wimp_lev < 0)
                char_printf(ch, "Heh, heh, heh.. we are jolly funny today, eh?\n");
            else if (wimp_lev > GET_MAX_HIT(ch))
                char_printf(ch, "That doesn't make much sense, now does it?\n");
            else if (wimp_lev > (GET_MAX_HIT(ch) >> 1))
                char_printf(ch, "You can't set your wimp level above half your hit points.\n");
            else {
                char_printf(ch, "Okay, you'll wimp out if you drop below {} hit points.\n", wimp_lev);
                GET_WIMP_LEV(ch) = wimp_lev;
            }
        } else {
            char_printf(ch, "Okay, you'll now tough out fights to the bitter end.\n");
            GET_WIMP_LEV(ch) = 0;
        }
    } else
        char_printf(ch, "Specify at how many hit points you want to wimp out at.  (0 to disable)\n");

    return;
}

ACMD(do_display) {
    int i, x;

    one_argument(argument, arg);

    if (!*arg || !is_number(arg)) {
        char_printf(ch, "The following pre-set prompts are availible...\n");
        for (i = 0; default_prompts[i][0]; i++) {
            char_printf(ch, "{:2d}. {:<20} {}\n", i, default_prompts[i][0], default_prompts[i][1]);
        }
        char_printf(ch, "Usage: display <number>\n");
        return;
    }

    i = atoi(arg);

    if (i < 0) {
        char_printf(ch, "The number cannot be negative.\n");
        return;
    }

    for (x = 0; default_prompts[x][0]; ++x)
        ;

    if (i >= x) {
        char_printf(ch, "The range for the prompt number is 0-{}.\n", x - 1);
        return;
    }

    if (GET_PROMPT(ch))
        free(GET_PROMPT(ch));

    GET_PROMPT(ch) = strdup(default_prompts[i][1]);
    char_printf(ch, "Set your prompt to the {} preset prompt.\n", default_prompts[i][0]);
}

ACMD(do_prompt) {
    skip_spaces(&argument);

    if (!*argument) {
        char_printf(ch, "Your prompt is currently: {}\n", (GET_PROMPT(ch) ? escape_ansi(GET_PROMPT(ch)) : "n/a"));
        return;
    }

    delete_doubledollar(argument);

    if (GET_PROMPT(ch))
        free(GET_PROMPT(ch));

    GET_PROMPT(ch) = strdup(argument);

    char_printf(ch, "Okay, set your prompt to: {}\n", escape_ansi(argument));
}

const char *idea_types[] = {"bug", "typo", "idea", "note"};

ACMD(do_gen_write) {
    FILE *fl;
    const char *filename;
    char buf[MAX_STRING_LENGTH];
    struct stat fbuf;
    extern int max_filesize;
    time_t ct;

    ch = REAL_CHAR(ch);

    if (IS_NPC(ch)) {
        char_printf(ch, "Monsters can't have ideas - go away.\n");
        return;
    }

    skip_spaces(&argument);
    delete_doubledollar(argument);

    switch (subcmd) {
    case SCMD_NOTE:
        argument = one_argument(argument, arg);
        if (!*argument) {
            char_printf(ch, "Usage: note <player> <text>\n");
            return;
        }
        get_pfilename(arg, buf, NOTES_FILE);
        filename = buf;
        char_printf(ch, "{}\n", buf);
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
                    "Please idle momentarily and try to submit your {} again.\n",
                    idea_types[subcmd]);
        return;
    }

    if (!*argument) {
        char_printf(ch, "Please enter a message to go with that bug, idea, or typo.\n");
        return;
    }

    log(LogSeverity::Stat, LVL_IMMORT, "{} by {} [{:d}]: {}", idea_types[subcmd], GET_NAME(ch), CH_RVNUM(ch), argument);

    if (stat(filename, &fbuf) >= 0 && fbuf.st_size >= max_filesize) {
        char_printf(ch, "Sorry, the file is full right now.. try again later.\n");
        return;
    }

    if (!(fl = fopen(filename, "a"))) {
        perror("do_gen_write");
        char_printf(ch, "Could not open the file.  Sorry.\n");
        return;
    }

    ct = time(0);
    strftime(buf1, 15, TIMEFMT_DATE, localtime(&ct));
    fprintf(fl, "%-8s (%11.11s) [%5d] %s\n", GET_NAME(ch), buf1, world[ch->in_room].vnum, argument);
    fclose(fl);
    char_printf(ch, "Thanks for the bug, idea, or typo comment!\n");
}

ACMD(do_peace) {
    CharData *vict, *next_v;
    one_argument(argument, arg);
    if (!is_abbrev(arg, "off")) {
        act("&7$n &4&bglows&0&7 with a &bbright white aura&0&7 as $e waves $s "
            "mighty hand!&0",
            false, ch, 0, 0, TO_ROOM);
        room_printf(ch->in_room, "&7&bA peaceful feeling washes into the room, dousing all violence!&0\n");
        for (vict = world[ch->in_room].people; vict; vict = next_v) {
            next_v = vict->next_in_room;
            if (FIGHTING(vict))
                stop_fighting(vict);
        }
        SET_FLAG(ROOM_FLAGS(ch->in_room), ROOM_PEACEFUL);
    } else {
        act("&7$n &4&bglows&0&7 with a &1&bbright red aura&0&7 as $e waves $s mighty hand!&0", false, ch, 0, 0,
            TO_ROOM);
        room_printf(ch->in_room,
                    "&1&bThe peaceful feeling in the room subsides... You don't feel quite as safe anymore.&0\n");
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
        char_printf(ch, "Petition is for those wimpy mortals!\n");
        return;
    }
    if (!*argument) {
        char_printf(ch, "Yes, but WHAT do you want to petition?\n");
        return;
    }

    if (!speech_ok(ch, 0))
        return;

    char_printf(ch, "&0&6You petition, '&b{}&0&6'&0\n", argument);

    for (d = descriptor_list; d; d = d->next) {
        if (!d->character)
            continue;
        tch = d->original ? d->original : d->character;
        if (PRF_FLAGGED(tch, PRF_NOPETI))
            continue;
        if (GET_LEVEL(tch) < LVL_IMMORT)
            continue;
        char_printf(d->character, "&0&6{}&0&6 petitions, '&b{}&0&6'&0\n", GET_NAME(REAL_CHAR(ch)), argument);
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
        char_printf(ch, "You can't concentrate enough while you are fighting.\n");
        return;
    }

    if ((GET_CLASS(ch) != CLASS_PALADIN) && (GET_CLASS(ch) != CLASS_ANTI_PALADIN)) {
        char_printf(ch, "You have no idea what you are trying to accomplish.\n");
        return;
    }

    if (GET_LEVEL(ch) < LVL_GOD) {
        for (fol = ch->followers; fol; fol = fol->next)
            if (IS_NPC(fol->follower) && MOB_FLAGGED(fol->follower, MOB_MOUNTABLE)) {
                char_printf(ch, "You already have a mount!\n");
                return;
            }
        if (GET_LEVEL(ch) < 15) {
            char_printf(ch, "You are not yet deemed worthy of a mount (try gaining some more experience)\n");
            return;
        }
        if (!IS_GOOD(ch) && !IS_NPC(ch) && (GET_CLASS(ch) == CLASS_PALADIN)) {
            char_printf(ch, "Not even horses can stand your offensive presence!\n");
            return;
        }
        if (CH_INDOORS(ch)) {
            char_printf(ch, "Try again, OUTDOORS THIS TIME!\n");
            return;
        }
        if (GET_COOLDOWN(ch, CD_SUMMON_MOUNT)) {
            i = GET_COOLDOWN(ch, CD_SUMMON_MOUNT) / (1 MUD_HR) + 1;
            if (i == 1)
                strcpy(buf1, "hour");
            else
                sprintf(buf1, "%d hours", i);
            char_printf(ch, "You must wait another {} before you can summon your mount.\n", buf1);
            return;
        }
    }

    char_printf(ch, "You begin calling for a mount..\n");

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
        char_printf(ch, "No mount could be found, please report this to a god.\n");
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
        GET_LEVEL(mount) = std::max(0, ideal_mountlevel(ch));
    GET_MAX_HIT(mount) = std::max(10, base_hp + random_number(50, 100));
    GET_HIT(mount) = GET_MAX_HIT(mount);
    GET_MAX_MOVE(mount) = std::max(10, base_mv + random_number(0, 50));
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
        char_printf(ch, "You whistle sharply.\n");

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
        char_printf(ch, "You don't have a pet to call out to.\n");
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
        char_printf(ch, "You don't have the ability to lay hands.\n");
        return;
    }

    /* Make sure we haven't already used it for the day */
    if (GET_COOLDOWN(ch, CD_LAY_HANDS)) {
        char_printf(ch, "You need more rest before laying hands again.\n");
        return;
    }

    one_argument(argument, arg);

    /*
     * Determine target.
     */
    if (*arg) {
        /* If a target was specified, attempt to use that. */
        if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
            char_printf(ch, NOPERSON);
            return;
        }
    } else {
        /* If no target was specified, default depending on class. */
        if (GET_CLASS(ch) == CLASS_PALADIN)
            vict = ch;
        else if (FIGHTING(ch)) /* Anti-paladin */
            vict = FIGHTING(ch);
        else {
            char_printf(ch, "How about a target this time!?\n");
            return;
        }
    }

    if (GET_CLASS(ch) == CLASS_PALADIN) {
        if (!IS_GOOD(ch)) {
            char_printf(ch, "For your evil ways, your god has forsaken you!\n");
            return;
        }

        /* Paladins heal all players, and good/neutral mobs, except undead. */
        if (IS_PC(vict) || (!IS_EVIL(vict) && GET_LIFEFORCE(vict) != LIFE_UNDEAD))
            dam = -1.5 * dam + random_number(0, 50);

        /* Paladins harm all evil mobs and undead, regardless of alignment. */
        else if (GET_LIFEFORCE(vict) == LIFE_UNDEAD)
            dam = 1.5 * dam + random_number(50, 150);
        else
            dam = 1.2 * dam + random_number(1, 50);
    } else { /* Anti-paladin */
        if (!IS_EVIL(ch)) {
            char_printf(ch, "For your benevolent ways, your god has forsaken you!\n");
            return;
        }

        /* Anti-paladins heal the undead, regardless of alignment. */
        if (GET_LIFEFORCE(vict) == LIFE_UNDEAD)
            dam = -1.5 * dam + random_number(1, 50);

        /* Anti-paladins have no effect on evils. */
        else if (IS_EVIL(vict)) {
            char_printf(ch, "Your harmful touch has no affect on other evils.\n");
            return;
        }

        /* Anti-paladins harm good and neutral mobs/players. */
        else
            dam = 1.2 * dam + random_number(1, 50);
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
    char_printf(ch, "You don't know how to control undead!\n");
    return;
  }

  if (FIGHTING(ch)) {
    char_printf(ch, "You can't gain control the dead while fighting!\n");
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
        char_printf(ch, "You are too busy fighting to attend to yourself!\n");
        return;
    }

    if (GET_COOLDOWN(ch, CD_FIRST_AID)) {
        char_printf(ch, "You can only do this once per day.\n");
        return;
    }

    if (GET_HIT(ch) >= GET_MAX_HIT(ch)) {
        char_printf(ch, "You're already in pristine health!\n");
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

    char_printf(ch, "You attempt to render first aid unto yourself. (" AHGRN "{:d}" ANRM ")\n", GET_HIT(ch) - orig_hp);

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
        char_printf(ch, "You feel sociable and stop ignoring anyone.\n");
        tch->player_specials->ignored = nullptr;
        return;
    }
    if (!(target = find_char_around_char(ch, find_vis_by_name(ch, arg))) || IS_NPC(target)) {
        char_printf(ch, NOPERSON);
        return;
    }
    char_printf(ch, "You now ignore {}.\n", GET_NAME(target));
    tch->player_specials->ignored = target;
}

ACMD(do_point) {
    int found;
    CharData *tch = nullptr;
    ObjData *tobj = nullptr;

    argument = one_argument(argument, arg);
    skip_spaces(&argument);

    if (!*arg) {
        char_printf(ch, "Point at what?  Or whom?\n");
        return;
    }

    if (!(found = generic_find(arg, FIND_OBJ_ROOM | FIND_CHAR_ROOM, ch, &tch, &tobj))) {
        char_printf(ch, "Can't find that!\n");
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
        char_printf(ch, "You point at yourself.\n");
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
