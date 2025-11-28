#!/usr/bin/env python3
"""
Extract comprehensive ability data from FieryMUD codebase.
Creates unified documentation combining spell and skill metadata with actual implementation details.
"""

import json
import csv
import re
from pathlib import Path
from typing import Dict, List, Any, Optional
from collections import defaultdict

# File paths
BASE_DIR = Path(__file__).parent
SPELL_METADATA = BASE_DIR / "spell_metadata.json"
SKILL_METADATA = BASE_DIR / "skill_metadata_comprehensive.json"
MAGIC_CPP = BASE_DIR / "legacy" / "src" / "magic.cpp"
OUTPUT_MD = BASE_DIR / "docs" / "ABILITIES.md"
OUTPUT_CSV = BASE_DIR / "docs" / "abilities.csv"

# Damage type mapping
DAMAGE_TYPE_MAP = {
    "DAM_FIRE": "FIRE",
    "DAM_COLD": "COLD",
    "DAM_ACID": "ACID",
    "DAM_SHOCK": "SHOCK",
    "DAM_POISON": "POISON",
    "DAM_SLASH": "PHYSICAL_SLASH",
    "DAM_PIERCE": "PHYSICAL_PIERCE",
    "DAM_CRUSH": "PHYSICAL_BLUNT",
    "DAM_MENTAL": "MENTAL",
    "DAM_ALIGN": "ALIGN",
    "DAM_UNDEFINED": "NONE",
    "0": "NONE"
}

# Target type modernization
TARGET_TYPE_MAP = {
    "TAR_CHAR_ROOM": "SINGLE_CHARACTER",
    "TAR_CHAR_WORLD": "SINGLE_CHARACTER",
    "TAR_FIGHT_VICT": "SINGLE_ENEMY",
    "TAR_FIGHT_SELF": "SINGLE_ALLY",
    "TAR_SELF_ONLY": "SELF",
    "TAR_IGNORE": "AREA_ROOM",
    "TAR_OBJ_INV": "OBJECT",
    "TAR_OBJ_ROOM": "OBJECT",
    "TAR_OBJ_WORLD": "OBJECT",
    "TAR_OBJ_EQUIP": "OBJECT"
}

# Position mapping
POSITION_MAP = {
    "POS_DEAD": "DEAD",
    "POS_MORTALLY_WOUNDED": "MORTALLY_WOUNDED",
    "POS_INCAPACITATED": "INCAPACITATED",
    "POS_STUNNED": "STUNNED",
    "POS_SLEEPING": "SLEEPING",
    "POS_RESTING": "RESTING",
    "POS_SITTING": "SITTING",
    "POS_FIGHTING": "FIGHTING",
    "POS_STANDING": "STANDING",
    "POS_FLYING": "FLYING",
    "POS_MOUNTED": "MOUNTED"
}

def load_json_file(filepath: Path) -> Dict:
    """Load and parse JSON file."""
    with open(filepath, 'r') as f:
        return json.load(f)

