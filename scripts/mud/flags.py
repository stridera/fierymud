SPELLS = [
    "NONE",
    "ARMOR",  # 1
    "TELEPORT",  # 2
    "BLESS",  # 3
    "BLINDNESS",  # 4
    "BURNING_HANDS",  # 5
    "CALL_LIGHTNING",  # 6
    "CHARM",  # 7
    "CHILL_TOUCH",  # 8
    "CLONE",  # 9
    "COLOR_SPRAY",  # 10
    "CONTROL_WEATHER",  # 11
    "CREATE_FOOD",  # 12
    "CREATE_WATER",  # 13
    "CURE_BLIND",  # 14
    "CURE_CRITIC",  # 15
    "CURE_LIGHT",  # 16
    "CURSE",  # 17
    "DETECT_ALIGN",  # 18
    "DETECT_INVIS",  # 19
    "DETECT_MAGIC",  # 20
    "DETECT_POISON",  # 21
    "DISPEL_EVIL",  # 22
    "EARTHQUAKE",  # 23
    "ENCHANT_WEAPON",  # 24
    "ENERGY_DRAIN",  # 25
    "FIREBALL",  # 26
    "HARM",  # 27
    "HEAL",  # 28
    "INVISIBLE",  # 29
    "LIGHTNING_BOLT",  # 30
    "LOCATE_OBJECT",  # 31
    "MAGIC_MISSILE",  # 32
    "POISON",  # 33
    "PROT_FROM_EVIL",  # 34
    "REMOVE_CURSE",  # 35
    "SANCTUARY",  # 36
    "SHOCKING_GRASP",  # 37
    "SLEEP",  # 38
    "STRENGTH",  # 39
    "SUMMON",  # 40
    "VENTRILOQUATE",  # 41
    "WORD_OF_RECALL",  # 42
    "REMOVE_POISON",  # 43
    "SENSE_LIFE",  # 44
    "ANIMATE_DEAD",  # 45
    "DISPEL_GOOD",  # 46
    "GROUP_ARMOR",  # 47
    "GROUP_HEAL",  # 48
    "GROUP_RECALL",  # 49
    "INFRAVISION",  # 50
    "WATERWALK",  # 51
    "STONE_SKIN",  # 52
    "FULL_HEAL",  # 53
    "FULL_HARM",  # 54
    "WALL_OF_FOG",  # 55
    "WALL_OF_STONE",  # 56
    "FLY",  # 57
    "SUMMON_DRACOLICH",  # 58
    "SUMMON_ELEMENTAL",  # 59
    "SUMMON_DEMON",  # 60
    "SUMMON_GREATER_DEMON",  # 61
    "DIMENSION_DOOR",  # 62
    "CREEPING_DOOM",  # 63
    "DOOM",  # 64
    "METEORSWARM",  # 65
    "BIGBYS_CLENCHED_FIST",  # 66
    "FARSEE",  # 67
    "HASTE",  # 68
    "BLUR",  # 69
    "GREATER_ENDURANCE",  # 70
    "MOONWELL",  # 71
    "INN_STRENGTH",  # 72
    "DARKNESS",  # 73
    "ILLUMINATION",  # 74
    "COMPREHEND_LANG",  # 75
    "CONE_OF_COLD",  # 76
    "ICE_STORM",  # 77
    "ICE_SHARDS",  # 78
    "MAJOR_PARALYSIS",  # 79
    "VAMPIRIC_BREATH",  # 80
    "RESURRECT",  # 81
    "INCENDIARY_NEBULA",  # 82
    "MINOR_PARALYSIS",  # 83
    "CAUSE_LIGHT",  # 84
    "CAUSE_SERIOUS",  # 85
    "CAUSE_CRITIC",  # 86
    "PRESERVE",  # 87
    "CURE_SERIOUS",  # 88
    "VIGORIZE_LIGHT",  # 89
    "VIGORIZE_SERIOUS",  # 90
    "VIGORIZE_CRITIC",  # 91
    "SOULSHIELD",  # 92
    "DESTROY_UNDEAD",  # 93
    "SILENCE",  # 94
    "FLAMESTRIKE",  # 95
    "UNHOLY_WORD",  # 96
    "HOLY_WORD",  # 97
    "PLANE_SHIFT",  # 98
    "DISPEL_MAGIC",  # 99
    "MINOR_CREATION",  # 100
    "CONCEALMENT",  # 101
    "RAY_OF_ENFEEB",  # 102
    "FEATHER_FALL",  # 103
    "WIZARD_EYE",  # 104
    "FIRESHIELD",  # 105
    "COLDSHIELD",  # 106
    "MINOR_GLOBE",  # 107
    "MAJOR_GLOBE",  # 108
    "DISINTEGRATE",  # 109
    "HARNESS",  # 110
    "CHAIN_LIGHTNING",  # 111
    "MASS_INVIS",  # 112
    "RELOCATE",  # 113
    "FEAR",  # 114
    "CIRCLE_OF_LIGHT",  # 115
    "DIVINE_BOLT",  # 116
    "PRAYER",  # 117
    "ELEMENTAL_WARDING",  # 118
    "DIVINE_RAY",  # 119
    "LESSER_EXORCISM",  # 120
    "DECAY",  # 121
    "SPEAK_IN_TONGUES",  # 122
    "ENLIGHTENMENT",  # 123
    "EXORCISM",  # 124
    "SPINECHILLER",  # 125
    "WINGS_OF_HEAVEN",  # 126
    "BANISH",  # 127
    "WORD_OF_COMMAND",  # 128
    "DIVINE_ESSENCE",  # 129
    "HEAVENS_GATE",  # 130
    "DARK_PRESENCE",  # 131
    "DEMONSKIN",  # 132
    "DARK_FEAST",  # 133
    "HELL_BOLT",  # 134
    "DISEASE",  # 135
    "INSANITY",  # 136
    "DEMONIC_ASPECT",  # 137
    "HELLFIRE_BRIMSTONE",  # 138
    "STYGIAN_ERUPTION",  # 139
    "DEMONIC_MUTATION",  # 140
    "WINGS_OF_HELL",  # 141
    "SANE_MIND",  # 142
    "HELLS_GATE",  # 143
    "BARKSKIN",  # 144
    "NIGHT_VISION",  # 145
    "WRITHING_WEEDS",  # 146
    "CREATE_SPRING",  # 147
    "NOURISHMENT",  # 148
    "GAIAS_CLOAK",  # 149
    "NATURES_EMBRACE",  # 150
    "ENTANGLE",  # 151
    "INVIGORATE",  # 152
    "WANDERING_WOODS",  # 153
    "URBAN_RENEWAL",  # 154
    "SUNRAY",  # 155
    "ARMOR_OF_GAIA",  # 156
    "FIRE_DARTS",  # 157
    "MAGIC_TORCH",  # 158
    "SMOKE",  # 159
    "MIRAGE",  # 160
    "FLAME_BLADE",  # 161
    "POSITIVE_FIELD",  # 162
    "FIRESTORM",  # 163
    "MELT",  # 164
    "CIRCLE_OF_FIRE",  # 165
    "IMMOLATE",  # 166
    "SUPERNOVA",  # 167
    "CREMATE",  # 168
    "NEGATE_HEAT",  # 169
    "ACID_BURST",  # 170
    "ICE_DARTS",  # 171
    "ICE_ARMOR",  # 172
    "ICE_DAGGER",  # 173
    "FREEZING_WIND",  # 174
    "FREEZE",  # 175
    "WALL_OF_ICE",  # 176
    "ICEBALL",  # 177
    "FLOOD",  # 178
    "VAPORFORM",  # 179
    "NEGATE_COLD",  # 180
    "WATERFORM",  # 181
    "EXTINGUISH",  # 182
    "RAIN",  # 183
    "REDUCE",  # 184
    "ENLARGE",  # 185
    "IDENTIFY",  # 186
    "BONE_ARMOR",  # 187
    "SUMMON_CORPSE",  # 188
    "SHIFT_CORPSE",  # 189
    "GLORY",  # 190
    "ILLUSORY_WALL",  # 191
    "NIGHTMARE",  # 192
    "DISCORPORATE",  # 193
    "ISOLATION",  # 194
    "FAMILIARITY",  # 195
    "HYSTERIA",  # 196
    "MESMERIZE",  # 197
    "SEVERANCE",  # 198
    "SOUL_REAVER",  # 199
    "DETONATION",  # 200
    "FIRE_BREATH",  # 201
    "GAS_BREATH",  # 202
    "FROST_BREATH",  # 203
    "ACID_BREATH",  # 204
    "LIGHTNING_BREATH",  # 205
    "LESSER_ENDURANCE",  # 206
    "ENDURANCE",  # 207
    "VITALITY",  # 208
    "GREATER_VITALITY",  # 209
    "DRAGONS_HEALTH",  # 210
    "REBUKE_UNDEAD",  # 211
    "DEGENERATION",  # 212
    "SOUL_TAP",  # 213
    "NATURES_GUIDANCE",  # 214
    "MOONBEAM",  # 215
    "PHANTASM",  # 216
    "SIMULACRUM",  # 217
    "MISDIRECTION",  # 218
    "CONFUSION",  # 219
    "PHOSPHORIC_EMBERS",  # 220
    "RECALL",  # 221
    "PYRE",  # 222
    "IRON_MAIDEN",  # 223
    "FRACTURE",  # 224
    "FRACTURE_SHRAPNEL",  # 225
    "BONE_CAGE",  # 226
    "PYRE_RECOIL",  # 227
    "WORLD_TELEPORT",  # 228
    "INN_SYLL",  # 229
    "INN_TREN",  # 230
    "INN_TASS",  # 231
    "INN_BRILL",  # 232
    "INN_ASCEN",  # 233
    "SPIRIT_ARROWS",  # 234
    "PROT_FROM_GOOD",  # 235
    "ANCESTRAL_VENGEANCE",  # 236
    "CIRCLE_OF_DEATH",  # 237
    "BALEFUL_POLYMORPH",  # 238
    "SPIRIT_RAY",  # 239
    "VICIOUS_MOCKERY",  # 240
    "REMOVE_PARALYSIS",  # 241
    "CLOUD_OF_DAGGERS",  # 242
    "REVEAL_HIDDEN",  # 243
    "BLINDING_BEAUTY",  # 244
    "ACID_FOG",  # 245
    "WEB",  # 246
    "EARTH_BLESSING",  # 247
    "PROTECT_FIRE",  # 248
    "PROTECT_COLD",  # 249
    "PROTECT_ACID",  # 250
    "PROTECT_SHOCK",  # 251
    "ENHANCE_STR",  # 252
    "ENHANCE_DEX",  # 253
    "ENHANCE_CON",  # 254
    "ENHANCE_INT",  # 255
    "ENHANCE_WIS",  # 256
    "ENHANCE_CHA",  # 257
    "MONK_FIRE",  # 258
    "MONK_COLD",  # 259
    "MONK_ACID",  # 260
    "MONK_SHOCK",  # 261
    "STATUE",  # 262
    "WATER_BLAST",  # 263
    "DISPLACEMENT",  # 264
    "GREATER DISPLACEMENT",  # 265
]

