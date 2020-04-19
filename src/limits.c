/***************************************************************************
 * $Id: limits.c,v 1.119 2009/07/17 00:48:17 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: limits.c                                       Part of FieryMUD *
 *  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998 - 2003 by the Fiery Consortium             *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "limits.h"

#include "casting.h"
#include "clan.h"
#include "class.h"
#include "comm.h"
#include "conf.h"
#include "db.h"
#include "dg_scripts.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "math.h"
#include "pfiles.h"
#include "players.h"
#include "races.h"
#include "regen.h"
#include "screen.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

extern long max_exp_gain(struct char_data *ch);
extern long exp_death_loss(struct char_data *ch, int level);

ACMD(do_shapechange);

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6) {

    if (age < 15)
        return (p0); /* < 15   */
    else if (age <= 29)
        return (int)(p1 + (((age - 15) * (p2 - p1)) / 15)); /* 15..29 */
    else if (age <= 44)
        return (int)(p2 + (((age - 30) * (p3 - p2)) / 15)); /* 30..44 */
    else if (age <= 59)
        return (int)(p3 + (((age - 45) * (p4 - p3)) / 15)); /* 45..59 */
    else if (age <= 79)
        return (int)(p4 + (((age - 60) * (p5 - p4)) / 20)); /* 60..79 */
    else
        return (p6); /* >= 80 */
}

/* manapoint gain pr. game hour */
int mana_gain(struct char_data *ch) {
    int gain;

    if (IS_NPC(ch)) {
        /* Neat and fast */
        gain = GET_LEVEL(ch);
    } else {
        gain = graf(age(ch).year, 4, 8, 12, 16, 12, 10, 8);
        gain = gain + MIN(ch->char_specials.managain, 100);

        /* Class calculations */

        /* Skill/Spell calculations */

        /* Position calculations    */
        switch (GET_STANCE(ch)) {
        case STANCE_SLEEPING:
            gain <<= 1;
            break;
        case STANCE_RESTING:
            gain += (gain >> 1); /* Divide by 2 */
            break;
        default:
            if (GET_POS(ch) == POS_SITTING)
                gain += (gain >> 2); /* Divide by 4 */
            break;
        }

        gain = (gain * MANA_REGEN_FACTOR(ch)) / 100;

        /* Do not allow this code to operate on NPCs!  Crash! */
        /* if (IS_HUNGRY(ch) || IS_THIRSTY(ch))
           gain >>= 2; */
    }

    if (EFF_FLAGGED(ch, EFF_POISON))
        gain >>= 2;

    return (gain);
}

int hit_gain(struct char_data *ch)
/* Hitpoint gain pr. game hour */
{
    int gain;

    if (IS_NPC(ch)) {
        gain = GET_LEVEL(ch);
        /* Neat and fast */
    } else {

        gain = graf(age(ch).year, 8, 12, 20, 32, 16, 10, 4);

        /* This brings your max_hp into the formula... */
        gain = ((GET_MAX_HIT(ch) * .05) + gain) / 2;

        /* Max hitgain stat on a char is 100 */
        gain = gain + MIN(ch->char_specials.hitgain, 100) + 2;

        if (GET_RACE(ch) == RACE_TROLL)
            gain += gain * 2;

        /* Class/Level calculations */

        /* Skill/Spell calculations */

        /* Position calculations    */
        switch (GET_STANCE(ch)) {
        case STANCE_SLEEPING:
            gain += (gain << 2); /* Total = 5x */
            break;
        case STANCE_RESTING:
            gain += (gain << 1); /* Total = 3x */
            break;
        case STANCE_FIGHTING:
            gain = (gain >> 1); /* Total = 0.5x */
            break;
        default:
            if (GET_POS(ch) == POS_SITTING)
                gain += (gain >> 1); /* Total = 1.5x */
        }

        gain = (gain * HIT_REGEN_FACTOR(ch)) / 100;

        /* Do not allow this code to operate on NPCs!  Crash! */
        /* if (IS_HUNGRY(ch) || IS_THIRSTY(ch))
           gain >>= 2; */
    }

    if (EFF_FLAGGED(ch, EFF_POISON))
        gain >>= 2;

    return (gain);
}

int move_gain(struct char_data *ch)
/* move gain pr. game hour */
{
    int gain;

    if (IS_NPC(ch)) {
        return ((GET_MAX_MOVE(ch) * .1) + GET_LEVEL(ch));
        /* Neat and fast */
    } else {
        gain = graf(age(ch).year, 2, 3, 3, 5, 4, 3, 1);

        gain = ((GET_MAX_MOVE(ch) * .1) + gain) / 2;

        /* Class/Level calculations */

        /* Skill/Spell calculations */

        /* Position calculations    */
        switch (GET_STANCE(ch)) {
        case STANCE_SLEEPING:
            gain += (gain << 2); /* Total = 5x */
            break;
        case STANCE_RESTING:
            gain += (gain << 1); /* Total = 3x */
            break;
        case STANCE_FIGHTING:
            gain = (gain >> 1); /* Total = 0.5 x */
            break;
        default:
            if (GET_POS(ch) == POS_SITTING)
                gain += (gain >> 1); /* Total = 1.5x */
            break;
        }
        /* Do not allow this code to operate on NPCs!  Crash! */
        /* if (IS_HUNGRY(ch) || IS_THIRSTY(ch))
           gain >>= 2; */
    }

    if (EFF_FLAGGED(ch, EFF_POISON))
        gain >>= 2;

    gain = (gain * MV_REGEN_FACTOR(ch)) / 100;

    gain += (gain >> 1);

    return (gain);
}

