cmake_minimum_required(VERSION 3.15)
project(fierymud C)
set(CMAKE_VERBOSE_MAKEFILE ON)

# gcc *.c -lm -m32 -lcrypt -ggdb3 -Wall -pedantic -fpack-struct=4 -o ../bin/test

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "-m32 -ggdb3 -O0 -Wall -Wextra -fpack-struct=4")

#include_directories(src)

add_executable(fierymud
        src/act.comm.c
        src/act.get.c
        src/act.informative.c
        src/act.item.c
        src/act.movement.c
        src/act.offensive.c
        src/act.other.c
        src/act.social.c
        src/act.wizard.c
        src/act.wizinfo.c
        src/ai_utils.c
        src/ban.c
        src/board.c
        src/casting.c
        src/chars.c
        src/charsize.c
        src/clan.c
        src/clansys.c
        src/class.c
        src/cleric.c
        src/comm.c
        src/commands.c
        src/composition.c
        src/config.c
        src/constants.c
        src/cooldowns.c
        src/corpse_save.c
        src/damage.c
        src/db.c
        src/dg_comm.c
        src/dg_db_scripts.c
        src/dg_debug.c
        src/dg_handler.c
        src/dg_mobcmd.c
        src/dg_objcmd.c
        src/dg_olc.c
        src/dg_scripts.c
        src/dg_triggers.c
        src/dg_wldcmd.c
        src/directions.c
        src/diskio.c
        src/editor.c
        src/effects.c
        src/events.c
        src/exits.c
        src/fight.c
        src/find.c
        src/genzon.c
        src/graph.c
        src/handler.c
        src/hedit.c
        src/house.c
        src/interpreter.c
        src/ispell.c
        src/lifeforce.c
        src/limits.c
        src/magic.c
        src/mail.c
        src/math.c
        src/medit.c
        src/mobact.c
        src/modify.c
        src/money.c
        src/movement.c
        src/objects.c
        src/oedit.c
        src/olc.c
        src/pfiles.c
        src/players.c
        src/prefs.c
        src/privileges.c
        src/quest.c
        src/queue.c
        src/races.c
        src/redit.c
        src/regen.c
        src/retain_comms.c
        src/rogue.c
        src/rooms.c
        src/rules.c
        src/rulesys.c
        src/screen.c
        src/sdedit.c
        src/sedit.c
        src/shop.c
        src/skills.c
        src/sorcerer.c
        src/spec_assign.c
        src/spec_procs.c
        src/spell_mem.c
        src/spell_parser.c
        src/spells.c
        src/spheres.c
        src/strings.c
        src/text.c
        src/textfiles.c
        src/trophy.c
        src/utils.c
        src/vsearch.c
        src/warrior.c
        src/weather.c
        src/zedit.c
        src/zmalloc.c)

target_link_libraries(fierymud crypt m)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")