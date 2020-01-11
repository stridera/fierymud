/***************************************************************************
 *  File: skills.h                                        Part of FieryMUD *
 *  Usage: header file for character skill management                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_SKILLS_H
#define __FIERY_SKILLS_H

#include "sysdep.h"
#include "structs.h"
#include "class.h"

/* The general term for the abilities known as spells, skills, chants,
 * and songs is TALENT. */
#define TALENT 0
#define SPELL 1
#define SKILL 2
#define CHANT 3
#define SONG 4

#define ROLL_SKILL_PROF (-1)

#define MAX_SPELLS 400
#define MAX_SKILLS 550
#define MAX_SONGS 600
#define MAX_CHANTS 650

#define IS_SKILL(n) ((n) > MAX_SPELLS && (n) <= MAX_SKILLS)
#define IS_SPELL(n) ((n) >= 1 && (n) <= MAX_SPELLS)
#define IS_CHANT(n) ((n) > MAX_SONGS && (n) <= MAX_CHANTS)
#define IS_SONG(n) ((n) > MAX_SKILLS && (n) <= MAX_SONGS)

/* If you add another spell sphere, you'll have to modify these defines. */
#define IS_SPHERE_SKILL(n) ((n) >= SKILL_SPHERE_GENERIC && (n) <= SKILL_SPHERE_DIVIN)
#define NUM_SPHERE_SKILLS (SKILL_SPHERE_DIVIN - SKILL_SPHERE_GENERIC + 1)

