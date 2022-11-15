/***************************************************************************
 *   File: clansys.c                                      Part of FieryMUD *
 *  Usage: Infrastructure for the clan system                              *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#include "clan.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "limits.hpp"
#include "math.hpp"
#include "players.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/*
 * Needed for mkdir in init_clans.  This is only guaranteed to work on
 * systems with the GNU C library.  But I don't forsee us switching to
 * Windows or something anytime soon.
 */
#include <sys/stat.h>

/***************************************************************************
 * Clan Variables
 ***************************************************************************/
static unsigned int num_of_clans = 0;
static Clan **clans = nullptr;

/***************************************************************************
 * Clan Infrastructure
 ***************************************************************************/

static void sort_clan_people(Clan *clan) {
    ClanMembership **array, *member;
    size_t i, count;
    bool swap;

    if (clan->people_count == 0)
        return;

    /* copy the linked list to an array */
    CREATE(array, ClanMembership *, clan->people_count);
    member = clan->people;
    for (i = 0; i < clan->people_count; ++i) {
        array[i] = member;
        member = member->next;
    }

    /* bubble sort by rank */
    count = clan->people_count;
    do {
        swap = false;
        --count;
        for (i = 0; i < count; ++i)
            if (OUTRANKS(array[i + 1]->rank, array[i]->rank)) {
                member = array[i];
                array[i] = array[i + 1];
                array[i + 1] = member;
                swap = true;
            }
    } while (swap);

    /* redo the linked list */
    clan->people = array[0];
    for (i = 1; i < clan->people_count; ++i)
        array[i - 1]->next = array[i];
    array[i - 1]->next = nullptr;

    free(array);
}

static void load_clan_member(Clan *clan, const char *line) {
    ClanMembership *member, *alt;
    int num;
    char name[MAX_NAME_LENGTH + 1];

    line = fetch_word(line, name, sizeof(name));
    CAP(name);
    if ((num = get_ptable_by_name(name)) < 0)
        return;

    CREATE(member, ClanMembership, 1);
    member->name = strdup(name);
    member->clan = clan;
    member->relation.alts = nullptr;
    member->player = nullptr;

    line = fetch_word(line, name, sizeof(name));
    member->rank = MAX(1, atoi(name));

    line = fetch_word(line, name, sizeof(name));
    member->since = atoi(name);

    if (IS_MEMBER_RANK(member->rank)) {
        clan->power += player_table[num].level;
        clan->member_count++;
    } else if (IS_ADMIN_RANK(member->rank))
        clan->admin_count++;
    else if (IS_APPLICANT_RANK(member->rank))
        clan->applicant_count++;
    else if (IS_REJECT_RANK(member->rank))
        clan->reject_count++;

    for (;;) {
        line = fetch_word(line, name, sizeof(name));
        if (!*name)
            break;
        CAP(name);
        if (get_ptable_by_name(name) < 0)
            continue;
        CREATE(alt, ClanMembership, 1);
        alt->name = strdup(name);
        alt->rank = ALT_RANK_OFFSET + member->rank;
        alt->clan = clan;
        alt->relation.member = member;
        alt->since = member->since;
        alt->next = member->relation.alts;
        member->relation.alts = alt;
    }

    member->next = clan->people;
    clan->people = member;
    clan->people_count++;
}

static void refresh_list_pointers(Clan *clan) {
    ClanMembership *member;

#define CHANGE_IF_NULL(a, b) ((a) = ((a) ? (a) : (b)))

    clan->admins = nullptr;
    clan->members = nullptr;
    clan->applicants = nullptr;
    clan->rejects = nullptr;

    for (member = clan->people; member; member = member->next)
        if (IS_ADMIN_RANK(member->rank))
            CHANGE_IF_NULL(clan->admins, member);
        else if (IS_MEMBER_RANK(member->rank))
            CHANGE_IF_NULL(clan->members, member);
        else if (IS_APPLICANT_RANK(member->rank))
            CHANGE_IF_NULL(clan->applicants, member);
        else if (IS_REJECT_RANK(member->rank))
            CHANGE_IF_NULL(clan->rejects, member);

#undef CHANGE_IF_NULL
}

void update_clan(Clan *clan) {
    sort_clan_people(clan);
    refresh_list_pointers(clan);
}

