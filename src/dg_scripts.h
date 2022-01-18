/***************************************************************************
 *   File: scripts.h                                     Part of FieryMUD  *
 *  Usage: header file for script structures and contstants, and           *
 *         function prototypes for scripts.c                               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *                                                                         *
 * This code was received origonally from HubisMUD in 1998 and no lable or *
 * claim of ownership or copyright was made anywhere in the file.          *
 ***************************************************************************/

#ifndef __FIERY_DG_SCRIPTS_H
#define __FIERY_DG_SCRIPTS_H

#include "fight.h"
#include "structs.h"
#include "sysdep.h"

#define MOB_TRIGGER 0
#define OBJ_TRIGGER 1
#define WLD_TRIGGER 2

#define DG_NO_TRIG (1u << 6u) /* don't check act trigger   */

#define NUM_TRIG_TYPE_FLAGS 20

/* mob trigger types */
#define MTRIG_GLOBAL (1u << 0u)    /* check even if zone empty   */
#define MTRIG_RANDOM (1u << 1u)    /* checked randomly           */
#define MTRIG_COMMAND (1u << 2u)   /* character types a command  */
#define MTRIG_SPEECH (1u << 3u)    /* a char says a word/phrase  */
#define MTRIG_ACT (1u << 4u)       /* word or phrase sent to act */
#define MTRIG_DEATH (1u << 5u)     /* character dies             */
#define MTRIG_GREET (1u << 6u)     /* something enters room seen */
#define MTRIG_GREET_ALL (1u << 7u) /* anything enters room       */
#define MTRIG_ENTRY (1u << 8u)     /* the mob enters a room      */
#define MTRIG_RECEIVE (1u << 9u)   /* character is given obj     */
#define MTRIG_FIGHT (1u << 10u)    /* each pulse while fighting  */
#define MTRIG_HITPRCNT (1u << 11u) /* fighting and below some hp */
#define MTRIG_BRIBE (1u << 12u)    /* coins are given to mob     */
#define MTRIG_SPEECHTO (1u << 13u) /* ask/whisper/tell           */
#define MTRIG_LOAD (1u << 14u)     /* the mob is loaded          */
#define MTRIG_CAST (1u << 15u)     /* mob is target of cast      */
#define MTRIG_LEAVE (1u << 16u)    /* someone leaves room seen   */
#define MTRIG_DOOR (1u << 17u)     /* door manipulated in room   */
#define MTRIG_TIME (1u << 19u)     /* trigger based on game hour */

/* obj trigger types */
#define OTRIG_GLOBAL (1u << 0u)   /* unused                     */
#define OTRIG_RANDOM (1u << 1u)   /* checked randomly           */
#define OTRIG_COMMAND (1u << 2u)  /* character types a command  */
#define OTRIG_ATTACK (1u << 3u)   /* Trigger for weapons on attack */
#define OTRIG_DEFEND (1u << 4u)   /* Trigger for weapons on defense */
#define OTRIG_TIMER (1u << 5u)    /* item's timer expires       */
#define OTRIG_GET (1u << 6u)      /* item is picked up          */
#define OTRIG_DROP (1u << 7u)     /* character tries to drop obj */
#define OTRIG_GIVE (1u << 8u)     /* character tries to give obj */
#define OTRIG_WEAR (1u << 9u)     /* character tries to wear obj */
#define OTRIG_DEATH (1u << 10u)   /* character dies             */
#define OTRIG_REMOVE (1u << 11u)  /* character tries to remove obj */
#define OTRIG_LOAD (1u << 14u)    /* the object is loaded       */
#define OTRIG_CAST (1u << 15u)    /* object targeted by spell  */
#define OTRIG_LEAVE (1u << 16u)   /* some leaves room seen      */
#define OTRIG_CONSUME (1u << 18u) /* char tries to eat/drink obj */
#define OTRIG_TIME (1u << 19u)    /* trigger based on game hour */

/* wld trigger types */
#define WTRIG_GLOBAL (1u << 0u)    /* check even if zone empty   */
#define WTRIG_RANDOM (1u << 1u)    /* checked randomly           */
#define WTRIG_COMMAND (1u << 2u)   /* character types a command  */
#define WTRIG_SPEECH (1u << 3u)    /* a char says word/phrase    */
#define WTRIG_RESET (1u << 5u)     /* zone has been reset        */
#define WTRIG_PREENTRY (1u << 6u)  /* someone is about to enter */
#define WTRIG_DROP (1u << 7u)      /* something dropped in room  */
#define WTRIG_POSTENTRY (1u << 8u) /* someone has just entered */
#define WTRIG_CAST (1u << 15u)     /* spell cast in room */
#define WTRIG_LEAVE (1u << 16u)    /* character leaves the room */
#define WTRIG_DOOR (1u << 17u)     /* door manipulated in room  */
#define WTRIG_TIME (1u << 19u)     /* trigger based on game hour */

