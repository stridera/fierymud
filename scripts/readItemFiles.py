import json
import re

SPELLS = [
    "NONE",
    "ARMOR",                 #  1
    "TELEPORT",              #  2
    "BLESS",                 #  3
    "BLINDNESS",             #  4
    "BURNING_HANDS",         #  5
    "CALL_LIGHTNING",        #  6
    "CHARM",                 #  7
    "CHILL_TOUCH",           #  8
    "CLONE",                 #  9
    "COLOR_SPRAY",           # 10
    "CONTROL_WEATHER",       # 11
    "CREATE_FOOD",           # 12
    "CREATE_WATER",          # 13
    "CURE_BLIND",            # 14
    "CURE_CRITIC",           # 15
    "CURE_LIGHT",            # 16
    "CURSE",                 # 17
    "DETECT_ALIGN",          # 18
    "DETECT_INVIS",          # 19
    "DETECT_MAGIC",          # 20
    "DETECT_POISON",         # 21
    "DISPEL_EVIL",           # 22
    "EARTHQUAKE",            # 23
    "ENCHANT_WEAPON",        # 24
    "ENERGY_DRAIN",          # 25
    "FIREBALL",              # 26
    "HARM",                  # 27
    "HEAL",                  # 28
    "INVISIBLE",             # 29
    "LIGHTNING_BOLT",        # 30
    "LOCATE_OBJECT",         # 31
    "MAGIC_MISSILE",         # 32
    "POISON",                # 33
    "PROT_FROM_EVIL",        # 34
    "REMOVE_CURSE",          # 35
    "SANCTUARY",             # 36
    "SHOCKING_GRASP",        # 37
    "SLEEP",                 # 38
    "STRENGTH",              # 39
    "SUMMON",                # 40
    "VENTRILOQUATE",         # 41
    "WORD_OF_RECALL",        # 42
    "REMOVE_POISON",         # 43
    "SENSE_LIFE",            # 44
    "ANIMATE_DEAD",          # 45
    "DISPEL_GOOD",           # 46
    "GROUP_ARMOR",           # 47
    "GROUP_HEAL",            # 48
    "GROUP_RECALL",          # 49
    "INFRAVISION",           # 50
    "WATERWALK",             # 51
    "STONE_SKIN",            # 52
    "FULL_HEAL",             # 53
    "FULL_HARM",             # 54
    "WALL_OF_FOG",           # 55
    "WALL_OF_STONE",         # 56
    "FLY",                   # 57
    "SUMMON_DRACOLICH",      # 58
    "SUMMON_ELEMENTAL",      # 59
    "SUMMON_DEMON",          # 60
    "SUMMON_GREATER_DEMON",  # 61
    "DIMENSION_DOOR",        # 62
    "CREEPING_DOOM",         # 63
    "DOOM",                  # 64
    "METEORSWARM",           # 65
    "BIGBYS_CLENCHED_FIST",  # 66
    "FARSEE",                # 67
    "HASTE",                 # 68
    "BLUR",                  # 69
    "GREATER_ENDURANCE",     # 70
    "MOONWELL",              # 71
    "INN_STRENGTH",          # 72
    "DARKNESS",              # 73
    "ILLUMINATION",          # 74
    "COMPREHEND_LANG",       # 75
    "CONE_OF_COLD",          # 76
    "ICE_STORM",             # 77
    "ICE_SHARDS",            # 78
    "MAJOR_PARALYSIS",       # 79
    "VAMPIRIC_BREATH",       # 80
    "RESURRECT",             # 81
    "INCENDIARY_NEBULA",     # 82
    "MINOR_PARALYSIS",       # 83
    "CAUSE_LIGHT",           # 84
    "CAUSE_SERIOUS",         # 85
    "CAUSE_CRITIC",          # 86
    "PRESERVE",              # 87
    "CURE_SERIOUS",          # 88
    "VIGORIZE_LIGHT",        # 89
    "VIGORIZE_SERIOUS",      # 90
    "VIGORIZE_CRITIC",       # 91
    "SOULSHIELD",            # 92
    "DESTROY_UNDEAD",        # 93
    "SILENCE",               # 94
    "FLAMESTRIKE",           # 95
    "UNHOLY_WORD",           # 96
    "HOLY_WORD",             # 97
    "PLANE_SHIFT",           # 98
    "DISPEL_MAGIC",          # 99
    "MINOR_CREATION",        #100
    "CONCEALMENT",           #101
    "RAY_OF_ENFEEB",         #102
    "LEVITATE",              #103
    "WIZARD_EYE",            #104
    "FIRESHIELD",            #105
    "COLDSHIELD",            #106
    "MINOR_GLOBE",           #107
    "MAJOR_GLOBE",           #108
    "DISINTEGRATE",          #109
    "HARNESS",               #110
    "CHAIN_LIGHTNING",       #111
    "MASS_INVIS",            #112
    "RELOCATE",              #113
    "FEAR",                  #114
    "CIRCLE_OF_LIGHT",       #115
    "DIVINE_BOLT",           #116
    "PRAYER",                #117
    "ELEMENTAL_WARDING",     #118
    "DIVINE_RAY",            #119
    "LESSER_EXORCISM",       #120
    "DECAY",                 #121
    "SPEAK_IN_TONGUES",      #122
    "ENLIGHTENMENT",         #123
    "EXORCISM",              #124
    "SPINECHILLER",          #125
    "WINGS_OF_HEAVEN",       #126
    "BANISH",                #127
    "WORD_OF_COMMAND",       #128
    "DIVINE_ESSENCE",        #129
    "HEAVENS_GATE",          #130
    "DARK_PRESENCE",         #131
    "DEMONSKIN",             #132
    "DARK_FEAST",            #133
    "HELL_BOLT",             #134
    "DISEASE",               #135
    "INSANITY",              #136
    "DEMONIC_ASPECT",        #137
    "HELLFIRE_BRIMSTONE",    #138
    "STYGIAN_ERUPTION",      #139
    "DEMONIC_MUTATION",      #140
    "WINGS_OF_HELL",         #141
    "SANE_MIND",             #142
    "HELLS_GATE",            #143
    "BARKSKIN",              #144
    "NIGHT_VISION",          #145
    "WRITHING_WEEDS",        #146
    "CREATE_SPRING",         #147
    "NOURISHMENT",           #148
    "GAIAS_CLOAK",           #149
    "NATURES_EMBRACE",       #150
    "ENTANGLE",              #151
    "INVIGORATE",            #152
    "WANDERING_WOODS",       #153
    "URBAN_RENEWAL",         #154
    "SUNRAY",                #155
    "ARMOR_OF_GAIA",         #156
    "FIRE_DARTS",            #157
    "MAGIC_TORCH",           #158
    "SMOKE",                 #159
    "MIRAGE",                #160
    "FLAME_BLADE",           #161
    "POSITIVE_FIELD",        #162
    "FIRESTORM",             #163
    "MELT",                  #164
    "CIRCLE_OF_FIRE",        #165
    "IMMOLATE",              #166
    "SUPERNOVA",             #167
    "CREMATE",               #168
    "NEGATE_HEAT",           #169
    "ACID_BURST",            #170
    "ICE_DARTS",             #171
    "ICE_ARMOR",             #172
    "ICE_DAGGER",            #173
    "FREEZING_WIND",         #174
    "FREEZE",                #175
    "WALL_OF_ICE",           #176
    "ICEBALL",               #177
    "FLOOD",                 #178
    "VAPORFORM",             #179
    "NEGATE_COLD",           #180
    "WATERFORM",             #181
    "EXTINGUISH",            #182
    "RAIN",                  #183
    "REDUCE",                #184
    "ENLARGE",               #185
    "IDENTIFY",              #186
    "BONE_ARMOR",            #187
    "SUMMON_CORPSE",         #188
    "SHIFT_CORPSE",          #189
    "GLORY",                 #190
    "ILLUSORY_WALL",         #191
    "NIGHTMARE",             #192
    "DISCORPORATE",          #193
    "ISOLATION",             #194
    "FAMILIARITY",           #195
    "HYSTERIA",              #196
    "MESMERIZE",             #197
    "SEVERANCE",             #198
    "SOUL_REAVER",           #199
    "DETONATION",            #200
    "FIRE_BREATH",           #201
    "GAS_BREATH",            #202
    "FROST_BREATH",          #203
    "ACID_BREATH",           #204
    "LIGHTNING_BREATH",      #205
    "LESSER_ENDURANCE",      #206
    "ENDURANCE",             #207
    "VITALITY",              #208
    "GREATER_VITALITY",      #209
    "DRAGONS_HEALTH",        #210
    "REBUKE_UNDEAD",         #211
    "DEGENERATION",          #212
    "SOUL_TAP",              #213
    "NATURES_GUIDANCE",      #214
    "MOONBEAM",              #215
    "PHANTASM",              #216
    "SIMULACRUM",            #217
    "MISDIRECTION",          #218
    "CONFUSION",             #219
    "PHOSPHORIC_EMBERS",     #220
    "RECALL",                #221
    "PYRE",                  #222
    "IRON_MAIDEN",           #223
    "FRACTURE",              #224
    "FRACTURE_SHRAPNEL",     #225
    "BONE_DRAW",             #226
    "PYRE_RECOIL",           #227
    "WORLD_TELEPORT",        #228
]

