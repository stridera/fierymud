"""
Flags for the MUD.  These are all items that can be set via a bitflag.
"""

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
    "NIMBLE",  # 266
    "CLARITY",  # 267
]

SKILLS = {
    1: "SPELL_ARMOR",
    2: "SPELL_TELEPORT",
    3: "SPELL_BLESS",
    4: "SPELL_BLINDNESS",
    5: "SPELL_BURNING_HANDS",
    6: "SPELL_CALL_LIGHTNING",
    7: "SPELL_CHARM",
    8: "SPELL_CHILL_TOUCH",
    9: "SPELL_CLONE",
    10: "SPELL_COLOR_SPRAY",
    11: "SPELL_CONTROL_WEATHER",
    12: "SPELL_CREATE_FOOD",
    13: "SPELL_CREATE_WATER",
    14: "SPELL_CURE_BLIND",
    15: "SPELL_CURE_CRITIC",
    16: "SPELL_CURE_LIGHT",
    17: "SPELL_CURSE",
    18: "SPELL_DETECT_ALIGN",
    19: "SPELL_DETECT_INVIS",
    20: "SPELL_DETECT_MAGIC",
    21: "SPELL_DETECT_POISON",
    22: "SPELL_DISPEL_EVIL",
    23: "SPELL_EARTHQUAKE",
    24: "SPELL_ENCHANT_WEAPON",
    25: "SPELL_ENERGY_DRAIN",
    26: "SPELL_FIREBALL",
    27: "SPELL_HARM",
    28: "SPELL_HEAL",
    29: "SPELL_INVISIBLE",
    30: "SPELL_LIGHTNING_BOLT",
    31: "SPELL_LOCATE_OBJECT",
    32: "SPELL_MAGIC_MISSILE",
    33: "SPELL_POISON",
    34: "SPELL_PROT_FROM_EVIL",
    35: "SPELL_REMOVE_CURSE",
    36: "SPELL_SANCTUARY",
    37: "SPELL_SHOCKING_GRASP",
    38: "SPELL_SLEEP",
    39: "SPELL_ENHANCE_ABILITY",
    40: "SPELL_SUMMON",
    41: "SPELL_VENTRILOQUATE",
    42: "SPELL_WORD_OF_RECALL",
    43: "SPELL_REMOVE_POISON",
    44: "SPELL_SENSE_LIFE",
    45: "SPELL_ANIMATE_DEAD",
    46: "SPELL_DISPEL_GOOD",
    47: "SPELL_GROUP_ARMOR",
    48: "SPELL_GROUP_HEAL",
    49: "SPELL_GROUP_RECALL",
    50: "SPELL_INFRAVISION",
    51: "SPELL_WATERWALK",
    52: "SPELL_STONE_SKIN",
    53: "SPELL_FULL_HEAL",
    54: "SPELL_FULL_HARM",
    55: "SPELL_WALL_OF_FOG",
    56: "SPELL_WALL_OF_STONE",
    57: "SPELL_FLY",
    58: "SPELL_SUMMON_DRACOLICH",
    59: "SPELL_SUMMON_ELEMENTAL",
    60: "SPELL_SUMMON_DEMON",
    61: "SPELL_SUMMON_GREATER_DEMON",
    62: "SPELL_DIMENSION_DOOR",
    63: "SPELL_CREEPING_DOOM",
    64: "SPELL_DOOM",
    65: "SPELL_METEORSWARM",
    66: "SPELL_BIGBYS_CLENCHED_FIST",
    67: "SPELL_FARSEE",
    68: "SPELL_HASTE",
    69: "SPELL_BLUR",
    70: "SPELL_GREATER_ENDURANCE",
    71: "SPELL_MOONWELL",
    72: "SPELL_INN_CHAZ",
    73: "SPELL_DARKNESS",
    74: "SPELL_ILLUMINATION",
    75: "SPELL_COMPREHEND_LANG",
    76: "SPELL_CONE_OF_COLD",
    77: "SPELL_ICE_STORM",
    78: "SPELL_ICE_SHARDS",
    79: "SPELL_MAJOR_PARALYSIS",
    80: "SPELL_VAMPIRIC_BREATH",
    81: "SPELL_RESURRECT",
    82: "SPELL_INCENDIARY_NEBULA",
    83: "SPELL_MINOR_PARALYSIS",
    84: "SPELL_CAUSE_LIGHT",
    85: "SPELL_CAUSE_SERIOUS",
    86: "SPELL_CAUSE_CRITIC",
    87: "SPELL_PRESERVE",
    88: "SPELL_CURE_SERIOUS",
    89: "SPELL_VIGORIZE_LIGHT",
    90: "SPELL_VIGORIZE_SERIOUS",
    91: "SPELL_VIGORIZE_CRITIC",
    92: "SPELL_SOULSHIELD",
    93: "SPELL_DESTROY_UNDEAD",
    94: "SPELL_SILENCE",
    95: "SPELL_FLAMESTRIKE",
    96: "SPELL_UNHOLY_WORD",
    97: "SPELL_HOLY_WORD",
    98: "SPELL_PLANE_SHIFT",
    99: "SPELL_DISPEL_MAGIC",
    100: "SPELL_MINOR_CREATION",
    101: "SPELL_CONCEALMENT",
    102: "SPELL_RAY_OF_ENFEEB",
    103: "SPELL_FEATHER_FALL",
    104: "SPELL_WIZARD_EYE",
    105: "SPELL_FIRESHIELD",
    106: "SPELL_COLDSHIELD",
    107: "SPELL_MINOR_GLOBE",
    108: "SPELL_MAJOR_GLOBE",
    109: "SPELL_DISINTEGRATE",
    110: "SPELL_HARNESS",
    111: "SPELL_CHAIN_LIGHTNING",
    112: "SPELL_MASS_INVIS",
    113: "SPELL_RELOCATE",
    114: "SPELL_FEAR",
    115: "SPELL_CIRCLE_OF_LIGHT",
    116: "SPELL_DIVINE_BOLT",
    117: "SPELL_PRAYER",
    118: "SPELL_ELEMENTAL_WARDING",
    119: "SPELL_DIVINE_RAY",
    120: "SPELL_LESSER_EXORCISM",
    121: "SPELL_DECAY",
    122: "SPELL_SPEAK_IN_TONGUES",
    123: "SPELL_ENLIGHTENMENT",
    124: "SPELL_EXORCISM",
    125: "SPELL_SPINECHILLER",
    126: "SPELL_WINGS_OF_HEAVEN",
    127: "SPELL_BANISH",
    128: "SPELL_WORD_OF_COMMAND",
    129: "SPELL_DIVINE_ESSENCE",
    130: "SPELL_HEAVENS_GATE",
    131: "SPELL_DARK_PRESENCE",
    132: "SPELL_DEMONSKIN",
    133: "SPELL_DARK_FEAST",
    134: "SPELL_HELL_BOLT",
    135: "SPELL_DISEASE",
    136: "SPELL_INSANITY",
    137: "SPELL_DEMONIC_ASPECT",
    138: "SPELL_HELLFIRE_BRIMSTONE",
    139: "SPELL_STYGIAN_ERUPTION",
    140: "SPELL_DEMONIC_MUTATION",
    141: "SPELL_WINGS_OF_HELL",
    142: "SPELL_SANE_MIND",
    143: "SPELL_HELLS_GATE",
    144: "SPELL_BARKSKIN",
    145: "SPELL_NIGHT_VISION",
    146: "SPELL_WRITHING_WEEDS",
    147: "SPELL_CREATE_SPRING",
    148: "SPELL_NOURISHMENT",
    149: "SPELL_GAIAS_CLOAK",
    150: "SPELL_NATURES_EMBRACE",
    151: "SPELL_ENTANGLE",
    152: "SPELL_INVIGORATE",
    153: "SPELL_WANDERING_WOODS",
    154: "SPELL_URBAN_RENEWAL",
    155: "SPELL_SUNRAY",
    156: "SPELL_ARMOR_OF_GAIA",
    157: "SPELL_FIRE_DARTS",
    158: "SPELL_MAGIC_TORCH",
    159: "SPELL_SMOKE",
    160: "SPELL_MIRAGE",
    161: "SPELL_FLAME_BLADE",
    162: "SPELL_POSITIVE_FIELD",
    163: "SPELL_FIRESTORM",
    164: "SPELL_MELT",
    165: "SPELL_CIRCLE_OF_FIRE",
    166: "SPELL_IMMOLATE",
    167: "SPELL_SUPERNOVA",
    168: "SPELL_CREMATE",
    169: "SPELL_NEGATE_HEAT",
    170: "SPELL_ACID_BURST",
    171: "SPELL_ICE_DARTS",
    172: "SPELL_ICE_ARMOR",
    173: "SPELL_ICE_DAGGER",
    174: "SPELL_FREEZING_WIND",
    175: "SPELL_FREEZE",
    176: "SPELL_WALL_OF_ICE",
    177: "SPELL_ICEBALL",
    178: "SPELL_FLOOD",
    179: "SPELL_VAPORFORM",
    180: "SPELL_NEGATE_COLD",
    181: "SPELL_WATERFORM",
    182: "SPELL_EXTINGUISH",
    183: "SPELL_RAIN",
    184: "SPELL_REDUCE",
    185: "SPELL_ENLARGE",
    186: "SPELL_IDENTIFY",
    187: "SPELL_BONE_ARMOR",
    188: "SPELL_SUMMON_CORPSE",
    189: "SPELL_SHIFT_CORPSE",
    190: "SPELL_GLORY",
    191: "SPELL_ILLUSORY_WALL",
    192: "SPELL_NIGHTMARE",
    193: "SPELL_DISCORPORATE",
    194: "SPELL_ISOLATION",
    195: "SPELL_FAMILIARITY",
    196: "SPELL_HYSTERIA",
    197: "SPELL_MESMERIZE",
    198: "SPELL_SEVERANCE",
    199: "SPELL_SOUL_REAVER",
    200: "SPELL_DETONATION",
    201: "SPELL_FIRE_BREATH",
    202: "SPELL_GAS_BREATH",
    203: "SPELL_FROST_BREATH",
    204: "SPELL_ACID_BREATH",
    205: "SPELL_LIGHTNING_BREATH",
    206: "SPELL_LESSER_ENDURANCE",
    207: "SPELL_ENDURANCE",
    208: "SPELL_VITALITY",
    209: "SPELL_GREATER_VITALITY",
    210: "SPELL_DRAGONS_HEALTH",
    211: "SPELL_REBUKE_UNDEAD",
    212: "SPELL_DEGENERATION",
    213: "SPELL_SOUL_TAP",
    214: "SPELL_NATURES_GUIDANCE",
    215: "SPELL_MOONBEAM",
    216: "SPELL_PHANTASM",
    217: "SPELL_SIMULACRUM",
    218: "SPELL_MISDIRECTION",
    219: "SPELL_CONFUSION",
    220: "SPELL_PHOSPHORIC_EMBERS",
    221: "SPELL_RECALL",
    222: "SPELL_PYRE",
    223: "SPELL_IRON_MAIDEN",
    224: "SPELL_FRACTURE",
    225: "SPELL_FRACTURE_SHRAPNEL",
    226: "SPELL_BONE_CAGE",
    227: "SPELL_PYRE_RECOIL",
    228: "SPELL_WORLD_TELEPORT",
    229: "SPELL_INN_SYLL",
    230: "SPELL_INN_TREN",
    231: "SPELL_INN_TASS",
    232: "SPELL_INN_BRILL",
    233: "SPELL_INN_ASCEN",
    234: "SPELL_SPIRIT_ARROWS",
    235: "SPELL_PROT_FROM_GOOD",
    236: "SPELL_ANCESTRAL_VENGEANCE",
    237: "SPELL_CIRCLE_OF_DEATH",
    238: "SPELL_BALEFUL_POLYMORPH",
    239: "SPELL_SPIRIT_RAY",
    240: "SPELL_VICIOUS_MOCKERY",
    241: "SPELL_REMOVE_PARALYSIS",
    242: "SPELL_CLOUD_OF_DAGGERS",
    243: "SPELL_REVEAL_HIDDEN",
    244: "SPELL_BLINDING_BEAUTY",
    245: "SPELL_ACID_FOG",
    246: "SPELL_WEB",
    247: "SPELL_EARTH_BLESSING",
    248: "SPELL_PROTECT_FIRE",
    249: "SPELL_PROTECT_COLD",
    250: "SPELL_PROTECT_ACID",
    251: "SPELL_PROTECT_SHOCK",
    252: "SPELL_ENHANCE_STR",
    253: "SPELL_ENHANCE_DEX",
    254: "SPELL_ENHANCE_CON",
    255: "SPELL_ENHANCE_INT",
    256: "SPELL_ENHANCE_WIS",
    257: "SPELL_ENHANCE_CHA",
    258: "SPELL_FIRES_OF_SAINT_AUGUSTINE",
    259: "SPELL_BLIZZARDS_OF_SAINT_AUGUSTINE",
    260: "SPELL_TREMORS_OF_SAINT_AUGUSTINE",
    261: "SPELL_TEMPEST_OF_SAINT_AUGUSTINE",
    262: "SPELL_STATUE",
    263: "SPELL_WATER_BLAST",
    264: "SPELL_DISPLACEMENT",
    265: "SPELL_GREATER_DISPLACEMENT",
    266: "SPELL_NIMBLE",
    267: "SPELL_CLARITY",
    # PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS
    401: "SKILL_BACKSTAB",
    402: "SKILL_BASH",
    403: "SKILL_HIDE",
    404: "SKILL_KICK",
    405: "SKILL_PICK_LOCK",
    406: "SKILL_PUNCH",
    407: "SKILL_RESCUE",
    408: "SKILL_SNEAK",
    409: "SKILL_STEAL",
    410: "SKILL_TRACK",
    411: "SKILL_DUAL_WIELD",
    412: "SKILL_DOUBLE_ATTACK",
    413: "SKILL_BERSERK",
    414: "SKILL_SPRINGLEAP",
    415: "SKILL_MOUNT",
    416: "SKILL_RIDING",
    417: "SKILL_TAME",
    418: "SKILL_THROATCUT",
    419: "SKILL_DOORBASH",
    420: "SKILL_PARRY",
    421: "SKILL_DODGE",
    422: "SKILL_RIPOSTE",
    423: "SKILL_MEDITATE",
    424: "SKILL_QUICK_CHANT",
    425: "SKILL_2BACK",
    426: "SKILL_CIRCLE",
    427: "SKILL_BODYSLAM",
    428: "SKILL_BIND",
    429: "SKILL_SHAPECHANGE",
    430: "SKILL_SWITCH",
    431: "SKILL_DISARM",
    432: "SKILL_DISARM_FUMBLING_WEAP",
    433: "SKILL_DISARM_DROPPED_WEAP",
    434: "SKILL_GUARD",
    435: "SKILL_BREATHE_LIGHTNING",
    436: "SKILL_SWEEP",
    437: "SKILL_ROAR",
    438: "SKILL_DOUSE",
    439: "SKILL_AWARE",
    440: "SKILL_INSTANT_KILL",
    441: "SKILL_HITALL",
    442: "SKILL_HUNT",
    443: "SKILL_BANDAGE",
    444: "SKILL_FIRST_AID",
    445: "SKILL_VAMP_TOUCH",
    446: "SKILL_CHANT",
    447: "SKILL_SCRIBE",
    448: "SKILL_SAFEFALL",
    449: "SKILL_BAREHAND",
    450: "SKILL_SUMMON_MOUNT",
    451: "SKILL_KNOW_SPELL",
    452: "SKILL_SPHERE_GENERIC",
    453: "SKILL_SPHERE_FIRE",
    454: "SKILL_SPHERE_WATER",
    455: "SKILL_SPHERE_EARTH",
    456: "SKILL_SPHERE_AIR",
    457: "SKILL_SPHERE_HEALING",
    458: "SKILL_SPHERE_PROT",
    459: "SKILL_SPHERE_ENCHANT",
    460: "SKILL_SPHERE_SUMMON",
    461: "SKILL_SPHERE_DEATH",
    462: "SKILL_SPHERE_DIVIN",
    463: "SKILL_BLUDGEONING",
    464: "SKILL_PIERCING",
    465: "SKILL_SLASHING",
    466: "SKILL_2H_BLUDGEONING",
    467: "SKILL_2H_PIERCING",
    468: "SKILL_2H_SLASHING",
    469: "SKILL_MISSILE",
    470: "SPELL_ON_FIRE",
    471: "SKILL_LAY_HANDS",
    472: "SKILL_EYE_GOUGE",
    473: "SKILL_RETREAT",
    474: "SKILL_GROUP_RETREAT",
    475: "SKILL_CORNER",
    476: "SKILL_STEALTH",
    477: "SKILL_SHADOW",
    478: "SKILL_CONCEAL",
    479: "SKILL_PECK",
    480: "SKILL_CLAW",
    481: "SKILL_ELECTRIFY",
    482: "SKILL_TANTRUM",
    483: "SKILL_GROUND_SHAKER",
    484: "SKILL_BATTLE_HOWL",
    485: "SKILL_MAUL",
    486: "SKILL_BREATHE_FIRE",
    487: "SKILL_BREATHE_FROST",
    488: "SKILL_BREATHE_ACID",
    489: "SKILL_BREATHE_GAS",
    490: "SKILL_PERFORM",
    491: "SKILL_CARTWHEEL",
    492: "SKILL_LURE",
    493: "SKILL_SNEAK_ATTACK",
    494: "SKILL_REND",
    495: "SKILL_ROUNDHOUSE",
    # Bardic songs start at 551 and go to 600
    551: "SONG_INSPIRATION",
    552: "SONG_TERROR",
    553: "SONG_ENRAPTURE",
    554: "SONG_HEARTHSONG",
    555: "SONG_CROWN_OF_MADNESS",
    556: "SONG_SONG_OF_REST",
    557: "SONG_BALLAD_OF_TEARS",
    558: "SONG_HEROIC_JOURNEY",
    559: "SONG_FREEDOM_SONG",
    560: "SONG_JOYFUL_NOISE",
    # Monk chants go from 601 to 650
    601: "CHANT_REGENERATION",
    602: "CHANT_BATTLE_HYMN",
    603: "CHANT_WAR_CRY",
    604: "CHANT_PEACE",
    605: "CHANT_SHADOWS_SORROW_SONG",
    606: "CHANT_IVORY_SYMPHONY",
    607: "CHANT_ARIA_OF_DISSONANCE",
    608: "CHANT_SONATA_OF_MALAISE",
    609: "CHANT_APOCALYPTIC_ANTHEM",
    610: "CHANT_SEED_OF_DESTRUCTION",
    611: "CHANT_SPIRIT_WOLF",
    612: "CHANT_SPIRIT_BEAR",
    613: "CHANT_INTERMINABLE_WRATH",
    614: "CHANT_HYMN_OF_SAINT_AUGUSTINE",
    615: "CHANT_FIRES_OF_SAINT_AUGUSTINE",
    616: "CHANT_BLIZZARDS_OF_SAINT_AUGUSTINE",
    617: "CHANT_TREMORS_OF_SAINT_AUGUSTINE",
    618: "CHANT_TEMPEST_OF_SAINT_AUGUSTINE",
}


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


