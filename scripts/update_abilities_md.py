#!/usr/bin/env python3
"""
Update ABILITIES.md with implementation details from ability_implementation_mechanics.json
"""

import json
import re
from pathlib import Path

# Mapping of spell names to their implementation data keys
SPELL_MAPPINGS = {
    "Armor": "SPELL_ARMOR",
    "Barkskin": "SPELL_BARKSKIN",
    "Bless": "SPELL_BLESS",
    "Blindness": "SPELL_BLINDNESS",
    "Blur": "SPELL_BLUR",
    "Bone Armor": "SPELL_BONE_ARMOR",
    "Coldshield": "SPELL_COLDSHIELD",
    "Confusion": "SPELL_CONFUSION",
    "Curse": "SPELL_CURSE",
    "Dark Presence": "SPELL_DARK_PRESENCE",
    "Demonic Aspect": "SPELL_DEMONIC_ASPECT",
    "Detect Alignment": "SPELL_DETECT_ALIGN",
    "Detect Invisibility": "SPELL_DETECT_INVIS",
    "Haste": "SPELL_HASTE",
    "Sanctuary": "SPELL_SANCTUARY",
    "Stone Skin": "SPELL_STONE_SKIN",
}

# Effect flag mappings
EFFECT_MAPPINGS = {
    "Bless": "EFF_BLESS",
    "Blindness": "EFF_BLIND",
    "Blur": "EFF_BLUR",
    "Confusion": "EFF_CONFUSION",
    "Curse": "EFF_CURSE",
    "Detect Alignment": "EFF_DETECT_ALIGN",
    "Detect Invisibility": "EFF_DETECT_INVIS",
    "Haste": "EFF_HASTE",
    "Sanctuary": "EFF_SANCTUARY",
    "Stone Skin": "EFF_STONE_SKIN",
}

def load_json_data():
    """Load the implementation mechanics JSON file"""
    json_path = Path("/home/strider/Code/mud/docs/ability_implementation_mechanics.json")
    with open(json_path, 'r') as f:
        return json.load(f)

