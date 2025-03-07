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

#include "act.hpp"
#include "casting.hpp"
#include "clan.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "cooldowns.hpp"
#include "db.hpp"
#include "events.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "limits.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "movement.hpp"
#include "skills.hpp"
#include "specprocs.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <math.h>

/* extern functions */
EVENTFUNC(recall_event);
int room_recall_check(CharData *ch, CharData *victim, ObjData *obj);
void apply_cost(int cost, CharData *ch);

int red_recall_room(CharData *ch);
int green_recall_room(CharData *ch);
int blue_recall_room(CharData *ch);
int gray_recall_room(CharData *ch);

struct social_type {
    std::string_view cmd;
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
        skip_spaces(argument);

        if (getbaseclass(GET_CLASS((CharData *)(me))) != getbaseclass(GET_CLASS(ch))) {
            act("$N tells you \"I can't raise you!  Find your own guildmaster.\"", false, ch, 0, (CharData *)me,
                TO_CHAR);
            return true;
        }

        if (GET_EXP(ch) == exp_next_level(GET_LEVEL(ch), GET_CLASS(ch)) - 1) {
            if (GET_LEVEL(ch) < LVL_MAX_MORT) {
                sprintf(buf, "$n says \"$N is ready for level %d!\"", GET_LEVEL(ch) + 1);
                act(buf, false, (CharData *)me, 0, ch, TO_ROOM);
                /* advance to the next level! */
                gain_exp(ch, 1, GAIN_IGNORE_LEVEL_BOUNDARY);
                GET_HIT(ch) = std::max(GET_HIT(ch), GET_MAX_HIT(ch));
                GET_MOVE(ch) = std::max(GET_MOVE(ch), GET_MAX_MOVE(ch));
                do_save(ch, "", 0, 0);
            } else {
                act("$N tells you \"You are as powerful as a mortal can be.\"", false, ch, 0, (CharData *)me, TO_CHAR);
            }
        } else {
            sprintf(buf, "$N tells you \"You are not ready for level %d yet.\"", GET_LEVEL(ch) + 1);
            act(buf, false, ch, 0, (CharData *)me, TO_CHAR);
        }
        return true;
    }

    return 0; /* command not intercepted above (level, practice) */
}

SPECIAL(dump) {
    ObjData *k;
    /*  int value = 0; */

    ACMD(do_drop);

    for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
        act("$p vanishes in a puff of smoke!", false, 0, k, 0, TO_ROOM);
        extract_obj(k);
    }

    if (!CMD_IS("drop"))
        return 0;

    do_drop(ch, argument, cmd, 0);

    for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
        act("$p vanishes in a puff of smoke!", false, 0, k, 0, TO_ROOM);
        extract_obj(k);
    }
    return 1;
}

/*******************************************************************
 *              Special procedures for mobiles                     *
 *******************************************************************/

SPECIAL(guild_guard) {
    int i, IS_GUARDED = false;
    extern int guild_info[][3];
    CharData *guard = (CharData *)me;

    if (!IS_MOVE(cmd) || EFF_FLAGGED(guard, EFF_BLIND))
        return false;

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return false;

    /* Is the direction even being guarded? */
    for (i = 0; guild_info[i][0] != -1; i++) {
        if (world[ch->in_room].vnum == guild_info[i][1] && cmd == guild_info[i][2]) {
            IS_GUARDED = true;
        }
    }

    if (!IS_GUARDED)
        return false;

    /* Ok, it's guarded, can you class get in? */
    for (i = 0; guild_info[i][0] != -1; i++) {
        if (!IS_NPC(ch) && GET_CLASS(ch) == guild_info[i][0] && world[ch->in_room].vnum == guild_info[i][1] &&
            cmd == guild_info[i][2]) {
            return false;
        }
    }

    // Ok, it's guarded and you can't get in :P and no other case will allow you in.
    char_printf(ch, "The guard humiliates you, and blocks your way.\n");
    act("The guard humiliates $n, and blocks $s way.", false, ch, 0, 0, TO_ROOM);
    return true;
}

