/***************************************************************************
 *   File: utils.h                                        Part of FieryMUD *
 *  Usage: header file: utility macros and prototypes of utility funcs     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "money.hpp"
#include "rooms.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "text.hpp"

#include <math.h>

/* external declarations and prototypes **********************************/

// struct ZoneData *zone_table;
// struct IndexData *obj_index;

/* This should be in weather.h, but IS_DARK uses hemisphere data, so we
 * need it here. */
// struct HemisphereData hemispheres[NUM_HEMISPHERES];

/* public functions in utils.c */
void perform_random_gem_drop(CharData *);
bool event_target_valid(CharData *ch);
int con_aff(CharData *ch);
int static_ac(int dex);
int touch(const char *path);

int get_line(FILE *fl, char *buf);
TimeInfoData age(CharData *ch);
int num_pc_in_room(RoomData *room);
int load_modifier(CharData *ch);
const char *movewords(CharData *ch, int cmd, int room, int leaving);
void build_count(void);
int monk_weight_penalty(CharData *ch);
int find_zone(int num);
int parse_obj_name(CharData *ch, const char *arg, const char *objname, int numobjs, void *objects, int objsize);
void init_flagvectors();
long exp_next_level(int level, int class_num);
void init_exp_table(void);
CharData *is_playing(char *vict_name);

void sort(void algorithm(int[], int, int(int a, int b)), int array[], int count, int comparator(int, int));
void bubblesort(int array[], int count, int comparator(int a, int b));
void insertsort(int array[], int count, int comparator(int a, int b));
void quicksort(int array[], int count, int comparator(int a, int b));
void optquicksort(int array[], int count, int comparator(int a, int b));

void update_pos(CharData *victim);

int yesno_result(char *answer);
#define YESNO_NONE 0  /* just pressed enter (probably wanted default) */
#define YESNO_OTHER 1 /* typed something, but not starting with y or n */
#define YESNO_YES 2   /* entered something starting with y or Y */
#define YESNO_NO 3    /* entered something starting with n or N */

char *statelength(int inches);
char *stateweight(float pounds);
char *format_apply(int apply, int amount);
void drop_core(CharData *ch, const char *desc);

/* various constants *****************************************************/

/* generic multi-purpose flag constants */
#define FAIL_SILENTLY (1 << 0)

/* breadth-first searching */
#define BFS_ERROR -1
#define BFS_ALREADY_THERE -2
#define BFS_NO_PATH -3

/* mud-life time */
#define HOURS_PER_DAY 24
#define DAYS_PER_WEEK 7
#define WEEKS_PER_MONTH 5
#define DAYS_PER_MONTH (DAYS_PER_WEEK * WEEKS_PER_MONTH)
#define MONTHS_PER_YEAR 16
#define WEEKS_PER_YEAR (WEEKS_PER_MONTH * MONTHS_PER_YEAR)
#define DAYS_PER_YEAR (DAYS_PER_MONTH * DAYS_PER_MONTH)
#define SECS_PER_MUD_HOUR 75
#define SECS_PER_MUD_DAY (HOURS_PER_DAY * SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH (DAYS_PER_MONTH * SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR (MONTHS_PER_YEAR * SECS_PER_MUD_MONTH)

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN 60
#define SECS_PER_REAL_HOUR (60 * SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY (24 * SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR (365 * SECS_PER_REAL_DAY)

/* string utils **********************************************************/

#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

/* IS_UPPER and IS_LOWER added by dkoepke */
#define IS_UPPER(c) ((c) >= 'A' && (c) <= 'Z')
#define IS_LOWER(c) ((c) >= 'a' && (c) <= 'z')

#define LOWER(c) (IS_UPPER(c) ? ((c) + ('a' - 'A')) : (c))
#define UPPER(c) (IS_LOWER(c) ? ((c) + ('A' - 'a')) : (c))

#define IS_NEWLINE(ch) ((ch) == '\n')
#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")

/* memory utils **********************************************************/

#define CREATE(result, type, number)                                                                                   \
    do {                                                                                                               \
        if (!((result) = (type *)calloc((number), sizeof(type)))) {                                                    \
            perror("malloc failure");                                                                                  \
            abort();                                                                                                   \
        }                                                                                                              \
    } while (0)

#define RECREATE(result, type, number)                                                                                 \
    do {                                                                                                               \
        if (!((result) = (type *)realloc((result), sizeof(type) * (number)))) {                                        \
            perror("realloc failure");                                                                                 \
            abort();                                                                                                   \
        }                                                                                                              \
    } while (0)

