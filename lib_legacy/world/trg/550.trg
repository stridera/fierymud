#55000
quest_cryo_greet~
0 dg 100
hi hello~
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
switch %actor.quest_stage[subclass]%
  case 1
    msend %actor% %self.name% says, 'Welcome back.'
    wait 2
    msend %actor% %self.name% says, 'It will take a great mage with a dedication to the cold arts to complete the &6&bquest&0 I lay before you.  Your reward is simple if you succeed, and I am sure you will enjoy a life of the cold.'
    break
  case 2
    msend %actor% %self.name% says, 'I see you return.'
    wait 2
    msend %actor% %self.name% says, 'Recall, what Emmath and I battled over may still be &6&bsuffering&0.'
    break
  case 3
    msend %actor% %self.name% says, 'Why have you returned already?  The shrub still suffers!'
    break
  case 4
    msend %actor% %self.name% says, 'Does the shrub still suffer?'
    break
  default
    if %actor.class% /= sorcerer
      switch %actor.race%
        case dragonborn_fire
        case arborean
          halt
          break
        default
          if %actor.level% >= 10 && %actor.level% <= 45
            grin %actor.name%
            msend %actor% %self.name% says, 'Greetings %actor.name%, have you come to me for a specific &6&breason&0?'
            eye %actor.name%
          elseif %actor.level% < 10
            grin %actor.name%
            msend %actor% %self.name% says, 'Greetings %actor.name%.  I fear we are meeting before you are ready.  Gain some more experience, then seek me out again.'
            wait 2s
            msend %actor% %self.name% says, 'I look forward to our next encounter.'
          endif
      done
    endif
done
~
#55001
Tech_Master_Shaman_1~
0 h 100
~
*Add Kourrya 6-06, loading for the Minithawkin troll mask quest
if %actor.race% /= troll
  mjunk mangrove-branch
  mload obj 37080
endif
wait 2
*
* for Wizard Eye
*
if ((%actor.class% /= Sorcerer || %actor.class% /= Illusionist) && %actor.level% > 80) || %actor.quest_stage[%type%_wand]% == 5
  if (%actor.class% /= Sorcerer || %actor.class% /= Illusionist) && %actor.level% > 80
    set stage %actor.quest_stage[wizard_eye]%
    switch %stage%
      case 1
        msend %actor% %self.name% tells you, 'You best seek out the gypsy witch.  Without her advice, I cannot help you further.'
        break
      case 2
        msend %actor% %self.name% tells you, 'Have you brought what the witch suggested?'
        break
      case 3
      case 4
        msend %actor% %self.name% tells you, 'How is the Seer in her cave on Griffin Isle?  I have not seen her for many years!'
        break
      case 5
        msend %actor% %self.name% tells you, 'Have you brought what the Seer suggested?'
        break
      case 6
      case 7
        msend %actor% %self.name% tells you, 'Before you return to me, speak with the Green Woman in Anduin.  She has learned a great many things from her post behind the counter.'
        break
      case 8
        msend %actor% %self.name% tells you, 'Have you brought what the Green Woman suggested?'
        break
      case 9
      case 10
        msend %actor% %self.name% tells you, 'The orbs are far too dangerous to bring back here without consulting the Oracle of Justice!'
        if %actor.inventory[3218]% || %actor.inventory[53424]% || %actor.inventory[43021]% || %actor.inventory[4003]% || %actor.wearing[3218]% || %actor.wearing[53424]% || %actor.wearing[43021]% || %actor.wearing[4003]%
          msend %actor% %self.name% tells you, 'Leave my chamber before these orbs corrupt it!'
          msend %actor% %self.name% shoves you back out into the hallway!
          mechoaround %actor% %self.name% shoves %actor.name% back out into the hallway!
          mteleport %actor.name% 55073
          mat 55073 mforce %actor.name% look
        endif
        break
      case 11
        msend %actor% %self.name% tells you, 'I see you have returned enlightened!  Let me see the crystal ball and I shall make the preparations.'
        break
      case 12
        msend %actor% %self.name% tells you, 'Come, &6&bsleep&0, let your dreams impart mastery of divination to you.'
        break
      default
        if !%actor.has_completed[wizard_eye]%
          msend %actor% %self.name% tells you, 'You are more powerful than most visitors.  Have you come to see as the Great Snow Leopard does?'
        endif
        break
    done
    if %actor.quest_stage[%type%_wand]% == 5
      if %actor.quest_variable[%type%_wand:greet]% == 0
        peer %actor%
        wait 1s
        msend %actor% %self.name% tells you, 'Or perhaps you come about a crafting &6&b[upgrade]&0...'
      else
        msend %actor% %self.name% tells you, 'Or have you brought the necessary components for the wand?'
      endif
    endif
  endif
  *
  * for phase wands
  *
  if %actor.quest_stage[%type%_wand]% == 5
    set job1 %actor.quest_stage[%type%_wand:wandtask1]%
    set job2 %actor.quest_stage[%type%_wand:wandtask2]%
    set job3 %actor.quest_stage[%type%_wand:wandtask3]%
    set job4 %actor.quest_stage[%type%_wand:wandtask4]%    
    if %actor.level% >= 40
      if %actor.quest_variable[%type%_wand:greet]% == 0
        msend %actor% %self.name% tells you, 'I see you're crafting something.  If you come for the Great Snow Leopard's help, let's discuss an &6&b[upgrade]&0.'
      elseif %actor.quest_stage[%type%_wand]% == 5 && %job1% && %job2% && %job3% && %job4%
        msend %actor% %self.name% tells you, 'It looks like the wand is ready for upgrading!  Please give it to me.'
      else
        msend %actor% %self.name% tells you, 'Do you have what I need for the wand?'
      endif
    endif
  endif
*
* This is the original greet trig for the shaman quest
*
else
  msend %actor% %self.name% raises an eyebrow.
  msend %actor% %self.name% mutters something about people disturbing her while she considers what to do about what's missing.
  msend %actor% %self.name% grumbles incoherently about something or other.
