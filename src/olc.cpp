/***************************************************************************
 *   File: olc.c                                          Part of FieryMUD *
 *  Usage:                                                                 *
 *     By: Harvey Gilpin of TwyliteMud                                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  Copyright 1996 Harvey Gilpin.                                          *
 ***************************************************************************/

#define _OASIS_OLC_

#include "olc.hpp"

#include "casting.hpp"
#include "comm.hpp"
#include "commands.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "dg_olc.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "math.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/*. External functions .*/
extern void zedit_setup(descriptor_data *d, int room_num);
extern void zedit_save_to_disk(int zone);
extern void zedit_new_zone(char_data *ch, int new_zone);
extern void medit_setup_new(descriptor_data *d);
extern void medit_setup_existing(descriptor_data *d, int rmob_num);
extern void medit_save_to_disk(int zone);
extern void redit_setup_new(descriptor_data *d);
extern void redit_setup_existing(descriptor_data *d, int rroom_num);
extern void redit_save_to_disk(int zone);
extern void oedit_setup_new(descriptor_data *d);
extern void oedit_setup_existing(descriptor_data *d, int robj_num);
extern void oedit_reverse_exdescs(int zone, char_data *ch);
int oedit_reverse_exdesc(int real_num, char_data *ch);
extern void oedit_save_to_disk(int zone);
extern void sedit_setup_new(descriptor_data *d);
extern void sedit_setup_existing(descriptor_data *d, int robj_num);
extern void sedit_save_to_disk(int zone);
extern void sdedit_setup_new(descriptor_data *d);
extern void sdedit_setup_existing(descriptor_data *d, int robj_num);
extern void hedit_save_to_disk(descriptor_data *d);
extern int real_shop(int vnum);
extern void free_shop(shop_data *shop);
extern void free_room(room_data *room);
extern void medit_free_mobile(char_data *mob);
extern void free_help(help_index_element *help);
extern int find_help(char *keyword);
extern void trigedit_setup_new(descriptor_data *d);
extern void trigedit_setup_existing(descriptor_data *d, int rtrg_num);
extern int real_trigger(int vnum);

/*. Internal function prototypes .*/
void olc_saveinfo(char_data *ch);

/*. Internal data .*/

const char *save_info_msg[5] = {"Rooms", "Objects", "Zone info", "Mobiles", "Shops"};

/*
 * Internal data structures.
 */

struct olc_scmd_data {
    char *text;
    int con_type;
};

struct olc_scmd_data olc_scmd_info[] = {{"room", CON_REDIT},      {"object", CON_OEDIT}, {"room", CON_ZEDIT},
                                        {"mobile", CON_MEDIT},    {"shop", CON_SEDIT},   {"help", CON_HEDIT},
                                        {"trigger", CON_TRIGEDIT}};

bool has_olc_access(char_data *ch, zone_vnum zone) {
    struct olc_zone_list *list;
    for (list = GET_OLC_ZONES(ch); list; list = list->next)
        if (list->zone == zone)
            return TRUE;
    return FALSE;
}

void free_olc_zone_list(char_data *ch) {
    struct olc_zone_list *list, *next;
    for (list = GET_OLC_ZONES(ch); list; list = next) {
        next = list->next;
        free(list);
    }
}

void olc_delete(char_data *ch, int subcmd, int vnum) {
    char *ename;
    int rnum;

    switch (subcmd) {
    case SCMD_OLC_ZEDIT:
        ename = "zone";
        break;
    case SCMD_OLC_REDIT:
        ename = "room";
        break;
    case SCMD_OLC_MEDIT:
        /* delete mobile */
        if ((rnum = real_mobile(vnum)) == NOTHING) {
            send_to_char("There is no such mobile.\r\n", ch);
            return;
        }
        sprintf(buf, "OLC: %s deletes mobile #%d.", GET_NAME(ch), vnum);
        if (delete_mobile(rnum)) {
            sprintf(buf, "Mobile %d deleted.\r\n", vnum);
            send_to_char(buf, ch);
        } else {
            send_to_char("ERROR.  Mobile not deleted.\r\n", ch);
        }
        return;
        break;
    case SCMD_OLC_OEDIT:
        /* delete object */
        if ((rnum = real_object(vnum)) == NOTHING) {
            send_to_char("There is no such object.\r\n", ch);
            return;
        }
        sprintf(buf, "OLC: %s deletes object #%d.", GET_NAME(ch), vnum);
        if (delete_object(rnum)) {
            sprintf(buf, "Object %d deleted.\r\n", vnum);
            send_to_char(buf, ch);
        } else {
            send_to_char("ERROR.  Object not deleted.\r\n", ch);
        }
        return;
        break;
    case SCMD_OLC_TRIGEDIT:
        ename = "trigger";
        break;
    default:
        ename = "those";
    }

    cprintf(ch, "You can't delete %ss.\r\n", ename);
}