/*
 * A macro to remove an item from a list: if it's the list head, change the
 * head, else traverse the list looking for the item before the one to be
 * removed. To use, just make sure that there is a variable 'temp' declared as
 * the same type as the list to be manipulated.
 */
#define REMOVE_FROM_LIST(item, head, next)                                                                             \
    do {                                                                                                               \
        if ((item) == (head))                                                                                          \
            head = (item)->next;                                                                                       \
        else {                                                                                                         \
            temp = head;                                                                                               \
            while (temp && (temp->next != (item)))                                                                     \
                temp = temp->next;                                                                                     \
            if (temp)                                                                                                  \
                temp->next = (item)->next;                                                                             \
        }                                                                                                              \
    } while (0)

#define LOOP_THRU_PEOPLE(IN_ROOM, PLAYER)                                                                              \
    for ((IN_ROOM) = world[(PLAYER)->in_room].people; (IN_ROOM) != nullptr; (IN_ROOM) = (IN_ROOM)->next_in_room)

/* basic bitvector utils *************************************************/

#define IS_SET(flag, bit) (((unsigned int)flag) & ((unsigned int)bit))
#define SET_BIT(var, bit) ((var) |= (bit))
#define REMOVE_BIT(var, bit) ((var) &= ~(bit))
#define TOGGLE_BIT(var, bit) ((var) = (var) ^ (bit))

/* extended bitvector utils */
#define FIELD(x) ((long)(x) / FLAGBLOCK_SIZE)
#define FLAG(x) ((flagvector)1 << ((x) % FLAGBLOCK_SIZE))
#define IS_FLAGGED(field, flag) (((field)[FIELD(flag)] & FLAG(flag)) ? 1 : 0)
#define SET_FLAG(field, flag) ((field)[FIELD(flag)] |= FLAG(flag))
#define REMOVE_FLAG(field, flag) ((field)[FIELD(flag)] &= ~FLAG(flag))
#define TOGGLE_FLAG(field, flag) ((field)[FIELD(flag)] ^= FLAG(flag))
bool ALL_FLAGGED(const flagvector field[], const flagvector flags[], const int num_flags);
bool ANY_FLAGGED(const flagvector field[], const flagvector flags[], const int num_flags);
void SET_FLAGS(flagvector field[], const flagvector flags[], const int num_flags);
void REMOVE_FLAGS(flagvector field[], const flagvector flags[], const int num_flags);
void TOGGLE_FLAGS(flagvector field[], const flagvector flags[], const int num_flags);
void COPY_FLAGS(flagvector field[], const flagvector flags[], const int num_flags);

extern flagvector *ALL_FLAGS;
#define HAS_FLAGS(field, num_flags) (ANY_FLAGGED((field), ALL_FLAGS, (num_flags)))
#define CLEAR_FLAGS(field, num_flags) (REMOVE_FLAGS((field), ALL_FLAGS, (num_flags)))

/* Event flags */
#define GET_EVENTS(o) ((o)->events)
#define GET_EVENT_FLAGS(o) ((o)->event_flags)
#define EVENT_FLAGGED(o, flag) IS_FLAGGED(GET_EVENT_FLAGS(o), (flag))

#define MOB_FLAGS(ch) ((ch)->char_specials.act)
#define PLR_FLAGS(ch) ((ch)->char_specials.act)
#define PRF_FLAGS(ch) ((ch)->player_specials->pref)
#define PRV_FLAGS(ch) ((ch)->player_specials->privileges)
#define EFF_FLAGS(ch) ((ch)->char_specials.effects)
#define ROOM_FLAGS(loc) (world[(loc)].room_flags)
#define ROOM_EFFECTS(loc) (world[(loc)].room_effects)

#define IS_NPC(ch) IS_FLAGGED(MOB_FLAGS(ch), MOB_ISNPC)
#define IS_MOB(ch) (IS_NPC(ch) && ((ch)->mob_specials.nr > -1))
#define POSSESSED(ch) ((ch)->desc && (ch)->desc->original)
#define POSSESSOR(ch) ((ch)->desc && (ch)->desc->original ? (ch)->desc->original : NULL)
#define REAL_CHAR(ch) ((ch)->desc && (ch)->desc->original ? (ch)->desc->original : (ch))
#define FORWARD(ch) ((ch)->forward ? (ch)->forward : (ch))
#define IS_PC(ch) (!IS_NPC(REAL_CHAR(ch)))
/* PLAYERALLY - true for a player, or a player's pet */
#define PLAYERALLY(ch) (IS_PC(ch) || (EFF_FLAGGED(ch, EFF_CHARM) && (ch)->master && IS_PC((ch)->master)))
#define IS_PET(ch) (PLAYERALLY(ch) && MOB_FLAGGED(ch, MOB_PET)) /* True if player bought/tamed mob only */

