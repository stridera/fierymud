/***************************************************************************
 * $Id: utils.c,v 1.130 2010/06/20 19:53:47 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: utils.c                                        Part of FieryMUD *
 *  Usage: various internal functions of a utility nature                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "utils.h"

#include "casting.h"
#include "comm.h"
#include "composition.h"
#include "conf.h"
#include "constants.h"
#include "db.h"
#include "events.h"
#include "handler.h"
#include "interpreter.h"
#include "lifeforce.h"
#include "math.h"
#include "money.h"
#include "pfiles.h"
#include "races.h"
#include "screen.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "weather.h"

#include <sys/stat.h> /* For stat() */

int monk_weight_penalty(struct char_data *ch) {
    if (!IS_NPC(ch) && GET_CLASS(ch) == CLASS_MONK && GET_LEVEL(ch) >= 20) {
        int limit = CAN_CARRY_W(ch) * (0.8 + (GET_STR(ch) / 1000.0));

        if (IS_CARRYING_W(ch) < limit)
            return 0;

        return (IS_CARRYING_W(ch) - limit) / 10;
    } else
        return 0;
}

static int exp_table[LVL_IMPL + 1];

void init_exp_table(void) {
    long exp, last_exp;
    int lvl, i;

    const struct {
        int level;
        int static_add;
        int level_mod;
        int multiplier;
    } eval[] = {{0, 44000, 8, 12500},
                {17, 144000, 16, 17500},
                {25, 284000, 24, 22500},
                {33, 464000, 32, 25000},
                {49, 864000, 48, 30000},
                {90, 2094000, 89, 60000},
                {92, 2214000, 91, 70000},
                {95, 2424000, 94, 80000},
                {96, 2504000, 95, 90000},
                {98, 2684000, 97, 100000},
                {999, 0, 0, 0}};

    last_exp = 0;

    for (lvl = 0; lvl <= LVL_IMPL; ++lvl) {

        if (lvl < 9)
            exp = ((lvl * lvl) + lvl) / 2 * 5500;

        else if (lvl < 99) { /*(lvl <= LVL_IMMORT) { */
            for (i = 0; lvl >= eval[i].level; ++i)
                ;
            --i;
            exp = eval[i].static_add;
            exp += (lvl - eval[i].level_mod) * eval[i].multiplier;
            exp += last_exp;
        }

        else if (lvl == 99) {
            for (i = 0; lvl >= eval[i].level; ++i)
                ;
            --i;
            exp = eval[i].static_add;
            exp += (lvl - eval[i].level_mod) * eval[i].multiplier;

            for (i = 0; lvl + 1 >= eval[i].level; ++i)
                ;
            --i;
            exp += eval[i].static_add;
            exp += (lvl + 1 - eval[i].level_mod) * eval[i].multiplier;

            exp += last_exp;
        }

        else
            exp = 300000001 + 2 * (lvl - LVL_IMMORT - 1);

        exp_table[lvl] = exp;
        last_exp = exp;
    }
}

long exp_next_level(int level, int class) {
    double gain_factor;

    if (level > LVL_IMPL || level < 0) {
        log("SYSERR: Requesting exp for invalid level %d!", level);
        return 0;
    }

    /* God levels should all be the same for any class */
    if (level >= LVL_IMMORT)
        gain_factor = 1;
    else
        gain_factor = EXP_GAIN_FACTOR(class);

    return exp_table[level] * gain_factor;
}

/* log a death trap hit */
void log_death_trap(struct char_data *ch) {
    mprintf(L_STAT, LVL_IMMORT, "%s hit death trap #%d (%s)", GET_NAME(ch), world[IN_ROOM(ch)].vnum,
            world[IN_ROOM(ch)].name);
}

/* writes a string to the log */
void log(const char *str, ...) {
    char timestr[32];
    time_t ct;
    va_list args;

    static unsigned int vcount = 0;
    static unsigned int scount = 0;

    ct = time(0);
    strftime(timestr, 32, TIMEFMT_LOG, localtime(&ct));
    fprintf(stderr, "%-24.24s :: ", timestr);
    if (strchr(str, '%')) {
        va_start(args, str);
        vfprintf(stderr, str, args);
        va_end(args);
        ++vcount;
    } else {
        fputs(str, stderr);
        ++scount;
    }
    fputs("\n", stderr);

    /* TODO: check this debug data and determine if the split
     * for static strings is necessary
     */
    if ((vcount + scount) % 100 == 0)
        fprintf(stderr, "DEBUG :: log :: vprintf calls - %u, fputs calls - %u\n", vcount, scount);
}

/* the "touch" command, essentially. */
int touch(const char *path) {
    FILE *fl;

    if (!(fl = fopen(path, "a"))) {
        perror(path);
        return -1;
    } else {
        fclose(fl);
        return 0;
    }
}

/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(const char *str, unsigned char type, int level, byte file) {
    switch (type) {
    case OFF:
        type = L_CRIT;
        break;
    case BRF:
        type = L_WARN;
        break;
    case NRM:
        type = L_STAT;
        break;
    case CMP:
        type = L_DEBUG;
        break;
    }
    if (!file)
        type |= L_NOFILE;
    mudlog_printf(type, level, "%s", str);
}

void mudlog_printf(int severity, int level, const char *str, ...) {
    static char buf[MAX_STRING_LENGTH], timestr[32];
    struct descriptor_data *i;
    time_t ct;
    va_list args;
    size_t slen;

    ct = time(0);
    strftime(timestr, 32, TIMEFMT_LOG, localtime(&ct));

    va_start(args, str);
    slen = vsnprintf(buf + 2, sizeof(buf) - 2, str, args);
    va_end(args);
    if (slen >= sizeof(buf) - 7)
        slen = sizeof(buf) - 8;

    if (!IS_SET(severity, L_NOFILE))
        fprintf(stderr, "%-24.24s :: %s\n", timestr, buf + 2);
    /* Drop the L_NOFILE bit if it was set */
    REMOVE_BIT(severity, L_NOFILE);
    if (level < 0 || level > LVL_IMPL)
        return;

    /* Make it "[ buf ]" */
    buf[0] = '[';
    buf[1] = ' ';
    strcpy(buf + slen + 2, " ]\r\n");

    for (i = descriptor_list; i; i = i->next)
        if (!i->connected && !PLR_FLAGGED(i->character, PLR_WRITING) && !EDITING(i))
            if (GET_LEVEL(i->character) >= level && GET_LOG_VIEW(i->character) <= severity)
                string_to_output(i, buf);
}

