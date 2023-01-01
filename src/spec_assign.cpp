/***************************************************************************
 *   File: spec_assign.c                                  Part of FieryMUD *
 *  Usage: Functions to assign function pointers to objs/mobs/rooms        *
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
#include "interpreter.hpp"
#include "logging.hpp"
#include "specprocs.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* functions to perform assignments */

void ASSIGNMOB(int mob, SPECIAL(fname)) {
    if (real_mobile(mob) >= 0)
        mob_index[real_mobile(mob)].func = fname;
#ifndef TEST
    else
        log("SYSERR: Attempt to assign spec to non-existent mob #{:d}", mob);
#endif
}

void ASSIGNOBJ(int obj, SPECIAL(fname)) {
    if (real_object(obj) >= 0)
        obj_index[real_object(obj)].func = fname;
#ifndef TEST
    else
        log("SYSERR: Attempt to assign spec to non-existent obj #{:d}", obj);
#endif
}

void ASSIGNROOM(int room, SPECIAL(fname)) {
    if (real_room(room) >= 0)
        world[real_room(room)].func = fname;
#ifndef TEST
    else
        log("SYSERR: Attempt to assign spec to non-existent rm. #{:d}", room);
#endif
}

/*********************************************************************
 *                        Assignments                                *
 *********************************************************************/

