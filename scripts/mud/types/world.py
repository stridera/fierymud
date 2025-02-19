from dataclasses import dataclass

from mud.bitflags import BitFlags
from mud.flags import ROOM_FLAGS
from mud.mudfile import MudData
from mud.types import Direction


@dataclass
class Room:
    name: str
    description: str
    sector: str
    flags: list[str]
    affects: dict[str, str] | None = None
    exits: dict[str, dict[str, str]] | None = None
    extra_descriptions: dict[str, str] | None = None


@dataclass
class World:
    """Construct an World"""

    rooms: list[Room]

    @classmethod
    def parse(cls, world_file: MudData):
        rooms = []
        for room_data in world_file.split_by_delimiter():
            room = {}
            room["name"] = room_data.read_string()
            room["description"] = room_data.read_string()
            (_zone, flags, sector) = room_data.get_next_line().split()
            room["flags"] = BitFlags.read_flags(flags, ROOM_FLAGS)
            room["sector"] = sector
            while line := room_data.get_next_line():
                if line == "S":
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
                        print(f"Duplicate exit for direction {direction} in room {room_data.vnum}")
                    room["exits"][direction] = exit
                elif line.startswith("E"):
                    if "extra_descriptions" not in room:
                        room["extra_descriptions"] = {}
                    keyword = room_data.read_string()
                    description = room_data.read_string()
                    room["extra_descriptions"][keyword] = description
                else:
                    print(f"Unknown line: {line}")

            rooms.append(Room(**room))

        return cls(rooms)
