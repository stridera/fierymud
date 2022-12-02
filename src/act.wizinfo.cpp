/***************************************************************************
 *   File: act.wizinfo.c                                  Part of FieryMUD *
 *  Usage: Informative player-level god commands                           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "ai.hpp"
#include "casting.hpp"
#include "charsize.hpp"
#include "clan.hpp"
#include "class.hpp"
#include "cleric.hpp"
#include "comm.hpp"
#include "commands.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "cooldowns.hpp"
#include "corpse_save.hpp"
#include "damage.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "events.hpp"
#include "exits.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "house.hpp"
#include "interpreter.hpp"
#include "lifeforce.hpp"
#include "limits.hpp"
#include "messages.hpp"
#include "modify.hpp"
#include "olc.hpp"
#include "pfiles.hpp"
#include "players.hpp"
#include "quest.hpp"
#include "races.hpp"
#include "rogue.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "sorcerer.hpp"
#include "spell_mem.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"
#include "version.hpp"
#include "vsearch.hpp"
#include "weather.hpp"

#include <fmt/format.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

void garble_text(char *string, int percent);
void dismount_char(CharData *ch);
void check_new_surroundings(CharData *ch, bool old_room_was_dark, bool tx_obvious);

int Valid_Name(char *newname);
int reserved_word(char *argument);

/* extern functions */
void send_to_xnames(char *name);
int find_zone(int num);
void cure_laryngitis(CharData *ch);
void reboot_mud_prep();
void rebootwarning(int minutes);

/* Internal funct */
void do_wiztitle(char *outbuf, CharData *vict, char *argu);

#define ZOCMD zone_table[zrnum].cmd[subcmd]

void list_zone_commands_room(CharData *ch, char *buf, room_num rvnum) {
    int zrnum = find_real_zone_by_room(rvnum);
    int rrnum = real_room(rvnum), cmd_room = NOWHERE;
    int subcmd = 0, count = 0;

    if (zrnum == NOWHERE || rrnum == NOWHERE) {
        strcpy(buf, "No zone information available.\n");
        return;
    }

    get_char_cols(ch);

    sprintf(buf, "Zone commands in this room:%s\n", yel);
    while (ZOCMD.command != 'S') {
        switch (ZOCMD.command) {
        case 'M':
        case 'O':
        case 'V':
            cmd_room = ZOCMD.arg3;
            break;
        case 'D':
        case 'R':
            cmd_room = ZOCMD.arg1;
            break;
        default:
            break;
        }
        if (cmd_room == rrnum) {
            count++;
            /* start listing */
            switch (ZOCMD.command) {
            case 'M':
                sprintf(buf1, "%sLoad %s [%s%d%s], Max : %d\n", ZOCMD.if_flag ? " then " : "",
                        mob_proto[ZOCMD.arg1].player.short_descr, cyn, mob_index[ZOCMD.arg1].vnum, yel, ZOCMD.arg2);
                break;
            case 'G':
                sprintf(buf1, "%sGive it %s [%s%d%s], Max : %d\n", ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel, ZOCMD.arg2);
                break;
            case 'O':
                sprintf(buf1, "%sLoad %s [%s%d%s], Max : %d\n", ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel, ZOCMD.arg2);
                break;
            case 'E':
                sprintf(buf1, "%sEquip with %s [%s%d%s], %s, Max : %d\n", ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel,
                        equipment_types[ZOCMD.arg3], ZOCMD.arg2);
                break;
            case 'P':
                sprintf(buf1, "%sPut %s [%s%d%s] in %s [%s%d%s], Max : %d\n", ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel,
                        obj_proto[ZOCMD.arg3].short_description, cyn, obj_index[ZOCMD.arg3].vnum, yel, ZOCMD.arg2);
                break;
            case 'R':
                sprintf(buf1, "%sRemove %s [%s%d%s] from room.\n", ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg2].short_description, cyn, obj_index[ZOCMD.arg2].vnum, yel);
                break;
            case 'D':
                sprintf(buf1, "%sSet door %s as %s.\n", ZOCMD.if_flag ? " then " : "", dirs[ZOCMD.arg2],
                        ZOCMD.arg3 ? ((ZOCMD.arg3 == 1)
                                          ? "closed"
                                          : ((ZOCMD.arg3 == 2)
                                                 ? "locked"
                                                 : ((ZOCMD.arg3 == 3) ? "hidden"
                                                                      : ((ZOCMD.arg3 == 4) ? "hidden/closed/locked"
                                                                                           : "hidden/closed"))))
                                   : "open");
                break;
            default:
                strcpy(buf1, "<Unknown Command>\n");
                break;
            }
            sprintf(buf, "%s%s%3d - %s%s", buf, nrm, count, yel, buf1);
        }
        subcmd++;
    }
    strcat(buf, nrm);
    if (!count)
        strcat(buf, "  None.\n");
}

void stat_extra_descs(ExtraDescriptionData *ed, CharData *ch, char *buf, bool showtext) {
    ExtraDescriptionData *desc;
    int count;

    if (!ed) {
        strcpy(buf, "No extra descs.\n");
        return;
    }

    if (showtext) {
        strcpy(buf, "");
        for (desc = ed; desc; desc = desc->next)
            sprintf(buf, "%s&4*&0 %s%s%s\n%s\n", buf, CLR(ch, FCYN), desc->keyword, CLR(ch, ANRM), desc->description);
    } else {
        count = 0;
        for (desc = ed; desc; desc = desc->next)
            count++;
        sprintf(buf, "Extra desc%s (%d):%s", ed->next ? "s" : "", count, CLR(ch, FCYN));
        for (desc = ed; desc; desc = desc->next) {
            if (desc != ed)
                strcat(buf, ",");
            strcat(buf, " ");
            strcat(buf, desc->keyword);
        }
        strcat(buf, CLR(ch, ANRM));
        strcat(buf, "\n");
    }
}

void do_stat_room(CharData *ch, int rrnum) {
    RoomData *rm = &world[rrnum];
    int i, found = 0;
    ObjData *j = 0;
    CharData *k = 0;
    RoomEffectNode *reff;
    const sectordef *sector;
    std::string resp;

    if (!ch->desc)
        return;

    sector = &sectors[CH_SECT(ch)];

    resp += fmt::format("Room name: {}{}{}\n", CLR(ch, FCYN), rm->name, CLR(ch, ANRM));

    sprintf(buf2, "%s%s&0 (%d mv)", sector->color, sector->name, sector->mv);
    resp += fmt::format("Zone: [{}], VNum: [{}{}{}], RNum: [{}], Sector: {}\n", rm->zone, CLR(ch, FGRN), rm->vnum,
                        CLR(ch, ANRM), rrnum, buf2);

    sprintflag(buf2, rm->room_flags, NUM_ROOM_FLAGS, room_bits);
    resp += fmt::format("SpecProc: {}, Flags: {}\n", (rm->func == nullptr) ? "None" : "Exists", buf2);

    sprintflag(buf2, rm->room_effects, NUM_ROOM_EFF_FLAGS, room_effects);
    resp += fmt::format("Room effects: {}\n", buf2);

    resp += fmt::format("Ambient Light : {}\n", rm->light);

    resp += "Description:\n";
    resp += rm->description ? rm->description : "  None.\n";

    stat_extra_descs(rm->ex_description, ch, buf, false);
    resp += buf;

    sprintf(buf2, "Chars present:%s", CLR(ch, FYEL));
    for (found = 0, k = rm->people; k; k = k->next_in_room) {
        if (!CAN_SEE(ch, k))
            continue;
        sprintf(buf2, "%s%s %s(%s)", buf2, found++ ? "," : "", GET_NAME(k),
                (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
        if (strlen(buf2) >= 62) {
            resp += fmt::format("{}{}\n", buf2, k->next_in_room ? "," : "");
            *buf2 = found = 0;
        }
    }
    if (*buf2)
        resp += fmt::format("{}\n", buf2);
    resp += CLR(ch, ANRM);

    if (rm->contents) {
        sprintf(buf2, "Contents:%s", CLR(ch, FGRN));
        for (found = 0, j = rm->contents; j; j = j->next_content) {
            if (!CAN_SEE_OBJ(ch, j))
                continue;
            sprintf(buf2, "%s%s %s", buf2, found++ ? "," : "", j->short_description);
            if (strlen(buf2) >= 62) {
                resp += fmt::format("{}{}\n", buf2, j->next_content ? "," : "");
                *buf2 = found = 0;
            }
        }

        if (*buf2)
            resp += fmt::format("{}\n", buf2);
        resp += CLR(ch, ANRM);
    }

    for (i = 0; i < NUM_OF_DIRS; i++) {
        if (rm->exits[i]) {
            if (rm->exits[i]->to_room == NOWHERE)
                sprintf(buf1, " %sNONE%s", CLR(ch, FCYN), CLR(ch, ANRM));
            else
                sprintf(buf1, "%s%5d%s", CLR(ch, FCYN), world[rm->exits[i]->to_room].vnum, CLR(ch, ANRM));
            sprintbit(rm->exits[i]->exit_info, exit_bits, buf2);
            resp += fmt::format("Exit {}{:5}{}:  To: [{}], Key: [{:5}], Keywrd: {}, Type: {}\n", CLR(ch, FCYN), dirs[i],
                                CLR(ch, ANRM), buf1, rm->exits[i]->key,
                                rm->exits[i]->keyword ? rm->exits[i]->keyword : "None", buf2);
            if (rm->exits[i]->general_description)
                resp += rm->exits[i]->general_description;
        }
    }

    /* Mention spells/effects */
    for (reff = room_effect_list; reff; reff = reff->next) {
        if (reff->room == rrnum) {
            sprinttype(reff->effect, room_effects, buf2);
            resp += fmt::format("SPL: ({:3}) &6{:21}&0, sets {}\n", reff->timer, skills[reff->spell].name, buf2);
        }
    }

    list_zone_commands_room(ch, buf, rm->vnum);
    resp += buf;

    /* check the room for a script */
    do_sstat_room(ch, buf, rm);
    resp += buf;

    page_string(ch, resp.c_str());
}

// TODO: Update to return a string
void stat_spellbook(ObjData *obj, char *buf) {
    int spage = 0, fpage = 0;
    SpellBookList *entry;
    char list_buf[MAX_STRING_LENGTH];

    list_buf[0] = 0;

    strcpy(buf, "&7&b---Spellbook Contents---&0\n");

    /* If we can't get a list of the spells in the book,
     * we need to say *exactly* why. */
    if (!obj->spell_book) {
        strcat(buf, "&1&b*&0 Its spell list is NULL.\n");
        return;
    } else if (!obj->spell_book->spell) {
        strcat(buf, "&1&b*&0 The first spell in its spell list is 0.\n");
        return;
    }

    for (entry = obj->spell_book; entry; entry = entry->next) {
        spage = fpage + 1;
        fpage = fpage + entry->length;
        sprintf(list_buf, "%s&6%3d-%3d)&0 &6&b%s&0\n", list_buf, spage, fpage, skills[entry->spell].name);
    }

    if ((GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES) - fpage) > 1) {
        sprintf(list_buf, "%s&6%3d-%3d)&0 &4&b--Blank--&0\n", list_buf, fpage + 1,
                GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES));
    } else if ((GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES) - fpage) == 1) {
        sprintf(list_buf, "%s    &6%3d)&0 &4&b--Blank--&0\n", list_buf, GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES));
    }

    strcat(buf, list_buf);
}

