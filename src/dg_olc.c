/***************************************************************************
 * $Id: dg_olc.c,v 1.26 2009/01/17 00:28:02 myc Exp $
 ***************************************************************************/
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

#include "dg_olc.h"

#include "comm.h"
#include "conf.h"
#include "db.h"
#include "events.h"
#include "interpreter.h"
#include "math.h"
#include "modify.h"
#include "olc.h"
#include "stack.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* prototype externally defined functions */
void trig_data_copy(trig_data *this, const trig_data *trg);

void trigedit_disp_menu(struct descriptor_data *d);
void trigedit_save(struct descriptor_data *d);

/* called when a mob or object is being saved to disk, so its script can */
/* be saved */
void script_save_to_disk(FILE *fp, void *item, int type) {
    struct trig_proto_list *t;

    if (type == MOB_TRIGGER)
        t = ((struct char_data *)item)->proto_script;
    else if (type == OBJ_TRIGGER)
        t = ((struct obj_data *)item)->proto_script;
    else if (type == WLD_TRIGGER)
        t = ((struct room_data *)item)->proto_script;
    else {
        log("SYSERR: Invalid type passed to script_save_mobobj_to_disk()");
        return;
    }

    while (t) {
        fprintf(fp, "T %d\n", t->vnum);
        t = t->next;
    }
}

void trigedit_setup_new(struct descriptor_data *d) {
    struct trig_data *trig;

    /*
     * Allocate a scratch trigger structure
     */
    CREATE(trig, struct trig_data, 1);

    trig->nr = -1;

    /*
     * Set up some defaults
     */
    trig->name = strdup("new trigger");
    trig->trigger_type = MTRIG_GREET;

    /* cmdlist will be a large char string until the trigger is saved */
    CREATE(OLC_STORAGE(d), char, MAX_CMD_LENGTH);
    strcpy(OLC_STORAGE(d), "say My trigger commandlist is not complete!\r\n");
    trig->narg = 100;

    OLC_TRIG(d) = trig;
    OLC_VAL(d) = 0; /* Has changed flag. (It hasn't so far, we just made it.) */

    trigedit_disp_menu(d);
}

void trigedit_setup_existing(struct descriptor_data *d, int rtrg_num) {
    struct trig_data *trig;
    struct cmdlist_element *c;

    /*
     * Allocate a scratch trigger structure
     */
    CREATE(trig, struct trig_data, 1);

    trig_data_copy(trig, trig_index[rtrg_num]->proto);

    /* convert cmdlist to a char string */
    c = trig->cmdlist;
    CREATE(OLC_STORAGE(d), char, MAX_CMD_LENGTH);
    strcpy(OLC_STORAGE(d), "");

    while (c) {
        strcat(OLC_STORAGE(d), c->cmd);
        strcat(OLC_STORAGE(d), "\r\n");
        c = c->next;
    }
    /* now trig->cmdlist is something to pass to the text editor */
    /* it will be converted back to a real cmdlist_element list later */

    OLC_TRIG(d) = trig;
    OLC_VAL(d) = 0; /* Has changed flag. (It hasn't so far, we just made it.) */

    trigedit_disp_menu(d);
}

