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

#include "ai.h"
#include "casting.h"
#include "charsize.h"
#include "clan.h"
#include "class.h"
#include "comm.h"
#include "commands.h"
#include "composition.h"
#include "conf.h"
#include "constants.h"
#include "cooldowns.h"
#include "corpse_save.h"
#include "damage.h"
#include "db.h"
#include "dg_scripts.h"
#include "events.h"
#include "exits.h"
#include "fight.h"
#include "handler.h"
#include "house.h"
#include "interpreter.h"
#include "lifeforce.h"
#include "limits.h"
#include "modify.h"
#include "olc.h"
#include "pfiles.h"
#include "players.h"
#include "quest.h"
#include "races.h"
#include "screen.h"
#include "skills.h"
#include "spell_mem.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"
#include "vsearch.h"
#include "weather.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

void garble_text(char *string, int percent);
void dismount_char(struct char_data *ch);
void check_new_surroundings(struct char_data *ch, bool old_room_was_dark, bool tx_obvious);

extern int should_restrict;
extern int restrict_reason;
extern int max_group_difference;

extern int races_allowed;
extern int evil_races_allowed;
extern int pk_allowed;
extern int sleep_allowed;
extern int charm_allowed;
extern int summon_allowed;
extern int roomeffect_allowed;
extern int approve_names;
extern int napprove_pause;
extern int gossip_channel_active;
extern int nameserver_is_slow;
extern int level_gain;
extern int damage_amounts;
int Valid_Name(char *newname);
int reserved_word(char *argument);

/* Automatic rebooting */
extern int reboot_auto;
extern long global_pulse;
extern long reboot_pulse;
extern int reboot_warning;
extern int last_reboot_warning;
extern int reboot_warning_minutes;

/* extern functions */
void send_to_xnames(char *name);
int find_zone(int num);
void cure_laryngitis(struct char_data *ch);
void reboot_mud_prep();
void rebootwarning(int minutes);

/* Internal funct */
void do_wiztitle(char *outbuf, struct char_data *vict, char *argu);

#define ZOCMD zone_table[zrnum].cmd[subcmd]

void list_zone_commands_room(struct char_data *ch, char *buf, room_num rvnum) {
    int zrnum = find_real_zone_by_room(rvnum);
    int rrnum = real_room(rvnum), cmd_room = NOWHERE;
    int subcmd = 0, count = 0;

    if (zrnum == NOWHERE || rrnum == NOWHERE) {
        strcpy(buf, "No zone information available.\r\n");
        return;
    }

    get_char_cols(ch);

    sprintf(buf, "Zone commands in this room:%s\r\n", yel);
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
                sprintf(buf1, "%sLoad %s [%s%d%s], Max : %d\r\n", ZOCMD.if_flag ? " then " : "",
                        mob_proto[ZOCMD.arg1].player.short_descr, cyn, mob_index[ZOCMD.arg1].virtual, yel, ZOCMD.arg2);
                break;
            case 'G':
                sprintf(buf1, "%sGive it %s [%s%d%s], Max : %d\r\n", ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].virtual, yel, ZOCMD.arg2);
                break;
            case 'O':
                sprintf(buf1, "%sLoad %s [%s%d%s], Max : %d\r\n", ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].virtual, yel, ZOCMD.arg2);
                break;
            case 'E':
                sprintf(buf1, "%sEquip with %s [%s%d%s], %s, Max : %d\r\n", ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].virtual, yel,
                        equipment_types[ZOCMD.arg3], ZOCMD.arg2);
                break;
            case 'P':
                sprintf(buf1, "%sPut %s [%s%d%s] in %s [%s%d%s], Max : %d\r\n", ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].virtual, yel,
                        obj_proto[ZOCMD.arg3].short_description, cyn, obj_index[ZOCMD.arg3].virtual, yel, ZOCMD.arg2);
                break;
            case 'R':
                sprintf(buf1, "%sRemove %s [%s%d%s] from room.\r\n", ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg2].short_description, cyn, obj_index[ZOCMD.arg2].virtual, yel);
                break;
            case 'D':
                sprintf(buf1, "%sSet door %s as %s.\r\n", ZOCMD.if_flag ? " then " : "", dirs[ZOCMD.arg2],
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
                strcpy(buf1, "<Unknown Command>\r\n");
                break;
            }
            sprintf(buf, "%s%s%3d - %s%s", buf, nrm, count, yel, buf1);
        }
        subcmd++;
    }
    strcat(buf, nrm);
    if (!count)
        strcat(buf, "  None.\r\n");
}

void stat_extra_descs(struct extra_descr_data *ed, struct char_data *ch, char *buf, bool showtext) {
    struct extra_descr_data *desc;
    int count;

    if (!ed) {
        strcpy(buf, "No extra descs.\r\n");
        return;
    }

    if (showtext) {
        strcpy(buf, "");
        for (desc = ed; desc; desc = desc->next)
            sprintf(buf, "%s&4*&0 %s%s%s\r\n%s", buf, CLR(ch, FCYN), desc->keyword, CLR(ch, ANRM), desc->description);
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
        strcat(buf, "\r\n");
    }
}

void do_stat_room(struct char_data *ch, int rrnum) {
    struct room_data *rm = &world[rrnum];
    int i, found = 0;
    struct obj_data *j = 0;
    struct char_data *k = 0;
    struct room_effect_node *reff;
    const struct sectordef *sector;

    if (!ch->desc)
        return;

    sector = &sectors[CH_SECT(ch)];
    str_start(buf, sizeof(buf));

    str_catf(buf, "Room name: %s%s%s\r\n", CLR(ch, FCYN), rm->name, CLR(ch, ANRM));

    sprintf(buf2, "%s%s&0 (%d mv)", sector->color, sector->name, sector->mv);
    str_catf(buf, "Zone: [%3d], VNum: [%s%5d%s], RNum: [%5d], Sector: %s\r\n", rm->zone, CLR(ch, FGRN), rm->vnum,
             CLR(ch, ANRM), rrnum, buf2);

    sprintflag(buf2, rm->room_flags, NUM_ROOM_FLAGS, room_bits);
    str_catf(buf, "SpecProc: %s, Flags: %s\r\n", (rm->func == NULL) ? "None" : "Exists", buf2);

    sprintflag(buf2, rm->room_effects, NUM_ROOM_EFF_FLAGS, room_effects);
    str_catf(buf, "Room effects: %s\r\n", buf2);

    str_catf(buf, "Ambient Light : %d\r\n", rm->light);

    str_cat(buf, "Description:\r\n");
    str_cat(buf, rm->description ? rm->description : "  None.\r\n");

    stat_extra_descs(rm->ex_description, ch, str_end(buf), FALSE);

    sprintf(buf2, "Chars present:%s", CLR(ch, FYEL));
    for (found = 0, k = rm->people; k; k = k->next_in_room) {
        if (!CAN_SEE(ch, k))
            continue;
        sprintf(buf2, "%s%s %s(%s)", buf2, found++ ? "," : "", GET_NAME(k),
                (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
        if (strlen(buf2) >= 62) {
            str_catf(buf, "%s%s\r\n", buf2, k->next_in_room ? "," : "");
            *buf2 = found = 0;
        }
    }
    if (*buf2)
        str_catf(buf, "%s\r\n", buf2);
    str_cat(buf, CLR(ch, ANRM));

    if (rm->contents) {
        sprintf(buf2, "Contents:%s", CLR(ch, FGRN));
        for (found = 0, j = rm->contents; j; j = j->next_content) {
            if (!CAN_SEE_OBJ(ch, j))
                continue;
            sprintf(buf2, "%s%s %s", buf2, found++ ? "," : "", j->short_description);
            if (strlen(buf2) >= 62) {
                str_catf(buf, "%s%s\r\n", buf2, j->next_content ? "," : "");
                *buf2 = found = 0;
            }
        }

        if (*buf2)
            str_catf(buf, "%s\r\n", buf2);
        str_cat(buf, CLR(ch, ANRM));
    }

    for (i = 0; i < NUM_OF_DIRS; i++) {
        if (rm->exits[i]) {
            if (rm->exits[i]->to_room == NOWHERE)
                sprintf(buf1, " %sNONE%s", CLR(ch, FCYN), CLR(ch, ANRM));
            else
                sprintf(buf1, "%s%5d%s", CLR(ch, FCYN), world[rm->exits[i]->to_room].vnum, CLR(ch, ANRM));
            sprintbit(rm->exits[i]->exit_info, exit_bits, buf2);
            str_catf(buf, "Exit %s%-5s%s:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n", CLR(ch, FCYN), dirs[i],
                     CLR(ch, ANRM), buf1, rm->exits[i]->key, rm->exits[i]->keyword ? rm->exits[i]->keyword : "None",
                     buf2);
            if (rm->exits[i]->general_description)
                str_cat(buf, rm->exits[i]->general_description);
        }
    }

    /* Mention spells/effects */
    for (reff = room_effect_list; reff; reff = reff->next) {
        if (reff->room == rrnum) {
            sprinttype(reff->effect, room_effects, buf2);
            str_catf(buf, "SPL: (%3d) &6%-21s&0, sets %s\r\n", reff->timer, skills[reff->spell].name, buf2);
        }
    }

    list_zone_commands_room(ch, str_end(buf), rm->vnum);

    /* check the room for a script */
    do_sstat_room(ch, str_end(buf), rm);

    page_string(ch, buf);
}

void stat_spellbook(struct obj_data *obj, char *buf) {
    int spage = 0, fpage = 0;
    struct spell_book_list *entry;
    char list_buf[MAX_STRING_LENGTH];

    list_buf[0] = 0;

    strcpy(buf, "&7&b---Spellbook Contents---&0\r\n");

    /* If we can't get a list of the spells in the book,
     * we need to say *exactly* why. */
    if (!obj->spell_book) {
        strcat(buf, "&1&b*&0 Its spell list is NULL.\r\n");
        return;
    } else if (!obj->spell_book->spell) {
        strcat(buf, "&1&b*&0 The first spell in its spell list is 0.\r\n");
        return;
    }

    for (entry = obj->spell_book; entry; entry = entry->next) {
        spage = fpage + 1;
        fpage = fpage + entry->length;
        sprintf(list_buf, "%s&6%3d-%3d)&0 &6&b%s&0\r\n", list_buf, spage, fpage, skills[entry->spell].name);
    }

    if ((GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES) - fpage) > 1) {
        sprintf(list_buf, "%s&6%3d-%3d)&0 &4&b--Blank--&0\r\n", list_buf, fpage + 1,
                GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES));
    } else if ((GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES) - fpage) == 1) {
        sprintf(list_buf, "%s    &6%3d)&0 &4&b--Blank--&0\r\n", list_buf, GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES));
    }

    strcat(buf, list_buf);
}

