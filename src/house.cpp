/***************************************************************************
 *   File: house.c                                        Part of FieryMUD *
 *  Usage: Handling of player houses                                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998 - 2006 by the Fiery Consortium             *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "house.hpp"

#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "directions.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "objects.hpp"
#include "pfiles.hpp"
#include "players.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

HouseControlRec house_control[MAX_HOUSES];
int num_of_houses = 0;

/* First, the basics: finding the filename; loading/saving objects */

/* Return a filename given a house vnum */
int House_get_filename(int vnum, char *filename) {
    if (vnum < 0)
        return 0;

    sprintf(filename, "house/%d.house", vnum);
    return 1;
}

/* Load all objects for a house */
int House_load(int vnum) {
    FILE *fl;
    char fname[MAX_STRING_LENGTH];
    ObjData *obj, *containers[MAX_CONTAINER_DEPTH];
    int rnum, location, depth, i;

    if ((rnum = real_room(vnum)) == -1)
        return 0;
    if (!House_get_filename(vnum, fname))
        return 0;
    if (!(fl = fopen(fname, "r"))) {
        /* no file found */
        return 0;
    }

    while (!feof(fl)) {
        if (!build_object(fl, &obj, &location))
            break;
        depth = MAX(0, -location);
        for (i = MAX_CONTAINER_DEPTH - 1; i >= depth; --i)
            containers[i] = nullptr;
        containers[depth] = obj;
        if (depth > 0)
            obj_to_obj(obj, containers[depth - 1]);
        else
            obj_to_room(obj, rnum);
    }

    fclose(fl);

    return 1;
}

/* Save all objects in a house */
void House_crashsave(int vnum) {
    int rnum;
    char buf[MAX_STRING_LENGTH];
    FILE *fp;

    if ((rnum = real_room(vnum)) == -1)
        return;
    if (!House_get_filename(vnum, buf))
        return;
    if (!(fp = fopen(buf, "w"))) {
        perror("SYSERR: Error saving house file");
        return;
    }
    write_objects(world[rnum].contents, fp, WEAR_INVENTORY);
    fclose(fp);
    REMOVE_FLAG(ROOM_FLAGS(rnum), ROOM_HOUSE_CRASH);
}

/* Delete a house save file */
void House_delete_file(int vnum) {
    char buf[MAX_INPUT_LENGTH], fname[MAX_INPUT_LENGTH];
    FILE *fl;

    if (!House_get_filename(vnum, fname))
        return;
    if (!(fl = fopen(fname, "r"))) {
        if (errno != ENOENT) {
            sprintf(buf, "SYSERR: Error deleting house file #%d. (1)", vnum);
            perror(buf);
        }
        return;
    }
    fclose(fl);
    if (unlink(fname) < 0) {
        sprintf(buf, "SYSERR: Error deleting house file #%d. (2)", vnum);
        perror(buf);
    }
}

