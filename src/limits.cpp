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

#include "limits.hpp"

#include "casting.hpp"
#include "clan.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "math.hpp"
#include "pfiles.hpp"
#include "players.hpp"
#include "races.hpp"
#include "regen.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

long max_exp_gain(CharData *ch);
long exp_death_loss(CharData *ch, int level);

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
int mana_gain(CharData *ch) {
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

int hit_gain(CharData *ch)
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
        if (EFF_FLAGGED(ch, EFF_SONG_OF_REST) &&
            (GET_STANCE(ch) == STANCE_SLEEPING || GET_STANCE(ch) == STANCE_RESTING))
            gain += (gain << 1);

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

int move_gain(CharData *ch)
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
        if (EFF_FLAGGED(ch, EFF_SONG_OF_REST) &&
            (GET_STANCE(ch) == STANCE_SLEEPING || GET_STANCE(ch) == STANCE_RESTING))
            gain += (gain << 1);

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

void set_title(CharData *ch, char *title) {
    if (!title) {
        GET_TITLE(ch) = nullptr;
        return;
    }

    if (strlen(title) >= MAX_TITLE_LENGTH)
        title[MAX_TITLE_LENGTH - 1] = '\0';

    if (GET_TITLE(ch))
        free(GET_TITLE(ch));

    GET_TITLE(ch) = strdup(title);
}

void gain_exp(CharData *ch, long gain, unsigned int mode) {
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
                char_printf(ch, AHCYN "You can gain no more levels or experience until your name is approved!\n" ANRM);
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
                char_printf(ch, AHCYN "You are ready for the next level!\n" ANRM);
            else if (GET_LEVEL(ch) == LVL_MAX_MORT) {
                if (PLR_FLAGGED(ch, PLR_GOTSTARS)) {
                    char_printf(ch, AFMAG "You got your %s " AFMAG "back again!\n" ANRM, CLASS_STARS(ch));
                    all_except_printf(ch, "%s regained %s %s!\n", GET_NAME(ch), HSHR(ch), CLASS_STARS(ch));
                } else {
                    SET_FLAG(PLR_FLAGS(ch), PLR_GOTSTARS);
                    char_printf(ch, AFMAG "You have achieved %s " AFMAG "status in %s" AFMAG "!!\n" ANRM,
                                CLASS_STARS(ch), CLASS_FULL(ch));
                    all_except_printf(ch, AFMAG "%s " AFMAG "has achieved %s " AFMAG "status in %s" AFMAG "!!\n" ANRM,
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
                char_printf(ch, AHCYN
                            "Your name must be approved by a god before "
                            "level 10!\nPlease contact a god as soon as "
                            "possible!\n" ANRM);
            }
        }

        if (num_levels == 1) {
            char_printf(ch, AHWHT "You gain a level!\n" ANRM);
            all_except_printf(ch, "%s advanced to level %d!\n", GET_NAME(ch), GET_LEVEL(ch));
            mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
        } else if (num_levels > 1) {
            char_printf(ch, AHWHT "You gain %d levels to %d!\n" ANRM, num_levels, GET_LEVEL(ch));
            all_except_printf(ch, "%s advances %d levels to level %d!\n", GET_NAME(ch), num_levels, GET_LEVEL(ch));
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
            char_printf(ch, AHWHT "You lose a level!\n" ANRM);
            all_except_printf(ch, "%s lost level %d!\n", GET_NAME(ch), GET_LEVEL(ch) + 1);
            mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "%s lost level %d", GET_NAME(ch), GET_LEVEL(ch) + 1);
        } else if (num_levels > 1) {
            char_printf(ch, AHWHT "You lose %d levels from %d to %d!\n" ANRM, num_levels, GET_LEVEL(ch) + num_levels,
                        GET_LEVEL(ch));
            all_except_printf(ch, "%s lost %d levels from %d to %d!\n", GET_NAME(ch), num_levels,
                              GET_LEVEL(ch) + num_levels, GET_LEVEL(ch));
            mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "%s lost %d levels from %d to %d", GET_NAME(ch),
                    num_levels, GET_LEVEL(ch) + num_levels, GET_LEVEL(ch));
        }
    }
}

void gain_condition(CharData *ch, int condition, int value) {
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
            send_to_char("You're a little hungry.\n", ch);
            return;
        case THIRST:
            send_to_char("You're a little thirsty.\n", ch);
            return;
        }
    } else if (!GET_COND(ch, condition)) {
        switch (condition) {
        case FULL:
            send_to_char("You are hungry.\n", ch);
            return;
        case THIRST:
            send_to_char("You are thirsty.\n", ch);
            return;
        case DRUNK:
            if (intoxicated)
                send_to_char("You are now sober.\n", ch);
            return;
        default:
            break;
        }
    }
}

