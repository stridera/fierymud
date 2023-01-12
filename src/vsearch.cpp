/**************************************************************************
 *   File: vsearch.c                                      Part of FieryMUD *
 *  Usage: Function for searching world files                              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "vsearch.hpp"

#include "casting.hpp"
#include "charsize.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "damage.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "directions.hpp"
#include "exits.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "lifeforce.hpp"
#include "logging.hpp"
#include "messages.hpp"
#include "modify.hpp"
#include "olc.hpp"
#include "races.hpp"
#include "screen.hpp"
#include "shop.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"
#include "weather.hpp"

/* External function prototypes */
void list_shops(CharData *ch, int first, int last);

/* vsearch function prototypes */
ACMD(do_vsearch);
ACMD(do_msearch);
ACMD(do_osearch);
ACMD(do_vitem);
ACMD(do_vwear);
ACMD(do_rsearch);
ACMD(do_ssearch);
ACMD(do_tsearch);
ACMD(do_zsearch);
ACMD(do_csearch);
ACMD(do_esearch);
ACMD(do_ksearch);

/* Field types; used by parse_vsearch_args in vsearch_type arrays */
#define INTEGER 0
#define STRING 1
#define FLAGS 2
#define TYPE 3
#define APPLY 4
#define TRGTYPE 5
#define SPLNAME 6
#define BOOLEAN 7
#define CLASS 8
#define RACE 9
#define ZONHEMIS 10
#define ZONCLIME 11
#define SIZE 12
#define COMPOSITION 13
#define LIFEFORCE 14
#define OBJ_TYPE 15
#define SPHERE 16
#define DAMTYPE 17
#define LIQUID 18

/* Numeric comparison modes */
#define ANY 0 /* any match */
#define EQ 1  /* x == y */
#define GT 2  /* x > y  */
#define LT 3  /* x < y  */
#define GE 4  /* x >= y */
#define LE 5  /* x <= y */
#define NE 6  /* x != y */
#define BT 7  /* x <= y <= z */

/* Common macros */
#define UNRECOGNIZED_ARG(arg)                                                                                          \
    {                                                                                                                  \
        char_printf(ch, "Unrecognized vsearch mode: {}\n", (arg));                                                     \
        return;                                                                                                        \
    }
#define UNRECOGNIZED_RETURN(arg, ret)                                                                                  \
    {                                                                                                                  \
        char_printf(ch, "Unrecognized vsearch mode: {}\n", (arg));                                                     \
        return (ret);                                                                                                  \
    }

/*
 * vsearch buffer
 */
#define VBUF_LEN (100 * 1024) /* 100 kb ~= 1000 lines */
static char vbuf[VBUF_LEN];

static void page_char_to_char(CharData *mob, CharData *ch, int nfound) {
    int count;

    /* Extra chars to account for: */
    count = count_color_chars(RACE_ABBR(mob));

    paging_printf(ch, "{:4d}. [{:s}{:5d}{:s}] {:s} {:s} {:3d} {:s} {:s}{:s}&0 {:s}{:s}&0\n", nfound, grn,
                  GET_MOB_VNUM(mob), nrm, ellipsis(mob->player.short_descr, 39), CLASS_ABBR(mob), GET_LEVEL(mob),
                  RACE_ABBR(mob), LIFEFORCE_COLOR(mob), capitalize((LIFEFORCE_NAME(mob))), COMPOSITION_COLOR(mob),
                  capitalize((COMPOSITION_NAME(mob))));
}

/* vsearch_type struct used by each vsearch subcommand */
struct VSearchType {
    int type;
    char *name;
    char data;
    const char **lookup;
};

/*
 * numeric_compare
 *
 * Compares value with the match, depending on the numeric comparison
 * mode.  If the mode is BETWEEN, then the match argument is the lower
 * bound and the bound argument is the upper bound.
 */
bool numeric_compare(int value, int match, int bound, int mode) {
    switch (mode) {
    case ANY:
        return true;
    case EQ:
        return (value == match);
    case GT:
        return (value > match);
    case LT:
        return (value < match);
    case GE:
        return (value >= match);
    case LE:
        return (value <= match);
    case NE:
        return (value != match);
    case BT:
        return (value >= match && value <= bound);
    }
    return false;
}

/*
 * string_find
 *
 * Determines if query is contained in string.  If name is asserted,
 * only finds word-boundary-delimited matches.
 */
bool string_find(const char *query, const char *string, bool name) {
    if (!string)
        return false;
    else if (name)
        return isname(query, string);
    else
        return strcasestr(string, query) ? true : false;
}

/* string_start
 *
 * Determines if string starts with query.
 */
bool string_start(const char *query, const char *string) {
    if (!string)
        return false;
    return strcasestr(string, query) == string;
}

/*
 * Does a numeric comparison on all the trigger vnums in the list.
 * Returns true if a trigger matching the given comparison is found.
 */
bool check_trigger_vnums(TriggerPrototypeList *list, int trig_vnum, int bound, int mode) {
    while (list) {
        if (numeric_compare(list->vnum, trig_vnum, bound, mode))
            return true;
        list = list->next;
    }
    return false;
}

/*
 * Searches through the given extra description list for a description
 * with keywords matching the given string.  Returns true if any one
 * of the extra descriptions' keywords match.
 */
bool check_extra_descs(ExtraDescriptionData *ed, char *string) {
    if (!ed)
        return false;
    if (ed->keyword && isname(string, ed->keyword))
        return true;
    return check_extra_descs(ed->next, string);
}

/*
 * Copies all characters in the string to a static buffer, converting
 * any uppercase letters to lowercase.  Designed to be used to print
 * out lists of types, so it has a small buffer.
 */
static char *lowercase(const char *string) {
    static char lower[50];
    char *p = lower;
    /*
     * Gotta use tolower() because LOWER() would cause the ++ to go off
     * twice...resulting in bad things.  (memory overflow -> game lockup)
     */
    while ((*(p++) = tolower(*(string++))))
        ;
    return lower;
}

/*
 * parse_vlist_args
 *
 * Processes the command arguments for a command whose parameters are
 * as follows:
 *
 *    <starting vnum> <ending vnum>
 *  (or)
 *    <zone vnum>
 *  (or)
 *    "*"
 *  (or)
 *    (nothing, in which case it defaults to the current zone)
 *
 * The return value is true if valid argument(s) were received.  In
 * this case, the first and second integer values have been set.  If
 * both were specified, that's what they have been set to.  If only
 * a zone was specified (or if none was specified and the current
 * zone was used), then the first value is the beginning of the zone
 * and the second value is the "top of the zone".
 *
 * If a "*" was given, the values are set to 0 and MAX_VNUM, so all
 * requested items can be listed.
 *
 * The return value is false if invalid arguments were received.
 * In this case, a message has already been sent to the character.
 *
 * This function is meant to be useful for the vlist mode of the
 * vsearch commands.
 */

bool parse_vlist_args(CharData *ch, char *argument, int *first, int *last) {

    argument = any_one_arg(argument, arg);

    if (is_abbrev(arg, "from"))
        argument = any_one_arg(argument, arg);
    else if (!strcasecmp("*", arg)) {
        *first = 0;
        *last = MAX_VNUM;
        return true;
    }

    if (!*arg)
        *first = IN_ZONE_VNUM(ch);
    else
        *first = atoi(arg);

    if (*arg) {
        argument = any_one_arg(argument, arg);
        if (is_abbrev(arg, "to"))
            argument = any_one_arg(argument, arg);
    }

    if (*arg) {
        *last = atoi(arg);
        if (!*last && *arg != '0') {
            char_printf(ch, "That's not a valid vnum.\n");
            return false;
        }
    } else {
        *last = find_zone(*first);
        *first *= 100;
        if (*last == NOWHERE)
            *last = *first + 99;
        else
            *last = zone_table[*last].top;
    }

    if (*first < 0 || *first > MAX_VNUM || *last < 0 || *last > MAX_VNUM) {
        char_printf(ch, "Values must be between 0 and %d.\n", MAX_VNUM);
        return false;
    }

    /* If the order of the vnums are reversed, then swap em. */
    if (*first > *last) {
        int temp = *first;
        *first = *last;
        *last = temp;
    }

    return true;
}

/*
 * parse_vsearch_args
 *
 * Processes command arguments for the vsearch suite of commands whose
 * parameters typically follow this pattern:
 *
 *    <field> <query> <vnum bound>
 *
 * Valid fields, such as name or level, depend on the vsearch_type modes
 * array passed.  For example, do_msearch would pass a list of
 * mobile fields to search on, while do_rsearch would pass a list of
 * room fields.
 *
 * This function will determine which field the user wants, and then
 * parse any further arguments for that type of field, depending on
 * the information given in the vsearch_type list.  It will then parse
 * the next argument(s) (the query) on the argument list based on the
 * field type.
 *
 * For instance, if the field is "name", the field type will be string,
 * and the next argument will be returned in the **string argument.
 *
 * Query values are typically returned in value.
 *
 * After the query, a vnum bound may be specified, which will be parsed
 * by parse_vlist_args.
 */
