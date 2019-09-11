/***************************************************************************
 *  File: players.c                                       Part of FieryMUD *
 *  Usage: Player loading/saving and utility routines.                     *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "players.h"
#include "dg_scripts.h"
#include "comm.h"
#include "interpreter.h"
#include "quest.h"
#include "math.h"
#include "chars.h"
#include "olc.h"
#include "class.h"
#include "races.h"
#include "clan.h"
#include "skills.h"
#include "constants.h"
#include "casting.h"
#include "trophy.h"
#include "pfiles.h"
#include "privileges.h"
#include "composition.h"
#include "charsize.h"
#include "money.h"
#include "screen.h"
#include "cooldowns.h"

/* local functions */
static void load_effects(FILE *fl, struct char_data *ch);
static void load_skills(FILE *fl, struct char_data *ch);
static void scan_slash(const char *line, int *cur, int *max);
static void write_aliases_ascii(FILE *file, struct char_data *ch);
static void read_aliases_ascii(FILE *file, struct char_data *ch);
static void load_spell_mem(FILE *file, struct char_data *ch);
static void load_cooldowns(FILE *fl, struct char_data *ch);
static void load_coins(char *line, int coins[]);
static void load_clan(char *line, struct char_data *ch);

/*
 * These are the cooldowns that are saved in player files.  End this 
 * list with a -1.
 */
static int saved_cooldowns[] = {
  CD_SUMMON_MOUNT,
  CD_LAY_HANDS,
  CD_FIRST_AID,
  CD_THROATCUT,
  CD_SHAPECHANGE,
  CD_CHANT,
  CD_INNATE_INVISIBLE,
  CD_INNATE_STRENGTH,
  CD_INNATE_DARKNESS,
  CD_INNATE_LEVITATE,
  -1
};

static char *quit_reenter_message[NUM_QUITTYPES] = {
   "%s reenters the game in %s.",
   "%s un-rents in %s.",
   "%s un-cryos in %s.",
   "%s returns from a voidout in %s.",
   "%s pops out after the hotboot in %s.",
   "%s reenters the game in %s.",
   "%s reenters the game in %s.",
   "%s breaks camp in %s.",
   "%s un-rents in %s.",
   "%s is reincorporated at %s.",
   "%s enters the game from a save-point at %s."
};

static char *quit_statement[NUM_QUITTYPES] = {
   "%s left for unknown reasons in %s.",
   "%s rented in %s.",
   "%s cryo'd in %s.",
   "%s voided out in %s.",
   "%s was tucked away for a hotboot in %s.",
   "%s quit in %s.",
   "%s quit in %s.",
   "%s camped in %s.",
   "%s was rented by a trigger in %s.",
   "%s was &1&bpurged&0 in %s.",
   "%s was saved at %s."
};

int get_pfilename(const char *name, char *filename, int mode)
{
  const char *prefix, *suffix;

  if (!name || !*name)
    return 0;

  switch (mode) {
  case OBJ_FILE:
    prefix = PLR_PREFIX;
    suffix = POBJ_SUFFIX;
    break;
  case QUEST_FILE:
    prefix = PLR_PREFIX;
    suffix = PQUEST_SUFFIX;
    break;
  case PLR_FILE:
    prefix = PLR_PREFIX;
    suffix = PLR_SUFFIX;
    break;
  case NOTES_FILE:
    prefix = PLR_PREFIX;
    suffix = PNOTES_SUFFIX;
    break;
  case TEMP_FILE:
    prefix = PLR_PREFIX;
    suffix = PTEMP_SUFFIX;
    break;
  default:
    return 0;
  }

  sprintf(filename, "%s/%c/%c%s%s", prefix, UPPER(*name), UPPER(*name),
          name + 1, suffix);
  return 1;

}

/* New version to build player index for ASCII Player Files. Generate index 
 * table for the player file. */
void build_player_index(void)
{
  int rec_count = 0, i;
  FILE *plr_index;
  char index_name[40], line[256], bits[65];
  char name[80];

  sprintf(index_name, "%s/%s", PLR_PREFIX, INDEX_FILE);
  if (!(plr_index = fopen(index_name, "r"))) {
    top_of_p_table = -1;
    log("No player index file!  First new char will be IMP!");
    return;
  }

  while (get_line(plr_index, line))
    if (*line != '~')
      rec_count++;
  rewind(plr_index);

  if (rec_count == 0) {
    player_table = NULL;
    top_of_p_table = -1;
    return;
  }

  CREATE(player_table, struct player_index_element, rec_count);
  for (i = 0; i < rec_count; i++) {
    get_line(plr_index, line);
    sscanf(line, "%ld %s %d %s %ld", &player_table[i].id, name,
      &player_table[i].level, bits, (long *)&player_table[i].last);
    player_table[i].name = strdup(name);
    player_table[i].flags = asciiflag_conv(bits);
    top_idnum = MAX(top_idnum, player_table[i].id);
  }

  fclose(plr_index);
  top_of_p_file = top_of_p_table = i - 1;
}

/* Create a new entry in the in-memory index table for the player file. If the 
 * name already exists, by overwriting a deleted character, then we re-use the 
 * old position. */
int create_player_index_entry(char *name)
{
  int i, pos;

  if (top_of_p_table == -1) {	/* no table */
    pos = top_of_p_table = 0;
    CREATE(player_table, struct player_index_element, 1);
  } else if ((pos = get_ptable_by_name(name)) == -1) {	/* new name */
    i = ++top_of_p_table + 1;

    RECREATE(player_table, struct player_index_element, i);
    pos = top_of_p_table;
  }

  CREATE(player_table[pos].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field, cap first char */
  *player_table[pos].name = UPPER(*name);
  for (i = 1; (player_table[pos].name[i] = LOWER(name[i])); ++i);
  player_table[pos].name[i] = '\0';

  /* clear the bitflag in case we have garbage data */
  player_table[pos].flags = 0;

  return (pos);
}

/* This function necessary to save a seperate ASCII player index */
void save_player_index(void)
{
  int i;
  char index_name[50], bits[64];
  FILE *index_file;

  sprintf(index_name, "%s/%s", PLR_PREFIX, INDEX_FILE);
  if (!(index_file = fopen(index_name, "w"))) {
    log("SYSERR: Could not write player index file");
    return;
  }

  for (i = 0; i <= top_of_p_table; i++)
    if (*player_table[i].name) {
      sprintascii(bits, player_table[i].flags);
      fprintf(index_file, "%ld %s %d %s %ld\n", player_table[i].id,
	player_table[i].name, player_table[i].level, *bits ? bits : "0",
        (long)player_table[i].last);
    }
  fprintf(index_file, "~\n");

  fclose(index_file);
}

void free_player_index(void)
{
  int tp;

  if (!player_table)
    return;

  for (tp = 0; tp <= top_of_p_table; tp++)
    if (player_table[tp].name)
      free(player_table[tp].name);

  free(player_table);
  player_table = NULL;
  top_of_p_table = 0;
}

long get_ptable_by_name(const char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, name))
      return (i);

  return (-1);
}

long get_id_by_name(const char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, name))
      return (player_table[i].id);

  return (-1);
}

char *get_name_by_id(long id)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == id)
      return (player_table[i].name && *player_table[i].name ?
              player_table[i].name : NULL);

  return (NULL);
}

/* Stuff related to the save/load player system. */
/* New load_char reads ASCII Player Files. Load a char, TRUE if loaded, FALSE
 * if not. */
