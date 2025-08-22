# MUD File Formats Documentation

This document provides an overview and detailed breakdown of the various file formats used by FieryMUD. Understanding these formats is crucial for debugging, creating new content, and developing tools that interact with the MUD's data.

The information presented here is derived from the Python scripts located in the `scripts/mud/` directory, which are used to parse and convert these files.

## Common Concepts

Several concepts are common across most MUD file formats:

*   **Delimiters**: Files often use specific characters or sequences to delimit sections or records:
    *   `~`: Typically terminates strings.
    *   `#`: Denotes the start of a new record (e.g., a new room, mob, or object).
    *   `$`: Marks the end of a list or a file.
    *   `~~`: Used in player object files (`.objs`) to separate individual objects.
*   **String Handling**: Strings are generally terminated by a `~` character. When reading, this `~` is consumed.
*   **Key-Value Pairs**: Many data fields are represented as `key: value` on a single line.
*   **Bitflags**: Flags (e.g., for object properties, room characteristics, or player preferences) are often stored as integers or as ASCII characters, where each character represents a specific bit. The `scripts/mud/bitflags.py` and `scripts/mud/flags.py` modules define how these are encoded and decoded.
*   **Nested Structures**: Some data types contain nested information (e.g., a container object containing other objects). These are typically handled by recursive parsing or by specific delimiters.

## File Type Specifics

### Player File (`.plr`)

**Overview:**
The `.plr` file stores the main character data for a player. It's a text-based file with key-value pairs and some structured sections. It contains core player attributes, statistics, and references to other player-related files (like inventory, quests, pets).

**Structure Breakdown (based on `scripts/mud/types/player.py` and `scripts/mud/parser.py`):**

The `Player` dataclass in `scripts/mud/types/player.py` defines the high-level structure. The `Parser.parse` method in `scripts/mud/parser.py` details how each field is read from the `MudData` object.

Here's a breakdown of the fields and their expected format in the `.plr` file:

*   **`id`**: Integer. Read from `id: <value>`.
*   **`name`**: String. Read from `name: <value>`.
*   **`password`**: String. Read from `password: <value>`.
*   **`prompt`**: String. Read from `prompt: <value>`.
*   **`level`**: Integer. Read from `level: <value>`.
*   **`home`**: String. Read from `home: <value>`.
*   **`birth_time`**: Timestamp (integer, Unix epoch). Read from `birthtime: <value>`.
*   **`time_played`**: Integer (seconds). Read from `timeplayed: <value>`.
*   **`last_login_time`**: Timestamp (integer, Unix epoch). Read from `lastlogintime: <value>`.
*   **`host`**: IP Address (string, dot-separated octets). Read from `host: <value>`.
*   **`height`**: Integer. Read from `height: <value>`.
*   **`weight`**: Integer. Read from `weight: <value>`.
*   **`base_height`**: Integer. Read from `base_height: <value>`.
*   **`base_weight`**: Integer. Read from `base_weight: <value>`.
*   **`base_size`**: Integer. Read from `base_size: <value>`.
*   **`natural_size`**: Integer. Read from `natural_size: <value>`.
*   **`alignment`**: Integer. Read from `alignment: <value>`.
*   **`saving_throws`**: Five integers separated by spaces. Read from `savingthrows: <val1> <val2> <val3> <val4> <val5>`.
*   **`load_room`**: Integer. Read from `loadroom: <value>`.
*   **`last_level`**: Integer. Read from `lastlevel: <value>`.
*   **`hit_points`**: `current/max` string. Read from `hitpoints: <current>/<max>`.
*   **`move`**: `current/max` string. Read from `move: <current>/<max>`.
*   **`damage_roll`**: Integer. Read from `damroll: <value>`.
*   **`hit_roll`**: Integer. Read from `hitroll: <value>`.
*   **`stats`**: Nested key-value pairs for `Str`, `Int`, `Wis`, `Dex`, `Con`, `Cha`.
    ```
    Str: <value>
    Int: <value>
    Wis: <value>
    Dex: <value>
    Con: <value>
    Cha: <value>
    ```