bool parse_vsearch_args(CharData *ch, char *argument, int subcmd, int *mode, const VSearchType *modes, int *value,
                        int *bound, char **string, int *compare, flagvector *flags, int *first, int *last) {
    int type, temp;

    if (!mode || !modes || !value || !bound || !string || !compare || !flags || !first || !last) {
        if (ch)
            char_printf(ch, "Error in parsing arguments.\n");
        log("Bad internal argument passed to parse_vsearch_args.");
        return false;
    }

    /* Initialize values. */
    *mode = 0;
    *value = 0;
    *bound = 0;
    *string = nullptr;
    *compare = EQ;
    *flags = 0;
    *(flags + 1) = 0;
    *(flags + 2) = 0;
    *(flags + 3) = 0;
    *first = 0;
    *last = MAX_VNUM;

    any_one_arg(argument, arg);

    /*
     * If there's nothing else on the argument list, then:
     *   if this is a vsearch, we list the possible search fields;
     *   otherwise, we return true since we don't have to parse anything,
     *       but we still want the calling function to execute successfully.
     */
    if (!*arg) {
        if (subcmd != SCMD_VSEARCH)
            return true;
        char_printf(ch, "Allowed search fields:");
        for (temp = 0; modes[temp].type; ++temp)
            char_printf(ch, "{}{:<16s}", !(temp % 4) ? "\n" : "", modes[temp].name);
        char_printf(ch, "\n");
        return false;
    }
    /*
     * If the next argument is a number or "from", then assume that there
     * are no field/query arguments, and skip straight to the vnum bounds.
     */
    else if (isdigit(*arg) || is_abbrev(arg, "from"))
        return parse_vlist_args(ch, argument, first, last);

    /*
     * The vnum command automatically uses the "name" field to query,
     * which should always be the first item in the modes list.  If
     * it's not, your *num commands will give you odd results.
     */
    if (subcmd == SCMD_VNUM)
        type = 0;
    else {
        /*
         * Figure out which field to search on.
         */
        argument = any_one_arg(argument, arg);

        /* Figure out which field we're searching on. */
        for (type = 0; modes[type].type; ++type)
            if (is_abbrev(arg, modes[type].name))
                break;
        if (!modes[type].type)
            UNRECOGNIZED_RETURN(arg, false);
    }

    /* Tell the caller which field to search on. */
    *mode = modes[type].type;

    /*
     * Now figure out the search query based on the type of field.
     */
    switch (modes[type].data) {
        /*
         * Booleans are either true or false.  Value is returned in *value.
         */
    case BOOLEAN:
        argument = any_one_arg(argument, arg);
        if (!strcasecmp(arg, "1") || is_abbrev(arg, "true") || is_abbrev(arg, "yes") || is_abbrev(arg, "one") ||
            is_abbrev(arg, "on"))
            *value = 1;
        else if (is_abbrev(arg, "00000000") || is_abbrev(arg, "false") || is_abbrev(arg, "no") ||
                 is_abbrev(arg, "zero") || is_abbrev(arg, "off"))
            *value = 0;
        else {
            char_printf(ch, "Search value should be boolean ('yes' or 'no').\n");
            return false;
        }
        break;
        /*
         * Single-quote delimited string.  Spell number is returned in *value.
         */
    case SPLNAME:
        argument = delimited_arg(argument, arg, '\'');
        *value = find_spell_num(arg);
        if (*value < 0) {
            char_printf(ch, "Invalid spell name.\n");
            return false;
        }
        break;
        /*
         * A list of types must be refered to for the given search field in
         * the vsearch_types list.  Here we look through the list of types
         * for the one specified by the user in the query.  The type is
         * returned in *value.
         * APPLY is a special case of TYPE.  It follows the format
         *   <apply type> <apply amount>
         * and thus goes through both the TYPE and INTEGER blocks.  If
         * the data mode is APPLY, the type is returned in *flags instead
         * of *value.
         */
    case APPLY:
    case TYPE:
        argument = delimited_arg(argument, arg, '\'');
        if (!*arg) {
            sprintf(buf, "You can search for the following %s types:", modes[type].name);
            for (temp = 0; *modes[type].lookup[temp] != '\n'; ++temp) {
                if (!(temp % 4))
                    strcat(buf, "\n");
                sprintf(buf, "%s%-16s", buf, modes[type].lookup[temp]);
            }
            char_printf(ch, "{}\n", buf);
            return false;
        }
        *value = search_block(arg, modes[type].lookup, false);
        if (*value < 0) {
            char_printf(ch, "Invalid {} type: {}\n", modes[type].name, arg);
            return false;
        }
        if (modes[type].data == TYPE)
            break;
        else if (modes[type].data == APPLY) {
            *flags = *value;
            *value = 0;
            *compare = ANY;
            any_one_arg(argument, arg);
            if (!*arg || is_abbrev(arg, "from"))
                break;
            /* Fall through for APPLY mode when the numeric part follows. */
        }
        /*
         * Interpret the query as an integer.  A comparison mode is optional;
         * if none is given, then equals is assumed.  Returns the argument
         * query in *value, and the comparison mode in *compare.  If the
         * comparison mode is BETWEEN, then the lower bound is in *value and
         * the upper bound is in *bound.  The user may also specify "zone"
         * as a pseudo-comparison mode.  In this case, one numerical argument
         * is required from the user, the comparison mode returned is BETWEEN,
         * and the lower and upper bounds are the bottom and top vnums of the
         * zone given by the numerical argument.
         */
    case INTEGER:
        argument = any_one_arg(argument, arg);
        if (*arg == '*') {
            *value = 0;
            *bound = MAX_VNUM;
            *compare = BT;
            break;
        }
        *value = atoi(arg);
        if (*value || *arg == '0') {
            *compare = EQ;
            break;
        } else if (is_abbrev(arg, "equals") || is_abbrev(arg, "is") || is_abbrev(arg, "=="))
            *compare = EQ;
        else if (!strcasecmp(arg, "gt") || !strcasecmp(arg, ">"))
            *compare = GT;
        else if (!strcasecmp(arg, "lt") || !strcasecmp(arg, "<"))
            *compare = LT;
        else if (!strcasecmp(arg, "ge") || !strcasecmp(arg, ">="))
            *compare = GE;
        else if (!strcasecmp(arg, "le") || !strcasecmp(arg, "<="))
            *compare = LE;
        else if (!strcasecmp(arg, "ne") || is_abbrev(arg, "not") || !strcasecmp(arg, "!=") || !strcasecmp(arg, "<>"))
            *compare = NE;
        else if (!strcasecmp(arg, "bt") || is_abbrev(arg, "between")) {
            *compare = BT;
            argument = any_one_arg(argument, arg);
            if (!*arg || (!isdigit(*arg) && *arg != '-' && *arg != '+')) {
                char_printf(ch, "Expected numeric bound, but didn't get it.\n");
                return false;
            }
            *bound = atoi(arg);
            any_one_arg(argument, arg);
            if (is_abbrev(arg, "and"))
                argument = any_one_arg(argument, arg);
        } else if (is_abbrev(arg, "zone")) {
            *compare = BT;
            argument = any_one_arg(argument, arg);
            if (!*arg || !isdigit(*arg)) {
                char_printf(ch, "Expected zone number, but didn't get it.\n");
                return false;
            }
            *value = atoi(arg);
            *bound = find_zone(*value);
            *value *= 100;
            if (*bound == NOWHERE)
                *bound = *bound + 99;
            else
                *bound = zone_table[*bound].top;
            break;
        }
        argument = any_one_arg(argument, arg);
        if (!*arg || (!isdigit(*arg) && *arg != '-' && *arg != '+')) {
            char_printf(ch, "Expected numeric comparison, but didn't get it.\n");
            return false;
        }
        *value = atoi(arg);
        if (*compare == BT && *value > *bound) {
            temp = *value;
            *value = *bound;
            *bound = temp;
        }
        break;
    case CLASS:
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            strcpy(buf, "You can search for the following classes:");
            for (temp = 0; temp < NUM_CLASSES; ++temp) {
                if (!(temp % 4))
                    strcat(buf, "\n");
                sprintf(buf, "%s%-16s", buf, classes[temp].plainname);
            }
            char_printf(ch, "{}\n", buf);
            return false;
        }
        *value = parse_class(0, 0, arg);
        if (*value < 0) {
            char_printf(ch, buf);
            sprintf(buf, "Invalid class: %s\n", arg);
            return false;
        }
        break;
    case RACE:
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            strcpy(buf, "You can search for the following races:");
            for (temp = 0; temp < NUM_RACES; ++temp) {
                if (!(temp % 4))
                    strcat(buf, "\n");
                sprintf(buf, "%s%-16s", buf, races[temp].plainname);
            }
            char_printf(ch, "{}\n", buf);
            return false;
        }
        *value = parse_race(0, 0, arg);
        if (*value < 0) {
            char_printf(ch, "Invalid race: %s\n", arg);
            return false;
        }
        break;
    case SIZE:
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            strcpy(buf, "You can search for the following sizes:");
            for (temp = 0; temp < NUM_SIZES; ++temp) {
                if (!(temp % 4))
                    strcat(buf, "\n");
                sprintf(buf, "%s%-16s", buf, sizes[temp].name);
            }
            char_printf(ch, "{}\n", buf);
            return false;
        }
        *value = parse_size(0, arg);
        if (*value < 0) {
            char_printf(ch, "Invalid size: %s\n", arg);
            return false;
        }
        break;
        /*
         * Take the next word and return it in **string.  A nul bit is placed
         * after this word in char *argument and we continue to use it
         * normally.
         */
    case STRING:
        *string = argument + 1;
        *compare = (*(argument + 1) != '\''); /* single quote = exact matching */
        argument = delimited_arg(argument, arg, '\'');
        if (!*arg) {
            char_printf(ch, "What string do you want to search for?\n");
            return false;
        }
        if (*argument == ' ')
            *(argument++) = '\0';
        if (!strcasecmp(arg, "is") && **string != '\'') {
            argument = delimited_arg(argument, arg, '\'');
            if (*argument == ' ')
                *(argument++) = '\0';
        }
        /*
         * Now we have the search query in arg, copy it back to *string,
         * which points back to a position in argument that we know has
         * enough space (that's where we got the argument after all.
         * We can't leave it in arg, because we might re-use that buffer
         * again.
         */
        strcpy(*string, arg);
        break;
        /*
         * Interpret the next few arguments as flags.  A type array should
         * have been defined in the vsearch_types list, and this type array
         * is used to build a bitfield to search with.  This part of the
         * function ASSUMES that *flags actually refers to the first long
         * in a contiguous block of memory (i.e., an array of longs), and
         * places bits in the next two longs if more than 1 lookup type array
         * is defined.  This allows us to handle AFF, AFF2, and AFF3 flags.
         */
    case FLAGS:
        delimited_arg(argument, arg, '\'');
        if (!*arg) {
            sprintf(buf, "You can search for the following %s flags:", modes[type].name);
            for (temp = 0; *modes[type].lookup[temp] != '\n'; ++temp) {
                if (!(temp % 4))
                    strcat(buf, "\n");
                sprintf(buf, "%s%-16s", buf, modes[type].lookup[temp]);
            }
            char_printf(ch, "{}\n", buf);
            return false;
        }
        for (temp = 0; *arg; delimited_arg(argument, arg, '\'')) {
            if ((*value = search_block(arg, modes[type].lookup, false)) >= 0)
                SET_FLAG(flags, *value);
            else if (is_abbrev(arg, "from")) {
                if (!temp) {
                    char_printf(ch, "No flags provided before vnum bound.\n");
                    return false;
                }
                break;
            } else
                UNRECOGNIZED_RETURN(arg, false);
            argument = delimited_arg(argument, arg, '\'');
            ++temp;
        }
        break;
        /*
         * Stupid trigger type.  Lets you specify something like
         * "mobile greet", so there isn't any ambiguity between, say,
         * "object command" and "world command" types.  Returns the
         * trigger intention (i.e., mob, obj, or room) in *value and
         * the trigger type in *flags.
         */
    case TRGTYPE:
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            char_printf(ch, "What kind of trigger do you want to search for?\n");
            return false;
        }
        if (is_abbrev(arg, "mobile"))
            *value = MOB_TRIGGER;
        else if (is_abbrev(arg, "object"))
            *value = OBJ_TRIGGER;
        else if (is_abbrev(arg, "room"))
            *value = WLD_TRIGGER;
        else {
            char_printf(ch, "Unknown trigger attachment type: {}\n", arg);
            return false;
        }
        any_one_arg(argument, arg);
        if (!*arg) {
            char_printf(ch, "What kind of trigger do you want to search for?\n");
            return false;
        }
        for (temp = 0; *arg; any_one_arg(argument, arg)) {
            if (*value == MOB_TRIGGER && (*bound = search_block(arg, trig_types, false)) >= 0)
                *flags |= (1 << *bound);
            else if (*value == OBJ_TRIGGER && (*bound = search_block(arg, otrig_types, false)) >= 0)
                *flags |= (1 << *bound);
            else if (*value == WLD_TRIGGER && (*bound = search_block(arg, wtrig_types, false)) >= 0)
                *flags |= (1 << *bound);
            else if (is_abbrev(arg, "from")) {
                if (!temp) {
                    char_printf(ch, "No types provided before vnum bound.\n");
                    return false;
                }
                break;
            } else
                UNRECOGNIZED_RETURN(arg, false);
            argument = any_one_arg(argument, arg);
            ++temp;
        }
        break;
    case ZONHEMIS:
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            strcpy(buf, "You can search for the following hemispheres:");
            for (temp = 0; temp < NUM_HEMISPHERES; ++temp) {
                if (!(temp % 4))
                    strcat(buf, "\n");
                sprintf(buf, "%s%-16s", buf, hemispheres[temp].name);
            }
            char_printf(ch, "{}\n", buf);
            return false;
        }
        for (temp = 0; temp < NUM_HEMISPHERES; ++temp)
            if (is_abbrev(arg, hemispheres[temp].name))
                break;
        *value = temp;
        if (*value >= NUM_HEMISPHERES) {
            char_printf(ch, "Invalid hemisphere: {}\n", arg);
            return false;
        }
        break;
    case ZONCLIME:
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            strcpy(buf, "You can search for the following climates:");
            for (temp = 0; temp < NUM_CLIMATES; ++temp) {
                if (!(temp % 4))
                    strcat(buf, "\n");
                sprintf(buf, "%s%-16s", buf, climates[temp].name);
            }
            char_printf(ch, "{}\n", buf);
            return false;
        }
        for (temp = 0; temp < NUM_CLIMATES; ++temp)
            if (is_abbrev(arg, climates[temp].name))
                break;
        *value = temp;
        if (*value >= NUM_CLIMATES) {
            char_printf(ch, "Invalid climate: {}\n", arg);
            return false;
        }
        break;
    case LIFEFORCE:
        argument = any_one_arg(argument, arg);
        if ((*value = parse_lifeforce(ch, arg)) < 0)
            return false;
        break;
    case COMPOSITION:
        argument = any_one_arg(argument, arg);
        if ((*value = parse_composition(ch, arg)) < 0)
            return false;
        break;
    case OBJ_TYPE:
        argument = any_one_arg(argument, arg);
        if ((*value = parse_obj_type(ch, arg)) < 0)
            return false;
        break;
    case SPHERE:
        strcpy(arg, "sphere of ");
        argument = any_one_arg(argument, arg + 10);
        *value = find_talent_num(arg, 0);
        if (*value < 0) {
            char_printf(ch, "Invalid sphere name.\n");
            return false;
        }
        break;
    case DAMTYPE:
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            sprintf(buf, "You can search for the following damage types:");
            for (temp = 0; temp < NUM_DAMTYPES; ++temp) {
                if (!(temp % 4))
                    strcat(buf, "\n");
                sprintf(buf, "%-16s", damtypes[temp].name);
            }
            char_printf(ch, "{}\n", buf);
            return false;
        }
        if ((*value = parse_damtype(ch, arg)) < 0)
            return false;
        break;
    case LIQUID:
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            char_printf(ch, "You can search for the following liquid types:");
            for (temp = 0; temp < NUM_LIQ_TYPES; ++temp) {
                if (!(temp % 4))
                    char_printf(ch, "\n");
                char_printf(ch, "{:20}", LIQ_NAME(temp));
            }
            char_printf(ch, "\n");
            return false;
        }
        if ((*value = parse_liquid(ch, arg)) < 0)
            return false;
        break;
    }

    /* See if there are any vnum bounds now. */
    any_one_arg(argument, arg);
    if (isdigit(*arg) || is_abbrev(arg, "from"))
        return parse_vlist_args(ch, argument, first, last);

    return true;
}

