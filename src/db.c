/***************************************************************************
 * $Id: db.c,v 1.194 2010/06/05 15:09:42 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: db.c                                           Part of FieryMUD *
 *  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#define __DB_C__

#include <math.h>

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "casting.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "corpse_save.h"
#include "dg_scripts.h"
#include "weather.h"
#include "clan.h"
#include "quest.h"
#include "races.h"
#include "skills.h"
#include "constants.h"
#include "math.h"
#include "events.h"
#include "players.h"
#include "trophy.h"
#include "pfiles.h"
#include "regen.h"
#include "fight.h"
#include "commands.h"
#include "movement.h"
#include "board.h"
#include "composition.h"
#include "charsize.h"
#include "directions.h"
#include "money.h"
#include "textfiles.h"
#include "cooldowns.h"

void init_clans(void);
char err_buf[MAX_STRING_LENGTH];
struct player_special_data dummy_mob;

/*object limit function*/
void boot_quests();

void setup_drinkcon(struct obj_data *obj, int newliq);

/***************************************************************************
 *  declarations of most of the 'global' variables                         *
 ***************************************************************************/


struct room_data *world = NULL;        /* array of rooms                 */
int top_of_world = 0;                /* ref to top element of world         */
struct room_effect_node *room_effect_list = NULL;  /* list of room effects */

struct char_data *character_list = NULL; /* global linked list of chars */

struct index_data **trig_index; /* index table for triggers      */
int top_of_trigt = 0;           /* top of trigger index table    */
long max_id = 100000;           /* for unique mob/obj id's       */

struct index_data *mob_index;        /* index table for mobile file         */
struct char_data *mob_proto;        /* prototypes for mobs                 */
int top_of_mobt = 0;                /* top of mobile index table         */
struct obj_data *object_list = NULL;        /* global linked list of objs         */
struct index_data *obj_index;        /* index table for object file         */
struct obj_data *obj_proto;        /* prototypes for objs                 */
int top_of_objt = 0;                /* top of object index table         */
struct spell_dam spell_dam_info[MAX_SPELLS + 1];/*internal spell dam*/
struct zone_data *zone_table;        /* zone table                         */
int top_of_zone_table = 0;        /* top element of zone tab         */
struct message_list fight_messages[MAX_MESSAGES];        /* fighting messages         */

struct player_index_element *player_table = NULL;        /* index to plr file         */
int top_of_p_table = 0;                /* ref to top of table                 */
int top_of_p_file = 0;                /* ref of size of p file         */
long top_idnum = 0;                /* highest idnum in use                 */

int no_mail = 0;                /* mail disabled?                 */
#ifdef DEV
int mini_mud = 1;                /* mini-mud mode?                 */
#else
int mini_mud = 0;                /* mini-mud mode?                 */
#endif
time_t *boot_time = NULL;                /* times of mud boots (size = 1 + num_hotboots) */
int num_hotboots = 0;                    /* are we doing a hotboot? */
int should_restrict = 0;                /* level of game restriction         */
int restrict_reason = RESTRICT_NONE;   /* reason for should_restrict > 0 */
int r_mortal_start_room;        /* rnum of mortal start room         */
int r_immort_start_room;        /* rnum of immort start room         */
int r_frozen_start_room;        /* rnum of frozen start room         */

struct help_index_element *help_table = 0;        /* the help table         */
int top_of_helpt = 0;                /* top of help index table         */

struct time_info_data time_info;/* the infomation about the time    */

struct reset_q_type reset_q;        /* queue of zones to be reset         */

struct str_app_type str_app[101];
struct dex_skill_type dex_app_skill[101];
struct dex_app_type dex_app[101];
struct con_app_type con_app[101];
struct int_app_type int_app[101];
struct wis_app_type wis_app[101];

void setup_dir(FILE * fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode);
void parse_trigger(FILE *fl, int virtual_nr);
void parse_room(FILE * fl, int virtual_nr);
void parse_mobile(FILE * mob_f, int nr);
char *parse_object(FILE * obj_f, int nr);
void load_zones(FILE * fl, char *zonename);
void load_help(FILE *fl);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
int is_empty(int zone_nr);
void reset_zone(int zone, byte pop);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(int zone, int cmd_no, char *message);
void reset_time(void);
void clear_char(struct char_data * ch);
long get_set_exp(int level, int race, int class, int zone);
sh_int get_set_hit(int level, int race, int class, int state);
sbyte get_set_hd(int level, int race, int class, int state);
int get_set_dice(int level, int race, int class, int state);
int get_copper(int);
/* external functions */
void boot_social_messages(void);
void sort_commands(void);
void sort_spells(void);
void load_banned(void);
void Read_Xname_List(void);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
int hsort(const void *a, const void *b);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
void build_count(void);
extern void load_str_app(void);
extern void load_thief_dex(void);
extern void load_dex_app(void);
extern void load_con_app(void);
extern void load_int_app(void);
extern void load_wis_app(void);
extern void clear_memory(struct char_data *ch);

/* external vars */
extern int no_specials;

#define READ_SIZE 256
#define GET_ZONE(ch) (ch)->mob_specials.zone

#define GET_SDESC(mob) ((mob)->player.short_descr)
/*#define GET_LDESC(mob) ((mob)->player.long_descr)*/
#define GET_NDD(mob) ((mob)->mob_specials.damnodice)
#define GET_SDD(mob) ((mob)->mob_specials.damsizedice)
#define MOB_MONSUM_I                130
#define MOB_MONSUM_II                140
#define MOB_MONSUM_III                150
#define MOB_GATE_I                160
#define MOB_GATE_II                170
#define MOB_GATE_III                180
#define MOB_ELEMENTAL_BASE        110
#define MOB_CLONE                69
#define MOB_ZOMBIE                11
#define MOB_AERIALSERVANT        109
#define MOB_MENTAL              17
#define MOB_MENTAL2             21
/*************************************************************************
 *  routines for booting the system                                      *
 *************************************************************************/

long get_set_exp(int level, int race, int class, int zone)
     /*class/species*/
{
  long exp;
  int cfactor = 100;
  int sfactor = 100;
  int zfactor = 100;
  for (zfactor = 0; zfactor <= top_of_zone_table; zfactor++) {
    if (zone == zone_table[zfactor].number)
      break;
  }
  zfactor = zone_table[zfactor].zone_factor;
  /*zfactor = 100;*/

  /*The cfactor is the factor of class to adjust exp number is percentage
    100 being standard no change*/
  cfactor = CLASS_EXPFACTOR(class);
  sfactor = RACE_EXPFACTOR(race);

  if (level < 50)
    exp = (long) (((float)(level * level * level)) + 1000);
  else if (level >= 50)/*50 and under equation*/
    exp = (long) (level * level * 50) + 1000;
  /*51 and over equation*/

  sfactor = ((int)((sfactor + zfactor + cfactor)/3));
  exp = (long) (((float)((sfactor*exp)/100)));
  return exp;
}

sh_int get_set_hit(int level, int race, int class, int state)
     /*class/species*/
{
  /*main - is the main bonus chucnk of hps
   */
  /*struct descriptor_data *d;
   */
  sh_int xmain = 0;
  sh_int face = 1;
  int cfactor = 100;
  int sfactor = 100;

  /* The cfactor is the factor of class to adjust exp number is percentage
     100 being standard no change */
  cfactor = CLASS_HITFACTOR(class);
  sfactor = RACE_HITFACTOR(race);

  /* here is the function*/
  /*auto setting sets hit and mana to 5 and 10 thus 5d10 extra = average 25*/
  if (level < 20)
    xmain = (sh_int) (3 * ((float)level * (float)(level/1.25)));         /*50 and under equation*/

  else if (level < 35)
    xmain = (sh_int) (3 * ((float)level * (float)(level/1.35)));         /*50 and under equation*/

  else if (level < 50)
    xmain = (sh_int) (3 * ((float)level * (float)level/1.25));         /*50 and under equation*/

  else if (level >= 50)
    xmain = (sh_int) (3 * ((float)level * (float)level/1.25));  /*51 and over equation*/
  if (level <=5)
    face = 1;
  else if (level <=10) {
    xmain -= 25;
    face = 5;
  }
  else if (level <= 20) {
    xmain -= 100;
    face = 10;
  }
  else if (level <=30)  {
    xmain -= 200;
    face = 20;
  }
  else {
    xmain -= 2000;
    face = 200;
  }


   /*finally taking the factors into account*/
  sfactor = ((int)((sfactor + cfactor)/2));
   xmain = (sh_int) (((float)(sfactor*xmain)/100));


   if (state == 2)
     return face;
   else
     return xmain/(2 - (level/100.0));


}


sbyte get_set_hd(int level, int race, int class, int state)
     /*class/species*/
{
  sbyte hit = 0;
  sbyte dam = 0;
  int cfactor = 100;
  int sfactor = 100;


  /*The cfactor is the factor of class to adjust exp number is percentage
    100 being standard no change*/
  cfactor = CLASS_HDFACTOR(class);
  sfactor = RACE_HDFACTOR(race);

  /*hit calculations*/
  if (!state) {
    if (level < 10)
      hit = (sbyte) (level/2.0);
    else if (level < 24)
      hit = (sbyte) (level/2.4);
    else if (level < 32)
      hit = (sbyte) (level/2.6);
    else if (level < 50)
      hit = (sbyte) (level/2.8);
    else if (level < 62)
      hit = (sbyte) (level/3.0);
    else if (level < 75)
      hit = (sbyte) (level/3.2);        /*50 and under equation*/
    else if (level < 82)
      hit = (sbyte) (level/3.4);
    else if (level >= 90)
      hit = (sbyte) (level/3.6);         /*51 and over equation*/


    /*hit factor considerations*/
    sfactor = ((int)((sfactor + cfactor)/2));
    hit = (sbyte) (((float)(sfactor*hit)/100));


  }

  /*dam calculations*/
  if (state) {
    if (level < 10)
      dam = (sbyte) (level /4.0);
    else if (level < 20)
      dam = (sbyte) (level / 4.0); /*under 20*/
    else if (level < 35)
      dam = (sbyte) (level/4.3); /*under 35*/

    else if (level < 50)
      dam = (sbyte) (level / 4.6);        /*50 and under equation*/

    else if (level >= 50)
      dam = (sbyte) (level / 4.4);        /*51 and over equation*/

    /*dam factor considerations*/
    sfactor = ((int)((sfactor + cfactor)/2));
    dam = (sbyte) (((float)(sfactor*dam)/100));
  }



  if (!state)
    return hit;
  else
    return dam;
}

int get_set_dice(int level, int race, int class, int state)

     /*class/species*/
{
  int dice = 0;
  int face = 0;
  int cfactor = 100;
  int sfactor = 100;


  /*The cfactor is the factor of class to adjust exp number is percentage
    100 being standard no change*/
  cfactor = CLASS_DICEFACTOR(class);
  sfactor = RACE_DICEFACTOR(race);

  /*dice calculations*/
  if (!state) {
    if (level < 10)
      dice = MAX(1, (int) ((level/3) + .5));

    else if (level < 30)
      dice = (int) ((float)(level/3) + .5); /*under 30*/

    else if (level <= 50)
      dice = (int) ((level /3) + .5);        /*50 and under equation*/

    else if (level > 50)
      dice = (int) ((level / 2.5) + .5);        /*51 and over equation*/

    sfactor = ((int)((sfactor + cfactor)/2));
    dice = (sbyte) (((float)(sfactor*dice)/100));
  }
  /*face calucs*/
  if (state) {
    if (level < 10)
      face = 3;
    else if (level < 26)
      face = 4;
    else if (level < 36)
      face = 4;

    else if (level <= 50)
      face = 5;        /*50 and under equation*/
    else if ((level > 50) && (level <= 60))
      face = 8;           /*50 over equation*/
    else if (level > 60)
      face = 10;

    /*face = (sbyte) (((float)sfactor/100) * ((float)cfactor/100) * face);
     */}



  if (!state)
    return dice;
  else
    return face;
}


int get_copper(int i)
     /*class/species*/
{
  /*mob_proto[i].player.class, mob_proto[i].player.race, mob_proto[i].player.level, GET_ZONE(mob_proto + i)*/
  int copper = 0;
  int cfactor = 100;
  int sfactor = 100;
  int zfactor;
  for (zfactor = 0; zfactor <= top_of_zone_table; zfactor++)
  {
    if (GET_ZONE(mob_proto + i) == zone_table[zfactor].number)
      break;
  }
  zfactor = zone_table[zfactor].zone_factor;


  /*The cfactor is the factor of class to adjust exp number is percentage
    100 being standard no change*/
  cfactor = CLASS_COPPERFACTOR((int)mob_proto[i].player.class);
  sfactor = RACE_COPPERFACTOR((int)mob_proto[i].player.race);

  if ((sfactor == 0) || (cfactor == 0))
    return 0;

  /*copper calculations*/
  copper = (number(1, 150)) * mob_proto[i].player.level;
  sfactor = (int)((sfactor + cfactor + zfactor)/3);

  copper = (int)((float)((sfactor/100.0) * copper));

  return copper;
}


int get_ac(int level, int race, int class)
     /*class/species*/
{
  /*mob_proto[i].player.class, mob_proto[i].player.race, mob_proto[i].player.level, GET_ZONE(mob_proto + i)*/
  int ac = 0;
  int cfactor = 100;
  int sfactor = 100;

  /*The cfactor is the factor of class to adjust exp number is percentage
    100 being standard no change*/
  cfactor = CLASS_ACFACTOR(class);
  sfactor = RACE_ACFACTOR(race);

  /*ac calculations*/
  ac = 90 - (int)(2 * level * (float)(sfactor / 100.0));
  ac = MIN(100, MAX(-100, ac));
  return ac;
}

/*This requres a file in lib/misc/spell_dams, It will tell you if you dont
  It then boots, note that if anything goes wrong to file
  you can re-write it by saving under sdedit, the viewer is in act.informative.c
  The olc code is in sdedit.c
*/
void boot_spell_dams()
{
  int i;
  FILE *ifptr;
  char line[256];
  char *err = "Spell Dam";
  if ((ifptr = fopen(SPELL_DAM_FILE, "r")) == NULL)
    {
      log("No spells dam file it should be at lib/misc/spell_dam.");
      exit(1);
    }
  else
    {
      get_line(ifptr, line);
      get_line(ifptr, line);
      get_line(ifptr, line);
/*
      sprintf(buf, "%s", line);
      log(buf);
      get_line(ifptr, line);
      sprintf(buf, "%s", line);
      log(buf);
      get_line(ifptr, line);
      sprintf(buf, "%s", line);
      log(buf);
*/
      if (str_cmp(line, "spell_dam"))
        {
          log("Error in booting spell dams");
          /* return;
           */  }
      for (i = 1; i <= MAX_SPELLS;i++)
        {
          get_line(ifptr, line);
          sscanf(line, "%hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd",
                 &SD_SPELL(i), &SD_INTERN_DAM(i),
                 &SD_NPC_STATIC(i), &SD_NPC_NO_DICE(i), &SD_NPC_NO_FACE(i),
                 &SD_PC_STATIC(i), &SD_PC_NO_DICE(i), &SD_PC_NO_FACE(i), &SD_NPC_REDUCE_FACTOR(i), &SD_USE_BONUS(i),
                 &SD_BONUS(i), &SD_LVL_MULT(i));
          if (SD_NPC_REDUCE_FACTOR(i) == 0)
            SD_NPC_REDUCE_FACTOR(i) = 100;

          SD_NOTE(i) = fread_string(ifptr, err);
        }
      fclose (ifptr);
    }
}

