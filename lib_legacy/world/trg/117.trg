#11700
11765_to_11766_south_exit~
2 c 100
wave fog~
wdoor 11765 south room 11766
wdoor 11765 south description A wall of fog is parted to the south, allowing passage.
wdoor 11765 south name mistwall mistdoor
wechoaround %actor% %actor.name% waves %actor.p% hands around causing the fog south to dissipate allowing passage.
wsend %actor% By moving your hands you clear a passage through the fog to the south.
wat 11766 wecho The fog to the north clears a little.
wait 1 t
wecho The fog slowly closes back around the exit to the south.
wdoor 11765 south purge
~
#11701
branches_attack_rand~
2 g 100
~
wait 2
if %actor.vnum% == -1 &%actor.level% < 100
   set dice1 %random.55%
   set dice2 %random.55%
   eval damage %dice1% + %dice2%
   set victim %random.char%
   wdamage %victim% %damage% slash
   if %damdone% == 0
      wechoaround %victim% One of the branches suddenly whips out at %victim.name%!
      wsend %victim% A gnarled tree branch reached out and tried to slash you!
   else
      wechoaround %victim% One of the branches suddenly whips out at %victim.name%, cutting %victim.o%. (&4%damdone%&0)
      wsend %victim% A gnarled tree branch reached out and slashed you!! (&1%damdone%&0)
   end
   wait 3 s
endif
~
#11798
mysa_death_dropcoins~
0 f 100
~
return 0
mecho With a mighty roar Mysa falls!!
mload obj 11798
drop treasure
mecho  
mecho A huge pile of treasure is now available upon the dragon's demise!
~
#11799
Mysa_Attack~
0 k 15
~
set val %random.10%
switch %val%
case 1
breath lightning
break
case 2
case 3
sweep
break
case 4
case 5
roar
break
default
growl
done
~
#11800
mist_caster_nofight~
0 k 100
~
mload obj 11800
return 0
mecho The mist begins to fall apart...
drop ball
mecho The violent magic has dispersed the mist temporarily.
mpurge no-kill
~
#11801
mist_caster_rand_fog~
0 b 15
~
mset mist invis 100
wait 2
cast 'wall of fog'
~
#11802
room_reload_mob~
2 h 100
ball~
if %object.vnum% == 11800
   wait 2s
   wpurge ball-mist
   wecho The mist begins to reform.
   wload mob 11800
endif
~
#11811
assassin_vine_load~
0 o 100
~
mskillset %self% backstab
~
#11812
assassin_vine_greet~
0 g 100
~
if %actor.vnum% == -1
    emote lunches toward %actor.name%.
    backstab %actor.name%
endif
~
#11823
rock_fall_on_head~
2 g 100
~
wait 2
if %actor.level% < 100
   set die1 %random.60%
   set die2 %random.60%
   eval damage %die1% + %die2%
   set victim %random.char%
   wdamage %victim% %damage% crush
   if %damdone% == 0
      wechoaround %victim% A large stone slipped off of the entrance to a cave, but %victim.name% is unharmed.
      wsend %victim% A large rock from the cave entrace to the west just missed you!
   else
      wechoaround %victim% A large stone slipped off of the entrance to a cave to the west and hit %victim.name%. (&4%damdone%&0)
      wsend %victim% A large rock from the cave entrace to the west hits you in the head! (&1%damdone%&0)
   end
   wait 3 s
end
~
#11838
mist_attack~
2 c 100
look~
if ((%arg% == carving) || (%arg% == carvings))
    return 1
    set now %time.stamp%
    if ((%mist_loaded% != 1) && (%last_load% < %now% - 2))
        set mist_loaded 1
        global mist_loaded
        set last_load %now%
        global last_load
        wechoaround %actor% As %actor.name% looks at the carvings, an eerie stillness seems to set in...
        wsend %actor% As you look at the carvings, an eerie stillness seems to set in...
        wait 2s
        wload mob 11804
        wecho The mist seem to come alive, filled with hostility!
        wechoaround %actor% The mist roars and charges at %actor.name%.
        wsend %actor% The mist roars and charges at YOU!
        wait 2
        wforce mist-demon kill %actor.name%
    else
        wsend %actor% The mists around the carvings seem ominous and hostile.
    endif
else
    return 0
endif
~
#11839
mist_death~
0 f 100
~
m_run_room_trig 11840
~
#11840
mist_death_reset~
2 a 100
~
set mist_loaded 0
global mist_loaded
~
$~