ACMD(do_vsearch) {
    int mode;
    const struct {
        ACMD(*subcmd);
        char *name;
    } vsearch_modes[] = {
        {do_msearch, "mobiles"}, {do_osearch, "objects"}, {do_rsearch, "rooms"},    {do_tsearch, "triggers"},
        {do_ssearch, "shops"},   {do_zsearch, "zones"},   {do_csearch, "commands"}, {do_csearch, "resetcommands"},
        {do_esearch, "exits"},   {do_esearch, "doors"},   {do_ksearch, "skills"},   {do_ksearch, "spells"},
        {nullptr, nullptr},
    };

    argument = any_one_arg(argument, arg);
    if (!*arg) {
        if (subcmd == SCMD_VSEARCH)
            char_printf(ch, "Usage: vsearch <type> <field> <query> [[from] <start_vnum> [to] [<end_vnum>]]\n");
        else if (subcmd == SCMD_VLIST)
            char_printf(ch, "Usage: vlist <type> [[from] <start_vnum> [to] [<end_vnum>] | *]\n");
        else if (subcmd == SCMD_VNUM)
            char_printf(ch, "Usage: vnum <type> <name> [[from] <start_vnum> [to] <end_vnum>]\n");
        return;
    }

    for (mode = 0; vsearch_modes[mode].name; ++mode)
        if (is_abbrev(arg, vsearch_modes[mode].name))
            break;

    if (vsearch_modes[mode].name)
        (vsearch_modes[mode].subcmd)(ch, argument, cmd, subcmd);
    /*
     * If the search mode wasn't found, see if it's an item type or wear
     * position; if so, we can switch to vitem or vwear mode.
     */
    else {
        if ((cmd = parse_obj_type(nullptr, arg)) >= 0)
            subcmd = SCMD_VITEM;
        else if ((cmd = search_block(arg, wear_bits, false)) >= 0) {
            subcmd = SCMD_VWEAR;
            cmd = (1 << cmd);
        } else
            UNRECOGNIZED_ARG(arg);
        do_osearch(ch, argument, cmd, subcmd);
    }
}

const struct VSearchType vsearch_mobile_modes[] = {
    {1, "name", STRING}, /* must be first */
    {1, "alias", STRING},
    {2, "short", STRING},
    {3, "long", STRING},
    {4, "desc", STRING},
    {5, "race", RACE},
    {6, "size", SIZE},
    {7, "sex", TYPE, genders},
    {7, "gender", TYPE, genders},
    {8, "level", INTEGER},
    {9, "class", CLASS},
    {10, "alignment", INTEGER},
    {11, "hitpoints", INTEGER},
    {11, "hp", INTEGER},
    {12, "movement", INTEGER},
    {12, "mv", INTEGER},
    {13, "hitroll", INTEGER},
    {13, "hr", INTEGER},
    {14, "damroll", INTEGER},
    {14, "dr", INTEGER},
    {15, "damage", INTEGER},
    {15, "averagedamage", INTEGER},
    {16, "damdice", INTEGER},
    {16, "damnodice", INTEGER},
    {17, "sizedamdie", INTEGER},
    {17, "sizedamdice", INTEGER},
    {17, "damsizedice", INTEGER},
    {18, "attacktype", STRING}, /* weapon */
    {19, "armor", INTEGER},
    {19, "ac", INTEGER},
    {20, "platinum", INTEGER},
    {21, "gold", INTEGER},
    {22, "silver", INTEGER},
    {23, "copper", INTEGER},
    {24, "perception", INTEGER},
    {25, "hiddenness", INTEGER},
    {26, "position", TYPE, position_types},
    {27, "defaultposition", TYPE, position_types},
    {28, "triggervnum", INTEGER},
    {29, "actflags", FLAGS, action_bits},
    {30, "effflags", FLAGS, effect_flags},
    {31, "composition", COMPOSITION},
    {32, "lifeforce", LIFEFORCE},
    {0, nullptr, 0},
};

ACMD(do_msearch) {
    int mode, value, found = 0, compare, bound, nr, first, last, temp;
    char *string;
    flagvector flags[4];
    bool match;
    CharData *mob;

    if (subcmd == SCMD_VLIST) {
        if (!parse_vlist_args(ch, argument, &first, &last))
            return;
        mode = 0;
    } else {
        any_one_arg(argument, arg);
        if (subcmd == SCMD_VNUM && !*arg) {
            char_printf(ch, "Usage: mnum <name> [[from] <start_vnum> [to] <end_vnum>]\n");
            return;
        }
        if (!parse_vsearch_args(ch, argument, subcmd, &mode, vsearch_mobile_modes, &value, &bound, &string, &compare,
                                &flags[0], &first, &last))
            return;
        /* Special handling for 18: attack type */
        if (mode == 18) {
            for (found = temp = 0; temp <= TYPE_ALIGN - TYPE_HIT; ++temp)
                if (is_abbrev(string, attack_hit_text[temp].plural)) {
                    found = 1;
                    value = temp;
                }
            if (!found) {
                char_printf(ch, "That is not a valid attack type.\n");
                return;
            }
        }
    }

    /* See if the character is using color. */
    get_char_cols(ch);

    /* Loop through the mobile file. */
    for (nr = 0; nr <= top_of_mobt && (mob_index[nr].vnum <= last); ++nr) {
        if (mob_index[nr].vnum < first)
            continue;
        mob = &mob_proto[nr];
        match = false;
        /* Check to see if this mobile is a match based on the search mode. */
        switch (mode) {
        case 0:
            match = true;
            break;
        case 1:
            match = string_find(string, GET_NAMELIST(mob), compare);
            break;
        case 2:
            match = string_find(string, mob->player.short_descr, compare);
            break;
        case 3:
            match = string_find(string, mob->player.long_descr, compare);
            break;
        case 4:
            match = string_find(string, mob->player.description, compare);
            break;
        case 5:
            match = (GET_RACE(mob) == value);
            break;
        case 6:
            match = (GET_SIZE(mob) == value);
            break;
        case 7:
            match = (GET_SEX(mob) == value);
            break;
        case 8:
            match = numeric_compare(GET_LEVEL(mob), value, bound, compare);
            break;
        case 9:
            match = (GET_CLASS(mob) == value);
            break;
        case 10:
            match = numeric_compare(GET_ALIGNMENT(mob), value, bound, compare);
            break;
        case 11:
            /* Generate the AVERAGE max hitpoints. */
            if (!mob->points.max_hit)
                match = numeric_compare(mob->points.hit * (mob->points.mana + 1) / 2 + GET_EX_MAIN_HP(mob) +
                                            mob->points.move,
                                        value, bound, compare);
            else
                match = numeric_compare((mob->points.hit + mob->points.mana) / 2, value, bound, compare);
            break;
        case 12:
            match = numeric_compare(GET_MAX_MOVE(mob), value, bound, compare);
            break;
        case 13:
            match = numeric_compare(GET_HITROLL(mob), value, bound, compare);
            break;
        case 14:
            match = numeric_compare(GET_DAMROLL(mob), value, bound, compare);
            break;
        case 15:
            match = numeric_compare(mob->mob_specials.damnodice * (mob->mob_specials.damsizedice + 1) / 2, value, bound,
                                    compare);
            break;
        case 16:
            match = numeric_compare(mob->mob_specials.damnodice, value, bound, compare);
            break;
        case 17:
            match = numeric_compare(mob->mob_specials.damsizedice, value, bound, compare);
            break;
        case 18:
            match = (mob->mob_specials.attack_type == value);
            break;
        case 19:
            match = numeric_compare(GET_AC(mob), value, bound, compare);
            break;
        case 20:
            match = numeric_compare(GET_PLATINUM(mob), value, bound, compare);
            break;
        case 21:
            match = numeric_compare(GET_GOLD(mob), value, bound, compare);
            break;
        case 22:
            match = numeric_compare(GET_SILVER(mob), value, bound, compare);
            break;
        case 23:
            match = numeric_compare(GET_COPPER(mob), value, bound, compare);
            break;
        case 24:
            match = numeric_compare(GET_PERCEPTION(mob), value, bound, compare);
            break;
        case 25:
            match = numeric_compare(GET_HIDDENNESS(mob), value, bound, compare);
            break;
        case 26:
            match = (GET_POS(mob) == value);
            break;
        case 27:
            match = (mob->mob_specials.default_pos == value);
            break;
        case 28:
            match = check_trigger_vnums(mob->proto_script, value, bound, compare);
            break;
        case 29:
            match = ALL_FLAGGED(MOB_FLAGS(mob), flags, NUM_MOB_FLAGS);
            break;
        case 30:
            match = ALL_FLAGGED(EFF_FLAGS(mob), flags, NUM_EFF_FLAGS);
            break;
        case 31: /* COMPOSITION */
            match = GET_COMPOSITION(mob) == value;
            break;
        case 32: /* LIFE FORCE */
            match = GET_LIFEFORCE(mob) == value;
            break;
        }
        if (match) {
            if (!found) {
                paging_printf(ch,
                              "Index  VNum   Mobile Short-Desc                       "
                              "Class/Level/Race/Life Force/Composition\n");
                paging_printf(ch,
                              "----- ------- --------------------------------------- "
                              "---------------------------------------\n");
            }
            page_char_to_char(mob, ch, ++found);
        }
    }

    if (found) {
        start_paging(ch);
    } else
        char_printf(ch, "No matches found.\n");
}