void set_title(struct char_data *ch, char *title) {
    if (!title) {
        GET_TITLE(ch) = NULL;
        return;
    }

    if (strlen(title) >= MAX_TITLE_LENGTH)
        title[MAX_TITLE_LENGTH - 1] = '\0';

    if (GET_TITLE(ch))
        free(GET_TITLE(ch));

    GET_TITLE(ch) = strdup(title);
}

void gain_exp(struct char_data *ch, long gain, unsigned int mode) {
    int num_levels = 0;
    long xp_needed, old_xp;

    extern int level_gain;

    /* Make sure to give xp to the actual player, if shapechanged */
    ch = REAL_CHAR(ch);

    if (!IS_SET(mode, GAIN_IGNORE_LOCATION)) {
        /* Don't gain or lose xp in an arena room */
        if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA))
            return;
    }

    if (!IS_SET(mode, GAIN_IGNORE_MORTAL_BOUNDARY)) {
        /* Immortals don't gain exp */
        if (GET_LEVEL(ch) >= LVL_IMMORT)
            return;
    }

    if (!IS_SET(mode, GAIN_IGNORE_CHUNK_LIMITS)) {
        gain = MIN(max_exp_gain(ch), gain);
        gain = MAX(-exp_death_loss(ch, GET_LEVEL(ch)), gain);
    }

    /* NPCs don't worry about gaining levels, so we can get out */
    if (IS_NPC(ch)) {
        GET_EXP(ch) += gain;
        if (GET_EXP(ch) < 1)
            GET_EXP(ch) = 1;
        return;
    }

    old_xp = GET_EXP(ch);

    if (gain > 0) {
        if (!IS_SET(mode, GAIN_IGNORE_NAME_BOUNDARY)) {
            /* There's a level 10 limit for players with disapproved names */
            if (PLR_FLAGGED(ch, PLR_NAPPROVE) && GET_LEVEL(ch) >= 10) {
                cprintf(ch, AHCYN "You can gain no more levels or experience "
                                  "until your name is approved!\r\n" ANRM);
                return;
            }
        }

        GET_EXP(ch) += gain;

        if (!IS_SET(mode, GAIN_IGNORE_MORTAL_BOUNDARY)) {
            /* XP needed for ** */
            xp_needed = exp_next_level(99, GET_CLASS(ch));

            /* Cap exp at the ** amount (which is level 100 - 1 xp) */
            if (GET_EXP(ch) >= xp_needed)
                GET_EXP(ch) = xp_needed - 1;
        }

        /* XP needed for next level */
        xp_needed = exp_next_level(GET_LEVEL(ch), GET_CLASS(ch));

        if (!IS_SET(mode, GAIN_IGNORE_LEVEL_BOUNDARY)) {
            /* If you're ready to level, but need to go to the guildmaster to
             * gain it, you get set to next-level-xp minus 1
             */
            if (level_gain && GET_LEVEL(ch) < LVL_MAX_MORT && GET_EXP(ch) >= xp_needed &&
                !PRV_FLAGGED(ch, PRV_AUTO_GAIN))
                GET_EXP(ch) = xp_needed - 1;
        }

        /* Receive notification if you just became ready for next level
         * and level gaining is on, or if you just achieved ** for the first
         * time.
         */
        if (GET_EXP(ch) != old_xp && GET_EXP(ch) == xp_needed - 1) {
            if (level_gain && GET_LEVEL(ch) < LVL_MAX_MORT)
                cprintf(ch, AHCYN "You are ready for the next level!\r\n" ANRM);
            else if (GET_LEVEL(ch) == LVL_MAX_MORT) {
                if (PLR_FLAGGED(ch, PLR_GOTSTARS)) {
                    cprintf(ch, AFMAG "You got your %s " AFMAG "back again!\r\n" ANRM, CLASS_STARS(ch));
                    all_except_printf(ch, "%s regained %s %s!", GET_NAME(ch), HSHR(ch), CLASS_STARS(ch));
                } else {
                    SET_FLAG(PLR_FLAGS(ch), PLR_GOTSTARS);
                    cprintf(ch, AFMAG "You have achieved %s " AFMAG "status in %s" AFMAG "!!\r\n" ANRM, CLASS_STARS(ch),
                            CLASS_FULL(ch));
                    all_except_printf(ch, AFMAG "%s " AFMAG "has achieved %s " AFMAG "status in %s" AFMAG "!!\r\n" ANRM,
                                      GET_NAME(ch), CLASS_STARS(ch), CLASS_FULL(ch));
                }
            }
        }

        /* Check to see if the player is gaining any levels */
        while (GET_LEVEL(ch) < (IS_SET(mode, GAIN_IGNORE_MORTAL_BOUNDARY) ? LVL_IMPL : LVL_IMMORT) &&
               GET_EXP(ch) >= exp_next_level(GET_LEVEL(ch), GET_CLASS(ch))) {
            GET_LEVEL(ch) += 1;
            ++num_levels;
            advance_level(ch, LEVEL_GAIN);
        }

        if (num_levels && !IS_SET(mode, GAIN_IGNORE_NAME_BOUNDARY)) {
            if (PLR_FLAGGED(ch, PLR_NAPPROVE)) {
                cprintf(ch, AHCYN "Your name must be approved by a god before "
                                  "level 10!\r\nPlease contact a god as soon as "
                                  "possible!\r\n" ANRM);
            }
        }

        if (num_levels == 1) {
            cprintf(ch, AHWHT "You gain a level!\r\n" ANRM);
            all_except_printf(ch, "%s advanced to level %d!", GET_NAME(ch), GET_LEVEL(ch));
            mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
        } else if (num_levels > 1) {
            cprintf(ch, AHWHT "You gain %d levels to %d!\r\n" ANRM, num_levels, GET_LEVEL(ch));
            all_except_printf(ch, "%s advances %d levels to level %d!", GET_NAME(ch), num_levels,
                              GET_LEVEL(ch));
            mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "%s advanced to level %d (from %d)", GET_NAME(ch),
                    GET_LEVEL(ch), GET_LEVEL(ch) - num_levels);
        }
    } else if (gain < 0) {
        GET_EXP(ch) += gain;
        if (GET_EXP(ch) < 1)
            GET_EXP(ch) = 1;
        while ((GET_LEVEL(ch) < LVL_IMMORT || IS_SET(mode, GAIN_IGNORE_MORTAL_BOUNDARY)) &&
               GET_EXP(ch) <= exp_next_level(GET_LEVEL(ch) - 1, GET_CLASS(ch))) {
            GET_LEVEL(ch) -= 1;
            ++num_levels;
            advance_level(ch, LEVEL_LOSE);
        }
        if (num_levels == 1) {
            cprintf(ch, AHWHT "You lose a level!\r\n" ANRM);
            all_except_printf(ch, "%s lost level %d!", GET_NAME(ch), GET_LEVEL(ch) + 1);
            mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "%s lost level %d", GET_NAME(ch), GET_LEVEL(ch) + 1);
        } else if (num_levels > 1) {
            cprintf(ch, AHWHT "You lose %d levels from %d to %d!\r\n" ANRM, num_levels, GET_LEVEL(ch) + num_levels,
                    GET_LEVEL(ch));
            all_except_printf(ch, "%s lost %d levels from %d to %d!", GET_NAME(ch), num_levels,
                              GET_LEVEL(ch) + num_levels, GET_LEVEL(ch));
            mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "%s lost %d levels from %d to %d", GET_NAME(ch),
                    num_levels, GET_LEVEL(ch) + num_levels, GET_LEVEL(ch));
        }
    }
}

