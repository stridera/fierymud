/***************************************************************************
 *   File: ban.c                                          Part of FieryMUD *
 *  Usage: banning/unbanning/checking sites and player names               *
 *                                                                         *
 *  FieryMUD enhancements by David Endre and Jimmy Kincaid                 *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 ***************************************************************************/

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

BanListElement *ban_list = nullptr;
XName *xname_list = nullptr;

const char *ban_types[] = {"no", "new", "select", "all", "ERROR"};

void load_banned(void) {
    FILE *fl;
    int i, date;
    char site_name[BANNED_SITE_LENGTH + 1], ban_type[100];
    char name[MAX_NAME_LENGTH + 1];
    BanListElement *next_node;

    ban_list = 0;

    if (!(fl = fopen(BAN_FILE, "r"))) {
        perror("Unable to open banfile");
        return;
    }
    while (fscanf(fl, " %s %s %d %s ", ban_type, site_name, &date, name) == 4) {
        CREATE(next_node, BanListElement, 1);
        strncpy(next_node->site, site_name, BANNED_SITE_LENGTH);
        next_node->site[BANNED_SITE_LENGTH] = '\0';
        strncpy(next_node->name, name, MAX_NAME_LENGTH);
        next_node->name[MAX_NAME_LENGTH] = '\0';
        next_node->date = date;

        for (i = BAN_NOT; i <= BAN_ALL; i++)
            if (!strcasecmp(ban_type, ban_types[i]))
                next_node->type = i;

        next_node->next = ban_list;
        ban_list = next_node;
    }

    fclose(fl);
}

int isbanned(char *hostname) {
    int i;
    BanListElement *banned_node;
    char *nextchar;

    if (!hostname || !*hostname)
        return (0);

    i = 0;
    for (nextchar = hostname; *nextchar; nextchar++)
        *nextchar = LOWER(*nextchar);

    for (banned_node = ban_list; banned_node; banned_node = banned_node->next)
        if (strcasestr(hostname, banned_node->site)) /* if hostname is a substring */
            i = MAX(i, banned_node->type);

    return i;
}

void _write_one_node(FILE *fp, BanListElement *node) {
    if (node) {
        _write_one_node(fp, node->next);
        fprintf(fp, "%s %s %ld %s\n", ban_types[node->type], node->site, (long)node->date, node->name);
    }
}

void write_ban_list(void) {
    FILE *fl;

    if (!(fl = fopen(BAN_FILE, "w"))) {
        perror("write_ban_list");
        return;
    }
    _write_one_node(fl, ban_list); /* recursively write from end to start */
    fclose(fl);
    return;
}

ACMD(do_ban) {
    char flag[MAX_INPUT_LENGTH], site[MAX_INPUT_LENGTH], format[MAX_INPUT_LENGTH], *nextchar;
    int i;
    BanListElement *ban_node;

    *buf = '\0';

    if (!*argument) {
        if (!ban_list) {
            char_printf(ch, "No sites are banned.\n");
            return;
        }
        strcpy(format, "%-25.25s  %-8.8s  %-11.11s  %-16.16s\n");
        sprintf(buf, format, "Banned Site Name", "Ban Type", "Banned On", "Banned By");
        char_printf(ch, buf);
        sprintf(buf, format, "---------------------------------", "---------------------------------",
                "---------------------------------", "---------------------------------");
        char_printf(ch, buf);

        for (ban_node = ban_list; ban_node; ban_node = ban_node->next) {
            if (ban_node->date) {
                strftime(buf1, 11, TIMEFMT_DATE, localtime(&ban_node->date));
                strcpy(site, buf1);
            } else
                strcpy(site, "Unknown");
            sprintf(buf, format, ban_node->site, ban_types[ban_node->type], site, ban_node->name);
            char_printf(ch, buf);
        }
        return;
    }
    two_arguments(argument, flag, site);
    if (!*site || !*flag) {
        char_printf(ch, "Usage: ban {all | select | new} site_name\n");
        return;
    }
    if (!(!strcasecmp(flag, "select") || !strcasecmp(flag, "all") || !strcasecmp(flag, "new"))) {
        char_printf(ch, "Flag must be ALL, SELECT, or NEW.\n");
        return;
    }
    for (ban_node = ban_list; ban_node; ban_node = ban_node->next) {
        if (!strcasecmp(ban_node->site, site)) {
            char_printf(ch, "That site has already been banned -- unban it to change the ban type.\n");
            return;
        }
    }

    CREATE(ban_node, BanListElement, 1);
    strncpy(ban_node->site, site, BANNED_SITE_LENGTH);
    for (nextchar = ban_node->site; *nextchar; nextchar++)
        *nextchar = LOWER(*nextchar);
    ban_node->site[BANNED_SITE_LENGTH] = '\0';
    strncpy(ban_node->name, GET_NAME(ch), MAX_NAME_LENGTH);
    ban_node->name[MAX_NAME_LENGTH] = '\0';
    ban_node->date = time(0);

    for (i = BAN_NEW; i <= BAN_ALL; i++)
        if (!strcasecmp(flag, ban_types[i]))
            ban_node->type = i;

    ban_node->next = ban_list;
    ban_list = ban_node;

    sprintf(buf, "%s has banned %s for %s players.", GET_NAME(ch), site, ban_types[ban_node->type]);
    log(LogSeverity::Stat, MAX(LVL_GOD, GET_INVIS_LEV(ch)), buf);
    char_printf(ch, "Site banned.\n");
    write_ban_list();
}

