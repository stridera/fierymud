/***************************************************************************
 *   File: act.wizard.c                                   Part of FieryMUD *
 *  Usage: Player-level god commands and other goodies                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "act.hpp"

#include "ban.hpp"
#include "casting.hpp"
#include "charsize.hpp"
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
#include "exits.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "house.hpp"
#include "interpreter.hpp"
#include "legacy_structs.hpp"
#include "lifeforce.hpp"
#include "limits.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "modify.hpp"
#include "movement.hpp"
#include "olc.hpp"
#include "pfiles.hpp"
#include "players.hpp"
#include "quest.hpp"
#include "races.hpp"
#include "regen.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "textfiles.hpp"
#include "utils.hpp"
#include "weather.hpp"

#include <algorithm>
#include <array>
#include <fcntl.h>
#include <fmt/ranges.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void check_new_surroundings(CharData *ch, bool old_room_was_dark, bool tx_obvious);

int reserved_word(std::string_view argument);

/* extern functions */
void send_to_xnames(std::string_view name);
int find_zone(int num);
void cure_laryngitis(CharData *ch);
void reboot_mud_prep();
void rebootwarning(int minutes);
void update_stats(CharData *ch);

ACMD(do_inctime) {
    auto arg = argument.shift();
    if (!matches(arg, "yes")) {
        char_printf(ch,
                    "Are you sure you want to move time forward?\n"
                    "If so, type 'inctime yes'.\n");
        return;
    }

    char_printf(ch, "Advancing game time by one hour.\n");
    increment_game_time();
}

ACMD(do_echo) {
    auto echo = argument.shift();

    if (echo.empty())
        char_printf(ch, "Yes.. but what?\n");
    else {
        std::string buf;
        if (subcmd == SCMD_EMOTE || subcmd == SCMD_EMOTES) {
            if (EFF_FLAGGED(ch, EFF_SILENCE)) {
                char_printf(ch, "Your lips move, but no sound forms.\n");
                return;
            }
            if (!speech_ok(ch, 0))
                return;
            if (GET_LEVEL(REAL_CHAR(ch)) < LVL_IMMORT && !IS_NPC(ch))
                echo = strip_ansi(echo);
            buf = fmt::format("$n{} {}&0", subcmd == SCMD_EMOTES ? "'s" : "", echo);
        } else
            buf = fmt::format("{}&0", echo);

        act(buf, subcmd != SCMD_ECHO, ch, 0, 0, TO_ROOM);

        if (PRF_FLAGGED(ch, PRF_NOREPEAT))
            char_printf(ch, OK);
        else
            act(buf, false, ch, 0, 0, TO_CHAR);
    }
}

ACMD(do_send) {
    CharData *vict;

    auto arg = argument.shift();
    auto buf = argument.get();

    if (arg.empty()) {
        char_printf(ch, "Send what to who?\n");
        return;
    }
    if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
        char_printf(ch, NOPERSON);
        return;
    }
    char_printf(vict, "{}@0\n", buf);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        char_printf(ch, "Sent.\n");
    else
        char_printf(ch, "You send '{}@0' to {}.\n", buf, GET_NAME(vict));
}

void perform_ptell(CharData *ch, CharData *vict, std::string_view arg) {
    DescriptorData *d;

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        char_printf(ch, OK);
    else {
        act(fmt::format("&6You respond to $N, '&b{}&0&6'&0", arg), false, ch, 0, vict, TO_CHAR | TO_SLEEP);
    }
    if (PRF_FLAGGED(vict, PRF_AFK)) {
        char_printf(vict, "You received the previous message while AFK.\n");
        char_printf(ch, "That person is AFK right now but received your message.\n");
    }

    auto buf =
        fmt::format("&6{} responds to {}'s petition, '&b{}&0&6'&0\n", GET_NAME(REAL_CHAR(ch)), GET_NAME(vict), arg);
    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) == CON_PLAYING && d != ch->desc && GET_LEVEL(d->character) >= LVL_IMMORT &&
            !PLR_FLAGGED(d->character, PLR_WRITING) && !PLR_FLAGGED(d->character, PLR_MAILING) && !EDITING(d))
            char_printf(d->character, buf);
    }

    if (vict->forward && !vict->desc)
        vict = vict->forward;

    act(fmt::format("&6$n responds to your petition, '&b%s&0&6'&0", arg), false, REAL_CHAR(ch), 0, vict,
        TO_VICT | TO_SLEEP);
}

ACMD(do_ptell) {
    CharData *vict;

    auto buf = argument.shift();
    auto buf2 = argument.get();

    if (buf.empty() || buf2.empty())
        char_printf(ch, "Who do you wish to ptell??\n");
    else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, buf))))
        char_printf(ch, NOPERSON);
    else if (ch == vict)
        char_printf(ch, "You need mental help. Try ptelling someone besides yourself.\n");
    else if (GET_LEVEL(vict) >= LVL_IMMORT)
        char_printf(ch, "Just use wiznet!\n");
    else if (!IS_NPC(vict) && !vict->desc && (!vict->forward || !vict->forward->desc)) /* linkless */
        act("$E's clueless at the moment.", false, ch, 0, vict, TO_CHAR | TO_SLEEP);
    else if (PLR_FLAGGED(vict, PLR_WRITING))
        act("$E's writing a message right now; try again later.", false, ch, 0, vict, TO_CHAR | TO_SLEEP);
    else if (vict->desc && EDITING(vict->desc))
        act("$E's writing a message right now; try again later.", false, ch, 0, vict, TO_CHAR | TO_SLEEP);
    else
        perform_ptell(ch, vict, buf2);
}

/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
int find_target_room(CharData *ch, std::string_view roomstr) {
    int location;
    CharData *target_mob;
    ObjData *target_obj;

    if (roomstr.empty()) {
        char_printf(ch, "You must supply a room number or name.\n");
        return NOWHERE;
    }
    if (is_integer(roomstr) && roomstr.find('.') == std::string_view::npos) {
        int tmp = svtoi(roomstr);
        if ((location = real_room(tmp)) < 0) {
            char_printf(ch, "No room exists with that number.\n");
            return NOWHERE;
        }
    } else if ((target_mob = find_char_around_char(ch, find_vis_by_name(ch, roomstr))))
        location = target_mob->in_room;
    else if ((target_obj = find_obj_around_char(ch, find_vis_by_name(ch, roomstr)))) {
        if (target_obj->in_room != NOWHERE)
            location = target_obj->in_room;
        else {
            char_printf(ch, "That object is not available.\n");
            return NOWHERE;
        }
    } else {
        char_printf(ch, "No such creature or object around.\n");
        return NOWHERE;
    }

    /* a location has been found -- if you're < GRGOD, check restrictions. */
    if (GET_LEVEL(ch) < LVL_GOD) {
        if (ROOM_FLAGGED(location, ROOM_GODROOM)) {
            char_printf(ch, "You are not godly enough to use that room!\n");
            return NOWHERE;
        }
        if (ROOM_FLAGGED(location, ROOM_PRIVATE) && world[location].people && world[location].people->next_in_room) {
            char_printf(ch, "There's a private conversation going on in that room.\n");
            return NOWHERE;
        }
        if (ROOM_FLAGGED(location, ROOM_HOUSE) && !House_can_enter(ch, world[location].vnum)) {
            char_printf(ch, "That's private property -- no trespassing!\n");
            return NOWHERE;
        }
    }
    return location;
}

ACMD(do_at) {
    int location, original_loc;

    auto buf = argument.shift();
    auto command = argument.get();

    if (buf.empty()) {
        char_printf(ch, "You must supply a room number or a name.\n");
        return;
    }

    if (command.empty()) {
        char_printf(ch, "What do you want to do there?\n");
        return;
    }

    if ((location = find_target_room(ch, buf)) < 0)
        return;

    /* a location has been found. */
    original_loc = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, location);
    command_interpreter(ch, command);

    /* check if the char is still there */
    if (ch->in_room == location) {
        char_from_room(ch);
        char_to_room(ch, original_loc);
    }
}

ACMD(do_goto) {
    int location;
    FollowType *k;

    auto arg = argument.shift();

    if (matches(arg, "home")) {
        if ((location = real_room(GET_HOMEROOM(ch))) < 0) {
            char_printf(ch, "Your home room is invalid.\n");
            return;
        }
    } else if ((location = find_target_room(ch, arg)) < 0) {
        return;
    }

    std::string poofout_msg = [&]() {
        if (GET_POOFOUT(ch).empty())
            return std::string{"$n disappears in a puff of smoke."};
        return GET_POOFOUT(ch);
    }();

    act(poofout_msg, true, ch, 0, 0, TO_ROOM);
    dismount_char(ch);
    char_from_room(ch);
    char_to_room(ch, location);

    std::string poofin_msg = [&]() {
        if (GET_POOFIN(ch).empty())
            return std::string{"$n appears with an ear-splitting bang."};
        return GET_POOFIN(ch);
    }();
    act(poofin_msg, true, ch, 0, 0, TO_ROOM);
    look_at_room(ch, false);

    for (k = ch->followers; k; k = k->next) {
        if (IS_PET(k->follower) && k->follower->master == ch) {
            char_from_room(k->follower);
            char_to_room(k->follower, location);
            look_at_room(k->follower, true);
        }
    }
}

ACMD(do_linkload) {
    CharData *victim = 0;

    auto arg = argument.shift();

    if (arg.empty()) {
        char_printf(ch, "Linkload who?\n");
        return;
    }
    if (find_char_around_char(ch, find_plr_by_name(arg))) {
        char_printf(ch, "They are already connected!\n");
        return;
    }
    CREATE(victim, CharData, 1);
    clear_char(victim);
    if (load_player(arg, victim) > -1) {
        if (GET_LEVEL(victim) <= GET_LEVEL(ch)) {
            victim->player.time.logon = time(0);
            log(LogSeverity::Warn, GET_LEVEL(ch) + 1, "(GC) {} has link-loaded {}.", GET_NAME(ch), GET_NAME(victim));
            char_to_room(victim, IN_ROOM(ch));
            load_quests(victim);
            load_objects(victim);
            victim->next = character_list;
            character_list = victim;
            victim->desc = nullptr;
            act("You linkload $N.", false, ch, 0, victim, TO_CHAR);
            act("$n linkloads $N.", false, ch, 0, victim, TO_NOTVICT);
        } else {
            char_printf(ch, "Sorry, you aren't high enough to link-load that char.\n");
            free_char(victim);
        }
    } else {
        char_printf(ch, "No such player exists.\n");
        free(victim);
    }
    return;
}

ACMD(do_trans) {
    DescriptorData *i;
    CharData *victim;
    bool wasdark;

    auto buf = argument.shift();
    if (buf.empty())
        char_printf(ch, "Whom do you wish to transfer?\n");
    else if (matches("all", buf)) {
        if (GET_LEVEL(ch) < LVL_GRGOD) {
            char_printf(ch, "I think not.\n");
            return;
        }

        for (i = descriptor_list; i; i = i->next)
            if (!i->connected && i->character && i->character != ch) {
                victim = i->character;
                if (GET_LEVEL(victim) >= GET_LEVEL(ch))
                    continue;
                act("$n disappears in a mushroom cloud.", false, victim, 0, 0, TO_ROOM);
                wasdark = IS_DARK(victim->in_room) && !CAN_SEE_IN_DARK(victim);
                dismount_char(victim);
                char_from_room(victim);
                char_to_room(victim, ch->in_room);
                act("$n arrives from a puff of smoke.", false, victim, 0, 0, TO_ROOM);
                act("$n has transferred you!", false, ch, 0, victim, TO_VICT);
                check_new_surroundings(victim, wasdark, true);
            }
        char_printf(ch, OK);
    } else {
        if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, buf))))
            char_printf(ch, NOPERSON);
        else if (victim == ch)
            char_printf(ch, "That doesn't make much sense, does it?\n");
        else {
            if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
                char_printf(ch, "Go transfer someone your own size.\n");
                return;
            }
            act("$n disappears in a mushroom cloud.", false, victim, 0, 0, TO_ROOM);
            wasdark = IS_DARK(victim->in_room) && !CAN_SEE_IN_DARK(victim);
            dismount_char(victim);
            char_from_room(victim);
            char_to_room(victim, ch->in_room);
            act("$n arrives from a puff of smoke.", false, victim, 0, 0, TO_ROOM);
            act("$n has transferred you!", false, ch, 0, victim, TO_VICT);
            check_new_surroundings(victim, wasdark, true);
        }
    }
}

ACMD(do_teleport) {
    CharData *victim;
    int target;
    bool wasdark;

    auto buf = argument.shift();
    auto buf2 = argument.get();

    if (buf.empty())
        char_printf(ch, "Whom do you wish to teleport?\n");
    else if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, buf))))
        char_printf(ch, NOPERSON);
    else if (victim == ch)
        char_printf(ch, "Use 'goto' to teleport yourself.\n");
    else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
        char_printf(ch, "Maybe you shouldn't do that.\n");
    else if (buf2.empty())
        char_printf(ch, "Where do you wish to send this person?\n");
    else if ((target = find_target_room(ch, buf2)) >= 0) {
        char_printf(ch, OK);
        act("$n disappears in a puff of smoke.", false, victim, 0, 0, TO_ROOM);
        wasdark = IS_DARK(victim->in_room) && !CAN_SEE_IN_DARK(victim);
        dismount_char(victim);
        char_from_room(victim);
        char_to_room(victim, target);
        act("$n arrives from a puff of smoke.", false, victim, 0, 0, TO_ROOM);
        act("$n has teleported you!", false, ch, 0, victim, TO_VICT);
        check_new_surroundings(victim, wasdark, true);
    }
}

