/***************************************************************************
 *   File: rules.c                                        Part of FieryMUD *
 *  Usage: character rules                                                 *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "rules.h"

#include "clan.h"
#include "conf.h"
#include "handler.h"
#include "interpreter.h"
#include "screen.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* our own little private buffer, similar to what is declared in db.c */
static char arg[MAX_STRING_LENGTH];

DECLARE_RULE_VTABLE(clan);
/* DECLARE_RULE_VTABLE(command); */
DECLARE_RULE_VTABLE(level);
DECLARE_RULE_VTABLE(namelist);

DECLARE_RULE_VTABLE(and);
DECLARE_RULE_VTABLE(not );
DECLARE_RULE_VTABLE(or);

void init_rules() {
    struct rule_vtable *vtable_list[] = {&RULE_VTABLE(and), &RULE_VTABLE(clan),
                                         /* &RULE_VTABLE(command), */
                                         &RULE_VTABLE(level), &RULE_VTABLE(namelist), &RULE_VTABLE(not ),
                                         &RULE_VTABLE(or), NULL};
    size_t pos;

    for (pos = 0; vtable_list[pos]; ++pos)
        REGISTER_RULE_VTABLE(vtable_list[pos]);
}

/*
 * CLAN RULE
 */

/* rule data type */
BEGIN_RULE_DATATYPE;
unsigned int number;
unsigned int rank;
PUBLISH_RULE_DATATYPE(clan_rule);

RULE_CONSTRUCTOR(make_clan_rule)(unsigned int clan, unsigned int rank) {
    clan_rule *cr = MAKE_RULE(clan, clan_rule);
    cr->number = clan;
    cr->rank = rank;
    return (rule_t *)cr;
}

BEGIN_RULE_FUNC_MATCHER(clan, clan_rule) {
    return GET_CLAN(ch) && GET_CLAN(ch)->number == rule->number && !OUTRANKS(rule->rank, GET_CLAN_RANK(ch));
}

END_RULE_FUNC BEGIN_RULE_FUNC_DESTRUCTOR(clan, clan_rule) { free(rule); }

END_RULE_FUNC BEGIN_RULE_FUNC_PRINT(clan, clan_rule) { snprintf(buf, size, "%u %u", rule->number, rule->rank); }

END_RULE_FUNC BEGIN_RULE_FUNC_ABBR(clan, clan_rule) {
    snprintf(buf, size, "%u" FCYN "%u" ANRM, rule->number, rule->rank);
}

END_RULE_FUNC BEGIN_RULE_FUNC_VERBOSE(clan, clan_rule) {
    struct clan *clan = find_clan_by_number(rule->number);
    if (clan)
        snprintf(buf, size, "clan %s, rank %u", clan->abbreviation, rule->rank);
    else
        snprintf(buf, size, "unknown clan %u, rank %u", rule->number, rule->rank);
}

END_RULE_FUNC BEGIN_RULE_FUNC_PARSE(clan, clan_rule) {
    struct clan *clan;
    unsigned int number, rank;
    if (sscanf(buf, "%u %u", &number, &rank) == 2)
        return make_clan_rule(number, rank);
    else {
        buf = fetch_word(buf, arg, sizeof(arg));
        skip_over(buf, S_WHITESPACE);
        if (!(clan = find_clan(arg)))
            return NULL; /* invalid clan */
        else if (!is_number(buf))
            return NULL; /* invalid rank */
        else
            return make_clan_rule(clan->number, atoi(buf));
    }
}

END_RULE_FUNC PUBLISH_RULE_VTABLE(clan);

/*
 * LEVEL RANGE RULE
 */

BEGIN_RULE_DATATYPE;
int min;
int max;
PUBLISH_RULE_DATATYPE(range_rule);

RULE_CONSTRUCTOR(make_level_rule)(int min_level, int max_level) {
    range_rule *rr = MAKE_RULE(level, range_rule);
    rr->min = min_level;
    rr->max = max_level;
    return (rule_t *)rr;
}

BEGIN_RULE_FUNC_MATCHER(level, range_rule) { return GET_LEVEL(ch) >= rule->min && GET_LEVEL(ch) <= rule->max; }

END_RULE_FUNC BEGIN_RULE_FUNC_DESTRUCTOR(level, range_rule) { free(rule); }

END_RULE_FUNC BEGIN_RULE_FUNC_PRINT(level, range_rule) { snprintf(buf, size, "%d %d", rule->min, rule->max); }

END_RULE_FUNC BEGIN_RULE_FUNC_ABBR(level, range_rule) { snprintf(buf, size, "%d", rule->min); }

