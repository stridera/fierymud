#include "act.hpp"

#include "arguments.hpp"
#include "clan.hpp"
#include "comm.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "find.hpp"
#include "handler.hpp"
#include "logging.hpp"
#include "messages.hpp"
#include "modify.hpp"
#include "money.hpp"
#include "objects.hpp"
#include "players.hpp"
#include "screen.hpp"
#include "utils.hpp"

#include <expected>
#include <fmt/format.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#define CLANCMD(name) static void(name)(CharData * ch, Arguments argument)

// Get the clan from the character. Gods always need to specify the clan.
static std::shared_ptr<ClanMembership> find_memberships(CharData *ch, Arguments &argument) {
    auto memberships = ch->player_specials->clan_memberships;
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (memberships.empty()) {
            char_printf(ch, "You are not a member of a clan.\n");
            return nullptr;
        }

        if (memberships.size() == 1) {
            return memberships[0];
        }
    }

    if (argument.empty()) {
        if (!memberships.empty()) {
            return memberships[0];
        }
        char_printf(ch, "Please specify a clan.\n");
        return nullptr;
    }

    // Gods can specify the clan. We'll create a temporary membership for them.
    auto clan = [&argument]() -> std::optional<ClanPtr> {
        auto clan_id_opt = argument.try_shift_number();
        if (clan_id_opt) {
            return clan_repository.find_by_id(*clan_id_opt);
        }
        auto clan_name = argument.shift();
        return clan_repository.find_by_abbreviation(clan_name);
    }();

    if (!clan) {
        char_printf(ch, "No such clan.\n");
        return nullptr;
    }

    auto me = std::shared_ptr<CharData>(ch, [](CharData *) { /* Non-owning */ });
    if (auto membership = (*clan)->get_membership(me)) {
        return *membership;
    }

    // Create a temporary membership for gods with all permissions
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        PermissionSet perms;
        perms.set(static_cast<size_t>(ClanPrivilege::Admin));
        return std::make_shared<ClanMembership>(*clan, me, ClanRank("God", perms));
    }

    char_printf(ch, "You are not a member of that clan.\n");
    return nullptr;
}

CLANCMD(clan_list) {
    if (clan_repository.count() == 0) {
        char_printf(ch, "No clans have formed yet.\n");
        return;
    }

    /* List clans, # of members, power, and app fee */
    paging_printf(ch, AUND " Num   Clan           Members  App Fee/Lvl\n" ANRM);
    for (const auto &clan : clan_repository.all()) {
        paging_printf(ch, fmt::format("[{:3}] {:<{}} {:3}   {:5}p/{:3}\n", clan->id(), clan->abbreviation(),
                                      18 + count_color_chars(clan->abbreviation()) * 2, clan->member_count(),
                                      clan->app_fee(), clan->min_application_level()));
    }

    start_paging(ch);
}

CLANCMD(clan_deposit) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    auto arg = argument.shift();

    auto money_opt = parse_money(argument.get());
    if (!money_opt || money_opt->value() <= 0) {
        char_printf(ch, "How much do you want to deposit?\n");
        return;
    }

    Money coins = *money_opt;
    /* Gods have bottomless pockets */
    if (GET_LEVEL(ch) < LVL_GOD) {
        Money owned = ch->points.money;
        if (!owned.charge(coins)) {
            char_printf(ch, "You do not have that kind of money!\n");
            return;
        }
        for (int i = 0; i < NUM_COIN_TYPES; i++) {
            ch->points.money[i] -= coins[i];
        }
    }

    if (auto result = membership->add_clan_treasure(coins); !result) {
        char_printf(ch, "{}.\n", result.error());
        return;
    }

    save_player_char(ch);

    char_printf(ch, "You {}'s account: {}\n", membership->get_clan_abbreviation(), statemoney(coins));
    clan_repository.save();
}

CLANCMD(clan_withdraw) {

    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    auto arg = argument.shift();

    auto money_opt = parse_money(argument.get());
    if (!money_opt || money_opt->value() <= 0) {
        char_printf(ch, "How much do you want to withdraw?\n");
        return;
    }

    Money coins = *money_opt;

    auto treasure = membership->get_clan_treasure();
    if (treasure.value() < coins.value()) {
        char_printf(ch, "The clan is not wealthy enough for your needs!\n");
        return;
    }

    if (auto result = membership->subtract_clan_treasure(coins); !result) {
        char_printf(ch, "{}.\n", result.error());
        return;
    }

    for (int i = 0; i < NUM_COIN_TYPES; i++) {
        ch->points.money[i] -= coins[i];
    }
    save_player_char(ch);

    char_printf(ch, "You withdraw from  {}'s account: {}\n", membership->get_clan_abbreviation(), statemoney(coins));
    clan_repository.save();
}

