/***************************************************************************
 * $Id: act.informative.c,v 1.339 2010/06/30 21:22:20 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: act.informative.c                              Part of FieryMUD *
 *  Usage: Player-level commands of an informative nature                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "act.h"
#include "board.h"
#include "casting.h"
#include "charsize.h"
#include "comm.h"
#include "commands.h"
#include "composition.h"
#include "conf.h"
#include "constants.h"
#include "cooldowns.h"
#include "damage.h"
#include "db.h"
#include "dg_scripts.h"
#include "directions.h"
#include "events.h"
#include "exits.h"
#include "handler.h"
#include "interpreter.h"
#include "lifeforce.h"
#include "math.h"
#include "modify.h"
#include "players.h"
#include "races.h"
#include "retain_comms.h"
#include "rooms.h"
#include "screen.h"
#include "skills.h"
#include "spell_mem.h"
#include "spheres.h"
#include "structs.h"
#include "sysdep.h"
#include "textfiles.h"
#include "trophy.h"
#include "utils.h"
#include "vsearch.h"
#include "weather.h"

/* extern variables */
extern int pk_allowed;
extern struct spell_dam spell_dam_info[MAX_SPELLS + 1];

/* global */
int boot_high = 0;

void print_spells_in_book(struct char_data *ch, struct obj_data *obj, char *dest_buf);
void look_at_target(struct char_data *ch, char *arg);
int ideal_mountlevel(struct char_data *ch);
int ideal_tamelevel(struct char_data *ch);
int mountlevel(struct char_data *ch);
void speech_report(struct char_data *ch, struct char_data *tch);
static void do_farsee(struct char_data *ch, int dir);

bool senses_living(struct char_data *ch, struct char_data *vict, int basepct) {
    /* You cannot sense anyone if you're unconscious, or they have wizinvis on. */
    if (!EFF_FLAGGED(ch, EFF_SENSE_LIFE) || !AWAKE(ch) || (GET_INVIS_LEV(vict) > GET_LEVEL(ch)))
        return FALSE;
    /* Being busy with fighting or casting reduces your sensitivity. */
    if (FIGHTING(ch) || CASTING(ch))
        basepct = (67 * basepct) / 100;
    /* "Life" is defined as that life force which is susceptible to healing magic,
     * and life is what this ability detects. Here we adjust the sensitivity
     * accordingly. */
    basepct = (susceptibility(vict, DAM_HEAL) * basepct) / 100;
    return number(1, 100) < basepct;
}

/* This function will also determine whether you sense a creature, but it will
 * return false if you could see it by any other means: if it's visible, or
 * you can see it in infrared. */
bool senses_living_only(struct char_data *ch, struct char_data *vict, int basepct) {
    /* CAN_SEE takes care of darkness, invisibility, and wizinvis.
     * CAN_SEE_BY_INFRA takes care of infravision. */
    if (CAN_SEE(ch, vict) || CAN_SEE_BY_INFRA(ch, vict))
        return FALSE;
    return senses_living(ch, vict, basepct);
}

const char *relative_location_str(int bits) {
    const char *obj_relative[] = {"(carried)", "(here)", "(used)", "(somewhere)"};

    switch (bits) {
    case FIND_OBJ_INV:
        return obj_relative[0];
    case FIND_OBJ_ROOM:
        return obj_relative[1];
    case FIND_OBJ_EQUIP:
        return obj_relative[2];
    default:
        return obj_relative[3];
    }
}

/* Component function of print_obj_to_char */
static void print_note_to_char(struct obj_data *obj, struct char_data *ch) {
    if (obj->ex_description)
        cprintf(ch, "%s", obj->ex_description->description);
    if (obj->action_description)
        cprintf(ch, "%s%s", obj->ex_description ? "There is something written upon it:\r\n\r\n" : "",
                obj->action_description);
    else
        cprintf(ch, "It's blank.\r\n");
}

/* Component function of print_obj_to_char */
static void print_obj_vnum_to_char(struct obj_data *obj, struct char_data *ch) {
    cprintf(ch, " @W[@B%d@W]@0", GET_OBJ_VNUM(obj));
}

/* Component function of print_obj_to_char */
static void print_obj_flags_to_char(struct obj_data *obj, struct char_data *ch) {
    /* Small local buffer.   Increase the size when adding additional flags */
    char buf[300];

    str_start(buf, sizeof(buf));

    if (obj->in_room != NOWHERE && IS_WATER(obj->in_room))
        str_cat(buf, " &0(@Bfloating&0)");
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, VAL_LIGHT_LIT))
        str_cat(buf, " @y(&billuminated@y)&0");
    if (OBJ_FLAGGED(obj, ITEM_INVISIBLE))
        str_cat(buf, " (invisible)");
    if (GET_OBJ_HIDDENNESS(obj) > 0) {
        if (GET_LEVEL(ch) >= LVL_IMMORT)
            str_catf(buf, " (h%ld)", GET_OBJ_HIDDENNESS(obj));
        else
            str_cat(buf, " (hidden)");
    }
    if (OBJ_FLAGGED(obj, ITEM_MAGIC) && EFF_FLAGGED(ch, EFF_DETECT_MAGIC))
        str_cat(buf, " (&4&bmagic&0)");
    if (OBJ_FLAGGED(obj, ITEM_GLOW))
        str_cat(buf, " @L(@mglowing@L)&0");
    if (OBJ_FLAGGED(obj, ITEM_HUM))
        str_cat(buf, " @r(@chumming@r)&0");
    if (EFF_FLAGGED(ch, EFF_DETECT_POISON) && IS_POISONED(obj))
        str_cat(buf, " (@Mpoisoned@0)");
    if (EFF_FLAGGED(ch, EFF_DETECT_ALIGN) && OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL)) {
        if (OBJ_FLAGGED(obj, ITEM_ANTI_GOOD) && !OBJ_FLAGGED(obj, ITEM_ANTI_EVIL))
            str_cat(buf, " (@rRed Aura@0)");
        if (OBJ_FLAGGED(obj, ITEM_ANTI_EVIL) && !OBJ_FLAGGED(obj, ITEM_ANTI_GOOD))
            str_cat(buf, " (@YGold Aura@0)");
    }
    if (OBJ_FLAGGED(obj, ITEM_NOFALL))
        str_cat(buf, " &6&b(&0&5hovering&6&b)&0");

    /* Should be safe; shouldn't be any % symbols from above */
    cprintf(ch, buf);
}

/* Component function of print_obj_to_char */
static void print_obj_auras_to_char(struct obj_data *obj, struct char_data *ch) {
    if (OBJ_EFF_FLAGGED(obj, EFF_HEX))
        cprintf(ch, "It is imbued with a &9&bdark aura&0.\r\n");
    if (OBJ_EFF_FLAGGED(obj, EFF_BLESS))
        cprintf(ch, "It is surrounded by a &6blessed aura&0.\r\n");
}

/* Show one of an obj's desc to a char; uses SHOW_ flags */
void print_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode, char *additional_args) {
    bool show_flags = IS_SET(mode, SHOW_FLAGS);

    /* Remove the flags from the show mode */
    REMOVE_BIT(mode, SHOW_MASK);

    switch (mode) {
    case SHOW_SHORT_DESC:
        if (obj->short_description)
            cprintf(ch, "%s", obj->short_description);
        if (show_flags && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
            print_obj_vnum_to_char(obj, ch);
        break;

    case SHOW_LONG_DESC:
        if (obj->description)
            cprintf(ch, "%s", obj->description);
        if (show_flags && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
            print_obj_vnum_to_char(obj, ch);
        break;

    case SHOW_BASIC_DESC:
    case SHOW_FULL_DESC:
        if (obj->ex_description && !(GET_OBJ_TYPE(obj) == ITEM_BOARD && is_number(additional_args)))
            cprintf(ch, "%s", obj->ex_description->description);
        if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
            cprintf(ch, "%s", "It looks like a drink container.");
        else if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
            print_spells_in_book(ch, obj, NULL);
        else if (GET_OBJ_TYPE(obj) == ITEM_NOTE)
            print_note_to_char(obj, ch);
        else if (GET_OBJ_TYPE(obj) == ITEM_BOARD) {
            if (is_number(additional_args))
                read_message(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), atoi(additional_args));
            else
                look_at_board(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), obj);
        } else if (!obj->ex_description)
            cprintf(ch, "You see nothing special about %s.", obj->short_description);
        print_obj_auras_to_char(obj, ch);
        break;

    default:
        log("SYSERR: invalid SHOW_ mode %d passed to print_obj_for_char", mode);
        return;
    }

    if (show_flags)
        print_obj_flags_to_char(obj, ch);

    cprintf(ch, "\r\n");
}

/* Print a list of objs to a char; uses SHOW_ flags */
void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode) {
    struct obj_data *i, *j, *display;
    bool found = FALSE;
    int num, raw_mode = mode;

    /* Remove the flags from the show mode.   The flags can still be
     * accessed from raw_mode. */
    REMOVE_BIT(mode, SHOW_MASK);

#define STRINGS_MATCH(x, y, member) ((x)->member == (y)->member || !strcmp((x)->member, (y)->member))
#define OBJECTS_MATCH(x, y)                                                                                            \
    ((x)->item_number == (y)->item_number &&                                                                           \
     (mode == SHOW_LONG_DESC ? STRINGS_MATCH((x), (y), description) : STRINGS_MATCH((x), (y), short_description)))

    /* Loop through the list of objects */
    for (i = list; i; i = i->next_content) {
        /* Set to 0 in case this object can't be seen */
        num = 0;

        if (IS_SET(raw_mode, SHOW_STACK) && !PRF_FLAGGED(ch, PRF_EXPAND_OBJS)) {
            /* Check the list to see if we've already counted this object */
            for (j = list; j != i; j = j->next_content)
                if (OBJECTS_MATCH(i, j))
                    break; /* found a matching object */
            if (i != j)
                continue; /* we counted object i earlier in the list */

            /* Count matching objects, including this one */
            for (display = j = i; j; j = j->next_content)
                if (OBJECTS_MATCH(i, j))
                    if (CAN_SEE_OBJ(ch, j)) {
                        ++num;
                        /* If the original item can't be seen, switch it for this one */
                        if (display == i && !CAN_SEE_OBJ(ch, display))
                            display = j;
                    }
        } else if (CAN_SEE_OBJ(ch, i)) {
            display = i;
            num = 1;
        }

        /* This object or one like it can be seen by the char, so show it */
        if (num > 0) {
            if (num != 1)
                cprintf(ch, "[%d] ", num);
            print_obj_to_char(display, ch, raw_mode, NULL);
            found = TRUE;
        }
    }
    if (!found && !IS_SET(raw_mode, SHOW_NO_FAIL_MSG))
        cprintf(ch, "  Nothing.\r\n");

#undef STRINGS_MATCH
#undef CHARS_MATCH
}

/* Component function for print_char_to_char */
static void print_char_vnum_to_char(struct char_data *targ, struct char_data *ch) {
    if (IS_NPC(targ))
        cprintf(ch, "@W[@B%5d@W]@0 ", GET_MOB_VNUM(targ));
}

/* Component function for print_char_to_char */
static void print_char_position_to_char(struct char_data *targ, struct char_data *ch) {
    if (GET_POS(targ) < 0 || GET_POS(targ) >= NUM_POSITIONS || GET_STANCE(targ) < 0 ||
        GET_STANCE(targ) >= NUM_STANCES) {
        cprintf(ch, " is here in a very unnatural pose.");
        return;
    }

    if (IS_WATER(IN_ROOM(targ))) {
        switch (GET_POS(targ)) {
        case POS_SITTING:
            cprintf(ch, " floating here.");
            break;
        case POS_STANDING:
            cprintf(ch, " floating here in the water.");
            break;
        case POS_FLYING:
            cprintf(ch, " hovering here above the water.");
            break;
        default:
            cprintf(ch, " floating here, %s.", stance_types[(int)GET_STANCE(targ)]);
        }
        return;
    }

    cprintf(ch, " is %s here%s%s.",
            GET_POS(targ) == POS_PRONE    ? "lying"
            : GET_POS(targ) == POS_FLYING ? "floating"
                                          : position_types[(int)GET_POS(targ)],
            GET_STANCE(targ) == STANCE_ALERT      ? ""
            : GET_STANCE(targ) == STANCE_SLEEPING ? " "
                                                  : ", ",
            GET_STANCE(targ) == STANCE_ALERT                                     ? ""
            : GET_STANCE(targ) == STANCE_RESTING && GET_POS(targ) > POS_KNEELING ? "at ease"
                                                                                 : stance_types[(int)GET_STANCE(targ)]);
}