bool load_clan(const char *clan_num, Clan *clan) {
    FILE *fl;
    char filename[128], tag[128], *line = buf;
    const char *string;
    int num, i;

    /* Open clan file for reading */
    snprintf(filename, sizeof(filename), "%s/%s%s", CLAN_PREFIX, clan_num, CLAN_SUFFIX);
    if (!(fl = fopen(filename, "r"))) {
        log("Couldn't open clan file '%s'", filename);
        return false;
    }

    /* Initialize fields */
    clan->people_count = 0;
    clan->member_count = 0;
    clan->admin_count = 0;
    clan->applicant_count = 0;
    clan->reject_count = 0;
    clan->rank_count = 0;
    CREATE(clan->ranks, ClanRank, MAX_CLAN_RANKS);
    clan->people = nullptr;
    clan->power = 0;

    /* Tag-based ASCII file parser */
    while (get_line(fl, line)) {
        tag_argument(line, tag);
        num = atoi(line);

        switch (toupper(*tag)) {
        case 'A':
            if (TAG_IS("appfee"))
                clan->app_fee = num;
            else if (TAG_IS("applevel"))
                clan->app_level = num;
            else if (TAG_IS("abbr")) {
                char *space = strchr(line, ' ');
                if (space)
                    *space = '\0';
                clan->abbreviation = strdup(line);
            } else
                goto bad_tag;
            break;
        case 'C':
            if (TAG_IS("copper"))
                clan->treasure[COPPER] = num;
            else
                goto bad_tag;
            break;
        case 'D':
            if (TAG_IS("dues"))
                clan->dues = num;
            else if (TAG_IS("description"))
                clan->description = fread_string(fl, "load_clan");
            else
                goto bad_tag;
            break;
        case 'G':
            if (TAG_IS("gold"))
                clan->treasure[GOLD] = num;
            else
                goto bad_tag;
            break;
        case 'M':
            if (TAG_IS("member"))
                load_clan_member(clan, line);
            else if (TAG_IS("motd"))
                clan->motd = fread_string(fl, "load_clan");
            else
                goto bad_tag;
            break;
        case 'N':
            if (TAG_IS("name"))
                clan->name = strdup(line);
            else if (TAG_IS("number"))
                clan->number = num;
            else
                goto bad_tag;
            break;
        case 'P':
            if (TAG_IS("platinum"))
                clan->treasure[PLATINUM] = num;
            else if (TAG_IS("privilege")) {
                if (num <= 0 || num > MAX_CLAN_RANKS)
                    log("SYSERR: load_clan: attempt to set clan privilege for "
                        "invalid rank %d",
                        num);
                else {
                    string = skip_chars(skip_over(line, S_DIGITS), ' ');
                    while (*string) {
                        string = fetch_word(string, buf1, sizeof(buf1));
                        for (i = 0; i < NUM_CLAN_PRIVS; ++i)
                            if (!strcmp(clan_privileges[i].abbr, buf1)) {
                                clan->rank_count = MAX(clan->rank_count, num);
                                SET_FLAG(clan->ranks[num - 1].privileges, i);
                                break;
                            }
                        if (i >= NUM_CLAN_PRIVS)
                            log("SYSERR: load_clan: attempt to assign invalid clan "
                                "privilege %s to rank %d",
                                buf1, num);
                    }
                }
            } else
                goto bad_tag;
            break;
        case 'S':
            if (TAG_IS("silver"))
                clan->treasure[SILVER] = num;
            else
                goto bad_tag;
            break;
        case 'T':
            if (TAG_IS("title")) {
                if (num <= 0 || num > MAX_CLAN_RANKS)
                    log("SYSERR: load_clan: attempt to set clan title for "
                        "invalid rank %d",
                        num);
                else {
                    string = skip_chars(skip_over(line, S_DIGITS), ' ');
                    if (clan->ranks[num - 1].title)
                        log("SYSERR: load_clan: attempt to load duplicate clan "
                            "title for rank %d",
                            num);
                    else {
                        clan->rank_count = MAX(clan->rank_count, num);
                        clan->ranks[num - 1].title = strdup(string);
                    }
                }
            } else
                goto bad_tag;
            break;
        default:
        bad_tag:
            log("SYSERR: Unknown tag %s in clan file %s: %s", tag, clan_num, line);
            break;
        }
    }

    RECREATE(clan->ranks, ClanRank, clan->rank_count);

    update_clan(clan);

    return true;
}

