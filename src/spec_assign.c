/**************************************************************************
 * $Id: spec_assign.c,v 1.73 2010/07/02 14:09:16 mud Exp $
 ***************************************************************************/
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

#include "conf.h"
#include "db.h"
#include "interpreter.h"
#include "specprocs.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* functions to perform assignments */

void ASSIGNMOB(int mob, SPECIAL(fname)) {
    if (real_mobile(mob) >= 0)
        mob_index[real_mobile(mob)].func = fname;
#ifndef TEST
    else
        log("SYSERR: Attempt to assign spec to non-existent mob #%d", mob);
#endif
}

void ASSIGNOBJ(int obj, SPECIAL(fname)) {
    if (real_object(obj) >= 0)
        obj_index[real_object(obj)].func = fname;
#ifndef TEST
    else
        log("SYSERR: Attempt to assign spec to non-existent obj #%d", obj);
#endif
}

void ASSIGNROOM(int room, SPECIAL(fname)) {
    if (real_room(room) >= 0)
        world[real_room(room)].func = fname;
#ifndef TEST
    else
        log("SYSERR: Attempt to assign spec to non-existent rm. #%d", room);
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

    ASSIGNMOB(6114, guild_guard); /* mercenary */
    ASSIGNMOB(6007, guild_guard); /* diabolist */
    ASSIGNMOB(6008, guild_guard); /* antipal */
    ASSIGNMOB(6040, guild_guard); /* assassin */
    ASSIGNMOB(6041, guild_guard); /* thief */
    ASSIGNMOB(6175, guild_guard); /* druid guard */
    ASSIGNMOB(6216, guild_guard); /* priest/cleric guard */
    ASSIGNMOB(6217, guild_guard); /* paladin */
    ASSIGNMOB(6219, guild_guard); /* necro */
    ASSIGNMOB(6201, guild_guard); /* pyro */
    ASSIGNMOB(6202, guild_guard); /* cryo */
    ASSIGNMOB(6206, guild_guard); /* sorceror */
    ASSIGNMOB(6210, guild_guard); /* illusionist */

    ASSIGNMOB(6006, guild); /* Princess Signess */
    ASSIGNMOB(6007, guild); /* Ruin Wormheart */

    ASSIGNMOB(6231, guild); /* the Archmage - not the drunk (6031): RLS */
    ASSIGNMOB(6032, guild); /* black priestess */
    ASSIGNMOB(6080, guild); /* horsemaster (anti-paladin) */
    ASSIGNMOB(6020, guild); /* Doorindark (assassin) */

    ASSIGNMOB(6113, guild); /* mercenary */
    ASSIGNMOB(6176, guild); /* warrior */

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

    ASSIGNMOB(10000, guild); /* High Priest of Zalish */
    ASSIGNMOB(10001, guild); /* warrior coach */
    ASSIGNMOB(10002, guild); /* archmage */
    ASSIGNMOB(10003, guild); /* elite mercenary */

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
    SPECIAL(holyw_weapon);
    SPECIAL(fire_weapon);
    SPECIAL(lightning_weapon);
    SPECIAL(frost_weapon);
    SPECIAL(vampiric_weapon);
    SPECIAL(red_recall);
    SPECIAL(green_recall);
    SPECIAL(blue_recall);
    SPECIAL(gray_recall);
    SPECIAL(summon_dragon);

    ASSIGNOBJ(3056, red_recall);      /* Red scrolls of recall goodness! */
    ASSIGNOBJ(3057, green_recall);    /* Red scrolls of recall goodness! */
    ASSIGNOBJ(3058, blue_recall);     /* Red scrolls of recall goodness! */
    ASSIGNOBJ(30010, gray_recall);    /* Red scrolls of recall goodness! */
    ASSIGNOBJ(1213, vampiric_weapon); /* Calian Slayer */

    ASSIGNOBJ(5104, lightning_weapon); /* stone dagger */
    ASSIGNOBJ(48804, frost_weapon);    /* lightweight baton */
    ASSIGNOBJ(6366, fire_weapon);      /* Celestial Betrayer */
    ASSIGNOBJ(18890, summon_dragon);   /* dragonhelm */
    ASSIGNOBJ(18891, summon_dragon);   /* dragonhelm */

    /* Ok, there's a if not defined section here for the difference between the
       test and production builds.  All assigns on the test mud that are NOT on
       production should fall into here and as the areas are moved to prodcution
       they should come out of this section.  RSD */

#ifndef PRODUCTION
    ASSIGNOBJ(36112, holyw_weapon); /* Unholy Bane */

#endif
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
    ASSIGNROOM(3089, bank);     /* The Mielikki Dump */
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

    if (dts_are_dumps)
        for (i = 0; i < top_of_world; i++)
            if (ROOM_FLAGGED(i, ROOM_DEATH))
                world[i].func = dump;
}

