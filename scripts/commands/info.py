"""
Information and diagnostics commands for FieryMUD tools.
"""

import argparse
import json
import os
import sys
from pathlib import Path
from typing import List, Optional, Dict, Any

from .base import BaseCommand
from mud import MudFiles


class InfoCommand(BaseCommand):
    """Commands for getting information about FieryMUD installation and data."""
    
    @classmethod
    def add_parser(cls, subparsers: argparse._SubParsersAction) -> argparse.ArgumentParser:
        """Add info command parser."""
        
        parser = subparsers.add_parser(
            'info',
            help='Get information about FieryMUD installation and data',
            description='Display diagnostics, statistics, and system information'
        )
        
        # Subcommands for info operations
        info_subs = parser.add_subparsers(
            dest='info_action',
            title='Information Actions',
            description='Available information commands'
        )
        
        # System info
        system_parser = info_subs.add_parser(
            'system',
            help='Show system and installation information'
        )
        system_parser.add_argument(
            '--detailed',
            action='store_true',
            help='Show detailed system information'
        )
        
        # World statistics
        world_parser = info_subs.add_parser(
            'world',
            help='Show world statistics and information'
        )
        world_parser.add_argument(
            '--zone', '-z',
            type=int,
            help='Show statistics for specific zone'
        )
        world_parser.add_argument(
            '--format',
            choices=['table', 'json', 'summary'],
            default='summary',
            help='Output format'
        )
        
        # Player statistics
        players_parser = info_subs.add_parser(
            'players',
            help='Show player statistics and information'
        )
        players_parser.add_argument(
            '--format',
            choices=['table', 'json', 'summary'],
            default='summary',
            help='Output format'
        )
        players_parser.add_argument(
            '--active-days',
            type=int,
            default=30,
            help='Days to consider for recent activity (default: 30)'
        )
        
        # Dependencies info
        deps_parser = info_subs.add_parser(
            'deps',
            help='Show Python dependencies and versions'
        )
        
        # Config info
        config_parser = info_subs.add_parser(
            'config',
            help='Show FieryMUD configuration information'
        )
        config_parser.add_argument(
            '--paths-only',
            action='store_true',
            help='Show only path information'
        )
        
        return parser
    
    @classmethod
    def execute(cls, args: argparse.Namespace) -> int:
        """Execute info command."""
        
        if not args.info_action:
            cls.print_error("No info action specified. Use --help for available actions.")
            return 1
        
        try:
            if args.info_action == 'system':
                return cls._show_system_info(args)
            elif args.info_action == 'world':
                world_path = cls.validate_world_path(args.lib_path)
                return cls._show_world_info(world_path, args)
            elif args.info_action == 'players':
                players_path = cls.validate_players_path(args.lib_path)
                return cls._show_players_info(players_path, args)
            elif args.info_action == 'deps':
                return cls._show_dependencies_info()
            elif args.info_action == 'config':
                return cls._show_config_info(args)
            else:
                cls.print_error(f"Unknown info action: {args.info_action}")
                return 1
                
        except Exception as e:
            cls.print_error(str(e))
            return 1
    
    @classmethod
    def _show_system_info(cls, args: argparse.Namespace) -> int:
        """Show system and installation information."""
        
        print("🖥️  FieryMUD Tools System Information")
        print("=" * 50)
        
        # Python information
        print(f"🐍 Python Version: {sys.version}")
        print(f"📂 Python Executable: {sys.executable}")
        print(f"📋 Platform: {sys.platform}")
        
        # Script location
        script_path = Path(__file__).parent.parent.absolute()
        print(f"🛠️  Tools Location: {script_path}")
        
        # Environment variables
        if args.detailed:
            print("\n📊 Environment Variables:")
            mud_env_vars = {k: v for k, v in os.environ.items() if 'MUD' in k.upper() or 'FIERY' in k.upper()}
            if mud_env_vars:
                for key, value in mud_env_vars.items():
                    print(f"   {key}: {value}")
            else:
                print("   No MUD-related environment variables found")
        
        # Check for common dependencies
        print("\n📦 Dependencies Status:")
        dependencies = [
            ('mud', 'MUD file parsing'),
            ('json', 'JSON processing'),
            ('pathlib', 'Path handling'),
            ('argparse', 'Command line parsing'),
        ]
        
        for dep, description in dependencies:
            try:
                __import__(dep)
                print(f"   ✅ {dep}: Available ({description})")
            except ImportError:
                print(f"   ❌ {dep}: Missing ({description})")
        
        # Optional dependencies
        optional_deps = [
            ('jsonschema', 'JSON schema validation'),
            ('tqdm', 'Progress bars'),
            ('colorama', 'Colored output'),
        ]
        
        print("\n📦 Optional Dependencies:")
        for dep, description in optional_deps:
            try:
                __import__(dep)
                print(f"   ✅ {dep}: Available ({description})")
            except ImportError:
                print(f"   ⭕ {dep}: Optional ({description})")
        
        return 0
    
    @classmethod
    def _show_world_info(cls, world_path: Path, args: argparse.Namespace) -> int:
        """Show world statistics and information."""
        
        try:
            zone_files_list = list(MudFiles.zone_files(str(world_path), args.zone))
        except Exception as e:
            cls.print_error(f"Failed to scan world files: {e}")
            return 1
        
        if not zone_files_list:
            cls.print_warning("No world files found")
            return 0
        
        if args.zone:
            # Detailed info for specific zone
            zone_files = zone_files_list[0]
            zone_id = zone_files.id
            
            print(f"🏰 Zone {zone_id} Detailed Information")
            print("=" * 40)
            
            # Basic file info
            print(f"📁 Path: {zone_files.path}")
            print(f"📄 Source Files: {len(zone_files.files)}")
            
            for file in zone_files.files:
                file_path = Path(file.filename)
                size = file_path.stat().st_size if file_path.exists() else 0
                print(f"   • {file.get_mud_type()}: {file.filename} ({size:,} bytes)")
            
            # JSON file info
            json_file = Path(zone_files.path) / f"{zone_id}.json"
            if json_file.exists():
                print(f"\n📄 JSON File: {json_file}")
                print(f"   Size: {json_file.stat().st_size:,} bytes")
                
                try:
                    with open(json_file) as f:
                        zone_data = json.load(f)
                    
                    print(f"   Zone Name: {zone_data.get('name', 'Unknown')}")
                    print(f"   Description: {zone_data.get('description', 'None')}")
                    print(f"   Reset Time: {zone_data.get('reset_minutes', 'Unknown')} minutes")
                    print(f"   Level Range: {zone_data.get('min_level', '?')}-{zone_data.get('max_level', '?')}")
                    
                    if 'rooms' in zone_data:
                        print(f"   🏠 Rooms: {len(zone_data['rooms'])}")
                        room_ids = [r.get('id', 0) for r in zone_data['rooms']]
                        if room_ids:
                            print(f"      Range: {min(room_ids)}-{max(room_ids)}")
                    
                    if 'mobs' in zone_data:
                        print(f"   🧌 Mobs: {len(zone_data['mobs'])}")
                        
                    if 'objects' in zone_data:
                        print(f"   📦 Objects: {len(zone_data['objects'])}")
                    
                    if 'commands' in zone_data:
                        print(f"   ⚙️  Zone Commands: {len(zone_data['commands'])}")
                        
                except Exception as e:
                    cls.print_warning(f"   Could not parse JSON: {e}")
            else:
                print("\n📄 JSON File: Not found")
        
        else:
            # Summary for all zones
            total_zones = len(zone_files_list)
            total_rooms = 0
            total_mobs = 0
            total_objects = 0
            total_json_files = 0
            total_size = 0
            
            zone_stats = []
            
            for zone_files in zone_files_list:
                zone_id = zone_files.id
                zone_info = {
                    'id': zone_id,
                    'files': len(zone_files.files),
                    'rooms': 0,
                    'mobs': 0,
                    'objects': 0,
                    'has_json': False,
                    'size': 0
                }
                
                # Calculate file sizes
                for file in zone_files.files:
                    file_path = Path(file.filename)
                    if file_path.exists():
                        size = file_path.stat().st_size
                        zone_info['size'] += size
                        total_size += size
                
                # Check JSON file
                json_file = Path(zone_files.path) / f"{zone_id}.json"
                if json_file.exists():
                    zone_info['has_json'] = True
                    total_json_files += 1
                    
                    try:
                        with open(json_file) as f:
                            zone_data = json.load(f)
                        
                        if 'rooms' in zone_data:
                            zone_info['rooms'] = len(zone_data['rooms'])
                            total_rooms += zone_info['rooms']
                        
                        if 'mobs' in zone_data:
                            zone_info['mobs'] = len(zone_data['mobs'])
                            total_mobs += zone_info['mobs']
                        
                        if 'objects' in zone_data:
                            zone_info['objects'] = len(zone_data['objects'])
                            total_objects += zone_info['objects']
                            
                    except Exception:
                        pass
                
                zone_stats.append(zone_info)
            
            if args.format == 'json':
                print(json.dumps({
                    'summary': {
                        'total_zones': total_zones,
                        'total_rooms': total_rooms,
                        'total_mobs': total_mobs,
                        'total_objects': total_objects,
                        'total_json_files': total_json_files,
                        'total_size': total_size
                    },
                    'zones': zone_stats
                }, indent=2))
            
            elif args.format == 'table':
                print(f"{'Zone':<6} {'Files':<6} {'Rooms':<6} {'Mobs':<6} {'Objs':<6} {'JSON':<6} {'Size'}")
                print("-" * 60)
                for info in zone_stats:
                    json_status = "✅" if info['has_json'] else "⭕"
                    size_str = f"{info['size']:,}B" if info['size'] > 0 else "0B"
                    print(f"{info['id']:<6} {info['files']:<6} {info['rooms']:<6} {info['mobs']:<6} {info['objects']:<6} {json_status:<6} {size_str}")
            
            else:  # summary format
                print("🌍 World Summary")
                print("=" * 30)
                print(f"📊 Total Zones: {total_zones}")
                print(f"🏠 Total Rooms: {total_rooms}")
                print(f"🧌 Total Mobs: {total_mobs}")
                print(f"📦 Total Objects: {total_objects}")
                print(f"📄 JSON Files: {total_json_files}/{total_zones} ({100*total_json_files/total_zones:.1f}%)")
                print(f"💾 Total Size: {total_size:,} bytes")
                
                if total_zones > 0:
                    print(f"📈 Average per Zone:")
                    print(f"   Rooms: {total_rooms/total_zones:.1f}")
                    print(f"   Mobs: {total_mobs/total_zones:.1f}")
                    print(f"   Objects: {total_objects/total_zones:.1f}")
        
        return 0
    
    @classmethod
    def _show_players_info(cls, players_path: Path, args: argparse.Namespace) -> int:
        """Show player statistics and information."""
        
        try:
            player_files_list = list(MudFiles.player_files(str(players_path)))
        except Exception as e:
            cls.print_error(f"Failed to scan player files: {e}")
            return 1
        
        if not player_files_list:
            cls.print_warning("No player files found")
            return 0
        
        total_players = len(player_files_list)
        total_size = 0
        total_json_files = 0
        recent_activity = 0
        
        player_stats = []
        
        import time
        cutoff_time = time.time() - (args.active_days * 24 * 60 * 60)
        
        for player_files in player_files_list:
            player_name = player_files.id
            player_info = {
                'name': player_name,
                'files': len(player_files.files),
                'size': 0,
                'has_json': False,
                'last_modified': 0
            }
            
            # Calculate file info
            for file in player_files.files:
                file_path = Path(file.filename)
                if file_path.exists():
                    stat = file_path.stat()
                    size = stat.st_size
                    player_info['size'] += size
                    total_size += size
                    player_info['last_modified'] = max(player_info['last_modified'], stat.st_mtime)
            
            # Check JSON file
            json_file = Path(player_files.path) / f"{player_name}.json"
            if json_file.exists():
                player_info['has_json'] = True
                total_json_files += 1
            
            # Check recent activity
            if player_info['last_modified'] > cutoff_time:
                recent_activity += 1
            
            player_stats.append(player_info)
        
        if args.format == 'json':
            print(json.dumps({
                'summary': {
                    'total_players': total_players,
                    'recent_activity': recent_activity,
                    'total_json_files': total_json_files,
                    'total_size': total_size,
                    'active_days_threshold': args.active_days
                },
                'players': player_stats
            }, indent=2))
        
        elif args.format == 'table':
            print(f"{'Player':<20} {'Files':<6} {'Size':<10} {'JSON':<6} {'Last Modified'}")
            print("-" * 60)
            for info in player_stats:
                json_status = "✅" if info['has_json'] else "⭕"
                size_str = f"{info['size']:,}B" if info['size'] > 0 else "0B"
                
                if info['last_modified'] > 0:
                    import datetime
                    last_mod = datetime.datetime.fromtimestamp(info['last_modified']).strftime('%Y-%m-%d')
                else:
                    last_mod = "Unknown"
                
                print(f"{info['name']:<20} {info['files']:<6} {size_str:<10} {json_status:<6} {last_mod}")
        
        else:  # summary format
            print("👥 Player Summary")
            print("=" * 30)
            print(f"📊 Total Players: {total_players}")
            print(f"🔥 Recent Activity ({args.active_days} days): {recent_activity}")
            print(f"📄 JSON Files: {total_json_files}/{total_players} ({100*total_json_files/total_players:.1f}%)")
            print(f"💾 Total Size: {total_size:,} bytes")
            
            if total_players > 0:
                print(f"📈 Average per Player:")
                print(f"   File Size: {total_size/total_players:,.0f} bytes")
                print(f"   Activity Rate: {100*recent_activity/total_players:.1f}% recent")
        
        return 0
    
    @classmethod
    def _show_dependencies_info(cls) -> int:
        """Show Python dependencies and versions."""
        
        print("📦 Python Dependencies Information")
        print("=" * 50)
        
        # Core dependencies
        core_deps = [
            'sys', 'os', 'pathlib', 'argparse', 'json', 'typing'
        ]
        
        print("🔧 Core Dependencies:")
        for dep in core_deps:
            try:
                module = __import__(dep)
                version = getattr(module, '__version__', 'Built-in')
                print(f"   ✅ {dep}: {version}")
            except ImportError:
                print(f"   ❌ {dep}: Missing")
        
        # Project-specific dependencies
        project_deps = [
            'mud'
        ]
        
        print("\n🎯 Project Dependencies:")
        for dep in project_deps:
            try:
                module = __import__(dep)
                version = getattr(module, '__version__', 'Unknown version')
                print(f"   ✅ {dep}: {version}")
                
                # Show additional info for mud module
                if dep == 'mud' and hasattr(module, 'MudFiles'):
                    print(f"      MudFiles class: Available")
                    print(f"      Encoder class: {'Available' if hasattr(module, 'Encoder') else 'Missing'}")
                
            except ImportError:
                print(f"   ❌ {dep}: Missing (required for MUD file operations)")
        
        # Development/Optional dependencies
        optional_deps = [
            'jsonschema', 'tqdm', 'colorama', 'pytest', 'black', 'mypy'
        ]
        
        print("\n⭕ Optional Dependencies:")
        for dep in optional_deps:
            try:
                module = __import__(dep)
                version = getattr(module, '__version__', 'Unknown version')
                print(f"   ✅ {dep}: {version}")
            except ImportError:
                print(f"   ⭕ {dep}: Not installed")
        
        return 0
    
    @classmethod
    def _show_config_info(cls, args: argparse.Namespace) -> int:
        """Show FieryMUD configuration information."""
        
        print("⚙️  FieryMUD Configuration Information")
        print("=" * 50)
        
        # Path information
        print("📁 Paths:")
        print(f"   Library Path: {args.lib_path}")
        print(f"   World Path: {args.lib_path / 'world'}")
        print(f"   Players Path: {args.lib_path / 'players'}")
        
        # Check if paths exist
        lib_path = Path(args.lib_path)
        world_path = lib_path / 'world'
        players_path = lib_path / 'players'
        
        print("\n📊 Path Status:")
        print(f"   Library: {'✅ Exists' if lib_path.exists() else '❌ Missing'}")
        print(f"   World: {'✅ Exists' if world_path.exists() else '❌ Missing'}")
        print(f"   Players: {'✅ Exists' if players_path.exists() else '❌ Missing'}")
        
        if not args.paths_only:
            # Check for common config files
            config_files = [
                'etc/config',
                'etc/players',
                'etc/admins',
                'etc/wizlist',
                'etc/motd'
            ]
            
            print("\n📄 Config Files:")
            for config_file in config_files:
                file_path = lib_path / config_file
                if file_path.exists():
                    size = file_path.stat().st_size
                    print(f"   ✅ {config_file}: {size:,} bytes")
                else:
                    print(f"   ⭕ {config_file}: Not found")
            
            # Check for backup directories
            backup_paths = [
                'backup',
                'logs',
                'tmp'
            ]
            
            print("\n📂 Additional Directories:")
            for backup_path in backup_paths:
                dir_path = lib_path / backup_path
                if dir_path.exists() and dir_path.is_dir():
                    file_count = len(list(dir_path.iterdir()))
                    print(f"   ✅ {backup_path}: {file_count} files")
                else:
                    print(f"   ⭕ {backup_path}: Not found")
        
        return 0