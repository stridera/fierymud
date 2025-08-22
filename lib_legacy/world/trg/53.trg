#5303
Head Monk chant~
0 b 100
~
set chant %random.4%
switch %chant%
   case 1
      chant 'war cry'
      break
   case 2
      chant 'battle hymn'
      break
   case 3
      chant 'peace'
      break
   case 4
   default
      chant 'regeneration'
done
~
#5310
Beast Master Pumahl greet~
0 h 100
~
wait 2
if !%actor.has_completed[beast_master]%
  if !%actor.quest_stage[beast_master]%
    msend %actor% %self.name% says, 'Welcome to the Hall of the Hunt, home of the Beast Masters.  We give out assignments to &6&b[hunt]&0 and defeat legendary creatures.  Great rewards wait those who prove their mettle.'
  elseif %actor.quest_variable[beast_master:hunt]% == running
    msend %actor% %self.name% says, 'You're still on the hunt.  What are you doing here?  If you lost your assignment say, &6&b"I need a new assignment"&0.'
  elseif %actor.quest_variable[beast_master:hunt]% == dead
    msend %actor% %self.name% says, 'Welcome back!  If your hunt was successful give me your assignment.  If you lost your assignment say, &6&b"I need a new assignment"&0.'
  elseif %actor.quest_stage[beast_master]% >= 1 && !%actor.has_completed[beast_master]%
    msend %actor% %self.name% says, 'Ah, back for another creature to &6&b[hunt]&0 I see.'
  endif
  if !%actor.has_completed[ranger_trophy]% && %actor.level% > 9 && (%actor.class% /= Warrior || %actor.class% /= Berserker || %actor.class% /= Ranger || %actor.class% /= Mercenary)
    wait 1s
    msend %actor% %self.name% says, 'Or maybe you're here to prove your &6&b[skill]&0 as a hunter.'
  endif
else
  if (%actor.class% /= Ranger || %actor.class% /= Warrior || %actor.class% /= Berserker || %actor.class% /= Mercenary) && !%actor.quest_stage[ranger_trophy]% && %actor.level% > 9
    msend %actor% %self.name% says, 'A new hunter looking to prove your &6&b[skill]&0 I see.'
  elseif %actor.quest_stage[ranger_trophy]% && !%actor.has_completed[ranger_trophy]%
    msend %actor% %self.name% says, 'Ah, you must be looking to prove your &6&b[skill]&0 again.'
  endif
endif
~
#5311
Beast Master Pumahl speech1~
0 d 100
hunt beasts beast master legendary creatures creature~
wait 2
if %actor.has_completed[beast_master]%
  msend %actor% %self.name% says, 'You've already proven your dominion over the beasts of Ethilien!'
elseif (!%actor.quest_stage[beast_master]% || (%actor.quest_stage[beast_master]% == 1 && !%actor.quest_variable[beast_master:hunt]%))
  msend %actor% %self.name% says, 'I have an excellent beast for you to start with.'
  wait 4
  msend %actor% %self.name% says, 'Good hunters know terrible monsters lurk everywhere, even beneath our feet in this very city.'
  wait 4
  msend %actor% %self.name% says, 'The sewers beneath Mielikki are teeming with life - some of it much more dangerous than the rest.  The slime monsters are particularly pointy and bitey.  Find and kill the biggest of these abominations and we'll welcome you to the ranks of the Beast Masters!'
  wait 4
  msend %actor% %self.name% says, 'Ready to go spelunking through the sludge?'
elseif %actor.quest_variable[beast_master:hunt]% == dead
  msend %actor% %self.name% says, 'Give me your current assignment first.'
  halt
else
  if %actor.level% >= (%actor.quest_stage[beast_master]% - 1) * 10
    switch %actor.quest_stage[beast_master]%
      case 1
        if %actor.quest_variable[beast_master:hunt]% == running
          msend %actor% %self.name% says, 'You're still on the hunt.  Slay an abominable slime creature in the sewers beneath Mielikki.'
        endif
        break
      case 2
        if %actor.quest_variable[beast_master:hunt]% == running
          msend %actor% %self.name% says, 'You're still on the hunt.  Hunt down a large buck in the forests just outside of Mielikki.'
        else
          msend %actor% %self.name% says, 'Knowing what lurks below Mielikki now, it shouldn't be surprising that danger is just outside our doors.'
          wait 4
          msend %actor% %self.name% says, 'The forests out east seem safe at first glance, but something evil has taken hold in the deepest heart of the woods.  It has twisted and corrupted many of the animals that made that place their homes and now they present a threat as dangerous as any sewer-dwelling monster.'
          wait 4
          msend %actor% %self.name% says, 'Take down one of the bucks ripping up the forest.  Maybe it will help the forest recover a little.'
          wait 4
          msend %actor% %self.name% says, 'Do you feel strong enough to handle that?'
        endif
        break
      case 3
        if %actor.quest_variable[beast_master:hunt]% == running
          msend %actor% %self.name% says, 'You're still on the hunt.  Track down the giant scorpion of Gothra.'
        else
          msend %actor% %self.name% says, 'There are some monsters we think are only stories.  Like out in the Gothra Desert, there's the story of a giant scorpion trapped in a cave.  Other monster hunters have found the cave, but no one has had any luck getting inside.'
          wait 4        
          msend %actor% %self.name% says, 'Maybe you can find someone who knows how to get in and you can kill whatever's on the other side of the door.  Just think what a name you could make for yourself if you pull it off!'
          wait 4
          msend %actor% %self.name% says, 'Are you up for some grand exploration?'
        endif
        break
      case 4
        if %actor.quest_variable[beast_master:hunt]% == running
          msend %actor% %self.name% says, 'You're still on the hunt.  Head far to the south and kill a monstrous canopy spider.'
        else
          msend %actor% %self.name% says, 'Since you had some luck with that giant scorpion, let's see you take down another kind of giant arachnid!'
          wait 4
          msend %actor% %self.name% says, 'There's nothing quite so famous as giant spiders.  Fangorn Forest in South Caelia is full of 'em.  Find the most monstrous canopy spider you can and show it who's the real master of beasts!'
          wait 4
          msend %actor% %self.name% says, 'Ready to walk the spiderwebs?'
        endif
        break
      case 5
        if %actor.quest_variable[beast_master:hunt]% == running
          msend %actor% %self.name% says, 'You're still on the hunt.  Behead the famed chimera of Fiery Island.'
        else
          msend %actor% %self.name% says, 'Between North and South Caelia are a number of small islands.  All of them are home to some awe-inspiring creatures, each more deadly than the last.'
          wait 4
          msend %actor% %self.name% says, 'One unique creature in particular stands out: a three-headed chimera!  Nothing like killing three beasts in one!'
          wait 4
          msend %actor% %self.name% says, 'Oh, and I understand there's a local shaman out there who knows more about the creature and the island itself.  Might want to stop and talk to him while you're there.  Who knows what you could learn.'
          wati 4
          msend %actor% %self.name% says, 'You want to go knock a few heads together?'
        endif
        break
      case 6
        if %actor.quest_variable[beast_master:hunt]% == running
          msend %actor% %self.name% says, 'You're still on the hunt.  Slay the "king" of the abominations known as driders.'
        else
          msend %actor% %self.name% says, 'You've done great against the surface world, but you've only just begun to scratch... well... the surface.  Below this upper world lies a vast universe of caves and tunnels known as the Underdark.  Whole bunch of the worst kinds of monsters live down there.'
          wait 4
          msend %actor% %self.name% says, 'The drow, the dark elves, are one of the more infamous.  One of their coming of age rituals involves testing their young in some kind of brutal and savage contest.  Those who fail are turned into grotesque horrors called "driders".  They get thrown out of society and form a colony of sorts under the most evil and murderous of them all, who they call a "king".'
          wait 4
          msend %actor% %self.name% says, 'There's an entrance to a drow city in the caves just east of Mielikki.  There's sure to be a drider king down there!'
          wait 4
          msend %actor% %self.name% says, 'Can you prove you're better than some half-elf, half-spider abomination?'
        endif
        break
      case 7
        if %actor.quest_variable[beast_master:hunt]% == running
          msend %actor% %self.name% says, 'You're still on the hunt.  Close the eyes of a beholder from under Mt. Frostbite.'
        else
          msend %actor% %self.name% says, 'Ever heard of a beholder?  It's a great big floating eye with teeth, surrounded by a bunch of little eyes on stalks.  They're big, mean, and exceptionally deadly!'  
          wait 4
          msend %actor% %self.name% says, 'For whatever reason, they show up in uncommonly large numbers in the ice caverns beneath Mt. Frostbite.  And somehow, they've learned to live with the cult up there too.  Just think of it as an added bonus hunt!'
          wait 4
          msend %actor% %self.name% says, 'You ready to brave the cold?'
        endif
        break
      case 8
        if %actor.quest_variable[beast_master:hunt]% == running
          msend %actor% %self.name% says, 'You're still on the hunt.  Lay the Banshee to eternal rest.'
        else
          msend %actor% %self.name% says, 'Some things are so dangerous, you have to kill them twice.'
          wait 4
          msend %actor% %self.name% says, 'There used to be a castle up north.  Something terrible happened to it and it sunk into the Northern Swamps, killing everyone inside.  But, anyone who manages to cross through comes back with terrible stories of ghosts and lizardfolk crawling out of the ruins.'
          wait 4
          msend %actor% %self.name% says, 'Some even tell of ghostly piercing screams echoing through the swamp.  Screaming ghosts can only mean one thing - a banshee.  Destroying a banshee would make for an epic tale!'
          wait 4
          msend %actor% %self.name% says, 'You in?'
        endif
        break
      case 9
        if %actor.quest_variable[beast_master:hunt]% == running
          msend %actor% %self.name% says, 'You're still on the hunt.  Put an end to Baba Yaga's dreamy witchcraft.'
        else
          msend %actor% %self.name% says, 'I got a great one for you, a literal walking legend.'
          wait 4
          msend %actor% %self.name% says, 'Some of the wildest monsters ever known have been popping up in the Syric Mountains.  Among them is a troll witch, goes by the name Baba Yaga.  Lives in a house with legs.  Literal chicken legs.'
          wait 4
          msend %actor% %self.name% says, 'Infiltrate the Realm of the King of Dreams, find Baba Yaga, and kill her.'
          wait 4
          msend %actor% %self.name% says, 'It's going to be an incredibly dangerous hunt, one worth of your skills.  Wanna take it on?'
        endif
        break
      case 10
        if %actor.quest_variable[beast_master:hunt]% == running
          msend %actor% %self.name% says, 'You're still on the hunt.  Defeat the medusa below the city of Templace.'
        else
          msend %actor% %self.name% says, 'The last truly monstrous creature I have for you to hunt down is a horrifying sight to behold.'
          wait 4
          msend %actor% %self.name% says, 'Somewhere below the ruins of Templace is the lair of a hideous creature with two tusks protruding from her mouth and snakes growing from her head.  She's almost certainly hidden so keep your eyes peeled.'
          wait 4
          msend %actor% %self.name% says, 'Prepared to stalk the ruined streets of Templace?'
        endif
    done
  else
    msend %actor% %self.name% says, 'Unfortunately I don't have any beasts for you to pursue at the moment.  Check back later!'
  endif
endif
~
#5312
Beast Master Pumahl speech yes~
0 d 100
yes~
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
    if !%person.quest_stage[beast_master]%
      quest start beast_master %person%
    endif
    if %person.has_completed[beast_master]%
      msend %person% %self.name% says, 'You've already proven your dominion over the beasts of Ethilien!'
    elseif %person.quest_variable[beast_master:hunt]% == dead
      msend %person% %self.name% says, 'Give me your current assignment first.'
    else
      if %person.level% >= (%person.quest_stage[beast_master]% - 1) * 10
        if %person.quest_variable[beast_master:hunt]% != running
          msend %person% %self.name% says, 'Excellent!'
          switch %person.quest_stage[beast_master]%
            case 1
              set notice 5300
              break
            case 2
              set notice 5301
              break
            case 3
              set notice 5302
              break
            case 4
              set notice 5303
              break
            case 5
              set notice 5304
              break
            case 6
              set notice 5305
              break
            case 7
              set notice 5306
              break
            case 8
              set notice 5307
              break
            case 9
              set notice 5308
              break
            case 10
              set notice 5309
              break
          done
          mload obj %notice%
          give assignment %person%
          msend %person% &0  
          msend %person% %self.name% says, 'When you've slayed the creature, bring that assignment back to me.  I'll reward you then.'
          msend %person% &0   
          msend %person% %self.name% says, 'Be brave, be strong, and good luck!'
          quest variable beast_master %person% hunt running
        else
          switch %person.quest_stage[beast_master]% 
            case 1
              msend %person% %self.name% says, 'You're still on the hunt.  Slay an abominable slime creature in the sewers beneath Mielikki.'
              break
            case 2
              msend %person% %self.name% says, 'You're still on the hunt.  Hunt down a large buck in the forests just outside of Mielikki.'
              break
            case 3
              msend %person% %self.name% says, 'You're still on the hunt.  Track down the giant scorpion of Gothra.'
              break
            case 4
              msend %person% %self.name% says, 'You're still on the hunt.  Head far to the south and kill a monstrous canopy spider.'
              break
            case 5
              msend %person% %self.name% says, 'You're still on the hunt.  Behead the famed chimera of Fiery Island.'
              break
            case 6
              msend %person% %self.name% says, 'You're still on the hunt.  Slay the "king" of the abominations known as driders.'
              break
            case 7
              msend %person% %self.name% says, 'You're still on the hunt.  Close the eyes of a beholder from under Mt. Frostbite.'
              break
            case 8
              msend %person% %self.name% says, 'You're still on the hunt.  Lay the Banshee to eternal rest.'
              break
            case 9
              msend %person% %self.name% says, 'You're still on the hunt.  Put an end to Baba Yaga's dreamy witchcraft.'
              break
            case 10
              msend %person% %self.name% says, 'You're still on the hunt.  Defeat the medusa below the city of Templace.'
          done
        endif
      else
        msend %person% %self.name% says, 'Unfortunately I don't have any beasts for you to pursue at the moment.  Check back later!'
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#5313
Beast Master Ranger Trophy receive~
0 j 100
~
switch %object.vnum%
  * hunt notices here
  case 5300
    set stage 1
    set victim1 an abominable slime creature
    set go hunt
    break
  case 5301
    set stage 2
    set victim1 a large buck
    set go hunt
    break
  case 5302
    set stage 3
    set victim1 the giant scorpion
    set go hunt
    break
  case 5303
    set stage 4
    set victim1 a monstrous canopy spider
    set go hunt
    break
  case 5304
    set stage 5
    set victim1 the chimera
    set go hunt
    break
  case 5305
    set stage 6
    set victim1 the drider king
    set go hunt
    break
  case 5306
    set stage 7
    set victim1 a beholder
    set go hunt
    break
  case 5307
    set stage 8
    set victim1 the Banshee
    set go hunt
    break
  case 5308
    set stage 9
    set victim1 Baba Yaga
    set go hunt
    break
  case 5309
    set stage 10
    set victim1 the medusa
    set go hunt
    break
  * ranger trophy items start here
  case 370
    set trophystage 1
    set item quest
    set go trophy
    break
  case 1607
    set trophystage 1
    set item trophy
    set go trophy
    break
  case 55579
    set trophystage 1
    set item gem
    set go trophy
    break
  case 371
    set trophystage 2
    set item quest
    set go trophy
    break
  case 17806
    set trophystage 2
    set item trophy
    set go trophy
    break
  case 55591
    set trophystage 2
    set item gem
    set go trophy
    break
  case 372
    set trophystage 3
    set item quest
    set go trophy
    break
  case 1805
    set trophystage 3
    set item trophy
    set go trophy
    break
  case 55628
    set trophystage 3
    set item gem
    set go trophy
    break
  case 373
    set trophystage 4
    set item quest
    set go trophy
    break
  case 62513
    set trophystage 4
    set item trophy
    set go trophy
    break
  case 55652
    set trophystage 4
    set item gem
    set go trophy
    break
  case 374
    set trophystage 5
    set item quest
    set go trophy
    break
  case 23803
    set trophystage 5
    set item trophy
    set go trophy
    break
  case 55664
    set trophystage 5
    set item gem
    set go trophy
    break
  case 375
    set trophystage 6
    set item quest
    set go trophy
    break
  case 43009
    set trophystage 6
    set item trophy
    set go trophy
    break  
  case 55685
    set trophystage 6
    set item gem
    set go trophy
    break
  case 376
    set trophystage 7
    set item quest
    set go trophy
    break
  case 47008
    set trophystage 7
    set item trophy
    set go trophy
    break
  case 55705
    set trophystage 7
    set item gem
    set go trophy
    break
  case 377
    set trophystage 8
    set item quest
    set go trophy
    break
  case 53323 
  case 53311
    set trophystage 8
    set item trophy
    set go trophy
    break
  case 55729
    set trophystage 8
    set item gem
    set go trophy
    break
  case 378
    set trophystage 9
    set item quest
    set go trophy
    break
  case 52014
    set trophystage 9
    set item trophy
    set go trophy
    break
  case 55741
    set trophystage 9
    set item gem
    set go trophy
    break
  default
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    say What is this for?
    halt
done
if %go% == hunt
  if %actor.quest_stage[beast_master]% == %stage% && %actor.quest_variable[beast_master:hunt]% == dead
    wait 2
    mjunk %object%
    cheer
    msend %actor% %self.name% says, 'Congratulations!  Here's your reward.'
    eval money %stage% * 10
    give %money% platinum %actor%
    if %stage% == 1
      set expcap 5
    else
      eval bonus (%stage% - 1) * 10
      set expcap %bonus%
    endif
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
    set anti Anti-Paladin
    switch %person.class%
      case Warrior
      case Berserker 
          eval expmod (%expmod% + (%expmod% / 10))
          break
      case Paladin
      case %anti%
      case Ranger
          eval expmod (%expmod% + ((%expmod% * 2) / 15))
          break
      case Sorcerer
      case Pyromancer
      case Cryomancer
      case Illusionist
      case Bard
          eval expmod (%expmod% + (%expmod% / 5))
          break
      case Necromancer
      case Monk
          eval expmod (%expmod% + (%expmod% * 2) / 5)
          break
      default
          set expmod %expmod%
    done
    msend %actor% &3&bYou gain experience!&0
    eval setexp (%expmod% * 10)
    set loop 0
    while %loop% < 3
      mexp %actor% %setexp%
      eval loop %loop% + 1
    done
    quest variable beast_master %actor% target1 0
    quest variable beast_master %actor% hunt 0
    wait 2
    if %stage% < 10
      quest advance beast_master %actor%
      msend %actor% %self.name% says, 'Check in again if you have time for more hunts.'
    else
      quest complete beast_master %actor%
      msend %actor% %self.name% says, 'You have earned your place among the greatest Beast Masters in the realm!'
      wait 1s
      msend %actor% %self.name% says, 'Take these gloves.  Wear them to show everyone your unstoppable prowess.'
      mload obj 413
      give gloves %actor%
    endif
    if (%actor.class% /= ranger || %actor.class% /= warrior || %actor.class% /= berserker || %actor.class% /= Mercenary) && %actor.quest_stage[ranger_trophy]% == 0
      wait 2s
      msend %actor% %self.name% says, 'I think you've earned this too.'
      mload obj 370
      give trophy %actor%
      wait 1s
      msend %actor% %self.name% says, 'Trophies like these are proof of your skill as a hunter.'
      quest start ranger_trophy %actor%
      wait 2s
      if %actor.level% > 9
        msend %actor% %self.name% says, 'This is an opportune time to further demonstrate your &6&b[skill]&0.'
      else
        msend %actor% %self.name% says, 'Come back with that trophy after you reach level 10.  We can discuss your skills then.'
      endif
    endif
  elseif %actor.quest_stage[beast_master]% > %stage%
    return 0
    shake
    mecho %self.name% refuses the notice.
    wait 2
    msend %actor% %self.name% says, 'You already killed this creature!'
  elseif %actor.quest_stage[beast_master]% < %stage%
    wait 2
    eye %actor%
    msend %actor% %self.name% says, 'How'd you get this?!  You steal it off someone else??'
    mecho %self.name% rips up the notice!
    mjunk %object%
  elseif %actor.quest_variable[beast_master:hunt]% != dead
    return 0
    mecho %self.name% refuses the notice.
    wait 2
    msend %actor% %self.name% says, 'You have to slay kill the beast first!  %victim1% is still out there.
  endif