/* MORTALALLY - same as PLAYERALLY except that the involved player
 * must be mortal for it to return true */
#define MORTALALLY(ch)                                                                                                 \
    ((!IS_NPC(REAL_CHAR(ch)) && GET_LEVEL(REAL_CHAR(ch)) < LVL_IMMORT) ||                                              \
     (EFF_FLAGGED(ch, EFF_CHARM) && (ch)->master &&                                                                    \
      (!IS_NPC(REAL_CHAR(ch)) && GET_LEVEL(REAL_CHAR(ch)) < LVL_IMMORT)))

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_FLAGGED(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_FLAGGED(PLR_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag) IS_FLAGGED(PRF_FLAGS(REAL_CHAR(ch)), (flag))
#define PRV_FLAGGED(ch, flag) IS_FLAGGED(PRV_FLAGS(REAL_CHAR(ch)), (flag))
#define EFF_FLAGGED(ch, flag) IS_FLAGGED(EFF_FLAGS(ch), (flag))
#define ROOM_FLAGGED(loc, flag) IS_FLAGGED(ROOM_FLAGS(loc), (flag))
#define ROOM_EFF_FLAGGED(loc, eff) IS_FLAGGED(ROOM_EFFECTS(loc), (eff))

#define AGGR_TO_PLAYERS(ch)                                                                                            \
    (MOB_FLAGGED((ch), MOB_AGGR_EVIL) || MOB_FLAGGED((ch), MOB_AGGR_GOOD) || MOB_FLAGGED((ch), MOB_AGGR_NEUTRAL) ||    \
     MOB_FLAGGED((ch), MOB_AGGR_EVIL_RACE) || MOB_FLAGGED((ch), MOB_AGGR_GOOD_RACE))
#define PLR_TOG_CHK(ch, flag) (TOGGLE_FLAG(PLR_FLAGS(ch), (flag)) & FLAG(flag))
#define PRF_TOG_CHK(ch, flag) (TOGGLE_FLAG(PRF_FLAGS(ch), (flag)) & FLAG(flag))
#define PRV_TOG_CHK(ch, flag) (TOGGLE_FLAG(PRV_FLAGS(ch), (flag)) & FLAG(flag))
#define CONFUSED(ch) EFF_FLAGGED(ch, EFF_CONFUSION)

/* Mob performs scripts?  (specprocs and triggers)
 * - is an NPC
 * - doesn't have !script flag
 * - isn't charmed
 */
#define MOB_PERFORMS_SCRIPTS(ch) (IS_NPC(ch) && !MOB_FLAGGED(ch, MOB_NOSCRIPT) && !EFF_FLAGGED(ch, EFF_CHARM))

#define MEMMING(ch) EVENT_FLAGGED((ch), EVENT_MEM)

/* char utils ************************************************************/

/* Identifier accessors */
#define GET_PFILEPOS(ch) ((ch)->pfilepos)
#define GET_NAMELIST(ch) ((ch)->player.namelist)
#define GET_NAME(ch) ((ch)->player.short_descr)
#define GET_SHORT(ch) ((ch)->player.short_descr)
#define GET_TITLE(ch) ((ch)->player.title)
#define GET_WIZ_TITLE(ch) ((ch)->player_specials->wiz_title)
#define GET_PERM_TITLES(ch) ((ch)->player_specials->perm_titles)
#define GET_LDESC(ch) ((ch)->player.long_descr)
#define GET_DESCRIPTION(ch) ((ch)->player.description)

/* General accessors */
#define IN_ROOM(ch) ((ch)->in_room)
#define IN_ROOM_VNUM(ch) (IN_ROOM(ch) > NOWHERE && IN_ROOM(ch) < top_of_world ? world[IN_ROOM(ch)].vnum : NOWHERE)
#define IN_ZONE_RNUM(ch) (world[IN_ROOM(ch)].zone)
#define IN_ZONE_VNUM(ch) (zone_table[IN_ZONE_RNUM(ch)].number)
#define GET_WAS_IN(ch) ((ch)->was_in_room)
#define GET_LEVEL(ch) ((ch)->player.level)
#define GET_LIFEFORCE(ch) ((ch)->player.lifeforce)
#define GET_COMPOSITION(ch) ((ch)->player.composition)
#define GET_SIZE(ch) ((ch)->player.affected_size)
#define GET_AGE(ch) (age(ch).year)
#define GET_CLASS(ch) ((ch)->player.class_num)
#define GET_HEIGHT(ch) ((ch)->player.height)
#define GET_WEIGHT(ch) ((ch)->player.weight)
#define GET_SEX(ch) ((ch)->player.sex)
#define GET_EXP(ch) ((ch)->points.exp)
#define GET_MOVE(ch) ((ch)->points.move)
#define GET_MAX_MOVE(ch) ((ch)->points.max_move)
#define GET_MANA(ch) ((ch)->points.mana)
#define GET_MAX_MANA(ch) ((ch)->points.max_mana)
#define GET_COINS(ch) ((ch)->points.coins)
#define GET_PURSE_COINS(ch, coin) (GET_COINS(ch)[coin])
#define GET_PLATINUM(ch) (GET_COINS(ch)[PLAT])
#define GET_GOLD(ch) (GET_COINS(ch)[GOLD])
#define GET_SILVER(ch) (GET_COINS(ch)[SILVER])
#define GET_COPPER(ch) (GET_COINS(ch)[COPPER])
#define GET_BANK_COINS(ch) ((ch)->points.bank)
#define GET_ACCOUNT_COINS(ch, coin) (GET_BANK_COINS(ch)[coin])
#define GET_BANK_PLATINUM(ch) (GET_BANK_COINS(ch)[PLAT])
#define GET_BANK_GOLD(ch) (GET_BANK_COINS(ch)[GOLD])
#define GET_BANK_SILVER(ch) (GET_BANK_COINS(ch)[SILVER])
#define GET_BANK_COPPER(ch) (GET_BANK_COINS(ch)[COPPER])
#define GET_CASH(ch) (GET_PLATINUM(ch) * 1000 + GET_GOLD(ch) * 100 + GET_SILVER(ch) * 10 + GET_COPPER(ch))
#define GET_AC(ch) ((ch)->points.armor)
#define GET_HIT(ch) ((ch)->points.hit)
#define GET_MAX_HIT(ch) ((ch)->points.max_hit)
#define GET_BASE_HIT(ch) ((ch)->player_specials->base_hit)
#define GET_BASE_HITROLL(ch) ((ch)->points.base_hitroll)
#define GET_HITROLL(ch) ((ch)->points.hitroll)
#define GET_BASE_DAMROLL(ch) ((ch)->points.base_damroll)
#define GET_DAMROLL(ch) ((ch)->points.damroll)

#define GET_POS(ch) ((ch)->char_specials.position)
#define GET_STANCE(ch) ((ch)->char_specials.stance)
#define AWAKE(ch) (GET_STANCE(ch) > STANCE_SLEEPING)
#define SLEEPING(ch) (GET_STANCE(ch) == STANCE_SLEEPING)
#define SITTING(ch) (GET_POS(ch) == POS_SITTING)
#define STANDING(ch) (GET_POS(ch) == POS_STANDING)
#define FLYING(ch) (GET_POS(ch) == POS_FLYING)
#define ALIVE(ch) (GET_STANCE(ch) != STANCE_DEAD)
#define DECEASED(ch) (!(ALIVE(ch)))

#define GET_QUIT_REASON(ch) ((ch)->char_specials.quit_reason)
#define GET_PERCEPTION(ch) ((ch)->char_specials.perception)
#define GET_HIDDENNESS(ch) ((ch)->char_specials.hiddenness)
#define IS_HIDDEN(ch) (GET_HIDDENNESS(ch) > 0)
#define GET_IDNUM(ch) ((ch)->char_specials.idnum)
#define GET_ID(x) ((x)->id)
#define GET_SAVE(ch, i) ((ch)->char_specials.apply_saving_throw[(i)])
#define GET_RAGE(ch) ((ch)->char_specials.rage)
#define IS_ANGRY(ch) ((ch)->char_specials.rage >= RAGE_ANGRY)
#define CAN_SEE_IN_DARK(ch) (PRF_FLAGGED(ch, PRF_HOLYLIGHT) || EFF_FLAGGED(ch, EFF_ULTRAVISION) || IS_MOB(ch))
#define GET_ALIGNMENT(ch) ((ch)->char_specials.alignment)
#define IS_GOOD(ch) (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch) (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_VICIOUS(ch) (IS_NPC(ch) ? !MOB_FLAGGED((ch), MOB_NOVICIOUS) : PRF_FLAGGED((ch), PRF_VICIOUS))
#define GET_SKILL(ch, a) ((ch)->char_specials.skills[(a)] / 10)
#define SET_SKILL(ch, a, p) ((ch)->char_specials.skills[(a)] = (p))
#define GET_ISKILL(ch, a) ((ch)->char_specials.skills[(a)])
#define OUTDOOR_SNEAK(ch)                                                                                              \
    (((GET_RACE(ch) == RACE_HALFLING) && ((world[(ch)->in_room].sector_type >= SECT_FIELD) &&                          \
                                          ((world[(ch)->in_room].sector_type <= SECT_MOUNTAIN) ||                      \
                                           (world[(ch)->in_room].sector_type == SECT_GRASSLANDS)))) ||                 \
     (((GET_RACE(ch) == RACE_SVERFNEBLIN) &&                                                                           \
       ((world[(ch)->in_room].sector_type == SECT_CAVE) || (world[(ch)->in_room].sector_type == SECT_MOUNTAIN) ||      \
        (world[(ch)->in_room].sector_type == SECT_UNDERDARK)))))
#define IS_STARSTAR(ch) ((GET_LEVEL(ch) == 99 && GET_EXP(ch) >= (exp_next_level(99, GET_CLASS(ch)) - 1)))

#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
/* There's room for improvement here.  The FLY spell could have an effectiveness
 * that depends on the power of the casting, and determines how much weight
 * can be lifted. */
#define MAXIMUM_FLIGHT_LOAD(ch) (950 * CAN_CARRY_W(ch) / 1000)
#define ADDED_WEIGHT_OK(ch, obj)                                                                                       \
    ((GET_OBJ_EFFECTIVE_WEIGHT(obj) + IS_CARRYING_W(ch) <= CAN_CARRY_W(ch)) &&                                         \
     (GET_OBJ_EFFECTIVE_WEIGHT(obj) <= CAN_CARRY_W(ch)))
#define ADDED_WEIGHT_REFUSED(ch, obj) ((GET_OBJ_EFFECTIVE_WEIGHT(obj) + IS_CARRYING_W(ch) > MAXIMUM_FLIGHT_LOAD(ch)))
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
#define CAN_CARRY_W(ch) (str_app[GET_STR(ch)].carry_w)
#define CAN_CARRY_N(ch) (5 + (GET_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define CURRENT_LOAD(ch)                                                                                               \
    (IS_CARRYING_W(ch) >= CAN_CARRY_W(ch) ? 100                                                                        \
     : IS_CARRYING_W(ch) < 0              ? 0                                                                          \
                                          : (int)((IS_CARRYING_W(ch) * 10) / CAN_CARRY_W(ch)))
#define CAN_CARRY_OBJ(ch, obj)                                                                                         \
    (((IS_CARRYING_W(ch) + GET_OBJ_EFFECTIVE_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&                                       \
     ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))
#define CAN_GET_OBJ(ch, obj)                                                                                           \
    (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch), (obj)) && CAN_SEE_OBJ((ch), (obj)) &&                      \
     GET_OBJ_LEVEL(obj) <= GET_LEVEL(ch))

