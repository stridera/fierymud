#pragma once

#include "actor.hpp"
#include "money.hpp"

/** Learned ability data stored on Player */
struct LearnedAbility {
    int ability_id = 0;
    std::string name;               // Cached ability name
    std::string plain_name;         // Plain text name for lookups
    std::string description;        // Cached description for help system
    bool known = false;             // Has the player learned this?
    int proficiency = 0;            // 0-100 skill percentage
    std::chrono::system_clock::time_point last_used{};  // Last usage time

    // Ability metadata (cached from database)
    std::string type;               // SPELL, SKILL, CHANT, SONG
    int min_level = 1;              // Minimum level to use
    bool violent = false;           // Is this a combat ability?
    int circle = 0;                 // Spell circle (1-9) for spells, 0 for skills
    std::string sphere;             // Spell sphere (fire, water, healing, etc.)
};

/** Color output level preference */
enum class ColorLevel {
    Off = 0,        // No color codes
    Sparse = 1,     // Minimal color (highlights only)
    Normal = 2,     // Standard color usage
    Complete = 3    // Full color everywhere
};

/** Player preference flags */
enum class PlayerFlag {
    // Display preferences
    Brief = 0,          // Show shortened room descriptions
    Compact,            // Reduce blank lines in output
    NoRepeat,           // Don't echo commands back

    // Auto-actions
    AutoLoot,           // Automatically loot corpses after kills
    AutoGold,           // Automatically take gold from corpses
    AutoSplit,          // Automatically split gold with group
    AutoExit,           // Automatically show exits
    AutoAssist,         // Automatically assist group members

    // Combat preferences
    Wimpy,              // Flee when HP is low (threshold stored separately)
    ShowDiceRolls,      // Show detailed dice rolls and damage calculations in combat

    // Social/Status
    Afk,                // Away from keyboard
    Deaf,               // Ignore shouts and gossip
    NoTell,             // Refuse tells from non-gods
    NoSummon,           // Refuse summon spells
    Quest,              // Currently on a quest

    // PK/Consent
    PkEnabled,          // Player killing enabled
    Consent,            // Consent to dangerous actions from other players

    // Misc
    ColorBlind,         // Use colorblind-friendly colors
    Msp,                // MUD Sound Protocol enabled
    MxpEnabled,         // MUD eXtension Protocol enabled

    // Immortal preferences
    HolyLight,          // See everything (invisible, dark, hidden, etc.)
    ShowIds,            // Show entity IDs on mobs/objects/rooms

    // Count for bitset sizing
    MAX_PLAYER_FLAGS
};

/** Player class */
class Player : public Actor {
public:
    /** Create player */
    static Result<std::unique_ptr<Player>> create(EntityId id, std::string_view name);

    /** Load from JSON */
    static Result<std::unique_ptr<Player>> from_json(const nlohmann::json& json);

    /** Get entity type name */
    std::string_view type_name() const override { return "Player"; }

    /** JSON serialization */
    nlohmann::json to_json() const override;

    /** Display name for players returns their actual name */
    std::string display_name(bool with_article = false) const override;

    /** Communication (Players queue output to session) */
    void send_message(std::string_view message) override;
    void receive_message(std::string_view message) override;

    /** Death handling - becomes ghost (corpse created on release)
     *  Returns nullptr (player corpse created on release, not death) */
    std::shared_ptr<Container> die() override;

    /** Database persistence */
    std::string_view database_id() const { return database_id_; }
    void set_database_id(std::string_view id) { database_id_ = id; }

    /** Player-specific properties */
    std::string_view account() const { return account_; }
    void set_account(std::string_view account) { account_ = account; }

    /** Level management */
    void set_level(int level) { stats().level = std::max(1, level); }

    bool is_online() const { return online_; }
    void set_online(bool value) { online_ = value; }

    /** Start room (login location) management */
    EntityId start_room() const { return start_room_; }
    void set_start_room(EntityId room_id) { start_room_ = room_id; }

    /** Recall room (touchstone/recall point) management */
    EntityId recall_room() const { return recall_room_; }
    void set_recall_room(EntityId room_id) { recall_room_ = room_id; }

    /** Output queue for session handling */
    const std::vector<std::string>& get_output_queue() const { return output_queue_; }
    void clear_output_queue() { output_queue_.clear(); }

    /** Player privileges */
    bool is_god() const { return god_level_ > 0; }
    int god_level() const { return god_level_; }
    void set_god_level(int level) { god_level_ = std::max(0, level); }