int load_player(const char *name, struct char_data *ch)
{
  int id, i, num;
  FILE *fl;
  char fname[40];
  char buf[MAX_INPUT_LENGTH], line[MAX_INPUT_LENGTH + 1], tag[128];
  bool found_damroll = FALSE;
  bool found_hitroll = FALSE;

  extern int mortal_start_room;
  extern void do_wiztitle(char *outbuf, struct char_data *vict, char *argument);

  if ((id = get_ptable_by_name(name)) < 0)
    return (-1);

  if (!get_pfilename(player_table[id].name, fname, PLR_FILE))
    return (-1);

  /* Return quietly if the file does not exist. */
  num = access(fname, R_OK);
  if (num & ENOENT) return -1;

  if (!(fl = fopen(fname, "r"))) {
    sprintf(buf, "SYSERR: Couldn't open player file %s", fname);
    mudlog(buf, NRM, LVL_GOD, TRUE);
    return (-1);
  }

  if (!ch->player_specials)
    CREATE(ch->player_specials, struct player_special_data, 1);

  GET_PFILEPOS(ch) = id;

  /* Character initializations. Necessary to keep some things straight. */
  ch->effects = NULL;
  for (i = 1; i <= TOP_SKILL; i++)
    SET_SKILL(ch, i, 0);
  GET_LOADROOM(ch) = mortal_start_room;
  GET_COND(ch, FULL) = PFDEF_HUNGER;
  GET_COND(ch, THIRST) = PFDEF_THIRST;
  GET_COND(ch, DRUNK) = PFDEF_DRUNK;
  GET_PLATINUM(ch) = PFDEF_COINS;
  GET_GOLD(ch) = PFDEF_COINS;
  GET_SILVER(ch) = PFDEF_COINS;
  GET_COPPER(ch) = PFDEF_COINS;
  GET_BANK_PLATINUM(ch) = PFDEF_BANK;
  GET_BANK_GOLD(ch) = PFDEF_BANK;
  GET_BANK_SILVER(ch) = PFDEF_BANK;
  GET_BANK_COPPER(ch) = PFDEF_BANK;
  GET_PAGE_LENGTH(ch) = DEFAULT_PAGE_LENGTH;
  GET_AUTOINVIS(ch) = -1;
  ch->player.time.logon = time(0);

  init_trophy(ch);

  while (get_line(fl, line)) {
    tag_argument(line, tag);
    num = atoi(line);

    switch (toupper(*tag)) {
    case 'A':
      if (!strcmp(tag, "ac"))			GET_AC(ch) = num;
      else if (!strcmp(tag, "alignment"))	GET_ALIGNMENT(ch) = LIMIT(-1000, num, 1000);
      else if (!strcmp(tag, "aliases"))		read_aliases_ascii(fl, ch);
      else if (!strcmp(tag, "aggression"))	GET_AGGR_LEV(ch) = num;
      else if (!strcmp(tag, "autoinvis"))	GET_AUTOINVIS(ch) = num;
      else goto bad_tag;
      break;

    case 'B':
           if (!strcmp(tag, "badpasswords"))	GET_BAD_PWS(ch) = num;
      else if (!strcmp(tag, "bank"))		load_coins(line, GET_BANK_COINS(ch));
      else if (!strcmp(tag, "birthtime"))	ch->player.time.birth = atol(line);
      else if (!strcmp(tag, "base_height"))	ch->player.base_height = num;
      else if (!strcmp(tag, "base_weight"))	ch->player.base_weight = num;
      else if (!strcmp(tag, "base_size"))	ch->player.base_size = LIMIT(0, num, NUM_SIZES - 1);
      else goto bad_tag;
      break;

    case 'C':
           if (!strcmp(tag, "charisma"))	GET_NATURAL_CHA(ch) = num;
      else if (!strcmp(tag, "class"))		GET_CLASS(ch) = LIMIT(0, num, NUM_CLASSES - 1);
      else if (!strcmp(tag, "constitution"))	GET_NATURAL_CON(ch) = num;
      else if (!strcmp(tag, "cash"))		load_coins(line, GET_COINS(ch));
      else if (!strcmp(tag, "clan"))		load_clan(line, ch);
      else if (!strcmp(tag, "currenttitle"))	GET_TITLE(ch) = strdup(line);
      else if (!strcmp(tag, "composition"))	BASE_COMPOSITION(ch) = num;
      else if (!strcmp(tag, "cooldowns"))	load_cooldowns(fl, ch);
      else goto bad_tag;
      break;

    case 'D':
           if (!strcmp(tag, "description"))	ch->player.description = fread_string(fl, "load_player");
      else if (!strcmp(tag, "dexterity"))	GET_NATURAL_DEX(ch) = num;
      else if (!strcmp(tag, "drunkenness"))	GET_COND(ch, DRUNK) = LIMIT(-1, num, 24);
      else if (!strcmp(tag, "damroll")) {
        GET_BASE_DAMROLL(ch) = num;
        found_damroll = TRUE;
      }
      else goto bad_tag;
      break;

    case 'E':
           if (!strcmp(tag, "experience"))	GET_EXP(ch) = atol(line);
      else if (!strcmp(tag, "effectflags"))	load_ascii_flags(EFF_FLAGS(ch), NUM_EFF_FLAGS, line);
      else if (!strcmp(tag, "effects"))		load_effects(fl, ch);
      else goto bad_tag;
      break;

    case 'F':
           if (!strcmp(tag, "freezelevel"))	GET_FREEZE_LEV(ch) = LIMIT(0, num, LVL_IMPL);
      else goto bad_tag;
      break;

    case 'G':
           if (!strcmp(tag, "grants"))		read_player_grants(fl, &GET_GRANTS(ch));
      else if (!strcmp(tag, "grantgroups"))	read_player_grant_groups(fl, &GET_GRANT_GROUPS(ch));
      else goto bad_tag;
      break;

    case 'H':
           if (!strcmp(tag, "hitpoints"))	scan_slash(line, &GET_HIT(ch), &GET_BASE_HIT(ch));
      else if (!strcmp(tag, "height"))		GET_HEIGHT(ch) = num;
      else if (!strcmp(tag, "host")) { 
        if (GET_HOST(ch)) 
          free(GET_HOST(ch)); 
        GET_HOST(ch) = strdup(line); 
      }
      else if (!strcmp(tag, "hunger"))		GET_COND(ch, FULL) = LIMIT(-1, num, 24);
      else if (!strcmp(tag, "home"))		GET_HOMEROOM(ch) = num;
      else if (!strcmp(tag, "hitroll")) {
        GET_BASE_HITROLL(ch) = num;
        found_hitroll = TRUE;
      }
      else goto bad_tag;
      break;

    case 'I':
           if (!strcmp(tag, "id"))		GET_IDNUM(ch) = atol(line);
      else if (!strcmp(tag, "intelligence"))	GET_NATURAL_INT(ch) = num;
      else if (!strcmp(tag, "invislevel"))	GET_INVIS_LEV(ch) = LIMIT(0, num, LVL_IMPL);
      else goto bad_tag;
      break;

    case 'L':
           if (!strcmp(tag, "level"))		GET_LEVEL(ch) = LIMIT(0, num, LVL_IMPL);
      else if (!strcmp(tag, "lastlevel"))	GET_LASTLEVEL(ch) = num;
      else if (!strcmp(tag, "lastlogintime"))	ch->player.time.logon = num;
      else if (!strcmp(tag, "lifeforce"))	GET_LIFEFORCE(ch) = num;
      else if (!strcmp(tag, "loadroom"))	GET_LOADROOM(ch) = num;
      else if (!strcmp(tag, "logview"))		GET_LOG_VIEW(ch) = num;
      else goto bad_tag;
      break;

    case 'M':
           if (!strcmp(tag, "mana"))		scan_slash(line, &GET_MANA(ch), &GET_MAX_MANA(ch));
      else if (!strcmp(tag, "move"))		scan_slash(line, &GET_MOVE(ch), &GET_MAX_MOVE(ch));
      else if (!strcmp(tag, "mem"))		load_spell_mem(fl, ch);
      else goto bad_tag;
      break;

    case 'N':
           if (!strcmp(tag, "name")) {
              GET_NAME(ch) = strdup(line);
              GET_NAMELIST(ch) = strdup(line);
           }
      else if (!strcmp(tag, "natural_size"))	ch->player.natural_size = LIMIT(0, num, NUM_SIZES - 1);
      else goto bad_tag;
      break;

    case 'O':
           if (!strcmp(tag, "olczones")) {
        struct olc_zone_list *zone;
        char *next = line;
        while ((next = any_one_arg(next, buf)) && is_number(buf)) {
          CREATE(zone, struct olc_zone_list, 1);
          zone->zone = atoi(buf);
          zone->next = GET_OLC_ZONES(ch);
          GET_OLC_ZONES(ch) = zone;
        }
      }
      else goto bad_tag;
      break;

    case 'P':
           if (!strcmp(tag, "pagelength"))	GET_PAGE_LENGTH(ch) = LIMIT(5, num, 250);
      else if (!strcmp(tag, "password"))	strcpy(GET_PASSWD(ch), line);
      else if (!strcmp(tag, "playerflags"))	load_ascii_flags(PLR_FLAGS(ch), NUM_PLR_FLAGS, line);
      else if (!strcmp(tag, "poofin"))		GET_POOFIN(ch) = strdup(line);
      else if (!strcmp(tag, "poofout"))		GET_POOFOUT(ch) = strdup(line);
      else if (!strcmp(tag, "prefflags"))	load_ascii_flags(PRF_FLAGS(ch), NUM_PRF_FLAGS, line);
      else if (!strcmp(tag, "privflags"))	load_ascii_flags(PRV_FLAGS(ch), NUM_PRV_FLAGS, line);
      else if (!strcmp(tag, "prompt"))		GET_PROMPT(ch) = strdup(line);
      else goto bad_tag;
      break;

    case 'R':
           if (!strcmp(tag, "race"))		GET_RACE(ch) = LIMIT(0, num, NUM_RACES - 1);
      else goto bad_tag;
      break;

    case 'S':
      if (!strcmp(tag, "savingthrows"))	{
        char *next = line;
        i = 0;
        while (*(next = any_one_arg(next, buf)) && i < NUM_SAVES)
          GET_SAVE(ch, i++) = atoi(buf);
      }
      else if (!strcmp(tag, "saveroom"))	GET_SAVEROOM(ch) = num;
      else if (!strcmp(tag, "sex"))		GET_SEX(ch) = LIMIT(0, num, NUM_SEXES - 1);
      /* "size" is a holdover which meant the same thing as "base_size" */
      else if (!strcmp(tag, "size"))		ch->player.base_size = LIMIT(0, num, NUM_SIZES - 1);
      else if (!strcmp(tag, "skills"))		load_skills(fl, ch);
      else if (!strcmp(tag, "strength"))	GET_NATURAL_STR(ch) = num;
      else goto bad_tag;
      break;

    case 'Q':
      if (!strcmp(tag, "quit_reason")) GET_QUIT_REASON(ch) = num;
      break;

    case 'T':
           if (!strcmp(tag, "thirst"))		GET_COND(ch, THIRST) = LIMIT(-1, num, 24);
      else if (!strcmp(tag, "title"))		add_perm_title(ch, line);
      else if (!strcmp(tag, "timeplayed"))	ch->player.time.played = num;
      else if (!strcmp(tag, "trophy"))		load_trophy(fl, ch);
      else goto bad_tag;
      break;

    case 'U':
           if (!strcmp(tag, "revokes"))	read_player_grants(fl, &GET_REVOKES(ch));
      else if (!strcmp(tag, "revokegroups"))	read_player_grant_groups(fl, &GET_REVOKE_GROUPS(ch));
      else goto bad_tag;
      break;

    case 'W':
           if (!strcmp(tag, "weight"))		GET_WEIGHT(ch) = num;
      else if (!strcmp(tag, "wimpy"))		GET_WIMP_LEV(ch) = LIMIT(0, num, LVL_IMPL);
      else if (!strcmp(tag, "wisdom"))		GET_NATURAL_WIS(ch) = num;
      else if (!strcmp(tag, "wiztitle"))	do_wiztitle(buf, ch, line);
      else goto bad_tag;
      break;

    default:
    bad_tag:
      sprintf(buf, "SYSERR: Unknown tag %s in pfile %s: %s", tag, name, line);
      log(buf);
    }
  }

  /* Old pfiles don't have base damroll and hitroll set */
  if (VALID_RACE(ch)) {
    if (!found_damroll)
      GET_BASE_DAMROLL(ch) = races[(int) GET_RACE(ch)].bonus_damroll;
    if (!found_hitroll)
      GET_BASE_HITROLL(ch) = races[(int) GET_RACE(ch)].bonus_hitroll;
  }

  /* Remove some unwanted flags */
  REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
  REMOVE_FLAG(PLR_FLAGS(ch), PLR_REMOVING);
  REMOVE_FLAG(PLR_FLAGS(ch), PLR_SAVING);
  REMOVE_FLAG(EFF_FLAGS(ch), EFF_ANIMATED);
  REMOVE_FLAG(EFF_FLAGS(ch), EFF_SHADOWING);

  if (!GET_PAGE_LENGTH(ch))
    GET_PAGE_LENGTH(ch) = DEFAULT_PAGE_LENGTH;

  /* Cache grants */
  cache_grants(ch);

  GET_MAX_HIT(ch) = GET_BASE_HIT(ch);

  effect_total(ch);

  /* initialization for imms */
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (i = 1; i <= TOP_SKILL; i++)
      SET_SKILL(ch, i, 1000);
    GET_COND(ch, FULL) = -1;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = -1;
  }

  /*
   * If you're not poisioned and you've been away for more than an hour of
   * real time, we'll set your HMV back to full.
   *
   * However, note that equipment/spell effects have not yet been applied,
   * so the true maximum may be higher than GET_MAX_XXX might say here.
   * To avoid REDUCING these values from their affected max to their
   * natural max, make sure GET_MAX_XXX is greater than the current value
   * before doing anything.
   */

  if (!EFF_FLAGGED(ch, EFF_POISON) &&
      (((long) (time(0) - ch->player.time.logon)) >= SECS_PER_REAL_HOUR)) {
    if (GET_HIT(ch) < GET_MAX_HIT(ch))
      GET_HIT(ch) = GET_MAX_HIT(ch);
    if (GET_MOVE(ch) < GET_MAX_MOVE(ch))
      GET_MOVE(ch) = GET_MAX_MOVE(ch);
    if (GET_MANA(ch) < GET_MAX_MANA(ch))
      GET_MANA(ch) = GET_MAX_MANA(ch);
  }

  num = (time(0) - ch->player.time.logon) RL_SEC;
  for (i = 0; i < NUM_COOLDOWNS; ++i)
    if (GET_COOLDOWN(ch, i)) {
      if (GET_COOLDOWN(ch, i) < num)
        GET_COOLDOWN(ch, i) = 0;
      else
        GET_COOLDOWN(ch, i) -= num;
    }

  /* Double-check base weight/height/size */
  if (ch->player.base_height == 0 || ch->player.base_weight == 0) {
     ch->player.natural_size = ch->player.base_size;
     ch->player.base_weight = GET_WEIGHT(ch);
     ch->player.base_height = GET_HEIGHT(ch);
  }
  reset_height_weight(ch);

  fclose(fl);
  return(id);
}

