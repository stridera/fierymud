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

#pragma once

#include "fight.hpp"
#include "rooms.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

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
#define MTRIG_LOOK (1u << 18u)     /* the mob is looked at       */
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
#define OTRIG_LOOK (1u << 12u)    /* object is looked at        */
#define OTRIG_USE (1u << 13u)     /* object is used             */
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
struct CmdlistElement {
    char *cmd; /* one line of a trigger */
    CmdlistElement *original;
    CmdlistElement *next;
};

struct TriggerVariableData {
    char *name;  /* name of variable  */
    char *value; /* value of variable */

    TriggerVariableData *next;
};

/* structure for triggers */
struct TrigData {
    int nr;                        /* trigger's rnum                  */
    byte attach_type;              /* mob/obj/wld intentions          */
    byte data_type;                /* type of game_data for trig      */
    char *name;                    /* name of trigger                 */
    long trigger_type;             /* type of trigger (for bitvector) */
    CmdlistElement *cmdlist;       /* top of command list             */
    CmdlistElement *curr_state;    /* ptr to current line of trigger  */
    int narg;                      /* numerical argument              */
    char *arglist;                 /* argument list                   */
    int depth;                     /* depth into nest ifs/whiles/etc  */
    int loops;                     /* loop iteration counter          */
    Event *wait_event;             /* event to pause the trigger      */
    ubyte purged;                  /* trigger is set to be purged     */
    ubyte running;                 /* trigger is running              */
    int damdone;                   /* Amount of damage done by a *damage command */
    TriggerVariableData *var_list; /* list of local vars for trigger  */

    TrigData *next;
    TrigData *next_in_world; /* next in the global trigger list */
};
extern TrigData *trigger_list;

/* a complete script (composed of several triggers) */
struct ScriptData {
    long types;                       /* bitvector of trigger types */
    TrigData *trig_list;              /* list of triggers           */
    TriggerVariableData *global_vars; /* list of global variables   */
    ubyte purged;                     /* script is set to be purged */

    ScriptData *next; /* used for purged_scripts    */
};

/* function prototypes for dg_scripts.c */
int find_real_zone_by_room(room_num vznum);
int real_zone(int zvnum);

/* function prototypes from triggers.c */
void act_mtrigger(const CharData *ch, const char *str, const CharData *actor, const CharData *victim,
                  const ObjData *obj, const ObjData *target, char *arg, char *arg2);
void speech_mtrigger(CharData *actor, const char *str);
void speech_to_mtrigger(CharData *actor, CharData *ch, const char *str);
void speech_wtrigger(CharData *actor, const char *str);
int greet_mtrigger(CharData *actor, int dir);
int entry_mtrigger(CharData *ch, int destination);
int preentry_wtrigger(RoomData *room, CharData *actor, int dir);
int postentry_wtrigger(CharData *actor, int dir);
int timer_otrigger(ObjData *obj);
int drop_otrigger(ObjData *obj, CharData *actor, ObjData *cont);
int get_otrigger(ObjData *obj, CharData *actor, ObjData *cont);
int drop_wtrigger(ObjData *obj, CharData *actor);
int give_otrigger(ObjData *obj, CharData *actor, CharData *victim);
int remove_otrigger(ObjData *obj, CharData *actor);
int receive_mtrigger(CharData *ch, CharData *actor, ObjData *obj);
void bribe_mtrigger(CharData *ch, CharData *actor, Money cPtr);
int wear_otrigger(ObjData *obj, CharData *actor, int where);
int command_mtrigger(CharData *actor, char *cmd, char *argument);
int command_otrigger(CharData *actor, char *cmd, char *argument);
int command_wtrigger(CharData *actor, char *cmd, char *argument);
int death_mtrigger(CharData *ch, CharData *actor);
int death_otrigger(CharData *ch);
void fight_mtrigger(CharData *ch);
void attack_otrigger(CharData *actor, CharData *victim, int dam);
void hitprcnt_mtrigger(CharData *ch);
void load_mtrigger(CharData *ch);
void load_otrigger(ObjData *obj);
int cast_mtrigger(CharData *actor, CharData *ch, int spellnum);
int cast_otrigger(CharData *actor, ObjData *obj, int spellnum);
int cast_wtrigger(CharData *actor, CharData *vict, ObjData *obj, int spellnum);