ACMD(do_shutdown) {
    extern int circle_shutdown, circle_reboot;

    if (subcmd != SCMD_SHUTDOWN) {
        char_printf(ch, "If you want to shut something down, say so!\n");
        return;
    }
    auto arg = argument.shift();

    if (arg.empty()) {
        log("(GC) Shutdown by {}.", GET_NAME(ch));
        all_printf("Shutting down.\n");
        circle_shutdown = 1;
    } else if (matches(arg, "reboot")) {
        log("(GC) Reboot by {}.", GET_NAME(ch));
        reboot_mud_prep();
        circle_shutdown = circle_reboot = 1;
    } else if (matches(arg, "now")) {
        log("(GC) Shutdown NOW by {}.", GET_NAME(ch));
        all_printf(
            "Rebooting.. come back in a minute or two.\n"
            "           &1&b** ****** ****&0\n"
            "         &1&b**&0 &3&b***     *****&0  &1&b**&0\n"
            "       &1&b**&0 &3&b**      &1&b*&0     &3&b***&0  &1&b*&0\n"
            "       &1&b*&0    &3&b** **   *   *  *&0 &1&b**&0\n"
            "      &1&b*&0  &3&b** * *&0          &1&b*&0     &1&b*&0\n"
            "      &1&b*&0  &3&b*&0    &1&b**&0            &3&b* *&0 &1&b*&0\n"
            "     &1&b*&0 &3&b* &1&b** *&0     &3&b*   ******&0  &1&b*&0\n"
            "      &1&b*&0   &3&b* &1&b* **&0  &3&b***&0     &1&b*&0  &3&b*&0 &1&b*&0\n"
            "        &1&b*&0  &3&b*  *&0 &1&b**********&0  &3&b***&0 &1&b*&0\n"
            "         &1&b*****&0   &3&b*     *   * *&0 &1&b*&0\n"
            "                &1&b*&0   &3&b*&0 &1&b*&0\n"
            "               &1&b*&0  &3&b* *&0  &1&b*&0\n"
            "              &1&b*&0  &3&b*  **&0  &1&b*&0\n"
            "              &1&b*&0 &3&b**   *&0 &1&b*&0\n"
            "                &1&b*&0 &3&b*&0 &1&b*&0\n"
            "                &1&b*&0 &3&b*&0  &1&b**&0\n"
            "               &1&b**&0     &1&b****&0\n"
            "              &1&b***&0  &3&b* *&0    &1&b****&0\n");
        circle_shutdown = 1;
        circle_reboot = 2;

    } else if (matches(arg, "die")) {
        log("(GC) Shutdown by {}.", GET_NAME(ch));
        all_printf("Shutting down for maintenance.\n");
        touch("../.killscript");
        circle_shutdown = 1;
    } else if (matches(arg, "pause")) {
        log("(GC) Shutdown by {}.", GET_NAME(ch));
        all_printf("Shutting down for maintenance.\n");
        touch("../pause");
        circle_shutdown = 1;
    } else
        char_printf(ch, "Unknown shutdown option.\n");
}

void stop_snooping(CharData *ch) {
    if (!ch->desc->snooping)
        char_printf(ch, "You aren't snooping anyone.\n");
    else {
        char_printf(ch, "You stop snooping.\n");
        ch->desc->snooping->snoop_by = nullptr;
        ch->desc->snooping = nullptr;
    }
}

void perform_snoop(CharData *ch, DescriptorData *d) {
    CharData *tch;

    tch = nullptr;

    if (d->snooping == ch->desc) {
        char_printf(ch, "Don't be stupid.\n");
        return;
    } else if (d->snoop_by) {
        char_printf(ch, "Busy already.\n");
        return;
    } else if (d == ch->desc) {
        char_printf(ch, "Not a good idea.\n");
        return;
    } else if (STATE(d) != CON_PLAYING) {
        /* This is to prevent snoopers from seeing passwords. */
        char_printf(ch, "Please wait until they've logged in.\n");
        return;
    }

    if (d->original)
        tch = d->original;
    else if (d->character)
        tch = d->character;

    if (tch && GET_LEVEL(tch) >= GET_LEVEL(ch)) {
        char_printf(ch, "You can't.\n");
        return;
    }

    /* It's ok to snoop on. */
    char_printf(ch, OK);

    /* Stop snooping whoever you were snooping on before */
    if (ch->desc->snooping)
        ch->desc->snooping->snoop_by = nullptr;

    ch->desc->snooping = d;
    d->snoop_by = ch->desc;
}

ACMD(do_snoop) {
    CharData *victim;
    DescriptorData *d;
    int dnum;

    if (!ch->desc)
        return;

    auto arg = argument.shift();

    if (arg.empty())
        stop_snooping(ch);
    else if (is_integer(arg)) {
        dnum = svtoi(arg);
        for (d = descriptor_list; d; d = d->next) {
            if (d->desc_num == dnum) {
                perform_snoop(ch, d);
                return;
            }
        }
        char_printf(ch, "No such descriptor.\n");
    } else if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, arg))))
        char_printf(ch, "No such person around.\n");
    else if (!victim->desc)
        char_printf(ch, "There's no link.. nothing to snoop.\n");
    else if (victim == ch)
        char_printf(ch, "Not a good idea.\n");
    else
        perform_snoop(ch, victim->desc);
}

ACMD(do_switch) {
    CharData *victim;

    auto arg = argument.shift();

    if (POSSESSED(ch))
        char_printf(ch, "You're already switched.\n");
    else if (arg.empty())
        char_printf(ch, "Switch with who?\n");
    else if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, arg))))
        char_printf(ch, "No such character.\n");
    else if (ch == victim)
        char_printf(ch, "Hee hee... we are jolly funny today, eh?\n");
    else if (victim->desc)
        char_printf(ch, "You can't do that, the body is already in use!\n");
    else if ((GET_LEVEL(ch) < LVL_GRGOD) && !IS_NPC(victim))
        char_printf(ch, "You aren't holy enough to use a mortal's body.\n");
    else if ((GET_LEVEL(ch) < GET_LEVEL(victim)))
        char_printf(ch, "You WISHED.\n");
    else {
        char_printf(ch, OK);

        ch->desc->character = victim;
        ch->desc->original = ch;

        victim->desc = ch->desc;
        ch->desc = nullptr;

        if (victim->player.prompt.empty() || IS_NPC(victim))
            victim->player.prompt = ch->player.prompt;
    }
}

ACMD(do_rename) {
    int player_i;
    CharData *victim = nullptr;

    auto arg1 = argument.shift();
    auto arg2 = argument.get();

    if (!ch || IS_NPC(ch))
        return;

    if (arg1.empty() || arg2.empty()) {
        char_printf(ch, "Usage: rename <player name> <new name>\n");
        return;
    }

    victim = is_playing(arg1);

    if (!victim) {
        char_printf(ch, "You can only rename a player currently logged in.\n");
        return;
    }

    if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
        char_printf(ch, "You don't have permission to change that name.\n");
        return;
    }

    auto tmp_name = _parse_name(arg2);
    if (tmp_name.empty() || tmp_name.length() < 2 || tmp_name.length() > MAX_NAME_LENGTH || !is_valid_name(tmp_name) ||
        fill_word(tmp_name) || reserved_word(tmp_name)) {
        char_printf(ch, "The new name is invalid.\n");
        return;
    }

    if ((player_i = get_id_by_name(tmp_name)) >= 0) {
        char_printf(ch, "There is already a player with that name.\n");
        return;
    }

    char_printf(ch, "&1&bYou have renamed &7{}&1 to &7{}&0\n", GET_NAME(victim), capitalize_first(tmp_name));
    log(LogSeverity::Stat, LVL_HEAD_C, "(GC) {} has renamed {} to {}", GET_NAME(ch), GET_NAME(victim), tmp_name);

    rename_player(victim, tmp_name);

    char_printf(victim, "&1&b!!! You have been renamed to &7{}&1.&0\n", GET_NAME(victim));
}

ACMD(do_return) {
    if (POSSESSED(ch)) {
        if (GET_LEVEL(POSSESSOR(ch)) < 101 && subcmd == 0) {
            char_printf(ch, "Huh?!?\n");
            return;
        }
        if (GET_COOLDOWN(ch, CD_SHAPECHANGE) < 0 && subcmd == 0) {
            char_printf(ch, "Use 'shapechange me' to return to your body.\n");
            return;
        }

        char_printf(ch, "You return to your original body.\n");

        /* if someone switched into your original body, disconnect them */
        if (ch->desc->original->desc)
            close_socket(ch->desc->original->desc);

        ch->desc->character = ch->desc->original;
        ch->desc->original = nullptr;

        ch->desc->character->desc = ch->desc;
        ch->desc = nullptr;
    } else
        char_printf(ch, "Huh?!?\n");
}

ACMD(do_load) {
    CharData *mob;
    ObjData *obj;
    int r_num;

    auto load_type = argument.shift();
    auto number_opt = argument.try_shift_number();

    if (load_type.empty() || !number_opt) {
        char_printf(ch, "Usage: load { obj | mob } <number>\n");
        return;
    }
    auto number = *number_opt;
    if (matches_start(load_type, "mob")) {
        if ((r_num = real_mobile(number)) < 0) {
            char_printf(ch, "There is no monster with that number.\n");
            return;
        }
        mob = read_mobile(r_num, REAL);
        char_to_room(mob, ch->in_room);

        act("$n makes a quaint, magical gesture with one hand.", true, ch, 0, 0, TO_ROOM);
        act("$n has created $N!", false, ch, 0, mob, TO_ROOM);
        act("You create $N.", false, ch, 0, mob, TO_CHAR);
        log(LogSeverity::Stat, std::max<int>(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} loads mob,  {}", GET_NAME(ch),
            GET_NAME(mob));
        load_mtrigger(mob);
    } else if (matches_start(load_type, "obj")) {
        if ((r_num = real_object(number)) < 0) {
            char_printf(ch, "There is no object with that number.\n");
            return;
        }
        obj = read_object(r_num, REAL);
        /*obj_to_room(obj, ch->in_room); */
        obj_to_char(obj, ch);
        act("$n makes a strange magical gesture.", true, ch, 0, 0, TO_ROOM);
        act("$n has created $p!", true, ch, obj, 0, TO_ROOM);
        act("You create $p.", false, ch, obj, 0, TO_CHAR);
        log(LogSeverity::Stat, std::max<int>(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} loads OBJ,  {}", GET_NAME(ch),
            (obj)->short_description);
    } else
        char_printf(ch, "That'll have to be either 'obj' or 'mob'.\n");
}

/* clean a room of all mobiles and objects */
ACMD(do_purge) {
    CharData *vict, *next_v;
    ObjData *obj, *next_o;

    auto buf = argument.shift();

    if (!buf.empty()) { /* argument supplied. destroy single object or char */
        if ((vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf)))) {
            if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict)) && (GET_LEVEL(ch) >= LVL_HEAD_B)) {
                char_printf(ch, "Fuuuuuuuuu!\n");
                return;
            }
            act("$n disintegrates $N.", false, ch, 0, vict, TO_NOTVICT);
            act("$n disintegrates YOU!", true, ch, 0, vict, TO_VICT);
            if (!IS_NPC(vict)) {
                log(LogSeverity::Warn, LVL_GOD, "(GC) {} has purged {}.", GET_NAME(ch), GET_NAME(vict));
                if (vict->desc) {
                    close_socket(vict->desc);
                    vict->desc = nullptr;
                }
                remove_player_from_game(vict, QUIT_PURGE);
            } else
                extract_char(vict);
        } else if ((obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, buf)))) {
            act("$n destroys $p.", false, ch, obj, 0, TO_ROOM);
            extract_obj(obj);
        } else {
            char_printf(ch, "Nothing here by that name.\n");
            return;
        }
        char_printf(ch, OK);
    } else { /* no argument. clean out the room */
        act("$n gestures... You are surrounded by scorching flames!", false, ch, 0, 0, TO_ROOM);
        room_printf(ch->in_room, "The world seems a little cleaner.\n");
        for (vict = world[ch->in_room].people; vict; vict = next_v) {
            next_v = vict->next_in_room;
            if (IS_NPC(vict))
                extract_char(vict);
        }
        for (obj = world[ch->in_room].contents; obj; obj = next_o) {
            next_o = obj->next_content;
            if (GET_OBJ_WEAR(obj) != 0) { /* This prevents items w/o take flags from
                                             getting purged like in Fiery v1 RSD */
                extract_obj(obj);
            }
        }
    }
}

ACMD(do_advance) {
    CharData *victim;

    auto name = argument.shift();
    auto level = argument.try_shift_number();

    if (name.empty()) {
        char_printf(ch, "Advance who?\n");
        return;
    }
    if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, name)))) {
        char_printf(ch, "That player is not here.\n");
        return;
    }

    if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
        char_printf(ch, "Maybe that's not such a great idea.\n");
        return;
    }
    if (IS_NPC(victim)) {
        char_printf(ch, "NO!  Not on NPC's.\n");
        return;
    }
    if (!level || *level <= 0) {
        char_printf(ch, "That's not a level!\n");
        return;
    }
    int newlevel = *level;
    if (newlevel > LVL_IMPL) {
        char_printf(ch, "{} is the highest possible level.\n", LVL_IMPL);
        return;
    }

    if (newlevel > GET_LEVEL(ch)) {
        char_printf(ch, "Yeah, right.\n");
        return;
    }
    if (newlevel == GET_LEVEL(victim)) {
        char_printf(ch, "They are already at that level.\n");
        return;
    }

    int oldlevel = GET_LEVEL(victim);
    if (newlevel < GET_LEVEL(victim)) {
        char_printf(ch,
                    "&0&9&bYou are momentarily enveloped by darkness!&0\n"
                    "&0&9&bYou feel somewhat diminished.&0\n");
    } else {
        act("$n makes some strange gestures.\n"
            "A strange feeling comes upon you,\n"
            "Like a giant hand, light comes down\n"
            "from above, grabbing your body, that\n"
            "begins to pulse with colored lights\n"
            "from inside.\n\n"
            "Your head seems to be filled with demons\n"
            "from another plane as your body dissolves\n"
            "to the elements of time and space itself.\n"
            "Suddenly a silent explosion of light\n"
            "snaps you back to reality.\n\n"
            "You feel slightly different.",
            false, ch, 0, victim, TO_VICT);
    }

    if (oldlevel >= LVL_IMMORT && newlevel < LVL_IMMORT) {
        /* If they are no longer an immortal, remove the immortal only flags. */
        REMOVE_FLAG(PRF_FLAGS(victim), PRF_LOG1);
        REMOVE_FLAG(PLR_FLAGS(victim), PRF_LOG2);
        REMOVE_FLAG(PRF_FLAGS(victim), PRF_NOHASSLE);
        REMOVE_FLAG(PLR_FLAGS(victim), PRF_HOLYLIGHT);
        REMOVE_FLAG(PLR_FLAGS(victim), PRF_SHOWVNUMS);
        REMOVE_FLAG(PLR_FLAGS(victim), PRF_ROOMVIS);
        REMOVE_FLAG(PLR_FLAGS(victim), PRF_ROOMVIS);
    } else if (oldlevel < LVL_IMMORT && newlevel >= LVL_IMMORT) {
        SET_FLAG(PRF_FLAGS(victim), PRF_HOLYLIGHT);
        SET_FLAG(PRF_FLAGS(victim), PRF_SHOWVNUMS);
        SET_FLAG(PRF_FLAGS(victim), PRF_AUTOEXIT);
        for (int i = 1; i <= MAX_SKILLS; i++)
            SET_SKILL(victim, i, 100);
        /* Make sure they have an empty olc zone list */
        free_olc_zone_list(victim);
    }

    char_printf(ch, OK);

    if (newlevel < oldlevel)
        log(LogSeverity::Stat, std::max<int>(LVL_IMMORT, GET_LEVEL(ch)),
            "(GC) {} has demoted {} from level {:d} to {:d}.", GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
    else
        log(LogSeverity::Stat, std::max<int>(LVL_IMMORT, GET_LEVEL(ch)),
            "(GC) {} has advanced {} to level {:d} (from {:d})", GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

    gain_exp(victim, exp_next_level(newlevel - 1, GET_CLASS(victim)) - GET_EXP(victim) + 1, GAIN_IGNORE_ALL);
    save_player_char(victim);
}

void perform_restore(CharData *vict) {
    extern void dispel_harmful_magic(CharData * ch);

    dispel_harmful_magic(vict);
    GET_HIT(vict) = std::max(GET_MAX_HIT(vict), GET_HIT(vict));
    GET_MANA(vict) = std::max(GET_MAX_MANA(vict), GET_MANA(vict));
    GET_MOVE(vict) = std::max(GET_MAX_MOVE(vict), GET_MOVE(vict));
    cure_laryngitis(vict);

    /* Since we didn't call alter_hit, which calls hp_pos_check */
    hp_pos_check(vict, nullptr, 0);
}

ACMD(do_restore) {
    CharData *vict;

    if (environment == ENV_PROD && GET_LEVEL(ch) < LVL_RESTORE) {
        char_printf(ch, "You are not godly enough for that!\n");
        return;
    }

    auto buf = argument.shift();
    if (buf.empty())
        char_printf(ch, "Whom do you wish to restore?\n");
    else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, buf))))
        char_printf(ch, NOPERSON);
    else {
        char_printf(ch, "They gasp as their body is fully healed.\n");
        act("You have been fully healed by $N!", false, vict, 0, ch, TO_CHAR | TO_SLEEP);
        perform_restore(vict);
        if ((GET_LEVEL(ch) >= LVL_GRGOD) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
            for (int i = 1; i <= TOP_SKILL; i++)
                SET_SKILL(vict, i, 1000);
            effect_total(vict);
        }
    }
}