void check_idling(CharData *ch) {
    void perform_immort_invis(CharData * ch, int level); /* act.wizard.c */

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
            if (GET_AUTOINVIS(ch) > 0 && GET_INVIS_LEV(ch) < GET_AUTOINVIS(ch)) {
                /* this char is not already invis beyond the immorts invis level */
                sprintf(buf, "You have been idle for ten minutes.  Auto-invis to level %d engaged.\n",
                        GET_AUTOINVIS(ch));
                send_to_char(buf, ch);
                perform_immort_invis(ch, GET_AUTOINVIS(ch));
            }
        }
        return;
    }

    if (ch->char_specials.timer >= 4) {
        if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE && (!ch->forward || ch->in_room != 0)) {
            GET_WAS_IN(ch) = ch->in_room;
            act("$n disappears into the void.", true, ch, 0, 0, TO_ROOM);
            send_to_char("You have been idle, and are pulled into a void.\n", ch);
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
                ch->desc = nullptr;
                sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
                mudlog(buf, BRF, LVL_GOD, true);
                remove_player_from_game(ch, QUIT_TIMEOUT);
            }
        }
    }
}

void weardown_light(ObjData *obj) {
    CharData *ch;
    char *lightmsg = nullptr;

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
        lightmsg = nullptr;
    }

    if (lightmsg) {
        if (ch) {
            /* Messages sent when the light's being carried by someone */
            sprintf(buf, "&3Your $o&3 %s&0", lightmsg);
            act(buf, false, ch, obj, 0, TO_CHAR);
            sprintf(buf, "&3$n&3's $o&3 %s&0", lightmsg);
            act(buf, false, ch, obj, 0, TO_ROOM);
        } else if (obj->in_room != NOWHERE && world[obj->in_room].people) {
            /* Messages sent when the light is on the ground */
            sprintf(buf, "&3$p&3 %s&0", lightmsg);
            act(buf, false, world[obj->in_room].people, obj, 0, TO_ROOM);
            act(buf, false, world[obj->in_room].people, obj, 0, TO_CHAR);
        }
    }

    if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) == 0) {
        /* The fuel is now expended. */
        if (obj->in_room != NOWHERE)
            world[obj->in_room].light--;
        else if (ch)
            world[ch->in_room].light--;
        /* Set the object to "not lit" */
        GET_OBJ_VAL(obj, VAL_LIGHT_LIT) = false;
    }
}

void extract_corpse(ObjData *obj) {
    ObjData *i, *next;

    if (obj->carried_by)
        act("$p decays in your hands.", false, obj->carried_by, obj, 0, TO_CHAR);
    else if ((obj->in_room != NOWHERE) && (world[obj->in_room].people)) {
        act("A quivering horde of maggots consumes $p.", true, world[obj->in_room].people, obj, 0, TO_ROOM);
        act("A quivering horde of maggots consumes $p.", true, world[obj->in_room].people, obj, 0, TO_CHAR);
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
            assert(false);
    }

    extract_obj(obj);
}

#define BLOOD_DROP_OBJ 34 /* the vnum of the blood object */
#define BLOOD_POOL_OBJ 35 /* the vnum of the blood object */
void decay_object(ObjData *obj) {
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
            msg =
                "The gaping hole in the ground closes, leaving a blackened piece "
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
            ObjData *temp = read_object(BLOOD_DROP_OBJ, VIRTUAL);
            GET_OBJ_DECOMP(temp) = 2;
            GET_OBJ_VAL(temp, VAL_BLOOD_ROOM) = obj->in_room;
            obj_to_room(temp, obj->in_room);
            /* Silent extraction. */
            msg = nullptr;
        }
        break;
    case ITEM_FOUNTAIN:
        /* Druid's fountain. */
        if (GET_OBJ_VNUM(obj) == 75)
            msg = "$p dries up.";
        break;
    }

    if (world[obj->in_room].people) {
        act(msg, true, world[obj->in_room].people, obj, 0, TO_ROOM);
        act(msg, true, world[obj->in_room].people, obj, 0, TO_CHAR);
    }
    extract_obj(obj);
}

/* Update PCs, NPCs, and objects */
void point_update(void) {
    CharData *i, *next_char;
    ObjData *j;

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
            act("&3$n&3 pauses a moment as $e purges $s stomach contents.&0", true, i, 0, 0, TO_ROOM);
            act("&3You feel VERY ill and purge the contents of your stomach.&0", false, i, 0, 0, TO_CHAR);
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
             * rapidly.\n", i); */
            else {
                /* send_to_char("You are feeling weak from hunger.\n", i); */
                gain_condition(i, THIRST, -HOURLY_THIRST_CHANGE);
            }
        } else if (IS_THIRSTY(i)) {
            /* send_to_char("You feel dizzy from extreme thirst.\n", i); */
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
                mudlog("POINT_UPDATE OBJ ERROR: Object in NOWHERE, extracting", NRM, LVL_GOD, true);
                sprintf(buf, "Object %d, \"%s\"", GET_OBJ_VNUM(j), j->name);
                mudlog(buf, NRM, LVL_GOD, true);
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
void start_decomposing(ObjData *obj) {
    ObjData *o;
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

void stop_decomposing(ObjData *obj) {
    ObjData *o;

    REMOVE_FLAG(GET_OBJ_FLAGS(obj), ITEM_DECOMP);
    for (o = obj->contains; o; o = o->next_content)
        stop_decomposing(o);
}
