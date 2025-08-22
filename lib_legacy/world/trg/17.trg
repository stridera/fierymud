#1700
glimmering_recharge~
2 h 100
~
if (%object.vnum% == 1700)
   wait 1 s
   wecho Just as the ring hits the ground, the chest opens as if by magic.
   wait 1 s
   wecho A bright light eminates from chest, focusing on the ring.
   wait 1 s
   wecho The light subsides and the chest closes without help.
   wpurge glimmering
   wload obj 1700
end if
~
#1703
dormitory_sleep~
2 c 100
sleep~
if !%actor%
   return 0
elseif %actor.vnum% >= 0
   return 0
elseif s == %cmd%
   return 0
elseif %actor.stance% != resting && %actor.stance% != alert
   return 0
else
   wsend %actor% You retreat into your cubicle, lay down your belongings, and rest.
   wechoaround %actor% %actor.name% enters %actor.p% cubicle and tunes out the world.
   wrent %actor%
endif
~
#1792
Beast Dirt Trig~
0 gk 100
~
mechoaround %actor% he &1Enraged Beast&0 kicks up a &3claw full of dirt&0 into %actor.name%'s face &9&bblinding&0 him!
The &1%Enraged Beast%&0 kicks up a &3claw full of dirt&0 into your face &9&bblinding&0 you!
~
#1793
Rip Hunk~
1 g 10
~
osend %actor% With a ferocious roar the %self% rips out a hunk of your flesh!
oechoaround %actor% With a ferocious roar the %self% rips a hunk of flesh from %actor.name%!
odamage %actor% 700
~
#1794
Beast_10~
0 l 10
10~
if !%done10%
mecho %self% howls out in pain, writhing to and fro, its eyes glowing in rage and fear!
mecho RUN FOR YOUR LIFE!
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
mload mob 48024
mforce rat kill %actor.name%
endif
set done10 yes
global done10
~
#1795
Beast_25~
0 l 25
25~
if !%done25%
mecho The ground all around begins to rustle with life...
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
mload mob 43001
mforce maudlin-panther kill %actor.name%
endif
set done25 yes
global done25
~
#1796
Beast_50~
0 l 50
50~
if !%done50%
mecho %self.name% writhes in pain, slamming its enormous claws on the ground!
mecho -     -    -   -  - -----BANG----- -  -   -    -     -
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
mload mob 2801
mforce ripples kill %actor.name%
endif
set done50 yes
global done50
~
#1797
Beast_75~
0 l 75
75~
if !%done75%
mecho %self% screeches as its face contorts in pain!
mecho The despicable beast of the realm come to its assistance!
mload mob 16009
mforce demon kill %actor.name%
mload mob 16009
mforce demon kill %actor.name%
mload mob 16009
mforce demon kill %actor.name%
mload mob 16009
mforce demon kill %actor.name%
mload mob 16009
mforce demon kill %actor.name%
mload mob 16009
mforce demon kill %actor.name%
mload mob 16009
mforce demon kill %actor.name%
mload mob 16009
mforce demon kill %actor.name%
mload mob 16009
mforce demon kill %actor.name%
mload mob 16009
mforce demon kill %actor.name%
mload mob 16009
mforce demon kill %actor.name%
endif
set done75 yes
global done75
~
#1798
Head Tear~
1 g 80
~
osend %actor% Seemingly for no reason %self% sets its gaze on you...
osend %actor%
osend %actor%
osend %actor% charging across the field of battle, the %self% leaps at you!
osend %actor%
osend %actor%
osend %actor% landing on you, %self% begins to maul the living life out of you!!
osend %actor%
osend %actor%
osend %actor% you begin seeing red, but alas its too late, the %self% rips your head off!!!!!!
oechoaround %actor% %self% leaps onto %actor.name% mauling him into a bloody pulp!
odamage %actor% 30000
~
#1799
Growl Trigger~
0 g 15
~
msend %actor% As you enter the room %self% raises its head towards you and growls ferociously...
msend %actor% 
msend %actor% ggggggggggggggggggggg
msend %actor% 
msend %actor%  ggggGggggGggggGggggGggg
msend %actor% 
msend %actor%   gggGGGgggGGGgggGGGggg
msend %actor% 
msend %actor%    GGGgGGGgGGGgGGGgGGGg
msend %actor% 
msend %actor%     GGGGGGGGGGGGGGGGGGG
msend %actor% 
msend %actor%      GrGrGrGrGrGrGrGrGrGrGrGrGrG
msend %actor% 
msend %actor%       GRGRGRGRGRGRGRGRGRGRGRG
msend %actor% 
msend %actor%         RRRRRRRRRRRRRRRRRRRRRRRRRRR
msend %actor% 
msend %actor%          RoRoRoRoRoRoRoRoRoRoRoRoRoRoRoRoR
msend %actor% 
msend %actor%           OOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
msend %actor%           
msend %actor%             OwOwOwOwOwOwOwOwOwOwOwOwOwOwOwOwO
msend %actor%  
msend %actor%               WWWWWWWWWWWWWWWWWWWWWWW
msend %actor% 
msend %actor%                 WlWlWlWlWlWlWlWlWlWlWlWlWlWlWlWlWlWlWlWlW
msend %actor% 
msend %actor%                   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
msend %actor% 
msend %actor%                     !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
mdamage %actor% 30000
~
$~
