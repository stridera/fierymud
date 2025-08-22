#16101
Gothra_Old_Man_rece~
0 j 100
~
* This is the script that give the prize for the effort
if %object.vnum% == 2023
   wait 1
   mjunk bracelet
   shout Woo Hoo!
   say YES!
   thanks %actor.name%
   say I may actually get out of the dog house now!
   grin me
   wait 3
   eye %actor.name%
   say Hey, if you are resourceful enough to find this, then maybe you are up for some adventure!
   grin
   mload obj 16104
   give lever %actor.name%
   say Good luck...
   set person %actor%
   set i %person.group_size%
   if %i%
      set a 1
   else
      set a 0
   endif
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if !%person.quest_stage[desert_quest]%
            quest start desert_quest %person.name%
         endif
         quest variable desert_quest %person.name% lever 1
      elseif %person%
         eval i %i% + 1 
      endif
      eval a %a% + 1
   done
endif
~
#16102
Gothra_Old_Man_speech1~
0 d 100
precious~
sigh
mechoaround %actor% An old man speaks to %actor.name% in a low voice.
msend %actor% An old man whispers to you 'Yes, during our battles a gift from my dear wife was lost.'
wince
wait 1
mechoaround %actor% An old man speaks to %actor.name% in a low voice.
msend %actor% An old man whispers to you 'She wasn't happy, she's quite the rogue and the bracelet was an heirloom.'
frown
msend %actor% An old man whispers to you 'If I only had it back....'
~
#16103
Gothra_Old_Man_speech2~
0 d 100
scorpion~
grin %actor.name%
say Oh yea...
smile man
mechoaround %actor% An old man speaks to %actor.name% in a low voice.
msend %actor% An old man whispers to you 'I was able to trick the vile beast and trap it.'
wait 1
msend %actor% An old man whispers to you 'It was a bit too mighty for me and my men to actually slay.'
sigh
mechoaround %actor% An old man speaks to %actor.name% in a low voice.
msend %actor% An old man whispers to you 'The battle was quite brutal and I lost something precious.'
frown
~
#16104
Gothra_Old_Man_speech3~
0 d 100
trouble~
nod %actor.name%
say Yes I said trouble, kids today, why when I was your age I was taming the realms! And I even caught me a giant scorpion!
nod man
~
#16105
Gothra_Old_Man_greet~
0 g 100
~
wait 1
eye
msend %actor% An old man rubs his chin and ponders about you.
msend %actor% An old man says 'hrmff probably some adventurer looking for trouble.'
~
#16106
Scorpion desert quest death~
0 f 100
~
set person %actor%
set i %person.group_size%
if %i%
   set a 1
else
   set a 0
endif
while %i% >= %a%
   set person %actor.group_member[%a%]%
   if %person.room% == %self.room%
      if %person.quest_stage[desert_quest]%
         quest variable desert_quest %person.name% scorpion 1
      endif
   elseif %person%
      eval i %i% + 1
   endif
   eval a %a% + 1
done
~
$~
