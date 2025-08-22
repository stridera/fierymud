#62501
calling for help~
0 k 33
~
if %self.vnum% == 62501
   if %get.mob_count[62502] < 5
      mecho &2&b%self.name% touches a nearby Rhell tree, calling on it for help.&0
      wait 1s
      mecho &2&b A Mighty Rhell Tree springs to life!&0
      wait 1s
      mload mob 62502
      mforce tree assist dryad
   else
      halt
   endif
endif
if %self.vnum% == 62508
   wait 3 seconds
   if %get.mob_count[62509]% > 5
      halt
   elseif if %random.10% > 6
      if %self.aff_flagged[SILENCE]%
         mecho &3&b%self.name% opens its mouth to howl but no sound comes out!&0
         halt
      else
         mecho &3%self.name% lets out a deep howl, calling for others in the pack.&0
         set wolf %random.13%
         wait 2s
         if %wolf% > 6
            mecho &3&bYou hear several answering howls! The PACK is coming to join %self.name%!&0
            wait 2s
            mload mob 62509
            mforce wolf assist 2.wolf
            wait 5s
            mload mob 62509
            mforce wolf assist 2.wolf
         endif
      endif
   endif
endif
if %self.vnum% == 62509
   if %get.mob_count[62509]% > 4
      halt
   elseif %random.10% > 6
      wait 3s
      set wolf %random.15%
      wait 1s
      if %self.aff_flagged[SILENCE]%
         mecho &3&b%self.name% opens its mouth to howl but no sound comes out!&0
         halt
      else
         mecho &3&b%self.name% howls, calling for others in the pack.&0
         wait 2s
         if %wolf% < 2
            mecho &3You hear a deep howl answering %self.name%'s call for help.&0
            wait 2s
            mload mob 62508
            mforce wolf assist 2.wolf
         elseif %wolf% < 5
            mecho &3&bYou hear a distant howl. Someone is coming to the aid of %self.name%!&0
            wait 2s
            mload mob 62509
            mforce wolf assist 2.wolf
         endif
      endif
   endif
endif
~
#62502
tree's rescue~
0 ko 100
~
rescue dryad
~
#62503
rhell death~
0 f 100
~
if %random.10% < 4
mload obj 62502
endif
~
#62504
ursa's room~
2 a 100
~
set pop %self.people[count]%
set count 0
set target %self.people%
wechoaround Ursa &9&bUrsa makes your frame quake with a vicious &1ROOOOOAAAAAARRRRRR!&0
while %count% < %pop%
   eval dam 1.5*%target.level% - %random.12%
   if %target.vnum% != 62507 && %target.vnum% != 62550 && %target.level% < 100
      if %target.class% == RANGER || %target.class% == DRUID
         wsend %target% &3Ursa's roar invigorates your wild spirit!&3&b&0 (&1&b0&0)
         wechoaround %target% &3Ursa's roar invigortates %target.name%'s wild spirit!&0 (&1&b0&0)
      else
         wdamage %target% %dam%
         wsend %target% &3&bYou shrink in pain at Ursa's mighty roar!&0 (&1&b%damdone%&0)
         wechoaround %target% &3Ursa's roar causes %target.name% to shrink in pain.&0 (&4%damdone%&0)
      endif
   endif
   set target %target.next_in_room%
   eval count %count% + 1
done
~
#62505
Treant refuse~
0 j 0
62504~
switch %object.vnum%
  case %wandgem%
  case %wandvnum%
  case %wandtask3%
    halt
    break
  default
    wait 1s
    grumble
    drop %object%
done
~
#62506
random treeant grumbles about dogs~
0 b 100
~
if %random.10% > 6
   grumble
   mecho %self.name% says, 'I've kept my trees for an age, and I've never had pests quite
   mecho &0like these before.'
endif
~
#62507
mild -> Ursa~
0 k 100
100~
set mode %random.10%
if %mode% > 7
   set actor %actor.name%
   mechoaround %self% &3&b%self.name% falls to his knees, letting out a disturbing ROAR!!&0
   wait 2s
   mechoaround %self% &3&b%self.name% stretches to twice his normal size!&0
   wait 2s
   mechoaround %self% &3&b%self.name% contorts into a wearbear!&0
   wait 1s
   mpurge %self%
   mload mob 62507
   mdamage ursa 200
   if %actor.vnum% != 62506
      mforce ursa kill %actor%
   endif
endif
~
#62508
Ursa -> mild~
0 b 50
~
*This needs to change him back to merchant if out of a fight for some time
set here %self.room%
set look %get.people[%here.vnum%]%
if %look% == 1
   emote calms down, and reverts to his normal self.
   mload m 62506
   mpurge self
else
   halt
endif
~
#62509
Ursa's roar~
0 gk 100
~
set chance %random.10%
wait 1s
if %chance% < 4
   wait 1s
   m_run_room_trig 62504
elseif %chance% < 6
   hitall
elseif %chance% < 8
   roar
endif
~
#62510
lowbee blocker~
0 c 100
south~
if %actor.vnum% == -1
   if %actor.level% < 20
      return 1
      whisper %actor.name% You've wandered too far.
      whisper %actor.name% Try again in a few levels.
      wink %actor.name%
   else
      return 0
   endif
else
   return 0
endif
~
#62511
ursa greet~
0 g 100
~
if %actor.vnum% == -1
   if %actor.level% < 100
      stand
      roar
      hitall
      msend %actor% %self.name% snarls at you.
      mechoaround %actor% %self.name% snarls at %actor.name%.
   endif
endif
~
#62512
trees miss~
0 k 100
~
set smash %random.100%
if %smash% < 30
   mecho A branch from %self.name% smashes the ground!
   return 1
   halt
else
   return 0
