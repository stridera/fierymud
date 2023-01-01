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
#include "logging.hpp"
#include "math.hpp"
#include "screen.hpp"
#include "shop.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

const char *nrm, *grn, *cyn, *yel, *blk, *red;
OLCSaveInfo *olc_save_list;
const char *save_info_msg[5] = {"Rooms", "Objects", "Zone info", "Mobiles", "Shops"};
OLCSCommandData olc_scmd_info[] = {{"room", CON_REDIT},      {"object", CON_OEDIT}, {"room", CON_ZEDIT},
                                   {"mobile", CON_MEDIT},    {"shop", CON_SEDIT},   {"help", CON_HEDIT},
                                   {"trigger", CON_TRIGEDIT}};

//  External functions
void zedit_setup(DescriptorData *d, int room_num);
void zedit_save_to_disk(int zone);
void zedit_new_zone(CharData *ch, int new_zone);
void medit_setup_new(DescriptorData *d);
void medit_setup_existing(DescriptorData *d, int rmob_num);
void medit_save_to_disk(int zone);
void redit_setup_new(DescriptorData *d);
void redit_setup_existing(DescriptorData *d, int rroom_num);
void redit_save_to_disk(int zone);
void oedit_setup_new(DescriptorData *d);
void oedit_setup_existing(DescriptorData *d, int robj_num);
void oedit_reverse_exdescs(int zone, CharData *ch);
int oedit_reverse_exdesc(int real_num, CharData *ch);
void oedit_save_to_disk(int zone);
void sedit_setup_new(DescriptorData *d);
void sedit_setup_existing(DescriptorData *d, int robj_num);
void sedit_save_to_disk(int zone);
void sdedit_setup_new(DescriptorData *d);
void sdedit_setup_existing(DescriptorData *d, int robj_num);
void hedit_save_to_disk(DescriptorData *d);
int real_shop(int vnum);
void free_shop(ShopData *shop);
void free_room(RoomData *room);
void medit_free_mobile(CharData *mob);
void free_help(HelpIndexElement *help);
int find_help(char *keyword);
void trigedit_setup_new(DescriptorData *d);
void trigedit_setup_existing(DescriptorData *d, int rtrg_num);
int real_trigger(int vnum);

/*. Internal function prototypes .*/
void olc_saveinfo(CharData *ch);

/*. Internal data .*/

bool has_olc_access(CharData *ch, zone_vnum zone) {
    OLCZoneList *list;
    for (list = GET_OLC_ZONES(ch); list; list = list->next)
        if (list->zone == zone)
            return true;
    return false;
}

void free_olc_zone_list(CharData *ch) {
    OLCZoneList *list, *next;
    for (list = GET_OLC_ZONES(ch); list; list = next) {
        next = list->next;
        free(list);
    }
}