void do_stat_object(CharData *ch, ObjData *j) {
    std::string resp;
    int i, vnum, found;
    ObjData *j2;
    Event *e;

    if (!ch->desc)
        return;

    vnum = GET_OBJ_VNUM(j);
    resp +=
        fmt::format("Name: '{}{}{}', Aliases: {}, Level: {}\n", CLR(ch, FYEL),
                    j->short_description ? j->short_description : "<None>", CLR(ch, ANRM), j->name, GET_OBJ_LEVEL(j));

    if (GET_OBJ_RNUM(j) >= 0)
        strcpy(buf2, obj_index[GET_OBJ_RNUM(j)].func ? "Exists" : "None");
    else
        strcpy(buf2, "None");
    resp += fmt::format("VNum: [{}{:5d}{}], RNum: [{:5d}], Type: {}, SpecProc: {}\n", CLR(ch, FGRN), vnum,
                        CLR(ch, ANRM), GET_OBJ_RNUM(j), OBJ_TYPE_NAME(j), buf2);

    resp += fmt::format("L-Des: {}\n", j->description ? j->description : "None");

    if (j->action_description)
        resp += fmt::format("Action desc:\n{}{}{}\n", CLR(ch, FYEL), j->action_description, CLR(ch, ANRM));

    sprintbit(j->obj_flags.wear_flags, wear_bits, buf1);
    resp += fmt::format("Can be worn on: {}{}{}\n", CLR(ch, FCYN), buf1, CLR(ch, ANRM));

    sprintflag(buf1, GET_OBJ_FLAGS(j), NUM_ITEM_FLAGS, extra_bits);
    resp += fmt::format("Extra flags   : {}{}{}\n", CLR(ch, FGRN), buf1, CLR(ch, ANRM));

    *buf1 = '\0';
    sprintflag(buf1, GET_OBJ_EFF_FLAGS(j), NUM_EFF_FLAGS, effect_flags);
    resp += fmt::format("Spell Effects : {}{}{}\n", CLR(ch, FYEL), buf1, CLR(ch, ANRM));

    resp += fmt::format(
        "Weight: {:.2f}, Value: {}, "
        "Timer: {}, Decomp time: {}, Hiddenness: {}\n",
        GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_TIMER(j), GET_OBJ_DECOMP(j), GET_OBJ_HIDDENNESS(j));

    if (j->in_room == NOWHERE)
        strcpy(buf1, "Nowhere");
    else
        sprintf(buf1, "%d", world[j->in_room].vnum);
    resp +=
        fmt::format("In room: {}, In object: {}, Carried by: {}, Worn by: {}\n", buf1,
                    j->in_obj ? j->in_obj->short_description : "None",
                    j->carried_by ? GET_NAME(j->carried_by) : "Nobody", j->worn_by ? GET_NAME(j->worn_by) : "Nobody");

    switch (GET_OBJ_TYPE(j)) {
    case ITEM_LIGHT:
        if (GET_OBJ_VAL(j, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT)
            resp += fmt::format("Hours left: Infinite\n");
        else
            resp += fmt::format("Hours left: [{}]  Initial hours: [{}]\n", GET_OBJ_VAL(j, VAL_LIGHT_REMAINING),
                                GET_OBJ_VAL(j, VAL_LIGHT_CAPACITY));
        break;
    case ITEM_SCROLL:
    case ITEM_POTION:
        resp +=
            fmt::format("Spells: (Level {}) {}, {}, {}\n", GET_OBJ_VAL(j, VAL_SCROLL_LEVEL),
                        skill_name(GET_OBJ_VAL(j, VAL_SCROLL_SPELL_1)), skill_name(GET_OBJ_VAL(j, VAL_SCROLL_SPELL_2)),
                        skill_name(GET_OBJ_VAL(j, VAL_SCROLL_SPELL_3)));
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        resp += fmt::format("Spell: {} at level {}, {} (of {}) charges remaining\n",
                            skill_name(GET_OBJ_VAL(j, VAL_WAND_SPELL)), GET_OBJ_VAL(j, VAL_WAND_LEVEL),
                            GET_OBJ_VAL(j, VAL_WAND_CHARGES_LEFT), GET_OBJ_VAL(j, VAL_WAND_MAX_CHARGES));
        break;
    case ITEM_WEAPON:
        resp += fmt::format(
            "Todam: {}d{} (avg {:.1f}), Message type: {}, '{}'\n", GET_OBJ_VAL(j, VAL_WEAPON_DICE_NUM),
            GET_OBJ_VAL(j, VAL_WEAPON_DICE_SIZE), WEAPON_AVERAGE(j), GET_OBJ_VAL(j, VAL_WEAPON_DAM_TYPE),
            GET_OBJ_VAL(j, VAL_WEAPON_DAM_TYPE) >= 0 && GET_OBJ_VAL(j, VAL_WEAPON_DAM_TYPE) <= TYPE_ALIGN - TYPE_HIT
                ? attack_hit_text[GET_OBJ_VAL(j, VAL_WEAPON_DAM_TYPE)].singular
                : "<&1INVALID&0>");
        break;
    case ITEM_ARMOR:
    case ITEM_TREASURE:
        resp += fmt::format("AC-apply: [{}]\n", GET_OBJ_VAL(j, VAL_ARMOR_AC));
        break;
    case ITEM_TRAP:
        resp += fmt::format("Spell: {}, - Hitpoints: {}\n", GET_OBJ_VAL(j, VAL_TRAP_SPELL),
                            GET_OBJ_VAL(j, VAL_TRAP_HITPOINTS));
        break;
    case ITEM_CONTAINER:
        if (!IS_CORPSE(j)) {
            sprintbit(GET_OBJ_VAL(j, VAL_CONTAINER_BITS), container_bits, buf2);
            resp += fmt::format("Weight capacity: {}, Lock Type: {}, Key Num: {}, Weight Reduction: {}%, Corpse: {}\n",
                                GET_OBJ_VAL(j, VAL_CONTAINER_CAPACITY), buf2, GET_OBJ_VAL(j, VAL_CONTAINER_KEY),
                                GET_OBJ_VAL(j, VAL_CONTAINER_WEIGHT_REDUCTION), YESNO(IS_CORPSE(j)));
        } else {
            resp += fmt::format(
                "Weight capacity: {}, Id: {}, Corpse: {}, Player "
                "Corpse: {}, Raisable: {}\n",
                GET_OBJ_VAL(j, VAL_CONTAINER_CAPACITY), GET_OBJ_VAL(j, VAL_CORPSE_ID), YESNO(IS_CORPSE(j)),
                YESNO(IS_PLR_CORPSE(j)), YESNO(GET_OBJ_VAL(j, VAL_CONTAINER_CORPSE) == CORPSE_NPC));
        }
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        resp += fmt::format("Capacity: {}, Contains: {}, Poisoned: {}, Liquid: {}\n",
                            GET_OBJ_VAL(j, VAL_DRINKCON_CAPACITY), GET_OBJ_VAL(j, VAL_DRINKCON_REMAINING),
                            YESNO(IS_POISONED(j)), LIQ_NAME(GET_OBJ_VAL(j, VAL_DRINKCON_LIQUID)));
        break;
    case ITEM_NOTE:
        resp += fmt::format("Tongue: {}\n", GET_OBJ_VAL(j, 0));
        break;
    case ITEM_KEY:
        break;
    case ITEM_FOOD:
        resp +=
            fmt::format("Makes full: {}, Poisoned: {}\n", GET_OBJ_VAL(j, VAL_FOOD_FILLINGNESS), YESNO(IS_POISONED(j)));
        break;
    case ITEM_MONEY:
        resp +=
            fmt::format("Coins: {}p {}g {}s {}c\n", GET_OBJ_VAL(j, VAL_MONEY_PLATINUM), GET_OBJ_VAL(j, VAL_MONEY_GOLD),
                        GET_OBJ_VAL(j, VAL_MONEY_SILVER), GET_OBJ_VAL(j, VAL_MONEY_COPPER));
        break;
    case ITEM_PORTAL:
        resp += fmt::format("To room: {}\n", GET_OBJ_VAL(j, VAL_PORTAL_DESTINATION));
        if (GET_OBJ_VAL(j, VAL_PORTAL_ENTRY_MSG) >= 0) {
            for (i = 0; i < GET_OBJ_VAL(j, VAL_PORTAL_ENTRY_MSG) && *portal_entry_messages[i] != '\n'; ++i)
                ;
            if (*portal_entry_messages[i] != '\n')
                resp += fmt::format("Entry-Room message: {}", portal_entry_messages[i]);
        }
        if (GET_OBJ_VAL(j, VAL_PORTAL_CHAR_MSG) >= 0) {
            for (i = 0; i < GET_OBJ_VAL(j, VAL_PORTAL_CHAR_MSG) && *portal_character_messages[i] != '\n'; ++i)
                ;
            if (*portal_character_messages[i] != '\n')
                resp += fmt::format("To-Char message   : {}", portal_character_messages[i]);
        }
        if (GET_OBJ_VAL(j, VAL_PORTAL_EXIT_MSG) >= 0) {
            for (i = 0; i < GET_OBJ_VAL(j, VAL_PORTAL_EXIT_MSG) && *portal_exit_messages[i] != '\n'; ++i)
                ;
            if (*portal_exit_messages[i] != '\n')
                resp += fmt::format("Exit-Room message : {}", portal_exit_messages[i]);
        }
        break;
    default:
        resp +=
            fmt::format("Values: [{}] [{}] [{}] [{}] [{}] [{}] [{}]\n", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
                        GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3), GET_OBJ_VAL(j, 4), GET_OBJ_VAL(j, 5), GET_OBJ_VAL(j, 6));
        break;
    }

    /*
     * I deleted the "equipment status" code from here because it seemed
     * more or less useless and just takes up valuable screen space.
     */

    if (j->contains) {
        sprintf(buf1, "\nContents:%s", CLR(ch, FGRN));
        for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
            sprintf(buf1, "%s%s %s", buf1, found++ ? "," : "", j2->short_description);
            if (strlen(buf1) >= 62) {
                resp += fmt::format("{}{}\n", buf1, j2->next_content ? "," : "");
                *buf1 = found = 0;
            }
        }

        if (*buf1)
            resp += fmt::format("{}\n", buf1);
        resp += CLR(ch, ANRM);
    }

    found = 0;
    resp += "Applies:";
    for (i = 0; i < MAX_OBJ_APPLIES; i++)
        if (j->applies[i].modifier) {
            sprinttype(j->applies[i].location, apply_types, buf2);
            resp +=
                fmt::format("{} {}", found++ ? "," : "", format_apply(j->applies[i].location, j->applies[i].modifier));
        }
    resp += fmt::format("{}\n", found ? "" : " None");

    if (GET_OBJ_TYPE(j) == ITEM_SPELLBOOK) {
        stat_spellbook(j, buf);
        resp += buf;
    }

    stat_extra_descs(j->ex_description, ch, buf, false);
    resp += buf;

    /* Report any events attached. */
    if (j->events) {
        e = j->events;
        resp += fmt::format("Events: {}", eventname(e));
        while (e->next) {
            e = e->next;
            resp += fmt::format(" {}", eventname(e));
        }
        resp += "\n";
    } else {
        resp += "No events.\n";
    }

    /* check the object for a script */
    do_sstat_object(ch, buf, j);
    resp += buf;

    page_string(ch, resp.c_str());
}