void gain_condition(struct char_data *ch, int condition, int value) {
    bool intoxicated;

    /* mobs don't get hungry */
    if (IS_NPC(ch))
        return;

    /* already completely hungry/thirsty? */
    if (GET_COND(ch, condition) == -1)
        return;

    intoxicated = (GET_COND(ch, DRUNK) > 0);

    GET_COND(ch, condition) += value;

    /* update regen rates if we were just on empty */
    if ((condition != DRUNK) && (value > 0) && (GET_COND(ch, condition) == value))
        check_regen_rates(ch);

    GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
    GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

    if (PLR_FLAGGED(ch, PLR_WRITING))
        return;

    if (ch->desc && EDITING(ch->desc))
        return;

    if (GET_COND(ch, condition) == 5) {
        switch (condition) {
        case FULL:
            send_to_char("You're a little hungry.\r\n", ch);
            return;
        case THIRST:
            send_to_char("You're a little thirsty.\r\n", ch);
            return;
        }
    } else if (!GET_COND(ch, condition)) {
        switch (condition) {
        case FULL:
            send_to_char("You are hungry.\r\n", ch);
            return;
        case THIRST:
            send_to_char("You are thirsty.\r\n", ch);
            return;
        case DRUNK:
            if (intoxicated)
                send_to_char("You are now sober.\r\n", ch);
            return;
        default:
            break;
        }
    }
}

void check_idling(struct char_data *ch) {
    void perform_immort_invis(struct char_data * ch, int level); /* act.wizard.c */

    ++(ch->char_specials.timer);

    /*
     * Also increment the timer on the mob the player is shapechanged into.
     * This is to keep track of how long they are in a shapechanged state.
     */
    if (ch->forward)
        ++(ch->forward->char_specials.timer);

    /*  Scheme for immortal+ idling:  */
    /*  1. After 10 minutes of idle time, auto-invis to level 100. */
    /*  Assumptions: 75-second ticks */
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        /*  this char is an immort+ */

        if (ch->char_specials.timer >= 8) {
            /* this immort has been idle for 10+ mins */
            if (GET_AUTOINVIS(ch) > 0 && GET_INVIS_LEV(ch) < GET_AUTOINVIS(ch) ) {
                /* this char is not already invis beyond the immorts invis level */
                sprintf(buf, "You have been idle for ten minutes.  Auto-invis to level %d engaged.\r\n", GET_AUTOINVIS(ch));
                send_to_char(buf, ch);
                perform_immort_invis(ch, GET_AUTOINVIS(ch));
            }
        }
        return;
    }

    if (ch->char_specials.timer >= 4) {
        if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE && (!ch->forward || ch->in_room != 0)) {
            GET_WAS_IN(ch) = ch->in_room;
            act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
            send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
            save_player(ch);
            char_from_room(ch);
            char_to_room(ch, 0);
        } else if (ch->char_specials.timer >= 7) {
            if (ch->forward && ch->forward->desc) {
                do_shapechange(ch->forward, "me", 0, 1);
            } else {
                if (ch->in_room != NOWHERE && ch->in_room != 0) {
                    char_from_room(ch);
                    char_to_room(ch, 0);
                }
                if (ch->desc)
                    close_socket(ch->desc);
                ch->desc = NULL;
                sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
                mudlog(buf, BRF, LVL_GOD, TRUE);
                remove_player_from_game(ch, QUIT_TIMEOUT);
            }
        }
    }
}

