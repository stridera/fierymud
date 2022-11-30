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
#include "structs.hpp"
#include "sysdep.hpp"
#include "textfiles.hpp"
#include "utils.hpp"
#include "weather.hpp"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void garble_text(char *string, int percent);
void check_new_surroundings(CharData *ch, bool old_room_was_dark, bool tx_obvious);

int Valid_Name(char *newname);
int reserved_word(char *argument);

/* extern functions */
void send_to_xnames(char *name);
int find_zone(int num);
void cure_laryngitis(CharData *ch);
void reboot_mud_prep();
void rebootwarning(int minutes);
void update_stats(CharData *ch);

/* Internal funct */
void do_wiztitle(char *outbuf, CharData *vict, char *argu);

ACMD(do_inctime) {
    skip_spaces(&argument);

    if (strcmp(argument, "yes") != 0) {
        send_to_char(
            "Are you sure you want to move time forward?\n"
            "If so, type 'inctime yes'.\n",
            ch);
        return;
    }

    send_to_char("Advancing game time by one hour.\n", ch);
    increment_game_time();
}

ACMD(do_iptables) {
    extern int normalize_ip_address(char *in, char *out); /* utils.c */
    char cBuf[MAX_INPUT_LENGTH];
    char cBuf2[MAX_INPUT_LENGTH];
    char cBuf3[MAX_INPUT_LENGTH];
    char tmpfile[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CharData *vict;
    DescriptorData *desc;

    /* List of ports we want to open for coders */
    int ports_to_open[2] = {22, 80};
    /* Make sure this number matches the size of ports_to_open */
    int num_ports = 2;

    int i, num_ports_open;
    bool ports_open[2];

    FILE *tmp;

    sprintf(tmpfile, "/tmp/iptables.list.%d", getpid());

    argument = one_argument(argument, arg);

    /* "list" is the default action */
    if (strcmp(arg, "list") == 0 || !arg[0]) {
        sprintf(cBuf3, "/sbin/iptables --list FieryMUD -v -n --line-numbers > %s", tmpfile);
        system(cBuf3);

        tmp = fopen(tmpfile, "rt");

        while (get_line(tmp, cBuf))
            pprintf(ch, "%s\n", cBuf);

        start_paging(ch);

        fclose(tmp);

        sprintf(cBuf3, "rm %s", tmpfile);
        system(cBuf3);
    } else if (strcmp(arg, "add") == 0) {
        one_argument(argument, arg);

        if (!*arg) {
            send_to_char("What player or IP address do you want allowed?\n", ch);
            return;
        } else {
            /* Must get normalized IP address string into buf now */

            if (normalize_ip_address(arg, buf)) {
                /* The string given is not a good IP address.  See if it's a player's
                 * name. */
                if (!strcmp("me", arg)) {
                    vict = ch;
                } else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
                    send_to_char("Nobody by that name (or invalid IP address).\n", ch);
                    return;
                }

                /* We have a player's name. */
                desc = nullptr;
                if (!IS_NPC(vict)) {
                    if (vict->desc) {
                        desc = vict->desc;
                    } else if (vict->forward && vict->forward->desc) {
                        desc = vict->forward->desc;
                    }
                }
                if (!desc) {
                    /* linkless */
                    act("$E's clueless at the moment.", false, ch, 0, vict, TO_CHAR | TO_SLEEP);
                    return;
                }
                if (!desc->host || !(*(desc->host))) {
                    act("I can't seem to determine $s IP address.", false, ch, 0, vict, TO_CHAR);
                    return;
                }
                /* We have a valid player with a link */
                if (normalize_ip_address(desc->host, buf)) {
                    sprintf(buf2, "Error! Could not normalize host '%s'!\n", desc->host);
                    send_to_char(buf2, ch);
                    return;
                }
                /* buf now contains the normlized IP address string for vict */
            }

            /* buf now contains normalized IP address string */

            if (!strcmp("127.0.0.1", buf)) {
                send_to_char("Cancelled - adding 127.0.0.1 is unnecessary.\n", ch);
                return;
            }

            /* Determine whether the address is already in the table. */

            sprintf(cBuf3, "/sbin/iptables --list FieryMUD -v -n --line-numbers > %s", tmpfile);
            system(cBuf3);

            tmp = fopen(tmpfile, "rt");

            /* Prepare the string to use when looking for the ip address */
            sprintf(buf2, " %s ", buf);
            num_ports_open = 0;
            for (i = 0; i < num_ports; i++)
                ports_open[i] = false;

            /* Find out which ports (if any) are already opened for the
             * requested address */
            while (get_line(tmp, cBuf)) {
                if (strstr(cBuf, buf2)) {
                    /* ip address found */
                    for (i = 0; i < num_ports; i++) {
                        if (!ports_open[i]) {
                            sprintf(buf1, " tcp dpt:%d ", ports_to_open[i]);
                            if (strstr(cBuf, buf1)) {
                                ports_open[i] = true;
                                num_ports_open++;
                            }
                        }
                    }
                }
            }

            sprintf(cBuf3, "rm %s", tmpfile);
            system(cBuf3);
            fclose(tmp);

            if (num_ports_open == num_ports) {
                sprintf(buf1, "%s is already in the table.\n", buf);
                send_to_char(buf1, ch);
            } else {
                /* Open up the desired ports */
                for (i = 0; i < num_ports; i++) {
                    if (!ports_open[i]) {
                        sprintf(cBuf,
                                "/sbin/iptables -I FieryMUD --source %s --protocol tcp "
                                "--dport %d --jump ACCEPT",
                                buf, ports_to_open[i]);
                        system(cBuf);
                    }
                }
                sprintf(buf2, "Ok, added %s to the table.\n", buf);
                send_to_char(buf2, ch);
            }
        }

    } else if (strcmp(arg, "del") == 0) {
        one_argument(argument, arg);
        if (!*arg) {
            send_to_char("Which line should be deleted?\n", ch);
        } else {
            int lineNum = 0;
            lineNum = atoi(arg);
            if (lineNum < 1) {
                send_to_char("That is not a valid line number.\n", ch);
            } else {
                sprintf(cBuf2, "/sbin/iptables -D FieryMUD %d", lineNum);
                system(cBuf2);

                send_to_char("Ok.\n", ch);
            }
        }
    } else {
        send_to_char("Usage:  iptables [add <ip/player>|del <number>|list]\n", ch);
    }
}

ACMD(do_echo) {
    skip_spaces(&argument);

    if (!*argument)
        send_to_char("Yes.. but what?\n", ch);
    else {
        if (subcmd == SCMD_EMOTE || subcmd == SCMD_EMOTES) {
            if (EFF_FLAGGED(ch, EFF_SILENCE)) {
                send_to_char("Your lips move, but no sound forms.\n", ch);
                return;
            }
            if (!speech_ok(ch, 0))
                return;
            if (GET_LEVEL(REAL_CHAR(ch)) < LVL_IMMORT)
                argument = strip_ansi(argument);
            sprintf(buf, "$n%s %s&0", subcmd == SCMD_EMOTES ? "'s" : "", argument);
        } else
            sprintf(buf, "%s@0", argument);

        act(buf, subcmd != SCMD_ECHO, ch, 0, 0, TO_ROOM);

        if (PRF_FLAGGED(ch, PRF_NOREPEAT))
            send_to_char(OK, ch);
        else
            act(buf, false, ch, 0, 0, TO_CHAR);
    }
}

ACMD(do_send) {
    CharData *vict;

    half_chop(argument, arg, buf);

    if (!*arg) {
        send_to_char("Send what to who?\n", ch);
        return;
    }
    if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
        send_to_char(NOPERSON, ch);
        return;
    }
    char_printf(vict, "%s@0\n", buf);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        send_to_char("Sent.\n", ch);
    else
        char_printf(ch, "You send '%s@0' to %s.\n", buf, GET_NAME(vict));
}

void perform_ptell(CharData *ch, CharData *vict, char *arg) {
    DescriptorData *d;

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        send_to_char(OK, ch);
    else {
        sprintf(buf, "&6You respond to $N, '&b%s&0&6'&0", arg);
        act(buf, false, ch, 0, vict, TO_CHAR | TO_SLEEP);
    }
    if (PRF_FLAGGED(vict, PRF_AFK)) {
        send_to_char("You received the previous message while AFK.\n", vict);
        send_to_char("That person is AFK right now but received your message.\n", ch);
    }

    sprintf(buf, "&6%s responds to %s's petition, '&b%s&0&6'&0\n", GET_NAME(REAL_CHAR(ch)), GET_NAME(vict), arg);
    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) == CON_PLAYING && d != ch->desc && GET_LEVEL(d->character) >= LVL_IMMORT &&
            !PLR_FLAGGED(d->character, PLR_WRITING) && !PLR_FLAGGED(d->character, PLR_MAILING) && !EDITING(d))
            send_to_char(buf, d->character);
    }

    if (vict->forward && !vict->desc)
        vict = vict->forward;

    sprintf(buf, "&6$n responds to your petition, '&b%s&0&6'&0", arg);
    act(buf, false, REAL_CHAR(ch), 0, vict, TO_VICT | TO_SLEEP);
}

ACMD(do_ptell) {
    CharData *vict;

    half_chop(argument, buf, buf2);

    if (!*buf || !*buf2)
        send_to_char("Who do you wish to ptell??\n", ch);
    else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, buf))))
        send_to_char(NOPERSON, ch);
    else if (ch == vict)
        send_to_char("You need mental help. Try ptelling someone besides yourself.\n", ch);
    else if (GET_LEVEL(vict) >= LVL_IMMORT)
        send_to_char("Just use wiznet!\n", ch);
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
int find_target_room(CharData *ch, char *rawroomstr) {
    int tmp;
    int location;
    CharData *target_mob;
    ObjData *target_obj;
    char roomstr[MAX_INPUT_LENGTH];

    one_argument(rawroomstr, roomstr);

    if (!*roomstr) {
        send_to_char("You must supply a room number or name.\n", ch);
        return NOWHERE;
    }
    if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
        tmp = atoi(roomstr);
        if ((location = real_room(tmp)) < 0) {
            send_to_char("No room exists with that number.\n", ch);
            return NOWHERE;
        }
    } else if ((target_mob = find_char_around_char(ch, find_vis_by_name(ch, roomstr))))
        location = target_mob->in_room;
    else if ((target_obj = find_obj_around_char(ch, find_vis_by_name(ch, roomstr)))) {
        if (target_obj->in_room != NOWHERE)
            location = target_obj->in_room;
        else {
            send_to_char("That object is not available.\n", ch);
            return NOWHERE;
        }
    } else {
        send_to_char("No such creature or object around.\n", ch);
        return NOWHERE;
    }

    /* a location has been found -- if you're < GRGOD, check restrictions. */
    if (GET_LEVEL(ch) < LVL_GOD) {
        if (ROOM_FLAGGED(location, ROOM_GODROOM)) {
            send_to_char("You are not godly enough to use that room!\n", ch);
            return NOWHERE;
        }
        if (ROOM_FLAGGED(location, ROOM_PRIVATE) && world[location].people && world[location].people->next_in_room) {
            send_to_char("There's a private conversation going on in that room.\n", ch);
            return NOWHERE;
        }
        if (ROOM_FLAGGED(location, ROOM_HOUSE) && !House_can_enter(ch, world[location].vnum)) {
            send_to_char("That's private property -- no trespassing!\n", ch);
            return NOWHERE;
        }
    }
    return location;
}

