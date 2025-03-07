/***************************************************************************
 *  File: dg_olc.c                                        Part of FieryMUD *
 * Usage: This source file is used in extending Oasis style OLC for        *
 *        dg-scripts onto a CircleMUD that already has dg-scripts (as      *
 *        released by Mark Heilpern on 1/1/98) implemented.                *
 * Parts of this file by Chris Jacobson of _Aliens vs Predator: The MUD_   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "dg_olc.hpp"

#include "bitflags.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "events.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "modify.hpp"
#include "olc.hpp"
#include "stack.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* prototype externally defined functions */
void trig_data_copy(TrigData *cur, const TrigData *trg);

void trigedit_disp_menu(DescriptorData *d);
void trigedit_save(DescriptorData *d);

/* called when a mob or object is being saved to disk, so its script can */
/* be saved */
void script_save_to_disk(FILE *fp, void *item, int type) {
    TriggerPrototypeList *t;

    if (type == MOB_TRIGGER)
        t = ((CharData *)item)->proto_script;
    else if (type == OBJ_TRIGGER)
        t = ((ObjData *)item)->proto_script;
    else if (type == WLD_TRIGGER)
        t = ((RoomData *)item)->proto_script;
    else {
        log("SYSERR: Invalid type passed to script_save_mobobj_to_disk()");
        return;
    }

    while (t) {
        fprintf(fp, "T %d\n", t->vnum);
        t = t->next;
    }
}

void trigedit_setup_new(DescriptorData *d) {
    TrigData *trig;

    /*
     * Allocate a scratch trigger structure
     */
    CREATE(trig, TrigData, 1);

    trig->nr = -1;

    /*
     * Set up some defaults
     */
    trig->name = strdup("new trigger");
    trig->trigger_type = MTRIG_GREET;

    /* cmdlist will be a large char string until the trigger is saved */
    OLC_STORAGE(d) = "say My trigger commandlist is not complete!\n";
    trig->narg = 100;

    OLC_TRIG(d) = trig;
    OLC_VAL(d) = 0; /* Has changed flag. (It hasn't so far, we just made it.) */

    trigedit_disp_menu(d);
}

void trigedit_setup_existing(DescriptorData *d, int rtrg_num) {
    TrigData *trig;
    CmdlistElement *c;

    /*
     * Allocate a scratch trigger structure
     */
    CREATE(trig, TrigData, 1);

    trig_data_copy(trig, trig_index[rtrg_num]->proto);

    /* convert cmdlist to a char string */
    c = trig->cmdlist;
    OLC_STORAGE(d).clear();

    while (c) {
        OLC_STORAGE(d) += std::format("{}\n", c->cmd);
        c = c->next;
    }
    /* now trig->cmdlist is something to pass to the text editor */
    /* it will be converted back to a real cmdlist_element list later */

    OLC_TRIG(d) = trig;
    OLC_VAL(d) = 0; /* Has changed flag. (It hasn't so far, we just made it.) */

    trigedit_disp_menu(d);
}

void trigedit_disp_menu(DescriptorData *d) {
    TrigData *trig = OLC_TRIG(d);
    std::string_view attach_type;
    std::string trgtypes;

    get_char_cols(d->character);

    if (trig->attach_type == OBJ_TRIGGER) {
        attach_type = "Objects";
        trgtypes = sprintbit(GET_TRIG_TYPE(trig), otrig_types);
    } else if (trig->attach_type == WLD_TRIGGER) {
        attach_type = "Rooms";
        trgtypes = sprintbit(GET_TRIG_TYPE(trig), wtrig_types);
    } else {
        attach_type = "Mobiles";
        trgtypes = sprintbit(GET_TRIG_TYPE(trig), trig_types);
    }

    char_printf(d->character,
#if defined(CLEAR_SCREEN)
                "[H[J"
#endif
                "Trigger Editor [{}{}{}]\n\n"
                "{}1){} Name         : {}{}\n"
                "{}2){} Intended for : {}{}\n"
                "{}3){} Trigger types: {}{}\n"
                "{}4){} Numeric Arg  : {}{}\n"
                "{}5){} Arguments    : {}{}\n"
                "{}6){} Commands:\n{}{}\n"
                "{}Q){} Quit\n"
                "Enter Choice:\n",
                grn, OLC_NUM(d), nrm,               /* vnum on the title line */
                grn, nrm, yel, GET_TRIG_NAME(trig), /* name                   */
                grn, nrm, yel, attach_type,         /* attach type            */
                grn, nrm, yel, trgtypes,            /* greet/drop/etc         */
                grn, nrm, yel, trig->narg,          /* numeric arg            */
                grn, nrm, yel, trig->arglist,       /* strict arg             */
                grn, nrm, cyn, OLC_STORAGE(d),      /* the command list       */
                grn, nrm);                          /* quit colors            */

    OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
}