void perform_pain(CharData *vict) {
    int change;

    change = GET_HIT(vict) * 0.1;
    hurt_char(vict, nullptr, change, true);
    change = GET_MOVE(vict) * 0.1;
    alter_move(vict, change);
}

ACMD(do_pain) {
    CharData *vict;

    auto buf = argument.shift();
    if (buf.empty())
        char_printf(ch, "To whom do you wish to cause pain?\n");
    else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, buf))))
        char_printf(ch, NOPERSON);
    else {
        int change = GET_HIT(vict) * 0.1;
        perform_pain(vict);
        char_printf(ch, "Their body writhes in pain.  Your displeasure has been known. ({})\n", change);
        act("A wave of pain and pestilence sent by $N harms you!", false, vict, 0, ch, TO_CHAR | TO_SLEEP);
    }
}

void perform_immort_vis(CharData *ch) {
    void appear(CharData * ch);

    if (GET_INVIS_LEV(ch) == 0 && !EFF_FLAGGED(ch, EFF_INVISIBLE) && !IS_HIDDEN(ch) &&
        !EFF_FLAGGED(ch, EFF_CAMOUFLAGED)) {
        char_printf(ch, "You are already fully visible.\n");
        return;
    }

    GET_INVIS_LEV(ch) = 0;
    appear(ch);
    char_printf(ch, "You are now fully visible.\n");
}

void perform_immort_invis(CharData *ch, int level) {
    CharData *tch;

    if (IS_NPC(ch))
        return;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
        if (tch == ch)
            continue;
        if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
            act("You blink and suddenly realize that $n is gone.", false, ch, 0, tch, TO_VICT);
        if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
            act("You suddenly realize that $n is standing beside you.", false, ch, 0, tch, TO_VICT);
    }

    GET_INVIS_LEV(ch) = level;
    char_printf(ch, "Your invisibility level is {:d}.\n", level);
}

ACMD(do_invis) {
    if (IS_NPC(ch)) {
        char_printf(ch, "You can't do that!\n");
        return;
    }

    auto level_opt = argument.try_shift_number();
    if (!level_opt) {
        if (GET_INVIS_LEV(ch) > 0)
            perform_immort_vis(ch);
        else
            perform_immort_invis(ch, GET_LEVEL(ch));
    } else {
        int level = *level_opt;
        if (level > GET_LEVEL(ch))
            char_printf(ch, "You can't go invisible above your own level.\n");
        else if (level < 1)
            perform_immort_vis(ch);
        else
            perform_immort_invis(ch, level);
    }
}

ACMD(do_gecho) {
    DescriptorData *pt;

    if (argument.empty())
        char_printf(ch, "That must be a mistake...\n");
    else {
        auto msg = argument.get();
        for (pt = descriptor_list; pt; pt = pt->next)
            if (!pt->connected && pt->character && pt->character != ch)
                char_printf(pt->character, "{}&0\n", msg);
        if (PRF_FLAGGED(ch, PRF_NOREPEAT))
            char_printf(ch, OK);
        else
            char_printf(ch, "{}&0\n", msg);
    }
}

/* completely redone in order to add poofs to the pfile */
/* --Fingon                                             */
ACMD(do_poofset) {

    switch (subcmd) {
    case SCMD_POOFIN:
        if (argument.empty()) {
            GET_POOFIN(ch) = argument.get();
            char_printf(ch, "Your poofin is now: {}\n", GET_POOFIN(ch));
        } else
            char_printf(ch, "Your poofin is: {}\n", GET_POOFIN(ch));
        break;
    case SCMD_POOFOUT:
        if (argument.empty()) {
            GET_POOFOUT(ch) = argument.get();
            char_printf(ch, "Your poofin is now: {}\n", GET_POOFOUT(ch));
        } else
            char_printf(ch, "Your poofout is: {}\n", GET_POOFOUT(ch));
        break;
    default:
        return;
        break;
    }
}

ACMD(do_dc) {
    DescriptorData *d = nullptr;

    auto arg = argument.shift();
    if (is_integer(arg)) {
        int num_to_dc = svtoi(arg);
        for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next)
            ;
        if (!d) {
            char_printf(ch, "No such connection.\n");
            return;
        }
    } else if (!arg.empty())
        for (d = descriptor_list; d; d = d->next) {
            if (d->character && !GET_NAME(d->character).empty() && matches(GET_NAME(d->character), arg))
                break;
            if (d->original && !GET_NAME(d->original).empty() && matches(GET_NAME(d->original), arg))
                break;
        }

    if (!d) {
        char_printf(ch, "Usage: DC <connection number | character name> (type USERS for a list)\n");
        return;
    }

    if ((d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) ||
        (d->original && GET_LEVEL(d->original) >= GET_LEVEL(ch))) {
        char_printf(ch, "Umm.. maybe that's not such a good idea...\n");
        return;
    }
    close_socket(d);
    char_printf(ch, "Connection #{:d} closed.\n", d->desc_num);
    log("(GC) Connection closed by {}.", GET_NAME(ch));
}

ACMD(do_wizlock) {
    int value;
    std::string_view when;

    auto arg = argument.try_shift_number();
    if (arg) {
        value = *arg;
        if (value < 0 || value > GET_LEVEL(ch)) {
            char_printf(ch, "Invalid wizlock value.\n");
            return;
        }
        should_restrict = value;
        when = "now";

        if (should_restrict)
            restrict_reason = RESTRICT_MANUAL;
        else
            restrict_reason = RESTRICT_NONE;
    } else
        when = "currently";

    switch (should_restrict) {
    case 0:
        char_printf(ch, "The game is {} completely open.\n", when);
        break;
    case 1:
        char_printf(ch, "The game is {} closed to new players.\n", when);
        break;
    default:
        char_printf(ch, "Only level {:d} and above may enter the game {}.\n", should_restrict, when);
        break;
    }
    if (!arg && restrict_reason == RESTRICT_AUTOBOOT)
        char_printf(ch, "The restriction was set automatically by the automatic rebooting system.\n");
}

ACMD(do_date) {
    time_t mytime;
    int d, h, m;
    extern time_t *boot_time;

    if (subcmd == SCMD_DATE)
        mytime = time(nullptr);
    else
        mytime = boot_time[0];

    std::string tmstr = std::ctime(&mytime);
    tmstr.pop_back(); // Remove the newline character

    if (subcmd == SCMD_DATE)
        char_printf(ch, "Current machine time: {}\n", tmstr);
    else {
        mytime = time(nullptr) - boot_time[0];
        d = mytime / 86400;
        h = (mytime / 3600) % 24;
        m = (mytime / 60) % 60;
        char_printf(ch, "Up since {}: {:d} day{}, {:d}:{:02d}\n", tmstr, d, ((d == 1) ? "" : "s"), h, m);
    }
}

ACMD(do_force) {
    DescriptorData *i, *next_desc;
    CharData *vict, *next_force;

    auto who = argument.shift();
    auto command = argument.get();

    std::string output = fmt::format("$n has forced you to '{}'.", command);

    if (who.empty())
        char_printf(ch, "Whom do you wish to force do what?\n");
    else if (command.empty())
        char_printf(ch, "What do you wish them to do?\n");
    else if ((GET_LEVEL(ch) < LVL_GRGOD) || (!matches("all", who) && !matches("room", who))) {
        if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, who))))
            char_printf(ch, NOPERSON);
        else if (GET_LEVEL(ch) <= GET_LEVEL(vict))
            char_printf(ch, "No, no, no!\n");
        else {
            char_printf(ch, OK);
            act(output, false, ch, nullptr, vict, TO_VICT);
            log(LogSeverity::Stat, std::max<int>(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} forced {} to {}", GET_NAME(ch),
                GET_NAME(vict), command);
            command_interpreter(vict, command);
        }
    } else if (matches("room", who)) {
        char_printf(ch, OK);
        log(LogSeverity::Stat, std::max<int>(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} forced room {:d} to {}",
            GET_NAME(ch), world[ch->in_room].vnum, command);

        for (vict = world[ch->in_room].people; vict; vict = next_force) {
            next_force = vict->next_in_room;
            if (GET_LEVEL(vict) >= GET_LEVEL(ch))
                continue;
            act(output, false, ch, nullptr, vict, TO_VICT);
            command_interpreter(vict, command);
        }
    } else { /* force all */
        char_printf(ch, OK);
        log(LogSeverity::Stat, std::max<int>(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} forced all to {}", GET_NAME(ch),
            command);

        for (i = descriptor_list; i; i = next_desc) {
            next_desc = i->next;

            if (i->connected || !(vict = i->character) || GET_LEVEL(vict) >= GET_LEVEL(ch))
                continue;
            act(output, false, ch, nullptr, vict, TO_VICT);
            command_interpreter(vict, command);
        }
    }
}

ACMD(do_wiznet) {
    DescriptorData *d;
    char any = false;
    bool explicit_level = false;
    int level = LVL_IMMORT;

    if (argument.empty()) {
        char_printf(ch,
                    "Usage: \n"
                    "   wiznet <text> - Send a message over wiznet to all gods.\n "
                    "   wiznet #<level> <text> - Send message to all gods above <level>\n"
                    "   wiznet @@ - Show online gods.\n");
        return;
    }

    auto command = argument.command_shift(true);
    if (!command.empty()) {
        switch (command[0]) {
        case '#': {
            auto level_opt = argument.try_shift_number();
            if (level_opt) {
                level = std::max(*level_opt, 1);
                if (level > GET_LEVEL(ch)) {
                    char_printf(ch, "You can't wiznet above your own level.\n");
                    return;
                }
                explicit_level = true;
            }
        } break;
        case '@': {
            std::string buf;
            for (d = descriptor_list; d; d = d->next) {
                if (!d->connected && GET_LEVEL(d->character) >= LVL_IMMORT && !PRF_FLAGGED(d->character, PRF_NOWIZ) &&
                    (CAN_SEE(ch, d->character) || GET_LEVEL(ch) == LVL_IMPL)) {
                    if (!any) {
                        buf = "Gods online:\n";
                        any = true;
                    }
                    buf += "  " + GET_NAME(d->character);
                    if (PLR_FLAGGED(d->character, PLR_WRITING))
                        buf += " (Writing)\n";
                    else if (EDITING(d))
                        buf += " (Writing)\n";
                    else if (PLR_FLAGGED(d->character, PLR_MAILING))
                        buf += " (Writing mail)\n";
                    else
                        buf += "\n";
                }
            }
            any = false;
            for (d = descriptor_list; d; d = d->next) {
                if (!d->connected && GET_LEVEL(d->character) >= LVL_IMMORT && PRF_FLAGGED(d->character, PRF_NOWIZ) &&
                    CAN_SEE(ch, d->character)) {
                    if (!any) {
                        buf += "Gods offline:\n";
                        any = true;
                    }
                    buf += "  " + GET_NAME(d->character) + "\n";
                }
            }
            char_printf(ch, buf);
            return;
        } break;
        case '\\':
            break;
        default:
            break;
        }
    }

    if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
        char_printf(ch, "You are offline!\n");
        return;
    }

    if (argument.empty()) {
        char_printf(ch, "Don't bother the gods like that!\n");
        return;
    }

    for (d = descriptor_list; d; d = d->next) {
        if (!IS_PLAYING(d))
            continue;
        if (STATE(d) != CON_PLAYING || PLR_FLAGGED(d->character, PLR_WRITING) ||
            PLR_FLAGGED(d->character, PLR_MAILING) || EDITING(d))
            if (!PRF_FLAGGED(d->character, PRF_OLCCOMM))
                continue;
        if (d->original ? GET_LEVEL(d->original) < level : GET_LEVEL(d->character) < level)
            continue;
        if (PRF_FLAGGED(d->character, PRF_NOWIZ))
            continue;
        if (d == ch->desc && PRF_FLAGGED(d->character, PRF_NOREPEAT))
            continue;
        if (CAN_SEE(d->character, ch)) {
            if (explicit_level)
                char_printf(d->character, "&6{}: <{}> {}&0\n", GET_NAME(ch), level, argument);
            else
                char_printf(d->character, "&6{}: {}&0\n", GET_NAME(ch), argument);

        } else if (explicit_level)
            char_printf(d->character, "&6Someone: <{}> {}&0\n", level, argument);
        else
            char_printf(d->character, "&6Someone: {}&0\n", argument);
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        char_printf(ch, OK);
}

/* added for name approval system by fingh 3/31/99 */
/* Complete name approval system redone by Zantir/David Endre 5/4/99 */
void broadcast_name(std::string_view name) {
    DescriptorData *d;
    for (d = descriptor_list; d; d = d->next)
        if (IS_PLAYING(d) && d->character && GET_LEVEL(d->character) >= LVL_IMMORT &&
            !PLR_FLAGGED(d->character, PLR_WRITING) && !PLR_FLAGGED(d->character, PLR_MAILING) && !EDITING(d))
            string_to_output(d, "&2&b{} is awaiting name approval.&0\n", name);
}

