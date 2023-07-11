/***************************************************************************
 *   File: act.movement.c                                 Part of FieryMUD *
 *  Usage: movement commands, door handling, & sleep/rest/etc state        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "act.hpp"

#include "board.hpp"
#include "casting.hpp"
#include "charsize.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "directions.hpp"
#include "events.hpp"
#include "exits.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "house.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "magic.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "modify.hpp"
#include "movement.hpp"
#include "races.hpp"
#include "regen.hpp"
#include "rooms.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"
#include "weather.hpp"

#include <math.h>

/* external functs */
int special(CharData *ch, int cmd, char *arg);
void appear(CharData *ch);
bool senses_living(CharData *ch, CharData *vict, int basepct); /* act.informative.c */

#define BLOOD_DROP_OBJ 34 /* the vnum of the blood object */
#define BLOOD_POOL_OBJ 35 /* the vnum of the blood object */

void spill_blood(CharData *ch) {
    ObjData *obj, *next;

    if (GET_HIT(ch) > (GET_MAX_HIT(ch) * 3) / 10)
        return;

    if (ch->in_room == NOWHERE)
        return;

    for (obj = world[ch->in_room].contents; obj; obj = next) {
        next = obj->next_content;
        if (GET_OBJ_VNUM(obj) == 34 || GET_OBJ_VNUM(obj) == 35)
            extract_obj(obj);
    }

    if (GET_HIT(ch) > GET_MAX_HIT(ch) / 10) {
        obj = read_object(BLOOD_POOL_OBJ, VIRTUAL);
        GET_OBJ_DECOMP(obj) = 3;
    } else {
        obj = read_object(BLOOD_DROP_OBJ, VIRTUAL);
        GET_OBJ_DECOMP(obj) = 2;
    }

    GET_OBJ_VAL(obj, 0) = ch->in_room;
    obj_to_room(obj, ch->in_room);
}

int do_misdirected_move(CharData *actor, int dir) {
    CharData *people;
    char mmsg[MAX_STRING_LENGTH];
    char rmsg[MAX_STRING_LENGTH];

    sprintf(mmsg, "$n %s %s.", movewords(actor, dir, actor->in_room, true), dirs[dir]);
    sprintf(rmsg, "$n's &5illusion &0%s %s.", movewords(actor, dir, actor->in_room, true), dirs[dir]);

    LOOP_THRU_PEOPLE(people, actor) {
        if (actor == people) {
            char_printf(actor, "Your &5illusion&0 {} {}.\n", movewords(actor, dir, actor->in_room, true), dirs[dir]);
        } else if (!AWAKE(people))
            ;
        else if (SEES_THROUGH_MISDIRECTION(people, actor))
            act(rmsg, false, actor, 0, people, TO_VICT);
        else
            act(mmsg, false, actor, 0, people, TO_VICT);
    }

    return 1;
}

bool try_to_sense_arrival(CharData *observer, CharData *mover) {
    if (senses_living(observer, mover, 50)) {
        char_printf(observer, "You sense that a living creature has arrived.\n");
        return true;
    } else {
        return false;
    }
}

bool try_to_sense_departure(CharData *observer, CharData *mover) {
    if (senses_living(observer, mover, 50)) {
        char_printf(observer, "You feel that a living creature has departed.\n");
        return true;
    } else {
        return false;
    }
}

void observe_char_leaving(CharData *observer, CharData *mover, CharData *mount, char *msg, int direction) {
    if (observer == mover || observer == mount || !AWAKE(observer))
        return;

    if (observer == RIDING(mover)) {
        sprintf(buf, "%s you.", msg);
        act(buf, false, mover, 0, observer, TO_VICT);
        return;
    }

    if (mount) {
        /* MOUNTED MOVEMENT */
        if (CAN_SEE(observer, mount) || CAN_SEE(observer, mover)) {
            sprintf(buf, "%s %s.", msg, CAN_SEE(observer, mount) ? GET_NAME(mount) : "something");
            act(buf, false, mover, 0, observer, TO_VICT);
        } else if (CAN_SEE_BY_INFRA(observer, mover) || CAN_SEE_BY_INFRA(observer, mount)) {
            char_printf(observer, "&1&bA {}-sized creature rides {} on a {} mount.&0\n", SIZE_DESC(mover),
                        dirs[direction], SIZE_DESC(mount));
        } else if (!try_to_sense_departure(observer, mount)) {
            /* Try sensing because it's too dark or mover is invis
             * (sneaking and misdirection do not apply when mounted) */
            try_to_sense_departure(observer, mover);
        }
        /* NO MOUNT */
    } else if (EFF_FLAGGED(mover, EFF_MISDIRECTING)) {
        if (SEES_THROUGH_MISDIRECTION(observer, mover))
            act(msg, false, mover, 0, observer, TO_VICT);
    } else if (!CAN_SEE(observer, mover)) {
        if (CAN_SEE_BY_INFRA(observer, mover)) {
            char_printf(observer, "&1&bA {}-sized creature leaves {}.&0\n", SIZE_DESC(mover), dirs[direction]);
        } else {
            /* Try sensing because it's too dark, or mover is invis */
            try_to_sense_departure(observer, mover);
        }
    } else if (!IS_HIDDEN(mover) && !OUTDOOR_SNEAK(mover) && !EFF_FLAGGED(mover, EFF_SNEAK))
        act(msg, false, mover, 0, observer, TO_VICT);
    else if (GET_LEVEL(observer) >= LVL_IMMORT ? GET_LEVEL(observer) >= GET_LEVEL(mover)
                                               : GET_PERCEPTION(observer) >= GET_HIDDENNESS(mover))
        act(msg, false, mover, 0, observer, TO_VICT);
    else
        /* Try sensing because mover is sneaking */
        try_to_sense_departure(observer, mover);
}

void observe_char_arriving(CharData *observer, CharData *mover, CharData *mount, char *mountmsg, char *stdmsg,
                           int direction) {
    if (observer == mover || observer == mount || !AWAKE(observer))
        return;

    if (mount) {
        /* MOUNTED MOVEMENT */
        sprintf(buf2, "%s%s.", mountmsg, CAN_SEE(observer, mount) ? GET_NAME(mount) : "something");
        if (CAN_SEE(observer, mount) || CAN_SEE(observer, mover)) {
            act(buf2, false, mover, 0, observer, TO_VICT);
        } else if (CAN_SEE_BY_INFRA(observer, mover) || CAN_SEE_BY_INFRA(observer, mount)) {
            char_printf(observer, "&1&bA {}-sized creature arrives from {}{}, riding a {} mount.&0\n", SIZE_DESC(mover),
                        (direction < UP ? "the " : ""),
                        (direction == UP     ? "below"
                         : direction == DOWN ? "above"
                                             : dirs[rev_dir[direction]]),
                        SIZE_DESC(mount));
        } else if (!try_to_sense_arrival(observer, mount)) {
            try_to_sense_arrival(observer, mover);
        }
        /* NO MOUNT */
    } else if (!CAN_SEE(observer, mover)) {
        /* Invisibility or darkness - try to see by infra, or sense life. */
        if (CAN_SEE_BY_INFRA(observer, mover)) {
            char_printf(observer, "&1&bA {}-sized creature arrives from {}{}.&0\n", SIZE_DESC(mover),
                        (direction < UP ? "the " : ""),
                        (direction == UP     ? "below"
                         : direction == DOWN ? "above"
                                             : dirs[rev_dir[direction]]));
        } else {
            try_to_sense_arrival(observer, mover);
        }
    } else if (!IS_HIDDEN(mover) && !OUTDOOR_SNEAK(mover) && !EFF_FLAGGED(mover, EFF_SNEAK))
        /* No invisibility, darkness, or sneaking - mover is visible */
        act(stdmsg, false, mover, 0, observer, TO_VICT);
    else if (PRF_FLAGGED(observer, PRF_HOLYLIGHT) ? GET_LEVEL(observer) >= GET_LEVEL(mover)
                                                  : GET_PERCEPTION(observer) >= GET_HIDDENNESS(mover))
        act(stdmsg, false, mover, 0, observer, TO_VICT);
    /* Not visible by any standard means - try to sense life */
    else
        try_to_sense_arrival(observer, mover);
}

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 */

