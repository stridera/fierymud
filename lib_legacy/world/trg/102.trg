#10201
Guard Ice Cult~
0 c 100
west~
if %actor.vnum% == -1
  if %actor.level% < 40
     whisper %actor.name% You are much too weak to venture through this tunnel.
     wait 1
     whisper %actor.name% Try other areas first.
     nudge %actor.name%
  else
     return 0
  endif
else
   return 0
endif
~
$~
