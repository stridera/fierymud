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
const int mob_sorcerer_offensives[] = {SPELL_DEGENERATION,
                                       SPELL_DISINTEGRATE,
                                       SPELL_ICEBALL,
                                       SPELL_MELT,
                                       SPELL_SOUL_TAP,
                                       SPELL_BIGBYS_CLENCHED_FIST,
                                       SPELL_ACID_BURST,
                                       SPELL_FREEZE,
                                       SPELL_HYSTERIA,
                                       SPELL_FIREBALL,
                                       SPELL_WATER_BLAST,
                                       SPELL_POSITIVE_FIELD,
                                       SPELL_CONE_OF_COLD,
                                       SPELL_ENERGY_DRAIN,
                                       SPELL_CONFUSION,
                                       SPELL_NIGHTMARE,
                                       SPELL_LIGHTNING_BOLT,
                                       SPELL_MESMERIZE,
                                       SPELL_FEAR,
                                       SPELL_PHOSPHORIC_EMBERS,
                                       SPELL_SMOKE,
                                       SPELL_SHOCKING_GRASP,
                                       SPELL_CHILL_TOUCH,
                                       SPELL_DETONATION,
                                       SPELL_FIRE_DARTS,
                                       SPELL_BURNING_HANDS,
                                       SPELL_MAGIC_MISSILE,
                                       SPELL_ICE_DARTS,
                                       0};

const int mob_sorcerer_area_spells[] = {SPELL_SUPERNOVA,
                                        SPELL_SOUL_REAVER,
                                        SPELL_SEVERANCE,
                                        SPELL_ICE_SHARDS,
                                        SPELL_FLOOD,
                                        SPELL_METEORSWARM,
                                        SPELL_CREMATE,
                                        SPELL_CHAIN_LIGHTNING,
                                        SPELL_ICE_STORM,
                                        SPELL_FIRESTORM,
                                        SPELL_COLOR_SPRAY,
                                        SPELL_FREEZING_WIND,
                                        0};