const struct VSearchType vsearch_object_modes[] = {
    {1, "name", STRING}, /* must be first */
    {1, "alias", STRING},
    {2, "shortdesc", STRING},
    {3, "longdesc", STRING},
    {4, "actiondesc", STRING},
    {4, "adesc", STRING},
    {5, "type", OBJ_TYPE},
    {6, "extraflags", FLAGS, extra_bits},
    {6, "bits", FLAGS, extra_bits},
    {7, "wear", FLAGS, wear_bits},
    {7, "worn", FLAGS, wear_bits},
    {8, "weight", INTEGER},
    {9, "cost", INTEGER},
    /* 10 is unused */
    {11, "timer", INTEGER},
    {12, "level", INTEGER},
    {13, "hiddenness", INTEGER},
    {14, "apply", APPLY, apply_types},
    {15, "extradescs", STRING},
    /* 16 is available */
    {17, "spellflags", FLAGS, effect_flags},
    {17, "effflags", FLAGS, effect_flags},
    {18, "triggervnum", INTEGER},
    {19, "lasts", INTEGER},            /* light */
    {20, "casts", SPLNAME},            /* wand, staff, potion, scroll */
    {21, "chargesinitial", INTEGER},   /* staff, wand */
    {22, "chargesremaining", INTEGER}, /* staff, wand */
    {23, "damnodice", INTEGER},        /* weapon */
    {24, "damsizedice", INTEGER},      /* weapon */
    {25, "attacktype", STRING},        /* weapon */
    {26, "average", INTEGER},          /* weapon */
    {27, "armor", INTEGER},            /* armor */
    {28, "capacity", INTEGER},         /* container, drinkcon, fountain */
    {29, "contains", INTEGER},         /* drinkcon, fountain */
    {30, "liquid", LIQUID},            /* drinkcon, fountain */
    {31, "fillingness", INTEGER},      /* food */
    {32, "poisoned", BOOLEAN},         /* food, drinkcon, fountain */
    {33, "platinum", INTEGER},         /* money */
    {34, "gold", INTEGER},             /* money  */
    {35, "silver", INTEGER},           /* money  */
    {36, "copper", INTEGER},           /* money  */
    {37, "targetroom", INTEGER},       /* portal */
    {38, "entrymessage", INTEGER},     /* portal */
    {39, "charmessage", INTEGER},      /* portal */
    {40, "exitmessage", INTEGER},      /* portal */
    {41, "direction", TYPE, dirs},     /* wall */
    {42, "crumbles", BOOLEAN},         /* wall */
    {43, "hitpoints", INTEGER},        /* wall */
    {43, "hp", INTEGER},               /* wall */
    {44, "key", INTEGER},              /* container */
    {0, nullptr, 0},
};

#define MAX_SEARCH_ITEM_TYPES 4
const struct vsearch_object_value_type {
    int type;
    int item_types[MAX_SEARCH_ITEM_TYPES];
} value_types[] = {
    {19, {ITEM_LIGHT}},                                      /* lasts */
    {20, {ITEM_WAND, ITEM_STAFF, ITEM_POTION, ITEM_SCROLL}}, /* casts */
    {21, {ITEM_WAND, ITEM_STAFF}},                           /* chargesinitial */
    {22, {ITEM_WAND, ITEM_STAFF}},                           /* chargesremaining */
    {23, {ITEM_WEAPON}},                                     /* damnodice */
    {24, {ITEM_WEAPON}},                                     /* damsizedice */
    {25, {ITEM_WEAPON}},                                     /* attacktype */
    {26, {ITEM_WEAPON}},                                     /* average */
    {27, {ITEM_ARMOR, ITEM_TREASURE}},                       /* armor */
    {28, {ITEM_CONTAINER, ITEM_DRINKCON, ITEM_FOUNTAIN}},    /* capacity */
    {29, {ITEM_DRINKCON, ITEM_FOUNTAIN}},                    /* contains */
    {30, {ITEM_DRINKCON, ITEM_FOUNTAIN}},                    /* liquid */
    {31, {ITEM_FOOD}},                                       /* fillingness */
    {32, {ITEM_DRINKCON, ITEM_FOUNTAIN, ITEM_FOOD}},         /* poisoned */
    {33, {ITEM_MONEY}},                                      /* platinum */
    {34, {ITEM_MONEY}},                                      /* gold */
    {35, {ITEM_MONEY}},                                      /* silver */
    {36, {ITEM_MONEY}},                                      /* copper */
    {37, {ITEM_PORTAL}},                                     /* targetroom */
    {38, {ITEM_PORTAL}},                                     /* entrymessage */
    {39, {ITEM_PORTAL}},                                     /* charmessage */
    {40, {ITEM_PORTAL}},                                     /* exitmessage */
    {41, {ITEM_WALL}},                                       /* direction */
    {42, {ITEM_WALL}},                                       /* crumbles */
    {43, {ITEM_TRAP}},                                       /* hitpoints */
    {44, {ITEM_CONTAINER}},                                  /* key */
    {0, {0}},
};

ACMD(do_osearch) {
    int mode, value, found, compare, bound, nr, first, last, temp, temp_found, type = -1;
    char *string;
    const char *header_type;
    flagvector flags[4];
    bool match;
    ObjData *obj;
    char header1[255], header2[255];

    if (subcmd == SCMD_VLIST) {
        if (!parse_vlist_args(ch, argument, &first, &last))
            return;
        mode = 0;
    } else {
        any_one_arg(argument, arg);
        if (subcmd == SCMD_VNUM && !*arg) {
            char_printf(ch, "Usage: onum <name> [[from] <start_vnum> [to] <end_vnum>]\n");
            return;
        }
        if (!parse_vsearch_args(ch, argument, subcmd, &mode, vsearch_object_modes, &value, &bound, &string, &compare,
                                &flags[0], &first, &last))
            return;

        /* Special handling for 25: attack type */
        if (mode == 25) {
            for (found = temp = 0; temp <= TYPE_ALIGN - TYPE_HIT; ++temp)
                if (is_abbrev(string, attack_hit_text[temp].plural)) {
                    found = 1;
                    value = temp;
                }
            if (!found) {
                char_printf(ch, "That is not a valid attack type.\n");
                return;
            }
        }

        for (found = type = 0; value_types[type].type; ++type)
            if (value_types[type].type == mode) {
                if (subcmd == SCMD_VITEM) {
                    for (nr = 0; nr < MAX_SEARCH_ITEM_TYPES; ++nr)
                        if (value_types[type].item_types[nr] == cmd)
                            found = 1;
                    if (!found) {
                        char_printf(ch, "Invalid search mode for the given object type.\n");
                        return;
                    }
                }
                break;
            }
        if (!value_types[type].type)
            type = -1;
    }

    get_char_cols(ch);

    /* Initialize vbuf with headers */
    if (subcmd == SCMD_VITEM)
        switch (cmd) {
        case ITEM_LIGHT:
            header_type = "Light Duration";
            break;
        case ITEM_SCROLL:
        case ITEM_POTION:
            header_type = "Spell(s)";
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            header_type = "Charges Left/Total, Spell";
            break;
        case ITEM_WEAPON:
            header_type = "To-Dam, Average";
            break;
        case ITEM_ARMOR:
        case ITEM_TREASURE:
            header_type = "AC-Apply";
            break;
        case ITEM_TRAP:
            header_type = "Trap HP, Spell";
            break;
        case ITEM_CONTAINER:
            header_type = "Container Size";
            break;
        case ITEM_DRINKCON:
            header_type = "Contents/Capacity, Liquid";
            break;
        case ITEM_FOOD:
            header_type = "Hours Makes Full";
            break;
        case ITEM_MONEY:
            header_type = "Value";
            break;
        case ITEM_PORTAL:
            header_type = "Target Room";
            break;
        default:
            header_type = "";
            break;
        }
    else if (subcmd == SCMD_VWEAR)
        header_type = "Object Stats";
    else
        header_type = "Type   Object Values";
    sprintf(header1, "Index  VNum   Object Short-Desc              Lvl Wt    Value  %s\n", header_type);
    sprintf(header2, "----- ------- ------------------------------ --- ----- ------ %s\n",
            *header_type ? "-------------------------" : "");

    /* Loop through object prototypes. */
    for (found = nr = 0; nr <= top_of_objt && (obj_index[nr].vnum <= last); ++nr) {
        if (obj_index[nr].vnum < first)
            continue;
        obj = &obj_proto[nr];
        if (subcmd == SCMD_VITEM && GET_OBJ_TYPE(obj) != cmd)
            continue;
        if (subcmd == SCMD_VWEAR && !CAN_WEAR(obj, cmd))
            continue;
        if (type >= 0) {
            for (temp_found = temp = 0; temp < MAX_SEARCH_ITEM_TYPES && value_types[type].item_types[temp]; ++temp)
                if (value_types[type].item_types[temp] == GET_OBJ_TYPE(obj))
                    temp_found = 1;
            if (!temp_found)
                continue;
        }
        match = false;
        switch (mode) {
        case 0:
            match = true;
            break;
        case 1:
            match = string_find(string, obj->name, compare);
            break;
        case 2:
            match = string_find(string, obj->short_description, compare);
            break;
        case 3:
            match = string_find(string, obj->description, compare);
            break;
        case 4:
            match = string_find(string, obj->action_description, compare);
            break;
        case 5:
            match = (GET_OBJ_TYPE(obj) == value);
            break;
        case 6:
            match = ALL_FLAGGED(GET_OBJ_FLAGS(obj), flags, NUM_ITEM_FLAGS);
            break;
        case 7:
            match = (CAN_WEAR(obj, flags[0]) == flags[0]);
            break;
        case 8:
            match = numeric_compare(GET_OBJ_EFFECTIVE_WEIGHT(obj), value, bound, compare);
            break;
        case 9:
            match = numeric_compare(GET_OBJ_COST(obj), value, bound, compare);
            break;
        case 10:
            /* EMPTY can be reused */
            break;
        case 11:
            match = numeric_compare(GET_OBJ_TIMER(obj), value, bound, compare);
            break;
        case 12:
            match = numeric_compare(GET_OBJ_LEVEL(obj), value, bound, compare);
            break;
        case 13:
            match = numeric_compare(GET_OBJ_HIDDENNESS(obj), value, bound, compare);
            break;
        case 14:
            for (temp = 0; temp < MAX_OBJ_APPLIES; ++temp) {
                if (obj->applies[temp].location != flags[0])
                    continue;
                if (!numeric_compare(obj->applies[temp].modifier, value, bound, compare))
                    continue;
                match = true;
                break;
            }
            break;
        case 15:
            match = check_extra_descs(obj->ex_description, string);
            break;
            /* case 16 is available */
        case 17:
            match = ALL_FLAGGED(GET_OBJ_EFF_FLAGS(obj), flags, NUM_EFF_FLAGS);
            break;
        case 18:
            match = check_trigger_vnums(obj->proto_script, value, bound, compare);
            break;
        case 27:
        case 28:
        case 31:
        case 33:
        case 37:
            match = numeric_compare(GET_OBJ_VAL(obj, 0), value, bound, compare);
            break;
        case 21:
        case 23:
        case 29:
        case 34:
        case 38:
            match = numeric_compare(GET_OBJ_VAL(obj, 1), value, bound, compare);
            break;
        case 19:
        case 22:
        case 24:
        case 35:
        case 39:
        case 43:
        case 44:
            match = numeric_compare(GET_OBJ_VAL(obj, 2), value, bound, compare);
            break;
        case 36:
        case 40:
            match = numeric_compare(GET_OBJ_VAL(obj, 3), value, bound, compare);
            break;
        case 20:
            if (GET_OBJ_TYPE(obj) == ITEM_STAFF || GET_OBJ_TYPE(obj) == ITEM_WAND)
                match = (GET_OBJ_VAL(obj, VAL_STAFF_SPELL) == value);
            else if (GET_OBJ_TYPE(obj) == ITEM_SCROLL || GET_OBJ_TYPE(obj) == ITEM_POTION)
                match =
                    (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1) == value || GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_2) == value ||
                     GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_3) == value);
            break;
        case 25:
            match = (GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == value);
            break;
        case 26:
            match = numeric_compare(WEAPON_AVERAGE(obj), value, bound, compare);
            break;
        case 30:
            match = (GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID) == value);
            break;
        case 32:
            match = ((!!IS_POISONED(obj)) == value);
            break;
        case 41:
            match = (GET_OBJ_VAL(obj, VAL_WALL_DIRECTION) == value);
            break;
        case 42:
            match = ((!!GET_OBJ_VAL(obj, VAL_WALL_DISPELABLE)) == value);
            break;
        }

