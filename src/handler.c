/***************************************************************************
 * $Id: handler.c,v 1.163 2010/06/05 04:43:57 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: handler.c                                      Part of FieryMUD *
 *  Usage: internal funcs: moving chars/objs and handling effects          *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "handler.h"

#include "ai.h"
#include "casting.h"
#include "chars.h"
#include "charsize.h"
#include "clan.h"
#include "class.h"
#include "comm.h"
#include "composition.h"
#include "conf.h"
#include "constants.h"
#include "corpse_save.h"
#include "db.h"
#include "dg_scripts.h"
#include "events.h"
#include "fight.h"
#include "interpreter.h"
#include "limits.h"
#include "math.h"
#include "movement.h"
#include "pfiles.h"
#include "players.h"
#include "quest.h"
#include "races.h"
#include "regen.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "trophy.h"
#include "utils.h"

/* external vars */
extern char *MENU;

/* external functions */
void free_char(struct char_data *ch);
void remove_follower(struct char_data *ch);
void abort_casting(struct char_data *ch);

static int apply_ac(struct char_data *ch, int eq_pos);

char *fname(const char *namelist) {
    static char holder[30];
    register char *point;

    for (point = holder; isalpha(*namelist); namelist++, point++)
        *point = *namelist;

    *point = '\0';

    return (holder);
}

int isname(const char *str, const char *namelist) {
    register const char *curname, *curstr;

    curname = namelist;
    for (;;) {
        for (curstr = str;; curstr++, curname++) {
            if (!*curstr && !isalpha(*curname))
                return (1);

            if (!*curname)
                return (0);

            if (!*curstr || *curname == ' ')
                break;

            if (LOWER(*curstr) != LOWER(*curname))
                break;
        }

        /* skip to next name */

        for (; isalpha(*curname); curname++)
            ;
        if (!*curname)
            return (0);
        curname++; /* first char of new name */
    }
}

void effect_modify(struct char_data *ch, byte loc, sh_int mod, flagvector bitv[], bool add) {
    int i;

    if (add) {
        /* Special behaviors for aff flags. */
        if (IS_FLAGGED(bitv, EFF_LIGHT) && ch->in_room != NOWHERE && !EFF_FLAGGED(ch, EFF_LIGHT))
            world[ch->in_room].light++;

        /* Add effect flags. */
        for (i = 0; i < NUM_EFF_FLAGS; ++i)
            if (IS_FLAGGED(bitv, i))
                SET_FLAG(EFF_FLAGS(ch), i);
    } else {
        /* Special behaviors for aff flags. */
        if (IS_FLAGGED(bitv, EFF_LIGHT) && ch->in_room != NOWHERE && EFF_FLAGGED(ch, EFF_LIGHT))
            world[ch->in_room].light--;
        if (IS_FLAGGED(bitv, EFF_CAMOUFLAGED))
            GET_HIDDENNESS(ch) = 0;

        /* Remove effect flags. */
        for (i = 0; i < NUM_EFF_FLAGS; ++i)
            if (IS_FLAGGED(bitv, i))
                REMOVE_FLAG(EFF_FLAGS(ch), i);

        /* Negative modifier for !add. */
        mod = -mod;
    }

    switch (loc) {
    case APPLY_NONE:
        break;

    case APPLY_STR:
        GET_ACTUAL_STR(ch) += mod;
        break;
    case APPLY_DEX:
        GET_ACTUAL_DEX(ch) += mod;
        break;
    case APPLY_INT:
        GET_ACTUAL_INT(ch) += mod;
        break;
    case APPLY_WIS:
        GET_ACTUAL_WIS(ch) += mod;
        break;
    case APPLY_CON:
        GET_ACTUAL_CON(ch) += mod;
        break;
    case APPLY_CHA:
        GET_ACTUAL_CHA(ch) += mod;
        break;

    case APPLY_CLASS:
        break;

    case APPLY_LEVEL:
        GET_LEVEL(ch) += mod;
        break;

    case APPLY_AGE:
        ch->player.time.birth -= (mod * SECS_PER_MUD_YEAR);
        break;

    case APPLY_CHAR_WEIGHT:
        GET_WEIGHT(ch) += mod;
        break;

    case APPLY_CHAR_HEIGHT:
        GET_HEIGHT(ch) += mod;
        break;

    case APPLY_MANA:
        GET_MAX_MANA(ch) += mod;
        break;

    case APPLY_HIT:
        GET_MAX_HIT(ch) += mod;
        GET_HIT(ch) += mod;
        break;

    case APPLY_MOVE:
        GET_MAX_MOVE(ch) += mod;
        GET_MOVE(ch) += mod;
        break;

    case APPLY_SIZE:
        adjust_size(ch, mod);
        break;
    case APPLY_GOLD:
        break;
    case APPLY_EXP:
        break;
    case APPLY_AC:
        /* Subtract from AC because negative AC is better */
        GET_AC(ch) -= mod;
        break;
    case APPLY_HITROLL:
        GET_HITROLL(ch) += mod;
        break;
    case APPLY_DAMROLL:
        GET_DAMROLL(ch) += mod;
        break;
    case APPLY_SAVING_PARA:
        GET_SAVE(ch, SAVING_PARA) += mod;
        break;

    case APPLY_SAVING_ROD:
        GET_SAVE(ch, SAVING_ROD) += mod;
        break;

    case APPLY_SAVING_PETRI:
        GET_SAVE(ch, SAVING_PETRI) += mod;
        break;

    case APPLY_SAVING_BREATH:
        GET_SAVE(ch, SAVING_BREATH) += mod;
        break;

    case APPLY_SAVING_SPELL:
        GET_SAVE(ch, SAVING_SPELL) += mod;
        break;
    case APPLY_MANA_REGEN:
        ch->char_specials.managain += mod;
        break;
    case APPLY_HIT_REGEN:
        ch->char_specials.hitgain += mod;
        break;
    case APPLY_PERCEPTION:
        GET_PERCEPTION(ch) += mod;
        break;
    case APPLY_HIDDENNESS:
        GET_HIDDENNESS(ch) = LIMIT(0, GET_HIDDENNESS(ch) + mod, 1000);
        break;
    case APPLY_COMPOSITION:
        if (mod >= 0)
            convert_composition(ch, mod);
        else
            convert_composition(ch, BASE_COMPOSITION(ch));
        break;
    default:
        sprintf(buf, "SYSERR:handler.c:effect_modify() Unknown apply adjust attempt for: %s", GET_NAME(ch));
        log(buf);
        break;
    }

    scale_attribs(ch); /* recalc the affected attribs */
}