elseif %go% == trophy
  if %actor.class% != warrior && %actor.class% != ranger && %actor.class% != mercenary && %actor.class% != berserker
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, '%actor.class%s aren't fit for these kinds of tests.'
  elseif %actor.quest_stage[beast_master]% < %actor.quest_stage[ranger_trophy]%
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'Defeat some more monsters first.'
  elseif %actor.level% < (%actor.quest_stage[ranger_trophy]% * 10)
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'You need to gain some more experience first.'
  elseif %trophystage% > %actor.quest_stage[ranger_trophy]%
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'You don't need that to demonstrate your skills yet.  Be patient!'
  elseif %trophystage% < %actor.quest_stage[ranger_trophy]%
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'You've already used that as proof.'
  else
    if %item% == quest
      set job1 %actor.quest_variable[ranger_trophy:trophytask1]%
      set job2 %actor.quest_variable[ranger_trophy:trophytask2]%
      set job3 %actor.quest_variable[ranger_trophy:trophytask3]%
      set job4 %actor.quest_variable[ranger_trophy:trophytask4]%
      if %job1% && %job2% && %job3% && %job4%
        wait 2
        eval reward %object.vnum% + 1
        mjunk %object%
        nod
        msend %actor% %self.name% says, 'Well done!  You've demonstrated your skills well.'
        mload obj %reward%
        give trophy %actor%
        eval expcap %trophystage% * 10
        if %expcap% < 17
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
        switch %person.class%
          case Warrior
          case Berserker 
              eval expmod (%expmod% + (%expmod% / 10))
              break
          case Ranger
              eval expmod (%expmod% + ((%expmod% * 2) / 15))
              break
          default
              set expmod %expmod%
        done
        msend %actor% &3&bYou gain experience!&0
        eval setexp (%expmod% * 10)
        set loop 0
        while %loop% < 7
          mexp %actor% %setexp%
          eval loop %loop% + 1
        done
        set number 1
        while %number% < 5
          quest variable ranger_trophy %actor% trophytask%number% 0
          eval number %number% + 1
        done
        if %actor.quest_stage[ranger_trophy]% < 9
          quest advance ranger_trophy %actor%
        else
          quest complete ranger_trophy %actor%
        endif
      else
        return 0
        shake
        mecho %self.name% refuses %object.shortdesc%.
        wait 2
        msend %actor% %self.name% says, 'You need to do everything else before you exchange your trophy!'
      endif
    elseif %item% == trophy
      set task2 %actor.quest_variable[ranger_trophy:trophytask2]% 
      if %task2% == %object.vnum% || (%object.vnum% == 53311 && %task2% == 53323) || (%object.vnum% == 53323 && %task2% == 53311)
        set accept no
      else
        set accept yes
        quest variable ranger_trophy %actor% trophytask2 %object.vnum%
      endif
    elseif %item% == gem
      if %actor.quest_variable[ranger_trophy:trophytask3]% == %object.vnum%
        set accept no
      else
        set accept yes
        quest variable ranger_trophy %actor% trophytask3 %object.vnum%
      endif
    endif
    if %accept% == no
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      msend %actor% %self.name% says, 'You already gave me that.'
    elseif %accept% == yes
      wait 2
      mjunk %object%
      set job1 %actor.quest_variable[ranger_trophy:trophytask1]%
      set job2 %actor.quest_variable[ranger_trophy:trophytask2]%
      set job3 %actor.quest_variable[ranger_trophy:trophytask3]%
      set job4 %actor.quest_variable[ranger_trophy:trophytask4]%
      if %job1% && %job2% && %job3% && %job4%
        msend %actor% %self.name% says, 'Excellent.  Now give me your current trophy.'
      else
        msend %actor% %self.name% says, 'Good, now finish the rest.'
      endif
    endif
  endif
endif
~
#5314
Beast Master creature death~
0 f 100
~
switch %self.vnum%
  case 3133
  * abominable slime creature
    set stage 1
    set target1 abominable_slime_creature
    break
  case 12003
  * a large buck
    set target1 buck
    set stage 2
    break
  case 16105
  * giant scorpion
    set target1 giant_scorpion
    set stage 3
    break
  case 2308
  * monstrous canopy spider
    set target1 monstrous_canopy_spider
    set stage 4
    break
  case 48120
  * chimera
    set target1 chimera
    set stage 5
    break
  case 23730
  * drider king
    set target1 drider_king
    set stage 6
    break
  case 53305
  * beholder
    set target1 beholder
    set stage 7
    break
  case 53003
  * banshee
    set target1 banshee
    set stage 8
    break
  case 58401
  * Baba Yaga
    set target1 baba_yaga
    set stage 9
    break
  case 52006
  * medusa
    set target1 medusa
    set stage 10
    break
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
    if %person.quest_stage[beast_master]% == %stage% && %person.quest_variable[beast_master:hunt]% == running
      quest variable beast_master %person% target1 %target1%
      quest variable beast_master %person% hunt dead
      msend %person% &1&bYou cross %self.name% off your list.&0
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#5315
Ranger Trophy assignment look~
1 m 100
~
switch %self.vnum%
  case 5300
    set victim1 an abominable slime creature
    set stage 1
    break
  case 5301
    set victim1 a large buck
    set stage 2
    break
  case 5302
    set victim1 the giant scorpion
    set stage 3
    break
  case 5303
    set victim1 a monstrous canopy spider
    set stage 4
    break
  case 5304
    set victim1 the chimera
    set stage 5
    break
  case 5305
    set victim1 the drider king
    set stage 6
    break
  case 5306
    set victim1 a beholder
    set stage 7
    break
  case 5307
    set victim1 the Banshee
    set stage 8
    break
  case 5308
    set victim1 Baba Yaga
    set stage 9
    break
  case 5309
    set victim1 the medusa
    set stage 10
    break
done
return 0
osend %actor% This is a notice to slay %victim1%.
if %actor.quest_variable[beast_master:hunt]% == dead && %actor.quest_stage[beast_master]% == %stage%
  osend %actor% You have completed the hunt.  
  osend %actor% Return your assignment to Pumahl for your reward!
endif
~
#5316
Ranger Trophy assignment examine~
1 c 3
examine~
if %arg% == assignment
  switch %self.vnum%
    case 5300
      set victim1 an abominable slime creature
      set stage 1
      break
    case 5301
      set victim1 a large buck
      set stage 2
      break
    case 5302
      set victim1 the giant scorpion
      set stage 3
      break
    case 5303
      set victim1 a monstrous canopy spider
      set stage 4
      break
    case 5304
      set victim1 the chimera
      set stage 5
      break
    case 5305
      set victim1 the drider king
      set stage 6
      break
    case 5306
      set victim1 a beholder
      set stage 7
      break
    case 5307
      set victim1 the Banshee
      set stage 8
      break
    case 5308
      set victim1 Baba Yaga
      set stage 9
      break
    case 5309
      set victim1 the medusa
      set stage 10
      break
  done
else
  return 0
  halt
endif
osend %actor% This is a notice to slay %victim1%.
if %actor.quest_variable[beast_master:hunt]% == dead && %actor.quest_stage[beast_master]% == %stage%
  osend %actor% You have completed the hunt.  
  osend %actor% Return your assignment to Pumahl for your reward!
endif
~
#5317
Ranger Trophy Pumahl speech skill~
0 d 100
skill~
set trophystage %actor.quest_stage[ranger_trophy]%
set huntstage %actor.quest_stage[beast_master]%
set job1 %actor.quest_variable[ranger_trophy:trophytask1]%
set job2 %actor.quest_variable[ranger_trophy:trophytask2]%
set job3 %actor.quest_variable[ranger_trophy:trophytask3]%
set job4 %actor.quest_variable[ranger_trophy:trophytask4]%
wait 2
if %actor.class% != Warrior && %actor.class% != Ranger && %actor.class% != Berserker && %actor.class% != Mercenary
  shake
  msend %actor% %self.name% says, 'Only warriors, rangers, berserkers, and mercenaries possess the skills required for this.'
  halt
elseif %actor.level% < 10
  msend %actor% %self.name% says, 'You aren't ready for this test yet.  Come back when you've grown a bit.'
  halt
elseif %actor.level% < (%trophystage% * 10)
  msend %actor% %self.name% says, 'You aren't ready for another test yet.  Come back when you've gained some more experience.'
  halt
elseif %actor.has_completed[ranger_trophy]%
  msend %actor% %self.name% says, 'You've already proven your skills as much as possible!'
  halt
endif
if %trophystage% == 0
  mecho %self.name% says, 'Sure.  You must &6&b[hunt]&0 a great beast first though.'
  halt
elseif (%trophystage% >= %huntstage%) && !%actor.has_completed[beast_master]%
  msend %actor% %self.name% says, 'Prove your dominion over some more great beasts first and then we can talk.'
  halt
elseif %job1% && %job2% && %job3% && %job4%
  msend %actor% %self.name% says, 'You're all ready, just give me your old trophy.'
  halt
endif
switch %trophystage%
  case 1
    set trophy 1607
    set gem 55579
    set place A Coyote's Den
    set hint near the Kingdom of the Meer Cats.
    break
  case 2
    set trophy 17806
    set gem 55591
    set place In the Lions' Den
    set hint in the western reaches of Gothra.
    break
  case 3
    set trophy 1805
    set gem 55628
    set place either of the two Gigantic Roc Nests
    set hint in the Wailing Mountains.
    break
  case 4
    set trophy 62513
    set gem 55652
    set place Chieftain's Lair
    set hint in Nukreth Spire in South Caelia.
    break
  case 5
    set trophy 23803
    set gem 55664
    set place The Heart of the Den
    set hint where the oldest unicorn in South Caelia makes its home.
    break
  case 6
    set trophy 43009
    set gem 55685
    set place Giant Lynx's Lair
    set hint far to the north beyond Mt. Frostbite.
    break
  case 7
    set trophy 47008
    set gem 55705
    set place Giant Griffin's Nest
    set hint tucked away in a secluded and well guarded corner of Griffin Island.
    break
  case 8
    set trophy 53323
    set gem 55729
    set place Witch's Den
    set hint entombed with an ancient evil king.
    break
  case 9
    set trophy 52014
    set gem 55741
    set place Dargentan's Lair
    set hint at the pinnacle of his flying fortress.
    break
done
eval attacks %trophystage% * 100
nod
msend %actor% %self.name% says, 'With each act mission you'll earn a new trophy.  Undertake the following:
msend %actor% - Attack &2&b%attacks%&0 times while wearing your current trophy.
msend %actor% - Find &2&b%get.obj_shortdesc[%trophy%]%&0 as another demonstration of mastery over the beasts of the wild.
msend %actor% - Find &2&b%get.obj_shortdesc[%gem%]%&0 for decoration.
msend %actor% &0    
msend %actor% You also need to take your trophy and &2&b[forage]&0 in a great beast's home.
msend %actor% Find "&2&b%place%&0".  It's %hint%
msend %actor% &0  
msend %actor% You can ask about your &6&b[progress]&0 at any time.'
~
#5318
Ranger Trophy command forage~
1 c 3
forage~
switch %cmd%
  case f
  case fo
  case for
    return 0
    halt
done
set trophystage %actor.quest_stage[ranger_trophy]%
switch %self.vnum%
  case 370
    if %actor.room% == 8614 && %trophystage% == 1
      set continue yes
    endif
    break
  case 371
    if (%actor.room% >= 20352 && %actor.room% <= 20354) && %trophystage% == 2
      set continue yes
    endif
    break
  case 372
    if (%actor.room% == 2392 || %actor.room% == 2410) && %trophystage% == 3
      set continue yes
    endif
    break
  case 373
    if %actor.room% == 46244 && %trophystage% == 4
      set continue yes
    endif
    break
  case 374
    if %actor.room% == 12429 && %trophystage% == 5
      set continue yes
    endif
    break
  case 375
    if %actor.room% == 53568 && %trophystage% == 6
      set continue yes
    endif
    break
  case 376
    if %actor.room% == 49094 && %trophystage% == 7
      set continue yes
    endif
    break
  case 377
    if %actor.room% == 48080 && %trophystage% == 8
      set continue yes
    endif
    break
  case 378
    if %actor.room% == 23892 && %trophystage% == 9
      set continue yes
    endif
done
if %continue% == yes
  osend %actor% You forage through the lair.
  wait 2
  osend %actor% &2&bYour connection to %self.shortdesc% takes on new meaning!&0
  quest variable ranger_trophy %actor% trophytask4 1
else
  return 0
endif
~
#5319
Ranger Trophy Pumahl progress~
0 d 1
status progress~
wait 2
msend %actor% &2&bBeast Masters&0
if %actor.has_completed[beast_master]%
  msend %actor% %self.name% says, 'You've already proven your dominion over the beasts of Ethilien!'
elseif %actor.level% >= (%actor.quest_stage[beast_master]% - 1) * 10
  if %actor.quest_variable[beast_master:hunt]% != running
    msend %actor% %self.name% says, 'You aren't on a hunt at the moment.'
  else
    switch %actor.quest_stage[beast_master]% 
      case 1
        msend %actor% %self.name% says, 'You're still on the hunt.  Slay an abominable slime creature in the sewers beneath Mielikki.'
        break
      case 2
        msend %actor% %self.name% says, 'You're still on the hunt.  Hunt down a large buck in the forests just outside of Mielikki.'
        break
      case 3
        msend %actor% %self.name% says, 'You're still on the hunt.  Track down the giant scorpion of Gothra.'
        break
      case 4
        msend %actor% %self.name% says, 'You're still on the hunt.  Head far to the south and kill a monstrous canopy spider.'
        break
      case 5
        msend %actor% %self.name% says, 'You're still on the hunt.  Behead the famed chimera of Fiery Island.'
        break
      case 6
        msend %actor% %self.name% says, 'You're still on the hunt.  Slay the "king" of the abominations known as driders.'
        break
      case 7
        msend %actor% %self.name% says, 'You're still on the hunt.  Close the eyes of a beholder from under Mt. Frostbite.'
        break
      case 8
        msend %actor% %self.name% says, 'You're still on the hunt.  Lay the Banshee to eternal rest.'
        break
      case 9
        msend %actor% %self.name% says, 'You're still on the hunt.  Put an end to Baba Yaga's dreamy witchcraft.'
        break
      case 10
        msend %actor% %self.name% says, 'You're still on the hunt.  Defeat the medusa below the city of Templace.'
    done
  endif
else
  msend %actor% %self.name% says, 'Unfortunately I don't have any beasts for you to pursue at the moment.  Check back later!'
endif
if %actor.class% == Warrior || %actor.class% == Ranger || %actor.class% == Berserker || %actor.class% == Mercenary
  msend %actor%  &0 
  msend %actor% &2&bEye of the Tiger&0
  set huntstage %actor.quest_stage[beast_master]%
  set trophystage %actor.quest_stage[ranger_trophy]%
  set job1 %actor.quest_variable[ranger_trophy:trophytask1]%
  set job2 %actor.quest_variable[ranger_trophy:trophytask2]%
  set job3 %actor.quest_variable[ranger_trophy:trophytask3]%
  set job4 %actor.quest_variable[ranger_trophy:trophytask4]%
  if %actor.level% < 10
    msend %actor% %self.name% says, 'You aren't ready for this test yet.  Come back when you've grown a bit.'
    halt
  elseif %actor.level% < (%trophystage% * 10)
    msend %actor% %self.name% says, 'You aren't ready for another test yet.  Come back when you've gained some more experience.'
    halt
  elseif %actor.has_completed[ranger_trophy]%
    msend %actor% %self.name% says, 'You've already proven your skills as much as possible!'
    halt
  endif
  if %trophystage% == 0
    msend %actor% %self.name% says, 'You must &6&b[hunt]&0 a great beast to demonstrate your skills first.'
    halt
  elseif (%trophystage% >= %huntstage%) && !%actor.has_completed[beast_master]%
    msend %actor% %self.name% says, 'Prove your dominion over some more great beasts first and then we can talk.'
    halt
  endif
  switch %trophystage%
    case 1
      set trophy 1607
      set gem 55579
      set place A Coyote's Den
      set hint near the Kingdom of the Meer Cats.
      break
    case 2
      set trophy 17806
      set gem 55591
      set place In the Lions' Den
      set hint in the western reaches of Gothra.
      break
    case 3
      set trophy 1805
      set gem 55628
      set place either of the two Gigantic Roc Nests
      set hint in the Wailing Mountains.
      break
    case 4
      set trophy 62513
      set gem 55652
      set place Chieftain's Lair
      set hint in Nukreth Spire in South Caelia.
      break
    case 5
      set trophy 23803
      set gem 55664
      set place The Heart of the Den
      set hint where the oldest unicorn in South Caelia makes its home.
      break
    case 6
      set trophy 43009
      set gem 55685
      set place Giant Lynx's Lair
      set hint far to the north beyond Mt. Frostbite.
      break
    case 7
      set trophy 47008
      set gem 55705
      set place Giant Griffin's Nest
      set hint tucked away in a secluded and well guarded corner of Griffin Island.
      break
    case 8
      set trophy 53323
      set gem 55729
      set place Witch's Den
      set hint entombed with an ancient evil king.
      break
    case 9
      set trophy 52014
      set gem 55741
      set place Dargentan's Lair
      set hint at the pinnacle of his flying fortress.
      break
  done
  eval attack %trophystage% * 100
  if %job1% || %job2% || %job3% || %job4% 
    msend %actor% %self.name% says, 'You've done the following:'
    if %job1%
      msend %actor% - attacked %attack% times
    endif
    if %job2%
      msend %actor% - found %get.obj_shortdesc[%trophy%]%
    endif
    if %job3%
      msend %actor% - found %get.obj_shortdesc[%gem%]%
    endif
    if %job4%
      msend %actor% - foraged in %place%
    endif
  endif
  msend %actor%
  msend %actor% You need to:
  if %job1% && %job2% && %job3% && %job4%
    msend %actor% Just give me your old trophy.
    halt
  endif
  if !%job1%
    eval remaining %attack% - %actor.quest_variable[ranger_trophy:attack_counter]%
    msend %actor% - attack &2&b%remaining%&0 more times while wearing your trophy.
  endif
  if !%job2%
    msend %actor% - find &2&b%get.obj_shortdesc[%trophy%]%&0
  endif
  if !%job3%
    msend %actor% - find &2&b%get.obj_shortdesc[%gem%]%&0
  endif
  if !%job4%
    msend %actor% - &2&bforage&0 in a place called "&2&b%place%&0".
    msend %actor%&0   It's &2&b%hint%&0
  endif
endif
~
#5320
Honus Treasure Hunter greet~
0 h 100
~
wait 2
if !%actor.has_completed[treasure_hunter]%
  if !%actor.quest_stage[treasure_hunter]%
    msend %actor% %self.name% says, 'Well hello and welcome!  I'm Honus, global representative of the Guild of Treasure Hunters.  We're always seeking bold explorers to &6&b[hunt]&0 down and recover exquisite rarities.'
    wait 1s
    msend %actor% %self.name% says, 'We pay well, of course.'
  elseif %actor.quest_variable[treasure_hunter:hunt]% == running
    msend %actor% %self.name% says, 'You're still on the hunt.  What are you doing here?  If you lost your order say, &6&b"I need a new order".&0'
  elseif %actor.quest_variable[treasure_hunter:hunt]% == found
    msend %actor% %self.name% says, 'Welcome back!  If your hunt was successful give me your order.  If you lost your order say, &6&b"I need a new order".&0'
  elseif %actor.quest_stage[treasure_hunter]% >= 1 && !%actor.has_completed[treasure_hunter]%
    msend %actor% %self.name% says, 'Ah, back to &6&b[hunt]&0 for more treasure!'
  endif
  if !%actor.has_completed[rogue_cloak]% && %actor.level% > 9 && (%actor.class% /= Rogue || %actor.class% /= Thief || %actor.class% /= Bard)
    wait 1s
    msend %actor% %self.name% says, 'Or maybe you're here to seek a &6&b[promotion]&0.'
  endif
else
  if (%actor.class% /= Rogue || %actor.class% /= Thief || %actor.class% /= Bard) && !%actor.quest_stage[rogue_cloak]% && %actor.level% > 9
    msend %actor% %self.name% says, 'Oh look, a new recruit gunning for a &6&b[promotion]&0.'
  elseif %actor.quest_stage[rogue_cloak]% && !%actor.has_completed[rogue_cloak]%
    msend %actor% %self.name% says, 'Ah, you must be looking for another &6&b[promotion]&0.'
  endif
endif
~
#5321
Honus speech hunt treasure~
0 d 100
hunt treasure~
wait 2
if %actor.has_completed[treasure_hunter]%
  msend %actor% %self.name% says, 'Only great treasures, the stuff of legend, still wait out there!'
elseif !%actor.quest_stage[treasure_hunter]%
  msend %actor% %self.name% says, 'Treasure hunting is equal parts cleverness and strength.  You can't always just hack your way through the hordes to your prize.  You have to be smart.'
  wait 2
  msend %actor% %self.name% says, 'If you want to see what I mean, here's one to start with.'
  wait 4
  msend %actor% %self.name% says, 'Out east is an enchanted hollow.  I hear tell a fancy weapon, a strange singing chain, is kept as a prized possession by a nixie deep inside.'
  wait 4
  msend %actor% %self.name% says, 'I'll pay you well if you can bring it back.  The place is filthy with faeries though, so be prepared for puzzles and trickery at every turn!'
  wait 4
  msend %actor% %self.name% says, 'Are you up to the challenge?'
elseif %actor.quest_variable[treasure_hunter:hunt]% == found
  msend %actor% %self.name% says, 'Give me your current order first.'
  halt
