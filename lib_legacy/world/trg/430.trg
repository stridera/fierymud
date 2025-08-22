#43001
Demise_dying_knight_greet~
0 g 100
~
wait 1s
mecho %self.name% coughs loudly.
mecho %self.name% screams in frustration!
mecho %self.name% says, 'Hey you! wait!'
wait 2s
mecho %self.name% shouts, 'Wait!'
mecho %self.name% coughs loudly, spewing blood.
mecho %self.name% says, 'Turn back, it's awful! you will all die!'
mecho %self.name% rolls over in fit of bile-filled coughing.
~
#43002
Demise_dying_knight_speech1~
0 d 1
them~
say monsters....everywhere...too many.
cough
~
#43003
Demise_dying_knight_speech2~
0 d 1
monster monsters~
say i am too weak
say its...look to the one who cannot...
emote spits up blood.
~
#43004
Demise_dying_knight_speech3~
0 d 1
awful~
emote says, 'the blood....all the blood, the monsters.'
cough
~
#43005
Demise_dying_knight_speech4~
0 d 1
help~
wince
mecho %self.name% says, 'kill me, don't let them get me!'
beg %actor.name%
~
#43006
Demise_dying_knight_rand1~
0 b 25
~
mecho %self.name% is mortally wounded and will die soon.
~
#43007
Demise_dying_knight_rand2~
0 b 22
~
groan
mecho %self.name% says softly 'help me....'
mecho %self.name% tries to pull himself off the trail onto a stump so as not to drown in his own blood.
~
#43008
Demise_dying_knight_rand3~
0 b 18
~
cough
mecho %self.name% spews blood and bile from his gaping wounds.
mecho %self.name% rolls over and stares blankly at the sky...
~
#43017
Demise_Cyprianum_Greet~
0 g 100
~
emote coils about itself at your approach!
emote booms, "Your life is forfeit! Prepare for battle!"
emote ROARS!
~
#43018
Demise_Cyprianum_Combat_1~
0 k 4
~
emote nearly flails you with a whiplike tentacle of energy!
say Your mortality binds you to slower flesh!
cackle
~
#43019
Demise_Cyprianum_Combat_2~
0 k 3
~
emote ROARS!
emote crushes the marble just inches from your feet!
say Soon you too will be among my slaves!
~
#43020
Demise_Cyprianum_Combat_3~
0 k 2
~
emote flairs a darker shade of red.
say You have no magic which can harm me!
emote ROARS!
~
#43021
Demise_Cyprianum_Death_Word_of_Command~
0 f 100
~
emote roars, 'Time is but a shell...'
mecho A non-descript wooden door bursts from Cyprianum!
mload obj 43026
set i %actor.group_size%
if %i%
   set a 1
else
   set a 0
endif
while %i% >= %a%
  set person %actor.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[word_command]% == 2
      quest advance word_command %person.name%
    endif
  elseif %person%
    eval i %i% + 1
  endif
  eval a %a% + 1
done
set room %self.room%
if %room.people[43021]%
   mecho %get.mob_shortdesc[43021]% says, 'At last!!  The way out!'
   mecho Lord Dargo dashes for the door!
   mforce dargo enter door
endif
~
#43022
Demise_Cronkneb_Combat_1~
0 k 8
~
emote hisses at you!
say My master will reward me for bringing him a fresh soul!
~
#43023
Demise_Cronkneb_Combat_2~
0 i 9
~
cackle
emote mumbles something under its breath.
~
#43024
Demise_Cronkneb_Combat_3~
0 k 4
~
emote throws back its head and howls!
emote bares its fangs at you!
~
#43027
Demise_Magical_Exit_43027~
2 g 100
~
wsend %actor.name% The magical portal shines even brighter and then fades as you pass through it.
~
#43028
not used - delete~
1 a 0
~
Nothing.
~
#43050
word_command_random_shout~
2 ab 1
~
if %get.mob_count[43021]% == 0
  wzoneecho 430 Someone shouts, 'Help!  Can anyone hear me?!  I'm trapped in the maze!  Help!!'
endif
~
#43051
word_command_dargo_preentry_plus_43157~
2 g 100
~
wsend %actor% Pillars of darkness converge on you, enfolding you in their nothingness!
wsend %actor% You feel yourself swept even deeper into the keep.
if %get.mob_count[43021]% == 0
  wload mob 43021