    /** Linkdead state management */
    bool is_linkdead() const { return linkdead_; }
    void set_linkdead(bool linkdead) { linkdead_ = linkdead; }

    /** Composer system - multi-line text input mode */
    void start_composing(std::shared_ptr<ComposerSystem> composer);
    void stop_composing();
    void interrupt_composing(std::string_view reason = "");  // Cancel due to external event
    bool is_composing() const { return active_composer_ != nullptr; }
    std::shared_ptr<ComposerSystem> get_composer() const { return active_composer_; }

    /** GMCP support methods */
    nlohmann::json get_vitals_gmcp() const;
    nlohmann::json get_status_gmcp() const;
    void send_gmcp_vitals_update();

    /** Player output interface for network layer */
    void set_output(std::shared_ptr<class PlayerOutput> output) { output_ = output; }
    std::shared_ptr<class PlayerOutput> get_output() const { return output_; }

    /** Convenience methods for network layer */
    std::shared_ptr<Room> current_room_ptr() const { return current_room(); }
    int level() const { return stats().level; }
    std::string player_class() const { return player_class_; }

    /** Override race() to return string instead of string_view for compatibility */
    std::string_view race() const override { return Actor::race(); }

    /** Set character class */
    void set_class(std::string_view character_class) {
        player_class_ = character_class;
        initialize_spell_slots();
    }

    /** Player title management */
    std::string_view title() const { return title_; }
    void set_title(std::string_view player_title) { title_ = player_title; }

    /** Player description */
    std::string_view description() const override { return description_; }
    void set_description(std::string_view desc) override { description_ = desc; }

    /** Player preference flags */
    bool has_player_flag(PlayerFlag flag) const {
        return player_flags_.test(static_cast<size_t>(flag));
    }
    void set_player_flag(PlayerFlag flag, bool value = true) {
        player_flags_.set(static_cast<size_t>(flag), value);
    }
    void toggle_player_flag(PlayerFlag flag) {
        player_flags_.flip(static_cast<size_t>(flag));
    }
    void clear_player_flag(PlayerFlag flag) { set_player_flag(flag, false); }

    /** Convenience flag accessors */
    bool is_brief() const { return has_player_flag(PlayerFlag::Brief); }
    bool is_compact() const { return has_player_flag(PlayerFlag::Compact); }
    bool is_autoloot() const { return has_player_flag(PlayerFlag::AutoLoot); }
    bool is_autogold() const { return has_player_flag(PlayerFlag::AutoGold); }
    bool is_autosplit() const { return has_player_flag(PlayerFlag::AutoSplit); }
    bool is_autoexit() const { return has_player_flag(PlayerFlag::AutoExit); }
    bool is_afk() const override { return has_player_flag(PlayerFlag::Afk); }
    bool is_deaf() const { return has_player_flag(PlayerFlag::Deaf); }
    bool is_notell() const { return has_player_flag(PlayerFlag::NoTell); }
    bool is_pk_enabled() const { return has_player_flag(PlayerFlag::PkEnabled); }
    bool is_holylight() const override { return has_player_flag(PlayerFlag::HolyLight); }
    bool is_show_ids() const override { return has_player_flag(PlayerFlag::ShowIds); }
    bool is_show_dice_rolls() const { return has_player_flag(PlayerFlag::ShowDiceRolls); }

    /** AFK message (displayed to people who try to interact) */
    std::string_view afk_message() const { return afk_message_; }
    void set_afk_message(std::string_view msg) { afk_message_ = msg; }

    /** Prompt format string - used for command prompt display */
    std::string_view prompt() const { return prompt_; }
    void set_prompt(std::string_view prompt_format) { prompt_ = prompt_format; }

    /** Color level preference */
    ColorLevel color_level() const { return color_level_; }
    void set_color_level(ColorLevel level) { color_level_ = level; }

    /** Wimpy threshold - flee when HP falls below this value */
    int wimpy_threshold() const { return wimpy_threshold_; }
    void set_wimpy_threshold(int threshold) {
        wimpy_threshold_ = std::max(0, threshold);
        set_player_flag(PlayerFlag::Wimpy, wimpy_threshold_ > 0);
    }

    /** Get player flags as strings (for database persistence) */
    std::vector<std::string> get_player_flags_as_strings() const;