ACMD(do_estat) {
    ObjData *obj;
    char *otarg;
    int tmp, r_num;

    half_chop(argument, buf1, buf2);

    if (!ch->desc)
        return;

    if (subcmd == SCMD_RESTAT || !strcasecmp(buf1, "room")) {
        if (subcmd == SCMD_RESTAT && *buf1)
            tmp = isdigit(*buf1) ? real_room(atoi(buf1)) : NOWHERE;
        else if (subcmd != SCMD_RESTAT && *buf2)
            tmp = isdigit(*buf2) ? real_room(atoi(buf2)) : NOWHERE;
        else
            tmp = ch->in_room;
        if (tmp == NOWHERE)
            send_to_char("No such room.\n", ch);
        else {
            stat_extra_descs(world[tmp].ex_description, ch, buf, true);
            page_string(ch, buf);
        }
    } else {
        if (subcmd == SCMD_OESTAT)
            otarg = buf1;
        else if (!strcasecmp(buf1, "obj"))
            otarg = buf2;
        else
            otarg = buf1;

        if (!*otarg) {
            send_to_char("Usage: estat room [<vnum>]\n", ch);
            send_to_char("       estat obj <name>\n", ch);
        } else if (isdigit(*otarg)) {
            tmp = atoi(otarg);
            if ((r_num = real_object(tmp)) < 0)
                send_to_char("There is no object with that number.\n", ch);
            else {
                stat_extra_descs(obj_proto[r_num].ex_description, ch, buf, true);
                page_string(ch, buf);
            }
        } else if ((obj = find_obj_around_char(ch, find_vis_by_name(ch, otarg)))) {
            stat_extra_descs(obj->ex_description, ch, buf, true);
            page_string(ch, buf);
        } else {
            send_to_char("No such object around.\n", ch);
        }
    }
}

void do_stat_character(CharData *ch, CharData *k) {
    int i, found, a, mprinted;
    GroupType *g;
    FollowType *fol;
    effect *eff;
    QuestList *quest;
    Event *e;
    CharData *m;
    MemoryRec *memory;
    std::string resp;

    if (!ch->desc)
        return;

    /* Vital character data */
    resp += fmt::format("{} '{}'  IDNum: [{:5}], In room [{:5}]\n", (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
                        GET_NAME(k), GET_IDNUM(k), k->in_room != NOWHERE ? world[k->in_room].vnum : 0);

    /* Prototyped mobile specific data. */
    if (IS_MOB(k))
        resp +=
            fmt::format("Alias: {}, VNum: [{:5}], RNum: [{:5}]\n", GET_NAMELIST(k), GET_MOB_VNUM(k), GET_MOB_RNUM(k));

    /* Strings */
    if (IS_NPC(k)) {
        resp += fmt::format("L-Des: {}\n", (GET_LDESC(k) ? GET_LDESC(k) : "<None>"));
        if (k->player.description) {
            resp += fmt::format("Desc: {}\n", k->player.description);
        }
    } else
        resp += fmt::format("Title: {}\n", (k->player.title ? k->player.title : "<None>"));

    /* Various character stats */
    resp += fmt::format("Race: {}, Race Align: {}, ", RACE_PLAINNAME(k), RACE_ALIGN_ABBR(k));
    /*
      if (!IS_NPC(k))
        resp += fmt::format( "Deity: {}, ", GET_DIETY(ch) >= 0 ?
                Dieties[(int) GET_DIETY(ch)].diety_name : "None");
    */
    sprinttype(GET_SEX(k), genders, buf1);
    resp += fmt::format("Size: {}, Gender: {}\n", capitalize(SIZE_DESC(k)), buf1);
    resp += fmt::format("Life force: {}{}&0, Composition: {}{}&0\n", LIFEFORCE_COLOR(k), capitalize(LIFEFORCE_NAME(k)),
                        COMPOSITION_COLOR(k), capitalize(COMPOSITION_NAME(k)));
    resp += fmt::format("Class: {}, Lev: [{}{:2d}{}], XP: [{}{:7}{}], Align: [{:4d}]\n", CLASS_FULL(k), CLR(ch, FYEL),
                        GET_LEVEL(k), CLR(ch, ANRM), CLR(ch, FYEL), GET_EXP(k), CLR(ch, ANRM), GET_ALIGNMENT(k));

    /* Player specific data. */
    if (!IS_NPC(k)) {
        strftime(buf1, MAX_STRING_LENGTH, TIMEFMT_DATE, localtime(&(k->player.time.birth)));
        strftime(buf2, MAX_STRING_LENGTH, TIMEFMT_DATE, localtime(&(k->player.time.logon)));

        resp += fmt::format(
            "Created: [{}], Last Logon: [{}], Played: [{}h {}m]\n"
            "Age: [{}], Homeroom: [{}], Speaks: [{}/{}/{}]",
            buf1, buf2, k->player.time.played / 3600, ((k->player.time.played / 3600) % 60), age(k).year,
            GET_HOMEROOM(k), GET_TALK(k, 0), GET_TALK(k, 1), GET_TALK(k, 2));

        if (GET_CLAN(k)) {
            resp += fmt::format(", Clan: [{}], Rank: [{}]", GET_CLAN(k)->abbreviation, GET_CLAN_RANK(k));
        }

        /* Display OLC zones for immorts */
        if (GET_LEVEL(k) >= LVL_IMMORT) {
            OLCZoneList *zone;
            *buf1 = '\0';
            for (zone = GET_OLC_ZONES(k); zone; zone = zone->next)
                sprintf(buf1, "%s%d%s", buf1, zone->zone, zone->next ? ", " : "");
            resp += fmt::format(", OLC Zones: [{}]", *buf1 ? buf1 : "NONE");
        }

        resp += "\n";
    }

    /* Attributes, points, and money. */
    resp += fmt::format(
        "         STR   INT   WIS   DEX   CON   CHA\n"
        "ACTUAL   {}{:3}   {:3}   {:3}   {:3}   {:3}   {:3}{}\n"
        "NATURAL  {}{:3}   {:3}   {:3}   {:3}   {:3}   {:3}{}\n"
        "AFFECTED {}{:3}   {:3}   {:3}   {:3}   {:3}   {:3}{}\n",
        CLR(ch, FCYN), GET_ACTUAL_STR(k), GET_ACTUAL_INT(k), GET_ACTUAL_WIS(k), GET_ACTUAL_DEX(k), GET_ACTUAL_CON(k),
        GET_ACTUAL_CHA(k), CLR(ch, ANRM), CLR(ch, FCYN), GET_NATURAL_STR(k), GET_NATURAL_INT(k), GET_NATURAL_WIS(k),
        GET_NATURAL_DEX(k), GET_NATURAL_CON(k), GET_NATURAL_CHA(k), CLR(ch, ANRM), CLR(ch, FCYN), GET_AFFECTED_STR(k),
        GET_AFFECTED_INT(k), GET_AFFECTED_WIS(k), GET_AFFECTED_DEX(k), GET_AFFECTED_CON(k), GET_AFFECTED_CHA(k),
        CLR(ch, ANRM));

    resp += fmt::format("HP: [{}{}/{}+{}{}]  MP: [{}{}/{}+{}{}]  MV: [{}{}/{}+{}{}]\n", CLR(ch, FGRN), GET_HIT(k),
                        GET_MAX_HIT(k), hit_gain(k), CLR(ch, ANRM), CLR(ch, FGRN), GET_MANA(k), GET_MAX_MANA(k),
                        mana_gain(k), CLR(ch, ANRM), CLR(ch, FGRN), GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k),
                        CLR(ch, ANRM));

    resp += fmt::format(
        "Coins: [{}{}{}p / {}{}{}g / {}{}{}s / {}{}{}c], "
        "Bank: [{}{}{}p / {}{}{}g / {}{}{}s / {}{}{}c]\n",
        CLR(ch, FCYN), GET_PLATINUM(k), CLR(ch, ANRM), CLR(ch, HYEL), GET_GOLD(k), CLR(ch, ANRM), CLR(ch, FWHT),
        GET_SILVER(k), CLR(ch, ANRM), CLR(ch, FYEL), GET_COPPER(k), CLR(ch, ANRM), CLR(ch, FCYN), GET_BANK_PLATINUM(k),
        CLR(ch, ANRM), CLR(ch, HYEL), GET_BANK_GOLD(k), CLR(ch, ANRM), CLR(ch, FWHT), GET_BANK_SILVER(k), CLR(ch, ANRM),
        CLR(ch, FYEL), GET_BANK_COPPER(k), CLR(ch, ANRM));

    resp += fmt::format("AC: [{}/10], Hitroll: [{:2}], Damroll: [{:2}], ", GET_AC(k) + 5 * monk_weight_penalty(k),
                        GET_HITROLL(k) - monk_weight_penalty(k), GET_DAMROLL(k) - monk_weight_penalty(k));
    resp += fmt::format("Saving throws: [{}/{}/{}/{}/{}]\n", GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2),
                        GET_SAVE(k, 3), GET_SAVE(k, 4));
    resp += fmt::format("Perception: [{:4}], Hiddenness: [{:4}], Rage: [{:4}]\n", GET_PERCEPTION(k), GET_HIDDENNESS(k),
                        GET_RAGE(k));

    /* Status data. */
    sprinttype(GET_POS(k), position_types, buf1);
    sprinttype(GET_STANCE(k), stance_types, buf2);
    resp += fmt::format("Pos: {} ({})", buf1, buf2);
    if (IS_NPC(k)) {
        sprinttype(k->mob_specials.default_pos, position_types, buf1);
        resp += fmt::format(", Default Pos: {}", buf1);
    }
    resp += fmt::format(", Fighting: {}", FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "<none>");
    if (k->forward)
        resp +=
            fmt::format(", {} into: {}", GET_LEVEL(k) > LVL_IMMORT ? "Switched" : "Shapechanged", GET_NAME(k->forward));
    resp += "\n";

    *buf2 = '\0';
    if (!IS_NPC(k))
        sprintf(buf2, "Idle: [%d tic%s]", k->char_specials.timer, k->char_specials.timer == 1 ? "" : "s");
    if (k->desc) {
        sprinttype(k->desc->connected, connected_types, buf1);
        sprintf(buf2, "%s%sConnected: %s", buf2, *buf2 ? ", " : "", buf1);
    }
    if (POSSESSED(k))
        sprintf(buf2, "%s%s%s into by: %s", buf2, *buf2 ? ", " : "",
                GET_LEVEL(POSSESSOR(k)) > LVL_IMMORT ? "Switched" : "Shapechanged", GET_NAME(POSSESSOR(k)));
    if (*buf2)
        resp += fmt::format("{}\n", buf2);

    if (IS_MOB(k)) {
        if (k->mob_specials.attack_type >= 0 && k->mob_specials.attack_type <= TYPE_STAB - TYPE_HIT)
            strcpy(buf2, attack_hit_text[k->mob_specials.attack_type].singular);
        else
            strcpy(buf2, "<&1INVALID&0>");
        resp += fmt::format("Mob Spec-Proc: {}, NPC Bare Hand Dam: {}d{}, Attack type: {}\n",
                            (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"), k->mob_specials.damnodice,
                            k->mob_specials.damsizedice, buf2);
    }

    /* Character flags. */
    if (IS_NPC(k)) {
        sprintflag(buf1, MOB_FLAGS(k), NUM_MOB_FLAGS, action_bits);
        resp += fmt::format("NPC flags: {}{}{}\n", CLR(ch, FCYN), buf1, CLR(ch, ANRM));
    } else {
        sprintflag(buf2, PLR_FLAGS(k), NUM_PLR_FLAGS, player_bits);
        resp += fmt::format("PLR: {}{}{}\n", CLR(ch, FCYN), buf2, CLR(ch, ANRM));
        sprintflag(buf2, PRF_FLAGS(k), NUM_PRF_FLAGS, preference_bits);
        resp += fmt::format("PRF: {}{}{}\n", CLR(ch, FGRN), buf2, CLR(ch, ANRM));
        sprintflag(buf2, PRV_FLAGS(k), NUM_PRV_FLAGS, privilege_bits);
        resp += fmt::format("PRV: {}{}{}\n", CLR(ch, FGRN), buf2, CLR(ch, ANRM));
    }

    /* Weight and objects. */
    for (i = a = found = 0; i < NUM_WEARS; i++)
        if (GET_EQ(k, i)) {
            ++found;
            a += GET_OBJ_WEIGHT(GET_EQ(k, i));
        }
    resp += fmt::format(
        "Max Carry: {} ({} weight); "
        "Carried: {} ({:.2f} weight); "
        "Worn: {} ({} weight)\n",
        CAN_CARRY_N(k), CAN_CARRY_W(k), IS_CARRYING_N(k), IS_CARRYING_W(k), found, a);

    /* Conditions. */
    resp += "Hunger: ";
    if (GET_COND(k, FULL) < 0)
        resp += "Off";
    else
        resp += fmt::format("{}", GET_COND(k, FULL));
    resp += ", Thirst: ";
    if (GET_COND(k, THIRST) < 0)
        resp += "Off";
    else
        resp += fmt::format("{}", GET_COND(k, THIRST));
    resp += ", Drunk: ";
    if (GET_COND(k, DRUNK) < 0)
        resp += "Off\n";
    else
        resp += fmt::format("{}\n", GET_COND(k, DRUNK));

    /* Group, follower, guard, etc. */
    sprintf(buf1, "Consented: %s, Master is: %s, Followers are:", (CONSENT(ch) ? GET_NAME(CONSENT(ch)) : "<none>"),
            (k->master ? GET_NAME(k->master) : "<none>"));

    found = 0;
    for (fol = k->followers; fol; fol = fol->next) {
        if (strlen(buf1) + strlen(PERS(fol->follower, ch)) >= 78) {
            resp += fmt::format("{}{}\n", buf1, found ? "," : "");
            *buf1 = found = 0;
        }
        sprintf(buf1, "%s%s %s", buf1, found++ ? "," : "", PERS(fol->follower, ch));
    }
    if (*buf1)
        resp += fmt::format("{}\n", buf1);

    /* Group list */
    sprintf(buf1, "&0&2&bGroup Master&0 is: %s, &0&2&bgroupees are:&0",
            ((k->group_master) ? GET_NAME(k->group_master) : "<none>"));

    found = 0;
    for (g = k->groupees; g; g = g->next) {
        if (strlen(buf1) + strlen(PERS(g->groupee, ch)) >= 78) {
            resp += fmt::format("{}{}\n", buf1, found ? "," : "");
            *buf1 = found = 0;
        }
        sprintf(buf1, "%s%s %s", buf1, found++ ? "," : "", PERS(g->groupee, ch));
    }
    if (*buf1)
        resp += fmt::format("{}\n", buf1);

    if (k->guarding || k->guarded_by)
        resp += fmt::format("Guarding: {}, Guarded by: {}\n", k->guarding ? GET_NAME(k->guarding) : "<none>",
                            k->guarded_by ? GET_NAME(k->guarded_by) : "<none>");
    if (k->cornering || k->cornered_by)
        resp += fmt::format("Cornering: {}, Cornered by: {}\n", k->cornering ? GET_NAME(k->cornering) : "<none>",
                            k->cornered_by ? GET_NAME(k->cornered_by) : "<none>");

    /* Effect bitvectors */
    sprintflag(buf1, EFF_FLAGS(k), NUM_EFF_FLAGS, effect_flags);
    resp += fmt::format("EFF: {}{}{}\n", CLR(ch, FYEL), buf1, CLR(ch, ANRM));

    /* NPC spell circle status */
    if (IS_NPC(k) && MEM_MODE(ch) != MEM_NONE) {
        const char *color;
        resp += "Spellbank: ";
        for (i = 1; i <= NUM_SPELL_CIRCLES; ++i)
            if (spells_of_circle[(int)GET_LEVEL(k)][i]) {
                a = (GET_MOB_SPLBANK(k, i) * 100) / spells_of_circle[(int)GET_LEVEL(k)][i];
                if (a >= 90)
                    color = AHGRN;
                else if (a >= 75)
                    color = AFGRN;
                else if (a >= 60)
                    color = AHYEL;
                else if (a >= 40)
                    color = AFYEL;
                else if (a >= 20)
                    color = AFRED;
                else
                    color = AHRED;
                resp += fmt::format("{}({}{}@0) ", i, color, GET_MOB_SPLBANK(k, i));
            }
        resp += "\n";
    }

    /* List spells the character is affected by. */
    for (eff = k->effects; eff; eff = eff->next) {
        if (eff->duration < 0)
            resp += fmt::format("SPL: (perma) {}{:<21}{} ", CLR(ch, FCYN), skills[eff->type].name, CLR(ch, ANRM));
        else
            resp += fmt::format("SPL: ({:3}hr) {}{:<21}{} ", eff->duration + 1, CLR(ch, FCYN), skills[eff->type].name,
                                CLR(ch, ANRM));
        if (eff->modifier)
            resp += fmt::format("{:+d} to {}", eff->modifier, apply_types[(int)eff->location]);
        if (HAS_FLAGS(eff->flags, NUM_EFF_FLAGS)) {
            sprintflag(buf1, eff->flags, NUM_EFF_FLAGS, effect_flags);
            resp += fmt::format("{}sets {}", eff->modifier ? ", " : "", buf1);
        }
        resp += "\n";
    }

    /* Run through the quests the player is on */
    if (k->quests) {
        if (all_quests) {
            quest = k->quests;
            while (quest) {
                if ((a = real_quest(quest->quest_id)) >= 0) {
                    resp += fmt::format("Quest {}: ", all_quests[a].quest_name);
                    if (quest->stage == QUEST_SUCCESS)
                        resp += "Completed\n";
                    else if (quest->stage == QUEST_FAILURE)
                        resp += "Failed\n";
                    else
                        resp += fmt::format("Stage {}\n", quest->stage);
                }
                quest = quest->next;
            }
        } else
            log("SYSERR: do_stat_character: k->quests non-null but no quests exist "
                "(all_quests == NULL)");
    }

    /* Report any events attached. */
    if (k->events) {
        e = k->events;
        resp += fmt::format("Events: {}", eventname(e));
        while (e->next) {
            e = e->next;
            resp += fmt::format(" {}", eventname(e));
        }
        resp += "\n";
    } else {
        resp += "No events.\n";
    }

    /* List cooldowns */
    for (i = found = 0; i < NUM_COOLDOWNS; ++i) {
        if (!GET_COOLDOWN(k, i))
            continue;
        if (!found)
            resp += "Cooldowns:\n";
        resp += fmt::format("{:<25}: {}/{} sec\n", cooldowns[i], GET_COOLDOWN(k, i) / PASSES_PER_SEC,
                            GET_COOLDOWN_MAX(k, i) / PASSES_PER_SEC);
        found = true;
    }

    /* Check mobiles for a script */
    if (IS_NPC(k)) {
        do_sstat_character(ch, buf, k);
        resp += buf;
    }

    /* List enemies the mob remembers */
    mprinted = 0;
    for (memory = MEMORY(k); memory; memory = memory->next) {
        for (m = character_list; m; m = m->next)
            if (GET_IDNUM(m) == memory->id) {
                if (!mprinted) {
                    resp += fmt::format("Remembered enemies: &2{}&0", GET_NAME(m));
                    mprinted = 1;
                } else {
                    resp += fmt::format(", &2{}&0", GET_NAME(m));
                }
                break;
            }
    }
    if (mprinted)
        resp += "\n";

    /* List permanent player titles */
    if (!IS_NPC(k) && GET_PERM_TITLES(k)) {
        resp += "Permanent Titles:\n";
        for (i = 0; GET_PERM_TITLES(k)[i]; ++i)
            resp += fmt::format("  P{}) {}\n", i + 1, GET_PERM_TITLES(k)[i]);
    }

    page_string(ch, resp.c_str());
}