CLANCMD(clan_store) {
    ObjData *obj;

    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    auto arg_pair = argument.try_shift_number_and_arg();

    if (!arg_pair || arg_pair->second.empty()) {
        char_printf(ch, "What do you want to store?\n");
        return;
    }

    if (arg_pair->first && arg_pair->first != 1) {
        char_printf(ch, "You can only store one item at a time.\n");
        return;
    }

    auto obj_name = arg_pair->second;
    if (!(obj =
              find_obj_in_list(ch->carrying, find_vis_by_name(ch, const_cast<char *>(std::string(obj_name).data()))))) {
        char_printf(ch, "You aren't carrying {} {}.\n", an(obj_name), obj_name);
    } else if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
        act("You can't store $p in $P because it's CURSED!", false, ch, obj, nullptr, TO_CHAR);
    } else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains) {
        char_printf(ch, "You can't store a container with items in it.\n");
    } else if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
        char_printf(ch, "You can't store money in the clan vault.\n");
    } else if (GET_OBJ_VNUM(obj) == -1) {
        char_printf(ch, "That item is much too unique to store.\n");
    } else {
        obj_from_char(obj);
        auto vnum = GET_OBJ_VNUM(obj);
        auto name = obj->short_description;
        free_obj(obj);

        char_printf(ch, "You store {} in the clan vault.\n", name);
        if (auto result = membership->add_clan_storage_item(vnum, 1); !result) {
            char_printf(ch, "{}.\n", result.error());
            return;
        }

        membership->notify_clan("{} stores {} in the clan vault.", GET_NAME(ch), name);
    }
}

CLANCMD(clan_retrieve) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    auto arg_pair = argument.try_shift_number_and_arg();

    if (!arg_pair || arg_pair->second.empty()) {
        char_printf(ch, "What do you want to retrieve?\n");
        return;
    }

    if (arg_pair->first && arg_pair->first != 1) {
        char_printf(ch, "You can only retrieve one item at a time.\n");
        return;
    }

    auto obj_name = arg_pair->second;
    // We need to find the vnum in the storage that matches this name
    ObjectId obj_vnum = 0;
    bool found = false;

    // Find the object in storage matching the name
    for (const auto &[vnum, count] : membership->get_clan_storage()) {
        if (count <= 0)
            continue;

        auto obj = read_object(vnum, VIRTUAL);
        if (!obj)
            continue;

        if (isname(std::string(obj_name).c_str(), obj->name)) {
            obj_vnum = vnum;
            found = true;
            free_obj(obj);
            break;
        }
        free_obj(obj);
    }

    if (!found) {
        char_printf(ch, "There is no {} {} in the clan vault.\n", an(obj_name), obj_name);
        return;
    }

    auto obj = read_object(obj_vnum, VIRTUAL);
    if (!obj) {
        char_printf(ch, "Error retrieving object.\n");
        return;
    }

    obj_to_char(obj, ch);

    char_printf(ch, "You retrieve {} from the clan vault.\n", obj->short_description);
    if (auto result = membership->remove_clan_storage_item(obj_vnum, 1); !result) {
        char_printf(ch, "{}.\n", result.error());
        return;
    }

    membership->notify_clan("{} retrieves {} from the clan vault.", GET_NAME(ch), obj->short_description);
}

