/* ************************************************************************
*  file:  plrtoascii.c                                   Part of FieryMUD *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
************************************************************************* */

#include "../conf.h"
#include "../sysdep.h"

typedef unsigned long int bitvector_t;

struct trophy_queue {
  /* dummy declaration */
  int dummy;
};

struct alias_data {
  /* dummy declaration */
  int dummy;
};

#include "structs.h"
#include "../interpreter.h"
#include "../utils.h"
#include "../db.h"
#include "../players.h"


#define SLASH "/"

int plr_filename(char *orig_name, char *filename);

void convert(char *filename)
{
  FILE *fl, *outfile, *index_file;
  struct char_file_u player;
  char index_name[40], outname[40], bits[128], bits2[128], bits3[128];
  int i, j;
  struct char_special_data_saved *csds;
  struct player_special_data_saved *psds;
  struct char_ability_data *cad;
  struct char_point_data *cpd;
  struct affected_type *aff;

  if (!(fl = fopen(filename, "r+"))) {
    perror("error opening playerfile");
    exit(1);
  }
  sprintf(index_name, "%s/newindex", PLR_PREFIX);
  if (!(index_file = fopen(index_name, "a"))) {
    perror("error opening index file");
    exit(1);
  }
  for (;;) {
    fread(&player, sizeof(struct char_file_u), 1, fl);
    if (feof(fl)) {
      fclose(fl);
      fclose(index_file);
      exit(1);
    }

    if (!plr_filename(player.name, outname))
      exit(1);
    printf("writing: %s\n", outname);

    fprintf(index_file, "%ld %s %d 0 %ld\n", 
	player.char_specials_saved.idnum, player.name, player.level,
	(long)player.last_logon);

    if (!(outfile = fopen(outname, "w"))) {
      printf("error opening output file");
      exit(1);
    }

/* char_file_u */
    fprintf(outfile, "name: %s\n", player.name);
    fprintf(outfile, "password: %s\n", player.pwd);
    if (*player.title)
      fprintf(outfile, "currenttitle: %s\n", player.title);
    if (player.description && *player.description)
      fprintf(outfile, "description:\n%s~\n", player.description);
    if (player.prompt && *player.prompt)
      fprintf(outfile, "prompt: %s\n", player.prompt);
    fprintf(outfile, "sex: %d\n", (int)player.sex); 
    fprintf(outfile, "class: %d\n", (int)player.class); 
    fprintf(outfile, "race: %d\n", (int)player.race);
    fprintf(outfile, "level: %d\n", (int)player.level); 
    fprintf(outfile, "home: %d\n", player.hometown);
    fprintf(outfile, "birthtime: %d\n", (int)player.birth);
    fprintf(outfile, "timeplayed: %d\n", (int)player.played);
    fprintf(outfile, "lastlogintime: %d\n", (int)player.last_logon);
    fprintf(outfile, "host: %s\n", player.host);
    fprintf(outfile, "height: %d\n", (int)player.height);
    fprintf(outfile, "weight: %d\n", (int)player.weight);
    fprintf(outfile, "size: %d\n", (int)player.size);

/* char_special_data_saved */
    csds = &(player.char_specials_saved);
    fprintf(outfile, "alignment: %d\n", csds->alignment);
    fprintf(outfile, "id: %d\n", (int)csds->idnum);
    fprintf(outfile, "playerflags: %d\n", (int)csds->act);
    sprintascii(bits, csds->affected_by);
    sprintascii(bits2, csds->affected_by2);
    sprintascii(bits3, csds->affected_by3);
    fprintf(outfile, "effectflags: %s %s %s\n", bits, bits2, bits3);
    fprintf(outfile, "savingthrows: %d %d %d %d %d\n",
            csds->apply_saving_throw[0], csds->apply_saving_throw[1],
            csds->apply_saving_throw[2], csds->apply_saving_throw[3],
            csds->apply_saving_throw[4]);

/* player_special_data_saved */
    psds = &(player.player_specials_saved);
    if (player.level < LVL_IMMORT) {
      fprintf(outfile, "skills:\n");
      for (i = 1; i <= TOP_SKILL; i++) {
	if (psds->skills[i])
	  fprintf(outfile, "%d %d\n", i, (int)psds->skills[i]);
      }
      fprintf(outfile, "0 0\n");
    }
    if (psds->wimp_level)
      fprintf(outfile, "wimpy: %d\n", psds->wimp_level);
    if (psds->freeze_level)
      fprintf(outfile, "freezelevel: %d\n", (int)psds->freeze_level);
    if (psds->invis_level)
      fprintf(outfile, "invislevel: %d\n", (int)psds->invis_level);
    if (psds->load_room)
      fprintf(outfile, "loadroom: %d\n", (int)psds->load_room);
    sprintascii(bits, psds->pref);
    fprintf(outfile, "prefflags: %s\n", bits);
    if (psds->conditions[FULL] && player.level < LVL_IMMORT &&
	psds->conditions[FULL] != PFDEF_HUNGER)
      fprintf(outfile, "hunger: %d\n", (int)psds->conditions[0]);
    if (psds->conditions[THIRST] && player.level < LVL_IMMORT &&
	psds->conditions[THIRST] != PFDEF_THIRST)
      fprintf(outfile, "thirst: %d\n", (int)psds->conditions[1]);
    if (psds->conditions[2] && player.level < LVL_IMMORT &&
	psds->conditions[DRUNK] != PFDEF_DRUNK)
      fprintf(outfile, "drunkenness: %d\n", (int)psds->conditions[2]);
    fprintf(outfile, "trophy:\n");
    j = psds->top == 0 ? USE_TROPHY : psds->top - 1;
    for (i = 0; i <= USE_TROPHY; ++i, --j) {
      if (psds->trophy[j].value == 0)
        break;
      if (psds->trophy[j].virtual >= 0)
        fprintf(outfile, "1 %d %.3f\n",
                psds->trophy[j].virtual,
                psds->trophy[j].value);
      if (j <= 0)
        j = USE_TROPHY + 1;
    }
    fprintf(outfile, "0 0\n");
    fprintf(outfile, "aliases:\n");
    for (i = 0; i < NUM_ALIASES; ++i)
      if (psds->aliases[i].type != ALIAS_NONE)
        fprintf(outfile, "%s %s\n",
                psds->aliases[i].alias,
                psds->aliases[i].replacement + 1);
    fprintf(outfile, "0\n");
    if (psds->spells_in_mem) {
      fprintf(outfile, "mem:\n");
      for (i = 0; i < psds->spells_in_mem; ++i)
        if (psds->memmed_spells[i][0])
          fprintf(outfile, "%d %d\n",
                  psds->memmed_spells[i][0],
                  psds->memmed_spells[i][1]);
      fprintf(outfile, "0 0\n");
    }
    if (psds->page_length != 22 && psds->page_length > 0)
      fprintf(outfile, "pagelength: %d\n", psds->page_length);
    if (psds->clan)
      fprintf(outfile, "clan: %d\n", psds->clan);
    if (psds->clan_rank)
      fprintf(outfile, "clanrank: %d\n", psds->clan_rank);
    if (psds->aggressive)
      fprintf(outfile, "aggression: %d\n", psds->aggressive);
    if (psds->chant)
      fprintf(outfile, "chant: %d\n", psds->chant);
    fprintf(outfile, "lastlevel: %d\n", psds->lastlevel);
    if (*psds->poofin)
      fprintf(outfile, "poofin: %s\n", psds->poofin);
    if (*psds->poofout)
      fprintf(outfile, "poofout: %s\n", psds->poofout);
    for (i = 0; i < NUM_P_TITLES; ++i)
      if (*psds->titles[i])
        fprintf(outfile, "title: %s\n", psds->titles[i]);
    if (*psds->wiz_title)
      fprintf(outfile, "wiztitle: &0%s\n", psds->wiz_title);

/* char_ability_data */
    cad = &(player.abilities);
    fprintf(outfile, "strength: %d\n", cad->str);
    fprintf(outfile, "intelligence: %d\n", cad->intel);
    fprintf(outfile, "wisdom: %d\n", cad->wis);
    fprintf(outfile, "dexterity: %d\n", cad->dex);
    fprintf(outfile, "constitution: %d\n", cad->con);
    fprintf(outfile, "charisma: %d\n", cad->cha);

/* char_point_data */
    cpd = &(player.points);
    fprintf(outfile, "hitpoints: %d/%d\n", cpd->hit, psds->nathps);
    fprintf(outfile, "mana: %d/%d\n", cpd->mana, cpd->max_mana);
    fprintf(outfile, "move: %d/%d\n", cpd->move, cpd->max_move);
    fprintf(outfile, "ac: %d\n", cpd->armor);
    fprintf(outfile, "cash: %d %d %d %d\n",
            cpd->coins.plat, cpd->coins.gold,
            cpd->coins.silver, cpd->coins.copper);
    fprintf(outfile, "bank: %ld %ld %ld %ld\n",
            cpd->coins.bank_plat, cpd->coins.bank_gold, 
            cpd->coins.bank_silver, cpd->coins.bank_copper);
    fprintf(outfile, "experience: %ld\n", cpd->exp);
    fprintf(outfile, "hitroll: %d\n", cpd->hitroll);
    fprintf(outfile, "damroll: %d\n", cpd->damroll);

/* affected_type */
    fprintf(outfile, "effects:\n");
    for (i = 0; i < MAX_AFFECT; i++) {
      aff = &(player.affected[i]);
      if (aff->type)
	fprintf(outfile, "%d %d %d %d %d\n", aff->type, aff->duration,
	  aff->modifier, aff->location, (int)aff->bitvector);
    }
    fprintf(outfile, "0 0 0 0 0\n");

    fclose(outfile);
  }
}


int sprintascii(char *out, bitvector_t bits)
{
  int i, j = 0;
  /* 32 bits, don't just add letters to try to get more unless your bitvector_t is also as large. */
  char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

  for (i = 0; flags[i]; ++i)
    if (bits & (1 << i))
      out[j++] = flags[i];

  if (j == 0) /* Didn't write anything. */
    out[j++] = '0';

  /* Nul terminate the output string. */
  out[j++] = '\0';
  return j;
}


int main(int argc, char **argv)
{
  if (argc != 2)
    printf("Usage: %s playerfile-name\n", argv[0]);
  else
    convert(argv[1]);

  return 0;
}


int plr_filename(char *orig_name, char *filename)
{
  char name[64], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    perror("error getting player file name");
    return (0);
  }

  strcpy(name, orig_name);
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  sprintf(filename, "%s/%c/%c%s.plr", PLR_PREFIX, UPPER(*name), UPPER(*name), name + 1);
  return (1);
}