SPECIAL(puff) {
    ACMD(do_say);

    if (cmd)
        return (0);

    switch (random_number(0, 60)) {
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
    ObjData *i;

    if (cmd || !AWAKE(ch))
        return (false);

    for (i = world[ch->in_room].contents; i; i = i->next_content) {
        if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
            continue;
        if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
            continue;
        act("$n picks up some trash.", false, ch, 0, 0, TO_ROOM);
        obj_from_room(i);
        obj_to_char(i, ch);
        get_check_money(ch, i);
        return true;
    }

    return false;
}

SPECIAL(cityguard) {
    CharData *tch, *evil;
    int max_evil;

    if (cmd || !AWAKE(ch) || FIGHTING(ch))
        return false;

    max_evil = 1000;
    evil = 0;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
        if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
            act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", false, ch, 0, 0, TO_ROOM);
            attack(ch, tch);
            return (true);
        }
    }

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
        if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_THIEF)) {
            act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", false, ch, 0, 0, TO_ROOM);
            attack(ch, tch);
            return (true);
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
        act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", false, ch, 0, 0, TO_ROOM);
        attack(ch, evil);
        return (true);
    }
    return (false);
}

void convert_coins_copper(CharData *ch) {
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

void copper_to_coins(CharData *ch) {
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

#define PET_PRICE(pet) ((GET_LEVEL(pet) * GET_LEVEL(pet)) + (3 * GET_LEVEL(pet)))
#define PET_INSPECT_PRICE(pet) (((GET_LEVEL(pet) * GET_LEVEL(pet)) + (3 * GET_LEVEL(pet))) / 10)

SPECIAL(pet_shop) {
    int ideal_mountlevel(CharData * ch);
    int mountlevel(CharData * ch);
    int found = 0;

    int pet_room, bp, temp, temp2, temp3, temp4, mountdiff;
    CharData *pet;
    FollowType *flw;

    pet_room = ch->in_room + 1;

    if (CMD_IS("list")) {
        char_printf(ch, "Pet                                     Cost           Ridability\n");
        char_printf(ch, "--------------------------------------  -------------  ----------\n");
        for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
            mountdiff = mountlevel(pet) - ideal_mountlevel(ch);
            bp = PET_PRICE(pet);
            temp = ((int)(bp) / 1000);
            temp2 = ((int)((int)(bp / 100) - (temp * 10)));
            temp3 = ((int)((((int)(bp / 10)) - (temp * 100)) - temp2 * 10));
            temp4 = ((int)((((bp) - (temp * 1000)) - (temp2 * 100)) - (temp3 * 10)));
            char_printf(ch, "{}{:d} {:<36}  &0&b&6{:3d}&0p,&b&3{:d}&0g,&0{:d}s,&0&3{:d}&0c    {}\n",
                        found++ % 2 == 0 ? "&K" : "", found, GET_NAME(pet), temp, temp2, temp3, temp4,
                        !MOB_FLAGGED(pet, MOB_MOUNTABLE) ? "n/a"
                        : mountdiff > MOUNT_LEVEL_FUDGE  ? "impossible"
                        : mountdiff < 1                  ? "good"
                        : mountdiff < 2                  ? "fair"
                        : mountdiff < 4                  ? "bad"
                                                         : "awful");
        }
        return (true);
    } else if (CMD_IS("buy")) {
        auto what = argument.shift();
        auto pet_name = argument.shift();

        if (what.empty()) {
            char_printf(ch, "What do you want to buy?\n");
            return (true);
        }
        for (flw = ch->followers; flw; flw = flw->next) {
            if (EFF_FLAGGED(flw->follower, EFF_CHARM)) {
                char_printf(ch, "You already have a pet!\n");
                return (true);
            }
        }

        if (!(pet = find_char_in_room(&world[pet_room], find_by_name(what)))) {
            char_printf(ch, "There is no such pet!\n");
            return (true);
        }
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            if (GET_CASH(ch) < PET_PRICE(pet)) {
                char_printf(ch, "You don't have enough money!\n");
                return true;
            }
            apply_cost(PET_PRICE(pet), ch);
        }

        pet = read_mobile(GET_MOB_RNUM(pet), REAL);
        GET_EXP(pet) = 0;
        GET_MAX_MOVE(pet) *= 15;
        GET_MOVE(pet) = GET_MAX_MOVE(pet);
        SET_FLAG(EFF_FLAGS(pet), EFF_CHARM);
        SET_FLAG(MOB_FLAGS(pet), MOB_PET);

        if (!pet_name.empty()) {
            GET_NAMELIST(pet) = fmt::format("{} {}", GET_NAMELIST(pet), pet_name);

            pet->player.description = fmt::format("{}A small sign on a chain around the neck says 'My name is {}'\n",
                                                  pet->player.description, pet_name);
        }
        char_to_room(pet, ch->in_room);
        add_follower(pet, ch);

        char_printf(ch, "May you enjoy your pet.\n");
        act("$n buys $N as a pet.", false, ch, 0, pet, TO_ROOM);

        return 1;
    } else if (CMD_IS("inspect")) {
        auto buf = argument.shift();

        if (buf.empty()) {
            char_printf(ch, "Inspection Services                                              \n");
            char_printf(ch, "Pet                                     Cost           Ridability\n");
            char_printf(ch, "--------------------------------------  -------------  ----------\n");
            for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
                mountdiff = mountlevel(pet) - ideal_mountlevel(ch);
                bp = PET_INSPECT_PRICE(pet);
                temp = ((int)(bp) / 1000);
                temp2 = ((int)((int)(bp / 100) - (temp * 10)));
                temp3 = ((int)((((int)(bp / 10)) - (temp * 100)) - temp2 * 10));
                temp4 = ((int)((((bp) - (temp * 1000)) - (temp2 * 100)) - (temp3 * 10)));
                char_printf(ch, "{}{:d} {:<36}  &0&b&6{:3d}&0p,&b&3{:d}&0g,&0{:d}s,&0&3{:d}&0c    {}\n",
                            found++ % 2 == 0 ? "&K" : "", found, GET_NAME(pet), temp, temp2, temp3, temp4,
                            !MOB_FLAGGED(pet, MOB_MOUNTABLE) ? "n/a"
                            : mountdiff > MOUNT_LEVEL_FUDGE  ? "impossible"
                            : mountdiff < 1                  ? "good"
                            : mountdiff < 2                  ? "fair"
                            : mountdiff < 4                  ? "bad"
                                                             : "awful");
            }
            return (true);
        }

        if (!(pet = find_char_in_room(&world[pet_room], find_by_name(buf)))) {
            char_printf(ch, "There is no such pet!\n");
            return (true);
        }
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            if (GET_CASH(ch) < PET_INSPECT_PRICE(pet)) {
                char_printf(ch, "You don't have enough money!\n");
                return true;
            }
            apply_cost(PET_INSPECT_PRICE(pet), ch);
        }

        pet = read_mobile(GET_MOB_RNUM(pet), REAL);
        GET_EXP(pet) = 0;
        GET_MAX_MOVE(pet) *= 15;
        GET_MOVE(pet) = GET_MAX_MOVE(pet);
        SET_FLAG(EFF_FLAGS(pet), EFF_CHARM);
        SET_FLAG(MOB_FLAGS(pet), MOB_PET);

        std::string output = fmt::format("Name: {}\n", GET_NAME(pet));
        output += fmt::format("Level: {}, Hit Points: {}, Movement Points: {}\n", GET_LEVEL(pet), GET_HIT(pet),
                              GET_MAX_MOVE(pet));
        output += fmt::format("AC: {}, Hitroll: {}, Damroll: {}\n", GET_AC(pet), GET_HITROLL(pet), GET_DAMROLL(pet));
        output += fmt::format("Str: {}, Int: {}, Wis: {}, Dex: {}, Con: {}, Cha: {}\n", GET_STR(pet), GET_INT(pet),
                              GET_WIS(pet), GET_DEX(pet), GET_CON(pet), GET_CHA(pet));
        output += fmt::format("$E is composed of {}, and $S nature is {}.", COMPOSITION_COLOR(pet),
                              COMPOSITION_NAME(pet), LIFEFORCE_COLOR(pet), LIFEFORCE_NAME(pet));
        act(output, false, ch, 0, pet, TO_CHAR);

        return (true);
    }
    /* All commands except list and buy */
    return 0;
}

