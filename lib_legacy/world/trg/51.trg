#5101
Stone-statue-dagger~
2 a 100
~
wload obj 5104
~
#5102
Stone-statue-ring--dagger~
0 j 100
5102~
if %actor.vnum% == -1
  if %object.vnum% == 5102
    return 1
    wait 2s
    mecho &4&bThe eyes of the immense stone statue glow &1&bred&0 &4&bas a deafening hum shakes the cavern!&0
    wait 2 s
    mecho &3&bThe statue comes to life!&0
    wait 1 s
    mecho &bThe statue makes your soul quake with a vicious&0 &1&bROOOOOAAAAAARRRRRR!&0
    mecho You panic, but you can not escape!
    wait 1 s
    mecho &6&bThe statue shouts 'I will free you my master!'&0
    wait 4
    mecho &3&bThe statue starts casting a spell...&0
    wait 3 s
    mecho &3&bThe statue's molecules loosen and eventually compress into a ball of light!&0
    wait 4
    mecho &6&bThe ball disappears like&0 &4&blightning&0 &6&bwith a loud crackle of energy!&0
    wait 4
    mecho A small stone dagger falls from its place, a gift for freedom.
    m_run_room_trig 5101
    mpurge stone-ring
    mpurge statue
  else
    return 0
    mechoaround %actor% %actor.name% offers %object.shortdesc% to %self.name%.
    mechoaround %actor% %self.name% does not stir.
    msend %actor% You offer up %object.shortdesc% to %self.name%, but there is no reaction.
  endif
endif
~
#5104
Arre Matu refuse~
0 j 0
16110~
switch %object.vnum%
  default
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 1s
    eye %actor.name%
    msend %actor% %self.name% says, 'What is this? I have no need for %object.shortdesc%.'
done
~
#5105
monk_subclass_quest_arre_greet~
0 h 100
~
wait 2
switch %actor.quest_stage[monk_subclass]%
  case 1
    msend %actor% %self.name% says, 'Have you returned to join the Brotherhood?  You will be rewarded for your success with wonderful training, but only those pure of mind will complete the &6&bquest&0.'
    break
  case 2
    msend %actor% %self.name% says, 'Remember my story.  I long for the return of my prized bronze sash.'
    wait 1s
    emote shakes her head.
    msend %actor% %self.name% says, 'I was told it looked wonderful on me.  %actor.name%, can you track down these fiends?'
    mecho &0
    wait 1s
    msend %actor% %self.name% whispers to you, 'Please?'
    wait 2s
    msend %actor% %self.name% says, 'Long ago some ruthless &6&bfiends&0 made off with it.'
    break
  case 3
  case 4
    msend %actor% %self.name% says, 'Have you recovered my bronze sash from those desert thieves?'
    break
  default
    switch %actor.race%
*      case ADD RESTRICTED RACES HERE
*        halt
*        break
      default
        if %actor.class% /= warrior && (%actor.level% >= 10 && %actor.level% <= 25)
          look %actor.name%
          wait 1
          snicker
          msend %actor% %self.name% says, 'Well hello, what have you come &6&bhere&0 for?'
          smile %actor.name%
        endif
    done
done
~
#5106
monk_subclass_quest_arre_speech1~
0 d 100
here here? where where?~
if %actor.class% /= Warrior 
  switch %actor.race%
*    case ADD RESTRICTED RACES HERE
*      if %actor.level% >= 10 && %actor.level% <= 25
*        msend %actor% &1Your race may not subclass to monk.&0
*      endif
*      halt
*      break
    default
      wait 2
      if %actor.level% < 10
        eye %actor.name%
        msend %actor% %self.name% says, 'Perhaps in time, after you've gained a little more experience, we can talk more.'
      elseif %actor.level% >= 10 && %actor.level% <= 25
        wait 2
        chuckle
        msend %actor% %self.name% says, 'Do you not know where you are?  Silly youth.'
        wait 1
        msend %actor% %self.name% says, 'Why did you come to my chambers if you do not have a reason?'
        wait 1s
        frown
        wait 1
        msend %actor% %self.name% says, 'I suppose you are here for the same &6&breason&0 as everyone else that ever visits me.'
        sigh
      else
        msend %actor% %self.name% says, 'Ah, you are no longer suited for my teachings.  So I shall not waste my time with you.'
      endif
  done