ACMD(do_unban) {
    char site[80];
    BanListElement *ban_node, *temp;
    int found = 0;

    one_argument(argument, site);
    if (!*site) {
        char_printf(ch, "A site to unban might help.\n");
        return;
    }
    ban_node = ban_list;
    while (ban_node && !found) {
        if (!strcasecmp(ban_node->site, site))
            found = 1;
        else
            ban_node = ban_node->next;
    }

    if (!found) {
        char_printf(ch, "That site is not currently banned.\n");
        return;
    }
    REMOVE_FROM_LIST(ban_node, ban_list, next);
    char_printf(ch, "Site unbanned.\n");
    sprintf(buf, "%s removed the %s-player ban on %s.", GET_NAME(ch), ban_types[ban_node->type], ban_node->site);
    log(LogSeverity::Stat, MAX(LVL_GOD, GET_INVIS_LEV(ch)), buf);

    free(ban_node);
    write_ban_list();
}

/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/

int Valid_Name(char *newname) {
    int i;
    DescriptorData *dt;
    XName *tmp_xname;
    char tempname[MAX_NAME_LENGTH];
    char invalid_name[MAX_NAME_LENGTH];

    /*
     * Make sure someone isn't trying to create this same name.  We want to
     * do a 'strcasecmp' so people can't do 'Bob' and 'BoB'.  The creating login
     * will not have a character name yet and other people sitting at the
     * prompt won't have characters yet.
     */
    for (dt = descriptor_list; dt; dt = dt->next) {
        if (dt->character && GET_NAME(dt->character) && !strcasecmp(GET_NAME(dt->character), newname)) {
            return (STATE(dt) == CON_PLAYING);
        }
    }
    /* return valid if list doesn't exist */
    if (!xname_list) {
        log("no xname_list found");
        return 1;
    }
    /* change to lowercase */
    strcpy(tempname, newname);
    for (i = 0; tempname[i]; i++)
        tempname[i] = LOWER(tempname[i]);

    /* Does the desired name contain a string in the invalid list? */

    /* Tested, fixed  & commented by David Endre 1/13/99
       We check the first character for a '#', if it contains one then
       we allow the name to contain that word, but not be only that
       word. Ex: #cum in the xname file...cucumber would be allowed
       along with cumter, however cum would not be allowed. If the name
       doesn't contain '#' then we go on as usual. Original idea and
       code by Therlos on 1/13/99
     */
    /* xnames is now a linked list --gurlaek 6/9/1999 */
    for (tmp_xname = xname_list; tmp_xname; tmp_xname = tmp_xname->next) {
        strcpy(invalid_name, tmp_xname->name);
        if (invalid_name[0] == '#') {
            int z = 0;
            while (invalid_name[z + 1] != '\0') {
                invalid_name[z] = invalid_name[z + 1];
                z++;
            }
            invalid_name[z] = '\0';
            if (strcasecmp(tempname, invalid_name) == 0)
                return 0;
        } else {
            if (strcasestr(tempname, invalid_name))
                return 0;
        }
    }
    /* is there a mobile with that name? */
    for (i = 0; i < top_of_mobt; i++) {
        if (isname(tempname, mob_proto[i].player.namelist))
            return 0;
    }

    return 1;
}