OBJECT_TYPES = [
    "NOTHING",
    "LIGHT",        # 1,   # /* Item is a light source          */
    "SCROLL",       # 2,   # /* Item is a scroll                */
    "WAND",         # 3,   # /* Item is a wand                  */
    "STAFF",        # 4,   # /* Item is a staff                 */
    "WEAPON",       # 5,   # /* Item is a weapon                */
    "FIREWEAPON",   # 6,   # /* Unimplemented                   */
    "MISSILE",      # 7,   # /* Unimplemented                   */
    "TREASURE",     # 8,   # /* Item is a treasure, not gold    */
    "ARMOR",        #9 ,   # /* Item is armor                   */
    "POTION",       # 10,  # /* Item is a potion                */
    "WORN",         # 11,  # /* Unimplemented                   */
    "OTHER",        # 12,  # /* Misc object                     */
    "TRASH",        # 13,  # /* Trash - shopkeeps won't buy     */
    "TRAP",         # 14,  # /* Unimplemented                   */
    "CONTAINER",    # 15,  # /* Item is a container             */
    "NOTE",         # 16,  # /* Item is note                    */
    "DRINKCON",     # 17,  # /* Item is a drink container       */
    "KEY",          # 18,  # /* Item is a key                   */
    "FOOD",         # 19,  # /* Item is food                    */
    "MONEY",        # 20,  # /* Item is money (gold)            */
    "PEN",          # 21,  # /* Item is a pen                   */
    "BOAT",         # 22,  # /* Item is a boat                  */
    "FOUNTAIN",     # 23,  # /* Item is a fountain              */
    "PORTAL",       # 24,  # /* Item teleports to another room  */
    "ROPE",         # 25,  # /* Item is used to bind chars      */
    "SPELLBOOK",    # 26,  # /* Spells can be scribed for mem   */
    "WALL",         # 27,  # /* Blocks passage in one direction */
    "TOUCHSTONE",   # 28,  # /* Item sets homeroom when touched */
    "BOARD",        # 29,  # Bullitin board
]

