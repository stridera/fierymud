/***************************************************************************
 *  File: pfiles.c                                        Part of FieryMUD *
 *  Usage: loading/saving player objects and quests                        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "pfiles.h"

#include "act.h"
#include "casting.h"
#include "comm.h"
#include "conf.h"
#include "constants.h"
#include "db.h"
#include "dg_scripts.h"
#include "handler.h"
#include "interpreter.h"
#include "legacy_structs.h"
#include "math.h"
#include "modify.h"
#include "movement.h"
#include "players.h"
#include "quest.h"
#include "skills.h"
#include "specprocs.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* Extern functions */
ACMD(do_tell);

static void extract_unrentables(struct char_data *ch);
static bool write_rent_code(FILE *fl, int rentcode);
static bool write_object_record(struct obj_data *obj, FILE *fl, int location);
int delete_objects_file(char *name);
static void read_objects(struct char_data *ch, FILE *fl);
static void list_objects(struct obj_data *obj, struct char_data *ch, int indent, int last_indent,
                         const char *first_indent);
static bool load_binary_objects(struct char_data *ch);

void save_player_objects(struct char_data *ch) {
    FILE *fl;
    int i;
    char filename[MAX_INPUT_LENGTH], tempfilename[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
        return;

    if (!get_pfilename(GET_NAME(ch), tempfilename, TEMP_FILE)) {
        sprintf(buf, "SYSERR: Couldn't make temporary file name for saving objects for %s.", GET_NAME(ch));
        mudlog(buf, NRM, LVL_GOD, TRUE);
        return;
    }

    if (!get_pfilename(GET_NAME(ch), filename, OBJ_FILE)) {
        sprintf(buf, "SYSERR: Couldn't make final file name for saving objects for %s.", GET_NAME(ch));
        mudlog(buf, NRM, LVL_GOD, TRUE);
        return;
    }

    if (!(fl = fopen(tempfilename, "w"))) {
        sprintf(buf, "SYSERR: Couldn't open player file %s for write", tempfilename);
        mudlog(buf, NRM, LVL_GOD, TRUE);
        return;
    }

    if (ch->carrying == NULL) {
        for (i = 0; i < NUM_WEARS && !GET_EQ(ch, i); ++i)
            ;
        if (i == NUM_WEARS) { /* No equipment or inventory. */
            fclose(fl);
            delete_objects_file(GET_NAME(ch));
            return;
        }
    }

    /* Legacy.  Reason for leaving game is now in struct char_special_data
     * (thus the character save file). The loading code expects a value here,
     * and all modern object save files have one. */
    write_rent_code(fl, 1);

    for (i = 0; i < NUM_WEARS; ++i)
        if (GET_EQ(ch, i)) {
            if (!write_objects(GET_EQ(ch, i), fl, i)) {
                fclose(fl);
                return;
            }
        }

    if (!write_objects(ch->carrying, fl, WEAR_INVENTORY)) {
        fclose(fl);
        return;
    }

    if (fclose(fl)) {
        sprintf(buf, "SYSERR: Error closing rent file for %s after write", GET_NAME(ch));
        log(buf);
    } else if (rename(tempfilename, filename)) {
        sprintf(buf, "SYSERR: Error renaming temporary rent file for %s after write", GET_NAME(ch));
        log(buf);
    }
}

static bool is_object_unrentable(struct obj_data *obj) {
    if (!obj)
        return FALSE;

    if (OBJ_FLAGGED(obj, ITEM_NORENT))
        return TRUE;

    return FALSE;
}

static void extract_unrentables_from_list(struct obj_data *obj) {
    if (obj) {
        extract_unrentables_from_list(obj->contains);
        extract_unrentables_from_list(obj->next_content);
        if (is_object_unrentable(obj))
            extract_obj(obj);
    }
}

static void extract_unrentables(struct char_data *ch) {
    int i;

    for (i = 0; i < NUM_WEARS; ++i)
        if (GET_EQ(ch, i)) {
            if (is_object_unrentable(GET_EQ(ch, i)))
                obj_to_char(unequip_char(ch, i), ch);
            else
                extract_unrentables_from_list(GET_EQ(ch, i)->contains);
        }

    extract_unrentables_from_list(ch->carrying);
}

static bool write_rent_code(FILE *fl, int rentcode) {
    fprintf(fl, "%d\n", rentcode);
    return FALSE;
}

bool write_objects(struct obj_data *obj, FILE *fl, int location) {
    struct obj_data *temp;
    bool success = TRUE;

    if (obj) {
        /*
         * Traverse the list in reverse order so when they are loaded
         * and placed back on the char using obj_to_char, they will be
         * in the correct order.
         */
        write_objects(obj->next_content, fl, location);

        for (temp = obj->contains; temp; temp = temp->next_content)
            GET_OBJ_WEIGHT(obj) -= GET_OBJ_WEIGHT(temp);

        success = write_object_record(obj, fl, location);

        for (temp = obj->contains; temp; temp = temp->next_content)
            GET_OBJ_WEIGHT(obj) += GET_OBJ_WEIGHT(temp);

        /*
         * The contents of an item must be written directly after the
         * container in order to determine which container to re-place
         * them in when being loaded.
         */
        write_objects(obj->contains, fl, MIN(0, location) - 1);
    }

    return success;
}

static bool write_object_record(struct obj_data *obj, FILE *fl, int location) {
    int i;
    struct extra_descr_data *desc;
    struct spell_book_list *spell;
    trig_data *trig;
    struct trig_var_data *var;

    fprintf(fl, "vnum: %d\n", GET_OBJ_VNUM(obj));
    fprintf(fl, "location: %d\n", location);

    fprintf(fl, "values:\n");
    for (i = 0; i < NUM_VALUES; ++i)
        fprintf(fl, "%d\n", GET_OBJ_VAL(obj, i));
    fprintf(fl, "~\n");

    fprintf(fl, "flags: ");
    write_ascii_flags(fl, GET_OBJ_FLAGS(obj), NUM_ITEM_FLAGS);
    fprintf(fl, "\n");

    /* Strings */
    if (obj->name)
        fprintf(fl, "name: %s\n", filter_chars(buf, obj->name, "\r\n"));
    if (obj->short_description)
        fprintf(fl, "shortdesc: %s\n", filter_chars(buf, obj->short_description, "\r\n"));
    if (obj->description)
        fprintf(fl, "desc: %s\n", filter_chars(buf, obj->description, "\r\n"));
    if (obj->action_description)
        fprintf(fl, "adesc:\n%s~\n", filter_chars(buf, obj->action_description, "\r~"));

    fprintf(fl, "type: %d\n", GET_OBJ_TYPE(obj));
    fprintf(fl, "weight: %.2f\n", GET_OBJ_WEIGHT(obj));
    fprintf(fl, "cost: %d\n", GET_OBJ_COST(obj));
    fprintf(fl, "timer: %d\n", GET_OBJ_TIMER(obj));
    if (IS_CORPSE(obj))
        fprintf(fl, "decomp: %d\n", GET_OBJ_DECOMP(obj));
    fprintf(fl, "level: %d\n", GET_OBJ_LEVEL(obj));
    fprintf(fl, "effects: ");
    write_ascii_flags(fl, GET_OBJ_EFF_FLAGS(obj), NUM_EFF_FLAGS);
    fprintf(fl, "\n");
    fprintf(fl, "wear: %d\n", GET_OBJ_WEAR(obj));

    for (i = 0; i < MAX_OBJ_APPLIES && !obj->applies[i].location; ++i)
        ;
    if (i < MAX_OBJ_APPLIES) {
        fprintf(fl, "applies:\n");
        for (i = 0; i < MAX_OBJ_APPLIES; ++i)
            if (obj->applies[i].modifier && obj->applies[i].location)
                fprintf(fl, "%d %d\n", obj->applies[i].location, obj->applies[i].modifier);
        fprintf(fl, "~\n");
    }

    if (obj->ex_description)
        for (desc = obj->ex_description; desc; desc = desc->next)
            if (desc->keyword && *desc->keyword && desc->description && *desc->description)
                fprintf(fl, "extradesc: %s\n%s~\n", filter_chars(buf1, desc->keyword, "\r\n"),
                        filter_chars(buf2, desc->description, "\r~"));

    /* Spellbooks */
    if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK && obj->spell_book) {
        fprintf(fl, "spells:\n");
        for (spell = obj->spell_book; spell; spell = spell->next)
            fprintf(fl, "%d %d\n", spell->spell, spell->length);
        fprintf(fl, "~\n");
    }

    /* Scripts */
    if (SCRIPT(obj)) {
        /* Trigger list */
        if (TRIGGERS(SCRIPT(obj))) {
            fprintf(fl, "triggers:\n");
            for (trig = TRIGGERS(SCRIPT(obj)); trig; trig = trig->next)
                fprintf(fl, "%d\n", GET_TRIG_VNUM(trig));
            fprintf(fl, "~\n");
        }

        /* Global variables */
        if (SCRIPT(obj)->global_vars) {
            fprintf(fl, "variables:\n");
            for (var = SCRIPT(obj)->global_vars; var; var = var->next)
                if (*var->name != '~')
                    fprintf(fl, "%s %s\n", var->name, filter_chars(buf, var->value, "\r\n"));
            fprintf(fl, "~\n");
        }
    }

    fprintf(fl, "~~\n");

    return TRUE;
}

