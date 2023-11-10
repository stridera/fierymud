/***************************************************************************
 *   File: spell_mem.c                                    Part of FieryMUD *
 *  Usage: This file contains all of the code for spell_memorization.      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *                                                                         *
 * This code was received origonally from HubisMUD in 1998 and no lable or *
 * claim of ownership or copyright was made anywhere in the file.          *
 ***************************************************************************/

#include "casting.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "events.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "players.hpp"
#include "regen.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <algorithm>
#include <fmt/format.h>

#define MEM_INTERVAL PULSE_VIOLENCE / 4
#define SCRIBE_INTERVAL PASSES_PER_SEC

/* --------function prototypes ---------*/
ACMD(do_meditate);
ACMD(do_action);
ACMD(do_scribe);

/* memorizing and praying */
void show_spell_list(CharData *ch, CharData *tch);
int rem_spell(CharData *ch, int spell);
void update_spell_mem(void);
void start_studying(CharData *ch);
int set_mem_time(CharData *ch, int spell);
void save_mem_list(CharData *ch);
int restore_spells(CharData *ch, int spell, int state);
void assign_mem_list(CharData *ch);
void done_memming(CharData *ch);

/* spell book stuff */
int add_spell_to_book(CharData *ch, ObjData *obj, int spell);
int book_contains_spell(ObjData *obj, int spell);
ObjData *find_spellbook_with_spell(CharData *ch, int spell);
CharData *find_teacher_for_spell(CharData *ch, int spell);
void print_spells_in_book(CharData *ch, ObjData *obj, char *dest_buf);
int room_in_book(ObjData *obj, int pages);
void start_scribing(CharData *ch);
void clear_scribing(CharData *ch);
int add_spell_scribe(CharData *ch, int spell);
int rem_spell_scribe(CharData *ch, int spell);
int get_spell_pages(CharData *ch, int spell);
int start_scribing_spell(CharData *ch, ObjData *spellbook, Scribing *scr);
int spells_used_for_circle(CharData *ch, int circle);

// Globals
const char *circle_abbrev[NUM_SPELL_CIRCLES + 1] = {"!UNUSED!", " 1st", " 2nd", " 3rd", " 4th", " 5th", " 6th", " 7th",
                                                    " 8th",     " 9th", "10th", "11th", "12th", "13th", "14th"};

/*---- spell table -------
 * This table describes how many spells a caster can memorize from a
 * specific CIRCLE at each level. This is class Independent. Class
 * dependencies can be controlled using spell_level() calls in class.c
 * IF YOU CHANGE THIS ARRAY, YOU MUST CHANGE STRUCTS.H MAX_CHAR_SPELLS
 * to the new value...
 */