END_RULE_FUNC BEGIN_RULE_FUNC_VERBOSE(level, range_rule) {
    snprintf(buf, size, "level%s %d - %d", rule->min == rule->max ? "" : "s", rule->min, rule->max);
}

END_RULE_FUNC BEGIN_RULE_FUNC_PARSE(level, range_rule) {
    int min, max;
    if (sscanf(buf, "%d %d", &min, &max) == 2)
        return make_level_rule(min, max);
    else
        return NULL;
}

END_RULE_FUNC PUBLISH_RULE_VTABLE(level);

/*
 * NAMELIST RULE
 */

BEGIN_RULE_DATATYPE;
char *namelist;
PUBLISH_RULE_DATATYPE(namelist_rule);

RULE_CONSTRUCTOR(make_namelist_rule)(const char *namelist) {
    namelist_rule *nr = MAKE_RULE(namelist, namelist_rule);
    nr->namelist = strdup(namelist);
    return (rule_t *)nr;
}

BEGIN_RULE_FUNC_MATCHER(namelist, namelist_rule) { return isname(GET_NAME(ch), rule->namelist); }

END_RULE_FUNC BEGIN_RULE_FUNC_DESTRUCTOR(namelist, namelist_rule) {
    free(rule->namelist);
    free(rule);
}

END_RULE_FUNC BEGIN_RULE_FUNC_PRINT(namelist, namelist_rule) { snprintf(buf, size, "%s", rule->namelist); }

END_RULE_FUNC BEGIN_RULE_FUNC_ABBR(namelist, namelist_rule) { snprintf(buf, size, "%s", rule->namelist); }

END_RULE_FUNC BEGIN_RULE_FUNC_VERBOSE(namelist, namelist_rule) { snprintf(buf, size, "names: %s", rule->namelist); }

END_RULE_FUNC BEGIN_RULE_FUNC_PARSE(namelist, namelist_rule) { return make_namelist_rule(buf); }

END_RULE_FUNC PUBLISH_RULE_VTABLE(namelist);

/*
 * AND/OR RULES
 */

BEGIN_RULE_DATATYPE;
rule_t *left;
rule_t *right;
PUBLISH_RULE_DATATYPE(binary_rule);

#define BINARY_AND 0
#define BINARY_OR 1
#define BINARY_NAND 2 /* not implemented */
#define BINARY_NOR 3  /* not implemented */

static RULE_CONSTRUCTOR(make_binary_rule)(rule_t *left, rule_t *right, int binary_type) {
    binary_rule *br = NULL;
    switch (binary_type) {
    case BINARY_AND:
        br = MAKE_RULE(and, binary_rule);
        break;
    case BINARY_OR:
        br = MAKE_RULE(or, binary_rule);
        break;
    default:
        return NULL;
    }
    br->left = left;
    br->right = right;
    return (rule_t *)br;
}

RULE_CONSTRUCTOR(make_and_rule)(rule_t *left, rule_t *right) { return make_binary_rule(left, right, 0); }

RULE_CONSTRUCTOR(make_or_rule)(rule_t *left, rule_t *right) { return make_binary_rule(left, right, 1); }

BEGIN_RULE_FUNC_MATCHER(and, binary_rule) { return rule_matches(rule->left, ch) && rule_matches(rule->right, ch); }

END_RULE_FUNC BEGIN_RULE_FUNC_MATCHER(or, binary_rule) {
    return rule_matches(rule->left, ch) || rule_matches(rule->right, ch);
}

END_RULE_FUNC BEGIN_RULE_FUNC_DESTRUCTOR(binary, binary_rule) {
    if (rule->left != rule->right)
        free_rule(rule->left);
    free_rule(rule->right);
    free(rule);
}

END_RULE_FUNC
/* Safe Unsigned Decrement */
#define SUD(var, amt) ((var) -= (var) < (amt) ? (var) : (amt))
void print_binary_rule(char *buf, size_t size, const char *insert, const binary_rule *rule, bool verbose) {
    char *p = buf;
    size_t diff;

    snprintf(p, size, "(");

    SUD(size, 1);
    ++p;
    if (verbose)
        rule_verbose(p, size, rule->left);
    else
        sprint_rule(p, size, rule->left);

    diff = strlen(p);
    SUD(size, diff);
    p += diff;
    snprintf(buf, size, ")%s(", insert);

    SUD(size, 3);
    p += 3;
    if (verbose)
        rule_verbose(p, size, rule->right);
    else
        sprint_rule(p, size, rule->right);

    diff = strlen(p);
    SUD(size, diff);
    p += diff;
    snprintf(buf, size, ")");
}

