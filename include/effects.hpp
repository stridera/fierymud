/***************************************************************************
 *   File: effects.h                                      Part of FieryMUD *
 *  Usage: header file for effects                                         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

/* Effect bits: used in char_data.char_specials.effects */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define EFF_BLIND 0         /* (R) Char is blind            */
#define EFF_INVISIBLE 1     /* Char is invisible            */
#define EFF_DETECT_ALIGN 2  /* Char is sensitive to align   */
#define EFF_DETECT_INVIS 3  /* Char can see invis chars     */
#define EFF_DETECT_MAGIC 4  /* Char is sensitive to magic   */
#define EFF_SENSE_LIFE 5    /* Char can sense hidden life   */
#define EFF_WATERWALK 6     /* Char can walk on water       */
#define EFF_SANCTUARY 7     /* Char protected by sanct.     */
#define EFF_CONFUSION 8     /* Char is confused             */
#define EFF_CURSE 9         /* Char is cursed               */
#define EFF_INFRAVISION 10  /* Char can see in dark         */
#define EFF_POISON 11       /* (R) Char is poisoned         */
#define EFF_PROTECT_EVIL 12 /* Char protected from evil     */
#define EFF_PROTECT_GOOD 13 /* Char protected from good     */
#define EFF_SLEEP 14        /* (R) Char magically asleep    */
#define EFF_NOTRACK 15      /* Char can't be tracked        */
#define EFF_TAMED 16        /* Tamed!                       */
#define EFF_BERSERK 17      /* Char is berserking           */
#define EFF_SNEAK 18        /* Char is sneaking             */
#define EFF_STEALTH 19      /* Char is using stealth        */
#define EFF_FLY 20          /* Char has the ability to fly  */
#define EFF_CHARM 21        /* Char is charmed              */
#define EFF_STONE_SKIN 22
#define EFF_FARSEE 23
#define EFF_HASTE 24
#define EFF_BLUR 25
#define EFF_VITALITY 26
#define EFF_GLORY 27
#define EFF_MAJOR_PARALYSIS 28
#define EFF_FAMILIARITY 29 /* Char is considered friend    */
#define EFF_MESMERIZED 30  /* Super fasciated by something */
#define EFF_IMMOBILIZED 31 /* Char cannot move             */
#define EFF_LIGHT 32
/* Room for future expansion 33 */
#define EFF_MINOR_PARALYSIS 34
#define EFF_HURT_THROAT 35
#define EFF_LEVITATE 36
#define EFF_WATERBREATH 37
#define EFF_SOULSHIELD 38
#define EFF_SILENCE 39
#define EFF_PROT_FIRE 40
#define EFF_PROT_COLD 41
#define EFF_PROT_AIR 42
#define EFF_PROT_EARTH 43
#define EFF_FIRESHIELD 44
#define EFF_COLDSHIELD 45
#define EFF_MINOR_GLOBE 46
#define EFF_MAJOR_GLOBE 47
#define EFF_HARNESS 48
#define EFF_ON_FIRE 49
#define EFF_FEAR 50
#define EFF_TONGUES 51
#define EFF_DISEASE 52
#define EFF_INSANITY 53
#define EFF_ULTRAVISION 54
#define EFF_NEGATE_HEAT 55
#define EFF_NEGATE_COLD 56
#define EFF_NEGATE_AIR 57
#define EFF_NEGATE_EARTH 58
#define EFF_REMOTE_AGGR 59    /* Your aggro action won't remove invis/bless etc. */
#define EFF_FIREHANDS 60      /* Make Monks do burn damage with their hands */
#define EFF_ICEHANDS 61       /* Make Monks do cold damage with their hands */
#define EFF_LIGHTNINGHANDS 62 /* Make Monks do shock damage with their hands */
#define EFF_ACIDHANDS 63      /* Make Monks do acid damage with their hands */
#define EFF_AWARE 64
#define EFF_REDUCE 65
#define EFF_ENLARGE 66
#define EFF_VAMP_TOUCH 67
#define EFF_RAY_OF_ENFEEB 68
#define EFF_ANIMATED 69
/* Room for future expansion 70 */
#define EFF_SHADOWING 71
#define EFF_CAMOUFLAGED 72
#define EFF_SPIRIT_WOLF 73
#define EFF_SPIRIT_BEAR 74
#define EFF_WRATH 75
#define EFF_MISDIRECTION 76  /* Capable of performing misdirection */
#define EFF_MISDIRECTING 77  /* Currently actually moving but misdirecting */
#define EFF_BLESS 78         /* When blessed, your barehand attacks hurt ether chars */
#define EFF_HEX 79           /* The evil side of blessing, to hurt ether chars */
#define EFF_DETECT_POISON 80 /* Char is sensitive to poison */
#define EFF_SONG_OF_REST 81
#define NUM_EFF_FLAGS 82 /* Keep me updated */

extern const char *effect_flags[NUM_EFF_FLAGS + 1];
