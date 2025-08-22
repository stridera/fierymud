#48400
keeper-keys-speech~
0 h 100
~
msend %actor% %self.name% eyes you mercilessly.
mechoaround %actor% %self.name% eyes %actor.name% mercilessly.
wait 1s
if %actor.level% < 85
  msend %actor% %self.name% says to you, 'You are far too weak to seek entry into
  msend %actor% &0the elemental planes.  Come back when you are stronger.'  
  mechoaround %actor% %self.name% says to %actor.name%, 'You are far too weak to seek
  mechoaround %actor% &0entry into the elemental planes.  Come back when you are stronger.'
elseif %actor.quest_stage[doom_entrance]% < 1
  mecho %self.name% says, 'So, %actor.name%, you wish to seek entry into
  mecho &0Lokari's Keep?'
elseif %actor.has_completed[doom_entrance]%
  mecho %self.name% says, 'Well done, %actor.name%!
  mecho &0Please take this key and with it our hopes.'
  mload obj 48401
  give sacred-key %actor.name%
  bow
  say Use it well!
end
~
#48401
oracle-sun-greet~
0 h 100
~
wait 2
if %wandstep%
  eval minlevel (%wandstep% - 1) * 10
elseif %macestep%
  eval minlevel %macestep% * 10
endif
if %actor.quest_stage[%type%_wand]% == %wandstep% && %actor.quest_stage[doom_entrance]% != 5 && %actor.quest_stage[doom_entrance]% != 6 && !%actor.has_completed[doom_entrance]%
  if %actor.level% >= %minlevel%
    if %actor.quest_variable[%type%_wand:greet]% == 0
      msend %actor% %self.name% says, 'Timun, God of the Sun, sees you are progressing toward divine fire.  Tell us what you are working on.'
    else
      msend %actor% %self.name% says, 'Do you have what I require to craft your staff?'
    endif
  endif
elseif %actor.quest_stage[phase_mace]% == %macestep% && %actor.quest_stage[doom_entrance]% != 5 && %actor.quest_stage[doom_entrance]% != 6 && !%actor.has_completed[doom_entrance]%
  if %actor.level% >= %minlevel%
    if %actor.quest_variable[phase_mace:greet]% == 0
      msend %actor% %self.name% says, 'I sense a ghostly presence about your weapons.  If you want my help, we can talk about &6&b[upgrades]&0.'
    else
      msend %actor% %self.name% says, 'Do you have what I need?'
    endif
  endif
elseif %actor.quest_stage[doom_entrance]% == 5
  bow %actor.name%
  wait 1s
  msend %actor% %self.name% says, 'At last, one has come to challenge the wrath of the God of the Moonless Night.'
  wait 1s
  msend %actor% %self.name% says, 'You shall need Timun's Light if you are to defeat Lokari, %actor.name%.'
  mload obj 48431
  give vial %actor.name%
  wait 1s
  msend %actor% %self.name% says, 'You must take this vial to a place steeped in darkness.  You shall know it by the false light shed there and the mockery of the sun it proclaims.'
  wait 2s
  msend %actor% %self.name% says, 'Then drop it on the ground and fill the place with holy light!  Once you have completed this task, return to me and I shall inform the Keeper of the Keys that you are indeed worthy.'
  wait 4s
  if %actor.quest_stage[%type%_wand]% == %wandstep%
    if %actor.level% >= %minlevel%
      if %actor.quest_variable[%type%_wand:greet]% == 0
        msend %actor% %self.name% says, 'Additionally Timun, God of the Sun, sees you are progressing toward divine fire.  Tell us what you are working on.'
        wait 1s
      else
        msend %actor% %self.name% says, 'Do you have what I require to craft your staff?'
        wait 1s
      endif
    endif
  elseif %actor.quest_stage[phase_mace]% == %macestep%
    if %actor.level% >= %minlevel%
      if %actor.quest_variable[phase_mace:greet]% == 0
        msend %actor% %self.name% says, 'Timun sees you also seek His lambent blessing to repel the forces of the dead.  Tell us what you are working on.'
        wait 1s
      else
        msend %actor% %self.name% says, 'Do you have what I require to bless your mace?'
        wait 1s
      endif
    endif
  endif
  msend %actor% With those words, the Oracle bows his head and refuses to utter another word.
elseif %actor.quest_stage[doom_entrance]% == 6
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
      if %person.quest_stage[doom_entrance]% == 6
        msend %person% %self.name% says, 'You have done well, %person.name%. Your task is completed.'
        wait 1s
        msend %person% %self.name% says, 'May the light of the Sun find you even in the depths of Severan's Doom.'
        quest complete doom_entrance %person.name%
        wait 1s
        msend %person% %self.name% says, 'Do not forget to show the Keeper of the Keys that you have finished!'
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
  mload obj 48421
  give key %actor.name%
  if %actor.quest_stage[%type%_wand]% == %wandstep%
    eval minlevel (%wandstep% - 1) * 10
    if %actor.level% >= %minlevel%
      wait 3s
      if %actor.quest_variable[%type%_wand:greet]% == 0
        msend %actor% %self.name% says, 'Additionally Timun, God of the Sun, sees you are progressing toward divine fire.  Tell us what you are working on.'
        wait 1s
      else
        msend %actor% %self.name% says, 'Do you have what I require to craft your staff?'
        wait 1s
      endif
    endif
  elseif %actor.quest_stage[phase_mace]% == %macestep%
    if %actor.level% >= %minlevel%
      if %actor.quest_variable[phase_mace:greet]% == 0
        msend %actor% %self.name% says, 'Timun sees you also seek His lambent blessing to repel the forces of the dead.  Tell us what you are working on.'
        wait 1s
      else
        msend %actor% %self.name% says, 'Do you have what I require to bless your mace?'
        wait 1s
      endif
    endif
  endif