#define SPELL_ARMOR 1
#define SPELL_TELEPORT 2
#define SPELL_BLESS 3
#define SPELL_BLINDNESS 4
#define SPELL_BURNING_HANDS 5
#define SPELL_CALL_LIGHTNING 6
#define SPELL_CHARM 7
#define SPELL_CHILL_TOUCH 8
#define SPELL_CLONE 9
#define SPELL_COLOR_SPRAY 10
#define SPELL_CONTROL_WEATHER 11
#define SPELL_CREATE_FOOD 12
#define SPELL_CREATE_WATER 13
#define SPELL_CURE_BLIND 14
#define SPELL_CURE_CRITIC 15
#define SPELL_CURE_LIGHT 16
#define SPELL_CURSE 17
#define SPELL_DETECT_ALIGN 18
#define SPELL_DETECT_INVIS 19
#define SPELL_DETECT_MAGIC 20
#define SPELL_DETECT_POISON 21
#define SPELL_DISPEL_EVIL 22
#define SPELL_EARTHQUAKE 23
#define SPELL_ENCHANT_WEAPON 24
#define SPELL_ENERGY_DRAIN 25
#define SPELL_FIREBALL 26
#define SPELL_HARM 27
#define SPELL_HEAL 28
#define SPELL_INVISIBLE 29
#define SPELL_LIGHTNING_BOLT 30
#define SPELL_LOCATE_OBJECT 31
#define SPELL_MAGIC_MISSILE 32
#define SPELL_POISON 33
#define SPELL_PROT_FROM_EVIL 34
#define SPELL_REMOVE_CURSE 35
#define SPELL_SANCTUARY 36
#define SPELL_SHOCKING_GRASP 37
#define SPELL_SLEEP 38
#define SPELL_STRENGTH 39
#define SPELL_SUMMON 40
#define SPELL_VENTRILOQUATE 41
#define SPELL_WORD_OF_RECALL 42
#define SPELL_REMOVE_POISON 43
#define SPELL_SENSE_LIFE 44
#define SPELL_ANIMATE_DEAD 45
#define SPELL_DISPEL_GOOD 46
#define SPELL_GROUP_ARMOR 47
#define SPELL_GROUP_HEAL 48
#define SPELL_GROUP_RECALL 49
#define SPELL_INFRAVISION 50
#define SPELL_WATERWALK 51
#define SPELL_STONE_SKIN 52
#define SPELL_FULL_HEAL 53
#define SPELL_FULL_HARM 54
#define SPELL_WALL_OF_FOG 55
#define SPELL_WALL_OF_STONE 56
#define SPELL_FLY 57
#define SPELL_SUMMON_DRACOLICH 58
#define SPELL_SUMMON_ELEMENTAL 59
#define SPELL_SUMMON_DEMON 60
#define SPELL_SUMMON_GREATER_DEMON 61
#define SPELL_DIMENSION_DOOR 62
#define SPELL_CREEPING_DOOM 63
#define SPELL_DOOM 64
#define SPELL_METEORSWARM 65
#define SPELL_BIGBYS_CLENCHED_FIST 66
#define SPELL_FARSEE 67
#define SPELL_HASTE 68
#define SPELL_BLUR 69
#define SPELL_GREATER_ENDURANCE 70
#define SPELL_MOONWELL 71
#define SPELL_INN_STRENGTH 72
#define SPELL_DARKNESS 73
#define SPELL_ILLUMINATION 74
#define SPELL_COMPREHEND_LANG 75
#define SPELL_CONE_OF_COLD 76
#define SPELL_ICE_STORM 77
#define SPELL_ICE_SHARDS 78
#define SPELL_MAJOR_PARALYSIS 79
#define SPELL_VAMPIRIC_BREATH 80
#define SPELL_RESURRECT 81
#define SPELL_INCENDIARY_NEBULA 82
#define SPELL_MINOR_PARALYSIS 83
#define SPELL_CAUSE_LIGHT 84
#define SPELL_CAUSE_SERIOUS 85
#define SPELL_CAUSE_CRITIC 86
#define SPELL_PRESERVE 87
#define SPELL_CURE_SERIOUS 88
#define SPELL_VIGORIZE_LIGHT 89
#define SPELL_VIGORIZE_SERIOUS 90
#define SPELL_VIGORIZE_CRITIC 91
#define SPELL_SOULSHIELD 92
#define SPELL_DESTROY_UNDEAD 93
#define SPELL_SILENCE 94
#define SPELL_FLAMESTRIKE 95
#define SPELL_UNHOLY_WORD 96
#define SPELL_HOLY_WORD 97
#define SPELL_PLANE_SHIFT 98
#define SPELL_DISPEL_MAGIC 99
#define SPELL_MINOR_CREATION 100
#define SPELL_CONCEALMENT 101
#define SPELL_RAY_OF_ENFEEB 102
#define SPELL_LEVITATE 103
#define SPELL_WIZARD_EYE 104
#define SPELL_FIRESHIELD 105
#define SPELL_COLDSHIELD 106
#define SPELL_MINOR_GLOBE 107
#define SPELL_MAJOR_GLOBE 108
#define SPELL_DISINTEGRATE 109
#define SPELL_HARNESS 110
#define SPELL_CHAIN_LIGHTNING 111
#define SPELL_MASS_INVIS 112
#define SPELL_RELOCATE 113
#define SPELL_FEAR 114
#define SPELL_CIRCLE_OF_LIGHT 115
#define SPELL_DIVINE_BOLT 116
#define SPELL_PRAYER 117
#define SPELL_ELEMENTAL_WARDING 118
#define SPELL_DIVINE_RAY 119
#define SPELL_LESSER_EXORCISM 120
#define SPELL_DECAY 121
#define SPELL_SPEAK_IN_TONGUES 122
#define SPELL_ENLIGHTENMENT 123
#define SPELL_EXORCISM 124
#define SPELL_SPINECHILLER 125
#define SPELL_WINGS_OF_HEAVEN 126
#define SPELL_BANISH 127
#define SPELL_WORD_OF_COMMAND 128
#define SPELL_DIVINE_ESSENCE 129
#define SPELL_HEAVENS_GATE 130
#define SPELL_DARK_PRESENCE 131
#define SPELL_DEMONSKIN 132
#define SPELL_DARK_FEAST 133
#define SPELL_HELL_BOLT 134
#define SPELL_DISEASE 135
#define SPELL_INSANITY 136
#define SPELL_DEMONIC_ASPECT 137
#define SPELL_HELLFIRE_BRIMSTONE 138
#define SPELL_STYGIAN_ERUPTION 139
#define SPELL_DEMONIC_MUTATION 140
#define SPELL_WINGS_OF_HELL 141
#define SPELL_SANE_MIND 142
#define SPELL_HELLS_GATE 143
#define SPELL_BARKSKIN 144
#define SPELL_NIGHT_VISION 145
#define SPELL_WRITHING_WEEDS 146
#define SPELL_CREATE_SPRING 147
#define SPELL_NOURISHMENT 148
#define SPELL_GAIAS_CLOAK 149
#define SPELL_NATURES_EMBRACE 150
#define SPELL_ENTANGLE 151
#define SPELL_INVIGORATE 152
#define SPELL_WANDERING_WOODS 153
#define SPELL_URBAN_RENEWAL 154
#define SPELL_SUNRAY 155
#define SPELL_ARMOR_OF_GAIA 156
#define SPELL_FIRE_DARTS 157
#define SPELL_MAGIC_TORCH 158
#define SPELL_SMOKE 159
#define SPELL_MIRAGE 160
#define SPELL_FLAME_BLADE 161
#define SPELL_POSITIVE_FIELD 162
#define SPELL_FIRESTORM 163
#define SPELL_MELT 164
#define SPELL_CIRCLE_OF_FIRE 165
#define SPELL_IMMOLATE 166
#define SPELL_SUPERNOVA 167
#define SPELL_CREMATE 168
#define SPELL_NEGATE_HEAT 169
#define SPELL_ACID_BURST 170
#define SPELL_ICE_DARTS 171
#define SPELL_ICE_ARMOR 172
#define SPELL_ICE_DAGGER 173
#define SPELL_FREEZING_WIND 174
#define SPELL_FREEZE 175
#define SPELL_WALL_OF_ICE 176
#define SPELL_ICEBALL 177
#define SPELL_FLOOD 178
#define SPELL_VAPORFORM 179
#define SPELL_NEGATE_COLD 180
#define SPELL_WATERFORM 181
#define SPELL_EXTINGUISH 182
#define SPELL_RAIN 183
#define SPELL_REDUCE 184
#define SPELL_ENLARGE 185
#define SPELL_IDENTIFY 186
#define SPELL_BONE_ARMOR 187
#define SPELL_SUMMON_CORPSE 188
#define SPELL_SHIFT_CORPSE 189
#define SPELL_GLORY 190
#define SPELL_ILLUSORY_WALL 191
#define SPELL_NIGHTMARE 192
#define SPELL_DISCORPORATE 193
#define SPELL_ISOLATION 194
#define SPELL_FAMILIARITY 195
#define SPELL_HYSTERIA 196
#define SPELL_MESMERIZE 197
#define SPELL_SEVERANCE 198
#define SPELL_SOUL_REAVER 199
#define SPELL_DETONATION 200
#define SPELL_FIRE_BREATH 201
#define SPELL_GAS_BREATH 202
#define SPELL_FROST_BREATH 203
#define SPELL_ACID_BREATH 204
#define SPELL_LIGHTNING_BREATH 205
#define SPELL_LESSER_ENDURANCE 206
#define SPELL_ENDURANCE 207
#define SPELL_VITALITY 208
#define SPELL_GREATER_VITALITY 209
#define SPELL_DRAGONS_HEALTH 210
#define SPELL_REBUKE_UNDEAD 211
#define SPELL_DEGENERATION 212
#define SPELL_SOUL_TAP 213
#define SPELL_NATURES_GUIDANCE 214
#define SPELL_MOONBEAM 215
#define SPELL_PHANTASM 216
#define SPELL_SIMULACRUM 217
#define SPELL_MISDIRECTION 218
#define SPELL_CONFUSION 219
#define SPELL_PHOSPHORIC_EMBERS 220
#define SPELL_RECALL 221
#define SPELL_PYRE 222
#define SPELL_IRON_MAIDEN 223
#define SPELL_FRACTURE 224
#define SPELL_FRACTURE_SHRAPNEL 225
#define SPELL_BONE_DRAW 226
#define SPELL_PYRE_RECOIL 227
#define SPELL_WORLD_TELEPORT 228
/* Insert new spells here, up to MAX_SPELLS */

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB 401
#define SKILL_BASH 402
#define SKILL_HIDE 403
#define SKILL_KICK 404
#define SKILL_PICK_LOCK 405
#define SKILL_PUNCH 406
#define SKILL_RESCUE 407
#define SKILL_SNEAK 408
#define SKILL_STEAL 409
#define SKILL_TRACK 410
#define SKILL_DUAL_WIELD 411
#define SKILL_DOUBLE_ATTACK 412
#define SKILL_BERSERK 413
#define SKILL_SPRINGLEAP 414
#define SKILL_MOUNT 415
#define SKILL_RIDING 416
#define SKILL_TAME 417
#define SKILL_THROATCUT 418
#define SKILL_DOORBASH 419
#define SKILL_PARRY 420
#define SKILL_DODGE 421
#define SKILL_RIPOSTE 422
#define SKILL_MEDITATE 423
#define SKILL_QUICK_CHANT 424
#define SKILL_2BACK 425
#define SKILL_CIRCLE 426
#define SKILL_BODYSLAM 427
#define SKILL_BIND 428
#define SKILL_SHAPECHANGE 429
#define SKILL_SWITCH 430
#define SKILL_DISARM 431
#define SKILL_DISARM_FUMBLING_WEAP 432
#define SKILL_DISARM_DROPPED_WEAP 433
#define SKILL_GUARD 434
#define SKILL_BREATHE 435
#define SKILL_SWEEP 436
#define SKILL_ROAR 437
#define SKILL_DOUSE 438
#define SKILL_AWARE 439
#define SKILL_INSTANT_KILL 440
#define SKILL_HITALL 441
#define SKILL_HUNT 442
#define SKILL_BANDAGE 443
#define SKILL_FIRST_AID 444
#define SKILL_VAMP_TOUCH 445
#define SKILL_CHANT 446
#define SKILL_SCRIBE 447
#define SKILL_SAFEFALL 448
#define SKILL_BAREHAND 449
#define SKILL_SUMMON_MOUNT 450
#define SKILL_KNOW_SPELL 451
#define SKILL_SPHERE_GENERIC 452
#define SKILL_SPHERE_FIRE 453
#define SKILL_SPHERE_WATER 454
#define SKILL_SPHERE_EARTH 455
#define SKILL_SPHERE_AIR 456
#define SKILL_SPHERE_HEALING 457
#define SKILL_SPHERE_PROT 458
#define SKILL_SPHERE_ENCHANT 459
#define SKILL_SPHERE_SUMMON 460
#define SKILL_SPHERE_DEATH 461
#define SKILL_SPHERE_DIVIN 462
#define SKILL_BLUDGEONING 463
#define SKILL_PIERCING 464
#define SKILL_SLASHING 465
#define SKILL_2H_BLUDGEONING 466
#define SKILL_2H_PIERCING 467
#define SKILL_2H_SLASHING 468
#define SKILL_MISSILE 469
/* what's this doing here ? */
/* We need a skill define for fire so we can have a damage message in the
   messages file. */
