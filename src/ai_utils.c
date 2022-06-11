/***************************************************************************
 *  File: ai_utils.c                                      Part of FieryMUD *
 *  Usage: General Utility functions for mob ai                            *
 *                                                                         *
 *  By: Ben Horner (Proky of HubisMUD)                                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "ai.h"
#include "casting.h"
#include "class.h"
#include "comm.h"
#include "conf.h"
#include "db.h"
#include "events.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "math.h"
#include "movement.h"
#include "races.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

bool mob_cast(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int spellnum);

int value_spell(int spellnum, bool is_aggro_good) {
    if (!is_aggro_good && SINFO.violent)
        return -SINFO.lowest_level;
    return SINFO.lowest_level;
}

int value_spell_effect(int flag) {
    switch (flag) {
    case EFF_MAJOR_PARALYSIS:
        return -50;
    case EFF_BLIND:
        return -40;
    case EFF_SILENCE:
    case EFF_INSANITY:
    case EFF_HURT_THROAT:
        return -30;
    case EFF_POISON:
    case EFF_SLEEP:
    case EFF_MINOR_PARALYSIS:
    case EFF_ON_FIRE:
    case EFF_DISEASE:
    case EFF_ANIMATED:
        return -20;
    case EFF_CHARM:
    case EFF_TAMED:
    case EFF_FEAR:
        return -10;

    case EFF_VITALITY:
        return -5;

    case EFF_CURSE:
        return 3;

    case EFF_ENLARGE:
    case EFF_REDUCE:
    case EFF_BLESS:
    case EFF_HEX:
        return 10;
    case EFF_TONGUES:
    case EFF_LEVITATE:
    case EFF_CAMOUFLAGED:
    case EFF_RAY_OF_ENFEEB:
        return 15;
    case EFF_FARSEE:
    case EFF_DETECT_ALIGN:
    case EFF_DETECT_POISON:
    case EFF_DETECT_MAGIC:
    case EFF_WATERBREATH:
    case EFF_MINOR_GLOBE:
    case EFF_SHADOWING:
        return 20;
    case EFF_WATERWALK:
    case EFF_INVISIBLE:
    case EFF_NOTRACK:
    case EFF_LIGHT:
        return 25;
    case EFF_SNEAK:
    case EFF_SENSE_LIFE:
    case EFF_INFRAVISION:
        return 35;
    case EFF_DETECT_INVIS:
        return 40;
    case EFF_PROTECT_EVIL:
    case EFF_PROTECT_GOOD:
    case EFF_FLY:
    case EFF_SOULSHIELD:
    case EFF_PROT_FIRE:
    case EFF_PROT_COLD:
    case EFF_PROT_AIR:
    case EFF_PROT_EARTH:
    case EFF_FIRESHIELD:
    case EFF_COLDSHIELD:
    case EFF_ULTRAVISION:
    case EFF_AWARE:
    case EFF_VAMP_TOUCH:
        return 50;
    case EFF_HASTE:
        return 60;
    case EFF_MAJOR_GLOBE:
    case EFF_HARNESS:
    case EFF_NEGATE_HEAT:
    case EFF_NEGATE_COLD:
    case EFF_NEGATE_AIR:
    case EFF_NEGATE_EARTH:
        return 70;
    case EFF_BLUR:
        return 80;
    case EFF_SANCTUARY:
        return 90;
    case EFF_STONE_SKIN:
        return 100;

    default:
        return 0;
    }
}

int value_spell_effects(flagvector flags[]) {
    int i, value = 0;
    for (i = 0; i < NUM_EFF_FLAGS; ++i)
        if (IS_FLAGGED(flags, i))
            value += value_spell_effect(i);
    return value;
}

int value_obj_flags(struct char_data *ch, struct obj_data *obj) {
    int value = 0, i;
    for (i = 0; i < 32; ++i)
        if (OBJ_FLAGGED(obj, i))
            switch (1 << i) {
            case ITEM_GLOW:
            case ITEM_HUM:
            case ITEM_NOSELL:
            case ITEM_WAS_DISARMED:
                break;
            case ITEM_NODROP:
                if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
                    value += 15;
                value -= 5;
                break;
            case ITEM_MAGIC:
                value += 1;
                break;
            case ITEM_NORENT:
                value -= 5;
                break;
            case ITEM_NOINVIS:
            case ITEM_INVISIBLE:
                value += 5;
                break;
            case ITEM_NOBURN:
                if (CAN_WEAR(obj, ITEM_WEAR_TAKE) && GET_OBJ_WEAR(obj) != ITEM_WEAR_TAKE)
                    value += 10;
                break;
            case ITEM_NOLOCATE:
                value += 3;
                break;
            case ITEM_FLOAT:
                if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
                    value += 10;
                value += 3;
                break;
            case ITEM_ANTI_GOOD:
                if (IS_GOOD(ch))
                    value -= 20;
                break;
            case ITEM_ANTI_NEUTRAL:
                if (IS_NEUTRAL(ch))
                    value -= 20;
                break;
            case ITEM_ANTI_EVIL:
                if (IS_EVIL(ch))
                    value -= 20;
                break;
            case ITEM_ANTI_SORCERER:
            case ITEM_ANTI_CLERIC:
            case ITEM_ANTI_ROGUE:
            case ITEM_ANTI_WARRIOR:
            case ITEM_ANTI_PALADIN:
            case ITEM_ANTI_ANTI_PALADIN:
            case ITEM_ANTI_RANGER:
            case ITEM_ANTI_DRUID:
            case ITEM_ANTI_SHAMAN:
            case ITEM_ANTI_ASSASSIN:
            case ITEM_ANTI_MERCENARY:
            case ITEM_ANTI_NECROMANCER:
            case ITEM_ANTI_CONJURER:
            case ITEM_ANTI_MONK:
            case ITEM_ANTI_BERSERKER:
                value -= 2;
                break;
            }
    if (NOWEAR_CLASS(ch, obj))
        value -= 100;
    return value;
}

int value_effect(int location, int modifier) {

    switch (location) {
    case APPLY_STR:
    case APPLY_DEX:
    case APPLY_INT:
    case APPLY_WIS:
    case APPLY_CON:
    case APPLY_CHA:
        return 3 * modifier;
        break;
    case APPLY_MANA:
    case APPLY_HIT:
    case APPLY_MOVE:
        return modifier;
    case APPLY_AC:
        return 2 * modifier;
    case APPLY_HITROLL:
    case APPLY_DAMROLL:
        return 5 * modifier;
    case APPLY_SAVING_PARA:
    case APPLY_SAVING_ROD:
    case APPLY_SAVING_PETRI:
    case APPLY_SAVING_BREATH:
    case APPLY_SAVING_SPELL:
        return -modifier;
    case APPLY_SIZE:
        return 10 * modifier;
    case APPLY_HIT_REGEN:
    case APPLY_MANA_REGEN:
        return 2 * modifier;
    case APPLY_PERCEPTION:
    case APPLY_HIDDENNESS:
        return modifier / 2;
    case APPLY_NONE:
    case APPLY_CLASS:
    case APPLY_LEVEL:
    case APPLY_AGE:
    case APPLY_CHAR_WEIGHT:
    case APPLY_CHAR_HEIGHT:
    case APPLY_GOLD:
    case APPLY_EXP:
    default:
        return 0;
    }
}

int appraise_item(struct char_data *ch, struct obj_data *obj) {
    int value = 0, i;

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_LIGHT:
        if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT)
            value = 50;
        else {
            value = GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) / 100; /* remaining light power */
            value += GET_OBJ_VAL(obj, VAL_LIGHT_CAPACITY) / 200; /* original light power */
        }
        break;
    case ITEM_WEAPON:
        value = WEAPON_AVERAGE(obj);
        break;
    case ITEM_SCROLL:
    case ITEM_POTION:
        value = GET_OBJ_VAL(obj, VAL_SCROLL_LEVEL);
        value += value_spell(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1), GET_OBJ_TYPE(obj) == ITEM_POTION);
        value += value_spell(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_2), GET_OBJ_TYPE(obj) == ITEM_POTION);
        value += value_spell(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_3), GET_OBJ_TYPE(obj) == ITEM_POTION);
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        value = value_spell(GET_OBJ_VAL(obj, VAL_WAND_SPELL), TRUE) * GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT);
        if (GET_OBJ_TYPE(obj) == ITEM_STAFF)
            value *= 3;
        value += GET_OBJ_VAL(obj, VAL_WAND_LEVEL);
        /* charges total (value 1) doesn't matter right now */
        break;
    case ITEM_ARMOR:
        value = GET_OBJ_VAL(obj, VAL_ARMOR_AC);
        break;
    case ITEM_CONTAINER:
        value = GET_OBJ_VAL(obj, VAL_CONTAINER_CAPACITY);
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        value = GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY);
        value += GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING);
        if (IS_POISONED(obj))
            value *= -1;
        break;
    case ITEM_FOOD:
        value = GET_OBJ_VAL(obj, VAL_FOOD_FILLINGNESS);
        if (IS_POISONED(obj))
            value *= -1;
        break;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_TAKE) && GET_OBJ_WEAR(obj) != ITEM_WEAR_TAKE)
        for (i = 0; i < MAX_OBJ_APPLIES; ++i)
            value += value_effect(obj->applies[i].location, obj->applies[i].modifier);

    value += GET_OBJ_COST(obj) / 100;

    value += value_spell_effects(GET_OBJ_EFF_FLAGS(obj));
    value += value_obj_flags(ch, obj);

    value += 100 - GET_OBJ_LEVEL(obj);

    return value;
}

