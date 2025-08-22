#13300
bloodline~
0 g 100
~
if %actor.vnum% == -1
say Hello %actor.name%
say How are you today?
wait 2 s
say Good weather yes?
say You know a Bum named doal i hear their is a price on his head
mload obj 13301
give pick %actor.name%
mforce %actor.name% wield pick
endif
~
#13301
raph_greet_queststart~
0 g 100
~
smile %actor.name%
emote tosses an empty canteen across the room.
say Do you have anything for me, or have you already tried to help me once?
wait 2s
say If you have something for me, gimme.  If you have done this quest, go away.
else
say I have been looking for something to eat in this world but to no avail, can you help me?
glare %actor.name%
*this is a quest for group heal for cleric classes, major globe for mages
*and either switch or picklock skill enhancements for other classes
~
#13302
raph_speak_yesno~
0 d 100
yes no~
if %actor.vnum% == -1
if %speech% /= yes
say So you want to help me do you? Well that is useful, maybe I will survive.
quest start get_raph_food %actor.name%
smile
say Please go get me some grain, I am very hungry and may pass on at any time. 
endif
if %speech% /= no
say Fine, let me die get out!
spit %actor.name%
wait 1 s
msend %actor% Raph taps his wrists together and you are covered in smoke!
mechoaround %actor% Raph glares at %actor.name% and sends him elsewhere!
mteleport %actor% 13301
endif
endif
~
#13303
exit_from_raph~
0 d 100
exit exit?~
* to make this generic use %self.name% instead of raph, room is toughter..
growl
say You are too cruel!
msend %actor% Raph taps his wrists together and you are covered in smoke!
mechoaround %actor% Raph glares at %actor.name% and sends him elsewhere!
mteleport %actor% 13301
~
#13304
raph_get_him_donuts~
0 j 100
~
if %actor.vnum%== -1
wait 3
if  %object.vnun% == 16211
if  %actor.quest_stage[get_raph_food]% == 2
emote frowns as his tummy growls like a roar of thunder.
wait 3s
shake %actor.name%
say Thank you for the grain but alas I'm too weak to make it into anything, go and get me a dozen donuts?
quest advance get_raph_food %actor.name%
smile
else
switch %actor.quest_stage[get_raph_food]%
case 1
say How ever did you manage to get the grain without actually picking it up yourself?!
frown %actor.name%
break
default
say What a lovely bit of food, thank you!
thank %actor.name%
wait 2
say Shame you were not performing a quest, you seem like you could have helped an old man.
done
endif
endif
endif
~
#13305
raph_get_donut_want_cheese~
0 j 100
~
if %actor.vnum%== -1
wait 3
if  %object.vnun% == 36303
if  %actor.quest_stage[get_raph_food]% == 4
emote whines as his tummy growls again.
wait 3s
shake %actor.name%
say Thank you for the donuts but sadly I fear I am close too death to enjoy the sweets.  But perhaps if you would get me some cheese... that would really save me.
quest advance get_raph_food %actor.name%
smile
else
switch %actor.quest_stage[get_raph_food]%
case 1
case 2
say Unusual that you can get an object I have not even asked for yet.
wait 2
say You shouldn't know to have this yet, cheater.
break
case 3
say How ever did you manage to get the donuts without actually picking it up yourself?!
frown %actor.name%
break
default
say What a lovely bit of food, thank you!
thank %actor.name%
wait 2
say Shame you were not performing a quest, you seem like you could have helped an old man.
done
endif
endif
endif
~
#13306
raph_recei_cheese_want_mussel~
0 j 100
~
if %actor.vnum%== -1
wait 3
if  %object.vnun% == 16015
if  %actor.quest_stage[get_raph_food]% == 6
emote groans hungrily.
wait 3s
shake %actor.name%
say Thank you for the cheese, but sadly I fear only eating a mussel would save me.  I know I am getting picky, but I hear they have some magical healing properties.
say Go humor an old man and get a mussel for me, so I might see another day.
wait 2s
emote looks sickly.
say I have not had one since I was last down on that sea near those islands.
emote licks his lips hungrily.
quest advance get_raph_food %actor.name%
smile
else
switch %actor.quest_stage[get_raph_food]%
case 1
case 2
case 3
case 4
say Unusual that you can get an object I have not even asked for yet.
wait 2
say You shouldn't know to have this yet, cheater.
break
case 5
say How ever did you manage to get the cheese without actually picking it up yourself?!
frown %actor.name%
break
default
say What a lovely bit of food, thank you!
thank %actor.name%
wait 2
say Shame you were not performing a quest, you seem like you could have helped an old man.
done
endif
endif
endif
~
#13307
raph_recei_mussel~
0 j 100
~
if %actor.vnum%== -1
wait 3s
if  %object.vnun% == 49024
if  %actor.quest_stage[get_raph_food]% == 8
emote smiles a bit as his tummy growls very much.
wait 3
shake %actor.name%
say I thank you for all of your effort, but I am afraid you are too late.  Death is coming for me.
quest advance get_raph_food %actor.name%
tell %actor.name% If you would like your prize, 'tell raph Please'.
wait 2s
else
switch %actor.quest_stage[get_raph_food]%
case 1
case 2
case 3
case 4
case 5
case 6
say Unusual that you can get an object I have not even asked for yet.
wait 2
say You shouldn't know to have this yet, cheater.
break
case 7
say How ever did you manage to get the mussel without actually picking it up yourself?!
frown %actor.name%
break
default
say What a lovely bit of food, thank you!
thank %actor.name%
wait 2
say Shame you were not performing a quest, you seem like you could have helped an old man.
done
wait 2s
endif
endif
endif
~
#13308
raph_get_grain~
1 g 100
~
if %actor.vnum% == -1
if %actor.quest_stage[get_raph_food]% == 1
if %already_retrieved_grain% == 1
oechoaround %actor% The grain flows through %actor.name%'s hand, making a 
pile appear on the floor.
osend %actor% The grain pass between your fingers, scratching you on the way.
odamage %actor% 53
return 0
else
quest advance get_raph_food %actor.name%
endif
endif
endif
set already_retrieved_grain 1
global already_retrieved_grain
~
#13309
raph_get_donuts~
1 g 100
~
if %actor.vnum% == -1
if %actor.quest_stage[get_raph_food]% == 3
if %already_retrieved_donuts% == 1
oechoaround %actor% The donuts crumble in %actor.name%'s hands, turning to dust.
osend %actor% The donuts crumble to dust in your hands, turning to dust.
return 0
else
quest advance get_raph_food %actor.name%
endif
endif
endif
set already_retrieved_donuts 1
global already_retrieved_donuts
~
#13310
raph_get_cheese~
1 g 100
~
if %actor.vnum% == -1
if %actor.quest_stage[get_raph_food]% == 5
if %already_retrieved_cheese% == 1
oechoaround %actor% The cheese turns to a mushy goo in %actor.name%'s hands.
osend %actor% You squished the cheese into goo!
return 0
else
quest advance get_raph_food %actor.name%
endif
endif
endif
set already_retrieved_cheese 1
global already_retrieved_cheese
~
#13311
raph_get_mussel~
1 g 100
~
if %actor.vnum% == -1
if %actor.quest_stage[get_raph_food]% == 7
if %already_retrieved_mussel% == 1
oechoaround %actor% The mussel slips from %actor.name%'s fingers, splatting on the ground.
osend %actor% The mussel slips from your hands, darn slimy things.
return 0
else
quest advance get_raph_food %actor.name%
endif
endif
endif
set already_retrieved_mussel 1
global already_retrieved_mussel
~
#13312
tell_raph_Please~
0 j 100
please Please~
wait 2
if %actor.vnum% == -1
if %actor.quest_stageget_raph_food% == 9
quest complete get_raph_food %actor.name%
if %actor.class% /= Cleric
smile %actor.name%
mskillset %actor.name% 'group heal' 1000
if %actor.level% < 57
tell %actor.name% You will not be able to use your new ability for several levels.
endif
msave %actor%
tell %actor.name% There you go, enjoy your new powers.
elseif %actor.class% /= Priest
smile %actor.name%
mskillset %actor.name% 'group heal' 1000
if %actor.level% < 57
tell %actor.name% You will not be able to use your new ability for several levels.
endif
msave %actor%
tell %actor.name% There you go, enjoy your new powers.
elseif %actor.class% /= Diabolist
smile %actor.name%
mskillset %actor.name% 'group heal' 1000
if %actor.level% < 57
tell %actor.name% You will not be able to use your new ability for several levels.
endif
msave %actor%
tell %actor.name% There you go, enjoy your new powers.
elseif %actor.class% /= Druid
smile %actor.name%
mskillset %actor.name% 'invigorate' 1000
if %actor.level% < 65
tell %actor.name% You will not be able to use your new ability for several levels.
endif
msave %actor%
tell %actor.name% There you go, enjoy your new powers.
elseif %actor.class% /= Sorcerer
smile %actor.name%
mskillset %actor.name% 'major globe' 1000
if %actor.level% < 57
tell %actor.name% You will not be able to use your new ability for several levels.
endif
msave %actor%
tell %actor.name% There you go, enjoy your new powers.
elseif %actor.class% /= Pyromancer
smile %actor.name%
mskillset %actor.name% 'major globe' 1000
if %actor.level% < 57
tell %actor.name% You will not be able to use your new ability for several levels.
endif
msave %actor%
tell %actor.name% There you go, enjoy your new powers.
elseif %actor.class% /= Cryomancer
smile %actor.name%
mskillset %actor.name% 'major globe' 1000
if %actor.level% < 57
tell %actor.name% You will not be able to use your new ability for several levels.
endif
msave %actor%
tell %actor.name% There you go, enjoy your new powers.
elseif %actor.class% /= Necromancer
smile %actor.name%
mskillset %actor.name% 'summon dracolich' 1000
if %actor.level% < 73
tell %actor.name% You will not be able to use your new ability for several levels.
endif
msave %actor%
tell %actor.name% There you go, enjoy your new powers.
elseif %actor.class% /= Warrior
smile %actor.name%
* 
eval total %actor.level% * 10 + 50
skillset %actor.name% 'switch' total
msave %actor%
tell %actor.name% There you go, enjoy your enhanced skills.
elseif %actor.class% /= Thief
smile %actor.name%
* 
eval total %actor.level% * 10 + 50
skillset %actor.name% 'pick lock' total
msave %actor%
tell %actor.name% There you go, enjoy your enhanced skills.
elseif %actor.class% /= Paladin
smile %actor.name%
* 
eval total %actor.level% * 10 + 50
skillset %actor.name% 'switch' total
msave %actor%
tell %actor.name% There you go, enjoy your enhanced skills.
elseif %actor.class% /= Antipaladin
smile %actor.name%
* 
eval total %actor.level% * 10 + 50
skillset %actor.name% 'switch' total
msave %actor%
tell %actor.name% There you go, enjoy your enhanced skills.
elseif %actor.class% /= Ranger
smile %actor.name%
* 
eval total %actor.level% * 10 + 50
skillset %actor.name% 'switch' total
msave %actor%
tell %actor.name% There you go, enjoy your enhanced skills.
elseif %actor.class% /= Assassin
smile %actor.name%
* 
eval total %actor.level% * 10 + 50
skillset %actor.name% 'pick lock' total
msave %actor%
tell %actor.name% There you go, enjoy your enhanced skills.
elseif %actor.class% /= Mercenary
smile %actor.name%
* 
eval total %actor.level% * 10 + 50
skillset %actor.name% 'switch' total
msave %actor%
tell %actor.name% There you go, enjoy your enhanced skills.
elseif %actor.class% /= Monk
smile %actor.name%
* 
eval total %actor.level% * 10 + 50
skillset %actor.name% 'switch' total
msave %actor%
tell %actor.name% There you go, enjoy your enhanced skills.
elseif %actor.class% /= Berserker
smile %actor.name%
* 
eval total %actor.level% * 10 + 50
skillset %actor.name% 'switch' total
msave %actor%
tell %actor.name% There you go, enjoy your enhanced skills.
elseif %actor.class% /= Rogue
smile %actor.name%
* 
eval total %actor.level% * 10 + 50
skillset %actor.name% 'pick lock' total
msave %actor%
tell %actor.name% There you go, enjoy your enhanced skills.
elseif %actor.class% /= Bard
smile %actor.name%
* 
eval total %actor.level% * 10 + 50
skillset %actor.name% 'pick lock' total
msave %actor%
tell %actor.name% There you go, enjoy your enhanced skills.
endif
else
tell %actor.name% What do you want, go about your business.
endif
endif
~
#13321
dwarf-sharpening-pickaxe~
0 b 10
~
mecho &3&bSparks&0 fly as a dwarf runs a sharpening-stone down the length of his pickaxe!
~
#13322
overseer-greet~
0 g 100
~
stomp
The overseer's face turns red with exertion.
say This isn't the first thing that has gone missing!
~
#13323
overseer-missing~
0 n 100
missing?~
fume
say I know I left it in plain sight in the break room!
say There must be a thief about!
glare %actor.name%
~
#13324
overseer-receive~
0 j 100
~
if %object.vnum% == 13304
wait 1s
blink
say This is mine.  It was stolen.
wait 1s
mecho %self.name% inspects the pickaxe carefully.
frown
say It smells like lizard, so you couldn't have stolen it.
wait 2s
shrug
say I am in your debt for returning it.
mload obj 13305
bow %actor.name%
wait 1s
say Please accept these as a token of my gratitude.
give goggles %actor.name%
wait 1s
wield overseer-pickaxe
mecho %self.name% gets back to work.
else
frown
say No, that's not what's gone missing.
say Youngsters these days all have rocks for brains.
endif
~
#13325
heavens_gate_starlight_greet~
0 h 100
~
set stage %actor.quest_stage[heavens_gate]%
msend %actor%   
msend %actor% &7&bSoft starlight glitters throughout the cavern in welcome as you enter.&0
if %actor.class% /= Priest && %actor.level% > 80
  if %stage% == 0
    msend %actor%   
    msend %actor% &7&bA strange light glints off a strange rock formation near the center of the cavern.&0
    if %get.obj_count[13350]% == 0
      mload obj 13350
    endif
  elseif %stage% == 1
    if %get.obj_count[13350]% == 0
      mload obj 13350
    endif
    msend %actor% &7&b%self.name% coalesces into the ephemeral shape of a bent old woman, standing before the rock formation.&0
    msend %actor% &7&bSwirling stars form the shape of a bowl in her hands, which she puts on the rock pedestal.&0
    msend %actor%   
    msend %actor% &7&bThe crone invites you to &6&b[commune] &7&bfor visions of your progress.&0
  elseif %stage% == 2
    if %get.obj_count[13350]% == 0
      mload obj 13350
    endif
    msend %actor% &7&bThe floating apparition of a valiant young knight constructed from starbeams hovers over the pedestal.&0
    msend %actor% &7&bHe seems to invite you to &6&b[put] &7&bsomething on the &7&b[pedestal]&7&b.&0
    msend %actor%   
    msend %actor% &7&bThe knight invites you to &6&b[commune] &7&bfor visions of your progress.&0
  elseif %stage% == 3
    msend %actor% &7&bA starry serpent slithers through the rocky cavern.&0
    msend %actor% &7&bIt looks at you with expectant, curious eyes.&0
    msend %actor%   
    msend %actor% &7&bThe serpent invites you to &6&b[commune] &7&bfor visions of your progress or a new Key.&0
  elseif %stage% == 4
    wait 1s
    msend %actor% &7&bThe starlight manifests as the heavenly raven again.&0
    wait 2s
    msend %actor% This time, at last, it speaks:&_
    msend %actor% &6&b'I I I I am the book.  Open me prophet; read; decypher.&0
    msend %actor% &6&bOn you, in you, in your blood, they write, have written.&0
    msend %actor% &6&bSpeak it but aloud to know the path of heaven for I I I I am the final key.&0
    msend %actor% &6&bI I I I have shown you visions, and through me you shall read.'&0
    msend %actor% 
    msend %actor% &3yamo lv soeeiy vrtvln eau okia khz lrrvzryp gvxrj bzjbie hi&0
  endif