const char *sprint_log_severity(int severity) {
    REMOVE_BIT(severity, L_NOFILE);

    return log_severities[LIMIT(0, (severity - 1) / 10, 6)];
}

int parse_log_severity(const char *severity) {
    int sev = search_block(severity, log_severities, FALSE);
    if (sev >= 0)
        return (sev + 1) * 10;
    else
        return sev;
}

flagvector *ALL_FLAGS = NULL;

void init_flagvectors() {
    const int num_flags[] = {
        NUM_EFF_FLAGS,       NUM_MOB_FLAGS,  NUM_PLR_FLAGS,      NUM_PRF_FLAGS,  NUM_PRV_FLAGS, NUM_ITEM_FLAGS,
        NUM_ITEM_WEAR_FLAGS, NUM_ROOM_FLAGS, NUM_ROOM_EFF_FLAGS, NUM_ITEM_FLAGS, MAX_EVENT,
    };
    int i, max = 0;

    for (i = 0; i < sizeof(num_flags) / sizeof(int); ++i)
        max = MAX(max, num_flags[i]);

    if (ALL_FLAGS)
        free(ALL_FLAGS);
    CREATE(ALL_FLAGS, flagvector, FLAGVECTOR_SIZE(max));
    for (i = 0; i < FLAGVECTOR_SIZE(max); ++i)
        ALL_FLAGS[i] = ~0;

    if (sizeof(flagvector) != 4) {
        log("SYSERR: WARNING! Flagvector type size isn't the expected 4 bytes!");
        log("SYSERR: WARNING! You may have to fix a lot of things...");
        log("SYSERR: WARNING! This may cause problems with player and world files "
            "especially.");
    }
}

bool ALL_FLAGGED(const flagvector field[], const flagvector flags[], const int num_flags) {
    int i;
    for (i = 0; i < FLAGVECTOR_SIZE(num_flags); ++i)
        if (flags[i] && !IS_SET(field[i], flags[i]))
            return FALSE;
    return TRUE;
}

bool ANY_FLAGGED(const flagvector field[], const flagvector flags[], const int num_flags) {
    int i;
    for (i = 0; i < FLAGVECTOR_SIZE(num_flags); ++i)
        if (IS_SET(field[i], flags[i]))
            return TRUE;
    return FALSE;
}

void SET_FLAGS(flagvector field[], const flagvector flags[], const int num_flags) {
    int i;
    for (i = 0; i < FLAGVECTOR_SIZE(num_flags); ++i)
        SET_BIT(field[i], flags[i]);
}

void REMOVE_FLAGS(flagvector field[], const flagvector flags[], const int num_flags) {
    int i;
    for (i = 0; i < FLAGVECTOR_SIZE(num_flags); ++i)
        REMOVE_BIT(field[i], flags[i]);
}

void TOGGLE_FLAGS(flagvector field[], const flagvector flags[], const int num_flags) {
    int i;
    for (i = 0; i < FLAGVECTOR_SIZE(num_flags); ++i)
        TOGGLE_BIT(field[i], flags[i]);
}

void COPY_FLAGS(flagvector field[], const flagvector flags[], const int num_flags) {
    int i;
    for (i = 0; i < FLAGVECTOR_SIZE(num_flags); ++i)
        field[i] = flags[i];
}

/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data real_time_passed(time_t t2, time_t t1) {
    long secs;
    struct time_info_data now;

    secs = (long)(t2 - t1);

    now.hours = (secs / SECS_PER_REAL_HOUR) % 24; /* 0..23 hours */
    secs -= SECS_PER_REAL_HOUR * now.hours;

    now.day = (secs / SECS_PER_REAL_DAY); /* 0..34 days  */
    secs -= SECS_PER_REAL_DAY * now.day;

    now.month = -1;
    now.year = -1;

    return now;
}

/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data mud_time_passed(time_t t2, time_t t1) {
    long secs;
    struct time_info_data now;

    secs = (long)(t2 - t1);

    now.hours = (secs / SECS_PER_MUD_HOUR) % 24; /* 0..23 hours */
    secs -= SECS_PER_MUD_HOUR * now.hours;

    now.day = (secs / SECS_PER_MUD_DAY) % 35; /* 0..34 days  */
    secs -= SECS_PER_MUD_DAY * now.day;

    now.month = (secs / SECS_PER_MUD_MONTH) % 17; /* 0..16 months */
    secs -= SECS_PER_MUD_MONTH * now.month;

    now.year = (secs / SECS_PER_MUD_YEAR); /* 0..XX? years */

    return now;
}

struct time_info_data age(struct char_data *ch) {
    struct time_info_data player_age;

    player_age = mud_time_passed(time(0), ch->player.time.birth);

    player_age.year += 17; /* All players start at 17 */

    return player_age;
}

EVENTFUNC(mobquit_event) {
    struct char_data *ch = (struct char_data *)event_obj;

    /* Critter is going to leave the game now */

    /* Send a message to those nearby */
    if (GET_POS(ch) < POS_SITTING)
        act("With a heroic effort, $n drags $mself to $s feet and runs off.", TRUE, ch, 0, 0, TO_ROOM);
    else
        act("$n turns and moves off, disappearing swiftly into the distance.", TRUE, ch, 0, 0, TO_ROOM);

    /* Destroy all of its items (else extract_char() will dump them
     * on the ground) */
    extract_objects(ch);

    /* And finally get rid of this critter. */
    extract_char(ch);
    return EVENT_FINISHED;
}