ACMD(do_at) {
    char command[MAX_INPUT_LENGTH];
    int location, original_loc;

    half_chop(argument, buf, command);
    if (!*buf) {
        send_to_char("You must supply a room number or a name.\n", ch);
        return;
    }

    if (!*command) {
        send_to_char("What do you want to do there?\n", ch);
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

    skip_spaces(&argument);
    if (strcmp(argument, "home") == 0) {
        if ((location = real_room(GET_HOMEROOM(ch))) < 0) {
            send_to_char("Your home room is invalid.\n", ch);
            return;
        }
    } else if ((location = find_target_room(ch, argument)) < 0) {
        return;
    }
    if (GET_POOFOUT(ch) && *GET_POOFOUT(ch))
        strcpy(buf, GET_POOFOUT(ch));
    else
        strcpy(buf, "$n disappears in a puff of smoke.");

    act(buf, true, ch, 0, 0, TO_ROOM);
    dismount_char(ch);
    char_from_room(ch);
    char_to_room(ch, location);

    if (GET_POOFIN(ch) && *GET_POOFIN(ch))
        strcpy(buf, GET_POOFIN(ch));
    else
        strcpy(buf, "$n appears with an ear-splitting bang.");

    act(buf, true, ch, 0, 0, TO_ROOM);
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

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char("Linkload who?\n", ch);
        return;
    }
    if (find_char_around_char(ch, find_plr_by_name(arg))) {
        send_to_char("They are already connected!\n", ch);
        return;
    }
    CREATE(victim, CharData, 1);
    clear_char(victim);
    if (load_player(arg, victim) > -1) {
        if (GET_LEVEL(victim) <= GET_LEVEL(ch)) {
            victim->player.time.logon = time(0);
            sprintf(buf, "(GC) %s has link-loaded %s.", GET_NAME(ch), GET_NAME(victim));
            mudlog(buf, BRF, GET_LEVEL(ch) + 1, true);
            char_to_room(victim, IN_ROOM(ch));
            load_quests(victim);
            load_objects(victim);
            victim->next = character_list;
            character_list = victim;
            victim->desc = nullptr;
            act("You linkload $N.", false, ch, 0, victim, TO_CHAR);
            act("$n linkloads $N.", false, ch, 0, victim, TO_NOTVICT);
        } else {
            send_to_char("Sorry, you aren't high enough to link-load that char.\n", ch);
            free_char(victim);
        }
    } else {
        send_to_char("No such player exists.\n", ch);
        free(victim);
    }
    return;
}

ACMD(do_trans) {
    DescriptorData *i;
    CharData *victim;
    bool wasdark;

    one_argument(argument, buf);
    if (!*buf)
        send_to_char("Whom do you wish to transfer?\n", ch);
    else if (strcmp("all", buf)) {
        if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, buf))))
            send_to_char(NOPERSON, ch);
        else if (victim == ch)
            send_to_char("That doesn't make much sense, does it?\n", ch);
        else {
            if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
                send_to_char("Go transfer someone your own size.\n", ch);
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
    } else { /* Trans All */
        if (GET_LEVEL(ch) < LVL_GRGOD) {
            send_to_char("I think not.\n", ch);
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
        send_to_char(OK, ch);
    }
}

ACMD(do_teleport) {
    CharData *victim;
    int target;
    bool wasdark;

    two_arguments(argument, buf, buf2);

    if (!*buf)
        send_to_char("Whom do you wish to teleport?\n", ch);
    else if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, buf))))
        send_to_char(NOPERSON, ch);
    else if (victim == ch)
        send_to_char("Use 'goto' to teleport yourself.\n", ch);
    else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
        send_to_char("Maybe you shouldn't do that.\n", ch);
    else if (!*buf2)
        send_to_char("Where do you wish to send this person?\n", ch);
    else if ((target = find_target_room(ch, buf2)) >= 0) {
        send_to_char(OK, ch);
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
        send_to_char("If you want to shut something down, say so!\n", ch);
        return;
    }
    one_argument(argument, arg);

    if (!*arg) {
        sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
        log("%s", buf);
        ;
        send_to_all("Shutting down.\n");
        circle_shutdown = 1;
    } else if (!strcmp(arg, "reboot")) {
        sprintf(buf, "(GC) Reboot by %s.", GET_NAME(ch));
        log("%s", buf);
        ;
        reboot_mud_prep();
        circle_shutdown = circle_reboot = 1;
    } else if (!strcmp(arg, "now")) {
        sprintf(buf, "(GC) Shutdown NOW by %s.", GET_NAME(ch));
        log("%s", buf);
        ;
        send_to_all(
            "Rebooting.. come back in a minute or two.\n"
            "           &1&b** ****** ****&0\n"
            "         &1&b**&0 &3&b***     *****&0  &1&b**&0\n"
            "       &1&b**&0 &3&b**      &1&b*&0     &3&b***&0  &1&b*&0\n"
            "       &1&b*&0    &3&b** **   *   *  *&0 &1&b**&0\n"
            "      &1&b*&0  &3&b** * *&0          &1&b*&0     &1&b*&0\n"
            "      &1&b*&0  &3&b*&0    &1&b**&0            &3&b* *&0 &1&b*&0\n"
            "     &1&b*&0 &3&b* &1&b** *&0     &3&b*   ******&0  &1&b*&0\n"
            "      &1&b*&0   &3&b* &1&b* **&0  &3&b***&0     &1&b*&0  &3&b*&0 "
            "&1&b*&0\n");
        send_to_all(
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

    } else if (!strcmp(arg, "die")) {
        sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
        log("%s", buf);
        ;
        send_to_all("Shutting down for maintenance.\n");
        touch("../.killscript");
        circle_shutdown = 1;
    } else if (!strcmp(arg, "pause")) {
        sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
        log("%s", buf);
        ;
        send_to_all("Shutting down for maintenance.\n");
        touch("../pause");
        circle_shutdown = 1;
    } else
        send_to_char("Unknown shutdown option.\n", ch);
}

void stop_snooping(CharData *ch) {
    if (!ch->desc->snooping)
        send_to_char("You aren't snooping anyone.\n", ch);
    else {
        send_to_char("You stop snooping.\n", ch);
        ch->desc->snooping->snoop_by = nullptr;
        ch->desc->snooping = nullptr;
    }
}

void perform_snoop(CharData *ch, DescriptorData *d) {
    CharData *tch;

    tch = nullptr;

    if (d->snooping == ch->desc) {
        send_to_char("Don't be stupid.\n", ch);
        return;
    } else if (d->snoop_by) {
        send_to_char("Busy already.\n", ch);
        return;
    } else if (d == ch->desc) {
        send_to_char("Not a good idea.\n", ch);
        return;
    } else if (STATE(d) != CON_PLAYING) {
        /* This is to prevent snoopers from seeing passwords. */
        send_to_char("Please wait until they've logged in.\n", ch);
        return;
    }

    if (d->original)
        tch = d->original;
    else if (d->character)
        tch = d->character;

    if (tch && GET_LEVEL(tch) >= GET_LEVEL(ch)) {
        send_to_char("You can't.\n", ch);
        return;
    }

    /* It's ok to snoop on. */
    send_to_char(OK, ch);

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

    one_argument(argument, arg);

    if (!*arg)
        stop_snooping(ch);
    else if (isdigit(*arg)) {
        dnum = atoi(arg);
        for (d = descriptor_list; d; d = d->next) {
            if (d->desc_num == dnum) {
                perform_snoop(ch, d);
                return;
            }
        }
        send_to_char("No such descriptor.\n", ch);
    } else if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, arg))))
        send_to_char("No such person around.\n", ch);
    else if (!victim->desc)
        send_to_char("There's no link.. nothing to snoop.\n", ch);
    else if (victim == ch)
        send_to_char("Not a good idea.\n", ch);
    else
        perform_snoop(ch, victim->desc);
}

ACMD(do_switch) {
    CharData *victim;

    one_argument(argument, arg);

    if (POSSESSED(ch))
        send_to_char("You're already switched.\n", ch);
    else if (!*arg)
        send_to_char("Switch with who?\n", ch);
    else if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, arg))))
        send_to_char("No such character.\n", ch);
    else if (ch == victim)
        send_to_char("Hee hee... we are jolly funny today, eh?\n", ch);
    else if (victim->desc)
        send_to_char("You can't do that, the body is already in use!\n", ch);
    else if ((GET_LEVEL(ch) < LVL_GRGOD) && !IS_NPC(victim))
        send_to_char("You aren't holy enough to use a mortal's body.\n", ch);
    else if ((GET_LEVEL(ch) < GET_LEVEL(victim)))
        send_to_char("You WISHED.\n", ch);
    else {
        send_to_char(OK, ch);

        ch->desc->character = victim;
        ch->desc->original = ch;

        victim->desc = ch->desc;
        ch->desc = nullptr;

        if (!GET_PROMPT(victim) || IS_NPC(victim))
            GET_PROMPT(victim) = strdup(GET_PROMPT(ch));
    }
}

ACMD(do_rename) {
    int player_i;
    CharData *victim = nullptr;
    char tmp_name[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, arg2);

    if (!ch || IS_NPC(ch))
        return;

    if (!*arg1 || !*arg2) {
        send_to_char("Usage: rename <player name> <new name>\n", ch);
        return;
    }

    victim = is_playing(arg1);

    if (!victim) {
        send_to_char("You can only rename a player currently logged in.\n", ch);
        return;
    }

    if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
        send_to_char("You don't have permission to change that name.\n", ch);
        return;
    }

    if (_parse_name(arg2, tmp_name) || strlen(tmp_name) < 2 || strlen(tmp_name) > MAX_NAME_LENGTH ||
        !Valid_Name(tmp_name) || fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {
        send_to_char("The new name is invalid.\n", ch);
        return;
    }

    if ((player_i = get_id_by_name(tmp_name)) >= 0) {
        send_to_char("There is already a player with that name.\n", ch);
        return;
    }

    sprintf(buf2, "&1&bYou have renamed &7%s&1 to &7%s&0\n", GET_NAME(victim), CAP(tmp_name));
    send_to_char(buf2, ch);
    sprintf(buf2, "(GC) %s has renamed %s to %s", GET_NAME(ch), GET_NAME(victim), tmp_name);
    mudlog(buf2, NRM, LVL_HEAD_C, true);

    rename_player(victim, tmp_name);

    sprintf(buf2, "&1&b!!! You have been renamed to &7%s&1.&0\n", GET_NAME(victim));
    send_to_char(buf2, victim);
}

ACMD(do_return) {
    if (POSSESSED(ch)) {
        if (GET_LEVEL(POSSESSOR(ch)) < 101 && subcmd == 0) {
            send_to_char("Huh?!?\n", ch);
            return;
        }
        if (GET_COOLDOWN(ch, CD_SHAPECHANGE) < 0 && subcmd == 0) {
            send_to_char("Use 'shapechange me' to return to your body.\n", ch);
            return;
        }

        send_to_char("You return to your original body.\n", ch);

        /* if someone switched into your original body, disconnect them */
        if (ch->desc->original->desc)
            close_socket(ch->desc->original->desc);

        ch->desc->character = ch->desc->original;
        ch->desc->original = nullptr;

        ch->desc->character->desc = ch->desc;
        ch->desc = nullptr;
    } else
        send_to_char("Huh?!?\n", ch);
}

ACMD(do_load) {
    CharData *mob;
    ObjData *obj;
    int number, r_num;

    two_arguments(argument, buf, buf2);

    if (!*buf || !*buf2 || !isdigit(*buf2)) {
        send_to_char("Usage: load { obj | mob } <number>\n", ch);
        return;
    }
    if ((number = atoi(buf2)) < 0) {
        send_to_char("A NEGATIVE number??\n", ch);
        return;
    }
    if (is_abbrev(buf, "mob")) {
        if ((r_num = real_mobile(number)) < 0) {
            send_to_char("There is no monster with that number.\n", ch);
            return;
        }
        mob = read_mobile(r_num, REAL);
        char_to_room(mob, ch->in_room);

        act("$n makes a quaint, magical gesture with one hand.", true, ch, 0, 0, TO_ROOM);
        act("$n has created $N!", false, ch, 0, mob, TO_ROOM);
        act("You create $N.", false, ch, 0, mob, TO_CHAR);
        sprintf(buf, "(GC) %s loads mob,  %s", GET_NAME(ch), GET_NAME(mob));
        mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), true);
        load_mtrigger(mob);
    } else if (is_abbrev(buf, "obj")) {
        if ((r_num = real_object(number)) < 0) {
            send_to_char("There is no object with that number.\n", ch);
            return;
        }
        obj = read_object(r_num, REAL);
        /*obj_to_room(obj, ch->in_room); */
        obj_to_char(obj, ch);
        act("$n makes a strange magical gesture.", true, ch, 0, 0, TO_ROOM);
        act("$n has created $p!", true, ch, obj, 0, TO_ROOM);
        act("You create $p.", false, ch, obj, 0, TO_CHAR);
        sprintf(buf, "(GC) %s loads OBJ,  %s", GET_NAME(ch), (obj)->short_description);
        mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), true);
    } else
        send_to_char("That'll have to be either 'obj' or 'mob'.\n", ch);
}