#define GET_EQ(ch, i) ((ch)->equipment[i])
#define CONSENT(ch) ((ch)->char_specials.consented)

/* FIGHTING is intentionally a READ-ONLY macro.
 * Call set_battling() and stop_battling() if you need to modify this.
 * See fight.h. */
#define FIGHTING(ch) (1 ? (ch)->target : 0)

#define HUNTING(ch) ((ch)->char_specials.hunting)
#define RIDING(ch) ((ch)->char_specials.riding)
#define RIDDEN_BY(ch) ((ch)->char_specials.ridden_by)
#define IS_CORNERED(ch)                                                                                                \
    ((ch)->cornered_by && CAN_SEE((ch)->cornered_by, ch) && FIGHTING((ch)->cornered_by) == (ch) &&                     \
     (ch)->in_room == (ch)->cornered_by->in_room)
#define IS_GROUPED(ch) ((ch)->group_master || (ch)->groupees)
#define GET_GROUP_LEADER(ch) (ch->group_master ? ch->group_master : ch)
#define IS_IN_GROUP(ch, tch) (IS_GROUPED(ch) && IS_GROUPED(tch) && GET_GROUP_LEADER(ch) == GET_GROUP_LEADER(tch))

#define VIEWED_ABIL(ch, abil) (LIMIT(MIN_ABILITY_VALUE, GET_ACTUAL_##abil(ch), MAX_ABILITY_VALUE))