/* Write the vital data of a player to the player file. */
/* This is the ASCII Player Files save routine. */
void save_player_char(struct char_data * ch)
{
  FILE *fl;
  char fname[PLAYER_FILENAME_LENGTH], frename[PLAYER_FILENAME_LENGTH];
  int i, id, save_index = FALSE, orig_pos;
  struct effect *eff, tmp_eff[MAX_EFFECT];
  struct obj_data *char_eq[NUM_WEARS];

  if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0) {
    sprintf(buf, "SYSERR: Attempt to save %s (NPC or no PFILEPOS)", GET_NAME(ch));
    log(buf);
    return;
  }

  if (IN_ROOM_VNUM(ch) != NOWHERE)
     GET_SAVEROOM(ch) = IN_ROOM_VNUM(ch);

  /* If ch->desc is not null, then update session data before saving. */
  if (ch->desc) {
    if (ch->desc->host && *ch->desc->host) {
      if (!GET_HOST(ch))
        GET_HOST(ch) = strdup(ch->desc->host);
      else if (GET_HOST(ch) && strcmp(GET_HOST(ch), ch->desc->host)) {
        free(GET_HOST(ch));
        GET_HOST(ch) = strdup(ch->desc->host);
      }
    }

    /* Only update the time.played and time.logon if the character is playing. */
    if (STATE(ch->desc) == CON_PLAYING) {
      ch->player.time.played += time(0) - ch->player.time.logon;
      ch->player.time.logon = time(0);
    }
  }

  if (!get_pfilename(GET_NAME(ch), fname, TEMP_FILE)) {
    sprintf(buf, "SYSERR: Couldn't make file name for saving %s.", GET_NAME(ch));
    log(buf);
    return;
  }

  if (!get_pfilename(GET_NAME(ch), frename, PLR_FILE)) {
    sprintf(buf, "SYSERR: Couldn't make final file name for %s.", GET_NAME(ch));
    log(buf);
    return;
  }

  if (!(fl = fopen(fname, "w"))) {
    sprintf(buf, "SYSERR: Couldn't open player file %s for write", fname);
    mudlog(buf, NRM, LVL_GOD, TRUE);
    return;
  }

  /* As we remove the effects, the player will lose fly even if entitled to it.
   * We'll save the position here so that flying can be restored after the
   * effects are restored. */
  orig_pos = GET_POS(ch);
  /* Stop effect_total from making things happen in game due to effect changes */
  SET_FLAG(PLR_FLAGS(ch), PLR_SAVING);

  /* Unaffect everything a character can be affected by. */
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      char_eq[i] = unequip_char(ch, i);
    else
      char_eq[i] = NULL;
  }

  for (eff = ch->effects, i = 0; i < MAX_EFFECT; i++) {
    if (eff) {
      tmp_eff[i] = *eff;
      tmp_eff[i].next = 0;
      eff = eff->next;
    } else {
      tmp_eff[i].type = 0;	/* Zero signifies not used */
      tmp_eff[i].duration = 0;
      tmp_eff[i].modifier = 0;
      tmp_eff[i].location = 0;
      CLEAR_FLAGS(tmp_eff[i].flags, NUM_EFF_FLAGS);
      tmp_eff[i].next = 0;
    }
  }

  /* Remove the effects so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in. */

  while (ch->effects)
    effect_remove(ch, ch->effects);

  if ((i >= MAX_EFFECT) && eff && eff->next)
    log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  ch->affected_abils = ch->natural_abils;
  /* end char_to_store code */
  fprintf(fl, "name: %s\n", GET_NAME(ch));
  fprintf(fl, "password: %s\n", GET_PASSWD(ch));
  if (GET_TITLE(ch) && *GET_TITLE(ch))
    fprintf(fl, "currenttitle: %s\n", GET_TITLE(ch));
  if (ch->player.description && *ch->player.description)
    fprintf(fl, "description\n%s~\n",
            filter_chars(buf, ch->player.description, "\r~"));
  if (GET_PROMPT(ch) && *GET_PROMPT(ch))
    fprintf(fl, "prompt: %s\n", GET_PROMPT(ch));
  if (GET_POOFIN(ch))
    fprintf(fl, "poofin: %s\n", GET_POOFIN(ch));
  if (GET_POOFOUT(ch))
    fprintf(fl, "poofout: %s\n", GET_POOFOUT(ch));
  if (GET_AUTOINVIS(ch) > -1)
    fprintf(fl, "autoinvis: %d\n", GET_AUTOINVIS(ch));
  fprintf(fl, "sex: %d\n", GET_SEX(ch));
  fprintf(fl, "class: %d\n", GET_CLASS(ch));
  fprintf(fl, "race: %d\n", GET_RACE(ch));
  fprintf(fl, "level: %d\n", GET_LEVEL(ch));
  fprintf(fl, "home: %d\n", GET_HOMEROOM(ch));
  fprintf(fl, "lifeforce: %d\n", GET_LIFEFORCE(ch));
  fprintf(fl, "composition: %d\n", BASE_COMPOSITION(ch));

  fprintf(fl, "id: %ld\n", GET_IDNUM(ch));
  fprintf(fl, "birthtime: %ld\n", (long)ch->player.time.birth);
  fprintf(fl, "timeplayed: %d\n",  ch->player.time.played);
  fprintf(fl, "lastlogintime: %ld\n", (long)ch->player.time.logon);

  if (GET_HOST(ch))
    fprintf(fl, "host: %s\n", GET_HOST(ch));
  fprintf(fl, "height: %d\n", GET_HEIGHT(ch));
  fprintf(fl, "weight: %d\n", GET_WEIGHT(ch));
  fprintf(fl, "base_height: %d\n", ch->player.base_height);
  fprintf(fl, "base_weight: %d\n", ch->player.base_weight);
  fprintf(fl, "base_size: %d\n", ch->player.base_size);
  fprintf(fl, "natural_size: %d\n", ch->player.natural_size);
  fprintf(fl, "alignment: %d\n", GET_ALIGNMENT(ch));

  fprintf(fl, "playerflags: ");
  write_ascii_flags(fl, PLR_FLAGS(ch), NUM_PLR_FLAGS);
  fprintf(fl, "\n");
 
  fprintf(fl, "effectflags: ");
  write_ascii_flags(fl, EFF_FLAGS(ch), NUM_EFF_FLAGS);
  fprintf(fl, "\n");
 
  fprintf(fl, "prefflags: ");
  write_ascii_flags(fl, PRF_FLAGS(ch), NUM_PRF_FLAGS);
  fprintf(fl, "\n");

  fprintf(fl, "privflags: ");
  write_ascii_flags(fl, PRV_FLAGS(ch), NUM_PRV_FLAGS);
  fprintf(fl, "\n");

  fprintf(fl, "savingthrows:");
  for (i = 0; i < NUM_SAVES; ++i)
    fprintf(fl, " %d", GET_SAVE(ch, i));
  fprintf(fl, "\n");

  if (GET_WIMP_LEV(ch))
    fprintf(fl, "wimpy: %d\n", GET_WIMP_LEV(ch));
  if (GET_FREEZE_LEV(ch))
    fprintf(fl, "freezelevel: %d\n", GET_FREEZE_LEV(ch));
  if (GET_INVIS_LEV(ch))
    fprintf(fl, "invislevel: %d\n", GET_INVIS_LEV(ch));
  if (GET_AGGR_LEV(ch))
    fprintf(fl, "aggression: %d\n", GET_AGGR_LEV(ch));
  if (GET_LOADROOM(ch) != NOWHERE)
    fprintf(fl, "loadroom: %d\n", GET_LOADROOM(ch));

  if (GET_BAD_PWS(ch))
    fprintf(fl, "badpasswords: %d\n", GET_BAD_PWS(ch));

  if (GET_COND(ch, FULL) != PFDEF_HUNGER && GET_LEVEL(ch) < LVL_IMMORT)
    fprintf(fl, "hunger: %d\n", GET_COND(ch, FULL));
  if (GET_COND(ch, THIRST) != PFDEF_THIRST && GET_LEVEL(ch) < LVL_IMMORT)
    fprintf(fl, "thirst: %d\n", GET_COND(ch, THIRST));
  if (GET_COND(ch, DRUNK) != PFDEF_DRUNK  && GET_LEVEL(ch) < LVL_IMMORT)
    fprintf(fl, "drunkenness: %d\n", GET_COND(ch, DRUNK));
  fprintf(fl, "lastlevel: %d\n", GET_LASTLEVEL(ch));
  /* Save BASE hit instead of MAX hit, since max is dynamically
   * calculated from base. */
  fprintf(fl, "hitpoints: %d/%d\n", GET_HIT(ch),  GET_BASE_HIT(ch));
  fprintf(fl, "mana: %d/%d\n", GET_MANA(ch), GET_MAX_MANA(ch));
  fprintf(fl, "move: %d/%d\n", GET_MOVE(ch), GET_MAX_MOVE(ch));

  /* Save BASE hitroll and damroll since everything else comes from EQ */
  fprintf(fl, "damroll: %d\n", GET_BASE_DAMROLL(ch));
  fprintf(fl, "hitroll: %d\n", GET_BASE_HITROLL(ch));

  fprintf(fl, "strength: %d\n", GET_NATURAL_STR(ch));
  fprintf(fl, "intelligence: %d\n", GET_NATURAL_INT(ch));
  fprintf(fl, "wisdom: %d\n", GET_NATURAL_WIS(ch));
  fprintf(fl, "dexterity: %d\n", GET_NATURAL_DEX(ch));
  fprintf(fl, "constitution: %d\n", GET_NATURAL_CON(ch));
  fprintf(fl, "charisma: %d\n", GET_NATURAL_CHA(ch));

  /* No need to save AC since it's dynamically calculated. */
  /* fprintf(fl, "ac: %d\n", GET_AC(ch)); */
  if (GET_PLATINUM(ch) != PFDEF_COINS || GET_GOLD(ch) != PFDEF_COINS ||
      GET_SILVER(ch) != PFDEF_COINS || GET_COPPER(ch) != PFDEF_COINS)
    fprintf(fl, "cash: %d %d %d %d\n",
            GET_PLATINUM(ch), GET_GOLD(ch),
            GET_SILVER(ch), GET_COPPER(ch));
  if (GET_BANK_PLATINUM(ch) != PFDEF_BANK ||
      GET_BANK_GOLD(ch) != PFDEF_BANK ||
      GET_BANK_SILVER(ch) != PFDEF_BANK ||
      GET_BANK_COPPER(ch) != PFDEF_BANK)
    fprintf(fl, "bank: %d %d %d %d\n",
            GET_BANK_PLATINUM(ch), GET_BANK_GOLD(ch),
            GET_BANK_SILVER(ch), GET_BANK_COPPER(ch));
  fprintf(fl, "experience: %ld\n", GET_EXP(ch));

  if (GET_OLC_ZONES(ch)) {
    struct olc_zone_list *zone;
    fprintf(fl, "olczones:");
    for (zone = GET_OLC_ZONES(ch); zone; zone = zone->next)
      fprintf(fl, " %d", zone->zone);
    fprintf(fl, "\n");
  }
  if (GET_PAGE_LENGTH(ch) != DEFAULT_PAGE_LENGTH)
    fprintf(fl, "pagelength: %d\n", GET_PAGE_LENGTH(ch));
  if (GET_CLAN(ch))
    fprintf(fl, "clan: %d\n", GET_CLAN(ch)->number);
  if (GET_LOG_VIEW(ch))
    fprintf(fl, "logview: %d\n", GET_LOG_VIEW(ch));

  if (GET_PERM_TITLES(ch))
    for (i = 0; GET_PERM_TITLES(ch)[i]; ++i)
      fprintf(fl, "title: %s\n", GET_PERM_TITLES(ch)[i]);
  if (GET_WIZ_TITLE(ch)) {
    strcpy(buf, GET_WIZ_TITLE(ch));
    trim_spaces(buf);
    fprintf(fl, "wiztitle: %s\n", buf);
  }

  fprintf(fl, "quit_reason: %d\n", GET_QUIT_REASON(ch));
  fprintf(fl, "saveroom: %d\n", GET_SAVEROOM(ch));

  /*
   * Only save cooldowns if there are any that are in the saved
   * list and nonzero.
   */
  for (i = 0; saved_cooldowns[i] >= 0; ++i)
    if (GET_COOLDOWN(ch, saved_cooldowns[i]))
      break;
  if (saved_cooldowns[i] >= 0) {
    fprintf(fl, "cooldowns:\n");
    for (i = 0; saved_cooldowns[i] >= 0; ++i)
      if (GET_COOLDOWN(ch, saved_cooldowns[i]))
        fprintf(fl, "%d %d/%d\n", saved_cooldowns[i],
                GET_COOLDOWN(ch, saved_cooldowns[i]),
                GET_COOLDOWN_MAX(ch, saved_cooldowns[i]));
    fprintf(fl, "-1 0\n");
  }
  
  /* Save skills */
  if (GET_LEVEL(ch) < LVL_IMMORT) {
    fprintf(fl, "skills:\n");
    for (i = 1; i <= TOP_SKILL; i++) {
     if (GET_SKILL(ch, i))
	fprintf(fl, "%d %d\n", i, GET_ISKILL(ch, i));
    }
    fprintf(fl, "0 0\n");
  }

   /* Save trophy */
  save_trophy(fl, ch);

  /* Save effects */
  if (tmp_eff[0].type > 0) {
    fprintf(fl, "effects:\n");
    for (i = 0; i < MAX_EFFECT; i++) {
      eff = &tmp_eff[i];
      if (!eff->type)
        continue;
      if (eff->type == SKILL_BERSERK)
        continue;
      fprintf(fl, "%d %d %d %d %ld %ld %ld\n", eff->type, eff->duration,
              eff->modifier, eff->location, eff->flags[0], eff->flags[1], eff->flags[2]);
    }
    fprintf(fl, "0 0 0 0 0\n");
  }

  if (GET_SPELL_MEM(ch).num_spells) {
    struct mem_list *mem;
    fprintf(fl, "mem:\n");
    for (mem = GET_SPELL_MEM(ch).list_head; mem; mem = mem->next)
      fprintf(fl, "%d %d %d\n", mem->spell, mem->can_cast, mem->mem_time);
    fprintf(fl, "0 0 0\n");
  }

  write_aliases_ascii(fl, ch);

  write_player_grants(fl, ch);

  if (fclose(fl)) {
    sprintf(buf, "SYSERR: Error closing player file for %s after write",
          GET_NAME(ch));
    log(buf);
  } else if (rename(fname, frename)) {
    sprintf(buf, "SYSERR: Error renaming player file for %s after write",
          GET_NAME(ch));
    log(buf);
  }

  /* More char_to_store code to add spell and eq affections back in. */
  for (i = 0; i < MAX_EFFECT; ++i) {
    if (tmp_eff[i].type)
      effect_to_char(ch, &tmp_eff[i]);
  }

  for (i = 0; i < NUM_WEARS; i++)
    if (char_eq[i])
      equip_char(ch, char_eq[i], i);

  /* Add racial and class effects back */
  update_char(ch);

  /* Restore original position. */
  GET_POS(ch) = orig_pos;
  effect_total(ch);

  REMOVE_FLAG(PLR_FLAGS(ch), PLR_SAVING);
  /* end char_to_store code */

  if ((id = get_ptable_by_name(GET_NAME(ch))) < 0)
    return;

  /* update the player in the player index */
  if (player_table[id].level != GET_LEVEL(ch)) {
    save_index = TRUE;
    player_table[id].level = GET_LEVEL(ch);
  }
  if (player_table[id].last != ch->player.time.logon) {
    save_index = TRUE;
    player_table[id].last = ch->player.time.logon;
  }
  i = player_table[id].flags;
  if (PLR_FLAGGED(ch, PLR_DELETED))
    SET_BIT(player_table[id].flags, PINDEX_DELETED);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_DELETED);
  if (PLR_FLAGGED(ch, PLR_NODELETE) || PLR_FLAGGED(ch, PLR_CRYO))
    SET_BIT(player_table[id].flags, PINDEX_NODELETE);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_NODELETE);

  if (PLR_FLAGGED(ch, PLR_FROZEN))
    SET_BIT(player_table[id].flags, PINDEX_FROZEN);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_FROZEN);

  if (player_table[id].flags != i || save_index)
    save_player_index();
}

