"""Mud Objects"""

from abc import ABC, abstractmethod
from enum import Enum, auto
from typing import List
from json import JSONEncoder
import re

from .mudfile import MudFile
from .bitflags import BitFlags


class MudTypes(Enum):
    """
    Enum for the different types of mud objects
    """

    MOB = auto()
    OBJ = auto()
    SHOP = auto()
    TRIGGER = auto()
    WORLD = auto()
    ZONE = auto()

    PLAYER = auto()
    PLAYER_OBJECTS = auto()
    PLAYER_QUESTS = auto()
    PLAYER_PETS = auto()
    PLAYER_NOTES = auto()


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


class Encoder(JSONEncoder):
    # Writing files
    def default(self, obj):
        """
        Default method for converting the object to json
        :param obj: The object to convert
        :return: The converted object
        """
        if hasattr(obj, "json_repr"):
            return obj.json_repr()

        return obj.__dict__ if hasattr(obj, "__dict__") else str(obj)


class Base(ABC, Encoder):
    """MUD Object Base Class"""

    def __init__(self, vnum: int, verbose: bool = False):
        self.vnum = vnum
        self.verbose = verbose
        self.type = None
        self.stats = {}

    @classmethod
    def from_mudfile(cls, mudfile: MudFile):
        """
        Creates a new object from a file
        :param mudfile: The file to read from
        :return: The object
        """
        objects = []
        data = []
        try:
            for line in mudfile:
                if line.startswith("#"):
                    if data:
                        obj = cls.from_data(data)
                        if obj:
                            objects.append(obj)
                    data = [line]
                else:
                    data.append(line)
            if data:
                obj = cls.from_data(data)
                if obj:
                    objects.append(obj)
            return objects
        except Exception as e:
            print(f"\nError parsing {cls.__name__} file: {e}")
            raise e

    def json_repr(self):
        return {"vnum": self.vnum, "stats": self.stats}

    @classmethod
    def from_data(cls, data: List[str]):
        """
        Creates a new object from data
        :param data: The data to read from
        :return: The object
        """
        line = data.pop(0)
        if line.startswith("$"):
            # Empty file.
            return None
        elif line.startswith("CircleMUD v3.0 Shop File~"):
            # Skip the shop file header
            return None

        if not line.startswith("#"):
            print(f"Invalid data: {line}")
            return None

        if line.endswith("~"):
            # Shop files have a ~ at the end of the shop vnum
            line = line[:-1]

        vnum = int(line[1:])
        obj = cls(vnum)
        try:
            obj.parse(data)
        except Exception as e:
            print(f"\nError parsing {obj.type} vnum: {vnum}: {e}")
            raise e
        return obj

    @abstractmethod
    def parse(self, data):
        """
        Parses the data into the object.  Used for World Mud Objects.
        :param data: The data to parse
        :return: None
        """

    # Processing data
    @staticmethod
    def read_string(data: List[str]) -> str:
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

        return text.rstrip()

    @staticmethod
    def read_flag_list(data: str, flags: List[str]) -> BitFlags:
        """
        Reads a list of flags from the data
        :param data: The data to read from
        :return: The list of flags read
        """
        data_lst = data.split(" ")
        if len(data_lst) == 0:
            return BitFlags(flags)

        bitflags = BitFlags(flags)
        for i, flag in enumerate(data_lst):
            bitflags.set_flags(flag, 32 * i)
        return bitflags

    @staticmethod
    def read_flags(data: str, flags: List[str], offset: int = 0) -> BitFlags:
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
        flag = int(flag)
        if flag.bit_length() > len(flaglist):
            raise ValueError(
                f"Flag out of range! {flag}({bin(flag)}:{flag.bit_length()} bits = {len(flaglist)} ({flaglist})"
            )
        for i in range(len(flaglist)):
            if flag & (1 << i + offset):
                active.append(flaglist[i])
        return ",".join(active)

    @staticmethod
    def get_flag(flag: int, flaglist: list[str]) -> str:
        """Gets a flag from a list"""
        flag = int(flag)
        if flag >= len(flaglist):
            raise ValueError(f"Flag out of range! {flag} >= {len(flaglist)} ({flaglist})")
        return flaglist[flag]

    @staticmethod
    def decolor(str):
        """Removes color codes (&d+) from a string"""
        return re.sub(r"&[0-9a-fA-F]", "", str)