void olc_delete(CharData *ch, int subcmd, int vnum) {
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
            char_printf(ch, "There is no such mobile.\n");
            return;
        }
        sprintf(buf, "OLC: %s deletes mobile #%d.", GET_NAME(ch), vnum);
        if (delete_mobile(rnum)) {
            sprintf(buf, "Mobile %d deleted.\n", vnum);
            char_printf(ch, buf);
        } else {
            char_printf(ch, "ERROR.  Mobile not deleted.\n");
        }
        return;
        break;
    case SCMD_OLC_OEDIT:
        /* delete object */
        if ((rnum = real_object(vnum)) == NOTHING) {
            char_printf(ch, "There is no such object.\n");
            return;
        }
        sprintf(buf, "OLC: %s deletes object #%d.", GET_NAME(ch), vnum);
        if (delete_object(rnum)) {
            sprintf(buf, "Object %d deleted.\n", vnum);
            char_printf(ch, buf);
        } else {
            char_printf(ch, "ERROR.  Object not deleted.\n");
        }
        return;
        break;
    case SCMD_OLC_TRIGEDIT:
        ename = "trigger";
        break;
    default:
        ename = "those";
    }

    char_printf(ch, "You can't delete {}s.\n", ename);
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
    DescriptorData *d;

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
            sprintf(buf, "Specify a %s VNUM to edit.\n", olc_scmd_info[subcmd].text);
            char_printf(ch, buf);
            return;
        case SCMD_OLC_HEDIT:
            char_printf(ch, "Specify a help topic to edit.\n");
            return;
        case SCMD_OLC_SDEDIT:
            char_printf(ch, "Specify a spell name to edit.\r\n");
            return;
        case SCMD_OLC_OCOPY:
        case SCMD_OLC_ZCOPY:
        case SCMD_OLC_MCOPY:
        case SCMD_OLC_SCOPY:
        case SCMD_OLC_TRIGCOPY:
            char_printf(ch, "Specify a source and target VNUM.\r\n");
            return;
        }
    }

    if (!*buf1) { /* No argument given. */
        switch (subcmd) {
        case SCMD_OLC_REDIT:
        case SCMD_OLC_OCOPY:
        case SCMD_OLC_ZCOPY:
        case SCMD_OLC_MCOPY:
        case SCMD_OLC_SCOPY:
        case SCMD_OLC_TRIGCOPY:
            char_printf(ch, "Specify a source and target VNUM.\r\n");
            return;
        }
    }

    if (!isdigit(*buf1)) {
        if (strncasecmp("save", buf1, 4) == 0) {
            if (!*buf2) {
                if (subcmd == SCMD_OLC_HEDIT) {
                    action = OLC_ACTION_SAVE;
                    number = 0;
                } else if (GET_OLC_ZONES(ch)) {
                    action = OLC_ACTION_SAVE;
                    number = (GET_OLC_ZONES(ch)->zone * 100);
                } else {
                    char_printf(ch, "Save which zone?\n");
                    return;
                }
            } else {
                action = OLC_ACTION_SAVE;
                number = atoi(buf2) * 100;
            }
        } else if (!strncasecmp("del", buf1, 4)) {
            if (environment == ENV_PROD) {
                char_printf(ch, "Don't delete things in the production mud please!\n");
                return;
            }

            if (!*buf2) {
                char_printf(ch, "Delete which entity?\n");
                return;
            } else if (!isdigit(*buf2)) {
                char_printf(ch, "Please supply the vnum of the entity to be deleted.\n");
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

        } else if (subcmd == SCMD_OLC_OEDIT && strncasecmp("revex", buf1, 5) == 0) {
            if (!*buf2) {
                char_printf(ch, "Reverse extra descs for which object?\n");
                return;
            }

            number = atoi(buf2);
            if ((real_num = real_object(number)) < 0) {
                char_printf(ch, "There is no such object.\n");
                return;
            }

            log(LogSeverity::Debug, MAX(LVL_GOD, GET_INVIS_LEV(ch)),
                "OLC: {} reverses object extra descs for {:d}, {}.", GET_NAME(ch), number,
                obj_proto[real_num].short_description);
            oedit_reverse_exdesc(real_num, nullptr);
            return;

        } else if (subcmd == SCMD_OLC_OEDIT && strncasecmp("zrevex", buf1, 6) == 0) {
            if (!*buf2) {
                char_printf(ch, "Reverse object extra descs in which zone?\n");
                return;
            }

            number = atoi(buf2) * 100;
            if ((real_num = find_real_zone_by_room(number)) == -1) {
                char_printf(ch, "Sorry, there is no zone for that number!\n");
                return;
            }

            log(LogSeverity::Debug, MAX(LVL_GOD, GET_INVIS_LEV(ch)),
                "OLC: {} reverses object extra descs for zone {:d}.", GET_NAME(ch), zone_table[real_num].number);
            oedit_reverse_exdescs(real_num, ch);
            return;

        } else if (subcmd == SCMD_OLC_ZEDIT && GET_LEVEL(ch) >= LVL_HEAD_B && *buf1) {
            if ((strncasecmp("new", buf1, 3) == 0) && *buf2)
                zedit_new_zone(ch, atoi(buf2));
            else
                char_printf(ch, "Specify a zone number.\n");
            return;
        } else if (subcmd != SCMD_OLC_SDEDIT && subcmd != SCMD_OLC_ZEDIT && subcmd != SCMD_OLC_REDIT) {
            char_printf(ch, "Yikes!   Stop that, someone will get hurt!\n");
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
                    sprintf(buf, "Help files are currently being edited by %s\n",
                            (CAN_SEE(ch, d->character) ? GET_NAME(d->character) : "someone"));
                else
                    sprintf(buf, "That %s is currently being edited by %s.\n", olc_scmd_info[subcmd].text,
                            GET_NAME(d->character));
                char_printf(ch, buf);
                return;
            }
    d = ch->desc;

    /*. Give descriptor an OLC struct . */
    CREATE(d->olc, OLCData, 1);

    /*
     * Find the zone.
     */
    if ((subcmd != SCMD_OLC_HEDIT) && (subcmd != SCMD_OLC_SDEDIT)) {
        if ((OLC_ZNUM(d) = find_real_zone_by_room(number)) == -1) {
            char_printf(ch, "Sorry, there is no zone for that number!\n");
            free(d->olc);
            return;
        }
    }
    /*
     * Everyone but IMPLs can only edit zones they have been assigned.
     */
    if (subcmd == SCMD_OLC_HEDIT && GET_LEVEL(ch) < LVL_GOD && !has_olc_access(ch, HEDIT_PERMISSION)) {
        char_printf(ch, "You do not have permission to edit help files.\n");
        free(d->olc);
        return;
    } else if (GET_LEVEL(ch) < LVL_GRGOD && !has_olc_access(ch, zone_table[OLC_ZNUM(d)].number)) {
        char_printf(ch, "You do not have permission to edit this zone.\n");
        free(d->olc);
        return;
    }
    if (action == OLC_ACTION_SAVE) {
        const char *type = nullptr;

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
            char_printf(ch, "Oops, I forgot what you wanted to save.\n");
            return;
        }
        char_printf(ch, "Saving all {}s in zone {:d}.\n", type, zone_table[OLC_ZNUM(d)].number);
        log(LogSeverity::Debug, MAX(LVL_GOD, GET_INVIS_LEV(ch)), "OLC: {} saves {} info for zone {:d}.", GET_NAME(ch),
            type, zone_table[OLC_ZNUM(d)].number);

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
                char_printf(ch, "That help topic already exists.\n");
                return;
            }
            char_printf(ch, "Do you wish to add a help topic on '{}'? ", OLC_STORAGE(d));
            OLC_MODE(d) = HEDIT_CONFIRM_ADD;
        } else {
            char_printf(ch, "Do you wish to edit the help topic on '{}'? ", help_table[OLC_ZNUM(d)].keyword);
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
        if ((real_num < 0) || (real_num > TOP_SKILL) || (!strcasecmp("!UNUSED!", skills[real_num].name))) {
            char_printf(ch, "Your spell could not be found.\n");
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
            char_printf(ch, "That room does not exist.\n");
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
    case SCMD_OLC_RCOPY:
        if ((real_num = real_room(number)) < 0) {
            char_printf(ch, "That source room does not exist.\r\n");
            return;
        }

        number = atoi(buf2);
        if ((real_room(number)) >= 0) {
            char_printf(ch, "The target room already exists.\r\n");
            return;
        }

        OLC_NUM(d) = number;
        redit_setup_existing(d, real_num);
        STATE(d) = CON_REDIT;
        break;
    case SCMD_OLC_OCOPY:
        if ((real_num = real_object(number)) < 0) {
            char_printf(ch, "The source object does not exist.\r\n");
            return;
        }

        number = atoi(buf2);
        if ((real_object(number)) >= 0) {
            char_printf(ch, "The target object already exists.\r\n");
            return;
        }

        OLC_NUM(d) = number;
        oedit_setup_existing(d, real_num);
        STATE(d) = CON_OEDIT;
        break;
    case SCMD_OLC_MCOPY:
        if ((real_num = real_mobile(number)) < 0) {
            char_printf(ch, "The source mobile does not exist.\r\n");
            return;
        }

        number = atoi(buf2);
        if ((real_mobile(number)) >= 0) {
            char_printf(ch, "The target mobile already exists.\r\n");
            return;
        }

        OLC_NUM(d) = number;
        medit_setup_existing(d, real_num);
        STATE(d) = CON_MEDIT;
        break;
    case SCMD_OLC_TRIGCOPY:
        if ((real_num = real_trigger(number)) < 0) {
            char_printf(ch, "The source trigger does not exist.\r\n");
            return;
        }

        number = atoi(buf2);
        if ((real_trigger(number)) >= 0) {
            char_printf(ch, "The target trigger already exists.\r\n");
            return;
        }

        OLC_NUM(d) = number;
        trigedit_setup_existing(d, real_num);
        STATE(d) = CON_TRIGEDIT;
        break;
    case SCMD_OLC_ZCOPY:
    case SCMD_OLC_SCOPY:
        char_printf(ch, "Sorry, this command is not yet implemented.\r\n");
        return;
    }

    act("$n starts using OLC.", true, d->character, 0, 0, TO_ROOM);
    SET_FLAG(PLR_FLAGS(ch), PLR_WRITING);
}

