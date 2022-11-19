from .mudobject import MudObject, MudTypes


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