void trigedit_disp_types(DescriptorData *d) {
    int columns = 0;
    const std::string_view *types;

    switch (OLC_TRIG(d)->attach_type) {
    case WLD_TRIGGER:
        types = wtrig_types;
        break;
    case OBJ_TRIGGER:
        types = otrig_types;
        break;
    case MOB_TRIGGER:
    default:
        types = trig_types;
        break;
    }

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif
    for (int i = 0; i < NUM_TRIG_TYPE_FLAGS; i++)
        char_printf(d->character,
                    fmt::format("{}{:2d}{}) {:20.20s}  {}", grn, i + 1, nrm, types[i], !(++columns % 2) ? "\n" : ""));
    char_printf(d->character, fmt::format("\nCurrent types : {}{}{}\nEnter type (0 to quit):\n", cyn,
                                          sprintbit(GET_TRIG_TYPE(OLC_TRIG(d)), types), nrm));
}

void trigedit_parse(DescriptorData *d, std::string_view arg) {
    int i = 0;

    switch (OLC_MODE(d)) {
    case TRIGEDIT_MAIN_MENU:
        switch (tolower(arg.front())) {
        case 'q':
            if (OLC_VAL(d)) { /* Anything been changed? */
                if (!GET_TRIG_TYPE(OLC_TRIG(d))) {
                    char_printf(d->character, "Invalid Trigger Type! Answer a to abort quit!\n");
                }
                char_printf(d->character, "Do you wish to save the changes to the trigger? (y/n)\n");
                OLC_MODE(d) = TRIGEDIT_CONFIRM_SAVESTRING;
            } else
                cleanup_olc(d, CLEANUP_ALL);
            return;
        case '1':
            OLC_MODE(d) = TRIGEDIT_NAME;
            char_printf(d->character, "Name:\n");
            break;
        case '2':
            OLC_MODE(d) = TRIGEDIT_INTENDED;
            char_printf(d->character, "0: Mobiles, 1: Objects, 2: Rooms:\n");
            break;
        case '3':
            OLC_MODE(d) = TRIGEDIT_TYPES;
            trigedit_disp_types(d);
            break;
        case '4':
            OLC_MODE(d) = TRIGEDIT_NARG;
            char_printf(d->character, "Numeric argument:\n");
            break;
        case '5':
            OLC_MODE(d) = TRIGEDIT_ARGUMENT;
            char_printf(d->character, "Argument:\n");
            break;
        case '6':
            OLC_MODE(d) = TRIGEDIT_COMMANDS;
            char_printf(d->character, "Enter trigger commands: (/s saves /h for help)\n\n");
            string_write(d, OLC_STORAGE(d), MAX_CMD_LENGTH);
            OLC_VAL(d) = 1;

            break;
        default:
            trigedit_disp_menu(d);
            return;
        }
        return;

    case TRIGEDIT_CONFIRM_SAVESTRING:
        switch (tolower(arg.front())) {
        case 'y':
            trigedit_save(d);
            log(LogSeverity::Debug, std::max<int>(LVL_BUILDER, GET_INVIS_LEV(d->character)),
                "OLC: {} edits trigger {:d}", GET_NAME(d->character), OLC_NUM(d));
            /* fall through */
        case 'n':
            cleanup_olc(d, CLEANUP_ALL);
            return;
        case 'a': /* abort quitting */
            break;
        default:
            char_printf(d->character, "Invalid choice!\n");
            char_printf(d->character, "Do you wish to save the trigger?\n");
            return;
        }
        break;

    case TRIGEDIT_NAME:
        OLC_TRIG(d)->name = arg.empty() ? "undefined" : arg;
        OLC_VAL(d)++;
        break;

    case TRIGEDIT_INTENDED:
        if ((svtoi(arg) >= MOB_TRIGGER) || (svtoi(arg) <= WLD_TRIGGER))
            OLC_TRIG(d)->attach_type = svtoi(arg);
        OLC_VAL(d)++;
        break;

    case TRIGEDIT_NARG:
        OLC_TRIG(d)->narg = svtoi(arg);
        OLC_VAL(d)++;
        break;

    case TRIGEDIT_ARGUMENT:
        OLC_TRIG(d)->arglist = arg;
        OLC_VAL(d)++;
        break;

    case TRIGEDIT_TYPES:
        if ((i = svtoi(arg)) == 0)
            break;
        else if (!((i < 0) || (i > NUM_TRIG_TYPE_FLAGS)))
            TOGGLE_BIT((GET_TRIG_TYPE(OLC_TRIG(d))), 1 << (i - 1));
        OLC_VAL(d)++;
        trigedit_disp_types(d);
        return;

    case TRIGEDIT_COMMANDS:
        break;
    }

    OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
    trigedit_disp_menu(d);
}