/* rewritten by gurlaek 6/9/1999 */
void Read_Xname_List(void) {
    FILE *fp;
    /* MAX_NAME_LENGTH + 3 is for the # the NULL and the CR */
    char string[MAX_NAME_LENGTH + 3];
    XName *tmp_xname, *tmp2_xname;

    if (!(fp = fopen(XNAME_FILE, "r"))) {
        perror("Unable to open invalid name file");
        return;
    }
    /* build the xname linked list */
    while (fgets(string, MAX_NAME_LENGTH + 3, fp) != nullptr) {
        /* make sure we don't load a blank xname --gurlaek 6/12/1999 */
        if ((strlen(string) - 1)) {
            if (!xname_list) {
                CREATE(xname_list, XName, 1);
                strncpy(xname_list->name, string, strlen(string) - 1);
                xname_list->next = nullptr;
            } else {
                CREATE(tmp_xname, XName, 1);
                strncpy(tmp_xname->name, string, strlen(string) - 1);
                tmp_xname->next = nullptr;
                tmp2_xname = xname_list;
                while (tmp2_xname->next) {
                    tmp2_xname = tmp2_xname->next;
                }
                tmp2_xname->next = tmp_xname;
            }
        }
    }

    fclose(fp);
}

void reload_xnames() {
    XName *cur, *next;

    for (cur = xname_list; cur; cur = next) {
        next = cur->next;
        free(cur);
    }
    xname_list = nullptr;
    Read_Xname_List();
}

/* send rejected names to the xnames file */
void send_to_xnames(char *name) {
    FILE *xnames;
    char input[MAX_NAME_LENGTH + 3];
    char tempname[MAX_NAME_LENGTH + 3];
    int i = 0;
    XName *tmp_xname, *tmp2_xname;

    *input = '\0';
    *tempname = '\0';

    if (!(xnames = fopen(XNAME_FILE, "a"))) {
        log(LogSeverity::Warn, LVL_IMMORT, "SYSERR: Cannot open xnames file.\n");
        return;
    }

    strcpy(tempname, name);
    for (i = 0; tempname[i]; i++)
        tempname[i] = LOWER(tempname[i]);

    /* print it to the xnames file with # prepended and a \n appended */
    fprintf(xnames, "#%s\n", tempname);

    fclose(xnames);

    sprintf(input, "#%s", tempname);

    /* dynamicly add it to the xname_list -gurlaek 6/9/1999 */

    if (!xname_list) {
        CREATE(xname_list, XName, 1);
        strncpy(xname_list->name, input, strlen(input));
        xname_list->next = nullptr;
    } else {
        CREATE(tmp_xname, XName, 1);
        strncpy(tmp_xname->name, input, strlen(input));
        tmp_xname->next = nullptr;
        tmp2_xname = xname_list;
        while (tmp2_xname->next) {
            tmp2_xname = tmp2_xname->next;
        }
        tmp2_xname->next = tmp_xname;
    }
}

void free_invalid_list() {
    XName *name, *next;
    BanListElement *ban;

    for (name = xname_list; name; name = next) {
        next = name->next;
        free(name);
    }

    while (ban_list) {
        ban = ban_list;
        ban_list = ban->next;
        free(ban);
    }
}

ACMD(do_xnames) {
    char flag[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];

    two_arguments(argument, flag, name);

    if (*flag && !strcasecmp(flag, "reload")) {
        reload_xnames();
        char_printf(ch, "Done.\n");
        return;
    }

    if (!*flag || !*name) {
        char_printf(ch, "Usage: xnames {add NAME | reload}\n");
        return;
    }

    if (is_abbrev(flag, "add")) {
        if (!Valid_Name(name)) {
            char_printf(ch, "Name is already banned.\n");
            return;
        }
        send_to_xnames(name);
        char_printf(ch, "Done.\n");
    } else {
        char_printf(ch, "Usage: xnames {add NAME | reload}\n");
    }
}
