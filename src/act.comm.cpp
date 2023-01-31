/***************************************************************************
 *   File: act.comm.c                                     Part of FieryMUD *
 *  Usage: Player-level communication commands                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "act.hpp"

#include "board.hpp"
#include "clan.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "editor.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "modify.hpp"
#include "regen.hpp"
#include "retain_comms.hpp"
#include "screen.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* extern variables */
void garble_text(char *string, int percent) {
    char letters[] = "aeiousthpwxyz";
    int i, len = strlen(string);

    for (i = 0; i < len; ++i)
        if (isalpha(string[i]) && random_number(0, 1) && random_number(0, 100) > percent)
            string[i] = letters[random_number(0, 12)];
}

//  Drunk structure and releated code to slur a players speech if they are drunk. Zantir 3/23/01
std::string drunken_speech(std::string speech, int drunkenness) {
    const struct {
        int min_drunk_level;
        int replacement_count;
        const char *replacements[10];
    } drunk_letters['z' - 'a' + 1] = {/* # of letters in alphabet */
                                      {3, 9, {"a", "a", "A", "aa", "ah", "Ah", "ao", "aw", "ahhhh"}},
                                      {8, 5, {"b", "b", "B", "B", "vb"}},
                                      {3, 5, {"c", "C", "cj", "sj", "zj"}},
                                      {5, 2, {"d", "D"}},
                                      {3, 3, {"e", "eh", "E"}},
                                      {4, 5, {"f", "ff", "fff", "fFf", "F"}},
                                      {8, 2, {"g", "G"}},
                                      {9, 6, {"h", "hh", "hhh", "Hhh", "HhH", "H"}},
                                      {7, 6, {"i", "Iii", "ii", "iI", "Ii", "I"}},
                                      {9, 5, {"j", "jj", "Jj", "jJ", "J"}},
                                      {7, 2, {"k", "K"}},
                                      {3, 2, {"l", "L"}},
                                      {5, 8, {"m", "mm", "mmm", "mmmm", "mmmmm", "MmM", "mM", "M"}},
                                      {6, 6, {"n", "nn", "Nn", "nnn", "nNn", "N"}},
                                      {3, 6, {"o", "ooo", "ao", "aOoo", "Ooo", "ooOo"}},
                                      {3, 2, {"p", "P"}},
                                      {5, 5, {"q", "Q", "ku", "ququ", "kukeleku"}},
                                      {4, 2, {"r", "R"}},
                                      {2, 5, {"ss", "zzZzssZ", "ZSssS", "sSzzsss", "sSss"}},
                                      {5, 2, {"t", "T"}},
                                      {3, 6, {"u", "uh", "Uh", "Uhuhhuh", "uhU", "uhhu"}},
                                      {4, 2, {"v", "V"}},
                                      {4, 2, {"w", "W"}},
                                      {5, 6, {"x", "X", "ks", "iks", "kz", "xz"}},
                                      {3, 2, {"y", "Y"}},
                                      {2, 8, {"z", "ZzzZz", "Zzz", "szz", "sZZz", "ZSz", "zZ", "Z"}}};

    if (drunkenness <= 0)
        return speech;

    std::string drunkbuf;
    for (auto ch : speech) {
        if (isdigit(ch))
            drunkbuf += '0' + random_number(0, 9);
        else if (isalpha(ch)) {
            char temp = toupper(ch) - 'A';
            if (drunkenness > drunk_letters[temp].min_drunk_level) {
                drunkbuf += drunk_letters[temp].replacements[random_number(0, drunk_letters[temp].replacement_count - 1)];
            } else {
                drunkbuf += ch;
            }
        } else
            drunkbuf += ch;
    }
    return drunkbuf;
}

void afk_message(CharData *ch, CharData *vict) {
    if (PRF_FLAGGED(vict, PRF_AFK)) {
        act("$N is AFK right now, but received your message.", true, ch, nullptr, vict, TO_CHAR);
        char_printf(vict, "You received the previous message while AFK.\n");
    }
}