*   **`experience`**: Integer. Read from `experience: <value>`.
*   **`save_room`**: Integer. Read from `saveroom: <value>`.
*   **`gender`**: Integer (enum value, see `Gender` enum in `types/__init__.py`). Read from `sex: <value>`.
*   **`player_class`**: Integer (enum value, see `Class` enum in `types/__init__.py`). Read from `class: <value>`.
*   **`races`**: Integer (enum value, see `Race` enum in `types/__init__.py`). Read from `race: <value>`.
*   **`life_force`**: Integer (enum value, see `LifeForce` enum in `types/__init__.py`). Read from `lifeforce: <value>`.
*   **`composition`**: Integer (enum value, see `Composition` enum in `types/__init__.py`). Read from `composition: <value>`.
*   **`player_flags`**: ASCII flags (see `PLAYER_FLAGS` in `flags.py`). Read from `playerflags: <value>`.
*   **`effects`**: List of dictionaries. Read from `effects:` followed by lines of `effect_type duration modifier location flags` until `0 0 0`.
*   **`effect_flags`**: ASCII flags (see `EFFECTS` in `flags.py`). Read from `effectflags: <value>`.
*   **`preference_flags`**: ASCII flags (see `PREFERENCE_FLAGS` in `flags.py`). Read from `prefflags: <value>`.
*   **`privilege_flags`**: ASCII flags (see `PRIVILEGE_FLAGS` in `flags.py`). Read from `privflags: <value>`.
*   **`quit_reason`**: Integer (enum value, see `QuitReason` enum in `types/__init__.py`). Read from `quit_reason: <value>`.
*   **`description`**: String. Read from `desc: <value>` (if value is empty, then `mudfile.read_string()` is used to read a multi-line string terminated by `~`).
*   **`skills`**: Dictionary of skill name to level. Read from `skills:` followed by lines of `skill_id level` until `0 0`. `skill_id` maps to `SKILLS` in `flags.py`.
*   **`trophy`**: List of dictionaries. Read from `trophy:` followed by lines of `kill_type id count` until `0 0`. `kill_type` maps to `KillTypeFlags` in `types/__init__.py`.
*   **`tells`**: List of lists. Read from `tells:` followed by lines of `timestamp comment` until `$`.
*   **`gossips`**: List of lists. Read from `gossips:` followed by lines of `timestamp comment` until `$`.
*   **`aliases`**: Dictionary of alias to replacement. Read from `aliases:` followed by lines of `alias command` until `0`.
*   **`cooldowns`**: Dictionary of cooldown name to `time/max` dictionary. Read from `cooldowns:` followed by lines of `cooldown_id timedata` until `-1 0`. `cooldown_id` maps to `Cooldown` in `types/__init__.py`.
*   **`spell_casts`**: Dictionary of spell name to count. Read from `spellcasts:` followed by lines of `spell_id count` until `0 0`. `spell_id` maps to `SPELLS` in `flags.py`.
*   **`wimpy`**: Integer. Read from `wimpy: <value>`.
*   **`auto_invis`**: Integer. Read from `autoinvis: <value>`.
*   **`money`**: Platinum, gold, silver, copper. Read from `cash: <plat> <gold> <silver> <copper>`.
*   **`bank`**: Platinum, gold, silver, copper. Read from `bank: <plat> <gold> <silver> <copper>`.
*   **`clan`**: Integer. Read from `clan: <value>`.
*   **`title`**: String. Read from `title: <value>`.
*   **`current_title`**: String. Read from `current_title: <value>`.
*   **`hunger`**: Integer. Read from `hunger: <value>`.
*   **`thirst`**: Integer. Read from `thirst: <value>`.
*   **`drunkenness`**: Integer. Read from `drunkenness: <value>`.
*   **`size`**: Integer. Read from `size: <value>`.
*   **`armor_class`**: Integer. Read from `ac: <value>`.
*   **`aggression`**: Integer. Read from `aggression: <value>`.
*   **`page_length`**: Integer. Read from `pagelength: <value>`.
*   **`log_view`**: Integer. Read from `logview: <value>`.
*   **`bad_passwords`**: Integer. Read from `badpasswords: <value>`.
*   **`freeze_level`**: Integer. Read from `freezelevel: <value>`.
*   **`wiz_title`**: String. Read from `wiztitle: <value>`.
*   **`poof_in`**: String. Read from `poofin: <value>`.
*   **`poof_out`**: String. Read from `poofout: <value>`.
*   **`invis_level`**: Integer. Read from `invislevel: <value>`.
*   **`olc_zones`**: List of integers. Read from `olczones: <value>`.
*   **`grant_groups`**: List of dictionaries. Read from `grantgroups:` followed by lines of `command grantor level` until `~`.
*   **`grants`**: List of dictionaries. Read from `grants:` followed by lines of `command grantor level` until `~`.

This is the content for the `.plr` file format. I will now write this to `MUD_FILE_FORMATS.md`.
