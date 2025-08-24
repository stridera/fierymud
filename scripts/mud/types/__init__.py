import re
from dataclasses import dataclass
from enum import Enum, auto

from mud.bitflags import BitFlags
from mud.flags import DAMAGE_TYPES, LIQUIDS, SPELLS


class MudTypes(Enum):
    """
    Enum for the different types of mud objects
    """

    ZONE = auto()
    MOB = auto()
    OBJECT = auto()
    SHOP = auto()
    TRIGGER = auto()
    WORLD = auto()

    PLAYER = auto()
    QUESTS = auto()
    PET = auto()
    NOTES = auto()

    @classmethod
    def from_ext(cls, ext: str):
        """
        Gets the MudType from the file extension
        :param ext: The file extension
        :return: The MudType
        """
        match ext:
            case "mob":
                return cls.MOB
            case "obj":
                return cls.OBJECT
            case "shp":
                return cls.SHOP
            case "trg":
                return cls.TRIGGER
            case "zon":
                return cls.ZONE
            case "wld":
                return cls.WORLD

            case "plr":
                return cls.PLAYER
            case "objs":
                return cls.OBJECT
            case "pet":
                return cls.PET
            case "quest":
                return cls.QUESTS
            case "notes":
                return cls.NOTES
            case _:
                raise ValueError(f"Invalid file extension: {ext}")

    def cls(self):
        """
        Gets the class for the MudType
        :return: The class
        """
        match self:
            case MudTypes.MOB:
                from mud.types.mob import Mob

                return Mob
            case MudTypes.OBJECT:
                from mud.types.object import Object

                return Object
            case MudTypes.SHOP:
                from mud.types.shop import Shop

                return Shop
            case MudTypes.TRIGGER:
                from mud.types.trigger import Trigger

                return Trigger
            case MudTypes.WORLD:
                from mud.types.world import World

                return World
            case MudTypes.ZONE:
                from mud.types.zone import Zone

                return Zone
            case MudTypes.PLAYER:
                from mud.types.player import Player

                return Player
            case MudTypes.PET:
                from mud.types.pet import Pet

                return Pet
            case MudTypes.QUESTS:
                from mud.types.quests import Quests

                return Quests
            case MudTypes.NOTES:
                from mud.types.notes import Notes

                return Notes
            case _:
                raise ValueError(f"Invalid MudType: {self}")

    def get_json_id(self):
        match self:
            case MudTypes.MOB:
                return "mobs"
            case MudTypes.OBJECT:
                return "objects"
            case MudTypes.SHOP:
                return "shops"
            case MudTypes.TRIGGER:
                return "triggers"
            case MudTypes.WORLD:
                return "rooms"
            case MudTypes.ZONE:
                return "zone"
            case MudTypes.PLAYER:
                return "player"
            case MudTypes.PET:
                return "pet"
            case MudTypes.QUESTS:
                return "quests"
            case MudTypes.NOTES:
                return "notes"
            case _:
                raise ValueError(f"Invalid MudType: {self}")


@dataclass
class CurrentMax:
    current: int
    max: int

    def to_json(self):
        from dataclasses import asdict

        return asdict(self)


@dataclass
class Stats:
    strength: int
    intelligence: int
    wisdom: int
    dexterity: int
    constitution: int
    charisma: int

    def to_json(self):
        from dataclasses import asdict

        return asdict(self)


@dataclass
class Money:
    copper: int
    silver: int
    gold: int
    platinum: int

    def to_json(self):
        from dataclasses import asdict

        return asdict(self)


@dataclass
class SavingThrows:
    paralysis: int
    rod: int
    petrification: int
    breath: int
    spell: int

    def to_json(self):
        from dataclasses import asdict

        return asdict(self)


@dataclass
class Dice:
    num: int
    size: int
    bonus: int

    @staticmethod
    def from_string(dice_string):
        num, size, bonus = re.split(r"[+d]", dice_string)
        return Dice(int(num), int(size), int(bonus))

    def __str__(self):
        return f"{self.num}d{self.size}+{self.bonus}"

    def __repr__(self):
        return f"{self.num}d{self.size}+{self.bonus}"

    def to_json(self):
        return {"num": self.num, "size": self.size, "bonus": self.bonus}


class Direction(Enum):
    North = 0
    East = 1
    South = 2
    West = 3
    Up = 4
    Down = 5