/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
/* The character may not be in a room at this point.                    */
void effect_total(struct char_data *ch) {
    void start_char_falling(struct char_data * ch);

    struct effect *eff;
    int i, j, old_hp = GET_MAX_HIT(ch);

    /* Remove effects of equipment. */
    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i)) {
            if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR)
                GET_AC(ch) += apply_ac(ch, i);
            for (j = 0; j < MAX_OBJ_APPLIES; j++)
                effect_modify(ch, GET_EQ(ch, i)->applies[j].location, GET_EQ(ch, i)->applies[j].modifier,
                              GET_OBJ_EFF_FLAGS(GET_EQ(ch, i)), FALSE);
        }

    /* Remove spell effects. */
    for (eff = ch->effects; eff; eff = eff->next)
        effect_modify(ch, eff->location, eff->modifier, eff->flags, FALSE);

    /* Now that all affects should be removed...let's make sure that
       their natural stats are where they should be. This was done in
       attempt to fix equipment messing up a players stats. DCE 12-18-01 */
    ch->actual_abils = ch->natural_abils;
    GET_COMPOSITION(ch) = BASE_COMPOSITION(ch);
    if (!IS_NPC(ch)) {
        GET_DAMROLL(ch) = GET_BASE_DAMROLL(ch);
        GET_HITROLL(ch) = GET_BASE_HITROLL(ch);
        GET_AC(ch) = MAX_AC;
        GET_MAX_HIT(ch) = GET_BASE_HIT(ch);

        /* Perception bonus
         * It comes out to base 480 for a level 99 human with maxed int and wis.
         */
        GET_PERCEPTION(ch) = (GET_LEVEL(ch) * ((GET_INT(ch) + GET_WIS(ch)) / 30));
    }

    /* Alrighty, add the equipment effects back in. */
    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i)) {
            if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR)
                GET_AC(ch) -= apply_ac(ch, i);
            for (j = 0; j < MAX_OBJ_APPLIES; j++)
                effect_modify(ch, GET_EQ(ch, i)->applies[j].location, GET_EQ(ch, i)->applies[j].modifier,
                              GET_OBJ_EFF_FLAGS(GET_EQ(ch, i)), TRUE);
        }

    /* And put the spells back too. */
    for (eff = ch->effects; eff; eff = eff->next)
        effect_modify(ch, eff->location, eff->modifier, eff->flags, TRUE);

    /* Update stats */
    if (!IS_NPC(ch)) {
        scale_attribs(ch);

        /* Now that we know current dex, add static AC and then cap */
        GET_AC(ch) -= static_ac(GET_DEX(ch));
        GET_AC(ch) = LIMIT(MIN_AC, GET_AC(ch), MAX_AC);

        /* Calculate HP bonus */
        GET_MAX_HIT(ch) += con_aff(ch);

        /* Fix hp */
        alter_hit(ch, old_hp - GET_MAX_HIT(ch), TRUE);

        /* Cap perception stat */
        GET_PERCEPTION(ch) = MAX(0, MIN(GET_PERCEPTION(ch), 1000));

        /* Cap damroll/hitroll stats */
        GET_DAMROLL(ch) = LIMIT(MIN_DAMROLL, GET_DAMROLL(ch), MAX_DAMROLL);
        GET_HITROLL(ch) = LIMIT(MIN_HITROLL, GET_HITROLL(ch), MAX_HITROLL);
    } else {
        GET_DEX(ch) = LIMIT(MIN_ABILITY_VALUE, GET_DEX(ch), MAX_ABILITY_VALUE);
        GET_INT(ch) = LIMIT(MIN_ABILITY_VALUE, GET_INT(ch), MAX_ABILITY_VALUE);
        GET_WIS(ch) = LIMIT(MIN_ABILITY_VALUE, GET_WIS(ch), MAX_ABILITY_VALUE);
        GET_CON(ch) = LIMIT(MIN_ABILITY_VALUE, GET_CON(ch), MAX_ABILITY_VALUE);
        GET_STR(ch) = LIMIT(MIN_ABILITY_VALUE, GET_STR(ch), MAX_ABILITY_VALUE);
        GET_CHA(ch) = LIMIT(MIN_ABILITY_VALUE, GET_CHA(ch), MAX_ABILITY_VALUE);
    }

    check_regen_rates(ch); /* update regen rates (for age) */

    if (IN_ROOM(ch) != NOWHERE && !PLR_FLAGGED(ch, PLR_SAVING)) {
        /* Check for issues with flying and such */
        if (SECT(IN_ROOM(ch)) == SECT_AIR) {
            if (!EFF_FLAGGED(ch, EFF_FLY) || GET_POS(ch) != POS_FLYING) {
                if (GET_POS(ch) == POS_FLYING)
                    GET_POS(ch) = POS_STANDING;
                start_char_falling(ch);
            } else
                overweight_check(ch);
        } else if (GET_POS(ch) == POS_FLYING) {
            if (!EFF_FLAGGED(ch, EFF_FLY)) {
                if (!EVENT_FLAGGED(ch, EVENT_FALLTOGROUND)) {
                    SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_FALLTOGROUND);
                    event_create(EVENT_FALLTOGROUND, falltoground_event, ch, FALSE, &(ch->events), 0);
                }
            } else {
                overweight_check(ch);
            }
        }
        composition_check(ch);
        alter_pos(ch, GET_POS(ch), GET_STANCE(ch));
    }
}

/* Insert an effect in a char_data structure
   Automatically sets apropriate bits and apply's */
void effect_to_char(struct char_data *ch, struct effect *eff) {
    struct effect *effect_alloc;

    CREATE(effect_alloc, struct effect, 1);

    *effect_alloc = *eff;
    effect_alloc->next = ch->effects;
    ch->effects = effect_alloc;

    effect_modify(ch, eff->location, eff->modifier, eff->flags, TRUE);

    effect_total(ch);
}

/*
 * Remove an effect structure from a char (called when duration
 * reaches zero). Pointer *eff must never be NIL!  Frees mem and calls
 * effect_total
 */
void effect_remove(struct char_data *ch, struct effect *eff) {
    struct effect *temp;

    assert(ch->effects);

    effect_modify(ch, eff->location, eff->modifier, eff->flags, FALSE);

    REMOVE_FROM_LIST(eff, ch->effects, next);
    free(eff);
    effect_total(ch);
}

/* Call effect_remove with every spell of spelltype "skill" */
void effect_from_char(struct char_data *ch, int type) {
    struct effect *hjp, *next;

    for (hjp = ch->effects; hjp; hjp = next) {
        next = hjp->next;
        if (hjp->type == type)
            effect_remove(ch, hjp);
    }
}

/* Call active_effect_remove with every spell of spelltype "skill"
 *
 * This function allows you to remove spells/skills and send the
 * wearoff message. */
void active_effect_from_char(struct char_data *ch, int type) {
    struct effect *hjp, *next;

    for (hjp = ch->effects; hjp; hjp = next) {
        next = hjp->next;
        if (hjp->type == type)
            active_effect_remove(ch, hjp);
    }
}

void lose_levitation_messages(struct char_data *ch) {
    if (GET_POS(ch) >= POS_STANDING) {
        cprintf(ch, "%s\r\n", skills[SPELL_LEVITATE].wearoff);
        act("$n floats back to the ground.", TRUE, ch, 0, 0, TO_ROOM);
    } else {
        send_to_char("Your weight feels normal again.\r\n", ch);
    }
}

void active_effect_remove(struct char_data *ch, struct effect *effect) {
    if (skills[effect->type].wearoff != NULL) {
        /* Whether to send a message about the spell going away: */

        /* Naturally, there has to be a message to send. */
        if (*skills[effect->type].wearoff &&
            /* But if the next effect is caused by the same spell, don't
             * send a message.  This way, we'll only send a dispel message
             * once per spell, even for spells that set multiple effects. */
            !(effect->next && effect->next->type == effect->type)) {

            if (GET_STANCE(ch) > STANCE_STUNNED && skills[effect->type].wearoff) {
                switch (effect->type) {
                case SPELL_LEVITATE:
                    lose_levitation_messages(ch);
                    break;
                default:
                    cprintf(ch, "%s\r\n", skills[effect->type].wearoff);
                }
            }
        }
    } else {
        sprintf(buf,
                "SYSERR: handler.c active_effect_remove() no wear-off message; "
                "effect->type = %d on %s.",
                effect->type, GET_NAME(ch));
        log(buf);
    }

    effect_remove(ch, effect);
}

/*
 * Return if a char is affected by a spell (SPELL_XXX), NULL indicates
 * not affected
 */
bool affected_by_spell(struct char_data *ch, int type) {
    struct effect *hjp;
    if (ch->effects == NULL)
        return FALSE;
    for (hjp = ch->effects; hjp; hjp = hjp->next)
        if (hjp->type == type)
            return TRUE;

    return FALSE;
}

void effect_join(struct char_data *ch, struct effect *eff, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod,
                 bool refresh) {
    struct effect *hjp;
    int i;

    for (hjp = ch->effects; hjp; hjp = hjp->next)
        if (hjp->type == eff->type && hjp->location == eff->location) {
            if (add_dur)
                eff->duration += hjp->duration;
            if (avg_dur)
                eff->duration /= 2;
            if (!refresh)
                eff->duration = hjp->duration;

            if (add_mod)
                eff->modifier += hjp->modifier;
            if (avg_mod)
                eff->modifier /= 2;

            /* copy effect flags from hjp to eff */
            for (i = 0; i < NUM_EFF_FLAGS; ++i)
                if (IS_FLAGGED(hjp->flags, i))
                    SET_FLAG(eff->flags, i);

            effect_remove(ch, hjp);
            effect_to_char(ch, eff);
            return;
        }

    /* A matching affect was not found above. */
    effect_to_char(ch, eff);
}

/* init_char()
 *
 * Initialize a character according to class and race.
 * For new characters only! */

void init_char(struct char_data *ch) {
    init_char_class(ch); /* class.c */
    init_char_race(ch);  /* races.c */
    update_char(ch);
}

/* update_char()
 *
 * Set up a character according to class and race.  Makes changes
 * that are ok for new or existing characters, unlike init_char().
 * Therefore, it's appropriate for when you change a character's
 * race, or they subclass or gain a level. */