int leave_mtrigger(CharData *actor, int dir);
int leave_otrigger(RoomData *room, CharData *actor, int dir);
int leave_wtrigger(RoomData *room, CharData *actor, int dir);
int consume_otrigger(ObjData *obj, CharData *actor, int cmd);
int door_mtrigger(CharData *actor, int subcmd, int dir);
int door_wtrigger(CharData *actor, int subcmd, int dir);

void time_mtrigger(CharData *ch);
void time_otrigger(ObjData *obj);
void time_wtrigger(RoomData *room);

int look_otrigger(ObjData *obj, CharData *actor, char *arg, const char *additional_args);
int look_mtrigger(CharData *ch, CharData *actor, const char *arg);

int use_otrigger(ObjData *obj, ObjData *tobj, CharData *actor, CharData *victim);

void reset_wtrigger(RoomData *room);

void random_mtrigger(CharData *ch);
void random_otrigger(ObjData *obj);
void random_wtrigger(RoomData *ch);

/* function prototypes from scripts.c */
void script_trigger_check(void);
void add_trigger(ScriptData *sc, TrigData *t, int loc);

void do_stat_trigger(CharData *ch, TrigData *trig);
void do_sstat_room(CharData *ch, char *buf, RoomData *rm);
void do_sstat_object(CharData *ch, char *buf, ObjData *j);
void do_sstat_character(CharData *ch, char *buf, CharData *k);

void script_log(TrigData *t, char *msg);
void dg_read_trigger(FILE *fp, void *i, int type);
void dg_obj_trigger(char *line, ObjData *obj);
void assign_triggers(void *i, int type);
TrigData *read_trigger(int nr);
void parse_trigger(FILE *trig_f, int nr);
int real_trigger(int vnum);
void extract_script(ScriptData *sc);
void fullpurge_char(CharData *ch);
void check_time_triggers(void);
void free_trigger(TrigData *trig);
void free_varlist(TriggerVariableData *vd);
void free_proto_script(TriggerPrototypeList **list);
bool format_script(DescriptorData *d, int indent_quantum);

void add_var(TriggerVariableData **var_list, const char *name, const char *value);
int remove_var(TriggerVariableData **var_list, const char *name);

/* Macros for scripts */

#define UID_CHAR '\005'

#define GET_TRIG_NAME(t) ((t)->name)
#define GET_TRIG_RNUM(t) ((t)->nr)
#define GET_TRIG_VNUM(t) (trig_index[(t)->nr]->vnum)
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
// typedef struct index_data index_data;
// typedef struct room_data room_data;
// typedef obj_data ObjData;
// typedef struct trig_data trig_data;
// typedef char_data CharData;

int script_driver(void *go_address, TrigData *trig, int type, int mode);

/* mob trigger types */
constexpr std::string_view trig_types[] = {"Global",    "Random", "Command", "Speech", "Act",      "Death", "Greet",
                                           "Greet-All", "Entry",  "Receive", "Fight",  "HitPrcnt", "Bribe", "SpeechTo*",
                                           "Load",      "Cast",   "Leave",   "Door",   "Look",     "Time",  "\n"};

/* obj trigger types */
constexpr std::string_view otrig_types[] = {"Global", "Random", "Command", "Attack", "Defense", "Timer", "Get",
                                            "Drop",   "Give",   "Wear",    "Death",  "Remove",  "Look",  "Use",
                                            "Load",   "Cast",   "Leave",   "UNUSED", "Consume", "Time",  "\n"};

/* wld trigger types */
constexpr std::string_view wtrig_types[] = {"Global", "Random",    "Command", "Speech", "UNUSED", "Reset",  "Preentry",
                                            "Drop",   "Postentry", "UNUSED",  "UNUSED", "UNUSED", "UNUSED", "UNUSED",
                                            "UNUSED", "Cast",      "Leave",   "Door",   "UNUSED", "Time",   "\n"};
