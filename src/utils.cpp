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

#include "utils.hpp"

#include "casting.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "events.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "lifeforce.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "money.hpp"
#include "pfiles.hpp"
#include "races.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "weather.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <memory>
#include <string>
#include <sys/stat.h> /* For stat() */
#include <vector>

flagvector *ALL_FLAGS;

int monk_weight_penalty(CharData *ch) {
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

long exp_next_level(int level, int class_num) {
    double gain_factor;

    if (level > LVL_IMPL || level < 0) {
        log("SYSERR: Requesting exp for invalid level {:d}!", level);
        return 0;
    }

    /* God levels should all be the same for any class_num */
    if (level >= LVL_IMMORT)
        gain_factor = 1;
    else
        gain_factor = EXP_GAIN_FACTOR(class_num);

    return exp_table[level] * gain_factor;
}

/* the "touch" command, essentially. */
int touch(const std::string_view path) {
    FILE *fl;

    if (!(fl = fopen(std::string(path), "a"))) {
        perror(std::string(path));
        return -1;
    } else {
        fclose(fl);
        return 0;
    }
}

void init_flagvectors() {
    const int num_flags[] = {
        NUM_EFF_FLAGS,       NUM_MOB_FLAGS,  NUM_PLR_FLAGS,      NUM_PRF_FLAGS,  NUM_PRV_FLAGS, NUM_ITEM_FLAGS,
        NUM_ITEM_WEAR_FLAGS, NUM_ROOM_FLAGS, NUM_ROOM_EFF_FLAGS, NUM_ITEM_FLAGS, MAX_EVENT,
    };
    int i, max = 0;

    for (i = 0; i < sizeof(num_flags) / sizeof(int); ++i)
        max = std::max(max, num_flags[i]);

    if (ALL_FLAGS)
        free(ALL_FLAGS);
    CREATE(ALL_FLAGS, flagvector, FLAGVECTOR_SIZE(max));
    for (i = 0; i < FLAGVECTOR_SIZE(max); ++i)
        ALL_FLAGS[i] = ~0;
}

bool ALL_FLAGGED(const flagvector field[], const flagvector flags[], const int num_flags) {
    int i;
    for (i = 0; i < FLAGVECTOR_SIZE(num_flags); ++i)
        if (flags[i] && !IS_SET(field[i], flags[i]))
            return false;
    return true;
}

bool ANY_FLAGGED(const flagvector field[], const flagvector flags[], const int num_flags) {
    int i;
    for (i = 0; i < FLAGVECTOR_SIZE(num_flags); ++i)
        if (IS_SET(field[i], flags[i]))
            return true;
    return false;
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
TimeInfoData real_time_passed(time_t t2, time_t t1) {
    long secs;
    TimeInfoData now;

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
TimeInfoData mud_time_passed(time_t t2, time_t t1) {
    long secs;
    TimeInfoData now;

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

TimeInfoData age(CharData *ch) {
    TimeInfoData player_age;

    player_age = mud_time_passed(time(0), ch->player.time.birth);

    player_age.year += 17; /* All players start at 17 */

    return player_age;
}

EVENTFUNC(mobquit_event) {
    CharData *ch = (CharData *)event_obj;

    /* Critter is going to leave the game now */

    /* Send a message to those nearby */
    if (GET_POS(ch) < POS_SITTING)
        act("With a heroic effort, $n drags $mself to $s feet and runs off.", true, ch, 0, 0, TO_ROOM);
    else
        act("$n turns and moves off, disappearing swiftly into the distance.", true, ch, 0, 0, TO_ROOM);

    /* Destroy all of its items (else extract_char() will dump them
     * on the ground) */
    extract_objects(ch);

    /* And finally get rid of this critter. */
    extract_char(ch);
    return EVENT_FINISHED;
}

EVENTFUNC(autodouse_event) {
    CharData *ch = (CharData *)event_obj;

    if (EFF_FLAGGED(ch, EFF_ON_FIRE)) {
        act("$n's flames go out with a hiss as $e enters the water.", false, ch, 0, 0, TO_ROOM);
        act("Your flames quickly go out as you enter the water.", false, ch, 0, 0, TO_CHAR);
        REMOVE_FLAG(EFF_FLAGS(ch), EFF_ON_FIRE);
    }

    return EVENT_FINISHED;
}

void die_consentee_clean(CharData *ch) {
    /*aim is to search through character list and check if he been consented
       if so clean it up*/
    static CharData *i;

    for (i = character_list; i; i = i->next) {
        if (CONSENT(i) == ch) {
            char_printf(i, "&0&7&bThe person you consented to has left the game.&0\n");
            CONSENT(i) = nullptr;
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
std::optional<std::string> get_line(FILE *fl) {
    char temp[256];

    while (fgets(temp, sizeof(temp), fl)) {
        if (temp[0] != '*' && !std::string(temp).empty()) {
            temp[strcspn(temp, "\n")] = '\0'; // Remove newline character
            return std::string(temp);
        }
    }

    return std::nullopt; // Return empty optional if end of file is reached
}

int num_pc_in_room(RoomData *room) {
    int i = 0;
    CharData *ch;

    for (ch = room->people; ch != nullptr; ch = ch->next_in_room)
        if (!IS_NPC(ch))
            i++;

    return i;
}

CharData *is_playing(std::string_view vict_name) {
    DescriptorData *i;
    for (i = descriptor_list; i; i = i->next)
        if (i->connected == CON_PLAYING) {
            if (!matches(GET_NAME(i->character), vict_name))
                return i->character;
            else if (i->original && !matches(GET_NAME(i->original), vict_name))
                return i->original;
        }
    return nullptr;
}

int load_modifier(CharData *ch) {
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
    p = 100 - std::max<int>(0, ((ccw - IS_CARRYING_W(ch)) * 100) / ccw);
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

const std::string_view movewords(CharData *ch, int dir, int room, int leaving) {
    if (GET_POS(ch) == POS_FLYING)
        return (leaving ? "flies" : "flies in");

    if (EFF_FLAGGED(ch, EFF_FEATHER_FALL) && (dir == DOWN || dir == UP))
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

int pick_random_gem_drop(CharData *ch) {

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

    if (!IS_NPC(ch) || ch->desc)
        return 0;

    if (random_number(1, 100) > 7)
        return 0;

    if (GET_LEVEL(ch) > 60) {
        slot = random_number(13, 15);
        phase = 3;
    } else {
        slot = GET_LEVEL(ch) / 4;
        phase = GET_LEVEL(ch) / 20;
    }

    roll = random_number(1, 100);

    /* You win the shiny prize!  The shiny prize is rusted decayed armor. */
    if (roll > 74) {

        int item = 4 - random_number(1, 7) + GET_LEVEL(ch);

        if (item < 0)
            item = 0;
        if (item > 76)
            /* If item is over 76 (worn robe), then reset the base to the first helm armor and roll random helm, arm,
             * leg, or chest */
            item = 61 + random_number(1, 15);

        return armor_vnums[item];
    }

    roll = random_number(1, 100);

    if (roll < 20) {
        slot--;
    } else if (roll < 70) {
    } else if (roll < 85) {
        slot++;
    } else {
        /* If we're in the highest section of a phase, there's a chance of leg gems.
         */
        if (slot % 5 == 4) {
            return random_number(leg_gem_vnums[phase][0], leg_gem_vnums[phase][1]);
        }

        slot++;
        slot++;
    }

    if (slot < 0)
        slot = 0;
    if (slot > 14)
        slot = 14;

    return random_number(common_gem_vnums[slot][0], common_gem_vnums[slot][1]);
}

void perform_random_gem_drop(CharData *ch) {
    int vnum = pick_random_gem_drop(ch);
    int rnum;
    ObjData *od;

    if (!vnum)
        return;

    rnum = real_object(vnum);

    if (rnum < 0) {
        log(LogSeverity::Stat, LVL_IMMORT, "SYSERR: Can't perform random gem drop - no object with vnum {}", vnum);
        return;
    }

    od = read_object(rnum, REAL);

    if (!od) {
        log(LogSeverity::Warn, LVL_IMMORT, "RGD Error: Could not read object (vnum {})!", vnum);
        return;
    }

    obj_to_char(od, ch);
}

int yesno_result(std::string_view answer) {
    if (answer.empty())
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

int normalize_ip_address(const std::string_view in, std::string &out) {
    int octets[4] = {0};
    int octet = 0;
    int octval = 0;

    for (char ch : in) {
        if (ch == '.') {
            if (octval > 255) {
                // Invalid value
                return 1;
            }
            octets[octet++] = octval;
            octval = 0;
            if (octet >= 4) {
                // Too many octets
                return 1;
            }
        } else if (std::isdigit(ch)) {
            octval = octval * 10 + (ch - '0');
        } else {
            // Invalid character
            return 1;
        }
    }

    if (octval > 255 || octet != 3) {
        // Invalid value or incorrect number of octets
        return 1;
    }
    octets[octet] = octval;

    out = fmt::format("{}.{}.{}.{}", octets[0], octets[1], octets[2], octets[3]);
    return 0;
}

std::string_view statelength(int inches) {
    if (inches < 12)
        return fmt::format("{} inch{}", inches, inches == 1 ? "" : "es");
    else if (inches < 1200 && inches % 12)
        return fmt::format("{} {}, {} inch{}", inches / 12, inches / 12 == 1 ? "foot" : "feet", inches % 12,
                           inches % 12 == 1 ? "" : "es");
    else
        return fmt::format("{} {}", inches / 12, inches / 12 == 1 ? "foot" : "feet");
}

std::string_view stateweight(float pounds) {
    if (pounds < 2000)
        return fmt::format("{:.2f} pound{}", pounds, pounds == 1 ? "" : "s");
    else if (pounds < 2100)
        return "1 ton";
    else {
        auto res = fmt::format("{:.1f}", pounds / 2000.0);
        /* Lop off trailing ".0" */
        if (res.size() > 2 && res.back() == '0' && res[res.size() - 2] == '.')
            res.erase(res.size() - 2);
        res += " tons";
        return res;
    }
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
 * sizeof( sizedef));
 *
 *    ch          Optional.  A character who has typed something to be
 * identified and may need feedback. arg         The string to identify objname
 * What kind of thing we're identifying.  Only needed if ch is not NULL. numobjs
 * How many kinds of this thing exist? objects     The array of structs. objsize
 * The size of the structs.
 */

struct objdef {
    std::string_view name;
};

#define OBJNAME(i) (((objdef *)((objects) + objsize * i))->name)

int parse_obj_name(CharData *ch, const std::string_view arg, const std::string_view objname, int numobjs, void *objects,
                   int objsize) {
    int i, answer = -1, best = -1;

    if (arg.empty()) {
        if (ch) {
            char_printf(ch, "What {}?\n", objname);
        }
        return -1;
    }

    if (isdigit(arg[0])) {
        answer = svtoi(arg);
        if (answer < 0 || answer >= numobjs)
            answer = -1;
    } else
        for (i = 0; i < numobjs; i++) {
            if (!matches_start(arg, OBJNAME(i))) {
                if (!matches(arg, OBJNAME(i))) {
                    answer = i;
                    break;
                }
                if (best == -1)
                    best = i;
            } else if (matches_start(arg, OBJNAME(i))) {
                if (best == -1)
                    best = i;
            }
        }

    if (answer == -1)
        answer = best;
    if (answer == -1) {
        if (ch) {
            char_printf(ch, "There is no such {}.\n", objname);
        }
    }
    return answer;
}

std::string_view format_apply(int apply, int modifier) {
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
        swap = false;
        --count;
        for (i = 0; i < count; ++i)
            if (comparator(array[i], array[i + 1]) > 0) {
                t = array[i];
                array[i] = array[i + 1];
                array[i + 1] = t;
                swap = true;
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
                return /* false */;
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
    return /* true */;

#undef MAX_LEVELS
}

void drop_core(CharData *ch, const std::string_view desc) {
    static int corenum = 0;
    char initcorename[MAX_STRING_LENGTH];
    char corename[MAX_STRING_LENGTH];
    struct stat finfo;
    bool dropped = false;

    pid_t child = fork();

    if (child == 0) {
        abort();
    } else {
        if (desc.empty()) {
            sprintf(corename, "core-%d.%d", corenum, getpid());
        } else {
            sprintf(corename, "core-%s-%d.%d", desc, corenum, getpid());
        }
        corenum++;
        waitpid(child, 0, 0);
        sprintf(initcorename, "core.%d", child);
        if (stat(initcorename, &finfo) == 0) {
            rename(initcorename, corename);
            dropped = true;
        } else if (stat("core", &finfo) == 0) {
            rename("core", corename);
            dropped = true;
        } else {
            log("SYSERR: Could not find core file named {} or {}", initcorename, "core");
        }
        if (ch) {
            if (dropped) {
                char_printf(ch, "The core was dumped to {}\n", corename);
            } else {
                char_printf(ch, "Sorry, the core dump failed!\n");
            }
        }
    }
}