/* delete_player() removes all files associated with a player who is 
 * self-deleted, deleted by an immortal, or deleted by the auto-wipe 
 * system (if enabled). */
void delete_player(int pfilepos)
{
  char fname[40];
  int i;

  if (pfilepos > top_of_p_table || !*player_table[pfilepos].name)
    return;

  /* Delete all player-owned files */
  for (i = 0; i < NUM_PLR_FILES; ++i)
    if (get_pfilename(player_table[pfilepos].name, fname, i))
      unlink(fname);

  player_table[pfilepos].name[0] = '\0';
  save_player_index();
}

void rename_player(struct char_data *victim, char *newname)
{
  int pfilepos = get_ptable_by_name(GET_NAME(victim));
  int i;
  char fname1[40], fname2[40];

  if (pfilepos < 0)
    return;

  CAP(newname);

  if (player_table[pfilepos].name)
    free(player_table[pfilepos].name);
  player_table[pfilepos].name = strdup(newname);

  /* Rename all player-owned files */
  for (i = 0; i < NUM_PLR_FILES; i++) {
    if (get_pfilename(player_table[pfilepos].name, fname1, i))
      if (get_pfilename(newname, fname2, i))
        rename(fname1, fname2);
  }

  if (GET_NAME(victim))
    free(GET_NAME(victim));
  GET_NAME(victim) = strdup(newname);

  save_player(victim);
}