void show_rent(struct char_data *ch, char *argument) {
    char name[MAX_INPUT_LENGTH];
    FILE *fl;
    struct char_data *tch;
    int i;
    bool found;

    any_one_arg(argument, name);

    if (!*name) {
        send_to_char("Show rent for whom?\r\n", ch);
        return;
    }

    fl = open_player_obj_file(name, ch, FALSE);
    if (!fl)
        return;

    name[0] = UPPER(name[0]);

    if (!get_line(fl, buf)) {
        send_to_char("Error reading reading rent code.\r\n", ch);
        return;
    }

    if (!is_integer(buf)) {
        send_to_char("This file is in the obsolete binary format.  Please use 'objupdate' to fix it.\r\n", ch);
        return;
    }

    tch = create_char();
    load_player(name, tch);
    char_to_room(tch, 0);

    read_objects(tch, fl);

    fclose(fl);

    for (i = 0, found = FALSE; i < NUM_WEARS; ++i)
        if (GET_EQ(tch, i)) {
            found = TRUE;
            break;
        }

    if (found) {
        act("\r\n$N is wearing:", FALSE, ch, 0, tch, TO_CHAR);
        for (i = 0; i < NUM_WEARS; ++i)
            if (GET_EQ(tch, wear_order_index[i]))
                list_objects(GET_EQ(tch, wear_order_index[i]), ch, strlen(where[wear_order_index[i]]), 0,
                             where[wear_order_index[i]]);
    }

    if (tch->carrying) {
        act("\r\n$N is carrying:", FALSE, ch, 0, tch, TO_CHAR);
        list_objects(tch->carrying, ch, 1, 0, NULL);
    }

    extract_objects(tch);

    /* Set this so extract_char() doesn't try to do an emergency save */
    SET_FLAG(PLR_FLAGS(tch), PLR_REMOVING);
    extract_char(tch);
}

static void list_objects(struct obj_data *list, struct char_data *ch, int indent, int last_indent,
                         const char *first_indent) {
    struct obj_data *i, *j, *display;
    int pos, num;
    static char buf[100];

#define PRETTY_INDENTATION FALSE
#define OBJECTS_MATCH(x, y)                                                                                            \
    ((x)->item_number == (y)->item_number &&                                                                           \
     ((x)->short_description == (y)->short_description || !strcmp((x)->short_description, (y)->short_description)) &&  \
     (x)->contains == (y)->contains)

    /* Loop through the list of objects */
    for (i = list; i; i = i->next_content) {
        /* Set to 0 in case this object can't be seen */
        num = 0;

        if (!PRF_FLAGGED(ch, PRF_EXPAND_OBJS)) {
            /* Check the list to see if we've already counted this object */
            for (j = list; j != i; j = j->next_content)
                if (OBJECTS_MATCH(i, j))
                    break; /* found a matching object */
            if (i != j)
                continue; /* we counted object i earlier in the list */

            /* Count matching objects, including this one */
            for (display = j = i; j; j = j->next_content)
                if (OBJECTS_MATCH(i, j) && CAN_SEE_OBJ(ch, j)) {
                    ++num;
                    /* If the original item can't be seen, switch it for this one */
                    if (display == i && !CAN_SEE_OBJ(ch, display))
                        display = j;
                }
        } else if (CAN_SEE_OBJ(ch, i)) {
            display = i;
            num = 1;
        }

        /* This object or one like it can be seen by the char, so show it */
        if (num > 0) {
            buf[pos = MIN(indent, sizeof(buf) - 1)] = '\0';
            while (pos >= last_indent)
                buf[--pos] = ' ';
            if (first_indent)
                cprintf(ch, "%s", first_indent);
            else
                cprintf(ch, "%s", buf);
            if (num != 1)
                cprintf(ch, "[%d] ", num);
            print_obj_to_char(display, ch, SHOW_SHORT_DESC | SHOW_FLAGS, NULL);
            if (display->contains) {
                list_objects(display->contains, ch, indent + 3, indent, NULL);
                last_indent = indent;
            }
        }
    }
}