OBJECT_FLAGS = [
    "GLOW",  # 0               /* Item is glowing               */
    "HUM",  # 1                /* Item is humming               */
    "NORENT",  # 2             /* Item cannot be rented         */
    "ANTI_BERSERKER",  # 3     /* Not usable by berserkers      */
    "NOINVIS",  # 4            /* Item cannot be made invis     */
    "INVISIBLE",  # 5          /* Item is invisible             */
    "MAGIC",  # 6              /* Item is magical               */
    "NODROP",  # 7             /* Item can't be dropped         */
    "PERMANENT",  # 8          /* Item doesn't decompose        */
    "ANTI_GOOD",  # 9          /* Not usable by good people     */
    "ANTI_EVIL",  # 10         /* Not usable by evil people     */
    "ANTI_NEUTRAL",  # 11      /* Not usable by neutral people  */
    "ANTI_SORCERER",  # 12     /* Not usable by sorcerers       */
    "ANTI_CLERIC",  # 13       /* Not usable by clerics         */
    "ANTI_ROGUE",  # 14        /* Not usable by rogues          */
    "ANTI_WARRIOR",  # 15      /* Not usable by warriors        */
    "NOSELL",  # 16            /* Shopkeepers won't touch it    */
    "ANTI_PALADIN",  # 17      /* Not usable by paladins        */
    "ANTI_ANTI_PALADIN",  # 18 /* Not usable by anti-paladins   */
    "ANTI_RANGER",  # 19       /* Not usable by rangers         */
    "ANTI_DRUID",  # 20        /* Not usable by druids          */
    "ANTI_SHAMAN",  # 21       /* Not usable by shamans         */
    "ANTI_ASSASSIN",  # 22     /* Not usable by assassins       */
    "ANTI_MERCENARY",  # 23    /* Not usable by mercenaries     */
    "ANTI_NECROMANCER",  # 24  /* Not usable by necromancers    */
    "ANTI_CONJURER",  # 25     /* Not usable by conjurers       */
    "NOBURN",  # 26            /* Not destroyed by purge/fire   */
    "NOLOCATE",  # 27          /* Cannot be found by locate obj */
    "DECOMP",  # 28            /* Item is currently decomposing */
    "FLOAT",  # 29             /* Floats in water rooms         */
    "NOFALL",  # 30            /* Doesn't fall - unaffected by gravity */
    "WAS_DISARMED",  # 31      /* Disarmed from mob             */
    "ANTI_MONK",  # 32         /* Not usable by monks           */
    "ANTI_BARD",  # 33
    "ELVEN",  # 34   /* Item usable by Elves          */
    "DWARVEN",  # 35 /* Item usable by Dwarves        */
    "ANTI_THIEF",  # 36
    "ANTI_PYROMANCER",  # 37
    "ANTI_CRYOMANCER",  # 38
    "ANTI_ILLUSIONIST",  # 39
    "ANTI_PRIEST",  # 40
    "ANTI_DIABOLIST",  # 41
    "ANTI_TINY",  # 42
    "ANTI_SMALL",  # 43
    "ANTI_MEDIUM",  # 44
    "ANTI_LARGE",  # 45
    "ANTI_HUGE",  # 46
    "ANTI_GIANT",  # 47
    "ANTI_GARGANTUAN",  # 48
    "ANTI_COLOSSAL",  # 49
    "ANTI_TITANIC",  # 50
    "ANTI_MOUNTAINOUS",  # 51
    "ANTI_ARBOREAN",  # 52 /* Not usable by Arboreans */
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
    "SONG_OF_REST",  # 81
    "DISPLACEMENT",  # 82
    "GREATER_DISPLACEMENT",  # 83
    "FIRE_WEAPON",  # 84
    "ICE_WEAPON",  # 85
    "POISON_WEAPON",  # 86
    "ACID_WEAPON",  # 87
    "SHOCK_WEAPON",  # 88
    "RADIANT_WEAPON",  # 89
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
]