OBJECT_TYPES = [
    "NOTHING",
    "LIGHT",  # 1,   # /* Item is a light source          */
    "SCROLL",  # 2,   # /* Item is a scroll                */
    "WAND",  # 3,   # /* Item is a wand                  */
    "STAFF",  # 4,   # /* Item is a staff                 */
    "WEAPON",  # 5,   # /* Item is a weapon                */
    "FIREWEAPON",  # 6,   # /* Unimplemented                   */
    "MISSILE",  # 7,   # /* Unimplemented                   */
    "TREASURE",  # 8,   # /* Item is a treasure, not gold    */
    "ARMOR",  # 9 ,   # /* Item is armor                   */
    "POTION",  # 10,  # /* Item is a potion                */
    "WORN",  # 11,  # /* Unimplemented                   */
    "OTHER",  # 12,  # /* Misc object                     */
    "TRASH",  # 13,  # /* Trash - shopkeeps won't buy     */
    "TRAP",  # 14,  # /* Unimplemented                   */
    "CONTAINER",  # 15,  # /* Item is a container             */
    "NOTE",  # 16,  # /* Item is note                    */
    "DRINKCON",  # 17,  # /* Item is a drink container       */
    "KEY",  # 18,  # /* Item is a key                   */
    "FOOD",  # 19,  # /* Item is food                    */
    "MONEY",  # 20,  # /* Item is money (gold)            */
    "PEN",  # 21,  # /* Item is a pen                   */
    "BOAT",  # 22,  # /* Item is a boat                  */
    "FOUNTAIN",  # 23,  # /* Item is a fountain              */
    "PORTAL",  # 24,  # /* Item teleports to another room  */
    "ROPE",  # 25,  # /* Item is used to bind chars      */
    "SPELLBOOK",  # 26,  # /* Spells can be scribed for mem   */
    "WALL",  # 27,  # /* Blocks passage in one direction */
    "TOUCHSTONE",  # 28,  # /* Item sets homeroom when touched */
    "BOARD",  # 29,  # Bullitin board
    "INSTRUMENT",  # 30, # /* Item is a musical instrument */
]

