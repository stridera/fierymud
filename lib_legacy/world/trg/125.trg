#12500
Bloodworm_attack~
0 g 25
~
* Basically, let them stew in the pit for a little bit before they get whacked.
if %actor.level% < 100
wait 14s
kill %actor.name%
endif
~
#12501
Ring_Bell~
2 c 100
pull~
if %arg% /= rope
  wsend %actor% As you pull the rope, you think you hear the faint ringing of a bell. 
  wechoaround %actor% As %actor.name% pulls the rope, you seem to hear a bell ringing.
  if %get.obj_count[12521]% || %get.obj_count[12522]% 
    wat 12602 wpurge field
  endif
  if %get.obj_count[12521]% || %get.obj_count[12522]% 
    wat 12602 wpurge field
  endif
  wat 12602 wload obj 12521
  wat 12602 wload obj 12522
  wat 12602 wecho The echo of a bell ringing echoes through the cavern.
  wat 12602 wecho In response, glowing fields envelope the tunnel entrances to the west and east.
else
  return 0
endif
~
#12502
Slam_door_trigger~
2 g 100
~
if %actor.vnum% == -1
wait 2s
wecho The door slams shut with a loud click!
wdoor 12533 east flags abcde
wdoor 12534 west flags abcd
endif
~
#12503
brother~
0 d 100
brother Brother brother? Brother?~
switch %actor.quest_stage[krisenna_quest]%
case 0
case 1
   wait 1s
   mecho %self.name% says, 'Our father warned him not to go... But he wouldn't
   mecho &0listen.'
   wait 2s
   say Now he's probably dead...
   wait 1s
   emote starts bawling like a baby. 
   wait 3s
   emote sobs, 'You've got to find him!!  Will you find him?'
   break
case 2
   wait 1 s
   say He's dead??  Oh no!
   wait 2 s
   cry
   wait 2 s
   emote dries his tears on his bloodstained sleeve.
   wait 1 s
   mecho %self.name% says, 'He carried our grandfather's warhammer.  The
   mecho &0warhammer is very precious to my family.  Would you please find it?
   mecho &0I will reward you as best I can!'
   break
default
   wait 1 s
   mecho %self.name% says, 'He was very dear to me... foolhardy, but a good
   mecho &0brother.'
done
~
#12504
Drop_rune~
2 h 100
~
if (%object.vnum% == 12504)
  wait 1
  wpurge %object%
  wait 5
  wecho The runestone flares green as it falls into place.
  wait 1s
  wecho As the stone settles, a field of green energy spreads across the circle of stones. 
  wload obj 12505
  wait 1s
endif
~
#12505
Trapped~
0 d 100
trapped Trapped trapped? Trapped?~
wait 1s
say They say the key is at the top of the tower. Watch your step though...
wait 2s
say When I tried, I fell through the floor, spraining my ankle.
wait 2s
say If you could free my brother, my family would be quite thankful.
wait 2s
msend %actor.name% %self.name% looks at you pleadingly.
mechoaround %actor% %self.name% looks at %actor.name% pleadingly.
~
#12506
Get_Book~
1 g 100
~
odamage %actor% 100 slash
if %damdone% == 0
   return 1
else
   return 0
   osend %actor% As you grab the book, blades rip through your hand - it must have been trapped! (&3&b%damdone%&0)
   oechoaround %actor% %actor.name% grabs the green book, and screams in pain as blades rip through %actor.p% hand. (&3&b%damdone%&0)
