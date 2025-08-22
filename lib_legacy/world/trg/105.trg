#10501
Witch_rand1~
0 b 17
~
mecho &0The &2witchwoman&0 looks at you then turns away with a look of disappointment.&0
sigh
~
#10502
MOB_RAND_SOCIAL_CURSE_30~
0 b 30
~
curse
~
#10503
Witch_rand2~
0 b 20
~
mecho &0&2The &0&b&9witchwoman&0&2 looks around frantically.&0
say Damn!
frown
say Hey you.
wait 2
say Yes you! Got any mushrooms? I need some for this spell I'm doing.
pond
wait 1
say I'll reward you if you get me one.
mecho &0She goes back to searching the room.&0
~
#10504
Witch_receive~
0 j 100
~
say thanks %actor.alias%
* This is the cute little script that gives players
* spells for their efforts.
if %object.vnum% == 10509
   wait 1
   mjunk mushroom
   cast 'create food' 
   mload obj 10510
wait 3s
give mushroom %actor.name%
give waybread %actor.name%
   say Thanks for the thought, but thats no good for me.
else
if %object.vnum% == 10508
   wait 1
   mjunk mushroom
cast 'armor' %actor.name%
   thank %actor.name%
else
if %object.vnum% == 10507
   wait 1
   junk mushroom
   thank %actor.name%
   cast 'cure crit' %actor.name%
else
   wait 1
   eye %actor.name%
end
end
end
~
#10505
MOB_GREET_STEAL_40~
0 g 40
~
steal coins %actor.name%
~
#10506
Old_Dweller_greet~
0 h 100
~
* A level checking greet trig.
wait 1
msend %actor% &0&b&6The old man tells you, '&0Ahh Welcome my friend.&b&6'&0
if %actor.level% > 25
if %actor.level% < 71
msend %actor% &0&b&6The old man tells you, '&0This is just a happy little forest.&b&6'&0
msend %actor% &0&b&6The old man tells you, '&0Not worth such a mighty one as you even entering.&b&6'&0
msend %actor% &0&b&6The old man tells you, '&0Please leave.&b&6'&0
end
end
if %actor.level% > 70
bow %actor.name%
msend %actor% &0&b&6The old man says 'It's a pleasure %actor.name%. Always welcome into my home.'
end
if %actor.level% > 5
if %actor.level% < 26
msend %actor% &0&b&6The old man tells you, '&0Hmm You are welcome here, but please don't hurt anything within.&b&6'&0
end
end
if %actor.level% < 6
msend %actor% &0&b&6The old man tells you, '&0Hello there! Welcome to my forest.&b&6'&0
msend %actor% &0&b&6The old man tells you, '&0Its a beautiful, safe little place that I call home.&b&6'&0
end
~
$~
