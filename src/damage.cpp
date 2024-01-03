/***************************************************************************
 *  File: damage.c                                       Part of FieryMUD  *
 *  Usage: Source file for damage types                                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "damage.hpp"

#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

DamageDef damtypes[NUM_DAMTYPES] = {{"slash", "&3", "slash", "slashes", "slash"},
                                    {"pierce", "&3", "pierce", "pierces", "slash"},
                                    {"crush", "&3", "crush", "crushes", "crush"},
                                    {"shock", "&4&b", "shock", "shocks", "shock"},
                                    {"fire", "&1&b", "burn", "burns", "flame"},
                                    {"water", "&4", "drown", "drowns", "flood"},
                                    {"cold", "&4", "freeze", "freezes", "freeze"},
                                    {"acid", "&2", "corrode", "corrodes", "spray"},
                                    {"poison", "&2&b", "poison", "poisons", "poison"},
                                    {"heal", "&6", "harm", "harms", "harm"},
                                    {"align", "&6&b", "rebuke", "rebukes", "retribution"},
                                    {"dispel", "&5&b", "dispel", "dispels", "dispersion"},
                                    {"discorporate", "&5", "discorporate", "discorporates", "discorporation"},
                                    {"mental", "", "punish", "punishes", "punishment"}};

int parse_damtype(CharData *ch, char *arg) {
    return parse_obj_name(ch, arg, "damage type", NUM_DAMTYPES, damtypes, sizeof(DamageDef));
}

/* damage_evasion()
 *
 * Whether a character avoids some negative effect entirely.
 * It's based on susceptibility.  If susceptibility to the given damage type is
 * 100 or more, the character will not evade.  If susceptibility is 0, the
 * character is guaranteed to evade.  Between that, it curves so that if you're
 * closer to 100, the probability of evasion is very low.  You have to really
 * start getting toward 0 susceptibility before evasion becomes very
 * probable. */

bool damage_evasion(CharData *ch, CharData *attacker, ObjData *weapon, int dtype) {
    int s = 0;

    /* Ether mobs are not immune at all to blessed physical attacks. */
    if (attacker &&
        ((dtype == DAM_PIERCE || dtype == DAM_SLASH || dtype == DAM_CRUSH) && GET_COMPOSITION(ch) == COMP_ETHER)) {
        return !(EFF_FLAGGED(attacker, EFF_BLESS));
    }

    /* Alignment-type damage is never subject to evasion */
    if (attacker && weapon && OBJ_EFF_FLAGGED(weapon, EFF_RADIANT_WEAPON))
        return false;

    /* If the weapon is a physical weapon and also deals special damage, see what the best option is for evasion */
    if (attacker && weapon &&
        (OBJ_EFF_FLAGGED(weapon, EFF_FIRE_WEAPON) || OBJ_EFF_FLAGGED(weapon, EFF_ICE_WEAPON) ||
         OBJ_EFF_FLAGGED(weapon, EFF_POISON_WEAPON) || OBJ_EFF_FLAGGED(weapon, EFF_ACID_WEAPON) ||
         OBJ_EFF_FLAGGED(weapon, EFF_SHOCK_WEAPON))) {
        if (OBJ_EFF_FLAGGED(weapon, EFF_FIRE_WEAPON))
            s = std::max(susceptibility(ch, skill_to_dtype(GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT)),
                         susceptibility(ch, DAM_FIRE));
        if (OBJ_EFF_FLAGGED(weapon, EFF_ICE_WEAPON)) {
            if (s)
                s = std::max(s, susceptibility(ch, DAM_COLD));
            else
                s = std::max(susceptibility(ch, skill_to_dtype(GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT)),
                             susceptibility(ch, DAM_COLD));
        }
        if (OBJ_EFF_FLAGGED(weapon, EFF_POISON_WEAPON)) {
            if (s)
                s = std::max(s, susceptibility(ch, DAM_POISON));
            else
                s = std::max(susceptibility(ch, skill_to_dtype(GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT)),
                             susceptibility(ch, DAM_POISON));
        }
        if (OBJ_EFF_FLAGGED(weapon, EFF_ACID_WEAPON)) {
            if (s)
                s = std::max(s, susceptibility(ch, DAM_ACID));
            else
                s = std::max(susceptibility(ch, skill_to_dtype(GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT)),
                             susceptibility(ch, DAM_ACID));
        }
        if (OBJ_EFF_FLAGGED(weapon, EFF_SHOCK_WEAPON)) {
            if (s)
                s = std::max(s, susceptibility(ch, DAM_SHOCK));
            else
                s = std::max(susceptibility(ch, skill_to_dtype(GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT)),
                             susceptibility(ch, DAM_SHOCK));
        }
    } else
        s = susceptibility(ch, dtype);
    return random_number(1, 1000000) > 1000000 - (100 - s) * (100 - s) * (100 - s);
}