endif
~
#55002
Tech_Master_Shaman_2~
0 d 1
missing~
* This is the initial conversation to get quest data
mecho The &3&bMaster Shaman&0 rolls her eyes in disgust.
msend %actor% The &3&bMaster Shaman&0 whispers to you, 'Yes, my predecessors and ancestors lost many
msend %actor% &0battles with demons and a cult of the followers of Lokari and in the process
msend %actor% &0lost the keys.'
mechoaround %actor% The &3&bMaster Shaman&0 speaks in a low voice to %actor.name%.
mecho The &3&bMaster Shaman&0 pulls out her unused thinking cap, and begins to think.
~
#55003
Tech_Master_Shaman_3~
0 d 1
evils~
* Yet more quest information
mechoaround %actor% The &3&bMaster Shaman&0 raises an eyebrow at %actor.name%.
msend %actor% The &3&bMaster Shaman&0 raises an eyebrow at you.
mechoaround %actor% The &3&bMaster Shaman&0 speaks to %actor.name% in a low voice.
msend %actor% The &3&bMaster Shaman&0 whispers to you, 'Yes, Huitzipa the war god and that cursed
msend %actor% &0death god, I can't even say his name.'
mecho The &3&bMaster Shaman&0 spits over her shoulder.
mechoaround %actor% The &3&bMaster Shaman&0 speaks to %actor.name% in a low voice.
msend %actor% The &3&bMaster Shaman&0 whispers to you, 'If it weren't for the great Snow Leopard of
msend %actor% &0the ancient age, our kingdom would have perished long long ago.'
mecho The &3&bMaster Shaman&0 nods to herself.
~
#55004
Tech_Master_Shaman_4~
0 d 1
keys~
* Still more quest banter
mecho The &3&bMaster Shaman&0 nods.
mechoaround %actor% The &3&bMaster Shaman&0 speaks to %actor.name% in a low voice.
msend %actor% The &3&bMaster Shaman&0 whispers to you, 'Whatever they are, no one even remembers
msend %actor% &0anymore.  They are just the means by which the greatest evils my kingdom has
msend %actor% &0ever known can be set free again.'
mecho The &3&bMaster Shaman&0 frowns.
~
#55005
Tech_Master_Shaman_5~
0 d 1
huitzipa~
* More quest conversation
mecho The &3&bMaster Shaman&0 blinks.
mechoaround %actor% The &3&bMaster Shaman&0 speaks to %actor.name% in a low voice.
msend %actor% The &3&bMaster Shaman&0 whispers to you, 'How dare you repeat that vile name in my
msend %actor% &0presence!  If it were not for the Great Leopard we would be lost.  The
msend %actor% &0injustice and recklessness of them, I wish him destroyed!'
mecho The &3&bMaster Shaman&0 growls.
~
#55006
Master Shaman receive tear~
0 j 100
55023~
wait 1
eye %actor.name%
mjunk golden
say This looks like one of the legendary keys!
wait 2
thank %actor.name%
wait 2
say Oh thank you!  Take this as a reward.
mload obj 55019
give fang %actor.name%
drop fang
~
#55007
Master Shaman skull receive~
0 j 100
55005~
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
   if !%person.quest_stage[tech_mysteries_quest]%
      quest start tech_mysteries_quest %person.name%
   endif
   quest variable tech_mysteries_quest %person.name% cloak 1
   elseif %person%
   eval i %i% + 1
   endif
   eval a %a% + 1
done
mjunk skull
say Oh my!  You have destroyed a great evil!  My kingdom will never be threatened by it again!
wait 2
thank %actor.name%
wait 2
say I know the Snow Leopard would want you to have this!
mload obj 55008
give cloak %actor.name%
drop cloak
~
#55008
Master Shaman refuse~
0 j 0
55005 55023~
set stage %actor.quest_stage[wizard_eye]%
switch %object.vnum%
  case %wandgem%
  case %wandvnum%
  case %wandtask3%
    halt
    break
  case 58609
    if %stage% == 2
      halt
    endif
    break
  case 55030
    if %stage% == 5
      halt
    endif
    break
  case 55032
    if %stage% == 8
      halt
    endif
    break
  case 55033
    if %stage% == 11
      halt
    endif
done
if %stage% == 2
  set response Is %object.shortdesc% really what she sent you to find?
elseif %stage% == 5 || %stage% == 8
  set response Is %object.shortdesc% really what she suggested you use?
elseif %stage% == 11
  set response What?  %object.shortdesc%?  This can't be right.
else
  set response What is this?
endif
if %response%
  return 0
  eye %actor.name%
  msend %actor% %self.name% says, '%response%'
  mecho %self.name% refuses %object.shortdesc%.
endif
~
#55010
Knight_Champ_speech1~
0 d 100
deities~
spit
mechoaround %actor% The Knight Champion speaks in a low voice to %actor.name%.
msend %actor% The Knight Champion whispers to you, 'Xapizo the nemesis of death and that war
msend %actor% &0god!'
wait 1s
growl
mechoaround %actor% The Knight Champion speaks in a low voice to %actor.name%.
msend %actor% The Knight Champion whispers to you, 'They were nearly our undoing!'
wait 1s
eye
msend %actor% The Knight Champion whispers to you, 'To destroy Xapizo, now there would be a
msend %actor% &0battle to earn respect! I wish I were so strong.'
~
#55011
Knight_Champ_speech2~
0 d 100
evil~
nod
mechoaround %actor% The Knight Champion speaks in a low voice to %actor.name%.
msend %actor% 
The Knight Champion whispers to you, 'Yes there was evil in the kingdom long
msend %actor% &0ago two reckless deities warring. The great Snow Leopard saved us all.'
~
#55012
Knight_Champ_speech3~
0 d 100
victory~
nod %actor.name%
mechoaround %actor% The Knight Champion speaks in a low voice to %actor.name%.
msend %actor% The Knight Champion whispers to you 'Yes a victory in battle over a foe of
msend %actor% &0great evil.  Or an army of the darkness.'
~
#55013
Knight_Champ_speech4~
0 d 100
worth worthy~
trip %actor.name%
mecho %self.name% says, 'You are unworthy!  Standing here like a helpless
mecho &0pig!'
growl
~
#55014
Knight_Champ_speech5~
0 d 100
prove~
mechoaround %actor% The Knight Champion speaks in a low voice to %actor.name%.
msend %actor% The Knight Champion whispers to you 'Yes prove you are worthy of honour and
msend %actor% &0respect.'
think
wait 1s
msend %actor% The Knight Champion whispers to you 'Perhaps win a great victory over a great
msend %actor% &0evil.'
~
#55015
Knight_Champ_rece~
0 j 100
~
* Knight Champion Receive object for quest goodie
if %object.vnum% == 55004
  wait 1
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
      if !%person.quest_stage[tech_mysteries_quest]%
         quest start tech_mysteries_quest %person.name%
      endif
      quest variable tech_mysteries_quest %person.name% girth 1
   elseif %person%
      eval i %i% + 1
   endif
   eval a %a% + 1
   done
  mecho %self.name% says, 'You did it!  You have the book!  The only way would have been to venture to the planes and destroy Xapizo!'
  mjunk codex
  say I know the Snow Leopard would want you to have this.
  mload obj 55013
  give girth %actor.name%
  drop girth
  else
  wait 1
  eye %actor.name%
