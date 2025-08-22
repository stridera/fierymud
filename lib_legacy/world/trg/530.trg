#53001
spider_rand~
0 b 45
~
* At a random chance attack a player in the room
set rnd %random.char%
if %rnd.level% < 100
if %rnd.vnum% == -1
kill %rnd.name%
end
end
~
#53002
sunken_rand_attack~
0 b 35
~
   * At a random chance attack a player in the room
   set rnd %random.char%
   if %rnd.level% < 100
   if %rnd.vnum% == -1
   kill %rnd.name%
   else
   end
   else
   end
~
$~
