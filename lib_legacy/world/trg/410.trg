#41001
red_death_spawn_yellow~
0 f 100
~
   * checking if yellow queen is already loaded
   if %self.mexists[41009]% < 1
      * load the yellow queen' 
      mat 41021 mload mob 41009
      mecho %self.name% screams, 'My yellow sister shall avenge my death!'
   else
      mecho %self.name% screams, 'My yellow sister shall avenge my death!'
   endif
~
#41002
yellow_death_spawn_green~
0 f 100
~
   * checking if green queen is already loaded
   if %self.mexists[41010]% < 1
      mat 41030 mload mob 41010
      mecho %self.name% moans, 'My green sister will make you sorry!'
   else
      mecho %self.name% moans, 'My green sister will make you sorry!'
   endif
~
#41003
green_death_spawn_blue~
0 f 100
~
   * checking if blue queen is already loaded
   if %self.mexists[41011]% < 1
      mat 41039 mload mob 41011
      mecho %self.name% mutters, 'Mother always said my blue sister would look after me...'
   else
      mecho %self.name% mutters, 'Mother always said my blue sister would look after me...'
   endif
~
#41004
warrior_death_spawn_red~
0 f 100
~
   *check if red queen already loaded
   if %self.mexists[41008]% < 1
      * load red queen
      mat 41015 mload mob 41008
      mecho %self.name% shouts, 'I'm sorry, my Queen!'
   else
      mecho %self.name% shouts, 'I'm sorry, my Queen!'
   endif
~
#41005
**UNUSED**~
0 f 100
~
*
* Trigger 61554 for Creeping Doom
*
set person %actor%
set i %actor.group_size%
if %i%
   unset person
   while %i% > 0
      set person %actor.group_member[%i%]%
      if %person.room% == %self.room%
         if %person.quest_stage[creeping_doom]% == 2 || (%person.level% > 80 && (%person.vnum% >= 1000 && %person.vnum% <= 1038))
            set rnd %random.50%
            if %rnd% <= %self.level%
               mload obj 61517
            endif
         endif
      endif
      eval i %i% - 1
   done
elseif %person.quest_stage[creeping_doom]% == 2 || (%person.level% > 80 && (%person.vnum% >= 1000 && %person.vnum% <= 1038))
   set rnd %random.50%
   if %rnd% <= %self.level%
     mload obj 61517
   endif
endif
*
* Death trigger for random gem and armor drops - 55567
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 &%will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55573
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   endif
endif
~
#41006
**UNUSED**~
0 f 100
~
*
* Trigger 61554 for Creeping Doom
*
set person %actor%
set i %actor.group_size%
if %i%
   unset person
   while %i% > 0
      set person %actor.group_member[%i%]%
      if %person.room% == %self.room%
         if %person.quest_stage[creeping_doom]% == 2 || (%person.level% > 80 && (%person.vnum% >= 1000 && %person.vnum% <= 1038))
            set rnd %random.50%
            if %rnd% <= %self.level%
               mload obj 61517
            endif
         endif
      endif
      eval i %i% - 1
   done
elseif %person.quest_stage[creeping_doom]% == 2 || (%person.level% > 80 && (%person.vnum% >= 1000 && %person.vnum% <= 1038))
   set rnd %random.50%
   if %rnd% <= %self.level%
     mload obj 61517
   endif
endif
*
* Death trigger for random gem and armor drops - 55565
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55569
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 &%will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55569
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   endif
endif
~
#41007
**UNUSED**~
0 g 100
~
*
* Trigger 61554 for Creeping Doom
*
set person %actor%
set i %actor.group_size%
if %i%
   unset person
   while %i% > 0
      set person %actor.group_member[%i%]%
      if %person.room% == %self.room%
         if %person.quest_stage[creeping_doom]% == 2 || (%person.level% > 80 && (%person.vnum% >= 1000 && %person.vnum% <= 1038))
            set rnd %random.50%
            if %rnd% <= %self.level%
               mload obj 61517
            endif
         endif
      endif
      eval i %i% - 1
   done
elseif %person.quest_stage[creeping_doom]% == 2 || (%person.level% > 80 && (%person.vnum% >= 1000 && %person.vnum% <= 1038))
   set rnd %random.50%
   if %rnd% <= %self.level%
     mload obj 61517
   endif
endif
*
* Death trigger for random gem and armor drops - 55570
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
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 &%will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55577
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   endif
endif
~
$~