/* obj command trigger types */
#define OCMD_EQUIP (1u << 0u) /* obj must be in char's equip */
#define OCMD_INVEN (1u << 1u) /* obj must be in char's inv   */
#define OCMD_ROOM (1u << 2u)  /* obj must be in char's room  */

#define TRIG_NEW 0     /* trigger starts from top     */
#define TRIG_RESTART 1 /* trigger restarting          */

/*
 * These are slightly off of PULSE_MOBILE so
 * everything isnt happening at the same time
 */
#define PULSE_DG_SCRIPT (13 RL_SEC)

#define MAX_SCRIPT_DEPTH                                                                                               \
    10 /* maximum depth triggers can                                                                                   \
          recurse into each other */

/* one line of the trigger */
struct cmdlist_element {
    char *cmd; /* one line of a trigger */
    struct cmdlist_element *original;
    struct cmdlist_element *next;
};

struct trig_var_data {
    char *name;  /* name of variable  */
    char *value; /* value of variable */

    struct trig_var_data *next;
};

/* structure for triggers */
struct trig_data {
    int nr;                             /* trigger's rnum                  */
    byte attach_type;                   /* mob/obj/wld intentions          */
    byte data_type;                     /* type of game_data for trig      */
    char *name;                         /* name of trigger                 */
    long trigger_type;                  /* type of trigger (for bitvector) */
    struct cmdlist_element *cmdlist;    /* top of command list             */
    struct cmdlist_element *curr_state; /* ptr to current line of trigger  */
    int narg;                           /* numerical argument              */
    char *arglist;                      /* argument list                   */
    int depth;                          /* depth into nest ifs/whiles/etc  */
    int loops;                          /* loop iteration counter          */
    struct event *wait_event;           /* event to pause the trigger      */
    ubyte purged;                       /* trigger is set to be purged     */
    ubyte running;                      /* trigger is running              */
    int damdone;                        /* Amount of damage done by a *damage command */
    struct trig_var_data *var_list;     /* list of local vars for trigger  */

    struct trig_data *next;
    struct trig_data *next_in_world; /* next in the global trigger list */
};

/* a complete script (composed of several triggers) */
struct script_data {
    long types;                        /* bitvector of trigger types */
    struct trig_data *trig_list;       /* list of triggers           */
    struct trig_var_data *global_vars; /* list of global variables   */
    ubyte purged;                      /* script is set to be purged */

    struct script_data *next; /* used for purged_scripts    */
};

/* function prototypes for dg_scripts.c */
int find_real_zone_by_room(room_num vznum);
int real_zone(int zvnum);

/* function prototypes from triggers.c */
void act_mtrigger(struct char_data *ch, const char *str, const struct char_data *actor, const struct char_data *victim,
                  const struct obj_data *object, const struct obj_data *target, char *arg, char *arg2);
void speech_mtrigger(struct char_data *actor, char *str);
void speech_to_mtrigger(struct char_data *actor, struct char_data *ch, char *str);
void speech_wtrigger(struct char_data *actor, char *str);
int greet_mtrigger(struct char_data *actor, int dir);
int entry_mtrigger(struct char_data *ch, int destination);
int preentry_wtrigger(struct room_data *room, struct char_data *actor, int dir);
int postentry_wtrigger(struct char_data *actor, int dir);
int timer_otrigger(struct obj_data *obj);
int drop_otrigger(struct obj_data *obj, struct char_data *actor);
int get_otrigger(struct obj_data *obj, struct char_data *actor);
int drop_wtrigger(struct obj_data *obj, struct char_data *actor);
int give_otrigger(struct obj_data *obj, struct char_data *actor, struct char_data *victim);
int remove_otrigger(struct obj_data *obj, struct char_data *actor);
int receive_mtrigger(struct char_data *ch, struct char_data *actor, struct obj_data *obj);
void bribe_mtrigger(struct char_data *ch, struct char_data *actor, int *cPtr);
int wear_otrigger(struct obj_data *obj, struct char_data *actor, int where);
int command_mtrigger(struct char_data *actor, char *cmd, char *argument);
int command_otrigger(struct char_data *actor, char *cmd, char *argument);
int command_wtrigger(struct char_data *actor, char *cmd, char *argument);
int death_mtrigger(struct char_data *ch, struct char_data *actor);
int death_otrigger(struct char_data *ch);
void fight_mtrigger(struct char_data *ch);
void attack_otrigger(struct char_data *actor, struct char_data *victim, int dam);
void hitprcnt_mtrigger(struct char_data *ch);
void load_mtrigger(struct char_data *ch);
void load_otrigger(struct obj_data *obj);
int cast_mtrigger(struct char_data *actor, struct char_data *ch, int spellnum);
int cast_otrigger(struct char_data *actor, struct obj_data *obj, int spellnum);
int cast_wtrigger(struct char_data *actor, struct char_data *vict, struct obj_data *obj, int spellnum);

