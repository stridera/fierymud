#5200
quest_pyro_dooropen~
2 c 100
search~
switch %cmd%
  case s
  case se
    return 0
    halt
done
return 1
wsend %actor% &bSomething doesn't seem quite right here...&0
wait 1s
wsend %actor% You feel a sharp gust of air suck at you as a wall to the east opens.
wechoaround %actor% A sharp gust of wind sucks around %actor.name%'s feet.
wait 1s
wsend %actor% You suddenly feel yourself being pulled into the opening!
wechoaround %actor% %actor.name% suddenly is pulled into the opening!
wat 5220 wecho A shift in the flamewall allows %actor.name% to pop through.
wdoor 5219 east room 5220
wforce %actor.name% east
wait 1
wecho The wall closes again as if it were never there.
wdoor 5219 east purge
~
#5201
quest_pyro_exit~
2 d 100
exit exit?~
wecho Emmath Firehand sighs loudly.
wecho Emmath Firehand says, 'Oh very well, I suppose it is time you left anyway.'
wecho Emmath Firehand grumbles incoherently about something or other.
wait 1s
wecho Emmath Firehand opens the flamewall with a wave of his hand.
wdoor 5220 west flags a
wechoaround %actor% Emmath Firehand pushes %actor.name% through the flamewall.
wsend %actor% Emmath Firehand pushes you through the flamewall.
wforce %actor% west
wait 3s
wecho Emmath Firehand says, 'I think you all should leave as well.'
wait 2s
wecho Emmath Firehand says, 'You had your chance, now it will just be more work for both of us for you to leave.'
wait 1s
wecho The flamewall reseals itself.
wdoor 5220 west flags abcd
~
#5202
pyromancer_subclass_quest_emmath_greet~
0 h 100
~
wait 2
if %actor.quest_stage[%type%_wand]% == %wandstep%
  eval minlevel (%wandstep% - 1) * 10
  if %actor.level% >= %minlevel%
    if %actor.quest_variable[%type%_wand:greet]% == 0
      msend %actor% %self.name% says, 'I see you're crafting something.  If you want my help, we can talk about &6&b[upgrades]&0.'
    else
      if %actor.quest_variable[%type%_wand:wandtask1]% && %actor.quest_variable[%type%_wand:wandtask2]% && %actor.quest_variable[%type%_wand:wandtask3]%
        msend %actor% %self.name% says, 'I sense you're ready!  Let me see the staff.'
      else
        msend %actor% %self.name% says, 'Do you have what I need for the %weapon%?'
      endif
    endif
  endif
endif
set stage %actor.quest_stage[pyromancer_subclass]%
switch %stage%
  case 1
    msend %actor% %self.name% says, 'Come back to try again have you?  Only the best and most motivated of mages will complete the quest I lay before you.'
    smile
    wait 2s
    msend %actor% %self.name% says, 'However, I am sure it is in you, if it is truly your desire, to complete this &1&bquest&0 and become a pyromancer.'
    break
  case 2
    msend %actor% %self.name% says, 'Are you ready to finish listening now?  I once controlled all three parts of the flame: &7&bWhite&0, &bGray&0, and &9&bBlack&0.'
    wait 2s
    frown
    msend %actor% %self.name% says, 'But one of them was taken from my &1&bcontrol&0.'
    wait 1s
    sigh
    break
  case 3
  case 4
    msend %actor% %self.name% says, 'Do you have the %actor.quest_variable[pyromancer_subclass:part]% flame?  Give it here!'
    break
  default
    if %actor.class% /= Sorcerer
      switch %actor.race%
        case dragonborn_frost
        case arborean
          halt
          break
        default
          if %actor.level% >= 10 && %actor.level% <= 45
            mecho Emmath Firehand's glowing eyes flash brightly for a moment.
            msend %actor% %self.name% says, 'Some love the life of &1&bflame&0.  Do you?'
            emote flicks a grin around the room.
          elseif %actor.class% /= Sorcerer && %actor.level% < 10
            emote stares in amazement at %actor.name%.
            msend %actor% %self.name% says, 'I have no idea how you found me but you are far too inexperienced to be in these tunnels in the first place, and even less to ask for my guidance.'
          endif
      done
    endif
