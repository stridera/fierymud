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

#include "db.hpp"

#include "board.hpp"
#include "casting.hpp"
#include "charsize.hpp"
#include "clan.hpp"
#include "comm.hpp"
#include "commands.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "cooldowns.hpp"
#include "corpse_save.hpp"
#include "dg_scripts.hpp"
#include "directions.hpp"
#include "events.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "house.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "mail.hpp"
#include "math.hpp"
#include "money.hpp"
#include "movement.hpp"
#include "pfiles.hpp"
#include "players.hpp"
#include "quest.hpp"
#include "races.hpp"
#include "regen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "textfiles.hpp"
#include "trophy.hpp"
#include "utils.hpp"
#include "weather.hpp"

#include <algorithm>
#include <math.h>
#include <sys/stat.h>

void init_clans(void);
char err_buf[MAX_STRING_LENGTH];
PlayerSpecialData dummy_mob;

/*object limit function*/
void boot_quests();

void setup_drinkcon(ObjData *obj, int newliq);

/***************************************************************************
 *  declarations of most of the 'global' variables                         *
 ***************************************************************************/
char buf[MAX_STRING_LENGTH];
char buf1[MAX_STRING_LENGTH];
char buf2[MAX_STRING_LENGTH];
char arg[MAX_STRING_LENGTH];

RoomData *world = nullptr;                  /* array of rooms                 */
int top_of_world = 0;                       /* ref to top element of world         */
RoomEffectNode *room_effect_list = nullptr; /* list of room effects */

CharData *character_list = nullptr; /* global linked list of chars */

IndexData **trig_index; /* index table for triggers      */
int top_of_trigt = 0;   /* top of trigger index table    */
long max_id = 100000;   /* for unique mob/obj id's       */

IndexData *mob_index;                       /* index table for mobile file         */
CharData *mob_proto;                        /* prototypes for mobs                 */
ObjData *obj_proto;                         /* prototypes for objs                 */
int top_of_mobt = 0;                        /* top of mobile index table         */
ObjData *object_list = nullptr;             /* global linked list of objs         */
IndexData *obj_index;                       /* index table for object file         */
                                            /* prototypes for objs                 */
int top_of_objt = 0;                        /* top of object index table         */
SpellDamage spell_dam_info[MAX_SPELLS + 1]; /*internal spell dam */
ZoneData *zone_table;                       /* zone table                         */
int top_of_zone_table = 0;                  /* top element of zone tab         */
message_list fight_messages[MAX_MESSAGES];  /* fighting messages */

PlayerIndexElement *player_table = nullptr; /* index to plr file */
int top_of_p_table = 0;                     /* ref to top of table                 */
int top_of_p_file = 0;                      /* ref of size of p file         */
long top_idnum = 0;                         /* highest idnum in use                 */

HelpIndexElement *help_table = 0; /* the help table         */
int top_of_helpt = 0;             /* top of help index table         */

// ObjData *obj_proto;
// int top_of_objt;
// IndexData *mob_index;
// CharData *mob_proto;
// int top_of_mobt;
// ZoneData *zone_table;
// int top_of_zone_table;
// IndexData **trig_index;
// int top_of_trigt;
// long max_id;

// int top_of_helpt;
// HelpIndexElement *help_table;

TimeInfoData time_info;

stat_bonus_type stat_bonus[101];

int no_mail = 0; /* mail disabled?                 */

ResetQType reset_q; /* queue of zones to be reset         */

void setup_dir(FILE *fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE *fl, int mode);
void parse_trigger(FILE *fl, int virtual_nr);
void parse_room(FILE *fl, int virtual_nr);
void parse_mobile(FILE *mob_f, int nr);
char *parse_object(FILE *obj_f, int nr);
void load_zones(FILE *fl, char *zonename);
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
void log_zone_error(int zone, int cmd_no, const char *message);
void reset_time(void);
void clear_char(CharData *ch);
long get_set_exp(int level, int race, int class_num, int zone);
sh_int get_set_hit(int level, int race, int class_num, int state);
sbyte get_set_hd(int level, int race, int class_num, int state);
int get_set_dice(int level, int race, int class_num, int state);
int get_copper(int);
/* external functions */
void boot_social_messages(void);
void sort_commands(void);
void sort_spells(void);
void load_banned(void);
void Read_Xname_List(void);
void boot_the_shops(FILE *shop_f, char *filename, int rec_count);
int hsort(const void *a, const void *b);
void boot_the_shops(FILE *shop_f, char *filename, int rec_count);
void build_count(void);
void load_stat_bonus(void);
void clear_memory(CharData *ch);

#define READ_SIZE 256
#define GET_ZONE(ch) (ch)->mob_specials.zone

#define GET_SDESC(mob) ((mob)->player.short_descr)
/*#define GET_LDESC(mob) ((mob)->player.long_descr)*/
#define GET_NDD(mob) ((mob)->mob_specials.damnodice)
#define GET_SDD(mob) ((mob)->mob_specials.damsizedice)
#define MOB_MONSUM_I 130
#define MOB_MONSUM_II 140
#define MOB_MONSUM_III 150
#define MOB_GATE_I 160
#define MOB_GATE_II 170
#define MOB_GATE_III 180
#define MOB_ELEMENTAL_BASE 110
#define MOB_CLONE 69
#define MOB_ZOMBIE 11
#define MOB_AERIALSERVANT 109
#define MOB_MENTAL 17
#define MOB_MENTAL2 21
/*************************************************************************
 *  routines for booting the system                                      *
 *************************************************************************/

long get_set_exp(int level, int race, int class_num, int zone)
/*class/species */
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
    /*zfactor = 100; */

    /*The cfactor is the factor of class to adjust exp number is percentage
       100 being standard no change */
    cfactor = CLASS_EXPFACTOR(class_num);
    sfactor = RACE_EXPFACTOR(race);

    if (level < 50)
        exp = (long)(((float)(level * level * level)) + 1000);
    else if (level >= 50) /*50 and under equation */
        exp = (long)(level * level * 50) + 1000;
    /*51 and over equation */

    sfactor = ((int)((sfactor + zfactor + cfactor) / 3));
    exp = (long)(((float)((sfactor * exp) / 100)));
    return exp;
}

sh_int get_set_hit(int level, int race, int class_num, int state)
/*class/species */
{
    /*main - is the main bonus chunk of hps
     */
    /*  descriptor_data *d;
     */
    sh_int xmain = 0;
    sh_int face = 1;
    int cfactor = 100;
    int sfactor = 100;

    /* The cfactor is the factor of class to adjust exp number is percentage
       100 being standard no change */
    cfactor = CLASS_HITFACTOR(class_num);
    sfactor = RACE_HITFACTOR(race);

    /* here is the function */
    /*auto setting sets hit and mana to 5 and 10 thus 5d10 extra = average 25 */
    if (level < 20)
        xmain = (sh_int)(3 * ((float)level * (float)(level / 1.25))); /*50 and under equation */

    else if (level < 35)
        xmain = (sh_int)(3 * ((float)level * (float)(level / 1.35))); /*50 and under equation */

    else if (level < 50)
        xmain = (sh_int)(3 * ((float)level * (float)level / 1.25)); /*50 and under equation */

    else if (level >= 50)
        xmain = (sh_int)(3 * ((float)level * (float)level / 1.25)); /*51 and over equation */
    if (level <= 5)
        face = 1;
    else if (level <= 10) {
        xmain -= 25;
        face = 5;
    } else if (level <= 20) {
        xmain -= 100;
        face = 10;
    } else if (level <= 30) {
        xmain -= 200;
        face = 20;
    } else {
        xmain -= 2000;
        face = 200;
    }

    /*finally taking the factors into account */
    sfactor = ((int)((sfactor + cfactor) / 2));
    xmain = (sh_int)(((float)(sfactor * xmain) / 100));

    if (state == 2)
        return face;
    else
        return xmain / (2 - (level / 100.0));
}