#define SPELL_ON_FIRE 470
#define SKILL_LAY_HANDS 471
#define SKILL_EYE_GOUGE 472
#define SKILL_RETREAT 473
#define SKILL_GROUP_RETREAT 474
#define SKILL_CORNER 475
#define SKILL_STEALTH 476
#define SKILL_SHADOW 477
#define SKILL_CONCEAL 478
#define SKILL_PECK 479
#define SKILL_CLAW 480
#define SKILL_ELECTRIFY 481
#define SKILL_TANTRUM 482
#define SKILL_GROUND_SHAKER 483
#define SKILL_BATTLE_HOWL 484
#define SKILL_MAUL 485

/* IF THIS GETS PAST 499, update char_data for skill timers! */

/* Bardic songs start at 551 and go to 600 */

/* Monk chants go from 601 to 650 */
#define CHANT_REGENERATION 601
#define CHANT_BATTLE_HYMN 602
#define CHANT_WAR_CRY 603
#define CHANT_PEACE 604
#define CHANT_SHADOWS_SORROW_SONG 605
#define CHANT_IVORY_SYMPHONY 606
#define CHANT_ARIA_OF_DISSONANCE 607
#define CHANT_SONATA_OF_MALAISE 608
#define CHANT_APOCALYPTIC_ANTHEM 609
#define CHANT_SEED_OF_DESTRUCTION 610
#define CHANT_SPIRIT_WOLF 611
#define CHANT_SPIRIT_BEAR 612
#define CHANT_INTERMINABLE_WRATH 613

