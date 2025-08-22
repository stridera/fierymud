#6302
frozen ice mask~
1 l 100
~
osend %actor% You can't pull it off!  It's frozen itself to your skin!
~
#6390
Herlequin vial trigger~
1 c 2
pour vial volcano~
wait 5
osend %actor% Metamorpho's vial is ripped from your hands.
oechoaround %actor% Metamorpho's vial is ripped from %actor.name%'s 
hands!
wait 6
oecho Metamorpho's vial hovers above the massive volcano before turning 
upside $
wait 2
oecho The top of the vial pops open unleashing a &6&bFLOOD&0 of water!
wait 2
oecho The massive volcano slowly burns itself out with a puff of smoke!
opurge volcano
oload obj 75
oload obj 75
oload obj 75
oload obj 75
opurge %self%
~
#6391
Herlequin volcano trigger 1~
1 b 100
~
oecho A blast of &9&bsmoke&0 and a burst of &1&bflame&0 charges into 
the air!
wait 5
oecho As the smoke clears a figure appears.
wait 5
set val %random.10%
switch %val%
case 1
case 2
case 3
oecho A herlequin warrior lets off a horrible screech!
oload mob 6390
break
case 4
case 5
oecho A herlequin assassin finds a shadow to hide in.
case 6
case 7
oload mob 6393
oload obj 3020
oforce assassin get dagger
oforce assassin wield dagger
break
oecho A herlequin mage begins to chant a demonic mantra.
case 8
case 9
oload mob 6392
default
done
~
#6392
Herlequin volcano trigger 2~
1 b 100
~
set dice1 %random.100%
set dice2 %random.100%
eval damage %dice1% + %dice2%
set victim %random.char%
oecho A massive volcano shoots out a &1&bflaming rock&0!
wait 2
odamage %victim% %damage% crush
if %damdone% == 0
oechoaround %victim% A &1&bflaming&0 rock falls right next to 
%victim.name%!
osend %victim% A &1&bflaming&0 rock falls right next to you!
else
oechoaround %victim% A &1&bflaming&0 rock falls from the sky, smashing 
%victim.name% right on the head! (&1&b%damdone%&0)
osend %victim% A &1&bflaming rock&0 slams you directly on the head!  
OUCH! (&1&b%damdone%&0)
end
~
$~