void write_ascii_flags(FILE *fl, flagvector flags[], int num_flags)
{
  int i;
  char flagbuf[FLAGBLOCK_SIZE + 1];

  for (i = 0; i < FLAGVECTOR_SIZE(num_flags); ++i) {
    sprintascii(flagbuf, flags[i]);
    fprintf(fl, "%s%s", i ? " " : "", flagbuf);
  }
}


static void load_effects(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, num3 = 0, num4 = 0, i;
  long num5 = 0, num6 = 0, num7 = 0;
  char line[MAX_INPUT_LENGTH + 1];
  struct effect eff;

  i = 0;
  do {
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %ld %ld %ld", &num, &num2, &num3, &num4, &num5, &num6, &num7);
    if (num > 0) {
      eff.type = num;
      eff.duration = num2;
      eff.modifier = num3;
      eff.location = num4;
      eff.flags[0] = num5;
      eff.flags[1] = num6;
      eff.flags[2] = num7;
      effect_to_char(ch, &eff);
      i++;
    }
  } while (num != 0);
}

static void load_skills(FILE *fl, struct char_data *ch)
{
  int skill = 0, proficiency = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do {
    get_line(fl, line);
    sscanf(line, "%d %d", &skill, &proficiency);
    if (skill != 0)
      SET_SKILL(ch, skill, proficiency);
  } while (skill != 0);
}

