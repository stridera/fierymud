#16201
neriskaan speech pyramid~
2 d 100
neris'kaan Neris'Kaan~
wdoor 16271 n room 16267
wecho &3&bDust fills the air as a wind groans by you flowing to the north.&0
wecho &3Massive pillars of stone open to form a new passage.&0
return 0
~
#16275
pyramid_entrance_newbie_guard~
0 c 100
north~
if %actor.vnum% == -1
   if %actor.level% < 40
      laugh %actor.name%
      whisper %actor.name% You should go try somewhere a little more manageable for you.
      wait 1
      whisper %actor.name% You will grow to adventure here soon enough.
      bow
   else
      return 0
   endif
else
   return 0
endif
~
$~