void update_char(struct char_data *ch) {
    update_skills(ch);     /* spells.c */
    update_char_class(ch); /* class.c */
    update_char_race(ch);  /* races.c */
}

/* How many lights on this guy? */
int char_lightlevel(struct char_data *ch) {
    int j, llevel = 0;
    obj_data *obj, *next_obj;

    for (j = 0; j < NUM_WEARS; j++)
        if (GET_EQ(ch, j) != NULL && GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_LIGHT &&
            GET_OBJ_VAL(GET_EQ(ch, j), VAL_LIGHT_LIT))
            llevel++;

    for (obj = ch->carrying; obj; obj = next_obj) {
        next_obj = obj->next_content;
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, VAL_LIGHT_LIT))
            llevel++;
    }

    if (EFF_FLAGGED(ch, EFF_LIGHT))
        ++llevel;

    return llevel;
}

/* move a player out of a room */
void char_from_room(struct char_data *ch) {
    struct char_data *temp;

    if (ch == NULL || ch->in_room == NOWHERE) {
        log("SYSERR: NULL or NOWHERE in handler.c, char_from_room");
        /*exit(1); */
    }

    if (FIGHTING(ch) != NULL)
        stop_fighting(ch);
    stop_attackers(ch);

    world[ch->in_room].light -= char_lightlevel(ch);

    REMOVE_FROM_LIST(ch, world[ch->in_room].people, next_in_room);
    ch->in_room = NOWHERE;
    ch->next_in_room = NULL;
}

/* place a character in a room */
void char_to_room(struct char_data *ch, int room) {
    EVENTFUNC(autodouse_event);
    void start_char_falling(struct char_data * ch);
    char ctrbuf[100];

    if (!ch) {
        log("SYSERR:handler.c:char_to_room() NULL char pointer");
    } else if (room < 0 || room > top_of_world) {
        sprintf(ctrbuf, "SYSERR: char_to_room: name)%s room)%d", GET_NAME(ch), room);
        log(ctrbuf);
    } else {
        ch->next_in_room = world[room].people;
        world[room].people = ch;
        ch->in_room = room;

        world[room].light += char_lightlevel(ch);

        /* Kill flames immediately upon entering a water room */
        if (EFF_FLAGGED(ch, EFF_ON_FIRE) && IS_WATER(IN_ROOM(ch))) {
            event_create(EVENT_AUTODOUSE, autodouse_event, ch, FALSE, &(ch->events), 0);
        }

        if (SECT(room) == SECT_AIR)
            falling_check(ch);

        /* Quick aggro for players */
        else if (!ALONE(ch) && !IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
            struct char_data *tch = find_aggr_target(ch);
            if (tch && number(0, 5))
                event_create(EVENT_QUICK_AGGRO, quick_aggro_event, mkgenericevent(ch, tch, 0), TRUE, &(ch->events), 0);
        }
    }
}

/* give an object to a char   */
void obj_to_char(struct obj_data *object, struct char_data *ch) {
    if (object && ch) {
        object->next_content = ch->carrying;
        ch->carrying = object;
        object->carried_by = ch;
        if (GET_OBJ_TYPE(object) == ITEM_LIGHT && GET_OBJ_VAL(object, VAL_LIGHT_LIT))
            world[ch->in_room].light++;
        object->in_room = NOWHERE;
        IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
        IS_CARRYING_N(ch)++;

        if (!IS_NPC(ch))
            SET_FLAG(PLR_FLAGS(ch), PLR_AUTOSAVE);
        if (PLAYERALLY(ch))
            stop_decomposing(object);
        overweight_check(ch);
    } else
        log("SYSERR:handler.c:obj_to_char() NULL obj or char");
}

/* take an object from a char */
void obj_from_char(struct obj_data *object) {
    struct obj_data *temp;

    if (object == NULL) {
        log("SYSERR:handler.c:obj_from_char() NULL object");
        return;
    }
    if (GET_OBJ_TYPE(object) == ITEM_LIGHT && GET_OBJ_VAL(object, VAL_LIGHT_LIT))
        world[object->carried_by->in_room].light--;

    REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

    if (!IS_NPC(object->carried_by))
        SET_FLAG(PLR_FLAGS(object->carried_by), PLR_AUTOSAVE);
    if (MORTALALLY(object->carried_by))
        start_decomposing(object);

    IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(object->carried_by)--;
    object->carried_by = NULL;
    object->next_content = NULL;
}

/* Return the effect of a piece of armor in position eq_pos */
static int apply_ac(struct char_data *ch, int eq_pos) {
    int factor;

    assert(GET_EQ(ch, eq_pos));

    if (GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) != ITEM_ARMOR)
        return 0;

    switch (eq_pos) {

    case WEAR_BODY:
        factor = 1;
        break; /* 30% */
    case WEAR_HEAD:
        factor = 1;
        break; /* 20% */
    case WEAR_LEGS:
        factor = 1;
        break; /* 20% */
    default:
        factor = 1;
        break; /* all others 10% */
    }

    return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), VAL_ARMOR_AC));
}

void count_hand_eq(struct char_data *ch, int *hands_used, int *weapon_hands_used) {
    *hands_used = *weapon_hands_used = 0;
    if (GET_EQ(ch, WEAR_2HWIELD)) {
        (*hands_used) += 2;
        (*weapon_hands_used) += 2;
    }
    if (GET_EQ(ch, WEAR_HOLD))
        (*hands_used)++;
    if (GET_EQ(ch, WEAR_HOLD2))
        (*hands_used)++;
    if (GET_EQ(ch, WEAR_SHIELD))
        (*hands_used)++;
    if (GET_EQ(ch, WEAR_WIELD)) {
        (*hands_used)++;
        (*weapon_hands_used)++;
    }
    if (GET_EQ(ch, WEAR_WIELD2)) {
        (*hands_used)++;
        (*weapon_hands_used)++;
    }
}