/* clean a room of all mobiles and objects */
ACMD(do_purge) {
    CharData *vict, *next_v;
    ObjData *obj, *next_o;

    one_argument(argument, buf);

    if (*buf) { /* argument supplied. destroy single object
                 * or char */
        if ((vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf)))) {
            if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict)) && (GET_LEVEL(ch) >= LVL_HEAD_B)) {
                send_to_char("Fuuuuuuuuu!\n", ch);
                return;
            }
            act("$n disintegrates $N.", false, ch, 0, vict, TO_NOTVICT);
            act("$n disintegrates YOU!", true, ch, 0, vict, TO_VICT);
            if (!IS_NPC(vict)) {
                sprintf(buf, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
                mudlog(buf, BRF, LVL_GOD, true);
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
            send_to_char("Nothing here by that name.\n", ch);
            return;
        }
        send_to_char(OK, ch);
    } else { /* no argument. clean out the room */
        act("$n gestures... You are surrounded by scorching flames!", false, ch, 0, 0, TO_ROOM);
        send_to_room("The world seems a little cleaner.\n", ch->in_room);
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
    char *name = arg, *level = buf2;
    int newlevel, oldlevel, i;

    two_arguments(argument, name, level);

    if (!*name) {
        char_printf(ch, "Advance who?\n");
        return;
    }
    if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, name)))) {
        send_to_char("That player is not here.\n", ch);
        return;
    }

    /*
     * I don't know which wise guy commented out this next check, but if
     * they ever do it again, I'll kill them.
     */
    if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
        send_to_char("Maybe that's not such a great idea.\n", ch);
        return;
    }
    if (IS_NPC(victim)) {
        send_to_char("NO!  Not on NPC's.\n", ch);
        return;
    }
    if (!*level || (newlevel = atoi(level)) <= 0) {
        send_to_char("That's not a level!\n", ch);
        return;
    }
    if (newlevel > LVL_IMPL) {
        sprintf(buf, "%d is the highest possible level.\n", LVL_IMPL);
        send_to_char(buf, ch);
        return;
    }

    if (newlevel > GET_LEVEL(ch)) {
        send_to_char("Yeah, right.\n", ch);
        return;
    }
    if (newlevel == GET_LEVEL(victim)) {
        send_to_char("They are already at that level.\n", ch);
        return;
    }
    oldlevel = GET_LEVEL(victim);
    if (newlevel < GET_LEVEL(victim)) {
        send_to_char(
            "&0&9&bYou are momentarily enveloped by darkness!&0\n"
            "&0&9&bYou feel somewhat diminished.&0\n",
            victim);
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
        for (i = 1; i <= MAX_SKILLS; i++)
            SET_SKILL(victim, i, 100);
        /* Make sure they have an empty olc zone list */
        free_olc_zone_list(victim);
    }

    char_printf(ch, "%s", OK);

    if (newlevel < oldlevel)
        mprintf(L_STAT, MAX(LVL_IMMORT, GET_LEVEL(ch)), "(GC) %s has demoted %s from level %d to %d.", GET_NAME(ch),
                GET_NAME(victim), oldlevel, newlevel);
    else
        mprintf(L_STAT, MAX(LVL_IMMORT, GET_LEVEL(ch)), "(GC) %s has advanced %s to level %d (from %d)", GET_NAME(ch),
                GET_NAME(victim), newlevel, oldlevel);

    gain_exp(victim, exp_next_level(newlevel - 1, GET_CLASS(victim)) - GET_EXP(victim) + 1, GAIN_IGNORE_ALL);
    save_player_char(victim);

    /*
     * Normally, advance_level calls init_mem_list when losing levels,
     * but since we start over from level 1 in do_advance, we need
     * to call it ourselves.
     */
    if (newlevel < oldlevel)
        init_mem_list(victim);
}

void perform_restore(CharData *vict) {
    extern void dispel_harmful_magic(CharData * ch);

    dispel_harmful_magic(vict);
    GET_HIT(vict) = MAX(GET_MAX_HIT(vict), GET_HIT(vict));
    GET_MANA(vict) = MAX(GET_MAX_MANA(vict), GET_MANA(vict));
    GET_MOVE(vict) = MAX(GET_MAX_MOVE(vict), GET_MOVE(vict));
    cure_laryngitis(vict);

    /* Since we didn't call alter_hit, which calls hp_pos_check */
    hp_pos_check(vict, nullptr, 0);
}

ACMD(do_restore) {
    CharData *vict;
    int i;

    one_argument(argument, buf);
    if (!*buf)
        send_to_char("Whom do you wish to restore?\n", ch);
    else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, buf))))
        send_to_char(NOPERSON, ch);
    else {
        send_to_char("They gasp as their body is fully healed.\n", ch);
        act("You have been fully healed by $N!", false, vict, 0, ch, TO_CHAR | TO_SLEEP);
        perform_restore(vict);
        if ((GET_LEVEL(ch) >= LVL_GRGOD) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
            for (i = 1; i <= TOP_SKILL; i++)
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

    one_argument(argument, buf);
    if (!(*buf))
        send_to_char("To whom do you wish to cause pain?\n", ch);
    else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, buf))))
        send_to_char(NOPERSON, ch);
    else {
        int change = GET_HIT(vict) * 0.1;
        perform_pain(vict);
        sprintf(buf,
                "Their body writhes in pain.  Your displeasure has been known. "
                "(%d)\n",
                change);
        send_to_char(buf, ch);
        act("A wave of pain and pestilence sent by $N harms you!", false, vict, 0, ch, TO_CHAR | TO_SLEEP);
    }
}

void perform_immort_vis(CharData *ch) {
    void appear(CharData * ch);

    if (GET_INVIS_LEV(ch) == 0 && !EFF_FLAGGED(ch, EFF_INVISIBLE) && !IS_HIDDEN(ch) &&
        !EFF_FLAGGED(ch, EFF_CAMOUFLAGED)) {
        send_to_char("You are already fully visible.\n", ch);
        return;
    }

    GET_INVIS_LEV(ch) = 0;
    appear(ch);
    send_to_char("You are now fully visible.\n", ch);
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
    sprintf(buf, "Your invisibility level is %d.\n", level);
    send_to_char(buf, ch);
}

ACMD(do_invis) {
    int level;

    if (IS_NPC(ch)) {
        send_to_char("You can't do that!\n", ch);
        return;
    }

    one_argument(argument, arg);
    if (!*arg) {
        if (GET_INVIS_LEV(ch) > 0)
            perform_immort_vis(ch);
        else
            perform_immort_invis(ch, GET_LEVEL(ch));
    } else {
        level = atoi(arg);
        if (level > GET_LEVEL(ch))
            send_to_char("You can't go invisible above your own level.\n", ch);
        else if (level < 1)
            perform_immort_vis(ch);
        else
            perform_immort_invis(ch, level);
    }
}

ACMD(do_gecho) {
    DescriptorData *pt;

    skip_spaces(&argument);

    if (!*argument)
        send_to_char("That must be a mistake...\n", ch);
    else {
        sprintf(buf, "%s&0\n", argument);
        for (pt = descriptor_list; pt; pt = pt->next)
            if (!pt->connected && pt->character && pt->character != ch)
                send_to_char(buf, pt->character);
        if (PRF_FLAGGED(ch, PRF_NOREPEAT))
            send_to_char(OK, ch);
        else
            send_to_char(buf, ch);
    }
}

/* completely redone in order to add poofs to the pfile */
/* --Fingon                                             */
ACMD(do_poofset) {
    skip_spaces(&argument);

    switch (subcmd) {
    case SCMD_POOFIN:
        if (argument[0] != '\0') {
            GET_POOFIN(ch) = strdup(argument);
            sprintf(buf, "Your poofin is now: %s\n", GET_POOFIN(ch));
        } else
            sprintf(buf, "Your poofin is: %s\n", GET_POOFIN(ch));
        break;
    case SCMD_POOFOUT:
        if (argument[0] != '\0') {
            GET_POOFOUT(ch) = strdup(argument);
            sprintf(buf, "Your poofin is now: %s\n", GET_POOFOUT(ch));
        } else
            sprintf(buf, "Your poofout is: %s\n", GET_POOFOUT(ch));
        break;
    default:
        return;
        break;
    }

    send_to_char(buf, ch);
}

ACMD(do_dc) {
    DescriptorData *d = nullptr;
    int num_to_dc;

    one_argument(argument, arg);
    if ((num_to_dc = atoi(arg))) {
        for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next)
            ;
        if (!d) {
            send_to_char("No such connection.\n", ch);
            return;
        }
    } else if (*arg)
        for (d = descriptor_list; d; d = d->next) {
            if (d->character && GET_NAME(d->character) && !strcasecmp(GET_NAME(d->character), arg))
                break;
            if (d->original && GET_NAME(d->original) && !strcasecmp(GET_NAME(d->original), arg))
                break;
        }

    if (!d) {
        send_to_char("Usage: DC <connection number | character name> (type USERS for a list)\n", ch);
        return;
    }

    if ((d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) ||
        (d->original && GET_LEVEL(d->original) >= GET_LEVEL(ch))) {
        send_to_char("Umm.. maybe that's not such a good idea...\n", ch);
        return;
    }
    num_to_dc = d->desc_num;
    close_socket(d);
    sprintf(buf, "Connection #%d closed.\n", num_to_dc);
    send_to_char(buf, ch);
    sprintf(buf, "(GC) Connection closed by %s.", GET_NAME(ch));
    log("%s", buf);
    ;
}

ACMD(do_wizlock) {
    int value;
    const char *when;

    one_argument(argument, arg);
    if (*arg) {
        value = atoi(arg);
        if (value < 0 || value > GET_LEVEL(ch)) {
            send_to_char("Invalid wizlock value.\n", ch);
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
        sprintf(buf, "The game is %s completely open.\n", when);
        break;
    case 1:
        sprintf(buf, "The game is %s closed to new players.\n", when);
        break;
    default:
        sprintf(buf, "Only level %d and above may enter the game %s.\n", should_restrict, when);
        break;
    }
    send_to_char(buf, ch);
    if (!*arg && restrict_reason == RESTRICT_AUTOBOOT)
        send_to_char("The restriction was set automatically by the automatic rebooting system.\n", ch);
}

ACMD(do_date) {
    char *tmstr;
    time_t mytime;
    int d, h, m;
    extern time_t *boot_time;

    if (subcmd == SCMD_DATE)
        mytime = time(0);
    else
        mytime = boot_time[0];

    tmstr = (char *)asctime(localtime(&mytime));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    if (subcmd == SCMD_DATE)
        sprintf(buf, "Current machine time: %s\n", tmstr);
    else {
        mytime = time(0) - boot_time[0];
        d = mytime / 86400;
        h = (mytime / 3600) % 24;
        m = (mytime / 60) % 60;

        sprintf(buf, "Up since %s: %d day%s, %d:%02d\n", tmstr, d, ((d == 1) ? "" : "s"), h, m);
    }

    send_to_char(buf, ch);
}

ACMD(do_force) {
    DescriptorData *i, *next_desc;
    CharData *vict, *next_force;
    char to_force[MAX_INPUT_LENGTH + 2];

    half_chop(argument, arg, to_force);

    sprintf(buf1, "$n has forced you to '%s'.", to_force);

    if (!*arg || !*to_force)
        send_to_char("Whom do you wish to force do what?\n", ch);
    else if ((GET_LEVEL(ch) < LVL_GRGOD) || (strcmp("all", arg) && strcmp("room", arg))) {
        if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg))))
            send_to_char(NOPERSON, ch);
        else if (GET_LEVEL(ch) <= GET_LEVEL(vict))
            send_to_char("No, no, no!\n", ch);
        else {
            send_to_char(OK, ch);
            act(buf1, false, ch, nullptr, vict, TO_VICT);
            sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
            mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), true);
            command_interpreter(vict, to_force);
        }
    } else if (!strcmp("room", arg)) {
        send_to_char(OK, ch);
        sprintf(buf, "(GC) %s forced room %d to %s", GET_NAME(ch), world[ch->in_room].vnum, to_force);
        mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), true);

        for (vict = world[ch->in_room].people; vict; vict = next_force) {
            next_force = vict->next_in_room;
            if (GET_LEVEL(vict) >= GET_LEVEL(ch))
                continue;
            act(buf1, false, ch, nullptr, vict, TO_VICT);
            command_interpreter(vict, to_force);
        }
    } else { /* force all */
        send_to_char(OK, ch);
        sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
        mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), true);

        for (i = descriptor_list; i; i = next_desc) {
            next_desc = i->next;

            if (i->connected || !(vict = i->character) || GET_LEVEL(vict) >= GET_LEVEL(ch))
                continue;
            act(buf1, false, ch, nullptr, vict, TO_VICT);
            command_interpreter(vict, to_force);
        }
    }
}

