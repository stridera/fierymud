/***************************************************************************
 *   File: act.item.c                                     Part of FieryMUD *
 *  Usage: object handling routines -- get/drop and container handling     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "board.hpp"
#include "chars.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "cooldowns.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "limits.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "money.hpp"
#include "pfiles.hpp"
#include "races.hpp"
#include "regen.hpp"
#include "screen.hpp"
#include "shop.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <math.h>

/* GIVE return codes */
#define GIVE_FAIL 0
#define GIVE_SUCCESS 1
#define GIVE_FAIL_FULL 2
#define GIVE_FAIL_FLUID 3
#define GIVE_FAIL_DONTHAVE 4

/* extern variables */
ACMD(do_action);
ACMD(do_say);
ACMD(do_bash);

/*
 * Returns an integer.  100 and below should be failure.  Above 100 should
 * be interpreted as success.
 */
#define MAX_CONCEAL_WEIGHT 200
int conceal_roll(CharData *ch, ObjData *obj) {
    int skill = GET_SKILL(ch, SKILL_CONCEAL);
    int lower_bound = -0.0008 * pow(skill, 3) + 0.1668 * pow(skill, 2) - 3.225 * skill;
    int upper_bound = 2000 * skill / (3 * GET_DEX(ch) + GET_INT(ch));
    int roll = random_number(lower_bound, upper_bound) + stat_bonus[GET_DEX(ch)].rogue_skills;

    /* You can't conceal/palm/stow an item higher than your level. */
    if (GET_OBJ_LEVEL(obj) > GET_LEVEL(ch))
        return 0;

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        /* Modify roll by weight. */
        roll *= (MAX_CONCEAL_WEIGHT - GET_OBJ_EFFECTIVE_WEIGHT(obj)) / ((float)MAX_CONCEAL_WEIGHT);

        /* It is more difficult to conceal items of a level closer to your own. */
        roll -= GET_OBJ_LEVEL(obj) - GET_LEVEL(ch);
    }

    return std::max(0, roll);
}

void perform_put(CharData *ch, ObjData *obj, ObjData *cont) {
    if (!drop_otrigger(obj, ch, cont) || !drop_otrigger(cont, ch, obj))
        return;
    if (GET_OBJ_EFFECTIVE_WEIGHT(cont) + GET_OBJ_EFFECTIVE_WEIGHT(obj) > GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY))
        act("$p won't fit in $P.", false, ch, obj, cont, TO_CHAR);
    else if (OBJ_FLAGGED(obj, ITEM_NODROP))
        act("You can't put $p in $P - it must be CURSED!", false, ch, obj, cont, TO_CHAR);
    else {
        if (!RIGID(ch) && cont->carried_by != ch)
            act("$p becomes solid again as it leaves your grasp.", false, ch, obj, 0, TO_CHAR);
        obj_from_char(obj);
        obj_to_obj(obj, cont);
        /* When you put a nonpermanent light in a container, it is
         * extinguished. */
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, VAL_LIGHT_LIT) &&
            GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) >= 0) {
            GET_OBJ_VAL(obj, 0) = 0;
            /* obj_from_char takes care of decrementing room light */
            act("You extinguish $p and put it in $P.", false, ch, obj, cont, TO_CHAR);
            act("$n extinguishes $p and puts it in $P.", true, ch, obj, cont, TO_ROOM);
        } else {
            act("You put $p in $P.", false, ch, obj, cont, TO_CHAR);
            act("$n puts $p in $P.", true, ch, obj, cont, TO_ROOM);
        }
    }
}

/* The following put modes are supported by the code below:

   1) put <object> <container>
   2) put all.<object> <container>
   3) put all <container>

   <container> must be in inventory or on ground.
   all objects to be put into container must be in inventory.
*/

ACMD(do_put) {
    char argbuf1[MAX_INPUT_LENGTH], *arg1 = argbuf1;
    char argbuf2[MAX_INPUT_LENGTH], *arg2 = argbuf2;
    ObjData *obj, *next_obj, *cont;
    CharData *tmp_char;
    int obj_dotmode, cont_dotmode, found = 0;

    two_arguments(argument, arg1, arg2);
    obj_dotmode = find_all_dots(&arg1);
    cont_dotmode = find_all_dots(&arg2);

    if (!*arg1)
        char_printf(ch, "Put what in what?\n");
    else if (cont_dotmode != FIND_INDIV)
        char_printf(ch, "You can only put things into one container at a time.\n");
    else if (!*arg2)
        char_printf(ch, "What do you want to put {} in?\n", ((obj_dotmode == FIND_INDIV) ? "it" : "them"));
    else if (CONFUSED(ch))
        char_printf(ch, "You're too confused.\n");
    else {
        generic_find(arg2, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
        if (!cont)
            char_printf(ch, "You don't see {} {} here.\n", AN(arg2), arg2);
        else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
            act("$p is not a container.", false, ch, cont, 0, TO_CHAR);
        else if (IS_SET(GET_OBJ_VAL(cont, VAL_CONTAINER_BITS), CONT_CLOSED))
            char_printf(ch, "You'd better open it first!\n");
        else {
            if (obj_dotmode == FIND_INDIV) { /* put <obj> <container> */
                if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1))))
                    char_printf(ch, "You aren't carrying {} {}.\n", AN(arg1), arg1);
                else if (obj == cont)
                    char_printf(ch, "You attempt to fold it into itself, but fail.\n");
                else
                    perform_put(ch, obj, cont);
            } else {
                for (obj = ch->carrying; obj; obj = next_obj) {
                    next_obj = obj->next_content;
                    if (obj != cont && CAN_SEE_OBJ(ch, obj) && (obj_dotmode == FIND_ALL || isname(arg1, obj->name))) {
                        found = 1;
                        perform_put(ch, obj, cont);
                    }
                }
                if (!found) {
                    if (obj_dotmode == FIND_ALL)
                        char_printf(ch, "You don't seem to have anything to put in it.\n");
                    else
                        char_printf(ch, "You don't seem to have any {}{}.\n", arg1, isplural(arg1) ? "" : "s");
                }
            }
        }
    }
}

ACMD(do_stow) {
    char argbuf1[MAX_INPUT_LENGTH], *arg1 = argbuf1;
    char argbuf2[MAX_INPUT_LENGTH], *arg2 = argbuf2;
    ObjData *obj, *cont = nullptr;
    CharData *tch;
    int obj_dotmode, cont_dotmode;

    ACMD(do_drop);

    if (CONFUSED(ch)) {
        char_printf(ch, "You are too confused.\n");
        return;
    }

    two_arguments(argument, arg1, arg2);
    obj_dotmode = find_all_dots(&arg1);
    cont_dotmode = find_all_dots(&arg2);

    if (!GET_SKILL(ch, SKILL_CONCEAL)) {
        if (*arg2)
            do_put(ch, argument, cmd, 0);
        else
            do_drop(ch, argument, cmd, SCMD_DROP);
    } else if (!*arg1)
        char_printf(ch, "Stow what in what?\n");
    else if (obj_dotmode != FIND_INDIV)
        char_printf(ch, "You can only stow one item at a time.\n");
    else if (cont_dotmode != FIND_INDIV)
        char_printf(ch, "You'll need to break it into pieces to put it into more than one container!\n");
    else {
        if (*arg2)
            generic_find(arg2, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tch, &cont);
        if (*arg2 && !cont)
            char_printf(ch, "You don't see {} {} here.\n", AN(arg2), arg2);
        else if (cont && GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
            act("$p is not a container.", false, ch, cont, 0, TO_CHAR);
        else if (cont && IS_SET(GET_OBJ_VAL(cont, VAL_CONTAINER_BITS), CONT_CLOSED))
            char_printf(ch, "You'd better open it first!\n");
        else {
            if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1))))
                char_printf(ch, "You aren't carrying {} {}.\n", AN(arg1), arg1);
            else if (obj == cont)
                char_printf(ch, "You attempt to fold it into itself, but fail.\n");
            else if (!drop_otrigger(obj, ch, cont))
                return;
            else if (OBJ_FLAGGED(obj, ITEM_NODROP))
                act("You can't stow $p in $P because it's CURSED!", false, ch, obj, cont, TO_CHAR);
            else if (cont && GET_OBJ_EFFECTIVE_WEIGHT(cont) + GET_OBJ_EFFECTIVE_WEIGHT(obj) >
                                 GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY))
                act("$p won't fit in $P.", false, ch, obj, cont, TO_CHAR);
            else {
                int roll = conceal_roll(ch, obj);
                obj_from_char(obj);
                if (!*arg2) {
                    obj_to_room(obj, ch->in_room);
                    act("You sneakily toss $p by your feet.", false, ch, obj, 0, TO_CHAR);
                } else {
                    obj_to_obj(obj, cont);
                    /* When you stow a nonpermanent light in a container, it is
                     * automatically extinguished. */
                    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, VAL_LIGHT_LIT) &&
                        GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) >= 0) {
                        GET_OBJ_VAL(obj, VAL_LIGHT_LIT) = 0;
                        roll -= 20;
                        /* obj_from_char takes care of decrementing room light */
                        act("You extinguish $p and slip it inside $P.", false, ch, obj, cont, TO_CHAR);
                    } else
                        act("You furtively slip $p inside $P.", false, ch, obj, cont, TO_CHAR);
                }
                LOOP_THRU_PEOPLE(tch, ch) {
                    if (tch == ch)
                        continue;
                    if (!CAN_SEE(tch, ch))
                        continue;
                    if (CAN_SEE(ch, tch) ? (GET_PERCEPTION(tch) < roll - 50 + random_number(0, 50))
                                         : (GET_PERCEPTION(tch) < roll / 2))
                        continue;
                    if (!*arg2)
                        act("$n tosses $p by $s feet.", false, ch, obj, tch, TO_VICT);
                    else {
                        sprintf(buf, "%s looks around, furtively slipping $p into $P.", GET_NAME(ch));
                        act(buf, true, tch, obj, cont, TO_CHAR);
                    }
                }
                WAIT_STATE(ch, PULSE_VIOLENCE);
                improve_skill(ch, SKILL_CONCEAL);
            }
        }
    }
}

