/***************************************************************************
 * $Id: act.item.c,v 1.170 2010/06/20 19:53:47 mud Exp $
 ***************************************************************************/
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

#include "board.h"
#include "chars.h"
#include "comm.h"
#include "composition.h"
#include "conf.h"
#include "constants.h"
#include "db.h"
#include "dg_scripts.h"
#include "handler.h"
#include "interpreter.h"
#include "limits.h"
#include "math.h"
#include "money.h"
#include "pfiles.h"
#include "races.h"
#include "regen.h"
#include "screen.h"
#include "shop.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

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
int conceal_roll(struct char_data *ch, struct obj_data *obj) {
    int skill = GET_SKILL(ch, SKILL_CONCEAL);
    int lower_bound = -0.0008 * pow(skill, 3) + 0.1668 * pow(skill, 2) - 3.225 * skill;
    int upper_bound = 2000 * skill / (3 * GET_DEX(ch) + GET_INT(ch));
    int roll = number(lower_bound, upper_bound) + dex_app_skill[GET_DEX(ch)].hide;

    /* You can't conceal/palm/stow an item higher than your level. */
    if (GET_OBJ_LEVEL(obj) > GET_LEVEL(ch))
        return 0;

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        /* Modify roll by weight. */
        roll *= (MAX_CONCEAL_WEIGHT - GET_OBJ_WEIGHT(obj)) / ((float)MAX_CONCEAL_WEIGHT);

        /* It is more difficult to conceal items of a level closer to your own. */
        roll -= GET_OBJ_LEVEL(obj) - GET_LEVEL(ch);
    }

    return MAX(0, roll);
}

void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont) {
    if (!drop_otrigger(obj, ch))
        return;
    if (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY))
        act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
    else if (OBJ_FLAGGED(obj, ITEM_NODROP))
        act("You can't put $p in $P - it must be CURSED!", FALSE, ch, obj, cont, TO_CHAR);
    else {
        if (!RIGID(ch) && cont->carried_by != ch)
            act("$p becomes solid again as it leaves your grasp.", FALSE, ch, obj, 0, TO_CHAR);
        obj_from_char(obj);
        obj_to_obj(obj, cont);
        /* When you put a nonpermanent light in a container, it is
         * extinguished. */
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, VAL_LIGHT_LIT) &&
            GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) >= 0) {
            GET_OBJ_VAL(obj, 0) = 0;
            /* obj_from_char takes care of decrementing room light */
            act("You extinguish $p and put it in $P.", FALSE, ch, obj, cont, TO_CHAR);
            act("$n extinguishes $p and puts it in $P.", TRUE, ch, obj, cont, TO_ROOM);
        } else {
            act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
            act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);
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
    struct obj_data *obj, *next_obj, *cont;
    struct char_data *tmp_char;
    int obj_dotmode, cont_dotmode, found = 0;

    two_arguments(argument, arg1, arg2);
    obj_dotmode = find_all_dots(&arg1);
    cont_dotmode = find_all_dots(&arg2);

    if (!*arg1)
        send_to_char("Put what in what?\r\n", ch);
    else if (cont_dotmode != FIND_INDIV)
        send_to_char("You can only put things into one container at a time.\r\n", ch);
    else if (!*arg2)
        cprintf(ch, "What do you want to put %s in?\r\n", ((obj_dotmode == FIND_INDIV) ? "it" : "them"));
    else if (CONFUSED(ch))
        send_to_char("You're too confused.\r\n", ch);
    else {
        generic_find(arg2, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
        if (!cont)
            cprintf(ch, "You don't see %s %s here.\r\n", AN(arg2), arg2);
        else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
            act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
        else if (IS_SET(GET_OBJ_VAL(cont, VAL_CONTAINER_BITS), CONT_CLOSED))
            send_to_char("You'd better open it first!\r\n", ch);
        else {
            if (obj_dotmode == FIND_INDIV) { /* put <obj> <container> */
                if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1))))
                    cprintf(ch, "You aren't carrying %s %s.\r\n", AN(arg1), arg1);
                else if (obj == cont)
                    send_to_char("You attempt to fold it into itself, but fail.\r\n", ch);
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
                        send_to_char("You don't seem to have anything to put in it.\r\n", ch);
                    else
                        cprintf(ch, "You don't seem to have any %s%s.\r\n", arg1, isplural(arg1) ? "" : "s");
                }
            }
        }
    }
}

ACMD(do_stow) {
    char argbuf1[MAX_INPUT_LENGTH], *arg1 = argbuf1;
    char argbuf2[MAX_INPUT_LENGTH], *arg2 = argbuf2;
    struct obj_data *obj, *cont = NULL;
    struct char_data *tch;
    int obj_dotmode, cont_dotmode;

    ACMD(do_drop);

    if (CONFUSED(ch)) {
        send_to_char("You are too confused.\r\n", ch);
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
        send_to_char("Stow what in what?\r\n", ch);
    else if (obj_dotmode != FIND_INDIV)
        send_to_char("You can only stow one item at a time.\r\n", ch);
    else if (cont_dotmode != FIND_INDIV)
        send_to_char("You'll need to break it into pieces to put it into more than "
                     "one container!\r\n",
                     ch);
    else {
        if (*arg2)
            generic_find(arg2, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tch, &cont);
        if (*arg2 && !cont)
            cprintf(ch, "You don't see %s %s here.\r\n", AN(arg2), arg2);
        else if (cont && GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
            act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
        else if (cont && IS_SET(GET_OBJ_VAL(cont, VAL_CONTAINER_BITS), CONT_CLOSED))
            send_to_char("You'd better open it first!\r\n", ch);
        else {
            if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1))))
                cprintf(ch, "You aren't carrying %s %s.\r\n", AN(arg1), arg1);
            else if (obj == cont)
                send_to_char("You attempt to fold it into itself, but fail.\r\n", ch);
            else if (!drop_otrigger(obj, ch))
                return;
            else if (OBJ_FLAGGED(obj, ITEM_NODROP))
                act("You can't stow $p in $P because it's CURSED!", FALSE, ch, obj, cont, TO_CHAR);
            else if (cont && GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY))
                act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
            else {
                int roll = conceal_roll(ch, obj);
                obj_from_char(obj);
                if (!*arg2) {
                    obj_to_room(obj, ch->in_room);
                    act("You sneakily toss $p by your feet.", FALSE, ch, obj, 0, TO_CHAR);
                } else {
                    obj_to_obj(obj, cont);
                    /* When you stow a nonpermanent light in a container, it is
                     * automatically extinguished. */
                    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, VAL_LIGHT_LIT) &&
                        GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) >= 0) {
                        GET_OBJ_VAL(obj, VAL_LIGHT_LIT) = 0;
                        roll -= 20;
                        /* obj_from_char takes care of decrementing room light */
                        act("You extinguish $p and slip it inside $P.", FALSE, ch, obj, cont, TO_CHAR);
                    } else
                        act("You furtively slip $p inside $P.", FALSE, ch, obj, cont, TO_CHAR);
                }
                LOOP_THRU_PEOPLE(tch, ch) {
                    if (tch == ch)
                        continue;
                    if (!CAN_SEE(tch, ch))
                        continue;
                    if (CAN_SEE(ch, tch) ? (GET_PERCEPTION(tch) < roll - 50 + number(0, 50))
                                         : (GET_PERCEPTION(tch) < roll / 2))
                        continue;
                    if (!*arg2)
                        act("$n tosses $p by $s feet.", FALSE, ch, obj, tch, TO_VICT);
                    else {
                        sprintf(buf, "%s looks around, furtively slipping $p into $P.", GET_NAME(ch));
                        act(buf, TRUE, tch, obj, cont, TO_CHAR);
                    }
                }
                WAIT_STATE(ch, PULSE_VIOLENCE);
                improve_skill(ch, SKILL_CONCEAL);
            }
        }
    }
}

void perform_drop(struct char_data *ch, struct obj_data *obj, byte mode, const char *verb) {
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
        act(buf, FALSE, ch, obj, 0, TO_CHAR);
        return;
    }

    if (OBJ_FLAGGED(obj, ITEM_NOFALL))
        sname = "release";

    /* Fiery does not use 'donate', and 'junk' should not cause a drop
     * trigger to go off. */

    if (mode == SCMD_DROP || mode == SCMD_LETDROP) {
        if (!drop_otrigger(obj, ch))
            return;
        if (!drop_wtrigger(obj, ch))
            return;
    }

    if (mode != SCMD_LETDROP) {
        sprintf(buf, "You %s $p.", sname);
        act(buf, FALSE, ch, obj, 0, TO_CHAR);
        sprintf(buf, "$n %ss $p.", sname);
        act(buf, !HIGHLY_VISIBLE(obj) || GET_INVIS_LEV(ch), ch, obj, 0, TO_ROOM);
    }

    if (mode != SCMD_JUNK && !RIGID(ch))
        act("$p becomes solid again as it leaves your grasp.", FALSE, ch, obj, 0, TO_CHAR);
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

struct obj_data *random_inventory_object(struct char_data *ch) {
    struct obj_data *obj, *chosen = NULL;
    int count = 0;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (CAN_SEE_OBJ(ch, obj)) {
            if (chosen == NULL || number(0, count) == 0)
                chosen = obj;
            count++;
        }
    }

    return chosen;
}

struct obj_data *confused_inventory_switch(struct char_data *ch, struct obj_data *obj) {
    struct obj_data *chosen = obj;

    if (CONFUSED(ch) && number(0, 1) == 0) {
        chosen = random_inventory_object(ch);
        if (chosen && obj != chosen) {
            act("&5Fumbling about, you grasp $p&0&5...&0", FALSE, ch, chosen, 0, TO_CHAR);
            return chosen;
        }
    }

    return obj;
}

void drop_random_object(struct char_data *ch) {
    struct obj_data *obj;
    if ((obj = random_inventory_object(ch)))
        perform_drop(ch, obj, SCMD_DROP, "drop");
}