elseif %actor.has_completed[doom_entrance]%
  msend %actor% As you enter the chamber %self.name% greets you warmly.
  mechoaround %actor% %self.name% greets %actor.name% warmly.
  wait 1s
  msend %actor% %self.name% says, 'Heroes, the day of Prophecy has arrived.  Know that in a nearby chamber is a Gateway to another Realm.  From that Realm, you will have access to others; and it is in one of these other Realms that you will find the way to Lokari's Domain.'
  wait 1s
  msend %actor% %self.name% says, 'Collect the three godly keys to the Domain, which were scattered by the gods upon the Champion's death, and slay the mad god for his crimes.'
  wait 1s
  msend %actor% %self.name% says, 'Go now, the Universe awaits!'
  wait 1s
  if %actor.quest_stage[%type%_wand]% == %wandstep%
    eval minlevel (%wandstep% - 1) * 10
    if %actor.level% >= %minlevel%
      if %actor.quest_variable[%type%_wand:greet]% == 0
        msend %actor% %self.name% says, 'Additionally Timun, God of the Sun, sees you are progressing toward divine fire.  Tell us what you are working on.'
        wait 1s
      else
        msend %actor% %self.name% says, 'Do you have what I require to craft your staff?'
        wait 1s
      endif
    elseif %actor.quest_stage[phase_mace]% == %macestep%
      if %actor.level% >= %minlevel%
        if %actor.quest_variable[phase_mace:greet]% == 0
          msend %actor% %self.name% says, 'Timun sees you also seek His lambent blessing to repel the forces of the dead.  Tell us what you are working on.'
          wait 1s
        else
          msend %actor% %self.name% says, 'Do you have what I require to bless your mace?'
          wait 1s
        endif
      endif
    endif
  endif
  msend %actor% With those words, the Oracle bows his head and refuses to utter another word.
endif
~
#48402
justice-oracle-greet~
0 h 100
~
wait 2
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
    *
    * Added by Daedela 3-2021 for Wizard Eye
    *
    if %person.quest_stage[wizard_eye]% == 9
      quest advance wizard_eye %person.name%
      msend %person% %self.name% says, 'I have foreseen this moment and anticipated your arrival.  If you seek my guidance, present to me the crystals in question.'
    else
      if %person.quest_stage[doom_entrance]% == 3
        msend %person% %self.name% says, 'Greetings, %person.name%.'
        wait 2s
        msend %person% %self.name% says, 'It has come to my attention that you seek to prove your worth to Rhalean.  As the goddess of Justice, Rhalean has but one foe: her evil sister, trapped underground in the ages before the Riftwars.'
        wait 1s
        msend %person% %self.name% says, 'To prove yourself, return here after you have slain this vile creature.'
      elseif %person.quest_stage[doom_entrance]% == 4
        quest advance doom_entrance %person.name%
        msend %person% &7&bYou have advanced the quest!&0
        msend %person% %self.name% says, 'Excellent!'
        wait 1s
        msend %person% %self.name% says, 'You have proven yourself to Rhalean, %person.name%. Please, continue on to the Oracle of the Sun.'
      else
        msend %person% As thoughts of Justice run through your mind, the Oracle clenches a calloused hand.
        wait 1s
        msend %person% %self.name% says, 'All crimes may be forgiven, save the murder of innocents.  For this sin, even the gods must take reckoning.  For this reason, you must do what the gods are prevented from doing, and slay Lokari.'
      endif
    endif
  elseif %person%
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#48403
hunt-oracle-greet~
0 h 100
~
if %actor.quest_stage[doom_entrance]% == 1
  emote looks at %actor.name% once and nods.
  mecho %self.name% says, 'To prove to Azkrael that you are worthy of
  mecho &0seeking out her traitorous brother, you shall have to find and slay her
  mecho &0favorite prey, a white hart.'
  wait 1s
  mload obj 48430
  give rag %actor.name%
  mecho %self.name% says, 'The white hart disguises itself as a
  mecho &0white-tailed deer.  But if you hold this rag in its presence, it will reveal
  mecho &0its true nature to you, %actor.name%.'
  wait 1s
  say All normal deer will simply flee.
  wait 1s
  mecho %self.name% says, 'Slay the hart and return its antlers to me
  mecho &0as proof.'
elseif %actor.quest_stage[doom_entrance]% == 2
  smile %actor.name%
  wait 1s
  mecho %self.name% says, 'Give me the antlers of the white hart,
  mecho &0%actor.name%.'
