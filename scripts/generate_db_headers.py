#!/usr/bin/env python3
"""
Generate C++ headers from Prisma schema.

Usage:
    python scripts/generate_db_headers.py

Reads:  ../muditor/packages/db/prisma/schema.prisma
Writes: src/database/generated/db_enums.hpp
        src/database/generated/db_tables.hpp
"""

import re
from pathlib import Path
from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional


@dataclass
class PrismaEnum:
    """Represents a Prisma enum definition."""
    name: str
    values: list[str] = field(default_factory=list)
    comment: str = ""


@dataclass
class PrismaField:
    """Represents a field in a Prisma model."""
    name: str              # camelCase field name from Prisma
    prisma_type: str       # Prisma type (String, Int, etc.)
    db_column: str         # Actual database column name
    is_array: bool = False
    is_nullable: bool = False
    is_relation: bool = False


@dataclass
class PrismaModel:
    """Represents a Prisma model definition."""
    name: str              # Model name (Room, Mobs, etc.)
    table_name: str        # @@map value or model name
    fields: list[PrismaField] = field(default_factory=list)
    primary_key: list[str] = field(default_factory=list)


def screaming_to_pascal(name: str) -> str:
    """Convert SCREAMING_SNAKE_CASE to PascalCase.

    Examples:
        NO_MOB -> NoMob
        DETECT_ALIGN -> DetectAlign
        ANTI_BERSERKER -> AntiBerserker
        DRAGONBORN_FIRE -> DragonbornFire
    """
    parts = name.split('_')
    return ''.join(part.capitalize() for part in parts)


def camel_to_snake(name: str) -> str:
    """Convert camelCase to snake_case.

    Examples:
        roomDescription -> room_description
        zoneId -> zone_id
        hitPointsMax -> hit_points_max
    """
    result = []
    for i, char in enumerate(name):
        if char.isupper() and i > 0:
            result.append('_')
        result.append(char.lower())
    return ''.join(result)


def camel_to_screaming(name: str) -> str:
    """Convert camelCase to SCREAMING_SNAKE_CASE.

    Examples:
        roomDescription -> ROOM_DESCRIPTION
        zoneId -> ZONE_ID
    """
    return camel_to_snake(name).upper()


