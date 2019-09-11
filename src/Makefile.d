act.comm.o: act.comm.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h interpreter.h handler.h find.h db.h \
  screen.h dg_scripts.h clan.h privileges.h math.h regen.h modify.h \
  constants.h act.h editor.h board.h
act.get.o: act.get.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h interpreter.h handler.h find.h db.h \
  dg_scripts.h skills.h class.h chars.h math.h screen.h composition.h \
  events.h act.h
act.informative.o: act.informative.c conf.h sysdep.h structs.h \
  spell_mem.h specprocs.h money.h prefs.h exits.h objects.h effects.h \
  utils.h strings.h text.h rooms.h directions.h comm.h interpreter.h \
  handler.h find.h db.h casting.h events.h screen.h weather.h races.h \
  skills.h class.h chars.h cooldowns.h constants.h math.h players.h \
  trophy.h commands.h modify.h act.h board.h composition.h lifeforce.h \
  charsize.h damage.h vsearch.h spheres.h textfiles.h
act.item.o: act.item.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h interpreter.h handler.h find.h db.h \
  dg_scripts.h races.h skills.h class.h chars.h constants.h math.h \
  pfiles.h screen.h shop.h regen.h limits.h composition.h board.h
act.movement.o: act.movement.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h constants.h comm.h interpreter.h \
  handler.h find.h db.h casting.h events.h house.h dg_scripts.h weather.h \
  races.h skills.h class.h chars.h math.h screen.h regen.h fight.h \
  magic.h modify.h act.h movement.h composition.h charsize.h board.h
act.offensive.o: act.offensive.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h comm.h interpreter.h handler.h \
  find.h db.h casting.h events.h screen.h races.h skills.h class.h \
  chars.h cooldowns.h constants.h math.h magic.h regen.h fight.h \
  movement.h damage.h
act.other.o: act.other.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h comm.h interpreter.h handler.h \
  find.h db.h casting.h events.h screen.h house.h dg_scripts.h quest.h \
  class.h chars.h skills.h cooldowns.h races.h clan.h privileges.h math.h \
  players.h constants.h pfiles.h magic.h regen.h fight.h movement.h \
  limits.h composition.h lifeforce.h
act.social.o: act.social.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h comm.h interpreter.h handler.h \
  find.h db.h math.h
act.wizard.o: act.wizard.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h comm.h interpreter.h handler.h \
  find.h db.h casting.h events.h house.h screen.h olc.h dg_scripts.h \
  clan.h privileges.h quest.h weather.h class.h chars.h races.h skills.h \
  cooldowns.h constants.h math.h players.h pfiles.h regen.h fight.h \
  legacy_structs.h modify.h act.h movement.h limits.h composition.h \
  lifeforce.h charsize.h textfiles.h
act.wizinfo.o: act.wizinfo.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h comm.h cooldowns.h interpreter.h \
  handler.h find.h db.h casting.h events.h house.h screen.h olc.h \
  dg_scripts.h clan.h privileges.h quest.h weather.h class.h chars.h \
  races.h skills.h constants.h math.h players.h pfiles.h ai.h vsearch.h \
  fight.h corpse_save.h commands.h modify.h limits.h composition.h \
  lifeforce.h charsize.h damage.h
ai_utils.o: ai_utils.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h comm.h interpreter.h handler.h find.h \
  casting.h events.h ai.h skills.h class.h chars.h math.h races.h fight.h \
  movement.h
ban.o: ban.c conf.h sysdep.h structs.h spell_mem.h specprocs.h money.h \
  prefs.h exits.h objects.h effects.h utils.h strings.h text.h rooms.h \
  directions.h comm.h interpreter.h handler.h find.h db.h math.h
board.o: board.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h db.h interpreter.h handler.h find.h \
  editor.h board.h math.h clan.h privileges.h vsearch.h screen.h \
  commands.h modify.h rules.h
casting.o: casting.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h casting.h events.h handler.h find.h db.h \
  interpreter.h spell_parser.h class.h chars.h races.h skills.h \
  constants.h math.h magic.h fight.h pfiles.h act.h movement.h limits.h \
  composition.h lifeforce.h damage.h spells.h
chars.o: chars.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h casting.h events.h math.h comm.h \
  interpreter.h handler.h find.h class.h chars.h races.h skills.h \
  dg_scripts.h screen.h constants.h fight.h act.h movement.h magic.h \
  composition.h lifeforce.h charsize.h damage.h spell_parser.h
