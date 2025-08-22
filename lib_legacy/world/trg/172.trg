#17200
Grand Master responds to 'guild'~
0 d 100
guild~
wait 1 s
if %actor.class% /= Sorcerer
  eyebrow %actor.name%
  wait 3 s
  msend %actor% %self.name% says, 'I know and you know that it's right behind Bigby's shop!'
  wait 4 s
  msend %actor% %self.name% says, 'Now stop wasting my time.'
elseif %actor.class% /= Cryomancer
  msend %actor% %self.name% says, 'Your guild?  Ickle, perhaps?'
  wait 4 s
  msend %actor% %self.name% says, 'I'm impressed that you haven't melted into a puddle from being so far south.'
elseif %actor.class% /= Pyromancer
  msend %actor% %self.name% says, 'Where's your guild?  Probably somewhere hot, I should say?'
  wait 3 s
  chuckle grand
  wait 3 s
  msend %actor% %self.name% says, 'Sorry, I've been shut up in this tower far too long!'
elseif %actor.class% /= Illusionist
  msend %actor% %self.name% says, 'Yes, you'll be wanting to visit your guild.'
  wait 3 s
  nod %actor.name%
  wait 3 s
  msend %actor% %self.name% says, 'I haven't visited Mielikki in ages, but there is certainly one there for you.  But as to where...'
  wait 3 s
  sigh
  wait 3 s
  msend %actor% %self.name% says, 'The best person to ask would be the Archmage.  She really ought to keep track of these things.'
else
  sigh
  wait 3 s
  msend %actor% %self.name% says, 'Look, I'm no good at these sorts of questions.'
  wait 4 s
  msend %actor% %self.name% says, 'Guilds are usually found near centers of commerce.'
  wait 4 s
  msend %actor% %self.name% says, 'You're the adventurer, find it yourself!'
end
~
#17201
Ill-subclass: Grand Master greeting~
0 g 100
~
* switch case is inverted with case 0 starting and all cases after 2 as default
switch %actor.quest_stage[illusionist_subclass]%
  case 0
    if %actor.class% /= Sorcerer
      switch %actor.race%
*        case ADD RESTRICTED RACES HERE
*          break
        default
          if %actor.level% >= 10 && %actor.level% <= 45
            wait 1s
            msend %actor% %self.name% says, 'Hello, %actor.name%.'
            wait 2s
            msend %actor% %self.name% says, 'I'm looking for bright young sorcerers like yourself to become illusionists.  But I need evidence of your dedication...  And resourcefulness.'
            wait 3s
            msend %actor% %self.name% says, 'Do you have time to do a small favor for me?'
          endif
      done
    endif
  case 1
    * Don't do anything - they're on their way to the hideout, and might be
    * slightly confused as to the best way out of the Citadel, or just cleaning
    * up here on the top floor.
    break
  default
    if !%actor.has_completed[illusionist_subclass]%
      * Quester dropped the vial, but wasn't in the Smuggler leader's room when
      * the invasion illusion started.  Quester needs help.
      wait 1s
      smile %actor.name%
      wait 1s
      msend %actor% %self.name% says, 'Quest going well, I hope?  Say &6&b'help'&0 if you're having any trouble.'
    endif
done
~
#17202
Ill-subclass: Grand Master's exposition, part 1~
0 d 100
yes y ok okay sure favor? what? favor what~
switch %actor.quest_stage[illusionist_subclass]%
  case 0
    if %actor.class% /= Sorcerer
      switch %actor.race%
