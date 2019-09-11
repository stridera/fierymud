/***************************************************************************
 * $Id: casting.c,v 1.30 2009/08/02 20:19:12 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: casting.c                                      Part of FieryMUD *
 *  Usage: Functions for spellcasting.                                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000, 2001 by the Fiery Consortium. *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University.                                       *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "casting.h"
#include "handler.h"
#include "db.h"
#include "events.h"
#include "interpreter.h"
#include "spell_parser.h"
#include "class.h"
#include "races.h"
#include "skills.h"
#include "constants.h"
#include "math.h"
#include "magic.h"
#include "exits.h"
#include "rooms.h"
#include "fight.h"
#include "pfiles.h"
#include "utils.h"
#include "act.h"
#include "movement.h"
#include "limits.h"
#include "objects.h"
#include "composition.h"
#include "lifeforce.h"
#include "damage.h"
#include "spells.h"

/**********************************************************/
/* Event functions for spells that use time-based effects */
/**********************************************************/

EVENTFUNC(room_undo_event)
{
   struct room_undo_event_obj *room_undo = (struct room_undo_event_obj *) event_obj;
   int room, exit, connect_room;

   room = room_undo->room;
   exit = room_undo->exit;
   connect_room = room_undo->connect_room;

   world[room].exits[exit]->to_room = connect_room;
   send_to_room("&2The forest seems to come alive... Trees and shrubs move about, finally resting in different locations.&0\r\n", room);
   REMOVE_FLAG(ROOM_FLAGS(room), ROOM_ALT_EXIT);
   return EVENT_FINISHED;
}

EVENTFUNC(delayed_cast_event)
{
   struct delayed_cast_event_obj *cast = (struct delayed_cast_event_obj *) event_obj;
   struct char_data *ch = cast->ch;
   struct char_data *victim = cast->victim;
   int spellnum = cast->spell;
   int room = cast->room;
   int routines = cast->routines;
   int rounds = --cast->rounds;
   int wait = rounds > 0 ? cast->wait : EVENT_FINISHED;
   int skill = cast->skill;
   int savetype = cast->savetype;
   bool sustained = cast->sustained;

   /* Event checking */

   if (!ch || event_target_valid(ch) == 0)
      return EVENT_FINISHED;

   if (!IS_NPC(ch) && (!ch->desc || STATE(ch->desc) != CON_PLAYING))
      return EVENT_FINISHED;

   if (sustained) {
      if (ch->in_room != room)
         return EVENT_FINISHED;
      if (victim && ch->in_room != victim->in_room)
         return EVENT_FINISHED;
      if (!valid_cast_stance(ch, spellnum))
          return EVENT_FINISHED;
      if (IS_SET(routines, MAG_GROUP) && !IS_GROUPED(ch))
         return wait;
   }

   if (victim && event_target_valid(victim) == 0)
      return EVENT_FINISHED;

   /* Cast checking */

   if (ROOM_FLAGGED(room, ROOM_PEACEFUL) && SINFO.violent) {
      if (IS_SPELL(spellnum)) {
         if (ch && IN_ROOM(ch) == room) {
            send_to_char("A flash of white light fills the room, dispelling your "
                               "violent magic!\r\n", ch);
         }
         if (victim)
            act("White light from no particular source suddenly fills the room, "
                  "then vanishes.", FALSE, victim, 0, 0, TO_ROOM);
      }
      else { /* song/chant */
         if (ch && IN_ROOM(ch) == room) {
            send_to_char("Your words dissolve into peaceful nothingness...\r\n", ch);
            act("$n's words fade away into peaceful nothingness...\r\n",
                  FALSE, ch, 0, 0, TO_ROOM);
         }
      }
      return EVENT_FINISHED;
   }

   /* Check routines */

   if (IS_SPELL(spellnum) && victim && evades_spell(ch, victim, spellnum, skill))
      return wait;

   /* Set REMOTE_AGGR flag so that aggr_lose_spells() won't take
    * off invis, etc. */
   SET_FLAG(EFF_FLAGS(ch), EFF_REMOTE_AGGR);

   if (IS_SET(routines, MAG_DAMAGE))
      mag_damage(skill, ch, victim, spellnum, savetype);

   if (victim && DECEASED(victim)) {
      wait = EVENT_FINISHED;
   } else {

      if (IS_SET(routines, MAG_AFFECT))
         mag_affect(skill, ch, victim, spellnum, savetype, CAST_SPELL);

      if (IS_SET(routines, MAG_UNAFFECT))
         mag_unaffect(skill, ch, victim, spellnum, savetype);

      if (IS_SET(routines, MAG_POINT))
         mag_point(skill, ch, victim, spellnum, savetype);

      if (IS_SET(routines, MAG_AREA))
         mag_area(skill, ch, spellnum, savetype);

      if (IS_SET(SINFO.routines, MAG_GROUP))
         mag_group(skill, ch, spellnum, savetype);

      if (IS_SET(SINFO.routines, MAG_MASS))
         mag_mass(skill, ch, spellnum, savetype);

      if (IS_SET(SINFO.routines, MAG_BULK_OBJS))
         mag_bulk_objs(skill, ch, spellnum, savetype);

      if (IS_SET(SINFO.routines, MAG_ROOM))
         mag_room(skill, ch, spellnum);

      if (IS_SET(SINFO.routines, MAG_MANUAL)) {
         if (spellnum == SPELL_PYRE && wait == EVENT_FINISHED)
           SET_FLAG(EFF_FLAGS(ch), EFF_ON_FIRE);
         switch (spellnum) {
         case SPELL_PYRE:      spell_pyre_recur(spellnum, skill, ch, victim, NULL, savetype); break;
         case SPELL_SOUL_TAP:  spell_soul_tap_recur(spellnum, skill, ch, victim, NULL, savetype); break;
         }
         if (victim && DECEASED(victim))
            wait = EVENT_FINISHED;
      }

      /* Violent spells cause fights. */
      if (SINFO.violent && ch && victim && attack_ok(victim, ch, FALSE))
            set_fighting(victim, ch, FALSE);
   }
   REMOVE_FLAG(EFF_FLAGS(ch), EFF_REMOTE_AGGR);

   return wait;
}

