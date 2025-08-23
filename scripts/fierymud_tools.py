#!/usr/bin/env python3
"""
FieryMUD Development Tools
A unified command-line interface for managing FieryMUD world files, players, and development tasks.
"""

import argparse
import sys
from pathlib import Path
from typing import Optional

from commands import WorldCommand, PlayerCommand, ValidateCommand, InfoCommand


def create_parser() -> argparse.ArgumentParser:
    """Create the main argument parser with subcommands."""
    
    parser = argparse.ArgumentParser(
        prog='fierymud-tools',
        description='FieryMUD Development Tools - Unified CLI for world file management',
        epilog='Use "fierymud-tools <command> --help" for command-specific help'
    )
    
    parser.add_argument(
        '--version', 
        action='version', 
        version='FieryMUD Tools 2.0.0'
    )
    
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Enable verbose output'
    )
    
    parser.add_argument(
        '--lib-path',
        type=Path,
        default=Path('../lib'),
        help='Path to lib directory (default: ../lib)'
    )
    
    # Create subparsers for different commands
    subparsers = parser.add_subparsers(
        dest='command',
        title='Commands',
        description='Available FieryMUD development commands',
        help='Use --help with any command for detailed information'
    )
    
    # World file management
    WorldCommand.add_parser(subparsers)
    
    # Player file management
    PlayerCommand.add_parser(subparsers)
    
    # Validation tools
    ValidateCommand.add_parser(subparsers)
    
    # Information and diagnostics
    InfoCommand.add_parser(subparsers)
    
    return parser


def main() -> int:
    """Main entry point for the FieryMUD tools CLI."""
    
    parser = create_parser()
    args = parser.parse_args()
    
    # Show help if no command provided
    if not args.command:
        parser.print_help()
        return 1
    
    # Validate lib path exists
    if not args.lib_path.exists():
        print(f"Error: Library path '{args.lib_path}' does not exist", file=sys.stderr)
        return 1
    
    try:
        # Dispatch to appropriate command handler
        if args.command == 'world':
            return WorldCommand.execute(args)
        elif args.command == 'player':
            return PlayerCommand.execute(args)
        elif args.command == 'validate':
            return ValidateCommand.execute(args)
        elif args.command == 'info':
            return InfoCommand.execute(args)
        else:
            print(f"Error: Unknown command '{args.command}'", file=sys.stderr)
            return 1
            
    except KeyboardInterrupt:
        print("\nOperation cancelled by user", file=sys.stderr)
        return 130
    except Exception as e:
        if args.verbose:
            import traceback
            traceback.print_exc()
        else:
            print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())