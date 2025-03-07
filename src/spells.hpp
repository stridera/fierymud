/***************************************************************************
 *   File: spells.h                                       Part of FieryMUD *
 *  Usage: header file for spells                                          *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium.       *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University.                                       *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

#define ASPELL(spellname)                                                                                              \
    int spellname(int spellnum, int skill, CharData *ch, CharData *victim, ObjData *obj, int savetype)

ASPELL(spell_acid_fog);
ASPELL(spell_armor_of_gaia);
ASPELL(spell_banish);
ASPELL(spell_charm);
ASPELL(spell_cloud_of_daggers);
ASPELL(spell_color_spray);
ASPELL(spell_control_weather);
ASPELL(spell_create_water);
ASPELL(spell_creeping_doom);
ASPELL(spell_enchant_weapon);
ASPELL(spell_degeneration);
ASPELL(spell_detect_poison);
ASPELL(spell_dark_feast);
ASPELL(spell_darkness);
ASPELL(spell_dimension_door);
ASPELL(spell_dispel_magic);
ASPELL(spell_divine_essence);
ASPELL(spell_energy_drain);
ASPELL(spell_enlightenment);
ASPELL(spell_fear);
ASPELL(spell_fire_breath);
ASPELL(spell_fire_darts);
ASPELL(spell_flame_blade);
ASPELL(spell_flood);
ASPELL(spell_fracture);
ASPELL(spell_frost_breath);
ASPELL(spell_gas_breath);
ASPELL(spell_greater_invocation);
ASPELL(spell_heavens_gate);
ASPELL(spell_hells_gate);
ASPELL(spell_ice_dagger);
ASPELL(spell_ice_darts);
ASPELL(spell_identify);
ASPELL(spell_illumination);
ASPELL(spell_immolate);
ASPELL(spell_isolation);
ASPELL(spell_lesser_invocation);
ASPELL(spell_lightning_breath);
ASPELL(spell_locate_object);
ASPELL(spell_magical_wall);
ASPELL(spell_magic_missile);
ASPELL(spell_major_paralysis);
ASPELL(spell_melt);
ASPELL(spell_minor_creation);
ASPELL(spell_moonbeam);
ASPELL(spell_moonwell);
ASPELL(spell_phosphoric_embers);
ASPELL(spell_plane_shift);
ASPELL(spell_preserve);
ASPELL(spell_pyre);
ASPELL(spell_pyre_recur);
ASPELL(spell_rain);
ASPELL(spell_recall);
ASPELL(spell_relocate);
ASPELL(spell_remove_curse);
ASPELL(spell_remove_paralysis);
ASPELL(spell_resurrect);
ASPELL(spell_reveal_hidden);
ASPELL(spell_shift_corpse);
ASPELL(spell_soul_tap);
ASPELL(spell_soul_tap_recur);
ASPELL(spell_spirit_arrows);
ASPELL(spell_summon);
ASPELL(spell_summon_corpse);
ASPELL(spell_supernova);
ASPELL(spell_world_teleport);
ASPELL(spell_teleport);
ASPELL(spell_ventriloquate);
ASPELL(spell_vitality);
ASPELL(spell_wandering_woods);
ASPELL(spell_wizard_eye);
ASPELL(spell_word_of_command);
ASPELL(chant_peace);
ASPELL(chant_ivory_symphony);
ASPELL(chant_apocalyptic_anthem);
