# FieryMUD Development Tools

A unified command-line interface for managing FieryMUD world files, players, and development tasks.

## Overview

The FieryMUD development tools provide a modern, extensible CLI system for managing your MUD server. The tools are organized around command categories and designed for ease of use and maintenance.

## Installation

The tools use Python Poetry for dependency management:

```bash
cd scripts
poetry install
```

## Usage

The main entry point is `fierymud_tools.py`:

```bash
./fierymud_tools.py --help
```

All commands support the `--help` flag for detailed usage information.

## Command Categories

### World Management (`world`)

Manage world files, zones, rooms, objects, and mobiles.

**Basic Operations:**
```bash
# List all zones
./fierymud_tools.py world list-zones

# Show detailed zone information
./fierymud_tools.py world zone-info 1

# List rooms in a zone
./fierymud_tools.py world list-rooms --zone 1

# Show room details
./fierymud_tools.py world room-info 100

# List objects and mobiles
./fierymud_tools.py world list-objects --zone 1
./fierymud_tools.py world list-mobs --zone 1
```

**Legacy File Conversion:**
```bash
# Convert legacy CircleMUD-style files to modern JSON
./fierymud_tools.py world convert-legacy

# Convert specific zones only
./fierymud_tools.py world convert-legacy --zones 1,2,3

# Dry run to see what would be converted
./fierymud_tools.py world convert-legacy --dry-run
```

### Player Management (`player`)

Work with player files and character data.

```bash
# List all players
./fierymud_tools.py player list

# Show player information
./fierymud_tools.py player info PlayerName

# Convert player file formats
./fierymud_tools.py player convert PlayerName

# Backup player files
./fierymud_tools.py player backup --all
```

### Validation (`validate`)

Comprehensive validation and integrity checking.

```bash
# Validate all world files
./fierymud_tools.py validate world

# Validate player files
./fierymud_tools.py validate players

# Check JSON syntax
./fierymud_tools.py validate json

# Validate cross-references (rooms, objects, mobs)
./fierymud_tools.py validate references
```

### System Information (`info`)

Get information about your FieryMUD installation and data.

```bash
# System overview
./fierymud_tools.py info system

# World statistics
./fierymud_tools.py info world-stats

# Player statistics  
./fierymud_tools.py info player-stats

# Check dependencies
./fierymud_tools.py info dependencies
```

## File Structure

```
scripts/
├── fierymud_tools.py          # Main CLI entry point
├── commands/                  # Command modules
│   ├── base.py               # Base command class
│   ├── world.py              # World management
│   ├── player.py             # Player management
│   ├── validate.py           # Validation commands
│   └── info.py               # Information commands
├── pyproject.toml            # Poetry dependencies
└── README.md                 # This file
```

## Modern JSON Format

The tools work with FieryMUD's modern JSON world format:

**Zone Structure:**
```json
{
  "id": 1,
  "name": "Test Zone",
  "description": "A test zone for development",
  "reset_minutes": 30,
  "reset_mode": "Empty",
  "min_level": 1,
  "max_level": 10,
  "first_room": 100,
  "last_room": 199,
  "rooms": [...],
  "objects": [...],
  "mobs": [...],
  "commands": [...]
}
```

**Room Structure:**
```json
{
  "id": 100,
  "name": "Starting Room",
  "description": "A safe starting area",
  "exits": {
    "north": {
      "to_room": 101,
      "description": "A corridor leads north"
    }
  },
  "flags": ["peaceful"],
  "sector": "indoors"
}
```

## Legacy File Support

The tools can convert legacy FieryMUD files to the modern JSON format:

- **Zones** (.zon) → Combined JSON with all zone data
- **Rooms** (.wld) → Embedded in zone JSON
- **Objects** (.obj) → Embedded in zone JSON  
- **Mobiles** (.mob) → Embedded in zone JSON
- **Triggers** (.trg) → Preserved as scripts
- **Shops** (.shp) → Converted to modern format

## Error Handling

All commands include comprehensive error handling:

- **File Access Errors**: Clear messages for missing or unreadable files
- **JSON Validation**: Detailed syntax error reporting
- **Cross-Reference Validation**: Identifies broken links between game objects
- **Recovery Suggestions**: Actionable steps to resolve issues

## Extending the Tools

To add new commands:

1. Create a new module in `commands/`
2. Inherit from `BaseCommand` class
3. Implement required methods
4. Register in `fierymud_tools.py`

Example:
```python
from .base import BaseCommand

class MyCommand(BaseCommand):
    def add_arguments(self, parser):
        parser.add_argument('--option', help='Example option')
    
    def execute(self, args):
        self.info(f"Executing with option: {args.option}")
        return True
```

## Development Guidelines

- **Modern Python**: Use type hints, dataclasses, and modern idioms
- **Error Handling**: Always provide helpful error messages
- **Logging**: Use the built-in logging methods (info, warning, error)
- **Testing**: Add tests for new functionality
- **Documentation**: Update this README for new commands

## Migration from Old Scripts

If you have existing scripts, the new system provides equivalent functionality:

| Old Script | New Command |
|------------|-------------|
| `zone_loader.py` | `./fierymud_tools.py world list-zones` |
| `player_utils.py` | `./fierymud_tools.py player list` |
| `validate_world.py` | `./fierymud_tools.py validate world` |

The new system is more robust, provides better error handling, and has a consistent interface across all operations.