/*
** print out the letter codes pertaining to the bits set in 'data'
*/
std::string sprintbits(int data) {
    std::string dest;
    for (int i = 0; i < 32; i++) {
        if (data & (1 << i)) {
            dest += (i <= 25) ? ('a' + i) : ('A' + i);
        }
    }
    return dest;
}

/* save the zone's triggers to internal memory and to disk */
void trigedit_save(DescriptorData *d) {
    int trig_rnum, i;
    bool found = false;
    std::string_view s;
    TrigData *proto;
    TrigData *trig = OLC_TRIG(d);
    TrigData *live_trig;
    CmdlistElement *cmd, *next_cmd;
    std::vector<IndexData *> new_index;
    DescriptorData *dsc;
    FILE *trig_file;
    int zone, top;
    char buf[MAX_CMD_LENGTH];
    char fname[MAX_INPUT_LENGTH];

    auto get_next_arg = [&]() {
        s = s.substr(s.find(' ') + 1);
        return s.substr(0, s.find(' '));
    };

    if ((trig_rnum = real_trigger(OLC_NUM(d))) != -1) {
        proto = trig_index[trig_rnum]->proto;
        for (cmd = proto->cmdlist; cmd; cmd = next_cmd) {
            next_cmd = cmd->next;
            free(cmd);
        }

        /* Recompile the command list from the new script */
        s = OLC_STORAGE(d);

        CREATE(trig->cmdlist, CmdlistElement, 1);
        if (!s.empty()) {
            trig->cmdlist->cmd = get_next_arg();
            s.remove_prefix(s.find('\n') + 1);
            cmd = trig->cmdlist;
            cmd->next = nullptr; /*don't want dangling pointers.. */

            while (!(s = get_next_arg()).empty()) {
                CREATE(cmd->next, CmdlistElement, 1);
                cmd = cmd->next;
                cmd->cmd = strdup(s.data());
                cmd->next = nullptr; /*don't want dangling pointers.. */
            }
        } else {
            trig->cmdlist->cmd = strdup("* No Script");
        }

        /* make the prototype look like what we have */
        trig_data_copy(proto, trig);

        /* go through the mud and replace existing triggers */
        live_trig = trigger_list;
        while (live_trig) {
            if (GET_TRIG_RNUM(live_trig) == trig_rnum) {
                live_trig->arglist = proto->arglist;
                live_trig->name = proto->name;
                live_trig->cmdlist = proto->cmdlist;
                live_trig->curr_state = live_trig->cmdlist;
                live_trig->trigger_type = proto->trigger_type;
                live_trig->attach_type = proto->attach_type;
                live_trig->narg = proto->narg;
                live_trig->data_type = proto->data_type;
                live_trig->depth = 0;
                live_trig->wait_event = nullptr;
                if (GET_TRIG_WAIT(live_trig))
                    event_cancel(GET_TRIG_WAIT(live_trig));
                free_varlist(live_trig->var_list);
            }

            live_trig = live_trig->next_in_world;
        }
    } else {
        /* this is a new trigger */
        new_index.resize(top_of_trigt + 2);

        /* Recompile the command list from the new script */
        s = OLC_STORAGE(d);

        CREATE(trig->cmdlist, CmdlistElement, 1);
        if (!s.empty()) {
            trig->cmdlist->cmd = get_next_arg();
            cmd = trig->cmdlist;
            cmd->next = nullptr; /*don't want dangling pointers.. */

            while (!(s = get_next_arg()).empty()) {
                CREATE(cmd->next, CmdlistElement, 1);
                cmd = cmd->next;
                cmd->cmd = strdup(s.data());
                cmd->next = nullptr; /*don't want dangling pointers.. */
            }
        } else {
            trig->cmdlist->cmd = strdup("* No Script");
        }

        for (i = 0; i < top_of_trigt; i++) {
            if (!found) {
                if (trig_index[i]->vnum > OLC_NUM(d)) {
                    found = true;
                    trig_rnum = i;

                    CREATE(new_index[trig_rnum], IndexData, 1);
                    GET_TRIG_RNUM(OLC_TRIG(d)) = trig_rnum;
                    new_index[trig_rnum]->vnum = OLC_NUM(d);
                    new_index[trig_rnum]->number = 0;
                    new_index[trig_rnum]->func = nullptr;
                    CREATE(proto, TrigData, 1);
                    new_index[trig_rnum]->proto = proto;
                    trig_data_copy(proto, trig);

                    if (!trig->name.empty())
                        proto->name = trig->name;
                    if (!trig->arglist.empty())
                        proto->arglist = trig->arglist;

                    new_index[trig_rnum + 1] = trig_index[trig_rnum];

                    proto = trig_index[trig_rnum]->proto;
                    proto->nr = trig_rnum + 1;
                } else {
                    new_index[i] = trig_index[i];
                }
            } else {
                new_index[i + 1] = trig_index[i];
                proto = trig_index[i]->proto;
                proto->nr = i + 1;
            }
        }

        if (!found) {
            trig_rnum = i;
            CREATE(new_index[trig_rnum], IndexData, 1);
            GET_TRIG_RNUM(OLC_TRIG(d)) = trig_rnum;
            new_index[trig_rnum]->vnum = OLC_NUM(d);
            new_index[trig_rnum]->number = 0;
            new_index[trig_rnum]->func = nullptr;

            CREATE(proto, TrigData, 1);
            new_index[trig_rnum]->proto = proto;
            trig_data_copy(proto, trig);

            if (!trig->name.empty())
                proto->name = trig->name;
            if (!trig->arglist.empty())
                proto->arglist = trig->arglist;
        }

        trig_index = new_index.data();
        top_of_trigt++;

        /* Update all triggers with higher rnum */
        for (live_trig = trigger_list; live_trig; live_trig = live_trig->next_in_world)
            if (GET_TRIG_RNUM(live_trig) > trig_rnum)
                GET_TRIG_RNUM(live_trig)++;

        /* Update other trigs being edited */
        for (dsc = descriptor_list; dsc; dsc = dsc->next)
            if (dsc->connected == CON_TRIGEDIT)
                if (GET_TRIG_RNUM(OLC_TRIG(dsc)) >= trig_rnum)
                    GET_TRIG_RNUM(OLC_TRIG(dsc))++;
    }

    /* Write the trigger out to disk */
    zone = zone_table[OLC_ZNUM(d)].number;
    top = zone_table[OLC_ZNUM(d)].top;

    fname = fmt::format("{}/{}.new", TRG_PREFIX, zone);
    if (!(trig_file = fopen(fname, "w"))) {
        log(LogSeverity::Warn, std::max<int>(LVL_GOD, GET_INVIS_LEV(d->character)),
            "SYSERR: OLC: Can't open trig file \"{}\"", fname);
        return;
    }

    for (i = zone * 100; i <= top; i++) {
        if ((trig_rnum = real_trigger(i)) != -1) {
            trig = trig_index[trig_rnum]->proto;

            if (fprintf(trig_file, "#%d\n", i) < 0) {
                log(LogSeverity::Warn, std::max<int>(LVL_GOD, GET_INVIS_LEV(d->character)),
                    "SYSERR: OLC: Can't write trig file!");
                fclose(trig_file);
                return;
            }

            fprintf(trig_file,
                    "%s~\n"
                    "%d %s %d\n"
                    "%s~\n",
                    (GET_TRIG_NAME(trig)) ? (GET_TRIG_NAME(trig)) : "unknown trigger", trig->attach_type,
                    sprintbits(GET_TRIG_TYPE(trig)).c_str(), GET_TRIG_NARG(trig),
                    !GET_TRIG_ARG(trig).empty() ? GET_TRIG_ARG(trig).c_str() : "");

            /* Build the text for the script */
            std::string script_text;
            for (cmd = trig->cmdlist; cmd; cmd = cmd->next) {
                script_text += cmd->cmd + "\n";
            }

            if (script_text.empty())
                script_text = "* Empty script";

            fprintf(trig_file, "%s~\n", script_text.c_str());
        }
    }

    fprintf(trig_file, "$~\n");
    fclose(trig_file);

    snprintf(buf, sizeof(buf), "%s/%d.trg", TRG_PREFIX, zone);
    rename(fname, buf);
}

