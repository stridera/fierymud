/***************************************************************************
 * $Id: spec_procs.c,v 1.103 2010/07/06 03:07:02 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: spec_procs.c                                   Part of FieryMUD *
 *  Usage: implementation of special procedures for mobiles/objects/rooms  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "act.h"
#include "casting.h"
#include "clan.h"
#include "class.h"
#include "comm.h"
#include "conf.h"
#include "constants.h"
#include "cooldowns.h"
#include "db.h"
#include "events.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "limits.h"
#include "math.h"
#include "movement.h"
#include "skills.h"
#include "specprocs.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

#include <math.h>

/* extern functions */
EVENTFUNC(recall_event);
void money_convert(struct char_data *ch, int amount);
int room_recall_check(struct char_data *ch, struct char_data *victim, struct obj_data *obj);
void apply_cost(int cost, struct char_data *ch);

int red_recall_room(struct char_data *ch);
int green_recall_room(struct char_data *ch);
int blue_recall_room(struct char_data *ch);
int gray_recall_room(struct char_data *ch);

struct social_type {
    char *cmd;
    int next_line;
};

/********************************************************************
 *              Special procedures for mobiles                      *
 ********************************************************************/

SPECIAL(guild) {
    ACMD(do_save);

    if (IS_NPC(ch))
        return 0;

    if (CMD_IS("level")) {
        skip_spaces(&argument);

        if (!*argument)
            return 0; /* process via the regular command */

        if (str_cmp(argument, "gain"))
            return 0; /* process via the regular command */

        if (getbaseclass(GET_CLASS((struct char_data *)(me))) != getbaseclass(GET_CLASS(ch))) {
            act("$N tells you \"I can't raise you!  Find your own guildmaster.\"", FALSE, ch, 0, me, TO_CHAR);
            return TRUE;
        }

        if (GET_EXP(ch) == exp_next_level(GET_LEVEL(ch), GET_CLASS(ch)) - 1) {
            if (GET_LEVEL(ch) < LVL_MAX_MORT) {
                sprintf(buf, "$n says \"$N is ready for level %d!\"", GET_LEVEL(ch) + 1);
                act(buf, FALSE, me, 0, ch, TO_ROOM);
                /* advance to the next level! */
                gain_exp(ch, 1, GAIN_IGNORE_LEVEL_BOUNDARY);
                GET_HIT(ch) = MAX(GET_HIT(ch), GET_MAX_HIT(ch));
                GET_MOVE(ch) = MAX(GET_MOVE(ch), GET_MAX_MOVE(ch));
                do_save(ch, "", 0, 0);
            } else {
                act("$N tells you \"You are as powerful as a mortal can be.\"", FALSE, ch, 0, me, TO_CHAR);
            }
        } else {
            sprintf(buf, "$N tells you \"You are not ready for level %d yet.\"", GET_LEVEL(ch) + 1);
            act(buf, FALSE, ch, 0, me, TO_CHAR);
        }
        return TRUE;
    }

    return 0; /* command not intercepted above (level, practice) */
}

SPECIAL(dump) {
    struct obj_data *k;
    /*  int value = 0; */

    ACMD(do_drop);

    for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
        act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
        extract_obj(k);
    }

    if (!CMD_IS("drop"))
        return 0;

    do_drop(ch, argument, cmd, 0);

    for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
        act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
        /*   value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10)); */
        extract_obj(k);
    }
    return 1;
}

/*******************************************************************
 *              Special procedures for mobiles                     *
 *******************************************************************/

SPECIAL(guild_guard) {
    int i, IS_GUARDED = FALSE;
    extern int guild_info[][3];
    struct char_data *guard = (struct char_data *)me;
    char *buf = "The guard humiliates you, and blocks your way.\r\n";
    char *buf2 = "The guard humiliates $n, and blocks $s way.";

    if (!IS_MOVE(cmd) || EFF_FLAGGED(guard, EFF_BLIND))
        return FALSE;

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return FALSE;

    /* Is the direction even being guarded? */

    for (i = 0; guild_info[i][0] != -1; i++) {
        if (world[ch->in_room].vnum == guild_info[i][1] && cmd == guild_info[i][2]) {
            IS_GUARDED = TRUE;
        }
    }

    if (!IS_GUARDED) {
        return FALSE;
    }

    /* Ok, it's guarded, can you class get in? */

    for (i = 0; guild_info[i][0] != -1; i++) {
        if (!IS_NPC(ch) && GET_CLASS(ch) == guild_info[i][0] && world[ch->in_room].vnum == guild_info[i][1] &&
            cmd == guild_info[i][2]) {
            return FALSE;
        }
    }

    /* Ok, it's guarded and you can't get in :P and no other case
       will allow you in */

    send_to_char(buf, ch);
    act(buf2, FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
}

SPECIAL(puff) {
    ACMD(do_say);

    if (cmd)
        return (0);

    switch (number(0, 60)) {
    case 0:
        do_say(ch, "My god!  It's full of stars!", 0, 0);
        return (1);
    case 1:
        do_say(ch, "How'd all those fish get up here?", 0, 0);
        return (1);
    case 2:
        do_say(ch, "I'm a very female dragon.", 0, 0);
        return (1);
    case 3:
        do_say(ch, "I've got a peaceful, easy feeling.", 0, 0);
        return (1);
    default:
        return (0);
    }
}

SPECIAL(janitor) {
    struct obj_data *i;

    if (cmd || !AWAKE(ch))
        return (FALSE);

    for (i = world[ch->in_room].contents; i; i = i->next_content) {
        if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
            continue;
        if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
            continue;
        act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
        obj_from_room(i);
        obj_to_char(i, ch);
        get_check_money(ch, i);
        return TRUE;
    }

    return FALSE;
}

SPECIAL(cityguard) {
    struct char_data *tch, *evil;
    int max_evil;

    if (cmd || !AWAKE(ch) || FIGHTING(ch))
        return FALSE;

    max_evil = 1000;
    evil = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
        if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
            act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
            attack(ch, tch);
            return (TRUE);
        }
    }

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
        if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_THIEF)) {
            act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
            attack(ch, tch);
            return (TRUE);
        }
    }

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
        if (CAN_SEE(ch, tch) && FIGHTING(tch)) {
            if ((GET_ALIGNMENT(tch) < max_evil) && (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
                max_evil = GET_ALIGNMENT(tch);
                evil = tch;
            }
        }
    }

    if (evil && (GET_ALIGNMENT(FIGHTING(evil)) >= 0)) {
        act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
        attack(ch, evil);
        return (TRUE);
    }
    return (FALSE);
}