/********************************************************************
 *                Special procedures for objects                    *
 ********************************************************************/

SPECIAL(bank) {
    Money coins;

    if (CMD_IS("balance")) {
        char_printf(ch, "Coins carried:   {}\n", statemoney(GET_COINS(ch)));
        char_printf(ch, "Coins in bank:   {}\n", statemoney(GET_BANK_COINS(ch)));
        return true;
    } else if (CMD_IS("deposit")) {
        auto coins = parse_money(argument.get());
        if (!coins) {
            char_printf(ch, "You can only deposit platinum, gold, silver, and copper coins.\n");
            return true;
        }

        for (int i = 0; i < NUM_COIN_TYPES; ++i)
            if (coins[i] > GET_COINS(ch)[i]) {
                char_printf(ch, "You don't have enough {}!\n", COIN_NAME(i));
                return true;
            }

        act("$n makes a bank transaction.", true, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You deposit {}.\n", *coins.to_string());

        for (i = 0; i < NUM_COIN_TYPES; ++i) {
            GET_COINS(ch)[i] -= coins[i];
            GET_BANK_COINS(ch)[i] += coins[i];
        }

        return true;
    } else if (CMD_IS("dump")) {
        if (GET_CASH(ch) <= 0)
            char_printf(ch, "You don't have any coins to deposit!\n");
        else {
            char_printf(ch, "You dump all your coins on the counter to be deposited.\n");
            for (int i = 0; i < NUM_COIN_TYPES; ++i) {
                GET_BANK_COINS(ch)[i] += GET_COINS(ch)[i];
                coins[i] = GET_COINS(ch)[i];
                GET_COINS(ch)[i] = 0;
            }
            char_printf(ch, "You were carrying {}.\n", statemoney(coins));
        }
        return true;
    } else if (CMD_IS("withdraw")) {
        auto coin_opt = parse_money(argument.get());
        if (!coin_opt) {
            char_printf(ch, "You can only withdraw platinum, gold, silver, and copper coins.\n");
            return true;
        }
        auto coins = *coin_opt;
        for (int i = 0; i < NUM_COIN_TYPES; ++i)
            if (coins[i] > GET_BANK_COINS(ch)[i]) {
                char_printf(ch, "You don't have enough {} in the bank!\n", COIN_NAME(i));
                return true;
            }

        act("$n makes a bank transaction.", true, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You withdraw {}.\n", statemoney(coins));

        for (int i = 0; i < NUM_COIN_TYPES; ++i) {
            GET_BANK_COINS(ch)[i] -= coins[i];
            GET_COINS(ch)[i] += coins[i];
        }

        return true;
    } else if (CMD_IS("exchange")) {
        int amount;
        double exchange_rate;
        int copper, charge;

        auto amount_opt = argument.try_shift_number();
        if (amount_opt) {
            amount = *amount_opt;

            if (amount < 1) {
                char_printf(ch, "You can't exchange a negative amount!\n");
                return 1;
            }

            auto from = argument.shift();
            auto to = argument.shift();

            if (from.empty()) {
                char_printf(ch, "Exchange {} of what? Platinum? Gold? Silver? Copper?\n", amount);
                return 1;
            }
            auto from_type = parse_coin_type(from);
            if (!from_type) {
                char_printf(ch, "What kind of currency is {}?\n", from);
                return 1;
            }

            if (to.empty()) {
                char_printf(ch, "Exchange {} {} to what? Platinum? Gold? Silver? Copper?\n", amount, from);
                return 1;
            }
            auto to_type = parse_coin_type(to);
            if (!to_type) {
                char_printf(ch, "What kind of currency is {}?\n", to);
                return 1;
            }

            if (from_type == to_type) {
                char_printf(ch, "That would be pointless, try using two different types of currency.\n");
                return 1;
            }

            if (GET_COINS(ch)[from_type] < amount) {
                char_printf(ch, "You don't have that many {}!\n", COIN_NAME(from_type));
                return 1;
            }

            auto to_amount = amount / COIN_SCALE(to_type);

            if (GET_LEVEL(ch) > LVL_IMMORT) {
                charge = 0;
            } else {
                exchange_rate = (17 - (GET_CHA(ch) / 6.0) + random_number(0, 2) - (random_number(0, 4) / 10.0) +
                                 (random_number(0, 9) / 10.0)) /
                                100.0;
                amount = amount * COIN_SCALE(from_type);
                charge = (int)(ceil(exchange_rate * amount));
            }

            if (to_amount < 1) {
                char_printf(ch, "That's not enough to exchange!\n");
                return 1;
            } else if (amount % COIN_SCALE(to_type) != 0) {
                char_printf(ch, "That's not the right multiple to convert to that type!\n");
                return 1;
            }

            if (!charge_char(ch, amount + charge)) {
                char_printf(ch, "You don't have enough money!\n");
                char_printf(ch, "The fee for that transaction would be {:d} copper.\n", charge);
                return 1;
            }

            char_printf(ch, "You receive {:d} {}.\n", to_amount, COIN_NAME(to_type));
            if (charge > 0) {
                char_printf(ch, "You pay {:d} copper for the transaction.\n", charge);
            } else {
                char_printf(ch, "There is no charge for this transaction.\n");
            }

            return 1;
        } else {
            char_printf(ch,
                        "Format: exchange <quantity> <from type> <to type>\n      "
                        "  example: exchange 10 copper silver\n");
            return 1;
        }
    } else {
        return 0;
    }
    return 0;
}

/* weapon_spell() function and special procedures -dak */
void weapon_spell(std::string_view to_ch, std::string_view to_vict, std::string_view to_room, CharData *ch,
                  CharData *vict, ObjData *obj, int spl) {
    int level = LVL_IMPL + 1, i;

    for (i = 0; i < NUM_CLASSES; i++)
        if (skills[spl].min_level[i] < level)
            level = skills[spl].min_level[i];
    level = std::clamp(level, 1, LVL_IMMORT - 1);

    act(to_ch, false, ch, obj, vict, TO_CHAR);
    act(to_vict, false, ch, obj, vict, TO_VICT);
    act(to_room, false, ch, obj, vict, TO_NOTVICT);
    call_magic(ch, vict, 0, spl, level, CAST_SPELL);
}

SPECIAL(holyw_weapon) {
    CharData *vict = FIGHTING(ch);

    if (cmd || !vict || !random_number(0, 9))
        return 0;
    if (random_number(1, 100) > 5)
        return 0;

    weapon_spell(
        "&3&bYour The Holy Avenger of &0&6&bSalinth&0&3&b glows with a soft "
        "light as holy wrath pours from its blade!&0",
        "&3&b$n's The Holy Avenger of &0&6&bSalinth&0&3&b glows with a soft "
        "light as holy wrath pours from its blade and strikes you!&0",
        "&3&b$n's The Holy Avenger of &0&6&bSalinth&0&3&b glows with a soft "
        "light as holy wrath pours from its blade to strike $N!&0",
        ch, vict, (ObjData *)me, SPELL_HOLY_WORD);
    return 1;
}

SPECIAL(vampiric_weapon) {
    CharData *vict = FIGHTING(ch);

    if (!(GET_CLASS(ch) == CLASS_ANTI_PALADIN) || cmd || !vict || random_number(0, 100) < 97)
        return 0;

    weapon_spell("A &9&bblack haze&0 forms around your $p as you strike $N!",
                 "A &9&bblack haze&0 forms around $n's $p as $e strikes you!",
                 "A &9&bblack haze&0 forms around $n's $p as $e strikes $N.", ch, vict, (ObjData *)me,
                 SPELL_VAMPIRIC_BREATH);
    return 1;
}

SPECIAL(fire_weapon) {
    CharData *vict = FIGHTING(ch);

    if (cmd || !vict)
        return 0;
    if (random_number(0, 100) < 90)
        return 0;

    weapon_spell("Your $o twinkles &3brightly&0 and sends waves of &6plasma&0 into $N!",
                 "$n's $o twinkles &3brightly&0 and sends waves of &6plasma&0 into you!",
                 "$n's $o twinkles &3brightly&0 and sends waves of &6plasma&0 into $N!", ch, vict, (ObjData *)me,
                 SPELL_FIREBALL);

    return 1;
}

SPECIAL(lightning_weapon) {
    CharData *vict = FIGHTING(ch);

    if (cmd || !vict)
        return 0;
    if (random_number(0, 100) < 90)
        return 0;

    weapon_spell("Your $o glows &4blue&0 and a bolt rushes through the air at $N!",
                 "$n's $o glows &4blue&0 and a bolt rushes through the air at you!",
                 "$n's $o glows &4blue&0 and a bolt rushes through the air at $N.", ch, vict, (ObjData *)me,
                 SPELL_LIGHTNING_BOLT);
    return 1;
}

SPECIAL(frost_weapon) {
    CharData *vict = FIGHTING(ch);

    if (cmd || !vict)
        return 0;

    if (random_number(0, 100) < 80)
        return 0;

    weapon_spell("A burst of freezing air suddenly bursts from your $o!",
                 "A burst of freezing air suddenly bursts from $n's $o!",
                 "A burst of freezing air suddenly bursts from $n's $o!", ch, vict, (ObjData *)me, SPELL_CONE_OF_COLD);

    return 1;
}

/*
 *  This is a generic function to implement the core of the recall scripts.
 *  Additional recall types (K-town, or whatever) should be easily addable
 *  by adding additional colors and indices.
 */

int do_recall(CharData *ch, ObjData *obj, int cmd, Arguments argument) {
    CharData *targ;
    RecallEventObj *recall;
    int room;

    if (!CMD_IS("recite"))
        return false;

    /* Make sure an object was supplied, or else the script will automatically run. */
    if (argument.empty()) {
        return false;
    }

    auto location = argument.shift();
    auto target = argument.shift();

    /* Make sure the player specified one of the scroll names. */
    if (obj != find_obj_in_list(ch->carrying, find_vis_by_name(ch, location))) {
        return false;
    }

    /* Identify the target (who will be recalled). */
    if (target.empty()) {
        targ = ch;
    } else {
        targ = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, target));
    }

    if (matches(target, "self") || matches(target, "me")) {
        targ = ch;
    }

    /* Make sure the target is valid. */
    if (!targ) {
        char_printf(ch, "You do not see that person here!\n");
        return true;
    }

    if (IS_NPC(targ)) {
        char_printf(ch, "The guildmaster would think you funny...\n");
        return true;
    }

    if (!room_recall_check(ch, targ, obj))
        return true;

    if (GET_LEVEL(targ) >= LVL_IMMORT && targ != ch) {
        char_printf(ch, "Mortal magicks on a GOD?! Are you CRAZY?!\n");
        return true;
    }

    /* Determine the destination room. */

    room = NOWHERE;
    switch (GET_OBJ_VNUM(obj)) {
    case 3056:
        room = red_recall_room(targ);
        break;
    case 3057:
        room = green_recall_room(targ);
        break;
    case 3058:
        room = blue_recall_room(targ);
        break;
    case 30010:
        room = gray_recall_room(targ);
        break;
    };

    if (room == NOWHERE)
        return true;

    /* Create the event that will actually move the target in a few seconds. */

    CREATE(recall, RecallEventObj, 1);
    recall->ch = targ;
    recall->from_room = ch->in_room;

    recall->room = room;
    event_create(EVENT_RECALL, recall_event, recall, true, &(ch->events), 8 RL_SEC);

    /* Finally, after all sanity-checking, we know the scroll is going
     * to work and that it has a proper target.  We can destroy it and
     * send out the appropriate messages. */

    char_printf(ch, "You recite the scroll, which dissolves.\n");

    if (targ == ch) {
        act("$n recites a scroll of recall at $mself.", false, ch, 0, targ, TO_ROOM);
    } else {
        act("$n recites a scroll of recall at YOU!", false, ch, 0, targ, TO_VICT);
        act("$n recites a scroll of recall at $N.", false, ch, 0, targ, TO_NOTVICT);
    }

    extract_obj(obj);

    return true;
}