else
  wait 2
  msend %actor% %self.name% says, 'Ah, you are not well suited to my teaching.  So I shall not waste my time with you.'
endif
~
#5107
monk_subclass_quest_arre_speech2~
0 d 100
reason reason?~
if %actor.class% /= Warrior
  switch %actor.race%
*    case ADD RESTRICTED RACES HERE
*      if %actor.level% <= 25
*        msend %actor% &1Your race may not subclass to monk.&0
*      endif
*      halt
*      break
    default
      wait 2
      if !%actor.quest_stage[monk_subclass]%
        if %actor.level% >= 10
          grumble
          msend %actor% %self.name% says, 'Yes a reason...'
          wait 1s
          msend %actor% %self.name% says, 'You do not know much do you?'
          roll
          mutter
          wait 3s
          eye %actor.name%
          msend %actor% %self.name% says, 'Are you certain you wish to study as a monk and have strength over mind and body alike?'
        elseif %actor.level% < 10
          eye %actor.name%
          msend %actor% %self.name% says, 'Perhaps in time, after you've gained a little more experience, we can talk more.'
        else
          msend %actor% %self.name% says, 'Ah, you are no longer suited for my teachings.  So I shall not waste my time with you.'
        endif
      endif
  done
else
  wait 2
  msend %actor% %self.name% says, 'Ah, you are not well suited to my teaching.  So I shall not waste my time with you.'
endif
~
#5108
monk_subclass_quest_arre_speech3~
0 d 100
yes no~
if %actor.class% /= Warrior
  switch %actor.race%
*    case ADD RESTRICTED RACES HERE
*      if %actor.level% >= 10 && %actor.level% <= 25
*        msend %actor% &1Your race may not subclass to monk.&0
*      endif
*      halt
*      break
    default
      if %speech% /= yes
        wait 2
        if %actor.level% >= 10
          if %actor.quest_stage[monk_subclass]% == 0
            quest start monk_subclass %actor.name% Mon
            bow
            msend %actor% %self.name% says, 'It is wonderful to hear of others wanting to join the Brotherhood of the Monks.  You will be rewarded for your success with wonderful training, but only those pure of mind will complete the &6&bquest&0.'
            mecho &0 
            msend %actor% %self.name% says, 'You may ask about your &6&b[subclass progress]&0 or at any time.'
          elseif %actor.quest_stage[monk_subclass]% == 2
            msend %actor% %self.name% says, 'Thank you.  I'm certain those &6&bfiends&0 still have it.'
          endif
        elseif %actor.level% < 10
          eye %actor.name%
          msend %actor% %self.name% says, 'Perhaps in time, after you've gained a little more experience, we can talk more.'
        else
          msend %actor% %self.name% says, 'Ah, you are no longer suited for my teachings.  So I shall not waste my time with you.'
        endif
      else
        wait 2
        if %actor.quest_stage[monk_subclass]% == 0
          msend %actor% %self.name% says, 'It is too bad then, we could use more in our ranks, but only if they truly wish to belong.'
          smile
        elseif %actor.quest_stage[monk_subclass]% == 2
          msend %actor% %self.name% says, 'Then I am unwilling to continue your training.'
        endif
        wait 1s
        msend %actor% %self.name% says, 'Begone then.
        msend %actor% Your eyes blink repeatedly and you find yourself elsewhere.
        mechoaround %actor% There is a brief sparkle, and %actor.name% is sent elsewhere.
        mteleport %actor% 58001
      endif
  done
