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
#include "logging.hpp"


#include <fmt/format.h>

#define MEM_INTERVAL PULSE_VIOLENCE / 4
#define SCRIBE_INTERVAL PASSES_PER_SEC

/* --------function prototypes ---------*/
ACMD(do_meditate);
ACMD(do_pray);
ACMD(do_action);
ACMD(do_scribe);

/* memorizing and praying */
void show_spell_list(CharData *ch, CharData *tch);
int rem_spell(CharData *ch, int spell);
int check_spell_memory(CharData *ch, int spellnum);
void charge_mem(CharData *ch, int spellnum);
void update_spell_mem(void);
void start_memming(CharData *ch);
void rem_memming(CharData *ch);
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

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 0 */
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 1 CIRCLE 1 */
    {0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 2 */
    {0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 3 */
    {0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 4 */
    {0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 5 */
    {0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 6 */
    {0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 7 */
    {0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 8 */
    {0, 7, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 9 CIRCLE 2 */
    {0, 7, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 10 */
    {0, 7, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 11 */
    {0, 7, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 12 */
    {0, 7, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 13 */
    {0, 7, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 14 */
    {0, 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 15 */
    {0, 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 16 */
    {0, 7, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 17 CIRCLE 3 */
    {0, 7, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 18 */
    {0, 7, 6, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 19 */
    {0, 7, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 20 */
    {0, 7, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 21 */
    {0, 7, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 22 */
    {0, 7, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 23 */
    {0, 7, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 24 */
    {0, 7, 7, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 25 CIRCLE 4 */
    {0, 7, 7, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 26 */
    {0, 7, 7, 6, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 27 */
    {0, 7, 7, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 28 */
    {0, 7, 7, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 29 */
    {0, 7, 7, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 30 */
    {0, 7, 7, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 31 */
    {0, 7, 7, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 32 */
    {0, 7, 7, 6, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 33 CIRCLE 5 */
    {0, 7, 7, 6, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 34 */
    {0, 7, 7, 6, 6, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 35 */
    {0, 7, 7, 6, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 36 */
    {0, 7, 7, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 37 */
    {0, 7, 7, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 38 */
    {0, 7, 7, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 39 */
    {0, 7, 7, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 40 */
    {0, 7, 7, 6, 6, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 41 CIRCLE 6 */
    {0, 7, 7, 6, 6, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 42 */
    {0, 7, 7, 6, 6, 6, 3, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 43 */
    {0, 7, 7, 6, 6, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 44 */
    {0, 7, 7, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 45 */
    {0, 7, 7, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 46 */
    {0, 7, 7, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 47 */
    {0, 7, 7, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0},       /* level 48 */
    {0, 7, 7, 7, 6, 6, 6, 1, 0, 0, 0, 0, 0, 0, 0},       /* level 49 CIRCLE 7 */
    {0, 7, 7, 7, 6, 6, 6, 2, 0, 0, 0, 0, 0, 0, 0},       /* level 50 */
    {0, 7, 7, 7, 6, 6, 6, 3, 0, 0, 0, 0, 0, 0, 0},       /* level 51 */
    {0, 7, 7, 7, 6, 6, 6, 4, 0, 0, 0, 0, 0, 0, 0},       /* level 52 */
    {0, 7, 7, 7, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0},       /* level 53 */
    {0, 7, 7, 7, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0},       /* level 54 */
    {0, 7, 7, 7, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0},       /* level 55 */
    {0, 7, 7, 7, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0},       /* level 56 */
    {0, 7, 7, 7, 6, 6, 6, 6, 1, 0, 0, 0, 0, 0, 0},       /* level 57 CIRCLE 8 */
    {0, 7, 7, 7, 6, 6, 6, 6, 2, 0, 0, 0, 0, 0, 0},       /* level 58 */
    {0, 7, 7, 7, 6, 6, 6, 6, 3, 0, 0, 0, 0, 0, 0},       /* level 59 */
    {0, 7, 7, 7, 6, 6, 6, 6, 4, 0, 0, 0, 0, 0, 0},       /* level 60 */
    {0, 7, 7, 7, 6, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0},       /* level 61 */
    {0, 7, 7, 7, 6, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0},       /* level 62 */
    {0, 7, 7, 7, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0},       /* level 63 */
    {0, 7, 7, 7, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0},       /* level 64 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 1, 0, 0, 0, 0, 0},       /* level 65 CIRCLE 9 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 2, 0, 0, 0, 0, 0},       /* level 66 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 3, 0, 0, 0, 0, 0},       /* level 67 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 4, 0, 0, 0, 0, 0},       /* level 68 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 5, 0, 0, 0, 0, 0},       /* level 69 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 5, 0, 0, 0, 0, 0},       /* level 70 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 5, 0, 0, 0, 0, 0},       /* level 71 */
    {0, 7, 7, 7, 7, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0},       /* level 72 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 1, 0, 0, 0, 0},       /* level 73 CIRCLE 10 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 2, 0, 0, 0, 0},       /* level 74 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 3, 0, 0, 0, 0},       /* level 75 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0, 0},       /* level 76 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 5, 0, 0, 0, 0},       /* level 77 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 5, 0, 0, 0, 0},       /* level 78 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 5, 0, 0, 0, 0},       /* level 79 */
    {0, 8, 7, 7, 7, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0},       /* level 80 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 1, 0, 0, 0},       /* level 81 CIRCLE 11 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 2, 0, 0, 0},       /* level 82 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 3, 0, 0, 0},       /* level 83 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0},       /* level 84 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0},       /* level 85 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0},       /* level 86 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0},       /* level 87 */
    {0, 8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 4, 0, 0, 0},       /* level 88 */
    {0, 8, 8, 7, 7, 7, 7, 6, 6, 6, 6, 5, 1, 0, 0},       /* level 89 CIRCLE 12 */
    {0, 8, 8, 8, 7, 7, 7, 6, 6, 6, 6, 5, 2, 0, 0},       /* level 90 */
    {0, 8, 8, 8, 7, 7, 7, 6, 6, 6, 6, 5, 3, 0, 0},       /* level 91 */
    {0, 8, 8, 8, 8, 7, 7, 7, 6, 6, 6, 5, 4, 0, 0},       /* level 92 */
    {0, 8, 8, 8, 8, 7, 7, 7, 6, 6, 6, 5, 4, 0, 0},       /* level 93 */
    {0, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 5, 4, 0, 0},       /* level 94 */
    {0, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 5, 4, 0, 0},       /* level 95 */
    {0, 8, 8, 8, 8, 8, 8, 7, 7, 6, 6, 5, 4, 0, 0},       /* level 96 */
    {0, 9, 9, 8, 8, 8, 8, 8, 7, 7, 6, 5, 4, 1, 0},       /* level 97 CIRCLE 13 */
    {0, 9, 9, 9, 9, 9, 9, 8, 7, 7, 6, 5, 4, 2, 0},       /* level 98 */
    {0, 10, 10, 10, 10, 10, 10, 9, 8, 7, 6, 5, 4, 3, 0}, /* level 99 */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0},
    /* level 100 */                                          /* Immortal+ */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0}, /* level 101 */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0}, /* level 102 */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0}, /* level 103 */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0}, /* level 104 */
    {0, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 8, 7, 6, 0}  /* level 105 CIRCLE 14 */
}; /* 11+11+11+11+11+11+11+11+11+11+8+7+6 = 131 = max_char_spells  */

// Locals
CharData *memming = 0; /* head of memming characters linked list */

int mob_mem_time(CharData *ch, int circle) {
    double mem_time;

    mem_time = 9 - (GET_LEVEL(ch) - circle_to_level(circle)) / 2;

    if (mem_time < 2)
        mem_time = 2;

    /* Now adjust it for meditation. */
    if (GET_SKILL(ch, SKILL_MEDITATE))
        mem_time *= 0.3 + 0.007 * (100 - GET_SKILL(ch, SKILL_MEDITATE));

    return MAX(mem_time, 1);
}

EVENTFUNC(memming_event) {
    CharData *ch = (CharData *)event_obj;
    MemorizedList *cur = 0;
    char buf[256];
    int i;

    if (FIGHTING(ch)) {
        char_printf(ch,
                    "Your studies are rudely interrupted!\n"
                    "You abort your studies.\n");
        rem_memming(ch);
        return EVENT_FINISHED;
    }

    if (IS_DRUNK(ch)) {
        char_printf(ch,
                    "You cannot study while intoxicated.\n"
                    "You abort your studies.\n");
        rem_memming(ch);
        return EVENT_FINISHED;
    }

    if (GET_POS(ch) != POS_SITTING || GET_STANCE(ch) < STANCE_RESTING || GET_STANCE(ch) > STANCE_ALERT) {
        send_to_char("You abort your studies.\n", ch);
        rem_memming(ch);
        return EVENT_FINISHED;
    }

    if (EVENT_FLAGGED(ch, EVENT_SCRIBE)) {
        send_to_char("You can't memorize and scribe at the same time!\n", ch);
        rem_memming(ch);
        return EVENT_FINISHED;
    }

    /* Mobs memorize spells differently than players */
    if (IS_NPC(ch)) {
        for (i = NUM_SPELL_CIRCLES; i > 0; --i)
            if (GET_MOB_SPLBANK(ch, i) < spells_of_circle[(int)GET_LEVEL(ch)][i]) {
                if (GET_MOB_SPLMEM_TIME(ch) >= 0)
                    GET_MOB_SPLMEM_TIME(ch) -= 2;
                else
                    GET_MOB_SPLMEM_TIME(ch) = mob_mem_time(ch, i) - 2;
                if (GET_MOB_SPLMEM_TIME(ch) <= 0)
                    GET_MOB_SPLBANK(ch, i)++;
                return MEM_INTERVAL;
            }
        rem_memming(ch);
        return EVENT_FINISHED;
    }

    cur = GET_SPELL_MEM(ch).list_head;
    while (cur && cur->can_cast)
        cur = cur->next;
    if (!cur) {
        done_memming(ch);
        return EVENT_FINISHED;
    } else {
        if (MEM_MODE(ch) == MEMORIZE) {
            if (!find_spellbook_with_spell(ch, cur->spell)) {
                send_to_char("You need a spellbook with that spell written in it.\n", ch);
                rem_memming(ch);
                return EVENT_FINISHED;
            }
        }

        /* using pulse violence, which means we only update every _2_ seconds
           please do NOT change this, unless you change in comm.c to
           pulse_violence/2  */
        cur->mem_time -= 1;

        /* check meditate skill */
        if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
            if (number(0, 20) > 17)
                improve_skill(ch, SKILL_MEDITATE);
        }

        if (cur->mem_time < 1) {
            if (MEM_MODE(ch) == MEMORIZE)
                sprintf(buf, "You have finished memorizing %s.\n", skill_name(cur->spell));
            else if (MEM_MODE(ch) == PRAY)
                sprintf(buf, "You have finished praying for %s.\n", skill_name(cur->spell));
            send_to_char(buf, ch);

            /* reset so the guy has to remem after casting it. */
            cur->mem_time = set_mem_time(ch, cur->spell);
            cur->can_cast = true;
            GET_SPELL_MEM(ch).num_memmed++;
            if (GET_SPELL_MEM(ch).num_memmed == GET_SPELL_MEM(ch).num_spells || !cur->next) {
                done_memming(ch);
                return EVENT_FINISHED;
            }
        }
    }
    return MEM_INTERVAL;
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
        send_to_char("Your work is rudely interrupted!\n", ch);
        send_to_char("You stop scribing.\n", ch);
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    if (IS_DRUNK(ch)) {
        send_to_char("Sober up first, lush!\n", ch);
        send_to_char("You stop scribing.\n", ch);
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    if (GET_POS(ch) != POS_SITTING || GET_STANCE(ch) < STANCE_RESTING || GET_STANCE(ch) > STANCE_ALERT) {
        send_to_char("You stop scribing.\n", ch);
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    if (GET_SPELL_MEM(ch).mem_status) {
        send_to_char("You can't memorize and scribe at the same time!\n", ch);
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
        send_to_char("You need something to write with.\n", ch);
        send_to_char("You stop scribing.\n", ch);
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    if (right_type != ITEM_SPELLBOOK) {
        if (left_type != ITEM_SPELLBOOK) {
            send_to_char("You are no longer holding a spellbook.\n", ch);
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
        send_to_char("It is too dark, so you stop scribing.\n", ch);
        act("Unable to see, $n gives up scribing.", true, ch, 0, 0, TO_ROOM);
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    cur = ch->scribe_list;
    cur->scribe_time += 1;

    /* Did your teacher go away?  Lose your source book somehow? */

    if (!find_spellbook_with_spell(ch, cur->spell) && !find_teacher_for_spell(ch, cur->spell)) {
        sprintf(buf, "You've lost your source for %s!\n", skill_name(cur->spell));
        send_to_char(buf, ch);
        send_to_char("&3With a weary sigh, you stop scribing.&0\n", ch);
        act("$n sighs and stops scribing.", true, ch, 0, 0, TO_ROOM);
        clear_scribing(ch);
        return EVENT_FINISHED;
    }

    /* There is a chance to improve the scribe skill in each round of scribing. */

    if (number(1, 20) > 15) {
        improve_skill(ch, SKILL_SCRIBE);
    }

    if (cur->scribe_time >= PAGE_SCRIBE_TIME) {

        /* A page has been finished. */

        cur->scribe_time = 0;
        cur->pages_left -= 1;

        if (cur->pages_left > 0) {
            if (PAGE_SCRIBE_TIME > 1) {
                sprintf(buf, "You finish a page in your spellbook.\n");
                send_to_char(buf, ch);
            }
            return SCRIBE_INTERVAL;
        }

        /* A spell has been finished. */

        /* Add this spell to the spellbook. */
        next_scribing = cur->next;
        add_spell_to_book(ch, obj, cur->spell);

        /* Any more spells in this person's scribe list? */

        if (!next_scribing) {
            sprintf(buf, "&6You have finished scribing %s.  &3You are done scribing.&0\n", skill_name(cur->spell));
            send_to_char(buf, ch);
            clear_scribing(ch);
            return EVENT_FINISHED;
        } else {
            sprintf(buf, "&6You have finished scribing %s&0.\n", skill_name(cur->spell));
            send_to_char(buf, ch);
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

        send_to_char("&3You are done scribing.&0\n", ch);
        clear_scribing(ch);
        return EVENT_FINISHED;
    }
    return SCRIBE_INTERVAL;
}

/* do_memorize can be called with or without arguments, causing a spell
to be added to the list, or just display the current list, respectively. */
ACMD(do_memorize) {
    int spell, circle;
    CharData *tch;

    if (!ch || IS_NPC(ch))
        return;

    argument = delimited_arg_all(argument, arg, '\'');

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        if (!*arg)
            char_printf(ch, "You don't need to memorize spells to cast them.\n");
        else if (!(tch = find_char_around_char(ch, find_vis_by_name(ch, arg))))
            char_printf(ch,  NOPERSON);
        else if (MEM_MODE(tch) != MEMORIZE)
            char_printf(ch, "%s does not study sorcery.\n", GET_NAME(tch));
        else
            show_spell_list(ch, tch);
        return;
    }

    if (MEM_MODE(ch) != MEMORIZE) {
        send_to_char("You do not study sorcery.\n", ch);
        return;
    }

    /* if no arg, show the current spell list */
    if (!*arg) {
        show_spell_list(ch, ch);
        if (GET_SPELL_MEM(ch).num_spells - GET_SPELL_MEM(ch).num_memmed > 0 && GET_POS(ch) == POS_SITTING &&
            (GET_STANCE(ch) == STANCE_RESTING || GET_STANCE(ch) == STANCE_ALERT)) {
            send_to_char("\nYou continue your studies.\n", ch);
            start_memming(ch);
        }
        return;
    }

    if (FIGHTING(ch)) {
        send_to_char("If you want to commit suicide just say so!\n", ch);
        return;
    }

    /* check the char's position */
    if (GET_POS(ch) != POS_SITTING || GET_STANCE(ch) < STANCE_RESTING || GET_STANCE(ch) > STANCE_ALERT) {
        send_to_char("You are not comfortable enough to study.\n", ch);
        return;
    }

    /* for the spell name, find the spell num, and add it to the mem list. */
    spell = find_spell_num(arg);

    if (!IS_SPELL(spell)) {
        send_to_char("Memorize what?!\n", ch);
        return;
    }

    if (GET_LEVEL(ch) < SKILL_LEVEL(ch, spell)) {
        send_to_char("That spell is beyond your knowledge.\n", ch);
        return;
    }

    if (GET_SKILL(ch, spell) == 0) {
        send_to_char("You don't know that spell.\n", ch);
        return;
    }

    circle = SPELL_CIRCLE(ch, spell);

    /* check number of spells already memmed against the spell_table */
    if (spells_of_circle[(int)GET_LEVEL(ch)][circle] <= GET_SPELL_MEM(ch).num_circle[circle]) {
        sprintf(buf, "You can memorize no more spells from Circle %d.\n", circle);
        send_to_char(buf, ch);
        return;
    }

    /* check for a spellbook */
    if (!find_spellbook_with_spell(ch, spell)) {
        send_to_char("You need a spellbook with that spell written in it.\n", ch);
        return;
    }

    /* finally, the spell is available...add it to the list */
    add_spell(ch, spell, false, 0, true);
    if (!MEMMING(ch))
        act("$n takes out $s books and begins to study.", true, ch, 0, 0, TO_ROOM);
    start_memming(ch);
}

ACMD(do_pray) {
    int spell, circle;
    CharData *tch;

    if (!ch || IS_NPC(ch))
        return;

    argument = delimited_arg_all(argument, arg, '\'');

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        if (!*arg)
            char_printf(ch, "You don't need to pray for spells to cast them.\n");
        else if (!(tch = find_char_around_char(ch, find_vis_by_name(ch, arg))))
            char_printf(ch,  NOPERSON);
        else if (MEM_MODE(tch) != PRAY)
            char_printf(ch, "{} does not pray for spells.\n", GET_NAME(tch));
        else
            show_spell_list(ch, tch);
        return;
    }

    if (MEM_MODE(ch) != PRAY) {
        do_action(ch, argument, cmd, subcmd);
        return;
    }

    /* if no arg, show the current spell list */
    if (!*arg) {
        show_spell_list(ch, ch);
        if (GET_SPELL_MEM(ch).num_spells - GET_SPELL_MEM(ch).num_memmed > 0 && GET_POS(ch) == POS_SITTING &&
            (GET_STANCE(ch) == STANCE_RESTING || GET_STANCE(ch) == STANCE_ALERT)) {
            send_to_char("\nYou continue to pray.\n", ch);
            start_memming(ch);
        }
        return;
    }

    /* check the char's position */
    if (GET_POS(ch) != POS_SITTING || GET_STANCE(ch) < STANCE_RESTING || GET_STANCE(ch) > STANCE_ALERT) {
        send_to_char("You are not comfortable enough to pray to your deity.\n", ch);
        return;
    }

    /* for the spell name, find the spell num, and add it to the mem list. */
    spell = find_spell_num(arg);
    if (!IS_SPELL(spell)) {
        send_to_char("Pray for What?!\n", ch);
        return;
    }

    if (GET_LEVEL(ch) < SKILL_LEVEL(ch, spell)) {
        send_to_char("That spell is beyond your knowledge.\n", ch);
        return;
    }

    if (GET_SKILL(ch, spell) == 0) {
        send_to_char("You have heard of that spell, but have no idea how to cast it.\n", ch);
        return;
    }

    circle = SPELL_CIRCLE(ch, spell);

    /* check number of spells already memmed against the spell_table */
    if (spells_of_circle[(int)GET_LEVEL(ch)][circle] <= GET_SPELL_MEM(ch).num_circle[circle]) {
        sprintf(buf, "You can pray for no more spells from Circle %d.\n", circle);
        send_to_char(buf, ch);
        return;
    }

    add_spell(ch, spell, false, 0, true);
    if (!MEMMING(ch))
        act("$n begins praying to $s deity.", true, ch, 0, 0, TO_ROOM);
    start_memming(ch);
}

void wipe_mem(CharData *ch) {
    int i;
    MemorizedList *cur, *next;

    if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
        act("$n ceases $s meditative trance.", true, ch, 0, 0, TO_ROOM);
        send_to_char("&8You stop meditating.\n&0", ch);
        REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
    }

    if (GET_SPELL_MEM(ch).mem_status) {
        send_to_char("You abort your studies.\n", ch);
        rem_memming(ch);
    }

    GET_SPELL_MEM(ch).num_spells = 0;
    GET_SPELL_MEM(ch).num_memmed = 0;

    cur = GET_SPELL_MEM(ch).list_head;
    GET_SPELL_MEM(ch).list_head = nullptr;
    GET_SPELL_MEM(ch).list_tail = nullptr;

    while (cur) {
        next = cur->next;
        free(cur);
        cur = next;
    }

    for (i = 1; i <= NUM_SPELL_CIRCLES; ++i)
        GET_SPELL_MEM(ch).num_circle[i] = 0;

    save_player_char(ch);
    send_to_char("You purge all spells from your mind.\n", ch);
}

ACMD(do_forget) {
    int spell;
    char buf[128];

    if (!ch || IS_NPC(ch) || GET_LEVEL(ch) >= LVL_IMMORT) {
        send_to_char("You have no need to forget spells.\n", ch);
        return;
    }

    argument = delimited_arg_all(argument, arg, '\'');

    if (!*arg) {
        send_to_char("Are you trying to forget something in particular?\n", ch);
        return;
    }

    if (!strcasecmp(arg, "all")) {
        wipe_mem(ch);
        return;
    }

    spell = find_spell_num(arg);

    if (!IS_SPELL(spell)) {
        send_to_char("Forget What?!\n", ch);
        return;
    } else {
        if (rem_spell(ch, spell)) {
            sprintf(buf, "You purge %s from your memory.\n", skill_name(spell));
            send_to_char(buf, ch);
        } else {
            sprintf(buf, "You do not have that spell memorized!\n");
            send_to_char(buf, ch);
        }
    }
}

/* set the meditate flag */
ACMD(do_meditate) {
    if (IS_NPC(ch)) {
        send_to_char("You don't need to meditate!\n", ch);
        return;
    }
    if (GET_SKILL(ch, SKILL_MEDITATE) == 0) {
        send_to_char("You just can't seem to focus your mind enough.\n", ch);
        return;
    }
    if (GET_POS(ch) != POS_SITTING || GET_STANCE(ch) < STANCE_RESTING || GET_STANCE(ch) > STANCE_ALERT) {
        send_to_char("Try resting first....\n", ch);
        return;
    }
    if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
        send_to_char("You're already meditating!\n", ch);
        return;
    }

    if (GET_CLASS(ch) == CLASS_BERSERKER) {
        act("$n closes $s eyes and begins meditating.\n", true, ch, 0, 0, TO_ROOM);
        send_to_char("You begin to meditate, letting your rage build...\n", ch);
        check_regen_rates(ch);
    } else {
        act("$n begins meditating to improve $s concentration.", true, ch, 0, 0, TO_ROOM);
        send_to_char("You begin to meditate.\n", ch);
    }

    SET_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
    WAIT_STATE(ch, PULSE_VIOLENCE * 2); /* stun time */
    improve_skill(ch, SKILL_MEDITATE);
}

/* add a spell to the char's mem_list */
int add_spell(CharData *ch, int spell, int can_cast, int mem_time, bool verbose) {
    MemorizedList *cur;
    char buf[128];
    int circle = SPELL_CIRCLE(ch, spell);

    /* see if ch can even use that circle of spell.... */
    if (!spells_of_circle[(int)GET_LEVEL(ch)][circle]) {
        if (verbose) {
            sprintf(buf, "You can't use spells from Circle %d yet.\n", circle);
            send_to_char(buf, ch);
        }
        return 0;
    }

    /* initialize the ptr and check it before proceeding */
    if (GET_SPELL_MEM(ch).num_spells == 0 || !GET_SPELL_MEM(ch).list_tail || !GET_SPELL_MEM(ch).list_head) {
        CREATE(cur, MemorizedList, 1);
        cur->next = nullptr;
        GET_SPELL_MEM(ch).list_head = cur;
        GET_SPELL_MEM(ch).list_tail = cur;
    } else {
        CREATE(cur, MemorizedList, 1);
        cur->next = nullptr;
        GET_SPELL_MEM(ch).list_tail->next = cur;
        GET_SPELL_MEM(ch).list_tail = cur;
    }

    cur->spell = spell;
    cur->can_cast = can_cast;
    cur->mem_time = (mem_time <= 0 ? set_mem_time(ch, spell) : mem_time);

    GET_SPELL_MEM(ch).num_circle[circle]++;
    GET_SPELL_MEM(ch).num_spells++;
    if (can_cast)
        GET_SPELL_MEM(ch).num_memmed++;

    if (verbose) {
        sprintf(buf, "You begin %s %s, which will take %d seconds.\n",
                MEM_MODE(ch) == MEMORIZE ? "memorizing" : "praying for", skill_name(spell), cur->mem_time);
        send_to_char(buf, ch);
    }

    return 1;
}

/* remove the first instance of a spell from the char's memorize list */
int rem_spell(CharData *ch, int spell) {
    MemorizedList dummy, *cur, *temp = nullptr;
    int found = 0;

    dummy.next = GET_SPELL_MEM(ch).list_head;

    for (cur = &dummy; cur->next; cur = cur->next) {
        if (cur->next->spell != spell)
            continue;
        temp = cur->next;
        cur->next = temp->next;
        GET_SPELL_MEM(ch).num_spells--;
        if (temp->can_cast)
            GET_SPELL_MEM(ch).num_memmed--;
        free(temp);
        GET_SPELL_MEM(ch).num_circle[SPELL_CIRCLE(ch, spell)]--;
        found = 1;
        break;
    }

    if (dummy.next != GET_SPELL_MEM(ch).list_head)
        GET_SPELL_MEM(ch).list_head = dummy.next;

    if (!GET_SPELL_MEM(ch).list_head)
        GET_SPELL_MEM(ch).list_tail = nullptr;

    if (temp == GET_SPELL_MEM(ch).list_tail)
        GET_SPELL_MEM(ch).list_tail = cur;

    return found;
}

void free_mem_list(CharData *ch) {
    MemorizedList *next, *mem = GET_SPELL_MEM(ch).list_head;
    while (mem) {
        next = mem->next;
        free(mem);
        mem = next;
    }
    GET_SPELL_MEM(ch).list_head = nullptr;
    GET_SPELL_MEM(ch).list_tail = nullptr;
}

void free_scribe_list(CharData *ch) {
    Scribing *next;
    while (ch->scribe_list) {
        next = ch->scribe_list->next;
        free(ch->scribe_list);
        ch->scribe_list = next;
    }
}

void show_memorized_slots(CharData *ch, CharData *tch)
#define _MEM_PER_CIRCLE 10
{
    struct {
        struct {
            int spellnum;
            int memorized;
        } memorized[_MEM_PER_CIRCLE];
        int num_memorized;
    } circles[NUM_SPELL_CIRCLES];
    int circle, pos, memming = false, time_remaining, show_next;
    MemorizedList *mem;
    std::string resp;

    memset(circles, 0x0, sizeof(circles));

    for (mem = GET_SPELL_MEM(tch).list_head; mem; mem = mem->next) {
        if (!mem->can_cast) {
            memming = true;
            continue;
        }
        /* Spell circles are 1-based whereas our array is 0-based */
        circle = SPELL_CIRCLE(tch, mem->spell) - 1;
        for (pos = 0; pos < circles[circle].num_memorized; ++pos)
            if (circles[circle].memorized[pos].spellnum == mem->spell)
                break;
        if (pos >= _MEM_PER_CIRCLE) {
            continue;
        }
        if (pos == circles[circle].num_memorized)
            ++circles[circle].num_memorized;
        circles[circle].memorized[pos].spellnum = mem->spell;
        ++circles[circle].memorized[pos].memorized;
    }

    sprintf(buf2, "%s %s %s:", ch == tch ? "You" : GET_NAME(tch), ch == tch ? "have" : "has",
            MEM_MODE(tch) == MEMORIZE ? "memorized" : "prayed for");
    if (memming)
        resp += fmt::format("{:<49}{} {} currently {}:\n\n", buf2, ch == tch ? "You" : GET_NAME(tch),
                            ch == tch ? "are" : "is", MEM_MODE(tch) == MEMORIZE ? "memorizing" : "praying for");
    else
        resp += fmt::format("{}\n\n", buf2);

    if (memming) {
        mem = GET_SPELL_MEM(tch).list_head;
        while (mem && mem->can_cast)
            mem = mem->next;
    }

    time_remaining = 0;
    show_next = 0;
    for (circle = 0; circle < NUM_SPELL_CIRCLES; ++circle) {
        for (pos = 0; pos < circles[circle].num_memorized; ++pos) {
            if (pos == 0)
                resp += fmt::format(AHBLU "Circle {:2}" ANRM "  ", circle + 1);
            else
                resp += fmt::format(AHBLU "{:9}" ANRM "  ", "");

            resp += fmt::format("{:2} - {:<31}{}", circles[circle].memorized[pos].memorized,
                                skill_name(circles[circle].memorized[pos].spellnum),
                                mem || time_remaining || show_next ? "" : "\n");

            if (mem) {
                resp += fmt::format("{:3} sec: (" AHBLU "{}" ANRM ") {}\n", mem->mem_time, CIRCLE_ABBR(tch, mem->spell),
                                    skill_name(mem->spell));
                time_remaining += mem->mem_time;
                do
                    mem = mem->next;
                while (mem && mem->can_cast);
            } else if (time_remaining) {
                resp += "\n";
                show_next = time_remaining;
                time_remaining = 0;
            } else if (show_next) {
                resp += fmt::format("{:3} second{} remaining...\n", show_next, show_next == 1 ? "" : "s");
                show_next = 0;
            }
        }
    }

    pos = 0;
    while (mem) {
        resp += fmt::format("{:<37}{:3} sec: (" AHBLU "{}" ANRM ") {}\n",
                            !pos++ && GET_SPELL_MEM(tch).num_memmed < 1 ? "   None!" : "", mem->mem_time,
                            CIRCLE_ABBR(tch, mem->spell), skill_name(mem->spell));
        time_remaining += mem->mem_time;
        do
            mem = mem->next;
        while (mem && mem->can_cast);
    }

    if (show_next)
        time_remaining = show_next;
    if (time_remaining)
        resp += fmt::format("{}{:<37}{:3} second{} remaining...\n", show_next ? "" : "\n", "", time_remaining,
                            time_remaining == 1 ? "" : "s");
    else if (GET_SPELL_MEM(tch).num_memmed < 1)
        resp += "   None!\n";

    resp += "\n";

    send_to_char(resp.c_str(), ch);
}

#undef _MEM_PER_CIRCLE

void show_available_slots(CharData *ch, CharData *tch) {
    std::string resp;
    int circle, avail, found;

    /* Display available spell slots for each circle */
    resp += ch == tch ? "You" : GET_NAME(tch);
    if (MEM_MODE(tch) == MEMORIZE)
        resp += " can memorize";
    else if (MEM_MODE(tch) == PRAY)
        resp += " can pray for";

    for (circle = 1, found = 0; circle <= NUM_SPELL_CIRCLES; ++circle) {
        avail = spells_of_circle[(int)GET_LEVEL(ch)][circle] - GET_SPELL_MEM(ch).num_circle[circle];
        if (avail > 0) {
            resp += fmt::format(" ({}){}{}", avail, circle,
                                circle == 1   ? "st"
                                : circle == 2 ? "nd"
                                : circle == 3 ? "rd"
                                              : "th");
            found += avail;
        }
    }

    if (found)
        resp += " circle";
    else
        resp += " no more";
    resp += fmt::format(" spell{}\n", avail == 1 ? "" : "s");

    send_to_char(resp.c_str(), ch);
}

void show_spell_list(CharData *ch, CharData *tch) {
    show_memorized_slots(ch, tch);
    show_available_slots(ch, tch);
}

int check_spell_memory(CharData *ch, int spellnum) {
    MemorizedList *cur;

    cur = GET_SPELL_MEM(ch).list_head;

    /* traverse the list and find out if the spell is memmed or not. */
    while (cur) {
        if (cur->spell == spellnum && cur->can_cast)
            return true;

        cur = cur->next;
    }

    /* couldn't find a memmed copy of that spell anywhere... */
    return 0;
}

void charge_mem(CharData *ch, int spellnum) {
    MemorizedList *cur;

    /*
     * Mobs don't memorize specific spells; they only recharge
     * slots in circles in their spell bank.
     */
    if (IS_NPC(ch)) {
        if (GET_MOB_SPLBANK(ch, SPELL_CIRCLE(ch, spellnum)) > 0)
            GET_MOB_SPLBANK(ch, SPELL_CIRCLE(ch, spellnum))--;
        return;
    }

    cur = GET_SPELL_MEM(ch).list_head;
    /* traverse the list to find the spell. */
    while (cur) {
        if (cur->spell == spellnum && cur->can_cast) {
            /* okay, this is a valid copy of the spell */
            cur->can_cast = 0;
            GET_SPELL_MEM(ch).num_memmed--;
            return;
        }
        cur = cur->next;
    }
}

void start_memming(CharData *ch) {
    if (!MEMMING(ch)) {
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_MEM);
        event_create(EVENT_MEM, memming_event, ch, false, &(ch->events), MEM_INTERVAL);
    }
}

void done_memming(CharData *ch) {
    if (MEM_MODE(ch) == MEMORIZE) {
        send_to_char("You have completed your studies.\n", ch);
        act("$n closes $s book and smiles.", true, ch, 0, 0, TO_ROOM);
    } else if (MEM_MODE(ch) == PRAY) {
        send_to_char("Your prayers are complete.\n", ch);
        act("$n finishes praying to $s deity.", true, ch, 0, 0, TO_ROOM);
    }
    rem_memming(ch);
}

void rem_memming(CharData *ch) {
    REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_MEM);
    cancel_event(GET_EVENTS(ch), EVENT_MEM);

    if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
        act("$n ceases $s meditative trance.", true, ch, 0, 0, TO_ROOM);
        send_to_char("&8You stop meditating.\n&0", ch);
        REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
    }
}

int spell_mem_time(CharData *ch, int spell) {
    double mem_time;

    mem_time = 9 - ((int)GET_LEVEL(ch) - skills[spell].min_level[(int)GET_CLASS(ch)]) / 2;

    if (mem_time < 2)
        mem_time = 2;

    /* Now adjust it for meditation. */
    if (PLR_FLAGGED(ch, PLR_MEDITATE))
        mem_time *= 0.3 + 0.007 * (100 - GET_SKILL(ch, SKILL_MEDITATE));

    return MAX((int)mem_time, 1);
}

int set_mem_time(CharData *ch, int spell) {
    int mem_time;

    mem_time = spell_mem_time(ch, spell);

    if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
        improve_skill(ch, SKILL_MEDITATE);

        /* There's a 1-5% chance it will be a deep trance and take only 1 second. */
        if (number(1, 100) <= 1 + GET_SKILL(ch, SKILL_MEDITATE) / 25) {
            send_to_char("You go into a deep trance...\n", ch);
            act("$n falls into a deep trance...", true, ch, 0, 0, TO_ROOM);
            mem_time = 1;
        }
    }

    return MAX((int)mem_time, 1);
}

/* Okay, we can finally save the char's spell list on disk. */
void init_mem_list(CharData *ch) {
    int remove, circle;
    MemorizedList *mem, *next_mem;

    if (!ch || IS_NPC(ch))
        return;

    for (mem = GET_SPELL_MEM(ch).list_head; mem; mem = next_mem) {
        next_mem = mem->next;
        remove = false;
        circle = SPELL_CIRCLE(ch, mem->spell);
        if (GET_LEVEL(ch) < skills[mem->spell].min_level[(int)GET_CLASS(ch)])
            remove = true;
        else if (spells_of_circle[(int)GET_LEVEL(ch)][circle] < GET_SPELL_MEM(ch).num_circle[circle])
            remove = true;
        if (remove)
            rem_spell(ch, mem->spell);
    }

} /* end init_mem_list() */

/********************/
/**** SPELLBOOKS ****/
/********************/

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
            send_to_char(obuf, ch);
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
        send_to_char(obuf, ch);
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
        send_to_char("That spell is already in this book.\n", ch);
        return false;
    }

    pages = get_spell_pages(ch, spell);

    if (!room_in_book(obj, pages)) {
        send_to_char("Your spellbook is too full for that spell.\n", ch);
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
        send_to_char("If you wanna commit suicide just say so!\n", ch);
        return;
    }

    /* Can't scribe in darkness. */

    if (!LIGHT_OK(ch)) {
        send_to_char("It is too dark for writing.\n", ch);
        return;
    }

    /* Can't scribe while confused. */

    if (CONFUSED(ch)) {
        send_to_char("You're too confused to write!\n", ch);
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
        send_to_char("You don't seem to have anything to write with.\n", ch);
        return;
    }

    /* Find the spellbook. */

    if (right_type == ITEM_SPELLBOOK) {
        book = held_right;
    } else if (left_type == ITEM_SPELLBOOK) {
        book = held_left;
    } else {
        send_to_char("You need to hold a spellbook before you can write in it.\n", ch);
        return;
    }

    /* Make sure player is in proper position. */

    if (GET_POS(ch) != POS_SITTING || GET_STANCE(ch) < STANCE_RESTING || GET_STANCE(ch) > STANCE_ALERT) {
        send_to_char("You have to be sitting to scribe.\n", ch);
        return;
    }

    /* Identify the spell to be scribed. */

    argument = delimited_arg_all(argument, arg, '\'');

    if (!*arg) {
        send_to_char("What spell do you want to scribe?\n", ch);
        return;
    }

    spellnum = find_spell_num(arg);

    if (!IS_SPELL(spellnum)) {
        send_to_char("Try all you want, but there's no such thing.\n", ch);
        return;
    }

    /* Make sure there's a source for the spell - another character, or a
     * spellbook. */

    sourcebook = find_spellbook_with_spell(ch, spellnum);
    teacher = find_teacher_for_spell(ch, spellnum);
    if (!sourcebook && !teacher) {
        send_to_char("There is nobody here to teach that spell, and nothing to copy it from.\n", ch);
        return;
    }

    /* Make sure it's a spell they are allowed to use, according to class and
     * level. */

    if ((int)GET_LEVEL(ch) < skills[spellnum].min_level[(int)GET_CLASS(ch)] || GET_SKILL(ch, spellnum) == 0) {
        send_to_char("You don't understand the magic used in that spell.\n", ch);
        return;
    }

    /* Don't allow a spell to be scribed into a book twice (waste of time/space).
     */

    if (book_contains_spell(book, spellnum)) {
        send_to_char("That spell is already written in this book.\n", ch);
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
                send_to_char("You're already scribing that spell!\n", ch);
            else
                send_to_char("You are already planning on scribing that spell.\n", ch);
            return;
        }

    /* Add the spell to the list of spells that this character is scribing. */

    add_spell_scribe(ch, spellnum);

    /* Make sure player is scribing, and provide feedback. */

    if (!EVENT_FLAGGED(ch, EVENT_SCRIBE)) {
        start_scribing(ch);
        act("$n picks up $s $o and starts writing in $P.", true, ch, pen, book, TO_ROOM);
        sprintf(buf, "You begin scribing %s.\n", skill_name(spellnum));
        send_to_char(buf, ch);
    } else {
        sprintf(buf, "You make a mental note to scribe %s.\n", skill_name(spellnum));
        send_to_char(buf, ch);
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

    sprintf(buf, "There is no source for %s nearby, so you skip it for now.\n", skill_name(scr->spell));
    send_to_char(buf, ch);
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