/*
 * The recalls are all done through a common function above.  It simplifies
 * things a lot, and should help out on keeping the code clean.
 */

SPECIAL(gray_recall) { return do_recall(ch, (ObjData *)me, cmd, argument); }

SPECIAL(blue_recall) { return do_recall(ch, (ObjData *)me, cmd, argument); }

SPECIAL(green_recall) { return do_recall(ch, (ObjData *)me, cmd, argument); }

SPECIAL(red_recall) { return do_recall(ch, (ObjData *)me, cmd, argument); }

int red_recall_room(CharData *ch) {
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
            char_printf(ch, "ERROR: Could not find your guild, nor the gates of Anduin. Please tell a god!\n");
            log(LogSeverity::Stat, LVL_IMMORT, "ERROR: Couldn't find the real room for vnums {} or {}", room, 6001);

            return NOWHERE;
        };

        char_printf(ch, "ERROR: Could not find your guild! Please tell a god!\n");
        return real_room(6001);
    };

    return real_room(room);
}

int green_recall_room(CharData *ch) {
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
            char_printf(ch, "ERROR: Could not find your guild, nor the Mielikki altar. Please tell a god!\n");
            log(LogSeverity::Stat, LVL_IMMORT, "ERROR: Couldn't find the real room for vnums {} or {}", room, 3002);

            return NOWHERE;
        };

        char_printf(ch, "ERROR: Could not find your guild! Please tell a god!\n");
        return real_room(3002);
    };

    return real_room(room);
}