bool may_wear_eq(struct char_data *ch, /* Who is trying to wear something */
                 struct obj_data *obj, /* The object being put on */
                 int *where,           /* The position it should be worn at */
                 bool sendmessage      /* Whether to send a rejection message to ch,
                                          if the item could not be worn. */
) {
    int a = 0, w = 0;
    /*
     * ITEM_WEAR_TAKE is used for objects that do not require special bits
     * to be put into that position (e.g. you can hold any object, not just
     * an object with a HOLD bit.)
     */
    const int wear_bitvectors[] = {
        ITEM_WEAR_TAKE,  ITEM_WEAR_FINGER, ITEM_WEAR_FINGER,  ITEM_WEAR_NECK,  ITEM_WEAR_NECK,  ITEM_WEAR_BODY,
        ITEM_WEAR_HEAD,  ITEM_WEAR_LEGS,   ITEM_WEAR_FEET,    ITEM_WEAR_HANDS, ITEM_WEAR_ARMS,  ITEM_WEAR_SHIELD,
        ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST,  ITEM_WEAR_WRIST,   ITEM_WEAR_WRIST, ITEM_WEAR_WIELD, ITEM_WEAR_WIELD,
        ITEM_WEAR_HOLD,  ITEM_WEAR_HOLD,   ITEM_WEAR_2HWIELD, ITEM_WEAR_EYES,  ITEM_WEAR_FACE,  ITEM_WEAR_EAR,
        ITEM_WEAR_EAR,   ITEM_WEAR_BADGE,  ITEM_WEAR_OBELT};

    char *already_wearing[] = {"You're already using a light.\r\n",
                               "YOU SHOULD NEVER SEE THIS MESSAGE.   PLEASE REPORT.\r\n",
                               "You're already wearing something on both of your ring fingers.\r\n",
                               "YOU SHOULD NEVER SEE THIS MESSAGE.   PLEASE REPORT.\r\n",
                               "You can't wear anything else around your neck.\r\n",
                               "You're already wearing something on your body.\r\n",
                               "You're already wearing something on your head.\r\n",
                               "You're already wearing something on your legs.\r\n",
                               "You're already wearing something on your feet.\r\n",
                               "You're already wearing something on your hands.\r\n",
                               "You're already wearing something on your arms.\r\n",
                               "You're already using a shield.\r\n",
                               "You're already wearing something about your body.\r\n",
                               "You already have something around your waist.\r\n",
                               "YOU SHOULD NEVER SEE THIS MESSAGE.   PLEASE REPORT.\r\n",
                               "You're already wearing something around both of your wrists.\r\n",
                               "You're already wielding a weapon.\r\n",
                               "You're already wielding a weapon there!\r\n",
                               "You're already holding something.\r\n",
                               "You're already holding something.\r\n",
                               "You're already wielding a weapon.\r\n",
                               "You're already wearing something on your eyes.\r\n",
                               "You're already wearing something on your face.\r\n",
                               "YOU SHOULD NEVER SEE THIS REPORT IT!.\r\n",
                               "You're already wearing something in both of your ears.\r\n",
                               "You're already wearing a badge.\r\n",
                               "You can't attach any more to your belt.\r\n"};

    /* first, make sure that the wear position is valid. */
    /* Only allow light items in the light pos, and then only
       when they cannot be worn anywhere else. */
    if (!CAN_WEAR(obj, wear_bitvectors[*where]) || (*where == 0 && GET_OBJ_TYPE(obj) != ITEM_LIGHT) ||
        (*where == 0 && GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_WEAR(obj) != ITEM_WEAR_TAKE + ITEM_WEAR_HOLD)) {
        if (!(IS_NPC(ch)) && sendmessage)
            act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
        return FALSE;
    }

    /* for neck, finger, wrist, held, and wielded, try pos 2 if pos 1 is already
     * full */

    if ((*where == WEAR_FINGER_R) || (*where == WEAR_NECK_1) || (*where == WEAR_WRIST_R) || (*where == WEAR_HOLD) ||
        (*where == WEAR_LEAR) || (*where == WEAR_WIELD))
        if (GET_EQ(ch, *where))
            (*where)++;

    /* for shield, weapon, or held, must make sure the right number of hands are
     * free */

    if ((*where == WEAR_SHIELD) || (*where == WEAR_WIELD) || (*where == WEAR_WIELD2) || (*where == WEAR_HOLD) ||
        (*where == WEAR_HOLD2) || (*where == WEAR_2HWIELD)) {

        /* a = hands occupied; w = hands wielding a weapon */
        count_hand_eq(ch, &a, &w);

        if ((GET_OBJ_WEAR(obj) & ITEM_WEAR_2HWIELD) && a) {
            if (sendmessage)
                send_to_char("You need both hands free for this weapon!\r\n", ch);
            return FALSE;
        } else if (a > 1) {
            if (sendmessage)
                send_to_char("Both of your hands are already using something.\r\n", ch);
            return FALSE;
        } else if (a == 1 && w > 0 && (*where == WEAR_WIELD || *where == WEAR_WIELD2) &&
                   !(GET_SKILL(ch, SKILL_DUAL_WIELD))) {
            if (sendmessage)
                send_to_char("You don't have the co-ordination to dual wield.\r\n", ch);
            return FALSE;
        }
    }

    /* If something is in that position, you can't wear something else there. */
    if (GET_EQ(ch, *where)) {
        if (sendmessage)
            send_to_char(already_wearing[*where], ch);
        return FALSE;
    }

    /* The NO_EQ_RESTRICT flag overrides the remaining considerations. */
    if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NO_EQ_RESTRICT))
        return TRUE;

    /* Check the level of the object. */
    if (GET_OBJ_LEVEL(obj) > GET_LEVEL(ch)) {
        if (!IS_NPC(ch) && sendmessage) {
            act("You're not a high enough level to use $p.", FALSE, ch, obj, 0, TO_CHAR);
            act("$n struggles and fails to use $p.", TRUE, ch, obj, 0, TO_ROOM);
        }
        return FALSE;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        /* Check alignment restrictions. */
        if ((OBJ_FLAGGED(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) || (OBJ_FLAGGED(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
            (OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)) || NOWEAR_CLASS(ch, obj)) {
            if (sendmessage)
                act("You can not use $p.", FALSE, ch, obj, 0, TO_CHAR);
            return FALSE;
        }

        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && GET_OBJ_WEIGHT(obj) > str_app[GET_STR(ch)].wield_w) {
            if (sendmessage)
                send_to_char("It's too heavy for you to use.\r\n", ch);
            return FALSE;
        }
    }

    /* You can only wear something on your belt if you are wearing a belt. */
    if (*where == WEAR_OBELT && !GET_EQ(ch, WEAR_WAIST)) {
        if (sendmessage)
            act("You'll need to wear a belt first.", FALSE, ch, obj, 0, TO_CHAR);
        return FALSE;
    }

    return TRUE;
}

enum equip_result equip_char(struct char_data *ch, struct obj_data *obj, int pos) {
    int j;

    assert(pos >= 0 && pos < NUM_WEARS);

    if (GET_EQ(ch, pos)) {
        mprintf(L_ERROR, LVL_GOD, "SYSERR: Char is already equipped: %s, %s", GET_NAME(ch), obj->short_description);
        return EQUIP_RESULT_ERROR;
    }
    if (obj->carried_by) {
        mprintf(L_ERROR, LVL_GOD, "SYSERR: EQUIP: Obj is carried_by when equip.");
        return EQUIP_RESULT_ERROR;
    }
    if (obj->in_room != NOWHERE) {
        mprintf(L_ERROR, LVL_GOD, "SYSERR: EQUIP: Obj is in_room when equip.");
        return EQUIP_RESULT_ERROR;
    }

    GET_EQ(ch, pos) = obj;
    obj->worn_by = ch;
    obj->worn_on = pos;

    if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
        GET_AC(ch) -= apply_ac(ch, pos);

    if (ch->in_room != NOWHERE) {
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, VAL_LIGHT_LIT))
            world[ch->in_room].light++;
    } else
        mprintf(L_ERROR, LVL_GOD, "SYSERR: ch->in_room = NOWHERE when equipping char.");

    for (j = 0; j < MAX_OBJ_APPLIES; j++)
        effect_modify(ch, obj->applies[j].location, obj->applies[j].modifier, GET_OBJ_EFF_FLAGS(obj), TRUE);

    if (PLAYERALLY(ch))
        stop_decomposing(obj);
    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);
    effect_total(ch);
    return EQUIP_RESULT_SUCCESS;
}

struct obj_data *unequip_char(struct char_data *ch, int pos) {
    int j;
    struct obj_data *obj;

    assert(pos >= 0 && pos < NUM_WEARS);
    assert(GET_EQ(ch, pos));

    obj = GET_EQ(ch, pos);
    obj->worn_by = NULL;
    obj->worn_on = -1;

    if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
        GET_AC(ch) += apply_ac(ch, pos);

    if (ch->in_room != NOWHERE) {
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, VAL_LIGHT_LIT))
            world[ch->in_room].light--;
    } else
        log("SYSERR: ch->in_room = NOWHERE when unequipping char.");

    GET_EQ(ch, pos) = NULL;
    IS_CARRYING_W(ch) -= GET_OBJ_WEIGHT(obj);

    /* Reapply all the racial effects in case they were removed above. */
    update_char(ch);

    for (j = 0; j < MAX_OBJ_APPLIES; j++)
        effect_modify(ch, obj->applies[j].location, obj->applies[j].modifier, obj->obj_flags.effect_flags, FALSE);

    if (MORTALALLY(ch))
        start_decomposing(obj);
    effect_total(ch);
    return (obj);
}

EVENTFUNC(sink_and_lose_event) {
    struct sink_and_lose *data = (struct sink_and_lose *)event_obj;
    struct obj_data *obj = data->obj;
    int room = data->room;
    char *conjugation;

    if (obj->in_room == room) {
        if (isplural(GET_OBJ_NAME(obj)))
            conjugation = "";
        else
            conjugation = "s";
        act("$p&4&b sink$T like a rock.&0", FALSE, 0, obj, conjugation, TO_ROOM);
        extract_obj(obj);
    }

    return EVENT_FINISHED;
}