endif
~
#55016
Knight_Champ_greet~
0 g 100
~
msend %actor% The Knight Champion raises his eyebrow at you.
msend %actor% The Knight Champion says, 'Hrmm?  More worthless dogs come to prove your
msend %actor% &0worth?'
~
#55020
quest_suralla_speak1~
0 d 100
reason reason?~
if %actor.class% /= Sorcerer
  switch %actor.race%
    case dragonborn_fire
    case arborean
      if (%actor.level% >= 10 && %actor.level% <= 45)
        msend %actor% &1Your race cannot subclass to cryomancer.&0
      endif
      halt
      break
    default
      wait 2
      if (%actor.level% >= 10 && %actor.level% <= 45)
        msend %actor% %self.name% says, 'Do you wish to become a mage mastering the elements of ice and water and wind?'
      elseif %actor.class% /= Sorcerer && %actor.level% < 10
        msend %actor% %self.name% says, 'Come back to me once you've gained more experience.'
      elseif %actor.class% /= Sorcerer && %actor.level% > 45
        msend %actor% %self.name% says, 'Unfortunately you are too dedicated to your universalist ways for me to teach you now.'
      endif
  done
endif
~
#55021
quest_suralla_yesno~
0 d 100
yes no~
if %actor.class% /= Sorcerer
  switch %actor.race%
    case dragonborn_fire
    case arborean
      if (%actor.level% >= 10 && %actor.level% <= 45)
        msend %actor% &1Your race cannot subclass to cryomancer.&0
      endif
      halt
      break
    default
      wait 2
      if %speech% /= yes
        if (%actor.level% >= 10 && %actor.level% <= 45)
          quest start cryomancer_subclass %actor.name% Cry
          nod
          msend %actor% %self.name% says, 'It will take a great mage with a dedication to the cold arts to complete the &6&bquest&0 I lay before you.  Your reward is simple if you succeed, and I am sure you will enjoy a life of the cold.'
          wait 2s
          msend %actor% %self.name% says, 'At any time you may check your &6&b[subclass progress]&0.'
        elseif %actor.level% < 10
          msend %actor% %self.name% says, 'Come back to me once you've gained more experience.'
        elseif %actor.class% /= Sorcerer && %actor.level% > 45
          msend %actor% %self.name% says, 'Unfortunately you are too dedicated to your universalist ways for me to teach you now.'
        endif
      else
        msend %actor% %self.name% says, 'Very well, begone.  I feel like I have wasted time now thanks to you.'
        sigh
        msend %actor% Suralla blinks several times at you, then it becomes very bright indeed!
        mechoaround %actor%  Suralla begins blinking at %actor.name% who is suddenly taken away in a flash of light.
        mteleport %actor.name% 55015
      endif
  done
endif
~
#55022
quest_suralla_speak2~
0 d 100
quest reward~
wait 2
if %actor.quest_stage[cryomancer_subclass]% == 1
  quest advance cryomancer_subclass %actor.name%
  wait 1s
  msend %actor% %self.name% says, 'It is quite simple.'
  wait 1s
  msend %actor% %self.name% says, 'My counterpart, the great Emmath Firehand, long ago battled with me once.'
  emote reminisces for a brief moment, looking thoughtful.
  wait 1
  msend %actor% %self.name% says, 'It was not serious by any means, but it did end in a stalemate.  The catch however...'
  wait 1s
  sigh
  wait 1s
  msend %actor% %self.name% says, '... is that what we battled over may still be &6&bsuffering&0.'
  emote sighs deeply again.
endif
~
#55023
quest_suralla_speak3~
0 d 100
suffering suffer battled still what~
wait 2
if %actor.quest_stage[cryomancer_subclass]% == 2
  wait 1s
  emote smiles sadly.
  wait 1s
  msend %actor% %self.name% says, 'It is a shame really, that poor shrub, it really was an innocent in all of that.'
  wait 1s
  msend %actor% %self.name% says, 'I do feel bad about it.  The poor thing tried to flee us and sought the shaman who created him.'
  wait 2s
  msend %actor% %self.name% says, 'I do not know if that will help you end his suffering or not, but I hope it does.'
  wait 2s
  grin
  wait 2s
  msend %actor% %self.name% says, 'And the reward will be great if you do.  The shrub muttered something about a place with rushing water and some odd warriors being his safety.'
  wait 1s
  msend %actor% %self.name% says, 'Oh!  One last thing.  When you return to claim your reward, be sure to say to me &6&b"the shrub suffers no longer"&0, and the prize will be yours.'
  wait 2s
  smile
  wait 2s
  msend %actor% %self.name% says, 'Go about it now.'
  quest advance cryomancer_subclass %actor.name%