void init_clans(void) {
    FILE *fl;
    char clan_id[17];
    struct load_list {
        Clan *clan;
        load_list *next;
    } *temp, *load = nullptr;
    unsigned int pos;

    /*
     * Legacy-checking code.  If we successfully open the file, that means
     * it is not a directory, and is probably the binary format from the
     * old clan system.  In this case, back it up with a .old extension.
     */
    if ((fl = fopen(CLAN_PREFIX, "r"))) {
        struct stat statbuf;
        fstat(fileno(fl), &statbuf);
        fclose(fl);
        if (!S_ISDIR(statbuf.st_mode)) {
            rename(CLAN_PREFIX, CLAN_PREFIX_OLD);
            /*
             * This is a POSIX-only.  Create the new clan directory.
             */
            log("Backed-up old clans file and creating new clan directory.");
            mkdir(CLAN_PREFIX, 0775);
        }
    }

    if (!(fl = fopen(CLAN_INDEX_FILE, "r"))) {
        log("No clan index.  Creating a new one.");
        touch(CLAN_INDEX_FILE);
        if (!(fl = fopen(CLAN_INDEX_FILE, "r"))) {
            perror("fatal error opening clan index");
            exit(1);
        }
    }

    num_of_clans = 0;
    while (fgets(clan_id, sizeof(clan_id), fl)) {
        clan_id[strlen(clan_id) - 1] = '\0'; /* remove the \n */
        CREATE(temp, load_list, 1);
        CREATE(temp->clan, Clan, 1);
        if (load_clan(clan_id, temp->clan)) {
            ++num_of_clans;
            temp->next = load;
            load = temp;
        } else
            free(temp);
    }

    CREATE(clans, Clan *, num_of_clans);
    for (pos = num_of_clans; pos > 0; --pos) {
        clans[pos - 1] = load->clan;
        temp = load->next;
        free(load);
        load = temp;
    }

    log("%3d clan%s loaded.", num_of_clans, num_of_clans == 1 ? "" : "s");

    fclose(fl);
}

static void save_clan_index(void) {
    FILE *fl;
    unsigned int i;

    if (!(fl = fopen(CLAN_INDEX_FILE, "w"))) {
        log("No clan index.  Creating a new one.");
        touch(CLAN_INDEX_FILE);
        if (!(fl = fopen(CLAN_INDEX_FILE, "w"))) {
            perror("fatal error opening clan index");
            exit(1);
        }
    }

    for (i = 0; i < num_of_clans; ++i)
        fprintf(fl, "%u\n", clans[i]->number);

    fclose(fl);
}

static void perform_save_clan(const Clan *clan) {
    FILE *fl;
    char temp_filename[128];
    char filename[128];
    ClanMembership *member, *alt;
    unsigned int i, j;

    sprintf(filename, "%s/%d%s", CLAN_PREFIX, clan->number, CLAN_SUFFIX);
    sprintf(temp_filename, "%s/%d.tmp", CLAN_PREFIX, clan->number);
    if (!(fl = fopen(temp_filename, "w"))) {
        mprintf(L_ERROR, LVL_GOD, "SYSERR: Couldn't open temp clan file %s for write", temp_filename);
        return;
    }

    fprintf(fl, "number: %d\n", clan->number);
    fprintf(fl, "name: %s\n", clan->name);
    fprintf(fl, "abbr: %s\n", clan->abbreviation);
    if (clan->description)
        fprintf(fl, "description:\n%s~\n", filter_chars(buf, clan->description, "\r~"));
    if (clan->motd)
        fprintf(fl, "motd:\n%s~\n", filter_chars(buf, clan->motd, "\r~"));
    fprintf(fl, "dues: %u\n", clan->dues);
    fprintf(fl, "appfee: %u\n", clan->app_fee);
    fprintf(fl, "applevel: %u\n", clan->app_level);

    fprintf(fl, "copper: %d\n", clan->treasure[COPPER]);
    fprintf(fl, "silver: %d\n", clan->treasure[SILVER]);
    fprintf(fl, "gold: %d\n", clan->treasure[GOLD]);
    fprintf(fl, "platinum: %d\n", clan->treasure[PLATINUM]);

    for (i = 0; i < clan->rank_count; ++i) {
        fprintf(fl, "title: %u %s\n", i + 1, clan->ranks[i].title);
        fprintf(fl, "privilege: %u", i + 1);
        for (j = 0; j < NUM_CLAN_PRIVS; ++j)
            if (IS_FLAGGED(clan->ranks[i].privileges, j))
                fprintf(fl, " %s", clan_privileges[j].abbr);
        fprintf(fl, "\n");
    }

    for (member = clan->people; member; member = member->next) {
        fprintf(fl, "member: %s %u %lu", member->name, member->rank, member->since);
        for (alt = member->relation.alts; alt; alt = alt->next)
            fprintf(fl, " %s", alt->name);
        fprintf(fl, "\n");
    }

    fclose(fl);

    if (rename(temp_filename, filename))
        log("SYSERR: Error renaming temp clan file %s to %s: %s", temp_filename, filename, strerror(errno));
}