WEAR_FLAGS = [
    'TAKE',      # /* Item can be takes         */
    'FINGER',    # /* Can be worn on finger     */
    'NECK',      # /* Can be worn around neck   */
    'BODY',      # /* Can be worn on body       */
    'HEAD',      # /* Can be worn on head       */
    'LEGS',      # /* Can be worn on legs       */
    'FEET',      # /* Can be worn on feet       */
    'HANDS',     # /* Can be worn on hands      */
    'ARMS',      # /* Can be worn on arms       */
    'SHIELD',    # /* Can be used as a shield   */
    'ABOUT',    # /* Can be worn about body    */
    'WAIST',    # /* Can be worn around waist  */
    'WRIST',    # /* Can be worn on wrist      */
    'WIELD',    # /* Can be wielded            */
    'HOLD',     # /* Can be held               */
    '2HWIELD',  # /* Can be wielded two handed */
    'EYES',     # /* Can be worn on eyes       */
    'FACE',     # /* Can be worn upon face     */
    'EAR',      # /* Can be worn in ear        */
    'BADGE',    # /* Can be worn as badge      */
    'BELT',     # /* Can be worn on belt       */
]

EXTRA_OBJ_FLAGS = [
    "GLOW",              # 0,      # /* Item is glowing               */
    "HUM",               # 1,      # /* Item is humming               */
    "NORENT",            # 2,      # /* Item cannot be rented         */
    "NODONATE",          # 3,      # /* Item cannot be donated        */
    "NOINVIS",           # 4,      # /* Item cannot be made invis     */
    "INVISIBLE",         # 5,      # /* Item is invisible             */
    "MAGIC",             # 6,      # /* Item is magical               */
    "NODROP",            # 7,      # /* Item can't be dropped         */
    "PERMANENT",         # 8,      # /* Item doesn't decompose        */
    "ANTI_GOOD",         # 9,      # /* Not usable by good people     */
    "ANTI_EVIL",         # 10,     # /* Not usable by evil people     */
    "ANTI_NEUTRAL",      # 11,     # /* Not usable by neutral people  */
    "ANTI_SORCERER",     # 12,     # /* Not usable by sorcerers       */
    "ANTI_CLERIC",       # 13,     # /* Not usable by clerics         */
    "ANTI_ROGUE",        # 14,     # /* Not usable by rogues          */
    "ANTI_WARRIOR",      # 15,     # /* Not usable by warriors        */
    "NOSELL",            # 16,     # /* Shopkeepers won't touch it    */
    "ANTI_PALADIN",      # 17,     # /* Not usable by paladins        */
    "ANTI_ANTI_PALADIN",  # 18,     # /* Not usable by anti-paladins   */
    "ANTI_RANGER",       # 19,     # /* Not usable by rangers         */
    "ANTI_DRUID",        # 20,     # /* Not usable by druids          */
    "ANTI_SHAMAN",       # 21,     # /* Not usable by shamans         */
    "ANTI_ASSASSIN",     # 22,     # /* Not usable by assassins       */
    "ANTI_MERCENARY",    # 23,     # /* Not usable by mercenaries     */
    "ANTI_NECROMANCER",  # 24,     # /* Not usable by necromancers    */
    "ANTI_CONJURER",     # 25,     # /* Not usable by conjurers       */
    "NOBURN",            # 26,     # /* Not destroyed by purge/fire   */
    "NOLOCATE",          # 27,     # /* Cannot be found by locate obj */
    "DECOMP",            # 28,     # /* Item is currently decomposint */
    "FLOAT",             # 29,     # /* Floats in water rooms         */
    "NOFALL",            # 30,     # /* Doesn't fall - unaffected by gravity */
    "WAS_DISARMED",      # 31,     # /* Disarmed from mob             */
]