end
~
#12507
Fall_into_pit~
2 g 40
~
Nothing.
~
#12508
Statue_trigger~
2 b 10
~
wecho Blue crackles of lightning seem to crawl up the statues leg!
wait 2s
wecho Before your very eyes, the top of the leg glows blue, and starts to liquefy. 
wait 2s
wecho The leg resolidifies, with more of the thigh than before intact! 
~
#12509
Dragon Guards Chest~
0 c 0
open chest~
msend %actor.name% As you reach towards the chest, %self.name% swipes at you, forestalling the attempt!
mechoaround %actor.name% %self.name% guards the chest as %actor.name% approaches it.
~
#12510
Unused trigger?~
1 j 63
~
oexp %actor% -100000
osend %actor.name% As you grab the fist, you feel a slight jolt, and feel your life force being drained. 
~
#12511
injured halfling speech start~
0 d 100
hi hello hi? hello? progress progress? status status?~
switch %actor.quest_stage[krisenna_quest]%
case 0
   wait 1s
   emote looks panicked.
   wait 1s
   say You've got to help me...
   break
case 1
   wait 1 s
   mecho %self.name% says, 'Have you found my brother yet?  He must be in the
   mecho &0tower somewhere!'
   break
case 2
   wait 1 s
   say He's dead??  Oh no!
   wait 2 s
   cry
   wait 2 s
   emote dries his tears on his bloodstained sleeve.
   wait 1 s
   mecho %self.name% says, 'He carried our grandfather's warhammer.  The
   mecho &0warhammer is very precious to my family.  Would you please find it?
   mecho &0I will reward you as best I can!'
   break
case 3
case 4
   wait 1 s
   say A... demon has the warhammer, you say?
   wait 2 s
   cringe
   wait 2 s
   mecho %self.name% says, 'I must have that warhammer!  Losing my brother is
   mecho &0bad enough!'
   break
default
   * You already returned the warhammer to him.
   wait 1 s
   say I thank you again for finding my brother, friend.
   wait 2 s
   say I will be returning home soon...
done
~
#12512
Injured Halfling help~
0 d 100
help help?~
switch %actor.quest_stage[krisenna_quest]%
case 0
   wait 1s
   say You'll help??
   wait 1s
   emote looks quite relieved. 
   wait 3s
   mecho %self.name% says, 'About a month ago, my brother came here and we
   mecho &0haven't heard from him since.'
   wait 2s
   mecho %self.name% says, 'They say he went into the tower, never to be seen
   mecho &0again.'
   wait 1s
   emote bursts into tears.
   wait 4s
   say I went after him, but I got hurt quite badly.
   wait 1s
   emote limps about a bit. 
   wait 3s
   say Could you... find my brother?
   break
case 1
   wait 1 s
   mecho %self.name% says, 'Have you found my brother yet?  He must be in the
   mecho &0tower somewhere!'
   break
case 2
   wait 1 s
   say He's dead??  Oh no!
   wait 2 s
   cry
   wait 2 s
   emote dries his tears on his bloodstained sleeve.
   wait 1 s
   mecho %self.name% says, 'He carried our grandfather's warhammer.  The
   mecho &0warhammer is very precious to my family.  Would you please find it?
   mecho &0I will reward you as best I can!'
   break
case 3
case 4
   wait 1 s
   say A... demon has the warhammer, you say?
   wait 2 s
   cringe
   wait 2 s
   mecho %self.name% says, 'I must have that warhammer!  Losing my brother is
   mecho &0bad enough!'
   break
default
   * You already returned the warhammer to him.
   wait 1 s
   say I thank you again for finding my brother, friend.
   wait 2 s
   say I will be returning home soon...
done
~
#12513
Red_wall~
1 c 100
west~
return 1
set which %random.10%
if %which% == 4
   osend %actor%  You feel a burning sensation, but push through!
   oechoaround %actor% %actor.name% pushes through the red field!
   oteleport %actor% 12608
   oforce %actor% look
else
   odamage %actor% 75 fire
   if %damdone% == 0
      return 1
   else
      return 0
      osend %actor% The red field burns you, and you are forced back! (&1%damdone%&0)
      oechoaround %actor% %actor.name% is forced back by the red field. (&1%damdone%&0)
   end
end
~
#12514
Blue_field~
1 c 100
east~
odamage %actor% 115 shock
if %damdone% == 0
   return 0
else
   return 1
   osend %actor% The blue field shocks you, flinging you back into the room! (&4&b%damdone%&0)
   oechoaround %actor% %actor.name% is forced back by the blue field. (&4&b%damdone%&0)