ACMD(do_stat) {
    void do_stat_shop(CharData * ch, char *arg);
    CharData *victim = 0;
    ObjData *obj = 0;
    int tmp;

    half_chop(argument, buf1, buf2);

    if (subcmd == SCMD_RSTAT || is_abbrev(buf1, "room")) {
        if (subcmd == SCMD_RSTAT && *buf1)
            tmp = isdigit(*buf1) ? real_room(atoi(buf1)) : NOWHERE;
        else if (subcmd != SCMD_RSTAT && *buf2)
            tmp = isdigit(*buf2) ? real_room(atoi(buf2)) : NOWHERE;
        else
            tmp = ch->in_room;
        if (tmp == NOWHERE)
            send_to_char("No such room.\n", ch);
        else
            do_stat_room(ch, tmp);
    } else if (subcmd == SCMD_SSTAT || is_abbrev(buf1, "shop")) {
        if (subcmd == SCMD_SSTAT)
            do_stat_shop(ch, buf1);
        else
            do_stat_shop(ch, buf2);
    } else if (!*buf1) {
        send_to_char("Stats on who or what?\n", ch);
    } else if (is_abbrev(buf1, "mob")) {
        if (!*buf2)
            send_to_char("Stats on which mobile?\n", ch);
        else {
            if ((victim = find_char_around_char(ch, find_vis_by_name(ch, buf2))))
                do_stat_character(ch, victim);
            else
                send_to_char("No such mobile around.\n", ch);
        }
    } else if (is_abbrev(buf1, "player")) {
        if (!*buf2) {
            send_to_char("Stats on which player?\n", ch);
        } else {
            if ((victim = find_char_around_char(ch, find_vis_plr_by_name(ch, buf2))))
                do_stat_character(ch, victim);
            else
                send_to_char("No such player around.\n", ch);
        }
    } else if (is_abbrev(buf1, "file")) {
        if (!*buf2) {
            send_to_char("Stats on which player?\n", ch);
        } else {
            CREATE(victim, CharData, 1);
            clear_char(victim);
            if (load_player(buf2, victim) > -1) {
                if (GET_LEVEL(victim) > GET_LEVEL(ch))
                    send_to_char("Sorry, you can't do that.\n", ch);
                else
                    do_stat_character(ch, victim);
                free_char(victim);
            } else {
                send_to_char("There is no such player.\n", ch);
                free(victim);
            }
        }
    } else if (is_abbrev(buf1, "object")) {
        if (!*buf2)
            send_to_char("Stats on which object?\n", ch);
        else {
            if ((obj = find_obj_around_char(ch, find_vis_by_name(ch, buf2))))
                do_stat_object(ch, obj);
            else
                send_to_char("No such object around.\n", ch);
        }
    } else {
        if ((obj = find_obj_in_eq(ch, nullptr, find_vis_by_name(ch, buf1))))
            do_stat_object(ch, obj);
        else if ((obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, buf1))))
            do_stat_object(ch, obj);
        else if ((victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf1))))
            do_stat_character(ch, victim);
        else if ((obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, buf1))))
            do_stat_object(ch, obj);
        else if ((victim = find_char_around_char(ch, find_vis_by_name(ch, buf1))))
            do_stat_character(ch, victim);
        else if ((obj = find_obj_in_world(find_vis_by_name(ch, buf1))))
            do_stat_object(ch, obj);
        else
            send_to_char("Nothing around by that name.\n", ch);
    }
}

