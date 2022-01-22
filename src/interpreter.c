/***************************************************************************
 *   File: interpreter.c                                  Part of FieryMUD *
 *  Usage: parse user commands, search for specials, call ACMD functions   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#define __INTERPRETER_C__

#include "interpreter.h"

#include "act.h"
#include "casting.h"
#include "clan.h"
#include "class.h"
#include "comm.h"
#include "commands.h"
#include "conf.h"
#include "constants.h"
#include "cooldowns.h"
#include "db.h"
#include "dg_scripts.h"
#include "events.h"
#include "fight.h"
#include "handler.h"
#include "mail.h"
#include "math.h"
#include "modify.h"
#include "olc.h"
#include "pfiles.h"
#include "players.h"
#include "privileges.h"
#include "races.h"
#include "screen.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "textfiles.h"
#include "utils.h"

extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern char *subclass_descrip;
extern char *subclass_descrip2;
extern int should_restrict;
extern int restrict_reason;
extern int approve_names;
extern int napprove_pause;
extern int races_allowed;
extern int good_races_allowed;

/* external functions */
void broadcast_name(char *name);
void echo_on(struct descriptor_data *d);
void echo_off(struct descriptor_data *d);
int special(struct char_data *ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);
void oedit_parse(struct descriptor_data *d, char *arg);
void redit_parse(struct descriptor_data *d, char *arg);
void zedit_parse(struct descriptor_data *d, char *arg);
void medit_parse(struct descriptor_data *d, char *arg);
void sedit_parse(struct descriptor_data *d, char *arg);
void hedit_parse(struct descriptor_data *d, char *arg);
void sdedit_parse(struct descriptor_data *d, char *arg);
int roll_table[6];
void send_to_xnames(char *name);
void personal_reboot_warning(struct char_data *ch);

void display_question(struct descriptor_data *d);
/*void rolls_display(struct char_data *ch, char *[], char *[]);*/
void roll_natural_abils(struct char_data *ch);
void new_rollor_display(struct char_data *ch, int[]);
int bonus_stat(struct char_data *ch, char arg);
int parse_good_race(char arg); /* Put in until such time that all races are allowed */
void set_innate(struct char_data *ch, char *arg);
void trigedit_parse(struct descriptor_data *d, char *arg);
extern char *diety_selection;
void appear(struct char_data *ch);
int make_count = 0;
EVENTFUNC(name_timeout);
extern bool ispell_name_check(char *);

/* prototypes for all do_x functions. */
ACMD(do_abort);
ACMD(do_abandon);
ACMD(do_action);
ACMD(do_advance);
ACMD(do_aggr);
ACMD(do_alert);
ACMD(do_alias);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bandage);
ACMD(do_bash);
ACMD(do_berserk);
ACMD(do_bind);
ACMD(do_boardadmin);
ACMD(do_disarm);
ACMD(do_disengage);
ACMD(do_breathe);
ACMD(do_buck);
ACMD(do_call);
ACMD(do_cast);
ACMD(do_camp);
ACMD(do_clan);
ACMD(do_claw);
ACMD(do_compare);
ACMD(do_conceal);
ACMD(do_consent);
ACMD(do_coredump);
ACMD(do_corner);
ACMD(do_credits);
ACMD(do_ctell);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_desc);
ACMD(do_diagnose);
ACMD(do_dismount);
ACMD(do_display);
ACMD(do_disband);
ACMD(do_dig);
ACMD(do_doorbash);
ACMD(do_douse);
ACMD(do_drag);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_dump);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_edit);
ACMD(do_electrify);
ACMD(do_enter);
ACMD(do_estat);
ACMD(do_exit);
ACMD(do_extinguish);
ACMD(do_eye_gouge);
ACMD(do_flee);
ACMD(do_fly);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_forget);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_gretreat);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_guard);
ACMD(do_hcontrol);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_hitall);
ACMD(do_hotboot);
ACMD(do_house);
ACMD(do_hunt);
ACMD(do_iedit);
ACMD(do_ignore);
ACMD(do_inctime);
ACMD(do_info);
ACMD(do_infodump);
ACMD(do_insult);
ACMD(do_invis);
ACMD(do_iptables);
#ifndef CIRCLE_WINDOWS
ACMD(do_ispell);
#endif
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_kneel);
ACMD(do_ksearch);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_level);
ACMD(do_light);
ACMD(do_linkload);
ACMD(do_load);
ACMD(do_meditate);
ACMD(do_memorize);
ACMD(do_mob_log);
ACMD(do_mount);
ACMD(do_move);
ACMD(do_name);
ACMD(do_note);
ACMD(do_not_here);
ACMD(do_olc);
ACMD(do_order);
ACMD(do_page);
ACMD(do_pain);
ACMD(do_palm);
ACMD(do_peace);
ACMD(do_peck);
ACMD(do_petition);
ACMD(do_pfilemaint);
ACMD(do_point);
ACMD(do_players);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_pray);
ACMD(do_prompt);
ACMD(do_pscan);
ACMD(do_ptell);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quest);
ACMD(do_qadd);
ACMD(do_qdel);
ACMD(do_qlist);
ACMD(do_qstat);
ACMD(do_quit);
ACMD(do_read);
ACMD(do_recline);
ACMD(do_rest);
ACMD(do_reload);
ACMD(do_remove);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_reply);
ACMD(do_restore);
ACMD(do_rrestore);
ACMD(do_return);
ACMD(do_retreat);
ACMD(do_rpain);
ACMD(do_rsdiamimp);
ACMD(do_rclone);
ACMD(do_readlist);
ACMD(do_rename);
ACMD(do_roar);
ACMD(do_save);
ACMD(do_say);
ACMD(do_scribe);
ACMD(do_send);
ACMD(do_set);
ACMD(do_shapechange);
ACMD(do_show);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_mskillset);
ACMD(do_sleep);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_split);
ACMD(do_springleap);
ACMD(do_sdedit);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_stomp);
ACMD(do_stow);
ACMD(do_subclass);
ACMD(do_sweep);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_tame);
ACMD(do_tedit);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_terminate);
ACMD(do_throatcut);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_touch);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_unban);
ACMD(do_unbind);
ACMD(do_use);
ACMD(do_varset);
ACMD(do_varunset);
ACMD(do_visible);
ACMD(do_vsearch);
ACMD(do_csearch);
ACMD(do_esearch);
ACMD(do_msearch);
ACMD(do_osearch);
ACMD(do_rsearch);
ACMD(do_ssearch);
ACMD(do_tsearch);
ACMD(do_zsearch);
ACMD(do_vstat);
ACMD(do_vitem);
ACMD(do_vwear);
ACMD(do_xnames);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_where);
ACMD(do_wield);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zreset);
ACMD(do_zstat);

ACMD(do_game);
ACMD(do_autoboot);
ACMD(do_world);
ACMD(do_objupdate);

/* DG Script ACMD's */
ACMD(do_attach);
ACMD(do_detach);
ACMD(do_tstat);
ACMD(do_masound);
ACMD(do_mkill);
ACMD(do_mjunk);
ACMD(do_mechoaround);
ACMD(do_msend);
ACMD(do_mecho);
ACMD(do_mload);
ACMD(do_mpurge);
ACMD(do_mgoto);
ACMD(do_mat);
ACMD(do_mteleport);
ACMD(do_mforce);
ACMD(do_mexp);
ACMD(do_mdamage);
ACMD(do_mgold);
ACMD(do_m_run_room_trig);
ACMD(do_msave);
ACMD(do_layhand);
ACMD(do_first_aid);
ACMD(do_summon_mount);

int num_of_cmds;
struct sort_struct *cmd_sort_info = NULL;

/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