endif
~
#13326
heavens_gate_starlight_look~
1 m 100
~
if %actor.class% /= Priest && %actor.level% > 80 && %actor.quest_stage[heavens_gate]% == 0
  oechoaround %actor% %actor.name% stands completely still, staring intently at nothing.
  wait 1s
  osend %actor% &7&bYour mind is suddenly overwhelmed by the vision of a swirling vortex of stars above the formation.&0
  osend %actor% &7&bIt opens a gateway through the heavens!&0
  wait 4s
  osend %actor% &7&bSwirls of galaxies and flares of stars begin to meld within the vortex.&0
  osend %actor% &7&bThey form a cosmic raven, which soars down to greet you.&0
  wait 4s
  osend %actor% &7&bIn dance-like flight, it mimics a priest, bent over the rock pedestal in prayer.&0
  osend %actor% &7&bThe starlight glistens in the shape of a bowl on the rock.&0
  wait 4s
  osend %actor% &7&bThe raven finishes the dance and melds with the bowl.&0
  osend %actor% &7&bYou find yourself gazing into a bowl with the symbol of a raven outlined in silver.&0
  osend %actor% &7&bIts wings outstretched with feathers and talons marked with thin lines.&0
  wait 4s
  osend %actor% &7&bThe bowl seems to promise ancient sacred knowledge if only it could be brought to this place!&0
  wait 4s
  osend %actor% &7&bThe raven invites you to &6&b[commune] &7&bto receive visions of your progress.&0
  wait 2s
  osend %actor% Returning to reality, you find yourself staring at the rock pedestal.
  quest start heavens_gate %actor.name%