ACMD(do_vstat) {
    CharData *mob;
    ObjData *obj;
    int number, r_num;

    two_arguments(argument, buf, buf2);

    if (subcmd == SCMD_VSTAT && (!*buf || !*buf2 || !isdigit(*buf2))) {
        send_to_char("Usage: vstat { obj | mob } <number>\n", ch);
    } else if (subcmd == SCMD_MSTAT && (!*buf || !isdigit(*buf))) {
        send_to_char("Usage: mstat <number>\n", ch);
    } else if (subcmd == SCMD_OSTAT && (!*buf || !isdigit(*buf))) {
        send_to_char("Usage: ostat <number>\n", ch);
    } else if ((number = (subcmd == SCMD_VSTAT ? atoi(buf2) : atoi(buf))) < 0) {
        send_to_char("A NEGATIVE number??\n", ch);
    } else if (subcmd == SCMD_MSTAT || (subcmd == SCMD_VSTAT && is_abbrev(buf, "mob"))) {
        if ((r_num = real_mobile(number)) < 0) {
            send_to_char("There is no monster with that number.\n", ch);
            return;
        }
        mob = read_mobile(r_num, REAL);
        char_to_room(mob, 0);
        do_stat_character(ch, mob);
        extract_char(mob);
    } else if (subcmd == SCMD_OSTAT || (subcmd == SCMD_VSTAT && is_abbrev(buf, "obj"))) {
        if ((r_num = real_object(number)) < 0) {
            send_to_char("There is no object with that number.\n", ch);
            return;
        }
        obj = read_object(r_num, REAL);
        do_stat_object(ch, obj);
        extract_obj(obj);
    } else
        send_to_char("That'll have to be either 'obj' or 'mob'.\n", ch);
}

ACMD(do_zstat) {
    int vnum, rnum;
    char str[MAX_INPUT_LENGTH];
    ZoneData *z;

    half_chop(argument, str, argument);

    if (!*str)
        vnum = IN_ZONE_VNUM(ch);
    else
        vnum = atoi(str);

    rnum = find_zone(vnum);
    if (rnum == -1) {
        send_to_char("There is no such zone.\n", ch);
        return;
    }

    z = &(zone_table[rnum]);
    sprintf(buf,
            "Zone &6%d&0: &2%s&0\n"
            "Vnum range     : &3%d-%d&0\n"
            "Reset mode     : &3%s&0\n"
            "Lifespan       : &3%d minutes&0\n"
            "Zone factor%%   : &3%d&0\n"
            "Hemisphere     : &3%s&0\n"
            "Climate        : &3%s&0\n"
            "Temperature    : &6%d degrees&0\n"
            "Precipitation  : &6%d&0\n"
            "Wind speed     : &6%s&0 (&6%d&0)\n"
            "Wind direction : &6%s&0\n",
            vnum, z->name, vnum * 100, z->top,
            z->reset_mode ? ((z->reset_mode == 1) ? "Reset when no players are in zone" : "Normal reset")
                          : "Never reset",
            z->lifespan, z->zone_factor,
            z->hemisphere >= 0 && z->hemisphere < NUM_HEMISPHERES ? hemispheres[z->hemisphere].name : "<INVALID>",
            z->climate >= 0 && z->climate < NUM_CLIMATES ? climates[z->climate].name : "<INVALID>", z->temperature,
            z->precipitation, z->wind_speed == WIND_NONE ? "none" : wind_speeds[z->wind_speed], z->wind_speed,
            z->wind_dir == NORTH   ? "North"
            : z->wind_dir == SOUTH ? "South"
            : z->wind_dir == EAST  ? "East"
            : z->wind_dir == WEST  ? "West"
                                   : "<INVALID>");
    send_to_char(buf, ch);
}

ACMD(do_players) {
    int i, count = 0, buflen = 0;
    const char *color;
    long bitfield = 0;

    any_one_arg(argument, arg);

    /* show usage */
    if (strlen(arg) != 1 || !isalpha(*arg)) {
        send_to_char("'players <letter>' shows all player names beginning with <letter>\n", ch);
        return;
    }

    *arg = tolower(*arg);

    /* Go through all the names in the pfile */
    for (i = 0; i <= top_of_p_table; i++) {
        /* Check if the first letter matches the argument */
        if (tolower(*player_table[i].name) == *arg) {
            ++count;

            if (buflen > sizeof(buf) - 100) {
                buflen += sprintf(buf + buflen, "\nToo many players!  Buffer full.");
                break;
            }

            bitfield = player_table[i].flags;

            if (IS_SET(bitfield, PINDEX_FROZEN))
                color = FCYN;
            else if (IS_SET(bitfield, PINDEX_NEWNAME))
                color = FRED;
            else if (IS_SET(bitfield, PINDEX_NAPPROVE))
                color = FYEL;
            else
                color = FGRN;
            buflen += sprintf(buf + buflen, "  %s&b%-15.20s(%3d) &0%s", color, player_table[i].name,
                              player_table[i].level, count % 3 ? "" : "\n");
        }
    }

    if (count == 0)
        strcpy(buf, "No players for that letter.\n");
    else if (count % 3)
        strcpy(buf + buflen, "\n");

    /* Show the name list */
    page_string(ch, buf);
}

ACMD(do_last) {
    CharData *victim;

    one_argument(argument, arg);
    if (!*arg) {
        send_to_char("For whom do you wish to search?\n", ch);
        return;
    }

    CREATE(victim, CharData, 1);
    clear_char(victim);

    if (load_player(arg, victim) < 0) {
        send_to_char("There is no such player.\n", ch);
        free(victim);
        return;
    }
    if (GET_LEVEL(victim) > GET_LEVEL(ch) && GET_LEVEL(ch) < LVL_HEAD_B) {
        send_to_char("You are not sufficiently godly for that!\n", ch);
        free_char(victim);
        return;
    }
    sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\n", GET_IDNUM(victim), GET_LEVEL(victim), CLASS_ABBR(victim),
            GET_NAME(victim), GET_HOST(victim), ctime(&victim->player.time.logon));
    send_to_char(buf, ch);

    free_char(victim);
}

/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

static std::string print_zone_to_buf(int zone) {
    return fmt::format("{:3} {:^30} Age: {:3}; Reset: {:3} ({}); ZF:{}: Top: {:5}\n", zone_table[zone].number,
                       zone_table[zone].name, zone_table[zone].age, zone_table[zone].lifespan,
                       zone_table[zone].reset_mode, zone_table[zone].zone_factor, zone_table[zone].top);
}

void do_show_sectors(CharData *ch, char *argument) {
    (void)argument;

    int i;
    const sectordef *s;

    pprintf(ch, "Sector type     Mv  Camp  Wet  Notes\n");
    pprintf(ch,
            "--------------  --  ----  ---  "
            "----------------------------------------------\n");
    for (i = 0; i < NUM_SECTORS; i++) {
        s = &sectors[i];
        pprintf(ch, " %s%-13s&0  %2d  %s  %s  %s\n", s->color, s->name, s->mv, s->campable ? "&2Camp&0" : "    ",
                s->wet ? "&6Wet&0" : "   ", s->notes);
    }
    start_paging(ch);
}

void do_show_compositions(CharData *ch, char *argument) {
    (void)argument;

    int i;

    sprintf(buf,
            "Idx  Composition  %sSlash&0  %sPierce&0  %sCrush&0  %sShock&0  "
            "%sFire&0  %sWater&0  %sCold&0  %sAcid&0  %sPoison&0\n",
            damtypes[DAM_SLASH].color, damtypes[DAM_PIERCE].color, damtypes[DAM_CRUSH].color, damtypes[DAM_SHOCK].color,
            damtypes[DAM_FIRE].color, damtypes[DAM_WATER].color, damtypes[DAM_COLD].color, damtypes[DAM_ACID].color,
            damtypes[DAM_POISON].color);
    send_to_char(buf, ch);
    send_to_char("---  -----------  -----  ------  -----  -----   ---  ----  -----  ----  ------\n", ch);
    for (i = 0; i < NUM_COMPOSITIONS; i++) {
        sprintf(buf,
                "%2d.  %s%-11s  % 5d  % 6d  % 5d  % 5d  % 4d  % 4d  % 5d  % 4d  "
                "% 6d&0\n",
                i, compositions[i].color, capitalize(compositions[i].name), compositions[i].sus_slash,
                compositions[i].sus_pierce, compositions[i].sus_crush, compositions[i].sus_shock,
                compositions[i].sus_fire, compositions[i].sus_water, compositions[i].sus_cold, compositions[i].sus_acid,
                compositions[i].sus_poison);
        send_to_char(buf, ch);
    }
}

void do_show_lifeforces(CharData *ch, char *argument) {
    (void)argument;

    int i;

    sprintf(buf, "Idx  Life force   %sHeal&0  %sDisc.&0  %sDispel&0  %sMental&0\n", damtypes[DAM_HEAL].color,
            damtypes[DAM_DISCORPORATE].color, damtypes[DAM_DISPEL].color, damtypes[DAM_MENTAL].color);
    send_to_char(buf, ch);
    send_to_char("---  -----------  ----  -----  ------  ------\n", ch);
    for (i = 0; i < NUM_LIFEFORCES; i++) {
        sprintf(buf, "%2d.  %s%-11s  % 4d  % 5d  % 6d  % 6d&0\n", i, lifeforces[i].color,
                capitalize(lifeforces[i].name), lifeforces[i].sus_heal, lifeforces[i].sus_discorporate,
                lifeforces[i].sus_dispel, lifeforces[i].sus_mental);
        send_to_char(buf, ch);
    }
}

void do_show_damtypes(CharData *ch, char *argument) {
    int i;

    send_to_char("Idx  Damage type      Verb 1st         Verb 2nd          Action \n", ch);
    send_to_char("---  -------------    -------------    --------------    ----------\n", ch);
    for (i = 0; i < NUM_DAMTYPES; i++) {
        sprintf(buf, "%2d.  %s%-15s  %-15s  %-15s   %-15s&0\n", i, damtypes[i].color, damtypes[i].name,
                damtypes[i].verb1st, damtypes[i].verb2nd, damtypes[i].action);
        send_to_char(buf, ch);
    }
}

void do_show_zones(CharData *ch, char *argument) {
    int zrnum;
    std::string zonebuf;

    any_one_arg(argument, arg);

    if (!strcasecmp(arg, ".")) {
        zonebuf += print_zone_to_buf(world[ch->in_room].zone);
        send_to_char(zonebuf.c_str(), ch);
    } else if (*arg && is_number(arg)) {
        if ((zrnum = find_zone(atoi(arg))) == NOWHERE)
            send_to_char("That is not a valid zone.\n", ch);
        else {
            zonebuf += print_zone_to_buf(zrnum);
            send_to_char(zonebuf.c_str(), ch);
        }
    } else {
        for (zrnum = 0; zrnum <= top_of_zone_table; ++zrnum)
            zonebuf += print_zone_to_buf(zrnum);
        page_string(ch, zonebuf.c_str());
    }
}

