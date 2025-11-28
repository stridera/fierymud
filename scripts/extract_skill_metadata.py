#!/usr/bin/env python3
"""
Extract comprehensive skill metadata from FieryMUD legacy source code.
Analyzes skills, songs, and chants - extracting definitions, implementations, and help text.
"""

import re
import json
from pathlib import Path
from typing import Dict, List, Any, Optional
from collections import defaultdict

# Paths
FIERYMUD_ROOT = Path(__file__).parent.parent
LEGACY_SRC = FIERYMUD_ROOT / "legacy" / "src"
LIB_DIR = FIERYMUD_ROOT / "lib"

# Skill ID ranges from defines.hpp
SKILL_RANGE = (401, 495)
SONG_RANGE = (551, 560)
CHANT_RANGE = (601, 618)

class SkillExtractor:
    def __init__(self):
        self.skills = {}
        self.implementations = defaultdict(list)
        self.help_text = {}
        self.skill_ids = {}  # name -> id mapping

    def extract_skill_ids(self):
        """Extract skill ID definitions from defines.hpp"""
        defines_file = LEGACY_SRC / "defines.hpp"
        skill_pattern = re.compile(r'#define\s+(SKILL_|SONG_|CHANT_)(\w+)\s+(\d+)')

        with open(defines_file, 'r', encoding='latin-1') as f:
            for line in f:
                match = skill_pattern.match(line.strip())
                if match:
                    prefix, name, skill_id = match.groups()
                    full_name = prefix + name
                    skill_id = int(skill_id)
                    self.skill_ids[full_name] = skill_id

        print(f"Extracted {len(self.skill_ids)} skill IDs")
        return self.skill_ids

    def extract_skill_definitions(self):
        """Extract skill definitions from skills.cpp"""
        skills_file = LEGACY_SRC / "skills.cpp"

        # Patterns for different skill types
        skillo_pattern = re.compile(
            r'skillo\(\s*'
            r'(\w+)\s*,\s*'              # skill ID
            r'"([^"]+)"\s*,\s*'          # name
            r'(true|false)\s*,\s*'       # humanoid
            r'([^,]+)\s*,\s*'            # targets
            r'([^)]+)\)'                 # wearoff
        )

        chanto_pattern = re.compile(
            r'chanto\(\s*'
            r'(\w+)\s*,\s*'              # chant ID
            r'"([^"]+)"\s*,\s*'          # name
            r'(\w+)\s*,\s*'              # minpos
            r'(true|false)\s*,\s*'       # ok_fighting
            r'([^,]+)\s*,\s*'            # targets
            r'(true|false)\s*,\s*'       # violent
            r'([^,]+)\s*,\s*'            # routines
            r'([^,]+)\s*,\s*'            # damage
            r'(true|false)\s*,\s*'       # quest
            r'([^)]+)\)'                 # wearoff
        )

        songo_pattern = re.compile(
            r'songo\(\s*'
            r'(\w+)\s*,\s*'              # song ID
            r'"([^"]+)"\s*,\s*'          # name
            r'(\w+)\s*,\s*'              # minpos
            r'(true|false)\s*,\s*'       # ok_fighting
            r'([^,]+)\s*,\s*'            # targets
            r'(true|false)\s*,\s*'       # violent
            r'([^,]+)\s*,\s*'            # routines
            r'([^,]+)\s*,\s*'            # damage
            r'(true|false)\s*,\s*'       # quest
            r'([^)]+)\)'                 # wearoff
        )

        with open(skills_file, 'r', encoding='latin-1') as f:
            content = f.read()

        # Extract skillo definitions
        for match in skillo_pattern.finditer(content):
            skill_id, name, humanoid, targets, wearoff = match.groups()
            if skill_id in self.skill_ids:
                self.skills[skill_id] = {
                    'id': self.skill_ids[skill_id],
                    'name': name,
                    'type': 'skill',
                    'humanoid': humanoid == 'true',
                    'targets': targets.strip(),
                    'wearoff': wearoff.strip() if wearoff.strip() != 'nullptr' else None,
                    'fighting_ok': True,  # default for skills
                    'violent': 'TAR_CONTACT' in targets or 'TAR_DIRECT' in targets,
                }

        # Extract chanto definitions
        for match in chanto_pattern.finditer(content):
            chant_id, name, minpos, ok_fighting, targets, violent, routines, damage, quest, wearoff = match.groups()
            if chant_id in self.skill_ids:
                self.skills[chant_id] = {
                    'id': self.skill_ids[chant_id],
                    'name': name,
                    'type': 'chant',
                    'minpos': minpos,
                    'fighting_ok': ok_fighting == 'true',
                    'targets': targets.strip(),
                    'violent': violent == 'true',
                    'routines': routines.strip(),
                    'damage': damage.strip(),
                    'quest': quest == 'true',
                    'wearoff': wearoff.strip().strip('"') if 'nullptr' not in wearoff else None,
                    'humanoid': False,
                }

        # Extract songo definitions
        for match in songo_pattern.finditer(content):
            song_id, name, minpos, ok_fighting, targets, violent, routines, damage, quest, wearoff = match.groups()
            if song_id in self.skill_ids:
                self.skills[song_id] = {
                    'id': self.skill_ids[song_id],
                    'name': name,
                    'type': 'song',
                    'minpos': minpos,
                    'fighting_ok': ok_fighting == 'true',
                    'targets': targets.strip(),
                    'violent': violent == 'true',
                    'routines': routines.strip(),
                    'damage': damage.strip(),
                    'quest': quest == 'true',
                    'wearoff': wearoff.strip().strip('"') if 'nullptr' not in wearoff else None,
                    'humanoid': False,
                }

        print(f"Extracted {len(self.skills)} skill definitions")
        return self.skills

    def find_skill_implementations(self):
        """Find where skills are implemented in source files"""
        # Common skill command patterns
        command_patterns = [
            (r'ACMD\(\s*do_(\w+)\s*\)', 'command'),
            (r'ASKILL\(\s*skill_(\w+)\s*\)', 'skill_function'),
            (r'case\s+SKILL_(\w+):', 'switch_case'),
        ]

        # Files to search
        search_files = [
            'act.offensive.cpp',
            'act.movement.cpp',
            'act.other.cpp',
            'fight.cpp',
            'skills.cpp',
            'warrior.cpp',
            'casting.cpp',
        ]

        for filename in search_files:
            filepath = LEGACY_SRC / filename
            if not filepath.exists():
                continue

            with open(filepath, 'r', encoding='latin-1', errors='ignore') as f:
                content = f.read()

            # Search for each skill
            for skill_id, skill_data in self.skills.items():
                skill_name = skill_data['name'].lower().replace(' ', '_')

                # Look for command implementations
                for pattern, impl_type in command_patterns:
                    matches = re.finditer(pattern, content)
                    for match in matches:
                        func_name = match.group(1).lower() if match.lastindex >= 1 else skill_id.lower()
                        if func_name in skill_name or skill_name in func_name:
                            self.implementations[skill_id].append({
                                'file': filename,
                                'type': impl_type,
                                'function': match.group(0),
                                'line': content[:match.start()].count('\n') + 1
                            })

                # Look for direct skill name references
                if re.search(rf'\b{skill_id}\b', content):
                    # Find the context around this reference
                    for match in re.finditer(rf'(.{{0,100}})\b{skill_id}\b(.{{0,100}})', content):
                        context = match.group(1) + match.group(2)
                        if 'ACMD' in context or 'ASKILL' in context or 'case' in context:
                            self.implementations[skill_id].append({
                                'file': filename,
                                'type': 'reference',
                                'context': context.strip()[:200],
                                'line': content[:match.start()].count('\n') + 1
                            })

        print(f"Found implementations for {len(self.implementations)} skills")
        return self.implementations

    def categorize_skills(self):
        """Categorize skills by type and implementation pattern"""
        categories = {
            'combat': [],
            'utility': [],
            'passive': [],
            'weapon_proficiency': [],
            'spell_sphere': [],
            'movement': [],
            'stealth': [],
            'songs': [],
            'chants': [],
            'unimplemented': []
        }

        # Weapon proficiency skills
        weapon_skills = ['BLUDGEONING', 'PIERCING', 'SLASHING', '2H_BLUDGEONING',
                        '2H_PIERCING', '2H_SLASHING', 'MISSILE', 'BAREHAND', 'DUAL_WIELD']

        # Spell sphere skills
        sphere_skills = ['SPHERE_GENERIC', 'SPHERE_FIRE', 'SPHERE_WATER', 'SPHERE_EARTH',
                        'SPHERE_AIR', 'SPHERE_HEALING', 'SPHERE_PROT', 'SPHERE_ENCHANT',
                        'SPHERE_SUMMON', 'SPHERE_DEATH', 'SPHERE_DIVIN', 'KNOW_SPELL']

        # Movement skills
        movement_skills = ['SNEAK', 'HIDE', 'TRACK', 'MOUNT', 'RIDING', 'RETREAT',
                          'GROUP_RETREAT', 'SAFEFALL', 'SPRINGLEAP']

        # Stealth skills
        stealth_skills = ['BACKSTAB', 'CIRCLE', 'HIDE', 'SNEAK', 'STEAL', 'PICK_LOCK',
                         'SHADOW', 'CONCEAL', 'STEALTH', 'SNEAK_ATTACK', 'THROATCUT']

        # Combat skills
        combat_skills = ['BASH', 'KICK', 'PUNCH', 'DISARM', 'RESCUE', 'PARRY', 'DODGE',
                        'RIPOSTE', 'BERSERK', 'BODYSLAM', 'SWEEP', 'ROUNDHOUSE', 'MAUL']

        # Passive skills
        passive_skills = ['DOUBLE_ATTACK', 'AWARE', 'QUICK_CHANT', 'MEDITATE']

        for skill_id, skill_data in self.skills.items():
            skill_name_upper = skill_id.replace('SKILL_', '').replace('SONG_', '').replace('CHANT_', '')

            if skill_data['type'] == 'song':
                categories['songs'].append(skill_id)
            elif skill_data['type'] == 'chant':
                categories['chants'].append(skill_id)
            elif any(weapon in skill_name_upper for weapon in weapon_skills):
                categories['weapon_proficiency'].append(skill_id)
            elif any(sphere in skill_name_upper for sphere in sphere_skills):
                categories['spell_sphere'].append(skill_id)
            elif any(move in skill_name_upper for move in movement_skills):
                categories['movement'].append(skill_id)
            elif any(stealth in skill_name_upper for stealth in stealth_skills):
                categories['stealth'].append(skill_id)
            elif any(combat in skill_name_upper for combat in combat_skills):
                categories['combat'].append(skill_id)
            elif any(passive in skill_name_upper for passive in passive_skills):
                categories['passive'].append(skill_id)
            elif skill_id in self.implementations and len(self.implementations[skill_id]) > 0:
                categories['combat'].append(skill_id)  # Default active skills to combat
            else:
                categories['utility'].append(skill_id)

        return categories

    def extract_help_text(self):
        """Extract help text from help files"""
        help_files = [
            LIB_DIR / "text" / "help" / "commands.hlp",
            LIB_DIR / "text" / "help" / "skills.hlp",
        ]

        for help_file in help_files:
            if not help_file.exists():
                continue

            try:
                with open(help_file, 'r', encoding='latin-1', errors='ignore') as f:
                    content = f.read()

                # Parse help entries (format: topic name followed by text until next #)
                entries = re.split(r'\n#\s*\n', content)

                for entry in entries:
                    lines = entry.strip().split('\n')
                    if len(lines) < 2:
                        continue

                    # First line is the topic
                    topics = [t.strip().upper() for t in lines[0].strip().split()]
                    help_text = '\n'.join(lines[1:]).strip()

                    # Try to match topics to skills
                    for topic in topics:
                        for skill_id, skill_data in self.skills.items():
                            skill_name = skill_data['name'].upper().replace(' ', '_')
                            if topic in skill_name or skill_name in topic:
                                self.help_text[skill_id] = help_text
                                break
            except Exception as e:
                print(f"Error reading {help_file}: {e}")

        print(f"Extracted help text for {len(self.help_text)} skills")
        return self.help_text

    def generate_summary(self):
        """Generate summary statistics"""
        categories = self.categorize_skills()

        summary = {
            'total_skills': len(self.skills),
            'by_type': {
                'skills': sum(1 for s in self.skills.values() if s['type'] == 'skill'),
                'songs': sum(1 for s in self.skills.values() if s['type'] == 'song'),
                'chants': sum(1 for s in self.skills.values() if s['type'] == 'chant'),
            },
            'by_category': {cat: len(skills) for cat, skills in categories.items()},
            'implemented': len(self.implementations),
            'with_help': len(self.help_text),
            'humanoid_only': sum(1 for s in self.skills.values() if s.get('humanoid', False)),
            'violent': sum(1 for s in self.skills.values() if s.get('violent', False)),
            'quest_only': sum(1 for s in self.skills.values() if s.get('quest', False)),
        }

        return summary

    def run(self):
        """Run full extraction pipeline"""
        print("=== FieryMUD Skill System Analysis ===\n")

        # Step 1: Extract skill IDs
        print("Step 1: Extracting skill IDs from defines.hpp...")
        self.extract_skill_ids()

        # Step 2: Extract skill definitions
        print("\nStep 2: Extracting skill definitions from skills.cpp...")
        self.extract_skill_definitions()

        # Step 3: Find implementations
        print("\nStep 3: Finding skill implementations...")
        self.find_skill_implementations()

        # Step 4: Extract help text
        print("\nStep 4: Extracting help text...")
        self.extract_help_text()

        # Step 5: Categorize
        print("\nStep 5: Categorizing skills...")
        categories = self.categorize_skills()

        # Step 6: Generate outputs
        print("\nStep 6: Generating output files...")

        # Merge all data
        full_metadata = {}
        for skill_id, skill_data in self.skills.items():
            full_metadata[skill_id] = {
                **skill_data,
                'implementations': self.implementations.get(skill_id, []),
                'help_text': self.help_text.get(skill_id, None),
            }

        # Write skill_metadata.json
        output_file = FIERYMUD_ROOT / "skill_metadata.json"
        with open(output_file, 'w') as f:
            json.dump(full_metadata, f, indent=2)
        print(f"  → {output_file}")

        # Write skill_implementation_analysis.json
        impl_analysis = {
            'categories': categories,
            'implementations': dict(self.implementations),
            'implementation_patterns': {
                'command_based': sum(1 for impls in self.implementations.values()
                                   if any(i['type'] == 'command' for i in impls)),
                'skill_function': sum(1 for impls in self.implementations.values()
                                    if any(i['type'] == 'skill_function' for i in impls)),
                'switch_case': sum(1 for impls in self.implementations.values()
                                 if any(i['type'] == 'switch_case' for i in impls)),
            }
        }

        output_file = FIERYMUD_ROOT / "skill_implementation_analysis.json"
        with open(output_file, 'w') as f:
            json.dump(impl_analysis, f, indent=2)
        print(f"  → {output_file}")

        # Write skill_summary.md
        summary = self.generate_summary()

        md_content = f"""# FieryMUD Skill System Summary

## Overview

Total Skills/Songs/Chants: **{summary['total_skills']}**

### By Type
- Skills: {summary['by_type']['skills']}
- Songs (Bard): {summary['by_type']['songs']}
- Chants (Monk): {summary['by_type']['chants']}

### By Category
"""
        for cat, count in sorted(summary['by_category'].items()):
            md_content += f"- {cat.replace('_', ' ').title()}: {count}\n"

        md_content += f"""
### Implementation Status
- Implemented: {summary['implemented']} ({summary['implemented']/summary['total_skills']*100:.1f}%)
- With Help Text: {summary['with_help']} ({summary['with_help']/summary['total_skills']*100:.1f}%)

### Properties
- Humanoid Only: {summary['humanoid_only']}
- Violent: {summary['violent']}
- Quest Only: {summary['quest_only']}

## Skill Categories

### Combat Skills ({len(categories['combat'])})
"""
        for skill_id in sorted(categories['combat']):
            skill = self.skills[skill_id]
            md_content += f"- {skill['name']} (ID: {skill['id']})\n"

        md_content += f"\n### Weapon Proficiencies ({len(categories['weapon_proficiency'])})\n"
        for skill_id in sorted(categories['weapon_proficiency']):
            skill = self.skills[skill_id]
            md_content += f"- {skill['name']} (ID: {skill['id']})\n"

        md_content += f"\n### Movement Skills ({len(categories['movement'])})\n"
        for skill_id in sorted(categories['movement']):
            skill = self.skills[skill_id]
            md_content += f"- {skill['name']} (ID: {skill['id']})\n"

        md_content += f"\n### Stealth Skills ({len(categories['stealth'])})\n"
        for skill_id in sorted(categories['stealth']):
            skill = self.skills[skill_id]
            md_content += f"- {skill['name']} (ID: {skill['id']})\n"

        md_content += f"\n### Bard Songs ({len(categories['songs'])})\n"
        for skill_id in sorted(categories['songs']):
            skill = self.skills[skill_id]
            md_content += f"- {skill['name']} (ID: {skill['id']})\n"

        md_content += f"\n### Monk Chants ({len(categories['chants'])})\n"
        for skill_id in sorted(categories['chants']):
            skill = self.skills[skill_id]
            md_content += f"- {skill['name']} (ID: {skill['id']})\n"

        md_content += f"\n### Spell Spheres ({len(categories['spell_sphere'])})\n"
        for skill_id in sorted(categories['spell_sphere']):
            skill = self.skills[skill_id]
            md_content += f"- {skill['name']} (ID: {skill['id']})\n"

        md_content += f"\n### Passive Skills ({len(categories['passive'])})\n"
        for skill_id in sorted(categories['passive']):
            skill = self.skills[skill_id]
            md_content += f"- {skill['name']} (ID: {skill['id']})\n"

        md_content += f"\n### Utility Skills ({len(categories['utility'])})\n"
        for skill_id in sorted(categories['utility']):
            skill = self.skills[skill_id]
            md_content += f"- {skill['name']} (ID: {skill['id']})\n"

        output_file = FIERYMUD_ROOT / "skill_summary.md"
        with open(output_file, 'w') as f:
            f.write(md_content)
        print(f"  → {output_file}")

        print("\n=== Analysis Complete ===")
        print(f"\nSummary:")
        print(f"  Total: {summary['total_skills']} skills/songs/chants")
        print(f"  Implemented: {summary['implemented']}")
        print(f"  With Help: {summary['with_help']}")

if __name__ == '__main__':
    extractor = SkillExtractor()
    extractor.run()