void weardown_light(struct obj_data *obj) {
    struct char_data *ch;
    char *lightmsg = NULL;

    /* Don't wear down permanant lights. */
    if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT)
        return;

    /* Make a note of who, if anyone, is carrying this light */
    ch = obj->carried_by ? obj->carried_by : obj->worn_by;

    /* give warning of impending light failure */
    switch (--GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING)) { /* weardown light */
    case 5:
        lightmsg = "begins to flicker and fade.";
        break;
    case 3:
        lightmsg = "grows slightly dimmer.";
        break;
    case 1:
        lightmsg = "is almost out.";
        break;
    case 0:
        lightmsg = "sputters out and dies.";
        break;
    default:
        lightmsg = NULL;
    }

    if (lightmsg) {
        if (ch) {
            /* Messages sent when the light's being carried by someone */
            sprintf(buf, "&3Your $o&3 %s&0", lightmsg);
            act(buf, FALSE, ch, obj, 0, TO_CHAR);
            sprintf(buf, "&3$n&3's $o&3 %s&0", lightmsg);
            act(buf, FALSE, ch, obj, 0, TO_ROOM);
        } else if (obj->in_room != NOWHERE && world[obj->in_room].people) {
            /* Messages sent when the light is on the ground */
            sprintf(buf, "&3$p&3 %s&0", lightmsg);
            act(buf, FALSE, world[obj->in_room].people, obj, 0, TO_ROOM);
            act(buf, FALSE, world[obj->in_room].people, obj, 0, TO_CHAR);
        }
    }

    if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) == 0) {
        /* The fuel is now expended. */
        if (obj->in_room != NOWHERE)
            world[obj->in_room].light--;
        else if (ch)
            world[ch->in_room].light--;
        /* Set the object to "not lit" */
        GET_OBJ_VAL(obj, VAL_LIGHT_LIT) = FALSE;
    }
}

void extract_corpse(struct obj_data *obj) {
    struct obj_data *i, *next;

    if (obj->carried_by)
        act("$p decays in your hands.", FALSE, obj->carried_by, obj, 0, TO_CHAR);
    else if ((obj->in_room != NOWHERE) && (world[obj->in_room].people)) {
        act("A quivering horde of maggots consumes $p.", TRUE, world[obj->in_room].people, obj, 0, TO_ROOM);
        act("A quivering horde of maggots consumes $p.", TRUE, world[obj->in_room].people, obj, 0, TO_CHAR);
    }

    for (i = obj->contains; i; i = next) {
        next = i->next_content; /* Next in inventory */
        obj_from_obj(i);
        if (obj->in_obj)
            obj_to_obj(i, obj->in_obj);
        else if (obj->carried_by)
            obj_to_room(i, obj->carried_by->in_room);
        else if (obj->in_room != NOWHERE) {
            obj_to_room(i, obj->in_room);
            start_decomposing(i);
        } else
            assert(FALSE);
    }

    extract_obj(obj);
}

#define BLOOD_DROP_OBJ 34 /* the vnum of the blood object */
#define BLOOD_POOL_OBJ 35 /* the vnum of the blood object */
void decay_object(struct obj_data *obj) {
    char *msg;

    /* Nothing special if it's inside another object */
    if (obj->in_room == NOWHERE) {
        extract_obj(obj);
        return;
    }

    /* The default crumble message. */
    msg = "$p&0 crumbles to dust and blows away.";

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_CONTAINER:
        /* Corpses get special treatment. */
        if (IS_CORPSE(obj))
            extract_corpse(obj);
        return;
    case ITEM_PORTAL:
        /* Pick a message based on object vnum. */
        switch (GET_OBJ_VNUM(obj)) {
        case OBJ_VNUM_MOONWELL:
            msg = "$p fades from existence.";
            break;
        case OBJ_VNUM_HEAVENSGATE:
            msg = "The glowing tunnel of light closes into nothingness.";
            break;
        case OBJ_VNUM_HELLGATE:
            msg = "The gaping hole in the ground closes, leaving a blackened piece "
                  "of earth.";
            break;
        }
        break;
    case ITEM_WALL:
        /* Choose a message based on the material of the wall.
         * We determine the material from the spell that caused it, for now.
         * At some time in the future, objects may have their materials
         * identified more explicitly. */
        switch (GET_OBJ_VAL(obj, VAL_WALL_SPELL)) {
        case SPELL_WALL_OF_ICE:
            msg = "$p &7&8shatters&0 into small, &4quickly-melting&0 shards.";
            break;
        }
        break;
    case ITEM_OTHER:
        /* Blood pools "crumble" into smaller blood droplets. */
        if (GET_OBJ_VNUM(obj) == BLOOD_POOL_OBJ) {
            struct obj_data *temp = read_object(BLOOD_DROP_OBJ, VIRTUAL);
            GET_OBJ_DECOMP(temp) = 2;
            GET_OBJ_VAL(temp, VAL_BLOOD_ROOM) = obj->in_room;
            obj_to_room(temp, obj->in_room);
            /* Silent extraction. */
            msg = NULL;
        }
        break;
    case ITEM_FOUNTAIN:
        /* Druid's fountain. */
        if (GET_OBJ_VNUM(obj) == 75)
            msg = "$p dries up.";
        break;
    }

    if (world[obj->in_room].people) {
        act(msg, TRUE, world[obj->in_room].people, obj, 0, TO_ROOM);
        act(msg, TRUE, world[obj->in_room].people, obj, 0, TO_CHAR);
    }
    extract_obj(obj);
}