void do_show_player(CharData *ch, char *argument) {
    CharData *vict;

    one_argument(argument, arg);

    if (!*arg) {
        char_printf(ch, "A name would help.\n");
        return;
    }

    CREATE(vict, CharData, 1);
    clear_char(vict);

    if (load_player(arg, vict) < 0) {
        char_printf(ch, "There is no such player.\n");
        free(vict);
        return;
    }
    strftime(buf1, MAX_STRING_LENGTH, TIMEFMT_DATE, localtime(&(vict->player.time.birth)));
    strftime(buf2, MAX_STRING_LENGTH, TIMEFMT_DATE, localtime(&(vict->player.time.logon)));
    char_printf(ch, "Player: %-12s (%s) [%2d %s] (%s)\n", GET_NAME(vict), genders[(int)GET_SEX(vict)], GET_LEVEL(vict),
                CLASS_ABBR(vict), RACE_ABBR(vict));
    char_printf(ch,
                "Coins held:    [%7dp / %7dg / %7ds / %7dc]\n"
                "Coins banked:  [%7dp / %7dg / %7ds / %7dc]\n"
                "Exp: %-8ld   Align: %-5d\n"
                "Started: %s   Last: %s   Played: %3dh %2dm\n",
                GET_PLATINUM(vict), GET_GOLD(vict), GET_SILVER(vict), GET_COPPER(vict), GET_BANK_PLATINUM(vict),
                GET_BANK_GOLD(vict), GET_BANK_SILVER(vict), GET_BANK_COPPER(vict), GET_EXP(vict), GET_ALIGNMENT(vict),
                buf1, buf2, (int)(vict->player.time.played / 3600), (int)(vict->player.time.played / 60 % 60));
    char_printf(ch, "\n");
    send_save_description(vict, ch, false);
    free_char(vict);
}

void do_show_stats(CharData *ch, char *argument) {
    int mobiles = 0;
    int players = 0;
    int connected = 0;
    int objects = 0;
    CharData *vict;
    ObjData *obj;

    extern int buf_largecount;
    extern int buf_switches;
    extern int buf_overflows;

    for (vict = character_list; vict; vict = vict->next) {
        if (IS_NPC(vict))
            mobiles++;
        else if (CAN_SEE(ch, vict)) {
            players++;
            if (vict->desc)
                connected++;
        }
    }
    for (obj = object_list; obj; obj = obj->next)
        objects++;
    sprintf(buf,
            "Current stats:\n"
            "   %5d players in game       %5d connected\n"
            "   %5d registered\n"
            "   %5d mobiles               %5d prototypes\n"
            "   %5d objects               %5d prototypes\n"
            "   %5d rooms                 %5d zones\n"
            "   %5d large bufs\n"
            "   %5d buf switches          %5d overflows\n",
            players, connected, top_of_p_table + 1, mobiles, top_of_mobt + 1, objects, top_of_objt + 1,
            top_of_world + 1, top_of_zone_table + 1, buf_largecount, buf_switches, buf_overflows);
    send_to_char(buf, ch);
}

void do_show_errors(CharData *ch, char *argument) {
    (void)argument;

    int j, rn;

    strcpy(buf, "Errant Rooms\n------------\n");
    for (rn = 0; rn <= top_of_world; ++rn) {
        buf2[0] = '\0';
        for (j = 0; j < NUM_OF_DIRS; j++) {
            if (world[rn].exits[j]) {
                if (world[rn].exits[j]->to_room == 0)
                    snprintf(buf2, sizeof(buf2), "%s%s to void, ", buf2, dirs[j]);
                if (world[rn].exits[j]->to_room == NOWHERE && !world[rn].exits[j]->general_description)
                    snprintf(buf2, sizeof(buf2), "%s%s to NOWHERE, ", buf2, dirs[j]);
            }
        }
        if (buf2[0]) {
            buf2[strlen(buf2) - 2] = '\0'; /* cut off last comma */
            sprintf(buf, "%s [%5d] %-30s %s\n", buf, world[rn].vnum, world[rn].name, buf2);
        }
    }
    send_to_char(buf, ch);
}

void do_show_death(CharData *ch, char *argument) {
    (void)argument;
    std::string resp;
    int room, found;

    resp = "Death Traps\n-----------\n";
    for (room = 0, found = 0; room <= top_of_world; ++room)
        if (ROOM_FLAGGED(room, ROOM_DEATH))
            resp += fmt::format("{:2}: [{:5}] {}\n", ++found, world[room].vnum, world[room].name);
    send_to_char(resp.c_str(), ch);
}

void do_show_godrooms(CharData *ch, char *argument) {
    (void)argument;
    std::string resp;
    room_num room;
    int found;

    resp = "Godrooms\n--------------------------\n";
    for (room = 0, found = 0; room <= top_of_world; ++room)
        if (ROOM_FLAGGED(room, ROOM_GODROOM))
            resp += fmt::format("{:2}: [{:5}] {}\n", ++found, world[room].vnum, world[room].name);
    send_to_char(resp.c_str(), ch);
}

void do_show_houses(CharData *ch, char *argument) {
    (void)argument;
    extern void hcontrol_list_houses(CharData * ch);

    hcontrol_list_houses(ch);
}

void do_show_notes(CharData *ch, char *argument) {
    FILE *notes;
    std::string resp;

    any_one_arg(argument, arg);

    if (!*arg) {
        send_to_char("A name would help.\n", ch);
        return;
    }
    if (!get_pfilename(arg, buf2, NOTES_FILE)) {
        send_to_char("Couldn't find that player.\n", ch);
        return;
    }

    if (!(notes = fopen(buf2, "rt"))) {
        send_to_char("There are no notes for that file.\n", ch);
        return;
    }

    while (get_line(notes, buf2)) {
        resp += buf2;
        resp += "\n";
    }

    fclose(notes);
    page_string(ch, resp.c_str());
}

void do_show_classes(CharData *ch, char *argument) {
    (void)argument;
    std::string resp;
    int i, chars;
    const char *mem_mode;

    resp =
        "Class          Magic  Homeroom  Base Class  Max Subclass Level\n"
        "-------------  -----  --------  ----------  ------------------\n";
    for (i = 0; i < NUM_CLASSES; i++) {
        chars = count_color_chars(classes[i].displayname);
        if (classes[i].mem_mode == MEMORIZE)
            mem_mode = "mem";
        else if (classes[i].mem_mode == PRAY)
            mem_mode = "pray";
        else
            mem_mode = "none";
        if (classes[i].max_subclass_level > 0)
            sprintf(buf1, "%-2d", classes[i].max_subclass_level);
        else
            strcpy(buf1, "  ");
        resp += fmt::format(" {:>{}}  {:>5}  {:>8}  {:^10}  {}\n", 13 + chars, 13 + chars, classes[i].displayname,
                            mem_mode, classes[i].homeroom,
                            classes[i].subclass_of == CLASS_UNDEFINED ? "" : classes[classes[i].subclass_of].plainname,
                            buf1);
    }
    page_string(ch, resp.c_str());
}

void do_show_races(CharData *ch, char *argument) {
    int i, chars;
    std::string resp;

    skip_spaces(&argument);

    if (!*argument) {
        resp =
            "Race              Humanoid  Align    Size      HR/DR\n"
            "----------------  --------  -------  --------  ------\n";
        for (i = 0; i < NUM_RACES; i++) {
            chars = count_color_chars(races[i].fullname);
            resp += fmt::format(" {:>{}}   {:>8}  {:>5}    {:^8}  {}/{}\n", 15 + chars, 15 + chars, races[i].fullname,
                                YESNO(races[i].humanoid), races[i].def_align, sizes[races[i].def_size].name,
                                races[i].bonus_hitroll, races[i].bonus_damroll);
        }
    } else if ((i = parse_race(ch, ch, argument)) == RACE_UNDEFINED)
        return;
    else {
        RaceDef *race = &races[i];
        resp += fmt::format(
            "Race              : {}@0 (@g{}@0) @c{}@0\n"
            "Playable?         : @c{}@0\n"
            "Humanoid?         : @c{}@0\n"
            "Race Alignment    : @c{}@0\n"
            "Default Alignment : {}{}@0\n"
            "Default Size      : {}{}@0\n"
            "Default Lifeforce : {}{}@0\n"
            "Def. Composition  : {}{}@0\n"
            "Damroll/Hitroll   : @c{}@0/@c{}@0\n"
            "Weight            : male: @c{}@0-@c{}@0 female: @c{}@0-@c{}@0\n"
            "Height            : male: @c{}@0-@c{}@0 female: @c{}@0-@c{}@0\n",
            race->fullname, i, race->names, YESNO(race->playable), YESNO(race->humanoid),
            race_align_abbrevs[race->racealign], align_color(race->def_align), race->def_align,
            sizes[race->def_size].color, sizes[race->def_size].name, lifeforces[race->def_lifeforce].color,
            lifeforces[race->def_lifeforce].name, compositions[race->def_composition].color,
            compositions[race->def_composition].name, race->bonus_damroll, race->bonus_hitroll, race->mweight_lo,
            race->mweight_hi, race->fweight_lo, race->fweight_hi, race->mheight_lo, race->mheight_hi, race->fheight_lo,
            race->fheight_hi);

        sprintflag(buf2, race->effect_flags, NUM_EFF_FLAGS, effect_flags);
        resp += fmt::format(
            "Attribute Scales  : Str  Dex  Int  Wis  Con  Cha\n"
            "                  : @c{:3}% {:3}% {:3}% {:3}% {:3}% {:3}%@0\n"
            "Perm. Effects     : @y{}@0\n",
            race->attrib_scales[0], race->attrib_scales[1], race->attrib_scales[2], race->attrib_scales[3],
            race->attrib_scales[4], race->attrib_scales[5], buf2);

        resp += "Skills            : @c";
        for (i = 0; race->skills[i].skill; ++i)
            resp += fmt::format("{}{}", i == 0 ? "" : "@0, @c", skills[race->skills[i].skill].name);
        resp += i == 0 ? "None.@0\n" : "@0\n";

        resp += fmt::format(
            "Mob Factors\n"
            "  Experience      : @c{}%@0\n"
            "  Hitpoints       : @c{}%@0\n"
            "  Hitroll/Damroll : @c{}%@0\n"
            "  Damdice         : @c{}%@0\n"
            "  Copper          : @c{}%@0\n"
            "  Armor Class     : @c{}%@0\n",
            race->exp_factor, race->hit_factor, race->hd_factor, race->dice_factor, race->copper_factor,
            race->ac_factor);
    }
    page_string(ch, resp.c_str());
}

void do_show_exp(CharData *ch, char *argument) {
    int clazz, level;
    std::string resp;

    any_one_arg(argument, arg);

    if (!*arg) {
        send_to_char("Usage: show exp <class>\n", ch);
        return;
    }

    for (clazz = 0; clazz < NUM_CLASSES; ++clazz) {
        if (is_abbrev(arg, classes[clazz].name)) {
            resp =
                "   Level           Experience Needed           Total\n"
                " ---------         -----------------         ---------\n";
            for (level = 1; level < LVL_IMPL; ++level)
                resp += fmt::format(" {:3} - {:3}                 {:9}         {:9}\n", level, level + 1,
                                    exp_next_level(level, clazz) - exp_next_level(level - 1, clazz),
                                    exp_next_level(level, clazz));
            page_string(ch, resp.c_str());
            return;
        }
    }
    send_to_char("Invalid class.\n", ch);
}