charsize.o: charsize.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h db.h comm.h math.h utils.h \
  strings.h text.h rooms.h directions.h races.h charsize.h
clan.o: clan.c conf.h sysdep.h structs.h spell_mem.h specprocs.h money.h \
  prefs.h exits.h objects.h effects.h utils.h strings.h text.h rooms.h \
  directions.h comm.h db.h interpreter.h handler.h find.h clan.h \
  privileges.h math.h players.h screen.h modify.h editor.h act.h
clansys.o: clansys.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h db.h interpreter.h handler.h find.h clan.h \
  privileges.h math.h players.h screen.h limits.h
class.o: class.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h db.h utils.h strings.h \
  text.h rooms.h directions.h casting.h events.h interpreter.h handler.h \
  find.h clan.h privileges.h class.h chars.h comm.h races.h skills.h \
  math.h players.h regen.h pfiles.h
cleric.o: cleric.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h comm.h interpreter.h handler.h find.h \
  casting.h events.h ai.h skills.h class.h chars.h races.h math.h magic.h \
  lifeforce.h
commands.o: commands.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h comm.h utils.h strings.h \
  text.h rooms.h directions.h interpreter.h commands.h db.h olc.h \
  constants.h math.h modify.h
comm.o: comm.c conf.h sysdep.h structs.h spell_mem.h specprocs.h money.h \
  prefs.h exits.h objects.h effects.h utils.h strings.h text.h rooms.h \
  directions.h comm.h interpreter.h handler.h find.h db.h house.h \
  screen.h olc.h events.h dg_scripts.h weather.h casting.h skills.h \
  class.h chars.h constants.h math.h board.h clan.h privileges.h mail.h \
  players.h pfiles.h races.h cooldowns.h fight.h commands.h modify.h \
  act.h editor.h rules.h
composition.o: composition.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h db.h composition.h casting.h \
  events.h math.h comm.h interpreter.h handler.h find.h class.h chars.h \
  races.h skills.h dg_scripts.h screen.h constants.h fight.h act.h \
  movement.h magic.h damage.h
config.o: config.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h
constants.o: constants.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h db.h utils.h \
  strings.h text.h rooms.h directions.h
cooldowns.o: cooldowns.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h comm.h skills.h class.h chars.h \
  fight.h events.h cooldowns.h
corpse_save.o: corpse_save.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h comm.h \
  handler.h find.h db.h interpreter.h utils.h strings.h text.h rooms.h \
  directions.h corpse_save.h pfiles.h math.h limits.h
damage.o: damage.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h comm.h skills.h class.h chars.h math.h \
  damage.h composition.h
db.o: db.c conf.h sysdep.h structs.h spell_mem.h specprocs.h money.h \
  prefs.h exits.h objects.h effects.h utils.h strings.h text.h rooms.h \
  directions.h db.h comm.h handler.h find.h casting.h events.h mail.h \
  interpreter.h house.h corpse_save.h dg_scripts.h weather.h clan.h \
  privileges.h quest.h races.h skills.h class.h chars.h constants.h \
  math.h players.h trophy.h pfiles.h regen.h fight.h commands.h \
  movement.h board.h composition.h charsize.h textfiles.h cooldowns.h
dg_comm.o: dg_comm.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h dg_scripts.h utils.h \
  strings.h text.h rooms.h directions.h comm.h handler.h find.h db.h \
  screen.h
dg_db_scripts.o: dg_db_scripts.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h dg_scripts.h \
  utils.h strings.h text.h rooms.h directions.h db.h handler.h find.h \
  events.h comm.h
dg_debug.o: dg_debug.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h dg_scripts.h utils.h \
  strings.h text.h rooms.h directions.h comm.h interpreter.h handler.h \
  find.h db.h olc.h
dg_handler.o: dg_handler.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h dg_scripts.h \
  utils.h strings.h text.h rooms.h directions.h comm.h db.h handler.h \
  find.h events.h
dg_mobcmd.o: dg_mobcmd.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h dg_scripts.h \
  db.h utils.h strings.h text.h rooms.h directions.h handler.h find.h \
  interpreter.h comm.h skills.h class.h chars.h casting.h events.h \
  quest.h screen.h math.h pfiles.h fight.h players.h movement.h limits.h \
  damage.h