bool do_simple_move(CharData *ch, int dir, int need_specials_check) {
    int flying = 0, levitating = 0;
    bool boat;
    int need_movement, vnum;
    char mmsg[MAX_STRING_LENGTH];
    char tmp[MAX_STRING_LENGTH];
    CharData *observer;

    /* Possible situations:
     *                             Actor           Motivator      Mount
     * 1. ch alone                 ch              ch             NULL
     * 2. ch mounted upon someone  ch              RIDING(ch)     RIDING(ch)
     * 3. ch ridden by someone     RIDDEN_BY(ch)   ch             ch
     *
     * actor      - the one who is *said* to be moving
     * motivator  - the one who is doing the work
     */
    CharData *actor = ch, *motivator = ch, *mount = nullptr;
    int was_in = IN_ROOM(actor);

    /*
     * Check for special routines (North is 1 in command list, but 0 here) Note
     * -- only check if following; this avoids 'double spec-proc' bug
     */
    if (need_specials_check && special(ch, dir + 1, ""))
        return false;

    /* Check in room afterwards in case there was a teleport in the trigger. */
    if (!leave_mtrigger(ch, dir) || IN_ROOM(ch) != was_in)
        return false;
    if (!leave_wtrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in)
        return false;
    if (!leave_otrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in)
        return false;

    if (EVENT_FLAGGED(ch, EVENT_GRAVITY) && !EFF_FLAGGED(ch, EFF_FEATHER_FALL)) {
        char_printf(ch, "You're in free fall!\n");
        return false;
    }

    if (EFF_FLAGGED(ch, EFF_IMMOBILIZED)) {
        char_printf(ch, "You're unable to move!\n");
        return false;
    }

    /* Work out who's the actor and who's the motivator */

    if (RIDING(ch)) {
        /* Sanity check - are you in the same room as your mount? */
        if (IN_ROOM(ch) == IN_ROOM(RIDING(ch)))
            motivator = RIDING(ch);
        else {
            log(LogSeverity::Stat, LVL_IMMORT, "do_simple_move: {} is at {:d}, while mount {} is at {:d}", GET_NAME(ch),
                IN_ROOM(ch), GET_NAME(RIDING(ch)), IN_ROOM(RIDING(ch)));
        }
        mount = motivator;
    } else if (RIDDEN_BY(ch)) {
        /* Sanity check - are you in the same room as your rider? */
        if (IN_ROOM(ch) == IN_ROOM(RIDDEN_BY(ch)))
            actor = RIDDEN_BY(ch);
        else {
            log(LogSeverity::Stat, LVL_IMMORT, "do_simple_move: {} is at {:d}, while rider {} is at {:d}", GET_NAME(ch),
                IN_ROOM(ch), GET_NAME(RIDDEN_BY(ch)), IN_ROOM(RIDDEN_BY(ch)));
        }
        mount = motivator;
    }

    /* Is mount standing and conscious? */
    if (mount) {
        if (GET_STANCE(mount) < STANCE_RESTING) {
            sprintf(buf, "You aren't riding $N anywhere while $E's %s.", stance_types[GET_STANCE(mount)]);
            act(buf, false, actor, 0, mount, TO_CHAR);
            sprintf(buf, "$n tries to ride the %s $D.", stance_types[GET_STANCE(mount)]);
            act(buf, true, actor, 0, mount, TO_ROOM);
            return false;
        }
        if (GET_POS(mount) < POS_STANDING) {
            sprintf(buf, "You can't ride away on $N while $E's %s.", position_types[GET_POS(mount)]);
            act(buf, false, actor, 0, mount, TO_CHAR);
            return false;
        }
        if (GET_STANCE(mount) == STANCE_RESTING) {
            alter_pos(mount, GET_POS(mount), STANCE_ALERT);
            act("$n stops resting and looks ready to head out.", true, mount, 0, 0, TO_ROOM);
        }
    }

    /* BOAT */
    boat = can_travel_on_water(actor) || can_travel_on_water(motivator);

    /* FLYING */
    flying = GET_POS(actor) == POS_FLYING || GET_POS(motivator) == POS_FLYING || GET_LEVEL(actor) >= LVL_IMMORT ||
             GET_LEVEL(motivator) >= LVL_IMMORT;

    /* FEATHER FALL */
    if (!flying && (EFF_FLAGGED(actor, EFF_FEATHER_FALL) || EFF_FLAGGED(motivator, EFF_FEATHER_FALL)))
        levitating = 1;

    /* Determine movement points needed */

    if (GET_LEVEL(motivator) >= LVL_IMMORT)
        need_movement = 0;
    else if (flying)
        need_movement = 1;
    else if (boat && SECT(motivator->in_room) == SECT_SHALLOWS)
        need_movement = 2;
    else
        need_movement = (sectors[SECT(CH_NDEST(motivator, dir))].mv + sectors[SECT(motivator->in_room)].mv) / 2;

    if (levitating && need_movement > 2)
        need_movement = 2;

    /* Do you have enough energy? */

    if (GET_MOVE(motivator) < need_movement) {
        if (need_specials_check && motivator->master)
            char_printf(motivator, "You are too exhausted to follow.\n");
        else
            char_printf(motivator, "You are too exhausted.\n");
        if (mount) {
            if (need_specials_check && motivator->master)
                char_printf(actor, "Your mount is too exhausted to follow.\n");
            else
                char_printf(actor, "Your mount is too exhausted.\n");
        }
        return false;
    }

    /* What to do if a mount attempts to wander */

    if (mount && ch != actor) {

        /* A tamed mount shouldn't wander off while you're mounted upon it */
        if (EFF_FLAGGED(ch, EFF_TAMED))
            char_printf(ch, "You've been tamed.  Now act it!\n");

        /* A non-tamed mount will attempt to buck instead of wandering, during a
         * fight */
        else if (FIGHTING(actor) && movement_bucked(actor, mount)) {
            act("$N rears backwards, throwing you to the ground.", false, actor, 0, mount, TO_CHAR);
            act("You rear backwards, throwing $n to the ground.", false, actor, 0, mount, TO_VICT);
            act("$N rears backwards, throwing $n to the ground.", false, actor, 0, mount, TO_NOTVICT);
            improve_skill(actor, SKILL_RIDING);
            damage(actor, actor, roll_dice(1, 6), -1);
            dismount_char(actor);
        }

        return false;
    }

    /* If load 90% and over, bite the dust. */
    if (!mount && !flying && CURRENT_LOAD(actor) >= 9 && GET_LEVEL(actor) < LVL_IMMORT) {
        act("You stagger about, then fall under your heavy load.", false, actor, 0, 0, TO_CHAR);
        act("$n staggers about, then collapses under $s heavy load.", true, actor, 0, 0, TO_ROOM);
        alter_pos(actor, POS_SITTING, STANCE_ALERT);
        return false;
    }

    /* Charmed critters can't wander off (unless animated) */
    if (EFF_FLAGGED(actor, EFF_CHARM) && !MOB_FLAGGED(actor, MOB_ANIMATED) && actor->master &&
        actor->in_room == actor->master->in_room) {
        char_printf(actor, "The thought of leaving your master makes you weep.\n");
        return false;
    }

    /* check for a wall in room */
    if (wall_block_check(actor, motivator, dir))
        return false;

    /* Fishes and the like can't enter land rooms. */
    if (MOB_FLAGGED(actor, MOB_AQUATIC) && !flying) {
        if (!IS_WATER(actor->in_room)) {
            char_printf(actor, "There isn't enough water here to swim that way!\n");
            act("$n flounders on the ground, gasping for air!", false, actor, 0, 0, TO_ROOM);
            return false;
        }
        if (!IS_WATER(CH_NDEST(actor, dir)) && SECT(CH_NDEST(actor, dir)) != SECT_AIR) {
            char_printf(actor, "You can't swim that way!\n");
            return false;
        }
    } else if (mount && MOB_FLAGGED(mount, MOB_AQUATIC) && !flying) {
        if (!IS_WATER(actor->in_room)) {
            char_printf(actor, "There isn't enough water here to swim that way!\n");
            act("$n flounders on the ground, gasping for air!", false, mount, 0, 0, TO_ROOM);
            return false;
        }
        if (!IS_WATER(CH_NDEST(actor, dir)) && SECT(CH_NDEST(actor, dir)) != SECT_AIR) {
            act("$N can't swim that way!", false, actor, 0, mount, TO_CHAR);
            return false;
        }
    }

    if ((SECT(actor->in_room) == SECT_AIR) || (SECT(CH_NDEST(actor, dir)) == SECT_AIR)) {
        if (!flying && dir == UP) {
            char_printf(actor, "&7Try flapping your wings.&0\n");
            return false;
        }
    }

    /* We need either a boat or fly spell in water rooms */
    if ((SECT(actor->in_room) == SECT_WATER) || (SECT(CH_NDEST(actor, dir)) == SECT_WATER)) {
        if (!boat && !flying) {
            char_printf(actor, "You need a boat or wings to go there.\n");
            return false;
        }
    }

    /* Riding check for each move to see if the rider gets tossed */

    if (mount) {
        improve_skill(actor, SKILL_RIDING);
        if (movement_bucked(actor, mount)) {
            act("$N rears backwards, throwing you to the ground.", false, actor, 0, mount, TO_CHAR);
            act("You rear backwards, throwing $n to the ground.", false, actor, 0, mount, TO_VICT);
            act("$N rears backwards, throwing $n to the ground.", false, actor, 0, mount, TO_NOTVICT);
            dismount_char(actor);
            damage(actor, actor, roll_dice(1, 6), -1);
            return false;
        }
    }

    vnum = CH_VDEST(actor, dir);

    if (ROOM_FLAGGED(CH_NROOM(actor), ROOM_ATRIUM)) {
        if (!House_can_enter(actor, vnum)) {
            char_printf(actor, "That's private property -- no trespassing!\n");
            return false;
        }
    }

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (ROOM_FLAGGED(CH_NDEST(actor, dir), ROOM_TUNNEL)) {
            if (mount) {
                char_printf(actor, "There isn't enough room there, while mounted.\n");
                return false;
            }
            if (num_pc_in_room(CH_DEST(actor, dir)) > 1) {
                char_printf(actor, "There isn't enough room there for more than one person!\n");
                return false;
            }
        }
    }

    /* see if an entry trigger disallows the move */
    if (!entry_mtrigger(actor, vnum))
        return false;
    if (!preentry_wtrigger(CH_DEST(actor, dir), actor, dir))
        return false;

    /* Don't allow people in god rooms (unless following a deity) */
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (ch->master) {
            if (GET_LEVEL(ch->master) < LVL_IMMORT) {
                if (ROOM_FLAGGED(CH_NDEST(ch, dir), ROOM_GODROOM)) {
                    char_printf(ch, "&0A mysterious powerful force pushes you back.&0\n");
                    return false;
                }
            } else {
                if (ch->master->in_room != CH_NDEST(ch, dir)) {
                    if (ROOM_FLAGGED(CH_NDEST(ch, dir), ROOM_GODROOM)) {
                        char_printf(ch, "&0A mysterious powerful force pushes you back.&0\n");
                        return false;
                    }
                }
            }
        } else {
            if (ROOM_FLAGGED(CH_NDEST(ch, dir), ROOM_GODROOM)) {
                char_printf(ch, "&0A mysterious powerful force pushes you back.&0\n");
                return false;
            }
        }
    }

    /* alter the need_movement depending on wind direction and speed */
    if (CH_OUTSIDE(motivator) && (zone_table[world[motivator->in_room].zone]).wind_speed >= WIND_GALE) {
        if ((zone_table[world[motivator->in_room].zone]).wind_dir == (dir + 2) % 4) {
            need_movement *= 2;
            char_printf(actor, "The wind beats hard upon your body as you forge ahead.\n");
        }
    }

    /* If camouflaged and entering a building, snap into visibility */
    if (EFF_FLAGGED(ch, EFF_CAMOUFLAGED) && INDOORS(CH_NDEST(actor, dir))) {
        char_printf(ch, "You reveal yourself as you step indoors.\n");
        effect_from_char(ch, SPELL_NATURES_EMBRACE);
        GET_HIDDENNESS(ch) = 0;
    }

    alter_move(motivator, need_movement);

    if (mount) {
        sprintf(buf2, "You ride %s on %s.\n", dirs[dir], PERS(mount, actor));
        act(buf2, true, actor, 0, 0, TO_CHAR);
        sprintf(mmsg, "$n rides %s on", dirs[dir]);
    } else {
        sprintf(mmsg, "$n %s %s.", movewords(actor, dir, actor->in_room, true), dirs[dir]);
    }

    if (IS_HIDDEN(motivator) || EFF_FLAGGED(motivator, EFF_SNEAK)) {
        if (EFF_FLAGGED(motivator, EFF_SNEAK)) {
            if (!IS_NPC(motivator))
                GET_HIDDENNESS(motivator) = std::max(0l, GET_HIDDENNESS(motivator) - random_number(2, 5));
        }
        /* Camouflage makes you lose minimal hide points per move */
        else if (EFF_FLAGGED(motivator, EFF_CAMOUFLAGED))
            GET_HIDDENNESS(motivator) = std::max(0l, GET_HIDDENNESS(motivator) - random_number(3, 7));
        /* Chance for hiddenness to decrease with a bonus for dex apply
         * and for already sneaking. */
        else if (random_number(1, 101) >
                 GET_SKILL(motivator, SKILL_SNEAK) + dex_app_skill[GET_DEX(motivator)].sneak + 15)
            GET_HIDDENNESS(motivator) = std::max(0l, GET_HIDDENNESS(motivator) - GET_LEVEL(motivator) / 2);
        if (!IS_HIDDEN(motivator))
            effect_from_char(motivator, SPELL_NATURES_EMBRACE);
        if (GET_SKILL(motivator, SKILL_SNEAK))
            improve_skill(motivator, SKILL_SNEAK);
        if (GET_SKILL(motivator, SKILL_STEALTH))
            improve_skill(motivator, SKILL_STEALTH);
    }

    LOOP_THRU_PEOPLE(observer, actor) { observe_char_leaving(observer, actor, mount, mmsg, dir); }

    /* BLOOD TRAILS (pk only) */
    if (pk_allowed)
        spill_blood(actor);

    /* If the room is affected by a circle of fire, damage the person */
    if (ROOM_EFF_FLAGGED(actor->in_room, ROOM_EFF_CIRCLE_FIRE) && !EFF_FLAGGED(actor, EFF_NEGATE_HEAT)) {
        if (GET_LEVEL(actor) < LVL_IMMORT) {
            mag_damage(GET_LEVEL(actor) + random_number(1, 10), actor, actor, SPELL_CIRCLE_OF_FIRE, SAVING_SPELL);
        }
        if (mount && GET_LEVEL(mount) < LVL_IMMORT) {
            mag_damage(GET_LEVEL(mount) + random_number(1, 10), mount, mount, SPELL_CIRCLE_OF_FIRE, SAVING_SPELL);
        }
        if (DECEASED(actor) || (mount && DECEASED(mount)))
            return false;
    }

    /* At last, it is time to actually move */
    char_from_room(actor);
    char_to_room(actor, world[was_in].exits[dir]->to_room);
    if (mount) {
        char_from_room(mount);
        char_to_room(mount, actor->in_room);
        look_at_room(mount, true);

        sprintf(tmp, "$n arrives from %s%s, riding ", (dir < UP ? "the " : ""),
                (dir == UP     ? "below"
                 : dir == DOWN ? "above"
                               : dirs[rev_dir[dir]]));
    } else {
        sprintf(buf, "$n %s from %s%s.", movewords(actor, dir, actor->in_room, false), (dir < UP ? "the " : ""),
                (dir == UP     ? "below"
                 : dir == DOWN ? "above"
                               : dirs[rev_dir[dir]]));
    }

    LOOP_THRU_PEOPLE(observer, actor) { observe_char_arriving(observer, actor, mount, tmp, buf, dir); }

    /* If the room is affected by a circle of fire, damage the person */
    /* if it dies, don't do anything else */
    /* Note, now that we've officially moved, we should return true after this point since otherwise their followers */
    /* wouldn't follow correctly. */
    if (ROOM_EFF_FLAGGED(actor->in_room, ROOM_EFF_CIRCLE_FIRE) && !EFF_FLAGGED(actor, EFF_NEGATE_HEAT)) {
        mag_damage(GET_LEVEL(actor) + random_number(1, 5), actor, actor, SPELL_CIRCLE_OF_FIRE, SAVING_SPELL);
        if (DECEASED(actor))
            return true;
    }

    if (actor->desc != nullptr)
        look_at_room(actor, false);

    if (!greet_mtrigger(actor, dir)) {
        if (DECEASED(actor))
            return true;
        char_from_room(actor);
        char_to_room(actor, was_in);
        look_at_room(actor, true);
    }

    /* Check for post-entry triggers */

    postentry_wtrigger(actor, dir);
    /* Please leave this death check, in case anything substantive ever
     * gets added beyond this point. The trigger may kill the actor. */
    if (DECEASED(actor))
        return true;

    return true;
}