void do_show_snoop(CharData *ch, char *argument) {
    (void)argument;
    int i = 0;
    DescriptorData *d;

    send_to_char(
        "People currently snooping:\n"
        "--------------------------\n",
        ch);
    for (d = descriptor_list; d; d = d->next) {
        if (d->snooping == nullptr || d->character == nullptr)
            continue;
        if (STATE(d) != CON_PLAYING || GET_LEVEL(ch) < GET_LEVEL(d->character))
            continue;
        if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
            continue;
        i++;
        sprintf(buf, "%-10s%s - snooped by %s%s.\n", GET_NAME(d->snooping->character), QNRM, GET_NAME(d->character),
                QNRM);
        send_to_char(buf, ch);
    }
    if (i == 0)
        send_to_char("No one is currently snooping.\n", ch);
}

void do_show_sizes(CharData *ch, char *argument) {
    (void)argument;
    show_sizes(ch);
}

/* David Endre 2/23/99 To view basic files online */
void do_show_file(CharData *ch, char *argument) {
    FILE *file;
    int cur_line = 0, num_lines = 0, req_lines = 0, i;
    std::string filebuf;
    const struct file_struct {
        const char *name;
        const char level;
        const char *path;
    } fields[] = {{"bug", LVL_GOD, "../lib/misc/bugs"},      {"typo", LVL_GOD, "../lib/misc/typos"},
                  {"ideas", LVL_GOD, "../lib/misc/ideas"},   {"xnames", LVL_GOD, "../lib/misc/xnames"},
                  {"levels", LVL_GOD, "../log/levels"},      {"rip", LVL_GOD, "../log/rip"},
                  {"players", LVL_GOD, "../log/newplayers"}, {"rentgone", LVL_GOD, "../log/rentgone"},
                  {"godcmds", LVL_GOD, "../log/godcmds"},    {"syslog", LVL_GOD, "../syslog"},
                  {"crash", LVL_GOD, "../syslog.CRASH"},     {nullptr, 0, "\n"}};

    argument = any_one_arg(argument, arg);

    if (!*arg) {
        strcpy(buf, "Usage: show file <name> <num lines>\n\nFile options:\n");
        for (i = 0; fields[i].level; ++i)
            if (fields[i].level <= GET_LEVEL(ch))
                sprintf(buf, "%s%-15s%s\n", buf, fields[i].name, fields[i].path);
        send_to_char(buf, ch);
        return;
    }

    for (i = 0; fields[i].name; ++i)
        if (!strncasecmp(arg, fields[i].name, strlen(arg)))
            break;

    if (!fields[i].name) {
        send_to_char("That is not a valid option!\n", ch);
        return;
    }
    if (GET_LEVEL(ch) < fields[i].level) {
        send_to_char("You are not godly enough to view that file!\n", ch);
        return;
    }

    if (!*argument)
        req_lines = 15; /* default is the last 15 lines */
    else
        req_lines = atoi(argument);

    /* open the requested file */
    if (!(file = fopen(fields[i].path, "r"))) {
        sprintf(buf, "SYSERR: Error opening file %s using 'file' command.", fields[i].path);
        mudlog(buf, BRF, LVL_IMPL, true);
        return;
    }

    /* count lines in requested file */
    get_line(file, buf);
    while (!feof(file)) {
        num_lines++;
        get_line(file, buf);
    }
    fclose(file);

    /* Limit # of lines printed to # requested or # of lines in file or
       80 lines */
    if (req_lines > num_lines)
        req_lines = num_lines;
    if (req_lines > 500)
        req_lines = 500;

    /* close and re-open */
    if (!(file = fopen(fields[i].path, "r"))) {
        sprintf(buf2, "SYSERR: Error opening file %s using 'file' command.", fields[i].path);
        mudlog(buf2, BRF, LVL_IMPL, true);
        return;
    }

    /* And print the requested lines */
    get_line(file, buf);
    for (cur_line = 0; !feof(file); get_line(file, buf)) {
        ++cur_line;
        if (cur_line > num_lines - req_lines)
            filebuf += fmt::format("{}\n", buf);
    }

    fclose(file);

    page_string(ch, filebuf.c_str());
}

bool will_npcs_cast(int spell) {
    // const SpellPair mob_sorcerer_buffs[];
    // const int mob_sorcerer_offensives[];
    // const int mob_sorcerer_area_spells[];
    // const SpellPair mob_cleric_buffs[];
    // const SpellPair mob_cleric_hindrances[];
    // const int mob_cleric_offensives[];
    // const int mob_cleric_area_spells[];
    // const int mob_cleric_heals[];
    // const SpellPair mob_bard_buffs[];
    // const SpellPair mob_bard_hindrances[];
    // const int mob_bard_offensives[];
    // const int mob_bard_area_spells[];
    // const int mob_bard_heals[];

    const int *npc_spell_lists[] = {mob_sorcerer_offensives, mob_sorcerer_area_spells, mob_cleric_offensives,
                                    mob_cleric_area_spells,  mob_cleric_heals,         mob_bard_offensives,
                                    mob_bard_area_spells,    mob_bard_heals,           nullptr};
    const SpellPair *npc_pair_lists[] = {mob_sorcerer_buffs, mob_cleric_buffs,    mob_cleric_hindrances,
                                         mob_bard_buffs,     mob_bard_hindrances, nullptr};
    int i, j;
    const int *list;
    const SpellPair *pair;

    for (i = 0; npc_spell_lists[i]; ++i)
        for (list = npc_spell_lists[i], j = 0; list[j]; ++j)
            if (list[j] == spell)
                return true;

    for (i = 0; npc_pair_lists[i]; ++i)
        for (pair = npc_pair_lists[i], j = 0; pair[j].spell; ++j)
            if (pair[j].spell == spell || pair[j].remover == spell)
                return true;

    return false;
}

static SkillDef *skill;
static int skill_class_comparator(int a, int b) {
    if (skill->min_level[a] > skill->min_level[b])
        return 1;
    return (skill->min_level[a] < skill->min_level[b] ? -1 : 0);
}

void do_show_spell(CharData *ch, int spellnum) {
    int i;
    SkillDef *spell;
    bool anytargets = false, anyroutines = false, anyassignments = false;

    if (!IS_SPELL(spellnum)) {
        send_to_char("There is no such spell.\n", ch);
        return;
    }

    spell = &skills[spellnum];

    /* Number and name */
    strcpy(buf2, spell->name);
    sprintf(buf, "&2Spell #%d&0, &5&b%s&0\n", spellnum, CAP(buf2));
    send_to_char(buf, ch);

    /* Stance */
    char_printf(ch, "Min pos     : %s\n", position_types[spell->minpos]);
    char_printf(ch, "Fighting?   : %s\n", spell->fighting_ok ? "&1Yes&0" : "No");

    /* Targets */
    sprintf(buf, "Targets     :");
    for (i = 0; i < NUM_TAR_FLAGS; i++)
        if (spell->targets & (1 << i)) {
            strcat(buf, " ");
            strcat(buf, targets[i]);
            anytargets = true;
        }
    if (!anytargets)
        strcat(buf, " -none-");
    strcat(buf, "\n");
    send_to_char(buf, ch);

    /* Violent */
    /* Damage type */
    if (spell->violent) {
        send_to_char("Violent     : &1&bYes&0\n", ch);
        if (VALID_DAMTYPE(spell->damage_type))
            sprintf(buf, "Damtype     : %s%s&0\n", damtypes[spell->damage_type].color,
                    damtypes[spell->damage_type].name);
        else
            sprintf(buf, "Damtype     : &1&bINVALID (%d)&0\n", spell->damage_type);
        send_to_char(buf, ch);
    } else {
        send_to_char("Violent     : &6No&0\n", ch);
        send_to_char("Damtype     : -na-\n", ch);
    }

    /* Sphere */
    if (IS_SPHERE_SKILL(spell->sphere)) {
        sprintf(buf, "Sphere      : %s\n", skills[spell->sphere].name);
        send_to_char(buf, ch);
    } else {
        sprintf(buf, "Sphere      : &1&bINVALID (%d)&0\n", spell->sphere);
        send_to_char(buf, ch);
    }

    /* Routines */
    sprintf(buf, "Routines    :");
    for (i = 0; i < NUM_ROUTINE_TYPES; i++)
        if (spell->routines & (1 << i)) {
            strcat(buf, " ");
            strcat(buf, routines[i]);
            anyroutines = true;
        }
    if (!anyroutines)
        strcat(buf, " -none-");
    strcat(buf, "\n");
    send_to_char(buf, ch);

    /* Mana */
    sprintf(buf, "Mana        : max %d  min %d  chg %d\n", spell->mana_max, spell->mana_min, spell->mana_change);
    send_to_char(buf, ch);

    /* Mem time */
    sprintf(buf, "Mem time    : %d\n", spell->mem_time);
    send_to_char(buf, ch);

    /* Cast time */
    sprintf(buf, "Cast time   : %d\n", spell->cast_time);
    send_to_char(buf, ch);

    /* Pages */
    sprintf(buf, "Pages       : &3%d&0\n", spell->pages);
    send_to_char(buf, ch);

    /* Quest */
    sprintf(buf, "Quest       : %s\n", spell->quest ? "&2&bYes&0" : "&4&bNo&0");
    send_to_char(buf, ch);

    /* Wearoff message */
    if (spell->wearoff && *(spell->wearoff)) {
        sprintf(buf, "Wearoff     : &6%s&0\n", spell->wearoff);
        send_to_char(buf, ch);
    } else
        send_to_char("Wearoff     : -none-\n", ch);

    /* Assignments */
    for (i = 0; i < NUM_CLASSES; i++)
        if (spell->min_level[i] > 0 && spell->min_level[i] < LVL_IMMORT) {
            sprintf(buf, "%s%-*s  circle %d\n", anyassignments ? "              " : "Assignments : ",
                    13 + count_color_chars(classes[i].displayname), classes[i].displayname,
                    level_to_circle(spell->min_level[i]));
            anyassignments = true;
            send_to_char(buf, ch);
        }
    if (!anyassignments)
        send_to_char("Assignments : -none-\n", ch);
}