void do_stat_object(struct char_data *ch, struct obj_data *j) {
    int i, virtual, found;
    struct obj_data *j2;
    struct event *e;

    extern const char *portal_entry_messages[];
    extern const char *portal_character_messages[];
    extern const char *portal_exit_messages[];

    if (!ch->desc)
        return;

    str_start(buf, sizeof(buf));

    virtual = GET_OBJ_VNUM(j);
    str_catf(buf, "Name: '%s%s%s', Aliases: %s, Level: %d\r\n", CLR(ch, FYEL),
             j->short_description ? j->short_description : "<None>", CLR(ch, ANRM), j->name, GET_OBJ_LEVEL(j));

    if (GET_OBJ_RNUM(j) >= 0)
        strcpy(buf2, obj_index[GET_OBJ_RNUM(j)].func ? "Exists" : "None");
    else
        strcpy(buf2, "None");
    str_catf(buf, "VNum: [%s%5d%s], RNum: [%5d], Type: %s, SpecProc: %s\r\n", CLR(ch, FGRN), virtual, CLR(ch, ANRM),
             GET_OBJ_RNUM(j), OBJ_TYPE_NAME(j), buf2);

    str_catf(buf, "L-Des: %s\r\n", j->description ? j->description : "None");

    if (j->action_description)
        str_catf(buf, "Action desc:\r\n%s%s%s\r\n", CLR(ch, FYEL), j->action_description, CLR(ch, ANRM));

    sprintbit(j->obj_flags.wear_flags, wear_bits, buf1);
    str_catf(buf, "Can be worn on: %s%s%s\r\n", CLR(ch, FCYN), buf1, CLR(ch, ANRM));

    sprintflag(buf1, GET_OBJ_FLAGS(j), NUM_ITEM_FLAGS, extra_bits);
    str_catf(buf, "Extra flags   : %s%s%s\r\n", CLR(ch, FGRN), buf1, CLR(ch, ANRM));

    *buf1 = '\0';
    sprintflag(buf1, GET_OBJ_EFF_FLAGS(j), NUM_EFF_FLAGS, effect_flags);
    str_catf(buf, "Spell Effects : %s%s%s\r\n", CLR(ch, FYEL), buf1, CLR(ch, ANRM));

    str_catf(buf,
             "Weight: %.2f, Value: %d, "
             "Timer: %d, Decomp time: %d, Hiddenness: %ld\r\n",
             GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_TIMER(j), GET_OBJ_DECOMP(j), GET_OBJ_HIDDENNESS(j));

    if (j->in_room == NOWHERE)
        strcpy(buf1, "Nowhere");
    else
        sprintf(buf1, "%d", world[j->in_room].vnum);
    str_catf(buf, "In room: %s, In object: %s, Carried by: %s, Worn by: %s\r\n", buf1,
             j->in_obj ? j->in_obj->short_description : "None", j->carried_by ? GET_NAME(j->carried_by) : "Nobody",
             j->worn_by ? GET_NAME(j->worn_by) : "Nobody");

    switch (GET_OBJ_TYPE(j)) {
    case ITEM_LIGHT:
        if (GET_OBJ_VAL(j, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT)
            str_catf(buf, "Hours left: Infinite\r\n");
        else
            str_catf(buf, "Hours left: [%d]  Initial hours: [%d]\r\n", GET_OBJ_VAL(j, VAL_LIGHT_REMAINING),
                     GET_OBJ_VAL(j, VAL_LIGHT_CAPACITY));
        break;
    case ITEM_SCROLL:
    case ITEM_POTION:
        str_catf(buf, "Spells: (Level %d) %s, %s, %s\r\n", GET_OBJ_VAL(j, VAL_SCROLL_LEVEL),
                 skill_name(GET_OBJ_VAL(j, VAL_SCROLL_SPELL_1)), skill_name(GET_OBJ_VAL(j, VAL_SCROLL_SPELL_2)),
                 skill_name(GET_OBJ_VAL(j, VAL_SCROLL_SPELL_3)));
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        str_catf(buf, "Spell: %s at level %d, %d (of %d) charges remaining\r\n",
                 skill_name(GET_OBJ_VAL(j, VAL_WAND_SPELL)), GET_OBJ_VAL(j, VAL_WAND_LEVEL),
                 GET_OBJ_VAL(j, VAL_WAND_CHARGES_LEFT), GET_OBJ_VAL(j, VAL_WAND_MAX_CHARGES));
        break;
    case ITEM_WEAPON:
        str_catf(buf, "Todam: %dd%d (avg %.1f), Message type: %d, '%s'\r\n", GET_OBJ_VAL(j, VAL_WEAPON_DICE_NUM),
                 GET_OBJ_VAL(j, VAL_WEAPON_DICE_SIZE), WEAPON_AVERAGE(j), GET_OBJ_VAL(j, VAL_WEAPON_DAM_TYPE),
                 GET_OBJ_VAL(j, VAL_WEAPON_DAM_TYPE) >= 0 && GET_OBJ_VAL(j, VAL_WEAPON_DAM_TYPE) <= TYPE_ALIGN - TYPE_HIT
                     ? attack_hit_text[GET_OBJ_VAL(j, VAL_WEAPON_DAM_TYPE)].singular
                     : "<&1INVALID&0>");
        break;
    case ITEM_ARMOR:
    case ITEM_TREASURE:
        str_catf(buf, "AC-apply: [%d]\r\n", GET_OBJ_VAL(j, VAL_ARMOR_AC));
        break;
    case ITEM_TRAP:
        str_catf(buf, "Spell: %d, - Hitpoints: %d\r\n", GET_OBJ_VAL(j, VAL_TRAP_SPELL),
                 GET_OBJ_VAL(j, VAL_TRAP_HITPOINTS));
        break;
    case ITEM_CONTAINER:
        if (!IS_CORPSE(j)) {
            sprintbit(GET_OBJ_VAL(j, VAL_CONTAINER_BITS), container_bits, buf2);
            str_catf(buf, "Weight capacity: %d, Lock Type: %s, Key Num: %d, Weight Reduction: %d%%, Corpse: %s\r\n",
                     GET_OBJ_VAL(j, VAL_CONTAINER_CAPACITY), buf2, GET_OBJ_VAL(j, VAL_CONTAINER_KEY),
                     GET_OBJ_VAL(j, VAL_CONTAINER_WEIGHT_REDUCTION), YESNO(IS_CORPSE(j)));
        } else {
            str_catf(buf,
                     "Weight capacity: %d, Id: %d, Corpse: %s, Player "
                     "Corpse: %s, Raisable: %s\r\n",
                     GET_OBJ_VAL(j, VAL_CONTAINER_CAPACITY), GET_OBJ_VAL(j, VAL_CORPSE_ID), YESNO(IS_CORPSE(j)),
                     YESNO(IS_PLR_CORPSE(j)), YESNO(GET_OBJ_VAL(j, VAL_CONTAINER_CORPSE) == CORPSE_NPC));
        }
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        str_catf(buf, "Capacity: %d, Contains: %d, Poisoned: %s, Liquid: %s\r\n", GET_OBJ_VAL(j, VAL_DRINKCON_CAPACITY),
                 GET_OBJ_VAL(j, VAL_DRINKCON_REMAINING), YESNO(IS_POISONED(j)),
                 LIQ_NAME(GET_OBJ_VAL(j, VAL_DRINKCON_LIQUID)));
        break;
    case ITEM_NOTE:
        str_catf(buf, "Tongue: %d\r\n", GET_OBJ_VAL(j, 0));
        break;
    case ITEM_KEY:
        break;
    case ITEM_FOOD:
        str_catf(buf, "Makes full: %d, Poisoned: %s\r\n", GET_OBJ_VAL(j, VAL_FOOD_FILLINGNESS), YESNO(IS_POISONED(j)));
        break;
    case ITEM_MONEY:
        str_catf(buf, "Coins: %dp %dg %ds %dc\r\n", GET_OBJ_VAL(j, VAL_MONEY_PLATINUM), GET_OBJ_VAL(j, VAL_MONEY_GOLD),
                 GET_OBJ_VAL(j, VAL_MONEY_SILVER), GET_OBJ_VAL(j, VAL_MONEY_COPPER));
        break;
    case ITEM_PORTAL:
        str_catf(buf, "To room: %d\r\n", GET_OBJ_VAL(j, VAL_PORTAL_DESTINATION));
        if (GET_OBJ_VAL(j, VAL_PORTAL_ENTRY_MSG) >= 0) {
            for (i = 0; i < GET_OBJ_VAL(j, VAL_PORTAL_ENTRY_MSG) && *portal_entry_messages[i] != '\n'; ++i)
                ;
            if (*portal_entry_messages[i] != '\n')
                str_catf(buf, "Entry-Room message: %s", portal_entry_messages[i]);
        }
        if (GET_OBJ_VAL(j, VAL_PORTAL_CHAR_MSG) >= 0) {
            for (i = 0; i < GET_OBJ_VAL(j, VAL_PORTAL_CHAR_MSG) && *portal_character_messages[i] != '\n'; ++i)
                ;
            if (*portal_character_messages[i] != '\n')
                str_catf(buf, "To-Char message   : %s", portal_character_messages[i]);
        }
        if (GET_OBJ_VAL(j, VAL_PORTAL_EXIT_MSG) >= 0) {
            for (i = 0; i < GET_OBJ_VAL(j, VAL_PORTAL_EXIT_MSG) && *portal_exit_messages[i] != '\n'; ++i)
                ;
            if (*portal_exit_messages[i] != '\n')
                str_catf(buf, "Exit-Room message : %s", portal_exit_messages[i]);
        }
        break;
    default:
        str_catf(buf, "Values: [%d] [%d] [%d] [%d] [%d] [%d] [%d]\r\n", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
                 GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3), GET_OBJ_VAL(j, 4), GET_OBJ_VAL(j, 5), GET_OBJ_VAL(j, 6));
        break;
    }

    /*
     * I deleted the "equipment status" code from here because it seemed
     * more or less useless and just takes up valuable screen space.
     */

    if (j->contains) {
        sprintf(buf1, "\r\nContents:%s", CLR(ch, FGRN));
        for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
            sprintf(buf1, "%s%s %s", buf1, found++ ? "," : "", j2->short_description);
            if (strlen(buf1) >= 62) {
                str_catf(buf, "%s%s\r\n", buf1, j2->next_content ? "," : "");
                *buf1 = found = 0;
            }
        }

        if (*buf1)
            str_catf(buf, "%s\r\n", buf1);
        str_cat(buf, CLR(ch, ANRM));
    }

    found = 0;
    str_catf(buf, "Applies:");
    for (i = 0; i < MAX_OBJ_APPLIES; i++)
        if (j->applies[i].modifier) {
            sprinttype(j->applies[i].location, apply_types, buf2);
            str_catf(buf, "%s %s", found++ ? "," : "", format_apply(j->applies[i].location, j->applies[i].modifier));
        }
    str_catf(buf, "%s\r\n", found ? "" : " None");

    if (GET_OBJ_TYPE(j) == ITEM_SPELLBOOK)
        stat_spellbook(j, str_end(buf));

    stat_extra_descs(j->ex_description, ch, str_end(buf), FALSE);

    /* Report any events attached. */
    if (j->events) {
        e = j->events;
        str_catf(buf, "Events: %s", eventname(e));
        while (e->next) {
            e = e->next;
            str_catf(buf, " %s", eventname(e));
        }
        str_cat(buf, "\r\n");
    } else {
        str_cat(buf, "No events.\r\n");
    }

    /* check the object for a script */
    do_sstat_object(ch, str_end(buf), j);

    page_string(ch, buf);
}

