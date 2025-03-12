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
#include "bitflags.hpp"
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
#include "logging.hpp"
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

#define ZOCMD zone_table[zrnum].cmd[subcmd]

std::string list_zone_commands_room(CharData *ch, room_num rvnum) {
    std::string door_state;

    int zrnum = find_real_zone_by_room(rvnum);
    int rrnum = real_room(rvnum);
    int cmd_room = NOWHERE;
    int subcmd = 0, count = 0;

    if (zrnum == NOWHERE || rrnum == NOWHERE) {
        return "No zone information available.\n";
    }

    get_char_cols(ch);

    std::string output;
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
                output += fmt::format("{}Load {} [{}], Max : {}\n", ZOCMD.if_flag ? " then " : "",
                                      mob_proto[ZOCMD.arg1].player.short_descr, cyn, mob_index[ZOCMD.arg1].vnum, yel,
                                      ZOCMD.arg2);
                break;
            case 'G':
                output += fmt::format("{}Give it {} [{}{}{}], Max : {}\n", ZOCMD.if_flag ? " then " : "",
                                      obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel,
                                      ZOCMD.arg2);
                break;
            case 'O':
                output += fmt::format("{}Load {} [{}], Max : {}\n", ZOCMD.if_flag ? " then " : "",
                                      obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel,
                                      ZOCMD.arg2);
                break;
            case 'E':
                output += fmt::format("{}Equip with {} [{}], {}, Max : {}\n", ZOCMD.if_flag ? " then " : "",
                                      obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel,
                                      equipment_types[ZOCMD.arg3], ZOCMD.arg2);
                break;
            case 'P':
                output += fmt::format("{}Put {} [{}] in {} [{}], Max : {}\n", ZOCMD.if_flag ? " then " : "",
                                      obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel,
                                      obj_proto[ZOCMD.arg3].short_description, cyn, obj_index[ZOCMD.arg3].vnum, yel,
                                      ZOCMD.arg2);
                break;
            case 'R':
                output += fmt::format("{}Remove {} [{}] from room.\n", ZOCMD.if_flag ? " then " : "",
                                      obj_proto[ZOCMD.arg2].short_description, cyn, obj_index[ZOCMD.arg2].vnum, yel);
                break;
            case 'D':
                switch (ZOCMD.arg3) {
                case 0:
                    door_state = "open";
                    break;
                case 1:
                    door_state = "closed";
                    break;
                case 2:
                    door_state = "locked";
                    break;
                case 3:
                    door_state = "hidden";
                    break;
                case 4:
                    door_state = "hidden/closed/locked";
                    break;
                default:
                    door_state = "hidden/closed";
                    break;
                }
                output +=
                    fmt::format("{}Set door {} as {}.\n", ZOCMD.if_flag ? " then " : "", dirs[ZOCMD.arg2], door_state);
                break;
            default:
                output += "<Unknown Command>\n";
                break;
            }

            output += fmt::format("{}{:3d} - {}\n", nrm, count, ZOCMD.command);
        }
        subcmd++;
    }

    if (!count) {
        output += "  No commands for this room.\n";
    }

    return output;
}

std::string stat_extra_descs(ExtraDescriptionData *ed, CharData *ch, bool showtext) {
    ExtraDescriptionData *desc;
    int count;

    if (!ed) {
        return "No extra descs.\n";
    }

    std::string output = "";
    if (showtext) {
        for (desc = ed; desc; desc = desc->next)
            output += fmt::format("&4*&0 {}{}{}\n{}\n", CLR(ch, FCYN), desc->keyword, CLR(ch, ANRM), desc->description);
    } else {
        count = 0;
        for (desc = ed; desc; desc = desc->next)
            count++;
        output += fmt::format("Extra desc{} ({}) :{}\n", ed->next ? "s" : "", count, CLR(ch, FCYN));
        for (desc = ed; desc; desc = desc->next) {
            if (desc != ed)
                output += ", ";
            output += desc->keyword;
        }
        output += CLR(ch, ANRM);
        output += "\n";
    }
    return output;
}

void do_stat_room(CharData *ch, int rrnum) {
    RoomData *rm = &world[rrnum];
    int found = 0;
    ObjData *j = nullptr;
    CharData *k = nullptr;
    RoomEffectNode *reff;
    const sectordef *sector;
    std::string resp;

    if (!ch->desc)
        return;

    sector = &sectors[CH_SECT(ch)];

    resp += fmt::format("Room name: {}{}{}\n", CLR(ch, FCYN), rm->name, CLR(ch, ANRM));

    resp += fmt::format("Zone: [{}], VNum: [{}{}{}], RNum: [{}], Sector: {}\n", rm->zone, CLR(ch, FGRN), rm->vnum,
                        CLR(ch, ANRM), rrnum, fmt::format("{}{}&0 ({} mv)", sector->color, sector->name, sector->mv));

    resp += fmt::format("SpecProc: {}, Flags: {}\n", (rm->func == nullptr) ? "None" : "Exists",
                        sprintflag(rm->room_flags, NUM_ROOM_FLAGS, room_bits));

    resp += fmt::format("Room effects: {}\n", sprintflag(rm->room_effects, NUM_ROOM_EFF_FLAGS, room_effects));

    resp += fmt::format("Ambient Light : {}\n", rm->light);

    resp += fmt::format("Description:\n{}\n", rm->description.empty() ? " None." : rm->description);
    resp += stat_extra_descs(rm->ex_description, ch, false);

    std::string chars_present = fmt::format("Chars present:{}", CLR(ch, FYEL));
    for (found = 0, k = rm->people; k; k = k->next_in_room) {
        if (!CAN_SEE(ch, k))
            continue;

        chars_present += fmt::format("{} {}({})", found++ ? "," : "", GET_NAME(k),
                                     (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
        if (chars_present.length() >= 62) {
            resp += fmt::format("{}{}\n", chars_present, k->next_in_room ? "," : "");
            chars_present.clear();
            found = 0;
        }
    }
    if (!chars_present.empty())
        resp += fmt::format("{}\n", chars_present);
    resp += CLR(ch, ANRM);

    if (rm->contents) {
        std::string contents = fmt::format("Contents:{}", CLR(ch, FGRN));
        for (found = 0, j = rm->contents; j; j = j->next_content) {
            if (!CAN_SEE_OBJ(ch, j))
                continue;
            contents += fmt::format("{} {}", found++ ? "," : "", j->short_description);
            if (contents.length() >= 62) {
                resp += fmt::format("{}{}\n", contents, j->next_content ? "," : "");
                contents.clear();
                found = 0;
            }
        }

        if (!contents.empty())
            resp += fmt::format("{}\n", contents);
        resp += CLR(ch, ANRM);
    }

    for (int i = 0; i < NUM_OF_DIRS; i++) {
        if (rm->exits[i]) {
            std::string to_room =
                (rm->exits[i]->to_room == NOWHERE)
                    ? fmt::format(" {}NONE{}", CLR(ch, FCYN), CLR(ch, ANRM))
                    : fmt::format("{}{}{}", CLR(ch, FCYN), world[rm->exits[i]->to_room].vnum, CLR(ch, ANRM));
            std::string exit_info = sprintbit(rm->exits[i]->exit_info, exit_bits);
            resp += fmt::format("Exit {}{:5}{}:  To: [{}], Key: [{:5}], Keywrd: {}, Type: {}\n", CLR(ch, FCYN), dirs[i],
                                CLR(ch, ANRM), to_room, rm->exits[i]->key,
                                rm->exits[i]->keyword.empty() ? "None" : rm->exits[i]->keyword, exit_info);
            if (!rm->exits[i]->general_description.empty())
                resp += fmt::format("Extra Desc: {}\n", rm->exits[i]->general_description);
        }
    }

    /* Mention spells/effects */
    for (reff = room_effect_list; reff; reff = reff->next) {
        if (reff->room == rrnum) {
            std::string effect = sprinttype(reff->effect, room_effects);
            resp += fmt::format("SPL: ({:3}) &6{:21}&0, sets {}\n", reff->timer, skills[reff->spell].name, effect);
        }
    }

    resp += list_zone_commands_room(ch, rm->vnum);

    /* check the room for a script */
    resp += do_sstat_room(ch, rm);

    page_string(ch, resp);
}

// TODO: Update to return a string
std::string stat_spellbook(ObjData *obj) {
    int spage = 0, fpage = 0;
    SpellBookList *entry;

    /* If we can't get a list of the spells in the book, we need to say *exactly* why. */
    if (!obj->spell_book) {
        return "&1&b*&0 Its spell list is NULL.\n";
    } else if (!obj->spell_book->spell) {
        return "&1&b*&0 The first spell in its spell list is 0.\n";
    }

    std::string output{"&7&b---Spellbook Contents---&0\n"};
    for (entry = obj->spell_book; entry; entry = entry->next) {
        spage = fpage + 1;
        fpage = fpage + entry->length;
        output += fmt::format("&6{:3d}-{:3d})&0 &6&b{}&0\n", spage, fpage, skills[entry->spell].name);
    }

    if ((GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES) - fpage) > 1) {
        output += fmt::format("&6{:3d}-{:3d})&0 &4&b--Blank--&0\n", fpage + 1, GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES));
    } else if ((GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES) - fpage) == 1) {
        output += fmt::format("    &6{:3d})&0 &4&b--Blank--&0\n", GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES));
    }
    return output;
}