bool has_effect(struct char_data *ch, const struct spell_pair *effect) {
    if (!effect || effect->spell <= 0)
        return FALSE;
    else if (effect->flag)
        return EFF_FLAGGED(ch, effect->flag) ? 1 : 0;
    else
        return affected_by_spell(ch, effect->spell);
}

/*
 * victim should never be null coming into this.  I can't be held
 * responsible for any side-effects if victim is null :P  Or if
 * no one is fighting the mob.
 */
struct char_data *weakest_attacker(struct char_data *ch, struct char_data *victim) {
    struct char_data *tch;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
        if (FIGHTING(tch) != ch)
            continue; /* Only switch to enemies */
        if (!IS_NPC(tch))
            continue; /* Don't switch to other mobs */
        if (victim) {
            if (GET_HIT(tch) >= GET_HIT(victim))
                continue; /* Only switch targets if their hp is lower */
            if (number(0, 60) > GET_LEVEL(ch))
                continue; /* Higher level mobs are better at switching targets */
        }
        victim = tch;
    }
    return victim;
}

bool mob_heal_up(struct char_data *ch) {
    int i, counter = 0;
    extern const int mob_cleric_heals[];

    for (i = 0; mob_cleric_heals[i]; i++) {
        if (mob_cast(ch, ch, NULL, mob_cleric_heals[i]))
            return TRUE;
        else
            counter++;
        /* Attempt up to 3 spells. */
        if (counter > 3)
            break;
    }
    return FALSE;
}