AFFECTS = [
    "NONE",            # 0        /* No effect                       */
    "STR",             # 1        /* Apply to strength               */
    "DEX",             # 2        /* Apply to dexterity              */
    "INT",             # 3        /* Apply to constitution           */
    "WIS",             # 4        /* Apply to wisdom                 */
    "CON",             # 5        /* Apply to constitution           */
    "CHA",             # 6        /* Apply to charisma               */
    "CLASS",           # 7        /* Reserved                        */
    "LEVEL",           # 8        /* Reserved                        */
    "AGE",             # 9        /* Apply to age                    */
    "CHAR_WEIGHT",     # 10        /* Apply to weight                 */
    "CHAR_HEIGHT",     # 11        /* Apply to height                 */
    "MANA",            # 12        /* Apply to max mana               */
    "HIT",             # 13        /* Apply to max hit points         */
    "MOVE",            # 14        /* Apply to max move points        */
    "GOLD",            # 15        /* Reserved                        */
    "EXP",             # 16        /* Reserved                        */
    "AC",              # 17        /* Apply to Armor Class            */
    "HITROLL",         # 18        /* Apply to hitroll                */
    "DAMROLL",         # 19        /* Apply to damage roll            */
    "SAVING_PARA",     # 20        /* Apply to save throw: paralz     */
    "SAVING_ROD",      # 21        /* Apply to save throw: rods       */
    "SAVING_PETRI",    # 22        /* Apply to save throw: petrif     */
    "SAVING_BREATH",   # 23        /* Apply to save throw: breath     */
    "SAVING_SPELL",    # 24        /* Apply to save throw: spells     */
    "SIZE",            # 25        /* Apply to size                   */
    "HIT_REGEN",       # 26
    "MANA_REGEN",      # 27
    "PERCEPTION",      # 28
    "HIDDENNESS",      # 29
    "COMPOSITION",     # 30
]

