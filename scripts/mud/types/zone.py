from dataclasses import dataclass
from enum import Enum
from typing import DefaultDict

from mud.mudfile import MudData
from mud.types import Direction


class ResetModes(Enum):
    Never = 0
    Empty = 1
    Normal = 2


class Hemispheres(Enum):
    NORTHWEST = 0
    NORTHEAST = 1
    SOUTHWEST = 2
    SOUTHEAST = 3

class Climates(Enum):
    NONE = 0  #        /* do not report weather */
    SEMIARID = 1  #    /* plains */
    ARID = 2  #        /* deserts */
    OCEANIC = 3  #     /* ocean */
    TEMPERATE = 4  #   /* mediterranean climate */
    SUBTROPICAL = 5  # /* florida */
    TROPICAL = 6  #    /* equatorial / jungle */
    SUBARCTIC = 7  #   /* high elevation */
    ARCTIC = 8  #      /* extreme polar */
    ALPINE = 9  #      /* mountain */


@dataclass
class Command:
    cmd: str
    cont: bool
    arg1: int
    arg2: int
    arg3: int
    sarg: str


@dataclass
class Zone:
    id: int
    name: str
    top: int
    lifespan: int
    reset_mode: ResetModes
    hemisphere: Hemispheres
    climate: Climates
    commands: list[Command]

    @classmethod
    def parse_commands(cls, zone_file: MudData):
        commands = []
        for line in zone_file.read_until_starts("S"):
            (cmd, cont, arg1, arg2, arg3, sarg) = line.split(" ", 5)
            command = Command(cmd, int(cont) == 1, int(arg1), int(arg2), int(arg3), sarg.strip())
            commands.append(command)

        return commands

    @staticmethod
    def parse_door_state(state):
        return [
            ["OPEN"],  # Door open
            ["CLOSED"],  # Door closed
            ["LOCKED"],  # Door locked
            ["HIDDEN"],  # Exit hidden
            ["HIDDEN", "CLOSED", "LOCKED"],  # Door hidden closed and locked
            ["HIDDEN", "CLOSED"],  # Door hidden and closed
        ][state]

    @classmethod
    def parse(cls, zone_data: MudData):
        zone = {}
        zone["id"] = zone_data.get_next_line().lstrip("#")
        zone["name"] = zone_data.read_string()
        args = zone_data.get_next_line().split()
        zone["top"] = int(args[0])
        zone["lifespan"] = int(args[1])
        zone["reset_mode"] = ResetModes(int(args[2]))
        zone["hemisphere"] = Hemispheres(int(args[4]) if len(args) > 4 else 0)
        zone["climate"] = Climates(int(args[5]) if len(args) > 5 else 0)

        """
            M: Load Mobile to room              mob_id, max_exist, room_id, (monster name)
            - E: Equip mobile with object       obj_id, max_exist, equip_location, (object name)
            - G: Give an object to a mobile     obj_id, max_exist, unused, (object name)
            O: Load Object to room              obj_id, max_exist, room_id, (object name)
            - P: Put object in another object   obj_id, max_exist, obj_id, (object name)
            R: Remove an object from the room   room_id, obj_id, (object name)
            F: Force a mobile to do...          mob_id, unused, unused, (command) 
            D: Open/Close/Lock a Door           room_id, door_direction, state, (door name)
        """
        commands = DefaultDict(list)
        steps = cls.parse_commands(zone_data)
        while steps:
            step = steps.pop(0)
            if step.cont:
                print(f"Warning, invalid continuation for step: {step}")

            match step.cmd:
                case "M":
                    mob = {"id": step.arg1, "max": step.arg2, "room": step.arg3, "name": step.sarg}
                    while steps and steps[0].cont and steps[0].cmd in ["G", "E"]:
                        step = steps.pop(0)
                        if step.cmd == "G":
                            if "carrying" not in mob:
                                mob["carrying"] = []
                            object = {"id": step.arg1, "max": step.arg2, "name": step.sarg}
                            if step.arg3 != -1:
                                print(f"Error, weird arg3 for giving an object. {step}")
                            while steps and steps[0].cmd == "P" and steps[0].cont:
                                step = steps.pop(0)
                                if "contains" not in object:
                                    object["contains"] = []
                                if object["id"] != step.arg3:
                                    print(f"Error, attempting to put object in object with different parent: {step}")
                                object["contains"].append(
                                    {"id": step.arg1, "max": step.arg2, "container": step.arg3, "name": step.sarg}
                                )
                            mob["carrying"].append(object)
                        elif step.cmd == "E":
                            if "equipped" not in mob:
                                mob["equipped"] = []
                            object = {"id": step.arg1, "max": step.arg2, "location": step.arg3, "name": step.sarg}
                            while steps and steps[0].cmd == "P" and steps[0].cont:
                                step = steps.pop(0)
                                if "contains" not in object:
                                    object["contains"] = []
                                if object["id"] != step.arg3:
                                    print(f"Error, attempting to put object in object with different parent: {step}")
                                object["contains"].append(
                                    {"id": step.arg1, "max": step.arg2, "container": step.arg3, "name": step.sarg}
                                )
                            mob["equipped"].append(object)
                    commands["mob"].append(mob)
                case "E" | "G":
                    print(f"Error, attempting to equip or give item w/o a mob: {step}")
                case "O":
                    object = {"id": step.arg1, "max": step.arg2, "room": step.arg3, "name": step.sarg}
                    while steps and steps[0].cont and steps[0].cmd == "P":
                        step = steps.pop(0)
                        if "create_objects" not in object:
                            object["create_objects"] = []
                        inside = {"id": step.arg1, "max": step.arg2, "container": step.arg3, "name": step.sarg}
                        if object["id"] != inside["container"]:
                            print(f"Error, attempting to put object in object with different parent: {step}")
                        if steps and steps[0].cmd == "P" and steps[0].cont:
                            while steps and steps[0].cmd == "P" and steps[0].cont and steps[0].arg3 == inside["id"]:
                                inside["contains"] = []
                                step = steps.pop(0)
                                inside_inside = {
                                    "id": step.arg1,
                                    "max": step.arg2,
                                    "container": step.arg3,
                                    "name": step.sarg,
                                }
                                if inside["id"] != inside_inside["container"]:
                                    print(f"Error, attempting to put object in object with different parent: {step}")
                                inside["contains"].append(inside_inside)
                        object["create_objects"].append(inside)
                    commands["object"].append(object)
                case "P":
                    print(f"Error, attempting to put object in object with no parent: {step}")
                case "R":
                    object = {"room": step.arg1, "id": step.arg2, "name": step.sarg}
                    commands["remove"].append(object)
                case "F":
                    print("Forcing a mobile to do something.")
                case "D":
                    room = step.arg1
                    door_direction = step.arg2
                    state = cls.parse_door_state(step.arg3)
                    while steps and steps[0].cont and steps[0].cmd == "D":
                        step = steps.pop(0)
                        if room != step.arg1:
                            print(f"Error, attempting to open/close/lock door in a different room: {step}")
                        if door_direction != step.arg2:
                            print(f"Error, attempting to open/close/lock a different door: {step}")
                        state.append(cls.parse_door_state(step.arg3))
                    door = {"room": room, "direction": Direction(door_direction).name, "state": state}
                    commands["door"].append(door)
                case _:
                    print(f"Error, unknown command: {step}")

        zone["commands"] = commands

        return cls(**zone)

    def to_json(self):
        from dataclasses import asdict

        d = asdict(self)
        # commands is a DefaultDict(list) of lists of dicts; ensure it's JSON serializable
        d["commands"] = {k: v for k, v in self.commands.items()}
        if hasattr(self.reset_mode, "name"):
            d["reset_mode"] = self.reset_mode.name
        if hasattr(self.climate, "name"):
            d["climate"] = self.climate.name
        return d