endif
~
#5109
monk_subclass_quest_arre_speech4~
0 d 100
quest quest?~
wait 2
if %actor.quest_stage[monk_subclass]% == 1
  quest advance monk_subclass %actor.name%
  msend %actor% %self.name% says, 'Alright, well sit back.'
  wait 2s
  msend %actor% %self.name% says, 'Usually people come in here promising me the return of something long lost of mine.'
  scratch
  wait 3s
  msend %actor% %self.name% says, 'The thing is, I have not always led this life.  When I was young I was quite the rabble-rouser.'
  emote looks wistfully for a moment.
  wait 4s
  msend %actor% %self.name% says, 'Well, it is rather embarrassing, but...  I miss my old sash, and I want it back.'
  wait 2s
  emote shakes her head.
  msend %actor% %self.name% says, 'I was told it looked wonderful on me.'
  wait 3s
  msend %actor% %self.name% says, '%actor.name%, can you recover it?'
  wait 1s
  msend %actor% %self.name% whispers to you, 'Please?'
  wait 3s
  msend %actor% %self.name% says, 'Long ago some ruthless &6&bfiends&0 made off with it.'
endif
~
#5110
quest_exit_arre~
0 d 100
exit exit?~
wait 2
sigh
say Again, my time has been wasted.  Very well, leave.
msend %actor% %self.name% waves her arms, sending you out of her chamber.
mechoaround %actor% %self.name% sweeps her arms through the air, sending %actor.name% away.
mteleport %actor% 58001
msend %actor% You blink and find yourself back on the pebbled path!
mforce %actor% look
~
#5111
monk_subclass_quest_arre_speech5~
0 d 100
fiend? fiends? who who? fiend fiends~
wait 2
if %actor.quest_stage[monk_subclass]% == 2
  sigh %actor.name%
  msend %actor% %self.name% says, 'Yes, fiends.  The filthy thieves from the blistering sands in North Caelia.'
  shiver
  wait 1
  msend %actor% %self.name% says, 'Personally I enjoy much cooler weather, but they seem to manage themselves fine there.'
  wait 2
  grin
  msend %actor% %self.name% says, 'I would not mind seeing them vanish outright, however.'
  emote sighs again.
  wait 1
  msend %actor% %self.name% says, 'Enough, I have grown tired of your company.  You bring back bad memories.'
  quest advance monk_subclass %actor.name%
  mecho %self.name% dismisses you.
  mteleport all 58025
  mforce all look
  mgoto 58024
endif
~
#5112
monk_subclass_quest_arre_receieve~
0 j 100
16110~
wait 2
switch %actor.quest_stage[monk_subclass]%
  case 1
  case 2
    msend %actor% %self.name% says, 'Funny, I don't even remember telling you what you were questing for.'
    break
  case 3
    *got sash without getting sash themselves
    msend %actor% %self.name% says, 'How on earth did you get me this?  Did you come by it through your own means?'
    eye %actor.name%
    emote shakes her head.
    wait 2s
    msend %actor% %self.name% says, 'I cannot help those who cheat themselves.'
    break
  case 4
    emote cheers wildly, dancing all around the room.
    wait 3
    emote pants heavily for a moment calming down.
    wait 1
    shake %actor.name%
    msend %actor% %self.name% says, 'Wonderful, this shows your true dedication to helping one of the brotherhood.'
    grin
    wait 2
    msend %actor% %self.name% says, 'Type &9&b'subclass'&0 to proceed.'
    quest complete monk_subclass %actor.name%
    break
  default
    msend %actor% %self.name% says, 'What a wonderful sash, thank you.'
    wait 1s
    msend %actor% %self.name% says, 'Shame you were not questing for me, this is a great prize you have given me.'
done
mjunk sash
~
#5113
monk_subclass_quest_sash_get~
1 g 100
~
if %actor.quest_stage[monk_subclass]% == 3
    quest advance monk_subclass %actor.name%
endif
~
#5114
room_monkquest_entry~
2 c 0
bow priest~
switch %cmd%
  case b
  case bo
    return 0
    halt