void perform_drop(CharData *ch, ObjData *obj, byte mode, const char *verb) {
    const char *sname = verb;
    /* Possible modes here are SCMD_DROP, SCMD_JUNK, SCMD_DONATE,
     * and SCMD_LETDROP.  That last mode occurs when you tried to
     * give an object to someone who was insubstantial, so the
     * object falls to the ground.  You could also use it for any
     * drop-like action where you don't want this function to
     * print messages. */

    /* If you cannot drop the item, you cannot invoke its drop trigger,
     * so this comes before the trigger checks. */

    if (OBJ_FLAGGED(obj, ITEM_NODROP) && GET_LEVEL(ch) < 100) {
        sprintf(buf, "You can't %s $p - it must be CURSED!", verb);
        act(buf, false, ch, obj, 0, TO_CHAR);
        return;
    }

    if (OBJ_FLAGGED(obj, ITEM_NOFALL))
        sname = "release";

    /* Fiery does not use 'donate', and 'junk' should not cause a drop
     * trigger to go off. */

    if (mode == SCMD_DROP || mode == SCMD_LETDROP) {
        if (!drop_otrigger(obj, ch, nullptr))
            return;
        if (!drop_wtrigger(obj, ch))
            return;
    }

    if (mode != SCMD_LETDROP) {
        sprintf(buf, "You %s $p.", sname);
        act(buf, false, ch, obj, 0, TO_CHAR);
        sprintf(buf, "$n %ss $p.", sname);
        act(buf, !HIGHLY_VISIBLE(obj) || GET_INVIS_LEV(ch), ch, obj, 0, TO_ROOM);
    }

    if (mode != SCMD_JUNK && !RIGID(ch))
        act("$p becomes solid again as it leaves your grasp.", false, ch, obj, 0, TO_CHAR);
    obj_from_char(obj);

    switch (mode) {
    case SCMD_DROP:
    case SCMD_LETDROP:
        obj_to_room(obj, ch->in_room);
        break;
    case SCMD_JUNK:
        extract_obj(obj);
        break;
    default:
        log("SYSERR: Incorrect argument passed to perform_drop");
        break;
    }
}

ObjData *random_inventory_object(CharData *ch) {
    ObjData *obj, *chosen = nullptr;
    int count = 0;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (CAN_SEE_OBJ(ch, obj)) {
            if (chosen == nullptr || random_number(0, count) == 0)
                chosen = obj;
            count++;
        }
    }

    return chosen;
}

ObjData *confused_inventory_switch(CharData *ch, ObjData *obj) {
    ObjData *chosen = obj;

    if (CONFUSED(ch) && random_number(0, 1) == 0) {
        chosen = random_inventory_object(ch);
        if (chosen && obj != chosen) {
            act("&5Fumbling about, you grasp $p&0&5...&0", false, ch, chosen, 0, TO_CHAR);
            return chosen;
        }
    }

    return obj;
}

void drop_random_object(CharData *ch) {
    ObjData *obj;
    if ((obj = random_inventory_object(ch)))
        perform_drop(ch, obj, SCMD_DROP, "drop");
}

ACMD(do_drop) {
    ObjData *obj, *next_obj;
    int dotmode, amount, type, total;
    const char *cmdname = "drop";
    int coins[4] = {0, 0, 0, 0};
    FindContext context;
    char *name = arg;

    if (subcmd == SCMD_JUNK) {
        cmdname = "junk";
        if (CONFUSED(ch)) {
            char_printf(ch, "That's impossible in your confused state.\n");
            return;
        }
    }

    if (parse_money(&argument, coins)) {
        if (!CASH_VALUE(coins)) {
            char_printf(ch, "You drop 0 coins.  Okaaayy...\n");
            return;
        }
        for (type = 0; type < NUM_COIN_TYPES; ++type)
            if (GET_COINS(ch)[type] < coins[type]) {
                char_printf(ch, "You don't have enough {}!\n", COIN_NAME(type));
                return;
            }
        obj = create_money(coins);
        obj_to_char(obj, ch);
        for (type = 0; type < NUM_COIN_TYPES; ++type)
            GET_COINS(ch)[type] -= coins[type];
        perform_drop(ch, obj, subcmd, cmdname);
        return;
    }

    argument = one_argument(argument, name);

    if (!*name) {
        char_printf(ch, "What do you want to {}?\n", cmdname);
        return;
    }

    dotmode = find_all_dots(&name);

    if (dotmode == FIND_ALL) {
        if (subcmd == SCMD_JUNK)
            char_printf(ch, "Go to the dump if you want to junk EVERYTHING!\n");
        else if (!ch->carrying)
            char_printf(ch, "You don't seem to be carrying anything.\n");
        else
            for (obj = ch->carrying; obj; obj = next_obj) {
                next_obj = obj->next_content;
                perform_drop(ch, obj, subcmd, cmdname);
            }
    } else if (dotmode == FIND_ALLDOT) {
        context = find_vis_by_name(ch, name);
        if (!*name)
            char_printf(ch, "What do you want to {} all of?\n", cmdname);
        else if (!(obj = find_obj_in_list(ch->carrying, context)))
            char_printf(ch, "You don't seem to have any {}{}.\n", name, isplural(name) ? "" : "s");
        else
            while (obj) {
                next_obj = find_obj_in_list(obj->next_content, context);
                perform_drop(ch, obj, subcmd, cmdname);
                obj = next_obj;
            }
    } else {
        amount = 1;
        if (is_number(name)) {
            skip_spaces(&argument);
            if (*argument) {
                amount = atoi(name);
                one_argument(argument, name);
            }
        }
        context = find_vis_by_name(ch, name);

        if (!amount)
            char_printf(ch, "So...you don't want to drop anything?\n");
        else if (!(obj = find_obj_in_list(ch->carrying, context)))
            char_printf(ch, "You don't seem to have {} {}{}.\n", amount == 1 ? AN(name) : "any", arg,
                        amount == 1 || isplural(name) ? "" : "s");
        else {
            total = amount;

            while (obj && amount > 0) {
                next_obj = find_obj_in_list(obj->next_content, context);
                --amount;
                if (CONFUSED(ch) && random_number(0, 1) == 0) {
                    drop_random_object(ch);
                    /* Cannot continue loop; drop_random_object may drop next_obj */
                    break;
                } else
                    perform_drop(ch, obj, subcmd, cmdname);
                obj = next_obj;
            }

            if (!CONFUSED(ch) && amount)
                char_printf(ch, "You only had {:d} {}{}.\n", total - amount, name,
                            isplural(name) || total - amount == 1 ? "" : "s");
        }
    }
}

int check_container_give(ObjData *obj, CharData *ch, CharData *vict) {
    ObjData *cont;
    int retval = 0;

    if (obj->contains) {
        cont = obj->contains;
        while (cont) {
            if (GET_OBJ_LEVEL(cont) > GET_LEVEL(vict) + 15) {
                sprintf(buf, "%s isn't experienced enough to use $p that is in $P.", GET_NAME(vict));
                act(buf, false, ch, cont, obj, TO_CHAR);
                return 1;
            }
            if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
                if (check_container_give(cont, ch, vict))
                    return 1;
            }
            cont = cont->next_content;
        }
    }

    return retval;
}