void dg_olc_script_free(DescriptorData *d) {
    TriggerPrototypeList *editscript, *prevscript;

    editscript = OLC_SCRIPT(d);
    while (editscript) {
        prevscript = editscript;
        editscript = editscript->next;
        free(prevscript);
    }
}

void dg_olc_script_copy(DescriptorData *d) {
    TriggerPrototypeList *origscript, *editscript;

    if (OLC_ITEM_TYPE(d) == MOB_TRIGGER)
        origscript = OLC_MOB(d)->proto_script;
    else if (OLC_ITEM_TYPE(d) == OBJ_TRIGGER)
        origscript = OLC_OBJ(d)->proto_script;
    else
        origscript = OLC_ROOM(d)->proto_script;

    if (origscript) {
        CREATE(editscript, TriggerPrototypeList, 1);
        OLC_SCRIPT(d) = editscript;

        while (origscript) {
            editscript->vnum = origscript->vnum;
            origscript = origscript->next;
            if (origscript)
                CREATE(editscript->next, TriggerPrototypeList, 1);
            editscript = editscript->next;
        }
    } else
        OLC_SCRIPT(d) = nullptr;
}

void dg_script_menu(DescriptorData *d) {
    TriggerPrototypeList *editscript;
    int i = 0;

    /* make sure our input parser gets used */
    OLC_MODE(d) = OLC_SCRIPT_EDIT;
    OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;

#if defined(CLEAR_SCREEN) /* done wierd to compile with the vararg send() */

    /* Hey does that line entionally have those control characters? RSD */

#define FMT "[H[J     Script Editor\n\n     Trigger List:\n"
#else
#define FMT "     Script Editor\n\n     Trigger List:\n"
#endif
    char_printf(d->character, FMT);
#undef FMT

    editscript = OLC_SCRIPT(d);
    while (editscript) {
        sprintf(buf, "     %2d) [%s%d%s] %s%s%s", ++i, cyn, editscript->vnum, nrm, cyn,
                trig_index[real_trigger(editscript->vnum)]->proto->name, nrm);
        char_printf(d->character, buf);
        if (trig_index[real_trigger(editscript->vnum)]->proto->attach_type != OLC_ITEM_TYPE(d))
            sprintf(buf, "   %s** Mis-matched Trigger Type **%s\n", grn, nrm);
        else
            sprintf(buf, "\n");
        char_printf(d->character, buf);

        editscript = editscript->next;
    }
    if (i == 0)
        char_printf(d->character, "     <none>\n");

    sprintf(buf,
            "\n"
            " %sN%s)  New trigger for this script\n"
            " %sD%s)  Delete a trigger in this script\n"
            " %sX%s)  Exit Script Editor\n\n"
            "     Enter choice:\n",
            grn, nrm, grn, nrm, grn, nrm);
    char_printf(d->character, buf);
}