void convert_coins_copper(struct char_data *ch) {
    if (GET_CASH(ch) < 1) {
        return;
    }
    do {
        if (GET_SILVER(ch) > 0) {
            GET_SILVER(ch) = (GET_SILVER(ch) - (1));
            GET_COPPER(ch) = (GET_COPPER(ch) + 10);
        }
        if (GET_GOLD(ch) > 0) {
            GET_GOLD(ch) = (GET_GOLD(ch) - (1));
            GET_COPPER(ch) = (GET_COPPER(ch) + (100));
        }
        if (GET_PLATINUM(ch) > 0) {
            GET_PLATINUM(ch) = (GET_PLATINUM(ch) - (1));
            GET_COPPER(ch) = (GET_COPPER(ch) + (1000));
        }
    } while (!(GET_CASH(ch) == GET_COPPER(ch)));
    return;
}

void copper_to_coins(struct char_data *ch) {
    if (GET_CASH(ch) < 1) {
        return;
    }
    do {
        if (GET_COPPER(ch) > 9) {
            (GET_COPPER(ch) = (GET_COPPER(ch) - (10)));
            (GET_SILVER(ch) = (GET_SILVER(ch) + (1)));
        }
    } while (GET_COPPER(ch) > 9);
    do {
        if (GET_SILVER(ch) > 9) {
            (GET_SILVER(ch) = (GET_SILVER(ch) - (10)));
            (GET_GOLD(ch) = (GET_GOLD(ch) + (1)));
        }
    } while (GET_SILVER(ch) > 9);
    do {
        if (GET_GOLD(ch) > 9) {
            (GET_GOLD(ch) = (GET_GOLD(ch) - (10)));
            (GET_PLATINUM(ch) = (GET_PLATINUM(ch) + (1)));
        }
    } while (GET_GOLD(ch) > 9);
    return;
}

void money_convert(struct char_data *ch, int amount) {
    if (GET_CASH(ch) < 1) {
        if (GET_LEVEL(ch) < 100)
            send_to_char("You don't have enough!\r\n", ch);
        else
            return;
        return;
    }
    if (amount < GET_COPPER(ch)) {
        return;
    }
    do {
        if (GET_SILVER(ch) > 0) {
            (GET_SILVER(ch) = (GET_SILVER(ch) - (1)));
            (GET_COPPER(ch) = (GET_COPPER(ch) + (10)));
        }
        if ((GET_SILVER(ch) < 1) && (GET_GOLD(ch) > 0)) {
            if (GET_COPPER(ch) < amount) {
                (GET_GOLD(ch) = (GET_GOLD(ch) - (1)));
                (GET_SILVER(ch) = (GET_SILVER(ch) + (10)));
            }
        }
        if ((GET_SILVER(ch) < 1) && (GET_GOLD(ch) < 1)) {
            if (((GET_COPPER(ch) + GET_SILVER(ch)) < amount) && (GET_PLATINUM(ch) > 0)) {
                (GET_PLATINUM(ch) = (GET_PLATINUM(ch) - (1)));
                (GET_GOLD(ch) = (GET_GOLD(ch) + (10)));
            }
        }
    } while (amount > GET_COPPER(ch));
    return;
}

#define PET_PRICE(pet) ((GET_LEVEL(pet) * GET_LEVEL(pet)) + (3 * GET_LEVEL(pet)))

SPECIAL(pet_shop) {
    int ideal_mountlevel(struct char_data * ch);
    int mountlevel(struct char_data * ch);

    char buf[MAX_STRING_LENGTH], pet_name[256];
    int pet_room, bp, temp, temp2, temp3, temp4, mountdiff;
    struct char_data *pet;
    struct follow_type *flw;

    pet_room = ch->in_room + 1;

    if (CMD_IS("list")) {
        send_to_char("Pet                                     Cost           Ridability\r\n", ch);
        send_to_char("--------------------------------------  -------------  ----------\r\n", ch);
        for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
            mountdiff = mountlevel(pet) - ideal_mountlevel(ch);
            bp = PET_PRICE(pet);
            temp = ((int)(bp) / 1000);
            temp2 = ((int)((int)(bp / 100) - (temp * 10)));
            temp3 = ((int)((((int)(bp / 10)) - (temp * 100)) - temp2 * 10));
            temp4 = ((int)((((bp) - (temp * 1000)) - (temp2 * 100)) - (temp3 * 10)));
            sprintf(buf, "%-36s  &0&b&6%3d&0p,&b&3%d&0g,&0%ds,&0&3%d&0c    %s\r\n", GET_NAME(pet), temp, temp2, temp3,
                    temp4,
                    !MOB_FLAGGED(pet, MOB_MOUNTABLE)
                        ? "n/a"
                        : mountdiff > MOUNT_LEVEL_FUDGE
                              ? "impossible"
                              : mountdiff < 1 ? "good" : mountdiff < 2 ? "fair" : mountdiff < 4 ? "bad" : "awful");
            send_to_char(buf, ch);
        }
        return (TRUE);
    } else if (CMD_IS("buy")) {
        argument = one_argument(argument, buf);
        argument = one_argument(argument, pet_name);

        if (!*buf) {
            send_to_char("What do you want to buy?\r\n", ch);
            return (TRUE);
        }
        for (flw = ch->followers; flw; flw = flw->next) {
            if (EFF_FLAGGED(flw->follower, EFF_CHARM)) {
                send_to_char("You already have a pet!\r\n", ch);
                return (TRUE);
            }
        }

        if (!(pet = find_char_in_room(&world[pet_room], find_by_name(buf)))) {
            send_to_char("There is no such pet!\r\n", ch);
            return (TRUE);
        }
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            if (GET_CASH(ch) < PET_PRICE(pet)) {
                send_to_char("You don't have enough money!\r\n", ch);
                return TRUE;
            }
            apply_cost(PET_PRICE(pet), ch);
        }

        pet = read_mobile(GET_MOB_RNUM(pet), REAL);
        GET_EXP(pet) = 0;
        SET_FLAG(EFF_FLAGS(pet), EFF_CHARM);

        if (*pet_name) {
            sprintf(buf, "%s %s", GET_NAMELIST(pet), pet_name);
            GET_NAMELIST(pet) = strdup(buf);

            sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n", pet->player.description,
                    pet_name);
            pet->player.description = strdup(buf);
        }
        char_to_room(pet, ch->in_room);
        add_follower(pet, ch);

        send_to_char("May you enjoy your pet.\r\n", ch);
        act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

        return 1;
    }
    /* All commands except list and buy */
    return 0;
}

