"""
Player file management commands for FieryMUD tools.
"""

import argparse
import json
from pathlib import Path
from typing import List, Optional, Dict, Any

from .base import BaseCommand
from mud import MudFiles, Encoder


class PlayerCommand(BaseCommand):
    """Commands for managing player files and data."""
    
    @classmethod
    def add_parser(cls, subparsers: argparse._SubParsersAction) -> argparse.ArgumentParser:
        """Add player command parser."""
        
        parser = subparsers.add_parser(
            'player',
            help='Manage player files and data',
            description='Convert, validate, and manage FieryMUD player files'
        )
        
        # Subcommands for player operations
        player_subs = parser.add_subparsers(
            dest='player_action',
            title='Player Actions',
            description='Available player file operations'
        )
        
        # Convert command
        convert_parser = player_subs.add_parser(
            'convert',
            help='Convert player files to JSON format'
        )
        convert_parser.add_argument(
            '--player', '-p',
            type=str,
            help='Convert specific player (default: all players)'
        )
        convert_parser.add_argument(
            '--output', '-o',
            type=Path,
            help='Output directory or file (default: same as input)'
        )
        convert_parser.add_argument(
            '--stdout',
            action='store_true',
            help='Output to stdout instead of files'
        )
        convert_parser.add_argument(
            '--force', '-f',
            action='store_true',
            help='Overwrite existing JSON files'
        )
        
        # List command
        list_parser = player_subs.add_parser(
            'list',
            help='List available players and their status'
        )
        list_parser.add_argument(
            '--format',
            choices=['table', 'json', 'simple'],
            default='table',
            help='Output format'
        )
        list_parser.add_argument(
            '--active-only',
            action='store_true',
            help='Show only players with recent activity'
        )
        
        # Info command
        info_parser = player_subs.add_parser(
            'info',
            help='Show detailed information about a player'
        )
        info_parser.add_argument(
            'player',
            type=str,
            help='Player name to inspect'
        )
        
        # Backup command
        backup_parser = player_subs.add_parser(
            'backup',
            help='Create backup of player files'
        )
        backup_parser.add_argument(
            '--player', '-p',
            type=str,
            help='Backup specific player (default: all players)'
        )
        backup_parser.add_argument(
            '--output', '-o',
            type=Path,
            required=True,
            help='Backup directory'
        )
        backup_parser.add_argument(
            '--compress',
            action='store_true',
            help='Compress backup files'
        )
        
        return parser
    
    @classmethod
    def execute(cls, args: argparse.Namespace) -> int:
        """Execute player command."""
        
        if not args.player_action:
            cls.print_error("No player action specified. Use --help for available actions.")
            return 1
        
        try:
            players_path = cls.validate_players_path(args.lib_path)
            
            if args.player_action == 'convert':
                return cls._convert_player_files(players_path, args)
            elif args.player_action == 'list':
                return cls._list_players(players_path, args)
            elif args.player_action == 'info':
                return cls._show_player_info(players_path, args)
            elif args.player_action == 'backup':
                return cls._backup_players(players_path, args)
            else:
                cls.print_error(f"Unknown player action: {args.player_action}")
                return 1
                
        except Exception as e:
            cls.print_error(str(e))
            return 1
    
    @classmethod
    def _convert_player_files(cls, players_path: Path, args: argparse.Namespace) -> int:
        """Convert player files to JSON."""
        
        cls.print_info("Starting player file conversion...")
        
        # Determine output directory
        output_dir = args.output if args.output else players_path
        
        if args.output and not args.output.exists() and not args.stdout:
            args.output.mkdir(parents=True, exist_ok=True)
            cls.print_info(f"Created output directory: {args.output}")
        
        converted_count = 0
        error_count = 0
        
        # Get players to process
        try:
            player_files_list = list(MudFiles.player_files(str(players_path), args.player))
        except Exception as e:
            cls.print_error(f"Failed to scan player files: {e}")
            return 1
        
        if not player_files_list:
            cls.print_warning("No player files found to convert")
            return 0
        
        total_players = len(player_files_list)
        cls.print_info(f"Found {total_players} player(s) to process")
        
        for i, player_files in enumerate(player_files_list, 1):
            player_name = player_files.id
            
            try:
                cls.print_progress(i, total_players, "players")
                print(f"🔄 Processing Player {player_name}...")
                
                # Parse all files for this player
                player_data = {}
                for file in player_files.files:
                    if args.verbose:
                        print(f"   📄 Processing {file.get_mud_type()} file: {file.filename}")
                    player_data[file.mud_type.name] = file.parse_player()
                
                # Handle output
                if args.stdout:
                    print(json.dumps(player_data, cls=Encoder, indent=2))
                    converted_count += 1
                    continue
                
                # Determine output file
                if args.output and args.output.is_dir():
                    output_file = args.output / f"{player_name}.json"
                elif args.output and args.output.suffix == '.json':
                    output_file = args.output
                else:
                    output_file = Path(player_files.path) / f"{player_name}.json"
                
                # Check if file exists and handle accordingly
                if output_file.exists() and not args.force:
                    cls.print_warning(f"Skipping Player {player_name} - output file exists (use --force to overwrite)")
                    continue
                
                # Write JSON file
                with open(output_file, 'w', encoding='utf-8') as f:
                    json.dump(player_data, f, cls=Encoder, indent=2, ensure_ascii=False)
                    f.write('\n')  # Add final newline
                
                cls.print_success(f"Converted Player {player_name} → {output_file}")
                converted_count += 1
                
            except Exception as e:
                cls.print_error(f"Failed to convert Player {player_name}: {e}")
                if args.verbose:
                    import traceback
                    traceback.print_exc()
                error_count += 1
        
        # Summary
        print("\n" + "="*50)
        cls.print_info(f"Conversion Summary:")
        print(f"   ✅ Successfully converted: {converted_count}")
        if error_count > 0:
            print(f"   ❌ Failed conversions: {error_count}")
        
        return 0 if error_count == 0 else 1
    
    @classmethod
    def _list_players(cls, players_path: Path, args: argparse.Namespace) -> int:
        """List available players."""
        
        try:
            player_files_list = list(MudFiles.player_files(str(players_path)))
        except Exception as e:
            cls.print_error(f"Failed to scan players: {e}")
            return 1
        
        if not player_files_list:
            cls.print_warning("No players found")
            return 0
        
        players_info = []
        for player_files in player_files_list:
            player_name = player_files.id
            json_file = Path(player_files.path) / f"{player_name}.json"
            
            # Get basic file info
            file_count = len(player_files.files)
            total_size = sum(Path(f.filename).stat().st_size for f in player_files.files if Path(f.filename).exists())
            
            info = {
                'player_name': player_name,
                'files': file_count,
                'total_size': total_size,
                'has_json': json_file.exists(),
                'path': player_files.path
            }
            
            # Filter active players if requested
            if args.active_only and total_size == 0:
                continue
                
            players_info.append(info)
        
        # Output in requested format
        if args.format == 'json':
            print(json.dumps(players_info, indent=2))
        elif args.format == 'simple':
            for info in players_info:
                status = "✅" if info['has_json'] else "⭕"
                print(f"{status} {info['player_name']}")
        else:  # table format
            print(f"{'Player':<20} {'Files':<6} {'Size':<10} {'JSON':<6} {'Path'}")
            print("-" * 70)
            for info in players_info:
                status = "✅" if info['has_json'] else "⭕"
                size_str = f"{info['total_size']:,}B" if info['total_size'] > 0 else "0B"
                print(f"{info['player_name']:<20} {info['files']:<6} {size_str:<10} {status:<6} {info['path']}")
        
        return 0
    
    @classmethod
    def _show_player_info(cls, players_path: Path, args: argparse.Namespace) -> int:
        """Show detailed information about a specific player."""
        
        player_name = args.player
        
        try:
            # Find player files
            player_files_list = list(MudFiles.player_files(str(players_path), player_name))
            if not player_files_list:
                cls.print_error(f"Player {player_name} not found")
                return 1
            
            player_files = player_files_list[0]
            
            print(f"👤 Player {player_name} Information")
            print("=" * 40)
            print(f"📁 Path: {player_files.path}")
            print(f"📄 Files: {len(player_files.files)}")
            
            # List individual files
            total_size = 0
            for file in player_files.files:
                file_path = Path(file.filename)
                size = file_path.stat().st_size if file_path.exists() else 0
                total_size += size
                print(f"   • {file.get_mud_type()}: {file.filename} ({size:,} bytes)")
            
            print(f"📊 Total Size: {total_size:,} bytes")
            
            # Check for JSON file
            json_file = Path(player_files.path) / f"{player_name}.json"
            if json_file.exists():
                print(f"📄 JSON file: {json_file} ({json_file.stat().st_size:,} bytes)")
                
                # Try to load and show basic stats
                try:
                    with open(json_file) as f:
                        data = json.load(f)
                    
                    if isinstance(data, dict):
                        for section, content in data.items():
                            if isinstance(content, dict) and 'level' in content:
                                print(f"   📊 {section}: Level {content.get('level', 'Unknown')}")
                            elif isinstance(content, list):
                                print(f"   📊 {section}: {len(content)} items")
                            elif isinstance(content, dict):
                                print(f"   📊 {section}: {len(content)} items")
                            
                except Exception as e:
                    cls.print_warning(f"Could not parse JSON file: {e}")
            else:
                print("📄 JSON file: Not found")
            
        except Exception as e:
            cls.print_error(f"Failed to get player info: {e}")
            return 1
        
        return 0
    
    @classmethod
    def _backup_players(cls, players_path: Path, args: argparse.Namespace) -> int:
        """Create backup of player files."""
        
        backup_dir = args.output
        if not backup_dir.exists():
            backup_dir.mkdir(parents=True, exist_ok=True)
            cls.print_info(f"Created backup directory: {backup_dir}")
        
        try:
            player_files_list = list(MudFiles.player_files(str(players_path), args.player))
        except Exception as e:
            cls.print_error(f"Failed to scan player files: {e}")
            return 1
        
        if not player_files_list:
            cls.print_warning("No player files found to backup")
            return 0
        
        backed_up = 0
        error_count = 0
        
        cls.print_info(f"Starting backup of {len(player_files_list)} player(s)...")
        
        for i, player_files in enumerate(player_files_list, 1):
            player_name = player_files.id
            
            try:
                cls.print_progress(i, len(player_files_list), "players")
                print(f"📦 Backing up Player {player_name}...")
                
                player_backup_dir = backup_dir / player_name
                player_backup_dir.mkdir(exist_ok=True)
                
                # Copy all player files
                import shutil
                for file in player_files.files:
                    src_file = Path(file.filename)
                    if src_file.exists():
                        dst_file = player_backup_dir / src_file.name
                        shutil.copy2(src_file, dst_file)
                        if args.verbose:
                            print(f"   📄 Copied {src_file.name}")
                
                # Copy JSON file if it exists
                json_file = Path(player_files.path) / f"{player_name}.json"
                if json_file.exists():
                    dst_json = player_backup_dir / f"{player_name}.json"
                    shutil.copy2(json_file, dst_json)
                    if args.verbose:
                        print(f"   📄 Copied {player_name}.json")
                
                backed_up += 1
                
            except Exception as e:
                cls.print_error(f"Failed to backup Player {player_name}: {e}")
                error_count += 1
        
        # Compress if requested
        if args.compress and backed_up > 0:
            cls.print_info("Compressing backup...")
            try:
                import tarfile
                import datetime
                
                timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                archive_name = backup_dir.parent / f"player_backup_{timestamp}.tar.gz"
                
                with tarfile.open(archive_name, "w:gz") as tar:
                    tar.add(backup_dir, arcname=backup_dir.name)
                
                cls.print_success(f"Created compressed backup: {archive_name}")
                
                # Optionally remove uncompressed backup
                import shutil
                shutil.rmtree(backup_dir)
                
            except Exception as e:
                cls.print_warning(f"Failed to compress backup: {e}")
        
        # Summary
        print("\n" + "="*50)
        cls.print_info(f"Backup Summary:")
        print(f"   ✅ Successfully backed up: {backed_up}")
        if error_count > 0:
            print(f"   ❌ Failed backups: {error_count}")
        
        return 0 if error_count == 0 else 1