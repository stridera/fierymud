#8600
captain trigger~
0 g 100
~
if (%direction% == south)
say Greetings, %actor.name%.  Welcome to the Kingdom of the Meer Cats!
say Are you a friend or a foe?
elseif (%direction% == north)
say Fare thee well, %actor.name%.
endif
~
#8601
captain_guard_death~
0 f 100
~
mjunk key
~
#8602
captain_guard_speak1~
0 d 100
foe foe?~
say That is unacceptable!
wait 1
kill %actor.name%
~
#8603
captain_guard_speak2~
0 d 100
friend friend?~
say Very good!
unlock trellis
open trellis
say Welcome to the Kingdom of the Meer Cats.
bow %actor.name%
~
#8604
captain_guard_speak3~
0 d 100
yes no~
wait 1s
say Very funny, %actor.name%.
~
$~