endif
~
#62515
treeant dialog~
0 d 1
pests pest trees~
growl
mecho %self.name% says, 'In the Month of the Stranger I first started seeing the feral
mecho &0dogs.  They make tunnels under the trees to eat their roots.'
wait 1s
mutter
mecho %self.name% says, 'I'd like to tear them out of there and...  But I can't get to
mecho &0them.  If you kill them all, and bring me their pelts...  I'll make it worth
mecho &0your time.'
wink
~
#62516
treant receive~
0 j 100
62504~
if %get.mob_count[62500]% > 0
  wait 2
  mpurge %object%
  wait 8
  say You've done pretty well clearing out those pests for me.  The trees thank you, and so do I.
  emote grabs his shoulder and peels some bark off.
  wait 1s
  say Accept this token of gratitude on behalf of the forest.
  mload obj 62506
  give badge %actor%
elseif %get.mob_count[62500]% == 0
  wait 2
  mpurge %object%
  wait 8
  say You've done exceptionally well clearing out those pests for me.  The trees thank you, and so do I.
  emote grabs a branch growing off his back and breaks it off!
  wait 1s
  say Accept this token of gratitude on behalf of the forest.
  mload obj 62503
  give limb %actor%
endif
~
#62517
ursa load~
0 o 100
~
mskillset %self% hitall
mskillset %self% roar
roar
set hit %random.char%
if %hit.vnum% != 62506
   kill %hit%
else
   halt
endif
~
#62518
merchant dialogue~
0 dn 1
help yes sacred spring power permanent solution powerful people hi hello hurt ok okay~
wait 1s
set speech %speech%
if %actor.vnum% == -1
  if %actor.quest_stage[ursa_quest]% < 1
    if %speech% /= help || %speech% /= help? || %speech% /= ok || %speech% /= yes
      quest start ursa_quest %actor.name%
      emote looks at you.
      wait 1s
      msend %actor% %self.name% says, 'You'll help me?!'
      halt
    elseif %speech% /= hi || %speech% /= hello
      wait 1s
      msend %actor% %self.name% says, 'Please, I'm very ill.  Can you &6&bhelp&0 me?'
      halt
    elseif %speech% /= hurt || %speech% /= hurt?
      wait 1s
      msend %actor% %self.name% says, 'I'm very sick.  The disease makes me lose control.'
      wait 2s
      msend %actor% %self.name% says, 'When I do, I... hurt people.'
      shudder
      wait 2s
      msend %actor% %self.name% says, 'Please &6&bhelp&0 me!
    endif
  endif
  if %actor.quest_stage[ursa_quest]% == 1
    if %speech% /= yes
      msend %actor% %self.name% says, 'I am suffering from werebear lycanthropy in its early stages.'
      wait 1s
      msend %actor% %self.name% says, 'The &6&bpower of this sacred spring&0 is halting the disease's progression.'
      halt
    elseif %speech% /= sacred || %speech% /= spring || %speech% /= power
      msend %actor% %self.name% says, 'As long as I am near this spring, and no one gives fuel to the disease's rage...'
      wait 1s
      msend %actor% %self.name% says, 'I can retain my true form.'
      wait 2s
      msend %actor% %self.name% says, '...for now.'
      wait 2s
      msend %actor% %self.name% says, 'But, I must find a more &6&bpermanent solution&0.'
      halt
    elseif %speech% /= permanent || %speech% /= solution
      sigh
      wait 1s
      msend %actor% %self.name% says, 'Many say there is no cure for lycanthropy.'
      wait 1s
      msend %actor% %self.name% says, '...aside from death.'
      wait 1s
      shudder
      wait 2s
      msend %actor% %self.name% says, 'But in my travels I've met many &6&bpowerful people&0.  At least one of them has to know more...'
      wait 1s
      emote hesitates for a moment, grasping for words.
      wait 1s
      msend %actor% %self.name% says, '...more favorable remedy.'
      halt
    elseif %speech% /= powerful || %speech% /= people
      msend %actor% %self.name% says, 'On a nearby island are a very refined people.  They've found cures for many things, and are in good favor with powerful gods.  Perhaps the Lord of their island will know how to help me.'
      wait 6s
      msend %actor% %self.name% says, 'I also know that Blackmourne has always shown a lot of interest and ability in diseases of the mind.  I've traveled to the Red City many times to bring the cultists fresh...'
      wait 4s
      emote hesitates.
      wait 2s    
      msend %actor% %self.name% says, '...supplies.  Perhaps one of them might help.'
      wait 6s
      msend %actor% %self.name% says, 'Then there's the old nut in the swamp.  If all else fails, maybe he's got some sort of remedy.'
      wait 1s
      msend %actor% %self.name% says, 'Whichever one you manage to contact, they will know who I am.  Just bring me back news of what can be done to help me.  I'm sure one of them will know what to do.'
      halt
    endif
  endif
  if %actor.quest_stage[ursa_quest]% > 1
    msend %actor% %self.name% says, 'Please, what do you have for me?'
    halt
  endif
endif
~
#62519
Rhell merchant refuse~
0 j 100
~
if %actor.has_completed[ursa_quest]%
  set response I don't really need this.
elseif !%actor.quest_stage[ursa_quest]%
  set response I don't think this will help me.
elseif %actor.quest_stage[ursa_quest]% == 1
  switch %object.vnum%
    case 62511
    case 62510
    case 62512
      halt
      break
    default
      set response This isn't helpful.
  done
endif
if !%response%
  halt
else
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 1s
  say %response%
  if %actor.quest_stage[ursa_quest]% == 1
    wait 2
    say Maybe you could ask someone who might know.
  endif
endif
~
#62520
merchant greet~
0 g 100
~
wait 2
if %actor.vnum% == -1
   if %actor.quest_stage[ursa_quest]% < 1
      set greet %random.100%
      if %greet% > 70
         say I don't want to hurt you!
      elseif %greet% > 40
         say Stay back!  I don't want to hurt anyone else.
      elseif %greet% > 20
         say Please, help me.
      else
         emote cowers, hoping you'll pass by.
      endif
   elseif %actor.quest_stage[ursa_quest]% > 0
      say Do you have anything for me?
   endif
endif
~
#62521
Dire Wolf Death~
0 f 100
~
if %random.10% > 5
   mload obj 62513
