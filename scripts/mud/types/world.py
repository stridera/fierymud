from dataclasses import dataclass
from enum import Enum

from mud.bitflags import BitFlags
from mud.flags import ROOM_FLAGS
from mud.mudfile import MudData
from mud.types import Direction

class Sectors(Enum):
    STRUCTURE = 0 # A building of some kind
    CITY = 1       # In a city
    FIELD = 2      # In a field
    FOREST = 3     # In a forest
    HILLS = 4      # In the hills
    MOUNTAIN = 5   # On a mountain
    SHALLOWS = 6   # Easily passable water
    WATER = 7      # Water - need a boat
    UNDERWATER = 8 # Underwater
    AIR = 9        # Wheee!
    ROAD = 10
    GRASSLANDS = 11
    CAVE = 12
    RUINS = 13
    SWAMP = 14
    BEACH = 15
    UNDERDARK = 16
    ASTRALPLANE = 17
    AIRPLANE = 18
    FIREPLANE = 19
    EARTHPLANE = 20
    ETHEREALPLANE = 21
    AVERNUS = 22


@dataclass
class World:
    """Construct an World"""

    id: int
    name: str
    description: str
    sector: Sectors
    flags: list[str]
    exits: dict[str, dict[str, str]] | None = None
    extra_descriptions: dict[str, str] | None = None

    @classmethod
    def parse(cls, world_file: MudData):
        rooms = []
        for room_data in world_file.split_by_delimiter():
            room = {}
            room["id"] = room_data.get_next_line().lstrip("#")
            room["name"] = room_data.read_string()
            room["description"] = room_data.read_string()
            (_zone, flags, sector) = room_data.get_next_line().split()
            room["flags"] = BitFlags.read_flags(flags, ROOM_FLAGS)
            room["sector"] = Sectors(int(sector))
            while line := room_data.get_next_line():
                if line == "S" or line.startswith("$"):
                    break
                elif line.startswith("D"):
                    if "exits" not in room:
                        room["exits"] = {}
                    exit = {}
                    direction = Direction(int(line[1:])).name
                    exit["description"] = room_data.read_string()
                    exit["keyword"] = room_data.read_string()
                    exit_type, key, destination = room_data.get_next_line().split()
                    if exit_type == "1":
                        exit["type"] = "Door"
                    elif exit_type == "2":
                        exit["type"] = "Pick-proof Door"
                    elif exit_type == "3":
                        exit["type"] = "Description only"
                    exit["key"] = key
                    exit["destination"] = destination
                    if direction in room["exits"]:
                        print(f"Duplicate exit for direction {direction} in room {room_data.id}")
                    room["exits"][direction] = exit
                elif line.startswith("E"):
                    if "extra_descriptions" not in room:
                        room["extra_descriptions"] = {}
                    keyword = room_data.read_string()
                    description = room_data.read_string()
                    room["extra_descriptions"][keyword] = description
                else:
                    print(f"Unknown line: {line}")

            rooms.append(room)

        return rooms

    def to_json(self):
        from dataclasses import asdict

        return asdict(self)
