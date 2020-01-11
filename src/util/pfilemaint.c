/***************************************************************************
 *   File: pfilemaint.c                                  Part of FieryMUD  *
 *  Usage: Cleans the player file of bogus records.                        *
 *         Loops through the player file and reads each entry,             *
 *         then checks for a name.  If the name exists then                *
 *         save it to PLAYER_FILE.new.                                     *
 *                                                                         *
 *     By: Jimmy Kincaid AKA Gurlaek the Cruel  7/15/1999                  *
 ***************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "db.h"
#include "utils.h"

int main(void) {
  FILE *pfile, *pfile_new;
  struct char_file_u *player;
  int plrid = 0, saved = 0, axed = 0;
  char logbuf[50];

  if (!(pfile = fopen("../lib/"PLAYER_FILE, "r"))) {
    if (errno != ENOENT) {
      perror("fatal error opening playerfile");
      return 1;
    } else {
      perror("playerfile doesn't exist");
      return 1;
    }
  }

  CREATE(player, struct char_file_u, 1);

  pfile_new = fopen("../lib/"PLAYER_FILE".new", "w");
  
  while (fread(player , sizeof(struct char_file_u), 1, pfile)) {
    printf("Player: %s", player->name);
    if(strlen(player->name)) {
      fwrite(player, sizeof(struct char_file_u), 1, pfile_new);
      printf("...Saved\r\n");
      saved++;
    } else {
      printf("...Discarded\r\n");
      axed++;
    }
    plrid++;
  }
  fclose(pfile);
  fclose(pfile_new);
  free(player);

  sprintf(logbuf, "PFILEMAINT:Original: %d Discarded: %d Saved: %d", plrid, axed, saved);
  printf("%s\r\n", logbuf);
  
  return 0;
}



