/* Component function for print_char_to_char */
static void print_char_long_desc_to_char(struct char_data *targ, struct char_data *ch) {
    char buf[513];

    if (IS_NPC(targ) && !MOB_FLAGGED(targ, MOB_PLAYER_PHANTASM) && GET_LDESC(targ) &&
        GET_POS(targ) == GET_DEFAULT_POS(targ) && GET_STANCE(targ) != STANCE_FIGHTING &&
        !EFF_FLAGGED(targ, EFF_MINOR_PARALYSIS) && !EFF_FLAGGED(targ, EFF_MAJOR_PARALYSIS)) {
        /* Copy to buffer, and cut off te newline. */
        strcpy(buf, GET_LDESC(targ));
        buf[MAX(strlen(GET_LDESC(targ)) - 2, 0)] = '\0';
        cprintf(ch, "%s", buf);
        return;
    }

    if (IS_NPC(targ) && !MOB_FLAGGED(targ, MOB_PLAYER_PHANTASM))
        snprintf(buf, sizeof(buf), "%s", GET_SHORT(targ));
    else
        snprintf(buf, sizeof(buf), "@0%s%s%s @W(@0%s@W)@0 @L(@0%s@L)@0", GET_NAME(targ), GET_TITLE(targ) ? " " : "",
                 GET_TITLE(targ) ? GET_TITLE(targ) : "", RACE_ABBR(targ), SIZE_DESC(targ));

    cprintf(ch, "%s", CAP(buf));

    if (RIDDEN_BY(targ) && RIDDEN_BY(targ)->in_room == targ->in_room && !IS_NPC(targ))
        cprintf(ch, " is here, ridden by %s.", RIDING(targ) == ch ? "you" : PERS(RIDDEN_BY(targ), ch));

    else if (RIDING(targ) && RIDING(targ)->in_room == targ->in_room)
        cprintf(ch, " is here, mounted upon %s.", RIDING(targ) == ch ? "you" : PERS(RIDING(targ), ch));

    else if (EFF_FLAGGED(targ, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(targ, EFF_MAJOR_PARALYSIS)) {
        /* The spell of entangle can set major or minor paralysis. */
        if (affected_by_spell(targ, SPELL_ENTANGLE))
            cprintf(ch, " is here, %s.",
                    EFF_FLAGGED(targ, EFF_MAJOR_PARALYSIS) ? "held still by thick vines"
                                                           : "entangled in a mass of vines");
        else
            cprintf(ch, " is standing here, completely motionless.");
    }

    else if (EFF_FLAGGED(targ, EFF_MESMERIZED))
        cprintf(ch, " is here, gazing carefully at a point in front of %s nose nose.", HSHR(targ));

    else if (GET_STANCE(targ) != STANCE_FIGHTING)
        print_char_position_to_char(targ, ch);

    else if (FIGHTING(targ))
        cprintf(ch, " is here, fighting %s!",
                FIGHTING(targ) == ch                       ? "YOU!"
                : targ->in_room == FIGHTING(targ)->in_room ? PERS(FIGHTING(targ), ch)
                                                           : "someone who has already left");

    else /* NULL fighting pointer */
        cprintf(ch, " is here struggling with thin air.");
}

/* Component function for print_char_to_char */
static void print_char_flags_to_char(struct char_data *targ, struct char_data *ch) {
    /* Small local buf.   Make bigger if more flags are added */
    char buf[514];

    str_start(buf, sizeof(buf));

    if (PRF_FLAGGED(ch, PRF_HOLYLIGHT) && MOB_FLAGGED(targ, MOB_ILLUSORY))
        str_cat(buf, " [@millusory@0]");

    if (EFF_FLAGGED(targ, EFF_INVISIBLE))
        str_cat(buf, " (@Winvisible@0)");

    if (IS_HIDDEN(targ)) {
        if (PRF_FLAGGED(ch, PRF_HOLYLIGHT))
            str_catf(buf, " (@Wh%ld@0)", GET_HIDDENNESS(targ));
        else
            str_catf(buf, " (@Whiding@0)");
    }

    if (!IS_NPC(targ) && !targ->desc && PRF_FLAGGED(ch, PRF_HOLYLIGHT))
        str_catf(buf, " (@r%s%s@0)", targ->forward ? "switched: @0" : "clueless",
                 targ->forward ? GET_SHORT(targ->forward) : "");

    if (PLR_FLAGGED(targ, PLR_WRITING))
        str_cat(buf, " (@cwriting@0)");
    else if (targ->desc && EDITING(targ->desc))
        str_cat(buf, " (@cwriting@0)");

    if (PRF_FLAGGED(targ, PRF_AFK) && GET_LEVEL(REAL_CHAR(targ)) < LVL_IMMORT)
        str_cat(buf, " [@RAFK@0]");

    if (EFF_FLAGGED(ch, EFF_DETECT_POISON) && EFF_FLAGGED(targ, EFF_POISON))
        str_cat(buf, " (@Mpoisoned&0)");

    if (EFF_FLAGGED(ch, EFF_DETECT_MAGIC) && GET_LIFEFORCE(targ) == LIFE_MAGIC)
        str_cat(buf, " (@Bmagic@0)");

    if (EFF_FLAGGED(ch, EFF_DETECT_ALIGN)) {
        if (IS_EVIL(targ))
            str_cat(buf, " (@RRed Aura@0)");
        else if (IS_GOOD(targ))
            str_cat(buf, " (@YGold Aura@0)");
    }

    if (EFF_FLAGGED(targ, EFF_ON_FIRE))
        str_cat(buf, " (@Rburning@0)");

    if (CASTING(targ))
        str_catf(buf, " (@mCasting%s%s@0)", PRF_FLAGGED(ch, PRF_HOLYLIGHT) ? " " : "",
                 PRF_FLAGGED(ch, PRF_HOLYLIGHT) ? skill_name(targ->casting.spell) : "");

    cprintf(ch, "%s", buf);
}

const char *status_string(int cur, int max, int mode) {
    struct {
        char *pre;
        char *color;
        char *cond;
        char *post;
    } css;
    int percent;
    static char retbuf[64][3];
    static int lastbuf = -1;

#define STRINGS(a, b, c, d)                                                                                            \
    do {                                                                                                               \
        css.pre = a;                                                                                                   \
        css.color = b;                                                                                                 \
        css.cond = c;                                                                                                  \
        css.post = d;                                                                                                  \
    } while (0)

    /* Choose which static buffer to use. */
    if (++lastbuf >= 3)
        lastbuf = 0;

    if (max > 0)
        percent = (100 * cur) / max;
    else
        percent = -1;

    if (percent >= 100)
        STRINGS("is in", AFGRN, "excellent", "condition");
    else if (percent >= 88)
        STRINGS("has a few", AFYEL, "scratches", "");
    else if (percent >= 75)
        STRINGS("has some", AHYEL, "small wounds", "and bruises");
    else if (percent >= 50)
        STRINGS("has quite a", AHMAG, "few wounds", "");
    else if (percent >= 30)
        STRINGS("has some big", AFMAG, "nasty wounds", "and scratches");
    else if (percent >= 15)
        STRINGS("is", AHRED, "pretty hurt", "");
    else if (percent >= 0)
        STRINGS("is in", AFRED, "awful condition", "");
    else
        STRINGS("is", AFRED, "bleeding awfully", "from large wounds");

    switch (mode) {
    case STATUS_COLOR:
        return css.color;
    case STATUS_ALIAS:
        sprintf(retbuf[lastbuf], "%s%s" ANRM, css.color, css.cond);
        break;
    case STATUS_PHRASE:
    default:
        sprintf(retbuf[lastbuf], "%s %s%s%s%s" ANRM ".", css.pre, css.color, css.cond, *css.post ? " " : "", css.post);
        break;
    }

    return retbuf[lastbuf];

#undef STRINGS
}

/* Component function for print_char_to_char */
static void print_char_appearance_to_char(struct char_data *targ, struct char_data *ch) {
    char buf[16];

    strncpy(buf, HSSH(targ), sizeof(buf));
    buf[15] = '\0';

    /* Size and composition */
    if (GET_COMPOSITION(targ) == COMP_FLESH)
        cprintf(ch, "%s is %s%s&0 in size.\r\n", CAP(buf), SIZE_COLOR(targ), SIZE_DESC(targ));
    else if (GET_COMPOSITION(targ) == COMP_ETHER)
        cprintf(ch, "%s is %s%s&0 in size, and is %sinsubstantial&0.\r\n", CAP(buf), SIZE_COLOR(targ), SIZE_DESC(targ),
                COMPOSITION_COLOR(targ));
    else
        cprintf(ch, "%s is %s%s&0 in size, and is composed of %s%s&0.\r\n", CAP(buf), SIZE_COLOR(targ), SIZE_DESC(targ),
                COMPOSITION_COLOR(targ), COMPOSITION_MASS(targ));
}

/* Component function for print_char_to_char */
static void print_char_riding_status_to_char(struct char_data *targ, struct char_data *ch) {
    char buf[257];

    if (RIDING(targ) && RIDING(targ)->in_room == targ->in_room) {
        if (RIDING(targ) == ch)
            act("$e is riding on you.", FALSE, targ, 0, ch, TO_VICT);
        else {
            sprintf(buf, "$e is riding on %s.", PERS(RIDING(targ), ch));
            act(buf, FALSE, targ, 0, ch, TO_VICT);
        }
    } else if (RIDDEN_BY(targ) && RIDDEN_BY(targ)->in_room == targ->in_room) {
        if (RIDDEN_BY(targ) == ch)
            act("You are mounted upon $m.", FALSE, targ, 0, ch, TO_VICT);
        else {
            sprintf(buf, "%s is mounted upon $m.", PERS(RIDDEN_BY(targ), ch));
            act(buf, FALSE, targ, 0, ch, TO_VICT);
        }
    }
}

/* Component function for print_char_to_char */
static void print_char_equipment_to_char(struct char_data *targ, struct char_data *ch) {
    int j;
    bool found = FALSE;
    for (j = 0; !found && j < NUM_WEARS; j++)
        if (GET_EQ(targ, j) && CAN_SEE_OBJ(ch, GET_EQ(targ, j))) {
            found = TRUE;
            break;
        }

    if (found) {
        act("$n is using:", FALSE, targ, 0, ch, TO_VICT);
        for (j = 0; j < NUM_WEARS; j++)
            if (GET_EQ(targ, wear_order_index[j]) && CAN_SEE_OBJ(ch, GET_EQ(targ, wear_order_index[j]))) {
                cprintf(ch, "%s", where[wear_order_index[j]]);
                print_obj_to_char(GET_EQ(targ, wear_order_index[j]), ch, SHOW_SHORT_DESC | SHOW_FLAGS, NULL);
            }
    }
}

/* Component function for print_char_to_char */
static void print_char_spells_to_char(struct char_data *targ, struct char_data *ch) {
    /* Things you can only see with detect magic */
    if (EFF_FLAGGED(ch, EFF_DETECT_MAGIC) || PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
        if (affected_by_spell(targ, SPELL_ARMOR))
            act("A &7&btranslucent shimmering aura&0 surrounds $M.&0", TRUE, ch, 0, targ, TO_CHAR);
        if (affected_by_spell(targ, SPELL_BLESS))
            act("The &8shimmering telltales&0 of a &3&bmagical blessing&0 flutter "
                "about $S head.&0",
                TRUE, ch, 0, targ, TO_CHAR);
        if (affected_by_spell(targ, SPELL_DEMONIC_ASPECT))
            act("A &1demonic tinge&0 circulates in $S &1blood&0.", TRUE, ch, 0, targ, TO_CHAR);
        if (affected_by_spell(targ, SPELL_DEMONIC_MUTATION))
            act("Two &1large &8red&0&1 horns&0 sprout from $S head.", TRUE, ch, 0, targ, TO_CHAR);
        if (affected_by_spell(targ, SPELL_DARK_PRESENCE))
            act("You sense a &9&bdark presence&0 within $M.&0", TRUE, ch, 0, targ, TO_CHAR);
        if (affected_by_spell(targ, SPELL_DRAGONS_HEALTH))
            act("The power of &5dragon's blood&0 fills $M&0!", TRUE, ch, 0, targ, TO_CHAR);
        else if (affected_by_spell(targ, SPELL_LESSER_ENDURANCE) || affected_by_spell(targ, SPELL_ENDURANCE) ||
                 affected_by_spell(targ, SPELL_GREATER_ENDURANCE) || affected_by_spell(targ, SPELL_VITALITY) ||
                 affected_by_spell(targ, SPELL_GREATER_VITALITY))
            act("$S health appears to be bolstered by magical power.", TRUE, ch, 0, targ, TO_CHAR);
        if (EFF_FLAGGED(targ, EFF_RAY_OF_ENFEEB))
            act("A &3malevolent spell&0 is draining $S &1strength&0.", TRUE, ch, 0, targ, TO_CHAR);
        if (affected_by_spell(targ, SPELL_CHILL_TOUCH))
            act("A &6weakening chill&0 circulates in $S&0 veins.", TRUE, ch, 0, targ, TO_CHAR);
    }

    /* AC protection spells */
    if (EFF_FLAGGED(targ, EFF_STONE_SKIN))
        act("&9&b$S body seems to be made of stone!&0", TRUE, ch, 0, targ, TO_CHAR);
    if (affected_by_spell(targ, SPELL_BARKSKIN))
        act("&3$S skin is thick, brown, and wrinkly.&0", TRUE, ch, 0, targ, TO_CHAR);
    if (affected_by_spell(targ, SPELL_BONE_ARMOR))
        act("&7Heavy bony plates cover $S body.&0", TRUE, ch, 0, targ, TO_CHAR);
    if (affected_by_spell(targ, SPELL_DEMONSKIN))
        act("&1$S skin is shiny, smooth, and &bvery red&0&1.&0", TRUE, ch, 0, targ, TO_CHAR);
    if (affected_by_spell(targ, SPELL_GAIAS_CLOAK))
        act("&2A whirlwind of leaves and &3sticks&2 whips around $S body.&0", TRUE, ch, 0, targ, TO_CHAR);
    if (affected_by_spell(targ, SPELL_ICE_ARMOR))
        act("&8A layer of &4solid ice&0&8 covers $M entirely.&0", TRUE, ch, 0, targ, TO_CHAR);
    if (affected_by_spell(targ, SPELL_MIRAGE))
        act("&7$S image &1wavers&7 and &9&bshimmers&0&7 and is somewhat "
            "indistinct.&0",
            TRUE, ch, 0, targ, TO_CHAR);

    /* Miscellaneous spell effects */
    if (EFF_FLAGGED(targ, EFF_BLIND))
        act("$S &0&b&9dull &0eyes suggest $E is blind!&0", TRUE, ch, 0, targ, TO_CHAR);
    if (EFF_FLAGGED(targ, EFF_CONFUSION)) {
        if (GET_POS(targ) >= POS_KNEELING) {
            if (GET_STANCE(targ) >= STANCE_RESTING)
                act("$S eyes &5drift&0 and &3dart&0 comically about, and $E has "
                    "trouble balancing.",
                    TRUE, ch, 0, targ, TO_CHAR);
            else
                act("$E seems to have trouble balancing.", TRUE, ch, 0, targ, TO_CHAR);
        } else if (GET_STANCE(targ) >= STANCE_RESTING)
            act("$S eyes &5drift&0 and &3dart&0 comically about.", TRUE, ch, 0, targ, TO_CHAR);
    }
    if (EFF_FLAGGED(targ, EFF_SANCTUARY)) {
        if (IS_EVIL(targ))
            act("&9&b$S body is surrounded by a black aura!&0", TRUE, ch, 0, targ, TO_CHAR);
        else if (IS_GOOD(targ))
            act("&7&b$S body is surrounded by a white aura!&0", TRUE, ch, 0, targ, TO_CHAR);
        else
            act("&4&b$S body is surrounded by a blue aura!&0", TRUE, ch, 0, targ, TO_CHAR);
    }
    if (EFF_FLAGGED(targ, EFF_SOULSHIELD)) {
        if (IS_EVIL(targ))
            act("&1&b$S body is surrounded by cleansing flames!&0", TRUE, ch, 0, targ, TO_CHAR);
        else
            act("&7&b$S body is surrounded by cleansing flames!&0", TRUE, ch, 0, targ, TO_CHAR);
    }
    if (EFF_FLAGGED(targ, EFF_FIRESHIELD))
        act("&1&b$S body is encased in fire!&0", TRUE, ch, 0, targ, TO_CHAR);
    if (EFF_FLAGGED(targ, EFF_COLDSHIELD))
        act("&4&b$S body is encased in jagged ice!&0", TRUE, ch, 0, targ, TO_CHAR);
    if (EFF_FLAGGED(targ, EFF_MINOR_GLOBE) && EFF_FLAGGED(ch, EFF_DETECT_MAGIC))
        act("&1$S body is encased in a shimmering globe!&0", TRUE, ch, 0, targ, TO_CHAR);
    if (EFF_FLAGGED(targ, EFF_MAJOR_GLOBE))
        act("&1&b$S body is encased in shimmering globe of force!&0", TRUE, ch, 0, targ, TO_CHAR);
    if (EFF_FLAGGED(targ, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(targ, EFF_MAJOR_PARALYSIS)) {
        /* The spell of entangle can set major or minor paralysis. */
        if (affected_by_spell(targ, SPELL_ENTANGLE)) {
            if (EFF_FLAGGED(targ, EFF_MAJOR_PARALYSIS))
                act("&2$E is held fast by &bthick twining vines&0&2.&0", TRUE, ch, 0, targ, TO_CHAR);
            else
                act("&2$E is entwined by a tangled mass of vines.&0", TRUE, ch, 0, targ, TO_CHAR);
        } else
            act("&6$E is completely still, and shows no awareness of $S "
                "surroundings.&0",
                TRUE, ch, 0, targ, TO_CHAR);
    } else if (EFF_FLAGGED(targ, EFF_MESMERIZED))
        act("$E gazes carefully at a point in the air directly in front of $S "
            "nose,\r\n"
            "as if deliberating upon a puzzle or problem.",
            TRUE, ch, 0, targ, TO_CHAR);
    if (affected_by_spell(targ, SPELL_WINGS_OF_HELL))
        act("&1&bHuge leathery &9bat-like&1 wings sprout from $S back.&0", TRUE, ch, 0, targ, TO_CHAR);
    if (affected_by_spell(targ, SPELL_WINGS_OF_HEAVEN))
        act("&7&b$E has a pair of beautiful bright white wings.&0", TRUE, ch, 0, targ, TO_CHAR);
    if (affected_by_spell(targ, SPELL_MAGIC_TORCH))
        act("$E is being followed by a &1bright glowing light&0.", TRUE, ch, 0, targ, TO_CHAR);

    /* Other miscellaneous effects */
    if (EFF_FLAGGED(ch, EFF_DETECT_POISON) && EFF_FLAGGED(targ, EFF_POISON))
        act("You sense @Mpoison@0 coursing through $S veins.", TRUE, ch, 0, targ, TO_CHAR);
    if (EFF_FLAGGED(targ, EFF_ON_FIRE))
        act("&1&b$E is on FIRE!&0", TRUE, ch, 0, targ, TO_CHAR);
    if (affected_by_spell(targ, SPELL_CIRCLE_OF_LIGHT))
        act("&7&bA circle of light floats over $S head.&0", TRUE, ch, 0, targ, TO_CHAR);
    if (PRF_FLAGGED(targ, PRF_HOLYLIGHT) && !IS_NPC(targ))
        act("$S entire body &0&bglows&0&6 faintly!&0", TRUE, ch, 0, targ, TO_CHAR);
}

/* Show a character to another character; uses SHOW_ flags */
void print_char_to_char(struct char_data *targ, struct char_data *ch, int mode) {
    bool show_flags = IS_SET(mode, SHOW_FLAGS);

    /* Remove the flags from the show mode. */
    REMOVE_BIT(mode, SHOW_MASK);

    switch (mode) {
    case SHOW_SHORT_DESC:
        if (show_flags && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
            print_char_vnum_to_char(targ, ch);
        if (GET_SHORT(targ))
            cprintf(ch, "%s", GET_SHORT(targ));
        if (show_flags)
            print_char_flags_to_char(targ, ch);
        cprintf(ch, "\r\n");
        break;

    case SHOW_LONG_DESC:
        if (show_flags && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
            print_char_vnum_to_char(targ, ch);
        if (show_flags && PRF_FLAGGED(ch, PRF_HOLYLIGHT) && POSSESSED(targ))
            cprintf(ch, "@C[@0%s@C]@0 ", GET_NAME(POSSESSOR(targ)));
        print_char_long_desc_to_char(targ, ch);
        if (show_flags)
            print_char_flags_to_char(targ, ch);
        cprintf(ch, "\r\n");
        break;

    case SHOW_FULL_DESC:
        if (targ->player.description)
            cprintf(ch, "%s", targ->player.description);
        else
            cprintf(ch, "You see nothing special about %s.\r\n", HMHR(targ));
        /* Fall through */
    case SHOW_BASIC_DESC:
        strcpy(buf, GET_NAME(targ));
        cprintf(ch, "%s %s\r\n", CAP(buf), status_string(GET_HIT(targ), GET_MAX_HIT(targ), STATUS_PHRASE));
        print_char_appearance_to_char(targ, ch);
        print_char_spells_to_char(targ, ch);
        if (mode == SHOW_FULL_DESC) {
            print_char_riding_status_to_char(targ, ch);
            print_char_equipment_to_char(targ, ch);
            if (targ != ch && (GET_CLASS(ch) == CLASS_THIEF || PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
                cprintf(ch, "\r\nYou attempt to peek at %s inventory:\r\n", HSHR(targ));
                list_obj_to_char(targ->carrying, ch, SHOW_SHORT_DESC | SHOW_FLAGS | SHOW_STACK);
            }
        }
    }
}

/* Component function of list_char_to_char */
static void print_life_sensed_msg(struct char_data *ch, int num_sensed) {
#define SENSELIFECLR "&7&b"

    if (num_sensed == 0)
        return;
    if (num_sensed == 1) {
        send_to_char(SENSELIFECLR "You sense a hidden lifeform.&0\r\n", ch);
    } else if (num_sensed < 4) {
        send_to_char(SENSELIFECLR "You sense a few hidden lifeforms.&0\r\n", ch);
    } else if (num_sensed < 11) {
        send_to_char(SENSELIFECLR "You sense several hidden lifeforms.&0\r\n", ch);
    } else if (num_sensed < 53) {
        send_to_char(SENSELIFECLR "You sense many hidden lifeforms.&0\r\n", ch);
    } else {
        send_to_char(SENSELIFECLR "You sense a great horde of hidden lifeforms!&0\r\n", ch);
    }

#undef SENSELIFECLR
}

/* Component function of list_char_to_char */
static void print_char_infra_to_char(struct char_data *targ, struct char_data *ch, int mode) {
    /* Remove the flags from the show mode. */
    REMOVE_BIT(mode, SHOW_MASK);

    if (mode != SHOW_LONG_DESC)
        return;

    if (RIDING(targ) && RIDING(targ)->in_room == targ->in_room)
        cprintf(ch, "@rThe red shape of a %s being is here, mounted upon %s%s%s.@0\r\n", SIZE_DESC(targ),
                RIDING(targ) == ch ? "you.@0" : "a ", RIDING(targ) == ch ? "" : SIZE_DESC(RIDING(targ)),
                RIDING(targ) == ch ? "" : "-sized creature");
    else
        cprintf(ch, "@rThe red shape of a %c%s&0&1 living being%s is here.&0\r\n", LOWER(*SIZE_DESC(targ)),
                SIZE_DESC(targ) + 1, EFF_FLAGGED(targ, EFF_INFRAVISION) ? " &bwith glowing red eyes&0&1" : "");
}

/* Prints a list of chars to another char; uses SHOW_ flags */
void list_char_to_char(struct char_data *list, struct char_data *ch, int mode) {
    struct char_data *i, *j, *display;
    int num_seen;
    int num_sensed = 0; /* How many characters will you detect only via sense life? */
    int raw_mode = mode;

    /* Remove the flags from the show mode.   The flags can still be
     * accessed in raw_mode */
    REMOVE_BIT(mode, SHOW_MASK);

#define STRINGS_MATCH(x, y, member)                                                                                    \
    ((x)->player.member == (y)->player.member || !strcmp((x)->player.member, (y)->player.member))
#define CHARS_MATCH(x, y)                                                                                              \
    (GET_MOB_VNUM(x) == GET_MOB_VNUM(y) && FIGHTING(x) == FIGHTING(y) && CASTING(x) == CASTING(y) &&                   \
     (mode == SHOW_SHORT_DESC ? STRINGS_MATCH((x), (y), short_descr) : STRINGS_MATCH((x), (y), long_descr)) &&         \
     ((CAN_SEE(ch, (x)) && CAN_SEE(ch, (y))) || (CAN_SEE_BY_INFRA(ch, (x)) && CAN_SEE_BY_INFRA(ch, (y)))))

    for (i = list; i; i = i->next_in_room) {
        if (ch == i && IS_SET(raw_mode, SHOW_SKIP_SELF))
            continue;

        /* The value 100 is for the acuity of the viewer's life-sensing.
         * Since it's a flag, we just assume everyone with the flag is
         * very good at it. But if it becomes a more varied value in the
         * future, that value should be used instead of 100. */
        if (senses_living_only(ch, i, 100)) {
            ++num_sensed;
            continue;
        }

        if (RIDDEN_BY(i) && RIDDEN_BY(i)->in_room == i->in_room && IS_NPC(i) && CAN_SEE(ch, RIDDEN_BY(i)))
            continue;

        /* Set to 0 in case this char can't be seen */
        num_seen = 0;

        /* Only try to stack if it's a mob */
        if (IS_NPC(i) && IS_SET(raw_mode, SHOW_STACK) && !PRF_FLAGGED(ch, PRF_EXPAND_MOBS)) {
            /* Check the list to see if we've already counted this char */
            for (j = list; j != i; j = j->next_in_room)
                if (CHARS_MATCH(i, j))
                    break; /* found a matching char   */
            if (i != j)
                continue; /* we counted char j earlier in the list */

            /* Count matching chars, including this one */
            for (display = j = i; j; j = j->next_in_room)
                if (CHARS_MATCH(i, j)) {
                    ++num_seen;
                    /* If the original char can't be seen, switch it for this one */
                    if (display == i && !CAN_SEE(ch, display))
                        display = j;
                }
        } else if (CAN_SEE(ch, i) || CAN_SEE_BY_INFRA(ch, i)) {
            display = i;
            num_seen = 1;
        }

        /* This char or one like it can be seen by the char, so show it */
        if (num_seen > 0) {
            if (num_seen != 1)
                cprintf(ch, "[%d] ", num_seen);
            if (CAN_SEE(ch, display))
                print_char_to_char(display, ch, raw_mode);
            else /* CAN_SEE_BY_INFRA would return TRUE here */
                print_char_infra_to_char(display, ch, raw_mode);
        }
    }

    if (num_sensed > 0)
        print_life_sensed_msg(ch, num_sensed);

#undef STRINGS_MATCH
#undef CHARS_MATCH
}

ACMD(do_trophy) {
    struct char_data *tch;

    one_argument(argument, arg);

    if (GET_LEVEL(ch) >= LVL_IMMORT && *arg) {
        if (!(tch = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
            cprintf(ch, "There's nobody here by that name.\r\n");
            return;
        }
    } else {
        tch = ch;
    }

    show_trophy(ch, tch);
}

ACMD(do_viewdam) {

    char *nrm = CLR(ch, ANRM);
    char *grn = CLR(ch, FGRN);
    char *red = CLR(ch, FRED);
    char *bld = CLR(ch, ABLD);
    int i;
    bool use = FALSE;

    one_argument(argument, arg);
    if (!*arg)
        use = TRUE;

    sprintf(buf2, "&0&4Fiery Spells, List of Damages&0\r\n");
    sprintf(buf2,
            "%s%sSPELL                         ID %sNPC%s%s- DI FA RF   "
            "%sPC%s%s- DI FA %sB%s%s-LM   MAX   ON/OFF %sINT%s%s- ON/OFF%s\r\n",
            buf2, red, bld, nrm, red, bld, nrm, red, bld, nrm, red, bld, nrm, red, nrm);
    if (is_number(arg)) {
        if (is_number(arg)) {
            i = atoi(arg);
            if ((i < MAX_SKILLS) && (i > 0)) {
                if (str_cmp(skills[i].name, "!UNUSED!")) {
                    cprintf(ch, "%s%s%-22s%-8d%-3d%-3d%-8d%-3d%-5d%-4d%-5d%-12s%s%s \r\n", buf2, grn, (skills[i].name),
                            SD_SPELL(i), SD_NPC_NO_DICE(i), SD_NPC_NO_FACE(i), SD_NPC_REDUCE_FACTOR(i),
                            SD_PC_NO_DICE(i), SD_PC_NO_FACE(i), SD_LVL_MULT(i), SD_BONUS(i),
                            (SD_USE_BONUS(i) ? "TRUE" : "FALSE"), (SD_INTERN_DAM(i) ? "TRUE" : "FALSE"), nrm);
                    return;
                }
            }
        }

    } else if (!str_cmp("all", arg)) {
        for (i = 1; i <= MAX_SPELLS; i++) {
            if (str_cmp(skills[i].name, "!UNUSED!")) {
                cprintf(ch, "%s%-22s%-8d%-3d%-3d%-8d%-3d%-5d%-4d%-3d%s%s\r\n", grn, skills[i].name, SD_SPELL(i),
                        SD_NPC_NO_DICE(i), SD_NPC_NO_FACE(i), SD_NPC_REDUCE_FACTOR(i), SD_PC_NO_DICE(i),
                        SD_PC_NO_FACE(i), SD_LVL_MULT(i), SD_BONUS(i), (SD_USE_BONUS(i) ? "TRUE" : "FALSE"), nrm);
            }
        }
        return;
    }
    if (use) {
        for (i = 1; i <= MAX_SPELLS; i++) {
            if ((SD_NPC_NO_DICE(i) == 0) && (SD_NPC_NO_FACE(i) == 0) && (SD_PC_NO_DICE(i) == 0))
                continue;
            if (!(SD_INTERN_DAM(i)))
                continue;
            sprintf(buf2, "%s%s%-22s%-8d%-3d%-3d%-8d%-3d%-5d%-4d%-3d%s%s\r\n", buf2, grn, (skills[i].name), SD_SPELL(i),
                    SD_NPC_NO_DICE(i), SD_NPC_NO_FACE(i), SD_NPC_REDUCE_FACTOR(i), SD_PC_NO_DICE(i), SD_PC_NO_FACE(i),
                    SD_LVL_MULT(i), SD_BONUS(i), (SD_USE_BONUS(i) ? "TRUE" : "FALSE"), nrm);
        }
        page_string(ch, buf2);
        return;
    }

    cprintf(ch,
            "USAGE: viewdam - Those active\r\n            viewdam all - view "
            "ALL\r\n            viewdam x - view spell x\r\n");
    return;
}

/* search_for_doors() - returns TRUE if a door was located. */
static int search_for_doors(struct char_data *ch, char *arg) {
    int door;

    for (door = 0; door < NUM_OF_DIRS; ++door)
        if (CH_EXIT(ch, door) && CH_EXIT(ch, door)->to_room != NOWHERE &&
            IS_SET(CH_EXIT(ch, door)->exit_info, EX_HIDDEN)) {
            if (GET_LEVEL(ch) >= LVL_IMMORT ||
                (CH_EXIT(ch, door)->keyword && arg && isname(arg, CH_EXIT(ch, door)->keyword)) ||
                GET_INT(ch) > number(0, 200)) {
                sprintf(buf, "&8You have found%s hidden %s %s.&0",
                        CH_EXIT(ch, door)->keyword && isplural(CH_EXIT(ch, door)->keyword) ? "" : " a",
                        CH_EXIT(ch, door)->keyword ? "$F" : "door", dirpreposition[door]);
                act(buf, FALSE, ch, 0, CH_EXIT(ch, door)->keyword, TO_CHAR);
                sprintf(buf, "$n has found%s hidden %s %s.",
                        CH_EXIT(ch, door)->keyword && isplural(CH_EXIT(ch, door)->keyword) ? "" : " a",
                        CH_EXIT(ch, door)->keyword ? "$F" : "door", dirpreposition[door]);
                act(buf, FALSE, ch, 0, CH_EXIT(ch, door)->keyword, TO_ROOM);
                REMOVE_BIT(CH_EXIT(ch, door)->exit_info, EX_HIDDEN);
                send_gmcp_room(ch);
                return TRUE;
            }
        }

    return FALSE;
}

ACMD(do_search) {
    long orig_hide;
    bool found_something = FALSE, target = FALSE;
    struct char_data *j;
    struct obj_data *k;

    /*
     * If for some reason you can't see, then you can't search.
     */
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (EFF_FLAGGED(ch, EFF_BLIND)) {
            cprintf(ch, "You're blind and can't see a thing!\r\n");
            return;
        }
        if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_FOG)) {
            cprintf(ch, "The fog is too thick to find anything!\r\n");
            return;
        }
        if (FIGHTING(ch)) {
            cprintf(ch, "You will be searching for your head if you keep this up!\r\n");
            return;
        }
        if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS) && !EFF_FLAGGED(ch, EFF_ULTRAVISION)) {
            /* Automatic failure. */
            cprintf(ch, "You don't find anything you didn't see before.\r\n");
            WAIT_STATE(ch, PULSE_VIOLENCE / 2);
            return;
        }
    }

    one_argument(argument, arg);

    if (!*arg)
        k = world[ch->in_room].contents;
    else {
        generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &j, &k);
        if (k) {
            if (GET_OBJ_TYPE(k) == ITEM_CONTAINER) {
                if (IS_SET(GET_OBJ_VAL(k, VAL_CONTAINER_BITS), CONT_CLOSED)) {
                    cprintf(ch, "Opening it would probably improve your chances.\r\n");
                    return;
                } else {
                    k = k->contains;
                    target = TRUE;
                }
            } else {
                /* Maybe they meant to search for a door by that name? */
                found_something = search_for_doors(ch, arg);
                if (!found_something) {
                    cprintf(ch, "There's no way to search that.\r\n");
                    return;
                }
            }
        }
    }

    for (; k && (!found_something || GET_LEVEL(ch) >= LVL_IMMORT); k = k->next_content)
        if ((orig_hide = GET_OBJ_HIDDENNESS(k)) > 0) {
            GET_OBJ_HIDDENNESS(k) = 0;
            if (!CAN_SEE_OBJ(ch, k)) {
                GET_OBJ_HIDDENNESS(k) = orig_hide;
                continue;
            }
            GET_OBJ_HIDDENNESS(k) = MAX(0, orig_hide - number(GET_PERCEPTION(ch) / 2, GET_PERCEPTION(ch)));
            if (GET_OBJ_HIDDENNESS(k) <= GET_PERCEPTION(ch) || GET_LEVEL(ch) >= LVL_IMMORT) {
                GET_OBJ_HIDDENNESS(k) = 0;
                if (!OBJ_FLAGGED(k, ITEM_WAS_DISARMED))
                    k->last_to_hold = NULL;
                if (orig_hide <= GET_PERCEPTION(ch) && !GET_OBJ_HIDDENNESS(k)) {
                    act("You reveal $p!", FALSE, ch, k, 0, TO_CHAR);
                    act("$n reveals $p!", TRUE, ch, k, 0, TO_ROOM);
                } else {
                    act("You find $p!", FALSE, ch, k, 0, TO_CHAR);
                    act("$n finds $p!", TRUE, ch, k, 0, TO_ROOM);
                }
                found_something = TRUE;
            }
        }

    if (!target && !found_something) {
        found_something = search_for_doors(ch, arg);

        if (!found_something)
            for (j = world[ch->in_room].people; j && (!found_something || GET_LEVEL(ch) >= LVL_IMMORT);
                 j = j->next_in_room)
                if (IS_HIDDEN(j) && j != ch && !IS_IN_GROUP(ch, j)) {
                    /* Check whether the searcher could see this character if it weren't
                     * hidden. */
                    orig_hide = GET_HIDDENNESS(j);
                    GET_HIDDENNESS(j) = 0;
                    if (!CAN_SEE(ch, j)) {
                        GET_HIDDENNESS(j) = orig_hide;
                        continue;
                    }
                    /* The searcher COULD see this character if it weren't hidden. Will
                     * the searcher discover it? */
                    GET_HIDDENNESS(j) = MAX(0, orig_hide - number(GET_PERCEPTION(ch) / 2, GET_PERCEPTION(ch)));
                    if (GET_HIDDENNESS(j) <= GET_PERCEPTION(ch) || GET_LEVEL(ch) >= LVL_IMMORT) {
                        GET_HIDDENNESS(j) = 0;
                        if (orig_hide <= GET_PERCEPTION(ch) && !IS_HIDDEN(j))
                            act("You point out $N lurking here!", FALSE, ch, 0, j, TO_CHAR);
                        else
                            act("You find $N lurking here!", FALSE, ch, 0, j, TO_CHAR);
                        act("$n points out $N lurking here!", TRUE, ch, 0, j, TO_NOTVICT);
                        if (GET_PERCEPTION(j) + number(0, 200) > GET_PERCEPTION(ch))
                            act("You think $n has spotted you!", TRUE, ch, 0, j, TO_VICT);
                        found_something = TRUE;
                    }
                }
    }

    if (!found_something)
        cprintf(ch, "You don't find anything you didn't see before.\r\n");
    WAIT_STATE(ch, PULSE_VIOLENCE / 2);
}

ACMD(do_exits) {
    if (EFF_FLAGGED(ch, EFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT)
        cprintf(ch, YOU_ARE_BLIND);
    else if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_FOG) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
        cprintf(ch, "Your view is obscured by thick fog.\r\n");
    else
        send_full_exits(ch, ch->in_room);
}

/* Shows a room to a char */
void print_room_to_char(room_num room_nr, struct char_data *ch, bool ignore_brief) {
    if (IS_DARK(room_nr) && !CAN_SEE_IN_DARK(ch)) {
        /* The dark version... you can't see much, but you might see exits to
         * nearby, lighted rooms. You might see creatures by infravision. */
        cprintf(ch, "&9&bIt is too dark to see.&0\r\n");
        if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
            send_auto_exits(ch, room_nr);
        if (EFF_FLAGGED(ch, EFF_INFRAVISION))
            list_char_to_char(world[room_nr].people, ch, SHOW_LONG_DESC | SHOW_FLAGS | SHOW_SKIP_SELF | SHOW_STACK);
        return;
    }

    /* The lighted version */
    if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
        sprintflag(buf, ROOM_FLAGS(room_nr), NUM_ROOM_FLAGS, room_bits);
        sprintf(buf1, "%s%s", sectors[SECT(room_nr)].color, sectors[SECT(room_nr)].name);
        cprintf(ch, "@L[@0%5d@L]@W %s @L[@0%s@0: %s@L]@0\r\n", world[room_nr].vnum, world[room_nr].name, buf1, buf);
    } else
        cprintf(ch, "%s%s%s\r\n", CLR(ch, FCYN), world[room_nr].name, CLR(ch, ANRM));

    if (!PRF_FLAGGED(ch, PRF_BRIEF) || ignore_brief || ROOM_FLAGGED(room_nr, ROOM_DEATH))
        cprintf(ch, "%s", world[room_nr].description);

    if (ROOM_EFF_FLAGGED(room_nr, ROOM_EFF_ILLUMINATION))
        cprintf(ch, "&3A soft &7glow&0&3 suffuses the area with &blight&0&3.&0\r\n");
    if (ROOM_EFF_FLAGGED(room_nr, ROOM_EFF_FOREST))
        cprintf(ch, "&2&bThick foliage appears to have overgrown the whole area.&0\r\n");
    if (ROOM_EFF_FLAGGED(room_nr, ROOM_EFF_CIRCLE_FIRE))
        cprintf(ch, "&1&8A circle of fire burns wildly here, surrounding the area.&0\r\n");

    /* autoexits */
    if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
        send_auto_exits(ch, room_nr);

    /* now list characters & objects */
    list_obj_to_char(world[room_nr].contents, ch, SHOW_LONG_DESC | SHOW_FLAGS | SHOW_STACK | SHOW_NO_FAIL_MSG);
    list_char_to_char(world[room_nr].people, ch, SHOW_LONG_DESC | SHOW_FLAGS | SHOW_SKIP_SELF | SHOW_STACK);
}

void look_at_room(struct char_data *ch, int ignore_brief) {
    if (EFF_FLAGGED(ch, EFF_BLIND))
        cprintf(ch, "You see nothing but infinite darkness...\r\n");
    else if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_FOG) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
        cprintf(ch, "Your view is obscured by thick fog.\r\n");
    else
        print_room_to_char(ch->in_room, ch, ignore_brief);

    send_gmcp_room(ch);
}

/* Checking out your new surroundings when magically transported.
 * You could have been asleep, stunned, or anything.  You did not
 * type look.
 *
 * summon
 * teleport (spell)
 * word of recall
 * recall scroll
 * teleport (command)
 * dimension door
 * relocate
 */

void check_new_surroundings(struct char_data *ch, bool old_room_was_dark, bool tx_obvious) {

    if (GET_STANCE(ch) < STANCE_SLEEPING)
        /* You know nothing. */
        return;
    else if (SLEEPING(ch))
        cprintf(ch, "The earth seems to shift slightly beneath you.\r\n");
    else if (EFF_FLAGGED(ch, EFF_BLIND))
        cprintf(ch, "The ground seems to shift a bit beneath your feet.\r\n");
    else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
        if (old_room_was_dark) {
            if (tx_obvious) {
                cprintf(ch, "Darkness still surrounds you.\r\n");
            } else {
                cprintf(ch, "The ground seems to shift a bit beneath your feet.\r\n");
            }
        } else {
            cprintf(ch, "Suddenly everything goes dark!\r\n");
        }
        if (EFF_FLAGGED(ch, EFF_INFRAVISION)) {
            list_char_to_char(world[ch->in_room].people, ch, SHOW_LONG_DESC | SHOW_FLAGS | SHOW_SKIP_SELF | SHOW_STACK);
            cprintf(ch, "%s", CLR(ch, ANRM));
        }
    } else {
        look_at_room(ch, 0);
    }
}