ACMD(do_wiznet) {
    DescriptorData *d;
    char any = false;
    bool explicit_level = false;
    int level = LVL_IMMORT;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument) {
        send_to_char(
            "Usage: wiznet <text> | #<level> <text>\n "
            "       wiznet @<level> | wiz @\n",
            ch);
        return;
    }

    switch (*argument) {
    case '#':
        one_argument(argument + 1, buf1);
        if (is_number(buf1)) {
            half_chop(argument + 1, buf1, argument);
            level = MAX(atoi(buf1), 1);
            if (level > GET_LEVEL(ch)) {
                send_to_char("You can't wizline above your own level.\n", ch);
                return;
            }
            explicit_level = true;
        }
        break;
    case '@':
        for (d = descriptor_list; d; d = d->next) {
            if (!d->connected && GET_LEVEL(d->character) >= LVL_IMMORT && !PRF_FLAGGED(d->character, PRF_NOWIZ) &&
                (CAN_SEE(ch, d->character) || GET_LEVEL(ch) == LVL_IMPL)) {
                if (!any) {
                    sprintf(buf1, "Gods online:\n");
                    any = true;
                }
                sprintf(buf1, "%s  %s", buf1, GET_NAME(d->character));
                if (PLR_FLAGGED(d->character, PLR_WRITING))
                    sprintf(buf1, "%s (Writing)\n", buf1);
                else if (EDITING(d))
                    sprintf(buf1, "%s (Writing)\n", buf1);
                else if (PLR_FLAGGED(d->character, PLR_MAILING))
                    sprintf(buf1, "%s (Writing mail)\n", buf1);
                else
                    sprintf(buf1, "%s\n", buf1);
            }
        }
        any = false;
        for (d = descriptor_list; d; d = d->next) {
            if (!d->connected && GET_LEVEL(d->character) >= LVL_IMMORT && PRF_FLAGGED(d->character, PRF_NOWIZ) &&
                CAN_SEE(ch, d->character)) {
                if (!any) {
                    sprintf(buf1, "%sGods offline:\n", buf1);
                    any = true;
                }
                sprintf(buf1, "%s  %s\n", buf1, GET_NAME(d->character));
            }
        }
        send_to_char(buf1, ch);
        return;
        break;
    case '\\':
        ++argument;
        break;
    default:
        break;
    }
    if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
        send_to_char("You are offline!\n", ch);
        return;
    }
    skip_spaces(&argument);

    if (!*argument) {
        send_to_char("Don't bother the gods like that!\n", ch);
        return;
    }

    if (explicit_level) {
        sprintf(buf1, "&6%s: <%d> %s&0\n", GET_NAME(ch), level, argument);
        sprintf(buf2, "&6Someone: <%d> %s&0\n", level, argument);
    } else {
        sprintf(buf1, "&6%s: %s&0\n", GET_NAME(ch), argument);
        sprintf(buf2, "&6Someone: %s&0\n", argument);
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
        if (CAN_SEE(d->character, ch))
            send_to_char(buf1, d->character);
        else
            send_to_char(buf2, d->character);
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        send_to_char(OK, ch);
}

/* added for name approval system by fingh 3/31/99 */
/* Complete name approval system redone by Zantir/David Endre 5/4/99 */
void broadcast_name(char *name) {
    char temp[80];
    DescriptorData *d;

    *temp = '\0';

    sprintf(temp, "&2&b%s is awaiting name approval.&0\n", name);

    for (d = descriptor_list; d; d = d->next)
        if (IS_PLAYING(d) && d->character && GET_LEVEL(d->character) >= LVL_IMMORT &&
            !PLR_FLAGGED(d->character, PLR_WRITING) && !PLR_FLAGGED(d->character, PLR_MAILING) && !EDITING(d))
            write_to_output(temp, d);
}

const char *reasons[] = {
    "Compound words. Examples: LadyJade, BraveBlade, ImmortalSoul\n These "
    "types of names give a player an unearned arrogance above\n other "
    "players and in some instances above the gods.",
    "Offensive words or names, mispelled or backwards. Eg: Sgurd Kcuf Emod",
    "Names from known mythos, movies etc... Examples: Feyd, Conan, Zeus\n "
    "Come on, you can be more creative than reliance on an existing known\n "
    "character.",
    "Names that do not fit the character you are playing. Examples:\n A "
    "human named Gruzel\n A troll named Bob\n A boy named Sue\n An orc "
    "named Ripenthiel",
    "No offensive words or names from other languages or cultures.",
    "No Nouns verbs or adverbs. Eg. Jester, Red, Dog, Bloody, Hotly, "
    "Freedom\n Pervert, PC, Trouble, McIntosh, Desperado.  Sorry guys LAME.",
    "No names that resemble common nouns verbs or adverbs. Eg. Jesterx,\n "
    "Redx, Chamelion, Desperato etc...",
    "No modern names. Examples: Rina John Mike Steve",
    "Other...Please talk to a god for further information."};

ACMD(do_name) {
    char buffer[MAX_STRING_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    DescriptorData *d;
    int choice = 0;
    int z = 0;

    *buffer = '\0';

    switch (subcmd) {
    case SCMD_ACCEPT:
        one_argument(argument, arg);
        if (!*arg) {
            send_to_char("Accept WHO?\n", ch);
            return;
        }
        /* cycle through the current connections */
        for (d = descriptor_list; d; d = d->next) {

            /* added to stop crashes while people are at CON_QANSI etc. */
            if (!d->character)
                continue;

            /* make sure player is either at the CON_NAME_WAIT_APPROVAL state
               or has been auto-accepted and still pending approval
               and the player is not yourself
             */
            if (((STATE(d) == CON_NAME_WAIT_APPROVAL) || (PLR_FLAGGED(d->character, PLR_NAPPROVE))) &&
                ((d != ch->desc) && (strncasecmp(GET_NAME(d->character), arg, strlen(arg)) == 0))) {
                /* create the player in the pfile if he isn't there already */
                if (GET_PFILEPOS(d->character) < 0) {
                    GET_PFILEPOS(d->character) = create_player_index_entry(GET_NAME(d->character));
                    /* initialize the the player */
                    init_player(d->character);
                    /* log the new player */
                    sprintf(buf, "%s [%s] new player.", GET_NAME(d->character), d->host);
                    mudlog(buf, NRM, LVL_IMMORT, true);
                }
                /* accept the players name */
                REMOVE_FLAG(PLR_FLAGS(d->character), PLR_NAPPROVE);
                save_player_char(d->character);

                /* log the acceptance of the name */
                sprintf(buf, "The name: %s has been accepted by %s.", GET_NAME(d->character), GET_NAME(ch));
                mudlog(buf, NRM, LVL_IMMORT, true);

                /* remove the choose new name flag */
                if (PLR_FLAGGED(d->character, PLR_NEWNAME))
                    REMOVE_FLAG(PLR_FLAGS(d->character), PLR_NEWNAME);

                /* check if the player is waiting approval or already auto approved */
                if (STATE(d) == CON_NAME_WAIT_APPROVAL) {
                    /* send the motd and change connection states */
                    write_to_output(get_text(TEXT_MOTD), d);
                    write_to_output("\n\n*** PRESS RETURN: ", d);
                    STATE(d) = CON_RMOTD;
                }

                /* tell the player the name has been accepted */
                sprintf(buf, "&2&bThe name: %s has been accepted.&0\n", GET_NAME(d->character));
                send_to_char(buf, ch);
                send_to_char(buf, d->character);
                return;
            }
        }
        send_to_char("No such player waiting for approval.\n", ch);
        break;
    case SCMD_DECLINE:
        two_arguments(argument, arg, arg2);
        if (!*arg) {
            send_to_char("Decline WHO?\n", ch);
            return;
        }
        if (!*arg2) {
            send_to_char("For what reason? Type 'nlist 0' for a list.\n", ch);
            return;
        }

        choice = atoi(arg2);
        choice--;
        if (choice < 0 || choice > 7)
            choice = 8;
        /* cycle through connected players */
        for (d = descriptor_list; d; d = d->next) {
            /* added to stop crashes while people are at CON_QANSI etc. */
            if (!d->character)
                continue;

            if (((STATE(d) == CON_NAME_WAIT_APPROVAL) || (PLR_FLAGGED(d->character, PLR_NAPPROVE))) &&
                (d != ch->desc) && (strncasecmp(GET_NAME(d->character), arg, strlen(arg)) == 0)) {
                sprintf(buf, "The name: %s has been declined by %s, reason %d.", GET_NAME(d->character), GET_NAME(ch),
                        choice + 1);
                mudlog(buf, NRM, LVL_IMMORT, true);
                sprintf(buf, "&2&bThe name: %s has been declined.&0\n", GET_NAME(d->character));
                send_to_char(buf, ch);
                send_to_xnames(GET_NAME(d->character));
                write_to_output("That name has been rejected, because it breaks this rule:\n", d);
                *buf = '\0';
                sprintf(buf, "%s\n", reasons[choice]);
                write_to_output(buf, d);
                SET_FLAG(PLR_FLAGS(d->character), PLR_NEWNAME);
                if (STATE(d) == CON_NAME_WAIT_APPROVAL) {
                    write_to_output("Please try another name.\n", d);
                    write_to_output("Name: ", d);
                    STATE(d) = CON_NEW_NAME;
                } else {
                    write_to_output(
                        "&6&bYou are welcome to play; however, you will be "
                        "prompted&0\n&6&bfor a new name on your next login.&0\n",
                        d);
                    save_player_char(d->character);
                }
                return;
            }
        }
        send_to_char("No such player waiting for approval.\n", ch);
        break;
    case SCMD_LIST:
        one_argument(argument, arg);

        if (*arg) {
            if (atoi(arg) == 0) {
                *buf = '\0';
                for (z = 0; z < 8; z++)
                    sprintf(buf, "%s%d. %s\n", buf, z + 1, reasons[z]);
                send_to_char(buf, ch);
            }
        }

        for (d = descriptor_list; d; d = d->next) {
            /* added this NULL check to stop crashes when people where CON_QANSI etc
             */
            if (d->character) {
                if (((STATE(d) == CON_NAME_WAIT_APPROVAL) ||
                     (PLR_FLAGGED(d->character, PLR_NAPPROVE) && !(PLR_FLAGGED(d->character, PLR_NEWNAME)))) &&
                    (d != ch->desc)) {
                    sprintf(buffer, "%s%s\n", buffer, GET_NAME(d->character));
                }
            }
        }

        if (strlen(buffer) < 2) {
            sprintf(buffer, "No one.\n");
            send_to_char(buffer, ch);
        } else {
            send_to_char("The following people are awaiting name approval:\n", ch);
            send_to_char(buffer, ch);
        }
        break;
    default:
        break; /* maybe send an error to log here? */
    }          /* end switch */
}

ACMD(do_zreset) {
    void reset_zone(int zone, byte pop);

    int i, j;

    one_argument(argument, arg);

    if (!*arg || *arg == '.') {
        i = world[ch->in_room].zone;
    } else if (*arg == '*') {
        if (GET_LEVEL(ch) <= LVL_HEAD_B)
            return;
        for (i = 0; i <= top_of_zone_table; i++)
            reset_zone(i, false);
        send_to_char("Reset world.\n", ch);
        sprintf(buf, "(GC) %s reset entire world.", GET_NAME(ch));
        mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), true);
        return;
    } else if (!isdigit(*arg)) {
        send_to_char("Usage: zreset [<zone-number>]\n", ch);
        return;
    } else {
        j = atoi(arg);
        for (i = 0; i <= top_of_zone_table; i++)
            if (zone_table[i].number == j)
                break;
    }
    if (GET_LEVEL(ch) < LVL_GRGOD && !has_olc_access(ch, zone_table[i].number)) {
        send_to_char("Testing testing !!are you clear to do this!! no I dont think so\n", ch);
        return;
    }
    if (i >= 0 && i <= top_of_zone_table) {
        reset_zone(i, false);
        sprintf(buf, "Reset zone %d (#%d): %s.\n", i, zone_table[i].number, zone_table[i].name);
        send_to_char(buf, ch);
        sprintf(buf, "(GC) %s reset zone %d (%s)", GET_NAME(ch), zone_table[i].number, zone_table[i].name);
        mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), true);
    } else
        send_to_char("Invalid zone number.\n", ch);
}

static char *center_wiztitle(char *wiztitle, int len, int noansi_len) {
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

    return strdup(centerbuf);
}