#define OBJ_TITLE_LENGTH 30

        if (match) {
            std::string vbuf;
            // How many extra spaces do we need to pad the title to OBJ_TITLE_LENGTH?
            std::string short_desc = ellipsis(obj->short_description, OBJ_TITLE_LENGTH);
            briefmoney(buf, 6, GET_OBJ_COST(obj));
            vbuf =
                fmt::format("{:4d}. [{}{:5d}{}] {:<{}} {}{:<3d}&0  {}{:>4}&0 {:<{}}&0 ", ++found, grn,
                            obj_index[nr].vnum, nrm, short_desc, OBJ_TITLE_LENGTH + count_color_chars(short_desc) * 2,
                            GET_OBJ_LEVEL(obj) > 104   ? "&5&b"
                            : GET_OBJ_LEVEL(obj) > 103 ? "&6&b"
                            : GET_OBJ_LEVEL(obj) > 102 ? "&2&b"
                            : GET_OBJ_LEVEL(obj) > 101 ? "&4&b"
                            : GET_OBJ_LEVEL(obj) > 100 ? "&1&b"
                            : GET_OBJ_LEVEL(obj) > 99  ? "&3&b"
                            : GET_OBJ_LEVEL(obj) > 89  ? "&5"
                            : GET_OBJ_LEVEL(obj) > 74  ? "&6"
                            : GET_OBJ_LEVEL(obj) > 49  ? "&2"
                                                       : "",
                            GET_OBJ_LEVEL(obj), GET_OBJ_EFFECTIVE_WEIGHT(obj) > 9999 ? "&5" : "",
                            GET_OBJ_EFFECTIVE_WEIGHT(obj) > 9999 ? 9999 : GET_OBJ_EFFECTIVE_WEIGHT(obj), buf,
                            5 + count_color_chars(buf));
            switch (subcmd) {
            case SCMD_VWEAR:
                if ((GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_TREASURE) &&
                    GET_OBJ_VAL(obj, VAL_ARMOR_AC))
                    vbuf += fmt::format(" {:d}ac", GET_OBJ_VAL(obj, VAL_ARMOR_AC));
                for (temp = 0; temp < MAX_OBJ_APPLIES; ++temp)
                    if (obj->applies[temp].modifier) {
                        sprinttype(obj->applies[temp].location, apply_abbrevs, buf);
                        vbuf += fmt::format("{}{}", obj->applies[temp].modifier, buf);
                    }
                break;
            case SCMD_VLIST:
            case SCMD_VSEARCH:
            case SCMD_VNUM:
                vbuf += fmt::format("{:>7} ", OBJ_TYPE_NAME(obj));
            /* fall through */
            case SCMD_VITEM:
                switch (GET_OBJ_TYPE(obj)) {
                case ITEM_LIGHT:
                    if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT)
                        vbuf += "<infinite>";
                    else
                        vbuf += fmt::format("{:4d} hours", GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING));
                    break;
                case ITEM_SCROLL:
                case ITEM_POTION:
                    if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1) == GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_2) &&
                        GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_2) == GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_3) &&
                        GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1) > 0) {
                        vbuf += fmt::format("{}{}{} x 3", cyn, skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1)), nrm);
                    } else {
                        for (temp = 1, temp_found = 0; temp <= 3; ++temp)
                            if (GET_OBJ_VAL(obj, temp) > 0)
                                vbuf += fmt::format("{}{}{}{}", temp_found++ ? ", " : "", cyn,
                                                    skill_name(GET_OBJ_VAL(obj, temp)), nrm);
                    }
                    break;
                case ITEM_WAND:
                case ITEM_STAFF:
                    vbuf += fmt::format("{:2d}/{:2d} {}{}{}", GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT),
                                        GET_OBJ_VAL(obj, VAL_WAND_MAX_CHARGES), cyn,
                                        skill_name(GET_OBJ_VAL(obj, VAL_WAND_SPELL)), nrm);
                    break;
                case ITEM_WEAPON:
                    vbuf += fmt::format("{:3d}d{:2d} avg {:.1f}", GET_OBJ_VAL(obj, VAL_WEAPON_DICE_NUM),
                                        GET_OBJ_VAL(obj, VAL_WEAPON_DICE_SIZE), WEAPON_AVERAGE(obj));
                    break;
                case ITEM_ARMOR:
                case ITEM_TREASURE:
                    vbuf += fmt::format("{:2d}ac", GET_OBJ_VAL(obj, VAL_ARMOR_AC));
                    break;
                case ITEM_TRAP:
                    vbuf += fmt::format("{:d}hp {}", GET_OBJ_VAL(obj, VAL_TRAP_HITPOINTS),
                                        skill_name(GET_OBJ_VAL(obj, VAL_TRAP_SPELL)));
                    break;
                case ITEM_CONTAINER:
                    vbuf += fmt::format("{}{:d}", subcmd == SCMD_VITEM ? "" : "size ",
                                        GET_OBJ_VAL(obj, VAL_CONTAINER_CAPACITY));
                    break;
                case ITEM_DRINKCON:
                case ITEM_FOUNTAIN:
                    vbuf += fmt::format("{:2d}/{:2d} {}{}", GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING),
                                        GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY), IS_POISONED(obj) ? "poison " : "",
                                        LIQ_NAME(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)));
                    break;
                case ITEM_FOOD:
                    vbuf += fmt::format("{:d} hrs{}", GET_OBJ_VAL(obj, VAL_FOOD_FILLINGNESS),
                                        IS_POISONED(obj) ? " poisoned" : "");
                    break;
                case ITEM_MONEY:
                    temp_found = false;
                    if (GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM)) {
                        vbuf += fmt::format("{:d}{}p{}", GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM), CLR(ch, HCYN), nrm);
                        temp_found = true;
                    }
                    if (GET_OBJ_VAL(obj, VAL_MONEY_GOLD))
                        vbuf += fmt::format("{}{:d}{}g{}", temp_found++ ? " " : "", GET_OBJ_VAL(obj, VAL_MONEY_GOLD),
                                            CLR(ch, HYEL), nrm);
                    if (GET_OBJ_VAL(obj, VAL_MONEY_SILVER))
                        vbuf += fmt::format("{}{:d}{}s{}", temp_found++ ? " " : "", GET_OBJ_VAL(obj, VAL_MONEY_SILVER),
                                            CLR(ch, HGRN), nrm);
                    if (GET_OBJ_VAL(obj, VAL_MONEY_COPPER))
                        vbuf += fmt::format("{}{:d}{}c{}", temp_found++ ? " " : "", GET_OBJ_VAL(obj, VAL_MONEY_COPPER),
                                            CLR(ch, HRED), nrm);
                    break;
                case ITEM_PORTAL:
                    temp = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DESTINATION));
                    vbuf += fmt::format("{}[{}{:5d}{}] {:.18s}", subcmd == SCMD_VITEM ? "" : "to room ", grn,
                                        GET_OBJ_VAL(obj, VAL_PORTAL_DESTINATION), nrm,
                                        temp == NOWHERE ? "NOWHERE" : world[temp].name);
                    break;
                }
            }
            if (found == 1) {
                paging_printf(ch, header1);
                paging_printf(ch, header2);
            }
            paging_printf(ch, "{}\n", vbuf);
        }
    }
    if (found)
        start_paging(ch);
    else
        char_printf(ch, "No matches found.\n");
}

ACMD(do_vitem) {
    int i;

    argument = any_one_arg(argument, arg);

    if ((cmd = parse_obj_type(nullptr, arg)) >= 0) {
        do_osearch(ch, argument, cmd, SCMD_VITEM);
        return;
    }

    strcpy(buf,
           "Usage: vitem <type> [<field> <query>] [[from] <start_vnum> [to] "
           "[<end_vnum>]]\n"
           "Possible types are:");
    for (i = 0; i < NUM_ITEM_TYPES; ++i)
        sprintf(buf, "%s%s%-15s", buf, !(i % 5) ? "\n" : "", lowercase(item_types[i].name));
    char_printf(ch, strcat(buf, "\n"));
}

ACMD(do_vwear) {
    int i;

    argument = any_one_arg(argument, arg);

    if ((cmd = search_block(arg, wear_bits, false)) >= 0) {
        do_osearch(ch, argument, (1 << cmd), SCMD_VWEAR);
        return;
    }

    strcpy(buf,
           "Usage: vwear <position> [<field> <query>] [[from] <start_vnum> "
           "[to] [<end_vnum>]]\n"
           "Possible positions are:");
    for (i = 0; *wear_bits[i] != '\n'; ++i)
        sprintf(buf, "%s%s%-15s", buf, !(i % 5) ? "\n" : "", lowercase(wear_bits[i]));
    char_printf(ch, strcat(buf, "\n"));
}

const struct VSearchType vsearch_room_modes[] = {
    {1, "name", STRING}, /* must be first */
    {1, "title", STRING}, {2, "sector", STRING},          {3, "description", STRING},
    {4, "extra", STRING}, {5, "flags", FLAGS, room_bits}, {6, "triggervnum", INTEGER},
    {0, nullptr, 0},
};

