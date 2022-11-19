from abc import ABC, abstractmethod
from enum import Enum
import re
from typing import List


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

    def readString(self, data: List[str]):
        """
        Reads a string from the data
        :param data: The data to read from
        :return: The string read
        """
        print(f"Reading string from {data}")
        text = ""
        line = data.pop(0).rstrip()
        while not line.endswith("~"):
            text += line + " "
            line = data.pop(0).rstrip()
        text += line[:-1]

        return re.sub(r'&.', '', text)

    def readFlags(self, data: List[str]):
        """
        Reads a string of flags from the data
        :param data: The data to read from
        :return: The flags read
        """
        print(f"Reading flags from {data}")
        flags = []

        return flags