EFFECTS = [
    "BLIND",             # 0   /* (R) Char is blind            */
    "INVISIBLE",         # 1   /* Char is invisible            */
    "DETECT_ALIGN",      # 2   /* Char is sensitive to align   */
    "DETECT_INVIS",      # 3   /* Char can see invis chars     */
    "DETECT_MAGIC",      # 4   /* Char is sensitive to magic   */
    "SENSE_LIFE",        # 5   /* Char can sense hidden life   */
    "WATERWALK",         # 6   /* Char can walk on water       */
    "SANCTUARY",         # 7   /* Char protected by sanct.     */
    "CONFUSION",         # 8   /* Char is confused             */
    "CURSE",             # 9   /* Char is cursed               */
    "INFRAVISION",      # 10   /* Char can see in dark         */
    "POISON",           # 11   /* (R) Char is poisoned         */
    "PROTECT_EVIL",     # 12   /* Char protected from evil     */
    "PROTECT_GOOD",     # 13   /* Char protected from good     */
    "SLEEP",            # 14   /* (R) Char magically asleep    */
    "NOTRACK",          # 15   /* Char can't be tracked        */
    "TAMED",            # 16   /* Tamed!                       */
    "BERSERK",          # 17   /* Char is berserking           */
    "SNEAK",            # 18   /* Char is sneaking             */
    "STEALTH",          # 19   /* Char is using stealth        */
    "FLY",              # 20   /* Char has the ability to fly  */
    "CHARM",            # 21   /* Char is charmed              */
    "STONE_SKIN",       # 22
    "FARSEE",           # 23
    "HASTE",            # 24
    "BLUR",             # 25
    "VITALITY",         # 26
    "GLORY",            # 27
    "MAJOR_PARALYSIS",  # 28
    "FAMILIARITY",      # 29   /* Char is considered friend    */
    "MESMERIZED",       # 30   /* Super fasciated by something */
    "IMMOBILIZED",      # 31   /* Char cannot move             */
    "LIGHT",            # 32
    "UNUSED",           # 33
    "MINOR_PARALYSIS",  # 34
    "HURT_THROAT",      # 35
    "LEVITATE",         # 36
    "WATERBREATH",      # 37
    "SOULSHIELD",       # 38
    "SILENCE",          # 39
    "PROT_FIRE",        # 40
    "PROT_COLD",        # 41
    "PROT_AIR",         # 42
    "PROT_EARTH",       # 43
    "FIRESHIELD",       # 44
    "COLDSHIELD",       # 45
    "MINOR_GLOBE",      # 46
    "MAJOR_GLOBE",      # 47
    "HARNESS",          # 48
    "ON_FIRE",          # 49
    "FEAR",             # 50
    "TONGUES",          # 51
    "DISEASE",          # 52
    "INSANITY",         # 53
    "ULTRAVISION",      # 54
    "NEGATE_HEAT",      # 55
    "NEGATE_COLD",      # 56
    "NEGATE_AIR",       # 57
    "NEGATE_EARTH",     # 58
    "REMOTE_AGGR",      # 59   /* Your aggro action won't remove invis/bless etc. */
    "UNUSED",           # 60
    "UNUSED",           # 61
    "UNUSED",           # 62
    "UNUSED",           # 63
    "AWARE",            # 64
    "REDUCE",           # 65
    "ENLARGE",          # 66
    "VAMP_TOUCH",       # 67
    "RAY_OF_ENFEEB",    # 68
    "ANIMATED",         # 69
    "UNUSED",           # 70
    "SHADOWING",        # 71
    "CAMOUFLAGED",      # 72
    "SPIRIT_WOLF",      # 73
    "SPIRIT_BEAR",      # 74
    "WRATH",            # 75
    "MISDIRECTION",     # 76   /* Capable of performing misdirection */
    "MISDIRECTING",     # 77   /* Currently actually moving but misdirecting */
    "BLESS",            # 78   /* When blessed,# our barehand attacks hurt ether chars */
    "HEX",              # 79   /* The evil side of blessing,# o hurt ether chars */
    "DETECT_POISON",    # 80   /* Char is sensitive to poison */
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
]