int convert_weapon_damage(ObjData *weapon) {
    if (weapon) {
        if (OBJ_EFF_FLAGGED(weapon, EFF_RADIANT_WEAPON))
            return DAM_ALIGN;
        else if (OBJ_EFF_FLAGGED(weapon, EFF_FIRE_WEAPON))
            return DAM_FIRE;
        else if (OBJ_EFF_FLAGGED(weapon, EFF_ICE_WEAPON))
            return DAM_COLD;
        else if (OBJ_EFF_FLAGGED(weapon, EFF_POISON_WEAPON))
            return DAM_POISON;
        else if (OBJ_EFF_FLAGGED(weapon, EFF_SHOCK_WEAPON))
            return DAM_SHOCK;
        else if (OBJ_EFF_FLAGGED(weapon, EFF_ACID_WEAPON))
            return DAM_ACID;
        else
            return skill_to_dtype(GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT);
    }

    return DAM_CRUSH;
}

int convert_weapon_type(ObjData *weapon) {
    if (weapon) {
        if (OBJ_EFF_FLAGGED(weapon, EFF_RADIANT_WEAPON))
            return TYPE_ALIGN;
        else if (OBJ_EFF_FLAGGED(weapon, EFF_FIRE_WEAPON))
            return TYPE_FIRE;
        else if (OBJ_EFF_FLAGGED(weapon, EFF_ICE_WEAPON))
            return TYPE_COLD;
        else if (OBJ_EFF_FLAGGED(weapon, EFF_POISON_WEAPON))
            return TYPE_POISON;
        else if (OBJ_EFF_FLAGGED(weapon, EFF_SHOCK_WEAPON))
            return TYPE_SHOCK;
        else if (OBJ_EFF_FLAGGED(weapon, EFF_ACID_WEAPON))
            return TYPE_ACID;
        else
            return (GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT);
    }

    return TYPE_HIT;
}

int skill_to_dtype(int skill) {
    switch (skill) {
    case TYPE_SHOCK:
        return DAM_SHOCK;
    case TYPE_FIRE:
        return DAM_FIRE;
    case TYPE_COLD:
        return DAM_COLD;
    case TYPE_ACID:
        return DAM_ACID;
    case TYPE_POISON:
        return DAM_POISON;
    case TYPE_ALIGN:
        return DAM_ALIGN;
    case TYPE_BITE:
    case TYPE_PIERCE:
    case TYPE_STAB:
    case TYPE_STING:
        return DAM_PIERCE;
    case TYPE_CLAW:
    case TYPE_SLASH:
    case TYPE_THRASH:
    case TYPE_WHIP:
        return DAM_SLASH;
    case TYPE_BLAST:
    case TYPE_BLUDGEON:
    case TYPE_CRUSH:
    case TYPE_HIT:
    case TYPE_MAUL:
    case TYPE_POUND:
    case TYPE_PUNCH:
    default:
        return DAM_CRUSH;
    }
}

