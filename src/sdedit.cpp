/***************************************************************************
 *  File: sdedit.c                                        Part of FieryMUD *
 *  Usage: Edits an array that is the size of the spell list               *
 *         (spell_dam_info). This array is then used in the magic.c system *
 *         that affects damages etc etc.                                   *
 *                                                                         *
 *  By: Proky of HubisMUD                                                  *
 *  Control M's removed by: Scott Davis                                    *
 ***************************************************************************/

/***************************************************************************
 *  For Hubis, By Proky.
 * This is pretty simple it just edits a array that is the size of the spell
 * list. (spell_dam_info).  This array is then used in the magic.c system
 * that affects damages etc etc.
 * HERE IS A COPY OF THE STRUCTURE (in structs.h)
 * struct spell_dam {
 *    sh_int spell;
 *    bool intnern_dam;
 *    sh_int npc_no_dice;
 *    sh_int npc_no_face;
 *    sh_int pc_no_dice;
 *    sh_int pc_no_face;
 *    sh_int npc_reduce_factor;
 *    bool use_bonus;
 *    sh_int max_bonus;
 *    sh_int npc_static;
 *    sh_int pc_static;
 *    sh_int lvl_mult;
 *   };
 * NOTES:
 * - Rewrites the file evry time save occures.
 * - viewdam is the word to view the list online.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *This is used to edit the online damages etc from spells.
 *this array will be used in magic.c to decide the damages of many spells
 */
#include "casting.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "olc.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* function protos */
void sdedit_disp_menu(DescriptorData *d);
void sdedit_parse(DescriptorData *d, std::string_view arg);
void sdedit_setup_existing(DescriptorData *d, int real_num);
void sdedit_save_to_disk(DescriptorData *d);
void sdedit_save_internally(DescriptorData *d);

/*------------------------------------------------------------------------*/

void sdedit_setup_existing(DescriptorData *d, int real_num) {
    SpellDamage *spell;

    if (GET_LEVEL(d->character) < LVL_HEAD_B)
        return;
    CREATE(spell, SpellDamage, 1);
    *spell = spell_dam_info[real_num];
    OLC_SD(d) = spell;
    OLC_VAL(d) = 0;
    if (spell->note)
        OLC_SD(d)->note = strdup(spell->note); /*duplicate the dynamic part */
    sdedit_disp_menu(d);
}

void sdedit_disp_menu(DescriptorData *d) {
    char cbuf2[600];
    SpellDamage *spell;
    const std::string_view nrm = CLR(d->character, ANRM);
    const std::string_view yel = CLR(d->character, FYEL);
    const std::string_view grn = CLR(d->character, FGRN);
    const std::string_view red = CLR(d->character, FRED);
    spell = OLC_SD(d);

    sprintf(buf,
            "Editing Spell: %s%-21s%s %s%d%s\n\n"
            "         %sDAMAGES TO NPC:%s\n\n"
            "%s1%s) Number of dice to be used:        %s%d%s\n"
            "%s2%s) Number on the face of the dice:   %s%d%s\n"
            "%sR%s) %% dmg to NPCs 100 full effect :   %s%d%s\n"
            "%sA%s) NPC Static number                 %s%d%s\n"
            "         %sDAMAGES TO PC:%s\n\n"
            "%s3%s) Number of dice to be used:        %s%d%s\n"
            "%s4%s) Number on the face of the dice:   %s%d%s\n"
            "%sB%s) PC Static number:                 %s%d%s\n"
            "         %sGENERAL TO BOTH:%s\n\n",
            grn, (skills[spell->spell].name), nrm, grn, spell->spell, nrm, red, nrm, yel, nrm, grn, spell->npc_no_dice,
            grn,
            /*2 */ yel, nrm, grn, spell->npc_no_face, nrm,
            /*R*/ yel, nrm, grn, spell->npc_reduce_factor, nrm, /*A*/ yel, nrm, grn, spell->npc_static, nrm, red, nrm,
            /*3 */ yel, nrm, grn, spell->pc_no_dice, nrm,
            /*4 */ yel, nrm, grn, spell->pc_no_face, nrm,
            /*B*/ yel, nrm, grn, spell->pc_static, nrm, red, nrm);

    sprintf(cbuf2,
            "%s5%s) Max Bonus possible:               %s%d%s\n"
            "%sT%s) Toggle Wether to use Max Bonus:   %s%s%s\n"
            "%sI%s) Toggle Internal Damage On/Off:    %s%s%s\n"
            "%sE%s) LVL Multiplie		%s%d%s\n"
            "%sN%s) Edit Spell Note.\n"
            "%sQ%s) Quit\n"
            "\n"
            "%sUSED BY PC to NPC%s: - %sNPC + std::min(Max_bonus, level/2) + "
            "NPC_STATIC%s\n"
            "%sUSED BY NPC to PC%s: - %sNPC + std::min(Max_bonus, level/2) + "
            "NPC_STATIC%s\n"
            "%sUSED BY PC to PC %s: - %sPC  + std::min(Max_bonus, level/4) + "
            "PC_STATIC%s\n\n"
            "%sSpell Note:%s\n"
            "%s%s%s\n"
            "\n"
            "Enter choice:\n",
            /*5 */ yel, nrm, grn, spell->max_bonus, nrm,
            /*Bonus */ yel, nrm, grn, (spell->use_bonus ? "true" : "false"), nrm,
            /*Intern*/ yel, nrm, grn, (spell->intern_dam ? "true" : "false"), nrm,
            /*E*/ yel, nrm, grn, spell->lvl_mult, nrm, yel, nrm, yel, nrm, yel, nrm, grn, nrm, yel, nrm, grn, nrm, yel,
            nrm, grn, nrm, red, nrm, grn, (spell->note ? spell->note : ""), nrm);
    char_printf(d->character, buf);
    char_printf(d->character, cbuf2);

    OLC_MODE(d) = SDEDIT_MAIN_MENU;
}

