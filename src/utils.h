/***************************************************************************
 * $Id: utils.h,v 1.180 2009/07/17 01:19:01 myc Exp $
 ***************************************************************************/
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

#ifndef __FIERY_UTILS_H
#define __FIERY_UTILS_H

#include "strings.h"
#include "text.h"
#include "rooms.h"
#include <math.h>
#include "money.h"

/* external declarations and prototypes **********************************/

extern struct zone_data *zone_table;
extern struct index_data *obj_index;

/* This should be in weather.h, but IS_DARK uses hemisphere data, so we
 * need it here. */
extern struct hemisphere_data hemispheres[NUM_HEMISPHERES];

#define log log_printf
#define mprintf mudlog_printf

/* deprecated functions */
void mudlog(const char *str, unsigned char type, int level, byte file);

/* public functions in utils.c */
void perform_random_gem_drop(struct char_data *);
bool event_target_valid(struct char_data *ch);
int con_aff(struct char_data *ch);
int static_ac(int dex);
void log(const char *str, ...) __attribute__ ((format (printf, 1, 2)));
int touch(const char *path);
void mudlog_printf(int severity, int level, const char *str, ...) __attribute__ ((format (printf, 3, 4))) ;
const char *sprint_log_severity(int severity);
int parse_log_severity(const char *severity);
void log_death_trap(struct char_data *ch);
int get_line(FILE *fl, char *buf);
struct time_info_data age(struct char_data *ch);
int num_pc_in_room(struct room_data *room);
int load_modifier(struct char_data *ch);
const char *movewords(struct char_data *ch, int cmd, int room, int leaving);
void build_count(void);
int monk_weight_penalty(struct char_data *ch);
int find_zone(int num);
int parse_obj_name(struct char_data *ch, char *arg, char *objname, int numobjs, void *objects, int objsize);
void init_flagvectors();
long exp_next_level(int level, int class);
void init_exp_table(void);

void sort(void algorithm(int[], int, int(int a, int b)),
          int array[], int count, int comparator(int, int));
void bubblesort(int array[], int count, int comparator(int a, int b));
void insertsort(int array[], int count, int comparator(int a, int b));
void quicksort(int array[], int count, int comparator(int a, int b));
void optquicksort(int array[], int count, int comparator(int a, int b));

void update_pos(struct char_data *victim);

int yesno_result(char *answer);
#define YESNO_NONE  0    /* just pressed enter (probably wanted default) */
#define YESNO_OTHER 1    /* typed something, but not starting with y or n */
#define YESNO_YES   2    /* entered something starting with y or Y */
#define YESNO_NO    3    /* entered something starting with n or N */

char *statelength(int inches);
char *stateweight(float pounds);
char *format_apply(int apply, int amount);
void drop_core(struct char_data *ch, const char *desc);


/* various constants *****************************************************/

/* generic multi-purpose flag constants */
#define FAIL_SILENTLY       (1 << 0)


/* defines for mudlog() */
#define OFF   0
#define BRF   1
#define NRM   2
#define CMP   3

/* severity defines for mprintf(): gaps for future additions */
#define L_CRIT       70 /* serious errors (data corruption, need reboot) */
#define L_ERROR      60 /* other non-fatal errors: something's broke */
#define L_WARN       50 /* warnings: stuff is still working */
#define L_STAT       40 /* mud status: players login, deaths, etc */
#define L_DEBUG      30 /* coding debug/script errors */
#define L_INFO       20 /* monotonous info: zone resets, etc */
#define L_TRACE      10 /* trace info: hard core debug maybe? */
#define L_NOFILE     (1 << 7)

/* breadth-first searching */
#define BFS_ERROR      -1
#define BFS_ALREADY_THERE   -2
#define BFS_NO_PATH      -3


/* mud-life time */
#define HOURS_PER_DAY       24
#define DAYS_PER_WEEK       7
#define WEEKS_PER_MONTH     5
#define DAYS_PER_MONTH      (DAYS_PER_WEEK * WEEKS_PER_MONTH)
#define MONTHS_PER_YEAR     16
#define WEEKS_PER_YEAR      (WEEKS_PER_MONTH * MONTHS_PER_YEAR)
#define DAYS_PER_YEAR       (DAYS_PER_MONTH * DAYS_PER_MONTH)
#define SECS_PER_MUD_HOUR   75
#define SECS_PER_MUD_DAY    (HOURS_PER_DAY * SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH  (DAYS_PER_MONTH * SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR   (MONTHS_PER_YEAR * SECS_PER_MUD_MONTH)


/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN   60
#define SECS_PER_REAL_HOUR  (60 * SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY   (24 * SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR  (365 * SECS_PER_REAL_DAY)

/* string utils **********************************************************/

#define YESNO(a)       ((a) ? "YES" : "NO")
#define ONOFF(a)       ((a) ? "ON" : "OFF")

/* IS_UPPER and IS_LOWER added by dkoepke */
#define IS_UPPER(c)    ((c) >= 'A' && (c) <= 'Z')
#define IS_LOWER(c)    ((c) >= 'a' && (c) <= 'z')

#define LOWER(c)       (IS_UPPER(c) ? ((c)+('a'-'A')) : (c))
#define UPPER(c)       (IS_LOWER(c) ? ((c)+('A'-'a')) : (c))

#define IS_NEWLINE(ch) ((ch) == '\n' || (ch) == '\r')
#define CAP(st)        (cap_by_color(st))

#define AN(string)     (strchr("aeiouAEIOU", *string) ? "an" : "a")

/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
   if (!((result) = (type *) calloc ((number), sizeof(type))))\
      { perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
      { perror("realloc failure"); abort(); } } while(0)

/*
 * A macro to remove an item from a list: if it's the list head, change the head,
 * else traverse the list looking for the item before the one to be removed.
 * To use, just make sure that there is a variable 'temp' declared as the same
 * type as the list to be manipulated.
 */
#define REMOVE_FROM_LIST(item, head, next) do {\
   if ((item) == (head)) \
      head = (item)->next; \
   else { \
      temp = head; \
      while (temp && (temp->next != (item))) \
    temp = temp->next; \
      if (temp) \
         temp->next = (item)->next; \
   } \
} while (0)

#define LOOP_THRU_PEOPLE(IN_ROOM, PLAYER) \
for ((IN_ROOM) = world[(PLAYER)->in_room].people; (IN_ROOM) != NULL; (IN_ROOM) = (IN_ROOM)->next_in_room)

/* basic bitvector utils *************************************************/

#define IS_SET(flag,bit)         ((flag) & (bit))
#define SET_BIT(var,bit)         ((var) |= (bit))
#define REMOVE_BIT(var,bit)      ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit)      ((var) = (var) ^ (bit))

/* extended bitvector utils */
#define FIELD(x)                 ((unsigned long int) (x) / FLAGBLOCK_SIZE)
#define FLAG(x)                  (1 << ((x) % FLAGBLOCK_SIZE))
#define IS_FLAGGED(field, flag)  (((field)[FIELD(flag)] & FLAG(flag)) ? 1 : 0)
#define SET_FLAG(field, flag)    ((field)[FIELD(flag)] |= FLAG(flag))
#define REMOVE_FLAG(field, flag) ((field)[FIELD(flag)] &= ~FLAG(flag))
#define TOGGLE_FLAG(field, flag) ((field)[FIELD(flag)] ^= FLAG(flag))
bool ALL_FLAGGED(const flagvector field[], const flagvector flags[], const int num_flags);
bool ANY_FLAGGED(const flagvector field[], const flagvector flags[], const int num_flags);
void SET_FLAGS(flagvector field[], const flagvector flags[], const int num_flags);
void REMOVE_FLAGS(flagvector field[], const flagvector flags[], const int num_flags);
void TOGGLE_FLAGS(flagvector field[], const flagvector flags[], const int num_flags);
void COPY_FLAGS(flagvector field[], const flagvector flags[], const int num_flags);
extern flagvector *ALL_FLAGS;
#define HAS_FLAGS(field, num_flags)   (ANY_FLAGGED((field), ALL_FLAGS, (num_flags)))
#define CLEAR_FLAGS(field, num_flags) (REMOVE_FLAGS((field), ALL_FLAGS, (num_flags)))