ACMD(do_desc) {
    int maxlen, maxlines;

    if (IS_NPC(ch)) {
        char_printf(ch, "You can't change your description, silly-head!\n");
        return;
    }

    maxlen = GET_LEVEL(ch) < LVL_IMMORT ? PLAYER_DESC_LENGTH : IMMORT_DESC_LENGTH;
    maxlines = GET_LEVEL(ch) < LVL_IMMORT ? PLAYER_DESC_LINES : IMMORT_DESC_LINES;

    if (GET_STANCE(ch) > STANCE_SLEEPING)
        act("$n appears rather introspective.", true, ch, nullptr, nullptr, TO_ROOM);

    editor_init(ch->desc, &ch->player.description, maxlen);
    editor_set_begin_string(
        ch->desc, "Enter the text you'd like others to see when they look at you (limit %d lines).\n", maxlines);
    editor_set_max_lines(ch->desc, maxlines);
}

ACMD(do_say) {
    std::string speech = delete_doubledollar(argument);
    speech = trim(speech);

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        char_printf(ch, "You lips move, but no sound forms.\n");
        return;
    }
    if (speech.empty()) {
        char_printf(ch, "Yes, but WHAT do you want to say?\n");
        return;
    }

    if (!speech_ok(ch, 0))
        return;

    if (GET_LEVEL(REAL_CHAR(ch)) < LVL_IMMORT)
        speech = strip_ansi(speech);

    speech = drunken_speech(speech, GET_COND(ch, DRUNK));
    act("You say, '$T@0'", false, ch, nullptr, speech.c_str(), TO_CHAR | TO_OLC);
    act("$n says, '$T@0'", false, ch, nullptr, speech.c_str(), TO_ROOM | TO_OLC);

    /* trigger check */
    speech_mtrigger(ch, speech.c_str());
    speech_wtrigger(ch, speech.c_str());
}

ACMD(do_gsay) {
    CharData *k;
    GroupType *f;
    std::string speech = std::string(trim(argument));

    if (!ch->group_master && !ch->groupees) {
        char_printf(ch, "But you are not the member of a group!\n");
        return;
    }
    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        char_printf(ch, "You lips move, but no sound forms.\n");
        return;
    }

    if (speech.empty()) {
        char_printf(ch, "Yes, but WHAT do you want to group-say?\n");
        return;
    }

    if (GET_LEVEL(REAL_CHAR(ch)) < LVL_IMMORT)
        speech = strip_ansi(speech);

    speech = drunken_speech(speech, GET_COND(ch, DRUNK));
    if (ch->group_master)
        k = ch->group_master;
    else
        k = ch;

    sprintf(buf, "@g$n@g tells the group, '&0%s@g'@0", argument);
    if (k != ch)
        act(buf, false, ch, nullptr, k, TO_VICT | TO_SLEEP | TO_OLC);
    for (f = k->groupees; f; f = f->next)
        if (f->groupee != ch)
            act(buf, false, ch, nullptr, f->groupee, TO_VICT | TO_SLEEP | TO_OLC);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        char_printf(ch, OK);
    else {
        sprintf(buf, "@gYou group say, '&0%s@g'@0", argument);
        act(buf, false, ch, nullptr, nullptr, TO_CHAR | TO_SLEEP | TO_OLC);
    }
}

static void perform_tell(CharData *ch, CharData *vict, std::string arg) {
    std::string tell{arg};
    if (GET_LEVEL(REAL_CHAR(ch)) < LVL_IMMORT)
        tell = strip_ansi(tell);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        char_printf(ch, OK);
    else {
        sprintf(buf, "@WYou tell $N@W, '%s@W'@0", tell.c_str());
        act(buf, false, ch, nullptr, vict, TO_CHAR | TO_SLEEP | TO_OLC);
    }

    if (vict->forward && !vict->desc)
        vict = vict->forward;

    sprintf(buf, "@W$n@W tells you, '%s@W'@0", tell.c_str());
    act(buf, false, REAL_CHAR(ch), nullptr, vict, TO_VICT | TO_SLEEP | TO_OLC);

    /* No need to reply to mobs.  Doesn't matter since we use the IDNUM which is always 0 for mobs. */
    if (!IS_MOB(ch))
        GET_LAST_TELL(vict) = GET_IDNUM(REAL_CHAR(ch));

    afk_message(ch, vict);
    if (IS_MOB(vict)) {
        speech_to_mtrigger(ch, vict, tell.c_str());
    } else {
        std::string formatted = format_act(buf, ch, 0, vict, vict);
        add_retained_comms(vict, TYPE_RETAINED_TELLS, formatted.c_str());
    }
}