else
  if %actor.level% >= (%actor.quest_stage[treasure_hunter]% - 1) * 10
    switch %actor.quest_stage[treasure_hunter]% 
      case 1
        if %actor.quest_variable[treasure_hunter:hunt]% == running
          msend %actor% %self.name% says, 'You still have a treasure to find.  Find that singing chain and I'll pay you for your time.'
        endif
        break
      case 2
        if %actor.quest_variable[treasure_hunter:hunt]% == running
          msend %actor% %self.name% says, 'You still have a treasure to find.  Find one of the true fire rings the theatre in Anduin gives out.  Not the fake prop ones most of the performers carry around, but the real ones they give out at their grand finale.'
        else
          msend %actor% %self.name% says, 'The city of Anduin has a world-famous theatre company that puts on lavish spectacles of murder and mayhem.  It might seem unlikely, but they have a few exceedingly rare treasures they keep too!'
          wait 4
          msend %actor% %self.name% says, 'One is a magic weapon, a ring made of pure fire.  They allegedly only give them out to participants of their "grand finale," which they rarely perform.'
          wait 4
          msend %actor% %self.name% says, 'See if you can get them to perform it and bring back one of those rings.'
          wait 4
          msend %actor% %self.name% says, 'You wanna give it a go?'
        endif
        break
      case 3
        if %actor.quest_variable[treasure_hunter:hunt]% == running
          msend %actor% %self.name% says, 'You still have a treasure to find.  Find a sandstone ring in the caves to the west.'
        else
          msend %actor% %self.name% says, 'There's a whacky old guy who lives in the Gothra Desert out west.  Claims he caught him a giant scorpion or something.'
          wait 4        
          msend %actor% %self.name% says, 'I don't care about that part.  What I AM interested in is a small ring he apparently locked in a cave with the scorpion.  I hear it's made out of sandstone and has some pretty special properties.'
          wait 4
          msend %actor% %self.name% says, 'I'd wager the old man is the only one who knows how to get into the cave.  You'll have to work with him to figure out how to open it and get inside.'
          wait 4
          msend %actor% %self.name% says, 'Are you up for a little spelunking?'
        endif
        break
      case 4
        if %actor.quest_variable[treasure_hunter:hunt]% == running
          msend %actor% %self.name% says, 'You still have a treasure to find.  Recover the electrum hoop lost in the bayou shipwreck.'
        else
          msend %actor% %self.name% says, 'You know who's really good at collecting treasure?  Pirates.  They're also really good at wrecking their ships and leaving their treasure for other people to find after they die.'
          wait 4
          msend %actor% %self.name% says, 'Out past the forests to the east is a bayou which apparently at one time connected to the Arabel Ocean.  There, the remains of a shipwreck peak up from the marsh.'
          wait 4
          msend %actor% %self.name% says, 'And so do the remains of the crew!  The place is filthy with zombies!'
          wait 4
          msend %actor% %self.name% says, 'But it seems the ship crashed because of some nefarious treasure the captain had brought on board.  Check it out, see if you can find out what it was.'
          wait 4
          msend %actor% %self.name% says, 'Ready to plunder the ship?'
        endif
        break
      case 5
        if %actor.quest_variable[treasure_hunter:hunt]% == running
          msend %actor% %self.name% says, 'You still have a treasure to find.  Bring me back a Rainbow Shell from the volcanic islands.'
        else
          msend %actor% %self.name% says, 'You might be surprised to know shells can be some of the most valuable things in the world.  Some cultures even use them as currency still!'
          wait 4
          msend %actor% %self.name% says, 'In particular, there's an extremely rare shell that can only be found on the volcanic island in the Arabel Ocean.  One of the tribal groups out there apparently gives them out as precious gifts.'
          wait 4
          msend %actor% %self.name% says, 'Think you can figure out how to get your hands on one for me?'
        endif
        break
      case 6
        if %actor.quest_variable[treasure_hunter:hunt]% == running
          msend %actor% %self.name% says, 'You still have a treasure to find.  Raid Mystwatch and bring back the legendary Stormshield.'
        else
          msend %actor% %self.name% says, 'Word has it the Knights Templar are preparing to raid the Fortress of Mystwatch just north of Mielikki.  And that means it's open season on the wonders inside.'
          wait 4
          msend %actor% %self.name% says, 'The demons that control Mystwatch are a unique species, said to control the power of the storm itself.  One of their most powerful relics is something called the Stormshield, a pitch black disc forged from pure night with the power of lightning.'
          wait 4
          msend %actor% %self.name% says, 'Why don't you join an upcoming raid and see if you can find the shield?  I'll make it worth your time.'
          wait 4
          msend %actor% %self.name% says, 'You up for it?'
        endif
        break
      case 7
        if %actor.quest_variable[treasure_hunter:hunt]% == running
          msend %actor% %self.name% says, 'You still have a treasure to find.  Get your hands on a Snow Leopard Cloak.'
        else
          msend %actor% %self.name% says, 'Up North is a secretive society built on worship of the Great Snow Leopard.'  
          wait 4
          msend %actor% %self.name% says, 'While they have great power still, they apparently lost the keys to some of their deepest secrets in various wars over the centuries.  I hear they award a Snow Leopard Cloak to those who can help solve some of those mysteries.'
          wait 4
          msend %actor% %self.name% says, 'See if you can figure it out and bring back a cloak.  You ready to brave the cold?'
        endif
        break
      case 8
        if %actor.quest_variable[treasure_hunter:hunt]% == running
          msend %actor% %self.name% says, 'You still have a treasure to find.  Find a magic ladder that uncoils itself.'
        else
          msend %actor% %self.name% says, 'So I heard about another magic trinket floating around the islands in the Arabel Ocean.'
          wait 4
          msend %actor% %self.name% says, 'This one is supposedly a magic ladder that can coil and uncoil itself without anyone holding it.  Just think of all the places we could get into if we had one of those!'
          wait 4
          msend %actor% %self.name% says, 'See if you can figure out who has one and what you need to do to get it from them.'
          wait 4
          msend %actor% %self.name% says, 'You interested?'
        endif
        break
      case 9
        if %actor.quest_variable[treasure_hunter:hunt]% == running
          msend %actor% %self.name% says, 'You still have a treasure to find.  Seek out a glowing phoenix feather.'
        else
          msend %actor% %self.name% says, 'You've done such a great job, I'm moving you beyond the typical magic weapons and jewelry to the real legendary stuff.'
          wait 4
          msend %actor% %self.name% says, 'There's a place in the Syric Mountains where legends literally come to life.  All manner of fantastical creatures, and their treasures, can be found there.'
          wait 4
          msend %actor% %self.name% says, 'Some have even said the glowing feather of a phoenix can be found for the right price.  Whatever that price is, pay it and bring back one of those feathers.'
          wait 4
          msend %actor% %self.name% says, 'Are you up to the challenge?'
        endif
        break
      case 10
        if %actor.quest_variable[treasure_hunter:hunt]% == running
          msend %actor% %self.name% says, 'You still have a treasure to find.  Secure a piece of sleet armor.'
        else
          msend %actor% %self.name% says, 'This last thing I'm looking for hasn't been seen for centuries.'
          wait 4
          msend %actor% %self.name% says, 'The frost elves had a kind of magical metal they called "sleet" - as light as cloth but hard as ice.  It was probably a relative of mythril, but since none exists now that's only speculation.'
          wait 4
          msend %actor% %self.name% says, 'However, it seems a device exists for traveling through time to a place where the frost elves still live.  If you could find that device, you might be able to figure out where to go to get a piece of sleet armor.'
          wait 4
          msend %actor% %self.name% says, 'But BE READY.  Legend has it the frost elves were some of the most deadly warriors Ethilien has ever seen.  Stealing anything from them is surely to be the fight of your life.'
          wait 4
          msend %actor% %self.name% says, 'Ready to take a trip through time?'
        endif
    done
  else
    msend %actor% %self.name% says, 'There's still plenty of treasure out there, but it's too dangerous without more experience.  Come back when you've grown a little more.'
  endif
endif
~
#5322
Honus speech yes~
0 d 100
yes~
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
    if !%person.quest_stage[treasure_hunter]%
      quest start treasure_hunter %person%
    endif
    if %person.has_completed[treasure_hunter]%
      msend %person% %self.name% says, 'Only great treasures, the stuff of legend, still wait out there!'
    elseif %person.quest_variable[treasure_hunter:hunt]% == found
      msend %person% %self.name% says, 'Give me your current order first.'
    else
      if %person.level% >= (%person.quest_stage[treasure_hunter]% - 1) * 10
        if %person.quest_variable[treasure_hunter:hunt]% != running
          msend %person% %self.name% says, 'Excellent!'
          switch %person.quest_stage[treasure_hunter]%
            case 1
              set order 5310
              break
            case 2
              set order 5311
              break
            case 3
              set order 5312
              break
            case 4
              set order 5313
              break
            case 5
              set order 5314
              break
            case 6
              set order 5315
              break
            case 7
              set order 5316
              break
            case 8
              set order 5317
              break
            case 9
              set order 5318
              break
            case 10
              set order 5319
              break
          done
          mload obj %order%
          give order %person%
          msend %person% &0  
          msend %person% %self.name% says, 'When you've secured the goods, bring it and that order back to me.  I'll reward you then.  You can check your &6&b[progress]&0 at any time.'
          msend %person% &0   
          msend %person% %self.name% says, 'Have fun!'
          quest variable treasure_hunter %person% hunt running
        else
          switch %person.quest_stage[treasure_hunter]% 
            case 1
              msend %person% %self.name% says, 'You still have a treasure to find.  Find that singing chain and I'll pay you for your time.'
              break
            case 2
              msend %person% %self.name% says, 'You still have a treasure to find.  Find one of the true fire rings the theatre in Anduin gives out.  Not the fake prop ones most of the performers carry around, but the real ones they give out at their grand finale.'
              break
            case 3
              msend %person% %self.name% says, 'You still have a treasure to find.  Find a sandstone ring in the caves to the west.'
              break
            case 4
              msend %person% %self.name% says, 'You still have a treasure to find.  Recover the electrum hoop lost in the bayou shipwreck.'
              break
            case 5
              msend %person% %self.name% says, 'You still have a treasure to find.  Bring me back a Rainbow Shell from the volcanic islands.'
              break
            case 6
              msend %person% %self.name% says, 'You still have a treasure to find.  Raid Mystwatch and bring back the legendary Stormshield.'
              break
            case 7
              msend %person% %self.name% says, 'You still have a treasure to find.  Get your hands on a Snow Leopard Cloak.'
              break
            case 8
              msend %person% %self.name% says, 'You still have a treasure to find.  Find a magic ladder that uncoils itself.'
              break
            case 9
              msend %person% %self.name% says, 'You still have a treasure to find.  Seek out a glowing phoenix feather.'
              break
            case 10
              msend %person% %self.name% says, 'You still have a treasure to find.  Secure a piece of sleet armor.'
          done
        endif
      else
          msend %person% %self.name% says, 'There's still plenty of treasure out there, but it's too dangerous without more experience.  Come back when you've grown a little more.'
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#5323
Honus order receive~
0 j 100
5310 5311 5312 5313 5314 5315 5316 5317 5318 5319~
switch %object.vnum%
  * hunt orders here
  case 5310
    set stage 1
    set treasure1 a singing chain
    break
  case 5311
    set stage 2
    set treasure1 a fire ring
    break
  case 5312
    set stage 3
    set treasure1 a sandstone ring
    break
  case 5313
    set stage 4
    set treasure1 a crimson-tinged electrum hoop
    break
  case 5314
    set stage 5
    set treasure1 a Rainbow Shell
    break
  case 5315
    set stage 6
    set treasure1 the Stormshield
    break
  case 5316
    set stage 7
    set treasure1 the Snow Leopard Cloak
    break
  case 5317
    set stage 8
    set treasure1 a coiled rope ladder
    break
  case 5318
    set stage 9
    set treasure1 a glowing phoenix feather
    break
  case 5319
    set stage 10
    set treasure1 a piece of sleet armor
done
if %actor.quest_stage[treasure_hunter]% == %stage% && %actor.quest_variable[treasure_hunter:hunt]% == returned
  set anti Anti-Paladin
  wait 2
  mjunk %object%
  cheer
  msend %actor% %self.name% says, 'Congratulations!  Here's your reward.'
  eval money %stage% * 10
  give %money% platinum %actor%
  if %stage% == 1
    set expcap 5
  else
    eval bonus (%stage% - 1) * 10
    set expcap %bonus%
  endif
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
  switch %person.class%
    case Warrior
    case Berserker 
        eval expmod (%expmod% + (%expmod% / 10))
        break
    case Paladin
    case %anti%
    case Ranger
        eval expmod (%expmod% + ((%expmod% * 2) / 15))
        break
    case Sorcerer
    case Pyromancer
    case Cryomancer
    case Illusionist
    case Bard
        eval expmod (%expmod% + (%expmod% / 5))
        break
    case Necromancer
    case Monk
        eval expmod (%expmod% + (%expmod% * 2) / 5)
        break
    default
        set expmod %expmod%
  done
  msend %actor% &3&bYou gain experience!&0
  eval setexp (%expmod% * 10)
  set loop 0
  while %loop% < 3
    mexp %actor% %setexp%
    eval loop %loop% + 1
  done
  quest variable treasure_hunter %actor% treasure1 0
  quest variable treasure_hunter %actor% hunt 0
  wait 2
  if %stage% < 10
    quest advance treasure_hunter %actor%
    msend %actor% %self.name% says, 'Check in again if you have time for more work.'
  else
    quest complete treasure_hunter %actor%
    msend %actor% %self.name% says, 'You have earned your place among the greatest treasure hunters in the realm!'
    wait 1s
    msend %actor% %self.name% says, 'Here, take this bracelet as payment for all you've done.  Wear this proudly.'
    mload obj 12417
    give bracelet %actor%
  endif
  if (%actor.class% /= Rogue || %actor.class% /= Thief || %actor.class% /= Bard) && %actor.quest_stage[rogue_cloak]% == 0
    wait 2s
    msend %actor% %self.name% says, 'I think you've earned this too.'
    mload obj 380
    give cloak %actor%
    wait 1s
    msend %actor% %self.name% says, 'Cloaks like these show off your rank in the various cloak and dagger guilds.'
    quest start rogue_cloak %actor%
    wait 2s
    if %actor.level% > 9
      msend %actor% %self.name% says, 'This puts you in line for a &6&b[promotion]&0.'
    else
      msend %actor% %self.name% says, 'Come back with that cloak after you reach level 10.  We can discuss a promotion then.'
    endif
  endif
elseif %actor.quest_stage[treasure_hunter]% > %stage%
  return 0
  shake
  mecho %self.name% refuses the order.
  wait 2
  msend %actor% %self.name% says, 'You already stole - er, recovered this treasure!'
elseif %actor.quest_stage[treasure_hunter]% < %stage%
  wait 2
  eye %actor%
  msend %actor% %self.name% says, 'How'd you get this?!  You steal it off someone else??'
  mecho %self.name% rips up the order!
  mjunk %object%
elseif %actor.quest_variable[treasure_hunter:hunt]% == running
  return 0
  mecho %self.name% refuses the order.
  wait 2
  msend %actor% %self.name% says, 'You have to find the treasure still!  %treasure1% is still out there.
elseif %actor.quest_variable[treasure_hunter:hunt]% == found
  return 0
  mecho %self.name% refuses the order.
  wait 2
  msend %actor% %self.name% says, 'You have to give me the treasure first!  You still have %treasure1% in your possession.'
endif
~
#5324
Treasure Hunter treasure get~
1 g 100
~
switch %self.vnum%
  case 61514
    * singing chain
    set stage 1
    break
  case 4319
    * fire ring
    set stage 2
    break
  case 16103
    * sandstone ring
    set stage 3
    break
  case 50215
    * electrum hoop
    set stage 4
    break
  case 48101
    * rainbow shell
    set stage 5
    break
  case 16009
    * stormshield
    set stage 6
    break
  case 55008
    * snow leopard cloak
    set stage 7
    break
  case 49041
    * rope ladder
    set stage 8
    break
  case 58401
    * phoenix feather
    set stage 9
    break
  case 53500
  case 53501
  case 53505
  case 53506
    * sleet armor
    set stage 10
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
    if %person.quest_stage[treasure_hunter]% == %stage% && %person.quest_variable[treasure_hunter:hunt]% == running
      switch %self.vnum%
        case 61514
          * singing chain
          set stage 1
          set variable %person.has_completed[enchanted_hollow_quest]%
          break
        case 4319
          * fire ring
          set stage 2
          set variable %person.quest_variable[theatre:fire_ring]%
          break
        case 16103
          * sandstone ring
          set stage 3
          set variable %person.quest_variable[desert_quest:scorpion]%
          break
        case 50215
          * electrum hoop
          set stage 4
          set variable %person.has_completed[bayou_quest]%
          break
        case 48101
          * rainbow shell
          set stage 5
          set variable %person.quest_variable[fieryisle_quest:shell]%
          break
        case 16009
          * stormshield
          set stage 6
          if %person.quest_variable[mystwatch_quest:step]% == complete
            set variable 1
          endif
          break
        case 55008
          * snow leopard cloak
          set stage 7
          set variable %person.quest_variable[tech_mysteries_quest:cloak]%
          break
        case 49041
          * rope ladder
          set stage 8
          set variable %person.quest_variable[griffin_quest:ladder]%
          break
        case 58401
          * phoenix feather
          set stage 9
          set variable %person.quest_variable[KoD_quest:feather]%
          break
        case 53500
        case 53501
        case 53505
        case 53506
          * sleet armor
          set stage 10
          set variable %person.quest_variable[frost_valley_quest:elf]%
      done
      if %variable%
        quest variable treasure_hunter %person% hunt found
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#5325
Honus treasure receive~
0 j 100
61514 4319 16103 50215 48101 16009 55008 49041 58401 53500 53501 53505 53506~
switch %object.vnum%
  case 61514
    * singing chain
    set stage 1
    break
  case 4319
    * fire ring
    set stage 2
    break
  case 16103
    * sandstone ring
    set stage 3
    break
  case 50215
    * electrum hoop
    set stage 4
    break
  case 48101
    * rainbow shell
    set stage 5
    break
  case 16009
    * stormshield
    set stage 6
    break
  case 55008
    * snow leopard cloak
    set stage 7
    break
  case 49041
    * rope ladder
    set stage 8
    break
  case 58401
    * phoenix feather
    set stage 9
    break
  case 53500
  case 53501
  case 53505
  case 53506
    * sleet armor
    set stage 10
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
    if %person.quest_stage[treasure_hunter]% == %stage%
      if %person.quest_variable[treasure_hunter:hunt]% == found
        quest variable treasure_hunter %person% hunt returned
        set accept 1
        set refuse 0
      elseif %actor.quest_variable[treasure_hunter:hunt]% == running
        if %accept%
          set refuse 0
        else
          set refuse 1
        endif
      elseif %actor.quest_variable[treasure_hunter:hunt]% == returned
        if %accept%
          set refuse 0
        else
          set refuse 2
        endif
      elseif %actor.quest_stage[treasure_hunter]% > %stage%
        if %accept%
          set refuse 0
        else
          set refuse 2
        endif
      elseif %actor.quest_stage[treasure_hunter]% < %stage%
        if %accept%
          set refuse 0
        else
          set refuse 3
        endif      
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done    
if %accept% == 1
  wait 2
  mjunk %object%
  grin    
  mecho %self.name% says, 'Excellent!  Now just give me the order paperwork and I can pay you.'
elseif %refuse%
  return 0
  shake
  mecho %self.name% refuses %object.shortdesc%.
   if %refuse% == 1
    wait 2
    mecho %self.name% says, 'Nah nah nah, you gotta put in at least a little effort toward finding this yourself, come on.'
  elseif %refuse% == 2
    wait 2
    mecho %self.name% says, 'You already stole - er, recovered this treasure!'
  elseif %refuse% == 3
    wait 2
    mecho %self.name% says, 'I'm not looking for that...  Yet.'
  endif
endif
~
#5326
Honus cloak receive~
0 j 100
380 58801 55585 381 17307 55593 382 10308 55619 383 12325 55659 384 43022 55663 385 23810 55674 386 51013 55714 387 58410 55740 388 52009 55741~
switch %object.vnum%
  case 380
    set cloakstage 1
    set item quest
    break
  case 58801
    set cloakstage 1
    set item cloak
    break
  case 55585
    set cloakstage 1
    set item gem
    break
  case 381
    set cloakstage 2
    set item quest
    break
  case 17307
    set cloakstage 2
    set item cloak
    break
  case 55593
    set cloakstage 2
    set item gem
    break
  case 382
    set cloakstage 3
    set item quest
    break
  case 10308
    set cloakstage 3
    set item cloak
    break
  case 55619
    set cloakstage 3
    set item gem
    break
  case 383
    set cloakstage 4
    set item quest
    break
  case 12325
    set cloakstage 4
    set item cloak
    break
  case 55659
    set cloakstage 4
    set item gem
    break
  case 384
    set cloakstage 5
    set item quest
    break
  case 43022
    set cloakstage 5
    set item cloak
    break
  case 55663
    set cloakstage 5
    set item gem
    break
  case 385
    set cloakstage 6
    set item quest
    break
  case 23810
    set cloakstage 6
    set item cloak
    break  
  case 55674
    set cloakstage 6
    set item gem
    break
  case 386
    set cloakstage 7
    set item quest
    break
  case 51013
    set cloakstage 7
    set item cloak
    break
  case 55714
    set cloakstage 7
    set item gem
    break
  case 387
    set cloakstage 8
    set item quest
    break
  case 58410
    set cloakstage 8
    set item cloak
    break
  case 55740
    set cloakstage 8
    set item gem
    break
  case 388
    set cloakstage 9
    set item quest
    break
  case 52009
    set cloakstage 9
    set item cloak
    break
  case 55741
    set cloakstage 9
    set item gem