done
~
#5203
pyromancer_subclass_quest_emmath_speech1~
0 d 1
flame? flames? life? flame flames life~
if %actor.class% /= Pyromancer
  wait 2
  chuckle
  msend %actor% %self.name% says, 'I have nothing more to teach you, young one.'
elseif !%actor.quest_stage[pyromancer_subclass]%
  if %actor.class% /= Sorcerer
    switch %actor.race%
      case dragonborn_frost
      case arborean
        if %actor.level% >= 10 && %actor.level% <= 45
          msend %actor% &1Your race may not subclass to pyromancer.&0
          halt
        endif
        break
      default
        wait 2
        if %actor.level% >= 10 && %actor.level% <= 45
          nod %actor.name%
          msend %actor% %self.name% says, 'Do you wish to become one with the flame, becoming a mage of fire?'
        elseif %actor.level% < 10
          msend %actor% %self.name% says, 'Not yet, for you are still an initiate.  Come back when you have gained more experience.'
        else
          msend %actor% %self.name% says, 'Unfortunately you are too dedicated to your universalist ways for me to teach you now.'
        endif
    done
  else
    wait 2
    msend %actor% %self.name% says, 'Sorry, I cannot help you learn the ways of the flame.'
  endif
endif
~
#5204
pyromancer_subclass_quest_emmath_speech2~
0 d 1
yes no~
if !%actor.quest_stage[pyromancer_subclass]% && %actor.class% /= Sorcerer
  if %speech% /= yes
    if %actor.level% >= 10 && %actor.level% <= 45
      switch %actor.race%
        case dragonborn_frost
        case arborean
          msend %actor% &1Your race may not subclass to Pyromancer.&0
          halt
          break
        default
          wait 2
          quest start pyromancer_subclass %actor.name% Pyr
          nod
          msend %actor% %self.name% says, 'Only the best and most motivated of mages will complete the &1&bquest&0 I lay before you.'
          smile
          wait 2s
          msend %actor% %self.name% says, 'However, I am sure it is in you, if it is truly your desire, to complete this quest and become a pyromancer.'
          wait 4s
          msend %actor% %self.name% says, 'You may ask about your quest &1&b[subclass progress]&0 at any time.'
          wait 2s
          msend %actor% %self.name% says, 'Don't ask me to repeat myself though.'
          wait 1s
          msend %actor% %self.name% says, 'I hate that.'
      done
    elseif %actor.level% < 10
      msend %actor% %self.name% says, 'Come back to me once you've gained more experience.'
    elseif %actor.level% > 45
      msend %actor% %self.name% says, 'Unfortunately you are too dedicated to your universalist ways for me to teach you now.'
    endif
  else
    wait 2
    msend %actor% %self.name% says, 'Very well, then begone from here.  I suppose you do not have it in you to be a mage of fire anyway.'
    msend %actor% Emmath places his fiery palm in the air, making the room glow brightly.
    mechoaround %actor% Emmath raises his palm into the air, making the room very bright.
    msend %actor% The air around you wavers.
    mechoaround %actor% %actor.name% suddenly disappears at Emmath's command.
    mteleport %actor% 5191
    mat 5191 mforce %actor% look
  endif
endif
~
#5205
pyromancer_subclass_quest_emmath_speech3~
0 d 1
quest quest? ~
wait 2
if %actor.quest_stage[pyromancer_subclass]% == 1
  quest advance pyromancer_subclass %actor.name%
  if %actor.align% >= 350
    set part white
  elseif %actor.align% <= -350
    set part black
  else
    set part gray
  endif
  quest variable pyromancer_subclass %actor.name% part %part%
  msend %actor% %self.name% says, 'I seem to have a problem now, because some time ago...'
  emote sighs, looking troubled.
  wait 2s
  msend %actor% %self.name% says, 'It is just...'
  wait 1s
  mecho &0msend %actor% %self.name% says, 'Well, part of the essence of fire is no longer under my power.'
  emote shakes his head sadly.
  wait 2s
  msend %actor% %self.name% says, 'I once controlled all three parts of the flame: &7&bWhite&0, &bGray&0, and &9&bBlack&0.'
  wait 2s
  frown
  msend %actor% %self.name% says, 'But one of them was taken from my &1&bcontrol&0.'
  wait 1s
  sigh
