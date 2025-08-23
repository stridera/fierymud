"""
World file mapping constants for FieryMUD legacy conversion.

This module provides human-readable mappings for magic numbers, bitflags,
and enum values used in legacy CircleMUD-style world files.
"""

from typing import Dict, List, Set
import re

# Equipment wear locations (magic number -> readable name)
EQUIPMENT_LOCATIONS: Dict[int, str] = {
    0: "light_source",
    1: "worn_finger_right", 
    2: "worn_finger_left",
    3: "worn_neck_1",
    4: "worn_neck_2", 
    5: "worn_body",
    6: "worn_head",
    7: "worn_legs",
    8: "worn_feet",
    9: "worn_hands",
    10: "worn_arms",
    11: "worn_shield", 
    12: "worn_about_body",
    13: "worn_waist",
    14: "worn_wrist_right",
    15: "worn_wrist_left",
    16: "wielded_weapon",
    17: "held_item",
    18: "worn_hold",
    19: "worn_badge",
    20: "wielded_twohanded"
}

# Room sector types (magic number -> readable name)
ROOM_SECTORS: Dict[int, str] = {
    0: "indoors",
    1: "city", 
    2: "field",
    3: "forest",
    4: "hills",
    5: "mountains",
    6: "water_shallow",
    7: "water_deep",
    8: "underwater",
    9: "flying",
    10: "desert",
    11: "arctic",
    12: "swamp",
    13: "jungle",
    14: "underground",
    15: "road",
    16: "entrance",
    17: "ruins",
    18: "astral",
    19: "ethereal",
    20: "elemental_air",
    21: "elemental_earth", 
    22: "elemental_fire",
    23: "elemental_water"
}

# Object types (magic number -> readable name)  
OBJECT_TYPES: Dict[int, str] = {
    0: "misc",
    1: "light_source",
    2: "scroll", 
    3: "wand",
    4: "staff",
    5: "weapon",
    6: "furniture",
    7: "component",
    8: "treasure",
    9: "armor",
    10: "potion",
    11: "clothing",
    12: "other",
    13: "trash",
    14: "trap",
    15: "container",
    16: "note",
    17: "liquid_container",
    18: "key",
    19: "food",
    20: "money",
    21: "pen",
    22: "boat",
    23: "fountain",
    24: "portal",
    25: "spellbook"
}

# Character classes (magic number -> readable name)
CHARACTER_CLASSES: Dict[int, str] = {
    0: "warrior",
    1: "cleric", 
    2: "rogue",
    3: "sorcerer",
    4: "paladin",
    5: "ranger",
    6: "bard"
}

# Character races (magic number -> readable name) 
CHARACTER_RACES: Dict[int, str] = {
    0: "human",
    1: "elf",
    2: "dwarf", 
    3: "halfling",
    4: "gnome",
    5: "orc",
    6: "troll",
    7: "ogre",
    8: "minotaur",
    9: "lizardman",
    10: "giant"
}

# Reset modes (magic number -> readable name)
RESET_MODES: Dict[int, str] = {
    0: "never",
    1: "empty", 
    2: "always"
}

# Position types (magic number -> readable name)
POSITIONS: Dict[int, str] = {
    0: "dead",
    1: "mortally_wounded",
    2: "incapacitated", 
    3: "stunned",
    4: "sleeping",
    5: "resting",
    6: "sitting",
    7: "fighting",
    8: "standing",
    9: "mounted"
}

# Gender types (magic number -> readable name)
GENDERS: Dict[int, str] = {
    0: "neutral",
    1: "male",
    2: "female"
}

# Common flag normalization
def normalize_flag_name(flag: str) -> str:
    """Convert a flag name to normalized lowercase form."""
    if not flag:
        return ""
    
    # Remove common prefixes and convert to lowercase
    flag = flag.upper()
    
    # Remove common prefixes
    prefixes = ["AFF_", "MOB_", "OBJ_", "ROOM_", "PLR_", "WEAR_", "ANTI_"]
    for prefix in prefixes:
        if flag.startswith(prefix):
            flag = flag[len(prefix):]
            break
    
    # Convert to lowercase and replace underscores
    flag = flag.lower().replace("_", "-")
    
    # Handle special mappings
    special_mappings = {
        "isnpc": "npc",
        "nobash": "no-bash",
        "nosummon": "no-summon", 
        "nosleep": "no-sleep",
        "nocharm": "no-charm",
        "noclass-ai": "no-class-ai"
    }
    
    return special_mappings.get(flag, flag)

def parse_flag_string(flag_string: str) -> List[str]:
    """Parse a comma-separated flag string into normalized list."""
    if not flag_string or flag_string.strip() == "":
        return []
    
    # Split by comma and clean up each flag
    flags = []
    for flag in flag_string.split(","):
        flag = flag.strip()
        if flag:
            normalized = normalize_flag_name(flag)
            if normalized:
                flags.append(normalized)
    
    return sorted(list(set(flags)))  # Remove duplicates and sort