    /** Set player flags from strings (from database load) */
    void set_player_flags_from_strings(const std::vector<std::string>& flags);

    // =========================================================================
    // Currency (Money-based API)
    // =========================================================================

    /** Get wallet (money on person) */
    fiery::Money& wallet() { return wallet_; }
    const fiery::Money& wallet() const { return wallet_; }

    /** Set wallet */
    void set_wallet(const fiery::Money& money) { wallet_ = money; }

    /** Get bank account */
    fiery::Money& bank() { return bank_; }
    const fiery::Money& bank() const { return bank_; }

    /** Set bank account */
    void set_bank(const fiery::Money& money) { bank_ = money; }

    /** Check if wallet can afford an amount */
    bool can_afford(const fiery::Money& cost) const { return wallet_.can_afford(cost); }
    bool can_afford(long copper_cost) const override { return wallet_.can_afford(copper_cost); }

    // Unified Wealth Interface implementation (uses wallet)
    long wealth() const override { return wallet_.value(); }
    void give_wealth(long copper_amount) override {
        if (copper_amount > 0) wallet_.receive(copper_amount);
    }
    bool take_wealth(long copper_amount) override {
        return spend(copper_amount);
    }

    /** Check if bank can afford an amount */
    bool bank_can_afford(const fiery::Money& cost) const { return bank_.can_afford(cost); }
    bool bank_can_afford(long copper_cost) const { return bank_.can_afford(copper_cost); }

    /** Spend from wallet (uses highest denominations first, makes change)
     *  Returns true if successful, false if insufficient funds */
    bool spend(const fiery::Money& cost) { return wallet_.charge(cost); }
    bool spend(long copper_cost) { return wallet_.charge(copper_cost); }

    /** Spend from bank
     *  Returns true if successful, false if insufficient funds */
    bool spend_from_bank(const fiery::Money& cost) { return bank_.charge(cost); }
    bool spend_from_bank(long copper_cost) { return bank_.charge(copper_cost); }

    /** Receive money into wallet */
    void receive(const fiery::Money& money) { wallet_ += money; }
    void receive(long copper_amount) { wallet_.receive(copper_amount); }

    /** Deposit from wallet to bank */
    bool deposit(const fiery::Money& amount) {
        if (!wallet_.charge(amount)) return false;
        bank_ += amount;
        return true;
    }

    /** Withdraw from bank to wallet */
    bool withdraw(const fiery::Money& amount) {
        if (!bank_.charge(amount)) return false;
        wallet_ += amount;
        return true;
    }

    // =========================================================================
    // Account Bank (shared across all characters on account)
    // =========================================================================

    /** Get account bank (shared wealth accessible by all characters on account) */
    fiery::Money& account_bank() { return account_bank_; }
    const fiery::Money& account_bank() const { return account_bank_; }

    /** Set account bank */
    void set_account_bank(const fiery::Money& money) { account_bank_ = money; }

    /** Check if account bank can afford an amount */
    bool account_can_afford(const fiery::Money& cost) const { return account_bank_.can_afford(cost); }
    bool account_can_afford(long copper_cost) const { return account_bank_.can_afford(copper_cost); }

    /** Deposit from wallet to account bank */
    bool account_deposit(const fiery::Money& amount) {
        if (!wallet_.charge(amount)) return false;
        account_bank_ += amount;
        return true;
    }

    /** Withdraw from account bank to wallet */
    bool account_withdraw(const fiery::Money& amount) {
        if (!account_bank_.charge(amount)) return false;
        wallet_ += amount;
        return true;
    }

    /** User account link for database persistence */
    std::string_view user_id() const { return user_id_; }
    void set_user_id(std::string_view id) { user_id_ = id; }
    bool has_user_account() const { return !user_id_.empty(); }

    /** Transfer money to another player */
    bool transfer_to(Player& recipient, const fiery::Money& amount) {
        if (!wallet_.charge(amount)) return false;
        recipient.wallet() += amount;
        return true;
    }

    /** Take all money from a Mobile (for loot) */
    fiery::Money loot_money_from(class Mobile& mob);

    /** Time tracking */
    std::time_t creation_time() const { return creation_time_; }
    std::time_t last_logon_time() const { return last_logon_time_; }
    std::chrono::seconds total_play_time() const { return total_play_time_; }
    void set_last_logon(std::time_t time) { last_logon_time_ = time; }
    void add_play_time(std::chrono::seconds time) { total_play_time_ += time; }