*        case ADD RESTRICTED RACES HERE
*          if %actor.level% >= 10 && %actor.level% <= 45
*            msend %actor% &1%Your race may not subclass to illusionist.&0
*          endif
*          break
        default
          wait 1s
          if %actor.level% >= 10 && %actor.level% <= 45
            msend %actor% %self.name% says, 'Are you ready to hear the story?  Well, here it comes!'
            wait 4s
            msend %actor% %self.name% says, 'When you feel that you're ready, you can say &6&b'begin'&0.  Then I'll outfit you for this quest.  But for now, listen carefully.'
            wait 5s
            msend %actor% %self.name% says, 'This is what I need you to do...'
            wait 3s
            msend %actor% %self.name% says, 'That ruffian in the smuggler's hideout once stole a woman from me, on the very eve of our betrothal announcement.  Or my pride, perhaps.  It appears that she was his from the very beginning.'
            wait 7s
            msend %actor% %self.name% says, 'She has departed, she is lost, my dear Cestia...  She boarded a ship for the southern seas and never returned.  I fear she is dead.  But she may have survived, and on that fact this plan hinges.'
            wait 7s
            msend %actor% %self.name% says, 'I gave her a valuable onyx choker as a betrothal gift.  She took it in her deceit.'
            wait 7s
            msend %actor% %self.name% says, 'The smuggler leader took the choker, and keeps it as a prize.  To humiliate me, perhaps.  And now he has raised defenses that even I cannot penetrate.'
            wait 7s
            msend %actor% %self.name% says, 'He knows my fatal weakness: hay fever.'
            wait 3s
            msend %actor% %self.name% says, 'So he has placed flowers, to reveal me should I enter his home in disguise.'
            wait 5s
            emote pauses for breath.
            wait 4s
            msend %actor% %self.name% says, 'Say &6&b'continue'&0 when you're ready to hear the rest.'
          elseif %actor.level% < 10
            msend %actor% %self.name% says, 'You're not ready to have your mind blown!'
          else
            msend %actor% %self.name% says, 'It's okay, you're too late to care.'
          endif
      done
    else
      wait 1s
      msend %actor% %self.name% says, 'Nope, not a sorcerer, can't help.'
    endif
    break
  case 1
    wait 1s
    msend %actor% %self.name% says, 'Well?  Take the &3&bvial&0 to the smuggler's hideout.  &6&bDrop it&0, and go see the leader.'
    break
  case 2
    wait 1s
    msend %actor% %self.name% says, 'Ah, foolish one.  You must be quick, once the smugglers are up in arms.  If you want another vial to try again say &6&b'begin'&0.'
    quest restart illusionist_subclass %actor.name%
    break
  default
    wait 1s
    msend %actor% %self.name% says, 'Good work!  Do you have the choker?  Please give it to me.'
done
~
#17203
Ill-subclass: Grand Master starts~
0 d 100
begin~
switch %actor.quest_stage[illusionist_subclass]%
  case 0
    if %actor.class% /= Sorcerer
      switch %actor.race%
*        case ADD RESTRICTED RACES HERE
*          if %actor.level% >= 10 && %actor.level% <= 45
*            msend %actor% &1Your race may not subclass to illusionist.&0
*          endif
*          break
        default
          if %actor.level% >= 10 && %actor.level% <= 45
            wait 1s
            msend %actor% %self.name% says, 'Good, good.  Please stand still while I enchant you so that the smugglers will see you as Cestia.'
            wait 3s
            emote utters the words, 'incognito'.
            wait 2s
            msend %actor% %self.name% says, 'Now... take this vial.'
            mload obj 17215
            give vial %actor.name%
            mjunk vial
            wait 2s
            msend %actor% %self.name% says, 'Now go to the hideout, &6&bdrop the vial&0, and quickly go to to see the leader.  Remember not to let anyone see you drop it!'
            wait 2s
            msend %actor% %self.name% says, 'Come back and ask for &6&bhelp&0 if you get stuck!'
            quest start illusionist_subclass %actor.name% Illusionist
          endif
      done
    endif
    break
  case 1
    wait 1s
    quest erase illusionist_subclass %actor.name%
    msend %actor% %self.name% says, 'Did you have some trouble?  No matter.  I will remove your disguise.'
    wait 2s
    msend %actor% %self.name% says, 'Say &6&b'begin'&0 if you want to try again.  But let me rest a moment first.'
    wait 5s
    break
  case 2
    wait 1s
    quest erase illusionist_subclass %actor.name%
    msend %actor% %self.name% says, 'Did you meet with the leader?  I hope the disguise was sufficient.'
    wait 4s
    msend %actor% %self.name% says, 'Perhaps it would be worthwhile to try again.  The smuggler leader should be searching high and low for Cestia, now.'
    wait 4s
    msend %actor% %self.name% says, 'If you are willing, I will refresh your disguise for another attempt.  Say &6&b'begin'&0 when you are ready.'
    break
  default
    wait 1s
    msend %actor% %self.name% says, 'Do you have the choker?  Did you lose it?'
    wait 3s
    sigh
    wait 2s
    msend %actor% %self.name% says, 'Very well.  If you want to try again, say &6&b'restart'&0 and I will refresh your magical disguise.'
