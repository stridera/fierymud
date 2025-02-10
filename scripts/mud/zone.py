from typing import DefaultDict
from .base import Base, Dice, MudTypes
from .flags import *
from dataclasses import dataclass


class Zone(Base):
    """Construct an Item"""

    RESET_MODES = ["NEVER", "EMPTY", "NORMAL"]
    CLIMATES = [
        "CLIMATE_NONE",  # 0        /* do not report weather */
        "CLIMATE_SEMIARID",  # 1    /* plains */
        "CLIMATE_ARID",  # 2        /* deserts */
        "CLIMATE_OCEANIC",  # 3     /* ocean */
        "CLIMATE_TEMPERATE",  # 4   /* mediterranean climate */
        "CLIMATE_SUBTROPICAL",  # 5 /* florida */
        "CLIMATE_TROPICAL",  # 6    /* equatorial / jungle */
        "CLIMATE_SUBARCTIC",  # 7   /* high elevation */
        "CLIMATE_ARCTIC",  # 8      /* extreme polar */
        "CLIMATE_ALPINE",  # 9      /* mountain */
    ]

    @dataclass
    class Command:
        cmd: str
        cont: bool
        arg1: int
        arg2: int
        arg3: int
        sarg: str

    def __init__(self, vnum):
        super().__init__(vnum)
        self.type = MudTypes.ZONE

    def parse_commands(self, data):
        commands = []
        while line := data.pop(0):
            if line == "S":
                break
            (cmd, cont, arg1, arg2, arg3, sarg) = line.split(" ", 5)
            # cont = rest.pop(0) if rest else None
            # arg1 = rest.pop(0) if rest else None
            # arg2 = rest.pop(0) if rest else None
            # arg3 = rest.pop(0) if rest else None
            # sarg = rest

            command = self.Command(cmd, int(cont) == 1, int(arg1), int(arg2), int(arg3), sarg.strip())
            commands.append(command)

        return commands

    def parse_door_state(self, state):
        return [
            ["OPEN"],  # Door open
            ["CLOSED"],  # Door closed
            ["LOCKED"],  # Door locked
            ["HIDDEN"],  # Exit hidden
            ["HIDDEN", "CLOSED", "LOCKED"],  # Door hidden closed and locked
            ["HIDDEN", "CLOSED"],  # Door hidden and closed
        ][state]

    def parse(self, data):
        self.stats = {}
        self.stats["name"] = data.pop(0).rstrip("~")
        args = data.pop(0).split()
        self.stats["top"] = int(args[0])
        self.stats["lifespan"] = int(args[1])
        self.stats["reset_mode"] = self.RESET_MODES[int(args[2])]
        self.stats["zone_factor"] = int(args[3]) if len(args) > 3 else 100
        self.stats["hemisphere"] = int(args[4]) if len(args) > 4 else 0
        self.stats["climate"] = self.CLIMATES[int(args[5]) if len(args) > 5 else 0]

        """
            M: Load Mobile to room              mob_vnum, max_exist, room_vnum, (monster name)
            - E: Equip mobile with object       obj_vnum, max_exist, equip_location, (object name)
            - G: Give an object to a mobile     obj_vnum, max_exist, unused, (object name)
            O: Load Object to room              obj_vnum, max_exist, room_vnum, (object name)
            - P: Put object in another object   obj_vnum, max_exist, obj_vnum, (object name)
            R: Remove an object from the room   room_vnum, obj_vnum, (object name)
            F: Force a mobile to do...          mob_vnum, unused, unused, (command) 
            D: Open/Close/Lock a Door           room_vnum, door_direction, state, (door name)
        """
        commands = DefaultDict(list)
        steps = self.parse_commands(data)
        while steps:
            step = steps.pop(0)
            if step.cont:
                print(f"Warning, invalid continuation for step: {step}")

            match step.cmd:
                case "M":
                    mob = {"vnum": step.arg1, "max": step.arg2, "room": step.arg3, "name": step.sarg}
                    while steps and steps[0].cont and steps[0].cmd in ["G", "E"]:
                        step = steps.pop(0)
                        if step.cmd == "G":
                            if "carrying" not in mob:
                                mob["carrying"] = []
                            object = {"vnum": step.arg1, "max": step.arg2, "name": step.sarg}
                            if step.arg3 != -1:
                                print(f"Error, weird arg3 for giving an object. {step}")
                            while steps and steps[0].cmd == "P" and steps[0].cont:
                                step = steps.pop(0)
                                if "contains" not in object:
                                    object["contains"] = []
                                if object["vnum"] != step.arg3:
                                    print(f"Error, attempting to put object in object with different parent: {step}")
                                object["contains"].append(
                                    {"vnum": step.arg1, "max": step.arg2, "container": step.arg3, "name": step.sarg}
                                )
                            mob["carrying"].append(object)
                        elif step.cmd == "E":
                            if "equipped" not in mob:
                                mob["equipped"] = []
                            object = {"vnum": step.arg1, "max": step.arg2, "location": step.arg3, "name": step.sarg}
                            while steps and steps[0].cmd == "P" and steps[0].cont:
                                step = steps.pop(0)
                                if "contains" not in object:
                                    object["contains"] = []
                                if object["vnum"] != step.arg3:
                                    print(f"Error, attempting to put object in object with different parent: {step}")
                                object["contains"].append(
                                    {"vnum": step.arg1, "max": step.arg2, "container": step.arg3, "name": step.sarg}
                                )
                            mob["equipped"].append(object)
                    commands["mob"].append(mob)
                case "E" | "G":
                    print(f"Error, attempting to equip or give item w/o a mob: {step}")
                case "O":
                    object = {"vnum": step.arg1, "max": step.arg2, "room": step.arg3, "name": step.sarg}
                    while steps and steps[0].cont and steps[0].cmd == "P":
                        step = steps.pop(0)
                        if "create_objects" not in object:
                            object["create_objects"] = []
                        inside = {"vnum": step.arg1, "max": step.arg2, "container": step.arg3, "name": step.sarg}
                        if object["vnum"] != inside["container"]:
                            print(f"Error, attempting to put object in object with different parent: {step}")
                        if steps and steps[0].cmd == "P" and steps[0].cont:
                            while steps and steps[0].cmd == "P" and steps[0].cont and steps[0].arg3 == inside["vnum"]:
                                inside["contains"] = []
                                step = steps.pop(0)
                                inside_inside = {
                                    "vnum": step.arg1,
                                    "max": step.arg2,
                                    "container": step.arg3,
                                    "name": step.sarg,
                                }
                                if inside["vnum"] != inside_inside["container"]:
                                    print(f"Error, attempting to put object in object with different parent: {step}")
                                inside["contains"].append(inside_inside)
                        object["create_objects"].append(inside)
                    commands["object"].append(object)
                case "P":
                    print(f"Error, attempting to put object in object with no parent: {step}")
                case "R":
                    object = {"room": step.arg1, "vnum": step.arg2, "name": step.sarg}
                    commands["remove"].append(object)
                case "F":
                    print("Forcing a mobile to do something.")
                case "D":
                    room = step.arg1
                    door_direction = step.arg2
                    state = self.parse_door_state(step.arg3)
                    while steps and steps[0].cont and steps[0].cmd == "D":
                        step = steps.pop(0)
                        if room != step.arg1:
                            print(f"Error, attempting to open/close/lock door in a different room: {step}")
                        if door_direction != step.arg2:
                            print(f"Error, attempting to open/close/lock a different door: {step}")
                        state.append(self.parse_door_state(step.arg3))
                    door = {"room": room, "direction": DIRECTIONS[door_direction], "state": state}
                    commands["door"].append(door)
                case _:
                    print(f"Error, unknown command: {step}")

        self.stats["commands"] = commands