ACMD(do_estat) {
    struct obj_data *obj;
    char *otarg;
    int tmp, r_num;

    half_chop(argument, buf1, buf2);

    if (!ch->desc)
        return;

    if (subcmd == SCMD_RESTAT || !strcmp(buf1, "room")) {
        if (subcmd == SCMD_RESTAT && *buf1)
            tmp = isdigit(*buf1) ? real_room(atoi(buf1)) : NOWHERE;
        else if (subcmd != SCMD_RESTAT && *buf2)
            tmp = isdigit(*buf2) ? real_room(atoi(buf2)) : NOWHERE;
        else
            tmp = ch->in_room;
        if (tmp == NOWHERE)
            send_to_char("No such room.\r\n", ch);
        else {
            stat_extra_descs(world[tmp].ex_description, ch, buf, TRUE);
            page_string(ch, buf);
        }
    } else {
        if (subcmd == SCMD_OESTAT)
            otarg = buf1;
        else if (!strcmp(buf1, "obj"))
            otarg = buf2;
        else
            otarg = buf1;

        if (!*otarg) {
            send_to_char("Usage: estat room [<vnum>]\r\n", ch);
            send_to_char("       estat obj <name>\r\n", ch);
        } else if (isdigit(*otarg)) {
            tmp = atoi(otarg);
            if ((r_num = real_object(tmp)) < 0)
                send_to_char("There is no object with that number.\r\n", ch);
            else {
                stat_extra_descs(obj_proto[r_num].ex_description, ch, buf, TRUE);
                page_string(ch, buf);
            }
        } else if ((obj = find_obj_around_char(ch, find_vis_by_name(ch, otarg)))) {
            stat_extra_descs(obj->ex_description, ch, buf, TRUE);
            page_string(ch, buf);
        } else {
            send_to_char("No such object around.\r\n", ch);
        }
    }
}