endif
~
#13328
heavens_gate_pedestal_put~
1 h 100
~
switch %target.vnum%
  case 23817
    if %actor.quest_stage[heavens_gate]% == 1
      wait 2
      quest advance heavens_gate %actor.name%
      opurge bowl
      wait 2
      oecho Starlight settles on the burnished silver of %get.obj_shortdesc[23817]%.
      wait 1s
      osend %actor.name% &7&bFor a moment, all is calm, until the starlight crone rushes at you!&0
      wait 2s
      osend %actor% &7&bYou receive a vision of seven keys to seven gates &6&b[put] &7&bupon the &6&b[pedestal]&0.
      wait 1s
      osend %actor% &7&bOne by one you receive a vision of each key:&0&_
      osend %actor% &6&b1. A small skeleton key forged of night and shadow&0
      osend %actor% &0   hidden deep in a twisted labyrinth.&0&_
      osend %actor% &6&b2. A key made from a piece of the black and pitted wood&0
      osend %actor% &0   typical of trees in the Twisted Forest near Mielikki.&0&_
      osend %actor% &6&b3. A large, black key humming with magical energy from&0
      osend %actor% &0   a twisted cruel city in a huge underground cavern.&0&_
      osend %actor% &6&b4. A key covered in oil&0
      osend %actor% &0   kept by a long-dead caretaker in a necropolis.&0&_
      osend %actor% &6&b5. A rusted but well cared for key&0
      osend %actor% &0   carried by an enormous griffin.&0&_
      osend %actor% &6&b6. A golden plated, wrought-iron key&0
      osend %actor% &0   held at the gates to a desecrated city.&0&_
      osend %actor% &6&b7. One nearly impossible to see&0
      osend %actor% &0   guarded by a fiery beast with many heads.&0
      wait 1s
      osend %actor% &7&bEach key transforms into a star forming a circle in the sky.&0
      osend %actor% &7&bThey open some kind of gateway, leading into the unknown.&0
      wait 1s
      osend %actor% &7&bThe starlight crone gestures to the pedestal before dissipating into the dark.&0
    else
      set response default
    endif
    break
  case 4005
  case 12142
  case 23709
  case 47009
  case 49008
  case 52012
  case 52013
    if %actor.quest_stage[heavens_gate]% == 2
      if %actor.quest_variable[heavens_gate:%target.vnum%]%
        return 0
        osend %actor% The star knight refuses to take a second copy of the key.
        halt
      else
        quest variable heavens_gate %actor.name% %target.vnum% 1
        wait 1
        opurge %self.contents%
        wait 2
        set key1 %actor.quest_variable[heavens_gate:4005]%
        set key2 %actor.quest_variable[heavens_gate:12142]%
        set key3 %actor.quest_variable[heavens_gate:23709]%
        set key4 %actor.quest_variable[heavens_gate:47009]%
        set key5 %actor.quest_variable[heavens_gate:49008]%
        set key6 %actor.quest_variable[heavens_gate:52012]%
        set key7 %actor.quest_variable[heavens_gate:52013]%
        if (%key1% + %key2% + %key3% + %key4% + %key5% + %key6% + %key7%) == 7
          quest advance heavens_gate %actor.name%
          wait 1s
          osend %actor% &7&bThe starry knight lifts the seven keys into the air.&0
          wait 2s
          osend %actor% &7&bThe keys begin to glow and swirl around one another.&0
          wait 2s
          osend %actor% &7&bThey fuse together into a single shining electrum key!&0
          oload obj 13351
          oforce %actor% get key
          wait 1s
          osend %actor% &7&bThe valiant star knight reveals to you seven anomalies that threaten Ethilien.&0
          wait 1s
          osend %actor% &7&bYou see:&0
          osend %actor% &0&6&b1. A pool in a temple of ice and stone&0
          osend %actor% &0   leading to the realm of a death god.&0&_
          osend %actor% &0&6&b2. A pool in a temple of ice and stone&0
          osend %actor% &0   leading to the realm of a war god.&0&_
          osend %actor% &0&6&b3. A pool hidden under a well&0
          osend %actor% &0   on an island filled with ferocious beasts.&0&_
          osend %actor% &0&6&b4. A portal from black rock&0
          osend %actor% &0   to black ice.&0&_
          osend %actor% &0&6&b5. A portal from a fortress of the undead&0
          osend %actor% &0   to a realm of demons.&0&_
          osend %actor% &0&6&b6. An archway that delivers demons&0
          osend %actor% &0   to the fortress of the dead.&0&_
          osend %actor% &0&6&b7. An arch hidden in another plane&0
          osend %actor% &0   granting demons access to an enchanted village of mutants.&0
          wait 1s
          osend %actor% &7&bUnintelligible words burble forth from the rifts... &0
          wait 2s
          osend %actor% &7&bIn your soul you hear the call to take the Key of Heaven and &6&bseal &7&bthe &6&brifts!&0
          opurge %self%
        else
          osend %actor% &7&bThe starlight twinkles in recognition of your deed!&0&_ &_
          osend %actor% &7&bIt places the key in the bowl and beckons you to bring another.&0
        endif
      endif
    else
      set response default
    endif
    break
  default
    set response default