void look_in_direction(struct char_data *ch, int dir) {
    bool look_at_magic_wall(struct char_data * ch, int dir, bool sees_next_room);
    struct exit *exit = CH_EXIT(ch, dir);

    if (ROOM_EFF_FLAGGED(CH_NROOM(ch), ROOM_EFF_ISOLATION) && GET_LEVEL(ch) < LVL_IMMORT) {
        cprintf(ch, "%s",
                exit && EXIT_IS_DESCRIPTION(exit) ? exit->general_description : "You see nothing special.\r\n");
        return;
    }

    if (exit) {
        if (ROOM_EFF_FLAGGED(CH_NROOM(ch), ROOM_EFF_ISOLATION))
            cprintf(ch, "You peer beyond a veil of &5isolation&0...\r\n");
        if (exit->general_description)
            cprintf(ch, "%s", exit->general_description);
        else
            cprintf(ch, "You see nothing special.\r\n");

        /* If the exit is hidden, we don't want to display any info! */
        if (EXIT_IS_HIDDEN(exit) && GET_LEVEL(ch) < LVL_IMMORT) {
            look_at_magic_wall(ch, dir, FALSE);
        } else if (EXIT_IS_CLOSED(exit)) {
            cprintf(ch, "The %s %s closed.\r\n", exit_name(exit), isplural(exit_name(exit)) ? "are" : "is");
            look_at_magic_wall(ch, dir, FALSE);
        } else {
            if (EXIT_IS_DOOR(exit))
                cprintf(ch, "The %s %s open.\r\n", exit_name(exit), isplural(exit_name(exit)) ? "are" : "is");
            if (EFF_FLAGGED(ch, EFF_FARSEE) || GET_CLASS(ch) == CLASS_RANGER)
                do_farsee(ch, dir);
            else if (exit->to_room != NOWHERE && ROOM_EFF_FLAGGED(exit->to_room, ROOM_EFF_CIRCLE_FIRE))
                cprintf(ch,
                        "&1&8The edge of a circle of fire burns wildly in this "
                        "direction.&0\r\n");
            look_at_magic_wall(ch, dir, TRUE);
        }
    } else if (!look_at_magic_wall(ch, dir, FALSE))
        cprintf(ch, "You see nothing special.\r\n");
}

/* Component function for look_in_obj */
static const char *describe_fullness(int remaining, int capacity) {
    const char *fullness;
    switch ((remaining * 3) / capacity) {
    case 0:
        fullness = "less than half ";
        break;
    case 1:
        fullness = "about half ";
        break;
    case 2:
        fullness = "more than half ";
        break;
    default:
        fullness = "";
    }
    return fullness;
}

void look_in_obj(struct char_data *ch, char *arg) {
    struct obj_data *obj = NULL;
    struct char_data *dummy = NULL;
    int bits;

    if (!*arg)
        cprintf(ch, "Look in what?\r\n");
    else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj)))
        cprintf(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    else if (IS_VIEWABLE_GATE(obj))
        look_at_target(ch, arg);
    else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
        if (IS_SET(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_CLOSED))
            cprintf(ch, "It is closed.\r\n");
        else {
            cprintf(ch, "You look in %s %s:\r\n", obj->short_description, relative_location_str(bits));
            list_obj_to_char(obj->contains, ch, SHOW_SHORT_DESC | SHOW_FLAGS | SHOW_STACK);
        }
    } else if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON || GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) {
        if (GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING) <= 0)
            cprintf(ch, "It is empty.\r\n");
        else if (GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING) <= 0 ||
                 GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING) > GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY))
            cprintf(ch, "Its contents seem somewhat murky.\r\n"); /* BUG */
        else
            cprintf(
                ch, "It's %sfull of a %s liquid.\r\n",
                describe_fullness(GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING), GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY)),
                LIQ_COLOR(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)));
    } else
        cprintf(ch, "There's nothing inside that!\r\n");
}

static char *find_exdesc(char *word, struct extra_descr_data *list) {
    struct extra_descr_data *i;

    for (i = list; i; i = i->next)
        if (isname(word, i->keyword))
            return (i->description);

    return NULL;
}

static bool consider_obj_exdesc(struct obj_data *obj, char *arg, struct char_data *ch, char *additional_args) {
    char *desc;
    if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
        if (desc == obj->ex_description->description)
            /* First extra desc: show object normally */
            print_obj_to_char(obj, ch, SHOW_FULL_DESC, additional_args);
        else
            /* For subsequent extra descs, suppress special object output */
            page_string(ch, desc);
    } else if (!isname(arg, obj->name))
        return FALSE;
    else if (GET_OBJ_TYPE(obj) == ITEM_NOTE)
        print_obj_to_char(obj, ch, SHOW_FULL_DESC, NULL);
    else if (GET_OBJ_TYPE(obj) == ITEM_BOARD) {
        if (is_number(additional_args))
            read_message(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), atoi(additional_args));
        else
            look_at_board(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), obj);
    } else
        act("You see nothing special about $p.", FALSE, ch, obj, 0, TO_CHAR);
    return TRUE;
}

/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.   First, see if there is another char in the room
 * with the name.   Then check local objs for exdescs.
 */
void look_at_target(struct char_data *ch, char *argument) {
    int bits, j;
    struct char_data *found_char = NULL;
    struct obj_data *obj = NULL, *found_obj = NULL;
    char *desc;
    char arg[MAX_INPUT_LENGTH];
    char *number = "";

    if (!*argument) {
        cprintf(ch, "Look at what?\r\n");
        return;
    }

    number = one_argument(argument, arg);

    bits =
        generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &found_char, &found_obj);

    /* Is the target a character? */
    if (found_char) {
        print_char_to_char(found_char, ch, SHOW_FULL_DESC);
        if (ch != found_char) {
            act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
            act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
        }
        return;
    }
    /* Does the argument match an extra desc in the room? */
    if ((desc = find_exdesc(arg, world[ch->in_room].ex_description)) != NULL) {
        page_string(ch, desc);
        return;
    }

    /* Does the argument match an extra desc in the char's equipment? */
    for (j = 0; j < NUM_WEARS; j++)
        if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
            if (consider_obj_exdesc(GET_EQ(ch, j), arg, ch, number))
                return;

    /* Does the argument match an extra desc in the char's inventory? */
    for (obj = ch->carrying; obj; obj = obj->next_content)
        if (CAN_SEE_OBJ(ch, obj))
            if (consider_obj_exdesc(obj, arg, ch, number))
                return;

    /* Does the argument match an extra desc of an object in the room? */
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
        if (CAN_SEE_OBJ(ch, obj))
            if (consider_obj_exdesc(obj, arg, ch, number))
                return;

    /* If an object was found back in generic_find */
    if (bits)
        print_obj_to_char(found_obj, ch, SHOW_FULL_DESC, number);
    else
        cprintf(ch, "You do not see that here.\r\n");
}

#define MAX_FARSEE_DISTANCE 4
static void do_farsee(struct char_data *ch, int dir) {
    int original, distance;
    static const char *farsee_desc[MAX_FARSEE_DISTANCE + 1] = {
        "far beyond reason ", "ridiculously far ", "even farther ", "farther ", "",
    };

    distance = MAX_FARSEE_DISTANCE;
    original = ch->in_room;

    while (CAN_GO(ch, dir)) {
        ch->in_room = CH_EXIT(ch, dir)->to_room;

        if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
            cprintf(ch, "&0&b&9It is too dark to see!&0\r\n");
            break;
        }
        if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_FOG) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
            cprintf(ch, "&0&b&8The fog is too thick to see through!&0\r\n");
            break;
        }
        if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
            cprintf(ch, "&0&7It is pitch black...&0\r\n");
            break;
        }

        cprintf(ch, "\r\n&0&6You extend your vision %s%s%s%s.&0\r\n", farsee_desc[distance],
                (dir == UP || dir == DOWN) ? "" : "to the ", dirs[dir], (dir == UP || dir == DOWN) ? "wards" : "");

        print_room_to_char(ch->in_room, ch, TRUE);

        /* Yes, spell casters will be able to see farther. */
        if (!distance || number(1, 125) > GET_SKILL(ch, SKILL_SPHERE_DIVIN)) {
            cprintf(ch, "You can't see any farther.\r\n");
            break;
        }

        --distance;
    }

    ch->in_room = original;
}

ACMD(do_read) {
    skip_spaces(&argument);

    if (EFF_FLAGGED(ch, EFF_BLIND))
        cprintf(ch, YOU_ARE_BLIND);
    else if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_FOG) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
        cprintf(ch, "Your view is obscured by a thick fog.\r\n");
    else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch))
        cprintf(ch, "It is too dark to read.\r\n");
    else if (!*argument)
        cprintf(ch, "Read what?\r\n");
    else if (is_number(argument)) {
        struct obj_data *obj;
        universal_find(find_vis_by_type(ch, ITEM_BOARD), FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_WORLD, NULL, &obj);
        if (obj)
            read_message(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), atoi(argument));
        else
            look_at_target(ch, argument);
    } else
        look_at_target(ch, argument);
}

ACMD(do_look) {
    int look_type;
    char *orig_arg = argument;

    if (!ch->desc)
        return;

    argument = any_one_arg(argument, arg);
    skip_spaces(&argument);

    if (GET_STANCE(ch) < STANCE_SLEEPING)
        cprintf(ch, "You can't see anything but stars!\r\n");
    else if (GET_STANCE(ch) == STANCE_SLEEPING)
        cprintf(ch, "You seem to be sound asleep.\r\n");
    else if (EFF_FLAGGED(ch, EFF_BLIND))
        cprintf(ch, YOU_ARE_BLIND);
    else if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_FOG) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
        cprintf(ch, "Your view is obscured by a thick fog.\r\n");
    else if (!*arg) {
        /* If the room is dark, look_at_room() will handle it. */
        look_at_room(ch, TRUE);
    } else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
        cprintf(ch, "It is too dark to see anything.\r\n");
    } else if (is_abbrev(arg, "in"))
        look_in_obj(ch, argument);
    else if ((look_type = searchblock(arg, dirs, FALSE)) >= 0)
        look_in_direction(ch, look_type);
    else if (is_abbrev(arg, "at"))
        look_at_target(ch, argument);
    else
        look_at_target(ch, orig_arg);
}

ACMD(do_examine) {
    struct char_data *vict;
    struct obj_data *obj;

    one_argument(argument, arg);

    if (!*arg) {
        cprintf(ch, "Examine what?\r\n");
        return;
    }
    look_at_target(ch, arg);

    generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM | FIND_OBJ_EQUIP, ch, &vict, &obj);

    if (obj)
        if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON || GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN ||
            GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
            look_in_obj(ch, arg);
}

void identify_obj(struct obj_data *obj, struct char_data *ch, int location) {
    int i;
    trig_data *t;

    if (location)
        cprintf(ch,
                "You closely examine %s %s:\r\n"
                "Item type: %s\r\n",
                obj->short_description, relative_location_str(location), OBJ_TYPE_NAME(obj));
    else
        cprintf(ch, "Object '%s', Item type: %s\r\n", obj->short_description, OBJ_TYPE_NAME(obj));

    /* Tell about wearing here */

    /* Describe wielding positions only if it is a weapon */
    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        cprintf(ch, "Item is wielded: %s\r\n",
                CAN_WEAR(obj, ITEM_WEAR_WIELD)     ? "one-handed"
                : CAN_WEAR(obj, ITEM_WEAR_2HWIELD) ? "two-handed"
                                                   : "NONE");

    /* Describe any non-wield wearing positions (and not take) */
    if ((i = obj->obj_flags.wear_flags & (~ITEM_WEAR_WIELD & ~ITEM_WEAR_2HWIELD & ~ITEM_WEAR_TAKE))) {
        sprintbit(i, wear_bits, buf);
        cprintf(ch, "Item is worn: %s\r\n", buf);
    }

    /* Describe extra flags (hum, !drop, class/align restrictions, etc.) */
    sprintflag(buf, GET_OBJ_FLAGS(obj), NUM_ITEM_FLAGS, extra_bits);
    cprintf(ch, "Item is: %s\r\n", buf);

    /* Tell about spell effects here */
    if (HAS_FLAGS(GET_OBJ_EFF_FLAGS(obj), NUM_EFF_FLAGS)) {
        sprintflag(buf, GET_OBJ_EFF_FLAGS(obj), NUM_EFF_FLAGS, effect_flags);
        cprintf(ch, "Item provides: %s\r\n", buf);
    }

    cprintf(ch, "Weight: %.2f, Value: %d, Level: %d\r\n", GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_LEVEL(obj));

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_SCROLL:
    case ITEM_POTION:
        if ((GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1) < 1) && (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_2) < 1) &&
            (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_3) < 1)) {
            cprintf(ch, "Its magical spells are too esoteric to identify.\r\n");
        } else {
            sprintf(buf, "This %s casts: ", OBJ_TYPE_NAME(obj));

            if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1) >= 1)
                sprintf(buf, "%s %s", buf, skills[GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1)].name);
            if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_2) >= 1)
                sprintf(buf, "%s %s", buf, skills[GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_2)].name);
            if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_3) >= 1)
                sprintf(buf, "%s %s", buf, skills[GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_3)].name);
            cprintf(ch, "%s\r\n", buf);
        }
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        cprintf(ch,
                "This %s casts: %s\r\n"
                "It has %d maximum charge%s and %d remaining.\r\n",
                OBJ_TYPE_NAME(obj), skills[GET_OBJ_VAL(obj, VAL_WAND_SPELL)].name,
                GET_OBJ_VAL(obj, VAL_WAND_MAX_CHARGES), GET_OBJ_VAL(obj, VAL_WAND_MAX_CHARGES) == 1 ? "" : "s",
                GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT));
        break;
    case ITEM_WEAPON:
        cprintf(ch,
                "Damage Dice is '%dD%d' "
                "for an average per-round damage of %.1f.\r\n",
                GET_OBJ_VAL(obj, VAL_WEAPON_DICE_NUM), GET_OBJ_VAL(obj, VAL_WEAPON_DICE_SIZE), WEAPON_AVERAGE(obj));
        break;
    case ITEM_ARMOR:
        cprintf(ch, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, VAL_ARMOR_AC));
        break;
    case ITEM_LIGHT:
        if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) == -1)
            cprintf(ch, "Hours left: Infinite\r\n");
        else
            cprintf(ch, "Hours left: %d, Initial hours: %d\r\n", GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING),
                    GET_OBJ_VAL(obj, VAL_LIGHT_CAPACITY));
        break;
    case ITEM_CONTAINER:
        if (!IS_CORPSE(obj))
            cprintf(ch, "Weight capacity: %d, Weight Reduction: %d%%\r\n", GET_OBJ_VAL(obj, VAL_CONTAINER_CAPACITY),
                    GET_OBJ_VAL(obj, VAL_CONTAINER_WEIGHT_REDUCTION));
        break;
    case ITEM_DRINKCON:
        cprintf(ch, "Liquid capacity: %d, Liquid remaining: %d, Liquid: %s\r\n",
                GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY), GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING),
                LIQ_NAME(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)));
        break;
    case ITEM_FOOD:
        cprintf(ch, "Fillingness: %d\r\n", GET_OBJ_VAL(obj, VAL_FOOD_FILLINGNESS));
        break;
    }
    for (i = 0; i < MAX_OBJ_APPLIES; i++)
        if (obj->applies[i].location != APPLY_NONE)
            cprintf(ch, "   Apply: %s\r\n", format_apply(obj->applies[i].location, obj->applies[i].modifier));

    if (SCRIPT(obj)) {
        for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
            cprintf(ch, "   Special: %s\r\n", t->name);
        }
    }
}

ACMD(do_identify) {
    int bits;
    struct char_data *found_char = NULL;
    struct obj_data *obj = NULL;

    if (FIGHTING(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        cprintf(ch, "You're a little busy to be looking over an item right now!\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        cprintf(ch, "Identify what?\r\n");
        return;
    }

    bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &found_char, &obj);

    if (!obj) {
        cprintf(ch, "You can't see that here.\r\n");
        return;
    }

    /* Full identification */
    if (GET_LEVEL(ch) >= LVL_GOD || (GET_CLASS(ch) == CLASS_THIEF && GET_OBJ_LEVEL(obj) <= GET_LEVEL(ch) + 11)) {
        identify_obj(obj, ch, bits);
        return;
    }

    /* Partial identification */
    if (GET_CLASS(ch) == CLASS_THIEF || GET_OBJ_LEVEL(obj) < GET_LEVEL(ch) + 11) {
        strcpy(buf, obj->short_description);
        CAP(buf);
        cprintf(ch, "%s%s%s look%s like %s.\r\n", buf, bits == FIND_OBJ_INV ? "" : " ",
                bits == FIND_OBJ_INV ? "" : relative_location_str(bits), isplural(obj->name) ? "" : "s",
                OBJ_TYPE_DESC(obj));
        return;
    }

    cprintf(ch, "You can't make heads or tails of it.\r\n");
}

ACMD(do_inventory) {
    struct char_data *vict = NULL;

    /* Immortals can target another character to see their inventory */
    one_argument(argument, arg);
    if (*arg && GET_LEVEL(ch) >= LVL_IMMORT) {
        if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
            cprintf(ch, "%s", NOPERSON);
            return;
        }
    }
    if (!vict)
        vict = ch;
    if (vict == ch)
        cprintf(ch, "You are carrying:\r\n");
    else
        act("$N is carrying:", TRUE, ch, 0, vict, TO_CHAR);

    list_obj_to_char(vict->carrying, ch, SHOW_SHORT_DESC | SHOW_FLAGS | SHOW_STACK);
}

ACMD(do_equipment) {
    int i, found = 0;

    cprintf(ch, "You are using:\r\n");
    for (i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(ch, wear_order_index[i])) {
            if (CAN_SEE_OBJ(ch, GET_EQ(ch, wear_order_index[i]))) {
                cprintf(ch, where[wear_order_index[i]]);
                print_obj_to_char(GET_EQ(ch, wear_order_index[i]), ch, SHOW_SHORT_DESC | SHOW_FLAGS, NULL);
                found = TRUE;
            } else {
                cprintf(ch, "%sSomething.\r\n", where[wear_order_index[i]]);
                found = TRUE;
            }
        }
    }
    if (!found)
        cprintf(ch, " Nothing.\r\n");
}

ACMD(do_time) {
    char *suf;
    int weekday, day;

    /* 35 days in a month */
    weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

    cprintf(ch, "It is %d o'clock %s, on %s;\r\n", ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
            ((time_info.hours >= 12) ? "pm" : "am"), weekdays[weekday]);

    day = time_info.day + 1; /* day in [1..35] */

    if (day == 1)
        suf = "st";
    else if (day == 2)
        suf = "nd";
    else if (day == 3)
        suf = "rd";
    else if (day < 20)
        suf = "th";
    else if ((day % 10) == 1)
        suf = "st";
    else if ((day % 10) == 2)
        suf = "nd";
    else if ((day % 10) == 3)
        suf = "rd";
    else
        suf = "th";

    cprintf(ch, "The %d%s Day of the %s, Year %d.\r\n", day, suf, month_name[(int)time_info.month], time_info.year);

    if (time_info.month < 0 || time_info.month > 15)
        log("SYSERR:act.inf:do_time:time_info.month is reporting month %d out of "
            "range!",
            time_info.month);
}

ACMD(do_weather) {
    *buf2 = '\0';

    strcat(buf2, temperature_message(zone_table[IN_ZONE_RNUM(ch)].temperature));
    if (GET_LEVEL(ch) >= LVL_IMMORT)
        /* Cut off the original newline. */
        sprintf(buf2 + strlen(buf2) - 2, " (%d degrees)\r\n", zone_table[IN_ZONE_RNUM(ch)].temperature);

    strcat(buf2, wind_message(zone_table[IN_ZONE_RNUM(ch)].wind_speed, zone_table[IN_ZONE_RNUM(ch)].wind_speed));
    if (GET_LEVEL(ch) >= LVL_IMMORT && zone_table[IN_ZONE_RNUM(ch)].wind_speed != WIND_NONE)
        /* Cut off the original newline. */
        sprintf(buf2 + strlen(buf2) - 2, " (%s)\r\n", dirs[zone_table[IN_ZONE_RNUM(ch)].wind_dir]);

    strcat(buf2, precipitation_message(&zone_table[IN_ZONE_RNUM(ch)], zone_table[IN_ZONE_RNUM(ch)].precipitation));

    cprintf(ch, "%sIt is %s.\r\n", buf2, seasons[hemispheres[IN_HEMISPHERE(ch)].season]);
}

ACMD(do_help) {
    int chk, bot, top, mid, minlen;

    if (!ch->desc)
        return;

    skip_spaces(&argument);

    if (!*argument) {
        page_string(ch, get_text(TEXT_HELP));
        return;
    }
    if (!help_table) {
        cprintf(ch, "No help available.\r\n");
        return;
    }

    bot = 0;
    top = top_of_helpt;
    minlen = strlen(argument);

    for (;;) {
        mid = (bot + top) / 2;

        if (bot > top) {
            cprintf(ch, "There is no help on that word.\r\n");
            return;
        } else if (!(chk = strn_cmp(argument, help_table[mid].keyword, minlen))) {
            /* trace backwards to find first matching entry. Thanks Jeff Fink! */
            while ((mid > 0) && (!(chk = strn_cmp(argument, help_table[mid - 1].keyword, minlen))))
                mid--;
            /* Added level check */
            if (GET_LEVEL(ch) >= help_table[mid].min_level) {
                page_string(ch, help_table[mid].entry);
            } else {
                cprintf(ch, "There is no help on that word.\r\n");
            }
            return;
        } else {
            if (chk > 0)
                bot = mid + 1;
            else
                top = mid - 1;
        }
    }
}

/* Construct a single line of the who list, which depicts a mortal, and
 * concatenate it to a buffer. */

static void cat_mortal_wholine(char *mbuf, const char *title, const struct char_data *ch, const bool show_as_anon,
                               const bool show_area_in, const bool show_full_class, const bool show_color_nameflags) {
    if (show_as_anon)
        sprintf(mbuf, "%s&0[%s] %s%s&0 %s&0", mbuf, "&9&b-Anon-&0",
                !show_color_nameflags           ? ""
                : PLR_FLAGGED(ch, PLR_FROZEN)   ? "&6&b"
                : PLR_FLAGGED(ch, PLR_NEWNAME)  ? "&1&b"
                : PLR_FLAGGED(ch, PLR_NAPPROVE) ? "&3&b"
                                                : "",
                GET_NAME(ch), title);
    else if (IS_STARSTAR(ch))
        sprintf(mbuf, "%s&0[%s%c%s] %s%s&0 %s%s&0&9&b(&0%s&0&9&b)&0", mbuf,
                show_full_class ? CLASS_WIDE(ch) : CLASS_ABBR(ch), PRF_FLAGGED(ch, PRF_ANON) ? '*' : ' ',
                classes[(int)GET_CLASS(ch)].stars,
                !show_color_nameflags           ? ""
                : PLR_FLAGGED(ch, PLR_FROZEN)   ? "&6&b"
                : PLR_FLAGGED(ch, PLR_NEWNAME)  ? "&1&b"
                : PLR_FLAGGED(ch, PLR_NAPPROVE) ? "&3&b"
                                                : "",
                GET_NAME(ch), title, strlen(title) ? " " : "", RACE_ABBR(ch));
    else
        sprintf(mbuf, "%s&0[%s%c%2d] %s%s&0 %s%s&0&9&b(&0%s&0&9&b)&0", mbuf,
                show_full_class ? CLASS_WIDE(ch) : CLASS_ABBR(ch), PRF_FLAGGED(ch, PRF_ANON) ? '*' : ' ', GET_LEVEL(ch),
                !show_color_nameflags           ? ""
                : PLR_FLAGGED(ch, PLR_FROZEN)   ? "&6&b"
                : PLR_FLAGGED(ch, PLR_NEWNAME)  ? "&1&b"
                : PLR_FLAGGED(ch, PLR_NAPPROVE) ? "&3&b"
                                                : "",
                GET_NAME(ch), title, strlen(title) ? " " : "", RACE_ABBR(ch));

    if (show_area_in)
        sprintf(mbuf + strlen(mbuf), " (%s)", zone_table[world[IN_ROOM(ch)].zone].name);
}

char *WHO_USAGE = "Usage: who [minlev-maxlev] [-qrzw] [-n name] [-c classes]\r\n";

