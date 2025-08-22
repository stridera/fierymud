#18600
Desk_unlock_key_remove~
1 l 100
~
if %actor.room% == 18630
   oload obj 18607
   oforce %actor% get script_key
   oforce %actor% unlock oblong
   opurge script_key
endif
~
$~