#define GET_AFFECTED_STR(ch) ((ch)->affected_abils.str)
#define GET_VIEWED_STR(ch) VIEWED_ABIL(ch, STR)
#define GET_ACTUAL_STR(ch) ((ch)->actual_abils.str)
#define GET_NATURAL_STR(ch) ((ch)->natural_abils.str)
#define GET_STR(ch) GET_AFFECTED_STR(ch)

#define GET_AFFECTED_INT(ch) ((ch)->affected_abils.intel)
#define GET_VIEWED_INT(ch) VIEWED_ABIL(ch, INT)
#define GET_ACTUAL_INT(ch) ((ch)->actual_abils.intel)
#define GET_NATURAL_INT(ch) ((ch)->natural_abils.intel)
#define GET_INT(ch) GET_AFFECTED_INT(ch)

#define GET_AFFECTED_WIS(ch) ((ch)->affected_abils.wis)
#define GET_VIEWED_WIS(ch) VIEWED_ABIL(ch, WIS)
#define GET_ACTUAL_WIS(ch) ((ch)->actual_abils.wis)
#define GET_NATURAL_WIS(ch) ((ch)->natural_abils.wis)
#define GET_WIS(ch) GET_AFFECTED_WIS(ch)

#define GET_AFFECTED_DEX(ch) ((ch)->affected_abils.dex)
#define GET_VIEWED_DEX(ch) VIEWED_ABIL(ch, DEX)
#define GET_ACTUAL_DEX(ch) ((ch)->actual_abils.dex)
#define GET_NATURAL_DEX(ch) ((ch)->natural_abils.dex)
#define GET_DEX(ch) GET_AFFECTED_DEX(ch)