else
  wait 1s
  mecho As you look at %self.name%, you get a sense of her power
  mecho &0and majesty. This is no ordinary warrior, no ordinary person.
  wait 2s
  mecho You realize it'd be a bad idea to attack her.
  mecho And, as if reading your mind to see that you've realized this,
  mecho &0the Oracle speaks, her voice like silver.
  wait 1s
  mecho %self.name% says, 'The Goddess Azkrael, protectress of
  mecho &0huntresses and virgins, was doubly outraged when Lokari took the honor, then
  mecho &0the life, of the woman Irinia, then abused her further by forcing her spirit
  mecho &0to serve his evil whims.'
  wait 1s
  mecho %self.name% says, 'Though Lokari be Azkrael's own brother--
  mecho &0and a god-- he must be punished.'
  wait 3s
  mecho %self.name% says, 'Azkrael is forbidden to raise her hand
  mecho &0against Lokari herself, but if you will aid her, she will allow you the use
  mecho &0of the great Horn of the Hunt.  You will find it somewhere on one of the four
  mecho &0Elemental Planes.  Use it to gain entrance to Lokari's Keep, and set the ghost
  mecho &0of Irinia free.'
  wait 2s
  mecho The Oracle waits, as if expecting you to leave. She says no more.
end
~
#48404
lord-realm-spawn~
2 g 100
~
*This is for room 47658: The Heart of the Realm.
if !%lord_loaded%
 set lord_loaded 1
 global lord_loaded
 wait 1s
 wecho Before your eyes, a nondescript boulder molds itself into the image of a man of gigantic proportions.
 wload mob 48406
 wait 1s
 wecho When the transformation is complete, the elemental flexes his thick stone fingers fluidly.
 wforce lord-realm mload obj 48402
 wforce lord-realm mat 1100 hold staff-justice
 wforce lord-realm shout It has been long since I have taken this form! Beware--this time I will be the victorious one!
end
~
#48405
lord-realm-death~
0 f 100
~
m_run_room_trig 48406
~
#48406
lord-dead-unsetting-global~
2 a 100
~
set lord_loaded 0
global lord_loaded
~
#48407
ghost warrior greet~
0 g 100
~
if %actor.vnum% == -1
   wait 1s
   mecho The warrior murmurs some words...
   wait 2s
   mecho The words reach your ears as he attacks, 'Flee!  Flee! I beg you...'
endif
~
#48408
lord-realm fight~
0 k 100
~
if %has_blur%
    if !(%self.aff_flagged[BLUR]%)
       emote slows as %self.n% returns to normal speed.
       set has_blur 0
       global has_blur
    endif
endif
set victim %random.char%
if %victim%
   set action %random.10%
   if %action% > 8
      wait 2s
      mechoaround %victim% %self.name% swings a rocky fist at %victim.name%, toppling %victim.o%!
      msend %victim% %self.name% swings a rocky fist at you, knocking you over!
      msend %self% You swing a rocky fist at %victim.name%, knocking %victim.o% over!
      mteleport %victim% 1100
      mforce %victim% recline
      mteleport %victim% %self.room%
   elseif %action% > 7
      if %victim.vnum% == -1
         kill %victim.name%
      endif
   endif
endif
~
#48409
lord-realm hitprcnt 50~
0 l 25
50~
if !(%self.aff_flagged[BLIND]%) && !(%self.aff_flagged[BLUR]%)
   wait 2s
   mload obj 1159
   mat 1100 quaff blur
   mecho %self.name%'s rock formation cracks slightly...
   emote composes %self.o%self and %self.p% quickness seems to increase!
   set has_blur 1
   global has_blur
endif
~
#48414
earth elemental init~
0 o 100
~
* Teleport to another room so that other earth-elemental-leaders
* don't get in the way.
set room %self.room%
mgoto 1100
mload mob 48400
mload mob 48400
mload mob 48400
mforce earth-elemental-follower follow earth-elemental-leader
mteleport earth-elemental-follower %room%
mforce earth-elemental-follower follow earth-elemental-leader
mteleport earth-elemental-follower %room%
mforce earth-elemental-follower follow earth-elemental-leader
mteleport earth-elemental-follower %room%
mgoto %room%
~
#48415
ghost warrior init~
0 o 100
~
* Teleport to another room so that other undead-captains
* don't get in the way.
set room %self.room%
mgoto 1100
mload mob 48404
mload mob 48404
mload mob 48404
mforce ghost-warrior follow undead-captain
mteleport ghost-warrior %room%
mforce ghost-warrior follow undead-captain
mteleport ghost-warrior %room%
mforce ghost-warrior follow undead-captain
mteleport ghost-warrior %room%
mgoto %room%
~
#48416
**UNUSED**~
0 c 100
th~
return 0
~
#48417
prevent throatcut~
0 c 100
throatcut~
switch %cmd%
  case t
  case th
    return 0
    halt
done
if %arg%
  set no_throat 48400 48414 48500 48502 48503 48505 48513 48514 48515 48630 48631 48634 48803 48812 48813 48909 48919
  set undead 48404 48405 48902 48903 48905 48912 48913 48914 48915 48921 48922 48923
  set boss 48406 48504 48506 48507 48510 48632 48633 48635 48806 48809 48814 48901 48910 48911 48917 48918
  if %no_throat% /= %arg.vnum%
    return 1
    msend %actor% But %arg.name% doesn't even have a throat to cut!
  elseif %undead% /= %arg.vnum%
    return 1
    msend %actor% &1&b%arg.name% gurgles as you slice into %arg.p% throat, but there is little effect!&0 (&30&0)
    mechoaround %actor% &1&b%arg.name% looks expressionless as %actor.name% slices into %arg.p% throat!&0 (&30&0)
    mforce %arg% kill %actor.name%
  elseif %boss% /= %arg.vnum%
    return 1
    msend %actor% You just can't seem to get close enough to %arg.p% throat...
  endif