void do_stat_character(struct char_data *ch, struct char_data *k) {
    int i, found, a, mprinted;
    struct group_type *g;
    struct follow_type *fol;
    struct effect *eff;
    struct quest_list *quest;
    struct event *e;
    struct char_data *m;
    memory_rec *memory;

    if (!ch->desc)
        return;

    str_start(buf, sizeof(buf));

    /* Vital character data */
    str_catf(buf, "%s '%s'  IDNum: [%5ld], In room [%5d]\r\n", (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
             GET_NAME(k), GET_IDNUM(k), k->in_room != NOWHERE ? world[k->in_room].vnum : 0);

    /* Prototyped mobile specific data. */
    if (IS_MOB(k))
        str_catf(buf, "Alias: %s, VNum: [%5d], RNum: [%5ld]\r\n", GET_NAMELIST(k), GET_MOB_VNUM(k), GET_MOB_RNUM(k));

    /* Strings */
    if (IS_NPC(k)) {
        str_catf(buf, "L-Des: %s", (GET_LDESC(k) ? GET_LDESC(k) : "<None>\r\n"));
        if (k->player.description) {
            str_catf(buf, "Desc: %s", k->player.description);
        }
    } else
        str_catf(buf, "Title: %s\r\n", (k->player.title ? k->player.title : "<None>"));

    /* Various character stats */
    str_catf(buf, "Race: %s, Race Align: %s, ", RACE_PLAINNAME(k), RACE_ALIGN_ABBR(k));
    /*
      if (!IS_NPC(k))
        str_catf(buf, "Deity: %s, ", GET_DIETY(ch) >= 0 ?
                Dieties[(int) GET_DIETY(ch)].diety_name : "None");
    */
    sprinttype(GET_SEX(k), genders, buf1);
    str_catf(buf,
             "Size: %c%s, Gender: %s\r\n"
             "Life force: %s%c%s&0, Composition: %s%c%s&0\r\n"
             "Class: %s, Lev: [%s%2d%s], XP: [%s%7ld%s], Align: [%4d]\r\n",
             UPPER(*SIZE_DESC(k)), SIZE_DESC(k) + 1, buf1, LIFEFORCE_COLOR(k), UPPER(*LIFEFORCE_NAME(k)),
             LIFEFORCE_NAME(k) + 1, COMPOSITION_COLOR(k), UPPER(*COMPOSITION_NAME(k)), COMPOSITION_NAME(k) + 1,
             CLASS_FULL(k), CLR(ch, FYEL), GET_LEVEL(k), CLR(ch, ANRM), CLR(ch, FYEL), GET_EXP(k), CLR(ch, ANRM),
             GET_ALIGNMENT(k));

    /* Player specific data. */
    if (!IS_NPC(k)) {
        strftime(buf1, MAX_STRING_LENGTH, TIMEFMT_DATE, localtime(&(k->player.time.birth)));
        strftime(buf2, MAX_STRING_LENGTH, TIMEFMT_DATE, localtime(&(k->player.time.logon)));

        str_catf(buf,
                 "Created: [%s], Last Logon: [%s], Played: [%dh %dm]\r\n"
                 "Age: [%d], Homeroom: [%d], Speaks: [%d/%d/%d]",
                 buf1, buf2, k->player.time.played / 3600, ((k->player.time.played / 3600) % 60), age(k).year,
                 GET_HOMEROOM(k), GET_TALK(k, 0), GET_TALK(k, 1), GET_TALK(k, 2));

        if (GET_CLAN(k)) {
            str_catf(buf, ", Clan: [%s], Rank: [%d]", GET_CLAN(k)->abbreviation, GET_CLAN_RANK(k));
        }

        /* Display OLC zones for immorts */
        if (GET_LEVEL(k) >= LVL_IMMORT) {
            struct olc_zone_list *zone;
            *buf1 = '\0';
            for (zone = GET_OLC_ZONES(k); zone; zone = zone->next)
                sprintf(buf1, "%s%d%s", buf1, zone->zone, zone->next ? ", " : "");
            str_catf(buf, ", OLC Zones: [%s]", *buf1 ? buf1 : "NONE");
        }

        str_catf(buf, "\r\n");
    }

    /* Attributes, points, and money. */
    str_catf(buf,
             "         STR   INT   WIS   DEX   CON   CHA\r\n"
             "ACTUAL   %s%3d   %3d   %3d   %3d   %3d   %3d%s\r\n"
             "NATURAL  %s%3d   %3d   %3d   %3d   %3d   %3d%s\r\n"
             "AFFECTED %s%3d   %3d   %3d   %3d   %3d   %3d%s\r\n",
             CLR(ch, FCYN), GET_ACTUAL_STR(k), GET_ACTUAL_INT(k), GET_ACTUAL_WIS(k), GET_ACTUAL_DEX(k),
             GET_ACTUAL_CON(k), GET_ACTUAL_CHA(k), CLR(ch, ANRM), CLR(ch, FCYN), GET_NATURAL_STR(k), GET_NATURAL_INT(k),
             GET_NATURAL_WIS(k), GET_NATURAL_DEX(k), GET_NATURAL_CON(k), GET_NATURAL_CHA(k), CLR(ch, ANRM),
             CLR(ch, FCYN), GET_AFFECTED_STR(k), GET_AFFECTED_INT(k), GET_AFFECTED_WIS(k), GET_AFFECTED_DEX(k),
             GET_AFFECTED_CON(k), GET_AFFECTED_CHA(k), CLR(ch, ANRM));

    str_catf(buf, "HP: [%s%d/%d+%d%s]  MP: [%s%d/%d+%d%s]  MV: [%s%d/%d+%d%s]\r\n", CLR(ch, FGRN), GET_HIT(k),
             GET_MAX_HIT(k), hit_gain(k), CLR(ch, ANRM), CLR(ch, FGRN), GET_MANA(k), GET_MAX_MANA(k), mana_gain(k),
             CLR(ch, ANRM), CLR(ch, FGRN), GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k), CLR(ch, ANRM));

    str_catf(buf,
             "Coins: [%s%d%sp / %s%d%sg / %s%d%ss / %s%d%sc], "
             "Bank: [%s%d%sp / %s%d%sg / %s%d%ss / %s%d%sc]\r\n",
             CLR(ch, FCYN), GET_PLATINUM(k), CLR(ch, ANRM), CLR(ch, HYEL), GET_GOLD(k), CLR(ch, ANRM), CLR(ch, FWHT),
             GET_SILVER(k), CLR(ch, ANRM), CLR(ch, FYEL), GET_COPPER(k), CLR(ch, ANRM), CLR(ch, FCYN),
             GET_BANK_PLATINUM(k), CLR(ch, ANRM), CLR(ch, HYEL), GET_BANK_GOLD(k), CLR(ch, ANRM), CLR(ch, FWHT),
             GET_BANK_SILVER(k), CLR(ch, ANRM), CLR(ch, FYEL), GET_BANK_COPPER(k), CLR(ch, ANRM));

    str_catf(buf,
             "AC: [%d/10], Hitroll: [%2d], Damroll: [%2d], "
             "Saving throws: [%d/%d/%d/%d/%d]\r\n"
             "Perception: [%4ld], Hiddenness: [%4ld], Rage: [%4d]\r\n",
             GET_AC(k) + 5 * monk_weight_penalty(k), GET_HITROLL(k) - monk_weight_penalty(k),
             GET_DAMROLL(k) - monk_weight_penalty(k), GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2), GET_SAVE(k, 3),
             GET_SAVE(k, 4), GET_PERCEPTION(k), GET_HIDDENNESS(k), GET_RAGE(k));

    /* Status data. */
    sprinttype(GET_POS(k), position_types, buf1);
    sprinttype(GET_STANCE(k), stance_types, buf2);
    str_catf(buf, "Pos: %s (%s)", buf1, buf2);
    if (IS_NPC(k)) {
        sprinttype(k->mob_specials.default_pos, position_types, buf1);
        str_catf(buf, ", Default Pos: %s", buf1);
    }
    str_catf(buf, ", Fighting: %s", FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "<none>");
    if (k->forward)
        str_catf(buf, ", %s into: %s", GET_LEVEL(k) > LVL_IMMORT ? "Switched" : "Shapechanged", GET_NAME(k->forward));
    str_cat(buf, "\r\n");

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
        str_catf(buf, "%s\r\n", buf2);

    if (IS_MOB(k)) {
        if (k->mob_specials.attack_type >= 0 && k->mob_specials.attack_type <= TYPE_STAB - TYPE_HIT)
            strcpy(buf2, attack_hit_text[k->mob_specials.attack_type].singular);
        else
            strcpy(buf2, "<&1INVALID&0>");
        str_catf(buf, "Mob Spec-Proc: %s, NPC Bare Hand Dam: %dd%d, Attack type: %s\r\n",
                 (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"), k->mob_specials.damnodice,
                 k->mob_specials.damsizedice, buf2);
    }

    /* Character flags. */
    if (IS_NPC(k)) {
        sprintflag(buf1, MOB_FLAGS(k), NUM_MOB_FLAGS, action_bits);
        str_catf(buf, "NPC flags: %s%s%s\r\n", CLR(ch, FCYN), buf1, CLR(ch, ANRM));
    } else {
        sprintflag(buf2, PLR_FLAGS(k), NUM_PLR_FLAGS, player_bits);
        str_catf(buf, "PLR: %s%s%s\r\n", CLR(ch, FCYN), buf2, CLR(ch, ANRM));
        sprintflag(buf2, PRF_FLAGS(k), NUM_PRF_FLAGS, preference_bits);
        str_catf(buf, "PRF: %s%s%s\r\n", CLR(ch, FGRN), buf2, CLR(ch, ANRM));
        sprintflag(buf2, PRV_FLAGS(k), NUM_PRV_FLAGS, privilege_bits);
        str_catf(buf, "PRV: %s%s%s\r\n", CLR(ch, FGRN), buf2, CLR(ch, ANRM));
    }

    /* Weight and objects. */
    for (i = a = found = 0; i < NUM_WEARS; i++)
        if (GET_EQ(k, i)) {
            ++found;
            a += GET_OBJ_WEIGHT(GET_EQ(k, i));
        }
    str_catf(buf,
             "Max Carry: %d (%d weight); "
             "Carried: %d (%.2f weight); "
             "Worn: %d (%d weight)\r\n",
             CAN_CARRY_N(k), CAN_CARRY_W(k), IS_CARRYING_N(k), IS_CARRYING_W(k), found, a);

    /* Conditions. */
    str_cat(buf, "Hunger: ");
    if (GET_COND(k, FULL) < 0)
        str_cat(buf, "Off");
    else
        str_catf(buf, "%d", GET_COND(k, FULL));
    str_cat(buf, ", Thirst: ");
    if (GET_COND(k, THIRST) < 0)
        str_cat(buf, "Off");
    else
        str_catf(buf, "%d", GET_COND(k, THIRST));
    str_cat(buf, ", Drunk: ");
    if (GET_COND(k, DRUNK) < 0)
        str_cat(buf, "Off\r\n");
    else
        str_catf(buf, "%d\r\n", GET_COND(k, DRUNK));

    /* Group, follower, guard, etc. */
    sprintf(buf1, "Consented: %s, Master is: %s, Followers are:", (CONSENT(ch) ? GET_NAME(CONSENT(ch)) : "<none>"),
            (k->master ? GET_NAME(k->master) : "<none>"));

    found = 0;
    for (fol = k->followers; fol; fol = fol->next) {
        if (strlen(buf1) + strlen(PERS(fol->follower, ch)) >= 78) {
            str_catf(buf, "%s%s\r\n", buf1, found ? "," : "");
            *buf1 = found = 0;
        }
        sprintf(buf1, "%s%s %s", buf1, found++ ? "," : "", PERS(fol->follower, ch));
    }
    if (*buf1)
        str_catf(buf, "%s\r\n", buf1);

    /* Group list */
    sprintf(buf1, "&0&2&bGroup Master&0 is: %s, &0&2&bgroupees are:&0",
            ((k->group_master) ? GET_NAME(k->group_master) : "<none>"));

    found = 0;
    for (g = k->groupees; g; g = g->next) {
        if (strlen(buf1) + strlen(PERS(g->groupee, ch)) >= 78) {
            str_catf(buf, "%s%s\r\n", buf1, found ? "," : "");
            *buf1 = found = 0;
        }
        sprintf(buf1, "%s%s %s", buf1, found++ ? "," : "", PERS(g->groupee, ch));
    }
    if (*buf1)
        str_catf(buf, "%s\r\n", buf1);

    if (k->guarding || k->guarded_by)
        str_catf(buf, "Guarding: %s, Guarded by: %s\r\n", k->guarding ? GET_NAME(k->guarding) : "<none>",
                 k->guarded_by ? GET_NAME(k->guarded_by) : "<none>");
    if (k->cornering || k->cornered_by)
        str_catf(buf, "Cornering: %s, Cornered by: %s\r\n", k->cornering ? GET_NAME(k->cornering) : "<none>",
                 k->cornered_by ? GET_NAME(k->cornered_by) : "<none>");

    /* Effect bitvectors */
    sprintflag(buf1, EFF_FLAGS(k), NUM_EFF_FLAGS, effect_flags);
    str_catf(buf, "EFF: %s%s%s\r\n", CLR(ch, FYEL), buf1, CLR(ch, ANRM));

    /* NPC spell circle status */
    if (IS_NPC(k) && MEM_MODE(ch) != MEM_NONE) {
        const char *color;
        str_cat(buf, "Spellbank: ");
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
                str_catf(buf, "%d(%s%d@0) ", i, color, GET_MOB_SPLBANK(k, i));
            }
        str_cat(buf, "\r\n");
    }

    /* List spells the character is affected by. */
    for (eff = k->effects; eff; eff = eff->next) {
        if (eff->duration < 0)
            str_catf(buf, "SPL: (perma) %s%-21s%s ", CLR(ch, FCYN), skills[eff->type].name, CLR(ch, ANRM));
        else
            str_catf(buf, "SPL: (%3dhr) %s%-21s%s ", eff->duration + 1, CLR(ch, FCYN), skills[eff->type].name,
                     CLR(ch, ANRM));
        if (eff->modifier)
            str_catf(buf, "%+d to %s", eff->modifier, apply_types[(int)eff->location]);
        if (HAS_FLAGS(eff->flags, NUM_EFF_FLAGS)) {
            sprintflag(buf1, eff->flags, NUM_EFF_FLAGS, effect_flags);
            str_catf(buf, "%ssets %s", eff->modifier ? ", " : "", buf1);
        }
        str_cat(buf, "\r\n");
    }

    /* Run through the quests the player is on */
    if (k->quests) {
        if (all_quests) {
            quest = k->quests;
            while (quest) {
                if ((a = real_quest(quest->quest_id)) >= 0) {
                    str_catf(buf, "Quest %s: ", all_quests[a].quest_name);
                    if (quest->stage == QUEST_SUCCESS)
                        str_cat(buf, "Completed\r\n");
                    else if (quest->stage == QUEST_FAILURE)
                        str_cat(buf, "Failed\r\n");
                    else
                        str_catf(buf, "Stage %d\r\n", quest->stage);
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
        str_catf(buf, "Events: %s", eventname(e));
        while (e->next) {
            e = e->next;
            str_catf(buf, " %s", eventname(e));
        }
        str_cat(buf, "\r\n");
    } else {
        str_cat(buf, "No events.\r\n");
    }

    /* List cooldowns */
    for (i = found = 0; i < NUM_COOLDOWNS; ++i) {
        if (!GET_COOLDOWN(k, i))
            continue;
        if (!found)
            str_cat(buf, "Cooldowns:\r\n");
        str_catf(buf, "%25s: %d/%d sec\r\n", cooldowns[i], GET_COOLDOWN(k, i) / PASSES_PER_SEC,
                 GET_COOLDOWN_MAX(k, i) / PASSES_PER_SEC);
        found = TRUE;
    }

    /* Check mobiles for a script */
    if (IS_NPC(k))
        do_sstat_character(ch, str_end(buf), k);

    /* List enemies the mob remembers */
    mprinted = 0;
    for (memory = MEMORY(k); memory; memory = memory->next) {
        for (m = character_list; m; m = m->next)
            if (GET_IDNUM(m) == memory->id) {
                if (!mprinted) {
                    str_catf(buf, "Remembered enemies: &2%s&0", GET_NAME(m));
                    mprinted = 1;
                } else {
                    str_catf(buf, ", &2%s&0", GET_NAME(m));
                }
                break;
            }
    }
    if (mprinted)
        str_cat(buf, "\r\n");

    /* List permanent player titles */
    if (!IS_NPC(k) && GET_PERM_TITLES(k)) {
        str_cat(buf, "Permanent Titles:\r\n");
        for (i = 0; GET_PERM_TITLES(k)[i]; ++i)
            str_catf(buf, "  %d) %s\r\n", i + 1, GET_PERM_TITLES(k)[i]);
    }

    page_string(ch, buf);
}

ACMD(do_stat) {
    void do_stat_shop(struct char_data * ch, char *arg);
    struct char_data *victim = 0;
    struct obj_data *object = 0;
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
            send_to_char("No such room.\r\n", ch);
        else
            do_stat_room(ch, tmp);
    } else if (subcmd == SCMD_SSTAT || is_abbrev(buf1, "shop")) {
        if (subcmd == SCMD_SSTAT)
            do_stat_shop(ch, buf1);
        else
            do_stat_shop(ch, buf2);
    } else if (!*buf1) {
        send_to_char("Stats on who or what?\r\n", ch);
    } else if (is_abbrev(buf1, "mob")) {
        if (!*buf2)
            send_to_char("Stats on which mobile?\r\n", ch);
        else {
            if ((victim = find_char_around_char(ch, find_vis_by_name(ch, buf2))))
                do_stat_character(ch, victim);
            else
                send_to_char("No such mobile around.\r\n", ch);
        }
    } else if (is_abbrev(buf1, "player")) {
        if (!*buf2) {
            send_to_char("Stats on which player?\r\n", ch);
        } else {
            if ((victim = find_char_around_char(ch, find_vis_plr_by_name(ch, buf2))))
                do_stat_character(ch, victim);
            else
                send_to_char("No such player around.\r\n", ch);
        }
    } else if (is_abbrev(buf1, "file")) {
        if (!*buf2) {
            send_to_char("Stats on which player?\r\n", ch);
        } else {
            CREATE(victim, struct char_data, 1);
            clear_char(victim);
            if (load_player(buf2, victim) > -1) {
                if (GET_LEVEL(victim) > GET_LEVEL(ch))
                    send_to_char("Sorry, you can't do that.\r\n", ch);
                else
                    do_stat_character(ch, victim);
                free_char(victim);
            } else {
                send_to_char("There is no such player.\r\n", ch);
                free(victim);
            }
        }
    } else if (is_abbrev(buf1, "object")) {
        if (!*buf2)
            send_to_char("Stats on which object?\r\n", ch);
        else {
            if ((object = find_obj_around_char(ch, find_vis_by_name(ch, buf2))))
                do_stat_object(ch, object);
            else
                send_to_char("No such object around.\r\n", ch);
        }
    } else {
        if ((object = find_obj_in_eq(ch, NULL, find_vis_by_name(ch, buf1))))
            do_stat_object(ch, object);
        else if ((object = find_obj_in_list(ch->carrying, find_vis_by_name(ch, buf1))))
            do_stat_object(ch, object);
        else if ((victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf1))))
            do_stat_character(ch, victim);
        else if ((object = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, buf1))))
            do_stat_object(ch, object);
        else if ((victim = find_char_around_char(ch, find_vis_by_name(ch, buf1))))
            do_stat_character(ch, victim);
        else if ((object = find_obj_in_world(find_vis_by_name(ch, buf1))))
            do_stat_object(ch, object);
        else
            send_to_char("Nothing around by that name.\r\n", ch);
    }
}