CLANCMD(clan_tell) {
    CharData *me = REAL_CHAR(ch);

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        char_printf(ch, "Your lips move, but no sound forms.\n");
        return;
    }

    if (!speech_ok(ch, 0))
        return;

    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    if (argument.empty()) {
        char_printf(ch, "What do you want to tell the clan?\n");
        return;
    }

    auto speech = drunken_speech(std::string(argument.get()), GET_COND(ch, DRUNK));

    char_printf(ch, AFMAG "You tell {}" AFMAG ", '" AHMAG "{}" AFMAG "'\n" ANRM,
                ch->player.level < LVL_IMMORT ? "your clan" : membership->get_clan_abbreviation(), speech);

    for (auto member : membership->get_clan_members()) {
        auto target = member->character();
        if (!target || !target->desc || !IS_PLAYING(target->desc))
            continue;

        auto tch = REAL_CHAR(target.get());
        if (!tch || tch == me)
            continue;

        if (STATE(target->desc) != CON_PLAYING || PLR_FLAGGED(tch, PLR_WRITING) || PLR_FLAGGED(tch, PLR_MAILING) ||
            EDITING(target->desc)) {
            if (!PRF_FLAGGED(tch, PRF_OLCCOMM))
                continue;
        }

        if (PRF_FLAGGED(tch, PRF_NOCLANCOMM))
            continue;

        char_printf(tch, AFMAG "{} tells {}" AFMAG ", '" AHMAG "{}" AFMAG "'\n" ANRM,
                    GET_INVIS_LEV(me) > GET_LEVEL(tch) ? "Someone" : GET_NAME(me),
                    tch->player.level < LVL_IMMORT ? "your clan" : membership->get_clan_abbreviation(), speech);
    }
}

CLANCMD(clan_set) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    auto cmd = argument.shift();

    if (matches_start(cmd, "abbr")) {
        auto new_abbreviation = argument.get();
        if (new_abbreviation.empty()) {
            char_printf(ch, "What do you want to set the clan abbreviation to?\n");
            return;
        }

        if (ansi_strlen(new_abbreviation) > Clan::MAX_CLAN_ABBR_LEN) {
            char_printf(ch, "Clan abbreviations may be at most {} characters in length.\n", Clan::MAX_CLAN_ABBR_LEN);
            return;
        }

        auto result = membership->update_clan_abbreviation(std::string(new_abbreviation));
        if (result) {
            char_printf(ch, "Clan abbreviation updated to {}.\n", new_abbreviation);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "addrank")) {
        if (argument.empty()) {
            char_printf(ch, "What do you want to name the new rank?\n");
            return;
        }

        auto title = argument.get();
        // Create a default rank with empty privileges
        PermissionSet privileges;
        ClanRank rank(std::string(title), privileges);

        auto result = membership->update_clan_rank(rank);
        if (result) {
            char_printf(ch, "New rank '{}' added.\n", title);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "appfee")) {
        auto fee_opt = argument.try_shift_number();
        if (!fee_opt) {
            char_printf(ch, "How much platinum should the clan's application fee be?\n");
            return;
        }

        auto result = membership->update_clan_app_fee(*fee_opt);
        if (result) {
            char_printf(ch, "{}'s application fee is now {} platinum.\n", membership->get_clan_name(), *fee_opt);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "applev")) {
        auto level_opt = argument.try_shift_number();
        if (!level_opt) {
            char_printf(ch, "What should the clan's minimum application level be?\n");
            return;
        }

        if (*level_opt < 1 || *level_opt > LVL_IMPL) {
            char_printf(ch, "The minimum application level must be between 1 and {}.\n", LVL_IMPL);
            return;
        }

        auto result = membership->update_clan_min_application_level(*level_opt);
        if (result) {
            char_printf(ch, "{}'s minimum application level is now {}.\n", membership->get_clan_name(), *level_opt);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "delrank")) {
        auto rank_opt = argument.try_shift_number();
        if (!rank_opt) {
            char_printf(ch, "Which rank do you want to delete?\n");
            return;
        }

        // Need to implement rank deletion in the membership class
        char_printf(ch, "Not implemented yet.\n");
    } else if (matches_start(cmd, "dues")) {
        auto dues_opt = argument.try_shift_number();
        if (!dues_opt) {
            char_printf(ch, "How much platinum should the clan's dues be?\n");
            return;
        }

        auto result = membership->update_clan_dues(*dues_opt);
        if (result) {
            char_printf(ch, "{}'s monthly dues are now {} platinum.\n", membership->get_clan_name(), *dues_opt);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "name")) {
        auto new_name = argument.get();
        if (new_name.empty()) {
            char_printf(ch, "What do you want to name the clan?\n");
            return;
        }

        if (ansi_strlen(new_name) > Clan::MAX_CLAN_NAME_LEN) {
            char_printf(ch, "Clan names may be at most {} characters in length.\n", Clan::MAX_CLAN_NAME_LEN);
            return;
        }

        auto result = membership->update_clan_name(std::string{new_name});
        if (result) {
            char_printf(ch, "Clan name updated to {}.\n", new_name);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "title")) {
        auto rank_opt = argument.try_shift_number();
        if (!rank_opt) {
            char_printf(ch, "For which rank do you want to set a title?\n");
            return;
        }

        auto title = argument.get();
        if (title.empty()) {
            char_printf(ch, "What title do you want to set?\n");
            return;
        }

        if (ansi_strlen(title) > Clan::MAX_CLAN_TITLE_LEN) {
            char_printf(ch, "Clan titles may be at most {} characters long.\n", Clan::MAX_CLAN_TITLE_LEN);
            return;
        }

        // Need to get the rank and update its title - need clan method for this
        char_printf(ch, "Not implemented yet.\n");
    } else {
        log(LogSeverity::Error, LVL_GOD, "SYSERR: clan_set: unknown subcommand '{}'", cmd);
        return;
    }

    clan_repository.save();
}