# Mobile flags: used by char_data.char_specials.act
MOB_FLAGS = [
    "SPEC",  # 0          /* Mob has a callable spec-proc       */
    "SENTINEL",  # 1      /* Mob should not move                */
    "SCAVENGER",  # 2     /* Mob picks up stuff on the ground   */
    "ISNPC",  # 3         /* (R) Automatically set on all Mobs  */
    "AWARE",  # 4         /* Mob can't be backstabbed           */
    "AGGRESSIVE",  # 5    /* Mob hits players in the room       */
    "STAY_ZONE",  # 6     /* Mob shouldn't wander out of zone   */
    "WIMPY",  # 7         /* Mob flees if severely injured      */
    "AGGR_EVIL",  # 8     /* auto attack evil PC's              */
    "AGGR_GOOD",  # 9     /* auto attack good PC's              */
    "AGGR_NEUTRAL",  # 10 /* auto attack neutral PC's           */
    "MEMORY",  # 11       /* remember attackers if attacked     */
    "HELPER",  # 12       /* attack PCs fighting other NPCs     */
    "NOCHARM",  # 13      /* Mob can't be charmed               */
    "NOSUMMON",  # 14     /* Mob can't be summoned              */
    "NOSLEEP",  # 15      /* Mob can't be slept                 */
    "NOBASH",  # 16       /* Mob can't be bashed (e.g. trees)   */
    "NOBLIND",  # 17      /* Mob can't be blinded               */
    "MOUNTABLE",  # 18
    "NO_EQ_RESTRICT",  # 19
    "FAST_TRACK",  # 20
    "SLOW_TRACK",  # 21
    "CASTING",  # 22        /* mob casting            (not used)  */
    "SUMMONED_MOUNT",  # 23 /* resets CD_SUMMON_MOUNT when extracted */
    "AQUATIC",  # 24        /* Mob can't enter non-water rooms    */
    "AGGR_EVIL_RACE",  # 25
    "AGGR_GOOD_RACE",  # 26
    "NOSILENCE",  # 27
    "NOVICIOUS",  # 28
    "TEACHER",  # 29
    "ANIMATED",  # 30        /* mob is animated - die if no anim effect */
    "PEACEFUL",  # 31        /* mob can't be attacked.             */
    "NOPOISON",  # 32        /* Mob cannot be poisoned.            */
    "ILLUSORY",  # 33        /* is an illusion: does no harm, leaves no corpse */
    "PLAYER_PHANTASM",  # 34 /* illusion of player; mobs are aggro to */
    "NO_CLASS_AI",  # 35     /* Mob does not execute class AI      */
    "NOSCRIPT",  # 36        /* Mob does not execute triggers or specprocs */
    "PEACEKEEPER",  # 37     /* Attacks mobs with over 1350 align diff. Assists other PEACEKEEPERs */
    "PROTECTOR",  # 38       /* Assists players under attack, but not against PEACEKEEPER/PROTECTOR mobs */
    "PET",  # 39             /* Mob was purchased or tamed and is now a pet to a player. */
]