endif
~
#43052
word_command_dargo_greet~
0 h 100
~
wait 2
if %self.room% == 43148
  if %questor%
    say Thank the gods you found me again!  Help me get out of here!
  elseif (%actor.class% /= Priest || %actor.class% /= Diabolist) && %actor.level% > 72
    say Oh thank the gods, please help me!  Someone used a scroll
    mecho &0of World Teleport on me as a joke and I wound up here!'
    wait 1s
    mecho %self.name% says, 'I've been trying desperately to escape this place but every
    mecho &0time I think I found a way out, the demon reaper who rules this place
    mecho &0keeps pulling me back.'
    wait 2s
    say I don't know how long I've been down here...
    sob
    wait 2s
    say Help me confront the demon and his servants and destroy them!
    wait 3s
    mecho %self.name% says, 'If you can get me out of this hell, I can teach you a
    mecho &0powerful spell.  Will you help me?'
  else
    say Do you have any priestly friends who can protect us from the
    mecho &0demon who rules this keep??  Or even someone who knows the demonic arts?'
  endif
endif
~
#43053
word_command_dargo_speech1~
0 d 100
yes yes? sure okay yeah~
wait 2
if %questor% == %actor.name%
  say Lead on!
elseif %actor.has_completed[word_command]%
  emote stars in wide-eyed terror.
  wait 2s
  say Don't I know you already?!  Cyprianum is tormenting me again!!
  wait 1s
  mecho %self.name% screams, 'NOOOOOOOO!!!!!' as he runs wildly into the shadows!
  mpurge %self%
elseif (%actor.class% /= Priest || %actor.class% /= Diabolist) && %actor.level% > 72
  say Thank you!
  if !%actor.quest_stage[word_command]%
    quest start word_command %actor.name%
  endif
  set questor %actor.name%
  global questor
  fol %actor%
else
  mecho %self.name% says, 'I don't know if you can protect me from the demonic forces
  mecho &0here...  You'll need in-depth knowledge of demons to get us out of
  mecho &0here alive!'
endif
~
#43054
word_command_dargo_escape~
0 i 100
~
wait 2
if !%questor%
  halt
endif
if %self.room% == 43037
  if %get.mob_count[43017]% > 0
    mat 43176 mforce cyprianum shout You can never escape me!
    mecho &1%self.name% vanishes in a flash of light and crimson haze!&0
    emote shrieks as he disappears!
    mecho Cackling laughter echoes through the halls.
    mteleport %self% 43148
  else
    set i %questor.group_size%
    if %i%
      set a 1
    else
      set a 0
    endif
    while %i% >= %a%
      set person %questor.group_member[%i%]%
      if %person.room% == %self.room%
        if %person.quest_stage[word_command]% == 3
          set escape 1
        endif
      elseif %person%
        eval i %i% + 1
      endif
      eval a %a% + 1
    done
    if !%escape%
      mat 43092 mload mob 43016
      mecho Cyprianum the Reaper shouts, 'You didn't think you could escape without confronting me and my servant, did you Dargo?'
      mat 43176 mload mob 43017
      mecho %self.name% shrieks as he vanishes in a flash of light and crimson haze!
      mteleport %self% 43148
    endif
  endif
elseif %self.room% == 43176
  if %get.mob_count[43016]% > 0
    mecho %get.mob_shortdesc[43016]% shouts, 'I will never allow you to harm my master!'
    mecho &1&b%self.name% is consumed in a crackle of red lightning and vanishes!&0
    mecho Cackling laughter echoes through the maze!
    mteleport %self% 43148
  endif
