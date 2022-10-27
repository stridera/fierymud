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
    int found = 0;

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
            sprintf(buf, "%s%d %-36s  &0&b&6%3d&0p,&b&3%d&0g,&0%ds,&0&3%d&0c    %s\r\n", found++ % 2 == 0 ? "&K" : "",
                    found, GET_NAME(pet), temp, temp2, temp3, temp4,
                    !MOB_FLAGGED(pet, MOB_MOUNTABLE) ? "n/a"
                    : mountdiff > MOUNT_LEVEL_FUDGE  ? "impossible"
                    : mountdiff < 1                  ? "good"
                    : mountdiff < 2                  ? "fair"
                    : mountdiff < 4                  ? "bad"
                                                     : "awful");
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
        GET_MAX_MOVE(pet) *= 15;
        GET_MOVE(pet) = GET_MAX_MOVE(pet);
        SET_FLAG(EFF_FLAGS(pet), EFF_CHARM);
        SET_FLAG(MOB_FLAGS(pet), MOB_PET);

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
                send_to_char("That would be pointless, try using two different types of currency.\r\n", ch);
                return 1;
            }

            if (amount <= 0) {
                send_to_char("The bank doesn't make money because it's dumb! Try a positive value.\r\n", ch);
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
            send_to_char(
                "Format: exchange <quantity> <from type> <to type>\r\n      "
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

    weapon_spell(
        "&3&bYour The Holy Avenger of &0&6&bSalinth&0&3&b glows with a soft "
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
    case CLASS_ILLUSIONIST:
        room = 6234;
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
    case CLASS_BARD:
        room = 6093;
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

    case CLASS_BERSERKER:
        room = 55797;
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
            send_to_char(
                "ERROR: Could not find your guild, nor the gates "
                "of Anduin. Please tell a god!\r\n",
                ch);
            sprintf(buf,
                    "ERROR: Couldn't find the real room for vnums "
                    "%i or %i",
                    room, 6001);

            mudlog(buf, NRM, LVL_IMMORT, TRUE);
            return NOWHERE;
        };

        send_to_char(
            "ERROR: Could not find your guild! Please tell a "
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
    case CLASS_ILLUSIONIST:
        room = 3209;
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

    case CLASS_BERSERKER:
        room = 3212;
        break;

    case CLASS_BARD:
        room = 5311;
        break;

        /* Other warrior types, and anyone we missed */
    case CLASS_ANTI_PALADIN:
    default:
        room = 3022;
        break;
    };

    if (real_room(room) == NOWHERE) {
        if (real_room(3002) == NOWHERE) {
            send_to_char(
                "ERROR: Could not find your guild, nor the Mielikki "
                "altar. Please tell a god!\r\n",
                ch);
            sprintf(buf,
                    "ERROR: Couldn't find the real room for vnums "
                    "%i or %i",
                    room, 3002);

            mudlog(buf, NRM, LVL_IMMORT, TRUE);
            return NOWHERE;
        };

        send_to_char(
            "ERROR: Could not find your guild! Please tell a "
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
    case CLASS_ILLUSIONIST:
        room = 10030;
        break;

    case CLASS_ROGUE:
    case CLASS_THIEF:
    case CLASS_ASSASSIN:
    case CLASS_MERCENARY:
    case CLASS_BARD:
        room = 10048;
        break;

    case CLASS_CLERIC:
    case CLASS_PRIEST:
    case CLASS_DIABOLIST:
    case CLASS_DRUID:
        room = 10003;
        break;

    case CLASS_BERSERKER:
        room = 10242;
        break;

    default:
        room = 10013;
        break;
    };

    if (real_room(room) == NOWHERE) {
        if (real_room(10001) == NOWHERE) {
            send_to_char(
                "ERROR: Could not find your guild, nor the the "
                "Arctic Temple. Please tell a god!\r\n",
                ch);
            sprintf(buf,
                    "ERROR: Couldn't find the real room for vnums "
                    "%i or %i",
                    room, 6001);

            mudlog(buf, NRM, LVL_IMMORT, TRUE);
            return NOWHERE;
        };

        send_to_char(
            "ERROR: Could not find your guild! Please tell a "
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
    case CLASS_ILLUSIONIST:
        room = 30000;
        break;

    case CLASS_ROGUE:
    case CLASS_THIEF:
    case CLASS_ASSASSIN:
    case CLASS_MERCENARY:
    case CLASS_BARD:
        room = 30066;
        break;

    case CLASS_CLERIC:
    case CLASS_PRIEST:
    case CLASS_DIABOLIST:
    case CLASS_DRUID:
        room = 30070;
        break;

    case CLASS_BERSERKER:
        room = 30122;
        break;

    default:
        room = 30030;
        break;
    };

    if (real_room(room) == NOWHERE) {
        if (real_room(30030) == NOWHERE) {
            send_to_char(
                "ERROR: Could not find your guild, nor the the "
                "Ogakh itself. Please tell a god!\r\n",
                ch);
            sprintf(buf,
                    "ERROR: Couldn't find the real room for vnums "
                    "%i or %i",
                    room, 30030);

            mudlog(buf, NRM, LVL_IMMORT, TRUE);
            return NOWHERE;
        };

        send_to_char(
            "ERROR: Could not find your guild! Please tell a "
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