else
  return 0
endif
~
#48418
wild-hunt-rag-held~
0 ah 100
~
*Checks to see if the actor is indeed holding the rag like they should be
*for the doom entry quest.
  if %actor.quest_stage[doom_entrance]% == 1
   if (%actor.wearing[48430]%)
   if %self.vnum% == 55214
   Mecho The deer flees wildly at the sight of the blood-soaked rag!
   flee
   elseif %self.vnum% == 55244
   mecho The deer boldly stands its ground, nostrils flaring at the scent of blood.
   end
   end
  end
~
#48419
wild-hunt deer flee~
0 ac 100
drop~
* If drop the player is using the secret keyword to drop
* the rag, flee!  Unfortunately, players can force the
* deer to flee any time, so don't tell any of them this
* keyword.
return 0
if %actor.quest_stage[doom_entrance]% && %actor.quest_var[doom_entrance:wild_hunt]% == 1 && %arg% == ragtoscarewhitetaileddeer
   wait 1
   set room %self.room%
   while %room% == %self.room%
      flee
   done
endif
~
#48420
wild-hunt death~
0 af 100
~
* If this deer is the white hart, load the spirit of the white
* hart who runs away from the questor.
* If this deer is normal, load two more deer.
* If this deer is the spirit of the white hart, load the antlers.
if %self.vnum% == 55244
  if %actor.quest_stage[doom_entrance]% == 1
    return 0
    mecho The spirit of the white hart breaks from the body and scampers off!
    mload mob 55245
  endif
elseif %self.vnum% == 55214
  if %actor.quest_stage[doom_entrance]% == 1
    eval room 55210 + %random.82%
    mat %room% mload mob 55214
    eval room 55210 + %random.82%
    mat %room% mload mob 55214
  endif
elseif %self.vnum% == 55245
  set i %actor.group_size%
  if %i%
    set a 1
  else
    set a 0
  endif
  while %i% >= %a%
    set person %actor.group_member[%a%]%
    if %person.room% == %self.room%
      if %person.quest_stage[doom_entrance]% == 1
        quest advance doom_entrance %person.name%
        msend %person% &7&bYou have advanced the quest!&0
        set load yes
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
  if %load% == yes
    mload obj 48429
  endif
endif
~
#48421
wild-hunt-hold-rag~
0 ac 100
hold~
*Like trigger 48418, only when they are not already holding the rag.
if %actor.quest_stage[doom_entrance]% == 1
  mforce %actor% hold ragtoscarewhitetaileddeer
  wait 1
  if %self.vnum% == 55214
  mecho The deer flees wildly at the sight of the blood-soaked rag!
  flee
  elseif %self.vnum% == 55244
  mecho The deer boldly stands its ground, nostrils flaring at the scent of blood.
  end
else
  return 0
end
~
#48422
keeper-keys-yes~
0 d 100
yes~
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
    if !%person.quest_stage[doom_entrance]%
      quest start doom_entrance %person.name%
      set begin 1
      msend %person% &7&bYou have begun the quest to enter Lokari's Keep!&0
    endif
  elseif %person%
    eval i %i% + 1
  endif
  eval a %a% + 1
done
if %begin%
  wait 2
  bow %actor.name%
  mecho %self.name% says, 'Then go ask the Oracle of the Hunt if you can prove your worth.'
  wait 1s
  mecho %self.name% says, 'When you have proved yourself to all three Oracles, please come see me again.'
~
#48423
vial-drop~
1 h 100
~
return 0
if %actor.quest_stage[doom_entrance]% == 5
  if %actor.room% == 23731
    oecho The vial of sunlight &7&bflares brightly!&0
    wait 2s
    oecho &3&bSunlight fills the darkened cavern and voices cry out in pain!&0
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
        if %person.quest_stage[doom_entrance]% == 5
          quest advance doom_entrance %person.name%
          osend %person% &7&bYou have advanced the quest!&0
        endif
      elseif %person%
        eval i %i% + 1
      endif
      eval a %a% + 1
    done
    oecho &9&bThe vial dissolves in a flash of light.&0
    opurge %self%
  else
    oecho Dropping the vial here would have no effect!
  endif
endif
~
#48424
rhalean-sister-death~
0 f 100
~
set i %actor.group_size%
if %i%
   set a 1
else
   set a 0
endif
while %i% >= %a%
  set person %actor.group_member[%a%]%
  if %person.room% == %self.room%
      if %person.quest_stage[doom_entrance]% == 3
        quest advance doom_entrance %person.name%
        msend %person% &7&bYou have advanced the quest!&0
        set found 1
      endif
  elseif %person%
      eval i %i% + 1
  endif
  eval a %a% + 1
done
if %found%
  mecho &bRhalean's presence fills the room as her evil sister shrieks in pain!&0
endif
~
#48425
wild-hunt-rec-antlers~
0 j 100
48429~
wait 2
mjunk antlers
set accept 0
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
    if %person.quest_stage[doom_entrance]% == 2
      quest advance doom_entrance %person.name%
      bow %person.name%
      msend %person% %self.name% says, 'You are indeed worthy, %person.name%.  Please continue on to the Oracle of Justice.'
      msend %person% &7&bYou have advanced the quest!&0
      set accept 1
    endif
  elseif %person%
    eval i %i% + 1
  endif
  eval a %a% + 1