class WearFlags(Enum):
    Take = 0  # Item can be taken
    Finger = 1  # Can be worn on finger
    Neck = 2  # Can be worn around neck
    Body = 3  # Can be worn on body
    Head = 4  # Can be worn on head
    Legs = 5  # Can be worn on legs
    Feet = 6  # Can be worn on feet
    Hands = 7  # Can be worn on hands
    Arms = 8  # Can be worn on arms
    Shield = 9  # Can be used as a shield
    About = 10  # Can be worn about body
    Waist = 11  # Can be worn around waist
    Wrist = 12  # Can be worn on wrist
    Wield = 13  # Can be wielded
    Hold = 14  # Can be held
    TwoHandWield = 15  # Can be wielded two handed
    Eyes = 16  # Can be worn on eyes
    Face = 17  # Can be worn upon face
    Ear = 18  # Can be worn in ear
    Badge = 19  # Can be worn as badge
    Belt = 20  # Can be worn on belt
    Hover = 21  # Hovers above you


class ScriptType(Enum):
    Mob = 0
    Object = 1
    World = 2


class Gender(Enum):
    Neutral = 0
    Male = 1
    Female = 2
    NonBinary = 3


class Class(Enum):
    Sorcerer = 0
    Cleric = 1
    Thief = 2
    Warrior = 3
    Paladin = 4
    AntiPaladin = 5
    Ranger = 6
    Druid = 7
    Shaman = 8
    Assassin = 9
    Mercenary = 10
    Necromancer = 11
    Conjurer = 12
    Monk = 13
    Berserker = 14
    Priest = 15
    Diabolist = 16
    Mystic = 17
    Rogue = 18
    Bard = 19
    Pyromancer = 20
    Cryomancer = 21
    Illusionist = 22
    Hunter = 23
    Layman = 24


class Position(Enum):
    Prone = 0
    Sitting = 1
    Kneeling = 2
    Standing = 3
    Flying = 4


class Stance(Enum):
    Dead = 0  # dead
    Mort = 1  # mortally wounded
    Incapacitated = 2  # incapacitated
    Stunned = 3  # stunned
    Sleeping = 4  # sleeping
    Resting = 5  # resting
    Alert = 6  # alert
    Fighting = 7  # fighting


class Race(Enum):
    Human = 0
    Elf = 1
    Gnome = 2
    Dwarf = 3
    Troll = 4
    Drow = 5
    Duergar = 6
    Ogre = 7
    Orc = 8
    HalfElf = 9
    Barbarian = 10
    Halfling = 11
    Plant = 12
    Humanoid = 13
    Animal = 14
    DragonGeneral = 15
    Giant = 16
    Other = 17
    Goblin = 18
    Demon = 19
    Brownie = 20
    DragonFire = 21
    DragonFrost = 22
    DragonAcid = 23
    DragonLightning = 24
    DragonGas = 25
    DragonbornFire = 26
    DragonbornFrost = 27
    DragonbornAcid = 28
    DragonbornLightning = 29
    DragonbornGas = 30
    Sverfneblin = 31
    FaerieSeelie = 32
    FaerieUnseelie = 33
    Nymph = 34
    Arborean = 35


class LifeForce(Enum):
    Life = 0
    Undead = 1
    Magic = 2
    Celestial = 3
    Demonic = 4
    Elemental = 5


class Composition(Enum):
    Flesh = 0
    Earth = 1
    Air = 2
    Fire = 3
    Water = 4
    Ice = 5
    Mist = 6
    Ether = 7
    Metal = 8
    Stone = 9
    Bone = 10
    Lava = 11
    Plant = 12


class QuitReason(Enum):
    Undef = 0
    Rent = 1
    Cryo = 2
    Timeout = 3
    Hotboot = 4
    QuitMort = 5
    QuitImmortal = 6
    Camp = 7
    WRent = 8  # World Triggered Rent
    Purge = 9
    Autosave = 10


class Cooldown(Enum):
    Backstab = 0
    Bash = 1
    InstantKill = 2
    Disarm = 3
    FumblingPrimary = 4
    DroppedPrimary = 5
    FumblingSecondary = 6
    DroppedSecondary = 7
    SummonMount = 8
    LayHands = 9
    FirstAid = 10
    EyeGouge = 11
    ThroatCut = 12
    ShapeChange = 13
    DefenseChant = 14
    InnateInvisible = 15
    InnateChaz = 16
    InnateDarkness = 17
    InnateFeatherFall = 18
    InnateSyll = 19
    InnateTren = 20
    InnateTass = 21
    InnateBrill = 22
    InnateAscen = 23
    InnateHarness = 24
    Breathe = 25
    InnateCreate = 26
    InnateIllumination = 27
    InnateFaerieStep = 28
    Music1 = 29
    Music2 = 30
    Music3 = 31
    Music4 = 32
    Music5 = 33
    Music6 = 34
    Music7 = 35
    InnateBlindingBeauty = 36
    InnateStatue = 37
    InnateBarkSkin = 38
    OffenseChant = 39


