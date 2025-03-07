/***************************************************************************
 *  File: objects.h                                       Part of FieryMUD *
 *  Usage: header file for objects                                         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "structs.hpp"

void oedit_disp_container_flags_menu(DescriptorData *d);
void oedit_disp_extradesc_menu(DescriptorData *d);
void oedit_disp_weapon_menu(DescriptorData *d);
void oedit_disp_val1_menu(DescriptorData *d);
void oedit_disp_val2_menu(DescriptorData *d);
void oedit_disp_val3_menu(DescriptorData *d);
void oedit_disp_val4_menu(DescriptorData *d);
void oedit_disp_type_menu(DescriptorData *d);
void oedit_disp_extra_menu(DescriptorData *d);
void oedit_disp_wear_menu(DescriptorData *d);
void oedit_disp_menu(DescriptorData *d);

void oedit_parse(DescriptorData *d, std::string_view arg);
void oedit_disp_spells_menu(DescriptorData *d);
void oedit_liquid_type(DescriptorData *d);
void oedit_setup_new(DescriptorData *d);
void oedit_setup_existing(DescriptorData *d, int real_num);
void oedit_save_to_disk(int zone);
void oedit_reverse_exdescs(int zone, CharData *ch);
int oedit_reverse_exdesc(int real_num, CharData *ch);
void oedit_save_internally(DescriptorData *d);
void iedit_save_changes(DescriptorData *d);

template <typename T, size_t N>
void oedit_disp_portal_messages_menu(DescriptorData *d, const std::array<T, N> messages) {
    int i = 0;

    for (const auto &message : messages) {
        char_printf(d->character, "{}{}{}) {}", grn, i, nrm, messages[i]);
        ++i;
    }
}