static void scan_slash(const char *line, int *cur, int *max)
{
  int num1 = 0, num2 = 0;

  sscanf(line, "%d/%d", &num1, &num2);

  if (cur)
    *cur = num1;

  if (max)
    *max = num2;
}

/* Aliases are now saved in pfiles only. */
static void write_aliases_ascii(FILE *file, struct char_data *ch)
{
  struct alias_data *alias;

  if (GET_ALIASES(ch)) {
    fprintf(file, "aliases:\n");
    for (alias = GET_ALIASES(ch); alias; alias = alias->next)
      fprintf(file, "%s %s\n", alias->alias, alias->replacement);
    fprintf(file, "0\n");
  }
}

static void read_aliases_ascii(FILE *file, struct char_data *ch)
{
  struct alias_data *alias;
  char *replacement;

  do {
    /* Read the aliased command to buf. */
    get_line(file, buf);

    if (!strcmp(buf, "0")) /* last alias line a single 0 */
      return;

    replacement = any_one_arg(buf, buf1);
    skip_spaces(&replacement);

    CREATE(alias, struct alias_data, 1);
    alias->alias = strdup(buf1);
    alias->replacement = strdup(replacement);
    if (strchr(replacement, ALIAS_SEP_CHAR) || strchr(replacement, ALIAS_VAR_CHAR))
      alias->type = ALIAS_COMPLEX;
    else
      alias->type = ALIAS_SIMPLE;
    alias->next = GET_ALIASES(ch);
    GET_ALIASES(ch) = alias;
  } while (strcmp(buf, "0")); /* while buf is not "0" */
}

static void load_spell_mem(FILE *fl, struct char_data *ch) {
  int spell, time, can_cast, scanned;
  char line[MAX_INPUT_LENGTH + 1];
  int mem_time;

  int spell_mem_time(struct char_data *ch, int spell); /* spell_mem.c */

  do {
    get_line(fl, line);
    scanned = sscanf(line, "%d %d %d", &spell, &can_cast, &time);

    /* Compensate for a missing or corrupted time value: */
    mem_time = spell_mem_time(ch, spell);
    if (scanned < 3 || time > mem_time)
       time = mem_time;

    if (spell != 0)
      add_spell(ch, spell, can_cast, time, FALSE);
  } while (spell != 0);
}

