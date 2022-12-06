"""Mud Objects"""
from abc import ABC, abstractmethod
from enum import Enum
import re
from typing import List
import json

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

    def __init__(self, num: int, size: int, bonus: int):
        self.num = num
        self.size = size
        self.bonus = bonus

    def __str__(self):
        return f"{self.num}d{self.size}+{self.bonus}"

    def __repr__(self):
        return f"{self.num}d{self.size}+{self.bonus}"


class MudObject(ABC):
    """ MUD Object Base Class """

    def __init__(self, vnum: int):
        self.vnum = vnum
        self.type = None
        self.stats = {}

    @abstractmethod
    def parse(self, data):
        """
        Parses the data into the object
        :param data: The data to parse
        :return: None
        """

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

        return re.sub(r'&.', '', text).rstrip()

    def read_flags(self, data: str, flags: List[str], offset: int = 0) -> BitFlags:
        """
        Reads a string of flags from the data
        :param data: The data to read from
        :return: The flags read
        """
        return BitFlags(flags).set_flags(data, offset)

    def default(self, o):
        """
        Default method for converting the object to json
        :param o: The object to convert
        :return: The converted object
        """
        if hasattr(o, 'to_json'):
            return o.to_json()

        return o.__dict__ if hasattr(o, '__dict__') else str(o)

    def to_json(self):
        """
        Converts the object to a json object
        :return: The json object
        """
        response = {'vnum': self.vnum, 'stats': self.stats}
        return json.dumps(response, default=self.default)
