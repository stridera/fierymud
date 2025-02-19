import re
from dataclasses import dataclass

from mud.bitflags import BitFlags
from mud.flags import SHOP_FLAGS, SHOP_TRADES_WITH
from mud.mudfile import MudData
from mud.types.object import ObjectType


@dataclass
class Shop:
    vnum: int
    selling: list[dict[int, int]]
    buy_profit: float
    sell_profit: float
    accepts: str
    no_such_item1: str
    no_such_item2: str
    do_not_buy: str
    missing_cash1: str
    missing_cash2: str
    message_buy: str
    message_sell: str
    temper1: str
    flags: str
    keeper: str
    trades_with: str
    rooms: str
    hours: str

    @classmethod
    def parse(cls, data: MudData):
        shops = []
        version = data.get_next_line()
        if version.startswith("$"):
            return None
        if version != "CircleMUD v3.0 Shop File~":
            raise ValueError("Invalid shop file")
        for shop_data in data.split_by_delimiter():
            shop = {}
            shop["vnum"] = int(shop_data.get_next_line().lstrip("#").rstrip("~"))
            shop["selling"] = {}
            for line in shop_data.read_until_starts("-1"):
                item = line.split()
                amount = int(item[1]) if len(item) > 1 else 0
                shop["selling"][int(item[0])] = amount

            shop["buy_profit"] = float(shop_data.get_next_line())
            shop["sell_profit"] = float(shop_data.get_next_line())

            shop["accepts"] = []
            for line in shop_data.read_until_starts("-1"):
                groups = re.findall(r"(\d+|\D+)", line)
                item_type = ObjectType(int(groups[0]))
                shop["accepts"].append({"type": item_type, "keywords": groups[1] if len(groups) > 1 else ""})

            shop["no_such_item1"] = shop_data.read_string()
            shop["no_such_item2"] = shop_data.read_string()
            shop["do_not_buy"] = shop_data.read_string()
            shop["missing_cash1"] = shop_data.read_string()
            shop["missing_cash2"] = shop_data.read_string()
            shop["message_buy"] = shop_data.read_string()
            shop["message_sell"] = shop_data.read_string()
            shop["temper1"] = int(shop_data.get_next_line())
            shop["flags"] = BitFlags.read_flags(shop_data.get_next_line(), SHOP_FLAGS)
            shop["keeper"] = int(shop_data.get_next_line())
            shop["trades_with"] = BitFlags.read_flags(shop_data.get_next_line(), SHOP_TRADES_WITH)
            shop["rooms"] = []
            for line in shop_data.read_until_starts("-1"):
                shop["rooms"].append(int(line))
            shop["hours"] = [{"open": int(shop_data.get_next_line()), "close": int(shop_data.get_next_line())}]
            opens = int(shop_data.get_next_line())
            closes = int(shop_data.get_next_line())
            if any([opens, closes]):
                shop["hours"].append({"open": opens, "close": closes})
            shops.append(cls(**shop))
        return shops