elseif %actor.quest_stage[pyromancer_subclass]% > 1
  grumble
  wait 1s
  msend %actor% %self.name% says, 'I already told you about the quest.'
  wait 2s
  msend %actor% %self.name% says, 'Part of the flame is no longer under my &1&bcontrol&0.  You should remember that much.'
endif
~
#5206
pyromancer_subclass_quest_emmath_speech4~
0 d 1
control controlled control? controlled? taken taken?~
wait 2
if %actor.quest_stage[pyromancer_subclass]% == 2
  quest advance pyromancer_subclass %actor.name%
  nod %actor.name%
elseif %actor.quest_stage[pyromancer_subclass]% > 2
  roll
  msend %actor% %self.name% says, 'I already told you once.'
  wait 2s
  sigh
endif
if %actor.quest_stage[pyromancer_subclass]% >= 2
  msend %actor% %self.name% says, 'Yes, I once had control over all three fire elements.'
  emote again shakes his head sadly.
  wait 2s
  msend %actor% %self.name% says, 'But the %actor.quest_variable[pyromancer_subclass:part]% flame was taken from me.'
  look %actor.name%
  wait 2s
  msend %actor% %self.name% says, 'To truly help, I suggest you stop loitering and go recover it.'
  ponder
  wait 2s
  switch %actor.quest_variable[pyromancer_subclass:part]%
    case white
      set place &bin some kind of mine&0
      break
    case black
      set place &bin some kind of temple&0
      break
    case gray
    default
      set place &bnear some kind of hill&0
  done
  msend %actor% %self.name% says, 'Last I heard, it was &6&b%place%&0, or something of the like.'
  emote mutters something about the villainy of it all.
  wait 3s
  eye %actor.name%
  msend %actor% %self.name% says, 'Are you still here?  You should leave now.'
endif
~
#5207
quest_pyro_get~
1 g 100
~
if %actor.quest_stage[pyromancer_subclass]% == 3
  quest advance pyromancer_subclass %actor.name%
end
~
#5208
Emmath flames receive~
0 j 100
23822 17308 5211 5212~
if %object.vnum% == 23822
  mat 23890 m_run_room_trig 23814
  if %actor.quest_stage[%type%_wand]% == %wandstep% && !%actor.quest_variable[%type%_wand:wandtask2]%
    halt
  elseif %actor.quest_stage[emmath_flameball]% > 0 && %actor.quest_stage[emmath_flameball]% <= 3
    set flameball_item yes
  endif
elseif %object.vnum% == 17308
  if %actor.quest_stage[pyromancer_subclass]% > 0 && %actor.quest_stage[pyromancer_subclass]% <= 4
    set subclass_item yes
    if %actor.quest_variable[pyromancer_subclass:part]% == black
      set right_item yes
    endif
  elseif %actor.quest_stage[emmath_flameball]% >= 0 && %actor.quest_stage[emmath_flameball]% <= 3
    set flameball_item yes
  endif
elseif %object.vnum% == 5211
  if %actor.quest_stage[pyromancer_subclass]% > 0 && %actor.quest_stage[pyromancer_subclass]% <= 4
    set subclass_item yes
    if %actor.quest_variable[pyromancer_subclass:part]% == white
      set right_item yes
    endif
  elseif %actor.quest_stage[emmath_flameball]% >= 0 && %actor.quest_stage[emmath_flameball]% <= 3
    set flameball_item yes
  endif