endif
~
#62522
Rhell Merchant receive 1-1~
0 j 100
62511~
* the emperor's letter is returned, 'choice' is set to path 1: item is pepper, from graveyard.
if %actor.quest_stage[ursa_quest]% == 1
  wait 1s
  quest variable ursa_quest %actor.name% choice 1
  quest advance ursa_quest %actor.name% 
  say News from the Emperor?!  This is fantastic!
  wait 1s
  emote breathes a sigh of relief.
  wait 2s
  emote reads %object.shortdesc%.
  mjunk %object%
  wait 3s
  say He tells me this spring, if it's the spring he's read about in the archives, has the power to heal me, but not without the help of a few other items of power.
  wait 1s
  emote continues to read.
  wait 2s
  say The first thing his instructions call for is peppercorn.  I'm not sure where to get that, but sales prices for pepper always always seem lowest in the Anduin area.  Please bring me some.
endif
~
#62523
Rhell Merchant receive 1-2~
0 j 100
62510~
*if the player returns the letter from Ruin, 'choice' is set to path 2: the golden sceptre of imanhotep
if %actor.quest_stage[ursa_quest]% == 1
  wait 2
  mjunk %object%
  quest variable ursa_quest %actor.name% choice 2
  quest advance ursa_quest %actor.name% 
  wait 8
  gasp
  wait 1s
  say Ruin has unlocked the secrets of lycanthropy?!
  wait 2s
  emote skims over the letter.
  wait 3s
  frown
  wait 2s
  say I'm going to need some more help, it seems.
  wait 2s
  say Ruin speaks of an ancient king that once overcame his own case of lycanthropy.  He suggests if I follow this king's procedure closely, I may have a slight chance.
  wait 2s
  say He doesn't inspire much hope, does he?
  sigh
  wait 2s
  say At any rate, the first thing he mentions here is a particular gold sceptre that symbolized the king's undying leadership.
  wait 1s
  say Such a symbol would likely have made its way into the hands of other mighty rulers.  Find the sceptre and bring it to me.
endif
~
#62524
Rhell Merchant receive 1-3~
0 j 100
62512~
*if the player returns the letter from the hermit, 'choice' is set to path 3: the ring of stolen life
if %actor.quest_stage[ursa_quest]% == 1
  wait 2
  mjunk %object%
  quest variable ursa_quest %actor.name% choice 3
  quest advance ursa_quest %actor.name% 
  wait 8
  say He has always told me he's found a cure for everything he's ever heard of.  I just need you to help gather some things.
  wait 2s
  say He says a devourer has a ring with power to heal.  Do you know what ring he means?  Please, go get this ring.  Devourers are drawn to magic, and burrow to avoid the sun.  That is all I know.
endif
~
#62525
Rhell Merchant receive 2-1~
0 j 100
~
*for path 1, after pepper, the merchant needs a plant, found in the form of 'a bit of bones and plants' from blue fog trail.
if %actor.quest_stage[ursa_quest]%  == 2
  if %actor.quest_variable[ursa_quest:choice]% == 1
    if %object.vnum% == 23755
      wait 2
      quest advance ursa_quest %actor.name%
      mjunk %object%
      wait 1s
      say Good!  You managed to find some pepper.
      wait 1s
      say I've been looking over this next item, and it should be a lot less hassle.
      wait 2s
      say Just west of here, there is a plant that roots to sticks and rocks, and even beings if they hold still long enough.  I need some of that plant.  It shouldn't be too hard.
      wait 1s
      say ... I think.
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say This isn't what I need right now.  Please bring me some pepper.
    endif
  endif
endif
~
#62526
Rhell Merchant receive 2-2~
0 j 100
~
*Path 2: if the golden sceptre is returned, the merchant asks for the sunstone diadem from tech.
if %actor.quest_stage[ursa_quest]%  == 2
  if %actor.quest_variable[ursa_quest:choice]% == 2
    if %object.vnum% == 16201
      wait 2
      mjunk %object%
      quest advance ursa_quest %actor.name%
      wait 1
      emote runs his hand down the shaft of the golden sceptre.
      wait 2s
      say What power!
      wait 2s
      say The king also wore an emblem of the sun.
      wait 1s
      say Ruin believes this emblem to be the very one written of in the legends of the warring gods in the far north.
      wait 2s
      say I feel the sceptre longing for it.  Find the sunstone and bring it to me.
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say This isn't the king's golden sceptre.
    endif
  endif
endif
~
#62527
Rhell Merchant receive 2-3~
0 j 100
~
*Path 3: ring of stolen life is returned, merchant asks for the golden druidstaff
if %actor.quest_stage[ursa_quest]%  == 2
  if %actor.quest_variable[ursa_quest:choice]% == 3
    if %object.vnum% == 12538
      wait 2
      mjunk %object%
      quest advance ursa_quest %actor.name%
      wait 1s
      emote looks at the ring.
      say Incredible!  I can feel it's magic working!
      wait 2s
      say Now I need the... gothen droolstall?  I can't read his chicken scratch!  But he says it's in the Highlands, with "their" leader.
      wait 1s
      blink
      wait 2s
      Who's leader?!
      wait 1s
      grumble
      say I guess you'll just have to search the Highlands.  Let me know when you figure out what the hermit meant.
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say I don't think this is the ring...
    endif
  endif
endif
~
#62528
Rhell Merchant receive 3-1~
0 j 100
~
*for path 1, after the plant, comes the thorny staff, from The Citadel of Betrayal.
if %actor.quest_stage[ursa_quest]%  == 3
  if %actor.quest_variable[ursa_quest:choice]% == 1
    if %object.vnum% == 11810
      wait 2
      quest advance ursa_quest %actor.name%
      wait 1s
      emote looks at %object.shortdesc%.
      mjunk %object%
      wait 1s
      say What an odd item to have rooted itself to.
      chuckle
      wait 1s
      say Now, I need a particular thorny wood.  Because of it's unpleasant nature there are some unpleasant people that make staves and walking sticks out of it.
      wait 1s
      say Please find one, and bring it back here.  I have almost everything called for on these instructions.
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say This isn't it.  What I'm after right now is a plant from the Blue-Fog Trail that binds itself to just about anything.
    endif
  endif
