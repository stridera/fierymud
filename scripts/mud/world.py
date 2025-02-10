from .base import Base, Dice, MudTypes
from .flags import *


class World(Base):
    """Construct an Item"""

    def __init__(self, vnum):
        super().__init__(vnum)
        self.type = MudTypes.WORLD

    def parse(self, data):
        self.stats = {}
        self.stats["name"] = self.read_string(data)
        self.stats["description"] = self.read_string(data)
        (_zone, flags, sector) = data.pop(0).split()
        self.stats["flags"] = self.read_flags(flags, ROOM_FLAGS)
        self.stats["sector"] = sector
        while data:
            line = data.pop(0)
            if line.startswith("S"):
                break
            elif line.startswith("D"):
                if "exits" not in self.stats:
                    self.stats["exits"] = {}
                exit = {}
                direction = DIRECTIONS[int(line[1:])]
                exit["description"] = self.read_string(data)
                exit["keyword"] = self.read_string(data)
                exit_type, key, destination = data.pop(0).split()
                if exit_type == "1":
                    exit["type"] = "Door"
                elif exit_type == "2":
                    exit["type"] = "Pickproof Door"
                elif exit_type == "3":
                    exit["type"] = "Description only"
                exit["key"] = key
                exit["destination"] = destination
                if direction in self.stats["exits"]:
                    print(f"Duplicate exit for direction {direction} in room {self.vnum}")
                self.stats["exits"][direction] = exit
            elif line.startswith("E"):
                if "extra_descriptions" not in self.stats:
                    self.stats["extra_descriptions"] = {}
                keyword = self.read_string(data)
                description = self.read_string(data)
                self.stats["extra_descriptions"][keyword] = description

            else:
                break