ACMD(do_tell) {
    CharData *vict;

    half_chop(argument, buf, buf2);

    if (!*buf || !*buf2)
        char_printf(ch, "Who do you wish to tell what??\n");
    else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, buf))))
        char_printf(ch, NOPERSON);
    else if (ch == vict)
        char_printf(ch, "You try to tell yourself something.\n");
    else if (PRF_FLAGGED(ch, PRF_NOTELL))
        char_printf(ch, "You can't tell other people while you have notell on.\n");
    else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) && GET_LEVEL(ch) < LVL_IMMORT)
        char_printf(ch, "The walls seem to absorb your words.\n");
    else if (!IS_NPC(vict) && !vict->desc && (!vict->forward || !vict->forward->desc)) /* linkless */
        act("No one here by that name.", false, ch, nullptr, vict, TO_CHAR | TO_SLEEP | TO_OLC);
    else if (PLR_FLAGGED(vict, PLR_WRITING) && !PRF_FLAGGED(vict, PRF_OLCCOMM))
        act("$E's writing a message right now; try again later.", false, ch, nullptr, vict,
            TO_CHAR | TO_SLEEP | TO_OLC);
    else if (vict->desc && EDITING(vict->desc) && !PRF_FLAGGED(vict, PRF_OLCCOMM))
        act("$E's writing a message right now; try again later.", false, ch, nullptr, vict,
            TO_CHAR | TO_SLEEP | TO_OLC);
    else if (((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF)) &&
             (GET_LEVEL(ch) < LVL_IMMORT || (GET_LEVEL(vict) > LVL_IMMORT && GET_LEVEL(ch) < GET_LEVEL(vict))))
        act("$E can't hear you.", false, ch, nullptr, vict, TO_CHAR | TO_SLEEP | TO_OLC);
    else if (!IS_NPC(REAL_CHAR(vict)) && REAL_CHAR(vict)->player_specials->ignored == REAL_CHAR(ch))
        act("$N is ignoring you at the moment.  No dice.", false, ch, nullptr, REAL_CHAR(vict), TO_CHAR | TO_OLC);
    else {
        if (!speech_ok(ch, 0))
            return;
        perform_tell(ch, vict, drunken_speech(buf2, GET_COND(ch, DRUNK)));
    }
}

ACMD(do_reply) {
    CharData *tch = character_list;

    skip_spaces(&argument);

    if (GET_LAST_TELL(ch) == NOBODY)
        char_printf(ch, "You have no-one to reply to!\n");
    else if (!*argument)
        char_printf(ch, "What is your reply?\n");
    else {
        /*
         * Make sure the person you're replying to is still playing by searching
         * for them.  Note, now last tell is stored as player IDnum instead of
         * a pointer, which is much better because it's safer, plus will still
         * work if someone logs out and back in again.
         */

        while (tch != nullptr && GET_IDNUM(tch) != GET_LAST_TELL(ch))
            tch = tch->next;

        if (tch == nullptr)
            char_printf(ch, "They are no longer playing.\n");
        else if (PRF_FLAGGED(tch, PRF_NOTELL) && GET_LEVEL(ch) < LVL_GOD)
            char_printf(ch, "That person is now not listening to tells.\n");
        else if (speech_ok(ch, 0)) {
            perform_tell(ch, tch, drunken_speech(argument, GET_COND(ch, DRUNK)));
        }
    }
}

