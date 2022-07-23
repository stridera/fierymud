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
        if (GET_SKILL(ch, SKILL_BREATHE_FIRE))
            send_to_char(" breathe fire\r\n", ch);
        if (GET_SKILL(ch, SKILL_BREATHE_ACID))
            send_to_char(" breathe acid\r\n", ch);
        if (GET_SKILL(ch, SKILL_BREATHE_FROST))
            send_to_char(" breathe frost\r\n", ch);
        if (GET_SKILL(ch, SKILL_BREATHE_LIGHTNING))
            send_to_char(" breathe lightning\r\n", ch);
        if (GET_SKILL(ch, SKILL_BREATHE_GAS))
            send_to_char(" breathe gas\r\n", ch);
        if (GET_RACE(ch) == RACE_GNOME || GET_RACE(ch) == RACE_SVERFNEBLIN)
            send_to_char(" brill\r\n", ch);
        if (GET_RACE(ch) == RACE_ORC) 
            send_to_char(" chaz\r\n", ch);
        if (GET_RACE(ch) == RACE_GNOME)
            send_to_char(" create (as spell minor creation)\r\n", ch); 
        if (GET_RACE(ch) == RACE_DROW)
            send_to_char(" darkness\r\n", ch);
        if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_DIABOLIST || GET_CLASS(ch) == CLASS_PALADIN ||
            GET_CLASS(ch) == CLASS_ANTI_PALADIN)
            send_to_char(" detect alignment*\r\n", ch);
        if (GET_SKILL(ch, SKILL_DOORBASH))
            send_to_char(" doorbash\r\n", ch);
        if (GET_CLASS(ch) == CLASS_THIEF)
            send_to_char(" identify\r\n", ch);
        if (GET_RACE(ch) == RACE_ELF || GET_RACE(ch) == RACE_DWARF || GET_RACE(ch) == RACE_HALFLING ||
            GET_RACE(ch) == RACE_HALF_ELF || GET_RACE(ch) == RACE_GNOME || GET_RACE(ch) == RACE_DRAGONBORN_FIRE ||
            GET_RACE(ch) == RACE_DRAGONBORN_FROST || GET_RACE(ch) == RACE_DRAGONBORN_ACID || 
            GET_RACE(ch) == RACE_DRAGONBORN_LIGHTNING || GET_RACE(ch) == RACE_DRAGONBORN_GAS || GET_RACE(ch) == RACE_SVERFNEBLIN)
            send_to_char(" infravision*\r\n", ch);
        if (GET_RACE(ch) == RACE_DUERGAR)
            send_to_char(" invisible\r\n", ch);
        if (GET_CLASS(ch) == CLASS_PALADIN || GET_CLASS(ch) == CLASS_ANTI_PALADIN)
            send_to_char(" layhands\r\n", ch);
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
        if (GET_SKILL(ch, SKILL_SWEEP))
            send_to_char(" sweep\r\n", ch);
        if (GET_RACE(ch) == RACE_ELF)
            send_to_char(" syll\r\n", ch);
        if (GET_RACE(ch) == RACE_DWARF || GET_RACE(ch) == RACE_DUERGAR)
            send_to_char(" tass\r\n", ch);
        if (GET_RACE(ch) == RACE_DROW || GET_RACE(ch) == RACE_DUERGAR || GET_RACE(ch) == RACE_TROLL ||
            GET_RACE(ch) == RACE_OGRE || GET_RACE(ch) == RACE_DWARF)
            send_to_char(" ultravision*\r\n", ch);
        send_to_char("Effects marked with a * are always present.\r\n", ch);
    } else {
        if (is_abbrev(arg, "bodyslam") && GET_SKILL(ch, SKILL_BODYSLAM)) {
            send_to_char("Usage: bodyslam <victim>\r\n", ch);
            return;
        }

        if (is_abbrev(arg, "breathe") && (GET_SKILL(ch, SKILL_BREATHE_FIRE) || GET_SKILL(ch, SKILL_BREATHE_FROST) || 
            GET_SKILL(ch, SKILL_BREATHE_ACID) || GET_SKILL(ch, SKILL_BREATHE_GAS) || GET_SKILL(ch, SKILL_BREATHE_LIGHTNING) )) {
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
                        cprintf(ch, "You can create darkness again in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_INNATE_DARKNESS) / 10));
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
                    cprintf(ch, "You can turn invisible again in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_INNATE_INVISIBLE) / 10));
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
                    cprintf(ch, "You can levitate again in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_INNATE_LEVITATE) / 10));
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

        if (is_abbrev(arg, "chaz")) {
            if (GET_RACE(ch) == RACE_ORC) {
                if (!GET_COOLDOWN(ch, CD_INNATE_CHAZ)) {
                    call_magic(ch, ch, 0, SPELL_INN_CHAZ, GET_LEVEL(ch), CAST_SPELL);
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                        SET_COOLDOWN(ch, CD_INNATE_CHAZ, 7 MUD_HR);
                } else
                    send_to_char("You're too tired right now.\r\n", ch);
                    cprintf(ch, "You can strengthen again in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_INNATE_CHAZ) / 10));
                return;
            }
        }

        /*new innates*/

        if (is_abbrev(arg, "syll")) {
            if (GET_RACE(ch) == RACE_ELF) {
                if (!GET_COOLDOWN(ch, CD_INNATE_SYLL)) {
                    call_magic(ch, ch, 0, SPELL_INN_SYLL, GET_LEVEL(ch), CAST_SPELL);
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                        SET_COOLDOWN(ch, CD_INNATE_SYLL, 7 MUD_HR);
                } else
                    send_to_char("You're too tired right now.\r\n", ch);
                    cprintf(ch, "You can be more graceful again in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_INNATE_SYLL) / 10));
                return;
            }
        }
        if (is_abbrev(arg, "brill")) {
            if (GET_RACE(ch) == RACE_GNOME || GET_RACE(ch) == RACE_SVERFNEBLIN) {
                if (!GET_COOLDOWN(ch, CD_INNATE_BRILL)) {
                    call_magic(ch, ch, 0, SPELL_INN_BRILL, GET_LEVEL(ch), CAST_SPELL);
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                        SET_COOLDOWN(ch, CD_INNATE_BRILL, 7 MUD_HR);
                } else
                    send_to_char("You're too tired right now.\r\n", ch);
                    cprintf(ch, "You can boost your intelligence again in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_INNATE_BRILL) / 10));
                return;
            }
        }
        if (is_abbrev(arg, "tass")) {
            if (GET_RACE(ch) == RACE_DWARF || GET_RACE(ch) == RACE_DUERGAR) {
                if (!GET_COOLDOWN(ch, CD_INNATE_TASS)) {
                    call_magic(ch, ch, 0, SPELL_INN_TASS, GET_LEVEL(ch), CAST_SPELL);
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                        SET_COOLDOWN(ch, CD_INNATE_TASS, 7 MUD_HR);
                } else
                    send_to_char("You're too tired right now.\r\n", ch);
                    cprintf(ch, "You can seek wisdom again in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_INNATE_TASS) / 10));
                return;
            }
        }
        /*
        if (is_abbrev(arg, "tren")) {
            if (GET_RACE(ch) == RACE_ELF) {
                if (!GET_COOLDOWN(ch, CD_INNATE_TREN)) {
                    call_magic(ch, ch, 0, SPELL_INN_TREN, GET_LEVEL(ch), CAST_SPELL);
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                        SET_COOLDOWN(ch, CD_INNATE_TREN, 7 MUD_HR);
                } else
                    send_to_char("You're too tired right now.\r\n", ch);
                    cprintf(ch, "You can fortify yourself again in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_INNATE_TREN) / 10));
                return;
            }
        }
        if (is_abbrev(arg, "ascen")) {
            if (GET_RACE(ch) == RACE_ELF) {
                if (!GET_COOLDOWN(ch, CD_INNATE_ASCEN)) {
                    call_magic(ch, ch, 0, SPELL_INN_ASCEN, GET_LEVEL(ch), CAST_SPELL);
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                        SET_COOLDOWN(ch, CD_INNATE_ASCEN, 7 MUD_HR);
                } else
                    send_to_char("You're too tired right now.\r\n", ch);
                    cprintf(ch, "You can reliven your charming nature again in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_INNATE_ASCEN) / 10));
                return;
            }
        }
        */
        if (is_abbrev(arg, "harness")) {
            if (GET_RACE(ch) == RACE_ELF) {
                if (!GET_COOLDOWN(ch, CD_INNATE_HARNESS)) {
                    call_magic(ch, ch, 0, SPELL_HARNESS, GET_LEVEL(ch), CAST_SPELL);
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                        SET_COOLDOWN(ch, CD_INNATE_HARNESS, 10 MUD_HR);
                } else
                    send_to_char("You're too tired right now.\r\n", ch);
                    cprintf(ch, "You can boost your magical abilities again in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_INNATE_HARNESS) / 10));
                return;
            }

        }

        if (is_abbrev(arg, "sweep") && GET_SKILL(ch, SKILL_SWEEP)) {
            send_to_char("Usage: sweep\r\n", ch);
            return;
        }

        if (is_abbrev(arg, "create") && GET_RACE(ch) == RACE_GNOME) {
            send_to_char("Usage: create <object>\r\n", ch);
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