/********************************************************************
 *                Special procedures for objects                    *
 ********************************************************************/

SPECIAL(bank) {
    int coins[NUM_COIN_TYPES], i;

    if (CMD_IS("balance")) {
        statemoney(buf, GET_COINS(ch));
        cprintf(ch, "Coins carried:   %s\r\n", buf);
        statemoney(buf, GET_BANK_COINS(ch));
        cprintf(ch, "Coins in bank:   %s\r\n", buf);
        return TRUE;
    }

    else if (CMD_IS("deposit")) {

        if (!parse_money(&argument, coins)) {
            send_to_char("You can only deposit platinum, gold, silver, and copper coins.\r\n", ch);
            return TRUE;
        }

        for (i = 0; i < NUM_COIN_TYPES; ++i)
            if (coins[i] > GET_COINS(ch)[i]) {
                cprintf(ch, "You don't have enough %s!\r\n", COIN_NAME(i));
                return TRUE;
            }

        act("$n makes a bank transaction.", TRUE, ch, 0, 0, TO_ROOM);
        statemoney(buf, coins);
        cprintf(ch, "You deposit %s.\r\n", buf);

        for (i = 0; i < NUM_COIN_TYPES; ++i) {
            GET_COINS(ch)[i] -= coins[i];
            GET_BANK_COINS(ch)[i] += coins[i];
        }

        return TRUE;
    }

    else if (CMD_IS("dump")) {
        if (GET_CASH(ch) <= 0)
            send_to_char("You don't have any coins to deposit!\r\n", ch);
        else {
            send_to_char("You dump all your coins on the counter to be deposited.\r\n", ch);
            for (i = 0; i < NUM_COIN_TYPES; ++i) {
                GET_BANK_COINS(ch)[i] += GET_COINS(ch)[i];
                coins[i] = GET_COINS(ch)[i];
                GET_COINS(ch)[i] = 0;
            }
            statemoney(buf, coins);
            cprintf(ch, "You were carrying %s.\r\n", buf);
        }
        return TRUE;
    }

    else if (CMD_IS("withdraw")) {
        if (!parse_money(&argument, coins)) {
            send_to_char("You can only withdraw platinum, gold, silver, and copper coins.\r\n", ch);
            return TRUE;
        }

        for (i = 0; i < NUM_COIN_TYPES; ++i)
            if (coins[i] > GET_BANK_COINS(ch)[i]) {
                sprintf(buf, "You don't have enough %s in the bank!\r\n", COIN_NAME(i));
                send_to_char(buf, ch);
                return TRUE;
            }

        act("$n makes a bank transaction.", TRUE, ch, 0, 0, TO_ROOM);
        statemoney(buf, coins);
        cprintf(ch, "You withdraw %s.\r\n", buf);

        for (i = 0; i < NUM_COIN_TYPES; ++i) {
            GET_BANK_COINS(ch)[i] -= coins[i];
            GET_COINS(ch)[i] += coins[i];
        }

        return TRUE;
    }

    else if (CMD_IS("exchange")) {
        int amount, ok = 0;
        char arg1[MAX_INPUT_LENGTH];
        char arg2[MAX_INPUT_LENGTH];
        char arg3[MAX_INPUT_LENGTH];
        char arg4[MAX_INPUT_LENGTH];
        char ctype2[10];
        double exchange_rate;
        int copper, charge;
        int multto, multfrom;
        int type1, type2;

        half_chop(argument, arg1, arg2);
        if (is_number(arg1)) {
            amount = atoi(arg1);
            if (!*arg2) {
                sprintf(buf, "Exchange %s of what? Platinum?Gold?Silver?Copper?\r\n", arg1);
                send_to_char(buf, ch);
                return 1;
            }
            half_chop(arg2, arg3, arg2);
            if (!*arg3) {
                sprintf(buf, "Exchange %s to what? Platinum? Gold? Silver? Copper?\r\n", arg2);
                send_to_char(buf, ch);
                return 1;
            }
            half_chop(arg2, arg4, arg2);
            if (!*arg4) {
                sprintf(buf, "Exchange %s to what? Platinum? Gold? Silver? Copper?\r\n", arg3);
                send_to_char(buf, ch);
                return 1;
            }
            if (is_abbrev(arg3, "copper")) {
                type1 = 1;
                multfrom = 1;
            } else if (is_abbrev(arg3, "silver")) {
                type1 = 2;
                multfrom = 10;
            } else if (is_abbrev(arg3, "gold")) {
                type1 = 3;
                multfrom = 100;
            } else if (is_abbrev(arg3, "platinum")) {
                type1 = 4;
                multfrom = 1000;
            } else {
                sprintf(buf, "What kind of currency is that?\r\n");
                send_to_char(buf, ch);
                return 1;
            }
            if (is_abbrev(arg4, "copper")) {
                type2 = 1;
            } else if (is_abbrev(arg4, "silver")) {
                type2 = 2;
            } else if (is_abbrev(arg4, "gold")) {
                type2 = 3;
            } else if (is_abbrev(arg4, "platinum")) {
                type2 = 4;
            } else {
                sprintf(buf, "What kind of currency is that?\r\n");
                send_to_char(buf, ch);
                return 1;
            }

            if (type1 == type2) {
                send_to_char("That would be pointless, try using two different types of currency.\r\n",
                             ch);
                return 1;
            }

            if (amount <= 0) {
                send_to_char("The bank doesn't make money because it's dumb! Try a positive value.\r\n",
                             ch);
                return 1;
            }

            switch (type1) {
            case 1:
                if (GET_COPPER(ch) >= amount)
                    ok = 1;
                break;
            case 2:
                if (GET_SILVER(ch) >= amount)
                    ok = 1;
                break;
            case 3:
                if (GET_GOLD(ch) >= amount)
                    ok = 1;
                break;
            case 4:
                if (GET_PLATINUM(ch) >= amount)
                    ok = 1;
                break;
            }

            if (ok == 0) {
                send_to_char("You don't have that many coins of that type!\r\n", ch);
                return 1;
            }

            ok = 0;
            exchange_rate =
                ((17 - (GET_CHA(ch) / 6.0) + number(0, 2) - (number(0, 4) / 10.0) + (number(0, 9) / 10.0)) / 100.0);
            amount = amount * multfrom;
            charge = (int)(ceil(exchange_rate * amount));

            switch (type2) {
            case 1:
                strcpy(ctype2, "copper");
                multto = 1;
                if ((amount / 1 >= 1) && (amount % 1 == 0))
                    ok = 1;
                break;
            case 2:
                strcpy(ctype2, "silver");
                multto = 10;
                if ((amount / 10 >= 1) && (amount % 10 == 0))
                    ok = 1;
                break;
            case 3:
                strcpy(ctype2, "gold");
                multto = 100;
                if ((amount / 100 >= 1) && (amount % 100 == 0))
                    ok = 1;
                break;
            case 4:
                strcpy(ctype2, "platinum");
                multto = 1000;
                if ((amount / 1000 >= 1) && (amount % 1000 == 0))
                    ok = 1;
                break;
            default:
                log("SYSERR: bank error: invalid type2 in spec proc");
                send_to_char("Bank error.\r\n", ch);
                return 1;
            }

            if ((ok == 0) && type2 > type1) {
                send_to_char("That's not the right multiple to convert to that type!\r\n", ch);
                return 1;
            }

            if (GET_CASH(ch) < (amount + charge)) {
                send_to_char("You don't have enough money!\r\n", ch);
                sprintf(buf, "The fee for that transaction would be %d copper.\r\n", charge);
                send_to_char(buf, ch);
                return 1;
            }

            switch (type1) {
            case 1:
                GET_COPPER(ch) -= amount / multfrom;
                break;
            case 2:
                GET_SILVER(ch) -= amount / multfrom;
                break;
            case 3:
                GET_GOLD(ch) -= amount / multfrom;
                break;
            case 4:
                GET_PLATINUM(ch) -= amount / multfrom;
                break;
            }

            money_convert(ch, charge);
            GET_COPPER(ch) -= charge;
            copper = amount / multto;
            sprintf(buf, "You receive %d %s.\r\n", copper, ctype2);
            sprintf(buf, "%sYou pay %d copper for the transaction.\r\n", buf, charge);
            send_to_char(buf, ch);

            switch (type2) {
            case 1:
                GET_COPPER(ch) += copper;
                break;
            case 2:
                GET_SILVER(ch) += copper;
                break;
            case 3:
                GET_GOLD(ch) += copper;
                break;
            case 4:
                GET_PLATINUM(ch) += copper;
                break;
            }

            return 1;
        } else {
            send_to_char("Format: exchange <quantity> <from type> <to type>\r\n      "
                         "  exchange 10 copper silver\r\n",
                         ch);
            return 1;
        }
    } else {
        return 0;
    }
    return 0;
}