void trigedit_disp_menu(struct descriptor_data *d) {
    struct trig_data *trig = OLC_TRIG(d);
    char *attach_type;
    char trgtypes[256];

    get_char_cols(d->character);

    if (trig->attach_type == OBJ_TRIGGER) {
        attach_type = "Objects";
        sprintbit(GET_TRIG_TYPE(trig), otrig_types, trgtypes);
    } else if (trig->attach_type == WLD_TRIGGER) {
        attach_type = "Rooms";
        sprintbit(GET_TRIG_TYPE(trig), wtrig_types, trgtypes);
    } else {
        attach_type = "Mobiles";
        sprintbit(GET_TRIG_TYPE(trig), trig_types, trgtypes);
    }

    sprintf(buf,
#if defined(CLEAR_SCREEN)
            "[H[J"
#endif
            "Trigger Editor [%s%d%s]\r\n\r\n"
            "%s1)%s Name         : %s%s\r\n"
            "%s2)%s Intended for : %s%s\r\n"
            "%s3)%s Trigger types: %s%s\r\n"
            "%s4)%s Numeric Arg  : %s%d\r\n"
            "%s5)%s Arguments    : %s%s\r\n"
            "%s6)%s Commands:\r\n%s%s\r\n"
            "%sQ)%s Quit\r\n"
            "Enter Choice:\r\n",
            grn, OLC_NUM(d), nrm,               /* vnum on the title line */
            grn, nrm, yel, GET_TRIG_NAME(trig), /* name                   */
            grn, nrm, yel, attach_type,         /* attach type            */
            grn, nrm, yel, trgtypes,            /* greet/drop/etc         */
            grn, nrm, yel, trig->narg,          /* numeric arg            */
            grn, nrm, yel, trig->arglist,       /* strict arg             */
            grn, nrm, cyn, OLC_STORAGE(d),      /* the command list       */
            grn, nrm);                          /* quit colors            */

    send_to_char(buf, d->character);
    OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
}

void trigedit_disp_types(struct descriptor_data *d) {
    int i, columns = 0;
    const char **types;

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
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; i < NUM_TRIG_TYPE_FLAGS; i++) {
        sprintf(buf, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, types[i], !(++columns % 2) ? "\r\n" : "");
        send_to_char(buf, d->character);
    }
    sprintbit(GET_TRIG_TYPE(OLC_TRIG(d)), types, buf1);
    sprintf(buf, "\r\nCurrent types : %s%s%s\r\nEnter type (0 to quit) : ", cyn, buf1, nrm);
    send_to_char(buf, d->character);
}