constexpr std::array<std::string_view, 9> reasons = {
    "Compound words. Examples: LadyJade, BraveBlade, ImmortalSoul\n"
    "These types of names give a player an unearned arrogance above\n"
    "other players and in some instances above the gods.",
    "Offensive words or names, mispelled or backwards. Eg: Sgurd Kcuf Emod",
    "Names from known mythos, movies etc... Examples: Feyd, Conan, Zeus\n "
    "Come on, you can be more creative than reliance on an existing known\n "
    "character.",
    "Names that do not fit the character you are playing. Examples:\n"
    "A human named Gruzel\n A troll named Bob\n"
    "A boy named Sue\n"
    "An orc named Ripenthiel",
    "No offensive words or names from other languages or cultures.",
    "No Nouns verbs or adverbs. Eg. Jester, Red, Dog, Bloody, Hotly, "
    "Freedom\n Pervert, PC, Trouble, McIntosh, Desperado.  Sorry guys LAME.",
    "No names that resemble common nouns verbs or adverbs. Eg. Jesterx,\n "
    "Redx, Chamelion, Desperato etc...",
    "No modern names. Examples: Rina John Mike Steve",
    "Other...Please talk to a god for further information."};

ACMD(do_name) {
    DescriptorData *d;
    int choice = 0;
    int z = 0;

    switch (subcmd) {
    case SCMD_ACCEPT: {
        auto arg = argument.shift();
        if (arg.empty()) {
            char_printf(ch, "Accept WHO?\n");
            return;
        }
        /* cycle through the current connections */
        for (d = descriptor_list; d; d = d->next) {

            /* added to stop crashes while people are at CON_QANSI etc. */
            if (!d->character)
                continue;

            // make sure player is either at the CON_NAME_WAIT_APPROVAL state or has been auto-accepted and still
            // pending approval and the player is not yourself
            if (((STATE(d) == CON_NAME_WAIT_APPROVAL) || (PLR_FLAGGED(d->character, PLR_NAPPROVE))) &&
                ((d != ch->desc) && matches(GET_NAME(d->character), arg))) {
                /* create the player in the pfile if he isn't there already */
                if (GET_PFILEPOS(d->character) < 0) {
                    GET_PFILEPOS(d->character) = create_player_index_entry(GET_NAME(d->character));
                    /* initialize the the player */
                    init_player(d->character);
                    /* log the new player */
                    log(LogSeverity::Stat, LVL_IMMORT, "{} [{}] new player.", GET_NAME(d->character), d->host);
                }
                /* accept the players name */
                REMOVE_FLAG(PLR_FLAGS(d->character), PLR_NAPPROVE);
                save_player_char(d->character);

                /* log the acceptance of the name */
                log(LogSeverity::Stat, LVL_IMMORT, "The name: {} has been accepted by {}.", GET_NAME(d->character),
                    GET_NAME(ch));

                /* remove the choose new name flag */
                if (PLR_FLAGGED(d->character, PLR_NEWNAME))
                    REMOVE_FLAG(PLR_FLAGS(d->character), PLR_NEWNAME);

                /* check if the player is waiting approval or already auto approved */
                if (STATE(d) == CON_NAME_WAIT_APPROVAL) {
                    /* send the motd and change connection states */
                    string_to_output(d, get_text(TEXT_MOTD));
                    string_to_output(d, "\n\n*** PRESS RETURN: ");
                    STATE(d) = CON_RMOTD;
                }

                /* tell the player the name has been accepted */
                char_printf(ch, "&2&bThe name: {} has been accepted.&0\n", GET_NAME(d->character));
                return;
            }
        }
        char_printf(ch, "No such player waiting for approval.\n");
    } break;
    case SCMD_DECLINE: {
        auto arg = argument.shift();
        auto arg2 = argument.try_shift_number();

        if (arg.empty()) {
            char_printf(ch, "Decline WHO?\n");
            return;
        }
        if (!arg2 || *arg2 < 1 || *arg2 > reasons.size()) {
            char_printf(ch, "For what reason? Type 'nlist 0' for a list.\n");
            return;
        }

        choice = *arg2;
        /* cycle through connected players */
        for (d = descriptor_list; d; d = d->next) {
            /* added to stop crashes while people are at CON_QANSI etc. */
            if (!d->character)
                continue;

            if (((STATE(d) == CON_NAME_WAIT_APPROVAL) || (PLR_FLAGGED(d->character, PLR_NAPPROVE))) && d != ch->desc &&
                matches(GET_NAME(d->character), arg)) {
                log(LogSeverity::Stat, LVL_IMMORT, "The name: {} has been declined by {}, reason {:d}.",
                    GET_NAME(d->character), GET_NAME(ch), choice);
                char_printf(ch, "&2&bThe name: {} has been declined.&0\n", GET_NAME(d->character));
                send_to_xnames(GET_NAME(d->character));
                string_to_output(d, "That name has been rejected, because it breaks this rule:\n");
                string_to_output(d, "{}\n", reasons[choice - 1]);
                SET_FLAG(PLR_FLAGS(d->character), PLR_NEWNAME);
                if (STATE(d) == CON_NAME_WAIT_APPROVAL) {
                    string_to_output(d, "Please try another name.\n");
                    string_to_output(d, "Name: ");
                    STATE(d) = CON_NEW_NAME;
                } else {
                    string_to_output(d,
                                     "&6&bYou are welcome to play; however, you will be prompted&0\n"
                                     "&6&bfor a new name on your next login.&0\n");
                    save_player_char(d->character);
                }
                return;
            }
        }
        char_printf(ch, "No such player waiting for approval.\n");
    } break;
    case SCMD_LIST: {
        char_printf(ch, "The following people are awaiting name approval:\n");
        int count = 0;
        for (d = descriptor_list; d; d = d->next) {
            if (d->character) {
                if (((STATE(d) == CON_NAME_WAIT_APPROVAL) ||
                     (PLR_FLAGGED(d->character, PLR_NAPPROVE) && !(PLR_FLAGGED(d->character, PLR_NEWNAME)))) &&
                    (d != ch->desc)) {
                    char_printf(ch, GET_NAME(d->character) + "\n");
                    count++;
                }
            }
        }

        if (count == 0) {
            char_printf(ch, "No one.\n");
        }
    } break;
    default:
        break; /* maybe send an error to log here? */
    } /* end switch */
}

ACMD(do_zreset) {
    void reset_zone(int zone, byte pop);

    int i, j;

    auto arg = argument.shift();

    if (arg.empty()) {
        i = world[ch->in_room].zone;
    } else if (arg[0] == '*') {
        if (GET_LEVEL(ch) <= LVL_HEAD_B)
            return;
        for (i = 0; i <= top_of_zone_table; i++)
            reset_zone(i, false);
        char_printf(ch, "Reset world.\n");
        log(LogSeverity::Stat, std::max(LVL_GRGOD, GET_INVIS_LEV(ch)), "(GC) {} reset entire world.", GET_NAME(ch));
        return;
    } else if (!is_integer(arg)) {
        char_printf(ch, "Usage: zreset [<zone-number>]\n");
        return;
    } else {
        j = svtoi(arg);
        for (i = 0; i <= top_of_zone_table; i++)
            if (zone_table[i].number == j)
                break;
    }
    if (GET_LEVEL(ch) < LVL_GRGOD && !has_olc_access(ch, zone_table[i].number)) {
        char_printf(ch, "Testing testing !!are you clear to do this!! no I dont think so\n");
        return;
    }
    if (i >= 0 && i <= top_of_zone_table) {
        reset_zone(i, false);
        char_printf(ch, "Reset zone {:d} (#{:d}): {}.\n", i, zone_table[i].number, zone_table[i].name);
        log(LogSeverity::Stat, std::max(LVL_GRGOD, GET_INVIS_LEV(ch)), "(GC) {} reset zone {:d} ({})", GET_NAME(ch),
            zone_table[i].number, zone_table[i].name);
    } else
        char_printf(ch, "Invalid zone number.\n");
}

static std::string_view center_wiztitle(std::string_view wiztitle, int len, int noansi_len) {
    char centerbuf[MAX_INPUT_LENGTH];
    int i;
    bool add_zero;

    /* If the wiztitle doesn't end in &0, put it on. */
    add_zero = (len < 2 || wiztitle[len - 1] != '0' || (wiztitle[len - 2] != CREL && wiztitle[len - 2] != CABS));

    sprintf(centerbuf + ((WIZ_TITLE_WIDTH - noansi_len) / 2), "%s%s", wiztitle, add_zero ? "&0" : "");

    /* +2 for the &0 we just tacked on */
    if (add_zero)
        len += 2;

    for (i = 0; i < (WIZ_TITLE_WIDTH - noansi_len) / 2; ++i) {
        centerbuf[i] = ' ';
        centerbuf[WIZ_TITLE_WIDTH - 1 + (len - noansi_len) - i] = ' ';
    }

    /* if odd-lengthed string, need an extra space to cover up \0 */
    if (noansi_len & 1)
        centerbuf[WIZ_TITLE_WIDTH - 1 + (len - noansi_len) - i] = ' ';
    centerbuf[WIZ_TITLE_WIDTH + (len - noansi_len)] = '\0';

    return centerbuf;
}

void do_wiztitle(CharData *vict, Arguments argument) {
    auto new_title = argument.shift();

    int len = new_title.length();
    int noansi_len = len - count_color_chars(new_title);

    if (IS_NPC(vict)) {
        char_printf(vict, "Mobiles do not have godly titles...nice try.\n");
    } else if (new_title.find('[') != std::string::npos || new_title.find(']') != std::string::npos) {
        char_printf(vict, "Godly Titles can't contain the [ or ] characters.\n");
    } else if (len >= MAX_INPUT_LENGTH) {
        char_printf(vict, "Sorry, godly titles can't be longer than {} characters.\n", MAX_INPUT_LENGTH - 1);
    } else if (noansi_len > WIZ_TITLE_WIDTH) {
        char_printf(vict, "Sorry, text portion of godly titles can't be longer than 12 characters.\n");
    } else if (noansi_len == 0) {
        GET_WIZ_TITLE(vict).clear();
        char_printf(vict, "Okay, {}'s godly title reset to default.\n", GET_NAME(vict));
    } else {
        GET_WIZ_TITLE(vict) = center_wiztitle(new_title, len, noansi_len);
        char_printf(vict, "Okay, {}'s godly title is now [{}]\n", GET_NAME(vict), GET_WIZ_TITLE(vict));
    }
}

/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil) {
    CharData *vict;
    long result;

    auto arg = argument.shift();

    if (arg.empty())
        char_printf(ch, "Yes, but for whom?!?\n");
    else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg))))
        char_printf(ch, "There is no such player.\n");
    else if (IS_NPC(vict))
        char_printf(ch, "You can't do that to a mob!\n");
    else if (GET_LEVEL(vict) > GET_LEVEL(ch))
        char_printf(ch, "Hmmm...you'd better not.\n");
    else {
        switch (subcmd) {
        case SCMD_REROLL:
            char_printf(ch, "Rerolled not functinal for now\n");
            break;

            // log("(GC) {} has rerolled {}.", GET_NAME(ch), GET_NAME(vict));
            // char_printf(ch, "New stats: Str {:d}, Int {:d}, Wis {:d}, Dex {:d}, Con {:d}, Cha {:d}\n ",
            // GET_STR(vict),
            //             GET_INT(vict), GET_WIS(vict), GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
            // break;
        case SCMD_PARDON:
            if (!PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(vict, PLR_KILLER)) {
                char_printf(ch, "Your victim is not flagged.\n");
                return;
            }
            REMOVE_FLAG(PLR_FLAGS(vict), PLR_THIEF);
            REMOVE_FLAG(PLR_FLAGS(vict), PLR_KILLER);
            char_printf(ch, "Pardoned.\n");
            char_printf(vict, "You have been pardoned by the Gods!\n");
            log(LogSeverity::Warn, std::max<int>(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} pardoned by {}", GET_NAME(vict),
                GET_NAME(ch));
            break;
        case SCMD_NOTITLE:
            result = PLR_TOG_CHK(vict, PLR_NOTITLE);
            log(LogSeverity::Stat, std::max<int>(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) Notitle {} for {} by {}.",
                ONOFF(result), GET_NAME(vict), GET_NAME(ch));
            char_printf(ch, "\n");
            break;
        case SCMD_SQUELCH:
            result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
            log(LogSeverity::Warn, std::max<int>(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) Squelch {} for {} by {}.",
                ONOFF(result), GET_NAME(vict), GET_NAME(ch));
            char_printf(ch, "\n");
            break;
        case SCMD_FREEZE:
            if (ch == vict) {
                char_printf(ch, "Oh, yeah, THAT'S real smart...\n");
                return;
            }
            if (PLR_FLAGGED(vict, PLR_FROZEN)) {
                char_printf(ch, "Your victim is already pretty cold.\n");
                return;
            }
            SET_FLAG(PLR_FLAGS(vict), PLR_FROZEN);
            GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
            char_printf(
                vict, "A bitter wind suddenly rises and drains every erg of heat from your body!\nYou feel frozen!\n");
            char_printf(ch, "Frozen.\n");
            act("A sudden cold wind conjured from nowhere freezes $n!", false, vict, 0, 0, TO_ROOM);
            log(LogSeverity::Warn, std::max(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} frozen by {}.", GET_NAME(vict),
                GET_NAME(ch));
            break;
        case SCMD_THAW:
            if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
                char_printf(ch, "Sorry, your victim is not morbidly encased in ice at the moment.\n");
                return;
            }
            if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
                char_printf(ch, "Sorry, a level {:d} God froze {}... you can't unfreeze {}.\n", GET_FREEZE_LEV(vict),
                            GET_NAME(vict), HMHR(vict));
                return;
            }
            log(LogSeverity::Warn, std::max(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} un-frozen by {}.", GET_NAME(vict),
                GET_NAME(ch));
            REMOVE_FLAG(PLR_FLAGS(vict), PLR_FROZEN);
            char_printf(vict, "A fireball suddenly explodes in front of you, melting the ice!\nYou feel thawed.\n");
            char_printf(ch, "Thawed.\n");
            act("A sudden fireball conjured from nowhere thaws $n!", false, vict, 0, 0, TO_ROOM);
            break;
        case SCMD_UNAFFECT:
            if (vict->effects) {
                while (vict->effects)
                    effect_remove(vict, vict->effects);
                char_printf(vict,
                            "There is a brief flash of light!\n"
                            "You feel slightly different.\n");
                char_printf(ch, "All spells removed.\n");
                check_regen_rates(vict);
                /* if it had been an animated and now isnt then it needs to die */
                if (MOB_FLAGGED(vict, MOB_ANIMATED) && !EFF_FLAGGED(vict, EFF_ANIMATED)) {
                    act("$n freezes and falls twitching to the ground.", false, vict, 0, 0, TO_ROOM);
                    die(vict, nullptr);
                    return;
                }
            } else {
                char_printf(ch, "Your victim does not have any effects!\n");
                return;
            }
            break;
        case SCMD_BLESS:
            if IS_EVIL (vict) {
                mag_affect(100, ch, vict, SPELL_DARK_PRESENCE, 0, 0);
            } else {
                mag_affect(100, ch, vict, SPELL_BLESS, 0, 0);
            }
            mag_affect(100, ch, vict, SPELL_FLY, 0, 0);
            mag_affect(100, ch, vict, SPELL_STONE_SKIN, 0, 0);
            mag_affect(100, ch, vict, SPELL_BLUR, 0, 0);
            mag_affect(100, ch, vict, SPELL_BARKSKIN, 0, 0);
            perform_restore(vict);
            act("$N has been blessed by $n!", false, ch, 0, vict, TO_ROOM);
            break;
        default:
            log("SYSERR: Unknown subcmd passed to do_wizutil (act.wizard.c)");
            break;
        }
        save_player_char(vict);
    }
}

