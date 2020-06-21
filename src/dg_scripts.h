/***************************************************************************
 * $Id: dg_scripts.h,v 1.34 2009/03/07 09:34:47 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: scripts.h                                     Part of FieryMUD  *
 *  Usage: header file for script structures and contstants, and           *
 *         function prototypes for scripts.c                               *
 *  $Author: jps $                                                         *
 *  $Date: 2009/03/07 09:34:47 $                                           *
 *  $Revision: 1.34 $                                                       *
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

#include "sysdep.h"
#include "structs.h"
#include "fight.h"

#define MOB_TRIGGER 0
#define OBJ_TRIGGER 1
#define WLD_TRIGGER 2

#define DG_NO_TRIG (1 << 6) /* don't check act trigger   */

#define NUM_TRIG_TYPE_FLAGS 20

/* mob trigger types */
#define MTRIG_GLOBAL (1 << 0)    /* check even if zone empty   */
#define MTRIG_RANDOM (1 << 1)    /* checked randomly           */
#define MTRIG_COMMAND (1 << 2)   /* character types a command  */
#define MTRIG_SPEECH (1 << 3)    /* a char says a word/phrase  */
#define MTRIG_ACT (1 << 4)       /* word or phrase sent to act */
#define MTRIG_DEATH (1 << 5)     /* character dies             */
#define MTRIG_GREET (1 << 6)     /* something enters room seen */
#define MTRIG_GREET_ALL (1 << 7) /* anything enters room       */
#define MTRIG_ENTRY (1 << 8)     /* the mob enters a room      */
#define MTRIG_RECEIVE (1 << 9)   /* character is given obj     */
#define MTRIG_FIGHT (1 << 10)    /* each pulse while fighting  */
#define MTRIG_HITPRCNT (1 << 11) /* fighting and below some hp */
#define MTRIG_BRIBE (1 << 12)    /* coins are given to mob     */
#define MTRIG_SPEECHTO (1 << 13) /* ask/whisper/tell           */
#define MTRIG_LOAD (1 << 14)     /* the mob is loaded          */
#define MTRIG_CAST (1 << 15)     /* mob is target of cast      */
#define MTRIG_LEAVE (1 << 16)    /* someone leaves room seen   */
#define MTRIG_DOOR (1 << 17)     /* door manipulated in room   */
#define MTRIG_TIME (1 << 19)     /* trigger based on game hour */

/* obj trigger types */
#define OTRIG_GLOBAL (1 << 0)   /* unused                     */
#define OTRIG_RANDOM (1 << 1)   /* checked randomly           */
#define OTRIG_COMMAND (1 << 2)  /* character types a command  */
#define OTRIG_ATTACK (1 << 3)  /* Trigger for weapons on attack */
#define OTRIG_DEFEND (1 << 4)  /* Trigger for weapons on defense */
#define OTRIG_TIMER (1 << 5)    /* item's timer expires       */
#define OTRIG_GET (1 << 6)      /* item is picked up          */
#define OTRIG_DROP (1 << 7)     /* character tries to drop obj */
#define OTRIG_GIVE (1 << 8)     /* character tries to give obj */
#define OTRIG_WEAR (1 << 9)     /* character tries to wear obj */
#define OTRIG_DEATH (1 << 10)   /* character dies             */
#define OTRIG_REMOVE (1 << 11)  /* character tries to remove obj */
#define OTRIG_LOAD (1 << 14)    /* the object is loaded       */
#define OTRIG_CAST (1 << 15)    /* object targeted by spell  */
#define OTRIG_LEAVE (1 << 16)   /* some leaves room seen      */
#define OTRIG_CONSUME (1 << 18) /* char tries to eat/drink obj */
#define OTRIG_TIME (1 << 19)    /* trigger based on game hour */

/* wld trigger types */
#define WTRIG_GLOBAL (1 << 0)    /* check even if zone empty   */
#define WTRIG_RANDOM (1 << 1)    /* checked randomly           */
#define WTRIG_COMMAND (1 << 2)   /* character types a command  */
#define WTRIG_SPEECH (1 << 3)    /* a char says word/phrase    */
#define WTRIG_RESET (1 << 5)     /* zone has been reset        */
#define WTRIG_PREENTRY (1 << 6)  /* someone is about to enter */
#define WTRIG_DROP (1 << 7)      /* something dropped in room  */
#define WTRIG_POSTENTRY (1 << 8) /* someone has just entered */
#define WTRIG_CAST (1 << 15)     /* spell cast in room */
#define WTRIG_LEAVE (1 << 16)    /* character leaves the room */
#define WTRIG_DOOR (1 << 17)     /* door manipulated in room  */
#define WTRIG_TIME (1 << 19)     /* trigger based on game hour */

/* obj command trigger types */
#define OCMD_EQUIP (1 << 0) /* obj must be in char's equip */
#define OCMD_INVEN (1 << 1) /* obj must be in char's inv   */
#define OCMD_ROOM (1 << 2)  /* obj must be in char's room  */

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
void act_mtrigger(struct char_data *ch, const char *str, struct char_data *actor, struct char_data *victim,
                  struct obj_data *object, struct obj_data *target, char *arg, char *arg2);
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

