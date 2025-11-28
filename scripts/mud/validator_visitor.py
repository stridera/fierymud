import logging
from typing import Any

logger = logging.getLogger("mud_validator")

class Visitor:
    def visit_room(self, room: dict, context: dict):
        pass
    def visit_mob(self, mob: dict, context: dict):
        pass
    def visit_object(self, obj: dict, context: dict):
        pass
    def visit_shop(self, shop: dict, context: dict):
        pass
    def visit_zone(self, zone: dict, context: dict):
        pass



def get_id(obj):
    # Accept either 'id' or legacy 'vnum' as an identifier.
    if hasattr(obj, "id"):
        return obj.id
    if isinstance(obj, dict) and "id" in obj:
        return obj["id"]
    # Fallbacks
    if hasattr(obj, "vnum"):
        return getattr(obj, "vnum")
    if isinstance(obj, dict) and "vnum" in obj:
        return obj["vnum"]
    return None

class ValidatorVisitor(Visitor):
    def __init__(self, all_data: dict[str, Any], error_log: list[str]):
        self.all_data = all_data
        self.error_log = error_log
        self.valid_room_ids = set(str(get_id(r)) for r in all_data.get("rooms", []))
        self.valid_mob_ids = set(str(get_id(m)) for m in all_data.get("mobs", []))
        self.valid_object_ids = set(str(get_id(o)) for o in all_data.get("objects", []))
        # Collect invalid object ids during validation so they can be removed after pass
        self.invalid_object_ids: set[str] = set()
        # Collect invalid shops to be removed wholesale
        self.invalid_shop_ids: set[str] = set()

    def visit_room(self, room: Any, context: dict):
        # Validate exits
        exits = getattr(room, "exits", None) or (room.get("exits") if isinstance(room, dict) else None)
        room_id = get_id(room)
        if exits:
            invalid = []
            for dir, exit in exits.items():
                dest = str(exit.get("destination"))
                if dest not in self.valid_room_ids:
                    self.error_log.append(f"Room {room_id} exit {dir} points to invalid room {dest}")
                    invalid.append(dir)
            for dir in invalid:
                del exits[dir]

    def visit_object(self, obj: Any, context: dict):
        """Validate object references and mark invalid objects for removal.

        Criteria for invalid object:
        - Missing id
        - (Previously) invalid room/location reference -> now we clear the field instead of removing object
        - Contains references to other objects that are invalid (prune those refs only)
        """
        obj_id = get_id(obj)
        if obj_id is None:
            # If neither id nor vnum exists treat as invalid
            self.error_log.append("Object with missing id/vnum removed")
            self.invalid_object_ids.add("<missing>")
            return
        obj_id_str = str(obj_id)

        # Support multiple possible location keys
        location_keys = ["room", "location", "in_room", "room_id"]
        for key in location_keys:
            if isinstance(obj, dict):
                room_ref = obj.get(key)
            else:
                room_ref = getattr(obj, key, None)
            if room_ref is not None and str(room_ref) not in self.valid_room_ids:
                # Clear invalid location instead of removing the object
                if isinstance(obj, dict):
                    obj[key] = None
                else:
                    try:
                        setattr(obj, key, None)
                    except Exception:
                        pass
                self.error_log.append(f"Object {obj_id_str} cleared invalid {key} reference {room_ref}")

        # If object lists contained objects (e.g., inventory), ensure they reference valid objects.
        contained_key_candidates = ["contains", "contents", "inventory"]
        for key in contained_key_candidates:
            if isinstance(obj, dict):
                contained = obj.get(key)
            else:
                contained = getattr(obj, key, None)
            if not contained:
                continue
            # Expect list of object ids
            to_remove = [cid for cid in list(contained) if str(cid) not in self.valid_object_ids]
            if to_remove:
                for cid in to_remove:
                    self.error_log.append(f"Object {obj_id_str} contains invalid object ref {cid}; reference removed")
                    try:
                        contained.remove(cid)
                    except ValueError:
                        pass

    def visit_zone(self, zone: Any, context: dict):
        # Validate resets (mobs, objects, etc.)
        resets = getattr(zone, "resets", None)
        if resets is None and isinstance(zone, dict):
            resets = zone.get("resets", {})
        if resets is None:
            resets = {}
        zone_id = get_id(zone)
        mob_list = resets.get("mob") if isinstance(resets, dict) else None
        if mob_list is None:
            mob_list = []
        for mob in list(mob_list):
            mob_id = mob.get("id") if isinstance(mob, dict) else getattr(mob, "id", None)
            mob_room = mob.get("room") if isinstance(mob, dict) else getattr(mob, "room", None)
            if str(mob_id) not in self.valid_mob_ids:
                self.error_log.append(f"Zone {zone_id} reset mob {mob_id} invalid")
                mob_list.remove(mob)
            elif str(mob_room) not in self.valid_room_ids:
                self.error_log.append(f"Zone {zone_id} reset mob {mob_id} in invalid room {mob_room}")
                mob_list.remove(mob)
        obj_list = resets.get("object") if isinstance(resets, dict) else None
        if obj_list is None:
            obj_list = []
        for obj in list(obj_list):
            obj_id = obj.get("id") if isinstance(obj, dict) else getattr(obj, "id", None)
            obj_room = obj.get("room") if isinstance(obj, dict) else getattr(obj, "room", None)
            if str(obj_id) not in self.valid_object_ids:
                self.error_log.append(f"Zone {zone_id} reset object {obj_id} invalid")
                obj_list.remove(obj)
            elif str(obj_room) not in self.valid_room_ids:
                self.error_log.append(f"Zone {zone_id} reset object {obj_id} in invalid room {obj_room}")
                obj_list.remove(obj)

    def visit_shop(self, shop: Any, context: dict):
        shop_id = get_id(shop)
        sid = str(shop_id)
        reasons = []
        # Keeper: only fatal if invalid; missing means unusable
        keeper = shop.get("keeper") if isinstance(shop, dict) else getattr(shop, "keeper", None)
        keeper_invalid = False
        if keeper is None:
            reasons.append("missing_keeper")
            keeper_invalid = True
        elif str(keeper) not in self.valid_mob_ids:
            reasons.append("invalid_keeper")
            keeper_invalid = True
        # Rooms: require at least one valid room; ignore individual invalid rooms if at least one valid present
        rooms = shop.get("rooms") if isinstance(shop, dict) else getattr(shop, "rooms", None)
        no_valid_room = False
        if not rooms:
            reasons.append("no_rooms")
            no_valid_room = True
        else:
            valid_room_present = any(str(r) in self.valid_room_ids for r in rooms)
            if not valid_room_present:
                reasons.append("no_valid_rooms")
                no_valid_room = True
        # Selling: allow empty dict (some shops might populate later). If present and every item invalid => fatal.
        selling = shop.get("selling") if isinstance(shop, dict) else getattr(shop, "selling", None)
        if selling is not None and isinstance(selling, dict) and selling:
            bad_items = [item_id for item_id in list(selling.keys()) if str(item_id) not in self.valid_object_ids]
            if bad_items and len(bad_items) == len(selling):
                reasons.append("all_items_invalid")
        # Removal decision: if keeper invalid/missing OR no valid rooms.
        fatal = keeper_invalid or no_valid_room
        if fatal:
            self.invalid_shop_ids.add(sid)
            self.error_log.append(f"Shop {shop_id} removed reasons={','.join(reasons)} (keeper_or_rooms)")
        else:
            # Non-fatal warnings for partially invalid selling items
            if selling:
                for item_id in list(selling.keys()):
                    if str(item_id) not in self.valid_object_ids:
                        self.error_log.append(f"Shop {shop_id} ignoring invalid item {item_id}")


    # Add more visit_* as needed for other types