/*------------------------------------------------------------*\
 Eported ACMD do_olc function

 This function is the OLC interface.   It deals with all the
 generic OLC stuff, then passes control to the sub-olc sections.
\*------------------------------------------------------------*/

#define OLC_ACTION_NONE -1
#define OLC_ACTION_VNUM 0
#define OLC_ACTION_SAVE 1

ACMD(do_olc) {
    int number = -1, real_num, action = OLC_ACTION_NONE;
    struct descriptor_data *d;

    if (IS_NPC(ch))
        /*. No screwing arround . */
        return;

    if (subcmd == SCMD_OLC_SAVEINFO) {
        olc_saveinfo(ch);
        return;
    }

    /*. Parse any arguments . */
    two_arguments(argument, buf1, buf2);
    if (!*buf1) { /* No argument given. */
        switch (subcmd) {
        case SCMD_OLC_ZEDIT:
        case SCMD_OLC_REDIT:
            number = world[IN_ROOM(ch)].vnum;
            break;
        case SCMD_OLC_TRIGEDIT:
        case SCMD_OLC_OEDIT:
        case SCMD_OLC_MEDIT:
        case SCMD_OLC_SEDIT:
            sprintf(buf, "Specify a %s VNUM to edit.\r\n", olc_scmd_info[subcmd].text);
            send_to_char(buf, ch);
            return;
        case SCMD_OLC_HEDIT:
            send_to_char("Specify a help topic to edit.\r\n", ch);
            return;
        case SCMD_OLC_SDEDIT:
            send_to_char("Specify a spell name to edit.\r\n", ch);
            return;
        }
    }

    if (!isdigit(*buf1)) {
        if (strn_cmp("save", buf1, 4) == 0) {
            if (!*buf2) {
                if (subcmd == SCMD_OLC_HEDIT) {
                    action = OLC_ACTION_SAVE;
                    number = 0;
                } else if (GET_OLC_ZONES(ch)) {
                    action = OLC_ACTION_SAVE;
                    number = (GET_OLC_ZONES(ch)->zone * 100);
                } else {
                    send_to_char("Save which zone?\r\n", ch);
                    return;
                }
            } else {
                action = OLC_ACTION_SAVE;
                number = atoi(buf2) * 100;
            }
        } else if (!strn_cmp("del", buf1, 4)) {

#ifdef PRODUCTION
            send_to_char("Don't delete things in the production mud please!\r\n", ch);
            return;
#endif

            if (!*buf2) {
                send_to_char("Delete which entity?\r\n", ch);
                return;
            } else if (!isdigit(*buf2)) {
                send_to_char("Please supply the vnum of the entity to be deleted.\r\n", ch);
                return;
            }
            number = atoi(buf2);
            olc_delete(ch, subcmd, number);
            return;
        } else if (subcmd == SCMD_OLC_HEDIT) {
            number = 0;

            /* REVEX - reverse extra descs. A measure that will only be needed for
             *             a short while, to reverse the order of extra descriptions
             *             on objects that got reversed on load by now-supplanted code
             *             in db.c */

        } else if (subcmd == SCMD_OLC_OEDIT && strn_cmp("revex", buf1, 5) == 0) {
            if (!*buf2) {
                send_to_char("Reverse extra descs for which object?\r\n", ch);
                return;
            }

            number = atoi(buf2);
            if ((real_num = real_object(number)) < 0) {
                send_to_char("There is no such object.\r\n", ch);
                return;
            }

            sprintf(buf, "OLC: %s reverses object extra descs for %d, %s.", GET_NAME(ch), number,
                    obj_proto[real_num].short_description);
            mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
            oedit_reverse_exdesc(real_num, NULL);
            return;

        } else if (subcmd == SCMD_OLC_OEDIT && strn_cmp("zrevex", buf1, 6) == 0) {
            if (!*buf2) {
                send_to_char("Reverse object extra descs in which zone?\r\n", ch);
                return;
            }

            number = atoi(buf2) * 100;
            if ((real_num = find_real_zone_by_room(number)) == -1) {
                send_to_char("Sorry, there is no zone for that number!\r\n", ch);
                return;
            }

            sprintf(buf, "OLC: %s reverses object extra descs for zone %d.", GET_NAME(ch), zone_table[real_num].number);
            mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
            oedit_reverse_exdescs(real_num, ch);
            return;

        } else if (subcmd == SCMD_OLC_ZEDIT && GET_LEVEL(ch) >= LVL_HEAD_B && *buf1) {
            if ((strn_cmp("new", buf1, 3) == 0) && *buf2)
                zedit_new_zone(ch, atoi(buf2));
            else
                send_to_char("Specify a zone number.\r\n", ch);
            return;
        } else if (subcmd != SCMD_OLC_SDEDIT && subcmd != SCMD_OLC_ZEDIT && subcmd != SCMD_OLC_REDIT) {
            send_to_char("Yikes!   Stop that, someone will get hurt!\r\n", ch);
            return;
        }
    }

    /*. If a numeric argument was given, get it . */
    if (subcmd == SCMD_OLC_HEDIT)
        number = HEDIT_PERMISSION;
    else if (number == -1)
        number = atoi(buf1);

    /*. Check whatever it is isn't already being edited . */
    for (d = descriptor_list; d; d = d->next)
        if (d->connected == olc_scmd_info[subcmd].con_type)
            if (d->olc && OLC_NUM(d) == number && number != NOTHING) {
                if (subcmd == SCMD_OLC_HEDIT)
                    sprintf(buf, "Help files are currently being edited by %s\r\n",
                            (CAN_SEE(ch, d->character) ? GET_NAME(d->character) : "someone"));
                else
                    sprintf(buf, "That %s is currently being edited by %s.\r\n", olc_scmd_info[subcmd].text,
                            GET_NAME(d->character));
                send_to_char(buf, ch);
                return;
            }
    d = ch->desc;

    /*. Give descriptor an OLC struct . */
    CREATE(d->olc, olc_data, 1);

    /*
     * Find the zone.
     */
    if ((subcmd != SCMD_OLC_HEDIT) && (subcmd != SCMD_OLC_SDEDIT)) {
        if ((OLC_ZNUM(d) = find_real_zone_by_room(number)) == -1) {
            send_to_char("Sorry, there is no zone for that number!\r\n", ch);
            free(d->olc);
            return;
        }
    }
    /*
     * Everyone but IMPLs can only edit zones they have been assigned.
     */
    if (subcmd == SCMD_OLC_HEDIT && GET_LEVEL(ch) < LVL_GOD && !has_olc_access(ch, HEDIT_PERMISSION)) {
        send_to_char("You do not have permission to edit help files.\r\n", ch);
        free(d->olc);
        return;
    } else if (GET_LEVEL(ch) < LVL_GRGOD && !has_olc_access(ch, zone_table[OLC_ZNUM(d)].number)) {
        send_to_char("You do not have permission to edit this zone.\r\n", ch);
        free(d->olc);
        return;
    }
    if (action == OLC_ACTION_SAVE) {
        const char *type = NULL;

        switch (subcmd) {
        case SCMD_OLC_REDIT:
            type = "room";
            break;
        case SCMD_OLC_ZEDIT:
            type = "zone";
            break;
        case SCMD_OLC_SEDIT:
            type = "shop";
            break;
        case SCMD_OLC_MEDIT:
            type = "mobile";
            break;
        case SCMD_OLC_OEDIT:
            type = "object";
            break;
        case SCMD_OLC_HEDIT:
            type = "help";
            break;
        }
        if (!type) {
            send_to_char("Oops, I forgot what you wanted to save.\r\n", ch);
            return;
        }
        sprintf(buf, "Saving all %ss in zone %d.\r\n", type, zone_table[OLC_ZNUM(d)].number);
        send_to_char(buf, ch);
        sprintf(buf, "OLC: %s saves %s info for zone %d.", GET_NAME(ch), type, zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

        switch (subcmd) {
        case SCMD_OLC_REDIT:
            redit_save_to_disk(OLC_ZNUM(d));
            break;
        case SCMD_OLC_ZEDIT:
            zedit_save_to_disk(OLC_ZNUM(d));
            break;
        case SCMD_OLC_OEDIT:
            oedit_save_to_disk(OLC_ZNUM(d));
            break;
        case SCMD_OLC_MEDIT:
            medit_save_to_disk(OLC_ZNUM(d));
            break;
        case SCMD_OLC_SEDIT:
            sedit_save_to_disk(OLC_ZNUM(d));
            break;
        case SCMD_OLC_HEDIT:
            hedit_save_to_disk(d);
            break;
        }
        free(d->olc);
        return;
    }

    if (subcmd != SCMD_OLC_HEDIT)
        OLC_NUM(d) = number;
    else {
        OLC_NUM(d) = HEDIT_PERMISSION;
        OLC_STORAGE(d) = strdup(buf1);
        for (OLC_ZNUM(d) = 0; (OLC_ZNUM(d) <= top_of_helpt); OLC_ZNUM(d)++) {
            if (is_abbrev(OLC_STORAGE(d), help_table[OLC_ZNUM(d)].keyword))
                break;
        }
        if (OLC_ZNUM(d) > top_of_helpt) {
            if (find_help(OLC_STORAGE(d)) > NOTHING) {
                cleanup_olc(d, CLEANUP_ALL);
                send_to_char("That help topic already exists.\r\n", ch);
                return;
            }
            sprintf(buf, "Do you wish to add a help topic on '%s'? ", OLC_STORAGE(d));
            send_to_char(buf, ch);
            OLC_MODE(d) = HEDIT_CONFIRM_ADD;
        } else {
            sprintf(buf, "Do you wish to edit the help topic on '%s'? ", help_table[OLC_ZNUM(d)].keyword);
            send_to_char(buf, ch);
            OLC_MODE(d) = HEDIT_CONFIRM_EDIT;
        }
    }

    /*
     * Steal player's descriptor start up subcommands.
     */
    switch (subcmd) {
    case SCMD_OLC_TRIGEDIT:
        if ((real_num = real_trigger(number)) >= 0)
            trigedit_setup_existing(d, real_num);
        else
            trigedit_setup_new(d);
        STATE(d) = CON_TRIGEDIT;
        break;
    case SCMD_OLC_SDEDIT:
        real_num = find_spell_num(argument);
        if ((real_num < 0) || (real_num > TOP_SKILL) || (!str_cmp("!UNUSED!", skills[real_num].name))) {
            send_to_char("Your spell could not be found.\r\n", ch);
            return;
        }
        sdedit_setup_existing(d, real_num);
        STATE(d) = CON_SDEDIT;
        break;
    case SCMD_OLC_REDIT:
        if ((real_num = real_room(number)) >= 0)
            redit_setup_existing(d, real_num);
        else
            redit_setup_new(d);
        STATE(d) = CON_REDIT;
        break;
    case SCMD_OLC_ZEDIT:
        if ((real_num = real_room(number)) < 0) {
            send_to_char("That room does not exist.\r\n", ch);
            free(d->olc);
            return;
        }
        zedit_setup(d, real_num);
        STATE(d) = CON_ZEDIT;
        break;
    case SCMD_OLC_MEDIT:
        if ((real_num = real_mobile(number)) < 0)
            medit_setup_new(d);
        else
            medit_setup_existing(d, real_num);
        STATE(d) = CON_MEDIT;
        break;
    case SCMD_OLC_OEDIT:
        if ((real_num = real_object(number)) >= 0)
            oedit_setup_existing(d, real_num);
        else
            oedit_setup_new(d);
        STATE(d) = CON_OEDIT;
        break;
    case SCMD_OLC_SEDIT:
        if ((real_num = real_shop(number)) >= 0)
            sedit_setup_existing(d, real_num);
        else
            sedit_setup_new(d);
        STATE(d) = CON_SEDIT;
        break;
    case SCMD_OLC_HEDIT:
        STATE(d) = CON_HEDIT;
        break;
    }
    act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
    SET_FLAG(PLR_FLAGS(ch), PLR_WRITING);
}

/*------------------------------------------------------------*\
 Internal utlities
\*------------------------------------------------------------*/

void olc_saveinfo(char_data *ch) {
    struct olc_save_info *entry;

    if (olc_save_list)
        send_to_char("The following OLC components need saving:-\r\n", ch);
    else
        send_to_char("The database is up to date.\r\n", ch);

    for (entry = olc_save_list; entry; entry = entry->next) {
        sprintf(buf, " - %s for zone %d.\r\n", save_info_msg[(int)entry->type], entry->zone);
        send_to_char(buf, ch);
    }
}

/*------------------------------------------------------------*\
   Exported utlities
\*------------------------------------------------------------*/

/*. Add an entry to the 'to be saved' list .*/

void olc_add_to_save_list(int zone, byte type) {
    struct olc_save_info *new;

    /*. Return if it's already in the list . */
    for (new = olc_save_list; new; new = new->next)
        if ((new->zone == zone) && (new->type == type))
            return;

    CREATE(new, olc_save_info, 1);
    new->zone = zone;
    new->type = type;
    new->next = olc_save_list;
    olc_save_list = new;
}

/*. Remove an entry from the 'to be saved' list .*/

void olc_remove_from_save_list(int zone, byte type) {
    struct olc_save_info **entry;
    struct olc_save_info *temp;

    for (entry = &olc_save_list; *entry; entry = &(*entry)->next)
        if (((*entry)->zone == zone) && ((*entry)->type == type)) {
            temp = *entry;
            *entry = temp->next;
            free(temp);
            return;
        }
}

/*. Set the colour string pointers for that which this char will
   see at color level NRM.   Changing the entries here will change
   the colour scheme throught the OLC.*/

void get_char_cols(char_data *ch) {
    nrm = CLR(ch, ANRM);
    grn = CLR(ch, FGRN);
    cyn = CLR(ch, FCYN);
    yel = CLR(ch, FYEL);
    blk = CLR(ch, FBLK);
    red = CLR(ch, FRED);
}

/*. This procedure removes the '\r\n' from a string so that it may be
   saved to a file.   Use it only on buffers, not on the oringinal
   strings.*/

void strip_string(char *buffer) {
    char *ptr, *str;

    ptr = buffer;
    str = ptr;

    while ((*str = *ptr)) {
        str++;
        ptr++;
        if (*ptr == '\r')
            ptr++;
    }
}

/*. This procdure frees up the strings and/or the structures
   attatched to a descriptor, sets all flags back to how they
   should be .*/

void cleanup_olc(descriptor_data *d, byte cleanup_type) {
    if (d->olc) {
        /*
         * Check for a room.
         */
        if (OLC_ROOM(d)) {
            /*
             * free_room doesn't perform sanity checks, must be careful here.
             */
            switch (cleanup_type) {
            case CLEANUP_ALL:
                free_room(OLC_ROOM(d)); /* fall through */
                dg_olc_script_free(d);
            case CLEANUP_STRUCTS:
                free(OLC_ROOM(d));
                break;
            default: /* The caller has screwed up. */
                break;
            }
        }
        /*
         * Check for an object.
         */
        if (OLC_OBJ(d)) {
            /*
             * free_obj() makes sure strings aern't part of the prototype.
             */
            if (cleanup_type == CLEANUP_ALL)
                dg_olc_script_free(d);
            free_obj(OLC_OBJ(d));
        }

        if (OLC_IOBJ(d)) {
            OLC_IOBJ(d)->next = object_list;
            object_list = OLC_IOBJ(d);
            if (d->character)
                obj_to_char(OLC_IOBJ(d), d->character);
        }

        /*
         * Check for a mob.
         */
        if (OLC_MOB(d)) {
            /*
             * medit_free_mobile() makes sure strings are not in the prototype.
             */
            if (cleanup_type == CLEANUP_ALL)
                dg_olc_script_free(d);
            medit_free_mobile(OLC_MOB(d));
        }

        /*
         * Check for a zone.
         */
        if (OLC_ZONE(d)) {
            /*
             * cleanup_type is irrelevant here, free() everything.
             */
            free(OLC_ZONE(d)->name);
            free(OLC_ZONE(d)->cmd);
            free(OLC_ZONE(d));
        }

        /*
         * Check for a shop.
         */
        if (OLC_SHOP(d)) {
            /*
             * free_shop doesn't perform sanity checks, we must be careful here.
             */
            switch (cleanup_type) {
            case CLEANUP_ALL:
                free_shop(OLC_SHOP(d));
                break;
            case CLEANUP_STRUCTS:
                free(OLC_SHOP(d));
                break;
            default: /* The caller has screwed up. */
                break;
            }
        }

        if (OLC_HELP(d)) {
            switch (cleanup_type) {
            case CLEANUP_ALL:
                free_help(OLC_HELP(d));
                break;
            case CLEANUP_STRUCTS:
                free(OLC_HELP(d));
                break;
            default:
                /* Caller has screwed up */
                break;
            }
        }
        if (OLC_SD(d)) {
            if (OLC_SD(d)->note)
                free(OLC_SD(d)->note);
            free(OLC_SD(d));
        }

        if (OLC_GROUP(d)) {
            if (OLC_GROUP(d)->commands)
                free(OLC_GROUP(d)->commands);
            free(OLC_GROUP(d)->alias);
            free(OLC_GROUP(d)->name);
            free(OLC_GROUP(d)->description);
            free(OLC_GROUP(d));
        }

        /*
         * Restore descriptor playing status.
         */
        if (d->character) {
            REMOVE_FLAG(PLR_FLAGS(d->character), PLR_WRITING);
            STATE(d) = CON_PLAYING;
            act("$n stops using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
        }
        free(d->olc);
        d->olc = NULL;
    }
}

void free_save_list() {
    struct olc_save_info *sld, *next_sld;

    for (sld = olc_save_list; sld; sld = next_sld) {
        next_sld = sld->next;
        free(sld);
    }
}