sbyte get_set_hd(int level, int race, int class_num, int state)
/*class/species */
{
    sbyte hit = 0;
    sbyte dam = 0;
    int cfactor = 100;
    int sfactor = 100;

    /*The cfactor is the factor of class to adjust exp number is percentage
       100 being standard no change */
    cfactor = CLASS_HDFACTOR(class_num);
    sfactor = RACE_HDFACTOR(race);

    /*hit calculations */
    if (!state) {
        if (level < 10)
            hit = (sbyte)(level / 2.0);
        else if (level < 24)
            hit = (sbyte)(level / 2.4);
        else if (level < 32)
            hit = (sbyte)(level / 2.6);
        else if (level < 50)
            hit = (sbyte)(level / 2.8);
        else if (level < 62)
            hit = (sbyte)(level / 3.0);
        else if (level < 75)
            hit = (sbyte)(level / 3.2); /*50 and under equation */
        else if (level < 82)
            hit = (sbyte)(level / 3.4);
        else if (level >= 90)
            hit = (sbyte)(level / 3.6); /*51 and over equation */

        /*hit factor considerations */
        sfactor = ((int)((sfactor + cfactor) / 2));
        hit = (sbyte)(((float)(sfactor * hit) / 100));
    }

    /*dam calculations */
    if (state) {
        if (level < 10)
            dam = (sbyte)(level / 4.0);
        else if (level < 20)
            dam = (sbyte)(level / 4.0); /*under 20 */
        else if (level < 35)
            dam = (sbyte)(level / 4.3); /*under 35 */

        else if (level < 50)
            dam = (sbyte)(level / 4.6); /*50 and under equation */

        else if (level >= 50)
            dam = (sbyte)(level / 4.4); /*51 and over equation */

        /*dam factor considerations */
        sfactor = ((int)((sfactor + cfactor) / 2));
        dam = (sbyte)(((float)(sfactor * dam) / 100));
    }

    if (!state)
        return hit;
    else
        return dam;
}

int get_set_dice(int level, int race, int class_num, int state)
/*class/species */
{
    int dice = 0;
    int face = 0;
    int cfactor = 100;
    int sfactor = 100;

    /*The cfactor is the factor of class to adjust exp number is percentage
       100 being standard no change */
    cfactor = CLASS_DICEFACTOR(class_num);
    sfactor = RACE_DICEFACTOR(race);

    /*number of damage dice calculations */
    if (!state) {
        if (level < 10)
            dice = std::max(1, (int)((level / 3) + .5));

        else if (level < 30)
            dice = (int)((float)(level / 3) + .5); /*under 30 */

        else
            dice = (int)((level / 3) + .5); /*50 and under equation */

        sfactor = ((int)((sfactor + cfactor) / 2));
        dice = (sbyte)(((float)(sfactor * dice) / 100));
    }

    /*size of damage dice face calcs */
    if (state) {
        if (level < 22)
            face = 3;
        else if (level < 40)
            face = 4;
        else
            face = (level / 10) + 1;
    }

    if (!state)
        return dice;
    else
        return face;
}

int get_copper(int i)
/*class/species */
{
    /*mob_proto[i].player.class_num, mob_proto[i].player.race,
     * mob_proto[i].player.level, GET_ZONE(mob_proto + i) */
    int copper = 0;
    int cfactor = 100;
    int sfactor = 100;
    int zfactor;
    for (zfactor = 0; zfactor <= top_of_zone_table; zfactor++) {
        if (GET_ZONE(mob_proto + i) == zone_table[zfactor].number)
            break;
    }
    zfactor = zone_table[zfactor].zone_factor;

    /*The cfactor is the factor of class to adjust exp number is percentage
       100 being standard no change */
    cfactor = CLASS_COPPERFACTOR((int)mob_proto[i].player.class_num);
    sfactor = RACE_COPPERFACTOR((int)mob_proto[i].player.race);

    if ((sfactor == 0) || (cfactor == 0))
        return 0;

    /*copper calculations */
    copper = (random_number(1, 150)) * mob_proto[i].player.level;
    sfactor = (int)((sfactor + cfactor + zfactor) / 3);

    copper = (int)((float)((sfactor / 100.0) * copper));

    return copper;
}

int get_ac(int level, int race, int class_num)
/*class/species */
{
    /*mob_proto[i].player.class_num, mob_proto[i].player.race,
     * mob_proto[i].player.level, GET_ZONE(mob_proto + i) */
    int ac = 0;
    int sfactor = 100;

    /* The cfactor is the factor of class to adjust exp number is percentage
        100 being standard no change */
    /*
    // Removing this while I figure out how it was used (strider)
    int cfactor = 100;
    cfactor = CLASS_ACFACTOR(class_num);*/

    sfactor = RACE_ACFACTOR(race);

    /*ac calculations */
    ac = 90 - (int)(2 * level * (float)(sfactor / 100.0));
    ac = std::clamp(ac, -100, 100);
    return ac;
}

/*This requres a file in lib/misc/spell_dams, It will tell you if you dont
  It then boots, note that if anything goes wrong to file
  you can re-write it by saving under sdedit, the viewer is in act.informative.c
  The olc code is in sdedit.c
*/
void boot_spell_dams() {
    int i;
    FILE *ifptr;
    char line[256];
    const char *err = "Spell Dam";
    if ((ifptr = fopen(SPELL_DAM_FILE, "r")) == nullptr) {
        log("No spells dam file it should be at lib/misc/spell_dam.");
        exit(1);
    } else {
        get_line(ifptr, line);
        get_line(ifptr, line);
        get_line(ifptr, line);
        /*
              log(line);
              get_line(ifptr, line);
              log(line);
              get_line(ifptr, line);
              log(line);
        */
        if (strcasecmp(line, "spell_dam")) {
            log("Error in booting spell dams");
            /* return;
	     */ }
            for (i = 1; i <= MAX_SPELLS; i++) {
                get_line(ifptr, line);
                sscanf(line, "%hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd", &SD_SPELL(i), &SD_INTERN_DAM(i),
                       &SD_NPC_STATIC(i), &SD_NPC_NO_DICE(i), &SD_NPC_NO_FACE(i), &SD_PC_STATIC(i), &SD_PC_NO_DICE(i),
                       &SD_PC_NO_FACE(i), &SD_NPC_REDUCE_FACTOR(i), &SD_USE_BONUS(i), &SD_BONUS(i), &SD_LVL_MULT(i));
                if (SD_NPC_REDUCE_FACTOR(i) == 0)
                    SD_NPC_REDUCE_FACTOR(i) = 100;

                SD_NOTE(i) = fread_string(ifptr, err);
            }
            fclose(ifptr);
    }
}

void free_spell_dams(void) {
    int i;
    extern struct SpellDamage spell_dam_info[];

    for (i = 1; i <= MAX_SPELLS; i++)
        free(spell_dam_info[i].note);
}

void boot_world(void) {
    log("Loading attribute bonus tables.");
    load_stat_bonus();

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
    memset(&dummy_mob, 0, sizeof(PlayerSpecialData));
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
void boot_db(void) {
    int i;

    log("Boot db -- BEGIN.");

    log("Reading anews, credits, help, bground, info and motds.");
    boot_text();

    log("   Skills.");
    init_skills();
	
    log("Assigning skills and spells to classes.");
    assign_class_skills();

    log("Assigning skills and spells to races.");
    assign_race_skills();

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
        log("Resetting {} (rooms {:d}-{:d}).", zone_table[i].name, (i ? (zone_table[i - 1].top + 1) : 0),
            zone_table[i].top);
        reset_zone(i, true);
    }

    reset_q.head = reset_q.tail = nullptr;

    log("Booting corpses.");
    boot_corpses();
    log("Booting quests.");
    boot_quests();

    CREATE(boot_time, time_t, 1);
    *boot_time = time(0);

    log("Boot db -- DONE.");
}

void free_extra_descriptions(ExtraDescriptionData *edesc) {
    ExtraDescriptionData *enext;

    for (; edesc; edesc = enext) {
        enext = edesc->next;

        if (edesc->keyword)
            free(edesc->keyword);
        if (edesc->description)
            free(edesc->description);
        free(edesc);
    }
}

void copy_extra_descriptions(ExtraDescriptionData **to, ExtraDescriptionData *from) {
    ExtraDescriptionData *wpos;

    CREATE(*to, ExtraDescriptionData, 1);
    wpos = *to;

    for (; from; from = from->next, wpos = wpos->next) {
        if (from->keyword)
            wpos->keyword = strdup(from->keyword);
        if (from->description)
            wpos->description = strdup(from->description);
        if (from->next)
            CREATE(wpos->next, ExtraDescriptionData, 1);
    }
}

