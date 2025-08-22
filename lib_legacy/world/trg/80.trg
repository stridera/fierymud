#8030
fire ant mandible drop~
0 f 100
~
if %random.20% == 20
  mload obj 8050
endif
~
#8031
ice_drake_death~
0 f 100
~
*
* Now for the cycle portion..
*
* Does next mob in spawn cycle already exist?
If %get.mob_count[8032]% < 1
   * Generate random room number to spawn drakes in.
   * Thanks to the evil Pergus for inspiring me to be
   * more evil.
   set rnd_range %random.126%
   eval rnd_room %rnd_range% + 8049
   mat 16095 mload mob 8032
   set rnd %random.100%
   mat 16095 mteleport ash %rnd_room%
   * Sometimes creatures don't get teleported out of the loading
   * room so we're gonna go back and purge it just incase.
   mat 16095 mpurge
   mecho %self.name% cries out for help from his ashen brother!
   mecho From somewhere else in the farmlands comes another mighty roar!
   mload obj 8031
endif
*
*
*
~
#8032
ash_drake_death~
0 f 100
~
*
* Now for the cycle portion..
*
* Does next mob in spawn cycle already exist?
If %get.mob_count[8033]% < 1
   * Generate random room number to spawn drakes in.
   * Thanks to the evil Pergus for inspiring me to be
   * more evil.
   set rnd_range %random.126%
   eval rnd_room %rnd_range% + 8049
   mat 16095 mload mob 8033
   set rnd %random.100%
   mat 16095 mteleport emerald %rnd_room%
   * Sometimes creatures don't get teleported out of the loading
   * room so we're gonna go back and purge it just incase.
   mat 16095 mpurge
   mecho %self.name% cries out for help from his emerald brother!
   mecho From somewhere else in the farmlands comes another mighty roar!
   mload obj 8032
endif
*
*
*
~
#8033
emerald_drake_death~
0 f 100
~
*
* Now for the cycle portion..
*
* Does next mob in spawn cycle already exist?
If %get.mob_count[8034]% < 1
   * Generate random room number to spawn drakes in.
   * Thanks to the evil Pergus for inspiring me to be
   * more evil.
   set rnd_range %random.126%
   eval rnd_room %rnd_range% + 8049
   mat 16095 mload mob 8034
   set rnd %random.100%
   mat 16095 mteleport wug %rnd_room%
   * Sometimes creatures don't get teleported out of the loading
   * room so we're gonna go back and purge it just incase.
   mat 16095 mpurge
   mecho %self.name% cries out for help from his brother Wug!
   mecho From somewhere else in the farmlands comes another mighty roar!
   mload obj 8033
endif
*
*
*
~
#8034
wug_drake_death~
0 f 100
~
say You have vanquished me for now but I will return!
mload obj 8034
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* boss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 20
   * drop nothing and bail
   halt
endif
if %will_drop% <= 60 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 &%will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55585
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
   endif
endif
~
#8035
fly_away~
0 k 100
~
*
* this is intended to make the creature fly off
* to a random room to avoid high levels killing
* it..
*
* Exception written in to allow players on the 
* dragon hunt to kill them.
* 
* Generate random room number to spawn drakes in.
* Thanks to the evil Pergus for inspiring me to be
* more evil.
if %actor.level% > 30 && %actor.quest_stage[dragon_slayer]% != 3
  set rnd_range %random.126%
  eval rnd_room %rnd_range% + 8049
  mecho %self.name% flies off because you seem to be a bit to powerful.
  mecho (Let the newbies < 30 do this)
  mteleport drakling %rnd_room%
endif
~
#8039
Morgan_hill_newbie_guard~
0 c 100
south~
if %actor.vnum% == -1
if %actor.level% < 10
return 1
msend %actor% %self.name% places a hand in front of you.
mechoaround %actor% %self.name% places a hand up in front of %actor.name%.
whisper %actor.name% Hold on there!  South of here is terribly dangerous for someone of your skill.
wait 1s
whisper %actor.name% I suggest adventuring elsewhere for now.
bow %actor.name%
else
return 0
endif
else
return 0
endif
~
$~