ACMD(do_vstat) {
    struct char_data *mob;
    struct obj_data *obj;
    int number, r_num;

    two_arguments(argument, buf, buf2);

    if (subcmd == SCMD_VSTAT && (!*buf || !*buf2 || !isdigit(*buf2))) {
        send_to_char("Usage: vstat { obj | mob } <number>\r\n", ch);
    } else if (subcmd == SCMD_MSTAT && (!*buf || !isdigit(*buf))) {
        send_to_char("Usage: mstat <number>\r\n", ch);
    } else if (subcmd == SCMD_OSTAT && (!*buf || !isdigit(*buf))) {
        send_to_char("Usage: ostat <number>\r\n", ch);
    } else if ((number = (subcmd == SCMD_VSTAT ? atoi(buf2) : atoi(buf))) < 0) {
        send_to_char("A NEGATIVE number??\r\n", ch);
    } else if (subcmd == SCMD_MSTAT || (subcmd == SCMD_VSTAT && is_abbrev(buf, "mob"))) {
        if ((r_num = real_mobile(number)) < 0) {
            send_to_char("There is no monster with that number.\r\n", ch);
            return;
        }
        mob = read_mobile(r_num, REAL);
        char_to_room(mob, 0);
        do_stat_character(ch, mob);
        extract_char(mob);
    } else if (subcmd == SCMD_OSTAT || (subcmd == SCMD_VSTAT && is_abbrev(buf, "obj"))) {
        if ((r_num = real_object(number)) < 0) {
            send_to_char("There is no object with that number.\r\n", ch);
            return;
        }
        obj = read_object(r_num, REAL);
        do_stat_object(ch, obj);
        extract_obj(obj);
    } else
        send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}

ACMD(do_zstat) {
    int vnum, rnum;
    char str[MAX_INPUT_LENGTH];
    struct zone_data *z;

    half_chop(argument, str, argument);

    if (!*str)
        vnum = IN_ZONE_VNUM(ch);
    else
        vnum = atoi(str);

    rnum = find_zone(vnum);
    if (rnum == -1) {
        send_to_char("There is no such zone.\r\n", ch);
        return;
    }

    z = &(zone_table[rnum]);
    sprintf(buf,
            "Zone &6%d&0: &2%s&0\r\n"
            "Vnum range     : &3%d-%d&0\r\n"
            "Reset mode     : &3%s&0\r\n"
            "Lifespan       : &3%d minutes&0\r\n"
            "Zone factor%%   : &3%d&0\r\n"
            "Hemisphere     : &3%s&0\r\n"
            "Climate        : &3%s&0\r\n"
            "Temperature    : &6%d degrees&0\r\n"
            "Precipitation  : &6%d&0\r\n"
            "Wind speed     : &6%s&0 (&6%d&0)\r\n"
            "Wind direction : &6%s&0\r\n",
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
        send_to_char("'players <letter>' shows all player names beginning with <letter>\r\n", ch);
        return;
    }

    *arg = tolower(*arg);

    /* Go through all the names in the pfile */
    for (i = 0; i <= top_of_p_table; i++) {
        /* Check if the first letter matches the argument */
        if (tolower(*player_table[i].name) == *arg) {
            ++count;

            if (buflen > sizeof(buf) - 100) {
                buflen += sprintf(buf + buflen, "\r\nToo many players!  Buffer full.");
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
                              player_table[i].level, count % 3 ? "" : "\r\n");
        }
    }

    if (count == 0)
        strcpy(buf, "No players for that letter.\r\n");
    else if (count % 3)
        strcpy(buf + buflen, "\r\n");

    /* Show the name list */
    page_string(ch, buf);
}

ACMD(do_last) {
    struct char_data *victim;

    one_argument(argument, arg);
    if (!*arg) {
        send_to_char("For whom do you wish to search?\r\n", ch);
        return;
    }

    CREATE(victim, struct char_data, 1);
    clear_char(victim);

    if (load_player(arg, victim) < 0) {
        send_to_char("There is no such player.\r\n", ch);
        free(victim);
        return;
    }
    if (GET_LEVEL(victim) > GET_LEVEL(ch) && GET_LEVEL(ch) < LVL_HEAD_B) {
        send_to_char("You are not sufficiently godly for that!\r\n", ch);
        free_char(victim);
        return;
    }
    sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n", GET_IDNUM(victim), GET_LEVEL(victim), CLASS_ABBR(victim),
            GET_NAME(victim), GET_HOST(victim), ctime(&victim->player.time.logon));
    send_to_char(buf, ch);

    free_char(victim);
}

/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

static void print_zone_to_buf(char *bufptr, int zone) {
    str_catf(bufptr, "%3d %-30.30s Age: %3d; Reset: %3d (%1d); ZF:%d: Top: %5d\r\n", zone_table[zone].number,
             zone_table[zone].name, zone_table[zone].age, zone_table[zone].lifespan, zone_table[zone].reset_mode,
             zone_table[zone].zone_factor, zone_table[zone].top);
}

void do_show_sectors(struct char_data *ch, char *argument) {
    int i;
    const struct sectordef *s;

    pprintf(ch, "Sector type     Mv  Camp  Wet  Notes\r\n");
    pprintf(ch,
            "--------------  --  ----  ---  "
            "----------------------------------------------\r\n");
    for (i = 0; i < NUM_SECTORS; i++) {
        s = &sectors[i];
        pprintf(ch, " %s%-13s&0  %2d  %s  %s  %s\r\n", s->color, s->name, s->mv, s->campable ? "&2Camp&0" : "    ",
                s->wet ? "&6Wet&0" : "   ", s->notes);
    }
    start_paging(ch);
}

void do_show_compositions(struct char_data *ch, char *argument) {
    int i;

    sprintf(buf,
            "Idx  Composition  %sSlash&0  %sPierce&0  %sCrush&0  %sShock&0  "
            "%sFire&0  %sWater&0  %sCold&0  %sAcid&0  %sPoison&0\r\n",
            damtypes[DAM_SLASH].color, damtypes[DAM_PIERCE].color, damtypes[DAM_CRUSH].color, damtypes[DAM_SHOCK].color,
            damtypes[DAM_FIRE].color, damtypes[DAM_WATER].color, damtypes[DAM_COLD].color, damtypes[DAM_ACID].color,
            damtypes[DAM_POISON].color);
    send_to_char(buf, ch);
    send_to_char("---  -----------  -----  ------  -----  -----   ---  ----  -----  ----  ------\r\n", ch);
    for (i = 0; i < NUM_COMPOSITIONS; i++) {
        sprintf(buf,
                "%2d.  %s%c%-10s  % 5d  % 6d  % 5d  % 5d  % 4d  % 4d  % 5d  % 4d  "
                "% 6d&0\r\n",
                i, compositions[i].color, UPPER(*(compositions[i].name)), compositions[i].name + 1,
                compositions[i].sus_slash, compositions[i].sus_pierce, compositions[i].sus_crush,
                compositions[i].sus_shock, compositions[i].sus_fire, compositions[i].sus_water,
                compositions[i].sus_cold, compositions[i].sus_acid, compositions[i].sus_poison);
        send_to_char(buf, ch);
    }
}

void do_show_lifeforces(struct char_data *ch, char *argument) {
    int i;

    sprintf(buf, "Idx  Life force   %sHeal&0  %sDisc.&0  %sDispel&0  %sMental&0\r\n", damtypes[DAM_HEAL].color,
            damtypes[DAM_DISCORPORATE].color, damtypes[DAM_DISPEL].color, damtypes[DAM_MENTAL].color);
    send_to_char(buf, ch);
    send_to_char("---  -----------  ----  -----  ------  ------\r\n", ch);
    for (i = 0; i < NUM_LIFEFORCES; i++) {
        sprintf(buf, "%2d.  %s%c%-10s  % 4d  % 5d  % 6d  % 6d&0\r\n", i, lifeforces[i].color,
                UPPER(*(lifeforces[i].name)), lifeforces[i].name + 1, lifeforces[i].sus_heal,
                lifeforces[i].sus_discorporate, lifeforces[i].sus_dispel, lifeforces[i].sus_mental);
        send_to_char(buf, ch);
    }
}

void do_show_damtypes(struct char_data *ch, char *argument) {
    int i;

    send_to_char("Idx  Damage type      Verb 1st         Verb 2nd          Action \r\n", ch);
    send_to_char("---  -------------    -------------    --------------    ----------\r\n", ch);
    for (i = 0; i < NUM_DAMTYPES; i++) {
        sprintf(buf, "%2d.  %s%-15s  %-15s  %-15s   %-15s&0\r\n", i, damtypes[i].color, damtypes[i].name,
                damtypes[i].verb1st, damtypes[i].verb2nd, damtypes[i].action);
        send_to_char(buf, ch);
    }
}

void do_show_zones(struct char_data *ch, char *argument) {
    int zrnum;
    char zonebuf[50000];

    any_one_arg(argument, arg);

    str_start(zonebuf, sizeof(zonebuf));

    if (!strcmp(arg, ".")) {
        print_zone_to_buf(zonebuf, world[ch->in_room].zone);
        send_to_char(zonebuf, ch);
    } else if (*arg && is_number(arg)) {
        if ((zrnum = find_zone(atoi(arg))) == NOWHERE)
            send_to_char("That is not a valid zone.\r\n", ch);
        else {
            print_zone_to_buf(zonebuf, zrnum);
            send_to_char(zonebuf, ch);
        }
    } else {
        for (zrnum = 0; zrnum <= top_of_zone_table; ++zrnum)
            print_zone_to_buf(zonebuf, zrnum);
        page_string(ch, zonebuf);
    }
}

