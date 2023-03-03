#pragma once

#include "structs.hpp"

/* Sorcerer spell lists */
const SpellPair mob_sorcerer_buffs[] = {{SPELL_STONE_SKIN, 0, EFF_STONE_SKIN},
                                        {SPELL_HASTE, 0, EFF_HASTE},
                                        {SPELL_COLDSHIELD, 0, EFF_COLDSHIELD},
                                        {SPELL_FIRESHIELD, 0, EFF_FIRESHIELD},
                                        {SPELL_WATERFORM, 0, 0},
                                        {SPELL_VAPORFORM, 0, 0},
                                        {SPELL_NEGATE_HEAT, 0, EFF_NEGATE_HEAT},
                                        {SPELL_NEGATE_COLD, 0, EFF_NEGATE_COLD},
                                        {SPELL_MAJOR_GLOBE, 0, EFF_MAJOR_GLOBE},
                                        {SPELL_MINOR_GLOBE, 0, EFF_MINOR_GLOBE},
                                        {SPELL_DETECT_INVIS, 0, EFF_DETECT_INVIS},
                                        {SPELL_INVISIBLE, 0, EFF_INVISIBLE},
                                        {SPELL_FLY, 0, EFF_FLY},
                                        {SPELL_MIRAGE, 0, 0},
                                        {SPELL_MISDIRECTION, 0, EFF_MISDIRECTION},
                                        {SPELL_ICE_ARMOR, 0, 0},
                                        {SPELL_BONE_ARMOR, 0, 0},
                                        {0, 0, 0}};

/* In order of spells to attempt */
const int mob_sorcerer_offensives[] = {
    SPELL_DISINTEGRATE,   SPELL_BIGBYS_CLENCHED_FIST, SPELL_ICEBALL,    SPELL_MELT,         SPELL_ACID_BURST,
    SPELL_POSITIVE_FIELD, SPELL_DEGENERATION,         SPELL_SOUL_TAP,   SPELL_CONE_OF_COLD, SPELL_ENERGY_DRAIN,
    SPELL_CONFUSION,      SPELL_DISINTEGRATE,         SPELL_NIGHTMARE,  SPELL_MESMERIZE,    SPELL_FEAR,
    SPELL_FIREBALL,       SPELL_PHOSPHORIC_EMBERS,    SPELL_FREEZE,     SPELL_HYSTERIA,     SPELL_LIGHTNING_BOLT,
    SPELL_SHOCKING_GRASP, SPELL_CHILL_TOUCH,          SPELL_DETONATION, SPELL_FIRE_DARTS,   SPELL_BURNING_HANDS,
    SPELL_ICE_DARTS,      SPELL_MAGIC_MISSILE,        SPELL_SMOKE,      SPELL_SOUL_TAP,     SPELL_WATER_BLAST,
    0};

const int mob_sorcerer_area_spells[] = {SPELL_CHAIN_LIGHTNING,
                                        SPELL_COLOR_SPRAY,
                                        SPELL_CREMATE,
                                        SPELL_FIRESTORM,
                                        SPELL_FLOOD,
                                        SPELL_FREEZING_WIND,
                                        SPELL_ICE_SHARDS,
                                        SPELL_ICE_STORM,
                                        SPELL_METEORSWARM,
                                        SPELL_SEVERANCE,
                                        SPELL_SOUL_REAVER,
                                        SPELL_SUPERNOVA,
                                        0};