static void load_coins(char *line, int coins[]) {
  int c1, c2, c3, c4;
  if (sscanf(line, "%d %d %d %d", &c1, &c2, &c3, &c4) == 4) {
    coins[PLATINUM] = c1;
    coins[GOLD] = c2;
    coins[SILVER] = c3;
    coins[COPPER] = c4;
  }
  else {
    coins[PLATINUM] = coins[GOLD] = coins[SILVER] = 0;
    coins[COPPER] = atoi(line);
  }
}

static void load_cooldowns(FILE *fl, struct char_data *ch) {
  int cooldown = 0, time = 0, max = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do {
    get_line(fl, line);
    sscanf(line, "%d %d/%d", &cooldown, &time, &max);
    if (time != 0 && cooldown >= 0 && cooldown < NUM_COOLDOWNS) {
      SET_COOLDOWN(ch, cooldown, time);
      GET_COOLDOWN_MAX(ch, cooldown) = max;
    }
  } while (time != 0);
}

void load_ascii_flags(flagvector flags[], int num_flags, char *line)
{
  int i = 0;

  skip_spaces(&line);

  line = strtok(line, " ");

  while (line && *line) {
    if (FLAGVECTOR_SIZE(num_flags) <= i) {
      if (*line != '0') {
        sprintf(buf, "SYSERR: load_ascii_flags: attempting to read in flags for "
                     "block %d, but only %d blocks allowed for flagvector type",
                i, FLAGVECTOR_SIZE(num_flags));
        log(buf);
      }
    }
    else
      flags[i] = asciiflag_conv(line);
    line = strtok(NULL, " ");
    ++i;
  }
}


static void load_clan(char *line, struct char_data *ch) {
  struct clan *clan = find_clan(line);
  ch->player_specials->clan = find_clan_membership_in_clan(GET_NAME(ch), clan);
  if (GET_CLAN_MEMBERSHIP(ch))
    GET_CLAN_MEMBERSHIP(ch)->player = ch;
}


void add_perm_title(struct char_data *ch, char *line) {
  int i;
  if (!GET_PERM_TITLES(ch)) {
    CREATE(GET_PERM_TITLES(ch), char *, 2);
    i = 0;
  }
  else {
    for (i = 0; GET_PERM_TITLES(ch)[i]; ++i);
    RECREATE(GET_PERM_TITLES(ch), char *, i + 2);
  }
  GET_PERM_TITLES(ch)[i] = strdup(line);
  GET_PERM_TITLES(ch)[i + 1] = NULL;
}


/*
 * Initialize a player.
 *
 * This is called when character creation is confirmed.  In other words,
 * it makes a new player character real.
 */
void init_player(struct char_data *ch)
{
  extern int mortal_start_room;
  int i;

  /* Make sure the character has a player structure */
  if (!ch->player_specials)
    CREATE(ch->player_specials, struct player_special_data, 1);

  /* If this is our first player, make 'em god */
  if (top_of_p_table == 0) {
    GET_EXP(ch) = 7000000;
    GET_LEVEL(ch) = LVL_IMPL;
    GET_BASE_HIT(ch) = 500;
    GET_MAX_MANA(ch) = 100;
  }

  GET_TITLE(ch) = NULL;
  GET_PROMPT(ch) = strdup(default_prompts[DEFAULT_PROMPT][1]);
  GET_LDESC(ch) = NULL;
  ch->player.description = NULL;
  ch->player.time.birth = time(0);
  ch->player.time.played = 0;
  ch->player.time.logon = time(0);

  for (i = 0; i < MAX_TONGUE; ++i)
    GET_TALK(ch, i) = 0;

  set_base_size(ch, races[(int) GET_RACE(ch)].def_size);
  GET_MAX_MANA(ch) = 100;
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MAX_HIT(ch) = GET_BASE_HIT(ch);
  GET_MAX_MOVE(ch) = natural_move(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  GET_AC(ch) = 100;

  player_table[GET_PFILEPOS(ch)].id = GET_IDNUM(ch) = ++top_idnum;

  for (i = 1; i < TOP_SKILL; ++i)
    SET_SKILL(ch, i, GET_LEVEL(ch) == LVL_IMPL ? 1000 : 0);

  CLEAR_FLAGS(EFF_FLAGS(ch), NUM_EFF_FLAGS);

  for (i = 0; i < NUM_SAVES; ++i)
    GET_SAVE(ch, i) = 0;

  GET_COND(ch, FULL) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);
  GET_COND(ch, THIRST) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);
  GET_COND(ch, DRUNK) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 0);

  GET_HOMEROOM(ch) = mortal_start_room;
  GET_LOADROOM(ch) = NOWHERE;
  GET_ALIASES(ch) = NULL;

  /* Set default preferences */
  SET_FLAG(PRF_FLAGS(ch), PRF_VICIOUS);	/* Finish off opponents */
  SET_FLAG(PRF_FLAGS(ch), PRF_AUTOEXIT);	/* Always show room exits */
  GET_PAGE_LENGTH(ch) = DEFAULT_PAGE_LENGTH;

  /* Set height, weight, size, lifeforce, composition to race defaults */
  init_proto_race(ch);

  cache_grants(ch);
}


/*
 * start_player()
 *
 * Sets a player's stats to be consistent with being level 1.
 * Should be called whenever a player becomes level 1, such as when:
 *   -- first created
 *   -- level is set to 1 by vengeful gods
 */
void start_player(struct char_data *ch)
{
  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;
  if (GET_TITLE(ch))
    free(GET_TITLE(ch));
  GET_TITLE(ch) = NULL;
  GET_BASE_HIT(ch) = 15;

  init_char(ch);
  advance_level(ch, LEVEL_GAIN);
  init_trophy(ch);

  GET_HIT(ch) = GET_MAX_HIT(ch) = GET_BASE_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;

  ch->player.time.logon = time(0);
}

void remove_player_from_game(struct char_data *ch, int quit_mode)
{
   clan_notification(GET_CLAN(ch), ch,
                     "%s has left the game.", GET_NAME(ch));

   GET_QUIT_REASON(ch) = quit_mode;
   save_player(ch);
   extract_objects(ch);

   /* Set this so extract_char() doesn't try to do an emergency save */
   SET_FLAG(PLR_FLAGS(ch), PLR_REMOVING);
   extract_char(ch);
}

void send_save_description(struct char_data *ch, struct char_data *dest, bool entering)
{
   int quitreason = GET_QUIT_REASON(ch);
   int room;

   if (entering)
      room = GET_LOADROOM(ch);
   else
      room = GET_SAVEROOM(ch);

   if (real_room(room) != NOWHERE) {
      sprintf(buf1, "%s (%d)", world[real_room(room)].name,
            room);
   } else {
      sprintf(buf1, "&1&bNOWHERE&0 (&5&b%d&0)", room);
   }

   if (!VALID_QUITTYPE(quitreason)) quitreason = QUIT_UNDEF;
   if (entering) {
      sprintf(buf, quit_reenter_message[quitreason], GET_NAME(ch), buf1);
   } else {
      sprintf(buf, quit_statement[quitreason], GET_NAME(ch), buf1);
   }
   if (dest) {
      cprintf(dest, "%s\r\n", buf);
   } else {
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
   }
}


