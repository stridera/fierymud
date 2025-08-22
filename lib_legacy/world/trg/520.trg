#52000
rock_demon_call_rock~
2 a 100
~
wait 1 s
wecho The demon has animated some rocks which spin around you!
wait 1 s
set numhits %random.3%
eval numhits %numhits% + 2
set thishit 0
while %thishit% < %numhits%
   set dmg %random.50%
   eval dmg %dmg% + 50
   set rnd %random.char%
   if %rnd.vnum% == 52017
      wechoaround %rnd% A lump of rock merges with %rnd.name% and he seems stronger! (&3%dmg%&0)
      wsend %rnd% You absorb strength from the rock! (&3%dmg%&0)
      wheal %rnd% %dmg%
   else
      if %rnd.quest_stage[meteorswarm]% == 2 || %rnd.quest_variable[meteorswarm:new]% /= yes
         if %rnd.quest_stage[meteorswarm]% == 2
            quest advance meteorswarm %rnd%
         elseif %rnd.quest_variable[meteorswarm:new]% /= yes
            quest variable meteorswarm %rnd% new no
         endif
        wload obj 48252
        wsend %rnd% &1&bA flaming meteor shoots off the towering rock demon,&0
        wsend %rnd% &1&bsoars through the sky, and begins to fall toward the ground!&0
        wechoaround %rnd% &1&bA flaming meteor shoots off the towering rock demon,&0
        wechoaround %rnd% &1&bbut %rnd.name% catches it!&0
        wforce %rnd% get meteorite
        wsend %rnd% &1&bYou now have an appropriate focus!&0
      else
        wdamage %rnd% %dmg% crush
        if %damdone% == 0
           wsend %rnd% A lump of rock whizzes right through you!
           wechoaround %rnd% %rnd.name% watches a rock zip right through %rnd.p% body.
        else
           wsend %rnd% A lump of rock whizzes towards you.  You can't dodge! (&1&b%damdone%&0)
           wechoaround %rnd% %rnd.name% gets whacked by a lump of flying rock! (&4%damdone%&0)
        endif
      endif
   endif
   eval thishit %thishit% + 1
done
~
#52001
grow_hydra_head~
0 f 100
~
* Only do this if killer %actor% exists, otherwise they were killed by death of body!
if %actor%
   * return 0 for no death cry...
   return 0
   * If killer holds a lit branch (obj 52034) then no new head
   set no_head 0
   if (%actor.wearing[52035]%)
      set no_head %actor.wearing[52035]%
   elseif (%actor.wearing[52034]%)
      set no_head %actor.wearing[52034]%
   endif
   if %no_head%
      mechoaround %actor% %actor.name% seals the Hydra's neck with %no_head.shortdesc%, preventing another head from growing!
      msend %actor% You cauterize the wound with %no_head.shortdesc% - no more heads from there!
   elseif %random.100% > 65
      mechoaround %actor% As soon as %actor.name% strikes the head from the body a new one grows!
      msend %actor% As soon as you chop the head off, another one starts to grow!
      mload mob 52009
   else
      mechoaround %actor% As soon as %actor.name% strikes the head from the body TWO new ones grow!
      msend %actor% As soon as you chop the head off, TWO new ones appear!
      mload mob 52009
      mload mob 52009
   endif
   if %get.mob_count[52009]% >= 30
      mecho The heads begin to scream and fly in all directions, and COMPLETELY ANNIHILATE YOU!
   endif
endif
~
#52002
break_burning_branch~
1 h 100
~
wait 1
oecho The branch cracks and breaks into 3 pieces!
oload obj 52035
oload obj 52035
oload obj 52035
opurge burning-branch
~
#52003
hydra_death_cry~
0 f 100
~
m_run_room_trig 52004
~
#52004
all_hydra_heads_die~
2 a 100
(Run automatically by trigger 52003 on mob 52010)~
set person %self.people%
while %person%
   set next %person.next_in_room%
   if %person.vnum% == 52009
      wdamage %person% 50000
   endif
   set person %next%