void destroy_db(void) {
    ssize_t cnt, itr;
    CharData *chtmp;
    ObjData *objtmp;

    extern void destroy_shops(void);
    extern void stop_groupee(CharData * ch, bool hide);

    /* Active Characters */
    while (character_list) {
        chtmp = character_list;
        character_list = character_list->next;
        if (chtmp->master)
            stop_follower(chtmp, false);
        if (chtmp->group_master)
            ungroup(chtmp, false, false);
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
            SCRIPT(&world[cnt]) = nullptr;
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
        ResetQElement *ftemp = reset_q.head, *temp;
        while (ftemp) {
            temp = ftemp->next;
            free(ftemp);
            ftemp = temp;
        }
    }

    /* Triggers */
    for (cnt = 0; cnt < top_of_trigt; cnt++) {
        if (trig_index[cnt]->proto) {
            /* make sure to nuke the command list (memory leak) */
            /* free_trigger() doesn't free the command list */
            if (trig_index[cnt]->proto->cmdlist) {
                CmdlistElement *i, *j;
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
void reset_time(void) {
    long beginning_of_time = 650336715;
    TimeInfoData mud_time_passed(time_t t2, time_t t1);

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

    log("Setting up seasons.");

    /* setup the seasons */
    if (time_info.month < 4) {
        hemispheres[HEMISPHERE_NORTHWEST].season = WINTER;
        hemispheres[HEMISPHERE_SOUTHWEST].season = SUMMER;
        hemispheres[HEMISPHERE_NORTHEAST].season = WINTER;
        hemispheres[HEMISPHERE_SOUTHEAST].season = SUMMER;
    } else if (time_info.month < 8) {
        hemispheres[HEMISPHERE_NORTHWEST].season = SPRING;
        hemispheres[HEMISPHERE_SOUTHWEST].season = AUTUMN;
        hemispheres[HEMISPHERE_NORTHEAST].season = SPRING;
        hemispheres[HEMISPHERE_SOUTHEAST].season = AUTUMN;
    } else if (time_info.month < 12) {
        hemispheres[HEMISPHERE_NORTHWEST].season = SUMMER;
        hemispheres[HEMISPHERE_SOUTHWEST].season = WINTER;
        hemispheres[HEMISPHERE_NORTHEAST].season = SUMMER;
        hemispheres[HEMISPHERE_SOUTHEAST].season = WINTER;
    } else {
        hemispheres[HEMISPHERE_NORTHWEST].season = AUTUMN;
        hemispheres[HEMISPHERE_SOUTHWEST].season = SPRING;
        hemispheres[HEMISPHERE_NORTHEAST].season = AUTUMN;
        hemispheres[HEMISPHERE_SOUTHEAST].season = SPRING;
    }

    log("Current Gametime: {:d}H {:d}D {:d}M {:d}Y.", time_info.hours, time_info.day, time_info.month, time_info.year);

    log("Initializing weather.");
    init_weather();
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE *fl) {
    char buf[128];
    int count = 0;

    while (fgets(buf, 128, fl))
        if (*buf == '#')
            count++;

    return count;
}

void index_boot(int mode) {
    const char *index_filename, *prefix;
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
        CREATE(trig_index, IndexData *, rec_count);
        break;
    case DB_BOOT_WLD:
        CREATE(world, RoomData, rec_count);
        break;
    case DB_BOOT_MOB:
        CREATE(mob_proto, CharData, rec_count);
        CREATE(mob_index, IndexData, rec_count);
        break;
    case DB_BOOT_OBJ:
        CREATE(obj_proto, ObjData, rec_count);
        CREATE(obj_index, IndexData, rec_count);
        break;
    case DB_BOOT_ZON:
        CREATE(zone_table, ZoneData, rec_count);
        break;
    case DB_BOOT_HLP:
        CREATE(help_table, HelpIndexElement, rec_count * 2);
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
        qsort(help_table, top_of_helpt, sizeof(HelpIndexElement), hsort);
        top_of_helpt--;
    }
}

int tmp_debug = 0;

void discrete_load(FILE *fl, int mode) {
    int nr = -1, last = 0;
    char line[256];
    const char *modes[] = {"world", "mob", "obj", "ZON", "SHP", "HLP", "trg"};
    for (;;) {
        /*
         * we have to do special processing with the obj files because they have
         * no end-of-record marker :(
         */
        if (mode != DB_BOOT_OBJ || nr < 0) {
            tmp_debug++;
            if (!get_line(fl, line)) {
                fprintf(stderr, "Format error after %s #%d\n", modes[mode], nr);
                fprintf(stderr, "Offending line: '%s'in file down list%d\n", line, tmp_debug);
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
            fprintf(stderr, "Format error in %s file near %s #%d\n", modes[mode], modes[mode], nr);
            fprintf(stderr, "Offending line: '%s'\n", line);
            exit(1);
        }
    }
}

long asciiflag_conv(char *flag) {
    long flags = 0;
    char *p;

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

char fread_letter(FILE *fp) {
    char c;
    do {
        c = getc(fp);
    } while (isspace(c));
    return c;
}

/* load the rooms */
void parse_room(FILE *fl, int virtual_nr) {
    static int room_nr = 0, zone = 0;
    int t[10], i;
    char line[256], flags[128];
    ExtraDescriptionData *new_descr;
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

    world[room_nr].func = nullptr;
    world[room_nr].contents = nullptr;
    world[room_nr].people = nullptr;
    world[room_nr].light = 0; /* Zero light sources */

    for (i = 0; i < NUM_OF_DIRS; i++)
        world[room_nr].exits[i] = nullptr;

    world[room_nr].ex_description = nullptr;

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
            CREATE(new_descr, ExtraDescriptionData, 1);
            new_descr->keyword = fread_string(fl, buf2);
            new_descr->description = fread_string(fl, buf2);
            new_descr->next = world[room_nr].ex_description;
            world[room_nr].ex_description = new_descr;
            break;
        case 'S': /* end of room */
            letter = fread_letter(fl);
            ungetc(letter, fl);
            while (letter == 'T') {
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
void setup_dir(FILE *fl, int room, int dir) {
    int t[5];
    char line[256];

    sprintf(buf2, "room #%d, direction D%d", world[room].vnum, dir);
    /* added by gurlaek to stop memory leaks detected by insure++ 8/26/1999 */
    if (world[room].exits[dir]) {
        log("SYSERR:db.c:setup_dir:creating direction [{:d}] for room {:d} twice!", dir, world[room].vnum);
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
void check_start_rooms(void) {

    if ((r_mortal_start_room = real_room(mortal_start_room)) < 0) {
        log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
        exit(1);
    }
    if ((r_immort_start_room = real_room(immort_start_room)) < 0) {
        log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
        r_immort_start_room = r_mortal_start_room;
    }
    if ((r_frozen_start_room = real_room(frozen_start_room)) < 0) {
        log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
        r_frozen_start_room = r_mortal_start_room;
    }
}

/* resolve all vnums into rnums in the world */
void renum_world(void) {
    int rnum;
    int room, door;

    for (room = 0; room <= top_of_world; room++)
        for (door = 0; door < NUM_OF_DIRS; door++)
            if (world[room].exits[door])
                if (world[room].exits[door]->to_room != NOWHERE) {
                    rnum = real_room(world[room].exits[door]->to_room);
                    world[room].exits[door]->to_room = rnum;
                    if (rnum == NOWHERE) {
                        log("SYSERR:db.c:renum_world():Invalid exit to NOWHERE for dir {} in room {:d}", dirs[door],
                            world[room].vnum);
                    }
                }
}

#define ZCMD zone_table[zone].cmd[cmd_no]

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void) {
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
                ZCMD.command = '*';
            }
        }
}

#define OLC_MOB(d) ((d)->olc->mob)
void parse_simple_mob(FILE *mob_f, int i, int nr) {
    int j, t[10];
    char line[256];
    long k = 0;
    CharData *mobproto;
    extern void roll_natural_abils(CharData * ch);

    get_line(mob_f, line);
    if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
        fprintf(stderr,
                "Format error in mob #%d, first line after S flag\n"
                "...expecting line of form '# # # #d#+# #d#+#'\n",
                nr);
        exit(1);
    }
    GET_LEVEL(mob_proto + i) = t[0];
    /*  No negative hitroll bonus DO_NOT CHANGE -Banyal */
    mob_proto[i].mob_specials.ex_hitroll = std::clamp(t[1], 0, 20);

    mob_proto[i].mob_specials.ex_armor = 10 * t[2];
    /*if ac onmedit is 0 then ignore it */

    /* max hit = 0 is a flag that H, M, V is xdy+z */
    mob_proto[i].points.max_hit = 0;

    mob_proto[i].mob_specials.ex_max_hit = 0;
    mob_proto[i].mob_specials.ex_no_dice = t[3];
    mob_proto[i].mob_specials.ex_face = t[4];

    /*mob_proto[i].points.hit = t[3];
       mob_proto[i].points.mana = t[4];
     */

    if (mob_proto[i].mob_specials.ex_focus)
        mob_proto[i].char_specials.focus = mob_proto[i].mob_specials.ex_focus;
    else
        mob_proto[i].char_specials.focus = (mob_proto[i].player.level / 2) + 10;

    mob_proto[i].points.move = t[5];
    mob_proto[i].points.max_mana = 10;
    mob_proto[i].points.max_move = 200;

    mob_proto[i].mob_specials.ex_damnodice = t[6];
    mob_proto[i].mob_specials.ex_damsizedice = t[7];
    mob_proto[i].mob_specials.ex_damroll = t[8];

    get_line(mob_f, line);
    if (sscanf(line, " %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5) > 5) {
        GET_ZONE(mob_proto + i) = t[4];
        GET_EX_COPPER(mob_proto + i) = t[0];
        GET_EX_SILVER(mob_proto + i) = t[1];
        GET_EX_GOLD(mob_proto + i) = t[2];
        GET_EX_PLATINUM(mob_proto + i) = t[3];
    } else if (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3) > 3) {
        GET_ZONE(mob_proto + i) = t[2];
        GET_EX_GOLD(mob_proto + i) = t[0];
        GET_EX_PLATINUM(mob_proto + i) = t[1];
    } else {
        GET_ZONE(mob_proto + i) = (nr / 100);
        GET_EX_GOLD(mob_proto + i) = t[0];
        GET_EX_PLATINUM(mob_proto + i) = 0;
    }

    get_line(mob_f, line);
    if ((sscanf(line, " %d %d %d %d %d %d %d %d ", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7)) > 3) {
        mob_proto[i].player.class_num = t[3];
        mob_proto[i].player.race = t[4];
        mob_proto[i].player.race_align = t[5];
        set_base_size(&(mob_proto[i]), t[6]);
        mob_proto[i].char_specials.position = t[0];
        mob_proto[i].mob_specials.default_pos = t[1];
        mob_proto[i].player.sex = t[2];
    } else {
        mob_proto[i].player.class_num = CLASS_DEFAULT;
        t[3] = CLASS_DEFAULT;
        mob_proto[i].player.race = DEFAULT_RACE;
        t[4] = DEFAULT_RACE;
        mob_proto[i].player.race_align = 0;
        set_base_size(&(mob_proto[i]), SIZE_OF_RACE(DEFAULT_RACE));
        mob_proto[i].char_specials.position = t[0];
        mob_proto[i].mob_specials.default_pos = t[1];
        mob_proto[i].player.sex = t[2];
    }
    mob_proto[i].char_specials.stance = STANCE_ALERT;
    if (!VALID_CLASSNUM((int)mob_proto[i].player.class_num))
        mob_proto[i].player.class_num = CLASS_DEFAULT;
    mobproto = &mob_proto[i];
    mobproto->player_specials = &dummy_mob;           /* dummy player_specials for mobs */
    roll_natural_abils(mobproto);                     /* mobs now get rolled just like PC's */
    mobproto->actual_abils = mobproto->natural_abils; /* set the initail viewed abils */
    scale_attribs(mobproto);                          /* this scales the attribs for race */

    mob_proto[i].player.height = 198;

    mob_proto[i].points.coins[PLATINUM] = 0;
    mob_proto[i].points.coins[GOLD] = 0;
    mob_proto[i].points.coins[SILVER] = 0;
    mob_proto[i].points.coins[COPPER] = 0;

    /*Money adder */
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

    mob_proto[i].points.coins[PLATINUM] =
        std::max(0, mob_proto[i].points.coins[PLATINUM] + GET_EX_PLATINUM(mob_proto + i));
    mob_proto[i].points.coins[GOLD] = std::max(0, mob_proto[i].points.coins[GOLD] + GET_EX_GOLD(mob_proto + i));
    mob_proto[i].points.coins[COPPER] = std::max(0, mob_proto[i].points.coins[COPPER] + GET_EX_COPPER(mob_proto + i));
    mob_proto[i].points.coins[SILVER] = std::max(0, mob_proto[i].points.coins[SILVER] + GET_EX_SILVER(mob_proto + i));

    if ((mob_proto[i].mob_specials.ex_armor != 100))
        mob_proto[i].points.armor = mob_proto[i].mob_specials.ex_armor +
                                    get_ac(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i));
    else
        mob_proto[i].points.armor =
            (sh_int)get_ac(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i));

    /*num of hp dice is hit */
    GET_EXP(mob_proto + i) = get_set_exp(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i),
                                         GET_ZONE(mob_proto + i));
    GET_EXP(mob_proto + i) += GET_EX_EXP(mob_proto + i);

    mob_proto[i].points.mana =
        get_set_hit(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i), 2);
    mob_proto[i].points.hit = 7;
    ;
    mob_proto[i].points.mana += mob_proto[i].mob_specials.ex_face;
    mob_proto[i].points.hit += mob_proto[i].mob_specials.ex_no_dice;
    GET_EX_MAIN_HP(mob_proto + i) =
        (get_set_hit(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i), 1));

    mob_proto[i].points.damroll =
        get_set_hd(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i), 1);
    mob_proto[i].points.damroll += mob_proto[i].mob_specials.ex_damroll;
    /*  Changed to get rid of the -199 mobb hitrolls Do NOT change -Banyal */
    mob_proto[i].points.hitroll = std::clamp<int>(
        get_set_hd(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i), 0), 10, 80);
    mob_proto[i].points.hitroll += mob_proto[i].mob_specials.ex_hitroll;

    mob_proto[i].mob_specials.damnodice =
        get_set_dice(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i), 0);
    mob_proto[i].mob_specials.damnodice += mob_proto[i].mob_specials.ex_damnodice;
    mob_proto[i].mob_specials.damsizedice =
        get_set_dice(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i), GET_CLASS(mob_proto + i), 1);
    mob_proto[i].mob_specials.damsizedice += mob_proto[i].mob_specials.ex_damsizedice;

    /*last check values above 0 */
    mob_proto[i].mob_specials.damsizedice = std::max<sbyte>(0, mob_proto[i].mob_specials.damsizedice);
    mob_proto[i].mob_specials.damnodice = std::max<sbyte>(0, mob_proto[i].mob_specials.damnodice);
    mob_proto[i].points.hitroll = std::max(0, mob_proto[i].points.hitroll);
    mob_proto[i].points.damroll = std::max(0, mob_proto[i].points.damroll);
    mob_proto[i].points.armor = std::clamp(mob_proto[i].points.armor, -100, 100);

    for (j = 0; j < 3; j++)
        GET_COND(mob_proto + i, j) = -1;

    /*
     * these are now save applies; base save numbers for MOBs are now from
     * the warrior save table.
     */
    for (j = 0; j < 5; j++)
        GET_SAVE(mob_proto + i, j) = 0;
    /* mobs now get the same spells/skills as pc's */
    /*set_mob_skills(i, t[3], t[4]); */
}

