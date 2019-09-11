/***************************************************************************
 *  File: players.h                                       Part of FieryMUD *
 *  Usage: Player loading/saving and utility routines.                     *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_PLAYERS_H
#define __FIERY_PLAYERS_H

/* Reasons a player was removed from the game or saved.
 * Reason                    Objects-kept   Keys-kept    Loadroom */
#define QUIT_UNDEF       0
#define QUIT_RENT        1  /*  yes           no           here   */
#define QUIT_CRYO        2  /*  yes           no           here   */
#define QUIT_TIMEOUT     3  /*  yes           no           home   */
#define QUIT_HOTBOOT     4  /*  yes           yes          home   */
#define QUIT_QUITMORT    5  /*  no            no           temple */
#define QUIT_QUITIMM     6  /*  yes           no           here   */
#define QUIT_CAMP        7  /*  yes           no           here   */
#define QUIT_WRENT       8  /*  yes           yes          here   */
#define QUIT_PURGE       9  /*  yes           yes          home   */
#define QUIT_AUTOSAVE   10  /*  yes           yes          last   */  /* Not really quit */
#define NUM_QUITTYPES   11
#define VALID_QUITTYPE(s) ((s) >= 0 && (s) < NUM_QUITTYPES)

extern void build_player_index(void);
extern int create_player_index_entry(char *name);
extern void save_player_index(void);
extern void free_player_index(void);

extern long get_ptable_by_name(const char *name);
extern long get_id_by_name(const char *name);
extern char *get_name_by_id(long id);
extern int get_pfilename(const char *name, char *filename, int mode);

extern int load_player(const char *name, struct char_data *ch);
extern void save_player_char(struct char_data *ch);
extern void delete_player(int pfilepos);
extern void rename_player(struct char_data *victim, char *newname);
extern void init_player(struct char_data *ch);
extern void start_player(struct char_data *ch);

extern void add_perm_title(struct char_data *ch, char *line);

extern void load_ascii_flags(flagvector flags[], int num_flags, char *line);
extern void write_ascii_flags(FILE *fl, flagvector flags[], int num_flags);

extern void remove_player_from_game(struct char_data *ch, int removal_mode);
extern void send_save_description(struct char_data *ch, struct char_data *dest, bool entering);



/* get_pfilename() codes */
#define OBJ_FILE        0
#define PLR_FILE        1
#define QUEST_FILE      2
#define NOTES_FILE      3
#define TEMP_FILE       4
#define NUM_PLR_FILES   5

/* player index flags */
#define PINDEX_FROZEN      (1 << 0)
#define PINDEX_NEWNAME     (1 << 1)
#define PINDEX_NAPPROVE    (1 << 2)
#define PINDEX_DELETED     (1 << 3)
#define PINDEX_NODELETE    (1 << 4)
#define NUM_PINDEX_FLAGS   5

/* Changing the values below may screw up existing ascii player files. */

#define DEFAULT_PAGE_LENGTH   22
#define PFDEF_HUNGER      0
#define PFDEF_THIRST      0
#define PFDEF_DRUNK       0
#define PFDEF_COINS       0
#define PFDEF_BANK        0


#define MAX_TITLE_WIDTH      60
#define WIZ_TITLE_WIDTH      12

#endif

/***************************************************************************
 * $Log: players.h,v $
 * Revision 1.9  2008/09/20 17:01:58  jps
 * Fix whitespace and standardize header defs.
 *
 * Revision 1.8  2008/09/08 05:24:50  jps
 * Put autosave as the "quit reason" when autosaving. This is a temporary fix
 * that should stop people from losing keys when autosave code thinks their
 * quit reason is something else, like renting.
 *
 * Revision 1.7  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.6  2008/07/27 05:24:10  jps
 * Renamed save_player to save_player char, and added remove_player_from_game.
 *
 * Revision 1.5  2008/06/05 02:07:43  myc
 * Added better unknown tag error reporting in load_player.
 * Replaced calls to strip_cr with filter_chars.  Rewrote the
 * rent file saving and loading to use an ascii file format.
 *
 * Revision 1.4  2008/03/16 00:22:22  jps
 * Removed function prototypes for trophies, which are in trophy.h.
 *
 * Revision 1.3  2008/03/08 19:01:12  jps
 * Add RCS Log marker.
 *
 **************************************************************************/
