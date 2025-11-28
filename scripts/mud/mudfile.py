import os
import re
import traceback
from dataclasses import dataclass
from enum import Enum
from json import JSONEncoder
from typing import Optional, Self, Type, TypeVar, cast

from mud.types import MudTypes

T = TypeVar("T")


class Encoder(JSONEncoder):
    # Writing files
    def default(self, o):
        """
        Default method for converting the object to json, dropping null fields.
        :param obj: The object to convert
        :return: The converted object
        """
        if hasattr(o, "json_repr"):
            result = o.json_repr()
        elif isinstance(o, Enum):
            return o.name
        elif hasattr(o, "__dict__"):
            result = o.__dict__
        else:
            return str(o)

        if isinstance(result, dict):
            # Drop fields with value None
            return {k: v for k, v in result.items() if v is not None}
        return result


class MudData:
    def __init__(self, data: list[str]):
        self.data: list[str] = data
        self.current_line: int = 0

    # Processing data
    def get_next_line(self) -> str | None:
        while self.current_line < len(self.data):
            line = self.data[self.current_line].strip()
            self.current_line += 1
            if line == "":
                continue
            return line

    def split_by_delimiter(self, delimiter: str = "#") -> list[Self]:
        objects = []
        object_data = []
        while line := self.get_next_line():
            if line.startswith(delimiter) or line == "$" or line == "$~":
                if object_data:
                    objects.append(MudData(object_data))
                if line == "$" or line == "$~":
                    break
                object_data = [line]
            else:
                object_data.append(line)
        return objects

    def read_string(self) -> str:
        """
        Reads a string from the data
        :param data: The data to read from
        :return: The string read
        """
        text = ""
        while line := self.get_next_line():
            if line.endswith("~"):
                text += line[:-1]
                return text
            text += line + " "
        return text

    def read_key_value(self) -> Optional[tuple[str, str]]:
        line = self.get_next_line()
        if not line:
            return None
        if ":" in line:
            key, value = line.split(":", 1)
            return key.strip(), value.strip()
        return line.strip(), ""

    def read_until_starts(self, delimiter: str) -> list[str]:
        """
        Reads data until a delimiter is found
        :param data: The data to read from
        :param delimiter: The delimiter to stop at
        :return: The data read
        """
        result = []
        while (line := self.get_next_line()) is not None:
            if line.startswith(delimiter):
                break
            result.append(line)
        return result

    def read_until_match(self, delimiter: str) -> list[str]:
        """Read until a line matches the delimiter"""
        result = []
        while (line := self.get_next_line()) is not None:
            if line == delimiter:
                break
            result.append(line)
        return result

    def read_to_list(self, delimiter: str, data_type: Type[T]) -> list[T]:
        """
        Read a list of data until a delimiter is found
        :param data: The data to read
        :param delimiter: The delimiter to stop at
        :return: The list of data
        """
        result = []
        for line in self.read_until_starts(delimiter):
            result.append(cast(data_type, line))
        return result

    def current_file(self):
        return self.filename

    @staticmethod
    def decolor(str):
        """Removes color codes (&d+) from a string"""
        return re.sub(r"&[0-9a-fA-F]", "", str)


@dataclass
class MudFile:
    filename: str
    mud_type: MudTypes

    data = MudData

    def __post_init__(self):
        """Process a file"""
        data = open(self.filename, encoding="ascii").readlines()
        self.data = MudData(data)

    def get_mud_type(self) -> str:
        return self.mud_type.name

    def get_json_id(self) -> str:
        return self.mud_type.get_json_id()

    def parse_world(self):
        cls = self.mud_type.cls()
        try:
            return cls.parse(self.data)
        except Exception as e:
            print(f"Error parsing {self.filename}:{self.data.current_line}: {e}")
            print(traceback.format_exc())

    def parse_player(self):
        cls = self.mud_type.cls()
        try:
            if hasattr(cls, "parse_player"):
                return cls.parse_player(self.data)
            else:
                raise AttributeError(f"{cls.__name__} does not support player parsing.")
        except Exception as e:
            print(f"Error parsing {self.filename}:{self.data.current_line}: {e}")
            print(traceback.format_exc())


@dataclass
class MudFiles:
    id: str
    path: str
    files: list[MudFile]

    @classmethod
    def _get_zone_files(cls, path: str, zone: int | None = None) -> Self:
        files = []
        for ext in ["zon", "mob", "obj", "shp", "trg", "wld"]:
            file = os.path.join(path, ext, f"{zone}.{ext}")
            if os.path.exists(file):
                files.append(MudFile(filename=file, mud_type=MudTypes.from_ext(ext)))
        return cls(str(zone), path, files)

    @classmethod
    def zone_files(cls, path: str, zone: int | None = None) -> list[Self]:
        file_list = []

        index_file = os.path.join(path, "index")
        if os.path.exists(index_file):
            for line in open(index_file, encoding="ascii"):
                if line.strip() == "":
                    continue
                if line.startswith("$"):
                    break
                zone_number = int(line.split(".")[0])
                if zone is None or zone_number == zone:
                    file_list.append(cls(str(zone_number), path, cls._get_zone_files(path, zone_number)))

        return file_list

    @classmethod
    def _get_player_files(cls, path: str, player: str):
        player_name = player[0].upper() + player[1:].lower()
        root = f"{path}/{player[0].upper()}/{player_name}"
        files = []
        for ext in ["plr", "objs", "pet", "quest", "notes"]:
            file = f"{root}.{ext}"
            if os.path.exists(file):
                files.append(MudFile(filename=file, mud_type=MudTypes.from_ext(ext)))
        return cls(player, path, files)

    @classmethod
    def player_files(cls, path: str, player: str):
        file_list = []
        if player is None:
            for _root, _dirs, found_files in os.walk(path):
                for file in found_files:
                    if file.endswith(".plr"):
                        player = file[:-4]
                        file_list.append(cls(player, path, cls._get_player_files(path, player)))
        else:
            file_list.append(cls(player, path, cls._get_player_files(path, player)))

        return file_list

    def __iter__(self):
        for file in self.files:
            yield file
