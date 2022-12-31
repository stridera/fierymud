/***************************************************************************
 *  File: charsize.c                                     Part of FieryMUD  *
 *  Usage: Source file for character sizes                                 *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "charsize.hpp"

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "races.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* name, color, weight_min, weight_max, height_min, height_max */
struct sizedef sizes[NUM_SIZES] = {
    /* TINY */ {"tiny", "&b&1", 1, 3, 1, 18},
    /* SMALL */ {"small", "&b&8", 5, 40, 19, 42},
    /* MEDIUM */ {"medium", "&3", 40, 300, 42, 92},
    /* LARGE */ {"large", "&b&4", 300, 1000, 90, 186},
    /* HUGE */ {"huge", "&b&3", 1000, 4000, 196, 384},
    /* GIANT */ {"giant", "&5", 4000, 16000, 384, 768},
    /* GARGANTUAN */ {"gargantuan", "&1", 16000, 64000, 768, 1536},
    /* COLOSSAL */ {"colossal", "&2&b", 64000, 256000, 1536, 3072},
    /* TITANIC */ {"titanic", "&6&b", 256000, 1024000, 3072, 6144},
    /* MOUNTAINOUS */ {"mountainous", "&7&b", 1024000, 4096000, 6144, 12288}};

int parse_size(CharData *ch, char *arg) { return parse_obj_name(ch, arg, "size", NUM_SIZES, sizes, sizeof(sizedef)); }

/* Set a character's height and weight. */
void set_init_height_weight(CharData *ch) {
    int race = GET_RACE(ch), defsize;

    /* First give the char a height and weight based on the standard size
     * of its race. */

    if (race >= 0 && race < NUM_RACES) {
        defsize = races[race].def_size;
        if (ch->player.sex == SEX_MALE) {
            ch->player.base_weight = number(races[race].mweight_lo, races[race].mweight_hi);
            ch->player.base_height = number(races[race].mheight_lo, races[race].mheight_hi);
        } else if (ch->player.sex == SEX_FEMALE) {
            ch->player.base_weight = number(races[race].fweight_lo, races[race].fweight_hi);
            ch->player.base_height = number(races[race].fheight_lo, races[race].fheight_hi);
        } else {
            ch->player.base_weight = number(races[race].fweight_lo, races[race].mweight_hi);
            ch->player.base_height = number(races[race].fheight_lo, races[race].mheight_hi);
        }
    } else {
        if (GET_SIZE(ch) >= 0 && GET_SIZE(ch) < NUM_SIZES)
            defsize = GET_SIZE(ch);
        else
            defsize = SIZE_MEDIUM;
        ch->player.base_weight = number(sizes[defsize].weight_min, sizes[defsize].weight_max);
        ch->player.base_height = number(sizes[defsize].height_min, sizes[defsize].height_max);
    }

    /* For prototyped mobs, base_size at this point contains the builder's
     * chosen size.  defsize contians the default size according to race. */

    if (ch->player.base_size != defsize) {
        /* When a builder has overridden a mob's racial size, we'll give it a
         * height and weight that are in the builder's chosen size, but
         * proportional to the race's height/weight range within the race's
         * standard size. */
        ch->player.natural_size = ch->player.base_size;
        ch->player.base_size = defsize;
        reset_height_weight(ch);
        ch->player.base_size = ch->player.natural_size;
        ch->player.base_weight = ch->player.weight;
        ch->player.base_height = ch->player.height;
    } else
        reset_height_weight(ch);
}

/* reset_height_weight()
 *
 * Change a character's height and weight to reflect its size, so that they
 * are proportional according to the char's base height and weight.
 *
 * For example, suppose you're medium size, and your height is 52 inches.
 * The minimum height of medium is 42 inches, and the maximum is 92.
 * So you are 10 inches over the minimum, and, you have 1/5 of your
 * potential within that range.
 *
 * This function would cause you to use 1/5 of the available range in
 * your new size. So, if changing to LARGE, (90 to 186 in) you would
 * have an available range of 96 in (which is 186 - 90). Your height
 * will end up as 90 + 96 * (1/5), which is 109 inches.
 *
 * Also, due to base values being saved, you would always have the same
 * values when you change back to your normal size. */

void reset_height_weight(CharData *ch) {
    int bsize, asize;

    asize = ch->player.natural_size + ch->player.mod_size;

    if (asize < 0)
        asize = 0;
    else if (asize >= NUM_SIZES)
        asize = NUM_SIZES - 1;

    ch->player.affected_size = asize;
    bsize = ch->player.base_size;

    if (asize == bsize) {
        GET_WEIGHT(ch) = ch->player.base_weight;
        GET_HEIGHT(ch) = ch->player.base_height;
        return;
    }

    GET_WEIGHT(ch) = MAX(1, sizes[asize].weight_min + (sizes[asize].weight_max - sizes[asize].weight_min) *
                                                          (ch->player.base_weight - sizes[bsize].weight_min) /
                                                          (sizes[bsize].weight_max - sizes[bsize].weight_min));

    GET_HEIGHT(ch) = MAX(1, sizes[asize].height_min + (sizes[asize].height_max - sizes[asize].height_min) *
                                                          (ch->player.base_height - sizes[bsize].height_min) /
                                                          (sizes[bsize].height_max - sizes[bsize].height_min));
}

/* set_base_size()
 *
 * Set a character's size, AT creation. */

void set_base_size(CharData *ch, int newsize) {
    ch->player.base_size = newsize;
    ch->player.natural_size = newsize;
    ch->player.base_height = number(sizes[newsize].height_min, sizes[newsize].height_max);
    ch->player.base_weight = number(sizes[newsize].weight_min, sizes[newsize].weight_max);
    reset_height_weight(ch);
}

/* change_natural_size()
 *
 * Bluntly changes a character's natural size.  For use *after* creation.
 * Probably from a wizard using "set <char> size <foo>". */

void change_natural_size(CharData *ch, int newsize) {
    if (newsize == ch->player.natural_size)
        return;
    if (newsize < 0 || newsize >= NUM_SIZES) {
        sprintf(buf, "SYSERR: change_size(): invalid size %d", newsize);
        log(buf);
        ;
        return;
    }

    ch->player.natural_size = newsize;
    reset_height_weight(ch);
}

/* adjust_size()
 *
 * Changes a character's size temporarily. The value given is a difference:
 * if positive, the char is getting bigger; if negative, the character
 * is shrinking. */

void adjust_size(CharData *ch, int delta) {
    ch->player.mod_size += delta;
    reset_height_weight(ch);
}

void show_sizes(CharData *ch) {
    int i;
    char hrange[MAX_STRING_LENGTH];
    char wrange[MAX_STRING_LENGTH];

    char_printf(ch, "The character sizes are:\n\n");
    char_printf(ch, "Idx  Name          Height range                              Weight range\n");
    char_printf(ch,
                "---  ------------  ----------------------------------------  "
                "-------------------------\n");
    for (i = 0; i < NUM_SIZES; i++) {
        sprintf(hrange, "%-18s - %-18s", statelength(sizes[i].height_min), statelength(sizes[i].height_max));
        sprintf(wrange, "%-11s - %-11s", stateweight(sizes[i].weight_min), stateweight(sizes[i].weight_max));
        sprintf(buf, "% 3d  %s%-12s&0  %-40s  %s\n", i, sizes[i].color, sizes[i].name, hrange, wrange);
        char_printf(ch, buf);
    }
}