/* Update PCs, NPCs, and objects */
void point_update(void) {
    struct char_data *i, *next_char;
    struct obj_data *j;

    extern struct obj_data *go_iterator;

    /* characters */
    for (i = character_list; i; i = next_char) {
        next_char = i->next;

        if (GET_STANCE(i) >= STANCE_STUNNED) {
            if (EFF_FLAGGED(i, EFF_POISON))
                damage(i, i, GET_LEVEL(i) / 2, SPELL_POISON);
            if (GET_STANCE(i) == STANCE_DEAD)
                continue;
            /* Do damage with a maximum of 5% of HP for PCs and 2% for NPCs,
             * depending on level. */
            if (EFF_FLAGGED(i, EFF_ON_FIRE))
                damage(i, i, ((GET_MAX_HIT(i) / (IS_NPC(i) ? 50 : 20)) * GET_LEVEL(i)) / 100 + 1, SPELL_ON_FIRE);
            if (DECEASED(i))
                continue;
        }

        if (EFF_FLAGGED(i, EFF_DISEASE)) {
            act("&3$n&3 pauses a moment as $e purges $s stomach contents.&0", TRUE, i, 0, 0, TO_ROOM);
            act("&3You feel VERY ill and purge the contents of your stomach.&0", FALSE, i, 0, 0, TO_CHAR);
            gain_condition(i, FULL, -6);
            gain_condition(i, DRUNK, -1);
            gain_condition(i, THIRST, -6 * HOURLY_THIRST_CHANGE);
        }

        if (IS_NPC(i))
            continue;

        if (IS_HUNGRY(i)) {
            if (IS_THIRSTY(i))
                ;
            /* send_to_char("Your hunger and thirst are draining your energy
             * rapidly.\r\n", i); */
            else {
                /* send_to_char("You are feeling weak from hunger.\r\n", i); */
                gain_condition(i, THIRST, -HOURLY_THIRST_CHANGE);
            }
        } else if (IS_THIRSTY(i)) {
            /* send_to_char("You feel dizzy from extreme thirst.\r\n", i); */
            gain_condition(i, FULL, -1);
        } else {
            gain_condition(i, FULL, -1);
            gain_condition(i, THIRST, -HOURLY_THIRST_CHANGE);
        }
        gain_condition(i, DRUNK, -1);

        check_idling(i);
    }

    /* objects:
     *   -- lights run out
     *   -- objects decay
     */
    for (j = object_list; j; j = go_iterator) {
        /* See handler.c, extract_obj() about go_iterator */
        go_iterator = j->next;

        /* Try to catch invalid objects that could crash the mud. */
        if (j->in_room < 0 || j->in_room > top_of_world) {
            if (!(j->worn_by || j->carried_by || j->in_obj)) {
                mudlog("POINT_UPDATE OBJ ERROR: Object in NOWHERE, extracting", NRM, LVL_GOD, TRUE);
                sprintf(buf, "Object %d, \"%s\"", GET_OBJ_VNUM(j), j->name);
                mudlog(buf, NRM, LVL_GOD, TRUE);
                extract_obj(j);
                continue;
            }
        }

        if (GET_OBJ_TYPE(j) == ITEM_LIGHT && GET_OBJ_VAL(j, VAL_LIGHT_LIT)) /* Is lit */
            weardown_light(j);

        /* Decompose things */
        if (OBJ_FLAGGED(j, ITEM_DECOMP)) {
            if (GET_OBJ_DECOMP(j) > 0)
                --GET_OBJ_DECOMP(j);
            if (!GET_OBJ_DECOMP(j)) {
                /* The item's decomp timer has run out. Check its trigger, then
                 * decay it. */

                /* If timer_otrigger returns 0, the normal timeout action should not
                 * occur.  In this case, if the trigger doesn't opurge the object,
                 * then we will attempt to run the trigger again in the next tick. */
                if (!timer_otrigger(j))
                    continue;
                decay_object(j);
            }
        }
    }
}

/* Set something decomposing.  A player probably dropped it.
 *
 * The time to decompose is in ticks. A tick is approximately 75 real seconds.
 */