LIQUIDS = [
    "WATER",         # 0
    "BEER",          # 1
    "WINE",          # 2
    "ALE",           # 3
    "DARKALE",       # 4
    "WHISKY",        # 5
    "LEMONADE",      # 6
    "FIREBRT",       # 7
    "LOCALSPC",      # 8
    "SLIME",         # 9
    "MILK",         # 10
    "TEA",          # 11
    "COFFEE",       # 12
    "BLOOD",        # 13
    "SALTWATER",    # 14
    "RUM",          # 15
    "NECTAR",       # 16
    "SAKE",         # 17
    "CIDER",        # 18
    "TOMATOSOUP",   # 19
    "POTATOSOUP",   # 20
    "CHAI",         # 21
    "APPLEJUICE",   # 22
    "ORNGJUICE",    # 23
    "PNAPLJUICE",   # 24
    "GRAPEJUICE",   # 25
    "POMJUICE",     # 26
    "MELONAE",      # 27
    "COCOA",        # 28
    "ESPRESSO",     # 29
    "CAPPUCCINO",   # 30
    "MANGOLASSI",   # 31
    "ROSEWATER",    # 32
    "GREENTEA",     # 33
    "CHAMOMILE",    # 34
    "GIN",          # 35
    "BRANDY",       # 36
    "MEAD",         # 37
    "CHAMPAGNE",    # 38
    "VODKA",        # 39
    "TEQUILA",      # 40
    "ABSINTHE",     # 41
    "LIQ_TYPES",    # 42
]


def objVal(type, *args):
    if type == "WEAPON":
        dice = int(args[1])
        dice_size = int(args[2])
        average = (float(dice + 1) / 2.0) * float(dice_size)
        return {
            'Hitroll': args[0],
            'Hit Dice': args[1],
            'Dice Size': args[2],
            'Average': average,
            'Damage Type': DAMAGE_TYPES[int(args[3])]
        }
    elif type == "ARMOR":
        return {
            'AC': args[0]
        }
    elif type == "LIGHT":
        return {
            'Is_Lit:': args[0],
            'Capacity': args[1],
            'Remaining': "PERMANENT" if int(args[2]) == -1 else args[2]
        }
    elif type == "SPELLBOOK":
        return {
            'Pages': args[0],
        }
    elif type == "SCROLL" or type == "POTION":
        return {
            'Level': args[0],
            'Spell 1': SPELLS[int(args[1])],
            'Spell 2': SPELLS[int(args[2])],
            'Spell 3': SPELLS[int(args[3])],
        }
    elif type == "WAND" or type == "STAFF":
        return {
            'Level': args[0],
            'Max_Charges': args[1],
            'Charges_Left': args[2],
            'Spell': SPELLS[int(args[3])],
        }
    elif type == "CONTAINER":
        flags = buildFlags(args[1], ['Closeable', 'Pickproof', 'Closed', 'Locked'])
        return {
            'Capacity': args[0],
            'flags': flags,
            'Key': args[2],
            'Corpse': args[3],
        }
    elif type == "DRINKCON":
        return {
            'Capacity': args[0],
            'Remaining': args[1],
            'Liquid': LIQUIDS[int(args[2])],
            'Poisoned': args[3],
        }
    elif type == "FOOD":
        return {
            'Fillingness': args[0],
            'Poisoned': args[1],
        }
    elif type == "MONEY":
        return {
            'Platinum': args[0],
            'Gold': args[1],
            'Silver': args[2],
            'Copper': args[3],
        }
    elif type == "FOUNTAIN":
        return {
            'Capacity': args[0],
            'Remaining': args[1],
            'Liquid': LIQUIDS[int(args[2])],
            'Poisoned': args[3],
        }
    elif type == "PORTAL":
        return {
            'Destination': args[0],
            'Entry_Message': args[1],
            'Character_Message': args[2],
            'Exit_Message': args[3],
        }
    elif type == "WALL":
        return {
            'Direction': args[0],
            'Dispelable': args[1],
            'Hitpoints': args[2],
            'Spell': args[3],
        }
    elif type == "BOARD":
        return {
            'Pages': args[0],
        }
    else:
        # return("Unknown type " + type, "Flags:", args)
        return {}


def buildFlags(flag, flaglist, offset=0):
    active = []
    for i in range(len(flaglist)):
        if int(flag) & (1 << i):
            active.append(flaglist[i+offset])

    return ",".join(active)


def readString(f):
    text = ""
    line = f.readline().rstrip()
    while not line.endswith("~"):
        text += line + " "
        line = f.readline().rstrip()
    text += line[:-1]

    return re.sub(r'&.', '', text)