done
~
#52005
UNUSED~
2 g 100
~
wait 5
if %smoke_demon% != pi
if %actor.vnum% == -1
wload mob 52019
wecho A spirit demon forms out of the air around you.
set smoke_demon pi
global smoke_demon
wforce spirit-demon kill %actor.name%
endif
endif
~
#52006
UNUSED~
2 g 100
~
* hope this works as well as the theory
* idea is that this trigger runs _before_ players
* enter. Course it only calcs the size of the last
* group to enter, so if a player comes in later
* the global var gets scragged :-(
* hehe if only room had a vnum field this could be generic!
set peeps %people.52059%
wait 5
eval peeps %people.52059% - %peeps%
global peeps
~
#52007
death_of_tree~
0 f 100
~
mload obj 52034
emote whispers 'Ah, the relief...'
return 0
~
#52010
bronze_guard_greet1~
0 g 100
~
if %actor.vnum% == -1
if %actor.level% > 100
glare %actor.name%
say I was supposed to get PAID for this gig.
poke %actor.name%
else
mechoaround %actor.name% The statue's eyes seem to follow %actor.name% around for a moment, they look a bit uncomfortable.
msend %actor.name% The statue's eyes seem to stare directly at you for a moment, how disturbing.
endif
endif
~
#52011
Sagece_Attack~
0 k 30
~
wait 1s
set value %random.10%
switch %value%
   case 1
      breath fire
      break
   case 2
   case 3
      sweep
      break
   case 4
   case 5
      roar
      break
   case 6
      m_run_room_trig 52020
      break
   case 7
      m_run_room_trig 52021
      break
   default
      growl
done
~
#52012
sagece_greet1~
0 g 100
~
if %actor% && (%actor.level% < 100)
   emote screams, 'I have been waiting for you!!'
   kill %actor.name%
   if %actor.vnum% != -1
      breath fire
   endif
endif
~
#52013
dark_elf_open1~
0 c 0
open~
if %arg% /= chest
msend %actor% A dark elf stops you from opening a chest!
mechoaround %actor% A dark elf jumps in %actor.name%'s way, keeping them from the chest!
say Insolent fools!  Such blatant thievery.
close panel west
kill %actor.name%
end if
~
#52014
assassin_greet1~
0 g 80
~
if %actor.level% < 100
emote whispers, 'Trespassers!'
backstab %actor.name%
else
bow %actor.name%
say Welcome, Diety.
end
~
#52015
rock_demon_fight~
0 k 60
~
wait 1s
emote starts to glow slightly and his eyes burn hotly.
say uruk chachronu vuur
emote gestures upwards with his hands.
m_run_room_trig 52000
~
#52016
dying_tree_speak1~
0 d 100
help? hydra?~
sigh
mecho %self.name% says, 'I was the Tree of Life, and now I live in agony
mecho &0thanks to the Hydra's fire.  Kill me, and take a branch to help you kill the
mecho &0Hydra.'
~
#52017
dying_tree_allgreet1~
0 h 100
~
if %actor.vnum% == -1
say %actor.name%, have you come to help me?
endif
~
#52018
dying_tree_rand1~
0 b 25
~
switch %random.3%
   case 1
      mecho A few falling leaves burst into flames as they get too close to the burning tree.
      break
   case 2
      mecho The burning tree emits a loud hissing noise that startles a few nearby rodents.
      break
   case 3
      emote curses the Hydra.
      break
done
~
#52019
UNUSED~
0 f 100
~
unset smoke_demon
~
#52020
sagece squish~
2 a 100
~
wecho &6&bSagece of Raymif beats her monstrous wings, launching herself into the air!&0
wait 5s
set victim 0
set count 0
while %victim% == 0
   set victim %random.char%
   if %victim.vnum% == 52012
      set victim 0
   endif
   eval count %count% + 1
   if %count% > 10
      wecho &3Sagece of Raymif slowly returns to the floor, rattling the hall.&0
      halt
   endif
done
wdamage %victim% 5000 crush
wechoaround %victim% &6Sagece of Raymif swoops down, dropping her bulk onto %victim.name%!&0 (&4%damdone%&0)
wsend %victim% &6Sagece of Raymif swoops down, dropping her massive bulk on &1&bYOU&0&6!&0 (&1%damdone%&0)
~
#52021
sagece fling~
2 a 100
~
set victim %random.char%
if %victim.vnum% != 52012
   eval damage 75 + %random.40%
   wechoaround %victim% &2Sagece of Raymif sweeps with her enormous tail, throwing %victim.name% from the area!&0 (&4%damage%&0)
   wsend %victim% &2Sagece of Raymif sweeps with her enormous tail, knocking you out of the area!&0 (&1&b%damage%&0)
   eval room 52082 + %random.5%
   if %room% == 52086
      set room 52083
   endif
   wteleport %victim% %room%
   wforce %victim% look
endif
~
#52022
UNUSED~
0 f 100
~
mecho A HUGE glob of acid from Sagece's corpse spews forth and hits you.
m_run_room_trig 52024
~
#52023
UNUSED~
0 f 100
~
m_run_room_trig 52022
~
#52024
UNUSED~
2 cfg 0
~
set damage %random.2000%
while %people.52086% > 0
mdamage %random.char% %damage%
done
~
#52025
rock_well_load_door~
2 a 100
~
wdoor 2201 up room 52039
wdoor 2201 up description A ruin mansion lies just above.  If only it was reachable.
wdoor 2201 up name Basement Ceiling
set person %self.people%
while %person%
   if %person.quest_stage[meteorswarm]% == 2 || %person.quest_variable[meteorswarm:new]% /= yes
      if %person.quest_stage[meteorswarm]% == 2
         quest advance meteorswarm %person.name%
      elseif %person.quest_variable[meteorswarm:new]% /= yes
         quest variable meteorswarm %person.name% new no
      endif
      wload obj 48252
      wecho A flaming meteor shoots off the towering rock demon, soars through the sky, and begins to fall toward the ground!
   endif
   set person %person.next_in_room%
done
~
#52026
rock_demon_walk_in~
0 h 100
~
msend %actor% The basement ceiling collapses in on itself sealing you in!
~
#52027
rock_well_death~
0 f 100
~
m_run_room_trig 52025
~
#52052
sagece-dead~
0 f 100
~
mat 52093 m_run_room_trig 52054
~
#52053
UNUSED~
2 d 0
nevereverusethistriggerorIeatyouforbreakfast~
wload obj 52052
~
#52054
hoard-door~
2 a 100
~
wdoor 52093 east flags abce
wdoor 52093 east room 52094
wdoor 52093 east name panel massive
wdoor 52093 east key 52013
wdoor 52094 west flags abc
wdoor 52094 west room 52093
wdoor 52094 west name panel massive
wdoor 52094 west key 52013
~
#52055
dark-elves-fight~
0 k 100
~
wait 2s
hitall
~
$~