ACMD(do_who) {
    struct descriptor_data *d;
    struct char_data *wch;
    char Imm_buf_title[MAX_STRING_LENGTH];
    char Imm_buf[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    char Mort_buf_title[MAX_STRING_LENGTH];
    char Mort_buf[MAX_STRING_LENGTH];
    char name_search[MAX_NAME_LENGTH + 1];
    char mode;
    char a[120];
    char check[3];
    int low = 0, high = LVL_IMPL, showclass = CLASS_UNDEFINED;
    bool who_room = FALSE, who_zone = FALSE, who_quest = 0;
    bool outlaws = FALSE, noimm = FALSE, nomort = FALSE;
    bool who_where = FALSE, level_range = FALSE;
    bool show_as_anon = FALSE, show_area_in = FALSE;
    int b, c, e;
    int Wizards = 0, Mortals = 0;

    const char *WizLevels[LVL_IMPL - (LVL_IMMORT - 1)] = {"   &0&7&bAvatar&0   ", " &0&1QuasiDeity&0 ",
                                                          " &0&2MinorDeity&0 ",   " &0&1&bMajorDeity&0 ",
                                                          " &0&3SoulForger&0 ",   "  &0&9&bOverlord&0  "};

    /* Set string buffers empty */
    buf[0] = buf1[0] = buf2[0] = arg[0] = Imm_buf_title[0] = Imm_buf[0] = Mort_buf_title[0] = Mort_buf[0] = 0;

    skip_spaces(&argument);
    strcpy(buf, argument);
    name_search[0] = 0;

    while (*buf) {
        half_chop(buf, arg, buf1);
        if (isdigit(*arg)) {
            sscanf(arg, "%d-%d", &low, &high);
            strcpy(buf, buf1);
            level_range = TRUE;
        } else if (*arg == '-') {
            mode = *(arg + 1); /* just in case; we destroy arg in the switch */
            switch (mode) {
                /* 'o' - show only outlaws */
            case 'o':
                outlaws = TRUE;
                strcpy(buf, buf1);
                break;
                /* 'z' - show only folks in the same zone as me */
            case 'z':
                if (pk_allowed && GET_LEVEL(ch) < LVL_IMMORT) {
                    who_zone = FALSE;
                    cprintf(ch, "Wouldn't you like to know?\r\n");
                    return;
                } else {
                    who_zone = TRUE;
                    strcpy(buf, buf1);
                }
                break;
                /* 'q' - show only folks with (quest) flag */
            case 'q':
                who_quest = TRUE;
                strcpy(buf, buf1);
                break;
                /* 'l' - show only folks within a certain level range */
            case 'l':
                half_chop(buf1, arg, buf);
                sscanf(arg, "%d-%d", &low, &high);
                level_range = TRUE;
                break;
                /* 'n' - show only someone with a specific name */
                /* BUG (?) - if no name matches, it then goes on to check titles? */
            case 'n':
                half_chop(buf1, name_search, buf);
                break;
                /* 'r' - show only people in the same room with me */
            case 'r':
                who_room = TRUE;
                strcpy(buf, buf1);
                break;
                /* 'w' - show where people are, by putting their area name after their
                 * name */
            case 'w':
                who_where = TRUE;
                strcpy(buf, buf1);
                break;
                /* 'c' - show only people of a specific class */
            case 'c':
                half_chop(buf1, arg, buf);
                showclass = parse_class(0, 0, arg);
                break;
            default:
                cprintf(ch, "%s", WHO_USAGE);
                return;
                break;
            } /* end of switch (mode) */
        } else {
            /* Arguments were provided, but not switches, so assume that the
             * caller wants to search by name */
            if (!(wch = find_char_by_desc(find_vis_plr_by_name(ch, buf)))) {
                send_to_char(NOPERSON, ch);
                return;
            }

            strcpy(buf, "***\r\n");
            if (GET_LEVEL(wch) >= LVL_IMMORT) {
                sprintf(buf, "%s%s[%s] %s %s", buf, CLRLV(ch, ADAR, C_SPR),
                        GET_WIZ_TITLE(wch) ? GET_WIZ_TITLE(wch) : WizLevels[GET_LEVEL(wch) - LVL_IMMORT], GET_NAME(wch),
                        GET_TITLE(wch) ? GET_TITLE(wch) : "");
            } else {
                cat_mortal_wholine(
                    buf, buf2, wch,
                    PRF_FLAGGED(wch, PRF_ANON) && GET_LEVEL(ch) < LVL_IMMORT && wch != ch, /* show_as_anon */
                    ((!pk_allowed && !PRF_FLAGGED(wch, PRF_ANON)) || wch == ch || GET_LEVEL(ch) >= LVL_IMMORT) &&
                        who_where,              /* show_area_in */
                    TRUE,                       /* show_full_class */
                    GET_LEVEL(ch) >= LVL_IMMORT /* show mortal name flag colors (freeze, rename) */
                );
            }
            if (PRF_FLAGGED(wch, PRF_AFK)) {
                strcat(buf, " (AFK)");
            }
            strcat(buf, "\r\n***\r\n");
            send_to_char(buf, ch);
            return;
        }
    } /* end while arg process */

    /* begin "who" with no args */
#ifdef PRODUCTION
    strcpy(Imm_buf_title, "*** Active deities on FieryMud:\r\n\r\n");
    strcpy(Mort_buf_title, "*** Active players on FieryMud:\r\n\r\n");
#else
    strcpy(Imm_buf_title, "*** Active TEST deities on FieryTESTMud:\r\n\r\n");
    strcpy(Mort_buf_title, "*** Active TEST players on FieryTESTMud:\r\n\r\n");
#endif

    for (d = descriptor_list; d; d = d->next) {
        /* Check various reasons we should skip this player... */
        if (!IS_PLAYING(d)) {
            continue;
        }
        if (d->original) {
            wch = d->original;
        } else if (!(wch = d->character)) {
            continue;
        }
        if (!CAN_SEE(ch, wch)) {
            continue;
        }
        if (GET_LEVEL(wch) < low || GET_LEVEL(wch) > high) {
            continue;
        }
        if ((noimm && GET_LEVEL(wch) >= LVL_IMMORT) || (nomort && GET_LEVEL(wch) < LVL_IMMORT)) {
            continue;
        }
        if (*name_search && str_cmp(GET_NAME(wch), name_search) &&
            (!GET_TITLE(wch) || !strstr(GET_TITLE(wch), name_search))) {
            continue;
        }
        if (outlaws && !PLR_FLAGGED(wch, PLR_KILLER) && !PLR_FLAGGED(wch, PLR_THIEF)) {
            continue;
        }
        if (who_quest && !PRF_FLAGGED(wch, PRF_QUEST)) {
            continue;
        }
        if (who_zone && (world[ch->in_room].zone != world[wch->in_room].zone ||
                         (EFF_FLAGGED(wch, EFF_STEALTH) && GET_HIDDENNESS(wch)))) {
            continue;
        }
        if (who_room && (wch->in_room != ch->in_room)) {
            continue;
        }
        if (showclass != CLASS_UNDEFINED && showclass != GET_CLASS(wch)) {
            continue;
        }
        if ((GET_LEVEL(ch) < LVL_IMMORT) && (PRF_FLAGGED(wch, PRF_ANON)) &&
            (showclass != CLASS_UNDEFINED || level_range)) {
            continue;
        }

        /* Prepare the string that will represent this player in the who list: buf2
         */
        buf2[0] = 0;

        /* First put on the player's title. */
        if (GET_TITLE(wch))
            strcpy(a, GET_TITLE(wch));
        else
            *a = '\0';

        /* The title can take up a maximum of 45 spaces.  That's spaces, not
         * characters, so the following loops examine &X color codes as they try to
         * put the longest title possible that doesn't exceed 45. */
        c = 0;
        for (b = 0; a[b] != '\0'; b++) {
            /*This little section uncounts any ansi numbers */
            sprintf(check, "%c%c", ((b - 1) > 0) ? a[(b - 1)] : a[0], a[b]);
            if (check[0] == CREL || check[0] == CABS) {
                e = 49;
                while (e < 58) {
                    if ((check[1] == (char)e)) {
                        c = c + 2;
                    }
                    e++;
                }
            }

            if (check[1] == (char)98 && check[0] == CREL) {
                c = c + 2;
            }
            /*end of ansi uncounting */

            if (MAX(0, (b - c)) <= 45) {
                sprintf(buf2, "%s%c", buf2, a[b]);
            } else {
                break;
            }
        }

        if (GET_LEVEL(wch) >= LVL_IMMORT) {

            /* Display an immortal */

            sprintf(Imm_buf, "%s%s&0[%s] %s %s", Imm_buf, CLRLV(ch, ADAR, C_SPR),
                    GET_WIZ_TITLE(wch) ? GET_WIZ_TITLE(wch) : WizLevels[GET_LEVEL(wch) - LVL_IMMORT], GET_NAME(wch),
                    GET_TITLE(wch) ? GET_TITLE(wch) : "");
            Wizards++;
        } else {

            /* Display a mortal */

            /* Decide what details to display */

            show_area_in = FALSE;
            show_as_anon = FALSE;

            if (GET_LEVEL(ch) >= LVL_IMMORT || ch == wch || (!PRF_FLAGGED(wch, PRF_ANON) && !pk_allowed)) {
                show_area_in = who_where;
            } else if (PRF_FLAGGED(wch, PRF_ANON)) {
                show_as_anon = TRUE;
            }

            cat_mortal_wholine(Mort_buf, buf2, wch, show_as_anon, show_area_in, showclass != CLASS_UNDEFINED,
                               GET_LEVEL(ch) >= LVL_IMMORT);

            Mortals++;
        }

        buf[0] = 0;

        if (GET_INVIS_LEV(wch))
            sprintf(buf, "%s (i%d)", buf, GET_INVIS_LEV(wch));
        else if (EFF_FLAGGED(wch, EFF_INVISIBLE))
            strcat(buf, " (invis)");

        if (PLR_FLAGGED(wch, PLR_MAILING))
            strcat(buf, " (mailing)");
        else if (PLR_FLAGGED(wch, PLR_WRITING))
            strcat(buf, " (writing)");
        else if (wch->desc && EDITING(wch->desc))
            strcat(buf, " (writing)");

        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            switch (STATE(d)) {
            case CON_OEDIT:
                strcat(buf, " (Object Edit)");
                break;
            case CON_MEDIT:
                strcat(buf, " (Mobile Edit)");
                break;
            case CON_ZEDIT:
                strcat(buf, " (Zone Edit)");
                break;
            case CON_SEDIT:
                strcat(buf, " (Shop Edit)");
                break;
            case CON_REDIT:
                strcat(buf, " (Room Edit)");
                break;
            case CON_TRIGEDIT:
                strcat(buf, " (Trigger Edit)");
                break;
            case CON_HEDIT:
                strcat(buf, " (Help Edit)");
                break;
            case CON_SDEDIT:
                strcat(buf, " (Skill Edit)");
                break;
            case CON_GEDIT:
                strcat(buf, " (Grant Edit)");
                break;
            }
        }

        if (IS_HIDDEN(wch)) {
            if (GET_LEVEL(ch) >= LVL_IMMORT)
                sprintf(buf, "%s (h%ld)", buf, GET_HIDDENNESS(wch));
            else
                strcat(buf, " (hidden)");
        }
        if (PRF_FLAGGED(wch, PRF_DEAF))
            strcat(buf, " (deaf)");
        if (PRF_FLAGGED(wch, PRF_NOTELL))
            strcat(buf, " (notell)");
        if (PRF_FLAGGED(wch, PRF_QUEST))
            strcat(buf, " (quest)");
        if (PLR_FLAGGED(wch, PLR_THIEF))
            strcat(buf, " (THIEF)");
        if (GET_LEVEL(wch) >= LVL_IMMORT)
            strcat(buf, CLRLV(ch, ANRM, C_SPR));
        if (PRF_FLAGGED(wch, PRF_AFK))
            strcat(buf, " (AFK)"); /* do check so mortals cant see if immos afk? */
        strcat(buf, "\r\n");

        if (GET_LEVEL(wch) >= LVL_IMMORT)
            strcat(Imm_buf, buf);
        else
            strcat(Mort_buf, buf);
    } /* end of for */

    /* To allow for flexibility, add the title strings now */
    if (Mortals) {
        strcat(Mort_buf_title, Mort_buf);
        strcpy(Mort_buf, Mort_buf_title);
    }
    if (Wizards) {
        strcat(Imm_buf_title, Imm_buf);
        strcpy(Imm_buf, Imm_buf_title);
        if (Mortals)
            strcat(Imm_buf, "\r\n");
        strcat(Imm_buf, Mort_buf);
        strcpy(Mort_buf, Imm_buf);
    }

    if ((Wizards + Mortals) == 0)
        strcpy(buf, "No wizards or mortals are currently visible to you.\r\n");

    if (Wizards)
        sprintf(buf, "\r\nThere %s %d visible deit%s%s", (Wizards == 1 ? "is" : "are"), Wizards,
                (Wizards == 1 ? "y" : "ies"), (Mortals ? " and there" : "."));

    if (Mortals) {
        sprintf(buf, "%s %s %d visible mortal%s.\r\n", (Wizards ? buf : "\r\nThere"), (Mortals == 1 ? "is" : "are"),
                Mortals, (Mortals == 1 ? "" : "s"));
    }

    if (!Mortals && Wizards)
        strcat(buf, "\r\n");
    strcat(buf, "\r\n");
    if ((Wizards + Mortals) > boot_high)
        boot_high = Wizards + Mortals;
    sprintf(buf, "%sThere is a boot-time high of %d player%s.\r\n", buf, boot_high, (boot_high == 1 ? "" : "s"));

    strcat(Mort_buf, buf);
    page_string(ch, Mort_buf);
}

char iplist[300][40];
char userlist[300][MAX_STRING_LENGTH];

typedef int DType;
DType realx[1][MAX_STRING_LENGTH];

static void swapc(int i, int j) {
    char itemt[MAX_STRING_LENGTH];
    char itemr[MAX_STRING_LENGTH];
    strcpy(itemt, iplist[i]);
    strcpy(itemr, userlist[i]);
    strcpy(iplist[i], iplist[j]);
    strcpy(userlist[i], userlist[j]);
    strcpy(iplist[j], itemt);
    strcpy(userlist[j], itemr);
}

static void sortc(int l, int u) {
    int i, j;
    char item[MAX_STRING_LENGTH];
    if (l >= u)
        return;
    strcpy(item, iplist[l]);
    i = l;
    j = u + 1;
    for (;;) {
        do
            i++;
        while (i <= u && strcmp(iplist[i], item) < 0);
        do
            j--;
        while (strcmp(iplist[j], item) > 0);
        if (i > j)
            break;
        swapc(i, j);
    }
    swapc(l, j);
    sortc(l, j - 1);
    sortc(j + 1, u);
}

#define USERS_FORMAT                                                                                                   \
    "format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c "                                                  \
    "classlist]\r\n              [-o] [-p] [-i]\r\n"

ACMD(do_users) {
    char line[200], line2[220], idletime[10], classname[20];
    char state[30], *timeptr, *format, mode, hostnum[40];
    char roomstuff[200], room[26], nametrun[11], position[17]; /* Changed position to 17 from 9 cuz
                                                                  mortally wounded was over flowing */
    char ipbuf[MAX_STRING_LENGTH], userbuf[MAX_STRING_LENGTH];
    char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
    struct char_data *tch;
    struct descriptor_data *d;
    size_t i;
    int low = 0, high = LVL_IMPL, num_can_see = 0;
    int showclass = CLASS_UNDEFINED, outlaws = 0, playing = 0, deadweight = 0, ipsort = 0;
    int counter = 0, forcnt, cntr;
    char color[10];
    int repeats[300];

    host_search[0] = name_search[0] = '\0';
    *ipbuf = '\0';
    *userbuf = '\0';

    strcpy(buf, argument);
    while (*buf) {
        half_chop(buf, arg, buf1);
        if (*arg == '-') {
            mode = *(arg + 1); /* just in case; we destroy arg in the switch */
            switch (mode) {
            case 'o':
            case 'k':
                outlaws = 1;
                playing = 1;
                strcpy(buf, buf1);
                break;
            case 'p':
                playing = 1;
                strcpy(buf, buf1);
                break;
            case 'd':
                deadweight = 1;
                strcpy(buf, buf1);
                break;
            case 'l':
                playing = 1;
                half_chop(buf1, arg, buf);
                sscanf(arg, "%d-%d", &low, &high);
                break;
            case 'n':
                playing = 1;
                half_chop(buf1, name_search, buf);
                break;
            case 'h':
                playing = 1;
                half_chop(buf1, host_search, buf);
                break;
            case 'c':
                playing = 1;
                half_chop(buf1, arg, buf);
                showclass = parse_class(0, 0, arg);
                break;
            case 'i':
                ipsort = 1;
                for (i = 0; i <= 300; i++) {
                    *iplist[i] = '\0';
                    *userlist[i] = '\0';
                    repeats[i] = 0;
                }
                strcpy(buf, buf1);
                break;
            default:
                send_to_char(USERS_FORMAT, ch);
                return;
                break;
            } /* end of switch */

        } else { /* endif */
            send_to_char(USERS_FORMAT, ch);
            return;
        }
    } /* end while (parser) */

    strcpy(line,
           "Soc  Username    User's Host   Idl  Login        "
           "RoomNo/RoomName      Position\r\n");
    strcat(line,
           "--- ---------- --------------- --- -------- "
           "------------------------- ---------\r\n");
    strcpy(userbuf, line);

    one_argument(argument, arg);

    for (d = descriptor_list; d; d = d->next) {
        if ((STATE(d) != CON_PLAYING) && playing)
            continue;
        if ((STATE(d) == CON_PLAYING) && deadweight)
            continue;
        if (STATE(d) == CON_PLAYING) {
            if (d->original)
                tch = d->original;
            else if (!(tch = d->character))
                continue;

            if (*host_search && !strstr(d->host, host_search))
                continue;
            if (*name_search && str_cmp(GET_NAME(tch), name_search))
                continue;
            if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
                continue;
            if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) && !PLR_FLAGGED(tch, PLR_THIEF))
                continue;
            if (showclass != CLASS_UNDEFINED && showclass != GET_CLASS(tch))
                continue;
            if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
                continue;

            if (d->original)
                sprintf(classname, "[%2d %s]", GET_LEVEL(d->original), CLASS_ABBR(d->original));
            else
                sprintf(classname, "[%2d %s]", GET_LEVEL(d->character), CLASS_ABBR(d->character));
        } else
            strcpy(classname, "  ---  ");

        timeptr = asctime(localtime(&d->login_time));
        timeptr += 11;
        *(timeptr + 8) = '\0';

        if (!d->connected && d->original)
            strcpy(state, "Switched");
        else
            strcpy(state, connected_types[d->connected]);

        if (d->original && !d->connected)
            sprintf(idletime, "%3d", d->original->char_specials.timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
        else if (d->character && !d->connected)
            sprintf(idletime, "%3d", d->character->char_specials.timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
        else
            strcpy(idletime, "");

        format = "%3d %-10s %-14s %-3s %-8s %-25s&0 %-9s\r\n";

        hostnum[0] = '\0';
        roomstuff[0] = '\0';
        room[0] = '\0';
        nametrun[0] = '\0';
        position[0] = '\0';

        strcat(room, state);

        if (d->host && *d->host) {
            strcat(hostnum, d->host);
            if (ipsort) {
                strcpy(iplist[counter], hostnum);
            }
        } else {
            strcat(hostnum, " Unknown Host");
        }

        if (d->connected) {
            sprintf(line2, "%s%s%s", CLRLV(ch, FGRN, C_SPR), hostnum, CLRLV(ch, ANRM, C_SPR));
            strcpy(hostnum, line2);
        }

        /* Going to have to truncate room name and player names!! */

        if (d->character && GET_NAME(d->character)) {
            if (d->original) {
                if ((d->original->in_room != NOWHERE) && (d->connected == 0)) {
                    sprintf(roomstuff, "%d/%s", world[d->original->in_room].vnum,
                            strip_ansi(world[d->original->in_room].name));
                    *(room) = '\0';
                    strncat(room, roomstuff, sizeof(room) - 1);
                    room[sizeof(room) - 1] = '\0';
                }
                sprinttype(GET_POS(d->original), position_types, position);
                strncat(nametrun, GET_NAME(d->original), sizeof(nametrun) - 1);
                sprintf(line, format, d->desc_num, nametrun, hostnum, idletime, timeptr, room, position);
            } else {
                if ((d->character->in_room != NOWHERE) && (d->connected == 0)) {
                    sprintf(roomstuff, "%d/%s", world[d->character->in_room].vnum,
                            strip_ansi(world[d->character->in_room].name));
                    *(room) = '\0';
                    strncat(room, roomstuff, sizeof(room) - 1);
                    room[sizeof(room) - 1] = '\0';
                }
                sprinttype(GET_POS(d->character), position_types, position);
                strncat(nametrun, GET_NAME(d->character), sizeof(nametrun) - 1);
                sprintf(line, format, d->desc_num, nametrun, hostnum, idletime, timeptr, room, position);
            }
        } else
            sprintf(line, format, d->desc_num, "  ---   ", hostnum, idletime, timeptr, " At menu screen or Other ",
                    "   -None   ");

        if (d->connected || (!d->connected && CAN_SEE(ch, d->character))) {
            if (ipsort) {
                strcpy(userlist[counter], strip_ansi(line));
            } else {
                sprintf(userbuf, "%s%s", userbuf, line);
            }
            num_can_see++;
        }
        counter++;
    } /*end descriptor loop */

    if (ipsort) {
        sortc(0, counter);
        for (forcnt = 0; forcnt <= counter; forcnt++) {
            for (cntr = 0; cntr <= counter; cntr++)
                if (strcmp(iplist[forcnt], iplist[cntr]) == 0)
                    repeats[forcnt]++;
        }
        for (forcnt = 0; forcnt <= counter; forcnt++)
            if (*userlist[forcnt]) {
                if (forcnt > 0) {
                    if (strcmp(iplist[forcnt], iplist[forcnt - 1]) != 0) {
                        if (strcmp(color, "&7") == 0)
                            strcpy(color, "&7&b");
                        else
                            strcpy(color, "&7");
                    }
                    if (repeats[forcnt] == 3)
                        strcpy(color, "&1&b");
                    sprintf(userbuf, "%s%s%s&0", userbuf, color, userlist[forcnt]);
                } else {
                    strcpy(color, "&7");
                    sprintf(userbuf, "%s%s%s&0", userbuf, color, userlist[forcnt]);
                }
            }
    }

    sprintf(userbuf, "%s\r\n%d visible sockets connected.\r\n", userbuf, num_can_see);
    page_string(ch, userbuf);
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps) {
    switch (subcmd) {
    case SCMD_CLEAR:
        send_to_char("\033[H\033[J", ch);
        break;
    case SCMD_VERSION:
        send_to_char(circlemud_version, ch);
        break;
    case SCMD_WHOAMI:
        if (POSSESSED(ch))
            sprintf(buf, "You are %s, currently inhabiting %s's body.\r\n", GET_NAME(POSSESSOR(ch)), GET_NAME(ch));
        else
            sprintf(buf, "You are %s.\r\n", GET_NAME(ch));
        send_to_char(buf, ch);
        break;
    default:
        return;
        break;
    }
}

static void perform_mortal_where(struct char_data *ch, char *arg) {
    register struct char_data *i;
    register struct descriptor_data *d;

    if (!*arg) {
        send_to_char("Players in your zone\r\n--------------------\r\n", ch);
        for (d = descriptor_list; d; d = d->next)
            if (!d->connected) {
                i = (d->original ? d->original : d->character);
                if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE) &&
                    (world[ch->in_room].zone == world[i->in_room].zone)) {
                    sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room].name);
                    send_to_char(buf, ch);
                }
            }
    } else { /* print only FIRST char, not all. */
        for (i = character_list; i; i = i->next)
            if (world[i->in_room].zone == world[ch->in_room].zone && CAN_SEE(ch, i) && (i->in_room != NOWHERE) &&
                isname(arg, GET_NAMELIST(i))) {
                sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room].name);
                send_to_char(buf, ch);
                return;
            }
        send_to_char("No-one around by that name.\r\n", ch);
    }
}

static void print_object_location(int num, struct obj_data *obj, struct char_data *ch, int recur) {
    if (num > 0)
        sprintf(buf, "O%3d. %-25s - ", num, strip_ansi(obj->short_description));
    else
        sprintf(buf, "%34s", " - ");

    if (obj->in_room > NOWHERE)
        pprintf(ch, "%s[%5d] %s\r\n", buf, world[obj->in_room].vnum, world[obj->in_room].name);
    else if (obj->carried_by)
        pprintf(ch, "%scarried by %s\r\n", buf, PERS(obj->carried_by, ch));
    else if (obj->worn_by)
        pprintf(ch, "%sworn by %s\r\n", buf, PERS(obj->worn_by, ch));
    else if (obj->in_obj) {
        pprintf(ch, "%sinside %s%s\r\n", buf, obj->in_obj->short_description, (recur ? ", which is" : " "));
        if (recur)
            print_object_location(0, obj->in_obj, ch, recur);
    } else
        pprintf(ch, "%sin an unknown location\r\n", buf);
}

static void perform_immort_where(struct char_data *ch, char *arg) {
    register struct char_data *i;
    register struct obj_data *k;
    struct descriptor_data *d;
    int num = 0, found = 0;

    if (!*arg) {
        send_to_char("Players\r\n-------\r\n", ch);
        for (d = descriptor_list; d; d = d->next)
            if (!d->connected) {
                i = (d->original ? d->original : d->character);
                if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE)) {
                    if (d->original)
                        sprintf(buf, "%-20s - [%5d] %s (in %s)\r\n", GET_NAME(i), world[d->character->in_room].vnum,
                                world[d->character->in_room].name, GET_NAME(d->character));
                    else
                        sprintf(buf, "%-20s - [%5d] %s\r\n", GET_NAME(i), world[i->in_room].vnum,
                                world[i->in_room].name);
                    send_to_char(buf, ch);
                }
            }
    } else {
        for (i = character_list; i; i = i->next)
            if (CAN_SEE(ch, i) && i->in_room != NOWHERE && isname(arg, GET_NAMELIST(i))) {
                found = 1;
                pprintf(ch, "M%3d. %-25s - [%5d] %s\r\n", ++num, GET_NAME(i), world[i->in_room].vnum,
                        world[i->in_room].name);
            }
        for (num = 0, k = object_list; k; k = k->next)
            if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
                found = 1;
                print_object_location(++num, k, ch, TRUE);
            }
        if (found)
            start_paging(ch);
        else
            send_to_char("Couldn't find any such thing.\r\n", ch);
    }
}

ACMD(do_where) {
    one_argument(argument, arg);

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        perform_immort_where(ch, arg);
    else
        perform_mortal_where(ch, arg);
}

ACMD(do_consider) {
    struct char_data *victim;
    int diff, mountdiff, tamediff, mountmsg = 0;
    int apparent_vlvl;

    one_argument(argument, buf);

    if (!(victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf)))) {
        send_to_char("Consider killing who?\r\n", ch);
        return;
    }
    if (victim == ch) {
        send_to_char("You're considering killing yourself?!?\r\n", ch);
        return;
    }

    /* Our mob level difficulties are a bit warped, so adjust. */
    apparent_vlvl = GET_LEVEL(victim);
    if (GET_LEVEL(ch) > 20)
        apparent_vlvl += 9;
    else if (GET_LEVEL(ch) > 10)
        apparent_vlvl += GET_LEVEL(ch) - 10;
    diff = apparent_vlvl - GET_LEVEL(ch);

    if (!IS_NPC(ch) || !IS_NPC(victim)) {
        act("$n sizes $N up with a quick glance.", FALSE, ch, 0, victim, TO_NOTVICT);
        act("$n sizes you up with a quick glance.", FALSE, ch, 0, victim, TO_VICT);
    }

    if (diff <= -10)
        send_to_char("Now where did that chicken go?\r\n", ch);
    else if (diff <= -5)
        send_to_char("You could do it with a needle!\r\n", ch);
    else if (diff <= -2)
        send_to_char("Easy.\r\n", ch);
    else if (diff <= -1)
        send_to_char("Fairly easy.\r\n", ch);
    else if (diff == 0)
        send_to_char("The perfect match!\r\n", ch);
    else if (diff <= 1)
        send_to_char("You would need some luck!\r\n", ch);
    else if (diff <= 2)
        send_to_char("You would need a lot of luck!\r\n", ch);
    else if (diff <= 3)
        send_to_char("You would need a lot of luck and great equipment!\r\n", ch);
    else if (diff <= 5)
        send_to_char("Do you feel lucky, punk?\r\n", ch);
    else if (diff <= 10)
        send_to_char("Are you mad!?\r\n", ch);
    else if (diff <= 20)
        send_to_char("You ARE mad!\r\n", ch);
    else if (diff <= 30)
        send_to_char("What do you want your epitaph to say?\r\n", ch);
    else
        send_to_char("This thing will kill you so FAST it's not EVEN funny!\r\n", ch);

    if (GET_LEVEL(victim) < LVL_IMMORT && IS_NPC(victim) && MOB_FLAGGED(victim, MOB_MOUNTABLE)) {
        mountmsg = 1;
        mountdiff = mountlevel(victim) - ideal_mountlevel(ch);
        sprintf(buf, "$E looks %s",
                mountdiff > MOUNT_LEVEL_FUDGE ? "impossible to ride"
                : mountdiff < 1               ? "easy to ride"
                : mountdiff < 2               ? "tough to ride"
                : mountdiff < 4               ? "hard to ride"
                                              : "difficult to ride");
    }
    if (mountmsg && GET_SKILL(ch, SKILL_TAME) > 0 &&
        !(EFF_FLAGGED(victim, EFF_TAMED) || EFF_FLAGGED(victim, EFF_TAMED))) {
        tamediff = mountlevel(victim) - ideal_tamelevel(ch);
        sprintf(buf, "%s, and %s", buf,
                tamediff > MOUNT_LEVEL_FUDGE ? "impossible to tame"
                : tamediff < 2               ? "easy to tame"
                : tamediff < 3               ? "tough to tame"
                : tamediff < 4               ? "hard to tame"
                                             : "difficult to tame");
    }
    if (mountmsg) {
        sprintf(buf, "%s.", buf);
        act(buf, FALSE, ch, 0, victim, TO_CHAR);
    }
}