int leave_mtrigger(struct char_data *actor, int dir);
int leave_otrigger(struct room_data *room, struct char_data *actor, int dir);
int leave_wtrigger(struct room_data *room, struct char_data *actor, int dir);
int consume_otrigger(struct obj_data *obj, struct char_data *actor, int cmd);
int door_mtrigger(struct char_data *actor, int subcmd, int dir);
int door_wtrigger(struct char_data *actor, int subcmd, int dir);

void time_mtrigger(struct char_data *ch);
void time_otrigger(struct obj_data *obj);
void time_wtrigger(struct room_data *room);

void reset_wtrigger(struct room_data *room);

void random_mtrigger(struct char_data *ch);
void random_otrigger(struct obj_data *obj);
void random_wtrigger(struct room_data *ch);

/* function prototypes from scripts.c */
void script_trigger_check(void);
void add_trigger(struct script_data *sc, struct trig_data *t, int loc);

void do_stat_trigger(struct char_data *ch, struct trig_data *trig);
void do_sstat_room(struct char_data *ch, char *buf, struct room_data *rm);
void do_sstat_object(struct char_data *ch, char *buf, struct obj_data *j);
void do_sstat_character(struct char_data *ch, char *buf, struct char_data *k);

void script_log(struct trig_data *t, char *msg);
void dg_read_trigger(FILE *fp, void *i, int type);
void dg_obj_trigger(char *line, struct obj_data *obj);
void assign_triggers(void *i, int type);
struct trig_data *read_trigger(int nr);
void parse_trigger(FILE *trig_f, int nr);
int real_trigger(int vnum);
void extract_script(struct script_data *sc);
void fullpurge_char(struct char_data *ch);
void check_time_triggers(void);
void free_trigger(struct trig_data *trig);
void free_varlist(struct trig_var_data *vd);
void free_proto_script(struct trig_proto_list **list);
bool format_script(struct descriptor_data *d, int indent_quantum);

/* Macros for scripts */

#define UID_CHAR '\005'

#define GET_TRIG_NAME(t) ((t)->name)
#define GET_TRIG_RNUM(t) ((t)->nr)
#define GET_TRIG_VNUM(t) (trig_index[(t)->nr]->virtual)
#define GET_TRIG_TYPE(t) ((t)->trigger_type)
#define GET_TRIG_DATA_TYPE(t) ((t)->data_type)
#define GET_TRIG_NARG(t) ((t)->narg)
#define GET_TRIG_ARG(t) ((t)->arglist)
#define GET_TRIG_VARS(t) ((t)->var_list)
#define GET_TRIG_WAIT(t) ((t)->wait_event)
#define GET_TRIG_DEPTH(t) ((t)->depth)
#define GET_TRIG_LOOPS(t) ((t)->loops)

#define ROOM_ID_BASE 50000

#define SCRIPT(o) ((o)->script)

#define SCRIPT_TYPES(s) ((s)->types)
#define TRIGGERS(s) ((s)->trig_list)

#define SCRIPT_CHECK(go, type) (SCRIPT(go) && IS_SET(SCRIPT_TYPES(SCRIPT(go)), type))
#define TRIGGER_CHECK(t, type) (IS_SET(GET_TRIG_TYPE(t), type) && !GET_TRIG_DEPTH(t))

#define ADD_UID_VAR(buf, trig, go, name)                                                                               \
    {                                                                                                                  \
        sprintf(buf, "%c%ld", UID_CHAR, GET_ID(go));                                                                   \
        add_var(&GET_TRIG_VARS(trig), name, buf);                                                                      \
    }

/* typedefs that the dg functions rely on */
typedef struct index_data index_data;
typedef struct room_data room_data;
typedef struct obj_data obj_data;
typedef struct trig_data trig_data;
typedef struct char_data char_data;

#ifndef __DG_SCRIPTS_C__
extern struct trig_data *trigger_list;
#endif

#ifndef __DG_TRIGGERS_C__
extern const char *trig_types[];
extern const char *otrig_types[];
extern const char *wtrig_types[];

#endif

extern int script_driver(void *go_address, trig_data *trig, int type, int mode);

#endif