#46401
MOB_SOCIAL_DROOL_10~
0 b 10
~
drool
~
#46402
Aviar_troll_greet~
0 h 100
~
* Simple greet trig
set percent %random.100%
if %actor.vnum% == -1
if %actor.level% > 99
wait 1
msend %actor% %self.name% cringes in terror as you appear!
else
if %actor.level% < 100
wait 1
msend %actor% %self.name% sniffs at the air and wrinkles its nose.  Its stomach growls ominously.
msend %actor% %self.name% whispers to you, 'Is you tasty good-eats?'
if %percent% > 65
wait 1
mechoaround %actor% %self.name% roars in terrible hunger and lunges for %actor.name%!!
msend %actor% %self.name% roars in terrible hunger and lunges for you!
kill %actor.name%
end
end
end
else
end
~
#46403
bittern_rand_attack~
0 b 40
~
* Random chance to attack!
set rnd %random.char%
if %rnd.vnum% == -1
if %rnd.level% < 100
wait 1
mechoaround %rnd% %self.name% flies into a rage!
msend %rnd% %self.name% flies into a rage and attacks you!
kill %rnd.name%
end
end
~
#46404
bittern_greet_attack~
0 g 60
~
* attack them players!
if %actor.vnum% == -1
if %actor.level% < 100
wait 1
mechoaround %actor% %self.name% flies into a rage!
msend %actor% %self.name% flies into a rage and attacks you!
kill %actor.name%
end
end
~
#46405
angry-gardener_speech1~
0 d 1
friends~
say My friends... in the topiary.  Even took their scissors.
~
#46406
angry-gardener_speech2~
0 d 1
scissors~
say I know it was them that did it... they took his scissors and left him for dead.
~
#46407
angry-gardener_speech~
0 d 1
gardener~
wait 1
say Those bastards kill them all...
grumble
wait 1
say One day they'll all pay for what they did to my friends.
~
#46408
angry_gardener_death~
0 f 100
~
mecho %self.name% shouts, 'Noooooooo!   I WILL be avenged!  %actor.name%, you wretch!'
~
#46409
angry-gardener_fight~
0 k 20
~
snarl %actor.name%
tell %actor.name% I will kill you.
~
#46410
angry-gardener_greet~
0 g 100
~
msend %actor% %self.name% peers at you intently.
wait 1
msend %actor% %self.name% tells you, 'Was it you?  Did you kill my friends?'
~
#46411
angry-gardener_receive~
0 j 100
~
* 
* comment
*
if %object.vnum% == 18000
wait 1
scream
wait 1
shout %actor.name% is a murderer!  I'm going to kill them.
mload obj 46413
mat 46449 drop bracelet
wait 1
kill %actor.name%
~
#46412
angry-gardener_rand1~
0 b 15
~
say Watch yourself, kiddo.  I've got my eye on you.
~
#46413
angry-gardener_rand2~
0 b 15
~
mutter
~
$~