    /** PK mode timing */
    std::time_t pk_enabled_time() const { return pk_enabled_time_; }
    void set_pk_enabled_time(std::time_t time) { pk_enabled_time_ = time; }
    bool can_disable_pk() const {
        if (pk_enabled_time_ == 0) return true;
        constexpr std::time_t SECONDS_IN_24_HOURS = 24 * 60 * 60;
        return (std::time(nullptr) - pk_enabled_time_) >= SECONDS_IN_24_HOURS;
    }

    /** Clan/Guild membership */
    bool in_clan() const { return clan_id_ > 0; }
    unsigned int clan_id() const { return clan_id_; }
    int clan_rank_index() const { return clan_rank_index_; }
    std::string_view clan_name() const { return clan_name_; }
    std::string_view clan_abbreviation() const { return clan_abbreviation_; }
    std::string_view clan_rank_title() const { return clan_rank_title_; }
    void set_clan(unsigned int clan_id, std::string_view name, std::string_view abbreviation,
                  int rank_index, std::string_view rank_title) {
        clan_id_ = clan_id;
        clan_name_ = name;
        clan_abbreviation_ = abbreviation;
        clan_rank_index_ = rank_index;
        clan_rank_title_ = rank_title;
    }
    void clear_clan() {
        clan_id_ = 0;
        clan_rank_index_ = -1;
        clan_name_.clear();
        clan_abbreviation_.clear();
        clan_rank_title_.clear();
    }

    /** Initialize spell slots based on class and level */
    void initialize_spell_slots();

protected:
    Player(EntityId id, std::string_view name);

    /** Room change notification - sends GMCP room updates */
    void on_room_change(std::shared_ptr<Room> old_room, std::shared_ptr<Room> new_room) override;

    /** Position change notification - sends GMCP vitals updates */
    void on_position_change(Position old_pos, Position new_pos) override;

    /** Level up notification - sends GMCP vitals updates */
    void on_level_up(int old_level, int new_level) override;

private:
    std::string database_id_;  // UUID from database for persistence
    std::string account_;
    bool online_ = false;
    bool linkdead_ = false;  // Player connection lost but still in world
    int god_level_ = 0;
    std::shared_ptr<ComposerSystem> active_composer_;  // Multi-line text composer
    EntityId start_room_ = INVALID_ENTITY_ID;  // Player's login location (where they logged out)
    EntityId recall_room_ = INVALID_ENTITY_ID; // Player's recall/touchstone location
    std::vector<std::string> output_queue_;
    std::shared_ptr<class PlayerOutput> output_;

    // Character creation fields
    std::string player_class_ = "warrior";  // Default class
    std::string title_ = "";                // Player title
    std::string description_ = "";          // Player description

    // Player preferences
    std::bitset<static_cast<size_t>(PlayerFlag::MAX_PLAYER_FLAGS)> player_flags_;
    int wimpy_threshold_ = 0;               // Flee when HP falls below this
    std::string afk_message_ = "";          // AFK message to display
    std::string prompt_ = "<%h/%Hhp %v/%Vs>";  // Prompt format string
    ColorLevel color_level_ = ColorLevel::Normal;  // Color output preference

    // Currency
    fiery::Money wallet_;        // Money on person
    fiery::Money bank_;          // Money in character's bank account
    fiery::Money account_bank_;  // Money in shared account bank (all characters)
    std::string user_id_;        // UUID of linked user account (for account bank persistence)

    // Time tracking fields
    std::time_t creation_time_ = std::time(nullptr);  // Time of character creation
    std::time_t last_logon_time_ = 0;                 // Time of last logon
    std::chrono::seconds total_play_time_{0};         // Total accumulated play time
    std::time_t pk_enabled_time_ = 0;                 // When PK mode was enabled (0 = never)

    // Clan/Guild membership
    unsigned int clan_id_ = 0;               // Clan ID (0 = no clan)
    int clan_rank_index_ = -1;               // Rank within clan (-1 = not in clan)
    std::string clan_name_;                  // Cached clan name for display
    std::string clan_abbreviation_;          // Cached clan abbreviation
    std::string clan_rank_title_;            // Cached rank title for display

    // Communication tracking
    std::string last_tell_sender_;           // Name of last player who sent us a tell
    std::vector<std::string> tell_history_;  // Recent tell history
    static constexpr size_t MAX_TELL_HISTORY = 20;