void do_stat_object(CharData *ch, ObjData *j) {
    std::string resp;
    int i, vnum, found;
    ObjData *j2;
    Event *e;

    if (!ch->desc)
        return;

    vnum = GET_OBJ_VNUM(j);
    resp += fmt::format("Name: '{}{}{}', Aliases: {}, Level: {}\n", CLR(ch, FYEL),
                        j->short_description.empty() ? "<None>" : j->short_description, CLR(ch, ANRM), j->name,
                        GET_OBJ_LEVEL(j));

    std::string spec_proc;
    if (GET_OBJ_RNUM(j) >= 0)
        spec_proc = obj_index[GET_OBJ_RNUM(j)].func ? "Exists" : "None";
    else
        spec_proc = "None";
    resp += fmt::format("VNum: [{}{:5d}{}], RNum: [{:5d}], Type: {}, SpecProc: {}\n", CLR(ch, FGRN), vnum,
                        CLR(ch, ANRM), GET_OBJ_RNUM(j), OBJ_TYPE_NAME(j), spec_proc);

    resp += fmt::format("L-Des: {}\n", j->description.empty() ? "None" : j->description);

    if (!j->action_description.empty())
        resp += fmt::format("Action desc:\n{}{}{}\n", CLR(ch, FYEL), j->action_description, CLR(ch, ANRM));

    resp += fmt::format("Can be worn on: {}{}{}\n", CLR(ch, FCYN), sprintbit(j->obj_flags.wear_flags, wear_bits),
                        CLR(ch, ANRM));

    resp += fmt::format("Extra flags   : {}{}{}\n", CLR(ch, FGRN),
                        sprintflag(GET_OBJ_FLAGS(j), NUM_ITEM_FLAGS, extra_bits), CLR(ch, ANRM));

    resp += fmt::format("Spell Effects : {}{}{}\n", CLR(ch, FYEL),
                        sprintflag(GET_OBJ_EFF_FLAGS(j), NUM_EFF_FLAGS, effect_flags), CLR(ch, ANRM));

    resp +=
        fmt::format("Weight: {:.2f}, Effective Weight: {:.2f}, Value: {}, Timer: {}, Decomp time: {}, Hiddenness: {}\n",
                    GET_OBJ_WEIGHT(j), GET_OBJ_EFFECTIVE_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_TIMER(j),
                    GET_OBJ_DECOMP(j), GET_OBJ_HIDDENNESS(j));

    std::string room_str;
    if (j->in_room == NOWHERE)
        room_str = "Nowhere";
    else
        room_str = fmt::format("Room: {}", world[j->in_room].vnum);
    resp +=
        fmt::format("In room: {}, In object: {}, Carried by: {}, Worn by: {}\n", room_str,
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
    case ITEM_INSTRUMENT:
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

            resp += fmt::format(
                "Weight capacity: {}, Lock Type: {}, Key Num: {}, Weight Reduction: {}%, Corpse: {}\n",
                GET_OBJ_VAL(j, VAL_CONTAINER_CAPACITY), sprintbit(GET_OBJ_VAL(j, VAL_CONTAINER_BITS), container_bits),
                GET_OBJ_VAL(j, VAL_CONTAINER_KEY), GET_OBJ_VAL(j, VAL_CONTAINER_WEIGHT_REDUCTION), YESNO(IS_CORPSE(j)));
        } else {
            resp +=
                fmt::format("Weight capacity: {}, Id: {}, Corpse: {}, Player Corpse: {}, Raisable: {}\n",
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
            for (i = 0; i < GET_OBJ_VAL(j, VAL_PORTAL_ENTRY_MSG) && i < portal_entry_messages.size(); ++i)
                ;
            resp += fmt::format("Entry-Room message: {}", portal_entry_messages[i]);
        }
        if (GET_OBJ_VAL(j, VAL_PORTAL_CHAR_MSG) >= 0) {
            for (i = 0; i < GET_OBJ_VAL(j, VAL_PORTAL_CHAR_MSG) && i < portal_character_messages.size(); ++i)
                ;
            resp += fmt::format("To-Char message   : {}", portal_character_messages[i]);
        }
        if (GET_OBJ_VAL(j, VAL_PORTAL_EXIT_MSG) >= 0) {
            for (i = 0; i < GET_OBJ_VAL(j, VAL_PORTAL_EXIT_MSG) && i < portal_exit_messages.size(); ++i)
                ;
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
        auto buf1 = fmt::format("\nContents:%s", CLR(ch, FGRN));
        for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
            buf1 += fmt::format("{} {}", found++ ? "," : "", j2->short_description);
            if (buf1.length() >= 62) {
                resp += fmt::format("{}{}\n", buf1, j2->next_content ? "," : "");
                buf1.clear();
                found = 0;
            }
        }

        if (!buf1.empty())
            resp += fmt::format("{}\n", buf1);
        resp += CLR(ch, ANRM);
    }

    found = 0;
    resp += "Applies:";
    for (i = 0; i < MAX_OBJ_APPLIES; i++)
        if (j->applies[i].modifier) {
            resp +=
                fmt::format("{} {}", found++ ? "," : "", format_apply(j->applies[i].location, j->applies[i].modifier));
        }
    resp += fmt::format("{}\n", found ? "" : " None");

    if (GET_OBJ_TYPE(j) == ITEM_SPELLBOOK) {
        resp += stat_spellbook(j);
    }

    resp += stat_extra_descs(j->ex_description, ch, false);

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
    resp += do_sstat_object(ch, j);

    page_string(ch, resp);
}

ACMD(do_estat) {
    ObjData *obj;
    std::string_view otarg;
    int tmp, r_num;

    auto buf1 = argument.shift();
    auto buf2 = argument.get();

    if (!ch->desc)
        return;

    if (subcmd == SCMD_RESTAT || matches(buf1, "room")) {
        if (subcmd == SCMD_RESTAT && !buf1.empty())
            tmp = is_integer(buf1) ? real_room(svtoi(buf1)) : NOWHERE;
        else if (subcmd != SCMD_RESTAT && !buf2.empty())
            tmp = is_integer(buf2) ? real_room(svtoi(buf2)) : NOWHERE;
        else
            tmp = ch->in_room;
        if (tmp == NOWHERE)
            char_printf(ch, "No such room.\n");
        else {
            page_string(ch, stat_extra_descs(world[tmp].ex_description, ch, true));
        }
    } else {
        if (subcmd == SCMD_OESTAT)
            otarg = buf1;
        else if (matches(buf1, "obj"))
            otarg = buf2;
        else
            otarg = buf1;

        if (otarg.empty()) {
            char_printf(ch, "Usage: estat room [<vnum>]\n");
            char_printf(ch, "       estat obj <name>\n");
        } else if (is_integer(otarg)) {
            tmp = svtoi(otarg);
            if ((r_num = real_object(tmp)) < 0)
                char_printf(ch, "There is no object with that number.\n");
            else {
                page_string(ch, stat_extra_descs(obj_proto[r_num].ex_description, ch, true));
            }
        } else if ((obj = find_obj_around_char(ch, find_vis_by_name(ch, otarg)))) {
            page_string(ch, stat_extra_descs(obj->ex_description, ch, true));
        } else {
            char_printf(ch, "No such object around.\n");
        }
    }
}