done
if %response% == default
  osend %actor% Nothing happens.
endif
~
#13329
heavens_gate_key_seal~
1 c 3
seal~
if %arg% /= rift || %arg% /= portal || %arg% /= pool || %arg% /= arch
    set seal %actor.quest_variable[heavens_gate:sealed]%
    if %self.room% == 51077 || %self.room% == 16407 || %self.room% == 16094 || %self.room% == 55735 || %self.room% == 49024 || %self.room% == 55126 || %self.room% == 55112
      if %actor.quest_variable[heavens_gate:%self.room%]%
        osend %actor% You have already sealed this anomaly.
      else
        if %actor.quest_stage[heavens_gate]% == 3
          quest variable heavens_gate %actor.name% %self.room% 1
          osend %actor% You begin to chant...
          oechoaround %actor% %actor.name% begins to chant...
          osend %actor% The power of the heavens courses through %self.shortdesc%.
          wait 2s
          oecho %self.shortdesc% begins to burn with a fierce energy!
          wait 2s
          oecho Brilliant rays of light shoot out of %self.shortdesc%, sealing the dimensional portal!
          if %self.room% == 51077
            opurge arch
            oteleport all 51003
          elseif %self.room% == 16407
            opurge arch
          elseif %self.room% == 49024 || %self.room% == 55126 || %self.room% == 55112
            opurge energy
          elseif %self.room% == 16094 || %self.room% == 55735
            opurge portal
          endif
          eval sealed %seal% + 1
          quest variable heavens_gate %actor.name% sealed %sealed%
          wait 1s
          switch %sealed%
            case 1
              osend %actor% As the rift collapses, words float up in your mind:
              osend %actor% &7&byamo lv&0 
              break
            case 2
              osend %actor% As the rift collapses, words float up in your mind:
              osend %actor% &7&byamo lv soeeiy&0 
              break
            case 3
              osend %actor% As the rift collapses, words float up in your mind:
              osend %actor% &7&byamo lv soeeiy vrtvln&0 
              break
            case 4
              osend %actor% As the rift collapses, words float up in your mind:
              osend %actor% &7&byamo lv soeeiy vrtvln eau okia khz&0 
              break
            case 5
              osend %actor% As the rift collapses, words float up in your mind:
              osend %actor% &7&byamo lv soeeiy vrtvln eau okia khz lrrvzryp&0 
              break
            case 6
              osend %actor% As the rift collapses, words float up in your mind:
              osend %actor% &7&byamo lv soeeiy vrtvln eau okia khz lrrvzryp gvxrj&0 
              break
            case 7
              osend %actor% As the rift collapses, words float up in your mind:
              osend %actor% &7&byamo lv soeeiy vrtvln eau okia khz lrrvzryp gvxrj bzjbie hi&0
              wait 2s
              set room %get.room[13358]%
              osend %actor% &7&bA vision of starlight beckons you back to %room.name%.&0
              quest advance heavens_gate %actor.name%
              break
            default
              return 0
          done
        endif
      endif
    else
      osend %actor% There are no active rifts here to seal.
    endif