CLANCMD(clan_alt) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    auto target_name = argument.shift();
    if (target_name.empty()) {
        char_printf(ch, "Whom do you want to add or remove as an alt?\n");
        return;
    }

    // First check if we're removing an alt
    auto alts = membership->get_alts(membership->main_character_name());
    for (const auto &alt : alts) {
        if (matches(alt, target_name)) {
            // Need to implement alt removal
            membership->add_alt(alt);
            char_printf(ch, "You remove {} as one of your clan alts.\n", alt);
            // TODO: implement proper alt removal
            return;
        }
    }

    // Adding a new alt
    auto target = find_char_by_desc(find_vis_by_name(ch, const_cast<char *>(std::string(target_name).data())));
    if (!target) {
        char_printf(ch, "There's no one online by the name of {}.\n", target_name);
        return;
    }

    if (ch == target) {
        char_printf(ch, "You want to be your own alt?\n");
        return;
    }

    auto target_membership = get_clan_memberships(target);
    if (!target_membership.empty()) {
        char_printf(ch, "{} is already in a clan!\n", GET_NAME(target));
        return;
    }

    // Check for permission if not admin
    if (GET_LEVEL(ch) < LVL_IMMORT && !membership->has_permission(ClanPrivilege::Admin) &&
        strcasecmp(ch->desc->host, target->desc->host) != 0) {
        char_printf(ch, "{} was not found logged in as your alt.\n", GET_NAME(target));
        return;
    }

    // TODO: Implement alt adding properly
    char_printf(ch, "You make {} one of your clan alts.\n", GET_NAME(target));
    char_printf(target, AFMAG "{} makes you one of their clan alts.\n" ANRM, GET_NAME(ch));

    // Need to implement alt registration
    char_printf(ch, "Alt functionality not fully implemented yet.\n");
}

CLANCMD(clan_apply) {
    // TODO: Implement apply
    char_printf(ch, "Not implemented yet.\n");
}

CLANCMD(clan_create) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You don't have permission to create clans.\n");
        return;
    }

    auto abbreviation = argument.shift();
    if (abbreviation.empty()) {
        char_printf(ch, "What is the abbreviation for the new clan?\n");
        return;
    }

    if (ansi_strlen(abbreviation) > Clan::MAX_CLAN_ABBR_LEN) {
        char_printf(ch, "Clan abbreviations can be at most {} visible characters long.\n", Clan::MAX_CLAN_ABBR_LEN);
        return;
    }

    if (clan_repository.find_by_abbreviation(abbreviation)) {
        char_printf(ch, "A clan with a similar abbreviation already exists.\n");
        return;
    }

    // Generate a new ID - this is simplistic, should be improved
    ClanId new_id = clan_repository.count() + 1;

    auto clan = clan_repository.create(new_id, std::string{abbreviation}, std::string{abbreviation});
    if (!clan) {
        char_printf(ch, "Error creating clan.\n");
        return;
    }

    // Add leader rank
    // TODO: Add proper rank initialization

    char_printf(ch, "New clan {} created.\n", abbreviation);
    log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} creates new clan: {}", GET_NAME(ch), abbreviation);

    clan_repository.save();
}

CLANCMD(clan_destroy) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    auto clan_name = membership->get_clan_name();
    auto clan_id = membership->get_clan_id();

    // TODO: This should notify all members that the clan is disbanded
    membership->notify_clan("Your clan has been disbanded!");

    char_printf(ch, AFMAG "You have deleted the clan {}.\n" ANRM, clan_name);
    log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} has destroyed the clan {}.", GET_NAME(ch), clan_name);

    clan_repository.remove(clan_id);
    clan_repository.save();
}