class PrismaParser:
    """Parse Prisma schema file."""

    # Regex patterns
    ENUM_PATTERN = re.compile(r'enum\s+(\w+)\s*\{([^}]+)\}', re.DOTALL)
    # Model pattern just finds start - body extracted with balanced braces
    MODEL_START_PATTERN = re.compile(r'model\s+(\w+)\s*\{')
    FIELD_PATTERN = re.compile(
        r'^\s*(\w+)\s+(\w+)(\[\])?(\?)?\s*(.*)$'
    )
    MAP_PATTERN = re.compile(r'@map\("([^"]+)"\)')
    TABLE_MAP_PATTERN = re.compile(r'@@map\("([^"]+)"\)')
    COMPOSITE_KEY_PATTERN = re.compile(r'@@id\(\[([^\]]+)\]\)')

    def __init__(self, schema_path: Path):
        self.schema_path = schema_path
        self.content = schema_path.read_text()
        # Known model names for detecting relations
        self._model_names: set[str] = set()
        self._extract_model_names()

    def _extract_balanced_braces(self, content: str, start_pos: int) -> str:
        """Extract content between balanced braces, handling nested braces in strings."""
        depth = 1
        pos = start_pos
        in_string = False
        escape_next = False

        while pos < len(content) and depth > 0:
            char = content[pos]

            if escape_next:
                escape_next = False
            elif char == '\\':
                escape_next = True
            elif char == '"' and not in_string:
                in_string = True
            elif char == '"' and in_string:
                in_string = False
            elif not in_string:
                if char == '{':
                    depth += 1
                elif char == '}':
                    depth -= 1

            pos += 1

        return content[start_pos:pos - 1]  # Exclude the final }

    def _extract_model_names(self) -> None:
        """Extract all model names for relation detection."""
        for match in self.MODEL_START_PATTERN.finditer(self.content):
            self._model_names.add(match.group(1))

    def parse_enums(self) -> list[PrismaEnum]:
        """Extract all enum definitions."""
        enums = []

        for match in self.ENUM_PATTERN.finditer(self.content):
            enum_name = match.group(1)
            enum_body = match.group(2)

            # Extract enum values (skip comments and empty lines)
            values = []
            for line in enum_body.strip().split('\n'):
                line = line.strip()
                # Skip comments and empty lines
                if not line or line.startswith('//'):
                    continue
                # Extract just the enum value name
                value = line.split()[0] if line else None
                if value and value.isidentifier():
                    values.append(value)

            if values:
                enums.append(PrismaEnum(name=enum_name, values=values))

        return enums

    def parse_models(self) -> list[PrismaModel]:
        """Extract all model definitions."""
        models = []

        for match in self.MODEL_START_PATTERN.finditer(self.content):
            model_name = match.group(1)
            # Extract body using balanced braces (handles {} inside strings)
            body_start = match.end()
            model_body = self._extract_balanced_braces(self.content, body_start)

            # Check for @@map("TableName")
            table_map = self.TABLE_MAP_PATTERN.search(model_body)
            table_name = table_map.group(1) if table_map else model_name

            # Check for @@id([field1, field2])
            pk_match = self.COMPOSITE_KEY_PATTERN.search(model_body)
            primary_key = []
            if pk_match:
                primary_key = [f.strip() for f in pk_match.group(1).split(',')]

            # Parse fields
            fields = []
            for line in model_body.strip().split('\n'):
                field = self._parse_field(line)
                if field:
                    fields.append(field)

            models.append(PrismaModel(
                name=model_name,
                table_name=table_name,
                fields=fields,
                primary_key=primary_key
            ))

        return models

    def _parse_field(self, line: str) -> Optional[PrismaField]:
        """Parse a single field line."""
        line = line.strip()

        # Skip empty lines, comments, and directives
        if not line or line.startswith('//') or line.startswith('@@'):
            return None

        # Match field pattern
        match = self.FIELD_PATTERN.match(line)
        if not match:
            return None

        field_name = match.group(1)
        prisma_type = match.group(2)
        is_array = match.group(3) is not None
        is_nullable = match.group(4) is not None
        rest = match.group(5) or ""

        # Skip relation fields (type matches a model name)
        if prisma_type in self._model_names:
            return None

        # Get database column name from @map or convert camelCase
        map_match = self.MAP_PATTERN.search(rest)
        if map_match:
            db_column = map_match.group(1)
        else:
            # Prisma uses field name as-is if no @map
            db_column = field_name

        return PrismaField(
            name=field_name,
            prisma_type=prisma_type,
            db_column=db_column,
            is_array=is_array,
            is_nullable=is_nullable,
            is_relation=False
        )


