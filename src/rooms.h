/***************************************************************************
 * $Id: rooms.h,v 1.13 2010/06/05 04:43:57 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: rooms.h                                         Part of FieryMUD *
 *  Usage: header file for rooms                                           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_ROOMS_H
#define __FIERY_ROOMS_H

#include "specprocs.h"
#include "directions.h"

extern struct room_data *world;

/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK               0   /* Dark                           */
#define ROOM_DEATH              1   /* Death trap                     */
#define ROOM_NOMOB              2   /* MOBs not allowed               */
#define ROOM_INDOORS            3   /* Indoors                        */
#define ROOM_PEACEFUL           4   /* Violence not allowed           */
#define ROOM_SOUNDPROOF         5   /* Shouts, gossip blocked         */
#define ROOM_NOTRACK            6   /* Track won't go through         */
#define ROOM_NOMAGIC            7   /* Magic not allowed              */
#define ROOM_TUNNEL             8   /* room for only 2 pers           */
#define ROOM_PRIVATE            9   /* Can't teleport in              */
#define ROOM_GODROOM           10   /* LVL_GOD+ only allowed          */
#define ROOM_HOUSE             11   /* (R) Room is a house            */
#define ROOM_HOUSE_CRASH       12   /* (R) House needs saving         */
#define ROOM_ATRIUM            13   /* (R) The door to a house        */
#define ROOM_OLC               14   /* (R) Modifyable/!compress       */
#define ROOM_BFS_MARK          15   /* (R) breadth-first srch mrk     */
#define ROOM_NOWELL            16   /* No spell portals like moonwell */
#define ROOM_NORECALL          17   /* No recalling                   */
#define ROOM_UNDERDARK         18   /*                   (not used)   */
#define ROOM_NOSUMMON          19   /* Can't summon to or from. Can't banish here. */
#define ROOM_NOSHIFT           20   /* no plane shift    (not used)   */
#define ROOM_GUILDHALL         21   /*                   (not used)   */
#define ROOM_NOSCAN            22   /* Unable to scan to/from rooms   */
#define ROOM_ALT_EXIT          23   /* Room's exits are altered       */
#define ROOM_MAP               24   /* Room on surface map (unused)   */
#define ROOM_ALWAYSLIT         25   /* Makes the room lit             */
#define ROOM_ARENA             26   /* (safe) PK allowed in room      */
#define ROOM_OBSERVATORY       27   /* see into adjacent ARENA rooms  */
#define NUM_ROOM_FLAGS         28   /* Keep me updated! */

#define ROOM_EFF_FOG            0   /* Tough to see anything */
#define ROOM_EFF_DARKNESS       1   /* Magically made dark */
#define ROOM_EFF_ILLUMINATION   2   /* Magically made lit */
#define ROOM_EFF_FOREST         3   /* Um, magically made foresty */
#define ROOM_EFF_CIRCLE_FIRE    4   /* This spell hurts people */
#define ROOM_EFF_ISOLATION      5   /* Exits aren't visible */
#define NUM_ROOM_EFF_FLAGS      6   /* Keep me updated! */

#define ROOM_RNUM_TO_VNUM(rnum) (rnum >= 0 && rnum < top_of_world ? world[rnum].vnum : NOWHERE)

extern const char *room_bits[NUM_ROOM_FLAGS + 1];
extern const char *room_effects[];
extern const int movement_loss[];

struct room_data {
   room_num vnum;                      /* Room's virtual number              */
   int zone;                           /* Room zone (for resetting)          */
   int         sector_type;            /* sector type (move/hide)            */
   char        *name;                  /* Rooms name 'You are ...'           */
   char        *description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look              */
   struct exit *exits[NUM_OF_DIRS];

   /* DEATH,DARK ... etc                 */
   flagvector room_flags[FLAGVECTOR_SIZE(NUM_ROOM_FLAGS)];

   /* bitvector for spells/skills */
   flagvector room_effects[FLAGVECTOR_SIZE(NUM_ROOM_EFF_FLAGS)];

   byte light;                  /* Number of light sources in room     */
   SPECIAL(*func);

   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;  /* script info for the object         */

   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room          */
   int bfs_distance;
};