/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

#define CASE(test) if (!matched && !strcasecmp(keyword, test) && (matched = 1))
/* modified for the 100 attrib scale */
void interpret_espec(char *keyword, char *value, int i, int nr) {
    int num_arg, matched = 0;

    num_arg = atoi(value);

    CASE("BareHandAttack") {
        num_arg = std::clamp(num_arg, 0, 99);
        mob_proto[i].mob_specials.attack_type = num_arg;
    }

    CASE("Str") {
        num_arg = std::clamp(num_arg, 30, 100);
        mob_proto[i].natural_abils.str = num_arg;
    }

    CASE("Int") {
        num_arg = std::clamp(num_arg, 30, 100);
        mob_proto[i].natural_abils.intel = num_arg;
    }

    CASE("Wis") {
        num_arg = std::clamp(num_arg, 30, 100);
        mob_proto[i].natural_abils.wis = num_arg;
    }

    CASE("Dex") {
        num_arg = std::clamp(num_arg, 30, 100);
        mob_proto[i].natural_abils.dex = num_arg;
    }

    CASE("Con") {
        num_arg = std::clamp(num_arg, 30, 100);
        mob_proto[i].natural_abils.con = num_arg;
    }

    CASE("Cha") {
        num_arg = std::clamp(num_arg, 30, 100);
        mob_proto[i].natural_abils.cha = num_arg;
    }

    CASE("AFF2") { EFF_FLAGS(mob_proto + i)[1] = num_arg; }

    CASE("AFF3") { EFF_FLAGS(mob_proto + i)[2] = num_arg; }

    CASE("MOB2") { MOB_FLAGS(mob_proto + i)[1] = num_arg; }

    CASE("PERC") { GET_PERCEPTION(mob_proto + i) = num_arg; }

    CASE("HIDE") { GET_HIDDENNESS(mob_proto + i) = num_arg; }

    CASE("Lifeforce") { GET_LIFEFORCE(mob_proto + i) = num_arg; }

    CASE("Composition") {
        BASE_COMPOSITION(mob_proto + i) = num_arg;
        GET_COMPOSITION(mob_proto + i) = num_arg;
    }

    CASE("Stance") { GET_STANCE(mob_proto + i) = num_arg; }

    if (!matched) {
        fprintf(stderr, "Warning: unrecognized espec keyword %s in mob #%d\n", keyword, nr);
    }
}