#define GET_AFFECTED_CON(ch) ((ch)->affected_abils.con)
#define GET_VIEWED_CON(ch) VIEWED_ABIL(ch, CON)
#define GET_ACTUAL_CON(ch) ((ch)->actual_abils.con)
#define GET_NATURAL_CON(ch) ((ch)->natural_abils.con)
#define GET_CON(ch) GET_AFFECTED_CON(ch)

#define GET_AFFECTED_CHA(ch) ((ch)->affected_abils.cha)
#define GET_VIEWED_CHA(ch) VIEWED_ABIL(ch, CHA)
#define GET_ACTUAL_CHA(ch) ((ch)->actual_abils.cha)
#define GET_NATURAL_CHA(ch) ((ch)->natural_abils.cha)
#define GET_CHA(ch) GET_AFFECTED_CHA(ch)

/* Player accessors */
#define GET_PAGE_LENGTH(ch) ((ch)->player_specials->page_length)
#define GET_PASSWD(ch) ((ch)->player.passwd)
#define GET_PROMPT(ch) ((ch)->player.prompt)
#define GET_GMCP_PROMPT(ch) ((ch)->player.gmcp_prompt)
#define GET_HOMEROOM(ch) ((ch)->player.homeroom)
#define GET_LASTLEVEL(ch) ((ch)->player_specials->lastlevel)
#define GET_COND(ch, i) ((ch)->player_specials->conditions[(i)])
#define IS_HUNGRY(ch) (GET_COND((ch), FULL) == 0)
#define IS_THIRSTY(ch) (GET_COND((ch), THIRST) == 0)
#define IS_DRUNK(ch) (GET_COND((ch), DRUNK) > 0)
#define GET_LOADROOM(ch) ((ch)->player_specials->load_room)
#define GET_SAVEROOM(ch) ((ch)->player_specials->save_room)
#define GET_AUTOINVIS(ch) ((ch)->player_specials->autoinvis_level)
#define GET_INVIS_LEV(ch) ((ch)->player_specials->invis_level)
#define GET_WIMP_LEV(ch) ((ch)->player_specials->wimp_level)
#define GET_AGGR_LEV(ch) ((ch)->player_specials->aggressive)
#define GET_FREEZE_LEV(ch) ((ch)->player_specials->freeze_level)
#define GET_BAD_PWS(ch) ((ch)->player_specials->bad_pws)
#define GET_TALK(ch, i) ((ch)->player_specials->talks[i])
#define GET_LOG_VIEW(ch) ((ch)->player_specials->log_view)
#define GET_POOFIN(ch) ((ch)->player_specials->poofin)
#define GET_POOFOUT(ch) ((ch)->player_specials->poofout)
#define GET_LAST_OLC_TARG(ch) ((ch)->player_specials->last_olc_targ)
#define GET_LAST_OLC_MODE(ch) ((ch)->player_specials->last_olc_mode)
#define GET_ALIASES(ch) ((ch)->player_specials->aliases)
#define GET_LAST_TELL(ch) ((ch)->player_specials->last_tell)
#define GET_ROLL(ch, id) ((ch)->player_specials->roll[id])
#define GET_HOST(ch) ((ch)->player_specials->host)
#define GET_SPELL_MEM(ch) ((ch)->spell_memory)
#define GET_GRANT_CACHE(ch) ((ch)->player_specials->grant_cache)
#define GET_REVOKE_CACHE(ch) ((ch)->player_specials->revoke_cache)
#define GET_GRANTS(ch) ((ch)->player_specials->grants)
#define GET_REVOKES(ch) ((ch)->player_specials->revokes)
#define GET_GRANT_GROUPS(ch) ((ch)->player_specials->grant_groups)
#define GET_REVOKE_GROUPS(ch) ((ch)->player_specials->revoke_groups)