ACMD(do_diagnose) {
    struct char_data *vict;

    one_argument(argument, buf);

    if (*buf) {
        if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf))))
            send_to_char(NOPERSON, ch);
        else
            print_char_to_char(vict, ch, SHOW_BASIC_DESC);
    } else if (FIGHTING(ch))
        print_char_to_char(FIGHTING(ch), ch, SHOW_BASIC_DESC);
    else
        send_to_char("Diagnose who?\r\n", ch);
}

static const char *ctypes[] = {"off", "sparse", "normal", "complete", "\n"};

ACMD(do_color) {
    int tp;

    if (IS_NPC(ch))
        return;

    one_argument(argument, arg);

    if (!*arg) {
        sprintf(buf, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
        send_to_char(buf, ch);
        return;
    }
    if (((tp = searchblock(arg, ctypes, FALSE)) == -1)) {
        send_to_char("Usage: color { Off | Sparse | Normal | Complete }\r\n", ch);
        return;
    }
    REMOVE_FLAG(PRF_FLAGS(ch), PRF_COLOR_1);
    REMOVE_FLAG(PRF_FLAGS(ch), PRF_COLOR_2);
    if (IS_SET(tp, 1))
        SET_FLAG(PRF_FLAGS(ch), PRF_COLOR_1);
    if (IS_SET(tp, 2))
        SET_FLAG(PRF_FLAGS(ch), PRF_COLOR_2);

    sprintf(buf, "Your %scolor%s is now %s.\r\n", CLRLV(ch, FRED, C_SPR), CLRLV(ch, ANRM, C_SPR), ctypes[tp]);
    send_to_char(buf, ch);
}

#define COMMANDS_LIST_COLUMNS 7

ACMD(do_commands) {
    int i, j, v, cmd_num;
    int wizhelp = 0, socials = 0;
    struct char_data *vict;

    int num_elements, column_height, box_height, num_xcolumns, num_rows, cont;

    one_argument(argument, arg);

    if (*arg && GET_LEVEL(ch) >= LVL_IMMORT) {
        if (!(vict = find_char_by_desc(find_vis_by_name(ch, arg))) || IS_NPC(vict)) {
            send_to_char("Who is that?\r\n", ch);
            return;
        }
        if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
            send_to_char("You can't see the commands of people above your level.\r\n", ch);
            return;
        }
    } else
        vict = ch;

    if (subcmd == SCMD_SOCIALS)
        socials = 1;
    else if (subcmd == SCMD_WIZHELP)
        wizhelp = 1;

    sprintf(buf, "The following %s%s are available to %s:\r\n", wizhelp ? "privileged " : "",
            socials ? "socials" : "commands", vict == ch ? "you" : GET_NAME(vict));
    send_to_char(buf, ch);

    /* Prepare to make a list of commands with vertically-sorted columns. */

    /* Count how many commands will be printed */

    for (num_elements = 0, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
        i = cmd_sort_info[cmd_num].sort_pos;
        if (cmd_info[i].minimum_level >= 0 && can_use_command(vict, i) &&
            (cmd_info[i].minimum_level >= LVL_IMMORT) == wizhelp && (wizhelp || socials == cmd_sort_info[i].is_social))
            num_elements++;
    }

    box_height = num_elements / COMMANDS_LIST_COLUMNS + 1;
    num_xcolumns = num_elements % COMMANDS_LIST_COLUMNS;
    num_rows = box_height + (num_xcolumns ? 1 : 0);

    /* Fill the text buffer with spaces and appropriate newlines */
    for (j = 0; j < num_rows; j++) {
        for (i = 0; i < 77; i++)
            buf[j * 79 + i] = ' ';
        buf[(j + 1) * 79 - 2] = '\r';
        buf[(j + 1) * 79 - 1] = '\n';
    }

    /* Insert the commands into the buffer */

    cmd_num = 1;

    for (i = 0; i < COMMANDS_LIST_COLUMNS; i++) {
        column_height = box_height + (i < num_xcolumns ? 1 : 0);
        for (j = 0; j < column_height; j++) {
            /* Ready to print an element - find the next valid command */
            for (cont = 1; cmd_num < num_of_cmds && cont; cmd_num++) {
                v = cmd_sort_info[cmd_num].sort_pos;
                if (cmd_info[v].minimum_level >= 0 && can_use_command(vict, v) &&
                    (cmd_info[v].minimum_level >= LVL_IMMORT) == wizhelp &&
                    (wizhelp || socials == cmd_sort_info[v].is_social)) {
                    strncpy(buf + j * 79 + i * 11, cmd_info[v].command, strlen(cmd_info[v].command));
                    /* Break out of cmd_num loop, let j loop continue */
                    cont = 0;
                }
            }
        }
        /* Cmd number always seems to be one ahead now */
        cmd_num--;
    }

    buf[79 * (box_height - 1) + 11 * num_xcolumns] = '\0';
    /*
     * If the number of columns with an extra row is 0, then the output
     * already ends with \r\n.   But if it's not 0, then add it.   Either
     * way, make sure the string is null-terminated.
     */
    if (num_xcolumns) {
        buf[79 * (box_height - 1) + 11 * num_xcolumns] = '\r';
        buf[79 * (box_height - 1) + 11 * num_xcolumns + 1] = '\n';
        buf[79 * (box_height - 1) + 11 * num_xcolumns + 2] = '\0';
    } else
        buf[79 * (box_height - 1) + 11 * num_xcolumns] = '\0';
    page_string(ch, buf);
}

const char *save_message(int save) {
    if (save < -19)
        return "Awesome!";
    else if (save < -15)
        return "Very Good";
    else if (save < -12)
        return "Good";
    else if (save < -9)
        return "Fair";
    else if (save < -6)
        return "Average";
    else if (save <= 0)
        return "Poor";
    else
        return "Cursed";
}

const char *align_message(int align) {
    if (align <= -350)
        return "Evil";
    else if (align < 350)
        return "Neutral";
    else
        return "Good";
}

const char *hitdam_message(int value) {
    char *messages[] = {
        "Bad", "Poor", "Average", "Fair", "Good", "Very Good",
    };

    if (value < 0)
        return "Cursed!";

    if (value >= 30)
        return "Awesome!";

    return messages[value / 5];
}

const char *armor_message(int ac) {
    if (ac <= -100)
        return "Stronger than dragon armor!";
    else if (ac < -80)
        return "Extremely heavily armored";
    else if (ac < -60)
        return "Very heavily armored";
    else if (ac < -40)
        return "Heavily armored";
    else if (ac < -20)
        return "Well armored";
    else if (ac < 0)
        return "Armored";
    else if (ac < 20)
        return "Moderately armored";
    else if (ac < 40)
        return "Modestly armored";
    else if (ac < 60)
        return "Fairly armored";
    else if (ac < 80)
        return "Lightly armored";
    else if (ac < 100)
        return "Sparsely armored";
    else
        return "That birthday suit provides great protection!";
}

const char *perception_message(int perception) {
    if (perception <= 0)
        return "unconcious";
    else if (perception <= 50)
        return "oblivious";
    else if (perception <= 115)
        return "inattentive";
    else if (perception <= 230)
        return "distracted";
    else if (perception <= 345)
        return "concious";
    else if (perception <= 460)
        return "alert";
    else if (perception <= 575)
        return "observant";
    else if (perception <= 690)
        return "sharp-eyed";
    else if (perception <= 805)
        return "very perceptive";
    else if (perception <= 920)
        return "aware";
    else
        return "all-seeing";
}

const char *hiddenness_message(int hiddenness) {
    if (hiddenness <= 0)
        return "visible";
    else if (hiddenness <= 150)
        return "unnoticeable";
    else if (hiddenness <= 300)
        return "disguised";
    else if (hiddenness <= 450)
        return "hidden";
    else if (hiddenness <= 600)
        return "well hidden";
    else if (hiddenness <= 750)
        return "nearly invisible";
    else if (hiddenness <= 900)
        return "unseen";
    else
        return "godlike";
}

const char *ability_message(int value) {
    if (value > 90)
        return rolls_abils_result[0];
    else if (value > 80)
        return rolls_abils_result[1];
    else if (value > 62)
        return rolls_abils_result[2];
    else if (value > 52)
        return rolls_abils_result[3];
    else
        return rolls_abils_result[4];
}

long xp_percentage(struct char_data *ch) {
    long current, total, next_level;

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (IS_STARSTAR(ch)) {
            return 100;
        }
        next_level = exp_next_level(GET_LEVEL(ch), GET_CLASS(ch));
        total = next_level - exp_next_level(GET_LEVEL(ch) - 1, GET_CLASS(ch));

        if (total < 1) {
            return 0;
        }

        current = total - next_level + GET_EXP(ch);

        if (total - current == 1) {
            return 100;
        }

        return ((100 * current) / total);
    } else {
        return 0;
    }
}

const char *exp_message(struct char_data *ch) {
    long percent, current, total, etl;
    char *messages[] = {
        "&4&8You still have a very long way to go to your next level.&0",
        "&4&8You have gained some progress towards your next level.&0",
        "&6You are about one-quarter of the way to your next level.&0",
        "&4&8You are about a third of the way to your next level.&0",
        "&4&8You are almost half-way to your next level.&0",
        "&6You are just past the half-way point to your next level.&0",
        "&4&8You are well on your way to your next level.&0",
        "&4&8You are about three-quarters of the way to your next level.&0",
        "&6You are almost ready to attain your next level.&0",
        "&4&8You should level anytime now!&0",
        "&4You are SO close to the next level.&0",
    };

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return "Experience has no meaning for you.";

    /* Get exp to level once, because it's actually expensive to calculate. */
    etl = exp_next_level(GET_LEVEL(ch), GET_CLASS(ch));
    total = etl - exp_next_level(GET_LEVEL(ch) - 1, GET_CLASS(ch));

    if (total < 1)
        /* You're probably level 0 */
        return "&2You're fairly weak.&0";

    current = total - etl + GET_EXP(ch);
    percent = (100 * current) / total;

    if (IS_STARSTAR(ch))
        return "&3You are as powerful as a mortal can be!&0";
    else if (GET_LEVEL(ch) == LVL_MAX_MORT && current > total)
        return "&4&8You are working towards getting your stars!&0";
    else if (total - current == 1)
        return "&4&8You are ready for the next level!&0";
    else if (percent < 4)
        return "&4&8You have just begun the journey to your next level.&0";
    else if (percent >= 0 && percent <= 100)
        return messages[percent / 10];
    else
        return "&1You are somewhere along the way to your next level.&0";
}

const char *exp_bar(struct char_data *ch, int length, int gradations, int sub_gradations, bool color) {
    static char bar[120];
    int i;
    int grad_count, length_per_grad, distance;
    int sub_grad_count, length_per_sub_grad, sub_distance;
    long current, total, next_level, exp_per_grad, exp_towards_next_grad;

    length = MIN(80, MAX(length, 1));

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        /* Get exp to level once, because it's actually expensive to calculate. */
        next_level = exp_next_level(GET_LEVEL(ch), GET_CLASS(ch));
        total = next_level - exp_next_level(GET_LEVEL(ch) - 1, GET_CLASS(ch));

        /* Avoid divide by zero errors. */
        if (total < 1) {
            strcpy(bar, "&2-?-&0");
            return bar;
        }

        /* No negative values, please */
        current = MAX(total - next_level + GET_EXP(ch), 0);

        grad_count = (length * current) / total;
        length_per_grad = length / gradations;
        /*
         * Dividing and multiplying by the same integer value has the
         * effect of rounding in amounts of that value.
         */
        distance = (grad_count / length_per_grad) * length_per_grad;

        exp_per_grad = total / gradations;
        exp_towards_next_grad = current % exp_per_grad;
        sub_grad_count = (length * exp_towards_next_grad) / exp_per_grad;
        length_per_sub_grad = length / sub_gradations;
        sub_distance = (sub_grad_count / length_per_sub_grad) * length_per_sub_grad;
        ++sub_distance;
    }
    /*
     * The code above does fuuunky stuff with immortals (i.e. the game
     * crashes) so, we'll just set distance and sub_distance for immortals
     * to some arbitrary value.
     */
    else {
        distance = 0;
        sub_distance = 0;
        total = current = 0;
    }

    /*
     * Set the marker to the end of the bar if we're ready to level in
     * case the number of gradations and the length aren't common
     * multiples.
     */
    if (total - current == 1)
        distance = length - 1;
    if (GET_LEVEL(ch) >= LVL_IMMORT)
        distance = 0;

    /* Just in case. */
    distance = MIN(length, MAX(distance, 0));

    memset(bar, '\0', sizeof(bar));

    /*
     * If color is on:
     *    for immortals, just make the whole bar red, without a marker
     *    if ready for next level, make whole bar cyan, with a marker
     *    otherwise, bar is cyan until marker, then white.
     */

    if (color) {
        if (GET_LEVEL(ch) >= LVL_IMMORT)
            strcpy(bar, HRED);
        else if (total - current > 1)
            strcpy(bar, HBLU);
        else if (IS_STARSTAR(ch))
            strcpy(bar, HYEL);
        else if (GET_LEVEL(ch) == LVL_MAX_MORT)
            strcpy(bar, HGRN);
        else
            strcpy(bar, HCYN); /* ready for next level */
    }

    for (i = 0; i < length; ++i) {
        if (GET_LEVEL(ch) >= LVL_IMMORT || (GET_LEVEL(ch) == LVL_MAX_MORT && current > total))
            strcat(bar, "-");
        else if (i == distance) {
            if (color)
                strcat(bar, FCYN);
            strcat(bar, "*");
            if (color) {
                if (sub_distance - 1 <= distance)
                    strcat(bar, ANRM);
                strcat(bar, FBLU);
            }
        } else if (i <= sub_distance - 1) {
            strcat(bar, "=");
            if (color && i == sub_distance - 1)
                strcat(bar, AFBLU);
        } else
            strcat(bar, "-");
    }

    if (color)
        strcat(bar, ANRM);

    return bar;
}

const char *cooldown_bar(struct char_data *ch, int cooldown, int length, int gradations, bool color) {
    static char bar[120];
    char *p = bar;
    int i, grad_count, length_per_grad, distance, current, total;
    float percent = 0.0f;

    length = MIN(80, MAX(length, 1));

    current = GET_COOLDOWN(ch, cooldown);
    total = GET_COOLDOWN_MAX(ch, cooldown);

    if (total != 0) {
        grad_count = (length * current) / total;
        length_per_grad = length / gradations;
        /* round to the closest length_per_grad */
        distance = (grad_count / length_per_grad) * length_per_grad;
        percent = current / (float)total;
    }

    /* Just in case. */
    distance = MIN(length, MAX(distance, 0));

    memset(bar, '\0', sizeof(bar));

    if (color) {
        if (current == 0 || total == 0)
            strcpy(bar, AFCYN);
        else if (percent < 0.33f)
            strcpy(bar, AFGRN);
        else if (percent < 0.66f)
            strcpy(bar, AFYEL);
        else
            strcpy(bar, AFRED);
        p += strlen(bar);
    }

    for (i = 0; i < length; ++i) {
        if (total == 0 || current == 0 || i > distance)
            *(p++) = '-';
        else if (i == distance)
            p += sprintf(p, "%s*%s", color ? FWHT : "", color ? FBLU : "");
        else
            *(p++) = '=';
    }

    if (color)
        strcpy(p, ANRM);

    return bar;
}

static void show_abilities(struct char_data *ch, struct char_data *tch, bool verbose) {
    if (verbose)
        sprintf(buf,
                "Str: &3%s&0    Int: &3%s&0     Wis: &3%s&0\r\n"
                "Dex: &3%s&0    Con: &3%s&0     Cha: &3%s&0\r\n",
                ability_message(GET_VIEWED_STR(tch)), ability_message(GET_VIEWED_INT(tch)),
                ability_message(GET_VIEWED_WIS(tch)), ability_message(GET_VIEWED_DEX(tch)),
                ability_message(GET_VIEWED_CON(tch)), ability_message(GET_VIEWED_CHA(tch)));
    else
        sprintf(buf,
                "Str: &3&b%d&0(&3%d&0)     %s%s"
                "Int: &3&b%d&0(&3%d&0)     %s%s"
                "Wis: &3&b%d&0(&3%d&0)\r\n"
                "Dex: &3&b%d&0(&3%d&0)     %s%s"
                "Con: &3&b%d&0(&3%d&0)     %s%s"
                "Cha: &3&b%d&0(&3%d&0)\r\n",
                GET_VIEWED_STR(tch), GET_NATURAL_STR(tch), GET_VIEWED_STR(tch) < 100 ? " " : "",
                GET_NATURAL_STR(tch) < 100 ? " " : "", GET_VIEWED_INT(tch), GET_NATURAL_INT(tch),
                GET_VIEWED_INT(tch) < 100 ? " " : "", GET_NATURAL_INT(tch) < 100 ? " " : "", GET_VIEWED_WIS(tch),
                GET_NATURAL_WIS(tch), GET_VIEWED_DEX(tch), GET_NATURAL_DEX(tch), GET_VIEWED_DEX(tch) < 100 ? " " : "",
                GET_NATURAL_DEX(tch) < 100 ? " " : "", GET_VIEWED_CON(tch), GET_NATURAL_CON(tch),
                GET_VIEWED_CON(tch) < 100 ? " " : "", GET_NATURAL_CON(tch) < 100 ? " " : "", GET_VIEWED_CHA(tch),
                GET_NATURAL_CHA(tch));
    send_to_char(buf, ch);
}

static void show_points(struct char_data *ch, struct char_data *tch, bool verbose) {
    sprintf(buf,
            "Hit points: &1&b%d&0[&1%d&0] (&3%d&0)   Moves: &2&b%d&0[&2%d&0] "
            "(&3%d&0)\r\n",
            GET_HIT(tch), GET_MAX_HIT(tch), GET_BASE_HIT(tch), GET_MOVE(tch), GET_MAX_MOVE(tch), natural_move(tch));

    if (verbose)
        sprintf(buf, "%sArmor class: &3&8%s&0 (&3&8%d&0)  ", buf,
                armor_message(GET_AC(tch) + 5 * monk_weight_penalty(tch)), GET_AC(tch) + 5 * monk_weight_penalty(tch));
    else
        sprintf(buf, "%sArmor class: &3&8%d&0  ", buf, GET_AC(tch) + 5 * monk_weight_penalty(tch));

    if (verbose)
        sprintf(buf,
                "%sHitroll: &3&8%s&0 (&3&8%d&0)  "
                "Damroll: &3&8%s&0 (&3&8%d&0)\r\n",
                buf, hitdam_message(GET_HITROLL(tch) - monk_weight_penalty(tch)),
                GET_HITROLL(tch) - monk_weight_penalty(tch),
                hitdam_message(GET_DAMROLL(tch) - monk_weight_penalty(tch)),
                GET_DAMROLL(tch) - monk_weight_penalty(tch));
    else
        sprintf(buf, "%sHitroll: &3&b%d&0  Damroll: &3&b%d&0", buf, GET_HITROLL(tch) - monk_weight_penalty(tch),
                GET_DAMROLL(tch) - monk_weight_penalty(tch));

    if (GET_RAGE(tch) || GET_SKILL(tch, SKILL_BERSERK))
        sprintf(buf, "%s\r\nAnger: &3&b%d&0", buf, GET_RAGE(tch));

    sprintf(buf, "%s  Perception: &3&b%ld&0  Concealment: &3&b%ld&0\r\n", buf, GET_PERCEPTION(tch),
            GET_HIDDENNESS(tch));

    send_to_char(buf, ch);
}

static void show_alignment(struct char_data *ch, struct char_data *tch, bool verbose) {
    if (verbose)
        sprintf(buf, "Alignment: %s%s&0 (%s%d&0)  ", IS_EVIL(tch) ? "&1&b" : (IS_GOOD(tch) ? "&3&b" : "&2&b"),
                align_message(GET_ALIGNMENT(tch)), IS_EVIL(tch) ? "&1&b" : (IS_GOOD(tch) ? "&3&b" : "&2&b"),
                GET_ALIGNMENT(tch));
    else
        sprintf(buf, "Alignment: %s%d&0  ", IS_EVIL(tch) ? "&1&b" : (IS_GOOD(tch) ? "&3&b" : "&2&b"),
                GET_ALIGNMENT(tch));
    send_to_char(buf, ch);
}

static void show_load(struct char_data *ch, struct char_data *tch, bool verbose) {
    if (verbose)
        sprintf(buf, "Encumbrance: &3&8%s&0 (&3&8%.2f&0/&3%d&0 lb)  ",
                (CURRENT_LOAD(tch) >= 0 && CURRENT_LOAD(tch) <= 10) ? carry_desc[CURRENT_LOAD(tch)]
                                                                    : "Your load is ridiculous!",
                IS_CARRYING_W(tch), CAN_CARRY_W(tch));
    else
        sprintf(buf, "Encumbrance: &3&8%.2f&0/&3%d&0 lb  ", IS_CARRYING_W(tch), CAN_CARRY_W(tch));
    send_to_char(buf, ch);
}

static void show_saves(struct char_data *ch, struct char_data *tch, bool verbose) {
    if (verbose)
        sprintf(buf,
                "Saving throws: PAR[&3&8%s&0/&3&8%d&0] "
                "ROD[&3&8%s&0/&3&8%d&0] PET[&3&8%s&0/&3&8%d&0] "
                "BRE[&3&b%s&0/&3&8%d&0] SPE[&3&b%s&0/&3&8%d&0]\r\n",
                save_message(GET_SAVE(tch, 0)), GET_SAVE(tch, 0), save_message(GET_SAVE(tch, 1)), GET_SAVE(tch, 1),
                save_message(GET_SAVE(tch, 2)), GET_SAVE(tch, 2), save_message(GET_SAVE(tch, 3)), GET_SAVE(tch, 3),
                save_message(GET_SAVE(tch, 4)), GET_SAVE(tch, 4));
    else
        sprintf(buf,
                "Saving throws: PAR[&3&b%d&0]  ROD[&3&b%d&0]  "
                "PET[&3&b%d&0]  BRE[&3&b%d&0]  SPE[&3&b%d&0]\r\n",
                GET_SAVE(tch, 0), GET_SAVE(tch, 1), GET_SAVE(tch, 2), GET_SAVE(tch, 3), GET_SAVE(tch, 4));
    send_to_char(buf, ch);
}

#define COIN_FMT "%s%d " ANRM "%s"
#define PURSE_ELEMS(ch, coin) COIN_COLOR(coin), GET_PURSE_COINS(ch, coin), capitalize(COIN_NAME(coin))
#define BANK_ELEMS(ch, coin) COIN_COLOR(coin), GET_ACCOUNT_COINS(ch, coin), capitalize(COIN_NAME(coin))

static void show_coins(struct char_data *ch, struct char_data *tch) {
    cprintf(ch,
            "Coins carried: " COIN_FMT ", " COIN_FMT ", " COIN_FMT ", and " COIN_FMT EOL "Coins in bank: " COIN_FMT
            ", " COIN_FMT ", " COIN_FMT ", and " COIN_FMT EOL,
            PURSE_ELEMS(tch, PLATINUM), PURSE_ELEMS(tch, GOLD), PURSE_ELEMS(tch, SILVER), PURSE_ELEMS(tch, COPPER),
            BANK_ELEMS(tch, PLATINUM), BANK_ELEMS(tch, GOLD), BANK_ELEMS(tch, SILVER), BANK_ELEMS(tch, COPPER));
}

static void show_conditions(struct char_data *ch, struct char_data *tch, bool verbose) {
    buf[0] = '\0';
    if (verbose) {
        if (GET_COND(tch, FULL) < 6 && GET_COND(tch, FULL) > -1) {
            if (!GET_COND(tch, FULL))
                strcat(buf, "You are hungry.");
            else
                strcat(buf, "You're a little hungry.");
            if (GET_COND(tch, THIRST) < 6 && GET_COND(tch, THIRST) > -1)
                strcat(buf, "  ");
            else
                strcat(buf, "\r\n");
        }
        if (GET_COND(tch, THIRST) < 6 && GET_COND(tch, THIRST) > -1) {
            if (!GET_COND(tch, THIRST))
                strcat(buf, "You are thirsty.\r\n");
            else
                strcat(buf, "You're a little thirsty.\r\n");
        }
        if (GET_COND(tch, DRUNK) > 6)
            strcat(buf, "You are drunk.\r\n");
        else if (GET_COND(tch, DRUNK) > 0)
            strcat(buf, "You're a little bit intoxicated.\r\n");
    } else
        /* the number 0 looks like the letter O */
        sprintf(buf, "Hunger: %d%s  Thirst: %d%s  Drunkenness: %d%s\r\n",
                GET_COND(tch, FULL) < 0 ? 0 : 24 - GET_COND(tch, FULL), GET_COND(tch, FULL) < 0 ? "ff" : "",
                GET_COND(tch, THIRST) < 0 ? 0 : 24 - GET_COND(tch, THIRST), GET_COND(tch, THIRST) < 0 ? "ff" : "",
                GET_COND(tch, DRUNK) < 0 ? 0 : GET_COND(tch, DRUNK), GET_COND(tch, DRUNK) < 0 ? "ff" : "");
    send_to_char(buf, ch);
}

static void show_exp(struct char_data *ch, struct char_data *tch) {
    sprintf(buf, "Exp: %s\r\n", exp_message(tch));
    if (!IS_STARSTAR(tch) && GET_LEVEL(tch) > 0)
        sprintf(buf + strlen(buf), "   <%s>\r\n", exp_bar(tch, 60, 20, 20, COLOR_LEV(ch) >= C_NRM));
    send_to_char(buf, ch);
}

static void show_active_spells(struct char_data *ch, struct char_data *tch) {
    struct effect *eff;

    if (tch->effects) {
        strcpy(buf,
               "\r\n"
               "Active Spell/Status Effects\r\n"
               "===========================\r\n");
        for (eff = tch->effects; eff; eff = eff->next)
            if (eff->duration >= 0 && (!eff->next || eff->next->type != eff->type)) {
                strcat(buf, "   ");
                strcat(buf, skills[eff->type].name);
                if (EFF_FLAGGED(ch, EFF_DETECT_MAGIC)) {
                    if (eff->duration <= 1)
                        strcat(buf, " (&1fading rapidly&0)");
                    else if (eff->duration <= 3)
                        strcat(buf, " (&1&bfading&0)");
                }
                strcat(buf, "\r\n");
            }
        send_to_char(buf, ch);
    }
}

ACMD(do_experience) {
    cprintf(ch, "%s\r\n<%s>\r\n", exp_message(REAL_CHAR(ch)),
            exp_bar(REAL_CHAR(ch), 60, 20, 20, COLOR_LEV(ch) >= C_NRM));
}