ACMD(do_spec_comm) {
    CharData *vict;
    const char *action_sing, *action_plur, *action_others;

    if (subcmd == SCMD_WHISPER) {
        action_sing = "whisper to";
        action_plur = "whispers to";
        action_others = "$n whispers something to $N.";
    } else {
        action_sing = "ask";
        action_plur = "asks";
        action_others = "$n asks $N a question.";
    }

    half_chop(argument, buf, buf2);

    if (!*buf || !*buf2)
        char_printf(ch, "Whom do you want to {}.. and what??\n", action_sing);
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf))))
        char_printf(ch, NOPERSON);
    else if (vict == ch)
        char_printf(ch, "You can't get your mouth close enough to your ear...\n");
    else if (PLR_FLAGGED(vict, PLR_WRITING) && !PRF_FLAGGED(vict, PRF_OLCCOMM))
        act("$E's writing a message right now; try again later.", false, ch, 0, vict, TO_CHAR | TO_SLEEP | TO_OLC);
    else if (vict->desc && EDITING(vict->desc) && !PRF_FLAGGED(vict, PRF_OLCCOMM))
        act("$E's writing a message right now; try again later.", false, ch, 0, vict, TO_CHAR | TO_SLEEP | TO_OLC);
    else {
        if (!speech_ok(ch, 0))
            return;
        sprintf(buf, "$n %s you, '%s@0'", action_plur, drunken_speech(buf2, GET_COND(ch, DRUNK)).c_str());
        act(buf, false, ch, nullptr, vict, TO_VICT | TO_OLC);
        if (PRF_FLAGGED(ch, PRF_NOREPEAT))
            char_printf(ch, OK);
        else {
            sprintf(buf, "You %s %s, '%s@0'", action_sing, GET_NAME(vict), buf2);
            act(buf, false, ch, nullptr, nullptr, TO_CHAR | TO_OLC);
        }
        act(action_others, false, ch, nullptr, vict, TO_NOTVICT);
        afk_message(ch, vict);
        if (IS_MOB(vict))
            speech_to_mtrigger(ch, vict, buf2);
    }
}

#define MAX_NOTE_LENGTH 1000 /* arbitrary */

ACMD(do_write) {
    ObjData *surface, *implement;
    char *surface_name = buf1, *implement_name = buf2;
    CharData *dummy;

    if (!ch->desc)
        return;

    argument = one_argument(argument, surface_name);

    if (!*surface_name) {
        char_printf(ch, "Write?  With what?  On what?  What are you trying to do?\n");
        return;
    }

    generic_find(surface_name, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &surface);

    if (!surface) {
        char_printf(ch, "You can't find a {} to write on.\n", surface_name);
        return;
    }

    if (GET_OBJ_TYPE(surface) == ITEM_BOARD) {
        skip_spaces(&argument);
        write_message(ch, board(GET_OBJ_VAL(surface, VAL_BOARD_NUMBER)), argument);
        return;
    }

    one_argument(argument, implement_name);

    if (*implement_name) {
        generic_find(implement_name, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &dummy, &implement);
        if (!implement) {
            char_printf(ch, "You can't find a {} to write with.\n", implement_name);
            return;
        }
    } else if (GET_OBJ_TYPE(surface) == ITEM_NOTE) {
        /* No writing implement specified, see if the player has one */
        if (GET_EQ(ch, WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_PEN)
            implement = GET_EQ(ch, WEAR_HOLD);
        else if (GET_EQ(ch, WEAR_HOLD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD2)) == ITEM_PEN)
            implement = GET_EQ(ch, WEAR_HOLD2);
        else
            for (implement = ch->carrying; implement; implement = implement->next_content)
                if (GET_OBJ_TYPE(implement) == ITEM_PEN)
                    if (CAN_SEE_OBJ(ch, implement))
                        break;
        if (!implement) {
            char_printf(ch, "You don't have anything to write with.\n");
            return;
        }
    }

    if (GET_OBJ_TYPE(implement) == ITEM_NOTE && GET_OBJ_TYPE(surface) == ITEM_PEN) {
        /* swap */
        ObjData *temp = surface;
        surface = implement;
        implement = temp;
    }

    if (GET_OBJ_TYPE(surface) != ITEM_NOTE) {
        char_printf(ch, "You can't write on {}.\n", surface->short_description);
        return;
    }

    if (GET_OBJ_TYPE(implement) != ITEM_PEN) {
        char_printf(ch, "You can't write with {}.\n", implement->short_description);
        return;
    }

    /* we can write - hooray! */
    char_printf(ch, "Write your note.  (/s saves /h for help)\n");
    string_write(ch->desc, &surface->action_description, MAX_NOTE_LENGTH);
    act("$n begins to jot down a note.", true, ch, nullptr, nullptr, TO_ROOM);
}