/* put an object in a room */
void obj_to_room(struct obj_data *object, int room) {
    void start_obj_falling(struct obj_data * obj);
    char otrbuf[50];
    struct sink_and_lose *sinkdata;

    if (!object) {
        log("SYSERR: NULL object pointer passed to obj_to_room");
    } else if (room < 0 || room > top_of_world) {
        sprintf(otrbuf, "SYSERR: obj_to_room: obj)%d room)%d", GET_OBJ_VNUM(object), room);
        log(otrbuf);
    } else {
        object->next_content = world[room].contents;
        world[room].contents = object;
        object->in_room = room;
        object->carried_by = NULL;
        if (GET_OBJ_TYPE(object) == ITEM_LIGHT && GET_OBJ_VAL(object, VAL_LIGHT_LIT))
            world[object->in_room].light++;

        /* Falling or sinking - the !FALL flag prevents both */
        if (!OBJ_FLAGGED(object, ITEM_NOFALL)) {
            /* Will the object sink in water? */
            if ((SECT(room) == SECT_SHALLOWS || SECT(room) == SECT_WATER) && (!OBJ_FLAGGED(object, ITEM_FLOAT))) {
                /* Yep, say goodbye. */
                CREATE(sinkdata, struct sink_and_lose, 1);
                sinkdata->room = room;
                sinkdata->obj = object;
                event_create(EVENT_SINK_AND_LOSE, &sink_and_lose_event, sinkdata, TRUE, &(object->events), 2);
                return;
            } else if (SECT(room) == SECT_AIR)
                start_obj_falling(object);

            if (ROOM_FLAGGED(room, ROOM_HOUSE))
                SET_FLAG(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);
            /* if this is a player corpse, save the new room vnum to
               the corpse control record. This can later be optimized as
               a flag, much like houses  -  nechtrous */
            if (IS_PLR_CORPSE(object))
                update_corpse(object);
        }
    }
}

/* Take an object from a room */
void obj_from_room(struct obj_data *object) {
    struct obj_data *temp;

    if (!object || object->in_room == NOWHERE) {
        log("SYSERR:handler.c:obj_from_room() NULL obj or obj->in_room = -1");
        return;
    }

    if (GET_OBJ_TYPE(object) == ITEM_LIGHT && GET_OBJ_VAL(object, VAL_LIGHT_LIT))
        world[object->in_room].light--;

    REMOVE_FROM_LIST(object, world[object->in_room].contents, next_content);

    if (ROOM_FLAGGED(object->in_room, ROOM_HOUSE))
        SET_FLAG(ROOM_FLAGS(object->in_room), ROOM_HOUSE_CRASH);
    object->in_room = NOWHERE;
    object->next_content = NULL;
}

/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to) {
    struct obj_data *tmp_obj;
    int weight_reduction;
    float reduction;

    if (!obj || !obj_to || obj == obj_to) {
        log("SYSERR: NULL object or same source and target obj passed to "
            "obj_to_obj");
        return;
    }

    obj->next_content = obj_to->contains;
    obj_to->contains = obj;
    obj->in_obj = obj_to;

    weight_reduction = GET_OBJ_VAL(obj_to, VAL_CONTAINER_WEIGHT_REDUCTION);
    reduction = 0.0f;
    if (weight_reduction > 0) {
        reduction = GET_OBJ_WEIGHT(obj) * (weight_reduction / 100.0);
    }

    for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj) {
        GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj) - reduction;
    }

    /* top level object.  Subtract weight from inventory if necessary. */
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj) - reduction;

    if (tmp_obj->carried_by) {
        IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj) - reduction;
        if (PLAYERALLY(tmp_obj->carried_by))
            stop_decomposing(obj);
    } else if (tmp_obj->worn_by) {
        IS_CARRYING_W(tmp_obj->worn_by) += GET_OBJ_WEIGHT(obj) - reduction;
        if (PLAYERALLY(tmp_obj->worn_by))
            stop_decomposing(obj);
    }
}

/* remove an object from an object */
void obj_from_obj(struct obj_data *obj) {
    struct obj_data *temp, *obj_from;
    extern int short_pc_corpse_time;
    int weight_reduction;
    float reduction;

    if (obj->in_obj == NULL) {
        log("error (handler.c): trying to illegally extract obj from obj");
        return;
    }

    obj_from = obj->in_obj;
    REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

    weight_reduction = GET_OBJ_VAL(obj_from, VAL_CONTAINER_WEIGHT_REDUCTION);
    reduction = 0.0f;
    if (weight_reduction > 0) {
        reduction = GET_OBJ_WEIGHT(obj) * (weight_reduction / 100.0);
    }

    /* Subtract weight from containers container */
    for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
        GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj) - reduction;

    /* Subtract weight from char that carries the object */
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj) - reduction;
    if (temp->carried_by)
        IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj) - reduction;
    else if (temp->worn_by)
        IS_CARRYING_W(temp->worn_by) -= GET_OBJ_WEIGHT(obj) - reduction;

    obj->in_obj = NULL;
    obj->next_content = NULL;

    /* to fix some eq duping, if obj_from is a pcorpse, save the corpse */
    if (IS_PLR_CORPSE(obj_from)) {
        save_corpse(obj_from);
        /* When you remove all objects from a PC corpse, its decomp time shortens */
        if (!obj_from->contains && GET_OBJ_DECOMP(obj_from) > short_pc_corpse_time)
            GET_OBJ_DECOMP(obj_from) = short_pc_corpse_time;
    }
}

/* Global object iterator
 *
 * Helps the object extractor in limits.c to iterate over all objects
 * in the world while destroying some of them. */

struct obj_data *go_iterator = NULL;

/* Extract an object from the world */
void extract_obj(struct obj_data *obj) {
    struct obj_data *temp;

    if (obj->casters)
        obj_forget_casters(obj);
    /* rewriting to use IS_PLR_CORPSE macro - 321 */
    if (IS_PLR_CORPSE(obj)) {
        destroy_corpse(obj);
        /* Log when a player corpse decomposes so people can't
           falsely claim the code ate their corpse. RSD 11/12/2000
         */
        if (obj->in_room != NOWHERE)
            log("CORPSE: %s has been extracted from %s [%d]", obj->short_description, world[obj->in_room].name,
                world[obj->in_room].vnum);
        else if (obj->in_obj)
            log("CORPSE: %s has been extracted from inside %s", obj->short_description, obj->in_obj->short_description);
        else if (obj->carried_by)
            log("CORPSE: %s has been extracted from %s's inventory", obj->short_description, GET_NAME(obj->carried_by));
        else
            log("CORPSE: %s has been extracted from an unknown location", obj->short_description);
    }
    if (obj->worn_by != NULL)
        if (unequip_char(obj->worn_by, obj->worn_on) != obj)
            log("SYSERR: Inconsistent worn_by and worn_on pointers!!");

    if (obj->in_room != NOWHERE)
        obj_from_room(obj);
    else if (obj->carried_by)
        obj_from_char(obj);
    else if (obj->in_obj)
        obj_from_obj(obj);

    /* Get rid of the contents of the object, as well. */
    while (obj->contains)
        extract_obj(obj->contains);

    if (obj == go_iterator)
        go_iterator = obj->next;

    REMOVE_FROM_LIST(obj, object_list, next);

    if (GET_OBJ_RNUM(obj) != NOTHING)
        (obj_index[GET_OBJ_RNUM(obj)].number)--;

    if (obj->events)
        cancel_event_list(&(obj->events));

    free_obj(obj);
}

void purge_objs(struct char_data *ch) {
    int i;

    while (ch->carrying)
        extract_obj(ch->carrying);

    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i))
            extract_obj(GET_EQ(ch, i));
}