/***************************************************************************
 *
 * $Log: players.c,v $
 * Revision 1.61  2010/06/05 14:56:27  mud
 * Moving cooldowns to their own file.
 *
 * Revision 1.60  2009/07/16 19:16:12  myc
 * Moved privilege stuff from grant.c to privileges.c
 *
 * Revision 1.59  2009/06/09 21:50:21  myc
 * Clan notification when a player logs out.
 *
 * Revision 1.58  2009/06/09 19:33:50  myc
 * Rewrote gain_exp and retired gain_exp_regardless.
 *
 * Revision 1.57  2009/06/09 05:46:41  myc
 * Modifying load/save player functions to work with the new
 * clan interface.  Also loading and saving privilege flags.
 *
 * Revision 1.56  2009/03/21 19:11:37  myc
 * Save the duration each cooldown started at.
 *
 * Revision 1.55  2009/03/16 19:17:52  jps
 * Change macro GET_HOME to GET_HOMEROOM
 *
 * Revision 1.54  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.53  2009/03/09 05:41:31  jps
 * Moved money stuff into money.h, money.c
 *
 * Revision 1.52  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.51  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.50  2008/09/21 04:54:23  myc
 * Added grant caches to the player structure to make can_use_command
 * take less execution time.
 *
 * Revision 1.49  2008/09/08 05:24:50  jps
 * Put autosave as the "quit reason" when autosaving. This is a temporary fix
 * that should stop people from losing keys when autosave code thinks their
 * quit reason is something else, like renting.
 *
 * Revision 1.48  2008/09/07 20:36:47  jps
 * Add periods to game re-entry messages.
 *
 * Revision 1.47  2008/09/07 01:30:37  jps
 * Add a flag for player saving, so that effect changes in the midst of it
 * can be ignored.
 *
 * Revision 1.46  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.45  2008/09/01 18:29:38  jps
 * consolidating cooldown code in skills.c/h
 *
 * Revision 1.44  2008/08/31 18:38:33  myc
 * Set max hp to base hp when loading.
 *
 * Revision 1.43  2008/08/30 04:34:05  myc
 * Missing break in switch for case 'g' in load_player.
 *
 * Revision 1.42  2008/08/30 01:31:51  myc
 * Changed the way stats are calculated in effect_total; ability
 * stats are saved in a raw form now, and only capped when accessed.
 * Damroll and hitroll are recalculated everytime effect_total
 * is called, using cached base values.
 *
 * Revision 1.41  2008/08/14 23:10:35  myc
 * Made immortal log view save to player files.
 *
 * Revision 1.40  2008/07/27 05:28:45  jps
 * Using new save_player function. Added remove_player_from_game.
 * Setting new players' home rooms to the mortal start toom.
 *
 * Revision 1.39  2008/07/15 17:55:06  myc
 * Make grants and grant groups save in player files.
 *
 * Revision 1.38  2008/06/21 08:53:09  myc
 * Read lastlogintime in and put it in the player structure so it
 * can be accessed by commands like last and show player.  To avoid
 * the lots-of-playing-time bug, last logon time must be set whenever
 * loading a player that is actually entering the game.  (See
 * enter_player_game() and link loading.)
 *
 * Revision 1.37  2008/06/16 03:59:03  myc
 * Fix play time accumulating on players.
 *
 * Revision 1.36  2008/06/05 02:07:43  myc
 * Added better unknown tag error reporting in load_player.
 * Replaced calls to strip_cr with filter_chars.  Rewrote the
 * rent file saving and loading to use an ascii file format.
 *
 * Revision 1.35  2008/04/13 03:41:16  jps
 * Using the def for the length of a player filename.
 *
 * Revision 1.34  2008/04/05 16:50:04  myc
 * Fix the reading and writing of flagvectors to be more flexible; you
 * shouldn't need to modify the code here to accomodate additional future
 * eff, prf, or plr flags.
 *
 * Revision 1.33  2008/04/03 17:37:43  jps
 * Saving autoinvis value in main player files.
 *
 * Revision 1.32  2008/04/02 04:55:59  myc
 * Got rid of the coins struct.
 *
 * Revision 1.31  2008/04/02 03:24:44  myc
 * Rewrote group code and removed major group code.
 *
 * Revision 1.30  2008/03/30 17:30:38  jps
 * Renamed objsave.c to pfiles.c and introduced pfiles.h. Files using functions
 * from pfiles.c now include pfiles.h and depend on it in the makefile.
 *
 * Revision 1.29  2008/03/29 21:14:37  myc
 * Cooldowns wear off even when you are logged off.  Also fixed a bug where
 * the alias reading code wasn't eating up its final line.
 *
 * Revision 1.28  2008/03/28 18:49:15  jps
 * Stop saving hitroll/damroll in player files, only causing trouble.
 *
 * Revision 1.27  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.26  2008/03/23 00:27:23  jps
 * Save the base composition, not the affected one.
 *
 * Revision 1.25  2008/03/22 21:54:42  jps
 * Cause new players to have their racial defaults set
 * during creation.
 *
 * Revision 1.24  2008/03/22 20:01:06  jps
 * Save/load composition and lifeforce to player save files.
 *
 * Revision 1.23  2008/03/21 15:01:17  myc
 * Removed languages.
 *
 * Revision 1.22  2008/03/17 17:59:10  myc
 * Fix crash bug with new players by initializing trophy in start_player.
 *
 * Revision 1.21  2008/03/17 15:31:27  myc
 * Fixing hitroll/damroll saving/loading.
 *
 * Revision 1.20  2008/03/16 07:01:50  jps
 * Compensate for missing or corrupted time value in the saved spell mem list.
 *
 * Revision 1.19  2008/03/16 00:20:22  jps
 * Moving trophy code to trophy.c.
 *
 * Revision 1.18  2008/03/11 19:50:55  myc
 * Changed the way allowed olc zones are saved on an immortal from
 * a fixed number of slots to a variable-length linked list.
 *
 * Revision 1.17  2008/03/11 04:32:43  jps
 * Don't log an error for a nonexistent player file.
 *
 * Revision 1.16  2008/03/11 02:56:02  jps
 * Use natural_size and base_size when reading/writing player files.
 *
 * Revision 1.15  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.  Moving innate timers to cooldown system.
 *
 * Revision 1.14  2008/03/10 19:55:37  jps
 * Made a struct for sizes with name, height, and weight.  Save base height
 * weight and size so they stay the same over size changes.
 *
 * Revision 1.13  2008/03/10 18:01:17  myc
 * Remove some unwanted flags when reloading a player.  And remoe
 * berserk affect when saving.
 *
 * Revision 1.12  2008/03/09 08:46:33  jps
 * Save the internal skill value.
 *
 * Revision 1.11  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.10  2008/03/08 23:20:06  myc
 * The load_player function now fills in player_specials if it
 * hasn't already been allocated.
 *
 * Revision 1.9  2008/03/08 22:29:06  myc
 * Moving shapechange and chant to the cooldown system.
 *
 * Revision 1.8  2008/03/08 18:59:17  jps
 * Fix log :)
 *
 * Revision 1.7  2008/03/08 18:57:44  jps
 * Add RCS log marker.
 **************************************************************************/

