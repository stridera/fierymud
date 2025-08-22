#7475
North Swamp Entrance Guard~
0 c 100
east~
if %actor.vnum% == -1
   if %actor.level% < 20
      return 1
      whisper %actor.name% My, my, young adventurer, you are ambitious.
      whisper %actor.name% It is just a bit too dangerous for you to head further east.
      wait 1s
      whisper %actor.name% It is safer to head back the other way.
      bow %actor.name%
   else
      return 0
   endif
else
   return 0
endif
~
$~