ACMD(do_rsearch) {
    int mode, value, found = 0, compare, bound, nr, first, last;
    char *string;
    flagvector flags[4];
    bool match;

    if (subcmd == SCMD_VLIST) {
        if (!parse_vlist_args(ch, argument, &first, &last))
            return;
        mode = 0;
    } else {
        any_one_arg(argument, arg);
        if (subcmd == SCMD_VNUM && !*arg) {
            char_printf(ch, "Usage: rnum <name> [[from] <start_vnum> [to] <end_vnum>]\n");
            return;
        }
        if (!parse_vsearch_args(ch, argument, subcmd, &mode, vsearch_room_modes, &value, &bound, &string, &compare,
                                &flags[0], &first, &last))
            return;
    }

    get_char_cols(ch);

    /* Loop through the world array. */
    for (nr = 0; nr <= top_of_world && (world[nr].vnum <= last); ++nr) {
        if (world[nr].vnum < first)
            continue;
        match = false;
        switch (mode) {
        case 0:
            match = true;
            break;
        case 1:
            match = string_find(string, world[nr].name, compare);
            break;
        case 2:
            /*match = string_find(string, sectors[SECT(nr)].name, *compare); */
            match = string_start(string, sectors[SECT(nr)].name);
            break;
        case 3:
            match = string_find(string, world[nr].description, compare);
            break;
        case 4:
            match = check_extra_descs(world[nr].ex_description, string);
            break;
        case 5:
            match = ALL_FLAGGED(ROOM_FLAGS(nr), flags, NUM_ROOM_FLAGS);
            break;
        case 6:
            match = check_trigger_vnums(world[nr].proto_script, value, bound, compare);
            break;
        }
        if (match) {
            if (!found) {
                paging_printf(ch,
                              "Index VNum    Title                              Sector   "
                              "    Indoors Lit Exits\n");
                paging_printf(ch,
                              "----- ------- ---------------------------------- "
                              "------------ ------- --- -------\n");
            }
#define MARK_EXIT(r, d)                                                                                                \
    (!(r).exits[d]                       ? "&0&9"                                                                      \
     : EXIT_IS_DESCRIPTION((r).exits[d]) ? "&0&6"                                                                      \
     : !EXIT_DEST((r).exits[d])          ? "&1&b"                                                                      \
     : EXIT_IS_DOOR((r).exits[d])        ? "&0&3"                                                                      \
                                         : "&0&2")
#define ROOM_TITLE_LENGTH 34

            paging_printf(
                ch, "{:4d} [{}{:5d}{}] {} {}{}{}&0 {} {}{} {}{}{}{}{}{}{}{}{}{}{}&0\n", ++found, grn, world[nr].vnum,
                nrm, ellipsis(world[nr].name, ROOM_TITLE_LENGTH), sectors[world[nr].sector_type].color,
                sectors[world[nr].sector_type].name, sectors[world[nr].sector_type].mv,
                ROOM_FLAGGED(nr, ROOM_INDOORS) ? "&3indoors&0 " : "        ",
                ROOM_FLAGGED(nr, ROOM_ALWAYSLIT) || world[nr].sector_type == SECT_CITY ? "&3&blit&0" : "&9&bno &0",
                MARK_EXIT(world[nr], NORTH), capdirs[NORTH], MARK_EXIT(world[nr], SOUTH), capdirs[SOUTH],
                MARK_EXIT(world[nr], EAST), capdirs[EAST], MARK_EXIT(world[nr], WEST), capdirs[WEST],
                MARK_EXIT(world[nr], UP), capdirs[UP], MARK_EXIT(world[nr], DOWN), capdirs[DOWN]);
        }
    }
    if (found)
        start_paging(ch);
    else
        char_printf(ch, "No matches found.\n");
}

const struct VSearchType vsearch_exit_modes[] = {
    {1, "name", STRING}, /* must be first */
    {1, "keyword", STRING},  {2, "description", STRING}, {3, "bits", FLAGS, exit_bits},
    {4, "keyvnum", INTEGER}, {5, "roomvnum", INTEGER},   {0, nullptr, 0},
};

ACMD(do_esearch) {
    int mode, value, found = 0, compare, bound, nr, first, last, dir;
    char *string;
    flagvector flags[4];
    bool match;
    Exit *exit;

    if (subcmd == SCMD_VLIST) {
        if (!parse_vlist_args(ch, argument, &first, &last))
            return;
        mode = 0;
    } else {
        any_one_arg(argument, arg);
        if (subcmd == SCMD_VNUM && !*arg) {
            char_printf(ch, "Usage: enum <name> [[from] <start_vnum> [to] <end_vnum>]\n");
            return;
        }
        if (!parse_vsearch_args(ch, argument, subcmd, &mode, vsearch_exit_modes, &value, &bound, &string, &compare,
                                &flags[0], &first, &last))
            return;
    }

    get_char_cols(ch);

    /* Loop through the world array. */
    for (nr = 0; nr <= top_of_world && (world[nr].vnum <= last); ++nr) {
        if (world[nr].vnum < first)
            continue;
        for (dir = 0; dir < NUM_OF_DIRS; ++dir) {
            if (!world[nr].exits[dir])
                continue;
            exit = world[nr].exits[dir];
            match = false;
            switch (mode) {
            case 0:
                match = true;
                break;
            case 1:
                match = (exit->keyword && string_find(string, exit->keyword, compare));
                break;
            case 2:
                match = (exit->general_description && string_find(string, exit->general_description, compare));
                break;
            case 3:
                match = (IS_SET(exit->exit_info, flags[0]) != 0);
                break;
            case 4:
                match = numeric_compare(exit->key, value, bound, compare);
                break;
            case 5:
                match = (exit->to_room != NOWHERE && numeric_compare(world[exit->to_room].vnum, value, bound, compare));
                break;
            }
            if (match) {
                if (!found) {
                    paging_printf(ch,
                                  "Index Dir      RoomNum Room Title           Exit "
                                  "Name/Key/Bits\n");
                    paging_printf(ch,
                                  "----- -----    ---------------------------- "
                                  "-----------------------\n");
                }
                if (!exit->keyword)
                    *buf = '\0';
                else if (exit->key != NOTHING)
                    sprintf(buf, "%s%s%s (key %d): ", grn, exit->keyword, nrm, exit->key);
                else
                    sprintf(buf, "%s%s%s: ", grn, exit->keyword, nrm);
                sprintbit(exit->exit_info, exit_bits, buf + strlen(buf));
                buf[strlen(buf) - 1] = '\0'; /* remove trailing space */
                paging_printf(ch, "{:4d}. {}{:<4s}{} at [{}{:5d}{}] {:<20s} [{}]\n", ++found, yel,
                              capitalize(dirs[dir]), nrm, grn, world[nr].vnum, nrm, world[nr].name, buf);
            }
        }
    }
    if (found)
        start_paging(ch);
    else
        char_printf(ch, "No matches found.\n");
}

const struct VSearchType vsearch_shop_modes[] = {
    {1, "name", STRING}, /* must be first */
    {2, "room", INTEGER},          {3, "keeper", INTEGER},
    {4, "bits", FLAGS, shop_bits}, {5, "tradeswith", FLAGS, trade_letters},
    {6, "open1", INTEGER},         {7, "open2", INTEGER},
    {8, "close1", INTEGER},        {9, "close2", INTEGER},
    {10, "sellvnum", INTEGER},     {11, "buytype", OBJ_TYPE},
    {12, "buyword", STRING},       {0, nullptr, 0},
};

ACMD(do_ssearch) {
    int mode, value, found = 0, compare, bound, nr, first, last, temp;
    char *string;
    flagvector flags[4];
    bool match;

    if (subcmd == SCMD_VLIST) {
        if (!parse_vlist_args(ch, argument, &first, &last))
            return;
        list_shops(ch, first, last);
        return;
    } else {
        any_one_arg(argument, arg);
        if (subcmd == SCMD_VNUM && !*arg) {
            char_printf(ch, "Usage: snum <name> [[from] <start_vnum> [to] <end_vnum>]\n");
            return;
        }
        if (!parse_vsearch_args(ch, argument, subcmd, &mode, vsearch_shop_modes, &value, &bound, &string, &compare,
                                &flags[0], &first, &last))
            return;
    }

    get_char_cols(ch);

    /* Loop through the shop list. */
    for (nr = 0; nr < top_shop && (SHOP_NUM(nr) <= last); ++nr) {
        if (SHOP_NUM(nr) < first)
            continue;
        match = false;
        switch (mode) {
        case 0:
            match = true;
            break;
        case 1:
            for (temp = 0; SHOP_ROOM(nr, temp) != NOWHERE; ++temp) {
                value = real_room(SHOP_ROOM(nr, temp));
                if (value != NOWHERE)
                    if (string_find(string, world[value].name, compare))
                        match = true;
            }
            break;
        case 2:
            for (temp = 0; SHOP_ROOM(nr, temp) != NOWHERE; ++temp)
                if (numeric_compare(SHOP_ROOM(nr, temp), value, bound, compare))
                    match = true;
            break;
        case 3:
            match = (SHOP_KEEPER(nr) >= 0 && numeric_compare(mob_index[SHOP_KEEPER(nr)].vnum, value, bound, compare));
            break;
        case 4:
            match = IS_SET(SHOP_BITVECTOR(nr), flags[0]);
            break;
        case 5:
            match = IS_SET(SHOP_TRADE_WITH(nr), flags[0]);
            break;
        case 6:
            match = numeric_compare(SHOP_OPEN1(nr), value, bound, compare);
            break;
        case 7:
            match = numeric_compare(SHOP_OPEN2(nr), value, bound, compare);
            break;
        case 8:
            match = numeric_compare(SHOP_CLOSE1(nr), value, bound, compare);
            break;
        case 9:
            match = numeric_compare(SHOP_CLOSE2(nr), value, bound, compare);
            break;
        case 10:
            for (temp = 0; SHOP_PRODUCT(nr, temp) != NOTHING; ++temp)
                if (numeric_compare(obj_index[SHOP_PRODUCT(nr, temp)].vnum, value, bound, compare))
                    match = true;
            break;
        case 11:
            for (temp = 0; SHOP_BUYTYPE(nr, temp) != NOTHING; ++temp)
                if (SHOP_BUYTYPE(nr, temp) == value)
                    match = true;
            break;
        case 12:
            for (temp = 0; SHOP_BUYTYPE(nr, temp) != NOTHING; ++temp)
                if (SHOP_BUYWORD(nr, temp) && string_find(string, SHOP_BUYWORD(nr, temp), compare))
                    match = true;
            break;
        }
        if (match) {
            if (!found) {
                paging_printf(ch, "Index  VNum   RoomNum Room Title (Shop Keeper)\n");
                paging_printf(ch,
                              "----- ------- ------- "
                              "---------------------------------------\n");
            }
            temp = real_room(SHOP_ROOM(nr, 0));
            paging_printf(ch, "{:3d}. [{:5d}] ({:5d}) {} ({})\n", ++found, SHOP_NUM(nr), SHOP_ROOM(nr, 0),
                          temp == NOWHERE ? "<NOWHERE>" : world[temp].name,
                          SHOP_KEEPER(nr) == NOBODY ? "no one" : mob_proto[SHOP_KEEPER(nr)].player.short_descr);
        }
    }

    if (found)
        start_paging(ch);
    else
        char_printf(ch, "No matches found.\n");
}

const struct VSearchType vsearch_trigger_modes[] = {
    {1, "name", STRING}, /* must be first */
    {2, "type", TRGTYPE},
    {3, "argument", STRING},
    {4, "numbericarg", INTEGER},
    {4, "numericarg", INTEGER},
    {5, "commands", STRING},
    {6, "intention", STRING},
    {0, nullptr, 0},
};