WEAR_FLAGS = [
    "TAKE",  # /* Item can be takes         */
    "FINGER",  # /* Can be worn on finger     */
    "NECK",  # /* Can be worn around neck   */
    "BODY",  # /* Can be worn on body       */
    "HEAD",  # /* Can be worn on head       */
    "LEGS",  # /* Can be worn on legs       */
    "FEET",  # /* Can be worn on feet       */
    "HANDS",  # /* Can be worn on hands      */
    "ARMS",  # /* Can be worn on arms       */
    "SHIELD",  # /* Can be used as a shield   */
    "ABOUT",  # /* Can be worn about body    */
    "WAIST",  # /* Can be worn around waist  */
    "WRIST",  # /* Can be worn on wrist      */
    "WIELD",  # /* Can be wielded            */
    "HOLD",  # /* Can be held               */
    "2HWIELD",  # /* Can be wielded two handed */
    "EYES",  # /* Can be worn on eyes       */
    "FACE",  # /* Can be worn upon face     */
    "EAR",  # /* Can be worn in ear        */
    "BADGE",  # /* Can be worn as badge      */
    "BELT",  # /* Can be worn on belt       */
    "HOVER",  # /*Hovers above you    */
]

WEAR_LOCATIONS = [
    "WEAR_LIGHT",  # 0
    "WEAR_FINGER_R",  # 1
    "WEAR_FINGER_L",  # 2
    "WEAR_NECK_1",  # 3
    "WEAR_NECK_2",  # 4
    "WEAR_BODY",  # 5
    "WEAR_HEAD",  # 6
    "WEAR_LEGS",  # 7
    "WEAR_FEET",  # 8
    "WEAR_HANDS",  # 9
    "WEAR_ARMS",  # 10
    "WEAR_SHIELD",  # 11
    "WEAR_ABOUT",  # 12
    "WEAR_WAIST",  # 13
    "WEAR_WRIST_R",  # 14
    "WEAR_WRIST_L",  # 15
    "WEAR_WIELD",  # 16
    "WEAR_WIELD2",  # 17
    "WEAR_HOLD",  # 18
    "WEAR_HOLD2",  # 19
    "WEAR_2HWIELD",  # 20
    "WEAR_EYES",  # 21
    "WEAR_FACE",  # 22
    "WEAR_LEAR",  # 23
    "WEAR_REAR",  # 24
    "WEAR_BADGE",  # 25
    "WEAR_OBELT",  # 26
    "WEAR_HOVER",  # 27
]

EXTRA_OBJ_FLAGS = [
    "GLOW",  # 0,      # /* Item is glowing               */
    "HUM",  # 1,      # /* Item is humming               */
    "NORENT",  # 2,      # /* Item cannot be rented         */
    "NODONATE",  # 3,      # /* Item cannot be donated        */
    "NOINVIS",  # 4,      # /* Item cannot be made invis     */
    "INVISIBLE",  # 5,      # /* Item is invisible             */
    "MAGIC",  # 6,      # /* Item is magical               */
    "NODROP",  # 7,      # /* Item can't be dropped         */
    "PERMANENT",  # 8,      # /* Item doesn't decompose        */
    "ANTI_GOOD",  # 9,      # /* Not usable by good people     */
    "ANTI_EVIL",  # 10,     # /* Not usable by evil people     */
    "ANTI_NEUTRAL",  # 11,     # /* Not usable by neutral people  */
    "ANTI_SORCERER",  # 12,     # /* Not usable by sorcerers       */
    "ANTI_CLERIC",  # 13,     # /* Not usable by clerics         */
    "ANTI_ROGUE",  # 14,     # /* Not usable by rogues          */
    "ANTI_WARRIOR",  # 15,     # /* Not usable by warriors        */
    "NOSELL",  # 16,     # /* Shopkeepers won't touch it    */
    "ANTI_PALADIN",  # 17,     # /* Not usable by paladins        */
    "ANTI_ANTI_PALADIN",  # 18,     # /* Not usable by anti-paladins   */
    "ANTI_RANGER",  # 19,     # /* Not usable by rangers         */
    "ANTI_DRUID",  # 20,     # /* Not usable by druids          */
    "ANTI_SHAMAN",  # 21,     # /* Not usable by shamans         */
    "ANTI_ASSASSIN",  # 22,     # /* Not usable by assassins       */
    "ANTI_MERCENARY",  # 23,     # /* Not usable by mercenaries     */
    "ANTI_NECROMANCER",  # 24,     # /* Not usable by necromancers    */
    "ANTI_CONJURER",  # 25,     # /* Not usable by conjurers       */
    "NOBURN",  # 26,     # /* Not destroyed by purge/fire   */
    "NOLOCATE",  # 27,     # /* Cannot be found by locate obj */
    "DECOMP",  # 28,     # /* Item is currently decomposint */
    "FLOAT",  # 29,     # /* Floats in water rooms         */
    "NOFALL",  # 30,     # /* Doesn't fall - unaffected by gravity */
    "WAS_DISARMED",  # 31,     # /* Disarmed from mob             */
]