void do_wiztitle(char *outbuf, CharData *vict, char *argument) {
    int noansi_len, len;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    /* check size of title w/out ansi chars */
    len = strlen(argument);
    noansi_len = len - count_color_chars(argument);

    if (IS_NPC(vict))
        sprintf(buf, "Mobile's do not have godly titles...nice try.\n");
    else if (strstr(argument, "[") || strstr(argument, "]"))
        sprintf(buf, "Godly Titles can't contain the [ or ] characters.\n");
    else if (strlen(argument) >= MAX_INPUT_LENGTH)
        sprintf(buf, "Sorry, godly titles can't be longer than %d characters.\n", MAX_INPUT_LENGTH - 1);
    else if (noansi_len > WIZ_TITLE_WIDTH)
        sprintf(buf,
                "Sorry, text portion of godly titles can't be longer than 12 "
                "characters.\n");
    else if (noansi_len == 0) {
        GET_WIZ_TITLE(vict) = nullptr;
        sprintf(buf, "Okay, %s's godly title reset to default.\n", GET_NAME(vict));
    } else {
        if (GET_WIZ_TITLE(vict))
            free(GET_WIZ_TITLE(vict));
        GET_WIZ_TITLE(vict) = center_wiztitle(argument, len, noansi_len);
        sprintf(buf, "Okay, %s's godly title is now [%s]\n", GET_NAME(vict), GET_WIZ_TITLE(vict));
    }
}

/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil) {
    CharData *vict;
    long result;

    one_argument(argument, arg);

    if (!*arg)
        send_to_char("Yes, but for whom?!?\n", ch);
    else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg))))
        send_to_char("There is no such player.\n", ch);
    else if (IS_NPC(vict))
        send_to_char("You can't do that to a mob!\n", ch);
    else if (GET_LEVEL(vict) > GET_LEVEL(ch))
        send_to_char("Hmmm...you'd better not.\n", ch);
    else {
        switch (subcmd) {
        case SCMD_REROLL:
            send_to_char("Rerolled not functinal for now\n", ch);
            break;
            /*sprintf(buf, "(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
               log("%s", buf);;
               sprintf(buf, "New stats: Str %d/%d, Int %d, Wis %d, Dex %d, Con %d, Cha
               %d\n", GET_STR(vict), GET_ADD(vict), GET_INT(vict), GET_WIS(vict),
               GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
               send_to_char(buf, ch);
               break; */
        case SCMD_PARDON:
            if (!PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(vict, PLR_KILLER)) {
                send_to_char("Your victim is not flagged.\n", ch);
                return;
            }
            REMOVE_FLAG(PLR_FLAGS(vict), PLR_THIEF);
            REMOVE_FLAG(PLR_FLAGS(vict), PLR_KILLER);
            send_to_char("Pardoned.\n", ch);
            send_to_char("You have been pardoned by the Gods!\n", vict);
            sprintf(buf, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
            mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), true);
            break;
        case SCMD_NOTITLE:
            result = PLR_TOG_CHK(vict, PLR_NOTITLE);
            sprintf(buf, "(GC) Notitle %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
            mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), true);
            strcat(buf, "\n");
            send_to_char(buf, ch);
            break;
        case SCMD_SQUELCH:
            result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
            sprintf(buf, "(GC) Squelch %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
            mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), true);
            strcat(buf, "\n");
            send_to_char(buf, ch);
            break;
        case SCMD_FREEZE:
            if (ch == vict) {
                send_to_char("Oh, yeah, THAT'S real smart...\n", ch);
                return;
            }
            if (PLR_FLAGGED(vict, PLR_FROZEN)) {
                send_to_char("Your victim is already pretty cold.\n", ch);
                return;
            }
            SET_FLAG(PLR_FLAGS(vict), PLR_FROZEN);
            GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
            send_to_char(
                "A bitter wind suddenly rises and drains every erg of heat "
                "from your body!\nYou feel frozen!\n",
                vict);
            send_to_char("Frozen.\n", ch);
            act("A sudden cold wind conjured from nowhere freezes $n!", false, vict, 0, 0, TO_ROOM);
            sprintf(buf, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
            mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), true);
            break;
        case SCMD_THAW:
            if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
                send_to_char("Sorry, your victim is not morbidly encased in ice at the moment.\n", ch);
                return;
            }
            if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
                sprintf(buf, "Sorry, a level %d God froze %s... you can't unfreeze %s.\n", GET_FREEZE_LEV(vict),
                        GET_NAME(vict), HMHR(vict));
                send_to_char(buf, ch);
                return;
            }
            sprintf(buf, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
            mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), true);
            REMOVE_FLAG(PLR_FLAGS(vict), PLR_FROZEN);
            send_to_char("A fireball suddenly explodes in front of you, melting the ice!\nYou feel thawed.\n", vict);
            send_to_char("Thawed.\n", ch);
            act("A sudden fireball conjured from nowhere thaws $n!", false, vict, 0, 0, TO_ROOM);
            break;
        case SCMD_UNAFFECT:
            if (vict->effects) {
                while (vict->effects)
                    effect_remove(vict, vict->effects);
                send_to_char(
                    "There is a brief flash of light!\n"
                    "You feel slightly different.\n",
                    vict);
                send_to_char("All spells removed.\n", ch);
                check_regen_rates(vict);
                /* if it had been an animated and now isnt then it needs to die */
                if (MOB_FLAGGED(vict, MOB_ANIMATED) && !EFF_FLAGGED(vict, EFF_ANIMATED)) {
                    act("$n freezes and falls twitching to the ground.", false, vict, 0, 0, TO_ROOM);
                    die(vict, nullptr);
                    return;
                }
            } else {
                send_to_char("Your victim does not have any effects!\n", ch);
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

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

ACMD(do_set) {
    int i, l;
    CharData *vict = nullptr, *cbuf = nullptr;
    char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], val_arg[MAX_INPUT_LENGTH];
    int on = 0, off = 0, value = 0;
    char is_file = 0, is_player = 0, save = true;
    int player_i = 0;
    int get_race_align(CharData * ch);
    int parse_diety(CharData * ch, char arg);

    struct set_struct {
        const char *cmd;
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
                  {"\n", 0, BOTH, MISC}};

    half_chop(argument, name, buf);
    if (!strcmp(name, "file")) {
        is_file = 1;
        half_chop(buf, name, buf);
    } else if (!strcmp(name, "player")) {
        is_player = 1;
        half_chop(buf, name, buf);
    } else if (!strcmp(name, "mob")) {
        half_chop(buf, name, buf);
    }
    half_chop(buf, field, buf);
    strcpy(val_arg, buf);

    if (!*name || !*field) {
        pprintf(ch, "Usage: set <victim> <field> <value>\n");
        pprintf(ch, "Set fields currently available to you:\n");
        for (l = 0; *(fields[l].cmd) != '\n'; l++) {
            if (fields[l].level <= GET_LEVEL(ch) && *(fields[l].cmd)) {
                pprintf(ch, "%-20.20s %-20.20s %-6.6s\n", fields[l].cmd,
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
                send_to_char("There is no such player.\n", ch);
                return;
            }
        } else {
            if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, name)))) {
                send_to_char("There is no such creature.\n", ch);
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
                send_to_char("Sorry, you can't do that.\n", ch);
                return;
            }
            vict = cbuf;
        } else {
            free_char(cbuf);
            send_to_char("There is no such player.\n", ch);
            return;
        }
    }
    if (GET_LEVEL(ch) != LVL_HEAD_C) {
        if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
            send_to_char("Maybe that's not such a great idea...\n", ch);
            return;
        }
    }
    for (l = 0; *(fields[l].cmd) != '\n'; l++)
        if (!strncasecmp(field, fields[l].cmd, strlen(field)))
            break;

    if (GET_LEVEL(ch) < fields[l].level) {
        send_to_char("You are not godly enough for that!\n", ch);
        return;
    }
    if (IS_NPC(vict) && !(fields[l].pcnpc & NPC)) {
        send_to_char("You can't do that to a beast!\n", ch);
        return;
    } else if (!IS_NPC(vict) && !(fields[l].pcnpc & PC)) {
        send_to_char("That can only be done to a beast!\n", ch);
        return;
    }
    if (fields[l].type == BINARY) {
        if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
            on = 1;
        else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
            off = 1;
        if (!(on || off)) {
            send_to_char("Value must be on or off.\n", ch);
            return;
        }
    } else if (fields[l].type == NUMBER) {
        value = atoi(val_arg);
    }

    strcpy(buf, OK);
    switch (l) {
    case 0:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
        break;
    case 1:
        send_to_char("This option is unused.  You should not have seen this!\n", ch);
        return;
    case 2:
        set_title(vict, val_arg);
        sprintf(buf, "%s's title is now: %s\n", GET_NAME(vict), GET_TITLE(vict));
        break;
    case 3:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
        on = !on; /* so output will be correct */
        break;
    case 4:
        /* Base hit is in a player special. Don't set for npcs */
        if (IS_NPC(vict))
            GET_MAX_HIT(vict) = RANGE(1, 500000);
        else {
            GET_BASE_HIT(vict) = RANGE(1, 500000);
            effect_total(vict);
        }
        break;
    case 5:
        vict->points.max_mana = RANGE(1, 500000);
        effect_total(vict);
        break;
    case 6:
        vict->points.max_move = RANGE(1, 500000);
        effect_total(vict);
        break;
    case 7:
        vict->points.hit = RANGE(-9, vict->points.max_hit);
        effect_total(vict);
        break;
    case 8:
        vict->points.mana = RANGE(0, vict->points.max_mana);
        effect_total(vict);
        break;
    case 9:
        vict->points.move = RANGE(0, vict->points.max_move);
        effect_total(vict);
        break;
    case 10:
        GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
        effect_total(vict);
        break;
    case 11:
        GET_NATURAL_STR(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 12:
        /*  This is no longer used --Gurlaek 6/23/1999
           vict->view_abils.str_add = RANGE(0, 100);
           vict->view_abils.str_add = value;
           if (value > 0)
           vict->view_abils.str = 100;
           effect_total(vict);
         */
        break;
        /*Edited by Proky,  values for intel */
    case 13:
        GET_NATURAL_INT(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 14:
        GET_NATURAL_WIS(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 15:
        GET_NATURAL_DEX(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 16:
        GET_NATURAL_CON(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 17:
        if (!strcmp(val_arg, "male"))
            vict->player.sex = SEX_MALE;
        else if (!strcmp(val_arg, "female"))
            vict->player.sex = SEX_FEMALE;
        else if (!strcmp(val_arg, "neutral"))
            vict->player.sex = SEX_NEUTRAL;
        else if (!strcmp(val_arg, "nonbinary"))
            vict->player.sex = SEX_NONBINARY;
        else {
            send_to_char("Must be 'male', 'female', 'nonbinary', or 'neutral'.\n", ch);
            return;
        }
        break;
    case 18:
        GET_AC(vict) = RANGE(MIN_AC, MIN_AC);
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
                sprintf(buf, "Removed %d from %s allowed OLC zones.\n", value, GET_NAME(vict));
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
            sprintf(buf, "Added %d to %s allowed OLC zones.\n", value, GET_NAME(vict));
        }
    } break;
    case 20:
        GET_COOLDOWN(vict, CD_INNATE_ASCEN) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_BRILL) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_CHAZ) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_SYLL) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_TASS) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_TREN) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_INVISIBLE) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_LEVITATE) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_CREATE) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_DARKNESS) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_HARNESS) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_BREATHE) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_ILLUMINATION) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_FAERIE_STEP) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_BLINDING_BEAUTY) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_INNATE_STATUE) = RANGE(0, 100);
        break;
    case 21:
        vict->points.exp = RANGE(0, 299999999);
        break;
    case 22:
        GET_BASE_HITROLL(vict) = RANGE(MIN_DAMROLL, MAX_DAMROLL);
        effect_total(vict);
        break;
    case 23:
        GET_BASE_DAMROLL(vict) = RANGE(MIN_DAMROLL, MAX_DAMROLL);
        effect_total(vict);
        break;
    case 24:
        if (GET_LEVEL(ch) < LVL_HEAD_C && ch != vict) {
            strcpy(buf, "You aren't godly enough for that!\n");
            save = false;
            break;
        }
        GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
        break;
    case 25:
        if (GET_LEVEL(ch) < LVL_HEAD_C && ch != vict) {
            strcpy(buf, "You aren't godly enough for that!\n");
            save = false;
            break;
        }
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
        break;
    case 26:
        if (ch == vict) {
            send_to_char("Better not -- could be a long winter!\n", ch);
            /* OK: ch == vict so no memory was allocated */
            return;
        }
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
        break;
    case 27:
    case 28:
        /* GET_PRACTICES(vict) = RANGE(0, 100); */
        break;
    case 29:
    case 30:
    case 31:
        if (!strcmp(val_arg, "off")) {
            GET_COND(vict, (l - 29)) = (char)-1;
            sprintf(buf, "%s's %s now off.\n", GET_NAME(vict), fields[l].cmd);
        } else if (is_number(val_arg)) {
            value = atoi(val_arg);
            RANGE(0, 24);
            GET_COND(vict, (l - 29)) = (char)value;
            sprintf(buf, "%s's %s set to %d.\n", GET_NAME(vict), fields[l].cmd, value);
        } else {
            strcpy(buf, "Must be 'off' or a value from 0 to 24.\n");
            save = false;
            break;
        }
        check_regen_rates(vict);
        break;
    case 32:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
        break;
    case 33:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
        break;
    case 34:
        if (value > GET_LEVEL(ch) || value > LVL_OVERLORD) {
            strcpy(buf, "You can't do that.\n");
            save = false;
            break;
        }
        if (GET_CLAN(vict) && IS_CLAN_MEMBER(vict))
            GET_CLAN(vict)->power -= GET_LEVEL(vict);
        RANGE(0, LVL_IMPL);
        vict->player.level = (byte)value;
        if (GET_CLAN(vict) && IS_CLAN_MEMBER(vict))
            GET_CLAN(vict)->power += GET_LEVEL(vict);
        break;
    case 35:
        if ((i = real_room(value)) < 0) {
            strcpy(buf, "No room exists with that number.\n");
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
    case 39:
        if ((i = parse_class(ch, vict, val_arg)) == CLASS_UNDEFINED) {
            save = false;
            buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
            break;
        }
        if (i == GET_CLASS(vict)) {
            sprintf(buf, "%s already %s.", vict == ch ? "You're" : "$e's", with_indefinite_article(CLASS_FULL(vict)));
        } else {
            convert_class(vict, i);
            if (ch != vict) {
                sprintf(buf, "$n has changed your class to %s!", CLASS_FULL(vict));
                act(buf, false, ch, 0, vict, TO_VICT);
            }
            sprintf(buf, "Okay, %s now %s.", vict == ch ? "you are" : "$n is",
                    with_indefinite_article(CLASS_FULL(vict)));
        }
        act(buf, false, vict, 0, ch, TO_VICT);
        buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
        break;
    case 40:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
        break;
    case 41:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
        break;
    case 42:
        if (is_number(val_arg)) {
            value = atoi(val_arg);
            if (real_room(value) != NOWHERE) {
                GET_HOMEROOM(vict) = value;
                sprintf(buf, "%s's home room is now #%d.\n", GET_NAME(vict), GET_HOMEROOM(vict));
            } else {
                strcpy(buf, "That room does not exist!\n");
                save = false;
            }
        } else {
            strcpy(buf, "Must be a room's vnum.\n");
            save = false;
        }
        break;
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
            send_to_char("Not while they're logged in!\n", ch);
            return;
        }
        if (GET_IDNUM(ch) > 2 && !strcasecmp(GET_NAME(ch), "zuur")) {
            strcpy(buf, "Please don't use this command, yet.\n");
            save = false;
        } else if (GET_LEVEL(vict) > LVL_HEAD_C) {
            strcpy(buf, "You cannot change that.\n");
            save = false;
        } else {
            strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);
            GET_PASSWD(vict)[MAX_PWD_LENGTH] = '\0';
            sprintf(buf, "Password changed to '%s'.\n", val_arg);
        }
        break;
    case 46:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
        break;
    case 47:
        GET_NATURAL_CHA(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
        effect_total(vict);
        break;
    case 48:
        if ((i = parse_race(ch, vict, val_arg)) == RACE_UNDEFINED) {
            save = false;
            buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
            break;
        }
        if (i == GET_RACE(vict)) {
            sprintf(buf, "%s already %s.", vict == ch ? "You're" : "$e's", with_indefinite_article(RACE_FULL(vict)));
        } else {
            convert_race(vict, i);
            if (ch != vict) {
                sprintf(buf, "$n has changed your race to %s!", RACE_FULL(vict));
                act(buf, false, ch, 0, vict, TO_VICT);
            }
            sprintf(buf, "Okay, %s now %s.", vict == ch ? "you are" : "$n is",
                    with_indefinite_article(RACE_FULL(vict)));
        }
        act(buf, false, vict, 0, ch, TO_VICT);
        buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
        break;
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
        GET_PLATINUM(vict) = RANGE(0, 100000000);
        break;
    case 54:
        GET_GOLD(vict) = RANGE(0, 100000000);
        break;
    case 55:
        GET_SILVER(vict) = RANGE(0, 100000000);
        break;
    case 56:
        GET_COPPER(vict) = RANGE(0, 100000000);
        break;
    case 57:
        GET_BANK_PLATINUM(vict) = RANGE(0, 100000000);
        break;
    case 58:
        GET_BANK_GOLD(vict) = RANGE(0, 100000000);
        break;
    case 59:
        GET_BANK_SILVER(vict) = RANGE(0, 100000000);
        break;
    case 60:
        GET_BANK_COPPER(vict) = RANGE(0, 100000000);
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
        do_wiztitle(buf, vict, val_arg);
        break;
    case 67:
        GET_COOLDOWN(vict, CD_CHANT) = RANGE(0, 100);
        break;
    case 68:
        if (!*val_arg) {
            show_sizes(ch);
            save = false;
            buf[0] = '\0';
            break;
        } else if ((value = parse_size(ch, val_arg)) == SIZE_UNDEFINED) {
            save = false;
            buf[0] = '\0';
            break;
        }
        change_natural_size(vict, value);
        sprintf(buf, "%s %s now %s in size.\n", ch == vict ? "You" : GET_NAME(vict), ch == vict ? "are" : "is",
                SIZE_DESC(vict));
        break;
    case 69:
        GET_HIDDENNESS(vict) = RANGE(0, 1000);
        break;
    case 70:
        GET_RAGE(vict) = RANGE(0, 1000);
        check_regen_rates(vict);
        break;
    case 71:
        argument = any_one_arg(val_arg, field);
        skip_spaces(&argument);
        i = atoi(field);
        value = 1;
        if (GET_PERM_TITLES(ch))
            for (; GET_PERM_TITLES(ch)[value - 1]; ++value)
                ;
        if (i < 1 || i > value)
            sprintf(buf, "You can only set ptitles from 1 to %d.\n", value);
        else {
            if (strlen(argument) > MAX_TITLE_WIDTH)
                argument[MAX_TITLE_WIDTH] = '\0';
            if (i == value)
                add_perm_title(vict, argument);
            else {
                if (GET_PERM_TITLES(vict)[i - 1])
                    free(GET_PERM_TITLES(vict)[i - 1]);
                GET_PERM_TITLES(vict)[i - 1] = strdup(argument);
            }
            sprintf(buf, "%s's p-title %d is now: %s\n", GET_NAME(vict), i, GET_PERM_TITLES(vict)[i - 1]);
        }
        break;
    case 72:
        GET_HEIGHT(vict) = value;
        break;
    case 73:
        GET_WEIGHT(vict) = value;
        break;
    case 74: /* LIFE FORCE */
        if ((i = parse_lifeforce(ch, val_arg)) == LIFE_UNDEFINED) {
            save = false;
            buf[0] = '\0';
            break;
        }
        if (i == GET_LIFEFORCE(vict)) {
            sprintf(buf, "%s already infused with %s.", vict == ch ? "You're" : "$e's", LIFEFORCE_NAME(vict));
        } else {
            if (ch != vict) {
                sprintf(buf, "$n has changed your life force to %s!", lifeforces[i].name);
                act(buf, false, ch, 0, vict, TO_VICT);
            }
            sprintf(buf, "Okay, %s's life force is now %s.", vict == ch ? "you are" : "$n is", lifeforces[i].name);
            convert_lifeforce(vict, i);
        }
        act(buf, false, vict, 0, ch, TO_VICT);
        buf[0] = '\0';
        break;
    case 75: /* COMPOSITION */
        if ((i = parse_composition(ch, val_arg)) == COMP_UNDEFINED) {
            save = false;
            buf[0] = '\0';
            break;
        }
        if (i == BASE_COMPOSITION(vict)) {
            sprintf(buf, "%s already composed of %s.", vict == ch ? "You're" : "$e's", compositions[i].name);
        } else {
            if (ch != vict) {
                sprintf(buf, "$n has changed your composition to %s!", compositions[i].name);
                act(buf, false, ch, 0, vict, TO_VICT);
            }
            sprintf(buf, "Okay, %s now composed of %s.", vict == ch ? "you are" : "$n is", compositions[i].name);
            set_base_composition(vict, i);
        }
        act(buf, false, vict, 0, ch, TO_VICT);
        buf[0] = '\0';
        break;
    case 76: /* reset all bard music cooldowns */
        GET_COOLDOWN(vict, CD_MUSIC_1) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_2) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_3) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_4) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_5) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_6) = RANGE(0, 100);
        GET_COOLDOWN(vict, CD_MUSIC_7) = RANGE(0, 100);
        break;
    default:
        sprintf(buf, "Can't set that!\n");
        break;
    }

    if (fields[l].type == BINARY) {
        sprintf(buf, "%s %s for %s.\n", fields[l].cmd, ONOFF(on), GET_NAME(vict));
        CAP(buf);
    } else if (fields[l].type == NUMBER) {
        sprintf(buf, "%s's %s set to %d.\n", GET_NAME(vict), fields[l].cmd, value);
    }
    if (buf[0])
        send_to_char(CAP(buf), ch);

    if (!is_file && !IS_NPC(vict) && save)
        save_player_char(vict);

    if (is_file) {
        if (save) {
            GET_PFILEPOS(cbuf) = player_i;
            save_player_char(cbuf);
            send_to_char("Saved in file.\n", ch);
        }
        free_char(cbuf);
    }
}