ACMD(do_page) {
    DescriptorData *d;
    CharData *vict;

    half_chop(argument, arg, buf2);

    if (IS_NPC(ch))
        char_printf(ch, "Monsters can't page.. go away.\n");
    else if (!*arg)
        char_printf(ch, "Whom do you wish to page?\n");
    else {
        sprintf(buf, "\007\007*%s* %s\n", GET_NAME(ch), buf2);
        if (!strcasecmp(arg, "all")) {
            if (GET_LEVEL(ch) > LVL_GOD) {
                for (d = descriptor_list; d; d = d->next)
                    if (!d->connected && d->character)
                        act(buf, false, ch, nullptr, d->character, TO_VICT);
            } else
                char_printf(ch, "You will never be godly enough to do that!\n");
            return;
        }
        if ((vict = find_char_around_char(ch, find_vis_by_name(ch, arg))) != nullptr) {
            act(buf, false, ch, nullptr, vict, TO_VICT);
            if (PRF_FLAGGED(ch, PRF_NOREPEAT))
                char_printf(ch, OK);
            else
                act(buf, false, ch, nullptr, vict, TO_CHAR);
            return;
        } else
            char_printf(ch, "There is no such person in the game!\n");
    }
}

ACMD(do_order) {
    char name[100], message[256];
    char buf[256];
    bool found = false, anyawake = false;
    int org_room;
    CharData *vict;
    FollowType *k;

    half_chop(argument, name, message);

    if (!*name || !*message)
        char_printf(ch, "Order who to do what?\n");
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, name))) &&
             !is_abbrev(name, "followers"))
        char_printf(ch, "That person isn't here.\n");
    else if (ch == vict)
        char_printf(ch, "You obviously suffer from schizophrenia.\n");
    else {
        /* modified to allow animateds to order - 321 */
        if (EFF_FLAGGED(ch, EFF_CHARM)) {
            char_printf(ch, "Your superior would not approve of you giving orders.\n");
            return;
        }
        if (vict) {
            act("$n gives $N an order.", false, ch, nullptr, vict, TO_ROOM);

            if (GET_STANCE(vict) < STANCE_RESTING) {
                sprintf(buf, "$N is %s and can't hear you.", stance_types[GET_STANCE(vict)]);
                act(buf, false, ch, nullptr, vict, TO_CHAR);
            } else {
                sprintf(buf, "$N orders you to '%s'", message);
                act(buf, false, vict, nullptr, ch, TO_CHAR);
                if ((vict->master != ch) || !EFF_FLAGGED(vict, EFF_CHARM))
                    act("$n has an indifferent look.", false, vict, 0, 0, TO_ROOM);
                else {
                    char_printf(ch, OK);
                    command_interpreter(vict, message);
                    WAIT_STATE(ch, PULSE_VIOLENCE / 2);
                }
            }
        } else { /* This is order "followers" */
            act("$n calls out, '$T'.", false, ch, 0, message, TO_ROOM);
            org_room = ch->in_room;
            for (k = ch->followers; k; k = k->next) {
                if (org_room == k->follower->in_room && EFF_FLAGGED(k->follower, EFF_CHARM)) {
                    found = true;
                    if (GET_STANCE(k->follower) >= STANCE_RESTING) {
                        anyawake = true;
                        command_interpreter(k->follower, message);
                    }
                }
            }
            if (found) {
                if (anyawake) {
                    WAIT_STATE(ch, PULSE_VIOLENCE / 2);
                    char_printf(ch, OK);
                } else {
                    char_printf(ch, "None of your subjects are awake.\n");
                }
            } else
                char_printf(ch, "Nobody here is a loyal subject of yours!\n");
        }
    }
}

/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
 *********************************************************************/