endif
~
#62529
Rhell Merchant receive 3-2~
0 j 100
~
*Path 2: after bringing the sunstone diadem, merchant asks for the radiant dagger, an mload in sacred haven
if %actor.quest_stage[ursa_quest]%  == 3
  if %actor.quest_variable[ursa_quest:choice]% == 2
    if %object.vnum% == 55014
    wait 2
      quest advance ursa_quest %actor.name%
      mjunk %object%
      wait 1s
      emote inspects the stone.
      wait 2s
      say You are quite resourceful.  This was the ancient king's pendant.
      wait 2s
      say It is said the sunstone captured the sun's light over 2 ages.  Now it shines with that stolen light.
      wait 3s
      say There is a dagger that the king used that does quite the opposite.
      wait 3s
      say It absorbs no light, it seems, but is made of such fine gold and gems that it radiates light.  The priests of South Caelia recognize its fierce beauty.  You will find it there still.
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say This isn't the sunstone.  Find and return it.
    endif
  endif
endif
~
#62530
Rhell Merchant receive 3-3~
0 j 100
~
*Path 3: after the druidstaff, the merchant asks for milk
if %actor.quest_stage[ursa_quest]%  == 3
  if %actor.quest_variable[ursa_quest:choice]% == 3
    if %object.vnum% == 16305
      wait 2
      quest advance ursa_quest %actor.name%
      mjunk %object%
      wait 1s
      say Oh, Golden Druidstaff!  Well, now that makes a lot of sense.
      wait 1s
      say Now we are to crush up this ring, and put the powder in milk.  Please, bring me some milk.
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say I don't think this is the Golhen DrubStatt the hermit had in mind.
    endif
  endif
endif
~
#62531
Rhell Merchant receive 4-1~
0 j 100
~
*for path 1, the merchant thinks he's done, but realizes he needs a pitcher to pour water over the other items.
if %actor.quest_stage[ursa_quest]% == 4
  if %actor.quest_variable[ursa_quest:choice]% == 1
    if %object.vnum% == 8516
      wait 2
      quest advance ursa_quest %actor.name%
      mjunk %object%
      wait 1s
      say Excellent!  Now I have everything I need: the pepper, the plant, the staff, this spring's water...
      wait 2s
      emote stares blankly at the spring.
      wait 3s
      say So...
      wait 1s
      say I need to get the water in something functional.  Can you go get a pitcher?  That would do the job wonderfully.
      smile %actor%
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say This isn't the thorny wood I need.
    endif
  endif
endif
~
#62532
Rhell Merchant receive 4-2~
0 j 100
~
*for path 2, the merchant asks for hard liquor - firebreather is best.
if %actor.quest_stage[ursa_quest]% == 4
  if %actor.quest_variable[ursa_quest:choice]% == 2
    if %object.vnum% == 59012
      wait 2
      mjunk %object%
      quest advance ursa_quest %actor.name%
      wait 1s
      say Fantastic.  Rylee knew it had too much power to be wielded by just anyone.  Only this dagger can end the curse inside me.
      wait 1s
      say Now, it says here that the king was placed in his sarcophagus and slew the beast inside himself.
      wait 2s
      say I'm willing to do that... but not sober.  Fetch me something to drink, and make it strong!
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say What is this garbage?  I need the king's jeweled dagger.  Only it has the radiant power to kill the beast.
    endif
  endif
endif
~
#62533
Rhell Merchant receive 4-3~
0 j 100
~
if %actor.quest_stage[ursa_quest]% == 4
  if %actor.quest_variable[ursa_quest:choice]% == 3
    *for path 3, the merchant asks for an anvil
    *note - the anvil is extremely heavy but must be picked up and given to the merchant to complete the quest; it cannot just be dragged to him.
    if %object.type% == LIQCONTAINER
      if %object.val2% == 10
        wait 2
        mjunk %object%
        quest advance ursa_quest %actor.name%
        wait 1s
        say Excellent.  Now to crush this ring...
        emote drops the ring of stolen life.
        emote wields the Golden Druidstaff.
        wait 3s
        mecho &2&b%self.name% &0&2SMACKS&2&b the ring, which &0&2bounces&2&b and lands lightly back on the dirt.&0
        wait 3s
        blink
        wait 3s
        mecho &2&b%self.name% &0&2SMACKS&2&b the ring, which &0&2bounces&2&b and lands lightly back on the dirt.&0
        wait 3s
        grumble
        mecho %self.name% says, This isn't going to work.  Off the Great Road is a lumber mill.  Their smith has an anvil that will do perfectly.
      else
        wait 2
        mecho %self.name% examines %object.shortdesc%.
        wait 1s
        say Good, now put some milk in this, and bring it to me again.
        give %object% %actor%.
      endif
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say I'm not sure how one gets milk out of this.
    endif
  endif
endif
~
#62534
Rhell Merchant receive 5-1~
0 j 100
~
*final step on good quest: either the pitcher from dancing dolphin or hot springs will work; merchant is healed and the thorny staff transforms into the redeeming staff.
if %actor.quest_stage[ursa_quest]% == 5
  if %actor.quest_variable[ursa_quest:choice]% == 1
    if %object.vnum% == 10309 || %object.vnum% == 58706
      wait 1s
      mjunk %object%
      quest advance ursa_quest %actor.name%
      wait 1s
      m_run_room_trig 62550
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say This just won't work.  Really, what I need is a pitcher, so I can quickly and accurately pour a large amount of water.
    endif
  endif
endif
~
#62535
Rhell Merchant receive 5-2~
0 j 100
~
if %actor.quest_stage[ursa_quest]% == 5
  if %actor.quest_variable[ursa_quest:choice]% == 2
    *extra step for evil path: merchant drinks and asks for a big bag - either a saddle or the tattered bag will work
    if %object.type% == LIQCONTAINER
      wait 1s
      drink %object%
      wait 1s
      if %object.val2% == 7
        say This is the good stuff!
        quest advance ursa_quest %actor.name%
        wait 1s
        drink %object%
        wait 1s
        say Okay...  Now I'm ready.  All I need now is something to kill myself in.
        wait 1s
        emote looks around for a makeshift sarcophagus.
        wait 2s
        say Do you have something big enough for me to fit in?  Like a big box or a bag or something?
      else 
        spit %actor.name%
        wait 1s
        say This isn't going to cut it, kid.
        wait 2s
        say Got anything stronger?
        give %object% %actor.name%
      endif
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say I'm looking for a drink.  A strong one!
    endif
  endif