CLANCMD(clan_info) {
    auto clan_name = argument.get();
    ClanPtr clan;

    if (clan_name.empty()) {
        auto membership = find_memberships(ch, argument);
        if (!membership) {
            char_printf(ch, "Which clan's info do you want to view?\n");
            return;
        }
        clan = membership->clan();
    } else {
        auto found_clan = clan_repository.find_by_name(clan_name);
        if (!found_clan) {
            found_clan = clan_repository.find_by_abbreviation(clan_name);
        }

        if (!found_clan) {
            char_printf(ch, "'{}' does not refer to a valid clan.\n", clan_name);
            return;
        }
        clan = *found_clan;
    }

    // Show clan information
    if (!clan) {
        char_printf(ch, "Error getting clan information.\n");
        return;
    }

    std::string title = fmt::format("[ Clan {}: {} ]", clan->id(), clan->name());
    paging_printf(ch, "{:-^70}\n", title);

    paging_printf(ch,
                  "Nickname: " AFYEL "{}" ANRM
                  "  "
                  "Ranks: " AFYEL "{}" ANRM
                  "  "
                  "Members: " AFYEL "{}" ANRM
                  "\n"
                  "App Fee: " AFCYN "{}" ANRM
                  "  "
                  "App Level: " AFYEL "{}" ANRM
                  "  "
                  "Dues: " AFCYN "{}" ANRM "\n",
                  clan->abbreviation(), clan->ranks().size(), clan->member_count(), clan->app_fee(),
                  clan->min_application_level(), clan->dues());

    // Show additional information for members
    auto me = std::shared_ptr<CharData>(ch, [](CharData *) {});
    bool show_all =
        GET_LEVEL(ch) >= LVL_IMMORT || (clan->get_membership(me) && clan->has_permission(me, ClanPrivilege::None));

    if (show_all) {
        paging_printf(ch, "Treasure: {}\n", statemoney(clan->treasure()));

        paging_printf(ch, "\nRanks:\n");
        for (size_t i = 0; i < clan->ranks().size(); ++i) {
            paging_printf(ch, "{:3}  {}\n", i + 1, clan->ranks()[i].title());
        }

        paging_printf(ch, "\nPrivileges:\n");
        // Format privilege table
        // TODO: Implement privilege display
    }

    if (!clan->description().empty()) {
        paging_printf(ch, "\nDescription:\n{}", clan->description());
    }

    if (show_all && !clan->motd().empty()) {
        paging_printf(ch, "\nMessage of the Day:\n{}", clan->motd());
    }

    start_paging(ch);
}

// TODO: Implement the rest of the clan commands

ACMD(do_clan) {
    if (IS_NPC(ch) || !ch->desc) {
        char_printf(ch, HUH);
        return;
    }

    Arguments args(argument);
    auto command = args.shift();

    if (command.empty()) {
        // Display help
        char_printf(ch, "Available clan commands:\n");
        char_printf(ch, "   clan list         - List all clans\n");
        char_printf(ch, "   clan info [clan]  - Display clan information\n");
        char_printf(ch, "   clan bank         - Access clan bank\n");
        char_printf(ch, "   clan tell <msg>   - Send a message to all clan members\n");
        // Add more commands as implemented
        return;
    }

    if (matches_start(command, "list")) {
        clan_list(ch, args);
    } else if (matches_start(command, "info")) {
        clan_info(ch, args);
    } else if (matches_start(command, "deposit")) {
        clan_deposit(ch, args);
    } else if (matches_start(command, "withdraw")) {
        clan_withdraw(ch, args);
    } else if (matches_start(command, "tell")) {
        clan_tell(ch, args);
    } else if (matches_start(command, "set")) {
        clan_set(ch, args);
    } else if (matches_start(command, "store")) {
        clan_store(ch, args);
    } else if (matches_start(command, "retrieve")) {
        clan_retrieve(ch, args);
    } else if (matches_start(command, "create")) {
        clan_create(ch, args);
    } else if (matches_start(command, "destroy")) {
        clan_destroy(ch, args);
    } else if (matches_start(command, "alt")) {
        clan_alt(ch, args);
    } else if (matches_start(command, "apply")) {
        clan_apply(ch, args);
    } else {
        char_printf(ch, "Unknown clan command: '{}'\n", command);
    }
}

ACMD(do_ctell) {
    Arguments args(argument);
    if (args.empty()) {
        char_printf(ch, "What do you want to tell your clan?\n");
        return;
    }

    clan_tell(ch, args);
}