void save_quests(struct char_data *ch) {
    struct quest_list *curr;
    FILE *fp;
    char fname[PLAYER_FILENAME_LENGTH], frename[PLAYER_FILENAME_LENGTH];

    if (!get_pfilename(GET_NAME(ch), fname, TEMP_FILE)) {
        sprintf(buf, "SYSERR: save_quests() couldn't get temp file name for %s.", GET_NAME(ch));
        log(buf);
        return;
    }

    if (!get_pfilename(GET_NAME(ch), frename, QUEST_FILE)) {
        sprintf(buf, "SYSERR: save_quests() couldn't get quest file name for %s.", GET_NAME(ch));
        log(buf);
        return;
    }

    if (!(fp = fopen(fname, "w"))) {
        sprintf(buf, "SYSERR: save_quests() couldn't open file %s for write", fname);
        mudlog(buf, NRM, LVL_GOD, TRUE);
        return;
    }

    curr = ch->quests;
    while (curr) {
        int var_count;
        struct quest_var_list *vars;

        var_count = 0;
        vars = curr->variables;
        while (vars) {
            var_count++;
            vars = vars->next;
        }

        vars = curr->variables;

        fprintf(fp, "%d %d %d\n", curr->quest_id, curr->stage, var_count);

        while (vars) {
            struct quest_var_list *temp;
            temp = vars->next;

            fprintf(fp, "%s %s\n", vars->var, vars->val);

            vars = temp;
        }

        curr = curr->next;
    }

    if (fclose(fp)) {
        sprintf(buf, "SYSERR: Error closing quest file for %s after write", GET_NAME(ch));
        log(buf);
    } else if (rename(fname, frename)) {
        sprintf(buf, "SYSERR: Error renaming quest file for %s after write", GET_NAME(ch));
        log(buf);
    }
}

void save_pets(struct char_data *ch) {
    FILE *fp;
    char fname[PLAYER_FILENAME_LENGTH], frename[PLAYER_FILENAME_LENGTH];
    struct follow_type *k;

    if (!get_pfilename(GET_NAME(ch), fname, TEMP_FILE)) {
        sprintf(buf, "SYSERR: save_pets() couldn't get temp file name for %s.", GET_NAME(ch));
        log(buf);
        return;
    }

    if (!get_pfilename(GET_NAME(ch), frename, PET_FILE)) {
        sprintf(buf, "SYSERR: save_pets() couldn't get pet file name for %s.", GET_NAME(ch));
        log(buf);
        return;
    }

    if (!(fp = fopen(fname, "w"))) {
        sprintf(buf, "SYSERR: save_pets() couldn't open file %s for write", fname);
        mudlog(buf, NRM, LVL_GOD, TRUE);
        return;
    }

    for (k = ch->followers; k; k = k->next) {
        if (IS_PET(k->follower) && k->follower->master == ch) {
            fprintf(fp, "%d\n", GET_MOB_VNUM(k->follower));
            fprintf(fp, "namelist: %s\n", GET_NAMELIST(k->follower));
            fprintf(fp, "desc: %s\n", GET_DESCRIPTION(k->follower));
            fprintf(fp, "$\n");
        }
    }

    if (fclose(fp)) {
        sprintf(buf, "SYSERR: Error closing pet file for %s after write", GET_NAME(ch));
        log(buf);
    } else if (rename(fname, frename)) {
        sprintf(buf, "SYSERR: Error renaming pet file for %s after write", GET_NAME(ch));
        log(buf);
    }
}

int delete_objects_file(char *name) {
    char filename[50];
    FILE *fl;

    if (!get_pfilename(name, filename, OBJ_FILE))
        return 0;
    if (!(fl = fopen(filename, "r"))) {
        if (errno != ENOENT) { /* if it fails but NOT because of no file */
            sprintf(buf1, "SYSERR: deleting object file %s (1)", filename);
            perror(buf1);
        }
        return 0;
    }
    fclose(fl);

    if (unlink(filename) < 0) {
        if (errno != ENOENT) { /* if it fails, NOT because of no file */
            sprintf(buf1, "SYSERR: deleting object file %s (2)", filename);
            perror(buf1);
        }
    }
    return (1);
}