class CppGenerator:
    """Generate C++ header files from parsed Prisma schema."""

    # Enum groupings for organization
    ENUM_GROUPS = {
        'Room System': ['RoomFlag', 'Sector', 'Direction', 'ExitFlag', 'ExitState',
                       'MagicAffinity'],
        'Mob System': ['MobFlag', 'MobRole', 'MobTrait', 'MobBehavior',
                       'Position', 'Stance', 'Gender', 'Size', 'LifeForce',
                       'Composition', 'DamageType'],
        'Object System': ['ObjectType', 'ObjectFlag', 'WearFlag', 'Alignment'],
        'Effect System': ['EffectFlag', 'ElementType', 'ApplyType'],
        'Character System': ['Race', 'RaceAlign', 'SkillCategory', 'SkillType'],
        'Ability System': ['SpellSphere', 'TargetType', 'TargetScope', 'SaveType',
                          'SaveResult', 'StackingRule'],
        'Shop System': ['ShopFlag', 'ShopTradesWith'],
        'Zone System': ['Climate', 'Hemisphere', 'ResetMode'],
        'Script System': ['ScriptType', 'TriggerFlag'],
        'User System': ['UserRole', 'GrantResourceType', 'GrantPermission'],
        'Configuration': ['ConfigValueType', 'SystemTextCategory', 'LoginStage',
                         'CommandCategory'],
    }

    def __init__(self, enums: list[PrismaEnum], models: list[PrismaModel]):
        self.enums = enums
        self.models = models
        self.timestamp = datetime.now().isoformat()

    def generate_enums_header(self) -> str:
        """Generate db_enums.hpp content."""
        lines = [
            '#pragma once',
            '/**',
            ' * @file db_enums.hpp',
            ' * @brief Database enum definitions auto-generated from Prisma schema.',
            ' *',
            ' * DO NOT EDIT MANUALLY - Generated by scripts/generate_db_headers.py',
            ' * Source: muditor/packages/db/prisma/schema.prisma',
            f' * Generated: {self.timestamp}',
            ' */',
            '',
            '#include <string_view>',
            '#include <optional>',
            '#include <unordered_map>',
            '',
            'namespace db {',
            '',
        ]

        # Build enum lookup for grouping
        enum_by_name = {e.name: e for e in self.enums}
        used_enums = set()

        # Generate enums by group
        for group_name, enum_names in self.ENUM_GROUPS.items():
            group_enums = []
            for name in enum_names:
                if name in enum_by_name:
                    group_enums.append(enum_by_name[name])
                    used_enums.add(name)

            if group_enums:
                lines.append(f'// {"=" * 76}')
                lines.append(f'// {group_name}')
                lines.append(f'// {"=" * 76}')
                lines.append('')

                for enum in group_enums:
                    lines.extend(self._generate_enum(enum))
                    lines.append('')

        # Generate remaining enums not in any group
        remaining = [e for e in self.enums if e.name not in used_enums]
        if remaining:
            lines.append(f'// {"=" * 76}')
            lines.append('// Other Enums')
            lines.append(f'// {"=" * 76}')
            lines.append('')

            for enum in remaining:
                lines.extend(self._generate_enum(enum))
                lines.append('')

        lines.append('} // namespace db')
        lines.append('')

        return '\n'.join(lines)

    def _generate_enum(self, enum: PrismaEnum) -> list[str]:
        """Generate a single enum class definition with converters."""
        lines = []

        # Enum definition
        lines.append(f'enum class {enum.name} {{')
        for i, value in enumerate(enum.values):
            pascal_value = screaming_to_pascal(value)
            lines.append(f'    {pascal_value} = {i},')
        lines.append('};')
        lines.append('')

        # from_db converter
        func_name = camel_to_snake(enum.name)
        lines.append(f'/** Convert database string to {enum.name} enum */')
        lines.append(f'inline std::optional<{enum.name}> {func_name}_from_db(std::string_view s) {{')
        lines.append(f'    static const std::unordered_map<std::string_view, {enum.name}> lookup = {{')
        for value in enum.values:
            pascal_value = screaming_to_pascal(value)
            lines.append(f'        {{"{value}", {enum.name}::{pascal_value}}},')
        lines.append('    };')
        lines.append('    auto it = lookup.find(s);')
        lines.append('    return it != lookup.end() ? std::optional{it->second} : std::nullopt;')
        lines.append('}')
        lines.append('')

        # to_db converter
        lines.append(f'/** Convert {enum.name} enum to database string */')
        lines.append(f'inline std::string_view {func_name}_to_db({enum.name} e) {{')
        lines.append('    switch (e) {')
        for value in enum.values:
            pascal_value = screaming_to_pascal(value)
            lines.append(f'        case {enum.name}::{pascal_value}: return "{value}";')
        lines.append('    }')
        lines.append('    return "";')
        lines.append('}')

        return lines

    def generate_tables_header(self) -> str:
        """Generate db_tables.hpp content."""
        lines = [
            '#pragma once',
            '/**',
            ' * @file db_tables.hpp',
            ' * @brief Database table and column constants auto-generated from Prisma schema.',
            ' *',
            ' * DO NOT EDIT MANUALLY - Generated by scripts/generate_db_headers.py',
            ' * Source: muditor/packages/db/prisma/schema.prisma',
            f' * Generated: {self.timestamp}',
            ' */',
            '',
            '#include <string_view>',
            '',
            'namespace db {',
            '',
        ]

        for model in self.models:
            lines.extend(self._generate_table_namespace(model))
            lines.append('')

        lines.append('} // namespace db')
        lines.append('')

        return '\n'.join(lines)

    def _generate_table_namespace(self, model: PrismaModel) -> list[str]:
        """Generate namespace with column constants for a model."""
        lines = []

        lines.append(f'namespace {model.name} {{')
        lines.append(f'    constexpr std::string_view TABLE = "{model.table_name}";')
        lines.append('')

        # Add primary key comment if composite
        if len(model.primary_key) > 1:
            pk_str = ', '.join(model.primary_key)
            lines.append(f'    // Composite primary key: ({pk_str})')

        # Generate column constants
        for field in model.fields:
            const_name = camel_to_screaming(field.name)
            lines.append(f'    constexpr std::string_view {const_name} = "{field.db_column}";')

        lines.append('}')

        return lines