int perform_give(CharData *ch, CharData *vict, ObjData *obj, int silent) {

    // If a god wants to give something, nothing should stop them.
    if (GET_LEVEL(ch) < LVL_IMMORT || IS_NPC(ch)) {

        if (OBJ_FLAGGED(obj, ITEM_NODROP) && GET_LEVEL(ch) < 100 &&
            !(IS_NPC(ch) && (!(ch)->desc || GET_LEVEL(POSSESSOR(ch)) >= LVL_IMPL))) {
            act("You can't let go of $p!!  Yeech!", false, ch, obj, 0, TO_CHAR);
            return GIVE_FAIL;
        }
        if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict) && !(IS_NPC(ch) && !IS_NPC(vict))) {
            act("$N seems to have $S hands full.", false, ch, 0, vict, TO_CHAR);
            return GIVE_FAIL_FULL;
        }
        if (!ADDED_WEIGHT_OK(vict, obj) && !(IS_NPC(ch) && !IS_NPC(vict))) {
            act("$E can't carry that much weight.", false, ch, 0, vict, TO_CHAR);
            return GIVE_FAIL;
        }
        if (ADDED_WEIGHT_REFUSED(vict, obj) && !(IS_NPC(ch) && !IS_NPC(vict))) {
            act("$E doesn't look like $E could handle the additional weight.", false, ch, 0, vict, TO_CHAR);
            return GIVE_FAIL;
        }
        if ((GET_OBJ_LEVEL(obj) > GET_LEVEL(vict) + 15) || (GET_OBJ_LEVEL(obj) > 99 && GET_LEVEL(vict) < 100)) {
            act("$E isn't experienced enough to handle the awesome might of $p.", false, ch, obj, vict, TO_CHAR);
            return GIVE_FAIL;
        }
        if (!give_otrigger(obj, ch, vict) || !receive_mtrigger(vict, ch, obj))
            return GIVE_FAIL;

        if (give_shopkeeper_reject(ch, vict, obj))
            return GIVE_FAIL;

        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
            if (check_container_give(obj, ch, vict))
                return GIVE_FAIL;

        if (!RIGID(ch) && !RIGID(vict) && !MOB_FLAGGED(vict, MOB_ILLUSORY)) {
            /* Between fluid persons, the transfer seems completely normal */
        } else if (!RIGID(ch))
            act("$p becomes solid again as it leaves your grasp.", false, ch, obj, 0, TO_CHAR);

        /* When you give items to an illusory or fluid mob, they fall to the ground.
         */

        if (SOLIDCHAR(ch) && !SOLIDCHAR(vict) && GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(vict) < LVL_IMMORT) {
            /* Note that "silent" only applies to success messages. */
            act("$n gives you $p, but you can't hold onto it.", false, ch, obj, vict, TO_VICT);
            if (IS_SPLASHY(IN_ROOM(vict))) {
                act("You hand $p to $N, but it simply falls into the water.", false, ch, obj, vict, TO_CHAR);
                act("$n gives $p to $N, but it falls into the water.", !HIGHLY_VISIBLE(obj) || GET_INVIS_LEV(ch), ch,
                    obj, vict, TO_NOTVICT);
            } else {
                act("You hand $p to $N, but it simply falls to the ground.", false, ch, obj, vict, TO_CHAR);
                act("$n gives $p to $N, but it falls to the ground.", !HIGHLY_VISIBLE(obj) || GET_INVIS_LEV(ch), ch,
                    obj, vict, TO_NOTVICT);
            }
            perform_drop(ch, obj, SCMD_LETDROP, "release");
            return GIVE_FAIL;
        }
    }
    obj_from_char(obj);
    obj_to_char(obj, vict);

    if (!silent) {
        act("You give $p to $N.", false, ch, obj, vict, TO_CHAR);
        act("$n gives you $p.", false, ch, obj, vict, TO_VICT);
        act("$n gives $p to $N.", !HIGHLY_VISIBLE(obj) || GET_INVIS_LEV(ch), ch, obj, vict, TO_NOTVICT);
    }

    return GIVE_SUCCESS;
}

/* utility function for give */
CharData *give_find_vict(CharData *ch, char *arg) {
    CharData *vict;

    if (!*arg) {
        char_printf(ch, "To who?\n");
        return nullptr;
    } else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        char_printf(ch, NOPERSON);
        return nullptr;
    } else if (vict == ch) {
        char_printf(ch, "What's the point of that?\n");
        return nullptr;
    } else
        return vict;
}

void perform_give_money(CharData *ch, CharData *vict, int coins[]) {
    bool afford = true;
    int amount = 0, i;
    ObjData *obj;

    for (i = 0; i <= 3; i++)
        amount = amount + coins[i];

    if (amount <= 0) {
        char_printf(ch, "Heh heh heh ... we are jolly funny today, eh?\n");
        return;
    }

    if (GET_PLATINUM(ch) < coins[PLATINUM])
        afford = false;
    else if (GET_GOLD(ch) < coins[GOLD])
        afford = false;
    else if (GET_SILVER(ch) < coins[SILVER])
        afford = false;
    else if (GET_COPPER(ch) < coins[COPPER])
        afford = false;

    if (!afford) {
        char_printf(ch, "You don't have that many coins!\n");
        return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        GET_PLATINUM(ch) -= coins[PLATINUM];
        GET_GOLD(ch) -= coins[GOLD];
        GET_SILVER(ch) -= coins[SILVER];
        GET_COPPER(ch) -= coins[COPPER];
    }

    if (!SOLIDCHAR(vict) && GET_LEVEL(vict) < LVL_IMMORT) {
        obj = create_money(coins);
        act("The coins seem to fall right through $n's hands!", false, vict, 0, ch, TO_VICT);
        if (IS_SPLASHY(IN_ROOM(vict))) {
            act("$n tries to give some coins to $N, but they fell into the water!", true, ch, 0, vict, TO_NOTVICT);
        } else {
            act("$n tries to give some coins to $N, but they fell to the ground!", true, ch, 0, vict, TO_NOTVICT);
        }
        obj_to_room(obj, vict->in_room);
        return;
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        char_printf(ch, OK);
    else {
        strcpy(buf, "You give $n ");
        statemoney(buf + strlen(buf), coins);
        strcat(buf, ".");
        act(buf, false, vict, 0, ch, TO_VICT);
    }

    strcpy(buf, "$n gives you ");
    statemoney(buf + strlen(buf), coins);
    strcat(buf, ".");
    act(buf, false, ch, 0, vict, TO_VICT);

    act("$n gives some coins to $N.", true, ch, 0, vict, TO_NOTVICT);

    GET_PLATINUM(vict) += coins[PLATINUM];
    GET_GOLD(vict) += coins[GOLD];
    GET_SILVER(vict) += coins[SILVER];
    GET_COPPER(vict) += coins[COPPER];

    bribe_mtrigger(vict, ch, coins);

    if (coins[PLATINUM] > 50 || coins[GOLD] > 500) {
        log(LogSeverity::Debug, LVL_GOD, "{} gave {:d} Plat {:d} Gold to {}", GET_NAME(ch), coins[PLATINUM],
            coins[GOLD], GET_NAME(vict));
    }
}

ACMD(do_give) {
    int amount = 1, dotmode, result = GIVE_FAIL_DONTHAVE, i, counter = 0;
    CharData *vict;
    ObjData *obj, *next_obj, *ref_obj = nullptr;
    int cash[NUM_COIN_TYPES] = {0};
    char *name = strdup(arg);
    std::string item_list;
    std::unordered_map<ObjData *, int> vnums;
    int vnum;

    if (parse_money(&argument, cash)) {
        one_argument(argument, name);
        if (!(vict = give_find_vict(ch, name)))
            return;
        perform_give_money(ch, vict, cash);
        return;
    }

    argument = one_argument(argument, name);

    if (!*name) {
        char_printf(ch, "Give what to who?\n");
        return;
    }

    if (is_number(name)) {
        amount = atoi(name);
        argument = one_argument(argument, name);
    }

    /* And who are we giving this to? */
    one_argument(argument, buf1);
    if (!(vict = give_find_vict(ch, buf1)))
        return;

    dotmode = find_all_dots(&name);

    if (amount <= 0) {
        act("You give nothing to $N.", false, ch, 0, vict, TO_CHAR);
        return;
    }

    if (amount > 1 && dotmode != FIND_INDIV) {
        char_printf(ch, "Do you want to give '{:d}' or 'all'?  Make up your mind!\n", amount);
        return;
    }

    if (dotmode == FIND_INDIV) {
        for (i = 0; i < amount; ++i) {
            if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, name)))) {
                if (amount == 1)
                    char_printf(ch, "You don't seem to have a {}.\n", name);
                else
                    char_printf(ch, "You don't seem to have {:d} {}s.\n", amount, name);
                break;
            }
            obj = confused_inventory_switch(ch, obj);
            if (!ref_obj)
                ref_obj = obj;
            result = perform_give(ch, vict, obj, 1);
            if (result == GIVE_SUCCESS)
                ++counter;
            else if (result == GIVE_FAIL_FULL)
                break;
            auto it = std::find_if(vnums.begin(), vnums.end(), [&obj](const auto &pair) {
                return !strcmp(pair.first->short_description, obj->short_description);
            });
            if (it == vnums.end()) {
                vnums.insert(std::make_pair(obj, 1));
            } else {
                it->second++;
            }
        }
    } else if (CONFUSED(ch)) {
        char_printf(ch, "You are too confused for such juggling.\n");
        return;
    } else {
        if (dotmode == FIND_ALLDOT && !*name) {
            char_printf(ch, "All of what?\n");
            return;
        }
        counter = 0;
        for (obj = ch->carrying; obj; obj = next_obj) {
            next_obj = obj->next_content;
            if (!CAN_SEE_OBJ(ch, obj))
                continue;
            if (dotmode == FIND_ALL || isname(name, obj->name)) {
                if (!ref_obj)
                    ref_obj = obj;
                result = perform_give(ch, vict, obj, 1);
                if (result != GIVE_SUCCESS)
                    break;
                ++counter;
                auto it = std::find_if(vnums.begin(), vnums.end(), [&obj](const auto &pair) {
                    return !strcmp(pair.first->short_description, obj->short_description);
                });
                if (it == vnums.end()) {
                    vnums.insert(std::make_pair(obj, 1));
                } else {
                    it->second++;
                }
            }
        }
    }

    if (counter == 0) {
        if (result == GIVE_FAIL_DONTHAVE) {
            if (dotmode == FIND_ALLDOT)
                char_printf(ch, "You don't even have one {}!\n", name);
            else if (dotmode == FIND_ALL)
                char_printf(ch, "You don't seem to be holding anything.\n");
        }
    } else {
        for (auto [obj_ref2, qty] : vnums) {
            if (qty == 1) {
                act("You give $p to $N.", false, ch, obj_ref2, vict, TO_CHAR);
                act("$n gives you $p.", false, ch, obj_ref2, vict, TO_VICT);
                act("$n gives $p to $N.", !HIGHLY_VISIBLE(obj_ref2) || GET_INVIS_LEV(ch), ch, obj_ref2, vict,
                    TO_NOTVICT);
            } else {
                sprintf(buf, "You give $p to $N. (x%d)", qty);
                act(buf, false, ch, obj_ref2, vict, TO_CHAR);
                sprintf(buf, "$n gives you $p. (x%d)", qty);
                act(buf, false, ch, obj_ref2, vict, TO_VICT);
                sprintf(buf, "$n gives $p to $N. (x%d)", qty);
                act(buf, !HIGHLY_VISIBLE(obj_ref2) || GET_INVIS_LEV(ch), ch, obj_ref2, vict, TO_NOTVICT);
            }
        }
    }
}

