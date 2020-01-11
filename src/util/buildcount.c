/* This is a silly little program that increments a number in a file by 1 .*/
/* It is intended to be used as a counter to keep track of the number of   */
/* builds done of a source tree.                                           */
/* Written for FieryMUD by Jimmy Kincaid aka Fingon                        */

#include <stdio.h>
#include <stdlib.h>

void main(int argc, char * argv[]) {
  FILE *fd;
  int build_count;

  if((fd=fopen(argv[1], "r"))) {
    fscanf(fd, "%d", &build_count);
    fclose(fd);
  }
  if((fd=fopen(argv[1], "w"))) {
    printf("Build: %d\n", build_count + 1);
    fprintf(fd, "%d\n",(build_count + 1));
    fclose(fd);
  }
}