#undef CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr) {
    char *ptr;

    if ((ptr = strchr(buf, ':')) != nullptr) {
        *(ptr++) = '\0';
        while (isspace(*ptr))
            ptr++;
    } else
        ptr = strdup("");

    interpret_espec(buf, ptr, i, nr);
}

void parse_enhanced_mob(FILE *mob_f, int i, int nr) {
    char line[256];

    parse_simple_mob(mob_f, i, nr);

    while (get_line(mob_f, line)) {
        if (!strcasecmp(line, "E")) /* end of the ehanced section */
            return;
        else if (*line == '#') { /* we've hit the next mob, maybe? */
            fprintf(stderr, "Unterminated E section in mob #%d\n", nr);
            exit(1);
        } else
            parse_espec(line, i, nr);
    }

    fprintf(stderr, "Unexpected end of file reached after mob #%d\n", nr);
    exit(1);
}

void parse_mobile(FILE *mob_f, int nr) {
    static int i = 0;
    int j, t[10];
    char line[256], *tmpptr, letter;
    char f1[128], f2[128];

    mob_index[i].vnum = nr;
    mob_index[i].number = 0;
    mob_index[i].func = nullptr;
    clear_char(mob_proto + i);

    sprintf(buf2, "mob vnum %d", nr);

    /***** String data *** */
    mob_proto[i].player.namelist = fread_string(mob_f, buf2);
    tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
    if (tmpptr && *tmpptr)
        if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") || !strcasecmp(fname(tmpptr), "the"))
            *tmpptr = LOWER(*tmpptr);
    mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
    mob_proto[i].player.description = fread_string(mob_f, buf2);
    mob_proto[i].player.title = nullptr;
    mob_proto[i].player.prompt = nullptr;

    /* *** Numeric data *** */
    mob_proto[i].mob_specials.nr = i;
    get_line(mob_f, line);
    sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter);
    MOB_FLAGS(mob_proto + i)[0] = asciiflag_conv(f1);
    SET_FLAG(MOB_FLAGS(mob_proto + i), MOB_ISNPC);
    EFF_FLAGS(mob_proto + i)[0] = asciiflag_conv(f2);
    GET_ALIGNMENT(mob_proto + i) = t[2];

    switch (letter) {
    case 'S': /* Simple monsters */
        parse_simple_mob(mob_f, i, nr);
        break;
    case 'E': /* Circle3 Enhanced monsters */
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
    while (letter == 'T') {
        dg_read_trigger(mob_f, &mob_proto[i], MOB_TRIGGER);
        letter = fread_letter(mob_f);
        ungetc(letter, mob_f);
    }

    mob_proto[i].affected_abils = mob_proto[i].natural_abils;

    for (j = 0; j < NUM_WEARS; j++)
        mob_proto[i].equipment[j] = nullptr;

    mob_proto[i].desc = nullptr;

    letter = fread_letter(mob_f);
    if (letter == '>') {
        while (fread_letter(mob_f) != '|')
            ;
        fprintf(stderr, "Mob %d has a mobprog still!\n", nr);
    } else
        ungetc(letter, mob_f);

    if (mob_proto[i].mob_specials.default_pos < 0 || mob_proto[i].mob_specials.default_pos >= NUM_POSITIONS) {
        mob_proto[i].mob_specials.default_pos = POS_STANDING;
    }
    if (mob_proto[i].char_specials.position < 0 || mob_proto[i].char_specials.position >= NUM_POSITIONS) {
        mob_proto[i].char_specials.position = POS_STANDING;
    }

    top_of_mobt = i++;
}

void verify_obj_spell(ObjData *obj, int valnum, bool zero_ok) {
    if (!IS_SPELL(GET_OBJ_VAL(obj, valnum)) && !(zero_ok && GET_OBJ_VAL(obj, valnum) == 0)) {
        int object_vnum = GET_OBJ_VNUM(obj);
        int spellnum = GET_OBJ_VAL(obj, valnum);
        log(LogSeverity::Warn, LVL_IMMORT, "ERROR: Invalid spell in object prototype. vnum={:d} spellnum={:d}",
            object_vnum, spellnum);
        /* Replace invalid spell with a benign value */
        if (zero_ok)
            GET_OBJ_VAL(obj, valnum) = 0;
        else
            GET_OBJ_VAL(obj, valnum) = SPELL_IDENTIFY;
    }
}

/* Do some post-definition processing on certain object types */
void init_obj_proto(ObjData *obj) {
    char *s;
    ExtraDescriptionData *ed;

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
                if (!strcasecmp(ed->keyword, obj->name)) {
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
        verify_obj_spell(obj, VAL_SCROLL_SPELL_1, false);
        verify_obj_spell(obj, VAL_SCROLL_SPELL_2, true);
        verify_obj_spell(obj, VAL_SCROLL_SPELL_3, true);
        break;
    case ITEM_WAND:
        verify_obj_spell(obj, VAL_WAND_SPELL, false);
        break;
    case ITEM_STAFF:
    case ITEM_INSTRUMENT:
        verify_obj_spell(obj, VAL_STAFF_SPELL, false);
        break;
    case ITEM_POTION:
        verify_obj_spell(obj, VAL_POTION_SPELL_1, false);
        verify_obj_spell(obj, VAL_POTION_SPELL_2, true);
        verify_obj_spell(obj, VAL_POTION_SPELL_3, true);
        break;
    }

    /* Remove flags we don't want on prototypes */
    REMOVE_FLAG(GET_OBJ_FLAGS(obj), ITEM_WAS_DISARMED);
}