def processObjFile(filename):
    objects = []

    # print("Processing ", filename)
    with open(filename, 'r') as f:
        line = f.readline().strip()
        objID = -1
        while line:
            if objID == -1:
                if line.startswith('$'):
                    # print("End of File")
                    break
                elif line.startswith('#'):
                    objID = int(line[1:])
                else:
                    print("Error:  Expected ID, instead got", line)
                    exit()
            else:
                obj = {}
                name = readString(f)
                short_desc = readString(f)
                desc = readString(f)
                action_desc = readString(f)
                # print("Object", objID)
                # print("name:", name)
                # print("short:", short_desc)
                # print("desc:", desc)
                # print("action:", action_desc)

                type_flag, extra_flags, wear_flags, level = f.readline().strip().split(' ')
                f1, f2, f3, f4, f5, f6, f7 = f.readline().strip().split(' ')
                weight, cost, timer, aff1, _, _, aff2, aff3 = f.readline().strip().split(' ')
                itemType = OBJECT_TYPES[int(type_flag)]
                extra_flags_text = buildFlags(extra_flags, EXTRA_OBJ_FLAGS)
                wear_flags_text = buildFlags(wear_flags, WEAR_FLAGS)
                extra_stats = objVal(itemType, f1, f2, f3, f4, f5, f6, f7)

                # print("Type:", itemType)
                # print("Extra Flags:", extra_flags_text)
                # print("Wear Flags:", wear_flags_text)
                # print("Level:", level)
                # print("Stats:", extra_stats)
                # print("Weight: ", weight, "Cost:", cost, "Timer:", timer)

                affects = (int(aff3) << 26) | (int(aff2) << int(13)) | int(aff1)
                obj_effects = buildFlags(affects, EFFECTS)
                # if affects > 0:
                #     print("Affect: ", obj_effects)

                obj = {
                    "id": int(objID),
                    "name": name,
                    "short_desc": short_desc,
                    "desc": desc,
                    "action_desc": action_desc,
                    "type": itemType,
                    "obj_flags": extra_flags_text,
                    "wear_flags": wear_flags_text,
                    "level": int(level),
                    "stats": extra_stats,
                    "weight": float(weight),
                    "cost": int(cost),
                    "timer": int(timer),
                    "effects": obj_effects
                }

                while True:
                    line = f.readline().strip()
                    if line.startswith('E'):
                        keyword = readString(f)
                        extra_desc = readString(f)
                        if "extra_desc" not in obj.keys():
                            obj["extra_desc"] = []
                        obj["extra_desc"].append({'keyword': keyword, 'desc': extra_desc})
                        # print("Extra Desc: (", keyword, ") ", extra_desc)
                    elif line.startswith('A'):
                        location, modifier = f.readline().strip().split(' ')
                        if "affects" not in obj.keys():
                            obj["affects"] = []
                        obj["affects"].append({'location': AFFECTS[int(location)], 'modifier': int(modifier)})
                        # print("Applies:", AFFECTS[int(location)], modifier)
                    elif line.startswith('H'):
                        hiddenness = f.readline().strip()
                        obj['hiddenness'] = hiddenness
                        # print("Hiddenness", hiddenness)
                    elif line.startswith('T'):
                        if "triggers" not in obj.keys():
                            obj["triggers"] = []
                        obj["triggers"].append(int(line[2:]))
                        # print("Trigger", line[2:])
                    elif line.startswith(('#', '$')):
                        objects.append(obj)
                        objID = -1
                        break
                    else:
                        print("Unknown flag:", line)
                        exit()
    return objects


# def main():
#     path = '/home/strider/Code/fierymud/lib/world/obj/'
#     with open(path+"index", 'r') as f:
#         line = str.strip(f.readline())
#         while line:
#             if line == "$":
#                 pass
#                 # print("End of Index")
#             else:
#                 processObjFile(path + line)
#             line = f.readline().strip()

#     with open('fiery_items.json', 'w') as outfile:
#         json.dump(objects, outfile)

def processIndex(index_file):
    objects = []
    path = index_file[:-5]
    with open(index_file, 'r') as f:
        while line := f.readline().strip():
            if line.startswith('$'):
                print('End of index')
                return
            else:
                objects.append(processObjFile(path + line))
    return objects




def main(obj_file):
    objects = None
    if obj_file.endswith('index'):
        objects = processIndex(obj_file)
    else:
        objects = processObjFile(obj_file)

    with open('fiery_items.json', 'w') as outfile:
        json.dump(objects, outfile)


if __name__ == '__main__':
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Converts a mob file to use ascii flags.')
    parser.add_argument('--file', help='The mob file to convert or index file to convert all.',
                        type=str, default='lib/world/mob/index')
    parser.add_argument('-n', '--number', help='The mob number to convert.', type=int, default=-1)
    args = parser.parse_args()
    main(args.file, args.number)