ACMD(do_syslog) {
    int severity;

    one_argument(argument, arg);

    if (!*arg) {
        char_printf(ch, "The minimum severity of syslog messages you see is %s.\n",
                    sprint_log_severity(GET_LOG_VIEW(ch)));
        return;
    }

    if ((severity = parse_log_severity(arg)) < 0) {
        char_printf(ch, "Usage: syslog [ ");
        for (severity = 0; *log_severities[severity] != '\n'; ++severity)
            char_printf(ch, "%s%s", severity ? " | " : "", log_severities[severity]);
        char_printf(ch, " ]\n");
        return;
    }

    GET_LOG_VIEW(ch) = severity;

    char_printf(ch, "The minimum severity of syslog messages you will see is now %s.\n", sprint_log_severity(severity));
}

void send_to_imms(char *msg) {
    DescriptorData *pt;

    for (pt = descriptor_list; pt; pt = pt->next)
        if (!pt->connected && pt->character && GET_LEVEL(pt->character) >= LVL_GOD)
            send_to_char(msg, pt->character);
}

/* do_game recoded by 321 to allow different toggles to have different
 * minimum levels */
ACMD(do_game) {
    char field[MAX_INPUT_LENGTH], rest[MAX_INPUT_LENGTH];
    char shortbuf[40], linebuf[160];
    int i, n_visible = 0, value;
    const char *msg;

    struct {
        const char *name;
        const int has_value; /* Is it on/off (0) or does it have a value (1) */
        int *config;
        const int min_level;
        const char *turn_on;
        const char *turn_off;
        const char *enabled;
        const char *disabled;
    } commands[] = {
        {"RACES", 0, &races_allowed, LVL_ADMIN, "[&2&bSYS: %s allows race logins&0]\n",
         "[&1&bSYS: %s disallows race logins&0]\n", "Race login allowed", "Race login not allowed"},
        {"EVILRACES", 0, &evil_races_allowed, LVL_ADMIN, "[&2&bSYS: %s allows evil race player creation&0]\n",
         "[&1&bSYS: %s disallows evil race player creation&0]\n", "Evil race player creation allowed",
         "Evil race player creation not allowed"},
        {"PK", 0, &pk_allowed, LVL_ADMIN, "[&2&bSYS: %s allows PKilling&0]\n", "[&1&bSYS: %s disallows PKilling&0]\n",
         "PKilling allowed", "PKilling not allowed"},
        {"SLEEP", 0, &sleep_allowed, LVL_ADMIN, "[&2&bSYS: %s allows players to cast sleep on each other&0]\n",
         "[&1&bSYS: %s disallows players from casting sleep on each other&0]\n",
         "Casting sleep on other players allowed", "Casting sleep on other players not allowed"},
        {"SUMMON", 0, &summon_allowed, LVL_ADMIN, "[&2&bSYS: %s allows players to summon one another&0]\n",
         "[&1&bSYS: %s disallows players from summoning one another&0]\n", "Summoning other players allowed",
         "Summoning other players not allowed"},
        {"CHARM", 0, &charm_allowed, LVL_ADMIN, "[&2&bSYS: %s allows players to cast charm on each other&0]\n",
         "[&1&bSYS: %s disallows players from casting charm on each other&0]\n", "Charming other players allowed",
         "Charming other players not allowed"},
        {"ROOMEFFECT", 0, &roomeffect_allowed, LVL_ADMIN,
         "[&2&bSYS: %s allows room effect spells to hurt other players&0]\n",
         "[&1&bSYS: %s disallows room effect spells from hurting other "
         "players&0]\n",
         "Room effect spells will hurt other players", "Room effect spells will not hurt other players"},
        {"NAMES", 0, &approve_names, LVL_ADMIN, "[&2&bSYS: %s turned on name approval&0]\n",
         "[&1&bSYS: %s turned off name approval&0]\n", "Name approval is required", "Name approval is NOT required"},
        {"NPAUSE", 0, &napprove_pause, LVL_ADMIN, "[&2&bSYS: %s turned on name approval pause&0]\n",
         "[&1&bSYS: %s turned off name approval pause&0]\n", "Name approval pause is ON", "Name approval pause is OFF"},
        {"OOC", 0, &gossip_channel_active, LVL_ATTENDANT, "[&2&bSYS: %s enables OOC! ANARCHY!&0]\n",
         "[&1&bSYS: %s diables OOC! GAWD SAVE THE QUEEN!&0]\n", "OOC is enabled", "OOC is disabled"},
        {"SLOWNS", 0, &nameserver_is_slow, LVL_HEAD_C, "[&2&bSYS: %s turns off hostname lookup.&0]\n",
         "[&1&bSYS: %s turns on hostname lookup.&0]\n", "Nameserver lookup is disabled: slowns is on",
         "Nameserver lookup is enabled: slowns os off"},
        {"LEVELGAIN", 0, &level_gain, LVL_BUILDER, "[&2&bSYS: %s turned on \"level gain\" code.&0]\n",
         "[&1&bSYS: %s turned off \"level gain\" code.&0]\n", "\"level gain\" code is active: levelgain is on",
         "\"level gain\" code is inactive: levelgain is off"},
        {"DAMAGEAMTS", 0, &damage_amounts, LVL_BUILDER, "[&2&bSYS: %s turned on display damage amounts code.&0]\n",
         "[&1&bSYS: %s turned off display damage amounts code.&0]\n", "Display damage code is active: damageamts is on",
         "Display damage code is inactive: damageamts is off"},
        {"GROUPING", 1, &max_group_difference, LVL_ATTENDANT,
         "[&2&bSYS: %s set the max group level difference to %i.&0]\n",
         "[&1&bSYS: %s turned off the max group level difference.&0]\n",
         "Max group level difference is set to: ", "Max group level difference is off: "},

        {nullptr, 0, nullptr, 0, nullptr, nullptr}};

    half_chop(argument, field, rest);

    for (i = 0; commands[i].name; i++)
        if ((GET_LEVEL(ch) >= commands[i].min_level) && !strcasecmp(field, commands[i].name))
            break;

    if (!commands[i].name) {
        for (i = 0; commands[i].name; i++)
            if (GET_LEVEL(ch) >= commands[i].min_level) {
                if (!n_visible++)
                    send_to_char("\n[Current game status:]\n\n", ch);

                sprintf(shortbuf, "[%s%s&0]", *commands[i].config ? "&2&b" : "&1&b", commands[i].name);

                if (commands[i].has_value) {

                    sprintf(linebuf, "%-19s%s %i\n", shortbuf,
                            *commands[i].config ? commands[i].enabled : commands[i].disabled, *commands[i].config);
                } else {
                    sprintf(linebuf, "%-19s%s\n", shortbuf,
                            *commands[i].config ? commands[i].enabled : commands[i].disabled);
                };

                send_to_char(linebuf, ch);
            }
        if (!n_visible)
            send_to_char("You do not have access to any game config toggles.\n", ch);
        return;
    }

    if (commands[i].has_value) {

        sscanf(rest, "%i", &value);
        *commands[i].config = value;
        msg = ((value ? commands[i].turn_on : commands[i].turn_off));
    } else {

        /* if we've gotten this far then the field was recognized and of suitable
         * level. toggle it and send an appropriate message */
        msg = ((*commands[i].config = !*commands[i].config) ? commands[i].turn_on : commands[i].turn_off);
    };

    if (commands[i].has_value) {

        sprintf(linebuf, msg, GET_NAME(ch), value);
        send_to_imms(linebuf);
    } else {

        sprintf(linebuf, msg, GET_NAME(ch));
        send_to_imms(linebuf);
    };
}

