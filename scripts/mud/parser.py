import ipaddress
from collections import defaultdict
from datetime import datetime

from mud.bitflags import BitFlags
from mud.flags import (
    EFFECTS,
    OBJECT_FLAGS,
    OBJECT_TYPES,
    PLAYER_FLAGS,
    PREFERENCE_FLAGS,
    PRIVILEGE_FLAGS,
    SKILLS,
    SPELLS,
    WEAR_FLAGS,
)
from mud.mudfile import MudData
from mud.types import (
    ApplyTypes,
    Class,
    Composition,
    Cooldown,
    CurrentMax,
    Gender,
    KillTypeFlags,
    LifeForce,
    MudTypes,
    QuitReason,
    Race,
    SavingThrows,
    obj_val,
)


class Parser:
    def __init__(self, type: MudTypes):
        self.type = type

    def read_applies(self, mudfile: MudData) -> dict[ApplyTypes, int]:
        applies = defaultdict()
        data = mudfile.read_until_starts("~")
        for line in data:
            apply, val = [int(x) for x in line.split()]
            applies[str(ApplyTypes(apply))] = val
        return applies

    def parse_flags(self, flags) -> list:
        match self.type:
            case MudTypes.OBJECT:
                list(BitFlags.read_flag_list(flags, OBJECT_FLAGS))

    def parse(self, mudfile: MudData) -> dict[str, any]:
        prototype = defaultdict()
        while kv := mudfile.read_key_value():
            key, value = kv
            match key:
                case "ac":
                    prototype["armor_class"] = int(value)
                case "adesc":
                    prototype["action_description"] = mudfile.read_string()
                case "aggression":
                    prototype["aggression"] = int(value)
                case "aliases":
                    prototype["aliases"] = {}
                    for line in mudfile.read_until_match("0"):
                        if line.count(" ") > 0:
                            alias, command = line.split(" ", 1)
                            prototype["aliases"][alias] = command.strip()
                case "alignment":
                    prototype["alignment"] = int(value)
                case "applies":
                    prototype["applies"] = self.read_applies(mudfile)
                case "autoinvis":
                    prototype["auto_invis"] = int(value)
                case "badpasswords":
                    prototype["bad_passwords"] = int(value)
                case "base_height":
                    prototype["base_height"] = int(value)
                case "base_size":
                    prototype["base_size"] = int(value)
                case "base_weight":
                    prototype["base_weight"] = int(value)
                case "birthtime":
                    prototype["birth_time"] = datetime.fromtimestamp(int(value))
                case "cash" | "bank":
                    plat, gold, silver, copper = value.split()
                    prototype["money" if key == "cash" else "bank"] = {
                        "plat": int(plat),
                        "gold": int(gold),
                        "silver": int(silver),
                        "copper": int(copper),
                    }
                case "clan":
                    prototype["clan"] = value
                case "class":
                    prototype["player_class"] = Class(int(value))
                case "composition":
                    prototype["composition"] = Composition(int(value))
                case "cooldowns":
                    prototype["cooldowns"] = {}
                    for line in mudfile.read_until_starts("-1 0"):
                        cooldown, timedata = line.split()
                        cooldown = str(Cooldown(int(cooldown)))
                        time, max = timedata.split("/")
                        prototype["cooldowns"][cooldown] = {"time": time, "max": max}
                case "cost":
                    prototype["cost"] = int(value)
                case "current_title" | "currenttitle":
                    prototype["current_title"] = value
                case "damroll":
                    prototype["damage_roll"] = int(value)
                case "decomp":
                    prototype["decompose_timer"] = int(value)
                case "desc" | "description":
                    if value == "":
                        prototype["description"] = mudfile.read_string()
                    else:
                        prototype["description"] = value
                case "drunkenness":
                    prototype["drunkenness"] = int(value)
                case "effects":
                    if value == "":
                        prototype["effects"] = []
                        for line in mudfile.read_until_starts("0 0 0"):
                            effect_type, duration, modifier, location, flags = line.split(" ", 4)
                            prototype["effects"].append(
                                {
                                    "type": SKILLS[int(effect_type)],
                                    "duration": int(duration),
                                    "modifier": int(modifier),
                                    "location": int(location),
                                    "flags": flags,
                                }
                            )
                    else:
                        if "effect_flags" not in prototype:
                            prototype["effect_flags"] = []
                        prototype["effect_flags"] += list(BitFlags.read_flag_list(value, EFFECTS))
                case "effectflags":
                    if "effect_flags" not in prototype:
                        prototype["effect_flags"] = []
                    prototype["effect_flags"] += list(list(BitFlags.read_flag_list(value, EFFECTS)))
                case "experience":
                    prototype["experience"] = int(value)
                case "extradesc":
                    if "extra_descriptions" not in prototype:
                        prototype["extra_descriptions"] = defaultdict()
                    prototype["extra_descriptions"][value] = mudfile.read_string()
                case "flags":
                    prototype["flags"] = self.parse_flags(value)
                case "freezelevel":
                    prototype["freeze_level"] = int(value)
                case "gossips" | "tells":
                    prototype[key] = []
                    for line in mudfile.read_until_starts("$"):
                        if line == "":
                            continue
                        timestamp, comment = line.split(" ", 1)
                        prototype[key].append([datetime.fromtimestamp(int(timestamp)), comment])
                case "grantgroups":
                    prototype["grant_groups"] = []
                    for line in mudfile.read_until_starts("~"):
                        command, grantor, level = line.split()
                        prototype["grant_groups"].append({"command": command, "granted_by": grantor, "level": level})
                case "grants":
                    prototype["grants"] = []
                    for line in mudfile.read_until_starts("~"):
                        command, grantor, level = line.split()
                        prototype["grants"].append({"command": command, "granted_by": grantor, "level": level})
                case "height":
                    prototype["height"] = int(value)
                case "hiddenness" | "concealment":
                    prototype["concealment"] = int(value)
                case "hitpoints":
                    prototype["hit_points"] = CurrentMax(*[int(i) for i in value.split("/")])
                case "hitroll":
                    prototype["hit_roll"] = int(value)
                case "home":
                    prototype["home"] = value
                case "host":
                    value = ".".join([str(int(oct)) for oct in value.split(".")])
                    prototype["host"] = ipaddress.IPv4Address(value)
                case "hunger" | "thirst":
                    prototype[key] = int(value)
                case "id":
                    prototype["id"] = int(value)
                case "invislevel":
                    prototype["invis_level"] = int(value)
                case "lastlevel":
                    prototype["last_level"] = int(value)
                case "lastlogintime":
                    prototype["last_login_time"] = datetime.fromtimestamp(int(value))
                case "level":
                    prototype["level"] = int(value)
                case "lifeforce":
                    prototype["life_force"] = LifeForce(int(value))
                case "loadroom":
                    prototype["load_room"] = int(value)
                case "location":
                    prototype["location"] = int(value)
                case "logview":
                    prototype["log_view"] = int(value)
                case "mana":
                    pass
                case "mem":
                    # We no longer have memorized spells.  Skip.
                    mudfile.read_until_starts("0 0")
                case "move":
                    prototype["move"] = CurrentMax(*[int(i) for i in value.split("/")])
                case "name":
                    if self.type == MudTypes.PLAYER:
                        prototype["name"] = value
                    else:
                        prototype["name_list"] = value
                case "name_list":
                    prototype["name_list"] = value
                case "natural_size":
                    prototype["natural_size"] = int(value)
                case "olczones":
                    prototype["olc_zones"] = [int(i) for i in value.split()]
                case "pagelength":
                    prototype["page_length"] = int(value)
                case "password":
                    prototype["password"] = value
                case "playerflags":
                    prototype["player_flags"] = list(BitFlags.read_flags(value, PLAYER_FLAGS))
                case "poofin":
                    prototype["poof_in"] = value
                case "poofout":
                    prototype["poof_out"] = value
                case "prefflags":
                    prototype["preference_flags"] = list(BitFlags.read_flag_list(value, PREFERENCE_FLAGS))
                case "prompt":
                    prototype["prompt"] = value
                case "privflags":
                    prototype["privilege_flags"] = list(BitFlags.read_flags(value, PRIVILEGE_FLAGS))
                case "quit_reason":
                    prototype["quit_reason"] = QuitReason(int(value))
                case "race":
                    prototype["races"] = Race(int(value))
                case "savingthrows":
                    prototype["saving_throws"] = SavingThrows(*[int(i) for i in value.split()])
                case "saveroom":
                    prototype["save_room"] = int(value)
                case "sex":
                    prototype["gender"] = Gender(int(value))
                case "shortdesc":
                    prototype["short_description"] = value
                case "size":
                    prototype["size"] = int(value)
                case "skills":
                    prototype["skills"] = {}
                    for line in mudfile.read_until_starts("0 0"):
                        skill, level = line.split()
                        skill_name = SKILLS[int(skill)]
                        prototype["skills"][skill_name] = int(level)
                case "spells":
                    if "spells" not in prototype:
                        prototype["spells"] = defaultdict()
                    for line in mudfile.read_until_starts("~"):
                        spell, pages = line.split()
                        if int(spell) >= len(SPELLS):
                            print(f"Unknown spell number: {spell}")
                            continue
                        spell_name = SPELLS[int(spell)]
                        prototype["spells"][spell_name] = int(pages)
                case "spellcasts":
                    prototype["spell_casts"] = {}
                    for line in mudfile.read_until_starts("0 0"):
                        spell, count = line.split()
                        spell_num = int(spell)
                        if spell_num >= len(SPELLS):
                            raise ValueError(f"Unknown spell number: {spell_num}.")
                        prototype["spell_casts"][SPELLS[spell_num]] = count
                case "strength" | "dexterity" | "constitution" | "intelligence" | "wisdom" | "charisma":
                    if "stats" not in prototype:
                        prototype["stats"] = {}
                    prototype["stats"][key] = int(value)
                case "title":
                    prototype["title"] = value
                case "timeplayed":
                    prototype["time_played"] = int(value)
                case "timer":
                    prototype["timer"] = int(value)
                case "triggers":
                    prototype["triggers"] = mudfile.read_to_list("~", int)
                case "trophy":
                    prototype["trophy"] = []
                    for line in mudfile.read_until_starts("0 0"):
                        kill_type, id, count = line.split()
                        prototype["trophy"].append(
                            {"type": KillTypeFlags(int(kill_type)), "id": int(id), "count": float(count)}
                        )
                case "type":
                    prototype["type"] = BitFlags.get_flag(int(value), OBJECT_TYPES)
                case "values":
                    prototype["values"] = mudfile.read_to_list("~", int)
                case "variables":
                    prototype["script_variables"] = {}
                    for line in mudfile.read_until_starts("~"):
                        key, value = line.split(" ", 1)
                        prototype["script_variables"][key] = value
                case "id":
                    prototype["id"] = int(value)
                case "wear":
                    prototype["wear_flags"] = list(list(BitFlags.read_flags(value, WEAR_FLAGS)))
                case "weight":
                    prototype["weight"] = float(value)
                case "wimpy":
                    prototype["wimpy"] = int(value)
                case "wiztitle":
                    prototype["wiz_title"] = value
                case "~~":
                    pass  # Player Object Delimiter.
                case _:
                    raise ValueError(f"Invalid data: {key} = {value}")

        if "type" in prototype and "values" in prototype:
            prototype["values"] = obj_val(prototype["type"], *prototype["values"])

        return prototype