ACMD(do_drink) {
    ObjData *temp;
    effect eff;
    int amount; /* ounces */
    int on_ground = 0;

    if (FIGHTING(ch)) {
        char_printf(ch, "You are afraid to try in combat!\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        char_printf(ch, "Drink from what?\n");
        return;
    }
    if (!(temp = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg)))) {
        if (!(temp = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, arg)))) {
            act("You can't find it!", false, ch, 0, 0, TO_CHAR);
            return;
        } else
            on_ground = 1;
    }
    if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) && (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
        char_printf(ch, "You can't drink from that!\n");
        return;
    }
    if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
        char_printf(ch, "You have to be holding that to drink from it.\n");
        return;
    }
    if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
        /* The pig is drunk */
        char_printf(ch, "You can't seem to get close enough to your mouth.\n");
        act("$n tries to drink but misses $s mouth!", true, ch, 0, 0, TO_ROOM);
        return;
    }
    if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 5)) {
        char_printf(ch, "Your stomach can't contain any more!\n");
        return;
    }
    if (GET_COND(ch, THIRST) > MAX_THIRST - HOURLY_THIRST_CHANGE) {
        char_printf(ch, "You couldn't drink another drop!\n");
        return;
    }

    if (!GET_OBJ_VAL(temp, VAL_DRINKCON_REMAINING)) {
        char_printf(ch, "It's empty.\n");
        return;
    }

    if (!consume_otrigger(temp, ch, SCMD_DRINK))
        return;

    if (subcmd == SCMD_DRINK) {
        sprintf(buf, "$n drinks %s from $p.", LIQ_NAME(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)));
        act(buf, true, ch, temp, 0, TO_ROOM);

        char_printf(ch, "You drink the {}.\n", LIQ_NAME(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)));

        /* Let's say you normally drink 1/2 cup (a lot, but hey). */
        amount = 4;
    } else {
        act("$n sips from $p.", true, ch, temp, 0, TO_ROOM);
        char_printf(ch, "It tastes like {}.\n", LIQ_NAME(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)));
        amount = 1;
    }

    amount = std::min(amount, GET_OBJ_VAL(temp, VAL_DRINKCON_REMAINING));

    /* You can't subtract more than the object weighs */
    liquid_from_container(temp, amount);

    gain_condition(ch, DRUNK, (LIQ_COND(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID), DRUNK) * amount) / 4);

    gain_condition(ch, FULL, (LIQ_COND(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID), FULL) * amount) / 1);

    gain_condition(ch, THIRST, (LIQ_COND(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID), THIRST) * amount) / 1);

    if (GET_COND(ch, DRUNK) > 10)
        char_printf(ch, "You feel drunk.\n");

    if (GET_COND(ch, THIRST) > 20)
        char_printf(ch, "You don't feel thirsty any more.\n");

    if (GET_COND(ch, FULL) > 20)
        char_printf(ch, "You are full.\n");

    if (IS_POISONED(temp)) {
        char_printf(ch, "Oops, it tasted rather strange!\n");
        act("$n chokes and utters some strange sounds.", true, ch, 0, 0, TO_ROOM);

        memset(&eff, 0, sizeof(eff));
        eff.type = SPELL_POISON;
        eff.duration = amount * 3;
        eff.modifier = 0;
        eff.location = APPLY_NONE;
        SET_FLAG(eff.flags, EFF_POISON);
        effect_join(ch, &eff, false, false, false, false, true);
    }

    /* This will restore movement points, such that drinking the max
     * would restore 100% or 200, whichever is lower. */
    alter_move(ch, -std::min(200, GET_MAX_MOVE(ch) * amount / MAX_THIRST));

    return;
}

ACMD(do_eat) {
    ObjData *food;
    effect eff;
    int amount;

    if (FIGHTING(ch)) {
        char_printf(ch, "You are afraid to try in combat!\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        char_printf(ch, "Eat what?\n");
        return;
    }
    if (!(food = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg)))) {
        char_printf(ch, "You don't seem to have {} {}.\n", AN(arg), arg);
        return;
    }
    if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) || (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
        do_drink(ch, argument, 0, SCMD_SIP);
        return;
    }
    if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && (GET_LEVEL(ch) < LVL_GOD)) {
        char_printf(ch, "You can't eat THAT!\n");
        return;
    }
    if (GET_COND(ch, FULL) > 20) { /* Stomach full */
        act("You are too full to eat more!", false, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!consume_otrigger(food, ch, SCMD_EAT))
        return;

    if (subcmd == SCMD_EAT) {
        act("You eat the $o.", false, ch, food, 0, TO_CHAR);
        act("$n eats $p.", true, ch, food, 0, TO_ROOM);
    } else {
        act("You nibble a little bit of the $o.", false, ch, food, 0, TO_CHAR);
        act("$n tastes a little bit of $p.", true, ch, food, 0, TO_ROOM);
    }

    amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, VAL_FOOD_FILLINGNESS) : 1);

    gain_condition(ch, FULL, amount);

    if (GET_COND(ch, FULL) > 20)
        act("You are full.", false, ch, 0, 0, TO_CHAR);

    if (IS_POISONED(food) && (GET_LEVEL(ch) < LVL_IMMORT)) {
        char_printf(ch, "Oops, that tasted rather strange!\n");
        act("$n coughs and utters some strange sounds.", false, ch, 0, 0, TO_ROOM);

        memset(&eff, 0, sizeof(eff));
        eff.type = SPELL_POISON;
        eff.duration = amount * 2;
        eff.modifier = 0;
        eff.location = APPLY_NONE;
        SET_FLAG(eff.flags, EFF_POISON);
        effect_join(ch, &eff, false, false, false, false, true);
    }
    /* This will restore hit points, such that a meal of size 24
     * would restore 33% or 70, whichever is lower. */
    hurt_char(ch, 0, -std::min(70, GET_MAX_HIT(ch) * amount / 72), true);

    if (subcmd == SCMD_EAT)
        extract_obj(food);
    else {
        if (!(--GET_OBJ_VAL(food, VAL_FOOD_FILLINGNESS))) {
            char_printf(ch, "There's nothing left now.\n");
            extract_obj(food);
        }
    }
}

/* Object values for liquid containers and fountains:
 *
 *     GET_OBJ_VAL(from_obj, X)
 *
 * 0: max drink units
 * 1: current drink units
 * 2: liquid type
 * 3: poisoned (boolean)
 *
 * Liquid types (as of 2006 November 16):
 *
 *   0) water                  1) beer
 *   2) wine                   3) ale
 *   4) dark ale               5) whisky
 *   6) lemonade               7) firebreather
 *   8) local speciality       9) slime mold juice
 *   10) milk                  11) tea
 *   12) coffee                13) blood
 *   14) salt water            15) rum
 */