/* Event flags */
#define GET_EVENTS(o)              ((o)->events)
#define GET_EVENT_FLAGS(o)         ((o)->event_flags)
#define EVENT_FLAGGED(o, flag)     IS_FLAGGED(GET_EVENT_FLAGS(o), (flag))

#define MOB_FLAGS(ch)              ((ch)->char_specials.act)
#define PLR_FLAGS(ch)              ((ch)->char_specials.act)
#define PRF_FLAGS(ch)              ((ch)->player_specials->pref)
#define PRV_FLAGS(ch)              ((ch)->player_specials->privileges)
#define EFF_FLAGS(ch)              ((ch)->char_specials.effects)
#define ROOM_FLAGS(loc)            (world[(loc)].room_flags)
#define ROOM_EFFECTS(loc)          (world[(loc)].room_effects)

#define IS_NPC(ch)                 IS_FLAGGED(MOB_FLAGS(ch), MOB_ISNPC)
#define IS_MOB(ch)                 (IS_NPC(ch) && ((ch)->mob_specials.nr > -1))
#define POSSESSED(ch)              ((ch)->desc && (ch)->desc->original)
#define POSSESSOR(ch)              ((ch)->desc && (ch)->desc->original ? (ch)->desc->original : NULL)
#define REAL_CHAR(ch)              ((ch)->desc && (ch)->desc->original ? (ch)->desc->original : (ch))
#define FORWARD(ch)                ((ch)->forward ? (ch)->forward : (ch))
#define IS_PC(ch)                  (!IS_NPC(REAL_CHAR(ch)))
/* PLAYERALLY - true for a player, or a player's pet */
#define PLAYERALLY(ch) (IS_PC(ch) || \
      (EFF_FLAGGED(ch, EFF_CHARM) && (ch)->master && IS_PC((ch)->master)))

/* MORTALALLY - same as PLAYERALLY except that the involved player
 * must be mortal for it to return true */
#define MORTALALLY(ch) ( \
      (!IS_NPC(REAL_CHAR(ch)) && GET_LEVEL(REAL_CHAR(ch)) < LVL_IMMORT) || \
      (EFF_FLAGGED(ch, EFF_CHARM) && (ch)->master && \
         (!IS_NPC(REAL_CHAR(ch)) && GET_LEVEL(REAL_CHAR(ch)) < LVL_IMMORT)))

#define MOB_FLAGGED(ch, flag)      (IS_NPC(ch) && IS_FLAGGED(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag)      (!IS_NPC(ch) && IS_FLAGGED(PLR_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag)      IS_FLAGGED(PRF_FLAGS(REAL_CHAR(ch)), (flag))
#define PRV_FLAGGED(ch, flag)      IS_FLAGGED(PRV_FLAGS(REAL_CHAR(ch)), (flag))
#define EFF_FLAGGED(ch, flag)      IS_FLAGGED(EFF_FLAGS(ch), (flag))
#define ROOM_FLAGGED(loc, flag)    IS_FLAGGED(ROOM_FLAGS(loc), (flag))
#define ROOM_EFF_FLAGGED(loc, eff) IS_FLAGGED(ROOM_EFFECTS(loc), (eff))

#define AGGR_TO_PLAYERS(ch) (\
   MOB_FLAGGED((ch), MOB_AGGR_EVIL) || \
   MOB_FLAGGED((ch), MOB_AGGR_GOOD) || \
   MOB_FLAGGED((ch), MOB_AGGR_NEUTRAL) || \
   MOB_FLAGGED((ch), MOB_AGGR_EVIL_RACE) || \
   MOB_FLAGGED((ch), MOB_AGGR_GOOD_RACE))
#define PLR_TOG_CHK(ch, flag)      (TOGGLE_FLAG(PLR_FLAGS(ch), (flag)) & FLAG(flag))
#define PRF_TOG_CHK(ch, flag)      (TOGGLE_FLAG(PRF_FLAGS(ch), (flag)) & FLAG(flag))
#define PRV_TOG_CHK(ch, flag)      (TOGGLE_FLAG(PRV_FLAGS(ch), (flag)) & FLAG(flag))
#define CONFUSED(ch)               EFF_FLAGGED(ch, EFF_CONFUSION)

/* Mob performs scripts?  (specprocs and triggers)
 * - is an NPC
 * - doesn't have !script flag
 * - isn't charmed
 */
#define MOB_PERFORMS_SCRIPTS(ch) (IS_NPC(ch) && \
      !MOB_FLAGGED(ch, MOB_NOSCRIPT) && \
         !EFF_FLAGGED(ch, EFF_CHARM))

#define MEMMING(ch)                EVENT_FLAGGED((ch), EVENT_MEM)

/* char utils ************************************************************/

/* Identifier accessors */
#define GET_PFILEPOS(ch)    ((ch)->pfilepos)
#define GET_NAMELIST(ch)    ((ch)->player.namelist)
#define GET_NAME(ch)        ((ch)->player.short_descr)
#define GET_SHORT(ch)       ((ch)->player.short_descr)
#define GET_TITLE(ch)       ((ch)->player.title)
#define GET_WIZ_TITLE(ch)   ((ch)->player_specials->wiz_title)
#define GET_PERM_TITLES(ch) ((ch)->player_specials->perm_titles)
#define GET_LDESC(ch)       ((ch)->player.long_descr)

/* General accessors */
#define IN_ROOM(ch)         ((ch)->in_room)
#define IN_ROOM_VNUM(ch)    (IN_ROOM(ch) > NOWHERE && IN_ROOM(ch) < top_of_world ? \
      world[IN_ROOM(ch)].vnum : NOWHERE)
#define IN_ZONE_RNUM(ch)    (world[IN_ROOM(ch)].zone)
#define IN_ZONE_VNUM(ch)    (zone_table[IN_ZONE_RNUM(ch)].number)
#define GET_WAS_IN(ch)      ((ch)->was_in_room)
#define GET_LEVEL(ch)       ((ch)->player.level)
#define GET_LIFEFORCE(ch)   ((ch)->player.lifeforce)
#define GET_COMPOSITION(ch) ((ch)->player.composition)
#define GET_SIZE(ch)        ((ch)->player.affected_size)
#define GET_AGE(ch)         (age(ch).year)
#define GET_CLASS(ch)       ((ch)->player.class)
#define GET_HEIGHT(ch)      ((ch)->player.height)
#define GET_WEIGHT(ch)      ((ch)->player.weight)
#define GET_SEX(ch)         ((ch)->player.sex)
#define GET_EXP(ch)         ((ch)->points.exp)
#define GET_MOVE(ch)       ((ch)->points.move)
#define GET_MAX_MOVE(ch)    ((ch)->points.max_move)
#define GET_MANA(ch)       ((ch)->points.mana)
#define GET_MAX_MANA(ch)    ((ch)->points.max_mana)
#define GET_COINS(ch)       ((ch)->points.coins)
#define GET_PURSE_COINS(ch, coin) (GET_COINS(ch)[coin])
#define GET_PLATINUM(ch)    (GET_COINS(ch)[PLAT])
#define GET_GOLD(ch)        (GET_COINS(ch)[GOLD])
#define GET_SILVER(ch)      (GET_COINS(ch)[SILVER])
#define GET_COPPER(ch)      (GET_COINS(ch)[COPPER])
#define GET_BANK_COINS(ch)  ((ch)->points.bank)
#define GET_ACCOUNT_COINS(ch, coin) (GET_BANK_COINS(ch)[coin])
#define GET_BANK_PLATINUM(ch)   (GET_BANK_COINS(ch)[PLAT])
#define GET_BANK_GOLD(ch)   (GET_BANK_COINS(ch)[GOLD])
#define GET_BANK_SILVER(ch) (GET_BANK_COINS(ch)[SILVER])
#define GET_BANK_COPPER(ch) (GET_BANK_COINS(ch)[COPPER])
#define GET_CASH(ch)        (GET_PLATINUM(ch) * 1000 + \
                             GET_GOLD(ch) * 100 + \
                             GET_SILVER(ch) * 10 + \
                             GET_COPPER(ch))