AFFECTS = [
    "NONE",  # 0        /* No effect                       */
    "STR",  # 1        /* Apply to strength               */
    "DEX",  # 2        /* Apply to dexterity              */
    "INT",  # 3        /* Apply to constitution           */
    "WIS",  # 4        /* Apply to wisdom                 */
    "CON",  # 5        /* Apply to constitution           */
    "CHA",  # 6        /* Apply to charisma               */
    "CLASS",  # 7        /* Reserved                        */
    "LEVEL",  # 8        /* Reserved                        */
    "AGE",  # 9        /* Apply to age                    */
    "CHAR_WEIGHT",  # 10        /* Apply to weight                 */
    "CHAR_HEIGHT",  # 11        /* Apply to height                 */
    "MANA",  # 12        /* Apply to max mana               */
    "HIT",  # 13        /* Apply to max hit points         */
    "MOVE",  # 14        /* Apply to max move points        */
    "GOLD",  # 15        /* Reserved                        */
    "EXP",  # 16        /* Reserved                        */
    "AC",  # 17        /* Apply to Armor Class            */
    "HITROLL",  # 18        /* Apply to hitroll                */
    "DAMROLL",  # 19        /* Apply to damage roll            */
    "SAVING_PARA",  # 20        /* Apply to save throw: paralz     */
    "SAVING_ROD",  # 21        /* Apply to save throw: rods       */
    "SAVING_PETRI",  # 22        /* Apply to save throw: petrif     */
    "SAVING_BREATH",  # 23        /* Apply to save throw: breath     */
    "SAVING_SPELL",  # 24        /* Apply to save throw: spells     */
    "SIZE",  # 25        /* Apply to size                   */
    "HIT_REGEN",  # 26
    "FOCUS",  # 27
    "PERCEPTION",  # 28
    "HIDDENNESS",  # 29
    "COMPOSITION",  # 30
]

EFFECTS = [
    "BLIND",  # 0   /* (R) Char is blind            */
    "INVISIBLE",  # 1   /* Char is invisible            */
    "DETECT_ALIGN",  # 2   /* Char is sensitive to align   */
    "DETECT_INVIS",  # 3   /* Char can see invis chars     */
    "DETECT_MAGIC",  # 4   /* Char is sensitive to magic   */
    "SENSE_LIFE",  # 5   /* Char can sense hidden life   */
    "WATERWALK",  # 6   /* Char can walk on water       */
    "SANCTUARY",  # 7   /* Char protected by sanct.     */
    "CONFUSION",  # 8   /* Char is confused             */
    "CURSE",  # 9   /* Char is cursed               */
    "INFRAVISION",  # 10   /* Char can see in dark         */
    "POISON",  # 11   /* (R) Char is poisoned         */
    "PROTECT_EVIL",  # 12   /* Char protected from evil     */
    "PROTECT_GOOD",  # 13   /* Char protected from good     */
    "SLEEP",  # 14   /* (R) Char magically asleep    */
    "NOTRACK",  # 15   /* Char can't be tracked        */
    "TAMED",  # 16   /* Tamed!                       */
    "BERSERK",  # 17   /* Char is berserking           */
    "SNEAK",  # 18   /* Char is sneaking             */
    "STEALTH",  # 19   /* Char is using stealth        */
    "FLY",  # 20   /* Char has the ability to fly  */
    "CHARM",  # 21   /* Char is charmed              */
    "STONE_SKIN",  # 22
    "FARSEE",  # 23
    "HASTE",  # 24
    "BLUR",  # 25
    "VITALITY",  # 26
    "GLORY",  # 27
    "MAJOR_PARALYSIS",  # 28
    "FAMILIARITY",  # 29   /* Char is considered friend    */
    "MESMERIZED",  # 30   /* Super fasciated by something */
    "IMMOBILIZED",  # 31   /* Char cannot move             */
    "LIGHT",  # 32
    "UNUSED",  # 33
    "MINOR_PARALYSIS",  # 34
    "HURT_THROAT",  # 35
    "FEATHER_FALL",  # 36
    "WATERBREATH",  # 37
    "SOULSHIELD",  # 38
    "SILENCE",  # 39
    "PROT_FIRE",  # 40
    "PROT_COLD",  # 41
    "PROT_AIR",  # 42
    "PROT_EARTH",  # 43
    "FIRESHIELD",  # 44
    "COLDSHIELD",  # 45
    "MINOR_GLOBE",  # 46
    "MAJOR_GLOBE",  # 47
    "HARNESS",  # 48
    "ON_FIRE",  # 49
    "FEAR",  # 50
    "TONGUES",  # 51
    "DISEASE",  # 52
    "INSANITY",  # 53
    "ULTRAVISION",  # 54
    "NEGATE_HEAT",  # 55
    "NEGATE_COLD",  # 56
    "NEGATE_AIR",  # 57
    "NEGATE_EARTH",  # 58
    "REMOTE_AGGR",  # 59   /* Your aggro action won't remove invis/bless etc. */
    "UNUSED",  # 60
    "UNUSED",  # 61
    "UNUSED",  # 62
    "UNUSED",  # 63
    "AWARE",  # 64
    "REDUCE",  # 65
    "ENLARGE",  # 66
    "VAMP_TOUCH",  # 67
    "RAY_OF_ENFEEB",  # 68
    "ANIMATED",  # 69
    "EXPOSED",  # 70
    "SHADOWING",  # 71
    "CAMOUFLAGED",  # 72
    "SPIRIT_WOLF",  # 73
    "SPIRIT_BEAR",  # 74
    "WRATH",  # 75
    "MISDIRECTION",  # 76   /* Capable of performing misdirection */
    "MISDIRECTING",  # 77   /* Currently actually moving but misdirecting */
    "BLESS",  # 78   /* When blessed,# our barehand attacks hurt ether chars */
    "HEX",  # 79   /* The evil side of blessing,# o hurt ether chars */
    "DETECT_POISON",  # 80   /* Char is sensitive to poison */
]