bool is_tanking(struct char_data *ch) {
    struct char_data *tch;

    if (!FIGHTING(ch))
        return FALSE;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
        if (FIGHTING(tch) == ch)
            return TRUE;
    return FALSE;
}

bool evil_in_group(struct char_data *victim) {
    struct char_data *k;
    struct group_type *f;

    if (!IS_GROUPED(victim))
        return (GET_ALIGNMENT(victim) <= -500);
    k = (victim->group_master ? victim->group_master : victim);
    if (GET_ALIGNMENT(k) <= -500)
        return TRUE;
    for (f = k->groupees; f; f = f->next)
        if (GET_ALIGNMENT(f->groupee) <= -500)
            return TRUE;
    return FALSE;
}

bool good_in_group(struct char_data *victim) {
    struct char_data *k;
    struct group_type *f;
    if (!IS_GROUPED(victim))
        return (GET_ALIGNMENT(victim) >= 500);
    k = (victim->group_master ? victim->group_master : victim);
    if (GET_ALIGNMENT(k) >= 500)
        return TRUE;
    for (f = k->groupees; f; f = f->next)
        if (GET_ALIGNMENT(f->groupee) >= 500)
            return TRUE;
    return FALSE;
}

/*
 * group_size
 *
 * Returns the number of people in character's group who are also in
 * the same room.
 */