#define GET_AC(ch)          ((ch)->points.armor)
#define GET_HIT(ch)         ((ch)->points.hit)
#define GET_MAX_HIT(ch)     ((ch)->points.max_hit)
#define GET_BASE_HIT(ch)    ((ch)->player_specials->base_hit)
#define GET_BASE_HITROLL(ch)((ch)->points.base_hitroll)
#define GET_HITROLL(ch)     ((ch)->points.hitroll)
#define GET_BASE_DAMROLL(ch)((ch)->points.base_damroll)
#define GET_DAMROLL(ch)     ((ch)->points.damroll)

#define GET_POS(ch)       ((ch)->char_specials.position)
#define GET_STANCE(ch)       ((ch)->char_specials.stance)
#define AWAKE(ch)           (GET_STANCE(ch) > STANCE_SLEEPING)
#define SLEEPING(ch)        (GET_STANCE(ch) == STANCE_SLEEPING)
#define SITTING(ch)         (GET_POS(ch) == POS_SITTING)
#define STANDING(ch)        (GET_POS(ch) == POS_STANDING)
#define FLYING(ch)          (GET_POS(ch) == POS_FLYING)
#define ALIVE(ch)           (GET_STANCE(ch) != STANCE_DEAD)
#define DECEASED(ch)        (!(ALIVE(ch)))

#define GET_QUIT_REASON(ch) ((ch)->char_specials.quit_reason)
#define GET_PERCEPTION(ch)  ((ch)->char_specials.perception)
#define GET_HIDDENNESS(ch)  ((ch)->char_specials.hiddenness)
#define IS_HIDDEN(ch)       (GET_HIDDENNESS(ch) > 0)
#define GET_IDNUM(ch)       ((ch)->char_specials.idnum)
#define GET_ID(x)           ((x)->id)
#define GET_SAVE(ch, i)     ((ch)->char_specials.apply_saving_throw[(i)])
#define GET_RAGE(ch)        ((ch)->char_specials.rage)
#define IS_ANGRY(ch)        ((ch)->char_specials.rage >= RAGE_ANGRY)
#define CAN_SEE_IN_DARK(ch) \
   (PRF_FLAGGED(ch, PRF_HOLYLIGHT) || EFF_FLAGGED(ch, EFF_ULTRAVISION) \
    || IS_MOB(ch))
#define GET_ALIGNMENT(ch)   ((ch)->char_specials.alignment)
#define IS_GOOD(ch)         (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)         (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch)      (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_VICIOUS(ch)      (IS_NPC(ch) ? !MOB_FLAGGED((ch), MOB_NOVICIOUS) : PRF_FLAGGED((ch), PRF_VICIOUS))
#define GET_SKILL(ch, a)    ((ch)->char_specials.skills[(a)] / 10)
#define SET_SKILL(ch, a, p) ((ch)->char_specials.skills[(a)] = (p))
#define GET_ISKILL(ch, a)   ((ch)->char_specials.skills[(a)])
#define OUTDOOR_SNEAK(ch) \
 ((GET_RACE(ch) == RACE_ELF) \
 && ((world[(ch)->in_room].sector_type >= SECT_FIELD) && \
 ((world[(ch)->in_room].sector_type <= SECT_MOUNTAIN) || \
  (world[(ch)->in_room].sector_type == SECT_GRASSLANDS))))
#define IS_STARSTAR(ch)     ((GET_LEVEL(ch) == 99 && GET_EXP(ch) >= \
                             (exp_next_level(99, GET_CLASS(ch)) - 1)))



#define IS_CARRYING_W(ch)   ((ch)->char_specials.carry_weight)
/* There's room for improvement here.  The FLY spell could have an effectiveness
 * that depends on the power of the casting, and determines how much weight
 * can be lifted. */
#define MAXIMUM_FLIGHT_LOAD(ch) (950 * CAN_CARRY_W(ch) / 1000)
#define ADDED_WEIGHT_OK(ch, obj) ( \
      (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(ch) <= CAN_CARRY_W(ch)) && \
      (GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch)))
#define ADDED_WEIGHT_REFUSED(ch, obj) ( \
      (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(ch) > MAXIMUM_FLIGHT_LOAD(ch)))
#define IS_CARRYING_N(ch)   ((ch)->char_specials.carry_items)
#define CAN_CARRY_W(ch)     (str_app[GET_STR(ch)].carry_w)
#define CAN_CARRY_N(ch)     (5 + (GET_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define CURRENT_LOAD(ch)    (IS_CARRYING_W(ch) >= CAN_CARRY_W(ch) ? 100 : \
      (int)((IS_CARRYING_W(ch) * 10) / CAN_CARRY_W(ch)))
#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))
#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)) && GET_OBJ_LEVEL(obj) <= GET_LEVEL(ch))


#define GET_EQ(ch, i)       ((ch)->equipment[i])
#define CONSENT(ch)         ((ch)->char_specials.consented)

/* FIGHTING is intentionally a READ-ONLY macro.
 * Call set_battling() and stop_battling() if you need to modify this.
 * See fight.h. */
#define FIGHTING(ch) (1 ? (ch)->target : 0)

#define HUNTING(ch)         ((ch)->char_specials.hunting)
#define RIDING(ch)          ((ch)->char_specials.riding)
#define RIDDEN_BY(ch)       ((ch)->char_specials.ridden_by)
#define IS_CORNERED(ch)     ((ch)->cornered_by && \
                             CAN_SEE((ch)->cornered_by, ch) && \
                             FIGHTING((ch)->cornered_by) == (ch) && \
                             (ch)->in_room == (ch)->cornered_by->in_room)
#define IS_GROUPED(ch)      ((ch)->group_master || (ch)->groupees)

#define VIEWED_ABIL(ch, abil)(LIMIT(MIN_ABILITY_VALUE, GET_ACTUAL_##abil(ch), MAX_ABILITY_VALUE))

#define GET_AFFECTED_STR(ch) ((ch)->affected_abils.str)
#define GET_VIEWED_STR(ch)   VIEWED_ABIL(ch, STR)
#define GET_ACTUAL_STR(ch)   ((ch)->actual_abils.str)
#define GET_NATURAL_STR(ch)  ((ch)->natural_abils.str)
#define GET_STR(ch)          GET_AFFECTED_STR(ch)

#define GET_AFFECTED_INT(ch) ((ch)->affected_abils.intel)
#define GET_VIEWED_INT(ch)   VIEWED_ABIL(ch, INT)
#define GET_ACTUAL_INT(ch)   ((ch)->actual_abils.intel)
#define GET_NATURAL_INT(ch)  ((ch)->natural_abils.intel)
#define GET_INT(ch)          GET_AFFECTED_INT(ch)

#define GET_AFFECTED_WIS(ch) ((ch)->affected_abils.wis)
#define GET_VIEWED_WIS(ch)   VIEWED_ABIL(ch, WIS)
#define GET_ACTUAL_WIS(ch)   ((ch)->actual_abils.wis)
#define GET_NATURAL_WIS(ch)  ((ch)->natural_abils.wis)
#define GET_WIS(ch)          GET_AFFECTED_WIS(ch)

