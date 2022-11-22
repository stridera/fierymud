from .mudobject import MudObject, MudTypes
from .flags import MOB_FLAGS, EFFECTS
import re


class Mob(MudObject):
    def __init__(self, vnum):
        super().__init__(vnum)
        self.type = MudTypes.MOB

    def parse(self, data):
        self.stats = {}
        self.stats['namelist'] = self.readString(data)
        self.stats['short_descr'] = self.readString(data)
        self.stats['long_descr'] = self.readString(data)
        self.stats['description'] = self.readString(data)

        mob_flags, effect_flags, align, _ = data.pop(0).split()
        self.stats['mob_flags'] = self.readFlags(mob_flags, MOB_FLAGS)
        self.stats['effect_flags'] = self.readFlags(effect_flags, EFFECTS)
        self.stats['alignment'] = int(align)

        level, hitroll, ac, hp_num_dice, hp_size_dice, move, \
            dam_num_dice, dam_size_dice, dam_roll_bonus = re.split(r'[ d+]', data.pop(0))
        self.stats['level'] = int(level)
        self.stats['hitroll'] = int(hitroll)
        self.stats['ac'] = int(ac)
        self.stats['hp_num_dice'] = int(hp_num_dice)
        self.stats['hp_size_dice'] = int(hp_size_dice)
        self.stats['move'] = int(move)
        self.stats['extra_dam_num_dice'] = int(dam_num_dice)
        self.stats['extra_dam_size_dice'] = int(dam_size_dice)
        self.stats['extra_dam_roll_bonus'] = int(dam_roll_bonus)

        gold, plat, exp, zone = data.pop(0).split()
        self.stats['gold'] = int(gold)
        self.stats['plat'] = int(plat)
        self.stats['exp'] = int(exp)
        self.stats['zone'] = int(zone)

        position, default_position, gender, class_num, race, race_align, size = data.pop(0).split()

        self.stats['position'] = int(position)
        self.stats['default_position'] = int(default_position)
        self.stats['gender'] = int(gender)
        self.stats['class_num'] = int(class_num)
        self.stats['race'] = int(race)
        self.stats['race_align'] = int(race_align)
        self.stats['size'] = int(size)

        # End of static data, now we read the dynamic data
        for line in data:
            if line.startswith('AFF2'):
                self.stats['AFF2'] = self.readFlags(line.split()[1], EFFECTS, 32)
            elif line.startswith('AFF3'):
                self.stats['AFF3'] = self.readFlags(line.split()[1], EFFECTS, 64)
            elif line.startswith('MOB2'):
                self.stats['MOB2'] = self.readFlags(line.split()[1], MOB_FLAGS, 32)
            elif line.startswith('E'):
                break
            else:
                k, v = line.split()
                self.stats[k[:-1]] = v


'''
#3000
bigby wizard shopkeeper~
Bigby~
Bigby walks around behind the counter, muttering powerful incantations.
~
Bigby looks to be very old, but he is far from senile.  The raw magical power
that emanates from him is almost frightening, but he seems to be somewhat less
than concerned by the likes of you.  Best to not make him notice you - just buy
what you came in for and then leave him alone.    
~
-2147358694 8 900 E
99 20 0 0d0+0 0d0+0
30 0 0 30
3 3 1 0 13 0 2
Str: 30
Dex: 30
Int: 30
Wis: 30
Con: 30
Cha: 30
AFF2: 0
AFF3: 0
MOB2: 8
PERC: 0
HIDE: 0
Lifeforce: 0
Composition: 0
Stance: 6
E

'''