DAMAGE_TYPES = [
    "HIT",
    "STING",
    "WHIP",
    "SLASH",
    "BITE",
    "BLUDGEON",
    "CRUSH",
    "POUND",
    "CLAW",
    "MAUL",
    "THRASH",
    "PIERCE",
    "BLAST",
    "PUNCH",
    "STAB",
    "FIRE",
    "COLD",
    "ACID",
    "SHOCK",
    "POISON",
    "ALIGN",
]

LIQUIDS = [
    "WATER",  # 0
    "BEER",  # 1
    "WINE",  # 2
    "ALE",  # 3
    "DARKALE",  # 4
    "WHISKY",  # 5
    "LEMONADE",  # 6
    "FIREBRT",  # 7
    "LOCALSPC",  # 8
    "SLIME",  # 9
    "MILK",  # 10
    "TEA",  # 11
    "COFFEE",  # 12
    "BLOOD",  # 13
    "SALTWATER",  # 14
    "RUM",  # 15
    "NECTAR",  # 16
    "SAKE",  # 17
    "CIDER",  # 18
    "TOMATOSOUP",  # 19
    "POTATOSOUP",  # 20
    "CHAI",  # 21
    "APPLEJUICE",  # 22
    "ORNGJUICE",  # 23
    "PNAPLJUICE",  # 24
    "GRAPEJUICE",  # 25
    "POMJUICE",  # 26
    "MELONAE",  # 27
    "COCOA",  # 28
    "ESPRESSO",  # 29
    "CAPPUCCINO",  # 30
    "MANGOLASSI",  # 31
    "ROSEWATER",  # 32
    "GREENTEA",  # 33
    "CHAMOMILE",  # 34
    "GIN",  # 35
    "BRANDY",  # 36
    "MEAD",  # 37
    "CHAMPAGNE",  # 38
    "VODKA",  # 39
    "TEQUILA",  # 40
    "ABSINTHE",  # 41
    "LIQ_TYPES",  # 42
]

# Mobile flags: used by char_data.char_specials.act
MOB_FLAGS = [
    "MOB_SPEC",  # 0          /* Mob has a callable spec-proc       */
    "MOB_SENTINEL",  # 1      /* Mob should not move                */
    "MOB_SCAVENGER",  # 2     /* Mob picks up stuff on the ground   */
    "MOB_ISNPC",  # 3         /* (R) Automatically set on all Mobs  */
    "MOB_AWARE",  # 4         /* Mob can't be backstabbed           */
    "MOB_AGGRESSIVE",  # 5    /* Mob hits players in the room       */
    "MOB_STAY_ZONE",  # 6     /* Mob shouldn't wander out of zone   */
    "MOB_WIMPY",  # 7         /* Mob flees if severely injured      */
    "MOB_AGGR_EVIL",  # 8     /* auto attack evil PC's              */
    "MOB_AGGR_GOOD",  # 9     /* auto attack good PC's              */
    "MOB_AGGR_NEUTRAL",  # 10 /* auto attack neutral PC's           */
    "MOB_MEMORY",  # 11       /* remember attackers if attacked     */
    "MOB_HELPER",  # 12       /* attack PCs fighting other NPCs     */
    "MOB_NOCHARM",  # 13      /* Mob can't be charmed               */
    "MOB_NOSUMMON",  # 14     /* Mob can't be summoned              */
    "MOB_NOSLEEP",  # 15      /* Mob can't be slept                 */
    "MOB_NOBASH",  # 16       /* Mob can't be bashed (e.g. trees)   */
    "MOB_NOBLIND",  # 17      /* Mob can't be blinded               */
    "MOB_MOUNTABLE",  # 18
    "MOB_NO_EQ_RESTRICT",  # 19
    "MOB_FAST_TRACK",  # 20
    "MOB_SLOW_TRACK",  # 21
    "MOB_CASTING",  # 22        /* mob casting            (not used)  */
    "MOB_SUMMONED_MOUNT",  # 23 /* resets CD_SUMMON_MOUNT when extracted */
    "MOB_AQUATIC",  # 24        /* Mob can't enter non-water rooms    */
    "MOB_AGGR_EVIL_RACE",  # 25
    "MOB_AGGR_GOOD_RACE",  # 26
    "MOB_NOSILENCE",  # 27
    "MOB_NOVICIOUS",  # 28
    "MOB_TEACHER",  # 29
    "MOB_ANIMATED",  # 30        /* mob is animated - die if no anim effect */
    "MOB_PEACEFUL",  # 31        /* mob can't be attacked.             */
    "MOB_NOPOISON",  # 32        /* Mob cannot be poisoned.            */
    "MOB_ILLUSORY",  # 33        /* is an illusion: does no harm, leaves no corpse */
    "MOB_PLAYER_PHANTASM",  # 34 /* illusion of player; mobs are aggro to */
    "MOB_NO_CLASS_AI",  # 35     /* Mob does not execute class AI      */
    "MOB_NOSCRIPT",  # 36        /* Mob does not execute triggers or specprocs */
    "MOB_PEACEKEEPER",  # 37     /* Attacks mobs with over 1350 align diff. Assists other PEACEKEEPERs */
    "MOB_PROTECTOR",  # 38       /* Assists players under attack, but not against PEACEKEEPER/PROTECTOR mobs */
    "MOB_PET",  # 39             /* Mob was purchased or tamed and is now a pet to a player. */
]