char *t_listdisplay(int nr, int index) {
    static char tbuf[MAX_INPUT_LENGTH];
    char tbuf2[MAX_INPUT_LENGTH];
    TrigData *trig;

    trig = trig_index[nr]->proto;
    switch (trig_index[nr]->proto->attach_type) {
    case OBJ_TRIGGER:
        strcpy(tbuf2, "OBJ ");
        sprintbit(GET_TRIG_TYPE(trig), otrig_types, tbuf2 + 4);
        break;
    case WLD_TRIGGER:
        strcpy(tbuf2, "WLD ");
        sprintbit(GET_TRIG_TYPE(trig), wtrig_types, tbuf2 + 4);
        break;
    case MOB_TRIGGER:
        strcpy(tbuf2, "MOB ");
        sprintbit(GET_TRIG_TYPE(trig), trig_types, tbuf2 + 4);
        break;
    default:
        sprintf(tbuf2, "%s???%s ", red, nrm);
    }
    snprintf(tbuf, MAX_INPUT_LENGTH, "%4d. [%s%5d%s] %-40.40s %s\n", index, grn, trig_index[nr]->vnum, nrm,
             trig_index[nr]->proto->name, tbuf2);

    return tbuf;
}

ACMD(do_tsearch) {
    int mode, value, found = 0, compare, bound, nr, first, last;
    char *string;
    flagvector flags[4];
    bool match;
    TrigData *trig;
    CmdlistElement *line;

    if (subcmd == SCMD_VLIST) {
        if (!parse_vlist_args(ch, argument, &first, &last))
            return;
        mode = 0;
    } else {
        any_one_arg(argument, arg);
        if (subcmd == SCMD_VNUM && !*arg) {
            char_printf(ch, "Usage: tnum <name> [[from] <start_vnum> [to] <end_vnum>]\n");
            return;
        }
        if (!parse_vsearch_args(ch, argument, subcmd, &mode, vsearch_trigger_modes, &value, &bound, &string, &compare,
                                &flags[0], &first, &last))
            return;

        /* Special handling for 6: intention */
        if (mode == 6) {
            if (is_abbrev(string, "mobile"))
                value = MOB_TRIGGER;
            else if (is_abbrev(string, "object"))
                value = OBJ_TRIGGER;
            else if (is_abbrev(string, "room"))
                value = WLD_TRIGGER;
            else {
                char_printf(ch, "Unknown trigger attachment type: {}\n", string);
                return;
            }
        }
    }

    get_char_cols(ch);

    /* Loop through the trigger list. */
    for (nr = 0; nr < top_of_trigt && trig_index[nr]->vnum <= last; ++nr) {
        if (trig_index[nr]->vnum < first)
            continue;
        trig = trig_index[nr]->proto;
        match = false;
        switch (mode) {
        case 0:
            match = true;
            break;
        case 1:
            match = string_find(string, GET_TRIG_NAME(trig), compare);
            break;
        case 2:
            match = (trig->attach_type == value && (IS_SET(GET_TRIG_TYPE(trig), flags[0]) == flags[0]));
            break;
        case 3:
            match = string_find(string, GET_TRIG_ARG(trig), compare);
            break;
        case 4:
            match = numeric_compare(GET_TRIG_NARG(trig), value, bound, compare);
            break;
        case 5:
            for (line = trig->cmdlist; line; line = line->next)
                if (line->cmd && string_find(string, line->cmd, compare)) {
                    match = true;
                    break;
                }
            break;
        case 6:
            match = (trig->attach_type == value);
            break;
        }
        if (match) {
            if (!found) {
                paging_printf(ch,
                              "Index  VNum   Trigger Name                             "
                              "Trigger Type \n");
                paging_printf(ch,
                              "----- ------- ---------------------------------------- "
                              "------------------\n");
            }
            paging_printf(ch, t_listdisplay(nr, ++found));
        }
    }

    if (found)
        start_paging(ch);
    else
        char_printf(ch, "No matches found.\n");
}

const char *zone_reset_modes[] = {"never", "empty", "normal", "\n"};

const struct VSearchType vsearch_zone_modes[] = {
    {1, "name", STRING}, /* must be first */
    {2, "lifespan", INTEGER},
    {3, "age", INTEGER},
    {4, "factor", INTEGER},
    {5, "resetmode", TYPE, zone_reset_modes},
    {6, "hemisphere", ZONHEMIS},
    {7, "temperature", INTEGER},
    {8, "precipitation", TYPE, precip},
    {9, "climate", ZONCLIME},
    {10, "windspeed", TYPE, wind_speeds},
    {11, "winddir", TYPE, dirs},
    {0, nullptr, 0},
};

ACMD(do_zsearch) {
    int mode, value, found = 0, compare, bound, nr, first, last;
    char *string;
    flagvector flags[4];
    bool match;
    ZoneData *zone;

    if (subcmd == SCMD_VLIST) {
        mode = 0;

        /*
         * Unfortunately, zlist works a little differently from other *list
         * commands, so we can't use parse_vlist_args.
         */
        argument = any_one_arg(argument, arg);
        if (is_abbrev(arg, "from"))
            argument = any_one_arg(argument, arg);
        if (!*arg) {
            first = 0;
            last = MAX_VNUM;
        } else {
            first = atoi(arg);
            argument = any_one_arg(argument, arg);
            if (is_abbrev(arg, "to"))
                argument = any_one_arg(argument, arg);
            if (!*arg)
                last = first;
            else
                last = atoi(arg);
        }
        if (first < 0 || first > MAX_VNUM || last < 0 || last > MAX_VNUM) {
            char_printf(ch, "Values must be between 0 and {}.\n", MAX_VNUM);
            return;
        }
        /* If the order of the vnums are reversed, then swap em. */
        if (first > last) {
            int temp = first;
            first = last;
            last = temp;
        }
    } else {
        any_one_arg(argument, arg);
        if (subcmd == SCMD_VNUM && !*arg) {
            char_printf(ch, "Usage: znum <name> [[from] <start_vnum> [to] <end_vnum>]\n");
            return;
        }
        if (!parse_vsearch_args(ch, argument, subcmd, &mode, vsearch_zone_modes, &value, &bound, &string, &compare,
                                &flags[0], &first, &last))
            return;
    }

    get_char_cols(ch);

    /* Loop through the zone list */
    for (nr = 0; nr <= top_of_zone_table && (zone_table[nr].number <= last); ++nr) {
        if (zone_table[nr].number < first)
            continue;
        zone = &zone_table[nr];
        match = false;
        /* Check to see if this zone is a match based on the search mode. */
        switch (mode) {
        case 0:
            match = true;
            break;
        case 1:
            match = string_find(string, zone->name, compare);
            break;
        case 2:
            match = numeric_compare(zone->lifespan, value, bound, compare);
            break;
        case 3:
            match = numeric_compare(zone->age, value, bound, compare);
            break;
        case 4:
            match = numeric_compare(zone->zone_factor, value, bound, compare);
            break;
        case 5:
            match = (zone->reset_mode == value);
            break;
        case 6:
            match = (zone->hemisphere == value);
            break;
        case 7:
            match = numeric_compare(zone->temperature, value, bound, compare);
            break;
        case 8:
            match = (zone->precipitation == value);
            break;
        case 9:
            match = (zone->climate == value);
            break;
        case 10:
            match = (zone->wind_speed == value);
            break;
        case 11:
            match = (zone->wind_dir == value);
            break;
        }
        if (match) {
            if (!found) {
                paging_printf(ch,
                              "Num Name                           Age Reset  Freq Factor "
                              "Top VNum\n");
                paging_printf(ch,
                              "--- ------------------------------ --- ----------- ------ "
                              "--------\n");
            }
            ++found;
            sprinttype(zone->reset_mode, zone_reset_modes, buf);
            paging_printf(ch, "{:3d} {:<30s} {:3d} {:<6s}  {:3d} {:6d}    {:5d}\n", zone->number, zone->name, zone->age,
                          buf, zone->lifespan, zone->zone_factor, zone->top);
        }
    }
    if (found)
        start_paging(ch);
    else
        char_printf(ch, "No matches found.\n");
}

#define DOOR_RESET_OPEN (1 << 0)
#define DOOR_RESET_CLOSED (1 << 1)
#define DOOR_RESET_LOCKED (1 << 2)
#define DOOR_RESET_HIDDEN (1 << 3)
const char *door_reset_modes[] = {"open", "closed", "locked", "hidden", "\n"};

const struct VSearchType vsearch_zone_command_modes[] = {{1, "mobile", INTEGER},
                                                         {2, "object", INTEGER},
                                                         {3, "roomobject", INTEGER},
                                                         {4, "putobj", INTEGER},
                                                         {5, "putinobj", INTEGER},
                                                         {6, "giveobj", INTEGER},
                                                         {7, "givemob", INTEGER},
                                                         {8, "equipobj", INTEGER},
                                                         {9, "equipmob", INTEGER},
                                                         {10, "removeobj", INTEGER},
                                                         {11, "door", FLAGS, door_reset_modes},
                                                         {12, "doorroom", INTEGER},
                                                         {13, "objloadamt", INTEGER},
                                                         {14, "mobloadamt", INTEGER},
                                                         {0, nullptr, 0}};

