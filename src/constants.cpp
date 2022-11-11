/***************************************************************************
 *   File: constants.c                                    Part of FieryMUD *
 *  Usage: Numeric and string contants used by the MUD                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "conf.hpp"
#include "db.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* The following functions replace the hard coded bonus */
/* tables for attributes.  This was done for the conversion */
/* to the 100 base system.  These functions closely approximate */
/* the older 18 base hard coded arrays. -gurlaek 6/24/1999 */

void load_str_app(void) {
    int x;

    for (x = 0; x <= 100; x++) {
        /* (x, bonus) where [bonus=Mx + B] is the notation used below */
        /* M and B were determined from the old numbers */

        /* tohit bonus */
        if (x <= 28 && x >= 0) /* linear from (0,-5) to (28,-1) */
            str_app[x].tohit = (sh_int)((((float)1 / 7) * (float)x) - 5);
        if (x <= 64 && x >= 29) /* no bonus */
            str_app[x].tohit = 0;
        if (x <= 100 && x >= 65) /* linear from (65,1) to (100,7) */
            str_app[x].tohit = (sh_int)((((float)6 / 35) * (float)x) - ((float)75 / 7));

        /* todam bonus */
        if (x <= 20 && x >= 0) /* linear from (0,-4) to (20,-1) */
            str_app[x].todam = (sh_int)((((float)3 / 20) * (float)x) - 4);
        if (x <= 60 && x >= 21) /* no bonus */
            str_app[x].todam = 0;
        if (x <= 100 && x >= 61) /* linear from (61,1) to (100,14) */
            str_app[x].todam = (sh_int)((((float)13 / 39) * (float)x) - ((float)754 / 39));

#define CARRY_72_STR 300.0
#define CARRY_100_STR 786.0
        /* carry_w */
        if (x <= 72 && x >= 0)
            str_app[x].carry_w = (sh_int)((CARRY_72_STR * x) / 72);
        if (x <= 100 && x >= 73)
            str_app[x].carry_w = (sh_int)(CARRY_72_STR + ((CARRY_100_STR - CARRY_72_STR) * ((x - 72) / 28.0)));

        /* wield_w */
        if (x <= 72 && x >= 0) /* linear from (0,0) to (72, 20) */
            str_app[x].wield_w = (sh_int)((((float)5 / 18) * (float)x));
        if (x <= 100 && x >= 73) /* linear from (73,40) (100,70) */
            str_app[x].wield_w = (sh_int)((((float)30 / 27) * (float)x) - ((float)370 / 9));
    }
}

void load_thief_dex(void) {
    int x;

    for (x = 0; x <= 100; x++) {
        /* (x, bonus) where [bonus=Mx + B] is the notation used below */
        /* M and B were determined from the old numbers */

        /* pick pockets */
        if (x <= 44 && x >= 0) /* linear from (0,-99) to (44,-5) */
            dex_app_skill[x].p_pocket = (sh_int)((((float)47 / 22) * (float)x) - 99);
        if (x <= 64 && x >= 45) /* zero */
            dex_app_skill[x].p_pocket = 0;
        if (x <= 100 && x >= 65) /* linear from (65,5) to (100,25) */
            dex_app_skill[x].p_pocket = (sh_int)((((float)4 / 7) * (float)x) - ((float)225 / 7));

        /* pick locks */
        if (x <= 40 && x >= 0) /* linear from (0,-99) to (40,-5) */
            dex_app_skill[x].p_locks = (sh_int)((((float)47 / 20) * (float)x) - 99);
        if (x <= 60 && x >= 41) /* zero */
            dex_app_skill[x].p_locks = 0;
        if (x <= 100 && x >= 61) /* linear from (61,5) to (100,30) */
            dex_app_skill[x].p_locks = (sh_int)((((float)25 / 39) * (float)x) - ((float)1330 / 39));

        /* traps */
        if (x <= 44 && x >= 0) /* linear from (0,-90) to (44,-5) */
            dex_app_skill[x].traps = (sh_int)((((float)85 / 44) * (float)x) - 90);
        if (x <= 68 && x >= 45) /* zero */
            dex_app_skill[x].traps = 0;
        if (x <= 100 && x >= 69) /* linear from (69,5) to (100,15) */
            dex_app_skill[x].traps = (sh_int)((((float)10 / 31) * (float)x) - ((float)535 / 31));

        /* sneak */
        if (x <= 48 && x >= 0) /* linear from (0,-99) to (48,-5) */
            dex_app_skill[x].sneak = (sh_int)((((float)47 / 24) * (float)x) - 99);
        if (x <= 64 && x >= 49) /* zero */
            dex_app_skill[x].sneak = 0;
        if (x <= 100 && x >= 65) /* linear from (65,5) to (100,25) */
            dex_app_skill[x].sneak = (sh_int)((((float)4 / 7) * (float)x) - ((float)225 / 7));

        /* hide */
        if (x <= 40 && x >= 0) /* linear from (0,-99) to (40,-5) */
            dex_app_skill[x].hide = (sh_int)((((float)11 / 8) * (float)x) - 60);
        if (x <= 64 && x >= 41) /* zero */
            dex_app_skill[x].hide = 0;
        if (x <= 100 && x >= 65) /* linear from (65,5) to (100,25) */
            dex_app_skill[x].hide = (sh_int)((((float)4 / 7) * (float)x) - ((float)225 / 7));
    }
}