done
if !%accept%
  msend %actor% %self.name% says, 'What is this?'
  wait 2
  msend %actor% %self.name% says, 'You mock the Goddess of the Hunt!'
endif
~
#48426
oracle-sun-extra-key~
0 d 100
key~
if %actor.has_completed[doom_entrance]%
mload obj 48421
give key %actor.name%
say Of course, %actor.name%.
wait 1s
say Good luck against Lokari!
end
~
#48427
Oracle Sun refuse~
0 j 100
~
switch %object.vnum%
  case %maceitem2%
  case %maceitem3%
  case %maceitem4%
  case %maceitem5%
  case %macevnum%
  case %wandgem%
  case %wandtask3%
  case %wandtask4%
  case %wandvnum%
    halt
    break
  default
    set response I have no need of this.
done
if %response%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, '%response%'
endif
~
#48428
Justice refuse~
0 j 100
~
switch %object.vnum%
  default
    if %actor.quest_stage[wizard_eye]% == 10
      switch %object.vnum%
        case 3218
        case 53424
        case 43021
        case 4003
          halt
          break
        default
          set response This %get.obj_noadesc[%object.vnum%]% can't possibly be what you needed to ask me about.
      done
    else
      set response I'm sorry, do you need something?
    endif
done
if %response%
  return 0
  msend %actor% %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, '%response%'
endif
~
#48429
Hunt refuse~
0 j 0
48429~
switch %object.vnum%
  default
    set response Why do you bother the Goddess of the Hunt?
done
if %response%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, '%response%'
endif
~
#48503
mighty druid creeping doom~
2 a 100
~
set person %self.people%
while %person%
   set next %person.next_in_room%
   if (%person.vnum% < 48500) || (%person.vnum% > 48599)
      eval damage 190 + %random.20%
      if %person.aff_flagged[SANCT]%
         eval damage %damage% / 2
      endif
      if %person.aff_flagged[STONE]%
         eval damage %damage% / 2
      endif
      wecho &b&9The mighty druid sends out an endless wave of crawling &0&1arachnoids&b&9 and &0&2insects&b&9 to consume his foes!&0 (&1&b%damage%&0)
      wdamage %person% %damage%
   endif
   set person %next%
done
~
#48504
mighty druid random paralysis-wear-off~
0 b 100
~
set now %time.stamp%
if %paralysis_victim_1% && ((%paralysis_victim_1.room% != %self.room%) || (%paralysis_expire_1% <= %now%))
   if %paralysis_victim_1.room% == %self.room%
      mechoaround %paralysis_victim_1% &2The crop of roots recedes, releasing %paralysis_victim_1.name% from its grasp.&0
      msend %paralysis_victim_1% &2The crop of roots recedes, releasing you from its grasp.&0
   else
      mecho &2The crop of roots recedes, releasing %paralysis_victim_1.name% from its grasp.&0
   endif
   unset paralysis_victim_1
endif
if %paralysis_victim_2% && ((%paralysis_victim_2.room% != %self.room%) || (%paralysis_expire_2% <= %now%))
   if %paralysis_victim_2.room% == %self.room%
      mechoaround %paralysis_victim_2% &2The crop of roots recedes, releasing %paralysis_victim_2.name% from its grasp.&0
      msend %paralysis_victim_2% &2The crop of roots recedes, releasing you from its grasp.&0
   else
      mecho &2The crop of roots recedes, releasing %paralysis_victim_2.name% from its grasp.&0
   endif
   unset paralysis_victim_2
endif
if %paralysis_victim_3% && ((%paralysis_victim_3.room% != %self.room%) || (%paralysis_expire_3% <= %now%))
   if %paralysis_victim_3.room% == %self.room%
      mechoaround %paralysis_victim_3% &2The crop of roots recedes, releasing %paralysis_victim_3.name% from its grasp.&0
      msend %paralysis_victim_3% &2The crop of roots recedes, releasing you from its grasp.&0
   else
      mecho &2The crop of roots recedes, releasing %paralysis_victim_3.name% from its grasp.&0
   endif
   unset paralysis_victim_3
endif
if %paralysis_victim_4% && ((%paralysis_victim_4.room% != %self.room%) || (%paralysis_expire_4% <= %now%))
   if %paralysis_victim_4.room% == %self.room%
      mechoaround %paralysis_victim_4% &2The crop of roots recedes, releasing %paralysis_victim_4.name% from its grasp.&0
      msend %paralysis_victim_4% &2The crop of roots recedes, releasing you from its grasp.&0
   else
      mecho &2The crop of roots recedes, releasing %paralysis_victim_4.name% from its grasp.&0
   endif
   unset paralysis_victim_4
endif
~
#48505
mighty_druid entanglement~
0 k 100
~
set now %time.stamp%
if %paralysis_victim_1% && ((%paralysis_victim_1.room% != %self.room%) || (%paralysis_expire_1% <= %now%))
   * Clear paralysis on expired victim
   if %paralysis_victim_1.room% == %self.room%
      mechoaround %paralysis_victim_1% &2The crop of roots recedes, releasing %paralysis_victim_1.name% from its grasp.&0
      msend %paralysis_victim_1% &2The crop of roots recedes, releasing you from its grasp.&0
   else
      mecho &2The crop of roots recedes, releasing %paralysis_victim_1.name% from its grasp.&0
   endif
   unset paralysis_victim_1