ACMD(do_drop) {
    struct obj_data *obj, *next_obj;
    int dotmode, amount, type, total;
    const char *cmdname = "drop";
    int coins[4] = {0, 0, 0, 0};
    struct find_context context;
    char *name = arg;

    if (subcmd == SCMD_JUNK) {
        cmdname = "junk";
        if (CONFUSED(ch)) {
            send_to_char("That's impossible in your confused state.\r\n", ch);
            return;
        }
    }

    if (parse_money(&argument, coins)) {
        if (!CASH_VALUE(coins)) {
            send_to_char("You drop 0 coins.  Okaaayy...\r\n", ch);
            return;
        }
        for (type = 0; type < NUM_COIN_TYPES; ++type)
            if (GET_COINS(ch)[type] < coins[type]) {
                cprintf(ch, "You don't have enough %s!\r\n", COIN_NAME(type));
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
        cprintf(ch, "What do you want to %s?\r\n", cmdname);
        return;
    }

    dotmode = find_all_dots(&name);

    if (dotmode == FIND_ALL) {
        if (subcmd == SCMD_JUNK)
            send_to_char("Go to the dump if you want to junk EVERYTHING!\r\n", ch);
        else if (!ch->carrying)
            send_to_char("You don't seem to be carrying anything.\r\n", ch);
        else
            for (obj = ch->carrying; obj; obj = next_obj) {
                next_obj = obj->next_content;
                perform_drop(ch, obj, subcmd, cmdname);
            }
    } else if (dotmode == FIND_ALLDOT) {
        context = find_vis_by_name(ch, name);
        if (!*name)
            cprintf(ch, "What do you want to %s all of?\r\n", cmdname);
        else if (!(obj = find_obj_in_list(ch->carrying, context)))
            cprintf(ch, "You don't seem to have any %s%s.\r\n", name, isplural(name) ? "" : "s");
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
            send_to_char("So...you don't want to drop anything?\r\n", ch);
        else if (!(obj = find_obj_in_list(ch->carrying, context)))
            cprintf(ch, "You don't seem to have %s %s%s.\r\n", amount == 1 ? AN(name) : "any", arg,
                    amount == 1 || isplural(name) ? "" : "s");
        else {
            total = amount;

            while (obj && amount > 0) {
                next_obj = find_obj_in_list(obj->next_content, context);
                --amount;
                if (CONFUSED(ch) && number(0, 1) == 0) {
                    drop_random_object(ch);
                    /* Cannot continue loop; drop_random_object may drop next_obj */
                    break;
                } else
                    perform_drop(ch, obj, subcmd, cmdname);
                obj = next_obj;
            }

            if (!CONFUSED(ch) && amount)
                cprintf(ch, "You only had %d %s%s.\r\n", total - amount, name,
                        isplural(name) || total - amount == 1 ? "" : "s");
        }
    }
}

int check_container_give(struct obj_data *obj, struct char_data *ch, struct char_data *vict) {
    struct obj_data *cont;
    int retval = 0;

    if (obj->contains) {
        cont = obj->contains;
        while (cont) {
            if (GET_OBJ_LEVEL(cont) > GET_LEVEL(vict)) {
                sprintf(buf, "%s isn't experienced enough to use $p that is in $P.", GET_NAME(vict));
                act(buf, FALSE, ch, cont, obj, TO_CHAR);
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

int perform_give(struct char_data *ch, struct char_data *vict, struct obj_data *obj, int silent) {
    if (OBJ_FLAGGED(obj, ITEM_NODROP) && GET_LEVEL(ch) < 100 &&
        !(IS_NPC(ch) && (!(ch)->desc || GET_LEVEL(POSSESSOR(ch)) >= LVL_IMPL))) {
        act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
        return GIVE_FAIL;
    }
    if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
        act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
        return GIVE_FAIL_FULL;
    }
    if (!ADDED_WEIGHT_OK(vict, obj)) {
        act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
        return GIVE_FAIL;
    }
    if (ADDED_WEIGHT_REFUSED(vict, obj)) {
        act("$E doesn't look like $E could handle the additional weight.", FALSE, ch, 0, vict, TO_CHAR);
        return GIVE_FAIL;
    }
    if (GET_OBJ_LEVEL(obj) > GET_LEVEL(vict)) {
        act("$E isn't experienced enough to use $p.", FALSE, ch, obj, vict, TO_CHAR);
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
        act("$p becomes solid again as it leaves your grasp.", FALSE, ch, obj, 0, TO_CHAR);

    /* When you give items to an illusory or fluid mob, they fall to the ground.
     */

    if (SOLIDCHAR(ch) && !SOLIDCHAR(vict) && GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(vict) < LVL_IMMORT) {
        /* Note that "silent" only applies to success messages. */
        act("$n gives you $p, but you can't hold onto it.", FALSE, ch, obj, vict, TO_VICT);
        if (IS_SPLASHY(IN_ROOM(vict))) {
            act("You hand $p to $N, but it simply falls into the water.", FALSE, ch, obj, vict, TO_CHAR);
            act("$n gives $p to $N, but it falls into the water.", !HIGHLY_VISIBLE(obj) || GET_INVIS_LEV(ch), ch, obj,
                vict, TO_NOTVICT);
        } else {
            act("You hand $p to $N, but it simply falls to the ground.", FALSE, ch, obj, vict, TO_CHAR);
            act("$n gives $p to $N, but it falls to the ground.", !HIGHLY_VISIBLE(obj) || GET_INVIS_LEV(ch), ch, obj,
                vict, TO_NOTVICT);
        }
        perform_drop(ch, obj, SCMD_LETDROP, "release");
        return GIVE_FAIL;
    }

    obj_from_char(obj);
    obj_to_char(obj, vict);

    if (!silent) {
        act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
        act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
        act("$n gives $p to $N.", !HIGHLY_VISIBLE(obj) || GET_INVIS_LEV(ch), ch, obj, vict, TO_NOTVICT);
    }

    return GIVE_SUCCESS;
}

/* utility function for give */
struct char_data *give_find_vict(struct char_data *ch, char *arg) {
    struct char_data *vict;

    if (!*arg) {
        send_to_char("To who?\r\n", ch);
        return NULL;
    } else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        send_to_char(NOPERSON, ch);
        return NULL;
    } else if (vict == ch) {
        send_to_char("What's the point of that?\r\n", ch);
        return NULL;
    } else
        return vict;
}

void perform_give_money(struct char_data *ch, struct char_data *vict, int coins[]) {
    bool afford = TRUE;
    int amount = 0, i;
    struct obj_data *obj;

    for (i = 0; i <= 3; i++)
        amount = amount + coins[i];

    if (amount <= 0) {
        send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
        return;
    }

    if (GET_PLATINUM(ch) < coins[PLATINUM])
        afford = FALSE;
    else if (GET_GOLD(ch) < coins[GOLD])
        afford = FALSE;
    else if (GET_SILVER(ch) < coins[SILVER])
        afford = FALSE;
    else if (GET_COPPER(ch) < coins[COPPER])
        afford = FALSE;

    if (!afford) {
        send_to_char("You don't have that many coins!\r\n", ch);
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
        act("The coins seem to fall right through $n's hands!", FALSE, vict, 0, ch, TO_VICT);
        if (IS_SPLASHY(IN_ROOM(vict))) {
            act("$n tries to give some coins to $N, but they fell into the water!", TRUE, ch, 0, vict, TO_NOTVICT);
        } else {
            act("$n tries to give some coins to $N, but they fell to the ground!", TRUE, ch, 0, vict, TO_NOTVICT);
        }
        obj_to_room(obj, vict->in_room);
        return;
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        send_to_char(OK, ch);
    else {
        strcpy(buf, "You give $n ");
        statemoney(buf + strlen(buf), coins);
        strcat(buf, ".");
        act(buf, FALSE, vict, 0, ch, TO_VICT);
    }

    strcpy(buf, "$n gives you ");
    statemoney(buf + strlen(buf), coins);
    strcat(buf, ".");
    act(buf, FALSE, ch, 0, vict, TO_VICT);

    act("$n gives some coins to $N.", TRUE, ch, 0, vict, TO_NOTVICT);

    GET_PLATINUM(vict) += coins[PLATINUM];
    GET_GOLD(vict) += coins[GOLD];
    GET_SILVER(vict) += coins[SILVER];
    GET_COPPER(vict) += coins[COPPER];

    bribe_mtrigger(vict, ch, coins);

    if (coins[PLATINUM] > 50 || coins[GOLD] > 500) {
        sprintf(buf, "%s gave %d Plat %d Gold to %s", GET_NAME(ch), coins[PLATINUM], coins[GOLD], GET_NAME(vict));
        mudlog(buf, CMP, LVL_GOD, TRUE);
    }
}

ACMD(do_give) {
    int amount = 1, dotmode, result = GIVE_FAIL_DONTHAVE, i, counter = 0;
    struct char_data *vict;
    struct obj_data *obj, *next_obj, *ref_obj = NULL;
    int cash[NUM_COIN_TYPES] = {0};
    char *name = arg;

    if (parse_money(&argument, cash)) {
        one_argument(argument, name);
        if (!(vict = give_find_vict(ch, name)))
            return;
        perform_give_money(ch, vict, cash);
        return;
    }

    argument = one_argument(argument, name);

    if (!*name) {
        send_to_char("Give what to who?\r\n", ch);
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
        act("You give nothing to $N.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    if (amount > 1 && dotmode != FIND_INDIV) {
        cprintf(ch, "Do you want to give '%d' or 'all'?  Make up your mind!\r\n", amount);
        return;
    }

    if (dotmode == FIND_INDIV) {
        for (i = 0; i < amount; ++i) {
            if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, name)))) {
                if (amount == 1)
                    cprintf(ch, "You don't seem to have a %s.\r\n", name);
                else
                    cprintf(ch, "You don't seem to have %d %ss.\r\n", amount, name);
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
        }
    } else if (CONFUSED(ch)) {
        send_to_char("You are too confused for such juggling.\r\n", ch);
        return;
    } else {
        if (dotmode == FIND_ALLDOT && !*name) {
            send_to_char("All of what?\r\n", ch);
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
            }
        }
    }

    if (counter == 0) {
        if (result == GIVE_FAIL_DONTHAVE) {
            if (dotmode == FIND_ALLDOT)
                cprintf(ch, "You don't even have one %s!\r\n", name);
            else if (dotmode == FIND_ALL)
                send_to_char("You don't seem to be holding anything.\r\n", ch);
        }
    } else if (counter == 1) {
        act("You give $p to $N.", FALSE, ch, ref_obj, vict, TO_CHAR);
        act("$n gives you $p.", FALSE, ch, ref_obj, vict, TO_VICT);
        act("$n gives $p to $N.", !HIGHLY_VISIBLE(ref_obj) || GET_INVIS_LEV(ch), ch, ref_obj, vict, TO_NOTVICT);
    } else if (counter > 1) {
        sprintf(buf, "You give $p to $N. (x%d)", counter);
        act(buf, FALSE, ch, ref_obj, vict, TO_CHAR);
        sprintf(buf, "$n gives you $p. (x%d)", counter);
        act(buf, FALSE, ch, ref_obj, vict, TO_VICT);
        act("$n gives $p to $N. (multiple)", !HIGHLY_VISIBLE(ref_obj) || GET_INVIS_LEV(ch), ch, ref_obj, vict,
            TO_NOTVICT);
    }
}

ACMD(do_drink) {
    struct obj_data *temp;
    struct effect eff;
    int amount; /* ounces */
    int on_ground = 0;

    if (FIGHTING(ch)) {
        send_to_char("You are afraid to try in combat!\r\n", ch);
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char("Drink from what?\r\n", ch);
        return;
    }
    if (!(temp = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg)))) {
        if (!(temp = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, arg)))) {
            act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
            return;
        } else
            on_ground = 1;
    }
    if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) && (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
        send_to_char("You can't drink from that!\r\n", ch);
        return;
    }
    if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
        send_to_char("You have to be holding that to drink from it.\r\n", ch);
        return;
    }
    if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
        /* The pig is drunk */
        send_to_char("You can't seem to get close enough to your mouth.\r\n", ch);
        act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
        return;
    }
    if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 5)) {
        send_to_char("Your stomach can't contain any more!\r\n", ch);
        return;
    }
    if (GET_COND(ch, THIRST) > MAX_THIRST - HOURLY_THIRST_CHANGE) {
        send_to_char("You couldn't drink another drop!\r\n", ch);
        return;
    }

    if (!GET_OBJ_VAL(temp, VAL_DRINKCON_REMAINING)) {
        send_to_char("It's empty.\r\n", ch);
        return;
    }

    if (!consume_otrigger(temp, ch, SCMD_DRINK))
        return;

    if (subcmd == SCMD_DRINK) {
        sprintf(buf, "$n drinks %s from $p.", LIQ_NAME(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)));
        act(buf, TRUE, ch, temp, 0, TO_ROOM);

        sprintf(buf, "You drink the %s.\r\n", LIQ_NAME(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)));
        send_to_char(buf, ch);

        /* Let's say you normally drink 1/2 cup (a lot, but hey). */
        amount = 4;
    } else {
        act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
        cprintf(ch, "It tastes like %s.\r\n", LIQ_NAME(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)));
        amount = 1;
    }

    amount = MIN(amount, GET_OBJ_VAL(temp, VAL_DRINKCON_REMAINING));

    /* You can't subtract more than the object weighs */
    liquid_from_container(temp, amount);

    gain_condition(ch, DRUNK, (LIQ_COND(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID), DRUNK) * amount) / 4);

    gain_condition(ch, FULL, (LIQ_COND(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID), FULL) * amount) / 1);

    gain_condition(ch, THIRST, (LIQ_COND(GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID), THIRST) * amount) / 1);

    if (GET_COND(ch, DRUNK) > 10)
        send_to_char("You feel drunk.\r\n", ch);

    if (GET_COND(ch, THIRST) > 20)
        send_to_char("You don't feel thirsty any more.\r\n", ch);

    if (GET_COND(ch, FULL) > 20)
        send_to_char("You are full.\r\n", ch);

    if (IS_POISONED(temp)) {
        send_to_char("Oops, it tasted rather strange!\r\n", ch);
        act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);

        memset(&eff, 0, sizeof(eff));
        eff.type = SPELL_POISON;
        eff.duration = amount * 3;
        eff.modifier = 0;
        eff.location = APPLY_NONE;
        SET_FLAG(eff.flags, EFF_POISON);
        effect_join(ch, &eff, FALSE, FALSE, FALSE, FALSE, TRUE);
    }

    /* This will restore movement points, such that drinking the max
     * would restore 100% or 200, whichever is lower. */
    alter_move(ch, -MIN(200, GET_MAX_MOVE(ch) * amount / MAX_THIRST));

    return;
}