/* New skills may be added here up to MAX_ABILITIES (650) */
/* Don't add spells/skills/songs/chants that will be saved past 650.  The
 * pfile won't support it.  However, you can add "npc" or "object" spells,
 * meaning that they may only be used in the game, and cannot be set on
 * players.
 */

#define TOP_SKILL_DEFINE 750
/* NEW NPC/OBJECT SPELLS can be inserted here up to 750 */

/* WEAPON ATTACK TYPES */

#define TYPE_HIT 751
#define TYPE_STING 752
#define TYPE_WHIP 753
#define TYPE_SLASH 754
#define TYPE_BITE 755
#define TYPE_BLUDGEON 756
#define TYPE_CRUSH 757
#define TYPE_POUND 758
#define TYPE_CLAW 759
#define TYPE_MAUL 760
#define TYPE_THRASH 761
#define TYPE_PIERCE 762
#define TYPE_BLAST 763
#define TYPE_PUNCH 764
#define TYPE_STAB 765

/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING 850

struct skilldef {
    const char *name;
    int minpos;       /* Minimum position to cast */
    bool fighting_ok; /* Whether it can be cast when fighting */
    int mana_min;     /* Min amount of mana used by a spell (highest lev) */
    int mana_max;     /* Max amount of mana used by a spell (lowest lev) */
    int mana_change;  /* Change in mana used by spell from lev to lev */
    int min_level[NUM_CLASSES];
    int lowest_level;
    bool humanoid; /* Generally only available to humanoids */
    int routines;
    byte violent;
    int targets; /* See spells.h for use with TAR_XXX  */
    int mem_time;
    int cast_time;
    int damage_type;
    int sphere;
    int pages; /* base number of pages for spell in spellbook */
    int quest; /* weather the spell is a quest spell or not   */
    const char *wearoff;
};