endif
if %paralysis_victim_2% && ((%paralysis_victim_2.room% != %self.room%) || (%paralysis_expire_2% <= %now%))
   * Clear paralysis on expired victim
   if %paralysis_victim_2.room% == %self.room%
      mechoaround %paralysis_victim_2% &2The crop of roots recedes, releasing %paralysis_victim_2.name% from its grasp.&0
      msend %paralysis_victim_2% &2The crop of roots recedes, releasing you from its grasp.&0
   else
      mecho &2The crop of roots recedes, releasing %paralysis_victim_2.name% from its grasp.&0
   endif
   unset paralysis_victim_2
endif
if %paralysis_victim_3% && ((%paralysis_victim_3.room% != %self.room%) || (%paralysis_expire_3% <= %now%))
   * Clear paralysis on expired victim
   if %paralysis_victim_3.room% == %self.room%
      mechoaround %paralysis_victim_3% &2The crop of roots recedes, releasing %paralysis_victim_3.name% from its grasp.&0
      msend %paralysis_victim_3% &2The crop of roots recedes, releasing you from its grasp.&0
   else
      mecho &2The crop of roots recedes, releasing %paralysis_victim_3.name% from its grasp.&0
   endif
   unset paralysis_victim_3
endif
if %paralysis_victim_4% && ((%paralysis_victim_4.room% != %self.room%) || (%paralysis_expire_4% <= %now%))
   * Clear paralysis on expired victim
   if %paralysis_victim_4.room% == %self.room%
      mechoaround %paralysis_victim_4% &2The crop of roots recedes, releasing %paralysis_victim_4.name% from its grasp.&0
      msend %paralysis_victim_4% &2The crop of roots recedes, releasing you from its grasp.&0
   else
      mecho &2The crop of roots recedes, releasing %paralysis_victim_4.name% from its grasp.&0
   endif
   unset paralysis_victim_4
endif
* 25% chance to do creeping doom proc
set mode %random.4%
if %mode% == 1
   wait 2s
   m_run_room_trig 48503
   halt
endif
* 75% chance to attempt to entangle
if (!%paralysis_victim_1%) || (!%paralysis_victim_2%) || (!%paralysis_victim_3%) || (!%paralysis_victim_4%)
   if %paralysis_victim_1%
      if %paralysis_victim_2%
         if %paralysis_victim_3%
            if %paralysis_victim_4%
               * quit if four people already entangled
               halt
            else
               * 20% chance if three people are entangled
               set chance %random.10%
            endif
         else
            * 25% chance if two people are entangled
            set chance %random.8%
         endif
      else
         * 33% chance if one person is already entangled
         set chance %random.6%
      endif
   else
      * 50% chance if no one is already entangled
      set chance %random.4%
   endif
   if %chance% > 3
      halt
   endif
   set max_tries 10
   while %max_tries% >= 0
      * find a victim, preferably an assassin
      * but will settle for any player if no assassin found
      set victim %random.char%
      if (%victim.vnum% == -1)
         if (%paralysis_victim_1% && (%paralysis_victim_1.name% == %victim.name%)) || (%paralysis_victim_2% && (%paralysis_victim_2.name% == %victim.name%)) || (%paralysis_victim_3% && (%paralysis_victim_3.name% == %victim.name%))
            set victim %self%
         elseif (%victim.class% == Assassin)
            set max_tries 0
         else
            set secondary_victim %victim%
         endif
      endif
      eval max_tries %max_tries% - 1
   done
   * quit if no players
   if (%victim.vnum% == -1)
   elseif %secondary_victim%
      set victim %secondary_victim%
   else
      halt
   endif
   wait 2s
   mechoaround %victim% %self.name% snarls and waves a hand at the ground at %victim.name%'s feet.
   msend %victim% %self.name% snarls at you and waves a hand at the ground around your feet!
   mechoaround %victim% &2A crop of wriggling roots bursts from the ground, entangling %victim.name%!&0
   msend %victim% &2A crop of wriggling roots bursts from the ground, entangling you!&0
   * declare paralysis message
   set message &2The roots lock you in place, preventing movement!&0
   global message
   * create a basher to force the player into a bashed stance
   if (%victim.size% == Small) || (%victim.size% == Tiny)
      * Basher is medium
      mload mob 48515
   elseif (%victim.size% == Large) || (%victim.size% == Huge) || (%victim.size% == Giant)
      * Basher is huge
      mload mob 48513
   else
      * Basher is large
      mload mob 48514
   endif
   mforce bashing-roots mload obj 1016
   mforce bashing-roots mat 1100 wear shield
   mforce %victim% stand
   mforce bashing-roots bash %victim.name%
   mforce bashing-roots mjunk shield
   mpurge bashing-roots
   * reinitiate combat if victim was tanking
   if %actor.name% == %victim.name%
      wait 2s
      kill %victim.name%
   endif
   * save victim variable
   if %paralysis_victim_1%
      if %paralysis_victim_2%
         if %paralysis_victim_3%
            set paralysis_victim_4 %victim%
            global paralysis_victim_4
            eval paralysis_expire_4 %now% + 1
            global paralysis_expire_4
         else
            set paralysis_victim_3 %victim%
            global paralysis_victim_3
            eval paralysis_expire_3 %now% + 1
            global paralysis_expire_3
         endif
      else
         set paralysis_victim_2 %victim%
         global paralysis_victim_2
         eval paralysis_expire_2 %now% + 1
         global paralysis_expire_2
      endif
   else
      set paralysis_victim_1 %victim%
      global paralysis_victim_1
      eval paralysis_expire_1 %now% + 1
      global paralysis_expire_1
   endif