bool perform_move(CharData *ch, int dir, int need_specials_check, bool misdirection) {
    room_num was_in, to_room;
    FollowType *k, *next;

    if (ch == nullptr || dir < 0 || dir >= NUM_OF_DIRS)
        return 0;
    else if (!misdirection && !check_can_go(ch, dir, false))
        return 0;
    else {
        if (!ch->followers) {
            if (misdirection)
                return do_misdirected_move(ch, dir);
            else
                return do_simple_move(ch, dir, need_specials_check);
        }

        /* Could the followers see the leader before he leaves? */
        for (k = ch->followers; k; k = k->next)
            if (CAN_SEE_MOVING(k->follower, ch))
                k->can_see_master = true;
            else
                k->can_see_master = false;

        was_in = ch->in_room;
        /* During misdirection, there might not even be an exit... */
        if (!CH_EXIT(ch, dir))
            to_room = NOWHERE;
        else
            to_room = CH_NDEST(ch, dir);

        if (misdirection)
            do_misdirected_move(ch, dir);
        else if (!do_simple_move(ch, dir, need_specials_check))
            /* Failed the move for some reason.  No one follows. */
            return 0;

        for (k = ch->followers; k; k = next) {
            next = k->next;
            if (k->follower->in_room != to_room && k->follower->in_room == was_in &&
                GET_POS(k->follower) >= POS_STANDING && !FIGHTING(k->follower) && !CASTING(k->follower) &&
                k->can_see_master) {
                if (CONFUSED(k->follower) && random_number(1, 5) == 1) {
                    act("&5You tried to follow $N&0&5, but got all turned around.&0", false, k->follower, 0, ch,
                        TO_CHAR);
                    perform_move(k->follower, random_number(0, 5), 1, false);
                    if (IN_ROOM(k->follower) != IN_ROOM(ch))
                        act("&3Oops!  $n&0&3 seems to have wandered off again.&0", false, k->follower, 0, ch, TO_VICT);
                } else {
                    sprintf(buf, "You follow $N %s.\n", dirs[dir]);
                    act(buf, false, k->follower, 0, ch, TO_CHAR);
                    if (perform_move(k->follower, dir, 1, false) && IS_MOB(k->follower) && PLAYERALLY(k->follower)) {
                        if (GET_MOVE(k->follower) == 0) {
                            act("$N staggers, gasping for breath.  They don't look like they could walk another step.",
                                false, ch, 0, k->follower, TO_CHAR);
                            act("You gasp for breath.  You don't think you could move another step.", false, ch, 0,
                                k->follower, TO_VICT);
                            act("$N gasps for breath.  They don't look like they could walk another step.", false, ch,
                                0, k->follower, TO_NOTVICT);
                        } else if (100 * GET_MOVE(k->follower) / std::max(1, GET_MAX_MOVE(k->follower)) < 10) {
                            act("$N is panting loudly.", false, ch, 0, k->follower, TO_CHAR);
                            act("You pant loudly, attempting to catch your breath.", false, ch, 0, k->follower,
                                TO_VICT);
                            act("$N pants loudly, attempting to catch their breath.", false, ch, 0, k->follower,
                                TO_NOTVICT);
                        } else if (100 * GET_MOVE(k->follower) / std::max(1, GET_MAX_MOVE(k->follower)) < 20) {
                            act("$N is starting to pant softly.", false, ch, 0, k->follower, TO_CHAR);
                            act("You pant softly as exhaustion looms.", false, ch, 0, k->follower, TO_VICT);
                            act("$N starts panting softly.", false, ch, 0, k->follower, TO_NOTVICT);
                        } else if (100 * GET_MOVE(k->follower) / std::max(1, GET_MAX_MOVE(k->follower)) < 30) {
                            act("$N is looking tired.", false, ch, 0, k->follower, TO_CHAR);
                            act("You are starting to feel tired.", false, ch, 0, k->follower, TO_VICT);
                        }
                    }
                }
            }
        }
        return 1;
    }
    return 0;
}

ACMD(do_move) {
    /*
     * This is basically a mapping of cmd numbers to perform_move indices.
     * It cannot be done in perform_move because perform_move is called
     * by other functions which do not require the remapping.
     */

    int misdir;

    one_argument(argument, arg);

    if (*arg && EFF_FLAGGED(ch, EFF_MISDIRECTION) && subcmd >= SCMD_STAY && subcmd <= SCMD_DOWN) {

        if (RIDING(ch) || RIDDEN_BY(ch)) {
            char_printf(ch, "You can't misdirect people while riding.\n");
            return;
        }

        if (!strncasecmp("st", arg, 2)) {
            /* Misdirection is "stay" */
            misdir = -1;
        } else {
            misdir = parse_direction(arg);
            if (misdir < 0) {
                char_printf(ch, "That isn't a direction.\n");
                return;
            }
        }

        if (misdir > -1)
            perform_move(ch, misdir, 0, true);
        SET_FLAG(EFF_FLAGS(ch), EFF_MISDIRECTING);
        /* Send confused people off in a random direction. */
        if (CONFUSED(ch) && random_number(0, 1) == 0) {
            char_printf(ch, "&5You are confused!&0\n");
            subcmd = SCMD_STAY + random_number(1, 6);
        }
        perform_move(ch, subcmd - 1, 0, false);
        REMOVE_FLAG(EFF_FLAGS(ch), EFF_MISDIRECTING);
        return;
    }

    /* Send confused people off in a random direction. */
    if (CONFUSED(ch) && random_number(0, 1) == 0) {
        char_printf(ch, "&5You are confused!&0\n");
        subcmd = SCMD_STAY + random_number(1, 6);
    }

    switch (subcmd) {
    case SCMD_STAY:
        char_printf(ch, "Ok, you stay put.\n");
        break;
    case SCMD_NORTH:
    case SCMD_EAST:
    case SCMD_SOUTH:
    case SCMD_WEST:
    case SCMD_UP:
    case SCMD_DOWN:
        perform_move(ch, subcmd - 1, 0, false);
        break;
    default:
        if (argument && *arg && (subcmd = searchblock(arg, dirs, 0)) >= 0)
            perform_move(ch, subcmd, 0, false);
        else
            char_printf(ch, "Which way do you want to go?\n");
    }
}

int find_door(CharData *ch, const char *name, const char *dirname, const char *cmdname, int quiet) {
    int dir;
    Exit *exit;

    if (*dirname) { /* a direction was specified */

        if ((dir = parse_direction(dirname)) == -1) { /* Partial Match */
            if (!quiet)
                char_printf(ch, "That's not a direction.\n");
            return -1;
        }

        if ((exit = CH_EXIT(ch, dir)) && !EXIT_IS_HIDDEN(exit)) {
            if (exit_has_keyword(exit, name))
                return dir;
            else {
                if (!quiet) {
                    char_printf(ch, "I see no {} there.\n", name);
                }
                return -1;
            }
        } else {
            if (!quiet)
                char_printf(ch, "I really don't see how you can close anything there.\n");
            return -1;
        }
    } else { /* try to locate the keyword */
        if (!name || !*name) {
            if (!quiet) {
                char_printf(ch, "What is it you want to {}?\n", cmdname);
            }
            return -1;
        }
        for (dir = 0; dir < NUM_OF_DIRS; dir++) {
            if ((exit = CH_EXIT(ch, dir)) && !EXIT_IS_HIDDEN(exit) && exit_has_keyword(exit, name))
                return dir;
        }
        if (!quiet) {
            char_printf(ch, "There doesn't seem to be {} {} here.\n", AN(name), name);
        }
        return -1;
    }
}

/* For the following four functions - for opening, closing, locking, and
 * unlocking objects - it is assumed that the object is a closeable
 * container. The caller should have verified this. */

void open_object(CharData *ch, ObjData *obj, bool quiet) {
    /* The object must:
     *
     * 1. Be closed
     * 2. Be unlocked
     */

    if (OBJ_IS_OPEN(obj)) {
        if (!quiet && ch)
            char_printf(ch, "It's already open.\n");
        return;
    }

    if (!OBJ_IS_UNLOCKED(obj)) {
        if (!quiet && ch) {
            char_printf(ch, "It seems to be locked.\n");
            act("$n tries to open $p, but it's locked.", false, ch, obj, 0, TO_ROOM);
        }
        return;
    }

    /* Success. */

    REMOVE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_CLOSED);

    /* Feedback. */

    if (ch && !quiet) {
        act("$n opens $p.", false, ch, obj, 0, TO_ROOM);
        char_printf(ch, OK);
    }
}

void close_object(CharData *ch, ObjData *obj, bool quiet) {
    /* The object must:
     *
     * 1. Be open
     */

    if (!OBJ_IS_OPEN(obj)) {
        if (!quiet && ch)
            char_printf(ch, "It's already closed.\n");
        return;
    }

    /* Success. */

    SET_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_CLOSED);

    /* Feedback. */

    if (ch && !quiet) {
        act("$n closes $p.", false, ch, obj, 0, TO_ROOM);
        char_printf(ch, OK);
    }
}

void unlock_object(CharData *ch, ObjData *obj, bool quiet) {
    int keyvnum;
    ObjData *key = nullptr;

    /* The object must:
     *
     * 1. Be closed
     * 2. Be locked
     */

    if (OBJ_IS_OPEN(obj)) {
        if (!quiet && ch)
            char_printf(ch, "It's already open!\n");
        return;
    }

    /* If you aren't a god, you may need a key. */

    if (ch && GET_LEVEL(ch) < LVL_IMMORT) {
        keyvnum = GET_OBJ_VAL(obj, VAL_CONTAINER_KEY);
        if (keyvnum < 0) {
            if (!quiet)
                char_printf(ch, "Odd - you can't seem to find a keyhole.\n");
            return;
        }
        if (!(key = carried_key(ch, keyvnum))) {
            if (!quiet)
                char_printf(ch, "You don't seem to have the proper key.\n");
            return;
        }
    }

    /* Now that the key is known to be in hand, see whether the object
     * is already unlocked. */

    if (OBJ_IS_UNLOCKED(obj)) {
        if (!quiet && ch) {
            if (key) {
                act("You insert $P, only to find that $p isn't locked.", false, ch, obj, key, TO_CHAR);
                act("$n inserts $P into $p, only to find that it isn't locked.", false, ch, obj, key, TO_ROOM);
            } else {
                char_printf(ch, "Oh... it wasn't locked.\n");
            }
        }
        return;
    }

    /* Success. */

    REMOVE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_LOCKED);

    /* Feedback. */

    if (ch && !quiet) {
        if (key) {
            act("$n unlocks $p with $P.", false, ch, obj, key, TO_ROOM);
            act("You unlock $p with $P.", false, ch, obj, key, TO_CHAR);
        } else {
            act("$n unlocks $p.", false, ch, obj, 0, TO_ROOM);
            act("You unlock $p.", false, ch, obj, 0, TO_CHAR);
        }
    }
}

void lock_object(CharData *ch, ObjData *obj, bool quiet) {
    int keyvnum;
    ObjData *key = nullptr;

    /* The object must:
     *
     * 1. Be closed
     * 2. Be unlocked
     */

    if (OBJ_IS_OPEN(obj)) {
        if (!quiet && ch)
            char_printf(ch, "You'll need to close it first.\n");
        return;
    }

    /* If you aren't a god, you may need a key. */

    if (ch && GET_LEVEL(ch) < LVL_IMMORT) {
        keyvnum = GET_OBJ_VAL(obj, VAL_CONTAINER_KEY);
        if (keyvnum < 0) {
            if (!quiet)
                char_printf(ch, "Odd - you can't seem to find a keyhole.\n");
            return;
        }
        if (!(key = carried_key(ch, keyvnum))) {
            if (!quiet)
                char_printf(ch, "You don't seem to have the proper key.\n");
            return;
        }
    }

    /* Now that the key is known to be in hand, see whether the object
     * is already locked. */

    if (!OBJ_IS_UNLOCKED(obj)) {
        if (!quiet && ch) {
            if (key) {
                act("You insert $P, only to find that $p is already locked.", false, ch, obj, key, TO_CHAR);
                act("$n inserts $P into $p, only to find that it's already locked.", false, ch, obj, key, TO_ROOM);
            } else {
                char_printf(ch, "It seems to be locked already.\n");
            }
        }
        return;
    }

    /* Success. */

    SET_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_LOCKED);

    /* Feedback. */

    if (ch && !quiet) {
        if (key) {
            act("$n inserts $P into $p and locks it.", false, ch, obj, key, TO_ROOM);
            act("You insert $P into $p and lock it.", false, ch, obj, key, TO_CHAR);
        } else {
            act("$n locks $p.", false, ch, obj, 0, TO_ROOM);
            act("You lock $p.", false, ch, obj, 0, TO_CHAR);
        }
    }
}