#define GET_AFFECTED_DEX(ch) ((ch)->affected_abils.dex)
#define GET_VIEWED_DEX(ch)   VIEWED_ABIL(ch, DEX)
#define GET_ACTUAL_DEX(ch)   ((ch)->actual_abils.dex)
#define GET_NATURAL_DEX(ch)  ((ch)->natural_abils.dex)
#define GET_DEX(ch)          GET_AFFECTED_DEX(ch)

#define GET_AFFECTED_CON(ch) ((ch)->affected_abils.con)
#define GET_VIEWED_CON(ch)   VIEWED_ABIL(ch, CON)
#define GET_ACTUAL_CON(ch)   ((ch)->actual_abils.con)
#define GET_NATURAL_CON(ch)  ((ch)->natural_abils.con)
#define GET_CON(ch)          GET_AFFECTED_CON(ch)

#define GET_AFFECTED_CHA(ch) ((ch)->affected_abils.cha)
#define GET_VIEWED_CHA(ch)   VIEWED_ABIL(ch, CHA)
#define GET_ACTUAL_CHA(ch)   ((ch)->actual_abils.cha)
#define GET_NATURAL_CHA(ch)  ((ch)->natural_abils.cha)
#define GET_CHA(ch)          GET_AFFECTED_CHA(ch)


/* Player accessors */
#define GET_PAGE_LENGTH(ch) ((ch)->player_specials->page_length)
#define GET_PASSWD(ch)      ((ch)->player.passwd)
#define GET_PROMPT(ch)      ((ch)->player.prompt)
#define GET_HOMEROOM(ch)        ((ch)->player.homeroom)
#define GET_LASTLEVEL(ch)   ((ch)->player_specials->lastlevel)
#define GET_COND(ch, i)       ((ch)->player_specials->conditions[(i)])
#define IS_HUNGRY(ch)       (GET_COND((ch), FULL) == 0)
#define IS_THIRSTY(ch)      (GET_COND((ch), THIRST) == 0)
#define IS_DRUNK(ch)        (GET_COND((ch), DRUNK) > 0)
#define GET_LOADROOM(ch)    ((ch)->player_specials->load_room)
#define GET_SAVEROOM(ch)    ((ch)->player_specials->save_room)
#define GET_AUTOINVIS(ch)   ((ch)->player_specials->autoinvis_level)
#define GET_INVIS_LEV(ch)   ((ch)->player_specials->invis_level)
#define GET_WIMP_LEV(ch)    ((ch)->player_specials->wimp_level)
#define GET_AGGR_LEV(ch)    ((ch)->player_specials->aggressive)
#define GET_FREEZE_LEV(ch)  ((ch)->player_specials->freeze_level)
#define GET_BAD_PWS(ch)     ((ch)->player_specials->bad_pws)
#define GET_TALK(ch, i)     ((ch)->player_specials->talks[i])
#define GET_LOG_VIEW(ch)    ((ch)->player_specials->log_view)
#define GET_POOFIN(ch)      ((ch)->player_specials->poofin)
#define GET_POOFOUT(ch)     ((ch)->player_specials->poofout)
#define GET_LAST_OLC_TARG(ch) ((ch)->player_specials->last_olc_targ)
#define GET_LAST_OLC_MODE(ch) ((ch)->player_specials->last_olc_mode)
#define GET_ALIASES(ch)     ((ch)->player_specials->aliases)
#define GET_LAST_TELL(ch)   ((ch)->player_specials->last_tell)
#define GET_ROLL(ch, id)    ((ch)->player_specials->roll[id])
#define GET_HOST(ch)        ((ch)->player_specials->host)
#define GET_SPELL_MEM(ch)   ((ch)->spell_memory)
#define GET_GRANT_CACHE(ch)  ((ch)->player_specials->grant_cache)
#define GET_REVOKE_CACHE(ch) ((ch)->player_specials->revoke_cache)
#define GET_GRANTS(ch)      ((ch)->player_specials->grants)
#define GET_REVOKES(ch)     ((ch)->player_specials->revokes)
#define GET_GRANT_GROUPS(ch)((ch)->player_specials->grant_groups)
#define GET_REVOKE_GROUPS(ch) ((ch)->player_specials->revoke_groups)


/* Mob accessors */
#define GET_EX_HIT(ch)      ((ch)->mob_specials.ex_hit)
#define GET_EX_MAX_HIT(ch)  ((ch)->mob_specials.ex_max_hit)
#define GET_EX_MAIN_HP(ch)  ((ch)->mob_specials.ex_main_hp)
#define GET_EX_GOLD(ch)     ((ch)->mob_specials.ex_gold)
#define GET_EX_PLATINUM(ch)     ((ch)->mob_specials.ex_platinum)
#define GET_EX_EXP(ch)      ((ch)->mob_specials.ex_exp)
#define GET_EX_AC(ch)       ((ch)->mob_specials.ex_armor)
#define GET_EX_MANA(ch)     ((ch)->mob_specials.ex_mana)
#define GET_EX_MAX_MANA(ch) ((ch)->mob_specials.ex_max_mana)
#define GET_MOB_RNUM(mob)   ((mob)->mob_specials.nr)
#define GET_MOB_SPEC(ch)    (IS_MOB(ch) ? (mob_index[GET_MOB_RNUM(ch)].func) : NULL)
#define GET_MOB_VNUM(mob)   (IS_MOB(mob) ? \
             mob_index[GET_MOB_RNUM(mob)].virtual : -1)
#define GET_MOB_WAIT(ch)    ((ch)->mob_specials.wait_state)
#define GET_DEFAULT_POS(ch) ((ch)->mob_specials.default_pos)
#define MEMORY(ch)          ((ch)->mob_specials.memory)
#define GET_MOB_SPLBANK(ch, circle) \
    ((ch)->mob_specials.spell_bank[(circle)])
#define GET_MOB_SPLMEM_TIME(ch) \
    ((ch)->mob_specials.spell_mem_time)


/* descriptor-based utils ************************************************/

#define WAIT_STATE(ch, cycle) do { \
   if ((ch)->desc) \
   if (GET_LEVEL(ch) > LVL_IMMORT) \
      (ch)->desc->wait = 0; \
   else \
      (ch)->desc->wait = (cycle); \
   else if (IS_NPC(ch)) GET_MOB_WAIT(ch) = (cycle); } while (0)

#define CHECK_WAIT(ch) \
   (IS_NPC(ch) ? GET_MOB_WAIT(ch) : (((ch)->desc) ? ((ch)->desc->wait > 1) : 0))

#define STATE(d)   ((d)->connected)
#define IS_PLAYING(d)   (STATE(d) == CON_TRIGEDIT || STATE(d) == CON_REDIT || \
                         STATE(d) == CON_MEDIT || STATE(d) == CON_OEDIT || \
                         STATE(d) == CON_ZEDIT || STATE(d) == CON_SEDIT || \
                         STATE(d) == CON_SDEDIT || STATE(d) == CON_PLAYING || \
                         STATE(d) == CON_HEDIT)
#define EDITING(d) ((d)->editor)

/* compound utilities and other macros **********************************/

#define HSHR(ch)   (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "his":"her") : "its")
#define HSSH(ch)   (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch)   (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "him":"her") : "it")
#define HISHER(ch) HSHR(ch)
#define HESHE(ch)  HSSH(ch)
#define HIMHER(ch) HMHR(ch)


#define ANA(obj)   (strchr("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")
#define SANA(obj)  (strchr("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")

#define SENDOK(ch) ((ch)->desc && (AWAKE(ch) || sleep) && \
           ((!PLR_FLAGGED((ch), PLR_WRITING) && !EDITING((ch)->desc)) || \
                     (olc && PRF_FLAGGED((ch), PRF_OLCCOMM))))


/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)   (!EFF_FLAGGED(sub, EFF_BLIND) && \
   (IS_NPC(sub) || IS_LIGHT((sub)->in_room) || \
    EFF_FLAGGED((sub), EFF_ULTRAVISION)))