EVENTFUNC(autodouse_event) {
    struct char_data *ch = (struct char_data *)event_obj;

    if (EFF_FLAGGED(ch, EFF_ON_FIRE)) {
        act("$n's flames go out with a hiss as $e enters the water.", FALSE, ch, 0, 0, TO_ROOM);
        act("Your flames quickly go out as you enter the water.", FALSE, ch, 0, 0, TO_CHAR);
        REMOVE_FLAG(EFF_FLAGS(ch), EFF_ON_FIRE);
    }

    return EVENT_FINISHED;
}

void die_consentee_clean(struct char_data *ch) {
    /*aim is to search through character list and check if he been consented
       if so clean it up*/
    static struct char_data *i;

    for (i = character_list; i; i = i->next) {
        if (CONSENT(i) == ch) {
            send_to_char("&0&7&bThe person you consented to has left the game.&0\r\n", i);
            CONSENT(i) = NULL;
        }
    }
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE *fl, char *buf) {
    char temp[256];
    int lines = 0;

    temp[0] = 0; /* Otherwise, reading a 0-length file could lead to errors. */

    do {
        lines++;
        fgets(temp, 256, fl);
        if (*temp) {
            temp[strlen(temp) - 1] = '\0';
        }
    } while (!feof(fl) && (*temp == '*' || !*temp));

    if (feof(fl))
        return 0;
    else {
        strcpy(buf, temp);
        return lines;
    }
}

int num_pc_in_room(struct room_data *room) {
    int i = 0;
    struct char_data *ch;

    for (ch = room->people; ch != NULL; ch = ch->next_in_room)
        if (!IS_NPC(ch))
            i++;

    return i;
}

struct char_data *is_playing(char *vict_name) {
    struct descriptor_data *i;
    for (i = descriptor_list; i; i = i->next)
        if (i->connected == CON_PLAYING) {
            if (!str_cmp(GET_NAME(i->character), vict_name))
                return i->character;
            else if (i->original && !str_cmp(GET_NAME(i->original), vict_name))
                return i->original;
        }
    return NULL;
}

int load_modifier(struct char_data *ch) {
    int p, ccw;

    if (GET_LEVEL(ch) > 50)
        return 0;

    ccw = CAN_CARRY_W(ch);
    if (ccw < 1) {
        /* really light and/or wimpy NPCs */
        if (IS_CARRYING_W(ch) > 0)
            return 300;
        else
            return 0;
    }
    p = 100 - MAX(0, ((ccw - IS_CARRYING_W(ch)) * 100) / ccw);
    if (p < 20)
        return 25;
    if (p < 40)
        return 50;
    if (p < 65)
        return 100;
    if (p < 85)
        return 150;
    if (p < 95)
        return 200;
    return 300;
}

const char *movewords(struct char_data *ch, int dir, int room, int leaving) {
    if (GET_POS(ch) == POS_FLYING)
        return (leaving ? "flies" : "flies in");

    if (EFF_FLAGGED(ch, EFF_LEVITATE) && (dir == DOWN || dir == UP))
        return (leaving ? "floats" : "floats in");

    if (IS_WATER(ch->in_room)) {
        if (SECT(ch->in_room) == SECT_UNDERWATER)
            if (EFF_FLAGGED(ch, EFF_WATERBREATH))
                return (leaving ? "swims" : "swims in");

        if (SECT(ch->in_room) == SECT_SHALLOWS)
            return (leaving ? "swims" : "swims in");

        if (SECT(ch->in_room) == SECT_WATER)
            return (leaving ? "floats" : "floats in");

        return (leaving ? "thrashes" : "thrashes in");
    }

    if (load_modifier(ch) > 199)
        return (leaving ? "trudges" : "trudges in");

    if (!IS_NPC(ch) && GET_COND(ch, DRUNK) > 6)
        return (leaving ? "staggers" : "staggers in");

    if (EFF_FLAGGED(ch, EFF_BLIND) || (IS_DARK(room) && !EFF_FLAGGED(ch, EFF_INFRAVISION)))
        if (!PRF_FLAGGED(ch, PRF_HOLYLIGHT))
            return (leaving ? "stumbles" : "stumbles in");

    if (IS_HIDDEN(ch) || OUTDOOR_SNEAK(ch) || EFF_FLAGGED(ch, EFF_SNEAK))
        return (leaving ? "sneaks" : "sneaks in");

    if (GET_POS(ch) == POS_PRONE)
        return (leaving ? "slithers" : "slithers in");

    if (GET_POS(ch) == POS_KNEELING || GET_POS(ch) == POS_SITTING)
        return (leaving ? "crawls" : "crawls in");

    if (GET_LIFEFORCE(ch) == LIFE_UNDEAD)
        return (leaving ? "shambles" : "shambles in");

    if (leaving) {
        if (VALID_RACE(ch) && races[GET_RACE(ch)].leave_verb)
            return races[GET_RACE(ch)].leave_verb;
        else
            return "leaves";
    } else {
        if (VALID_RACE(ch) && races[GET_RACE(ch)].enter_verb)
            return races[GET_RACE(ch)].enter_verb;
        else
            return "enters";
    }
}

void build_count() {
    FILE *fd;
    extern int make_count;
    if ((fd = fopen(MAKE_COUNT, "r"))) {
        fscanf(fd, "%d", &make_count);
        fclose(fd);
    }
}