int dg_script_edit_parse(DescriptorData *d, const std::string_view arg) {
    TriggerPrototypeList *trig, *currtrig, *starttrig;
    int pos, vnum;

    switch (OLC_SCRIPT_EDIT_MODE(d)) {
    case SCRIPT_MAIN_MENU:
        switch (tolower(*arg)) {
        case 'x':
        case 'X':
            if (OLC_ITEM_TYPE(d) == MOB_TRIGGER) {
                trig = OLC_MOB(d)->proto_script;
                OLC_MOB(d)->proto_script = OLC_SCRIPT(d);
            } else if (OLC_ITEM_TYPE(d) == OBJ_TRIGGER) {
                trig = OLC_OBJ(d)->proto_script;
                OLC_OBJ(d)->proto_script = OLC_SCRIPT(d);
            } else {
                trig = OLC_ROOM(d)->proto_script;
                OLC_ROOM(d)->proto_script = OLC_SCRIPT(d);
            }

            return 0;
        case 'n':
        case 'N':
            char_printf(d->character, "\nPlease enter trigger vnum:\n");
            OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_NEW_TRIGGER;
            break;
        case 'd':
        case 'D':
            if (OLC_SCRIPT(d) == (TriggerPrototypeList *)nullptr)
                char_printf(d->character, "Cannot delete a trigger as there are none!\n");
            else {
                char_printf(d->character, "     Which entry should be deleted?  0 to abort:\n");
                OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_DEL_TRIGGER;
            }
            break;
        default:
            char_printf(d->character, "\nUnrecognized command.  Try again:\n");
            break;
        }
        return 1;

    case SCRIPT_NEW_TRIGGER:
        vnum = -1;
        vnum = atoi(arg);

        if (vnum <= 0)
            break; /* this aborts a new trigger entry */

        if (real_trigger(vnum) < 0) {
            char_printf(d->character,
                        "Invalid Trigger VNUM!\n"
                        "Please enter vnum:\n");
            return 1;
        }

        CREATE(trig, TriggerPrototypeList, 1);
        trig->vnum = vnum;
        trig->next = (TriggerPrototypeList *)nullptr;

        /* add the new info in position */
        if (OLC_SCRIPT(d) == (TriggerPrototypeList *)nullptr)
            OLC_SCRIPT(d) = trig;
        else {
            currtrig = OLC_SCRIPT(d);

            /* go to end of list */
            while (currtrig && currtrig->next)
                currtrig = currtrig->next;

            currtrig->next = trig;
        }
        OLC_VAL(d)++;
        break;

    case SCRIPT_DEL_TRIGGER:
        pos = atoi(arg);
        if (pos <= 0)
            break;

        currtrig = OLC_SCRIPT(d);
        starttrig = currtrig; /* hold the start of list trig and currtrig traverse the list
                                 and we need to know the beginning for when we return */
        trig = currtrig;
        while (--pos && currtrig->next) {
            trig = currtrig;
            currtrig = currtrig->next;
        }

        if (pos) { /* damn fool specified a non-existent position */
            char_printf(d->character, "No such trigger!\nTry Again:\n");
            return 1;
        }
        /* we are going to free currtrig...so we need to join up around it */

        if (trig == currtrig) { /* first node in list */
            /* first node in list is to be removed, so we just start list
             * at second node...
             * note: if there _is_ only 1 node, currtrig->next = NULL
             */
            starttrig = currtrig->next;
        } else {
            /* currtrig is the node to be removed, trig is the node before
             * so make trig->next skip currtrig...
             */
            trig->next = currtrig->next;
        }
        free(currtrig);

        OLC_SCRIPT(d) = starttrig;

        break;
    }

    dg_script_menu(d);
    return 1;
}