def extract_damage_formulas() -> Dict[str, str]:
    """
    Extract actual damage formulas from magic.cpp mag_damage() function.
    Returns dict mapping spell IDs to their damage formulas.
    Handles fall-through case statements.
    """
    formulas = {}

    if not MAGIC_CPP.exists():
        print(f"Warning: {MAGIC_CPP} not found")
        return formulas

    with open(MAGIC_CPP, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # Find the mag_damage switch statement
    switch_match = re.search(r'switch\s*\(spellnum\)\s*{(.*?)}\s*/\*.*?end mag_damage', content, re.DOTALL)
    if not switch_match:
        print("Warning: Could not find mag_damage switch statement")
        return formulas

    switch_content = switch_match.group(1)

    # Extract case blocks - handle fall-through cases
    # Match all consecutive case labels followed by the code block
    case_block_pattern = r'((?:case\s+(?:SPELL_\w+|SKILL_\w+):\s*)+)(.*?)(?=(?:case\s+(?:SPELL_\w+|SKILL_\w+):)|(?:default:)|(?:}\s*/\*))'

    for match in re.finditer(case_block_pattern, switch_content, re.DOTALL):
        case_labels = match.group(1)
        case_body = match.group(2)

        # Extract all spell names from fall-through cases
        spell_names = re.findall(r'case\s+(SPELL_\w+|SKILL_\w+):', case_labels)

        # Skip empty case bodies (fall-through to next case)
        if not case_body.strip() or case_body.strip().startswith('case '):
            continue

        # Determine the formula for this block
        formula = None

        # Pattern 1: dam += (pow(skill, 2) * X) / Y
        pow_match = re.search(r'dam\s*\+=\s*\(pow\(skill,\s*2\)\s*\*\s*(\d+)\)\s*/\s*(\d+)', case_body)
        if pow_match:
            x, y = pow_match.groups()
            formula = f"dam += (pow(skill, 2) * {x}) / {y}"

        # Pattern 2: dam = sorcerer_single_target(...)
        elif re.search(r'sorcerer_single_target', case_body):
            div_match = re.search(r'sorcerer_single_target.*?\s*/\s*(\d+)', case_body)
            if div_match:
                formula = f"sorcerer_single_target(ch, spellnum, skill) / {div_match.group(1)}"
            else:
                formula = "sorcerer_single_target(ch, spellnum, skill)"

        # Pattern 3: dam = skill + random_number(...)
        elif skill_rand_match := re.search(r'dam\s*=\s*skill\s*\+\s*random_number\(([^)]+)\)', case_body):
            formula = f"skill + random_number({skill_rand_match.group(1)})"

        # Pattern 4: dam = roll_dice(...)
        elif dice_match := re.search(r'dam\s*=\s*roll_dice\((\d+),\s*(\d+)\)', case_body):
            formula = f"roll_dice({dice_match.group(1)}, {dice_match.group(2)})"

        # Pattern 5: Simple arithmetic
        elif simple_match := re.search(r'dam\s*=\s*([^;]+);', case_body):
            if 'sorcerer' not in simple_match.group(1):
                formula_text = simple_match.group(1).strip()
                if len(formula_text) < 100:  # Avoid capturing too much
                    formula = formula_text

        # Apply formula to all fall-through spell names
        if formula:
            for spell_name in spell_names:
                formulas[spell_name] = formula

    return formulas

def categorize_ability(ability: Dict, ability_type: str) -> str:
    """Determine the category of an ability."""
    if ability_type == "spell":
        routines = ability.get("routines", "")
        damage = ability.get("damage", "0")

        if "MAG_DAMAGE" in routines or "MAG_AREA" in routines:
            return "DAMAGE"
        elif "MAG_AFFECT" in routines and "heal" in ability.get("name", "").lower():
            return "HEALING"
        elif "MAG_AFFECT" in routines:
            return "BUFF"
        elif "MAG_SUMMON" in routines:
            return "SUMMON"
        elif "TELEPORT" in ability.get("name", "").upper():
            return "TELEPORT"
        else:
            return "UTILITY"

    elif ability_type in ["skill", "song", "chant"]:
        categories = ability.get("categories", [])
        if "combat" in categories:
            if ability.get("violent", False):
                return "COMBAT"
            else:
                return "DEFENSIVE"
        elif "stealth" in categories:
            return "STEALTH"
        elif "movement" in categories:
            return "MOVEMENT"
        elif "weapon_proficiency" in categories:
            return "WEAPON_PROFICIENCY"
        else:
            return "UTILITY"

    return "UNKNOWN"

def parse_target_string(target_str) -> Dict[str, Any]:
    """Parse target string/list into modernized targeting system."""
    if not target_str:
        return {
            "targetType": "NONE",
            "targetScope": "SELF",
            "requiresLineOfSight": False,
            "canTargetSelf": False
        }

    # Parse flags - handle both string and list
    if isinstance(target_str, list):
        flags = target_str
    elif isinstance(target_str, str):
        flags = [f.strip() for f in target_str.split("|")]
    else:
        flags = []

    # Determine target type
    target_type = "NONE"
    if "TAR_SELF_ONLY" in flags:
        target_type = "SELF"
    elif "TAR_FIGHT_VICT" in flags:
        target_type = "SINGLE_ENEMY"
    elif "TAR_CHAR_ROOM" in flags or "TAR_CHAR_WORLD" in flags:
        target_type = "SINGLE_CHARACTER"
    elif "TAR_IGNORE" in flags:
        target_type = "AREA_ROOM"
    elif "TAR_OBJ" in target_str:
        target_type = "OBJECT"

    # Determine scope
    scope = "SAME_ROOM"
    if "TAR_CHAR_WORLD" in flags or "TAR_OBJ_WORLD" in flags:
        scope = "ANYWHERE"
    elif "TAR_CONTACT" in flags:
        scope = "TOUCH"

    # Line of sight
    los = "TAR_DIRECT" in flags

    # Can target self
    can_self = "TAR_SELF_ONLY" in flags or ("TAR_SELF_OK" in flags)

    return {
        "targetType": target_type,
        "targetScope": scope,
        "requiresLineOfSight": los,
        "canTargetSelf": can_self
    }

def unify_abilities() -> List[Dict[str, Any]]:
    """Combine spell and skill metadata with implementation details."""
    unified = []

    # Load metadata
    spell_data = load_json_file(SPELL_METADATA)
    skill_data = load_json_file(SKILL_METADATA)

    # Extract damage formulas
    damage_formulas = extract_damage_formulas()

    # Process spells - spell_data uses numeric string keys as IDs
    for spell_id_str, spell_info in spell_data.items():
        spell_enum = spell_info.get("enumName", f"SPELL_{spell_id_str}")

        ability = {
            "id": int(spell_id_str) if spell_id_str.isdigit() else None,
            "type": "SPELL",
            "enumName": spell_enum,
            "name": spell_info.get("name", ""),
            "displayName": spell_info.get("name", "").title(),
            "category": categorize_ability(spell_info, "spell"),

            # Usage requirements
            "minPosition": POSITION_MAP.get(spell_info.get("minPosition", "POS_STANDING"), "STANDING"),
            "canUseInCombat": spell_info.get("fightingOk", False),
            "violent": spell_info.get("violent", False),

            # Targeting
            **parse_target_string(spell_info.get("targets", [])),

            # Damage & Effects
            "damageType": DAMAGE_TYPE_MAP.get(str(spell_info.get("damageType", "DAM_UNDEFINED")), "NONE"),
            "damageFormula": damage_formulas.get(spell_enum, ""),
            "wearoff": spell_info.get("wearOffMessage"),

            # Class availability
            "classLevels": {},  # Need to extract from skills.cpp
            "sphere": spell_info.get("sphere"),

            # Restrictions
            "humanoidOnly": False,  # Not in spell metadata
            "questRequired": spell_info.get("questSpell", False),

            # Implementation
            "implementations": []  # Not in spell metadata
        }
        unified.append(ability)

    # Process skills/songs/chants
    for skill_enum, skill_info in skill_data.items():
        ability = {
            "id": skill_info.get("id"),
            "type": skill_info.get("type", "skill").upper(),
            "enumName": skill_enum,
            "name": skill_info.get("name", ""),
            "displayName": skill_info.get("name", "").title(),
            "category": categorize_ability(skill_info, skill_info.get("type", "skill")),

            # Usage requirements
            "minPosition": POSITION_MAP.get(skill_info.get("minpos", "POS_STANDING"), "STANDING"),
            "canUseInCombat": skill_info.get("fighting_ok", False),
            "violent": skill_info.get("violent", False),

            # Targeting
            **parse_target_string(skill_info.get("targets", "")),

            # Effects
            "damageType": DAMAGE_TYPE_MAP.get(str(skill_info.get("damage", "0")), "NONE"),
            "damageFormula": damage_formulas.get(skill_enum, ""),
            "wearoff": skill_info.get("wearoff"),

            # Class availability
            "classLevels": skill_info.get("class_levels", {}),

            # Restrictions
            "humanoidOnly": skill_info.get("humanoid", False),
            "questRequired": skill_info.get("quest", False),

            # Implementation
            "implementations": skill_info.get("implementations", [])
        }
        unified.append(ability)

    # Sort by ID (handle None values)
    unified.sort(key=lambda x: x.get("id") or 9999)

    return unified

def generate_markdown(abilities: List[Dict[str, Any]]) -> str:
    """Generate comprehensive markdown documentation."""
    lines = []

    # Header
    lines.append("# FieryMUD Ability System - Complete Reference")
    lines.append("")
    lines.append("**Source**: Extracted from actual C++ implementation code")
    lines.append("")

    # Statistics
    stats = defaultdict(int)
    categories = defaultdict(int)

    for ability in abilities:
        stats[ability["type"]] += 1
        categories[ability["category"]] += 1

    lines.append("## Overview")
    lines.append("")
    lines.append(f"- **Total abilities**: {len(abilities)}")
    for ability_type, count in sorted(stats.items()):
        lines.append(f"  - {ability_type.title()}s: {count}")
    lines.append("")

    lines.append("### Ability Categories")
    lines.append("")
    for category, count in sorted(categories.items(), key=lambda x: x[1], reverse=True):
        lines.append(f"- **{category}**: {count} abilities")
    lines.append("")

    # Group by category
    by_category = defaultdict(list)
    for ability in abilities:
        by_category[ability["category"]].append(ability)

    lines.append("---")
    lines.append("")
    lines.append("## Complete Ability Reference")
    lines.append("")

    for category in sorted(by_category.keys()):
        lines.append(f"### {category}")
        lines.append("")

        for ability in sorted(by_category[category], key=lambda x: x.get("name", "")):
            # Ability header
            lines.append(f"#### {ability['displayName']} (ID {ability['id']})")
            lines.append("")

            # Core info
            lines.append(f"- **Type**: {ability['type']}")
            lines.append(f"- **Enum**: `{ability['enumName']}`")
            lines.append(f"- **Command**: `{ability['name']}`")

            # Targeting
            lines.append(f"- **Target**: {ability['targetType']}")
            if ability['targetScope'] != "SELF":
                lines.append(f"  - Scope: {ability['targetScope']}")
            if ability['requiresLineOfSight']:
                lines.append(f"  - Requires line of sight")
            if ability['canTargetSelf']:
                lines.append(f"  - Can target self")

            # Damage/Effects
            if ability['damageType'] != "NONE":
                lines.append(f"- **Damage Type**: {ability['damageType']}")

            if ability['damageFormula']:
                lines.append(f"- **Damage Formula**: `{ability['damageFormula']}`")

            if ability['wearoff']:
                lines.append(f"- **Wearoff Message**: \"{ability['wearoff']}\"")

            # Usage
            lines.append(f"- **Min Position**: {ability['minPosition']}")
            lines.append(f"- **Combat Use**: {'Yes' if ability['canUseInCombat'] else 'No'}")
            lines.append(f"- **Violent**: {'Yes' if ability['violent'] else 'No'}")

            # Restrictions
            restrictions = []
            if ability['humanoidOnly']:
                restrictions.append("Humanoid only")
            if ability['questRequired']:
                restrictions.append("Quest required")
            if restrictions:
                lines.append(f"- **Restrictions**: {', '.join(restrictions)}")

            # Classes
            if ability['classLevels']:
                lines.append(f"- **Classes**:")
                for cls, level in sorted(ability['classLevels'].items()):
                    lines.append(f"  - {cls}: level {level}")

            # Implementation
            if ability['implementations']:
                impl = ability['implementations'][0]
                lines.append(f"- **Implementation**: `{impl.get('file', 'unknown')}::{impl.get('function', 'unknown')}`")

            lines.append("")

        lines.append("")

    return "\n".join(lines)

def generate_csv(abilities: List[Dict[str, Any]]) -> List[List[str]]:
    """Generate CSV data."""
    rows = []

    # Header
    rows.append([
        "id", "type", "name", "displayName", "category",
        "targetType", "targetScope", "damageType", "damageFormula",
        "minPosition", "canUseInCombat", "violent",
        "classes", "implementation"
    ])

    for ability in abilities:
        # Format classes
        classes_str = "; ".join([
            f"{cls}:{level}"
            for cls, level in sorted(ability['classLevels'].items())
        ])

        # Format implementation
        impl_str = ""
        if ability['implementations']:
            impl = ability['implementations'][0]
            impl_str = f"{impl.get('file', '')}::{impl.get('function', '')}"

        rows.append([
            str(ability['id']),
            ability['type'],
            ability['name'],
            ability['displayName'],
            ability['category'],
            ability['targetType'],
            ability['targetScope'],
            ability['damageType'],
            ability['damageFormula'],
            ability['minPosition'],
            "true" if ability['canUseInCombat'] else "false",
            "true" if ability['violent'] else "false",
            classes_str,
            impl_str
        ])

    return rows

def main():
    """Main execution."""
    print("Extracting ability data...")

    # Create docs directory if needed
    OUTPUT_MD.parent.mkdir(exist_ok=True)

    # Unify all ability data
    abilities = unify_abilities()
    print(f"Processed {len(abilities)} abilities")

    # Generate markdown
    print("Generating ABILITIES.md...")
    markdown = generate_markdown(abilities)
    with open(OUTPUT_MD, 'w') as f:
        f.write(markdown)
    print(f"Created {OUTPUT_MD}")

    # Generate CSV
    print("Generating abilities.csv...")
    csv_data = generate_csv(abilities)
    with open(OUTPUT_CSV, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerows(csv_data)
    print(f"Created {OUTPUT_CSV}")

    print("\nDone!")
    print(f"Total abilities documented: {len(abilities)}")

if __name__ == "__main__":
    main()