elseif %object.vnum% == 5212
  if %actor.quest_stage[pyromancer_subclass]% > 0 && %actor.quest_stage[pyromancer_subclass]% <= 4
    set subclass_item yes
    if %actor.quest_variable[pyromancer_subclass:part]% == gray
      set right_item yes
    endif
  elseif %actor.quest_stage[emmath_flameball]% >= 0 && %actor.quest_stage[emmath_flameball]% <= 3
    set flameball_item yes
  endif
endif
if %subclass_item%
  wait 2
  mjunk questobject
  switch %actor.quest_stage[pyromancer_subclass]%
    case 4
      if %right_item% == yes
        emote hops up and down rapidly.
        wait 1s
        msend %actor% %self.name% says, 'Goodness the spirit of the flame is complete once again!'
        shake %actor.name%
        grin
        wait 2s
        quest complete pyromancer_subclass %actor.name%
        mforce %actor% subclass
      else
        eyebrow
        msend %actor% %self.name% says, 'No, I already control this part of the flame.'
        wait 2s
        msend %actor% %self.name% says, 'I asked you to bring me the %actor.quest_variable[pyromancer_subclass:part]% one.'
      endif
      break
    case 3
      frown %actor.name%
      if %right_item% == yes
        msend %actor% %self.name% says, 'A shame you didn't retrieve the flame yourself.'
        wait 2s
        msend %actor% %self.name% says, 'Maybe you should do it yourself this time.'
      else
        msend %actor% %self.name% says, 'No, no.  You brought me the wrong flame!'
        wait 2s
        msend %actor% %self.name% says, 'I asked you to bring me the %actor.quest_variable[pyromancer_subclass:part]% one.'
      endif
      break
    case 2
    case 1
      msend %actor% %self.name% says, 'Curious...  I have not even told you what the quest is yet...'
      break
    default
      msend %actor% %self.name% says, 'What a lovely gift, it will fit nicely on my shelf.'
      thank %actor.name%
      wait 2s
      msend %actor% %self.name% says, 'Shame you were not performing a quest, you seem like a friend of fire.'
  done
elseif %flameball_item%
  if %actor.quest_stage[emmath_flameball]% == 1
    return 0
    wait 2
    frown
    msend %actor% %self.name% says, 'But I didn't even tell you what I wanted yet!'
    mecho %self.name% refuses %object.shortdesc%.
    wait 2s
    msend %actor% %self.name% says, 'How do you expect to gain power like this?'
  elseif %actor.quest_stage[emmath_flameball]% == 2
    if ((%object.vnum% == 5211) || (%object.vnum% == 5212) || (%object.vnum% == 17308))
      if %actor.quest_variable[emmath_flameball:%object.vnum%]% != 1
        eval count 1 + %actor.quest_variable[emmath_flameball:count]%
        quest variable emmath_flameball %actor.name% count %count%
        quest variable emmath_flameball %actor.name% %object.vnum% 1
      else
        return 0
        wait 2
        msend %actor% %self.name% says, 'You have already brought me %object.shortdesc%!'
        mecho %self.name% refuses %object.shortdesc%. 
      endif
      if %count%
        wait 2
        mjunk questobject
        switch %count%
          case 1
            msend %actor% %self.name% says, 'Excellent.  That's one.  Two to go.'
            break
          case 2
            msend %actor% %self.name% says, 'Good, good.  That's two.  One more!'
            break
          case 3
            quest advance emmath_flameball %actor.name%
            smile
            msend %actor% %self.name% says, 'I'm glad you were able to tame the three flames.'
            wait 2s
            scratch
            msend %actor% %self.name% says, 'There is one more, though, a renegade flame.'
            wait 1s
            sigh
            wait 2s
            msend %actor% %self.name% says, 'Some time ago, I battled my counterpart, the illustrious Suralla Iceeye.'
            emote looks thoughtful, reminiscing in his mind.
            wait 2s
            msend %actor% %self.name% says, 'Nothing terrible came of it, but she succeeded in...'
            wait 1s
            msend %actor% %self.name% says, '... changing one of my flames.'
            emote looks around himself.
            wait 3s
            msend %actor% %self.name% says, 'She was able to meld flame and ice, and form one neither cold nor warm.'
            wait 2s
            lick
            msend %actor% %self.name% says, 'I'm sure you understand when I say I cannot allow this flame to exist.'
            wait 3s
            ponder
            msend %actor% %self.name% says, 'Return it to me so that I might destroy it.'
            wait 7s
            msend %actor% %self.name% says, 'Well, don't wait around all day.'
        done
      endif
    elseif %object.vnum% == 23822
      mjunk blue-flame
      eye
      msend %actor% %self.name% says, 'I didn't ask you to bring me this yet.
      mecho %self.name% extinguishes %get.obj_shortdesc[23822]%.
    endif
  elseif %actor.quest_stage[emmath_flameball]% == 3
    if %object.vnum% == 23822
      wait 2
      mjunk blue-flame
      msend %actor% %self.name% says, 'Ah yes... the blue flame.'
      smile self
      wait 2s
      msend %actor% %self.name% says, 'Such a pity to destroy such an artifact as this.'
      emote pauses momentarily.
      wait 3s
      msend %actor% %self.name% says, 'But it must be done.'
      wait 1s
      emote crushes the blue flame in his hand, its essence evaporating into the air.
      wait 2s
      lick
      wait 2s
      look %actor.name%
      msend %actor% %self.name% says, 'Well now I suppose I owe you something, don't I?  You seem ready for the power.'
      quest erase emmath_flameball %actor.name%
      wait 2s
      msend %actor% %self.name% says, 'But remember!  With great power, comes great responsibility.'
      mload obj 5210
      give ball %actor.name%
    endif
  endif