ACMD(do_score) {
    struct time_info_data playing_time;
    struct time_info_data real_time_passed(time_t t2, time_t t1);
    struct char_data *tch;

    one_argument(argument, arg);

    if ((GET_LEVEL(ch) >= LVL_IMMORT) && (*arg)) {
        if (!(tch = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
            send_to_char("There's nobody here by that name.\r\n", ch);
            return;
        }
    } else
        tch = ch;

    str_start(buf, sizeof(buf));

    str_catf(buf, "@0%-*sCharacter attributes for %s\r\n\r\n", MAX(0, (45 - ansi_strlen(GET_NAME(tch))) / 2), "",
             GET_NAME(tch));

    if (EFF_FLAGGED(tch, EFF_ON_FIRE))
        str_cat(buf,
                "                     &3*&1*&3&b* "
                "&0&7&bYou are on &1&bFIRE&7! &3*&0&1*&3*\r\n\r\n");

    if (IS_STARSTAR(tch))
        sprintf(buf1, " &9&b{&0%s&9&b}&0", CLASS_STARS(tch));
    str_catf(buf, "Level: @Y%d@0%s  Class: %s", GET_LEVEL(tch), IS_STARSTAR(tch) ? buf1 : "", CLASS_FULL(tch));

    sprinttype(GET_SEX(tch), genders, buf1);
    if (GET_COMPOSITION(tch) != COMP_FLESH || BASE_COMPOSITION(tch) != COMP_FLESH || GET_LIFEFORCE(tch) != LIFE_LIFE) {
        if (GET_COMPOSITION(tch) == BASE_COMPOSITION(tch))
            *buf2 = '\0';
        else if (!VALID_COMPOSITIONNUM(BASE_COMPOSITION(tch)))
            strcpy(buf2, "@YInvalid!@0");
        else
            sprintf(buf2, "%s%c%s", compositions[BASE_COMPOSITION(tch)].color,
                    UPPER(*(compositions[BASE_COMPOSITION(tch)].name)), compositions[BASE_COMPOSITION(tch)].name + 1);
        str_catf(buf,
                 "  Size: &3&8%c%s&0  Sex: &3&8%s&0\r\n"
                 "Race: %s  Life force: %s%c%s&0  "
                 "Composition: %s%c%s&0%s%s&0%s\r\n",
                 UPPER(*SIZE_DESC(tch)), SIZE_DESC(tch) + 1, buf1, RACE_ABBR(tch), LIFEFORCE_COLOR(tch),
                 UPPER(*LIFEFORCE_NAME(tch)), LIFEFORCE_NAME(tch) + 1, COMPOSITION_COLOR(tch),
                 UPPER(*COMPOSITION_NAME(tch)), COMPOSITION_NAME(tch) + 1, *buf2 ? "(" : "", buf2, *buf2 ? ")" : "");
    } else
        str_catf(buf, "  Race: %s  Size: &3&8%c%s&0  Sex: &3&8%s&0\r\n", RACE_ABBR(tch), UPPER(*SIZE_DESC(tch)),
                 SIZE_DESC(tch) + 1, buf1);

    str_catf(buf,
             "Age: &3&b%d&0&3 year%s&0, &3&b%d&0&3 month%s&0  "
             "Height: &3&b%s&0  Weight: &3&b%s&0\r\n",
             age(tch).year, age(tch).year == 1 ? "" : "s", age(tch).month, age(tch).month == 1 ? "" : "s",
             statelength(GET_HEIGHT(tch)), stateweight(GET_WEIGHT(tch)));
    send_to_char(buf, ch);

    show_abilities(ch, tch, GET_LEVEL(ch) < 10);
    show_points(ch, tch, GET_LEVEL(ch) < 50);
    show_alignment(ch, tch, GET_LEVEL(ch) < 35);
    show_load(ch, tch, TRUE); /* always verbose */

    str_start(buf, sizeof(buf));

    str_cat(buf, "Status: ");

    if (GET_POS(tch) == POS_FLYING)
        str_cat(buf, "&6&bFlying&0\r\n");
    else
        switch (GET_STANCE(tch)) {
        case STANCE_DEAD:
            str_cat(buf, "&9&bDead&0\r\n");
            break;
        case STANCE_MORT:
            str_cat(buf, "&1Mortally wounded&0\r\n");
            break;
        case STANCE_INCAP:
            str_cat(buf, "&1&bIncapacitated&0\r\n");
            break;
        case STANCE_STUNNED:
            str_cat(buf, "&6&bStunned&0\r\n");
            break;
        case STANCE_SLEEPING:
            if (IN_ROOM(tch) != NOWHERE && IS_WATER(IN_ROOM(tch)))
                str_cat(buf, "&4Sleeping&0\r\n");
            else
                switch (GET_POS(tch)) {
                case POS_STANDING:
                    str_cat(buf, "&3Standing here sleeping&0\r\n");
                    break;
                case POS_KNEELING:
                    str_cat(buf, "&3Kneeling here sleeping&0\r\n");
                    break;
                case POS_SITTING:
                    str_cat(buf, "&3Sitting here sleeping&0\r\n");
                    break;
                case POS_PRONE:
                    str_cat(buf, "&3Sleeping&0\r\n");
                    break;
                default:
                    str_cat(buf, "&1&8Invalid&0\r\n");
                    break;
                }
            break;
        case STANCE_RESTING:
            if (IN_ROOM(tch) != NOWHERE && IS_WATER(IN_ROOM(tch)))
                str_cat(buf, "&4Resting&0\r\n");
            else
                switch (GET_POS(tch)) {
                case POS_STANDING:
                    str_cat(buf, "&3Standing here relaxed&0\r\n");
                    break;
                default:
                    str_cat(buf, "&3&bResting&0\r\n");
                    break;
                }
            break;
        case STANCE_ALERT:
            if (IN_ROOM(tch) != NOWHERE && IS_WATER(IN_ROOM(tch)))
                str_cat(buf, "&4Floating&0\r\n");
            else
                switch (GET_POS(tch)) {
                case POS_STANDING:
                    str_cat(buf, "&3Standing&0\r\n");
                    break;
                case POS_KNEELING:
                    str_cat(buf, "&3Kneeling&0\r\n");
                    break;
                case POS_SITTING:
                    str_cat(buf, "&3&bSitting&0\r\n");
                    break;
                case POS_PRONE:
                    str_cat(buf, "&3Lying alert&0\r\n");
                    break;
                default:
                    str_cat(buf, "&1&8Invalid&0\r\n");
                    break;
                }
            break;
        case STANCE_FIGHTING:
            str_cat(buf, "&1&bFighting&0\r\n");
            break;
        default:
            str_cat(buf, "&1&8Invalid&0\r\n");
            break;
        }

    send_to_char(buf, ch);

    show_saves(ch, tch, GET_LEVEL(ch) < 75);
    show_coins(ch, tch);

    playing_time =
        real_time_passed((time(0) - REAL_CHAR(tch)->player.time.logon) + REAL_CHAR(tch)->player.time.played, 0);
    cprintf(ch, "Playing time: &3&b%d&0&3 day%s&0 and &3&b%d&0&3 hour%s&0\r\n", playing_time.day,
            playing_time.day == 1 ? "" : "s", playing_time.hours, playing_time.hours == 1 ? "" : "s");

    show_conditions(ch, tch, TRUE);
    speech_report(ch, tch);
    if (GET_LEVEL(tch) < LVL_IMMORT)
        show_exp(ch, REAL_CHAR(tch));
    show_active_spells(ch, tch);
}

const char *proficiency_message(int proficiency) {
    if (proficiency == 0)
        return "not learned";
    else if (proficiency <= 100)
        return "awful";
    else if (proficiency <= 200)
        return "bad";
    else if (proficiency <= 400)
        return "poor";
    else if (proficiency <= 550)
        return "average";
    else if (proficiency <= 700)
        return "fair";
    else if (proficiency <= 800)
        return "good";
    else if (proficiency <= 850)
        return "very good";
    else if (proficiency <= 999)
        return "superb";
    else
        return "mastered";
}

#define MAX_CIRCLE 14
#define MAX_SPELLS_PER_CIRCLE 30

/* do_spells
 *
 * Usage:
 *
 * (I)    spells
 * (II)   spells <circle>
 * (III)  spells <name>              (wizard only)
 * (IV)   spells <name> <circle>     (wizard only)
 */

ACMD(do_spells) {
    int circle_spells[MAX_CIRCLE][MAX_SPELLS_PER_CIRCLE];
    int i, j, k, circle, skillnum, sphere, max_vis_circle, xcircle = 0, numspellsknown = 0;
    char *s_circle = NULL, *s_vict = NULL;
    struct char_data *tch;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    /* Extract the arguments of the command */
    argument = any_one_arg(argument, arg1);
    one_argument(argument, arg2);

    /* Determine the intent of the arguments */
    if (*arg1) {
        if (*arg2) {
            s_vict = arg1;
            s_circle = arg2;
        } else if (isdigit(*arg1))
            s_circle = arg1;
        else
            s_vict = arg1;
    }

    /* Determine whose spells shall be displayed */
    if (s_vict) {
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            send_to_char("That isn't a spell circle.\r\n", ch);
            return;
        }
        if (!(tch = find_char_around_char(ch, find_vis_by_name(ch, s_vict)))) {
            send_to_char("There's nobody here by that name.\r\n", ch);
            return;
        }
    } else
        tch = ch;

    /* Determine the maximum circle to be displayed */
    max_vis_circle = MIN(NUM_SPELL_CIRCLES, (GET_LEVEL(tch) - 1) / 8 + 1);

    if (s_circle) {
        xcircle = atoi(s_circle);
        if (xcircle < 1) {
            sprintf(buf, "The spell circle must be a number between 1 and %d.\r\n", max_vis_circle);
            send_to_char(buf, ch);
            return;
        } else if (xcircle > max_vis_circle) {
            if (tch == ch)
                sprintf(buf, "You only know spells up to circle %d.\r\n", max_vis_circle);
            else
                sprintf(buf, "%s only knows spells up to circle %d.\r\n", CAP(GET_NAME(tch)), max_vis_circle);
            send_to_char(buf, ch);
            return;
        }
    }

    /* Is this character in a class with spells? */
    if (MEM_MODE(tch) == MEM_NONE) {
        if (tch == ch)
            send_to_char("You don't know any spells.\r\n", ch);
        else {
            sprintf(buf, "Being a%s %s, %s has no spells.\r\n", startsvowel(CLASS_NAME(tch)) ? "n" : "",
                    CLASS_NAME(tch), GET_NAME(tch));
            send_to_char(buf, ch);
        }
        return;
    }

    /* Collect and count the spells known by the victim. */

    memset(&circle_spells, 0, sizeof(circle_spells));

    for (k = 0; k <= TOP_SKILL; ++k) {
        i = skill_sort_info[k];
        if (!IS_SPELL(i))
            continue;
        if (skills[i].min_level[GET_CLASS(tch)] < LVL_IMMORT && GET_SKILL(tch, i) > 0) {
            circle = (skills[i].min_level[GET_CLASS(tch)] - 1) / 8;
            for (j = 0; circle_spells[circle][j] && j < MAX_SPELLS_PER_CIRCLE && circle < max_vis_circle; ++j)
                ;
            if (!xcircle || xcircle == circle + 1)
                numspellsknown++;
            circle_spells[circle][j] = i;
        }
    }

    /* No spells known? */

    if (!numspellsknown) {
        if (xcircle) {
            if (tch == ch)
                sprintf(buf, "You don't know any spells in circle %d.\r\n", xcircle);
            else
                sprintf(buf, "%s doesn't know any spells in circle %d.\r\n", CAP(GET_NAME(tch)), xcircle);
        } else {
            if (tch == ch)
                sprintf(buf, "You don't know any spells.\r\n");
            else
                sprintf(buf, "%s doesn't know any spells.\r\n", CAP(GET_NAME(tch)));
        }
        send_to_char(buf, ch);
        return;
    }

    /* ALL TEXT FROM THIS POINT ON IS PAGED */

    /* Produce the introductory statement. */

    if (xcircle) {
        if (tch == ch)
            pprintf(ch, "You know of the following %s in &4&bcircle %d&0:" EOL EOL,
                    numspellsknown == 1 ? "spell" : "spells", xcircle);
        else
            pprintf(ch, "%s knows of the following %s in &4&bcircle %d&0:" EOL EOL, CAP(GET_NAME(tch)),
                    numspellsknown == 1 ? "spell" : "spells", xcircle);
    } else {
        if (tch == ch)
            pprintf(ch, "You know of the following %s:" EOL EOL, numspellsknown == 1 ? "spell" : "spells");
        else
            pprintf(ch, "%s knows of the following %s:" EOL EOL, GET_NAME(tch),
                    numspellsknown == 1 ? "spell" : "spells");
    }

    /* List the spells. */

    for (i = 0; i < max_vis_circle; ++i) {
        if (circle_spells[i][0]) {
            if (xcircle && xcircle != i + 1)
                continue;
            for (j = 0; circle_spells[i][j]; ++j) {
                if (j == 0 && !xcircle)
                    sprintf(buf, "&4&bCircle %2d&0", i + 1);
                else
                    sprintf(buf, "         ");
                skillnum = circle_spells[i][j];
                sphere = skill_to_sphere(skillnum);
                pprintf(ch, "%s  %s " ELLIPSIS_FMT " %s%-15s&0" EOL, buf, skills[skillnum].quest ? "&6*&0" : " ",
                        ELLIPSIS_STR(skills[skillnum].name, 25), spheres[sphere].color, spheres[sphere].name);
            }
        }
    }

    start_paging(ch);
}

/* This command has two operating modes.
 *
 * Usage: llistspells <class>
 *        listspells all | circles
 *
 * Specify a class to see all spells for that class.  103's and up can use
 * the "listspells all", which will list all the spells and their levels
 * for each class.  Also, "listspells circle" will list the spells and
 * their circles for each class.
 */

ACMD(do_listspells) {
    int circle_spells[MAX_CIRCLE][MAX_SPELLS_PER_CIRCLE];
    int i, j, k, circle, class, page_length;
    int magic_class_index[NUM_CLASSES], num_magic_classes;
    /* 20000 bytes is not enough: */
    char mybuf[50000];

    if (!ch || IS_NPC(ch))
        return;

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char("Usage: listspells <class> | all | circles\r\n", ch);
        return;
    }

    class = parse_class(0, 0, arg);

    /* Figure out the magical classes */
    num_magic_classes = 0;
    for (i = 0; i < NUM_CLASSES; i++) {
        if (classes[i].magical) {
            magic_class_index[num_magic_classes++] = i;
        }
    }

    if (!num_magic_classes) {
        send_to_char("There are no spellcasting classes.\r\n", ch);
        return;
    }

    /*
     * The argument wasn't a class.   Try and see if it's "all" or "circles"
     * to list all spells.
     */
    if (class == CLASS_UNDEFINED) {
        circle = is_abbrev(arg, "circles");
        if (GET_LEVEL(ch) >= LVL_HEAD_B && (circle || is_abbrev(arg, "all"))) {
            *mybuf = '\0';
            page_length = GET_PAGE_LENGTH(REAL_CHAR(ch));
            for (k = 0, j = 0; k <= TOP_SKILL; ++k) {
                i = skill_sort_info[k];
                if (!IS_SPELL(i))
                    continue;
                if (*skills[i].name == '!')
                    continue;
                /* Tack on the table header for each page. */
                if (!(page_length > 1 ? (j % (page_length - 1)) : j)) {
                    strcat(mybuf, "&4&uSpell               &0&u");
                    for (class = 0; class < num_magic_classes; ++class)
                        sprintf(mybuf, "%s %2.2s", mybuf, strip_ansi(classes[magic_class_index[class]].abbrev));
                    strcat(mybuf, "&0\r\n");
                }
                ++j;

                sprintf(mybuf, "%s%-20.20s ", mybuf, skills[i].name);
                for (class = 0; class < num_magic_classes; ++class)
                    if (skills[i].min_level[magic_class_index[class]] < LVL_IMMORT) {
                        if (circle)
                            sprintf(mybuf, "%s&3%2d ", mybuf,
                                    (skills[i].min_level[magic_class_index[class]] - 1) / 8 + 1);
                        else
                            sprintf(mybuf, "%s&3%2d ", mybuf, skills[i].min_level[magic_class_index[class]]);
                    } else
                        strcat(mybuf, "&0 0 ");
                strcat(mybuf, "&0\r\n");
            }
            page_string(ch, mybuf);
        } else
            send_to_char("Invalid class.\r\n", ch);
        return;
    }

    /* A class was specified. */
    memset(&circle_spells, 0, sizeof(circle_spells));

    *mybuf = '\0';

    for (i = 1; i <= MAX_SPELLS; ++i)
        if (skills[i].min_level[class] < LVL_IMMORT) {
            circle = (skills[i].min_level[class] - 1) / 8;
            for (j = 0; circle_spells[circle][j] && j < MAX_SPELLS_PER_CIRCLE && circle < MAX_CIRCLE; ++j)
                ;
            circle_spells[circle][j] = i;
        }

    for (i = 0; i < MAX_CIRCLE; ++i) {
        sprintf(mybuf, "%s&4&bCircle %d:&0\r\n", mybuf, i + 1);
        for (j = 0; circle_spells[i][j]; ++j)
            sprintf(mybuf, "%s%s%s\r\n", mybuf, skills[circle_spells[i][j]].name,
                    skills[circle_spells[i][j]].quest ? "*" : "");
        strcat(mybuf, "\r\n");
    }

    page_string(ch, mybuf);
}

ACMD(do_skills) {
    int level_max_skill(struct char_data * ch, int level, int skill);
    const char *fullmastered = " &7-&b*&0&7-&0";
    const char *mastered = "  * ";
    const char *normal = "    ";
    const char *mastery;
    int i, x;
    struct char_data *tch;
    char points[MAX_INPUT_LENGTH];
    bool godpeek;

    one_argument(argument, arg);

    if ((GET_LEVEL(ch) >= LVL_IMMORT) && (*arg)) {
        if ((tch = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
            if (IS_NPC(tch)) {
                send_to_char("Sadly, this does not work for NPCs.\r\n", ch);
                return;
            }
        } else {
            send_to_char("There's nobody here by that name.\r\n", ch);
            return;
        }
        sprintf(buf, "%s knows of the following skills:\r\n", GET_NAME(tch));
        godpeek = TRUE;
    } else {
        strcpy(buf, "You know of the following skills:\r\n");
        tch = ch;
        godpeek = FALSE;
    }

    strcpy(buf2, buf);
    points[0] = '\0';

    /* do_skills crashes when performed by non-switched mobs. */
    if (IS_NPC(ch) && !ch->desc)
        return;

    for (x = 0; x <= TOP_SKILL; ++x) {
        i = skill_sort_info[x];
        if (!IS_SKILL(i))
            continue;
        if (*skills[i].name == '!')
            continue;
        if (GET_SKILL(tch, i) <= 0)
            continue;

        if (godpeek)
            sprintf(points, " (%4d/%4d/%4d)", GET_ISKILL(tch, i), return_max_skill(tch, i),
                    level_max_skill(tch, LVL_MAX_MORT, i));

        /* Show a star if the skill is as good as possible. */
        if (GET_ISKILL(tch, i) >= level_max_skill(tch, LVL_MAX_MORT, i))
            mastery = fullmastered;
        else if (GET_ISKILL(tch, i) >= return_max_skill(tch, i))
            mastery = mastered;
        else
            mastery = normal;

        /* Show exact skill numbers of mobs when switched. */
        sprintf(buf1, "(%s)", proficiency_message(GET_ISKILL(tch, i)));
        if (POSSESSED(tch) && GET_LEVEL(POSSESSOR(tch)) >= LVL_IMMORT) {
            sprintf(buf, "%-22s %15s [%4d]%s%s\r\n", skills[i].name, buf1, GET_ISKILL(tch, i), mastery, points);
        } else {
            sprintf(buf, "%-22s %15s%s%s\r\n", skills[i].name, buf1, mastery, points);
        }
        strcat(buf2, buf);
    }
    page_string(ch, buf2);
}

static int scan_chars(struct char_data *ch, int room, int dis, int dir, int seen_any) {
    struct char_data *i;
    int count = 0;

    /*
     * Only the first 5 are currently used by do_scan (distance 0 through 4)
     * but it's an easy change to set up arbitrary-length scanning.   Just
     * change the maxdis values in do_scan and add any necessary distance
     * values here.
     */
    const char *distance[] = {
        "right", "immediately", "close by", "a ways off", "far far", "very far", "astoundingly far", "impossibly far",
    };

    for (i = world[room].people; i; i = i->next_in_room) {
        if (i == ch)
            continue;
        /*
         * This is mostly a duplication of parts of CAN_SEE, omitting
         * the LIGHT_OK code, since we want to be able to see people
         * in darkened rooms when we scan.
         */
        if (!(((GET_LEVEL(REAL_CHAR(ch)) >= GET_INVIS_LEV(i) && (INVIS_OK(ch, i) || PRF_FLAGGED(ch, PRF_HOLYLIGHT)))) ||
              IMM_VIS_OK(ch, i)))
            continue;

        if (!(seen_any + count)) {
            send_to_char("You scan the area, and see:\r\n", ch);
        }

        ++count;
        /*
              sprintf(buf, "   " ELLIPSIS_FMT ANRM " (%s" ANRM ") : %s %s\r\n",
                          ELLIPSIS_STR(GET_NAME(i), 25),
                          GET_POS(i) == POS_FLYING ? FCYN "fly" :
                             (GET_POS(i) > POS_SITTING ? "std" : FGRN "sit"),
                          distance[dis], dis ? dirs[dir] : "");
              send_to_char(buf, ch);
        */
        sprintf(buf, "%s %s", distance[dis], dis ? dirs[dir] : "here");
        cprintf(ch, "   %22s : %s" ANRM " (%s" ANRM ")\r\n", buf, GET_NAME(i),
                GET_POS(i) == POS_FLYING ? FCYN "fly" : (GET_POS(i) > POS_SITTING ? "std" : FGRN "sit"));
    }

    return count;
}

/* This tests for conditions that will stop a scan entirely.
 * If it returns false, you can't see through the exit at all. */
bool exit_is_scannable(struct char_data *ch, struct exit *exit) {

    if (!exit)
        return FALSE;

    if (!EXIT_IS_OPEN(exit))
        return FALSE;

    if (EXIT_IS_HIDDEN(exit) && GET_LEVEL(ch) < LVL_IMMORT)
        return FALSE;

    if (EXIT_NDEST(exit) == NOWHERE)
        return FALSE;

    if (EXIT_DEST(exit) == NULL)
        return FALSE;

    if (ROOM_FLAGGED(EXIT_NDEST(exit), ROOM_NOSCAN))
        return FALSE;

    return TRUE;
}

/* This tests for conditions that will stop you from scanning in a
 * specific room. But even if it returns false, you might be able
 * to scan beyond this room. */
bool room_is_scannable(struct char_data *ch, int room) {

    if (room == NOWHERE)
        return FALSE;

    /* Note that we DO want people to be able to scan creatures who
     * are in dark rooms. Though it's unrealistic, scan is just too
     * useless when that isn't allowed. */
    return TRUE;
}

ACMD(do_scan) {
    int dir, only_dir = -1, dis, maxdis, found = 0;
    int from_room, to_room;
    struct exit *exit;

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (EFF_FLAGGED(ch, EFF_BLIND)) {
            send_to_char(YOU_ARE_BLIND, ch);
            return;
        }
        if (ROOM_FLAGGED(ch->in_room, ROOM_NOSCAN)) {
            send_to_char("That's impossible in this environment!\r\n", ch);
            return;
        }
    }

    any_one_arg(argument, arg);
    if (*arg && (only_dir = searchblock(arg, dirs, FALSE)) == -1) {
        send_to_char("That is not a direction.\r\n", ch);
        return;
    }

    /*
     * You can have arbitrary-length scanning here, but you have to modify
     * the distance array in scan_chars to have maxdis + 1 entries.
     */
    if (GET_LEVEL(ch) >= LVL_IMMORT)
        maxdis = 3;
    else if (GET_CLASS(ch) == CLASS_ASSASSIN || GET_CLASS(ch) == CLASS_ROGUE)
        maxdis = 3;
    else
        maxdis = 1;

    if (EFF_FLAGGED(ch, EFF_FARSEE)) {
        ++maxdis;
        /* Yes only casters will be able to scan really far from farsee. */
        if (number(33, 75) <= GET_SKILL(ch, SKILL_SPHERE_DIVIN))
            ++maxdis;
        if (number(75, 150) <= GET_SKILL(ch, SKILL_SPHERE_DIVIN))
            ++maxdis;
        if (GET_CLASS(ch) == CLASS_ASSASSIN || GET_CLASS(ch) == CLASS_ROGUE)
            ++maxdis;
    }

    if (ROOM_EFF_FLAGGED(CH_NROOM(ch), ROOM_EFF_ISOLATION) && GET_LEVEL(ch) < LVL_IMMORT)
        maxdis = 0;

    act("$n scans the area.", TRUE, ch, 0, 0, TO_ROOM);

    found = scan_chars(ch, ch->in_room, 0, NORTH, found);

    for (dir = 0; dir < NUM_OF_DIRS; ++dir) {
        if (only_dir == -1 || only_dir == dir) {
            from_room = ch->in_room;
            for (dis = 1; dis <= maxdis; ++dis) {
                if ((exit = world[from_room].exits[dir]) && exit_is_scannable(ch, exit)) {
                    to_room = EXIT_NDEST(exit);
                    if (room_is_scannable(ch, to_room))
                        found += scan_chars(ch, to_room, dis, dir, found);
                    from_room = to_room;
                } else {
                    break;
                }
            }
        }
    }

    if (!found)
        send_to_char("You don't see anyone.\r\n", ch);
}

