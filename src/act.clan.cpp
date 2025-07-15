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
#include "function_registration.hpp"

#include <expected>
#include <fmt/format.h>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#define CLANCMD(name) static void(name)(CharData * ch, Arguments argument)
enum ClanPermissions : PermissionFlags {
    Everybody = 0,
    ClanMember = 1,
    ClanAdmin = 2,
    God = 3,
};

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
REGISTER_FUNCTION(clan_list, "clan_list", 0, ClanPermissions::Everybody,
                "List all clans and their application fees");

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
REGISTER_FUNCTION(clan_deposit, "clan_deposit", 0, ClanPermissions::ClanMember,
                "Deposit money into the clan treasury");

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
REGISTER_FUNCTION(clan_withdraw, "clan_withdraw", 0, ClanPermissions::ClanMember,
                "Withdraw money from the clan treasury");

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
REGISTER_FUNCTION(clan_store, "clan_store", 0, ClanPermissions::ClanMember,
                "Store an item in the clan vault");

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
    // Build a temporary cache for faster lookup
    std::unordered_map<std::string, ObjectId> name_to_vnum_cache;
    for (const auto &[vnum, count] : membership->get_clan_storage()) {
        if (count <= 0)
            continue;

        auto obj = read_object(vnum, VIRTUAL);
        if (!obj)
            continue;

        // Cache all possible names for this object
        std::string names = obj->name;
        std::string name_token;
        std::istringstream name_stream(names);
        while (name_stream >> name_token) {
            name_to_vnum_cache[name_token] = vnum;
        }
        free_obj(obj);
    }
    
    // Quick lookup in cache
    auto cache_it = name_to_vnum_cache.find(std::string(obj_name));
    if (cache_it != name_to_vnum_cache.end()) {
        obj_vnum = cache_it->second;
        found = true;
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
REGISTER_FUNCTION(clan_retrieve, "clan_retrieve", 0, ClanPermissions::ClanMember,
                "Retrieve an item from the clan vault");

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
        // Find the online character by name
        auto target = find_char_in_world(find_by_name(const_cast<char*>(member.name.c_str())));
        if (!target || !target->desc || !IS_PLAYING(target->desc))
            continue;

        auto tch = REAL_CHAR(target);
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
REGISTER_FUNCTION(clan_tell, "clan_tell", 0, ClanPermissions::ClanMember,
                "Send a message to your clan");

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
        
        if (*fee_opt < 0) {
            char_printf(ch, "Application fee cannot be negative.\n");
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
        
        if (*rank_opt < 1) {
            char_printf(ch, "Rank number must be positive.\n");
            return;
        }
        
        auto ranks = membership->get_clan_ranks();
        if (*rank_opt > ranks.size()) {
            char_printf(ch, "Invalid rank number. Clan has {} ranks.\n", ranks.size());
            return;
        }

        auto result = membership->delete_clan_rank(*rank_opt - 1);  // Convert to 0-based index
        if (result) {
            char_printf(ch, "Rank {} deleted.\n", *rank_opt);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "dues")) {
        auto dues_opt = argument.try_shift_number();
        if (!dues_opt) {
            char_printf(ch, "How much platinum should the clan's dues be?\n");
            return;
        }
        
        if (*dues_opt < 0) {
            char_printf(ch, "Dues cannot be negative.\n");
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
        
        if (*rank_opt < 1) {
            char_printf(ch, "Rank number must be positive.\n");
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
        
        auto ranks = membership->get_clan_ranks();
        if (*rank_opt > ranks.size()) {
            char_printf(ch, "Invalid rank number. Clan has {} ranks.\n", ranks.size());
            return;
        }

        auto result = membership->update_clan_rank_title(*rank_opt - 1, std::string(title));  // Convert to 0-based index
        if (result) {
            char_printf(ch, "Rank {} title set to '{}.'\n", *rank_opt, title);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else {
        log(LogSeverity::Error, LVL_GOD, "SYSERR: clan_set: unknown subcommand '{}'", cmd);
        return;
    }

    clan_repository.save();
}
REGISTER_FUNCTION(clan_set, "clan_set", 0, ClanPermissions::ClanAdmin,
                "Set clan properties (abbreviation, name, application fee, etc.)");


CLANCMD(clan_apply) {
    if (argument.empty()) {
        char_printf(ch, "Apply to which clan?\n");
        return;
    }
    
    auto clan_name = argument.shift();
    auto clan = clan_repository.find_by_name(clan_name);
    if (!clan) {
        clan = clan_repository.find_by_abbreviation(clan_name);
    }
    
    if (!clan) {
        char_printf(ch, "No such clan exists.\n");
        return;
    }
    
    // Check if character meets application requirements
    if (GET_LEVEL(ch) < (*clan)->min_application_level()) {
        char_printf(ch, "You must be at least level {} to apply to {}.\n", 
                   (*clan)->min_application_level(), (*clan)->name());
        return;
    }
    
    // Check if character can afford application fee
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        Money app_fee;
        app_fee[PLATINUM] = (*clan)->app_fee();
        Money owned = ch->points.money;
        if (!owned.charge(app_fee)) {
            char_printf(ch, "You cannot afford the {} platinum application fee.\n", (*clan)->app_fee());
            return;
        }
        
        // Charge the fee
        for (int i = 0; i < NUM_COIN_TYPES; i++) {
            ch->points.money[i] -= app_fee[i];
        }
    }
    
    // Check if already a member
    auto me = std::shared_ptr<CharData>(ch, [](CharData *) {});
    if ((*clan)->get_membership(me)) {
        char_printf(ch, "You are already a member of {}.\n", (*clan)->name());
        return;
    }
    
    // For now, applications are disabled - clan members must manually invite
    char_printf(ch, "Clan applications are currently disabled. Contact a clan member to be invited.\n");
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
    auto clan_id_opt = membership->get_clan_id();
    if (!clan_id_opt) {
        char_printf(ch, "Error: Invalid clan ID.\n");
        return;
    }
    auto clan_id = *clan_id_opt;

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