endif
~
#62536
Rhell Merchant receive 5-3~
0 j 100
~
*final step on neutral quest: merchant turns into Ursa and fights!
*note - the anvil is extremely heavy but must be picked up and given to the merchant to complete the quest; it cannot just be dragged to him.
if %actor.quest_stage[ursa_quest]% == 5
  if %actor.quest_variable[ursa_quest:choice]% == 3
    if %object.vnum% == 8702
      wait 2
      mjunk %object%
      quest advance ursa_quest %actor.name%
      wait 1s
      m_run_room_trig 62550
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say This won't do the job.  I need the anvil.
    endif
  endif
endif
~
#62537
Rhell Merchant receive 6~
0 j 100
~
if %actor.quest_stage[ursa_quest]% == 6
  if %actor.quest_variable[ursa_quest:choice]% == 2
    if %object.type% == CONTAINER
      wait 2
      mecho %self.name% examines %object.shortdesc%.
      if %object.val0% > 120
        * The following regular items can be picked up and have large enough values: 3147, 16102, 55029
        wait 2
        mjunk %object%
        m_run_room_trig 62550
      else
        wait 1s
        shake
        say I don't think I can fit in here.
        give %object% %actor%
      endif
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      say I need a body-bag or a large chest...
    endif
  endif
endif
~
#62549
undefined~
0 f 0
~
halt
~
#62550
quest room~
2 a 100
run~
set quester %self.people%
if %quester% == 0
  wforce mild say Where did you go?
  halt
endif
while %quester.quest_stage[ursa_quest]% != 6
  set quester %quester.next_in_room%
