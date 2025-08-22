#50201
Ghostly Diplomat Earring Quest~
0 j 100
~
switch %object.vnum%
   case 50203
      * The ghost of a diplomat has received the magical golden
      * earring. He knows of this talisman from his studies and
      * travels. He knows how to make it much more powerful. Now
      * that he has it, he will do so. However, the process will
      * drive him mad. The player's job will then be to find
      * him (with the help of the quiet ranger) and retrieve the
      * enhanced earring (now a crimson-tinged electrum hoop).
      wait 2
      mpurge %object%
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
            if !%person.quest_stage[bayou_quest]%
               quest start bayou_quest %person.name%
            endif
         elseif %person%
            eval i %i% + 1
         endif
         eval a %a% + 1
      done
      thank %actor.name%
      emote gazes upon the earring, overwhelmed by its beauty.
      mecho %self.name% says, 'You fool!  You should never have given it up so easily!'
      emote dashes out of the room.
      mteleport %self% 50243
      if %get.mob_count[50209]% < 1
         mload mob 50209
         remove all
         give all maddened-diplomat-spectre
         mforce maddened-diplomat-spectre wear all
         mforce maddened-diplomat-spectre d
      endif
      mpurge %self%
      break
   case 50201
   case 50202
      * China from Odz
      mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
      msend %actor% You give %object.shortdesc% to %self.name%.
      return 0
      wait 1 s
      emote looks over %object.shortdesc% fondly.
      wait 2 s
      mecho %self.name% says, 'Attractive, is it not?  A gift from our island friends.'
      wait 1 s
      mecho %self.name% says, 'But alas, it has no intrinsic magic that I can detect.'
      emote politely returns the item.
      break
   case 50204
   case 50209
      * Magical eq from bayou
      mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
      msend %actor% You give %object.shortdesc% to %self.name%.
      return 0
      wait 1 s
      emote stares closely at %object.shortdesc% for a time.
      wait 4 s
      sigh
      wait 2 s
      mecho %self.name% says, 'There is something here, but this is far too weak.  No, the captain was after something much more valuable.'
      wait 1 s
      emote returns the item.
      break
   case 50215
      * The electrum hoop!
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
            if !%person.quest_stage[bayou_quest]%
               quest start bayou_quest %person.name%
            endif
         elseif %person%
            eval i %i% + 1
         endif
         eval a %a% + 1
      done
      wait 2
      mjunk %object%
      if %get.mob_count[50209]% < 1
         mat 50243 mload mob 50209
         mat 50243 remove all
         mat 50243 give all maddened-diplomat-spectre
         mat 50243 mforce maddened-diplomat-spectre wear all
      endif
      wait 1 s
      gasp
      wait 1 s
      say What... how... I cannot believe it!
      wait 2 s
      mecho %self.name% says, 'And you willingly gave it to me!  Oh the gods of Garl'lixxil smile upon me this day!'
      wait 1 s
      mecho The ghost of a diplomat cackles, and seems to grow before your very eyes!
      * Now: move dip to secret room; load maddip; give earring maddip; purge dip; bring maddip back
      eval startroom %self.room%
      mteleport maddened-diplomat-spectre %startroom%
      wait 1
      mpurge %self%
      break
   default
      mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
      msend %actor% You give %object.shortdesc% to %self.name%.
      return 0
      wait 6
      laugh
      say I'm afraid not.
      wait 8
      emote politely returns the item.
done
~
#50202
Diplomat greet~
0 g 100
~
if %actor.vnum% == -1 && %actor.level% < 100
   wait 2
   msend %actor% The ghost of a diplomat stares at you for a moment.
   mechoaround %actor% The ghost of a diplomat stares at %actor.name%.
   wait 4
   mecho %self.name% says, 'Please forgive my rudeness.  It has been some
   mecho &0time since I was in the company of the living.'
   sigh
   wait 3 s
   say Since the accident...
end
~
#50203
Diplomat replies to 'accident'~
0 d 100
accident accident?~
wait 2 s
mecho %self.name% says, 'Yes, the horrible storm that wrecked our ships.
mecho &0It was horrendous and unnatural.  I have long wondered what evil magic
mecho &0spawned it, for the seas of Ethilien could scarce have birthed such a monster
mecho &0on their own.'
wait 6 s
emote looks pensive.
wait 4 s
mecho %self.name% says, 'After our ships wrecked, the captain was
mecho &0obsessed with finding some magical bauble or other -- he wouldn't say what,
mecho &0exactly -- and though he was fairly unscathed, he would render little help
mecho &0to the injured around him.  He disappeared soon after, leaving the rest of
mecho &0us to die.'
wait 6 s
sigh
wait 4 s
mecho %self.name% says, 'If only I knew what it was, perhaps I could
mecho &0free us from this unnatural bondage.  If you come across such a thing, would
mecho &0you be so kind as to allow me to examine it?'
~
#50204
Diplomat quest: ranger receiving things~
0 j 100
~
eval avnum %actor.vnum%
wait 1
if %object.vnum% == 50218
   wait 1
   if %avnum% == 50201
      set mdiplomat 1
      global mdiplomat
   elseif %avnum% == 50209
      set mdiplomat 0
      global mdiplomat
   endif
   mjunk diplomat-token
end
~
#50205
Ranger responds to yes~
0 d 100
yes~
wait 6
if %get.mob_count[50209]% < 1
   peer %actor.name%
else
   say Well, a ghostly fellow ran by here just a moment ago.
   m_run_room_trig 50206
endif
~
#50206
Ranger makes path to maddened diplomat~
2 a 100
~
wdoor 50216 south room 50242
wdoor 50216 south description A faint path leads through the thicket.
wait 2 s
wecho The quiet ranger points out a hidden trail heading south.
wait 20 s
wecho A gentle breeze disturbs the branches, and the path is no longer visible.
wdoor 50216 south purge
~
#50207
Spectre death~
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
      if %person.quest_stage[bayou_quest]% && !%person.has_completed[bayou_quest]%
         quest complete bayou_quest %person.name%
      endif
   elseif %person%
      eval i %i% + 1
   endif
   eval a %a% + 1
done
~
#50208
Ghost Pirate Captain greet~
0 g 100
~
wait 3
if %actor.level% < 100
  msend %actor% &b%self.name% tells you, 'Did you take my earring?  What have you&0
  msend %actor% &0&bdone with it, rapscallion!'&0
end
~
#50209
Diplomat responds to 'yes'~
0 d 100
yes~
wait 4
smile
say Then we would forever be in your debt.
~
#50210
Ranger greetings~
0 g 100
~
if %actor.vnum% == -1 && %actor.level% < 100
   wait 6
   if %get.mob_count[50209]% < 1
      emote discreetly takes note of your passage.
      wait 3 s
      emote notices you looking at him, and bows before you with a wry grin.
   else
      emote watches you with some interest.
      wait 2 s
      say Looking for someone?
   end
end
~
#50211
Ranger responds to hello~
0 d 100
hi hello~
if %actor.vnum% == -1 && %actor.level% < 100
   wait 6
   bow %actor.name%
   wait 2 s
   say Have a care in these swamps, little one, for evil is afoot.
end
~
#50212
Spectre load~
0 o 100
~
mload obj 50215
wear all
~
#50213
**UNUSED**~
0 o 100
~
mload obj 50215
wear all
~
$~