    // Text composer recovery (for 'redo' command)
    std::vector<std::string> redo_buffer_;   // Last interrupted text composition

    // Ability/Skill system
    std::unordered_map<int, LearnedAbility> abilities_;  // ability_id -> learned ability data

    // Command aliases
    std::unordered_map<std::string, std::string> aliases_;  // alias -> command

    // Ignored players (by character name, lowercase)
    std::unordered_set<std::string> ignored_players_;

    // Group/follow system
    std::weak_ptr<Actor> leader_;            // Who we are following (empty = we are leader or solo)
    std::vector<std::weak_ptr<Actor>> followers_; // Who is following us
    bool group_flag_ = true;                 // Whether we accept group invites

    // Meditation state
    bool is_meditating_ = false;             // Currently meditating for spell restoration

public:
    // Meditation methods (increases effective focus for spell slot restoration)
    bool is_meditating() const { return is_meditating_; }
    void start_meditation();
    void stop_meditation();

    /**
     * Get effective focus for spell slot restoration.
     * Meditation doubles the focus rate.
     */
    int get_spell_restore_rate() const;

    // Communication tracking methods
    void set_last_tell_sender(std::string_view sender) { last_tell_sender_ = sender; }
    std::string_view last_tell_sender() const { return last_tell_sender_; }

    void add_tell_to_history(std::string_view entry) {
        tell_history_.push_back(std::string(entry));
        if (tell_history_.size() > MAX_TELL_HISTORY) {
            tell_history_.erase(tell_history_.begin());
        }
    }
    const std::vector<std::string>& tell_history() const { return tell_history_; }
    void clear_tell_history() { tell_history_.clear(); }

    // Text composer redo buffer methods
    void set_redo_buffer(const std::vector<std::string>& lines) { redo_buffer_ = lines; }
    void set_redo_buffer(std::vector<std::string>&& lines) { redo_buffer_ = std::move(lines); }
    const std::vector<std::string>& redo_buffer() const { return redo_buffer_; }
    void clear_redo_buffer() { redo_buffer_.clear(); }
    bool has_redo_buffer() const { return !redo_buffer_.empty(); }

    // Group/follow system methods
    bool has_group() const {
        return !leader_.expired() || !followers_.empty();
    }

    bool is_group_leader() const {
        return leader_.expired() && !followers_.empty();
    }

    bool is_following() const {
        return !leader_.expired();
    }

    std::shared_ptr<Actor> get_leader() const {
        return leader_.lock();
    }

    const std::vector<std::weak_ptr<Actor>>& get_followers() const {
        return followers_;
    }

    void set_leader(std::shared_ptr<Actor> leader) {
        leader_ = leader;
    }

    void clear_leader() {
        leader_.reset();
    }

    void add_follower(std::shared_ptr<Actor> follower) {
        // Clean up any expired weak pointers first
        followers_.erase(
            std::remove_if(followers_.begin(), followers_.end(),
                [](const std::weak_ptr<Actor>& wp) { return wp.expired(); }),
            followers_.end());
        followers_.push_back(follower);
    }

    void remove_follower(std::shared_ptr<Actor> follower) {
        followers_.erase(
            std::remove_if(followers_.begin(), followers_.end(),
                [&follower](const std::weak_ptr<Actor>& wp) {
                    auto sp = wp.lock();
                    return !sp || sp == follower;
                }),
            followers_.end());
    }

    bool group_flag() const { return group_flag_; }
    void set_group_flag(bool value) { group_flag_ = value; }

    void send_to_group(std::string_view message);

    // ============================================================================
    // Ability/Skill System
    // ============================================================================

    /** Check if player has learned an ability (at any proficiency) */
    bool has_ability(int ability_id) const {
        auto it = abilities_.find(ability_id);
        return it != abilities_.end() && it->second.known;
    }

    /** Get a specific learned ability (nullptr if not found) */
    const LearnedAbility* get_ability(int ability_id) const {
        auto it = abilities_.find(ability_id);
        return (it != abilities_.end()) ? &it->second : nullptr;
    }

    /** Get mutable ability (nullptr if not found) */
    LearnedAbility* get_ability_mutable(int ability_id) {
        auto it = abilities_.find(ability_id);
        return (it != abilities_.end()) ? &it->second : nullptr;
    }

    /** Get all learned abilities (const reference) */
    const std::unordered_map<int, LearnedAbility>& get_abilities() const {
        return abilities_;
    }