/*------------------------------------------------------------*\
 Internal utlities
\*------------------------------------------------------------*/

void olc_saveinfo(CharData *ch) {
    OLCSaveInfo *entry;

    if (olc_save_list)
        char_printf(ch, "The following OLC components need saving:-\n");
    else
        char_printf(ch, "The database is up to date.\n");

    for (entry = olc_save_list; entry; entry = entry->next) {
        sprintf(buf, " - %s for zone %d.\n", save_info_msg[(int)entry->type], entry->zone);
        char_printf(ch, buf);
    }
}

/*------------------------------------------------------------*\
   Exported utlities
\*------------------------------------------------------------*/

/*. Add an entry to the 'to be saved' list .*/

void olc_add_to_save_list(int zone, byte type) {
    OLCSaveInfo *saveinfo;

    /*. Return if it's already in the list . */
    for (saveinfo = olc_save_list; saveinfo; saveinfo = saveinfo->next)
        if ((saveinfo->zone == zone) && (saveinfo->type == type))
            return;

    CREATE(saveinfo, OLCSaveInfo, 1);
    saveinfo->zone = zone;
    saveinfo->type = type;
    saveinfo->next = olc_save_list;
    olc_save_list = saveinfo;
}

/*. Remove an entry from the 'to be saved' list .*/

