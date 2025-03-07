/***************************************************************************
 *   File: prefs.c                                        Part of FieryMUD *
 *  Usage: Player preferences management and commands                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "clan.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#define TOG_OFF 0
#define TOG_ON 1

ACMD(do_toggle) {
    int i = 0, column, wimp_lev, page_length;
    bool set;
    long result = 0;
    CharData *tch;

    struct set_struct {
        const std::string_view cmd;
        int level;
        int bitvector;
    }
    /**********************
      These match SCMD defines in interpreter.h, to add or
      remove a toggle you must also update the SCMD's in
      interpreter.h.
    *********************/
    fields[] = {
        /* 00 */ {"NoSummon", LVL_IMMORT, PRF_SUMMONABLE},
        /* 01 */ {"NoHassle", LVL_IMMORT, PRF_NOHASSLE},
        /* 02 */ {"Brief", 0, PRF_BRIEF},
        /* 03 */ {"Compact", 0, PRF_COMPACT},
        /* 04 */ {"NoTell", 0, PRF_NOTELL},
        /* 05 */ {"AFK", 0, PRF_AFK},
        /* 06 */ {"NoShout", 0, PRF_DEAF},
        /* 07 */ {"NoGossip", 0, PRF_NOGOSS},
        /* 08 */ {"NoHints", 0, PRF_NOHINTS},
        /* 09 */ {"NoWiznet", LVL_IMMORT, PRF_NOWIZ},
        /* 10 */ {"Quest", LVL_IMMORT, PRF_QUEST},
        /* 11 */ {"RoomFlags", LVL_IMMORT, PRF_ROOMFLAGS},
        /* 12 */ {"NoRepeat", 0, PRF_NOREPEAT},
        /* 13 */ {"Holylight", LVL_IMMORT, PRF_HOLYLIGHT},
        /* 14 */ {"Autoexit", 0, PRF_AUTOEXIT},
        /* 15 */ {"NoPetition", LVL_IMMORT, PRF_NOPETI},
        /* 16 */ {"AutoSplit", 0, PRF_AUTOSPLIT},
        /* 17 */ {"Anonymous", 50, PRF_ANON},
        /* 18 */ {"ShowVnums", LVL_IMMORT, PRF_SHOWVNUMS},
        /* 19 */ {"Wimpy", 0, 0},
        /* 20 */ {"NiceArea", 0, PRF_NICEAREA},
        /* 21 */ {"Vicious", 0, PRF_VICIOUS},
        /* 22 */ {"Passive", 0, PRF_PASSIVE},
        /* 23 */ {"PageLength", 0, 0},
        /* 24 */ {"NoFollow", 0, PRF_NOFOLLOW},
        /* 25 */ {"RoomVis", LVL_IMMORT, PRF_ROOMVIS},
        /* 26 */ {"NoClanComm", 0, PRF_NOCLANCOMM},
        /* 27 */ {"OLCComm", LVL_IMMORT, PRF_OLCCOMM},
        /* 28 */ {"LineNums", 0, PRF_LINENUMS},
        /* 29 */ {"AutoLoot", 0, PRF_AUTOLOOT},
        /* 30 */ {"AutoTreas", 0, PRF_AUTOTREAS},
        /* 31 */ {"AutoInvis", LVL_IMMORT, 0},
        /* 32 */ {"ExpandObjs", 0, PRF_EXPAND_OBJS},
        /* 33 */ {"ExpandMobs", 0, PRF_EXPAND_MOBS},
        /* 34 */ {"Sacrificial", LVL_IMMORT, PRF_SACRIFICIAL},
        /* 35 */ {"PetAssist", 0, PRF_PETASSIST},
        /* 36 */ {"\n", 0, 0},
        /* If you add another toggle, add a corresponding SCMD_ define in
         * interpreter.h, even if you don't intend to use it. */

    };
    const std::string_view tog_messages[][2] = {
        /*00 */ {"You are now safe from summoning by other players.\n", "You may now be summoned by other players.\n"},
        /*01 */
        {"Nohassle disabled, mobs will attack you now.\n", "Nohassle enabled, mobs will leave you alone now.\n"},
        /*02 */ {"Brief mode off.\n", "Brief mode on.\n"},
        /*03 */ {"Compact mode off.\n", "Compact mode on.\n"},
        /*04 */ {"You can now hear tells.\n", "You are now deaf to tells.\n"},
        /*05 */ {"You are no longer AFK.\n", "You are now AFK.\n"},
        /*06 */
        {"You can now hear shouts.\n", "You are now deaf to shouts.\n"},
        /*07 */
        {"You can now hear gossip.\n", "You are now deaf to gossip.\n"},
        /*08 */
        {"You will receive hints when you enter typos.\n", "You will not receive any hints when entering typos.\n"},
        /*09 */
        {"You can now hear the Wiz-channel.\n", "You are now deaf to the Wiz-channel.\n"},
        /*10 */
        {"You are no longer part of the Quest.\n", "Okay, you are part of the Quest!\n"},
        /*11 */
        {"You will no longer see the room flags.\n", "You will now see the room flags.\n"},
        /*12 */
        {"You will now have your communication repeated.\n", "You will no longer have your communication repeated.\n"},
        /*13 */ {"HolyLight mode off.\n", "HolyLight mode on.\n"},
        /*14 */ {"Autoexits disabled.\n", "Autoexits enabled.\n"},
        /*15 */
        {"You will now receive petitions from mortals!\n", "You are now deaf to petitions!\n"},
        /*16 */
        {"You will no longer automatically split coins from corpses.\n",
         "You will now automatically split coins from corpses!\n"},
        /*17 */
        {"You are no longer anonymous.\n", "You are now anonymous.\n"},
        /*18 */
        {"You will no longer see vnums.\n", "You will now see vnums.\n"},
        /*19 */ {{}, {}},
        /*20 */
        {"Your area spells will now hit your race align in towns.\n",
         "Your area spells won't hit your race align in towns.\n"},
        /*21 */
        {"You feel nice and no longer vicious.\n", "You will now kill mortally wounded victims.\n"},
        /*22 */
        {"You will now auto-engage upon being offensively cast upon.\n",
         "You will no longer auto-engage upon being offensively cast upon.\n"},
        /*23 */ {{}, {}},
        /*24 */
        {"You will now let anyone follow you.\n", "You will now avoid attracting new followers.\n"},
        /*25 */
        {"You will now only be seen by people who might normally see you.\n",
         "You will now be seen by anyone in the same room as you.\n"},
        /*26 */
        {"You will now hear clan communication.\n", "You will no longer hear clan communication.\n"},
        /*27 */
        {"You will no longer hear communication while in OLC.\n", "You will now hear communication while in OLC.\n"},
        /*28 */
        {"Line numbers will not be displayed when entering the string "
         "editor.\n",
         "Line numbers will be displayed when entering the string editor.\n"},
        /*29 */
        {"You will no longer automatically loot items from corpses.\n",
         "You will now automatically loot items from corpses.\n"},
        /*30 */
        {"You will no longer automatically loot treasure from corpses.\n",
         "You will now automatically loot treasure from corpses.\n"},
        /*31 */ {{}, {}},
        /*32 */
        {"Objects will now stack in lists.\n", "Objects will no longer stack in lists.\n"},
        /*33 */
        {"Mobiles will now stack in lists.\n", "Mobiles will no longer stack in lists.\n"},
        /*34 */
        {"When you cast sacrificial spells, they may now auto-target you.\n",
         "When you cast sacrificial spells, they will no longer auto-target "
         "you.\n"},
        /*34 */
        {"Your pet will no longer assist you as you fight.\n", "Your pet will now assist you as you fight.\n"},
    };

    auto arg = argument.shift();
    tch = REAL_CHAR(ch);

    /*
     * First, see if the player wants to toggle something.
     */
    if (!arg.empty())
        for (; *fields[i].cmd != '\n'; ++i)
            if (matches_start(arg, fields[i].cmd))
                if (GET_LEVEL(tch) >= fields[i].level || (i == SCMD_ANON && PRV_FLAGGED(tch, PRV_ANON_TOGGLE)))
                    if (i != SCMD_NOCLANCOMM || GET_CLAN(tch))
                        break;

    if (arg.empty() || *fields[i].cmd == '\n') {
        /* Show a player his/her fields. */

        if (!arg.empty()) {
            if (GET_LEVEL(ch) < LVL_GOD || !(tch = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
                char_printf(ch, "Toggle what!?\n");
                return;
            }
            /* Handle switched/shapechanged players. */
            tch = REAL_CHAR(tch);
        }

        if (IS_NPC(tch)) {
            act("$N is an NPC.  They don't have toggles!", false, ch, 0, tch, TO_CHAR);
            return;
        }

        strcpy(buf,
               "             FieryMUD TOGGLES!  (See HELP TOGGLE)\n"
               "===============================================================\n");
        for (column = i = 0; *fields[i].cmd != '\n'; ++i) {
            if (i != SCMD_ANON || !PRV_FLAGGED(tch, PRV_ANON_TOGGLE))
                if (fields[i].level > GET_LEVEL(tch))
                    continue;
            if (i == SCMD_NOCLANCOMM && !GET_CLAN(tch))
                continue;

            set = false;
            switch (i) {
            case SCMD_WIMPY:
                if ((set = (1 && GET_WIMP_LEV(tch))))
                    sprintf(buf2, "%d", GET_WIMP_LEV(tch));
                else
                    strcpy(buf2, "NO");
                break;
            case SCMD_PAGELENGTH:
                if (GET_PAGE_LENGTH(tch) == 0)
                    strcpy(buf2, "NONE");
                else
                    sprintf(buf2, "%d", GET_PAGE_LENGTH(tch));
                set = GET_PAGE_LENGTH(tch);
                break;
            case SCMD_AUTOINVIS:
                if (GET_AUTOINVIS(tch) == -1) {
                    strcpy(buf2, "NO");
                } else {
                    set = true;
                    sprintf(buf2, "%d", GET_AUTOINVIS(tch));
                }
                break;
            default:
                set = 1 && PRF_FLAGGED(tch, fields[i].bitvector);
                strcpy(buf2, YESNO(set));
                break;
            }
            sprintf(buf, "%s %s%11s  %5s&0 %s", buf, set ? QHWHT : QWHT, fields[i].cmd, buf2,
                    column == 2 ? "\n" : "| ");
            if (++column >= 3)
                column = 0;
        }
        if (column)
            strcat(buf, "\n");
        strcat(buf, "===============================================================\n");
        char_printf(ch, buf);
        return;
    }

    if (IS_NPC(tch))
        return;

    switch (i) {
    case SCMD_WIMPY:
        arg = argument.shift();
        if (arg.empty()) {
            if (GET_WIMP_LEV(tch)) {
                char_printf(ch, "Your current wimp level is {:d} hit points.\n", GET_WIMP_LEV(tch));
            } else
                char_printf(ch, "At the moment, you're not a wimp.  (sure, sure...)\n");
        } else if (isdigit(*arg)) {
            if ((wimp_lev = atoi(arg))) {
                if (wimp_lev < 0)
                    char_printf(ch, "Heh, heh, heh.. we are jolly funny today, eh?\n");
                else if (wimp_lev > GET_MAX_HIT(tch))
                    char_printf(ch, "That doesn't make much sense, now does it?\n");
                else {
                    char_printf(tch, "Okay, you'll wimp out if you drop below {:d} hit points.\n", wimp_lev);
                    GET_WIMP_LEV(tch) = wimp_lev;
                }
            } else {
                char_printf(ch, "Okay, you'll now tough out fights to the bitter end.\n");
                GET_WIMP_LEV(tch) = 0;
            }
        } else
            char_printf(ch, "Specify at how many hit points you want to wimp out at.  (0 to disable)\n");
        return;
    case SCMD_PAGELENGTH:
        arg = argument.shift();
        if (arg.empty()) {
            if (GET_PAGE_LENGTH(tch) == 0) {
                char_printf(ch, "Pagelength is turned off.\n");
            } else if (GET_PAGE_LENGTH(tch) > 0) {
                char_printf(ch, "Your current page length is {:d}.\n", GET_PAGE_LENGTH(tch));
            } else {
                char_printf(ch, "Your page length is not valid.  Reset to off.\n");
                GET_PAGE_LENGTH(tch) = 0;
            }
        } else if (isdigit(*arg)) {
            page_length = atoi(arg);
            if (page_length == 0) {
                char_printf(ch, "Pagelength turned off.\n");
                GET_PAGE_LENGTH(tch) = 0;
            } else if (page_length < 1)
                char_printf(ch, "Invalid page length.\n");
            else {
                char_printf(ch, "Your new page length is {:d} lines.\n", page_length);
                GET_PAGE_LENGTH(tch) = page_length;
            }
        } else
            char_printf(ch, "Specify at how many lines you want your page length to be.  (0 to turn off.z`)\n");
        return;
    case SCMD_AUTOINVIS:
        any_one_arg(argument, arg);
        if (arg.empty()) {
            if (GET_AUTOINVIS(tch) == -1)
                char_printf(ch, "Autoinvis is off.\n");
            else if (GET_AUTOINVIS(tch) < -1 || GET_AUTOINVIS(tch) > GET_LEVEL(tch)) {
                char_printf(ch, "Your autoinvis is an invalid value: {:d}\n", GET_AUTOINVIS(tch));
            } else if (GET_AUTOINVIS(tch) == GET_LEVEL(tch)) {
                char_printf(ch, "Autoinvis is maxxed to &5&b{:d}&0.\n", GET_AUTOINVIS(tch));
            } else {
                char_printf(ch, "Autoinvis is set to &6&b{:d}&0.\n", GET_AUTOINVIS(tch));
            }
        } else {
            if (isdigit(*arg) || *arg == '-')
                i = atoi(arg);
            else if (matches(arg, "off"))
                i = -1;
            else if (matches(arg, "on"))
                i = GET_LEVEL(tch);
            else {
                char_printf(ch, "Invalid input: autoinvis is a number between 0 and your level.\n");
                char_printf(ch, "You may also enter -1 or 'off' to disable it, or 'on' which will\n");
                char_printf(ch, "set it to the maximum value: your level.\n");
                return;
            }

            if (GET_AUTOINVIS(tch) == i) {
                if (i == -1)
                    char_printf(ch, "Your autoinvis is already off.\n");
                else if (i == GET_LEVEL(tch)) {
                    char_printf(ch, "Your autoinvis is already maxxed to {}.\n", GET_AUTOINVIS(tch));
                } else {
                    char_printf(ch, "Your autoinvis is already {}.\n", i);
                }
            } else if (i < -1 || i > GET_LEVEL(tch)) {
                char_printf(ch, "Invalid input: autoinvis is a number between -1 and your level.\n");
            } else {
                GET_AUTOINVIS(tch) = i;
                if (i == -1)
                    char_printf(ch, "Autoinvis off.\n");
                else {
                    char_printf(ch, "Your autoinvis is set to {:d}.\n", i);
                }
            }
        }
        return;
    default:
        result = PRF_TOG_CHK(tch, fields[i].bitvector);
        break;
    }

    if (result)
        char_printf(ch, tog_messages[i][TOG_ON]);
    else
        char_printf(ch, tog_messages[i][TOG_OFF]);
}