end
~
#12515
SilentOneEnter~
0 g 100
~
wait 1s
msend %actor.name% %self.name% looks at you pleadingly. 
~
#12516
Demon_block~
0 c 100
open door~
msend %actor.name% The demon stands before the door, guarding it. 
mechoaround %actor.name% The demon guards the door, preventing %actor.name% from opening it. 
~
#12517
Lichs Ruby~
1 g 127
~
osend %actor.name% As you grab the gem, you feel memories and strength drain from you. 
oechoaround %actor.name%  As %actor.name% grabs the gem, it glows brightly for a moment. 
oexp %actor.name% -100000
~
#12518
SilentOneBrother~
2 a 100
~
wait 1s
wecho The Silent One's voice cracks as he starts to speak.
wait 2s
wforce silent emote says in a ragged voice, 'My brother... sent you?'
wait 1s
wecho The Silent One's eyes brighten.
wait 1s
wecho The Silent One says, 'Our grandfather's warhammer was stolen by Krisenna, the
wecho &0 demon lord.  The portal to his realm is nearby, but a stone to activate it is
wecho &0 missing.'
wait 3s
wforce silent say Please, find it, and return the hammer to my brother.
wait 3s
wforce silent emote clutches his wounds as his eyes roll back, and he collapses.
wdamage silent 20000
~
#12519
BrothersReunion~
0 j 100
~
if %object.vnum% == 12502
   return 1
   if %actor.quest_stage[krisenna_quest]% == 4
      mjunk warhammer
      wait 1s
      emote weeps as he realizes that his brother is dead.
      wait 3s
      mecho %self.name% says, 'Thank you for finding him.  I will take word to my
      mecho &0family.'
      wait 1s
      say Please, take this for your efforts.
      wait 1s
      say It was our great grandfather's.
      mload obj 12550
      give monocle %actor.name%
      wait 2s
      say And take these as well.
      set gem 0
      while %gem% < 3
         eval drop %random.11% + 55703
         mload obj %drop%
         eval gem %gem% + 1
      done
      give all.gem %actor.name%
      quest complete krisenna_quest %actor.name%
      *
      * Set X to the level of the award - code does not run without it
      * 
      if %actor.level% < 40
         set expcap %actor.level%
      else
         set expcap 40
      endif
      set expmod 0
      if %expcap% < 9
         eval expmod (((%expcap% * %expcap%) + %expcap%) / 2) * 55
      elseif %expcap% < 17
         eval expmod 440 + ((%expcap% - 8) * 125)
      elseif %expcap% < 25
         eval expmod 1440 + ((%expcap% - 16) * 175)
      elseif %expcap% < 34
         eval expmod 2840 + ((%expcap% - 24) * 225)
      elseif %expcap% < 49
         eval expmod 4640 + ((%expcap% - 32) * 250)
      elseif %expcap% < 90
         eval expmod 8640 + ((%expcap% - 48) * 300)
      else
         eval expmod 20940 + ((%expcap% - 89) * 600)
      endif
      *
      * Adjust exp award by class so all classes receive the same proprotionate amount
      *
      switch %actor.class%
         case Warrior
         case Berserker
         *
         * 110% of standard
         * 
            eval expmod (%expmod% + (%expmod% / 10))
            break
         case Paladin
         case Anti-Paladin
         case Ranger
         *
         * 115% of standard
         *
            eval expmod (%expmod% + ((%expmod% * 2) / 15))
            break
         case Sorcerer
         case Pyromancer
         case Cryomancer
         case Illusionist
         case Bard
         *
         * 120% of standard
         *
            eval expmod (%expmod% + (%expmod% / 5))
            break
         case Necromancer
         case Monk
         *
         * 130% of standard
         *
            eval expmod (%expmod% + (%expmod% * 2) / 5)
            break
         default
            set expmod %expmod%
      done
      msend %actor% &3&bYou gain experience!&0
      eval setexp (%expmod% * 10)
      set loop 0
      while %loop% < 10
      *
      * Xexp must be replaced by mexp, oexp, or wexp for this code to work
      * Pick depending on what is running the trigger
      *
         mexp %actor% %setexp%
         eval loop %loop% + 1
      done
      wait 2s
      mecho %self.name% becomes very silent.
   else
      wait 2s
      say Why yes, this -was- mine, thanks!
      mjunk warhammer
   endif