void free_spell_dams(void) {
  int i;
  extern struct spell_dam spell_dam_info[];


  for (i = 1; i <= MAX_SPELLS; i++)
    free(spell_dam_info[i].note);
}


void boot_world(void)
{
  log("Loading attribute bonus tables.");
  load_str_app();
  load_thief_dex();
  load_dex_app();
  load_con_app();
  load_int_app();
  load_wis_app();

  log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  log("Loading triggers and generating index.");
  index_boot(DB_BOOT_TRG);

  log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  log("Renumbering rooms.");
  renum_world();

  log("Checking start rooms.");
  check_start_rooms();

  log("Loading mobs and generating index.");
  memset(&dummy_mob, 0, sizeof(struct player_special_data));
  index_boot(DB_BOOT_MOB);

  log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  /* moved here by gurlaek 8/8/1999 */
  log("Generating player index.");
  build_player_index();

  log("Renumbering zone table.");
  renum_zone_table();

  log("Booting spell_dam's.");
  boot_spell_dams();

  if (!no_specials) {
    log("Loading shops.");
    index_boot(DB_BOOT_SHP);
  }

  /* Must happen after loading the player index */
  log("Booting Clans.");
  init_clans();

  log("Booting boards.");
  board_init();
}



/* body of the booting system */
void boot_db(void)
{
  int i;

  log("Boot db -- BEGIN.");

  log("Reading anews, credits, help, bground, info and motds.");
  boot_text();

  log("Getting Build Count.");
  build_count();

  log("   Skills.");
  init_skills();

  log("Assigning skills and spells to classes.");
  assign_class_skills();

  /* Command sorting needs to happen before many other loading
   * activities, because sorting the commands initializes the
   * num_of_cmds variable.
   */
  log("Sorting command list and skills.");
  sort_commands();
  sort_skills();

  /* Needs to happen after sorting commands */
  log("Booting command groups.");
  boot_command_groups();

  boot_world();

  log("Resetting the game time:");
  reset_time();

  log("Loading help entries.");
  index_boot(DB_BOOT_HLP);

  log("Loading fight messages.");
  load_messages();

  log("Loading social messages.");
  boot_social_messages();

  log("Assigning function pointers:");

  if (!no_specials) {
    log("   Mobiles.");
    assign_mobiles();
    log("   Shopkeepers.");
    assign_the_shopkeepers();
    log("   Objects.");
    assign_objects();
    log("   Rooms.");
    assign_rooms();
  }

  log("Booting mail system.");
  if (!scan_file()) {
    log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }
  log("Reading banned site and Xname list.");
  load_banned();
  Read_Xname_List();

  for (i = 0; i <= top_of_zone_table; i++) {
    sprintf(buf2, "Resetting %s (rooms %d-%d).",
            zone_table[i].name, (i ? (zone_table[i - 1].top + 1) : 0),
            zone_table[i].top);
    log(buf2);
    reset_zone(i, TRUE);
  }

  reset_q.head = reset_q.tail = NULL;

  if (!mini_mud) {
    log("Booting houses.");
    House_boot();
  }

  log("Booting corpses.");
  boot_corpses();
  log("Booting quests.");
  boot_quests();

  CREATE(boot_time, time_t, 1);
  *boot_time = time(0);

  log("Boot db -- DONE.");
}


void free_extra_descriptions(struct extra_descr_data *edesc) {
  struct extra_descr_data *enext;

  for (; edesc; edesc = enext) {
    enext = edesc->next;

    if (edesc->keyword)
      free(edesc->keyword);
    if (edesc->description)
      free(edesc->description);
    free(edesc);
  }
}

void copy_extra_descriptions(struct extra_descr_data **to, struct extra_descr_data *from)
{
  struct extra_descr_data *wpos;

  CREATE(*to, struct extra_descr_data, 1);
  wpos = *to;

  for (; from; from = from->next, wpos = wpos->next) {
    if (from->keyword)
      wpos->keyword = strdup(from->keyword);
    if (from->description)
      wpos->description = strdup(from->description);
    if (from->next)
      CREATE(wpos->next, struct extra_descr_data, 1);
  }
}

void destroy_db(void) {
  ssize_t cnt, itr;
  struct char_data *chtmp;
  struct obj_data *objtmp;

  extern void destroy_shops(void);
  extern void stop_groupee(struct char_data *ch, bool hide);

  /* Active Characters */
  while (character_list) {
    chtmp = character_list;
    character_list = character_list->next;
    if (chtmp->master)
      stop_follower(chtmp, FALSE);
    if (chtmp->group_master)
      ungroup(chtmp, FALSE, FALSE);
    free_char(chtmp);
  }

  /* Active Objects */
  while (object_list) {
    objtmp = object_list;
    object_list = object_list->next;
    free_obj(objtmp);
  }

  /* Rooms */
  for (cnt = 0; cnt <= top_of_world; cnt++) {
    if (world[cnt].name)
      free(world[cnt].name);
    if (world[cnt].description)
      free(world[cnt].description);
    free_extra_descriptions(world[cnt].ex_description);

    /* free any assigned scripts */
    if (SCRIPT(&world[cnt])) {
      extract_script(SCRIPT(&world[cnt]));
      SCRIPT(&world[cnt]) = NULL;
    }
    /* free script proto list */
    free_proto_script(&world[cnt].proto_script);

    for (itr = 0; itr < NUM_OF_DIRS; itr++) {
      if (!world[cnt].exits[itr])
        continue;

      if (world[cnt].exits[itr]->general_description)
        free(world[cnt].exits[itr]->general_description);
      if (world[cnt].exits[itr]->keyword)
        free(world[cnt].exits[itr]->keyword);
      free(world[cnt].exits[itr]);
    }
  }
  free(world);
  top_of_world = 0;

  /* Objects */
  for (cnt = 0; cnt <= top_of_objt; cnt++) {
    if (obj_proto[cnt].name)
      free(obj_proto[cnt].name);
    if (obj_proto[cnt].description)
      free(obj_proto[cnt].description);
    if (obj_proto[cnt].short_description)
      free(obj_proto[cnt].short_description);
    if (obj_proto[cnt].action_description)
      free(obj_proto[cnt].action_description);
    free_extra_descriptions(obj_proto[cnt].ex_description);

    /* free script proto list */
    free_proto_script(&obj_proto[cnt].proto_script);
  }
  free(obj_proto);
  free(obj_index);

  /* Mobiles */
  for (cnt = 0; cnt <= top_of_mobt; cnt++) {
    if (mob_proto[cnt].player.namelist)
      free(mob_proto[cnt].player.namelist);
    if (mob_proto[cnt].player.title)
      free(mob_proto[cnt].player.title);
    if (mob_proto[cnt].player.short_descr)
      free(mob_proto[cnt].player.short_descr);
    if (mob_proto[cnt].player.long_descr)
      free(mob_proto[cnt].player.long_descr);
    if (mob_proto[cnt].player.description)
      free(mob_proto[cnt].player.description);

    /* free script proto list */
    free_proto_script(&mob_proto[cnt].proto_script);

    while (mob_proto[cnt].effects)
      effect_remove(&mob_proto[cnt], mob_proto[cnt].effects);
  }
  free(mob_proto);
  free(mob_index);

  /* Shops */
  destroy_shops();

  /* Zones */
#define THIS_CMD zone_table[cnt].cmd[itr]

  for (cnt = 0; cnt <= top_of_zone_table; cnt++) {
    if (zone_table[cnt].name)
      free(zone_table[cnt].name);
    if (zone_table[cnt].cmd) {
      /* then free the command list */
      free(zone_table[cnt].cmd);
    }
  }
  free(zone_table);

#undef THIS_CMD

  /* zone table reset queue */
  if (reset_q.head) {
    struct reset_q_element *ftemp=reset_q.head, *temp;
    while (ftemp) {
      temp = ftemp->next;
      free(ftemp);
      ftemp = temp;
    }
  }

  /* Triggers */
  for (cnt=0; cnt < top_of_trigt; cnt++) {
    if (trig_index[cnt]->proto) {
      /* make sure to nuke the command list (memory leak) */
      /* free_trigger() doesn't free the command list */
      if (trig_index[cnt]->proto->cmdlist) {
        struct cmdlist_element *i, *j;
        i = trig_index[cnt]->proto->cmdlist;
        while (i) {
          j = i->next;
          if (i->cmd)
            free(i->cmd);
          free(i);
          i = j;
        }
      }
      free_trigger(trig_index[cnt]->proto);
    }
    free(trig_index[cnt]);
  }
  free(trig_index);

  /* Events */
  event_free_all();

  /* Online spell damage */
  free_spell_dams();

  /* Quests */
  free_quests();
}


/* reset the time in the game from file */
void reset_time(void)
{
  long beginning_of_time = 650336715;
  struct time_info_data mud_time_passed(time_t t2, time_t t1);

  time_info = mud_time_passed(time(0), beginning_of_time);

  log("Initializing daylight for hemispheres.");
  /* okay, saved time represents HEMISPHERE_NORTHWEST/ ...SOUTHWEST */
  if (time_info.hours <= 7) {
    hemispheres[HEMISPHERE_NORTHWEST].sunlight = SUN_DARK;
    hemispheres[HEMISPHERE_SOUTHWEST].sunlight = SUN_DARK;
    hemispheres[HEMISPHERE_NORTHEAST].sunlight = SUN_LIGHT;
    hemispheres[HEMISPHERE_SOUTHWEST].sunlight = SUN_LIGHT;
  } else if (time_info.hours == 8) {
    hemispheres[HEMISPHERE_NORTHWEST].sunlight = SUN_RISE;
    hemispheres[HEMISPHERE_SOUTHWEST].sunlight = SUN_RISE;
    hemispheres[HEMISPHERE_NORTHEAST].sunlight = SUN_SET;
    hemispheres[HEMISPHERE_SOUTHEAST].sunlight = SUN_SET;
  } else if (time_info.hours <= 20) {
    hemispheres[HEMISPHERE_NORTHWEST].sunlight = SUN_LIGHT;
    hemispheres[HEMISPHERE_SOUTHWEST].sunlight = SUN_LIGHT;
    hemispheres[HEMISPHERE_NORTHEAST].sunlight = SUN_DARK;
    hemispheres[HEMISPHERE_SOUTHEAST].sunlight = SUN_DARK;
  } else if (time_info.hours == 21) {
    hemispheres[HEMISPHERE_NORTHWEST].sunlight = SUN_SET;
    hemispheres[HEMISPHERE_SOUTHWEST].sunlight = SUN_SET;
    hemispheres[HEMISPHERE_NORTHEAST].sunlight = SUN_RISE;
    hemispheres[HEMISPHERE_SOUTHEAST].sunlight = SUN_RISE;
  } else {
    hemispheres[HEMISPHERE_NORTHWEST].sunlight = SUN_DARK;
    hemispheres[HEMISPHERE_SOUTHWEST].sunlight = SUN_DARK;
    hemispheres[HEMISPHERE_NORTHEAST].sunlight = SUN_LIGHT;
    hemispheres[HEMISPHERE_SOUTHEAST].sunlight = SUN_LIGHT;
  }

  sprintf(buf, "Setting up seasons: ");

  /* setup the seasons */
  if (time_info.month < 4) {
    hemispheres[HEMISPHERE_NORTHWEST].season = WINTER;
    hemispheres[HEMISPHERE_SOUTHWEST].season = SUMMER;
    hemispheres[HEMISPHERE_NORTHEAST].season = WINTER;
    hemispheres[HEMISPHERE_SOUTHEAST].season = SUMMER;
    strcat(buf, "Winter");
  } else if (time_info.month < 8) {
    hemispheres[HEMISPHERE_NORTHWEST].season = SPRING;
    hemispheres[HEMISPHERE_SOUTHWEST].season = AUTUMN;
    hemispheres[HEMISPHERE_NORTHEAST].season = SPRING;
    hemispheres[HEMISPHERE_SOUTHEAST].season = AUTUMN;
    strcat(buf, "Spring");
  } else if (time_info.month < 12) {
    hemispheres[HEMISPHERE_NORTHWEST].season = SUMMER;
    hemispheres[HEMISPHERE_SOUTHWEST].season = WINTER;
    hemispheres[HEMISPHERE_NORTHEAST].season = SUMMER;
    hemispheres[HEMISPHERE_SOUTHEAST].season = WINTER;
    strcat(buf, "Summer");
  } else {
    hemispheres[HEMISPHERE_NORTHWEST].season = AUTUMN;
    hemispheres[HEMISPHERE_SOUTHWEST].season = SPRING;
    hemispheres[HEMISPHERE_NORTHEAST].season = AUTUMN;
    hemispheres[HEMISPHERE_SOUTHEAST].season = SPRING;
    strcat(buf, "Autumn");
  }
  log(buf);

  log("Current Gametime: %dH %dD %dM %dY.",
      time_info.hours, time_info.day, time_info.month, time_info.year);

  log("Initializing weather.");
  init_weather();
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return count;
}