done
if %actor.quest_stage[treasure_hunter]% < %actor.quest_stage[rogue_cloak]%
  return 0
  shake
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, 'Find some more treasure first.'
elseif %actor.level% < (%actor.quest_stage[rogue_cloak]% * 10)
  return 0
  shake
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, 'You need to gain some more experience first.'
elseif %cloakstage% > %actor.quest_stage[rogue_cloak]%
  return 0
  shake
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, 'Your promotion doesn't involve that yet.  Be patient!'
elseif %cloakstage% < %actor.quest_stage[rogue_cloak]%
  return 0
  shake
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, 'You've already earned that promotion.'
else
  if %item% == quest
    set job1 %actor.quest_variable[rogue_cloak:cloaktask1]%
    set job2 %actor.quest_variable[rogue_cloak:cloaktask2]%
    set job3 %actor.quest_variable[rogue_cloak:cloaktask3]%
    set job4 %actor.quest_variable[rogue_cloak:cloaktask4]%
    if %job1% && %job2% && %job3% && %job4%
      wait 2
      eval reward %object.vnum% + 1
      mjunk %object%
      nod
      msend %actor% %self.name% says, 'Well done!  You've earned your promotion.'
      mload obj %reward%
      give cloak %actor%
      eval expcap %cloakstage% * 10
      if %expcap% < 17
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
      eval expmod %expmod% + ((%expmod% * 2) / 15)
      msend %actor% &3&bYou gain experience!&0
      eval setexp (%expmod% * 10)
      set loop 0
      while %loop% < 7
        mexp %actor% %setexp%
        eval loop %loop% + 1
      done
      set number 1
      while %number% < 5
        quest variable rogue_cloak %actor% cloaktask%number% 0
        eval number %number% + 1
      done
      if %actor.quest_stage[rogue_cloak]% < 9
        quest advance rogue_cloak %actor%
      else
        quest complete rogue_cloak %actor%
      endif
    else
      return 0
      shake
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      msend %actor% %self.name% says, 'You need to do everything else before you trade in your cloak!'
    endif
  elseif %item% == cloak
    if %actor.quest_variable[rogue_cloak:cloaktask2]% == %object.vnum%
      set accept no
    else
      set accept yes
      quest variable rogue_cloak %actor% cloaktask2 %object.vnum%
    endif
  elseif %item% == gem
    if %actor.quest_variable[rogue_cloak:cloaktask3]% == %object.vnum%
      set accept no
    else
      set accept yes
      quest variable rogue_cloak %actor% cloaktask3 %object.vnum%
    endif
  endif
  if %accept% == no
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'You already gave me that.'
  elseif %accept% == yes
    wait 2
    mjunk %object%
    set job1 %actor.quest_variable[rogue_cloak:cloaktask1]%
    set job2 %actor.quest_variable[rogue_cloak:cloaktask2]%
    set job3 %actor.quest_variable[rogue_cloak:cloaktask3]%
    set job4 %actor.quest_variable[rogue_cloak:cloaktask4]%
    if %job1% && %job2% && %job3% && %job4%
      msend %actor% %self.name% says, 'Excellent.  Now trade in your cloak for your promotion.'
    else
      msend %actor% %self.name% says, 'Good, now finish the rest.'
    endif
  endif
endif
~
#5327
Treasure Hunter order look~
1 m 100
~
return 0
switch %self.vnum%
  case 5310
    set stage 1
    set treasure1 a singing chain
    break
  case 5311
    set stage 2
    set treasure1 a true fire ring
    break
  case 5312
    set stage 3
    set treasure1 a sandstone ring
    break
  case 5313
    set stage 4
    set treasure1 a crimson-tinged electrum hoop
    break
  case 5314
    set stage 5
    set treasure1 a Rainbow Shell
    break
  case 5315
    set stage 6
    set treasure1 the Stormshield
    break
  case 5316
    set stage 7
    set treasure1 the Snow Leopard Cloak
    break
  case 5317
    set stage 8
    set treasure1 a coiled rope ladder
    break
  case 5318
    set stage 9
    set treasure1 a glowing phoenix feather
    break
  case 5319
    set stage 10
    set treasure1 a piece of sleet armor
    break
done
osend %actor% This is an order to find %treasure1%.
if %actor.quest_variable[treasure_hunter:hunt]% == found && %actor.quest_stage[treasure_hunter]% == %stage%
  osend %actor% You have found the treasure.
  osend %actor% Return it and this order to Honus for your reward!
endif
~
#5328
Honus speech promotion~
0 d 100
promotion~
set cloakstage %actor.quest_stage[rogue_cloak]%
set huntstage %actor.quest_stage[treasure_hunter]%
set job1 %actor.quest_variable[rogue_cloak:cloaktask1]%
set job2 %actor.quest_variable[rogue_cloak:cloaktask2]%
set job3 %actor.quest_variable[rogue_cloak:cloaktask3]%
set job4 %actor.quest_variable[rogue_cloak:cloaktask4]%
wait 2
if %actor.class% != Rogue && %actor.class% != Bard && %actor.class% != Thief
  shake
  msend %actor% %self.name% says, 'We only promote within the cloak and dagger guilds.  Sorry.'
  halt
elseif %actor.level% < 10
  msend %actor% %self.name% says, 'You aren't ready for a promotion yet.  Come back when you've grown a bit.'
  halt
elseif %actor.level% < (%cloakstage% * 10)
  msend %actor% %self.name% says, 'You aren't ready for another promotion yet.  Come back when you've gained some more experience.'
  halt
elseif %actor.has_completed[rogue_cloak]%
  msend %actor% %self.name% says, 'You've already been promoted as high as you can go!'
  halt
endif
if %cloakstage% == 0
  mecho %self.name% says, 'Sure.  You gotta &6&b[hunt]&0 down more treasure first though.'
  halt
elseif (%cloakstage% >= %huntstage%) && !%actor.has_completed[treasure_hunter]%
  msend %actor% %self.name% says, 'Find some more treasures and then we can talk.'
  halt
elseif %job1% && %job2% && %job3% && %job4%
  msend %actor% %self.name% says, 'You're all ready, just give me your old cloak.'
  halt
endif
switch %cloakstage%
  case 1
    set cloak 58801
    set gem 55585
    set place A Storage Room
    set hint in the house on the hill.
    break
  case 2
    set cloak 17307
    set gem 55593
    set place A Small Alcove
    set hint in the holy library.
    break
  case 3
    set cloak 10308
    set gem 55619
    set place either Treasure Room
    set hint in the paladin fortress.
    break
  case 4
    set cloak 12325
    set gem 55659
    set place The Treasure Room
    set hint beyond the Tower in the Wastes.
    break
  case 5
    set cloak 43022
    set gem 55663
    set place Treasury
    set hint in the ghostly fortress.
    break
  case 6
    set cloak 23810
    set gem 55674
    set place either Treasure Room with a chest
    set hint lost in the sands.
    break
  case 7
    set cloak 51013
    set gem 55714
    set place Mesmeriz's Secret Treasure Room
    set hint hidden deep underground.
    break
  case 8
    set cloak 58410
    set gem 55740
    set place Treasure Room
    set hint sunken in the swamp.
    break
  case 9
    set cloak 52009
    set gem 55741
    set place Treasure Room
    set hint buried with an ancient king.
    break
done
eval attacks %cloakstage% * 100
nod
msend %actor% %self.name% says, 'With each promotion you'll earn a new cloak to signify your new station.  Undertake the following:
msend %actor% - Attack &3&b%attacks%&0 times while wearing your current cloak.
msend %actor% - Find &3&b%get.obj_shortdesc[%cloak%]%&0 as the base for the new cloak.
msend %actor% - Find &3&b%get.obj_shortdesc[%gem%]%&0 for decoration.
msend %actor% &0    
msend %actor% You also need to take your cloak and &3&b[search]&0 in various treasure rooms.
msend %actor% Find "&3&b%place%&0".  It's %hint%
msend %actor% &0  
msend %actor% You can ask about your &6&b[progress]&0 at any time.'
~
#5329
Rogue cloak search~
1 c 3
search~
switch %cmd%
  case s
  case se
    return 0
    halt
done
set cloakstage %actor.quest_stage[rogue_cloak]%
switch %self.vnum%
  case 380
    if %actor.room% == 13660 && %cloakstage% == 1
      set continue yes
    endif
    break
  case 381
    if %actor.room% == 18549 && %cloakstage% == 2
      set continue yes
    endif
    break
  case 382
    if (%actor.room% == 59080 || %actor.room% == 59081) && %cloakstage% == 3
      set continue yes
    endif
    break
  case 383
    if %actor.room% == 12626 && %cloakstage% == 4
      set continue yes
    endif
    break
  case 384
    if %actor.room% == 16068 && %cloakstage% == 5
      set continue yes
    endif
    break
  case 385
    if (%actor.room% == 16215 || %actor.room% == 16220) && %cloakstage% == 6
      set continue yes
    endif
    break
  case 386
    if %actor.room% == 37062 && %cloakstage% == 7
      set continue yes
    endif
    break
  case 387
    if %actor.room% == 53103 && %cloakstage% == 8
      set continue yes
    endif
    break
  case 388
    if %actor.room% == 48083 && %cloakstage% == 9
      set continue yes
    endif
done
if %continue% == yes
  oforce %actor% search
  osend %actor% &3&bYou have found your target!&0
  quest variable rogue_cloak %actor% cloaktask4 1
else
  return 0
endif
~
#5330
Rogue Cloak Progress~
0 d 100
status progress~
wait 2
msend %actor% &2&bTreasure Hunters&0
if %actor.has_completed[treasure_hunter]%
  msend %actor% %self.name% says, 'Only great treasures, the stuff of legend, still wait out there!'
elseif !%actor.quest_stage[treasure_hunter]%
  msend %actor% %self.name% says, 'You aren't doing anything for me right now.'
elseif %actor.quest_variable[treasure_hunter:hunt]% == found
  msend %actor% %self.name% says, 'Give me your current order first.'
elseif %actor.level% >= (%actor.quest_stage[treasure_hunter]% - 1) * 10
  if %actor.quest_variable[treasure_hunter:hunt]% != running
    msend %actor% %self.name% says, 'You aren't doing anything for me right now.'
  else
    switch %actor.quest_stage[treasure_hunter]% 
      case 1
        msend %actor% %self.name% says, 'You still have a treasure to find.  Find that singing chain and I'll pay you for your time.'
        break
      case 2
        msend %actor% %self.name% says, 'You still have a treasure to find.  Find one of the true fire rings the theatre in Anduin gives out.  Not the fake prop ones most of the performers carry around, but the real ones they give out at their grand finale.'
        break
      case 3
        msend %actor% %self.name% says, 'You still have a treasure to find.  Find a sandstone ring in the caves to the west.'
        break
      case 4
        msend %actor% %self.name% says, 'You still have a treasure to find.  Recover the electrum hoop lost in the bayou shipwreck.'
        break
      case 5
        msend %actor% %self.name% says, 'You still have a treasure to find.  Bring me back a Rainbow Shell from the volcanic islands.'
        break
      case 6
        msend %actor% %self.name% says, 'You still have a treasure to find.  Raid Mystwatch and bring back the legendary Stormshield.'
        break
      case 7
        msend %actor% %self.name% says, 'You still have a treasure to find.  Get your hands on a Snow Leopard Cloak.'
        break
      case 8
        msend %actor% %self.name% says, 'You still have a treasure to find.  Find a magic ladder that uncoils itself.'
        break
      case 9
        msend %actor% %self.name% says, 'You still have a treasure to find.  Seek out a glowing phoenix feather.'
        break
      case 10
        msend %actor% %self.name% says, 'You still have a treasure to find.  Secure a piece of sleet armor.'
    done
  endif
else
    msend %actor% %self.name% says, 'There's still plenty of treasure out there, but it's too dangerous without more experience.  Come back when you've grown a little more.'
endif
if %actor.class% == Rogue || %actor.class% == Bard || %actor.class% == Thief
  msend %actor%  &0
  msend %actor% &2&bCloak and Shadow&0
  set huntstage %actor.quest_stage[treasure_hunter]%
  set cloakstage %actor.quest_stage[rogue_cloak]%
  set job1 %actor.quest_variable[rogue_cloak:cloaktask1]%
  set job2 %actor.quest_variable[rogue_cloak:cloaktask2]%
  set job3 %actor.quest_variable[rogue_cloak:cloaktask3]%
  set job4 %actor.quest_variable[rogue_cloak:cloaktask4]%
  if %actor.level% < 10
    msend %actor% %self.name% says, 'You aren't ready for an promotion yet.  Come back when you've grown a bit.'
    halt
  elseif %actor.level% < (%cloakstage% * 10)
    msend %actor% %self.name% says, 'You aren't ready for another promotion yet.  Come back when you've gained some more experience.'
    halt
  elseif %actor.has_completed[rogue_cloak]%
    msend %actor% %self.name% says, 'You've already been promoted as high as you can go!'
    halt
  endif
  if %cloakstage% == 0
    mecho %self.name% says, 'Sure.  You gotta &6&b[hunt]&0 down more treasure first though.'
    halt
  elseif (%cloakstage% >= %huntstage%) && !%actor.has_completed[treasure_hunter]%
    msend %actor% %self.name% says, 'Find some more treasures and then we can talk.'
    halt
  endif
  switch %cloakstage%
    case 1
      set cloak 58801
      set gem 55585
      set place A Storage Room
      set hint in the house on the hill.
      break
    case 2
      set cloak 17307
      set gem 55593
      set place A Small Alcove
      set hint in the holy library.
      break
    case 3
      set cloak 10308
      set gem 55619
      set place either Treasure Room
      set hint in the paladin fortress.
      break
    case 4
      set cloak 12325
      set gem 55659
      set place The Treasure Room
      set hint beyond the Tower in the Wastes.
      break
    case 5
      set cloak 43022
      set gem 55663
      set place Treasury
      set hint in the ghostly fortress.
      break
    case 6
      set cloak 23810
      set gem 55674
      set place either Treasure Room with a chest
      set hint lost in the sands.
      break
    case 7
      set cloak 51013
      set gem 55714
      set place Mesmeriz's Secret Treasure Room
      set hint hidden deep underground.
      break
    case 8
      set cloak 58410
      set gem 55740
      set place Treasure Room
      set hint sunken in the swamp.
      break
    case 9
      set cloak 52009
      set gem 55741
      set place Treasure Room
      set hint buried with an ancient king.
      break
  done
  eval attack %cloakstage% * 100
  if %job1% || %job2% || %job3% || %job4% 
    msend %actor% %self.name% says, 'You've done the following:'
    if %job1%
      msend %actor% - attacked %attack% times
    endif
    if %job2%
      msend %actor% - found %get.obj_shortdesc[%cloak%]%
    endif
    if %job3%
      msend %actor% - found %get.obj_shortdesc[%gem%]%
    endif
    if %job4%
      msend %actor% - searched in %place%
    endif
  endif
  msend %actor%
  msend %actor% You need to:
  if %job1% && %job2% && %job3% && %job4%
    msend %actor% Just give me your old cloak.
    halt
  endif
  if !%job1%
    eval remaining %attack% - %actor.quest_variable[rogue_cloak:attack_counter]%
    msend %actor% - attack &3&b%remaining%&0 more times while wearing your cloak.
  endif
  if !%job2%
    msend %actor% - find &3&b%get.obj_shortdesc[%cloak%]%&0
  endif
  if !%job3%
    msend %actor% - find &3&b%get.obj_shortdesc[%gem%]%&0
  endif
  if !%job4%
    msend %actor% - &3&bsearch&0 in a place called "&3&b%place%&0".
    msend %actor%&0   It's &3&b%hint%&0
  endif
endif
~
#5331
Honus receive refuse~
0 j 100
~
set treasurestage %actor.quest_stage[treasure_hunter]%
set cloakstage %actor.quest_stage[rogue_cloak]%
switch %object.vnum%
  case 380
  case 58801
  case 55585
  case 381
  case 17307
  case 55593
  case 382
  case 10308
  case 55619
  case 383
  case 12325
  case 55659
  case 384
  case 43022
  case 55663
  case 385
  case 23810
  case 55674
  case 386
  case 51013
  case 55714
  case 387
  case 58410
  case 55740
  case 388
  case 52009
  case 55741 
  case 61514
  case 5310
  case 4319
  case 5311
  case 16103
  case 5312
  case 50215
  case 5313
  case 48101
  case 5314
  case 16009
  case 5315
  case 55008
  case 5316
  case 49041
  case 5317
  case 58401
  case 5318
  case 53500
  case 53501
  case 53505
  case 53506
  case 5319
    halt
    break
  default
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'What is this for?'
done
~
#5332
Treasure Hunter object give~
1 i 100
~
switch %self.vnum%
  case 61514
    * singing chain
    set stage 1
    break
  case 4319
    * fire ring
    set stage 2
    break
  case 16103
    * sandstone ring
    set stage 3
    break
  case 50215
    * electrum hoop
    set stage 4
    break
  case 48101
    * rainbow shell
    break
  case 16009
    * stormshield
    set stage 6
    break
  case 55008
    * snow leopard cloak
    set stage 7
    break
  case 49041
    * rope ladder
    set stage 8
    break
  case 58401
    * phoenix feather
    set stage 9
    break
  case 53500
  case 53501
  case 53505
  case 53506
    * sleet armor
    set stage 10
done
set person %victim%
set i %person.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[treasure_hunter]% == %stage% && %person.quest_variable[treasure_hunter:hunt]% == running
      switch %self.vnum%
        case 61514
          * singing chain
          set variable %person.has_completed[enchanted_hollow_quest]%
          break
        case 4319
          * fire ring
          set variable %person.quest_variable[theatre:fire_ring]%
          break
        case 16103
          * sandstone ring
          set variable %person.quest_variable[desert_quest:scorpion]%
          break
        case 50215
          * electrum hoop
          set variable %person.has_completed[bayou_quest]%
          break
        case 48101
          * rainbow shell
          set variable %person.quest_variable[fieryisle_quest:shell]%
          break
        case 16009
          * stormshield
          if %person.quest_variable[mystwatch_quest:step]% == complete
            set variable 1
          endif
          break
        case 55008
          * snow leopard cloak
          set variable %person.quest_variable[tech_mysteries_quest:cloak]%
          break
        case 49041
          * rope ladder
          set variable %person.quest_variable[griffin_quest:ladder]%
          break
        case 58401
          * phoenix feather
          set variable %person.quest_variable[KoD_quest:feather]%
          break
        case 53500
        case 53501
        case 53505
        case 53506
          * sleet armor
          set variable %person.quest_variable[frost_valley_quest:elf]%
      done
      if %variable%
        quest variable treasure_hunter %person% hunt found
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#5333
Pumahl new assignment~
0 d 0
I need a new assignment~
wait 2
if %actor.level% >= (%actor.quest_stage[beast_master]% - 1) * 10
  if %actor.quest_variable[beast_master:hunt]%
    switch %actor.quest_stage[beast_master]%
      case 1
        set notice 5300
        break
      case 2
        set notice 5301
        break
      case 3
        set notice 5302
        break
      case 4
        set notice 5303
        break
      case 5
        set notice 5304
        break
      case 6
        set notice 5305
        break
      case 7
        set notice 5306
        break
      case 8
        set notice 5307
        break
      case 9
        set notice 5308
        break
      case 10
        set notice 5309
        break
    done
    grumble
    mload obj %notice%
    give assignment %actor%
    msend %actor% %self.name% says, 'Don't lose this one!'
  endif
endif
~
#5334
Honus new order~
0 d 0
I need a new order~
wait 2
if %actor.level% >= (%actor.quest_stage[treasure_hunter]% - 1) * 10
  if %actor.quest_variable[treasure_hunter:hunt]%
    switch %actor.quest_stage[treasure_hunter]%
      case 1
        set order 5310
        break
      case 2
        set order 5311
        break
      case 3
        set order 5312
        break
      case 4
        set order 5313
        break
      case 5
        set order 5314
        break
      case 6
        set order 5315
        break
      case 7
        set order 5316
        break
      case 8
        set order 5317
        break
      case 9
        set order 5318
        break
      case 10
        set order 5319
        break
    done
    grumble
    mload obj %order%
    give order %actor%
    msend %actor% %self.name% says, 'Be more careful next time.'
  endif
