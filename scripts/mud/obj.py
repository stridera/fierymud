from .base import Base, Dice, MudTypes
from .flags import *


class Obj(Base):
    """Construct an Item"""

    def __init__(self, vnum):
        super().__init__(vnum)
        self.type = MudTypes.OBJ

    def parse(self, data):
        self.stats = {}
        self.stats["namelist"] = self.read_string(data)
        self.stats["short_descr"] = self.read_string(data)
        self.stats["long_descr"] = self.read_string(data)
        self.stats["action_descr"] = self.read_string(data)

        type_flag, extra_flags, wear_flags, level = data.pop(0).split()
        f1, f2, f3, f4, f5, f6, f7 = data.pop(0).split()
        weight, cost, timer, aff1, _, _, aff2, aff3 = data.pop(0).split()
        item_types = self.get_flag(type_flag, OBJECT_TYPES)
        extra_flags_text = self.build_flags(extra_flags, EXTRA_OBJ_FLAGS)
        wear_flags_text = self.build_flags(wear_flags, WEAR_FLAGS)
        extra_stats = self.obj_val(item_types, f1, f2, f3, f4, f5, f6, f7)
        affects = (int(aff3) << 26) | (int(aff2) << int(13)) | int(aff1)
        obj_effects = self.build_flags(affects, EFFECTS)

        self.stats["type"] = item_types
        self.stats["wear_flags"] = wear_flags_text
        self.stats["object_flags"] = extra_flags_text
        self.stats["level"] = level
        self.stats["weight"] = weight
        self.stats["cost"] = cost
        self.stats["timer"] = timer
        self.stats["effects"] = obj_effects
        self.stats["stats"] = extra_stats

        while data:
            line = data.pop(0)
            if line.startswith("E"):
                keyword = self.read_string(data)
                extra_desc = self.read_string(data)
                if "extra_desc" not in self.stats:
                    self.stats["extra_desc"] = []
                self.stats["extra_desc"].append({"keyword": keyword, "desc": extra_desc})
            elif line.startswith("A"):
                location, modifier = data.pop(0).strip().split(" ")
                if "affects" not in self.stats:
                    self.stats["affects"] = []
                self.stats["affects"].append({"location": self.get_flag(location, AFFECTS), "modifier": int(modifier)})
            elif line.startswith("H"):
                hiddenness = data.pop(0).strip()
                self.stats["hiddenness"] = hiddenness
            elif line.startswith("T"):
                if "triggers" not in self.stats:
                    self.stats["triggers"] = []
                self.stats["triggers"].append(int(line[2:]))
            elif line.startswith("X"):
                if "object_flags" not in self.stats:
                    self.stats["object_flags"] = ""
                self.stats["object_flags"] += self.build_flags(data.pop(0).strip(), EXTRA_OBJ_FLAGS, 32)
            elif line.startswith(("#", "$")):
                break
            else:
                raise Exception(f"Unknown Extra Flags: {line}")

    @staticmethod
    def obj_val(itype: str, *args):
        """Return the object values for the given item type."""
        results = {}
        match itype:
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
                    "flags": Obj.build_flags(args[1], ["Closeable", "Pickproof", "Closed", "Locked"]),
                    "Key": args[2],
                    "IsCorpse": args[3],
                    "Weight Reduction": args[4],
                }
            case "DRINKCON" | "FOUNTAIN":
                results = {
                    "Capacity": args[0],
                    "Remaining": args[1],
                    "Liquid": Obj.get_flag(args[2], LIQUIDS),
                    "Poisoned": args[3],
                }
            case "FOOD":
                results = {
                    "Fillingness": args[0],
                    "Poisoned": args[1],
                }
            case "SPELLBOOK":
                results = {
                    "Pages": args[0],
                }
            case "SCROLL" | "POTION":
                results = {
                    "Level": args[0],
                    "Spell 1": Obj.get_flag(args[1], SPELLS),
                    "Spell 2": Obj.get_flag(args[2], SPELLS),
                    "Spell 3": Obj.get_flag(args[3], SPELLS),
                }
            case "WAND" | "STAFF" | "INSTRUMENT":
                results = {
                    "Level": args[0],
                    "Max_Charges": args[1],
                    "Charges_Left": args[2],
                    "Spell": Obj.get_flag(args[3], SPELLS),
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
                    "Dispelable": args[1],
                    "Hitpoints": args[2],
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
                    "Hitroll": args[0],
                    "Hit Dice": Dice(args[1], args[2], 0),
                    "Average": average,
                    "Damage Type": Obj.get_flag(args[3], DAMAGE_TYPES),
                }
            case "TRAP":
                results = {
                    "Spell": Obj.get_flag(args[0], SPELLS),
                    "Trap Hitpoints": args[1],
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
                raise ValueError(f"Unknown type {itype} Flags: {args}")
        return results