ACMD(do_pour) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ObjData *from_obj = nullptr, *to_obj = nullptr;
    int amount;

    if (FIGHTING(ch)) {
        char_printf(ch, "You can't coordinate the maneuver while fighting!\n");
        return;
    }
    two_arguments(argument, arg1, arg2);

    if (subcmd == SCMD_POUR) {
        if (!*arg1) { /* No arguments */
            act("From what do you want to pour?", false, ch, 0, 0, TO_CHAR);
            return;
        }
        if (!(from_obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1)))) {
            act("You can't find it!", false, ch, 0, 0, TO_CHAR);
            return;
        }
        if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
            act("You can't pour from that!", false, ch, 0, 0, TO_CHAR);
            return;
        }
    }
    if (subcmd == SCMD_FILL) {
        if (!*arg1) { /* no arguments */
            char_printf(ch, "What do you want to fill?  And what are you filling it from?\n");
            return;
        }
        if (!(to_obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1)))) {
            char_printf(ch, "You can't find it!\n");
            return;
        }
        if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
            act("You can't fill $p!", false, ch, to_obj, 0, TO_CHAR);
            return;
        }
        if (!*arg2) { /* no 2nd argument */
            act("What do you want to fill $p from?", false, ch, to_obj, 0, TO_CHAR);
            return;
        }
        if (!(from_obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, arg2)))) {
            char_printf(ch, "There doesn't seem to be {} {} here.\n", AN(arg2), arg2);
            return;
        }
        if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
            act("You can't fill something from $p.", false, ch, from_obj, 0, TO_CHAR);
            return;
        }
    }
    if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_REMAINING) == 0) {
        act("The $o is empty.", false, ch, from_obj, 0, TO_CHAR);
        return;
    }
    if (subcmd == SCMD_POUR) { /* pour */
        if (!*arg2) {
            act("Where do you want it?  Out or in what?", false, ch, 0, 0, TO_CHAR);
            return;
        }
        if (!strcasecmp(arg2, "out")) {
            act("$n empties $p.", false, ch, from_obj, 0, TO_ROOM);
            act("You empty $p.", false, ch, from_obj, 0, TO_CHAR);
            liquid_from_container(from_obj, GET_OBJ_VAL(from_obj, VAL_DRINKCON_REMAINING));
            return;
        }
        if (!(to_obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg2)))) {
            act("You can't find it!", false, ch, 0, 0, TO_CHAR);
            return;
        }
        if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) && (GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
            act("You can't pour anything into that.", false, ch, 0, 0, TO_CHAR);
            return;
        }
    }
    if (to_obj == from_obj) {
        act("A most unproductive effort.", false, ch, 0, 0, TO_CHAR);
        return;
    }
    if ((GET_OBJ_VAL(to_obj, VAL_DRINKCON_REMAINING) != 0) &&
        (GET_OBJ_VAL(to_obj, VAL_DRINKCON_LIQUID) != GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID))) {
        act("There is already another liquid in it!", false, ch, 0, 0, TO_CHAR);
        return;
    }
    if (GET_OBJ_VAL(to_obj, VAL_DRINKCON_REMAINING) >= GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY)) {
        act("There is no room for more.", false, ch, 0, 0, TO_CHAR);
        return;
    }
    if (subcmd == SCMD_POUR)
        char_printf(ch, "You pour the {} into the {}.\n", LIQ_NAME(GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID)), arg2);
    if (subcmd == SCMD_FILL) {
        act("You gently fill $p from $P.", false, ch, to_obj, from_obj, TO_CHAR);
        act("$n gently fills $p from $P.", !HIGHLY_VISIBLE(to_obj) || GET_INVIS_LEV(ch), ch, to_obj, from_obj, TO_ROOM);
    }

    /* how much to pour */
    amount = std::min(GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY) - GET_OBJ_VAL(to_obj, VAL_DRINKCON_REMAINING),
                      GET_OBJ_VAL(from_obj, VAL_DRINKCON_REMAINING));

    liquid_to_container(to_obj, amount, GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID), IS_POISONED(from_obj));

    if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
        liquid_from_container(from_obj, amount);
    }

    return;
}

void wear_message(CharData *ch, ObjData *obj, int where) {
    const char *wear_messages[][2] = {
        {"$n starts using $p as a light.", "You start using $p as a light."},
        {"$n slides $p onto $s right ring finger.", "You slide $p onto your right ring finger."},
        {"$n slides $p onto $s left ring finger.", "You slide $p onto your left ring finger."},
        {"$n wears $p around $s neck.", "You wear $p around your neck."},
        {"$n wears $p around $s neck.", "You wear $p around your neck."},
        {"$n wears $p on $s body.", "You wear $p on your body."},
        {"$n wears $p on $s head.", "You wear $p on your head."},
        {"$n puts $p on $s legs.", "You put $p on your legs."},
        {"$n wears $p on $s feet.", "You wear $p on your feet."},
        {"$n puts $p on $s hands.", "You put $p on your hands."},
        {"$n wears $p on $s arms.", "You wear $p on your arms."},
        {"$n straps $p around $s arm as a shield.", "You start to use $p as a shield."},
        {"$n wears $p about $s body.", "You wear $p around your body."},
        {"$n wears $p around $s waist.", "You wear $p around your waist."},
        {"$n puts $p on $s right wrist.", "You put $p on your right wrist."},
        {"$n puts $p on $s left wrist.", "You put $p on your left wrist."},
        {"$n wields $p.", "You wield $p."},
        {"$n wields $p.", "You wield $p."},
        {"$n grabs $p.", "You grab $p."},
        {"$n grabs $p.", "You grab $p."},
        {"$n wields $p.", "You wield $p."},
        {"$n wears $p over $s eyes.", "You wear $p over your eyes."},
        {"$n wears $p on $s face.", "You wear $p on your face."},
        {"$n wears $p in $s left ear.", "You wear $p in your left ear."},
        {"$n wears $p in $s right ear.", "You wear $p in your right ear."},
        {"$n wears $p as a badge.", "You wear $p as a badge."},
        {"$n attaches $p to $s belt.", "You attach $p to your belt."},
        {"$p begins to hover over $n's shoulder.", "$p starts to hover over your shoulder."}

    };

    act(wear_messages[where][0], true, ch, obj, 0, TO_ROOM);
    act(wear_messages[where][1], false, ch, obj, 0, TO_CHAR);
}

/* Returns true if the object was worn. */

bool perform_wear(CharData *ch,           /* Who is trying to wear something */
                  ObjData *obj,           /* The object being put on */
                  int where,              /* The positioni it should be worn at */
                  bool collective = false /* Whether the character has issued a command
                                     like "wear all" or "wear all.leather". This
                                     parameter will suppress the error message
                                     for objects that can't be worn. */
) {
    if (!may_wear_eq(ch, obj, &where, !collective))
        return false;
    if (!wear_otrigger(obj, ch, where))
        return false;
    wear_message(ch, obj, where);
    obj_from_char(obj);
    equip_char(ch, obj, where);
    return true;
}

int find_eq_pos(CharData *ch, ObjData *obj, char *arg) {
    int where = -1;

    static const char *keywords[] = {"!RESERVED!", "finger",     "!RESERVED!", "neck",       "!RESERVED!", "body",
                                     "head",       "legs",       "feet",       "hands",      "arms",       "shield",
                                     "about",      "waist",      "wrist",      "!RESERVED!", "!RESERVED!", "!RESERVED!",
                                     "!RESERVED!", "!RESERVED!", "!RESERVED!", "eyes",       "face",       "ear",
                                     "!RESERVED!", "badge",      "belt",       "hover",      "\n"};

    if (!arg || !*arg) {
        if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
            where = WEAR_FINGER_R;
        if (CAN_WEAR(obj, ITEM_WEAR_NECK))
            where = WEAR_NECK_1;
        if (CAN_WEAR(obj, ITEM_WEAR_BODY))
            where = WEAR_BODY;
        if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
            where = WEAR_HEAD;
        if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
            where = WEAR_LEGS;
        if (CAN_WEAR(obj, ITEM_WEAR_FEET))
            where = WEAR_FEET;
        if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
            where = WEAR_HANDS;
        if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
            where = WEAR_ARMS;
        if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
            where = WEAR_SHIELD;
        if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
            where = WEAR_ABOUT;
        if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
            where = WEAR_WAIST;
        if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
            where = WEAR_WRIST_R;
        if (CAN_WEAR(obj, ITEM_WEAR_EYES))
            where = WEAR_EYES;
        if (CAN_WEAR(obj, ITEM_WEAR_FACE))
            where = WEAR_FACE;
        if (CAN_WEAR(obj, ITEM_WEAR_EAR))
            where = WEAR_LEAR;
        if (CAN_WEAR(obj, ITEM_WEAR_BADGE))
            where = WEAR_BADGE;
        if (CAN_WEAR(obj, ITEM_WEAR_OBELT))
            where = WEAR_OBELT;
        if (CAN_WEAR(obj, ITEM_WEAR_HOVER))
            where = WEAR_HOVER;
        /* Add Wield and 2 Handed Wield to this search block - DE 8/13/99 */
        if (CAN_WEAR(obj, ITEM_WEAR_WIELD))
            where = WEAR_WIELD;
        if (CAN_WEAR(obj, ITEM_WEAR_2HWIELD))
            where = WEAR_2HWIELD;
    } else {
        /* 2/6/02 - DCE Put in a check for !. Players could wear item !,
           and it would put the item in the light position. */
        if (!strcasecmp(arg, "!") || (where = search_block(arg, keywords, false)) < 0) {
            char_printf(ch, "'{}'?  What part of your body is THAT?\n", arg);
        }
    }

    return where;
}

ACMD(do_wear) {
    char argbuf1[MAX_INPUT_LENGTH], *arg1 = argbuf1;
    char argbuf2[MAX_INPUT_LENGTH], *arg2 = argbuf2;
    ObjData *obj, *next_obj;
    int where, dotmode, wearable_items = 0, worn_items = 0;

    two_arguments(argument, arg1, arg2);

    if (GET_RACE(ch) == RACE_ANIMAL) {
        char_printf(ch, "Animals can't wear clothes!\n");
        return;
    }

    if (!*arg1) {
        char_printf(ch, "Wear what?\n");
        return;
    }
    dotmode = find_all_dots(&arg1);

    if (*arg2 && (dotmode != FIND_INDIV)) {
        char_printf(ch, "You can't specify the same body location for more than one item!\n");
        return;
    }
    if (dotmode == FIND_ALL) {
        if (CONFUSED(ch)) {
            char_printf(ch, "You're a bit too confused for mass equipment changes.\n");
            return;
        }
        for (obj = ch->carrying; obj; obj = next_obj) {
            next_obj = obj->next_content;
            if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
                wearable_items++;
                if (perform_wear(ch, obj, where, true))
                    worn_items++;
            }
        }
        if (!wearable_items || !worn_items)
            char_printf(ch, "You don't have anything you can wear.\n");
    }

    else if (dotmode == FIND_ALLDOT) {
        if (!*arg1) {
            char_printf(ch, "Wear all of what?\n");
            return;
        }
        if (CONFUSED(ch)) {
            char_printf(ch, "You're a bit too confused for mass equipment changes.\n");
            return;
        }
        if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1))))
            char_printf(ch, "You don't seem to have any {}{}.\n", arg1, isplural(arg1) ? "" : "s");
        else {
            while (obj) {
                next_obj = find_obj_in_list(obj->next_content, find_vis_by_name(ch, arg1));
                if ((where = find_eq_pos(ch, obj, 0)) >= 0) {
                    if (perform_wear(ch, obj, where, true))
                        worn_items++;
                }
                obj = next_obj;
            }
            if (!worn_items)
                char_printf(ch, "You don't have anything wearable like that.\n");
        }
    }

    /* FIND_INDIV */
    else {
        if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1)))) {
            char_printf(ch, "You don't seem to have {} {}.\n", AN(arg1), arg1);
            return;
        }
        obj = confused_inventory_switch(ch, obj);
        if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
            perform_wear(ch, obj, where, false);
        else if (!*arg2)
            act("You can't wear $p.", false, ch, obj, 0, TO_CHAR);
    }
}

