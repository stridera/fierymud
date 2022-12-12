from .base import Base, Dice, MudTypes
from .flags import *

from .mudfile import MudFile


class PlayerObj(Base):
    """Construct an Item"""

    def __init__(self, vnum):
        super().__init__(vnum)
        self.type = MudTypes.PLAYER_OBJECTS
        self.location = None
        self.contents = []

    @classmethod
    def from_mudfile(cls, mudfile: MudFile):
        """
        Creates a new object from a file
        :param mudfile: The file to read from
        :return: The object
        """
        objects = []
        data = []
        got_version = False
        try:
            for line in mudfile:
                if not got_version:
                    got_version = True
                    if line != "1":
                        raise Exception(f"Unsupported version: {version}")
                    continue

                if line.startswith("~~"):
                    if data:
                        obj = cls.from_data(data)
                        if obj and obj.location and obj.location < 0:
                            depth = - obj.location
                            parent = objects[-1]
                            for i in range(1, depth):
                                parent = parent.contents[-1]
                            parent.contents.append(obj)
                        else:
                            objects.append(obj)
                else:
                    data.append(line)
            return objects
        except Exception as e:
            print(f"\nError parsing {cls.__name__} file: {e}")
            raise e
            return []

    @classmethod
    def from_data(cls, data: list[str]):
        """
        Creates a new object from data
        :param data: The data to read from
        :return: The object
        """
        vnum = int(data.pop(0).split(':')[1].strip())
        obj = cls(vnum)
        try:
            obj.parse(data)
        except Exception as e:
            print(f"\nError parsing {obj.type} vnum: {vnum}: {e}")
            raise e
        return obj

    def parse(self, data):
        self.stats = {}
        while data:
            line = data.pop(0)

            if line.startswith("location:"):
                self.location = int(line.split(':')[1].strip())
                if self.location == 127:
                    self.stats['location'] = "INVENTORY"
                elif self.location > 0:
                    self.stats["location"] = self.get_flag(self.location, WEAR_LOCATIONS)
                else:
                    self.stats["location"] = "CONTAINER"
            elif line.startswith("values:"):
                values = []
                for _ in range(7):
                    line = data.pop(0)
                    values.append(int(line))
                self.stats["values"] = values
                data.pop(0)
            elif line.startswith("name:"):
                self.stats["namelist"] = line.split(':')[1].strip()
            elif line.startswith("shortdesc:"):
                self.stats["short_desc"] = line.split(':')[1].strip()
            elif line.startswith("desc:"):
                self.stats["long_descr"] = line.split(':')[1].strip()
            elif line.startswith("extradesc:"):
                if "extra_desc" not in self.stats:
                    self.stats["extra_desc"] = []
                extradesc = {"namelist": line.split(':')[1].strip(), "description": self.read_string(data)}
                self.stats["extra_desc"].append(extradesc)
            elif line.startswith("flags:"):
                self.stats["flags"] = self.read_flag_list(line.split(':')[1].strip(), EXTRA_OBJ_FLAGS)
            elif line.startswith("wear:"):
                self.stats["wear"] = self.read_flags(line.split(':')[1].strip(), WEAR_FLAGS)
            elif line.startswith("effects:"):
                effects = line.split(':')[1].strip()
                self.stats["effect"] = self.read_flag_list(effects, EFFECTS)
            elif line.startswith("applies:"):
                applies = {}
                line = data.pop(0).strip()
                while line != "~":
                    apply, value = line.split(' ')
                    affect = self.get_flag(apply, AFFECTS)
                    applies[affect] = value
                    line = data.pop(0).strip()
                self.stats["applies"] = applies
            elif line.startswith("triggers:"):
                line = data.pop(0).strip()
                triggers = []
                while line != "~":
                    triggers.append(line)
                    line = data.pop(0).strip()
                self.stats["triggers"] = triggers
            elif line.startswith("adesc:"):
                self.stats["adesc"] = self.read_string(data)
            else:
                k, v = line.split(':')
                self.stats[k] = v.strip()