/* weapon_spell() function and special procedures -dak */
void weapon_spell(char *to_ch, char *to_vict, char *to_room, struct char_data *ch, struct char_data *vict,
                  struct obj_data *obj, int spl) {
    int level = LVL_IMPL + 1, i;

    for (i = 0; i < NUM_CLASSES; i++)
        if (skills[spl].min_level[i] < level)
            level = skills[spl].min_level[i];
    level = MAX(1, MIN(LVL_IMMORT - 1, level));

    act(to_ch, FALSE, ch, obj, vict, TO_CHAR);
    act(to_vict, FALSE, ch, obj, vict, TO_VICT);
    act(to_room, FALSE, ch, obj, vict, TO_NOTVICT);
    call_magic(ch, vict, 0, spl, level, CAST_SPELL);
}

SPECIAL(holyw_weapon) {
    struct char_data *vict = FIGHTING(ch);

    if (cmd || !vict || !number(0, 9))
        return 0;
    if (number(1, 100) > 5)
        return 0;

    weapon_spell("&3&bYour The Holy Avenger of &0&6&bSalinth&0&3&b glows with a soft "
                 "light as holy wrath pours from its blade!&0",
                 "&3&b$n's The Holy Avenger of &0&6&bSalinth&0&3&b glows with a soft "
                 "light as holy wrath pours from its blade and strikes you!&0",
                 "&3&b$n's The Holy Avenger of &0&6&bSalinth&0&3&b glows with a soft "
                 "light as holy wrath pours from its blade to strike $N!&0",
                 ch, vict, (struct obj_data *)me, SPELL_HOLY_WORD);
    return 1;
}

SPECIAL(vampiric_weapon) {
    struct char_data *vict = FIGHTING(ch);

    if (!(GET_CLASS(ch) == CLASS_ANTI_PALADIN) || cmd || !vict || number(0, 100) < 97)
        return 0;

    weapon_spell("A &9&bblack haze&0 forms around your $p as you strike $N!",
                 "A &9&bblack haze&0 forms around $n's $p as $e strikes you!",
                 "A &9&bblack haze&0 forms around $n's $p as $e strikes $N.", ch, vict, (struct obj_data *)me,
                 SPELL_VAMPIRIC_BREATH);
    return 1;
}

SPECIAL(fire_weapon) {
    struct char_data *vict = FIGHTING(ch);

    if (cmd || !vict)
        return 0;
    if (number(0, 100) < 90)
        return 0;

    weapon_spell("Your $o twinkles &3brightly&0 and sends waves of &6plasma&0 into $N!",
                 "$n's $o twinkles &3brightly&0 and sends waves of &6plasma&0 into you!",
                 "$n's $o twinkles &3brightly&0 and sends waves of &6plasma&0 into $N!", ch, vict,
                 (struct obj_data *)me, SPELL_FIREBALL);

    return 1;
}

SPECIAL(lightning_weapon) {
    struct char_data *vict = FIGHTING(ch);

    if (cmd || !vict)
        return 0;
    if (number(0, 100) < 90)
        return 0;

    weapon_spell("Your $o glows &4blue&0 and a bolt rushes through the air at $N!",
                 "$n's $o glows &4blue&0 and a bolt rushes through the air at you!",
                 "$n's $o glows &4blue&0 and a bolt rushes through the air at $N.", ch, vict, (struct obj_data *)me,
                 SPELL_LIGHTNING_BOLT);
    return 1;
}