/* assign special procedures to mobiles */
void assign_mobiles(void) {
    SPECIAL(postmaster);
    SPECIAL(cityguard);
    SPECIAL(receptionist);
    SPECIAL(cryogenicist);
    SPECIAL(guild_guard);
    SPECIAL(guild);
    SPECIAL(puff);
    SPECIAL(janitor);

    /* Anduin */

    ASSIGNMOB(6011, cityguard);
    ASSIGNMOB(6012, cityguard);
    ASSIGNMOB(6018, cityguard);
    ASSIGNMOB(6101, cityguard);
    ASSIGNMOB(6102, cityguard);
    ASSIGNMOB(6154, cityguard);
    ASSIGNMOB(6106, cityguard);
    ASSIGNMOB(6104, cityguard);
    ASSIGNMOB(6200, cityguard);
    ASSIGNMOB(6203, cityguard);
    ASSIGNMOB(6204, cityguard);

    ASSIGNMOB(6114, guild_guard);  /* mercenary */
    ASSIGNMOB(6007, guild_guard);  /* diabolist */
    ASSIGNMOB(6008, guild_guard);  /* antipal */
    ASSIGNMOB(6040, guild_guard);  /* assassin */
    ASSIGNMOB(6041, guild_guard);  /* thief */
    ASSIGNMOB(6060, guild_guard);  /* bard */
    ASSIGNMOB(6175, guild_guard);  /* druid guard */
    ASSIGNMOB(6216, guild_guard);  /* priest/cleric guard */
    ASSIGNMOB(6217, guild_guard);  /* paladin */
    ASSIGNMOB(6219, guild_guard);  /* necro */
    ASSIGNMOB(6201, guild_guard);  /* pyro */
    ASSIGNMOB(6202, guild_guard);  /* cryo */
    ASSIGNMOB(6206, guild_guard);  /* sorceror */
    ASSIGNMOB(6210, guild_guard);  /* illusionist */
    ASSIGNMOB(55704, guild_guard); /* berserker */

    ASSIGNMOB(6006, guild); /* Princess Signess */
    ASSIGNMOB(6007, guild); /* Ruin Wormheart */

    ASSIGNMOB(6231, guild); /* the Archmage - not the drunk (6031): RLS */
    ASSIGNMOB(6032, guild); /* black priestess */
    ASSIGNMOB(6080, guild); /* horsemaster (anti-paladin) */
    ASSIGNMOB(6020, guild); /* Doorindark (assassin) */

    ASSIGNMOB(6061, guild); /* bard */

    ASSIGNMOB(6113, guild);  /* mercenary */
    ASSIGNMOB(6176, guild);  /* warrior */
    ASSIGNMOB(55703, guild); /* Jora Granitearm - berserker */

    ASSIGNMOB(6220, guild); /* pyromancer */
    ASSIGNMOB(6221, guild); /* cryomancer */
    ASSIGNMOB(6222, guild); /* druidess */
    ASSIGNMOB(6218, guild); /* Priestess */
    ASSIGNMOB(6223, guild); /* necromancer */
    ASSIGNMOB(6211, guild); /* illusionist */

    ASSIGNMOB(6227, receptionist);
    ASSIGNMOB(6171, postmaster);
    ASSIGNMOB(6115, janitor);

    ASSIGNMOB(6017, receptionist);

    /* End Anduin */

    /* Ickle */

    ASSIGNMOB(10054, receptionist); /* Ickle */
    ASSIGNMOB(10055, postmaster);
    ASSIGNMOB(10010, guild_guard); /*cleric */
    ASSIGNMOB(10011, guild_guard); /*warrior */
    ASSIGNMOB(10012, guild_guard); /*sorceror */
    ASSIGNMOB(10013, guild_guard); /*mercenary */
    ASSIGNMOB(10200, guild_guard); /* berserker */

    ASSIGNMOB(10000, guild); /* High Priest of Zalish */
    ASSIGNMOB(10001, guild); /* warrior coach */
    ASSIGNMOB(10002, guild); /* archmage */
    ASSIGNMOB(10003, guild); /* elite mercenary */
    ASSIGNMOB(10201, guild); /* Avaldr Mountainhelm - berserker */

    /* End Ickle */

    /* Mielikki */

    ASSIGNMOB(3005, receptionist);
    ASSIGNMOB(3061, janitor);
    ASSIGNMOB(3068, janitor);
    ASSIGNMOB(3095, postmaster);
    ASSIGNMOB(3014, guild_guard);  /* illusionist */
    ASSIGNMOB(3024, guild_guard);  /* sorc pyro cryo */
    ASSIGNMOB(3054, guild_guard);  /* cleric druid priest */
    ASSIGNMOB(3026, guild_guard);  /* rogue thief assa bard merc */
    ASSIGNMOB(3027, guild_guard);  /* warrior */
    ASSIGNMOB(3549, guild_guard);  /* ranger */
    ASSIGNMOB(5300, guild_guard);  /* paladin */
    ASSIGNMOB(5302, guild_guard);  /* monk */
    ASSIGNMOB(16911, guild_guard); /* necro HH */
    ASSIGNMOB(3201, guild_guard);  /* berserker */
    ASSIGNMOB(5304, guild_guard);  /* bard */

    ASSIGNMOB(3018, guild);  /* illusionist */
    ASSIGNMOB(3020, guild);  /* archmage */
    ASSIGNMOB(3021, guild);  /* high priestess */
    ASSIGNMOB(3022, guild);  /* master thief */
    ASSIGNMOB(3023, guild);  /* warrior coach */
    ASSIGNMOB(3053, guild);  /* high druid */
    ASSIGNMOB(3490, guild);  /* pyromancer */
    ASSIGNMOB(3491, guild);  /* cryomancer */
    ASSIGNMOB(3492, guild);  /* priest */
    ASSIGNMOB(3504, guild);  /* Galithel Silverwing (ranger) */
    ASSIGNMOB(5301, guild);  /* Grey Knight (paladin) */
    ASSIGNMOB(5303, guild);  /* monk */
    ASSIGNMOB(16910, guild); /* Asiri'Qaxt (necro) */
    ASSIGNMOB(3200, guild);  /* Tozug (berserker) */
    ASSIGNMOB(5305, guild);  /* bard */

    /* End Mielikki */

    /* Ogakh */

    ASSIGNMOB(30001, receptionist);
    ASSIGNMOB(30043, postmaster);
    ASSIGNMOB(30019, guild);
    ASSIGNMOB(30016, guild);
    ASSIGNMOB(30018, guild);
    ASSIGNMOB(30017, guild);
    ASSIGNMOB(30058, guild);
    ASSIGNMOB(30059, guild);
    ASSIGNMOB(30060, guild);
    ASSIGNMOB(30061, guild);
    ASSIGNMOB(30062, guild);
    ASSIGNMOB(30078, guild); /* Esh - Illusionist */
    ASSIGNMOB(30081, guild); /* Khargol - berserker */
    ASSIGNMOB(30039, guild_guard);
    ASSIGNMOB(30041, guild_guard);
    ASSIGNMOB(30042, guild_guard);
    ASSIGNMOB(30040, guild_guard);
    ASSIGNMOB(30046, guild_guard);
    ASSIGNMOB(30047, guild_guard);
    ASSIGNMOB(30048, guild_guard);
    ASSIGNMOB(30049, guild_guard);
    ASSIGNMOB(30057, guild_guard);
    ASSIGNMOB(30079, guild_guard); /* Illusionist */
    ASSIGNMOB(30082, guild_guard); /* Berserker */

    /* End Ogakh */

    /* Nymrill */

    ASSIGNMOB(49500, receptionist);
    ASSIGNMOB(49514, guild); /* necro */
    ASSIGNMOB(49522, guild); /* sorcerer */
    ASSIGNMOB(49525, guild); /* rogue */

    /* End Nymrill */

    /* Others */
    ASSIGNMOB(1402, receptionist);  /* GoT Guild Hall */
    ASSIGNMOB(17204, guild_guard);  /* Citadel of Testing guards */
    ASSIGNMOB(58701, receptionist); /* Dancing Dolphin Inn */
    ASSIGNMOB(1, receptionist);     /* Puff */
}