ACMD(do_innate) {
    /*struct char_data *vict; */
    *buf = '\0';

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char("You have the following innate skills and effects:\r\n", ch);
        if (GET_SKILL(ch, SKILL_BODYSLAM))
            send_to_char(" bodyslam\r\n", ch);
        if (GET_SKILL(ch, SKILL_BREATHE))
            send_to_char(" breathe\r\n", ch);
        if (GET_RACE(ch) == RACE_DROW)
            send_to_char(" darkness\r\n", ch);
        if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_DIABOLIST || GET_CLASS(ch) == CLASS_PALADIN ||
            GET_CLASS(ch) == CLASS_ANTI_PALADIN)
            send_to_char(" detect alignment*\r\n", ch);
        if (GET_SKILL(ch, SKILL_DOORBASH))
            send_to_char(" doorbash\r\n", ch);
        if (GET_RACE(ch) == RACE_ELF || GET_RACE(ch) == RACE_DWARF || GET_RACE(ch) == RACE_HALFLING ||
            GET_RACE(ch) == RACE_HALF_ELF || GET_RACE(ch) == RACE_GNOME)
            send_to_char(" infravision*\r\n", ch);
        if (GET_RACE(ch) == RACE_DUERGAR)
            send_to_char(" invisible\r\n", ch);
        if (GET_RACE(ch) == RACE_DROW)
            send_to_char(" levitate*\r\n", ch);
        if (GET_CLASS(ch) == CLASS_PALADIN)
            send_to_char(" protection from evil*\r\n", ch);
        if (GET_CLASS(ch) == CLASS_ANTI_PALADIN)
            send_to_char(" protection from good*\r\n", ch);
        if (GET_SKILL(ch, SKILL_ROAR))
            send_to_char(" roar\r\n", ch);
        if (GET_RACE(ch) == RACE_HALFLING)
            send_to_char(" sense life*\r\n", ch);
        if (GET_RACE(ch) == RACE_DWARF || GET_RACE(ch) == RACE_DUERGAR)
            send_to_char(" strength\r\n", ch);
        if (GET_SKILL(ch, SKILL_SWEEP))
            send_to_char(" sweep\r\n", ch);
        if (GET_RACE(ch) == RACE_DROW || GET_RACE(ch) == RACE_DUERGAR || GET_RACE(ch) == RACE_TROLL ||
            GET_RACE(ch) == RACE_OGRE)
            send_to_char(" ultravision*\r\n", ch);
        send_to_char("Effects marked with a * are always present.\r\n", ch);
    } else {
        if (is_abbrev(arg, "bodyslam") && GET_SKILL(ch, SKILL_BODYSLAM)) {
            send_to_char("Usage: bodyslam <victim>\r\n", ch);
            return;
        }

        if (is_abbrev(arg, "breathe") && GET_SKILL(ch, SKILL_BREATHE)) {
            send_to_char("Usage: breathe <fire|gas|frost|acid|lightning>\r\n", ch);
            return;
        }

        if (is_abbrev(arg, "doorbash") && GET_SKILL(ch, SKILL_DOORBASH)) {
            send_to_char("Usage: doorbash <direction>\r\n", ch);
            return;
        }

        if (is_abbrev(arg, "darkness")) {
            if (GET_RACE(ch) == RACE_DROW) {
                if (!ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS) && !ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_FOG)) {
                    if (!GET_COOLDOWN(ch, CD_INNATE_DARKNESS)) {
                        call_magic(ch, ch, 0, SPELL_DARKNESS, GET_LEVEL(ch), CAST_SPELL);
                        if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                            SET_COOLDOWN(ch, CD_INNATE_DARKNESS, 7 MUD_HR);
                    } else
                        send_to_char("You're too tired right now.\r\n", ch);
                } else
                    send_to_char("The room is pretty damn dark already!\r\n", ch);
                return;
            }
        }

        if (is_abbrev(arg, "detect") || is_abbrev(arg, "infravision") || is_abbrev(arg, "ultravision")) {
            send_to_char("This innate is always present.\r\n", ch);
            return;
        }

        if (is_abbrev(arg, "invisible")) {
            if (GET_RACE(ch) == RACE_DUERGAR) {
                if (EFF_FLAGGED(ch, EFF_INVISIBLE))
                    send_to_char("You already invisible.\r\n", ch);
                else if (!GET_COOLDOWN(ch, CD_INNATE_INVISIBLE)) {
                    call_magic(ch, ch, 0, SPELL_INVISIBLE, GET_LEVEL(ch), CAST_SPELL);
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                        SET_COOLDOWN(ch, CD_INNATE_INVISIBLE, 9 MUD_HR);
                } else
                    send_to_char("You're too tired right now.\r\n", ch);
                return;
            }
        }

        if (is_abbrev(arg, "levitate")) {
            if (GET_RACE(ch) == RACE_DROW) {
                if (EFF_FLAGGED(ch, EFF_LEVITATE))
                    send_to_char("You already levitating.\r\n", ch);
                else if (!GET_COOLDOWN(ch, CD_INNATE_LEVITATE)) {
                    call_magic(ch, ch, 0, SPELL_LEVITATE, GET_LEVEL(ch), CAST_SPELL);
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                        SET_COOLDOWN(ch, CD_INNATE_LEVITATE, 9 MUD_HR);
                } else
                    send_to_char("You're too tired right now.\r\n", ch);
                return;
            }
        }

        if (is_abbrev(arg, "roar") && GET_SKILL(ch, SKILL_ROAR)) {
            send_to_char("Usage: roar\r\n", ch);
            return;
        }

        if (is_abbrev(arg, "protection") || is_abbrev(arg, "sense")) {
            send_to_char("That innate is always present.\r\n", ch);
            return;
        }

        if (is_abbrev(arg, "strength")) {
            if (GET_RACE(ch) == RACE_DWARF || GET_RACE(ch) == RACE_DUERGAR) {
                if (!GET_COOLDOWN(ch, CD_INNATE_STRENGTH)) {
                    call_magic(ch, ch, 0, SPELL_INN_STRENGTH, GET_LEVEL(ch), CAST_SPELL);
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                        SET_COOLDOWN(ch, CD_INNATE_STRENGTH, 7 MUD_HR);
                } else
                    send_to_char("You're too tired right now.\r\n", ch);
                return;
            }
        }

        if (is_abbrev(arg, "sweep") && GET_SKILL(ch, SKILL_SWEEP)) {
            send_to_char("Usage: sweep\r\n", ch);
            return;
        }

        send_to_char("You have no such innate.\r\n", ch);
    }
}