def format_implementation_section(spell_name, json_data):
    """Generate the Implementation section for a spell"""

    # Get spell key and effect key
    spell_key = SPELL_MAPPINGS.get(spell_name)
    effect_key = EFFECT_MAPPINGS.get(spell_name)

    if not spell_key and not effect_key:
        return None

    lines = []
    lines.append("")
    lines.append("**Implementation**:")

    # Add duration if available
    if spell_key and spell_key in json_data.get("spell_durations", {}):
        duration_data = json_data["spell_durations"][spell_key]
        formula = duration_data.get("formula", "unknown")
        range_str = duration_data.get("range", "unknown")
        source = duration_data.get("source", "")
        lines.append(f"- **Duration**: `{formula}` ({range_str})")

    # Check for AC modifiers in combat_mechanics section
    has_effects = False
    combat_mechanics = json_data.get("combat_mechanics", {})
    ac_modifiers = combat_mechanics.get("ac_modifiers", {})

    # Check if this spell has AC modifier data
    spell_ac_key = None
    if spell_name == "Armor":
        spell_ac_key = "ARMOR"
    elif spell_name == "Barkskin":
        spell_ac_key = "BARKSKIN"
    elif spell_name == "Blindness":
        spell_ac_key = "BLIND"

    # Add effect flag data if available
    if effect_key and effect_key in json_data.get("effect_flags", {}):
        effect_data = json_data["effect_flags"][effect_key]

        # Stat modifiers
        if effect_data.get("stat_modifiers") or (spell_ac_key and spell_ac_key in ac_modifiers):
            if not has_effects:
                lines.append("- **Effects**:")
                has_effects = True

            # Track which locations we've already added
            added_locations = set()

            # Add AC modifier from combat_mechanics first if available
            if spell_ac_key and spell_ac_key in ac_modifiers:
                ac_data = ac_modifiers[spell_ac_key]
                ac_bonus = ac_data.get("bonus", ac_data.get("penalty", ""))
                formula = ac_data.get("formula", "")
                source = ac_data.get("source", "")
                note = ac_data.get("note", "")

                if formula:
                    if note:
                        lines.append(f"  - **APPLY_AC**: `{formula}` ({ac_bonus}) - {note}")
                    else:
                        lines.append(f"  - **APPLY_AC**: `{formula}` ({ac_bonus})")
                elif note:
                    lines.append(f"  - **APPLY_AC**: {ac_bonus} - {note}")
                else:
                    lines.append(f"  - **APPLY_AC**: {ac_bonus}")
                added_locations.add("APPLY_AC")

            # Then add stat modifiers from effect_data (skip if already added)
            for mod in effect_data.get("stat_modifiers", []):
                location = mod.get("location", "")

                # Skip if we already added this location from combat_mechanics
                if location in added_locations:
                    continue

                value = mod.get("value", "")
                formula = mod.get("formula", "")
                condition = mod.get("condition", "")
                note = mod.get("note", "")

                if formula:
                    if condition:
                        lines.append(f"  - **{location}**: `{formula}` ({value}) - {condition}")
                    elif note:
                        lines.append(f"  - **{location}**: `{formula}` ({value}) - {note}")
                    else:
                        lines.append(f"  - **{location}**: `{formula}` ({value})")
                else:
                    lines.append(f"  - **{location}**: {value}")

                added_locations.add(location)
    elif spell_ac_key and spell_ac_key in ac_modifiers:
        # No effect flag data, but we have AC modifier
        lines.append("- **Effects**:")
        ac_data = ac_modifiers[spell_ac_key]
        ac_bonus = ac_data.get("bonus", ac_data.get("penalty", ""))
        formula = ac_data.get("formula", "")
        source = ac_data.get("source", "")
        note = ac_data.get("note", "")

        if formula:
            if note:
                lines.append(f"  - **APPLY_AC**: `{formula}` ({ac_bonus}) - {note}")
            else:
                lines.append(f"  - **APPLY_AC**: `{formula}` ({ac_bonus})")
        elif note:
            lines.append(f"  - **APPLY_AC**: {ac_bonus} - {note}")
        else:
            lines.append(f"  - **APPLY_AC**: {ac_bonus}")

    # Add remaining effect flag data if available
    if effect_key and effect_key in json_data.get("effect_flags", {}):
        effect_data = json_data["effect_flags"][effect_key]

        # Duration from effect (if not already added)
        if not spell_key and "duration" in effect_data:
            dur = effect_data["duration"]
            if isinstance(dur, dict):
                formula = dur.get("formula", "unknown")
                range_str = dur.get("range", "unknown")
                lines.append(f"- **Duration**: `{formula}` ({range_str})")
            else:
                lines.append(f"- **Duration**: {dur}")

        # Special effects
        if effect_data.get("special_effects"):
            lines.append("- **Special Mechanics**:")
            for effect in effect_data["special_effects"]:
                lines.append(f"  - {effect}")

        # Damage reduction
        if effect_data.get("damage_reduction"):
            dr = effect_data["damage_reduction"]
            if isinstance(dr, dict):
                formula = dr.get("formula", "")
                percentage = dr.get("percentage", "")
                lines.append(f"- **Damage Reduction**: {percentage} (`{formula}`)")
            else:
                lines.append(f"- **Damage Reduction**: {dr}")

        # Attack bonus
        if effect_data.get("attack_bonus"):
            ab = effect_data["attack_bonus"]
            if isinstance(ab, dict):
                formula = ab.get("formula", "")
                lines.append(f"- **Attack Bonus**: `{formula}`")

        # Restrictions
        if effect_data.get("restrictions"):
            lines.append("- **Restrictions**:")
            for restriction in effect_data["restrictions"]:
                lines.append(f"  - {restriction}")

        # Immunities
        if effect_data.get("immunities"):
            lines.append("- **Immunities**:")
            for immunity in effect_data["immunities"]:
                lines.append(f"  - {immunity}")

        # Save allowed
        if effect_data.get("save_allowed"):
            save_type = effect_data.get("save_type", "unknown")
            lines.append(f"- **Save Allowed**: {save_type}")
            if effect_data.get("save_bonus"):
                sb = effect_data["save_bonus"]
                if isinstance(sb, dict):
                    formula = sb.get("formula", "")
                    lines.append(f"  - Bonus: `{formula}`")

    # Source references
    sources = []
    if spell_key and spell_key in json_data.get("spell_durations", {}):
        sources.append(json_data["spell_durations"][spell_key].get("source", ""))

    # Add AC modifier source if applicable
    if spell_ac_key and spell_ac_key in ac_modifiers:
        ac_source = ac_modifiers[spell_ac_key].get("source", "")
        if ac_source:
            sources.append(ac_source)

    if effect_key and effect_key in json_data.get("effect_flags", {}):
        effect_data = json_data["effect_flags"][effect_key]

        for mod in effect_data.get("stat_modifiers", []):
            if mod.get("source"):
                sources.append(mod["source"])

        if effect_data.get("damage_reduction", {}).get("source"):
            sources.append(effect_data["damage_reduction"]["source"])

        if effect_data.get("attack_bonus", {}).get("source"):
            sources.append(effect_data["attack_bonus"]["source"])

        if effect_data.get("checked_in"):
            sources.extend(effect_data["checked_in"])

    # Deduplicate and format sources
    unique_sources = list(dict.fromkeys(filter(None, sources)))
    if unique_sources:
        lines.append(f"- **Source**: {', '.join(unique_sources)}")

    return "\n".join(lines) if len(lines) > 2 else None

def update_abilities_md():
    """Update ABILITIES.md with implementation details"""

    # Load JSON data
    json_data = load_json_data()

    # Read ABILITIES.md
    md_path = Path("/home/strider/Code/mud/fierymud/docs/ABILITIES.md")
    with open(md_path, 'r') as f:
        content = f.read()

    # Split into lines for processing
    lines = content.split('\n')
    updated_lines = []
    i = 0

    while i < len(lines):
        line = lines[i]
        updated_lines.append(line)

        # Check if this is a spell header
        match = re.match(r'^#### (.+?) \(ID \d+\)$', line)
        if match:
            spell_name = match.group(1)

            # Find the end of this spell's metadata (next #### or end of section)
            j = i + 1
            while j < len(lines) and not lines[j].startswith('####'):
                updated_lines.append(lines[j])
                j += 1

            # Check if implementation section already exists
            has_implementation = False
            for k in range(i, j):
                if '**Implementation**:' in lines[k]:
                    has_implementation = True
                    break

            # Add implementation section if it doesn't exist and we have data
            if not has_implementation:
                impl_section = format_implementation_section(spell_name, json_data)
                if impl_section:
                    updated_lines.append(impl_section)

            i = j
            continue

        i += 1

    # Write updated content
    with open(md_path, 'w') as f:
        f.write('\n'.join(updated_lines))

    print(f"Updated {md_path}")
    print(f"Added implementation details for {len(SPELL_MAPPINGS)} spells")

if __name__ == "__main__":
    update_abilities_md()