ACMD(do_eat) {
    struct obj_data *food;
    struct effect eff;
    int amount;

    if (FIGHTING(ch)) {
        send_to_char("You are afraid to try in combat!\r\n", ch);
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char("Eat what?\r\n", ch);
        return;
    }
    if (!(food = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg)))) {
        cprintf(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        return;
    }
    if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) || (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
        do_drink(ch, argument, 0, SCMD_SIP);
        return;
    }
    if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && (GET_LEVEL(ch) < LVL_GOD)) {
        send_to_char("You can't eat THAT!\r\n", ch);
        return;
    }
    if (GET_COND(ch, FULL) > 20) { /* Stomach full */
        act("You are too full to eat more!", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!consume_otrigger(food, ch, SCMD_EAT))
        return;

    if (subcmd == SCMD_EAT) {
        act("You eat the $o.", FALSE, ch, food, 0, TO_CHAR);
        act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
    } else {
        act("You nibble a little bit of the $o.", FALSE, ch, food, 0, TO_CHAR);
        act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
    }

    amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, VAL_FOOD_FILLINGNESS) : 1);

    gain_condition(ch, FULL, amount);

    if (GET_COND(ch, FULL) > 20)
        act("You are full.", FALSE, ch, 0, 0, TO_CHAR);

    if (IS_POISONED(food) && (GET_LEVEL(ch) < LVL_IMMORT)) {
        send_to_char("Oops, that tasted rather strange!\r\n", ch);
        act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);

        memset(&eff, 0, sizeof(eff));
        eff.type = SPELL_POISON;
        eff.duration = amount * 2;
        eff.modifier = 0;
        eff.location = APPLY_NONE;
        SET_FLAG(eff.flags, EFF_POISON);
        effect_join(ch, &eff, FALSE, FALSE, FALSE, FALSE, TRUE);
    }
    /* This will restore hit points, such that a meal of size 24
     * would restore 33% or 70, whichever is lower. */
    hurt_char(ch, 0, -MIN(70, GET_MAX_HIT(ch) * amount / 72), TRUE);

    if (subcmd == SCMD_EAT)
        extract_obj(food);
    else {
        if (!(--GET_OBJ_VAL(food, VAL_FOOD_FILLINGNESS))) {
            send_to_char("There's nothing left now.\r\n", ch);
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
    struct obj_data *from_obj = NULL, *to_obj = NULL;
    int amount;

    if (FIGHTING(ch)) {
        send_to_char("You can't coordinate the maneuver while fighting!\r\n", ch);
        return;
    }
    two_arguments(argument, arg1, arg2);

    if (subcmd == SCMD_POUR) {
        if (!*arg1) { /* No arguments */
            act("From what do you want to pour?", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (!(from_obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1)))) {
            act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
            act("You can't pour from that!", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
    }
    if (subcmd == SCMD_FILL) {
        if (!*arg1) { /* no arguments */
            send_to_char("What do you want to fill?  And what are you filling it from?\r\n", ch);
            return;
        }
        if (!(to_obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1)))) {
            send_to_char("You can't find it!\r\n", ch);
            return;
        }
        if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
            act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
            return;
        }
        if (!*arg2) { /* no 2nd argument */
            act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
            return;
        }
        if (!(from_obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, arg2)))) {
            sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
            send_to_char(buf, ch);
            return;
        }
        if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
            act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
            return;
        }
    }
    if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_REMAINING) == 0) {
        act("The $o is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
        return;
    }
    if (subcmd == SCMD_POUR) { /* pour */
        if (!*arg2) {
            act("Where do you want it?  Out or in what?", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (!str_cmp(arg2, "out")) {
            act("$n empties $p.", FALSE, ch, from_obj, 0, TO_ROOM);
            act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);
            liquid_from_container(from_obj, GET_OBJ_VAL(from_obj, VAL_DRINKCON_REMAINING));
            return;
        }
        if (!(to_obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg2)))) {
            act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) && (GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
            act("You can't pour anything into that.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
    }
    if (to_obj == from_obj) {
        act("A most unproductive effort.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    if ((GET_OBJ_VAL(to_obj, VAL_DRINKCON_REMAINING) != 0) &&
        (GET_OBJ_VAL(to_obj, VAL_DRINKCON_LIQUID) != GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID))) {
        act("There is already another liquid in it!", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    if (GET_OBJ_VAL(to_obj, VAL_DRINKCON_REMAINING) >= GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY)) {
        act("There is no room for more.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    if (subcmd == SCMD_POUR)
        cprintf(ch, "You pour the %s into the %s.\r\n", LIQ_NAME(GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID)), arg2);
    if (subcmd == SCMD_FILL) {
        act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
        act("$n gently fills $p from $P.", !HIGHLY_VISIBLE(to_obj) || GET_INVIS_LEV(ch), ch, to_obj, from_obj, TO_ROOM);
    }

    /* how much to pour */
    amount = MIN(GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY) - GET_OBJ_VAL(to_obj, VAL_DRINKCON_REMAINING),
                 GET_OBJ_VAL(from_obj, VAL_DRINKCON_REMAINING));

    liquid_to_container(to_obj, amount, GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID), IS_POISONED(from_obj));

    if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
        liquid_from_container(from_obj, amount);
    }

    return;
}

void wear_message(struct char_data *ch, struct obj_data *obj, int where) {
    char *wear_messages[][2] = {
        {"$n starts using $p as a light.", "You start using $p as a light."},

        {"$n slides $p onto $s right ring finger.", "You slide $p onto your right ring finger."},

        {"$n slides $p onto $s left ring finger.", "You slide $p onto your left ring finger."},

        {"$n wears $p around $s neck.", "You wear $p around your neck."},

        {"$n wears $p around $s neck.", "You wear $p around your neck."},

        {
            "$n wears $p on $s body.",
            "You wear $p on your body.",
        },

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

        {"$n attaches $p to $s belt.", "You attach $p to your belt."}};

    act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
    act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}