done
if %quester.quest_stage[ursa_quest]% == 6
  *now find the right script to run, based on that players questvar: choice
  if %quester.quest_variable[ursa_quest:choice]% == 1
    wait 5
    wecho The mild mannered merchant says, 'Fantastic.  Now let's see what miracle the Emperor has found.'
    wforce mild emote digs a slight divot into the ground with the end of the thorny staff.
    wforce mild emote places the pepper and plant into the divot.
    wforce mild emote leaves the thorny staff standing erect out of the ground.
    wait 2s
    wforce mild hold pitcher
    wforce mild pour pitcher out
    wforce mild emote gently fills the pitcher from the Sacred Spring.
    wait 1s
    wforce mild emote pours sacred water over the thorny staff.
    wecho The ground at the base of the staff quickly soaks up the water as it trickles down.
    wait 3s
    wecho At the base of the &7thorny staff&0 &2&blife&0 breaks forth!
    wecho A &2vine&0 crawls up the &7staff&0, weaving between the &7thorns&0.
    wait 1s
    wecho The &2vine completely covers the &0&7thorny&0 &2staff.&0
    wait 2s
    wecho The mild mannered merchant reaches out his hand and grasps the &7thorny&0 &2staff&0.
    wecho The &2vine&0 soaks up the &1blood&0 as the &7thorns&0 pierce the merchant's hand.
    wait 2s
    wecho &3The merchant contorts wildly, shifting unevenly between man and beast, but retains his grasp on the &2staff.&0
    wait 2s
    wecho The mild mannered merchant removes his wounded hand, and the &2vine&0 recedes back into the earth, taking with it the blood and the thorns.
    wforce mild emote takes a moment to catch his breath.
    wait 3s
    wforce merchant emote pulls &2The Redeeming Staff&0 from the ground.
    *load staff and give it to the player
    wait 2s
    wload o 62509
    wforce mild get staff
    wforce mild emote runs his hand down the smooth staff.
    wait 2s
    wecho The mild mannered merchant says, 'I can not thank you enough!  I have myself back!'
    wait 1s
    wforce mild give staff %quester%
    wecho The mild mannered merchant says, 'I feel a lot of power in this staff.  I hope it can serve you well.'
    wait 2s
    wforce mild say And have these as a show of my gratitude.
    set gem 0
    while %gem% < 3
      eval drop %random.11% + 55736
      wforce mild mload obj %drop%
      eval gem %gem% + 1
    done
    wforce mild give all.gem %quester%
    wforce mild tip %quester%
    wforce mild emote leaves north toward Blue Fog Trail.
    wpurge merchant
  elseif %quester.quest_variable[ursa_quest:choice] == 2
    wait 1s
    wforce mild say Good enough.
    wforce mild wear %object%
    wforce mild drink firebreather
    wait 2s
    wforce mild emote suddenly falls face-down on the ground, holding the dagger in place.
    wecho The dagger drives home!
    wait 1s
    wecho The mild mannered merchant contorts wildly, flashing, his form unclear.
    wait 1s
    wecho More beast than man, &3Ursa roars in pain,&0 the dagger gleaming in his heart.
    wait 2s
    wforce mild emote slumps silently to the ground, and the dagger clatters to the ground beside him.
    wait 5s
    wforce mild groan
    wait 3s
    wforce mild emote looks himself up and down.
    wait 1s
    wforce mild say It worked!  I did it!
    wait 1s
    wforce mild emote removes each of the king's items, many of them having undergone radical changes in the ordeal.
    wforce mild emote tosses them one by one into the pool, letting them sink to the bottom.
    wait 1s
    wforce mild say Thanks, pal!
    wforce mild emote disappears up the trail toward Blue-Fog Trail.
    wpurge mild
    wait 2s
    wecho on the banks of the pool, something catches your eye.
    wait 1s
    wecho The diadem, having formed into hollow but hard glass, washes to the shore.
    wecho Upon second inspection you can see the form of the enraged Ursa, as if it were trying to escape its new glass home.
    wecho Several glittering gems wash up along with it.
    set gem 0
    while %gem% < 3
      eval drop %random.11% + 55736
      wload obj %drop%
      eval gem %gem% + 1
    done
    wload obj 62508
  elseif %quester.quest_variable[ursa_quest:choice] == 3
    wait 1s
    wforce mild emote tosses the anvil to the ground with a grunt.
    wait 3s
    wforce mild emote places the ring of stolen life on the anvil.
    wait 1s
    wforce mild emote wields the Golden Druidstaff.
    wait 3s
    wecho &2&bThe mild mannered merchant &0&1&bCRUSHES&0&2&b the ring of stolen life with a swift blow.&0
    wait 2s
    wforce mild emote gathers the powdered ring into the milk, gives it a swirl, and drinks the milk to the last drop.
    wait 2s
    wecho The mild mannered merchant says, 'That should do the job.  I'm feeling better already!'
    wait 4s
    wecho You hear a deep grumbling, from the direction of the merchant.
    wait 4s
    wforce merchant emote frantically pulls a misty blue sword from his things.
    wecho The mild mannered merchant says, 'It's coming back!  I don't think I'll be able to regain control!'
    wecho A mild mannered merchant falls on his own sword! (&1&b564&0)
    wait 2s
    wecho &3A mild mannered merchant lets out a disturbing &bROAR&0&3 as the last of his humanity slips away.&0
    wpurge mild
    wload mob 62550
    wload obj 62507
    wforce ursa get sword
    wforce ursa wi sword
    set gem 0
    while %gem% < 3
      eval drop %random.11% + 55736
      wforce ursa mload obj %drop%
      eval gem %gem% + 1
    done
  endif
  *
  * Set X to the level of the award - code does not run without it
  * 
  if %quester.level% < 50
    set expcap %quester.level%
  else
    set expcap 50
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
  * Adjust exp award by class so all classes receive the same proportionate amount
  *
  switch %quester.class%
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
      eval expmod (%expmod% + ((%expmod% * 2) / 15)
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
  wsend %quester% &3&bYou gain experience!&0
  eval setexp (%expmod% * 10)
  set loop 0
  while %loop% < 10
  *
  * Xexp must be replaced by mexp, oexp, or wexp for this code to work
  * Pick depending on what is running the trigger
  *
    wexp %quester% %setexp%
    eval loop %loop% + 1
  done
  quest complete ursa_quest %quester%
endif
~
#62551
letters for the merchant~
0 g 100
~
if %actor.quest_stage[ursa_quest] == 1
   wait 1s
   msend %actor% %self.name% notices the concerned look on your face.
   wait 1s
   msend %actor% %self.name% tells you, 'So, the merchant has gotten himself in quite a bit
   msend %actor% &0of trouble.'
   if %self.vnum% == 58008
      wait 1s
      msend %actor% %self.name% writes a beautifully crafted white epistle.
      wait 2s
      msend %actor% %self.name% tells you, 'Please, bring him this note from me.  It describes what
      msend %actor% &0he must do for the gods to heal him.'
      mload obj 62511
      give letter %actor%
   elseif %self.vnum% == 6007
      wait 1s
      msend %actor% %self.name% tells you, 'I know what he must do, but he won't like it.'
      chuckle
      wait 1s
      msend %actor% %self.name% dips his quill in a well of blood and scratches out a sinister letter.
      wait 2s
      msend %actor% %self.name% tells you, 'Quickly, take this to him!  Perhaps the Darkness
      msend %actor% &0still finds him...  amusing.'
      mload obj 62510
      give letter %actor%
   elseif %self.vnum% == 7310
      wait 1s
      msend %actor% %self.name% tells you, 'Fortunately, there isn't anything I haven't found
      msend %actor% &0the cure for.  He sent you to exactly the right person!'
      wait 2s
      msend %actor% %self.name% takes a crumpled piece of parchment from the floor of his hut and an exhausted coal from the fire.
      msend %actor% %self.name%, with coal in fist, scribbles out an ugly, barely legible note.
      wait 2s
      mload obj 62512
      give note %actor%
      wait 1s
      msend %actor% %self.name% tells you, 'Now get out of my swamp!'
   endif
endif
~
#62570
tight fitting armor~
1 j 100
~
if %actor.size% == tiny || %actor.size% == small || %actor.size% == medium
   return 1
else
   return 0
   osend %actor% It's too small for you!
   oechoaround %actor% %actor.name% struggles and fails to squeeze into %self.shortdesc%.
endif
~
#62571
the BIG hurt~
1 b 100
~
set wielder %self.worn_by%
set rm %wielder.room%
set target %wielder.fighting%
if %target%
   switch %wielder.size%
      case medium
         halt
         break
      case large
         switch %target.size%
            case large
               halt
            case huge
               halt
            case giant
               halt
            case gargantuan
               halt
            case colossal
               halt
            case titanic
               halt
            case mountainous
               halt
         done
         break
      case huge
         switch %target.size%
            case huge
               halt
            case giant
               halt
            case gargantuan
               halt
            case colossal
               halt
            case titanic
               halt
            case mountainous
               halt
         done
         break
      case giant
         switch %target.size%
            case giant
               halt
            case gargantuan
               halt
            case colossal
               halt
            case titanic
               halt
            case mountainous
               halt
         done
         break
      case gargantuan
         switch %target.size%
            case gargantuan
               halt
            case colossal
               halt
            case titanic
               halt
            case mountainous
               halt
         done
         break
   done
   switch %random.16%
      case 1
         if %rm.down[room]% == -1 || %rm.down[room]% == 0 || %target.flags% /= !bash
            eval dam 150 +%random.100%
            odamage %target% %dam% crush
            osend %wielder% Your smash sends %target.name% sprawling! (&3%damdone%&0)
            oechoaround %wielder% %wielder%'s smash sends %target.name% spawling! (&4%damdone%&0)
            oforce %target% abort
         else
            eval dam 250 +%random.100%
            odamage %target% %dam% crush
            oteleport %target% %rm.down%
            osend %wielder% &2&bYour smash sends %target.name% sailing down!&0 (&3%damdone%&0)
            oechoaround %wielder% &2&b%wielder.name%'s blow sends %target.name% sailing down!&0 (&4%damdone%&0)
            oforce %target% abort
         endif
         break
      case 2
         if %rm.north[room]% ==  -1 || %rm.north[room]% == 0 || %target.flags% /= !bash
            eval dam 150 +%random.100%
            odamage %target% %dam% crush
            osend %wielder% Your smash sends %target.name% into a rock! (&3%damdone%&0)
            oechoaround %wielder% %wielder%'s smash sends %target.name% flying head first into a rock! (&4%damdone%&0)
            oforce %target% abort
         else
            eval dam 250 +%random.100%
            odamage %target% %dam% crush
            oteleport %target% %rm.north%
            osend %wielder% &2&bYour smash sends %target.name% sailing north!&0 (&3%damdone%&0)
            oechoaround %wielder% &2&b%wielder.name%'s blow sends %target.name% sailing north!&0 (&4%damdone%&0)
            oforce %target% abort
         endif
         break
      case 3
         if %rm.south[room]% == -1 || %rm.south[room]% == 0 || %target.flags% /= !bash
            eval dam 150 +%random.100%
            odamage %target% %dam% crush
            osend %wielder% Your smash sends %target.name% into a rock! (&3%damdone%&0)
            oechoaround %wielder% %wielder%'s smash sends %target.name% flying head first into a rock! (&4%damdone%&0)
            oforce %target% abort
         else
            eval dam 250 +%random.100%
            odamage %target% %dam% crush
            oteleport %target% %rm.south%
            osend %wielder% &2&bYour smash sends %target.name% sailing south!&0 (&3%damdone%&0)
            oechoaround %wielder% &2&b%wielder.name%'s blow sends %target.name% sailing south!&0 (&4%damdone%&0)
            oforce %target% abort
         endif
         break
      case 4
         if %rm.east[room]% == -1 || %rm.east[room]% == 0 || %target.flags% /= !bash
            eval dam 150 +%random.100%
            odamage %target% %dam% crush
            osend %wielder% Your smash sends %target.name% into a rock! (&3%damdone%&0)
            oechoaround %wielder% %wielder%'s smash sends %target.name% flying head first into a rock! (&4%damdone%&0)
            oforce %target% abort
         else
            eval dam 250 +%random.100%
            odamage %target% %dam% crush
            oteleport %target% %rm.east%
            osend %wielder% &2&bYour smash sends %target.name% sailing east!&0 (&3%damdone%&0)
            oechoaround %wielder% &2&b%wielder.name%'s blow sends %target.name% sailing east!&0 (&4%damdone%&0)
            oforce %target% abort
         endif
         break
      case 5
         if %rm.west[room]% == -1 || %rm.west[room]% == 0 || %target.flags% /= !bash
            eval dam 150 +%random.100%
            odamage %target% %dam% crush
            osend %wielder% Your smash sends %target.name% into a rock! (&3%damdone%&0)
            oechoaround %wielder% %wielder%'s smash sends %target.name% flying head first into a rock! (&4%damdone%&0)
            oforce %target% abort
         else
            eval dam 250 +%random.100%
            odamage %target% %dam% crush
            oteleport %target% %rm.west%
            osend %wielder% &2&bYour smash sends %target.name% sailing west!&0 (&3%damdone%&0)
            oechoaround %wielder% &2&b%wielder.name%'s blow sends %target.name% sailing west!&0 (&4%damdone%&0)
            oforce %target% abort
         endif
         break
      case 6
         eval dam 150 +%random.100%
         odamage %target% %dam% crush
         osend %wielder% You send %target.name% skidding on %target.p% back, with your power-swing from %self.shortdesc%! (&3%damdone%&0)
         oechoaround %wielder% %target.name% goes skidding on %target.p% back after a powerful blow from %wielder.name%!&0 (&4%damdone%&0)
         oforce %target% abort
         break
      case 7
         eval dam 150 +%random.100%
         odamage %target% %dam% pierce
         osend %wielder% A branch of your oak tree catches %target.name%, goring him! (&3%damdone%&0)
         oechoaround %wielder% A branch of %wielder.name%'s oak tree catches %target.name%, goring him! (&4%damdone%&0)
         oforce %target% abort
         break
      case 8
         eval dam 150 +%random.100%
         odamage %target% %dam% crush
         osend %wielder% Your blow catches %target.name% in the back of the head! (&3%damdone%&0)
         oechoaround %wielder% %wielder.name%'s blow catches %target.name% in the back of the head! (&4%damdone%&0)
         oforce %target% abort
         break
      case 9
         eval dam 150 +%random.100%
         odamage %target% %dam% crush
         osend %wielder% Your blow catches %target.name% below the knees, sending him heels-up. (&3%damdone%&0)
         oechoaround %wielder% %wielder.name%'s blow catches %target.name% below the knees, sending him heels-up. (&4%damdone%&0)
         oforce %target% abort
         break
      case 10
         eval dam 150 +%random.100%
         odamage %target% %dam% crush
         osend %wielder% You stand %self.shortdesc% on end directly on top of %target.name%'s little body. (&3%damdone%&0)
         oechoaround %wielder% %wielder.name% brings %self.shortdesc% down end-first on top of %target.name%. (&4%damdone%&0)
         oforce %target% abort
         break
      case 11
         eval dam 150 +%random.100%
         odamage %target% %dam% pierce
         osend %wielder% You jab %target.name% with the end of %self.shortdesc%, puncturing %target.p% side. (&3%damdone%&0)
         oechoaround %wielder% %wielder.name% jabs %target.name% with the end of %self.shortdesc%. (&4%damdone%&0)
         oforce %target% abort
         break
      case 12
      case 13
      case 14
      case 15
      case 16
   done
endif
~
#62572
countdown 62569~
1 j 1
~
set maxcharge 1
set delay 200
global maxcharge
global delay
if %charges% > 0
   eval charges %charges% - 1
   global charges
   oecho %self.shortdesc% flashes brightly.
   oload mob 62569
   oforce purity mload obj 48928
   oforce purity recite scroll %actor%
   opurge purity
   set ready 0
   global ready
else
   osend %actor% %self.shortdesc%'s power is momentarily exhausted.
endif
~
#62573
count up~
1 b 100
~
if %delay% > 0 && %obj.worn_by% != 0
   wait %delay% s
   if %charges% >= 0 && %charges% <= %maxcharge%
      if %charges% < %maxcharge%
         eval charges %charges% + 1
         global charges
         oecho &3&b%self.shortdesc% hums lightly.&0
      elseif %charges% == %maxcharges%
      endif
   else
      set charges 0
      global charges
   endif
endif
~
#62574
seagulls, stop it now~
1 e 25
~
if %count% > 0
   oecho Count: %count%
   if %worn_on% != 0
      oforce %victim% say Stop it, %actor.name%!
      ocast 'minor paralysis' %actor%
      eval add %damage% / 100
      oecho First Add: %add%
      eval add %add% + 1
      oecho Second Add: %add%
      eval count %count% + %add%
      oecho count: %count%
      wait %count% s
   endif
   oecho count is over
   halt
else
   set count 1
   global count
endif
~
#62575
curse item~
1 p 100
7~
oecho &1%actor.name%'s spell attempts to disrupt %self.shortdesc%'s cursed nature.&0
wait 1 s
if %object.flagged[!DROP]%
   oecho &1%self.shortdesc% is undiminished by %actor.name%'s spell.&0
else
   oecho &1%self.shortdesc% turns to ashes, no longer held together by the Pit Fiends curse.&0
   oecho %self.shortdesc% crumbles to dust and blows away.&0
   opurge %self%
endif
~
#62580
demon wears or removes mask~
0 b 30
~
if %self.hit% > 450
   growl %next_in_room%
else
   if %wearing[62580]%
      remove mask
   else
      roar
   endif
endif
~
#62581
facade wear trigger~
1 j 100
~
wait 1
osend %actor% &9&bYour skin hardens and turns to stone!&0
oechoaround %actor% &9&b%actor.name%'s skin hardens and turns to stone!
~
#62582
facade remove trigger~
1 l 100
~
if %actor.vnum% == 62570
   osend %actor% As your patience dwindles you rip the rocky layer from your face, exposing the molten inner layers!
   oechoaround %actor% &9&bAs %actor.name%'s patience dwindles &1%actor.n% &br&3&bip&0&3s &1&bthe &9&brocky layer from &1&b%actor.p% face, &0&3expos&bing the 
   m&0&7ol&bten&0&7 in&3&bner &0&3la&1&byers!&0
   osend %actor% &3Your body melts, becoming more fluid.&0
   oechoaround %actor% &3&b%actor.name%'s body melts, becoming more fluid.&0
elseif %actor.vnum% < 1
   osend %actor% &3&bYour skin softens and returns to normal.&0
   oechoaround %actor% &3&b%actor.name%'s skin softens and returns to normal.&0
endif
~
#62590
harness obj~
1 b 100
0~
if %actor.eff_flagged[harness]%
   oecho %actor.name% has harness.
else
   oflagset %actor% harness
   oecho &1setting harness!&0
endif
~
#62599
Progress trigger~
0 d 100
progress progress?~
set stage %actor.quest_stage[ursa_quest]%
set path %actor.quest_variable[ursa_quest:choice]%
wait 2
* for debug, say stage %stage% path %path%
if %actor.has_completed[ursa_quest]%
    mecho %self.name% says, 'You have brought me the remedy, and I thank you
    mecho &0for that.'
    wait 1s
    if %path%==1 
        say I hope the Redeeming Staff was a good reward.
    elseif %path%==2 
        say I hope the glass bear was a good reward.
    elseif %path%==3 
        say I hope the misty blue sword was a good reward.
    endif
    halt
endif
if %stage% == 0
    say Will you help me?!  Please!
elseif %stage% == 1
     if %path% == 0
         mecho %self.name% says, 'Please visit one of these powerful people, and
         mecho &0ask for a cure:'
         mecho The Emperor, on a nearby island of very refined people 
         mecho Ruin Wormheart in the Red City
         mecho The crazy hermit of the swamps
     endif
* Stage 2 checks
elseif %stage% == 2
     if %path% == 1
         say Please bring me some pepper.
     elseif %path% == 2
         mecho %self.name% says, 'You need to return to me a particular sceptre
         mecho &0of gold that symbolized a king's undying leadership.'
     elseif %path% == 3
         mecho %self.name% says, 'A devourer has a ring with power to heal.
         mecho &0Please bring me this ring.'
     endif
* stage 3 checks
elseif %stage% == 3
     if %path% == 1
         mecho %self.name% says, 'Please bring me a plant, found as "a bit of
         mecho &0bones and plants" from Blue-Fog Trail.'
     elseif %path% == 2
         mecho %self.name% says, 'Please bring me an emblem of a king's power.
         mecho &0An emblem of the sun.  The one written of in legends of the warring gods in the
         mecho &0far north.'
     elseif %path% == 3
         mecho %self.name% says, 'Find the Golhen DrubStatt the hermit wrote
         mecho &0about from the Highlands, and bring it back to me quickly.'
     endif
* stage 4 checks
elseif %stage% == 4
     if %path% == 1
         mecho %self.name% says, 'I need a particular thorny wood.  Because of
         mecho &0it's unpleasant nature there are some unpleasant people that make staffs and
         mecho &0walking sticks out of it.  Please find one, and bring it back here.'
     elseif %path% == 2
         mecho %self.name% says, 'Please bring be the dagger that radiates
         mecho &0glorious light.  The priests of South Caelia know its fierce beauty.'
     elseif %path% == 3
         say Bring me milk, in any container.
     endif
* stage 5 checks
elseif %stage% == 5
     if %path% == 1
         mecho %self.name% says, 'Bring me a pitcher from the hot springs or the
         mecho &0Dancing Dolphin Inn.  Either will do.'
     elseif %path% == 2
         mecho %self.name% says, 'I can't do this sober!  Fetch me something to
         mecho &0drink, and make it strong!'
     elseif %path% == 3
         mecho %self.name% says, 'Off of the great road is a lumber mill.  Their
         mecho &0smith has an anvil that will do our work perfectly.  Please fetch it for me.'
     endif
* stage 6 checks
elseif %stage% == 6
     if %path% == 2
         mecho %self.name% says, 'I need a large container for a sarcophagus,
         mecho &0like a body-bag or a large chest.'
     endif
endif
~
$~