/* Extract a ch completely from the world, and leave his stuff behind  */
void extract_char(struct char_data *ch) {
    struct char_data *temp;
    struct descriptor_data *d;
    struct obj_data *obj;
    int i, freed = 0;
    void dismount_char(struct char_data * ch);
    void stop_guarding(struct char_data * ch);
    ACMD(do_return);
    void die_groupee(struct char_data * ch);
    void die_consentee_clean(struct char_data * ch);

    if (ch->in_room == NOWHERE) {
        sprintf(buf, "SYSERR:handler.c:extract_char: NOWHERE extracting char: %s", GET_NAME(ch));
        log(buf);
    }

    /*
     * Booting the (original) character of someone who has switched out, so
     * first we need to stuff them back into their own body.  This will set
     * ch->desc to the proper value below.
     */
    if (!IS_NPC(ch) && !ch->desc)
        for (d = descriptor_list; d; d = d->next)
            if (d->original == ch) {
                do_return(d->character, "", 0, 1);
                break;
            }

    /*
     * Now check to see if there is someone switched into this body
     * from another one, if so; put the switcher back into their own
     * body.
     */
    if (POSSESSED(ch))
        do_return(ch, NULL, 0, 0);

    die_consentee_clean(ch);

    if (ch->followers || ch->master)
        die_follower(ch);

    if (IS_GROUPED(ch))
        ungroup(ch, TRUE, FALSE);

    if (RIDING(ch) || RIDDEN_BY(ch))
        dismount_char(ch);

    if (ch->guarding)
        stop_guarding(ch);
    if (ch->guarded_by)
        stop_guarding(ch->guarded_by);

    if (ch->cornering) {
        if (ch->cornering->cornered_by == ch)
            ch->cornering->cornered_by = NULL;
        ch->cornering = NULL;
    }
    if (ch->cornered_by) {
        if (ch->cornered_by->cornering == ch)
            ch->cornered_by->cornering = NULL;
        ch->cornering = NULL;
    }
    /* Forget snooping, if applicable */
    if (ch->desc) {
        if (ch->desc->snooping) {
            ch->desc->snooping->snoop_by = NULL;
            ch->desc->snooping = NULL;
        }
        if (ch->desc->snoop_by) {
            write_to_output("Your victim is no longer among us.\r\n", ch->desc->snoop_by);
            ch->desc->snoop_by->snooping = NULL;
            ch->desc->snoop_by = NULL;
        }
    }

    /* Make any spells being cast upon me abort */
    if (ch->casters)
        char_forget_casters(ch);

    if (CASTING(ch)) {
        STOP_CASTING(ch);
    }

    /* transfer objects to room */
    while (ch->carrying) {
        obj = ch->carrying;
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
    }

    /* Remove runtime link to clan */
    if (GET_CLAN_MEMBERSHIP(ch))
        GET_CLAN_MEMBERSHIP(ch)->player = NULL;

    /* transfer equipment to room */
    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i))
            obj_to_room(unequip_char(ch, i), ch->in_room);

    char_from_room(ch);

    /* pull the char from the list */
    REMOVE_FROM_LIST(ch, character_list, next);

    /*
     * Take out events now, since the character may not be freed
     * below (in the case of players).
     */
    if (ch->events)
        cancel_event_list(&(ch->events));
    for (i = 0; i < EVENT_FLAG_FIELDS; ++i)
        ch->event_flags[i] = 0;

    if (!IS_NPC(ch)) {
        ch->in_room = NOWHERE;
        if (!PLR_FLAGGED(ch, PLR_REMOVING))
            save_player(ch);
    } else {
        /* if mobile, and wasn't raised from the dead, decrease load count */
        if (GET_MOB_RNUM(ch) > -1 && !MOB_FLAGGED(ch, MOB_ANIMATED))
            mob_index[GET_MOB_RNUM(ch)].number--;
        free_char(ch);
        freed = 1;
    }

    if (!freed && ch->desc != NULL) {
        STATE(ch->desc) = CON_MENU;
        write_to_output(MENU, ch->desc);
    } else { /* if a player gets purged from within the game */
        if (!freed)
            free_char(ch);
    }
}