else
   return 0
   mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
   msend %actor% You give %object.shortdesc% to %self.name%.
   wait 8
   say I don't want this.
   msend %actor% %self.name% returns %object.shortdesc% to you.
   mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
endif
~
#12520
SilentOneRRTrig~
0 d 100
brother Brother~
set stage 1
set person %actor%
set cap 4
set i %person.group_size%
if %i%
   set a 1
else
   set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
      if %person.quest_stage[krisenna_quest]% < %cap%
        set run yes
        if %person.quest_stage[krisenna_quest]% == %stage%
            quest advance krisenna_quest %person.name%
            msend %person% &7&bYou have furthered the quest!&0
        endif
      endif
  elseif %person%
      eval i %i% + 1
  endif
  eval a %a% + 1
done
if %run%
   m_run_room_trig 12518
endif
~
#12521
EnterDemonessRoom~
0 h 100
~
wait 2s
glare %actor.name%
wait 1s
mecho %self.name% says, 'The master will be displeased...  He would want me to kill
mecho &0you.'
~
#12522
FireBreather~
1 b 100
~
set room %get.room[%self.room%]%
if %room.people%
   oecho The dragon starts to rumble.
   wait 2s
   oecho The dragon blasts a gout of &1&bflame&0, incinerating the room.
   set prsn %random.char%
   set dmg %random.100%
   eval dmg %dmg% + 50
   odamage %prsn% %dmg% fire
   if %damdone% == 0
      if %fireproof%
         oechoaround %prsn.name% A fiery blast is absorbed by %prsn.name%.
         osend %prsn.name% Your body is hit by a &1fiery&0 blast! Luckily you absorb the blast.
      else
         oechoaround %prsn.name% %prsn.name% is caught in the fiery blast! (&1%damdone%&0)
         osend %prsn.name% You are caught in the &1fiery&0 blast! (&1%damdone%&0)
      end
   end
end
~
#12523
DemonessMaster~
0 d 100
master Master ~
wait 1s
mecho %self.name% says, 'Yes, my master.  We're to keep the unworthy out, to give
mecho &0him peace.'
wait 2s
say I don't know what to do with you.  Perhaps he will know.
emote primly folds her hands, closes her eyes, and becomes motionless.
wait 10s
msend %actor% %self.name% opens her eyes and refocuses them upon you.
mechoaround %actor% %self.name% opens her eyes, and refocuses them upon %actor.name%.
wait 2 s
say Very well, you may enter...  if you dare.
wait 1s
mpurge portal
mload obj 12543
emote waves her hand, causing the visage of a demon to appear.
~
#12524
Demoness kill~
0 d 100
kill Kill~
wait 1s
mecho %self.name% says, 'Well, yes.  The master likes his peace...  You will disturb
mecho &0his peace.'
wait 1s
msend %actor% %self.name% glares at you. 
mechoaround %actor% %self.name% glares at %actor.name%.
~
#12525
DemonLordBow~
0 c 100
bow~
switch %cmd%
  case b
  case bo
    return 0
    halt
done
set person %actor%
set i %person.group_size%
if %i%
   set a 1
else
   set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[krisenna_quest]% == 2
        set krisenna %person.quest_stage[krisenna_quest]%
        quest advance krisenna_quest %person.name%
        msend %person% &7&bYou have advanced the quest!&0
        set go zone
    endif
  elseif %person%
    eval i %i% + 1
  endif
  eval a %a% + 1