endif
~
#55024
Wizard Eye Master Shaman receive 1~
0 j 100
58609~
set stage %actor.quest_stage[wizard_eye]%
if %stage% == 2
  wait 2
  mjunk %object%
  quest advance wizard_eye %actor.name%
  msend %actor% %self.name% says, 'Ah, marigold for clairvoyance.  That makes sense.  I will store it while you seek the advice of the next sage.'
  msend %actor% %self.name% tucks the marigold poultice away in her chamber.
  wait 5s
  msend %actor% %self.name% says, 'There are a handful of true visionaries in Ethilien, but one stands out among the rest.  The &6&bSeer of Griffin Isle&0 always receives the most cryptic of visions.'
  wait 4s
  msend %actor% %self.name% says, 'Whatever a "sun screen" is I have no idea, but she speaks of it often.'
  ponder
  wait 1s
  msend %actor% %self.name% says, 'Often I am forced to question her sanity...'
  wait 4s
  msend %actor% %self.name% says, 'Regardless of her mental state, she is a truly master diviner.  &6&bVisit her&0 and see what she recommends you do to attune to your future crystal ball.'
endif
~
#55025
quest_suralla_burningshrub_dead~
0 f 100
~
mecho %self.name% says, 'Thank you for ending my pain.'
set person %actor%
set i %actor.group_size%
if %i%
   set a 1
   unset person
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[cryomancer_subclass]% == 3
            quest advance cryomancer_subclass %person.name%
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[cryomancer_subclass]% == 3
   quest advance cryomancer_subclass %person.name%
endif
~
#55026
quest_suralla_endquest~
0 d 0
the shrub suffers no longer~
wait 2
switch %actor.quest_stage[cryomancer_subclass]%
  case 1
  case 2
    msend %actor% %self.name% says, 'Interesting, how could you have completed the quest without actually knowing what it is?'
    break
  case 3
    msend %actor% %self.name% says, 'Nay, you did not partake in ending its suffering.'
    break
  case 4
    thank %actor.name%
    msend %actor% %self.name% says, 'Thank you so very much for righting my wrong.'
    wait 2s
    msend %actor% %self.name% says, 'It is for the best.'
    wait 2s
    bow %actor.name%
    wait 2
    msend %actor% %self.name% says, 'Type &6&b'subclass'&0 to proceed.'
    quest complete cryomancer_subclass %actor.name%
    break
  default
    msend %actor% %self.name% says, 'What is going on?  The voices again, it is all happening again......'
    wait 2
    msend %actor% %self.name% says, 'Wait, I seem to remember something about a quest...  Pity you were not on it.'
done
~
#55027
quest_suralla_opening~
2 c 0
look~
if %arg% /= ice || %arg% /= sculpture
  wsend %actor%  The light of the ice sculpture swirls around you and you find yourself moving.
  wechoaround %actor% %actor.name% looks at the ice sculpture and is engulfed by light.
  wdoor 55015 south room 55000
  wechoaround %actor% The ice sculpture splits in two and %actor.name% is sucked through by a strong gust!
  wsend %actor% The ice sculpture splits in two and you are sucked through by a strong gust!
  wat 55000 wecho There is a sudden split in the wall of ice!
  wforce %actor% south
  wait 1
  wdoor 55015 south purge
  wecho The sculpture seals behind %actor.o%.
else
  return 0
endif
~
#55028
cryomancer_subclass_status~
0 d 0
subclass progress~
switch %actor.quest_stage[cryomancer_subclass]%
  case 1
    wait 2
    msend %actor% %self.name% says, 'It will take a great mage with a dedication to the cold arts to complete the &6&bquest&0 I lay before you.  Your reward is simple if you succeed, and I am sure you will enjoy a life of the cold.'
    break
  case 2
    wait 2
    msend %actor% %self.name% says, 'It is quite simple.
    wait 1s
    msend %actor% %self.name% says, 'My counterpart, the great Emmath Firehand, long ago battled with me once.'
    emote reminisces for a brief moment, looking thoughtful.
    wait 1
    msend %actor% %self.name% says, 'It was not serious by any means, but it did end in a stalemate.  The catch however...'
    sigh
    wait 1s
    msend %actor% %self.name% says, 'Is that what we battled over may still be &6&bsuffering&0.'
    emote sighs deeply again.        
    break
  case 3
    wait 2
    emote smiles sadly.
    msend %actor% %self.name% says, 'It is a shame really, that poor shrub, it really was an innocent in all of that.'
    wait 1s
    msend %actor% %self.name% says, 'I do feel bad about it.  The poor thing tried to flee us and sought the shaman who created him.'
    wait 2s
    msend %actor% %self.name% says, 'I do not know if that will help you end his suffering or not, but I hope it does.'
    grin
    wait 1s
    msend %actor% %self.name% says, 'And the reward will be great if you do.
    wait 2s
    msend %actor% %self.name% says, 'The shrub muttered something about a place with rushing water and some odd warriors being his safety.'
    wait 1s
    msend %actor% %self.name% says, 'Oh!  One last thing.  When you return to claim your reward, be sure to say to me &6&b"The shrub suffers no longer"&0, and the prize will be yours.'
    smile
    msend %actor% %self.name% says, 'Go about it now.
    break
  case 4
    wait 2
    msend %actor% %self.name% says, 'Say &6&b"The shrub suffers no longer"&0, and the prize will be yours.'
    break
  default
    if %actor.class% /= Sorcerer
      switch %actor.race%
        case dragonborn_fire
        case arborean
          if (%actor.level% >= 10 && %actor.level% <= 45)
            msend %actor% &1Your race cannot subclass to cryomancer.&0
          endif
          break
        default
          wait 2
          if (%actor.level% >= 10 && %actor.level% <= 45)
            msend %actor% %self.name% says, 'You are not yet on my quest.'
          elseif %actor.level% < 10
            msend %actor% %self.name% says, 'Come back to me once you've gained more experience.'
          elseif %actor.class% /= Sorcerer && %actor.level% > 45
            msend %actor% %self.name% says, 'Unfortunately you are too dedicated to your universalist ways for me to teach you now.'
          endif
      done
    endif