void index_boot(int mode)
{
  char *index_filename, *prefix;
  FILE *index, *db_file;
  int rec_count = 0;

  switch (mode) {
  case DB_BOOT_TRG:
    prefix = TRG_PREFIX;
    break;
  case DB_BOOT_WLD:
    prefix = WLD_PREFIX;
    break;
  case DB_BOOT_MOB:
    prefix = MOB_PREFIX;
    break;
  case DB_BOOT_OBJ:
    prefix = OBJ_PREFIX;
    break;
  case DB_BOOT_ZON:
    prefix = ZON_PREFIX;
    break;
  case DB_BOOT_SHP:
    prefix = SHP_PREFIX;
    break;
  case DB_BOOT_HLP:
    prefix = HLP_PREFIX;
    break;
  default:
    log("SYSERR: Unknown subcommand to index_boot!");
    exit(1);
    break;
  }
/* Ok, not sure we ever use this in mini-mud mode so I'm making it
   to where the index is loaded instead of the MINDEX_FILE even
   in mini_mud mode which is dev mode. */
  if (mini_mud)
    index_filename = INDEX_FILE;
  else
    index_filename = INDEX_FILE;

  sprintf(buf2, "%s/%s", prefix, index_filename);

  if (!(index = fopen(buf2, "r"))) {
    sprintf(buf1, "Error opening index file '%s'", buf2);
    perror(buf1);
    exit(1);
  }

  /* first, count the number of records in the file so we can malloc */
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s/%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      perror(buf2);
      log("file listed in index not found");
      exit(1);
    } else {
      if (mode == DB_BOOT_ZON)
        rec_count++;
      else
        rec_count += count_hash_records(db_file);
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }

  /* Exit if 0 records, unless this is shops */
  if (!rec_count) {
    if (mode == DB_BOOT_SHP)
      return;
    log("SYSERR: boot error - 0 records counted");
    exit(1);
  }

  rec_count++;

  switch (mode) {
  case DB_BOOT_TRG:
    CREATE(trig_index, struct index_data *, rec_count);
    break;
  case DB_BOOT_WLD:
    CREATE(world, struct room_data, rec_count);
    break;
  case DB_BOOT_MOB:
    CREATE(mob_proto, struct char_data, rec_count);
    CREATE(mob_index, struct index_data, rec_count);
    break;
  case DB_BOOT_OBJ:
    CREATE(obj_proto, struct obj_data, rec_count);
    CREATE(obj_index, struct index_data, rec_count);
    break;
  case DB_BOOT_ZON:
    CREATE(zone_table, struct zone_data, rec_count);
    break;
  case DB_BOOT_HLP:
    CREATE(help_table, struct help_index_element, rec_count * 2);
    break;
  }

  rewind(index);
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s/%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      perror(buf2);
      exit(1);
    }
    switch (mode) {
    case DB_BOOT_TRG:
    case DB_BOOT_WLD:
    case DB_BOOT_OBJ:
    case DB_BOOT_MOB:
      discrete_load(db_file, mode);
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_HLP:
      load_help(db_file);
      break;
    case DB_BOOT_SHP:
      boot_the_shops(db_file, buf2, rec_count);
      break;
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }

  /* sort the help index */
  if (mode == DB_BOOT_HLP) {
    qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
    top_of_helpt--;
  }}

int tmp_debug = 0;

void discrete_load(FILE * fl, int mode)
{
  int nr = -1, last = 0;
  char line[256];
  char *modes[] = {"world", "mob", "obj", "ZON", "SHP", "HLP", "trg"};
  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      {
        tmp_debug++;
        if (!get_line(fl, line)) {
          fprintf(stderr, "Format error after %s #%d\n", modes[mode], nr);
          fprintf(stderr, "Offending line: '%s'in file down list%d\n", line,
                  tmp_debug);
          exit(1);
        }
      }
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
        fprintf(stderr, "Format error after %s #%d\n", modes[mode], last);
        fprintf(stderr, "Offending line: '%s'\n", line);
        exit(1);
      }
      if (nr >= 198999)
        return;
      else
        switch (mode) {
        case DB_BOOT_TRG:
          parse_trigger(fl, nr);
          break;
        case DB_BOOT_WLD:
          parse_room(fl, nr);
          break;
        case DB_BOOT_MOB:
          parse_mobile(fl, nr);
          break;
        case DB_BOOT_OBJ:
          strcpy(line, parse_object(fl, nr));
          break;
        }
    } else {
      fprintf(stderr, "Format error in %s file near %s #%d\n",
              modes[mode], modes[mode], nr);
      fprintf(stderr, "Offending line: '%s'\n", line);
      exit(1);
    }
  }
}


long asciiflag_conv(char *flag)
{
  long flags = 0;
  register char *p;

  if (is_integer(flag))
    flags = atol(flag);
  else
    for (p = flag; *p; p++) {
      if (islower(*p))
        flags |= 1 << (*p - 'a');
      else if (isupper(*p))
        flags |= 1 << (26 + (*p - 'A'));
    }

  return flags;
}

char fread_letter(FILE *fp)
{
  char c;
  do {
    c = getc(fp);
  } while (isspace(c));
  return c;
}



/* load the rooms */
void parse_room(FILE * fl, int virtual_nr)
{
  static int room_nr = 0, zone = 0;
  int t[10], i;
  char line[256], flags[128];
  struct extra_descr_data *new_descr;
  char letter;

  if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1)) {
    fprintf(stderr, "Room #%d is below zone %d.\n", virtual_nr, zone);
    exit(1);
  }
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      fprintf(stderr, "Room %d is outside of any zone.\n", virtual_nr);
      exit(1);
    }
  world[room_nr].zone = zone;
  world[room_nr].vnum = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);

  if (!get_line(fl, line) || sscanf(line, " %d %s %d ", t, flags, t + 2) != 3) {
    fprintf(stderr, "Format error in room #%d\n", virtual_nr);
    exit(1);
  }
  /* t[0] is the zone number; ignored with the zone-file system */
  /* room_flags is a flagvector array */
  world[room_nr].room_flags[0] = asciiflag_conv(flags);
  world[room_nr].sector_type = t[2];

  world[room_nr].func = NULL;
  world[room_nr].contents = NULL;
  world[room_nr].people = NULL;
  world[room_nr].light = 0;        /* Zero light sources */

  for (i = 0; i < NUM_OF_DIRS; i++)
    world[room_nr].exits[i] = NULL;

  world[room_nr].ex_description = NULL;

  sprintf(buf, "Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      fprintf(stderr, "%s\n", buf);
      exit(1);
    }
    switch (*line) {
    case 'D':
      setup_dir(fl, room_nr, atoi(line + 1));
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      new_descr->next = world[room_nr].ex_description;
      world[room_nr].ex_description = new_descr;
      break;
    case 'S':                        /* end of room */
      letter = fread_letter(fl);
      ungetc(letter, fl);
      while (letter=='T') {
        dg_read_trigger(fl, &world[room_nr], WLD_TRIGGER);
        letter = fread_letter(fl);
        ungetc(letter, fl);
      }
      top_of_world = room_nr++;
      return;
      break;
    default:
      fprintf(stderr, "%s\n", buf);
      exit(1);
      break;
    }
  }
}

/* read direction data */
void setup_dir(FILE * fl, int room, int dir)
{
  int t[5];
  char line[256];
  char log_buf[100];

  sprintf(buf2, "room #%d, direction D%d", world[room].vnum, dir);
  /* added by gurlaek to stop memory leaks detected by insure++ 8/26/1999 */
  if(world[room].exits[dir]) {
    sprintf(log_buf,
          "SYSERR:db.c:setup_dir:creating direction [%d] for room %d twice!",
          dir, world[room].vnum);
    log(log_buf);
  } else {
    world[room].exits[dir] = create_exit(NOWHERE);
  }
  world[room].exits[dir]->general_description = fread_string(fl, buf2);
  world[room].exits[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    fprintf(stderr, "Format error, %s\n", buf2);
    exit(1);
  }
  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    fprintf(stderr, "Format error, %s\n", buf2);
    exit(1);
  }
  if (t[0] == 1)
    world[room].exits[dir]->exit_info = EX_ISDOOR;
  else if (t[0] == 2)
    world[room].exits[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else if (t[0] == 3) {
    world[room].exits[dir]->exit_info = EX_DESCRIPT;
    t[1] = -1;
    t[2] = -1;
  } else
    world[room].exits[dir]->exit_info = 0;

  world[room].exits[dir]->key = t[1];
  world[room].exits[dir]->to_room = t[2];
}


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
  extern int mortal_start_room;
  extern int immort_start_room;
  extern int frozen_start_room;

  if ((r_mortal_start_room = real_room(mortal_start_room)) < 0) {
    log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    exit(1);
  }
  if ((r_immort_start_room = real_room(immort_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(frozen_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
    r_frozen_start_room = r_mortal_start_room;
  }
}


/* resolve all vnums into rnums in the world */
void renum_world(void)
{
  int rnum;
  register int room, door;

  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[room].exits[door])
        if (world[room].exits[door]->to_room != NOWHERE) {
          rnum = real_room(world[room].exits[door]->to_room);
          world[room].exits[door]->to_room = rnum;
          if(rnum == NOWHERE) {
            sprintf(buf, "SYSERR:db.c:renum_world():Invalid exit to NOWHERE for dir %s in room %d", dirs[door], world[room].vnum);
            log(buf);
          }
        }
}



#define ZCMD zone_table[zone].cmd[cmd_no]

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void)
{
  int zone, cmd_no, a, b;

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
      a = b = 0;
      switch (ZCMD.command) {
      case 'M':
        a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
        b = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'O':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        if (ZCMD.arg3 != NOWHERE)
          b = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'G':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        break;
      case 'E':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        break;
      case 'P':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        b = ZCMD.arg3 = real_object(ZCMD.arg3);
        break;
      case 'D':
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
        break;
      case 'R': /* rem obj from room */
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
        b = ZCMD.arg2 = real_object(ZCMD.arg2);
        break;
      }
      if (a < 0 || b < 0) {
        if (!mini_mud)
          log_zone_error(zone, cmd_no, "Invalid vnum, cmd disabled");
        ZCMD.command = '*';
      }
    }
}



#define OLC_MOB(d)        ((d)->olc->mob)
void parse_simple_mob(FILE *mob_f, int i, int nr)
{
  int j, t[10];
  char line[256];
  long k = 0;
  struct char_data *mobproto;
  extern void roll_natural_abils(struct char_data *ch);

  get_line(mob_f, line);
  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
             t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
    fprintf(stderr, "Format error in mob #%d, first line after S flag\n"
            "...expecting line of form '# # # #d#+# #d#+#'\n", nr);
    exit(1);
  }
  GET_LEVEL(mob_proto + i) = t[0];
   /*  No negative hitroll bonus DO_NOT CHANGE -Banyal */
  mob_proto[i].mob_specials.ex_hitroll = LIMIT(0, t[1], 20);

  mob_proto[i].mob_specials.ex_armor = 10 * t[2];
  /*if ac onmedit is 0 then ignore it*/


  /* max hit = 0 is a flag that H, M, V is xdy+z */
  mob_proto[i].points.max_hit = 0;

  mob_proto[i].mob_specials.ex_max_hit = 0;
  mob_proto[i].mob_specials.ex_no_dice = t[3];
  mob_proto[i].mob_specials.ex_face = t[4];

  /*mob_proto[i].points.hit = t[3];
    mob_proto[i].points.mana = t[4];
  */

  mob_proto[i].points.move = t[5];
  mob_proto[i].points.max_mana = 10;
  mob_proto[i].points.max_move = 50;

  mob_proto[i].mob_specials.ex_damnodice = t[6];
  mob_proto[i].mob_specials.ex_damsizedice = t[7];
  mob_proto[i].mob_specials.ex_damroll = t[8];

  get_line(mob_f, line);
  if (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3) > 3)
    {
      GET_ZONE(mob_proto + i) = t[2];
      GET_EX_GOLD(mob_proto + i) = t[0];
      GET_EX_PLATINUM(mob_proto + i) = t[1];
    }else
      {        GET_ZONE(mob_proto + i) = (nr/100);
      GET_EX_GOLD(mob_proto + i) = 0;
      GET_EX_PLATINUM(mob_proto + i) = 0;

      }        GET_EX_GOLD(mob_proto + i) = t[0];



  get_line(mob_f, line);
  if ((sscanf(line, " %d %d %d %d %d %d %d %d ", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7))  > 3) {
    mob_proto[i].player.class = t[3];
    mob_proto[i].player.race = t[4];
    mob_proto[i].player.race_align = t[5];
    set_base_size(&(mob_proto[i]), t[6]);
    mob_proto[i].char_specials.position = t[0];
    mob_proto[i].mob_specials.default_pos = t[1];
    mob_proto[i].player.sex = t[2];
  } else {
    mob_proto[i].player.class = CLASS_DEFAULT;t[3] = CLASS_DEFAULT;
    mob_proto[i].player.race = DEFAULT_RACE;t[4] = DEFAULT_RACE;
    mob_proto[i].player.race_align = 0;
    set_base_size(&(mob_proto[i]), SIZE_OF_RACE(DEFAULT_RACE));
    mob_proto[i].char_specials.position = t[0];
    mob_proto[i].mob_specials.default_pos = t[1];
    mob_proto[i].player.sex = t[2];
  }
  mob_proto[i].char_specials.stance = STANCE_ALERT;
  if (!VALID_CLASSNUM((int)mob_proto[i].player.class))
     mob_proto[i].player.class = CLASS_DEFAULT;
  mobproto = &mob_proto[i];
  mobproto->player_specials = &dummy_mob; /* dummy player_specials for mobs */
  roll_natural_abils(mobproto);  /* mobs now get rolled just like PC's */
  mobproto->actual_abils = mobproto->natural_abils;  /* set the initail viewed abils */
  scale_attribs(mobproto);  /* this scales the attribs for race */

  mob_proto[i].player.height = 198;

  mob_proto[i].points.coins[PLATINUM] = 0;
  mob_proto[i].points.coins[GOLD] = 0;
  mob_proto[i].points.coins[SILVER] = 0;
  mob_proto[i].points.coins[COPPER] = 0;

        /*Money adder*/
  k = j = 0;
  k = get_copper(i);
  j = (60 * k) / 100;
  mob_proto[i].points.coins[PLATINUM] = j / PLATINUM_SCALE;
  j = ((20 * k) / 100) + (j % PLATINUM_SCALE);
  mob_proto[i].points.coins[GOLD] = j / GOLD_SCALE;
  j = ((18 * k) / 100) + (j % GOLD_SCALE);
  mob_proto[i].points.coins[SILVER] = j / SILVER_SCALE;
  if (mob_proto[i].player.level > 20)
    j = (k / 200) + (j % SILVER_SCALE);
  j = ((2 * k) / 100) + (j % SILVER_SCALE);
  mob_proto[i].points.coins[COPPER] = j / COPPER_SCALE;

  mob_proto[i].points.coins[PLATINUM] = MAX(0, mob_proto[i].points.coins[PLATINUM] + GET_EX_PLATINUM(mob_proto + i));
  mob_proto[i].points.coins[GOLD] = MAX(0, mob_proto[i].points.coins[GOLD] + GET_EX_GOLD(mob_proto + i));

  /*auto ac stuff*/
  /*        l = get_ac(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i));
        sprintf(buf, "get is: %d", l);
        mudlog(buf, BRF, LVL_IMPL, TRUE);
  */
  if ((mob_proto[i].mob_specials.ex_armor != 100))
    mob_proto[i].points.armor = mob_proto[i].mob_specials.ex_armor + get_ac(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i));
        else
          mob_proto[i].points.armor = (sh_int) get_ac(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i));



  /*num of hp dice is hit*/
  GET_EXP(mob_proto + i) = get_set_exp(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i),  GET_ZONE(mob_proto + i));
  GET_EXP(mob_proto + i) += GET_EX_EXP(mob_proto + i);

  mob_proto[i].points.mana = get_set_hit(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i), 2);
  mob_proto[i].points.hit = 7;;
  mob_proto[i].points.mana += mob_proto[i].mob_specials.ex_face;
  mob_proto[i].points.hit += mob_proto[i].mob_specials.ex_no_dice;
  GET_EX_MAIN_HP(mob_proto + i) = (get_set_hit(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i), 1));

  mob_proto[i].points.damroll        = get_set_hd(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i), 1);
  mob_proto[i].points.damroll        += mob_proto[i].mob_specials.ex_damroll;
   /*  Changed to get rid of the -199 mobb hitrolls Do NOT change -Banyal */
  mob_proto[i].points.hitroll = LIMIT(10, get_set_hd(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i),
                                                       GET_CLASS(mob_proto + i), 0), 80);
  mob_proto[i].points.hitroll += mob_proto[i].mob_specials.ex_hitroll;

  mob_proto[i].mob_specials.damnodice = get_set_dice(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i), 0);
  mob_proto[i].mob_specials.damnodice += mob_proto[i].mob_specials.ex_damnodice;
  mob_proto[i].mob_specials.damsizedice = get_set_dice(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i), 1);
  mob_proto[i].mob_specials.damsizedice += mob_proto[i].mob_specials.ex_damsizedice;

  /*last check values above 0*/
  mob_proto[i].mob_specials.damsizedice = MAX(0, mob_proto[i].mob_specials.damsizedice);
  mob_proto[i].mob_specials.damnodice = MAX(0, mob_proto[i].mob_specials.damnodice);
  mob_proto[i].points.hitroll = MAX(0, mob_proto[i].points.hitroll);
  mob_proto[i].points.damroll = MAX(0, mob_proto[i].points.damroll);
  mob_proto[i].points.armor = MIN(100, MAX(-100, mob_proto[i].points.armor));

  for (j = 0; j <= NUM_SPELL_CIRCLES; ++j)
    GET_MOB_SPLBANK(&mob_proto[i], j) = spells_of_circle[(int) GET_LEVEL(&mob_proto[i])][j];

  for (j = 0; j < 3; j++)
    GET_COND(mob_proto + i, j) = -1;

  /*
   * these are now save applies; base save numbers for MOBs are now from
   * the warrior save table.
   */
  for (j = 0; j < 5; j++)
    GET_SAVE(mob_proto + i, j) = 0;
  /* mobs now get the same spells/skills as pc's */
  /*set_mob_skills(i, t[3], t[4]);*/
}




