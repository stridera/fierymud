from .base import Base, Dice, MudTypes
from .flags import MOB_FLAGS, EFFECTS
import re


class Mob(Base):
    def __init__(self, vnum):
        super().__init__(vnum)
        self.type = MudTypes.MOB

    def parse(self, data):
        self.stats = {}
        self.stats["namelist"] = self.read_string(data)
        self.stats["short_descr"] = self.read_string(data)
        self.stats["long_descr"] = self.read_string(data)
        self.stats["description"] = self.read_string(data)

        mob_flags, effect_flags, align, _ = data.pop(0).split()
        self.stats["mob_flags"] = self.read_flags(mob_flags, MOB_FLAGS)
        self.stats["effect_flags"] = self.read_flags(effect_flags, EFFECTS)
        self.stats["alignment"] = int(align)

        (level, hitroll, ac, hp_num_dice, hp_size_dice, move, dam_num_dice,
         dam_size_dice, dam_roll_bonus) = re.split(r"[ d+]", data.pop(0))
        self.stats["level"] = int(level)
        self.stats["hitroll"] = int(hitroll)
        self.stats["ac"] = int(ac)
        self.stats["hp_dice"] = Dice(int(hp_num_dice), int(hp_size_dice), 0)
        self.stats["move"] = int(move)
        self.stats["dam_dice"] = Dice(int(dam_num_dice), int(dam_size_dice), int(dam_roll_bonus))

        gold, plat, exp, zone = data.pop(0).split()
        self.stats["gold"] = int(gold)
        self.stats["plat"] = int(plat)
        self.stats["exp"] = int(exp)
        self.stats["zone"] = int(zone)

        (position, default_position, gender, class_num, race, race_align, size) = data.pop(0).split()

        self.stats["position"] = int(position)
        self.stats["default_position"] = int(default_position)
        self.stats["gender"] = int(gender)
        self.stats["class_num"] = int(class_num)
        self.stats["race"] = int(race)
        self.stats["race_align"] = int(race_align)
        self.stats["size"] = int(size)

        # End of static data, now we read the dynamic data
        for line in data:
            if line.startswith("AFF2"):
                self.stats["effect_flags"].set_flags(line.split()[1], 32)
            elif line.startswith("AFF3"):
                self.stats["effect_flags"].set_flags(line.split()[1], 64)
            elif line.startswith("MOB2"):
                self.stats["mob_flags"].set_flags(line.split()[1], 32)

            elif line.startswith("E"):
                break
            else:
                key, value = line.split()
                self.stats[key[:-1]] = value