class WearLocation(Enum):
    Light = 0
    FingerRight = 1
    FingerLeft = 2
    Neck1 = 3
    Neck2 = 4
    Body = 5
    Head = 6
    Legs = 7
    Feet = 8
    Hands = 9
    Arms = 10
    Shield = 11
    About = 12
    Waist = 13
    WristRight = 14
    WristLeft = 15
    Wield = 16
    Wield2 = 17
    Hold = 18
    Hold2 = 19
    TwoHandWield = 20
    Eyes = 21
    Face = 22
    Lear = 23
    Rear = 24
    Badge = 25
    OnBelt = 26
    Hover = 27


class ObjectFlags(Enum):
    Glow = 0  #               Item is glowing (Can be seen in the dark)
    Hum = 1  #                Item is humming (Can be seen while blind)
    NoRent = 2  #             Item cannot be rented
    AntiBerserker = 3  #     Not usable by berserker
    NoInvisible = 4  #            Item cannot be made invis
    Invisible = 5  #          Item is invisible
    Magic = 6  #              Item is magical
    NoDrop = 7  #             Item can't be dropped
    Permanent = 8  #          Item doesn't decompose
    AntiGood = 9  #          Not usable by good people
    AntiEvil = 10  #         Not usable by evil people
    AntiNeutral = 11  #      Not usable by neutral people
    AntiSorcerer = 12  #     Not usable by sorcerers
    AntiCleric = 13  #       Not usable by clerics
    AntiRogue = 14  #        Not usable by rogues
    AntiWarrior = 15  #      Not usable by warriors
    NoSell = 16  #            Shopkeepers won't touch it
    AntiPaladin = 17  #      Not usable by paladins
    AntiAntiPaladin = 18  # Not usable by anti-paladins
    AntiRanger = 19  #       Not usable by rangers
    AntiDruid = 20  #        Not usable by druids
    AntiShaman = 21  #       Not usable by shamans
    AntiAssassin = 22  #     Not usable by assassins
    AntiMercenary = 23  #    Not usable by mercenaries
    AntiNecromancer = 24  #  Not usable by necromancers
    AntiConjurer = 25  #     Not usable by conjurers
    NoBurn = 26  #            Not destroyed by purge/fire
    NoLocate = 27  #          Cannot be found by locate obj
    Decomposing = 28  #            Item is currently decomposing
    Float = 29  #             Floats in water rooms
    NoFall = 30  #            Doesn't fall - unaffected by gravity
    WasDisarmed = 31  #      Disarmed from mob
    AntiMonk = 32  #         Not usable by monks
    AntiBard = 33  #
    Elven = 34  #   Item usable by Elves
    Dwarven = 35  # Item usable by Dwarves
    AntiThief = 36  #
    AntiPyromancer = 37  #
    AntiCryomancer = 38  #
    AntiIllusionist = 39  #
    AntiPriest = 40  #
    AntiDiabolist = 41  #
    AntiTiny = 42  #
    AntiSmall = 43  #
    AntiMedium = 44  #
    AntiLarge = 45  #
    AntiHuge = 46  #
    AntiGiant = 47  #
    AntiGargantuan = 48  #
    AntiColossal = 49  #
    AntiTitanic = 50  #
    AntiMountainous = 51  #
    AntiArborean = 52


class KillTypeFlags(Enum):
    Mob = 1
    Player = 2


class ApplyTypes(Enum):
    Str = 1  # Apply to strength
    Dex = 2  # Apply to dexterity
    Int = 3  # Apply to constitution
    Wis = 4  # Apply to wisdom
    Con = 5  # Apply to constitution
    Cha = 6  # Apply to charisma
    Class = 7  # Reserved
    Level = 8  # Reserved
    Age = 9  # Apply to age
    CharacterWeight = 10  # Apply to weight
    CharacterHeight = 11  # Apply to height
    Mana = 12  # Apply to max mana
    Hit = 13  # Apply to max hit points
    Move = 14  # Apply to max move points
    Gold = 15  # Reserved
    Exp = 16  # Reserved
    AC = 17  # Apply to Armor Class
    HitRoll = 18  # Apply to hit roll
    DamRoll = 19  # Apply to damage roll
    SavingParalysis = 20  # Apply to save throw: paralysis
    SavingRod = 21  # Apply to save throw: rods
    SavingPetrification = 22  # Apply to save throw: petrification
    SavingBreath = 23  # Apply to save throw: breath
    SavingSpell = 24  # Apply to save throw: spells
    Size = 25  # Apply to size
    Regeneration = 26  # Restore hit points
    Focus = 27  # Apply to focus level
    Perception = 28
    Concealment = 29
    Composition = 30