#define INVIS_OK(sub, obj) \
 ((!EFF_FLAGGED((obj), EFF_INVISIBLE) || EFF_FLAGGED(sub, EFF_DETECT_INVIS)) && \
 (GET_HIDDENNESS(obj) <= GET_PERCEPTION(sub) ))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || PRF_FLAGGED(sub, PRF_HOLYLIGHT))

#define SELF(sub, obj)  ((sub) == (obj))

#define IMM_VIS_OK(sub, obj) \
  ((GET_LEVEL(obj) >= LVL_IMMORT) && ((CONSENT(obj) == sub) || \
   (PRF_FLAGGED((obj), PRF_ROOMVIS) && IN_ROOM(sub) == IN_ROOM(obj))))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) ((SELF(sub, obj)) || \
 ((GET_LEVEL(REAL_CHAR(sub)) >= GET_INVIS_LEV(obj)) && IMM_CAN_SEE(sub, obj)) || \
  IMM_VIS_OK(sub, obj))

#define CAN_SEE_BY_INFRA(sub, obj) ((SELF(sub, obj)) || \
 ((GET_LEVEL(REAL_CHAR(sub)) >= GET_INVIS_LEV(obj)) && \
   (( (!EFF_FLAGGED(sub, EFF_BLIND) && EFF_FLAGGED(sub, EFF_INFRAVISION)) \
  && INVIS_OK(sub, obj)) || PRF_FLAGGED(sub, PRF_HOLYLIGHT)) ) || \
  IMM_VIS_OK(sub, obj))

/* End of CAN_SEE */

#define CAN_SEE_MOVING(sub, obj) (CAN_SEE(sub, obj) && \
      (!EFF_FLAGGED(obj, EFF_MISDIRECTING) || PRF_FLAGGED(sub, PRF_HOLYLIGHT)))
#define SEES_THROUGH_MISDIRECTION(sub, obj) \
      (GET_LEVEL(sub) >= LVL_IMMORT && GET_LEVEL(sub) >= GET_LEVEL(obj))

#define PERS(ch, vict)   (CAN_SEE((vict), (ch)) ? GET_NAME(ch) : "someone")

#define ALONE(ch)  (!(ch) || ((ch)->in_room == NOWHERE) || \
          (((ch)->next_in_room == NULL) && (world[(ch)->in_room].people == (ch))))

/* OS compatibility ******************************************************/

/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
  #define NULL (void *)0
#endif

#if !defined(FALSE)
  #define FALSE 0
#endif

#if !defined(TRUE)
 #define TRUE  (!FALSE)
#endif

/* defines for fseek */
#ifndef SEEK_SET
  #define SEEK_SET   0
  #define SEEK_CUR   1
  #define SEEK_END   2
#endif

#if defined(NOCRYPT) || !defined(HAVE_CRYPT)
  #define CRYPT(a,b) (a)
#else
  #define CRYPT(a,b) ((char *) crypt((a),(b)))
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
#define TIMEFMT_LOG "%a %d %b %Y %H:%M:%S"   /* 24 characters */
#define TIMEFMT_DATE "%b %d %Y"              /* 11 characters */

#endif