bool delete_player_obj_file(struct char_data *ch) {
    char fname[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    FILE *fl;
    int rent_code;

    if (!get_pfilename(GET_NAME(ch), fname, OBJ_FILE))
        return FALSE;

    if (!(fl = fopen(fname, "r"))) {
        if (errno != ENOENT) { /* if it fails, NOT because of no file */
            sprintf(buf1, "SYSERR: checking for object file %s (3)", fname);
            perror(buf1);
        }
        return FALSE;
    }

    if (get_line(fl, buf)) {
        rent_code = atoi(buf);
        if (rent_code == SAVE_AUTO)
            delete_objects_file(GET_NAME(ch));
    }

    fclose(fl);

    return TRUE;
}

static int auto_equip(struct char_data *ch, struct obj_data *obj, int location) {
    if (location >= 0 && location != WEAR_INVENTORY) {
        if (GET_OBJ_LEVEL(obj) > GET_LEVEL(ch))
            location = WEAR_INVENTORY;
        if (location != WEAR_INVENTORY && !CAN_WEAR(obj, wear_flags[location]))
            location = WEAR_INVENTORY;
        /* Check alignment to prevent character from being zapped */
        if ((OBJ_FLAGGED(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) || (OBJ_FLAGGED(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
            (OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)))
            location = WEAR_INVENTORY;
        if (location != WEAR_INVENTORY) {
            if (!GET_EQ(ch, location))
                equip_char(ch, obj, location);
            else
                location = WEAR_INVENTORY;
        }
    }
    return location;
}

/*
 * Code to auto-equip objects from legacy binary object files.
 */
static int binary_auto_equip(struct char_data *ch, struct obj_data *obj, int location) {
    if (location > 0)
        location = (auto_equip(ch, obj, location - 1) != WEAR_INVENTORY);
    if (location <= 0)
        obj_to_char(obj, ch);
    return location;
}

/*
 * Parses and allocates memory for an object from a file.  Returns
 * TRUE if successful and FALSE otherwise.  When TRUE is returned,
 * *obj will point to the object and *location will be the object's
 * location when saved (negative numbers denote objects within
 * containers).
 */
bool build_object(FILE *fl, struct obj_data **objp, int *location) {
    struct obj_data *obj, *proto;
    int num, num2, apply = 0;
    float f;
    char line[MAX_INPUT_LENGTH], tag[128], *value;
    struct extra_descr_data *desc, *last_desc = NULL;
    struct spell_book_list *spell, *last_spell;
    trig_data *trig;

    extern void add_var(struct trig_var_data * *var_list, const char *name, const char *value);

    if (!objp || !location) {
        sprintf(buf, "SYSERR: Invalid obj or location pointers passed to build_object");
        log(buf);
        return FALSE;
    }

    *objp = obj = create_obj();
    *location = WEAR_INVENTORY;

    while (get_line(fl, line)) {
        if (!strcmp(line, "~~"))
            break;

        tag_argument(line, tag);
        num = atoi(line);
        f = atof(line);

        switch (UPPER(*tag)) {
        case 'A':
            if (!strcmp(tag, "adesc"))
                obj->action_description = fread_string(fl, "build_object");
            else if (!strcmp(tag, "applies")) {
                while (get_line(fl, line) && *line != '~' && apply < MAX_OBJ_APPLIES) {
                    sscanf(line, "%d %d", &num, &num2);
                    obj->applies[apply].location = LIMIT(0, num, NUM_APPLY_TYPES - 1);
                    obj->applies[apply].modifier = num2;
                    ++apply;
                }
            } else
                goto bad_tag;
            break;
        case 'C':
            if (!strcmp(tag, "cost"))
                GET_OBJ_COST(obj) = MAX(0, num);
            else
                goto bad_tag;
            break;
        case 'D':
            if (!strcmp(tag, "desc"))
                obj->description = strdup(line);
            else if (!strcmp(tag, "decomp"))
                GET_OBJ_DECOMP(obj) = MAX(0, num);
            else
                goto bad_tag;
            break;
        case 'E':
            if (!strcmp(tag, "effects"))
                load_ascii_flags(GET_OBJ_EFF_FLAGS(obj), NUM_EFF_FLAGS, line);
            else if (!strcmp(tag, "extradesc")) {
                CREATE(desc, struct extra_descr_data, 1);
                desc->keyword = strdup(line);
                desc->description = fread_string(fl, "build_object");
                if (last_desc)
                    last_desc->next = desc;
                else
                    obj->ex_description = desc;
                last_desc = desc;
            } else
                goto bad_tag;
            break;
        case 'F':
            if (!strcmp(tag, "flags"))
                load_ascii_flags(GET_OBJ_FLAGS(obj), NUM_ITEM_FLAGS, line);
            else
                goto bad_tag;
            break;
        case 'H':
            if (!strcmp(tag, "hiddenness"))
                GET_OBJ_HIDDENNESS(obj) = LIMIT(0, num, 1000);
            else
                goto bad_tag;
            break;
        case 'L':
            if (!strcmp(tag, "location"))
                *location = num;
            else if (!strcmp(tag, "level"))
                GET_OBJ_LEVEL(obj) = LIMIT(0, num, LVL_IMPL);
            else
                goto bad_tag;
            break;
        case 'N':
            if (!strcmp(tag, "name"))
                obj->name = strdup(line);
            else
                goto bad_tag;
            break;
        case 'S':
            if (!strcmp(tag, "shortdesc"))
                obj->short_description = strdup(line);
            else if (!strcmp(tag, "spells")) {
                for (last_spell = obj->spell_book; last_spell && last_spell->next; last_spell = last_spell->next)
                    ;
                while (get_line(fl, line) && *line != '~') {
                    CREATE(spell, struct spell_book_list, 1);
                    sscanf(line, "%d %d", &spell->spell, &spell->length);
                    if (last_spell)
                        last_spell->next = spell;
                    else /* This means obj->spell_book is NULL */
                        obj->spell_book = spell;
                    last_spell = spell;
                }
            } else
                goto bad_tag;
            break;
        case 'T':
            if (!strcmp(tag, "type"))
                GET_OBJ_TYPE(obj) = LIMIT(0, num, NUM_ITEM_TYPES - 1);
            else if (!strcmp(tag, "timer"))
                GET_OBJ_TIMER(obj) = MAX(0, num);
            else if (!strcmp(tag, "triggers")) {
                if (!SCRIPT(obj))
                    CREATE(SCRIPT(obj), struct script_data, 1);
                while (get_line(fl, line) && *line != '~') {
                    num = real_trigger(atoi(line));
                    if (num != NOTHING && (trig = read_trigger(num)))
                        add_trigger(SCRIPT(obj), trig, -1);
                }
            } else
                goto bad_tag;
            break;
        case 'V':
            if (!strcmp(tag, "vnum"))
                obj->item_number = real_object(num);
            else if (!strcmp(tag, "values")) {
                num = 0;
                while (get_line(fl, line) && *line != '~')
                    if (num < NUM_VALUES)
                        GET_OBJ_VAL(obj, num++) = atoi(line);
                limit_obj_values(obj);
            } else if (!strcmp(tag, "variables")) {
                if (!SCRIPT(obj))
                    CREATE(SCRIPT(obj), struct script_data, 1);
                while (get_line(fl, line) && *line != '~') {
                    for (value = line; *value; ++value)
                        if (*value == ' ') {
                            *(value++) = '\0';
                            break;
                        }
                    add_var(&SCRIPT(obj)->global_vars, line, value);
                }
            } else
                goto bad_tag;
            break;
        case 'W':
            if (!strcmp(tag, "weight"))
                GET_OBJ_WEIGHT(obj) = MAX(0, f);
            else if (!strcmp(tag, "wear"))
                GET_OBJ_WEAR(obj) = num;
            else
                goto bad_tag;
            break;
        default:
        bad_tag:
            sprintf(buf, "SYSERR: Unknown tag %s in rent file: %s", tag, line);
            log(buf);
            break;
        }
    }

    if (feof(fl)) {
        extract_obj(obj);
        return FALSE;
    }

    /*
     * Check to see if the loaded object has strings that match the
     * prototype.  If so, replace them.
     */
#define CHECK_PROTO_STR(address)                                                                                       \
    do {                                                                                                               \
        if (!obj->address)                                                                                             \
            obj->address = proto->address;                                                                             \
        else if (!*obj->address || (proto->address && *proto->address && !strcmp(obj->address, proto->address))) {     \
            free(obj->address);                                                                                        \
            obj->address = proto->address;                                                                             \
        }                                                                                                              \
    } while (FALSE);
#define CHECK_NULL_STR(address, str)                                                                                   \
    do {                                                                                                               \
        if (!*obj->address) {                                                                                          \
            free(obj->address);                                                                                        \
            obj->address = NULL;                                                                                       \
        }                                                                                                              \
        if (!obj->address)                                                                                             \
            obj->address = strdup(str);                                                                                \
    } while (FALSE);

    if (GET_OBJ_RNUM(obj) != NOTHING) {
        obj_index[GET_OBJ_RNUM(obj)].number++;

        proto = &obj_proto[GET_OBJ_RNUM(obj)];

        /* What with object weights being recalibrated, we'll overwrite them from
         * player files here. For a while...
         * Once the weights are all updated, it would be a good idea to linkload
         * and save everyone, to change all their object weights. */
        GET_OBJ_WEIGHT(obj) = GET_OBJ_WEIGHT(proto);
        /* We also need to fix the liquid container weights, since they will
         * have been set to empty just now. */
        if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
            setup_drinkcon(obj, -1);
        /* END of weight messing-around-with */

        CHECK_PROTO_STR(name);
        CHECK_PROTO_STR(description);
        CHECK_PROTO_STR(short_description);
        CHECK_PROTO_STR(action_description);
        /* See if -all- the extra descriptions are identical. */
        if (obj->ex_description) {
            num = TRUE;
            for (desc = obj->ex_description, last_desc = proto->ex_description; desc && last_desc;
                 desc = desc->next, last_desc = last_desc->next)
                if (strcmp(desc->keyword, last_desc->keyword) || strcmp(desc->description, last_desc->description))
                    num = FALSE;
            if (desc || last_desc)
                num = FALSE;
            if (num) {
                for (desc = obj->ex_description; desc; desc = last_desc) {
                    last_desc = desc->next;
                    free(desc->keyword);
                    free(desc->description);
                    free(desc);
                }
                obj->ex_description = proto->ex_description;
            }
        }
        obj->proto_script = proto->proto_script;
    }

    /* Still no strings?  Put in some defaults so we don't have null
     * pointers floating around. */
    CHECK_NULL_STR(name, "item undefined-item");
    CHECK_NULL_STR(short_description, "an undefined item");
    CHECK_NULL_STR(description, "An undefined item sits here.");

#undef CHECK_PROTO_STR
#undef CHECK_NULL_STR

    return TRUE;
}

bool load_objects(struct char_data *ch) {
    FILE *fl;
    char line[MAX_INPUT_LENGTH];

    fl = open_player_obj_file(GET_NAME(ch), NULL, FALSE);
    if (!fl)
        return FALSE;

    if (!(get_line(fl, line) && is_integer(line))) {
        /* Object file may be in the 'old' format.  Attempt to load thusly. */
        sprintf(buf, "Invalid rent code for %s: attempting to load via legacy code...", GET_NAME(ch));
        log(buf);
        fclose(fl);
        if (load_binary_objects(ch)) {
            log("   Success!");
            return TRUE;
        } else {
            log("   Failed!");
            send_to_char(
                "\r\n@W********************* NOTICE *********************\r\n"
                "There was a problem (error 02) loading your objects from disk.\r\n"
                "Contact a God for assistance.@0\r\n",
                ch);
            sprintf(buf, "%s entering game with no equipment. (error 02)", GET_NAME(ch));
            mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
            return FALSE;
        }
    }

    read_objects(ch, fl);
    fclose(fl);

    return TRUE;
}

static void read_objects(struct char_data *ch, FILE *fl) {
    int i, depth, location;
    struct obj_data *obj, *containers[MAX_CONTAINER_DEPTH];

    for (i = 0; i < MAX_CONTAINER_DEPTH; ++i)
        containers[i] = NULL;

    while (!feof(fl)) {
        if (!build_object(fl, &obj, &location))
            break;
        location = auto_equip(ch, obj, location);
        depth = MAX(0, -location);
        for (i = MAX_CONTAINER_DEPTH - 1; i >= depth; --i)
            containers[i] = NULL;
        containers[depth] = obj;
        if (location < 0) {
            if (containers[depth - 1])
                obj_to_obj(obj, containers[depth - 1]);
            else
                location = WEAR_INVENTORY;
        }
        if (location == WEAR_INVENTORY)
            obj_to_char(obj, ch);
    }
}

void load_quests(struct char_data *ch) {
    FILE *fl;
    char fname[MAX_STRING_LENGTH];
    int n;
    struct quest_list *plyrqsts, *curr;
    int qid, qst, qnum_vars;
    char var_name[21];
    char var_val[21];
    bool skipquest, duplicates = FALSE, nonexistent = FALSE;
    struct quest_var_list *last_var;

    if (!get_pfilename(GET_NAME(ch), fname, QUEST_FILE)) {
        ch->quests = (struct quest_list *)NULL;
    } else {
        if (!(fl = fopen(fname, "r"))) {
            if (errno != ENOENT) { /* if it fails, NOT because of no file */
                sprintf(buf1, "SYSERR: READING QUEST FILE %s (5)", fname);
                perror(buf1);
                send_to_char(
                    "\r\n********************* NOTICE *********************\r\n"
                    "There was a problem loading your quests from disk.\r\n"
                    "Contact a God for assistance.\r\n",
                    ch);
            }
            sprintf(buf, "%s starting up with no quests.", GET_NAME(ch));
            mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
            ch->quests = (struct quest_list *)NULL;
        } else {
            while (!feof(fl)) {
                char buf2[256];
                get_line(fl, buf2);

                if (feof(fl))
                    break;

                if (sscanf(buf2, "%i %i %i\n", &qid, &qst, &qnum_vars) == 2) {
                    qnum_vars = 0;
                }

                skipquest = FALSE;

                /* See if there's a quest duplication bug
                 * (see if the char already has this quest) */
                for (curr = ch->quests; curr; curr = curr->next) {
                    if (curr->quest_id == qid) {
                        if (!duplicates) {
                            duplicates = TRUE;
                            sprintf(buf,
                                    "SYSERR: Player %s had duplicate of quest %d (skipped) "
                                    "(possibly more)",
                                    GET_NAME(ch), qid);
                            mudlog(buf, NRM, LVL_GOD, TRUE);
                        }
                        skipquest = TRUE;
                        break;
                    }
                }

                /* This quest isn't valid for whatever reason. */
                if (!skipquest && real_quest(qid) < 0) {
                    skipquest = TRUE;
                    if (!nonexistent) {
                        nonexistent = TRUE;
                        sprintf(buf,
                                "SYSERR: Player %s had nonexistent quest %d (skipped) "
                                "(possibly more)",
                                GET_NAME(ch), qid);
                        mudlog(buf, NRM, LVL_GOD, TRUE);
                    }
                }

                if (skipquest) {
                    for (n = 0; n < qnum_vars; n++) {
                        /* Eat the data */
                        if (fscanf(fl, "%s %s\n", var_name, var_val) < 2)
                            break;
                    }
                    continue;
                }

                CREATE(curr, struct quest_list, 1);
                curr->quest_id = qid;
                curr->stage = qst;
                curr->variables = NULL;

                if (qnum_vars) {
                    int n = 0;
                    last_var = NULL;

                    while (n < qnum_vars) {
                        fscanf(fl, "%s %s\n", var_name, var_val);

                        if (last_var == NULL) {
                            CREATE(curr->variables, struct quest_var_list, 1);
                            CREATE(curr->variables->var, char, 21);
                            CREATE(curr->variables->val, char, 21);

                            strncpy(curr->variables->var, var_name, 20);
                            strncpy(curr->variables->val, var_val, 20);

                            curr->variables->var[20] = '\0';
                            curr->variables->val[20] = '\0';

                            curr->variables->next = NULL;

                            last_var = curr->variables;
                        } else {
                            CREATE(last_var->next, struct quest_var_list, 1);
                            CREATE(last_var->next->var, char, 21);
                            CREATE(last_var->next->val, char, 21);

                            strncpy(last_var->next->var, var_name, 20);
                            strncpy(last_var->next->val, var_val, 20);

                            last_var->next->var[20] = '\0';
                            last_var->next->val[20] = '\0';

                            last_var->next->next = NULL;

                            last_var = last_var->next;
                        }

                        n++;
                    }
                }

                curr->next = (struct quest_list *)NULL;
                plyrqsts = ch->quests;
                if (plyrqsts == (struct quest_list *)NULL)
                    ch->quests = curr;
                else {
                    while (plyrqsts->next)
                        plyrqsts = plyrqsts->next;
                    plyrqsts->next = curr;
                }
            }
            fclose(fl);
        }
    } /* Done getting quest info */
}

void load_pets(struct char_data *ch) {
    FILE *fl;
    char fname[MAX_STRING_LENGTH], line[MAX_INPUT_LENGTH], tag[128];
    struct char_data *pet;

    if (!get_pfilename(GET_NAME(ch), fname, PET_FILE))
        return;
    if (!(fl = fopen(fname, "r"))) {
        if (errno != ENOENT) { /* if it fails, NOT because of no file */
            sprintf(buf1, "SYSERR: READING PET FILE %s (5)", fname);
            perror(buf1);
        }
    } else {
        while (!feof(fl)) {
            get_line(fl, line);
            if (is_integer(line)) {
                pet = read_mobile(atoi(line), VIRTUAL);
                if (!pet) {
                    sprintf(buf1, "Attempt to load nonexisting pet vnum %s for player %s.", line, GET_NAME(ch));
                    perror(buf1);
                    break;
                }

                GET_EXP(pet) = 0;
                SET_FLAG(EFF_FLAGS(pet), EFF_CHARM);
                SET_FLAG(MOB_FLAGS(pet), MOB_PET);

                /* Over kill, but we might want to add more abilities soon. */
                while (get_line(fl, line)) {
                    tag_argument(line, tag);
                    if (*tag == '$')
                        break;

                    if (!strcmp(tag, "desc"))
                        GET_DESCRIPTION(pet) = strdup(line);
                    else if (!strcmp(tag, "namelist"))
                        GET_NAMELIST(pet) = strdup(line);
                    else {
                        sprintf(buf, "SYSERR: Unknown tag %s in %s's pet file: %s", tag, GET_NAME(ch), line);
                        perror(buf);
                    }
                }
                char_to_room(pet, ch->in_room);
                add_follower(pet, ch);
            }
        }
        fclose(fl);
    }
}

static struct obj_data *restore_binary_object(struct obj_file_elem *store, int *locate) {
    struct obj_data *obj;
    int j = 0, rnum;
    char *list_parse, *spell_parse, *list;
    struct spell_book_list *entry;

    if ((rnum = real_object(store->item_number)) < 0)
        return NULL;

    obj = read_object(store->item_number, VIRTUAL);
    *locate = (int)store->locate;
    GET_OBJ_VAL(obj, 0) = store->value[0];
    GET_OBJ_VAL(obj, 1) = store->value[1];
    GET_OBJ_VAL(obj, 2) = store->value[2];
    GET_OBJ_VAL(obj, 3) = store->value[3];
    GET_OBJ_FLAGS(obj)[0] = store->extra_flags;
    GET_OBJ_WEIGHT(obj) = store->weight;
    GET_OBJ_TIMER(obj) = store->timer;
    GET_OBJ_HIDDENNESS(obj) = store->hiddenness;

    /* Use the prototype's values for these, since they were usually saved in a
     * corrupted state in binary files. */
    GET_OBJ_EFF_FLAGS(obj)[0] = obj_proto[rnum].obj_flags.effect_flags[0];
    GET_OBJ_EFF_FLAGS(obj)[1] = obj_proto[rnum].obj_flags.effect_flags[1];
    GET_OBJ_EFF_FLAGS(obj)[2] = obj_proto[rnum].obj_flags.effect_flags[2];

    /* Handling spellbooks
     *
     * The spells written in it have been stored as a string, which
     * must be parsed so that the spell list can be restored.
     */
    if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
        GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES) = LEGACY_MAX_SPELLBOOK_PAGES;
        if (store->spells_in_book[0]) {
            list = store->spells_in_book;
            CREATE(obj->spell_book, struct spell_book_list, 1);
            entry = obj->spell_book;
            while ((list_parse = strsep(&list, ",")) && strlen(store->spells_in_book)) {
                if (list_parse && strlen(list_parse)) {
                    if (j > 0) {
                        CREATE(entry->next, struct spell_book_list, 1);
                        entry = entry->next;
                    }

                    spell_parse = strsep(&list_parse, "_");
                    if (!spell_parse || !*spell_parse) {
                        /* Corrupt spell list - just put magic missile */
                        entry->spell = SPELL_MAGIC_MISSILE;
                        entry->length = 1;
                        sprintf(buf,
                                "SYSERR: restore_binary_object() found corrupt spellbook "
                                "list '%s'",
                                store->spells_in_book);
                        log(buf);
                    } else {
                        entry->spell = atoi(spell_parse);
                        spell_parse = strsep(&list_parse, "_");
                        if (!spell_parse || !*spell_parse) {
                            /* Length corrupt - just put 1 page */
                            entry->length = 1;
                            sprintf(buf,
                                    "SYSERR: restore_binary_object() found corrupt spellbook "
                                    "list '%s'",
                                    store->spells_in_book);
                            log(buf);
                        } else {
                            entry->length = atoi(spell_parse);
                        }
                    }
                    j++;
                }
            }
        }
    }

    for (j = 0; j < LEGACY_MAX_OBJ_AFFECT; j++) {
        obj->applies[j].location = store->affected[j].location;
        obj->applies[j].modifier = store->affected[j].modifier;
    }

    return obj;
}

static bool load_binary_objects(struct char_data *ch) {
    FILE *fl;
    struct obj_file_elem object;
    struct rent_info rent;
    struct obj_data *obj;
    int locate, j, eq = 0;
    struct obj_data *obj1;
    struct obj_data *cont_row[MAX_CONTAINER_DEPTH];

    fl = open_player_obj_file(GET_NAME(ch), NULL, FALSE);
    if (!fl)
        return FALSE;

    if (!feof(fl))
        fread(&rent, sizeof(struct rent_info), 1, fl);

    for (j = 0; j < MAX_CONTAINER_DEPTH; j++)
        cont_row[j] = NULL; /* empty all cont lists (you never know ...) */

    while (!feof(fl)) {
        eq = 0;
        fread(&object, sizeof(struct obj_file_elem), 1, fl);
        if (ferror(fl)) {
            perror("Reading object file: load_binary_objects()");
            fclose(fl);
            return TRUE;
        }

        if (!feof(fl))
            if ((obj = restore_binary_object(&object, &locate))) {
                eq = binary_auto_equip(ch, obj, locate);
                /* 5/5/01 - Zantir - Found a bug in the auto_equip code
                   that would crash the mud if a player couldn't wear an
                   container that the mud thought they were already
                   wearing. So now the auto_equip function returns a value,
                   if a 1 isn't returned the assume the item can't be
                   reworn so set the locate value to 0. */
                if (eq == 0)
                    locate = 0;

                /*
                 * what to do with a new loaded item:
                 *
                 * if there's a list with <locate> less than 1 below this:
                 * (equipped items are assumed to have <locate>==0 here) then its
                 * container has disappeared from the file   *gasp*
                 * -> put all the list back to ch's inventory
                 *
                 * if there's a list of contents with <locate> 1 below this:
                 * check if it's a container
                 * - if so: get it from ch, fill it, and give it back to ch (this way
                 * the container has its correct weight before modifying ch)
                 * - if not: the container is missing -> put all the list to ch's
                 * inventory
                 *
                 * for items with negative <locate>:
                 * if there's already a list of contents with the same <locate> put obj
                 * to it if not, start a new list
                 *
                 * Confused? Well maybe you can think of some better text to be put here
                 * ...
                 *
                 * since <locate> for contents is < 0 the list indices are switched to
                 * non-negative
                 */

                if (locate > 0) { /* item equipped */
                    for (j = MAX_CONTAINER_DEPTH - 1; j > 0; --j)
                        if (cont_row[j]) { /* no container -> back to ch's inventory */
                            for (; cont_row[j]; cont_row[j] = obj1) {
                                obj1 = cont_row[j]->next_content;
                                obj_to_char(cont_row[j], ch);
                            }
                            cont_row[j] = NULL;
                        }
                    if (cont_row[0]) { /* content list existing */
                        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
                            /* rem item ; fill ; equip again */
                            obj = unequip_char(ch, locate - 1);
                            obj->contains = NULL; /* should be empty - but who knows */
                            for (; cont_row[0]; cont_row[0] = obj1) {
                                obj1 = cont_row[0]->next_content;
                                obj_to_obj(cont_row[0], obj);
                            }
                            equip_char(ch, obj, locate - 1);
                        } else { /* object isn't container -> empty content list */
                            for (; cont_row[0]; cont_row[0] = obj1) {
                                obj1 = cont_row[0]->next_content;
                                obj_to_char(cont_row[0], ch);
                            }
                            cont_row[0] = NULL;
                        }
                    }
                } else { /* locate <= 0 */
                    for (j = MAX_CONTAINER_DEPTH - 1; j > -locate; --j)
                        if (cont_row[j]) { /* no container -> back to ch's inventory */
                            for (; cont_row[j]; cont_row[j] = obj1) {
                                obj1 = cont_row[j]->next_content;
                                obj_to_char(cont_row[j], ch);
                            }
                            cont_row[j] = NULL;
                        }

                    if (j == -locate && cont_row[j]) { /* content list existing */
                        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
                            /* take item ; fill ; give to char again */
                            obj_from_char(obj);
                            obj->contains = NULL;
                            for (; cont_row[j]; cont_row[j] = obj1) {
                                obj1 = cont_row[j]->next_content;
                                obj_to_obj(cont_row[j], obj);
                            }
                            obj_to_char(obj, ch); /* add to inv first ... */
                        } else {                  /* object isn't container -> empty content list */
                            for (; cont_row[j]; cont_row[j] = obj1) {
                                obj1 = cont_row[j]->next_content;
                                obj_to_char(cont_row[j], ch);
                            }
                            cont_row[j] = NULL;
                        }
                    }

                    if (locate < 0 && locate >= -MAX_CONTAINER_DEPTH) {
                        /* let obj be part of content list
                           but put it at the list's end thus having the items
                           in the same order as before renting */
                        obj_from_char(obj);
                        if ((obj1 = cont_row[-locate - 1])) {
                            while (obj1->next_content)
                                obj1 = obj1->next_content;
                            obj1->next_content = obj;
                        } else
                            cont_row[-locate - 1] = obj;
                    }
                }
            }
    }

    fclose(fl);

    return TRUE;
}