/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))
/* modified for the 100 attrib scale */
void interpret_espec(char *keyword, char *value, int i, int nr)
{
  int num_arg, matched = 0;

  num_arg = atoi(value);

  CASE("BareHandAttack") {
    RANGE(0, 99);
    mob_proto[i].mob_specials.attack_type = num_arg;
  }

  CASE("Str") {
    RANGE(30, 100);
    mob_proto[i].natural_abils.str = num_arg;
  }

  CASE("Int") {
    RANGE(30, 100);
    mob_proto[i].natural_abils.intel = num_arg;
  }

  CASE("Wis") {
    RANGE(30, 100);
    mob_proto[i].natural_abils.wis = num_arg;
  }

  CASE("Dex") {
    RANGE(30, 100);
    mob_proto[i].natural_abils.dex = num_arg;
  }

  CASE("Con") {
    RANGE(30, 100);
    mob_proto[i].natural_abils.con = num_arg;
  }

  CASE("Cha") {
    RANGE(30, 100);
    mob_proto[i].natural_abils.cha = num_arg;
  }

  CASE("AFF2") {
     EFF_FLAGS(mob_proto + i)[1] = num_arg;
  }

  CASE("AFF3") {
     EFF_FLAGS(mob_proto + i)[2] = num_arg;
  }

  CASE("MOB2") {
     MOB_FLAGS(mob_proto + i)[1] = num_arg;
  }

  CASE("PERC") {
     GET_PERCEPTION(mob_proto + i) = num_arg;
  }

  CASE("HIDE") {
     GET_HIDDENNESS(mob_proto + i) = num_arg;
  }

  CASE("Lifeforce") {
     GET_LIFEFORCE(mob_proto + i) = num_arg;
  }

  CASE("Composition") {
     BASE_COMPOSITION(mob_proto + i) = num_arg;
     GET_COMPOSITION(mob_proto + i) = num_arg;
  }

  CASE("Stance") {
     GET_STANCE(mob_proto + i) = num_arg;
  }

  if (!matched) {
    fprintf(stderr, "Warning: unrecognized espec keyword %s in mob #%d\n",
            keyword, nr);
  }
}

#undef CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr)
{
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
  } else
    ptr = "";

  interpret_espec(buf, ptr, i, nr);
}


void parse_enhanced_mob(FILE *mob_f, int i, int nr)
{
  char line[256];

  parse_simple_mob(mob_f, i, nr);

  while (get_line(mob_f, line)) {
    if (!strcmp(line, "E"))        /* end of the ehanced section */
      return;
    else if (*line == '#') {        /* we've hit the next mob, maybe? */
      fprintf(stderr, "Unterminated E section in mob #%d\n", nr);
      exit(1);
    } else
      parse_espec(line, i, nr);
  }

  fprintf(stderr, "Unexpected end of file reached after mob #%d\n", nr);
  exit(1);
}


void parse_mobile(FILE * mob_f, int nr)
{
  static int i = 0;
  int j, t[10];
  char line[256], *tmpptr, letter;
  char f1[128], f2[128];

  mob_index[i].virtual = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;
  clear_char(mob_proto + i);

  sprintf(buf2, "mob vnum %d", nr);

  /***** String data *** */
  mob_proto[i].player.namelist = fread_string(mob_f, buf2);
  tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
        !str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);
  mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
  mob_proto[i].player.description = fread_string(mob_f, buf2);
  mob_proto[i].player.title = NULL;
  mob_proto[i].player.prompt = NULL;

  /* *** Numeric data *** */
  mob_proto[i].mob_specials.nr = i;
  get_line(mob_f, line);
  sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter);
  MOB_FLAGS(mob_proto + i)[0] = asciiflag_conv(f1);
  SET_FLAG(MOB_FLAGS(mob_proto + i), MOB_ISNPC);
  EFF_FLAGS(mob_proto + i)[0] = asciiflag_conv(f2);
  GET_ALIGNMENT(mob_proto + i) = t[2];

  switch (letter) {
  case 'S':        /* Simple monsters */
    parse_simple_mob(mob_f, i, nr);
    break;
  case 'E':        /* Circle3 Enhanced monsters */
    parse_enhanced_mob(mob_f, i, nr);
    break;
    /* add new mob types here.. */
  default:
    fprintf(stderr, "Unsupported mob type '%c' in mob #%d\n", letter, nr);
    exit(1);
    break;
  }

  letter = fread_letter(mob_f);
  ungetc(letter, mob_f);
  while (letter=='T') {
    dg_read_trigger(mob_f, &mob_proto[i], MOB_TRIGGER);
    letter = fread_letter(mob_f);
    ungetc(letter, mob_f);
  }


  mob_proto[i].affected_abils = mob_proto[i].natural_abils;

  for (j = 0; j < NUM_WEARS; j++)
    mob_proto[i].equipment[j] = NULL;

  mob_proto[i].desc = NULL;

  letter = fread_letter(mob_f);
  if (letter == '>')
  {
          while (fread_letter(mob_f) != '|');
          fprintf(stderr,"Mob %d has a mobprog still!\n",nr);
  }
  else
          ungetc(letter, mob_f);

  if (mob_proto[i].mob_specials.default_pos < 0 ||
        mob_proto[i].mob_specials.default_pos >= NUM_POSITIONS) {
     mob_proto[i].mob_specials.default_pos = POS_STANDING;
  }
  if (mob_proto[i].char_specials.position < 0 ||
        mob_proto[i].char_specials.position >= NUM_POSITIONS) {
     mob_proto[i].char_specials.position = POS_STANDING;
  }

  top_of_mobt = i++;
}

void verify_obj_spell(struct obj_data *obj, int valnum, bool zero_ok)
{
   if (!IS_SPELL(GET_OBJ_VAL(obj, valnum)) &&
         !(zero_ok && GET_OBJ_VAL(obj, valnum) == 0)) {
      mprintf(L_WARN, LVL_IMMORT,
            "ERROR: Invalid spell in object prototype. vnum=%d spellnum=%d",
            GET_OBJ_VNUM(obj), GET_OBJ_VAL(obj, valnum));
      /* Replace invalid spell with a benign value */
      if (zero_ok)
         GET_OBJ_VAL(obj, valnum) = 0;
      else
         GET_OBJ_VAL(obj, valnum) = SPELL_IDENTIFY;
   }
}


/* Do some post-definition processing on certain object types */
void init_obj_proto(struct obj_data *obj)
{
   char *s;
   struct extra_descr_data *ed;

   switch (GET_OBJ_TYPE(obj)) {
      case ITEM_LIGHT:
         /* Store a light's initial time as value 1, so the
          * illumination spell can restore the light accordingly. */
         GET_OBJ_VAL(obj, VAL_LIGHT_CAPACITY) = GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING);
         break;
      case ITEM_FOOD:
         /* Ensure that all food items have keyword "food". */
         if (!isname("food", obj->name)) {
            /* Prepare the new aliases string */
            /* length + "food" + space + terminator */
            s = (char *)malloc(strlen(obj->name) + 4 + 1 + 1);
            sprintf(s, "%s food", obj->name);

            /* If there is an extra description whose keywords exactly
             * match the object's aliases, give it "food" as well. */
            for (ed = obj->ex_description; ed; ed = ed->next)
               if (!strcmp(ed->keyword, obj->name)) {
                  free(ed->keyword);
                  ed->keyword = (char *)malloc(strlen(s) + 1);
                  strcpy(ed->keyword, s);
                  break;
               }

            free(obj->name);
            obj->name = s;
         }
         break;
      case ITEM_KEY:
         /* Prevent keys from being rented. */
         SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_NORENT);
         break;
      case ITEM_SCROLL:
         verify_obj_spell(obj, VAL_SCROLL_SPELL_1, FALSE);
         verify_obj_spell(obj, VAL_SCROLL_SPELL_2, TRUE);
         verify_obj_spell(obj, VAL_SCROLL_SPELL_3, TRUE);
         break;
      case ITEM_WAND:
         verify_obj_spell(obj, VAL_WAND_SPELL, FALSE);
         break;
      case ITEM_STAFF:
         verify_obj_spell(obj, VAL_STAFF_SPELL, FALSE);
         break;
      case ITEM_POTION:
         verify_obj_spell(obj, VAL_POTION_SPELL_1, FALSE);
         verify_obj_spell(obj, VAL_POTION_SPELL_2, TRUE);
         verify_obj_spell(obj, VAL_POTION_SPELL_3, TRUE);
         break;
   }

   /* Remove flags we don't want on prototypes */
   REMOVE_FLAG(GET_OBJ_FLAGS(obj), ITEM_WAS_DISARMED);
}

/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE * obj_f, int nr)
{
  static int i = 0, retval;
  static char line[256];
  int t[10], j;
  char *tmpptr;
  char f1[256], f2[256];
  struct extra_descr_data *new_descr, *edx;

  obj_index[i].virtual = nr;
  obj_index[i].number = 0;
  obj_index[i].func = NULL;

  clear_object(obj_proto + i);
  obj_proto[i].in_room = NOWHERE;
  obj_proto[i].item_number = i;

  sprintf(buf2, "object #%d", nr);

  /* *** string data *** */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL) {
    fprintf(stderr, "Null obj name or format error at or near %s\n", buf2);
    exit(1);
  }
  tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
  if (*tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
        !str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);

  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    *tmpptr = UPPER(*tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);

  /* *** numeric data *** */
  if (!get_line(obj_f, line) ||
      (retval = sscanf(line, " %d %s %s %d", t, f1, f2, t+1)) != 4) {
    if (retval == 3) {
      sscanf(line, " %d %s %s", t, f1, f2);
      t[1] = 0;
      fprintf(stderr, "Object #%d needs a level assigned to it.\n", nr);
    } else {
      fprintf(stderr, "Format error in first numeric line (expecting 4 args, got %d), %s\n", retval, buf2);
      /* exit(1);*/
    }
  }
  obj_proto[i].obj_flags.type_flag = t[0];
  obj_proto[i].obj_flags.extra_flags[0] = asciiflag_conv(f1);
  obj_proto[i].obj_flags.wear_flags = asciiflag_conv(f2);
  obj_proto[i].obj_flags.level = t[1]; /* Zantir 3/23/01 for level based objects */

  if (!get_line(obj_f, line) ||
      (retval = sscanf(line, "%d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6)) != 7) {
    fprintf(stderr, "Format error in second numeric line (expecting 7 args, got %d), %s\n", retval, buf2);
    /*exit(1);*/
  }
  obj_proto[i].obj_flags.value[0] = t[0];
  obj_proto[i].obj_flags.value[1] = t[1];
  obj_proto[i].obj_flags.value[2] = t[2];
  obj_proto[i].obj_flags.value[3] = t[3];
  obj_proto[i].obj_flags.value[4] = t[4];
  obj_proto[i].obj_flags.value[5] = t[5];
  obj_proto[i].obj_flags.value[6] = t[6];

  if (!get_line(obj_f, line) ||
      (retval = sscanf(line, "%f %d %d %d %d %d %d %d",
                       &obj_proto[i].obj_flags.weight,
                       t + 1, t + 2, t + 3, t+ 4, t + 5, t + 6, t + 7)) != 8)
    {
      fprintf(stderr, "Format error in third numeric line (expecting 8 args, got %d), %s\n", retval, buf2);
      /*exit(1);*/
    }

  obj_proto[i].obj_flags.cost = t[1];
  obj_proto[i].obj_flags.timer = t[2];
  obj_proto[i].obj_flags.effect_flags[0] = t[3];
/*    obj_proto[i].spell_component = t[4]; */
/*    obj_proto[i].object_limitation = t[5]; */
  obj_proto[i].obj_flags.effect_flags[1] = t[6];
  obj_proto[i].obj_flags.effect_flags[2] = t[7];

  /* *** extra descriptions and affect fields *** */

  for (j = 0; j < MAX_OBJ_APPLIES; j++) {
    obj_proto[i].applies[j].location = APPLY_NONE;
    obj_proto[i].applies[j].modifier = 0;
  }

  strcat(buf2, ", after numeric constants (expecting E/A/#xxx)");
  j = 0;

  for (;;) {
    if (!get_line(obj_f, line)) {
      fprintf(stderr, "Format error in %s\n", buf2);
      exit(1);
    }
    switch (*line) {
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(obj_f, buf2);
      new_descr->description = fread_string(obj_f, buf2);
      /* Put each extra desc at the end of the list as we read them.
       * Otherwise they will be reversed (yes, we really don't want that -
       * see act.informative.c, show_obj_to_char(), mode 5). */
      if ((edx = obj_proto[i].ex_description)) {
         for (;edx->next; edx = edx->next);
         edx->next = new_descr;
      } else
         obj_proto[i].ex_description = new_descr;
      break;
    case 'A':
      if (j >= MAX_OBJ_APPLIES) {
        fprintf(stderr, "Too many A fields (%d max), %s\n", MAX_OBJ_APPLIES, buf2);
        exit(1);
      }

      if (!get_line(obj_f, line) ||
          (retval = sscanf(line, " %d %d ", t, t + 1)) != 2)
        {
          fprintf(stderr, "Format error in Affect line (expecting 2 args, got %d), %s\n", retval, buf2);
          /*exit(1);*/
        }
      /* get_line(obj_f, line);
         sscanf(line, " %d %d ", t, t + 1); */
      obj_proto[i].applies[j].location = t[0];
      obj_proto[i].applies[j].modifier = t[1];
      j++;
      break;

    case 'H':  /* Hiddenness */
      get_line(obj_f, line);
      sscanf(line, "%d ", t);
      obj_proto[i].obj_flags.hiddenness = t[0];
      break;
    case 'T':  /* DG triggers */
      dg_obj_trigger(line, &obj_proto[i]);
      break;

    case '$':
    case '#':
      init_obj_proto(&obj_proto[i]);
      top_of_objt = i++;
      return line;
      break;
    default:
      fprintf(stderr, "Format error in %s\n", buf2);
      exit(1);
      break;
    }
  }
}


#define Z        zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE * fl, char *zonename)
{
  static int zone = 0;
  int cmd_no = 0, num_of_cmds = 0, line_num = 0, tmp, error;
  char *ptr, buf[256], zname[256];

  strcpy(zname, zonename);

  while (get_line(fl, buf) && buf[0] != '$')
    num_of_cmds++;

  rewind(fl);

  if (num_of_cmds == 0) {
    fprintf(stderr, "%s is empty!\n", zname);
    exit(0);
  } else
    CREATE(Z.cmd, struct reset_com, num_of_cmds);

  line_num += get_line(fl, buf);

  if (sscanf(buf, "#%d", &Z.number) != 1) {
    fprintf(stderr, "Format error in %s, line %d\n", zname, line_num);
    exit(0);
  }
  sprintf(buf2, "beginning of zone #%d", Z.number);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL)        /* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = strdup(buf);

  line_num += get_line(fl, buf);
  if (sscanf(buf, " %d %d %d %d %d %d", &Z.top, &Z.lifespan,
             &Z.reset_mode, &Z.zone_factor, &Z.hemisphere, &Z.climate) != 4)
    Z.zone_factor = 100;

  for (;;) {
    if ((tmp = get_line(fl, buf)) == 0) {
      fprintf(stderr, "Format error in %s - premature end of file\n", zname);
      exit(0);
    }
    line_num += tmp;
    ptr = buf;
    skip_spaces(&ptr);

    if ((ZCMD.command = *ptr) == '*')
      continue;

    ptr++;

    if (ZCMD.command == 'S' || ZCMD.command == '$') {
      ZCMD.command = 'S';
      break;
    }
    error = 0;
    if (ZCMD.command == 'F')  /* force mobile command */ {
      skip_spaces(&ptr);
      if (*ptr) {
        tmp = *ptr;
        ptr++;
        skip_spaces(&ptr);
        ZCMD.sarg = strdup(ptr);
      } else
        error = 1;
    } else
      if (strchr("MOEPD", ZCMD.command) == NULL) {        /* a 3-arg command */
        if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
          error = 1;
      } else {
        if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                   &ZCMD.arg3) != 4)
          error = 1;
      }

    ZCMD.if_flag = tmp;

    if (error) {
      fprintf(stderr, "Format error in %s, line %d: '%s'\n", zname,
              line_num, buf);
      exit(0);
    }
    ZCMD.line = line_num;
    cmd_no++;
  }

  top_of_zone_table = zone++;
}