void do_show_player(struct char_data *ch, char *argument) {
    struct char_data *vict;

    one_argument(argument, arg);

    if (!*arg) {
        cprintf(ch, "A name would help.\r\n");
        return;
    }

    CREATE(vict, struct char_data, 1);
    clear_char(vict);

    if (load_player(arg, vict) < 0) {
        cprintf(ch, "There is no such player.\r\n");
        free(vict);
        return;
    }
    strftime(buf1, MAX_STRING_LENGTH, TIMEFMT_DATE, localtime(&(vict->player.time.birth)));
    strftime(buf2, MAX_STRING_LENGTH, TIMEFMT_DATE, localtime(&(vict->player.time.logon)));
    cprintf(ch, "Player: %-12s (%s) [%2d %s] (%s)\r\n", GET_NAME(vict), genders[(int)GET_SEX(vict)], GET_LEVEL(vict),
            CLASS_ABBR(vict), RACE_ABBR(vict));
    cprintf(ch,
            "Coins held:    [%7dp / %7dg / %7ds / %7dc]\r\n"
            "Coins banked:  [%7dp / %7dg / %7ds / %7dc]\r\n"
            "Exp: %-8ld   Align: %-5d\r\n"
            "Started: %s   Last: %s   Played: %3dh %2dm\r\n",
            GET_PLATINUM(vict), GET_GOLD(vict), GET_SILVER(vict), GET_COPPER(vict), GET_BANK_PLATINUM(vict),
            GET_BANK_GOLD(vict), GET_BANK_SILVER(vict), GET_BANK_COPPER(vict), GET_EXP(vict), GET_ALIGNMENT(vict), buf1,
            buf2, (int)(vict->player.time.played / 3600), (int)(vict->player.time.played / 60 % 60));
    cprintf(ch, "\r\n");
    send_save_description(vict, ch, FALSE);
    free_char(vict);
}

void do_show_stats(struct char_data *ch, char *argument) {
    int mobiles = 0;
    int players = 0;
    int connected = 0;
    int objects = 0;
    struct char_data *vict;
    struct obj_data *obj;

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
            "Current stats:\r\n"
            "   %5d players in game       %5d connected\r\n"
            "   %5d registered\r\n"
            "   %5d mobiles               %5d prototypes\r\n"
            "   %5d objects               %5d prototypes\r\n"
            "   %5d rooms                 %5d zones\r\n"
            "   %5d large bufs\r\n"
            "   %5d buf switches          %5d overflows\r\n",
            players, connected, top_of_p_table + 1, mobiles, top_of_mobt + 1, objects, top_of_objt + 1,
            top_of_world + 1, top_of_zone_table + 1, buf_largecount, buf_switches, buf_overflows);
    send_to_char(buf, ch);
}

void do_show_errors(struct char_data *ch, char *argument) {
    int j, rn;

    strcpy(buf, "Errant Rooms\r\n------------\r\n");
    for (rn = 0; rn <= top_of_world; ++rn) {
        buf2[0] = '\0';
        for (j = 0; j < NUM_OF_DIRS; j++) {
            if (world[rn].exits[j]) {
                if (world[rn].exits[j]->to_room == 0)
                    sprintf(buf2, "%s%s to void, ", buf2, dirs[j]);
                if (world[rn].exits[j]->to_room == NOWHERE && !world[rn].exits[j]->general_description)
                    sprintf(buf2, "%s%s to NOWHERE, ", buf2, dirs[j]);
            }
        }
        if (buf2[0]) {
            buf2[strlen(buf2) - 2] = '\0'; /* cut off last comma */
            sprintf(buf, "%s [%5d] %-30s %s\r\n", buf, world[rn].vnum, world[rn].name, buf2);
        }
    }
    send_to_char(buf, ch);
}

void do_show_death(struct char_data *ch, char *argument) {
    int room, found;

    str_start(buf, sizeof(buf));
    str_cat(buf, "Death Traps\r\n-----------\r\n");
    for (room = 0, found = 0; room <= top_of_world; ++room)
        if (ROOM_FLAGGED(room, ROOM_DEATH))
            str_catf(buf, "%2d: [%5d] %s\r\n", ++found, world[room].vnum, world[room].name);
    send_to_char(buf, ch);
}

void do_show_godrooms(struct char_data *ch, char *argument) {
    room_num room;
    int found;

    str_start(buf, sizeof(buf));
    str_cat(buf, "Godrooms\r\n--------------------------\r\n");
    for (room = 0, found = 0; room <= top_of_world; ++room)
        if (ROOM_FLAGGED(room, ROOM_GODROOM))
            str_catf(buf, "%2d: [%5d] %s\r\n", ++found, world[room].vnum, world[room].name);
    send_to_char(buf, ch);
}

void do_show_houses(struct char_data *ch, char *argument) {
    extern void hcontrol_list_houses(struct char_data * ch);

    hcontrol_list_houses(ch);
}

void do_show_notes(struct char_data *ch, char *argument) {
    FILE *notes;

    any_one_arg(argument, arg);

    if (!*arg) {
        send_to_char("A name would help.\r\n", ch);
        return;
    }
    if (!get_pfilename(arg, buf2, NOTES_FILE)) {
        send_to_char("Couldn't find that player.\r\n", ch);
        return;
    }

    if (!(notes = fopen(buf2, "rt"))) {
        send_to_char("There are no notes for that file.\r\n", ch);
        return;
    }

    str_start(buf, sizeof(buf));

    while (get_line(notes, buf2)) {
        str_cat(buf, buf2);
        str_cat(buf, "\r\n");
    }

    fclose(notes);
    page_string(ch, buf);
}

void do_show_classes(struct char_data *ch, char *argument) {
    int i, chars;
    char *mem_mode;

    str_start(buf, sizeof(buf));
    str_cat(buf,
            "Class          Magic  Homeroom  Base Class  Max Subclass Level\r\n"
            "-------------  -----  --------  ----------  ------------------\r\n");
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
        str_catf(buf, " %-*.*s  %-5s  %-8d  %-10.10s  %s\r\n", 13 + chars, 13 + chars, classes[i].displayname, mem_mode,
                 classes[i].homeroom,
                 classes[i].subclass_of == CLASS_UNDEFINED ? "" : classes[classes[i].subclass_of].plainname, buf1);
    }
    page_string(ch, buf);
}

void do_show_races(struct char_data *ch, char *argument) {
    int i, chars;

    skip_spaces(&argument);

    str_start(buf, sizeof(buf));

    if (!*argument) {
        str_cat(buf,
                "Race              Humanoid  Align    Size      HR/DR\r\n"
                "----------------  --------  -------  --------  ------\r\n");
        for (i = 0; i < NUM_RACES; i++) {
            chars = count_color_chars(races[i].fullname);
            str_catf(buf, " %-*.*s   %-8s  %5d    %-8.8s  %d/%d\r\n", 15 + chars, 15 + chars, races[i].fullname,
                     YESNO(races[i].humanoid), races[i].def_align, sizes[races[i].def_size].name,
                     races[i].bonus_hitroll, races[i].bonus_damroll);
        }
    } else if ((i = parse_race(ch, ch, argument)) == RACE_UNDEFINED)
        return;
    else {
        struct racedef *race = &races[i];
        str_catf(buf,
                 "Race              : %s@0 (@g%d@0) @c%s@0\r\n"
                 "Playable?         : @c%s@0\r\n"
                 "Humanoid?         : @c%s@0\r\n"
                 "Race Alignment    : @c%s@0\r\n"
                 "Default Alignment : %s%d@0\r\n"
                 "Default Size      : %s%s@0\r\n"
                 "Default Lifeforce : %s%s@0\r\n"
                 "Def. Composition  : %s%s@0\r\n"
                 "Damroll/Hitroll   : @c%d@0/@c%d@0\r\n"
                 "Weight            : male: @c%d@0-@c%d@0 female: @c%d@0-@c%d@0\r\n"
                 "Height            : male: @c%d@0-@c%d@0 female: @c%d@0-@c%d@0\r\n",
                 race->fullname, i, race->names, YESNO(race->playable), YESNO(race->humanoid),
                 race_align_abbrevs[race->racealign], align_color(race->def_align), race->def_align,
                 sizes[race->def_size].color, sizes[race->def_size].name, lifeforces[race->def_lifeforce].color,
                 lifeforces[race->def_lifeforce].name, compositions[race->def_composition].color,
                 compositions[race->def_composition].name, race->bonus_damroll, race->bonus_hitroll, race->mweight_lo,
                 race->mweight_hi, race->fweight_lo, race->fweight_hi, race->mheight_lo, race->mheight_hi,
                 race->fheight_lo, race->fheight_hi);

        sprintflag(buf2, race->effect_flags, NUM_EFF_FLAGS, effect_flags);
        str_catf(buf,
                 "Attribute Scales  : Str  Dex  Int  Wis  Con  Cha\r\n"
                 "                  : @c%3d%% %3d%% %3d%% %3d%% %3d%% %3d%%@0\r\n"
                 "Perm. Effects     : @y%s@0\r\n",
                 race->attrib_scales[0], race->attrib_scales[1], race->attrib_scales[2], race->attrib_scales[3],
                 race->attrib_scales[4], race->attrib_scales[5], buf2);

        str_cat(buf, "Skills            : @c");
        for (i = 0; race->skills[i].skill; ++i)
            str_catf(buf, "%s%s", i == 0 ? "" : "@0, @c", skills[race->skills[i].skill].name);
        str_cat(buf, i == 0 ? "None.@0\r\n" : "@0\r\n");

        str_catf(buf,
                 "Mob Factors\r\n"
                 "  Experience      : @c%d%%@0\r\n"
                 "  Hitpoints       : @c%d%%@0\r\n"
                 "  Hitroll/Damroll : @c%d%%@0\r\n"
                 "  Damdice         : @c%d%%@0\r\n"
                 "  Copper          : @c%d%%@0\r\n"
                 "  Armor Class     : @c%d%%@0\r\n",
                 race->exp_factor, race->hit_factor, race->hd_factor, race->dice_factor, race->copper_factor,
                 race->ac_factor);
    }
    page_string(ch, buf);
}

