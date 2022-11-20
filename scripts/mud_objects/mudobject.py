from abc import ABC, abstractmethod
from enum import Enum
import re
from typing import List
from .utils import BitFlags


class MudTypes(Enum):
    """
    Enum for the different types of mud objects
    """
    ROOM = 1
    ITEM = 2
    MOB = 3
    PLAYER = 4


class MudObject(ABC):
    def __init__(self, vnum):
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
        pass

    def readString(self, data: List[str]) -> str:
        """
        Reads a string from the data
        :param data: The data to read from
        :return: The string read
        """
        text = ""
        line = data.pop(0).rstrip()
        while not line.endswith("~"):
            text += line + " "
            line = data.pop(0).rstrip()
        text += line[:-1]

        return re.sub(r'&.', '', text).rstrip()

    def readFlags(self, data: str, flags: List[str], int offset=0) -> BitFlags:
        """
        Reads a string of flags from the data
        :param data: The data to read from
        :return: The flags read
        """
        return BitFlags(data, flags, offset)