endif
~
#5209
flameball_speech_ball~
0 d 1
ball ball? flameball flameball?~
wait 2
if %actor.vnum% == -1
   if %actor.quest_stage[emmath_flameball]% == 1
      sigh
      say You've already inquired about the ball of flame.
      wait 2s
      peer %actor.name%
      say I'm not sure you're prepared for such power.
   elseif %actor.quest_stage[emmath_flameball]% == 2
      sigh
      say You've already inquired about the ball of flame.
      wait 3s
      mecho %self.name% says, 'I told you to bring me the three parts of fire and we
      mecho &0would see about it.'
   elseif %actor.quest_stage[emmath_flameball]% == 3
      growl
      say Stop bothering me.  You know what I want already.
      wait 2s
      sigh
      say The renegade flame--bring it to me.
   else
      quest start emmath_flameball %actor.name%
      say You seek this ball of flame, do you?
      ponder
      wait 2s
      mecho %self.name% says, 'Ah, to hold it in your palm...  You would need to prove
      mecho &0your worth for such power.'
      emote looks thoughtful for a moment.
   endif
endif
~
#5210
flameball_speech_power~
0 d 1
power power? worth worth? prove prove? how how?~
wait 2
switch %actor.quest_stage[emmath_flameball]%
  case 1
    quest advance emmath_flameball %actor.name%
    msend %actor% %self.name% says, 'Yes, you would need to show mastery over the fire.'
    emote pulls out his well-used thinking cap, and begins to think.
    wait 2s
    msend %actor% %self.name% says, 'Bring me the three parts of flame: &7&bWhite&0, &bGray&0, and &9&bBlack&0.  I think we can talk again then.'
    wait 3s
    msend %actor% %self.name% says, 'Ask about your &1&b[quest progress]&0 if you need.'
    wait 7s
    msend %actor% %self.name% says, 'Well? Go on, then!'
    break
  case 2
    mutter %actor.name%
    msend %actor% %self.name% says, 'I told you, not until you bring me all three flames!'
    break
  case 3
    peer %actor.name%
    msend %actor% %self.name% says, 'Didn't we talk about this already?'
    wait 1s
    msend %actor% %self.name% says, 'Bring me the renegade flame!'
