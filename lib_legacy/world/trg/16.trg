#1604
pool_of_chems_hurts~
1 g 100
~
set var1 %random.12%
eval damage 10 + %var1%
odamage %actor% %damage% acid
if %damdone% == 0
osend %actor% The chemicals dribble through your fingers back into the pool.
oechoaround %actor% %actor.name% tries to pick up the chemicals, but they dribble through %actor.p% fingers.
else
osend %actor% OUCH! The chemicals burn as they drip through your fingers. (&2%damdone%&0)
oechoaround %actor% %actor.name% yelps as %actor.p% fingers are burned by some chemicals. (&2%damdone%&0)
end
return 0
~
#1639
Entry_from_plateau_to_chimney~
2 c 100
down~
if %actor.size% == tiny || %actor.size% == small || %actor.size% == medium
   wechoaround %actor% %actor.name% leaves down.
   wteleport %actor% 1691
   wforce %actor% look
   wechoaround %actor% %actor.name% enters from above.
else
   wsend %actor% You're too large to go there.
   halt
end if
~
#1640
Chimney_top_burns~
2 b 100
~
if %self.people[count]% > 0
   set damage 1
   set vctm %random.char%
   wdamage %vctm% %damage% fire
   if %damdone% == 0
      wsend %vctm% The burning smoke blows into your eyes, but you are well protected from its damage.
      wechoaround %vctm% Burning smoke billows around %vctm.name%, deflected by %vctm.p% magic.
   else
      wsend %vctm% Burning smoke blows straight into your eyes!  It burns! (&1%damdone%&0)
      wechoaround %vctm% %vctm.name% cries out in pain as %vctm.n% is burned by the smoke in %vctm.p% eyes! (&1%damdone%&0)
   end
end
~
#1652
Shards_of_glass_hurt~
1 g 100
~
set var1 %random.10%
eval damage 10 + %var1%
odamage %actor% %damage% slash
if %damdone% == 0
osend %actor% You pick up a few shards, but find nothing interesting and drop them again.
oechoaround %actor% %actor.name% grabs some shards of glass, but drops them again.
else
osend %actor% The shards of glass fall through your fingers, leaving large slices in your hands! (&1%damdone%&0)
oechoaround %actor% %actor.name% curses as some shards of glass fall through %actor.p% fingers. (&1%damdone%&0)
end
return 0
~
#1679
Clean_room_echo~
2 b 50
~
if %self.people[count]%
  wecho A large arm swings over your head, spraying you with a chemical mix.
endif
~
#1685
open_door~
2 a 100
~
wdoor 1686 north room 1687
wdoor 1686 north description A small, hand-dug, exit is carved into the wall.
wait 2 s
wecho A haggard dwarf reveals a hand-dug tunnel to the north.
wat 1687 wecho A small noise can be heard as a few stones fall away to the south, revealing an exit.
wdoor 1687 south room 1686
wdoor 1687 south description A small, hand-dug, exit is carved into the wall.
wforce dwarf say Hurry, we don't have much time!
wecho A haggard dwarf rushs north.
wpurge dwarf
wait 10 s
wecho The hole in the wall collapses and is no more!
wat 1687 wecho The hole in the wall collapses and is no more!
wdoor 1686 north purge
wdoor 1687 south purge
~
#1686
dwarven_exit_load~
0 d 100
yes no~
if (%speech% /= yes)
   say Excellent, come with me then!  I know another way out!
   wait 2 s
   emot cautiously moves toward the back of the cell.
   wait 3 s
   mecho A haggard dwarf scratches at some rocks in the wall.
   m_run_room_trig 1685
end if
if (%speech% /= no)
   say Then you are not friend to us!
   kill %actor.name%
end if
~
#1687
haggard_dwarf_greet~
0 h 100
~
wait 5
emot rushes toward you, yelling in a frenzy!
wait 15
emot trips over his own feet and lands flat on his face!
wait 20
emot quickly scrambles back to a corner of the cell.
wait 15
say If you're here to kill me, then do it already!
wait 1 s
say If not, then why are you here?  Are you a friend to us dwarves?
wait 2
whine
~
#1689
Fireplace_burns~
2 b 100
~
if %self.people[count]% > 0
   eval damage %random.6% + 6
   set victim %random.char%
   if %victim%
      wdamage %victim% %damage% fire
      if %damdone% == 0
         wsend %victim% The roaring fire flares up, but you are well protected from its blaze.
         wechoaround %victim% A flare within the fire engulfs %victim.name%, but %actor.n% is well protected against the inferno.
      else
         wsend %victim% The roaring fire flares up, burning you! (&1%damdone%&0)
         wechoaround %victim% %victim.name% cries out in pain as %victim.n% is burned by the fire! (&1%damdone%&0)
      end
   end
end
~
#1690
UNUSED~
2 b 100
~
Nothing.
~
#1691
Entry_from_chimney_to_plateau~
2 c 100
up~
if %actor.size% == tiny || %actor.size% == small || %actor.size% == medium
   wechoaround %actor% %actor.name% leaves up.
   wteleport %actor% 1639
   wforce %actor% look
   wechoaround %actor% %actor.name% enters from below.
else
   wsend %actor% You're too large to go there.
   halt
end if
~
#1692
UNUSED~
2 b 100
~
Nothing.
~
$~