SPECIAL(frost_weapon) {
    struct char_data *vict = FIGHTING(ch);

    if (cmd || !vict)
        return 0;

    if (number(0, 100) < 80)
        return 0;

    weapon_spell("A burst of freezing air suddenly bursts from your $o!",
                 "A burst of freezing air suddenly bursts from $n's $o!",
                 "A burst of freezing air suddenly bursts from $n's $o!", ch, vict, (struct obj_data *)me,
                 SPELL_CONE_OF_COLD);

    return 1;
}

/*
 *  This is a generic function to implement the core of the recall scripts.
 *  Additional recall types (K-town, or whatever) should be easily addable
 *  by adding additional colors and indices.
 */

int do_recall(struct char_data *ch, struct obj_data *obj, int cmd, char *argument, int color) {
    char arg_1[MAX_INPUT_LENGTH];
    char target[MAX_INPUT_LENGTH];
    char *tmp;
    struct char_data *targ;
    struct recall_event_obj *recall;
    int room;
    char *color_names[] = {"red", "green", "blue", "gray"};

    if (!CMD_IS("recite"))
        return FALSE;

    tmp = one_argument(argument, arg_1);
    one_argument(tmp, target);

    /* Make sure the command given was "recite scroll" or "recite recall"
     * (Not sure if this is at all necessary...) */

    if (!is_abbrev(arg_1, "scroll") && !is_abbrev(arg_1, "recall") && !is_abbrev(arg_1, color_names[color])) {
        return FALSE;
    }

    /* Identify the target (who will be recalled). */

    if (strlen(target) < 1) {
        targ = ch;
    } else {
        targ = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, target));
    }

    if (!strcmp(target, "self") || !str_cmp(target, "me")) {
        targ = ch;
    }

    /* Make sure the target is valid. */

    if (!targ) {
        send_to_char("You do not see that person here!\r\n", ch);
        return TRUE;
    }

    if (IS_NPC(targ)) {
        send_to_char("The guildmaster would think you funny...\r\n", ch);
        return TRUE;
    }

    if (!room_recall_check(ch, targ, obj))
        return TRUE;

    if (GET_LEVEL(targ) >= LVL_IMMORT && targ != ch) {
        send_to_char("Mortal magicks on a GOD?! Are you CRAZY?!\r\n", ch);
        return TRUE;
    }

    /* Determine the destination room. */

    room = NOWHERE;
    switch (color) {
    case 0:
        room = red_recall_room(targ);
        break;
    case 1:
        room = green_recall_room(targ);
        break;
    case 2:
        room = blue_recall_room(targ);
        break;
    case 3:
        room = gray_recall_room(targ);
        break;
    };

    if (room == NOWHERE)
        return TRUE;

    /* Create the event that will actually move the target in a few seconds. */

    CREATE(recall, struct recall_event_obj, 1);
    recall->ch = targ;
    recall->from_room = ch->in_room;

    recall->room = room;
    event_create(EVENT_RECALL, recall_event, recall, TRUE, &(ch->events), 8 RL_SEC);

    /* Finally, after all sanity-checking, we know the scroll is going
     * to work and that it has a proper target.  We can destroy it and
     * send out the appropriate messages. */

    send_to_char("You recite the scroll, which dissolves.\r\n", ch);

    if (targ == ch) {
        act("$n recites a scroll of recall at $mself.", FALSE, ch, 0, targ, TO_ROOM);
    } else {
        act("$n recites a scroll of recall at YOU!", FALSE, ch, 0, targ, TO_VICT);
        act("$n recites a scroll of recall at $N.", FALSE, ch, 0, targ, TO_NOTVICT);
    }

    extract_obj(obj);

    return TRUE;
}

/*
 * The recalls are all done through a common function above.  It simplifies
 * things a lot, and should help out on keeping th code clean.
 */

SPECIAL(gray_recall) { return do_recall(ch, (struct obj_data *)me, cmd, argument, 3); }

SPECIAL(blue_recall) { return do_recall(ch, (struct obj_data *)me, cmd, argument, 2); }

SPECIAL(green_recall) { return do_recall(ch, (struct obj_data *)me, cmd, argument, 1); }

SPECIAL(red_recall) { return do_recall(ch, (struct obj_data *)me, cmd, argument, 0); }

int red_recall_room(struct char_data *ch) {
    int room;
    switch (GET_CLASS(ch)) {
    case CLASS_SORCERER:
        room = 6231;
        break;
    case CLASS_NECROMANCER:
        room = 6223;
        break;
    case CLASS_PYROMANCER:
        room = 6220;
        break;
    case CLASS_CRYOMANCER:
        room = 6221;
        break;

    case CLASS_ROGUE:
        room = 6068;
        break;
    case CLASS_THIEF:
        room = 6068;
        break;
    case CLASS_ASSASSIN:
        room = 6088;
        break;
    case CLASS_MERCENARY:
        room = 6170;
        break;

    case CLASS_CLERIC:
        room = 6218;
        break;
    case CLASS_PRIEST:
        room = 6218;
        break;
    case CLASS_DIABOLIST:
        room = 6075;
        break;
    case CLASS_DRUID:
        room = 6222;
        break;

    case CLASS_ANTI_PALADIN:
        room = 6080;
        break;

        /* Other warrior types, and anyone we missed */
    default:
        room = 6149;
        break;
    };

    if (real_room(room) == NOWHERE) {
        if (real_room(6001) == NOWHERE) {
            send_to_char("ERROR: Could not find your guild, nor the gates "
                         "of Anduin. Please tell a god!\r\n",
                         ch);
            sprintf(buf,
                    "ERROR: Couldn't find the real room for vnums "
                    "%i or %i",
                    room, 6001);

            mudlog(buf, NRM, LVL_IMMORT, TRUE);
            return NOWHERE;
        };

        send_to_char("ERROR: Could not find your guild! Please tell a "
                     "god!\r\n",
                     ch);
        return real_room(6001);
    };

    return real_room(room);
}

