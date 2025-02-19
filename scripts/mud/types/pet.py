from dataclasses import dataclass
from typing import Self

from mud.mudfile import MudData


@dataclass
class Pet:
    vnum: int
    name: str
    desc: str

    @classmethod
    def parse_player(cls, pet_file: MudData) -> Self:
        vnum = pet_file.get_next_line()
        if vnum:
            return cls(
                vnum=int(vnum),
                name=pet_file.get_next_line().split(":")[1].strip(),
                desc=pet_file.get_next_line().split(":")[1].strip(),
            )
        return None
