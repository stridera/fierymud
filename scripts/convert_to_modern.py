#!/usr/bin/env python3
"""
Convert Python-generated JSON to modern FieryMUD format
"""
import json
import sys
import argparse

def convert_zone_to_modern_format(legacy_json):
    """Convert legacy Python format to modern unified format"""
    
    zone_data = legacy_json.get("ZONE", {})
    world_data = legacy_json.get("WORLD", {})
    
    # Extract zone number from name (format: "#30 ")
    zone_name = zone_data.get("name", "#0 ")
    zone_id = int(zone_name.replace("#", "").strip())
    
    # Build modern zone format
    modern_zone = {
        "id": zone_id,
        "name": zone_name.strip(),
        "description": f"Zone {zone_id}",
        "short_description": f"Zone {zone_id}",
        "keywords": [f"zone{zone_id}"],
        "reset_minutes": zone_data.get("lifespan", 30),
        "reset_mode": zone_data.get("reset_mode", "Normal"),
        "min_level": 1,
        "max_level": 100,
        "first_room": zone_id * 100,
        "last_room": zone_data.get("top", zone_id * 100 + 99),
        "commands": [],
        "flags": [],
        "rooms": [],
        "objects": [],
        "mobs": []
    }
    
    # Convert rooms
    if world_data and "rooms" in world_data:
        for room_data in world_data["rooms"]:
            room_name = room_data.get("name", "")
            # Extract room ID from name (format: "#3001 ")
            room_id = int(room_name.replace("#", "").strip()) if room_name.startswith("#") else 0
            
            modern_room = {
                "id": room_id,
                "name": room_name.strip(),
                "description": room_data.get("description", ""),
                "short_description": room_name.strip(),
                "keywords": ["room"],
                "exits": {},
                "flags": [],
                "sector": "indoors"
            }
            
            # Convert exits
            if "exits" in room_data and room_data["exits"]:
                for direction, exit_data in room_data["exits"].items():
                    modern_room["exits"][direction.lower()] = {
                        "to_room": int(exit_data.get("destination", 0)),
                        "description": exit_data.get("description", ""),
                        "keywords": exit_data.get("keyword", "").split(),
                        "is_door": exit_data.get("type", "Normal") != "Normal",
                        "is_closed": False,
                        "is_locked": False,
                        "key_vnum": int(exit_data.get("key", -1)) if exit_data.get("key", "-1") != "-1" else 0
                    }
            
            modern_zone["rooms"].append(modern_room)
    
    # Convert mobs from zone commands
    zone_commands = zone_data.get("commands", {})
    if "mob" in zone_commands:
        for mob_command in zone_commands["mob"]:
            mob_id = mob_command.get("vnum", 0)
            modern_mob = {
                "id": mob_id,
                "name": mob_command.get("name", f"mob {mob_id}").strip("()"),
                "description": f"This is {mob_command.get('name', f'mob {mob_id}')}.",
                "short_description": mob_command.get("name", f"mob {mob_id}").strip("()"),
                "keywords": ["mob"],
                "level": 1,
                "max_hit": 100,
                "hit": 100,
                "max_move": 100,
                "move": 100,
                "flags": [],
                "position": "standing"
            }
            modern_zone["mobs"].append(modern_mob)
    
    # Convert objects from zone commands  
    if "object" in zone_commands:
        for obj_command in zone_commands["object"]:
            obj_id = obj_command.get("vnum", 0)
            modern_obj = {
                "id": obj_id,
                "name": obj_command.get("name", f"object {obj_id}").strip("()"),
                "description": f"This is {obj_command.get('name', f'object {obj_id}')}.",
                "short_description": obj_command.get("name", f"object {obj_id}").strip("()"),
                "keywords": ["object"],
                "type": "misc",
                "weight": 1,
                "value": 0,
                "level": 1,
                "flags": []
            }
            modern_zone["objects"].append(modern_obj)
    
    return modern_zone

def main():
    parser = argparse.ArgumentParser(description="Convert legacy Python JSON to modern format")
    parser.add_argument("input_file", help="Input JSON file from Python converter")
    parser.add_argument("-o", "--output", help="Output file (default: stdout)")
    
    args = parser.parse_args()
    
    # Read input file
    with open(args.input_file, 'r') as f:
        legacy_json = json.load(f)
    
    # Convert to modern format
    modern_json = convert_zone_to_modern_format(legacy_json)
    
    # Write output
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(modern_json, f, indent=2)
        print(f"Converted {args.input_file} to modern format: {args.output}")
    else:
        print(json.dumps(modern_json, indent=2))

if __name__ == "__main__":
    main()