struct room_effect_node {
   room_num room;        /* location in the world[] array of the room */
   int      timer;       /* how many ticks this effect lasts          */
   int      effect;      /* which effect does this room have          */
   int      spell;       /* the spell number                          */
   struct room_effect_node *next; /* link to the next node            */
};

#define SECT_STRUCTURE          0  /* A building of some kind   */
#define SECT_CITY               1  /* In a city                 */
#define SECT_FIELD              2  /* In a field                */
#define SECT_FOREST             3  /* In a forest               */
#define SECT_HILLS              4  /* In the hills              */
#define SECT_MOUNTAIN           5  /* On a mountain             */
#define SECT_SHALLOWS           6  /* Easily passable water     */
#define SECT_WATER              7  /* Water - need a boat       */
#define SECT_UNDERWATER         8  /* Underwater                */
#define SECT_AIR                9  /* Wheee!                    */
#define SECT_ROAD              10
#define SECT_GRASSLANDS        11
#define SECT_CAVE              12
#define SECT_RUINS             13
#define SECT_SWAMP             14
#define SECT_BEACH             15
#define SECT_UNDERDARK         16
#define SECT_ASTRALPLANE       17
#define SECT_AIRPLANE          18
#define SECT_FIREPLANE         19
#define SECT_EARTHPLANE        20
#define SECT_ETHEREALPLANE     21
#define SECT_AVERNUS           22
#define NUM_SECTORS            23   /* Keep me updated! */

struct sectordef {
   char name[40];            /* Name                   */
   char color[10];           /* Color                  */
   int mv;                   /* Movement cost          */
   int fall_mod;             /* Falling down modifier, for earthquakes        */
   int qdam_mod;             /* Quake damage modifier. 0 = cannot quake here  */
   bool campable;
   bool wet;
   char camp_excuse[200];
   char notes[200];
};

/*extern const char *sector_types[NUM_SECTORS + 1];
extern const char *sector_colors[NUM_SECTORS + 1]; */
extern const struct sectordef sectors[NUM_SECTORS];

#define SECT(rnum)   (world[rnum].sector_type)
#define SUN(rnum)   (hemispheres[zone_table[world[rnum].zone].hemisphere].sunlight)
#define IS_DARK(rnum)   ( \
      !ROOM_EFF_FLAGGED(rnum, ROOM_EFF_ILLUMINATION) && \
      !ROOM_EFF_FLAGGED(rnum, ROOM_EFF_CIRCLE_FIRE) && \
      ( \
         ( \
            world[rnum].light < 1 || ROOM_EFF_FLAGGED(rnum, ROOM_EFF_DARKNESS) \
         ) \
         && \
            !ROOM_FLAGGED(rnum, ROOM_ALWAYSLIT) \
         && \
            SECT(rnum) != SECT_CITY \
         && \
         ( \
            ROOM_FLAGGED(rnum, ROOM_DARK) || \
            ROOM_FLAGGED(rnum, ROOM_INDOORS) || \
            SUN(rnum) == SUN_SET || \
            SUN(rnum) == SUN_DARK \
         ) \
      ) \
   )

#define IS_LIGHT(rnum)  (!IS_DARK(rnum))

#define IS_WATER(rnum) \
   (SECT(rnum) == SECT_SHALLOWS || SECT(rnum) == SECT_WATER || \
   SECT(rnum) == SECT_UNDERWATER)

#define IS_SPLASHY(rnum) \
   (SECT(rnum) == SECT_SHALLOWS || SECT(rnum) == SECT_WATER || \
   SECT(rnum) == SECT_SWAMP)

#define IS_WET(rnum) (sectors[SECT(rnum)].wet)

#define IS_FOREST(rnum) \
   (SECT(rnum) == SECT_FOREST || ROOM_EFF_FLAGGED(rnum, ROOM_EFF_FOREST))

#define GET_ROOM_SPEC(rnum) ((rnum) >= 0 ? world[(rnum)].func : NULL)