void save_clan(const Clan *clan) {
    perform_save_clan(clan);
    save_clan_index();
}

void save_clans(void) {
    unsigned int i;
    for (i = 0; i < num_of_clans; ++i)
        perform_save_clan(clans[i]);
    save_clan_index();
}

Clan *find_clan_by_number(unsigned int number) {
    clan_iter iter;

    for (iter = clans_start(); iter != clans_end(); ++iter)
        if ((*iter)->number == number)
            return *iter;

    return nullptr;
}

Clan *find_clan_by_abbr(const char *abbr) {
    clan_iter iter;

    for (iter = clans_start(); iter != clans_end(); ++iter)
        if (isname(abbr, (*iter)->abbreviation))
            return *iter;

    return nullptr;
}

Clan *find_clan(const char *id) {
    Clan *clan;
    if (is_number(id))
        if ((clan = find_clan_by_number(atoi(id))))
            return clan;
    if ((clan = find_clan_by_abbr(id)))
        return clan;
    {
        clan_iter iter;
        for (iter = clans_start(); iter != clans_end(); ++iter)
            if (isname(id, (*iter)->name))
                return *iter;
    }
    return nullptr;
}

ClanMembership *find_clan_membership_in_clan(const char *name, const Clan *clan) {
    ClanMembership *member = clan->people;

    while (member) {
        if (!strcmp(name, member->name))
            return member;

        if (IS_ALT_RANK(member->rank)) {
            /* We're currently on an alt.  If there is another alt for
             * this member, go to that one.  Otherwise, go to the next
             * member in the clan.
             */
            if (member->next)
                member = member->next;
            else
                member = member->relation.member->next;
        } else {
            /* We're currently on a member.  If this member has alts,
             * go to the first alt in the list.  Otherwise go to the
             * next member in the clan.
             */
            if (member->relation.alts)
                member = member->relation.alts;
            else
                member = member->next;
        }
    }

    return nullptr;
}

ClanMembership *find_clan_membership(const char *name) {
    clan_iter iter;
    ClanMembership *member;

    for (iter = clans_start(); iter != clans_end(); ++iter)
        if ((member = find_clan_membership_in_clan(name, *iter)))
            return member;

    return nullptr;
}

void free_clan_membership(ClanMembership *member) {
    free(member->name);
    if (!IS_ALT_RANK(member->rank)) {
        ClanMembership *alt;
        for (alt = member->relation.alts; alt; alt = alt->next)
            free_clan_membership(alt);
    }
    if (member->player)
        member->player->player_specials->clan = nullptr;
    free(member);
}

/* Doesn't free the clan itself! */
void free_clan(Clan *clan) {
    ClanMembership *member;
    free(clan->name);
    free(clan->abbreviation);
    free(clan->description);
    free(clan->motd);
    free(clan->ranks);
    while (clan->people) {
        member = clan->people->next;
        free_clan_membership(clan->people);
        clan->people = member;
    }
    free(clan);
}

void free_clans(void) {
    unsigned int i;
    for (i = 0; i < num_of_clans; ++i)
        free_clan(clans[i]);
    free(clans);
}

bool revoke_clan_membership(ClanMembership *member) {
    ClanMembership *p;
    Clan *clan;
    int i;

    if (!member)
        return false;

    clan = member->clan;

    /* Remove from clan lists */
    if (IS_ALT_RANK(member->rank)) {
        p = member->relation.member;
        if (p->relation.alts == member)
            p->relation.alts = member->next;
        else
            for (p = p->relation.alts; p->next; p = p->next)
                if (p->next == member) {
                    p->next = member->next;
                    break;
                }
    } else if (clan) {
        if (clan->people == member)
            clan->people = member->next;
        else
            for (p = clan->people; p->next; p = p->next)
                if (p->next == member) {
                    p->next = member->next;
                    break;
                }
        if (IS_MEMBER_RANK(member->rank)) {
            clan->member_count--;
            if (member->player)
                clan->power -= GET_LEVEL(member->player);
            else if ((i = get_ptable_by_name(member->name)) >= 0)
                clan->power -= player_table[i].level;
        } else if (IS_ADMIN_RANK(member->rank))
            clan->admin_count--;
        else if (IS_APPLICANT_RANK(member->rank))
            clan->applicant_count--;
        else if (IS_REJECT_RANK(member->rank))
            clan->reject_count--;
    }

    free_clan_membership(member);

    if (clan)
        save_clan(clan);

    return true;
}