void pick_object(CharData *ch, ObjData *obj) {
    ObjData *key = nullptr;

    if (!ch) {
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: pick_object() called with no actor");
        return;
    }

    /* The object must:
     *
     * 1. Be closed
     * 2. Be locked
     * 3. Be pickable
     */

    if (OBJ_IS_OPEN(obj)) {
        char_printf(ch, "It's already open!\n");
        return;
    }

    if (OBJ_IS_UNLOCKED(obj)) {
        char_printf(ch, "Oh... it wasn't locked, after all.\n");
        act("$n sets to picking $p, but soon realizes that it isn't locked.", false, ch, obj, 0, TO_ROOM);
        return;
    }

    if (OBJ_IS_PICKPROOF(obj)) {
        char_printf(ch, "It resists your attempts to pick it.\n");
        act("$n sets to picking $p, but soon gives up.", false, ch, obj, 0, TO_ROOM);
        return;
    }

    /* Try your skill. */

    if (random_number(1, 101) > GET_SKILL(ch, SKILL_PICK_LOCK)) {
        char_printf(ch, "You failed to pick the lock.\n");
        improve_skill(ch, SKILL_PICK_LOCK);
        return;
    }

    /* Success. */

    REMOVE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_LOCKED);

    /* Feedback. */

    act("$n skillfully picks the lock on $p.", false, ch, obj, key, TO_ROOM);
    char_printf(ch, "You skillfully pick the lock.\n");
}

void perform_door_action(CharData *ch, int subcmd, int door) {
    if (!SOLIDCHAR(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You can't operate doors in your {}{}&0 state.\n", COMPOSITION_COLOR(ch),
                    COMPOSITION_ADJECTIVE(ch));
        return;
    }

    if (!door_mtrigger(ch, subcmd, door) || !door_wtrigger(ch, subcmd, door))
        return;

    switch (subcmd) {
    case SCMD_OPEN:
        open_door(ch, CH_NROOM(ch), door, false);
        break;
    case SCMD_CLOSE:
        close_door(ch, CH_NROOM(ch), door, false);
        break;
    case SCMD_UNLOCK:
        unlock_door(ch, CH_NROOM(ch), door, false);
        break;
    case SCMD_LOCK:
        lock_door(ch, CH_NROOM(ch), door, false);
        break;
    case SCMD_PICK:
        pick_door(ch, CH_NROOM(ch), door);
        break;
    default:
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: perform_door_action() called with invalid subcommand {}", subcmd);
        char_printf(ch, "Sorry, there's been an internal error.\n");
        break;
    }
}

ACMD(do_gen_door) {
    int door = -1, bits;
    char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
    ObjData *obj = nullptr;
    CharData *victim = nullptr;

    if (FIGHTING(ch)) {
        char_printf(ch, "You are too busy fighting to concentrate on that right now.\n");
        return;
    }

    skip_spaces(&argument);
    if (!*argument) {
        sprintf(buf, "%s what?\n", cmd_door[subcmd]);
        char_printf(ch, cap_by_color(buf));
        return;
    }

    two_arguments(argument, type, dir);

    /* Identify the thing to be manipulated.
     * If a direction is specified, or no such object could be found, we'll
     * try to find a door. */

    if (*dir || !(bits = generic_find(type, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))) {
        door = find_door(ch, type, dir, cmd_door[subcmd], false);
        if (door > -1)
            perform_door_action(ch, subcmd, door);
        return;
    }

    if (!obj)
        return;

    /* If you've named a board and you're trying to lock/unlock
     * it, see if you're allowed to do so. */
    if (GET_OBJ_TYPE(obj) == ITEM_BOARD && (subcmd == SCMD_LOCK || subcmd == SCMD_UNLOCK)) {
        BoardData *brd = board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER));
        if (!brd || !has_board_privilege(ch, brd, BPRIV_LOCK))
            char_printf(ch, "You can't figure out how.\n");
        else if (subcmd == SCMD_LOCK) {
            if (brd->locked)
                char_printf(ch, "{} is already locked.\n", obj->short_description);
            else
                char_printf(ch, "You lock {}, preventing others from posting.\n", obj->short_description);
            brd->locked = true;
        } else {
            if (brd->locked)
                char_printf(ch, "You unlock {}, allowing others to post.\n", obj->short_description);
            else
                char_printf(ch, "{} is already unlocked.\n", obj->short_description);
            brd->locked = false;
        }
        return;
    }

    /* If you've named an object that doesn't open/close, you might have
     * intended to manipulate a door. */

    if (!OBJ_IS_OPENABLE(obj)) {
        door = find_door(ch, type, dir, cmd_door[subcmd], true);
        if (door >= 0) {
            perform_door_action(ch, subcmd, door);
        } else {
            char_printf(ch, "You can't {} that.\n", cmd_door[subcmd]);
        }
        return;
    }

    /* If you're made of fluid, the only way you can open/close/etc. something
     * is if it's an object in your inventory. */
    if (!SOLIDCHAR(ch) && obj->carried_by != ch && GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You can't manipulate solid objects in your {}{}&0 state.\n", COMPOSITION_COLOR(ch),
                    COMPOSITION_ADJECTIVE(ch));
        return;
    }

    switch (subcmd) {
    case SCMD_OPEN:
        open_object(ch, obj, false);
        break;
    case SCMD_CLOSE:
        close_object(ch, obj, false);
        break;
    case SCMD_UNLOCK:
        unlock_object(ch, obj, false);
        break;
    case SCMD_LOCK:
        lock_object(ch, obj, false);
        break;
    case SCMD_PICK:
        pick_object(ch, obj);
        break;
    default:
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: do_gen_door() called with invalid subcommand {}", subcmd);
        char_printf(ch, "Sorry, there's been an internal error.\n");
        break;
    }
}

ACMD(do_enter) {
    ObjData *obj = nullptr;
    FollowType *k;
    int i, rnum;

    if (FIGHTING(ch)) {
        char_printf(ch, "You are too busy fighting to leave right now.\n");
        return;
    }

    one_argument(argument, arg);

    /* Enter without args: try to enter an adjacent building. */
    if (!*arg) {
        if (ROOM_FLAGGED(CH_NROOM(ch), ROOM_INDOORS))
            char_printf(ch, "You are already indoors.\n");
        else
            for (i = 0; i < NUM_OF_DIRS; ++i)
                if (CH_EXIT(ch, i) && !EXIT_IS_CLOSED(CH_EXIT(ch, i)) && ROOM_FLAGGED(CH_NDEST(ch, i), ROOM_INDOORS)) {
                    if (CONFUSED(ch)) {
                        char_printf(ch, "&5You are confused!&0\n");
                        i = random_number(0, 5);
                    }
                    perform_move(ch, i, 1, false);
                    return;
                }
        char_printf(ch, "You can't seem to find anything to enter.\n");
        return;
    }

    if (!(obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, arg)))) {
        char_printf(ch, "There is no {} here.\n", arg);
        return;
    }

    if (GET_OBJ_TYPE(obj) != ITEM_PORTAL) {
        char_printf(ch, "You can't enter that!\n");
        return;
    }

    if (GET_OBJ_LEVEL(obj) > GET_LEVEL(ch)) {
        act("You are not experienced enough to use $p.", false, ch, obj, 0, TO_CHAR);
        return;
    }

    if ((rnum = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DESTINATION))) == NOWHERE) {
        act("$p flares for a second then dies down.&0", false, ch, obj, 0, TO_ROOM);
        char_printf(ch, "It flares and then dies down - it must be broken!\n");
        return;
    }

    /* Some special cases */
    if (GET_OBJ_VNUM(obj) == OBJ_VNUM_HEAVENSGATE) {
        if (GET_ALIGNMENT(ch) < 500 && GET_ALIGNMENT(ch) > -500) {
            act("&0Upon attempting to enter $p&0, you are pushed back by a powerful "
                "force.",
                false, ch, obj, 0, TO_CHAR);
            act("&0Upon attempting to enter $p&0, $n&0 is pushed back by a powerful "
                "force.",
                true, ch, obj, 0, TO_ROOM);
            return;
        } else if (GET_ALIGNMENT(ch) <= -500) {
            act("&9&bUpon entering $p&9&b, you begin to &1burn&9... Your screams can "
                "be heard throughtout the &7heavens&9...&0",
                false, ch, obj, 0, TO_CHAR);
            act("&9&b$n&9&b enters $p&9&b... Following a &0&1blood&9&b curdling "
                "scream that spans the &7heavens&9, $n&9&b flails back out of the "
                "tunnel, &1&bon fire!&0",
                false, ch, obj, 0, TO_ROOM);
            damage(ch, ch, abs(GET_ALIGNMENT(ch)) / 10, TYPE_SUFFERING);
            SET_FLAG(EFF_FLAGS(ch), EFF_ON_FIRE);
            return;
        }
    } else if (GET_OBJ_VNUM(obj) == OBJ_VNUM_HELLGATE) {
        if (GET_ALIGNMENT(ch) < 500 && GET_ALIGNMENT(ch) > -500) {
            act("&0Upon attempting to enter $p&0, you are pushed back by a powerful "
                "force.",
                false, ch, obj, 0, TO_CHAR);
            act("&0Upon attempting to enter $p&0, $n&0 is pushed back by a powerful "
                "force.",
                true, ch, obj, 0, TO_ROOM);
            return;
        } else if (GET_ALIGNMENT(ch) >= 500) {
            act("&9&bUpon catching a glimpse of &1hell&9 itself, your &0&5mind&9&b "
                "twists and distorts... &0",
                false, ch, obj, 0, TO_CHAR);
            act("&9&b$n&9&b enters $p&9&b... Strange moaning sounds can be heard as "
                "$n&9&b wanders back out, twitching and drooling on $mself.&0",
                false, ch, obj, 0, TO_ROOM);
            damage(ch, ch, abs(GET_ALIGNMENT(ch)) / 10, TYPE_SUFFERING);
            mag_affect(70, ch, ch, SPELL_INSANITY, SAVING_SPELL, CAST_SPELL);
            mag_affect(70, ch, ch, SPELL_DISEASE, SAVING_SPELL, CAST_SPELL);
            return;
        }
    }

    /* Display entry message. */
    if (GET_OBJ_VAL(obj, VAL_PORTAL_ENTRY_MSG) >= 0) {
        for (i = 0; i < GET_OBJ_VAL(obj, VAL_PORTAL_ENTRY_MSG) && *portal_entry_messages[i] != '\n'; ++i)
            ;
        if (*portal_entry_messages[i] != '\n')
            act(portal_entry_messages[i], true, ch, obj, 0, TO_ROOM);
    }

    /* Display character message. */
    if (GET_OBJ_VAL(obj, 2) >= VAL_PORTAL_CHAR_MSG) {
        for (i = 0; i < GET_OBJ_VAL(obj, VAL_PORTAL_CHAR_MSG) && *portal_character_messages[i] != '\n'; ++i)
            ;
        if (*portal_character_messages[i] != '\n')
            act(portal_character_messages[i], true, ch, obj, 0, TO_CHAR);
    }

    char_from_room(ch);
    char_to_room(ch, rnum);

    /* Display exit message. */
    if (GET_OBJ_VAL(obj, VAL_PORTAL_EXIT_MSG) >= 0) {
        for (i = 0; i < GET_OBJ_VAL(obj, VAL_PORTAL_EXIT_MSG) && *portal_exit_messages[i] != '\n'; ++i)
            ;
        if (*portal_exit_messages[i] != '\n')
            act(portal_exit_messages[i], true, ch, obj, 0, TO_ROOM);
    }

    look_at_room(ch, true);

    if (RIDING(ch)) {
        char_from_room(RIDING(ch));
        char_to_room(RIDING(ch), rnum);
        look_at_room(RIDING(ch), true);
    }

    for (k = ch->followers; k; k = k->next) {
        if (IS_PET(k->follower) && k->follower->master == ch) {
            char_from_room(k->follower);
            char_to_room(k->follower, rnum);
            look_at_room(k->follower, true);
        }
    }
}

ACMD(do_leave) {
    int door;

    if (FIGHTING(ch)) {
        char_printf(ch, "You are too busy fighting to leave!\n");
        return;
    }
    if (!ROOM_FLAGGED(CH_NROOM(ch), ROOM_INDOORS))
        char_printf(ch, "You are outside.. where do you want to go?\n");
    else {
        for (door = 0; door < NUM_OF_DIRS; door++)
            if (CH_NDEST(ch, door) != NOWHERE)
                if (!EXIT_IS_CLOSED(CH_EXIT(ch, door)) && !ROOM_FLAGGED(CH_NDEST(ch, door), ROOM_INDOORS)) {
                    if (CONFUSED(ch)) {
                        char_printf(ch, "&5You are confused!&0\n");
                        door = random_number(0, 5);
                    }
                    perform_move(ch, door, 1, false);
                    return;
                }
        char_printf(ch, "I see no obvious exits to the outside.\n");
    }
}

