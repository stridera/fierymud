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
        ITEM_WEAR_EAR,   ITEM_WEAR_BADGE,  ITEM_WEAR_OBELT,   ITEM_WEAR_HOVER};

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
                               "You can't attach any more to your belt.\r\n",
                               "You already have something hovering around you.\r\n"};

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