endif
~
#5335
Elemental Chaos Hakujo greet~
0 g 100
~
wait 2
if !%actor.has_completed[elemental_chaos]%
  if !%actor.quest_stage[elemental_chaos]%
    bow
    msend %actor% %self.name% says, 'Welcome wanderer.  I have not seen the likes of you before.'
    wait 2s
    eye %actor%
    msend %actor% %self.name% says, 'Though now that I get a look at you, perhaps you can assist in our &6&b[mission]&0.'
  elseif %actor.quest_variable[elemental_chaos:bounty]% == running
    msend %actor% %self.name% says, 'What are you doing here so soon?  You still have a mission to accomplish.  If you misplaced it, say, &6&b"I need a new note"&0.'
  elseif %actor.quest_variable[elemental_chaos:bounty]% == dead
    msend %actor% %self.name% says, 'If you completed the deed, give me your mission.  If you misplaced it, say, &6&b"I need a new note"&0.'
  elseif %actor.quest_stage[elemental_chaos]% >= 1 && !%actor.has_completed[elemental_chaos]%
    msend %actor% %self.name% says, 'Ah, I have &6&b[news]&0 for you.'
  endif
  wait 1s
  if !%actor.quest_stage[monk_vision]% && %actor.level% > 9 && %actor.class% /= Monk
    msend %actor% %self.name% says, 'Or maybe you're here to discuss &6&b[enlightenment]&0.'
  elseif %actor.level% > 9 && %actor.class% /= Monk
    msend %actor% %self.name% says, 'Or have you come to seek further &6&b[enlightenment]&0.'
  endif
else
  if %actor.class% /= Monk && !%actor.quest_stage[monk_vision]% && %actor.level% > 9
    msend %actor% %self.name% says, 'Oh look, someone else in line for a &6&b[enlightenment]&0.'
  elseif %actor.quest_stage[monk_vision]% && !%actor.has_completed[monk_vision]%
    msend %actor% %self.name% says, 'Ah, you must be seeking further &6&b[enlightenment]&0.'
  endif
endif
~
#5336
Elemental Chaos Hakujo speech mission~
0 d 100
news update updates mission~
wait 2
if %actor.has_completed[elemental_chaos]%
  msend %actor% %self.name% says, 'We're still analyzing the information you brought us on your last mission.  Check back later.'
elseif !%actor.quest_stage[elemental_chaos]% || (%actor.quest_stage[elemental_chaos]% == 1 && !%actor.quest_variable[elemental_chaos:bounty]%)
  msend %actor% %self.name% says, 'Yes, something that will help bring Balance to the world.'
  wait 2
  msend %actor% %self.name% says, 'Those who follow the Way seek to preserve Harmony through universal Balance.  We are the anathema of Chaos.  When unnatural forces rise in unlikely places, we follow the Way to restore Balance.'
  wait 4s
  msend %actor% %self.name% says, 'Recently, I have heard rumor a reliable warrior commander in the far north has made an alliance with a strange bedfellow - a necromancer who trafficks with minor demons, commonly called imps.'
  wait 4s
  msend %actor% %self.name% says, 'Please go north and investigate.  The compound sits just outside of the town of Ickle.  If you discover any creatures like these, dispose of them.'
  wait 4s
  if %actor.level% < 20
    msend %actor% %self.name% says, 'Getting to Ickle from Mielikki by foot is extremely dangerous for new adventurers.  You may want to purchase a blue scroll of recall from Bigby's Magic Shoppe to take you there safely.'
    wait 4s
  endif
  msend %actor% %self.name% says, 'Can I count on you to get this done?'
elseif %actor.quest_variable[elemental_chaos:bounty]% == dead
  msend %actor% %self.name% says, 'Give me your current mission first.'
  halt
else
  if %actor.level% >= (%actor.quest_stage[elemental_chaos]% - 1) * 10
    switch %actor.quest_stage[elemental_chaos]%
      case 1
        if %actor.quest_variable[elemental_chaos:bounty]% == running
          msend %actor% %self.name% says, 'You still have a mission to complete.  Investigate the news of an imp and dispatch it if you find one.'
        endif
        break
      case 2
        if %actor.quest_variable[elemental_chaos:bounty]% == running
          msend %actor% %self.name% says, 'You still have a mission to accomplish.  Silence the seductive song of the Leading Player.'
        else
          msend %actor% %self.name% says, 'I'm still analyzing the information you gleaned about from Thraja's Ice Warrior Compound, but I've identified a second source of chaos right in the heart of civilization.'
          wait 4s
          msend %actor% %self.name% says, 'In Anduin is a suspicious theatre company, preparing for some ominous event.  Exactly what it is I do not know, but the are whispers they seek to bring death and destruction to their unwitting spectators!'
          wait 4s
          msend %actor% %self.name% says, 'I believe if you silence their leader, the troupe will fall apart.  Will you do this?'
        endif
        break
      case 3
        if %actor.quest_variable[elemental_chaos:bounty]% == running
          msend %actor% %self.name% says, 'You still have a mission to accomplish.  Destroy the Chaos and the cult worshipping it!'
        else
          msend %actor% %self.name% says, 'I've discovered something concerning in what you found in the Ice Warrior Compound.  It seems the necromancer she allied herself with was a missionary of a more sinister cult, one dedicated to the worship of Chaos itself!'
          wait 4s
          msend %actor% %self.name% says, 'My sources say they've erected a temple near the Gothra Desert, where they have managed to pull a manifestation of Chaos itself into our world!'
          wait 4s
          msend %actor% %self.name% says, 'If this is true, they need to be stopped immediately lest Balance be forever altered.  Are you willing to help?'
        endif
        break
      case 4
        if %actor.quest_variable[elemental_chaos:bounty]% == running
          msend %actor% %self.name% says, 'You still have a mission to accomplish.  Undertake the vision quest from the shaman in Three-Falls Canyon and defeat whatever awaits at the end.'
        else
          msend %actor% %self.name% says, 'You have done well so far, but I sense something sinister growing in the heart of the world.  Something malevolent...  Whatever it is, it seems to have entangled all of our fates.'
          wait 4s
          msend %actor% %self.name% says, 'Undertake the vision quest of the shaman in Three-Falls Canyon.  It may reveal what is pulling at the strings of fate.'
          wait 3s
          msend %actor% %self.name% says, 'Are you willing to do this?'
        endif
        break
      case 5
        if %actor.quest_variable[elemental_chaos:bounty]% == running
          msend %actor% %self.name% says, 'You still have a mission to accomplish.  Dispatch the Fangs of Yeenoghu.  Be sure to destroy all of them.'
        else
          msend %actor% %self.name% says, 'From what you have seen in your vision, it seems the chaos in the world is a symptom of demonic energy gnawing at the Wheel of Ages.'
          wait 4s
          msend %actor% %self.name% says, 'My own meditations have revealed several places where the fabric of reality is quickly unraveling due to their influence.'
          wait 3s
          msend %actor% %self.name% says, 'Of immediate concern is a den of filth-ridden animal abominations called gnolls in Nukreth Spire in South Caelia.'
          wait 3s
          msend %actor% %self.name% says, 'These creatures are the progeny of the demon lord Yeenoghu.  Their spiritual connection is maintained by a triumverant of gnolls known as his "Fangs."  Destroying them will certainly weaken the demonic chaos they spread.'
          wait 4s
          msend %actor% %self.name% says, 'Are you brave enough to take them on?'
        endif
        break
      case 6
        if %actor.quest_variable[elemental_chaos:bounty]% == running
          msend %actor% %self.name% says, 'You still have a mission to accomplish.  Extinguish the fire elemental lord who serves Krisenna.'
        else
          msend %actor% %self.name% says, 'With Yeenoghu crippled, other demon lords are putting their own plans into play.'
          wait 4s
          msend %actor% %self.name% says, 'Something is stirring within the tower in the wastelands West of the Black Rock Trail.  It appears there was once a gateway to another world there, but it has long since collapsed.'
          wait 4s
          msend %actor% %self.name% says, 'Something on the other side though seeks to return, a creature heralding the chaos and destruction of fire itself.'
          wait 4s
          msend %actor% %self.name% says, 'What do you think?  Can you get to the bottom of this?'
        endif
        break
      case 7
        if %actor.quest_variable[elemental_chaos:bounty]% == running
          msend %actor% %self.name% says, 'You still have a mission to accomplish.  Stop the acolytes in the Cathedral of Betrayal.'
        else
          msend %actor% %self.name% says, 'I have avoided thus far sending you after the more powerful and entrenched orders dedicated to the dark forces.  Unfortunately I cannot delay any longer.'
          wait 4s
          msend %actor% %self.name% says, 'It appears the diabolists who call the Cathedral of Betrayal their home are taking advantage of the disturbances across the world and using it to fuel their own dark purposes.'
          wait 4s
          msend %actor% %self.name% says, 'You can cripple their efforts if you kill the acolytes who prepare their altar.  It won't be a permanent solution but it will weaken them significantly.'
          wait 4s 
          msend %actor% %self.name% says, 'Will you harry them?'
        endif
        break
      case 8
        if %actor.quest_variable[elemental_chaos:bounty]% == running
          msend %actor% %self.name% says, 'You still have a mission to accomplish.  Destroy Cyprianum the Reaper in the heart of his maze.'
        else
          msend %actor% %self.name% says, 'Thanks to you I've discovered the Cathedral was involved in a plot to summon forth a catastrophic force.  Unfortunately I'm unsure if they sought to create a new disaster or make an existing one worse.'
          wait 3s
          msend %actor% %self.name% says, 'The greatest source of Chaos in Ethilian is a keep long ago overwhelmed by demonic forces.  Formerly the home of Seblan the Young, it is now his prison, ruled by Cyprianum the Reaper.  Cyprianum has warped the fortress into a maze, filled with soul-consuming beasts and mechanical abominations.  I believe this is what the Cathedral was attempting to bolster.'
          wait 3s
          msend %actor% %self.name% says, 'Cyprianum waits, surrounded by his disciples at the heart of the maze.  Destroy him and bring Balance back to the keep.'
          wait 3s
          msend %actor% %self.name% says, 'Can you do this?'
        endif
        break
      case 9
        if %actor.quest_variable[elemental_chaos:bounty]% == running
          msend %actor% %self.name% says, 'You still have a mission to accomplish.  Banish the Chaos Demon in Frost Valley.'
        else
          msend %actor% %self.name% says, 'I was wrong!  The Cathedral wasn't trying to help Cyprianum - they were summoning a terrifying Chaos Demon!'
          wait 3s
          msend %actor% %self.name% says, 'Even though you stopped their efforts, the damage was done.  They took advantage of the remnant energy from the Time Cataclysm and loosed the demon on Frost Valley.'
          wait 4s
          msend %actor% %self.name% says, 'Please, will you put a stop to the demon's rampage?'
        endif
        break
      case 10
        if %actor.quest_variable[elemental_chaos:bounty]% == running
          msend %actor% %self.name% says, 'You still have a mission to accomplish.  Slay one of the Norhamen.'
        else
          msend %actor% %self.name% says, 'Having driven the Chaos Demon back beyond the veil, I can sense at last the source of all this Chaos.'
          wait 4s
          msend %actor% %self.name% says, 'A pair of terrifying demon beasts called the Norhamen lurk deep in the remains of a fallen kingdom.  Long forgotten by time, the ruins have been warped and twisted into a hellish labyrinth of fallen angels and walking nightmares.'
          wait 4s
          msend %actor% %self.name% says, 'Help us restore Harmony.  Will you slay one of the Norhamen?'
        endif
    done
  else
    msend %actor% %self.name% says, 'Give me more time to strategize how to bring Balance to Chaos.  Come back after you've gained some more experience.'
  endif
endif
~
#5337
Elemental Chaos Hakujo speech yes~
0 d 100
yes~
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
    if !%person.quest_stage[elemental_chaos]%
      quest start elemental_chaos %person%
    endif
    if %person.has_completed[elemental_chaos]%
      msend %person% %self.name% says, 'Chaos can never be truly destroyed, but you have helped restore Balance.'
    elseif %person.quest_variable[elemental_chaos:bounty]% == dead
      msend %person% %self.name% says, 'Give me your current mission first.'
    else
      if %person.level% >= (%person.quest_stage[elemental_chaos]% - 1) * 10
        if %person.quest_variable[elemental_chaos:bounty]% != running
          msend %person% %self.name% says, 'Splendid.'
          switch %person.quest_stage[elemental_chaos]%
            case 1
              set mission 5320
              break
            case 2
              set mission 5321
              break
            case 3
              set mission 5322
              break
            case 4
              set mission 5323
              break
            case 5
              set mission 5324
              break
            case 6
              set mission 5325
              break
            case 7
              set mission 5326
              break
            case 8
              set mission 5327
              break
            case 9
              set mission 5328
              break
            case 10
              set mission 5329
              break
            default
              set mission 5320
          done
          mload obj %mission%
          give mission %person%
          msend %person% &0  
          msend %person% %self.name% says, 'When you've completed your task, bring that mission back to me.  I can give you a modest reward.  You can check your &6&b[progress]&0 at any time.'
          msend %person% &0   
          msend %person% %self.name% says, 'Walk the Way.'
          bow
          quest variable elemental_chaos %person% bounty running
        elseif %person.quest_variable[elemental_chaos:bounty]% != running
          switch %person.quest_stage[elemental_chaos]%
            case 1
              msend %person% %self.name% says, 'You still have a mission to accomplish.  Investigate the news of an imp and dispatch it if you find one.'
              break
            case 2
              msend %person% %self.name% says, 'You still have a mission to accomplish.  Silence the seductive song of the Leading Player.'
              break
            case 3
              msend %person% %self.name% says, 'You still have a mission to accomplish.  Destroy the Chaos and the cult worshipping it!'
              break
            case 4
              msend %person% %self.name% says, 'You still have a mission to accomplish.  Undertake the vision quest from the shaman in Three-Falls Canyon and defeat whatever awaits at the end.'
              break
            case 5
              msend %person% %self.name% says, 'You still have a mission to accomplish.  Dispatch the Fangs of Yeenoghu.  Be sure to destroy all of them.'
              break
            case 6
              msend %person% %self.name% says, 'You still have a mission to accomplish.  Extinguish the fire elemental lord who serves Krisenna.'
              break
            case 7
              msend %person% %self.name% says, 'You still have a mission to accomplish.  Stop the acolytes in the Cathedral of Betrayal.'
              break
            case 8
              msend %person% %self.name% says, 'You still have a mission to accomplish.  Destroy Cyprianum the Reaper in the heart of his maze.'
              break
            case 9
              msend %person% %self.name% says, 'You still have a mission to accomplish.  Banish the Chaos Demon in Frost Valley.'
              break
            case 10
              msend %person% %self.name% says, 'You still have a mission to accomplish.  Slay one of the Norhamen.'
          done
        endif
      else
        msend %person% %self.name% says, 'Give me more time to strategize how to bring Balance to Chaos.  Come back after you've gained some more experience.'
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#5338
Elemental Chaos Hakujo mission receive~
0 j 100
5320 5321 5322 5323 5324 5325 5326 5327 5328 5329~
switch %object.vnum%
  * mission targets here
  case 5320
    set stage 1
    set target1 an imp
    break
  case 5321
    set stage 2
    set target1 the Leading Player
    break
  case 5322
    set stage 3
    set target1 the Chaos
    break
  case 5323
    set stage 4
    set target1 whatever waits at the end of the shaman's vision quest
    break
  case 5324
    set stage 5
    set target1 the Fangs of Yeenoghu
    break
  case 5325
    set stage 6
    set target1 the fire elemental lord
    break
  case 5326
    set stage 7
    set target1 an acolyte of Betrayal
    break
  case 5327
    set stage 8
    set target1 for Cyprianum the Reaper
    break
  case 5328
    set stage 9
    set target1 a Chaos Demon
    break
  case 5329
    set stage 10
    set target1 the Norhamen
done
if %actor.quest_stage[elemental_chaos]% == %stage% && %actor.quest_variable[elemental_chaos:bounty]% == dead
  set anti Anti-Paladin
  wait 2
  mjunk %object%
  bow
  msend %actor% %self.name% says, 'Harmony follows your actions.'
  eval money %stage% * 10
  give %money% platinum %actor%
  if %stage% == 1
    set expcap 5
  else
    eval bonus (%stage% - 1) * 10
    set expcap %bonus%
  endif
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
  switch %person.class%
    case Warrior
    case Berserker 
        eval expmod (%expmod% + (%expmod% / 10))
        break
    case Paladin
    case %anti%
    case Ranger
        eval expmod (%expmod% + ((%expmod% * 2) / 15))
        break
    case Sorcerer
    case Pyromancer
    case Cryomancer
    case Illusionist
    case Bard
        eval expmod (%expmod% + (%expmod% / 5))
        break
    case Necromancer
    case Monk
        eval expmod (%expmod% + (%expmod% * 2) / 5)
        break
    default
        set expmod %expmod%
  done
  msend %actor% &3&bYou gain experience!&0
  eval setexp (%expmod% * 10)
  set loop 0
  while %loop% < 3
    mexp %actor% %setexp%
    eval loop %loop% + 1
  done
  quest variable elemental_chaos %actor% target1 0
  quest variable elemental_chaos %actor% bounty 0
  wait 2
  if %stage% < 10
    quest advance elemental_chaos %actor%
    msend %actor% %self.name% says, 'I will investigate further.  Return later and I shall tell you my findings.'
  else
    quest complete elemental_chaos %actor%
    msend %actor% %self.name% says, 'You have done Balance a great service.'
    wait 1s
    msend %actor% %self.name% says, 'May this ring ever remind you to ever walk the Way.'
    mload obj 12406
    give ring %actor%
  endif
  if %actor.class% /= Monk && %actor.quest_stage[monk_vision]% == 0
    wait 2s
    msend %actor% %self.name% says, 'You are ready to begin your own path to &6&b[Enlightenment]&0.'
    mload obj 390
    give vision %actor%
    wait 1s
    msend %actor% %self.name% says, 'These markings represent your voyage.'
    quest start monk_vision %actor%
    wait 2s
    if %actor.level% < 10
      msend %actor% %self.name% says, 'Come back with that vision after you reach level 10.  We can discuss your voyage toward enlightenment then.'
    endif
  endif
elseif %actor.quest_stage[elemental_chaos]% > %stage%
  return 0
  shake
  mecho %self.name% refuses the mission.
  wait 2
  msend %actor% %self.name% says, 'You already accomplished this!'
elseif %actor.quest_stage[elemental_chaos]% < %stage%
  wait 2
  eye %actor%
  msend %actor% %self.name% says, 'How'd you get this?!  You steal it off someone else??'
  mecho %self.name% rips up the mission!
  mjunk %object%
elseif %actor.quest_variable[elemental_chaos:bounty]% == running
  return 0
  mecho %self.name% refuses the mission.
  wait 2
  msend %actor% %self.name% says, 'You have to complete your mission first!  
  if %stage% == 5
    msend %actor% %target1% are still out there.'
  else
    msend %actor% %target1% is still out there.'
  endif
endif
~
#5339
Elemental Chaos mission look~
1 m 100
~
switch %self.vnum%
  case 5320
    set stage 1
    set victim1 an imp
    break
  case 5321
    set stage 2
    set victim1 the Leading Player
    break
  case 5322
    set stage 3
    set victim1 the Chaos
    break
  case 5323
    set stage 4
    set victim1 whatever waits at the end of the shaman's vision quest
    break
  case 5324
    set stage 5
    set victim1 the shaman Fang of Yeenoghu
    set victim2 the necromancer Fang of Yeenoghu
    set victim3 the diabolist Fang of Yeenoghu
    break
  case 5325
    set stage 6
    set victim1 the fire elemental lord
    break
  case 5326
    set stage 7
    set victim1 an acolyte of Betrayal
    break
  case 5327
    set stage 8
    set victim1 Cyprianum the Reaper
    break
  case 5328
    set stage 9
    set victim1 a Chaos Demon
    break
  case 5329
    set stage 10
    set victim1 the Norhamen
done
return 0
if %stage% == 5
  osend %actor% This is a mission to defeat %victim1%, %victim2%, and %victim3%.
else
  osend %actor% This is a mission to defeat %victim1%.
endif
if %actor.quest_variable[elemental_chaos:bounty]% == dead
  osend %actor% You have completed the mission.
  osend %actor% Return it to Hakujo!
elseif %stage% == 5
  if %actor.quest_variable[elemental_chaos:target1]%
    osend %actor% You have scratched %victim1% off the list.
  endif
  if %actor.quest_variable[elemental_chaos:target2]%
    osend %actor% You have scratched %victim2% off the list.
  endif
  if %actor.quest_variable[elemental_chaos:target3]%
    osend %actor% You have scratched %victim3% off the list.
  endif