BEGIN_RULE_FUNC_PRINT(binary, binary_rule) { print_binary_rule(buf, size, " ", rule, false); }

END_RULE_FUNC BEGIN_RULE_FUNC_ABBR(and, binary_rule) {
    rule = rule; /* to avoid compiler warning */
    snprintf(buf, size, "AND");
}

END_RULE_FUNC BEGIN_RULE_FUNC_ABBR(or, binary_rule) {
    rule = rule; /* to avoid compiler warning */
    snprintf(buf, size, "OR");
}

END_RULE_FUNC BEGIN_RULE_FUNC_VERBOSE(and, binary_rule) { print_binary_rule(buf, size, " and ", rule, true); }

END_RULE_FUNC BEGIN_RULE_FUNC_VERBOSE(or, binary_rule) { print_binary_rule(buf, size, " or ", rule, true); }

END_RULE_FUNC bool process_binary_rule(const char *buf, rule_t **left, rule_t **right) {
    const char *start = buf, *end;
    char *temp;
    *left = NULL;
    *right = NULL;

    if (*start != '(')
        return FALSE;
    for (end = ++start; *end && *end != ')'; ++end)
        ;
    if (start == end || *end != ')')
        return FALSE;
    CREATE(temp, char, end - start + 1);
    strncpy(temp, start, end - start);
    *left = parse_rule(temp);
    free(temp);
    if (!*left)
        return FALSE;
    start = ++end;
    if (*start != ' ' || *(start + 1) != '(' || *(start + 2) == '\0') {
        free_rule(*left);
        return FALSE;
    }
    start += 2;
    for (end = (start += 2); *end && *end != ')'; ++end)
        ;
    if (start == end || *end != ')') {
        free_rule(*left);
        return FALSE;
    }
    CREATE(temp, char, end - start + 1);
    strncpy(temp, start, end - start);
    *right = parse_rule(temp);
    free(temp);
    if (!*right) {
        free_rule(*left);
        return FALSE;
    }
    return TRUE;
}

BEGIN_RULE_FUNC_PARSE(and, binary_rule) {
    rule_t *left = NULL, *right = NULL;
    if (!process_binary_rule(buf, &left, &right))
        return NULL;
    return make_binary_rule(left, right, TRUE);
}

END_RULE_FUNC BEGIN_RULE_FUNC_PARSE(or, binary_rule) {
    rule_t *left = NULL, *right = NULL;
    if (!process_binary_rule(buf, &left, &right))
        return NULL;
    return make_binary_rule(left, right, FALSE);
}

END_RULE_FUNC PUBLISH_RULE_VTABLE_MANUAL(and, and, binary, binary, and, and, and);
PUBLISH_RULE_VTABLE_MANUAL(or, or, binary, binary, or, or, or);

/*
 * NOT RULE
 */
BEGIN_RULE_DATATYPE;
rule_t *child;
PUBLISH_RULE_DATATYPE(unary_rule);

RULE_CONSTRUCTOR(make_not_rule)(rule_t *child) {
    unary_rule *ur = MAKE_RULE(not, unary_rule);
    ur->child = child;
    return (rule_t *)ur;
}

BEGIN_RULE_FUNC_MATCHER(not, unary_rule) { return !rule_matches(rule->child, ch); }

END_RULE_FUNC BEGIN_RULE_FUNC_DESTRUCTOR(unary, unary_rule) {
    free_rule(rule->child);
    free(rule);
}

END_RULE_FUNC BEGIN_RULE_FUNC_PRINT(unary, unary_rule) { sprint_rule(buf, size, rule->child); }

END_RULE_FUNC BEGIN_RULE_FUNC_ABBR(not, unary_rule) {
    rule = rule; /* avoid compiler warning */
    snprintf(buf, size, "NOT");
}

END_RULE_FUNC BEGIN_RULE_FUNC_VERBOSE(not, unary_rule) {
    size_t diff;
    snprintf(buf, size, "not (");
    SUD(size, 3);
    buf += 3;
    rule_verbose(buf, size, rule->child);
    diff = strlen(buf);
    SUD(size, diff);
    buf += diff;
    snprintf(buf, size, ")");
}
END_RULE_FUNC BEGIN_RULE_FUNC_PARSE(not, unary_rule) {
    rule_t *child = parse_rule(buf);
    if (child)
        return make_not_rule(child);
    else
        return NULL;
}

END_RULE_FUNC PUBLISH_RULE_VTABLE_MANUAL(not, not, unary, unary, not, not, not );