struct delayed_cast_event_obj *construct_delayed_cast(struct char_data *ch,
            struct char_data *victim, int spellnum, int routines, int rounds,
            int wait, int skill, int savetype, bool sustained)
{
   struct delayed_cast_event_obj *event_obj;
   CREATE(event_obj, struct delayed_cast_event_obj, 1);
   event_obj->ch = ch;
   event_obj->victim = victim;
   event_obj->spell = spellnum;
   event_obj->room = ch->in_room;
   event_obj->routines = routines;
   event_obj->skill = skill;
   event_obj->savetype = savetype;
   event_obj->wait = wait;
   event_obj->rounds = rounds;
   event_obj->sustained = sustained;
   return event_obj;
}

/***************************************************************************
 * $Log: casting.c,v $
 * Revision 1.30  2009/08/02 20:19:12  myc
 * Special handling for pyre, which sets the primary victim on fire
 * after it finishes.
 *
 * Revision 1.29  2009/03/20 06:08:18  myc
 * Add manual spell support to delayed casting events for soul
 * tap's draining over time effect.
 *
 * Revision 1.28  2009/03/09 03:26:34  jps
 * Moved individual spell definitions to spell.c and spell.h.
 *
 * Revision 1.27  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.26  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.25  2009/03/07 22:29:42  jps
 * Set the flag EFF_REMOTE_AGGR while a continuing spell does damage to
 * a remote victim, so that aggro_remove_spells won't affect you.
 *
 * Revision 1.24  2009/03/04 05:14:16  myc
 * Change summon corpse/shift corpse to use a custom match function
 * on the new find obj system in order to only target corpses.
 *
 * Revision 1.23  2009/01/19 09:25:23  myc
 * Removed MOB_PET flag.
 *
 * Revision 1.22  2008/12/29 17:09:48  myc
 * Fix crash bug with ivory symphony (sending wrong target to
 * delayed command).
 *
 * Revision 1.21  2008/09/29 00:03:13  jps
 * Moved weight_change_object to objects.c/h.
 *
 * Revision 1.20  2008/09/27 04:54:49  jps
 * Immortals don't worry about NOWELL flags.
 *
 * Revision 1.19  2008/09/27 04:50:34  jps
 * Make delayed-cast spells sustained/non-sustained. The non-sustained ones go
 * on by themselves (a little) and don't need the caster to remain alert.
 *
 * Revision 1.18  2008/09/21 21:16:27  jps
 * Don't allow "flee" to be in a character's event list multiple times.
 *
 * Revision 1.17  2008/09/21 21:04:20  jps
 * Passing cast type to mag_affect so that potions of bless/dark presence can be quaffed by neutral people.
 *
 * Revision 1.16  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.15  2008/09/20 08:33:58  jps
 * Zero the effect structure when applying charm spell.
 *
 * Revision 1.14  2008/09/20 07:27:45  jps
 * set_fighting takes a 3rd parameter, reciprocate, which will set the attackee fighting
 * the attacker if true.
 *
 * Revision 1.13  2008/09/14 03:53:13  jps
 * Implement terror-fleeing as a delayed command.
 *
 * Revision 1.12  2008/09/11 02:50:02  jps
 * Changed skills so you have a minimum position, and fighting_ok fields.
 *
 * Revision 1.11  2008/09/02 07:20:03  jps
 * Using DECOMP for object decomposition
 *
 * Revision 1.10  2008/09/02 07:16:00  mud
 * Changing object TIMER uses into DECOMP where appropriate
 *
 * Revision 1.9  2008/09/02 06:52:30  jps
 * Using limits.h.
 *
 * Revision 1.8  2008/09/02 04:01:12  jps
 * Fix formatting on summon corpse without consent message.
 *
 * Revision 1.7  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.6  2008/08/30 20:25:38  jps
 * Moved count_hand_eq() into handler.c and mentioned it in handler.h.
 *
 * Revision 1.5  2008/08/29 16:55:00  myc
 * Made a few self-only spells still make sense if cast on someone else.
 *
 * Revision 1.4  2008/08/29 04:16:26  myc
 * Changed calls to look_at_room and added an include for act.h.
 *
 * Revision 1.3  2008/08/23 14:28:33  jps
 * Fix some moonwell messages.
 *
 * Revision 1.2  2008/08/21 05:42:03  jps
 * Fixed uninitialized variable in energy drain.
 *
 * Revision 1.1  2008/08/21 05:41:11  jps
 * Initial revision
 *
 * Revision 1.184  2008/07/29 17:54:30  jps
 * You can now call set_fighting without checking whether the subject
 * was fighting first.
 *
 * Revision 1.183  2008/07/27 06:50:12  jps
 * Fix format codes for dropped objects.
 *
 * Revision 1.182  2008/07/27 05:28:11  jps
 * Calling extract_objects - instead of duplicating the code (for banish).
 *
 * Revision 1.181  2008/07/21 18:45:26  jps
 * Use spellnum when looking up a skill record. Not skill! The skill
 * variable represents the power level, or skill proficiency - not
 * the identification number of the skill.
 *
 * Revision 1.180  2008/06/07 19:06:46  myc
 * Moved object-related constants and routines to objects.h.
 *
 * Revision 1.179  2008/06/05 02:07:43  myc
 * Changed object flags to use flagvectors.  Corpses now use
 * the normal object level to store the player's level at
 * the time of death instead of the cost_per_day field.
 *
 * Revision 1.178  2008/05/19 05:46:36  jps
 * Code formatting.
 *
 * Revision 1.177  2008/05/18 22:54:11  jps
 * Implemented hysteria as an area spell of fear.
 *
 * Revision 1.176  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.175  2008/05/18 05:18:06  jps
 * Renaming room_data struct's member "number" to "vnum", cos it's
 * a virtual number.
 *
 * Revision 1.174  2008/05/18 03:37:37  jps
 * Made dispelling invisibility from objects a bit easier, and dependent on skill.
 * Sent the message about dispelling the invisibility after it's been
 * done, so people without detect invis will know what the object is.
 *
 * Revision 1.173  2008/05/18 02:04:12  jps
 * Added isolation spell.
 *
 * Revision 1.172  2008/05/17 22:03:01  jps
 * Moving room-related code into rooms.h and rooms.c.
 *
 * Revision 1.171  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.170  2008/05/14 05:05:36  jps
 * Fix message when you're being resurrected.
 *
 * Revision 1.169  2008/05/12 03:37:57  jps
 * Fixed continuing spells like creeping doom and immolate. They were
 * broken due to stance/pos changes.
 *
 * Revision 1.168  2008/05/11 07:12:20  jps
 * Moved active_effect_remove to handler.c.
 *
 * Revision 1.167  2008/04/14 08:38:20  jps
 * Replaced wall of stone and wall of ice funcs with a single ASPELL
 * which does those two and illusory wall.
 *
 * Revision 1.166  2008/04/12 21:28:30  jps
 * Removing prototype that's in magic.h.
 *
 * Revision 1.165  2008/04/12 21:13:18  jps
 * Using new header file magic.h.
 *
 * Revision 1.164  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.163  2008/04/05 05:00:17  jps
 * Add life force to identify spell output for characters.
 *
 * Revision 1.162  2008/04/04 06:12:52  myc
 * Removed justice code.
 *
 * Revision 1.161  2008/04/03 19:15:09  jps
 * Un-break color spray!
 *
 * Revision 1.160  2008/04/02 03:24:44  myc
 * Rewrote group code and removed major group code.
 *
 * Revision 1.159  2008/03/29 16:30:59  jps
 * Disable evasion check in word of command - it's done in evades_spell().
 *
 * Revision 1.158  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.157  2008/03/26 23:38:12  jps
 * Word of command and charm use the victim's susceptibility to mental
 * attacks when determining success.
 *
 * Revision 1.156  2008/03/26 22:00:22  jps
 * The dispel magic spell is now an attack spell, against your
 * enemies that is.  It is not effective against most characters,
 * however.
 *
 * Revision 1.155  2008/03/26 18:16:28  jps
 * Updating damage_evasion() calls.
 *
 * Revision 1.154  2008/03/26 16:44:36  jps
 * Replaced all checks for undead race with checks for undead lifeforce.
 * Replaced the undead race with the plant race.
 *
 * Revision 1.153  2008/03/24 08:45:56  jps
 * Adding damage evasion checks to appropriate spells.
 *
 * Revision 1.152  2008/03/23 00:28:22  jps
 * The identify spell will tell you what a mobile is composed of.
 *
 * Revision 1.151  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.
 *
 * Revision 1.150  2008/03/09 09:27:56  jps
 * Added manual spell of fear.
 *
 * Revision 1.149  2008/03/09 03:03:39  jps
 * Prevent phantasm, animate dead, and charm from being dispelled.
 * Thus, dispel magic won't automatically destroy illusions and
 * raised mobs.
 *
 * Revision 1.148  2008/02/09 21:07:50  myc
 * Rewrote the delayed spell handler so it supports more spells.
 * It's used by immolate and creeping doom.
 *
 * Revision 1.147  2008/02/09 18:29:11  myc
 * The event code now takes care of freeing event objects.
 *
 * Revision 1.146  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.145  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.144  2008/01/29 16:51:12  myc
 * Making the vict_obj of act() const.
 *
 * Revision 1.143  2008/01/27 21:14:59  myc
 * Replace hit() with attack().
 *
 * Revision 1.142  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species into races.
 *
 * Revision 1.141  2008/01/27 09:45:41  jps
 * Got rid of the MCLASS_ defines and we now have a single set of classes
 * for both players and mobiles.
 *
 * Revision 1.140  2008/01/26 23:19:28  jps
 * Remove the equipment-destroying manual spell for acid breath.
 *
 * Revision 1.139  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.138  2008/01/23 14:43:48  jps
 * Fixed the humanoid skill requirement.
 *
 * Revision 1.137  2008/01/23 14:15:59  jps
 * update_skills() will take into account whether you're a humanoid
 * when determining which skills you're allowed to have.
 *
 * Revision 1.136  2008/01/22 22:29:25  myc
 * Fixed resurrect spell to grant back some exp for level 99's.
 *
 * Revision 1.135  2008/01/18 20:30:11  myc
 * Fixing some send_to_char strings that don't end with a newline.
 *
 * Revision 1.134  2008/01/15 03:26:00  myc
 * Fixing a typo in resurrect spell.
 *
 * Revision 1.133  2008/01/13 03:19:53  myc
 * Removed melt as a manual spell.
 *
 * Revision 1.132  2008/01/12 23:13:20  myc
 * Renamed clearMemory clear_memory.
 *
 * Revision 1.131  2008/01/10 05:39:43  myc
 * Removing the damage() prototype from the top of the file; it's already
 * declared in handler.h.
 *
 * Revision 1.130  2008/01/09 08:34:46  jps
 * Use utility functions to format the representation of heights and weights.
 *
 * Revision 1.129  2008/01/06 23:50:47  jps
 * Added spells project and simulacrum, and MOB2_ILLUSORY flag.
 *
 * Revision 1.128  2008/01/06 20:38:43  jps
 * Add spell of ventriloquate.
 *
 * Revision 1.127  2008/01/05 05:45:12  jps
 * Moved update_skills() here, since this seems to be skill central.
 *
 * Revision 1.126  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.125  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.124  2008/01/01 04:56:43  jps
 * Remove unused external variable.
 *
 * Revision 1.123  2007/12/31 04:42:47  jps
 * Made harmful spell-removal affect monk chants and bard songs as well.
 *
 * Revision 1.122  2007/12/31 04:00:19  jps
 * Generalized some spell-removal code.
 *
 * Revision 1.121  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.120  2007/11/18 16:51:55  myc
 * Improving ivory symphony effectiveness when it is dark in the room.
 *
 * Revision 1.119  2007/10/25 20:40:54  myc
 * Make it harder for ivory symphony to make sentinel mobs flee.
 *
 * Revision 1.118  2007/10/17 18:21:03  myc
 * Summon now uses the find_track_victim algorithm to locate the closest
 * target.  find_track_victim can be passed a room flag mask to skip
 * rooms with those flags.
 *
 * Revision 1.117  2007/10/13 20:13:09  myc
 * ITEM_NOLOCATE now prevents items from being found using the
 * locate object spell.
 *
 * Revision 1.116  2007/10/13 05:07:24  myc
 * Added new monk chants.
 *
 * Revision 1.115  2007/10/11 20:14:48  myc
 * Changed skill defines to support chants and songs as skills, but
 * slightly distinguished from spells and skills.  TOP_SKILL is the
 * old MAX_SKILLS.  Chants and songs now each have a block of 50
 * defines above the new MAX_SKILLS (550).
 * Monk chants are implemented as spells now.
 *
 * Revision 1.114  2007/10/04 16:20:24  myc
 * Transient item flag now makes things decay when they are on the ground.
 * Added this flag to wall of ice and stone.  Dispel magic updated to use
 * new decay functions.
 *
 * Revision 1.113  2007/10/02 02:52:27  myc
 * Energy drain is now a manual spell.
 *
 * Revision 1.112  2007/09/20 21:20:43  myc
 * Flood shouldn't hit group members, ever.  Dispel magic should improve
 * skill for failing to de-invis an invisible object.
 *
 * Revision 1.111  2007/09/15 15:36:48  myc
 * Natures embrace is now in mag_affects instead of a manual spell.
 *
 * Revision 1.110  2007/09/11 16:34:24  myc
 * Lightning breath and acid breath check the user's level for success
 * rate now.
 *
 * Revision 1.109  2007/09/09 01:20:14  jps
 * The result of casting a spell is no longer just TRUE or FALSE,
 * but two possible bits combined: charge and/or improve. If
 * CAST_RESULT_CHARGE is returned, the spell was used and the caster
 * will be charged (have the spell erased from memory).  If
 * CAST_RESULT_IMPROVE is returned, the caster may improve in that
 * sphere of magic.
 * At the same time, casters will now correctly be charged for
 * spells that are cast on objects.
 *
 * Revision 1.108  2007/09/07 20:32:19  jps
 * Adjustments to identify command.
 *
 * Revision 1.107  2007/09/07 19:40:41  jps
 * Moved object identification code to act.informative.c.
 *
 * Revision 1.106  2007/09/07 01:37:59  jps
 * Increase missiles from magic missile/ice darts/fire darts.
 * Stop disintegrate and melt from destroying objects.
 *
 * Revision 1.105  2007/09/04 06:49:19  myc
 * Added new TAR_OUTDOORS, TAR_NIGHT_ONLY, and TAR_DAY_ONLY checks for
 * spells to limit when and where they may be cast (like TAR_SELF_ONLY or
 * TAR_NOT_SELF).  This replaces checks within spells, and the checks are
 * made before the caster's spell memory is charged.
 *
 * Revision 1.104  2007/09/03 23:49:40  jps
 * Add mass_attack_ok() so that you could kill your own pet specifically,
 * but your area spells will not harm it.
 *
 * Revision 1.103  2007/09/03 21:19:17  jps
 * Standardize magic wall expiration.  Store the creation spell number in
 * magic wall object value 3, so you will know what kind it is later.
 *
 * Revision 1.102  2007/08/26 01:55:41  myc
 * Fire now does real damage.  All fire spells have a chance to catch the
 * victim on fire.  Mobs attempt to douse themselves.
 *
 * Revision 1.101  2007/08/23 01:25:33  jps
 * Return different failure messages for the various reasons that
 * a summon spell failed.
 *
 * Revision 1.100  2007/08/04 01:50:00  jps
 * Don't allow summoning into ARENA rooms.
 * Moonbeam will avoid group members (like any other area attack spell).
 *
 * Revision 1.99  2007/08/03 22:00:11  myc
 * Fixed some \r\n typoes in send_to_chars.
 *
 * Revision 1.98  2007/08/03 03:51:44  myc
 * check_pk is now attack_ok, and covers many more cases than before,
 * including peaced rooms, shapeshifted pk, and arena rooms.  Almost all
 * offensive attacks now use attack_ok to determine whether an attack is
 * allowed.
 *
 * Revision 1.97  2007/08/02 04:19:04  jps
 * Added "moonbeam" spell for Druids.
 *
 * Revision 1.96  2007/08/02 00:23:53  myc
 * Standardized magic check-PK function.  Cut out a LOT of unnecessary magic
 * code and cleaned up the whole system in general.  Magic casts are now
 * guaranteed to use sphere skills rather than level.  Almost all magic
 * functions like mag_damage or even manual spells return a boolean now:
 * TRUE if the cast deserves a skill improvement, FALSE if it doesn't.
 * This return value is ignored for object magic (wands, etc.).
 *
 * Revision 1.95  2007/07/31 00:40:00  jps
 * Typo fixes for dimension door and creeping doom.
 *
 * Revision 1.94  2007/07/25 22:24:21  jps
 * Re-jiggify locate object so that it chooses randomly among all eligible
 * items in the game. The power of the casting is properly used (instead
 * of just blindly looking at the skill proficiency of the caster). The
 * power is compared against the level of possible objects to locate, and
 * used to determine how many objects will be located.
 *
 * Revision 1.93  2007/07/24 01:24:08  myc
 * Identify shows liquid container capacity now.
 *
 * Revision 1.92  2007/07/19 21:59:52  jps
 * Dynamic strings for drink containers.
 *
 * Revision 1.91  2007/07/14 04:17:35  jps
 * Updated call to stop_follower(), which cares whether this is being
 * done due to a violent action or not.
 *
 * Revision 1.90  2007/07/04 02:21:58  myc
 * Can cast flame blade and ice dagger so you can cast with only one hand
 * free now.  Banish spell works significantly different and more often.
 * Wizard eye works on mobs.  Dispel magic can remove invisibility from
 * objects.  Removed superfluous code from summon/dim door.  Moved object
 * extraction in res spell so it comes after the spell is done with the
 * object.
 *
 * Revision 1.89  2007/06/16 00:15:49  myc
 * Three spells for necromancers: soul tap, rebuke undead,
 * and degeneration.  One spell for rangers: natures guidance.
 *
 * Revision 1.88  2007/05/11 20:13:28  myc
 * Vaporform is a new circle 13 spell for cryomancers.  It significantly
 * increases the caster's chance of dodging a hit.  It is a quest spell.
 *
 * Revision 1.87  2007/04/25 07:03:30  jps
 * Only send one dispel message for spells that set multiple effects.
 *
 * Revision 1.86  2007/04/18 19:38:59  jps
 * Set the spells you can dispel to the actual spells, excluding languages.
 *
 * Revision 1.85  2007/04/18 18:30:45  jps
 * Make it so you can dispel bone armor.
 *
 * Revision 1.84  2007/04/17 23:38:03  myc
 * Introducing the new improved color spray!  It's now an area spell that
 * causes various effects based on caster skill.
 *
 * Revision 1.83  2007/04/11 07:29:05  jps
 * Fix compile warning about declaration of room_recall_check.
 *
 * Revision 1.82  2007/04/04 13:40:12  jps
 * Implement NORECALL flag for rooms.
 *
 * Revision 1.81  2007/03/27 04:27:05  myc
 * Changed 'to the up' to 'upwards' and 'above' (ditto for down) in wall
 * of stone and wall of ice.  Made wizard eye game-wide.  Removed minor
 * creation's ambient light incrementor since obj_to_room takes care of
 * that.  Made summon's failure message always show on failure.
 *
 * Revision 1.80  2007/02/20 17:16:27  myc
 * Charm person checks for shapeshifted players.  Also changed its success
 * rate.  Identify spell shows containers weight capacity.
 *
 * Revision 1.79  2007/02/14 03:54:53  myc
 * Rain spell douses circle of fire.  Removed firewalk and greater firewalk.
 * Charm preson will not cause the mob to attack if it is succesful.  Fixed
 * dimension door.
 *
 * Revision 1.78  2006/12/19 04:36:53  dce
 * Modified Supernova to mimic Ice Shards.
 *
 * Revision 1.77  2006/12/06 15:03:53  myc
 * Identify really does show hours left for lights now.
 *
 * Revision 1.76  2006/12/05 18:37:46  myc
 * Identify spell shows time left on lights.  Illumination lights already lit lights, and always recharges at least to original duration.
 *
 * Revision 1.75  2006/11/27 02:07:05  jps
 * Add missing newline to synthetic extra descs for gate-spell objects.
 *
 * Revision 1.74  2006/11/20 22:24:17  jps
 * End the difficulties in interaction between evil and good player races.
 *
 * Revision 1.73  2006/11/20 03:55:21  jps
 * Send specific message if identify spell can't see any spells on
 * a casting object.
 *
 * Revision 1.72  2006/11/18 05:13:16  jps
 * Make darkness last 4-24 hours, and don't let it stack buggily.
 *
 * Revision 1.71  2006/11/18 04:26:32  jps
 * Renamed continual light spell to illumination, and it only works on
 * LIGHT items (still rooms too).
 *
 * Revision 1.70  2006/11/18 00:08:44  jps
 * continual light will improve when cast on objects, as well.
 *
 * Revision 1.69  2006/11/18 00:03:31  jps
 * Fix continual light items to always work when they have the
 * bit set.  Rooms now print an indicator of being continually lit.
 * Can't use it to make a room permanently lit any more.
 *
 * Revision 1.68  2006/11/16 18:42:45  jps
 * Awareness of new surroundings when magically tranported is related to
 * being asleep, blindness, etc.
 *
 * Revision 1.67  2006/11/16 17:05:41  jps
 * Wrong flag for !SUMMON mobs, doh...
 *
 * Revision 1.66  2006/11/15 20:01:01  jps
 * Actually stop !SUMMON mobs from being summoned!
 *
 * Revision 1.65  2006/11/14 23:35:16  jps
 * Fix dispel magic messages when cast on self.
 *
 * Revision 1.64  2006/11/14 18:41:37  jps
 * Introduce a chance of failure to the teleport spell.
 *
 * Revision 1.63  2006/11/12 02:34:00  jps
 * Fix crash if teleporting in an area with no suitable destination rooms.
 *
 * Revision 1.62  2006/11/12 02:31:01  jps
 * You become unmounted when magically moved to another room.
 *
 * Revision 1.61  2006/11/11 10:15:26  jps
 * Fix name-choosing bug in "locate object".
 *
 * Revision 1.60  2006/11/10 20:48:31  jps
 * Identify spell now describes worn positions and spell effects.
 * No longer states "rent".
 *
 * Revision 1.59  2006/11/08 09:07:28  jps
 * Fix typo in dispel magic message.
 *
 * Revision 1.58  2006/11/08 09:05:52  jps
 * Fix typos in wizard eye messages.
 *
 * Revision 1.57  2006/11/08 07:55:17  jps
 * Change verbal instances of "breath" to "breathe"
 *
 * Revision 1.56  2006/07/20 07:44:16  cjd
 * Typo fixes.
 *
 * Revision 1.55  2004/11/19 03:46:15  rsd
 * fixed a typo in rem curse
 * /s
 * .s
 *
 * Revision 1.54  2004/11/15 08:17:21  rls
 * *think* pointer problem with remove curse
 *
 * Revision 1.53  2004/11/11 20:03:37  cmc
 * commented out no longer in use reference to cha_app/cha_app_type
 * removed a compiler warning
 *
 * Revision 1.52  2003/06/25 05:06:59  jjl
 * More updates.  I seem to be off of my game.
 *
 * Revision 1.51  2003/06/23 01:47:09  jjl
 * Added a NOFOLLOW flag, and the "note" command, and show notes <player>
 *
 * Revision 1.50  2002/12/22 15:48:09  mud
 * RLS overworte JJL's last changeres changes. I added
 * the ress changes to RLS's copy.
 *
 * Revision 1.49  2002/12/22 09:59:24  rls
 * fixed universal remove curse bug *blush* needed to use next_containter :)
 *
 * Revision 1.47  2002/12/20 05:23:57  rls
 * added curly braces so stupid compiler wouldn't get confused about ambiguous else
 *
 * Revision 1.46  2002/12/19 07:42:35  rls
 * Added function spell_remove_curse which parses through a victims inventory
 * looking for cursed objects if victim is unaffected by curse.  It also still
 * takes the in_room/inventory argument
 *
 * Revision 1.45  2002/10/14 02:16:08  jjl
 * An update to turn vitality into a set of 6 spells, lesser endurance,
 * endurance, greater endurance, vitality, greater vitality, and dragon's
 * health.  Greater endurance is what vitality was.  The rest are scaled
 * appropriately.    The higher end may need scaled down, or may not.
 *
 * Revision 1.44  2002/09/15 04:27:59  jjl
 * Made flood no longer wash away walls and no take objects.
 *
 * Revision 1.42  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.41  2002/04/12 01:11:03  dce
 * Fixed the Dispel Magic bug. It was trying to dispel skills
 * also.
 *
 * Revision 1.40  2002/03/26 04:24:13  rls
 * Added anti pk code to melt spell, no longer able to melt PC eq without PK enabled
 *
 * Revision 1.39  2002/03/15 02:45:14  dce
 * Fixed the spell disenigrate so that it is not castable on players.
 *
 * Revision 1.38  2001/10/22 01:51:37  dce
 * Removed the extra improve_skill routine in dimension door.
 * Should only improve if successful.
 *
 * Revision 1.37  2001/10/16 15:46:36  rjd
 * Mobs will now attack players who summon them. Muahahahaha!
 *
 * Revision 1.36  2001/05/03 03:33:26  dce
 * Players no longer gain double their money when rez'ed.
 *
 * Revision 1.35  2001/03/24 19:55:46  dce
 * The identify spell will show the level of the object.
 *
 * Revision 1.34  2001/03/07 01:45:18  dce
 * Added checks so that players can not kill shapechanged players and
 * vise versa. Hopefully I didn't miss any...
 *
 * Revision 1.33  2001/01/28 18:25:24  dce
 * Fixed the rez spell so that level 98 victims no longer gain exp.
 *
 * Revision 1.32  2001/01/12 22:45:10  rsd
 * fixed res to allow res of 99's w/o them becoming level 100.
 *
 * Revision 1.31  2001/01/10 22:07:09  mtp
 * fixed a typo in dispel magic
 *
 * Revision 1.30  2000/12/26 04:42:49  rsd
 * Added a null check in dispel magic to avoid some crashing
 * added debug as well to attempt to narrow down where it's
 * coming from.
 *
 * Revision 1.29  2000/11/25 08:52:57  rsd
 * Altered comment header, and altered relocate to prevent
 * a player from relocating to another player in room -1.
 * It will log a SYSERR if the attempt is made as well.
 *
 * Revision 1.28  2000/11/25 02:33:15  rsd
 * Altered comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.27  2000/09/27 17:56:41  rsd
 * added a snippet for a betrayal spell to hose with
 * charmies.  It's just a comment as of now and the spell
 * is called betrayal.
 * /s
 *
 * Revision 1.26  2000/04/22 22:42:38  rsd
 * fixed several spellings of deity in player output. Fixed ice
 * dagger to actually have an object for it.
 * Fixed dim door so players couldn't dim door to mobiles
 *
 * Revision 1.25  2000/04/15 23:13:41  rsd
 * added improve skill calls to more spells, as well as tried to make
 * rez only give exp to players <= level 98.
 *
 * Revision 1.24  2000/04/09 22:34:17  rsd
 * the spell flood now references the proper skill sphere
 * to advance when cast.
 *
 * Revision 1.23  2000/04/08 08:37:57  rsd
 * added a check to dispel_magic to make sure the caster
 * can cast it on himself regardless of pk/consent status.
 * Also changed the act() messages of dispel_magic to
 * something more sane I hope.
 *
 * Revision 1.22  2000/04/05 06:33:07  rsd
 * added pkill check to dispel magic. also made all manual spells
 * have an improve skill call where they had none.
 *
 * Revision 1.21  2000/04/02 02:41:12  rsd
 * Added more spells into the spell proficiency system
 * by including improve_skill() calls in the ASPELLs.
 * Also lowered the exp recovered by Rez to 60% from 95%,
 * and made it to where level 99's don't get any exp back.
 * The 99 exp thing is a hack to prevent them from getting
 * 100 on rezzes.
 *
 * Revision 1.20  2000/03/24 23:50:29  rsd
 * changed comment header while I had file open to examine.
 *
 * Revision 1.19  2000/03/22 06:50:57  rsd
 * made ice darts actually do ice darts and fire fire..
 *
 * Revision 1.18  2000/03/19 20:32:32  rsd
 * Added check for dead victim in magic missile cuz it was
 * causing syserrs
 *
 * Revision 1.17  2000/03/18 06:15:42  rsd
 * rerwrote magic missile to make a chance at more darts at higher
 * level. Applied the reqrite to fire darts and ice darts.
 * added the extern:
 * extern void improve_skill (struct char_data *ch, int skill);
 * for skill improvement calls for spell usage. This was all done
 * mostly by JBK and polished off by RSD
 *
 * Revision 1.16  1999/12/10 22:13:45  jimmy
 * Exp tweaks.  Made Exp loss for dying a hardcoded 25% of what was needed for the next
 * level.  Fixed problems with grouping and exp.  Removed some redundant and unnecessary
 * exp code.
 *
 * Revision 1.15  1999/11/29 00:06:49  cso
 * made it so that animated mobs CAN cast charm
 *
 * Revision 1.14  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.13  1999/08/12 17:54:46  dce
 * Fixed experience so that there are no overflows of integers that are placed into longs.
 * Main problem was max_exp_gain and max_exp_loss. Both were overflowing due to poor
 * Hubis coding.
 *
 * Revision 1.12  1999/08/07 17:15:08  mud
 * fixed the centimeter reference in the identify spell for players
 * and mobiles. Also removed the ability to see attributes and stats
 * until such time that saves and or consent can be coded in with it
 *
 * Revision 1.11  1999/07/06 19:57:05  jimmy
 * This is a Mass check-in of the new skill/spell/language assignment system.
 * This New system combines the assignment of skill/spell/language for
 * both mobs and PCs.  LOts of code was touched and many errors were fixed.
 * MCLASS_VOID was moved from 13 to -1 to match CLASS_UNDEFINED for PC's.
 * MObs now get random skill/spell/language levels baseed on their race/class/level
 * that exactly align with PC's.  PC's no longer have to rent to use skills gained
 * by leveling or when first creating a char.  Languages no longer reset to defaults
 * when a PC levels.  Discovered that languages have been defined right in the middle
 * of the spell area.  This needs to be fixed.  A conversion util neeDs to be run on
 * the mob files to compensate for the 13 to -1 class change.
 * --gurlaek 7/6/1999
 *
 * Revision 1.10  1999/06/30 18:25:04  jimmy
 * >> This is a major conversion from the 18 point attribute system to the
 * >> 100 point attribute system.  A few of the major changes are:
 * >> All attributes are now on a scale from 0-100
 * >> Everyone views attribs the same but, the attribs for one race
 * >>   may be differeent for that of another even if they are the
 * >>   same number.
 * >> Mobs attribs now get rolled and scaled using the same algorithim as PC's
 * >> Mobs now have individual random attributes based on race/class.
 * >> The STR_ADD attrib has been completely removed.
 * >> All bonus tables for attribs in constants.c have been replaced by
 * >>   algorithims that closely duplicate the tables except on a 100 scale.
 * >> Some minor changes:
 * >> Race selection at char creation can now be toggled by using
 * >>   <world races off>
 * >> Lots of cleanup done to affected areas of code.
 * >> Setting attributes for mobs in the .mob file no longer functions
 * >>   but is still in the code for later use.
 * >> We now have a spare attribut structure in the pfile because the new
 * >>   system only used three instead of four.
 * >> --gurlaek 6/30/1999
 *
 * Revision 1.9  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list.  This code compiles fine under both gcc RH5.2 and egcs RH6.0
 *
 * Revision 1.8  1999/04/18 20:12:54  dce
 * Magic missile works like Fire Dart, Ice Dart works.
 *
 * Revision 1.7  1999/02/13 19:37:12  dce
 * Rewrote Continual Light and Darkness to be manual spells to meet our needs.
 *
 * Revision 1.6  1999/02/10 02:38:58  dce
 * Fixes some of continual light.
 *
 * Revision 1.5  1999/02/02 22:14:06  mud
 * Indented file
 * dos2unix
 *
 * Revision 1.4  1999/01/29 06:26:41  mud
 * last test, no changes
 *
 * Revision 1.3  1999/01/29 06:24:52  jimmy
 * no changes, second test
 *
 * Revision 1.2  1999/01/29 06:23:10  mud
 * no changes, test only
 *
 * Revision 1.1  1999/01/29 01:23:32  mud
 * Initial revision
 *
 ***************************************************************************/