void trigedit_parse(struct descriptor_data *d, char *arg) {
    int i = 0;

    switch (OLC_MODE(d)) {
    case TRIGEDIT_MAIN_MENU:
        switch (tolower(*arg)) {
        case 'q':
            if (OLC_VAL(d)) { /* Anything been changed? */
                if (!GET_TRIG_TYPE(OLC_TRIG(d))) {
                    send_to_char("Invalid Trigger Type! Answer a to abort quit!\r\n", d->character);
                }
                send_to_char("Do you wish to save the changes to the trigger? (y/n)\r\n", d->character);
                OLC_MODE(d) = TRIGEDIT_CONFIRM_SAVESTRING;
            } else
                cleanup_olc(d, CLEANUP_ALL);
            return;
        case '1':
            OLC_MODE(d) = TRIGEDIT_NAME;
            send_to_char("Name:\r\n", d->character);
            break;
        case '2':
            OLC_MODE(d) = TRIGEDIT_INTENDED;
            send_to_char("0: Mobiles, 1: Objects, 2: Rooms:\r\n", d->character);
            break;
        case '3':
            OLC_MODE(d) = TRIGEDIT_TYPES;
            trigedit_disp_types(d);
            break;
        case '4':
            OLC_MODE(d) = TRIGEDIT_NARG;
            send_to_char("Numeric argument:\r\n", d->character);
            break;
        case '5':
            OLC_MODE(d) = TRIGEDIT_ARGUMENT;
            send_to_char("Argument:\r\n", d->character);
            break;
        case '6':
            OLC_MODE(d) = TRIGEDIT_COMMANDS;
            send_to_char("Enter trigger commands: (/s saves /h for help)\r\n\r\n", d->character);
            string_write(d, &OLC_STORAGE(d), MAX_CMD_LENGTH);
            OLC_VAL(d) = 1;

            break;
        default:
            trigedit_disp_menu(d);
            return;
        }
        return;

    case TRIGEDIT_CONFIRM_SAVESTRING:
        switch (tolower(*arg)) {
        case 'y':
            trigedit_save(d);
            sprintf(buf, "OLC: %s edits trigger %d", GET_NAME(d->character), OLC_NUM(d));
            mudlog(buf, CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE);
            /* fall through */
        case 'n':
            cleanup_olc(d, CLEANUP_ALL);
            return;
        case 'a': /* abort quitting */
            break;
        default:
            send_to_char("Invalid choice!\r\n", d->character);
            send_to_char("Do you wish to save the trigger?\r\n", d->character);
            return;
        }
        break;

    case TRIGEDIT_NAME:
        if (OLC_TRIG(d)->name)
            free(OLC_TRIG(d)->name);
        OLC_TRIG(d)->name = strdup((arg && *arg) ? arg : "undefined");
        OLC_VAL(d)++;
        break;

    case TRIGEDIT_INTENDED:
        if ((atoi(arg) >= MOB_TRIGGER) || (atoi(arg) <= WLD_TRIGGER))
            OLC_TRIG(d)->attach_type = atoi(arg);
        OLC_VAL(d)++;
        break;

    case TRIGEDIT_NARG:
        OLC_TRIG(d)->narg = atoi(arg);
        OLC_VAL(d)++;
        break;

    case TRIGEDIT_ARGUMENT:
        OLC_TRIG(d)->arglist = (*arg ? strdup(arg) : NULL);
        OLC_VAL(d)++;
        break;

    case TRIGEDIT_TYPES:
        if ((i = atoi(arg)) == 0)
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
void sprintbits(int data, char *dest) {
    int i;
    char *p = dest;

    for (i = 0; i < 32; i++) {
        if (data & (1 << i)) {
            *p = ((i <= 25) ? ('a' + i) : ('A' + i));
            p++;
        }
    }
    *p = '\0';
}

/* save the zone's triggers to internal memory and to disk */
void trigedit_save(struct descriptor_data *d) {
    int trig_rnum, i;
    int found = 0;
    char *s;
    trig_data *proto;
    trig_data *trig = OLC_TRIG(d);
    trig_data *live_trig;
    struct cmdlist_element *cmd, *next_cmd;
    struct index_data **new_index;
    struct descriptor_data *dsc;
    FILE *trig_file;
    int zone, top;
    char buf[MAX_CMD_LENGTH];
    char bitBuf[MAX_INPUT_LENGTH];
    char fname[MAX_INPUT_LENGTH];
    char logbuf[MAX_INPUT_LENGTH];

    if ((trig_rnum = real_trigger(OLC_NUM(d))) != -1) {
        proto = trig_index[trig_rnum]->proto;
        for (cmd = proto->cmdlist; cmd; cmd = next_cmd) {
            next_cmd = cmd->next;
            if (cmd->cmd)
                free(cmd->cmd);
            free(cmd);
        }

        free(proto->arglist);
        free(proto->name);

        /* Recompile the command list from the new script */
        s = OLC_STORAGE(d);

        CREATE(trig->cmdlist, struct cmdlist_element, 1);
        if (s) {
            trig->cmdlist->cmd = strdup(strtok(s, "\r\n"));
            cmd = trig->cmdlist;
            cmd->next = NULL; /*don't want dangling pointers.. */

            while ((s = strtok(NULL, "\r\n"))) {
                CREATE(cmd->next, struct cmdlist_element, 1);
                cmd = cmd->next;
                cmd->cmd = strdup(s);
                cmd->next = NULL; /*don't want dangling pointers.. */
            }
        } else
            trig->cmdlist->cmd = strdup("* No Script");

        /* make the prorotype look like what we have */
        trig_data_copy(proto, trig);

        /* go through the mud and replace existing triggers         */
        live_trig = trigger_list;
        while (live_trig) {
            if (GET_TRIG_RNUM(live_trig) == trig_rnum) {
                if (live_trig->arglist) {
                    free(live_trig->arglist);
                    live_trig->arglist = NULL;
                }
                if (live_trig->name) {
                    free(live_trig->name);
                    live_trig->name = NULL;
                }

                if (proto->arglist)
                    live_trig->arglist = strdup(proto->arglist);
                if (proto->name)
                    live_trig->name = strdup(proto->name);

                live_trig->cmdlist = proto->cmdlist;
                live_trig->curr_state = live_trig->cmdlist;
                live_trig->trigger_type = proto->trigger_type;
                live_trig->attach_type = proto->attach_type;
                live_trig->narg = proto->narg;
                live_trig->data_type = proto->data_type;
                live_trig->depth = 0;
                live_trig->wait_event = NULL;
                if (GET_TRIG_WAIT(live_trig))
                    event_cancel(GET_TRIG_WAIT(live_trig));
                free_varlist(live_trig->var_list);
            }

            live_trig = live_trig->next_in_world;
        }
    } else {
        /* this is a new trigger */
        CREATE(new_index, struct index_data *, top_of_trigt + 2);

        /* Recompile the command list from the new script */
        s = OLC_STORAGE(d);

        CREATE(trig->cmdlist, struct cmdlist_element, 1);
        if (s) {
            trig->cmdlist->cmd = strdup(strtok(s, "\r\n"));
            cmd = trig->cmdlist;
            cmd->next = NULL; /*don't want dangling pointers.. */

            while ((s = strtok(NULL, "\r\n"))) {
                CREATE(cmd->next, struct cmdlist_element, 1);
                cmd = cmd->next;
                cmd->cmd = strdup(s);
                cmd->next = NULL; /*don't want dangling pointers.. */
            }
        } else
            trig->cmdlist->cmd = strdup("* No Script");

        for (i = 0; i < top_of_trigt; i++) {
            if (!found) {
                if (trig_index[i]->virtual > OLC_NUM(d)) {
                    found = TRUE;
                    trig_rnum = i;

                    CREATE(new_index[trig_rnum], struct index_data, 1);
                    GET_TRIG_RNUM(OLC_TRIG(d)) = trig_rnum;
                    new_index[trig_rnum]->virtual = OLC_NUM(d);
                    new_index[trig_rnum]->number = 0;
                    new_index[trig_rnum]->func = NULL;
                    CREATE(proto, struct trig_data, 1);
                    new_index[trig_rnum]->proto = proto;
                    trig_data_copy(proto, trig);

                    if (trig->name)
                        proto->name = strdup(trig->name);
                    if (trig->arglist)
                        proto->arglist = strdup(trig->arglist);

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
            CREATE(new_index[trig_rnum], struct index_data, 1);
            GET_TRIG_RNUM(OLC_TRIG(d)) = trig_rnum;
            new_index[trig_rnum]->virtual = OLC_NUM(d);
            new_index[trig_rnum]->number = 0;
            new_index[trig_rnum]->func = NULL;

            CREATE(proto, struct trig_data, 1);
            new_index[trig_rnum]->proto = proto;
            trig_data_copy(proto, trig);

            if (trig->name)
                proto->name = strdup(trig->name);
            if (trig->arglist)
                proto->arglist = strdup(trig->arglist);
        }

        free(trig_index);

        trig_index = new_index;
        top_of_trigt++;

        /* HERE IT HAS TO GO THROUGH AND FIX ALL SCRIPTS/TRIGS OF HIGHER RNUM */
        for (live_trig = trigger_list; live_trig; live_trig = live_trig->next_in_world)
            if (GET_TRIG_RNUM(live_trig) > trig_rnum)
                GET_TRIG_RNUM(live_trig)++;

        /*
         * Update other trigs being edited.
         */
        for (dsc = descriptor_list; dsc; dsc = dsc->next)
            if (dsc->connected == CON_TRIGEDIT)
                if (GET_TRIG_RNUM(OLC_TRIG(dsc)) >= trig_rnum)
                    GET_TRIG_RNUM(OLC_TRIG(dsc))++;
    }

    /* now write the trigger out to disk, along with the rest of the  */
    /* triggers for this zone, of course                              */
    /* note: we write this to disk NOW instead of letting the builder */
    /* have control because if we lose this after having assigned a   */
    /* new trigger to an item, we will get SYSERR's upon reboot that  */
    /* could make things hard to debug.                               */

    zone = zone_table[OLC_ZNUM(d)].number;
    top = zone_table[OLC_ZNUM(d)].top;

#ifdef CIRCLE_MAC
    sprintf(fname, "%s:%i.new", TRG_PREFIX, zone);
#else
    sprintf(fname, "%s/%i.new", TRG_PREFIX, zone);
#endif

    if (!(trig_file = fopen(fname, "w"))) {
        sprintf(logbuf, "SYSERR: OLC: Can't open trig file \"%s\"", fname);
        mudlog(logbuf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
        return;
    }

    for (i = zone * 100; i <= top; i++) {
        if ((trig_rnum = real_trigger(i)) != -1) {
            trig = trig_index[trig_rnum]->proto;

            if (fprintf(trig_file, "#%d\n", i) < 0) {
                sprintf(logbuf, "SYSERR: OLC: Can't write trig file!");
                mudlog(logbuf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
                fclose(trig_file);
                return;
            }
            sprintbits(GET_TRIG_TYPE(trig), bitBuf);
            fprintf(trig_file,
                    "%s~\n"
                    "%d %s %d\n"
                    "%s~\n",
                    (GET_TRIG_NAME(trig)) ? (GET_TRIG_NAME(trig)) : "unknown trigger", trig->attach_type, bitBuf,
                    GET_TRIG_NARG(trig), GET_TRIG_ARG(trig) ? GET_TRIG_ARG(trig) : "");

            /* Build the text for the script */
            strcpy(buf, "");
            for (cmd = trig->cmdlist; cmd; cmd = cmd->next) {
                strcat(buf, cmd->cmd);
                strcat(buf, "\r\n");
            }

            if (!buf[0])
                strcpy(buf, "* Empty script");

            fprintf(trig_file, "%s~\n", buf);
            *buf = '\0';
        }
    }

    fprintf(trig_file, "$~\n");
    fclose(trig_file);

#ifdef CIRCLE_MAC
    sprintf(buf, "%s:%d.trg", TRG_PREFIX, zone);
#else
    sprintf(buf, "%s/%d.trg", TRG_PREFIX, zone);
#endif

    rename(fname, buf);
}

void dg_olc_script_free(struct descriptor_data *d) {
    struct trig_proto_list *editscript, *prevscript;

    editscript = OLC_SCRIPT(d);
    while (editscript) {
        prevscript = editscript;
        editscript = editscript->next;
        free(prevscript);
    }
}

void dg_olc_script_copy(struct descriptor_data *d) {
    struct trig_proto_list *origscript, *editscript;

    if (OLC_ITEM_TYPE(d) == MOB_TRIGGER)
        origscript = OLC_MOB(d)->proto_script;
    else if (OLC_ITEM_TYPE(d) == OBJ_TRIGGER)
        origscript = OLC_OBJ(d)->proto_script;
    else
        origscript = OLC_ROOM(d)->proto_script;

    if (origscript) {
        CREATE(editscript, struct trig_proto_list, 1);
        OLC_SCRIPT(d) = editscript;

        while (origscript) {
            editscript->vnum = origscript->vnum;
            origscript = origscript->next;
            if (origscript)
                CREATE(editscript->next, struct trig_proto_list, 1);
            editscript = editscript->next;
        }
    } else
        OLC_SCRIPT(d) = NULL;
}

void dg_script_menu(struct descriptor_data *d) {
    struct trig_proto_list *editscript;
    int i = 0;

    /* make sure our input parser gets used */
    OLC_MODE(d) = OLC_SCRIPT_EDIT;
    OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;

#if defined(CLEAR_SCREEN) /* done wierd to compile with the vararg send() */

    /* Hey does that line entionally have those control characters? RSD */

#define FMT "[H[J     Script Editor\r\n\r\n     Trigger List:\r\n"
#else
#define FMT "     Script Editor\r\n\r\n     Trigger List:\r\n"
#endif
    send_to_char(FMT, d->character);
#undef FMT

    editscript = OLC_SCRIPT(d);
    while (editscript) {
        sprintf(buf, "     %2d) [%s%d%s] %s%s%s", ++i, cyn, editscript->vnum, nrm, cyn,
                trig_index[real_trigger(editscript->vnum)]->proto->name, nrm);
        send_to_char(buf, d->character);
        if (trig_index[real_trigger(editscript->vnum)]->proto->attach_type != OLC_ITEM_TYPE(d))
            sprintf(buf, "   %s** Mis-matched Trigger Type **%s\r\n", grn, nrm);
        else
            sprintf(buf, "\r\n");
        send_to_char(buf, d->character);

        editscript = editscript->next;
    }
    if (i == 0)
        send_to_char("     <none>\r\n", d->character);

    sprintf(buf,
            "\r\n"
            " %sN%s)  New trigger for this script\r\n"
            " %sD%s)  Delete a trigger in this script\r\n"
            " %sX%s)  Exit Script Editor\r\n\r\n"
            "     Enter choice:\r\n",
            grn, nrm, grn, nrm, grn, nrm);
    send_to_char(buf, d->character);
}

int dg_script_edit_parse(struct descriptor_data *d, char *arg) {
    struct trig_proto_list *trig, *currtrig, *starttrig;
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
            send_to_char("\r\nPlease enter trigger vnum: ", d->character);
            OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_NEW_TRIGGER;
            break;
        case 'd':
        case 'D':
            if (OLC_SCRIPT(d) == (struct trig_proto_list *)NULL)
                send_to_char("Cannot delete a trigger as there are none!\r\n", d->character);
            else {
                send_to_char("     Which entry should be deleted?  0 to abort:\r\n", d->character);
                OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_DEL_TRIGGER;
            }
            break;
        default:
            send_to_char("\r\nUnrecognized command.  Try again:\r\n", d->character);
            break;
        }
        return 1;

    case SCRIPT_NEW_TRIGGER:
        vnum = -1;
        vnum = atoi(arg);

        if (vnum <= 0)
            break; /* this aborts a new trigger entry */

        if (real_trigger(vnum) < 0) {
            send_to_char("Invalid Trigger VNUM!\r\n"
                         "Please enter vnum:\r\n",
                         d->character);
            return 1;
        }

        CREATE(trig, struct trig_proto_list, 1);
        trig->vnum = vnum;
        trig->next = (struct trig_proto_list *)NULL;

        /* add the new info in position */
        if (OLC_SCRIPT(d) == (struct trig_proto_list *)NULL)
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
            send_to_char("No such trigger!\r\nTry Again:\r\n", d->character);
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
 * Formats and indents a script.  Returns TRUE if the format was
 * successful.  Returns FALSE if the format failed--in which case
 * the original text is not replaced.
 */
bool format_script(struct descriptor_data *d, int indent_quantum) {
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
        snprintf(error, sizeof(error) - 1, msg " (line %d)!\r\n", line_num);                                           \
        write_to_output(error, d);                                                                                     \
    } while (0)
#define ABORT(msg)                                                                                                     \
    do {                                                                                                               \
        COMPLAIN(msg);                                                                                                 \
        free(script);                                                                                                  \
        return FALSE;                                                                                                  \
    } while (0)

    char out[MAX_CMD_LENGTH], *line, *script, error[100];
    int line_num = 0, indent = 0, len = 0, max_len, i, indent_next = 0, needed;
    struct scope {
        int scope;
        int line_num;
    } scope;
    array_stack(struct scope) stack;
    struct scope_types {
        int type;
        char *name;
        int length;
    } cmd_scopes[] = {
        {CS_IF, "if ", 3},       {CS_SWITCH, "switch ", 7}, {CS_CASE, "case", 4}, {CS_DEFAULT, "default", 7},
        {CS_WHILE, "while ", 6}, {CS_ELSEIF, "elseif ", 7}, /* needs to come before else check */
        {CS_ELSE, "else", 4},    {CS_NONE, NULL, 0},
    };

    if (!d->str || !*d->str)
        return FALSE;

    script = strdup(*d->str); /* Make a copy, because of strtok() */
    *out = '\0';
    max_len = MAX(d->max_str, sizeof(out));
    scope.scope = CS_NONE;
    scope.line_num = 0;
    as_init(stack, 10, scope);

    /* Iterate through the script line by line */
    for (line = strtok(script, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        ++line_num;
        skip_spaces(&line);

        /* Does this line open a new scope? */
        for (i = 0; cmd_scopes[i].name; ++i)
            if (!strn_cmp(line, cmd_scopes[i].name, cmd_scopes[i].length))
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
            if (!strn_cmp(line, "end", 3)) {
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
            } else if (!strn_cmp(line, "done", 4)) {
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
            } else if (!strn_cmp(line, "break", 5)) {
                array_stack(struct scope) temp;
                as_init(temp, as_size(stack), as_null(stack));
                /* Search the stack for a while, case, or default */
                for (i = FALSE; !as_empty(stack);) {
                    as_push(temp, as_pop(stack));
                    if (as_peek(temp).scope == CS_WHILE || as_peek(temp).scope == CS_CASE ||
                        as_peek(temp).scope == CS_DEFAULT) {
                        i = TRUE; /* found */
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

        /* How much space do we need for this line?  Include indent and \r\n */
        needed = strnlen(line, max_len - len + 1) + indent + 2;
        if (len + needed >= max_len)
            ABORT("String too long, formatting aborted");

        /* Indent, then copy line */
        for (i = 0; i < indent; ++i)
            out[len++] = ' ';
        len += sprintf(out + len, "%s\r\n", line); /* size checked above */

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

    return TRUE;

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

/***************************************************************************
 * $Log: dg_olc.c,v $
 * Revision 1.26  2009/01/17 00:28:02  myc
 * Fix possible use of uninitialized variable.
 *
 * Revision 1.25  2008/08/18 01:35:38  jps
 * Replaced all \\n\\r with \\r\\n, not that it was really necessary...
 *
 * Revision 1.24  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.23  2008/04/05 18:58:54  jps
 * FINALLY fixed that "Numberic Arg" silliness.
 *
 * Revision 1.22  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.21  2008/03/22 20:12:42  myc
 * Fixed indentation with else/elseif, and handling of 'end'.
 *
 * Revision 1.20  2008/03/22 20:05:59  myc
 * Fixed bug in script formatter: need to pop off if/else if before putting
 * another one on stack.
 *
 * Revision 1.19  2008/03/22 19:50:24  myc
 * Rewrote the script formatter to use a stack.  It is now leet haxorz.
 *
 * Revision 1.18  2008/03/22 03:22:38  myc
 * All invocations of the string editor now go through string_write()
 * instead of messing with the descriptor variables itself.  Also added
 * a toggle, LineNums, to decide whether to do /l or /n when entering
 * the string editor.
 *
 * Revision 1.17  2008/03/21 15:58:34  myc
 * Added a utility format scripts.
 *
 * Revision 1.16  2008/03/17 16:22:42  myc
 * Removing the 'script_copy' function, which was unnecessary and caused
 * confusion with dg_olc_script_copy.
 *
 * Revision 1.15  2008/02/16 20:26:04  myc
 * Moving free_varlist to header file.
 *
 * Revision 1.14  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.13  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.12  2008/01/17 01:29:10  myc
 * Took out the restriction that comments out mskillset.
 *
 * Revision 1.11  2007/07/24 23:02:52  jps
 * Minor typo fix.
 *
 * Revision 1.10  2007/04/18 00:34:11  myc
 * The script editor in medit, oedit, and redit will now give better
 * feedback when you enter a command it doesn't recognize.
 *
 * Revision 1.9  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.8  2000/11/28 01:18:21  mtp
 * replaceed dg_event.c code with events.c code
 *
 * Revision 1.7  2000/11/23 00:57:04  mtp
 * added mskillset which allows a mob to set a players skill (or spell)
 *proficiency this required a change to not allow lev 102 and below to save a
 *trigger with mskillset in it so any lines with the text mskillset are
 *commented
 *
 * Revision 1.6  2000/11/21 03:51:01  rsd
 * Altered the comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.5  2000/10/14 11:12:40  mtp
 * fixed the addition/removal of triggers from a script on
 * mob/obj/room, there was some dodgy freeing going on which has
 * been removed. HOPE there should be no memory leaks...:-)
 *
 * Revision 1.4  2000/03/07 07:42:05  mtp
 * In save of trigger, the memory structure to represent a sequence of command
 *lists is created, and this was not initialising the 'next' pointers to NULL. I
 *have done this now, to avoid problems when we follow the linked list freeing
 *the commands.
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/01/31 01:08:28  mud
 * Altered comment header and indented entire file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/