/* Returns TRUE if the object was worn. */

bool perform_wear(struct char_data *ch, /* Who is trying to wear something */
                  struct obj_data *obj, /* The object being put on */
                  int where,            /* The positioni it should be worn at */
                  bool collective       /* Whether the character has issued a command
                                           like "wear all" or "wear all.leather". This
                                           parameter will suppress the error message
                                           for objects that can't be worn. */
) {
    if (!may_wear_eq(ch, obj, &where, !collective))
        return FALSE;
    if (!wear_otrigger(obj, ch, where))
        return FALSE;
    wear_message(ch, obj, where);
    obj_from_char(obj);
    equip_char(ch, obj, where);
    return TRUE;
}

int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg) {
    int where = -1;

    static const char *keywords[] = {"light",      "finger",     "!RESERVED!", "neck",       "!RESERVED!", "body",
                                     "head",       "legs",       "feet",       "hands",      "arms",       "shield",
                                     "about",      "waist",      "wrist",      "!RESERVED!", "!RESERVED!", "!RESERVED!",
                                     "!RESERVED!", "!RESERVED!", "!RESERVED!", "eyes",       "face",       "ear",
                                     "!RESERVED!", "badge",      "belt",       "\n"};

    if (!arg || !*arg) {
        /* Allow wearing of light objects in light pos.  Gets overridden
           by any other can wear flags below. - myc 5 Dec 2006 */
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
            where = WEAR_LIGHT;
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
        /* Add Wield and 2 Handed Wield to this search block - DE 8/13/99 */
        if (CAN_WEAR(obj, ITEM_WEAR_WIELD))
            where = WEAR_WIELD;
        if (CAN_WEAR(obj, ITEM_WEAR_2HWIELD))
            where = WEAR_2HWIELD;
    } else {
        /* 2/6/02 - DCE Put in a check for !. Players could wear item !,
           and it would put the item in the light position. */
        if (!strcmp(arg, "!") || (where = search_block(arg, keywords, FALSE)) < 0) {
            sprintf(buf, "'%s'?  What part of your body is THAT?\r\n", arg);
            send_to_char(buf, ch);
        }
    }

    return where;
}

ACMD(do_wear) {
    char argbuf1[MAX_INPUT_LENGTH], *arg1 = argbuf1;
    char argbuf2[MAX_INPUT_LENGTH], *arg2 = argbuf2;
    struct obj_data *obj, *next_obj;
    int where, dotmode, wearable_items = 0, worn_items = 0;

    two_arguments(argument, arg1, arg2);

    if (GET_RACE(ch) == RACE_ANIMAL) {
        return;
    }

    if (!*arg1) {
        send_to_char("Wear what?\r\n", ch);
        return;
    }
    dotmode = find_all_dots(&arg1);

    if (*arg2 && (dotmode != FIND_INDIV)) {
        send_to_char("You can't specify the same body location for more than one item!\r\n", ch);
        return;
    }
    if (dotmode == FIND_ALL) {
        if (CONFUSED(ch)) {
            send_to_char("You're a bit too confused for mass equipment changes.\r\n", ch);
            return;
        }
        for (obj = ch->carrying; obj; obj = next_obj) {
            next_obj = obj->next_content;
            if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
                wearable_items++;
                if (perform_wear(ch, obj, where, TRUE))
                    worn_items++;
            }
        }
        if (!wearable_items || !worn_items)
            send_to_char("You don't have anything you can wear.\r\n", ch);
    }

    else if (dotmode == FIND_ALLDOT) {
        if (!*arg1) {
            send_to_char("Wear all of what?\r\n", ch);
            return;
        }
        if (CONFUSED(ch)) {
            send_to_char("You're a bit too confused for mass equipment changes.\r\n", ch);
            return;
        }
        if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1))))
            cprintf(ch, "You don't seem to have any %s%s.\r\n", arg1, isplural(arg1) ? "" : "s");
        else {
            while (obj) {
                next_obj = find_obj_in_list(obj->next_content, find_vis_by_name(ch, arg1));
                if ((where = find_eq_pos(ch, obj, 0)) >= 0) {
                    if (perform_wear(ch, obj, where, TRUE))
                        worn_items++;
                }
                obj = next_obj;
            }
            if (!worn_items)
                send_to_char("You don't have anything wearable like that.\r\n", ch);
        }
    }

    /* FIND_INDIV */
    else {
        if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1)))) {
            sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
            send_to_char(buf, ch);
            return;
        }
        obj = confused_inventory_switch(ch, obj);
        if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
            perform_wear(ch, obj, where, FALSE);
        else if (!*arg2)
            act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
    }
}

ACMD(do_wield) {
    struct obj_data *obj;
    int hands_used, weapon_hands_used;

    one_argument(argument, arg);

    /* Basic checks first */

    if (GET_RACE(ch) == RACE_ANIMAL)
        return;

    if (!*arg) {
        send_to_char("Wield what?\r\n", ch);
        return;
    }

    if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg)))) {
        sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        send_to_char(buf, ch);
        return;
    }

    if (!CAN_WEAR(obj, ITEM_WEAR_WIELD) && !CAN_WEAR(obj, ITEM_WEAR_2HWIELD)) {
        send_to_char("You can't wield that.\r\n", ch);
        return;
    }

    if (GET_OBJ_WEIGHT(obj) > str_app[GET_STR(ch)].wield_w) {
        send_to_char("It's too heavy for you to use.\r\n", ch);
        return;
    }

    /* By now we know it's a weapon that can be wielded by this char
     * under some circumstances, at least. So check for free hands,
     * dual wielding, and two-handed weapon. */

    count_hand_eq(ch, &hands_used, &weapon_hands_used);

    /* Both hands used? Bye. */
    if (hands_used > 1) {
        send_to_char("Your hands are full!\r\n", ch);
        return;
    }

    /* See if they need both hands free: two-handed weapon and they aren't an
     * ogre. */
    if (CAN_WEAR(obj, ITEM_WEAR_2HWIELD) && GET_CLASS(ch) != RACE_OGRE) {
        if (hands_used) {
            send_to_char("You need both hands for this weapon.\r\n", ch);
            return;
        }
        perform_wear(ch, obj, WEAR_2HWIELD, FALSE);
        return;
    }

    /* This weapon can be wielded one-handed, and ch has at least one hand free.
     */
    if (weapon_hands_used) {
        /* One weapon is already wielded. Got dual wield? */
        if
            GET_SKILL(ch, SKILL_DUAL_WIELD) {
                obj = confused_inventory_switch(ch, obj);
                perform_wear(ch, obj, WEAR_WIELD2, FALSE);
            }
        else
            send_to_char("You don't have the co-ordination to dual wield.\r\n", ch);
    } else {
        obj = confused_inventory_switch(ch, obj);
        perform_wear(ch, obj, WEAR_WIELD, FALSE);
    }
}

static MATCH_OBJ_FUNC(match_light_by_name) {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
        if (isname(context->string, obj->name))
            if (--context->number <= 0)
                return TRUE;
    return FALSE;
}

ACMD(do_light) {
    struct obj_data *obj;
    struct find_context find_darklights;

    one_argument(argument, arg);

    if (!*arg) {
        if (subcmd == SCMD_EXTINGUISH)
            send_to_char("Extinguish what?\r\n", ch);
        else
            send_to_char("Light what?\r\n", ch);
        return;
    }

    /* Lights are searched for:
     * - worn (visible or not)
     * - in inventory (visible first)
     * - in inventory (not visible - allows you to turn on lights in an dark room)
     */

    find_darklights = find_by_name(arg);
    find_darklights.obj_func = match_light_by_name;

    if (!(obj = (find_obj_in_eq(ch, NULL, find_darklights))) &&
        !(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg))) &&
        !(obj = find_obj_in_list(ch->carrying, find_darklights))) {
        cprintf(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        return;
    }

    obj = confused_inventory_switch(ch, obj);

    if (GET_OBJ_TYPE(obj) != ITEM_LIGHT) {
        if (subcmd == SCMD_EXTINGUISH)
            send_to_char("You can't extinguish that!\r\n", ch);
        else
            send_to_char("You can't light that!\r\n", ch);
        return;
    }

    if (!GET_OBJ_VAL(obj, VAL_LIGHT_LIT)) { /* It is not lit */

        if (subcmd == SCMD_EXTINGUISH) {
            send_to_char("It isn't lit.\r\n", ch);
            return;
        }

        if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) != 0) {    /* It is not worn out */
            if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) < 0) { /* It's a permanent light */
                act("You activate $p.", TRUE, ch, obj, 0, TO_CHAR);
                act("$n activates $p.", TRUE, ch, obj, 0, TO_ROOM);
            } else {
                act("You light $p.", TRUE, ch, obj, 0, TO_CHAR);
                act("$n lights $p.", TRUE, ch, obj, 0, TO_ROOM);
            }
            GET_OBJ_VAL(obj, VAL_LIGHT_LIT) = TRUE; /* Now it's lit */
            if (ch->in_room != NOWHERE)
                world[ch->in_room].light++;
            else
                log("SYSERR: do_light - my char_data* object wasn't in a room!");
        } else
            act("Sorry, there's no more power left in $p.", TRUE, ch, obj, 0, TO_CHAR);
    } else {
        /* It was already lit, so we are extinguishing it */
        if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) < 0) { /* It's a permanent light */
            act("You deactivate $p.", TRUE, ch, obj, 0, TO_CHAR);
            act("$n deactivates $p.", TRUE, ch, obj, 0, TO_ROOM);
        } else {
            act("You extinguish $p.", TRUE, ch, obj, 0, TO_CHAR);
            act("$n extinguishes $p.", TRUE, ch, obj, 0, TO_ROOM);
        }
        GET_OBJ_VAL(obj, VAL_LIGHT_LIT) = FALSE;
        if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) != 0) {
            if (ch->in_room != NOWHERE)
                world[ch->in_room].light--;
            else
                log("SYSERR: do_light (extinguishing) - my char_data* object wasn't in "
                    "a room!");
        }
    }
}