const struct command_info cmd_info[] = {
    {"RESERVED", 0, 0, 0, 0, 0, 0}, /* this must be first -- for specprocs */
    /* Name      , min position, min stance, ACMD,         min level, sub cmd,
       flags */
    /* directions must come before other commands but after RESERVED */
    {"north", POS_STANDING, STANCE_ALERT, do_move, 0, SCMD_NORTH, CMD_HIDE | CMD_OLC | CMD_NOFIGHT},
    {"east", POS_STANDING, STANCE_ALERT, do_move, 0, SCMD_EAST, CMD_HIDE | CMD_OLC | CMD_NOFIGHT},
    {"south", POS_STANDING, STANCE_ALERT, do_move, 0, SCMD_SOUTH, CMD_HIDE | CMD_OLC | CMD_NOFIGHT},
    {"west", POS_STANDING, STANCE_ALERT, do_move, 0, SCMD_WEST, CMD_HIDE | CMD_OLC | CMD_NOFIGHT},
    {"up", POS_STANDING, STANCE_ALERT, do_move, 0, SCMD_UP, CMD_HIDE | CMD_OLC | CMD_NOFIGHT},
    {"down", POS_STANDING, STANCE_ALERT, do_move, 0, SCMD_DOWN, CMD_HIDE | CMD_OLC | CMD_NOFIGHT},

    /* now, the main list */
    {"at", POS_PRONE, STANCE_DEAD, do_at, LVL_ATTENDANT - 1, 0, CMD_ANY},
    {"abort", POS_PRONE, STANCE_DEAD, do_abort, 0, 0, CMD_ANY ^ CMD_BOUND},
    {"abandon", POS_PRONE, STANCE_RESTING, do_abandon, 0, 0, 0},
    {"ack", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"advance", POS_PRONE, STANCE_DEAD, do_advance, LVL_ADMIN, 0, CMD_ANY},
    {"aggr", POS_PRONE, STANCE_DEAD, do_aggr, 0, 0, 0},
    {"alert", POS_PRONE, STANCE_RESTING, do_alert, 0, 0, CMD_CAST},
    {"alias", POS_PRONE, STANCE_DEAD, do_alias, 0, 0, CMD_MEDITATE | CMD_HIDE},
    {"accuse", POS_SITTING, STANCE_RESTING, do_action, 0, 0, 0},
    {"afk", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"agree", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"amaze", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"anews", POS_PRONE, STANCE_DEAD, do_textview, LVL_IMMORT, SCMD_ANEWS, CMD_ANY},
    {"apologize", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"applaud", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"appear", POS_STANDING, STANCE_ALERT, do_not_here, -1, 0, CMD_NOFIGHT},
    {"assist", POS_STANDING, STANCE_ALERT, do_assist, 1, 0, 0},
    {"ask", POS_PRONE, STANCE_RESTING, do_spec_comm, 0, SCMD_ASK, CMD_OLC},
    {"autoboot", POS_PRONE, STANCE_DEAD, do_autoboot, LVL_REBOOT_VIEW, 0, CMD_ANY},
    /*{ "auction"  , POS_PRONE   , STANCE_SLEEPING, do_gen_comm , LVL_GOD,
       SCMD_AUCTION, 0 },*/
    {"ayt", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},

    {"backstab", POS_STANDING, STANCE_ALERT, do_backstab, 1, 0, 0},
    {"ban", POS_PRONE, STANCE_DEAD, do_ban, LVL_GRGOD, 0, CMD_ANY},
    {"bandage", POS_STANDING, STANCE_ALERT, do_bandage, 1, 0, CMD_NOFIGHT},
    {"balance", POS_STANDING, STANCE_ALERT, do_not_here, 1, 0, CMD_NOFIGHT},
    {"bang", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"bark", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"bash", POS_STANDING, STANCE_ALERT, do_bash, 1, SCMD_BASH, 0},
    /*{ "bind"     , POS_STANDING, STANCE_ALERT   , do_bind     ,-1, 0, 0 },*/
    {"bodyslam", POS_STANDING, STANCE_ALERT, do_bash, 1, SCMD_BODYSLAM, 0},
    {"beckon", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"beer", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"beg", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"berserk", POS_STANDING, STANCE_ALERT, do_berserk, 0, 0, 0},
    {"bite", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"bird", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"blink", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"bleed", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"bless", POS_PRONE, STANCE_DEAD, do_wizutil, LVL_ATTENDANT, SCMD_BLESS, CMD_ANY},
    {"blush", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"boardadmin", POS_PRONE, STANCE_DEAD, do_boardadmin, LVL_ADMIN, 0, 0},
    {"boggle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"bonk", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"bored", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"bounce", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"bow", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},
    {"brb", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"breathe", POS_STANDING, STANCE_ALERT, do_breathe, -1, 0, 0},
    {"buck", POS_STANDING, STANCE_ALERT, do_buck, 0, 0, CMD_NOFIGHT},
    {"burp", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"buy", POS_STANDING, STANCE_ALERT, do_not_here, 0, 0, CMD_NOFIGHT},
    {"bug", POS_PRONE, STANCE_DEAD, do_gen_write, 0, SCMD_BUG, CMD_ANY},
    {"bye", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},

    {"cast", POS_SITTING, STANCE_RESTING, do_cast, 1, SCMD_CAST, 0},
    {"cackle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"call", POS_PRONE, STANCE_RESTING, do_call, 0, 0, 0},
    {"camp", POS_STANDING, STANCE_ALERT, do_camp, 1, 0, CMD_NOFIGHT},
    {"chant", POS_STANDING, STANCE_ALERT, do_cast, 0, SCMD_CHANT, 0},
    {"chuckle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"check", POS_STANDING, STANCE_ALERT, do_not_here, 1, 0, CMD_NOFIGHT},
    {"cheer", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"choke", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"clan", POS_PRONE, STANCE_SLEEPING, do_clan, 1, 0, CMD_MEDITATE},
    {"clap", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"claw", POS_STANDING, STANCE_ALERT, do_claw, 1, 0, 0},
    {"clear", POS_PRONE, STANCE_DEAD, do_gen_ps, 0, SCMD_CLEAR, CMD_ANY},
    {"close", POS_SITTING, STANCE_RESTING, do_gen_door, 0, SCMD_CLOSE, 0},
    {"cls", POS_PRONE, STANCE_DEAD, do_gen_ps, 0, SCMD_CLEAR, CMD_ANY},
    {"consider", POS_PRONE, STANCE_RESTING, do_consider, 0, 0, 0},
    {"color", POS_PRONE, STANCE_DEAD, do_color, 0, 0, CMD_ANY},
    {"compare", POS_PRONE, STANCE_RESTING, do_compare, 0, 0, CMD_MEDITATE},
    {"comfort", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"comb", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"commands", POS_PRONE, STANCE_DEAD, do_commands, 0, SCMD_COMMANDS, CMD_ANY},
    {"consent", POS_PRONE, STANCE_INCAP, do_consent, 0, 0, CMD_MEDITATE | CMD_HIDE | CMD_CAST | CMD_OLC},
    {"conceal", POS_STANDING, STANCE_ALERT, do_conceal, 0, 0, CMD_HIDE | CMD_NOFIGHT},
    {"coredump", POS_PRONE, STANCE_DEAD, do_coredump, LVL_HEAD_C, 0, 0},
    {"corner", POS_STANDING, STANCE_ALERT, do_corner, 0, 0, 0},
    {"cough", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"credits", POS_PRONE, STANCE_DEAD, do_textview, 0, SCMD_CREDITS, CMD_ANY},
    {"cringe", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"cry", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"clist", POS_PRONE, STANCE_DEAD, do_csearch, LVL_ATTENDANT, SCMD_VLIST, CMD_ANY},
    {"csearch", POS_PRONE, STANCE_DEAD, do_csearch, LVL_ATTENDANT, SCMD_VSEARCH, CMD_ANY},
    {"ctell", POS_PRONE, STANCE_SLEEPING, do_ctell, 0, 0, CMD_ANY},
    {"cuddle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"curse", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"curtsey", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},

    {"dance", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},
    {"date", POS_PRONE, STANCE_DEAD, do_date, 0, SCMD_DATE, CMD_ANY},
    {"daydream", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"dc", POS_PRONE, STANCE_DEAD, do_dc, LVL_ATTENDANT, 0, CMD_ANY},
    {"deposit", POS_STANDING, STANCE_ALERT, do_not_here, 1, 0, CMD_NOFIGHT},
    {"desc", POS_PRONE, STANCE_SLEEPING, do_desc, 0, 0, CMD_NOFIGHT},
    {"diagnose", POS_PRONE, STANCE_RESTING, do_diagnose, 0, 0, CMD_MEDITATE | CMD_HIDE | CMD_BOUND | CMD_OLC},
    {"dismount", POS_STANDING, STANCE_ALERT, do_dismount, 0, 0, CMD_NOFIGHT},
    {"display", POS_PRONE, STANCE_DEAD, do_display, 0, 0, CMD_ANY},
    {"disband", POS_PRONE, STANCE_SLEEPING, do_disband, 1, 0, 0},
    {"dig", POS_PRONE, STANCE_DEAD, do_dig, LVL_BUILDER, 0, CMD_ANY ^ CMD_OLC},
    {"disappear", POS_STANDING, STANCE_ALERT, do_not_here, -1, 0, CMD_NOFIGHT},
    {"disarm", POS_STANDING, STANCE_ALERT, do_disarm, 0, 0, 0},
    {"disengage", POS_STANDING, STANCE_ALERT, do_disengage, 0, 0, CMD_CAST},
    {"doorbash", POS_STANDING, STANCE_ALERT, do_doorbash, 0, 0, CMD_NOFIGHT},
    {"douse", POS_STANDING, STANCE_ALERT, do_douse, 0, 0, CMD_NOFIGHT},
    {"drag", POS_STANDING, STANCE_ALERT, do_drag, 1, 0, CMD_NOFIGHT},
    {"dream", POS_PRONE, STANCE_SLEEPING, do_action, 0, 0, 0},
    {"drink", POS_PRONE, STANCE_RESTING, do_drink, 0, SCMD_DRINK, 0},
    {"drop", POS_PRONE, STANCE_RESTING, do_drop, 0, SCMD_DROP, 0},
    {"drool", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"duck", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"duh", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"dump", POS_STANDING, STANCE_ALERT, do_not_here, 1, 0, CMD_NOFIGHT},

    {"eat", POS_PRONE, STANCE_RESTING, do_eat, 0, SCMD_EAT, 0},
    {"edit", POS_SITTING, STANCE_RESTING, do_edit, 3, 0, CMD_NOFIGHT},
    {"echo", POS_PRONE, STANCE_DEAD, do_echo, LVL_IMMORT, SCMD_ECHO, CMD_ANY},
    {"electrify", POS_STANDING, STANCE_ALERT, do_electrify, 1, 0, 0},
    {"emote", POS_PRONE, STANCE_RESTING, do_echo, 1, SCMD_EMOTE, CMD_OLC},
    {"emote's", POS_PRONE, STANCE_RESTING, do_echo, 1, SCMD_EMOTES, CMD_OLC},
    {":", POS_PRONE, STANCE_RESTING, do_echo, 1, SCMD_EMOTE, CMD_OLC},
    {"embrace", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},
    {"enter", POS_STANDING, STANCE_ALERT, do_enter, 0, 0, CMD_NOFIGHT},
    {"envy", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"equipment", POS_PRONE, STANCE_SLEEPING, do_equipment, 0, 0, CMD_ANY},
    {"exits", POS_PRONE, STANCE_RESTING, do_exits, 0, 0, CMD_HIDE | CMD_MEDITATE | CMD_OLC},
    {"examine", POS_PRONE, STANCE_RESTING, do_examine, 0, 0, CMD_HIDE | CMD_OLC},
    {"exchange", POS_STANDING, STANCE_ALERT, do_not_here, 1, 0, CMD_NOFIGHT},
    {"experience", POS_PRONE, STANCE_DEAD, do_experience, 0, 0, CMD_ANY},
    {"extinguish", POS_PRONE, STANCE_RESTING, do_light, 0, SCMD_EXTINGUISH, 0},
    {"eyebrow", POS_PRONE, STANCE_RESTING, do_action, 0, 0, CMD_HIDE},
    {"elist", POS_PRONE, STANCE_DEAD, do_esearch, LVL_ATTENDANT, SCMD_VLIST, CMD_ANY},
    {"enum", POS_PRONE, STANCE_DEAD, do_esearch, LVL_ATTENDANT, SCMD_VNUM, CMD_ANY},
    {"esearch", POS_PRONE, STANCE_DEAD, do_esearch, LVL_ATTENDANT, SCMD_VSEARCH, CMD_ANY},

    {"force", POS_PRONE, STANCE_DEAD, do_force, LVL_ATTENDANT, 0, CMD_ANY},
    {"flee", POS_PRONE, STANCE_RESTING, do_flee, 1, 0, CMD_CAST},
    {"fart", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"first aid", POS_PRONE, STANCE_RESTING, do_first_aid, 0, 0, 0},
    {"fill", POS_STANDING, STANCE_ALERT, do_pour, 0, SCMD_FILL, CMD_NOFIGHT},
    {"flanic", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},
    {"flex", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"flip", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},
    {"flirt", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"fly", POS_STANDING, STANCE_ALERT, do_fly, 0, 0, CMD_HIDE},
    {"follow", POS_PRONE, STANCE_RESTING, do_follow, 0, SCMD_FOLLOW, 0},
    {"fool", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"forget", POS_PRONE, STANCE_RESTING, do_forget, 0, 0, CMD_MEDITATE},
    {"fondle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"freeze", POS_PRONE, STANCE_DEAD, do_wizutil, LVL_FREEZE, SCMD_FREEZE, CMD_ANY},
    {"french", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"frown", POS_PRONE, STANCE_RESTING, do_action, 0, 0, CMD_HIDE},
    {"fume", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},

    {"get", POS_PRONE, STANCE_RESTING, do_get, 0, 0, 0},
    {"gag", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"gape", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"gasp", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"gecho", POS_PRONE, STANCE_DEAD, do_gecho, LVL_GOD, 0, CMD_ANY},
    {"give", POS_PRONE, STANCE_RESTING, do_give, 0, 0, 0},
    {"giggle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"glance", POS_PRONE, STANCE_RESTING, do_diagnose, 0, 0, CMD_MEDITATE | CMD_HIDE | CMD_BOUND | CMD_OLC},
    {"glare", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"glomp", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},
    {"glower", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"goto", POS_PRONE, STANCE_DEAD, do_goto, LVL_IMMORT, 0, CMD_ANY},
    {"go", POS_STANDING, STANCE_ALERT, do_move, 0, 0, CMD_HIDE | CMD_OLC | CMD_NOFIGHT},
    {"gossip", POS_PRONE, STANCE_SLEEPING, do_gen_comm, LVL_GOSSIP, SCMD_GOSSIP,
     CMD_MEDITATE | CMD_CAST | CMD_HIDE | CMD_OLC},
    {"gouge", POS_STANDING, STANCE_ALERT, do_eye_gouge, 1, 0, 0},
    {"groan", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"group", POS_PRONE, STANCE_SLEEPING, do_group, 1, 0, CMD_HIDE | CMD_OLC},
    {"grab", POS_PRONE, STANCE_RESTING, do_grab, 0, 0, 0},
    /*{ "grats"    , POS_PRONE   , STANCE_SLEEPING, do_gen_comm , LVL_GOD,
       SCMD_GRATZ, 0 },*/
    {"greport", POS_PRONE, STANCE_SLEEPING, do_report, 0, SCMD_GREPORT, CMD_ANY},
    {"greet", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"gretreat", POS_STANDING, STANCE_ALERT, do_gretreat, 0, 0, 0},
    {"grin", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"groan", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"grope", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"grovel", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"growl", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"grumble", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"gsay", POS_PRONE, STANCE_SLEEPING, do_gsay, 0, 0, CMD_ANY},
    {"gtell", POS_PRONE, STANCE_SLEEPING, do_gsay, 0, 0, CMD_ANY},
    {"guard", POS_STANDING, STANCE_ALERT, do_guard, 0, 0, CMD_NOFIGHT},
    {"grant", POS_PRONE, STANCE_DEAD, do_grant, LVL_ADMIN, SCMD_GRANT, CMD_ANY},
    {"gedit", POS_PRONE, STANCE_DEAD, do_gedit, LVL_ADMIN, 0, 0},

    {"help", POS_PRONE, STANCE_DEAD, do_help, 0, 0, CMD_ANY},
    {"hedit", POS_PRONE, STANCE_DEAD, do_olc, LVL_GAMEMASTER, SCMD_OLC_HEDIT, 0},
    {"handbook", POS_PRONE, STANCE_DEAD, do_textview, LVL_IMMORT, SCMD_HANDBOOK, CMD_ANY},
    {"halo", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"hcontrol", POS_PRONE, STANCE_DEAD, do_hcontrol, LVL_HEAD_C, 0, CMD_ANY ^ CMD_OLC},
    {"hhroom", POS_PRONE, STANCE_DEAD, do_rclone, LVL_BUILDER, 0, 0},
    {"hi5", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"hiccup", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"hide", POS_STANDING, STANCE_ALERT, do_hide, 1, 0, CMD_HIDE | CMD_NOFIGHT},
    {"hiss", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"hit", POS_STANDING, STANCE_ALERT, do_hit, 0, SCMD_HIT, 0},
    {"hitall", POS_STANDING, STANCE_ALERT, do_hitall, 0, SCMD_HITALL, 0},
    {"hold", POS_PRONE, STANCE_RESTING, do_grab, 1, 0, 0},
    {"hop", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},
    {"hotboot", POS_PRONE, STANCE_DEAD, do_hotboot, LVL_REBOOT_MASTER, 0, 0},
    {"house", POS_PRONE, STANCE_RESTING, do_house, -1, 0, 0},
    {"howl", POS_STANDING, STANCE_ALERT, do_roar, 0, SCMD_HOWL, 0},
    {"hunt", POS_STANDING, STANCE_ALERT, do_hunt, -1, 0, CMD_NOFIGHT},
    {"hug", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"hunger", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},

    {"inventory", POS_PRONE, STANCE_SLEEPING, do_inventory, 0, 0, CMD_ANY},
    {"identify", POS_PRONE, STANCE_RESTING, do_identify, 0, 0, CMD_HIDE | CMD_OLC},
    {"idea", POS_PRONE, STANCE_DEAD, do_gen_write, 0, SCMD_IDEA, CMD_ANY},
    {"iedit", POS_PRONE, STANCE_DEAD, do_iedit, LVL_BUILDER, 0, 0},
    {"imitate", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"imotd", POS_PRONE, STANCE_DEAD, do_textview, LVL_IMMORT, SCMD_IMOTD, CMD_ANY},
    {"impale", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"innate", POS_PRONE, STANCE_DEAD, do_innate, 0, 0, CMD_ANY},
    {"infodump", POS_PRONE, STANCE_DEAD, do_infodump, LVL_HEAD_C, 0, CMD_ANY},
    {"ignore", POS_PRONE, STANCE_DEAD, do_ignore, 0, 0, CMD_ANY},
    {"inctime", POS_PRONE, STANCE_DEAD, do_inctime, LVL_HEAD_C, 0, CMD_ANY},
    {"hour", POS_PRONE, STANCE_DEAD, do_inctime, LVL_HEAD_C, 0, CMD_ANY},
    {"info", POS_PRONE, STANCE_DEAD, do_textview, 0, SCMD_INFO, CMD_ANY},
    {"insult", POS_PRONE, STANCE_RESTING, do_insult, 0, 0, 0},
    {"invis", POS_PRONE, STANCE_DEAD, do_invis, LVL_IMMORT, 0, CMD_ANY},
    {"iptables", POS_PRONE, STANCE_DEAD, do_iptables, LVL_HEAD_C, 0, CMD_ANY},
#ifndef CIRCLE_WINDOWS
    {"ispell", POS_PRONE, STANCE_DEAD, do_ispell, LVL_IMMORT, 0, CMD_ANY},
#endif

    {"junk", POS_PRONE, STANCE_RESTING, do_drop, 0, SCMD_JUNK, 0},

    {"kick", POS_STANDING, STANCE_ALERT, do_kick, 1, 0, 0},
    {"kill", POS_STANDING, STANCE_ALERT, do_kill, 0, 0, 0},
    {"kiss", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"kneel", POS_PRONE, STANCE_RESTING, do_kneel, 0, 0, 0},
    {"ksearch", POS_PRONE, STANCE_DEAD, do_ksearch, LVL_ATTENDANT, SCMD_VSEARCH, CMD_ANY},

    {"look", POS_PRONE, STANCE_RESTING, do_look, 0, 0,
     CMD_MINOR_PARA | CMD_MEDITATE | CMD_HIDE | CMD_BOUND | CMD_CAST | CMD_OLC},
    {"lag", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"laugh", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"layhands", POS_STANDING, STANCE_ALERT, do_layhand, 0, 0, 0},
    {"last", POS_PRONE, STANCE_DEAD, do_last, LVL_GRGOD, 0, CMD_ANY},
    {"lasttells", POS_PRONE, STANCE_DEAD, do_last_tells, 0, 0, CMD_ANY},
    {"lastgos", POS_PRONE, STANCE_DEAD, do_last_gossips, 0, 0, CMD_ANY},
    {"lean", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"leave", POS_STANDING, STANCE_ALERT, do_leave, 0, 0, CMD_NOFIGHT},
    {"level", POS_PRONE, STANCE_DEAD, do_level, 0, 0, CMD_ANY},
    {"light", POS_PRONE, STANCE_RESTING, do_light, 0, SCMD_LIGHT, 0},
    {"list", POS_STANDING, STANCE_ALERT, do_not_here, 0, 0, CMD_NOFIGHT},
    {"listspells", POS_PRONE, STANCE_DEAD, do_listspells, LVL_ATTENDANT, 0, CMD_ANY},
    {"lick", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"lock", POS_SITTING, STANCE_RESTING, do_gen_door, 0, SCMD_LOCK, 0},
    {"linkload", POS_PRONE, STANCE_DEAD, do_linkload, LVL_HEAD_C, 0, CMD_ANY ^ CMD_OLC},
    {"load", POS_PRONE, STANCE_DEAD, do_load, LVL_ATTENDANT, 0, CMD_ANY ^ CMD_OLC},
    {"love", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},

    {"memorize", POS_PRONE, STANCE_RESTING, do_memorize, 0, 0, CMD_MEDITATE},
    {"maul", POS_STANDING, STANCE_ALERT, do_bash, 1, SCMD_MAUL, 0},
    {"moan", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"medit", POS_PRONE, STANCE_DEAD, do_olc, LVL_BUILDER, SCMD_OLC_MEDIT, 0},
    {"motd", POS_PRONE, STANCE_DEAD, do_textview, 0, SCMD_MOTD, CMD_ANY},
    {"mail", POS_STANDING, STANCE_ALERT, do_not_here, 1, 0, CMD_HIDE | CMD_NOFIGHT},
    {"massage", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"meditate", POS_PRONE, STANCE_RESTING, do_meditate, 0, 0, CMD_MEDITATE},
    {"moon", POS_STANDING, STANCE_ALERT, do_action, 0, 0, 0},
    {"mosh", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},
    {"mount", POS_STANDING, STANCE_ALERT, do_mount, 0, 0, CMD_NOFIGHT},
    {"mourn", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"mumble", POS_PRONE, STANCE_SLEEPING, do_action, 0, 0, 0},
    {"mute", POS_PRONE, STANCE_DEAD, do_wizutil, LVL_GOD, SCMD_SQUELCH, CMD_ANY},
    {"mutter", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"murder", POS_STANDING, STANCE_ALERT, do_hit, 0, SCMD_MURDER, 0},
    {"mlist", POS_PRONE, STANCE_DEAD, do_msearch, LVL_ATTENDANT, SCMD_VLIST, CMD_ANY},
    {"mnum", POS_PRONE, STANCE_DEAD, do_msearch, LVL_ATTENDANT, SCMD_VNUM, CMD_ANY},
    {"msearch", POS_PRONE, STANCE_DEAD, do_msearch, LVL_ATTENDANT, SCMD_VSEARCH, CMD_ANY},
    {"mstat", POS_PRONE, STANCE_DEAD, do_vstat, LVL_ATTENDANT, SCMD_MSTAT, CMD_ANY},

    {"nap", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"news", POS_PRONE, STANCE_DEAD, do_textview, 0, SCMD_NEWS, CMD_ANY},
    {"nibble", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"nod", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"nog", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"noogie", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"notitle", POS_PRONE, STANCE_DEAD, do_wizutil, LVL_GRGOD, SCMD_NOTITLE, CMD_ANY},
    {"note", POS_PRONE, STANCE_DEAD, do_gen_write, LVL_IMMORT, SCMD_NOTE, CMD_ANY},
    {"nudge", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"nuzzle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"naccept", POS_PRONE, STANCE_DEAD, do_name, LVL_IMMORT, SCMD_ACCEPT, CMD_ANY},
    {"ndecline", POS_PRONE, STANCE_DEAD, do_name, LVL_IMMORT, SCMD_DECLINE, CMD_ANY},
    {"nlist", POS_PRONE, STANCE_DEAD, do_name, LVL_IMMORT, SCMD_LIST, CMD_ANY},

    {"order", POS_PRONE, STANCE_RESTING, do_order, 1, 0, 0},
    {"open", POS_SITTING, STANCE_RESTING, do_gen_door, 0, SCMD_OPEN, 0},
    {"olc", POS_PRONE, STANCE_DEAD, do_olc, LVL_BUILDER, SCMD_OLC_SAVEINFO, CMD_ANY},
    {"oedit", POS_PRONE, STANCE_DEAD, do_olc, LVL_BUILDER, SCMD_OLC_OEDIT, 0},
    {"olist", POS_PRONE, STANCE_DEAD, do_osearch, LVL_ATTENDANT, SCMD_VLIST, CMD_ANY},
    {"onum", POS_PRONE, STANCE_DEAD, do_osearch, LVL_ATTENDANT, SCMD_VNUM, CMD_ANY},
    {"osearch", POS_PRONE, STANCE_DEAD, do_osearch, LVL_ATTENDANT, SCMD_VSEARCH, CMD_ANY},
    {"ostat", POS_PRONE, STANCE_DEAD, do_vstat, LVL_ATTENDANT, SCMD_OSTAT, CMD_ANY},

    {"put", POS_PRONE, STANCE_RESTING, do_put, 0, 0, 0},
    {"palm", POS_PRONE, STANCE_RESTING, do_palm, 0, 0, CMD_HIDE},
    {"panic", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},
    {"pant", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"pat", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"page", POS_PRONE, STANCE_DEAD, do_page, LVL_GOD, 0, CMD_ANY},
    {"pardon", POS_PRONE, STANCE_DEAD, do_wizutil, LVL_OVERLORD, SCMD_PARDON, CMD_ANY},
    {"peace", POS_PRONE, STANCE_DEAD, do_peace, LVL_GRGOD, 0, CMD_ANY},
    {"peck", POS_STANDING, STANCE_ALERT, do_peck, 1, 0, 0},
    {"peer", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"pet", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"petition", POS_PRONE, STANCE_DEAD, do_petition, 0, 0, CMD_ANY},
    {"pfilemaint", POS_PRONE, STANCE_DEAD, do_pfilemaint, LVL_OVERLORD, 0, 0},
    {"pick", POS_STANDING, STANCE_ALERT, do_gen_door, 1, SCMD_PICK, CMD_HIDE | CMD_NOFIGHT},
    {"players", POS_PRONE, STANCE_DEAD, do_players, LVL_HEAD_C, 0, CMD_ANY},
    {"point", POS_PRONE, STANCE_RESTING, do_point, 0, 0, 0},
    {"poke", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"policy", POS_PRONE, STANCE_DEAD, do_textview, 0, SCMD_POLICIES, CMD_ANY},
    {"ponder", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"poofin", POS_PRONE, STANCE_DEAD, do_poofset, LVL_IMMORT, SCMD_POOFIN, CMD_ANY},
    {"poofout", POS_PRONE, STANCE_DEAD, do_poofset, LVL_IMMORT, SCMD_POOFOUT, CMD_ANY},
    {"pounce", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"pour", POS_STANDING, STANCE_ALERT, do_pour, 0, SCMD_POUR, CMD_NOFIGHT},
    {"pout", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"pray", POS_PRONE, STANCE_RESTING, do_pray, 0, 0, CMD_MEDITATE},
    {"prompt", POS_PRONE, STANCE_DEAD, do_prompt, 0, 0, CMD_ANY},
    {"protect", POS_STANDING, STANCE_ALERT, do_action, 0, 0, 0},
    {"pscan", POS_PRONE, STANCE_DEAD, do_pscan, LVL_HEAD_C, 0, CMD_ANY},
    {"ptell", POS_PRONE, STANCE_DEAD, do_ptell, LVL_IMMORT, 0, CMD_ANY},
    {"puke", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"punch", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"purr", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"purge", POS_PRONE, STANCE_DEAD, do_purge, LVL_PURGE, 0, CMD_ANY},

    {"quaff", POS_PRONE, STANCE_RESTING, do_use, 0, SCMD_QUAFF, 0},
    {"qecho", POS_PRONE, STANCE_DEAD, do_qcomm, LVL_IMMORT, SCMD_QECHO, CMD_ANY},
    {"qui", POS_PRONE, STANCE_DEAD, do_quit, -1, 0, CMD_ANY ^ (CMD_CAST | CMD_OLC)},
    {"quit", POS_PRONE, STANCE_DEAD, do_quit, 0, SCMD_QUIT, CMD_ANY ^ (CMD_CAST | CMD_OLC)},
    {"qsay", POS_PRONE, STANCE_RESTING, do_qcomm, 0, SCMD_QSAY, CMD_ANY},

    {"rest", POS_PRONE, STANCE_RESTING, do_rest, 0, 0, CMD_MEDITATE},
    {"raise", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"read", POS_PRONE, STANCE_RESTING, do_read, 0, 0, 0},
    {"report", POS_PRONE, STANCE_RESTING, do_report, 0, SCMD_REPORT, 0},
    {"reply", POS_PRONE, STANCE_SLEEPING, do_reply, 0, 0, CMD_ANY},
    {"reload", POS_PRONE, STANCE_DEAD, do_reload, LVL_HEAD_C, 0, 0},
    {"recite", POS_PRONE, STANCE_RESTING, do_use, 0, SCMD_RECITE, 0},
    {"receive", POS_STANDING, STANCE_ALERT, do_not_here, 1, 0, CMD_NOFIGHT},
    {"recline", POS_PRONE, STANCE_RESTING, do_recline, 0, 0, 0},
    {"remove", POS_PRONE, STANCE_RESTING, do_remove, 0, 0, 0},
    {"rent", POS_STANDING, STANCE_ALERT, do_not_here, 1, 0, CMD_NOFIGHT},
    {"reroll", POS_PRONE, STANCE_DEAD, do_wizutil, LVL_GRGOD, SCMD_REROLL, 0},
    {"rescue", POS_STANDING, STANCE_ALERT, do_rescue, 0, 0, 0},
    /*{ "readlist" , POS_PRONE   , STANCE_DEAD    , do_readlist , LVL_GOD, 0, 0
       },*/
    {"restore", POS_PRONE, STANCE_DEAD, do_restore, LVL_RESTORE, 0, CMD_OLC},
    {"rrestore", POS_PRONE, STANCE_DEAD, do_rrestore, LVL_IMMORT, 0, CMD_OLC},
    {"pain", POS_PRONE, STANCE_DEAD, do_pain, LVL_RESTORE, 0, CMD_OLC},
    {"rpain", POS_PRONE, STANCE_DEAD, do_rpain, LVL_RESTORE, 0, CMD_OLC},
    {"retreat", POS_STANDING, STANCE_ALERT, do_retreat, 0, 0, 0},
    {"return", POS_PRONE, STANCE_DEAD, do_return, -1, 0, CMD_MINOR_PARA | CMD_MAJOR_PARA | CMD_BOUND},
    {"redit", POS_PRONE, STANCE_DEAD, do_olc, LVL_BUILDER, SCMD_OLC_REDIT, 0},
    {"rename", POS_PRONE, STANCE_DEAD, do_rename, LVL_GRGOD, 0, 0},
    {"revoke", POS_PRONE, STANCE_DEAD, do_grant, LVL_ADMIN, SCMD_REVOKE, CMD_ANY},
    {"roar", POS_STANDING, STANCE_ALERT, do_roar, 0, SCMD_ROAR, 0},
    {"rofl", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"roll", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"ready", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"ruffle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"rlist", POS_PRONE, STANCE_DEAD, do_rsearch, LVL_ATTENDANT, SCMD_VLIST, CMD_ANY},
    {"rnum", POS_PRONE, STANCE_DEAD, do_rsearch, LVL_ATTENDANT, SCMD_VNUM, CMD_ANY},
    {"rsearch", POS_PRONE, STANCE_DEAD, do_rsearch, LVL_ATTENDANT, SCMD_VSEARCH, CMD_ANY},
    {"rstat", POS_PRONE, STANCE_DEAD, do_stat, LVL_ATTENDANT, SCMD_RSTAT, CMD_ANY},
    {"sstat", POS_PRONE, STANCE_DEAD, do_stat, LVL_ATTENDANT, SCMD_SSTAT, CMD_ANY},
    {"rsdiamimp", POS_PRONE, STANCE_DEAD, do_rsdiamimp, -1, 0, 0},

    {"say", POS_PRONE, STANCE_RESTING, do_say, 0, 0, CMD_MINOR_PARA | CMD_BOUND | CMD_OLC},
    {"'", POS_PRONE, STANCE_RESTING, do_say, 0, 0, CMD_MINOR_PARA | CMD_BOUND | CMD_OLC},
    {"save", POS_PRONE, STANCE_SLEEPING, do_save, LVL_GOD, 0, CMD_ANY ^ CMD_CAST},
    {"score", POS_PRONE, STANCE_DEAD, do_score, 0, 0, CMD_ANY},
    {"scan", POS_STANDING, STANCE_ALERT, do_scan, 0, 0, CMD_NOFIGHT},
    {"salute", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"scribe", POS_PRONE, STANCE_RESTING, do_scribe, 0, 0, 0},
    {"scare", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"scold", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"scratch", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"scream", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"screw", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"sdedit", POS_PRONE, STANCE_DEAD, do_olc, LVL_HEAD_C, SCMD_OLC_SDEDIT, 0},
    {"sell", POS_STANDING, STANCE_ALERT, do_not_here, 0, 0, CMD_NOFIGHT},
    {"send", POS_PRONE, STANCE_DEAD, do_send, LVL_GRGOD, 0, CMD_ANY},
    {"set", POS_PRONE, STANCE_DEAD, do_set, LVL_GOD, 0, CMD_ANY},
    {"search", POS_STANDING, STANCE_ALERT, do_search, 0, 0, CMD_NOFIGHT},
    {"sedit", POS_PRONE, STANCE_DEAD, do_olc, LVL_BUILDER, SCMD_OLC_SEDIT, 0},
    {"seduce", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"shout", POS_PRONE, STANCE_RESTING, do_gen_comm, 0, SCMD_SHOUT, CMD_OLC},
    {"shake", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"shadow", POS_PRONE, STANCE_RESTING, do_follow, 0, SCMD_SHADOW, CMD_HIDE},
    {"shapechange", POS_STANDING, STANCE_ALERT, do_shapechange, 0, 0, CMD_NOFIGHT},
    {"shiver", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"show", POS_PRONE, STANCE_DEAD, do_show, LVL_IMMORT, 0, CMD_ANY},
    {"shrug", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"shudder", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"shutdow", POS_PRONE, STANCE_DEAD, do_shutdown, LVL_REBOOT_MASTER, 0, 0},
    {"shutdown", POS_PRONE, STANCE_DEAD, do_shutdown, LVL_REBOOT_MASTER, SCMD_SHUTDOWN, 0},
    {"sigh", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"sing", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"sip", POS_PRONE, STANCE_RESTING, do_drink, 0, SCMD_SIP, 0},
    {"sit", POS_PRONE, STANCE_RESTING, do_sit, 0, 0, 0},
    {"skills", POS_PRONE, STANCE_SLEEPING, do_skills, 1, 0, CMD_ANY},
    {"skillset", POS_PRONE, STANCE_DEAD, do_skillset, LVL_GAMEMASTER, 0, CMD_ANY},
    {"slist", POS_PRONE, STANCE_DEAD, do_ssearch, LVL_ATTENDANT, SCMD_VLIST, CMD_ANY},
    {"snum", POS_PRONE, STANCE_DEAD, do_ssearch, LVL_ATTENDANT, SCMD_VNUM, CMD_ANY},
    {"ssearch", POS_PRONE, STANCE_DEAD, do_ssearch, LVL_ATTENDANT, SCMD_VSEARCH, CMD_ANY},
    {"sleep", POS_PRONE, STANCE_SLEEPING, do_sleep, 0, 0, 0},
    {"slap", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"slobber", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"smell", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"smile", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"smirk", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"smoke", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"snicker", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"snap", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"snarl", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"sneeze", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"sniff", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"snoogie", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"snore", POS_PRONE, STANCE_SLEEPING, do_action, 0, 0, 0},
    {"snort", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"snowball", POS_STANDING, STANCE_ALERT, do_action, LVL_OVERLORD, 0, CMD_NOFIGHT},
    {"snoop", POS_PRONE, STANCE_DEAD, do_snoop, LVL_HEAD_B, 0, 0},
    {"snuggle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"songs", POS_PRONE, STANCE_DEAD, do_songs, 0, 0, CMD_ANY},
    {"socials", POS_PRONE, STANCE_DEAD, do_commands, 0, SCMD_SOCIALS, CMD_ANY},
    {"spam", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"split", POS_PRONE, STANCE_RESTING, do_split, 1, 0, 0},
    {"spells", POS_PRONE, STANCE_DEAD, do_spells, 1, 0, CMD_ANY},
    {"spank", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"spit", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"springleap", POS_PRONE, STANCE_RESTING, do_springleap, 0, 0, 0},
    {"squeeze", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"stand", POS_PRONE, STANCE_RESTING, do_stand, 0, 0, 0},
    {"stare", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"stat", POS_PRONE, STANCE_DEAD, do_stat, LVL_ATTENDANT, SCMD_STAT, CMD_ANY},
    {"stay", POS_PRONE, STANCE_RESTING, do_move, 0, SCMD_STAY, CMD_HIDE | CMD_OLC},
    {"steal", POS_STANDING, STANCE_ALERT, do_steal, 1, 0, CMD_HIDE | CMD_NOFIGHT},
    {"steam", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"stow", POS_PRONE, STANCE_RESTING, do_stow, 0, 0, CMD_HIDE},
    {"stomp", POS_STANDING, STANCE_ALERT, do_stomp, 0, 0, 0},
    {"stroke", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"strut", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},
    {"sulk", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"summon", POS_STANDING, STANCE_ALERT, do_summon_mount, 0, 0, CMD_NOFIGHT},
    {"swat", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"sweat", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"sweep", POS_STANDING, STANCE_ALERT, do_sweep, -1, 0, 0},
    {"switch", POS_PRONE, STANCE_DEAD, do_switch, LVL_GOD, 0, CMD_ANY ^ CMD_OLC},
    {"syslog", POS_PRONE, STANCE_DEAD, do_syslog, LVL_IMMORT, 0, CMD_ANY},
    {"stone", POS_STANDING, STANCE_ALERT, do_not_here, -1, 0, CMD_NOFIGHT},
    {"subclass", POS_PRONE, STANCE_RESTING, do_subclass, 0, 0, CMD_HIDE},

    {"tell", POS_PRONE, STANCE_SLEEPING, do_tell, 0, 0, CMD_ANY},
    {"terminate", POS_PRONE, STANCE_DEAD, do_terminate, LVL_HEAD_C, 0, 0},
    {"tackle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"take", POS_STANDING, STANCE_ALERT, do_get, 0, 0, 0},
    {"tantrum", POS_STANDING, STANCE_ALERT, do_hitall, 0, SCMD_TANTRUM, 0},
    {"tango", POS_STANDING, STANCE_ALERT, do_action, 0, 0, CMD_NOFIGHT},
    {"tame", POS_STANDING, STANCE_ALERT, do_tame, 0, 0, CMD_NOFIGHT},
    {"tap", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"tarzan", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"taunt", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"taste", POS_PRONE, STANCE_RESTING, do_eat, 0, SCMD_TASTE, 0},
    {"tease", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"tedit", POS_PRONE, STANCE_DEAD, do_tedit, LVL_HEAD_C, 0, 0},
    {"teleport", POS_PRONE, STANCE_DEAD, do_teleport, LVL_GOD, 0, CMD_ANY},
    {"thank", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"think", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"thaw", POS_PRONE, STANCE_DEAD, do_wizutil, LVL_FREEZE, SCMD_THAW, CMD_ANY},
    {"thirst", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"throatcut", POS_STANDING, STANCE_ALERT, do_throatcut, 0, 0, CMD_NOFIGHT},
    {"throw", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"tip", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"title", POS_PRONE, STANCE_DEAD, do_title, 1, 0, CMD_ANY},
    {"tickle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"time", POS_PRONE, STANCE_DEAD, do_time, 0, 0, CMD_ANY},
    {"tip", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"toggle", POS_PRONE, STANCE_DEAD, do_toggle, 0, 0, CMD_ANY ^ CMD_CAST},
    {"tongue", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"touch", POS_PRONE, STANCE_RESTING, do_touch, 0, 0, 0},
    {"track", POS_STANDING, STANCE_ALERT, do_track, 0, 0, CMD_NOFIGHT},
    {"transfer", POS_PRONE, STANCE_DEAD, do_trans, LVL_GOD, 0, CMD_ANY},
    {"trigedit", POS_PRONE, STANCE_DEAD, do_olc, LVL_BUILDER, SCMD_OLC_TRIGEDIT, 0},
    {"trip", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"trophy", POS_PRONE, STANCE_DEAD, do_trophy, 0, 0, CMD_ANY},
    {"tug", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"twibble", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"twiddle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"twitch", POS_PRONE, STANCE_SLEEPING, do_action, 0, 0, 0},
    {"typo", POS_PRONE, STANCE_DEAD, do_gen_write, 0, SCMD_TYPO, CMD_ANY},

    {"unlock", POS_SITTING, STANCE_RESTING, do_gen_door, 0, SCMD_UNLOCK, 0},
    {"unban", POS_PRONE, STANCE_DEAD, do_unban, LVL_GRGOD, 0, CMD_ANY},
    /*{ "unbind"   , POS_PRONE   , STANCE_DEAD    , do_unbind   ,-1, 0, CMD_HIDE
       },*/
    {"ungrant", POS_PRONE, STANCE_DEAD, do_grant, LVL_ADMIN, SCMD_UNGRANT, CMD_ANY},
    {"use", POS_SITTING, STANCE_RESTING, do_use, 1, SCMD_USE, 0},
    {"unaffect", POS_PRONE, STANCE_DEAD, do_wizutil, LVL_ATTENDANT, SCMD_UNAFFECT, CMD_ANY},
    {"users", POS_PRONE, STANCE_DEAD, do_users, LVL_ATTENDANT, 0, CMD_ANY},
    {"uptime", POS_PRONE, STANCE_DEAD, do_date, 0, SCMD_UPTIME, CMD_ANY},

    {"value", POS_STANDING, STANCE_ALERT, do_not_here, 0, 0, CMD_HIDE | CMD_NOFIGHT},
    {"varset", POS_PRONE, STANCE_DEAD, do_varset, LVL_GAMEMASTER, 0, CMD_ANY},
    {"varunset", POS_PRONE, STANCE_DEAD, do_varunset, LVL_GAMEMASTER, 0, CMD_ANY},
    {"version", POS_PRONE, STANCE_DEAD, do_gen_ps, 0, SCMD_VERSION, CMD_ANY},
    {"veto", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"visible", POS_PRONE, STANCE_RESTING, do_visible, 1, 0, CMD_HIDE},
    {"viewdam", POS_PRONE, STANCE_DEAD, do_viewdam, LVL_GRGOD, 0, CMD_ANY},
    {"vnum", POS_PRONE, STANCE_DEAD, do_vsearch, LVL_ATTENDANT, SCMD_VNUM, CMD_ANY},
    {"vlist", POS_PRONE, STANCE_DEAD, do_vsearch, LVL_ATTENDANT, SCMD_VLIST, CMD_ANY},
    {"vsearch", POS_PRONE, STANCE_DEAD, do_vsearch, LVL_ATTENDANT, SCMD_VSEARCH, CMD_ANY},
    {"vstat", POS_PRONE, STANCE_DEAD, do_vstat, LVL_ATTENDANT, SCMD_VSTAT, CMD_ANY},
    {"zstat", POS_PRONE, STANCE_DEAD, do_zstat, LVL_ATTENDANT, 0, CMD_ANY},
    {"estat", POS_PRONE, STANCE_DEAD, do_estat, LVL_ATTENDANT, 0, CMD_ANY},
    {"oestat", POS_PRONE, STANCE_DEAD, do_estat, LVL_ATTENDANT, SCMD_OESTAT, CMD_ANY},
    {"restat", POS_PRONE, STANCE_DEAD, do_estat, LVL_ATTENDANT, SCMD_RESTAT, CMD_ANY},
    {"vitem", POS_PRONE, STANCE_DEAD, do_vitem, LVL_ATTENDANT, 0, CMD_ANY},
    {"vwear", POS_PRONE, STANCE_DEAD, do_vwear, LVL_ATTENDANT, 0, CMD_ANY},

    {"wake", POS_PRONE, STANCE_SLEEPING, do_wake, 0, 0, 0},
    {"walk", POS_STANDING, STANCE_ALERT, do_move, 0, 0, CMD_HIDE | CMD_NOFIGHT},
    {"wave", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"wear", POS_PRONE, STANCE_RESTING, do_wear, 0, 0, 0},
    {"wait", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"weather", POS_PRONE, STANCE_RESTING, do_weather, 0, 0, CMD_ANY},
    {"wet", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"who", POS_PRONE, STANCE_DEAD, do_who, 0, 0, CMD_ANY},
    {"whap", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"whatever", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"whoami", POS_PRONE, STANCE_DEAD, do_gen_ps, 0, SCMD_WHOAMI, CMD_ANY},
    {"where", POS_PRONE, STANCE_DEAD, do_where, LVL_ATTENDANT, 0, CMD_ANY},
    {"whisper", POS_PRONE, STANCE_RESTING, do_spec_comm, 0, SCMD_WHISPER, CMD_OLC},
    {"whine", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"whistle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"wield", POS_PRONE, STANCE_RESTING, do_wield, 0, 0, 0},
    {"wiggle", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"wince", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"wink", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"withdraw", POS_STANDING, STANCE_ALERT, do_not_here, 1, 0, CMD_NOFIGHT},
    {"wiznet", POS_PRONE, STANCE_DEAD, do_wiznet, LVL_IMMORT, 0, CMD_ANY},
    {";", POS_PRONE, STANCE_DEAD, do_wiznet, LVL_IMMORT, 0, CMD_ANY},
    {"wizhelp", POS_PRONE, STANCE_DEAD, do_commands, LVL_IMMORT, SCMD_WIZHELP, CMD_ANY},
    {"wizlist", POS_PRONE, STANCE_DEAD, do_textview, 0, SCMD_WIZLIST, CMD_ANY},
    {"wizlock", POS_PRONE, STANCE_DEAD, do_wizlock, LVL_HEAD_B, 0, 0},
    {"worship", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"write", POS_STANDING, STANCE_ALERT, do_write, 3, 0, CMD_NOFIGHT},

    {"xnames", POS_PRONE, STANCE_DEAD, do_xnames, LVL_GRGOD, 0, CMD_ANY},

    {"yawn", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"yodel", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},

    {"zone", POS_PRONE, STANCE_RESTING, do_action, 0, 0, 0},
    {"zedit", POS_PRONE, STANCE_DEAD, do_olc, LVL_BUILDER, SCMD_OLC_ZEDIT, 0},
    {"zlist", POS_PRONE, STANCE_DEAD, do_zsearch, LVL_ATTENDANT, SCMD_VLIST, CMD_ANY},
    {"znum", POS_PRONE, STANCE_DEAD, do_zsearch, LVL_ATTENDANT, SCMD_VNUM, CMD_ANY},
    {"zreset", POS_PRONE, STANCE_DEAD, do_zreset, LVL_ATTENDANT, 0, CMD_ANY},
    {"zsearch", POS_PRONE, STANCE_DEAD, do_zsearch, LVL_ATTENDANT, SCMD_VSEARCH, CMD_ANY},

    {"game", POS_PRONE, STANCE_DEAD, do_game, LVL_ATTENDANT, 0, CMD_ANY},
    {"world", POS_PRONE, STANCE_DEAD, do_world, 0, 0, CMD_ANY},

    /* DG trigger commands */
    {"attach", POS_PRONE, STANCE_DEAD, do_attach, LVL_IMPL, 0, 0},
    {"z001#@#", POS_PRONE, STANCE_SLEEPING, do_action, -1, 0, 0},
    {"detach", POS_PRONE, STANCE_DEAD, do_detach, LVL_IMPL, 0, 0},
    {"tlist", POS_PRONE, STANCE_DEAD, do_tsearch, LVL_ATTENDANT, SCMD_VLIST, CMD_ANY},
    {"tnum", POS_PRONE, STANCE_DEAD, do_tsearch, LVL_ATTENDANT, SCMD_VNUM, CMD_ANY},
    {"tsearch", POS_PRONE, STANCE_DEAD, do_tsearch, LVL_ATTENDANT, SCMD_VSEARCH, CMD_ANY},
    {"tstat", POS_PRONE, STANCE_DEAD, do_tstat, LVL_ATTENDANT, 0, CMD_ANY},
    {"log", POS_PRONE, STANCE_DEAD, do_mob_log, -1, 0, CMD_ANY},
    {"m_run_room_trig", POS_PRONE, STANCE_DEAD, do_m_run_room_trig, -1, 0, CMD_ANY},
    {"masound", POS_PRONE, STANCE_DEAD, do_masound, -1, 0, CMD_ANY},
    {"mat", POS_PRONE, STANCE_DEAD, do_mat, -1, 0, CMD_ANY},
    {"mdamage", POS_PRONE, STANCE_DEAD, do_mdamage, -1, 0, CMD_HIDE},
    {"mecho", POS_PRONE, STANCE_DEAD, do_mecho, -1, 0, CMD_ANY},
    {"mechoaround", POS_PRONE, STANCE_DEAD, do_mechoaround, -1, 0, CMD_ANY},
    {"mexp", POS_PRONE, STANCE_DEAD, do_mexp, -1, 0, CMD_ANY},
    {"mforce", POS_PRONE, STANCE_DEAD, do_mforce, -1, 0, CMD_ANY},
    {"mgoto", POS_PRONE, STANCE_DEAD, do_mgoto, -1, 0, CMD_HIDE},
    {"mjunk", POS_PRONE, STANCE_DEAD, do_mjunk, -1, 0, CMD_ANY},
    {"mkill", POS_STANDING, STANCE_ALERT, do_mkill, -1, 0, CMD_NOFIGHT},
    {"mload", POS_PRONE, STANCE_DEAD, do_mload, -1, 0, CMD_ANY},
    {"mpurge", POS_PRONE, STANCE_DEAD, do_mpurge, -1, 0, CMD_ANY},
    {"msave", POS_PRONE, STANCE_DEAD, do_msave, -1, 0, CMD_ANY},
    {"msend", POS_PRONE, STANCE_DEAD, do_msend, -1, 0, CMD_ANY},
    {"mskillset", POS_PRONE, STANCE_DEAD, do_mskillset, -1, 0, CMD_ANY},
    {"mteleport", POS_PRONE, STANCE_DEAD, do_mteleport, -1, 0, CMD_ANY},
    {"quest", POS_PRONE, STANCE_DEAD, do_quest, -1, 0, CMD_ANY},
    {"qadd", POS_PRONE, STANCE_DEAD, do_qadd, LVL_HEAD_B, 0, CMD_ANY},
    {"qdel", POS_PRONE, STANCE_DEAD, do_qdel, LVL_HEAD_B, 0, CMD_ANY},
    {"qlist", POS_PRONE, STANCE_DEAD, do_qlist, LVL_ATTENDANT, 0, CMD_ANY},
    {"qstat", POS_PRONE, STANCE_DEAD, do_qstat, LVL_ATTENDANT, 0, CMD_ANY},
    {"objupdate", POS_PRONE, STANCE_DEAD, do_objupdate, LVL_HEAD_C, 0, CMD_ANY},

    {"\n", 0, 0, 0, 0, 0, CMD_HIDE}}; /* this must be last */

const char *command_flags[] = {"MEDITATE", "MAJOR PARA", "MINOR PARA", "HIDE", "BOUND", "CAST", "OLC", "NOFIGHT", "\n"};

const char *fill[] = {"in", "from", "with", "the", "on", "at", "to", "\n"};

const char *reserved[] = {"self", "me", "all", "room", "someone", "something", "\n"};

void list_similar_commands(struct char_data *ch, char *arg) {
    int found = FALSE, cmd;

    if (!PRF_FLAGGED(ch, PRF_NOHINTS)) {
        /* Display similar commands. */
        for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd) {
            if (*arg != *cmd_info[cmd].command)
                continue;
            if (!can_use_command(ch, cmd))
                continue;
            if (cmd_info[cmd].minimum_level < 0)
                continue;
            /* skip socials */
            if (cmd_info[cmd].command_pointer == do_action)
                continue;
            if (levenshtein_distance(arg, cmd_info[cmd].command) <= 2) {
                if (!found) {
                    send_to_char("\r\nDid you mean:\r\n", ch);
                    found = TRUE;
                }
                sprintf(buf, "  %s\r\n", cmd_info[cmd].command);
                send_to_char(buf, ch);
            }
        }
    }
}

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument) {
    int cmd, length;
    extern int no_specials;
    char *line;

    /* just drop to next line for hitting CR */
    skip_slash(&argument);
    skip_spaces(&argument);
    if (!*argument)
        return;

    /*
     * special case to handle one-character, non-alphanumeric commands;
     * requested by many people so "'hi" or ";godnet test" is possible.
     * Patch sent by Eric Green and Stefan Wasilewski.
     */
    if (!isalpha(*argument)) {
        arg[0] = argument[0];
        arg[1] = '\0';
        line = argument + 1;
    } else
        line = any_one_arg(argument, arg);

    /* otherwise, find the command */
    if (GET_LEVEL(ch) < LVL_IMMORT &&
        (command_wtrigger(ch, arg, line) || command_mtrigger(ch, arg, line) || command_otrigger(ch, arg, line)))
        return; /* command trigger took over */

    for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
        if (!strncmp(cmd_info[cmd].command, arg, length))
            if (can_use_command(ch, cmd))
                break;

    if (IS_HIDDEN(ch) && !IS_SET(cmd_info[cmd].flags, CMD_HIDE)) {
        effect_from_char(ch, SPELL_NATURES_EMBRACE);
        GET_HIDDENNESS(ch) = 0;
    }

    if (PLR_FLAGGED(ch, PLR_MEDITATE) && !IS_SET(cmd_info[cmd].flags, CMD_MEDITATE)) {
        REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
        act("$n ceases $s meditative trance.", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("&8You stop meditating.\r\n&0", ch);
    }

    if (*cmd_info[cmd].command == '\n') {
        send_to_char(HUH, ch);
        list_similar_commands(ch, arg);
    } else if (PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_HEAD_B)
        send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
    else if (ch->desc && STATE(ch->desc) != CON_PLAYING && !IS_SET(cmd_info[cmd].flags, CMD_OLC)) {
        if (ch->desc->olc)
            send_to_char("You can't use that command while in OLC.\r\n", ch);
        else
            send_to_char("You can't use that command while writing.\r\n", ch);
    } else if (PLR_FLAGGED(ch, PLR_BOUND) && GET_LEVEL(ch) < LVL_IMMORT && !IS_SET(cmd_info[cmd].flags, CMD_BOUND))
        send_to_char("You try, but you're bound tight...\r\n", ch);
    else if (EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS) && !IS_SET(cmd_info[cmd].flags, CMD_MAJOR_PARA))
        send_to_char("&6You're paralyzed to the bone!\r\n&0", ch);
    else if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) && !IS_SET(cmd_info[cmd].flags, CMD_MINOR_PARA))
        send_to_char("&6You're paralyzed to the bone!\r\n&0", ch);
    else if (EFF_FLAGGED(ch, EFF_MESMERIZED) && GET_LEVEL(ch) < 100)
        send_to_char("You are too preoccupied with pretty illusions to do anything.\r\n", ch);
    else if (CASTING(ch) && !IS_SET(cmd_info[cmd].flags, CMD_CAST))
        send_to_char("&8You are busy spellcasting...&0\r\n", ch);
    else if (cmd_info[cmd].command_pointer == NULL)
        send_to_char("Sorry, that command hasn't been implemented yet.\r\n", ch);
    else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT)
        send_to_char("You can't use immortal commands while switched.\r\n", ch);
    else if (GET_STANCE(ch) == STANCE_FIGHTING && cmd_info[cmd].flags & CMD_NOFIGHT)
        send_to_char("No way!  You're fighting for your life!\r\n", ch);
    else if (GET_STANCE(ch) < cmd_info[cmd].minimum_stance) {
        switch (GET_STANCE(ch)) {
        case STANCE_DEAD:
            send_to_char("Lie still; you are DEAD!!! :-(\r\n", ch);
            break;
        case STANCE_INCAP:
        case STANCE_MORT:
            send_to_char("You are in a pretty bad shape, unable to do anything!\r\n", ch);
            break;
        case STANCE_STUNNED:
            send_to_char("All you can do right now is think about the stars!\r\n", ch);
            break;
        case STANCE_SLEEPING:
            send_to_char("In your dreams, or what?\r\n", ch);
            break;
        case STANCE_RESTING:
            send_to_char("Nah... You feel too relaxed to do that..\r\n", ch);
            break;
        default:
            send_to_char("I don't know what you're up to, but you can't do that!\r\n", ch);
            break;
        }
    } else if (GET_POS(ch) < cmd_info[cmd].minimum_position)
        switch (cmd_info[cmd].minimum_position) {
        case POS_PRONE:
            send_to_char("I don't know what kind of pretzel you've twisted yourself into!\r\n", ch);
            break;
        case POS_SITTING:
            send_to_char("Maybe you should sit up first?\r\n", ch);
            break;
        case POS_KNEELING:
            send_to_char("Maybe you should kneel first?\r\n", ch);
            break;
        case POS_STANDING:
            send_to_char("Maybe you should get on your feet first?\r\n", ch);
            break;
        default:
            send_to_char("You'd better take to the air first.\r\n", ch);
            break;
        }
    else if (no_specials || !special(ch, cmd, line))
        ((*cmd_info[cmd].command_pointer)(ch, line, cmd, cmd_info[cmd].subcmd));
}

/**************************************************************************
 * Routines to handle aliasing                                            *
 **************************************************************************/
/* These routines were heavily modified to incorporate aliases            */
/* into the pfile.  --Fingon                                              */

/* completely rewritten --Fingon */
struct alias_data *find_alias(struct alias_data *alias, char *str) {
    for (; alias; alias = alias->next)
        if (!str_cmp(alias->alias, str))
            return alias;

    return NULL;
}

void free_alias(struct alias_data *a) {
    if (a->alias)
        free(a->alias);
    if (a->replacement)
        free(a->replacement);
    free(a);
}

void free_aliases(struct alias_data *alias_list) {
    struct alias_data *alias;
    while ((alias = alias_list)) {
        alias_list = alias->next;
        free_alias(alias);
    }
}

/* The interface to the outside world: do_alias */
/* Modified heavily --Fingon                    */
ACMD(do_alias) {
    char *repl;
    struct alias_data *alias, *temp;
    struct char_data *vict;

    repl = any_one_arg(argument, arg);

    if (GET_LEVEL(ch) >= LVL_GOD && (vict = find_char_around_char(ch, find_vis_plr_by_name(ch, arg))))
        repl = any_one_arg(repl, arg);
    else
        vict = ch;

    if (IS_NPC(vict)) {
        send_to_char("NPCs don't have aliases.\r\n", ch);
        return;
    }

    if (!*arg) {
        /* no argument specified -- list currently defined aliases */
        send_to_char("Currently defined aliases:\r\n", ch);

        if ((alias = GET_ALIASES(vict)))
            for (; alias; alias = alias->next) {
                sprintf(buf, "%-15s %s\r\n", alias->alias, alias->replacement);
                send_to_char(buf, ch);
            }
        else
            send_to_char(" None.\r\n", ch);
    } else if (ch == vict || (GET_LEVEL(ch) >= LVL_ADMIN && GET_LEVEL(ch) > GET_LEVEL(vict))) {
        /* otherwise, add or remove aliases */

        /* is this an alias we've already defined? */
        if ((alias = find_alias(GET_ALIASES(vict), arg))) {
            REMOVE_FROM_LIST(alias, GET_ALIASES(vict), next);
            free_alias(alias);
        }

        skip_spaces(&repl);

        /* if no replacement string is specified, assume we want to delete */
        if (!*repl) {
            if (alias)
                send_to_char("Alias deleted.\r\n", ch);
            else
                send_to_char("No such alias.\r\n", ch);
        } else {

            /* otherwise, either add or redefine an alias */

            if (!str_cmp(arg, "alias")) {
                send_to_char("You can't alias 'alias'.\r\n", ch);
                return;
            }

            /* find a blank alias slot */
            CREATE(alias, struct alias_data, 1);
            alias->alias = strdup(arg);
            delete_doubledollar(repl);
            alias->replacement = strdup(repl);
            if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
                alias->type = ALIAS_COMPLEX;
            else
                alias->type = ALIAS_SIMPLE;
            alias->next = GET_ALIASES(vict);
            GET_ALIASES(vict) = alias;
            send_to_char("Alias added.\r\n", ch);
        }
    } else {
        sprintf(buf, "You cannot modify %s's aliases.\r\n", GET_NAME(vict));
        send_to_char(buf, ch);
    }
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS 9

void perform_complex_alias(struct char_data *ch, struct txt_q *input_q, char *orig, struct alias_data *alias) {
    struct txt_q temp_queue;
    char *tokens[NUM_TOKENS], *temp, *write_point;
    int num_of_tokens = 0, num;

    /* First, parse the original string */
    temp = strtok(strcpy(buf2, orig), " ");
    while (temp != NULL && num_of_tokens < NUM_TOKENS) {
        tokens[num_of_tokens++] = temp;
        temp = strtok(NULL, " ");
    }

    /* initialize */
    write_point = buf;
    temp_queue.head = temp_queue.tail = NULL;

    /* now parse the alias */
    for (temp = alias->replacement; *temp; temp++) {
        if (*temp == ALIAS_SEP_CHAR) {
            *write_point = '\0';
            buf[MAX_INPUT_LENGTH - 1] = '\0';
            write_to_q(buf, &temp_queue, 1, ch->desc);
            write_point = buf;
        } else if (*temp == ALIAS_VAR_CHAR) {
            temp++;
            if ((num = *temp - '1') < num_of_tokens && num >= 0) {
                strcpy(write_point, tokens[num]);
                write_point += strlen(tokens[num]);
            } else if (*temp == ALIAS_GLOB_CHAR) {
                strcpy(write_point, orig);
                write_point += strlen(orig);
            } else if ((*(write_point++) = *temp) == '$') /* redouble $ for act safety */
                *(write_point++) = '$';
        } else
            *(write_point++) = *temp;
    }

    *write_point = '\0';
    buf[MAX_INPUT_LENGTH - 1] = '\0';
    write_to_q(buf, &temp_queue, 1, ch->desc);

    /* push our temp_queue on to the _front_ of the input queue */
    if (input_q->head == NULL)
        *input_q = temp_queue;
    else {
        temp_queue.tail->next = input_q->head;
        input_q->head = temp_queue.head;
    }
}

/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data *d, char *orig) {
    char first_arg[MAX_INPUT_LENGTH], *ptr;
    struct alias_data *alias;

    /* Mobs don't have aliases. */
    if (IS_NPC(d->character))
        return (0);

    /* Quit now if there aren't any aliases */
    if (!GET_ALIASES(d->character))
        return (0);

    /* find the alias we're supposed to match */
    ptr = any_one_arg(orig, first_arg);

    /* bail out if it's null */
    if (!*first_arg)
        return 0;

    /* if the first arg is not an alias, return without doing anything */
    if (!(alias = find_alias(GET_ALIASES(d->character), first_arg)))
        return 0;

    if (alias->type == ALIAS_SIMPLE) {
        strcpy(orig, alias->replacement);
        return 0;
    } else {
        perform_complex_alias(d->character, &d->input, ptr, alias);
        return 1;
    }
}

/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 *
 * searchblock follows a similar naming convention to strcmp:
 * searchblock is case-sensitive, search_block is case-insensitive.
 * Often, which one you use only depends on the case of items in your
 * list, because any_one_arg and one_argument always return lower case
 * arguments.
 */
int searchblock(char *arg, const char **list, bool exact) {
    register int i, l;

    /* Make into lower case, and get length of string */
    for (l = 0; *(arg + l); l++)
        *(arg + l) = LOWER(*(arg + l));

    if (exact) {
        for (i = 0; **(list + i) != '\n'; i++)
            if (!strcmp(arg, *(list + i)))
                return (i);
    } else {
        if (!l)
            l = 1; /* Avoid "" to match the first available
                    * string */
        for (i = 0; **(list + i) != '\n'; i++)
            if (!strncmp(arg, *(list + i), l))
                return (i);
    }

    return -1;
}

int search_block(const char *arg, const char **list, bool exact) {
    register int i, len;

    if (!arg)
        return -1;

    if (exact) {
        for (i = 0; **(list + i) != '\n'; i++)
            if (!str_cmp(arg, *(list + i)))
                return (i);
    } else {
        len = strlen(arg);
        if (!len)
            len = 1; /* Avoid "" to match the first available
                        string */
        for (i = 0; **(list + i) != '\n'; i++)
            if (!strn_cmp(arg, *(list + i), (unsigned)len))
                return (i);
    }

    return (-1);
}

/* \s*\d+ */
bool is_number(const char *str) {
    if (!str || !*str)
        return FALSE;

    while (*str && isspace(*str))
        ++str;

    while (*str)
        if (!isdigit(*(str++)))
            return FALSE;

    return TRUE;
}

/* \d+ */
bool is_positive_integer(const char *str) {
    if (!str || (!*str))
        return FALSE;

    while (*str)
        if (!isdigit(*(str++)))
            return FALSE;

    return TRUE;
}

/* [+-]\d+ */
bool is_integer(const char *str) {
    if (!str)
        return FALSE;

    if (*str == '-')
        ++str;

    return is_positive_integer(str);
}

/* -\d+ For completeness sake */
bool is_negative_integer(const char *str) {
    if (!str || *(str++) != '-')
        return FALSE;

    return is_positive_integer(str);
}

void skip_slash(char **string) {
    if (**string && ((**string == '/') || (**string == '\\')))
        (*string)++;
}

void skip_spaces(char **string) {
    for (; **string && isspace(**string); (*string)++)
        ;
}

/* Given a string, change all instances of double dollar signs ($$) to single
 * dollar signs ($).  When strings come in, all $'s are changed to $$'s to
 * avoid having users be able to crash the system if the inputted string is
 * eventually sent to act().  If you are using user input to produce screen
 * output AND YOU ARE SURE IT WILL NOT BE SENT THROUGH THE act() FUNCTION
 * (i.e., do_gecho, do_title, but NOT do_gsay), you can call
 * delete_doubledollar() to make the output look correct.
 * Modifies the string in-place. */
char *delete_doubledollar(char *string) {
    char *read, *write;

    /* If the string has no dollar signs, return immediately */
    if ((write = strchr(string, '$')) == NULL)
        return string;

    /* Start from the location of the first dollar sign */
    read = write;

    /* Until we reach the end of the string... */
    while (*read)
        if ((*(write++) = *(read++)) == '$') /* copy one char */
            if (*read == '$')
                read++; /* skip if we saw 2 $'s in a row */

    *write = '\0';

    return string;
}

int fill_word(char *argument) {
    /* Needs to be case-insensitive since fill_word is used by the nanny for
     * name-checking */
    return (search_block(argument, fill, TRUE) >= 0);
}

int reserved_word(char *argument) {
    /* Needs to be case-insensitive since fill_word is used by the nanny for
     * name-checking */
    return (search_block(argument, reserved, TRUE) >= 0);
}

/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg) {
    char *begin = first_arg;

    do {
        skip_spaces(&argument);

        first_arg = begin;
        while (*argument && !isspace(*argument)) {
            *(first_arg++) = LOWER(*argument);
            argument++;
        }

        *first_arg = '\0';
    } while (fill_word(begin));

    return argument;
}

char *delimited_arg(char *argument, char *first_arg, char delimiter) {
    skip_spaces(&argument);

    if (*argument == delimiter) {
        argument++;
        while (*argument && *argument != delimiter) {
            *(first_arg++) = LOWER(*argument);
            argument++;
        }
        argument++;
    } else {
        while (*argument && !isspace(*argument)) {
            *(first_arg++) = LOWER(*argument);
            argument++;
        }
    }

    *first_arg = '\0';

    return argument;
}

/* Like delimited_arg, but don't lowercase everything */
char *delimited_arg_case(char *argument, char *first_arg, char delimiter) {
    skip_spaces(&argument);

    if (*argument == delimiter) {
        argument++;
        while (*argument && *argument != delimiter) {
            *(first_arg++) = *argument;
            argument++;
        }
        argument++;
    } else {
        while (*argument && !isspace(*argument)) {
            *(first_arg++) = *argument;
            argument++;
        }
    }

    *first_arg = '\0';

    return argument;
}

char *delimited_arg_all(char *argument, char *first_arg, char delimiter) {
    skip_spaces(&argument);

    if (*argument == delimiter) {
        argument++;
        while (*argument && *argument != delimiter) {
            *(first_arg++) = LOWER(*argument);
            argument++;
        }
        argument++;
    } else {
        while (*argument) {
            *(first_arg++) = LOWER(*argument);
            argument++;
        }
    }

    *first_arg = '\0';

    return argument;
}

/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(char *argument, char *first_arg) { return delimited_arg(argument, first_arg, '\"'); }

/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg) {
    skip_spaces(&argument);

    while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
    }

    *first_arg = '\0';

    return argument;
}

/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg) {
    return one_argument(one_argument(argument, first_arg), second_arg); /* :-) */
}

/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 *
 * returns 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2) {
    if (!*arg1)
        return 0;

    for (; *arg1 && *arg2; arg1++, arg2++)
        if (LOWER(*arg1) != LOWER(*arg2))
            return 0;

    if (!*arg1)
        return 1;
    else
        return 0;
}

void display_classes(struct descriptor_data *d, int select) {
    /*  int x; */ /* Commented out for commenting of - Subclassing
                     explanation/preface RSD */
    int char_race;
    int mageok, warriorok, rogueok, clericok;

    char_race = (int)GET_RACE(d->character);
    *buf = '\0';
    *buf2 = '\0';
    mageok = class_ok_race[char_race][CLASS_SORCERER];
    warriorok = class_ok_race[char_race][CLASS_WARRIOR];
    rogueok = class_ok_race[char_race][CLASS_ROGUE];
    clericok = class_ok_race[char_race][CLASS_CLERIC];
    /* commenter out by Fingh 11/7 class_ok_race[char_race][CLASS_SHAMAN]; */
    if (select) {
        send_to_char(subclass_descrip, d->character);
        send_to_char(subclass_descrip2, d->character);
        send_to_char("\r\n&5Class selection menu - \r\n ", d->character); /* Added return newline after menu - RSD */
    }
    /*  send_to_char("&1&b(&0&5*&1&b) denotes a class available to your
     * race!&0&5\r\n\r\n", d->character); */

    /*

       for (x = 0; x < NUM_CLASSES; x++)
       if (class_ok_race[(int)GET_RACE(d->character)][x])
       send_to_char(class_display[x], d->character);
       send_to_char("\nClass: ", d->character);
     */
    /* Subclassing explaination/preface */
    /*  sprintf(buf,"&5&b");
      if(warriorok){
        sprintf(buf, "%s%-16.16s ", buf,
                strip_ansi(pc_class_types[CLASS_WARRIOR]));
        sprintf(buf2, "%-16.16s ", "=======");
      }
      if(clericok) {
        sprintf(buf, "%s%-16.16s ", buf,
                strip_ansi(pc_class_types[CLASS_CLERIC]));
        sprintf(buf2, "%s%-16.16s ", buf2, "======");
      }
      if(mageok) {
        sprintf(buf, "%s%-16.16s ", buf,
                strip_ansi(pc_class_types[CLASS_SORCERER]));
        sprintf(buf2, "%s%-16.16s ", buf2, "========");
      }
      if(rogueok) {
        sprintf(buf, "%s%-16.16s ", buf,
                strip_ansi(pc_class_types[CLASS_ROGUE]));
        sprintf(buf2, "%s%-16.16s ", buf2, "=====");
      }
      if(shamanok) {
        sprintf(buf, "%s%-16.16s", buf,
                strip_ansi(pc_class_types[CLASS_SHAMAN]));
        sprintf(buf2, "%s%-16.16s", buf2, "======");
      }
      sprintf(buf,"%s\r\n", buf);
      send_to_char(buf, d->character);
      send_to_char(buf2, d->character);
      sprintf(buf, "\r\n&5");
      for (x=0;x<MAX(MAX(MAX(WARRIOR_SUBCLASSES, CLERIC_SUBCLASSES),
      MAGE_SUBCLASSES), ROGUE_SUBCLASSES);x++) { if(warriorok) if (x <
      WARRIOR_SUBCLASSES) sprintf(buf, "%s%s%-15.15s ", buf,
                    class_ok_race[char_race][warrior_subclasses[x]] ? "*" : " ",
                    strip_ansi(pc_class_types[warrior_subclasses[x]]));
          else
            sprintf(buf, "%s%-16.16s ", buf, " ");
        if(clericok)
          if (x < CLERIC_SUBCLASSES)
            sprintf(buf, "%s%s%-15.15s ", buf,
                    class_ok_race[char_race][cleric_subclasses[x]] ? "*" : " ",
                    strip_ansi(pc_class_types[cleric_subclasses[x]]));
          else
            sprintf(buf, "%s%-16.16s ", buf, " ");
        if(mageok)
          if (x < MAGE_SUBCLASSES)
            sprintf(buf, "%s%s%-15.15s ", buf,
                    class_ok_race[char_race][mage_subclasses[x]] ? "*" : " ",
                    strip_ansi(pc_class_types[mage_subclasses[x]]));
          else
            sprintf(buf, "%s%-16.16s ", buf, " ");
        if(rogueok)
          if (x < ROGUE_SUBCLASSES)
            sprintf(buf, "%s%s%-15.15s", buf,
                    class_ok_race[char_race][rogue_subclasses[x]] ? "*" : " ",
                    strip_ansi(pc_class_types[rogue_subclasses[x]]));
          else
            sprintf(buf, "%s%-16.16s", buf, " ");

        sprintf(buf, "%s\r\n", buf);
      }
    */
    if (select)
        sprintf(buf, "%s\r\n&6Choose -%s%s%s%s: ", buf, warriorok ? " [&0&1&bw&0&6]arrior" : "",
                clericok ? " [&0&1&bc&0&6]leric" : "", mageok ? " [&0&1&bs&0&6]orcerer" : "",
                rogueok ? " [&0&1&br&0&6]ogue" : "");
    send_to_char(buf, d->character);
    send_to_char("&0", d->character);
}

/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2) {
    char *temp;

    temp = any_one_arg(string, arg1);
    skip_spaces(&temp);
    strcpy(arg2, temp);
}

/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(char *command) {
    int cmd;

    for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
        if (!strcmp(cmd_info[cmd].command, command))
            return cmd;

    return -1;
}

int parse_command(char *command) {
    int cmd, length = strlen(command);

    for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
        if (!strncmp(cmd_info[cmd].command, command, length))
            return cmd;

    return -1;
}

int special(struct char_data *ch, int cmd, char *arg) {
    register struct obj_data *i;
    register struct char_data *k;
    int j;

    /* special in room? */
    if (GET_ROOM_SPEC(ch->in_room) != NULL)
        if (GET_ROOM_SPEC(ch->in_room)(ch, world + ch->in_room, cmd, arg))
            return 1;

    /* special in equipment list? */
    for (j = 0; j < NUM_WEARS; j++)
        if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
            if (GET_OBJ_SPEC(GET_EQ(ch, j))(ch, GET_EQ(ch, j), cmd, arg))
                return 1;

    /* special in inventory? */
    for (i = ch->carrying; i; i = i->next_content)
        if (GET_OBJ_SPEC(i) != NULL)
            if (GET_OBJ_SPEC(i)(ch, i, cmd, arg))
                return 1;

    /* special in mobile present? */
    for (k = world[ch->in_room].people; k; k = k->next_in_room)
        if (GET_MOB_SPEC(k) != NULL && !MOB_FLAGGED(k, MOB_NOSCRIPT))
            if (GET_MOB_SPEC(k)(ch, k, cmd, arg))
                return 1;

    /* special in object present? */
    for (i = world[ch->in_room].contents; i; i = i->next_content)
        if (GET_OBJ_SPEC(i) != NULL)
            if (GET_OBJ_SPEC(i)(ch, i, cmd, arg))
                return 1;

    return 0;
}

/* *************************************************************************
 *  Stuff for controlling the non-playing sockets (get name, pwd etc)      *
 ************************************************************************* */

/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int find_name(char *name) {
    int i;

    for (i = 0; i <= top_of_p_table; i++) {
        if (!str_cmp((player_table + i)->name, name))
            return i;
    }

    return -1;
}

#define RECON 1
#define USURP 2
#define UNSWITCH 3

int perform_dupe_check(struct descriptor_data *d) {
    struct descriptor_data *k, *next_k;
    struct char_data *target = NULL, *ch, *next_ch;
    int mode = 0;

    int id = GET_IDNUM(d->character);

    /*
     * Now that this descriptor has successfully logged in, disconnect all
     * other descriptors controlling a character with the same ID number.
     */

    for (k = descriptor_list; k; k = next_k) {
        next_k = k->next;

        if (k == d)
            continue;

        if (k->original && (GET_IDNUM(k->original) == id)) { /* switched char */
            write_to_output("\r\nMultiple login detected -- disconnecting.\r\n", k);
            STATE(k) = CON_CLOSE;
            if (k->character && k->original->player.level <= 100) {
                if (POSSESSED(k->character))
                    do_shapechange(k->character, "me", 0, 1);
                mode = UNSWITCH;
            } else if (!target) {
                target = k->original;
                mode = UNSWITCH;
            }
            if (k->character)
                k->character->desc = NULL;
            k->character = NULL;
            k->original = NULL;
        } else if (k->character && (GET_IDNUM(k->character) == id)) {
            if (!target && STATE(k) == CON_PLAYING) {
                write_to_output("\r\nThis body has been usurped!\r\n", k);
                target = k->character;
                mode = USURP;
            }
            k->character->desc = NULL;
            k->character = NULL;
            k->original = NULL;
            write_to_output("\r\nMultiple login detected -- disconnecting.\r\n", k);
            STATE(k) = CON_CLOSE;
        }
    }

    /*
     * now, go through the character list, deleting all characters that
     * are not already marked for deletion from the above step (i.e., in the
     * CON_HANGUP state), and have not already been selected as a target for
     * switching into.  In addition, if we haven't already found a target,
     * choose one if one is available (while still deleting the other
     * duplicates, though theoretically none should be able to exist).
     */

    for (ch = character_list; ch; ch = next_ch) {
        next_ch = ch->next;

        if (IS_NPC(ch))
            continue;
        if (GET_IDNUM(ch) != id)
            continue;

        /* ignore chars with descriptors (already handled by above step) */
        if (ch->desc)
            continue;

        /* don't extract the target char we've found one already */
        if (ch == target)
            continue;

        /* we don't already have a target and found a candidate for switching */
        if (!target) {
            target = ch;
            mode = RECON;
            continue;
        }

        /* we've found a duplicate - blow him away, dumping his eq in limbo. */
        if (ch->in_room != NOWHERE)
            char_from_room(ch);
        char_to_room(ch, 1);
        extract_char(ch);
    }

    /* no target for swicthing into was found - allow login to continue */
    if (!target)
        return 0;

    /* Okay, we've found a target.  Connect d to target. */
    free_char(d->character); /* get rid of the old char */
    d->character = target;
    d->character->desc = d;
    d->original = NULL;
    d->character->char_specials.timer = 0;
    d->character->forward = NULL;
    REMOVE_FLAG(PLR_FLAGS(d->character), PLR_WRITING);
    REMOVE_FLAG(PLR_FLAGS(d->character), PLR_MAILING);
    STATE(d) = CON_PLAYING;

    switch (mode) {
    case RECON:
        write_to_output("Reconnecting.\r\n", d);
        act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
        sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
        mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
        break;
    case USURP:
        write_to_output("Overriding old connection.\r\n", d);
        act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
            "$n's body has been taken over by a new spirit!",
            TRUE, d->character, 0, 0, TO_ROOM);
        sprintf(buf, "%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
        mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
        break;
    case UNSWITCH:
        write_to_output("Reconnecting to unswitched char.", d);
        sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
        mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
        break;
    }

    return 1;
}

int enter_player_game(struct descriptor_data *d) {
    int load_result;
    int load_room;
    int i;

    extern int r_mortal_start_room;
    extern int r_immort_start_room;
    extern int r_frozen_start_room;

    reset_char(d->character);
    if (GET_AUTOINVIS(d->character) > -1)
        GET_INVIS_LEV(d->character) = MIN(GET_LEVEL(d->character), GET_AUTOINVIS(d->character));

    if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
        load_room = real_room(load_room);

    /* If char was saved with NOWHERE, or real_room above failed... */
    if (load_room == NOWHERE) {
        if (GET_LEVEL(d->character) >= LVL_IMMORT) {
            load_room = real_room(GET_HOMEROOM(d->character));
            if (load_room < 0) {
                log("SYSERR: An immortal start room does not exist.");
                load_room = r_immort_start_room;
            }
        } else {
            load_room = real_room(GET_HOMEROOM(d->character));
            if (load_room < 0) {
                log("SYSERR: Mortal start room does not exist.  Change in config.c.");
                load_room = r_mortal_start_room;
            }
        }
    }

    if (PLR_FLAGGED(d->character, PLR_FROZEN))
        load_room = r_frozen_start_room;

    char_to_room(d->character, load_room);
    if ((load_result = load_objects(d->character)))
        if (GET_LEVEL(d->character) < LVL_IMMORT && !PLR_FLAGGED(d->character, PLR_FROZEN)) {
            char_from_room(d->character);
            char_to_room(d->character, load_room);
        }
    load_quests(d->character);
    load_pets(d->character);

    d->character->player.time.logon = time(0);
    GET_ID(d->character) = GET_IDNUM(d->character);
    d->character->next = character_list;
    character_list = d->character;

    /* send_save_description() will use this actual, error-checked value for the
     * load room */
    GET_LOADROOM(d->character) = load_room == NOWHERE ? NOWHERE : world[load_room].vnum;
    send_save_description(d->character, NULL, TRUE);
    save_player_char(d->character);
    GET_QUIT_REASON(d->character) = QUIT_AUTOSAVE;

    /*
     * A couple of hacks to reconnect things if the player logged out
     * to the menu and then re-entered the game
     */
    if (GET_CLAN_MEMBERSHIP(d->character))
        GET_CLAN_MEMBERSHIP(d->character)->player = d->character;
    /* restart cooldowns */
    for (i = 0; i < NUM_COOLDOWNS; ++i)
        if (GET_COOLDOWN(d->character, i)) {
            SET_COOLDOWN(d->character, i, GET_COOLDOWN(d->character, i));
            break;
        }

    if (!(GET_LEVEL(d->character) >= LVL_IMMORT && GET_INVIS_LEV(d->character))) {
        all_except_printf(d->character, "The ground shakes slightly with the arrival of %s.\r\n",
                          GET_NAME(d->character));
    }

    return load_result;
}

void clear_player_name(struct char_data *ch) {
    if (ch->player.short_descr) {
        free(ch->player.short_descr);
        ch->player.short_descr = NULL;
    }
    if (ch->player.namelist) {
        free(ch->player.namelist);
        ch->player.namelist = NULL;
    }
}

void set_player_name(struct char_data *ch, char *name) {
    char *s;

    clear_player_name(ch);
    ch->player.short_descr = strdup(CAP(name));
    ch->player.namelist = strdup(name);

    for (s = ch->player.namelist; *s; s++)
        *s = tolower(*s);
}

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg) {
    char bcbuf[15] = "0";
    int player_i, load_result;
    char tmp_name[MAX_INPUT_LENGTH];
#ifdef PRODUCTION
    extern char *GREETINGS;
    extern char *GREETINGS2;
    extern char *GREETINGS3;
    extern char *GREETINGS4;
#else
    extern char *TEST_GREETING;
    extern char *TEST_GREETING2;
    extern char *TEST_GREETING3;
#endif
    extern char *WHOAREYOU;
    extern char *NEWSUPDATED1;
    extern char *NEWSUPDATED2;
#ifdef PRODUCTION
    extern char *NAMES_EXPLANATION;
#else
#endif
    extern int max_bad_pws;
    extern void modify_player_index_file(char *name, char *newname);
    extern int make_count;
    int color = 0;
    int set_new_home_town(int race, int class, char);
    int i;

    struct {
        int state;
        void (*func)(struct descriptor_data *, char *);
    } olc_functions[] = {
        {CON_OEDIT, oedit_parse},
        {CON_IEDIT, oedit_parse},
        {CON_ZEDIT, zedit_parse},
        {CON_SEDIT, sedit_parse},
        {CON_MEDIT, medit_parse},
        {CON_REDIT, redit_parse},
        {CON_TRIGEDIT, trigedit_parse},
        {CON_HEDIT, hedit_parse},
        {CON_SDEDIT, sdedit_parse},
        {CON_GEDIT, gedit_parse},
        {-1, NULL},
    };

    skip_spaces(&arg);

    if (d->character == NULL) {
        CREATE(d->character, struct char_data, 1);
        clear_char(d->character);
        CREATE(d->character->player_specials, struct player_special_data, 1);
        d->character->desc = d;
    }

    /* Quick check for the OLC states. */
    for (player_i = 0; olc_functions[player_i].state >= 0; ++player_i)
        if (STATE(d) == olc_functions[player_i].state) {
            /* Allow lines starting with ~ to go to the command interpreter */
            if (*arg == '~') {
                if (PRF_FLAGGED(d->character, PRF_OLCCOMM)) {
                    command_interpreter(d->character, ++arg);
                    write_to_output("~: ", d);
                } else
                    write_to_output("You must have OLCComm toggled on to use commands in OLC.\r\n", d);
            } else
                (*olc_functions[player_i].func)(d, arg);
            return;
        }

    switch (STATE(d)) {
    case CON_QANSI:
        if ((make_count >= 0)) {
            sprintf(bcbuf, " %d", make_count);
        }
        switch (yesno_result(arg)) {
        case YESNO_YES:
        case YESNO_NONE:
            SET_FLAG(PRF_FLAGS(d->character), PRF_COLOR_1);
            SET_FLAG(PRF_FLAGS(d->character), PRF_COLOR_2);
            write_to_output("Color is on.\r\n", d);
#ifdef PRODUCTION
            write_to_output(GREETINGS, d);
            write_to_output(GREETINGS2, d);
            write_to_output(GREETINGS3, d);
            write_to_output(GREETINGS4, d);
#else
            write_to_output(TEST_GREETING, d);
            write_to_output(TEST_GREETING2, d);
            write_to_output(TEST_GREETING3, d);
#endif
            write_to_output(bcbuf, d);
            write_to_output(WHOAREYOU, d);
            STATE(d) = CON_GET_NAME;
            break;
        case YESNO_NO:
            REMOVE_FLAG(PRF_FLAGS(d->character), PRF_COLOR_1);
            REMOVE_FLAG(PRF_FLAGS(d->character), PRF_COLOR_2);
            write_to_output("Color is off.\r\n", d);
#ifdef PRODUCTION
            write_to_output(GREETINGS, d);
            write_to_output(GREETINGS2, d);
            write_to_output(GREETINGS3, d);
            write_to_output(GREETINGS4, d);
#else
            write_to_output(TEST_GREETING, d);
            write_to_output(TEST_GREETING2, d);
            write_to_output(TEST_GREETING3, d);
#endif
            write_to_output(bcbuf, d);
            write_to_output(WHOAREYOU, d);
            STATE(d) = CON_GET_NAME;
            break;
        default:
            write_to_output("Please answer Y or N.\r\n", d);
            write_to_output("Do you want ANSI terminal support? (Y/n) ", d);
        }
        break;
    case CON_GET_NAME:
        /* They merely pressed enter... disconnect them. */
        if (!*arg) {
            close_socket(d);
            return;
        }

        /* Basic name kosherness check:
         *  - must be at least 2 characters long
         *  - must be no more than MAX_NAME_LENGTH long
         *  - fill_word()  (??)
         *  - reserved_word()  (??)
         */

        if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 || strlen(tmp_name) > MAX_NAME_LENGTH ||
            fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {
            write_to_output(
                "Invalid name, please try another.\r\n"
                "Name: ",
                d);
            return;
        }

        if ((player_i = load_player(tmp_name, d->character)) > -1) {

            /* See if a deleted player's file was loaded... */
            if (PLR_FLAGGED(d->character, PLR_DELETED)) {
                if ((player_i = get_ptable_by_name(tmp_name)) >= 0)
                    delete_player(player_i);
                free_char(d->character);
                CREATE(d->character, struct char_data, 1);
                clear_char(d->character);
                CREATE(d->character->player_specials, struct player_special_data, 1);
                d->character->desc = d;
                set_player_name(d->character, tmp_name);
                GET_PFILEPOS(d->character) = player_i;

                if (color) {
                    SET_FLAG(PRF_FLAGS(d->character), PRF_COLOR_1);
                    SET_FLAG(PRF_FLAGS(d->character), PRF_COLOR_2);
                } else {
                    REMOVE_FLAG(PRF_FLAGS(d->character), PRF_COLOR_1);
                    REMOVE_FLAG(PRF_FLAGS(d->character), PRF_COLOR_2);
                }
                sprintf(buf, "\r\nDo you want to make a new character called %s? ", tmp_name);
                write_to_output(buf, d);
                STATE(d) = CON_NAME_CNFRM;
            } else {
                /* An existing player's name was entered, and the pfile was successfully
                 * loaded */

                if (PRF_FLAGGED(d->character, PRF_COLOR_1))
                    color = 1;

                /* undo it just in case they are set */
                REMOVE_FLAG(PLR_FLAGS(d->character), PLR_WRITING);
                REMOVE_FLAG(PLR_FLAGS(d->character), PLR_MAILING);
                REMOVE_FLAG(PLR_FLAGS(d->character), PLR_CRYO);

                if (color) {
                    SET_FLAG(PRF_FLAGS(d->character), PRF_COLOR_1);
                    SET_FLAG(PRF_FLAGS(d->character), PRF_COLOR_2);
                } else {
                    REMOVE_FLAG(PRF_FLAGS(d->character), PRF_COLOR_1);
                    REMOVE_FLAG(PRF_FLAGS(d->character), PRF_COLOR_2);
                }

                write_to_output("Password: ", d);
                echo_off(d);
                d->idle_tics = 0;
                STATE(d) = CON_PASSWORD;
            }
        } else {
            /* No player loaded: we might make a new character. */

            if (ispell_name_check(tmp_name)) {
                /* Take a character name that is a word in the dictionary or
                   closely resembles a word in the dictionary and make them
                   think it's a valid existing character name and boot the
                   connection.  Yes I'm evil - RSD 8/29/2002 <-- genius
                 */
                sprintf(buf, "%s is being ninja rejected by the name approval code.", tmp_name);
                log(buf);
                write_to_output("Welcome back!\r\n", d);
                write_to_output("Password: ", d);
                echo_off(d);
                d->idle_tics = 0;
                STATE(d) = CON_ISPELL_BOOT;
                return;
            }

            if (!Valid_Name(tmp_name)) {
                write_to_output("Invalid name, please try another.\r\n", d);
                write_to_output("Name: ", d);
                return;
            }

            set_player_name(d->character, tmp_name);

            sprintf(buf, "\r\nDo you want to make a new character called %s? ", tmp_name);
            write_to_output(buf, d);
            STATE(d) = CON_NAME_CNFRM;
            /* End of new player business */
        }

        break;
    case CON_NAME_CNFRM: /* wait for conf. of new name    */
        switch (yesno_result(arg)) {
        case YESNO_YES:
            if (isbanned(d->host) >= BAN_NEW) {
                sprintf(buf, "Request for new char %s denied from [%s] (siteban)", GET_NAME(d->character), d->host);
                mudlog(buf, NRM, LVL_GOD, TRUE);
                write_to_output("Sorry, new characters are not allowed from your site!\r\n", d);
                STATE(d) = CON_CLOSE;
                return;
            }
            if (should_restrict) {
                if (restrict_reason == RESTRICT_AUTOBOOT) {
                    write_to_output("Sorry, no new players because the mud is rebooting shortly.\r\n", d);
                    write_to_output("Please try again in five minutes.\r\n", d);
                } else {
                    write_to_output("Sorry, new players can't be created at the moment.\r\n", d);
                }
                sprintf(buf, "Request for new char %s denied from %s (wizlock)", GET_NAME(d->character), d->host);
                mudlog(buf, NRM, LVL_GOD, TRUE);
                STATE(d) = CON_CLOSE;
                return;
            }

            /* This deletes any existing files a NEW character might have from a
               leftover old character of the same name... I hope RSD 10/11/2000 */
            get_pfilename(GET_NAME(d->character), buf, PLR_FILE);
            if (unlink(buf) == 0) {
                sprintf(buf, "SYSERR: Deleted existing player file for NEW ch %s.", GET_NAME(d->character));
                log(buf);
            }
            get_pfilename(GET_NAME(d->character), buf, OBJ_FILE);
            if (unlink(buf) == 0) {
                sprintf(buf, "SYSERR: Deleted existing object file for NEW ch %s.", GET_NAME(d->character));
                log(buf);
            }

#ifdef PRODUCTION
            STATE(d) = CON_NAME_CHECK;
            write_to_output(NAMES_EXPLANATION, d);
            sprintf(buf, "Do you believe \"%s\" is acceptable by these standards? (Y/N) ", GET_NAME(d->character));
            write_to_output(buf, d);
            break;
#else
            sprintf(buf, "\r\nGive me a password for %s: ", GET_NAME(d->character));
            write_to_output(buf, d);
            echo_off(d);
            STATE(d) = CON_NEWPASSWD;
#endif

            break;
        case YESNO_NO:
            write_to_output("Okay, what IS it, then? ", d);
            clear_player_name(d->character);
            STATE(d) = CON_GET_NAME;
            break;
        default:
            write_to_output("Please answer yes or no: ", d);
        }
        break;
    case CON_NEW_NAME:
        if (!*arg) {
            write_to_output("Name: ", d);
            return;
        }
        if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 || strlen(tmp_name) > MAX_NAME_LENGTH ||
            fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {
            write_to_output(
                "c> Invalid name, please try another.\r\n"
                "Name: ",
                d);
            return;
        }
        if ((player_i = load_player(tmp_name, d->character)) > -1) {
            write_to_output("That name is already taken!\r\n", d);
            write_to_output("Name: ", d);
            return;
        }
        if (!Valid_Name(arg)) {
            write_to_output("d> Invalid name, please try another.\r\n", d);
            write_to_output("Name: ", d);
            return;
        } else {
            if (PLR_FLAGGED(d->character, PLR_NEWNAME)) {
                sprintf(buf, "Renaming player %s to %s!", GET_NAME(d->character), arg);
                log(buf);

                /* send the old name to the invalid list */
                send_to_xnames(GET_NAME(d->character));

                /* Rename the player's files */
                rename_player(d->character, arg);
                set_player_name(d->character, arg);
                save_player_char(d->character);
            } else {
                set_player_name(d->character, arg);
            }

            write_to_output("Now you must wait to be re-approved.\r\n", d);
            if (!PLR_FLAGGED(d->character, PLR_NAPPROVE))
                SET_FLAG(PLR_FLAGS(d->character), PLR_NAPPROVE);
            event_create(EVENT_NAME_TIMEOUT, name_timeout, d, FALSE, NULL, NAME_TIMEOUT);
            REMOVE_FLAG(PLR_FLAGS(d->character), PLR_NEWNAME);
            write_to_output(
                "Now you must wait for your name to be approved by an immortal.\r\n"
                "If no one is available, you will be granted entry in a VERY short "
                "time.\r\n",
                d);
            broadcast_name(GET_NAME(d->character));
            STATE(d) = CON_NAME_WAIT_APPROVAL;
        }
        break;
    case CON_NAME_WAIT_APPROVAL:
        write_to_output("You must wait to be approved.\r\n", d);
        break;
    case CON_ISPELL_BOOT:
        write_to_output("\r\nWrong password... disconnecting.\r\n", d);
        STATE(d) = CON_CLOSE;
        break;
    case CON_PASSWORD: /* get pwd for known player      */
        /*
         * To really prevent duping correctly, the player's record should
         * be reloaded from disk at this point (after the password has been
         * typed).  However I'm afraid that trying to load a character over
         * an already loaded character is going to cause some problem down the
         * road that I can't see at the moment.  So to compensate, I'm going to
         * (1) add a 15 or 20-second time limit for entering a password, and (2)
         * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
         */

        echo_on(d); /* turn echo back on */

        if (!*arg)
            close_socket(d);
        else {
            if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
                sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
                mudlog(buf, BRF, LVL_GOD, TRUE);
                GET_BAD_PWS(d->character)++;
                save_player_char(d->character);
                if (++(d->bad_pws) >= max_bad_pws) { /* 3 strikes and you're out. */
                    write_to_output("Wrong password... disconnecting.\r\n", d);
                    STATE(d) = CON_CLOSE;
                } else {
                    write_to_output("Wrong password.\r\nPassword: ", d);
                    echo_off(d);
                }
                return;
            }
            load_result = GET_BAD_PWS(d->character);
            GET_BAD_PWS(d->character) = 0;

            if (isbanned(d->host) == BAN_SELECT && !PLR_FLAGGED(d->character, PLR_SITEOK)) {
                write_to_output(
                    "Sorry, this char has not been cleared for login from "
                    "your site!\r\n",
                    d);
                STATE(d) = CON_CLOSE;
                sprintf(buf, "Connection attempt for %s denied from %s", GET_NAME(d->character), d->host);
                mudlog(buf, NRM, LVL_GOD, TRUE);
                return;
            }
            if (GET_LEVEL(d->character) < should_restrict) {
                if (restrict_reason == RESTRICT_AUTOBOOT) {
                    write_to_output("The game is restricted due to an imminent reboot.\r\n", d);
                    write_to_output("Please try again in 2-3 minutes.\r\n", d);
                } else {
                    write_to_output(
                        "The game is temporarily restricted.  Please Try "
                        "again later.\r\n",
                        d);
                }
                STATE(d) = CON_CLOSE;
                sprintf(buf, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
                mudlog(buf, NRM, LVL_GOD, TRUE);
                return;
            }
            /* check and make sure no other copies of this player are logged in */
            if (perform_dupe_check(d))
                return;

            if (PLR_FLAGGED(d->character, PLR_NEWNAME)) {
                write_to_output(
                    "Your name has been deemed unacceptable.  Please "
                    "choose a new one.\r\n",
                    d);
                write_to_output("Name: ", d);
                STATE(d) = CON_NEW_NAME;
                return;
            }

            if (GET_LEVEL(d->character) >= LVL_IMMORT)
                write_to_output(get_text(TEXT_IMOTD), d);
            else
                write_to_output(get_text(TEXT_MOTD), d);

            if (d->character->player.time.logon < get_text_update_time(TEXT_NEWS)) {
                write_to_output(NEWSUPDATED1, d);
                write_to_output(NEWSUPDATED2, d);
            }

            if (GET_CLAN(d->character) && GET_CLAN(d->character)->motd)
                dprintf(d, "\r\n%s%s news:\r\n%s", GET_CLAN(d->character)->name, ANRM, GET_CLAN(d->character)->motd);

            sprintf(buf, "%s [%s] has connected.  Last login: %ld.", GET_NAME(d->character), d->host,
                    d->character->player.time.logon);
            mudlog(buf, BRF,
                   MAX(LVL_IMMORT,
                       MIN(GET_LEVEL(d->character), MAX(GET_AUTOINVIS(d->character), GET_INVIS_LEV(d->character)))),
                   TRUE);
            if (load_result) {
                sprintf(buf,
                        "\r\n\r\n\007\007\007"
                        "%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
                        CLRLV(d->character, FRED, C_SPR), load_result, (load_result > 1) ? "S" : "",
                        CLRLV(d->character, ANRM, C_SPR));
                write_to_output(buf, d);
                GET_BAD_PWS(d->character) = 0;
            }
            write_to_output("\r\n\n*** PRESS RETURN: ", d);
            STATE(d) = CON_RMOTD;
        }
        break;

    case CON_NEWPASSWD:
    case CON_CHPWD_GETNEW:
        if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 || !str_cmp(arg, GET_NAME(d->character))) {
            write_to_output("\r\nIllegal password.\r\n", d);
            write_to_output("Password: ", d);
            return;
        }
        strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_NAME(d->character)), MAX_PWD_LENGTH);
        *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

        write_to_output("\r\nPlease retype password: ", d);
        if (STATE(d) == CON_NEWPASSWD)
            STATE(d) = CON_CNFPASSWD;
        else
            STATE(d) = CON_CHPWD_VRFY;

        break;

    case CON_CNFPASSWD:
    case CON_CHPWD_VRFY:
        if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
            write_to_output("\r\nPasswords don't match... start over.\r\n", d);
            write_to_output("Password: ", d);
            if (STATE(d) == CON_CNFPASSWD)
                STATE(d) = CON_NEWPASSWD;
            else
                STATE(d) = CON_CHPWD_GETNEW;
            return;
        }
        echo_on(d);

        if (STATE(d) == CON_CNFPASSWD) {
            write_to_output("\r\nWhat is your sex (M/F)? ", d);
            STATE(d) = CON_QSEX;
        } else {
            save_player_char(d->character);
            echo_on(d);
            write_to_output("\r\nDone.\r\n", d);
            write_to_output(MENU, d);
            STATE(d) = CON_MENU;
        }

        break;

    case CON_QSEX: /* query sex of new user         */
        switch (*arg) {
        case 'm':
        case 'M':
            d->character->player.sex = SEX_MALE;
            break;
        case 'f':
        case 'F':
            d->character->player.sex = SEX_FEMALE;
            break;
        default:
            write_to_output(
                "\r\nThat is not a sex!\r\n"
                "What IS your sex? (M/F) ",
                d);
            return;
            break;
        }
        if (races_allowed) {
            send_race_menu(d);
            write_to_output("\r\nRace: ", d);
            STATE(d) = CON_QRACE;
            break;
        } else {
            GET_RACE(d->character) = RACE_HUMAN;
            STATE(d) = CON_QCLASS;
            display_classes(d, 1);
            break;
        }
    case CON_QGOODRACE:
        /* This state is no longer valid. Here's some code to GTFO. */
        GET_RACE(d->character) = RACE_HUMAN;
        STATE(d) = CON_QCLASS;
        display_classes(d, 1);
        break;
    case CON_QRACE:
        load_result = interpret_race_selection(*arg);
        sprintf(buf, " Argument: %d, Result: %d", *arg, load_result);
        log(buf);
        if (load_result == RACE_UNDEFINED) {
            write_to_output(
                "\r\n&3Please choose by entering the letter next to the "
                "race of your choice.&0\r\n",
                d);
            send_race_menu(d);
            write_to_output("\r\nRace: ", d);
            return;
        } else
            GET_RACE(d->character) = load_result;
        STATE(d) = CON_QCLASS;
        display_classes(d, 1);
        break;
    case CON_NAME_CHECK: /* extended cases for arg with the or's RSD */
        switch (yesno_result(arg)) {
        case YESNO_YES:
            sprintf(buf, "\r\nGive me a password for %s: ", GET_NAME(d->character));
            write_to_output(buf, d);
            echo_off(d);
            STATE(d) = CON_NEWPASSWD;
            break;
        default:
            tmp_name[0] = '\0';
            write_to_output("\r\nPlease enter a different name: ", d);
            clear_player_name(d->character);
            STATE(d) = CON_GET_NAME;
            break;
        }
        break;
    case CON_QCLASS:
        /*
           load_result = parse_class(NULL, d->character, *arg);
           if (load_result == CLASS_UNDEFINED) {
           write_to_output("\r\nInvalid selection.\r\nClass: ", d);
           return;
           } else
           GET_CLASS(d->character) = load_result;
         */
        switch (*arg) {
        case 'w':
            if (class_ok_race[(int)GET_RACE(d->character)][CLASS_WARRIOR])
                load_result = CLASS_WARRIOR;
            else {
                write_to_output("\r\nInvalid selection.\r\nClass: ", d);
                return;
            }
            break;
        case 'c':
            if (class_ok_race[(int)GET_RACE(d->character)][CLASS_CLERIC])
                load_result = CLASS_CLERIC;
            else {
                write_to_output("\r\nInvalid selection.\r\nClass: ", d);
                return;
            }
            break;
        case 's':
            if (class_ok_race[(int)GET_RACE(d->character)][CLASS_SORCERER])
                load_result = CLASS_SORCERER;
            else {
                write_to_output("\r\nInvalid selection.\r\nClass: ", d);
                return;
            }
            break;
        case 'r':
            if (class_ok_race[(int)GET_RACE(d->character)][CLASS_ROGUE])
                load_result = CLASS_ROGUE;
            else {
                write_to_output("\r\nInvalid selection.\r\nClass: ", d);
                return;
            }
            break;
            /*   case 'h':
               if (class_ok_race[(int)GET_RACE(d->character)][CLASS_SHAMAN])
               load_result = CLASS_SHAMAN;
               else {
               write_to_output("\r\nInvalid selection.\r\nClass: ", d);
               return;
               }
               break; */
            /*    case '?':
                  sprintf(buf2, "Class Help Menu\r\n-=-=-=-=-=-=-=-\r\n");
                  for(i=0;i<NUM_CLASSES;i++)
                    sprintf(buf2, "%s%s\r\n", buf2, class_display[i]);
                  sprintf(buf2, "%s0) Back to Class Selection\r\n\r\nSelection:
               ",buf2); page_string(d, buf2); STATE(d) = CON_CLASSHELP; return; */
        default:
            write_to_output("\r\nInvalid selection.\r\nClass: ", d);
            return;
        }
        GET_CLASS(d->character) = load_result;

        /* Hometown selection disabled. Here is the code for when you
         * DON'T ask about a hometown: */

        GET_HOMEROOM(d->character) = classes[(int)GET_CLASS(d->character)].homeroom;

        write_to_output("\r\nPlease press ENTER to roll your attributes: ", d);
        STATE(d) = CON_QROLLSTATS;

        /* This is the place we'd ask about hometown selection. */
        /* (code to display choices) */
        /* STATE(d) = CON_QHOMETOWN; */
        break;

    case CON_CLASSHELP:
        /* This state is not currently entered.  It would be useful.  This
         * would be a nice place to describe the classes that are being offered
         * for starting players. */

        /* Here is some code to gracefully exit this state if it gets entered
         * by mistake. */
        display_classes(d, 1);
        STATE(d) = CON_QCLASS;
        break;

    case CON_QHOMETOWN:
        /* Parse player's hometown choice */
        /* Attempt to set the player's hometown */

        /* The next step could be deity selection. */
        /* (display deity choices) */
        /* STATE(d) = CON_QDIETY; */

        /* Here is some code to gracefully exit this state if it gets entered
         * by mistake. */
        write_to_output("\r\nPlease press ENTER to roll your attributes: ", d);
        STATE(d) = CON_QROLLSTATS;
        break;

    case CON_QROLLSTATS:

        switch (*arg) {
        case 'y':
        case 'Y':
            if ((GET_NATURAL_STR(d->character)) > 0)
                break;
        case 'n':
        case 'N':
        default:

            roll_natural_abils(d->character);
            new_rollor_display(d->character, roll_table);
            sprintf(buf,
                    "\r\n\r        Con:  %s                Wis:  %s\r\n"
                    "        Str:  %s                Intel:%s\r\n"
                    "        Dex:  %s                Char: %s\r\r\r\r\n",
                    rolls_abils_result[roll_table[4]], rolls_abils_result[roll_table[2]],
                    rolls_abils_result[roll_table[0]], rolls_abils_result[roll_table[1]],
                    rolls_abils_result[roll_table[3]], rolls_abils_result[roll_table[5]]);
            write_to_output(buf, d);
            write_to_output(
                "\r\n\nYou may keep these stats if you wish (&0&6Enter "
                "y&0),\r\nor if you wish"
                " you may try for better stats (&0&6Enter n&0)(y/n):",
                d);
            return;
        }

        write_to_output(
            "\r\r\n\n&0&7&bYou have three bonus's to use choose the "
            "stat carefully:&0\r\n",
            d);
        write_to_output(stats_display, d);
        write_to_output("\r\n&0&7&bPlease enter your first bonus selection:&0\r\n", d);
        STATE(d) = CON_QBONUS1;
        break;
    case CON_QBONUS1:
        load_result = bonus_stat(d->character, *arg);
        if (!load_result) {
            write_to_output("&0&1That selection was not offered, please try again&0\r\n\r\n", d);
            return;
        }
        write_to_output(stats_display, d);
        write_to_output("\r\n&0&7&bPlease enter your second bonus selection:&0\r\n", d);

        STATE(d) = CON_QBONUS2;
        break;
    case CON_QBONUS2:
        load_result = bonus_stat(d->character, *arg);
        if (!load_result) {
            write_to_output("&0&1That selection was not offered, please try again&0\r\n\r\n", d);
            return;
        }
        write_to_output(stats_display, d);
        write_to_output("\r\n&0&7&bPlease enter your third bonus selection:&0\r\n", d);

        STATE(d) = CON_QBONUS3;
        break;
    case CON_QBONUS3:
        load_result = bonus_stat(d->character, *arg);

        if (!load_result) {
            write_to_output("&0&1That selection was not offered, please try again&0\r\n\r\n", d);
            return;
        }

        d->character->actual_abils = d->character->natural_abils;
        scale_attribs(d->character);

        write_to_output(
            "&0&4\r\n\r\nRolling for this character is complete!!\r\n"
            "&0&4&bDo you wish to keep this character (Y/n)?\r\n"
            "(Answering 'n' will take you back to gender selection.)&0  ",
            d);
        STATE(d) = CON_QCANCHAR;
        break;
    case CON_QCANCHAR:
        switch (yesno_result(arg)) {
        case YESNO_NO:
            /* They didn't want to keep this character */
            GET_NATURAL_STR(d->character) = 0;
            for (i = 0; i < 6; i++) {
                GET_ROLL(d->character, i) = 0;
            }
            write_to_output("\r\nWhat is your sex (M/F)? ", d);
            STATE(d) = CON_QSEX;
            break;
        case YESNO_OTHER:
            write_to_output("\r\nPlease answer yes or no.\r\n", d);
            write_to_output(
                "&0&4&bDo you wish to keep this character (Y/n)?\r\n"
                "(Answering 'n' will take you back to gender selection.)&0  ",
                d);
            break;
        default:
            if (approve_names && (top_of_p_table + 1) && napprove_pause) {
                if (!PLR_FLAGGED(d->character, PLR_NAPPROVE)) {
                    SET_FLAG(PLR_FLAGS(d->character), PLR_NAPPROVE);
                }
                event_create(EVENT_NAME_TIMEOUT, name_timeout, d, FALSE, NULL, NAME_TIMEOUT);
                REMOVE_FLAG(PLR_FLAGS(d->character), PLR_NEWNAME);
                write_to_output(
                    "\r\nNow you must wait for your name to be approved by "
                    "an immortal.\r\n"
                    "If no one is available, you will be auto approved in "
                    "a short time.\r\n",
                    d);
                broadcast_name(GET_NAME(d->character));
                STATE(d) = CON_NAME_WAIT_APPROVAL;
                break;
            } else {
                if (approve_names && !PLR_FLAGGED(d->character, PLR_NAPPROVE)) {
                    SET_FLAG(PLR_FLAGS(d->character), PLR_NAPPROVE);
                }
                if (GET_PFILEPOS(d->character) < 0) {
                    GET_PFILEPOS(d->character) = create_player_index_entry(GET_NAME(d->character));
                }
                init_player(d->character);
                save_player_char(d->character);
                sprintf(buf, "%s [%s] new player.", GET_NAME(d->character), d->host);
                mudlog(buf, NRM, LVL_IMMORT, TRUE);
                write_to_output("\r\n*** PRESS RETURN: ", d);
                STATE(d) = CON_RMOTD;
                break;
            }
        }
        break;
    case CON_RMOTD: /* read CR after printing motd   */
        write_to_output(MENU, d);
        STATE(d) = CON_MENU;
        break;

    case CON_MENU: /* get selection from main menu  */
        switch (*arg) {
        case '0':
            save_player_char(d->character);
            close_socket(d);
            break;
        case '1':

            load_result = enter_player_game(d);
            send_to_char(WELC_MESSG, d->character);
            act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);

            STATE(d) = CON_PLAYING;
            if (!GET_LEVEL(d->character)) {
                start_player(d->character);
                send_to_char(START_MESSG, d->character);
                give_newbie_eq(d->character);
            }
            look_at_room(d->character, FALSE);
            if (has_mail(GET_IDNUM(d->character)))
                send_to_char("You have mail waiting.\r\n", d->character);
            if (load_result == 2) { /* rented items lost */
                send_to_char(
                    "\r\n\007You could not afford your rent!\r\n"
                    "Your possesions have been donated to the Salvation Army!\r\n",
                    d->character);
            }
            personal_reboot_warning(d->character);
            d->prompt_mode = 1;
            break;

        case '2':
            page_string_desc(d, get_text(TEXT_BACKGROUND));
            STATE(d) = CON_RMOTD;
            break;

        case '3':
            write_to_output("\r\nEnter your old password: ", d);
            echo_off(d);
            STATE(d) = CON_CHPWD_GETOLD;
            break;
            /*    case '5':
                  write_to_output("\r\n This has been temporarily removed.\r\n", d);
                  write_to_output(MENU, d);
                  STATE(d) = CON_MENU; */
            /*      write_to_output("\r\nEnter your password for verification: ", d);
               echo_off(d);
               STATE(d) = CON_DELCNF1; */
            /*      break; */
        default:
            write_to_output("\r\nUnknown menu option.\r\n", d);
            write_to_output(MENU, d);
            break;
        }

        break;

    case CON_CHPWD_GETOLD:
        if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
            echo_on(d);
            write_to_output("\r\nIncorrect password.\r\n", d);
            write_to_output(MENU, d);
            STATE(d) = CON_MENU;
            return;
        } else {
            write_to_output("\r\nEnter a new password: ", d);
            STATE(d) = CON_CHPWD_GETNEW;
            return;
        }
        break;

    case CON_DELCNF1:
        echo_on(d);
        if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
            write_to_output("\r\nIncorrect password.\r\n", d);
            write_to_output(MENU, d);
            STATE(d) = CON_MENU;
        } else {
            write_to_output(
                "\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
                "ARE YOU ABSOLUTELY SURE?\r\n\r\n"
                "Please type \"yes\" to confirm: ",
                d);
            STATE(d) = CON_DELCNF2;
        }
        break;

    case CON_DELCNF2:
        if (!str_cmp(arg, "yes")) {
            if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
                write_to_output("You try to kill yourself, but the ice stops you.\r\n", d);
                write_to_output("Character not deleted.\r\n\r\n", d);
                STATE(d) = CON_CLOSE;
                return;
            }
            if (GET_CLAN_MEMBERSHIP(d->character))
                revoke_clan_membership(GET_CLAN_MEMBERSHIP(d->character));

            if ((player_i = get_ptable_by_name(GET_NAME(d->character))) >= 0) {
                SET_BIT(player_table[player_i].flags, PINDEX_DELETED);
                delete_player(player_i);
            }
            sprintf(buf,
                    "Character '%s' deleted!\r\n"
                    "Goodbye.\r\n",
                    GET_NAME(d->character));
            write_to_output(buf, d);
            sprintf(buf, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GET_LEVEL(d->character));
            mudlog(buf, NRM, LVL_GOD, TRUE);
            STATE(d) = CON_CLOSE;
            return;
        } else {
            write_to_output("\r\nCharacter not deleted.\r\n", d);
            write_to_output(MENU, d);
            STATE(d) = CON_MENU;
        }
        break;

    case CON_CLOSE:
        close_socket(d);
        break;

    default:
        log("SYSERR: Nanny: illegal state of con'ness; closing connection");
        close_socket(d);
        break;
    }
}