int group_size(struct char_data *ch) {
    struct char_data *k;
    struct group_type *f;
    int counter = 0;

    if (!IS_GROUPED(ch))
        return 0;

    /* Character IS grouped.  Find group master and count groupees. */
    k = (ch->group_master ? ch->group_master : ch);
    for (f = k->groupees; f; f = f->next)
        if (f->groupee->in_room == ch->in_room)
            counter++;
    return counter + 1;
}

/* Will this mob (ch) assist vict in battle? */
bool will_assist(struct char_data *ch, struct char_data *vict) {
    /* STAGE 0: Very basic sanity checks. */

    /* I'm already in battle. */
    if (FIGHTING(ch))
        return FALSE;

    /* Vict isn't in battle, or I'm fighting vict. */
    if (ch == vict || !FIGHTING(vict) || ch == FIGHTING(vict))
        return FALSE;

    /* The person vict is fighting isn't here. */
    if (FIGHTING(vict)->in_room != ch->in_room)
        return FALSE;

    /* STAGE 1: Vict is fighting someone in here... */

    /* Vict is a player (or appears to be). */
    if (!IS_NPC(vict) || MOB_FLAGGED(vict, MOB_PLAYER_PHANTASM)) {
        /* A protector might assist a player, UNLESS:
         *   -- the player is fighting a protector mob
         *   -- the player is fighting a peacekeeper mob
         *   -- they player is fighting a player */
        if (MOB_FLAGGED(ch, MOB_PROTECTOR) &&
            !MOB_FLAGGED(FIGHTING(vict), MOB_PROTECTOR) &&   /* not fighting a protector */
            !MOB_FLAGGED(FIGHTING(vict), MOB_PEACEKEEPER) && /* not fighting a peacekeeper */
            !(!IS_NPC(FIGHTING(vict)) || MOB_FLAGGED(FIGHTING(vict), MOB_PLAYER_PHANTASM)) /* not fighting a player */
        )
            return TRUE;
        /* Otherwise, mobs don't assist players. */
        return FALSE;
    }

    /* Vict is a mobile. */

    /* What would a peacekeeper/protector do? */
    if (MOB_FLAGGED(ch, MOB_PEACEKEEPER) || MOB_FLAGGED(ch, MOB_PROTECTOR)) {
        /* It wouldn't assist against another peacekeeper, or a protector. */
        if (MOB_FLAGGED(FIGHTING(vict), MOB_PEACEKEEPER) || MOB_FLAGGED(FIGHTING(vict), MOB_PROTECTOR))
            return FALSE;
        /* It would assist a peacekeeper or protector. */
        if (MOB_FLAGGED(vict, MOB_PEACEKEEPER) || MOB_FLAGGED(vict, MOB_PROTECTOR))
            return TRUE;
        /* A peacekeeper would assist against a badly-aligned char */
        if (MOB_FLAGGED(ch, MOB_PEACEKEEPER) && abs(GET_ALIGNMENT(ch) - GET_ALIGNMENT(FIGHTING(vict))) > 1350)
            return TRUE;
    }

    /* When do mobiles assist mobiles?  When they've got HELPER flag. */
    if (MOB_FLAGGED(ch, MOB_HELPER))
        return TRUE;

    return FALSE;
}