#define PC 1
#define NPC 2
#define BOTH 3

#define MISC 0
#define BINARY 1
#define NUMBER 2

#define SET_OR_REMOVE(flagset, flags)                                                                                  \
    do {                                                                                                               \
        if (on)                                                                                                        \
            SET_FLAG(flagset, flags);                                                                                  \
        else if (off)                                                                                                  \
            REMOVE_FLAG(flagset, flags);                                                                               \
    } while (0)

ACMD(do_set) {
    int i, l;
    CharData *vict = nullptr, *cbuf = nullptr;
    int on = 0, off = 0, value = 0;
    char is_file = 0, is_player = 0, save = true;
    int player_i = 0;
    int get_race_align(CharData * ch);
    int parse_diety(CharData * ch, char arg);

    struct set_struct {
        const std::string_view cmd;
        const char level;
        const char pcnpc;
        const char type;
    } fields[] = {{"brief", LVL_ATTENDANT, PC, BINARY}, /* 0 */
                  {"", LVL_GAMEMASTER, PC, BINARY},
                  /* 1 */ /* was invstart, now UNUSED */
                  {"title", LVL_GAMEMASTER, PC, MISC},
                  {"nosummon", LVL_GAMEMASTER, PC, BINARY},
                  {"maxhit", LVL_GOD, BOTH, NUMBER},
                  {"maxmana", LVL_GOD, BOTH, NUMBER}, /* 5 */
                  {"maxmove", LVL_GOD, BOTH, NUMBER},
                  {"hitpoints", LVL_GOD, BOTH, NUMBER},
                  {"manapoints", LVL_GOD, BOTH, NUMBER},
                  {"movepoints", LVL_GOD, BOTH, NUMBER},
                  {"align", LVL_GOD, BOTH, NUMBER}, /*10 */
                  {"strength", LVL_GOD, BOTH, NUMBER},
                  {"stradd", LVL_GOD, BOTH, NUMBER},
                  {"intelligence", LVL_GOD, BOTH, NUMBER},
                  {"wisdom", LVL_GOD, BOTH, NUMBER},
                  {"dexterity", LVL_GOD, BOTH, NUMBER}, /*15 */
                  {"constitution", LVL_GOD, BOTH, NUMBER},
                  {"gender", LVL_GOD, BOTH, MISC},
                  {"ac", LVL_GOD, BOTH, NUMBER},
                  {"olc", LVL_ADMIN, PC, NUMBER},
                  {"innates", LVL_GOD, PC, NUMBER}, /*20 */
                  {"experience", LVL_ADMIN, BOTH, NUMBER},
                  {"hitroll", LVL_GOD, BOTH, NUMBER},
                  {"damroll", LVL_GOD, BOTH, NUMBER},
                  {"invis", LVL_GAMEMASTER, PC, NUMBER},
                  {"nohassle", LVL_GAMEMASTER, PC, BINARY}, /*25 */
                  {"frozen", LVL_FREEZE, PC, BINARY},
                  {"practices", LVL_GAMEMASTER, PC, NUMBER},
                  {"lessons", LVL_GAMEMASTER, PC, NUMBER},
                  {"drunk", LVL_GOD, BOTH, MISC},
                  {"hunger", LVL_GOD, BOTH, MISC}, /*30 */
                  {"thirst", LVL_GOD, BOTH, MISC},
                  {"killer", LVL_ADMIN, PC, BINARY},
                  {"thief", LVL_ADMIN, PC, BINARY},
                  {"level", LVL_ADMIN, BOTH, NUMBER},
                  {"room", LVL_ATTENDANT, BOTH, NUMBER}, /*35 */
                  {"roomflag", LVL_ADMIN, PC, BINARY},
                  {"siteok", LVL_GAMEMASTER, PC, BINARY},
                  {"deleted", LVL_IMPL, PC, BINARY},
                  {"class", LVL_ATTENDANT, BOTH, MISC},
                  {"nowizlist", LVL_ATTENDANT, PC, BINARY}, /*40 */
                  {"quest", LVL_GOD, PC, BINARY},
                  {"homeroom", LVL_GOD, PC, MISC},
                  {"color", LVL_GOD, PC, BINARY},
                  {"idnum", LVL_IMPL, PC, NUMBER},
                  {"passwd", LVL_GAMEMASTER, PC, MISC}, /*45 */
                  {"nodelete", LVL_ATTENDANT, PC, BINARY},
                  {"charisma", LVL_GOD, BOTH, NUMBER},
                  {"race", LVL_GOD, PC, MISC},
                  {"olc2", LVL_ADMIN, PC, NUMBER},
                  {"olc3", LVL_ADMIN, PC, NUMBER}, /*50 */
                  {"olc4", LVL_ADMIN, PC, NUMBER},
                  {"olc5", LVL_ADMIN, PC, NUMBER},
                  {"platinum", LVL_GOD, BOTH, NUMBER},
                  {"gold", LVL_GOD, BOTH, NUMBER},
                  {"silver", LVL_GOD, BOTH, NUMBER}, /*55 */
                  {"copper", LVL_GOD, BOTH, NUMBER},
                  {"pbank", LVL_ATTENDANT, PC, NUMBER},
                  {"gbank", LVL_ATTENDANT, PC, NUMBER},
                  {"sbank", LVL_ATTENDANT, PC, NUMBER},
                  {"cbank", LVL_ATTENDANT, PC, NUMBER}, /*60 */
                  {"UNUSED", LVL_IMPL, PC, NUMBER},
                  {"anon", LVL_ATTENDANT, PC, BINARY},
                  {"rename", LVL_GOD, PC, BINARY},
                  {"napprove", LVL_GOD, PC, BINARY},
                  {"holylight", LVL_ATTENDANT, PC, BINARY}, /*65 */
                  {"wiztitle", LVL_GOD, PC, MISC},
                  {"chant", LVL_GAMEMASTER, PC, NUMBER},
                  {"size", LVL_GOD, PC, MISC},
                  {"hiddenness", LVL_GOD, BOTH, NUMBER},
                  {"rage", LVL_GOD, BOTH, NUMBER}, /*70 */
                  {"ptitle", LVL_GAMEMASTER, PC, MISC},
                  {"height", LVL_GOD, BOTH, NUMBER},
                  {"weight", LVL_GOD, BOTH, NUMBER},
                  {"lifeforce", LVL_GOD, BOTH, MISC},
                  {"composition", LVL_GOD, BOTH, MISC}, /*75 */
                  {"music", LVL_GAMEMASTER, PC, NUMBER},
                  {"summon mount", LVL_GOD, PC, NUMBER},
                  {"\n", 0, BOTH, MISC}};

    auto name = argument.shift();
    if (matches(name, "file")) {
        is_file = 1;
        name = argument.shift();
    } else if (matches(name, "player")) {
        is_player = 1;
        name = argument.shift();
    } else if (matches(name, "mob")) {
        name = argument.shift();
    }
    auto field = argument.shift();

    if (name.empty() || field.empty()) {
        paging_printf(ch, "Usage: set <victim> <field> <value>\n");
        paging_printf(ch, "Set fields currently available to you:\n");
        for (l = 0; fields[l].cmd[0] != '\n'; l++) {
            if (fields[l].level <= GET_LEVEL(ch)) {
                paging_printf(ch, "{:<20} {:<20} {:<6}\n", fields[l].cmd,
                              (fields[l].pcnpc == PC ? "Player Only" : "Player or Mob"),
                              (fields[l].type == BINARY ? "Binary" : (fields[l].type == NUMBER ? "Number" : "Misc")));
            }
        }
        start_paging(ch);
        return;
    }
    if (!is_file) {
        if (is_player) {
            if (!(vict = find_char_around_char(ch, find_vis_plr_by_name(ch, name)))) {
                char_printf(ch, "There is no such player.\n");
                return;
            }
        } else {
            if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, name)))) {
                char_printf(ch, "There is no such creature.\n");
                return;
            }
        }
    } else if (is_file) {
        CREATE(cbuf, CharData, 1);
        clear_char(cbuf);
        CREATE(cbuf->player_specials, PlayerSpecialData, 1);
        if ((player_i = load_player(name, cbuf)) > -1) {
            if (GET_LEVEL(cbuf) >= GET_LEVEL(ch)) {
                free_char(cbuf);
                char_printf(ch, "Sorry, you can't do that.\n");
                return;
            }
            vict = cbuf;
        } else {
            free_char(cbuf);
            char_printf(ch, "There is no such player.\n");
            return;
        }
    }
    if (GET_LEVEL(ch) != LVL_HEAD_C) {
        if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
            char_printf(ch, "Maybe that's not such a great idea...\n");
            return;
        }
    }
    for (l = 0; fields[l].cmd[0] != '\n'; l++)
        if (matches(field, fields[l].cmd))
            break;

    if (GET_LEVEL(ch) < fields[l].level) {
        char_printf(ch, "You are not godly enough for that!\n");
        return;
    }
    if (IS_NPC(vict) && !(fields[l].pcnpc & NPC)) {
        char_printf(ch, "You can't do that to a beast!\n");
        return;
    } else if (!IS_NPC(vict) && !(fields[l].pcnpc & PC)) {
        char_printf(ch, "That can only be done to a beast!\n");
        return;
    }
    if (fields[l].type == BINARY) {
        auto value = argument.shift();
        if (matches(value, "on") || matches(value, "yes"))
            on = 1;
        else if (matches(value, "off") || matches(value, "no"))
            off = 1;
        if (!(on || off)) {
            char_printf(ch, "Value must be on or off.\n");
            return;
        }
    } else if (fields[l].type == NUMBER) {
        auto value_opt = argument.try_shift_number();
        if (!value_opt) {
            char_printf(ch, "Value must be a number.\n");
            return;
        }
        value = *value_opt;
    }

    std::string buf{OK};
    switch (l) {
    case 0:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
        break;
    case 1:
        char_printf(ch, "This option is unused.  You should not have seen this!\n");
        return;
    case 2:
        set_title(vict, argument.get());
        buf = fmt::format("{}'s title is now: {}\n", GET_NAME(vict), GET_TITLE(vict));
        break;
    case 3:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
        on = !on; /* so output will be correct */
        break;
    case 4: {
        if (!value) {
            char_printf(ch, "Value must be a number.\n");
            return;
        }
        /* Base hit is in a player special. Don't set for npcs */
        if (IS_NPC(vict))
            GET_MAX_HIT(vict) = std::clamp(value, 1, 500000);
        else {
            GET_BASE_HIT(vict) = std::clamp(value, 1, 500000);
            effect_total(vict);
        }
    } break;
    case 5:
        vict->points.max_mana = std::clamp(value, 1, 500000);
        effect_total(vict);
        break;
    case 6:
        vict->points.max_move = std::clamp(value, 1, 500000);
        effect_total(vict);
        break;
    case 7:
        vict->points.hit = std::clamp(value, -9, vict->points.max_hit);
        effect_total(vict);
        break;
    case 8:
        vict->points.mana = std::clamp(value, 0, vict->points.max_mana);
        effect_total(vict);
        break;
    case 9:
        vict->points.move = std::clamp(value, 0, vict->points.max_move);
        effect_total(vict);
        break;
    case 10:
        GET_ALIGNMENT(vict) = std::clamp(value, -1000, 1000);
        effect_total(vict);
        break;
    case 11:
        GET_NATURAL_STR(vict) = std::clamp(value, MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 12:
        /*  This is no longer used --Gurlaek 6/23/1999
           vict->view_abils.str_add = std::clamp(value, 0, 100);
           vict->view_abils.str_add = value;
           if (value > 0)
           vict->view_abils.str = 100;
           effect_total(vict);
         */
        break;
        /*Edited by Proky,  values for intel */
    case 13:
        GET_NATURAL_INT(vict) = std::clamp(value, MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 14:
        GET_NATURAL_WIS(vict) = std::clamp(value, MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 15:
        GET_NATURAL_DEX(vict) = std::clamp(value, MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 16:
        GET_NATURAL_CON(vict) = std::clamp(value, MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 17: {
        auto gender = argument.shift();
        if (matches(gender, "male"))
            vict->player.sex = SEX_MALE;
        else if (matches(gender, "female"))
            vict->player.sex = SEX_FEMALE;
        else if (matches(gender, "neutral"))
            vict->player.sex = SEX_NEUTRAL;
        else if (matches(gender, "nonbinary"))
            vict->player.sex = SEX_NONBINARY;
        else {
            char_printf(ch, "Must be 'male', 'female', 'nonbinary', or 'neutral'.\n");
            return;
        }
    } break;
    case 18:
        GET_AC(vict) = std::clamp(value, MIN_AC, MIN_AC);
        effect_total(vict);
        break;
    case 19: {
        OLCZoneList dummy, *zone, *next;
        dummy.next = GET_OLC_ZONES(vict);
        for (zone = &dummy, i = false; zone->next; zone = next) {
            next = zone->next;
            /* If in list, remove it */
            if (next->zone == value) {
                zone->next = next->next;
                free(next);
                i = true;
                buf = fmt::format("Removed {} from {} allowed OLC zones.\n", value, GET_NAME(vict));
                break;
            }
        }
        GET_OLC_ZONES(vict) = dummy.next;
        /* If i is false, we didn't find it, so add it */
        if (!i) {
            CREATE(zone, OLCZoneList, 1);
            zone->zone = value;
            zone->next = GET_OLC_ZONES(vict);
            GET_OLC_ZONES(vict) = zone;
            buf = fmt::format("Added {} to {} allowed OLC zones.\n", value, GET_NAME(vict));
        }
    } break;
    case 20:
        GET_COOLDOWN(vict, CD_INNATE_ASCEN) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_BRILL) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_CHAZ) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_SYLL) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_TASS) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_TREN) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_INVISIBLE) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_FEATHER_FALL) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_CREATE) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_DARKNESS) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_HARNESS) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_BREATHE) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_ILLUMINATION) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_FAERIE_STEP) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_BLINDING_BEAUTY) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_STATUE) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_INNATE_BARKSKIN) = std::clamp(value, 0, 100);
        break;
    case 21:
        vict->points.exp = std::clamp(value, 0, 299999999);
        break;
    case 22:
        GET_BASE_HITROLL(vict) = std::clamp(value, MIN_DAMROLL, MAX_DAMROLL);
        effect_total(vict);
        break;
    case 23:
        GET_BASE_DAMROLL(vict) = std::clamp(value, MIN_DAMROLL, MAX_DAMROLL);
        effect_total(vict);
        break;
    case 24:
        if (GET_LEVEL(ch) < LVL_HEAD_C && ch != vict) {
            buf = "You aren't godly enough for that!\n";
            save = false;
            break;
        }
        GET_INVIS_LEV(vict) = std::clamp<int>(value, 0, GET_LEVEL(vict));
        break;
    case 25:
        if (GET_LEVEL(ch) < LVL_HEAD_C && ch != vict) {
            buf = "You aren't godly enough for that!\n";
            save = false;
            break;
        }
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
        break;
    case 26:
        if (ch == vict) {
            char_printf(ch, "Better not -- could be a long winter!\n");
            /* OK: ch == vict so no memory was allocated */
            return;
        }
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
        break;
    case 27:
    case 28:
        /* GET_PRACTICES(vict) = std::clamp(value, 0, 100); */
        break;
    case 29:
    case 30:
    case 31: {
        auto val_arg = argument.shift();
        if (matches(val_arg, "off")) {
            GET_COND(vict, (l - 29)) = (char)-1;
            buf = fmt::format("{}'s {} now off.\n", GET_NAME(vict), fields[l].cmd);
        } else if (is_integer(val_arg)) {
            value = svtoi(val_arg);
            value = std::clamp(value, 0, 24);
            GET_COND(vict, (l - 29)) = (char)value;
            buf = fmt::format("{}'s {} set to {}.\n", GET_NAME(vict), fields[l].cmd, value);
        } else {
            buf = "Must be 'off' or a value from 0 to 24.\n";
            save = false;
            break;
        }
        check_regen_rates(vict);
    } break;
    case 32:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
        break;
    case 33:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
        break;
    case 34:
        if (value > GET_LEVEL(ch) || value > LVL_OVERLORD) {
            buf = "You can't do that.\n";
            save = false;
            break;
        }
        value = std::clamp(value, 0, LVL_IMPL);
        vict->player.level = (byte)value;
        break;
    case 35:
        if ((i = real_room(value)) < 0) {
            buf = "No room exists with that number.\n";
            save = false;
            break;
        }
        if (!is_file) {
            if (IN_ROOM(vict) != NOWHERE)
                char_from_room(vict);
            char_to_room(vict, i);
        }
        break;
    case 36:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
        break;
    case 37:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
        break;
    case 38:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
        break;
    case 39: {
        auto new_class = argument.shift();
        if ((i = parse_class(ch, vict, new_class)) == CLASS_UNDEFINED) {
            save = false;
            buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
            break;
        }
        if (i == GET_CLASS(vict)) {
            buf = fmt::format("%s already %s.", vict == ch ? "You're" : "$e's",
                              with_indefinite_article(CLASS_FULL(vict)));
        } else {
            convert_class(vict, i);
            if (ch != vict) {
                buf = fmt::format("$n has changed your class to {}!", CLASS_FULL(vict));
                act(buf, false, ch, 0, vict, TO_VICT);
            }
            buf = fmt::format("Okay, {} now {}.", vict == ch ? "you are" : "$n is",
                              with_indefinite_article(CLASS_FULL(vict)));
        }
        act(buf, false, vict, 0, ch, TO_VICT);
        buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
    } break;
    case 40:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
        break;
    case 41:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
        break;
    case 42: {
        auto room_num_opt = argument.try_shift_number();
        if (room_num_opt) {
            value = *room_num_opt;
            if (real_room(value) != NOWHERE) {
                GET_HOMEROOM(vict) = value;
                buf = fmt::format("{}'s home room is now #{}.\n", GET_NAME(vict), GET_HOMEROOM(vict));
            } else {
                buf = "That room does not exist!\n";
                save = false;
            }
        } else {
            buf = "Must be a room's vnum.\n";
            save = false;
        }
    } break;
    case 43:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_COLOR_1);
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_COLOR_2);
        break;
    case 44:
        if (GET_IDNUM(ch) != 1 || IS_NPC(vict))
            return;
        GET_IDNUM(vict) = value;
        break;
    case 45:
        if (!is_file) {
            char_printf(ch, "Not while they're logged in!\n");
            return;
        }
        if (GET_LEVEL(vict) > LVL_HEAD_C) {
            buf = "You cannot change that.\n";
            save = false;
        } else {
            auto pass = argument.get();
            auto crypt_password = CRYPT(pass.data(), GET_NAME(vict).data());
            GET_PASSWD(vict) = crypt_password.substr(0, MAX_PWD_LENGTH);
            buf = fmt::format("Password changed to '{}'.\n", pass);
        }
        break;
    case 46:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
        break;
    case 47:
        GET_NATURAL_CHA(vict) = std::clamp(value, MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 48: {
        auto new_race = argument.shift();
        if ((i = parse_race(ch, vict, new_race)) == RACE_UNDEFINED) {
            save = false;
            buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
            break;
        }
        if (i == GET_RACE(vict)) {
            buf =
                fmt::format("{} already {}.", vict == ch ? "You're" : "$e's", with_indefinite_article(RACE_FULL(vict)));
        } else {
            convert_race(vict, i);
            if (ch != vict) {
                buf = fmt::format("$n has changed your race to {}!", RACE_FULL(vict));
                act(buf, false, ch, 0, vict, TO_VICT);
            }
            buf = fmt::format("Okay, %s now %s.", vict == ch ? "you are" : "$n is",
                              with_indefinite_article(RACE_FULL(vict)));
        }
        act(buf, false, vict, 0, ch, TO_VICT);
        buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
    } break;
    case 49:
        /* GET_OLC2_ZONE(vict) = value; */
        break;
    case 50:
        /* GET_OLC3_ZONE(vict) = value; */
        break;
    case 51:
        /* GET_OLC4_ZONE(vict) = value; */
        break;
    case 52:
        /* GET_OLC5_ZONE(vict) = value; */
        break;
    case 53:
        GET_PLATINUM(vict) = std::clamp(value, 0, 100000000);
        break;
    case 54:
        GET_GOLD(vict) = std::clamp(value, 0, 100000000);
        break;
    case 55:
        GET_SILVER(vict) = std::clamp(value, 0, 100000000);
        break;
    case 56:
        GET_COPPER(vict) = std::clamp(value, 0, 100000000);
        break;
    case 57:
        GET_BANK_PLATINUM(vict) = std::clamp(value, 0, 100000000);
        break;
    case 58:
        GET_BANK_GOLD(vict) = std::clamp(value, 0, 100000000);
        break;
    case 59:
        GET_BANK_SILVER(vict) = std::clamp(value, 0, 100000000);
        break;
    case 60:
        GET_BANK_COPPER(vict) = std::clamp(value, 0, 100000000);
        break;

    case 61:
        break;
    case 62:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ANON);
        break;
    case 63:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NEWNAME);
        break;
    case 64:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NAPPROVE);
        break;
    case 65:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_HOLYLIGHT);
        break;
    case 66:
        do_wiztitle(vict, argument);
        break;
    case 67:
        GET_COOLDOWN(vict, CD_OFFENSE_CHANT) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_DEFENSE_CHANT) = std::clamp(value, 0, 100);
        break;
    case 68: {
        auto new_size = argument.shift();
        if (new_size.empty()) {
            show_sizes(ch);
            save = false;
            buf.clear();
            break;
        } else if ((value = parse_size(ch, new_size)) == SIZE_UNDEFINED) {
            save = false;
            buf.clear();
            break;
        }
        change_natural_size(vict, value);
        buf = fmt::format("{} {} now {} in size.\n", ch == vict ? "You" : GET_NAME(vict), ch == vict ? "are" : "is",
                          SIZE_DESC(vict));
    } break;
    case 69:
        GET_HIDDENNESS(vict) = std::clamp(value, 0, 1000);
        break;
    case 70:
        GET_RAGE(vict) = std::clamp(value, 0, 1000);
        check_regen_rates(vict);
        break;
    case 71: {
        auto title_num = argument.try_shift_number();
        auto new_title = argument.get().substr(0, MAX_TITLE_WIDTH);
        int max_titles = GET_PERM_TITLES(vict).size();
        if (!title_num) {
            add_perm_title(vict, new_title);
        } else if (*title_num < 1 || *title_num > max_titles) {
            buf = fmt::format("You can only replace ptitles from 1 to {}.\n", max_titles);
            save = false;
        } else {
            if (GET_PERM_TITLES(vict).size() == max_titles)
                add_perm_title(vict, new_title);
            else {
                GET_PERM_TITLES(vict)[*title_num] = new_title;
            }
            buf = fmt::format("{}'s p-title {} is now: {}\n", GET_NAME(vict), i, new_title);
        }
    } break;
    case 72:
        GET_HEIGHT(vict) = value;
        break;
    case 73:
        GET_WEIGHT(vict) = value;
        break;
    case 74: /* LIFE FORCE */
        if ((i = parse_lifeforce(ch, argument.get())) == LIFE_UNDEFINED) {
            save = false;
            break;
        }
        if (i == GET_LIFEFORCE(vict)) {
            buf = fmt::format("%s already infused with %s.", vict == ch ? "You're" : "$e's", LIFEFORCE_NAME(vict));
        } else {
            if (ch != vict) {
                buf = fmt::format("$n has changed your life force to %s!", lifeforces[i].name);
                act(buf, false, ch, 0, vict, TO_VICT);
            }
            buf = fmt::format("Okay, %s's life force is now %s.", vict == ch ? "you are" : "$n is", lifeforces[i].name);
            convert_lifeforce(vict, i);
        }
        act(buf, false, vict, 0, ch, TO_VICT);
        break;
    case 75: /* COMPOSITION */
        if ((i = parse_composition(ch, argument.get())) == COMP_UNDEFINED) {
            save = false;
            break;
        }
        if (i == BASE_COMPOSITION(vict)) {
            buf = fmt::format("%s already composed of %s.", vict == ch ? "You're" : "$e's", compositions[i].name);
        } else {
            if (ch != vict) {
                buf = fmt::format("$n has changed your composition to %s!", compositions[i].name);
                act(buf, false, ch, 0, vict, TO_VICT);
            }
            buf = fmt::format("Okay, %s now composed of %s.", vict == ch ? "you are" : "$n is", compositions[i].name);
            set_base_composition(vict, i);
        }
        act(buf, false, vict, 0, ch, TO_VICT);
        break;
    case 76: /* reset all bard music cooldowns */
        GET_COOLDOWN(vict, CD_MUSIC_1) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_2) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_3) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_4) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_5) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_6) = std::clamp(value, 0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_7) = std::clamp(value, 0, 100);
        break;
    case 77: /* summon mount for anti and paladin */
        GET_COOLDOWN(vict, CD_SUMMON_MOUNT) = std::clamp(value, 0, 100);
        break;
    default:
        buf = "Can't set that!\n";
        break;
    }

    if (fields[l].type == BINARY) {
        buf = fmt::format("{} {} for {}.\n", fields[l].cmd, ONOFF(on), GET_NAME(vict));
    } else if (fields[l].type == NUMBER) {
        buf = fmt::format("{}'s {} set to {}.\n", GET_NAME(vict), fields[l].cmd, value);
    }
    if (buf[0])
        char_printf(ch, capitalize_first(buf));

    if (!is_file && !IS_NPC(vict) && save)
        save_player_char(vict);

    if (is_file) {
        if (save) {
            GET_PFILEPOS(cbuf) = player_i;
            save_player_char(cbuf);
            char_printf(ch, "Saved in file.\n");
        }
        free_char(cbuf);
    }
}