/* now hardcoded 25% loss every time you die */
long exp_death_loss(struct char_data *ch, int level) {
    long total;

    total = (long)exp_next_level(level, GET_CLASS(ch)) - exp_next_level((level - 1), GET_CLASS(ch));

    return (.25 * (float)total); /* percent you lose every time you die */
}

long max_exp_gain(struct char_data *ch) {
    long current, total;
    total = exp_next_level(GET_LEVEL(ch), GET_CLASS(ch)) - exp_next_level((GET_LEVEL(ch) - 1), GET_CLASS(ch));

    current = (long)(0.2 * (float)total);
    /*sprintf(buf, "max exp gain is %ld, but total %ld, percent 20\r\n", current,
       total); send_to_char(buf, ch);
     */
    return current;
}

void new_rollor_display(struct char_data *ch, int word[6]) {
    int statts[6];
    int j;

    statts[0] = GET_NATURAL_STR(ch);
    statts[1] = GET_NATURAL_INT(ch);
    statts[2] = GET_NATURAL_WIS(ch);
    statts[3] = GET_NATURAL_DEX(ch);
    statts[4] = GET_NATURAL_CON(ch);
    statts[5] = GET_NATURAL_CHA(ch);

    for (j = 0; j <= 5; j++) {
        if (statts[j] > 90)
            word[j] = 0;
        else if (statts[j] > 80)
            word[j] = 1;
        else if (statts[j] > 62)
            word[j] = 2;
        else if (statts[j] > 52)
            word[j] = 3;
        else
            word[j] = 4;
    }
}

