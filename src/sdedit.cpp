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

#include <algorithm>  // for std::min
#include <fmt/core.h> // For fmt::format
#include <fstream>
#include <string>

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
    OLC_SD(d)->note = spell->note;
    sdedit_disp_menu(d);
}

void sdedit_disp_menu(DescriptorData *d) {
    std::string menu_text;
    std::string details_text;
    SpellDamage *spell;
    spell = OLC_SD(d);

    menu_text = fmt::format(
        "Editing Spell: {0}{1}{2} {3}{4}{5}\n\n"
        "         {6}DAMAGES TO NPC:{7}\n\n"
        "{8}1{9}) Number of dice to be used:        {10}{11}{12}\n"
        "{13}2{14}) Number on the face of the dice:   {15}{16}{17}\n"
        "{18}R{19}) % dmg to NPCs 100 full effect :   {20}{21}{22}\n"
        "{23}A{24}) NPC Static number                 {25}{26}{27}\n"
        "         {28}DAMAGES TO PC:{29}\n\n"
        "{30}3{31}) Number of dice to be used:        {32}{33}{34}\n"
        "{35}4{36}) Number on the face of the dice:   {37}{38}{39}\n"
        "{40}B{41}) PC Static number:                 {42}{43}{44}\n"
        "         {45}GENERAL TO BOTH:{46}\n\n",
        grn, skills[spell->spell].name, nrm, grn, spell->spell, nrm, red, nrm, yel, nrm, grn, spell->npc_no_dice, nrm,
        /*2*/ yel, nrm, grn, spell->npc_no_face, nrm,
        /*R*/ yel, nrm, grn, spell->npc_reduce_factor, nrm,
        /*A*/ yel, nrm, grn, spell->npc_static, nrm, red, nrm,
        /*3*/ yel, nrm, grn, spell->pc_no_dice, nrm,
        /*4*/ yel, nrm, grn, spell->pc_no_face, nrm,
        /*B*/ yel, nrm, grn, spell->pc_static, nrm, red, nrm);

    details_text = fmt::format(
        "{0}5{1}) Max Bonus possible:               {2}{3}{4}\n"
        "{5}T{6}) Toggle Wether to use Max Bonus:   {7}{8}{9}\n"
        "{10}I{11}) Toggle Internal Damage On/Off:    {12}{13}{14}\n"
        "{15}E{16}) LVL Multiplie\t\t{17}{18}{19}\n"
        "{20}N{21}) Edit Spell Note.\n"
        "{22}Q{23}) Quit\n"
        "\n"
        "{24}USED BY PC to NPC{25}: - {26}NPC + std::min(Max_bonus, level/2) + NPC_STATIC{27}\n"
        "{28}USED BY NPC to PC{29}: - {30}NPC + std::min(Max_bonus, level/2) + NPC_STATIC{31}\n"
        "{32}USED BY PC to PC {33}: - {34}PC  + std::min(Max_bonus, level/4) + PC_STATIC{35}\n\n"
        "{36}Spell Note:{37}\n"
        "{38}{39}{40}\n"
        "\n"
        "Enter choice:\n",
        /*5*/ yel, nrm, grn, spell->max_bonus, nrm,
        /*Bonus*/ yel, nrm, grn, (spell->use_bonus ? "true" : "false"), nrm,
        /*Intern*/ yel, nrm, grn, (spell->intern_dam ? "true" : "false"), nrm,
        /*E*/ yel, nrm, grn, spell->lvl_mult, nrm, yel, nrm, yel, nrm, yel, nrm, grn, nrm, yel, nrm, grn, nrm, yel, nrm,
        grn, nrm, red, nrm, grn, spell->note, nrm);

    char_printf(d->character, menu_text);
    char_printf(d->character, details_text);

    OLC_MODE(d) = SDEDIT_MAIN_MENU;
}

void sdedit_save_internally(DescriptorData *d) {
    SpellDamage *spell = nullptr;
    spell = OLC_SD(d);

    spell_dam_info[OLC_SD(d)->spell] = *spell;
    spell_dam_info[OLC_SD(d)->spell].note = spell->note;
}

void sdedit_save_to_disk(DescriptorData *d) {
    sh_int i;
    std::ofstream outfile(SPELL_DAM_FILE);

    if (!outfile) {
        log(LogSeverity::Stat, LVL_IMPL, "Error writing spell damage file");
    } else {
        outfile << "Spell dam file:\n";
        outfile << "spell, intern, npc_static, npc_no_dice, npc_no_face, "
                << "pc_static, pc_no_face, pc_no_dice, npc_reduce, use_bonus, bonus, lvl_mult\n";
        outfile << "spell_dam\n";

        for (i = 1; i <= MAX_SPELLS; i++) {
            outfile << i << " " << SD_INTERN_DAM(i) << " " << SD_NPC_STATIC(i) << " " << SD_NPC_NO_DICE(i) << " "
                    << SD_NPC_NO_FACE(i) << " " << SD_PC_STATIC(i) << " " << SD_PC_NO_DICE(i) << " " << SD_PC_NO_FACE(i)
                    << " " << SD_NPC_REDUCE_FACTOR(i) << " " << SD_USE_BONUS(i) << " " << SD_BONUS(i) << " "
                    << SD_LVL_MULT(i) << "\n";
            outfile << SD_NOTE(i) << "~\n";
        }
        outfile.close();
    }
}

void sdedit_parse(DescriptorData *d, std::string_view arg) {
    int i = 0;

    switch (OLC_MODE(d)) {
    case SDEDIT_MAIN_MENU:
        switch (arg.front()) {
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
        OLC_SD(d)->npc_no_dice = atoi(arg.data());
        break;

    case SDEDIT_NPC_NO_FACE_ENTRY:
        OLC_SD(d)->npc_no_face = atoi(arg.data());
        break;

    case SDEDIT_PC_NO_DICE_ENTRY:
        OLC_SD(d)->pc_no_dice = atoi(arg.data());
        break;

    case SDEDIT_LVL_MULT_MENU:
        OLC_SD(d)->lvl_mult = atoi(arg.data());
        break;

    case SDEDIT_NOTE:
        OLC_SD(d)->note = arg.data();

    case SDEDIT_PC_STATIC:
        OLC_SD(d)->pc_static = atoi(arg.data());
        break;

    case SDEDIT_NPC_STATIC:
        OLC_SD(d)->npc_static = atoi(arg.data());
        break;

    case SDEDIT_PC_NO_FACE_ENTRY:
        OLC_SD(d)->pc_no_face = atoi(arg.data());
        break;

    case SDEDIT_NPC_REDUCE_FACTOR:
        OLC_SD(d)->npc_reduce_factor = atoi(arg.data());
        break;

    case SDEDIT_MAX_BONUS:
        OLC_SD(d)->max_bonus = atoi(arg.data());
        break;

        return;
    case SDEDIT_CONFIRM_SAVESTRING:
        switch (arg.front()) {
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