bool is_aggr_to(struct char_data *ch, struct char_data *tch) {
    if (!ch || !tch || ch == tch || !AWAKE(ch))
        return FALSE;

    if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MESMERIZED))
        return FALSE;

    if (!CAN_SEE(ch, tch) || PRF_FLAGGED(tch, PRF_NOHASSLE) || EFF_FLAGGED(tch, EFF_FAMILIARITY))
        return FALSE;

    if (is_grouped(ch, tch))
        return FALSE;

    if (!attack_ok(ch, tch, FALSE))
        return FALSE;

    if (IS_NPC(ch)) {
        /* Wimpy mobs don't attack alert folks.
         * Note: It would be nice to separate the fleeing bit from the
         * attacking-only- unconscious-folks bit. */
        if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(tch) && !MOB_FLAGGED(ch, MOB_PROTECTOR) &&
            !MOB_FLAGGED(ch, MOB_PEACEKEEPER))
            return FALSE;

        /* Peacekeepers are out to rid the world of evil (or good!) NPCs */
        if (IS_NPC(tch) && MOB_FLAGGED(ch, MOB_PEACEKEEPER) && abs(GET_ALIGNMENT(ch) - GET_ALIGNMENT(tch)) > 1350)
            return TRUE;

        /* Protectors don't like mobs that are aggro to PCs. */
        if (MOB_FLAGGED(ch, MOB_PROTECTOR) && AGGR_TO_PLAYERS(tch))
            return TRUE;

        /* Otherwise, NPCs do not attack NPCs. */
        if (IS_NPC(tch) && !MOB_FLAGGED(tch, MOB_PLAYER_PHANTASM))
            return FALSE;

        /* Now if the mob is marked any kind of aggro that matches the
           target, attack! */
        if (MOB_FLAGGED(ch, MOB_AGGRESSIVE))
            return TRUE;
        if (MOB_FLAGGED(ch, MOB_AGGR_GOOD_RACE) && GET_RACE_ALIGN(tch) == RACE_ALIGN_GOOD)
            return TRUE;
        if (MOB_FLAGGED(ch, MOB_AGGR_EVIL_RACE) && GET_RACE_ALIGN(tch) == RACE_ALIGN_EVIL)
            return TRUE;
        if (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(tch))
            return TRUE;
        if (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(tch))
            return TRUE;
        if (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(tch))
            return TRUE;

        /* Is the target remembered as an enemy? */
        if (MOB_FLAGGED(ch, MOB_MEMORY) && in_memory(ch, tch))
            return TRUE;

    }
    /* Otherwise is a player */
    else {
        /* Berserkers are aggressive. */
        if (IS_NPC(tch) && EFF_FLAGGED(ch, EFF_BERSERK))
            return TRUE;

        if (GET_WIMP_LEV(ch) >= GET_HIT(ch))
            return FALSE;
        if (GET_AGGR_LEV(ch) <= 0 || GET_AGGR_LEV(ch) > GET_HIT(ch))
            return FALSE;

        /* If not vicious, be merciful to sleeping/paralyzed mobs */
        if (!PRF_FLAGGED(ch, PRF_VICIOUS) &&
            (!AWAKE(tch) || EFF_FLAGGED(tch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(tch, EFF_MAJOR_PARALYSIS)))
            return FALSE;

        /*
         * The target must be an NPC or else you'll end up with an infinite
         * recursion situation.   Can you say segfault?
         */
        if (IS_NPC(tch) && is_aggr_to(tch, ch))
            return TRUE;
    }

    return FALSE;
}

int appraise_opponent(struct char_data *ch, struct char_data *vict) {
    int val;

    if (!IS_NPC(ch))
        return -1;

    val = GET_HIT(vict);

    if (IS_CLERIC(vict) || IS_MAGIC_USER(vict))
        val *= 2 / 3;
    else if (IS_WARRIOR(vict))
        val *= 2;
    if (!FIGHTING(vict))
        val /= (IS_ROGUE(ch) ? 4 : 2);
    if (EFF_FLAGGED(vict, EFF_AWARE) && IS_ROGUE(ch))
        val *= 1.5;
    if (GET_LEVEL(vict) < 60)
        val /= number(40 + GET_LEVEL(vict), 160 - GET_LEVEL(vict));
    return val;
}