void auto_save_all(void) {
    struct descriptor_data *d;
    for (d = descriptor_list; d; d = d->next) {
        if (d->connected == CON_PLAYING && !IS_NPC(d->character) && PLR_FLAGGED(d->character, PLR_AUTOSAVE)) {
            GET_QUIT_REASON(d->character) = QUIT_AUTOSAVE;
            save_player(d->character);
            REMOVE_FLAG(PLR_FLAGS(d->character), PLR_AUTOSAVE);
        }
    }
}

/*************************************************************************
 * Routines used for the receptionist                                    *
 *************************************************************************/

static int report_unrentables(struct char_data *ch, struct char_data *recep, struct obj_data *obj) {
    int unrentables = 0;

    if (obj) {
        if (is_object_unrentable(obj)) {
            unrentables = 1;
            act("$n tells you, 'You cannot store $t.'", FALSE, recep, (void *)OBJS(obj, ch), ch, TO_VICT);
        }
        unrentables += report_unrentables(ch, recep, obj->contains);
        unrentables += report_unrentables(ch, recep, obj->next_content);
    }
    return unrentables;
}

static int list_unrentables(struct char_data *ch, struct char_data *receptionist) {
    int i, unrentables;

    unrentables = report_unrentables(ch, receptionist, ch->carrying);
    for (i = 0; i < NUM_WEARS; i++)
        unrentables += report_unrentables(ch, receptionist, GET_EQ(ch, i));

    return unrentables;
}