void do_show_skill(CharData *ch, char *argument) {
    std::string resp;
    int skill_num, type, i, j;
    int skill_classes[NUM_CLASSES];

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char("Usage: show skill <skill name>\n", ch);
        return;
    }

    if ((skill_num = find_talent_num(argument, 0)) < 0) {
        send_to_char("Unrecognized skill name.\n", ch);
        return;
    }

    skill = &skills[skill_num];

    type = talent_type(skill_num);
    if (type == SPELL) {
        do_show_spell(ch, skill_num);
        return;
    }

    sprintbit(skill->targets, targets, buf2);

    resp += fmt::format(
        "Skill             : @y{}@0 (@g{}@0)\n"
        "Type              : @c{}@0\n"
        "Target Flags      : @c{}@0\n",
        skill->name, skill_num, talent_types[type], buf2);

    if (type == SKILL)
        resp += fmt::format("Humanoid only?    : @c{}@0\n", YESNO(skill->humanoid));
    else {
        sprintbit(skill->routines, routines, buf1);
        if (VALID_DAMTYPE(skill->damage_type))
            sprintf(buf2, "%s%s", damtypes[skill->damage_type].color, damtypes[skill->damage_type].name);
        else
            sprintf(buf2, "@RINVALID (%d)", skill->damage_type);
        resp += fmt::format(
            "Minimum position  : @c{}@0\n"
            "When fighting?    : @c{}@0\n"
            "Aggressive?       : @c{}@0\n"
            "Routines          : @c{}@0\n"
            "Damage Type       : {}@0\n"
            "Quest only?       : @c{}@0\n"
            "Wear-off Message  : {}@0\n",
            position_types[skill->minpos], YESNO(skill->fighting_ok), YESNO(skill->violent), buf1, buf2,
            YESNO(skill->quest), skill->wearoff ? skill->wearoff : "@cNone.");
    }

    if (type == SPELL) {
        if (IS_SPHERE_SKILL(skill->sphere))
            resp += fmt::format("Sphere            : @c{}@0\n", skills[skill->sphere].name);
        else
            resp += fmt::format("Sphere            : @RINVALID {}@0\n", skill->sphere);
        resp += fmt::format(
            "Mana              : @cmin {}, max {}, chg {}&0\n"
            "Mem Time          : @c{}@0\n"
            "Cast Time         : @c{}@0\n"
            "Pages             : @c{}@0\n",
            skill->mana_min, skill->mana_max, skill->mana_change, skill->mem_time, skill->cast_time, skill->pages);
    }

    sprintf(buf2, "(%s %d is lowest)", type == SPELL ? "circle" : "level",
            type == SPELL ? level_to_circle(skill->lowest_level) : skill->lowest_level);
    resp += fmt::format("Assignments       : @c{}@0\n", skill->lowest_level < LVL_IMMORT ? buf2 : "None.");

    if (skill->lowest_level < LVL_IMMORT) {
        for (i = 0; i < NUM_CLASSES; ++i)
            skill_classes[i] = i;

        sort(insertsort, skill_classes, NUM_CLASSES, skill_class_comparator);

        for (i = 0; skill->min_level[skill_classes[i]] < LVL_IMMORT && i < NUM_CLASSES; ++i) {
            j = skill_classes[i];
            resp += fmt::format("   {:>{}}: {}{}\n", classes[j].displayname,
                                15 + count_color_chars(classes[j].displayname), type == SPELL ? "circle " : "level ",
                                type == SPELL ? level_to_circle(skill->min_level[j]) : skill->min_level[j]);
        }
    }

    send_to_char(resp.c_str(), ch);
}

void do_show_date_names(CharData *ch, char *argument) {
    int i;

    pprintf(ch, "Day names:\n");
    for (i = 0; i < DAYS_PER_WEEK; ++i)
        pprintf(ch, "  %d - %s\n", i + 1, weekdays[i]);
    pprintf(ch, "\nMonth names:\n");
    for (i = 0; i < MONTHS_PER_YEAR; ++i)
        pprintf(ch, "  %d - %s\n", i + 1, month_name[i]);

    start_paging(ch);
}

void do_show_liquids(CharData *ch, char *argument) {
    int i;

    pprintf(ch,
            "&uLiquid           &0  &uColor             &0  &u+Drunk&0  "
            "&u+Full&0  &u-Thirst&0\n");

    for (i = 0; i < NUM_LIQ_TYPES; ++i)
        pprintf(ch, "%-17.17s  %-18.18s  %6d  %5d  %7d\n", liquid_types[i].name, liquid_types[i].color_desc,
                liquid_types[i].condition_effects[DRUNK], liquid_types[i].condition_effects[FULL],
                liquid_types[i].condition_effects[THIRST]);

    start_paging(ch);
}

ACMD(do_show) {
    std::string resp;
    int i;

    extern void show_shops(CharData * ch, char *argument);

    const struct show_struct {
        const char *name;
        char level;
        void (*command)(CharData *ch, char *argument);
    } fields[] = {/* Keep this list alphabetized */
                  {"compositions", LVL_IMMORT, do_show_compositions},
                  {"classes", LVL_IMMORT, do_show_classes},
                  {"command", LVL_GOD, do_show_command},
                  {"corpses", LVL_IMMORT, show_corpses},
                  {"damtypes", LVL_IMMORT, do_show_damtypes},
                  {"datenames", LVL_IMMORT, do_show_date_names},
                  {"death", LVL_GOD, do_show_death},
                  {"errors", LVL_GRGOD, do_show_errors},
                  {"exp", LVL_IMMORT, do_show_exp},
                  {"file", LVL_GRGOD, do_show_file},
                  {"godrooms", LVL_GOD, do_show_godrooms},
                  {"groups", LVL_GOD, do_show_command_groups},
                  {"houses", LVL_GOD, do_show_houses},
                  {"lifeforces", LVL_IMMORT, do_show_lifeforces},
                  {"liquids", LVL_IMMORT, do_show_liquids},
                  {"notes", LVL_IMMORT, do_show_notes},
                  {"player", LVL_GOD, do_show_player},
                  {"races", LVL_IMMORT, do_show_races},
                  {"rent", LVL_GOD, show_rent},
                  {"sectors", LVL_IMMORT, do_show_sectors},
                  {"shops", LVL_GOD, show_shops},
                  {"sizes", LVL_IMMORT, do_show_sizes},
                  {"skill", LVL_IMMORT, do_show_skill},
                  {"snoop", LVL_GOD, do_show_snoop},
                  {"spell", LVL_IMMORT, do_show_skill},
                  {"stats", LVL_IMMORT, do_show_stats},
                  {"zones", LVL_IMMORT, do_show_zones},
                  {nullptr, 0, nullptr}};

    skip_spaces(&argument);

    if (!*argument) {
        resp = "Show options:\n";
        for (i = 0; fields[i].level; ++i)
            if (fields[i].level <= GET_LEVEL(ch))
                resp += fmt::format("{:>15}{}", fields[i].name, (i + 1) % 5 ? "" : "\n");
        if (i % 5)
            resp += "\n";
        send_to_char(resp.c_str(), ch);
        return;
    }

    argument = any_one_arg(argument, arg);

    for (i = 0; fields[i].name; ++i)
        if (fields[i].level <= GET_LEVEL(ch))
            if (!strncasecmp(arg, fields[i].name, strlen(arg)))
                break;

    if (GET_LEVEL(ch) < fields[i].level) {
        send_to_char("You are not godly enough for that!\n", ch);
        return;
    }

    if (!fields[i].name) {
        send_to_char("That's not something you can show.\n", ch);
        return;
    }

    if (!fields[i].command) {
        send_to_char("Error retrieving information.\n", ch);
        return;
    }

    (fields[i].command)(ch, argument);
}

void reboot_info(CharData *ch) {
    int h, m, s;

    extern int num_hotboots;
    extern time_t *boot_time;

    h = (reboot_pulse - global_pulse) / (3600 * PASSES_PER_SEC);
    m = ((reboot_pulse - global_pulse) % (3600 * PASSES_PER_SEC)) / (60 * PASSES_PER_SEC);
    s = ((reboot_pulse - global_pulse) % (60 * PASSES_PER_SEC)) / PASSES_PER_SEC;
    if (reboot_auto)
        char_printf(ch, "Reboot in %02d:%02d:%02d.\n", h, m, s);
    else
        char_printf(ch, "Automatic rebooting is &1off&0; would reboot in %02d:%02d:%02d.\n", h, m, s);

    if (num_hotboots > 0) {
        char_printf(ch, "%d hotboot%s since last shutdown.  Hotboot history:\n", num_hotboots,
                    num_hotboots == 1 ? "" : "s");
        for (s = 0; s < num_hotboots; ++s) {
            strcpy(buf, ctime(&boot_time[s + 1]));
            buf[strlen(buf) - 1] = '\0';
            char_printf(ch, "  %s\n", buf);
        }
    }
}

ACMD(do_world) {
    extern ACMD(do_date);

    do_date(ch, nullptr, 0, SCMD_DATE);

    std::string git_hash = "";
    if (environment != ENV_PROD || GET_LEVEL(ch) >= LVL_GOD) {
        git_hash = get_git_hash();
        if (!git_hash.empty())
            git_hash = fmt::format(" Git Hash: {}", git_hash);
    }
    std::string bd{get_build_date()};
    char_printf(ch, "Build: %d  Compiled: %s %s\n", get_build_number(), bd.c_str(), git_hash.c_str());
    do_date(ch, nullptr, 0, SCMD_UPTIME);
    char_printf(ch, "There are %5d rooms in %3d zones online.\n", top_of_world + 1, top_of_zone_table + 1);

    if (GET_LEVEL(ch) >= LVL_REBOOT_VIEW)
        reboot_info(ch);
}

bool get_infodump_filename(const char *name, char *filename) {
    if (!name || !*name)
        return false;
    sprintf(filename, "%s/%s.txt", INFODUMP_PREFIX, name);
    return true;
}

void infodump_spellassign(CharData *ch, char *argument) {
    char fname[MAX_INPUT_LENGTH];
    FILE *fl;
    int class_num, skill;
    bool startedclass;

    if (!get_infodump_filename("spellassign", fname)) {
        send_to_char("ERROR: Could not get the output filename.\n", ch);
        return;
    }

    if (!(fl = fopen(fname, "w"))) {
        char_printf(ch, "ERROR: Could not write file %s.\n", fname);
        return;
    }

    /* WRITING SPELL CLASS ASSIGNMENTS */

    /* We're only writing as much info as we currently know we need.
     *
     * With any luck, we can add more info on to each line if we decide there's
     * a need for it, and not break any pre-existing scripts that may be used on
     * this dump file. The trick is not to modify any existing fields!
     *
     * ONLY add more to the end.
     *
     * Current fields:  SPELLNAME,CIRCLE,ISQUEST?
     */
    for (class_num = 0; class_num < NUM_CLASSES; class_num++) {
        startedclass = false;
        for (skill = 1; skill <= MAX_SPELLS; skill++) {
            if (skills[skill].min_level[class_num] < LVL_IMMORT) {
                if (!startedclass) {
                    fprintf(fl, "beginclass %s\n", classes[class_num].name);
                    startedclass = true;
                }
                fprintf(fl, "%s,%d,%s\n", skills[skill].name, level_to_circle(skills[skill].min_level[class_num]),
                        skills[skill].quest ? "true" : "false");
            }
        }
        if (startedclass)
            fprintf(fl, "endclass\n\n");
    }
    fclose(fl);
    sprintf(buf, "Dumped spell assignments to %s\n", fname);
    send_to_char(buf, ch);
}

/* The purpose of info dumps is to output game data in a format that will
 * be easily parsable by other programs. */

ACMD(do_infodump) {
    std::string resp;
    int i;
    const struct infodump_struct {
        const char *name;
        void (*command)(CharData *ch, char *argument);
    } fields[] = {/* Keep this list alphabetized */
                  {"spellassign", infodump_spellassign},
                  {nullptr, nullptr}};
    skip_spaces(&argument);

    if (!*argument) {
        resp = "Infodump options:\n";
        for (i = 0; fields[i].name; ++i)
            resp += fmt::format("{:>15}{}", fields[i].name, (i + 1) % 5 ? "" : "\n");
        if (i % 5)
            resp += "\n";
        send_to_char(resp.c_str(), ch);
        return;
    }

    argument = any_one_arg(argument, arg);

    for (i = 0; fields[i].name; ++i)
        if (!strncasecmp(arg, fields[i].name, strlen(arg)))
            break;

    if (!fields[i].name) {
        send_to_char("That's not something you can infodump.\n", ch);
        return;
    }

    if (!fields[i].command) {
        send_to_char("Error identifying what you wanted to dump.\n", ch);
        return;
    }

    (fields[i].command)(ch, argument);
}
