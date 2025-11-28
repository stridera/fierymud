#!/usr/bin/env python3
"""
Simple script to analyze world data consistency issues.
Finds missing object prototypes referenced in zone files.
"""

import json
import os
import sys
from collections import defaultdict

def find_all_object_prototypes(world_dir):
    """Find all object prototypes defined in world files."""
    objects = set()
    
    for filename in os.listdir(world_dir):
        if filename.endswith('.json'):
            filepath = os.path.join(world_dir, filename)
            try:
                with open(filepath, 'r') as f:
                    data = json.load(f)
                    
                # Check for objects in this zone
                if 'objects' in data:
                    for obj in data['objects']:
                        if 'id' in obj:
                            objects.add(int(obj['id']))
                            
            except Exception as e:
                print(f"Error reading {filename}: {e}")
                
    return objects

def find_all_object_references(world_dir):
    """Find all object references in zone resets and equipment."""
    references = defaultdict(list)
    
    for filename in os.listdir(world_dir):
        if filename.endswith('.json'):
            filepath = os.path.join(world_dir, filename)
            try:
                with open(filepath, 'r') as f:
                    data = json.load(f)
                    
                zone_name = data.get('zone', {}).get('name', filename)
                
                # Check object resets
                if 'zone' in data and 'resets' in data['zone']:
                    resets = data['zone']['resets']
                    
                    # Object spawns in rooms
                    if 'obj' in resets:
                        for obj_reset in resets['obj']:
                            if 'id' in obj_reset:
                                obj_id = int(obj_reset['id'])
                                references[obj_id].append(f"{zone_name}: room spawn")
                    
                    # Equipment on mobiles  
                    if 'mob' in resets:
                        for mob_reset in resets['mob']:
                            if 'equipped' in mob_reset:
                                for equip in mob_reset['equipped']:
                                    if 'id' in equip:
                                        obj_id = int(equip['id'])
                                        references[obj_id].append(f"{zone_name}: mob equipment")
                            
                            if 'inventory' in mob_reset:
                                for inv in mob_reset['inventory']:
                                    if 'id' in inv:
                                        obj_id = int(inv['id'])
                                        references[obj_id].append(f"{zone_name}: mob inventory")
                                        
            except Exception as e:
                print(f"Error reading {filename}: {e}")
                
    return references

def main():
    world_dir = "data/world"
    
    if not os.path.exists(world_dir):
        print(f"World directory {world_dir} not found")
        return 1
        
    print("Analyzing world data consistency...")
    
    # Find all defined objects and all references
    defined_objects = find_all_object_prototypes(world_dir)
    referenced_objects = find_all_object_references(world_dir)
    
    print(f"Found {len(defined_objects)} object prototypes")
    print(f"Found {len(referenced_objects)} unique object references")
    
    # Find missing objects
    missing_objects = set(referenced_objects.keys()) - defined_objects
    
    if missing_objects:
        print(f"\n❌ Found {len(missing_objects)} missing object prototypes:")
        for obj_id in sorted(missing_objects):
            print(f"\nObject {obj_id}:")
            for ref in referenced_objects[obj_id]:
                print(f"  - {ref}")
    else:
        print("\n✅ All object references are valid")
        
    # Find unused objects
    unused_objects = defined_objects - set(referenced_objects.keys())
    if unused_objects:
        print(f"\n⚠️  Found {len(unused_objects)} unused object prototypes: {sorted(unused_objects)}")
        
    return 0

if __name__ == "__main__":
    sys.exit(main())