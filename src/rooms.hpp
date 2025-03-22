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

#pragma once

#include "directions.hpp"
#include "exits.hpp"
#include "specprocs.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "weather.hpp"

/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK 0         /* Dark                           */
#define ROOM_DEATH 1        /* Death trap                     */
#define ROOM_NOMOB 2        /* MOBs not allowed               */
#define ROOM_INDOORS 3      /* Indoors                        */
#define ROOM_PEACEFUL 4     /* Violence not allowed           */
#define ROOM_SOUNDPROOF 5   /* Shouts, gossip blocked         */
#define ROOM_NOTRACK 6      /* Track won't go through         */
#define ROOM_NOMAGIC 7      /* Magic not allowed              */
#define ROOM_TUNNEL 8       /* room for only 2 pers           */
#define ROOM_PRIVATE 9      /* Can't teleport in              */
#define ROOM_GODROOM 10     /* LVL_GOD+ only allowed          */
#define ROOM_HOUSE 11       /* (R) Room is a house            */
#define ROOM_HOUSE_CRASH 12 /* (R) House needs saving         */
#define ROOM_ATRIUM 13      /* (R) The door to a house        */
#define ROOM_OLC 14         /* (R) Modifyable/!compress       */
#define ROOM_BFS_MARK 15    /* (R) breadth-first srch mrk     */
#define ROOM_NOWELL 16      /* No spell portals like moonwell */
#define ROOM_NORECALL 17    /* No recalling                   */
#define ROOM_UNDERDARK 18   /*                   (not used)   */
#define ROOM_NOSUMMON 19    /* Can't summon to or from. Can't banish here. */
#define ROOM_NOSHIFT 20     /* no plane shift    (not used)   */
#define ROOM_GUILDHALL 21   /*                   (not used)   */
#define ROOM_NOSCAN 22      /* Unable to scan to/from rooms   */
#define ROOM_ALT_EXIT 23    /* Room's exits are altered       */
#define ROOM_MAP 24         /* Room on surface map (unused)   */
#define ROOM_ALWAYSLIT 25   /* Makes the room lit             */
#define ROOM_ARENA 26       /* (safe) PK allowed in room      */
#define ROOM_OBSERVATORY 27 /* see into adjacent ARENA rooms  */
#define NUM_ROOM_FLAGS 28   /* Keep me updated! */

#define ROOM_EFF_FOG 0          /* Tough to see anything */
#define ROOM_EFF_DARKNESS 1     /* Magically made dark */
#define ROOM_EFF_ILLUMINATION 2 /* Magically made lit */
#define ROOM_EFF_FOREST 3       /* Um, magically made foresty */
#define ROOM_EFF_CIRCLE_FIRE 4  /* This spell hurts people */
#define ROOM_EFF_ISOLATION 5    /* Exits aren't visible */
#define NUM_ROOM_EFF_FLAGS 6    /* Keep me updated! */

#define ROOM_RNUM_TO_VNUM(rnum) (rnum >= 0 && rnum < top_of_world ? world[rnum].vnum : NOWHERE)

#define SECT_STRUCTURE 0  /* A building of some kind   */
#define SECT_CITY 1       /* In a city                 */
#define SECT_FIELD 2      /* In a field                */
#define SECT_FOREST 3     /* In a forest               */
#define SECT_HILLS 4      /* In the hills              */
#define SECT_MOUNTAIN 5   /* On a mountain             */
#define SECT_SHALLOWS 6   /* Easily passable water     */
#define SECT_WATER 7      /* Water - need a boat       */
#define SECT_UNDERWATER 8 /* Underwater                */
#define SECT_AIR 9        /* Wheee!                    */
#define SECT_ROAD 10
#define SECT_GRASSLANDS 11
#define SECT_CAVE 12
#define SECT_RUINS 13
#define SECT_SWAMP 14
#define SECT_BEACH 15
#define SECT_UNDERDARK 16
#define SECT_ASTRALPLANE 17
#define SECT_AIRPLANE 18
#define SECT_FIREPLANE 19
#define SECT_EARTHPLANE 20
#define SECT_ETHEREALPLANE 21
#define SECT_AVERNUS 22
#define NUM_SECTORS 23 /* Keep me updated! */

struct sectordef {
    char name[40];  /* Name                   */
    char color[10]; /* Color                  */
    int mv;         /* Movement cost          */
    int fall_mod;   /* Falling down modifier, for earthquakes        */
    int qdam_mod;   /* Quake damage modifier. 0 = cannot quake here  */
    bool campable;
    bool wet;
    char camp_excuse[200];
    char notes[200];
};