void glorion_distraction(struct char_data *ch, struct char_data *glorion) {
    if (ch == glorion)
        return;
    /* Will the glorion be attacked? */
    if (number(1, 100) < 3 && !PRF_FLAGGED(glorion, PRF_NOHASSLE)) {
        /* It's attacked! */
        act("$n forgets $s appreciation of $N's glorious appearance, and attacks!", TRUE, ch, 0, glorion, TO_NOTVICT);
        act("The look of awe in $N's eyes falters, and $e attacks!", TRUE, glorion, 0, ch, TO_CHAR);
        act("You see right through $N's magical disguise!", FALSE, ch, 0, glorion, TO_CHAR);
        event_create(EVENT_QUICK_AGGRO, quick_aggro_event, mkgenericevent(ch, glorion, 0), TRUE, &(ch->events), 0);
    } else {
        /* Glory wins: no attack. */
        if (number(1, 8) == 1) {
            act("$n looks upon $N with awe in $s eyes.", TRUE, ch, 0, glorion, TO_NOTVICT);
            act("$n gazes at you in wonder.", TRUE, ch, 0, glorion, TO_VICT);
            act("You are distracted by $N's unearthly beauty.", TRUE, ch, 0, glorion, TO_CHAR);
        }
    }
}

#define MAX_TARGETS 10
struct char_data *find_aggr_target(struct char_data *ch) {
    struct char_data *tch;
    struct aggr_target {
        struct char_data *target;
        int difficulty;
    } targets[MAX_TARGETS + 1];
    int i, j, k, num_targets, chosen_targets;

    struct char_data *glorion = NULL;
    int glorion_count;

    if (!ch || CH_NROOM(ch) == NOWHERE)
        return NULL;

    if (ROOM_FLAGGED(CH_NROOM(ch), ROOM_PEACEFUL))
        return NULL;

    if (!IS_NPC(ch) && !EFF_FLAGGED(ch, EFF_BERSERK)) {
        if (GET_WIMP_LEV(ch) >= GET_HIT(ch))
            return NULL;
        if (GET_AGGR_LEV(ch) <= 0 || GET_AGGR_LEV(ch) > GET_HIT(ch))
            return NULL;
    }

    if (PLR_FLAGGED(ch, PLR_BOUND) || !AWAKE(ch) || EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) ||
        EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS))
        return NULL;

    /* Your intelligence determines how many targets you will evaluate. */
    num_targets = MAX(1, GET_INT(ch) * MAX_TARGETS / 100);

    /* Choose #num_targets characters at random */
    glorion_count = 0;
    for (i = 0, tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
        /* If there are any folks in the room with GLORY, they will
         * interfere with aggression.  One will be chosen at random,
         * because there's a chance the mob will attack it.  But no
         * one else will be. */
        if (ch != tch && CAN_SEE(ch, tch)) {
            if (EFF_FLAGGED(tch, EFF_GLORY) && PLAYERALLY(tch)) {
                glorion_count++;
                if (number(1, glorion_count) == 1)
                    glorion = tch;
            } else if (is_aggr_to(ch, tch)) {
                if (i >= num_targets)
                    j = number(0, i);
                else
                    j = i;
                if (j < num_targets) {
                    targets[j].target = tch;
                    targets[j].difficulty = appraise_opponent(ch, tch);
                }
                i++;
            }
        }
    }

    if (glorion_count) {
        glorion_distraction(ch, glorion);
        return NULL;
    }

    if (i == 0)
        /* Didn't find anyone I was aggro to. */
        return NULL;

    /* This is the number of folks I decided I was aggro to, and stored in
     * targets[]. */
    if (i >= num_targets)
        chosen_targets = num_targets;
    else
        chosen_targets = i;

    /* Now choose one of them to attack. */

    /* Players and illusions just choose at random. */
    if (!IS_NPC(ch) || MOB_FLAGGED(ch, MOB_PLAYER_PHANTASM))
        return targets[number(0, chosen_targets - 1)].target;

    /* Normal mobiles choose the weakest enemy. */

    /* The extra struct-aggr_target is used to initialize the search for
     * the easiest opponent. */
    k = chosen_targets;
    targets[k].difficulty = 100000000; /* Arbitrarily large number */
    for (j = 0; j < chosen_targets; ++j)
        if (targets[j].difficulty < targets[k].difficulty)
            k = j;

    /* No potential targets in the room? */
    if (k >= chosen_targets)
        return NULL;

    return targets[k].target;
}