endif
~
#48506
Elder_Druid~
0 f 100
~
mecho %self.name% says, 'Finally my eternal torment is finished...
mecho  
mecho %self.name% says, 'You, you must punish the evil God that is responsible
mecho &0for the destruction of my precious nature.  You must attempt to kill him.'
mecho  
mecho %self.name% says, 'That vile bastard!'
mecho  
mecho %self.name% says, 'LOKARI!'
~
#48507
efreeti_fight~
0 k 100
~
set breathe_chance %random.4%
if %breathe_chance% == 4
   wait 2s
   breath fire
endif
~
#48508
efreeti_greet~
0 h 100
~
if !(%self.aff_flagged[FIRESHIELD]%) && !(%self.aff_flagged[BLIND]%)
   mload obj 1165
   mload obj 1172
   mat 1100 quaff fireshield
   mat 1100 quaff negate-heat
endif
~
#48530
paralysis_prevent_quaff~
0 c 100
quaff~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48531
paralysis_prevent_remove~
0 c 100
remove~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48532
paralysis_prevent_wear~
0 c 100
wear~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48533
paralysis_prevent_wield~
0 c 100
qield~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48534
paralysis_prevent_order~
0 c 100
order~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48535
paralysis_prevent_memorize~
0 c 100
memorize~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48536
paralysis_prevent_pray~
0 c 100
pray~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48537
paralysis_prevent_stand~
0 c 100
stand~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48538
paralysis_prevent_get~
0 c 100
get~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48539
paralysis_prevent_give~
0 c 100
give~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48540
paralysis_prevent_put~
0 c 100
put~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48541
paralysis_prevent_recite~
0 c 100
recite~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48542
paralysis_prevent_use~
0 c 100
use~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48543
paralysis_prevent_drop~
0 c 100
drop~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48544
unused~
0 c 100
west~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48545
unused~
0 c 100
east~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48546
unused~
0 c 100
up~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48547
unused~
0 c 100
down~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48548
unused~
0 c 100
order~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48549
unused~
0 c 100
rescue~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48550
load maddened tremaen limb~
0 f 100
~
set chance %random.10%
if (%chance% > 6) && (%get.obj_count[48417]% < 2)
   mload obj 48417