/* Attacktypes with grammar */

struct attack_hit_type {
    char *singular;
    char *plural;
};

#define SINFO skills[spellnum]

extern int level_to_circle(int level);
extern int circle_to_level(int circle);
#define IS_QUEST_SPELL(spellnum) (skills[(spellnum)].quest)
#define SKILL_LEVEL(ch, skillnum) (skills[(skillnum)].min_level[(int)GET_CLASS(ch)])
#define SPELL_CIRCLE(ch, spellnum) (level_to_circle(SKILL_LEVEL(ch, spellnum)))
#define CIRCLE_ABBR(ch, spellnum) (circle_abbrev[SPELL_CIRCLE((ch), (spellnum))])
#define SKILL_IS_TARGET(skill, tartype)                                                                                \
    ((skill) > 0 && (skill) <= TOP_SKILL_DEFINE && IS_SET(skills[skill].targets, (tartype)))

/* Function prototypes */

void init_skills(void);
void sort_skills(void);

int find_talent_num(char *name, int should_restrict);
int find_skill_num(char *name);
int find_spell_num(char *name);
int find_chant_num(char *name);
int find_song_num(char *name);

const char *skill_name(int num);
void improve_skill(struct char_data *ch, int skill);
void improve_skill_offensively(struct char_data *ch, struct char_data *victim, int skill);
void update_skills(struct char_data *ch);
void skill_assign(int skillnum, int class, int level);
int talent_type(int skill_num);
bool get_spell_assignment_circle(struct char_data *ch, int spell, int *circle_assignment, int *level_assignment);

/* Exported variables */

extern const char *talent_types[5];

extern struct skilldef skills[];
extern struct spell_dam spell_dam_info[MAX_SPELLS + 1];
extern int skill_sort_info[TOP_SKILL + 1];

#endif