/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE *obj_f, int nr) {
    static int i = 0, retval;
    static char line[256];
    int t[10], j;
    char *tmpptr;
    char f1[256], f2[256];
    ExtraDescriptionData *new_descr, *edx;

    obj_index[i].vnum = nr;
    obj_index[i].number = 0;
    obj_index[i].func = nullptr;

    clear_object(obj_proto + i);
    obj_proto[i].in_room = NOWHERE;
    obj_proto[i].item_number = i;

    sprintf(buf2, "object #%d", nr);

    /* *** string data *** */
    if ((obj_proto[i].name = fread_string(obj_f, buf2)) == nullptr) {
        fprintf(stderr, "Null obj name or format error at or near %s\n", buf2);
        exit(1);
    }
    tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
    if (*tmpptr)
        if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") || !strcasecmp(fname(tmpptr), "the"))
            *tmpptr = LOWER(*tmpptr);

    tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
    if (tmpptr && *tmpptr)
        *tmpptr = UPPER(*tmpptr);
    obj_proto[i].action_description = fread_string(obj_f, buf2);

    /* *** numeric data *** */
    if (!get_line(obj_f, line) || (retval = sscanf(line, " %d %s %s %d", t, f1, f2, t + 1)) != 4) {
        if (retval == 3) {
            sscanf(line, " %d %s %s", t, f1, f2);
            t[1] = 0;
            fprintf(stderr, "Object #%d needs a level assigned to it.\n", nr);
        } else {
            fprintf(stderr, "Format error in first numeric line (expecting 4 args, got %d), %s\n", retval, buf2);
            /* exit(1); */
        }
    }
    obj_proto[i].obj_flags.type_flag = t[0];
    obj_proto[i].obj_flags.extra_flags[0] = asciiflag_conv(f1);
    obj_proto[i].obj_flags.wear_flags = asciiflag_conv(f2);
    obj_proto[i].obj_flags.level = t[1]; /* Zantir 3/23/01 for level based objects */

    if (!get_line(obj_f, line) ||
        (retval = sscanf(line, "%d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6)) != 7) {
        fprintf(stderr, "Format error in second numeric line (expecting 7 args, got %d), %s\n", retval, buf2);
        /*exit(1); */
    }
    obj_proto[i].obj_flags.value[0] = t[0];
    obj_proto[i].obj_flags.value[1] = t[1];
    obj_proto[i].obj_flags.value[2] = t[2];
    obj_proto[i].obj_flags.value[3] = t[3];
    obj_proto[i].obj_flags.value[4] = t[4];
    obj_proto[i].obj_flags.value[5] = t[5];
    obj_proto[i].obj_flags.value[6] = t[6];

    if (!get_line(obj_f, line) || (retval = sscanf(line, "%f %d %d %d %d %d %d %d", &obj_proto[i].obj_flags.weight,
                                                   t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7)) != 8) {
        fprintf(stderr, "Format error in third numeric line (expecting 8 args, got %d), %s\n", retval, buf2);
        /*exit(1); */
    }

    obj_proto[i].obj_flags.effective_weight = obj_proto[i].obj_flags.weight;
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
            CREATE(new_descr, ExtraDescriptionData, 1);
            new_descr->keyword = fread_string(obj_f, buf2);
            new_descr->description = fread_string(obj_f, buf2);
            /* Put each extra desc at the end of the list as we read them.
             * Otherwise they will be reversed (yes, we really don't want that -
             * see act.informative.c, show_obj_to_char(), mode 5). */
            if ((edx = obj_proto[i].ex_description)) {
                for (; edx->next; edx = edx->next)
                    ;
                edx->next = new_descr;
            } else
                obj_proto[i].ex_description = new_descr;
            break;
        case 'A':
            if (j >= MAX_OBJ_APPLIES) {
                fprintf(stderr, "Too many A fields (%d max), %s\n", MAX_OBJ_APPLIES, buf2);
                exit(1);
            }

            if (!get_line(obj_f, line) || (retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
                fprintf(stderr, "Format error in Affect line (expecting 2 args, got %d), %s\n", retval, buf2);
                /*exit(1); */
            }
            /* get_line(obj_f, line);
               sscanf(line, " %d %d ", t, t + 1); */
            obj_proto[i].applies[j].location = t[0];
            obj_proto[i].applies[j].modifier = t[1];
            j++;
            break;

        case 'H': /* Hiddenness */
            get_line(obj_f, line);
            sscanf(line, "%d ", t);
            obj_proto[i].obj_flags.hiddenness = t[0];
            break;
        case 'T': /* DG triggers */
            dg_obj_trigger(line, &obj_proto[i]);
            break;

        case 'X':
            get_line(obj_f, line);
            sscanf(line, "%d ", t);
            obj_proto[i].obj_flags.extra_flags[1] = t[0];
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

#define Z zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE *fl, char *zonename) {
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
        CREATE(Z.cmd, ResetCommand, num_of_cmds);

    line_num += get_line(fl, buf);

    if (sscanf(buf, "#%d", &Z.number) != 1) {
        fprintf(stderr, "Format error in %s, line %d\n", zname, line_num);
        exit(0);
    }
    sprintf(buf2, "beginning of zone #%d", Z.number);

    line_num += get_line(fl, buf);
    if ((ptr = strchr(buf, '~')) != nullptr) /* take off the '~' if it's there */
        *ptr = '\0';
    Z.name = strdup(buf);

    line_num += get_line(fl, buf);
    if (sscanf(buf, " %d %d %d %d %d %d", &Z.top, &Z.lifespan, &Z.reset_mode, &Z.zone_factor, &Z.hemisphere,
               &Z.climate) != 4)
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
        if (ZCMD.command == 'F') { /* force mobile command */
            skip_spaces(&ptr);
            if (*ptr) {
                tmp = *ptr;
                ptr++;
                skip_spaces(&ptr);
                ZCMD.sarg = strdup(ptr);
            } else
                error = 1;
        } else if (strchr("MOEPD", ZCMD.command) == nullptr) { /* a 3-arg command */
            if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
                error = 1;
        } else {
            if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3) != 4)
                error = 1;
        }

        ZCMD.if_flag = tmp;

        if (error) {
            fprintf(stderr, "Format error in %s, line %d: '%s'\n", zname, line_num, buf);
            exit(0);
        }
        ZCMD.line = line_num;
        cmd_no++;
    }

    top_of_zone_table = zone++;
}

#undef Z

void get_one_line(FILE *fl, char *line_buf) {
    *line_buf = '\0';

    if (fgets(line_buf, READ_SIZE, fl) == nullptr) {
        log("error reading help file: not terminated with $?");
        exit(1);
    }

    line_buf[strlen(line_buf) - 1] = '\0'; /* take off the trailing \n */
}

void load_help(FILE *fl) {
    char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
    char line[READ_SIZE + 1], *scan;
    HelpIndexElement el;

    /* get the first keyword line */
    get_one_line(fl, key);

    if (!strlen(key)) {
        log("Illegal blank line in help file");
        abort();
    }
    while (*key != '$') {
        /* read in the corresponding help entry */
        strcpy(entry, strcat(key, "\n"));
        get_one_line(fl, line);
        while (*line != '#') {
            strcat(entry, strcat(line, "\n"));
            get_one_line(fl, line);
        }

        el.min_level = 0;
        if ((*line == '#') && (*(line + 1) != 0))
            el.min_level = atoi((line + 1));

        el.min_level = std::clamp(el.min_level, 0, LVL_IMPL);

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

        if (!strlen(key)) {
            log("Illegal blank line in help file");
            abort();
        }
    }
}

int hsort(const void *a, const void *b) {
    HelpIndexElement *a1, *b1;

    a1 = (HelpIndexElement *)a;
    b1 = (HelpIndexElement *)b;

    return (strcasecmp(a1->keyword, b1->keyword));
}

void free_help_table(void) {
    if (help_table) {
        int hp;
        for (hp = 0; hp <= top_of_helpt; hp++) {
            if (help_table[hp].keyword)
                free(help_table[hp].keyword);
            if (help_table[hp].entry && !help_table[hp].duplicate)
                free(help_table[hp].entry);
        }
        free(help_table);
        help_table = nullptr;
    }
    top_of_helpt = 0;
}

/*************************************************************************
 *  procedures for resetting, both play-time and boot-time                  *
 *********************************************************************** */

int vnum_zone(char *searchname, CharData *ch) {
    int nr, found = 0;

    for (nr = 0; nr <= top_of_zone_table; nr++) {
        if (isname(searchname, zone_table[nr].name)) {
            char_printf(ch, "{:3d}. [{:5d}] {}\n", ++found, zone_table[nr].number, zone_table[nr].name);
        }
    }
    return (found);
}

/* create a character, and add it to the char list */
CharData *create_char(void) {
    CharData *ch;

    CREATE(ch, CharData, 1);
    clear_char(ch);
    ch->next = character_list;
    character_list = ch;
    GET_ID(ch) = max_id++;
    return ch;
}

/* set up mob for loading. for use in read_mobile and anywhere else we want
 * to make a mob from scratch -- mostly copied n pasted from read_mobile - 321
 */