dg_objcmd.o: dg_objcmd.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h screen.h \
  dg_scripts.h utils.h strings.h text.h rooms.h directions.h comm.h \
  interpreter.h handler.h find.h db.h quest.h chars.h events.h regen.h \
  movement.h limits.h damage.h
dg_olc.o: dg_olc.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h db.h olc.h dg_olc.h dg_scripts.h events.h \
  math.h interpreter.h stack.h modify.h
dg_scripts.o: dg_scripts.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h dg_scripts.h \
  utils.h strings.h text.h rooms.h directions.h comm.h interpreter.h \
  handler.h find.h events.h db.h screen.h quest.h class.h chars.h races.h \
  clan.h privileges.h skills.h constants.h math.h casting.h olc.h \
  trophy.h modify.h charsize.h
dg_triggers.o: dg_triggers.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h dg_scripts.h \
  utils.h strings.h text.h rooms.h directions.h comm.h interpreter.h \
  handler.h find.h db.h olc.h constants.h skills.h class.h chars.h math.h
dg_wldcmd.o: dg_wldcmd.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h screen.h \
  dg_scripts.h utils.h strings.h text.h rooms.h directions.h comm.h \
  interpreter.h handler.h find.h db.h quest.h constants.h chars.h \
  events.h regen.h olc.h pfiles.h players.h movement.h limits.h damage.h
directions.o: directions.c conf.h sysdep.h directions.h
diskio.o: diskio.c conf.h sysdep.h diskio.h
editor.o: editor.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h editor.h math.h interpreter.h modify.h \
  screen.h events.h
effects.o: effects.c conf.h sysdep.h effects.h
events.o: events.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h queue.h events.h casting.h handler.h find.h comm.h \
  skills.h class.h chars.h db.h constants.h math.h dg_scripts.h screen.h \
  regen.h fight.h movement.h interpreter.h
exits.o: exits.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h handler.h find.h constants.h db.h math.h
fight.o: fight.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h handler.h find.h interpreter.h db.h \
  casting.h events.h screen.h dg_scripts.h corpse_save.h races.h skills.h \
  class.h chars.h cooldowns.h constants.h math.h players.h trophy.h \
  regen.h magic.h fight.h pfiles.h movement.h limits.h composition.h \
  lifeforce.h damage.h act.h clan.h privileges.h
find.o: find.c conf.h sysdep.h structs.h spell_mem.h specprocs.h money.h \
  prefs.h exits.h objects.h effects.h utils.h strings.h text.h rooms.h \
  directions.h comm.h db.h find.h dg_scripts.h handler.h interpreter.h \
  math.h
genzon.o: genzon.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h genzon.h dg_scripts.h
graph.o: graph.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h interpreter.h handler.h find.h db.h \
  casting.h events.h races.h skills.h class.h chars.h constants.h math.h \
  fight.h movement.h
handler.o: handler.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h db.h handler.h find.h interpreter.h \
  casting.h events.h dg_scripts.h corpse_save.h quest.h races.h chars.h \
  class.h skills.h constants.h math.h players.h trophy.h pfiles.h regen.h \
  fight.h movement.h limits.h ai.h composition.h charsize.h clan.h \
  privileges.h
hedit.o: hedit.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h interpreter.h handler.h \
  find.h comm.h utils.h strings.h text.h rooms.h directions.h db.h olc.h \
  screen.h math.h modify.h
house.o: house.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h comm.h handler.h find.h \
  db.h interpreter.h utils.h strings.h text.h rooms.h directions.h \
  house.h constants.h math.h players.h pfiles.h
interpreter.o: interpreter.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h comm.h \
  interpreter.h db.h commands.h utils.h strings.h text.h rooms.h \
  directions.h casting.h events.h handler.h find.h mail.h screen.h olc.h \
  dg_scripts.h clan.h privileges.h class.h chars.h races.h skills.h \
  constants.h math.h players.h pfiles.h fight.h modify.h act.h \
  cooldowns.h textfiles.h
ispell.o: ispell.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h interpreter.h db.h
lifeforce.o: lifeforce.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h lifeforce.h
limits.o: limits.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h casting.h events.h comm.h db.h handler.h find.h \
  limits.h interpreter.h dg_scripts.h clan.h privileges.h races.h class.h \
  chars.h skills.h math.h players.h pfiles.h regen.h fight.h screen.h
magic.o: magic.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h casting.h events.h handler.h find.h db.h \
  races.h skills.h class.h chars.h math.h constants.h magic.h regen.h \
  fight.h movement.h limits.h composition.h lifeforce.h charsize.h \
  damage.h spells.h