# /* Effect bits: used in char_data.char_specials.effects * /
EFFECTS = [
    "EFF_BLIND",  # 0         /* (R) Char is blind            */
    "EFF_INVISIBLE",  # 1     /* Char is invisible            */
    "EFF_DETECT_ALIGN",  # 2  /* Char is sensitive to align   */
    "EFF_DETECT_INVIS",  # 3  /* Char can see invis chars     */
    "EFF_DETECT_MAGIC",  # 4  /* Char is sensitive to magic   */
    "EFF_SENSE_LIFE",  # 5    /* Char can sense hidden life   */
    "EFF_WATERWALK",  # 6     /* Char can walk on water       */
    "EFF_SANCTUARY",  # 7     /* Char protected by sanct.     */
    "EFF_CONFUSION",  # 8     /* Char is confused             */
    "EFF_CURSE",  # 9         /* Char is cursed               */
    "EFF_INFRAVISION",  # 10  /* Char can see in dark         */
    "EFF_POISON",  # 11       /* (R) Char is poisoned         */
    "EFF_PROTECT_EVIL",  # 12 /* Char protected from evil     */
    "EFF_PROTECT_GOOD",  # 13 /* Char protected from good     */
    "EFF_SLEEP",  # 14        /* (R) Char magically asleep    */
    "EFF_NOTRACK",  # 15      /* Char can't be tracked        */
    "EFF_TAMED",  # 16        /* Tamed!                       */
    "EFF_BERSERK",  # 17      /* Char is berserking           */
    "EFF_SNEAK",  # 18        /* Char is sneaking             */
    "EFF_STEALTH",  # 19      /* Char is using stealth        */
    "EFF_FLY",  # 20          /* Char has the ability to fly  */
    "EFF_CHARM",  # 21        /* Char is charmed              */
    "EFF_STONE_SKIN",  # 22
    "EFF_FARSEE",  # 23
    "EFF_HASTE",  # 24
    "EFF_BLUR",  # 25
    "EFF_VITALITY",  # 26
    "EFF_GLORY",  # 27
    "EFF_MAJOR_PARALYSIS",  # 28
    "EFF_FAMILIARITY",  # 29 /* Char is considered friend    */
    "EFF_MESMERIZED",  # 30  /* Super fasciated by something */
    "EFF_IMMOBILIZED",  # 31 /* Char cannot move             */
    "EFF_LIGHT",  # 32
    "EFF_NIMBLE",  # 33
    "EFF_MINOR_PARALYSIS",  # 34
    "EFF_HURT_THROAT",  # 35
    "EFF_FEATHER_FALL",  # 36
    "EFF_WATERBREATH",  # 37
    "EFF_SOULSHIELD",  # 38
    "EFF_SILENCE",  # 39
    "EFF_PROT_FIRE",  # 40
    "EFF_PROT_COLD",  # 41
    "EFF_PROT_AIR",  # 42
    "EFF_PROT_EARTH",  # 43
    "EFF_FIRESHIELD",  # 44
    "EFF_COLDSHIELD",  # 45
    "EFF_MINOR_GLOBE",  # 46
    "EFF_MAJOR_GLOBE",  # 47
    "EFF_HARNESS",  # 48
    "EFF_ON_FIRE",  # 49
    "EFF_FEAR",  # 50
    "EFF_TONGUES",  # 51
    "EFF_DISEASE",  # 52
    "EFF_INSANITY",  # 53
    "EFF_ULTRAVISION",  # 54
    "EFF_NEGATE_HEAT",  # 55
    "EFF_NEGATE_COLD",  # 56
    "EFF_NEGATE_AIR",  # 57
    "EFF_NEGATE_EARTH",  # 58
    "EFF_REMOTE_AGGR",  # 59 /* Your aggro action won't remove invis/bless etc. */
    "EFF_FIREHANDS",  # 60   /* Make Monks do burn damage with their hands */
    "EFF_ICEHANDS",  # 61    /* Make Monks do cold damage with their hands */
    "EFF_LIGHTNINGHANDS",  # 62 /* Make Monks do shock damage with their hands */
    "EFF_ACIDHANDS",  # 63   /* Make Monks do acid damage with their hands */
    "EFF_AWARE",  # 64
    "EFF_REDUCE",  # 65
    "EFF_ENLARGE",  # 66
    "EFF_VAMP_TOUCH",  # 67
    "EFF_RAY_OF_ENFEEB",  # 68
    "EFF_ANIMATED",  # 69
    "EFF_EXPOSED",  # 70
    "EFF_SHADOWING",  # 71
    "EFF_CAMOUFLAGED",  # 72
    "EFF_SPIRIT_WOLF",  # 73
    "EFF_SPIRIT_BEAR",  # 74
    "EFF_WRATH",  # 75
    "EFF_MISDIRECTION",  # 76  /* Capable of performing misdirection */
    "EFF_MISDIRECTING",  # 77  /* Currently actually moving but misdirecting */
    "EFF_BLESS",  # 78         /* When blessed, your barehand attacks hurt ether chars */
    "EFF_HEX",  # 79           /* The evil side of blessing, to hurt ether chars */
    "EFF_DETECT_POISON",  # 80 /* Char is sensitive to poison */
    "EFF_SONG_OF_REST",  # 81
    "EFF_DISPLACEMENT",  # 82
    "EFF_GREATER_DISPLACEMENT",  # 83
]