done
if %actor.quest_stage[hell_trident]% == 2
  set hell %actor.quest_stage[hell_trident]%
  if %actor.level% >= 90
    if !%actor.quest_variable[hell_trident:helltask5]%
      if %actor.has_completed[resurrection_quest]%
        quest variable hell_trident %actor% helltask5 1
      endif
    endif
    if !%actor.quest_variable[hell_trident:helltask4]%
      if %actor.has_completed[hell_gate]%
        quest variable hell_trident %actor% helltask4 1
      endif
    endif
    if %actor.quest_variable[hell_trident:greet]% == 1
      unset go
      set go trident
    else
      set go zone
    endif
  endif
endif
if %go%
  msend %actor% You bow before him.
  mechoaround %actor% %actor.name% bows before %self.name%.
  wait 1s
  if %go% == trident
    say I see you still remember your manners.
    wait 1s
    msend %actor% %self.name% says, 'Why have you returned?  Have you brought your sacrifices?'
    if %krisenna% == 2
      wait 1s
      msend %actor% %self.name% says, 'Or is it something else?'
    endif
  elseif %go% == zone
    say Ah yes, the respect I deserve.
    wait 1s
    peer %actor.name%
    wait 2s
    say What brings you to me?
    wait 2
    if %hell% == 2
      msend %actor% %self.name% says, 'Your quest for a new &6&b[trident]&0 perhaps?'
      if %krisenna% == 2
        wait 1s
        msend %actor% %self.name% says, 'Or is it something else?'
      endif
    endif
  endif
else
  return 0
endif
~
#12526
DemonGreed~
0 d 100
greed money pouch  wergeld wergild~
if %actor.quest_stage[krisenna_quest]% == 3
   quest advance krisenna_quest %actor.name%
   wait 1s
   say Ahh yes, the sin of sins.  Very well, I will give you my coins.
   wait 2s
   say But, you must promise to not let others know about us.
   wait 1s
   say Saving ourselves the trouble would be worth it.
   wait 2s
   emote stops using a pouch full of money.
   mload obj 12544
   drop pouch
   say Now...
   wait 2
   say BEGONE!
endif
~
#12527
DemonAdventure~
0 d 100
adventure explore exploration curiousity morning star~
if %actor.quest_stage[krisenna_quest]% == 3
   quest advance krisenna_quest %actor.name%
   wait 1s
   say Such curiosity can be quite dangerous.  Very well.
   wait 2s
   say My morning star would be quite a trophy of your exploits...
   wait 2s
   emote stops using Krisenna's morning-star.
   mload obj 12545
   drop star
   say Now...
   wait 2
   say BEGONE!
endif
~
#12528
DemonHorn~
0 d 100
horn horns~
if %actor.quest_stage[krisenna_quest]% == 3
   quest advance krisenna_quest %actor.name%
   wait 1s
   mecho %self.name% says, 'Well, I suppose I must honor my obligations, and you have
   mecho &0certainly earned it.'
   wait 1s
   emote grasps a horn with both hands, grunts, and breaks it off.
   wait 1s 
   say Not as painful as I thought.
   wait 2s
   mload obj 12554
   drop horn