void damage_evasion_message(CharData *ch, CharData *vict, ObjData *weapon, int dtype) {
    int damtype = dtype;

    if (weapon) {
        damtype = skill_to_dtype(GET_OBJ_VAL(weapon, VAL_WEAPON_DAM_TYPE) + TYPE_HIT);
    }

    /* Check for physical attacks against not-so-solid opponents. */
    if ((damtype == DAM_SLASH || damtype == DAM_PIERCE || damtype == DAM_CRUSH) && !RIGID(vict)) {
        if (weapon) {
            act(EVASIONCLR "Your $o passes harmlessly through $N" EVASIONCLR "!&0", false, ch, weapon, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o passes harmlessly through $N" EVASIONCLR "!&0", false, ch, weapon,
                vict, TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o passes harmlessly through you.&0", false, ch, weapon, vict, TO_VICT);
        } else {
            act(EVASIONCLR "Your fist passes harmlessly through $N" EVASIONCLR "!&0", false, ch, 0, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s fist passes harmlessly through $N" EVASIONCLR "!&0", false, ch, 0, vict,
                TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s fist passes harmlessly through you.&0", false, ch, 0, vict, TO_VICT);
        }
        return;
    }

    /* For the physical attacks (slash, pierce, crush), the victim is known
     * to be rigid at this point. */
    switch (damtype) {
    case DAM_SLASH:
        if (weapon) {
            act(EVASIONCLR "Your $o slides harmlessly off $N" EVASIONCLR "!&0", false, ch, weapon, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o slides harmlessly off $N" EVASIONCLR "!&0", false, ch, weapon, vict,
                TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o slides harmlessly off you.&0", false, ch, weapon, vict, TO_VICT);
        } else {
            act(EVASIONCLR "Your blade slides harmlessly off $N" EVASIONCLR "!&0", false, ch, 0, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s blade slides harmlessly off $N" EVASIONCLR "!&0", false, ch, 0, vict,
                TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s blade slides harmlessly off you.&0", false, ch, 0, vict, TO_VICT);
        }
        break;
    case DAM_PIERCE:
        if (weapon) {
            act(EVASIONCLR "Your $o fails to pierce $N" EVASIONCLR " at all!&0", false, ch, weapon, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o fails to pierce $N" EVASIONCLR " at all!&0", false, ch, weapon, vict,
                TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o fails to pierce you at all.&0", false, ch, weapon, vict, TO_VICT);
        } else {
            act(EVASIONCLR "You fail to pierce $N" EVASIONCLR " at all!&0", false, ch, 0, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR " fails to pierce $N" EVASIONCLR " at all!&0", false, ch, 0, vict,
                TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR " fails to pierce you at all.&0", false, ch, 0, vict, TO_VICT);
        }
        break;
    case DAM_CRUSH:
        if (weapon) {
            act(EVASIONCLR "Your $o bounces harmlessly off $N" EVASIONCLR "!&0", false, ch, weapon, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o bounces harmlessly off $N" EVASIONCLR "!&0", false, ch, weapon, vict,
                TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s $o bounces harmlessly off you.&0", false, ch, weapon, vict, TO_VICT);
        } else {
            act(EVASIONCLR "Your fist bounces harmlessly off $N" EVASIONCLR "!&0", false, ch, 0, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR "'s fist bounces harmlessly off $N" EVASIONCLR "!&0", false, ch, 0, vict,
                TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR "'s fist bounces harmlessly off you.&0", false, ch, 0, vict, TO_VICT);
        }
        break;
    case DAM_POISON:
        sprintf(buf, EVASIONCLR "$n" EVASIONCLR " tries to %s $N" EVASIONCLR ", but $E is immune!&0",
                damtypes[damtype].verb1st);
        act(buf, false, ch, 0, vict, TO_NOTVICT);
        sprintf(buf, EVASIONCLR "You try to %s $N" EVASIONCLR ", but $E is immune!&0", damtypes[damtype].verb1st);
        act(buf, false, ch, 0, vict, TO_CHAR);
        sprintf(buf, EVASIONCLR "$n" EVASIONCLR " tries to %s you, but you are immune!&0", damtypes[damtype].verb1st);
        act(buf, false, ch, 0, vict, TO_VICT);
        break;
    case DAM_SHOCK:
    case DAM_FIRE:
    case DAM_WATER:
    case DAM_COLD:
    case DAM_ACID:
    case DAM_HEAL:
    case DAM_ALIGN:
    case DAM_DISPEL:
    case DAM_DISCORPORATE:
    default:
        if (random_number(1, 3) == 1)
            sprintf(buf, EVASIONCLR "$n" EVASIONCLR " tries to %s $N" EVASIONCLR ", but $E is completely unaffected!&0",
                    damtypes[damtype].verb1st);
        else
            sprintf(buf, EVASIONCLR "$n" EVASIONCLR "'s %s has no effect on $N" EVASIONCLR "!",
                    damtypes[damtype].action);
        act(buf, false, ch, 0, vict, TO_NOTVICT);
        if (random_number(1, 3) == 1)
            sprintf(buf, EVASIONCLR "You try to %s $N" EVASIONCLR ", but $E is completely unaffected!&0",
                    damtypes[damtype].verb1st);
        else
            sprintf(buf, EVASIONCLR "Your %s has no effect on $N" EVASIONCLR "!&0", damtypes[damtype].action);
        act(buf, false, ch, 0, vict, TO_CHAR);
        if (random_number(1, 3) == 1)
            sprintf(buf, EVASIONCLR "$n" EVASIONCLR " tries to %s you, but you are completely unaffected!&0",
                    damtypes[damtype].verb1st);
        else
            sprintf(buf, EVASIONCLR "$n" EVASIONCLR "'s %s has no effect on you!&0", damtypes[damtype].action);
        act(buf, false, ch, 0, vict, TO_VICT);
        break;
    }
}

int weapon_damtype(ObjData *obj) {
    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        return skill_to_dtype(TYPE_HIT + GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE));
    return DAM_CRUSH;
}

int physical_damtype(CharData *ch) {
    if (equipped_weapon(ch))
        return weapon_damtype(equipped_weapon(ch));
    else
        return COMPOSITION_DAM(ch);
}