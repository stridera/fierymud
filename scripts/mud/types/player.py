import ipaddress
from dataclasses import dataclass
from datetime import datetime

from mud.mudfile import MudData
from mud.parser import Parser
from mud.types import (
    Class,
    Composition,
    Cooldown,
    Gender,
    LifeForce,
    Money,
    MudTypes,
    QuitReason,
    Race,
    SavingThrows,
    Stats,
)


@dataclass
class Player:
    """Construct a Player"""

    KILL_TYPE_FLAGS = ["None", "Mob", "Player"]

    type = MudTypes.PLAYER

    id: int
    name: str
    password: str
    prompt: str
    level: int
    home: str
    birth_time: datetime
    time_played: int
    last_login_time: datetime
    host: ipaddress.IPv4Address
    height: int | None = None
    weight: int | None = None
    base_height: int | None = None
    base_weight: int | None = None
    base_size: int | None = None
    natural_size: int | None = None
    alignment: int | None = None
    saving_throws: SavingThrows | None = None
    load_room: int | None = None
    last_level: int | None = None
    hit_points: int | None = None
    move: int | None = None
    damage_roll: int | None = None
    hit_roll: int | None = None
    stats: Stats | None = None
    experience: int | None = None
    save_room: int | None = None
    gender: Gender | None = None
    player_class: Class | None = None
    races: Race | None = None
    life_force: LifeForce | None = None
    composition: Composition | None = None
    player_flags: list[str] | None = None
    effects: list[dict] | None = None
    effect_flags: list[str] | None = None
    preference_flags: list[str] | None = None
    privilege_flags: list[str] | None = None
    quit_reason: QuitReason | None = None
    description: str | None = None
    skills: dict[str, int] | None = None
    trophy: list[dict[str, int]] | None = None
    tells: list[str] | None = None
    gossips: list[str] | None = None
    aliases: dict[str, str] | None = None
    cooldowns: dict[Cooldown, dict[str, str]] | None = None
    spell_casts: dict[str, int] | None = None
    wimpy: int | None = None
    auto_invis: int | None = None
    money: Money | None = None
    bank: Money | None = None
    clan: int | None = None
    title: str | None = None
    current_title: str | None = None
    hunger: int | None = None
    thirst: int | None = None
    drunkenness: int | None = None
    size: int | None = None
    armor_class: int | None = None
    aggression: int | None = None
    page_length: int | None = None
    log_view: int | None = None
    bad_passwords: int | None = None

    # Clan Stuff
    title: str | None = None  # Clan title

    # God Stuff
    grant_groups: list[dict[str, str]] | None = None
    grants: list[dict[str, str]] | None = None

    freeze_level: int | None = None
    wiz_title: str | None = None
    poof_in: str | None = None
    poof_out: str | None = None
    invis_level: int | None = None
    olc_zones: list[int] | None = None

    @classmethod
    def parse_player(cls, player_file: MudData):
        parser = Parser(MudTypes.PLAYER)
        return cls(**parser.parse(player_file))