ACMD(do_csearch) {
    int mode, value, found = 0, compare, bound, nr, first, last, vbuflen, cnr, cmd_room = NOWHERE, cmd_mob = NOBODY;
    char *string;
    flagvector flags[4];
    bool match;
    ResetCommand *com;

    if (subcmd == SCMD_VLIST) {
        if (!parse_vlist_args(ch, argument, &first, &last))
            return;
        mode = 0;
    } else if (!parse_vsearch_args(ch, argument, subcmd, &mode, vsearch_zone_command_modes, &value, &bound, &string,
                                   &compare, &flags[0], &first, &last))
        return;

    get_char_cols(ch);

    /* Loop through the zone list */
    for (nr = 0; nr <= top_of_zone_table; ++nr) {
        if (zone_table[nr].top < first)
            continue;
        if (zone_table[nr].number * 100 > last)
            break;
        /* Loop through the command list for this zone */
        for (cnr = 0; zone_table[nr].cmd[cnr].command != 'S'; ++cnr) {
            com = &zone_table[nr].cmd[cnr];
            /* See if this command is in a matching room */
            switch (com->command) {
            case 'M':
                cmd_mob = com->arg1;
                /* fall through */
            case 'O':
            case 'V':
                cmd_room = com->arg3;
                break;
            case 'D':
            case 'R':
                cmd_room = com->arg1;
                break;
            default:
                /* Use the cmd_room from the last command */
                break;
            }
            if (cmd_room == NOWHERE || cmd_room > top_of_world || world[cmd_room].vnum < first ||
                world[cmd_room].vnum > last)
                continue;
            match = false;
            switch (mode) {
            case 0:
                match = true;
                break;
            case 1:
                match = (com->command == 'M' && numeric_compare(mob_index[com->arg1].vnum, value, bound, compare));
                break;
            case 2:
                switch (com->command) {
                case 'O':
                case 'P':
                case 'G':
                case 'E':
                    match = numeric_compare(obj_index[com->arg1].vnum, value, bound, compare);
                    break;
                }
                break;
            case 3:
                match = (com->command == 'O' && numeric_compare(obj_index[com->arg1].vnum, value, bound, compare));
                break;
            case 4:
                match = (com->command == 'P' && numeric_compare(obj_index[com->arg1].vnum, value, bound, compare));
                break;
            case 5:
                match = (com->command == 'P' && numeric_compare(obj_index[com->arg3].vnum, value, bound, compare));
                break;
            case 6:
                match = (com->command == 'G' && numeric_compare(obj_index[com->arg1].vnum, value, bound, compare));
                break;
            case 7:
                match = (com->command == 'G' && cmd_mob != NOWHERE && cmd_mob <= top_of_mobt &&
                         numeric_compare(mob_index[cmd_mob].vnum, value, bound, compare));
                break;
            case 8:
                match = (com->command == 'E' && numeric_compare(obj_index[com->arg1].vnum, value, bound, compare));
                break;
            case 9:
                match = (com->command == 'E' && cmd_mob != NOWHERE && cmd_mob <= top_of_mobt &&
                         numeric_compare(mob_index[cmd_mob].vnum, value, bound, compare));
                break;
            case 10:
                match = (com->command == 'R' && numeric_compare(obj_index[com->arg2].vnum, value, bound, compare));
                break;
            case 11:
                if (com->command != 'D')
                    break;
                match = true;
                for (value = 0; value < 4; ++value) {
                    if (!IS_SET(flags[0], (1 << value)))
                        continue;
                    switch ((1 << value)) {
                    case DOOR_RESET_OPEN:
                        if (com->arg3 != 0)
                            match = false;
                        break;
                    case DOOR_RESET_CLOSED:
                        if (com->arg3 != 1 && com->arg3 != 2 && com->arg3 != 4 && com->arg3 != 5)
                            match = false;
                        break;
                    case DOOR_RESET_LOCKED:
                        if (com->arg3 != 2 && com->arg3 != 4)
                            match = false;
                        break;
                    case DOOR_RESET_HIDDEN:
                        if (com->arg3 != 3 && com->arg3 != 4 && com->arg3 != 5)
                            match = false;
                        break;
                    }
                }
                break;
            case 12:
                match = (com->command == 'D' && com->arg2 >= 0 && com->arg2 < NUM_OF_DIRS &&
                         world[cmd_room].exits[com->arg2] && world[cmd_room].exits[com->arg2]->to_room != NOWHERE &&
                         numeric_compare(world[world[cmd_room].exits[com->arg2]->to_room].vnum, value, bound, compare));
                break;
            case 13:
                switch (com->command) {
                case 'O':
                case 'P':
                case 'G':
                case 'E':
                    match = numeric_compare(com->arg2, value, bound, compare);
                    break;
                }
                break;
            case 14:
                match = (com->command == 'M' && numeric_compare(com->arg2, value, bound, compare));
                break;
            }
            if (match) {
                if (!found) {
                    paging_printf(ch, "Index RoomNum Zone Command\n");
                    paging_printf(ch,
                                  "----- ------- "
                                  "----------------------------------------------------------\n");
                }

                vbuflen = sprintf(vbuf, "%4d. [%s%5d%s] ", ++found, grn, world[cmd_room].vnum, nrm);
                switch (com->command) {
                case 'M':
                    sprintf(vbuf + vbuflen, "Load mob %s (%s%d%s), Max: %d\n", mob_proto[com->arg1].player.short_descr,
                            grn, mob_index[com->arg1].vnum, nrm, com->arg2);
                    break;
                case 'O':
                    sprintf(vbuf + vbuflen, "Load obj %s (%s%d%s), Max: %d\n", obj_proto[com->arg1].short_description,
                            grn, obj_index[com->arg1].vnum, nrm, com->arg2);
                    break;
                case 'G':
                    sprintf(vbuf + vbuflen, "Give %s (%s%d%s) to %s (%s%d%s), Max: %d\n",
                            obj_proto[com->arg1].short_description, grn, obj_index[com->arg1].vnum, nrm,
                            cmd_mob != NOWHERE && cmd_mob < top_of_mobt ? mob_proto[cmd_mob].player.short_descr
                                                                        : "a mob",
                            grn, cmd_mob != NOWHERE && cmd_mob < top_of_mobt ? mob_index[cmd_mob].vnum : -1, nrm,
                            com->arg2);
                    break;
                case 'E':
                    sprintf(vbuf + vbuflen, "Equip %s (%s%d%s) to %s (%s%d%s) %s, Max: %d\n",
                            obj_proto[com->arg1].short_description, grn, obj_index[com->arg1].vnum, nrm,
                            cmd_mob != NOWHERE && cmd_mob < top_of_mobt ? mob_proto[cmd_mob].player.short_descr
                                                                        : "a mob",
                            grn, cmd_mob != NOWHERE && cmd_mob < top_of_mobt ? mob_index[cmd_mob].vnum : -1, nrm,
                            equipment_types[com->arg3], com->arg2);
                    break;
                case 'P':
                    sprintf(vbuf + vbuflen, "Put %s (%s%d%s) in %s (%s%d%s), Max: %d\n",
                            obj_proto[com->arg1].short_description, grn, obj_index[com->arg1].vnum, nrm,
                            obj_proto[com->arg3].short_description, grn, obj_index[com->arg3].vnum, nrm, com->arg2);
                    break;
                case 'R':
                    sprintf(vbuf + vbuflen, "Remove %s (%s%d%s) from room\n", obj_proto[com->arg2].short_description,
                            grn, obj_index[com->arg2].vnum, nrm);
                    break;
                case 'D':
                    sprintf(vbuf + vbuflen, "Set door %s as %s\n", dirs[com->arg2],
                            com->arg3
                                ? (com->arg3 == 1
                                       ? "closed"
                                       : (com->arg3 == 2 ? "locked"
                                                         : (com->arg3 == 3 ? "hidden"
                                                                           : (com->arg3 == 4 ? "hidden/closed/locked"
                                                                                             : "hidden/closed"))))
                                : "open");
                    break;
                }
                paging_printf(ch, vbuf);
            }
        }
    }
    if (found)
        start_paging(ch);
    else
        char_printf(ch, "No matches found.\n");
}

const struct VSearchType vsearch_skill_modes[] = {
    {1, "name", STRING}, /* must be first */
    {2, "type", TYPE, talent_types},
    {3, "minpos", TYPE, position_types},
    {4, "minmana", INTEGER},
    {5, "maxmana", INTEGER},
    {6, "manachange", INTEGER},
    {7, "level", INTEGER},
    {8, "circle", INTEGER},
    {9, "lowestlevel", INTEGER},
    {10, "class", CLASS},
    {11, "humanoid", BOOLEAN},
    {12, "routines", FLAGS, routines},
    {13, "violent", BOOLEAN},
    {14, "targets", FLAGS, targets},
    {15, "memtime", INTEGER},
    {16, "casttime", INTEGER},
    {17, "damagetype", DAMTYPE},
    {18, "sphere", SPHERE},
    {19, "pages", INTEGER},
    {20, "quest", BOOLEAN},
    {21, "wearoffmsg", STRING},
    {22, "fightingok", BOOLEAN},
    {0, nullptr, 0},
};

ACMD(do_ksearch) {
    int mode, value, found = 0, compare, bound, nr, first, last, temp;
    char *string;
    const char *color;
    flagvector flags[4];
    bool match;
    SkillDef *skill;

    if (subcmd != SCMD_VSEARCH) {
        char_printf(ch, HUH);
        return;
    } else if (!parse_vsearch_args(ch, argument, subcmd, &mode, vsearch_skill_modes, &value, &bound, &string, &compare,
                                   flags, &first, &last))
        return;

    /* See if the character is using color. */
    get_char_cols(ch);

    /* Loop through the skills table */
    for (nr = 0; nr <= TOP_SKILL_DEFINE; ++nr) {
        skill = &skills[nr];
        if (!skill->name || *skill->name == '!')
            continue;
        match = false;
        switch (mode) {
        case 0:
            match = true;
            break;
        case 1:
            match = string_find(string, skill->name, compare);
            break;
        case 2:
            match = (talent_type(nr) == value);
            break;
        case 3:
            match = (skill->minpos == value);
            break;
        case 4:
            match = numeric_compare(skill->mana_min, value, bound, compare);
            break;
        case 5:
            match = numeric_compare(skill->mana_max, value, bound, compare);
            break;
        case 6:
            match = numeric_compare(skill->mana_change, value, bound, compare);
            break;
        case 7:
            match = false;
            for (temp = 0; temp < NUM_CLASSES; ++temp)
                if (numeric_compare(skill->min_level[temp], value, bound, compare)) {
                    match = true;
                    break;
                }
            break;
        case 8:
            match = false;
            for (temp = 0; temp < NUM_CLASSES; ++temp)
                if (numeric_compare(level_to_circle(skill->min_level[temp]), value, bound, compare)) {
                    match = true;
                    break;
                }
            break;
        case 9:
            match = numeric_compare(skill->lowest_level, value, bound, compare);
            break;
        case 10:
            match = (skill->min_level[value] < LVL_IMMORT);
            break;
        case 11:
            match = (skill->humanoid == value);
            break;
        case 12:
            match = IS_SET(skill->routines, flags[0]);
            break;
        case 13:
            match = (skill->violent == value);
            break;
        case 14:
            match = IS_SET(skill->targets, flags[0]);
            break;
        case 15:
            match = numeric_compare(skill->mem_time, value, bound, compare);
            break;
        case 16:
            match = numeric_compare(skill->cast_time, value, bound, compare);
            break;
        case 17:
            match = (skill->damage_type == value);
            break;
        case 18:
            match = (skill->sphere == value);
            break;
        case 19:
            match = numeric_compare(skill->pages, value, bound, compare);
            break;
        case 20:
            match = (skill->quest == value);
            break;
        case 21:
            match = string_find(string, skill->wearoff, compare);
            break;
        case 22:
            match = skill->fighting_ok == value;
            break;
        }
        if (match) {
            if (!found) {
                paging_printf(ch,
                              "Skill/Spell/Chant          Vio  Qst  H/O  Routines     "
                              "Targets      Damage\n"
                              "-------------------------  ---  ---  ---  -----------  "
                              "-----------  --------\n");
            }
            ++found;
            temp = talent_type(nr);
            switch (temp) {
            case SKILL:
                color = AFRED;
                break;
            case SPELL:
                color = AFCYN;
                break;
            case CHANT:
                color = AFYEL;
                break;
            default:
                color = AFWHT;
                break;
            }
            if (skill->routines) {
                sprintbit(skill->routines, routines, buf1);
                if (strlen(buf1) > 12)
                    strcpy(buf1 + 9, "...");
            } else
                strcpy(buf1, "NONE");
            if (skill->targets) {
                sprintbit(skill->targets, targets, buf2);
                if (strlen(buf2) > 12)
                    strcpy(buf2 + 9, "...");
            } else
                strcpy(buf2, "NONE");
            paging_printf(ch, "{}{:<26s}@0 {:<3s}  {:<3s}  {:<3s}  {:<12s} {:<12s} {}{}@0\n", color, skill->name,
                          temp != SKILL ? YESNO(skill->violent) : "n/a", temp != SKILL ? YESNO(skill->quest) : "n/a",
                          temp == SKILL ? YESNO(skill->humanoid) : "n/a", temp != SKILL ? buf1 : "n/a", buf2,
                          VALID_DAMTYPE(skill->damage_type) ? damtypes[skill->damage_type].color : "",
                          VALID_DAMTYPE(skill->damage_type) ? damtypes[skill->damage_type].name : "n/a");
        }
    }

    if (found)
        start_paging(ch);
    else
        char_printf(ch, "No matches found.\n");
}