#undef Z

void get_one_line(FILE *fl, char *line_buf)
{
  *line_buf = '\0';

  if (fgets(line_buf, READ_SIZE, fl) == NULL) {
    log("error reading help file: not terminated with $?");
    exit(1);
  }

  line_buf[strlen(line_buf) - 1] = '\0'; /* take off the trailing \n */
}


void load_help(FILE *fl)
{
  char key[READ_SIZE+1], next_key[READ_SIZE+1], entry[32384];
  char line[READ_SIZE+1], *scan;
  struct help_index_element el;

  /* get the first keyword line */
  get_one_line(fl, key);

  if(!strlen(key)) {
    log("Illegal blank line in help file");
    abort();
  }
  while (*key != '$') {
    /* read in the corresponding help entry */
    strcpy(entry, strcat(key, "\r\n"));
    get_one_line(fl, line);
    while (*line != '#') {
      strcat(entry, strcat(line, "\r\n"));
      get_one_line(fl, line);
    }

    el.min_level = 0;
    if ((*line == '#') && (*(line + 1) != 0))
      el.min_level = atoi((line + 1));

    el.min_level = MAX(0, MIN(el.min_level, LVL_IMPL));

    /* now, add the entry to the index with each keyword on the keyword line */
    el.duplicate = 0;
    el.entry = strdup(entry);
    scan = one_word(key, next_key);
    while (*next_key) {
      el.keyword = strdup(next_key);
      help_table[top_of_helpt++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
    }

    /* get next keyword line (or $) */
    get_one_line(fl, key);

    if(!strlen(key)) {
      log("Illegal blank line in help file");
      abort();
    }
  }
}


int hsort(const void *a, const void *b)
{
  struct help_index_element *a1, *b1;

  a1 = (struct help_index_element *) a;
  b1 = (struct help_index_element *) b;

  return (str_cmp(a1->keyword, b1->keyword));
}

void free_help_table(void)
{
  if (help_table) {
    int hp;
    for (hp = 0; hp <= top_of_helpt; hp++) {
      if (help_table[hp].keyword)
        free(help_table[hp].keyword);
      if (help_table[hp].entry && !help_table[hp].duplicate)
        free(help_table[hp].entry);
    }
    free(help_table);
    help_table = NULL;
  }
  top_of_helpt = 0;
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time                  *
*********************************************************************** */



int vnum_zone(char *searchname, struct char_data * ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_zone_table; nr++) {
    if (isname(searchname, zone_table[nr].name)) {
      sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
              zone_table[nr].number,
              zone_table[nr].name);
      send_to_char(buf, ch);
    }
  }
  return (found);
}


/* create a character, and add it to the char list */
struct char_data *create_char(void)
{
  struct char_data *ch;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  ch->next = character_list;
  character_list = ch;
  GET_ID(ch) = max_id++;
  return ch;
}

/* set up mob for loading. for use in read_mobile and anywhere else we want
 * to make a mob from scratch -- mostly copied n pasted from read_mobile - 321
 */
void setup_mob(struct char_data *mob)
{
  extern void roll_natural_abils(struct char_data *ch);

  mob->player_specials = &dummy_mob;
  /* mobs now get rolled just like PC's --gurlaek 6/28/1999 */
  roll_natural_abils(mob);
  mob->actual_abils = mob->natural_abils;
  scale_attribs(mob);

  /* Set skills, innates, and other things according to class and species: */
  init_char(mob);

  if (!mob->points.max_hit)
    mob->points.max_hit = MAX(
          0,
          MIN(
             32000,
             dice(mob->points.hit, mob->points.mana) +
             GET_EX_MAIN_HP(mob) +
             mob->points.move));
  else
    mob->points.max_hit = number(mob->points.hit, mob->points.mana);

  mob->points.mana = mob->points.max_mana; /* rests mobs mana  */
  mob->points.max_move = natural_move(mob);
  mob->points.move = mob->points.max_move; /* resets mob moves */
  mob->points.hit = mob->points.max_hit;   /* sets full hps    */

  mob->player.time.birth = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon = time(0);

  assign_triggers(mob, MOB_TRIGGER);
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(int nr, int type)
{
  int i;
  struct char_data *mob;

  if (type == VIRTUAL)
  {
    if ((i = real_mobile(nr)) < 0)
    {
      sprintf(buf, "Mobile (V) %d does not exist in database.", nr);
      return (0);
    }
  }
  else
    i = nr;

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  *mob = mob_proto[i];

  setup_mob(mob);
  mob_index[i].number++;
  GET_ID(mob) = max_id++;

  mob->next = character_list;
  character_list = mob;

  return mob;
}

/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next = object_list;
  object_list = obj;
  GET_ID(obj) = max_id++;
  assign_triggers(obj, OBJ_TRIGGER);

  return obj;
}


/* create a new object from a prototype */
struct obj_data *read_object(int nr, int type)
{
  struct obj_data *obj;
  int i;

  if (nr < 0) {
    log("SYSERR: trying to create obj with negative num!");
    return NULL;
  }
  if (type == VIRTUAL) {
    if ((i = real_object(nr)) < 0) {
      sprintf(buf, "Object (V) %d does not exist in database.", nr);
      return NULL;
    }
  } else
    i = nr;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;

  obj_index[i].number++;
  GET_ID(obj) = max_id++;
  assign_triggers(obj, OBJ_TRIGGER);

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
     setup_drinkcon(obj, -1);

  return obj;
}



#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;
  char buf[128];

  /* jelson 10/22/92 */
  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    /* one minute has passed */
    /*
     * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
     * factor of 60
     */

    timer = 0;

    /* since one minute has passed, increment zone ages */
    for (i = 0; i <= top_of_zone_table; i++) {
      if (zone_table[i].age < zone_table[i].lifespan &&
          zone_table[i].reset_mode)
        (zone_table[i].age)++;

      if (zone_table[i].age >= zone_table[i].lifespan &&
          zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
        /* enqueue zone */

        CREATE(update_u, struct reset_q_element, 1);

        update_u->zone_to_reset = i;
        update_u->next = 0;

        if (!reset_q.head)
          reset_q.head = reset_q.tail = update_u;
        else {
          reset_q.tail->next = update_u;
          reset_q.tail = update_u;
        }

        zone_table[i].age = ZO_DEAD;
      }
    }
  }        /* end - one minute has passed */


  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
    if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
        is_empty(update_u->zone_to_reset)) {
      reset_zone(update_u->zone_to_reset, FALSE);
      sprintf(buf, "Auto zone reset: %s",
              zone_table[update_u->zone_to_reset].name);
      mudlog(buf, CMP, LVL_GOD, FALSE);
      /* dequeue */
      if (update_u == reset_q.head)
        reset_q.head = reset_q.head->next;
      else {
        for (temp = reset_q.head; temp->next != update_u;
             temp = temp->next);

        if (!update_u->next)
          reset_q.tail = temp;

        temp->next = update_u->next;
      }

      free(update_u);
      break;
    }
}

void log_zone_error(int zone, int cmd_no, char *message)
{
  char buf[256];

  sprintf(buf, "SYSERR: error in zone file: %s", message);
  mudlog(buf, NRM, LVL_GOD, TRUE);

  sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
          ZCMD.command, zone_table[zone].number, ZCMD.line);
  mudlog(buf, NRM, LVL_GOD, TRUE);
}

