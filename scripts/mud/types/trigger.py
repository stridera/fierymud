from dataclasses import dataclass

from mud.bitflags import BitFlags
from mud.flags import TRIGGER_TYPES
from mud.mudfile import MudData


@dataclass
class Trigger:
    id: int
    name: str
    attach_type: str
    flags: list[str]
    number_of_arguments: int
    argument_list: str
    commands: str

    ATTACH_TYPES = [
        "Object",
        "Room",
        "Mobile",
    ]

    @classmethod
    def parse(cls, trigger_file: MudData):
        triggers = []
        for trigger_data in trigger_file.split_by_delimiter():
            trigger = {}
            trigger["id"] = int(trigger_data.get_next_line().lstrip("#"))
            trigger["name"] = trigger_data.read_string()
            attach_type, flags, args = trigger_data.get_next_line().split()
            attach_type = cls.ATTACH_TYPES[int(attach_type)]
            trigger["attach_type"] = attach_type
            trigger["flags"] = list(BitFlags.read_flags(flags, TRIGGER_TYPES))
            trigger["number_of_arguments"] = int(args)
            trigger["argument_list"] = trigger_data.read_string()
            trigger["commands"] = trigger_data.read_string()
            triggers.append(trigger)

        return triggers

    def to_json(self):
        from dataclasses import asdict

        return asdict(self)
