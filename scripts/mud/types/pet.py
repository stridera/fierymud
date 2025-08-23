from dataclasses import dataclass
from typing import Self

from mud.mudfile import MudData


@dataclass
class Pet:
    id: int
    name: str
    desc: str

    @classmethod
    def parse_player(cls, pet_file: MudData) -> Self:
        id = pet_file.get_next_line()
        if id:
            return cls(
                id=int(id),
                name=pet_file.get_next_line().split(":")[1].strip(),
                desc=pet_file.get_next_line().split(":")[1].strip(),
            )
        return None

    def to_json(self):
        from dataclasses import asdict
        return asdict(self)