int pick_random_gem_drop(struct char_data *ch) {

    int slot;
    int phase;
    int roll;

    int armor_vnums[] = {

        55300, 55301, 55302, 55304, 55305, 55306, 55308, 55309, 55310, 55312, 55313, 55314, 55316, 55317, 55318,
        55320, 55321, 55322, 55324, 55325, 55326, 55328, 55329, 55330, 55331, 55332, 55333, 55334, 55335, 55336,

        55337, 55338, 55339, 55340, 55341, 55342, 55343, 55344, 55345, 55346, 55347, 55348, 55349, 55350, 55351,
        55352, 55353, 55354, 55355, 55356, 55357, 55358, 55359, 55360, 55361, 55362, 55363, 55364, 55365, 55366,

        55367, 55368, 55369, 55370, 55371, 55372, 55373, 55374, 55375, 55376, 55377, 55378,

        55379, 55380, 55381, 55382, 55383,
    };

    int leg_gem_vnums[][2] = {{55586, 55589}, {55649, 55659}, {55726, 55736}};

    int common_gem_vnums[][2] = {{55566, 55569}, {55570, 55573}, {55574, 55577}, {55578, 55581}, {55582, 55585},
                                 {55594, 55604}, {55605, 55615}, {55616, 55626}, {55627, 55637}, {55638, 55648},
                                 {55671, 55681}, {55682, 55693}, {55693, 55703}, {55704, 55714}, {55715, 55725}};

    if (!IS_NPC(ch) || ch->desc || GET_LEVEL(ch) > 60)
        return 0;

    if (number(1, 100) > 7)
        return 0;

    slot = GET_LEVEL(ch) / 4;
    phase = GET_LEVEL(ch) / 20;

    roll = number(1, 100);

    /* You win the shiny prize!  The shiny prize is rusted decayed armor. */
    if (roll > 98) {

        int item = 4 - number(1, 7) + GET_LEVEL(ch);

        if (item < 0)
            item = 0;
        if (item > 63)
            item = 63;

        return armor_vnums[item];
    }

    roll = number(1, 100);

    if (roll < 30) {
        slot--;
    } else if (roll < 80) {
    } else if (roll < 95) {
        slot++;
    } else {
        /* If we're in the highest section of a phase, there's a chance of leg gems.
         */
        if (slot % 5 == 4) {
            return number(leg_gem_vnums[phase][0], leg_gem_vnums[phase][1]);
        }

        slot++;
        slot++;
    }

    if (slot < 0)
        slot = 0;
    if (slot > 14)
        slot = 14;

    return number(common_gem_vnums[slot][0], common_gem_vnums[slot][1]);
}

void perform_random_gem_drop(struct char_data *ch) {
    int vnum = pick_random_gem_drop(ch);
    int rnum;
    struct obj_data *od;

    if (!vnum)
        return;

    rnum = real_object(vnum);

    if (rnum < 0) {
        sprintf(buf, "SYSERR: Can't perform random gem drop - no object with vnum %d", vnum);
        mudlog(buf, NRM, LVL_IMMORT, TRUE);
        return;
    }

    od = read_object(rnum, REAL);

    if (!od) {
        sprintf(buf, "RGD Error: Could not read object (vnum %d)!", vnum);
        mudlog(buf, BRF, LVL_IMMORT, TRUE);
        return;
    }

    obj_to_char(od, ch);
}

int yesno_result(char *answer) {
    if (!answer || !(*answer))
        return YESNO_NONE;
    else if (answer[0] == 'y' || answer[0] == 'Y')
        return YESNO_YES;
    else if (answer[0] == 'n' || answer[0] == 'N')
        return YESNO_NO;
    else
        return YESNO_OTHER;
}

/* Given a vnum, find out its zone's index into the zone table. */
int find_zone(int num) {
    int i;

    for (i = 0; zone_table[i].number != num && i <= top_of_zone_table; i++)
        ;
    if (i <= top_of_zone_table)
        return i;
    return -1;
}

/* Given an IP address string, produce a string representation of it,
 * with decimal octets (e.g., 255.255.255.255), without leading zeros.
 *
 * Returns 0 on success or 1 if the address is invalid.
 */

int normalize_ip_address(char *in, char *out) {
    int octets[4];
    int octet = 0;
    char *inx = in;
    int octval = 0;

    while (*inx && octet < 4) {
        switch (*inx) {
        case '.':
            if (octval > 255) {
                /* Invalid value */
                return 1;
            }
            octets[octet++] = octval;
            octval = 0;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            octval = octval * 10 + (int)(*inx - '0');
            break;
        default:
            /* Invalid character */
            return 1;
        }
        inx++;
    }
    if (octval > 255 || octet != 3) {
        /* Invalid value or incorrect number of octets */
        return 1;
    }
    octets[octet] = octval;
    sprintf(out, "%d.%d.%d.%d", octets[0], octets[1], octets[2], octets[3]);
    return 0;
}

char *statelength(int inches) {
    static char res0[400];
    static char res1[400];
    static int resptr = 0;
    char *res = res0;

    if (resptr == 0) {
        resptr = 1;
        res = res1;
    } else {
        resptr = 0;
        res = res0;
    }

    if (inches < 12)
        sprintf(res, "%d inch%s", inches, inches == 1 ? "" : "es");
    else if (inches < 1200 && inches % 12)
        sprintf(res, "%d %s, %d inch%s", inches / 12, inches / 12 == 1 ? "foot" : "feet", inches % 12,
                inches % 12 == 1 ? "" : "es");
    else
        sprintf(res, "%d %s", inches / 12, inches / 12 == 1 ? "foot" : "feet");

    return res;
}

char *stateweight(float pounds) {
    static char res0[400];
    static char res1[400];
    static int resptr = 0;
    char *res = res0;
    int len;

    if (resptr == 0) {
        resptr = 1;
        res = res1;
    } else {
        resptr = 0;
        res = res0;
    }

    if (pounds < 2000)
        sprintf(res, "%.2f pound%s", pounds, pounds == 1 ? "" : "s");
    else if (pounds < 2100)
        sprintf(res, "1 ton");
    else {
        sprintf(res, "%0.1f", pounds / 2000.0);
        /* Lop off trailing ".0" */
        len = strlen(res);
        if (len > 2 && res[len - 1] == '0' && res[len - 2] == '.')
            res[len - 2] = '\0';
        strcat(res, " tons");
    }

    return res;
}

/* parse_obj_name()
 *
 * This function is used to translate a string into a constant.
 *
 * Whenever you have an array of structs, and the first element of the struct
 * is a string that's the name of the object, this function can be used to
 * find out which struct in the array has a given name.
 *
 * Example: size = parse_obj_name(ch, arg, "size", NUM_SIZES, sizes,
 * sizeof(struct sizedef));
 *
 *    ch          Optional.  A character who has typed something to be
 * identified and may need feedback. arg         The string to identify objname
 * What kind of thing we're identifying.  Only needed if ch is not NULL. numobjs
 * How many kinds of this thing exist? objects     The array of structs. objsize
 * The size of the structs.
 */