ACMD(do_songs) {
    int i;
    bool found = FALSE;
    struct char_data *tch = ch;

    if (GET_SKILL(ch, SKILL_CHANT) < 1) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    one_argument(argument, arg);
    if (GET_LEVEL(ch) >= LVL_IMMORT && *arg) {
        if (!(tch = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
            send_to_char(NOPERSON, ch);
            return;
        }
    }

    if (ch == tch)
        strcpy(buf, "You know the following songs:\r\n");
    else
        sprintf(buf, "%c%s knows the following songs:\r\n", UPPER(*GET_NAME(tch)), GET_NAME(tch) + 1);

    for (i = MAX_SKILLS + 1; i <= MAX_CHANTS; ++i) {
        if (*skills[i].name == '!')
            continue;
        if (GET_SKILL(tch, i) <= 0)
            continue;
        sprintf(buf, "%s  %s\r\n", buf, skills[i].name);
        found = TRUE;
    }

    if (found)
        page_string(ch, buf);
    else
        send_to_char("You don't know any songs!\r\n", ch);
}

ACMD(do_last_tells) {
    struct char_data *tch;

    one_argument(argument, arg);

    if (GET_LEVEL(ch) >= LVL_IMMORT && *arg) {
        if (!(tch = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
            cprintf(ch, "There's nobody here by that name.\r\n");
            return;
        }
    } else {
        tch = ch;
    }

    show_retained_comms(ch, tch, TYPE_RETAINED_TELLS);
}

ACMD(do_last_gossips) {
    struct char_data *tch;

    one_argument(argument, arg);

    if (GET_LEVEL(ch) >= LVL_IMMORT && *arg) {
        if (!(tch = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
            cprintf(ch, "There's nobody here by that name.\r\n");
            return;
        }
    } else {
        tch = ch;
    }

    show_retained_comms(ch, tch, TYPE_RETAINED_GOSSIPS);
}

/***************************************************************************
 * $Log: act.informative.c,v $
 * Revision 1.339  2010/06/30 21:22:20  mud
 * Fix division by zero bug in string_status.
 *
 * Revision 1.338  2010/06/05 14:56:27  mud
 * Moving cooldowns to their own file.
 *
 * Revision 1.337  2009/07/16 19:15:54  myc
 * Moved command stuff from grant.c to commands.c
 *
 * Revision 1.336  2009/06/20 23:23:02  myc
 * Detect magic makes creatures with magic life force apparent
 * with a (magic) flag.
 *
 * Revision 1.335  2009/06/09 05:30:23  myc
 * Moved the old text file code in text.[ch] to textfiles.[ch].
 *
 * Revision 1.334  2009/03/21 19:11:37  myc
 * Add cooldown bar to prompt.
 *
 * Revision 1.333  2009/03/21 18:07:44  jps
 * Explain senses_living parameter
 *
 * Revision 1.332  2009/03/20 23:26:26  myc
 * Fix order of coins in score.
 *
 * Revision 1.331  2009/03/20 23:02:59  myc
 * Move text file handling routines into text.c
 *
 * Revision 1.330  2009/03/20 14:26:29  jps
 * Changed score's printout of cash.
 *
 * Revision 1.329  2009/03/20 06:10:14  jps
 * Add comment about scanning to dark rooms.
 *
 * Revision 1.328  2009/03/19 23:57:18  jps
 * Allow scanning of mobs in dark rooms, even if you can't see in the dark.
 *
 * Revision 1.327  2009/03/17 09:02:46  jps
 * Changed the output of the 'spells' command and added sphere
 * identification to each spell.
 *
 * Revision 1.326  2009/03/16 09:43:49  jps
 * Change RACE_DROW_ELF to RACE_DROW
 *
 * Revision 1.325  2009/03/15 20:41:33  jps
 * Update scanning. Can't scan through hidden exits.
 *
 * Revision 1.324  2009/03/15 07:22:02  jps
 * Fix color code in 'hovering'
 *
 * Revision 1.323  2009/03/15 07:09:24  jps
 * Add !FALL flag for objects
 *
 * Revision 1.322  2009/03/14 18:47:13  jps
 * Fix scan direction bug
 *
 * Revision 1.321  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.320  2009/03/09 16:57:47  myc
 * Added detect poison effect and detect align for objects.
 *
 * Revision 1.319  2009/03/09 06:29:49  myc
 * Fix alignment issues in scan.
 *
 * Revision 1.318  2009/03/09 04:50:38  myc
 * For shapechanged players, show their actual exp in the exp command.
 *
 * Revision 1.317  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.316  2009/03/09 03:45:17  jps
 * Extract some spell-mem related stuff from structs.h and put it in spell_mem.h
 *
 * Revision 1.315  2009/03/09 03:33:03  myc
 * Remove startsvowel declaration since it's already in a header.
 *
 * Revision 1.314  2009/03/09 03:14:38  myc
 * When a player is shapeshifted, show the original player's playing
 * time and experience in the score command.
 *
 * Revision 1.313  2009/03/09 02:22:32  myc
 * Added functionality for reading the new boards.  Required hacking
 * print_obj_to_char and consider_obj_exdesc to pass in additional,
 * possibly unused args.
 *
 * Revision 1.312  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.311  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.310  2009/03/07 11:31:15  jps
 * exits command won't work when foggy.
 *
 * Revision 1.309  2009/03/07 11:15:13  jps
 * Separated do_read from do_look.
 *
 * Revision 1.308  2009/03/03 19:41:50  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.307  2009/03/01 16:41:12  myc
 * Missing \r in newline when looking at character.
 *
 * Revision 1.306  2009/02/21 03:30:16  myc
 * Modified look and read to support new board system.
 *
 * Revision 1.305  2009/02/16 19:37:21  myc
 * Fix double 'look inside' message for examining item.
 *
 * Revision 1.304  2009/02/11 17:03:39  myc
 * Updating print_char_flags_to_char to show (writing) for new
 * editor flag wherever old WRITING flag was checked.
 *
 * Revision 1.303  2009/02/09 20:09:56  myc
 * Added status_string function to colorize and describe health status.
 * Replaces print_char_condition_to_char functionality.
 *
 * Revision 1.302  2009/02/08 17:21:35  myc
 * Send where command through pager.
 *
 * Revision 1.301  2009/01/22 23:39:42  myc
 * Don't stack mobs if they're fighting different targets or
 * casting/not casting.
 *
 * Revision 1.300  2009/01/17 00:28:02  myc
 * Fix possible use of uninitialized variable.
 *
 * Revision 1.299  2008/09/27 03:24:21  jps
 * Fix output of identify for weapons.
 *
 * Revision 1.298  2008/09/22 02:09:17  jps
 * Changed weight into a floating-point value. Precision is preserved to
 * the 1/100 place.
 *
 * Revision 1.297  2008/09/21 20:49:29  jps
 * Fix formatting of simple identification output.
 *
 * Revision 1.296  2008/09/20 18:10:35  jps
 * If you can't see the rider, do show the mount in the room.
 *
 * Revision 1.295  2008/09/20 06:44:02  jps
 * Put some adjustments in consider.
 *
 * Revision 1.294  2008/09/20 06:05:06  jps
 * Add macros POSSESSED and POSSESSOR. Fix crash bug when getting skills of
linkless person.
 *
 * Revision 1.293  2008/09/14 06:43:34  jps
 * Add ** to score if you've got it.
 *
 * Revision 1.292  2008/09/09 08:32:03  jps
 * Change secret door-finding message.
 *
 * Revision 1.291  2008/09/09 08:23:37  jps
 * Placed sector info into a struct and moved its macros into rooms.h.
 *
 * Revision 1.290  2008/09/07 20:05:27  jps
 * Renamed exp_to_level to exp_next_level to make it clearer what it means.
 *
 * Revision 1.289  2008/09/07 01:31:36  jps
 * Fix formatting of mob room desc when in a water room.
 *
 * Revision 1.288  2008/09/05 21:42:25  myc
 * Fix read command.
 *
 * Revision 1.287  2008/09/03 17:34:08  myc
 * Moved liquid information into a def struct array.
 *
 * Revision 1.286  2008/09/01 22:20:39  jps
 * Fix formatting of message about an infrared-detected creature in the room.
 *
 * Revision 1.285  2008/09/01 06:29:40  jps
 * Using room sector color.
 *
 * Revision 1.284  2008/09/01 00:48:20  mud
 * Remove prototype which is imported from skills.h.
 *
 * Revision 1.283  2008/08/31 21:44:03  jps
 * Renamed StackObjs and StackMobs prefs to ExpandObjs and ExpandMobs.
 *
 * Revision 1.282  2008/08/31 18:54:22  myc
 * Apparently snprintf doesn't always act like sprintf.  Fix the
 * long desc on players.
 *
 * Revision 1.281  2008/08/29 04:47:04  myc
 * Don't show vnum for players.
 *
 * Revision 1.280  2008/08/29 04:41:05  myc
 * Don't collapse players.
 *
 * Revision 1.279  2008/08/29 04:16:26  myc
 * Rewrote the whole show_obj and list_one_char set of functions.
 * Hopefully they're a little nicer now.  list_char_to_char also
 * supports mob stacking now too.
 *
 * Revision 1.278  2008/08/26 04:39:21  jps
 * Changed IN_ZONE to IN_ZONE_RNUM or IN_ZONE_VNUM and fixed zone_printf.
 *
 * Revision 1.277  2008/08/24 04:34:20  myc
 * Skills command is sorted now.
 *
 * Revision 1.276  2008/08/18 01:35:38  jps
 * Replaced all \\n\\r with \\r\\n, not that it was really necessary...
 *
 * Revision 1.275  2008/08/15 05:34:47  jps
 * Changed the color of (magic) flag to blue.
 *
 * Revision 1.274  2008/08/15 03:59:08  jps
 * Added pprintf for paging, and changed page_string to take a character.
 *
 * Revision 1.273  2008/08/14 23:02:11  myc
 * Replaced some send_to_char calls with cprintf.
 *
 * Revision 1.272  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.271  2008/08/09 20:35:57  jps
 * Changed sense life so that it has a chance of detecting the presence and
movement
 * of creatures with a "healable" life force. Increased spell duration to 17-50
hrs.
 *
 * Revision 1.270  2008/07/24 05:35:49  myc
 * Commands command only lets immortals see other players' commands now.
 * Songs command lets immortals see other player's songs now.
 *
 * Revision 1.269  2008/07/22 18:11:33  myc
 * Inventory command lets immortals target other people now.
 *
 * Revision 1.268  2008/07/15 17:49:24  myc
 * Added command group editor and command grants, so do_commands now
 * checks to see if the command is granted too.
 *
 * Revision 1.267  2008/07/10 23:00:29  jps
 * Avoid divide-by-zero errors when displaying the experience of
 * level 0 characters.
 *
 * Revision 1.266  2008/06/26 06:19:34  jps
 * Fix do_spell's command parsing.
 *
 * Revision 1.265  2008/06/21 17:29:58  jps
 * Added a message about being on fire in score.
 *
 * Revision 1.264  2008/06/19 18:53:12  myc
 * Replaced the item_types string list with a struct array.
 *
 * Revision 1.263  2008/06/11 21:35:35  jps
 * Tweaked do_spells. Changed some message capitalization.
 *
 * Revision 1.262  2008/06/07 19:06:46  myc
 * Moved all object-related constants and structures to objects.h
 *
 * Revision 1.261  2008/06/05 02:07:43  myc
 * Changing object flags to use flagvectors.
 *
 * Revision 1.260  2008/05/25 21:00:05  myc
 * Rewrote list_obj_to_char to be cleaner and probably more efficient.
 *
 * Revision 1.259  2008/05/25 18:10:01  myc
 * Renamed look_at_room2 to show_room.
 *
 * Revision 1.258  2008/05/23 18:47:54  myc
 * Tweak formatting in do_score to make room for rage and line things up.
 *
 * Revision 1.257  2008/05/19 05:48:33  jps
 * Add indications of being mesmerized.
 *
 * Revision 1.256  2008/05/18 05:18:06  jps
 * Renaming room_data struct's member "number" to "vnum", cos it's
 * a virtual number.
 *
 * Revision 1.255  2008/05/18 04:25:20  jps
 * Remove unused variable.
 *
 * Revision 1.254  2008/05/18 04:09:06  jps
 * Tweaks to output in score and identify.
 *
 * Revision 1.253  2008/05/18 02:03:45  jps
 * Moved exit-description functions to rooms.c. Implemented isolation spell.
 *
 * Revision 1.252  2008/05/17 22:03:01  jps
 * Moving room-related code into rooms.h and rooms.c.
 *
 * Revision 1.251  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.250  2008/05/07 15:49:06  jps
 * Tweak the formatting on the list of active effects when outputting
 * the score.
 *
 * Revision 1.249  2008/04/13 18:50:16  jps
 * When you look at a confused person, you can tell (usually).
 *
 * Revision 1.248  2008/04/07 04:41:33  jps
 * Needed to add a fighting check in list_one_char now that
 * fighting isn't wrapped up in position.
 *
 * Revision 1.247  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.246  2008/04/06 21:56:53  jps
 * Fix punctuation in the you're blind message.
 *
 * Revision 1.245  2008/04/06 03:22:29  jps
 * Enable score for NPCs.  Spiff up the whoami command a bit.
 *
 * Revision 1.244  2008/04/05 18:07:09  myc
 * Re-implementing stealth for hide points.
 *
 * Revision 1.243  2008/04/05 03:46:26  jps
 * When you look at a character, you'll be told what it's composed
 * of if it isn't made of flesh.
 *
 * Revision 1.242  2008/04/04 06:12:52  myc
 * Removed dieties/worship code.
 *
 * Revision 1.241  2008/04/04 05:13:46  myc
 * Removing maputil code.
 *
 * Revision 1.240  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.239  2008/04/02 05:36:19  myc
 * Removed the noname toggle.
 *
 * Revision 1.238  2008/04/02 03:24:44  myc
 * Rewrote group code, and removed all major group code.
 *
 * Revision 1.237  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.236  2008/03/27 17:28:52  jps
 * Start showing auras around objects when looked at, for certain
 * flags (bless and hex).
 *
 * Revision 1.235  2008/03/26 23:11:46  jps
 * The waterform and vaporform flags are retired.
 *
 * Revision 1.234  2008/03/26 20:28:30  jps
 * Fix score when showing comp/lifeforce to someone else.
 *
 * Revision 1.233  2008/03/23 00:27:54  jps
 * Score will only show lifeforce and composition when they're nonstandard
 * (not life and/or not flesh).
 *
 * Revision 1.232  2008/03/22 20:26:08  jps
 * Add life force and composition to score.
 *
 * Revision 1.231  2008/03/22 16:27:23  jps
 * Reveal mobile illusory nature to observers with holylight.
 *
 * Revision 1.230  2008/03/19 18:29:59  myc
 * Added a space between shapeshifted name and long-desc on look room.
 *
 * Revision 1.229  2008/03/16 00:20:57  jps
 * Moved trophy-printing code to trophy.c.
 *
 * Revision 1.228  2008/03/11 02:54:04  jps
 * Use new sizedef struct.
 *
 * Revision 1.227  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.  Also moving innate timers to the cooldown
 * system.
 *
 * Revision 1.226  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.225  2008/03/06 05:11:51  myc
 * Combined the 'saved' and 'unsaved' portions of the char_specials and
 * player_specials structures by moving all fields of each saved structure
 * to its parent structure.  Also combined the skills array from the
 * player and mob structures since they are identical.
 *
 * Revision 1.224  2008/03/05 05:21:56  myc
 * Bank coins are ints instead of longs now.  And took out frags.
 *
 * Revision 1.223  2008/03/05 03:03:54  myc
 * Redesigned the trophy for the new ascii pfiles.  It's now makes way more
 * sense, imo.  Changed the way the who command accesses player titles.
 * Renamed GET_NATHPS to GET_BASE_HIT.
 *
 * Revision 1.222  2008/02/16 20:26:04  myc
 * Moving command sorting from here to interpreter.c.
 *
 * Revision 1.221  2008/02/10 20:35:12  myc
 * Removing superfluous check for (writing) flag.
 * in who command.
 *
 * Revision 1.220  2008/02/10 20:33:50  myc
 * Oops, wrong check for IS_PLAYING.
 *
 * Revision 1.219  2008/02/10 20:30:03  myc
 * Show people in OLC on who list.
 *
 * Revision 1.218  2008/02/09 21:07:50  myc
 * Removing plr/mob casting flags and using an event flag instead.
 *
 * Revision 1.217  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.216  2008/02/08 03:07:11  myc
 * Oops, checking the wrong index on sorted skills.
 *
 * Revision 1.215  2008/02/07 01:46:14  myc
 * Removing the size_abbrevs array and renaming SIZE_ABBR to SIZE_DESC,
 * which points to the sizes array.
 *
 * Revision 1.214  2008/02/05 04:22:42  myc
 * Removing listexp command; its functionality is now part of the
 * show command.
 *
 * Revision 1.213  2008/02/02 05:35:14  myc
 * Making the spells and listspells commands have sorted output.
 *
 * Revision 1.212  2008/01/29 21:24:43  myc
 * Fix formatting in do_score so one of the lines doesn't need to wrap.
 *
 * Revision 1.211  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.210  2008/01/29 16:51:12  myc
 * Moving skill names to the skilldef struct.
 *
 * Revision 1.209  2008/01/28 02:38:18  jps
 * Use proper abbreviation for pounds.
 *
 * Revision 1.208  2008/01/28 01:09:22  jps
 * Don't show ** exp bar to immortal who's checking someone's score.
 *
 * Revision 1.207  2008/01/27 11:15:29  jps
 * Make listspells more dynamic, and show all of the spellcasting classes
 * and not skip rangers.
 *
 * Revision 1.206  2008/01/27 09:53:12  jps
 * Give proper feedback to a god trying to see spells of an NPC.
 *
 * Revision 1.205  2008/01/27 09:41:14  jps
 * Use the data from class definitions to determine whether a mob has
 * spells, rather than hardcoding a list of classes. Also stop using
 * mclass_types[] since there's now no separate set of classes for
 * players vs. mobiles.
 *
 * Revision 1.204  2008/01/27 01:37:50  jps
 * Minor format fix in score.
 *
 * Revision 1.203  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.202  2008/01/25 21:05:45  myc
 * Renamed monk_weight_pen to monk_weight_penalty.
 *
 * Revision 1.201  2008/01/24 15:43:24  myc
 * Score command was checking wrong character to determine whether or not
 * to show experience message.
 *
 * Revision 1.200  2008/01/23 17:07:21  jps
 * Tweak Age output in score.
 *
 * Revision 1.199  2008/01/23 16:55:18  jps
 * Fixed looking-at-notes, cuz it was broken.
 *
 * Revision 1.198  2008/01/23 08:12:47  jps
 * Make note text (like mudmail) visible again.
 *
 * Revision 1.197  2008/01/23 01:46:30  jps
 * More score tweaks.
 *
 * Revision 1.196  2008/01/23 01:34:53  jps
 * Remove unused mode from show_points().
 *
 * Revision 1.195  2008/01/22 22:29:25  myc
 * Fixed the exp bar to go all the way to the end when ready to level.
 * Hide the exp bar on score for ** characters.  Removed attributes
 * command.
 *
 * Revision 1.194  2008/01/20 03:33:09  myc
 * Fix crash bug (null pointer error) in do_search.
 *
 * Revision 1.193  2008/01/18 20:30:11  myc
 * Fixing some send_to_char strings that don't end with a newline.
 *
 * Revision 1.192  2008/01/15 06:51:47  myc
 * Fixed exp_bar and refined exp messages.  Added the exp bar to
 * the experience and score commands.
 *
 * Revision 1.191  2008/01/14 20:38:42  myc
 * Oops, need to capitalize the room desc when showing vnums in list_one_char.
 *
 * Revision 1.190  2008/01/14 18:52:26  myc
 * Show a mob's vnum when looking at room even when the mob isn't
 * in its default position.
 *
 * Revision 1.189  2008/01/11 18:19:09  myc
 * Fixing a bug in scan that allowed you to see invisible people in a dark
 * room.
 *
 * Revision 1.188  2008/01/11 17:34:44  myc
 * show_obj_to_char (and by extension, list_obj_to_char) now use a set
 * of SHOW_OBJ_* constants to determine which description to display.
 * The constants are actually bits, so SHOW_OBJ_FLAGS can be or'd in
 * to show flags with any type of description.  Fixed some formatting in
 * show_obj_to_char.  Replaced all IS_AFFECTED macros with the respective
 * AFF_FLAGGED one.  Changed thief-inventory-vision to use list_obj_to_char.
 *
 * Revision 1.187  2008/01/09 13:27:57  jps
 * Adjusted score-reported positions.
 *
 * Revision 1.186  2008/01/09 10:09:18  jps
 * Allow folks with infravision to see mounted chars in the room with them.
 *
 * Revision 1.185  2008/01/09 09:18:20  jps
 * Allow deities to view other folks' attribs by typing "attribute <name>".
 * Fix do_attribute to show the first few lines of its output.
 * Change the display of points values slightly, because the second
 * value shown is not an unaffected value, so it shouldn't look like all
 * the other unaffected values. Show the unaffected values of hit points
 * and move points in do_attributes.
 *
 * Revision 1.184  2008/01/09 08:34:08  jps
 * Use utility functions to format the representation of heights and weights.
 *
 * Revision 1.183  2008/01/09 07:18:31  jps
 * Change mounted-upon description.
 *
 * Revision 1.182  2008/01/07 10:36:37  jps
 * Slightly adjust the display for player phantasms.  It's unfortunate
 * that the "name" field in mobs and players serves a different purpose.
 * Someone should fix that.
 *
 * Revision 1.181  2008/01/05 06:51:02  jps
 * Fixed initialization of showclass variable in do_users().
 *
 * Revision 1.180  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.179  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.178  2008/01/01 20:54:06  rsd
 * Cut and paste between files munched a line of the exp bar code.
 *
 * Revision 1.177  2008/01/01 20:51:17  rsd
 * changed exp bar so that the ='s advanced as a modulus of the
 * star.
 * /s
 *
 * Revision 1.176  2008/01/01 20:14:10  myc
 * Fixing exp message in score for when players are *extremely* close to
 * the next level.  Also, putting playing time back in score.
 *
 * Revision 1.175  2007/12/31 02:54:42  jps
 * Allow specification of a specific circle with the "spells" command.
 *
 * Revision 1.174  2007/12/29 00:05:47  jps
 * Don't make "look at #.<obj>" skip over objects with extra descs.
 * Improve the "You see nothing special" message.
 *
 * Revision 1.173  2007/12/26 07:51:14  jps
 * Fixed buffer-overrun bug in do_users(). Apparently, using magic numbers with
 * strncat() while counting incorrectly leads to bad things.
 *
 * Revision 1.172  2007/12/25 21:12:03  jps
 * Allow players to search for a door with a particular name, even if they're
 * carrying an object of the same name (as long as the object is not a
 * container). Take a door's plurality into consideration when formatting
 * the found-it message.
 *
 * Revision 1.171  2007/12/23 21:30:11  myc
 * Fix formatting in exp bar for immortals.
 *
 * Revision 1.170  2007/12/23 21:15:04  myc
 * Fix buffer overwrite bug in exp_bar().
 *
 * Revision 1.169  2007/12/22 23:51:08  myc
 * When 'standing' or 'sitting' in water, it will say floating instead
 * (on look and score).
 *
 * Revision 1.168  2007/12/22 23:03:21  myc
 * Mobs are now (hiding) instead of (hidden), and it's shown even when
 * they're in default position.  Cleaned up list_one_char a bit too.
 * Players shouldn't be able to tell if an immortal is switched into
 * a mob by seeing the AFK or holylight flags anymore.
 *
 * Revision 1.167  2007/12/20 23:04:32  myc
 * Cleaned up do_score a bunch, and made most numbers visible to everyone.
 * Delegated much of the work to a slew of helper functions which are now
 * also used by do_attribute.  do_experience is also affected.  Renamed a
 * few of the functions.  Moved exp_mesg from interpreter.c to exp_message
 * in act.informative.c, and added a new exp_bar function that represents
 * a player's tnl in a more graphical way; currently this function is only
 * used by prompt.
 *
 * Revision 1.166  2007/12/19 21:07:56  myc
 * Demonic mutation now shows up on look.
 *
 * Revision 1.165  2007/11/24 01:21:16  jps
 * Generalize relative location string handling.
 *
 * Revision 1.164  2007/11/23 19:19:55  jps
 * Fix formatting of who output and clean up comments.
 *
 * Revision 1.163  2007/11/21 19:49:18  jps
 * Fix is_starstar macro to work when you have excess xp. This can happen
 * (temporarily) for monks who achieved ** before we lowered the amount
 * of xp they needed. It would look like they were level 99, which
 * was disconcerting.
 *
 * Revision 1.162  2007/11/18 17:40:58  myc
 * Getting rid of 'Mac-format' newlines.
 *
 * Revision 1.161  2007/11/18 17:16:15  myc
 * Attempting to fix corrupted file.
 *
 * Revision 1.158  2007/10/27 03:19:47  myc
 * Make score a little more readable.
 *
 * Revision 1.157  2007/10/27 02:16:37  myc
 * Taking out a debug message.
 *
 * Revision 1.156  2007/10/25 20:36:27  myc
 * Using new WEAPON_AVERAGE macro.  Changed do_score to say "Active Spell
 * and Status Effects" instead of "Active Spells".
 *
 * Revision 1.155  2007/10/17 17:18:04  myc
 * Renamed the search_block and search_block2 functions.
 * searchblock is now case sensitive, and search_block is not.
 *
 * Revision 1.154  2007/10/11 20:14:48  myc
 * Identify command doesn't work while fighting.
 * Got rid of the "Load problem, please report" message so people stop
 * reporting their overweight loads.
 * Moved songs command here from act.other.c.
 *
 * Revision 1.153  2007/10/02 02:52:27  myc
 * The users command will show the correct idle time for shapechanged druids
 * now.  Page length is now grabbed from the original player even when
 * switched.  Scan won't show self.
 *
 * Revision 1.152  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  AFF_HIDE and ITEM_HIDDEN are now
 * unused.  Changes to list_to_char, do_search, do_score, identify_obj,
 * and a fix to do_scan.
 *
 * Revision 1.151  2007/09/20 09:15:14  jps
 * Typo fix
 *
 * Revision 1.150  2007/09/15 15:36:48  myc
 * Farsee now helps you scan farther.
 *
 * Revision 1.149  2007/09/15 05:03:46  myc
 * Removing the distinction between AFF1, AFF2, and AFF3 flags within
 * the game.
 *
 * Revision 1.148  2007/09/11 16:34:24  myc
 * Skills command now shows skills the character has even if their class
 * doesn't normally have it.
 *
 * Revision 1.147  2007/09/07 20:32:19  jps
 * Adjustments to identify command.
 *
 * Revision 1.146  2007/09/07 19:41:35  jps
 * Added "identify" command, which works pretty well for thieves
 * but is only slightly useful for other classes.
 *
 * Revision 1.145  2007/09/04 06:49:19  myc
 * Modified weather command to use the new weather message functions in
 * weather.c.  Moved the 'list all spells' behavior from the spells command
 * to the listspells command.  It now omits non-caster classes, and can
 * show circle numbers instead of levels.
 *
 * Revision 1.144  2007/09/03 21:20:08  jps
 * You will see "wall" objects when looking in a direction.
 *
 * Revision 1.143  2007/08/28 20:19:12  myc
 * Looking in a direction will now tip you off about a circle of fire in
 * that direction.  Rewrote farsee and scan to allow for arbitrary distances,
 * which cleaned up the code a lot.  Farsee now has a longer range, which
 * is dependent on the user's skill in sphere of divination.
 *
 * Revision 1.142  2007/08/26 21:44:59  jps
 * "score" will tell you whether you're drunk.
 *
 * Revision 1.141  2007/08/26 21:37:27  jps
 * Also append total-max to the skill points viewed by gods.
 *
 * Revision 1.140  2007/08/26 21:34:02  jps
 * Change the mark of a mastered skill to "-*-" which it's fully mastered.
 * Append (current/max) skill points to skills when it's a god viewing.
 *
 * Revision 1.139  2007/08/23 23:59:31  myc
 * Exits command shows more information to imms now.
 *
 * Revision 1.138  2007/08/14 22:43:07  myc
 * NOTRACK flag now makes one invisible on 'who -z' for stealth skill.
 *
 * Revision 1.137  2007/08/14 10:42:21  jps
 * The score command will tell you if you've got laryngitis.
 *
 * Revision 1.136  2007/08/08 21:11:55  jps
 * Don't talk about barriers for mobs with NEGATE_* flags.
 *
 * Revision 1.135  2007/08/05 20:21:51  myc
 * Moved retreat to act.offensive.c
 *
 * Revision 1.134  2007/08/04 21:34:40  jps
 * Make the message for finding a hidden door more informative.
 *
 * Revision 1.133  2007/08/03 22:00:11  myc
 * Fixed several \r\n typos in send_to_chars.
 *
 * Revision 1.132  2007/08/02 00:23:22  myc
 * Get lines in 'where' command to line up.
 *
 * Revision 1.131  2007/07/14 02:16:22  jps
 * Consider now provides information about how easy it looks like
 * it would be to tame or ride a mountable mob.
 *
 * Revision 1.130  2007/07/04 15:23:36  jps
 * Prevent level 15 players from seeing their natural stats in score.
 *
 * Revision 1.129  2007/07/01 19:34:42  myc
 * do_spells wasn't clearing the buffer before using it.
 *
 * Revision 1.128  2007/06/30 21:51:29  myc
 * Sanctuary appears black for evil and blue for neutral now.  Soulshield
 * appears red for evil.  Holylight sees character affections previously
 * reserved for those affected by detect magic.
 *
 * Revision 1.127  2007/06/26 00:34:44  jps
 * Differentiating between the first and subsequent extra descs.
 * With the first one, you also see object type-specific messages,
 * such as spellbook spells.
 *
 * Revision 1.126  2007/06/24 02:51:44  jps
 * Cause "spells" and "skills" commands to take a character argument for
 * deities so they can invoke them on other people.
 *
 * Revision 1.125  2007/06/24 01:13:44  jps
 * Colorize room title when roomflags is on.
 *
 * Revision 1.124  2007/06/16 00:39:40  myc
 * Fixed format for attributes in do_score.
 *
 * Revision 1.123  2007/05/31 12:50:43  jps
 * Restore the rcs log to the end of this file.
 *
 * Revision 1.122  2007/05/31 12:44:46  jps
 * Fix looking at object extra descs, which I broke earlier.
 *
 * Revision 1.121  2007/05/24 06:35:18  jps
 * Set room desc of players to "<name> <title> <race>..." moving title next to
name.
 *
 * Revision 1.120  2007/05/24 06:01:01  jps
 * Display '*' next to each skill that's as advanced as possible in do_skills.
 *
 * Revision 1.119  2007/05/17 22:17:12  myc
 * do_attributes was showing wis for the wrong char under certain
 * circumstances.
 *
 * Revision 1.118  2007/05/12 21:14:22  myc
 * Commands list was using the wrong boundary when calculating columns.
 * Somehow we didn't have a problem with it before.
 *
 * Revision 1.117  2007/05/11 21:32:57  myc
 * Roar should show up on the socials list.
 *
 * Revision 1.116  2007/05/11 21:10:36  myc
 * Fixed a bug in do_commands that was cutting off commands at the end of
 * each column.  Fixed one in do_score that was using ch instead of tch.
 *
 * Revision 1.115  2007/04/25 08:00:53  jps
 * Show (Casting) for players as well as mobiles.
 *
 * Revision 1.114  2007/04/19 07:03:14  myc
 * Renamed RAY_OF_ENFEB as RAY_OF_ENFEEB.
 *
 * Revision 1.113  2007/04/18 21:14:21  jps
 * Make paralysis visible in normal room look.
 *
 * Revision 1.112  2007/04/18 20:20:24  jps
 * Made a large number of spells and effects visible when looking at characters.
 *
 * Revision 1.111  2007/04/15 05:11:23  jps
 * Show spellbook spells just by looking at the book.  Show object details
 * when looking at them (correctly now - bugfix).
 *
 * Revision 1.110  2007/04/11 09:41:12  jps
 * Fix positional description typo.
 *
 * Revision 1.109  2007/04/06 21:12:21  jps
 * Display the sector type when roomflags is on.
 *
 * Revision 1.108  2007/03/27 04:27:05  myc
 * New size message added to diag_char_to_char(), colossal.  Made holylight
 * see through wall of fog.  Changed exp command message for **.  Completely
 * revamped innate command, but functionality remains mainly the same.
 *
 * Revision 1.107  2007/01/25 17:06:12  myc
 * Shouldn't get crashes when newbies are looked at now.
 *
 * Revision 1.106  2006/12/08 05:05:34  myc
 * Missing final CRLF when looking at an item's action desc.
 *
 * Revision 1.105  2006/12/05 03:09:19  myc
 * Action descs on notes can now be seen using look again.
 *
 * Revision 1.104  2006/11/27 02:07:05  jps
 * Allow "look in" to work like "look at" for ALL gate-spell objects.
 *
 * Revision 1.103  2006/11/27 01:26:03  jps
 * When you look at "4.<thing>" you will see its first extra description,
 * if any.  Extra blank lines after object extra descs are gone.
 *
 * Revision 1.102  2006/11/24 07:27:28  jps
 * Allow the adjacent exit's normal description and door to be
 * seen, even with farsee.
 *
 * Revision 1.101  2006/11/24 07:02:28  jps
 * Use correct "is" or "are" depending on the name of a door
 *
 * Revision 1.100  2006/11/24 05:25:11  jps
 * Let deities type "score <player>" and see their score
 *
 * Revision 1.99  2006/11/23 00:36:24  jps
 * Fix null-string check when looking at a player's description.
 *
 * Revision 1.98  2006/11/20 22:35:16  jps
 * Fix string termination in commands list
 *
 * Revision 1.97  2006/11/20 22:24:17  jps
 * End the difficulties in interaction between evil and good player races.
 *
 * Revision 1.96  2006/11/20 18:18:18  jps
 * fixed some to->too typos
 *
 * Revision 1.95  2006/11/18 07:03:30  jps
 * Minor typo fixes
 *
 * Revision 1.94  2006/11/18 04:26:32  jps
 * Renamed continual light spell to illumination, and it only works on
 * LIGHT items (still rooms too).
 *
 * Revision 1.93  2006/11/18 00:03:31  jps
 * Fix continual light items to always work when they have the
 * bit set.  Rooms now print an indicator of being continually lit.
 * Can't use it to make a room permanently lit any more.
 *
 * Revision 1.92  2006/11/17 23:05:32  jps
 * Fix bug where evil-race mortals would never see good-race imms in the
 * who list, and the same problem with good-race mortals seeing evil-race imms.
 *
 * Revision 1.91  2006/11/16 19:42:39  jps
 * Fix confusion of looking in the dark, with or without infravision,
 * and trying to read in the dark.
 *
 * Revision 1.90  2006/11/16 18:42:45  jps
 * Awareness of new surroundings when magically tranported is related to
 * being asleep, blindness, etc.
 *
 * Revision 1.89  2006/11/13 15:54:22  jps
 * Fix widespread misuse of the hide_invisible parameter to act().
 *
 * Revision 1.88  2006/11/12 20:54:36  jps
 * Fix bug in who list - any anon made the rest look anon.
 *
 * Revision 1.87  2006/11/11 16:43:26  jps
 * Fix extra spacing in player long desc when they have no title.
 *
 * Revision 1.86  2006/11/11 15:57:58  jps
 * Fix extra space in who list when you have no title.  Stop anons from being
 * listed when a level range is specified.  Reenable -c <classes>, but don't
 * allow it to list anons.
 *
 * Revision 1.85  2006/11/11 10:11:04  jps
 * "score" now informs of hunger and thirst.
 *
 * Revision 1.84  2006/11/08 21:46:04  jps
 * Change socials list (and other command lists, if they ever get reinstated)
 * to display in vertically sorted columns rather than being horizontally
 * sorted. Also gets sent out via page_string now.
 *
 * Revision 1.83  2006/11/08 09:04:30  jps
 * Add punctuation, fix line space formatting for farsee.
 *
 * Revision 1.82  2006/11/08 08:37:44  jps
 * Typo fix: Sparsly -> Sparsely
 *
 * Revision 1.81  2006/08/07 17:24:35  rsd
 * made trophy to look at others trohpies level 100, retabbed
 * and re-braced do_trophy.
 *
 * Revision 1.80  2006/07/20 16:33:32  cjd
 * Additional Typo Fixes.
 *
 * Revision 1.79  2006/07/20 07:36:06  cjd
 * Typo fixes.
 *
 * Revision 1.78  2006/05/11 03:19:42  cjd
 * add function to allow imms to check trophies without
 * having to snoop the victim.
 *
 * Revision 1.77  2006/05/01 07:05:07  rsd
 * Ok, made search work first try for everyone level 1 and
 * above.  Additionally search cuases no more lag. omg.
 *
 * Revision 1.76  2005/08/05 06:23:56  jwk
 * Who is all messed up.... fixed it so that when you who when you're anonymous
your name is shown... apparently throught the who, someone is using the buf
buffer sometimes and the Mort_buff other times... need to go through this
thoroughly and verify that everything is kosher.
 *
 * Revision 1.75  2005/02/14 04:11:51  djb
 * Fixing mistake made when checking in the files.
 *
 * Revision 1.74  2005/02/14 02:17:57  djb
 * Removed the align checks done when looking to allow evil/good races to
see/group with each other.
 *
 * Revision 1.73  2004/10/27 22:02:29  rsd
 * added who usage fix to include -w.
 *
 * Revision 1.72  2004/10/27 21:50:51  rsd
 * Ok, changed the who not to show the (zone) people are in
 * by default.  Created the -w option for this and made it
 * so when pk is on it won't work etc etc.
 *
 * Revision 1.71  2004/10/25 02:36:52  rsd
 * Changed do_who to account for pkill being on such that if
 * pkill is on, you can't do a who -z (zone) and see who is
 * in your zone, and if PK is on zones will no longer show
 * on players in the who list.
 * /s
 *
 * Revision 1.70  2004/10/22 02:40:24  rsd
 * Ok, after some deliberation I've decided to show the zones
 * each mortal is in to the who list.  This will require some
 * testing... well because I didn't learn what each other the
 * possible 10 trillion who options did.  As a result most all
 * the who options will show the zone, maybe even of the gods
 * and anonymous people etc.. need to know what needs to be
 * trimmed down.  See do_who for changes.
 *
 * Revision 1.69  2003/06/28 02:02:55  jjl
 * Added the ability for shapechangers and switched gods to use the "score"
 * command to see their original stats.
 *
 * Revision 1.68  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.67  2002/09/11 03:00:58  jjl
 * Changed experincce te to experience, to fix a pet peeve.
 *
 * Revision 1.66  2002/07/16 19:29:14  rls
 * Fluff stuff, colorized a few things
 *
 * Revision 1.65  2002/07/14 02:43:54  rls
 * *** empty log message ***
 *
 * Revision 1.64  2002/07/13 21:02:28  rls
 * Changed do_users to do_sockets and "users" to "sockets"
 * (that way immortal+ can use "use")
 * Tried restructing but was causing "use" to be inactive for morts
 *
 * Revision 1.63  2002/03/15 02:45:14  dce
 * Increased wiztitle from 11 to 12.
 *
 * Revision 1.62  2001/10/16 00:13:36  rjd
 * Fly semantics improved.
 *
 * Revision 1.61  2001/10/13 03:39:16  rjd
 * Fixed bug with "scan" where if there were only invisible (N)PCs
 * in scanned rooms, a blank line would appear after the "you see"
 * type bit, instead of "Absolutely no one!".
 *
 * Revision 1.60  2001/10/12 22:28:26  rjd
 * Tweaked "who" command so that mortals can not identify anonymous players
 * via the minlevl-maxlev or class parameters to the command. Usage string
 * also updated.
 *
 * Revision 1.59  2001/03/26 01:09:49  dce
 * Fixed users so it pages correctly.
 *
 * Revision 1.58  2001/03/04 14:12:10  dce
 * Added an option to the do_users command to sort by ip.
 *
 * Revision 1.57  2001/01/20 03:39:10  rsd
 * made listing of all spells of all classes level 103 and higher
 *
 * Revision 1.56  2000/12/03 01:05:56  mtp
 * fix for farsee which allowed a player to escape battle (involved breaking
look_at_room
 * into two parts to keep interface for other files whicle allowing a specfied
room
 * instead of physically moving character)
 *
 * Revision 1.55  2000/11/19 01:38:36  rsd
 * Added back change log messages that weren't included thus
 * far.
 *
 * Revision 1.54  2000/11/18 05:10:44  rsd
 * Added color to the who list for gods to see name approval
 * states for unapproved frozen and declined players when
 * typing who...
 *
 * Revision 1.53  2000/11/16 01:59:31  rsd
 * Added debug to a time function to try to see why
 * the month name is reported wrong some times
 *
 * Revision 1.52  2000/05/21 23:54:22  rsd
 * The word test now shows in the who headers in do_who
 * for test builds.
 *
 * Revision 1.51  2000/05/20 01:35:47  rsd
 * put strip ansi into object short descs for perf_imm_where
 * so alignement would look better.
 *
 * Revision 1.50  2000/05/19 22:16:18  rsd
 * added strip_ansi to truncated room names in do_users
 *
 * Revision 1.49  2000/05/01 23:22:54  jimmy
 * Re added races back into the who list for mortals.
 *
 * Revision 1.48  2000/04/26 23:41:03  rsd
 * removed mana references from score display.
 *
 * Revision 1.47  2000/04/22 22:22:54  rsd
 * warrior types can't type spells and get a blank spell listing.
 * Also tabbed out and braced sections of code. Gave up on trying
 * to get listspell to show the sphere each spell is. It's commented
 * still though.
 *
 * Revision 1.46  2000/04/20 02:31:16  rsd
 * attribute command altered by Aredryk put in, search has a
 * stun time now.
 *
 * Revision 1.45  2000/04/19 04:17:12  cso
 * do_attributes had a send_to_char misplaced so that it only executed for chars
over lvl75. moved it.
 * /
 *
 * Revision 1.44  2000/04/05 06:29:50  rsd
 * made the spells listed in the listspell <class> command
 * go through 12th circle instead of 10th.
 *
 * Revision 1.43  2000/04/02 02:36:33  rsd
 * changed the comment header while I was browsing the file for
 * information.
 *
 * Revision 1.42  2000/03/22 21:17:05  rsd
 * fixed some code style and removed the listing of a deity from
 * the score and attribute command as they are unimplemented and
 * people ask us about it constantly.
 *
 * Revision 1.41  2000/02/26 05:10:44  cso
 * fixed problem in do_skills where unswitched mobs executing 'skills' would
crash the mud
 *
 * Revision 1.40  2000/02/25 03:33:29  cso
 * fixed several typos
 *
 * Revision 1.39  1999/12/10 22:13:45  jimmy
 * Exp tweaks.  Made Exp loss for dying a hardcoded 25% of what was needed for
the next
 * level.  Fixed problems with grouping and exp.  Removed some redundant and
unnecessary
 * exp code.
 *
 * Revision 1.38  1999/11/28 22:32:41  cso
 * modified do_users: room[] was not terminated in a few places. terminate
 * -ed it.
 *
 * Revision 1.37  1999/11/26 05:47:18  rsd
 * changed do_users declaration of position to 17 to end
 * overflow problem caused by mortally wounded, it was 9.
 * Changed do_who, made *** the cute ascii art delemiting
 * who <individual> as opposed to the =-=-=-=- it was.
 * Also removed the mention of goods and evils from the
 * who list pending me figuring out how to check the one
 * issuing the commands level so only 100+ can see that
 * part.
 * I didn't actually remove it I just commented it out.
 *
 * Revision 1.36  1999/10/11 22:14:31  rsd
 * Made one of the variables in do_users larger to fix an overflow
 *
 * Revision 1.35  1999/10/04 21:09:30  rsd
 * Removed farsee as a god innate! Also fixed the curly braces
 * for that entire function.
 *
 * Revision 1.34  1999/09/16 01:15:11  dce
 * Weight restrictions for monks...-hitroll, -damroll + ac
 *
 * Revision 1.33  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.32  1999/08/28 01:18:28  mtp
 * cant search wihile fighting[4~
 *
 * Revision 1.31  1999/08/12 20:42:01  dce
 * Level 99's can now reach **.
 *
 * Revision 1.30  1999/08/09 22:32:36  mtp
 * Added AFK tag for look/who command
 *
 * Revision 1.29  1999/07/26 02:43:54  jimmy
 * fixed do_user command.  It now shows people whoare at the menu's/motd
 * etc properly
 * --gurlaek
 *
 * Revision 1.28  1999/07/26 01:19:41  mud
 * added level check for help entries in do_help.
 *
 * Revision 1.27  1999/07/25 23:49:29  jimmy
 * Fixed the crashbug in viewdam all
 * it works fine now.
 * gurlaek
 *
 * Revision 1.26  1999/07/06 19:57:05  jimmy
 * This is a Mass check-in of the new skill/spell/language assignment system.
 * This New system combines the assignment of skill/spell/language for
 * both mobs and PCs.  LOts of code was touched and many errors were fixed.
 * MCLASS_VOID was moved from 13 to -1 to match CLASS_UNDEFINED for PC's.
 * MObs now get random skill/spell/language levels baseed on their
race/class/level
 * that exactly align with PC's.  PC's no longer have to rent to use skills
gained
 * by leveling or when first creating a char.  Languages no longer reset to
defaults
 * when a PC levels.  Discovered that languages have been defined right in the
middle
 * of the spell area.  This needs to be fixed.  A conversion util neeDs to be
run on
 * the mob files to compensate for the 13 to -1 class change.
 * --gurlaek 7/6/1999
 *
 * Revision 1.25  1999/06/30 18:11:09  jimmy
 * act.item.c         class.c       db.c         medit.c        utils.h
act.offensive.c    config.c      handler.c    spells.cThis is a major conversion
from the 18 point attribute system to the
 * 100 point attribute system.  A few of the major changes are:
 * All attributes are now on a scale from 0-100
 * Everyone views attribs the same but, the attribs for one race
 *   may be differeent for that of another even if they are the
 *   same number.
 * Mobs attribs now get rolled and scaled using the same algorithim as PC's
 * Mobs now have individual random attributes based on race/class.
 * The STR_ADD attrib has been completely removed.
 * All bonus tables for attribs in constants.c have been replaced by
 *   algorithims that closely duplicate the tables except on a 100 scale.
 * Some minor changes:
 * Race selection at char creation can now be toggled by using
 *   <world races off>
 * Lots of cleanup done to affected areas of code.
 * Setting attributes for mobs in the .mob file no longer functions
 *   but is still in the code for later use.
 * We now have a spare attribut structure in the pfile because the new
 *   system only used three instead of four.
 * --gurlaek 6/30/1999
 *
 * Revision 1.24  1999/06/21 18:25:40  jimmy
 * Fixed the object stacking code.  list_obj_to_char now shows the number of
 * each object in a room without listing it multiple times.
 * Gurlaek 6/21/1999
 *
 * Revision 1.23  1999/06/14 22:28:45  jimmy
 * added extra functionality to a complex structure based
 * listexp command.
 *
 * Revision 1.22  1999/05/04 19:44:08  dce
 * Fixed typos
 *
 * Revision 1.21  1999/04/23 23:27:10  jimmy
 * Fixed warnings/errors associated with the addition of the pendantic compiler
flag
 * yeeeeehaaawwww.  --gurlaek
 *
 * Revision 1.20  1999/04/21 04:05:04  dce
 * New Exp system.
 * Exp_fix.c is the converter.
 *
 * Revision 1.19  1999/04/09 20:33:04  dce
 * Added listexp command
 *
 * Revision 1.18  1999/04/07 01:20:18  dce
 * Allows extra descriptions on no exits.
 *
 * Revision 1.17  1999/03/26 19:43:00  dce
 * Fixed attribute, aligned new score and added old score back, under the
command
 * old.
 *
 * Revision 1.16  1999/03/18 17:00:21  dce
 * Changes score to show things at different levels
 *
 * Revision 1.15  1999/03/12 18:05:43  dce
 * Rewrote scan!
 *
 * Revision 1.14  1999/02/19 01:58:44  dce
 * Changes who to the old fiery method except with 3 letter classes.
 * Fixes anonymous ands godly levels.
 *
 * Revision 1.13  1999/02/17 19:20:01  dce
 * Object sematics for continual light. Also fiexeds the hidden flag.
 *
 * Revision 1.12  1999/02/13 19:37:12  dce
 * Gods can see through RAFF_DARKNESS
 *
 * Revision 1.11  1999/02/11 22:17:40  jimmy
 * Moved spell circles to every 8 levels.  Filled in the
 * spells array to extend from level 70 to 105.
 * fingon
 *
 * Revision 1.10  1999/02/11 17:41:16  dce
 * Fixes Continual Light problems.
 *
 * Revision 1.9  1999/02/10 22:21:42  jimmy
 * Added do_wiztitle that allows gods to edit their
 * godly title ie Overlord.  Also added this title
 * to the playerfile
 * fingon
 *
 * Revision 1.8  1999/02/10 05:57:14  jimmy
 * Added long description to player file.  Added AFK toggle.
 * removed NOAUCTION toggle.
 * fingon
 *
 * Revision 1.7  1999/02/10 02:38:58  dce
 * Fixes some of continual light.
 *
 * Revision 1.6  1999/02/04 18:41:13  jimmy
 * Hide races in the who list until we're ready.
 *
 * Revision 1.5  1999/02/04 16:42:34  jimmy
 * Combined attributes, score, and exp commands.
 *
 * Revision 1.4  1999/02/02 02:37:34  mud
 * Cleaned up comment header some more
 * replaced occurences of Hubis with Fiery
 *
 * Revision 1.3  1999/02/02 01:10:27  jimmy
 * improved do_spells
 *
 * Revision 1.2  1999/02/01 22:39:12  jimmy
 * hid spells that are beyond the circle of the caster
 *
 * Revision 1.1  1999/01/29 01:23:29  mud
 * Initial revision
 *
 ***************************************************************************/
