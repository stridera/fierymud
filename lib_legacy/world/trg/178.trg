#17800
fetch_is_dead~
0 f 100
~
mteleport %actor% 17868
msend %actor% Your vision suddenly blurs, and you find yourself back at the shaman.
mforce %actor% look
if !%get.mob_count[17806]%
   mat 17868 mload mob 17806
endif
mat 17868 mforce shaman say Well done, %actor.name%!
mat 17868 mforce shaman mload obj 17809
mat 17868 mforce shaman say You have passed the test and earned this.
mat 17868 mforce shaman give mask-self-knowledge %actor.name%
~
#17801
shaman_speak1~
0 d 0
master let the test begin~
if %self.fighting%
   say Your test may begin when mine has finished!
else
   if %get.mob_count[17811]%
      say I'm sorry.  I'm already helping someone else face their fears.
      say You may try when they are finished.
   else
      say It is your right to face your fears.
      say Let us hope you have the power to overcome them.
      wait 1s
      msend %actor% Everything blurs for a second as the shaman gestures.
      mteleport %actor% 17869
      emote gestures, and %actor.name% disappears in a blur.
      mat 17869 mforce %actor% look
      wait 1s
      msend %actor% &b%self.name% tells you, 'If you need to leave, my apprentice will show you the way out.'&0
      mat 17872 mload mob 17811
      if !%get.mob_count[17812]%
         mat 17872 mload mob 17812
      endif
   endif
endif
~
#17802
shaman_speak2~
0 d 100
yes fear?~
say This is no easy test, and many have failed.
sigh
wait 1s
say My last student still hasn't returned, but I know he is not yet dead.
consider %actor.name%
if %actor.level% < 18
   say I strongly suggest that you do not attempt this test yet.
   wait 1s
endif
say If you are certain you wish to continue, then say "Master let the test begin".
~
#17803
shaman_greet1~
0 g 100
~
*Add: Kourrya 6-06 for the troll mask quest in Minithawkin
if %actor.race% /= troll
  mjunk red-dye
  mload obj 37081
end
eval minlevel (%wandstep% - 1) * 10
wait 2
if %actor.quest_stage[%type%_wand]% == %wandstep% && %actor.level% >= %minlevel%
  if %actor.quest_variable[%type%_wand:greet]% == 0
    mecho %self.name% says, 'I see you're crafting something.  If you want my help, we can talk about &6&b[upgrades]&0.'
  else
    say Do you have what I need?
  endif
else
  say Have you come to face your greatest fear?
endif
~
#17804
apprentice_speak1~
0 d 0
i have failed my quest~
comfort %actor.name%
say Maybe another time you will succeed.
emote utters the words 'rednes ot nruter'.
mteleport %actor.name% 17868
mat fetch-apparition mpurge fetch-apparition
mforce %actor% look
~
#17805
apprentice_speak2~
0 d 100
help~
say If you want to return to the shaman say 'I have failed my quest'.
wait 1
say Then I will return you to the shaman.
~
#17806
apprentice_greet1~
0 g 75
~
if %actor.vnum% == -1
   sigh
   emote mumbles to himself about completing his training.
   say If you need help, just ask.
else
   flee
endif
~
#17813
burning_frozen_bush_rand~
0 bg 15
~
mecho Pitiful wails of pain scream from the burning part of the shrub.
wait 2
emote moans as best as shrubs can moan.
~
$~
