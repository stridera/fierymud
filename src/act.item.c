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
#include "cooldowns.h"
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
        send_to_char("You'll need to break it into pieces to put it into more than one container!\r\n", ch);
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
    if ((GET_OBJ_LEVEL(obj) > GET_LEVEL(vict) + 15) || (GET_OBJ_LEVEL(obj) > 99 && GET_LEVEL(vict) < 100)) {
        act("$E isn't experienced enough to handle the awesome might of $p.", FALSE, ch, obj, vict, TO_CHAR);
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
        if GET_SKILL (ch, SKILL_DUAL_WIELD) {
            obj = confused_inventory_switch(ch, obj);
            perform_wear(ch, obj, WEAR_WIELD2, FALSE);
        } else
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
                send_to_char("These weapons are too different to realistically compare them!\r\n", ch);
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

ACMD(do_create) {
    int r_num, i = 0, found = 0;
    struct obj_data *cobj;

    if (GET_RACE(ch) == RACE_GNOME) {
        if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC)) {
            if (!GET_COOLDOWN(ch, CD_INNATE_CREATE)) {
                if (!ch)
                    return 0;

                one_argument(argument, arg);

                if (!(arg) || !*(arg)) {
                    send_to_char("What are you trying to create?\r\n", ch);
                    return 0;
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
                        send_to_char("Something is wrong with minor create.  Please tell a god.\r\n", ch);
                        return 0;
                    }
                    cobj = read_object(r_num, REAL);
                    if (GET_OBJ_TYPE(cobj) == ITEM_LIGHT) {
                        GET_OBJ_VAL(cobj, VAL_LIGHT_LIT) = TRUE;
                    }
                    obj_to_room(cobj, ch->in_room);
                    act("$n &0&5tinkers with some spare materials...&0", TRUE, ch, 0, 0, TO_ROOM);
                    act("&7$n has created&0 $p&7!&0", FALSE, ch, cobj, 0, TO_ROOM);
                    act("&7You tinker with some spare materials.&0", FALSE, ch, cobj, 0, TO_CHAR);
                    act("&7You have created $p.&0", FALSE, ch, cobj, 0, TO_CHAR);

                    SET_COOLDOWN(ch, CD_INNATE_CREATE, 1 MUD_HR);
                } else {
                    send_to_char("You have no idea how to create such an item.\r\n", ch);
                    return 0;
                }
            } else {
                send_to_char("You need to regain your focus.\r\n", ch);
                cprintf(ch, "You can create again in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_INNATE_CREATE) / 10));
                return;
            }
        } else
            send_to_char("Your magical tinkering fizzles and fails.\r\n", ch);
            return;
    } else
        send_to_char("Only Gnomes can create without spells!\r\n", ch);
        return;
}
