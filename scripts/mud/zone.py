from .base import Base, Dice, MudTypes
from .flags import *


class Zone(Base):
    """Construct an Item"""

    def __init__(self, vnum):
        super().__init__(vnum)
        self.type = MudTypes.Zone

    def parse(self, data):
        raise NotImplementedError
