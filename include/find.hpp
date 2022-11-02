/***************************************************************************
 *   File: find.h                                         Part of FieryMUD *
 *  Usage: header file: prototypes of obj/char finding functions           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

/* Find obj/char system */
#define MATCH_OBJ_FUNC(func) bool(func)(find_context * context, obj_data * obj)
#define MATCH_CHAR_FUNC(func) bool(func)(find_context * context, char_data * ch)
#define OBJ_MATCH(ctx, obj) ((ctx).obj_func)(&(ctx), (obj))
#define CHAR_MATCH(ctx, ch) ((ctx).char_func)(&(ctx), (ch))

struct find_context {
    MATCH_OBJ_FUNC(*obj_func);
    MATCH_CHAR_FUNC(*char_func);

    struct char_data *ch;
    int number;
    char *string;

    bool override;
};

#define NULL_FCONTEXT                                                                                                  \
    { NULL, NULL, NULL, 0, NULL, FALSE }

#define DECLARE_ITERATOR_TYPE(prefix, var_name, type)                                                                  \
    struct prefix##_iterator {                                                                                         \
        type var_name; /* current obj */                                                                               \
        type next;     /* next to consider */                                                                          \
        int mode;                                                                                                      \
        int pos;                                                                                                       \
        void *data;                                                                                                    \
        struct find_context context;                                                                                   \
        type (*next_func)(prefix##_iterator *);                                                                        \
    }

DECLARE_ITERATOR_TYPE(obj, obj, obj_data *);
DECLARE_ITERATOR_TYPE(char, ch, char_data *);

#define NULL_ITER(defval)                                                                                              \
    { defval, defval, 0, 0, NULL, NULL_FCONTEXT, NULL }

#define next(iter) (((iter).next_func)(&(iter)))

extern int grab_number(char **name);

extern struct find_context find_vis(char_data *ch);
extern struct find_context find_by_name(char *name);
extern struct find_context find_vis_by_name(char_data *ch, char *name);
extern struct find_context find_by_rnum(int rnum);
extern struct find_context find_by_vnum(int vnum);
extern struct find_context find_vis_by_vnum(char_data *ch, int vnum);
extern struct find_context find_by_type(int type);
extern struct find_context find_vis_by_type(char_data *ch, int type);
extern struct find_context find_by_id(int id);
extern struct find_context find_vis_by_id(char_data *ch, int id);
extern struct find_context find_plr_by_name(char *name);
extern struct find_context find_vis_plr_by_name(char_data *ch, char *name);

struct obj_data *find_obj_in_list(obj_data *list, find_context context);
struct obj_data *find_obj_in_list_recur(obj_data *list, find_context context);
struct obj_data *find_obj_in_eq(char_data *ch, int *where, find_context context);
struct obj_data *find_obj_in_world(find_context context);
struct obj_data *find_obj_around_char(char_data *ch, find_context context);
struct obj_data *find_obj_around_obj(obj_data *obj, find_context context);
struct obj_data *find_obj_around_room(room_data *room, find_context context);
struct obj_data *find_obj_for_mtrig(char_data *ch, char *name);

struct char_data *find_char_in_room(room_data *room, find_context context);
struct char_data *find_char_in_world(find_context context);
struct char_data *find_char_by_desc(find_context context);
struct char_data *find_char_around_char(char_data *ch, find_context context);
struct char_data *find_char_around_obj(obj_data *obj, find_context context);
struct char_data *find_char_around_room(room_data *room, find_context context);
struct char_data *find_char_for_mtrig(char_data *ch, char *name);

struct obj_data *find_obj_for_keyword(obj_data *obj, const char *name);
struct char_data *find_char_for_keyword(char_data *ch, const char *name);

struct obj_iterator find_objs_in_list(obj_data *list, find_context context);

/* find all dots */

int find_all_dots(char **arg);

#define FIND_INDIV 0
#define FIND_ALL 1
#define FIND_ALLDOT 2

/* Generic Find */
int generic_find(char *arg, int bitvector, char_data *ch, char_data **tar_ch, obj_data **tar_obj);
int universal_find(find_context context, int bitvector, char_data **tch, obj_data **tobj);
struct obj_iterator find_objs(find_context context, int bitvector);
struct char_iterator find_chars(find_context context, int bitvector);

#define FIND_CHAR_ROOM (1 << 0)
#define FIND_CHAR_WORLD (1 << 1)
#define FIND_OBJ_INV (1 << 2)
#define FIND_OBJ_ROOM (1 << 3)
#define FIND_OBJ_WORLD (1 << 4)
#define FIND_OBJ_EQUIP (1 << 5)

/* Pick random char in same room as ch */
struct char_data *get_random_char_around(char_data *ch, int mode);
#define RAND_PLAYERS (1 << 0)
#define RAND_HASSLE (1 << 1)
#define RAND_VISIBLE (1 << 2)
#define RAND_NOT_SELF (1 << 3)
#define RAND_WIZ_VIS (1 << 4)

#define RAND_AGGRO (RAND_HASSLE | RAND_VISIBLE | RAND_NOT_SELF)
#define RAND_DG_MOB (RAND_AGGRO)
#define RAND_DG_OBJ (RAND_HASSLE | RAND_WIZ_VIS)
#define RAND_DG_WLD (RAND_HASSLE | RAND_WIZ_VIS)