mail.o: mail.c conf.h sysdep.h structs.h spell_mem.h specprocs.h money.h \
  prefs.h exits.h objects.h effects.h utils.h strings.h text.h rooms.h \
  directions.h comm.h db.h interpreter.h handler.h find.h mail.h \
  players.h modify.h
math.o: math.c math.h
medit.o: medit.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h comm.h casting.h events.h \
  utils.h strings.h text.h rooms.h directions.h db.h shop.h olc.h \
  handler.h find.h dg_olc.h dg_scripts.h races.h skills.h class.h chars.h \
  constants.h math.h fight.h modify.h genzon.h composition.h lifeforce.h \
  charsize.h
mobact.o: mobact.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h comm.h interpreter.h handler.h find.h \
  casting.h events.h ai.h skills.h class.h chars.h races.h math.h fight.h \
  movement.h composition.h act.h
modify.o: modify.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h interpreter.h commands.h handler.h find.h db.h \
  comm.h casting.h events.h mail.h olc.h clan.h privileges.h skills.h \
  class.h chars.h math.h dg_scripts.h screen.h modify.h
money.o: money.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h interpreter.h db.h math.h screen.h
movement.o: movement.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h comm.h handler.h find.h \
  db.h interpreter.h math.h utils.h strings.h text.h rooms.h directions.h \
  movement.h skills.h class.h chars.h events.h constants.h regen.h act.h \
  cooldowns.h
objects.o: objects.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h handler.h find.h db.h constants.h genzon.h shop.h \
  olc.h skills.h class.h chars.h math.h dg_scripts.h board.h
oedit.o: oedit.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h comm.h casting.h events.h \
  utils.h strings.h text.h rooms.h directions.h db.h shop.h olc.h \
  dg_olc.h dg_scripts.h skills.h class.h chars.h constants.h math.h \
  fight.h interpreter.h handler.h find.h modify.h board.h composition.h
olc.o: olc.c conf.h sysdep.h structs.h spell_mem.h specprocs.h money.h \
  prefs.h exits.h objects.h effects.h interpreter.h comm.h utils.h \
  strings.h text.h rooms.h directions.h db.h olc.h dg_olc.h dg_scripts.h \
  screen.h casting.h events.h skills.h class.h chars.h math.h commands.h \
  handler.h find.h
pfiles.o: pfiles.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h comm.h handler.h find.h \
  db.h interpreter.h utils.h strings.h text.h rooms.h directions.h \
  casting.h events.h quest.h math.h players.h pfiles.h dg_scripts.h \
  legacy_structs.h constants.h skills.h class.h chars.h modify.h act.h
players.o: players.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h handler.h find.h players.h dg_scripts.h \
  comm.h interpreter.h quest.h math.h chars.h olc.h class.h races.h \
  clan.h privileges.h skills.h constants.h casting.h events.h trophy.h \
  pfiles.h composition.h charsize.h screen.h cooldowns.h
prefs.o: prefs.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h interpreter.h handler.h find.h db.h \
  screen.h clan.h privileges.h
privileges.o: privileges.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h comm.h \
  interpreter.h db.h utils.h strings.h text.h rooms.h directions.h \
  screen.h commands.h privileges.h handler.h find.h math.h constants.h \
  modify.h clan.h
quest.o: quest.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h db.h utils.h strings.h \
  text.h rooms.h directions.h handler.h find.h interpreter.h comm.h \
  casting.h events.h quest.h dg_scripts.h races.h chars.h class.h math.h
queue.o: queue.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h queue.h events.h
races.o: races.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h interpreter.h utils.h \
  strings.h text.h rooms.h directions.h races.h class.h chars.h handler.h \
  find.h comm.h db.h casting.h events.h skills.h math.h regen.h \
  composition.h lifeforce.h charsize.h
redit.o: redit.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h comm.h utils.h strings.h \
  text.h rooms.h directions.h db.h olc.h dg_olc.h dg_scripts.h \
  constants.h math.h modify.h
regen.o: regen.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h casting.h events.h handler.h find.h regen.h \
  races.h comm.h skills.h class.h chars.h math.h interpreter.h fight.h \
  limits.h ai.h
rogue.o: rogue.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h comm.h interpreter.h handler.h find.h \
  casting.h events.h ai.h skills.h class.h chars.h math.h
