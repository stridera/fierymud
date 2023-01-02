/***************************************************************************
 *   File: screen.c                                       Part of FieryMUD *
 *  Usage: ANSI color code functions for online color                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "screen.hpp"

#include "conf.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <map>

std::string process_colors(std::string_view str, int mode) {
    static std::map<char, const char *> rel_color_list = {
        {CREL, AREL}, {'0', ANRM}, {'1', FRED}, {'2', FGRN}, {'3', FYEL}, {'4', FBLU}, {'5', FMAG}, {'6', FCYN},
        {'7', FWHT},  {'8', AUND}, {'9', FBLK}, {'u', AUND}, {'b', ABLD}, {'d', ADAR}, {'R', BRED}, {'G', BGRN},
        {'Y', BYEL},  {'B', BBLU}, {'M', BMAG}, {'C', BCYN}, {'W', BWHT}, {'L', BBLK}, {'K', BLBK}, {'_', "\n"}};

    static std::map<char, const char *> abs_color_list = {
        {CABS, AABS}, {'0', ANRM},  {'1', AFRED}, {'2', AFGRN}, {'3', AFYEL}, {'4', AFBLU}, {'5', AFMAG},
        {'6', AFCYN}, {'7', AFWHT}, {'9', AFBLK}, {'r', AFRED}, {'g', AFGRN}, {'y', AFYEL}, {'b', AFBLU},
        {'m', AFMAG}, {'c', AFCYN}, {'w', AFWHT}, {'d', AFBLK}, {'R', AHRED}, {'G', AHGRN}, {'Y', AHYEL},
        {'B', AHBLU}, {'M', AHMAG}, {'C', AHCYN}, {'W', AHWHT}, {'L', AHBLK}};
    std::string resp;
    char code = '\0';

    for (auto c : str) {
        if (mode == CLR_PARSE) {
            if (code != '\0') {
                try {
                    if (code == CREL)
                        resp += rel_color_list.at(c);
                    else if (code == CABS)
                        resp += abs_color_list.at(c);
                } catch (std::out_of_range &e) {
                    log("SYSERR: process_colors called with unknown color code {:c}", c);
                }
                code = '\0';
                continue;
            } else if (c == CREL || c == CABS)
                code = c;
            else {
                resp += c;
            }
            continue;
        } else if (mode == CLR_STRIP) {
            if (code != '\0') {
                code = '\0';
                continue;
            }
            if (c == CREL || c == CABS) {
                code = c;
                continue;
            }
        } else if (mode == CLR_ESCAPE) {
            if (c == CREL || c == CABS) {
                resp += c;
            }
        } else {
            log("SYSERR: process_colors called with unknown mode {:d}", mode);
            return std::string(str);
        }
        resp += c;
    }

    return resp;
}

int count_color_chars(std::string_view str) noexcept {
    return std::count_if(str.begin(), str.end(), [](char c) { return c == CREL || c == CABS; });
}

int ansi_strlen(std::string_view str) {
    if (str.empty())
        return 0;

    return str.length() - count_color_chars(str);
}
std::string strip_ansi(std::string_view string) { return process_colors(string, CLR_STRIP); }
std::string escape_ansi(std::string_view string) { return process_colors(string, CLR_ESCAPE); }