endif
~
#5340
Monk Vision Hakujo receive~
0 j 100
390 59006 55582 391 18505 55591 392 8501 55623 393 12532 55655 394 16209 55665 395 43013 55678 396 53009 55710 397 58415 55722 398 58412 55741 ~
switch %object.vnum%
  case 390
    set visionstage 1
    set item quest
    break
  case 59006
    set visionstage 1
    set item book
    break
  case 55582
    set visionstage 1
    set item gem
    break
  case 391
    set visionstage 2
    set item quest
    break
  case 18505
    set visionstage 2
    set item book
    break
  case 55591
    set visionstage 2
    set item gem
    break
  case 392
    set visionstage 3
    set item quest
    break
  case 8501
    set visionstage 3
    set item book
    break
  case 55623
    set visionstage 3
    set item gem
    break
  case 393
    set visionstage 4
    set item quest
    break
  case 12532
    set visionstage 4
    set item book
    break
  case 55655
    set visionstage 4
    set item gem
    break
  case 394
    set visionstage 5
    set item quest
    break
  case 16209
    set visionstage 5
    set item book
    break
  case 55665
    set visionstage 5
    set item gem
    break
  case 395
    set visionstage 6
    set item quest
    break
  case 43013
    set visionstage 6
    set item book
    break  
  case 55678
    set visionstage 6
    set item gem
    break
  case 396
    set visionstage 7
    set item quest
    break
  case 53009
    set visionstage 7
    set item book
    break
  case 55710
    set visionstage 7
    set item gem
    break
  case 397
    set visionstage 8
    set item quest
    break
  case 58415
    set visionstage 8
    set item book
    break
  case 55722
    set visionstage 8
    set item gem
    break
  case 398
    set visionstage 9
    set item quest
    break
  case 58412
    set visionstage 9
    set item book
    break
  case 55741
    set visionstage 9
    set item gem
done
if %actor.quest_stage[elemental_chaos]% < %actor.quest_stage[monk_vision]%
  return 0
  shake
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, 'Undertake more missions in service of Balance first.'
elseif %actor.level% < (%actor.quest_stage[monk_vision]% * 10)
  return 0
  shake
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, 'You need to gain some more experience first.'
elseif %visionstage% > %actor.quest_stage[monk_vision]%
  return 0
  shake
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, 'Your vision doesn't involve that yet.  Be patient!'
elseif %visionstage% < %actor.quest_stage[monk_vision]%
  return 0
  shake
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, 'You've already seen that vision.'
else
  if %item% == quest
    set job1 %actor.quest_variable[monk_vision:visiontask1]%
    set job2 %actor.quest_variable[monk_vision:visiontask2]%
    set job3 %actor.quest_variable[monk_vision:visiontask3]%
    set job4 %actor.quest_variable[monk_vision:visiontask4]%
    if %job1% && %job2% && %job3% && %job4%
      wait 2
      eval reward %object.vnum% + 1
      mjunk %object%
      nod
      msend %actor% %self.name% says, 'Well done!  You've earned your next vision mark.'
      mload obj %reward%
      give vision %actor%
      eval expcap %visionstage% * 10
      if %expcap% < 17
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
      eval expmod %expmod% + ((%expmod% * 2) / 15)
      msend %actor% &3&bYou gain experience!&0
      eval setexp (%expmod% * 10)
      set loop 0
      while %loop% < 7
        mexp %actor% %setexp%
        eval loop %loop% + 1
      done
      set number 1
      while %number% < 5
        quest variable monk_vision %actor% visiontask%number% 0
        eval number %number% + 1
      done
      if %actor.quest_stage[monk_vision]% < 9
        quest advance monk_vision %actor%
      else
        quest complete monk_vision %actor%
      endif
      if %actor.quest_stage[monk_vision]% >= 4
        wait 2s
        if !%actor.quest_stage[monk_chants]%
          msend %actor% %self.name% says, 'Your awareness has grown enough to handle the esoteric &6&b[chants]&0.  I can give you instruction if you wish.'
        elseif (%actor.quest_stage[monk_chants]% == 7 && %actor.level% < 75) || (%actor.quest_stage[monk_chants]% > 9 && %actor.level% < 99)
            msend %actor% %self.name% says, 'You will be ready for a new chant once you have gained more experience.'
        else
          msend %actor% %self.name% says, 'Your awareness has grown enough to comprehend a new esoteric &6&b[chant]&0.'
        endif
      endif
    else
      return 0
      shake
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      msend %actor% %self.name% says, 'You need to do everything else before you trade in your vision mark!'
    endif
  elseif %item% == book || %item% == gem
    if !%actor.quest_variable[monk_vision:visiontask4]%
      return 0
      shake
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      msend %actor% %actor% %self.name% says, 'You need to bring the gem and the text to the prescribed place and read first.'
      halt
    else
      if %item% == book
        if %actor.quest_variable[monk_vision:visiontask3]% == %object.vnum%
          set accept no
        else
          set accept yes
          quest variable monk_vision %actor% visiontask3 %object.vnum%
        endif
      elseif %item% == gem
        if %actor.quest_variable[monk_vision:visiontask2]% == %object.vnum%
          set accept no
        else
          set accept yes
          quest variable monk_vision %actor% visiontask2 %object.vnum%
        endif
      endif
    endif
  endif
  if %accept% == no
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'You already gave me that.'
  elseif %accept% == yes
    wait 2
    mjunk %object%
    set job1 %actor.quest_variable[monk_vision:visiontask1]%
    set job2 %actor.quest_variable[monk_vision:visiontask2]%
    set job3 %actor.quest_variable[monk_vision:visiontask3]%
    set job4 %actor.quest_variable[monk_vision:visiontask4]%
    if %job1% && %job2% && %job3% && %job4%
      msend %actor% %self.name% says, 'Excellent.  Now trade in your current vision mark.'
    else
      msend %actor% %self.name% says, 'Good, now finish the rest.'
    endif
  endif
endif
~
#5341
Monk Vision Hakujo speech vision~
0 d 100
vision enlightenment~
set visionstage %actor.quest_stage[monk_vision]%
set missionstage %actor.quest_stage[elemental_chaos]%
set job1 %actor.quest_variable[monk_vision:visiontask1]%
set job2 %actor.quest_variable[monk_vision:visiontask2]%
set job3 %actor.quest_variable[monk_vision:visiontask3]%
set job4 %actor.quest_variable[monk_vision:visiontask4]%
wait 2
if %actor.class% != Monk
  shake
  msend %actor% %self.name% says, 'Only monks have the mental fortitude to seek Enlightenment.'
  halt
elseif %actor.level% < 10
  msend %actor% %self.name% says, 'You aren't ready to start a journey to Enlightenment yet.  Come back when you've grown a bit.'
  halt
elseif %actor.level% < (%visionstage% * 10)
  msend %actor% %self.name% says, 'You aren't ready for another vision yet.  Come back when you've gained more experience.'
  halt
elseif %actor.has_completed[monk_vision]%
  msend %actor% %self.name% says, 'You are already awakened to the Illusion of Reality!'
  halt
endif
if %visionstage% == 0
  mecho %self.name% says, 'You must undertake a &6&b[mission]&0 in service of Balance first.'
  halt
elseif (%visionstage% >= %missionstage%) && !%actor.has_completed[elemental_chaos]%
  msend %actor% %self.name% says, 'You must walk further along the Way in service of Balance first.'
  halt
elseif %job1% && %job2% && %job3% && %job4%
  msend %actor% %self.name% says, 'Just give me your current vision mark.'
  halt
endif
switch %visionstage%
  case 1
    set book 59006
    set gem 55582
    set room %get.room[4328]%
    set place %room.name%
    set hint in a place to perform.
    break
  case 2
    set book 18505
    set gem 55591
    set room %get.room[58707]%
    set place %room.name%
    set hint near a sandy beach.
    break
  case 3
    set book 8501
    set gem 55623
    set room %get.room[18597]%
    set place %room.name%
    set hint in a cloistered library.
    break
  case 4
    set book 12532
    set gem 55655
    set room %get.room[58102]%
    set place %room.name%
    set hint on my home island.
    break
  case 5
    set book 16209
    set gem 55665
    set room %get.room[16057]%
    set place %room.name%
    set hint in the ghostly fortress.
    break
  case 6
    set book 43013
    set gem 55678
    set room %get.room[59054]%
    set place %room.name%
    set hint in the fortress of the zealous.
    break
  case 7
    set book 53009
    set gem 55710
    set room %get.room[49079]%
    set place %room.name%
    set hint off-shore of the island of great beasts.
    break
  case 8
    set book 58415
    set gem 55722
    set room %get.room[11820]%
    set place %room.name%
    set hint beyond the Blue-Fog Trail.
    break
  case 9
    set book 58412
    set gem 55741
    set room %get.room[52075]%
    set place %room.name%
    set hint in the shattered citadel of Templace.
    break
done
eval attacks %visionstage% * 100
nod
if %visionstage% == 1
  msend %actor% %self.name% says, 'You will undertake a series of vision quests, gathering esoteric works of philosophy, history, and magic.  With each vision quest you'll earn a new marking to signify your path toward Enlightenment.'
  msend %actor% &0   
endif
msend %actor% %self.name% says, 'For your vision quest do the following:
msend %actor% - Attack &3&b%attacks%&0 times while wearing your current vision marking.    
msend %actor% - Find &3&b%get.obj_shortdesc[%gem%]%&0 to awaken your Third Eye charka and &3&b%get.obj_shortdesc[%book%]%&0.
msend %actor% - While donning your vision marking, take gem and text to a place of deep reflection and &3&b[read]&0 the text to broaden your awareness. 
msend %actor% &0  Go to "&3&b%place%&0".  It's %hint%
msend %actor% &0  
msend %actor% Once you have, bring the gem, text, and your vision marking back to me.
msend %actor% &0   
msend %actor% You can ask about your &6&b[progress]&0 at any time.'
~
#5342
Monk Vision command read~
1 c 3
read~
switch %cmd%
  case r
  case re
    return 0
    halt
done
set visionstage %actor.quest_stage[monk_vision]%
switch %self.vnum%
  case 59006
    set place 4328
    set stage 1
    if %actor.inventory[55582]%
      set hasgem 1
    endif
    if %actor.wearing[390]%
      set hasvision 1
    endif
    break
  case 18505
    set place 58707
    set stage 2
    if %actor.inventory[55591]%
      set hasgem 1
    endif
    if %actor.wearing[391]%
      set hasvision 1
    endif
    break
  case 8501
    set place 18597
    set stage 3
    if %actor.inventory[55623]%
      set hasgem 1
    endif
    if %actor.wearing[392]%
      set hasvision 1
    endif
    break
  case 12532
    set place 58102
    set stage 4
    if %actor.inventory[55655]%
      set hasgem 1
    endif
    if %actor.wearing[393]%
      set hasvision 1
    endif
    break
  case 16209
    set place 16057
    set stage 5
    if %actor.inventory[55665]%
      set hasgem 1
    endif
    if %actor.wearing[394]%
      set hasvision 1
    endif
    break
  case 43013
    set place 59054
    set stage 6
    if %actor.inventory[55678]%
      set hasgem 1
    endif
    if %actor.wearing[395]%
      set hasvision 1
    endif
    break
  case 53009
    set place 49079
    set stage 7
    if %actor.inventory[55710]%
      set hasgem 1
    endif
    if %actor.wearing[396]%
      set hasvision 1
    endif
    break
  case 58415
    set place 11820
    set stage 8
    if %actor.inventory[55722]%
      set hasgem 1
    endif
    if %actor.wearing[397]%
      set hasvision 1
    endif
    break
  case 58412
    set place 52075
    set stage 9
    if %actor.inventory[55741]%
      set hasgem 1
    endif
    if %actor.wearing[398]% 
      set hasvision 1
    endif
done
if %place% == %actor.room% && %visionstage% == %stage% && %hasvision% && %hasgem%
  if %actor.quest_variable[monk_vision:visiontask4]%
    osend %actor% &5You have already expanded your mind to its current limits.&0
    halt
  else
    set continue yes
  endif
endif
if %continue% == yes
  if %actor.fighting%
    osend %actor% &5You cannot properly focus while fighting!&0
    halt
  else
    osend %actor% &5You begin to read %self.shortdesc%...&0
    wait 4s
    if %actor.fighting%
      osend %actor% &5You cannot properly focus while fighting!&0
      halt
    endif
    if %actor.room% != %place%
      osend %actor% &5You must be in the proper place to keep reading!&0
      halt
    endif
    set i 0
    while %i% < 3
      osend %actor% &5You continue to read...&0
      wait 4s
      eval i %i% + 1
      if %actor.fighting%
        osend %actor% &5You cannot properly focus while fighting!&0
        halt
      endif
      if %actor.room% != %place%
        osend %actor% &5You must be in the proper place to keep reading!&0
        halt
      endif
    done
    osend %actor% &5&bYou feel your awareness broadening!&0
    quest variable monk_vision %actor% visiontask4 1
  endif
else
  return 0
endif
~
#5343
Elemental Chaos Hakujo progress checker~
0 d 100
progress~
wait 2
msend %actor% &2&bElemental Chaos&0
if %actor.has_completed[elemental_chaos]%
  msend %actor% %self.name% says, 'Chaos can never be truly destroyed, but you have helped restore Balance.'
elseif !%actor.quest_stage[elemental_chaos]%
  msend %actor% %self.name% says, 'You aren't walking the Way with me.'
elseif %actor.quest_variable[elemental_chaos:bounty]% == dead
  msend %actor% %self.name% says, 'Give me your current mission first.'
elseif %actor.level% >= (%actor.quest_stage[elemental_chaos]% - 1) * 10
  if %actor.quest_variable[elemental_chaos:bounty]% != running
    msend %actor% %self.name% says, 'You aren't doing anything for me right now.'
  else
    switch %actor.quest_stage[elemental_chaos]% 
      case 1
        msend %actor% %self.name% says, 'You still have a mission to accomplish.  Investigate the news of an imp and dispatch it if you find one.'
        break
      case 2
        msend %actor% %self.name% says, 'You still have a mission to accomplish.  Silence the seductive song of the Leading Player.'
        break
      case 3
        msend %actor% %self.name% says, 'You still have a mission to accomplish.  Destroy the Chaos and the cult worshiping it!'
        break
      case 4
        msend %actor% %self.name% says, 'You still have a mission to accomplish.  Undertake the vision quest from the shaman in Three-Falls Canyon and defeat whatever awaits at the end.'
        break
      case 5
        msend %actor% %self.name% says, 'You still have a mission to accomplish.  Dispatch the Fangs of Yeenoghu.  Be sure to destroy all of them.'
        break
      case 6
        msend %actor% %self.name% says, 'You still have a mission to accomplish.  Extinguish the fire elemental lord who serves Krisenna.'
        break
      case 7
        msend %actor% %self.name% says, 'You still have a mission to accomplish.  Stop the acolytes in the Cathedral of Betrayal.'
        break
      case 8
        msend %actor% %self.name% says, 'You still have a mission to accomplish.  Destroy Cyprianum the Reaper in the heart of his maze.'
        break
      case 9
        msend %actor% %self.name% says, 'You still have a mission to accomplish.  Banish the Chaos Demon in Frost Valley.'
        break
      case 10
        msend %actor% %self.name% says, 'You still have a mission to accomplish.  Slay one of the Norhamen.'
    done
  endif
else
  msend %actor% %self.name% says, 'Give me more time to strategize how to bring Balance to Chaos.  Come back after you've gained some more experience.'
endif
if %actor.class% /= Monk
  msend %actor% &0  
  msend %actor% &0  
  msend %actor% &2&bEnlightenment&0
  set missionstage %actor.quest_stage[elemental_chaos]%
  set visionstage %actor.quest_stage[monk_vision]%
  set job1 %actor.quest_variable[monk_vision:visiontask1]%
  set job2 %actor.quest_variable[monk_vision:visiontask2]%
  set job3 %actor.quest_variable[monk_vision:visiontask3]%
  set job4 %actor.quest_variable[monk_vision:visiontask4]%
  if %actor.level% < 10
    msend %actor% %self.name% says, 'You aren't ready for a vision yet.  Come back when you've grown a bit.'
  elseif %actor.has_completed[monk_vision]%
    msend %actor% %self.name% says, 'You are already awakened to the Illusion of Reality!'
  elseif %actor.level% < (%visionstage% * 10)
    msend %actor% %self.name% says, 'You aren't ready to start a journey to Enlightenment yet.  Come back when you've grown a bit.'
  elseif %visionstage% == 0
    msend %actor% %self.name% says, 'You must undertake a &6&b[mission]&0 in service of Balance first.'
  elseif (%visionstage% >= %missionstage%) && !%actor.has_completed[elemental_chaos]%
    msend %actor% %self.name% says, 'You must walk further along the Way in service of Balance first.'
  else
    switch %visionstage%
      case 1
        set book 59006
        set gem 55582
        set room %get.room[4328]%
        set place %room.name%
        set hint in a place to perform.
        break
      case 2
        set book 18505
        set gem 55591
        set room %get.room[58707]%
        set place %room.name%
        set hint near a sandy beach.
        break
      case 3
        set book 8501
        set gem 55623
        set room %get.room[18597]%
        set place %room.name%
        set hint in a cloistered library.
        break
      case 4
        set book 12532
        set gem 55655
        set room %get.room[58102]%
        set place %room.name%
        set hint on my home island.
        break
      case 5
        set book 16209
        set gem 55665
        set room %get.room[16057]%
        set place %room.name%
        set hint in the ghostly fortress.
        break
      case 6
        set book 43013
        set gem 55678
        set room %get.room[59054]%
        set place %room.name%
        set hint in the fortress of the zealous.
        break
      case 7
        set book 53009
        set gem 55710
        set room %get.room[49079]%
        set place %room.name%
        set hint off-shore of the island of great beasts.
        break
      case 8
        set book 58415
        set gem 55722
        set room %get.room[11820]%
        set place %room.name%
        set hint beyond the Blue-Fog Trail.
        break
      case 9
        set book 58412
        set gem 55741
        set room %get.room[52075]%
        set place %room.name%
        set hint in the shattered citadel of Templace.
    done
    eval attack %visionstage% * 100
    if %job1% || %job2% || %job3% || %job4% 
      msend %actor% You've done the following:
      if %job1%
        msend %actor% - attacked %attack% times
      endif
      if %job2%
        msend %actor% - found %get.obj_shortdesc[%gem%]%
      endif
      if %job3%
        msend %actor% - found %get.obj_shortdesc[%book%]%
      endif
      if %job4%
        msend %actor% - read in %place%
      endif
      msend %actor% &0  
    endif
    msend %actor% You need to:
    if %job1% && %job2% && %job3% && %job4%
      msend %actor% Just give me your current vision mark.
    else
      if !%job1%
        eval remaining %attack% - %actor.quest_variable[monk_vision:attack_counter]%
        msend %actor% - attack &9&b%remaining%&0 more times while wearing your vision mark.
      endif
      if !%job4%
        msend %actor% - take &3&b%get.obj_shortdesc[%gem%]%&0 and &3&b%get.obj_shortdesc[%book%]%&0 and &3&bread&0 in a place called "&3&b%place%&0".
        msend %actor%&0   It's &2&b%hint%&0
      else
        if !%job2%
          msend %actor% - give me &3&b%get.obj_shortdesc[%gem%]%&0
        endif
        if !%job3%
          msend %actor% - give me &3&b%get.obj_shortdesc[%book%]%&0
        endif
      endif
    endif
  endif
  if %actor.quest_stage[monk_chants]%
    set chantstage %actor.quest_stage[monk_chants]%
    switch %chantstage%
      case 1
        set chant Tremors of Saint Augustine
        set level 30
        set item a book surrounded by trees and shadows
        set place in a place that is both natural and urban, serenely peaceful and profoundly sorrowful
        break
      case 2
        set chant Tempest of Saint Augustine
        set level 40
        set item a scroll, dedicated to this particular chant, guarded by a creature of the same elemental affinity
        set place at the marker on a southern mountain range.
        break
      case 3
        set chant Blizzards of Saint Augustine
        set level 50
        set item a book held by a master who in turn is a servant of a beast of winter
        set place in a temple shrouded in mists
        break
      case 4
        set chant Aria of Dissonance
        set level 60
        set item a book on war, held by a banished war god
        set place in a dark cave before a blasphemous book, near an unholy fire
        break
      case 5
        set chant Apocalyptic Anthem
        set level 75
        set item a scroll where illusion is inscribed over and over held by a brother who thirsts for escape
        set place in a chapel of the walking dead
        break
      case 6
        set chant Fires of Saint Augustine
        set level 80
        set item a scroll of curses, carried by children of air in a floating fortress
        set place at an altar dedicated to fire's destructive forces
        break
      case 7
        set chant Seed of Destruction
        set level 99
        set item the eye of one caught in an eternal feud
        set place at an altar deep in the outer realms surrounded by those who's vengeance was never satisfied
    done
    msend %actor% &0  
    msend %actor% &0  
    msend %actor% &2&bEsoteric Chants&0
    if %actor.level% >= %level% 
      if %chantstage% < (%visionstage% - 2)
        msend %actor% %self.name% says, 'You are seeking the chant %chant%.'
        msend %actor% &0
        msend %actor% %self.name% says, 'You are looking for %item%.
        msend %actor% Take it and &6&b[meditate]&0 %place%.'
      else
        msend %actor% %self.name% says, 'You must first take another step towards &6&b[enlightenment]&0 before you can grasp more esoteric knowledge.'
      endif
    else
      msend %actor% %self.name% says, 'You aren't ready to learn the next chant yet.'
    endif
  endif