endif
~
#12529
Krisenna refuse~
0 j 0
55739 2339~
return 0
mecho %self.name% refuses %object.shortdesc%.
wait 2
snarl %actor%
say You try to appease me with trinkets?  Bah.
~
#12530
SwordDeath~
0 fl 20
20~
m_run_room_trig 12531
return 0
mgoto 12594
~
#12531
RoomForSwordDeath~
2 d 100
automagic_trigger_from_12532~
wload obj 12527
wait 1s
wecho The dancing sword falls to the ground, lifeless. 
~
#12532
StoneCultWhat~
0 bg 25
~
set delay %rand.5%
wait %delay%s 
emote bows before the statue. 
wait 2
emote chants, 'Bring her back to flesh, back to life.'
~
#12533
TowerZap~
2 b 10
~
set rnd %random.10%
if %rnd% == 3
wecho A bolt of blue energy bursts from the tower, singing your arm! Ouch!
else
wecho A bolt of blue energy eminates from the tower, striking a rock. 
wait 3s
wecho Before your eyes, the rock is pulled against the tower. 
wait 2s
wecho The rock is absorbed into the tower!
wait 4s
wecho Crackles of energy crawl up the side of the tower. 
endif
~
#12534
DemonLordBo~
0 c 100
bo~
return 0
~
#12535
Krisenna greet~
0 h 100
~
if %actor.quest_stage[hell_trident]% == 2
  if %actor.level% >= 90
    wait 2
    if !%actor.quest_variable[hell_trident:helltask5]%
      if %actor.has_completed[resurrection_quest]%
        quest variable hell_trident %actor% helltask4 1
      endif
    endif
    if !%actor.quest_variable[hell_trident:helltask4]%
      if %actor.has_completed[hell_gate]%
        quest variable hell_trident %actor% helltask5 1
      endif
    endif
    if %actor.quest_variable[hell_trident:greet]% == 1
      set job1 %actor.quest_variable[hell_trident:helltask1]%
      set job2 %actor.quest_variable[hell_trident:helltask2]%
      set job3 %actor.quest_variable[hell_trident:helltask3]%
      set job4 %actor.quest_variable[hell_trident:helltask4]%
      set job5 %actor.quest_variable[hell_trident:helltask5]%
      set job6 %actor.quest_variable[hell_trident:helltask6]%
      if %job1% && %job2% && %job3% && %job4% && %job5% && %job6%
        msend %actor% %self.name% says, 'Your soul is ready to strike a pact.  Give me the trident to seal it.'
      else
        msend %actor% %self.name% says, 'Have you returned to meet my demands?'
      endif
    endif
  endif
endif
~
#12544
FlaminKey~
1 g 100
2~
set burn 1
wait 1
if %actor.class% /= Cry
   osend %actor.name% Your cool touch makes the fiery key's heat bearable. 
   set burn 0
endif
if %actor.class% /= Pyr
   set burn 0
   osend %actor.name% Your hands are accustomed to the heat, making the key easy to handle. 
endif
if %burn% == 1
   odamage %actor.name% 50 fire
   if %damdone% != 0
      osend %actor% OUCH!  You grab the key, but it's burning your hand! (&1%damdone%&0)
      oechoaround %actor% %actor.name% yelps as the key burns %actor.p% hand! (&1%damdone%&0)
   end
end
~
#12545
Injured halfling help~
0 d 100
yes Yes~
wait 2
set person %actor%
set i %person.group_size%
if %i%
   set a 1
else
   set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
      if %person.quest_stage[krisenna_quest]% < 2
        set instruct 1
        if !%person.quest_stage[krisenna_quest]%
            quest start krisenna_quest %person.name%
            msend %person% &7&bYou have now begun the Tower in the Wastes quest!&0
        endif
      endif
  elseif %person%
      eval i %i% + 1
  endif
  eval a %a% + 1
done
if %instruct%
   mecho %self.name%'s eyes brighten.
   wait 1s
   mecho %self.name% says, 'If you find him, tell him his brother is looking for him.'
   wait 1s
   mecho %self.name% says, 'If you need an update, you can check your &6&b[progress]&0 with me.'
endif
~
#12546
DemonWarhammer~
0 d 100
brother vengeance revenge hammer warhammer halfling~
if %actor.quest_stage[krisenna_quest]% == 3
   quest advance krisenna_quest %actor.name%
   msend %actor% &7&bYou have furthered the quest!&0
   msend %actor% &7&bGroup credit will not be awarded for the next step of this quest.&0
   wait 1s
   mecho %self.name% says, 'Its owner's death was such a shame, but unavoidable nonetheless.'
   wait 2s
   say Please, return his hammer to his family.
   wait 2s
   emote stops using a large stone warhammer.
   mload obj 12502
   drop hammer
endif
~
#12599
test~
0 d 100
foo~
mdoor 12500 north room 3002
~
$~
