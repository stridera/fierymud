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

#include "comm.h"
#include "conf.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "math.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

struct ban_list_element *ban_list = NULL;
struct xname *xname_list = NULL;

char *ban_types[] = {"no", "new", "select", "all", "ERROR"};

void load_banned(void) {
    FILE *fl;
    int i, date;
    char site_name[BANNED_SITE_LENGTH + 1], ban_type[100];
    char name[MAX_NAME_LENGTH + 1];
    struct ban_list_element *next_node;

    ban_list = 0;

    if (!(fl = fopen(BAN_FILE, "r"))) {
        perror("Unable to open banfile");
        return;
    }
    while (fscanf(fl, " %s %s %d %s ", ban_type, site_name, &date, name) == 4) {
        CREATE(next_node, struct ban_list_element, 1);
        strncpy(next_node->site, site_name, BANNED_SITE_LENGTH);
        next_node->site[BANNED_SITE_LENGTH] = '\0';
        strncpy(next_node->name, name, MAX_NAME_LENGTH);
        next_node->name[MAX_NAME_LENGTH] = '\0';
        next_node->date = date;

        for (i = BAN_NOT; i <= BAN_ALL; i++)
            if (!strcmp(ban_type, ban_types[i]))
                next_node->type = i;

        next_node->next = ban_list;
        ban_list = next_node;
    }

    fclose(fl);
}

int isbanned(char *hostname) {
    int i;
    struct ban_list_element *banned_node;
    char *nextchar;

    if (!hostname || !*hostname)
        return (0);

    i = 0;
    for (nextchar = hostname; *nextchar; nextchar++)
        *nextchar = LOWER(*nextchar);

    for (banned_node = ban_list; banned_node; banned_node = banned_node->next)
        if (strstr(hostname, banned_node->site)) /* if hostname is a substring */
            i = MAX(i, banned_node->type);

    return i;
}

void _write_one_node(FILE *fp, struct ban_list_element *node) {
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
    struct ban_list_element *ban_node;

    *buf = '\0';

    if (!*argument) {
        if (!ban_list) {
            send_to_char("No sites are banned.\r\n", ch);
            return;
        }
        strcpy(format, "%-25.25s  %-8.8s  %-11.11s  %-16.16s\r\n");
        sprintf(buf, format, "Banned Site Name", "Ban Type", "Banned On", "Banned By");
        send_to_char(buf, ch);
        sprintf(buf, format, "---------------------------------", "---------------------------------",
                "---------------------------------", "---------------------------------");
        send_to_char(buf, ch);

        for (ban_node = ban_list; ban_node; ban_node = ban_node->next) {
            if (ban_node->date) {
                strftime(buf1, 11, TIMEFMT_DATE, localtime(&ban_node->date));
                strcpy(site, buf1);
            } else
                strcpy(site, "Unknown");
            sprintf(buf, format, ban_node->site, ban_types[ban_node->type], site, ban_node->name);
            send_to_char(buf, ch);
        }
        return;
    }
    two_arguments(argument, flag, site);
    if (!*site || !*flag) {
        send_to_char("Usage: ban {all | select | new} site_name\r\n", ch);
        return;
    }
    if (!(!str_cmp(flag, "select") || !str_cmp(flag, "all") || !str_cmp(flag, "new"))) {
        send_to_char("Flag must be ALL, SELECT, or NEW.\r\n", ch);
        return;
    }
    for (ban_node = ban_list; ban_node; ban_node = ban_node->next) {
        if (!str_cmp(ban_node->site, site)) {
            send_to_char("That site has already been banned -- unban it to change the ban type.\r\n", ch);
            return;
        }
    }

    CREATE(ban_node, struct ban_list_element, 1);
    strncpy(ban_node->site, site, BANNED_SITE_LENGTH);
    for (nextchar = ban_node->site; *nextchar; nextchar++)
        *nextchar = LOWER(*nextchar);
    ban_node->site[BANNED_SITE_LENGTH] = '\0';
    strncpy(ban_node->name, GET_NAME(ch), MAX_NAME_LENGTH);
    ban_node->name[MAX_NAME_LENGTH] = '\0';
    ban_node->date = time(0);

    for (i = BAN_NEW; i <= BAN_ALL; i++)
        if (!str_cmp(flag, ban_types[i]))
            ban_node->type = i;

    ban_node->next = ban_list;
    ban_list = ban_node;

    sprintf(buf, "%s has banned %s for %s players.", GET_NAME(ch), site, ban_types[ban_node->type]);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
    send_to_char("Site banned.\r\n", ch);
    write_ban_list();
}