ROOM_FLAGS = [
    "DARK",  # 0         /* Dark                           */
    "DEATH",  # 1        /* Death trap                     */
    "NOMOB",  # 2        /* MOBs not allowed               */
    "INDOORS",  # 3      /* Indoors                        */
    "PEACEFUL",  # 4     /* Violence not allowed           */
    "SOUNDPROOF",  # 5   /* Shouts, gossip blocked         */
    "NOTRACK",  # 6      /* Track won't go through         */
    "NOMAGIC",  # 7      /* Magic not allowed              */
    "TUNNEL",  # 8       /* room for only 2 pers           */
    "PRIVATE",  # 9      /* Can't teleport in              */
    "GODROOM",  # 10     /* LVL_GOD+ only allowed          */
    "HOUSE",  # 11       /* (R) Room is a house            */
    "HOUSE_CRASH",  # 12 /* (R) House needs saving         */
    "ATRIUM",  # 13      /* (R) The door to a house        */
    "OLC",  # 14         /* (R) Modifyable/!compress       */
    "BFS_MARK",  # 15    /* (R) breadth-first srch mrk     */
    "NOWELL",  # 16      /* No spell portals like moonwell */
    "NORECALL",  # 17    /* No recalling                   */
    "UNDERDARK",  # 18   /*                   (not used)   */
    "NOSUMMON",  # 19    /* Can't summon to or from. Can't banish here. */
    "NOSHIFT",  # 20     /* no plane shift    (not used)   */
    "GUILDHALL",  # 21   /*                   (not used)   */
    "NOSCAN",  # 22      /* Unable to scan to/from rooms   */
    "ALT_EXIT",  # 23    /* Room's exits are altered       */
    "MAP",  # 24         /* Room on surface map (unused)   */
    "ALWAYSLIT",  # 25   /* Makes the room lit             */
    "ARENA",  # 26       /* (safe) PK allowed in room      */
    "OBSERVATORY",  # 27 /* see into adjacent ARENA rooms  */
]