done
~
#55030
**UNUSED**~
1 c 100
enter~
*
* Test trigger to see if checks can be done
* before a player can enter a portal.
*
if %actor.has_completed[doom_entrance]% == true
oechoaround %actor% Dude like disappears and stuff.
osend %actor% You step through the portal.
oteleport %actor% 1210
else
oechoaround %actor% Dude tries to enter... NOT.
osend %actor% Hows about you complete the quest first?
end
~
#55031
wizard_eye_shaman_speech1~
0 d 100
eye scrying divination ball yes~
wait 2
set stage %actor.quest_stage[wizard_eye]%
if (%actor.class% /= Sorcerer || %actor.class% /= Illusionist)
  if (%actor.level% > 80)
    if %stage% == 0
      msend %actor% %self.name% says, 'Ah, so you wish to gain the Sight of the Great Snow Leopard.'
      nod
      wait 2s
      msend %actor% %self.name% says, 'Yes, I can teach you this spell.  But casting it is much more complicated than simple words and gestures.  You will need a uniquely attuned crystal ball to scry upon.'
      wait 4s
      msend %actor% %self.name% says, 'There are four other master diviners in the world. You will need to visit each one to figure out which items are best attuned to you.'
      wait 4s
      msend %actor% %self.name% says, 'Let us start with the one furthest away.  There is a &6&bgypsy witch who dwells in South Caelia&0.  Find her and &6&bask what you will need for Wizard Eye&0.'
      quest start wizard_eye %actor.name%
      wait 4s
      msend %actor% %self.name% says, 'You may return to me and check your &6&b[spell progress]&0 at any time.'
    elseif %stage% == 2 || %stage% == 5 || %stage% == 8
      msend %actor% %self.name% says, 'Give it to me then, please.'
    endif
  else
    shake
    msend %actor% %self.name% says, 'You are not ready to learn such advanced magics.  Meddling with divinations you are not prepared to handle can rip your mind apart!  I will not be responsible for such a thing again...'
  endif
else
  shake
  msend %actor% %self.name% says, 'That spell is outside your ability to cast.'
endif
~
#55032
wizard_eye_gypsy_speech~
0 d 1
wizard eye wizard? eye?~
wait 2
if %actor.quest_stage[wizard_eye]% == 1
  msend %actor% %self.name% says, 'Why yes, I can help with that!  So you want your own crystal ball, ya say?'
  wait 2s
  msend %actor% %self.name% says, '&6&bShow&0 me your &6&bpalm&0.'
endif
~
#55033
wizard_eye_gypsy_command~
0 c 100
show~
if %actor.quest_stage[wizard_eye]% == 1 && (%arg% /= palm || %arg% /= hand)
  msend %actor% You show your palm to %self.name%.
  wait 2s
  msend %actor% %self.name% looks at your palm for a long moment.
  wait 3s
  msend %actor% %self.name% says, 'Hmmmmm...'
  wait 2s
  mdamage %actor% 5
  msend %actor% %self.name% suddenly stabs your hand with a knife! &1(%damdone%)&0
  mechoaround %actor% %self.name% suddenly stabs %actor.name%'s hand with a knife!  &1(%damdone%)&0
  msend %actor% %self.name% watches the blood drip from your hand.
  mechoaround %actor% %self.name% watches the blood drip from %actor.name%'s hand.
  wait 2s
  msend %actor% %self.name% says, 'Ah, I see.  All you need is a simple &3&bmarigold poultice&0.  It's popular with the healers out on the beachhead.  I'm sure you can "procure" some from one of them.'
  wait 4s
  msend %actor% %self.name% says, 'When the time comes, smear it on both your face and the crystal ball.  I'll send a note ahead to the Master Shaman.'
  wait 3s
  msend %actor% %self.name% says, 'Good luck!  Come back and see me sometime!'
  wave
  quest advance wizard_eye %actor.name%
else
  return 0
endif
~
#55034
wizard_eye_seer_greet~
0 g 100
~
wait 2
eval minlevel (%wandstep% - 1) * 10
if %actor.quest_stage[wizard_eye]% == 3
  msend %actor% %self.name% says, 'I had a feeling you would show up soon.  The shaman from Technitzitlan has sent you to me, yes?'
  if %actor.quest_stage[%type%_wand]% == %wandstep%
    if %actor.level% >= %minlevel%
      wait 1s
      if %actor.quest_variable[%type%_wand:greet]% == 0
        msend %actor% %self.name% says, 'Or is there some other reason you're here?'
      else
        msend %actor% %self.name% says, 'Do you have what I need for the staff?'
      endif
    endif
  endif
elseif %actor.quest_stage[wizard_eye]% == 4
  msend %actor% %self.name% says, 'Do you have the herbs?
  if %actor.quest_stage[%type%_wand]% == %wandstep%
    if %actor.level% >= %minlevel%
      wait 1s
      if %actor.quest_variable[%type%_wand:greet]% == 0
        msend %actor% %self.name% says, 'Or is there some other reason you're here?'
      else
        msend %actor% %self.name% says, 'And do you have what I need for the staff?'
      endif
    endif
  endif
elseif %actor.quest_stage[%type%_wand]% == %wandstep%
  if %actor.level% >= %minlevel%
    if %actor.quest_variable[%type%_wand:greet]% == 0
      msend %actor% %self.name% says, 'I've been expecting you!  Tell me, what brings you to me?'
    else
      msend %actor% %self.name% says, 'Do you have what I need for the staff?'
    endif
  endif
endif
~
#55035
wizard_eye_seer_speech~
0 d 100
yes~
wait 2
if %actor.quest_stage[wizard_eye]% == 3
  quest advance wizard_eye %actor.name%
  msend %actor% %self.name% says, 'As I suspected.'
  peer %actor%
  wait 1s
  msend %actor% %self.name% says, 'So you want to learn Wizard Eye do you?  No simple feat, that.'
  wait 4s
  emote tents her fingers and paces back and forth.
  msend %actor% %self.name% says, 'Now, what is best for you, hmmmm?'
  wait 4s
  eye %actor.name%
  msend %actor% %self.name% says, 'Perhaps an infusion of poppies?'
  wait 4s
  shake
  msend %actor% %self.name% says, 'Nah.  Overkill.'
  wait 4s
  point %actor%
  msend %actor% %self.name% says, 'Perhaps basilisk oil?'
  wait 2s
  shake
  msend %actor% %self.name% says, 'No, extinct.'
  wait 4s
  msend %actor% %self.name% says, 'Wait, I see it!'
  Wait 1s
  msend %actor% %self.name% says, 'A sachet of oracular herbs!  Yes!  For prophetic dreams!'
  wait 3s
  msend %actor% %self.name% says, 'I can make it for you too, if you bring me the materials.
  msend %actor% I need the following:
  msend %actor% - &5%get.obj_shortdesc[48005]%&0
  msend %actor% - &2Thyme&0
  msend %actor% - &2&bBay&0
  msend %actor% &0  
  msend %actor% &3&bBring these back to me&0 and I'll get you on your way.  I'll send a note to the Master Shaman in the meantime.'