ROOM_FLAGS = [
    "ROOM_DARK",  # 0         /* Dark                           */
    "ROOM_DEATH",  # 1        /* Death trap                     */
    "ROOM_NOMOB",  # 2        /* MOBs not allowed               */
    "ROOM_INDOORS",  # 3      /* Indoors                        */
    "ROOM_PEACEFUL",  # 4     /* Violence not allowed           */
    "ROOM_SOUNDPROOF",  # 5   /* Shouts, gossip blocked         */
    "ROOM_NOTRACK",  # 6      /* Track won't go through         */
    "ROOM_NOMAGIC",  # 7      /* Magic not allowed              */
    "ROOM_TUNNEL",  # 8       /* room for only 2 pers           */
    "ROOM_PRIVATE",  # 9      /* Can't teleport in              */
    "ROOM_GODROOM",  # 10     /* LVL_GOD+ only allowed          */
    "ROOM_HOUSE",  # 11       /* (R) Room is a house            */
    "ROOM_HOUSE_CRASH",  # 12 /* (R) House needs saving         */
    "ROOM_ATRIUM",  # 13      /* (R) The door to a house        */
    "ROOM_OLC",  # 14         /* (R) Modifyable/!compress       */
    "ROOM_BFS_MARK",  # 15    /* (R) breadth-first srch mrk     */
    "ROOM_NOWELL",  # 16      /* No spell portals like moonwell */
    "ROOM_NORECALL",  # 17    /* No recalling                   */
    "ROOM_UNDERDARK",  # 18   /*                   (not used)   */
    "ROOM_NOSUMMON",  # 19    /* Can't summon to or from. Can't banish here. */
    "ROOM_NOSHIFT",  # 20     /* no plane shift    (not used)   */
    "ROOM_GUILDHALL",  # 21   /*                   (not used)   */
    "ROOM_NOSCAN",  # 22      /* Unable to scan to/from rooms   */
    "ROOM_ALT_EXIT",  # 23    /* Room's exits are altered       */
    "ROOM_MAP",  # 24         /* Room on surface map (unused)   */
    "ROOM_ALWAYSLIT",  # 25   /* Makes the room lit             */
    "ROOM_ARENA",  # 26       /* (safe) PK allowed in room      */
    "ROOM_OBSERVATORY",  # 27 /* see into adjacent ARENA rooms  */
]

DIRECTIONS = ["NORTH", "EAST", "SOUTH", "WEST", "UP", "DOWN"]


EXIT_FLAGS = [
    "",
    "EX_ISDOOR",  # 0    /* Exit is a door             */
    "EX_CLOSED",  # 1    /* The door is closed         */
    "EX_LOCKED",  # 2    /* The door is locked         */
    "EX_PICKPROOF",  # 3 /* Lock can't be picked       */
    "EX_HIDDEN",  # 4    /* exit is hidden             */
    "EX_DESCRIPT",  # 5  /* Just an extra description  */
]

# Shop Related
SHOP_FLAGS = [
    "WILL_START_FIGHT",  # 1
    "WILL_BANK_MONEY",  # 2
]

SHOP_TRADES_WITH = [
    "TRADE_NOGOOD",  # 1
    "TRADE_NOEVIL",  # 2
    "TRADE_NONEUTRAL",  # 3
    "TRADE_NOMAGIC_USER",  # 4
    "TRADE_NOCLERIC",  # 5
    "TRADE_NOTHIEF",  # 6
    "TRADE_NOWARRIOR",  # 7
]

SEXES = [
    "NEUTRAL",  # 0
    "MALE",  # 1
    "FEMALE",  # 2
    "NONBINARY",  # 3
]

CLASSES = [
    "SORCERER",  # 0
    "CLERIC",  # 1
    "THIEF",  # 2
    "WARRIOR",  # 3
    "PALADIN",  # 4
    "ANTI_PALADIN",  # 5
    "RANGER",  # 6
    "DRUID",  # 7
    "SHAMAN",  # 8
    "ASSASSIN",  # 9
    "MERCENARY",  # 10
    "NECROMANCER",  # 11
    "CONJURER",  # 12
    "MONK",  # 13
    "BERSERKER",  # 14
    "PRIEST",  # 15
    "DIABOLIST",  # 16
    "MYSTIC",  # 17
    "ROGUE",  # 18
    "BARD",  # 19
    "PYROMANCER",  # 20
    "CRYOMANCER",  # 21
    "ILLUSIONIST",  # 22
    "HUNTER",  # 23
    "LAYMAN",  # 24
]

RACES = [
    "HUMAN",  # 0
    "ELF",  # 1
    "GNOME",  # 2
    "DWARF",  # 3
    "TROLL",  # 4
    "DROW",  # 5
    "DUERGAR",  # 6
    "OGRE",  # 7
    "ORC",  # 8
    "HALF_ELF",  # 9
    "BARBARIAN",  # 10
    "HALFLING",  # 11
    "PLANT",  # 12
    "HUMANOID",  # 13
    "ANIMAL",  # 14
    "DRAGON_GENERAL",  # 15
    "GIANT",  # 16
    "OTHER",  # 17
    "GOBLIN",  # 18
    "DEMON",  # 19
    "BROWNIE",  # 20
    "DRAGON_FIRE",  # 21
    "DRAGON_FROST",  # 22
    "DRAGON_ACID",  # 23
    "DRAGON_LIGHTNING",  # 24
    "DRAGON_GAS",  # 25
    "DRAGONBORN_FIRE",  # 26
    "DRAGONBORN_FROST",  # 27
    "DRAGONBORN_ACID",  # 28
    "DRAGONBORN_LIGHTNING",  # 29
    "DRAGONBORN_GAS",  # 30
    "SVERFNEBLIN",  # 31
    "FAERIE_SEELIE",  # 32
    "FAERIE_UNSEELIE",  # 33
    "NYMPH",  # 34
    "ARBOREAN",  # 35
]

LIFEFORCES = [
    "LIFE_LIFE",  # 0 /* normal folks */
    "LIFE_UNDEAD",  # 1
    "LIFE_MAGIC",  # 2     /* golems */
    "LIFE_CELESTIAL",  # 3 /* angels */
    "LIFE_DEMONIC",  # 4
    "LIFE_ELEMENTAL",  # 5
]

COMPOSITIONS = [
    "COMP_FLESH",  # 0
    "COMP_EARTH",  # 1
    "COMP_AIR",  # 2
    "COMP_FIRE",  # 3
    "COMP_WATER",  # 4
    "COMP_ICE",  # 5
    "COMP_MIST",  # 6
    "COMP_ETHER",  # 7 /* Having no physical incorporation */
    "COMP_METAL",  # 8
    "COMP_STONE",  # 9 /* Like earth, but tougher */
    "COMP_BONE",  # 10 /* Like flesh, but... */
    "COMP_LAVA",  # 11
    "COMP_PLANT",  # 12
    "NUM_COMPOSITIONS",  # 13 /* keep updated */
]