ACMD(do_syslog) {
    int severity;

    auto arg = argument.shift();

    if (arg.empty()) {
        char_printf(ch, "The minimum severity of syslog messages you see is {}.\n",
                    sprint_log_severity(GET_LOG_VIEW(ch)));
        return;
    }

    if ((severity = parse_log_severity(arg)) < 0) {
        char_printf(ch, "Usage: syslog {::}\n", fmt::join(log_severities, " | "));
        return;
    }

    GET_LOG_VIEW(ch) = severity;

    char_printf(ch, "The minimum severity of syslog messages you will see is now {}.\n", sprint_log_severity(severity));
}

void send_to_imms(std::string_view msg) {
    DescriptorData *pt;

    for (pt = descriptor_list; pt; pt = pt->next)
        if (!pt->connected && pt->character && GET_LEVEL(pt->character) >= LVL_GOD)
            char_printf(pt->character, msg);
}

/* do_game recoded by 321 to allow different toggles to have different minimum levels */
ACMD(do_game) {
    struct Command {
        const std::string_view name;
        const bool has_value; /* Is it on/off (false) or does it have a value (true) */
        bool *field;
        const int min_level;
        std::string_view turn_on;
        std::string_view turn_off;
        std::string_view enabled;
        std::string_view disabled;
    };

    constexpr std::array<Command, 14> commands = {
        Command{"RACES", false, &races_allowed, LVL_ADMIN, "[&2&bSYS: {} allows race logins&0]\n",
                "[&1&bSYS: {} disallows race logins&0]\n", "Race login allowed", "Race login not allowed"},
        Command{"EVILRACES", false, &evil_races_allowed, LVL_ADMIN,
                "[&2&bSYS: {} allows evil race player creation&0]\n",
                "[&1&bSYS: {} disallows evil race player creation&0]\n", "Evil race player creation allowed",
                "Evil race player creation not allowed"},
        Command{"PK", false, &pk_allowed, LVL_ADMIN, "[&2&bSYS: {} allows PKilling&0]\n",
                "[&1&bSYS: {} disallows PKilling&0]\n", "PKilling allowed", "PKilling not allowed"},
        Command{"SLEEP", false, &sleep_allowed, LVL_ADMIN,
                "[&2&bSYS: {} allows players to cast sleep on each other&0]\n",
                "[&1&bSYS: {} disallows players from casting sleep on each other&0]\n",
                "Casting sleep on other players allowed", "Casting sleep on other players not allowed"},
        Command{"SUMMON", false, &summon_allowed, LVL_ADMIN, "[&2&bSYS: {} allows players to summon one another&0]\n",
                "[&1&bSYS: {} disallows players from summoning one another&0]\n", "Summoning other players allowed",
                "Summoning other players not allowed"},
        Command{"CHARM", false, &charm_allowed, LVL_ADMIN,
                "[&2&bSYS: {} allows players to cast charm on each other&0]\n",
                "[&1&bSYS: {} disallows players from casting charm on each other&0]\n",
                "Charming other players allowed", "Charming other players not allowed"},
        Command{"ROOMEFFECT", false, &roomeffect_allowed, LVL_ADMIN,
                "[&2&bSYS: {} allows room effect spells to hurt other players&0]\n",
                "[&1&bSYS: {} disallows room effect spells from hurting other "
                "players&0]\n",
                "Room effect spells will hurt other players", "Room effect spells will not hurt other players"},
        Command{"NAMES", false, &approve_names, LVL_ADMIN, "[&2&bSYS: {} turned on name approval&0]\n",
                "[&1&bSYS: {} turned off name approval&0]\n", "Name approval is required",
                "Name approval is NOT required"},
        Command{"NPAUSE", false, &napprove_pause, LVL_ADMIN, "[&2&bSYS: {} turned on name approval pause&0]\n",
                "[&1&bSYS: {} turned off name approval pause&0]\n", "Name approval pause is ON",
                "Name approval pause is OFF"},
        Command{"OOC", false, &gossip_channel_active, LVL_ATTENDANT, "[&2&bSYS: {} enables OOC! ANARCHY!&0]\n",
                "[&1&bSYS: {} diables OOC! GAWD SAVE THE QUEEN!&0]\n", "OOC is enabled", "OOC is disabled"},
        Command{"SLOWNS", false, &nameserver_is_slow, LVL_HEAD_C, "[&2&bSYS: {} turns off hostname lookup.&0]\n",
                "[&1&bSYS: {} turns on hostname lookup.&0]\n", "Nameserver lookup is disabled: slowns is on",
                "Nameserver lookup is enabled: slowns os off"},
        Command{"LEVELGAIN", false, &level_gain, LVL_BUILDER, "[&2&bSYS: {} turned on \"level gain\" code.&0]\n",
                "[&1&bSYS: {} turned off \"level gain\" code.&0]\n", "\"level gain\" code is active: levelgain is on",
                "\"level gain\" code is inactive: levelgain is off"},
        Command{"DAMAGEAMTS", false, &damage_amounts, LVL_BUILDER,
                "[&2&bSYS: {} turned on display damage amounts code.&0]\n",
                "[&1&bSYS: {} turned off display damage amounts code.&0]\n",
                "Display damage code is active: damageamts is on",
                "Display damage code is inactive: damageamts is off"},
        Command{"GROUPING", true, &max_group_difference, LVL_ATTENDANT,
                "[&2&bSYS: {} set the max group level difference to {}.&0]\n",
                "[&1&bSYS: {} turned off the max group level difference.&0]\n",
                "Max group level difference is set to: ", "Max group level difference is off: "}};

    auto field = argument.shift();
    auto set = argument.get();

    // Find the first command that matches the field
    const auto *command = std::find_if(commands.begin(), commands.end(), [&](const auto &cmd) {
        return (GET_LEVEL(ch) >= cmd.min_level) && matches(field, cmd.name);
    });

    if (command == commands.end()) {
        int n_visible = 0;
        for (auto &cmd : commands)
            if (GET_LEVEL(ch) >= cmd.min_level) {
                if (!n_visible++)
                    char_printf(ch, "\n[Current game status:]\n\n");

                auto shortbuf = fmt::format("[{}{}&0]", cmd.field ? "&2&b" : "&1&b", cmd.name);

                auto linebuf = [&]() {
                    if (cmd.has_value) {
                        return fmt::format("{:19}{} {}\n", shortbuf, cmd.field ? cmd.enabled : cmd.disabled, cmd.field);
                    }
                    return fmt::format("{:19s}{}\n", shortbuf, cmd.field ? cmd.enabled : cmd.disabled);
                }();
                char_printf(ch, linebuf);
            }
        if (!n_visible)
            char_printf(ch, "You do not have access to any game config toggles.\n");
        return;
    }

    if (set.empty()) {
        char_printf(ch, "Usage: game {} <on|off|value>\n", command->name);
        return;
    }

    if (command->has_value) {
        int value = svtoi(set);
        *command->field = value;
        auto msg = command->field ? command->turn_on : command->turn_off;
        send_to_imms(fmt::vformat(msg, fmt::make_format_args(GET_NAME(ch), value)));
    } else {
        if (set.empty()) {
            *command->field = !command->field;
            const auto msg = command->field ? command->turn_on : command->turn_off;
            send_to_imms(fmt::vformat(command->turn_off, fmt::make_format_args(GET_NAME(ch), command->field)));
            return;
        }
        if (matches(set, "on")) {
            if (!command->field) {
                *command->field = true;
                send_to_imms(fmt::vformat(command->turn_on, fmt::make_format_args(GET_NAME(ch))));
            } else {
                char_printf(ch, "It's already on.\n");
            }
        } else if (matches(set, "off")) {
            if (command->field) {
                *command->field = false;
                send_to_imms(fmt::vformat(command->turn_off, fmt::make_format_args(GET_NAME(ch))));
            } else {
                char_printf(ch, "It's already off.\n");
            }
        } else {
            char_printf(ch, "Usage: game {} <on|off>\n", command->name);
        }
    }
}