struct objdef {
    char *name;
};

#define OBJNAME(i) (((struct objdef *)((char *)(objects) + objsize * i))->name)

int parse_obj_name(struct char_data *ch, char *arg, char *objname, int numobjs, void *objects, int objsize) {
    int i, answer = -1, best = -1;

    if (!*arg) {
        if (ch) {
            sprintf(buf, "What %s?\r\n", objname);
            send_to_char(buf, ch);
        }
        return -1;
    }

    if (isdigit(arg[0])) {
        answer = atoi(arg);
        if (answer < 0 || answer >= numobjs)
            answer = -1;
    } else
        for (i = 0; i < numobjs; i++) {
            if (!strncasecmp(arg, OBJNAME(i), strlen(arg))) {
                if (!strcasecmp(arg, OBJNAME(i))) {
                    answer = i;
                    break;
                }
                if (best == -1)
                    best = i;
            } else if (is_abbrev(arg, OBJNAME(i))) {
                if (best == -1)
                    best = i;
            }
        }

    if (answer == -1)
        answer = best;
    if (answer == -1) {
        if (ch) {
            sprintf(buf, "There is no such %s.\r\n", objname);
            send_to_char(buf, ch);
        }
    }
    return answer;
}

char *format_apply(int apply, int modifier) {
    static char f[MAX_STRING_LENGTH];

    if (apply == APPLY_COMPOSITION) {
        if (modifier >= 0 && modifier < NUM_COMPOSITIONS)
            sprintf(f, "COMPOSITION set to %s%s&0", compositions[modifier].color, compositions[modifier].name);
        else
            sprintf(f, "&3&bInvalid Composition!&0");
    } else if (apply > 0) {
        sprinttype(apply, apply_types, buf2);
        sprintf(f, "%+d to %s", modifier, buf2);
    } else {
        sprintf(f, "None.");
    }
    return f;
}

/* Sort functions */

void sort(void algorithm(int[], int, int(int a, int b)), int array[], int count, int comparator(int, int)) {
    algorithm(array, count, comparator);
}

int natural_order(int a, int b) { return (a > b ? 1 : (a < b ? -1 : 0)); }

void bubblesort(int array[], int count, int comparator(int a, int b)) {
    int i, t;
    bool swap;

    do {
        swap = FALSE;
        --count;
        for (i = 0; i < count; ++i)
            if (comparator(array[i], array[i + 1]) > 0) {
                t = array[i];
                array[i] = array[i + 1];
                array[i + 1] = t;
                swap = TRUE;
            }
    } while (swap);
}

void insertsort(int array[], int count, int comparator(int, int)) {
    int i, value, j;

    for (i = 1; i < count; ++i) {
        value = array[i];
        j = i - 1;
        for (j = i - 1; j >= 0 && comparator(array[j], value) > 0; --j)
            array[j + 1] = array[j];
        array[j + 1] = value;
    }
}

static void do_quicksort(int array[], int start, int end, int comparator(int a, int b)) {
    if (end > start + 1) {
        int pivot = array[start], left = start + 1, right = end, temp;
        while (left < right) {
            if (comparator(array[left], pivot) <= 0)
                ++left;
            else {
                --right;
                temp = array[left];
                array[left] = array[right];
                array[right] = temp;
            }
        }
        --left;
        temp = array[left];
        array[left] = array[start];
        array[start] = temp;
        do_quicksort(array, start, left, comparator);
        do_quicksort(array, right, end, comparator);
    }
}

void quicksort(int array[], int count, int comparator(int a, int b)) { do_quicksort(array, 0, count, comparator); }

void optquicksort(int array[], int count, int comparator(int a, int b)) {
#define MAX_LEVELS 1000

    int pivot, start[MAX_LEVELS], end[MAX_LEVELS], i = 0, L, R;

    start[0] = 0;
    end[0] = count;
    while (i >= 0) {
        L = start[i];
        R = end[i] - 1;
        if (L < R) {
            pivot = array[L];
            if (i == MAX_LEVELS - 1)
                return /* FALSE */;
            while (L < R) {
                while (comparator(array[R], pivot) >= 0 && L < R)
                    R--;
                if (L < R)
                    array[L++] = array[R];
                while (comparator(array[L], pivot) <= 0 && L < R)
                    L++;
                if (L < R)
                    array[R--] = array[L];
            }
            array[L] = pivot;
            start[i + 1] = L + 1;
            end[i + 1] = end[i];
            end[i++] = L;
        } else
            --i;
    }
    return /* TRUE */;

#undef MAX_LEVELS
}

void drop_core(struct char_data *ch, const char *desc) {
    static int corenum = 0;
    char initcorename[MAX_STRING_LENGTH];
    char corename[MAX_STRING_LENGTH];
    struct stat finfo;
    bool dropped = FALSE;

    pid_t child = fork();

    if (child == 0) {
        abort();
    } else {
        if (desc && *desc) {
            sprintf(corename, "core-%s-%d.%d", desc, corenum, getpid());
        } else {
            sprintf(corename, "core-%d.%d", corenum, getpid());
        }
        corenum++;
        waitpid(child, 0, 0);
        sprintf(initcorename, "core.%d", child);
        if (stat(initcorename, &finfo) == 0) {
            rename(initcorename, corename);
            dropped = TRUE;
        } else if (stat("core", &finfo) == 0) {
            rename("core", corename);
            dropped = TRUE;
        } else {
            log("SYSERR: Could not find core file named %s or %s", initcorename, "core");
        }
        if (ch) {
            if (dropped) {
                cprintf(ch, "The core was dumped to %s\r\n", corename);
            } else {
                cprintf(ch, "Sorry, the core dump failed!\r\n");
            }
        }
    }
}