void do_show_exp(struct char_data *ch, char *argument) {
    int clazz, level;

    any_one_arg(argument, arg);

    if (!*arg) {
        send_to_char("Usage: show exp <class>\r\n", ch);
        return;
    }

    for (clazz = 0; clazz < NUM_CLASSES; ++clazz) {
        if (is_abbrev(arg, classes[clazz].name)) {
            str_start(buf, sizeof(buf));
            str_cat(buf,
                    "   Level           Experience Needed           Total\r\n"
                    " ---------         -----------------         ---------\r\n");
            for (level = 1; level < LVL_IMPL; ++level)
                str_catf(buf, " %3d - %3d                 %9ld         %9ld\r\n", level, level + 1,
                         exp_next_level(level, clazz) - exp_next_level(level - 1, clazz), exp_next_level(level, clazz));
            page_string(ch, buf);
            return;
        }
    }
    send_to_char("Invalid class.\r\n", ch);
}

void do_show_snoop(struct char_data *ch, char *argument) {
    int i = 0;
    struct descriptor_data *d;

    send_to_char(
        "People currently snooping:\r\n"
        "--------------------------\r\n",
        ch);
    for (d = descriptor_list; d; d = d->next) {
        if (d->snooping == NULL || d->character == NULL)
            continue;
        if (STATE(d) != CON_PLAYING || GET_LEVEL(ch) < GET_LEVEL(d->character))
            continue;
        if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
            continue;
        i++;
        sprintf(buf, "%-10s%s - snooped by %s%s.\r\n", GET_NAME(d->snooping->character), QNRM, GET_NAME(d->character),
                QNRM);
        send_to_char(buf, ch);
    }
    if (i == 0)
        send_to_char("No one is currently snooping.\r\n", ch);
}

void do_show_sizes(struct char_data *ch, char *argument) { show_sizes(ch); }

/* David Endre 2/23/99 To view basic files online */
void do_show_file(struct char_data *ch, char *argument) {
    FILE *file;
    int cur_line = 0, num_lines = 0, req_lines = 0, i;
    char filebuf[50000];
    const struct file_struct {
        char *name;
        char level;
        char *path;
    } fields[] = {{"bug", LVL_GOD, "../lib/misc/bugs"},      {"typo", LVL_GOD, "../lib/misc/typos"},
                  {"ideas", LVL_GOD, "../lib/misc/ideas"},   {"xnames", LVL_GOD, "../lib/misc/xnames"},
                  {"levels", LVL_GOD, "../log/levels"},      {"rip", LVL_GOD, "../log/rip"},
                  {"players", LVL_GOD, "../log/newplayers"}, {"rentgone", LVL_GOD, "../log/rentgone"},
                  {"godcmds", LVL_GOD, "../log/godcmds"},    {"syslog", LVL_GOD, "../syslog"},
                  {"crash", LVL_GOD, "../syslog.CRASH"},     {NULL, 0, "\n"}};

    argument = any_one_arg(argument, arg);

    if (!*arg) {
        strcpy(buf, "Usage: show file <name> <num lines>\r\n\r\nFile options:\r\n");
        for (i = 0; fields[i].level; ++i)
            if (fields[i].level <= GET_LEVEL(ch))
                sprintf(buf, "%s%-15s%s\r\n", buf, fields[i].name, fields[i].path);
        send_to_char(buf, ch);
        return;
    }

    for (i = 0; fields[i].name; ++i)
        if (!strncmp(arg, fields[i].name, strlen(arg)))
            break;

    if (!fields[i].name) {
        send_to_char("That is not a valid option!\r\n", ch);
        return;
    }
    if (GET_LEVEL(ch) < fields[i].level) {
        send_to_char("You are not godly enough to view that file!\r\n", ch);
        return;
    }

    if (!*argument)
        req_lines = 15; /* default is the last 15 lines */
    else
        req_lines = atoi(argument);

    /* open the requested file */
    if (!(file = fopen(fields[i].path, "r"))) {
        sprintf(buf, "SYSERR: Error opening file %s using 'file' command.", fields[i].path);
        mudlog(buf, BRF, LVL_IMPL, TRUE);
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
        mudlog(buf2, BRF, LVL_IMPL, TRUE);
        return;
    }

    str_start(filebuf, sizeof(filebuf));

    /* And print the requested lines */
    get_line(file, buf);
    for (cur_line = 0; !feof(file); get_line(file, buf)) {
        ++cur_line;
        if (cur_line > num_lines - req_lines)
            str_catf(filebuf, "%s\r\n", buf);
    }

    fclose(file);

    page_string(ch, filebuf);
}

bool will_npcs_cast(int spell) {
    extern const struct spell_pair mob_sorcerer_buffs[];
    extern const int mob_sorcerer_offensives[];
    extern const int mob_sorcerer_area_spells[];
    extern const struct spell_pair mob_cleric_buffs[];
    extern const struct spell_pair mob_cleric_hindrances[];
    extern const int mob_cleric_offensives[];
    extern const int mob_cleric_area_spells[];
    extern const int mob_cleric_heals[];
    extern const struct spell_pair mob_bard_buffs[];
    extern const struct spell_pair mob_bard_hindrances[];
    extern const int mob_bard_offensives[];
    extern const int mob_bard_area_spells[];
    extern const int mob_bard_heals[];

    const int *npc_spell_lists[] = {mob_sorcerer_offensives, mob_sorcerer_area_spells, mob_cleric_offensives,
                                    mob_cleric_area_spells,  mob_cleric_heals,         mob_bard_offensives,
                                    mob_bard_area_spells,    mob_bard_heals,           NULL};
    const struct spell_pair *npc_pair_lists[] = {mob_sorcerer_buffs, mob_cleric_buffs, mob_cleric_hindrances, mob_bard_buffs, mob_bard_hindrances, NULL};
    int i, j;
    const int *list;
    const struct spell_pair *pair;

    for (i = 0; npc_spell_lists[i]; ++i)
        for (list = npc_spell_lists[i], j = 0; list[j]; ++j)
            if (list[j] == spell)
                return TRUE;

    for (i = 0; npc_pair_lists[i]; ++i)
        for (pair = npc_pair_lists[i], j = 0; pair[j].spell; ++j)
            if (pair[j].spell == spell || pair[j].remover == spell)
                return TRUE;

    return FALSE;
}

static struct skilldef *skill;
static int skill_class_comparator(int a, int b) {
    if (skill->min_level[a] > skill->min_level[b])
        return 1;
    return (skill->min_level[a] < skill->min_level[b] ? -1 : 0);
}

void do_show_spell(struct char_data *ch, int spellnum) {
    int i;
    struct skilldef *spell;
    bool anytargets = FALSE, anyroutines = FALSE, anyassignments = FALSE;

    if (!IS_SPELL(spellnum)) {
        send_to_char("There is no such spell.\r\n", ch);
        return;
    }

    spell = &skills[spellnum];

    /* Number and name */
    strcpy(buf2, spell->name);
    sprintf(buf, "&2Spell #%d&0, &5&b%s&0\r\n", spellnum, CAP(buf2));
    send_to_char(buf, ch);

    /* Stance */
    cprintf(ch, "Min pos     : %s\r\n", position_types[spell->minpos]);
    cprintf(ch, "Fighting?   : %s\r\n", spell->fighting_ok ? "&1Yes&0" : "No");

    /* Targets */
    sprintf(buf, "Targets     :");
    for (i = 0; i < NUM_TAR_FLAGS; i++)
        if (spell->targets & (1 << i)) {
            strcat(buf, " ");
            strcat(buf, targets[i]);
            anytargets = TRUE;
        }
    if (!anytargets)
        strcat(buf, " -none-");
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    /* Violent */
    /* Damage type */
    if (spell->violent) {
        send_to_char("Violent     : &1&bYes&0\r\n", ch);
        if (VALID_DAMTYPE(spell->damage_type))
            sprintf(buf, "Damtype     : %s%s&0\r\n", damtypes[spell->damage_type].color,
                    damtypes[spell->damage_type].name);
        else
            sprintf(buf, "Damtype     : &1&bINVALID (%d)&0\r\n", spell->damage_type);
        send_to_char(buf, ch);
    } else {
        send_to_char("Violent     : &6No&0\r\n", ch);
        send_to_char("Damtype     : -na-\r\n", ch);
    }

    /* Sphere */
    if (IS_SPHERE_SKILL(spell->sphere)) {
        sprintf(buf, "Sphere      : %s\r\n", skills[spell->sphere].name);
        send_to_char(buf, ch);
    } else {
        sprintf(buf, "Sphere      : &1&bINVALID (%d)&0\r\n", spell->sphere);
        send_to_char(buf, ch);
    }

    /* Routines */
    sprintf(buf, "Routines    :");
    for (i = 0; i < NUM_ROUTINE_TYPES; i++)
        if (spell->routines & (1 << i)) {
            strcat(buf, " ");
            strcat(buf, routines[i]);
            anyroutines = TRUE;
        }
    if (!anyroutines)
        strcat(buf, " -none-");
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    /* Mana */
    sprintf(buf, "Mana        : max %d  min %d  chg %d\r\n", spell->mana_max, spell->mana_min, spell->mana_change);
    send_to_char(buf, ch);

    /* Mem time */
    sprintf(buf, "Mem time    : %d\r\n", spell->mem_time);
    send_to_char(buf, ch);

    /* Cast time */
    sprintf(buf, "Cast time   : %d\r\n", spell->cast_time);
    send_to_char(buf, ch);

    /* Pages */
    sprintf(buf, "Pages       : &3%d&0\r\n", spell->pages);
    send_to_char(buf, ch);

    /* Quest */
    sprintf(buf, "Quest       : %s\r\n", spell->quest ? "&2&bYes&0" : "&4&bNo&0");
    send_to_char(buf, ch);

    /* Wearoff message */
    if (spell->wearoff && *(spell->wearoff)) {
        sprintf(buf, "Wearoff     : &6%s&0\r\n", spell->wearoff);
        send_to_char(buf, ch);
    } else
        send_to_char("Wearoff     : -none-\r\n", ch);

    /* Assignments */
    for (i = 0; i < NUM_CLASSES; i++)
        if (spell->min_level[i] > 0 && spell->min_level[i] < LVL_IMMORT) {
            sprintf(buf, "%s%-*s  circle %d\r\n", anyassignments ? "              " : "Assignments : ",
                    13 + count_color_chars(classes[i].displayname), classes[i].displayname,
                    level_to_circle(spell->min_level[i]));
            anyassignments = TRUE;
            send_to_char(buf, ch);
        }
    if (!anyassignments)
        send_to_char("Assignments : -none-\r\n", ch);
}

