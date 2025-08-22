#8700
blacksmith quest~
0 g 100
~
wait 1s
msend %actor% %self.name% says, 'Hello traveler, I was wondering if you had seen a dwarven
msend %actor% &0boy heading this way?  My supplies are late, they should have been here a day
msend %actor% &0ago.  Doren is never late, I wish i could go looking for him.  I fear something
msend %actor% &0has happened to him, will you go find him?'
~
#8701
Helping_blacksmith_quest~
0 d 100
yes yes.~
wait 2
if %get.mob_count[8712]% == 0
  msend %actor% %self.name% says, 'Thank you, thank you I will ever be in your debt if you
  msend %actor% &0find Doren.  He will be traveling here from the dwarven mines near Anduin.  If
  msend %actor% &0you find Doren tell him that his uncle sent you.  Please hurry!  He could be 
  msend %actor% &0hurt or in trouble!'
  mat 55932 mload mob 8713
  mat 55932 mload mob 8713
  mat 55932 mload mob 8712
  wait 1s
  mat 55932 mforce doren mload obj 8709
  wait 1s
  mat 55932 mforce doren drop handcart
else
  msend %actor% %self.name% says, 'Please go find Doren, he could be hurt.'
endif
~
#8702
drag the handcart ~
1 c 4
drag~
return 0
wait 1s
if %arg% /= cart ||  %arg% /= wagon
   osend %actor% The handcart creaks along behind you.
   oechoaround %actor% The handcart creaks along behind %actor.name%.
   if %actor.room% == 8711
      wait 1s
      oforce blacksmith blink %actor.name%
      oforce blacksmith emote rubs his eyes in disbelief.
      osend %actor.name% The blacksmith says, 'Oh it's you, you found the handcart, thank the gods!'
      wait 1s
      oforce blacksmith mload obj 8700
      osend %actor.name% The blacksmith says, 'I must reward such heroism, and I know just the thing.'
      oforce blacksmith give axe %actor.name%
      oforce blacksmith emote moves the handcart into a bay near his tools.
      opurge %self%
   elseif %random.6% == 3
      osend %actor% &2You hear a rustling in the grass nearby.&0
      wait 1s
      oload mob 8713
      oforce bandit kill %actor%
      oload mob 8713
      oforce bandit kill %actor%
   endif
endif
~
#8703
doren greet~
0 g 100
~
if %actor.vnum% == -1
  wait 1s
  shout Help!  Help!
  mforce bandit kill %actor%
  wait 1s
  mecho &2A bandit flees from %actor.name% into the undergrowth.&0
  mpurge 2.bandit
  wait 10s
  emote limps away from the bandit, blood running down his leg.
  wait 2s
  msend %actor% %self.name% says, 'Thank you for saving me.  I can make it home but I must go now.  
  msend %actor% &0Please take the handcart to my uncle in the logging camp to the east.  He is
  msend %actor% &0the blacksmith there.  He will reward you for saving me and getting the 
  msend %actor% &0handcart safely to him.'
  wait 2s
  mecho &2Doren limps though the grass heading for home.
  mpurge %self%
endif
~
#8709
muleard_greet~
0 g 100
~
if %actor.vnum% == -1
   wait 2s
   msend %actor% %self.name% turns around and looks at you.
   wait 1s 
   tell %actor.name% Welcome to my store.
   wait 1s 
   tell %actor.name% You can call me Mule.
endif
~
#8710
ambush~
1 c 100
drag~
oecho checking..
if %random.10% < 2
oecho proc
endif
~
#8711
anti-get trigger~
1 g 100
~
if %actor.size% == tiny || %actor.size% == small || %actor.size% == medium || %actor.size% == large || %actor.size% == huge
   return 0
   odamage %actor% 50 crush
   if %damdone% != 0
      osend %actor% Your back buckles and pain shoots though you joints! (&1&b%damdone%&0)
      osend %actor% You just can't hold that much weight!
      oechoaround %actor% %actor.name% shouts in pain. (&4%damdone%&0)
      osend %actor% You drop %self.shortdesc%.
      oechoaround %actor% %actor.name% drops %self.shortdesc%.
   else
      osend %actor% You can't seem to get a grip on %self.shortdesc%.
   endif