extern const struct sectordef sectors[NUM_SECTORS];
extern const char *room_bits[NUM_ROOM_FLAGS + 1];
extern const char *room_effects[NUM_ROOM_EFF_FLAGS + 1];

struct RoomData {
    room_num vnum;                        /* Room's vnum              */
    int zone;                             /* Room zone (for resetting)          */
    int sector_type;                      /* sector type (move/hide)            */
    char *name;                           /* Rooms name 'You are ...'           */
    char *description;                    /* Shown when entered                 */
    ExtraDescriptionData *ex_description; /* for examine/look              */
    Exit *exits[NUM_OF_DIRS];

    /* DEATH,DARK ... etc                 */
    flagvector room_flags[FLAGVECTOR_SIZE(NUM_ROOM_FLAGS)];

    /* bitvector for spells/skills */
    flagvector room_effects[FLAGVECTOR_SIZE(NUM_ROOM_EFF_FLAGS)];

    int light; /* Number of light sources in room     */
    SPECIAL(*func);

    TriggerPrototypeList *proto_script; /* list of default triggers  */
    ScriptData *script;                 /* script info for the object         */

    ObjData *contents; /* List of items in room              */
    CharData *people;  /* List of NPC / PC in room          */
    int bfs_distance;
};

struct RoomEffectNode {
    room_num room;        /* location in the world[] array of the room */
    int timer;            /* how many ticks this effect lasts          */
    int effect;           /* which effect does this room have          */
    int spell;            /* the spell number                          */
    RoomEffectNode *next; /* link to the next node            */
};

#define SECT(rnum) (world[rnum].sector_type)
#define SUN(rnum) (hemispheres[zone_table[world[rnum].zone].hemisphere].sunlight)
#define IS_DARK(rnum)                                                                                                  \
    (!ROOM_EFF_FLAGGED(rnum, ROOM_EFF_ILLUMINATION) && !ROOM_EFF_FLAGGED(rnum, ROOM_EFF_CIRCLE_FIRE) &&                \
     ((world[rnum].light < 1 || ROOM_EFF_FLAGGED(rnum, ROOM_EFF_DARKNESS)) && !ROOM_FLAGGED(rnum, ROOM_ALWAYSLIT) &&   \
      SECT(rnum) != SECT_CITY &&                                                                                       \
      (ROOM_FLAGGED(rnum, ROOM_DARK) || ROOM_FLAGGED(rnum, ROOM_INDOORS) || SUN(rnum) == SUN_SET ||                    \
       SUN(rnum) == SUN_DARK)))

#define IS_LIGHT(rnum) (!IS_DARK(rnum))

#define IS_WATER(rnum) (SECT(rnum) == SECT_SHALLOWS || SECT(rnum) == SECT_WATER || SECT(rnum) == SECT_UNDERWATER)

#define IS_SPLASHY(rnum) (SECT(rnum) == SECT_SHALLOWS || SECT(rnum) == SECT_WATER || SECT(rnum) == SECT_SWAMP)

#define IS_WET(rnum) (sectors[SECT(rnum)].wet)

#define IS_FOREST(rnum) (SECT(rnum) == SECT_FOREST || ROOM_EFF_FLAGGED(rnum, ROOM_EFF_FOREST))

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
#define INDOORS(rnum)                                                                                                  \
    ((rnum) == NOWHERE                                                                                                 \
         ? false                                                                                                       \
         : ROOM_FLAGGED(rnum, ROOM_INDOORS) || SECT(rnum) == SECT_UNDERDARK || SECT(rnum) == SECT_UNDERWATER)
#define CH_INDOORS(ch) (INDOORS(CH_NROOM(ch)))
#define CH_OUTSIDE(ch) (!CH_INDOORS(ch))
#define QUAKABLE(rnum) ((rnum) == NOWHERE ? false : sectors[SECT(rnum)].qdam_mod)

bool check_can_go(CharData *ch, int dir, bool quiet);

/* The following four functions - for opening, closing, unlocking, and locking
 * doors - may be called without an actor. In other words, ch can be NULL. */

void open_door(CharData *ch, room_num roomnum, int dir, bool quiet);
void close_door(CharData *ch, room_num roomnum, int dir, bool quiet);
void unlock_door(CharData *ch, room_num roomnum, int dir, bool quiet);
void lock_door(CharData *ch, room_num roomnum, int dir, bool quiet);

void pick_door(CharData *ch, room_num roomnum, int dir);

/* Informative stuff */

void send_auto_exits(CharData *ch, int roomnum);
void send_full_exits(CharData *ch, int roomnum);
bool room_contains_char(int roomnum, CharData *ch);
bool can_see_exit(CharData *ch, int roomnum, Exit *exit);