/* this addes a number from 2-6 to the stat but doesn't exceed 100 */
int bonus_stat(struct char_data *ch, char arg) {
    int b;
    int a;
    arg = LOWER(arg);
    switch (arg) {
    case 'w':
        b = number(2, 6);
        GET_NATURAL_WIS(ch) = MIN(100, (GET_NATURAL_WIS(ch) + b));
        a = TRUE;
        break;
    case 'i':
        b = number(2, 6);
        GET_NATURAL_INT(ch) = MIN(100, (GET_NATURAL_INT(ch) + b));
        a = TRUE;
        break;
    case 'm':
        b = number(2, 6);
        GET_NATURAL_CHA(ch) = MIN(100, (GET_NATURAL_CHA(ch) + b));
        a = TRUE;
        break;
    case 'c':
        b = number(2, 6);
        GET_NATURAL_CON(ch) = MIN(100, (GET_NATURAL_CON(ch) + b));
        a = TRUE;
        break;
    case 'd':
        b = number(2, 6);
        GET_NATURAL_DEX(ch) = MIN(100, (GET_NATURAL_DEX(ch) + b));
        a = TRUE;
        break;
    case 's':
        b = number(2, 6);
        GET_NATURAL_STR(ch) = MIN(100, (GET_NATURAL_STR(ch) + b));
        a = TRUE;
        break;
    default:
        a = FALSE;
        break;
    }
    return a;
}

