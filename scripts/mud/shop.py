from .base import Base, Dice, MudTypes
from .flags import *


class Shop(Base):
    """Construct an Item"""

    def __init__(self, vnum):
        super().__init__(vnum)
        self.type = MudTypes.Shop

    def parse(self, data):
        raise NotImplementedError
