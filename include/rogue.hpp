#pragma once

#include "defines.hpp"

/* Bard spell lists */
const SpellPair mob_bard_buffs[] = {{SPELL_DETECT_INVIS, 0, EFF_DETECT_INVIS}, {SPELL_HASTE, 0, EFF_HASTE}, {0, 0, 0}};

/* These spells should all be castable in combat. */
const SpellPair mob_bard_hindrances[] = {{SPELL_BLINDNESS, SPELL_CURE_BLIND, EFF_BLIND},
                                         {SPELL_WEB, SPELL_REMOVE_PARALYSIS, EFF_IMMOBILIZED},
                                         {SPELL_INSANITY, SPELL_SANE_MIND, EFF_INSANITY},
                                         {SPELL_POISON, SPELL_REMOVE_POISON, EFF_POISON},
                                         {SPELL_DISEASE, SPELL_HEAL, EFF_DISEASE},
                                         {SPELL_SILENCE, 0, 0}, /* Try to cast this, but there's no cure */
                                         {SPELL_CURSE, SPELL_REMOVE_CURSE, EFF_CURSE},
                                         {SPELL_ENTANGLE, SPELL_REMOVE_PARALYSIS, 0},
                                         {0, 0, 0}};

const int mob_bard_offensives[] = {
    SPELL_VICIOUS_MOCKERY, SPELL_HARM,          SPELL_SHOCKING_GRASP, SPELL_CAUSE_CRITIC, SPELL_CHILL_TOUCH,
    SPELL_CAUSE_SERIOUS,   SPELL_BURNING_HANDS, SPELL_MAGIC_MISSILE,  SPELL_CAUSE_LIGHT,  0};

const int mob_bard_area_spells[] = {SPELL_CLOUD_OF_DAGGERS, SPELL_COLOR_SPRAY, 0};

const int mob_bard_heals[] = {SPELL_HEAL, SPELL_CURE_CRITIC, SPELL_CURE_SERIOUS, SPELL_CURE_LIGHT, 0};
