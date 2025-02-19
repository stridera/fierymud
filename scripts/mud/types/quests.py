from dataclasses import dataclass
from typing import Self

from mud.mudfile import MudData


@dataclass
class Quests:
    id: int
    stage: int
    variables: dict[str, str]

    @classmethod
    def parse_player(cls, quest_file: MudData) -> list[Self]:
        quests = []
        while line := quest_file.get_next_line():
            quest_id, stage, vars = [int(x) for x in line.split()]
            variables = {}
            for _ in range(vars):
                key, value = quest_file.get_next_line().split()
                variables[key] = value
            quests.append(
                cls(
                    id=quest_id,
                    stage=stage,
                    variables=variables,
                )
            )
        return quests