done
~
#17204
Ill-subclass: Grand Master restarts~
0 d 100
restart~
if %actor.class% /= Sorcerer && %actor.level% > 9 && %actor.level% < 46
  if %actor.quest_stage[illusionist_subclass]% > 0
    wait 2
    msend %actor% %self.name% says, 'Very well, if you insist.'
    quest restart illusionist_subclass %actor.name%
    wait 1s
    emote rummages about in his things.
    wait 2s
    msend %actor% %self.name% says, 'Here is another vial.'
    wait 1s
    mload obj 17215
    give vial %actor.name%
    mjunk vial
    wait 2s
    msend %actor% %self.name% says, 'And don't muck it up this time!'
  endif
endif
~
#17205
Ill-subclass: Return the choker to the Grand Master~
0 j 100
17214~
if %actor.quest_stage[illusionist_subclass]% == 6
  wait 2
  mjunk choker
  wait 2
  smile %actor.name%
  wait 1s
  msend %actor% %self.name% says, 'Fantastic work!  The smugglers must be in utter disarray!  You've done well.'
  wait 3s
  msend %actor% %self.name% says, 'I can see that your skill in misdirection is extremely extremely promising.  You have my leave to join the illusionists.'
  wait 4s
  msend %actor% %self.name% says, 'Now, type &5&b'subclass'&0.'
  quest complete illusionist_subclass %actor.name%
  wait 2s
  wink %actor.name%
else
  return 0
  wait 2
  frown
  wait 2s
  emote peers closely at the choker.
  wait 3s
  msend %actor% %self.name% says, 'I'm sorry... there must be some mistake.  This isn't the choker I gave Cestia.'
  wait 1s
  msend %actor% %self.name% says, 'I'm certain of it.'
  wait 2s
  emote returns the choker.
endif
~
#17206
Ill-subclass: Drop the vial~
1 h 100
~
return 1
wait 1
if %actor.room% > 36314 && %actor.room% < 36340 && %actor.quest_stage[illusionist_subclass]% == 1
  oecho The vial breaks easily, and the small gray puff of gas quickly disperses.
  oecho As the moments pass, you sense a magical tension building.
  oecho It spreads outward as its strength grows.
  quest advance illusionist_subclass %actor.name%
  * Now check for smugglers...
  set room %get.room[%self.room%]%
  set person %room.people%
  set smuggler_found 0
  set chief_found 0
  set leader_found 0
  while %person%
    if %person.aff_flagged[blind]% || %person.stance% /= mortally || %person.stance% /= incapacitated || %person.stance% /= stunned || %person.stance% /= sleeping
      set dummy 0
    elseif %person.vnum% == 36300 || %person.vnum% == 36303 || %person.vnum% == 36304%
      set smuggler_found 1
    elseif %person.vnum% == 36306
      set chief_found 1
      set smuggler_found 1
    elseif %person.vnum% == 36301
      set leader_found 1
      set smuggler_found 1
    endif
    set person %person.next_in_room%
  done
  if %leader_found% == 1
    wait 1 s
    oforce gannigan gasp
    oforce gannigan say Cestia... what on earth are you doing?
  elseif %chief_found% == 1
    wait 1 s
    oforce chief say Hrnn?  Irksome wench!  Gannigan shall hear of this!
  elseif %smuggler_found% == 1
    wait 1 s
    oforce smuggler emote looks somewhat confused, but also suspicious.
    oforce smuggler say Hey... ummm...  I'd better let the big guy know you're up to something...  No offense ma'am, but that didn't look too innocent.
  endif
  if %smuggler_found% == 1
    quest advance illusionist_subclass %actor.name%
  endif
else
  oecho The vial breaks easily, and a small gray puff of gas quickly disperses.
  oecho A sense of magic is felt, but it quickly fades.
  oecho It appears as though something has gone wrong.
endif
opurge %self%
~
#17207
Ill-subclass: Gannigan greets the quester~
0 g 100
~
wait 2s
switch %actor.quest_stage[illusionist_subclass]%
  case 1
  case 2
    emote looks up from his paperwork briefly.
    wait 1s
    emote gasps in surprise, and rises quickly from his chair!
    wait 1s
    hug %actor.name%
    wait 3s
    say Cestia... my beloved.  Where have you been?
    break
  case 3
    emote looks up from his paperwork.  He seems angry.
    wait 1s
    say So.  Already up to your old tricks?
    wait 2s
    emote appears saddened, but angry nonetheless.
    wait 3s
    say I am aware of the spell you cast, here in my home.  Tell me, what madness awaits us?
    break
  case 4
    say Please, Cestia!  You must conceal yourself!  The forces of Mielikki will not overlook your crimes again!
    wait 3 s
    msend %actor% %self.name% looks upon you with pleading in his eyes.
    mechoaround %actor% %self.name% looks upon %actor.name% with pleading in his eyes.
    wait 2 s
    say You do remember the incantation, don't you?
    break
  case 5
    say A raid?  How did you get the townsfolk back on your side?  It would be an amazing accomplishment -
    wait 3s
    say - if not for the vileness that must lie within your heart to turn upon me so!
    break
  default
    emote looks up from his paperwork briefly.
    wait 2 s
    snort