rooms.o: rooms.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h math.h db.h handler.h find.h skills.h \
  class.h chars.h constants.h
rules.o: rules.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h rules.h screen.h clan.h privileges.h interpreter.h \
  handler.h find.h
rulesys.o: rulesys.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h rules.h
screen.o: screen.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h screen.h math.h
sdedit.o: sdedit.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h interpreter.h handler.h \
  find.h comm.h utils.h strings.h text.h rooms.h directions.h db.h olc.h \
  screen.h casting.h events.h skills.h class.h chars.h
sedit.o: sedit.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h comm.h utils.h strings.h \
  text.h rooms.h directions.h db.h shop.h olc.h constants.h math.h
shop.o: shop.c conf.h sysdep.h structs.h spell_mem.h specprocs.h money.h \
  prefs.h exits.h objects.h effects.h comm.h handler.h find.h db.h \
  interpreter.h utils.h strings.h text.h rooms.h directions.h shop.h \
  class.h chars.h constants.h math.h screen.h modify.h limits.h \
  composition.h
skills.o: skills.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h math.h interpreter.h comm.h skills.h class.h \
  chars.h casting.h events.h races.h fight.h damage.h
sorcerer.o: sorcerer.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h comm.h interpreter.h handler.h find.h \
  casting.h events.h ai.h skills.h class.h chars.h races.h math.h \
  spell_parser.h magic.h composition.h lifeforce.h
spec_assign.o: spec_assign.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h db.h \
  interpreter.h utils.h strings.h text.h rooms.h directions.h
spec_procs.o: spec_procs.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h comm.h interpreter.h handler.h \
  find.h db.h casting.h events.h clan.h privileges.h skills.h class.h \
  chars.h constants.h math.h limits.h movement.h fight.h act.h \
  cooldowns.h
spell_mem.o: spell_mem.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h interpreter.h casting.h events.h \
  handler.h find.h comm.h db.h skills.h class.h chars.h constants.h \
  math.h players.h regen.h screen.h
spell_parser.o: spell_parser.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h interpreter.h casting.h events.h \
  handler.h find.h comm.h db.h spell_parser.h screen.h class.h chars.h \
  skills.h dg_scripts.h math.h magic.h constants.h fight.h spells.h \
  cooldowns.h
spells.o: spells.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h spells.h casting.h events.h handler.h \
  find.h db.h interpreter.h spell_parser.h class.h chars.h races.h \
  skills.h constants.h math.h magic.h fight.h pfiles.h act.h movement.h \
  limits.h composition.h lifeforce.h damage.h
spheres.o: spheres.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h skills.h class.h chars.h \
  spheres.h
strings.o: strings.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h screen.h math.h
text.o: text.c conf.h sysdep.h structs.h spell_mem.h specprocs.h money.h \
  prefs.h exits.h objects.h effects.h utils.h strings.h text.h rooms.h \
  directions.h screen.h math.h editor.h
textfiles.o: textfiles.c conf.h sysdep.h structs.h spell_mem.h \
  specprocs.h money.h prefs.h exits.h objects.h effects.h utils.h \
  strings.h text.h rooms.h directions.h comm.h interpreter.h handler.h \
  find.h db.h textfiles.h modify.h editor.h
trophy.o: trophy.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h comm.h screen.h players.h trophy.h
utils.o: utils.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h screen.h casting.h events.h handler.h \
  find.h weather.h db.h races.h skills.h class.h chars.h math.h \
  interpreter.h constants.h pfiles.h composition.h lifeforce.h
vsearch.o: vsearch.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h interpreter.h handler.h find.h db.h \
  casting.h events.h dg_scripts.h skills.h class.h chars.h races.h \
  constants.h weather.h shop.h olc.h screen.h vsearch.h fight.h modify.h \
  composition.h lifeforce.h charsize.h damage.h
warrior.o: warrior.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h db.h comm.h interpreter.h handler.h find.h \
  casting.h events.h ai.h skills.h class.h chars.h math.h movement.h
weather.o: weather.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h utils.h strings.h text.h \
  rooms.h directions.h comm.h handler.h find.h interpreter.h db.h \
  weather.h math.h
zedit.o: zedit.c conf.h sysdep.h structs.h spell_mem.h specprocs.h \
  money.h prefs.h exits.h objects.h effects.h comm.h utils.h strings.h \
  text.h rooms.h directions.h db.h olc.h weather.h constants.h math.h \
  genzon.h
zmalloc.o: zmalloc.c
