from .base import Base, MudTypes
from .flags import *


class PlayerPets(Base):
    """Construct an Item"""

    def __init__(self, vnum):
        super().__init__(vnum)
        self.type = MudTypes.PLAYER_PETS

    def parse(self, data):
        pass