# Equipment slot validation
def get_equipment_location(slot_num: int) -> str:
    """Get human-readable equipment location name."""
    return EQUIPMENT_LOCATIONS.get(slot_num, f"unknown_slot_{slot_num}")

def get_room_sector(sector_num: int) -> str:
    """Get human-readable room sector name.""" 
    return ROOM_SECTORS.get(sector_num, f"unknown_sector_{sector_num}")

def get_object_type(type_num: int) -> str:
    """Get human-readable object type name."""
    return OBJECT_TYPES.get(type_num, f"unknown_type_{type_num}")

def get_character_class(class_num: int) -> str:
    """Get human-readable character class name."""
    return CHARACTER_CLASSES.get(class_num, f"unknown_class_{class_num}")

def get_character_race(race_num: int) -> str:
    """Get human-readable character race name."""
    return CHARACTER_RACES.get(race_num, f"unknown_race_{race_num}")

def get_reset_mode(mode_num: int) -> str:
    """Get human-readable reset mode name."""
    return RESET_MODES.get(mode_num, f"unknown_mode_{mode_num}")

def get_position(pos_num: int) -> str:
    """Get human-readable position name."""
    return POSITIONS.get(pos_num, f"unknown_position_{pos_num}")

def get_gender(gender_num: int) -> str:
    """Get human-readable gender name."""
    return GENDERS.get(gender_num, f"unknown_gender_{gender_num}")

# Value parsing for different object types
def parse_object_values(obj_type: str, values: List[int]) -> Dict[str, any]:
    """Parse object type-specific values into C++ JSON format."""
    
    if obj_type == "weapon" and len(values) >= 3:
        return {
            "damage_profile": {
                "base_damage": values[0],
                "dice_count": values[1], 
                "dice_sides": values[2],
                "damage_bonus": 0  # Could be values[3] if available
            }
        }
    
    elif obj_type == "armor" and len(values) >= 1:
        return {
            "armor_class": values[0]
        }
    
    elif obj_type == "container" and len(values) >= 3:
        container_info = {
            "capacity": values[0],
            "weight_capacity": values[0] * 10,  # Reasonable default
            "closeable": bool(values[1] & 1),
            "closed": False,
            "lockable": bool(values[1] & 2),
            "locked": False
        }
        if values[2] > 0:
            container_info["key_id"] = values[2]
        
        return {
            "container_info": container_info
        }
    
    elif obj_type == "liquid_container" and len(values) >= 3:
        return {
            "container_info": {
                "capacity": values[0],
                "weight_capacity": values[0] * 10,
                "closeable": True,
                "closed": False,
                "lockable": False,
                "locked": False
            },
            "_liquid_type": values[2],  # Store for reference
            "_current_amount": values[1]
        }
    
    elif obj_type == "light" and len(values) >= 2:
        return {
            "light_info": {
                "duration": values[2] if len(values) > 2 else -1,
                "brightness": 2,  # Default brightness
                "lit": False
            }
        }
    
    # Default: store raw values with warning
    return {"_raw_values": values, "_note": f"Unparsed {obj_type} values"}

def parse_container_flags(flag_value: int) -> List[str]:
    """Parse container flag bitfield.""" 
    flags = []
    if flag_value & 1:
        flags.append("closeable")
    if flag_value & 2:
        flags.append("pickproof") 
    if flag_value & 4:
        flags.append("closed")
    if flag_value & 8:
        flags.append("locked")
    return flags

# Room flags (bitfield flags for room properties)
ROOM_FLAGS: Dict[int, str] = {
    0: "dark",
    1: "death",
    2: "no-mob",
    3: "indoors", 
    4: "peaceful",
    5: "soundproof",
    6: "no-track",
    7: "no-magic",
    8: "tunnel",
    9: "private",
    10: "godroom",
    11: "house",
    12: "house-crash",
    13: "atrium",
    14: "olc",
    15: "bfs-mark",
    16: "vehicle",
    17: "underground",
    18: "current",
    19: "timed-dt",
    20: "earth-bonus",
    21: "air-bonus", 
    22: "fire-bonus",
    23: "water-bonus",
    24: "no-attack"
}

# Direction mappings
DIRECTIONS = {
    0: "north", 1: "east", 2: "south", 3: "west",
    4: "up", 5: "down", 6: "northeast", 7: "northwest", 
    8: "southeast", 9: "southwest"
}

def get_direction(dir_num: int) -> str:
    """Get human-readable direction name."""
    return DIRECTIONS.get(dir_num, f"unknown_direction_{dir_num}")

def get_room_flag(flag_bit: int) -> str:
    """Get human-readable room flag name."""
    return ROOM_FLAGS.get(flag_bit, f"unknown_room_flag_{flag_bit}")

def parse_room_flags(flag_value: int) -> List[str]:
    """Parse room flag bitfield."""
    flags = []
    for bit in range(32):  # Check up to 32 bits
        if flag_value & (1 << bit):
            flag_name = get_room_flag(bit)
            flags.append(flag_name)
    return flags