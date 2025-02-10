from .base import Base, MudTypes
from .flags import *
from .mudfile import MudFile


class Player(Base):
    """Construct an Item"""

    KILL_TYPE_FLAGS = ["None", "Mob", "Player"]

    def __init__(self, vnum):
        super().__init__(vnum)
        self.type = MudTypes.PLAYER

    @classmethod
    def from_mudfile(cls, mudfile: MudFile):
        """
        Convert the Player file to a json object
        :param mudfile: The file to read from
        :return: The object
        """
        data = []
        try:
            for line in mudfile:
                data.append(line)
            return cls.parse(data)
        except Exception as e:
            print(f"\nError parsing {cls.__name__} file: {e}")
            raise e

    @classmethod
    def parse(cls, data):
        """
        Parse the data from the file
        :param data: The data to read from
        """
        player = {}
        while data:
            line = data.pop(0)
            if line.startswith("description"):
                player["description"] = cls.read_string(data)
            elif line.startswith("sex"):
                value = line.split(":")[1].strip()
                player["sex"] = SEXES[int(value)]
            elif line.startswith("class"):
                value = line.split(":")[1].strip()
                player["class"] = CLASSES[int(value)]
            elif line.startswith("race"):
                value = line.split(":")[1].strip()
                player["races"] = RACES[int(value)]
            elif line.startswith("lifeforce"):
                value = line.split(":")[1].strip()
                player["lifeforce"] = LIFEFORCES[int(value)]
            elif line.startswith("composition"):
                value = line.split(":")[1].strip()
                player["composition"] = COMPOSITIONS[int(value)]
            elif line.startswith("playerflags"):
                value = line.split(":")[1].strip()
                player["playerflags"] = cls.read_flags(value, PLAYER_FLAGS)
            elif line.startswith("effectflags"):
                flags = line.split(":")[1].strip()
                player["effectflags"] = cls.read_flag_list(flags, EFFECTS)
            elif line.startswith("prefflags"):
                flags = line.split(":")[1].strip()
                player["prefflags"] = cls.read_flag_list(flags, PREF_FLAGS)
            elif line.startswith("privflags"):
                flags = line.split(":")[1].strip()
                player["privflags"] = cls.read_flags(flags, PRIV_FLAGS)
            elif line.startswith("quit_reason"):
                value = line.split(":")[1].strip()
                player["quit_reason"] = QUIT_REASONS[int(value)]
            elif line.startswith("trophy"):
                player["trophy"] = []
                while line := data.pop(0):
                    if line == "0 0 0":
                        break
                    kill_type, id, count = line.split()
                    player["trophy"].append(
                        {"type": cls.KILL_TYPE_FLAGS[int(kill_type)], "id": int(id), "count": float(count)}
                    )
            elif line.startswith("tells"):
                player["tells"] = []
                while line := data.pop(0):
                    if line == "$":
                        break
                    player["tells"].append(line)
            elif line.startswith("gossips"):
                player["gossips"] = []
                while line := data.pop(0):
                    if line == "$":
                        break
                    player["gossips"].append(line)
            elif line.startswith("aliases"):
                player["aliases"] = {}
                while line := data.pop(0):
                    if line == "0":
                        break
                    alias, command = line.split(" ", 1)
                    player["aliases"][alias] = command.strip()
            elif line.startswith("cash"):
                plat, gold, silver, copper = line[5:].split()
                player["money"] = {"plat": int(plat), "gold": int(gold), "silver": int(silver), "copper": int(copper)}
            else:
                if ":" in line:
                    key, value = line.split(":", 1)
                    player[key] = value.strip()
                else:
                    raise ValueError(f"Invalid data: {line}")
        return player
