#
# FieryMUD Makefile
# Generated automatically from Makefile.in by configure.
# CircleMUD 3.0 Makefile.in - Makefile template used by 'configure'
#

# C compiler to use
CC = gcc

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall -pedantic -m32
#flags for profiling (see hacker.doc for more information)
PROFILE = 
# any flags for the linker
LFLAGS = -m32
# Test Flags
TFLAGS = -DTEST
# Production Flags
PFLAGS = -DPRODUCTION
# Dev Flags
DFLAGS = -DDEV
# Memtest Flags
MFLAGS = -DMEMORY_DEBUG
# Libraries
LDLIBS = -lm -lcrypt

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = ../bin

LOGDIR = ../log

#VPATH = .:rules

# Unfortunately, -I- is deprecated.  It's "replacement" is -iquote,
# but the functionality is subtly different.
#INC_FLAGS = -I. -Irules -I-

#INC_FLAGS = -iquote . -iquote rules

COMP_FLAGS = -ggdb3 $(MYFLAGS) $(PROFILE) 

CFLAGS = $(COMP_FLAGS) $(INC_FLAGS)

OBJFILES = comm.o act.comm.o act.informative.o act.movement.o act.item.o \
	act.offensive.o act.other.o act.social.o act.wizard.o ban.o \
	class.o config.o constants.o db.o events.o fight.o graph.o handler.o \
	house.o interpreter.o limits.o magic.o mail.o mobact.o modify.o \
	pfiles.o olc.o races.o shop.o spec_assign.o spec_procs.o \
	spell_parser.o casting.o utils.o weather.o queue.o regen.o hedit.o redit.o \
	oedit.o zedit.o medit.o sedit.o sdedit.o players.o \
	spell_mem.o dg_comm.o dg_db_scripts.o dg_debug.o dg_handler.o \
	dg_mobcmd.o dg_objcmd.o dg_scripts.o dg_triggers.o dg_wldcmd.o \
	dg_olc.o ispell.o corpse_save.o clansys.o clan.o ai_utils.o \
	sorcerer.o cleric.o warrior.o rogue.o quest.o vsearch.o \
	diskio.o chars.o skills.o math.o zmalloc.o trophy.o screen.o \
	act.wizinfo.o objects.o genzon.o exits.o rooms.o commands.o \
	movement.o editor.o board.o find.o composition.o lifeforce.o \
	charsize.o damage.o strings.o spells.o directions.o \
	effects.o money.o spheres.o act.get.o textfiles.o \
	rules.o rulesys.o text.o privileges.o cooldowns.o prefs.o \
	retain_comms.o

default: test

test:
	@$(BINDIR)/test_make_info; \
	$(MAKE) $(BINDIR)/fiery CFLAGS="$(CFLAGS) $(TFLAGS)";
	# $(BINDIR)/buildcount $(LOGDIR)/make_count

production:
	@$(BINDIR)/production_make_info; \
	$(MAKE) $(BINDIR)/fiery CFLAGS="$(CFLAGS) $(PFLAGS)";
	# $(BINDIR)/buildcount $(LOGDIR)/make_count

dev:
	@$(BINDIR)/dev_make_info; \
	$(MAKE) $(BINDIR)/fiery CFLAGS="$(CFLAGS) $(DFLAGS)";
	# $(BINDIR)/buildcount $(LOGDIR)/make_count

memtest:
	@$(BINDIR)/dev_make_info; \
	$(MAKE) $(BINDIR)/fiery CFLAGS="$(CFLAGS) $(MFLAGS)";
	# $(BINDIR)/buildcount $(LOGDIR)/make_count

utils:
	$(MAKE) $(BINDIR)/delobjs
	$(MAKE) $(BINDIR)/listrent
	$(MAKE) $(BINDIR)/mudpasswd
	$(MAKE) $(BINDIR)/showplay
	$(MAKE) $(BINDIR)/sign
	$(MAKE) $(BINDIR)/split
	$(MAKE) $(BINDIR)/wld2html
	$(MAKE) $(BINDIR)/portscan
	$(MAKE) $(BINDIR)/buildcount
	$(MAKE) $(BINDIR)/plrtoascii

all:
	$(MAKE) test
	$(MAKE) utils

delobjs:
	$(MAKE) $(BINDIR)/delobjs
listrent:
	$(MAKE) $(BINDIR)/listrent
mudpasswd:
	$(MAKE) $(BINDIR)/mudpasswd
showplay:
	$(MAKE) $(BINDIR)/showplay
sign:
	$(MAKE) $(BINDIR)/sign
split:
	$(MAKE) $(BINDIR)/split
wld2html:
	$(MAKE) $(BINDIR)/wld2html
portscan:
	$(MAKE) $(BINDIR)/portscan
buildcount:
	$(MAKE) $(BINDIR)/buildcount
plrtoascii:
	$(MAKE) $(BINDIR)/plrtoascii

$(BINDIR)/delobjs: util/delobjs.c conf.h sysdep.h structs.h
	$(CC) $(CFLAGS) -o $(BINDIR)/delobjs util/delobjs.c $(LFLAGS)
$(BINDIR)/listrent: util/listrent.c conf.h sysdep.h structs.h
	$(CC) $(CFLAGS) -o $(BINDIR)/listrent util/listrent.c $(LFLAGS)
$(BINDIR)/mudpasswd: util/mudpasswd.c conf.h sysdep.h structs.h utils.h
	$(CC) $(CFLAGS) -o $(BINDIR)/mudpasswd util/mudpasswd.c -lcrypt $(LFLAGS)  
$(BINDIR)/showplay: util/showplay.c conf.h sysdep.h structs.h
	$(CC) $(CFLAGS) -o $(BINDIR)/showplay util/showplay.c $(LFLAGS)
$(BINDIR)/sign: util/sign.c conf.h sysdep.h
	$(CC) $(CFLAGS) -o $(BINDIR)/sign util/sign.c -lcrypt $(LFLAGS)
$(BINDIR)/split: util/split.c
	$(CC) $(CFLAGS) -o $(BINDIR)/split util/split.c $(LFLAGS)
$(BINDIR)/wld2html: util/wld2html.c
	$(CC) $(CFLAGS) -o $(BINDIR)/wld2html util/wld2html.c $(LFLAGS)
$(BINDIR)/portscan: util/portscan.c
	$(CC) $(CFLAGS) -o $(BINDIR)/portscan util/portscan.c $(LFLAGS)
$(BINDIR)/buildcount: util/buildcount.c
	$(CC) $(CFLAGS) -o $(BINDIR)/buildcount util/buildcount.c $(LFLAGS)
$(BINDIR)/plrtoascii: util/plrtoascii.c structs.h interpreter.h \
	utils.h db.h players.h conf.h sysdep.h legacy_structs.h \
        exits.h rooms.h
	$(CC) $(CFLAGS) -o $(BINDIR)/plrtoascii util/plrtoascii.c $(LFLAGS)
$(BINDIR)/fiery: $(OBJFILES)
	$(CC) -o $(BINDIR)/fiery $(PROFILE) $(OBJFILES) $(LDLIBS) $(LFLAGS)

$(OBJFILES):
	@echo $(CC) $(COMP_FLAGS) -c -o $@ $<
	@$(CC) $(CFLAGS) -c -o $@ $<


mine:
	chmod u+w *.c
	chmod u+w *.h

clean:
	rm -f *.o

supahclean: clean
	rm -f *.c *.h Makefile.d

depend:
	$(CC) -MM *.c > Makefile.d

# Dependencies for the object files
# (automagically generated with gcc -MM *.c)
-include Makefile.d
-include */Makefile.d