void setup_mob(CharData *mob) {
    extern void roll_natural_abils(CharData * ch);

    mob->player_specials = &dummy_mob;
    /* mobs now get rolled just like PC's --gurlaek 6/28/1999 */
    roll_natural_abils(mob);
    mob->actual_abils = mob->natural_abils;
    scale_attribs(mob);

    /* Set skills, innates, and other things according to class and species: */
    init_char(mob);

    if (!mob->points.max_hit)
        mob->points.max_hit =
            std::clamp(roll_dice(mob->points.hit, mob->points.mana) + GET_EX_MAIN_HP(mob) + mob->points.move, 0, 32000);
    else
        mob->points.max_hit = random_number(mob->points.hit, mob->points.mana);

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
CharData *read_mobile(int nr, int type) {
    int i;
    CharData *mob;

    if (type == VIRTUAL) {
        if ((i = real_mobile(nr)) < 0) {
            sprintf(buf, "Mobile (V) %d does not exist in database.", nr);
            return (0);
        }
    } else
        i = nr;

    CREATE(mob, CharData, 1);
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
ObjData *create_obj(void) {
    ObjData *obj;

    CREATE(obj, ObjData, 1);
    clear_object(obj);
    obj->next = object_list;
    object_list = obj;
    GET_ID(obj) = max_id++;
    assign_triggers(obj, OBJ_TRIGGER);

    return obj;
}

/* create a new object from a prototype */
ObjData *read_object(int nr, int type) {
    ObjData *obj;
    int i;

    if (nr < 0) {
        log("SYSERR: trying to create obj with negative num!");
        return nullptr;
    }
    if (type == VIRTUAL) {
        if ((i = real_object(nr)) < 0) {
            sprintf(buf, "Object (V) %d does not exist in database.", nr);
            return nullptr;
        }
    } else
        i = nr;

    CREATE(obj, ObjData, 1);
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

#define ZO_DEAD 999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void) {
    int i;
    ResetQElement *update_u, *temp;
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
            if (zone_table[i].age < zone_table[i].lifespan && zone_table[i].reset_mode)
                (zone_table[i].age)++;

            if (zone_table[i].age >= zone_table[i].lifespan && zone_table[i].age < ZO_DEAD &&
                zone_table[i].reset_mode) {
                /* enqueue zone */

                CREATE(update_u, ResetQElement, 1);

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
    } /* end - one minute has passed */

    /* dequeue zones (if possible) and reset */
    /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
    for (update_u = reset_q.head; update_u; update_u = update_u->next)
        if (zone_table[update_u->zone_to_reset].reset_mode == 2 || is_empty(update_u->zone_to_reset)) {
            reset_zone(update_u->zone_to_reset, false);
            if (update_u == reset_q.head)
                reset_q.head = reset_q.head->next;
            else {
                for (temp = reset_q.head; temp->next != update_u; temp = temp->next)
                    ;

                if (!update_u->next)
                    reset_q.tail = temp;

                temp->next = update_u->next;
            }

            free(update_u);
            break;
        }
}

void log_zone_error(int zone, int cmd_no, const char *message) {
    log(LogSeverity::Stat, LVL_GOD, "SYSERR: error in zone file: {}", message);
    log(LogSeverity::Stat, LVL_GOD, "SYSERR: ...offending cmd: '{:c}' cmd in zone #{:d}, line {:d}", ZCMD.command,
        zone_table[zone].number, ZCMD.line);
}

#define ZONE_ERROR(message)                                                                                            \
    {                                                                                                                  \
        log_zone_error(zone, cmd_no, message);                                                                         \
        last_cmd = 0;                                                                                                  \
    }

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
void reset_zone(int zone, byte pop) {
    int cmd_no, cmd_other, last_cmd = 0;
    CharData *mob = nullptr;
    ObjData *obj, *obj_to;
    room_num other_room;
    ResetCommand *ocmd;

    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {

        if (ZCMD.if_flag && !last_cmd)
            continue;

        switch (ZCMD.command) {
        case '*': /* ignore command */
            last_cmd = 0;
            break;

        case 'F': /* force mobile to do action */
            if (!mob) {
                ZONE_ERROR("attempt to force-command a non-existent mob");
                break;
            }
            command_interpreter(mob, ZCMD.sarg);
            break;

        case 'M': /* read a mobile */
            if (mob_index[ZCMD.arg1].number < ZCMD.arg2) {
                mob = read_mobile(ZCMD.arg1, REAL);
                char_to_room(mob, ZCMD.arg3);
                load_mtrigger(mob);
                last_cmd = 1;
            } else
                last_cmd = 0;
            break;

        case 'O': /* read an object */

            if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
                obj = read_object(ZCMD.arg1, REAL);
                if (ZCMD.arg3 >= 0) {
                    obj_to_room(obj, ZCMD.arg3);
                    last_cmd = 1;
                } else {
                    obj->in_room = NOWHERE;
                    last_cmd = 1;
                }
            } else
                last_cmd = 0;
            break;

        case 'P': /* object to object */
            if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
                obj = read_object(ZCMD.arg1, REAL);
                if (!(obj_to = find_obj_in_world(find_by_rnum(ZCMD.arg3)))) {
                    ZONE_ERROR("target obj not found");
                    extract_obj(obj);
                    break;
                }
                obj_to_obj(obj, obj_to);
                last_cmd = 1;
            } else
                last_cmd = 0;
            break;

        case 'G': /* obj_to_char */
            if (!mob) {
                ZONE_ERROR("attempt to give obj to non-existent mob");
                break;
            }

            if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
                obj = read_object(ZCMD.arg1, REAL);

                if (GET_LEVEL(mob) < GET_OBJ_LEVEL(obj)) {
                    char error_buf[2048];
                    sprintf(error_buf,
                            "Mob %s [%d] in room %d  cannot use object "
                            "%s [%d] because its level is to low.",
                            GET_NAME(mob), GET_MOB_VNUM(mob), mob->in_room, obj->short_description, GET_OBJ_VNUM(obj));
                }

                obj_to_char(obj, mob);
                last_cmd = 1;
            } else
                last_cmd = 0;
            break;

        case 'E': /* object to equipment list */
            if (!mob) {
                ZONE_ERROR("trying to equip non-existent mob");
                break;
            } /*was make sure number less then ZCMD.arg2 */
            if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
                if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
                    ZONE_ERROR("invalid equipment pos number");
                } else {
                    obj = read_object(ZCMD.arg1, REAL);
                    if (equip_char(mob, obj, ZCMD.arg3) != EQUIP_RESULT_SUCCESS) {
                        log(LogSeverity::Error, 101,
                            "EQUIP zone command for {} [{:d}] in room {:d} to equip {} [{:d}] failed.", GET_NAME(mob),
                            GET_MOB_VNUM(mob), ROOM_RNUM_TO_VNUM(mob->in_room), obj->short_description,
                            GET_OBJ_VNUM(obj));
                        extract_obj(obj);
                    }
                    last_cmd = 1;
                }
            } else
                last_cmd = 0;
            break;

        case 'R': /* rem obj from room */
            if ((obj = find_obj_in_list(world[ZCMD.arg1].contents, find_by_rnum(ZCMD.arg2))) != nullptr) {
                obj_from_room(obj);
                extract_obj(obj);
            }
            last_cmd = 1;
            break;

        case 'D': /* set state of door */
            if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS || (world[ZCMD.arg1].exits[ZCMD.arg2] == nullptr)) {
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
                if ((other_room = world[ZCMD.arg1].exits[ZCMD.arg2]->to_room) != NOWHERE) {

                    /* Make sure the destination room is in a different zone */
                    if (world[other_room].zone != world[ZCMD.arg1].zone) {

                        /* Make sure the destination room has an exit pointing back */
                        if (world[other_room].exits[rev_dir[ZCMD.arg2]]) {

                            /* Make sure that exit is pointing back to this room */
                            if (world[other_room].exits[rev_dir[ZCMD.arg2]]->to_room == ZCMD.arg1) {

                                /* Ready to perform its door zone commands */

                                /* Must examine every zone command in that zone */
                                for (cmd_other = 0;; cmd_other++) {
                                    ocmd = &(zone_table[world[other_room].zone].cmd[cmd_other]);
                                    if (ocmd->command == 'S')
                                        break;

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

/* for use in reset_zone; return true if zone 'nr' is free of PC's  */
int is_empty(int zone_nr) {
    DescriptorData *i;

    for (i = descriptor_list; i; i = i->next)
        if (!i->connected)
            if (world[i->character->in_room].zone == zone_nr)
                return 0;

    return 1;
}

/* now scaled for 100 point attribs */
int static_ac(int dex) {
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
int con_aff(CharData *ch) {
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
char *fread_string(FILE *fl, const char *error) {
    char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
    char *point;
    int done = 0, length = 0, templength = 0;

    *buf = '\0';

    do {
        if (!fgets(tmp, 512, fl)) {
            fprintf(stderr, "SYSERR: fread_string: format error at or near %s\n", error);
            exit(1);
        }
        /* If there is a '~', end the string; else put an "\n" over the '\n'. */
        if ((point = strchr(tmp, '~')) != nullptr) {
            *point = '\0';
            done = 1;
        } else {
            point = tmp + strlen(tmp) - 1;
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

    // Strip trailing newlines.
    while (length > 0 && (buf[length - 1] == '\n' || buf[length - 1] == '\r'))
        buf[--length] = '\0';

    /* allocate space for the new string and copy it */
    if (strlen(buf) > 0) {
        CREATE(rslt, char, length + 1);
        strcpy(rslt, buf);
    } else
        rslt = nullptr;

    return rslt;
}

/* release memory allocated for a char struct */
void free_char(CharData *ch) {
    int i;
    KnowSpell *tmp, *tmp2;

    /* Generally speaking, extract_char() should have called char_from_room()
     * which would have cleared all battling variables.  So if ch is *still*
     * in some kind of battle mode here, something is definitely wrong. */
    if (ch->attackers || ch->target)
        log(LogSeverity::Error, LVL_GOD, "free_char: {} is in battle", GET_NAME(ch));

    while (ch->effects)
        effect_remove(ch, ch->effects);

    /* free spell recognition list if it exists */
    for (tmp = ch->see_spell; tmp; tmp = tmp2) {
        tmp2 = tmp->next;
        free(tmp);
    }
    ch->see_spell = nullptr;

    if (ch->casting.misc)
        free(ch->casting.misc);

    if (ch->player_specials != nullptr && ch->player_specials != &dummy_mob) {
        free_trophy(ch);
        free_aliases(GET_ALIASES(ch));

        /* Remove runtime link to clan */
        if (GET_CLAN_MEMBERSHIP(ch))
            GET_CLAN_MEMBERSHIP(ch)->player = nullptr;

        if (GET_WIZ_TITLE(ch))
            free(GET_WIZ_TITLE(ch));
        if (GET_PERM_TITLES(ch)) {
            for (i = 0; GET_PERM_TITLES(ch)[i]; ++i)
                free(GET_PERM_TITLES(ch)[i]);
            free(GET_PERM_TITLES(ch));
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
    } else if ((i = GET_MOB_RNUM(ch)) > -1) {
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
void free_obj(ObjData *obj) {
    SpellBookList *this_book, *next_book;

    free_obj_strings(obj);

    for (this_book = obj->spell_book; this_book; this_book = next_book) {
        next_book = this_book->next;
        free(this_book);
    }

    if (SCRIPT(obj))
        extract_script(SCRIPT(obj));

    free(obj);
}

/* read contents of a text file, alloc space, point buf to it */
int file_to_string_alloc(const char *name, char **buf) {
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
int file_to_string(const char *name, char *buf) {
    FILE *fl;
    char tmp[READ_SIZE + 3];

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
        strcat(tmp, "\n");
        if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
            fclose(fl);
            return 0;
        }
        strcat(buf, tmp);
    }
    fclose(fl);
    return (0);
}

/* Get the unix timestamp of the last file write. */
time_t file_last_update(const char *name) {
    struct stat attr;
    stat(name, &attr);
    return attr.st_mtim.tv_sec;
}

/* clear some of the the working variables of a char */
void reset_char(CharData *ch) {
    int i;

    for (i = 0; i < NUM_WEARS; i++)
        GET_EQ(ch, i) = nullptr;
    REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
    CONSENT(ch) = nullptr;
    ch->scribe_list = nullptr;
    ch->followers = nullptr;
    ch->master = nullptr;
    ch->groupees = nullptr;
    ch->group_master = nullptr;
    ch->followers = nullptr;
    ch->master = nullptr;
    ch->in_room = NOWHERE;
    ch->carrying = nullptr;
    ch->next = nullptr;
    ch->next_fighting = nullptr;
    ch->next_in_room = nullptr;
    ch->target = nullptr;
    ch->attackers = nullptr;
    ch->next_attacker = nullptr;
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
}

/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(CharData *ch) {
    memset((char *)ch, 0, sizeof(CharData));

    ch->in_room = NOWHERE;
    GET_PFILEPOS(ch) = -1;
    GET_WAS_IN(ch) = NOWHERE;
    GET_POS(ch) = POS_STANDING;
    GET_STANCE(ch) = STANCE_ALERT;
    ch->mob_specials.default_pos = POS_STANDING;
    clear_cooldowns(ch);
    GET_AC(ch) = 100; /* Basic Armor */
    if (ch->points.max_mana < 100)
        ch->points.max_mana = 100;
}

void clear_object(ObjData *obj) {
    memset((char *)obj, 0, sizeof(ObjData));

    obj->item_number = NOTHING;
    obj->in_room = NOWHERE;
}

/* returns the real number of the room with given vnum */
int real_room(int vnum) {
    int bot, top, mid;

    bot = 0;
    top = top_of_world;

    /* perform binary search on world-table */
    for (;;) {
        mid = (bot + top) / 2;

        if ((world + mid)->vnum == vnum)
            return mid;
        if (bot >= top)
            return NOWHERE;
        if ((world + mid)->vnum > vnum)
            top = mid - 1;
        else
            bot = mid + 1;
    }
}

int real_quest(unsigned short vnum) {
    int bot, top, mid;

    if (!all_quests) {
        return NOWHERE;
    }

    bot = 0;
    top = max_quests;

    /* perform binary search on quest-table */
    for (;;) {
        mid = (bot + top) / 2;

        if ((all_quests + mid)->quest_id == vnum)
            return mid;
        if (bot >= top)
            return NOWHERE;
        if ((all_quests + mid)->quest_id > vnum)
            top = mid - 1;
        else
            bot = mid + 1;
    }
}

/* returns the real number of the monster with given vnum */
int real_mobile(int vnum) {
    int bot, top, mid;

    bot = 0;
    top = top_of_mobt;

    /* perform binary search on mob-table */
    for (;;) {
        mid = (bot + top) / 2;

        if ((mob_index + mid)->vnum == vnum)
            return (mid);
        if (bot >= top)
            return (-1);
        if ((mob_index + mid)->vnum > vnum)
            top = mid - 1;
        else
            bot = mid + 1;
    }
}

/* returns the real number of the object with given vnum */
int real_object(int vnum) {
    int bot, top, mid;

    bot = 0;
    top = top_of_objt;

    /* perform binary search on obj-table */
    for (;;) {
        mid = (bot + top) / 2;

        if ((obj_index + mid)->vnum == vnum)
            return (mid);
        if (bot >= top)
            return (-1);
        if ((obj_index + mid)->vnum > vnum)
            top = mid - 1;
        else
            bot = mid + 1;
    }
}

/*
 * Read a number from a file.
 */
int fread_number(FILE *fp) {
    int number;
    bool sign;
    char c;

    do {
        c = getc(fp);
    } while (isspace(c));

    number = 0;

    sign = false;
    if (c == '+') {
        c = getc(fp);
    } else if (c == '-') {
        sign = true;
        c = getc(fp);
    }

    if (!isdigit(c)) {
        log("Fread_number: bad format.");
        exit(1);
    }

    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
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
void fread_to_eol(FILE *fp) {
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
char *fread_word(FILE *fp) {
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd;

    do {
        cEnd = getc(fp);
    } while (isspace(cEnd));

    if (cEnd == '\'' || cEnd == '"') {
        pword = word;
    } else {
        word[0] = cEnd;
        pword = word + 1;
        cEnd = ' ';
    }

    for (; pword < word + MAX_INPUT_LENGTH; pword++) {
        *pword = getc(fp);
        if (cEnd == ' ' ? isspace(*pword) || *pword == '~' : *pword == cEnd) {
            if (cEnd == ' ' || cEnd == '~')
                ungetc(*pword, fp);
            *pword = '\0';
            return word;
        }
    }

    log("SYSERR: Fread_word: word too long.");
    exit(1);
    return nullptr;
}

IndexData *get_obj_index(int vnum) {
    int nr;
    for (nr = 0; nr <= top_of_objt; nr++) {
        if (obj_index[nr].vnum == vnum)
            return &obj_index[nr];
    }
    return nullptr;
}

IndexData *get_mob_index(int vnum) {
    int nr;
    for (nr = 0; nr <= top_of_mobt; nr++) {
        if (mob_index[nr].vnum == vnum)
            return &mob_index[nr];
    }
    return nullptr;
}

bool _parse_name(char *arg, char *name) {
    int i;
    char test[32];
    const char *smart_ass[] = {"someone", "somebody", "me",   "self",      "all",   "group", "local", "them",
                               "they",    "nobody",   "any",  "something", "other", "no",    "yes",   "north",
                               "east",    "south",    "west", "up",        "down",  "shape", /* infra.. */
                               "shadow",                                                     /* summon */
                               "\n"};

    test[0] = 0;
    for (i = 0; (*name = *arg); arg++, i++, name++) {
        *(test + i) = LOWER(*arg);
        if ((*arg < 0) || !isalpha(*arg) || (i > 15) || (i && (*(test + i) != *arg)))
            return (true);
    }

    if (i < 2)
        return (true);

    /* We need the case-insensitive search_block since arg can have uppercase */
    for (i = 0; *cmd_info[i].command != '\n'; ++i)
        if (!strcasecmp(arg, cmd_info[i].command))
            return true;
    if (search_block(arg, smart_ass, true) >= 0)
        return true;

    return false;
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
    while (*tmp == ':' || *tmp == ' ')
        tmp++;

    while (*tmp)
        *(wrt++) = *(tmp++);
    *wrt = '\0';
}

/* remove ^M's from file output */
/* There is a *similar* function in Oasis, but it has a bug. */
void kill_ems(char *str) {
    char *ptr1, *ptr2;

    ptr1 = str;
    ptr2 = str;

    while (*ptr1) {
        if (*ptr1 == '\r')
            ptr1++;
        else {
            *(ptr2++) = *(ptr1++);
        }
    }
    *ptr2 = '\0';
}
