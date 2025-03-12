/***************************************************************************
 *   File: clan.c                                         Part of FieryMUD *
 *  Usage: Front-end for the clan system                                   *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#include "clan.hpp"

#include "act.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "editor.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "modify.hpp"
#include "players.hpp"
#include "screen.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
