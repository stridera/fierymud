#47001
banshee_shriek_greet~
0 g 80
~
if %actor.level% < 100
wait 4
msend %actor% %self.name% shrieks in utter &3&bterror&0 at your entrance!
mechoaround %actor% %self.name% shrieks in utter &3&bterror&0 as %actor.name% enters!
endif
~
#47002
spectral-wife_receive~
0 j 100
~
* this is a comment
if %object.vnum% == 47019
   If %actor.vnum% == -1
      if %actor.level% > 99
         wait 1
         msend %actor% %self.name% says, '&3&bSilly immortal, tricks are for kids!&0'
         give broken-wedding-ring %actor.name%
      else
         wait 1
         mecho %self.name% says, '&3&bOh my poor husband!  What have you done to him?!?&0'
         remove ethereal-ring-undead
         unlock door south
         open door south
         mechoabout %actor% %self.name% rears back in rage and %actor.name% stumbles towards the south!
         msend %actor% %self.name% rears back in rage and you stumble towards the south!
         mforce %actor% south
         stand
         south
         close door north
         lock door north
         close folding-doors east
         wear ethereal-ring-undead
         mecho %self.name% screams in tremendous rage, '&3&bI will avenge my husband's death!!&0'
         kill %actor.name%
      end
   else
   end
else
end
~
#47003
spectral-wife_greet~
0 h 100
~
wait 1
msend %actor% %self.name% says to you, '&3&bGood day my friend.  It is not often I see'
msend %actor% '&3&bcity folk in these parts.  Won't you stay and talk?&0'
~
#47075
GY_newbie_guard~
0 c 100
south~
if %actor.vnum% == -1
   if %actor.level% < 30
      return 1
      msend %actor% %self.name% places a hand in front of you.
      mechoaround %actor% %self.name% places a hand up in front of %actor.name%.
      whisper %actor.name% Hold on there!  South of here is terribly dangerous for someone of your skill.
      wait 1s
      whisper %actor.name% I suggest adventuring elsewhere for now.
      bow %actor.name%
   else
      return 0
   endif
else
   return 0
endif
~
$~