void add_clan_membership(Clan *clan, ClanMembership *member) {
    ClanMembership *m;

    if (!clan->people || OUTRANKS(member->rank, clan->people->rank)) {
        member->next = clan->people;
        clan->people = member;
    } else {
        member->next = nullptr;
        for (m = clan->people; m->next; m = m->next)
            if (OUTRANKS(member->rank, m->next->rank)) {
                member->next = m->next;
                break;
            }
        m->next = member;
    }

    if (IS_ADMIN_RANK(member->rank))
        clan->admin_count++;
    else if (IS_MEMBER_RANK(member->rank))
        clan->member_count++;
    else if (IS_APPLICANT_RANK(member->rank))
        clan->applicant_count++;
    else if (IS_REJECT_RANK(member->rank))
        clan->reject_count++;

    clan->people_count++;

    if (!IS_ALT_RANK(member->rank))
        refresh_list_pointers(clan);

    member->clan = clan;
}

void clan_notification(Clan *clan, CharData *skip, const char *messg, ...) {
    DescriptorData *d;
    size_t found = false;
    va_list args;
    char comm_buf[MAX_STRING_LENGTH];

    if (clan && messg && *messg)
        for (d = descriptor_list; d; d = d->next)
            if (IS_PLAYING(d) && d->character && (!skip || (d->character != skip && d->original != skip)) &&
                GET_CLAN(REAL_CHAR(d->character)) == clan && !PRF_FLAGGED(d->character, PRF_NOCLANCOMM) &&
                !OUTRANKS(RANK_APPLICANT, GET_CLAN_RANK(REAL_CHAR(d->character)))) {
                if (!found) {
                    strcpy(comm_buf, AFMAG "[[ ");
                    found = strlen(comm_buf);
                    va_start(args, messg);
                    vsnprintf(comm_buf + found, sizeof(comm_buf) - found, messg, args);
                    va_end(args);
                    strcat(comm_buf, AFMAG " ]]\n" ANRM);
                    found = true;
                }
                string_to_output(d, comm_buf);
            }
}

clan_iter clans_start(void) { return clans; }

clan_iter clans_end(void) { return clans + num_of_clans; }

unsigned int clan_count(void) { return num_of_clans; }

void clan_set_title(CharData *ch) {
    if (IS_CLAN_MEMBER(ch)) {
        sprintf(buf, "%s %s", GET_CLAN_TITLE(ch), GET_CLAN(ch)->abbreviation);
        set_title(ch, buf);
    } else
        set_title(ch, nullptr);
}

Clan *alloc_clan() {
    clan_iter iter;
    unsigned int number = 0;

    for (iter = clans_start(); iter != clans_end(); ++iter)
        number = MAX(number, (*iter)->number);

    ++num_of_clans;
    RECREATE(clans, Clan *, num_of_clans);
    CREATE(clans[num_of_clans - 1], Clan, 1);
    clans[num_of_clans - 1]->number = number + 1;
    save_clan_index();
    return clans[num_of_clans - 1];
}

void dealloc_clan(Clan *clan) {
    clan_iter dest, src;

    for (dest = src = clans_start(); src != clans_end(); ++src)
        if (*src != clan)
            *(dest++) = *src;
    free_clan(clan);
    --num_of_clans;
    save_clan_index();
}

unsigned int days_until_reapply(const ClanMembership *member) {
    if (IS_REJECT_RANK(member->rank)) {
        double diff = difftime(time(0), member->since);
        if (diff > 0)
            return diff;
    }

    return 0;
}

PRIV_FUNC(clan_admin_check) {
    if (PRV_FLAGGED(ch, PRV_CLAN_ADMIN) && GET_CLAN_MEMBERSHIP(ch))
        revoke_clan_membership(GET_CLAN_MEMBERSHIP(ch));
}