int blue_recall_room(CharData *ch) {
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
            char_printf(ch, "ERROR: Could not find your guild, nor the the Arctic Temple. Please tell a god!\n");
            log(LogSeverity::Stat, LVL_IMMORT, "ERROR: Couldn't find the real room for vnums {} or {}", room, 6001);
            return NOWHERE;
        };

        char_printf(ch, "ERROR: Could not find your guild! Please tell a god!\n");
        return real_room(6001);
    };

    return real_room(room);
}

int gray_recall_room(CharData *ch) {
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
            char_printf(ch, "ERROR: Could not find your guild, nor the the Ogakh itself. Please tell a god!\n");
            log(LogSeverity::Stat, LVL_IMMORT, "ERROR: Couldn't find the real room for vnums {} or {}", room, 30030);
            return NOWHERE;
        };

        char_printf(ch, "ERROR: Could not find your guild! Please tell a god!\n");
        return real_room(30030);
    };

    return real_room(room);
}

SPECIAL(summon_dragon) {
    FollowType *fol;
    ObjData *item;
    extern void summon_mount(CharData * ch, int mob_vnum, int base_hp, int base_mv);

    if (!cmd || !CMD_IS("summon"))
        return false;

    if (!ch || IS_NPC(ch))
        return false;

    if (FIGHTING(ch)) {
        char_printf(ch, "You can't concentrate enough while you are fighting.\n");
        return true;
    }

    if ((GET_CLASS(ch) != CLASS_PALADIN) && (GET_CLASS(ch) != CLASS_ANTI_PALADIN)) {
        char_printf(ch, "You have no idea what you are trying to accomplish.\n");
        return true;
    }

    /* Must be wearing the dragon summoning item to use it */
    if ((item = (ObjData *)me) == nullptr || item->worn_by != ch)
        return false;

    /* Fewer limitations for gods */
    if (GET_LEVEL(ch) < LVL_GOD) {
        if (CH_INDOORS(ch)) {
            char_printf(ch, "That won't work indoors!\n");
            return true;
        }

        if (GET_COOLDOWN(ch, CD_SUMMON_MOUNT)) {
            int i = GET_COOLDOWN(ch, CD_SUMMON_MOUNT) / (1 MUD_HR) + 1;
            auto time_str = [](int i) -> std::string {
                if (i == 1)
                    return "hour";
                else
                    return fmt::format("{} hours", i);
            };
            char_printf(ch, "You must wait another {} before you can summon your mount.\n", time_str(i));
            return true;
        }

        /* Only allow one mount */
        for (fol = ch->followers; fol; fol = fol->next)
            if (IS_NPC(fol->follower) && MOB_FLAGGED(fol->follower, MOB_MOUNTABLE)) {
                char_printf(ch, "You already have a mount!\n");
                return true;
            }
    }

    char_printf(ch, "You begin calling for a mount...\n");

    if (GET_CLASS(ch) == CLASS_PALADIN)
        summon_mount(ch, 18890, 4 * GET_LEVEL(ch), GET_ALIGNMENT(ch) / 2);
    else if (GET_CLASS(ch) == CLASS_ANTI_PALADIN)
        summon_mount(ch, 18891, 4 * GET_LEVEL(ch), -(GET_ALIGNMENT(ch) / 2));
    return true;
}