else
  return 0
endif
~
#13330
heavens_gate_starlight_speech1~
0 d 0
Hark ye starry angels and open the heavenly gates before me~
if %actor.quest_stage[heavens_gate] == 4
  wait 1s
  mecho &7&bStarlight spreads wide across the cavern.&0
  mecho &6A &btunnel of &7light &6opens up!&0
  wait 3s
  msend %actor% &7&bFrom the reaches of the heavens some unknowable entity touches your soul.&0
  msend %actor% &7&bIt imparts a prayer to you.&0
  msend %actor%  
  mskillset %actor.name% heavens gate 
  quest complete heavens_gate %actor.name%
  msend %actor% &6&bYou have learned &7Heavens Gate&6!&0
  wait 2s
  mecho &7&bThe starlight pours into the &6&btunnel &7&band is gone!&0
  mpurge %self%
endif
~
#13331
heavens_gate_starlight_status_checker~
0 c 100
commune~
switch %cmd%
  case c
  case co
  case com
  case comm
    return 0
    halt
done
set stage %actor.quest_stage[heavens_gate]%
if %stage% > 0 || %actor.has_completed[heavens_gate]%
  msend %actor% You extend your awareness to the stars above.
  mechoaround %actor% %actor.name% begins to commune with the beam of starlight.
  wait 2s
  if %stage% == 1
    msend %actor% &7&bYou receive a vision of a silver prayer bowl brought to this cavern and &6&b[put] &7&bon the &6&b[pedestal]&7&b.&0
  elseif %stage% == 2
    set key1 %actor.quest_variable[heavens_gate:4005]%
    set key2 %actor.quest_variable[heavens_gate:12142]%
    set key3 %actor.quest_variable[heavens_gate:23709]%
    set key4 %actor.quest_variable[heavens_gate:47009]%
    set key5 %actor.quest_variable[heavens_gate:49008]%
    set key6 %actor.quest_variable[heavens_gate:52012]%
    set key7 %actor.quest_variable[heavens_gate:52013]%
    msend %actor% You receive a vision of seven keys to seven gates brought to the pedestal.
    mecho   
    if %key1% || %key2% || %key3% || %key4% || %key5% || %key6% || %key7%
      msend %actor% You have returned:
      if %key1%
        msend %actor% %get.obj_shortdesc[4005]%
      endif
      if %key2%
        msend %actor% %get.obj_shortdesc[12142]%
      endif
      if %key3%
        msend %actor% %get.obj_shortdesc[23709]%
      endif
      if %key4%
        msend %actor% %get.obj_shortdesc[47009]%
      endif
      if %key5%
        msend %actor% %get.obj_shortdesc[49008]%
      endif
      if %key6%
        msend %actor% %get.obj_shortdesc[52012]%
      endif
      if %key7%
        msend %actor% %get.obj_shortdesc[52013]%
      endif
    endif
    mecho   
    msend %actor% In your mind, you see images of:
    if !%key1%
      msend %actor% &6&bA small skeleton key forged of night and shadow&0
      msend %actor% &0hidden deep in a twisted labyrinth.&0
      msend %actor%   
    endif
    if !%key2%
      msend %actor% &6&bA key made from a piece of the black and pitted wood&0
      msend %actor% &0typical of trees in the Twisted Forest near Mielikki.&0
      msend %actor%   
    endif
    if !%key3%
      msend %actor% &6&bA large, black key humming with magical energy&0
      msend %actor% &0from a twisted cruel city in a huge underground cavern.&0
      msend %actor%   
    endif
    if !%key4%
      msend %actor% &6&bA key covered in oil&0
      msend %actor% &0kept by a long-dead caretaker in a necropolis.&0
      msend %actor%   
    endif
    if !%key5%
      msend %actor% &6&bA rusted but well cared for key&0
      msend %actor% &0carried by an enormous griffin.&0
      msend %actor%   
    endif
    if !%key7%
      msend %actor% &6&bA golden plated, wrought-iron key&0
      msend %actor% &0held at the gates to a desecrated city.&0
      msend %actor%   
    endif
    if !%key6%
      msend %actor% &6&bOne nearly impossible to see&0
      msend %actor% &0guarded by a fiery beast with many heads.&0
    endif
  elseif %stage% == 3
    set sealed %actor.quest_variable[heavens_gate:sealed]%
    set seal1 %actor.quest_variable[heavens_gate:51077]%
    set seal2 %actor.quest_variable[heavens_gate:16407]%
    set seal3 %actor.quest_variable[heavens_gate:16094]%
    set seal4 %actor.quest_variable[heavens_gate:55735]%
    set seal5 %actor.quest_variable[heavens_gate:49024]%
    set seal6 %actor.quest_variable[heavens_gate:55126]%
    set seal7 %actor.quest_variable[heavens_gate:55112]%
    msend %actor% Visions of seven rifts in the fabric of reality which you must &6&bseal&0 float up in your mind.
    if %seal1% || %seal2% || %seal3% || %seal4% || %seal5% || %seal6% || %seal7%
      msend %actor%   
      msend %actor% You have already sealed the rifts in:
      if %seal1%
        * Nordus
        set room %get.room[51077]%
        msend %actor% %room.name% 
      endif
      if %seal2%
        * Mystwatch demon
        set room %get.room[16407]%
        msend %actor% %room.name%
      endif
      if %seal3%
        * Mystwatch fortress
        set room %get.room[16094]%
        msend %actor% %room.name%
      endif
      if %seal4%
        * Black rock trail
        set room %get.room[55735]%
        msend %actor% %room.name%
      endif
      if %seal5%
        * Griffin
        set room %get.room[49024]%
        msend %actor% %room.name%
      endif
      if %seal6%
        * Huitzipia - war
        set room %get.room[55126]%
        msend %actor% %room.name%
      endif
      if %seal7%
        * Xapizo - death
        set room %get.room[55112]%
        msend %actor% %room.name%
      endif
    endif
    msend %actor%   
    msend %actor% You see visions of:
    if !%seal1%
      msend %actor% &6&bAn arch hidden in another plane&0
      msend %actor% &0granting demons access to an enchanted village of mutants.&0
    endif
    if !%seal2%
      msend %actor%   
      msend %actor% &6&bAn archway that delivers demons&0
      msend %actor% &0to the fortress of the dead.&0
    endif
    if !%seal3%
      msend %actor%   
      msend %actor% &6&bA portal from a fortress of the undead&0
      msend %actor% &0to a realm of demons.&0
    endif
    if !%seal4%
      msend %actor%   
      msend %actor% &6&bA portal from black rock&0
      msend %actor% &0to black ice.&0
    endif
    if !%seal5%
      msend %actor%   
      msend %actor% &6&bA pool hidden under a well&0
      msend %actor% &0on an island filled with ferocious beasts.&0
    endif
    if !%seal6%
      msend %actor%   
      msend %actor% &6&bA pool in a temple of ice and stone&0
      msend %actor% &0leading to the realm of a war god.&0
    endif
    if !%seal7%
      msend %actor%   
      msend %actor% &6&bA pool in a temple of ice and stone&0
      msend %actor% &0leading to the realm of a death god.&0
    endif
    switch %sealed%
      case 0
        return 0
        break
      case 1
        set phrase yamo lv 
        break
      case 2
        set phrase yamo lv soeeiy
        break
      case 3
        set phrase yamo lv soeeiy vrtvln
        break
      case 4
        set phrase yamo lv soeeiy vrtvln eau okia khz 
        break
      case 5
        set phrase yamo lv soeeiy vrtvln eau okia khz lrrvzryp
        break
      case 6
        set phrase yamo lv soeeiy vrtvln eau okia khz lrrvzryp gvxrj
        break
      case 7
        set phrase yamo lv soeeiy vrtvln eau okia khz lrrvzryp gvxrj bzjbie
        break
      default
        return 0
        halt
    done
    if %sealed%
      msend %actor%   
      msend %actor% Words float up in your mind: &6&b%phrase%&0
    endif
    mecho    
    msend %actor% If you need a new Key, beseech the starlight, &6&b"Grant me a new key"&0.
  elseif %stage% == 4
    msend %actor% The starlight manifests as the heavenly raven again.
    msend %actor% This time, at last, it speaks:&_
    msend %actor% &6&b'I I I I am the book.  Open me prophet; read; decypher.&0
    msend %actor% &6&bOn you, in you, in your blood, they write, have written.&0
    msend %actor% &6&bSpeak it but aloud to know the path of heaven for I I I I am the final key.&0
    msend %actor% &6&bI I I I have shown you visions, and through me you shall read.'&0&_
    msend %actor% &3yamo lv soeeiy vrtvln eau okia khz lrrvzryp gvxrj bzjbie hi&0
  elseif %actor.has_completed[heavens_gate]%
    msend %actor% &6&bYou feel you have learned all you can from this place.&0
  endif
else
  return 0
endif
~
#13332
**UNUSED**~
0 c 100
commun~
return 0
~
#13333
heavens_gate_key_replacement~
0 d 0
grant me a new key~
if %actor.quest_stage[heavens_gate]% == 3
  wait 2
  if %actor.inventory[13351]%
    msend %actor% &6&bYou already possess a Key of Heaven.&0
  else
    mecho &7&bThe stars coalesce into a shining key!&0
    mload obj 13351
    give key-heaven %actor%
  endif
endif
~
$~