endif
~
#5344
Hakujo refuse~
0 j 100
~
set treasurestage %actor.quest_stage[elemental_chaos]%
set visionstage %actor.quest_stage[monk_vision]%
switch %object.vnum%
  case 390
  case 59006
  case 55582
  case 391
  case 18505
  case 55591
  case 392
  case 8501
  case 55623
  case 393
  case 12532
  case 55655
  case 394
  case 16209
  case 55665
  case 395
  case 43013
  case 55678
  case 396
  case 53009
  case 55710
  case 397
  case 58415
  case 55722
  case 398
  case 58412
  case 55741 
  case 5320
  case 5321
  case 5322
  case 5323
  case 5324
  case 5325
  case 5326
  case 5327
  case 5328
  case 5329
    halt
    break
  default
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'What is this for?'
done
~
#5345
Elemental Chaos Hakujo new mission~
0 d 0
I need a new note~
wait 2
if %actor.level% >= (%actor.quest_stage[elemental_chaos]% - 1) * 10
  if %actor.quest_variable[elemental_chaos:bounty]%
    switch %actor.quest_stage[elemental_chaos]%
      case 1
        set mission 5320
        break
      case 2
        set mission 5321
        break
      case 3
        set mission 5322
        break
      case 4
        set mission 5323
        break
      case 5
        set mission 5324
        break
      case 6
        set mission 5325
        break
      case 7
        set mission 5326
        break
      case 8
        set mission 5327
        break
      case 9
        set mission 5328
        break
      case 10
        set mission 5329
        break
    done
    grumble
    mload obj %mission%
    give mission %actor%
    msend %actor% %self.name% says, 'Be more careful next time.'
  endif
endif
~
#5346
Elemental Chaos target death~
0 f 100
~
switch %self.vnum%
  case 58808
  * imp
    set stage 1
    set target1 imp
    break
  case 4399
  * Leading Player
    set target1 Leading_player
    set stage 2
    break
  case 17306
  * the Chaos
    set target1 Chaos
    set stage 3
    break
  case 17811
  * the Fetch
    set target1 Fetch
    set stage 4
    break
  case 46206
  * shaman Fang 
    set target1 shaman_Fang
    set stage 5
    break
  case 46207
  * necro Fang
    set target2 necro_Fang
    set stage 5
    break
  case 46208
  * dia Fang
    set target3 dia_Fang
    set stage 5
    break
  case 12523
  * fire lord
    set target1 fire_lord
    set stage 6
    break
  case 8509
  * acolyte
    set target1 acolyte
    set stage 7
    break
  case 43017
  * Cyprianum the Reaper
    set target1 Cyprianum
    set stage 8
    break
  case 53417
  * chaos demon
    set target1 Chaos_Demon
    set stage 9
    break
  case 4009
  * Norhamen
    set target1 Norhamen
    set stage 10
done
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
    if %person.quest_stage[elemental_chaos]% == %stage% && %person.quest_variable[elemental_chaos:bounty]% == running
      if %target1%
        quest variable elemental_chaos %person% target1 %target1%
        msend %person% &1&bYou cross %self.name% off your list.&0
      elseif %target2%
        quest variable elemental_chaos %person% target2 %target2%
        msend %person% &1&bYou cross %self.name% off your list.&0
      elseif %target3%
        quest variable elemental_chaos %person% target3 %target3%
        msend %person% &1&bYou cross %self.name% off your list.&0
      endif
      if %stage% == 5
        if %person.quest_variable[elemental_chaos:target1]% && %person.quest_variable[elemental_chaos:target2]% && %person.quest_variable[elemental_chaos:target3]%
          quest variable elemental_chaos %person% bounty dead
        endif
      else
        quest variable elemental_chaos %person% bounty dead
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#5347
Monk Chants command meditate~
1 c 3
meditate~
switch %cmd%
  case m
  case me
    return 0
    halt
done
switch %self.vnum%
  case 1807
    set visionstage 3
    set chantstage 1
    set level 30
    set place 3205
    if %actor.wearing[393]% || %actor.wearing[394]% || %actor.wearing[395]% || %actor.wearing[396]% || %actor.wearing[397]% || %actor.wearing[398]% || %actor.wearing[399]%
      set mark yes
    endif
    set text1 &3The power of the earth begins to flow through your fists!&0
    set text2 &3You have learned Tremors of Saint Augustine.&0
    set chant tremors of saint augustine
    break
  case 12508
    set visionstage 4
    set chantstage 2
    set level 40
    set place 4752
    if %actor.wearing[394]% || %actor.wearing[395]% || %actor.wearing[396]% || %actor.wearing[397]% || %actor.wearing[398]% || %actor.wearing[399]%
      set mark yes
    endif
    set text1 &4&bThe power of the storm begins to flow through your fists!&0
    set text2 &4&bYou have learned Tempest of Saint Augustine.&0
    set chant tempest of saint augustine
    break
  case 55003
    set visionstage 5
    set chantstage 3
    set level 50
    set place 11837
    if %actor.wearing[395]% || %actor.wearing[396]% || %actor.wearing[397]% || %actor.wearing[398]% || %actor.wearing[399]%
      set mark yes
    endif
    set text1 &6&bThe power of ice begins to flow through your fists!&0
    set text2 &6&bYou have learned Blizzards of Saint Augustine.&0
    set chant blizzards of saint augustine
    break
  case 55004
    set visionstage 6
    set chantstage 4
    set level 60
    set place 2786
    if %actor.wearing[396]% || %actor.wearing[397]% || %actor.wearing[398]% || %actor.wearing[399]%
      set mark yes
    endif
    set text1 &bReverberating from the walls are the discordant words of power!&0
    set text2 &bYou have learned Aria of Dissonance.&0
    set chant aria of dissonance
    break
  case 49066
    set visionstage 7
    set chantstage 5
    set level 75
    set place 16057
    if %actor.wearing[397]% || %actor.wearing[398]% || %actor.wearing[399]%
      set mark yes
    endif
    set text1 &5The roaring of the dead forms into a chant!&0
    set text2 &5You have learned Apocalyptic Anthem.&0
    set chant apocalyptic anthem
    break
  case 23826
    set visionstage 8
    set chantstage 6
    set level 80
    set place 5272
    if %actor.wearing[398]% || %actor.wearing[399]%
      set mark yes
    endif
    set text1 &1&bThe power of fire begins to flow through your fists!&0
    set text2 &1&bYou have learned Fires of Saint Augustine.&0
    set chant fires of saint augustine
    break
  case 48037
    set visionstage 9
    set chantstage 7
    set level 99
    set place 48913
    if %actor.wearing[399]%
      set mark yes
    endif
    set text1 &9&bThe chant to engender catastrophe echoes from the altar!&0
    set text2 &9&bYou have learned Seed of Destruction.&0
    set chant seed of destruction
done
if %actor.quest_stage[monk_vision]% > %visionstage% && %actor.quest_stage[monk_chants]% == %chantstage% && %actor.level% >= %level% && %actor.room% == %place%
  if %mark%
    if %actor.fighting%
      osend %actor% &5You cannot properly focus while fighting!&0
      halt
    elseif %actor.position% != sitting
      osend %actor% &5You cannot properly focus while %actor.position%!&0
      halt
    else
      osend %actor% &5You begin to meditate...&0
      wait 4s
      if %actor.room% != %place%
        osend %actor% &5You must be in the proper place to keep meditating!&0
        halt
      elseif %actor.fighting%
        osend %actor% &5You cannot properly focus while fighting!&0
        halt
      elseif %actor.position% != sitting
        osend %actor% &5You cannot properly focus while %actor.position%!&0
        halt
      endif
      set i 0
      while %i% < 3
        osend %actor% &5You continue to meditate...&0
        wait 4s
        eval i %i% + 1
        if %actor.room% != %place%
          osend %actor% &5You must be in the proper place to keep reading!&0
          halt
        elseif %actor.fighting%
          osend %actor% &5You cannot properly focus while fighting!&0
          halt
        elseif %actor.position% != sitting
          osend %actor% &5You cannot properly focus while %actor.position%!&0
          halt
        endif
      done
      osend %actor% %text1%
      osend %actor% %text2%
      oskillset %actor% %chant%
      if %actor.quest_stage[monk_chants]% < 7
        quest advance monk_chants %actor%
      else
        quest complete monk_chants %actor%
      endif
      wait 2
      osend %actor% %self.shortdesc% crumbles to dust and blows away...
      opurge %self%
    endif
  else
    return 0
  endif
else
  return 0
endif
~
#5348
**UNUSED**~
1 c 3
meditate~
* For Tempest of Saint Augustine on item 12508
switch %cmd%
  case m
  case me
    return 0
    halt
done
if %actor.quest_stage[monk_vision]% > 4 && %actor.quest_stage[monk_chants]% == 2 && %actor.level% >= 40 && %actor.room% == 58186
  if %actor.wearing[394]% || %actor.wearing[395]% || %actor.wearing[396]% || %actor.wearing[397]% || %actor.wearing[398]% || %actor.wearing[399]%
    if %actor.fighting%
      osend %actor% &5You cannot properly focus while fighting!&0
      halt
    elseif %actor.position% != sitting
      osend %actor% &5You cannot properly focus while %actor.position%!&0
      halt
    else
      osend %actor% &5You begin to meditate...&0
      wait 4s
      if %actor.room% != %place%
        osend %actor% &5You must be in the proper place to keep meditating!&0
        halt
      elseif %actor.fighting%
        osend %actor% &5You cannot properly focus while fighting!&0
        halt
      elseif %actor.position% != sitting
        osend %actor% &5You cannot properly focus while %actor.position%!&0
        halt
      endif
      set i 0
      while %i% < 3
        osend %actor% &5You continue to meditate...&0
        wait 4s
        eval i %i% + 1
        if %actor.room% != %place%
          osend %actor% &5You must be in the proper place to keep reading!&0
          halt
        elseif %actor.fighting%
          osend %actor% &5You cannot properly focus while fighting!&0
          halt
        elseif %actor.position% != sitting
          osend %actor% &5You cannot properly focus while %actor.position%!&0
          halt
        endif
      done
      osend %actor% &3&bThe power of the storm begins to flow through your fists!&0
      osend %actor% &3&bYou have learned Tempest of Saint Augustine.&0
      oskillset %actor% tempest of saint augustine
      quest advance monk_chants %actor%
      wait 2s
      osend %actor% %self.shortdesc% crumbles to dust and blows away...
      opurge %self%
    endif
  else
    return 0
  endif
else
  return 0
endif
~
#5349
**UNUSED**~
1 c 3
meditate~
* For Blizzards of Saint Augustine on item 55003
switch %cmd%
  case m
  case me
    return 0
    halt
done
if %actor.quest_stage[monk_vision]% > 5 && %actor.quest_stage[monk_chants]% == 3 && %actor.level% >= 50 && %actor.room% == 11837
  if %actor.wearing[395]% || %actor.wearing[396]% || %actor.wearing[397]% || %actor.wearing[398]% || %actor.wearing[399]%
    if %actor.fighting%
      osend %actor% &5You cannot properly focus while fighting!&0
      halt
    elseif %actor.position% != sitting
      osend %actor% &5You cannot properly focus while %actor.position%!&0
      halt
    else
      osend %actor% &5You begin to meditate...&0
      wait 4s
      if %actor.room% != %place%
        osend %actor% &5You must be in the proper place to keep meditating!&0
        halt
      elseif %actor.fighting%
        osend %actor% &5You cannot properly focus while fighting!&0
        halt
      elseif %actor.position% != sitting
        osend %actor% &5You cannot properly focus while %actor.position%!&0
        halt
      endif
      set i 0
      while %i% < 3
        osend %actor% &5You continue to meditate...&0
        wait 4s
        eval i %i% + 1
        if %actor.room% != %place%
          osend %actor% &5You must be in the proper place to keep reading!&0
          halt
        elseif %actor.fighting%
          osend %actor% &5You cannot properly focus while fighting!&0
          halt
        elseif %actor.position% != sitting
          osend %actor% &5You cannot properly focus while %actor.position%!&0
          halt
        endif
      done
      osend %actor% &6&bThe power of ice begins to flow through your fists!&0
      osend %actor% &6&bYou have learned Blizzards of Saint Augustine.&0
      oskillset %actor% blizzards of saint augustine
      quest advance monk_chants %actor%
      wait 2s
      osend %actor% %self.shortdesc% crumbles to dust and blows away...
      opurge %self%      
    endif
  else
    return 0
  endif
else
  return 0
endif
~
#5350
**UNUSED**~
1 c 3
meditate~
* For Aria of Dissonance on item 55004
switch %cmd%
  case m
  case me
    return 0
    halt
done
if %actor.quest_stage[monk_vision]% > 6 && %actor.quest_stage[monk_chants]% == 4 && %actor.level% >= 60 && %actor.room% == 2786
  if %actor.wearing[396]% || %actor.wearing[397]% || %actor.wearing[398]% || %actor.wearing[399]%
    if %actor.fighting%
      osend %actor% &5You cannot properly focus while fighting!&0
      halt
    elseif %actor.position% != sitting
      osend %actor% &5You cannot properly focus while %actor.position%!&0
      halt
    else
      osend %actor% &5You begin to meditate...&0
      wait 4s
      if %actor.room% != %place%
        osend %actor% &5You must be in the proper place to keep meditating!&0
        halt
      elseif %actor.fighting%
        osend %actor% &5You cannot properly focus while fighting!&0
        halt
      elseif %actor.position% != sitting
        osend %actor% &5You cannot properly focus while %actor.position%!&0
        halt
      endif
      set i 0
      while %i% < 3
        osend %actor% &5You continue to meditate...&0
        wait 4s
        eval i %i% + 1
        if %actor.room% != %place%
          osend %actor% &5You must be in the proper place to keep reading!&0
          halt
        elseif %actor.fighting%
          osend %actor% &5You cannot properly focus while fighting!&0
          halt
        elseif %actor.position% != sitting
          osend %actor% &5You cannot properly focus while %actor.position%!&0
          halt
        endif
      done
      osend %actor% &bReverberating from the walls are the discordant words of power!&0
      osend %actor% &bYou have learned Aria of Dissonance.&0
      oskillset %actor% aria of dissonance
      quest advance monk_chants %actor%
      wait 2s
      osend %actor% %self.shortdesc% crumbles to dust and blows away...
      opurge %self%      
    endif
  else
    return 0
  endif
else
  return 0
endif
~
#5351
**UNUSED**~
1 c 3
meditate~
* For Apocalyptic Anthem on item 49066
switch %cmd%
  case m
  case me
    return 0
    halt
done
if %actor.quest_stage[monk_vision]% > 7 && %actor.quest_stage[monk_chants]% == 5 && %actor.level% >= 75 && %actor.room% == 16072
  if %actor.wearing[397]% || %actor.wearing[398]% || %actor.wearing[399]%
    if %actor.fighting%
      osend %actor% &5You cannot properly focus while fighting!&0
      halt
    elseif %actor.position% != sitting
      osend %actor% &5You cannot properly focus while %actor.position%!&0
      halt
    else
      osend %actor% &5You begin to meditate...&0
      wait 4s
      if %actor.room% != %place%
        osend %actor% &5You must be in the proper place to keep meditating!&0
        halt
      elseif %actor.fighting%
        osend %actor% &5You cannot properly focus while fighting!&0
        halt
      elseif %actor.position% != sitting
        osend %actor% &5You cannot properly focus while %actor.position%!&0
        halt
      endif
      set i 0
      while %i% < 3
        osend %actor% &5You continue to meditate...&0
        wait 4s
        eval i %i% + 1
        if %actor.room% != %place%
          osend %actor% &5You must be in the proper place to keep reading!&0
          halt
        elseif %actor.fighting%
          osend %actor% &5You cannot properly focus while fighting!&0
          halt
        elseif %actor.position% != sitting
          osend %actor% &5You cannot properly focus while %actor.position%!&0
          halt
        endif
      done
      osend %actor% &5The roaring of the dead forms into a chant!&0
      osent %actor% &5You have learned Apocalyptic Anthem.&0
      oskillset %actor% Apocalyptic anthem
      quest advance monk_chants %actor%
      wait 2s
      osend %actor% %self.shortdesc% crumbles to dust and blows away...
      opurge %self%      
    endif
  else
    return 0
  endif
else
  return 0
endif
~
#5352
**UNUSED**~
1 c 3
meditate~
* For Fires of Saint Augustine on item 23826
switch %cmd%
  case m
  case me
    return 0
    halt
done
if %actor.quest_stage[monk_vision]% > 8 && %actor.quest_stage[monk_chants]% == 6 && %actor.level% >= 80 && %actor.room% == 5272
  if %actor.wearing[398]% || %actor.wearing[399]%
    if %actor.fighting%
      osend %actor% &5You cannot properly focus while fighting!&0
      halt
    elseif %actor.position% != sitting
      osend %actor% &5You cannot properly focus while %actor.position%!&0
      halt
    else
      osend %actor% &5You begin to meditate...&0
      wait 4s
      if %actor.room% != %place%
        osend %actor% &5You must be in the proper place to keep meditating!&0
        halt
      elseif %actor.fighting%
        osend %actor% &5You cannot properly focus while fighting!&0
        halt
      elseif %actor.position% != sitting
        osend %actor% &5You cannot properly focus while %actor.position%!&0
        halt
      endif
      set i 0
      while %i% < 3
        osend %actor% &5You continue to meditate...&0
        wait 4s
        eval i %i% + 1
        if %actor.room% != %place%
          osend %actor% &5You must be in the proper place to keep reading!&0
          halt
        elseif %actor.fighting%
          osend %actor% &5You cannot properly focus while fighting!&0
          halt
        elseif %actor.position% != sitting
          osend %actor% &5You cannot properly focus while %actor.position%!&0
          halt
        endif
      done
      osend %actor% &1&bThe power of fire begins to flow through your fists!&0
      osent %actor% &1&bYou have learned Fires of Saint Augustine.&0
      oskillset %actor% fires of saint augustine
      quest advance monk_chants %actor%
      wait 2s
      osend %actor% %self.shortdesc% crumbles to dust and blows away...
      opurge %self%      
    endif
  else
    return 0
  endif
else
  return 0
endif
~
#5353
**UNUSED**~
1 c 3
meditate~
* For Seed of Destruction on item 48037
switch %cmd%
  case m
  case me
    return 0
    halt
done
if %actor.quest_stage[monk_vision]% >= 9 && %actor.quest_stage[monk_chants]% == 7 && %actor.level% == 99 && %actor.room% == 48913
  if %actor.wearing[399]%
    if %actor.fighting%
      osend %actor% &5You cannot properly focus while fighting!&0
      halt
    elseif %actor.position% != sitting
      osend %actor% &5You cannot properly focus while %actor.position%!&0
      halt
    else
      osend %actor% &5You begin to meditate...&0
      wait 4s
      if %actor.room% != %place%
        osend %actor% &5You must be in the proper place to keep meditating!&0
        halt
      elseif %actor.fighting%
        osend %actor% &5You cannot properly focus while fighting!&0
        halt
      elseif %actor.position% != sitting
        osend %actor% &5You cannot properly focus while %actor.position%!&0
        halt
      endif
      set i 0
      while %i% < 3
        osend %actor% &5You continue to meditate...&0
        wait 4s
        eval i %i% + 1
        if %actor.room% != %place%
          osend %actor% &5You must be in the proper place to keep reading!&0
          halt
        elseif %actor.fighting%
          osend %actor% &5You cannot properly focus while fighting!&0
          halt
        elseif %actor.position% != sitting
          osend %actor% &5You cannot properly focus while %actor.position%!&0
          halt
        endif
      done
      osend %actor% &9&bThe chant to engender catastrophe echoes from the altar!&0
      osent %actor% &9&bYou have learned Seed of Destruction.&0
      oskillset %actor% seed of destruction
      quest complete monk_chants %actor%
      wait 2s
      osend %actor% %self.shortdesc% crumbles to dust and blows away...
      opurge %self%
    endif
  else
    return 0
  endif
else
  return 0
