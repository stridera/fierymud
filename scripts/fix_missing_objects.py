#!/usr/bin/env python3
"""
Fix missing object references in world data.
Removes or comments out equipment references to non-existent objects.
"""

import json
import os
import shutil

# Missing object IDs identified by analysis
MISSING_OBJECTS = {1208, 2222, 16909, 20036, 23770, 23771}

def backup_file(filepath):
    """Create a backup of the file before modifying."""
    backup_path = filepath + '.backup'
    shutil.copy2(filepath, backup_path)
    print(f"Created backup: {backup_path}")

def fix_zone_file(filepath):
    """Fix missing object references in a zone file."""
    with open(filepath, 'r') as f:
        data = json.load(f)
    
    zone_name = data.get('zone', {}).get('name', os.path.basename(filepath))
    changes_made = False
    
    if 'zone' in data and 'resets' in data['zone'] and 'mob' in data['zone']['resets']:
        for mob_reset in data['zone']['resets']['mob']:
            # Fix equipment
            if 'equipped' in mob_reset:
                original_equipped = mob_reset['equipped'][:]
                mob_reset['equipped'] = [
                    eq for eq in mob_reset['equipped'] 
                    if eq.get('id') not in MISSING_OBJECTS
                ]
                if len(mob_reset['equipped']) != len(original_equipped):
                    removed = [eq for eq in original_equipped if eq.get('id') in MISSING_OBJECTS]
                    for eq in removed:
                        print(f"  Removed missing equipment {eq.get('id')} from mob {mob_reset.get('id', '?')}")
                    changes_made = True
            
            # Fix inventory
            if 'inventory' in mob_reset:
                original_inventory = mob_reset['inventory'][:]
                mob_reset['inventory'] = [
                    inv for inv in mob_reset['inventory']
                    if inv.get('id') not in MISSING_OBJECTS
                ]
                if len(mob_reset['inventory']) != len(original_inventory):
                    removed = [inv for inv in original_inventory if inv.get('id') in MISSING_OBJECTS]
                    for inv in removed:
                        print(f"  Removed missing inventory {inv.get('id')} from mob {mob_reset.get('id', '?')}")
                    changes_made = True
    
    # Fix object resets
    if 'zone' in data and 'resets' in data['zone'] and 'obj' in data['zone']['resets']:
        original_obj_resets = data['zone']['resets']['obj'][:]
        data['zone']['resets']['obj'] = [
            obj for obj in data['zone']['resets']['obj']
            if obj.get('id') not in MISSING_OBJECTS
        ]
        if len(data['zone']['resets']['obj']) != len(original_obj_resets):
            removed = [obj for obj in original_obj_resets if obj.get('id') in MISSING_OBJECTS]
            for obj in removed:
                print(f"  Removed missing object reset {obj.get('id')}")
            changes_made = True
    
    if changes_made:
        backup_file(filepath)
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
        print(f"Fixed zone: {zone_name}")
    
    return changes_made

def main():
    world_dir = "data/world"
    
    if not os.path.exists(world_dir):
        print(f"World directory {world_dir} not found")
        return 1
    
    print("Fixing missing object references...")
    
    total_fixes = 0
    for filename in os.listdir(world_dir):
        if filename.endswith('.json'):
            filepath = os.path.join(world_dir, filename)
            try:
                if fix_zone_file(filepath):
                    total_fixes += 1
            except Exception as e:
                print(f"Error fixing {filename}: {e}")
    
    print(f"\nCompleted: Fixed {total_fixes} zone files")
    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main())