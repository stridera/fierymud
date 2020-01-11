/***************************************************************************
 * $Id: find.h,v 1.3 2009/03/19 23:16:23 myc Exp $
 ***************************************************************************/
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

#ifndef __FIERY_FIND_H
#define __FIERY_FIND_H

#include "sysdep.h"
#include "structs.h"

/* Find obj/char system */
#define MATCH_OBJ_FUNC(func) bool(func)(struct find_context * context, struct obj_data * obj)
#define MATCH_CHAR_FUNC(func) bool(func)(struct find_context * context, struct char_data * ch)
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
        type (*next_func)(struct prefix##_iterator *);                                                                 \
    }

DECLARE_ITERATOR_TYPE(obj, obj, struct obj_data *);
DECLARE_ITERATOR_TYPE(char, ch, struct char_data *);

#define NULL_ITER(defval)                                                                                              \
    { defval, defval, 0, 0, NULL, NULL_FCONTEXT, NULL }

#define next(iter) (((iter).next_func)(&(iter)))

extern int grab_number(char **name);

extern struct find_context find_vis(struct char_data *ch);
extern struct find_context find_by_name(char *name);
extern struct find_context find_vis_by_name(struct char_data *ch, char *name);
extern struct find_context find_by_rnum(int rnum);
extern struct find_context find_by_vnum(int vnum);
extern struct find_context find_vis_by_vnum(struct char_data *ch, int vnum);
extern struct find_context find_by_type(int type);
extern struct find_context find_vis_by_type(struct char_data *ch, int type);
extern struct find_context find_by_id(int id);
extern struct find_context find_vis_by_id(struct char_data *ch, int id);
extern struct find_context find_plr_by_name(char *name);
extern struct find_context find_vis_plr_by_name(struct char_data *ch, char *name);

struct obj_data *find_obj_in_list(struct obj_data *list, struct find_context context);
struct obj_data *find_obj_in_list_recur(struct obj_data *list, struct find_context context);
struct obj_data *find_obj_in_eq(struct char_data *ch, int *where, struct find_context context);
struct obj_data *find_obj_in_world(struct find_context context);
struct obj_data *find_obj_around_char(struct char_data *ch, struct find_context context);
struct obj_data *find_obj_around_obj(struct obj_data *obj, struct find_context context);
struct obj_data *find_obj_around_room(struct room_data *room, struct find_context context);
struct obj_data *find_obj_for_mtrig(struct char_data *ch, char *name);

struct char_data *find_char_in_room(struct room_data *room, struct find_context context);
struct char_data *find_char_in_world(struct find_context context);
struct char_data *find_char_by_desc(struct find_context context);
struct char_data *find_char_around_char(struct char_data *ch, struct find_context context);
struct char_data *find_char_around_obj(struct obj_data *obj, struct find_context context);
struct char_data *find_char_around_room(struct room_data *room, struct find_context context);
struct char_data *find_char_for_mtrig(struct char_data *ch, char *name);

struct obj_data *find_obj_for_keyword(struct obj_data *obj, const char *name);
struct char_data *find_char_for_keyword(struct char_data *ch, const char *name);

struct obj_iterator find_objs_in_list(struct obj_data *list, struct find_context context);

/* find all dots */

int find_all_dots(char **arg);

#define FIND_INDIV 0
#define FIND_ALL 1
#define FIND_ALLDOT 2

/* Generic Find */
int generic_find(char *arg, int bitvector, struct char_data *ch, struct char_data **tar_ch, struct obj_data **tar_obj);
int universal_find(struct find_context context, int bitvector, struct char_data **tch, struct obj_data **tobj);
struct obj_iterator find_objs(struct find_context context, int bitvector);
struct char_iterator find_chars(struct find_context context, int bitvector);

#define FIND_CHAR_ROOM (1 << 0)
#define FIND_CHAR_WORLD (1 << 1)
#define FIND_OBJ_INV (1 << 2)
#define FIND_OBJ_ROOM (1 << 3)
#define FIND_OBJ_WORLD (1 << 4)
#define FIND_OBJ_EQUIP (1 << 5)

/* Pick random char in same room as ch */
struct char_data *get_random_char_around(struct char_data *ch, int mode);
#define RAND_PLAYERS (1 << 0)
#define RAND_HASSLE (1 << 1)
#define RAND_VISIBLE (1 << 2)
#define RAND_NOT_SELF (1 << 3)
#define RAND_WIZ_VIS (1 << 4)

#define RAND_AGGRO (RAND_HASSLE | RAND_VISIBLE | RAND_NOT_SELF)
#define RAND_DG_MOB (RAND_AGGRO)
#define RAND_DG_OBJ (RAND_HASSLE | RAND_WIZ_VIS)
#define RAND_DG_WLD (RAND_HASSLE | RAND_WIZ_VIS)

#endif

/***************************************************************************
 * $Log: find.h,v $
 * Revision 1.3  2009/03/19 23:16:23  myc
 * Added find_vis() premade context and several new iterators.
 *
 * Revision 1.2  2009/03/09 02:22:32  myc
 * Added find_vis_by_vnum and find_vis_by_type find context factories.
 * Also added universal_find, which is just like generic_find, but
 * accepts a custom find context.
 *
 * Revision 1.1  2009/03/03 19:43:44  myc
 * Initial revision
 *
 ***************************************************************************/
