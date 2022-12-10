"""Mud Objects"""
from abc import ABC, abstractmethod
from enum import Enum
import re
from typing import List
import json
import os

from .utils import BitFlags


class MudTypes(Enum):
    """
    Enum for the different types of mud objects
    """

    ROOM = 1
    ITEM = 2
    MOB = 3
    PLAYER = 4


class Dice:
    """
    Class for dice
    """

    def __init__(self, num: int, size: int, bonus: int = 0):
        self.num = num
        self.size = size
        self.bonus = bonus

    def __str__(self):
        return f"{self.num}d{self.size}+{self.bonus}"

    def __repr__(self):
        return f"{self.num}d{self.size}+{self.bonus}"


class MudObject(ABC):
    """MUD Object Base Class"""

    def __init__(self, vnum: int, verbose: bool = False):
        self.vnum = vnum
        self.verbose = verbose
        self.type = None
        self.stats = {}

    # Reading files
    def read_file(self, filename: str, vnum: int):
        """
        Reads the file and parses it
        :param filename: The file to read
        :return: None
        """
        if filename.endswith("index"):
            self._process_index(filename, vnum)
        else:
            self._process_file(filename, vnum)

    def _process_index(self, index_file: str, vnum: int):
        """Process an index file"""
        if self.verbose:
            print(f"Processing index file {index_file}")
        path = index_file[:-5]
        for line in open(index_file, encoding="ascii"):
            if line.startswith("$"):
                print("End of index")
                return
            elif line.rstrip().endswith(".mob"):
                self._process_file(path + line.rstrip(), vnum)

    def _process_file(self, filename: str, vnum: int = -1):
        """Process a file"""
        if self.verbose:
            print(f"Processing {filename}")
        current_vnum = -1
        mobs = []
        data = []
        with open(filename, "r", encoding="ascii") as f:
            for line in f:
                line = line.rstrip()
                if line.startswith("#"):
                    if current_vnum >= 0 and (vnum == -1 or vnum == current_vnum):
                        print(f"Processing vnum {current_vnum}")
                        mob = Mob(current_vnum)
                        # print(f"Data: {data}\n\n")
                        mob.parse(data)
                        mobs.append(mob)
                    current_vnum = int(line[1:])
                    data = []
                else:
                    data.append(line)

        with open(os.path.splitext(filename)[0] + ".json", "w", encoding="ascii") as f:
            for mob in mobs:
                f.write(mob.to_json())
                f.write("\n")

    @abstractmethod
    def parse(self, data):
        """
        Parses the data into the object
        :param data: The data to parse
        :return: None
        """

    # Processing data
    def read_string(self, data: List[str]) -> str:
        """
        Reads a string from the data
        :param data: The data to read from
        :return: The string read
        """
        text = ""
        line = data.pop(0).rstrip()
        while not line.endswith("~"):
            text += line + " "
            line = data.pop(0).strip()
        text += line[:-1]

        return re.sub(r"&.", "", text).rstrip()

    def read_flags(self, data: str, flags: List[str], offset: int = 0) -> BitFlags:
        """
        Reads a string of flags from the data
        :param data: The data to read from
        :return: The flags read
        """
        return BitFlags(flags).set_flags(data, offset)

    @staticmethod
    def build_flags(flag, flaglist, offset=0):
        """Builds a string of flags from a bitfield"""
        active = []
        for i in range(len(flaglist)):
            if int(flag) & (1 << i):
                active.append(flaglist[i + offset])
        return ",".join(active)

    # Writing files
    def default(self, obj):
        """
        Default method for converting the object to json
        :param obj: The object to convert
        :return: The converted object
        """
        if hasattr(obj, "to_json"):
            return obj.to_json()

        return obj.__dict__ if hasattr(obj, "__dict__") else str(obj)

    def to_json(self):
        """
        Converts the object to a json object
        :return: The json object
        """
        response = {"vnum": self.vnum, "stats": self.stats}
        return json.dumps(response, default=self.default)