EVENTFUNC(name_timeout) {
    struct descriptor_data *d = (struct descriptor_data *)event_obj;

    if (STATE(d) != CON_NAME_WAIT_APPROVAL)
        return EVENT_FINISHED;

    if (d->character->desc == NULL)
        return EVENT_FINISHED;

    if (GET_PFILEPOS(d->character) < 0)
        GET_PFILEPOS(d->character) = create_player_index_entry(GET_NAME(d->character));

    if (!PLR_FLAGGED(d->character, PLR_NEWNAME))
        init_player(d->character);

    save_player_char(d->character);

    if (!PLR_FLAGGED(d->character, PLR_NAPPROVE))
        SET_FLAG(PLR_FLAGS(d->character), PLR_NAPPROVE);

    if (!PLR_FLAGGED(d->character, PLR_NEWNAME)) {
        /* This IF was placed here so players whose names had been
           FREAKING DECLINED wouldn't sneak online anyway if they
           sat at the prompt not choosing a new name. - RSD 11/11/2000
         */
        write_to_output("You have been auto-approved.\r\n", d);
        write_to_output(get_text(TEXT_MOTD), d);
        write_to_output("\r\n\n*** PRESS RETURN: ", d);
        if (PLR_FLAGGED(d->character, PLR_NEWNAME)) {
            sprintf(buf, "%s [%s] has connected with a new name.", GET_NAME(d->character), d->host);
            mudlog(buf, NRM, LVL_IMMORT, TRUE);
            REMOVE_FLAG(PLR_FLAGS(d->character), PLR_NEWNAME);
        } else {
            sprintf(buf, "%s [%s] new player.", GET_NAME(d->character), d->host);
            mudlog(buf, NRM, LVL_IMMORT, TRUE);
        }
        STATE(d) = CON_RMOTD;
    }
    if (PLR_FLAGGED(d->character, PLR_NEWNAME))
        REMOVE_FLAG(PLR_FLAGS(d->character), PLR_NEWNAME);

    return EVENT_FINISHED;
}

void sort_commands(void) {
    int a, b, tmp;

    /* global variable */
    num_of_cmds = 0;

    /*
     * first, count commands.  num_of_cmds includes the 'reserved'
     * command, which is not copied to the cmd_sort_info array
     */
    while (*cmd_info[num_of_cmds].command != '\n')
        ++num_of_cmds;

    /* create data array */
    CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

    /* initialize it */
    for (a = 1; a < num_of_cmds; a++) {
        cmd_sort_info[a].sort_pos = a;
        cmd_sort_info[a].is_social = (cmd_info[a].command_pointer == do_action);
    }

    /* the infernal special case */
    cmd_sort_info[find_command("insult")].is_social = TRUE;
    cmd_sort_info[find_command("roar")].is_social = TRUE;
    cmd_sort_info[find_command("z001#@#")].is_social = FALSE;

    /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
    for (a = 1; a < num_of_cmds - 1; a++)
        for (b = a + 1; b < num_of_cmds; b++)
            if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command, cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
                tmp = cmd_sort_info[a].sort_pos;
                cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
                cmd_sort_info[b].sort_pos = tmp;
            }
}