void do_show_skill(struct char_data *ch, char *argument) {
    int skill_num, type, i, j;
    int skill_classes[NUM_CLASSES];

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char("Usage: show skill <skill name>\r\n", ch);
        return;
    }

    if ((skill_num = find_talent_num(argument, 0)) < 0) {
        send_to_char("Unrecognized skill name.\r\n", ch);
        return;
    }

    skill = &skills[skill_num];

    type = talent_type(skill_num);
    if (type == SPELL) {
        do_show_spell(ch, skill_num);
        return;
    }

    sprintbit(skill->targets, targets, buf2);

    str_start(buf, sizeof(buf));

    str_catf(buf,
             "Skill             : @y%s@0 (@g%d@0)\r\n"
             "Type              : @c%s@0\r\n"
             "Target Flags      : @c%s@0\r\n",
             skill->name, skill_num, talent_types[type], buf2);

    if (type == SKILL)
        str_catf(buf, "Humanoid only?    : @c%s@0\r\n", YESNO(skill->humanoid));
    else {
        sprintbit(skill->routines, routines, buf1);
        if (VALID_DAMTYPE(skill->damage_type))
            sprintf(buf2, "%s%s", damtypes[skill->damage_type].color, damtypes[skill->damage_type].name);
        else
            sprintf(buf2, "@RINVALID (%d)", skill->damage_type);
        str_catf(buf,
                 "Minimum position  : @c%s@0\r\n"
                 "When fighting?    : @c%s@0\r\n"
                 "Aggressive?       : @c%s@0\r\n"
                 "Routines          : @c%s@0\r\n"
                 "Damage Type       : %s@0\r\n"
                 "Quest only?       : @c%s@0\r\n"
                 "Wear-off Message  : %s@0\r\n",
                 position_types[skill->minpos], YESNO(skill->fighting_ok), YESNO(skill->violent), buf1, buf2,
                 YESNO(skill->quest), skill->wearoff ? skill->wearoff : "@cNone.");
    }

    if (type == SPELL) {
        if (IS_SPHERE_SKILL(skill->sphere))
            str_catf(buf, "Sphere            : @c%s@0\r\n", skills[skill->sphere].name);
        else
            str_catf(buf, "Sphere            : @RINVALID %d@0\r\n", skill->sphere);
        str_catf(buf,
                 "Mana              : @cmin %d, max %d, chg %d&0\r\n"
                 "Mem Time          : @c%d@0\r\n"
                 "Cast Time         : @c%d@0\r\n"
                 "Pages             : @c%d@0\r\n",
                 skill->mana_min, skill->mana_max, skill->mana_change, skill->mem_time, skill->cast_time, skill->pages);
    }

    sprintf(buf2, "(%s %d is lowest)", type == SPELL ? "circle" : "level",
            type == SPELL ? level_to_circle(skill->lowest_level) : skill->lowest_level);
    str_catf(buf, "Assignments       : @c%s@0\r\n", skill->lowest_level < LVL_IMMORT ? buf2 : "None.");

    if (skill->lowest_level < LVL_IMMORT) {
        for (i = 0; i < NUM_CLASSES; ++i)
            skill_classes[i] = i;

        sort(insertsort, skill_classes, NUM_CLASSES, skill_class_comparator);

        for (i = 0; skill->min_level[skill_classes[i]] < LVL_IMMORT && i < NUM_CLASSES; ++i) {
            j = skill_classes[i];
            str_catf(buf, "   %-*s: %s%d\r\n", 15 + count_color_chars(classes[j].displayname), classes[j].displayname,
                     type == SPELL ? "circle " : "level ",
                     type == SPELL ? level_to_circle(skill->min_level[j]) : skill->min_level[j]);
        }
    }

    send_to_char(buf, ch);
}

void do_show_date_names(struct char_data *ch, char *argument) {
    int i;

    pprintf(ch, "Day names:\r\n");
    for (i = 0; i < DAYS_PER_WEEK; ++i)
        pprintf(ch, "  %d - %s\r\n", i + 1, weekdays[i]);
    pprintf(ch, "\r\nMonth names:\r\n");
    for (i = 0; i < MONTHS_PER_YEAR; ++i)
        pprintf(ch, "  %d - %s\r\n", i + 1, month_name[i]);

    start_paging(ch);
}

void do_show_liquids(struct char_data *ch, char *argument) {
    int i;

    pprintf(ch,
            "&uLiquid           &0  &uColor             &0  &u+Drunk&0  "
            "&u+Full&0  &u-Thirst&0\r\n");

    for (i = 0; i < NUM_LIQ_TYPES; ++i)
        pprintf(ch, "%-17.17s  %-18.18s  %6d  %5d  %7d\r\n", liquid_types[i].name, liquid_types[i].color_desc,
                liquid_types[i].condition_effects[DRUNK], liquid_types[i].condition_effects[FULL],
                liquid_types[i].condition_effects[THIRST]);

    start_paging(ch);
}

ACMD(do_show) {
    int i;

    extern void show_shops(struct char_data * ch, char *argument);

    const struct show_struct {
        char *name;
        char level;
        void (*command)(struct char_data *ch, char *argument);
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
                  {NULL, 0, NULL}};

    skip_spaces(&argument);

    if (!*argument) {
        str_start(buf, sizeof(buf));
        str_cat(buf, "Show options:\r\n");
        for (i = 0; fields[i].level; ++i)
            if (fields[i].level <= GET_LEVEL(ch))
                str_catf(buf, "%-15s%s", fields[i].name, (i + 1) % 5 ? "" : "\r\n");
        if (i % 5)
            str_cat(buf, "\r\n");
        send_to_char(buf, ch);
        return;
    }

    argument = any_one_arg(argument, arg);

    for (i = 0; fields[i].name; ++i)
        if (fields[i].level <= GET_LEVEL(ch))
            if (!strncmp(arg, fields[i].name, strlen(arg)))
                break;

    if (GET_LEVEL(ch) < fields[i].level) {
        send_to_char("You are not godly enough for that!\r\n", ch);
        return;
    }

    if (!fields[i].name) {
        send_to_char("That's not something you can show.\r\n", ch);
        return;
    }

    if (!fields[i].command) {
        send_to_char("Error retrieving information.\r\n", ch);
        return;
    }

    (fields[i].command)(ch, argument);
}

void reboot_info(struct char_data *ch) {
    int h, m, s;

    extern int num_hotboots;
    extern time_t *boot_time;

    h = (reboot_pulse - global_pulse) / (3600 * PASSES_PER_SEC);
    m = ((reboot_pulse - global_pulse) % (3600 * PASSES_PER_SEC)) / (60 * PASSES_PER_SEC);
    s = ((reboot_pulse - global_pulse) % (60 * PASSES_PER_SEC)) / PASSES_PER_SEC;
    if (reboot_auto)
        cprintf(ch, "Reboot in %02d:%02d:%02d.\r\n", h, m, s);
    else
        cprintf(ch, "Automatic rebooting is &1off&0; would reboot in %02d:%02d:%02d.\r\n", h, m, s);

    if (num_hotboots > 0) {
        cprintf(ch, "%d hotboot%s since last shutdown.  Hotboot history:\r\n", num_hotboots,
                num_hotboots == 1 ? "" : "s");
        for (s = 0; s < num_hotboots; ++s) {
            strcpy(buf, ctime(&boot_time[s + 1]));
            buf[strlen(buf) - 1] = '\0';
            cprintf(ch, "  %s\r\n", buf);
        }
    }
}

ACMD(do_world) {
    struct stat statbuf;

    extern time_t *boot_time;
    extern int make_count;
    extern ACMD(do_date);

    do_date(ch, NULL, 0, SCMD_DATE);
    stat("../bin/fiery", &statbuf);
    strcpy(buf, ctime(&statbuf.st_mtime));
    buf[strlen(buf) - 1] = '\0'; /* cut off newline */
    cprintf(ch, "Build: %d  Compiled: %s\r\n", make_count, buf);
    do_date(ch, NULL, 0, SCMD_UPTIME);
    cprintf(ch, "There are %5d rooms in %3d zones online.\r\n", top_of_world + 1, top_of_zone_table + 1);

    if (GET_LEVEL(ch) >= LVL_REBOOT_VIEW)
        reboot_info(ch);
}

bool get_infodump_filename(const char *name, char *filename) {
    if (!name || !*name)
        return FALSE;
    sprintf(filename, "%s/%s.txt", INFODUMP_PREFIX, name);
    return TRUE;
}

void infodump_spellassign(struct char_data *ch, char *argument) {
    char fname[MAX_INPUT_LENGTH];
    FILE *fl;
    int class_num, skill;
    bool startedclass;

    if (!get_infodump_filename("spellassign", fname)) {
        send_to_char("ERROR: Could not get the output filename.\r\n", ch);
        return;
    }

    if (!(fl = fopen(fname, "w"))) {
        cprintf(ch, "ERROR: Could not write file %s.\r\n", fname);
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
        startedclass = FALSE;
        for (skill = 1; skill <= MAX_SPELLS; skill++) {
            if (skills[skill].min_level[class_num] < LVL_IMMORT) {
                if (!startedclass) {
                    fprintf(fl, "beginclass %s\n", classes[class_num].name);
                    startedclass = TRUE;
                }
                fprintf(fl, "%s,%d,%s\n", skills[skill].name, level_to_circle(skills[skill].min_level[class_num]),
                        skills[skill].quest ? "TRUE" : "FALSE");
            }
        }
        if (startedclass)
            fprintf(fl, "endclass\n\n");
    }
    fclose(fl);
    sprintf(buf, "Dumped spell assignments to %s\r\n", fname);
    send_to_char(buf, ch);
}

/* The purpose of info dumps is to output game data in a format that will
 * be easily parsable by other programs. */

ACMD(do_infodump) {
    int i;
    const struct infodump_struct {
        char *name;
        void (*command)(struct char_data *ch, char *argument);
    } fields[] = {/* Keep this list alphabetized */
                  {"spellassign", infodump_spellassign},
                  {NULL, NULL}};
    skip_spaces(&argument);

    if (!*argument) {
        str_start(buf, sizeof(buf));
        str_cat(buf, "Infodump options:\r\n");
        for (i = 0; fields[i].name; ++i)
            str_catf(buf, "%-15s%s", fields[i].name, (i + 1) % 5 ? "" : "\r\n");
        if (i % 5)
            str_cat(buf, "\r\n");
        send_to_char(buf, ch);
        return;
    }

    argument = any_one_arg(argument, arg);

    for (i = 0; fields[i].name; ++i)
        if (!strncmp(arg, fields[i].name, strlen(arg)))
            break;

    if (!fields[i].name) {
        send_to_char("That's not something you can infodump.\r\n", ch);
        return;
    }

    if (!fields[i].command) {
        send_to_char("Error identifying what you wanted to dump.\r\n", ch);
        return;
    }

    (fields[i].command)(ch, argument);
}
