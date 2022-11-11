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

#include "rooms.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

/* Find obj/char system */
#define MATCH_OBJ_FUNC(func) bool(func)(FindContext * context, ObjData * obj)
#define MATCH_CHAR_FUNC(func) bool(func)(FindContext * context, CharData * ch)
#define OBJ_MATCH(ctx, obj) ((ctx).obj_func)(&(ctx), (obj))
#define CHAR_MATCH(ctx, ch) ((ctx).char_func)(&(ctx), (ch))

struct FindContext {
    MATCH_OBJ_FUNC(*obj_func);
    MATCH_CHAR_FUNC(*char_func);

    CharData *ch;
    int number;
    char *string;

    bool override;
};

#define NULL_FCONTEXT                                                                                                  \
    { nullptr, nullptr, nullptr, 0, nullptr, false }

#define DECLARE_ITERATOR_TYPE(prefix, var_name, type)                                                                  \
    struct prefix##_iterator {                                                                                         \
        type var_name; /* current obj */                                                                               \
        type next;     /* next to consider */                                                                          \
        int mode;                                                                                                      \
        int pos;                                                                                                       \
        void *data;                                                                                                    \
        FindContext context;                                                                                           \
        type (*next_func)(prefix##_iterator *);                                                                        \
    }

DECLARE_ITERATOR_TYPE(obj, obj, ObjData *);
DECLARE_ITERATOR_TYPE(char, ch, CharData *);

#define NULL_ITER(defval)                                                                                              \
    { defval, defval, 0, 0, nullptr, NULL_FCONTEXT, nullptr }

#define next(iter) (((iter).next_func)(&(iter)))

int grab_number(char **name);

FindContext find_vis(CharData *ch);
FindContext find_by_name(char *name);
FindContext find_vis_by_name(CharData *ch, char *name);
FindContext find_by_rnum(int rnum);
FindContext find_by_vnum(int vnum);
FindContext find_vis_by_vnum(CharData *ch, int vnum);
FindContext find_by_type(int type);
FindContext find_vis_by_type(CharData *ch, int type);
FindContext find_by_id(int id);
FindContext find_vis_by_id(CharData *ch, int id);
FindContext find_plr_by_name(char *name);
FindContext find_vis_plr_by_name(CharData *ch, char *name);

ObjData *find_obj_in_list(ObjData *list, FindContext context);
ObjData *find_obj_in_list_recur(ObjData *list, FindContext context);
ObjData *find_obj_in_eq(CharData *ch, int *where, FindContext context);
ObjData *find_obj_in_world(FindContext context);
ObjData *find_obj_around_char(CharData *ch, FindContext context);
ObjData *find_obj_around_obj(ObjData *obj, FindContext context);
ObjData *find_obj_around_room(RoomData *room, FindContext context);
ObjData *find_obj_for_mtrig(CharData *ch, char *name);

CharData *find_char_in_room(RoomData *room, FindContext context);
CharData *find_char_in_world(FindContext context);
CharData *find_char_by_desc(FindContext context);
CharData *find_char_around_char(CharData *ch, FindContext context);
CharData *find_char_around_obj(ObjData *obj, FindContext context);
CharData *find_char_around_room(RoomData *room, FindContext context);
CharData *find_char_for_mtrig(CharData *ch, char *name);

ObjData *find_obj_for_keyword(ObjData *obj, const char *name);
CharData *find_char_for_keyword(CharData *ch, const char *name);

obj_iterator find_objs_in_list(ObjData *list, FindContext context);

/* find all dots */

int find_all_dots(char **arg);

#define FIND_INDIV 0
#define FIND_ALL 1
#define FIND_ALLDOT 2

/* Generic Find */
int generic_find(char *arg, int bitvector, CharData *ch, CharData **tar_ch, ObjData **tar_obj);
int universal_find(FindContext context, int bitvector, CharData **tch, ObjData **tobj);
obj_iterator find_objs(FindContext context, int bitvector);
char_iterator find_chars(FindContext context, int bitvector);

#define FIND_CHAR_ROOM (1 << 0)
#define FIND_CHAR_WORLD (1 << 1)
#define FIND_OBJ_INV (1 << 2)
#define FIND_OBJ_ROOM (1 << 3)
#define FIND_OBJ_WORLD (1 << 4)
#define FIND_OBJ_EQUIP (1 << 5)

/* Pick random char in same room as ch */
CharData *get_random_char_around(CharData *ch, int mode);
#define RAND_PLAYERS (1 << 0)
#define RAND_HASSLE (1 << 1)
#define RAND_VISIBLE (1 << 2)
#define RAND_NOT_SELF (1 << 3)
#define RAND_WIZ_VIS (1 << 4)

#define RAND_AGGRO (RAND_HASSLE | RAND_VISIBLE | RAND_NOT_SELF)
#define RAND_DG_MOB (RAND_AGGRO)
#define RAND_DG_OBJ (RAND_HASSLE | RAND_WIZ_VIS)
#define RAND_DG_WLD (RAND_HASSLE | RAND_WIZ_VIS)