void extract_objects(struct char_data *ch) {
    int i;

    for (i = 0; i < NUM_WEARS; ++i)
        if (GET_EQ(ch, i))
            extract_obj(GET_EQ(ch, i));

    while (ch->carrying)
        extract_obj(ch->carrying);
}

static int gen_receptionist(struct char_data *ch, struct char_data *recep, int cmd, char *arg, int mode) {
    int quit_mode = QUIT_RENT;

    if (!ch->desc || IS_NPC(ch))
        return FALSE;

    if (!CMD_IS("rent"))
        return FALSE;

    if (!AWAKE(recep)) {
        act("$E is unable to talk to you...", FALSE, ch, 0, recep, TO_CHAR);
        return TRUE;
    }

    if (!CAN_SEE(recep, ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        act("$n says, 'I don't deal with people I can't see!'", FALSE, recep, 0, 0, TO_ROOM);
        return TRUE;
    }

    if (list_unrentables(ch, recep)) {
        act("$N shakes $M head at $n.", TRUE, ch, 0, recep, TO_ROOM);
        return TRUE;
    }

    if (mode == SAVE_CRYO) {
        act("$n stores your belongings and helps you into your private chamber.\r\n"
            "A white mist appears in the room, chilling you to the bone...\r\n"
            "You begin to lose consciousness...",
            FALSE, recep, 0, ch, TO_VICT);
        quit_mode = QUIT_CRYO;
        sprintf(buf, "%s has cryo-rented.", GET_NAME(ch));
    } else {
        act("@W$n@W tells you, 'Rent?  Sure, come this way!'\r\n"
            "$U$n stores your belongings and helps you into your private "
            "chamber.&0",
            FALSE, recep, 0, ch, TO_VICT);
        quit_mode = QUIT_RENT;
        sprintf(buf, "%s has rented in %s (%d).", GET_NAME(ch), world[ch->in_room].name, world[ch->in_room].vnum);
    }

    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    act("$n helps $N into $S private chamber.", FALSE, recep, 0, ch, TO_NOTVICT);

    remove_player_from_game(ch, quit_mode);
    return TRUE;
}

SPECIAL(receptionist) { return gen_receptionist(ch, me, cmd, argument, SAVE_RENT); }

SPECIAL(cryogenicist) { return gen_receptionist(ch, me, cmd, argument, SAVE_CRYO); }

/* If quiet==TRUE, minor feedback will be suppressed.
 * But not errors. */
FILE *open_player_obj_file(char *player_name, struct char_data *ch, bool quiet) {
    FILE *fl;
    char filename[MAX_INPUT_LENGTH];

    if (!get_pfilename(player_name, filename, OBJ_FILE)) {
        sprintf(buf, "SYSERR: Unable to construct filename to load objects for %s", GET_NAME(ch));
        log(buf);
        if (ch)
            send_to_char("Couldn't construct the filename!\r\n", ch);
        return NULL;
    }

    if (!(fl = fopen(filename, "r"))) {
        if (errno != ENOENT) {
            sprintf(buf, "SYSERR: READING OBJECT FILE %s (5)", filename);
            perror(buf);
            if (ch) {
                sprintf(buf, "&1&bI/O Error %d&0: %s\r\n", errno, strerror(errno));
                send_to_char(buf, ch);
            }
        } else if (ch && !quiet) {
            sprintf(buf, "There is no object file for %s.\r\n", player_name);
            send_to_char(buf, ch);
        }
        return NULL;
    }
    return fl;
}

/* convert_player_obj_file
 *
 * The current player object file format is ASCII-based.
 * Sometimes, object files in the older format might be present due to
 * restoring players from backups.  This function is used to convert the old
 * file into the ASCII format.
 */
bool convert_player_obj_file(char *player_name, struct char_data *ch) {
    FILE *fl, *fnew;
    char filename[MAX_INPUT_LENGTH];
    char tempfilename[MAX_INPUT_LENGTH];
    char line[MAX_INPUT_LENGTH];
    struct rent_info rent;
    struct obj_file_elem object;
    struct obj_data *obj;
    int locate;

    fl = open_player_obj_file(player_name, ch, TRUE);
    if (!fl)
        return FALSE;

    if (get_line(fl, line) && is_integer(line)) {
        /* File is modern and doesn't need updating */
        fclose(fl);
        return FALSE;
    }

    fclose(fl);
    fl = open_player_obj_file(player_name, ch, TRUE);
    if (!fl) {
        mudlog("SYSERR: convert_player_file couldn't reopen object file!", NRM, LVL_GOD, TRUE);
        return FALSE;
    }

    /* Prepare output file */
    if (!get_pfilename(player_name, filename, OBJ_FILE)) {
        sprintf(buf, "SYSERR: Unable to construct filename to save objects for %s", player_name);
        log(buf);
        fclose(fl);
        return FALSE;
    }

    sprintf(tempfilename, "%s.temp", filename);
    if (!(fnew = fopen(tempfilename, "w"))) {
        sprintf(buf, "SYSERR: Couldn't open player file %s for write", tempfilename);
        mudlog(buf, NRM, LVL_GOD, TRUE);
        fclose(fl);
        return FALSE;
    }

    if (feof(fl)) {
        sprintf(buf, "SYSERR: Object file for %s is too small", player_name);
        log(buf);
        fclose(fl);
        fclose(fnew);
        return FALSE;
    }

    fread(&rent, sizeof(struct rent_info), 1, fl);
    write_rent_code(fnew, rent.rentcode);

    while (!feof(fl)) {
        fread(&object, sizeof(struct obj_file_elem), 1, fl);
        if (ferror(fl)) {
            perror("Reading player object file: convert_player_obj_file()");
            fclose(fl);
            fclose(fnew);
            return FALSE;
        }

        if ((obj = restore_binary_object(&object, &locate))) {
            /* Writing 0 for the location as I don't care to convert it */
            /* Yes, the value we just read would not be correct... */
            write_object_record(obj, fnew, WEAR_INVENTORY);
            extract_obj(obj);
        }
    }

    fclose(fl);
    fclose(fnew);

    if (rename(tempfilename, filename)) {
        sprintf(buf, " * * * Error renaming %s to %s: %s * * *", tempfilename, filename, strerror(errno));
        log(buf);
        return FALSE;
    } else {
        sprintf(buf, "Player object file converted to ASCII format: %s", player_name);
        log(buf);
        if (ch) {
            strcat(buf, "\r\n");
            page_string(ch, buf);
        }
    }

    return TRUE;
}

void convert_player_obj_files(struct char_data *ch) {
    int i;
    int converted = 0;

    for (i = 0; i <= top_of_p_table; ++i) {
        if (convert_player_obj_file(player_table[i].name, ch))
            converted++;
    }

    sprintf(buf, "Examined %d player object file%s and updated %d.\r\n", top_of_p_table + 1,
            top_of_p_table + 1 == 1 ? "" : "s", converted);
    send_to_char(buf, ch);
}

void convert_single_player_obj_file(struct char_data *ch, char *name) {
    if (!convert_player_obj_file(name, ch))
        send_to_char("The object file was not converted.\r\n", ch);
}

/* save_player
 *
 * Saves all data related to a player: character, objects, and quests.
 */
void save_player(struct char_data *ch) {
    int quit_mode;

    REMOVE_FLAG(PLR_FLAGS(ch), PLR_AUTOSAVE);
    quit_mode = GET_QUIT_REASON(ch);

    switch (quit_mode) {
    case QUIT_CAMP:
    case QUIT_RENT:
    case QUIT_TIMEOUT:
    case QUIT_CRYO:
    case QUIT_QUITMORT:
    case QUIT_QUITIMM:
        extract_unrentables(ch);
        break;
    }

    if (quit_mode == QUIT_QUITMORT)
        delete_player_obj_file(ch);
    else
        save_player_objects(ch);
    save_quests(ch);

    save_pets(ch);

    /* Set the load room */
    switch (quit_mode) {
    case QUIT_RENT:
    case QUIT_CRYO:
    case QUIT_CAMP:
    case QUIT_WRENT:
        GET_LOADROOM(ch) = world[ch->in_room].vnum;
        break;
    case QUIT_QUITIMM:
    case QUIT_QUITMORT:
        GET_LOADROOM(ch) = GET_HOMEROOM(ch);
        break;
    case QUIT_TIMEOUT:
    case QUIT_HOTBOOT:
    case QUIT_PURGE:
    case QUIT_AUTOSAVE:
    default:
        /* Imms stay where they are, but morts will reenter in the same place
         * they entered the game last time. */
        if (GET_LEVEL(ch) >= LVL_IMMORT)
            GET_LOADROOM(ch) = world[ch->in_room].vnum;
        ;
    }

    if (quit_mode == QUIT_CRYO) {
        SET_FLAG(PLR_FLAGS(ch), PLR_CRYO);
    }

    GET_QUIT_REASON(ch) = quit_mode;
    save_player_char(ch);
}