/***************************************************************************
 * $Log: utils.h,v $
 * Revision 1.180  2009/07/17 01:19:01  myc
 * Autosplit no longer gives an error message if no one else
 * is present.
 *
 * Revision 1.179  2009/06/10 02:27:14  myc
 * Replace a magic 100 with the correct LVL_IMMORT.
 *
 * Revision 1.178  2009/06/09 05:51:38  myc
 * Changing cap_by_color so CAP() still returns the string in
 * question, but doesn't require a comma sequence.  Adding
 * accessor macros for privilege flags and character forwarding.
 *
 * Revision 1.177  2009/03/20 23:02:59  myc
 * Remove text editor connection state.
 *
 * Revision 1.176  2009/03/20 14:25:58  jps
 * Added macros for getting coin amounts from characters.
 *
 * Revision 1.175  2009/03/16 19:17:52  jps
 * Change macro GET_HOME to GET_HOMEROOM
 *
 * Revision 1.174  2009/03/13 04:40:10  jps
 * Fix up some money macros.
 *
 * Revision 1.173  2009/03/13 04:21:41  jps
 * Update coin macros
 *
 * Revision 1.171  2009/03/09 05:51:25  jps
 * Moved some money-related functions from utils to money
 *
 * Revision 1.170  2009/03/09 03:33:03  myc
 * Split off string functions from this file into strings.c
 *
 * Revision 1.169  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.168  2009/03/07 11:13:30  jps
 * Updated seeing/darkness macros
 *
 * Revision 1.167  2009/02/21 03:30:16  myc
 * Removed L_FILE flag--mprintf now logs to file by default;
 * assert L_NOFILE to prevent that.
 *
 * Revision 1.166  2009/02/11 17:03:39  myc
 * Add smash_tilde(), which removes tildes from the end of lines.
 * Make str_ functions take const formats.  Check EDITING(d)
 * in SENDOK().
 *
 * Revision 1.165  2008/09/26 18:37:59  jps
 * Make FIGHTING a read-only macro.
 *
 * Revision 1.164  2008/09/25 04:47:49  jps
 * Add drop_core() function, to drop the mud's core without terminating.
 *
 * Revision 1.163  2008/09/22 02:09:17  jps
 * Changed weight into a floating-point value. Precision is preserved to
 * the 1/100 place.
 *
 * Revision 1.162  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.161  2008/09/21 04:54:23  myc
 * Added grant caches to the player structure to make can_use_command
 * take less execution time.
 *
 * Revision 1.160  2008/09/20 06:05:06  jps
 * Add macros POSSESSED and POSSESSOR.
 *
 * Revision 1.159  2008/09/09 08:23:37  jps
 * Placed sector info into a struct and moved its macros into rooms.h.
 *
 * Revision 1.158  2008/09/07 20:05:55  jps
 * Fixed IS_STARSTAR to correctly look at the exp for level 100.
 *
 * Revision 1.157  2008/09/07 18:45:15  jps
 * Added briefmoney function for printing the top two coins of a money
 * value in a given number of spaces. With color.
 *
 * Revision 1.156  2008/09/07 07:20:12  jps
 * Added MORTALALLY macro.
 *
 * Revision 1.155  2008/09/07 01:28:48  jps
 * Define maximum flight load. Also you can't be given enough weight to make
 * you fall from flying.
 *
 * Revision 1.154  2008/09/06 19:10:31  jps
 * Add PLAYERALLY macro.
 *
 * Revision 1.153  2008/09/04 06:47:36  jps
 * Changed sector constants to match their strings
 *
 * Revision 1.152  2008/09/03 17:34:08  myc
 * Moved liquid information into a def struct array.
 *
 * Revision 1.151  2008/09/02 06:50:25  jps
 * Moving some function prototypes to limits.h. Some other ones weren't used
 * anywhere so I deleted them.
 *
 * Revision 1.150  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.149  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.148  2008/09/01 18:29:38  jps
 * consolidating cooldown code in skills.c/h
 *
 * Revision 1.147  2008/09/01 00:01:44  jps
 * Added AGGR_TO_PLAYERS macro.
 *
 * Revision 1.146  2008/08/30 04:13:45  myc
 * Replaced the exp_to_level monstrosity with a lookup table that gets
 * populated at boot time.
 *
 * Revision 1.145  2008/08/30 01:31:51  myc
 * Changed the way stats are calculated in effect_total; ability
 * stats are saved in a raw form now, and only capped when accessed.
 * Damroll and hitroll are recalculated everytime effect_total
 * is called, using cached base values.
 *
 * Revision 1.144  2008/08/29 19:18:05  myc
 * Fixed abilities so that no information is lost; the caps occur
 * only when the viewed stats are accessed.
 *
 * Revision 1.143  2008/08/29 04:16:26  myc
 * Rewrote str_start/str_cat/str_catf and added several more
 * functions.  Also made it support up to 50 simultaneous
 * references to separate buffers.
 *
 * Revision 1.142  2008/08/26 04:39:21  jps
 * Changed IN_ZONE to IN_ZONE_RNUM or IN_ZONE_VNUM and fixed zone_printf.
 *
 * Revision 1.141  2008/08/25 00:20:33  myc
 * Changed the way mobs memorize spells.
 *
 * Revision 1.140  2008/08/14 23:10:35  myc
 * Added vararg functionality to log() and mudlog().  mprintf() is
 * the new vararg mudlog().  The old non-vararg mudlog() is still
 * available.  Added graduated log severity to the mudlog.
 *
 * Revision 1.139  2008/08/09 20:35:57  jps
 * Changed sense life so that it has a chance of detecting the presence and movement
 * of creatures with a "healable" life force. Increased spell duration to 17-50 hrs.
 *
 * Revision 1.138  2008/07/15 17:55:06  myc
 * Added accessor macros for grants and grant groups on the
 * player structure.
 *
 * Revision 1.137  2008/06/07 19:06:46  myc
 * Moved object-related constants and routines to objects.h.
 *
 * Revision 1.136  2008/06/05 02:07:43  myc
 * Replaced strip_cr with filter_chars/strip_chars.  Removed the
 * cost_per_day and spell component fields from the object
 * structure.  Changed object flags to use flagvectors.
 *
 * Revision 1.135  2008/05/18 04:42:09  jps
 * Code formatting change
 *
 * Revision 1.134  2008/05/17 22:03:01  jps
 * Moving room-related code into rooms.h and rooms.c.
 *
 * Revision 1.133  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.132  2008/04/19 21:11:22  myc
 * Added some general-purpose integer array sorting functions.
 * Right now, we've got bubble sort, insertion sort, quicksort,
 * and some apparently-optimized quicksort I found on Google :)
 *
 * Revision 1.131  2008/04/13 19:38:05  jps
 * Added macro for CONFUSED(ch).
 *
 * Revision 1.130  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.129  2008/04/04 06:12:52  myc
 * Removed justice and dieites/worship code.
 *
 * Revision 1.128  2008/04/03 17:33:57  jps
 * Added GET_AUTOINVIS macro.
 *
 * Revision 1.127  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.126  2008/04/02 19:31:02  myc
 * Added str_catf functions and used them in do_stat functions.
 *
 * Revision 1.125  2008/04/02 04:55:59  myc
 * Added a parse money function.
 *
 * Revision 1.124  2008/04/02 03:24:44  myc
 * Rewrote group code and removed major group code.
 *
 * Revision 1.123  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.122  2008/03/23 00:25:02  jps
 * Add a function to format applies, since it's done in at least
 * three places in the server.
 *
 * Revision 1.121  2008/03/22 19:08:19  jps
 * Added parse_obj_name as a generalized object identifier.
 * Added macros to get a character's life force and composition.
 *
 * Revision 1.120  2008/03/21 21:36:02  jps
 * Add functions without_article and pluralize, for modifying
 * nouns and noun phrases.
 *
 * Revision 1.119  2008/03/21 15:01:17  myc
 * Removed languages.
 *
 * Revision 1.118  2008/03/17 15:31:27  myc
 * Fix WAIT_STATE macro so it can be used properly as a single statement.
 *
 * Revision 1.117  2008/03/16 00:19:11  jps
 * Moved GET_TROPHY macro to trophy.h.
 *
 * Revision 1.116  2008/03/11 19:50:55  myc
 * Changed the way allowed olc zones are saved on an immortal from
 * a fixed number of slots to a variable-length linked list.  Also
 * got rid of practice points.
 *
 * Revision 1.115  2008/03/11 02:13:05  jps
 * Moving size macro to chars.h.
 *
 * Revision 1.114  2008/03/10 20:49:47  myc
 * Renamed POS1 to 'stance'.  Renamed hometown to homeroom.
 *
 * Revision 1.113  2008/03/10 19:55:37  jps
 * Made a struct for sizes with name, height, and weight.  Save base height
 * weight and size so they stay the same over size changes.
 *
 * Revision 1.112  2008/03/09 18:14:50  jps
 * Added defs for CAN_SEE_MOVING and SEES_THROUGH_MISDIRECTION.
 *
 * Revision 1.111  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.110  2008/03/08 23:55:21  jps
 * Added MOB_PERFORMS_SCRIPTS macro, which determines whether a mob can
 * do specprocs or triggers.
 *
 * Revision 1.109  2008/03/07 21:21:57  myc
 * Replaced action delays and skill delays with a single list of
 * 'cooldowns', which are decremented by a recurring event and
 * also save to the player file.
 *
 * Revision 1.108  2008/03/06 05:11:51  myc
 * Combined the 'saved' and 'unsaved' portions of the char_specials and
 * player_specials structures by moving all fields of each saved structure
 * to its parent structure.  Also combined the skills array from the
 * player and mob structures since they are identical.
 *
 * Revision 1.107  2008/03/06 04:35:02  myc
 * Moved the IS_VICIOUS macro here from structs.h.
 *
 * Revision 1.106  2008/03/05 05:21:56  myc
 * Made bank coins into ints instead of longs.
 *
 * Revision 1.105  2008/03/05 03:03:54  myc
 * Added sprintascii function.  Moved get_filename to players.c.  Got
 * rid of BOUNDED.  Added strip_cr and trim_spaces.
 *
 * Revision 1.104  2008/02/24 17:31:13  myc
 * Added a TO_OLC flag to act() to allow messages to be sent to
 * people while in OLC if they have OLCComm toggled on.
 *
 * Revision 1.103  2008/02/23 01:03:54  myc
 * Moving some spell circle and memorization functions from here to
 * spells.h.
 *
 * Revision 1.102  2008/02/16 20:31:32  myc
 * Commented out str_dup to help disambiguate memory leaks.
 *
 * Revision 1.101  2008/02/09 21:07:50  myc
 * Memming uses event flag instead of plr flag now.
 *
 * Revision 1.100  2008/02/09 18:29:11  myc
 * Camping and tracking now use event flags instead of having
 * their own event fields on the char_data struct..
 *
 * Revision 1.99  2008/02/09 07:05:37  myc
 * Adding an IS_PLAYING macro, which returns true if a descriptor
 * is playing or is in an OLC-type editor.
 *
 * Revision 1.98  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.97  2008/02/09 03:06:17  myc
 * Moving mathematical functions to math.c, and including math.h
 * everywhere utils.h is included.
 *
 * Revision 1.96  2008/02/07 01:46:14  myc
 * Removing the size abbrevs array and renaming SIZE_ABBR to SIZE_DESC,
 * which points to the sizes array.
 *
 * Revision 1.95  2008/02/06 21:53:53  myc
 * Adding a function to count color characters in a string (useful
 * for changing string widths in sprintf statements).
 *
 * Revision 1.94  2008/02/02 19:38:20  myc
 * Adding a levenshtein distance calculator for use by the
 * interpreter.  Also added a macro for accessing permanent
 * player titles.
 *
 * Revision 1.93  2008/01/30 19:20:57  myc
 * Adding support for the array bitvectors from newer versions of
 * Circle.  However, unlike stock circle, they are named FLAGs instead
 * of BIT_ARs.  So we have IS_FLAGGED instead of IS_SET, SET_FLAG for
 * SET_BIT, REMOVE_FLAG instead of REMOVE_BIT, and TOGGLE_FLAG instead
 * of TOGGLE_BIT.  The new event flags use these array bitvectors.
 *
 * Revision 1.92  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.91  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species into races.
 *
 * Revision 1.90  2008/01/27 12:11:58  jps
 * Fix the CHECK_WAIT macro. Learn to write macros!
 * Moved some IS_CLASS macros to class.h, deleted some unused ones.
 *
 * Revision 1.89  2008/01/27 09:45:41  jps
 * Got rid of the MCLASS_ defines and we now have a single set of classes
 * for both players and mobiles.
 *
 * Revision 1.88  2008/01/27 02:25:25  jps
 * Adjust CHECK_WAIT for mobs.
 *
 * Revision 1.87  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.86  2008/01/25 21:12:04  myc
 * Renamed monk_weight_pen to monk_weight_penalty.
 *
 * Revision 1.85  2008/01/17 01:29:10  myc
 * I'm pretty sure the mud only has 16 months...
 *
 * Revision 1.84  2008/01/15 06:51:47  myc
 * Added paranthesis to REAL_CHAR to make sure nothing goes awry.
 *
 * Revision 1.83  2008/01/15 03:18:19  myc
 * Changed SENSE_LIFE to its former function (showing any hidden mobs)
 * instead of adding some amount to perception.  That way it doesn't
 * apply to objects.
 *
 * Revision 1.82  2008/01/14 21:28:21  myc
 * Added GET_AGGR_LEV macro.
 *
 * Revision 1.81  2008/01/13 23:06:04  myc
 * Updated CAN_GET_OBJ macro to check the object's level.
 *
 * Revision 1.80  2008/01/13 03:19:53  myc
 * Removed GET_MSKILL and SET_MSKILL macros.
 *
 * Revision 1.79  2008/01/09 10:08:33  jps
 * Added CAN_SEE_BY_INFRA macro.
 *
 * Revision 1.78  2008/01/09 08:33:38  jps
 * Add functions to format strings for printin lengths and weights.
 *
 * Revision 1.77  2008/01/09 04:13:59  jps
 * New macro MEMMING for players who are memorizing.
 *
 * Revision 1.76  2008/01/09 02:29:33  jps
 * Modify GET_MOB_RNUM, real num moved to mob_specials.
 *
 * Revision 1.75  2008/01/09 01:51:25  jps
 * Get rid of obsolete defs for points and damage events.
 *
 * Revision 1.74  2008/01/05 21:55:09  jps
 * Added circular-dependency prevention defs.
 *
 * Revision 1.73  2008/01/05 05:39:03  jps
 * Removing function prototype advance_level() which is in class.c, not utils.c.
 *
 * Revision 1.72  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.71  2008/01/03 12:45:43  jps
 * New string function with_indefinite_article.
 * Renamed CLASS_MAGIC_USER to CLASS_SORCERER.
 *
 * Revision 1.70  2007/12/25 06:37:05  jps
 * Fix CURRENT_LOAD for highly loaded people (e.g., approaching max int).
 *
 * Revision 1.69  2007/12/20 23:13:03  myc
 * Cleaned up the CURRENT_LOAD macro.
 *
 * Revision 1.68  2007/12/19 20:58:40  myc
 * Renamed CLOAKED toggle to ROOMVIS.  Added const modifiers to str_cmp,
 * strn_cmp, log, touch, and mudlog.
 *
 * Revision 1.67  2007/10/27 03:18:58  myc
 * Fixed bug in CAN_SEE so mobs can see without lights.  Removed MCAN_SEE
 * since it does the same thing as CAN_SEE.
 *
 * Revision 1.66  2007/10/25 20:41:13  myc
 * Added WEAPON_AVERAGE macro and fixed typo in IS_WEAPON_BLUDGEONING.
 *
 * Revision 1.65  2007/10/02 02:52:27  myc
 * Renamed ORIG_CHAR as REAL_CHAR.  PRF_FLAGGED now uses REAL_CHAR,
 * so the preferences on the original player are always checked, even
 * when someone is switched or shapechanged.
 *
 * Revision 1.64  2007/09/28 20:49:35  myc
 * Added find_zone prototype.  Fixing a bug in HIDDEN_OK_OBJ that was
 * preventing objects from being seen.
 *
 * Revision 1.63  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  Rewrote CAN_SEE and CAN_SEE_OBJ
 * to handle this.  Also added cloaked toggle so people in the room can
 * see you even when everyone else can't (due to wizinvis).  Got rid of
 * all those racial stat defines and moved them to an array in db.c.
 *
 * Revision 1.62  2007/09/15 05:03:46  myc
 * Added MOB2 flags, which are saved as an espec in the mob files. The
 * MOB2_FLAGS and MOB2_FLAGGED macros are now available.
 *
 * Revision 1.61  2007/09/12 19:23:04  myc
 * Fix to IS_DARK to make city rooms always lit.
 *
 * Revision 1.60  2007/09/04 06:49:19  myc
 * Cleaned up IS_DARK macro.
 * IN_ZONE is now an rnum.
 *
 * Revision 1.59  2007/09/03 23:59:43  jps
 * Added macro ADDED_WEIGHT_OK for testing whether a char can have an
 * object added to its inventory.  Avoids an integer overflow problem
 * that could occur if an object's weight was near maxint.
 *
 * Revision 1.58  2007/08/16 11:53:39  jps
 * Remove various defunct specprocs.
 *
 * Revision 1.57  2007/08/14 22:43:07  myc
 * Adding conceal, corner, shadow, and stealth skills.
 *
 * Revision 1.56  2007/08/04 22:20:20  jps
 * Added some macros for rooms - IS_WATER, IS_SPLASHY
 *
 * Revision 1.55  2007/08/02 01:04:10  myc
 * check_pk() now works for all PK cases.  Moved from magic.c to fight.c
 *
 * Revision 1.54  2007/07/31 23:44:36  jps
 * New macros IS_HUNGRY, IS_THIRSTY, IS_DRUNK.
 *
 * Revision 1.53  2007/07/25 00:38:03  jps
 * Add macro IN_ZONE, like IN_ROOM only zonier.
 *
 * Revision 1.52  2007/07/19 21:59:36  jps
 * Add utility function next_line.
 *
 * Revision 1.51  2007/07/18 18:29:55  jps
 * Allow SIZE_ABBR to use GET_SIZE for NPCs.
 *
 * Revision 1.50  2007/07/15 17:16:12  jps
 * Add IS_POISONED macro, and moved HIGHLY_VISIBLE macro to utils.h
 *
 * Revision 1.49  2007/04/19 04:50:18  myc
 * Created macros for checking weapon types.
 *
 * Revision 1.48  2007/04/18 00:05:59  myc
 * Prompt parser has been totally rewritten so it won't print garbage
 * characters anymore.  Also, some new features were added.  Giving the
 * prompt command back to mortals.
 *
 * Revision 1.47  2007/04/04 13:31:02  jps
 * Add year to log timestamps and other dates.
 *
 * Revision 1.46  2007/03/27 04:27:05  myc
 * Changed spellings of innate timer macro.
 *
 * Revision 1.45  2007/02/04 18:12:31  myc
 * Page length now saves as a part of player specials.
 *
 * Revision 1.44  2006/11/27 02:07:05  jps
 * Allow "look in" to work like "look at" for ALL gate-spell objects.
 *
 * Revision 1.43  2006/11/26 08:31:17  jps
 * Added function yesno_result to standardize handling of nanny's
 * yes/no questions (in interpreter.c).
 *
 * Revision 1.42  2006/11/18 09:08:15  jps
 * Add function statemoney to pretty-print coins
 *
 * Revision 1.41  2006/11/18 07:22:34  jps
 * Add isplural function
 *
 * Revision 1.40  2006/11/18 04:26:32  jps
 * Renamed continual light spell to illumination, and it only works on
 * LIGHT items (still rooms too).
 *
 * Revision 1.39  2006/11/11 16:13:08  jps
 * Fix CAP so it correctly capitalizes strings with color codes at the beginning.
 *
 * Revision 1.38  2004/11/11 19:17:21  cmc
 * added prototype for perform_random_gem_drop()
 * warning message for fight.c no longer generated
 *
 * Revision 1.37  2003/06/23 01:47:09  jjl
 * Added a NOFOLLOW flag, and the "note" command, and show notes <player>
 *
 * Revision 1.36  2003/04/16 02:05:41  jjl
 * SKILL_DELAY addition.
 *
 * Revision 1.35  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.34  2001/12/07 15:42:12  dce
 * Fixed a duplicate entry.
 *
 * Revision 1.33  2001/04/08 17:13:10  dce
 * Added an alwayslit flag that makes a room lit no matter
 * of the sector or room type...
 *
 * Revision 1.32  2001/03/24 05:12:01  dce
 * Objects will now accept a level through olc and upon
 * booting the objects. The level code for the players will
 * follow.
 *
 * Revision 1.31  2000/11/25 02:33:15  rsd
 * Altered comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.30  2000/10/27 00:34:45  mtp
 * extra define for saving quests info
 *
 * Revision 1.29  2000/05/18 04:29:39  rsd
 * altered some of the racial balances of attributes.
 * Halflevs were less charismatic than humans? boggle
 *
 * Revision 1.28  2000/04/22 22:45:04  rsd
 * fixed comment header while browsing the file
 *
 * Revision 1.27  2000/02/24 01:07:44  dce
 * Gods no longer have a WAIT_STATE time. It is set to 0.
 *
 * Revision 1.26  2000/01/31 00:26:25  rsd
 * altered the define for PLR_CORPSE to make it so corpses
 * get defined properly.
 *
 * Revision 1.25  1999/11/29 00:13:41  cso
 * added define for GET_SHORT (from dg_scripts)
 * added defines for corpse types (CORPSE_PC, _NPC, _NPC_NORAISE)
 * changed define of IS_PLR_CORPSE
 * changed define of IS_MAGIC_USER to check for npc m-u
 * changed define of IS_THIEF to check for npc thief
 *
 * Revision 1.24  1999/10/30 16:16:48  rsd
 * Jimmy coded alignment restrictions for Paladins and exp
 * altered the definition of gain_exp() to include a check
 * for the victim.
 *
 * Revision 1.23  1999/09/16 01:15:11  dce
 * Weight restrictions for monks...-hitroll, -damroll + ac
 *
 * Revision 1.22  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.21  1999/08/12 17:54:46  dce
 * Fixed experience so that there are no overflows of integers that are placed into longs.
 * Main problem was max_exp_gain and max_exp_loss. Both were overflowing due to poor
 * Hubis coding.
 *
 * Revision 1.20  1999/08/12 04:25:39  jimmy
 * This is a Mass ci of the new pfile system.  The pfile has been split into
 * one file for each player in a directory A-Z.  The object files are also
 * located in the A-Z directories.  Fixed a stupid bug in pfilemaint that
 * screwed up the IDNUM of the person who typed it.  Commented out the frag
 * system completely.  It is slated for removal.  Fixed the rename command.
 * Fixed all supporting functions for the new system, I hope!
 * --Gurlaek 8/11/1999
 *
 * Revision 1.19  1999/08/08 19:37:25  mud
 * heh, well added additional check to the IS_PLR_CORPSE
 * macro to make doubly sure the item in question was in
 * fact a corpse and not any object with the second byte
 * set to 1. *whistle*
 *
 * Revision 1.18  1999/08/07 23:51:29  mud
 * removed some double defines of IS_CORPSE and added the
 * define for IS_PLR_CORPSE to reference the exiding
 * container structure.
 *
 * Revision 1.17  1999/07/24 20:50:18  dce
 * Exchange command for banks added.
 *
 * Revision 1.16  1999/07/06 19:57:05  jimmy
 * This is a Mass check-in of the new skill/spell/language assignment system.
 * This New system combines the assignment of skill/spell/language for
 * both mobs and PCs.  LOts of code was touched and many errors were fixed.
 * MCLASS_VOID was moved from 13 to -1 to match CLASS_UNDEFINED for PC's.
 * MObs now get random skill/spell/language levels baseed on their race/class/level
 * that exactly align with PC's.  PC's no longer have to rent to use skills gained
 * by leveling or when first creating a char.  Languages no longer reset to defaults
 * when a PC levels.  Discovered that languages have been defined right in the middle
 * of the spell area.  This needs to be fixed.  A conversion util neeDs to be run on
 * the mob files to compensate for the 13 to -1 class change.
 * --gurlaek 7/6/1999
 *
 * Revision 1.15  1999/06/30 18:25:04  jimmy
 * >> This is a major conversion from the 18 point attribute system to the
 * >> 100 point attribute system.  A few of the major changes are:
 * >> All attributes are now on a scale from 0-100
 * >> Everyone views attribs the same but, the attribs for one race
 * >>   may be differeent for that of another even if they are the
 * >>   same number.
 * >> Mobs attribs now get rolled and scaled using the same algorithim as PC's
 * >> Mobs now have individual random attributes based on race/class.
 * >> The STR_ADD attrib has been completely removed.
 * >> All bonus tables for attribs in constants.c have been replaced by
 * >>   algorithims that closely duplicate the tables except on a 100 scale.
 * >> Some minor changes:
 * >> Race selection at char creation can now be toggled by using
 * >>   <world races off>
 * >> Lots of cleanup done to affected areas of code.
 * >> Setting attributes for mobs in the .mob file no longer functions
 * >>   but is still in the code for later use.
 * >> We now have a spare attribut structure in the pfile because the new
 * >>   system only used three instead of four.
 * >> --gurlaek 6/30/1999
 *
 * Revision 1.14  1999/06/21 19:47:11  jimmy
 * Changed PLR_FLAGGED to do a ? between IS_NPC and IS_SET instead of an &&
 * This was done to prevent the crashes caused by mobs losing the
 * player_special=dummy_mob allocation.  --Gurlaek
 *
 * Revision 1.13  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list.  This code compiles fine under both gcc RH5.2 and egcs RH6.0
 *
 * Revision 1.12  1999/05/04 17:19:33  dce
 * Name accept system...version one...original code by Fingh, fixed up to work
 * by Zantir.
 *
 * Revision 1.11  1999/03/24 23:43:16  jimmy
 * Working on quest spells.  Still in progress.  HOwever, skills[] array now has a flag
 * quest.  If it's true then it's considerd a quest spell.  Also, allowed pyro/cryo's to
 * learn from any sorcerer type teacher
 * fingon
 *
 * Revision 1.10  1999/02/12 20:53:28  dce
 * Now only Sect_City & Room_Indoors will be lit, all other types
 * of Room_indoors and Sect's will be dark.
 *
 * Revision 1.9  1999/02/11 16:44:23  dce
 * Fixes IS_DARK macro.
 *
 * Revision 1.8  1999/02/10 22:27:54  jimmy
 * Added do_wiztitle
 *
 * Revision 1.7  1999/02/10 05:57:14  jimmy
 * Added long description to player file.  Added AFK toggle.
 * removed NOAUCTION toggle.
 * fingon
 *
 * Revision 1.6  1999/02/08 23:01:47  jimmy
 * Fixed mortally wounded bug.  Mortally wounded
 * victims now die when they read -11.  Also,
 * no more "attempt to damage corpse"
 * fingon
 *
 * Revision 1.5  1999/02/06 00:40:36  jimmy
 * Major change to incorporate aliases into the pfile
 * moved alias structure from interpreter.h to structs.h
 * heavily modified alias code in interpreter.c
 * Jimmy Kincaid AKA fingon
 *
 * Revision 1.4  1999/02/05 07:47:42  jimmy
 * Added Poofs to the playerfile as well as 4 extra strings for
 * future use.  fingon
 *
 * Revision 1.3  1999/02/04 18:08:43  mud
 * indented comment header
 * dos2unix
 *
 * Revision 1.2  1999/02/01 08:21:13  jimmy
 * improved build counter
 *
 * Revision 1.1  1999/01/29 01:23:32  mud
 * Initial revision
 *
 ***************************************************************************/