class Size(Enum):
    Tiny = 0
    Small = 1
    Medium = 2
    Large = 3
    Huge = 4
    Giant = 5
    Gargantuan = 6
    Colossal = 7
    Titanic = 8
    Mountainous = 9

class DamageType(Enum):
    HIT = 1
    STING = 2
    WHIP = 3
    SLASH = 4
    BITE = 5
    BLUDGEON = 6
    CRUSH = 7
    POUND = 8
    CLAW = 9
    MAUL = 10
    THRASH = 11
    PIERCE = 12
    BLAST = 13
    PUNCH = 14
    STAB = 15
    FIRE = 16
    COLD = 17
    ACID = 18
    SHOCK = 19
    POISON = 20
    ALIGN = 21


def obj_val(item_type: str, *args):
    """Return the object values for the given item type."""
    results = {}
    match item_type:
        case "ARMOR" | "TREASURE":
            results = {"AC": args[0]}
        case "LIGHT":
            results = {
                "Is_Lit:": args[0],
                "Capacity": args[1],
                "Remaining": "PERMANENT" if int(args[2]) == -1 else args[2],
            }
        case "CONTAINER":
            results = {
                "Capacity": args[0],
                "Flags": BitFlags.build_flags(args[1], ["Closeable", "PickProof", "Closed", "Locked"]),
                "Key": args[2],
                "IsCorpse": args[3],
                "Weight Reduction": args[4],
            }
        case "DRINKCON" | "FOUNTAIN":
            results = {
                "Capacity": args[0],
                "Remaining": args[1],
                "Liquid": BitFlags.get_flag(args[2], LIQUIDS),
                "Poisoned": args[3],
            }
        case "FOOD":
            results = {
                "Filling": args[0],
                "Poisoned": args[1],
            }
        case "SPELLBOOK":
            results = {
                "Pages": args[0],
            }
        case "SCROLL" | "POTION":
            spells = []
            if args[1]:
                spells.append(BitFlags.get_flag(args[1], SPELLS))
            if args[2]:
                spells.append(BitFlags.get_flag(args[2], SPELLS))
            if args[3]:
                spells.append(BitFlags.get_flag(args[3], SPELLS))
            results = {
                "Level": args[0],
                "Spell": spells,
            }
        case "WAND" | "STAFF" | "INSTRUMENT":
            results = {
                "Level": args[0],
                "Max_Charges": args[1],
                "Charges_Left": args[2],
                "Spell": BitFlags.get_flag(args[3], SPELLS),
            }

        case "MONEY":
            results = {
                "Platinum": args[0],
                "Gold": args[1],
                "Silver": args[2],
                "Copper": args[3],
            }
        case "PORTAL":
            results = {
                "Destination": args[0],
                "Entry_Message": args[1],
                "Character_Message": args[2],
                "Exit_Message": args[3],
            }
        case "WALL":
            results = {
                "Direction": args[0],
                "Dispellable": args[1],
                "HitPoints": args[2],
                "Spell": args[3],
            }
        case "BOARD":
            results = {
                "Pages": args[0],
            }
        case "WEAPON":
            dice = int(args[1])
            dice_size = int(args[2])
            average = (float(dice + 1) / 2.0) * float(dice_size)
            results = {
                "HitRoll": int(args[0]),
                "Hit Dice": Dice(args[1], args[2], 0),
                "Average": average,
                "Damage Type": DAMAGE_TYPES[int(args[3])]
            }
        case "TRAP":
            results = {
                "Spell": BitFlags.get_flag(args[0], SPELLS),
                "Trap HitPoints": args[1],
            }
        case "NOTE":
            results = {
                "Tongue": args[0],
            }
        case (
            "WORN"
            | "TRASH"
            | "NOTE"
            | "OTHER"
            | "KEY"
            | "NOTHING"
            | "PEN"
            | "BOAT"
            | "ROPE"
            | "TOUCHSTONE"
            | "MISSILE"
            | "FIREWEAPON"
        ):
            pass
        case _:
            raise ValueError(f"Unknown type {item_type} Flags: {args}")
    return results