void load_dex_app(void) {
    int x;

    for (x = 0; x <= 100; x++) {
        /* (x, bonus) where [bonus=Mx + B] is the notation used below */
        /* M and B were determined from the old numbers */

        /* reaction adjustment */
        if (x <= 20 && x >= 0) /* linear from (0,-7) to (20,-2) */
            dex_app[x].reaction = (sh_int)((((float)6 / 20) * (float)x) - 7);
        if (x <= 60 && x >= 21) /* zero */
            dex_app[x].reaction = 0;
        if (x <= 100 && x >= 61) /* linear from (61,1) to (100,5) */
            dex_app[x].reaction = (sh_int)((((float)4 / 39) * (float)x) - ((float)205 / 39));

        /* missile attacks to hit bonus */
        if (x <= 20 && x >= 0) /* linear from (0,-7) to (20,-2) */
            dex_app[x].miss_att = (sh_int)((((float)6 / 20) * (float)x) - 7);
        if (x <= 60 && x >= 21) /* zero */
            dex_app[x].miss_att = 0;
        if (x <= 100 && x >= 61) /* linear from (61,1) to (100,5) */
            dex_app[x].miss_att = (sh_int)((((float)4 / 39) * (float)x) - ((float)205 / 39));

        /* AC bonus */
        if (x <= 24 && x >= 0) /* linear from (0,6) to (24,1) */
            dex_app[x].defensive = (sh_int)((((float)-5 / 24) * (float)x) + 6);
        if (x <= 56 && x >= 25) /* zero */
            dex_app[x].defensive = 0;
        if (x <= 100 && x >= 57) /* linear from (57,-1) to (100,-6) */
            dex_app[x].defensive = (sh_int)((((float)-5 / 43) * (float)x) + ((float)242 / 43));
    }
}

void load_con_app(void) {
    int x;

    for (x = 0; x <= 100; x++) {
        /* (x, bonus) where [bonus=Mx + B] is the notation used below */
        /* M and B were determined from the old numbers */

        /* hit point bonus */
        if (x <= 24 && x >= 0) /* linear from (0,-4) to (24,-1) */
            con_app[x].hitp = (sh_int)((((float)1 / 8) * (float)x) - 4);
        if (x <= 56 && x >= 25) /* zero */
            con_app[x].hitp = 0;
        if (x <= 100 && x >= 57) /* linear from (57,1) to (100,5) */
            con_app[x].hitp = (sh_int)((((float)4 / 43) * (float)x) - ((float)185 / 43));

        /* system shock survival percentage */
        if (x <= 68 && x >= 0) /* linear from (0,20) to (68,97) */
            con_app[x].shock = (sh_int)((((float)77 / 68) * (float)x) + 20);
        if (x <= 100 && x >= 69)
            con_app[x].shock = 99;
    }
}

void load_int_app(void) {
    int x;

    for (x = 0; x <= 100; x++) {
        /* percent to learn spell or skill */
        if (x <= 100 && x >= 0) /* linear from (0,3) to (100,60) */
            int_app[x].learn = (byte)((((float)57 / 100) * (float)x) + 3);

        /* bonus to skills */
        if (x <= 44 && x >= 0) /*  zero */
            int_app[x].bonus = 0;
        if (x <= 100 && x >= 45) /* linear from (45,2) to (100,7) */
            int_app[x].bonus = (byte)((((float)1 / 11) * (float)x) - ((float)23 / 11));
    }
}

void load_wis_app(void) {
    int x;

    for (x = 0; x <= 100; x++) {
        /* bonus to skills */
        if (x <= 44 && x >= 0) /*  zero */
            wis_app[x].bonus = 0;
        if (x <= 100 && x >= 45) /* linear from (45,2) to (100,7) */
            wis_app[x].bonus = (byte)((((float)1 / 11) * (float)x) - ((float)23 / 11));
    }
}

void load_cha_app(void) {
    int x;

    for (x = 0; x <= 100; x++) {
        /* bardic music uses */
        if (x <= 64) /* no bonus */
            cha_app[x].music = 1;
        if (x <= 100 && x >= 65) /* linear from (65,2) to (100,7) */
            cha_app[x].music = (sh_int)((((float)6 / 35) * (float)x) - ((float)75 / 7) + 1);
    }
}