def validate_all(all_data: dict[str, Any]) -> list[str]:
    error_log = []
    visitor = ValidatorVisitor(all_data, error_log)
    # Snapshot counts before validation
    pre_counts = {
        "rooms": len(all_data.get("rooms") or []),
        "objects": len(all_data.get("objects") or []),
        "mobs": len(all_data.get("mobs") or []),
        "shops": len(all_data.get("shops") or []),
        "zones": len(all_data.get("zones") or []),
    }
    for room in all_data.get("rooms") or []:
        visitor.visit_room(room, all_data)
    # Validate objects after rooms so room references can be checked
    for obj in all_data.get("objects") or []:
        visitor.visit_object(obj, all_data)
    # Remove invalid objects collected during validation
    if visitor.invalid_object_ids:
        original_count = len(all_data.get("objects") or [])
        # Filter objects with valid ids only
        filtered_objects = []
        for o in all_data.get("objects") or []:
            oid = get_id(o)
            if oid is None:
                continue
            if str(oid) in visitor.invalid_object_ids:
                error_log.append(f"Removed invalid object {oid}")
            else:
                filtered_objects.append(o)
        all_data["objects"] = filtered_objects
        new_count = len(filtered_objects)
        error_log.append(f"Pruned {original_count - new_count} invalid objects")
        # Recompute valid object ids after pruning
        visitor.valid_object_ids = set(str(get_id(o)) for o in filtered_objects)
    for zone in all_data.get("zones") or []:
        visitor.visit_zone(zone, all_data)
    for shop in all_data.get("shops") or []:
        visitor.visit_shop(shop, all_data)
    # Prune invalid shops
    if visitor.invalid_shop_ids:
        shops = all_data.get("shops") or []
        removed = 0
        # Build list of valid shops first
        valid_shops = []
        for s in shops:
            sid = str(get_id(s))
            if sid in visitor.invalid_shop_ids:
                error_log.append(f"Removed invalid shop {sid}")
                removed += 1
            else:
                valid_shops.append(s)
        # In-place mutation to ensure any external references see updated content
        shops[:] = valid_shops
        error_log.append(f"Pruned {removed} invalid shops (in-place)")
    # Post counts
    post_counts = {
        "rooms": len(all_data.get("rooms") or []),
        "objects": len(all_data.get("objects") or []),
        "mobs": len(all_data.get("mobs") or []),
        "shops": len(all_data.get("shops") or []),
        "zones": len(all_data.get("zones") or []),
    }
    error_log.append(
        "SUMMARY before/after: " + ", ".join(
            f"{k} {pre_counts[k]}->{post_counts[k]}" for k in ["rooms","objects","mobs","shops","zones"]
        )
    )
    # Add more as needed
    return error_log