void start_decomposing(struct obj_data *obj) {
    struct obj_data *o;
    int ticks;

    if (GET_OBJ_LEVEL(obj) < 100 && !OBJ_FLAGGED(obj, ITEM_PERMANENT)) {
        /* Decide how long it will take the object to decompose. */
        ticks = GET_OBJ_LEVEL(obj) + 11;

        /* Add approximately 4 real hours if it's a key. */
        if (GET_OBJ_TYPE(obj) == ITEM_KEY)
            ticks += 192;

        /* It's OK to raise the amount of time to decomp, but not to lower it.
         * Some objects were given higher values (such as player corpses).
         * Then a god or someone might have picked up the object and dropped it.
         * If we blindly reset the number here, that action might have sped up the
         * decomposition undesirably. */
        if (GET_OBJ_DECOMP(obj) < ticks)
            GET_OBJ_DECOMP(obj) = ticks;
        SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_DECOMP);
    }

    /* The contents of corpses don't decompose. */
    if (!IS_CORPSE(obj))
        for (o = obj->contains; o; o = o->next_content)
            start_decomposing(o);
}

void stop_decomposing(struct obj_data *obj) {
    struct obj_data *o;

    REMOVE_FLAG(GET_OBJ_FLAGS(obj), ITEM_DECOMP);
    for (o = obj->contains; o; o = o->next_content)
        stop_decomposing(o);
}

