#!/usr/bin/env python3
"""
Analyze room reference issues in world data.
Find missing room references in zone files.
"""

import json
import os

def find_all_rooms(world_dir):
    """Find all room IDs defined in world files."""
    rooms = set()
    
    for filename in os.listdir(world_dir):
        if filename.endswith('.json'):
            filepath = os.path.join(world_dir, filename)
            try:
                with open(filepath, 'r') as f:
                    data = json.load(f)
                    
                # Check for rooms in this zone
                if 'rooms' in data:
                    for room in data['rooms']:
                        if 'id' in room:
                            rooms.add(int(room['id']))
                            
            except Exception as e:
                print(f"Error reading {filename}: {e}")
                
    return rooms

def find_room_references(world_dir):
    """Find all room references in zone resets and equipment."""
    references = {}
    
    for filename in os.listdir(world_dir):
        if filename.endswith('.json'):
            filepath = os.path.join(world_dir, filename)
            try:
                with open(filepath, 'r') as f:
                    data = json.load(f)
                    
                zone_name = data.get('zone', {}).get('name', filename)
                zone_refs = set()
                
                # Check mobile spawns and inventory assignments
                if 'zone' in data and 'resets' in data['zone'] and 'mob' in data['zone']['resets']:
                    for mob_reset in data['zone']['resets']['mob']:
                        # Mobile spawn room
                        if 'room' in mob_reset:
                            zone_refs.add(int(mob_reset['room']))
                        
                        # Check inventory (might reference rooms incorrectly)
                        if 'inventory' in mob_reset:
                            for inv in mob_reset['inventory']:
                                # Some inventory items might have room references
                                if 'room' in inv:
                                    zone_refs.add(int(inv['room']))
                
                # Check object spawns in rooms
                if 'zone' in data and 'resets' in data['zone'] and 'obj' in data['zone']['resets']:
                    for obj_reset in data['zone']['resets']['obj']:
                        if 'room' in obj_reset:
                            zone_refs.add(int(obj_reset['room']))
                
                if zone_refs:
                    references[zone_name] = zone_refs
                    
            except Exception as e:
                print(f"Error reading {filename}: {e}")
                
    return references

def main():
    world_dir = "data/world"
    
    if not os.path.exists(world_dir):
        print(f"World directory {world_dir} not found")
        return 1
        
    print("Analyzing room references...")
    
    # Find all defined rooms and all references
    defined_rooms = find_all_rooms(world_dir)
    zone_room_refs = find_room_references(world_dir)
    
    print(f"Found {len(defined_rooms)} room definitions")
    
    # Check each zone's room references
    missing_rooms = set()
    
    for zone_name, room_refs in zone_room_refs.items():
        zone_missing = room_refs - defined_rooms
        if zone_missing:
            print(f"\n❌ Zone '{zone_name}' references missing rooms: {sorted(zone_missing)}")
            missing_rooms.update(zone_missing)
    
    if missing_rooms:
        print(f"\n💔 Total missing rooms: {sorted(missing_rooms)}")
    else:
        print("\n✅ All room references are valid")
        
    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main())