/***************************************************************************
 *
 * $Log: spec_assign.c,v $
 * Revision 1.73  2010/07/02 14:09:16  mud
 * Adding cone of cold weapon.
 *
 * Revision 1.72  2010/04/25 22:57:16  mud
 * Changing Celestial Betrayer to shoot fireballs.
 *
 * Revision 1.71  2010/04/25 22:13:38  mud
 * Adding frost weapon spec proc.
 *
 * Revision 1.70  2009/06/09 05:47:39  myc
 * Suppress some 'non-existent object' warnings when building
 * for test.
 *
 * Revision 1.69  2009/03/20 20:19:51  myc
 * Removing dependency upon old board system.
 *
 * Revision 1.68  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.67  2008/08/10 06:54:33  jps
 * Added illusionist guild guards and guild masters.
 *
 * Revision 1.66  2008/04/20 18:11:36  jps
 * Remove board assignments from obsolete boards.
 *
 * Revision 1.65  2008/04/04 06:12:52  myc
 * Removed several old spec procs.
 *
 * Revision 1.64  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.63  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.62  2007/12/19 20:55:41  myc
 * Getting rid of a defunct spec proc for a defunct clan.
 *
 * Revision 1.61  2007/11/18 02:15:49  rsd
 * Added ifdef for production for wof boards since they are
 * production specific and not necessary for developer
 * builds out of the basemuds.
 *
 * Revision 1.60  2007/08/16 11:54:08  jps
 * Remove references to defunct specprocs.
 *
 * Revision 1.59  2007/07/24 23:02:52  jps
 * Minor typo fix.
 *
 * Revision 1.58  2007/06/25 07:43:27  jps
 * Assign new Mielikki cleric guild guard, mob 3054.
 *
 * Revision 1.57  2007/03/27 04:27:05  myc
 * Dragonmount special proc.
 *
 * Revision 1.56  2006/11/11 23:52:32  jps
 * Change Ogakh scroll spelling to "gray"
 *
 * Revision 1.55  2006/10/06 02:07:18  dce
 * Fix a typo'd mob
 *
 * Revision 1.54  2006/10/06 01:54:48  dce
 * Added spec_procs for Ogakh and associated grey recall.
 *
 * Revision 1.53  2006/07/26 17:14:15  cjd
 * Added gen for WoF's clan boards.
 *
 * Revision 1.52  2006/05/08 16:50:32  rls
 * Added GoT innkeeper to defines
 *
 * Revision 1.51  2004/12/03 07:05:32  rsd
 * Ok, moved a few spec assigns around to ifndef production
 * sections to remove syserrs on boot.
 *
 * Revision 1.50  2004/12/03 06:45:21  rsd
 * Added the spec assigns for Laich's new zone
 * and moved it to the production side of the ifndef.
 *
 * Revision 1.49  2004/12/02 10:05:17  mud
 * Changed the vnum on the archmage gm from the drunk gnome to the actual
 *Archmage
 *
 * Revision 1.48  2004/12/01 15:08:37  rsd
 * added the archmage in anduin to the guildmasters
 * list...
 *
 * Revision 1.47  2004/10/25 07:46:25  rls
 * Added priestess in Anduin to guildmaster list.
 *
 * Revision 1.46  2003/12/11 19:13:52  rsd
 * removed the templar as a mielikki guildguard
 *
 * Revision 1.45  2003/06/20 15:10:12  rls
 * Changed vnum of slayer to 1213.. since it floated up by one.
 *
 * Revision 1.44  2003/01/04 00:00:46  jjl
 * Added a receptionist for Laich.
 *
 * Revision 1.43  2002/10/19 18:29:52  jjl
 * New and improved red green and blue scrolls of recall. Yummy!
 *
 * Revision 1.42  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.41  2001/07/09 19:38:29  rsd
 * Added the holy word proc to a paladin quest
 * weapon.
 *
 * Revision 1.40  2001/01/13 22:25:27  rsd
 * removed more defined rooms from the production section
 * to prevent production build boot syserrs.
 *
 * Revision 1.39  2001/01/12 21:51:38  rsd
 * Added ifndefs for production to assigns for test not on
 * production won't cause syserrs on the production boot.
 * boo yea
 *
 * Revision 1.38  2001/01/12 04:45:42  rsd
 * removed a bogus mobile assign causes a boot syserr
 * .s
 *
 * Revision 1.37  2001/01/12 04:36:31  rsd
 * Ok, I altered many of the spec_assigns for mobiles and
 * grouped them by towns.  Also uncommented many old mobiles
 * that had been comment for some reason so many parts of
 * the testmud that had unworking banks and reception areas
 * may actually work now.
 * I also deleted unused commented assigns that pointed
 * to bogus things that don't exist anymore.
 *
 * Revision 1.36  2001/01/08 07:16:53  rsd
 * added monk guildmaster
 *
 * Revision 1.35  2001/01/04 22:49:24  mtp
 * added guard 6175 as a guild guard in 6148
 *
 * Revision 1.34  2000/12/29 20:17:35  rsd
 * added necro guildmaster back to active list.
 *
 * Revision 1.33  2000/11/29 00:19:24  mtp
 * checking guild guards for ickle/anduin/mielikki
 *
 * Revision 1.32  2000/11/25 02:33:15  rsd
 * Altered comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.31  2000/11/18 06:08:03  rsd
 * added pzam a tod0 board
 *
 * Revision 1.30  2000/11/15 16:14:41  rsd
 * added the lightning dagger back
 *
 * Revision 1.29  2000/11/12 07:15:43  rsd
 * altered some assigns to get rid of boot errors on test
 *
 * Revision 1.28  2000/10/19 23:20:52  mtp
 * removed diabolist from assigns
 *
 * Revision 1.27  2000/10/15 05:27:49  cmc
 * added teachers to the guild (level gain) code
 *
 * Revision 1.26  2000/10/13 20:06:48  cmc
 * added all non-Mielikki guildmasters to "level gain" procs
 *
 * Revision 1.25  2000/10/13 17:54:13  cmc
 * re-instate Mielikki guildmasters special procedure
 * if there are guildmasters not in my token world,
 * they will need to be added for "level gain" to work!
 *
 * Revision 1.24  2000/09/18 01:46:27  rsd
 * altered pet shop assigns
 *
 * Revision 1.23  2000/09/18 01:39:08  mud
 * added some pet shops.
 *
 * Revision 1.22  2000/09/05 03:20:27  rsd
 * added a postmaster to Ickle
 *
 * Revision 1.21  2000/05/14 05:41:58  rsd
 * added kerristone assigns
 *
 * Revision 1.20  2000/04/23 09:05:25  rsd
 * ok, I'm a doofus, altered comment header, and added guild_guard
 * assign to the citadel of testing guardians.
 *
 * Revision 1.19  2000/03/04 21:35:11  rsd
 * changed the weapon proc assign to the proper weapon.
 *
 * Revision 1.18  2000/03/04 21:23:25  rsd
 * attempted to add a dispel_good_weapon special no luck
 * I think the failure is associated with the lack of
 * success surrounding the entire dispel good spell itself.
 * Also added a weapon to hav ethe vampyric special.
 *
 * Revision 1.17  2000/02/14 05:11:39  cso
 * added various procs for khuzhadam and anduin, and
 * receptionists for the bazillion new areas as necessary.
 *
 * Revision 1.16  1999/12/27 18:22:14  rsd
 * added the vnum for the mielikki ranger guild guard
 * /
 *
 * Revision 1.15  1999/11/22 13:29:10  rsd
 * added the assigns to the mielikki guild guards.
 *
 * Revision 1.14  1999/11/19 05:31:07  cso
 * added buncha anduin stuff, moved other things around for readability
 * Assigned various anduin mobspecs, set mielikki mobspecs alone, added
 * bank roomspec.
 *
 * Revision 1.13  1999/11/16 00:11:44  rsd
 * added mobile assignment for clan0 guild guard, the town
 * guild guard assignemnts are to follow
 *
 * Revision 1.12  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.11  1999/08/22 22:39:33  mud
 * added assigns for new vnums for anduin and ickle receptionists.
 *
 * Revision 1.10  1999/08/14 22:32:09  mud
 * added dump proc for the dump in mielikki
 *
 * Revision 1.9  1999/06/24 02:24:47  mud
 * added the diab subclasser and removed the lightning affect from
 * the stone dagger due to it's spoilage.
 *
 * Revision 1.8  1999/06/20 03:59:42  mud
 * added the fido proc to a mobile for test
 *
 * Revision 1.7  1999/06/11 03:01:25  mud
 * added mob 61000 as a receptionist
 *
 * Revision 1.6  1999/04/14 02:10:32  mud
 * added an assign to an existing weapon proc
 *
 * Revision 1.5  1999/04/10 20:30:24  mud
 * added 10011 room as a bank
 *
 * Revision 1.4  1999/04/08 16:56:24  dce
 * Mail is back!
 *
 * Revision 1.3  1999/02/17 04:56:10  mud
 * Added 3089 as a bank. in the room specials
 *
 * Revision 1.2  1999/02/02 15:34:47  mud
 * dos2unix
 * Cleaned up comment header some
 * indented file
 *
 * Revision 1.1  1999/01/29 01:23:32  mud
 * Initial revision
 *
 ***************************************************************************/
