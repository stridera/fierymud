import re
from dataclasses import dataclass

from mud.bitflags import BitFlags
from mud.flags import EFFECTS, MOB_FLAGS
from mud.mudfile import MudData
from mud.types import (
    Class,
    Composition,
    DamageType,
    Dice,
    Gender,
    LifeForce,
    Money,
    Position,
    Race,
    Size,
    Stance,
    Stats,
)


@dataclass
class Mob:
    id: int
    keywords: str
    mob_class: str
    short_desc: str
    long_desc: str
    desc: str
    mob_flags: list[str]
    effect_flags: list[str]
    alignment: int
    level: int
    hp_dice: Dice
    move: int
    ac: int
    hit_roll: int
    damage_dice: Dice
    money: Money
    position: Position
    default_position: Position
    gender: Gender
    mob_class: Class
    race: Race
    race_align: int
    size: Size
    effect_flags: list[str]
    effect_flags: list[str]
    mob_flags: list[str]
    stats: Stats
    perception: int
    concealment: int
    life_force: LifeForce
    composition: Composition
    stance: Stance
    damage_type: DamageType = DamageType.HIT

    @classmethod
    def parse(cls, mob_file: MudData):
        mobs = []
        for mob_data in mob_file.split_by_delimiter():
            mob = {}
            line = mob_data.get_next_line()
            if line.startswith("*"):
                continue
            mob["id"] = int(line.lstrip("#"))
            mob["keywords"] = mob_data.read_string()
            mob["short_desc"] = mob_data.read_string()
            mob["long_desc"] = mob_data.read_string()
            mob["desc"] = mob_data.read_string()

            mob_flags, effect_flags, align, _ = mob_data.get_next_line().split()
            mob["mob_flags"] = BitFlags.read_flags(mob_flags, MOB_FLAGS)
            mob["effect_flags"] = BitFlags.read_flags(effect_flags, EFFECTS)
            mob["alignment"] = int(align)

            level, hit_roll, ac, hp_dice, dam_dice = mob_data.get_next_line().split()
            hp_dice_number, hp_dice_sides, move = re.split(r"[+d]", hp_dice)
            mob["level"] = int(level)
            mob["hit_roll"] = int(hit_roll)
            mob["ac"] = int(ac)
            mob["hp_dice"] = Dice(int(hp_dice_number), int(hp_dice_sides), 0)
            mob["move"] = int(move)
            mob["damage_dice"] = Dice.from_string(dam_dice)

            fields = mob_data.get_next_line().split()
            if len(fields) == 6:
                mob["money"] = Money(int(fields[0]), int(fields[1]), int(fields[2]), int(fields[3]))
            if len(fields) == 4:
                mob["money"] = Money(int(fields[0]), int(fields[1]), 0, 0)

            (position, default_position, gender, class_num, race, race_align, size) = mob_data.get_next_line().split()

            mob["position"] = int(position)
            mob["default_position"] = int(default_position)
            mob["gender"] = int(gender)
            mob["mob_class"] = Class(int(class_num))
            mob["race"] = int(race)
            mob["race_align"] = int(race_align)
            mob["size"] = int(size)

            # End of static data, now we read the dynamic data
            stats = {}
            while kv := mob_data.read_key_value():
                key, value = kv
                match key:
                    case "Str":
                        stats["strength"] = int(value)
                    case "Int":
                        stats["intelligence"] = int(value)
                    case "Wis":
                        stats["wisdom"] = int(value)
                    case "Dex":
                        stats["dexterity"] = int(value)
                    case "Con":
                        stats["constitution"] = int(value)
                    case "Cha":
                        stats["charisma"] = int(value)
                    case "AFF2":
                        mob["effect_flags"].set_flags(value, 32)
                    case "AFF3":
                        mob["effect_flags"].set_flags(value, 64)
                    case "MOB2":
                        mob["mob_flags"].set_flags(value, 32)
                    case "PERC":
                        mob["perception"] = int(value)
                    case "HIDE":
                        mob["concealment"] = int(value)
                    case "Lifeforce":
                        mob["life_force"] = LifeForce(int(value))
                    case "Composition":
                        mob["composition"] = Composition(int(value))
                    case "Stance":
                        mob["stance"] = Stance(int(value))
                    case "BareHandAttack":
                        mob["damage_type"] = DamageType(int(value))
                    case "E":
                        break
                    case _:
                        print(f"Unknown key {key} in mob {mob['id']}")
            mob["stats"] = Stats(**stats)
            mobs.append(cls(**mob))
        return mobs