def main():
    # Path configuration
    project_root = Path(__file__).parent.parent

    # Schema is in muditor (the persistent source of truth)
    schema_path = project_root.parent / "muditor" / "packages" / "db" / "prisma" / "schema.prisma"

    # Output to fierymud's database generated directory
    output_dir = project_root / "src" / "database" / "generated"

    if not schema_path.exists():
        print(f"Error: Schema not found at {schema_path}")
        print("Make sure muditor is at the expected location: ../muditor/")
        return 1

    print(f"Reading schema from: {schema_path}")

    # Parse schema
    parser = PrismaParser(schema_path)
    enums = parser.parse_enums()
    models = parser.parse_models()

    print(f"Found {len(enums)} enums and {len(models)} models")

    # Generate headers
    generator = CppGenerator(enums, models)

    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)

    # Write enum header
    enums_path = output_dir / "db_enums.hpp"
    enums_content = generator.generate_enums_header()
    enums_path.write_text(enums_content)
    print(f"Generated: {enums_path}")

    # Write tables header
    tables_path = output_dir / "db_tables.hpp"
    tables_content = generator.generate_tables_header()
    tables_path.write_text(tables_content)
    print(f"Generated: {tables_path}")

    # Write README
    readme_path = output_dir / "README.md"
    readme_content = f"""# Generated Database Headers

These files are auto-generated from the Prisma schema. Do not edit manually.

## Regeneration

To regenerate these files after schema changes:

```bash
cd /home/strider/Code/mud/fierymud
python scripts/generate_db_headers.py
```

## Source

Schema: `../muditor/packages/db/prisma/schema.prisma`

## Files

- `db_enums.hpp` - All enum definitions with from_db/to_db converters
- `db_tables.hpp` - Table and column name constants

## Usage

```cpp
#include "database/generated/db_enums.hpp"
#include "database/generated/db_tables.hpp"

// Use column constants
auto name = row[db::Room::NAME].as<std::string>();

// Use enum converters
auto flag = db::room_flag_from_db("NO_MOB");
if (flag) {{
    room->set_flag(*flag);
}}
```

Last generated: {datetime.now().isoformat()}
"""
    readme_path.write_text(readme_content)
    print(f"Generated: {readme_path}")

    print("\nDone!")
    return 0


if __name__ == "__main__":
    exit(main())