/* Mob accessors */
#define GET_EX_HIT(ch) ((ch)->mob_specials.ex_hit)
#define GET_EX_MAX_HIT(ch) ((ch)->mob_specials.ex_max_hit)
#define GET_EX_MAIN_HP(ch) ((ch)->mob_specials.ex_main_hp)
#define GET_EX_GOLD(ch) ((ch)->mob_specials.ex_gold)
#define GET_EX_PLATINUM(ch) ((ch)->mob_specials.ex_platinum)
#define GET_EX_EXP(ch) ((ch)->mob_specials.ex_exp)
#define GET_EX_AC(ch) ((ch)->mob_specials.ex_armor)
#define GET_EX_MANA(ch) ((ch)->mob_specials.ex_mana)
#define GET_EX_MAX_MANA(ch) ((ch)->mob_specials.ex_max_mana)
#define GET_MOB_RNUM(mob) ((mob)->mob_specials.nr)
#define GET_MOB_SPEC(ch) (IS_MOB(ch) ? (mob_index[GET_MOB_RNUM(ch)].func) : NULL)
#define GET_MOB_VNUM(mob) (IS_MOB(mob) ? mob_index[GET_MOB_RNUM(mob)].vnum : -1)
#define GET_MOB_WAIT(ch) ((ch)->mob_specials.wait_state)
#define GET_DEFAULT_POS(ch) ((ch)->mob_specials.default_pos)
#define MEMORY(ch) ((ch)->mob_specials.memory)
#define GET_MOB_SPLBANK(ch, circle) ((ch)->mob_specials.spell_bank[(circle)])
#define GET_MOB_SPLMEM_TIME(ch) ((ch)->mob_specials.spell_mem_time)

/* descriptor-based utils ************************************************/

#define WAIT_STATE(ch, cycle)                                                                                          \
    do {                                                                                                               \
        if ((ch)->desc)                                                                                                \
            if (GET_LEVEL(ch) > LVL_IMMORT)                                                                            \
                (ch)->desc->wait = 0;                                                                                  \
            else                                                                                                       \
                (ch)->desc->wait = (cycle);                                                                            \
        else if (IS_NPC(ch))                                                                                           \
            GET_MOB_WAIT(ch) = (cycle);                                                                                \
    } while (0)

#define CHECK_WAIT(ch) (IS_NPC(ch) ? GET_MOB_WAIT(ch) : (((ch)->desc) ? ((ch)->desc->wait > 1) : 0))

#define STATE(d) ((d)->connected)
#define IS_PLAYING(d)                                                                                                  \
    (STATE(d) == CON_TRIGEDIT || STATE(d) == CON_REDIT || STATE(d) == CON_MEDIT || STATE(d) == CON_OEDIT ||            \
     STATE(d) == CON_ZEDIT || STATE(d) == CON_SEDIT || STATE(d) == CON_SDEDIT || STATE(d) == CON_PLAYING ||            \
     STATE(d) == CON_HEDIT)
#define EDITING(d) ((d)->editor)

/* compound utilities and other macros **********************************/

#define HSHR(ch)                                                                                                       \
    (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "his" : (GET_SEX(ch) == SEX_FEMALE ? "her" : "their")) : "its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "he" : (GET_SEX(ch) == SEX_FEMALE ? "she" : "they")) : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "him" : (GET_SEX(ch) == SEX_FEMALE ? "her" : "them")) : "it")
#define HISHER(ch) HSHR(ch)
#define HESHE(ch) HSSH(ch)
#define HIMHER(ch) HMHR(ch)

#define ANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")

#define SENDOK(ch)                                                                                                     \
    ((ch)->desc && (AWAKE(ch) || sleep) &&                                                                             \
     ((!PLR_FLAGGED((ch), PLR_WRITING) && !EDITING((ch)->desc)) || (olc && PRF_FLAGGED((ch), PRF_OLCCOMM))))