ACMD(do_gen_comm) {
    extern int level_can_shout;
    extern int holler_move_cost;
    DescriptorData *i;
    char color_on[24];
    bool shapechanged = false;

    /* Array of flags which must _not_ be set in order for comm to be heard */
    static int channels[] = {0, PRF_DEAF, PRF_NOGOSS, 0, 0};

    /*
     * com_msgs: [0] Message if you can't perform the action because of noshout
     *           [1] name of the action
     *           [2] message if you're not on the channel
     *           [3] a color string.
     */
    static const char *com_msgs[][4] = {
        {"You cannot holler!!\n", "holler", "", FYEL},
        {"You cannot shout!!\n", "shout", "Turn off your noshout flag first!\n", FYEL},
        {"You cannot gossip!!\n", "gossip", "You aren't even on the channel!\n", FYEL},
        {"You cannot congratulate!\n", "congrat", "You aren't even on the channel!\n", FGRN}};

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        char_printf(ch, "Your lips move, but no sound forms.\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
        char_printf(ch, com_msgs[subcmd][0]);
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) && GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "The walls seem to absorb your words.\n");
        return;
    }
    /* level_can_shout defined in config.c */
    if (GET_LEVEL(ch) < level_can_shout) {
        char_printf(ch, "You must be at least level {:d} before you can {}.\n", level_can_shout, com_msgs[subcmd][1]);
        return;
    }

    /* make sure the char is on the channel */
    if (PRF_FLAGGED(ch, channels[subcmd])) {
        char_printf(ch, com_msgs[subcmd][2]);
        return;
    }

    /* This prevents gossiping when the channel is disabled... but allows gods */
    /*  to continue to do so... Selina, 3-26-99 */
    if (!gossip_channel_active && (GET_LEVEL(ch) < LVL_GOD) && (subcmd == SCMD_GOSSIP)) {
        char_printf(ch, "You try to gossip, but the heavens have disabled the channel for the time being.\n");
        return;
    }

    /* skip leading spaces */
    std::string speech = std::string(trim(argument));

    /* make sure that there is something there to say! */
    if (speech.empty()) {
        char_printf(ch, "Yes, {}, fine, {} we must, but WHAT???\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
        return;
    }
    if (subcmd == SCMD_HOLLER) {
        if ((int)GET_MOVE(ch) < holler_move_cost) {
            char_printf(ch, "You're too exhausted to holler.\n");
            return;
        } else
            alter_move(ch, holler_move_cost);
    }

    if (!speech_ok(ch, 0))
        return;

    if (GET_LEVEL(REAL_CHAR(ch)) < LVL_IMMORT)
        speech = strip_ansi(speech);

    speech = drunken_speech(speech, GET_COND(ch, DRUNK));

    /* set up the color on code */
    strcpy(color_on, com_msgs[subcmd][3]);

    /* first, set up strings to be given to the communicator */
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        char_printf(ch, OK);
    else {
        if (COLOR_LEV(ch) >= C_CMP)
            sprintf(buf1, "%sYou %s, '%s%s%s'%s", color_on, com_msgs[subcmd][1], speech.c_str(), ANRM, color_on, ANRM);
        else
            sprintf(buf1, "You %s, '%s@0'", com_msgs[subcmd][1], speech.c_str());
        act(buf1, false, ch, nullptr, nullptr, TO_CHAR | TO_SLEEP | TO_OLC);
    }

    sprintf(buf, "$n %ss, '%s'", com_msgs[subcmd][1], speech.c_str());

    /*
     * If the gossiper is shapechanged and this descriptor can see both
     * the shapechanged mob and the original druid, then use show
     * both names instead of just the actual gossiper.
     */
    if (subcmd == SCMD_GOSSIP && POSSESSED(ch) && GET_LEVEL(POSSESSOR(ch)) < 100 && GET_NAME(ch) && *GET_NAME(ch)) {
        shapechanged = true;
        sprintf(buf1, "%s (%s) gossips, '%s'\n", capitalize(GET_NAME(ch)), GET_NAME(POSSESSOR(ch)), speech.c_str());
    }

    /* now send all the strings out */
    for (i = descriptor_list; i; i = i->next) {
        if (!IS_PLAYING(i))
            continue;
        if (i == ch->desc || !i->character)
            continue;
        if (PRF_FLAGGED(i->character, channels[subcmd]))
            continue;
        if (ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF) && GET_LEVEL(i->character) < LVL_IMMORT)
            continue;
        if (STATE(i) != CON_PLAYING || PLR_FLAGGED(i->character, PLR_WRITING) ||
            PLR_FLAGGED(i->character, PLR_MAILING) || EDITING(i))
            if (!PRF_FLAGGED(i->character, PRF_OLCCOMM))
                continue;
        if (subcmd == SCMD_SHOUT &&
            (world[ch->in_room].zone != world[i->character->in_room].zone || !AWAKE(i->character)))
            continue;

        if (COLOR_LEV(i->character) >= C_NRM)
            desc_printf(i, color_on);

        /*
         * If the gossiper is shapechanged and this descriptor can see both
         * the shapechanged mob and the original druid, then use show
         * both names instead of just the actual gossiper.
         */
        if (shapechanged && CAN_SEE(i->character, ch) && CAN_SEE(i->character, POSSESSOR(ch)))
            desc_printf(i, buf1);
        else {
            act(buf, false, ch, nullptr, i->character, TO_VICT | TO_SLEEP | TO_OLC);
            auto formatted = format_act(buf, ch, 0, i->character, i->character);
            add_retained_comms(i->character, TYPE_RETAINED_GOSSIPS, formatted.c_str());
        }
        if (COLOR_LEV(i->character) >= C_NRM)
            desc_printf(i, ANRM);
    }
}