done
~
#17208
Ill-subclass: Tell Gannigan no~
0 d 100
no~
wait 6
switch %actor.quest_stage[illusionist_subclass]%
  case 4
    msend %actor% %self.name% says, 'Of course.  Here is the incantation: &6&b"where the dough ever rises"&0.'
    wait 4s
    say Now get yourself quickly across the waterfall - The townspeople are almost upon us!
    break
  case 5
    say We'll see about that.
    break
  default
    peer %actor.name%
    wait 2s
    eyebrow
done
~
#17209
Ill-subclass: Tell Gannigan yes~
0 d 100
yes~
wait 6
switch %actor.quest_stage[illusionist_subclass]%
  case 4
    say I thought you might.  Hurry off to the safe room!
    wait 4s
    say Please, Cestia!  I'll not have you clubbed by the filthy Mielikki militia!
    break
  case 5
    snort
    wait 3s
    say I had no doubt of it.  You never knew loyalty.  I should have seen it when you hurt that illusionist so cavalierly.
    break
  default
    glare %actor.name%
    wait 2s
    ponder
done
~
#17210
Ill-subclass: Reveal the walkway~
2 d 100
where the dough ever rises~
if %actor.quest_stage[illusionist_subclass]% == 4
  wait 15
  wecho The illusion falls like a sheet of water, revealing in a moment a walkway.
  wecho It snakes eastward across the face of the falls, narrow and dangerous.
  wdoor 36339 east room 36371
  wdoor 36339 east description A narrow walkway zigzags through the air.
  wait 8s
  wecho The walkway fades, revealing nothing but a misty drop into the waters below.
  wdoor 36339 east room 36340
  wdoor 36339 east description The balcony ends at the waterfall.  Oddly, there is no railing at this end.
endif
~
#17211
Ill-subclass: Load the choker~
2 g 100
~
if %actor.quest_stage[illusionist_subclass]% == 4
   quest advance illusionist_subclass %actor.name%
   quest advance illusionist_subclass %actor.name%
   set obj %self.objects%
   set chokerpresent 0
   while %obj%
      if %obj.vnum% == 17214
         set chokerpresent 1
      end
      set obj %obj.next_in_room%
   done
   if %chokerpresent% == 0
      wload obj 17214
   end
end
~
#17212
Ill-subclass: Relay to invasion illusion~
2 i 100
trigger_17212_go~
if %actor.quest_stage[illusionist_subclass]% == 2 || %actor.quest_stage[illusionist_subclass]% == 3
wat 36336 w_run_room_trig 17213
endif
~
#17213
Ill-subclass: Invasion illusion~
2 a 100
~
wait 1s
wzoneecho 363 &5The scent of magic is discernible, as a spell builds.&0
wait 5s
wzoneecho 363 &5The magical force is still spreading, and remains low-key.&0
wait 3s
wzoneecho 363 &5A spell seems to be coming together.  Slowly, it builds.&0
wait 3s
wzoneecho 363 &5Something supernatural is beginning to coalesce - and the sounds of a militant&0&_
wzoneecho 363 &0&5crowd are beginning to rise above the threshold of hearing.&0
wait 3s
wzoneecho 363 &7&bSuddenly, a shout is heard!&0&_
wzoneecho 363 &7&bIt is joined by others, and the sounds of battle commence!&0
wait 2s
* Now, if Gannigan and the quester are in here, Gannigan should react accordingly.
set gannigan 0
set quester 0
set person %self.people%
while %person%
  if %person.vnum% == 36301
    set gannigan %person%
  elseif %person.vnum% == -1 && %person.quest_stage[illusionist_subclass]% > 1
    set quester %person%
  endif
  set person %person.next_in_room%