void sdedit_save_internally(DescriptorData *d) {

    SpellDamage *spell = nullptr;
    spell = OLC_SD(d);

    /*free old note first!! */

    if (spell_dam_info[OLC_SD(d)->spell].note)
        free(spell_dam_info[OLC_SD(d)->spell].note);
    spell_dam_info[OLC_SD(d)->spell] = *spell;

    /*copy over new sring */

    if (spell->note)
        spell_dam_info[OLC_SD(d)->spell].note = strdup(spell->note);
}

/*------------------------------------------------------------------------*/

void sdedit_save_to_disk(DescriptorData *d) {
    sh_int i;
    FILE *ifptr;

    if ((ifptr = fopen(SPELL_DAM_FILE, "w")) == nullptr) {
        log(LogSeverity::Stat, LVL_IMPL, "Error writting spell dam file\n");
    } else {

        fprintf(ifptr, "Spell dam file:\n");
        fprintf(ifptr,
                "spell, intern, npc_static, npc_no_dice, npc_no_face, "
                "pc_static, pc_no_face, "
                "pc_no_dice, npc_reduce, use_bonus, bonus, lvl_mult\n");
        fprintf(ifptr, "spell_dam\n");

        for (i = 1; i <= MAX_SPELLS; i++) {
            fprintf(ifptr, "%hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd\n", i, SD_INTERN_DAM(i), SD_NPC_STATIC(i),
                    SD_NPC_NO_DICE(i), SD_NPC_NO_FACE(i), SD_PC_STATIC(i), SD_PC_NO_DICE(i), SD_PC_NO_FACE(i),
                    SD_NPC_REDUCE_FACTOR(i), SD_USE_BONUS(i), SD_BONUS(i), SD_LVL_MULT(i));
            fprintf(ifptr, "%s~\n", (SD_NOTE(i) ? SD_NOTE(i) : ""));
        }
        fclose(ifptr);
    }
}

