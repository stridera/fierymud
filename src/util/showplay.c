/* ************************************************************************
*  file:  showplay.c                                  Part of CircleMud   *
*  Usage: list a diku playerfile                                          *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
************************************************************************* */

#include "../conf.h"
#include "../sysdep.h"

#include "../structs.h"


void show(char *filename)
{
  char sexname;
  char classname[10];
  FILE *fl;
  struct char_file_u player;
  int num = 0;

  if (!(fl = fopen(filename, "r+"))) {
    perror("error opening playerfile");
    exit(1);
  }
  for (;;) {
    fread(&player, sizeof(struct char_file_u), 1, fl);
    if (feof(fl)) {
      fclose(fl);
      exit(1);
    }
    switch (player.class) {
    case CLASS_THIEF:
      strcpy(classname, "Th");
      break;
    case CLASS_WARRIOR:
      strcpy(classname, "Wa");
      break;
    case CLASS_MAGIC_USER:
      strcpy(classname, "Mu");
      break;
    case CLASS_CLERIC:
      strcpy(classname, "Cl");
      break;
    default:
      strcpy(classname, "--");
      break;
    }

    switch (player.sex) {
    case SEX_FEMALE:
      sexname = 'F';
      break;
    case SEX_MALE:
      sexname = 'M';
      break;
    case SEX_NEUTRAL:
      sexname = 'N';
      break;
    case SEX_NONBINARY:
      sexnam = 'NB';
    default:
      sexname = '-';
      break;
    }

    printf("%5d. ID: %5ld (%c) [%2d %s] %-16s %9dg %9ldb\n", ++num,
	   player.char_specials_saved.idnum, sexname, player.level,
	   classname, player.name, player.points.coins.gold,
	   player.points.coins.bank_gold);
  }
}


int main(int argc, char **argv)
{
  if (argc != 2)
    printf("Usage: %s playerfile-name\n", argv[0]);
  else
    show(argv[1]);

  return 0;
}