/***************************************************************************
 * $Log: handler.c,v $
 * Revision 1.163  2010/06/05 04:43:57  mud
 * Replacing ocean sector type with cave.
 *
 * Revision 1.162  2009/06/09 05:40:10  myc
 * Remove runtime link to clan when character is extracted.
 *
 * Revision 1.161  2009/05/22 17:56:14  myc
 * Fix verb conjugation for plural items when they sink in the water.
 *
 * Revision 1.160  2009/03/15 07:09:24  jps
 * Add !FALL flag for objects
 *
 * Revision 1.159  2009/03/09 20:36:00  myc
 * Moved money functions from here to money.c.
 *
 * Revision 1.158  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.157  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.156  2009/03/07 22:28:27  jps
 * Add function active_effect_from_char, which is called to remove effects
 * in-game and provide feedback.
 *
 * Revision 1.155  2009/03/03 19:43:44  myc
 * Split off target-finding protocol from handler into find.c.
 *
 * Revision 1.154  2009/02/21 03:30:16  myc
 * Removed L_FILE flag--mprintf now logs to file by default;
 * assert L_NOFILE to prevent that.
 *
 * Revision 1.153  2009/01/17 01:17:33  myc
 * Fix that "cleric ac bug".  AC will now be added and deducted
 * from mobs correctly.
 *
 * Revision 1.152  2009/01/16 23:36:34  myc
 * Correct typo in error message in active_effect_remove().
 *
 * Revision 1.151  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.150  2008/09/20 09:07:48  jps
 * Remove extra flight-loss messages.
 *
 * Revision 1.149  2008/09/20 06:05:06  jps
 * Add macros POSSESSED and POSSESSOR.
 *
 * Revision 1.148  2008/09/13 18:52:12  jps
 * Including ai.h
 *
 * Revision 1.147  2008/09/08 05:18:00  jps
 * obj_to_obj needs to stop decomposing things if a player is carrying the
 *container.
 *
 * Revision 1.146  2008/09/07 07:55:17  jps
 * Make obj_to_obj and obj_from_obj adjust weight amounts when the
 * enclosing container is being worn.
 *
 * Revision 1.145  2008/09/07 07:20:28  jps
 * Using MORTALALLY for the events that might cause decomposition to start.
 * This means that immortals won't cause items to start decomposing by
 * handling them. It also means that they can stop things decomposing by
 * grabbing them and then getting rid of them.
 *
 * Revision 1.144  2008/09/07 01:30:47  jps
 * Handle losing fly. You might fall to the ground, or fall farther.
 *
 * Revision 1.143  2008/09/06 19:10:49  jps
 * Use PLAYERALLY when deciding if objects should decompose.
 * Control decomposition when wearing equipment.
 *
 * Revision 1.142  2008/09/06 05:49:37  jps
 * Make worn equipment count towards weight-carried as well.
 *
 * Revision 1.141  2008/09/04 06:47:36  jps
 * Changed sector constants to match their strings
 *
 * Revision 1.140  2008/09/04 00:23:43  myc
 * Add newlines to fly and levitate wearoff messages.
 *
 * Revision 1.139  2008/09/02 07:16:00  mud
 * Changing object TIMER uses into DECOMP where appropriate
 *
 * Revision 1.138  2008/09/02 06:51:20  jps
 * Objects removed from players start decomposing and those being added to
 * players stop decomposing.
 *
 * Revision 1.137  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.136  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.135  2008/08/31 22:16:21  myc
 * Fixed hp by bypassing alter_hit and modifying hitpoints directly.
 *
 * Revision 1.134  2008/08/31 18:38:33  myc
 * Consider HP in effect_total before removing effects.
 *
 * Revision 1.133  2008/08/31 04:34:12  myc
 * Fixing HP again.
 *
 * Revision 1.132  2008/08/31 02:20:48  myc
 * Fix faulty handling of hitpoints in effect_total.
 *
 * Revision 1.131  2008/08/31 01:42:22  myc
 * Fix ARMOR applies so they subtract instead of add.
 *
 * Revision 1.130  2008/08/31 01:19:54  jps
 * You screwed up holding items in slots with two of the same position!
 * Fix.
 *
 * Revision 1.129  2008/08/30 22:02:42  myc
 * Actions affecting corpses just go to the syslog now, not the RIP log.
 *
 * Revision 1.128  2008/08/30 20:25:38  jps
 * Moved count_hand_eq() into handler.c and mentioned it in handler.h.
 *
 * Revision 1.127  2008/08/30 20:21:07  jps
 * Moved equipment-wearability checks into function may_wear_eq() and moved
 * it to handler.c.
 *
 * Revision 1.126  2008/08/30 18:20:53  myc
 * Changed an object rnum check to compare against NOTHING constant.
 *
 * Revision 1.125  2008/08/30 01:31:51  myc
 * Changed the way stats are calculated in effect_total; ability
 * stats are saved in a raw form now, and only capped when accessed.
 * Damroll and hitroll are recalculated everytime effect_total
 * is called, using cached base values.
 *
 * Revision 1.124  2008/08/29 19:18:05  myc
 * Fixed abilities so that no information is lost; the caps occur
 * only when the viewed stats are accessed.
 *
 * Revision 1.123  2008/08/17 08:11:56  jps
 * Logging equip errors to file.
 *
 * Revision 1.122  2008/08/17 06:50:34  jps
 * equip_char will return one of several result codes, indicating success,
 * failure, or bad error. It uses mprintf for logging.
 *
 * Revision 1.121  2008/07/27 05:25:55  jps
 * extract_char will save players who are being extracted, but only if they
 * are not being explicitly removed.
 *
 * Revision 1.120  2008/07/10 20:21:46  myc
 * unequip_char was making the wrong assertion.  Fixzored!
 *
 * Revision 1.119  2008/06/07 19:06:46  myc
 * Moved all object-related constants and structures to objects.h
 *
 * Revision 1.118  2008/06/05 02:07:43  myc
 * Changed object flags to use flagvectors.  Rewrote corpse saving and
 * loading to use ascii object files.
 *
 * Revision 1.117  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.116  2008/05/18 05:39:59  jps
 * Changed room_data member number to "vnum".
 *
 * Revision 1.115  2008/05/14 05:12:26  jps
 * alter_hit doesn't take the attacker any more.
 *
 * Revision 1.114  2008/05/11 07:12:40  jps
 * Moved active_effect_remove here, which is what you call when a spell
 * goes away. Modified it to handle some spells differently: fly and levitate.
 *
 * Revision 1.113  2008/05/11 05:49:48  jps
 * Using regen.h. alter_hit() takes the killer. alter_pos is now the way
 * to change position.
 *
 * Revision 1.112  2008/04/14 05:11:40  jps
 * Renamed EFF_FLYING to EFF_FLY, since it only indicates an ability
 * to fly - not that the characer is actually flying.
 *
 * Revision 1.111  2008/04/14 02:18:57  jps
 * Removing unused function prototype.
 *
 * Revision 1.110  2008/04/13 01:41:39  jps
 * Calling composition_check() which makes sure no fluid/rigid
 * mount/rider pairs are possible.
 *
 * Revision 1.109  2008/04/05 05:05:42  myc
 * Removed SEND_TO_Q macro, so call write_to_output directly.
 *
 * Revision 1.108  2008/04/02 04:55:59  myc
 * Made the disband group message show up when extracting a char.
 *
 * Revision 1.107  2008/04/02 03:24:44  myc
 * Rewrote group code and removed major group code.
 *
 * Revision 1.106  2008/04/01 00:13:09  jps
 * Perform in_room check during effect_total.
 *
 * Revision 1.105  2008/03/30 17:30:38  jps
 * Renamed objsave.c to pfiles.c and introduced pfiles.h. Files using functions
 * from pfiles.c now include pfiles.h and depend on it in the makefile.
 *
 * Revision 1.104  2008/03/29 17:34:55  myc
 * Autodouse event memory leak.
 *
 * Revision 1.103  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.102  2008/03/24 02:28:37  jps
 * Fix autodouse event bug.
 *
 * Revision 1.101  2008/03/23 00:27:38  jps
 * Implement the composition apply.
 *
 * Revision 1.100  2008/03/21 22:52:52  jps
 * Reset TRANSIENT objects' timers when they are placed in a room,
 * not when they are picked up.  Prior to this, TRANSIENT objects
 * would tend to dissolve in under an hour if not picked up, which
 * was really too short.
 *
 * Revision 1.99  2008/03/21 14:54:19  myc
 * Added freeing of the event list back to extract char, fixing the
 * death crash bug.
 *
 * Revision 1.98  2008/03/19 04:32:14  myc
 * Removed a bunch of frag code that was simply commented out before.
 *
 * Revision 1.97  2008/03/16 00:21:53  jps
 * Updated trophy code.
 *
 * Revision 1.96  2008/03/11 02:53:41  jps
 * Use adjust_size instead of direct modification.
 *
 * Revision 1.95  2008/03/10 18:01:17  myc
 * Quick aggro won't go off if you are flagged no hassle.
 *
 * Revision 1.94  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.93  2008/03/08 23:20:06  myc
 * Free events in free_char instead of in extract_char.
 *
 * Revision 1.92  2008/03/05 05:21:56  myc
 * Removed frags.
 *
 * Revision 1.91  2008/03/05 03:03:54  myc
 * Moved the trophy_update function to players.c and updated
 * perform_trophy_decrease for the new trophy structures.  Reversed
 * the order of scale_attribs and update_stats calls in affect_total
 * so it isn't stupid.
 *
 * Revision 1.90  2008/02/16 20:31:32  myc
 * Moving script extraction from extract_char/extract_obj to
 * free_char/free_obj.  Moving clear_quest list to free_char.
 * Moving clear_memory to free_char.
 *
 * Revision 1.89  2008/02/09 21:07:50  myc
 * Must provide a boolean to event_create saying whether to
 * free the event obj when done or not.
 *
 * Revision 1.88  2008/02/09 18:29:11  myc
 * The event code now handles freeing of event objects.
 *
 * Revision 1.87  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.86  2008/01/30 19:20:57  myc
 * Gravity is now an event, triggered whenever a character or object
 * enters a room, or when a character loses fly.
 *
 * Revision 1.85  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.84  2008/01/27 23:31:20  jps
 * Fix stop-casting for extracted chars.
 *
 * Revision 1.83  2008/01/27 13:40:58  jps
 * Removing unused function prototype scale_attribs().
 *
 * Revision 1.82  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.81  2008/01/26 07:27:27  jps
 * Stop a char from casting any spells as it's extracted.
 *
 * Revision 1.80  2008/01/17 04:10:07  myc
 * Updating get_obj_by_obj() and get_char_room_mscript() to check
 * UIDs against the calling object and char (since find_replacement
 * now returns a UID for the object/char instead of "self" now).
 *
 * Revision 1.79  2008/01/16 04:12:00  myc
 * Adding quick-aggro event.
 *
 * Revision 1.78  2008/01/14 21:28:54  myc
 * Disabled player "quick-aggro".
 *
 * Revision 1.77  2008/01/12 23:13:20  myc
 * Added multi-purpose get_random_char_around function to pick random chars
 * in a room.  Renamed clearMemory clear_memory.
 *
 * Revision 1.76  2008/01/12 19:08:14  myc
 * Rewrote a lot of mob AI functionality.
 * Renamed 'picktarget' as 'pick_target'.
 *
 * Revision 1.75  2008/01/10 05:39:43  myc
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.74  2008/01/09 01:51:42  jps
 * Get rid of obsolete points events.
 *
 * Revision 1.73  2008/01/06 05:32:56  jps
 * Use macro NOWEAR_CLASS for equipment restrictions.
 *
 * Revision 1.72  2008/01/05 05:37:31  jps
 * Added function init_char() to handle updating of class and race-
 * related skills, innates, and other stuff.
 *
 * Revision 1.71  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.70  2007/12/25 19:45:55  jps
 * Move flying check from affect_modify to affect_total so it doesn't get
 * executed as much.
 *
 * Revision 1.69  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.68  2007/12/23 19:27:26  jps
 * Use global object iterator to prevent decay events from operating on
 * already-extracted objects.
 *
 * Revision 1.67  2007/12/19 20:51:38  myc
 * save_player() no longer requires you to supply a save/load room
 * (which wasn't being used anyway).
 *
 * Revision 1.66  2007/12/09 03:31:46  jps
 * Add a position check - it will force the position to be updated
 * correctly when you use the wizard command 'set', among other situations.
 *
 * Revision 1.65  2007/11/25 00:04:59  jps
 * Spell targets will keep close track of whoever's casting a spell
 * at them.  This allows spells to be safely aborted if the target
 * is removed from the game before the spell is completed.
 *
 * Revision 1.64  2007/10/26 23:49:30  myc
 * AC applies should be negative.
 *
 * Revision 1.63  2007/10/25 20:39:11  myc
 * Fixed a bug where the corpse decomposition message expected the
 * corpse to be on the ground.  Not always true.
 *
 * Revision 1.62  2007/10/11 20:14:48  myc
 * Fixed some bugs in the behavior of affect_join when doing "refresh"
 * spells.
 *
 * Revision 1.61  2007/10/04 16:20:24  myc
 * Got rid of update_char_objects, because all objects are being updated
 * by point_update in limits.c
 *
 * Revision 1.60  2007/10/02 02:52:27  myc
 * Cleaned up extract_char.
 *
 * Revision 1.59  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  Fixed a lot of affect-handling
 * code to work more smoothly (and sanely).
 *
 * Revision 1.58  2007/09/15 15:36:48  myc
 * Cleaned up affect-handling code a lot.  It didn't properly handle
 * bitvectors 1, 2, and 3.  Now it does.
 *
 * Revision 1.57  2007/09/03 22:53:54  jps
 * Make a person's flames go out automatically if they enter a water room.
 *
 * Revision 1.56  2007/09/01 21:22:29  jps
 * Made _mscript detection routines.
 *
 * Revision 1.55  2007/09/01 20:45:47  jps
 * Fix!
 *
 * Revision 1.54  2007/09/01 20:34:10  jps
 * Add function get_char_in_room, which bypasses CAN_SEE, for
 * scripting m* commands.
 *
 * Revision 1.53  2007/08/30 20:00:09  jps
 * Oops, purge_objs wasn't actually purging.
 *
 * Revision 1.52  2007/08/30 19:42:46  jps
 * Add function purge_objs() to destroy all objects on a character.
 *
 * Revision 1.51  2007/08/14 22:43:07  myc
 * Added cornering/cornered_by clean-up to extract_char.
 *
 * Revision 1.50  2007/07/25 02:57:26  myc
 * Only set crashsave bit in obj_from_char for players.
 *
 * Revision 1.49  2007/07/24 01:24:37  myc
 * AFF2_LIGHT on objects will no longer cause permadarkened rooms.
 *
 * Revision 1.48  2007/07/14 01:42:38  jps
 * Stop trying to set the PLR_CRASH bit on NPCs when they receive an object.
 *
 * Revision 1.47  2007/04/11 14:15:28  jps
 * Give money piles proper keywords and make them dissolve when stolen.
 *
 * Revision 1.46  2006/12/07 02:34:42  myc
 * Aff2 Light flag won't cause permadark anymore.
 *
 * Revision 1.45  2006/12/04 17:56:29  myc
 * Fixed a crash bug with update_char_object, containers, and recursive looping.
 *
 * Revision 1.44  2006/11/21 03:45:52  jps
 * Running down of lights is now handled in limits.c.
 *
 * Revision 1.43  2006/11/20 22:24:17  jps
 * End the difficulties in interaction between evil and good player races.
 *
 * Revision 1.42  2006/11/18 07:36:48  jps
 * Specify what light object is burning out.
 *
 * Revision 1.41  2006/11/18 04:26:32  jps
 * Renamed continual light spell to illumination, and it only works on
 * LIGHT items (still rooms too).
 *
 * Revision 1.40  2006/11/18 00:03:31  jps
 * Fix continual light items to always work when they have the
 * bit set.  Rooms now print an indicator of being continually lit.
 * Can't use it to make a room permanently lit any more.
 *
 * Revision 1.39  2006/11/17 03:47:30  jps
 * Eliminate points_event events being enqueued for outgoing players.
 *
 * Revision 1.38  2006/11/15 06:19:18  jps
 * Fix crash where a player being extracted would have more regen events
 *enqueued.
 *
 * Revision 1.37  2006/11/13 18:15:05  jps
 * Fix lights to actually actually burn out.
 *
 * Revision 1.36  2006/11/08 21:36:05  jps
 * Fix so lights anywhere in inventory (not just top slot) can burn out.
 *
 * Revision 1.35  2006/11/07 06:15:02  jps
 * 'animate dead' - raised mobs would double-decrease their load
 * count, having died twice.
 *
 * Revision 1.34  2006/04/27 02:32:05  dce
 * Put a cap on hitgain of 100 to prevent crashes
 * because hitgain is only defined as a sbyte
 *
 * Revision 1.33  2002/09/19 01:07:53  jjl
 * Update to add in quest variables!
 *
 * Revision 1.32  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.31  2001/12/18 23:52:27  dce
 * Made a small addtion to affect_total to finish off fixing
 * the attributes and items problem. Which was items that
 * affect your atrribs mess up your attribs..
 *
 * Revision 1.30  2001/12/18 03:29:04  dce
 * Fixed the bug where if you have a natural stat of 100, and
 * then an item added to that stat. You would actually lose
 * points to the stat when you removed the item. The root
 * of the cause was in affect_modify. I added a check to see
 * if the affect was being added or subtracted. If it was being
 * subtracted then I made sure that AFFECTED_STAT - mod was
 * not less than NATURAL_STAT.
 *
 * Revision 1.29  2001/12/07 15:42:30  dce
 * Fixed a bug with object spell effects where if a player
 * was wearing an item and died, they would permanently
 * gain that ability.
 *
 * Revision 1.28  2001/03/10 18:45:33  dce
 * Changed do_return function to pass a subcommand of 1.
 * This way I can make it so players can't use the return command.
 *
 * Revision 1.27  2000/11/29 00:47:35  mtp
 * fix problem wthat makes quest list grow if player doesnt completely log out
 *
 * Revision 1.26  2000/11/25 08:07:20  rsd
 * Altered debug in corpse decay to use short description
 * and not the name to make the debug resemble english.
 *
 * Revision 1.25  2000/11/20 03:10:54  rsd
 * Altered comment header and added back rlog messages from
 * pior to the addition of the $log$ string.
 *
 * Revision 1.24  2000/11/13 02:38:21  rsd
 * Added log message for when a player corpse decomposes.
 *
 * Revision 1.23  1999/11/28 23:26:09  cso
 * obj_to_room: modified to use IS_PLR_CORPSE macro
 * obj_from_obj: same
 * extract_obj: same
 *
 * Revision 1.22  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.21  1999/03/05 20:02:36  dce
 * Chant added to, and songs craeted
 *
 * Revision 1.20  1999/03/04 20:13:51  jimmy
 * removed silly debug message
 * fingon
 *
 * Revision 1.19  1999/03/01 05:31:34  jimmy
 * Rewrote spellbooks.  Moved the spells from fingh's PSE to a standard linked
 * list.  Added Spellbook pages.  Rewrote Scribe to be a time based event based
 * on the spell mem code.  Very basic at this point.  All spells are 5 pages
 *long, and take 20 seconds to scribe each page.  This will be more dynamic when
 *the SCRIBE skill is introduced.  --Fingon.
 *
 * Revision 1.18  1999/02/26 22:30:30  dce
 * Monk additions/fixes
 *
 * Revision 1.17  1999/02/23 16:48:06  dce
 * Creates a new command called file. Allows us to view files
 * through the mud.
 *
 * Revision 1.16  1999/02/13 19:35:06  mud
 * commented out unused variable in line 1554 associated with
 *  / * Subclassing explaination/preface * / which was commented
 * earlier to hide subclasses from players at login.
 *
 * Revision 1.15  1999/02/12 21:43:38  mud
 * Ok, I finished moving the subclass issue from the class selection view
 * the todo list will have more on the work remaining on this.
 *
 * Revision 1.14  1999/02/12 21:41:28  mud
 * I removed the View of the subclasses from the class Selection screen
 * In doing so I've created a warning, I have no idea how to fix it,
 * someone take a peek at it?
 *
 * Revision 1.13  1999/02/10 22:21:42  jimmy
 * Added do_wiztitle that allows gods to edit their
 * godly title ie Overlord.  Also added this title
 * to the playerfile
 * fingon
 *
 * Revision 1.12  1999/02/07 07:29:01  mud
 * removed debug message
 *
 * Revision 1.11  1999/02/06 05:32:46  jimmy
 * Fixed buffer overflow in do_alias
 * fingon
 *
 * Revision 1.10  1999/02/06 04:29:51  dce
 * David Endre 2/5/99
 * Added do_light
 *
 * Revision 1.9  1999/02/06 00:40:36  jimmy
 * Major change to incorporate aliases into the pfile
 * moved alias structure from interpreter.h to structs.h
 * heavily modified alias code in interpreter.c
 * Jimmy Kincaid AKA fingon
 *
 * Revision 1.8  1999/02/04 16:42:34  jimmy
 * Combined attributes, score, and exp commands.
 *
 * Revision 1.7  1999/02/04 00:02:59  jimmy
 * max/min exp loss/gain set to 2 notches.
 *
 * Revision 1.6  1999/02/01 22:40:16  jimmy
 * made listspells an LVL_IMMORT command
 *
 * Revision 1.5  1999/02/01 08:15:46  jimmy
 * improved build counter
 *
 * Revision 1.4  1999/02/01 04:18:50  jimmy
 * Added buildcounter to GREETING --Fingon
 *
 * Revision 1.3  1999/01/31 06:43:09  mud
 * Indented file
 *
 * Revision 1.2  1999/01/29 04:06:46  jimmy
 * temp remove races from login menu
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial Revision
 *
 ***************************************************************************/