void sdedit_parse(DescriptorData *d, std::string_view arg) {
    int i = 0;

    switch (OLC_MODE(d)) {
    case SDEDIT_MAIN_MENU:
        switch (*arg) {
        case 'q':
        case 'Q':
            if (OLC_VAL(d)) {
                char_printf(d->character, "Do you wish to save this?\n");
                OLC_MODE(d) = SDEDIT_CONFIRM_SAVESTRING;
            } else
                cleanup_olc(d, CLEANUP_ALL);
            return;
        case '1':
            i++;
            OLC_MODE(d) = SDEDIT_NPC_NO_DICE_ENTRY;
            break;
        case '2':
            i++;
            OLC_MODE(d) = SDEDIT_NPC_NO_FACE_ENTRY;
            break;
        case '3':
            i++;
            OLC_MODE(d) = SDEDIT_PC_NO_DICE_ENTRY;
            break;
        case '4':
            i++;
            OLC_MODE(d) = SDEDIT_PC_NO_FACE_ENTRY;
            break;
        case '5':
            i++;
            OLC_MODE(d) = SDEDIT_MAX_BONUS;
            break;
        case 'a':
        case 'A':
            i++;
            OLC_MODE(d) = SDEDIT_NPC_STATIC;
            break;
        case 'b':
        case 'B':
            i++;
            OLC_MODE(d) = SDEDIT_PC_STATIC;
            break;
        case 'e':
        case 'E':
            i++;
            OLC_MODE(d) = SDEDIT_LVL_MULT_MENU;
            break;
        case 'n':
        case 'N':
            i--;
            OLC_MODE(d) = SDEDIT_NOTE;
            break;
        case 'i':
        case 'I':
            if (OLC_SD(d)->intern_dam)
                OLC_SD(d)->intern_dam = false;
            else
                OLC_SD(d)->intern_dam = true;
            OLC_MODE(d) = SDEDIT_MAIN_MENU;
            OLC_VAL(d) = 1;
            sdedit_disp_menu(d);
            return;
        case 'r':
        case 'R':
            i++;
            OLC_MODE(d) = SDEDIT_NPC_REDUCE_FACTOR;
            break;
        case 't':
        case 'T':
            /*switch bonus on and off SDEDIT_USE_BONUS */
            if (OLC_SD(d)->use_bonus)
                OLC_SD(d)->use_bonus = false;
            else
                OLC_SD(d)->use_bonus = true;
            OLC_MODE(d) = SDEDIT_MAIN_MENU;
            OLC_VAL(d) = 1;
            sdedit_disp_menu(d);
            return;
        default:
            char_printf(d->character, "Not a option sorry\n");
            sdedit_disp_menu(d);
            return;
        }
        if (i > 0) {
            char_printf(d->character, "Enter new value\n>");
            return;
        } else if (i < 0) {
            char_printf(d->character, "Enter Note Max 60 letters\n]");
            return;
        }
        return;

    case SDEDIT_NPC_NO_DICE_ENTRY:
        OLC_SD(d)->npc_no_dice = atoi(arg);
        break;

    case SDEDIT_NPC_NO_FACE_ENTRY:
        OLC_SD(d)->npc_no_face = atoi(arg);
        break;

    case SDEDIT_PC_NO_DICE_ENTRY:
        OLC_SD(d)->pc_no_dice = atoi(arg);
        break;

    case SDEDIT_LVL_MULT_MENU:
        OLC_SD(d)->lvl_mult = atoi(arg);
        break;

    case SDEDIT_NOTE:
        if (OLC_SD(d)->note)
            free(OLC_SD(d)->note);
        if (!arg.empty())
            OLC_SD(d)->note = strdup(arg);
        else
            OLC_SD(d)->note = nullptr;
        break;

    case SDEDIT_PC_STATIC:
        OLC_SD(d)->pc_static = atoi(arg);
        break;

    case SDEDIT_NPC_STATIC:
        OLC_SD(d)->npc_static = atoi(arg);
        break;

    case SDEDIT_PC_NO_FACE_ENTRY:
        OLC_SD(d)->pc_no_face = atoi(arg);
        break;

    case SDEDIT_NPC_REDUCE_FACTOR:
        OLC_SD(d)->npc_reduce_factor = atoi(arg);
        break;

    case SDEDIT_MAX_BONUS:
        OLC_SD(d)->max_bonus = atoi(arg);
        break;

        return;
    case SDEDIT_CONFIRM_SAVESTRING:
        switch (*arg) {
        case 'y':
        case 'Y':
            sdedit_save_internally(d);
            sdedit_save_to_disk(d);
            log(LogSeverity::Debug, LVL_IMPL, "OLC: {} edits spell damage.", GET_NAME(d->character));
            /* do not free the strings.. just the structure */
            cleanup_olc(d, CLEANUP_ALL);
            /*cleanup_olc(d, CLEANUP_STRUCTS); */
            char_printf(d->character, "Your spell dam changes have been saved to memory.\n");
            break;
        case 'n':
        case 'N':
            /*free everything up, including strings etc */
            char_printf(d->character, "Changes to spell damage not saved\n");
            cleanup_olc(d, CLEANUP_ALL);
            break;
        default:
            char_printf(d->character, "Invalid choice!\nDo you wish to save this help internally?\n");
            break;
        }
        return;
    }
    OLC_VAL(d) = 1;
    sdedit_disp_menu(d);
}