ACMD(do_unban) {
    char site[80];
    struct ban_list_element *ban_node, *temp;
    int found = 0;

    one_argument(argument, site);
    if (!*site) {
        send_to_char("A site to unban might help.\r\n", ch);
        return;
    }
    ban_node = ban_list;
    while (ban_node && !found) {
        if (!str_cmp(ban_node->site, site))
            found = 1;
        else
            ban_node = ban_node->next;
    }

    if (!found) {
        send_to_char("That site is not currently banned.\r\n", ch);
        return;
    }
    REMOVE_FROM_LIST(ban_node, ban_list, next);
    send_to_char("Site unbanned.\r\n", ch);
    sprintf(buf, "%s removed the %s-player ban on %s.", GET_NAME(ch), ban_types[ban_node->type], ban_node->site);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    free(ban_node);
    write_ban_list();
}

/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/

int Valid_Name(char *newname) {
    int i;
    struct descriptor_data *dt;
    struct xname *tmp_xname;
    char tempname[MAX_NAME_LENGTH];
    char invalid_name[MAX_NAME_LENGTH];

    /*
     * Make sure someone isn't trying to create this same name.  We want to
     * do a 'str_cmp' so people can't do 'Bob' and 'BoB'.  The creating login
     * will not have a character name yet and other people sitting at the
     * prompt won't have characters yet.
     */
    for (dt = descriptor_list; dt; dt = dt->next) {
        if (dt->character && GET_NAME(dt->character) && !str_cmp(GET_NAME(dt->character), newname)) {
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
            if (strcmp(tempname, invalid_name) == 0)
                return 0;
        } else {
            if (strstr(tempname, invalid_name))
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
    struct xname *tmp_xname, *tmp2_xname;

    if (!(fp = fopen(XNAME_FILE, "r"))) {
        perror("Unable to open invalid name file");
        return;
    }
    /* build the xname linked list */
    while (fgets(string, MAX_NAME_LENGTH + 3, fp) != NULL) {
        /* make sure we don't load a blank xname --gurlaek 6/12/1999 */
        if ((strlen(string) - 1)) {
            if (!xname_list) {
                CREATE(xname_list, struct xname, 1);
                strncpy(xname_list->name, string, strlen(string) - 1);
                xname_list->next = NULL;
            } else {
                CREATE(tmp_xname, struct xname, 1);
                strncpy(tmp_xname->name, string, strlen(string) - 1);
                tmp_xname->next = NULL;
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
    struct xname *cur, *next;

    for (cur = xname_list; cur; cur = next) {
        next = cur->next;
        free(cur);
    }
    xname_list = NULL;
    Read_Xname_List();
}

/* send rejected names to the xnames file */
void send_to_xnames(char *name) {
    FILE *xnames;
    char input[MAX_NAME_LENGTH + 3];
    char tempname[MAX_NAME_LENGTH + 3];
    int i = 0;
    struct xname *tmp_xname, *tmp2_xname;

    *input = '\0';
    *tempname = '\0';

    if (!(xnames = fopen(XNAME_FILE, "a"))) {
        mudlog("SYSERR: Cannot open xnames file.\r\n", BRF, LVL_IMMORT, TRUE);
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
        CREATE(xname_list, struct xname, 1);
        strncpy(xname_list->name, input, strlen(input));
        xname_list->next = NULL;
    } else {
        CREATE(tmp_xname, struct xname, 1);
        strncpy(tmp_xname->name, input, strlen(input));
        tmp_xname->next = NULL;
        tmp2_xname = xname_list;
        while (tmp2_xname->next) {
            tmp2_xname = tmp2_xname->next;
        }
        tmp2_xname->next = tmp_xname;
    }
}

void free_invalid_list() {
    struct xname *name, *next;
    struct ban_list_element *ban;

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

    if (*flag && !str_cmp(flag, "reload")) {
        reload_xnames();
        send_to_char("Done.\r\n", ch);
        return;
    }

    if (!*flag || !*name) {
        send_to_char("Usage: xnames {add NAME | reload}\r\n", ch);
        return;
    }

    if (is_abbrev(flag, "add")) {
        if (!Valid_Name(name)) {
            send_to_char("Name is already banned.\r\n", ch);
            return;
        }
        send_to_xnames(name);
        send_to_char("Done.\r\n", ch);
    } else {
        send_to_char("Usage: xnames {add NAME | reload}\r\n", ch);
    }
}