ACMD(do_grab) {
    struct obj_data *obj;
    struct find_context find_darklights;

    one_argument(argument, arg);

    find_darklights = find_by_name(arg);
    find_darklights.obj_func = match_light_by_name;

    if (!*arg)
        send_to_char("Hold what?\r\n", ch);
    else if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg))) &&
             !(obj = find_obj_in_list(ch->carrying, find_darklights)))
        cprintf(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    else {
        obj = confused_inventory_switch(ch, obj);
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
            perform_wear(ch, obj, WEAR_HOLD, FALSE);
        else {
            if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND && GET_OBJ_TYPE(obj) != ITEM_STAFF &&
                GET_OBJ_TYPE(obj) != ITEM_SCROLL && GET_OBJ_TYPE(obj) != ITEM_POTION)
                send_to_char("You can't hold that.\r\n", ch);
            else {
                perform_wear(ch, obj, WEAR_HOLD, FALSE);
            }
        }
    }
}

void perform_remove(struct char_data *ch, int pos) {
    struct obj_data *obj;

    if (!(obj = GET_EQ(ch, pos))) {
        log("Error in perform_remove: bad pos passed.");
        return;
    }
    if (!remove_otrigger(obj, ch))
        return;
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
        act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
    else {
        obj_to_char(unequip_char(ch, pos), ch);
        act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
    }
}

ACMD(do_remove) {
    struct obj_data *obj;
    int where, dotmode, found;
    char *name = arg;

    argument = one_argument(argument, name);

    if (!*name) {
        send_to_char("Remove what?\r\n", ch);
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
            send_to_char("You're not using anything.\r\n", ch);
    }

    else if (dotmode == FIND_ALLDOT) {
        if (!*name)
            send_to_char("Remove all of what?\r\n", ch);
        else {
            found = 0;
            for (where = 0; where < NUM_WEARS; ++where)
                if (GET_EQ(ch, where) && isname(name, GET_EQ(ch, where)->name)) {
                    perform_remove(ch, where);
                    found = 1;
                }
            if (!found)
                cprintf(ch, "You don't seem to be using any %s%s.\r\n", name, isplural(name) ? "" : "s");
        }
    }

    else if (is_number(name) &&
             universal_find(find_vis_by_type(ch, ITEM_BOARD), FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_WORLD, NULL,
                            &obj) &&
             obj)
        remove_message(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), atoi(name), obj);
    else if (is_number(argument) &&
             generic_find(name, FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_WORLD, ch, NULL, &obj) && obj &&
             GET_OBJ_TYPE(obj) == ITEM_BOARD)
        remove_message(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), atoi(argument), obj);
    else if (!(obj = (find_obj_in_eq(ch, &where, find_vis_by_name(ch, name)))))
        cprintf(ch, "You don't seem to be using %s %s.\r\n", AN(name), name);
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
        act("$p falls off as you remove your belt.", FALSE, ch, GET_EQ(ch, WEAR_OBELT), 0, TO_CHAR);
        act("$p falls off as $n removes $s belt.", TRUE, ch, GET_EQ(ch, WEAR_OBELT), 0, TO_ROOM);
        obj_to_char(unequip_char(ch, WEAR_OBELT), ch);
    }
}

bool check_get_disarmed_obj(struct char_data *ch, struct char_data *last_to_hold, struct obj_data *obj) {
    int rand;
    struct char_data *tmp_ch;
    bool retval = FALSE;
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
                act("$n nods briefly as $N reaches for $p.", TRUE, last_to_hold, obj, ch, TO_NOTVICT);
                act("You nod briefly as $N reaches for $p.", FALSE, last_to_hold, obj, ch, TO_CHAR);
                act("$n nods briefly as you reach for $p.", TRUE, last_to_hold, obj, ch, TO_VICT);
            } else if (GET_LEVEL(ch) >= LVL_IMMORT && GET_LEVEL(ch) >= GET_LEVEL(last_to_hold)) {
                /* nothing */
            } else if (number(1, 64) > 8) { /* darn, PC failed to grab it. now they pay. */
                rand = number(1, 4);

                if (rand == 1) {
                    act("$n makes a quick grab for $p!", FALSE, ch, obj, 0, TO_ROOM);
                    act("You lunge for $p!", FALSE, ch, obj, 0, TO_CHAR);

                    act("$N tries to block $n, but isn't quick enough.", FALSE, ch, 0, last_to_hold, TO_NOTVICT);
                    act("You try to block $N, but $E's too fast.", FALSE, last_to_hold, 0, ch, TO_CHAR);
                    act("$N tries to block you, but is far too slow.", FALSE, ch, 0, last_to_hold, TO_CHAR);
                } else {
                    act("$n tries to grab $p!", FALSE, ch, obj, 0, TO_ROOM);

                    strcpy(Gbuf4, "No you don't!! That belongs to me!");
                    do_say(last_to_hold, Gbuf4, cmd_say, 0);
                    strcpy(Gbuf4, GET_NAME(ch));
                    do_action(last_to_hold, Gbuf4, cmd_glare, 0);

                    act("$N plants $Mself directly in front of $n, blocking $m.", FALSE, ch, 0, last_to_hold,
                        TO_NOTVICT);
                    act("You plant yourself directly in front of $N, blocking $M.", FALSE, last_to_hold, 0, ch,
                        TO_CHAR);
                    act("$N steps directly in front of your path.  No way to get it now.", FALSE, ch, 0, last_to_hold,
                        TO_CHAR);
                    retval = TRUE;

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
    struct obj_data *obj;
    struct char_data *tch;
    extern void appear(struct char_data * ch);

    one_argument(argument, arg);
    if (CONFUSED(ch)) {
        send_to_char("You're too confused to hide things.\r\n", ch);
        return;
    }

    if (!GET_SKILL(ch, SKILL_CONCEAL))
        send_to_char("You aren't skilled enough to conceal an item.\r\n", ch);
    else if (!*arg)
        send_to_char("What do you want to conceal?\r\n", ch);
    else if (!str_cmp(arg, "all") || !strn_cmp(arg, "all.", 4))
        send_to_char("You can't conceal multiple items at once.\r\n", ch);
    else if (ch->in_room == NOWHERE ||
             !(obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, arg))))
        send_to_char("You don't see that here.\r\n", ch);
    else if (!CAN_WEAR(obj, ITEM_WEAR_TAKE) && GET_LEVEL(ch) < LVL_IMMORT) {
        act("You can't seem to shift $p's position.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n tugs at $p, unable to move it.", TRUE, ch, obj, 0, TO_ROOM);
    } else if (GET_OBJ_WEIGHT(obj) > MAX_CONCEAL_WEIGHT && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("You can't hide something that large!\r\n", ch);
        act("$n drags $p around, trying to conceal it, but it's just too large.", TRUE, ch, obj, 0, TO_ROOM);
    } else if (IS_WATER(IN_ROOM(ch)) && GET_LEVEL(ch) < LVL_IMMORT)
        send_to_char("There's nowhere to hide it!\r\n", ch);
    else if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, VAL_LIGHT_LIT))
        act("You should probably $T the $o before attempting to conceal it.", FALSE, ch, obj,
            GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT ? "deactivate" : "extinguish", TO_CHAR);
    else if (check_get_disarmed_obj(ch, obj->last_to_hold, obj))
        act("You won't be able to conceal $N's $p!", FALSE, ch, obj, obj->last_to_hold, TO_CHAR);
    else {
        int roll = conceal_roll(ch, obj);

        if (roll) {
            if (GET_OBJ_HIDDENNESS(obj)) {
                if (IS_FOREST(ch->in_room))
                    act("You try to conceal $p under another bush.", FALSE, ch, obj, 0, TO_CHAR);
                else if (CH_OUTSIDE(ch))
                    act("You surreptitiously move $p to another hiding spot.", FALSE, ch, obj, 0, TO_CHAR);
                else
                    act("You furtively switch $p's hiding spot.", FALSE, ch, obj, 0, TO_CHAR);
            } else {
                if (IS_FOREST(ch->in_room))
                    act("You steathily conceal $p under some bushes.", FALSE, ch, obj, 0, TO_CHAR);
                else if (CH_OUTSIDE(ch))
                    act("You surreptitiously cover up $p.", FALSE, ch, obj, 0, TO_CHAR);
                else
                    act("You furtively conceal $p in a corner.", FALSE, ch, obj, 0, TO_CHAR);
            }
            LOOP_THRU_PEOPLE(tch, ch) {
                if (tch == ch)
                    continue;
                if (!CAN_SEE(tch, ch))
                    continue;
                if (CAN_SEE(ch, tch) ? (GET_PERCEPTION(tch) < roll - 50 + number(0, 50))
                                     : (GET_PERCEPTION(tch) < roll / 2))
                    continue;
                if (GET_OBJ_HIDDENNESS(obj))
                    act("You notice $n trying to move $p!", FALSE, ch, obj, tch, TO_VICT);
                else
                    act("You spot $n concealing $p!", FALSE, ch, obj, tch, TO_VICT);
            }
            GET_OBJ_HIDDENNESS(obj) = roll;
            obj->last_to_hold = ch;
        }

        /* Failure. orz */
        else {
            if (number(0, 100) <= 30) {
                if (EFF_FLAGGED(ch, EFF_INVISIBLE))
                    appear(ch);
                else if (IS_HIDDEN(ch))
                    GET_HIDDENNESS(ch) = MAX(0, GET_HIDDENNESS(ch) - 100);
            }
            if (IS_FOREST(ch->in_room)) {
                act("You drag $p under some bushes, but they don't quite cover it.", FALSE, ch, obj, 0, TO_CHAR);
                act("$n tries to hide $p under some bushes, but they don't quite cover "
                    "it.",
                    TRUE, ch, obj, 0, TO_ROOM);
            } else {
                act("You try to conceal $p, but can't find a good spot.", FALSE, ch, obj, 0, TO_CHAR);
                act("$n clumsily tries to conceal $p, botching the attempt.", TRUE, ch, obj, 0, TO_ROOM);
            }
        }
        improve_skill(ch, SKILL_CONCEAL);
        WAIT_STATE(ch, PULSE_VIOLENCE);
    }
}

