/***************************************************************************
 *  File: bitflags.hpp                                    Part of FieryMUD *
 *  Usage: for bitflags and such                                           *
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
#include "utils.hpp"

#include <string>
#include <string_view>

[[nodiscard]] std::string sprintbit(long bitvector, const std::string_view names[]);
[[nodiscard]] std::string sprinttype(int type, const std::string_view names[]);
[[nodiscard]] std::string sprintflag(flagvector flags[], int num_flags, const std::string_view names[]);
[[nodiscard]] std::string sprintascii(flagvector bits);

template <std::size_t N> [[nodiscard]] std::string sprinttype(int type, const std::array<std::string_view, N> &names) {
    return std::string{names[type]};
}

template <std::size_t N>
[[nodiscard]] std::string sprintflag(flagvector flags[], const std::array<std::string_view, N> &names) {
    std::string result;

    for (int i = 0; i < names.size(); ++i) {
        if (IS_FLAGGED(flags, i)) {
            result += names[i];
            result += ' ';
        }
    }

    if (result.empty())
        result = "NO FLAGS";
    else
        result.pop_back(); /* Remove the trailing space */

    return result;
}