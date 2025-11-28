#!/usr/bin/env python3
"""Add effectTypes column to abilities.csv based on category and implementation analysis."""

import csv
import sys

def determine_effect_types(row):
    """Determine effect types based on ability characteristics."""
    category = row['category']
    damage_type = row['damageType']
    target_type = row['targetType']
    name = row['name'].lower()

    effect_types = []

    # Damage abilities
    if category == 'DAMAGE' or damage_type not in ['NONE', '']:
        if target_type == 'AREA_ROOM':
            effect_types.append('AREA_EFFECT')
        if 'magic missile' in name or 'darts' in name or 'arrows' in name:
            effect_types.append('MULTI_HIT')
        effect_types.append('DAMAGE')

    # Healing abilities
    if 'cure' in name or 'heal' in name or 'vigorize' in name:
        if 'cure blind' in name or 'cure poison' in name or 'cure curse' in name or 'cure paralysis' in name:
            effect_types.append('REMOVE_AFFECT')
        else:
            effect_types.append('HEAL')

    # Buff/Debuff abilities
    if category == 'BUFF':
        if name in ['blindness', 'sleep', 'poison', 'disease', 'paralysis', 'silence', 'fear',
                    'insanity', 'entangle', 'web', 'bone cage', 'confusion']:
            effect_types.append('APPLY_AFFECT')  # Debuff
        else:
            effect_types.append('APPLY_AFFECT')  # Buff

    # Removal effects
    if 'remove' in name or 'dispel' in name or 'cure blind' in name:
        if 'dispel magic' in name:
            effect_types.append('DISPEL')
        else:
            effect_types.append('REMOVE_AFFECT')

    # Teleport effects
    if category == 'TELEPORT' or 'teleport' in name or 'recall' in name or 'dimension door' in name or 'plane shift' in name:
        effect_types.append('TELEPORT')

    # Summon effects
    if category == 'SUMMON' or 'summon' in name or 'animate' in name or 'phantom' in name or 'simulacrum' in name:
        if 'summon corpse' in name or 'shift corpse' in name:
            effect_types.append('TELEPORT')
        elif row['name'] == 'summon':
            effect_types.append('TELEPORT')  # Summon spell brings target to caster
        else:
            effect_types.append('SUMMON')

    # Object creation
    if 'create' in name or 'flame blade' in name or 'ice dagger' in name or 'minor creation' in name:
        if 'create water' in name:
            effect_types.append('MODIFY_OBJECT')
        else:
            effect_types.append('CREATE_OBJECT')

    # Object modification
    if 'enchant' in name or 'bless' in name and 'weapon' in name:
        effect_types.append('MODIFY_OBJECT')

    # Message/utility effects
    if 'ventriloquate' in name or 'identify' in name or 'locate' in name or 'wizard eye' in name or 'farsee' in name:
        effect_types.append('MESSAGE')

    # Resource change
    if 'vigorize' in name or 'nourishment' in name:
        effect_types.append('RESOURCE_CHANGE')

    # Delayed effects (fog, pyre, circle of fire, etc.)
    if 'fog' in name or 'pyre' in name or 'circle of' in name or 'immolate' in name or 'soul tap' in name:
        if 'fog' in name or 'pyre' in name:
            effect_types.append('DELAYED_EFFECT')
        if damage_type != 'NONE' and 'DAMAGE' not in effect_types:
            effect_types.append('AREA_EFFECT')
            effect_types.append('DAMAGE')

    # Condition-based (harm heals undead)
    if name == 'harm':
        effect_types.append('CONDITION')
        effect_types.append('HEAL')  # Heals undead
        effect_types.append('DAMAGE')  # Damages living

    # Wall/environmental effects
    if 'wall' in name or 'darkness' in name or 'illumination' in name:
        effect_types.append('AREA_EFFECT')

    # Songs/Chants
    if row['type'] in ['SONG', 'CHANT']:
        effect_types.append('APPLY_AFFECT')

    # Skills
    if row['type'] == 'SKILL':
        if category == 'COMBAT' or category == 'STEALTH':
            if 'backstab' in name or 'throatcut' in name:
                effect_types.append('CONDITION')
                effect_types.append('DAMAGE')
            elif 'bash' in name:
                effect_types.append('DAMAGE')
                effect_types.append('APPLY_AFFECT')  # Knockdown
            elif 'disarm' in name:
                effect_types.append('CONDITION')
                effect_types.append('MODIFY_OBJECT')
            elif damage_type != 'NONE':
                effect_types.append('DAMAGE')
        elif category == 'MOVEMENT':
            if 'hide' in name or 'sneak' in name:
                effect_types.append('APPLY_AFFECT')
            elif 'track' in name:
                effect_types.append('MESSAGE')
        elif category == 'DEFENSIVE':
            if 'rescue' in name:
                effect_types.append('CONDITION')
            elif 'guard' in name:
                effect_types.append('CONDITION')
            else:
                effect_types.append('APPLY_AFFECT')
        elif 'steal' in name:
            effect_types.append('CONDITION')
            effect_types.append('MODIFY_OBJECT')

    # Default: if no effects determined and has damage, add DAMAGE
    if not effect_types and damage_type != 'NONE' and damage_type != '':
        effect_types.append('DAMAGE')

    # Default: if buff/utility with no specific effect
    if not effect_types and category in ['BUFF', 'UTILITY']:
        effect_types.append('APPLY_AFFECT')

    return ','.join(effect_types) if effect_types else 'NONE'


def main():
    input_file = '/home/strider/Code/mud/fierymud/docs/abilities.csv'
    output_file = '/home/strider/Code/mud/fierymud/docs/abilities_enhanced.csv'

    with open(input_file, 'r') as infile:
        reader = csv.DictReader(infile)
        fieldnames = reader.fieldnames + ['effectTypes']

        rows = []
        for row in reader:
            row['effectTypes'] = determine_effect_types(row)
            rows.append(row)

    with open(output_file, 'w', newline='') as outfile:
        writer = csv.DictWriter(outfile, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Enhanced CSV written to {output_file}")
    print(f"Processed {len(rows)} abilities")

if __name__ == '__main__':
    main()