/*
 * Formats and indents a script.  Returns true if the format was
 * successful.  Returns false if the format failed--in which case
 * the original text is not replaced.
 */
bool format_script(DescriptorData *d, int indent_quantum) {
#define CS_NONE 0
#define CS_IF 1
#define CS_SWITCH 2
#define CS_CASE 3
#define CS_DEFAULT 4
#define CS_WHILE 5
#define CS_ELSEIF 6
#define CS_ELSE 7
#define COMPLAIN(msg)                                                                                                  \
    do {                                                                                                               \
        snprintf(error, sizeof(error) - 1, msg " (line %d)!\n", line_num);                                             \
        string_to_output(d, error);                                                                                    \
    } while (0)
#define ABORT(msg)                                                                                                     \
    do {                                                                                                               \
        COMPLAIN(msg);                                                                                                 \
        return false;                                                                                                  \
    } while (0)

    std::string_view line, *script;
    char out[MAX_CMD_LENGTH], error[100];
    int line_num = 0, indent = 0, len = 0, max_len, i, indent_next = 0, needed;
    struct Scope {
        int scope;
        int line_num;
    } scope;
    array_stack(Scope) stack;
    struct scope_types {
        int type;
        const std::string_view name;
        int length;
    } cmd_scopes[] = {
        {CS_IF, "if ", 3},       {CS_SWITCH, "switch ", 7}, {CS_CASE, "case", 4}, {CS_DEFAULT, "default", 7},
        {CS_WHILE, "while ", 6}, {CS_ELSEIF, "elseif ", 7}, /* needs to come before else check */
        {CS_ELSE, "else", 4},    {CS_NONE, nullptr, 0},
    };

    if (!d->str || d.empty()->str)
        return false;

    script = strdup(*d->str); /* Make a copy, because of strtok() */
    *out = '\0';
    max_len = std::max(d->max_str, sizeof(out));
    scope.scope = CS_NONE;
    scope.line_num = 0;
    as_init(stack, 10, scope);

    /* Iterate through the script line by line */
    for (line = strtok(script, "\n"); line; line = get_next_arg()) {
        ++line_num;
        skip_spaces(line);

        /* Does this line open a new scope? */
        for (i = 0; cmd_scopes[i].name; ++i)
            if (!strncasecmp(line, cmd_scopes[i].name, cmd_scopes[i].length))
                break;

        /* If it does, i will not point to the last cmd_scopes entry */
        if (cmd_scopes[i].name) {
            /* Found a scope opener */
            switch (cmd_scopes[i].type) {
            case CS_CASE:
            case CS_DEFAULT:
                if (as_peek(stack).scope == CS_DEFAULT)
                    ABORT("Case/default after default statement");
                else if (as_peek(stack).scope == CS_CASE) {
                    /* Falling through from one case statement to another */
                    scope = as_pop(stack);
                    indent -= indent_quantum;
                } else if (as_peek(stack).scope != CS_SWITCH)
                    ABORT("Case/default outside switch");
                /* Scope is switch */
                scope.scope = cmd_scopes[i].type;
                scope.line_num = line_num;
                as_push(stack, scope);
                indent_next = indent_quantum;
                break;
            case CS_ELSEIF:
            case CS_ELSE:
                if (as_peek(stack).scope != CS_IF && as_peek(stack).scope != CS_ELSEIF)
                    ABORT("Unmatched 'else'");
                scope = as_pop(stack); /* pop off IF or ELSEIF */
                indent -= indent_quantum;
                scope.scope = cmd_scopes[i].type;
                scope.line_num = line_num;
                as_push(stack, scope);
                indent_next = indent_quantum;
                break;
            default: /* if, switch, while go here */
                scope.scope = cmd_scopes[i].type;
                scope.line_num = line_num;
                as_push(stack, scope);
                indent_next = indent_quantum;
                break;
            }
        } else {
            /* Didn't find a scope opener */
            if (matches(line, "end"){
                switch (as_peek(stack).scope) {
                case CS_IF:
                case CS_ELSEIF:
                case CS_ELSE:
                    scope = as_pop(stack);
                    indent -= indent_quantum;
                    break;
                default:
                    ABORT("Unmatched 'end'");
                }
            } else if (matches(line, "done"){
                do {
                    if (as_peek(stack).scope == CS_WHILE)
                        break;
                    if (as_peek(stack).scope == CS_SWITCH)
                        COMPLAIN("Switch/done without case/default");
                    if (as_peek(stack).scope == CS_CASE || as_peek(stack).scope == CS_DEFAULT) {
                        scope = as_pop(stack);
                        indent -= indent_quantum;
                    }
                    /* Keep going, check for switch */
                    if (as_peek(stack).scope == CS_SWITCH)
                        break;
                    ABORT("Unmatched 'done'");
                } while (0);
                scope = as_pop(stack);
                indent -= indent_quantum;
            } else if (matches(line, "break"){
                array_stack(Scope) temp;
                as_init(temp, as_size(stack), as_null(stack));
                /* Search the stack for a while, case, or default */
                for (i = false; !as_empty(stack);) {
                    as_push(temp, as_pop(stack));
                    if (as_peek(temp).scope == CS_WHILE || as_peek(temp).scope == CS_CASE ||
                        as_peek(temp).scope == CS_DEFAULT) {
                        i = true; /* found */
                        break;
                    }
                }
                if (!i)
                    COMPLAIN("Break not in 'case' or 'while'");
                /* Put everything back on the original stack */
                while (!as_empty(temp))
                    as_push(stack, as_pop(temp));
                as_destroy(temp);
            } else if (as_peek(stack).scope == CS_SWITCH)
                COMPLAIN("Non case/default statement directly after 'switch'");
        }

        /* How much space do we need for this line?  Include indent and \n */
        needed = strnlen(line, max_len - len + 1) + indent + 2;
        if (len + needed >= max_len)
            ABORT("String too long, formatting aborted");

        /* Indent, then copy line */
        for (i = 0; i < indent; ++i)
            out[len++] = ' ';
        len += sprintf(out + len, "%s\n", line); /* size checked above */

        /* Indent the next line */
        if (indent_next) {
            indent += indent_next;
            indent_next = 0;
        }
    }

    /* If the stack is not empty, complain about it */
    while (!as_empty(stack)) {
        scope = as_pop(stack);
        line_num = scope.line_num;
        switch (scope.scope) {
        case CS_IF:
            COMPLAIN("Unmatched 'if' ignored");
            break;
        case CS_SWITCH:
            COMPLAIN("Unmatched 'switch' ignored");
            break;
        case CS_CASE:
            COMPLAIN("Unmatched 'switch/case' ignored");
            break;
        case CS_DEFAULT:
            COMPLAIN("Unmatched 'default' ignored");
            break;
        case CS_WHILE:
            COMPLAIN("Unmatched 'while' ignored");
            break;
        case CS_ELSE:
            COMPLAIN("Unmatched 'else' ignored");
            break;
        case CS_ELSEIF:
            COMPLAIN("Unmatched 'elseif' ignored");
            break;
        }
    }

    /* Clean-up strings */
    free(*d->str);
    *d->str = strdup(out);
    free(script);
    as_destroy(stack);

    return true;

    /* Clean-up defines */
#undef CS_NONE
#undef CS_IF
#undef CS_SWITCH
#undef CS_CASE
#undef CS_DEFAULT
#undef CS_WHILE
#undef CS_ELSE
#undef CS_ELSEIF
#undef ABORT
#undef COMPLAIN
}
