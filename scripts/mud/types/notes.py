from dataclasses import dataclass

from mud.mudfile import MudData


@dataclass
class Notes:
    player: str
    note: str

    @classmethod
    def parse_player(cls, mudfile: MudData):
        notes = []
        while line := mudfile.get_next_line():
            player, note = line.split(" ", 1)
            notes.append(cls(player, note.strip()))
        return notes

    def to_json(self):
        from dataclasses import asdict
        return asdict(self)