elseif %self.room% == 43000
  if %get.mob_count[43017]% > 0
    mat 43176 mforce cyprianum shout You can never escape me!
    mecho &1%self.name% vanishes in a flash of light and crimson haze!&0
    emote shrieks as he disappears!
    mecho Cackling laughter echoes through the halls.
    mteleport %self% 43148
  else
    cheer
    say Thank you so much!
    set i %questor.group_size%
    if %i%
      set a 1
    else
      set a 0
    endif
    while %i% >= %a%
      set person %questor.group_member[%a%]%
      if %person.room% == %self.room%
        if %person.quest_stage[word_command]% == 3
          t %person% As promised, here is the spell.
          msend %person% %self.name% gives you a disintegrating scroll with an ancient spell on it.
          mskillset %person.name% word of command
          msend %person% &5&bYou have learned Word of Command!&0
          quest complete word_command %person.name%
        elseif %person.vnum% == -1
          t %person% To show my gratitude, take this.
          set count 0
            while %count% < 3
              set what_gem_drop %random.11%
              eval gem_vnum %what_gem_drop% + 55736
              mload obj %gem_vnum%
              eval count %count% + 1
            done
          give all.gem %person%
        endif
      elseif %person%
        eval i %i% + 1
      endif
      eval a %a% + 1
    done
    mecho %self.name% waves farewell!
    mpurge %self%
  endif
endif
~
#43055
word_command_voliangloch_death~
0 f 100
~
mecho %self.name% says, 'Cyprianum will never allow you to escape this place!'
set i %actor.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %actor.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[word_command]% == 1
      quest advance word_command %person.name%
    endif
  elseif %person%
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#43056
word_command_dargo_kill~
0 b 100
~
set room %self.room%
set person %room.people%
while %person%
  if %person.vnum% != -1 && %person.vnum% != 43021 
    if !%person.flagged[illusory]%
      mforce %person% say You will never escape this place, Dargo!
      mforce %person% kill %self%
    endif
  endif
  set person %person.next_in_room%
done
~
#43057
word_of_command_dargo_order~
0 c 100
order~
if %arg% /= enter door
  if %actor.quest_stage[word_command]% == 3
    if %self.room% == 43176
      enter door
    else
      mecho %self.name% looks frantically for a door and panics!
    endif
  elseif %actor.quest_stage[word_command]%
    say I'm not free from the curse yet!
  endif
else
  return 0
endif
~
#43099
Demise_BlueOrbs_Racing~
2 g 100
~
wsend %actor% What seems like hundreds of small, blue orbs circle about here.
wsend %actor% A group of these race towards you, quickly surrounding you!
wsend %actor% The corridor is suddenly illuminated by an ambient glow.
~
#43100
Demise_BlueOrbs_Abandon~
2 g 100
~
wsend %actor% You are plunged into darkness as the small, blue orbs abandon you!
~
#43101
Demise_Treasure_Trap~
2 g 100
~
wsend %actor% You have fallen into a magical trap!
wsend %actor% Reality ripples and you feel yourself magically transported!
~
#43128
Demise_Magical_Exit_43128~
2 g 100
~
wsend %actor% Wisps of smoke rise from a spot on the northern wall.
~
#43157
Demise_43157_Magical_Exits~
2 g 100
~
wsend %actor% Pillars of darkness converge on you, enfolding you in their nothingness!
wsend %actor% You feel yourself swept even deeper into the keep.
~
#43250
Demise_Keeper_NoEntry~
0 c 100
north~
msend %actor% %self.name% steps in your way.
mechoaround %actor% %actor.name% moves toward the door, but %self.name% steps in %actor.p% way.
msend %actor% %self.name% eyes you warily.
mechoaround %actor% %self.name% eyes %actor.name% warily.
say None shall pass but those of our order.
say Turn back, traveller...live your days well.
~
#43251
Demise_Exit_Portal~
2 g 100
east~
wsend %actor.name% A hole torn in the fabric of reality shimmers above you.
~
#43252
Demise_Keeper_Bloodlust~
0 g 100
~
grin
say I'm going to enjoy taking you out!
emote springs into motion!
~
#43253
Demise_Keeper_Outrage~
0 g 100
~
gasp
say You were warned!
~
#43254
Demise_StaffSeblan_Fire~
1 j 100
~
oecho Blue flames enfold %actor.name%, the master of the Staff of Seblan!
~
#43255
Demise_BlackScimitar_Wield~
1 j 100
~
oecho Shadows appear over %actor.name%, so vivid that they seem tangible.
oecho Suddenly they dive into %actor.name%'s body, merging with %actor.p% soul!
~
#43256
Demise_AckbrinRun_Restrict~
0 c 100
open~
frown
say You are not of our order.
emote folds his arms across his chest, blocking the way.
~
$~