ACMD(do_qcomm) {
    DescriptorData *i;

    if (!PRF_FLAGGED(ch, PRF_QUEST)) {
        char_printf(ch, "You aren't even part of the quest!\n");
        return;
    }
    std::string speech = std::string(trim(argument));

    if (speech.empty())
        char_printf(ch, "{}? Yes, fine {} we must, but WHAT??\n", capitalize(CMD_NAME), CMD_NAME);
    else if (speech_ok(ch, 0)) {
        speech = drunken_speech(speech, GET_COND(ch, DRUNK));

        if (PRF_FLAGGED(ch, PRF_NOREPEAT))
            char_printf(ch, OK);
        else {
            if (subcmd == SCMD_QSAY) {
                if (EFF_FLAGGED(ch, EFF_SILENCE)) {
                    char_printf(ch, "Your lips move, but no sound forms\n");
                    return;
                }
                sprintf(buf, "You quest-say, '%s@0'", speech.c_str());
            } else
                strcpy(buf, speech.c_str());
            act(buf, false, ch, nullptr, speech, TO_CHAR | TO_OLC | TO_SLEEP);
        }

        if (subcmd == SCMD_QSAY) {
            if (EFF_FLAGGED(ch, EFF_SILENCE)) {
                char_printf(ch, "Your lips move, but no sound forms\n");
                return;
            }
            sprintf(buf, "$n quest-says, '%s@0'", argument);
        } else
            strcpy(buf, argument);

        for (i = descriptor_list; i; i = i->next)
            if (!i->connected && i != ch->desc && PRF_FLAGGED(i->character, PRF_QUEST))
                act(buf, false, ch, nullptr, i->character, TO_VICT | TO_SLEEP | TO_OLC);
    }
}

ACMD(do_report) {
    char rbuf[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    sprintf(rbuf, "%s%sI have %s%d%s (%d) hit and %s%d%s (%d) movement points.", subcmd != SCMD_GREPORT ? arg : "",
            subcmd != SCMD_GREPORT && *arg ? " " : "", status_string(GET_HIT(ch), GET_MAX_HIT(ch), STATUS_COLOR),
            GET_HIT(ch), ANRM, GET_MAX_HIT(ch), status_string(GET_MOVE(ch), GET_MAX_MOVE(ch), STATUS_COLOR),
            GET_MOVE(ch), ANRM, GET_MAX_MOVE(ch));

    if (subcmd == SCMD_GREPORT)
        do_gsay(ch, rbuf, 0, 0);
    else if (*arg)
        do_tell(ch, rbuf, 0, 0);
    else
        do_say(ch, rbuf, 0, 0);
}