int green_recall_room(struct char_data *ch) {
    int room;
    switch (GET_CLASS(ch)) {
    case CLASS_SORCERER:
        room = 3046;
        break;
    case CLASS_NECROMANCER:
        room = 16932;
        break;
    case CLASS_PYROMANCER:
        room = 3094;
        break;
    case CLASS_CRYOMANCER:
        room = 3093;
        break;

    case CLASS_ROGUE:
    case CLASS_THIEF:
    case CLASS_ASSASSIN:
    case CLASS_MERCENARY:
        room = 3038;
        break;

        /* Do diabolists have their own guild? */
    case CLASS_DIABOLIST:
        room = 3003;
        break;
    case CLASS_CLERIC:
        room = 3003;
        break;
    case CLASS_PRIEST:
        room = 3095;
        break;
    case CLASS_DRUID:
        room = 3087;
        break;

    case CLASS_PALADIN:
        room = 5306;
        break;
    case CLASS_MONK:
        room = 5308;
        break;

    case CLASS_RANGER:
        room = 3550;
        break;

        /* Other warrior types, and anyone we missed */
    case CLASS_ANTI_PALADIN:
    default:
        room = 3022;
        break;
    };

    if (real_room(room) == NOWHERE) {
        if (real_room(3002) == NOWHERE) {
            send_to_char("ERROR: Could not find your guild, nor the Mielikki "
                         "altar. Please tell a god!\r\n",
                         ch);
            sprintf(buf,
                    "ERROR: Couldn't find the real room for vnums "
                    "%i or %i",
                    room, 3002);

            mudlog(buf, NRM, LVL_IMMORT, TRUE);
            return NOWHERE;
        };

        send_to_char("ERROR: Could not find your guild! Please tell a "
                     "god!\r\n",
                     ch);
        return real_room(3002);
    };

    return real_room(room);
}

int blue_recall_room(struct char_data *ch) {
    int room;
    switch (GET_CLASS(ch)) {
    case CLASS_SORCERER:
    case CLASS_NECROMANCER:
    case CLASS_PYROMANCER:
    case CLASS_CRYOMANCER:
        room = 10030;
        break;

    case CLASS_ROGUE:
    case CLASS_THIEF:
    case CLASS_ASSASSIN:
    case CLASS_MERCENARY:
        room = 10048;
        break;

    case CLASS_CLERIC:
    case CLASS_PRIEST:
    case CLASS_DIABOLIST:
    case CLASS_DRUID:
        room = 10003;
        break;

    default:
        room = 10013;
        break;
    };

    if (real_room(room) == NOWHERE) {
        if (real_room(10001) == NOWHERE) {
            send_to_char("ERROR: Could not find your guild, nor the the "
                         "Arctic Temple. Please tell a god!\r\n",
                         ch);
            sprintf(buf,
                    "ERROR: Couldn't find the real room for vnums "
                    "%i or %i",
                    room, 6001);

            mudlog(buf, NRM, LVL_IMMORT, TRUE);
            return NOWHERE;
        };

        send_to_char("ERROR: Could not find your guild! Please tell a "
                     "god!\r\n",
                     ch);
        return real_room(6001);
    };

    return real_room(room);
}

int gray_recall_room(struct char_data *ch) {
    int room;
    switch (GET_CLASS(ch)) {
    case CLASS_SORCERER:
    case CLASS_NECROMANCER:
    case CLASS_PYROMANCER:
    case CLASS_CRYOMANCER:
        room = 30073;
        break;

    case CLASS_ROGUE:
    case CLASS_THIEF:
    case CLASS_ASSASSIN:
    case CLASS_MERCENARY:
        room = 30066;
        break;

    case CLASS_CLERIC:
    case CLASS_PRIEST:
    case CLASS_DIABOLIST:
    case CLASS_DRUID:
        room = 30070;
        break;

    default:
        room = 30030;
        break;
    };

    if (real_room(room) == NOWHERE) {
        if (real_room(30030) == NOWHERE) {
            send_to_char("ERROR: Could not find your guild, nor the the "
                         "Ogakh itself. Please tell a god!\r\n",
                         ch);
            sprintf(buf,
                    "ERROR: Couldn't find the real room for vnums "
                    "%i or %i",
                    room, 30030);

            mudlog(buf, NRM, LVL_IMMORT, TRUE);
            return NOWHERE;
        };

        send_to_char("ERROR: Could not find your guild! Please tell a "
                     "god!\r\n",
                     ch);
        return real_room(30030);
    };

    return real_room(room);
}

SPECIAL(summon_dragon) {
    struct follow_type *fol;
    struct obj_data *item;
    extern void summon_mount(struct char_data * ch, int mob_vnum, int base_hp, int base_mv);

    if (!cmd || !CMD_IS("summon"))
        return FALSE;

    if (!ch || IS_NPC(ch))
        return FALSE;

    if (FIGHTING(ch)) {
        send_to_char("You can't concentrate enough while you are fighting.\r\n", ch);
        return TRUE;
    }

    if ((GET_CLASS(ch) != CLASS_PALADIN) && (GET_CLASS(ch) != CLASS_ANTI_PALADIN)) {
        send_to_char("You have no idea what you are trying to accomplish.\r\n", ch);
        return TRUE;
    }

    /* Must be wearing the dragon summoning item to use it */
    if ((item = (struct obj_data *)me) == NULL || item->worn_by != ch)
        return FALSE;

    /* Fewer limitations for gods */
    if (GET_LEVEL(ch) < LVL_GOD) {
        if (CH_INDOORS(ch)) {
            send_to_char("That won't work indoors!\r\n", ch);
            return TRUE;
        }

        if (GET_COOLDOWN(ch, CD_SUMMON_MOUNT)) {
            int i = GET_COOLDOWN(ch, CD_SUMMON_MOUNT) / (1 MUD_HR) + 1;
            if (i == 1)
                strcpy(buf1, "hour");
            else
                sprintf(buf1, "%d hours", i);
            cprintf(ch, "You must wait another %s before you can summon your mount.\r\n", buf1);
            return TRUE;
        }

        /* Only allow one mount */
        for (fol = ch->followers; fol; fol = fol->next)
            if (IS_NPC(fol->follower) && MOB_FLAGGED(fol->follower, MOB_MOUNTABLE)) {
                send_to_char("You already have a mount!\r\n", ch);
                return TRUE;
            }
    }

    send_to_char("You begin calling for a mount...\r\n", ch);

    if (GET_CLASS(ch) == CLASS_PALADIN)
        summon_mount(ch, 18890, 4 * GET_LEVEL(ch), GET_ALIGNMENT(ch) / 2);
    else if (GET_CLASS(ch) == CLASS_ANTI_PALADIN)
        summon_mount(ch, 18891, 4 * GET_LEVEL(ch), -(GET_ALIGNMENT(ch) / 2));
    return TRUE;
}

