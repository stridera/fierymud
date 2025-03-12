/***************************************************************************
 *  File: arguments.h                                     Part of FieryMUD *
 *  Usage: Allow easily parsing arguments from the user.                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "string_utils.hpp"

#include <optional>
#include <string>

class Arguments {
    std::string arg;

  public:
    Arguments(std::string_view argument) : arg(argument) {}

    // std::string copy constructor
    Arguments(const std::string &argument) : arg(argument) {}

    // Get the current argument list.
    [[nodiscard]] std::string_view get() const { return trim(arg); }

    [[nodiscard]] bool empty() const { return trim(arg).empty(); }

    // When parsing commands, we want to allow things like ";hi" or ".gossip" to allow aliases for say/gossip, etc.
    // This command will return the single character if it's not alpha, and shift the argument list.
    // If strict is true, it will only return the single character if it's not alpha.  Otherwise, it will
    // return the first word or quoted string like shift().
    [[nodiscard]] std::string_view command_shift(bool strict = false);

    // Shift out one argument from the argument list.  This will return the first word or quoted string
    // and remove it from the argument list.
    [[nodiscard]] std::string_view shift();

    // Shift out one argument from the argument list, and replace any "$$" with "$".
    // This should follow the same rules as remove_double_dollars.
    [[nodiscard]] std::string_view shift_clean();

    // Try to shift out a number from the argument list.  If the first argument is a positive int, it will be shifted
    // and returned.  Otherwise, nothing will be shifted and an empty optional will be returned.
    [[nodiscard]] std::optional<int> try_shift_number();
};