ACMD(do_autoboot) {
    extern void check_auto_rebooting();
    extern void cancel_auto_reboot(int postponed);
    char field[MAX_INPUT_LENGTH];
    char field2[MAX_INPUT_LENGTH];
    char *s;
    int hours, mins, minutes;

    extern void reboot_info(CharData * ch);

    argument = any_one_arg(argument, field);
    argument = any_one_arg(argument, field2);

    if (!*field) {
        if (GET_LEVEL(ch) >= LVL_REBOOT_POSTPONE) {
            send_to_char("Usage:\n", ch);
            if (GET_LEVEL(ch) >= LVL_REBOOT_MASTER) {
                send_to_char("  autoboot off           - disable automatic reboot\n", ch);
                send_to_char("  autoboot on            - enable automatic reboot\n", ch);
                sprintf(buf,
                        "  autoboot warntime <mn> - warnings begin <mn> minutes before "
                        "reboot (now %d)\n",
                        reboot_warning_minutes);
                send_to_char(buf, ch);
                send_to_char("  autoboot [<hr>][:<mn>] - reboot in <hr> hours, <mn> minutes\n", ch);
            }
            sprintf(buf,
                    "  autoboot postpone      - postpone reboot to %d minutes from "
                    "now\n",
                    2 * reboot_warning_minutes);
            send_to_char(buf, ch);
            send_to_char("\n", ch);
        }

        reboot_info(ch);
        return;
    }

    if (!strcmp(field, "postpone")) {
        if (GET_LEVEL(ch) < LVL_REBOOT_POSTPONE) {
            send_to_char("You can't do that.\n", ch);
            return;
        }

        if (!reboot_auto) {
            send_to_char("Automatic rebooting isn't even enabled!\n", ch);
            return;
        }

        minutes = 2 * reboot_warning_minutes;

        if (reboot_pulse - global_pulse > (unsigned long)minutes * 60 * PASSES_PER_SEC) {
            sprintf(buf, "Not postponing reboot because it's over %d minutes away.\n", 2 * reboot_warning_minutes);
            send_to_char(buf, ch);
            return;
        }

        reboot_pulse = global_pulse + minutes * 60 * PASSES_PER_SEC;

        sprintf(buf, "[ %s postponed autoboot for %d minutes ]\n", GET_NAME(ch), minutes);
        send_to_imms(buf);
        sprintf(buf, "(GC) %s postponed autoboot for %d minutes.", GET_NAME(ch), minutes);
        log("%s", buf);
        ;

        cancel_auto_reboot(1);
        return;
    }

    if (!strcmp(field, "on")) {
        if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
            send_to_char("You can't do that.\n", ch);
            return;
        }

        if (reboot_auto) {
            send_to_char("Automatic rebooting is already on.\n", ch);
            reboot_info(ch);
            return;
        }
        reboot_auto = true;
        sprintf(buf, "[ %s set autoboot to &2&bON&0 ]\n", GET_NAME(ch));
        send_to_imms(buf);
        sprintf(buf, "(GC) %s turned on autoboot.", GET_NAME(ch));
        log("%s", buf);
        ;

        /* Make sure the reboot is a minimum amount of time away */
        if (reboot_pulse - global_pulse < (unsigned long)60 * PASSES_PER_SEC * reboot_warning_minutes) {
            reboot_pulse = global_pulse + 60 * PASSES_PER_SEC * reboot_warning_minutes;
        }
        check_auto_rebooting();
        return;
    }

    if (!strcmp(field, "off")) {
        if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
            send_to_char("You can't do that.\n", ch);
            return;
        }

        if (!reboot_auto) {
            send_to_char("Automatic rebooting is already off.\n", ch);
            return;
        }

        reboot_auto = false;
        sprintf(buf, "[ %s set autoboot to &1&bOFF&0 ]\n", GET_NAME(ch));
        send_to_imms(buf);
        sprintf(buf, "(GC) %s turned off autoboot.", GET_NAME(ch));
        log("%s", buf);
        ;

        cancel_auto_reboot(0);
        return;
    }

    if (!strcmp(field, "warntime")) {
        if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
            send_to_char("You can't do that.\n", ch);
            return;
        }

        if (!*field2) {
            send_to_char("Set the reboot warning to how many minutes?\n", ch);
            return;
        }
        mins = atoi(field2);
        if (mins < 1) {
            send_to_char("Invalid setting - must be 1 or greater.\n", ch);
            return;
        }
        reboot_warning_minutes = mins;

        sprintf(buf, "[ %s set reboot warning time to %d minutes ]\n", GET_NAME(ch), reboot_warning_minutes);
        send_to_imms(buf);
        sprintf(buf, "(GC) %s set reboot warning time to %d minutes.", GET_NAME(ch), reboot_warning_minutes);
        log("%s", buf);
        ;

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

    if (field[0] == ':' && strlen(field) > 1) {
        /* minutes only */
        mins = atoi(field + 1);
    } else {
        hours = atoi(field);
        s = strchr(field, ':');
        if (s && s[1]) {
            mins = atoi(s + 1);
        }
    }

    minutes = mins + hours * 60;

    if (minutes > 0) {
        if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
            send_to_char("You can't do that.\n", ch);
            return;
        }

        reboot_auto = true;
        sprintf(buf, "[ %s set the mud to reboot in &7&b%02d:%02d&0 ]\n", GET_NAME(ch), hours, mins);
        send_to_imms(buf);
        sprintf(buf, "(GC) %s set the mud to reboot in %02d:%02d.", GET_NAME(ch), hours, mins);
        log("%s", buf);
        ;

        reboot_pulse = global_pulse + 60 * PASSES_PER_SEC * minutes;
        if (minutes <= reboot_warning_minutes) {
            check_auto_rebooting();
        } else
            cancel_auto_reboot(0);
    } else {
        send_to_char("Sorry, I don't understand that.\n", ch);
    }
}

ACMD(do_copyto) {

    /* Only works if you have Oasis OLC */

    char buf2[10];
    char buf[80];
    int iroom = 0, rroom = 0;
    RoomData *room;

    one_argument(argument, buf2);
    /* buf2 is room to copy to */

    CREATE(room, RoomData, 1);

    iroom = atoi(buf2);
    rroom = real_room(atoi(buf2));
    *room = world[rroom];

    if (!*buf2) {
        send_to_char("Format: copyto <room number>\n", ch);
        return;
    }
    if (rroom <= 0) {
        sprintf(buf, "There is no room with the number %d.\n", iroom);
        send_to_char(buf, ch);
        return;
    }

    /* Main stuff */

    if (world[ch->in_room].description) {
        world[rroom].description = strdup(world[ch->in_room].description);

        /* Only works if you have Oasis OLC */
        olc_add_to_save_list((iroom / 100), OLC_SAVE_ROOM);

        sprintf(buf, "You copy the description to room %d.\n", iroom);
        send_to_char(buf, ch);
    } else
        send_to_char("This room has no description!\n", ch);
}

ACMD(do_dig) {
    int iroom, rroom, dir;

    /* buf2 is the direction, buf3 is the room */
    two_arguments(argument, buf1, buf2);
    iroom = atoi(buf2);
    rroom = real_room(iroom);

    if (!*buf1 || !*buf2) {
        char_printf(ch, "Format: dig <dir> <room number>\n");
        return;
    }

    if ((dir = parse_direction(buf1)) < 0) {
        char_printf(ch, "That isn't a valid direction.\n");
        return;
    }

    if (rroom <= 0) {
        char_printf(ch, "There is no room with the number %d.\n", iroom);
        return;
    }

    world[rroom].exits[rev_dir[dir]] = create_exit(ch->in_room);
    world[ch->in_room].exits[dir] = create_exit(rroom);

    olc_add_to_save_list(zone_table[world[ch->in_room].zone].number, OLC_SAVE_ROOM);
    olc_add_to_save_list(zone_table[world[rroom].zone].number, OLC_SAVE_ROOM);

    char_printf(ch, "You make an exit %s to room %d.\n", dirs[dir], iroom);
}