ACMD(do_autoboot) {
    extern void check_auto_rebooting();
    extern void cancel_auto_reboot(int postponed);
    extern void reboot_info(CharData * ch);
    std::string_view s;
    int hours, mins, minutes;

    auto field = argument.shift();

    if (field.empty()) {
        if (GET_LEVEL(ch) >= LVL_REBOOT_POSTPONE) {
            char_printf(ch, "Usage:\n");
            if (GET_LEVEL(ch) >= LVL_REBOOT_MASTER) {
                char_printf(ch, "  autoboot off           - disable automatic reboot\n");
                char_printf(ch, "  autoboot on            - enable automatic reboot\n");
                char_printf(ch, "  autoboot warntime <mn> - warnings begin <mn> minutes before reboot (now {:d})\n",
                            reboot_warning_minutes);
                char_printf(ch, "  autoboot [<hr>][:<mn>] - reboot in <hr> hours, <mn> minutes\n");
            }
            char_printf(ch, "  autoboot postpone      - postpone reboot to {:d} minutes from now\n",
                        2 * reboot_warning_minutes);
            char_printf(ch, "\n");
        }

        reboot_info(ch);
        return;
    }

    if (matches(field, "postpone")) {
        if (GET_LEVEL(ch) < LVL_REBOOT_POSTPONE) {
            char_printf(ch, "You can't do that.\n");
            return;
        }

        if (!reboot_auto) {
            char_printf(ch, "Automatic rebooting isn't even enabled!\n");
            return;
        }

        minutes = 2 * reboot_warning_minutes;

        if (reboot_pulse - global_pulse > (unsigned long)minutes * 60 * PASSES_PER_SEC) {
            char_printf(ch, "Not postponing reboot because it's over {:d} minutes away.\n", 2 * reboot_warning_minutes);
            return;
        }

        reboot_pulse = global_pulse + minutes * 60 * PASSES_PER_SEC;

        send_to_imms(fmt::format("[ {} postponed autoboot for {} minutes ]\n", GET_NAME(ch), minutes));
        log("(GC) {} postponed autoboot for {:d} minutes.", GET_NAME(ch), minutes);

        cancel_auto_reboot(1);
        return;
    }

    if (matches(field, "on")) {
        if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
            char_printf(ch, "You can't do that.\n");
            return;
        }

        if (reboot_auto) {
            char_printf(ch, "Automatic rebooting is already on.\n");
            reboot_info(ch);
            return;
        }
        reboot_auto = true;
        send_to_imms(fmt::format("[ {} set autoboot to &2&bON&0 ]\n", GET_NAME(ch)));
        log("(GC) {} turned on autoboot.", GET_NAME(ch));

        /* Make sure the reboot is a minimum amount of time away */
        if (reboot_pulse - global_pulse < (unsigned long)60 * PASSES_PER_SEC * reboot_warning_minutes) {
            reboot_pulse = global_pulse + 60 * PASSES_PER_SEC * reboot_warning_minutes;
        }
        check_auto_rebooting();
        return;
    }

    if (matches(field, "off")) {
        if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
            char_printf(ch, "You can't do that.\n");
            return;
        }

        if (!reboot_auto) {
            char_printf(ch, "Automatic rebooting is already off.\n");
            return;
        }

        reboot_auto = false;
        send_to_imms(fmt::format("[ {} set autoboot to &1&bOFF&0 ]\n", GET_NAME(ch)));
        log("(GC) {} turned off autoboot.", GET_NAME(ch));

        cancel_auto_reboot(0);
        return;
    }

    if (matches(field, "warntime")) {
        if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
            char_printf(ch, "You can't do that.\n");
            return;
        }

        auto field2 = argument.try_shift_number();
        if (!field2) {
            char_printf(ch, "Set the reboot warning to how many minutes?\n");
            return;
        }
        mins = *field2;
        if (mins < 1) {
            char_printf(ch, "Invalid setting - must be 1 or greater.\n");
            return;
        }
        reboot_warning_minutes = mins;

        send_to_imms(
            fmt::format("[ {} set reboot warning time to {} minutes ]\n", GET_NAME(ch), reboot_warning_minutes));
        log("(GC) {} set reboot warning time to {} minutes.", GET_NAME(ch), reboot_warning_minutes);

        /* The rest of this block is about managing warnings, which is moot
         * unless reboot_auto is true, so: */
        if (!reboot_auto)
            return;

        /* See how many minutes until the reboot. */
        minutes = (reboot_pulse - global_pulse) / (60 * PASSES_PER_SEC);

        if (reboot_warning_minutes >= minutes && !reboot_warning) {
            check_auto_rebooting();
        } else if (reboot_warning) {
            /* You've set reboot_warning_minutes lower - but a moment ago, we were
             * sending warnings about an impending reboot. Set this to 0 to reflect
             * the fact that we are not sending warnings any more. */
            reboot_warning = 0;
        }
        return;
    }

    /* Any other parameter is taken to be an amount of time until an automatic
     * reboot. It may be:
     *    ##       - number of hours
     *    ##:##    - hours and minutes
     *    :##      - minutes only */

    hours = 0;
    minutes = 0;
    mins = 0;

    if (field.starts_with(':') && field.length() > 1) {
        // minutes only
        mins = svtoi(field.substr(1));
    } else {
        hours = svtoi(field);
        if (auto s = field.find(':'); s != std::string_view::npos && s + 1 < field.length()) {
            mins = svtoi(field.substr(s + 1));
        }
    }

    minutes = mins + hours * 60;

    if (minutes > 0) {
        if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
            char_printf(ch, "You can't do that.\n");
            return;
        }

        reboot_auto = true;
        send_to_imms(fmt::format("[ {} set the mud to reboot in &7&b{:02d}:{:02d}&0 ]\n", GET_NAME(ch), hours, mins));
        log("(GC) {} set the mud to reboot in {:02d}:{:02d}.", GET_NAME(ch), hours, mins);

        reboot_pulse = global_pulse + 60 * PASSES_PER_SEC * minutes;
        if (minutes <= reboot_warning_minutes) {
            check_auto_rebooting();
        } else
            cancel_auto_reboot(0);
    } else {
        char_printf(ch, "Sorry, I don't understand that.\n");
    }
}

ACMD(do_copyto) {
    /* Only works if you have Oasis OLC */

    int iroom = 0, rroom = 0;
    RoomData *room;

    auto room_num = argument.try_shift_number();
    if (!room_num) {
        char_printf(ch, "Format: copyto <room number>\n");
        return;
    }

    CREATE(room, RoomData, 1);

    rroom = real_room(*room_num);
    *room = world[rroom];

    if (rroom <= 0) {
        char_printf(ch, "There is no room with the number {:d}.\n", iroom);
        return;
    }

    /* Main stuff */

    if (!world[ch->in_room].description.empty()) {
        world[rroom].description = world[ch->in_room].description;

        /* Only works if you have Oasis OLC */
        olc_add_to_save_list((iroom / 100), OLC_SAVE_ROOM);

        char_printf(ch, "You copy the description to room {:d}.\n", iroom);
    } else
        char_printf(ch, "This room has no description!\n");
}

ACMD(do_dig) {
    int iroom, rroom, dir;

    auto dir_str = argument.shift();
    auto room_num = argument.try_shift_number();

    if (dir_str.empty() || room_num) {
        char_printf(ch, "Format: dig <dir> <room number>\n");
        return;
    }

    if ((dir = parse_direction(dir_str)) < 0) {
        char_printf(ch, "That isn't a valid direction.\n");
        return;
    }

    rroom = real_room(*room_num);
    if (rroom <= 0) {
        char_printf(ch, "There is no room with the number {:d}.\n", iroom);
        return;
    }

    world[rroom].exits[rev_dir[dir]] = create_exit(ch->in_room);
    world[ch->in_room].exits[dir] = create_exit(rroom);

    olc_add_to_save_list(zone_table[world[ch->in_room].zone].number, OLC_SAVE_ROOM);
    olc_add_to_save_list(zone_table[world[rroom].zone].number, OLC_SAVE_ROOM);

    char_printf(ch, "You make an exit {} to room {:d}.\n", dirs[dir], iroom);
}

ACMD(do_rrestore) {
    CharData *i;

    auto message = argument.get();
    for (i = character_list; i; i = i->next) {
        if (i != ch && (!IS_NPC(i) || i->desc) && !ROOM_FLAGGED(i->in_room, ROOM_ARENA)) {
            if (!message.empty())
                char_printf(i, "{}@0\n", message);
            else
                act("&0&b&4$n &0&b&9spreads $s &0&benergy&0&b&9 across the realms &0&6restoring&0&b&9 all in $s "
                    "path!&0",
                    false, ch, 0, i, TO_VICT | TO_SLEEP);
            perform_restore(i);
        }
    }
    if (message.empty())
        char_printf(ch, "You spread healing energy across the realm, restoring all in its path.\n");
    else
        char_printf(ch, "{}@0\n", message);
}

