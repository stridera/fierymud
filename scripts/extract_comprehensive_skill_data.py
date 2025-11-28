#!/usr/bin/env python3
"""
Enhanced skill metadata extraction with class-level data and comprehensive implementation analysis.
"""

import re
import json
from pathlib import Path
from typing import Dict, List, Any, Optional
from collections import defaultdict

# Paths
FIERYMUD_ROOT = Path(__file__).parent.parent
LEGACY_SRC = FIERYMUD_ROOT / "legacy" / "src"

# Class names from class.hpp
CLASS_NAMES = [
    "SORCERER", "CLERIC", "THIEF", "WARRIOR", "PALADIN", "ANTI_PALADIN",
    "RANGER", "DRUID", "SHAMAN", "ASSASSIN", "MERCENARY", "NECROMANCER",
    "CONJURER", "MONK", "BERSERKER", "PRIEST", "DIABOLIST", "MYSTIC",
    "ROGUE", "BARD", "HUNTER"  # Added HUNTER (CLASS_HUNTER)
]

class ComprehensiveSkillExtractor:
    def __init__(self):
        self.skills = {}
        self.skill_ids = {}
        self.class_assignments = defaultdict(lambda: defaultdict(int))  # skill -> class -> level
        self.race_assignments = defaultdict(lambda: defaultdict(int))   # skill -> race -> level
        self.implementations = defaultdict(list)

    def extract_skill_ids(self):
        """Extract skill ID definitions"""
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
                    'wearoff': wearoff.strip().strip('"') if wearoff.strip() != 'nullptr' else None,
                    'fighting_ok': True,
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

    def extract_class_assignments(self):
        """Extract skill_assign calls to get min level per class"""
        class_file = LEGACY_SRC / "class.cpp"

        # Pattern: skill_assign(SKILL_NAME, CLASS_NAME, level)
        assign_pattern = re.compile(
            r'skill_assign\(\s*'
            r'(SKILL_\w+|CHANT_\w+|SONG_\w+)\s*,\s*'  # skill
            r'(CLASS_\w+|i)\s*,\s*'                    # class (or 'i' in loop)
            r'(\d+)\s*\)'                              # level
        )

        with open(class_file, 'r', encoding='latin-1') as f:
            content = f.read()

        # Track current loop variable context
        for match in assign_pattern.finditer(content):
            skill_name, class_name, level = match.groups()
            level = int(level)

            # If class is 'i', it's in a loop - find the loop context
            if class_name == 'i':
                # Look backwards to find the loop
                before_text = content[:match.start()]
                loop_match = re.search(r'for\s*\(\s*int\s+i\s*=\s*(\d+)\s*;\s*i\s*<\s*(\d+)', before_text[::-1])
                if loop_match:
                    # This is a loop over all classes - store for all
                    for class_idx, class_name_str in enumerate(CLASS_NAMES):
                        self.class_assignments[skill_name][class_name_str] = level
            else:
                # Direct assignment to specific class
                class_short = class_name.replace('CLASS_', '')
                if skill_name in self.skills or skill_name in self.skill_ids:
                    self.class_assignments[skill_name][class_short] = level

        print(f"Extracted class assignments for {len(self.class_assignments)} skills")
        return self.class_assignments

    def find_implementations(self):
        """Find skill implementations in source code"""
        search_files = [
            'act.offensive.cpp',
            'act.movement.cpp',
            'act.other.cpp',
            'act.item.cpp',
            'fight.cpp',
            'skills.cpp',
            'warrior.cpp',
            'rogue.cpp',
            'casting.cpp',
            'spell_parser.cpp',
        ]

        for filename in search_files:
            filepath = LEGACY_SRC / filename
            if not filepath.exists():
                continue

            with open(filepath, 'r', encoding='latin-1', errors='ignore') as f:
                content = f.read()

            # Find ACMD functions
            acmd_pattern = re.compile(r'ACMD\(\s*do_(\w+)\s*\)\s*{')
            for match in acmd_pattern.finditer(content):
                func_name = match.group(1)
                line_num = content[:match.start()].count('\n') + 1

                # Try to match to skills
                for skill_id, skill_data in self.skills.items():
                    skill_name_normalized = skill_data['name'].lower().replace(' ', '_')
                    if func_name.lower() == skill_name_normalized or func_name.lower() in skill_name_normalized:
                        self.implementations[skill_id].append({
                            'file': filename,
                            'type': 'ACMD',
                            'function': f'do_{func_name}',
                            'line': line_num
                        })

            # Find direct skill references in switch statements
            for skill_id in self.skills.keys():
                pattern = re.compile(rf'\bcase\s+{skill_id}\s*:')
                for match in pattern.finditer(content):
                    line_num = content[:match.start()].count('\n') + 1
                    self.implementations[skill_id].append({
                        'file': filename,
                        'type': 'switch_case',
                        'context': skill_id,
                        'line': line_num
                    })

        print(f"Found implementations for {len(self.implementations)} skills")
        return self.implementations

    def categorize_skills(self):
        """Categorize skills"""
        categories = defaultdict(list)

        weapon_keywords = ['BLUDGEONING', 'PIERCING', 'SLASHING', 'MISSILE', 'BAREHAND', 'DUAL_WIELD']
        sphere_keywords = ['SPHERE_']
        movement_keywords = ['SNEAK', 'HIDE', 'TRACK', 'MOUNT', 'RIDING', 'RETREAT', 'SAFEFALL', 'SPRINGLEAP']
        stealth_keywords = ['BACKSTAB', 'CIRCLE', 'HIDE', 'SNEAK', 'STEAL', 'PICK_LOCK', 'SHADOW',
                           'CONCEAL', 'STEALTH', 'SNEAK_ATTACK', 'THROATCUT']
        combat_keywords = ['BASH', 'KICK', 'PUNCH', 'DISARM', 'RESCUE', 'PARRY', 'DODGE', 'RIPOSTE',
                          'BERSERK', 'BODYSLAM', 'SWEEP', 'ROUNDHOUSE', 'MAUL', 'BIND']
        passive_keywords = ['DOUBLE_ATTACK', 'AWARE', 'QUICK_CHANT', 'MEDITATE', 'KNOW_SPELL']

        for skill_id, skill_data in self.skills.items():
            skill_name_upper = skill_id.replace('SKILL_', '').replace('SONG_', '').replace('CHANT_', '')

            if skill_data['type'] == 'song':
                categories['songs'].append(skill_id)
            elif skill_data['type'] == 'chant':
                categories['chants'].append(skill_id)
            elif any(kw in skill_name_upper for kw in weapon_keywords):
                categories['weapon_proficiency'].append(skill_id)
            elif any(kw in skill_name_upper for kw in sphere_keywords):
                categories['spell_sphere'].append(skill_id)
            elif any(kw in skill_name_upper for kw in movement_keywords):
                categories['movement'].append(skill_id)
            elif any(kw in skill_name_upper for kw in stealth_keywords):
                categories['stealth'].append(skill_id)
            elif any(kw in skill_name_upper for kw in combat_keywords):
                categories['combat'].append(skill_id)
            elif any(kw in skill_name_upper for kw in passive_keywords):
                categories['passive'].append(skill_id)
            elif skill_id in self.implementations and len(self.implementations[skill_id]) > 0:
                categories['combat'].append(skill_id)
            else:
                categories['utility'].append(skill_id)

        return categories

    def run(self):
        """Run full extraction"""
        print("=== Comprehensive FieryMUD Skill System Analysis ===\n")

        print("Step 1: Extracting skill IDs...")
        self.extract_skill_ids()

        print("\nStep 2: Extracting skill definitions...")
        self.extract_skill_definitions()

        print("\nStep 3: Extracting class assignments...")
        self.extract_class_assignments()

        print("\nStep 4: Finding implementations...")
        self.find_implementations()

        print("\nStep 5: Categorizing...")
        categories = self.categorize_skills()

        print("\nStep 6: Generating output files...")

        # Build comprehensive metadata
        full_metadata = {}
        for skill_id, skill_data in self.skills.items():
            full_metadata[skill_id] = {
                **skill_data,
                'class_levels': dict(self.class_assignments.get(skill_id, {})),
                'implementations': self.implementations.get(skill_id, []),
                'categories': [cat for cat, skills in categories.items() if skill_id in skills],
            }

        # Write skill_metadata.json (comprehensive version)
        output_file = FIERYMUD_ROOT / "skill_metadata_comprehensive.json"
        with open(output_file, 'w') as f:
            json.dump(full_metadata, f, indent=2)
        print(f"  → {output_file}")

        # Write analysis
        analysis = {
            'categories': {cat: [self.skills[s]['name'] for s in skills]
                          for cat, skills in categories.items()},
            'implementation_patterns': {
                'ACMD_based': sum(1 for impls in self.implementations.values()
                                if any(i['type'] == 'ACMD' for i in impls)),
                'switch_case': sum(1 for impls in self.implementations.values()
                                 if any(i['type'] == 'switch_case' for i in impls)),
                'unimplemented': sum(1 for s in self.skills.keys()
                                   if s not in self.implementations or len(self.implementations[s]) == 0)
            },
            'class_coverage': {
                class_name: sum(1 for skill_assigns in self.class_assignments.values()
                              if class_name in skill_assigns)
                for class_name in CLASS_NAMES
            }
        }

        output_file = FIERYMUD_ROOT / "skill_analysis_comprehensive.json"
        with open(output_file, 'w') as f:
            json.dump(analysis, f, indent=2)
        print(f"  → {output_file}")

        # Summary
        print("\n=== Analysis Complete ===")
        print(f"  Total skills: {len(self.skills)}")
        print(f"  With class assignments: {len(self.class_assignments)}")
        print(f"  With implementations: {len(self.implementations)}")
        print(f"  Categories: {len(categories)}")

if __name__ == '__main__':
    extractor = ComprehensiveSkillExtractor()
    extractor.run()