/* List all objects in a house file */
void House_listrent(CharData *ch, int vnum) {
    FILE *fl;
    char fname[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    ObjData *obj;
    int location;

    if (!House_get_filename(vnum, fname))
        return;
    if (!(fl = fopen(fname, "r"))) {
        char_printf(ch, "No objects on file for house #{:d}.\n", vnum);
        return;
    }
    while (!feof(fl)) {
        if (!build_object(fl, &obj, &location))
            break;
        char_printf(ch, "[{:5d}] {}\n", GET_OBJ_VNUM(obj), obj->short_description);
        extract_obj(obj);
    }

    fclose(fl);
}

/******************************************************************
 *   Functions for house administration (creation, deletion, etc.   *
 *****************************************************************/

int find_house(int vnum) {
    int i;

    for (i = 0; i < num_of_houses; i++)
        if (house_control[i].vnum == vnum)
            return i;

    return -1;
}

/* Save the house control information */
void House_save_control(void) {
    FILE *fl;

    if (!(fl = fopen(HCONTROL_FILE, "wb"))) {
        perror("SYSERR: Unable to open house control file");
        return;
    }
    /* write all the house control recs in one fell swoop.   Pretty nifty, eh? */
    fwrite(house_control, sizeof(HouseControlRec), num_of_houses, fl);

    fclose(fl);
}

/* call from boot_db - will load control recs, load objs, set atrium bits */
/* should do sanity checks on vnums & remove invalid records */
void House_boot(void) {
    HouseControlRec temp_house;
    int real_house, real_atrium;
    FILE *fl;

    memset((char *)house_control, 0, sizeof(HouseControlRec) * MAX_HOUSES);

    if (!(fl = fopen(HCONTROL_FILE, "rb"))) {
        log("House control file does not exist.");
        return;
    }
    while (!feof(fl) && num_of_houses < MAX_HOUSES) {
        fread(&temp_house, sizeof(HouseControlRec), 1, fl);

        if (feof(fl))
            break;

        if (get_name_by_id(temp_house.owner) == nullptr)
            continue; /* owner no longer exists -- skip */

        if ((real_house = real_room(temp_house.vnum)) < 0)
            continue; /* this vnum doesn't exist -- skip */

        if ((find_house(temp_house.vnum)) >= 0)
            continue; /* this vnum is already a hosue -- skip */

        if ((real_atrium = real_room(temp_house.atrium)) < 0)
            continue; /* house doesn't have an atrium -- skip */

        if (temp_house.exit_num < 0 || temp_house.exit_num >= NUM_OF_DIRS)
            continue; /* invalid exit num -- skip */

        if (TOROOM(real_house, temp_house.exit_num) != real_atrium)
            continue; /* exit num mismatch -- skip */

        house_control[num_of_houses++] = temp_house;

        SET_FLAG(ROOM_FLAGS(real_house), ROOM_HOUSE);
        SET_FLAG(ROOM_FLAGS(real_house), ROOM_PRIVATE);
        SET_FLAG(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
        House_load(temp_house.vnum);
    }

    fclose(fl);
    House_save_control();
}

/* "House Control" functions */

char *HCONTROL_FORMAT =
    "Usage: hcontrol build <house vnum> <exit direction> <player name>\n"
    "          hcontrol destroy <house vnum>\n"
    "          hcontrol pay <house vnum>\n"
    "          hcontrol show\n";

#define NAME(x) ((temp = get_name_by_id(x)) == NULL ? "<UNDEF>" : temp)

void hcontrol_list_houses(CharData *ch) {
    int i, j;
    char *timestr, *temp;
    char built_on[128], last_pay[128], own_name[128];

    if (!num_of_houses) {
        char_printf(ch, "No houses have been defined.\n");
        return;
    }
    strcpy(buf, "Address   Atrium   Build Date   Guests   Owner            Last Paymt\n");
    strcat(buf, "-------   ------   ----------   ------   ------------ ----------\n");

    for (i = 0; i < num_of_houses; i++) {
        if (house_control[i].built_on) {
            timestr = asctime(localtime(&(house_control[i].built_on)));
            *(timestr + 10) = 0;
            strcpy(built_on, timestr);
        } else
            strcpy(built_on, "Unknown");

        if (house_control[i].last_payment) {
            timestr = asctime(localtime(&(house_control[i].last_payment)));
            *(timestr + 10) = 0;
            strcpy(last_pay, timestr);
        } else
            strcpy(last_pay, "None");

        strcpy(own_name, NAME(house_control[i].owner));

        sprintf(buf, "%s%7d %7d   %-10s      %2d      %-12s %s\n", buf, house_control[i].vnum, house_control[i].atrium,
                built_on, house_control[i].num_of_guests, cap_by_color(own_name), last_pay);

        if (house_control[i].num_of_guests) {
            strcat(buf, "       Guests: ");
            for (j = 0; j < house_control[i].num_of_guests; j++) {
                sprintf(buf2, "%s ", NAME(house_control[i].guests[j]));
                strcat(buf, cap_by_color(buf2));
            }
            strcat(buf, "\n");
        }
    }
    char_printf(ch, buf);
}

void hcontrol_build_house(CharData *ch, char *arg) {
    char arg1[MAX_INPUT_LENGTH];
    HouseControlRec temp_house;
    int virt_house, real_house, real_atrium, virt_atrium, exit_num;
    long owner;

    if (num_of_houses >= MAX_HOUSES) {
        char_printf(ch, "Max houses already defined.\n");
        return;
    }

    /* first arg: house's vnum */
    arg = one_argument(arg, arg1);
    if (!*arg1) {
        char_printf(ch, HCONTROL_FORMAT);
        return;
    }
    virt_house = atoi(arg1);
    if ((real_house = real_room(virt_house)) < 0) {
        char_printf(ch, "No such room exists.\n");
        return;
    }
    if ((find_house(virt_house)) >= 0) {
        char_printf(ch, "House already exists.\n");
        return;
    }

    /* second arg: direction of house's exit */
    arg = one_argument(arg, arg1);
    if (!*arg1) {
        char_printf(ch, HCONTROL_FORMAT);
        return;
    }
    if ((exit_num = searchblock(arg1, dirs, false)) < 0) {
        char_printf(ch, "'{}' is not a valid direction.\n", arg1);
        return;
    }
    if (TOROOM(real_house, exit_num) == NOWHERE) {
        char_printf(ch, "There is no exit {} from room {:d}.\n", dirs[exit_num], virt_house);
        return;
    }

    real_atrium = TOROOM(real_house, exit_num);
    virt_atrium = world[real_atrium].vnum;

    if (TOROOM(real_atrium, rev_dir[exit_num]) != real_house) {
        char_printf(ch, "A house's exit must be a two-way door.\n");
        return;
    }

    /* third arg: player's name */
    arg = one_argument(arg, arg1);
    if (!*arg1) {
        char_printf(ch, HCONTROL_FORMAT);
        return;
    }
    if ((owner = get_id_by_name(arg1)) < 0) {
        char_printf(ch, "Unknown player '{}'.\n", arg1);
        return;
    }

    temp_house.mode = HOUSE_PRIVATE;
    temp_house.vnum = virt_house;
    temp_house.atrium = virt_atrium;
    temp_house.exit_num = exit_num;
    temp_house.built_on = time(0);
    temp_house.last_payment = 0;
    temp_house.owner = owner;
    temp_house.num_of_guests = 0;

    house_control[num_of_houses++] = temp_house;

    SET_FLAG(ROOM_FLAGS(real_house), ROOM_HOUSE);
    SET_FLAG(ROOM_FLAGS(real_house), ROOM_PRIVATE);
    SET_FLAG(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
    House_crashsave(virt_house);

    char_printf(ch, "House built.   Mazel tov!\n");
    House_save_control();
}

void hcontrol_destroy_house(CharData *ch, char *arg) {
    int i, j;
    int real_atrium, real_house;

    if (!*arg) {
        char_printf(ch, HCONTROL_FORMAT);
        return;
    }
    if ((i = find_house(atoi(arg))) < 0) {
        char_printf(ch, "Unknown house.\n");
        return;
    }
    if ((real_atrium = real_room(house_control[i].atrium)) < 0)
        log("SYSERR: House had invalid atrium!");
    else
        REMOVE_FLAG(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

    if ((real_house = real_room(house_control[i].vnum)) < 0)
        log("SYSERR: House had invalid vnum!");
    else {
        REMOVE_FLAG(ROOM_FLAGS(real_house), ROOM_HOUSE);
        REMOVE_FLAG(ROOM_FLAGS(real_house), ROOM_PRIVATE);
        REMOVE_FLAG(ROOM_FLAGS(real_house), ROOM_HOUSE_CRASH);
    }

    House_delete_file(house_control[i].vnum);

    for (j = i; j < num_of_houses - 1; j++)
        house_control[j] = house_control[j + 1];

    num_of_houses--;

    char_printf(ch, "House deleted.\n");
    House_save_control();

    /*
     * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums,
     * just in case the house we just deleted shared an atrium with another
     * house.   --JE 9/19/94
     */
    for (i = 0; i < num_of_houses; i++)
        if ((real_atrium = real_room(house_control[i].atrium)) >= 0)
            SET_FLAG(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
}

void hcontrol_pay_house(CharData *ch, char *arg) {
    int i;

    if (!*arg)
        char_printf(ch, HCONTROL_FORMAT);
    else if ((i = find_house(atoi(arg))) < 0)
        char_printf(ch, "Unknown house.\n");
    else {
        log(LogSeverity::Stat, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "Payment for house {} collected by {}.", arg,
            GET_NAME(ch));

        house_control[i].last_payment = time(0);
        House_save_control();
        char_printf(ch, "Payment recorded.\n");
    }
}

/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    half_chop(argument, arg1, arg2);

    if (is_abbrev(arg1, "build"))
        hcontrol_build_house(ch, arg2);
    else if (is_abbrev(arg1, "destroy"))
        hcontrol_destroy_house(ch, arg2);
    else if (is_abbrev(arg1, "pay"))
        hcontrol_pay_house(ch, arg2);
    else if (is_abbrev(arg1, "show"))
        hcontrol_list_houses(ch);
    else
        char_printf(ch, HCONTROL_FORMAT);
}

/* The house command, used by mortal house owners to assign guests */
ACMD(do_house) {
    int i, j, id;
    char *temp;

    one_argument(argument, arg);

    if (!ROOM_FLAGGED(ch->in_room, ROOM_HOUSE))
        char_printf(ch, "You must be in your house to set guests.\n");
    else if ((i = find_house(world[ch->in_room].vnum)) < 0)
        char_printf(ch, "Um.. this house seems to be screwed up.\n");
    else if (GET_IDNUM(ch) != house_control[i].owner)
        char_printf(ch, "Only the primary owner can set guests.\n");
    else if (!*arg) {
        char_printf(ch, "Guests of your house:\n");
        if (house_control[i].num_of_guests == 0)
            char_printf(ch, "   None.\n");
        else
            for (j = 0; j < house_control[i].num_of_guests; j++) {
                strcpy(buf, NAME(house_control[i].guests[j]));
                char_printf(ch, strcat(cap_by_color(buf), "\n"));
            }
    } else if ((id = get_id_by_name(arg)) < 0)
        char_printf(ch, "No such player.\n");
    else if (id == GET_IDNUM(ch))
        char_printf(ch, "It's your house!\n");
    else {
        for (j = 0; j < house_control[i].num_of_guests; j++)
            if (house_control[i].guests[j] == id) {
                for (; j < house_control[i].num_of_guests; j++)
                    house_control[i].guests[j] = house_control[i].guests[j + 1];
                house_control[i].num_of_guests--;
                House_save_control();
                char_printf(ch, "Guest deleted.\n");
                return;
            }
        if (house_control[i].num_of_guests == MAX_GUESTS) {
            char_printf(ch, "You have too many guests already.\n");
            return;
        }
        j = house_control[i].num_of_guests++;
        house_control[i].guests[j] = id;
        House_save_control();
        char_printf(ch, "Guest added.\n");
    }
}

/* Misc. administrative functions */

/* crash-save all the houses */
void House_save_all(void) {
    int i;
    int real_house;

    for (i = 0; i < num_of_houses; i++)
        if ((real_house = real_room(house_control[i].vnum)) != NOWHERE)
            if (ROOM_FLAGGED(real_house, ROOM_HOUSE_CRASH))
                House_crashsave(house_control[i].vnum);
}

/* note: arg passed must be house vnum, so there. */
int House_can_enter(CharData *ch, int house) {
    int i, j;

    if (GET_LEVEL(ch) >= LVL_GRGOD || (i = find_house(house)) < 0)
        return 1;

    switch (house_control[i].mode) {
    case HOUSE_PRIVATE:
        if (GET_IDNUM(ch) == house_control[i].owner)
            return 1;
        for (j = 0; j < house_control[i].num_of_guests; j++)
            if (GET_IDNUM(ch) == house_control[i].guests[j])
                return 1;
        return 0;
        break;
    }

    return 0;
}