ACMD(do_doorbash)
#define EXITK(room, dir) (world[room].exits[dir])
#define OPEN_DOORK(room, door) (TOGGLE_BIT(EXITK(room, door)->exit_info, EX_CLOSED))
{
    char arg[MAX_INPUT_LENGTH];
    int dir = 0, chance, probability, dam;
    Exit *exit;
    RoomData *dest;
    room_num ndest;
    int was_in = IN_ROOM(ch);

    chance = random_number(0, 101);

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (FIGHTING(ch)) {
            char_printf(ch, "You can't take time out to do that just yet!\n");
            return;
        }
        if (GET_SIZE(ch) < SIZE_LARGE) {
            char_printf(ch, "&0You do not feel massive enough!&0\n");
            return;
        }
        if (!GET_SKILL(ch, SKILL_DOORBASH)) {
            char_printf(ch, "You don't have that skill.\n");
            return;
        }
        probability = chance + GET_LEVEL(ch);
    } else {
        probability = 101;
    }

    one_argument(argument, arg);
    if (!*arg) {
        char_printf(ch, "What direction?\n");
        return;
    }

    if ((dir = parse_direction(arg)) < 0) {
        char_printf(ch, "That isn't a valid direction.\n");
        return;
    }

    if (CONFUSED(ch) && random_number(0, 1) == 0) {
        char_printf(ch, "&5You are confused!&0\n");
        dir = random_number(0, 5);
    }

    /* There can be walls even where there aren't exits.  So now, prior to
     * checking for an exit, we check to see if you're bashing a wall. */

    /* Returns true if you ran into a wall, in which case our work here is done.
     */
    if (wall_charge_check(ch, dir))
        return;

    exit = CH_EXIT(ch, dir);
    if (!exit || EXIT_NDEST(exit) == NOWHERE || EXIT_IS_HIDDEN(exit)) {
        char_printf(ch, "Even YOU couldn't bash through that!\n");
        return;
    }

    dest = EXIT_DEST(exit);
    ndest = EXIT_NDEST(exit);

    /* Is the way open already? */
    if (EXIT_IS_OPEN(exit)) {
        char_printf(ch, "There is no obstruction!\n");
        return;
    }

    /* Is the exit broken? */
    if (exit->keyword == nullptr) {
        log(LogSeverity::Warn, LVL_GOD, "SYSERR:act.item.c:do_doorbash():A one sided door in room {:d}",
            world[ch->in_room].vnum);
        char_printf(ch, "This exit seems broken.   Please tell a god.\n");
        return;
    } else if (!EXIT_IS_DOOR(exit)) {
        log(LogSeverity::Warn, LVL_GOD, "SYSERR:act.item.c:do_doorbash():A closed nondoor exit in room {:d}",
            world[ch->in_room].vnum);
        char_printf(ch, "This exit seems broken.   Please tell a god.\n");
        return;
    }

    /* Established:
     *  - There is an exit that way
     *  - It goes to a room
     *  - It is not hidden
     *  - It is not broken
     *  - It is closed or locked (distinct in this mud...)
     */

    if (EXIT_IS_LOCKED(exit) && (EXIT_IS_PICKPROOF(exit) || probability < 80)) {
        /* You failed, or it was unbashable. */
        char_printf(ch, "You CHARGE at the {} &0but merely bounce off!&0\n", exit_name(exit));
        sprintf(buf, "$n &0CHARGES at the %s&0 and literally bounces off!", exit_name(exit));
        act(buf, false, ch, 0, 0, TO_ROOM);

        if (GET_LEVEL(ch) < LVL_IMMORT) {
            /* You're going to get hurt... */
            dam = ((chance / 10) * (GET_LEVEL(ch) / 10)) + GET_LEVEL(ch);
            /* But you won't die... */
            if (GET_HIT(ch) - dam < -5)
                dam = GET_HIT(ch) + 5;
            hurt_char(ch, nullptr, dam, true);
            /* You fell to a sitting position (unless you were knocked out) */
            if (GET_POS(ch) >= POS_STANDING)
                alter_pos(ch, POS_SITTING, STANCE_ALERT);
            WAIT_STATE(ch, PULSE_VIOLENCE * 3);
        }

        room_printf(ndest, "There is a *CRASH* and {} shudders a bit.\n", exit_name(exit));
        return;
    }

    /* Success */
    OPEN_DOORK(ch->in_room, dir);

    sprintf(buf, "&0$n &0*CRASHES* through the %s&0!", exit_name(exit));
    act(buf, false, ch, 0, 0, TO_ROOM);
    char_printf(ch, "&0You *CHARGE* at the {} &0and crash through it!&0\n", exit_name(exit));

    if (dest->exits[rev_dir[dir]]) {
        OPEN_DOORK(ndest, rev_dir[dir]);
    }

    /* This next bit is tricky. I wans to call perform_move so it will handle
     * movement points, circle of fire, etc., but I need to send a message
     * WITH this guy as the subject first. */

    /* The things we do to send a message to the destination room... */
    char_from_room(ch);
    char_to_room(ch, ndest);
    sprintf(buf, "&bSplinters and dust fly as $n &0&b*CRASHES* into the room!&0");
    act(buf, false, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, was_in);

    /* Wish this wouldn't send a message, but there would be significant coding to
     * replicate the movement here */
    perform_move(ch, dir, 1, false);
    WAIT_STATE(ch, PULSE_VIOLENCE);
}

void char_drag_entry(CharData *ch, CharData *vict) {
    act("&3$n&3 drags $N&3 behind $m.&0", true, ch, 0, vict, TO_NOTVICT);

    /* Get awakened by being dragged? */
    if (GET_STANCE(vict) == STANCE_SLEEPING && !EFF_FLAGGED(vict, EFF_SLEEP)) {
        GET_STANCE(vict) = STANCE_ALERT;

        act("&3You are rudely awakened, and discover $N&3 dragging you along!", false, vict, 0, ch, TO_CHAR);
        act("&3$n&3 awakes from $s slumber and looks around.&0", true, vict, 0, 0, TO_ROOM);
    }

    if (AWAKE(vict))
        look_at_room(vict, false);
    else
        char_printf(vict, "Your dreams grow bumpy, as if someone were dragging you...\n");
}

ACMD(do_drag) {
    int from_room, to_room, move_cost, found, dir;
    CharData *tch = nullptr;
    ObjData *tobj = nullptr;
    ObjData *portal = nullptr;

    argument = one_argument(argument, arg);
    skip_spaces(&argument);

    if (!*arg || !*argument) {
        char_printf(ch, "Drag what? Where?\n");
        return;
    }

    if (GET_POS(ch) != POS_STANDING) {
        char_printf(ch, "You don't have the proper leverage to do that.  Try standing.\n");
        return;
    }

    /*
     * Start by figuring out what (or whom) we're dragging.
     */
    if (!(found = generic_find(arg, FIND_OBJ_ROOM | FIND_CHAR_ROOM, ch, &tch, &tobj))) {
        char_printf(ch, "Can't find that!\n");
        return;
    }

    /* If you're made of fluid, you can only drag a similar person */
    /* (we don't do illusory here because that would give it away
     * also, you might think they're substantial-feeling because that's
     * part of the illusion) */
    if (tobj && !RIGID(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You can't handle solid objects in your {}{}&0 condition.\n", COMPOSITION_COLOR(ch),
                    COMPOSITION_ADJECTIVE(ch));
        return;
    } else if (tch && GET_LEVEL(ch) < LVL_IMMORT) {
        if (RIGID(ch) && !RIGID(tch)) {
            sprintf(buf, "$N's %s%s&0 seems to pass right through your fingers.", COMPOSITION_COLOR(tch),
                    COMPOSITION_MASS(tch));
            act(buf, false, ch, 0, tch, TO_CHAR);
            sprintf(buf, "$n tries to grab $N, but $e can't get a grip on $N's %s%s&0 flesh.", COMPOSITION_COLOR(tch),
                    COMPOSITION_ADJECTIVE(tch));
            act(buf, true, ch, 0, tch, TO_NOTVICT);
            sprintf(buf, "$n tries to grab you, but $e can't get a grip on your %s%s&0 flesh.", COMPOSITION_COLOR(tch),
                    COMPOSITION_ADJECTIVE(tch));
            act(buf, true, ch, 0, tch, TO_VICT);
            return;
        } else if (!RIGID(ch) && RIGID(tch)) {
            sprintf(buf, "You can't get hold of $N with your %s%s&0 grip.", COMPOSITION_COLOR(ch),
                    COMPOSITION_ADJECTIVE(ch));
            act(buf, false, ch, 0, tch, TO_CHAR);
            sprintf(buf, "$n tries to grab $N, but $s %s%s&0 fingers can't get a grip.", COMPOSITION_COLOR(ch),
                    COMPOSITION_ADJECTIVE(ch));
            act(buf, true, ch, 0, tch, TO_NOTVICT);
            sprintf(buf, "$n tries to grab you, but $s %s%s&0 fingers can't get a grip.", COMPOSITION_COLOR(ch),
                    COMPOSITION_ADJECTIVE(ch));
            act(buf, true, ch, 0, tch, TO_VICT);
            return;
        }
    }

    /* Trying to drag a character? */
    if (found == FIND_CHAR_ROOM) {

        if (ch == tch) {
            char_printf(ch, "One foot in front of the other, now...\n");
            return;
        }

        if (GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(tch) < LVL_IMMORT) {
            if (IS_NPC(tch)) {
                char_printf(ch, "You can't drag NPC's!\n");
                return;
            }

            if (CONSENT(tch) != ch) {
                char_printf(ch, "Not without consent you don't!\n");
                return;
            }

            /*
             * The code below can handle the maximum dragging position set to
             * as high as POS_SITTING.  I wouldn't recommend setting it any
             * higher.
             */
            if (GET_POS(tch) > POS_SITTING) {
                act("$N isn't quite relaxed enough to be dragged.", false, ch, 0, tch, TO_CHAR);
                return;
            }

            if (GET_WEIGHT(tch) > 3 * CAN_CARRY_W(ch)) {
                act("$N is too heavy for you to drag.\n", false, ch, 0, tch, TO_CHAR);
                return;
            }
        } else if (GET_LEVEL(tch) >= GET_LEVEL(ch) && CONSENT(tch) != ch) {
            /* Immorts can drag anyone, except other immorts of same or higher level
             */
            char_printf(ch, "You can't drag someone a higher level than you.\n");
            return;
        }

        move_cost = std::min(4, GET_WEIGHT(tch) / 50 + sectors[SECT(ch->in_room)].mv);
    }

    /* Trying to drag an object in the room? */
    else if (found == FIND_OBJ_ROOM) {

        if (GET_LEVEL(ch) < LVL_GOD) {
            /*
             * Items can be made draggable by giving them the TAKE wear flag.
             * Here we check for that flag.  But an exception is made so that
             * you can also drag corpses.
             */
            if (!CAN_WEAR(tobj, ITEM_WEAR_TAKE) && !IS_CORPSE(tobj)) {
                char_printf(ch, "You cant drag that!\n");
                return;
            }

            if (GET_OBJ_WEIGHT(tobj) > 3 * CAN_CARRY_W(ch)) {
                send_to_char("It is too heavy for you to drag.\n", ch);
                return;
            }
        }

        if (IS_PLR_CORPSE(tobj) && !has_corpse_consent(ch, tobj)) {
            log(LogSeverity::Stat, LVL_IMMORT, "CORPSE: {} tried to drag {} without CONSENT!", GET_NAME(ch),
                tobj->short_description);
            return;
        }

        move_cost = std::min<int>(4, GET_OBJ_WEIGHT(tobj) / 50 + sectors[SECT(ch->in_room)].mv);
    }

    else {
        char_printf(ch, "You can't drag that!\n");
        return;
    }

    from_room = IN_ROOM(ch);

    /* Now determine the direction. */
    argument = any_one_arg(argument, arg);

    if ((dir = parse_direction(arg)) < 0) {
        /* Not a valid direction.  Try to drag into a portal. */
        if (!(portal = find_obj_in_list(world[from_room].contents, find_vis_by_name(ch, arg)))) {
            char_printf(ch, "Can't find a '{}' to drag into!\n", arg);
            return;
        }

        if (GET_OBJ_TYPE(portal) != ITEM_PORTAL) {
            char_printf(ch, "You can only drag things into portals!\n");
            return;
        }

        if (portal == tobj) {
            act("You tug and pull, but are unable to drag $p into itself.", false, ch, tobj, portal, TO_CHAR);
            return;
        }

        if (tobj) {
            act("&3$n&3 drags $p&3 into $P&3.&0", true, ch, tobj, portal, TO_ROOM);
            act("&3You drag $p&3 into $P&3.&0", false, ch, tobj, portal, TO_CHAR);
        } else if (tch) {
            act("&3$n&3 drags $N&3 into $p&3.&0", true, ch, portal, tch, TO_NOTVICT);
            act("&3$n&3 drags you into $p&3.&0", true, ch, portal, tch, TO_VICT);
            act("&3You drag $N&3 into $p&3.&0", false, ch, portal, tch, TO_CHAR);
        }
        do_enter(ch, arg, 0, 0);
        to_room = IN_ROOM(ch);
        if (tobj) {
            act("&3$n&3 drags $p&3 from $P.&0", true, ch, tobj, portal, TO_ROOM);
            obj_from_room(tobj);
            obj_to_room(tobj, to_room);
        } else if (tch) {
            act("&3$n&3 drags $N&3 from $p.&0", true, ch, portal, tch, TO_ROOM);
            char_from_room(tch);
            char_to_room(tch, to_room);
            char_drag_entry(ch, tch);
        }

        if (tobj && IS_PLR_CORPSE(tobj))
            log("CORPSE: {} dragged {} through {} from room {:d} to room {:d}", GET_NAME(ch), tobj->short_description,
                strip_ansi(portal->short_description), world[from_room].vnum, world[to_room].vnum);
    }

    else {
        /* Try to drag in a direction. */

        if (GET_LEVEL(ch) < LVL_GOD) {
            if (GET_MOVE(ch) < move_cost + 6) {
                char_printf(ch, "You are too exhausted!\n");
                return;
            }
        }

        if (CONFUSED(ch)) {
            char_printf(ch, "&5You are confused!&0\n");
            dir = random_number(0, 5);
        }

        /* Take tch from the room so they don't see ch's leave message */
        if (tch)
            char_from_room(tch);
        if (!perform_move(ch, dir, false, false)) {
            if (tch) {
                char_to_room(tch, from_room);
                sprintf(buf, "&3Looking confused, $n&0&3 tried to drag $N&0&3 %s.&0", dirs[dir]);
                act(buf, false, ch, 0, tch, TO_NOTVICT);
                sprintf(buf, "&3Looking confused, $n&0&3 tried to drag you %s.&0", dirs[dir]);
                act(buf, false, ch, 0, tch, TO_VICT);
            } else if (tobj) {
                sprintf(buf, "&3Looking confused, $n&0&3 tried to drag $p&0&3 %s.&0", dirs[dir]);
                act(buf, false, ch, tobj, 0, TO_ROOM);
            }
            return;
        }

        alter_move(ch, move_cost);

        to_room = IN_ROOM(ch);

        /*
         * perform_move was successful, so move ch back to from_room so
         * we can display act() messages
         */
        char_from_room(ch);
        char_to_room(ch, from_room);
        if (tobj) {
            act("&3You drag $p&3 behind you.&0", false, ch, tobj, 0, TO_CHAR);
            act("&3$n&3 drags $p&3 behind $m.&0", true, ch, tobj, 0, TO_ROOM);
        } else if (tch) {
            act("&3You drag $N&3 behind you.&0", false, ch, 0, tch, TO_CHAR);
            act("&3$n&3 drags you $t behind $m.&0", true, ch, dirs[dir], tch, TO_VICT);
            act("&3$n&3 drags $N&3 behind $m.&0", true, ch, 0, tch, TO_NOTVICT);
        }

        /* now display act() messages to the target room */
        char_from_room(ch);
        char_to_room(ch, to_room);
        if (tobj) {
            obj_from_room(tobj);
            obj_to_room(tobj, to_room);
            act("&3$n&3 drags $p&3 behind $m.&0", true, ch, tobj, 0, TO_ROOM);
        } else if (tch) {
            /* char_from_room already called above. */
            char_to_room(tch, to_room);
            char_drag_entry(ch, tch);
        }

        if (tobj && IS_PLR_CORPSE(tobj))
            log("CORPSE: {} drags {} from room {:d} to room {:d}.", GET_NAME(ch), tobj->short_description,
                world[from_room].vnum, world[to_room].vnum);
    }
}