#define ZONE_ERROR(message) \
        { log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* reset_door: returns 0 on success, 1, on invalid command */
int reset_door(room_num roomnum, int dir, int resetcmd) {
   switch (resetcmd) {
      case 0:
         REMOVE_BIT(world[roomnum].exits[dir]->exit_info, EX_LOCKED);
         REMOVE_BIT(world[roomnum].exits[dir]->exit_info, EX_CLOSED);
         REMOVE_BIT(world[roomnum].exits[dir]->exit_info, EX_HIDDEN);
         break;
      case 1:
         SET_BIT(world[roomnum].exits[dir]->exit_info, EX_CLOSED);
         REMOVE_BIT(world[roomnum].exits[dir]->exit_info, EX_LOCKED);
         REMOVE_BIT(world[roomnum].exits[dir]->exit_info, EX_HIDDEN);
         break;
      case 2:
         SET_BIT(world[roomnum].exits[dir]->exit_info, EX_LOCKED);
         SET_BIT(world[roomnum].exits[dir]->exit_info, EX_CLOSED);
         REMOVE_BIT(world[roomnum].exits[dir]->exit_info, EX_HIDDEN);
         break;
      case 3:
         REMOVE_BIT(world[roomnum].exits[dir]->exit_info, EX_CLOSED);
         SET_BIT(world[roomnum].exits[dir]->exit_info, EX_HIDDEN);
         REMOVE_BIT(world[roomnum].exits[dir]->exit_info, EX_LOCKED);
/* allow hidden, but putting descr when no door seems to screw things up
**          SET_BIT(world[roomnum].exits[dir]->exit_info,
                  EX_DESCRIPT);
*/
          break;
      case 4:
         SET_BIT(world[roomnum].exits[dir]->exit_info, EX_CLOSED);
         SET_BIT(world[roomnum].exits[dir]->exit_info, EX_LOCKED);
         SET_BIT(world[roomnum].exits[dir]->exit_info, EX_HIDDEN);
         break;
      case 5:
         SET_BIT(world[roomnum].exits[dir]->exit_info, EX_CLOSED);
         REMOVE_BIT(world[roomnum].exits[dir]->exit_info, EX_LOCKED);
         SET_BIT(world[roomnum].exits[dir]->exit_info, EX_HIDDEN);
         break;
      default:
         return 1;
   }
   return 0;
}

/* execute the reset command table of a given zone */
void reset_zone(int zone, byte pop)
{
  int cmd_no, cmd_other, last_cmd = 0;
  struct char_data *mob = NULL;
  struct obj_data *obj, *obj_to;
  room_num other_room;
  struct reset_com *ocmd;

  for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {

    if (ZCMD.if_flag && !last_cmd)
      continue;

    switch (ZCMD.command) {
    case '*':                        /* ignore command */
      last_cmd = 0;
      break;

    case 'F':           /* force mobile to do action */
      if (!mob) {
        ZONE_ERROR("attempt to force-command a non-existent mob");
        break;
      }
      command_interpreter(mob, ZCMD.sarg);
      break;

    case 'M':                        /* read a mobile */
      if (mob_index[ZCMD.arg1].number < ZCMD.arg2) {
        mob = read_mobile(ZCMD.arg1, REAL);
        char_to_room(mob, ZCMD.arg3);
        load_mtrigger(mob);
        last_cmd = 1;
      } else
        last_cmd = 0;
      break;

    case 'O':                        /* read an object */

      if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
        {
          obj = read_object(ZCMD.arg1, REAL);
          if (ZCMD.arg3 >= 0)
            {
            obj_to_room(obj, ZCMD.arg3);
            last_cmd = 1;
            }
          else
            {
                  obj->in_room = NOWHERE;
                  last_cmd = 1;
            }
        }
      else
        last_cmd = 0;
      break;

    case 'P':                        /* object to object */
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
        {
          obj = read_object(ZCMD.arg1, REAL);
              if (!(obj_to = find_obj_in_world(find_by_rnum(ZCMD.arg3))))
                {
                  ZONE_ERROR("target obj not found");
                  extract_obj(obj);
                  break;
                }
              obj_to_obj(obj, obj_to);
              last_cmd = 1;
        }
      else
        last_cmd = 0;
      break;

    case 'G':                        /* obj_to_char */
      if (!mob)
        {
          ZONE_ERROR("attempt to give obj to non-existent mob");
          break;
        }

      if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
        {
          obj = read_object(ZCMD.arg1, REAL);

          if (GET_LEVEL(mob) < GET_OBJ_LEVEL(obj)) {
            char error_buf[2048];
            sprintf(error_buf, "Mob %s [%d] in room %d  cannot use object "
                    "%s [%d] because its level is to low.",
                    GET_NAME(mob), GET_MOB_VNUM(mob),
                    mob->in_room,
                    obj->short_description, GET_OBJ_VNUM(obj));
          }


              obj_to_char(obj, mob);
              last_cmd = 1;
        }
      else
        last_cmd = 0;
      break;

    case 'E':                        /* object to equipment list */
      if (!mob) {
         ZONE_ERROR("trying to equip non-existent mob");
         break;
      }/*was make sure number less then ZCMD.arg2*/
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
         if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
            ZONE_ERROR("invalid equipment pos number");
         } else {
            obj = read_object(ZCMD.arg1, REAL);
            if (equip_char(mob, obj, ZCMD.arg3) != EQUIP_RESULT_SUCCESS) {
               mprintf(L_ERROR, 101, "EQUIP zone command for %s [%d] in room %d to equip %s [%d] failed.",
                     GET_NAME(mob), GET_MOB_VNUM(mob),
                     ROOM_RNUM_TO_VNUM(mob->in_room),
                     obj->short_description, GET_OBJ_VNUM(obj));
               extract_obj(obj);
            }
            last_cmd = 1;
         }
     } else
         last_cmd = 0;
      break;

    case 'R': /* rem obj from room */
      if ((obj = find_obj_in_list(world[ZCMD.arg1].contents, find_by_rnum(ZCMD.arg2))) != NULL) {
        obj_from_room(obj);
        extract_obj(obj);
      }
      last_cmd = 1;
      break;


      case 'D':                        /* set state of door */
         if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
               (world[ZCMD.arg1].exits[ZCMD.arg2] == NULL)) {
            ZONE_ERROR("door does not exist");
         } else if (reset_door(ZCMD.arg1, ZCMD.arg2, ZCMD.arg3)) {
            ZONE_ERROR("unknown cmd in reset table; cmd disabled");
            ZCMD.command = '*';
         } else {
            /* reset_door() has returned 0, indicating success */

            /* If the destination room is in another zone, we must perform
             * its door reset commands now, else the opposing sides of the
             * door will become unsynchronized. */

            /* Make sure there is an actual destination room */
            if ((other_room = world[ZCMD.arg1].exits[ZCMD.arg2]->to_room)
                  != NOWHERE) {

               /* Make sure the destination room is in a different zone */
               if (world[other_room].zone != world[ZCMD.arg1].zone) {

                  /* Make sure the destination room has an exit pointing back */
                  if (world[other_room].exits[rev_dir[ZCMD.arg2]]) {

                     /* Make sure that exit is pointing back to this room */
                     if (world[other_room].exits[rev_dir[ZCMD.arg2]]->to_room
                           == ZCMD.arg1) {

                        /* Ready to perform its door zone commands */

                        /* Must examine every zone command in that zone */
                        for (cmd_other = 0;; cmd_other++) {
                           ocmd = &(zone_table[world[other_room].zone].cmd[cmd_other]);
                           if (ocmd->command == 'S') break;

                           /* if it's a door command */
                           if (ocmd->command == 'D' &&
                                 /* and it's applied to the room opposite this exit */
                                 ocmd->arg1 == other_room &&
                                 /* and it's going in the direction toward this room */
                                 ocmd->arg2 == rev_dir[ZCMD.arg2]) {

                              /* do the command */
                              reset_door(ocmd->arg1, ocmd->arg2, ocmd->arg3);
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
   zone_table[zone].age = 0;
}



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(int zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected)
      if (world[i->character->in_room].zone == zone_nr)
        return 0;

  return 1;
}


/* now scaled for 100 point attribs */
int static_ac(int dex)
{
  if (dex <= 32)
    return 10;
  else if (dex <= 40)
    return 15;
  else if (dex <= 56)
    return 20;
  else if (dex <= 64)
    return 25;
  else if (dex <= 72)
    return 30;
  else if (dex <= 76)
    return 35;
  else if (dex <= 80)
    return 40;
  else if (dex <= 84)
    return 45;
  else
    return 50;
}

/* fixed for the 100 point scale */
int con_aff(struct char_data *ch)
{
  int bonus = 0;

  if (GET_CON(ch) < 10)
    bonus = GET_LEVEL(ch) * -1;
  else if (GET_CON(ch) < 68)
    bonus = 0;
  else if (GET_CON(ch) < 76)
    bonus = GET_LEVEL(ch) * 1;
  else if (GET_CON(ch) < 84)
    bonus = GET_LEVEL(ch) * 2;
  else if (GET_CON(ch) < 92)
    bonus = GET_LEVEL(ch) * 2;
  else if (GET_CON(ch) < 96)
    bonus = GET_LEVEL(ch) * 3;
  else
    bonus = GET_LEVEL(ch) * 4;

  return bonus;
}



/************************************************************************
 *  funcs of a (more or less) general utility nature                        *
 ************************************************************************/


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
  register char *point;
  int done = 0, length = 0, templength = 0;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      fprintf(stderr, "SYSERR: fread_string: format error at or near %s\n",
              error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    if ((point = strchr(tmp, '~')) != NULL) {
      *point = '\0';
      done = 1;
    } else {
      point = tmp + strlen(tmp) - 1;
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
    }

    templength = strlen(tmp);

    if (length + templength >= MAX_STRING_LENGTH) {
      log("SYSERR: fread_string: string too large (db.c)");
      log(error);
      exit(1);
    } else {
      strcat(buf + length, tmp);
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  if (strlen(buf) > 0) {
    CREATE(rslt, char, length + 1);
    strcpy(rslt, buf);
  } else
    rslt = NULL;

  return rslt;
}


/* release memory allocated for a char struct */
void free_char(struct char_data *ch)
{
  int i;
  struct know_spell *tmp, *tmp2;

  /* Generally speaking, extract_char() should have called char_from_room()
   * which would have cleared all battling variables.  So if ch is *still*
   * in some kind of battle mode here, something is definitely wrong. */
  if (ch->attackers || ch->target)
    mprintf(L_ERROR, LVL_GOD, "free_char: %s is in battle", GET_NAME(ch));

  while (ch->effects)
    effect_remove(ch, ch->effects);

  /* free spell recognition list if it exists */
  for (tmp = ch->see_spell; tmp; tmp = tmp2) {
    tmp2 = tmp->next;
    free(tmp);
  }
  ch->see_spell = NULL;

  if (ch->casting.misc)
    free(ch->casting.misc);

  if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
    free_trophy(ch);
    free_aliases(GET_ALIASES(ch));

    /* Remove runtime link to clan */
    if (GET_CLAN_MEMBERSHIP(ch))
      GET_CLAN_MEMBERSHIP(ch)->player = NULL;

    if (GET_WIZ_TITLE(ch))
      free(GET_WIZ_TITLE(ch));
    if (GET_PERM_TITLES(ch)) {
      for (i = 0; GET_PERM_TITLES(ch)[i]; ++i)
        free(GET_PERM_TITLES(ch)[i]);
      free (GET_PERM_TITLES(ch));
    }
    if (GET_POOFIN(ch))
      free(GET_POOFIN(ch));
    if (GET_POOFOUT(ch))
      free(GET_POOFOUT(ch));
    if (GET_HOST(ch))
      free(GET_HOST(ch));

    if (GET_GRANT_CACHE(ch))
      free(GET_GRANT_CACHE(ch));
    if (GET_REVOKE_CACHE(ch))
      free(GET_REVOKE_CACHE(ch));

    free(ch->player_specials);
  }


  if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == -1)) {
    /* if this is a player, or a non-prototyped non-player, free all */
    if (GET_NAMELIST(ch))
      free(GET_NAMELIST(ch));
    if (ch->player.title)
      free(ch->player.title);
    if (ch->player.prompt)
      free(ch->player.prompt);
    if (GET_NAME(ch))
      free(GET_NAME(ch));
    if (GET_LDESC(ch))
      free(GET_LDESC(ch));
    if (ch->player.description)
      free(ch->player.description);
  }
  else if ((i = GET_MOB_RNUM(ch)) > -1) {
    /* otherwise, free strings only if the string is not pointing at proto */
    if (ch->player.namelist && ch->player.namelist != mob_proto[i].player.namelist)
      free(ch->player.namelist);
    if (ch->player.title && ch->player.title != mob_proto[i].player.title)
      free(ch->player.title);
    if (ch->player.prompt && ch->player.prompt != mob_proto[i].player.prompt)
      free(ch->player.prompt);
    if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
      free(ch->player.short_descr);
    if (GET_LDESC(ch) && GET_LDESC(ch) != mob_proto[i].player.long_descr)
      free(GET_LDESC(ch));
    if (ch->player.description && ch->player.description != mob_proto[i].player.description)
      free(ch->player.description);
  }

  if (IS_NPC(ch))
    clear_memory(ch);
  else {
    free_mem_list(ch);
    free_scribe_list(ch);
  }

  /*
   * Take out events (again).  extract_char removes events, but since
   * the load player process may attach a cooldown event, and loaded
   * players are frequently not extracted, we need to remove events
   * when freeing too.
   */
  if (ch->events)
     cancel_event_list(&(ch->events));
  for (i = 0; i < EVENT_FLAG_FIELDS; ++i)
    ch->event_flags[i] = 0;

  if (SCRIPT(ch))
    extract_script(SCRIPT(ch));

  free_quest_list(ch);

  free(ch);
}




/* release memory allocated for an obj struct */
void free_obj(struct obj_data * obj)
{
  struct spell_book_list *this_book, *next_book;

  free_obj_strings(obj);

  for (this_book = obj->spell_book; this_book; this_book = next_book) {
    next_book = this_book->next;
    free(this_book);
  }

  if (SCRIPT(obj))
    extract_script(SCRIPT(obj));

  free(obj);
}



/* read contets of a text file, alloc space, point buf to it */
int file_to_string_alloc(const char *name, char **buf)
{
  char temp[MAX_STRING_LENGTH];

  if (*buf)
    free(*buf);

  if (file_to_string(name, temp) < 0) {
    *buf = "";
    return -1;
  } else {
    *buf = strdup(temp);
    return 0;
  }
}


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
  FILE *fl;
  char tmp[READ_SIZE+3];

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    sprintf(tmp, "Error reading %s", name);
    perror(tmp);
    return (-1);
  }
  /*  Rewritten to truncate text files read in as opposed to overflowing
   *  and crashing the mud.  Typed by Zzur, told to type by Gurlaek
   *  10/4/99
   */
  while (fgets(tmp, READ_SIZE, fl)) {
    tmp[strlen(tmp) - 1] = '\0';
    strcat(tmp, "\r\n");
    if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
      fclose(fl);
      return 0;
    }
    strcat(buf, tmp);
  }
  fclose(fl);
  return (0);
}



/* clear some of the the working variables of a char */
void reset_char(struct char_data * ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    GET_EQ(ch, i) = NULL;
  REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
  CONSENT(ch) = NULL;
  ch->scribe_list = NULL;
  ch->followers = NULL;
  ch->master = NULL;
  ch->groupees = NULL;
  ch->group_master = NULL;
  ch->followers = NULL;
  ch->master = NULL;
  ch->in_room = NOWHERE;
  ch->carrying = NULL;
  ch->next = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room = NULL;
  ch->target = NULL;
  ch->attackers = NULL;
  ch->next_attacker = NULL;
  GET_POS(ch) = POS_STANDING;
  GET_STANCE(ch) = STANCE_ALERT;
  GET_DEFAULT_POS(ch) = POS_STANDING;
  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = GET_MAX_HIT(ch) * .10;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_MANA(ch) <= 0)
    GET_MANA(ch) = 1;
  if (GET_BASE_HIT(ch) <= 0)
    GET_BASE_HIT(ch) = GET_MAX_HIT(ch);
  check_regen_rates(ch); /* start regening points */

  GET_LAST_TELL(ch) = NOBODY;
  init_mem_list(ch);
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data * ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));

  ch->in_room = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  GET_STANCE(ch) = STANCE_ALERT;
  ch->mob_specials.default_pos = POS_STANDING;
  clear_cooldowns(ch);
  GET_AC(ch) = 100;                /* Basic Armor */
  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;
}


void clear_object(struct obj_data * obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));

  obj->item_number = NOTHING;
  obj->in_room = NOWHERE;
}