ACMD(do_wield) {
    ObjData *obj;
    int hands_used, weapon_hands_used;

    one_argument(argument, arg);

    /* Basic checks first */

    if (GET_RACE(ch) == RACE_ANIMAL)
        return;

    if (!*arg) {
        char_printf(ch, "Wield what?\n");
        return;
    }

    if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg)))) {
        char_printf(ch, "You don't seem to have {} {}.\n", AN(arg), arg);
        return;
    }

    if (!CAN_WEAR(obj, ITEM_WEAR_WIELD) && !CAN_WEAR(obj, ITEM_WEAR_2HWIELD)) {
        char_printf(ch, "You can't wield that.\n");
        return;
    }

    if (GET_OBJ_EFFECTIVE_WEIGHT(obj) > stat_bonus[GET_STR(ch)].wield) {
        char_printf(ch, "It's too heavy for you to use.\n");
        return;
    }

    /* By now we know it's a weapon that can be wielded by this char
     * under some circumstances, at least. So check for free hands,
     * dual wielding, and two-handed weapon. */

    count_hand_eq(ch, &hands_used, &weapon_hands_used);

    /* Both hands used? Bye. */
    if (hands_used > 1) {
        char_printf(ch, "Your hands are full!\n");
        return;
    }

    /* See if they need both hands free: two-handed weapon and they aren't an
     * ogre. */
    if (CAN_WEAR(obj, ITEM_WEAR_2HWIELD) && GET_CLASS(ch) != RACE_OGRE) {
        if (hands_used) {
            char_printf(ch, "You need both hands for this weapon.\n");
            return;
        }
        perform_wear(ch, obj, WEAR_2HWIELD, false);
        return;
    }

    /* This weapon can be wielded one-handed, and ch has at least one hand free.
     */
    if (weapon_hands_used) {
        /* One weapon is already wielded. Got dual wield? */
        if GET_SKILL (ch, SKILL_DUAL_WIELD) {
            obj = confused_inventory_switch(ch, obj);
            perform_wear(ch, obj, WEAR_WIELD2, false);
        } else
            char_printf(ch, "You don't have the co-ordination to dual wield.\n");
    } else {
        obj = confused_inventory_switch(ch, obj);
        perform_wear(ch, obj, WEAR_WIELD, false);
    }
}

static MATCH_OBJ_FUNC(match_light_by_name) {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
        if (isname(context->string, obj->name))
            if (--context->number <= 0)
                return true;
    return false;
}

ACMD(do_light) {
    ObjData *obj;
    FindContext find_darklights;

    one_argument(argument, arg);

    if (!*arg) {
        if (subcmd == SCMD_EXTINGUISH)
            char_printf(ch, "Extinguish what?\n");
        else
            char_printf(ch, "Light what?\n");
        return;
    }

    /* Lights are searched for:
     * - worn (visible or not)
     * - in inventory (visible first)
     * - in inventory (not visible - allows you to turn on lights in an dark room)
     */

    find_darklights = find_by_name(arg);
    find_darklights.obj_func = match_light_by_name;

    if (!(obj = (find_obj_in_eq(ch, nullptr, find_darklights))) &&
        !(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg))) &&
        !(obj = find_obj_in_list(ch->carrying, find_darklights))) {
        char_printf(ch, "You don't seem to have {} {}.\n", AN(arg), arg);
        return;
    }

    obj = confused_inventory_switch(ch, obj);

    if (GET_OBJ_TYPE(obj) != ITEM_LIGHT) {
        if (subcmd == SCMD_EXTINGUISH)
            char_printf(ch, "You can't extinguish that!\n");
        else
            char_printf(ch, "You can't light that!\n");
        return;
    }

    if (!GET_OBJ_VAL(obj, VAL_LIGHT_LIT)) { /* It is not lit */

        if (subcmd == SCMD_EXTINGUISH) {
            char_printf(ch, "It isn't lit.\n");
            return;
        }

        if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) != 0) {    /* It is not worn out */
            if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) < 0) { /* It's a permanent light */
                act("You activate $p.", true, ch, obj, 0, TO_CHAR);
                act("$n activates $p.", true, ch, obj, 0, TO_ROOM);
            } else {
                act("You light $p.", true, ch, obj, 0, TO_CHAR);
                act("$n lights $p.", true, ch, obj, 0, TO_ROOM);
            }
            GET_OBJ_VAL(obj, VAL_LIGHT_LIT) = true; /* Now it's lit */
            if (ch->in_room != NOWHERE)
                world[ch->in_room].light++;
            else
                log("SYSERR: do_light - my CharData* object wasn't in a room!");
        } else
            act("Sorry, there's no more power left in $p.", true, ch, obj, 0, TO_CHAR);
    } else {
        /* It was already lit, so we are extinguishing it */
        if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) < 0) { /* It's a permanent light */
            act("You deactivate $p.", true, ch, obj, 0, TO_CHAR);
            act("$n deactivates $p.", true, ch, obj, 0, TO_ROOM);
        } else {
            act("You extinguish $p.", true, ch, obj, 0, TO_CHAR);
            act("$n extinguishes $p.", true, ch, obj, 0, TO_ROOM);
        }
        GET_OBJ_VAL(obj, VAL_LIGHT_LIT) = false;
        if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) != 0) {
            if (ch->in_room != NOWHERE)
                world[ch->in_room].light--;
            else
                log("SYSERR: do_light (extinguishing) - my CharData* object wasn't in "
                    "a room!");
        }
    }
}

ACMD(do_grab) {
    ObjData *obj;
    FindContext find_darklights;

    one_argument(argument, arg);

    find_darklights = find_by_name(arg);
    find_darklights.obj_func = match_light_by_name;

    if (!*arg)
        char_printf(ch, "Hold what?\n");
    else if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg))) &&
             !(obj = find_obj_in_list(ch->carrying, find_darklights)))
        char_printf(ch, "You don't seem to have {} {}.\n", AN(arg), arg);
    else {
        obj = confused_inventory_switch(ch, obj);
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
            perform_wear(ch, obj, WEAR_HOLD, false);
        else {
            if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND && GET_OBJ_TYPE(obj) != ITEM_STAFF &&
                GET_OBJ_TYPE(obj) != ITEM_SCROLL && GET_OBJ_TYPE(obj) != ITEM_POTION &&
                GET_OBJ_TYPE(obj) != ITEM_INSTRUMENT)
                char_printf(ch, "You can't hold that.\n");
            else {
                perform_wear(ch, obj, WEAR_HOLD, false);
            }
        }
    }
}

void perform_remove(CharData *ch, int pos) {
    ObjData *obj;

    if (!(obj = GET_EQ(ch, pos))) {
        log("Error in perform_remove: bad pos passed.");
        return;
    }
    if (!remove_otrigger(obj, ch))
        return;
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
        act("$p: you can't carry that many items!", false, ch, obj, 0, TO_CHAR);
    else {
        obj_to_char(unequip_char(ch, pos), ch);
        act("You stop using $p.", false, ch, obj, 0, TO_CHAR);
        act("$n stops using $p.", true, ch, obj, 0, TO_ROOM);
    }
}