done
if %quester% && %gannigan%
  if %quester.quest_stage[illusionist_subclass]% == 2
    wforce %gannigan% say What?!  They attack?  Can Mielikki have gone mad?
    wait 2s
    wforce %gannigan% say Cestia, you must hide!
    wait 2s
    wforce %gannigan% say Do you recall the incantation?  The one that will reveal the passage above the falls?
    quest advance illusionist_subclass %quester.name%
    quest advance illusionist_subclass %quester.name%
  elseif %quester.quest_stage[illusionist_subclass]% == 3
    wforce %gannigan% say An attack?  Cestia!  What have you done?  It is betrayal!
    wait 2s
    wforce %gannigan% glare %quester.name%
    wait 2s
    wforce %gannigan% say You will not destroy me.
    wait 2s
    wforce %gannigan% emote raises his sword in anger!
    wait 1s
    wforce %gannigan% emote hesitates.
    wait 3s
    wforce %gannigan% say But I cannot bring myself to raise my sword against you.
    quest advance illusionist_subclass %quester.name%
    quest advance illusionist_subclass %quester.name%
  endif
endif
~
#17214
Ill-subclass: Grand Master answers 'help'~
0 d 100
help~
wait 1s
switch %actor.quest_stage[illusionist_subclass]%
  case 1
    msend %actor% %self.name% says, 'Ok, ok, here's what you need to do: Go to the smuggler's hideout, &6&bdrop the vial&0, and go to see the leader.'
    wait 4s
    msend %actor% %self.name% says, 'Don't let anyone see you drop the vial!'
    wait 4s
    msend %actor% %self.name% says, 'And get that choker!  Now go.'
    break
  case 2
  case 4
    msend %actor% %self.name% says, 'Hmm.  Did things get confusing?  Say &6&b'restart'&0 if you want to try again.'
    break
  case 3
  case 5
    msend %actor% %self.name% says, 'Someone saw you drop the vial?
    wait 3s
    shake
    wait 3s
    msend %actor% %self.name% says, 'You'll never fool them that way!'
    wait 3s
    msend %actor% %self.name% says, 'Give it some time.  Then come back here and say &6&b'restart'&0 to try again.'
    break
  case 6
    msend %actor% %self.name% says, 'You got the choker, you say?  Hand it over!'
    break
  default
  case 0
    msend %actor% %self.name% says, 'Help you?  Something the matter?'
    wait 3s
    msend %actor% %self.name% says, 'Psyche!  You don't work for me, I wasn't really concerned.'
    break
done
~
#17215
Ill-subclass: Grand Master responds to 'hi'~
0 d 100
hi hello howdy~
switch %actor.quest_stage[illusionist_subclass]%
  case 0
    if %actor.class% /= Sorcerer
      switch %actor.race%
*        case ADD RESTRICTED RACES HERE
*          if %actor.level% >= 10 && %actor.level% <= 45
*            msend %actor% &1Your race may not subclass to illusionist.&0
*          endif
*          break
        default
          wait 1s
          if %actor.level% >= 10 && %actor.level% <= 45
            msend %actor% %self.name% says, 'Well, hello there.'
            wait 3s
            msend %actor% %self.name% says, 'Do you have time to do me the very tiniest of favors?'
          elseif %actor.level% >= 10
            msend %actor% %self.name% says, 'Nope, too small, next!'
          else
            msend %actor% %self.name% says, 'Nope, too unrefined, next!'
          endif
      done
    endif
    break
  default
    msend %actor% %self.name% says, 'Quest going well, I hope?'
    wait 3s
    msend %actor% %self.name% says, 'Just say &6&b'help'&0 if you get stuck.'
    break
done
~
#17216
Ill-subclass: Grand Master's exposition, part 2~
0 d 100
continue~
switch %actor.quest_stage[illusionist_subclass]%
  case 0
    if %actor.class% /= Sorcerer
      switch %actor.race%
