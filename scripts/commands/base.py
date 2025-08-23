"""
Base command interface for FieryMUD tools.
"""

import argparse
from abc import ABC, abstractmethod
from pathlib import Path
from typing import Any


class BaseCommand(ABC):
    """Base class for all FieryMUD tool commands."""
    
    @classmethod
    @abstractmethod
    def add_parser(cls, subparsers: argparse._SubParsersAction) -> argparse.ArgumentParser:
        """Add this command's parser to the subparsers."""
        pass
    
    @classmethod
    @abstractmethod
    def execute(cls, args: argparse.Namespace) -> int:
        """Execute the command with parsed arguments. Return exit code."""
        pass
    
    @staticmethod
    def validate_world_path(lib_path: Path) -> Path:
        """Validate and return the world directory path."""
        world_path = lib_path / 'world'
        if not world_path.exists():
            raise FileNotFoundError(f"World directory not found: {world_path}")
        return world_path
    
    @staticmethod
    def validate_players_path(lib_path: Path) -> Path:
        """Validate and return the players directory path."""
        players_path = lib_path / 'players'
        if not players_path.exists():
            raise FileNotFoundError(f"Players directory not found: {players_path}")
        return players_path
    
    @staticmethod
    def print_success(message: str) -> None:
        """Print a success message with formatting."""
        print(f"✅ {message}")
    
    @staticmethod
    def print_warning(message: str) -> None:
        """Print a warning message with formatting."""
        print(f"⚠️  {message}")
    
    @staticmethod
    def print_error(message: str) -> None:
        """Print an error message with formatting."""
        print(f"❌ {message}")
    
    @staticmethod
    def print_info(message: str) -> None:
        """Print an info message with formatting."""
        print(f"ℹ️  {message}")
    
    @staticmethod
    def print_progress(current: int, total: int, item: str = "items") -> None:
        """Print progress information."""
        percentage = (current / total) * 100 if total > 0 else 0
        print(f"📊 Progress: {current}/{total} {item} ({percentage:.1f}%)")