EXIT_FLAGS = [
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

TRIGGER_TYPES = [
    "Global",  # 0 - check even if zone empty
    "Random",  # 1 - checked randomly
    "Command",  # 2 - character types a command
    "Speech",  # 3 - a char says a word/phrase
    "Act",  # 4 - word or phrase sent to act
    "Death",  # 5 - character dies
    "Greet",  # 6 - something enters room seen
    "GreetAll",  # 7 - anything enters room
    "Entry",  # 8 - the mob enters a room
    "Receive",  # 9 - character is given obj
    "Fight",  # 10 - each pulse while fighting
    "HitPercentage",  # 11 - fighting and below some hp
    "Bribe",  # 12 - coins are given to mob
    "SpeechTo",  # 13 - ask/whisper/tell
    "Load",  # 14 - the mob is loaded
    "Cast",  # 15 - mob is target of cast
    "Leave",  # 16 - someone leaves room seen
    "Door",  # 17 - door manipulated in room
    "Look",  # 18 - the mob is looked at
    "Time",  # 19 - the mud hour changes
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

PREFERENCE_FLAGS = [
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

PRIVILEGE_FLAGS = [
    "PRV_CLAN_ADMIN",  # 0  /* clan administrator */
    "PRV_TITLE",  # 1       /* can change own title */
    "PRV_ANON_TOGGLE",  # 2 /* can toggle anon */
    "PRV_AUTO_GAIN",  # 3   /* don't need to level gain */
]
