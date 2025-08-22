#7075
Mystwatch Entrance Guard (southern)~
0 c 100
north~
if %actor.vnum% == -1
  if %actor.level% < 30
    whisper %actor.name% Young adventurer, you are far too small to venture north of here.
    wait 1
    whisper %actor.name%  Try a safer location to pick battles for now.
    smile %actor.name%
  else
    return 0
  endif
else
  return 0
endif
~
$~