endif
~
#5354
Monk Chants Hakujo Speech chants~
0 d 100
chant chants~
wait 2
if %actor.quest_stage[monk_vision]% >= 3
  if !%actor.quest_stage[monk_chants]%
    msend %actor% %self.name% says, 'As you walk your path to enlightenment, your mind's capacity for esoteric knowledge will vastly increase.  This knowledge forms the basis for many powerful chants and hymns.'
    wait 2s
    msend %actor% %self.name% says, 'Each new vision marking represents your readiness for a new chant.  Much like the vision quests you undertake, you will need to find a source of knowledge and meditate on it in a place of great resonance.'
    quest start monk_chants %actor%
    wait 2s
  endif
  if %actor.quest_stage[monk_chants]% == 1
    if %actor.level% >= 30
      msend %actor% %self.name% says, 'The first esoteric chant you can learn is the &3&bTremors of Saint Augustine&0.  This prayer will channel the forces of the earth through your hands, allowing you to disintegrate your foes with your touch.'
      wait 4s
      msend %actor% %self.name% says, 'The secrets of this chant are contained in &3&ba book surrounded by trees and shadows&0.  To understand the deeper truth, you must find this book and &6&b[meditate]&0 in &3&ba place that is both natural and urban, serenely peaceful and profoundly sorrowful&0.'
      wait 4s
      msend %actor% %self.name% says, 'I cannot be more specific than this.  You must prove your capabilities and solve the puzzle on your own.  Do so and you will gain the knowledge of the Tremors of Saint Augustine.'
      wait 2s
      bow %actor%
    else
      set decline level
    endif
  elseif %actor.quest_stage[monk_chants]% == 2
    if %actor.quest_stage[monk_vision]% > 4
      if %actor.level% >= 40
        msend %actor% %self.name% says, 'Waiting for you next is the &3&bTempest of Saint Augustine&0.  This prayer will channel the electrical might of the storm through your fists.'
        wait 4s
        msend %actor% %self.name% says, 'There is &3&ba scroll dedicated to this particular chant, guarded by a creature of the same elemental affinity&0.  &6&b[Meditate]&0 on it at &3&bthe peak of Urchet Pass&0 and you will gain the knowledge of the chant.'
        wait 2s
        bow %actor%
      else
        set decline level
      endif
    else
      set decline vision
    endif
  elseif %actor.quest_stage[monk_chants]% == 3
    if %actor.quest_stage[monk_vision]% > 5
      if %actor.level% >= 50
        msend %actor% %self.name% says, 'You are ready for a third prayer to Saint Augustine.  This prayer will call the howling force of the blizzard, freezing anything you touch.'
        wait 4s
        msend %actor% %self.name% says, 'Knowledge of ice and snow is kept in &3&ba book held by a master who in turn is a servant of a beast of winter&0.  The secrets of the book can be revealed through &6&b[meditating]&0 in &3&ba temple shrouded in mists&0.'
        wait 4s
        msend %actor% %self.name% says, 'Complete this meditation and the &3&bBlizzards of Saint Augustine&0 will be revealed to you.'
        wait 2s
        bow %actor%
      else
        set decline level
      endif
    else
      set decline vision
    endif
  elseif %actor.quest_stage[monk_chants]% == 4
    if %actor.quest_stage[monk_vision]% > 6
      if %actor.level% >= 60
        msend %actor% %self.name% says, 'Words are powerful weapons.  Properly intoned, words can be as devastating as any sword.  The &3&bAria of Dissonance&0 is a mystical weapon of war that monks can unleash to descimate their opponent's defenses.'
        wait 4s
        msend %actor% %self.name% says, 'As a tool of war, the secrets are deeply encoded in &3&ba book on war, held by a banished war god&0.  They can be understood only by &6&b[meditating]&0 on them in &3&ba dark cave before a blasphemous book, near an unholy fire&0.'
        wait 2s
        bow %actor%
      else
        set decline level
      endif
    else
      set decline vision
    endif
  elseif %actor.quest_stage[monk_chants]% == 5
    if %actor.quest_stage[monk_vision]% > 7
      if %actor.level% >= 75
        msend %actor% %self.name% says, 'To augment the growth of the mind, it is important to understand the self is but a multifaceted illusion.  Understanding that will allow you to manipulate that illusion to provoke others to attacking you.'
        wait 4s
        msend %actor% %self.name% says, 'There is &3&ba scroll where this revelation is inscribed over and over held by a brother who thirsts for escape&0.  Give him what he most desires to earn the scroll.'
        wait 4s
        msend %actor% %self.name% says, 'With the scroll in hand, &6&b[meditate]&0 on it in &3&ba chapel of the walking dead&0 to hear the music that lays beyond the end of all things, the &3&bApocalyptic Anthem&0.'
        wait 2s
        bow %actor%
      else
        set decline level
      endif
    else
      set decline vision
    endif
  elseif %actor.quest_stage[monk_chants]% == 6
    if %actor.quest_stage[monk_vision]% > 8
      if %actor.level% >= 80
        msend %actor% %self.name% says, 'You are ready for the final prayer to Saint Augustine.  This prayer will allow you to hold fire in your hands, making you a living inferno.'
        wait 4s
        msend %actor% %self.name% says, 'Fire is both a blessing and a curse.  It provides heat vital to life and can incinerate life just as easily.  This duality is reflected in &3&bscrolls of curses, carried by children of air in a floating fortress&0.'
        wait 4s
        msend %actor% %self.name% says, 'Take one of these scrolls to &3&ban altar dedicated to fire's destructive forces&0 and &6&b[meditate]&0 to unleash the power of the &3&bFires of Saint Augustine&0.'
        wait 2s
        bow %actor%
      else
        set decline level
      endif
    else
      set decline vision
    endif
  elseif %actor.quest_stage[monk_chants]% == 7
    if %actor.quest_stage[monk_vision]% > 9
      if %actor.level% >= 99
        msend %actor% %self.name% says, 'The final esoteric chant is the ability to plant the seed of doubt in an opponent's very core, shattering the false reality that is corporeal form.'
        wait 4s
        msend %actor% %self.name% says, 'The chant is the ultimate weaponization of revenge.  An eye for an eye, as the saying goes, is the ultimately symbol of revenge.  Cut out the &3&beye of one caught in an eternal feud&0.  Bring it to &3&ban altar deep in the outer realms surrounded by those who's vengeance was never satisfied&0.'
        wait 4s
        msend %actor% %self.name% says, '&6&b[meditate]&0 on the illusory satisfaction of what revenge makes one think is certain to understand the doubt that is the &3&bSeed of Destruction&0.'
        wait 2s
        bow %actor%
      else
        msend %actor% %self.name% says, 'Unfortunately you must reach the pinnacle of mortal experience before you are ready to grasp this final chant.'
        halt
      endif
    else
      set decline vision
    endif
  endif
  if %decline% == level
    msend %actor% %self.name% says, 'You must gain more practical experience before you can take on more esoteric experience.'
  elseif %decline% == vision
    msend %actor% %self.name% says, 'You must first take another step towards &6&b[enlightenment]&0 before you can grasp more esoteric knowledge.'
  endif
endif
~
#5355
Hell Trident receive~
0 j 100
55662 2334 55739 2339~
set hellstage %actor.quest_stage[hell_trident]%
switch %self.vnum%
  * Black Priestess p2
  case 6032
    set reward 2339
    set phase 1
    set level 65
    set spell1 %actor.has_completed[banish]%
    set spell2 %actor.has_completed[hellfire_brimstone]%
    if !%actor.quest_variable[hell_trident:helltask6]%
      if %actor.quest_stage[vilekka_stew]% > 3
        quest variable hell_trident %actor% helltask6 1
      endif
    endif
    if %object.vnum% == 55662
      set go gem
    elseif %object.vnum% == 2334
      set go trident
    endif
    break
  case 12526
    set reward 2340
    set phase 2
    set level 90
    set spell1 %actor.has_completed[resurrection_quest]%
    set spell2 %actor.has_completed[hell_gate]%
    if %object.vnum% == 55739
      set go gem
    elseif %object.vnum% == 2339
      set go trident
    endif
done
* adding in case it's not caught somewhere else
if %hellstage% == %phase%
  if !%actor.quest_variable[hell_trident:helltask5]%
    if %spell1%
      quest variable hell_trident %actor% helltask5 1
    endif
  endif
  if !%actor.quest_variable[hell_trident:helltask4]%
    if %spell2%
      quest variable hell_trident %actor% helltask4 1
    endif
  endif
endif
if %actor.has_completed[hell_trident]%
  unset go
  set refuse 1
  set reason You already command the greatest power imaginable!
elseif %actor.level% < %level%
  unset go
  set refuse 1
  set reason You are not yet strong enough to handle more power.
elseif %hellstage% < %phase%
  unset go
  set refuse 1
  set reason Your trident is not ready to upgrade yet.
elseif %hellstage% > %phase%
  unset go
  set refuse 1
  set reason I've already done everything I can to help.
endif
if %go% == gem
  set gem_vnum %object.vnum%
  set gem_count %actor.quest_variable[hell_trident:gems]%
  if %gem_count% < 6
    wait 2
    mjunk %object.name%
    eval gem_count %gem_count% + 1
    quest variable hell_trident %actor.name% gems %gem_count%
    msend %actor% %self.name% says, 'Yes, this is perfect.'
    wait 2
    if %gem_count% == 1
      msend %actor% %self.name% says, 'You have given me 1 of 6 %get.obj_pldesc[%gem_vnum%]%.'
    else
      msend %actor% %self.name% says, 'You have given me %gem_count% of 6 %get.obj_pldesc[%gem_vnum%]%.'
    endif
    wait 2
    if %gem_count% >= 6
      quest variable hell_trident %actor% helltask3 1
      set job1 %actor.quest_variable[hell_trident:helltask1]%
      set job2 %actor.quest_variable[hell_trident:helltask2]%
      set job3 %actor.quest_variable[hell_trident:helltask3]%
      set job4 %actor.quest_variable[hell_trident:helltask4]%
      set job5 %actor.quest_variable[hell_trident:helltask5]%
      set job6 %actor.quest_variable[hell_trident:helltask6]%
      if %job1% && %job2% && %job3% && %job4% && %job5% && %job6%
        msend %actor% %self.name% says, 'Now present the trident.'
      else
        msend %actor% %self.name% says, 'Complete your other sacrifices and then return to me.'
      endif
    endif
  else
    return 0
    msend %actor% %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'You have already given me 6 %get.obj_pldesc[%gem_vnum%]%.'
  endif
elseif %go% == trident
  set job1 %actor.quest_variable[hell_trident:helltask1]%
  set job2 %actor.quest_variable[hell_trident:helltask2]%
  set job3 %actor.quest_variable[hell_trident:helltask3]%
  set job4 %actor.quest_variable[hell_trident:helltask4]%
  set job5 %actor.quest_variable[hell_trident:helltask5]%
  set job6 %actor.quest_variable[hell_trident:helltask6]%
  if %job1% && %job2% && %job3% && %job4% && %job5% && %job6%
    wait 2
    mjunk %object%
    if %self.vnum% == 12526
      msend %actor% %self.name% says, 'We are pleased.'
      grin
      wait 1s
      mecho %self.name% turns and faces out into the endless void of his vast realm.
      wait 1s
      mecho %self.name% growls, 'Ir ya roza aem ya iz ednuyt...'
      wait 2s
      mecho The inner &4&bire&0 of the %get.obj_noadesc[2339]% ignites and &1b&burn&0&1s&0!
      wait 2s
      mecho %self.name% roars, 'Liy ya gaoh yaezk'aqa, ir ya aehg mol'tiaer I kiwa yiz laodaer zes mina...'
      wait 2s
      mecho %self.name% erupts in glorious &1f&bl&3a&1m&0&1e&0!
      wait 2s
      mecho The &1f&3&bi&1r&0&1es&bt&3o&1r&0&1m&0 forms a vortex about the %get.obj_noadesc[2339]%, like an explosion in reverse.
      wait 1s
      mecho The trident &5&bt&0&5w&9&bi&0&5s&bts&0 and contorts in the wild &1f&3&bi&1r&0&1e&0!
      wait 3s
      mecho The fire subsides, leaving a &9&bmidnight-black&0 weapon pulsing with dark radiance.
      wait 2s
      msend %actor% %self.name% says, 'Our bond is forged.'
    else
      msend %actor% %self.name% says, 'The infernal ones are pleased.'
      wait 1s
      mecho %self.name%'s neck goes limp as her eyes roll back into her head.
      wait 2s
      mecho %self.name% begins to murmur... 'Hin tel'quiet nehel -nal rillis fis...'
      wait 3s
      mecho Brilliant &4b&9&blac&0&4k fire seeps out of %get.obj_noadesc[2334]% and spreads across its surface.
      wait 2s
      mecho %self.name% babbles in an alien voice, 'Aul adoe shunti mor ik mor...'
      wait 2s
      mecho &1Scarlet&0 &4fl&bames&0 ignite in %self.name%'s hands, pulsing in rhythm with her speech.
      wait 2s
      mecho %self.name% utters, 'Slidc ya qnoes oynaezz ya khozz qaed...' as she holds her hand over the %get.obj_noadesc[2334]%.
      wait 2s
      mecho The flames burn away the trident, leaving only a blazing tendril.
      wait 2s
      mecho %self.name% grabs the burning spire and commands, 'Le yaezzoth esaeu qae qota maenz!'
      mecho The wild flames contort as they cool into a new three-pointed form.
      wait 2s
      if %actor.level% >= 90
        msend %actor% %self.name% says, 'The Demon Lord Krisenna is known to traffic with mortals from time to time.  Impress him and perhaps he will grant you a boon.'
      else
        msend %actor% %self.name% says, 'Continue to prove your value to Hell and perhaps a Demon Lord might be willing to grant your their patronage.'
        msend %actor% &1You must be level %level% or greater to continue this quest.&0
      endif
    endif
    mload obj %reward%
    give trident %actor%
    eval expcap %level%
    if %expcap% < 17
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
    msend %actor% &3&bYou gain experience!&0
    eval setexp (%expmod% * 10)
    set loop 0
    while %loop% < 10
      mexp %actor% %setexp%
      eval loop %loop% + 1
    done
    set number 1
    while %number% < 7
      quest variable hell_trident %actor% helltask%number% 0
      eval number %number% + 1
    done
    quest variable hell_trident %actor% gems 0
    quest variable hell_trident %actor% greet 0
    if %actor.quest_stage[hell_trident]% == 1
      quest advance hell_trident %actor%
    else
      quest complete hell_trident %actor%
    endif
  else
    set refuse 1
    set reason You have to complete all your other offerings before you give me your trident.
  endif
endif
if %refuse%
  return 0
  msend %actor% %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, '%reason%'
endif
~
#5356
Hell Trident death~
0 f 100
~
switch %self.vnum%
  case 4001
  case 4005
  case 4010
  case 4015
  case 12307
    set phase 1
    set word angels
    break
  case 23810
  case 23811
  case 23812
    set phase 2
    set word celestials
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
    if %person.quest_stage[hell_trident]% == %phase%
      eval kills %person.quest_variable[hell_trident:celestials]% + 1
      quest variable hell_trident %person% celestials %kills%
      if %kills% >= 6
        quest variable hell_trident %person% helltask2 1
        quest variable hell_trident %person% celestials 0
        msend %person% &1&bYou have sufficiently bathed in the blood of the %word%!&0
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#5357
Hell Trident progress tracker~
0 d 0
weapon progress~
wait 2
if %actor.class% /= Diabolist
  if %actor.quest_stage[hell_trident]%
    set hellstage %actor.quest_stage[hell_trident]%
    if %hellstage% == 1
      set level 65
    elseif %hellstage% == 2
      set level 90
    endif
    switch %self.vnum%
      case 2311
        switch %hellstage%
          case 1
            set response stage2
            break
          case 2
            set response stage3
            break
          default
            if %actor.has_completed[hell_trident]%
              set response complete
            endif
        done
        break
      case 6032
        set step 1
        set gem 55662
        set pl_word angels
        set word angel
        set spell1 Hellfire and Brimstone
        set quest1 %actor.has_completed[hellfire_brimstone]%
        set spell2 Banish
        set quest2 %actor.has_completed[banish]%
        if !%actor.quest_variable[hell_trident:helltask6]% && %actor.level% >= %level% && %hellstage% == 1
          if %actor.quest_stage[vilekka_stew]% > 3
            quest variable hell_trident %actor% helltask6 1
          endif
        endif
        set task6 assisted Vilekka Kar'Shezden
        set task6do &3&bAssist the High Priestess of Lolth in hunting down and destroying the heretics of her Goddess.&0
        break
      case 12526
        set step 2
        set gem 55739
        set pl_word ghaeles, solars, or seraphs
        set word ghaele, solar, or seraph
        set spell1 Resurrect
        set quest1 %actor.has_completed[resurrection_quest]%
        set spell2 Hell Gate
        set quest2 %actor.has_completed[hell_gate]%
        set task6 Defeated the Undead Prince
        set task6do find one long-buried and branded an infidel.  &3&bFinish his undying duel for him&0 as a sacrifice of honor.
    done
    if %hellstage% == %step%
      if %actor.level% >= %level%
        if !%actor.quest_variable[hell_trident:helltask4]%
          if %quest1%
            quest variable hell_trident %actor% helltask4 1
          endif
        endif
        if !%actor.quest_variable[hell_trident:helltask5]%
          if %quest2%
            quest variable hell_trident %actor% helltask5 1
          endif
        endif
        if !%actor.quest_variable[hell_trident:greet]%
          msend %actor% %self.name% says, 'Tell me why you have come.'
        else
          set job1 %actor.quest_variable[hell_trident:helltask1]%
          set job2 %actor.quest_variable[hell_trident:helltask2]%
          set job3 %actor.quest_variable[hell_trident:helltask3]%
          set job4 %actor.quest_variable[hell_trident:helltask4]%
          set job5 %actor.quest_variable[hell_trident:helltask5]%
          set job6 %actor.quest_variable[hell_trident:helltask6]%
          if %job1% || %job2% || %job3% || %job4% || %job5% || %job6%
            msend %actor% %self.name% says, 'You have done the following:'
            if %job1%
              msend %actor% - attacked 666 times
            endif
            if %job2%
              msend %actor% - slayed 6 %pl_word%
            endif
            if %job3%
              msend %actor% - found 6 %get.obj_shortdesc[%gem%]%
            endif
            if %job4%
              msend %actor% - learned %spell1%
            endif
            if %job5%
              msend %actor% - learned %spell2%
            endif
            if %job6%
              msend %actor% - %task6%
            endif
            msend %actor% &0  
          endif
          if %job1% && %job2% && %job3% && %job4% && %job5% && %job6%
            msend %actor% %self.name% says, 'Give me your trident to finalize the pact.'
          else
            msend %actor% You must still:
            if !%job1%
              eval remaining 666 - %actor.quest_variable[hell_trident:attack_counter]%
              if %remaining% > 1
                msend %actor% - &3&battack %remaining% more times with your trident&0
              else 
                msend %actor% - &3&battack %remaining% more time with your trident&0
              endif
            endif
            if !%job2%
              eval kills 6 - %actor.quest_variable[hell_trident:celestials]%
              if %kills% > 1
                msend %actor% - &3&bslay %kills% more %pl_word%&0
              else
                msend %actor% - &3&bslay %kills% more %word%&0
              endif
            endif
            if !%job3%
              eval gems 6 - %actor.quest_variable[hell_trident:gems]%
              if %gems% > 1
                msend %actor% - &3&bfind %gems% more %get.obj_pldesc[%gem%]%&0
              else
                msend %actor% - &3&bfind %gems% more %get.obj_noadesc[%gem%]%&0
              endif
            endif
            if !%job4%
              msend %actor% - &3&blearn %spell1%&0
            endif
            if !%job5%
              msend %actor% - &3&blearn %spell2%&0
            endif
            if !%job6%
              msend %actor% - %task6do%
            endif
          endif
        endif
      else
        set response level
      endif  
    else
      if %actor.has_completed[hell_trident]%
        set response complete
      elseif %actor.quest_stage[hell_trident]% < %step%
        set response stage1
      else
        set response stage3
      endif
    endif
  endif
  if %response% == stage1
    msend %actor% %self.name% says, 'You must make the initial offerings with another dark priest.'
  elseif %response% == stage2
    if %actor.level% >= %level%
      msend %actor% %self.name% says, 'Hell hungers for more and will reward you greatly if you feed it.  Attack with that trident 666 times and then seek out the Black Priestess, the left hand of Ruin Wormheart.  She will guide your offerings.'
    else
      msend %actor% %self.name% says, 'Other forces of Hell will eventually take notice of you too now.  Seek out the left hand of Ruin Wormheart, the Black Priestess, after you have grown more.  She will be your emissary.'
      msend %actor% &1You must be level %level% or greater to continue this quest.&0
    endif
  elseif %response% == stage3
    if %actor.level% >= %level%
      msend %actor% %self.name% says, 'The Demon Lord Krisenna is known to traffic with mortals from time to time.  Impress him and perhaps he will grant you a boon.'
    else
      msend %actor% %self.name% says, 'Continue to prove your value to Hell and perhaps a Demon Lord might be willing to grant your their patronage.'
      msend %actor% &1You must be level %level% or greater to continue this quest.&0
    endif
  elseif %response% == complete
    msend %actor% %self.name% says, 'You've already marshalled the forces of Hell and Damnation to your side!'
  elseif %response% == level
    msend %actor% %self.name% say, 'Stand before me again when you have achieved a larger measure of greatness.'
    msend %actor% &1You must be level %level% or greater to continue this quest.&0
  endif
endif
~
$~