void do_stat_character(CharData *ch, CharData *k) {
    int i, a;
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
        resp += fmt::format("L-Des: {}\n", (GET_LDESC(k).empty() ? "<None>" : GET_LDESC(k)));
        if (!k->player.description.empty()) {
            resp += fmt::format("Desc: {}\n", k->player.description);
        }
    } else
        resp += fmt::format("Title: {}\n", (k->player.title.empty() ? "<None>" : k->player.title));

    /* Various character stats */
    resp += fmt::format("Race: {}, Race Align: {}, ", RACE_PLAINNAME(k), RACE_ALIGN_ABBR(k));
    /*
      if (!IS_NPC(k))
        resp += fmt::format( "Deity: {}, ", GET_DIETY(ch) >= 0 ?
                Dieties[(int) GET_DIETY(ch)].diety_name : "None");
    */
    resp += fmt::format("Size: {}, Gender: {}\n", capitalize_first(SIZE_DESC(k)), sprinttype(GET_SEX(k), genders));
    resp +=
        fmt::format("Life force: {}{}&0, Composition: {}{}&0\n", LIFEFORCE_COLOR(k),
                    capitalize_first(LIFEFORCE_NAME(k)), COMPOSITION_COLOR(k), capitalize_first(COMPOSITION_NAME(k)));
    resp += fmt::format("Class: {}, Lev: [{}{:2d}{}], XP: [{}{:7}{}], Align: [{:4d}]\n", CLASS_FULL(k), CLR(ch, FYEL),
                        GET_LEVEL(k), CLR(ch, ANRM), CLR(ch, FYEL), GET_EXP(k), CLR(ch, ANRM), GET_ALIGNMENT(k));

    /* Player specific data. */
    if (!IS_NPC(k)) {
        auto birthday = fmt::format(TIMEFMT_DATE, fmt::localtime(k->player.time.birth));
        auto last_login = fmt::format(TIMEFMT_DATE, fmt::localtime(k->player.time.logon));

        resp += fmt::format(
            "Created: [{}], Last Logon: [{}], Played: [{}h {}m]\n"
            "Age: [{}], Homeroom: [{}]",
            birthday, last_login, k->player.time.played / 3600, ((k->player.time.played / 3600) % 60), age(k).year,
            GET_HOMEROOM(k));

        if (GET_CLAN(k)) {
            resp += fmt::format(", Clan: [{}], Rank: [{}]", GET_CLAN(k)->abbreviation, GET_CLAN_RANK(k));
        }

        /* Display OLC zones for immorts */
        if (GET_LEVEL(k) >= LVL_IMMORT) {
            OLCZoneList *zone;
            std::string zone_msg;
            for (zone = GET_OLC_ZONES(k); zone; zone = zone->next)
                zone_msg += ("{}{}", zone->zone, zone->next ? ", " : "");
            resp += fmt::format(", OLC Zones: [{}]", zone_msg.empty() ? "None" : zone_msg);
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

    resp += fmt::format("HP: [{}{}/{}{}]  HP Gain: [{}{}{}] HP Regen Bonus: [{}{}{}]\n", CLR(ch, FGRN), GET_HIT(k),
                        GET_MAX_HIT(k), CLR(ch, ANRM), CLR(ch, FGRN), hit_gain(k), CLR(ch, ANRM), CLR(ch, FGRN),
                        k->char_specials.hitgain, CLR(ch, ANRM));
    resp += fmt::format("MV: [{}{}/{}{}]  MV Gain: [{}{}{}]\n", CLR(ch, FGRN), GET_MOVE(k), GET_MAX_MOVE(k),
                        CLR(ch, ANRM), CLR(ch, FGRN), move_gain(k), CLR(ch, ANRM));
    resp += fmt::format("Focus: [{}{}{}]\n", CLR(ch, FGRN), GET_FOCUS(k), CLR(ch, ANRM));
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
    auto position = sprinttype(GET_POS(k), position_types);
    auto stance = sprinttype(GET_STANCE(k), stance_types);
    resp += fmt::format("Pos: {} ({})", position, stance);
    if (IS_NPC(k)) {
        position = sprinttype(k->mob_specials.default_pos, position_types);
        resp += fmt::format(", Default Pos: {}", position);
    }
    resp += fmt::format(", Fighting: {}", FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "<none>");
    if (k->forward)
        resp +=
            fmt::format(", {} into: {}", GET_LEVEL(k) > LVL_IMMORT ? "Switched" : "Shapechanged", GET_NAME(k->forward));
    resp += "\n";

    std::string buf2;
    if (!IS_NPC(k))
        buf2 = fmt::format("Idle: [{} tic{}]", k->char_specials.timer, k->char_specials.timer == 1 ? "" : "s");
    if (k->desc) {
        auto connected = sprinttype(k->desc->connected, connected_types);
        buf2 += fmt::format("{}Connected: {}", buf2.empty() ? "" : ", ", connected);
    }
    if (POSSESSED(k))
        buf2 += fmt::format("{}{} into by: {}", buf2.empty() ? "" : ", ",
                            GET_LEVEL(POSSESSOR(k)) > LVL_IMMORT ? "Switched" : "Shapechanged", GET_NAME(POSSESSOR(k)));
    if (!buf2.empty())
        resp += fmt::format("{}\n", buf2);

    if (IS_MOB(k)) {
        std::string attack_type;
        if (k->mob_specials.attack_type >= 0 && k->mob_specials.attack_type <= TYPE_STAB - TYPE_HIT)
            attack_type = attack_hit_text[k->mob_specials.attack_type].singular;
        else
            attack_type = "<&1INVALID&0>";
        resp += fmt::format("Mob Spec-Proc: {}, NPC Bare Hand Dam: {}d{}, Attack type: {}\n",
                            (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"), k->mob_specials.damnodice,
                            k->mob_specials.damsizedice, attack_type);
    }

    /* Character flags. */
    if (IS_NPC(k)) {
        auto mob_flags = sprintflag(MOB_FLAGS(k), NUM_MOB_FLAGS, action_bits);
        resp += fmt::format("NPC flags: {}{}{}\n", CLR(ch, FCYN), mob_flags, CLR(ch, ANRM));
    } else {
        auto plr_flags = sprintflag(PLR_FLAGS(k), NUM_PLR_FLAGS, player_bits);
        resp += fmt::format("PLR: {}{}{}\n", CLR(ch, FCYN), plr_flags, CLR(ch, ANRM));
        auto prf_flags = sprintflag(PRF_FLAGS(k), NUM_PRF_FLAGS, preference_bits);
        resp += fmt::format("PRF: {}{}{}\n", CLR(ch, FGRN), prf_flags, CLR(ch, ANRM));
        auto prv_flags = sprintflag(PRV_FLAGS(k), NUM_PRV_FLAGS, privilege_bits);
        resp += fmt::format("PRV: {}{}{}\n", CLR(ch, FGRN), prv_flags, CLR(ch, ANRM));
    }

    /* Weight and objects. */
    int total_weight = 0;
    int worn_count = 0;
    for (int i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(k, i)) {
            ++worn_count;
            total_weight += GET_OBJ_EFFECTIVE_WEIGHT(GET_EQ(k, i));
        }
    }
    resp += fmt::format(
        "Max Carry: {} ({} weight); "
        "Carried: {} ({:.2f} weight); "
        "Worn: {} ({} weight)\n",
        CAN_CARRY_N(k), CAN_CARRY_W(k), IS_CARRYING_N(k), IS_CARRYING_W(k), worn_count, total_weight);

    /* Conditions. */
    resp += fmt::format("Hunger: {}, Thirst: {}, Drunk: {}\n",
                        GET_COND(k, FULL) < 0 ? "Off" : std::to_string(GET_COND(k, FULL)),
                        GET_COND(k, THIRST) < 0 ? "Off" : std::to_string(GET_COND(k, THIRST)),
                        GET_COND(k, DRUNK) < 0 ? "Off" : std::to_string(GET_COND(k, DRUNK)));

    /* Group, follower, guard, etc. */
    std::string followers;
    for (auto fol = k->followers; fol; fol = fol->next) {
        if (!followers.empty())
            followers += ", ";
        followers += PERS(fol->follower, ch);
    }
    resp += fmt::format("Consented: {}, Master is: {}, Followers are: {}\n",
                        (CONSENT(ch) ? GET_NAME(CONSENT(ch)) : "<none>"), (k->master ? GET_NAME(k->master) : "<none>"),
                        followers.empty() ? "<none>" : followers);

    /* Group list */
    std::string groupees;
    for (auto g = k->groupees; g; g = g->next) {
        if (!groupees.empty())
            groupees += ", ";
        groupees += PERS(g->groupee, ch);
    }
    resp +=
        fmt::format("&0&2&bGroup Master&0 is: {}, &0&2&bgroupees are:&0 {}\n",
                    ((k->group_master) ? GET_NAME(k->group_master) : "<none>"), groupees.empty() ? "<none>" : groupees);

    if (k->guarding || k->guarded_by)
        resp += fmt::format("Guarding: {}, Guarded by: {}\n", k->guarding ? GET_NAME(k->guarding) : "<none>",
                            k->guarded_by ? GET_NAME(k->guarded_by) : "<none>");
    if (k->cornering || k->cornered_by)
        resp += fmt::format("Cornering: {}, Cornered by: {}\n", k->cornering ? GET_NAME(k->cornering) : "<none>",
                            k->cornered_by ? GET_NAME(k->cornered_by) : "<none>");

    /* Effect bitvectors */
    auto eff_flags = sprintflag(EFF_FLAGS(k), NUM_EFF_FLAGS, effect_flags);
    resp += fmt::format("EFF: {}{}{}\n", CLR(ch, FYEL), eff_flags, CLR(ch, ANRM));

    /* NPC spell circle status */
    if (IS_NPC(k) && MEM_MODE(k) != MEM_NONE) {
        resp += "Spell slots available: ";
        auto slots = get_spell_slots_available(k);
        for (auto &slot : slots) {
            std::string_view color;
            if (slot == 0)
                color = CLR(ch, FRED);
            else if (slot == 1)
                color = CLR(ch, FYEL);
            else
                color = CLR(ch, FGRN);
            resp += fmt::format("{}{}{} ", color, slot, CLR(ch, ANRM));
        }
        resp += "\n";
    }

    /* List spells the character is affected by. */
    for (auto eff = k->effects; eff; eff = eff->next) {
        if (eff->duration < 0)
            resp += fmt::format("SPL: (perma) {}{:<21}{} ", CLR(ch, FCYN), skills[eff->type].name, CLR(ch, ANRM));
        else
            resp += fmt::format("SPL: ({:3}hr) {}{:<21}{} ", eff->duration + 1, CLR(ch, FCYN), skills[eff->type].name,
                                CLR(ch, ANRM));
        if (eff->modifier)
            resp += fmt::format("{:+d} to {}", eff->modifier, apply_types[(int)eff->location]);
        if (HAS_FLAGS(eff->flags, NUM_EFF_FLAGS)) {
            auto eff_flags = sprintflag(eff->flags, NUM_EFF_FLAGS, effect_flags);
            resp += fmt::format("{}sets {}", eff->modifier ? ", " : "", eff_flags);
        }
        resp += "\n";
    }

    /* Run through the quests the player is on */
    if (k->quests) {
        if (all_quests) {
            for (auto quest = k->quests; quest; quest = quest->next) {
                if ((a = real_quest(quest->quest_id)) >= 0) {
                    resp += fmt::format("Quest {}: ", all_quests[a].quest_name);
                    if (quest->stage == QUEST_SUCCESS)
                        resp += "Completed\n";
                    else if (quest->stage == QUEST_FAILURE)
                        resp += "Failed\n";
                    else
                        resp += fmt::format("Stage {}\n", quest->stage);
                }
            }
        } else
            log("SYSERR: do_stat_character: k->quests non-null but no quests exist (all_quests == NULL)");
    }

    /* Report any events attached. */
    if (k->events) {
        auto e = k->events;
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
    bool found = false;
    for (int i = 0; i < NUM_COOLDOWNS; ++i) {
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
        resp += do_sstat_character(ch, k);
    }

    /* List enemies the mob remembers */
    bool mprinted = false;
    for (auto memory = MEMORY(k); memory; memory = memory->next) {
        for (auto m = character_list; m; m = m->next) {
            if (GET_IDNUM(m) == memory->id) {
                if (!mprinted) {
                    resp += fmt::format("Remembered enemies: &2{}&0", GET_NAME(m));
                    mprinted = true;
                } else {
                    resp += fmt::format(", &2{}&0", GET_NAME(m));
                }
                break;
            }
        }
    }
    if (mprinted)
        resp += "\n";

    /* List permanent player titles */
    if (!IS_NPC(k) && !GET_PERM_TITLES(k).empty()) {
        resp += "Permanent Titles:\n";
        for (auto title : GET_PERM_TITLES(k)) {
            resp += fmt::format("  {}\n", title);
        }
    }

    page_string(ch, resp);
}

ACMD(do_stat) {
    void do_stat_shop(CharData * ch, std::string_view arg);
    CharData *victim = 0;
    ObjData *obj = 0;
    int tmp;

    auto buf1 = argument.shift();
    auto buf2 = argument.get();

    if (subcmd == SCMD_RSTAT || matches_start(buf1, "room")) {
        if (subcmd == SCMD_RSTAT && !buf1.empty())
            tmp = is_integer(buf1) ? real_room(svtoi(buf1)) : NOWHERE;
        else if (subcmd != SCMD_RSTAT && !buf2.empty())
            tmp = is_integer(buf2) ? real_room(svtoi(buf2)) : NOWHERE;
        else
            tmp = ch->in_room;
        if (tmp == NOWHERE)
            char_printf(ch, "No such room.\n");
        else
            do_stat_room(ch, tmp);
    } else if (subcmd == SCMD_SSTAT || matches_start(buf1, "shop")) {
        if (subcmd == SCMD_SSTAT)
            do_stat_shop(ch, buf1);
        else
            do_stat_shop(ch, buf2);
    } else if (buf1.empty()) {
        char_printf(ch, "Stats on who or what?\n");
    } else if (matches_start(buf1, "mob")) {
        if (buf2.empty())
            char_printf(ch, "Stats on which mobile?\n");
        else {
            if ((victim = find_char_around_char(ch, find_vis_by_name(ch, buf2))))
                do_stat_character(ch, victim);
            else
                char_printf(ch, "No such mobile around.\n");
        }
    } else if (matches_start(buf1, "player")) {
        if (buf2.empty()) {
            char_printf(ch, "Stats on which player?\n");
        } else {
            if ((victim = find_char_around_char(ch, find_vis_plr_by_name(ch, buf2))))
                do_stat_character(ch, victim);
            else
                char_printf(ch, "No such player around.\n");
        }
    } else if (matches_start(buf1, "file")) {
        if (buf2.empty()) {
            char_printf(ch, "Stats on which player?\n");
        } else {
            CREATE(victim, CharData, 1);
            clear_char(victim);
            if (load_player(buf2, victim) > -1) {
                if (GET_LEVEL(victim) > GET_LEVEL(ch))
                    char_printf(ch, "Sorry, you can't do that.\n");
                else
                    do_stat_character(ch, victim);
                free_char(victim);
            } else {
                char_printf(ch, "There is no such player.\n");
                free(victim);
            }
        }
    } else if (matches_start(buf1, "object")) {
        if (buf2.empty())
            char_printf(ch, "Stats on which object?\n");
        else {
            if ((obj = find_obj_around_char(ch, find_vis_by_name(ch, buf2))))
                do_stat_object(ch, obj);
            else
                char_printf(ch, "No such object around.\n");
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
            char_printf(ch, "Nothing around by that name.\n");
    }
}

ACMD(do_olocate) {
    int number = 0;
    ObjData *obj;
    std::string response;

    auto buf = argument.shift();

    if (buf.empty()) {
        char_printf(ch, "Search for what?\n");
        return;
    }

    response = fmt::format("Objects with name '{}' in the world:\n", buf);
    for (obj = object_list; obj; obj = obj->next) {
        if (isname(buf, obj->name)) {
            number++;
            response += fmt::format("  {:3} [{:5}] {:25} ", number, GET_OBJ_VNUM(obj), obj->short_description);
            if (obj->carried_by)
                response += fmt::format("carried by {:10} at {} [{}]", GET_NAME(obj->carried_by),
                                        world[obj->carried_by->in_room].name, obj->carried_by->in_room);
            else if (obj->worn_by)
                response += fmt::format("worn by {:10} at {} [{}]", GET_NAME(obj->worn_by),
                                        world[obj->worn_by->in_room].name, obj->worn_by->in_room);
            else if (obj->in_room != NOWHERE)
                response += fmt::format("in room {:10} [{}]", world[obj->in_room].name, obj->in_room);
            else if (obj->in_obj)
                response += fmt::format("inside {:10} at {} [{}]", obj->in_obj->short_description,
                                        world[obj->in_obj->in_room].name, obj->in_obj->in_room);
            else
                response += fmt::format("in an unknown location");
            response += "\n";

            if (number >= 100) {
                response += "More than 100 matches found.  Listing stopped.\n";
                break;
            }
        }
    }
    char_printf(ch, response);
}

ACMD(do_vstat) {
    CharData *mob;
    ObjData *obj;
    int number, r_num;

    auto buf = argument.shift();
    auto buf2 = argument.get();

    if (subcmd == SCMD_VSTAT && (buf.empty() || buf2.empty() || !is_integer(buf2))) {
        char_printf(ch, "Usage: vstat { obj | mob } <number>\n");
    } else if (subcmd == SCMD_MSTAT && (buf.empty() || !is_integer(buf))) {
        char_printf(ch, "Usage: mstat <number>\n");
    } else if (subcmd == SCMD_OSTAT && (buf.empty() || !is_integer(buf))) {
        char_printf(ch, "Usage: ostat <number>\n");
    } else if ((number = (subcmd == SCMD_VSTAT ? svtoi(buf2) : svtoi(buf))) < 0) {
        char_printf(ch, "A NEGATIVE number??\n");
    } else if (subcmd == SCMD_MSTAT || (subcmd == SCMD_VSTAT && matches_start(buf, "mob"))) {
        if ((r_num = real_mobile(number)) < 0) {
            char_printf(ch, "There is no monster with that number.\n");
            return;
        }
        mob = read_mobile(r_num, REAL);
        char_to_room(mob, 0);
        do_stat_character(ch, mob);
        extract_char(mob);
    } else if (subcmd == SCMD_OSTAT || (subcmd == SCMD_VSTAT && matches_start(buf, "obj"))) {
        if ((r_num = real_object(number)) < 0) {
            char_printf(ch, "There is no object with that number.\n");
            return;
        }
        obj = read_object(r_num, REAL);
        do_stat_object(ch, obj);
        extract_obj(obj);
    } else
        char_printf(ch, "That'll have to be either 'obj' or 'mob'.\n");
}

ACMD(do_zstat) {
    int vnum, rnum;
    ZoneData *z;

    auto str = argument.shift();

    if (str.empty())
        vnum = IN_ZONE_VNUM(ch);
    else
        vnum = svtoi(str);

    rnum = find_zone(vnum);
    if (rnum == -1) {
        char_printf(ch, "There is no such zone.\n");
        return;
    }

    z = &(zone_table[rnum]);
    char_printf(ch,
                "Zone &6{:d}&0: &2{}&0\n"
                "Vnum range     : &3{:d}-{:d}&0\n"
                "Reset mode     : &3{}&0\n"
                "Lifespan       : &3{:d} minutes&0\n"
                "Zone factor%   : &3{:d}&0\n"
                "Hemisphere     : &3{}&0\n"
                "Climate        : &3{}&0\n"
                "Temperature    : &6{:d} degrees&0\n"
                "Precipitation  : &6{:d}&0\n"
                "Wind speed     : &6{}&0 (&6{:d}&0)\n"
                "Wind direction : &6{}&0\n"
                "Last reset     : &6{:d} minutes ago&0\n",
                vnum, z->name, vnum * 100, z->top,
                z->reset_mode ? ((z->reset_mode == 1) ? "Reset when no players are in zone" : "Normal reset")
                              : "Never reset",
                z->lifespan, z->zone_factor,
                z->hemisphere >= 0 && z->hemisphere < NUM_HEMISPHERES ? hemispheres[z->hemisphere].name : "<INVALID>",
                z->climate >= 0 && z->climate < NUM_CLIMATES ? climates[z->climate].name : "<INVALID>", z->temperature,
                z->precipitation, z->wind_speed == WIND_NONE ? "none" : wind_speeds[z->wind_speed], z->wind_speed,
                dirs[z->wind_dir], z->age);
}

ACMD(do_players) {
    int count = 0;
    std::string_view color;
    long bitfield = 0;

    auto arg = argument.shift();

    /* show usage */
    if (arg.length() != 1 || !isalpha(arg[0])) {
        char_printf(ch, "'players <letter>' shows all player names beginning with <letter>\n");
        return;
    }

    std::string output = fmt::format("Players starting with '{}':\n", arg);

    /* Go through all the names in the pfile */
    for (int i = 0; i <= top_of_p_table; i++) {
        /* Check if the first letter matches the argument */
        if (matches(player_table[i].name, arg)) {
            ++count;

            bitfield = player_table[i].flags;

            if (IS_SET(bitfield, PINDEX_FROZEN))
                color = FCYN;
            else if (IS_SET(bitfield, PINDEX_NEWNAME))
                color = FRED;
            else if (IS_SET(bitfield, PINDEX_NAPPROVE))
                color = FYEL;
            else
                color = FGRN;
            output += fmt::format("  {}&b{:15.20}({:3d}) &0{}", color, player_table[i].name, player_table[i].level,
                                  count % 3 ? "" : "\n");
        }
    }

    if (count == 0)
        output += "No players for that letter.\n";
    else if (count % 3)
        output += "\n";

    /* Show the name list */
    page_string(ch, output);
}

ACMD(do_last) {
    CharData *victim;

    auto arg = argument.shift();
    if (arg.empty()) {
        char_printf(ch, "For whom do you wish to search?\n");
        return;
    }

    CREATE(victim, CharData, 1);
    clear_char(victim);

    if (load_player(arg, victim) < 0) {
        char_printf(ch, "There is no such player.\n");
        free(victim);
        return;
    }
    if (GET_LEVEL(victim) > GET_LEVEL(ch) && GET_LEVEL(ch) < LVL_HEAD_B) {
        char_printf(ch, "You are not sufficiently godly for that!\n");
        free_char(victim);
        return;
    }
    char_printf(ch, "[{:5}] [{:2d} {}] {:<12} : {:<18} : {:<20}\n", GET_IDNUM(victim), GET_LEVEL(victim),
                CLASS_ABBR(victim), GET_NAME(victim), GET_HOST(victim), ctime(&victim->player.time.logon));

    free_char(victim);
}

/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

static std::string print_zone_to_buf(int zone) {
    return fmt::format("{:3} {:<40} Age: {:3}; Reset: {:3} ({}); ZF:{}: Top: {:5}\n", zone_table[zone].number,
                       zone_table[zone].name, zone_table[zone].age, zone_table[zone].lifespan,
                       zone_table[zone].reset_mode, zone_table[zone].zone_factor, zone_table[zone].top);
}

void do_show_sectors(CharData *ch, Arguments argument) {
    (void)argument;

    int i;
    const sectordef *s;

    paging_printf(ch, "Sector type     Mv  Camp  Wet  Notes\n");
    paging_printf(ch, "--------------  --  ----  ---  ----------------------------------------------\n");
    for (i = 0; i < NUM_SECTORS; i++) {
        s = &sectors[i];
        paging_printf(ch, " {}{:<13s}&0  {:2d}  {}  {}  {}\n", s->color, s->name, s->mv,
                      s->campable ? "&2Camp&0" : "    ", s->wet ? "&6Wet&0" : "   ", s->notes);
    }
    start_paging(ch);
}

void do_show_compositions(CharData *ch, Arguments argument) {
    (void)argument;

    int i;

    char_printf(ch,
                "Idx  Composition  {}Slash&0  {}Pierce&0  {}Crush&0  {}Shock&0  "
                "{}Fire&0  {}Water&0  {}Cold&0  {}Acid&0  {}Poison&0\n",
                damtypes[DAM_SLASH].color, damtypes[DAM_PIERCE].color, damtypes[DAM_CRUSH].color,
                damtypes[DAM_SHOCK].color, damtypes[DAM_FIRE].color, damtypes[DAM_WATER].color,
                damtypes[DAM_COLD].color, damtypes[DAM_ACID].color, damtypes[DAM_POISON].color);
    char_printf(ch, "---  -----------  -----  ------  -----  -----   ---  ----  -----  ----  ------\n");
    for (i = 0; i < NUM_COMPOSITIONS; i++) {
        char_printf(ch, "{:2d}.  {}{:<11}  {: 5d}  {: 6d}  {: 5d}  {: 5d}  {: 4d}  {: 4d}  {: 5d}  {: 4d}  {: 6d}&0\n",
                    i, compositions[i].color, capitalize_first(compositions[i].name), compositions[i].sus_slash,
                    compositions[i].sus_pierce, compositions[i].sus_crush, compositions[i].sus_shock,
                    compositions[i].sus_fire, compositions[i].sus_water, compositions[i].sus_cold,
                    compositions[i].sus_acid, compositions[i].sus_poison);
    }
}

void do_show_lifeforces(CharData *ch, Arguments argument) {
    (void)argument;

    char_printf(ch, "Idx  Life force   {}Heal&0  {}Disc.&0  {}Dispel&0  {}Mental&0\n", damtypes[DAM_HEAL].color,
                damtypes[DAM_DISCORPORATE].color, damtypes[DAM_DISPEL].color, damtypes[DAM_MENTAL].color);
    char_printf(ch, "---  -----------  ----  -----  ------  ------\n");
    for (int i = 0; i < NUM_LIFEFORCES; i++) {
        char_printf(ch, "{:2d}.  {}{:<11}  {: 4d}  {: 5d}  {: 6d}  {: 6d}&0\n", i, lifeforces[i].color,
                    capitalize_first(lifeforces[i].name), lifeforces[i].sus_heal, lifeforces[i].sus_discorporate,
                    lifeforces[i].sus_dispel, lifeforces[i].sus_mental);
    }
}

void do_show_damtypes(CharData *ch, Arguments argument) {
    int i;

    char_printf(ch, "Idx  Damage type      Verb 1st         Verb 2nd          Action \n");
    char_printf(ch, "---  -------------    -------------    --------------    ----------\n");
    for (i = 0; i < NUM_DAMTYPES; i++) {
        char_printf(ch, "{:2d}.  {}{:<15}  {:<15}  {:<15}   {:<15}&0\n", i, damtypes[i].color, damtypes[i].name,
                    damtypes[i].verb1st, damtypes[i].verb2nd, damtypes[i].action);
    }
}

void do_show_zones(CharData *ch, Arguments argument) {
    int zrnum;
    std::string zonebuf;

    auto arg = argument.shift();

    if (matches(arg, ".")) {
        zonebuf += print_zone_to_buf(world[ch->in_room].zone);
        char_printf(ch, zonebuf);
    } else if (!arg.empty() && is_integer(arg)) {
        if ((zrnum = find_zone(svtoi(arg))) == NOWHERE)
            char_printf(ch, "That is not a valid zone.\n");
        else {
            zonebuf += print_zone_to_buf(zrnum);
            char_printf(ch, zonebuf);
        }
    } else {
        for (zrnum = 0; zrnum <= top_of_zone_table; ++zrnum)
            zonebuf += print_zone_to_buf(zrnum);
        page_string(ch, zonebuf);
    }
}

void do_show_player(CharData *ch, Arguments argument) {
    CharData *vict;

    auto arg = argument.shift();

    if (arg.empty()) {
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
    auto birthday = fmt::format(TIMEFMT_DATE, fmt::localtime(vict->player.time.birth));
    auto last_login = fmt::format(TIMEFMT_DATE, fmt::localtime(vict->player.time.logon));
    char_printf(ch, "Player: {:<12s} ({}) [{:2d} {}] ({})\n", GET_NAME(vict), genders[(int)GET_SEX(vict)],
                GET_LEVEL(vict), CLASS_ABBR(vict), RACE_ABBR(vict));
    char_printf(ch,
                "Coins held:    [{:7d}p / {:7d}g / {:7d}s / {:7d}c]\n"
                "Coins banked:  [{:7d}p / {:7d}g / {:7d}s / {:7d}c]\n"
                "Exp: {:-8}   Align: {:-5d}\n"
                "Started: {}   Last: {}   Played: {:3d}h {:2d}m\n\n",
                GET_PLATINUM(vict), GET_GOLD(vict), GET_SILVER(vict), GET_COPPER(vict), GET_BANK_PLATINUM(vict),
                GET_BANK_GOLD(vict), GET_BANK_SILVER(vict), GET_BANK_COPPER(vict), GET_EXP(vict), GET_ALIGNMENT(vict),
                birthday, last_login, (int)(vict->player.time.played / 3600),
                (int)(vict->player.time.played / 60 % 60));
    send_save_description(vict, ch, false);
    free_char(vict);
}

void do_show_stats(CharData *ch, Arguments argument) {
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
    char_printf(ch,
                "Current stats:\n"
                "   {:5d} players in game       {:5d} connected\n"
                "   {:5d} registered\n"
                "   {:5d} mobiles               {:5d} prototypes\n"
                "   {:5d} objects               {:5d} prototypes\n"
                "   {:5d} rooms                 {:5d} zones\n"
                "   {:5d} large bufs\n"
                "   {:5d} buf switches          {:5d} overflows\n",
                players, connected, top_of_p_table + 1, mobiles, top_of_mobt + 1, objects, top_of_objt + 1,
                top_of_world + 1, top_of_zone_table + 1, buf_largecount, buf_switches, buf_overflows);
}

void do_show_errors(CharData *ch, Arguments argument) {
    std::string output = "Errant Rooms\n------------\n";
    for (int rn = 0; rn <= top_of_world; ++rn) {
        std::string errors = "";
        for (int j = 0; j < NUM_OF_DIRS; j++) {
            if (world[rn].exits[j]) {
                if (world[rn].exits[j]->to_room == 0)
                    errors += fmt::format("{} to void, ", dirs[j]);
                if (world[rn].exits[j]->to_room == NOWHERE && world[rn].exits[j]->general_description.empty())
                    errors += fmt::format("{} to NOWHERE, ", dirs[j]);
            }
        }
        if (errors.length() > 0) {
            errors = errors.substr(0, errors.length() - 2); /* cut off last comma */
            output += fmt::format("[{:5}] {:30} {}\n", world[rn].vnum, world[rn].name, errors);
        }
    }
    char_printf(ch, output);
}

void do_show_death(CharData *ch, Arguments argument) {
    (void)argument;
    std::string resp;
    int room, found;

    resp = "Death Traps\n-----------\n";
    for (room = 0, found = 0; room <= top_of_world; ++room)
        if (ROOM_FLAGGED(room, ROOM_DEATH))
            resp += fmt::format("{:2}: [{:5}] {}\n", ++found, world[room].vnum, world[room].name);
    char_printf(ch, resp);
}

void do_show_godrooms(CharData *ch, Arguments argument) {
    (void)argument;
    std::string resp;
    room_num room;
    int found;

    resp = "Godrooms\n--------------------------\n";
    for (room = 0, found = 0; room <= top_of_world; ++room)
        if (ROOM_FLAGGED(room, ROOM_GODROOM))
            resp += fmt::format("{:2}: [{:5}] {}\n", ++found, world[room].vnum, world[room].name);
    char_printf(ch, resp);
}

void do_show_houses(CharData *ch, Arguments argument) {
    (void)argument;
    extern void hcontrol_list_houses(CharData * ch);

    hcontrol_list_houses(ch);
}

void do_show_notes(CharData *ch, Arguments argument) {
    FILE *notes;
    std::string resp;

    auto arg = argument.shift();

    if (arg.empty()) {
        char_printf(ch, "A name would help.\n");
        return;
    }
    auto filename = get_pfilename(arg, NOTES_FILE);
    if (filename.empty()) {
        char_printf(ch, "Couldn't find that player.\n");
        return;
    }

    if (!(notes = fopen(filename.c_str(), "rt"))) {
        char_printf(ch, "There are no notes for that file.\n");
        return;
    }

    while (auto buf = get_line(notes)) {
        resp += *buf;
        resp += "\n";
    }

    fclose(notes);
    page_string(ch, resp);
}

void do_show_classes(CharData *ch, Arguments argument) {
    (void)argument;
    std::string resp;
    int i, chars;
    std::string_view mem_mode;

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
        std::string buf1;
        if (classes[i].max_subclass_level > 0)
            buf1 = fmt::format("{:-2d}", classes[i].max_subclass_level);
        else
            buf1 = "  ";
        resp += fmt::format(
            " {:>{}}  {:>5}  {:>8}  {:^10}  {}\n", classes[i].displayname, 12 + chars, mem_mode, classes[i].homeroom,
            classes[i].subclass_of == CLASS_UNDEFINED ? "" : classes[classes[i].subclass_of].plainname, buf1);
    }
    page_string(ch, resp);
}

void do_show_races(CharData *ch, Arguments argument) {
    int i, chars;
    std::string resp;

    if (argument.empty()) {
        resp =
            "Race                   Humanoid  Align    Size        HR/DR\n"
            "---------------------  --------  -------  ----------  ------\n";
        for (i = 0; i < NUM_RACES; i++) {
            chars = count_color_chars(races[i].fullname);
            resp += fmt::format(" {:<{}}   {:<8}  {:<5}    {:<10}  {}/{}\n", races[i].fullname, 20 + chars,
                                YESNO(races[i].humanoid), races[i].def_align, sizes[races[i].def_size].name,
                                races[i].bonus_hitroll, races[i].bonus_damroll);
        }
    } else if ((i = parse_race(ch, ch, argument.get())) == RACE_UNDEFINED)
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

        resp += fmt::format(
            "Attribute Scales  : Str  Dex  Int  Wis  Con  Cha\n"
            "                  : @c{:3}% {:3}% {:3}% {:3}% {:3}% {:3}%@0\n"
            "Perm. Effects     : @y{}@0\n",
            race->attrib_scales[0], race->attrib_scales[1], race->attrib_scales[2], race->attrib_scales[3],
            race->attrib_scales[4], race->attrib_scales[5],
            sprintflag(race->effect_flags, NUM_EFF_FLAGS, effect_flags));

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
    page_string(ch, resp);
}

void do_show_exp(CharData *ch, Arguments argument) {
    int clazz, level;
    std::string resp;

    auto arg = argument.shift();

    if (arg.empty()) {
        char_printf(ch, "Usage: show exp <class>\n");
        return;
    }

    for (clazz = 0; clazz < NUM_CLASSES; ++clazz) {
        if (matches_start(arg, classes[clazz].name)) {
            resp =
                "   Level           Experience Needed           Total\n"
                " ---------         -----------------         ---------\n";
            for (level = 1; level < LVL_IMPL; ++level)
                resp += fmt::format(" {:3} - {:3}                 {:9}         {:9}\n", level, level + 1,
                                    exp_next_level(level, clazz) - exp_next_level(level - 1, clazz),
                                    exp_next_level(level, clazz));
            page_string(ch, resp);
            return;
        }
    }
    char_printf(ch, "Invalid class.\n");
}

void do_show_snoop(CharData *ch, Arguments argument) {
    (void)argument;
    int i = 0;
    DescriptorData *d;

    char_printf(ch,
                "People currently snooping:\n"
                "--------------------------\n");
    for (d = descriptor_list; d; d = d->next) {
        if (d->snooping == nullptr || d->character == nullptr)
            continue;
        if (STATE(d) != CON_PLAYING || GET_LEVEL(ch) < GET_LEVEL(d->character))
            continue;
        if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
            continue;
        i++;
        char_printf(ch, "{:<10}{} - snooped by {}{}.\n", GET_NAME(d->snooping->character), QNRM, GET_NAME(d->character),
                    QNRM);
    }
    if (i == 0)
        char_printf(ch, "No one is currently snooping.\n");
}

void do_show_sizes(CharData *ch, Arguments argument) {
    (void)argument;
    show_sizes(ch);
}

/* David Endre 2/23/99 To view basic files online */
void do_show_file(CharData *ch, Arguments argument) {
    FILE *file;
    int i;
    int cur_line = 0, num_lines = 0, req_lines = 0;
    std::string filebuf;
    const struct file_struct {
        std::string_view name;
        const char level;
        std::string path;
    } fields[] = {{"bug", LVL_GOD, "../lib/misc/bugs"},      {"typo", LVL_GOD, "../lib/misc/typos"},
                  {"ideas", LVL_GOD, "../lib/misc/ideas"},   {"xnames", LVL_GOD, "../lib/misc/xnames"},
                  {"levels", LVL_GOD, "../log/levels"},      {"rip", LVL_GOD, "../log/rip"},
                  {"players", LVL_GOD, "../log/newplayers"}, {"rentgone", LVL_GOD, "../log/rentgone"},
                  {"godcmds", LVL_GOD, "../log/godcmds"},    {"syslog", LVL_GOD, "../syslog"},
                  {"crash", LVL_GOD, "../syslog.CRASH"},     {{}, 0, "\n"}};

    auto arg = argument.shift();

    if (arg.empty()) {
        char_printf(ch, "Usage: show file <name> <num lines>\n\nFile options:\n");
        for (i = 0; fields[i].level; ++i)
            if (fields[i].level <= GET_LEVEL(ch))
                char_printf(ch, "{:<15}{}\n", fields[i].name, fields[i].path);
        return;
    }

    for (int i = 0; !fields[i].name.empty(); ++i)
        if (arg == fields[i].name)
            break;

    if (fields[i].name.empty()) {
        char_printf(ch, "That is not a valid option!\n");
        return;
    }
    if (GET_LEVEL(ch) < fields[i].level) {
        char_printf(ch, "You are not godly enough to view that file!\n");
        return;
    }

    if (argument.empty())
        req_lines = 15; /* default is the last 15 lines */
    else
        req_lines = svtoi(arg);

    /* open the requested file */
    if (!(file = fopen(fields[i].path.data(), "r"))) {
        log(LogSeverity::Warn, LVL_IMPL, "SYSERR: Error opening file {} using 'file' command.", fields[i].path);
        return;
    }

    /* count lines in requested file */
    while (get_line(file)) {
        num_lines++;
    }
    fclose(file);

    /* Limit # of lines printed to # requested or # of lines in file or
       80 lines */
    if (req_lines > num_lines)
        req_lines = num_lines;
    if (req_lines > 500)
        req_lines = 500;

    /* close and re-open */
    if (!(file = fopen(fields[i].path.data(), "r"))) {
        log(LogSeverity::Warn, LVL_IMPL, "SYSERR: Error opening file {} using 'file' command.", fields[i].path);
        return;
    }

    /* And print the requested lines */
    std::optional<std::string> buf;
    for (cur_line = 0, buf = get_line(file); buf; buf = get_line(file)) {
        ++cur_line;
        if (cur_line > num_lines - req_lines)
            filebuf += fmt::format("{}\n", *buf);
    }

    fclose(file);

    page_string(ch, filebuf);
}

bool will_npcs_cast(int spell) {
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
        char_printf(ch, "There is no such spell.\n");
        return;
    }

    spell = &skills[spellnum];

    /* Number and name */
    char_printf(ch, "&2Spell #{:d}&0, &5&b{}&0\n", spellnum, capitalize_first(spell->name));

    /* Stance */
    char_printf(ch, "Min pos     : {}\n", position_types[spell->minpos]);
    char_printf(ch, "Fighting?   : {}\n", spell->fighting_ok ? "&1Yes&0" : "No");

    /* Targets */
    char_printf(ch, "Targets     :");
    for (i = 0; i < NUM_TAR_FLAGS; i++)
        if (spell->targets & (1 << i)) {
            char_printf(ch, " {}", targets[i]);
            anytargets = true;
        }
    if (!anytargets)
        char_printf(ch, " -none-");
    char_printf(ch, "\n");

    /* Violent */
    /* Damage type */
    if (spell->violent) {
        char_printf(ch, "Violent     : &1&bYes&0\n");
        if (VALID_DAMTYPE(spell->damage_type))
            char_printf(ch, "Damtype     : {}{}&0\n", damtypes[spell->damage_type].color,
                        damtypes[spell->damage_type].name);
        else
            char_printf(ch, "Damtype     : &1&bINVALID ({:d})&0\n", spell->damage_type);
    } else {
        char_printf(ch, "Violent     : &6No&0\n");
        char_printf(ch, "Damtype     : -na-\n");
    }

    /* Sphere */
    if (IS_SPHERE_SKILL(spell->sphere)) {
        char_printf(ch, "Sphere      : {}\n", skills[spell->sphere].name);
    } else {
        char_printf(ch, "Sphere      : &1&bINVALID ({:d})&0\n", spell->sphere);
    }

    /* Routines */
    char_printf(ch, "Routines    :");
    for (i = 0; i < NUM_ROUTINE_TYPES; i++)
        if (spell->routines & (1 << i)) {
            char_printf(ch, " {}", routines[i]);
            anyroutines = true;
        }
    if (!anyroutines)
        char_printf(ch, " -none-");
    char_printf(ch, "\n");

    char_printf(ch, "Mana        : max {:d}  min {:d}  chg {:d}\n", spell->mana_max, spell->mana_min,
                spell->mana_change);
    char_printf(ch, "Mem time    : {:d}\n", spell->addl_mem_time);
    char_printf(ch, "Cast time   : {:d}\n", spell->cast_time);
    char_printf(ch, "Pages       : &3{:d}&0\n", spell->pages);
    char_printf(ch, "Quest       : {}\n", spell->quest ? "&2&bYes&0" : "&4&bNo&0");

    /* Wearoff message */
    if (spell->wearoff.empty())
        char_printf(ch, "Wearoff     : -none-\n");
    else
        char_printf(ch, "Wearoff     : &6{}&0\n", spell->wearoff);

    /* Assignments */
    for (i = 0; i < NUM_CLASSES; i++)
        if (spell->min_level[i] > 0 && spell->min_level[i] < LVL_IMMORT) {
            char_printf(ch, "{}{:<{}}  circle {:d}\n",
                        anyassignments ? "              " : "Assignments : ", classes[i].displayname,
                        13 + count_color_chars(classes[i].displayname), level_to_circle(spell->min_level[i]));
            anyassignments = true;
        }
    if (!anyassignments)
        char_printf(ch, "Assignments : -none-\n");
}

void do_show_skill(CharData *ch, Arguments argument) {
    std::string resp;
    int skill_num, type, i, j;
    int skill_classes[NUM_CLASSES];

    if (argument.empty()) {
        char_printf(ch, "Usage: show skill <skill name>\n");
        return;
    }

    if ((skill_num = find_talent_num(argument.get(), 0)) < 0) {
        char_printf(ch, "Unrecognized skill name.\n");
        return;
    }

    skill = &skills[skill_num];

    type = talent_type(skill_num);
    if (type == SPELL) {
        do_show_spell(ch, skill_num);
        return;
    }

    resp += fmt::format(
        "Skill             : @y{}@0 (@g{}@0)\n"
        "Type              : @c{}@0\n"
        "Target Flags      : @c{}@0\n",
        skill->name, skill_num, talent_types[type], sprintbit(skill->targets, targets));

    if (type == SKILL)
        resp += fmt::format("Humanoid only?    : @c{}@0\n", YESNO(skill->humanoid));
    else {
        auto damage_type = [&]() -> std::string {
            if (VALID_DAMTYPE(skill->damage_type))
                return fmt::format("{}{}", damtypes[skill->damage_type].color, damtypes[skill->damage_type].name);
            return fmt::format("@RINVALID ({:d})", skill->damage_type);
        }();
        resp += fmt::format(
            "Minimum position  : @c{}@0\n"
            "When fighting?    : @c{}@0\n"
            "Aggressive?       : @c{}@0\n"
            "Routines          : @c{}@0\n"
            "Damage Type       : {}@0\n"
            "Quest only?       : @c{}@0\n"
            "Wear-off Message  : {}@0\n",
            position_types[skill->minpos], YESNO(skill->fighting_ok), YESNO(skill->violent),
            sprintbit(skill->routines, routines), damage_type, YESNO(skill->quest),
            !skill->wearoff.empty() ? skill->wearoff : "@cNone.");
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
            skill->mana_min, skill->mana_max, skill->mana_change, skill->addl_mem_time, skill->cast_time, skill->pages);
    }

    resp += fmt::format("Assignments       : ");
    if (skill->lowest_level < LVL_IMMORT)
        resp += fmt::format("@c({} {} is lowest)@0", type == SPELL ? "circle" : "level",
                            type == SPELL ? level_to_circle(skill->lowest_level) : skill->lowest_level);
    else
        resp += "@cNone@0";

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

    char_printf(ch, resp);
}

void do_show_date_names(CharData *ch, Arguments argument) {
    int i;

    paging_printf(ch, "Day names:\n");
    for (i = 0; i < DAYS_PER_WEEK; ++i)
        paging_printf(ch, "  {:d} - {}\n", i + 1, weekdays[i]);
    paging_printf(ch, "\nMonth names:\n");
    for (i = 0; i < MONTHS_PER_YEAR; ++i)
        paging_printf(ch, "  {:d} - {}\n", i + 1, month_name[i]);

    start_paging(ch);
}

void do_show_liquids(CharData *ch, Arguments argument) {
    int i;

    paging_printf(ch,
                  "&uLiquid           &0  &uColor             &0  &u+Drunk&0  "
                  "&u+Full&0  &u-Thirst&0\n");

    for (i = 0; i < NUM_LIQ_TYPES; ++i)
        paging_printf(ch, "{:<17s}  {:<18s}  {:6d}  {:5d}  {:7d}\n", liquid_types[i].name, liquid_types[i].color_desc,
                      liquid_types[i].condition_effects[DRUNK], liquid_types[i].condition_effects[FULL],
                      liquid_types[i].condition_effects[THIRST]);

    start_paging(ch);
}

ACMD(do_show) {
    std::string resp;
    int i;

    const struct show_struct {
        std::string_view name;
        char level;
        void (*command)(CharData *ch, Arguments argument);
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
                  {{}, 0, nullptr}};

    if (argument.empty()) {
        resp = "Show options:\n";
        for (i = 0; fields[i].level; ++i)
            if (fields[i].level <= GET_LEVEL(ch))
                resp += fmt::format("{:>15}{}", fields[i].name, (i + 1) % 5 ? "" : "\n");
        if (i % 5)
            resp += "\n";
        char_printf(ch, resp);
        return;
    }

    auto arg = argument.shift();

    for (i = 0; !fields[i].name.empty(); ++i)
        if (fields[i].level <= GET_LEVEL(ch))
            if (matches(arg, fields[i].name))
                break;

    if (GET_LEVEL(ch) < fields[i].level) {
        char_printf(ch, "You are not godly enough for that!\n");
        return;
    }

    if (fields[i].name.empty()) {
        char_printf(ch, "That's not something you can show.\n");
        return;
    }

    if (!fields[i].command) {
        char_printf(ch, "Error retrieving information.\n");
        return;
    }

    (fields[i].command)(ch, argument);
}

void reboot_info(CharData *ch) {
    int h, m, s;

    extern time_t *boot_time;

    h = (reboot_pulse - global_pulse) / (3600 * PASSES_PER_SEC);
    m = ((reboot_pulse - global_pulse) % (3600 * PASSES_PER_SEC)) / (60 * PASSES_PER_SEC);
    s = ((reboot_pulse - global_pulse) % (60 * PASSES_PER_SEC)) / PASSES_PER_SEC;
    if (reboot_auto)
        char_printf(ch, "Reboot in {:02d}:{:02d}:{:02d}.\n", h, m, s);
    else
        char_printf(ch, "Automatic rebooting is &1off&0; would reboot in {:02d}:{:02d}:{:02d}.\n", h, m, s);
}

ACMD(do_world) {
    extern ACMD(do_date);

    do_date(ch, {""}, 0, SCMD_DATE);

    std::string git_hash = "";
    if (environment != ENV_PROD || GET_LEVEL(ch) >= LVL_GOD) {
        git_hash = get_git_hash();
        if (!git_hash.empty())
            git_hash = fmt::format(" Git Hash: {}", git_hash);
    }
    std::string bd{get_build_date()};
    char_printf(ch, "Build: {:d}  Compiled: {} {}\n", get_build_number(), bd, git_hash);
    do_date(ch, {}, 0, SCMD_UPTIME);
    char_printf(ch, "There are {:5d} rooms in {:3d} zones online.\n", top_of_world + 1, top_of_zone_table + 1);

    if (GET_LEVEL(ch) >= LVL_REBOOT_VIEW)
        reboot_info(ch);
}

std::string get_infodump_filename(const std::string_view name) {
    if (name.empty())
        return {};
    return fmt::format("{}/{}.txt", INFODUMP_PREFIX, name);
}

void infodump_spellassign(CharData *ch, Arguments argument) {
    char fname[MAX_INPUT_LENGTH];
    FILE *fl;
    int class_num, skill;
    bool startedclass;

    if (!get_infodump_filename("spellassign", fname)) {
        char_printf(ch, "ERROR: Could not get the output filename.\n");
        return;
    }

    if (!(fl = fopen(fname, "w"))) {
        char_printf(ch, "ERROR: Could not write file {}.\n", fname);
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
    char_printf(ch, "Dumped spell assignments to {}\n", fname);
}

/* The purpose of info dumps is to output game data in a format that will
 * be easily parsable by other programs. */

ACMD(do_infodump) {
    std::string resp;
    int i;
    const struct infodump_struct {
        const std::string_view name;
        void (*command)(CharData *ch, Arguments argument);
    } fields[] = {/* Keep this list alphabetized */
                  {"spellassign", infodump_spellassign},
                  {nullptr, nullptr}};
    skip_spaces(argument);

    if (argument.empty()) {
        resp = "Infodump options:\n";
        for (i = 0; fields[i].name; ++i)
            resp += fmt::format("{:>15}{}", fields[i].name, (i + 1) % 5 ? "" : "\n");
        if (i % 5)
            resp += "\n";
        char_printf(ch, resp);
        return;
    }

    argument = auto arg = argument.shift();

    for (i = 0; fields[i].name; ++i)
        if (!strncasecmp(arg, fields[i].name, strlen(arg)))
            break;

    if (!fields[i].name) {
        char_printf(ch, "That's not something you can infodump.\n");
        return;
    }

    if (!fields[i].command) {
        char_printf(ch, "Error identifying what you wanted to dump.\n");
        return;
    }

    (fields[i].command)(ch, argument);
}