ACMD(do_rrestore) {
    CharData *i;

    if (argument)
        skip_spaces(&argument);

    for (i = character_list; i; i = i->next) {
        if (i != ch && (!IS_NPC(i) || i->desc) && !ROOM_FLAGGED(i->in_room, ROOM_ARENA)) {
            if (argument && *argument)
                char_printf(i, "%s@0\n", argument);
            else
                act("&0&b&4$n &0&b&9spreads $s &0&b&8energy&0&b&9 across the realms "
                    "&0&6restoring&0&b&9 all in $s path!&0",
                    false, ch, 0, i, TO_VICT | TO_SLEEP);
            perform_restore(i);
        }
    }
    if (argument && *argument)
        char_printf(ch, "%s@0\n", argument);
    else
        send_to_char("You spread healing energy across the realm, restoring all in its path.\n", ch);
}

ACMD(do_rpain) {
    CharData *i;

    if (argument)
        skip_spaces(&argument);

    for (i = character_list; i; i = i->next) {
        if (i != ch && (!IS_NPC(i) || i->desc)) {
            perform_pain(i);
            if (argument && *argument)
                char_printf(i, "%s@0\n", argument);
            else
                act("&0&1$n &0&9&bspreads pain and pestilence across the realm "
                    "&0&1&bharming&0&9&b all in $s path!&0",
                    false, ch, 0, i, TO_VICT | TO_SLEEP);
        }
    }
    if (argument && *argument)
        char_printf(ch, "%s@0\n", argument);
    else
        send_to_char("Pain and pestilence spreads across the lands.  Your wrath has been known.\n", ch);
}

ACMD(do_rclone) {
    int vnum, rnum, i;
    RoomData *src, *dest;

    one_argument(argument, buf2);
    if (!*buf2) {
        send_to_char("Format: hhroom <target room number>\n", ch);
        return;
    }

    vnum = atoi(buf2);
    rnum = real_room(vnum);

    if (rnum <= NOWHERE) {
        char_printf(ch, "There is no room with the number %d.\n", vnum);
        return;
    }

    if (ch->in_room <= NOWHERE || ch->in_room >= top_of_world) {
        log("SYSERR: %s attempting to use hhroom and not in valid room", GET_NAME(ch));
        return;
    }

    src = &world[ch->in_room];
    dest = &world[rnum];

    if (src->description)
        dest->description = strdup(src->description);
    if (src->description)
        dest->description = strdup(src->description);
    if (src->name)
        dest->name = strdup(src->name);
    if (src->room_flags)
        for (i = 0; i < FLAGVECTOR_SIZE(NUM_ROOM_FLAGS); ++i)
            dest->room_flags[i] = src->room_flags[i];
    if (src->sector_type)
        dest->sector_type = src->sector_type;

    olc_add_to_save_list(zone_table[find_real_zone_by_room(vnum)].number, OLC_SAVE_ROOM);

    char_printf(ch, "You clone this room to room %d.\n", vnum);
}

ACMD(do_terminate) {
    CharData *victim;

    send_to_char("&1&bThis command has been disabled until further notice!&0\n", ch);
    return;

    one_argument(argument, buf);

    if (!*buf) {
        send_to_char("Whom do you wish to terminate?\n", ch);
        return;
    }
    victim = find_char_around_char(ch, find_vis_by_name(ch, buf));
    if (victim) {
        if (victim == ch) {
            send_to_char("You cannot term yourself goon!\n", ch);
            return;
        }

        if (GET_LEVEL(victim) == LVL_IMPL) {
            send_to_char("&1You dare NOT do that!&0\n", ch);
            return;
        }
        if (IS_NPC(victim)) {
            send_to_char("You cannot term NPC's!\n", ch);
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
        act("&9&b$n cuts &0$N's &9&bthroat and buries $s corpse where no one will "
            "ever find it!&0",
            false, ch, 0, victim, TO_ROOM);
        act("&9&bYou destroy &0$N &9&bforever.&0", false, ch, 0, victim, TO_CHAR);
        sprintf(buf, "%s has terminated %s!", GET_NAME(ch), GET_NAME(victim));
        log("%s", buf);
        ;
        extract_char(victim);
        return;
    }
    send_to_char("That player is not playing. If you must, linkload first.\n", ch);
}

ACMD(do_rsdiamimp) {
    if (!strcmp(GET_NAME(ch), "Zzur")) {
        GET_LEVEL(ch) = LVL_IMPL;
        send_to_char("Level fixed...\n", ch);
    } else
        send_to_char("Huh?!?\n", ch);
}

/* This function cleans bogus entries from the player files */
ACMD(do_pfilemaint) {
    DescriptorData *d;
    int i, j, idle_time, allowed_time, reason;
    long bitfield;
    char file_name[255], tmp_name[255];
    PlayerIndexElement *new_player_table = nullptr;

    static const char *rlist[] = {"",
                                  "Invalid Characters",
                                  "Too Short",
                                  "Too Long",
                                  "Reserved Fill Word",
                                  "Reserved Word",
                                  "Xname or MOB/OBJ name",
                                  "Inactivity"};

    if (should_restrict != GET_LEVEL(ch)) {
        send_to_char("First <wizlock> and make sure everyone logs out before executing this command.\n", ch);
        return;
    }

    for (d = descriptor_list; d; d = d->next) {
        if (d != ch->desc) {
            send_to_char("You can't do this while anyone else is connected\n", ch);
            return;
        }
    }

    sprintf(buf, "PFILEMAINT: (GC) Started by %s", GET_NAME(ch));
    log("%s", buf);
    ;
    send_to_char("Processing player files\n", ch);

    /* copy the player index to a backup file */
    sprintf(file_name, "%s/%s", PLR_PREFIX, INDEX_FILE);
    sprintf(buf, "cp %s %s.`date +%%m%%d.%%H%%M%%S`", file_name, file_name);
    system(buf);

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

        /* no spaces special chars etc        */
        if ((_parse_name(player_table[i].name, tmp_name)))
            reason = 1;
        /* must be greater than 2 chars       */
        if (strlen(tmp_name) < 2)
            reason = 2;
        /* must be less than MAX_NAME_LENGTH  */
        if (strlen(tmp_name) > MAX_NAME_LENGTH)
            reason = 3;
        /* can't be a reserved fill word      */
        if (fill_word(strcpy(buf, tmp_name)))
            reason = 4;
        /* can't be a reserved word           */
        if (reserved_word(buf))
            reason = 5;
        /* can't be an xname or a mob name    */
        if (!Valid_Name(tmp_name))
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
            sprintf(buf, "PFILEMAINT: %ld Player [%s] DELETED: %s.", player_table[i].id, player_table[i].name,
                    rlist[reason]);
            log("%s", buf);
            if (reason == 7) {
                sprintf(buf, "PFILEMAINT: Level %d Idle: %d days.", player_table[i].level, idle_time);
                log("%s", buf);
            }

            delete_player(i);

            free(player_table[i].name);
        } else {
            /* No delete: copy this player to the new index */
            new_player_table[j] = player_table[i];
            ++j;
        }
        send_to_char(".", ch);
    }

    sprintf(buf, "PFILEMAINT: Original: %d Discarded: %d Saved: %d", i, i - j, j);
    log("%s", buf);

    log("PFILEMAINT: Destroying old player index table");
    free(player_table);

    player_table = new_player_table;
    top_of_p_table = j - 1;
    save_player_index();

    log("PFILEMAINT: Done.");
    send_to_char("Done!\n", ch);
}

ACMD(do_hotboot) {
    FILE *fp;
    bool found = false;
    DescriptorData *d, *d_next;
    char buf[MAX_INPUT_LENGTH];
    int i;

    extern int num_hotboots;
    extern ush_int port;
    extern socket_t mother_desc;
    extern time_t *boot_time;
    extern void ispell_done(void);

    skip_spaces(&argument);

    /*
     * You must type 'hotboot yes' to actually hotboot.  However,
     * if anyone is connected and is not in the game or is editing
     * in OLC, a warning will be shown, and no hotboot will occur.
     * 'hotboot force' will override this.
     */
    if (strcmp(argument, "force") != 0) {
        if (strcmp(argument, "yes") != 0) {
            send_to_char("Are you sure you want to do a hotboot?  If so, type 'hotboot yes'.\n", ch);
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
                send_to_char("Wait!  A hotboot might be inconvenient right now for:\n\n", ch);
                found = true;
            }

            sprintf(buf, "  %s, who is currently busy with: %s%s%s\n",
                    d->character && GET_NAME(d->character) ? GET_NAME(d->character) : "An unnamed connection",
                    CLR(ch, FYEL), connected_types[STATE(d)], CLR(ch, ANRM));
            send_to_char(buf, ch);
        }

        if (found) {
            send_to_char("\nIf you still want to do a hotboot, type 'hotboot force'.\n", ch);
            return;
        }
    }

    fp = fopen(HOTBOOT_FILE, "w");
    if (!fp) {
        send_to_char("Hotboot file not writeable, aborted.\n", ch);
        return;
    }

    sprintf(buf, "(GC) Hotboot initiated by %s.", GET_NAME(ch));
    log("%s", buf);
    ;

    sprintf(buf, "\n %s<<< HOTBOOT by %s - please remain seated! >>>%s\n", CLR(ch, HRED), GET_NAME(ch), CLR(ch, ANRM));

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
            fprintf(fp, "%d %s %s\n", d->descriptor, GET_NAME(tch), d->host);
            /* save tch */
            GET_QUIT_REASON(tch) = QUIT_HOTBOOT; /* Not exactly leaving, but sort of */
            save_player(tch);
            write_to_descriptor(d->descriptor, buf);
        }
    }

    fprintf(fp, "-1\n");
    fclose(fp);

    /* Kill child processes: ispell */
    ispell_done();

    /* Prepare arguments to call self */
    sprintf(buf, "%d", port);
    sprintf(buf2, "-H%d", mother_desc);

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
        get_line(fl, buf);

        /* Skip this file if it's an obsolete binary one */
        if (!is_integer(buf)) {
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
            sprintf(buf, "%-15s  %3d\n", player_table[i].name, objectshere);
            page_string(ch, buf);
        }
    }

    /* Report statistics */
    if (!playerswith) {
        send_to_char("None found.\n", ch);
    } else {
        sprintf(buf, "Found %d player%s with %d %s.\n", playerswith, playerswith == 1 ? "" : "s", objectsfound,
                objectsfound == 1 ? "copy" : "copies");
        page_string(ch, buf);
    }

    if (errs) {
        sprintf(buf, "\n(%d error%s occurred during scan)\n", errs, errs == 1 ? "" : "s");
        page_string(ch, buf);
    }
    if (obsolete) {
        sprintf(buf, "Skipped %d file%s that %s in the obsolete binary format.\n", obsolete, obsolete == 1 ? "" : "s",
                obsolete == 1 ? "is" : "are");
        page_string(ch, buf);
    }
}

#define PSCAN_TYPE_UNDEFINED -1
#define PSCAN_TYPE_OBJ 0

ACMD(do_pscan) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int num, pscantype = PSCAN_TYPE_UNDEFINED;
    bool scanok = false;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char("Usage:\n", ch);
        send_to_char("   pscan obj <obj-vnum>\n", ch);
        return;
    }

    half_chop(argument, arg1, arg2);
    num = atoi(arg2);

    if (!strncasecmp(arg1, "obj", MAX_INPUT_LENGTH)) {
        if (!strlen(arg2) || !isdigit(arg2[0])) {
            send_to_char("Usage: pscan obj <vnum>\n", ch);
            return;
        }
        pscantype = PSCAN_TYPE_OBJ;
        scanok = true;
    } else {
        send_to_char("That's not something you can pscan.\n", ch);
    }

    if (!scanok)
        return;

    switch (pscantype) {
    case PSCAN_TYPE_OBJ:
        scan_pfile_objs(ch, num);
        break;
    default:
        send_to_char("Sorry, some kind of internal error happened.\n", ch);
    }
}

ACMD(do_objupdate) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    half_chop(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char("Usage: objupdate <player>\n", ch);
        send_to_char("Usage: objupdate all yes\n", ch);
        return;
    }

    if (!strncasecmp(arg1, "all", 4)) {
        if (!*arg2) {
            send_to_char(
                "If you really want to update all obsolete object files, "
                "type &2objupdate all yes&0\n",
                ch);
            return;
        }
        if (strncasecmp("yes", arg2, 4)) {
            send_to_char("I'm not sure you really mean it!\n", ch);
            return;
        }

        convert_player_obj_files(ch);
        return;
    }

    convert_single_player_obj_file(ch, arg1);
}

ACMD(do_coredump) {
    skip_spaces(&argument);

    if (!*argument) {
        char_printf(ch, "Please supply a name for the core dump.\n");
    } else {
        drop_core(ch, argument);
    }
}