/* assign special procedures to objects */
void assign_objects(void) {
    SPECIAL(bank);
    SPECIAL(red_recall);
    SPECIAL(green_recall);
    SPECIAL(blue_recall);
    SPECIAL(gray_recall);
    SPECIAL(summon_dragon);

    ASSIGNOBJ(3056, red_recall);     /* Red scrolls of recall goodness! */
    ASSIGNOBJ(3057, green_recall);   /* Red scrolls of recall goodness! */
    ASSIGNOBJ(3058, blue_recall);    /* Red scrolls of recall goodness! */
    ASSIGNOBJ(30010, gray_recall);   /* Red scrolls of recall goodness! */
    ASSIGNOBJ(18890, summon_dragon); /* dragonhelm */
    ASSIGNOBJ(18891, summon_dragon); /* dragonhelm */

    /* Ok, there's a if not defined section here for the difference between the
       test and production builds.  All assigns on the test mud that are NOT on
       production should fall into here and as the areas are moved to prodcution
       they should come out of this section.  RSD */
}

/* assign special procedures to rooms */
void assign_rooms(void) {
    extern int dts_are_dumps;
    int i;

    SPECIAL(dump);
    SPECIAL(pet_shop);
    SPECIAL(bank);

    /* Mielikki */
    ASSIGNROOM(3030, pet_shop); /* Kayla's Pet Shop */
    ASSIGNROOM(3035, dump);     /* The City Dump */
    ASSIGNROOM(3089, bank);     /* The Mielikki Bank */
    ASSIGNROOM(3091, pet_shop); /* Jorhan's Stables */

    /* Anduin */
    ASSIGNROOM(6228, pet_shop); /* Shula's Stablehouse */
    ASSIGNROOM(6230, bank);     /* A Bank */

    /* Ickle */
    ASSIGNROOM(10011, bank);     /* The Bank of Ickle */
    ASSIGNROOM(10056, pet_shop); /* Faric's Stables */

    /* Ogakh */
    ASSIGNROOM(30012, pet_shop); /* Arandidor's Dingy Companions */
    ASSIGNROOM(30026, bank);     /* Ogakh Bank */
    ASSIGNROOM(30031, pet_shop); /* Stable */

    /*Ethilien Academy*/
    ASSIGNROOM(51907, bank); /* The Banquet Hall */

    if (dts_are_dumps)
        for (i = 0; i < top_of_world; i++)
            if (ROOM_FLAGGED(i, ROOM_DEATH))
                world[i].func = dump;
}