endif
~
#8750
anti_thief_obj_trigger~
1 j 100
~
if %actor.class% /= thief
return 0
osend %actor% You cannot use %self.shortdesc%.
else
return 1
endif
~
#8751
to_small_wield~
1 j 100
~
if %actor.size% == tiny || %actor.size% == small || %actor.size% == medium
osend %actor% It's too big for you!
return 0
else
return 1
endif
~
#8758
white_mask_wear~
1 j 100
~
wait 1s
osend %actor.name% As you place the white mask to your face, it starts to flow untill it conforms to your face. 
oechoaround %actor.name% a white mask flows like liquid as it conforms to %actor.name%'s face.
return 1
~
#8791
Tr'ven (Speak fix)~
0 d 100
fix~
wait 10 
say Yes i can fix just about anything that is broken or outdated.
wait 5
say Lots of old Adventures have been bringing me items like ivory weddingbands.
wait 5 
say or Armor That they have Earned from there guilds and haveing me fix them up.
wait 5 
say to be as good as the items the would be rewarded with today.
wait 5
say just hand me items and I'll get started
~
#8795
Tr'ven(greet)~
0 g 100
~
wait 1s
if %actor.vnum% == -1
   say Welcome to my shop. Is there something I can fix for you?
endif
~
#8796
tr'ven(replace old items)~
0 j 100
~
if %object.vnum% => 1 
   set item %object.vnum%
   set name %object.shortdesc%
   wait 1s
   say %name%?
   mpurge %object%
   wait 1s
   say Yes, I think I can make %name% as good as new.
   mload obj %item%
   wait 1s
   msend %actor% &bThe air shudders as Tr'ven mutters a few words over %name%.&0
   mechoaround %actor% The air shudders as Tr'ven mutters a few words over %name%.
   wait 1s
   emote holds up %name% and smiles at a job well done.
   wait 2s
   give all %actor%
endif
~
#8797
skillset_skills_A-G~
0 d 1
skillset ready go cancel 2H backstab bandage barehand bash bludgeoning bodyslam chant conceal corner  disarm dodge doorbash double douse dual eye first group guard~
wait 1s
if %skill% && %mortal% && %command% 
   if %speech% == go && %actor.level% >= 101
      mskillset %mortal% %skill%
      say Done. Did it work?
   elseif %speech% == cancel
      say Ok, lets start over, starting with the command..
   endif
   unset command
   unset mortal
   unset skill
elseif %skill% && %command%
   set mortal %actor.name%
   global mortal
   say Ok, imm, if you want to %command% %skill% to %mortal%, just say go!
elseif %command%
   say Ok, I'll be %command%ing %speech%.
   say Mortal, if you are ready to get %speech%, say "ready".
   set skill %speech%
   global skill
elseif %speech% == skillset
   set command mskillset
   global command
   say what skill will I be setting?
endif
~
#8798
skillset_skills_H-V~
0 d 1
skillset ready go cancel hide hitall instant kick meditate  mount  pick parry piercing quick rescue  retreat riding safefall  scribe riposte shadow shape slashing sneak spell sphere springleap steal stealth summon switch tame throatcut track vamp ~
wait 1s
if %skill% && %mortal% && %command% 
if %speech% == go && %actor.level% >= 101
      mskillset %mortal% %skill%
      say Done. Did it work?
   elseif %speech% == cancel
      say Ok, lets start over, starting with the command..
   endif
   unset command
   unset mortal
   unset skill
elseif %skill% && %command%
   set mortal %actor.name%
   global mortal
   say Ok, imm, if you want to %command% %skill% to %mortal%, just say go!
elseif %command%
   say Ok, I'll be %command%ing %speech%.
   say Mortal, if you are ready to get %speech%, say "ready".
   set skill %speech%
   global skill
elseif %speech% == skillset
   set command mskillset
   global command
   say what skill will I be setting?
endif
~
#8799
skillset_questspells~
0 d 1
skillset ready go cancel banish blur charm creeping plane heavens dragons flood hell hellfire ice major relocate meteorswarm resurrect shift degeneration supernova wall vaporform word wizard aria seed apocalyptic group~
wait 1s
if %skill% && %mortal% && %command%
   if %speech% == go && %actor.level% >= 101
      mskillset %mortal% %skill%
      say Done. Did it work?
   elseif %speech% == cancel
      say Ok, lets start over, starting with the command..
   endif
   unset command
   unset mortal
   unset skill
elseif %skill% && %command%
   set mortal %actor.name%
   global mortal
   say Ok, imm, if you want to %command% %skill% to %mortal%, just say go!
elseif %command%
   say Ok, I'll be %command%ing %speech%.
   say Mortal, if you are ready to get %speech%, say "ready".
   set skill %speech%
   global skill
elseif %speech% == skillset
   set command mskillset
   global command
   say what skill will I be setting?
endif
~
$~