done
~
#5211
pyro_subclass_flameball_emmath_status~
0 d 0
subclass progress~
wait 2
switch %actor.quest_stage[pyromancer_subclass]%
  case 1
    msend %actor% %self.name% says, 'Only the best and most motivated of mages will complete the &1&bquest&0 I lay before you.'
    smile
    wait 2s
    msend %actor% %self.name% says, 'However, I am sure it is in you, if it is truly your desire, to complete this quest and become a pyromancer.'
    break
  case 2
    msend %actor% %self.name% says, 'You want to be a pyromancer?'
    wait 1s
    msend %actor% %self.name% says, 'I seem to have a problem now, because some time ago...'
    emote sighs, looking troubled.
    wait 2s
    msend %actor% %self.name% says, 'It is just...'
    wait 1s
    msend %actor% %self.name% says, 'Well, part of the essence of fire is no longer under my power.'
    emote shakes his head sadly.
    wait 2s
    msend %actor% %self.name% says, 'I once controlled all three parts of the flame: &7&bWhite&0, &bGray&0, and &9&bBlack&0.'
    wait 2s
    frown
    msend %actor% %self.name% says, 'But one of them was taken from my &1&bcontrol&0.'
    wait 1s
    sigh
    break
  case 3
  case 4
    msend %actor% %self.name% says, 'I'm giving you your quest to become a pyromancer.  Pay attention!'
    wait 2s
    msend %actor% %self.name% says, 'The %actor.quest_variable[pyromancer_subclass:part]% flame was taken from me.
    look %actor.name%
    wait 2s
    msend %actor% %self.name% says, 'To truly help, I suggest you stop loitering and go recover it.'
    ponder
    wait 2s
    switch %actor.quest_variable[pyromancer_subclass:part]%
      case white
        set place &bin some kind of mine&0
        break
      case black
        set place &bin some kind of temple&0
        break
      case gray
      default
        set place &bnear some kind of hill&0
    done
    msend %actor% %self.name% says, 'Last I heard, it was &6&b%place%&0, or something of the like.'
    break
  default
    if %actor.has_completed[pyromancer_subclass]%
      msend %actor% %self.name% says, 'You're already a pyromancer you ninny.'
    else
      switch %actor.race%
        case dragonborn_frost
        case arborean
          if %actor.level% >= 10 && %actor.level% <= 45
            msend %actor% &1Your race may not subclass to pyromancer.&0
            halt
          endif
          break
        default
          if %actor.level% >= 10 && %actor.level% <= 45
            msend %actor% %self.name% says, 'You aren't working to be a pyromancer.'
          elseif %actor.level% < 10
            msend %actor% %self.name% says, 'Not yet, for you are still an initiate.  Come back when you have gained more experience.'
          else
            msend %actor% %self.name% says, 'Unfortunately you are too dedicated to your universalist ways for me to teach you now.'
          endif
      done
    endif
done
~
#5212
**UNUSED**~
2 c 100
se~
return 0
~
#5213
Flameball quest status checker~
0 d 0
quest progress~
wait 2
switch %actor.quest_stage[emmath_flameball]%
  case 1
    msend %actor% %self.name% says, 'Do you still seek this ball of flame?'
    ponder
    wait 2s
    msend %actor% %self.name% says, 'Ah, to hold it in your palm...  You would need to prove your worth for such power.'
    emote looks thoughtful for a moment.
    break
  case 2
    msend %actor% %self.name% says, 'You need to show mastery over the fire.'
    emote pulls out his well-used thinking cap, and begins to think.
    wait 2s
    msend %actor% %self.name% says, 'Bring me the three parts of flame: &7&bWhite&0, &bGray&0, and &9&bBlack&0.  I think we can talk again then.'
    set black %actor.quest_variable[emmath_flameball:17308]%
    set white %actor.quest_variable[emmath_flameball:5211]%
    set gray %actor.quest_variable[emmath_flameball:5212]%
    if %black% || %white% || %gray%
      mecho  
      mecho You have brought me:
      if %white%
          mecho - &7&b%get.obj_shortdesc[5211]%&0
      endif
      if %gray%
          mecho - &b%get.obj_shortdesc[5212]%&0
      endif
      if %black%
          mecho - &9&b%get.obj_shortdesc[17308]%&0
      endif
    endif
    mecho  
    mecho I still need:
    if !%white%
      mecho - &7&b%get.obj_shortdesc[5211]%&0
    endif
    if !%gray%
      mecho - &b%get.obj_shortdesc[5212]%&0
    endif
    if !%black%
      mecho - &9&b%get.obj_shortdesc[17308]%&0
    endif
    wait 3s
    msend %actor% %self.name% says, 'Well?  Go on, then!'
    break
  case 3
    peer %actor.name%
    msend %actor% %self.name% says, 'Didn't we talk about this already?'
    wait 1s
    msend %actor% %self.name% says, 'Bring me the renegade &4&bblue flame&0!'
    break
  default
    msend %actor% %self.name% says, 'What quest?  You don't work for me.'