/* returns the real number of the room with given virtual number */
int real_room(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_world;

  /* perform binary search on world-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((world + mid)->vnum == virtual)
      return mid;
    if (bot >= top)
      return NOWHERE;
    if ((world + mid)->vnum > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}


int real_quest(unsigned short virtual)
{
  int bot, top, mid;

  if (!all_quests) {
    return NOWHERE;
  }

  bot = 0;
  top = max_quests;

  /* perform binary search on quest-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((all_quests + mid)->quest_id == virtual)
      return mid;
    if (bot >= top)
      return NOWHERE;
    if ((all_quests + mid)->quest_id > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/* returns the real number of the monster with given virtual number */
int real_mobile(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->virtual == virtual)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((mob_index + mid)->virtual > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}




/* returns the real number of the object with given virtual number */
int real_object(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->virtual == virtual)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((obj_index + mid)->virtual > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/*
 * Read a number from a file.
 */
int fread_number(FILE *fp)
{
  int number;
  bool sign;
  char c;

  do {
    c = getc(fp);
  } while (isspace(c));

  number = 0;

  sign   = FALSE;
  if (c == '+') {
    c = getc(fp);
  } else if (c == '-') {
    sign = TRUE;
    c = getc(fp);
  }


  if (!isdigit(c)) {
    log("Fread_number: bad format.");
    exit(1);
  }

  while (isdigit(c)) {
    number = number * 10 + c - '0';
    c      = getc(fp);
  }

  if (sign)
    number = 0 - number;

  if (c == '|')
    number += fread_number(fp);
  else if (c != ' ')
    ungetc(c, fp);

  return number;
}

/*
 * Read to end of line (for comments).
 */
void fread_to_eol(FILE *fp)
{
  char c;

  do {
    c = getc(fp);
  } while (c != '\n' && c != '\r');

  do {
    c = getc(fp);
  } while (c == '\n' || c == '\r');

  ungetc(c, fp);
  return;
}


/*
 * Read one word (into static buffer).
 */
char *fread_word(FILE *fp)
{
  static char word[MAX_INPUT_LENGTH];
  char *pword;
  char cEnd;

  do
    {
      cEnd = getc(fp);
    }
  while (isspace(cEnd));

  if (cEnd == '\'' || cEnd == '"')
    {
      pword   = word;
    }
  else
    {
      word[0] = cEnd;
      pword   = word+1;
      cEnd    = ' ';
    }

  for (; pword < word + MAX_INPUT_LENGTH; pword++)
    {
      *pword = getc(fp);
      if (cEnd == ' ' ? isspace(*pword) || *pword == '~' : *pword == cEnd)
        {
          if (cEnd == ' ' || cEnd == '~')
            ungetc(*pword, fp);
          *pword = '\0';
          return word;
        }
    }

  log("SYSERR: Fread_word: word too long.");
        exit(1);
        return NULL;
}


struct index_data *get_obj_index (int vnum)
{
  int nr;
  for(nr = 0; nr <= top_of_objt; nr++) {
    if(obj_index[nr].virtual == vnum) return &obj_index[nr];
  }
  return NULL;
}

struct index_data *get_mob_index (int vnum)
{
  int nr;
  for(nr = 0; nr <= top_of_mobt; nr++) {
    if(mob_index[nr].virtual == vnum) return &mob_index[nr];
  }
  return NULL;
}

bool _parse_name(char *arg, char *name)
{
  int i;
  char test[32];
  const char *smart_ass[] =
  {
    "someone",
    "somebody",
    "me",
    "self",
    "all",
    "group",
    "local",
    "them",
    "they",
    "nobody",
    "any",
    "something",
    "other",
    "no",
    "yes",
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "shape",                        /* infra.. */
    "shadow",                        /* summon */
    "\n"
  };

  test[0] = 0;
  for (i = 0; (*name = *arg); arg++, i++, name++) {
    *(test + i) = LOWER(*arg);
    if ((*arg < 0) || !isalpha(*arg) || (i > 15) ||
        (i && (*(test + i) != *arg)))
      return (TRUE);
  }

  if (i < 2)
    return (TRUE);

  /* We need the case-insensitive search_block since arg can have uppercase */
  for (i = 0; *cmd_info[i].command != '\n'; ++i)
    if (!str_cmp(arg, cmd_info[i].command))
      return TRUE;
  if (search_block(arg, smart_ass, TRUE) >= 0)
    return TRUE;

  return FALSE;
}

/*
 * New functions used by ASCII files.
 */

/* Separate a 126-character id tag from the data it precedes */
void tag_argument(char *argument, char *tag) {
  char *tmp = argument, *ttag = tag, *wrt = argument;
  int i;

  for (i = 0; i < 126 && *tmp && *tmp != ' ' && *tmp != ':'; i++)
    *(ttag++) = *(tmp++);
  *ttag = '\0';
  while(*tmp == ':' || *tmp == ' ')
    tmp++;

  while(*tmp)
    *(wrt++) = *(tmp++);
  *wrt = '\0';
}

/* remove ^M's from file output */
/* There is a *similar* function in Oasis, but it has a bug. */
void kill_ems(char *str)
{
  char *ptr1, *ptr2, *tmp;

  tmp = str;
  ptr1 = str;
  ptr2 = str;

  while(*ptr1) {
    if (*ptr1 == '\r')
      ptr1++;
    else {
      *(ptr2++) = *(ptr1++);
    }
  }
  *ptr2 = '\0';
}


/***************************************************************************
 * $Log: db.c,v $
 * Revision 1.194  2010/06/05 15:09:42  mud
 * Moving cooldowns to their own file.
 *
 * Revision 1.193  2009/07/16 19:15:54  myc
 * Moved command stuff from grant.c to commands.c
 *
 * Revision 1.192  2009/06/09 05:37:23  myc
 * Simplifying code in reset_time that prints out init logs.
 *
 * Revision 1.191  2009/03/20 23:02:59  myc
 * Move text file handling routines into text.c, including
 * the reload command.
 *
 * Revision 1.190  2009/03/20 15:00:40  jps
 * Check object spell validity while reading prototypes.
 *
 * Revision 1.189  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.188  2009/03/09 05:59:57  myc
 * The mud now keeps track of all previous boot times, including
 * hotboot times.
 *
 * Revision 1.187  2009/03/09 05:41:31  jps
 * Moved money stuff into money.h, money.c
 *
 * Revision 1.186  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.185  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.184  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.183  2009/03/08 03:54:13  jps
 * Minor code reformatting
 *
 * Revision 1.182  2009/03/03 19:43:44  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.181  2009/02/21 03:30:16  myc
 * Added hooks for new board system.  Removed L_FILE flag--mprintf
 * now logs to file by default; assert L_NOFILE to prevent that.
 *
 * Revision 1.180  2008/09/29 05:04:33  jps
 * Stop calculating liquid container weights when loading protos.
 *
 * Revision 1.179  2008/09/26 18:37:49  jps
 * Don't use FIGHTING macro when changing target.
 *
 * Revision 1.178  2008/09/22 02:30:31  myc
 * Moved clan initialization after the player index is loaded.
 *
 * Revision 1.177  2008/09/22 02:09:17  jps
 * Changed weight into a floating-point value. Precision is preserved to
 * the 1/100 place.
 *
 * Revision 1.176  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.175  2008/09/21 04:54:23  myc
 * Changed order of some boot activities too ensure command sorting
 * occurs before anything that needs num_of_cmds.
 *
 * Revision 1.174  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.173  2008/09/01 18:29:38  jps
 * consolidating cooldown code in skills.c/h
 *
 * Revision 1.172  2008/08/30 20:35:49  jps
 * Removing equipment restriction check from zone EQUIP commands.  So many current
 * zone commands violate the restrictions that it isn't worth it to enforce restrictions
 * at this time.
 *
 * Revision 1.171  2008/08/30 20:22:12  jps
 * Added flag MOB_NO_EQ_RESTRICT, which allows a mobile to wear equipment
 *  without regard to align, class, or level restrictions.
 *
 * Revision 1.170  2008/08/30 18:20:53  myc
 * Removed UNIQUE item flag.  Rewrote free_obj to make use of
 * free_obj_strings.
 *
 * Revision 1.169  2008/08/30 01:31:51  myc
 * Changed the way stats are calculated in effect_total; ability
 * stats are saved in a raw form now, and only capped when accessed.
 * Damroll and hitroll are recalculated everytime effect_total
 * is called, using cached base values.
 *
 * Revision 1.168  2008/08/29 19:18:05  myc
 * Fixed abilities so that no information is lost; the caps occur
 * only when the viewed stats are accessed.
 *
 * Revision 1.167  2008/08/29 05:26:06  myc
 * Make sure object prototypes don't have the UNIQUE or WAS_DISARMED flags.
 *
 * Revision 1.166  2008/08/25 00:20:33  myc
 * Changed the way mobs memorize spells.
 *
 * Revision 1.165  2008/08/17 08:11:56  jps
 * Logging equip errors to file.
 *
 * Revision 1.164  2008/08/17 06:51:42  jps
 * Zone command E (equip char) will handle failures better, and destroy the object
 * rather than letting it sit in nowhere.
 *
 * Revision 1.163  2008/07/22 07:25:26  myc
 * Added copy_extra_descriptions function.
 *
 * Revision 1.162  2008/07/15 17:49:24  myc
 * Boot command groups from file during boot-up sequence.
 *
 * Revision 1.161  2008/06/19 18:53:12  myc
 * Expanded item values to 7, and making obj timer values save to
 * world files.  Also don't make all spellbooks 100 pages anymore.
 *
 * Revision 1.160  2008/06/08 00:57:29  jps
 * Set a spellbook's number of pages when loading. The number is just
 * the default for now, since prototypes don't have a field for this.
 *
 * Revision 1.159  2008/06/07 19:31:44  myc
 * Fix count_applies.
 *
 * Revision 1.158  2008/06/07 19:06:46  myc
 * Moved all object-related constants and structures to objects.h
 *
 * Revision 1.157  2008/06/05 02:07:43  myc
 * Rewrote corpse_saving and rent files to use ascii object files.
 * Changed object flags to use flagvectors, removed cost_per_day
 * and spell component fields.
 *
 * Revision 1.156  2008/05/26 18:24:48  jps
 * Removed code that deletes player object files.
 *
 * Revision 1.155  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.154  2008/05/18 05:18:06  jps
 * Renaming room_data struct's member "number" to "vnum", cos it's
 * a virtual number.
 *
 * Revision 1.153  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.152  2008/05/14 05:13:22  jps
 * alter_hit doesn't take the attacker any more.
 *
 * Revision 1.151  2008/05/11 05:46:00  jps
 * Using regen.h. alter_hit() now takes the attacker.
 *
 * Revision 1.150  2008/04/07 17:24:22  jps
 * Add "stance" to mob e-specs.
 *
 * Revision 1.149  2008/04/07 04:31:23  jps
 * Ensure mob prototypes load with sensible position and default position.
 *
 * Revision 1.148  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.147  2008/04/05 16:39:18  jps
 * Rename the function that handles the "reload" command from do_reboot
 * to do_reload.
 *
 * Revision 1.146  2008/04/04 06:12:52  myc
 * Removed dieites/worship and ships code.
 *
 * Revision 1.145  2008/04/02 04:55:59  myc
 * Got rid of the coins struct.
 *
 * Revision 1.144  2008/04/02 03:24:44  myc
 * Rewrote group code and removed all major group code.
 *
 * Revision 1.143  2008/03/30 17:30:38  jps
 * Renamed objsave.c to pfiles.c and introduced pfiles.h. Files using functions
 * from pfiles.c now include pfiles.h and depend on it in the makefile.
 *
 * Revision 1.142  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.141  2008/03/23 00:24:29  jps
 * Make sure to reset base composition.
 *
 * Revision 1.140  2008/03/22 21:43:34  jps
 * Read life force and composition from mob proto files.
 *
 * Revision 1.139  2008/03/22 17:12:19  jps
 * Set the ITEM_NORENT flag on keys when created.
 *
 * Revision 1.138  2008/03/21 14:54:19  myc
 * Added a note to free_char about freeing the event list.
 *
 * Revision 1.137  2008/03/17 16:22:42  myc
 * Updating calls to free_proto_script, and nullifying SCRIPTs after
 * extraction.
 *
 * Revision 1.136  2008/03/16 00:20:02  jps
 * Updated call to free_trophy().
 *
 * Revision 1.135  2008/03/11 02:54:53  jps
 * Use set_base_size when creating mob prototypes.
 *
 * Revision 1.134  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.
 *
 * Revision 1.133  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.132  2008/03/08 23:20:06  myc
 * Free events in free_char instead of in extract_char.
 *
 * Revision 1.131  2008/03/07 21:21:57  myc
 * Replaced action delays and skill delays with a single list of
 * 'cooldowns', which are decremented by a recurring event and
 * also save to the player file.
 *
 * Revision 1.130  2008/03/05 05:21:56  myc
 * Removed save_char_file_u declaration.  Removed player frags.
 *
 * Revision 1.129  2008/03/05 05:08:28  jps
 * Changed ascii player tags and allow them to be of variable length up to
 * 126 characters.
 *
 * Revision 1.128  2008/03/05 03:03:54  myc
 * Moved many player functions from here to players.c.
 *
 * Revision 1.127  2008/02/23 01:03:54  myc
 * Renamed assign_mem_list to init_mem_list.  Freeing memory and scribe
 * lists when freeing a character.
 *
 * Revision 1.126  2008/02/16 20:26:04  myc
 * Adding functions to free spell dams, extra descriptions, mobiles,
 * objects, rooms, zones, triggers, text files, players, and the
 * help table at program termination.
 *
 * Revision 1.125  2008/02/10 20:19:19  jps
 * Further quest numbering tweaks/fixes.
 *
 * Revision 1.124  2008/02/10 19:43:38  jps
 * Subclass quests now store the target subclass as a quest variable rather
 * than as 3 bits in the quest id.
 *
 * Revision 1.123  2008/02/09 21:07:50  myc
 * Removing plr/mob casting flags and using an event flag instead.
 *
 * Revision 1.122  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.121  2008/02/09 03:04:23  myc
 * Commenting out some spell dam log messages that weren't needed at all.
 *
 * Revision 1.120  2008/01/30 19:20:57  myc
 * Removing the ch->regenerating field and replacing it with an event
 * flags member.
 *
 * Revision 1.119  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.118  2008/01/27 21:09:12  myc
 * Save rage for players in store_to_char and char_to_store.
 *
 * Revision 1.117  2008/01/27 13:41:28  jps
 * Moved species-related data to races.h and races.c.
 *
 * Revision 1.116  2008/01/27 09:45:41  jps
 * Got rid of the MCLASS_ defines and we now have a single set of classes
 * for both players and mobiles.
 *
 * Revision 1.115  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.114  2008/01/22 09:04:17  jps
 * Clear player flags associated with memorization and regeneration.
 *
 * Revision 1.113  2008/01/22 05:02:05  myc
 * Fix asciiflag_conv to work with negative numbers.
 *
 * Revision 1.112  2008/01/15 03:18:19  myc
 * Making SENSE_LIFE do the same thing it used to instead of add
 * a static amount to perception.  That way SENSE_LIFE only applies
 * to chars and not objs.
 *
 * Revision 1.111  2008/01/12 23:13:20  myc
 * Put the spells_of_circle declaration in a header file.
 *
 * Revision 1.110  2008/01/10 05:39:43  myc
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.109  2008/01/09 09:18:00  jps
 * Moved natural move calculations to races.c.
 *
 * Revision 1.108  2008/01/09 08:31:14  jps
 * Moved height and weight setting to races.c.
 *
 * Revision 1.107  2008/01/09 04:12:42  jps
 * Remove obsolete member next_scribing.
 *
 * Revision 1.106  2008/01/09 02:29:58  jps
 * Real mob nr moved to mob_specials.
 *
 * Revision 1.105  2008/01/06 18:16:10  jps
 * Moved player height, weight, and size to races.c.
 *
 * Revision 1.104  2008/01/06 05:35:55  jps
 * Remove function prototype which is provided by an include.
 *
 * Revision 1.103  2008/01/05 20:32:43  jps
 * Move some newbie-pref setting code in here.
 *
 * Revision 1.102  2008/01/05 05:43:10  jps
 * Calling init_char() and update_char() when appropriate. Racial
 * innates stuff moved to races.c. start_player() moved here.
 *
 * Revision 1.101  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.100  2007/12/25 19:46:25  jps
 * Remember whether a player was flying as we save them. Due to the removal of
 * all effect-causing agents, affect_modify() will stop a player from flying.
 * But since we're only saving a player, we don't want to change the player's
 * position.
 *
 * Revision 1.99  2007/12/25 16:39:53  jps
 * Fix armor calculations.
 *
 * Revision 1.98  2007/12/21 07:34:36  myc
 * Fixing positive armor applies to cause negative drops in armor.
 *
 * Revision 1.97  2007/12/19 20:46:56  myc
 * save_player() no longer requires the save room (which wasn't being used
 * anyway).  Updated clan checking code in store_to_char.  Added
 * tag_argument() function for use with ASCII files.  Currently used by
 * clan code, but could possibly be used for pfiles in the future.  Also
 * added kill_ems().  These two functions ported from v3.
 *
 * Revision 1.96  2007/10/17 17:18:04  myc
 * Renamed the search_block and search_block2 functions.
 * searchblock is now case sensitive, and search_block is not.
 *
 * Revision 1.95  2007/10/11 20:14:48  myc
 * Changed skill defines to support chants and songs as skills, but
 * slightly distinguished from spells and skills.  TOP_SKILL is the
 * old MAX_SKILLS.
 *
 * Revision 1.94  2007/10/02 02:52:27  myc
 * Changed sense life to be dynamically added in to perception in
 * update_stats().
 *
 * Revision 1.93  2007/09/28 20:49:35  myc
 * Removed vnum_mobile, vnum_object, vnum_room, vnum_trigger, vnum_mob_zone,
 * and vnum_object_zone functions whose functionality is now implemented
 * by the vsearch suite.
 *
 * Revision 1.92  2007/09/20 21:45:02  myc
 * Saving perception and hiddenness to player file :>
 *
 * Revision 1.91  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  They are saved for mobs and object
 * files.  Perception is calculated in update_stats (which is called
 * whenever affect_total runs).  Rewrote scale_attribs to use an array
 * instead of a mass of constant defines in utils.h.
 *
 * Revision 1.90  2007/09/15 05:03:46  myc
 * Added MOB2 flags.  They're saved on mobs as an espec.  Implemented
 * MOB2_NOPOISON.
 *
 * Revision 1.89  2007/09/12 22:23:04  myc
 * Mobs get autoexit autoset on now.
 *
 * Revision 1.88  2007/09/04 06:49:19  myc
 * Updated hemisphere initialization function.
 *
 * Revision 1.87  2007/08/24 22:49:05  jps
 * Added function vnum_trigger() for use with the "tnum" command.
 *
 * Revision 1.86  2007/08/24 17:01:36  myc
 * Adding ostat and mstat commands as shorthand for vstat, rstat for stat
 * room, and mnum and onum for vnum.  Also adding rnum and znum with new
 * functionality.
 *
 * Revision 1.85  2007/08/22 17:59:49  jps
 * Add restrict_reason variable.
 *
 * Revision 1.84  2007/08/17 02:23:36  jps
 * Add global variable restrict_manual, so that autobooting code won't
 * remove a login restriction that was set by hand.
 *
 * Revision 1.83  2007/08/02 00:23:34  myc
 * Remove a defunct function call.
 *
 * Revision 1.82  2007/07/25 02:58:01  myc
 * I think I fixed the locate object bug... NEVER EVER EVER stick any
 * objects on a char before they enter the game!
 *
 * Revision 1.81  2007/07/24 23:02:52  jps
 * Minor typo fix.
 *
 * Revision 1.80  2007/07/19 21:59:52  jps
 * Dynamic strings for drink containers.
 *
 * Revision 1.79  2007/07/18 18:47:38  jps
 * Stop object extra desc lists from being reversed while loading.
 *
 * Revision 1.78  2007/07/18 16:23:58  jps
 * Force FOOD objects to have keyword "food".
 *
 * Revision 1.77  2007/07/18 01:21:34  jps
 * Restore AFF2/AFF3 flags to object prototypes.
 *
 * Revision 1.76  2007/07/18 00:04:41  jps
 * Save AFF2 and AFF3 flags for mobiles.
 *
 * Revision 1.75  2007/07/14 02:16:22  jps
 * Mountable mobs have their mv points calculated when being created.
 *
 * Revision 1.74  2007/04/18 17:26:26  jps
 * Reset both sides of doors that span zones.
 *
 * Revision 1.73  2007/04/17 23:59:16  myc
 * New trigger type: Load.  It goes off any time a mobile is loaded, whether
 * it be god command, zone command, or trigger command.
 *
 * Revision 1.72  2007/03/31 23:31:30  jps
 * Fix ac calculation for mobs level 30-94
 *
 * Revision 1.71  2007/03/27 04:27:05  myc
 * Fixed bug in store_to_char preventing hp from healing after 1hr time.
 * Replaced innate timer macro spellings.
 *
 * Revision 1.70  2007/02/04 18:12:31  myc
 * Page length now saves as a part of player specials.
 *
 * Revision 1.69  2006/11/21 03:45:52  jps
 * Store a light's initial burning time in obj val 1, when creating it.
 *
 * Revision 1.68  2006/11/18 19:57:16  jps
 * Don't use the one-hour since last login bonus to REDUCE
 * hit/mv/mana below their affected max.
 *
 * Revision 1.67  2006/05/05 20:46:23  rls
 * Some issues with mobiles having no class, no race creating
 * large sums of platinum.  Moreso than they should have anyway
 *
 * Revision 1.66  2006/05/05 15:53:51  rls
 * Set default for factors @ 100 instead of 0
 * so that mobs without classes, etc wouldn't
 * zero out coinage amounts.
 *
 * Revision 1.65  2006/05/05 15:06:14  rls
 * Modified get_copper to give mo' money with a little bit
 * of sanity in regards to clan prices, scroll prices, etc.
 *
 * Revision 1.64  2005/07/27 03:06:25  jwk
 * Changed mob load so that when it is read from the mob file it reads the size in instead of basing it on the race.
 *
 * Revision 1.63  2004/11/01 03:29:38  rsd
 * Added a dev ifdef for compiling to supress syserrs by
 * setting the mini_mud INT to on if in dev mode.
 *
 * Revision 1.62  2004/11/01 02:56:56  rsd
 * Removed a typo in the boot sequence
 *
 * Revision 1.61  2003/07/06 20:40:37  jjl
 * Improved the message a touch.
 *
 * Revision 1.59  2002/11/29 21:54:45  jjl
 * Well, here goes something.  Things have been difficult, so this is a
 * minor tweak to change mob HP.  Basically,  Newbie mobs get hammered in
 * half,  midrange lose about 33%, and High level mobs have no change.  In the
 * end, if nothing else, it's meant to help newbies out, without wussifying
 * the whole deal.
 *
 * Revision 1.58  2002/11/09 19:09:45  jjl
 * Compressed a large block of if's into a switch.
 *
 * Revision 1.57  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.56  2002/06/07 22:15:56  dce
 * Adjusted hitrolls for mobs. Was based on 50 levels. I
 * smoothed it out and lowered it overall.
 *
 * Revision 1.55  2002/05/09 23:08:18  dce
 * Removes any saved size on a character.
 *
 * Revision 1.54  2001/12/21 02:44:35  dce
 * Undid what I previous just did. It was causing grouping
 * flags to disappear and who know what else...
 *
 * Revision 1.53  2001/12/18 01:37:34  dce
 * When a character saves all affects are removed from the
 * character and reapplied. This applies to innate affects.
 *
 * Revision 1.52  2001/12/16 20:07:53  dce
 * Fixed a problem where players could get the max dam and hitroll
 * possible by changing their alignment and getting "zapped" by
 * alignment sensitive equipment. The "zapped" they would not lose
 * the +hitroll or damroll effects because of silly Hubis code.
 *
 * Revision 1.51  2001/12/07 02:09:56  dce
 * Linkdead players will now lose exp when they die.
 * Linkdead shapechanged players will now shapechange
 * to their original form before linking out.
 *
 * Revision 1.50  2001/03/24 05:12:01  dce
 * Objects will now accept a level through olc and upon
 * booting the objects. The level code for the players will
 * follow.
 *
 * Revision 1.49  2001/01/12 03:45:50  dce
 * *** empty log message ***
 *
 * Revision 1.48  2001/01/05 01:30:24  mtp
 * moved assignement of mobs REAL NUM to before trigger check to allow
 * better error message (with REAL VNUM)
 *
 * Revision 1.47  2000/11/28 01:37:03  mtp
 * better error messgae if mobprog seen
 *
 * Revision 1.46  2000/11/28 01:10:58  mtp
 * removed more mobprog stuff
 *
 * Revision 1.44  2000/11/21 01:03:00  rsd
 * Added back rlog messages from prior to the addition
 * of the $log$ string.
 *
 * Revision 1.43  2000/11/15 00:24:09  mtp
 * removed a couple of fprintf(stderr) lines which showed quests loading
 *
 * Revision 1.42  2000/11/07 01:48:17  mtp
 * added check for qsubclass stuff in real_quest search routine and
 * also changed function according to new prototype
 *
 * Revision 1.41  2000/11/03 21:25:59  jimmy
 * Added debug to print out bogus exits
 *
 * Revision 1.40  2000/11/03 20:31:48  jimmy
 * removed last version
 *
 * Revision 1.38  2000/11/03 05:37:17  jimmy
 * Removed the quest.h file from structs.h arg!! and placed it
 * only in the appropriate files
 * Updated the dependancies in the Makefile and created
 * make supahclean.
 *
 * Revision 1.37  2000/10/31 23:31:44  mtp
 * changed harreferences to misc/quest to ALL_QUEST_FILE
 *
 * Revision 1.36  2000/10/31 00:43:37  mud
 * fixed a memlry leak in real_quest
 * /s
 *
 * Revision 1.35  2000/10/27 00:34:45  mtp
 * extra info for booting quests and a real_quest fn to find array
 * value given vnum of quest
 *
 * Revision 1.34  2000/10/11 23:54:20  rsd
 * move the piece of delete player that removes the player
 * from the index to another independant function that
 * delete_player calls.  the new function is
 * delete_player_from_index
 *
 * Revision 1.33  2000/06/05 19:03:59  rsd
 * Set players mv to be 100 or 2xCON whichever is maximum
 * when they are created.
 *
 * Revision 1.32  2000/06/01 20:09:16  rsd
 * Deleted a bunch of debug log messages from the init_weather()
 * section. Also made other log messgaes look better.
 *
 * Revision 1.31  2000/05/29 20:40:51  rsd
 * Altered ch->points.max_move to = GET_CON(ch) * 2
 * as opposed to 82 hard coded movement points.
 *
 * Revision 1.30  2000/04/26 22:51:45  rsd
 * made a delete_player function
 *
 * Revision 1.29  2000/04/22 22:35:49  rsd
 * Altered comment header while browsing the file.
 *
 * Revision 1.28  2000/03/20 04:32:39  rsd
 * added comments regarding void reboot_wizlists(void) as it
 * pertains to the removal of autowiz
 *
 * Revision 1.27  2000/03/08 22:05:09  mtp
 * fixed hidden exit with no door? for some reason setting EX_DESCRIPT seems to override
 * the other stuff about an exit...
 *
 * Revision 1.26  1999/11/29 00:21:47  cso
 * removed unused variables to kill compile warnings
 *
 * Revision 1.25  1999/11/28 23:09:31  cso
 * took unused argument from all calls to roll_natural_abils
 * new fn: setup_mob. does some of the work read_mobile used to
 * free_char: added call to free(ch->casting.misc) to plug mem leak
 *
 * Revision 1.24  1999/10/04 20:00:50  rsd
 * Fixed some function associated with reading in text files.
 * in do_reboot.  The read new truncates the files instead of
 * overflowing. good enough.
 *
 * Revision 1.23  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.22  1999/09/04 18:46:52  jimmy
 * More small but important bug fixes found with insure.  These are all runtime fixes.
 *
 * Revision 1.21  1999/08/29 07:06:04  jimmy
 * Many many small but ver significant bug fixes found using insure.  The
 * code now compiles cleanly and boots cleanly with insure.  The most significant
 * changes were moving all the BREATH's to within normal spell range, and
 * fixing the way socials were allocated.  Too many small fixes to list them
 * all. --gurlaek (now for the runtime debugging :( )
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
 * Revision 1.19  1999/07/21 03:10:19  jimmy
 * added an oversight to free_char.  Forgot to release mem for spell lists
 * just in case it was still there when a player was free'd
 * --gurlaek
 *
 * Revision 1.18  1999/07/09 22:30:27  jimmy
 * Attempt to control the spiraling of memory.  Added a free() to the
 * prompt code in comm.c to free memory allocated by parse_color().
 * made a global structure dummy_mob and malloc'ed it for mobs
 * to share as their player_specials to cut memory.
 * gurlaek
 *
 * Revision 1.17  1999/07/06 19:57:05  jimmy
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
 * Revision 1.16  1999/06/30 18:25:04  jimmy
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
 * Revision 1.15  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list. This code compiles fine under both gcc RH5.2 and egcs RH6.0.
 * --Gurlaek 6/10/1999
 *
 * Revision 1.14  1999/05/04 17:19:33  dce
 * Name accept system...version one...original code by Fingh, fixed up to work
 * by Zantir.
 *
 * Revision 1.13  1999/04/09 17:25:33  jen
 * Mobs now get abil rolls (unless they're special circle 3 ones).
 * The rolls depend on class & species.
 *
 * Revision 1.12  1999/04/07 01:20:18  dce
 * Allows extra descriptions on no exits.
 *
 * Revision 1.11  1999/03/06 23:51:54  dce
 * Add's chant songs, and can only chant once every four hours
 *
 * Revision 1.10  1999/03/03 20:11:02  jimmy
 * Many enhancements to scribe and spellbooks.  Lots of checks added.  Scribe is now a skill.
 * Spellbooks now have to be held to scribe as well as a quill in the other hand.
 *
 * -fingon
 *
 * Revision 1.9  1999/03/01 05:31:34  jimmy
 * Rewrote spellbooks.  Moved the spells from fingh's PSE to a standard linked
 * list.  Added Spellbook pages.  Rewrote Scribe to be a time based event based
 * on the spell mem code.  Very basic at this point.  All spells are 5 pages long,
 * and take 20 seconds to scribe each page.  This will be more dynamic when the
 * SCRIBE skill is introduced.  --Fingon.
 *
 * Revision 1.8  1999/02/26 18:42:19  jimmy
 * Added obj = NULL in free_obj to help stop pointers to
 * raw memory for objects crashing the mud.
 *
 * fingon
 *
 * Revision 1.7  1999/02/10 05:57:14  jimmy
 * Added long description to player file.  Added AFK toggle.
 * removed NOAUCTION toggle.
 * fingon
 *
 * Revision 1.6  1999/02/06 00:40:36  jimmy
 * Major change to incorporate aliases into the pfile
 * moved alias structure from interpreter.h to structs.h
 * heavily modified alias code in interpreter.c
 * Jimmy Kincaid AKA fingon
 *
 * Revision 1.5  1999/02/05 07:47:42  jimmy
 * Added Poofs to the playerfile as well as 4 extra strings for
 * future use.  fingon
 *
 * Revision 1.4  1999/02/02 07:35:17  jimmy
 * spellcasters now lose all memmed spells at death without
 * haveing to quit and log back in.
 *
 * Revision 1.3  1999/02/01 08:18:12  jimmy
 * improved build counter
 *
 * Revision 1.2  1999/01/30 22:12:02  mud
 * Indented entire file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/
