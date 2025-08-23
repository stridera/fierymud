"""
World file management commands for FieryMUD tools.
"""

import argparse
import json
from pathlib import Path
from typing import List, Optional, Dict, Any

from .base import BaseCommand
from .world_mappings import (
    EQUIPMENT_LOCATIONS, ROOM_SECTORS, OBJECT_TYPES, CHARACTER_CLASSES,
    CHARACTER_RACES, RESET_MODES, POSITIONS, GENDERS, ROOM_FLAGS,
    parse_flag_string, get_equipment_location, get_room_sector,
    get_object_type, get_character_class, get_character_race,
    get_reset_mode, get_position, get_gender, parse_object_values,
    get_direction, get_room_flag, parse_room_flags
)
from mud import MudFiles, Encoder


class WorldCommand(BaseCommand):
    """Commands for managing world files (zones, rooms, objects, mobs)."""
    
    @classmethod
    def add_parser(cls, subparsers: argparse._SubParsersAction) -> argparse.ArgumentParser:
        """Add world command parser."""
        
        parser = subparsers.add_parser(
            'world',
            help='Manage world files (zones, rooms, objects, mobs)',
            description='Manage and validate modern FieryMUD JSON world files'
        )
        
        # Subcommands for world operations
        world_subs = parser.add_subparsers(
            dest='world_action',
            title='World Actions',
            description='Available world file operations'
        )
        
        # List command
        list_parser = world_subs.add_parser(
            'list',
            help='List available zones and their status'
        )
        list_parser.add_argument(
            '--format',
            choices=['table', 'json', 'simple'],
            default='table',
            help='Output format'
        )
        
        # Info command
        info_parser = world_subs.add_parser(
            'info',
            help='Show detailed information about a zone'
        )
        info_parser.add_argument(
            'zone',
            type=int,
            help='Zone number to inspect'
        )
        
        # Validate command
        validate_parser = world_subs.add_parser(
            'validate',
            help='Validate world file structure and consistency'
        )
        validate_parser.add_argument(
            '--zone', '-z',
            type=int,
            help='Validate specific zone (default: all zones)'
        )
        validate_parser.add_argument(
            '--fix-errors',
            action='store_true',
            help='Attempt to fix validation errors automatically'
        )
        
        # Create command
        create_parser = world_subs.add_parser(
            'create',
            help='Create a new zone template'
        )
        create_parser.add_argument(
            'zone_id',
            type=int,
            help='Zone ID to create'
        )
        create_parser.add_argument(
            '--name',
            type=str,
            help='Zone name (default: Zone {id})'
        )
        create_parser.add_argument(
            '--rooms',
            type=int,
            default=3,
            help='Number of template rooms to create (default: 3)'
        )
        
        # Convert legacy command
        legacy_parser = world_subs.add_parser(
            'convert-legacy',
            help='Convert legacy CircleMUD files to modern JSON format'
        )
        legacy_parser.add_argument(
            '--legacy-path',
            type=Path,
            default=Path('../lib_legacy/world'),
            help='Path to legacy world files (default: ../lib_legacy/world)'
        )
        legacy_parser.add_argument(
            '--zone', '-z',
            type=int,
            help='Convert specific zone (default: all zones)'
        )
        legacy_parser.add_argument(
            '--target-dir', '--output', '-o',
            type=Path,
            help='Target directory for converted files (default: current world directory)'
        )
        legacy_parser.add_argument(
            '--force', '-f',
            action='store_true',
            help='Overwrite existing JSON files'
        )
        legacy_parser.add_argument(
            '--backup',
            action='store_true',
            help='Create backup of existing files before overwriting'
        )
        
        return parser
    
    @classmethod
    def execute(cls, args: argparse.Namespace) -> int:
        """Execute world command."""
        
        if not args.world_action:
            cls.print_error("No world action specified. Use --help for available actions.")
            return 1
        
        try:
            world_path = cls.validate_world_path(args.lib_path)
            
            if args.world_action == 'list':
                return cls._list_zones(world_path, args)
            elif args.world_action == 'info':
                return cls._show_zone_info(world_path, args)
            elif args.world_action == 'validate':
                return cls._validate_zones(world_path, args)
            elif args.world_action == 'create':
                return cls._create_zone(world_path, args)
            elif args.world_action == 'convert-legacy':
                return cls._convert_legacy_files(world_path, args)
            else:
                cls.print_error(f"Unknown world action: {args.world_action}")
                return 1
                
        except Exception as e:
            cls.print_error(str(e))
            return 1
    
    @classmethod
    def _find_zone_files(cls, world_path: Path) -> List[Dict[str, Any]]:
        """Find all zone JSON files in the world directory."""
        
        zones = []
        
        # Check zon directory for JSON files
        zon_dir = world_path / 'zon'
        if zon_dir.exists():
            for json_file in zon_dir.glob('*.json'):
                try:
                    zone_id = int(json_file.stem)
                    zones.append({
                        'id': zone_id,
                        'path': json_file,
                        'type': 'zone'
                    })
                except ValueError:
                    continue
        
        # Check wld directory for JSON files 
        wld_dir = world_path / 'wld'
        if wld_dir.exists():
            for json_file in wld_dir.glob('*.json'):
                try:
                    zone_id = int(json_file.stem)
                    zones.append({
                        'id': zone_id,
                        'path': json_file,
                        'type': 'world'
                    })
                except ValueError:
                    continue
        
        # Sort by zone ID
        zones.sort(key=lambda x: x['id'])
        return zones
    
    @classmethod
    def _list_zones(cls, world_path: Path, args: argparse.Namespace) -> int:
        """List available zones."""
        
        zones = cls._find_zone_files(world_path)
        
        if not zones:
            cls.print_warning("No zones found")
            return 0
        
        zones_info = []
        for zone in zones:
            zone_id = zone['id']
            zone_path = zone['path']
            
            info = {
                'zone_id': zone_id,
                'type': zone['type'],
                'has_json': zone_path.exists(),
                'path': str(zone_path.parent),
                'file_size': zone_path.stat().st_size if zone_path.exists() else 0,
                'rooms': 0,
                'mobs': 0,
                'objects': 0
            }
            
            # Try to get detailed info from JSON
            try:
                with open(zone_path) as f:
                    data = json.load(f)
                if isinstance(data, dict):
                    info['rooms'] = len(data.get('rooms', []))
                    info['mobs'] = len(data.get('mobs', []))
                    info['objects'] = len(data.get('objects', []))
            except Exception:
                pass
                
            zones_info.append(info)
        
        # Output in requested format
        if args.format == 'json':
            print(json.dumps(zones_info, indent=2))
        elif args.format == 'simple':
            for info in zones_info:
                print(f"Zone {info['zone_id']} ({info['type']})")
        else:  # table format
            print(f"{'Zone':<6} {'Type':<6} {'Rooms':<6} {'Mobs':<6} {'Objs':<6} {'Size':<10} {'Path'}")
            print("-" * 70)
            for info in zones_info:
                size_str = f"{info['file_size']:,}B" if info['file_size'] > 0 else "0B"
                print(f"{info['zone_id']:<6} {info['type']:<6} {info['rooms']:<6} {info['mobs']:<6} "
                      f"{info['objects']:<6} {size_str:<10} {Path(info['path']).name}")
        
        return 0
    
    @classmethod
    def _show_zone_info(cls, world_path: Path, args: argparse.Namespace) -> int:
        """Show detailed information about a specific zone."""
        
        zone_id = args.zone
        zones = cls._find_zone_files(world_path)
        
        # Find the zone
        zone_info = next((z for z in zones if z['id'] == zone_id), None)
        
        if not zone_info:
            cls.print_error(f"Zone {zone_id} not found")
            return 1
        
        zone_path = zone_info['path']
        
        try:
            print(f"🏰 Zone {zone_id} Information")
            print("=" * 40)
            print(f"📁 Path: {zone_path}")
            print(f"📄 Type: {zone_info['type']}")
            print(f"💾 Size: {zone_path.stat().st_size:,} bytes")
            
            # Try to load and show detailed stats
            with open(zone_path) as f:
                data = json.load(f)
            
            if isinstance(data, dict):
                print(f"🏷️  Name: {data.get('name', 'Unknown')}")
                print(f"📝 Description: {data.get('description', 'None')[:100]}{'...' if len(data.get('description', '')) > 100 else ''}")
                
                if 'reset_minutes' in data:
                    print(f"⏰ Reset Time: {data['reset_minutes']} minutes")
                if 'min_level' in data and 'max_level' in data:
                    print(f"📊 Level Range: {data['min_level']}-{data['max_level']}")
                
                # Room statistics
                if 'rooms' in data:
                    rooms = data['rooms']
                    print(f"\n🏠 Rooms: {len(rooms)}")
                    if rooms:
                        room_ids = [r.get('id', 0) for r in rooms if 'id' in r]
                        if room_ids:
                            print(f"   ID Range: {min(room_ids)}-{max(room_ids)}")
                        
                        # Count room features
                        rooms_with_exits = sum(1 for r in rooms if r.get('exits'))
                        print(f"   With Exits: {rooms_with_exits}")
                        
                        # Sample room names
                        sample_rooms = rooms[:3]
                        for room in sample_rooms:
                            print(f"   • [{room.get('id', '?')}] {room.get('name', 'Unnamed')}")
                        if len(rooms) > 3:
                            print(f"   ... and {len(rooms) - 3} more")
                
                # Mob statistics
                if 'mobs' in data:
                    mobs = data['mobs']
                    print(f"\n🧌 Mobs: {len(mobs)}")
                    if mobs:
                        mob_ids = [m.get('id', 0) for m in mobs if 'id' in m]
                        if mob_ids:
                            print(f"   ID Range: {min(mob_ids)}-{max(mob_ids)}")
                        
                        # Level statistics
                        levels = [m.get('level', 1) for m in mobs if 'level' in m]
                        if levels:
                            print(f"   Level Range: {min(levels)}-{max(levels)}")
                        
                        # Sample mobs
                        sample_mobs = mobs[:3]
                        for mob in sample_mobs:
                            level = mob.get('level', '?')
                            print(f"   • [{mob.get('id', '?')}] {mob.get('name', 'Unnamed')} (Level {level})")
                        if len(mobs) > 3:
                            print(f"   ... and {len(mobs) - 3} more")
                
                # Object statistics
                if 'objects' in data:
                    objects = data['objects']
                    print(f"\n📦 Objects: {len(objects)}")
                    if objects:
                        obj_ids = [o.get('id', 0) for o in objects if 'id' in o]
                        if obj_ids:
                            print(f"   ID Range: {min(obj_ids)}-{max(obj_ids)}")
                        
                        # Type statistics
                        types = [o.get('object_type', 'misc') for o in objects if 'object_type' in o]
                        type_counts = {}
                        for obj_type in types:
                            type_counts[obj_type] = type_counts.get(obj_type, 0) + 1
                        
                        if type_counts:
                            print("   Types:")
                            for obj_type, count in sorted(type_counts.items()):
                                print(f"     {obj_type}: {count}")
                        
                        # Sample objects
                        sample_objects = objects[:3]
                        for obj in sample_objects:
                            obj_type = obj.get('object_type', 'misc')
                            print(f"   • [{obj.get('id', '?')}] {obj.get('name', 'Unnamed')} ({obj_type})")
                        if len(objects) > 3:
                            print(f"   ... and {len(objects) - 3} more")
                
                # Zone commands
                if 'commands' in data:
                    commands = data['commands']
                    print(f"\n⚙️  Zone Commands: {len(commands)}")
                    if commands:
                        cmd_types = [c.get('command_type', 'unknown') for c in commands]
                        cmd_counts = {}
                        for cmd_type in cmd_types:
                            cmd_counts[cmd_type] = cmd_counts.get(cmd_type, 0) + 1
                        
                        print("   Command Types:")
                        for cmd_type, count in sorted(cmd_counts.items()):
                            print(f"     {cmd_type}: {count}")
                
        except json.JSONDecodeError as e:
            cls.print_error(f"Failed to parse JSON: {e}")
            return 1
        except Exception as e:
            cls.print_error(f"Error reading zone file: {e}")
            return 1
        
        return 0
    
    @classmethod
    def _validate_zones(cls, world_path: Path, args: argparse.Namespace) -> int:
        """Validate world file structure and consistency."""
        
        zones = cls._find_zone_files(world_path)
        
        if args.zone:
            # Validate specific zone
            zones = [z for z in zones if z['id'] == args.zone]
            if not zones:
                cls.print_error(f"Zone {args.zone} not found")
                return 1
        
        if not zones:
            cls.print_warning("No zones found to validate")
            return 0
        
        cls.print_info(f"Validating {len(zones)} zone(s)...")
        
        total_errors = 0
        total_warnings = 0
        
        # Track all entity IDs for cross-reference checking
        all_rooms = set()
        all_mobs = set()
        all_objects = set()
        
        for zone in zones:
            zone_id = zone['id']
            zone_path = zone['path']
            
            print(f"\n🔍 Validating Zone {zone_id}...")
            
            zone_errors = 0
            zone_warnings = 0
            
            try:
                with open(zone_path) as f:
                    data = json.load(f)
                
                # Validate basic structure
                if not isinstance(data, dict):
                    cls.print_error(f"   ❌ Zone file is not a valid JSON object")
                    zone_errors += 1
                    continue
                
                # Check required fields
                required_fields = ['id', 'name']
                for field in required_fields:
                    if field not in data:
                        cls.print_error(f"   ❌ Missing required field: {field}")
                        zone_errors += 1
                
                # Validate zone ID consistency
                if 'id' in data and data['id'] != zone_id:
                    cls.print_error(f"   ❌ Zone ID mismatch: file has {data['id']}, expected {zone_id}")
                    zone_errors += 1
                
                # Validate rooms
                if 'rooms' in data:
                    for i, room in enumerate(data['rooms']):
                        if not isinstance(room, dict):
                            cls.print_error(f"   ❌ Room {i} is not a valid object")
                            zone_errors += 1
                            continue
                        
                        room_id = room.get('id')
                        if room_id is None:
                            cls.print_error(f"   ❌ Room {i} missing ID")
                            zone_errors += 1
                        else:
                            all_rooms.add(room_id)
                            
                            # Check room ID range (traditional CircleMUD convention)
                            expected_min = zone_id * 100
                            expected_max = zone_id * 100 + 99
                            if not (expected_min <= room_id <= expected_max):
                                cls.print_warning(f"   ⚠️  Room {room_id} outside expected range {expected_min}-{expected_max}")
                                zone_warnings += 1
                        
                        # Check required room fields
                        required_room_fields = ['name', 'description']
                        for field in required_room_fields:
                            if field not in room or not room[field]:
                                cls.print_error(f"   ❌ Room {room_id} missing or empty {field}")
                                zone_errors += 1
                        
                        # Validate exits
                        if 'exits' in room:
                            for direction, exit_data in room['exits'].items():
                                if not isinstance(exit_data, dict):
                                    cls.print_error(f"   ❌ Room {room_id} exit {direction} is not valid")
                                    zone_errors += 1
                                    continue
                                
                                to_room = exit_data.get('to_room')
                                if to_room is not None and to_room <= 0:
                                    cls.print_warning(f"   ⚠️  Room {room_id} exit {direction} leads to invalid room {to_room}")
                                    zone_warnings += 1
                
                # Validate mobs
                if 'mobs' in data:
                    for i, mob in enumerate(data['mobs']):
                        if not isinstance(mob, dict):
                            cls.print_error(f"   ❌ Mob {i} is not a valid object")
                            zone_errors += 1
                            continue
                        
                        mob_id = mob.get('id')
                        if mob_id is None:
                            cls.print_error(f"   ❌ Mob {i} missing ID")
                            zone_errors += 1
                        else:
                            all_mobs.add(mob_id)
                        
                        # Check required mob fields
                        required_mob_fields = ['name', 'description']
                        for field in required_mob_fields:
                            if field not in mob or not mob[field]:
                                cls.print_error(f"   ❌ Mob {mob_id} missing or empty {field}")
                                zone_errors += 1
                        
                        # Validate numeric fields
                        numeric_fields = ['level', 'max_hit', 'hit', 'max_move', 'move']
                        for field in numeric_fields:
                            if field in mob:
                                value = mob[field]
                                if not isinstance(value, (int, float)) or value < 0:
                                    cls.print_warning(f"   ⚠️  Mob {mob_id} has invalid {field}: {value}")
                                    zone_warnings += 1
                
                # Validate objects
                if 'objects' in data:
                    for i, obj in enumerate(data['objects']):
                        if not isinstance(obj, dict):
                            cls.print_error(f"   ❌ Object {i} is not a valid object")
                            zone_errors += 1
                            continue
                        
                        obj_id = obj.get('id')
                        if obj_id is None:
                            cls.print_error(f"   ❌ Object {i} missing ID")
                            zone_errors += 1
                        else:
                            all_objects.add(obj_id)
                        
                        # Check required object fields
                        required_obj_fields = ['name', 'description']
                        for field in required_obj_fields:
                            if field not in obj or not obj[field]:
                                cls.print_error(f"   ❌ Object {obj_id} missing or empty {field}")
                                zone_errors += 1
                        
                        # Validate numeric fields
                        numeric_fields = ['weight', 'value', 'level']
                        for field in numeric_fields:
                            if field in obj:
                                value = obj[field]
                                if not isinstance(value, (int, float)) or value < 0:
                                    cls.print_warning(f"   ⚠️  Object {obj_id} has invalid {field}: {value}")
                                    zone_warnings += 1
                
                # Validate zone commands
                if 'commands' in data:
                    for i, cmd in enumerate(data['commands']):
                        if not isinstance(cmd, dict):
                            cls.print_error(f"   ❌ Command {i} is not a valid object")
                            zone_errors += 1
                            continue
                        
                        cmd_type = cmd.get('command_type')
                        if not cmd_type:
                            cls.print_error(f"   ❌ Command {i} missing command_type")
                            zone_errors += 1
                        
                        # Check entity references
                        entity_id = cmd.get('entity_id')
                        room_id = cmd.get('room_id')
                        
                        if room_id and room_id not in all_rooms:
                            cls.print_warning(f"   ⚠️  Command {i} references unknown room {room_id}")
                            zone_warnings += 1
                        
                        if cmd_type == 'Load_Mobile' and entity_id and entity_id not in all_mobs:
                            cls.print_warning(f"   ⚠️  Command {i} references unknown mob {entity_id}")
                            zone_warnings += 1
                        elif cmd_type == 'Load_Object' and entity_id and entity_id not in all_objects:
                            cls.print_warning(f"   ⚠️  Command {i} references unknown object {entity_id}")
                            zone_warnings += 1
                
                # Report zone results
                if zone_errors == 0 and zone_warnings == 0:
                    cls.print_success(f"   ✅ Zone {zone_id} validation passed")
                elif zone_errors == 0:
                    cls.print_warning(f"   ⚠️  Zone {zone_id} validation passed with {zone_warnings} warning(s)")
                else:
                    cls.print_error(f"   ❌ Zone {zone_id} validation failed with {zone_errors} error(s), {zone_warnings} warning(s)")
                
                total_errors += zone_errors
                total_warnings += zone_warnings
                
            except json.JSONDecodeError as e:
                cls.print_error(f"   ❌ Invalid JSON: {e}")
                total_errors += 1
            except Exception as e:
                cls.print_error(f"   ❌ Error validating zone: {e}")
                total_errors += 1
        
        # Summary
        print("\n" + "="*50)
        cls.print_info(f"Validation Summary:")
        print(f"   📊 Zones validated: {len(zones)}")
        print(f"   🏠 Total rooms: {len(all_rooms)}")
        print(f"   🧌 Total mobs: {len(all_mobs)}")
        print(f"   📦 Total objects: {len(all_objects)}")
        if total_warnings > 0:
            print(f"   ⚠️  Total warnings: {total_warnings}")
        if total_errors > 0:
            print(f"   ❌ Total errors: {total_errors}")
        else:
            cls.print_success("All validations passed!")
        
        return 0 if total_errors == 0 else 1
    
    @classmethod
    def _create_zone(cls, world_path: Path, args: argparse.Namespace) -> int:
        """Create a new zone template."""
        
        zone_id = args.zone_id
        zone_name = args.name or f"Zone {zone_id}"
        
        # Check if zone already exists
        zones = cls._find_zone_files(world_path)
        existing_zone = next((z for z in zones if z['id'] == zone_id), None)
        
        if existing_zone:
            cls.print_error(f"Zone {zone_id} already exists at {existing_zone['path']}")
            return 1
        
        # Create zone template
        zone_data = {
            "id": zone_id,
            "name": zone_name,
            "description": f"A new zone created for development",
            "short_description": zone_name,
            "keywords": [f"zone{zone_id}"],
            "reset_minutes": 30,
            "reset_mode": "Empty",
            "min_level": 1,
            "max_level": 10,
            "first_room": zone_id * 100,
            "last_room": zone_id * 100 + 99,
            "commands": [],
            "flags": [],
            "rooms": [],
            "objects": [],
            "mobs": []
        }
        
        # Create template rooms
        room_base = zone_id * 100
        for i in range(args.rooms):
            room_id = room_base + i
            room = {
                "id": room_id,
                "name": f"Room {i + 1}",
                "description": f"This is room {i + 1} in {zone_name}. It's a basic template room that can be customized.",
                "short_description": f"Room {i + 1}",
                "keywords": ["room"],
                "exits": {},
                "flags": [],
                "sector": "indoors"
            }
            
            # Connect rooms in a simple line
            if i > 0:
                room["exits"]["west"] = {
                    "to_room": room_base + i - 1,
                    "description": "A passage leads west",
                    "keywords": ["west", "passage"],
                    "is_door": False,
                    "is_closed": False,
                    "is_locked": False,
                    "key_id": 0
                }
            
            if i < args.rooms - 1:
                room["exits"]["east"] = {
                    "to_room": room_base + i + 1,
                    "description": "A passage leads east", 
                    "keywords": ["east", "passage"],
                    "is_door": False,
                    "is_closed": False,
                    "is_locked": False,
                    "key_id": 0
                }
            
            zone_data["rooms"].append(room)
        
        # Create template objects
        template_objects = [
            {
                "id": zone_id * 100 + 1,
                "name": "a simple torch",
                "description": "A basic torch that provides light in dark areas.",
                "short_description": "a simple torch",
                "keywords": ["torch", "light"],
                "object_type": "Light",
                "weight": 1,
                "value": 5,
                "level": 1,
                "flags": []
            }
        ]
        zone_data["objects"].extend(template_objects)
        
        # Create template mobs
        template_mobs = [
            {
                "id": zone_id * 100 + 1,
                "name": "a helpful guide",
                "description": "A friendly guide who can help new adventurers learn the basics.",
                "short_description": "a helpful guide",
                "keywords": ["guide", "helper"],
                "level": 1,
                "max_hit": 50,
                "hit": 50,
                "max_move": 100,
                "move": 100,
                "flags": ["peaceful"],
                "position": "standing"
            }
        ]
        zone_data["mobs"].extend(template_mobs)
        
        # Add some basic zone commands
        zone_data["commands"] = [
            {
                "command_type": "Load_Mobile",
                "if_flag": 1,
                "entity_id": zone_id * 100 + 1,
                "room_id": zone_id * 100,
                "max_count": 1,
                "comment": "Load helpful guide in first room"
            },
            {
                "command_type": "Load_Object",
                "if_flag": 1,
                "entity_id": zone_id * 100 + 1,
                "room_id": zone_id * 100,
                "max_count": 1,
                "comment": "Load torch in first room"
            }
        ]
        
        # Write zone file
        zon_dir = world_path / 'zon'
        zon_dir.mkdir(exist_ok=True)
        zone_file = zon_dir / f"{zone_id}.json"
        
        try:
            with open(zone_file, 'w', encoding='utf-8') as f:
                json.dump(zone_data, f, indent=2, ensure_ascii=False)
                f.write('\n')
            
            cls.print_success(f"Created Zone {zone_id}: {zone_file}")
            cls.print_info(f"Zone contains {len(zone_data['rooms'])} rooms, {len(zone_data['mobs'])} mobs, {len(zone_data['objects'])} objects")
            
            return 0
            
        except Exception as e:
            cls.print_error(f"Failed to create zone file: {e}")
            return 1
    
    @classmethod
    def _convert_legacy_files(cls, world_path: Path, args: argparse.Namespace) -> int:
        """Convert legacy CircleMUD files to modern JSON format."""
        
        legacy_path = args.legacy_path
        if not legacy_path.exists():
            cls.print_error(f"Legacy world path not found: {legacy_path}")
            return 1
        
        cls.print_info("Starting legacy world file conversion...")
        cls.print_info(f"Source: {legacy_path}")
        cls.print_info(f"Target: {world_path}")
        
        # Determine output directory
        output_dir = getattr(args, 'target_dir', None) or getattr(args, 'output', None) or world_path
        
        target_dir = getattr(args, 'target_dir', None) or getattr(args, 'output', None)
        if target_dir and not target_dir.exists():
            target_dir.mkdir(parents=True, exist_ok=True)
            cls.print_info(f"Created output directory: {target_dir}")
        
        converted_count = 0
        error_count = 0
        
        # Get zones to process using MudFiles
        try:
            zone_files_list = list(MudFiles.zone_files(str(legacy_path), args.zone))
        except Exception as e:
            cls.print_error(f"Failed to scan legacy zone files: {e}")
            cls.print_info("Make sure the legacy path contains valid CircleMUD zone files")
            return 1
        
        if not zone_files_list:
            cls.print_warning("No legacy zone files found to convert")
            cls.print_info(f"Checked path: {legacy_path}")
            cls.print_info("Expected to find .zon, .wld, .mob, .obj, .trg files")
            return 0
        
        total_zones = len(zone_files_list)
        cls.print_info(f"Found {total_zones} legacy zone(s) to convert")
        
        for i, zone_files in enumerate(zone_files_list, 1):
            zone_id = zone_files.id
            
            try:
                cls.print_progress(i, total_zones, "zones")
                print(f"🔄 Converting Legacy Zone {zone_id}...")
                
                # Parse all legacy files for this zone
                zone_data = {}
                for file in zone_files.files:
                    if args.verbose:
                        print(f"   📄 Processing {file.get_mud_type()} file: {file.filename}")
                    try:
                        file_data = file.parse_world()
                        zone_data[file.mud_type.name] = file_data.to_json() if hasattr(file_data, 'to_json') else file_data
                    except Exception as e:
                        cls.print_warning(f"   ⚠️  Failed to parse {file.filename}: {e}")
                        continue
                
                if not zone_data:
                    cls.print_error(f"   ❌ No valid data found for Zone {zone_id}")
                    error_count += 1
                    continue
                
                # Determine output file location
                output_dir.mkdir(exist_ok=True)
                output_file = output_dir / f"{zone_id}.json"
                
                # Check if file exists and handle accordingly
                if output_file.exists() and not args.force:
                    cls.print_warning(f"Skipping Zone {zone_id} - output file exists (use --force to overwrite)")
                    continue
                
                # Create backup if requested
                if args.backup and output_file.exists():
                    backup_file = output_file.with_suffix('.json.bak')
                    backup_file.write_text(output_file.read_text())
                    print(f"   💾 Created backup: {backup_file}")
                
                # Convert to modern unified format
                try:
                    modern_zone_data = cls._convert_to_modern_format(zone_data, zone_id)
                except Exception as e:
                    cls.print_error(f"   ❌ Failed to convert Zone {zone_id} to modern format: {e}")
                    error_count += 1
                    continue
                
                # Write modern JSON file
                with open(output_file, 'w', encoding='utf-8') as f:
                    json.dump(modern_zone_data, f, indent=2, ensure_ascii=False)
                    f.write('\n')  # Add final newline
                
                cls.print_success(f"Converted Legacy Zone {zone_id} → {output_file}")
                print(f"   📊 Contains: {len(modern_zone_data.get('rooms', []))} rooms, "
                      f"{len(modern_zone_data.get('mobs', []))} mobs, "
                      f"{len(modern_zone_data.get('objects', []))} objects")
                
                converted_count += 1
                
            except Exception as e:
                cls.print_error(f"Failed to convert Legacy Zone {zone_id}: {e}")
                if args.verbose:
                    import traceback
                    traceback.print_exc()
                error_count += 1
        
        # Summary
        print("\n" + "="*50)
        cls.print_info(f"Legacy Conversion Summary:")
        print(f"   ✅ Successfully converted: {converted_count}")
        if error_count > 0:
            print(f"   ❌ Failed conversions: {error_count}")
        
        if converted_count > 0:
            cls.print_info("Converted files are in modern unified JSON format")
            cls.print_info("You can now use 'world list', 'world info', and 'world validate' commands")
        
        return 0 if error_count == 0 else 1
    
    @classmethod
    def _convert_to_modern_format(cls, legacy_data: Dict[str, Any], zone_id: int) -> Dict[str, Any]:
        """Convert legacy parsed data to modern unified format with human-readable structure."""
        
        # Ensure zone_id is an integer to avoid string concatenation errors
        zone_id = int(zone_id)

        # Start with a modern zone template
        modern_zone = {
            "id": zone_id,
            "name": f"Zone {zone_id}",
            "description": f"Converted legacy zone {zone_id}",
            "short_description": f"Zone {zone_id}",
            "keywords": [f"zone{zone_id}"],
            "reset_minutes": 30,
            "reset_mode": "empty",
            "min_level": 1,
            "max_level": 100,
            "first_room": zone_id * 100,
            "last_room": zone_id * 100 + 99,
            "commands": [],
            "flags": [],
            "rooms": [],
            "objects": [],
            "mobs": []
        }
        
        # Convert zone data if available
        if 'ZONE' in legacy_data:
            zone_info = legacy_data['ZONE']
            if isinstance(zone_info, dict):
                # Clean up zone name
                zone_name = zone_info.get('name', f"Zone {zone_id}").strip()
                if zone_name.startswith(f"#{zone_id}"):
                    zone_name = zone_name[len(f"#{zone_id}"):].strip()
                
                modern_zone.update({
                    "name": zone_name,
                    "reset_minutes": zone_info.get('lifespan', 30),
                    "reset_mode": get_reset_mode(zone_info.get('reset_mode', 1)),
                    "last_room": zone_info.get('top', zone_id * 100 + 99)
                })
                
                # Convert zone commands with readable structure
                if 'commands' in zone_info:
                    commands = zone_info['commands']
                    if isinstance(commands, dict):
                        modern_zone['commands'] = cls._convert_zone_commands(commands)
                    elif isinstance(commands, list):
                        modern_zone['commands'] = commands
        
        # Convert room data with clean structure
        if 'WORLD' in legacy_data:
            world_info = legacy_data['WORLD']
            # Handle both dict format and MUD library object format
            if hasattr(world_info, 'rooms'):
                # MUD library object format
                rooms = world_info.rooms
                if isinstance(rooms, list):
                    modern_zone['rooms'] = [cls._convert_room(room) for room in rooms]
            elif isinstance(world_info, dict) and 'rooms' in world_info:
                # Dict format
                rooms = world_info['rooms']
                if isinstance(rooms, list):
                    modern_zone['rooms'] = [cls._convert_room(room) for room in rooms]
        
        # Convert mob data with clean structure - key is 'MOB' not 'MOBILE'
        if 'MOB' in legacy_data:
            mob_info = legacy_data['MOB']
            if isinstance(mob_info, list):
                converted_mobs = []
                for mob in mob_info:
                    try:
                        converted_mobs.append(cls._convert_mobile(mob.to_json() if hasattr(mob, 'to_json') else cls._object_to_dict(mob)))
                    except Exception as e:
                        print(f"   ⚠️  Warning: Failed to convert mob {getattr(mob, 'id', 'unknown')}: {e}")
                        continue
                modern_zone['mobs'] = converted_mobs
            elif hasattr(mob_info, '__len__') and hasattr(mob_info, '__iter__'):
                # Handle MUD library object format like rooms
                converted_mobs = []
                for mob in mob_info:
                    try:
                        converted_mobs.append(cls._convert_mobile(mob.to_json() if hasattr(mob, 'to_json') else cls._object_to_dict(mob)))
                    except Exception as e:
                        print(f"   ⚠️  Warning: Failed to convert mob {getattr(mob, 'id', 'unknown')}: {e}")
                        continue
                modern_zone['mobs'] = converted_mobs
            elif isinstance(mob_info, dict) and 'mobs' in mob_info:
                mobs = mob_info['mobs']
                if isinstance(mobs, list):
                    converted_mobs = []
                    for mob in mobs:
                        try:
                            converted_mobs.append(cls._convert_mobile(mob.to_json() if hasattr(mob, 'to_json') else cls._object_to_dict(mob)))
                        except Exception as e:
                            print(f"   ⚠️  Warning: Failed to convert mob {getattr(mob, 'id', 'unknown')}: {e}")
                            continue
                    modern_zone['mobs'] = converted_mobs
        
        # Convert object data with clean structure
        if 'OBJECT' in legacy_data:
            obj_info = legacy_data['OBJECT']
            if isinstance(obj_info, list):
                modern_zone['objects'] = [cls._convert_object(cls._object_to_dict(obj)) for obj in obj_info]
            elif isinstance(obj_info, dict) and 'objects' in obj_info:
                objects = obj_info['objects']
                if isinstance(objects, list):
                    modern_zone['objects'] = [cls._convert_object(cls._object_to_dict(obj)) for obj in objects]
        
        # Set reasonable defaults if we got data
        if modern_zone['rooms']:
            room_ids = [r.get('id', 0) for r in modern_zone['rooms'] if isinstance(r, dict) and 'id' in r]
            if room_ids:
                modern_zone['first_room'] = min(room_ids)
                modern_zone['last_room'] = max(room_ids)
        
        # Infer level range from mobs if available
        if modern_zone['mobs']:
            mob_levels = [m.get('level', 1) for m in modern_zone['mobs'] if isinstance(m, dict) and 'level' in m]
            if mob_levels:
                modern_zone['min_level'] = max(1, min(mob_levels) - 2)
                modern_zone['max_level'] = max(mob_levels) + 2
        
        return modern_zone
    
    @classmethod
    def _convert_zone_commands(cls, commands: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Convert zone commands to clean structure."""
        converted_commands = []
        
        # Handle mobile commands
        if 'mob' in commands and isinstance(commands['mob'], list):
            for mob_cmd in commands['mob']:
                cmd = {
                    "command_type": "load_mobile",
                    "mobile_id": mob_cmd.get('id', 0),
                    "room_id": mob_cmd.get('room', 0),
                    "max_count": mob_cmd.get('max', 1),
                    "comment": mob_cmd.get('name', '').strip('()')
                }
                
                # Add equipment if present
                if 'equipped' in mob_cmd and mob_cmd['equipped']:
                    cmd['equipment'] = []
                    for equip in mob_cmd['equipped']:
                        cmd['equipment'].append({
                            "object_id": equip.get('id', 0),
                            "location": get_equipment_location(equip.get('location', 0)),
                            "max_count": equip.get('max', 1),
                            "comment": equip.get('name', '').strip('()')
                        })
                
                # Add inventory if present  
                if 'carrying' in mob_cmd and mob_cmd['carrying']:
                    cmd['inventory'] = []
                    for item in mob_cmd['carrying']:
                        cmd['inventory'].append({
                            "object_id": item.get('id', 0),
                            "max_count": item.get('max', 1),
                            "comment": item.get('name', '').strip('()')
                        })
                
                converted_commands.append(cmd)
        
        # Handle object commands
        if 'object' in commands and isinstance(commands['object'], list):
            for obj_cmd in commands['object']:
                cmd = {
                    "command_type": "load_object",
                    "object_id": obj_cmd.get('id', 0),
                    "room_id": obj_cmd.get('room', 0),
                    "max_count": obj_cmd.get('max', 1),
                    "comment": obj_cmd.get('name', '').strip('()')
                }
                converted_commands.append(cmd)
        
        # Handle door commands
        if 'door' in commands and isinstance(commands['door'], list):
            for door_cmd in commands['door']:
                states = door_cmd.get('state', [])
                cmd = {
                    "command_type": "set_door",
                    "room_id": door_cmd.get('room', 0),
                    "direction": door_cmd.get('direction', '').lower(),
                    "door_state": {
                        "closed": "CLOSED" in states,
                        "locked": "LOCKED" in states,
                        "hidden": "HIDDEN" in states
                    }
                }
                converted_commands.append(cmd)
        
        return converted_commands
    
    @classmethod
    def _convert_room(cls, room: Dict[str, Any]) -> Dict[str, Any]:
        """Convert a room to clean structure."""
        if not isinstance(room, dict):
            return cls._make_serializable(room)
        print(room)
        converted = {
            "id": cls._safe_get_int(room, 'id', 0),
            "name": cls._clean_string(room.get('name', 'Unnamed Room')),
            "description": cls._clean_string(room.get('description', '')),
            "short_description": cls._clean_string(room.get('short_description', room.get('name', ''))),
            "keywords": cls._parse_keywords(room.get('name_list', '')),
            "sector_type": get_room_sector(cls._safe_get_int(room, 'sector_type', 0)),
            "flags": parse_room_flags(cls._safe_get_int(room, 'room_flags', 0)) if isinstance(room.get('room_flags'), (int, float)) else parse_flag_string(cls._safe_get_string(room, 'room_flags', '')),
            "exits": {}
        }
        
        # Convert exits to readable format
        if 'exits' in room and isinstance(room['exits'], dict):
            for direction, exit_data in room['exits'].items():
                if isinstance(exit_data, dict):
                    converted['exits'][direction] = {
                        "to_room": cls._safe_get_int(exit_data, 'to_room', 0),
                        "description": cls._clean_string(exit_data.get('description', '')),
                        "keywords": cls._parse_keywords(exit_data.get('keyword', '')),
                        "door": {
                            "present": bool(exit_data.get('is_door', False)),
                            "closed": bool(exit_data.get('is_closed', False)),
                            "locked": bool(exit_data.get('is_locked', False)),
                            "key_id": cls._safe_get_int(exit_data, 'key_id', 0) if cls._safe_get_int(exit_data, 'key_id', 0) > 0 else None
                        } if exit_data.get('is_door') else None
                    }
        
        return converted
    
    @classmethod  
    def _convert_mobile(cls, mobile: Dict[str, Any]) -> Dict[str, Any]:
        """Convert a mobile to clean structure."""
        if not isinstance(mobile, dict):
            return cls._make_serializable(mobile)
            
        # Handle dice objects properly
        hp_dice = mobile.get('hp_dice', {})
        damage_dice = mobile.get('damage_dice', {})
        
        # Extract dice values
        hp_max = 100  # default
        if isinstance(hp_dice, dict):
            # Calculate max HP from dice: number * sides + bonus
            number = cls._safe_get_int(hp_dice, 'number', 1)
            sides = cls._safe_get_int(hp_dice, 'sides', 100)
            bonus = cls._safe_get_int(hp_dice, 'bonus', 0)
            hp_max = (number * sides) + bonus
        elif hasattr(hp_dice, 'number') and hasattr(hp_dice, 'sides'):
            # Handle dice object
            number = getattr(hp_dice, 'number', 1)
            sides = getattr(hp_dice, 'sides', 100)
            bonus = getattr(hp_dice, 'bonus', 0)
            hp_max = (number * sides) + bonus
            
        # Handle damage dice
        damage_info = {}
        if isinstance(damage_dice, dict):
            damage_info = {
                "dice_count": cls._safe_get_int(damage_dice, 'number', 1),
                "dice_sides": cls._safe_get_int(damage_dice, 'sides', 4),
                "bonus": cls._safe_get_int(damage_dice, 'bonus', 0)
            }
        elif hasattr(damage_dice, 'number') and hasattr(damage_dice, 'sides'):
            damage_info = {
                "dice_count": getattr(damage_dice, 'number', 1),
                "dice_sides": getattr(damage_dice, 'sides', 4),
                "bonus": getattr(damage_dice, 'bonus', 0)
            }
            
        # Handle money object
        money_info = {}
        money_obj = mobile.get('money')
        if money_obj:
            if isinstance(money_obj, dict):
                money_info = {
                    "copper": cls._safe_get_int(money_obj, 'copper', 0),
                    "silver": cls._safe_get_int(money_obj, 'silver', 0),
                    "gold": cls._safe_get_int(money_obj, 'gold', 0),
                    "platinum": cls._safe_get_int(money_obj, 'platinum', 0)
                }
            elif hasattr(money_obj, 'copper'):
                money_info = {
                    "copper": getattr(money_obj, 'copper', 0),
                    "silver": getattr(money_obj, 'silver', 0),
                    "gold": getattr(money_obj, 'gold', 0),
                    "platinum": getattr(money_obj, 'platinum', 0)
                }
                
        # Handle stats object
        stats_info = {}
        stats_obj = mobile.get('stats')
        if stats_obj:
            if isinstance(stats_obj, dict):
                stats_info = {
                    "strength": cls._safe_get_int(stats_obj, 'strength', 13),
                    "intelligence": cls._safe_get_int(stats_obj, 'intelligence', 13),
                    "wisdom": cls._safe_get_int(stats_obj, 'wisdom', 13),
                    "dexterity": cls._safe_get_int(stats_obj, 'dexterity', 13),
                    "constitution": cls._safe_get_int(stats_obj, 'constitution', 13),
                    "charisma": cls._safe_get_int(stats_obj, 'charisma', 13)
                }
            elif hasattr(stats_obj, 'strength'):
                stats_info = {
                    "strength": getattr(stats_obj, 'strength', 13),
                    "intelligence": getattr(stats_obj, 'intelligence', 13),
                    "wisdom": getattr(stats_obj, 'wisdom', 13),
                    "dexterity": getattr(stats_obj, 'dexterity', 13),
                    "constitution": getattr(stats_obj, 'constitution', 13),
                    "charisma": getattr(stats_obj, 'charisma', 13)
                }
        
        # Handle mob_class properly (it's a Class enum, not a simple int)
        mob_class_value = mobile.get('mob_class')
        if hasattr(mob_class_value, 'value'):
            class_num = mob_class_value.value
        elif isinstance(mob_class_value, (int, float)):
            class_num = int(mob_class_value)
        else:
            class_num = 0
            
        # Handle race properly
        race_value = mobile.get('race')
        if hasattr(race_value, 'value'):
            race_num = race_value.value
        elif isinstance(race_value, (int, float)):
            race_num = int(race_value)
        else:
            race_num = 0
            
        # Handle position properly
        position_value = mobile.get('position')
        if hasattr(position_value, 'value'):
            position_num = position_value.value
        elif isinstance(position_value, (int, float)):
            position_num = int(position_value)
        else:
            position_num = 8  # standing
            
        # Handle default position
        default_position_value = mobile.get('default_position')
        if hasattr(default_position_value, 'value'):
            default_position_num = default_position_value.value
        elif isinstance(default_position_value, (int, float)):
            default_position_num = int(default_position_value)
        else:
            default_position_num = position_num
            
        # Handle gender properly
        gender_value = mobile.get('gender')
        if hasattr(gender_value, 'value'):
            gender_num = gender_value.value
        elif isinstance(gender_value, (int, float)):
            gender_num = int(gender_value)
        else:
            gender_num = 0
            
        # Handle flags properly (they might be lists already)
        mob_flags = mobile.get('mob_flags', [])
        if isinstance(mob_flags, list):
            mob_flags_list = mob_flags
        else:
            mob_flags_list = parse_flag_string(cls._safe_get_string(mobile, 'mob_flags', ''))
            
        effect_flags = mobile.get('effect_flags', [])
        if isinstance(effect_flags, list):
            effect_flags_list = effect_flags
        else:
            effect_flags_list = parse_flag_string(cls._safe_get_string(mobile, 'effect_flags', ''))
            
        converted = {
            "id": cls._safe_get_int(mobile, 'id', mobile.get('id', 0)),
            "name": cls._clean_string(mobile.get('short_description', mobile.get('name', 'Unnamed Mobile'))),
            "short_description": cls._clean_string(mobile.get('short_description', '')),
            "long_description": cls._clean_string(mobile.get('long_description', '')),
            "description": cls._clean_string(mobile.get('description', '')),
            "keywords": cls._parse_keywords(mobile.get('name_list', '')),
            "level": cls._safe_get_int(mobile, 'level', 1),
            "character_class": get_character_class(class_num),
            "race": get_character_race(race_num),
            "gender": get_gender(gender_num),
            "alignment": cls._safe_get_int(mobile, 'alignment', 0),
            "race_alignment": cls._safe_get_int(mobile, 'race_align', 0),
            "size": cls._safe_get_int(mobile, 'size', 6),  # Medium size default
            "flags": mob_flags_list,
            "effects": effect_flags_list,
            "combat": {
                "hit_points": {
                    "current": hp_max,
                    "max": hp_max,
                    "dice": {
                        "number": getattr(hp_dice, 'number', 1) if hasattr(hp_dice, 'number') else cls._safe_get_int(hp_dice, 'number', 1),
                        "sides": getattr(hp_dice, 'sides', 100) if hasattr(hp_dice, 'sides') else cls._safe_get_int(hp_dice, 'sides', 100),
                        "bonus": getattr(hp_dice, 'bonus', 0) if hasattr(hp_dice, 'bonus') else cls._safe_get_int(hp_dice, 'bonus', 0)
                    }
                },
                "move_points": cls._safe_get_int(mobile, 'move', 100),
                "armor_class": cls._safe_get_int(mobile, 'ac', 10),
                "hit_roll": cls._safe_get_int(mobile, 'hit_roll', 0),
                "damage": damage_info,
                "experience_value": cls._safe_get_int(mobile, 'exp', cls._safe_get_int(mobile, 'level', 1) * 100)
            },
            "position": get_position(position_num),
            "default_position": get_position(default_position_num),
            "stats": stats_info,
            "special": {
                "perception": cls._safe_get_int(mobile, 'perception', 0),
                "concealment": cls._safe_get_int(mobile, 'concealment', 0),
                "life_force": cls._safe_get_string(mobile, 'life_force', 'life'),
                "composition": cls._safe_get_string(mobile, 'composition', 'flesh'),
                "stance": cls._safe_get_string(mobile, 'stance', 'standing'),
                "bare_hand_attack": mobile.get('bare_hand_attack')
            }
        }
        
        # Add money if present
        if money_info:
            converted["money"] = money_info
            
        return converted
    
    @classmethod
    def _convert_object(cls, obj: Dict[str, Any]) -> Dict[str, Any]:
        """Convert an object to clean structure."""
        if not isinstance(obj, dict):
            # Convert non-dict objects to serializable format
            return cls._make_serializable(obj)
            
        obj_type_num = cls._safe_get_int(obj, 'type', 0)
        obj_type = get_object_type(obj_type_num)
        
        # Parse wear locations to get primary equip slot
        wear_locations = parse_flag_string(cls._safe_get_string(obj, 'wear_flags', ''))
        equip_slot = wear_locations[0] if wear_locations else None
        
        converted = {
            "id": cls._safe_get_int(obj, 'id', obj.get('id', 0)),
            "name": cls._clean_string(obj.get('short_description', obj.get('name', 'Unnamed Object'))),
            "short_description": cls._clean_string(obj.get('short_description', '')),
            "description": cls._clean_string(obj.get('long_description', obj.get('description', ''))),
            "keywords": cls._parse_keywords(obj.get('name_list', '')),
            "object_type": obj_type,
            "weight": cls._safe_get_int(obj, 'weight', 0),
            "value": cls._safe_get_int(obj, 'cost', obj.get('value', 0)),
            "condition": 100,  # Default condition
            "flags": parse_flag_string(cls._safe_get_string(obj, 'extra_flags', ''))
        }
        
        # Add equip slot if object can be worn/wielded
        if equip_slot:
            converted["equip_slot"] = equip_slot
        
        # Add type-specific data
        values = obj.get('value', [0, 0, 0, 0, 0, 0, 0])
        if isinstance(values, list) and len(values) > 0:
            type_data = parse_object_values(obj_type, values)
            converted.update(type_data)
        
        return converted
    
    @classmethod
    def _clean_string(cls, text: str) -> str:
        """Clean up legacy string formatting."""
        if not isinstance(text, str):
            return str(text) if text is not None else ""
            
        # Remove trailing tildes and clean whitespace
        text = text.rstrip('~').strip()
        
        # Handle color codes (preserve for now, but could convert to modern format)
        # This preserves existing color codes while cleaning structure
        
        return text
    
    @classmethod
    def _parse_keywords(cls, keyword_string: str) -> List[str]:
        """Parse keyword string into list."""
        if not keyword_string or not isinstance(keyword_string, str):
            return []
            
        # Split on spaces and clean each keyword
        keywords = [kw.strip() for kw in keyword_string.replace('~', '').split() if kw.strip()]
        
        return keywords
    
    @classmethod
    def _safe_get_int(cls, data: Dict[str, Any], key: str, default: int = 0) -> int:
        """Safely get an integer value from a dictionary."""
        try:
            value = data.get(key, default)
            if isinstance(value, (int, float)):
                return int(value)
            elif isinstance(value, str):
                return int(float(value)) if value.isdigit() or value.replace('.', '').replace('-', '').isdigit() else default
            else:
                return default
        except (ValueError, TypeError):
            return default
    
    @classmethod
    def _safe_get_string(cls, data: Dict[str, Any], key: str, default: str = "") -> str:
        """Safely get a string value from a dictionary."""
        try:
            value = data.get(key, default)
            return str(value) if value is not None else default
        except (ValueError, TypeError):
            return default
    
    @classmethod
    def _make_serializable(cls, obj: Any) -> Any:
        """Convert an object to JSON serializable format."""
        if obj is None:
            return None
        elif isinstance(obj, (str, int, float, bool)):
            return obj
        elif isinstance(obj, (list, tuple)):
            return [cls._make_serializable(item) for item in obj]
        elif isinstance(obj, dict):
            return {str(k): cls._make_serializable(v) for k, v in obj.items()}
        else:
            # For unknown objects, try to convert to string
            try:
                return str(obj)
            except:
                return f"<{type(obj).__name__}>"
    
    @classmethod
    def _object_to_dict(cls, obj: Any) -> Dict[str, Any]:
        """Convert MUD library objects to dictionary format for processing."""
        if isinstance(obj, dict):
            return obj
        elif hasattr(obj, '__dict__'):
            # Convert object attributes to dictionary
            obj_dict = {}
            for attr_name in dir(obj):
                if not attr_name.startswith('_'):  # Skip private attributes
                    try:
                        attr_value = getattr(obj, attr_name)
                        if not callable(attr_value):  # Skip methods
                            obj_dict[attr_name] = cls._make_serializable(attr_value)
                    except:
                        continue
            return obj_dict
        else:
            # Fallback: try to convert to dict if possible
            try:
                return dict(obj)
            except:
                return {"raw_data": cls._make_serializable(obj)}