int spells_of_circle[(LVL_IMPL + 1)][(NUM_SPELL_CIRCLES + 1)] = {
    /* level 0 and circle 0 are NOT USED!!! */
    /* 0  1   2   3   4   5   6   7   8   9  10  11  12  13  14<-SPELL CIRCLE */

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 0 */
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 1 CIRCLE 1 */
    {0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 2 */
    {0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 3 */
    {0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 4 */
    {0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 5 */
    {0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 6 */
    {0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 7 */
    {0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 8 */
    {0, 7, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 9 CIRCLE 2 */
    {0, 7, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 10 */
    {0, 7, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 11 */
    {0, 7, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 12 */
    {0, 7, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 13 */
    {0, 7, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 14 */
    {0, 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 15 */
    {0, 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 16 */
    {0, 7, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 17 CIRCLE 3 */
    {0, 7, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 18 */
    {0, 7, 6, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 19 */
    {0, 7, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 20 */
    {0, 7, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 21 */
    {0, 7, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 22 */
    {0, 7, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 23 */
    {0, 7, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 24 */
    {0, 7, 7, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 25 CIRCLE 4 */
    {0, 7, 7, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 26 */
    {0, 7, 7, 6, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 27 */
    {0, 7, 7, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 28 */
    {0, 7, 7, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 29 */
    {0, 7, 7, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 30 */
    {0, 7, 7, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 31 */
    {0, 7, 7, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 32 */
    {0, 7, 7, 6, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 33 CIRCLE 5 */
    {0, 7, 7, 6, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 34 */
    {0, 7, 7, 6, 6, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 35 */
    {0, 7, 7, 6, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 36 */
    {0, 7, 7, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 37 */
    {0, 7, 7, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 38 */
    {0, 7, 7, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 39 */
    {0, 7, 7, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 40 */
    {0, 7, 7, 6, 6, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 41 CIRCLE 6 */
    {0, 7, 7, 6, 6, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 42 */
    {0, 7, 7, 6, 6, 6, 3, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 43 */
    {0, 7, 7, 6, 6, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 44 */
    {0, 7, 7, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 45 */
    {0, 7, 7, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 46 */
    {0, 7, 7, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 47 */
    {0, 7, 7, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0},           /* level 48 */
    {0, 7, 7, 7, 6, 6, 6, 1, 0, 0, 0, 0, 0, 0, 0},           /* level 49 CIRCLE 7 */
    {0, 7, 7, 7, 6, 6, 6, 2, 0, 0, 0, 0, 0, 0, 0},           /* level 50 */
    {0, 7, 7, 7, 6, 6, 6, 3, 0, 0, 0, 0, 0, 0, 0},           /* level 51 */
    {0, 7, 7, 7, 6, 6, 6, 4, 0, 0, 0, 0, 0, 0, 0},           /* level 52 */
    {0, 7, 7, 7, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0},           /* level 53 */
    {0, 7, 7, 7, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0},           /* level 54 */
    {0, 7, 7, 7, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0},           /* level 55 */
    {0, 7, 7, 7, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0},           /* level 56 */
    {0, 7, 7, 7, 6, 6, 6, 6, 1, 0, 0, 0, 0, 0, 0},           /* level 57 CIRCLE 8 */
    {0, 7, 7, 7, 6, 6, 6, 6, 2, 0, 0, 0, 0, 0, 0},           /* level 58 */
    {0, 7, 7, 7, 6, 6, 6, 6, 3, 0, 0, 0, 0, 0, 0},           /* level 59 */
    {0, 7, 7, 7, 6, 6, 6, 6, 4, 0, 0, 0, 0, 0, 0},           /* level 60 */
    {0, 7, 7, 7, 6, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0},           /* level 61 */
    {0, 7, 7, 7, 6, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0},           /* level 62 */
    {0, 7, 7, 7, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0},           /* level 63 */
    {0, 7, 7, 7, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0},           /* level 64 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 1, 0, 0, 0, 0, 0},           /* level 65 CIRCLE 9 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 2, 0, 0, 0, 0, 0},           /* level 66 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 3, 0, 0, 0, 0, 0},           /* level 67 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 4, 0, 0, 0, 0, 0},           /* level 68 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 5, 0, 0, 0, 0, 0},           /* level 69 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 5, 0, 0, 0, 0, 0},           /* level 70 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 5, 0, 0, 0, 0, 0},           /* level 71 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0},           /* level 72 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 1, 0, 0, 0, 0},           /* level 73 CIRCLE 10 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 2, 0, 0, 0, 0},           /* level 74 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 3, 0, 0, 0, 0},           /* level 75 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0, 0},           /* level 76 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 5, 0, 0, 0, 0},           /* level 77 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 5, 0, 0, 0, 0},           /* level 78 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 5, 0, 0, 0, 0},           /* level 79 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0},           /* level 80 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 1, 0, 0, 0},           /* level 81 CIRCLE 11 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 2, 0, 0, 0},           /* level 82 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 3, 0, 0, 0},           /* level 83 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0},           /* level 84 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0},           /* level 85 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0},           /* level 86 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0},           /* level 87 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0},           /* level 88 */
    {0, 8, 8, 7, 7, 7, 7, 6, 6, 6, 6, 5, 1, 0, 0},           /* level 89 CIRCLE 12 */
    {0, 8, 8, 8, 7, 7, 7, 6, 6, 6, 6, 5, 2, 0, 0},           /* level 90 */
    {0, 8, 8, 8, 7, 7, 7, 6, 6, 6, 6, 5, 3, 0, 0},           /* level 91 */
    {0, 8, 8, 8, 8, 7, 7, 7, 6, 6, 6, 5, 4, 0, 0},           /* level 92 */
    {0, 8, 8, 8, 8, 7, 7, 7, 6, 6, 6, 5, 4, 0, 0},           /* level 93 */
    {0, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 5, 4, 0, 0},           /* level 94 */
    {0, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 5, 4, 0, 0},           /* level 95 */
    {0, 8, 8, 8, 8, 8, 8, 7, 7, 6, 6, 5, 4, 0, 0},           /* level 96 */
    {0, 9, 9, 8, 8, 8, 8, 8, 7, 7, 6, 5, 4, 1, 0},           /* level 97 CIRCLE 13 */
    {0, 9, 9, 9, 9, 9, 9, 8, 7, 7, 6, 5, 4, 2, 0},           /* level 98 */
    {0, 10, 10, 10, 10, 10, 10, 9, 8, 7, 6, 5, 4, 3, 0},     /* level 99 */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0}, /* level 100 */
                                                             /* Immortal+ */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0}, /* level 101 */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0}, /* level 102 */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0}, /* level 103 */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0}, /* level 104 */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0}  /* level 105 CIRCLE 14 */
}; /* 11+11+11+11+11+11+11+11+11+11+8+7+6 = 131 = max_char_spells  */

int circle_recover_time[(NUM_SPELL_CIRCLES + 1)] = {20, 25, 40, 55, 80, 95, 130, 145, 165, 210, 250, 290, 310};

// Locals
CharData *memming = 0; /* head of memming characters linked list */

int mob_mem_time(CharData *ch, int circle) {
    int mem_time;

    mem_time = 9 - (GET_LEVEL(ch) - circle_to_level(circle)) / 2;

    if (mem_time < 2)
        mem_time = 2;

    /* Now adjust it for meditation. */
    if (GET_SKILL(ch, SKILL_MEDITATE))
        mem_time *= 0.3 + 0.007 * (100 - GET_SKILL(ch, SKILL_MEDITATE));

    return std::max(mem_time, 1);
}

EVENTFUNC(scribe_event) {
    CharData *ch = (CharData *)event_obj;
    Scribing *cur, *next_scribing;
    ObjData *obj;
    ObjData *held_right = 0;
    ObjData *held_left = 0;
    int right_type = 0, left_type = 0;

    /* Check for conditions that will interrupt scribing. */

    if (FIGHTING(ch)) {
        char_printf(ch, "Your work is rudely interrupted!\n");
        char_printf(ch, "You stop scribing.\n");
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    if (IS_DRUNK(ch)) {
        char_printf(ch, "Sober up first, lush!\n");
        char_printf(ch, "You stop scribing.\n");
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    if (GET_POS(ch) != POS_SITTING || GET_STANCE(ch) < STANCE_RESTING || GET_STANCE(ch) > STANCE_ALERT) {
        char_printf(ch, "You stop scribing.\n");
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    /* Make sure writing instrument and book are held. */

    held_right = GET_EQ(ch, WEAR_HOLD);
    held_left = GET_EQ(ch, WEAR_HOLD2);

    if (held_right)
        right_type = GET_OBJ_TYPE(held_right);

    if (held_left)
        left_type = GET_OBJ_TYPE(held_left);

    if (right_type != ITEM_PEN && left_type != ITEM_PEN) {
        char_printf(ch, "You need something to write with.\n");
        char_printf(ch, "You stop scribing.\n");
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    if (right_type != ITEM_SPELLBOOK) {
        if (left_type != ITEM_SPELLBOOK) {
            char_printf(ch, "You are no longer holding a spellbook.\n");
            clear_scribing(ch);
            return EVENT_FINISHED;
        } else {
            obj = held_left;
        }
    } else {
        obj = held_right;
    }

    /* Can't scribe in the dark. */

    if (!LIGHT_OK(ch)) {
        char_printf(ch, "It is too dark, so you stop scribing.\n");
        act("Unable to see, $n gives up scribing.", true, ch, 0, 0, TO_ROOM);
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    cur = ch->scribe_list;
    cur->scribe_time += 1;

    /* Did your teacher go away?  Lose your source book somehow? */

    if (!find_spellbook_with_spell(ch, cur->spell) && !find_teacher_for_spell(ch, cur->spell)) {
        char_printf(ch, "You've lost your source for {}!\n", skill_name(cur->spell));
        char_printf(ch, "&3With a weary sigh, you stop scribing.&0\n");
        act("$n sighs and stops scribing.", true, ch, 0, 0, TO_ROOM);
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    /* There is a chance to improve the scribe skill in each round of scribing. */

    if (random_number(1, 20) > 15) {
        improve_skill(ch, SKILL_SCRIBE);
    }

    if (cur->scribe_time >= PAGE_SCRIBE_TIME) {

        /* A page has been finished. */

        cur->scribe_time = 0;
        cur->pages_left -= 1;

        if (cur->pages_left > 0) {
            if (PAGE_SCRIBE_TIME > 1) {
                char_printf(ch, "You finish a page in your spellbook.\n");
            }
            return SCRIBE_INTERVAL;
        }

        /* A spell has been finished. */

        /* Add this spell to the spellbook. */
        next_scribing = cur->next;
        add_spell_to_book(ch, obj, cur->spell);

        /* Any more spells in this person's scribe list? */

        if (!next_scribing) {
            char_printf(ch, "&6You have finished scribing {}.  &3You are done scribing.&0\n", skill_name(cur->spell));
            clear_scribing(ch);
            return EVENT_FINISHED;
        } else {
            char_printf(ch, "&6You have finished scribing {}&0.\n", skill_name(cur->spell));
        }

        rem_spell_scribe(ch, cur->spell);

        /* Scribe the next spell, if possible. */

        for (cur = next_scribing; cur; cur = next_scribing) {
            next_scribing = cur->next;
            if (start_scribing_spell(ch, obj, cur))
                return SCRIBE_INTERVAL;
            rem_spell_scribe(ch, cur->spell);
        }

        /* None of the spells in the scribing list could be scribed. */

        char_printf(ch, "&3You are done scribing.&0\n");
        clear_scribing(ch);
        return EVENT_FINISHED;
    }
    return SCRIBE_INTERVAL;
}

/* set the meditate flag */
ACMD(do_meditate) {
    if (GET_SKILL(ch, SKILL_MEDITATE) == 0) {
        char_printf(ch, "You just can't seem to focus your mind enough.\n");
        return;
    }
    if (GET_POS(ch) != POS_SITTING || GET_STANCE(ch) < STANCE_RESTING || GET_STANCE(ch) > STANCE_ALERT) {
        char_printf(ch, "Try resting first....\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
        char_printf(ch, "You're already meditating!\n");
        return;
    }

    if (GET_CLASS(ch) == CLASS_BERSERKER) {
        act("$n closes $s eyes and begins meditating.\n", true, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You begin to meditate, letting your rage build...\n");
        check_regen_rates(ch);
    } else {
        act("$n begins meditating to improve $s concentration.", true, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You begin to meditate.\n");
    }

    SET_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
    improve_skill(ch, SKILL_MEDITATE);
}

void free_scribe_list(CharData *ch) {
    Scribing *next;
    while (ch->scribe_list) {
        next = ch->scribe_list->next;
        free(ch->scribe_list);
        ch->scribe_list = next;
    }
}

void show_available_slots(CharData *ch, CharData *tch) {
    if (IS_NPC(tch)) {
        char_printf(ch, "NPCs don't have spell slots.\n");
        return;
    }

    int restore_rate = get_spellslot_restore_rate(tch);

    if (ch == tch) {
        char_printf(ch, "You have the following spell slots available:\n");
    } else {
        char_printf(ch, "{} has the following spell slots available:\n", GET_NAME(tch));
    }

    for (int i = 1; i <= NUM_SPELL_CIRCLES; i++) {
        int slots = spells_of_circle[GET_LEVEL(tch)][i] - spells_used_for_circle(tch, i);
        if (slots > 0)
            char_printf(ch, "Circle {:>2}: {}\n", i, slots);
    }

    if (!tch->spellcasts.empty()) {
        if (restore_rate > 0) {
            char_printf(ch, "\nRestoring:\n");
            for (auto &sc : tch->spellcasts) {
                if (sc.ticks > 0) {
                    char_printf(ch, "  Circle {:>2}   ({:>3} sec)\n", sc.circle, std::ceil(sc.ticks / restore_rate));
                }
            }
        } else {
            char_printf(ch, "\nYou focus is too low to restore any spell slots.\n");
        }
    }

    char_printf(ch, "\n[DEBUG] Current Restore Rate: {}.\n", restore_rate);
}

int spells_used_for_circle(CharData *ch, int circle) {
    return std::count_if(ch->spellcasts.begin(), ch->spellcasts.end(),
                         [ch, circle](const SpellCast &sc) { return circle == sc.circle; });
}

int slots_available_for_circle(CharData *ch, int circle) {
    return spells_of_circle[GET_LEVEL(ch)][circle] - spells_used_for_circle(ch, circle);
}

int spell_slot_available(CharData *ch, int spell) {
    int circle = SPELL_CIRCLE(ch, spell);

    if (circle < 1 || circle > NUM_SPELL_CIRCLES)
        return 0;

    if (IS_NPC(ch))
        return GET_MOB_SPLBANK(ch, circle) > 0;

    return std::count_if(ch->spellcasts.begin(), ch->spellcasts.end(), [ch, circle](const SpellCast &sc) {
               return circle == sc.circle;
           }) < spells_of_circle[GET_LEVEL(ch)][circle];
}

int get_next_spell_slot_available(CharData *ch, int spell) {
    int circle = SPELL_CIRCLE(ch, spell);

    if (circle < 1 || circle > NUM_SPELL_CIRCLES)
        return 0;

    if (IS_NPC(ch))
        return GET_MOB_SPLBANK(ch, circle) > 0;

    for (int i = circle; i < NUM_SPELL_CIRCLES; i++) {
        if (std::count_if(ch->spellcasts.begin(), ch->spellcasts.end(), [ch, i](const SpellCast &sc) {
                return i == sc.circle;
            }) < spells_of_circle[GET_LEVEL(ch)][i]) {
            return i;
        }
    }
    return 0;
}

int get_spellslot_restore_rate(CharData *ch) {
    double rate = GET_FOCUS(ch) / 10;
    int class_bonus;

    // Add Race Bonuses
    rate *= races[(int)GET_RACE(ch)].bonus_focus / 100.0;

    // Determine a bonus factor by class based on stats
    // Uses ability score 72 as "full" value; scores greater than 72 will add above scale
    switch (GET_CLASS(ch)) {
    case CLASS_CLERIC:
    case CLASS_DIABOLIST:
    case CLASS_DRUID:
    case CLASS_PRIEST:
        // base 105; add approx 20% scale - 13% from Wis, 7% from Int         * max: 132.77 *
        class_bonus = (13 * (GET_WIS(ch) / 72)) + (7 * (GET_INT(ch) / 72));
        break;
    case CLASS_CRYOMANCER:
    case CLASS_ILLUSIONIST:
    case CLASS_NECROMANCER:
    case CLASS_PYROMANCER:
    case CLASS_SORCERER:
        // base 105; add approx 20% scale - 13% from Int, 7% from Wis         * max: 132.77 *
        class_bonus = (13 * (GET_INT(ch) / 72)) + (7 * (GET_WIS(ch) / 72));
        break;
    case CLASS_ANTI_PALADIN:
    case CLASS_PALADIN:
        // base 110; add approx 10% scale - 7% from Wis, 3% from Int          * max: 123.88 *
        class_bonus = (7 * (GET_WIS(ch) / 72)) + (3 * (GET_INT(ch) / 72));
        break;
    case CLASS_RANGER:
        // base 110; add approx 10% scale - 7% from Int, 3% from Wis          * max: 123.88 *
        class_bonus = (7 * (GET_INT(ch) / 72)) + (3 * (GET_WIS(ch) / 72));
        break;
    case CLASS_BARD:
        // base 109; add approx 14% scale - 7% from Wis, 7% from Int          * max: 128.44 *
        class_bonus = (7 * (GET_WIS(ch) / 72)) + (7 * (GET_INT(ch) / 72));
        break;
    }

    // Add Class Bonuses = base class scale + bonus factor determined above
    rate *= (classes[(int)GET_CLASS(ch)].bonus_focus + class_bonus) / 100.0;

    // Add Meditate Bonus
    if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
        if (IS_NPC(ch))
            rate += (GET_LEVEL(ch) / 100 * MEDITATE_BONUS) + 1;
        if (GET_SKILL(ch, SKILL_MEDITATE))
            rate += (GET_SKILL(ch, SKILL_MEDITATE) / 100 * MEDITATE_BONUS) + 1;
    }
    return rate;
}

ACMD(do_study) {
    CharData *tch;

    if (!ch || IS_NPC(ch))
        return;

    show_available_slots(ch, ch);
}

void charge_mem(CharData *ch, int spellnum, int circle = -1) {

    // TODO: Uncomment this when testing is done.
    // if (GET_LEVEL(ch) >= LVL_IMMORT)
    //     return;

    int recover_time;

    if (circle == -1)
        circle = SPELL_CIRCLE(ch, spellnum);

    recover_time = circle_recover_time[circle] + skills[spellnum].addl_mem_time;

    ch->spellcasts.push_back(SpellCast(circle, recover_time));

    if (!EVENT_FLAGGED(ch, EVENT_REGEN_SPELLSLOT))
        set_regen_event(ch, EVENT_REGEN_SPELLSLOT);
}

/********************/
/**** SPELLBOOKS ****/
/********************/

bool has_spellbook(CharData *ch) {
    ObjData *obj;

    for (obj = ch->carrying; obj; obj = obj->next_content)
        if (obj->obj_flags.type_flag == ITEM_SPELLBOOK)
            return true;

    obj = ch->equipment[WEAR_HOLD];
    if (obj && obj->obj_flags.type_flag == ITEM_SPELLBOOK)
        return true;

    obj = ch->equipment[WEAR_HOLD2];
    if (obj && obj->obj_flags.type_flag == ITEM_SPELLBOOK)
        return true;

    return false;
}

/*
 * is_spellbook_with_spell
 *
 * Determine whether this object is a spellbook, and whether the
 * given spell is written in it.
 *
 */

int is_spellbook_with_spell(ObjData *obj, int spell) {
    return obj->obj_flags.type_flag == ITEM_SPELLBOOK && book_contains_spell(obj, spell);
}

/*
 * find_spellbook_with_spell
 *
 * Identify a spellbook on this person which has the requested spell.
 *
 */

ObjData *find_spellbook_with_spell(CharData *ch, int spell) {
    ObjData *obj;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (is_spellbook_with_spell(obj, spell) && CAN_SEE_OBJ(ch, obj))
            return obj;
    }

    obj = ch->equipment[WEAR_HOLD];
    if (obj && is_spellbook_with_spell(obj, spell) && CAN_SEE_OBJ(ch, obj))
        return obj;

    obj = ch->equipment[WEAR_HOLD2];
    if (obj && is_spellbook_with_spell(obj, spell) && CAN_SEE_OBJ(ch, obj))
        return obj;

    return 0;
}

/*
 * book_contains_spell
 *
 * Determine whether this object is a spellbook, in which is written
 * the given spell.
 *
 */
int book_contains_spell(ObjData *obj, int spell) {
    SpellBookList *entry;

    for (entry = obj->spell_book; entry; entry = entry->next)
        if (entry->spell == spell)
            return true;

    return false;
}

/*
 * print_spells_in_book
 *
 * Prints a list of spells in the spellbook to the character.
 *
 * You can also pass a buffer for the text, if desired.  If you do,
 * the text will be appended to any string in it.  If you do not,
 * the text will be sent to the character.
 *
 * (This feature is needed by show_obj_to_char() in act.informative.c)
 *
 */

void print_spells_in_book(CharData *ch, ObjData *obj, char *dest_buf) {
    int spage = 0, fpage = 0;
    SpellBookList *entry;
    char list_buf[MAX_STRING_LENGTH];
    char *obuf = list_buf;

    list_buf[0] = 0;
    if (dest_buf)
        obuf = dest_buf;

    if (!obj->spell_book) {
        sprintf(obuf, "%sThere is nothing written in it.\n", obuf);
        if (!dest_buf)
            char_printf(ch, obuf);
        return;
    }

    sprintf(obuf, "%sThe following is written in it:\n", obuf);

    for (entry = obj->spell_book; entry; entry = entry->next) {
        spage = fpage + 1;
        fpage = fpage + entry->length;
        if ((int)GET_LEVEL(ch) < skills[entry->spell].min_level[(int)GET_CLASS(ch)]) {
            sprintf(obuf, "%s&6%3d-%3d:&0 &9&b%s&0\n", obuf, spage, fpage, skills[entry->spell].name);
        } else {
            sprintf(obuf, "%s&6%3d-%3d:&0 &6&b%s&0\n", obuf, spage, fpage, skills[entry->spell].name);
        }
    }

    if ((GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES) - fpage) > 1) {
        sprintf(obuf, "%s&6%3d-%3d:&0 &4&b(blank)&0\n", obuf, fpage + 1, GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES));
    } else if ((GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES) - fpage) == 1) {
        sprintf(obuf, "%s    &6%3d:&0 &4&b(blank)&0\n", obuf, GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES));
    }

    if (!dest_buf)
        char_printf(ch, obuf);
    return;
}

/*
 * add_spell_to_book
 *
 * Yup, adds a spell to a book.  As a result of scribing.
 *
 */

int add_spell_to_book(CharData *ch, ObjData *obj, int spell) {
    SpellBookList *entry;
    int pages;

    if (book_contains_spell(obj, spell)) {
        char_printf(ch, "That spell is already in this book.\n");
        return false;
    }

    pages = get_spell_pages(ch, spell);

    if (!room_in_book(obj, pages)) {
        char_printf(ch, "Your spellbook is too full for that spell.\n");
        return false;
    }

    if (obj->spell_book) {
        for (entry = obj->spell_book; entry->next; entry = entry->next)
            ;
        CREATE(entry->next, SpellBookList, 1);
        entry = entry->next;
    } else {
        CREATE(entry, SpellBookList, 1);
        obj->spell_book = entry;
    }

    entry->spell = spell;
    entry->length = pages;
    return true;
}

/*
 * pages_left_in_book
 *
 * How many pages in this spellbook are still blank?
 *
 */

int pages_left_in_book(ObjData *obj) {
    SpellBookList *entry;
    int pages_used = 0;

    if (obj->obj_flags.type_flag != ITEM_SPELLBOOK)
        return 0;

    for (entry = obj->spell_book; entry; entry = entry->next)
        pages_used += entry->length;

    return GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES) - pages_used;
}

/*
 * do_scribe
 *
 * Someone has issued the scribe command, e.g. "scribe magic missile".
 *
 */

ACMD(do_scribe) {
    int spellnum, pages_needed, pages_left;
    ObjData *book = 0, *pen, *sourcebook;
    CharData *teacher;
    ObjData *held_right = GET_EQ(ch, WEAR_HOLD);
    ObjData *held_left = GET_EQ(ch, WEAR_HOLD2);
    int right_type = 0, left_type = 0;
    Scribing *sl;

    /* Mobs can't scribe. */

    if (IS_NPC(ch))
        return;

    /* Yeah, scribe a spell while fighitng... */

    if (FIGHTING(ch)) {
        char_printf(ch, "If you wanna commit suicide just say so!\n");
        return;
    }

    /* Can't scribe in darkness. */

    if (!LIGHT_OK(ch)) {
        char_printf(ch, "It is too dark for writing.\n");
        return;
    }

    /* Can't scribe while confused. */

    if (CONFUSED(ch)) {
        char_printf(ch, "You're too confused to write!\n");
        return;
    }

    /* Make sure they are holding a writing instrument and a spellbook. */

    if (held_right)
        right_type = GET_OBJ_TYPE(held_right);
    if (held_left)
        left_type = GET_OBJ_TYPE(held_left);

    /* Find the writing instrument. */

    if (right_type == ITEM_PEN) {
        pen = held_right;
    } else if (left_type == ITEM_PEN) {
        pen = held_left;
    } else {
        char_printf(ch, "You don't seem to have anything to write with.\n");
        return;
    }

    /* Find the spellbook. */

    if (right_type == ITEM_SPELLBOOK) {
        book = held_right;
    } else if (left_type == ITEM_SPELLBOOK) {
        book = held_left;
    } else {
        char_printf(ch, "You need to hold a spellbook before you can write in it.\n");
        return;
    }

    /* Make sure player is in proper position. */

    if (GET_POS(ch) != POS_SITTING || GET_STANCE(ch) < STANCE_RESTING || GET_STANCE(ch) > STANCE_ALERT) {
        char_printf(ch, "You have to be sitting to scribe.\n");
        return;
    }

    /* Identify the spell to be scribed. */

    argument = delimited_arg_all(argument, arg, '\'');

    if (!*arg) {
        char_printf(ch, "What spell do you want to scribe?\n");
        return;
    }

    spellnum = find_spell_num(arg);

    if (!IS_SPELL(spellnum)) {
        char_printf(ch, "Try all you want, but there's no such thing.\n");
        return;
    }

    /* Make sure there's a source for the spell - another character, or a
     * spellbook. */

    sourcebook = find_spellbook_with_spell(ch, spellnum);
    teacher = find_teacher_for_spell(ch, spellnum);
    if (!sourcebook && !teacher) {
        char_printf(ch, "There is nobody here to teach that spell, and nothing to copy it from.\n");
        return;
    }

    /* Make sure it's a spell they are allowed to use, according to class and
     * level. */

    if ((int)GET_LEVEL(ch) < skills[spellnum].min_level[(int)GET_CLASS(ch)] || GET_SKILL(ch, spellnum) == 0) {
        char_printf(ch, "You don't understand the magic used in that spell.\n");
        return;
    }

    /* Don't allow a spell to be scribed into a book twice (waste of time/space).
     */

    if (book_contains_spell(book, spellnum)) {
        char_printf(ch, "That spell is already written in this book.\n");
        return;
    }

    /* Make sure there's space in the book. */

    pages_needed = get_spell_pages(ch, spellnum);
    pages_left = pages_left_in_book(book);
    if (!room_in_book(book, pages_needed)) {
        if (pages_left == 0) {
            act("$P is full.", false, ch, 0, book, TO_CHAR);
        } else {
            sprintf(buf, "You'd need %d pages to scribe that, but $P only has %d page%s left.", pages_needed,
                    pages_left, pages_left == 1 ? "" : "s");
            act(buf, false, ch, 0, book, TO_CHAR);
        }
        return;
    }

    /* Make sure they aren't already scribing it, or planning to. */

    for (sl = ch->scribe_list; sl; sl = sl->next)
        if (sl->spell == spellnum) {
            if (sl == ch->scribe_list)
                char_printf(ch, "You're already scribing that spell!\n");
            else
                char_printf(ch, "You are already planning on scribing that spell.\n");
            return;
        }

    /* Add the spell to the list of spells that this character is scribing. */

    add_spell_scribe(ch, spellnum);

    /* Make sure player is scribing, and provide feedback. */

    if (!EVENT_FLAGGED(ch, EVENT_SCRIBE)) {
        start_scribing(ch);
        act("$n picks up $s $o and starts writing in $P.", true, ch, pen, book, TO_ROOM);
        char_printf(ch, "You begin scribing {}.\n", skill_name(spellnum));
    } else {
        char_printf(ch, "You make a mental note to scribe {}.\n", skill_name(spellnum));
    }
}

/*
 * find_teacher_for_spell
 *
 * Given a spell, locate a player or NPC who knows the spell and could
 * teach it, thus allowing someone to scribe it nearby.
 *
 */

CharData *find_teacher_for_spell(CharData *ch, int spell) {
    CharData *dude;

    for (dude = world[ch->in_room].people; dude; dude = dude->next_in_room) {

        if (dude == ch || !CAN_SEE(ch, dude))
            continue;

        /* It must be a player or a teacher */

        if (!(IS_NPC(dude) && MOB_FLAGGED(dude, MOB_TEACHER)) && IS_NPC(dude))
            continue;

        /* It must know that spell */

        if (GET_LEVEL(dude) < skills[spell].min_level[(int)GET_CLASS(dude)])
            continue;

        return dude;
    }

    return 0;
}

/*
 * room_in_book
 *
 * Does this book have enough pages?
 *
 */

int room_in_book(ObjData *obj, int pages) { return pages_left_in_book(obj) >= pages; }

/*
 * start_scribing_spell
 *
 * Determine if a spell can be scribed, and give feedback.
 * Returns true if it can be scribed, false otherwise.
 *
 */

int start_scribing_spell(CharData *ch, ObjData *spellbook, Scribing *scr) {
    CharData *teacher;
    ObjData *sourcebook;

    /* Is there room in the book? */

    if (scr->pages > pages_left_in_book(spellbook)) {
        act("&3There is no room to scribe $T&3 in $p&3.&0", false, ch, spellbook, skill_name(scr->spell), TO_CHAR);
        return false;
    }

    /* Find out who/what we're scribing from. */
    teacher = find_teacher_for_spell(ch, scr->spell);
    if (teacher) {
        sprintf(buf, "With $N's help, you start scribing %s.", skill_name(scr->spell));
        act(buf, false, ch, 0, teacher, TO_CHAR);
        sprintf(buf, "With $N's help, $n starts scribing %s.", skill_name(scr->spell));
        act(buf, true, ch, 0, teacher, TO_NOTVICT);
        sprintf(buf, "With your help, $n starts scribing %s.", skill_name(scr->spell));
        act(buf, false, ch, 0, teacher, TO_VICT);
        return true;
    }

    sourcebook = find_spellbook_with_spell(ch, scr->spell);
    if (sourcebook) {
        act("You start scribing $T from $o.", false, ch, sourcebook, skill_name(scr->spell), TO_CHAR);
        act("$n starts scribing from $o.", true, ch, sourcebook, 0, TO_ROOM);
        return true;
    }

    char_printf(ch, "There is no source for {} nearby, so you skip it for now.\n", skill_name(scr->spell));
    return false;
}

/*
 * clear_scribing
 *
 * Clear a character's scribing list.
 *
 */

void clear_scribing(CharData *ch) {
    Scribing *scribe;

    REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_SCRIBE);

    scribe = ch->scribe_list;
    while (scribe) {
        rem_spell_scribe(ch, scribe->spell);
        scribe = ch->scribe_list;
    }
    ch->scribe_list = 0;
}

/*
 * add_spell_scribe
 *
 * Add a spell to the list of spells that a player is scribing.
 *
 */

int add_spell_scribe(CharData *ch, int spell) {
    Scribing *cur, *sc;

    CREATE(cur, Scribing, 1);
    if (!ch->scribe_list) {
        ch->scribe_list = cur;
    } else {
        for (sc = ch->scribe_list; sc->next; sc = sc->next)
            ;
        sc->next = cur;
    }

    cur->spell = spell;
    cur->pages = get_spell_pages(ch, spell);
    cur->pages_left = get_spell_pages(ch, spell);
    return 1;
}

/*
 * rem_spell_scribe
 *
 * Remove a spell from the list of spells that a player is scribing.
 * Returns 1 if the spell was found and removed, else 0.
 *
 */

int rem_spell_scribe(CharData *ch, int spell) {
    Scribing *temp, *cur;

    cur = temp = ch->scribe_list;

    while (cur) {
        if (cur->spell == spell) {
            if (cur == ch->scribe_list)
                ch->scribe_list = cur->next;
            else
                temp->next = cur->next;
            free(cur);
            return 1;
        }
        temp = cur;
        cur = cur->next;
    }
    return 0;
}

/*
 * start_scribing
 *
 * Start a player scribing.
 *
 */

void start_scribing(CharData *ch) {
    if (!EVENT_FLAGGED(ch, EVENT_SCRIBE)) {
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_SCRIBE);
        event_create(EVENT_SCRIBE, scribe_event, ch, false, &(ch->events), SCRIBE_INTERVAL);
    }
}

/*
 *
 * get_spell_pages
 *
 * Get the number of pages that a spell would take to scribe,
 * according to the scribe skill of the player who wishes to
 * scribe it.
 *
 */

int get_spell_pages(CharData *ch, int spell) {
    float pages = 0, factor;
    int x;

    factor = (100 - (float)GET_SKILL(ch, SKILL_SCRIBE)) / 100;

    for (x = 1; x <= skills[spell].pages; x++) {
        pages += factor;
    }

    if (pages < 1)
        return 1;
    else
        return (int)pages + 1;
}