endif
~
#55036
wizard_eye_seer_receive~
0 j 100
2329 23753 48005~
if %actor.quest_stage[wizard_eye]% == 4
  switch %object.vnum%
    case 2329
      set item 1
      break
    case 23753
      set item 2
      break
    case 48005
      set item 3
  done
  if %actor.quest_variable[wizard_eye:item%item%]%
    return 0
    msend %actor% %self.name% says, 'You already brought me this!'
    msend %actor% %self.name% refuses %object.shortdesc%.
  else 
    quest variable wizard_eye %actor.name% item%item% 1
    wait 2
    mjunk %object%
    nod
    msend %actor% %self.name% says, 'Yep, this'll do.'
    if %actor.quest_variable[wizard_eye:item1]% && %actor.quest_variable[wizard_eye:item2]% && %actor.quest_variable[wizard_eye:item3]%
      quest advance wizard_eye %actor.name%
      set item 1
      while %item% <= 4
        quest variable wizard_eye %actor.name% item%item% 0
        eval item %item% + 1
      done
      wait 2
      msend %actor% %self.name% says, 'Let me just stitch up a little bag for you.'
      wait 1s
      emote cuts a large square from the indigo silk dress.
      emote quickly sews the square into a little pouch.
      emote fills the pouch with the thyme and bay leaves and stitches it shut.
      wait 2s
      msend %actor% %self.name% says, 'Here ya go!'
      mload obj 55030
      give sachet %actor.name%
      wait 1s
      msend %actor% %self.name% says, 'Sweet dreams!'
      cackle
    else
      wait 2
      if !%actor.quest_variable[wizard_eye:item1]%
        msend %actor% %self.name% says, 'What about the bay?'
      endif
      if !%actor.quest_variable[wizard_eye:item3]%
        wait 1s
        msend %actor% %self.name% says, 'Did you find a dress I can upcycle?'
      endif
      if !%actor.quest_variable[wizard_eye:item2]%
        wait 1s
        msend %actor% %self.name% says, 'Oh and do you have the thyme?'
        wait 1s   
        msend %actor% %self.name% says, 'Of course you don't, you don't have a wristwatch!'
        laugh
        wait 1s  
        msend %actor% %self.name% says, 'No but really, if you have it, give it to me.'
      endif
    endif
  endif
endif
~
#55037
wizard_eye_apothecary_greet~
0 g 100
~
wait 2
if %actor.vnum% == -1
  msend %actor% %self.name% says, 'What can I get for ya dearie?'
  if %actor.quest_stage[wizard_eye]% == 6
    wait 1s
    msend %actor% %self.name% says, 'Hmmm, there's something odd about you.  What have you come for?'
  endif
endif
~
#55038
wizard_eye_apothecary_speech~
0 d 100
wizard shaman crystal~
wait 2
if %speech% /= wizard eye || %speech% /= the shaman sent me || %speech% /= crystal ball
  if %actor.quest_stage[wizard_eye]% == 6
    quest advance wizard_eye %actor.name%
    grin
    msend %actor% %self.name% says, 'Well then that's a different sort of brew!'
    wait 1s
    peer %actor%
    wait 3s
    msend %actor% %self.name% says, 'No, not a brew...  But perhaps... a smell?'
    wait 3s
    msend %actor% %self.name% says, 'Yes, that should do it!  Incense to increase your psychic reach is what you need.'
    wait 3s
    msend %actor% %self.name% says, 'Two things that work very well together are &1roses&0 and &1cinnamon&0.
    wait 2s
    msend %actor% %self.name% says, 'Cinnamon is fairly straight-forward.'
    wait 3s
    msend %actor% %self.name% says, 'Roses, on the other hand, may be more tricky...'
    wait 1s
    msend %actor% %self.name% says, 'You need three varieties.'
    wait 2s
    msend %actor% %self.name% says, 'Two varieties of roses grow in Mielikki, one &1red&0 and one &9&bblack&0.  Bring back one of each.'
    wait 3s
    msend %actor% %self.name% says, 'A third kind is carried by serpents in Dargentan's sky tower.  Even though it's &4&bmade of sapphire&0 which is not the best for divination, it'll lend extra potency to the attunement process.'
    wait 4s
    msend %actor% %self.name% says, 'Bring me these ingredients and I'll make you a lovely incense.'
  endif
endif
~
#55039
wizard_eye_apothecary_receive~
0 j 100
23754 3298 23847 18001~
if %actor.quest_stage[wizard_eye]% == 7
  switch %object.vnum%
    case 23754
      set item 1
      break
    case 3298
      set item 2
      break
    case 23847
      set item 3
      break
    case 18001
      set item 4
  done
  if %actor.quest_variable[wizard_eye:item%item%]%
    return 0
    msend %actor% %self.name% refuses %object.shortdesc%.
    msend %actor% %self.name% says, 'You already brought me %object.shortdesc%.'
  else
    quest variable wizard_eye %actor.name% item%item% 1
    mjunk %object.name%      
    wait 2
    msend %actor% %self.name% says, 'Ah, %object.shortdesc%.'
    wait 4
    if %actor.quest_variable[wizard_eye:item1]% && %actor.quest_variable[wizard_eye:item2]% && %actor.quest_variable[wizard_eye:item3]% && %actor.quest_variable[wizard_eye:item4]%
      quest advance wizard_eye %actor.name%
      set item 1
      while %item% <= 4
        quest variable wizard_eye %actor.name% item%item% 0
        eval item %item% + 1
      done
      msend %actor% %self.name% says, 'That looks like everything.  Let me grind this all up!'
      wait 2s
      emote pulls out a mortar and pestle from behind the counter.
      wait 1s
      emote grinds the roses and cinnamon to a fine powder and shapes it into a small brick.
      wait 3s
      msend %actor% %self.name% says, 'This incense should be perfect for you!'
      mload obj 55032
      give incense %actor.name%
      wait 3s
      msend %actor% %self.name% says, 'Be careful!'
      grin %actor.name%
    else
      msend %actor% %self.name% says, 'Do you have the rest of the ingredients?'
    endif
  endif