void olc_remove_from_save_list(int zone, byte type) {
    OLCSaveInfo **entry;
    OLCSaveInfo *temp;

    for (entry = &olc_save_list; *entry; entry = &(*entry)->next)
        if (((*entry)->zone == zone) && ((*entry)->type == type)) {
            temp = *entry;
            *entry = temp->next;
            free(temp);
            return;
        }
}

/*. Set the colour string pointers for that which this char will
   see at color level LogSeverity::Stat.   Changing the entries here will change
   the colour scheme throught the OLC.*/

void get_char_cols(CharData *ch) {
    nrm = CLR(ch, ANRM);
    grn = CLR(ch, FGRN);
    cyn = CLR(ch, FCYN);
    yel = CLR(ch, FYEL);
    blk = CLR(ch, FBLK);
    red = CLR(ch, FRED);
}

/*. This procedure removes the '\n' from a string so that it may be
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

void cleanup_olc(DescriptorData *d, byte cleanup_type) {
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
            act("$n stops using OLC.", true, d->character, 0, 0, TO_ROOM);
        }
        free(d->olc);
        d->olc = nullptr;
    }
}

void free_save_list() {
    OLCSaveInfo *sld, *next_sld;

    for (sld = olc_save_list; sld; sld = next_sld) {
        next_sld = sld->next;
        free(sld);
    }
}