/***************************************************************************
 * $Log: limits.c,v $
 * Revision 1.119  2009/07/17 00:48:17  myc
 * Implemented auto gain privilege.
 *
 * Revision 1.118  2009/06/09 21:50:21  myc
 * Clan notification when someone attains their stars after the
 * initial time.  clan_notification now adds the color codes for you!
 *
 * Revision 1.117  2009/06/09 19:33:50  myc
 * Rewrote gain_exp and retired gain_exp_regardless.
 *
 * Revision 1.116  2009/06/09 05:42:14  myc
 * Removing some code that is no longer required in gain_exp_regardless
 * due to the new clan implementation.
 *
 * Revision 1.115  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.114  2009/02/11 17:03:39  myc
 * Adding check for EDITING(d) where PLR_WRITING is checked in
 * gain_condition.
 *
 * Revision 1.113  2008/09/29 03:24:44  jps
 * Make container weight automatic. Move some liquid container functions to
 *objects.c.
 *
 * Revision 1.112  2008/09/21 21:50:56  jps
 * Stop trying to keep track of who's attacking who when there's a shapechange,
 * since do_shapechange handles that internally now.
 *
 * Revision 1.111  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.110  2008/09/20 22:38:56  jps
 * Keys will take an extra four hours (real time) to decompose.
 *
 * Revision 1.109  2008/09/20 06:05:06  jps
 * Add macros POSSESSED and POSSESSOR.
 *
 * Revision 1.108  2008/09/08 05:17:42  jps
 * Fix warning
 *
 * Revision 1.107  2008/09/07 20:07:59  jps
 * Changed gain_exp to use exp_next_level correctly. It will send a mudwide
 * message when you first achieve **, and a personal message whenever you
 * get them back after losing them.
 *
 * Revision 1.106  2008/09/06 19:11:27  jps
 * Change the way corpses decompose.
 *
 * Revision 1.105  2008/09/02 07:26:20  jps
 * Better looping when decomposing an object's contents.
 *
 * Revision 1.104  2008/09/02 07:17:04  jps
 * Going to DECOMP
 *
 * Revision 1.103  2008/09/02 07:03:03  jps
 * Still honor TRANSIENT flag for now
 *
 * Revision 1.102  2008/09/02 06:51:48  jps
 * Changed the way things decompose: they use the DECOMP flag.
 *
 * Revision 1.101  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.100  2008/08/30 01:31:51  myc
 * Changed the way stats are calculated in effect_total; ability
 * stats are saved in a raw form now, and only capped when accessed.
 * Damroll and hitroll are recalculated everytime effect_total
 * is called, using cached base values.
 *
 * Revision 1.99  2008/07/27 05:27:56  jps
 * Using save_player and remove_player_from_game functions.
 *
 * Revision 1.98  2008/07/21 19:17:07  jps
 * Remove auto-AFK for immortals.
 *
 * Revision 1.97  2008/06/07 19:06:46  myc
 * Moved all object-related constants and structures to objects.h
 *
 * Revision 1.96  2008/06/05 02:07:43  myc
 * Rewrote rent saving to use ascii object files.
 *
 * Revision 1.95  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.94  2008/05/11 05:56:24  jps
 * Don't do suffering damage from point_update - bleeding out is
 * already accomplished in regen.c.
 *
 * Revision 1.93  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.92  2008/03/30 17:30:38  jps
 * Renamed objsave.c to pfiles.c and introduced pfiles.h. Files using functions
 * from pfiles.c now include pfiles.h and depend on it in the makefile.
 *
 * Revision 1.91  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.90  2008/03/20 03:26:05  myc
 * Fixed set_title again.
 *
 * Revision 1.89  2008/03/20 03:14:56  myc
 * Fix crash bug in set_title (trying to strlen possibly null pointer).
 *
 * Revision 1.88  2008/03/10 20:46:55  myc
 * Moving innate timers to the cooldown system.
 *
 * Revision 1.87  2008/03/05 03:03:54  myc
 * Title is saved differently in pfiles now, so must check for NULL
 * instead of empty string.
 *
 * Revision 1.86  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.85  2008/02/02 19:38:20  myc
 * Fixed possible buffer overwrite in set_title.
 *
 * Revision 1.84  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.83  2008/01/27 21:09:12  myc
 * Replace hit() with attack().
 *
 * Revision 1.82  2008/01/27 12:11:21  jps
 * Use regen factors in class.c.
 *
 * Revision 1.81  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.80  2008/01/05 05:39:41  jps
 * Changed name of save_char() to save_player().
 *
 * Revision 1.79  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.78  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.77  2007/12/25 20:37:32  jps
 * Apply the xp gain cap to NPCs as well.
 *
 * Revision 1.76  2007/12/25 17:03:32  jps
 * Minor cleanup.
 *
 * Revision 1.75  2007/12/25 06:22:19  jps
 * Hey, room 0 is ok!
 *
 * Revision 1.74  2007/12/23 19:27:26  jps
 * Use global object iterator to prevent decay events from operating on
 * already-extracted objects.
 *
 * Revision 1.73  2007/12/23 18:09:57  jps
 * Fixed bug in invalid decaying object detection.
 *
 * Revision 1.72  2007/12/20 03:04:23  jps
 * Fix bad bad-object extractor.
 *
 * Revision 1.71  2007/12/19 20:53:25  myc
 * Fixed possible crash bug in set_title.  Fixed gain_exp_regardless
 * to automatically remove a player from a clan if they are advanced
 * to clan god status.  save_player() no longer requires you to supply
 * a load/save room (which wasn't being used anyway).
 *
 * Revision 1.70  2007/12/17 18:26:27  jps
 * Try to catch some buggy objects.
 *
 * Revision 1.69  2007/11/02 02:32:46  jps
 * Typo fix when decaying a hellgate.
 *
 * Revision 1.68  2007/10/27 18:56:26  myc
 * When a mob dies of fire, poison, of suffering, if they were fighting
 * someone, their opponent gets the exp for the kill.
 *
 * Revision 1.67  2007/10/17 17:22:57  myc
 * If you die by fire while fighting, your opponent gets the kill.
 *
 * Revision 1.66  2007/10/04 16:20:24  myc
 * Transient item flag now makes things decay when they are on the ground.
 * Cleaned up point_update a bunch.  Took out update_char_objects, and
 * moved all the object decay stuff to functions.
 *
 * Revision 1.65  2007/10/02 02:52:27  myc
 * Fixed idle timeout for shapechanged druids.  (It checks the shapechanged
 * mob instead of the idle druid...sort of.)
 *
 * Revision 1.64  2007/09/21 18:08:04  jps
 * Stop the continuing nag messages about hunger and thirst.  Regeneration
 * won't be affected by hunger an thirst.
 *
 * Revision 1.63  2007/09/03 21:18:38  jps
 * Standardize magic wall expiration.
 *
 * Revision 1.62  2007/08/26 01:55:41  myc
 * Fire now does real damage.  All fire spells have a chance to catch the
 * victim on fire.  Mobs attempt to douse themselves.
 *
 * Revision 1.61  2007/08/04 01:15:32  jps
 * Prevent xp gain/loss in arena rooms.
 *
 * Revision 1.60  2007/07/31 23:44:36  jps
 * New macros IS_HUNGRY, IS_THIRSTY, IS_DRUNK.
 *
 * Revision 1.59  2007/07/25 01:36:18  jps
 * Clean up check_idling().
 *
 * Revision 1.58  2007/05/28 06:25:26  jps
 * Added coloring to light burnout messages.
 *
 * Revision 1.57  2007/04/11 07:50:03  jps
 * Improve light rundown feedback.
 *
 * Revision 1.56  2007/03/27 04:27:05  myc
 * Changed spellings for innate timer macros.
 *
 * Revision 1.55  2006/12/28 23:46:29  myc
 * Fixed typo in light failing message.
 *
 * Revision 1.54  2006/12/05 20:50:57  myc
 * Bug causing too many messages for failing lights on heartbeats.
 *
 * Revision 1.53  2006/12/05 18:37:46  myc
 * More warning before light failure.
 *
 * Revision 1.52  2006/11/21 03:45:52  jps
 * Running down of lights is handled here.  Lights on the ground
 * run down too.
 *
 * Revision 1.51  2006/11/16 16:59:30  jps
 * Add warning messages "You're a little hungry/thirsty" when
 * you reach 5 in those values.
 *
 * Revision 1.50  2006/11/13 04:15:10  jps
 * Fix occasional crash caused by checking mobs for hunger/thirst
 *
 * Revision 1.49  2006/11/11 10:11:04  jps
 * The first message received when you feel hungry or thirsty is the
 * same, but subsequent ones are a little more alerting to the fact
 * that being hungry and/or thirsty isn't a good state to be in.
 *
 * Revision 1.48  2006/11/08 21:28:21  jps
 * Once again, being hungry or thirsty cuts regeneration to 25% of normal.
 *
 * Revision 1.47  2006/11/08 07:58:23  jps
 * Typo fix "raise a level" -> "gain a level"
 *
 * Revision 1.46  2006/05/30 00:49:22  rls
 * Modified poison damage to base off level of poisoned player... I know, crazy!
 *
 * Revision 1.45  2004/11/19 20:43:16  rsd
 * back-rev'd to version 1.43
 *
 * Revision 1.43  2004/11/01 05:30:33  jjl
 * LVL_IMMORT now idles out, LVL_GOD+ do not.
 *
 * Revision 1.42  2003/07/14 05:46:14  rsd
 * altered the header to update copyright date and
 * to test is new load will allow RCS to function
 *
 * Revision 1.41  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.40  2002/05/13 23:35:23  dce
 * Fixed a bug where idling out when shapechanged cause you leave
 * the mob behind.
 *
 * Revision 1.39  2001/10/12 17:53:35  rjd
 * New immortal+ idle scheme implemented: 5 min idle activates AFK, 10 min idle
 *activates invis 100.
 *
 * Revision 1.38  2001/10/11 18:33:54  rjd
 * Took care of nit-pick warning. :P
 *
 * Revision 1.37  2001/10/10 23:30:22  rjd
 * Immortals+ who are idle for five or more tics have an auto-toggle-on for the
 *AFK toggle.
 *
 * Revision 1.36  2001/04/08 13:51:25  dce
 * TEMPORARY fix to shapechanged players voiding out.
 *
 * Revision 1.35  2001/03/31 00:19:42  dce
 * Think I fixed a crash bug with move gain.
 *
 * Revision 1.33  2001/02/24 04:04:15  dce
 * Shapechanged players gain experience
 *
 * Revision 1.32  2000/11/22 20:28:32  rsd
 * added back rlog messages from prior to the addition
 * of the $log$ string.
 *
 * Revision 1.31  2000/10/15 04:59:02  cmc
 * somehow forgot the "you are ready" message.
 * I wonder what color it'll end up being?
 * picked it randomly from an important looking message.
 *
 * Revision 1.30  2000/10/15 04:37:59  cmc
 * fixed level gain code so that ** could be achieved
 *
 * Revision 1.29  2000/10/13 17:52:56  cmc
 * optional "level gain" code implemented
 *
 * Revision 1.28  2000/09/13 22:21:27  rsd
 * made the idle timer work on 99 and below instead of
 * 100 and below.
 *
 * Revision 1.27  2000/05/01 01:31:22  rsd
 * removed all the gain_exp() changes
 * because it didn't even work.
 *
 * Revision 1.19  2000/04/22 22:37:47  rsd
 * Fixed the comment header, also fixed typo in You loose a level!
 *
 * Revision 1.18  2000/03/20 04:34:40  rsd
 * Commented out all references to autowiz.
 *
 * Revision 1.17  1999/12/10 22:13:45  jimmy
 * Exp tweaks.  Made Exp loss for dying a hardcoded 25% of what was needed for
 *the next level.  Fixed problems with grouping and exp.  Removed some redundant
 *and unnecessary exp code.
 *
 * Revision 1.16  1999/11/28 23:32:08  cso
 * point_update: modified to use IS_CORPSE macro
 *
 * Revision 1.15  1999/10/30 15:37:24  rsd
 * Jimmy coded alignemt restrictions for Paladins and exp.
 * Added a victim check in gain_exp() to check victims alignment.
 * Also coded in the ranges for alignment and the exp modifiers
 * for the different victim alignments.
 *
 * Revision 1.14  1999/09/08 07:06:03  jimmy
 * More insure++ runtime fixes.  Some small, but hardcore fixes mostly to do
 * with blood and killing
 * --gurlaek
 *
 * Revision 1.13  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.12  1999/08/14 02:43:10  dce
 * ** is one level up from 99
 *
 * Revision 1.11  1999/08/12 17:54:46  dce
 * Fixed experience so that there are no overflows of integers that are placed
 *into longs. Main problem was max_exp_gain and max_exp_loss. Both were
 *overflowing due to poor Hubis coding.
 *
 * Revision 1.10  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list.  This code compiles fine under both gcc RH5.2 and egcs RH6.0
 *
 * Revision 1.9  1999/05/04 17:19:33  dce
 * Name accept system...version one...original code by Fingh, fixed up to work
 * by Zantir.
 *
 * Revision 1.8  1999/05/01 18:45:19  dce
 * Players camp/rent after 8 min of idle time
 *
 * Revision 1.7  1999/04/08 03:37:33  dce
 * Fixed a nasty crash bug
 *
 * Revision 1.6  1999/03/31 20:17:22  jen
 * Changed move & hp regen code to increase event rates
 *
 * Revision 1.5  1999/03/17 22:45:34  jimmy
 * added check for NPC's to gain_condition so that
 * it would immediatly return for a mob.
 * fingon
 *
 * Revision 1.4  1999/03/14 14:28:11  jimmy
 * Movement now has bite!  removed extra "flying" from
 * movement_loss in constants.c to fix the mv bug.  reduced the
 * movement gain by 5 for all ages in limits.c.  Removed the +5
 * and +6 static movement gain so that it now actually updates
 * based on the function in regen.c.  Gosh i'm a bastard.
 * Fingon
 *
 * Revision 1.3  1999/02/04 00:02:59  jimmy
 * max/min exp loss/gain set to 2 notches.
 *
 * Revision 1.2  1999/01/31 16:11:37  mud
 * Indented file
 * moved the last 3 }'s to lines by themsleves and indented
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/