endif
~
#55040
wizard_eye_oracle_receive~
0 j 100
3218 53424 43021 4003~
if %actor.quest_stage[wizard_eye]% == 10
  switch %object.vnum%
    case 3218
      set item 1
      break
    case 53424
      set item 2
      break
    case 43021
      set item 3
      break
    case 4003
      set item 4
  done
  if %actor.quest_variable[wizard_eye:item%item%]%
    return 0
    msend %actor% %self.name% refuses %object.shortdesc%.
    msend %actor% %self.name% says, 'You already brought me %object.shortdesc%.'
  else
    quest variable wizard_eye %actor.name% item%item% 1
    wait 2
    mjunk %object.name%
    if %actor.quest_variable[wizard_eye:item1]% && %actor.quest_variable[wizard_eye:item2]% && %actor.quest_variable[wizard_eye:item3]% && %actor.quest_variable[wizard_eye:item4]%
      quest advance wizard_eye %actor.name%
      set item 1
      while %item% <= 4
        quest variable wizard_eye %actor.name% item%item% 0
        eval item %item% + 1
      done
      nod
      msend %actor% %self.name% says, 'This seems to be all of them.'
      wait 2s
      msend %actor% %self.name% says, 'O Rhalean, show me the balance of Justice!'
      emote utters a few words of power!
      wait 2s
      msend %actor% The four orbs begin to &7&bglow&0!
      msend %actor% They become fully transparent and colorless.
      wait 3s
      emote scrutinizes the four now identical mystic orbs.
      wait 2s
      msend %actor% %self.name% says, 'Yes I see...'
      emote picks up one of the four clear crystal balls.
      mload obj 55033
      give crystal-ball %actor.name%
      wait 2s
      msend %actor% %self.name% says, 'This will be your window to the universe.'
      wait 2s
      bow %actor.name%
      msend %actor% %self.name% says, 'May Justice follow you always.'
    else
      nod
      msend %actor% %self.name% says, 'And the rest?'
    endif
  endif
endif
~
#55041
wizard_eye_shaman_sleep~
0 c 100
sleep~
if %actor.quest_stage[wizard_eye]% == 12
  return 0
  wait 1s
  msend %actor% A hazy dreamscape appears before you.
  mecho 
  msend %actor% The Great Snow Leopard comes into focus!
  msend %actor% The Great Snow Leopard says, 'Come, follow where I walk.'
  wait 2s
  msend %actor% The Great Snow Leopard leads you on a journey through crisp, cold, snowy mountains...
  wait 3s
  msend %actor% hot burning deserts, sweltering jungles of sweet scented flowers...
  wait 3s
  msend %actor% distant mysterious islands of alien sounds... 
  wait 3s
  msend %actor% through cities of people speaking languages you do not understand...
  wait 6s
  msend %actor% Soaring through the open sky, the Great Snow Leopard roars to shake the heavens!
  mecho 
  msend %actor% In the echoes of the roar, you can see shape distant lands!
  msend %actor% The nature of the spell becomes clear!
  quest complete wizard_eye %actor.name%
  mskillset %actor% wizard eye
  msend %actor% &6&bYou have learned Wizard Eye!&0
else
  return 0
endif
~
#55042
wizard_eye_status_checker~
0 d 0
spell progress~
set stage %actor.quest_stage[wizard_eye]%
set item1 %actor.quest_variable[wizard_eye:item1]%
set item2 %actor.quest_variable[wizard_eye:item2]%
set item3 %actor.quest_variable[wizard_eye:item3]%
set item4 %actor.quest_variable[wizard_eye:item4]%
wait 2
if %actor.has_completed[wizard_eye]%
  msend %actor% %self.name% says, 'You already have the sight of the Great Snow Leopard.'
  halt
endif
switch %stage%
  case 1
    msend %actor% %self.name% says, 'You are trying to find the &6&bgypsy witch in South Caelia Highlands.  Ask her about Wizard Eye.&0'
    halt
    break
  case 2
    msend %actor% %self.name% says, 'The gypsy witch says to find &3&bmarigold poultice&0 from a healer on the beachhead.'
    halt
    break
  case 3
    msend %actor% %self.name% says, 'Go visit the &6&bSeer of Griffin Isle&0 to see what you need to do next.  &6&bAsk her about Wizard Eye&0.'
    halt
    break
  case 4
    set who her
    set visit have the Seer make you an herbal sachet
    set thing1 %get.obj_shortdesc[2329]%
    set thing2 %get.obj_shortdesc[23753]%
    set thing3 %get.obj_shortdesc[48005]%
    break
  case 5
    msend %actor% %self.name% says, 'Give me the sachet.'
    halt
    break
  case 6
    msend %actor% %self.name% says, 'You are looking for &6&bthe apothecary in Anduin&0.'
    halt
    break
  case 7
    set who her
    set visit get The Green Woman to make you incense
    set thing1 %get.obj_shortdesc[23754]%
    set thing2 %get.obj_shortdesc[3298]%
    set thing3 %get.obj_shortdesc[23847]%
    set thing4 %get.obj_shortdesc[18001]%
    break
  case 8
    msend %actor% %self.name% says, 'Give me the incense.'
    halt
    break
  case 9
  case 10
    set who him
    set thing1 %get.obj_shortdesc[3218]%
    set thing2 %get.obj_shortdesc[53424]%
    set thing3 %get.obj_shortdesc[43021]%
    set thing4 %get.obj_shortdesc[4003]%
    set visit see the Oracle of Justice
    break
  case 11
    msend %actor% %self.name% says, 'Give me the crystal ball.'
    halt
    break
  case 12
    msend %actor% %self.name% says, 'All you need to do now is lay back and go to &6&bsleep&0.'
    halt
    break
  default
    msend %actor% %self.name% says, 'Progress on what spell?  I am not guiding you.'
    halt