/***************************************************************************
 * $Log: skills.h,v $
 * Revision 1.37  2010/06/05 18:35:47  mud
 * Make pyre auto-target caster if sacrificial preference is
 * toggled on.
 *
 * Revision 1.36  2010/06/05 14:57:06  mud
 * Moving cooldowns to their own file.
 *
 * Revision 1.35  2009/08/02 20:20:27  myc
 * Adding pyre and fracture spells.
 *
 * Revision 1.34  2009/07/18 01:17:23  myc
 * Adding decay, iron maiden, spinechiller, and bone draw spells
 * for necromancer.
 *
 * Revision 1.33  2009/03/21 19:11:37  myc
 * Save the duration each cooldown started at.
 *
 * Revision 1.32  2009/03/20 16:06:04  jps
 * Removed spells of lesser/greater invocation.
 *
 * Revision 1.31  2009/03/20 15:12:04  jps
 * Added a generic 'recall' spell to be assigned to town recall
 * scrolls, so that the object-spell verifier doesn't complain.
 *
 * Revision 1.30  2009/03/20 06:15:17  myc
 * Adding a TAR_GROUND cast requirement.  Added detonation,
 * phosphoric embers, positive field, and acid burst spells.
 * Removed combust and heatwave.  Made soul tap a manual spell.
 *
 * Revision 1.29  2009/03/16 09:44:13  jps
 * Allow skills to improve normally if assigned by race
 *
 * Revision 1.28  2008/09/11 02:50:02  jps
 * Changed skills so you have a minimum position, and fighting_ok fields.
 *
 * Revision 1.27  2008/09/01 18:29:38  jps
 * consolidating cooldown code in skills.c/h
 *
 * Revision 1.26  2008/08/25 00:20:33  myc
 * Changed the way mobs memorize spells.
 *
 * Revision 1.25  2008/08/10 19:33:58  jps
 * Moved level_to_circle to skills.c. Added get_spell_assignment_circle so
 * damage spells can be properly calibrated.
 *
 * Revision 1.24  2008/08/10 01:58:49  jps
 * Added spells severance and soul reaver for illusionists.
 *
 * Revision 1.23  2008/05/19 05:48:55  jps
 * Add def for mesmerize spell.
 *
 * Revision 1.22  2008/05/18 22:53:22  jps
 * Adding hysteria spell.
 *
 * Revision 1.21  2008/05/18 17:58:07  jps
 * Adding spell of familiarity.
 *
 * Revision 1.20  2008/05/18 02:02:05  jps
 * Adding isolation spell.
 *
 * Revision 1.19  2008/05/12 00:43:30  jps
 * Define nightmare and discorporate spells.
 *
 * Revision 1.18  2008/04/19 21:10:31  myc
 * Added a 'show skill' subcommand, which required a list of
 * target flag and routine type strings.
 *
 * Revision 1.17  2008/04/14 08:35:49  jps
 * Add def for illusory wall spell.
 *
 * Revision 1.16  2008/04/14 02:17:34  jps
 * Adding def for glory spell.
 *
 * Revision 1.15  2008/04/13 18:29:51  jps
 * Add spell of confusion.
 *
 * Revision 1.14  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.13  2008/04/04 20:47:43  jps
 * Make a macro for checking skill targets, since some places will
 * query it with invalid skills.
 *
 * Revision 1.12  2008/03/30 15:37:36  jps
 * Fix spelling of riposte.
 *
 * Revision 1.11  2008/03/21 15:01:17  myc
 * Removed languages.
 *
 * Revision 1.10  2008/03/09 18:09:57  jps
 * Defined spell MISDIRECTION.
 *
 * Revision 1.9  2008/03/05 03:03:54  myc
 * Removing GET_MEMMED macro.
 *
 * Revision 1.8  2008/02/23 01:03:54  myc
 * Moving spell circle and spell mem macros here from utils.h
 *
 * Revision 1.7  2008/02/02 05:35:14  myc
 * Exporting the skill_sort_info array.
 *
 * Revision 1.6  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.5  2008/01/29 16:51:12  myc
 * Adding skill names to the skilldef struct.
 *
 * Revision 1.4  2008/01/27 21:18:20  myc
 * Adding berserker skill and chant defines.
 *
 * Revision 1.3  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.2  2008/01/26 12:31:47  jps
 * Add improve_skills_offensively() so you won't improve skills when used
 * against illusions.
 *
 * Revision 1.1  2008/01/26 10:44:41  jps
 * Initial revision
 *
 ***************************************************************************/