*        case ADD RESTRICTED RACES HERE
*          if %actor.level% >= 10 && %actor.level% <= 45
*            msend %actor% &1Your race may not subclass to illusionist.&0
*          endif
*          break
        default
          if %actor.level% >= 10 && %actor.level% <= 45
            wait 1s
            msend %actor% %self.name% says, 'This is where you come in.'
            wait 2s
            msend %actor% %self.name% says, 'I would like you to enter the hideout in disguise and take back the choker.  But beware: it is well hidden.'
            wait 5s
            msend %actor% %self.name% says, 'Somehow that brutish fool has engaged the services of an illusionist... I would like to know who helped him... but no matter.'
            wait 5s
            msend %actor% %self.name% says, 'I will enchant you to resemble dear Cestia when the smugglers look upon you.  Their leader will no doubt welcome you with open arms.'
            wait 7s
            msend %actor% %self.name% says, 'But we must ensure that he will reveal its hiding place to you.  For that, we will make it appear as if the guards of Mielikki have discovered their hideout.'
            wait 7s
            msend %actor% %self.name% says, 'It is my hope that the leader will hide you - Cestia - for safekeeping, in his most secure location.'
            wait 5s
            msend %actor% %self.name% says, 'There, perhaps, you will find the choker.'
            wait 4s
            msend %actor% %self.name% says, 'I will give you a vial of disturbance.  Drop it and once you find their leader Gannigan sounds of shouting and fighting will resonate throughout the area.  It will convince the smugglers that they are under attack.'
            wait 7s
            msend %actor% %self.name% says, 'You must take care to drop it out of sight of any smugglers, or they may become suspicious.  Then go immediately to the leader, and stall him until you hear the sounds of invasion.'
            wait 7s
            msend %actor% %self.name% says, 'Well, what say you?  Are you up for it?  Say &6&b'begin'&0 if you are ready!'
          endif
      done
    endif
    break
  default
    wait 1s
    msend %actor% %self.name% says, 'Continue what?  Why aren't you infiltrating the hideout?'
    wait 3s
    ponder %actor.name%
    wait 3s
    msend %actor% %self.name% says, 'Say &6&b'restart'&0 if you've gotten stuck.'
done
~
#17217
Ill-subclass: Smugglers react to quester~
0 g 10
~
switch %actor.quest_stage[illusionist_subclass]%
  case 0
    break
  case 1
  case 2
    wait 2 s
    gasp
    wait 2 s
    say Cestia!  How nice it is to see you again.
    wait 3 s
    smile %actor.name%
    break
  case 3
    wait 2 s
    look %actor.name%
    wait 3 s
    ponder
    break
  case 4
    wait 1 s
    emote looks rather worried.
    wait 3 s
    say I hope they don't make it this far.  I'm not that good at fighting!
    break
  case 5
    wait 1 s
    glare %actor.name%
    wait 2 s
    say Cestia, I heard you brought a posse from Mielikki with you!
    wait 4 s
    say We ought to offer you up to them!
    break
  case 6
    wait 1 s
    say Aha!  If it isn't the little thief herself!
    wait 4
    kill %actor.name%
done
~
#17218
Ill-subclass: Flowers make people sneeze~
2 g 5
~
if %actor.class% /= Illusionist
   wait 1
else
   wait 2 s
   set num %random.6%
   switch %num%
      case 1
         wforce %actor% sneeze
         wait 3 s
         wforce %actor% sneeze
         break
      case 2
      case 3
         wforce %actor% sneeze
         break
      case 4
         wforce %actor% emore blinks heavily, trying to clear the tears out of %actor.hisher% eyes.
         break
      case 5
         wsend %actor% Your nose feels all itchy.
         break
      default
         wforce %actor% emote wipes %actor.hisher% nose with %actor.hisher% sleeve.
   done
   * See if any smugglers are nearby to witness
   set smuggler_here 0
   set smuggler 0
   set person %self.people%
   while %person%
      if %person.vnum% == 36300
         set smuggler_here 1
         set smuggler %person%
      elseif %person.vnum% == 36301
         set smuggler_here 1
         set smuggler %person%
      elseif %person.vnum% == 36303
         set smuggler_here 1
         set smuggler %person%
      elseif %person.vnum% == 36304
         set smuggler_here 1
         set smuggler %person%
      elseif %person.vnum% == 36306
         set smuggler_here 1
         set smuggler %person%
      end
      set person %person.next_in_room%
   done
   if %smuggler_here% == 1
      set num %random.5%
      switch %num%
         case 1
            wsend %actor% You notice the smuggler paying close attention to you.
            break
         case 2
            wforce %smuggler% peer %actor.name%
            break
         case 3
            wforce %smuggler% look %actor.name%
            break
         case 4
            wforce %smuggler% emote smiles sweetly.
            wait 2 s
            wforce %smuggler% emote asks, 'Is everything alright?'
            break
         default
            wforce %smuggler% consider %actor.name%
      done
   end
end
~
#17219
Grand Master refuse~
0 j 0
17214~
switch %object.vnum%
  default
    return 0
    wait 1s
    msend %actor% %self.name% says, 'Eh?  Err... no thank you.'
    wait 2s
    mechoto %actor% %self.name% returns your gift.
    mechoaround %actor% %self.name% refuses to accept %actor.name%'s gift.
done
~
$~