ACMD(do_fly) {
    switch (GET_STANCE(ch)) {
    case STANCE_DEAD:
        act("Rest in peace.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_MORT:
    case STANCE_INCAP:
    case STANCE_STUNNED:
        act("Alas, you cannot summon the will.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_SLEEPING:
        act("You have to wake up first!", false, ch, 0, 0, TO_CHAR);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS)) {
        char_printf(ch, "You can't even twitch, much less fly!\n");
        return;
    }

    if (!EFF_FLAGGED(ch, EFF_FLY) && GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You do not have the means to fly.\n");
        return;
    }

    if (GET_POS(ch) != POS_FLYING && too_heavy_to_fly(ch)) {
        char_printf(ch, "You try to rise up, but you can't get off the ground!\n");
        act("$n rises up on $s toes, as if trying to fly.", true, ch, 0, 0, TO_ROOM);
        return;
    }

    switch (GET_POS(ch)) {
    case POS_STANDING:
        act("&6&bYou begin to float.&0", false, ch, 0, 0, TO_CHAR);
        act("&6&b$n&6&b begins to float.&0", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_FLYING:
        char_printf(ch, "You are already flying.\n");
        return;
    case POS_PRONE:
    case POS_SITTING:
    case POS_KNEELING:
    default:
        act("&6&bYou get to your feet and begin floating.&0", false, ch, 0, 0, TO_CHAR);
        act("&6&b$n&6&b gets to $s feet and begins to float.&0", true, ch, 0, 0, TO_ROOM);
        break;
    }

    GET_POS(ch) = POS_FLYING;
    GET_STANCE(ch) = FIGHTING(ch) ? STANCE_FIGHTING : STANCE_ALERT;
    mount_pos_check(ch);
}

ACMD(do_stand) {
    if (IS_NPC(ch)) {
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
    };

    switch (GET_STANCE(ch)) {
    case STANCE_DEAD:
        act("Rest in peace.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_MORT:
    case STANCE_INCAP:
    case STANCE_STUNNED:
        act("Alas, you cannot summon the will.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_SLEEPING:
        act("You have to wake up first!", false, ch, 0, 0, TO_CHAR);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS)) {
        char_printf(ch, "You can't even twitch, much less stand up!\n");
        return;
    }

    switch (GET_POS(ch)) {
    case POS_PRONE:
        act("You sit up and rise to your feet.", false, ch, 0, 0, TO_CHAR);
        act("$n sits up, and rises to $s feet.", false, ch, 0, 0, TO_ROOM);
        break;
    case POS_STANDING:
        act("You are already standing.", false, ch, 0, 0, TO_CHAR);
        return;
        break;
    case POS_FLYING:
        act("You slowly descend to the surface beneath you.", false, ch, 0, 0, TO_CHAR);
        act("$n gently descends to the surface beneath $s feet.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_SITTING:
    case POS_KNEELING:
    default:
        act("You stand up.", false, ch, 0, 0, TO_CHAR);
        act("$n clambers to $s feet.", true, ch, 0, 0, TO_ROOM);
        break;
    }

    GET_POS(ch) = POS_STANDING;
    GET_STANCE(ch) = FIGHTING(ch) ? STANCE_FIGHTING : STANCE_ALERT;
    falling_check(ch);
    mount_pos_check(ch);
}

ACMD(do_sit) {
    switch (GET_STANCE(ch)) {
    case STANCE_DEAD:
        act("Rest in peace.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_MORT:
    case STANCE_INCAP:
    case STANCE_STUNNED:
        act("Alas, you cannot summon the will.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_SLEEPING:
        act("You have to wake up first!", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_FIGHTING:
        act("Sit down while fighting?  Are you MAD?", false, ch, 0, 0, TO_CHAR);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS)) {
        char_printf(ch, "You can't even twitch, much less sit down!\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_BERSERK)) {
        char_printf(ch, "You're too incensed to sit down!\n");
        return;
    }

    switch (GET_POS(ch)) {
    case POS_PRONE:
        act("You sit up.", false, ch, 0, 0, TO_CHAR);
        act("$n sits up.", false, ch, 0, 0, TO_ROOM);
        break;
    case POS_SITTING:
        char_printf(ch, "You're sitting already.\n");
        return;
    case POS_KNEELING:
        act("You stop kneeling and sit down.", false, ch, 0, 0, TO_CHAR);
        act("$n stops kneeling, and sits down.", false, ch, 0, 0, TO_ROOM);
        break;
    case POS_FLYING:
        act("You stop flying and sit down.", false, ch, 0, 0, TO_CHAR);
        act("$n stops flying and sits down.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_STANDING:
    default:
        act("You sit down.", false, ch, 0, 0, TO_CHAR);
        act("$n sits down.", false, ch, 0, 0, TO_ROOM);
        break;
    }

    GET_POS(ch) = POS_SITTING;
    GET_STANCE(ch) = STANCE_ALERT;
    falling_check(ch);
    mount_pos_check(ch);
}

ACMD(do_kneel) {
    switch (GET_STANCE(ch)) {
    case STANCE_DEAD:
        act("Rest in peace.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_MORT:
    case STANCE_INCAP:
    case STANCE_STUNNED:
        act("Alas, you cannot summon the will.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_SLEEPING:
        char_printf(ch, "You seem to be having a dream of infancy.\n");
        return;
    case STANCE_FIGHTING:
        act("Kneel while fighting?  Are you MAD?", false, ch, 0, 0, TO_CHAR);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_BERSERK)) {
        char_printf(ch, "You're too incensed to kneel down!\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS)) {
        char_printf(ch, "You can't even twitch, much less get to your knees!\n");
        return;
    }

    switch (GET_POS(ch)) {
    case POS_PRONE:
        act("You swing up onto your knees.", false, ch, 0, 0, TO_CHAR);
        act("$n levers up onto $s knees.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_SITTING:
        act("You move from your butt to your knees.", false, ch, 0, 0, TO_CHAR);
        act("$n stops sitting and gets to $s knees.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_KNEELING:
        char_printf(ch, "You are kneeling already.\n");
        return;
    case POS_FLYING:
        act("You stop flying and kneel.", false, ch, 0, 0, TO_CHAR);
        act("$n stops flying and settles to $s knees.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_STANDING:
    default:
        act("You kneel.", false, ch, 0, 0, TO_CHAR);
        act("$n settles to $s knees.", true, ch, 0, 0, TO_ROOM);
    }
    GET_POS(ch) = POS_KNEELING;
    falling_check(ch);
    mount_pos_check(ch);
}

ACMD(do_recline) {
    switch (GET_STANCE(ch)) {
    case STANCE_DEAD:
        act("Rest in peace.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_MORT:
    case STANCE_INCAP:
    case STANCE_STUNNED:
        act("Alas, you cannot summon the will.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_SLEEPING:
        char_printf(ch, "You dream of lying down.\n");
        return;
    case STANCE_FIGHTING:
        act("Lie down while fighting?  Are you MAD?", false, ch, 0, 0, TO_CHAR);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS)) {
        char_printf(ch, "You can't even twitch, much less lie down!\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_BERSERK)) {
        char_printf(ch, "You're too incensed to lie down!\n");
        return;
    }

    switch (GET_POS(ch)) {
    case POS_PRONE:
        char_printf(ch, "You are already lying down.\n");
        return;
    case POS_SITTING:
        act("You stop sitting and lie down.", false, ch, 0, 0, TO_CHAR);
        act("$n stops sitting and lies down.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_KNEELING:
        act("You recline.", false, ch, 0, 0, TO_CHAR);
        act("$n eases off $s knees and lies down.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_FLYING:
        act("You gently land on your belly.", false, ch, 0, 0, TO_CHAR);
        act("$n stops flying and drops flat to the ground.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_STANDING:
    default:
        act("You drop to your belly.", false, ch, 0, 0, TO_CHAR);
        act("$n drops flat to the ground.", true, ch, 0, 0, TO_ROOM);
        break;
    }

    GET_POS(ch) = POS_PRONE;
    falling_check(ch);
    mount_pos_check(ch);
}

ACMD(do_rest) {
    switch (GET_STANCE(ch)) {
    case STANCE_DEAD:
        act("You are already resting, after a fashion.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_MORT:
    case STANCE_INCAP:
    case STANCE_STUNNED:
        act("That is the least of your concerns.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_SLEEPING:
        char_printf(ch, "You seem to be quite restful already.\n");
        return;
    case STANCE_RESTING:
        char_printf(ch, "You are already resting.\n");
        return;
    case STANCE_FIGHTING:
        act("Rest while fighting?  Are you MAD?", false, ch, 0, 0, TO_CHAR);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS)) {
        char_printf(ch, "In your paralyzed state, you find this impossible.\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_BERSERK)) {
        char_printf(ch, "You're too incensed to rest!\n");
        return;
    }

    switch (GET_POS(ch)) {
    case POS_PRONE:
        char_printf(ch, "You relax.\n");
        act("You see some of the tension leave $n's body.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_SITTING:
        act("You find a comfortable spot where you are sitting.", false, ch, 0, 0, TO_CHAR);
        act("$n finds a comfortable spot where $e is sitting.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_KNEELING:
        char_printf(ch, "You slump and relax your posture.\n");
        act("$n relaxes a bit.", true, ch, 0, 0, TO_ROOM);
        GET_POS(ch) = POS_SITTING;
        break;
    case POS_FLYING:
        char_printf(ch, "You stop floating and rest your tired bones.\n");
        act("$n stops floating and takes a rest.", true, ch, 0, 0, TO_ROOM);
        GET_POS(ch) = POS_SITTING;
        break;
    case POS_STANDING:
    default:
        char_printf(ch, "You sit down and relax.\n");
        act("$n sits down in a comfortable spot.", true, ch, 0, 0, TO_ROOM);
        GET_POS(ch) = POS_SITTING;
        break;
    }

    GET_STANCE(ch) = STANCE_RESTING;
    falling_check(ch);
    mount_pos_check(ch);
}

/* new, does opposite of 'rest' */

ACMD(do_alert) {
    switch (GET_STANCE(ch)) {
    case STANCE_DEAD:
        act("Totally impossible.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_MORT:
    case STANCE_INCAP:
    case STANCE_STUNNED:
        act("That is utterly beyond your current abilities.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_SLEEPING:
        char_printf(ch, "Let's try this in stages.  How about waking up first?\n");
        return;
    case STANCE_ALERT:
        char_printf(ch, "You are already about as tense as you can get.\n");
        return;
    case STANCE_FIGHTING:
        char_printf(ch, "Being as you're in a battle and all, you're pretty alert already!\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS)) {
        char_printf(ch, "In your paralyzed state, you find this impossible.\n");
        return;
    }

    switch (GET_POS(ch)) {
    case POS_PRONE:
        char_printf(ch, "You stop relaxing and try to become more aware of your surroundings.\n");
        break;
    case POS_SITTING:
        char_printf(ch, "You sit up straight and start to pay attention.\n");
        act("$n sits at attention.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_KNEELING:
        char_printf(ch, "You straighten up a bit.\n");
        act("$n straightens up a bit.", true, ch, 0, 0, TO_ROOM);
        break;
    default:
        char_printf(ch, "You tense up and become more alert.\n");
        break;
    }

    GET_STANCE(ch) = STANCE_ALERT;
    mount_pos_check(ch);
}

ACMD(do_sleep) {
    switch (GET_STANCE(ch)) {
    case STANCE_DEAD:
        act("Rest in peace.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_MORT:
    case STANCE_INCAP:
    case STANCE_STUNNED:
        act("You are beyond sleep.", false, ch, 0, 0, TO_CHAR);
        return;
    case STANCE_SLEEPING:
        char_printf(ch, "You are already sound asleep.\n");
        return;
    case STANCE_FIGHTING:
        char_printf(ch, "Sleep while fighting?  Are you MAD?\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS)) {
        char_printf(ch, "In your paralyzed state, you find this impossible.\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_BERSERK)) {
        char_printf(ch, "You're too incensed to take a nap!\n");
        return;
    }

    switch (GET_POS(ch)) {
    case POS_PRONE:
        char_printf(ch, "You go to sleep.\n");
        act("$n goes to sleep.", true, ch, 0, 0, TO_ROOM);
        break;
    case POS_FLYING:
        act("You stop floating, and lie down to sleep.", false, ch, 0, 0, TO_CHAR);
        act("$n stops floating, and lies down to sleep.", true, ch, 0, 0, TO_ROOM);
        break;
    default:
        char_printf(ch, "You lie down and go to sleep.\n");
        act("$n lies down and goes to sleep.", true, ch, 0, 0, TO_ROOM);
    }

    GET_POS(ch) = POS_PRONE;
    GET_STANCE(ch) = STANCE_SLEEPING;
    falling_check(ch);
    mount_pos_check(ch);
}

ACMD(do_wake) {
    CharData *vict;
    int self = 0;

    one_argument(argument, arg);
    if (*arg) {
        if (GET_STANCE(ch) < STANCE_RESTING)
            char_printf(ch, "Maybe you should wake yourself up first.\n");
        else if ((vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))) == nullptr)
            char_printf(ch, NOPERSON);
        else if (vict == ch)
            self = 1;
        else if (GET_STANCE(vict) > STANCE_SLEEPING)
            act("$E is already awake.", false, ch, 0, vict, TO_CHAR);
        else if (EFF_FLAGGED(vict, EFF_SLEEP))
            act("You can't wake $M up!", false, ch, 0, vict, TO_CHAR);
        else if (GET_STANCE(vict) < STANCE_SLEEPING)
            act("$E's in pretty bad shape!", false, ch, 0, vict, TO_CHAR);
        else {
            act("You wake $M up.", false, ch, 0, vict, TO_CHAR);
            act("You are awakened by $n.", false, ch, 0, vict, TO_VICT | TO_SLEEP);
            GET_POS(vict) = POS_SITTING;
            GET_STANCE(vict) = STANCE_RESTING;
        }
        if (!self)
            return;
    }
    if (EFF_FLAGGED(ch, EFF_SLEEP))
        char_printf(ch, "You can't wake up!\n");
    else if (GET_STANCE(ch) > STANCE_SLEEPING)
        char_printf(ch, "You are already awake...\n");
    else {
        char_printf(ch, "You awaken.\n");
        act("$n awakens.", true, ch, 0, 0, TO_ROOM);
        GET_POS(ch) = POS_SITTING;
        GET_STANCE(ch) = STANCE_RESTING;
    }
    mount_pos_check(ch);
}

ACMD(do_follow) {
    CharData *leader;

    if (subcmd == SCMD_SHADOW && !GET_SKILL(ch, SKILL_SHADOW)) {
        char_printf(ch, "You aren't skilled enough to shadow someone.\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        if (ch->master)
            act("You are following $N.", false, ch, 0, ch->master, TO_CHAR);
        else
            char_printf(ch, "You are not following anyone.\n");
        return;
    }

    if (!strcasecmp(arg, "off"))
        leader = ch;
    else if (!(leader = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        char_printf(ch, NOPERSON);
        return;
    }

    /* Charmies aren't allowed to stop following. */
    if (EFF_FLAGGED(ch, EFF_CHARM) && ch->master)
        act("But you only feel like following $N!", false, ch, 0, ch->master, TO_CHAR);
    /* If 'fol self' then stop following. */
    else if (ch == leader) {
        if (ch->master)
            stop_follower(ch, 0);
        else
            char_printf(ch, "You aren't following anyone.\n");
    } else if (ch->master == leader)
        act("You are already following $N.", false, ch, 0, leader, TO_CHAR);
    else if (GET_LEVEL(ch) < LVL_GOD && PRF_FLAGGED(leader, PRF_NOFOLLOW))
        char_printf(ch, "That person would rather not have followers right now.\n");
    else {
        if (ch->master)
            stop_follower(ch, 0);

        if (subcmd == SCMD_SHADOW)
            SET_FLAG(EFF_FLAGS(ch), EFF_SHADOWING);

        add_follower(ch, leader);

        if (subcmd == SCMD_SHADOW) {
            int chance = GET_SKILL(ch, SKILL_SHADOW);
            chance += dex_app_skill[GET_DEX(ch)].sneak;

            if (chance < random_number(1, 101)) {
                REMOVE_FLAG(EFF_FLAGS(ch), EFF_SHADOWING);
                act("You are noticed as you attempt to secretly follow $N.", false, ch, 0, leader, TO_CHAR);
                act("$n attempts to secretly follow you, but you spot $m.", true, ch, 0, leader, TO_VICT);
                act("$n attempts to follow $N secretly, but you notice $m.", true, ch, 0, leader, TO_NOTVICT);
            } else
                act("You start shadowing $N.", false, ch, 0, leader, TO_CHAR);

            WAIT_STATE(ch, PULSE_VIOLENCE / 2);
            improve_skill(ch, SKILL_SHADOW);
        }
    }
}

ACMD(do_abandon) {
    bool found = false;
    CharData *follower;
    FollowType *k;
    char buf[MAX_STRING_LENGTH];

    one_argument(argument, arg);

    buf[0] = 0;

    if (pk_allowed) {

        if (!*arg) {
            if (ch->followers) {
                sprintf(buf, "You are being followed by:\n");
                for (k = ch->followers; k; k = k->next) {
                    if (GET_INVIS_LEV(k->follower) < GET_LEVEL(ch)) {
                        if (CAN_SEE_MOVING(ch, k->follower)) {
                            sprintf(buf, "%s  %s\n", buf, GET_NAME(k->follower));
                            found = true;
                        }
                    }
                }
            }
            if (found) {
                page_string(ch, buf);
            } else {
                char_printf(ch, "Nobody is following you.\n");
            }
            return;
        }

        if (!strcasecmp(arg, "all")) {
            if (ch->followers) {
                for (k = ch->followers; k; k = k->next) {
                    if (GET_LEVEL(k->follower) < LVL_GOD) {
                        if (CAN_SEE_MOVING(ch, k->follower)) {
                            stop_follower(k->follower, 0);
                            found = true;
                        }
                    } else {
                        if (GET_INVIS_LEV(k->follower) < GET_LEVEL(ch)) {
                            char_printf(ch, "You can not abandon immortals.\n");
                            found = true;
                        }
                    }
                }
            }
            if (!found) {
                char_printf(ch, "Nobody is following you.\n");
            }
            return;
        }

        if (!strcasecmp(arg, "pets")) {
            if (ch->followers) {
                for (k = ch->followers; k; k = k->next) {
                    if (EFF_FLAGGED(k->follower, EFF_CHARM)) {
                        stop_follower(k->follower, 0);
                        found = true;
                    }
                }
            }
            if (!found) {
                char_printf(ch, "You do not have any pets following you.\n");
            }
            return;
        }

        if (!strcasecmp(arg, "players")) {
            if (ch->followers) {
                for (k = ch->followers; k; k = k->next) {
                    if (!(EFF_FLAGGED(k->follower, EFF_CHARM))) {
                        if (GET_LEVEL(k->follower) < LVL_GOD) {
                            if (CAN_SEE_MOVING(ch, k->follower)) {
                                stop_follower(k->follower, 0);
                                found = true;
                            }
                        } else {
                            if (GET_INVIS_LEV(k->follower) < GET_LEVEL(ch)) {
                                char_printf(ch, "You can not abandon immortals.\n");
                                found = true;
                            }
                        }
                    }
                }
            }
            if (!found) {
                char_printf(ch, "You do not have any players following you.\n");
            }
            return;
        }

        if (!(follower = find_char_for_mtrig(ch, arg))) {
            char_printf(ch, NOPERSON);
            return;
        } else if (ch == follower) {
            char_printf(ch, "You can't abandon yourself.\n");
            return;
        } else if (ch->followers) {
            for (k = ch->followers; k; k = k->next) {
                if ((k->follower == follower) && (CAN_SEE_MOVING(ch, k->follower))) {
                    if (GET_LEVEL(k->follower) < LVL_GOD) {
                        stop_follower(k->follower, 0);
                        found = true;
                    } else {
                        char_printf(ch, "You can not abandon immortals.\n");
                        found = true;
                    }
                }
            }
        } else if (!found) {
            char_printf(ch, NOPERSON);
        }
        return;
    } else {
        if (!*arg) {
            if (ch->followers) {
                sprintf(buf, "You are being followed by:\n");
                for (k = ch->followers; k; k = k->next) {
                    if (GET_INVIS_LEV(k->follower) < GET_LEVEL(ch)) {
                        if (EFF_FLAGGED(k->follower, EFF_INVISIBLE)) {
                            sprintf(buf, "%s  %s (invis)\n", buf, GET_NAME(k->follower));
                            found = true;
                        } else {
                            sprintf(buf, "%s  %s\n", buf, GET_NAME(k->follower));
                            found = true;
                        }
                    }
                }
            }
            if (found) {
                page_string(ch, buf);
            } else {
                char_printf(ch, "Nobody is following you.\n");
            }
            return;
        }

        if (!strcasecmp(arg, "all")) {
            if (ch->followers) {
                for (k = ch->followers; k; k = k->next) {
                    if (GET_LEVEL(k->follower) < LVL_GOD) {
                        if ((EFF_FLAGGED(k->follower, EFF_INVISIBLE)) && (!CAN_SEE(ch, k->follower))) {
                            char_printf(ch, "{} stops following you.\n", GET_NAME(k->follower));
                        }
                        stop_follower(k->follower, 0);
                        found = true;
                    } else {
                        if (GET_INVIS_LEV(k->follower) < GET_LEVEL(ch)) {
                            char_printf(ch, "You can not abandon immortals.\n");
                            found = true;
                        }
                    }
                }
            }
            if (!found) {
                char_printf(ch, "Nobody is following you.\n");
            }
            return;
        }

        if (!strcasecmp(arg, "pets")) {
            if (ch->followers) {
                for (k = ch->followers; k; k = k->next) {
                    if (EFF_FLAGGED(k->follower, EFF_CHARM)) {
                        stop_follower(k->follower, 0);
                        found = true;
                    }
                }
            }
            if (!found) {
                char_printf(ch, "You do not have any pets following you.\n");
            }
            return;
        }

        if (!strcasecmp(arg, "players")) {
            if (ch->followers) {
                for (k = ch->followers; k; k = k->next) {
                    if (!(EFF_FLAGGED(k->follower, EFF_CHARM))) {
                        if (GET_LEVEL(k->follower) < LVL_GOD) {
                            if ((EFF_FLAGGED(k->follower, EFF_INVISIBLE)) && (!CAN_SEE(ch, k->follower))) {
                                char_printf(ch, "{} stops following you.\n", GET_NAME(k->follower));
                            }
                            stop_follower(k->follower, 0);
                            found = true;
                        } else {
                            if (GET_INVIS_LEV(k->follower) < GET_LEVEL(ch)) {
                                char_printf(ch, "You can not abandon immortals.\n");
                                found = true;
                            }
                        }
                    }
                }
            }
            if (!found) {
                char_printf(ch, "You do not have any players following you.\n");
            }
            return;
        }

        if (!(follower = find_char_for_mtrig(ch, arg))) {
            char_printf(ch, NOPERSON);
            return;
        } else if (ch == follower) {
            char_printf(ch, "You can't abandon yourself.\n");
            return;
        } else if (ch->followers) {
            for (k = ch->followers; k; k = k->next) {
                if (k->follower == follower) {
                    if (GET_LEVEL(k->follower) < LVL_GOD) {
                        if ((EFF_FLAGGED(k->follower, EFF_INVISIBLE)) && (!CAN_SEE(ch, k->follower))) {
                            char_printf(ch, "{} stops following you.\n", GET_NAME(k->follower));
                        }
                        stop_follower(k->follower, 0);
                        found = true;
                    } else {
                        char_printf(ch, "You can not abandon immortals.\n");
                        found = true;
                    }
                }
            }
        }
        if (!found) {
            char_printf(ch, NOPERSON);
        }
        return;
    }
}

void perform_buck(CharData *mount, int whilemounting) {
    CharData *rider = RIDDEN_BY(mount);

    if (!rider)
        return;

    if (IS_SPLASHY(IN_ROOM(mount))) {
        if (whilemounting) {
            act("$N suddenly bucks upwards, throwing you violently into the water!", false, rider, 0, mount, TO_CHAR);
            act("$n is thrown down with a splash as $N violently bucks!", true, rider, 0, mount, TO_NOTVICT);
            act("You buck violently and throw $n into the water.", false, rider, 0, mount, TO_VICT);
        } else {
            act("You quickly buck, throwing $N into the water.", false, mount, 0, rider, TO_CHAR);
            act("$n quickly bucks, throwing you into the water.", false, mount, 0, rider, TO_VICT);
            act("$n quickly bucks, throwing $N into the water.", false, mount, 0, rider, TO_NOTVICT);
        }
    } else {
        if (whilemounting) {
            act("$N suddenly bucks upwards, throwing you violently to the ground!", false, rider, 0, mount, TO_CHAR);
            act("$n is thrown to the ground as $N violently bucks!", true, rider, 0, mount, TO_NOTVICT);
            act("You buck violently and throw $n to the ground.", false, rider, 0, mount, TO_VICT);
        } else {
            act("You quickly buck, throwing $N to the ground.", false, mount, 0, rider, TO_CHAR);
            act("$n quickly bucks, throwing you to the ground.", false, mount, 0, rider, TO_VICT);
            act("$n quickly bucks, throwing $N to the ground.", false, mount, 0, rider, TO_NOTVICT);
        }
    }

    dismount_char(rider);
    if (!whilemounting)
        alter_pos(rider, POS_SITTING, STANCE_ALERT);
    hurt_char(rider, nullptr, roll_dice(1, 3), true);
}

ACMD(do_mount) {
    char arg[MAX_INPUT_LENGTH];
    CharData *vict;

    if (FIGHTING(ch)) {
        char_printf(ch, "You are too busy fighting to try that right now.\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        char_printf(ch, "Mount who?\n");
        return;
    } else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        char_printf(ch, "There is no-one by that name here.\n");
        return;
    } else if (!IS_NPC(vict)) {
        char_printf(ch, "Ehh... no.\n");
        return;
    } else if (RIDING(ch) || RIDDEN_BY(ch)) {
        char_printf(ch, "You are already mounted.\n");
        return;
    } else if (RIDING(vict) || RIDDEN_BY(vict)) {
        char_printf(ch, "It is already mounted.\n");
        return;
    } else if (GET_LEVEL(ch) < LVL_IMMORT && !MOB_FLAGGED(vict, MOB_MOUNTABLE)) {
        char_printf(ch, "You can't mount that!\n");
        return;
    } else if (!GET_SKILL(ch, SKILL_MOUNT)) {
        char_printf(ch, "First you need to learn *how* to mount.\n");
        return;
    } else if (ch == vict) {
        char_printf(ch, "Not likely.\n");
        return;
    } else if (RIGID(ch) && !RIGID(vict) && GET_LEVEL(ch) < LVL_IMMORT) {
        if (GET_COMPOSITION(vict) == COMP_ETHER) {
            act("You go to mount $N, but you pass right through $M.", false, ch, 0, vict, TO_CHAR);
            act("$n tries to mount you", false, ch, 0, vict, TO_VICT);
            act("$n tries to mount $N, but only flails about in a vain attempt to "
                "touch $M.",
                true, ch, 0, vict, TO_NOTVICT);
        } else {
            act("You lay a hand on $N, but it slips right through.", false, ch, 0, vict, TO_CHAR);
            act("$n tries to mount you, but you are too fluid to support $m", false, ch, 0, vict, TO_VICT);
            act("$n tries to mount $N, but $E isn't solid enough to support a rider.", true, ch, 0, vict, TO_NOTVICT);
        }
        return;
    } else if (!RIGID(ch) && RIGID(vict) && GET_LEVEL(ch) < LVL_IMMORT) {
        if (GET_COMPOSITION(ch) == COMP_ETHER) {
            act("You try to climb $N, but you simply pass through $M.", false, ch, 0, vict, TO_CHAR);
            act("$n tries to mount you, but $e can't seem to touch you.", false, ch, 0, vict, TO_VICT);
            act("$n tries to mount $N, but makes $M shudder as $e passes through.", false, ch, 0, vict, TO_NOTVICT);
        } else {
            act("You try to climb $M, but you just flow right off.", false, ch, 0, vict, TO_CHAR);
            act("$n tries to mount you, but just flows right off.", false, ch, 0, vict, TO_VICT);
            act("$n tries to mount $N, but just flows right off.", false, ch, 0, vict, TO_NOTVICT);
        }
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE / 2);

    if (mount_fall(ch, vict)) {
        act("You try to mount $N, but slip and fall off.", false, ch, 0, vict, TO_CHAR);
        act("$n tries to mount you, but slips and falls off.", false, ch, 0, vict, TO_VICT);
        act("$n tries to mount $N, but slips and falls off.", true, ch, 0, vict, TO_NOTVICT);
        mount_warning(ch, vict);
        alter_pos(ch, POS_SITTING, STANCE_ALERT);
        improve_skill(ch, SKILL_MOUNT);
        hurt_char(ch, nullptr, roll_dice(1, 2), true);
        return;
    }

    act("You mount $N.", false, ch, 0, vict, TO_CHAR);
    act("$n mounts you.", false, ch, 0, vict, TO_VICT);
    act("$n mounts $N.", true, ch, 0, vict, TO_NOTVICT);
    mount_char(ch, vict);
    improve_skill(ch, SKILL_MOUNT);

    if (IS_NPC(vict) && !EFF_FLAGGED(vict, EFF_TAMED) && mount_bucked(ch, vict)) {
        perform_buck(vict, true);
        mount_warning(ch, vict);
    }
}

ACMD(do_dismount) {
    if (!RIDING(ch))
        char_printf(ch, "You aren't even riding anything.\n");
    else if (FIGHTING(ch))
        char_printf(ch, "You would get hacked to pieces if you dismount now!\n");
    else if (SECT(ch->in_room) == SECT_WATER && !can_travel_on_water(ch) && GET_POS(ch) != POS_FLYING)
        char_printf(ch, "Yah, right, and then drown...\n");
    else if (mount_fall(ch, RIDING(ch)) && random_number(0, 1) == 0) {
        act("As you begin dismounting, you slip and fall down.", false, ch, 0, RIDING(ch), TO_CHAR);
        act("$n starts to dismount, but slips and falls down.", false, ch, 0, RIDING(ch), TO_ROOM);
        alter_pos(ch, POS_SITTING, STANCE_ALERT);
        improve_skill(ch, SKILL_MOUNT);
        hurt_char(ch, nullptr, roll_dice(1, 2), true);
    } else {
        act("You dismount $N.", false, ch, 0, RIDING(ch), TO_CHAR);
        act("$n dismounts from you.", false, ch, 0, RIDING(ch), TO_VICT);
        act("$n dismounts $N.", true, ch, 0, RIDING(ch), TO_NOTVICT);
        dismount_char(ch);
    }
}

ACMD(do_buck) {
    if (!RIDDEN_BY(ch))
        char_printf(ch, "You're not even being ridden!\n");
    else if (FIGHTING(ch))
        char_printf(ch, "Worry about this &bafter&0 the battle!\n");
    else if (EFF_FLAGGED(ch, EFF_TAMED))
        char_printf(ch, "But you're tamed!\n");
    else
        perform_buck(ch, false);
}

ACMD(do_tame) {
    char arg[MAX_INPUT_LENGTH];
    effect eff;
    CharData *vict;
    int tame_duration = 0;
    int chance_tame, chance_attack, lvldiff;

    if (FIGHTING(ch)) {
        char_printf(ch, "You can't tame while fighting!\n");
        return;
    }
    one_argument(argument, arg);

    if (!*arg) {
        char_printf(ch, "Tame who?\n");
        return;
    } else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        char_printf(ch, "There is no such creature here.\n");
        return;
    } else if (!IS_NPC(vict) || (GET_LEVEL(ch) < LVL_IMMORT && !MOB_FLAGGED(vict, MOB_MOUNTABLE))) {
        act("You can't tame $N.", false, ch, 0, vict, TO_CHAR);
        return;
    } else if (EFF_FLAGGED(vict, EFF_TAMED)) {
        act("$E seems quite tame already.", false, ch, 0, vict, TO_CHAR);
        return;
    } else if (!GET_SKILL(ch, SKILL_TAME)) {
        char_printf(ch, "You don't even know how to tame something.\n");
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);

    lvldiff = mountlevel(vict) - ideal_tamelevel(ch);
    if (lvldiff < 1) {
        chance_tame = 1100;
        chance_attack = 0;
    } else {
        chance_tame = 5 + pow(2 * (MOUNT_LEVEL_FUDGE - lvldiff + 1) / MOUNT_LEVEL_FUDGE, 3) * 900 / 8;
        chance_attack = 5 + pow(2 * lvldiff / MOUNT_LEVEL_FUDGE, 3) * 600 / 8;
    }

    if (random_number(0, 999) >= chance_tame) {
        if (random_number(0, 999) < chance_attack) {
            act("Your shocking lack of tact has greatly annoyed $N!", false, ch, 0, vict, TO_CHAR);
            act("$n has really pissed $N off!", false, ch, 0, vict, TO_ROOM);
            if (IS_NPC(vict) && AWAKE(vict))
                attack(vict, ch);
        } else {
            act("You failed to tame $M.", false, ch, 0, vict, TO_CHAR);
            act("$n tries lamely to tame $N.", false, ch, 0, vict, TO_ROOM);
            improve_skill(ch, SKILL_TAME);
        }
        return;
    }

    improve_skill(ch, SKILL_TAME);

    tame_duration = (int)(GET_CHA(ch) / 3) + 10;
    if (GET_CLASS(ch) == CLASS_DRUID) {
        tame_duration += 30;
    }
    if (GET_CLASS(ch) == CLASS_RANGER) {
        tame_duration += 15;
    }

    memset(&eff, 0, sizeof(effect));
    eff.type = SKILL_TAME;
    eff.duration = tame_duration;
    eff.modifier = 0;
    eff.location = APPLY_NONE;
    SET_FLAG(eff.flags, EFF_TAMED);

    effect_to_char(vict, &eff);
    SET_FLAG(MOB_FLAGS(vict), MOB_PET);

    act("You tame $N.", false, ch, 0, vict, TO_CHAR);
    act("$n tames you.", false, ch, 0, vict, TO_VICT);
    act("$n tames $N.", false, ch, 0, vict, TO_NOTVICT);
}