ACMD(do_remove) {
    ObjData *obj;
    int where, dotmode, found;
    char *name = arg;

    if (GET_RACE(ch) == RACE_ANIMAL) {
        char_printf(ch, "Animals can't wear clothes!\n");
        return;
    }

    argument = one_argument(argument, name);

    if (!*name) {
        char_printf(ch, "Remove what?\n");
        return;
    }
    dotmode = find_all_dots(&name);

    if (dotmode == FIND_ALL) {
        found = 0;
        for (where = 0; where < NUM_WEARS; ++where)
            if (GET_EQ(ch, where)) {
                perform_remove(ch, where);
                found = 1;
            }
        if (!found)
            char_printf(ch, "You're not using anything.\n");
    }

    else if (dotmode == FIND_ALLDOT) {
        if (!*name)
            char_printf(ch, "Remove all of what?\n");
        else {
            found = 0;
            for (where = 0; where < NUM_WEARS; ++where)
                if (GET_EQ(ch, where) && isname(name, GET_EQ(ch, where)->name)) {
                    perform_remove(ch, where);
                    found = 1;
                }
            if (!found)
                char_printf(ch, "You don't seem to be using any {}{}.\n", name, isplural(name) ? "" : "s");
        }
    }

    else if (is_number(name) &&
             universal_find(find_vis_by_type(ch, ITEM_BOARD), FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_WORLD, nullptr,
                            &obj) &&
             obj)
        remove_message(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), atoi(name), obj);
    else if (is_number(argument) &&
             generic_find(name, FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_WORLD, ch, nullptr, &obj) && obj &&
             GET_OBJ_TYPE(obj) == ITEM_BOARD)
        remove_message(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), atoi(argument), obj);
    else if (!(obj = (find_obj_in_eq(ch, &where, find_vis_by_name(ch, name)))))
        char_printf(ch, "You don't seem to be using {} {}.\n", AN(name), name);
    else
        perform_remove(ch, where);

    /*
     * If the first weapon was removed but there is still a secondary
     * weapon, then move it to the first position.
     */
    if (GET_EQ(ch, WEAR_WIELD2) && !GET_EQ(ch, WEAR_WIELD))
        equip_char(ch, unequip_char(ch, WEAR_WIELD2), WEAR_WIELD);

    /*
     * If the character is wearing something on their belt, but is no
     * longer wearing a belt, remove the thing on their belt.
     */
    if (GET_EQ(ch, WEAR_OBELT) && !GET_EQ(ch, WEAR_WAIST)) {
        act("$p falls off as you remove your belt.", false, ch, GET_EQ(ch, WEAR_OBELT), 0, TO_CHAR);
        act("$p falls off as $n removes $s belt.", true, ch, GET_EQ(ch, WEAR_OBELT), 0, TO_ROOM);
        obj_to_char(unequip_char(ch, WEAR_OBELT), ch);
    }
}

bool check_get_disarmed_obj(CharData *ch, CharData *last_to_hold, ObjData *obj) {
    int rand;
    CharData *tmp_ch;
    bool retval = false;
    char Gbuf4[MAX_STRING_LENGTH];
    int cmd_say, cmd_glare;
    cmd_say = find_command("say");
    cmd_glare = find_command("glare");

    if (OBJ_FLAGGED(obj, ITEM_WAS_DISARMED)) {
        /* make sure owner isn't dead, or has left room. either case means */
        /* he doesn't care about his item(s) any longer */
        LOOP_THRU_PEOPLE(tmp_ch, ch)
        if (tmp_ch != ch && tmp_ch == last_to_hold)
            break;

        if (tmp_ch && AWAKE(tmp_ch)) {
            if (!IS_NPC(last_to_hold) && CONSENT(last_to_hold) == ch) {
                act("$n nods briefly as $N reaches for $p.", true, last_to_hold, obj, ch, TO_NOTVICT);
                act("You nod briefly as $N reaches for $p.", false, last_to_hold, obj, ch, TO_CHAR);
                act("$n nods briefly as you reach for $p.", true, last_to_hold, obj, ch, TO_VICT);
            } else if (GET_LEVEL(ch) >= LVL_IMMORT && GET_LEVEL(ch) >= GET_LEVEL(last_to_hold)) {
                /* nothing */
            } else if (random_number(1, 64) > 8) { /* darn, PC failed to grab it. now they pay. */
                rand = random_number(1, 4);

                if (rand == 1) {
                    act("$n makes a quick grab for $p!", false, ch, obj, 0, TO_ROOM);
                    act("You lunge for $p!", false, ch, obj, 0, TO_CHAR);

                    act("$N tries to block $n, but isn't quick enough.", false, ch, 0, last_to_hold, TO_NOTVICT);
                    act("You try to block $N, but $E's too fast.", false, last_to_hold, 0, ch, TO_CHAR);
                    act("$N tries to block you, but is far too slow.", false, ch, 0, last_to_hold, TO_CHAR);
                } else {
                    act("$n tries to grab $p!", false, ch, obj, 0, TO_ROOM);

                    strcpy(Gbuf4, "No you don't!! That belongs to me!");
                    do_say(last_to_hold, Gbuf4, cmd_say, 0);
                    strcpy(Gbuf4, GET_NAME(ch));
                    do_action(last_to_hold, Gbuf4, cmd_glare, 0);

                    act("$N plants $Mself directly in front of $n, blocking $m.", false, ch, 0, last_to_hold,
                        TO_NOTVICT);
                    act("You plant yourself directly in front of $N, blocking $M.", false, last_to_hold, 0, ch,
                        TO_CHAR);
                    act("$N steps directly in front of your path.  No way to get it now.", false, ch, 0, last_to_hold,
                        TO_CHAR);
                    retval = true;

                    /* Delay them so they can't spam grab attempts and bypass this
                     * too easily */
                    WAIT_STATE(ch, PULSE_VIOLENCE + 1);
                }
            }
        }
    }

    if (!retval)
        REMOVE_FLAG(GET_OBJ_FLAGS(obj), ITEM_WAS_DISARMED);

    return retval;
}

ACMD(do_conceal) {
    ObjData *obj;
    CharData *tch;
    extern void appear(CharData * ch);

    one_argument(argument, arg);
    if (CONFUSED(ch)) {
        char_printf(ch, "You're too confused to hide things.\n");
        return;
    }

    if (!GET_SKILL(ch, SKILL_CONCEAL))
        char_printf(ch, "You aren't skilled enough to conceal an item.\n");
    else if (!*arg)
        char_printf(ch, "What do you want to conceal?\n");
    else if (!strcasecmp(arg, "all") || !strncasecmp(arg, "all.", 4))
        char_printf(ch, "You can't conceal multiple items at once.\n");
    else if (ch->in_room == NOWHERE ||
             !(obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, arg))))
        char_printf(ch, "You don't see that here.\n");
    else if (!CAN_WEAR(obj, ITEM_WEAR_TAKE) && GET_LEVEL(ch) < LVL_IMMORT) {
        act("You can't seem to shift $p's position.", false, ch, obj, 0, TO_CHAR);
        act("$n tugs at $p, unable to move it.", true, ch, obj, 0, TO_ROOM);
    } else if (GET_OBJ_EFFECTIVE_WEIGHT(obj) > MAX_CONCEAL_WEIGHT && GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You can't hide something that large!\n");
        act("$n drags $p around, trying to conceal it, but it's just too large.", true, ch, obj, 0, TO_ROOM);
    } else if (IS_WATER(IN_ROOM(ch)) && GET_LEVEL(ch) < LVL_IMMORT)
        char_printf(ch, "There's nowhere to hide it!\n");
    else if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, VAL_LIGHT_LIT))
        act("You should probably $T the $o before attempting to conceal it.", false, ch, obj,
            GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT ? "deactivate" : "extinguish", TO_CHAR);
    else if (check_get_disarmed_obj(ch, obj->last_to_hold, obj))
        act("You won't be able to conceal $N's $p!", false, ch, obj, obj->last_to_hold, TO_CHAR);
    else {
        int roll = conceal_roll(ch, obj);

        if (roll) {
            if (GET_OBJ_HIDDENNESS(obj)) {
                if (IS_FOREST(ch->in_room))
                    act("You try to conceal $p under another bush.", false, ch, obj, 0, TO_CHAR);
                else if (CH_OUTSIDE(ch))
                    act("You surreptitiously move $p to another hiding spot.", false, ch, obj, 0, TO_CHAR);
                else
                    act("You furtively switch $p's hiding spot.", false, ch, obj, 0, TO_CHAR);
            } else {
                if (IS_FOREST(ch->in_room))
                    act("You steathily conceal $p under some bushes.", false, ch, obj, 0, TO_CHAR);
                else if (CH_OUTSIDE(ch))
                    act("You surreptitiously cover up $p.", false, ch, obj, 0, TO_CHAR);
                else
                    act("You furtively conceal $p in a corner.", false, ch, obj, 0, TO_CHAR);
            }
            LOOP_THRU_PEOPLE(tch, ch) {
                if (tch == ch)
                    continue;
                if (!CAN_SEE(tch, ch))
                    continue;
                if (CAN_SEE(ch, tch) ? (GET_PERCEPTION(tch) < roll - 50 + random_number(0, 50))
                                     : (GET_PERCEPTION(tch) < roll / 2))
                    continue;
                if (GET_OBJ_HIDDENNESS(obj))
                    act("You notice $n trying to move $p!", false, ch, obj, tch, TO_VICT);
                else
                    act("You spot $n concealing $p!", false, ch, obj, tch, TO_VICT);
            }
            GET_OBJ_HIDDENNESS(obj) = roll;
            obj->last_to_hold = ch;
        }

        /* Failure. orz */
        else {
            if (random_number(0, 100) <= 30) {
                if (EFF_FLAGGED(ch, EFF_INVISIBLE))
                    appear(ch);
                else if (IS_HIDDEN(ch))
                    GET_HIDDENNESS(ch) = std::max(0l, GET_HIDDENNESS(ch) - 100);
            }
            if (IS_FOREST(ch->in_room)) {
                act("You drag $p under some bushes, but they don't quite cover it.", false, ch, obj, 0, TO_CHAR);
                act("$n tries to hide $p under some bushes, but they don't quite cover "
                    "it.",
                    true, ch, obj, 0, TO_ROOM);
            } else {
                act("You try to conceal $p, but can't find a good spot.", false, ch, obj, 0, TO_CHAR);
                act("$n clumsily tries to conceal $p, botching the attempt.", true, ch, obj, 0, TO_ROOM);
            }
        }
        improve_skill(ch, SKILL_CONCEAL);
        WAIT_STATE(ch, PULSE_VIOLENCE);
    }
}

