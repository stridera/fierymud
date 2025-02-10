from .base import Base, MudTypes
from .flags import *
import re


class Shop(Base):
    """Construct an Item"""

    ITEM_TYPES = [
        "UNDEFINED",
        "LIGHT",
        "SCROLL",
        "WAND",
        "STAFF",
        "WEAPON",
        "FIREWEAPON",
        "MISSILE",
        "TREASURE",
        "ARMOR",
        "POTION",
        "WORN",
        "OTHER",
        "TRASH",
        "TRAP",
        "CONTAINER",
        "NOTE",
        "LIQCONTAINER",
        "KEY",
        "FOOD",
        "MONEY",
        "PEN",
        "BOAT",
        "FOUNTAIN",
        "PORTAL",
        "ROPE",
        "SPELLBOOK",
        "WALL",
        "TOUCHSTONE",
        "BOARD",
        "INSTRUMENT",
    ]

    def __init__(self, vnum):
        super().__init__(vnum)
        self.type = MudTypes.SHOP

    def parse(self, data):
        self.stats = {}
        self.stats["selling"] = []
        while data:
            item = data.pop(0).split()
            if item[0] == "-1":
                break
            else:
                amount = int(item[1]) if len(item) > 1 else 0
                self.stats["selling"].append({"item": int(item[0]), "amount": amount})

        self.stats["buy_profit"] = float(data.pop(0))
        self.stats["sell_profit"] = float(data.pop(0))

        self.stats["accepts"] = []
        while data:
            line = data.pop(0)
            if line == "-1":
                break

            groups = re.findall(r"(\d+|\D+)", line)
            item_type = self.ITEM_TYPES[int(groups[0])]
            self.stats["accepts"].append({"type": item_type, "keywords": groups[1] if len(groups) > 1 else ""})

        self.stats["no_such_item1"] = self.read_string(data)
        self.stats["no_such_item2"] = self.read_string(data)
        self.stats["do_not_buy"] = self.read_string(data)
        self.stats["missing_cash1"] = self.read_string(data)
        self.stats["missing_cash2"] = self.read_string(data)
        self.stats["message_buy"] = self.read_string(data)
        self.stats["message_sell"] = self.read_string(data)
        self.stats["temper1"] = int(data.pop(0))
        self.stats["flags"] = self.read_flags(data.pop(0), SHOP_FLAGS)
        self.stats["keeper"] = int(data.pop(0))
        self.stats["trades_with"] = self.read_flags(data.pop(0), SHOP_TRADES_WITH)
        self.stats["rooms"] = []
        while line := data.pop(0):
            if line == "-1":
                break
            else:
                self.stats["rooms"].append(int(line))
        self.stats["hours"] = []
        self.stats["hours"].append({"open": int(data.pop(0)), "close": int(data.pop(0))})
        opens = int(data.pop(0))
        closes = int(data.pop(0))
        if any([opens, closes]):
            self.stats["hours"].append({"open": opens, "close": closes})