/***************************************************************************
 * $Log: spec_procs.c,v $
 * Revision 1.103  2010/07/06 03:07:02  mud
 * Changed lightweight baton's frequency to 20%.
 *
 * Revision 1.102  2010/07/02 14:09:16  mud
 * Adding cone of cold weapon.
 *
 * Revision 1.101  2010/06/05 14:56:27  mud
 * Moving cooldowns to their own file.
 *
 * Revision 1.100  2010/04/25 22:57:16  mud
 * Changing Celestial Betrayer to shoot fireballs.
 *
 * Revision 1.99  2010/04/25 22:13:38  mud
 * Adding frost weapon spec proc.
 *
 * Revision 1.98  2009/06/18 06:02:11  myc
 * Fix command reference in summon_dragon.
 *
 * Revision 1.97  2009/06/11 13:38:13  myc
 * When you level gain, you're healed to full.
 *
 * Revision 1.96  2009/06/09 19:33:50  myc
 * Rewrote gain_exp and retired gain_exp_regardless.
 *
 * Revision 1.95  2009/03/20 13:56:22  jps
 * Moved coin info into an array of struct coindef.
 *
 * Revision 1.94  2009/03/19 23:16:23  myc
 * parse_money now takes a char** and moves the pointer up to
 * just past any money phrase it parses.
 *
 * Revision 1.93  2009/03/09 21:43:50  myc
 * Change statemoney from strcat to strcpy semantics.
 *
 * Revision 1.92  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.91  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.90  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.89  2009/03/03 19:43:44  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.88  2009/01/19 09:25:23  myc
 * Changed summon mount cooldown to count 12 hours from time
 * mount is lost.
 *
 * Revision 1.87  2009/01/17 00:28:02  myc
 * Fix possible use of uninitialized variable.
 *
 * Revision 1.86  2008/09/20 07:51:45  jps
 * Don't charge immortals money in shops.
 *
 * Revision 1.85  2008/09/20 07:25:50  jps
 * Including fight.h since fight.c-related header material was move from
 * handler.h to fight.h.
 *
 * Revision 1.84  2008/09/09 08:23:37  jps
 * Placed sector info into a struct and moved its macros into rooms.h.
 *
 * Revision 1.83  2008/09/07 20:05:27  jps
 * Renamed exp_to_level to exp_next_level to make it clearer what it means.
 *
 * Revision 1.82  2008/09/02 06:56:39  jps
 * Updated header file use
 *
 * Revision 1.81  2008/08/29 16:55:00  myc
 * room_recall_check now asks for a target char too.
 *
 * Revision 1.80  2008/08/18 01:35:38  jps
 * Replaced all \\n\\r with \\r\\n, not that it was really necessary...
 *
 * Revision 1.79  2008/05/18 05:39:59  jps
 * Changed room_data member number to "vnum".
 *
 * Revision 1.78  2008/04/04 06:12:52  myc
 * Removed several old spec procs.
 *
 * Revision 1.77  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.76  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.75  2008/03/07 21:21:57  myc
 * Replaced action delays and skill delays with a single list of
 * 'cooldowns', which are decremented by a recurring event and
 * also save to the player file.
 *
 * Revision 1.74  2008/03/05 05:21:56  myc
 * Bank coins are ints instead of longs now.
 *
 * Revision 1.73  2008/02/09 21:07:50  myc
 * Must provide a boolean to event_create saying whether to
 * free the event obj when done or not.
 *
 * Revision 1.72  2008/02/09 18:29:11  myc
 * Typo in recall scrolls (extra newline).  Allow immortals to use
 * recall scrolls on themselves.
 *
 * Revision 1.71  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.70  2008/02/09 03:06:17  myc
 * Was incorrectly including math.h.
 *
 * Revision 1.69  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.68  2008/01/27 21:14:59  myc
 * Replace hit() with attack().
 *
 * Revision 1.67  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.66  2008/01/23 02:34:43  jps
 * Lose unused variables.
 *
 * Revision 1.65  2008/01/23 02:33:26  jps
 * Fix payment for pets.
 *
 * Revision 1.64  2008/01/18 20:30:11  myc
 * Fixing some send_to_char strings that don't end with a newline.
 *
 * Revision 1.63  2008/01/05 05:44:45  jps
 * Removing unused function prototypes.
 *
 * Revision 1.62  2008/01/04 01:42:50  jps
 * Removed unused practice code.
 *
 * Revision 1.61  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.60  2008/01/02 02:10:34  jps
 * Removed unused external function definitions.
 *
 * Revision 1.59  2008/01/02 01:04:26  jps
 * Removing unused external function clear_skills().
 *
 * Revision 1.58  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.57  2007/12/19 20:55:56  myc
 * Getting rid of a defunct spec proc for a defunct clan.
 *
 * Revision 1.56  2007/10/11 20:14:48  myc
 * Changed skill defines to support chants and songs as skills, but
 * slightly distinguished from spells and skills.  TOP_SKILL is the
 * old MAX_SKILLS.
 *
 * Revision 1.55  2007/09/12 19:23:04  myc
 * Shop keepers give correct response when 'buy' is typed without arguments.
 * Shop keepers won't throw up on you anymore.
 *
 * Revision 1.54  2007/08/23 00:58:13  jps
 * Pet shops will indicate how easy it is for you to ride a mount
 * in the "list" so you can avoid buying impossible mounts.
 *
 * Revision 1.53  2007/08/16 11:54:19  jps
 * Remove defunct specprocs, such as hardcoded subclass quests, and others.
 *
 * Revision 1.52  2007/08/03 22:00:11  myc
 * Fixed some \r\n typoes in send_to_chars.
 *
 * Revision 1.51  2007/07/14 02:16:22  jps
 * Stop calculating mv points of pets here, because it's now
 * handled in db.c.
 *
 * Revision 1.50  2007/05/29 20:16:32  jps
 * Abstracted getting base class.
 *
 * Revision 1.49  2007/05/29 19:45:25  jps
 * Disable same-class-only level gains for now.
 *
 * Revision 1.48  2007/05/28 23:22:14  jps
 * You can only level gain by your own guildmaster.
 *
 * Revision 1.47  2007/04/11 16:05:27  jps
 * Scavengers who pick up money won't end up with the money pile object in
 *inventory.
 *
 * Revision 1.46  2007/04/11 14:18:12  jps
 * Don't have to hold recalls.
 *
 * Revision 1.45  2007/04/04 13:40:12  jps
 * Implement NORECALL flag for rooms.
 *
 * Revision 1.44  2007/03/27 04:27:05  myc
 * Dragonmount special proc.  Fixed typo in stone dagger proc.
 *
 * Revision 1.43  2006/11/20 03:47:24  jps
 * Make petstore list like other store lists.
 *
 * Revision 1.42  2006/11/11 23:52:32  jps
 * Change Ogakh scroll spelling to "gray"
 *
 * Revision 1.41  2006/11/08 09:16:04  jps
 * Fixed some loose-lose typos.
 *
 * Revision 1.40  2006/10/07 02:09:23  dce
 * Fixed typo in grey recall that sent players to blue recall locations.
 *
 * Revision 1.39  2006/10/06 01:54:48  dce
 * Added spec_procs for Ogakh and associated grey recall.
 *
 * Revision 1.38  2006/07/24 21:45:41  cjd
 * Added check in recall scrolls for immortals and "self"
 *
 * Revision 1.37  2005/07/13 21:39:28  cjd
 * commented the recall scroll fail code.
 *
 * Revision 1.36  2003/10/13 06:14:00  jjl
 * Fixed rangers and anti-paladins for green recalls.
 *
 * Revision 1.35  2002/11/09 19:18:27  jjl
 * Moved a NULL check to be BEFORE something that used the variable.  It was
 * causing crashes if you targetted someone who wasn't there.
 *
 * Revision 1.34  2002/10/19 19:14:02  jjl
 * DUH
 * I feel like such an idiot
 * I had left a return real_room(3001) from testing in.  Blue recalls are fixed
 * now.
 *
 * Revision 1.30  2002/10/19 18:29:52  jjl
 * New and improved red green and blue scrolls of recall. Yummy!
 *
 * Revision 1.29  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.28  2001/03/14 00:19:07  dce
 * Pet price increased.
 *
 * Revision 1.27  2001/01/06 22:02:55  rsd
 * Altered the guild guard proc to work for when builders
 * don't listen to you and add multiple classes for the
 * same direction in a guild entrance. Mutha fukas
 *
 * Revision 1.26  2000/11/25 02:33:15  rsd
 * Altered comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.25  2000/11/03 17:28:33  jimmy
 * Added better checks for real_room to stop players/objs from
 * being placed in room NOWHERE.  This should help pinpoint any
 * weirdness.
 *
 * Revision 1.24  2000/10/21 12:08:09  mtp
 * subclasser has been removed...
 *
 * Revision 1.23  2000/10/15 05:20:41  cmc
 * oops.. compiler warning corrected!
 *
 * Revision 1.22  2000/10/15 05:19:47  cmc
 * commented out guildmaster gossip
 * (by request)
 *
 * Revision 1.21  2000/10/13 17:55:52  cmc
 * why oh why did the guild() procedure still exist if it
 * wasn't being used anymore? and the only command it was
 * intercepting was practice, which no longer exists... sheesh!
 *
 * guild() modified for "level gain" code, now functional,
 * but further tweaks may be forthcoming in the near future.
 *
 * Happy Friday the 13th!
 *
 * Revision 1.20  2000/09/19 21:38:58  rsd
 * Metamorpho coded the for loop that checks to see if a
 * player already has a pet and prevents them from purchasing
 * more than one.  I just typed it in.
 *
 * Revision 1.19  2000/09/18 02:52:43  rsd
 * tweaked cost of pets
 *
 * Revision 1.17  2000/03/16 21:58:27  rsd
 * Altered weapon proc so that if hasted, not proper class, and
 * at limit of HP then it won't go off.
 *
 * Revision 1.16  2000/03/06 06:59:55  rsd
 * fixed the BoP guild areas guild guard
 *
 * Revision 1.15  2000/03/05 02:33:30  rsd
 * more grammar and ansi fixes
 * ,
 *
 * Revision 1.14  2000/03/05 01:26:34  rsd
 * fixed grammar in dispel_good weapon proc as well as changed the frequency
 *
 * Revision 1.13  2000/03/04 21:54:31  mud
 * altered the frequency of the vamp weapon proc
 *
 * Revision 1.12  2000/03/04 21:25:53  rsd
 * Altered the frequency at which the vamp weapon proc will
 * go off. Fixed a typo in the procs output. Added w/o
 * success the SPECIAL(dispel_good_weapon) proc.
 *
 * Revision 1.11  1999/11/16 00:18:51  rsd
 * altered the clan0 guild guard spec proc to match the current
 * clan zero.
 *
 * Revision 1.10  1999/10/30 16:03:05  rsd
 * Jimmy coded alignment restrictions for Paladins and exp.
 * Altered gain_exp() to check for a victim.
 *
 * Revision 1.9  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.8  1999/08/24 03:05:35  mud
 * commented out sections of dump that gave exp and money to players
 * for dropping stuff.
 *
 * Revision 1.7  1999/07/24 20:50:18  dce
 * Exchange command for banks added.
 *
 * Revision 1.6  1999/07/07 16:49:04  mud
 * added the quest obect for diabs and antis
 *
 * Revision 1.5  1999/04/30 19:12:56  dce
 * Free mail for gods
 *
 * Revision 1.4  1999/04/24 03:07:01  jimmy
 * fixed errors related to adding the pendantic compiler flag
 * -gurlaek
 *
 * Revision 1.3  1999/04/24 02:12:56  dce
 * Fixed warning messages
 *
 * Revision 1.2  1999/02/02 16:15:46  mud
 * dos2unix
 * indented entire file after putting all {}'s
 * on lines by themselves.
 *
 * Revision 1.1  1999/01/29 01:23:32  mud
 * Initial revision
 *
 ***************************************************************************/