ACMD(do_touch) {
    ObjData *obj;

    one_argument(argument, arg);

    if (!*arg)
        char_printf(ch, "Touch what?\n");
    else if (!strcasecmp(arg, "all") || !strncasecmp(arg, "all.", 4))
        char_printf(ch, "One at a time...\n");
    else if (ch->in_room == NOWHERE ||
             !(obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, arg))))
        char_printf(ch, "You don't see that here.\n");
    else {
        act("$n touches $p.", true, ch, obj, 0, TO_ROOM);
        if (GET_OBJ_TYPE(obj) == ITEM_TOUCHSTONE && !IS_NPC(ch)) {
            act("As you touch $p, you feel a magical link form.", false, ch, obj, 0, TO_CHAR);
            GET_HOMEROOM(ch) = world[ch->in_room].vnum;
        } else {
            act("You touch $p.  Nothing happens.", false, ch, obj, 0, TO_CHAR);
        }
    }
}

#define SWAP(a, b)                                                                                                     \
    {                                                                                                                  \
        temp = a;                                                                                                      \
        a = b;                                                                                                         \
        b = temp;                                                                                                      \
    }
ACMD(do_compare) {
    ObjData *obj1, *obj2, *temp;

    two_arguments(argument, buf1, buf2);

    if (!*buf1 || !*buf2)
        char_printf(ch, "Compare what?\n");
    else if (!strcasecmp(buf1, "all") || !strncasecmp(buf1, "all.", 4) || !strcasecmp(buf2, "all") ||
             !strncasecmp(buf2, "all.", 4))
        char_printf(ch, "You can only compare two items at a time!\n");
    else if (!(obj1 = find_obj_in_list(ch->carrying, find_vis_by_name(ch, buf1))))
        char_printf(ch, "You don't have a {}.\n", buf1);
    else if (!(obj2 = find_obj_in_list(ch->carrying, find_vis_by_name(ch, buf2))))
        char_printf(ch, "You don't have a {}.\n", buf2);
    else if (obj1 == obj2)
        char_printf(ch, "They're the same item!\n");
    else if (GET_OBJ_TYPE(obj1) != GET_OBJ_TYPE(obj2))
        act("The $o and $O have nothing in common.", false, ch, obj1, obj2, TO_CHAR);
    else
        switch (GET_OBJ_TYPE(obj1)) {
        case ITEM_LIGHT:
            if (GET_OBJ_VAL(obj1, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT ||
                GET_OBJ_VAL(obj2, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT)
                char_printf(ch, "You can't tell much about the two items.\n");
            else if (GET_OBJ_VAL(obj1, VAL_LIGHT_REMAINING) == GET_OBJ_VAL(obj2, VAL_LIGHT_REMAINING))
                char_printf(ch, "They look to have the same amount of fuel.\n");
            else {
                if (GET_OBJ_VAL(obj1, VAL_LIGHT_REMAINING) < GET_OBJ_VAL(obj2, VAL_LIGHT_REMAINING))
                    SWAP(obj1, obj2);
                act("$p looks to have more fuel than $P.", false, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_WEAPON:
            if ((IS_WEAPON_SLASHING(obj1) && !IS_WEAPON_SLASHING(obj2)) ||
                (IS_WEAPON_PIERCING(obj1) && !IS_WEAPON_PIERCING(obj2)) ||
                (IS_WEAPON_CRUSHING(obj1) && !IS_WEAPON_CRUSHING(obj2)))
                char_printf(ch, "These weapons are too different to realistically compare them!\n");
            else if (WEAPON_AVERAGE(obj1) == WEAPON_AVERAGE(obj2))
                char_printf(ch, "They each look about as dangerous as the other.\n");
            else {
                if (WEAPON_AVERAGE(obj1) < WEAPON_AVERAGE(obj2))
                    SWAP(obj1, obj2);
                act("$p looks more dangerous than $P.", false, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
        case ITEM_INSTRUMENT:
            if (!EFF_FLAGGED(ch, EFF_DETECT_MAGIC))
                char_printf(ch, "You can't tell anything about either item.\n");
            else if (GET_OBJ_VAL(obj1, VAL_WAND_CHARGES_LEFT) == GET_OBJ_VAL(obj2, VAL_WAND_CHARGES_LEFT))
                char_printf(ch, "They seem to each hold equal power.\n");
            else {
                if (GET_OBJ_VAL(obj1, VAL_WAND_CHARGES_LEFT) < GET_OBJ_VAL(obj2, VAL_WAND_CHARGES_LEFT))
                    SWAP(obj1, obj2);
                act("$p seems to have more charges than $P.", false, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_MONEY:
            if (GET_OBJ_COST(obj1) == GET_OBJ_COST(obj2))
                char_printf(ch, "They look equally valuable.\n");
            else {
                if (GET_OBJ_COST(obj1) < GET_OBJ_COST(obj2))
                    SWAP(obj1, obj2);
                act("$p looks more valuable than $P.", false, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_ARMOR:
        case ITEM_TREASURE:
            if (GET_OBJ_VAL(obj1, VAL_ARMOR_AC) == GET_OBJ_VAL(obj2, VAL_ARMOR_AC))
                char_printf(ch, "They look like they offer similar protection.\n");
            else {
                if (GET_OBJ_VAL(obj1, VAL_ARMOR_AC) < GET_OBJ_VAL(obj2, VAL_ARMOR_AC))
                    SWAP(obj1, obj2);
                act("$p looks more protective than $P.", false, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_CONTAINER:
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
            if (GET_OBJ_VAL(obj1, VAL_CONTAINER_CAPACITY) == GET_OBJ_VAL(obj2, VAL_CONTAINER_CAPACITY))
                char_printf(ch, "They appear to be very similar in size.\n");
            else {
                if (GET_OBJ_VAL(obj1, VAL_CONTAINER_CAPACITY) < GET_OBJ_VAL(obj2, VAL_CONTAINER_CAPACITY))
                    SWAP(obj1, obj2);
                act("$p looks larger than $P.", false, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_FOOD:
            if (GET_OBJ_VAL(obj1, VAL_FOOD_FILLINGNESS) == GET_OBJ_VAL(obj2, VAL_FOOD_FILLINGNESS))
                char_printf(ch, "They look equally filling.\n");
            else {
                if (GET_OBJ_VAL(obj1, VAL_FOOD_FILLINGNESS) < GET_OBJ_VAL(obj2, VAL_FOOD_FILLINGNESS))
                    SWAP(obj1, obj2);
                act("$p looks more filling than $P.", false, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_WORN:
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_OTHER:
        case ITEM_TRASH:
        case ITEM_TRAP:
        case ITEM_NOTE:
        case ITEM_KEY:
        case ITEM_PEN:
        case ITEM_BOAT:
        case ITEM_PORTAL:
        case ITEM_ROPE:
        case ITEM_SPELLBOOK:
        case ITEM_WALL:
        case ITEM_FIREWEAPON:
        case ITEM_TOUCHSTONE:
        default:
            char_printf(ch, "You can't decide which is better.\n");
            break;
        }
}

ACMD(do_create) {
    int r_num, i = 0, found = 0;
    ObjData *cobj;

    if (GET_RACE(ch) == RACE_GNOME) {
        if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC)) {
            if (!GET_COOLDOWN(ch, CD_INNATE_CREATE)) {
                if (!ch)
                    return;

                one_argument(argument, arg);

                if (!*(arg)) {
                    char_printf(ch, "What are you trying to create?\n");
                    return;
                }

                half_chop(arg, buf, buf2);
                while (*minor_creation_items[i] != '\n') {
                    if (is_abbrev(arg, minor_creation_items[i])) {
                        found = 1;
                        break;
                    } else
                        i++;
                }
                if (found) {
                    if ((r_num = real_object(1000 + i)) < 0) {
                        log("SYSERR: Error in function do_minor_creation: target item not found.");
                        char_printf(ch, "Something is wrong with minor create.  Please tell a god.\n");
                        return;
                    }
                    cobj = read_object(r_num, REAL);
                    if (GET_OBJ_TYPE(cobj) == ITEM_LIGHT) {
                        GET_OBJ_VAL(cobj, VAL_LIGHT_LIT) = true;
                    }
                    obj_to_room(cobj, ch->in_room);
                    act("$n &0&5tinkers with some spare materials...&0", true, ch, 0, 0, TO_ROOM);
                    act("&7$n has created&0 $p&7!&0", false, ch, cobj, 0, TO_ROOM);
                    act("&7You tinker with some spare materials.&0", false, ch, cobj, 0, TO_CHAR);
                    act("&7You have created $p.&0", false, ch, cobj, 0, TO_CHAR);

                    SET_COOLDOWN(ch, CD_INNATE_CREATE, 1 MUD_HR);
                } else {
                    char_printf(ch, "You have no idea how to create such an item.\n");
                    return;
                }
            } else {
                char_printf(ch, "You need to regain your focus.\n");
                char_printf(ch, "You can create again in {:d} seconds.\n", (GET_COOLDOWN(ch, CD_INNATE_CREATE) / 10));
                return;
            }
        } else
            char_printf(ch, "Your magical tinkering fizzles and fails.\n");
        return;
    } else
        char_printf(ch, "Only Gnomes can create without spells!\n");
    return;
}