PLAYER_FLAGS = [
    "PLR_KILLER",  # 0     /* a player-killer                           */
    "PLR_THIEF",  # 1      /* a player-thief                            */
    "PLR_FROZEN",  # 2     /* is frozen                                 */
    "PLR_DONTSET",  # 3    /* Don't EVER set (ISNPC bit)                */
    "PLR_WRITING",  # 4    /* writing (board/mail/olc)                  */
    "PLR_MAILING",  # 5    /* is writing mail                           */
    "PLR_AUTOSAVE",  # 6   /* needs to be autosaved                     */
    "PLR_SITEOK",  # 7     /* has been site-cleared                     */
    "PLR_NOSHOUT",  # 8    /* not allowed to shout/goss                 */
    "PLR_NOTITLE",  # 9    /* not allowed to set title       (not used) */
    "PLR_DELETED",  # 10   /* deleted - space reusable       (not used) */
    "PLR_LOADROOM",  # 11  /* uses nonstandard loadroom      (not used) */
    "PLR_NOWIZLIST",  # 12 /* shouldn't be on wizlist        (not used) */
    "PLR_NODELETE",  # 13  /* shouldn't be deleted           (may be used outside the server) */
    "PLR_INVSTART",  # 14  /* should enter game wizinvis     (not used) */
    "PLR_CRYO",  # 15      /* is cryo-saved (purge prog)     (not used) */
    "PLR_MEDITATE",  # 16  /* meditating - improves spell memorization  */
    "PLR_CASTING",  # 17   /* currently casting a spell      (not used) */
    "PLR_BOUND",  # 18     /* tied up                        (not used) */
    "PLR_SCRIBE",  # 19    /* scribing                       (not used) */
    "PLR_TEACHING",  # 20  /* teaching a skill/spell         (not used) */
    "PLR_NAPPROVE",  # 21  /* name not approved yet                     */
    "PLR_NEWNAME",  # 22   /* needs to choose a new name                */
    "PLR_REMOVING",  # 23  /* player is being removed and doesn't need emergency save */
    "PLR_SAVING",  # 24    /* player is being saved to file and effect changes are not relevant */
    "PLR_GOTSTARS",  # 25  /* player has achieved ** already            */
    "NUM_PLR_FLAGS",  # 26
]

PREF_FLAGS = [
    "PRF_BRIEF",  # 0       /* Room descs won't normally be shown */
    "PRF_COMPACT",  # 1     /* No extra CRLF pair before prompts  */
    "PRF_DEAF",  # 2        /* Can't hear shouts                  */
    "PRF_NOTELL",  # 3      /* Can't receive tells                */
    "PRF_OLCCOMM",  # 4     /* Can hear communication in OLC      */
    "PRF_LINENUMS",  # 5    /* Autodisplay linenums in stringedit */
    "PRF_AUTOLOOT",  # 6    /* Auto loot corpses when you kill    */
    "PRF_AUTOEXIT",  # 7    /* Display exits in a room            */
    "PRF_NOHASSLE",  # 8    /* Aggr mobs won't attack             */
    "PRF_QUEST",  # 9       /* On quest                           */
    "PRF_SUMMONABLE",  # 10 /* Can be summoned                    */
    "PRF_NOREPEAT",  # 11   /* No repetition of comm commands     */
    "PRF_HOLYLIGHT",  # 12  /* Can see in dark                    */
    "PRF_COLOR_1",  # 13    /* Color (low bit)                    */
    "PRF_COLOR_2",  # 14    /* Color (high bit)                   */
    "PRF_NOWIZ",  # 15      /* Can't hear wizline                 */
    "PRF_LOG1",  # 16       /* On-line System Log (low bit)       */
    "PRF_LOG2",  # 17       /* On-line System Log (high bit)      */
    "PRF_AFK",  # 18        /* away from keyboard                 */
    "PRF_NOGOSS",  # 19     /* Can't hear gossip channel          */
    "PRF_NOHINTS",  # 20    /* No hints when mistyping commands   */
    "PRF_ROOMFLAGS",  # 21  /* Can see room flags (ROOM_x)        */
    "PRF_NOPETI",  # 22     /* Can't hear petitions               */
    "PRF_AUTOSPLIT",  # 23  /* Auto split coins from corpses      */
    "PRF_NOCLANCOMM",  # 24 /* Can't hear clan communication      */
    "PRF_ANON",  # 25       /* Anon flag                          */
    "PRF_SHOWVNUMS",  # 26  /* Show Virtual Numbers               */
    "PRF_NICEAREA",  # 27
    "PRF_VICIOUS",  # 28
    "PRF_PASSIVE",  # 29 /* char will not engage upon being cast on */
    "PRF_ROOMVIS",  # 30
    "PRF_NOFOLLOW",  # 31  /* Cannot follow / well to this player*/
    "PRF_AUTOTREAS",  # 32 /* Automatically loots treasure from corpses */
    "PRF_EXPAND_OBJS",  # 33
    "PRF_EXPAND_MOBS",  # 34
    "PRF_SACRIFICIAL",  # 35 /* Sacrificial spells autotarget self */
    "PRF_PETASSIST",  # 36   /* Should your pet assist you as you fight */
    "NUM_PRF_FLAGS",  # 37
]

PRIV_FLAGS = [
    "PRV_CLAN_ADMIN",  # 0  /* clan administrator */
    "PRV_TITLE",  # 1       /* can change own title */
    "PRV_ANON_TOGGLE",  # 2 /* can toggle anon */
    "PRV_AUTO_GAIN",  # 3   /* don't need to level gain */
]

QUIT_REASONS = [
    "QUIT_UNDEF",
    "QUIT_RENT",
    "QUIT_CRYO",
    "QUIT_TIMEOUT",
    "QUIT_HOTBOOT",
    "QUIT_QUITMORT",
    "QUIT_QUITIMM",
    "QUIT_CAMP",
    "QUIT_WRENT",
    "QUIT_PURGE",
    "QUIT_AUTOSAVE",
    "NUM_QUITTYPES",
]