done
msend %actor% %self.name% says, 'You need to %visit%.
if %item1% || %item2% || %item3% || %item4%
  msend %actor%   
  msend %actor% %self.name% says, 'You have already given %who%:
  if %item1%
    msend %actor% - %thing1%
  endif
  if %item2%
    msend %actor% - %thing2%
  endif
  if %item3%
    msend %actor% - %thing3%
  endif
  if %item4%
    msend %actor% - %thing4%
  endif
endif
msend %actor%   
msend %actor% %self.name% says, 'You still need to bring %who%:
if !%item1%
  msend %actor% - &3&b%thing1%&0
endif
if !%item2%
  msend %actor% - &3&b%thing2%&0
endif
if !%item3%
  msend %actor% - &3&b%thing3%&0
endif
if %stage% != 4
  if !%item4%
    msend %actor% - &3&b%thing4%&0
  endif
endif
~
#55043
Wizard Eye Master Shaman receive 2~
0 j 100
55030~
set stage %actor.quest_stage[wizard_eye]%
if %stage% == 5
  wait 2
  mjunk sachet
  quest advance wizard_eye %actor.name%
  msend %actor% %self.name% says, 'Very smart blend of herbs.  It will bring prophetic dreams.  She always did know her sachets.'
  wait 1s
  msend %actor% %self.name% says, 'I will store it for you.'
  msend %actor% %self.name% tucks the herbal sachet away in her chamber.
  wait 5s
  msend %actor% %self.name% says, 'I believe it's time to &6&bvisit the witchy woman who runs the apothecary in Anduin&0.'
  wait 3s
  msend %actor% %self.name% says, 'Like the others you have visited, she has no name any can recall, simply going by "the Green Woman."'
  wait 3s
  msend %actor% %self.name% says, 'Though she may not look like much, she is both a powerful diviner and a master of secrets.'
  wait 3s
  msend %actor% %self.name% says, 'Other peoples' secrets.  Which she sells to the highest bidder.'
  wait 3s
  msend %actor% %self.name% says, 'See what she recommends.  Surely she has a trick or two up her sleeve.'
endif
~
#55044
Wizard Eye Master Shaman receive 3~
0 j 100
55032~
set stage %actor.quest_stage[wizard_eye]%
if %stage% == 8
  wait 2
  quest advance wizard_eye %actor.name%
  mjunk %object.name%
  emote smells the cinnamon-rose blend.
  msend %actor% %self.name% says, 'Ahhh, what a gorgeous fragrance!'
  wait 2s
  msend %actor% %self.name% says, 'Cinnamon and rose both open the third eye andincrease psychic awareness.  And I would never have thought to use powdered sapphire in an incense.  Very clever.'
  wait 3s
  msend %actor% %self.name% says, 'I shall put this with the other necessaries.'
  msend %actor% %self.name% tucks the incense away in her chamber.
  wait 6s
  msend %actor% %self.name% says, 'It looks like you have everything prepared.  Now for the most difficult part.'
  wait 2s
  msend %actor% %self.name% says, 'I have been scrying on my cauldron to determine the final steps of your journey.  I believe I have located &3&bfour potential orbs&0 to serve as your crystal ball.'
  wait 3s
  msend %actor% %self.name% says, 'Unfortunately, I'm not sure which of the four will be best suited to you:
  msend %actor% &0 
  msend %actor% &0- First is a simple &3&bquartz ball&0.
  msend %actor% &0  But be careful!  It is in the possession of a priest of Blackmourne in Anduin, and they are as dangerous as they are homicidal.
  msend %actor% &0  
  msend %actor% &0- Second is an &3&borb of pure Chaos&0.
  msend %actor% &0  It is kept by the Beast of Borgan, which dwells in the Layveran Labyrinth in the Black Ice Desert.
  msend %actor% &0  
  msend %actor% &0- Third is the &3&bOrb of Catastrophe&0.
  msend %actor% &0  Cyprianum the Reaper, the ruler of Demise Keep in the Syric Mountains lays claim to that one.
  msend %actor% &0   
  msend %actor% &0- Fourth is a &3&bglass globe&0 which controls the flow of time.
  msend %actor% &0  The mysterious Time Elemental Lord, stranded in Frost Valley after the Time Cataclysm, carries this one.'
  wait 6s
  msend %actor% %self.name% says, '&6&bGather all four and bring them to the Oracle of Justice&0 at our sister pyramid in the ancient forest near Anduin.  Only he can tell you which is best for you.'
  wait 3s
  msend %actor% %self.name% says, 'Once the proper choice has been determined, bring the final selection back to me.  I will lead you through the full casting process.'
endif 
~
#55045
Wizard Eye Master Shaman receive 4~
0 j 100
55033~
set stage %actor.quest_stage[wizard_eye]%
if %stage% == 11
  wait 2
  quest advance wizard_eye %actor.name%
  emote admires the crystal ball.
  msend %actor% %self.name% says, 'Truly a perfect tool for scrying.  It suits you.  Keep it and treasure it.'
  give crystal-ball %actor.name%
  wait 4s
  msend %actor% %self.name% says, 'Everything is prepared!'
  emote retrieves the poultice, the sachet, and the incense.
  wait 4s 
  msend %actor% %self.name% says, 'First, the poultice.'
  emote scoops up some of the pungent poultice and smears it on the crystal ball.
  wait 4s
  msend %actor% %self.name% says, 'Now your turn %actor.name%'
  msend %actor% %self.name% spreads the stinky poultice across your face.  Yuck!
  mechoaround %actor% %self.name% spreads the stinky poultice across %actor.name%'s face.
  mforce %actor% gag
  wait 4s
  msend %actor% %self.name% says, 'Next the sachet.'
  emote lays out a pillow and places the herbal sachet beneath it.
  wait 4s
  msend %actor% %self.name% says, 'And now, the incense.'
  emote lights the incense.
  msend %actor% The sweet smell of roses and cinnamon fills the chamber.
  wait 4s
  msend %actor% %self.name% says, 'Now, all that's left is for you to &6&bsleep&0, %actor.name%.'
endif
~
$~