ACMD(do_touch) {
    struct obj_data *obj;

    one_argument(argument, arg);

    if (!*arg)
        send_to_char("Touch what?\r\n", ch);
    else if (!str_cmp(arg, "all") || !strn_cmp(arg, "all.", 4))
        send_to_char("One at a time...\r\n", ch);
    else if (ch->in_room == NOWHERE ||
             !(obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, arg))))
        send_to_char("You don't see that here.\r\n", ch);
    else {
        act("$n touches $p.", TRUE, ch, obj, 0, TO_ROOM);
        if (GET_OBJ_TYPE(obj) == ITEM_TOUCHSTONE && !IS_NPC(ch)) {
            act("As you touch $p, you feel a magical link form.", FALSE, ch, obj, 0, TO_CHAR);
            GET_HOMEROOM(ch) = world[ch->in_room].vnum;
        } else {
            act("You touch $p.  Nothing happens.", FALSE, ch, obj, 0, TO_CHAR);
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
    struct obj_data *obj1, *obj2, *temp;

    two_arguments(argument, buf1, buf2);

    if (!*buf1 || !*buf2)
        send_to_char("Compare what?\r\n", ch);
    else if (!str_cmp(buf1, "all") || !strn_cmp(buf1, "all.", 4) || !str_cmp(buf2, "all") || !strn_cmp(buf2, "all.", 4))
        send_to_char("You can only compare two items at a time!\r\n", ch);
    else if (!(obj1 = find_obj_in_list(ch->carrying, find_vis_by_name(ch, buf1))))
        cprintf(ch, "You don't have a %s.\r\n", buf1);
    else if (!(obj2 = find_obj_in_list(ch->carrying, find_vis_by_name(ch, buf2))))
        cprintf(ch, "You don't have a %s.\r\n", buf2);
    else if (obj1 == obj2)
        send_to_char("They're the same item!\r\n", ch);
    else if (GET_OBJ_TYPE(obj1) != GET_OBJ_TYPE(obj2))
        act("The $o and $O have nothing in common.", FALSE, ch, obj1, obj2, TO_CHAR);
    else
        switch (GET_OBJ_TYPE(obj1)) {
        case ITEM_LIGHT:
            if (GET_OBJ_VAL(obj1, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT ||
                GET_OBJ_VAL(obj2, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT)
                send_to_char("You can't tell much about the two items.\r\n", ch);
            else if (GET_OBJ_VAL(obj1, VAL_LIGHT_REMAINING) == GET_OBJ_VAL(obj2, VAL_LIGHT_REMAINING))
                send_to_char("They look to have the same amount of fuel.\r\n", ch);
            else {
                if (GET_OBJ_VAL(obj1, VAL_LIGHT_REMAINING) < GET_OBJ_VAL(obj2, VAL_LIGHT_REMAINING))
                    SWAP(obj1, obj2);
                act("$p looks to have more fuel than $P.", FALSE, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_WEAPON:
            if ((IS_WEAPON_SLASHING(obj1) && !IS_WEAPON_SLASHING(obj2)) ||
                (IS_WEAPON_PIERCING(obj1) && !IS_WEAPON_PIERCING(obj2)) ||
                (IS_WEAPON_CRUSHING(obj1) && !IS_WEAPON_CRUSHING(obj2)))
                send_to_char("These weapons are too different to realistically compare "
                             "them!\r\n",
                             ch);
            else if (WEAPON_AVERAGE(obj1) == WEAPON_AVERAGE(obj2))
                send_to_char("They each look about as dangerous as the other.\r\n", ch);
            else {
                if (WEAPON_AVERAGE(obj1) < WEAPON_AVERAGE(obj2))
                    SWAP(obj1, obj2);
                act("$p looks more dangerous than $P.", FALSE, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            if (!EFF_FLAGGED(ch, EFF_DETECT_MAGIC))
                send_to_char("You can't tell anything about either item.\r\n", ch);
            else if (GET_OBJ_VAL(obj1, VAL_WAND_CHARGES_LEFT) == GET_OBJ_VAL(obj2, VAL_WAND_CHARGES_LEFT))
                send_to_char("They seem to each hold equal power.\r\n", ch);
            else {
                if (GET_OBJ_VAL(obj1, VAL_WAND_CHARGES_LEFT) < GET_OBJ_VAL(obj2, VAL_WAND_CHARGES_LEFT))
                    SWAP(obj1, obj2);
                act("$p seems to have more charges than $P.", FALSE, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_TREASURE:
        case ITEM_MONEY:
            if (GET_OBJ_COST(obj1) == GET_OBJ_COST(obj2))
                send_to_char("They look equally valuable.\r\n", ch);
            else {
                if (GET_OBJ_COST(obj1) < GET_OBJ_COST(obj2))
                    SWAP(obj1, obj2);
                act("$p looks more valuable than $P.", FALSE, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_ARMOR:
            if (GET_OBJ_VAL(obj1, VAL_ARMOR_AC) == GET_OBJ_VAL(obj2, VAL_ARMOR_AC))
                send_to_char("They look like they offer similar protection.\r\n", ch);
            else {
                if (GET_OBJ_VAL(obj1, VAL_ARMOR_AC) < GET_OBJ_VAL(obj2, VAL_ARMOR_AC))
                    SWAP(obj1, obj2);
                act("$p looks more protective than $P.", FALSE, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_CONTAINER:
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
            if (GET_OBJ_VAL(obj1, VAL_CONTAINER_CAPACITY) == GET_OBJ_VAL(obj2, VAL_CONTAINER_CAPACITY))
                send_to_char("They appear to be very similar in size.\r\n", ch);
            else {
                if (GET_OBJ_VAL(obj1, VAL_CONTAINER_CAPACITY) < GET_OBJ_VAL(obj2, VAL_CONTAINER_CAPACITY))
                    SWAP(obj1, obj2);
                act("$p looks larger than $P.", FALSE, ch, obj1, obj2, TO_CHAR);
            }
            break;
        case ITEM_FOOD:
            if (GET_OBJ_VAL(obj1, VAL_FOOD_FILLINGNESS) == GET_OBJ_VAL(obj2, VAL_FOOD_FILLINGNESS))
                send_to_char("They look equally filling.\r\n", ch);
            else {
                if (GET_OBJ_VAL(obj1, VAL_FOOD_FILLINGNESS) < GET_OBJ_VAL(obj2, VAL_FOOD_FILLINGNESS))
                    SWAP(obj1, obj2);
                act("$p looks more filling than $P.", FALSE, ch, obj1, obj2, TO_CHAR);
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
            send_to_char("You can't decide which is better.\r\n", ch);
            break;
        }
}

/***************************************************************************
 * $Log: act.item.c,v $
 * Revision 1.170  2010/06/20 19:53:47  mud
 * Log to file errors we might want to see.
 *
 * Revision 1.169  2009/03/20 13:56:22  jps
 * Moved coin info into an array of struct coindef.
 *
 * Revision 1.168  2009/03/19 23:16:23  myc
 * Moved the get command and its kids from here to act.get.c.
 *
 * Revision 1.167  2009/03/16 19:17:52  jps
 * Change macro GET_HOME to GET_HOMEROOM
 *
 * Revision 1.166  2009/03/15 07:09:24  jps
 * Add !FALL flag for objects
 *
 * Revision 1.165  2009/03/09 21:43:50  myc
 * Make get_check_money use statemoney.
 *
 * Revision 1.164  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 * Overhauled do_drop to work with dropping multiple types of
 * coins at once.  Also now supports dropping multiple objects
 * at once.
 *
 * Revision 1.163  2009/03/09 05:41:31  jps
 * Moved money stuff into money.h, money.c
 *
 * Revision 1.162  2009/03/09 02:22:32  myc
 * Added functionality for removing messages from the new boards
 * to the remove command.
 *
 * Revision 1.161  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.160  2009/03/03 19:41:50  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.159  2009/01/16 23:36:34  myc
 * Fix possible use of uninitialized variable in do_palm().
 *
 * Revision 1.158  2008/09/29 03:24:44  jps
 * Make container weight automatic. Move some liquid container functions to
 *objects.c.
 *
 * Revision 1.157  2008/09/22 02:09:17  jps
 * Changed weight into a floating-point value. Precision is preserved to
 * the 1/100 place.
 *
 * Revision 1.156  2008/09/20 06:05:06  jps
 * Add macros POSSESSED and POSSESSOR.
 *
 * Revision 1.155  2008/09/09 08:23:37  jps
 * Placed sector info into a struct and moved its macros into rooms.h.
 *
 * Revision 1.154  2008/09/07 01:29:12  jps
 * You can't be given enough weight to make you fall from flying.
 *
 * Revision 1.153  2008/09/03 17:34:08  myc
 * Moved liquid information into a def struct array.
 *
 * Revision 1.152  2008/09/02 06:52:30  jps
 * Using limits.h.
 *
 * Revision 1.151  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.150  2008/08/31 01:19:54  jps
 * You screwed up holding items in slots with two of the same position!
 * Fix.
 *
 * Revision 1.149  2008/08/30 22:02:42  myc
 * Actions affecting corpses just go to the syslog now, not the RIP log.
 *
 * Revision 1.148  2008/08/30 21:55:50  myc
 * Players retrieving items from their corpse won't be logged to the RIP
 * log anymore.
 *
 * Revision 1.147  2008/08/30 20:51:38  jps
 * Fix wearing bug.
 *
 * Revision 1.146  2008/08/30 20:25:38  jps
 * Moved count_hand_eq() into handler.c and mentioned it in handler.h.
 *
 * Revision 1.145  2008/08/30 20:21:07  jps
 * Moved equipment-wearability checks into function may_wear_eq() and moved
 * it to handler.c.
 *
 * Revision 1.144  2008/08/30 18:20:53  myc
 * Changed rnum check for an object to use NOTHING constant.
 *
 * Revision 1.143  2008/08/19 02:11:14  jps
 * Don't apply fluid/rigidity restrictions to immortals.
 *
 * Revision 1.142  2008/08/18 01:35:38  jps
 * Replaced all \\n\\r with \\r\\n, not that it was really necessary...
 *
 * Revision 1.141  2008/08/15 18:18:24  jps
 * Don't exempt immortals from the need to wear a belt before wearing
 * an item on the belt.
 *
 * Revision 1.140  2008/07/27 05:14:02  jps
 * Changed the name of SAVE_CRASH to SAVE_AUTO.
 *
 * Revision 1.139  2008/07/21 19:28:35  jps
 * Add Oxford comma to coin list.
 *
 * Revision 1.138  2008/07/10 20:11:36  myc
 * Crash bug: can't move stuff directly between equipment positions!  Must
 * use equip_char/unequip_char.  In do_remove...was causing a crash at
 * rent-time.
 *
 * Revision 1.137  2008/06/07 19:06:46  myc
 * Moved all object-related constants and structures to objects.h
 *
 * Revision 1.136  2008/06/05 02:07:43  myc
 * Changing object flags to use flagvectors.  Rewrote rent-saving code
 * to use ascii-format files.
 *
 * Revision 1.135  2008/05/18 05:39:59  jps
 * Changed room_data member number to "vnum".
 *
 * Revision 1.134  2008/05/14 05:10:28  jps
 * Using hurt_char for play-time harm, while alter_hit is for changing hp only.
 *
 * Revision 1.133  2008/05/11 05:42:46  jps
 * Using regen.h. alter_hit() now takes the attacker.
 *
 * Revision 1.132  2008/04/13 21:57:17  jps
 * Made it more difficult to manipulate objects when confused.
 * You usually can't do "all.", and you will frequently fumble
 * and choose the wrong object. You can't conceal or stow.
 * Moved do_doorbash to act.movement.c
 *
 * Revision 1.131  2008/04/13 10:03:27  jps
 * Don't send the message about object becoming solid again when
 * you're junking it.
 *
 * Revision 1.130  2008/04/13 01:53:46  jps
 * Fix error message when you failed to give when using "all." or "all".
 *
 * Revision 1.129  2008/04/08 02:13:48  jps
 * Oops couldn't get objects out of containers.
 *
 * Revision 1.128  2008/04/06 19:47:00  jps
 * Use perform_drop when things fall because you gave them to fluid
 * people.  Thus drop triggers will run.
 *
 * Revision 1.127  2008/04/04 03:42:57  jps
 * Add call to give_shopkeeper_reject().  Also allow passing objects
 * between two fluid creatures.
 *
 * Revision 1.126  2008/04/03 02:05:34  myc
 * Depending on screen.h now.
 *
 * Revision 1.125  2008/04/02 05:36:19  myc
 * Added the autosplit toggle, which causes players to automatically
 * split the coins they pick up from corpses.
 *
 * Revision 1.124  2008/03/30 17:30:38  jps
 * Renamed objsave.c to pfiles.c and introduced pfiles.h. Files using functions
 * from pfiles.c now include pfiles.h and depend on it in the makefile.
 *
 * Revision 1.123  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.122  2008/03/26 23:31:34  jps
 * Prevent handling of objects when in a fluid state.  If you drop
 * something, it becomes solid, and you can't pick it up again.
 *
 * Revision 1.121  2008/03/21 22:52:52  jps
 * Reset TRANSIENT objects' timers when they are placed in a room,
 * not when they are picked up.  Prior to this, TRANSIENT objects
 * would tend to dissolve in under an hour if not picked up, which
 * was really too short.
 *
 * Revision 1.120  2008/03/19 04:32:14  myc
 * Fixed capitalization in a message in do_palm.
 *
 * Revision 1.119  2008/03/10 20:46:55  myc
 * Renamed hometown to homeroom and using GET_HOME macro.
 *
 * Revision 1.118  2008/03/09 18:11:17  jps
 * perform_move may be misdirected now.
 *
 * Revision 1.117  2008/03/05 03:03:54  myc
 * Removed defunct do_home command.  (This functionality is available via
 * do_goto).
 *
 * Revision 1.116  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.115  2008/02/02 04:27:55  myc
 * Making get object trigger go off before illusory mob check.
 * Adding consume object trigger.
 *
 * Revision 1.114  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.113  2008/01/28 05:16:11  jps
 * Suppress can't-wear messages for individual items when you say
 * "wear all" or "wear all.foo".
 *
 * Revision 1.112  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species
 *into races.
 *
 * Revision 1.111  2008/01/27 01:42:03  jps
 * Fix grammar when informing player of cursed objects.
 *
 * Revision 1.110  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.109  2008/01/26 12:29:44  jps
 * Use skills.h to import improve_skill().
 *
 * Revision 1.108  2008/01/22 22:29:25  myc
 * Refined perform_wear.  Only allow wearing an item 'on belt' if actually
 * wearing a belt.  If the belt is removed, any item 'on belt' falls off.
 *
 * Revision 1.107  2008/01/15 03:21:11  myc
 * Rewrote give command so it always gives feedback.  Also made REMOVE
 * triggers not prevent immortals from removing items.
 *
 * Revision 1.106  2008/01/13 23:06:04  myc
 * Took out a redundant check in can_take_obj.
 *
 * Revision 1.105  2008/01/11 02:04:32  myc
 * Moved do_drag to act.movement.c
 *
 * Revision 1.104  2008/01/10 05:39:43  myc
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.103  2008/01/09 10:11:56  jps
 * Jacked up the drink-mv restoration to 100% for drinking 24 units.
 *
 * Revision 1.102  2008/01/09 07:41:32  jps
 * Give proper feedback for "give all <vict>", similar to "give all.foo <vict>"
 *
 * Revision 1.101  2008/01/07 11:57:31  jps
 * Fix output for failure to give things to illusions.
 *
 * Revision 1.100  2008/01/06 23:50:47  jps
 * Added spells project and simulacrum, and MOB2_ILLUSORY flag.
 *
 * Revision 1.99  2008/01/06 05:31:35  jps
 * Macro NOWEAR_CLASS for item flags that stop you from wearing equipment.
 *
 * Revision 1.98  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.97  2007/12/21 07:51:43  myc
 * Give command now gives feedback for 'give all.item person' when you
 * have none of the item.
 *
 * Revision 1.96  2007/12/19 20:36:01  myc
 * Replaced static '4' with 'NUM_COIN_TYPES'.
 *
 * Revision 1.95  2007/11/18 20:01:05  jps
 * Cause transient objects' timers to be reset when picked up.
 *
 * Revision 1.94  2007/11/18 16:51:55  myc
 * Changing LVL_BUILDER reference to LVL_GOD.
 *
 * Revision 1.93  2007/10/25 20:37:00  myc
 * Fixed a bug that was throwing garbage bits in the RIP log.
 * Added the compare command, which lets you compare two similar items.
 *
 * Revision 1.92  2007/10/17 17:18:04  myc
 * Renamed the search_block and search_block2 functions.
 * searchblock is now case sensitive, and search_block is not.
 *
 * Revision 1.91  2007/10/04 16:20:24  myc
 * The transient item flag now makes things decay when they are on the
 * ground.
 *
 * Revision 1.90  2007/09/21 18:08:29  jps
 * Eating food will restore hit points, while drinking water will
 * restore movement points.
 *
 * Revision 1.89  2007/09/21 08:44:45  jps
 * Added object type "touchstone" and command "touch" so you can set
 * your home room by touching specific objects.
 *
 * Revision 1.88  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  AFF_HIDE and ITEM_HIDDEN are now
 * unused.  Changes to conceal, palm, stow, and get.
 *
 * Revision 1.87  2007/09/03 23:59:43  jps
 * Added macro ADDED_WEIGHT_OK for testing whether a char can have an
 * object added to its inventory.  Avoids an integer overflow problem
 * that could occur if an object's weight was near maxint.
 *
 * Revision 1.86  2007/08/26 01:55:19  myc
 * Adding lag to conceal, palm, stow.
 *
 * Revision 1.85  2007/08/17 03:34:41  myc
 * Correct error message for do_conceal when no argument.
 *
 * Revision 1.84  2007/08/16 19:53:38  myc
 * Adding stow/palm commands as secondary functionality to conceal skill.
 *
 * Revision 1.83  2007/08/14 22:43:07  myc
 * Adding 'conceal' skill which lets you hide items in the room.
 *
 * Revision 1.82  2007/08/03 22:00:11  myc
 * Fixed several \r\n typos in send_to_chars.
 *
 * Revision 1.81  2007/07/19 21:59:52  jps
 * Dynamic strings for drink containers.
 *
 * Revision 1.80  2007/07/19 15:32:01  jps
 * Add "extinguish" as a subcommand of light.
 *
 * Revision 1.79  2007/07/15 18:01:46  myc
 * You can now put lit permanent lights into containers.  Lit nonpermanent
 * lights are automatically extinguished.
 *
 * Revision 1.78  2007/07/15 17:16:12  jps
 * Add IS_POISONED macro, and moved HIGHLY_VISIBLE macro to utils.h
 *
 * Revision 1.77  2007/07/14 14:34:15  myc
 * Actions on highly visible objects are only seen when the actor is not
 * wiz-invis.
 *
 * Revision 1.76  2007/07/14 01:00:14  jps
 * Clear the af struct before use, so that poison doesn't set all
 * kinds of random unintended effects.
 *
 * Revision 1.75  2007/05/28 23:41:34  jps
 * put and get will interact first with equipped containers.
 *
 * Revision 1.74  2007/05/28 06:35:49  jps
 * Use the verb "activate" for permanent lights and "extinguish" otherwise.
 *
 * Revision 1.73  2007/05/27 17:45:28  jps
 * Fix pluralization in not-found messages.
 *
 * Revision 1.72  2007/05/27 17:34:42  jps
 * Typo fix "You slide <item> onto your ... finger"
 *
 * Revision 1.71  2007/05/24 05:02:49  jps
 * Rewrite do_wield. Fix bug with refusing to wield when one hand holds an item.
 *
 * Revision 1.70  2007/05/21 01:35:22  jps
 * Fix 'wear <weapon>' allowing dual wield without the skill.
 *
 * Revision 1.69  2007/04/18 00:23:34  myc
 * Fixed a typo in perform_wear.
 *
 * Revision 1.68  2007/04/15 10:36:53  jps
 * Make take/drop/give messages always visible when the object being
 * manipulated is highly visible (lit, glowing, or large).
 *
 * Revision 1.67  2007/04/11 14:15:28  jps
 * Give money piles proper keywords and make them dissolve when stolen.
 *
 * Revision 1.66  2007/04/11 07:25:43  jps
 * Remove magic numbers from hand-equipping code.  Make the wear command
 * equip weapons wielded secondary as well.
 *
 * Revision 1.65  2007/02/20 17:16:27  myc
 * Fixed crash bug in doorbash.
 *
 * Revision 1.64  2006/12/08 05:06:58  myc
 * Bribe triggers now give proper amounts and variables.
 *
 * Revision 1.63  2006/12/06 02:14:21  myc
 * Allowed use of 'worn as light' pos to players for lights that
 * are holdable and not wearable anywhere else.
 *
 * Revision 1.62  2006/11/30 05:06:24  jps
 * Add remove trigger for objects
 *
 * Revision 1.61  2006/11/24 06:50:02  jps
 * Draggability is now conferred with the TAKE wear flag.
 * An exception is made for corpses.
 *
 * Revision 1.60  2006/11/21 20:17:04  jps
 * Fix feedback when trying to pour: "The <name> is empty."
 *
 * Revision 1.59  2006/11/21 03:45:52  jps
 * The 'light' command now looks first at equipped items.
 *
 * Revision 1.58  2006/11/18 21:00:28  jps
 * Reworked disarm skill and disarmed-weapon retrieval.
 *
 * Revision 1.57  2006/11/18 09:08:15  jps
 * Use pretty-printing statemoney to format coins
 *
 * Revision 1.56  2006/11/18 04:26:32  jps
 * Renamed continual light spell to illumination, and it only works on
 * LIGHT items (still rooms too).
 *
 * Revision 1.55  2006/11/18 00:03:31  jps
 * Fix continual light items to always work when they have the
 * bit set.  Rooms now print an indicator of being continually lit.
 * Can't use it to make a room permanently lit any more.
 *
 * Revision 1.54  2006/11/16 16:42:14  jps
 * Prevent fountains from being emptied.
 *
 * Revision 1.53  2006/11/14 19:00:43  jps
 * Even if you are full, you can drink when you are a little thirsty.
 *
 * Revision 1.52  2006/11/12 20:22:32  jps
 * JUNKing an item will no longer invoke any drop triggers.
 * Attempting to drop a cursed item also won't invoke drop triggers.
 *
 * Revision 1.51  2006/11/08 10:12:28  jps
 * Fixed typo where you wore bracelets "on around" your wrist.
 *
 * Revision 1.50  2006/11/08 08:47:58  jps
 * Fixed gender in message about dragging a living creature.
 * Added slightly humorous message for when you try to drag yourself.
 *
 * Revision 1.49  2006/11/08 07:57:08  jps
 * Typo fix "stomach can't contain anymore" -> "any more"
 *
 * Revision 1.48  2006/11/07 11:07:57  jps
 * Allow belt items to be worn normally, and fix belt message typos.
 *
 * Revision 1.47  2006/11/07 08:29:16  jps
 * Can't drink any more when not thirsty.
 *
 * Revision 1.46  2006/07/20 07:38:42  cjd
 * Typo fixes.
 *
 * Revision 1.45  2003/07/24 22:47:15  jjl
 * Added the ability for mobs to give away cursed items.
 *
 * Revision 1.44  2003/07/06 20:00:30  jjl
 * Added some checking to avoid giving mobs items they can't use.
 *
 * Revision 1.43  2002/12/21 21:01:21  jjl
 * Refixed doorbash.  The hell was I thinking?
 *
 * Revision 1.42  2002/09/15 04:01:18  jjl
 * Fixed Doorbash so that the door is OPEN WHEN you go through.  Also,
 * stuns you even if there is no return door.
 *
 * Revision 1.41  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.40  2002/02/07 00:15:03  dce
 * Modified the code so players could not 'wear gloves !' and
 * have the item put in the light wear position.
 *
 * Revision 1.39  2001/12/18 02:29:41  dce
 * Gods can now drop and give cursed items away. This was
 * done for the quest.
 *
 * Revision 1.38  2001/12/07 15:42:30  dce
 * Fixed a bug with object spell effects where if a player
 * was wearing an item and died, they would permanently
 * gain that ability.
 *
 * Revision 1.37  2001/04/23 03:31:37  dce
 * Players will not be able to pick up an object that is
 * a higher level than they are. Also players will not
 * be able to give a low level character a high level item
 * in a bag. I also fixed cursed items by not allowing them
 * to be placed into a container.
 *
 * Revision 1.36  2001/04/08 13:25:12  dce
 * Prevented perform_give from give a high level boat to
 * a low level character.
 *
 * Revision 1.35  2001/04/07 14:10:18  dce
 * Can't get a boat that is a higher level than you.
 *
 * Revision 1.34  2001/03/31 21:22:01  dce
 * Fixed the doorbash bug when an exit doesn't exist.
 *
 * Revision 1.33  2001/03/24 15:10:26  dce
 * Players can not wear equipment until they are the proper
 * level based on the object.
 *
 * Revision 1.32  2001/01/16 19:25:37  rsd
 * Added a null check to doorbash to prevent crashing on
 * doorbash of invalid door. RE-tabbed section of doorbash
 * code
 * ..
 *
 * Revision 1.31  2000/11/28 00:44:00  mtp
 * removed mobprog references
 *
 * Revision 1.30  2000/11/25 08:09:43  rsd
 * Added corpse dragging debug to do_drag to track when player
 * corpses are drug from one room to another.  Also added debug
 * to perform_get_from_container() or something like that to
 * log each object looted from a player corpse to prevent
 * players from claiming that their eq just 'disappeared'.
 * Also retabbed and braced {} sections of the associated code
 * in an attempt to make it readable.
 *
 * Revision 1.29  2000/11/20 03:34:31  rsd
 * added some missing and back rlog messages from prior
 * to the addition of the $log$ string.
 *
 * Revision 1.28  2000/11/10 23:32:26  mtp
 * fixed the wear/wield weirdness including weight check for weapons
 *
 * Revision 1.27  2000/11/10 00:04:25  mtp
 * fixed the wear all allowing 2 handers + 2 weapons
 *
 * Revision 1.26  2000/11/03 17:28:33  jimmy
 * Added better checks for real_room to stop players/objs from
 * being placed in room NOWHERE.  This should help pinpoint any
 * weirdness.
 *
 * Revision 1.25  2000/09/04 19:41:01  rsd
 * Retabbed doorbash and added a skill ckeck early in the function.
 *
 * Revision 1.24  2000/04/22 22:24:46  rsd
 * No more animals wearing eq and wielding weapons
 *
 * Revision 1.23  2000/04/05 22:55:57  rsd
 * more cmc enhancements to do give...
 *
 * Revision 1.22  2000/04/05 22:31:12  rsd
 * More cmc tweaks on do give :)
 *
 * Revision 1.21  2000/04/05 21:53:33  rsd
 * CMC altered perform_give() to include the summation of many
 * items given to someone. Also altered do_give() to reflect
 * the implementation of these changes.
 *
 * Revision 1.20  2000/04/05 20:49:32  rsd
 * Chris added a fix to do_get to prevent a huge loop of
 * trying to give non-existant objects to someone, Ie give
 * 48000 cpper person.
 *
 * Revision 1.19  2000/04/05 06:30:44  rsd
 * changed the comment header to make it a fiery file
 *
 * Revision 1.18  1999/12/02 23:17:10  rsd
 * removed mode == SCMD_JUNK from #define VANISH(mode)
 * in order to drop the silly vanishes message from junking
 *
 * Revision 1.17  1999/11/23 15:48:23  jimmy
 * Fixed the slashing weapon skill.  I had it erroneously as stabbing. Doh.
 * Reinstated dual wield.
 * Allowed mobs/players to pick up items while fighting.
 * Fixed a bug in the damage message that wrongfully indicated a miss
 * due to a rounding error in the math.
 * This was all done in order to facilitate the chance to sling your
 * weapon in combat.  Dex and proficiency checks are now made on any missed
 * attact and a failure of both causes the weapon to be slung.
 *
 * Revision 1.16  1999/09/10 01:07:05  mtp
 * also message if can-t 't dual wield (no skill)
 *
 * Revision 1.15  1999/09/10 01:04:25  mtp
 * message if trying to wield when already wielding 2 weapons
 *
 * Revision 1.14  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.13  1999/09/03 22:57:43  mtp
 * added some IS_FIGHTING checks
 *
 * Revision 1.12  1999/08/13 21:33:19  dce
 * Wear all and rent now does weapons.
 *
 * Revision 1.11  1999/08/07 23:49:16  mud
 * Added the function corpse_consent to assist in checking
 * for player consent to messing with their corpses. Added the
 * functionality to looting (do_get) and dragging corpses.
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
 * Revision 1.8  1999/05/02 16:50:16  dce
 * You can now get one coin.
 *
 * Revision 1.7  1999/04/09 03:38:36  dce
 * Junk command is back!
 *
 * Revision 1.6  1999/03/12 18:05:43  dce
 * Players can no longer hold lights in the light position
 *
 * Revision 1.5  1999/02/20 18:41:36  dce
 * Adds improve_skill calls so that players can imprve their skills.
 *
 * Revision 1.4  1999/02/12 16:25:09  jimmy
 * Fixed poofs to show a default poof if none is set.
 *
 * Revision 1.3  1999/02/10 02:38:58  dce
 * Fixes some of continual light.
 *
 * Revision 1.2  1999/02/06 04:09:01  dce
 * David Endre 2/5/99
 * Adds do_light, to allow lights to be turned on while in
 * someones inventory.
 *
 * Revision 1.1  1999/01/29 01:23:29  mud
 * Initial Revision
 *
 ***************************************************************************/