    /** Add or update a learned ability */
    void set_ability(const LearnedAbility& ability) {
        abilities_[ability.ability_id] = ability;
    }

    /** Get proficiency for a specific ability (0 if not learned) */
    int get_proficiency(int ability_id) const {
        auto it = abilities_.find(ability_id);
        return (it != abilities_.end() && it->second.known) ? it->second.proficiency : 0;
    }

    /** Set proficiency for an ability (must already exist) */
    bool set_proficiency(int ability_id, int proficiency) {
        auto it = abilities_.find(ability_id);
        if (it != abilities_.end()) {
            it->second.proficiency = std::clamp(proficiency, 0, 100);
            return true;
        }
        return false;
    }

    /** Mark ability as known/learned */
    bool learn_ability(int ability_id) {
        auto it = abilities_.find(ability_id);
        if (it != abilities_.end()) {
            it->second.known = true;
            return true;
        }
        return false;
    }

    /** Get abilities filtered by type (SPELL, SKILL, CHANT, SONG) */
    std::vector<const LearnedAbility*> get_abilities_by_type(std::string_view type) const {
        std::vector<const LearnedAbility*> result;
        for (const auto& [id, ability] : abilities_) {
            if (ability.type == type) {
                result.push_back(&ability);
            }
        }
        return result;
    }

    /** Get all known abilities (learned with proficiency > 0) */
    std::vector<const LearnedAbility*> get_known_abilities() const {
        std::vector<const LearnedAbility*> result;
        for (const auto& [id, ability] : abilities_) {
            if (ability.known) {
                result.push_back(&ability);
            }
        }
        return result;
    }

    /** Find an ability by name (partial match, case-insensitive) */
    const LearnedAbility* find_ability_by_name(std::string_view search) const {
        std::string search_lower{search};
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

        for (const auto& [id, ability] : abilities_) {
            std::string name_lower = ability.name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
            std::string plain_lower = ability.plain_name;
            std::transform(plain_lower.begin(), plain_lower.end(), plain_lower.begin(), ::tolower);

            // Match from beginning of either name format
            if (name_lower.find(search_lower) == 0 || plain_lower.find(search_lower) == 0) {
                return &ability;
            }
        }
        return nullptr;
    }

    /** Record ability usage time */
    void record_ability_use(int ability_id) {
        auto it = abilities_.find(ability_id);
        if (it != abilities_.end()) {
            it->second.last_used = std::chrono::system_clock::now();
        }
    }

    /** Clear all abilities (for reloading from database) */
    void clear_abilities() {
        abilities_.clear();
    }

    // ============================================================================
    // Alias System
    // ============================================================================

    /** Get all aliases */
    const std::unordered_map<std::string, std::string>& get_aliases() const {
        return aliases_;
    }

    /** Set an alias (creates or updates) */
    void set_alias(std::string_view alias, std::string_view command) {
        aliases_[std::string(alias)] = std::string(command);
    }

    /** Remove an alias */
    bool remove_alias(std::string_view alias) {
        return aliases_.erase(std::string(alias)) > 0;
    }

    /** Check if an alias exists */
    bool has_alias(std::string_view alias) const {
        return aliases_.find(std::string(alias)) != aliases_.end();
    }

    /** Get the command for an alias (returns nullopt if not found) */
    std::optional<std::string_view> get_alias(std::string_view alias) const {
        auto it = aliases_.find(std::string(alias));
        if (it != aliases_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /** Clear all aliases */
    void clear_aliases() {
        aliases_.clear();
    }

    // ============================================================================
    // Ignored Players System
    // ============================================================================

    /** Ignore a player (stores lowercase name) */
    void ignore_player(std::string_view player_name) {
        std::string name_lower{player_name};
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
        ignored_players_.insert(name_lower);
    }

    /** Stop ignoring a player */
    bool unignore_player(std::string_view player_name) {
        std::string name_lower{player_name};
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
        return ignored_players_.erase(name_lower) > 0;
    }

    /** Check if ignoring a player */
    bool is_ignoring(std::string_view player_name) const {
        std::string name_lower{player_name};
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
        return ignored_players_.find(name_lower) != ignored_players_.end();
    }

    /** Get all ignored players */
    const std::unordered_set<std::string>& get_ignored_players() const {
        return ignored_players_;
    }

    /** Clear all ignored players */
    void clear_ignored_players() {
        ignored_players_.clear();
    }
};