done
~
#5214
Emmath blue flame receive~
0 j 100
23822~
mat 23890 m_run_room_trig 23814
* flameball
if %actor.quest_stage[emmath_flameball]% == 2 && %actor.quest_stage[%type%_wand]% != %step%
  wait 2
  mjunk blue-flame
  eye
  msend %actor% %self.name% says, 'I didn't ask you to bring me this yet.'
  msend %actor% %self.name% extinguishes %get.obj_shortdesc[23822]%.
elseif %actor.quest_stage[emmath_flameball]% == 3
  wait 2
  mjunk blue-flame
  msend %actor% %self.name% says, 'Ah yes... the blue flame.'
  smile self
  wait 2s
  msend %actor% %self.name% says, 'Such a pity to destroy such an artifact as this.'
  emote pauses momentarily.
  wait 3s
  msend %actor% %self.name% says, 'But it must be done.'
  wait 1s
  emote crushes the blue flame in his hand, its essence evaporating into the air.
  wait 2s
  lick
  wait 2s
  look %actor.name%
  msend %actor% %self.name% says, 'Well now I suppose I owe you something, don't I.  You seem ready for the power.'
  quest erase emmath_flameball %actor.name%
  wait 2s
  msend %actor% %self.name% says, 'But remember!  With great power, comes great responsibility.'
  mload obj 5210
  give ball %actor.name%
endif
* phase wand
if %actor.quest_stage[%type%_wand]%
  eval minlevel (%step% - 1) * 10
  if %actor.level% < ((%step% - 1) * 10)
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 1s
    msend %actor% %self.name% says, 'You'll need to be at least level %minlevel% before I can improve your bond with your weapon.'
    halt    
  elseif %actor.has_completed[%type%_wand]%
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 1s
    msend %actor% %self.name% says, 'You already have the most powerful %type% %weapon% in existence!'
    halt
  elseif %actor.quest_stage[%type%_wand]% < %step%
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 1s
    msend %actor% %self.name% says, 'Your %weapon% isn't ready for improvement yet.'
    halt
  elseif %actor.quest_stage[%type%_wand]% == %step%
    set stage %actor.quest_stage[%type%_wand]%
    if %actor.quest_variable[%type%_wand:task2]%
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      msend %actor% %self.name% says, 'You already gave me this.'
    else
      quest variable %type%_wand %actor% task2 1
      wait 2
      mjunk %object%
      msend %actor% %self.name% says, 'This is just what I need.'
      wait 1s
      set job1 %actor.quest_variable[%type%_wand:task1]%
      set job2 %actor.quest_variable[%type%_wand:task2]%
      set job3 %actor.quest_variable[%type%_wand:task3]%
      if %job1% && %job2% && %job3%
        msend %actor% %self.name% says, 'Let me prime the %weapon%.'
      else
        msend %actor% %self.name% says, 'Now finish practicing with your %weapon%.'
      endif
    endif
  endif
endif
~
#5215
Emmath staff receive~
0 j 100
318~
eval minlevel (%step% - 1) * 10
if %actor.level% < ((%step% - 1) * 10)
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 1s
  msend %actor% %self.name% says, 'You'll need to be at least level %minlevel% before I can improve your bond with your weapon.'
  halt    