/***************************************************************************
 * $Log: dg_scripts.h,v $
 * Revision 1.34  2009/03/07 09:34:47  jps
 * Changed name of room Entry trigger to Preentry. Added a Postentry room
 *trigger type.
 *
 * Revision 1.33  2008/09/02 03:00:59  jps
 * Changed mob speech and ask triggers to respond to all speech.
 *
 * Revision 1.32  2008/08/26 03:58:13  jps
 * Replaced real_zone calls with find_real_zone_by_room, since that's what it
 *did. Except the one for wzoneecho, since it needed to find a real zone by zone
 *number.
 *
 * Revision 1.31  2008/08/15 04:56:34  jps
 * Adding prototype for script_driver().
 *
 * Revision 1.30  2008/06/05 02:07:43  myc
 * Adding read_trigger as a public function.
 *
 * Revision 1.29  2008/04/05 20:41:20  jps
 * Adding standard header ifdef.
 *
 * Revision 1.28  2008/04/05 19:42:51  jps
 * Add variable to trigger struct to store damage done by a *damage
 * command.
 *
 * Revision 1.27  2008/03/22 19:51:56  myc
 * Rewrote the script formatter to use a stack.  It is now leet haxorz.
 *
 * Revision 1.26  2008/03/21 15:58:34  myc
 * Added a utility format scripts.
 *
 * Revision 1.25  2008/03/17 16:22:42  myc
 * Signature for free_proto_script changed.
 *
 * Revision 1.24  2008/02/24 17:31:13  myc
 * Added a TO_OLC flag to act() to allow messages to be sent to people
 * while in OLC if they have OLCComm toggled on.
 *
 * Revision 1.23  2008/02/16 20:26:04  myc
 * Adding free_trigger, free_var_list, and free_proto_script.
 *
 * Revision 1.22  2008/02/06 03:45:08  myc
 * Stat room and stat obj now use the pager.
 *
 * Revision 1.21  2008/02/04 00:22:05  myc
 * Making stat char use the pager.
 *
 * Revision 1.20  2008/02/02 04:27:55  myc
 * Adding several new trigger types: cast, leave, door, time, load,
 * and consume.
 *
 * Revision 1.19  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.18  2008/01/15 06:49:52  myc
 * When a mob had a trigger marked both ask and speech and the mob
 * was asked a question, that same trigger would be executed twice,
 * once as an ask trigger, and once as a speech trigger.  Fixed this
 * by going through the trigger list only once looking for ask or
 * speech triggers.  However, this isn't optimal--it should give
 * priority to ask triggers.
 *
 * Revision 1.17  2007/10/04 16:20:24  myc
 * Added object timer trigger.
 *
 * Revision 1.16  2007/08/31 05:36:19  jps
 * Add variable "destination" to mob Entry trigger.
 *
 * Revision 1.15  2007/08/30 19:42:46  jps
 * Cause *purge dg script commands to destroy all of a mobile's inventory
 * and equipment when purging mobs.
 *
 * Revision 1.14  2007/08/24 17:01:36  myc
 * Adding ostat and mstat commands as shorthand for vstat, rstat for stat
 * room, and mnum and onum for vnum.  Also adding rnum and znum with new
 * functionality.
 *
 * Revision 1.13  2007/04/17 23:59:16  myc
 * New trigger type: Load.  It goes off any time a mobile is loaded, whether
 * it be god command, zone command, or trigger command.
 *
 * Revision 1.12  2006/12/08 05:06:58  myc
 * Bribe triggers now give proper amounts and variables.
 *
 * Revision 1.11  2006/11/30 05:06:24  jps
 * Add remove trigger for objects
 *
 * Revision 1.10  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.9  2001/07/25 06:59:02  mtp
 * modified logging to hopefully be a bit more helpful by specifying the
 * trigger id wherever possible. This does not apply to logging of mob trigs yet
 * as mobs use the same commands as players :-(
 *
 * Revision 1.8  2000/11/28 01:16:25  mtp
 * replaced dg_event code with events.c code
 *
 * Revision 1.7  2000/11/21 04:22:16  rsd
 * Altered the comment header to look like what we have mostly
 * and touched up the white space to make things look neater.
 * Also added the missing initial revision rlog message.
 *
 * Revision 1.6  2000/11/11 12:03:42  mtp
 * made function ask_mtrigger() void cos it doesnt return anything
 *
 * Revision 1.5  2000/11/11 01:38:18  mtp
 * added ASK trigger for mobs
 *
 * Revision 1.4  2000/02/13 07:34:13  mtp
 * fixed opurge/mpurge problems by not freeing the running
 * trigger until it completes (added running flag to dg_scripts.h)
 *
 * Revision 1.3  1999/11/28 23:11:31  cso
 * removed define for GET_SHORT: moved it to utils.h
 *
 * Revision 1.2  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/