/***************************************************************************
 * $Log: utils.c,v $
 * Revision 1.130  2010/06/20 19:53:47  mud
 * Log to file errors we might want to see.
 *
 * Revision 1.129  2009/06/10 02:27:14  myc
 * Added an optimization to log which checks to see if the message
 * to be printed has any %'s before attempting to vfprintf.  This
 * can save some time, but to see whether it's really worth it,
 * we're going to count the number of times each branch is used and
 * log it.
 *
 * Revision 1.128  2009/06/09 05:51:26  myc
 * Adding NUM_PRV_FLAGS to the init_flagvectors array.
 *
 * Revision 1.127  2009/03/09 05:51:25  jps
 * Moved some money-related functions from utils to money
 *
 * Revision 1.126  2009/03/09 05:41:31  jps
 * Moved money stuff into money.h, money.c
 *
 * Revision 1.125  2009/03/09 03:33:03  myc
 * Split off string functions from this file into strings.c
 *
 * Revision 1.124  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.123  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.122  2009/02/21 03:30:16  myc
 * Removed L_FILE flag--mprintf now logs to file by default;
 * assert L_NOFILE to prevent that.
 *
 * Revision 1.121  2009/02/11 17:03:39  myc
 * Add smash_tilde(), which removes tildes from the end of lines.
 * Make str_ functions take const formats.  Check EDITING(d)
 * where PLR_WRITING is checked in mudlog.
 *
 * Revision 1.120  2009/01/17 00:28:02  myc
 * Fix possible use of uninitialized variable.
 *
 * Revision 1.119  2008/09/25 04:47:49  jps
 * Add drop_core() function, to drop the mud's core without terminating.
 *
 * Revision 1.118  2008/09/22 02:09:17  jps
 * Changed weight into a floating-point value. Precision is preserved to
 * the 1/100 place.
 *
 * Revision 1.117  2008/09/21 21:21:29  jps
 * Remove debugging printf.
 *
 * Revision 1.116  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.115  2008/09/07 20:06:13  jps
 * Changed the exp needed for level 100 so that it's the same as the amount
 * we've always been requiring due to a bug.
 *
 * Revision 1.114  2008/09/07 18:45:15  jps
 * Added briefmoney function for printing the top two coins of a money
 * value in a given number of spaces. With color.
 *
 * Revision 1.113  2008/09/04 06:47:36  jps
 * Changed sector constants to match their strings
 *
 * Revision 1.112  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.111  2008/08/31 04:17:55  myc
 * Fix exp for level 100.
 *
 * Revision 1.110  2008/08/30 04:13:45  myc
 * Replaced the exp_to_level monstrosity with a lookup table that gets
 * populated at boot time.
 *
 * Revision 1.109  2008/08/29 04:16:26  myc
 * Get rid of the trailing space in sprintflag.
 *
 * Revision 1.108  2008/08/16 21:33:01  jps
 * Allow a negative return value if you pass an invalid syslog severity.
 *
 * Revision 1.107  2008/08/14 23:10:35  myc
 * Added vararg functionality to log() and mudlog().  mprintf() is
 * the new vararg mudlog().  The old non-vararg mudlog() is still
 * available.  Added graduated log severity to the mudlog.
 *
 * Revision 1.106  2008/08/03 21:32:32  jps
 * Stop generating a log entry every time someone leaves the game when
 * someone was consented to that person.
 *
 * Revision 1.105  2008/06/21 17:26:33  jps
 * Moved movement strings into race definitions.
 *
 * Revision 1.104  2008/06/05 02:07:43  myc
 * Replaced strip_cr with filter_chars/strip_chars.
 *
 * Revision 1.103  2008/05/18 05:39:59  jps
 * Changed room_data member number to "vnum".
 *
 * Revision 1.102  2008/04/19 21:11:22  myc
 * Added some general-purpose integer array sorting functions.
 * Right now, we've got bubble sort, insertion sort, quicksort,
 * and some apparently-optimized quicksort I found on Google :)
 *
 * Revision 1.101  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.100  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.99  2008/04/02 19:31:02  myc
 * Added str_catf functions and used them in do_stat functions.
 *
 * Revision 1.98  2008/04/02 04:55:59  myc
 * Added a parse money function.
 *
 * Revision 1.97  2008/04/02 03:24:44  myc
 * Rewrote group code and removed major group code.
 *
 * Revision 1.96  2008/03/30 17:30:38  jps
 * Renamed objsave.c to pfiles.c and introduced pfiles.h. Files using functions
 * from pfiles.c now include pfiles.h and depend on it in the makefile.
 *
 * Revision 1.95  2008/03/29 21:14:37  myc
 * Fix not-very-potential memory leak.
 *
 * Revision 1.94  2008/03/29 17:34:55  myc
 * Autodouse event memory leak.
 *
 * Revision 1.93  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.92  2008/03/26 22:22:56  jps
 * Better error checking/reporting in perform_random_gem_drop.
 *
 * Revision 1.91  2008/03/26 16:44:36  jps
 * Replaced all checks for undead race with checks for undead lifeforce.
 * Replaced the undead race with the plant race.
 *
 * Revision 1.90  2008/03/23 00:25:23  jps
 * Add a function to format applies, since it's done in at least
 * three places in the server.
 *
 * Revision 1.89  2008/03/22 21:24:25  jps
 * Some code formatting.
 *
 * Revision 1.88  2008/03/22 19:50:24  myc
 * Added strnlen implementation.
 *
 * Revision 1.87  2008/03/22 19:08:58  jps
 * Added parse_obj_name() function, which is a generalized function
 * for identifying structs.
 *
 * Revision 1.86  2008/03/21 21:59:31  jps
 * Add some more methods of pluralizing.
 *
 * Revision 1.85  2008/03/21 21:36:02  jps
 * Add functions without_article and pluralize, for modifying
 * nouns and noun phrases.
 *
 * Revision 1.84  2008/03/11 02:57:16  jps
 * Change state_weight and state_length to allow single-reentrancy.
 * Also, the ".0" on the end of a "tons" statement is suppressed.
 *
 * Revision 1.83  2008/03/10 20:49:47  myc
 * Renamed POS1 to 'stance'.
 *
 * Revision 1.82  2008/03/08 18:55:20  jps
 * Fix strip_cr
 *
 * Revision 1.81  2008/03/05 03:03:54  myc
 * Added sprintascii function.  Moved get_filename to players.c.  Got
 * rid of BOUNDED.  Added strip_cr and trim_spaces.
 *
 * Revision 1.80  2008/02/16 20:31:32  myc
 * Commented out str_dup to help disambiguate memory leaks.
 *
 * Revision 1.79  2008/02/09 21:07:50  myc
 * Must provide a boolean to event_create saying whether to
 * free the event obj when done or not..
 *
 * Revision 1.78  2008/02/09 18:29:11  myc
 * The event code now takes care of freeing event objects.
 *
 * Revision 1.77  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.76  2008/02/09 03:06:17  myc
 * Moving mathematical functions to math.c.
 *
 * Revision 1.75  2008/02/06 21:53:53  myc
 * Slight bug in the levenshtein_distance function.
 *
 * Revision 1.74  2008/02/02 19:38:20  myc
 * Added a levenshtein distance calculator for use by the interpreter.
 * Also adding a count_color_chars function.
 *
 * Revision 1.73  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.72  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species
 *into races.
 *
 * Revision 1.71  2008/01/27 12:11:45  jps
 * Use exp factor from class.c.
 *
 * Revision 1.70  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.69  2008/01/25 21:11:52  myc
 * Renamed monk_weight_pen to monk_weight_penalty.
 *
 * Revision 1.68  2008/01/15 03:18:19  myc
 * Changed statemoney to accept an array instead of a pointer.
 *
 * Revision 1.67  2008/01/13 23:06:04  myc
 * Removed some unused functions: number_of_groupees (use group_size
 * instead), NumAttackers, SanityCheck, statsave, STAT_INDEX,
 * are_together (use is_grouped), and has_help.  Also cleaned up
 * movewords a bunch.
 *
 * Revision 1.66  2008/01/09 08:34:36  jps
 * Add functions to format strings for printin lengths and weights.
 *
 * Revision 1.65  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.64  2008/01/03 12:46:20  jps
 * New string function with_indefinite_article.
 * Renamed CLASS_MAGIC_USER to CLASS_SORCERER.
 *
 * Revision 1.63  2007/12/25 20:31:54  jps
 * Also make exp_group_bonus() not overflow.
 *
 * Revision 1.62  2007/12/25 20:28:57  jps
 * Change exp_highlevel_bonus() not to overflow with large values.
 *
 * Revision 1.61  2007/12/25 05:49:58  jps
 * event_target_valid() is of no further use in many places.
 * Actually, it sucks hard, but it's better than nothing in some
 * other places (such as fight.c).
 *
 * Revision 1.60  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.59  2007/12/20 03:04:11  jps
 * Fix ip formatting func.
 *
 * Revision 1.58  2007/12/19 20:57:58  myc
 * Added const modifiers to str_cmp, strn_cmp, log, touch, and mudlog.
 * Made is_playing check descriptor->original too.
 *
 * Revision 1.57  2007/11/22 23:33:23  jps
 * Added normalize_ip_address function.
 *
 * Revision 1.56  2007/10/11 20:35:06  myc
 * Since monks got nerfed a long time ago, decreasing the exp to level.
 *
 * Revision 1.55  2007/10/02 02:52:27  myc
 * Added AFF_SNEAK back in, so changed movewords.  Got rid of
 * stop_ignoring_me.  There's no way to check who you are ignoring now.
 *
 * Revision 1.54  2007/09/21 01:20:19  myc
 * Fixing escape_ansi's behavior with &&.
 *
 * Revision 1.53  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  Changing movewords to handle this.
 *
 * Revision 1.52  2007/09/03 22:53:54  jps
 * Make a person's flames go out automatically if they enter a water room.
 *
 * Revision 1.51  2007/08/14 22:43:07  myc
 * Adding conceal, corner, shadow, and stealth skills.
 *
 * Revision 1.50  2007/08/04 14:40:35  myc
 * sprintbit couldn't handle the 32nd bit previously, because of an infinite
 * loop bug.  Now it's fine.
 *
 * Revision 1.49  2007/07/19 21:59:20  jps
 * Add utility funciton next_line.
 *
 * Revision 1.48  2007/07/14 04:18:11  jps
 * Pets (i.e., charmed mobs that were not charmed by a spell) will now
 * quit the game when attacked by their master, or when their master
 * leaves the game. The main reason is to keep powerful summoned
 * mounts out of the hands of other players, but it will also cut
 * back on clutter from ordinary mounts and pets.
 *
 * Revision 1.47  2007/07/02 04:40:05  jps
 * Fix off-by-one-crash-causing bug in format_text.
 *
 * Revision 1.46  2007/07/02 04:22:32  jps
 * Fix str_replace so you can't cause segfaults by increasing buffers
 * by one byte with /r command. And removed a minor memory leak.
 *
 * Revision 1.45  2007/06/24 02:51:44  jps
 * Add function startsvowel.
 *
 * Revision 1.44  2007/05/29 00:36:03  jps
 * Make a utility function find_zone, to find a zone's entry in the zone
 * table, from a vnum.
 *
 * Revision 1.43  2007/05/11 19:34:15  myc
 * Modified the quest command functions so they are thin wrappers for
 * perform_quest() in quest.c.  Error handling and messages should be
 * much better now.  Advance and rewind now accept another argument
 * specifying how many stages to advance or rewind.
 *
 * Revision 1.42  2007/04/18 00:05:59  myc
 * Prompt parser has been totally rewritten so it won't print garbage
 * characters anymore.  Also, some new features were added.  Giving the
 * prompt command back to mortals.
 *
 * Revision 1.41  2007/04/04 13:31:02  jps
 * Add year to log timestamps and other dates.
 *
 * Revision 1.40  2007/03/27 04:27:05  myc
 * Fixed crash bug in is_grouped...major group should be better behaved
 * now.
 *
 * Revision 1.39  2006/11/26 08:31:17  jps
 * Added function yesno_result to standardize handling of nanny's
 * yes/no questions (in interpreter.c).
 *
 * Revision 1.38  2006/11/18 09:08:15  jps
 * Add function statemoney to pretty-print coins
 *
 * Revision 1.37  2006/11/18 07:22:34  jps
 * Add isplural function
 *
 * Revision 1.36  2006/11/13 15:54:22  jps
 * Fix widespread misuse of the hide_invisible parameter to act().
 *
 * Revision 1.35  2006/11/13 02:48:27  jps
 * Fix pointer bug when reading a 0-length file.
 *
 * Revision 1.34  2006/11/11 16:13:08  jps
 * Fix CAP so it correctly capitalizes strings with color codes at the
 *beginning.
 *
 * Revision 1.33  2006/11/08 09:16:40  jps
 * Fixed missing punctuation in "You have become new leader" messages
 *
 * Revision 1.32  2006/11/08 05:29:11  jps
 * Better code for trimming the end of formatted descs.
 *
 * Revision 1.31  2006/11/07 18:02:05  jps
 * Stop extra blank lines from appearing after /f or /fi formatting.
 *
 * Revision 1.30  2006/07/20 07:39:31  cjd
 * Typo fixes.
 *
 * Revision 1.29  2003/08/04 02:11:30  jjl
 * Enabled random gem drop on production.
 *
 * Revision 1.28  2003/08/02 17:42:50  jjl
 * Added a random gem drop for mobs under level 60, for Zzur's insano-quest.
 *
 * Revision 1.27  2003/06/23 01:47:09  jjl
 * Added a NOFOLLOW flag, and the "note" command, and show notes <player>
 *
 * Revision 1.26  2003/02/01 07:27:54  jjl
 * Bout time.  This should fix (at least part) of the necro / zombie crash
 * bug.  Sooooomeone used a 50 char buffer to store 80 character strings.
 * *sigh*
 *
 * Revision 1.25  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.24  2001/03/13 01:07:34  dce
 * Fixed some typos.
 *
 * Revision 1.23  2001/01/23 04:05:43  dce
 * Fixed a crash bug in the stop_groupee function. Someone put a mud
 * log before the free statement. And because the GET_NAME(ch) is no
 * longer valid it was causing a crash. So it was the mudlog fuction
 * sending nulls, that made it look like the free(k) was the problem
 * this was also a problem for multiple groupees. Both are fixed.
 *
 * Revision 1.22  2000/11/25 02:33:15  rsd
 * Altered comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.21  2000/10/27 00:34:45  mtp
 * extra define for saving quests info
 *
 * Revision 1.20  2000/04/22 22:44:10  rsd
 * fixed comment header. retabbed and braced sections of the code.
 * Also fixed grammital error in disband.
 *
 * Revision 1.19  1999/09/16 01:15:11  dce
 * Weight restrictions for monks...-hitroll, -damroll + ac
 *
 * Revision 1.18  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.17  1999/09/04 18:46:52  jimmy
 * More small but important bug fixes found with insure.  These are all runtime
 *fixes.
 *
 * Revision 1.16  1999/08/14 02:43:10  dce
 * ** is one level up from 99
 *
 * Revision 1.15  1999/08/12 17:54:46  dce
 * Fixed experience so that there are no overflows of integers that are placed
 *into longs. Main problem was max_exp_gain and max_exp_loss. Both were
 *overflowing due to poor Hubis coding.
 *
 * Revision 1.14  1999/08/12 04:25:39  jimmy
 * This is a Mass ci of the new pfile system.  The pfile has been split into
 * one file for each player in a directory A-Z.  The object files are also
 * located in the A-Z directories.  Fixed a stupid bug in pfilemaint that
 * screwed up the IDNUM of the person who typed it.  Commented out the frag
 * system completely.  It is slated for removal.  Fixed the rename command.
 * Fixed all supporting functions for the new system, I hope!
 * --Gurlaek 8/11/1999
 *
 * Revision 1.13  1999/06/14 21:41:16  mud
 * fixed the monk XP multiplier to 2.5 where it should be. This was overwritten
 *by someone who checked in changes between version 1.09 and 1.10 without
 *diffing.
 * --gurlaek 6/14/1999
 *
 * Revision 1.12  1999/06/11 17:18:40  jimmy
 * removed a really stupid log message that was spamming the syslog
 *
 * Revision 1.11  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list.  This code compiles fine under both gcc RH5.2 and egcs RH6.0
 *
 * Revision 1.10  1999/05/04 17:19:33  dce
 * Name accept system...version one...original code by Fingh, fixed up to work
 * by Zantir.
 *
 * Revision 1.9  1999/05/02 20:29:30  mud
 * Ok, moved monks to an exp-factor of 2.5 in a compromise
 * with zantir grumble
 *
 * Revision 1.8  1999/05/02 20:25:51  mud
 * made the exp-factor 3.0 for the overpowered monks
 * take it back down when monks are balanced.
 *
 * Revision 1.7  1999/04/21 04:05:04  dce
 * New Exp system.
 * Exp_fix.c is the converter.
 *
 * Revision 1.6  1999/04/08 03:59:20  dce
 * Fixed a crash bug
 *
 * Revision 1.5  1999/03/25 20:18:18  jimmy
 * Added IS_NPC check to DRUNK code in movewards()
 * fingon
 *
 * Revision 1.4  1999/02/06 02:06:21  jimmy
 * I have no idea why, but a debug statment log(name)
 * actually makes this work.  Hmmmm.  Can't pfile wipe
 * without it.
 * --Fingon
 *
 * Revision 1.3  1999/02/03 21:03:25  mud
 * dos2unix
 * Indented file
 *
 * Revision 1.2  1999/02/01 08:20:33  jimmy
 * improved build counter
 *
 * Revision 1.1  1999/01/29 01:23:32  mud
 * Initial revision
 *
 ***************************************************************************/