ACMD(do_rpain) {
    CharData *i;

    auto message = argument.get();
    for (i = character_list; i; i = i->next) {
        if (i != ch && (!IS_NPC(i) || i->desc)) {
            perform_pain(i);
            if (!message.empty())
                char_printf(i, "{}@0\n", argument);
            else
                act("&0&1$n &0&9&bspreads pain and pestilence across the realm "
                    "&0&1&bharming&0&9&b all in $s path!&0",
                    false, ch, 0, i, TO_VICT | TO_SLEEP);
        }
    }
    if (message.empty())
        char_printf(ch, "Pain and pestilence spreads across the lands.  Your wrath has been known.\n");
    else
        char_printf(ch, "{}@0\n", argument);
}

ACMD(do_rclone) {
    int vnum, rnum, i;
    RoomData *src, *dest;

    auto room_num = argument.try_shift_number();
    if (!room_num) {
        char_printf(ch, "Format: hhroom <target room number>\n");
        return;
    }

    rnum = real_room(*room_num);

    if (rnum <= NOWHERE) {
        char_printf(ch, "There is no room with the number {}.\n", *room_num);
        return;
    }

    if (ch->in_room <= NOWHERE || ch->in_room >= top_of_world) {
        log("SYSERR: {} attempting to use hhroom and not in valid room", GET_NAME(ch));
        return;
    }

    src = &world[ch->in_room];
    dest = &world[rnum];

    if (!src->description.empty())
        dest->description = src->description;
    if (!src->description.empty())
        dest->description = src->description;
    if (!src->name.empty())
        dest->name = src->name;
    for (i = 0; i < FLAGVECTOR_SIZE(NUM_ROOM_FLAGS); ++i)
        dest->room_flags[i] = src->room_flags[i];
    if (src->sector_type)
        dest->sector_type = src->sector_type;

    olc_add_to_save_list(zone_table[find_real_zone_by_room(vnum)].number, OLC_SAVE_ROOM);

    char_printf(ch, "You clone this room to room {}.\n", vnum);
}

ACMD(do_terminate) {
    CharData *victim;

    char_printf(ch, "&1&bThis command has been disabled until further notice!&0\n");
    return;

    auto buf = argument.shift();

    if (buf.empty()) {
        char_printf(ch, "Whom do you wish to terminate?\n");
        return;
    }
    victim = find_char_around_char(ch, find_vis_by_name(ch, buf));
    if (victim) {
        if (victim == ch) {
            char_printf(ch, "You cannot term yourself goon!\n");
            return;
        }

        if (GET_LEVEL(victim) == LVL_IMPL) {
            char_printf(ch, "&1You dare NOT do that!&0\n");
            return;
        }
        if (IS_NPC(victim)) {
            char_printf(ch, "You cannot term NPC's!\n");
            return;
        }
        /* delete and purge */
        if (GET_CLAN_MEMBERSHIP(victim))
            revoke_clan_membership(GET_CLAN_MEMBERSHIP(victim));
        SET_FLAG(PLR_FLAGS(victim), PLR_DELETED);
        save_player_char(victim);
        delete_player_obj_file(victim);
        if (victim->desc) {
            close_socket(victim->desc);
            victim->desc = nullptr;
        }
        act("&9&b$n cuts &0$N's &9&bthroat and buries $s corpse where no one will ever find it!&0", false, ch, 0,
            victim, TO_ROOM);
        act("&9&bYou destroy &0$N &9&bforever.&0", false, ch, 0, victim, TO_CHAR);
        log("{} has terminated {}!", GET_NAME(ch), GET_NAME(victim));
        extract_char(victim);
        return;
    }
    char_printf(ch, "That player is not playing. If you must, linkload first.\n");
}

/* This function cleans bogus entries from the player files */
ACMD(do_pfilemaint) {
    DescriptorData *d;
    int i, j, idle_time, allowed_time, reason;
    long bitfield;
    char tmp_name[255];
    PlayerIndexElement *new_player_table = nullptr;

    static const std::string_view rlist[] = {"",
                                             "Invalid Characters",
                                             "Too Short",
                                             "Too Long",
                                             "Reserved Fill Word",
                                             "Reserved Word",
                                             "Xname or MOB/OBJ name",
                                             "Inactivity"};

    if (should_restrict != GET_LEVEL(ch)) {
        char_printf(ch, "First <wizlock> and make sure everyone logs out before executing this command.\n");
        return;
    }

    for (d = descriptor_list; d; d = d->next) {
        if (d != ch->desc) {
            char_printf(ch, "You can't do this while anyone else is connected\n");
            return;
        }
    }

    log("PFILEMAINT: (GC) Started by {}", GET_NAME(ch));
    char_printf(ch, "Processing player files\n");

    /* copy the player index to a backup file */
    auto file_name = fmt::format("{}/{}", PLR_PREFIX, INDEX_FILE);
    auto buf = fmt::format("cp {} {}.`date +%%m%%d.%%H%%M%%S`", file_name, file_name);
    system(buf.c_str());

    CREATE(new_player_table, PlayerIndexElement, top_of_p_table + 1);

    /* loop through the player index */
    for (i = j = 0; i <= top_of_p_table; ++i) {
        /* days since last login */
        idle_time = (time(0) - player_table[i].last) / 86400;
        /* 4 weeks base plus 3 days per level */
        allowed_time = 28 + (player_table[i].level - 1) * 3;

        bitfield = player_table[i].flags;
        /* assume no delete at first          */
        reason = 0;

        auto tmp_name = _parse_name(player_table[i].name);
        /* no spaces special chars etc        */
        if (tmp_name.empty())
            reason = 1;
        /* must be greater than 2 chars       */
        if (tmp_name.length() < 2)
            reason = 2;
        /* must be less than MAX_NAME_LENGTH  */
        if (tmp_name.length() > MAX_NAME_LENGTH)
            reason = 3;
        /* can't be a reserved fill word      */
        if (fill_word(tmp_name))
            reason = 4;
        /* can't be a reserved word           */
        if (reserved_word(buf))
            reason = 5;
        /* can't be an xname or a mob name    */
        if (!is_valid_name(tmp_name))
            reason = 6;
        /* too much idle time/wasting space   */
        if (idle_time > allowed_time)
            reason = 7;
        else {
            /* don't del frozen players */
            if (IS_SET(bitfield, PINDEX_FROZEN))
                reason = 0;
            /* don't del players pending new name */
            if (IS_SET(bitfield, PINDEX_NEWNAME))
                reason = 0;
        }

        /* Never delete player 0, or yourself, or immortals */
        if (!player_table[i].id || player_table[i].id == GET_IDNUM(ch) || player_table[i].level >= LVL_IMMORT)
            reason = 0;

        if (reason) {
            log("PFILEMAINT: {} Player [{}] DELETED: {}.", player_table[i].id, player_table[i].name, rlist[reason]);
            if (reason == 7) {
                log("PFILEMAINT: Level {} Idle: {} days.", player_table[i].level, idle_time);
            }

            delete_player(i);
        } else {
            /* No delete: copy this player to the new index */
            new_player_table[j] = player_table[i];
            ++j;
        }
        char_printf(ch, ".");
    }

    log("PFILEMAINT: Original: {:d} Discarded: {:d} Saved: {:d}", i, i - j, j);

    log("PFILEMAINT: Destroying old player index table");
    free(player_table);

    player_table = new_player_table;
    top_of_p_table = j - 1;
    save_player_index();

    log("PFILEMAINT: Done.");
    char_printf(ch, "Done!\n");
}

ACMD(do_hotboot) {
    FILE *fp;
    bool found = false;
    DescriptorData *d, *d_next;
    int i;

    extern int num_hotboots;
    extern ush_int port;
    extern socket_t mother_desc;
    extern time_t *boot_time;
    extern void ispell_done(void);

    auto arg = argument.shift();

    /*
     * You must type 'hotboot yes' to actually hotboot.  However,
     * if anyone is connected and is not in the game or is editing
     * in OLC, a warning will be shown, and no hotboot will occur.
     * 'hotboot force' will override this.
     */
    if (!matches(arg, "force") || !matches(arg, "yes")) {
        char_printf(ch, "Are you sure you want to do a hotboot?  If so, type 'hotboot yes'.\n");
        return;
    }

    /*
     * First scan the descriptors to see if it would be particularly
     * inconvenient for anyone to have a hotboot right now.
     */
    for (d = descriptor_list; d; d = d->next) {
        if (d->character && STATE(d) == CON_PLAYING)
            continue; /* Okay, hopefully they're not too busy. */

        if (!found) {
            char_printf(ch, "Wait!  A hotboot might be inconvenient right now for:\n\n");
            found = true;
        }

        char_printf(ch, "  {}, who is currently busy with: {}{}{}\n",
                    d->character && !GET_NAME(d->character).empty() ? GET_NAME(d->character) : "An unnamed connection",
                    CLR(ch, FYEL), connected_types[STATE(d)], CLR(ch, ANRM));
    }

    if (found) {
        char_printf(ch, "\nIf you still want to do a hotboot, type 'hotboot force'.\n");
        return;
    }

    fp = fopen(HOTBOOT_FILE, "w");
    if (!fp) {
        char_printf(ch, "Hotboot file not writeable, aborted.\n");
        return;
    }

    log("(GC) Hotboot initiated by {}.", GET_NAME(ch));

    auto msg = fmt::format("\n {}<<< HOTBOOT by {} - please remain seated! >>>{}\n", CLR(ch, HRED), GET_NAME(ch),
                           CLR(ch, ANRM));

    /* Write boot_time as first line in file */
    fprintf(fp, "%d", num_hotboots + 1); /* num of boots so far */
    for (i = 0; i <= num_hotboots; ++i)
        fprintf(fp, " %ld", boot_time[i]); /* time of each boot */
    fprintf(fp, "\n");

    /* For each playing descriptor, save its state */
    for (d = descriptor_list; d; d = d_next) {
        /* We delete from the list, so need to save this. */
        d_next = d->next;

        /* Drop those logging on */
        if (!d->character || !IS_PLAYING(d)) {
            write_to_descriptor(d->descriptor, "\nSorry, we are rebooting.  Come back in a minute.\n");
            close_socket(d); /* throw 'em out */
        } else {
            CharData *tch = d->character;
            fprintf(fp, "%d %s %s\n", d->descriptor, GET_NAME(tch).c_str(), d->host);
            /* save tch */
            GET_QUIT_REASON(tch) = QUIT_HOTBOOT; /* Not exactly leaving, but sort of */
            save_player(tch);
            write_to_descriptor(d->descriptor, msg);
        }
    }

    fprintf(fp, "-1\n");
    fclose(fp);

    /* Kill child processes: ispell */
    ispell_done();

    /* Prepare arguments to call self */
    auto buf = fmt::format("{}", port);
    auto buf2 = fmt::format("-H{}", mother_desc);

    /* Ugh, seems it is expected we are 1 step above lib - this may be dangerous!
     */
    chdir("..");

    /* exec - descriptors are inherited! */
    execl("bin/fiery", "fiery", buf2, buf, (char *)nullptr);

    /* Failed - successful exec will not return */
    perror("do_hotboot: execl");
    write_to_descriptor(ch->desc->descriptor, "Hotboot FAILED!\n");

    /* Too much trouble to try and recover! */
    exit(1);
}

void scan_pfile_objs(CharData *ch, int vnum) {
    FILE *fl;
    int i, errs = 0, obsolete = 0, location;
    ObjData *object;
    int playerswith = 0, objectsfound = 0, objectshere;

    for (i = 0; i <= top_of_p_table; ++i) {

        fl = open_player_obj_file(player_table[i].name, ch, true);
        if (!fl) {
            continue;
        }

        /* Skip the rent code at the beginning */
        auto version = get_line(fl);

        /* Skip this file if it's an obsolete binary one */
        if (!version || !is_integer(*version)) {
            ++obsolete;
            fclose(fl);
            continue;
        }

        objectshere = 0;

        /* Read the objects */
        /* This could stand a major efficiency upgrade by skipping
         * build_object and doing the parsing ourselves, at the cost
         * of this function 'knowing' things about the pfile structure
         */
        while (!feof(fl)) {
            if (build_object(fl, &object, &location)) {
                if (GET_OBJ_VNUM(object) == vnum) {
                    if (!objectshere)
                        playerswith++;
                    objectshere++;
                    objectsfound++;
                }

                extract_obj(object);
            }
        }

        /* Done analyzing this player's objects */
        fclose(fl);

        if (objectshere) {
            page_string(ch, fmt::format("{:15s}  {:3d}\n", player_table[i].name, objectshere));
        }
    }

    /* Report statistics */
    if (!playerswith) {
        char_printf(ch, "None found.\n");
    } else {
        page_string(ch, fmt::format("Found {} player{} with {} {}.\n", playerswith, playerswith == 1 ? "" : "s",
                                    objectsfound, objectsfound == 1 ? "copy" : "copies"));
    }

    if (errs) {
        page_string(ch, fmt::format("\n({} error{} occurred during scan)\n", errs, errs == 1 ? "" : "s"));
    }
    if (obsolete) {
        page_string(ch, fmt::format("Skipped {} file{} that {} in the obsolete binary format.\n", obsolete,
                                    obsolete == 1 ? "" : "s", obsolete == 1 ? "is" : "are"));
    }
}

ACMD(do_pscan) {
    if (argument.empty()) {
        char_printf(ch, "Usage:\n");
        char_printf(ch, "   pscan obj <obj-vnum>\n");
        return;
    }

    auto arg1 = argument.shift();
    auto num_opt = argument.try_shift_number();

    if (matches(arg1, "obj")) {
        if (!num_opt) {
            char_printf(ch, "Usage: pscan obj <vnum>\n");
            return;
        }
        scan_pfile_objs(ch, *num_opt);
    } else {
        char_printf(ch, "That's not something you can pscan.\n");
    }
}

ACMD(do_objupdate) {
    auto arg1 = argument.shift();
    auto arg2 = argument.get();

    if (arg1.empty()) {
        char_printf(ch, "Usage: objupdate <player>\n");
        char_printf(ch, "Usage: objupdate all yes\n");
        return;
    }

    if (matches(arg1, "all")) {
        if (arg2.empty()) {
            char_printf(ch, "If you really want to update all obsolete object files, type &2objupdate all yes&0\n");
            return;
        }
        if (!matches("yes", arg2)) {
            char_printf(ch, "I'm not sure you really mean it!\n");
            return;
        }

        convert_player_obj_files(ch);
        return;
    }

    convert_single_player_obj_file(ch, arg1);
}

ACMD(do_coredump) {
    if (argument.empty()) {
        char_printf(ch, "Please supply a name for the core dump.\n");
    } else {
        drop_core(ch, argument.get());
    }
}