/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)                                                                                                  \
    (!EFF_FLAGGED(sub, EFF_BLIND) && (IS_NPC(sub) || IS_LIGHT((sub)->in_room) || EFF_FLAGGED((sub), EFF_ULTRAVISION)))

#define INVIS_OK(sub, obj)                                                                                             \
    ((!EFF_FLAGGED((obj), EFF_INVISIBLE) || EFF_FLAGGED(sub, EFF_DETECT_INVIS)) &&                                     \
     (GET_HIDDENNESS(obj) <= GET_PERCEPTION(sub)))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) (MORT_CAN_SEE(sub, obj) || PRF_FLAGGED(sub, PRF_HOLYLIGHT))

#define SELF(sub, obj) ((sub) == (obj))

#define IMM_VIS_OK(sub, obj)                                                                                           \
    ((GET_LEVEL(obj) >= LVL_IMMORT) &&                                                                                 \
     ((CONSENT(obj) == sub) || (PRF_FLAGGED((obj), PRF_ROOMVIS) && IN_ROOM(sub) == IN_ROOM(obj))))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj)                                                                                              \
    ((SELF(sub, obj)) || IS_IN_GROUP(sub, obj) ||                                                                      \
     ((GET_LEVEL(REAL_CHAR(sub)) >= GET_INVIS_LEV(obj)) && IMM_CAN_SEE(sub, obj)) || IMM_VIS_OK(sub, obj))

#define CAN_SEE_BY_INFRA(sub, obj)                                                                                     \
    ((SELF(sub, obj)) ||                                                                                               \
     ((GET_LEVEL(REAL_CHAR(sub)) >= GET_INVIS_LEV(obj)) &&                                                             \
      (((!EFF_FLAGGED(sub, EFF_BLIND) && EFF_FLAGGED(sub, EFF_INFRAVISION)) && INVIS_OK(sub, obj)) ||                  \
       PRF_FLAGGED(sub, PRF_HOLYLIGHT))) ||                                                                            \
     IMM_VIS_OK(sub, obj))

/* End of CAN_SEE */

#define CAN_SEE_MOVING(sub, obj)                                                                                       \
    (CAN_SEE(sub, obj) && (!EFF_FLAGGED(obj, EFF_MISDIRECTING) || PRF_FLAGGED(sub, PRF_HOLYLIGHT)))
#define SEES_THROUGH_MISDIRECTION(sub, obj) (GET_LEVEL(sub) >= LVL_IMMORT && GET_LEVEL(sub) >= GET_LEVEL(obj))

#define PERS(ch, vict) (CAN_SEE((vict), (ch)) ? GET_NAME(ch) : "someone")

#define ALONE(ch)                                                                                                      \
    (!(ch) || ((ch)->in_room == NOWHERE) || (((ch)->next_in_room == nullptr) && (world[(ch)->in_room].people == (ch))))

/* OS compatibility ******************************************************/

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#if defined(NOCRYPT) || !defined(HAVE_CRYPT)
#define CRYPT(a, b) (a)
#else
#define CRYPT(a, b) ((char *)crypt((a), (b)))
#endif

#define SD_LVL_MULT(i) spell_dam_info[i].lvl_mult
#define SD_SPELL(i) spell_dam_info[i].spell
#define SD_INTERN_DAM(i) spell_dam_info[i].intern_dam
#define SD_NPC_NO_DICE(i) spell_dam_info[i].npc_no_dice
#define SD_NPC_NO_FACE(i) spell_dam_info[i].npc_no_face
#define SD_PC_NO_DICE(i) spell_dam_info[i].pc_no_dice
#define SD_PC_NO_FACE(i) spell_dam_info[i].pc_no_face
#define SD_NPC_REDUCE_FACTOR(i) spell_dam_info[i].npc_reduce_factor
#define SD_USE_BONUS(i) spell_dam_info[i].use_bonus
#define SD_BONUS(i) spell_dam_info[i].max_bonus
#define SD_NPC_STATIC(i) spell_dam_info[i].npc_static
#define SD_PC_STATIC(i) spell_dam_info[i].pc_static
#define SD_NOTE(i) spell_dam_info[i].note
#define SPELL_DAM_FILE "misc/spell_dams" /*for spell dams*/

/* Format strings for strftime */
#define TIMEFMT_LOG "%a %d %b %Y %H:%M:%S" /* 24 characters */
#define TIMEFMT_DATE "%b %d %Y"            /* 11 characters */