endif
~
#48551
unused~
0 c 100
memorize~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48552
unused~
0 c 100
pray~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48553
unused~
0 c 100
steal~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48554
unused~
0 c 100
mount~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48555
unused~
0 c 100
enter~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48556
unused~
0 c 100
summon~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48557
unused~
0 c 100
disarm~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48558
unused~
0 c 100
guard~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48559
unused~
0 c 100
track~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48560
unused~
0 c 100
leave~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48561
unused~
0 c 100
hide~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48562
unused~
0 c 100
throatcut~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48563
unused~
0 c 100
stand~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48564
unused~
0 c 100
alert~
if %paralysis_victim_1% && (%paralysis_victim_1.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_2% && (%paralysis_victim_2.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_3% && (%paralysis_victim_3.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_4% && (%paralysis_victim_4.name% == %actor.name%)
   set block 1
elseif %paralysis_victim_5% && (%paralysis_victim_5.name% == %actor.name%)
   set block 1
endif
if %block%
   return 1
   if %message%
      msend %actor% %message%
   else
      msend %actor% You are trapped, and unable to move!
   endif
else
   return 0
endif
~
#48565
unused~
0 g 100
~
say My trigger commandlist is not complete!
~
#48566
unused~
0 g 100
~
say My trigger commandlist is not complete!
~
#48630
ice elemental lord - fight~
0 k 100
~
set chance %random.10%
if %chance% > 5
   wait 2s
   m_run_room_trig 48631
endif
~
#48631
ice elemental lord - damage~
2 a 100
~
* Hit half as many people are in the room
set negatecold !COLD
eval pop %self.people[count]% / 2
set max 15
set count 1
wecho &6&bThe Ice Elemental Lord crashes into the wall, bringing down a shower of ice!&0
while %count% <= %pop%
   set victim %random.char%
   * Don't hit the Ice Elemental Lord
   if %victim.vnum% != 48632
      set flags %victim.aff_flags%
      * Base damage between 100 and 130
      eval damage 100 + %random.30%
      if %flags% /= SANCT
         eval damage %damage% / 2
      endif
      if %flags% /= WATERFORM
         * Heal players with waterform
         wsend %victim% &4&bThe falling ice melts into your body!&0 (&2%damage%&0)
         wechoaround %victim% &6%victim.name%'s body absorbs the falling ice!&0 (&3%damage%&0)
         wheal %victim% %damage%
      elseif (%flags% /= %negatecold%) || (%flags% /= PROT_COLD)
         * No effect for negate cold or elemental warding for ice
         wsend %victim% &6The falling ice bounces off your skin, settling lightly on the floor.&0
         wechoaround %victim% &6The falling ice bounces off %victim.name%'s skin, settling lightly on the floor.&0
      elseif %flags% /= FIRESHIELD
         * No effect for fireshield
         wsend %victim% &6The barrage of ice &3vaporizes&6 as it touches the &1flames&6 around your body!&0
         wechoaround %victim% &6The barrage of ice &3vaporizes&6 on the &1flames&6 about %victim.name%'s body!&0
      elseif %flags% /= BLUR
         * Decrease damage for blur
         eval damage %damage% - 35
         wsend %victim% &6You dance through the barrage and only a few blocks of ice strike you!&0 (&1&b%damage%&0)
         wechoaround %victim% &6Dancing through the barrage, %victim.name% is struck by few ice blocks.&0 (&4%damage%&0)
         wdamage %victim% %damage%
      elseif %flags% /= FLY
         * Decrease damage for fly
         eval damage %damage% / 2
         wsend %victim% &6You dart through the air, missing much of the falling ice!&0 (&1&b%damage%&0)
         wechoaround %victim% &6%victim.name% darts through the air, evading much of the falling ice!&0 (&4%damage%&0)
         wdamage %victim% %damage%
      elseif %flags% /= STONE
         * Increase damage for stone skin
         eval damage %damage% + 60
         wsend %victim% &6Slowed by your stony skin, you are unable to dodge a slab of falling ice!&0 (&1&b%damage%&0)
         wechoaround %victim% &6Unable to dodge, %victim.name% takes the full brunt of the hail!&0 (&4%damage%&0)
         wdamage %victim% %damage%
      else
         * Hit everyone else
         wsend %victim% &6As you look up, a huge block of ice strikes you on the head!&0 (&1&b%damage%&0)
         wechoaround %victim% &6A hail of icy shards crashes down upon %victim.name%!&0 (&4%damage%&0)
         wdamage %victim% %damage%
      endif
      eval count %count% + 1
   endif
   eval max %max% - 1
   if %max% < 1
      eval count %pop% + 1
   endif
done
~
#48632
ice elemental lord - greet~
0 h 100
~
if %self.aff_flagged[BLIND]%
   halt
endif
if %self.aff_flagged[COLDSHIELD]%
   mload obj 1160
   mat 1100 quaff coldshield
endif
set coldproof %self.aff_flagged[!COLD]%
if %coldproof%
   mload obj 1171
   mat 1100 quaff negate-cold
endif
if %self.aff_flagged[WATERFORM]%
   mload obj 1178
   mat 1100 quaff waterform
endif
~
#48635
leviathan fight~
0 k 50
~
wait 2s
set mode %random.10%
if %mode% < 6
   sweep
else
   set victim %random.char%
   if (%victim.name% != %actor.name%) && ((%victim.class% == Warrior) || (%victim.class% /= Anti) || (%victim.class% == Ranger) || (%victim.class% == Paladin) || (%victim.class% == Monk) || (%victim.class% == Mercenary) || (%victim.class% == Berserker)) 
      if %victim.room% == %self.room%
         mgoto 1100
         mgoto %actor.room%
         mecho &7&bThe Leviathan thrashes about madly, switching opponents!&0
         kill %victim.name%
      endif
   endif
endif
~
#48636
leviathan block~
2 g 100
~
if %actor% && (%actor.vnum% == -1) && %self.people[48635]% && (%actor.level% < 100)
   wsend %actor% The colossal body of the Leviathan blocks your passage, throwing you back.
   return 0
endif
~
#48637
titan fight~
0 k 100
~
set action %random.10%
if %action% > 7
   * 30% chance to punch the tank
   wait 2s
   if %actor% && (%actor.room% == %self.room%) && (%actor.vnum% == -1)
      eval damage 300 + %random.50%
      if %actor.aff_flagged[SANCT]%
         eval damage %damage% / 2
      endif
      if %actor.aff_flagged[STONE]%
         eval damage %damage% / 2
      endif
      * Chance for critical hit
      set variant %random.15%
      if %variant% == 1
         eval damage %damage% - 100
      elseif %variant% == 15
         eval damage %damage% + 200
      endif
      if %damage% > 0
         mdamage %actor% %damage% crush
         mechoaround %actor% %self.name% punches %actor.name% in the face, giving %actor.o% a black eye. (&3%damdone%&0)
         msend %actor% %self.name% punches you in the face. (&1&b%damdone%&0)
      else
         mechoaround %actor% %self.name% tries to punch %actor.name% in the face, but can't seem to make contact.
         msend %actor% %self.name% tries to punch you, but can't seem to make contact.
      end
   endif
elseif (%action% > 4)
   * 30% chance to try to blind the room
   wait 1s
   emote clasps %get.obj_shortdesc[48424]% in his giant hands.
   mecho %get.obj_shortdesc[48424]% begins to &bglow&0 brightly.
   wait 2s
   mecho %get.obj_shortdesc[48424]% flares brightly, throwing blinding light in all directions!
   set room %self.room%
   set person %room.people%
   while %person%
      if %person.vnum% == -1
         mcast blindness %person% 100
      endif
      set person %person.next_in_room%
   done
endif
* 40% chance to do nothing
~
#48638
titan death~
0 f 100
~
if %self.wearing[48424]%
   * get rid of the staff sphere and load the key one
   mjunk timuns-golden-sphere
   mload obj 48422
endif
~
$~
