#pragma once

#include "structs.hpp"

/* Clerical spell lists */
const SpellPair mob_cleric_buffs[] = {{SPELL_SOULSHIELD, 0, EFF_SOULSHIELD},
                                      {SPELL_PROT_FROM_EVIL, 0, EFF_PROTECT_EVIL},
                                      {SPELL_PROT_FROM_GOOD, 0, EFF_PROTECT_GOOD},
                                      {SPELL_ARMOR, 0, 0},
                                      {SPELL_DEMONSKIN, 0, 0},
                                      {SPELL_GAIAS_CLOAK, 0, 0},
                                      {SPELL_BARKSKIN, 0, 0},
                                      {SPELL_DEMONIC_MUTATION, 0, 0},
                                      {SPELL_DEMONIC_ASPECT, 0, 0},
                                      {SPELL_SENSE_LIFE, 0, EFF_SENSE_LIFE},
                                      {SPELL_PRAYER, 0, 0},
                                      {SPELL_DARK_PRESENCE, 0, 0},
                                      {0, 0, 0}};

/* These spells should all be castable in combat. */
const SpellPair mob_cleric_hindrances[] = {{SPELL_BLINDNESS, SPELL_CURE_BLIND, EFF_BLIND},
                                           {SPELL_POISON, SPELL_REMOVE_POISON, EFF_POISON},
                                           {SPELL_DISEASE, SPELL_HEAL, EFF_DISEASE},
                                           {SPELL_CURSE, SPELL_REMOVE_CURSE, EFF_CURSE},
                                           {SPELL_INSANITY, SPELL_SANE_MIND, EFF_INSANITY},
                                           {SPELL_SILENCE, 0, 0}, /* Try to cast this, but there's no cure */
                                           {SPELL_ENTANGLE, SPELL_REMOVE_PARALYSIS, 0},
                                           {SPELL_WEB, SPELL_REMOVE_PARALYSIS, EFF_IMMOBILIZED},
                                           {0, 0, 0}};

const int mob_cleric_offensives[] = {
    SPELL_FULL_HARM,    SPELL_SUNRAY,         SPELL_FLAMESTRIKE,      SPELL_DIVINE_RAY,
    SPELL_HARM,         SPELL_DESTROY_UNDEAD, SPELL_STYGIAN_ERUPTION, SPELL_DISPEL_EVIL,
    SPELL_DISPEL_GOOD,  SPELL_WRITHING_WEEDS, SPELL_HELL_BOLT,        SPELL_DIVINE_BOLT,
    SPELL_CAUSE_CRITIC, SPELL_CAUSE_SERIOUS,  SPELL_CAUSE_LIGHT,      0};

const int mob_cleric_area_spells[] = {SPELL_HOLY_WORD, SPELL_UNHOLY_WORD, SPELL_EARTHQUAKE, 0};

const int mob_cleric_heals[] = {SPELL_FULL_HEAL,    SPELL_HEAL,       SPELL_CURE_CRITIC,
                                SPELL_CURE_SERIOUS, SPELL_CURE_LIGHT, 0};