elseif %actor.has_completed[%type%_wand]%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 1s
  msend %actor% %self.name% says, 'You already have the most powerful %type% %weapon% in existence!'
  halt
elseif %actor.quest_stage[%type%_wand]% < %step%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 1s
  msend %actor% %self.name% says, 'Your %weapon% isn't ready for improvement yet.'
  halt
elseif %actor.quest_stage[%type%_wand]% == %step%
  set stage %actor.quest_stage[%type%_wand]%
  if %actor.quest_variable[%type%_wand:task4]%
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'I already primed your %weapon%.'
  else
    set job1 %actor.quest_variable[%type%_wand:task1]%
    set job2 %actor.quest_variable[%type%_wand:task2]%
    set job3 %actor.quest_variable[%type%_wand:task3]%
    if %job1% && %job2% && %job3%
      set room %get.room[%task4%]%
      quest variable %type%_wand %actor% task4 1
      wait 2
      msend %actor% %self.name% utters eldritch incantations over %object.shortdesc%.
      wait 2s
      mecho %object.shortdesc% begins to crackle with supreme elemental power!
      wait 1s
      msend %actor% %self.name% says, 'Now that you've captured the demon's energies, you must make your way deep into the elementals planes.  There, in the full glory of elemental %type%, find %room.name% and &6&bimbue&0 it with the power of the plane.'
      wait 6s
      msend %actor% %self.name% says, 'It will forge the most powerful %weapon% of %type% in all the realms!'
      give all %actor%
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      if !%job1%
        eval remaining ((%actor.quest_stage[%type%_wand]% - 1) * 50) - %actor.quest_variable[%type%_wand:attack_counter]%
        msend %actor% %self.name% says, 'You still need to attack %remaining% more times to fully bond with your %weapon%!'
        wait 1s
      endif
      if !%job2%
        msend %actor% %self.name% says, 'You have to give me %get.obj_shortdesc[%gem%]% first.'
        wait 1s
      endif
      if !%job3%
        msend %actor% %self.name% says, 'You still need to slay %get.mob_shortdesc[%task3%]%.'
      endif
    endif
  endif
endif
~
#5216
Emmath receive decline~
0 j 0
5211 5212 17308 23822 318~
switch %object.vnum%
  default
    if %actor.quest_stage[pyromancer_subclass]% > 0 && %actor.quest_stage[pyromancer_subclass]% <= 4
      set response I asked you to bring me the %actor.quest_variable[pyromancer_subclass:part]% flame, not this nonsense.
    elseif %actor.quest_stage[%type%_wand]% == %step%
      set response I can't craft with this!
    elseif %actor.quest_stage[emmath_flameball] > 1
      set response You're supposed to be out collecting flames, not whatever this is.
    else
      set response Why are you bringing me this trash?
    endif
done
if %response%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, '%response%'
endif
~
#5278
lava_death_trig~
2 g 100
(null)~
wait 2
while %self.people%
   set dice1 %random.60%
   set dice2 %random.60%
   eval damage %dice1% + %dice2%
   set victim %random.char%
   if %victim%
      wdamage %victim% %damage% fire
      if %damdone% == 0
         wechoaround %victim% A large lava bubble bursts near %victim.name%, but %victim.p% body absorbs the blast!
         wsend %victim% A large lava bubble bursts right next to you.  Luckily, you are well -protected.
      else
         wechoaround %victim% A large lava bubble bursts near %victim.name%, burning %victim.o%! (&1%damdone%&0)
         wsend %victim% A large lava bubble bursts right next to you - OUCH, that burns! (&1%damdone%&0)
      endif
      if %victim.quest_stage[meteorswarm]% == 3 &&%victim.quest_variable[meteorswarm:fire]% == 1
         quest advance meteorswarm %victim%
         wait 1s
         wsend %actor% &1You feel your mastery over fire growing!&0
      endif
   else
      halt
   end
   wait 2 s
done
~
$~
