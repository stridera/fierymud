"""
Validation commands for FieryMUD tools.
"""

import argparse
import json
from pathlib import Path
from typing import List, Optional, Dict, Any, Tuple

from .base import BaseCommand
from mud import MudFiles


class ValidateCommand(BaseCommand):
    """Commands for validating FieryMUD data files and consistency."""
    
    @classmethod
    def add_parser(cls, subparsers: argparse._SubParsersAction) -> argparse.ArgumentParser:
        """Add validate command parser."""
        
        parser = subparsers.add_parser(
            'validate',
            help='Validate FieryMUD data files and consistency',
            description='Check world files, player data, and cross-references for errors'
        )
        
        # Subcommands for validation operations
        validate_subs = parser.add_subparsers(
            dest='validate_action',
            title='Validation Actions',
            description='Available validation operations'
        )
        
        # World validation
        world_parser = validate_subs.add_parser(
            'world',
            help='Validate world files and consistency'
        )
        world_parser.add_argument(
            '--zone', '-z',
            type=int,
            help='Validate specific zone (default: all zones)'
        )
        world_parser.add_argument(
            '--check-refs',
            action='store_true',
            help='Check cross-references (rooms, objects, mobs)'
        )
        world_parser.add_argument(
            '--fix-errors',
            action='store_true',
            help='Attempt to fix validation errors automatically'
        )
        
        # Player validation
        player_parser = validate_subs.add_parser(
            'players',
            help='Validate player files and data'
        )
        player_parser.add_argument(
            '--player', '-p',
            type=str,
            help='Validate specific player (default: all players)'
        )
        player_parser.add_argument(
            '--check-items',
            action='store_true',
            help='Check player inventory item references'
        )
        
        # JSON validation
        json_parser = validate_subs.add_parser(
            'json',
            help='Validate JSON file syntax and structure'
        )
        json_parser.add_argument(
            'files',
            nargs='+',
            type=Path,
            help='JSON files to validate'
        )
        json_parser.add_argument(
            '--schema',
            type=Path,
            help='JSON schema file for validation'
        )
        
        # Cross-reference validation
        refs_parser = validate_subs.add_parser(
            'refs',
            help='Validate cross-references between data files'
        )
        refs_parser.add_argument(
            '--missing-only',
            action='store_true',
            help='Show only missing references'
        )
        refs_parser.add_argument(
            '--generate-report',
            type=Path,
            help='Generate detailed report file'
        )
        
        return parser
    
    @classmethod
    def execute(cls, args: argparse.Namespace) -> int:
        """Execute validate command."""
        
        if not args.validate_action:
            cls.print_error("No validation action specified. Use --help for available actions.")
            return 1
        
        try:
            if args.validate_action == 'world':
                world_path = cls.validate_world_path(args.lib_path)
                return cls._validate_world_files(world_path, args)
            elif args.validate_action == 'players':
                players_path = cls.validate_players_path(args.lib_path)
                return cls._validate_player_files(players_path, args)
            elif args.validate_action == 'json':
                return cls._validate_json_files(args)
            elif args.validate_action == 'refs':
                world_path = cls.validate_world_path(args.lib_path)
                return cls._validate_cross_references(world_path, args)
            else:
                cls.print_error(f"Unknown validation action: {args.validate_action}")
                return 1
                
        except Exception as e:
            cls.print_error(str(e))
            return 1
    
    @classmethod
    def _validate_world_files(cls, world_path: Path, args: argparse.Namespace) -> int:
        """Validate world files and consistency."""
        
        cls.print_info("Starting world file validation...")
        
        error_count = 0
        warning_count = 0
        validated_count = 0
        
        # Get zones to validate
        try:
            zone_files_list = list(MudFiles.zone_files(str(world_path), args.zone))
        except Exception as e:
            cls.print_error(f"Failed to scan zone files: {e}")
            return 1
        
        if not zone_files_list:
            cls.print_warning("No zone files found to validate")
            return 0
        
        total_zones = len(zone_files_list)
        cls.print_info(f"Found {total_zones} zone(s) to validate")
        
        # Track all room/mob/object IDs for cross-reference checking
        all_rooms = set()
        all_mobs = set()
        all_objects = set()
        
        for i, zone_files in enumerate(zone_files_list, 1):
            zone_id = zone_files.id
            
            try:
                cls.print_progress(i, total_zones, "zones")
                print(f"🔍 Validating Zone {zone_id}...")
                
                zone_errors = 0
                zone_warnings = 0
                
                # Check if JSON file exists
                json_file = Path(zone_files.path) / f"{zone_id}.json"
                if json_file.exists():
                    # Validate JSON structure
                    try:
                        with open(json_file) as f:
                            zone_data = json.load(f)
                        
                        # Validate zone structure
                        zone_errs, zone_warns = cls._validate_zone_structure(zone_data, zone_id)
                        zone_errors += zone_errs
                        zone_warnings += zone_warns
                        
                        # Collect IDs for cross-reference checking
                        if 'rooms' in zone_data:
                            for room in zone_data['rooms']:
                                if 'id' in room:
                                    all_rooms.add(room['id'])
                        
                        if 'mobs' in zone_data:
                            for mob in zone_data['mobs']:
                                if 'id' in mob:
                                    all_mobs.add(mob['id'])
                        
                        if 'objects' in zone_data:
                            for obj in zone_data['objects']:
                                if 'id' in obj:
                                    all_objects.add(obj['id'])
                        
                    except json.JSONDecodeError as e:
                        cls.print_error(f"   ❌ Invalid JSON in {json_file}: {e}")
                        zone_errors += 1
                    except Exception as e:
                        cls.print_error(f"   ❌ Error reading {json_file}: {e}")
                        zone_errors += 1
                else:
                    cls.print_warning(f"   ⚠️  No JSON file found for Zone {zone_id}")
                    zone_warnings += 1
                
                # Validate original files
                for file in zone_files.files:
                    try:
                        # Test parsing
                        if hasattr(file, 'parse_world'):
                            data = file.parse_world()
                        else:
                            cls.print_warning(f"   ⚠️  Cannot validate {file.filename} - no parser")
                            zone_warnings += 1
                            
                    except Exception as e:
                        cls.print_error(f"   ❌ Failed to parse {file.filename}: {e}")
                        zone_errors += 1
                
                if zone_errors == 0 and zone_warnings == 0:
                    cls.print_success(f"   ✅ Zone {zone_id} validation passed")
                elif zone_errors == 0:
                    cls.print_warning(f"   ⚠️  Zone {zone_id} validation passed with {zone_warnings} warning(s)")
                else:
                    cls.print_error(f"   ❌ Zone {zone_id} validation failed with {zone_errors} error(s), {zone_warnings} warning(s)")
                
                error_count += zone_errors
                warning_count += zone_warnings
                validated_count += 1
                
            except Exception as e:
                cls.print_error(f"Failed to validate Zone {zone_id}: {e}")
                error_count += 1
        
        # Cross-reference checking
        if args.check_refs:
            cls.print_info("Checking cross-references...")
            ref_errors = cls._check_world_references(world_path, all_rooms, all_mobs, all_objects)
            error_count += ref_errors
        
        # Summary
        print("\n" + "="*50)
        cls.print_info(f"Validation Summary:")
        print(f"   📊 Zones validated: {validated_count}")
        print(f"   ✅ Total rooms found: {len(all_rooms)}")
        print(f"   🧌 Total mobs found: {len(all_mobs)}")
        print(f"   📦 Total objects found: {len(all_objects)}")
        if warning_count > 0:
            print(f"   ⚠️  Total warnings: {warning_count}")
        if error_count > 0:
            print(f"   ❌ Total errors: {error_count}")
        else:
            cls.print_success("All validations passed!")
        
        return 0 if error_count == 0 else 1
    
    @classmethod
    def _validate_zone_structure(cls, zone_data: Dict[str, Any], zone_id: int) -> Tuple[int, int]:
        """Validate the structure of a zone JSON file."""
        
        errors = 0
        warnings = 0
        
        # Required fields
        required_fields = ['id', 'name', 'rooms', 'objects', 'mobs']
        for field in required_fields:
            if field not in zone_data:
                cls.print_error(f"   ❌ Missing required field: {field}")
                errors += 1
        
        # Validate zone ID consistency
        if 'id' in zone_data and zone_data['id'] != zone_id:
            cls.print_error(f"   ❌ Zone ID mismatch: file has {zone_data['id']}, expected {zone_id}")
            errors += 1
        
        # Validate room IDs are in zone range
        if 'rooms' in zone_data:
            for room in zone_data['rooms']:
                if 'id' in room:
                    room_id = room['id']
                    expected_min = zone_id * 100
                    expected_max = zone_id * 100 + 99
                    if not (expected_min <= room_id <= expected_max):
                        cls.print_warning(f"   ⚠️  Room {room_id} outside expected range {expected_min}-{expected_max}")
                        warnings += 1
                
                # Check for required room fields
                required_room_fields = ['id', 'name', 'description']
                for field in required_room_fields:
                    if field not in room:
                        cls.print_error(f"   ❌ Room {room.get('id', 'unknown')} missing field: {field}")
                        errors += 1
        
        # Validate mob and object structures
        for entity_type in ['mobs', 'objects']:
            if entity_type in zone_data:
                for entity in zone_data[entity_type]:
                    if 'id' not in entity:
                        cls.print_error(f"   ❌ {entity_type[:-1]} missing ID field")
                        errors += 1
                    if 'name' not in entity:
                        cls.print_error(f"   ❌ {entity_type[:-1]} {entity.get('id', 'unknown')} missing name")
                        errors += 1
        
        return errors, warnings
    
    @classmethod
    def _check_world_references(cls, world_path: Path, all_rooms: set, all_mobs: set, all_objects: set) -> int:
        """Check cross-references between world files."""
        
        errors = 0
        
        # Check room exit references
        try:
            zone_files_list = list(MudFiles.zone_files(str(world_path)))
            for zone_files in zone_files_list:
                zone_id = zone_files.id
                json_file = Path(zone_files.path) / f"{zone_id}.json"
                
                if json_file.exists():
                    with open(json_file) as f:
                        zone_data = json.load(f)
                    
                    if 'rooms' in zone_data:
                        for room in zone_data['rooms']:
                            room_id = room.get('id', 0)
                            if 'exits' in room:
                                for direction, exit_data in room['exits'].items():
                                    to_room = exit_data.get('to_room', 0)
                                    if to_room > 0 and to_room not in all_rooms:
                                        cls.print_error(f"   ❌ Room {room_id} exit {direction} leads to non-existent room {to_room}")
                                        errors += 1
                    
                    # Check zone command references
                    if 'commands' in zone_data:
                        for cmd in zone_data['commands']:
                            cmd_type = cmd.get('command_type', '')
                            entity_id = cmd.get('entity_id', 0)
                            room_id = cmd.get('room_id', 0)
                            
                            if room_id > 0 and room_id not in all_rooms:
                                cls.print_error(f"   ❌ Zone command references non-existent room {room_id}")
                                errors += 1
                            
                            if cmd_type == 'Load_Mobile' and entity_id not in all_mobs:
                                cls.print_error(f"   ❌ Zone command references non-existent mob {entity_id}")
                                errors += 1
                            elif cmd_type == 'Load_Object' and entity_id not in all_objects:
                                cls.print_error(f"   ❌ Zone command references non-existent object {entity_id}")
                                errors += 1
        
        except Exception as e:
            cls.print_error(f"Error checking references: {e}")
            errors += 1
        
        return errors
    
    @classmethod
    def _validate_player_files(cls, players_path: Path, args: argparse.Namespace) -> int:
        """Validate player files."""
        
        cls.print_info("Starting player file validation...")
        
        error_count = 0
        warning_count = 0
        validated_count = 0
        
        try:
            player_files_list = list(MudFiles.player_files(str(players_path), args.player))
        except Exception as e:
            cls.print_error(f"Failed to scan player files: {e}")
            return 1
        
        if not player_files_list:
            cls.print_warning("No player files found to validate")
            return 0
        
        total_players = len(player_files_list)
        cls.print_info(f"Found {total_players} player(s) to validate")
        
        for i, player_files in enumerate(player_files_list, 1):
            player_name = player_files.id
            
            try:
                cls.print_progress(i, total_players, "players")
                print(f"🔍 Validating Player {player_name}...")
                
                player_errors = 0
                player_warnings = 0
                
                # Validate each player file
                for file in player_files.files:
                    try:
                        data = file.parse_player()
                        
                        # Check for common issues
                        if isinstance(data, dict):
                            # Check for required fields based on file type
                            if file.get_mud_type() == 'Player' and 'name' not in data:
                                cls.print_error(f"   ❌ Player file missing name field")
                                player_errors += 1
                            
                            # Check for negative values that shouldn't be negative
                            for field in ['level', 'hit', 'max_hit', 'move', 'max_move']:
                                if field in data and isinstance(data[field], int) and data[field] < 0:
                                    cls.print_warning(f"   ⚠️  Negative value for {field}: {data[field]}")
                                    player_warnings += 1
                        
                    except Exception as e:
                        cls.print_error(f"   ❌ Failed to parse {file.filename}: {e}")
                        player_errors += 1
                
                # Check JSON file if it exists
                json_file = Path(player_files.path) / f"{player_name}.json"
                if json_file.exists():
                    try:
                        with open(json_file) as f:
                            player_data = json.load(f)
                        
                        # Basic validation of JSON structure
                        if not isinstance(player_data, dict):
                            cls.print_error(f"   ❌ JSON file is not a valid object")
                            player_errors += 1
                        
                    except json.JSONDecodeError as e:
                        cls.print_error(f"   ❌ Invalid JSON: {e}")
                        player_errors += 1
                
                if player_errors == 0 and player_warnings == 0:
                    cls.print_success(f"   ✅ Player {player_name} validation passed")
                elif player_errors == 0:
                    cls.print_warning(f"   ⚠️  Player {player_name} validation passed with {player_warnings} warning(s)")
                else:
                    cls.print_error(f"   ❌ Player {player_name} validation failed with {player_errors} error(s), {player_warnings} warning(s)")
                
                error_count += player_errors
                warning_count += player_warnings
                validated_count += 1
                
            except Exception as e:
                cls.print_error(f"Failed to validate Player {player_name}: {e}")
                error_count += 1
        
        # Summary
        print("\n" + "="*50)
        cls.print_info(f"Validation Summary:")
        print(f"   📊 Players validated: {validated_count}")
        if warning_count > 0:
            print(f"   ⚠️  Total warnings: {warning_count}")
        if error_count > 0:
            print(f"   ❌ Total errors: {error_count}")
        else:
            cls.print_success("All validations passed!")
        
        return 0 if error_count == 0 else 1
    
    @classmethod
    def _validate_json_files(cls, args: argparse.Namespace) -> int:
        """Validate JSON file syntax and structure."""
        
        error_count = 0
        
        for file_path in args.files:
            if not file_path.exists():
                cls.print_error(f"File not found: {file_path}")
                error_count += 1
                continue
            
            try:
                print(f"🔍 Validating {file_path}...")
                
                with open(file_path) as f:
                    data = json.load(f)
                
                cls.print_success(f"   ✅ Valid JSON syntax")
                
                # Schema validation if provided
                if args.schema:
                    try:
                        import jsonschema
                        
                        with open(args.schema) as f:
                            schema = json.load(f)
                        
                        jsonschema.validate(data, schema)
                        cls.print_success(f"   ✅ Schema validation passed")
                        
                    except ImportError:
                        cls.print_warning("   ⚠️  jsonschema package not available for schema validation")
                    except Exception as e:
                        cls.print_error(f"   ❌ Schema validation failed: {e}")
                        error_count += 1
                
            except json.JSONDecodeError as e:
                cls.print_error(f"   ❌ Invalid JSON syntax: {e}")
                error_count += 1
            except Exception as e:
                cls.print_error(f"   ❌ Error reading file: {e}")
                error_count += 1
        
        return 0 if error_count == 0 else 1
    
    @classmethod
    def _validate_cross_references(cls, world_path: Path, args: argparse.Namespace) -> int:
        """Validate cross-references between all data files."""
        
        cls.print_info("Starting comprehensive cross-reference validation...")
        
        # This would implement more thorough cross-reference checking
        # For now, delegate to the world validation with refs enabled
        mock_args = argparse.Namespace()
        mock_args.zone = None
        mock_args.check_refs = True
        mock_args.fix_errors = False
        mock_args.verbose = getattr(args, 'verbose', False)
        
        return cls._validate_world_files(world_path, mock_args)