#define CH_NROOM(ch) (ch->in_room)
#define CH_ROOM(ch) (CH_NROOM(ch) == NOWHERE ? NULL : &world[CH_NROOM(ch)])
#define CH_RVNUM(ch) (CH_ROOM(ch) ? CH_ROOM(ch)->vnum : NOWHERE)
#define CH_EXIT(ch, dir) (CH_ROOM(ch) ? CH_ROOM(ch)->exits[dir] : NULL)
#define CH_NDEST(ch, dir) (CH_EXIT(ch, dir) ? CH_EXIT(ch, dir)->to_room : NOWHERE)
#define CH_VDEST(ch, dir) (CH_NDEST(ch, dir) == NOWHERE ? NOWHERE : world[CH_NDEST(ch, dir)].vnum)
#define CH_DEST(ch, dir) (CH_NDEST(ch, dir) == NOWHERE ? NULL : &world[CH_NDEST(ch, dir)])
#define CH_DEST_ZONE(ch, dir) (CH_NDEST(ch, dir) == NOWHERE ? NULL : world[CH_NDEST(ch, dir)].zone)
#define CH_SECT(ch) (CH_NROOM(ch) == NOWHERE ? SECT_STRUCTURE : world[CH_NROOM(ch)].sector_type)
#define INDOORS(rnum) ((rnum) == NOWHERE ? FALSE : \
      ROOM_FLAGGED(rnum, ROOM_INDOORS) || \
      SECT(rnum) == SECT_UNDERDARK || \
      SECT(rnum) == SECT_UNDERWATER)
#define CH_INDOORS(ch) (INDOORS(CH_NROOM(ch)))
#define CH_OUTSIDE(ch) (!CH_INDOORS(ch))
#define QUAKABLE(rnum) ((rnum) == NOWHERE ? FALSE : \
      sectors[SECT(rnum)].qdam_mod)

bool check_can_go(struct char_data *ch, int dir, bool quiet);

/* The following four functions - for opening, closing, unlocking, and locking
 * doors - may be called without an actor. In other words, ch can be NULL. */

void open_door(struct char_data *ch, room_num roomnum, int dir, bool quiet);
void close_door(struct char_data *ch, room_num roomnum, int dir, bool quiet);
void unlock_door(struct char_data *ch, room_num roomnum, int dir, bool quiet);
void lock_door(struct char_data *ch, room_num roomnum, int dir, bool quiet);

void pick_door(struct char_data *ch, room_num roomnum, int dir);

/* Informative stuff */

void send_auto_exits(struct char_data *ch, int roomnum);
void send_full_exits(struct char_data *ch, int roomnum);
bool room_contains_char(int roomnum, struct char_data *ch);

#endif

/***************************************************************************
 * $Log: rooms.h,v $
 * Revision 1.13  2010/06/05 04:43:57  mud
 * Replacing ocean sector type with cave.
 *
 * Revision 1.12  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.11  2009/03/07 11:13:54  jps
 * Incorporated flags of darkness, illumination, and circle-of-fire into
 * light/dark macros.
 *
 * Revision 1.10  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.9  2008/09/14 03:02:02  jps
 * Added room_contains_char - a safer way to check whether a character is in a
 * room.
 *
 * Revision 1.8  2008/09/09 08:23:37  jps
 * Placed sector info into a struct and moved its macros into rooms.h.
 *
 * Revision 1.7  2008/09/04 06:47:36  jps
 * Changed sector constants to match their strings
 *
 * Revision 1.6  2008/09/01 08:34:47  jps
 * Added macro for getting the vnum of a room you're about to enter.
 *
 * Revision 1.5  2008/09/01 05:51:33  jps
 * Added a list of colors for sector types.
 *
 * Revision 1.4  2008/08/17 06:51:10  jps
 * Added macro room_rnum_to_vnum.
 *
 * Revision 1.3  2008/05/18 05:16:59  jps
 * Passing real room num rather than a room.. easier this way.
 *
 * Revision 1.2  2008/05/18 02:01:28  jps
 * Added some room-related constants. Also prototypes for
 * functions to inform about exits.
 *
 * Revision 1.1  2008/05/17 22:02:43  jps
 * Initial revision
 *
 ***************************************************************************/