done
wforce %actor% bow
wforce monk_quest_priest emote smiles broadly, pressing in a crack in the wall.
wdoor 58025 east flags a
wdoor 58025 east room 58024
wdoor 58025 east name heavy stone door
wdoor 58025 east key -1
wdoor 58025 east description A heavy slab of stone sits in your way.
wait 3
wecho %get.mob_shortdesc[5131]% presses a little harder, fully opening the stone door.
wechoaround %actor% %get.mob_shortdesc[5131]% points to the east and nudges %actor.name% forward.
wsend %actor% %get.mob_shortdesc[5131]% points to the east and nudges you forward.
~
#5115
priest_openwall_entry_hint~
0 g 100
~
msend %actor% As you enter this tiny alcove, you feel compelled to pay respectful homage to the priest before you.
~
#5116
close_wall_arreroom_entry~
2 g 100
~
wait 5
wecho The stone door slides shut!
wdoor 58025 east purge
wat 58025 wecho The stone door slides shut!
~
#5117
monk_subclass_quest_arre_status~
0 d 0
subclass progress~
switch %actor.quest_stage[monk_subclass]%
  case 1
    wait 2
    msend %actor% %self.name% says, 'It is wonderful to hear of others wanting to join the Brotherhood of the Monks.  You will be rewarded for your success with wonderful training, but only those pure of mind will complete the &6&bquest&0.'
    break
  case 2
    wait 2
    msend %actor% %self.name% says, 'Alright, well sit back.'
    wait 2s
    msend %actor% %self.name% says, 'Usually people come in here promising me the return of something long lost of mine.'
    scratch
    wait 3s
    msend %actor% %self.name% says, 'The thing is, I have not always lead this life.  When I was young I was quite the rabble-rouser.'
    emote looks wistfully for a moment.
    wait 4s
    msend %actor% %self.name% says, 'Well, it is rather embarrassing, but...  I miss my old sash, and I want it back.'
    wait 2s
    emote shakes her head.
    msend %actor% %self.name% says, 'I was told it looked wonderful on me.'
    wait 3s
    msend %actor% %self.name% says, '%actor.name%, can you recover it?'
    wait 1s
    msend %actor% %self.name% whispers to you, 'Please?'
    wait 3s
    msend %actor% %self.name% says, 'Long ago some ruthless &6&bfiends&0 made off with it.'
    break
  case 3
  case 4
    wait 2
    msend %actor% %self.name% says, 'Have you recovered my sash from those thieves out west?  If you return my sash, I will complete your initiation as a monk.'
    break
  default
    if %actor.class% /= Warrior
      switch %actor.race%
*        case ADD RESTRICTED RACES HERE
*          if %actor.level% >= 10 && %actor.level% <= 25
*            msend %actor% &1Your race may not subclass to monk.&0
*          endif
*          halt
*          break
        default
          wait 2
          if %actor.level% >= 10 && %actor.level% <= 25
            msend %actor% %self.name% says, 'You are not trying to join the Brotherhood.'
          elseif %actor.level% < 10
            eye %actor.name%
            msend %actor% %self.name% says, 'Perhaps in time, after you've gained a little more experience, we can talk more.'
          else
            msend %actor% %self.name% says, 'Ah, you are no longer suited for my teachings.  So I shall not waste my time with you.'
          endif
      done
    else
      msend %actor% %self.name% says, 'Ah, you are not well suited to my teaching.  So I shall not waste my time with you.'
    endif
done
~
#5118
dagger_lightning_bolt~
1 d 5
~
ocast 'lightning bolt' %victim%
~
#5119
lantern shows the path~
2 c 0
look~
switch %arg%
  case l
  case la
  case lan
  case lant
  case lante
  case lanter
  case lantern
    wforce %actor% look lantern
    wechoaround %actor% %actor.name% looks at the stone lantern.
    wait 2s
    wecho An eerie &3&bglow&0 begins emitting from the lantern...
    wait 5s
    wdoor 58001 west room 58017
    wecho The light reveals a well-concealed break in the rocky hills to the west!
    wait 20s
    wecho The light begins to flicker and fade...
    wait 5s
    wdoor 58001 west purge
    wecho The passage west is obscured again as the glow of the lantern fades.
done
return 0
~
$~
