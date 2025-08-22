#3000
Receptionist_no_item_trigger~
0 j 100
~
return 0
msend %actor% %self.name% says, 'I'm sorry, but I cannot accept this.'
msend %actor% %self.name% refuses your item.
~
#3001
Drunk_Socail_stunts1~
0 b 70
~
set antic %random.4%
switch %antic%
  case 1
    sing
    dance drunk
    say Good day to you.
    wave
    break
  case 2
    ponder
    say I uh..
    wait 2
    say Er wonder where I put that beeeer...
    giggle
    break
  case 3
    sing
    hiccup
    say I used to be a big cheese round here you know.
    hiccup
    break
  default
    drink drunkdrink
    burp
    emote wipes his mouth.
    say Aaaahhhh thas the good stuff...
done    
~
#3002
MOB_RAND_SOCIAL_HICCUP~
0 b 14
~
hic
~
#3003
recep_vis_trig~
2 g 100
rent~
wait 2
if %self.people[count]%
   wforce recep vis
   wait 2s
endif
~
#3004
Enter Illusionist Guild Mielikki~
2 c 100
south~
return 1
wdoor 3064 south room 3207
wforce %actor% south
wdoor 3064 south purge
~
#3005
Illusionists sense illusion~
2 g 100
~
wait 1
if %actor.class% /= Illusionist
   wsend %actor% &5You sense the magic of illusion at work upon your senses.&0
end
~
#3006
Archmage responds to 'guild'~
0 n 100
guild~
wait 1 s
if %actor.class% /= Sorcerer
   smirk
   wait 3 s
   ruffle %actor.name%
elseif %actor.class% /= Cryomancer || %actor.class% /= Pyromancer
   say Silly bun!  It's right outside!
   wait 3 s
   pat %actor.name%
elseif %actor.class% /= Necromancer
   say Why, you're looking for creepy old Asiri!
   wait 2 s
   nod
   wait 3 s
   say I suppose he's around here somewhere.  Somewhere creepy!  I don't really know.
elseif %actor.class% /= Illusionist
   say Oh dear.  You're asking the wrong spellcaster.  You illusionists, so good at hiding.
   wait 4 s
   say But there is one thing.  Old Eamus - Ermie?  What was his name?
   wait 3 s
   ponder
   wait 3 s
   say Well anyway, he'd always head to the bank after tea.  But he didn't go into the bank.
   wait 4 s
   say Come to think of it, you illusionists are always sneaking off to the bank.   Or not.
   say Oh, I'm so confused!
else
   sigh
   wait 3 s
   say I'm so sorry, dearie, I've no earthly idea where your guild is.
end
~
#3007
Archmage responds to 'necromancer'~
0 n 100
necromancer~
wait 1 s
if %actor.class% /= Sorcerer && %actor.level% > 9 && %actor.level% < 46
   say So you want to take on the dark arts, eh?  The foul arts?  The stinky death arts?
   wait 3 s
   sniff
   wait 3 s
   say I suppose there's no accounting for taste.  You might want to find a pungent necromancer to help with this endeavour.  If you absolutely must.
   wait 4 s
   say Or a diabolist, even.  Seriously.  They're all the same, painted dark purple and wearing old umbrellas.
   wait 4 s
   say Yes, there IS a necromancer nearby.  I should think the odor would have given him away to you long ago.
   wait 4 s
   say Go look around the creepy nasty parts of town.  No, NOT underground!  This is Asiri we're talking about here!
   say He's vain and no doubt sips blood in a musty parlour, but he won't stand for mold.
   wait 4 s
   say I suppose a run-down shack of some kind would suit him.  I've never been to visit.  Never will.
elseif %actor.class% /= Sorcerer && %actor.level% < 10
   say You want to know about evil, scheming death mages?  A fine young sorcerer like yourself?
   wait 4 s
   say I canNOT imagine why.  They lead frightful lives.  Entertained and served by corpses.
   wait 3 s
   shudder
   wait 3 s
   say At any rate, Asiri won't do a thing for you at your age.  So you can just put the idea out of your mind.
   wait 4 s
   say Preferably forever.
elseif %actor.class% /= Necromancer
   msend %actor% %self.name% looks you over.
   mechoaround %actor% %self.name% looks %actor.name% over.
   wait 4 s
   say Indeed.  I suppose you are.  Well.
   wait 4 s
   say It can't be helped, can it?  You're... well, you're what you are, aren't you.
   wait 4 s
   say If you're looking for your guild, I can only assure you that it's nowhere nearby.
   wait 4 s
   say Well, it can't be TOO far away, as I can smell the rot of old Asiri.  It drifts over half the town.
   wait 3 s
   gag
   wait 3 s
   say Why don't you look for a few undead folks?  Asiri seems to discard as many corpses as he employs.
else
   say You smell it, too?
   wait 4 s
   say I'm going to start a petition to have Asiri evicted from our fine town.
   wait 4 s
   say I hope I can count on you to sign promptly.
   wait 4 s
   eyebrow %actor.name%
end
~
#3008
Archmage responds to 'illusionist'~
0 n 100
illusionist~
wait 1 s
if %actor.class% /= Sorcerer && %actor.level% > 9 && %actor.level% < 46
   say If you want to become an illusionist, you might want to talk to the Grand Master in the citadel.
   wait 3 s
   say Unfortunately, he has been in a foul mood of late.  He may not agree to help you.  But I know of no one else.
elseif %actor.class% /= Sorcerer && %actor.level% < 10
   say Illusionists?  Crafty folk, you can never be quite sure what's going on around them.
   wait 3 s
   say You could become one if you wanted, but you'll have to wait until you have achieved level 10.
elseif %actor.class% /= Illusionist
   say Are you looking for the guild?  I hear that they place crafty illusions over the entrances.  But only visual ones...
   wait 3 s
   say They always seem to be near banks, as well.  Sorry, I don't know anything more about them.
else
   say I always used to take along an illusionist when adventuring.  You never know when they'll come in handy!
end
~
#3009
Erasmus responds to "charm person"~
0 n 100
charm~
wait 2 s
if %actor.class% /= Illusionist
   if %actor.skill[charm person]% == 100
      say A nice spell, isn't it?  I hope you're enjoying it.
   elseif %actor.level% < 17
      say Ah, charm person.  A powerful spell, capable of great evil - or good.
      wait 2 s
      say Come back when you have a bit more experience, and I'll teach it to you.
   else
      say Oh, the charm person spell.  You're ready to learn that one!
      wait 2 s
      say Stand still there while I teach you.
      wait 2 s
      emote launches into a marathon of lectures and speeches about diverse arcana.
      wait 5 s
      emote is still spewing forth obscure knowledge at a furious rate!
      wait 5 s
      emote seems to be winding down, as if the lesson were almost over.
      wait 5 s
      if %actor.room% == %self.room%
         say I'm impressed with your attentiveness, %actor.name%.
         wait 3 s
         say I can see that you've learned charm person rather well.
         mskillset %actor% charm person
      else
         say Eh?  Where did %actor.name% go?  Must not have a thirst for knowledge!
      end
   end
elseif %actor.class% /= Sorcerer
   if %actor.skill[charm person]% == 100
      gasp
      wait 3 s
      say What's this?  An ordinary sorcerer with charm person?!
      wait 5 s
      say Will wonders never cease!
      wait 5 s
      shake
   else
      say Don't look at me!  I'm a legendary teacher, but some things are just beyond me!
      wait 5 s
      say You sorcerers require extra help learning this... rather simple spell.
      wait 5 s
      say And you'll get none of it for me, because I have better things to do.
   end
else
   sigh
   wait 3 s
   say Yes, yes, a wonderful spell, how incredible we illusionists are, certainly.
   wait 3 s
   roll
   wait 3 s
   say Why don't you go bash some poor rodents' heads in.  Or whatever it is you do.
end
~
#3010
Baked with love~
1 s 100
~
Nothing.
~
#3011
lets do get help~
0 k 50
~
roar
wait 2
em yells out for assistance from anyone that can help!
wait 2
mload mob 3052
mecho A half-elven town guard arrives so quickly, you're not sure what direction he came from.
~
#3020
Rent block for inn bar~
0 c 100
rent~
switch %cmd%
  case r
  case re
    return 0
    halt
done
tell %actor% If you want to rent a room, please go upstairs to the Reception Area.
~
#3025
Magistrate_intro~
0 g 100
~
if %actor.vnum% == -1
  wait 5
  msend %actor% %self.name% says to you, 'Greetings, %actor.name%.  It is good to see you.  If you would like, I have some &6&b[quests]&0 you could give me a hand with.'
  wait 1s
  if (%actor.quest_stage[phase_mace]% == 1 || (!%actor.quest_stage[phase_mace]% && (%actor.wearing[340]% || %actor.inventory[340]%))) && !%actor.quest_variable[phase_mace:greet]%
    msend %actor% %self.name% says, 'Oh and you've found quite an unusual mace!  I could probably help you &6&b[upgrade]&0 it.'
  elseif %actor.quest_stage[phase_mace]% == 1 && %actor.quest_variable[phase_mace:greet]%
    msend %actor% %self.name% says, 'Have you found what I require?'
  endif
endif
~
#3026
test-random~
0 d 100
random~
* This is a test to generate a random number to be used
* in many ways
say My trigger commandlist is not complete!
set random_number %random.100%
if %random_number% >=51
say We're loading object.
else
say We're not loading object.
endif
say %random_number%
set mob %self.mexists[3055]%
set obj %self.oexists[1127]%
msend %actor% There are %mob% Druidic guards of 3055 in the game.
msend %actor% There are %obj% iron-banded girth's in the game.
~
#3027
quest_banter_magistrate1~
0 d 1
quests quest assist help~
wait 2
if %actor.level% < 30
  msend %actor% %self.name% tells you, 'There is a temple just outside the West Gate
  msend %actor% &0of town, dedicated to the Kaaz clan, the great heroes of the Rift Wars.  
  msend %actor% Mystics from all around Caelia go there hoping to find some kind of special
  msend %actor% &0power.'
  wait 1s
  msend %actor% %self.name% tells you, 'It seems they found something.'
else
  msend %actor% %self.name% tells you, 'I fear that I have little time to banter, for
  msend %actor% &0as you can see, I must prepare our defenses for yet another attack.'
endif
~
#3028
quest_banter_magistrate2~
0 d 100
attack attacks~
wait 2
mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
msend %actor% %self.name% says to you, 'Yes yes..  That accursed Demon Lord who holds
msend %actor% &0dominion over the northern fortress of Mystwatch, and his fat headed general
msend %actor% &0too.'
sigh
wait 2
msend %actor% %self.name% says to you, 'If only we could be rid of them once and for
msend %actor% &0all, maybe these attacks would stop.  Will you help rid us of this curse?'
~
#3029
quest_banter_magistrate3~
0 d 100
yes~
if %actor.vnum% == -1
  wait 1
  * If any of the quest guys have spawned in Mystwatch don't give another totem.
  if %get.mob_count[16008]% || %get.mob_count[16010]% || %get.mob_count[16011]% || %get.mob_count[16015]% || %get.mob_count[16016]% || %get.mob_count[16017]% || %get.mob_count[16018]% || %get.mob_count[16019]%
    mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
    msend %actor% %self.name% says to you, 'Splendid, however, someone is currently after the Demon Lord's hide.  You will have to wait until they are finished or if they fail you can finish for them.'
  else
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
        if !%person.quest_stage[mystwatch_quest]%
          quest start mystwatch_quest %person.name%
          msend %person% &7&bYou have begun the Mystwatch quest!&0
        endif
        quest variable mystwatch_quest %actor.name% step totem
      elseif %person%
        eval i %i% + 1
      endif
      eval a %a% + 1
    done
    mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
    msend %actor% %self.name% says to you, 'Splendid, Mielikki be praised that one valiant enough has come amongst us to help rid us of this nuisance.'
    wait 2
    think
    msend %actor% %self.name% says to you, 'Here.  Give this to that rat general in Mystwatch and it should start you on your way.'
    mload obj 3026
    give totem %actor.name%
  endif
endif
~
#3030
Myst_quest_reward~
0 j 100
16023~
wait 1
mjunk shard
* Create a random number by which to judge prize table off of.
set rnd %random.100%
if %rnd% >= 90
  mload obj 401
elseif %rnd% < 90 && %rnd% >= 70
  mload obj 3040
elseif %rnd% < 70 && %rnd% >= 30
  switch %random.19%
    case 1
      * cure crit x3
      mload obj 8342
      break
    case 2
      * strength
      mload obj 3274
      break
    case 3
      * wisdom
      mload obj 3278
      break
    case 4
      * monk fire
      mload obj 3249
      break
    case 5
      * monk acid
      mload obj 3258
      break
    case 6
      * bless, remove curse, cure serious
      mload obj 11701
      break
    case 7
      * prot evil, enhance con, remove poison
      mload obj 11702
      break
    case 8
      * prot evil, enhance wis
      mload obj 17808
      break
    case 9
      * prot cold, monk ice
      mload obj 3241
      break
    case 10
      * monk lightning
      mload obj 3255
      break
    case 11
      * monk ice
      mload obj 3252
      break
    case 12
      * prot cold
      mload obj 3265
      break
    case 13
      * prot fire
      mload obj 3262
      break
    case 14
      * prot air
      mload obj 3268
      break
    case 15
      * prot earth
      mload obj 3271
      break
    case 16
      * intelligence
      mload obj 3282
      break
    case 17
      * charisma
      mload obj 3286
      break
    case 18
      * constitution
      mload obj 3290
      break
    case 19
      * dexterity
      mload obj 3294
      break
    default
      Say This quest reward has a broken potion random chance, let a god know.
  done
elseif %rnd% < 30
  mload obj 3041
else
  say The reward for this quest is broken, contact a god and let them know.
endif
shout Hark all and listen, %actor.name% has fended off a great evil for our fair town!
wait 1
say Here, %actor.name% you have earned this as a reward.
wait 1
give all %actor.name%
if !%actor.has_completed[mystwatch_quest]% && %actor.quest_variable[mystwatch_quest:step]% == complete
  quest complete mystwatch_quest %actor.name%
  if %actor.level% < 50
    set expcap %actor.level%
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
  switch %actor.class%
    case Warrior
    case Berserker
      eval expmod (%expmod% + (%expmod% / 10))
      break
    case Paladin
    case Anti-Paladin
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
      eval expmod (%expmod% + ((%expmod% * 2) / 5))
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
endif
if %get.mob_count[16007]% < 1
  mat 16064 mload mob 16007
endif
~
#3031
Phase Mace Templar speech upgrade~
0 d 100
upgrade upgrades upgrading improvement improvements blessing blessings~
wait 2
if %actor.level% > 9
  if %actor.quest_stage[phase_mace]% == 1 || (!%actor.quest_stage[phase_mace]% && (%actor.wearing[340]% || %actor.inventory[340]%))
    if !%actor.quest_stage[phase_mace]%
      quest start phase_mace %actor%
    endif
    quest variable phase_mace %actor% greet 1
    msend %actor% %self.name% says, 'I could bless your mace against the undead, if I had the proper materials.'
    msend %actor% &0 
    msend %actor% Bring me the following:
    msend %actor% - &3&b%get.obj_shortdesc[55211]%&0 to use as a model
    msend %actor% - &3&b%get.obj_shortdesc[55577]%&0 and
    msend %actor% - &3&b%get.obj_shortdesc[13614]%&0 for their protection against malevolent spirits
    msend %actor% - &3&b%get.obj_shortdesc[58809]%&0 as a flame to ward against the dark
    msend %actor% &0 
    msend %actor% Also attack with %get.obj_shortdesc[340]% &3&b50&0 times to fully bond with it.
    wait 2s
    msend %actor% %self.name% says, 'You can ask about your &6&b[mace progress]&0 at any time.'
  elseif %actor.has_completed[phase_mace]% 
    say There is no weapon greater than the mace of disruption!
  else
    msend %actor% %self.name% says, 'I've already done all I can.  If you want to further improve your mace, you'll have to seek out a different master crafter.'
    wait 1s
    switch %actor.quest_stage[phase_mace]%
      case 2
        msend %actor% %self.name% says, 'Someone familiar with the grave will be able to work on this mace.  Seek out the Sexton in the Abbey west of the Village of Mielikki.'
        break
      case 3
        msend %actor% %self.name% says, 'The Cleric Guild is capable of some miraculous crafting.  Visit the Cleric Guild Master in the Arctic Village of Ickle and talk to High Priest Zalish.  He should be able to help you.'
        break
      case 4
        msend %actor% %self.name% says, 'Continue with the Cleric Guild Masters.  Check in with the High Priestess in the City of Anduin.'
        break
      case 5
        msend %actor% %self.name% says, 'Sometimes to battle the dead, we need to use their own dark natures against them.  Few are as knowledgeable about the dark arts as Ziijhan, the Defiler, in the Cathedral of Betrayal.'
        break
      case 6
        msend %actor% %self.name% says, 'Return again to the Abbey of St. George and seek out Silania.  Her mastry of spiritual matters will be necessary to improve this mace any further.'
        break
      case 7
        msend %actor% %self.name% says, 'I'm loathe to admit it, but of the few remaining who are capable of improving your weapon, one is a priest of a god most foul.  Find Ruin Wormheart, that most heinous of Blackmourne's servators.'
        break
      case 8
        msend %actor% %self.name% says, 'The most powerful force in the war against the dead is the sun itself.  Consult with the sun's Oracle in the ancient pyramid near Anduin.'
        break
      case 9
        msend %actor% %self.name% says, 'With everything prepared, return to the very beginning of your journey.  The High Priestess of Mielikki, the very center of the Cleric Guild, will know what to do.'
    done
  endif
else
  msend %actor% %self.name% says, 'Come back after you've gained some more experience.  I can help you then.'
endif
~
#3032
quest_wug_magistrate~
0 d 100
power something what found~
wait 2
if %actor.level% < 30
  nod
  msend %actor% %self.name% tells you, 'They have discovered a secret chamber below
  msend %actor% &0the main temple.  It seems something down there is using extraordinary
  msend %actor% &0magical power for reasons that aren't yet clear.'
  wait 1s
  msend %actor% %self.name% tells you, 'You're not yet known in the realm, which is
  msend %actor% &0perfect for my plan.  No one will know you're there on a mission.  Go
  msend %actor% &0investigate and find out what is using this power.  Destroy it if you
  msend %actor% &0must, just don't let one of those deranged mystics get it!'
endif
~
#3033
Magistrate speech Wug~
0 d 100
wug~
if %actor.quest_stage[dragon_slayer]% == 3
  msend %actor% %self.name% tells you, 'There is a temple just outside the West Gate
  msend %actor% &0of town dedicated to the Kaaz clan, the great heroes of the Rift Wars.  
  msend %actor% Mystics from all around Caelia go there hoping to find some kind of special
  msend %actor% &0power.'
  wait 4
  msend %actor% %self.name% tells you, 'They have discovered a secret chamber below
  msend %actor% &0the main temple.  It seems something down there has sealed away a whole slew of
  msend %actor% &0nasty draklings.'
  wait 4
  msend %actor% %self.name% tells you, 'Legend has it they're repelled by powerful
  msend %actor% &0heroes like the Kaaz clan, but dragon slayers like you should be able to fight
  msend %actor% &0them without issue.'
endif
~
#3034
TCD_Entrance~
2 c 100
knock~
wsend %actor% A dark and sinister looking portal opens suddenly and pulls you inside.
wechoaround %actor% A dark and sinister looking portal opens suddenly and pulls %actor.name% inside.
wdoor 3034 up room 18800
wforce %actor% up
wdoor 3034 up purge
~
#3035
Magistrate refuse~
0 j 0
55211 55577 13614 58809 16023 340~
switch %objet.vnum%
  default
    wait 1
    say Thanks!
done
~
#3040
blob-to-pawnbroker~
0 bd 1
drop off the junk~
   mgoto 6172
   mjunk all.key
   mteleport anduin_pawnbroker 6172
   remove all
   drop all
   mforce pawnbroker get all
   mteleport anduin_pawnbroker 6034
   set randmielrm %random.18%
   eval randloadrm %randmielrm% + 3051
   mgoto %randloadrm% 
~
#3041
blob_nofight~
0 k 100
~
set room %self.room%
emote seems to disintegrate, melting into the ground.
mteleport %self% 1100
wait 5s
mteleport %self% %room%
mecho Green gelatin suddenly bubbles out of the ground, wriggling into a blob.
~
#3050
Armor Exchange greeting~
0 h 100
~
wait 2
if %actor.vnum% == -1
  set item %actor.quest_variable[armor_exchange:gem_vnum]%
  if %actor.quest_stage[armor_exchange]% == 1
    msend %actor% %self.name% shouts to you, 'Well howdy-hoo!  Welcome back!'
    grin %actor%
    wait 2s
    if %item% == 0
      msend %actor% %self.name% says, 'Tell me dearie, what can I help ya find today?'
    else
      msend %actor% %self.name% says, 'Are you still looking for %get.obj_shortdesc[%item%]%?'
    endif
  else
    msend %actor% %self.name% hollers, 'Well hello there dearie!  Welcome to a land of rare and valuable treasures!'
    msend %actor% %self.name% indicates the massive piles of junk.
    wait 1s
    msend %actor% %self.name% says to you, 'We can trade all manner of precious old goodies here.  People leave behind armor they don't want anymore for stuff they do.'
    wait 4s
    msend %actor% %self.name% says, 'So what can I help you find today?'
  endif
endif
~
#3051
Armor Exchange set type~
0 d 100
tarnished worn creased crushed burned corroded decayed rusted flimsy~
wait 2
if !%actor.quest_stage[armor_exchange]%
  quest start armor_exchange %actor%
endif
if %speech% /= rusted
  if %speech% /= plate
    if %speech% /= boots
      set armor_vnum 55304
    elseif %speech% /= bracer
      set armor_vnum 55308
    else
      set armor_vnum 55324
    endif
  elseif %speech% /= boots
    set armor_vnum 55304
  elseif %speech% /= bracer
    set armor_vnum 55308
  elseif %speech% /= gauntlets
    set armor_vnum 55300
  elseif %speech% /= helm
    set armor_vnum 55312
  elseif %speech% /= vambraces
    set armor_vnum 55316
  elseif %speech% /= greaves
    set armor_vnum 55320
  endif
elseif %speech% /= flimsy
  if %speech% /= gloves
    set armor_vnum 55301
  elseif %speech% /= boots
    set armor_vnum 55305
  elseif %speech% /= bracer
    set armor_vnum 55309
  elseif %speech% /= cap
    set armor_vnum 55313
  elseif %speech% /= sleeves
    set armor_vnum 55317
  elseif %speech% /= leggings
    set armor_vnum 55321
  elseif %speech% /= tunic
    set armor_vnum 55325
  endif
elseif %speech% /= decayed
  if %speech% /= mittens
    set armor_vnum 55302
  elseif %speech% /= slippers
    set armor_vnum 55306
  elseif %speech% /= bracelet
    set armor_vnum 55310
  elseif %speech% /= turban
    set armor_vnum 55314
  elseif %speech% /= sleeves
    set armor_vnum 55318
  elseif %speech% /= leggings
    set armor_vnum 55322
  elseif %speech% /= robe
    set armor_vnum 55326
  endif
elseif %speech% /= crushed
  if %speech% /= leather
    if %speech% /= gloves
      set armor_vnum 55329
    elseif %speech% /= boots
      set armor_vnum 55333
    elseif %speech% /= bracer
      set armor_vnum 55337
    elseif %speech% /= sleeves
      set armor_vnum 55345
    elseif %speech% /= leggings
      set armor_vnum 55349
    elseif %speech% /= tunic
      set armor_vnum 55353
    endif
  elseif %speech% /= hood
    set armor_vnum 55341
  elseif %speech% /= plate
    if %speech% /= boots
      set armor_vnum 55332
    elseif %speech% /= bracer
      set armor_vnum 55336
    else
      set armor_vnum 55352
    endif
  elseif %speech% /= gauntlets
    set armor_vnum 55328
  elseif %speech% /= helm
    set armor_vnum 55340
  elseif %speech% /= vambraces
    set armor_vnum 55344
  elseif %speech% /= greaves
    set armor_vnum 55348
  endif
elseif %speech% /= burned
  if %speech% /= gloves
    set armor_vnum 55330
  elseif %speech% /= mittens
    set armor_vnum 55331
  elseif %speech% /= boots
    set armor_vnum 55334
  elseif %speech% /= slippers
    set armor_vnum 55335
  elseif %speech% /= wristband
    set armor_vnum 55338
  elseif %speech% /= bracelet
    set armor_vnum 55339
  elseif %speech% /= cap
    set armor_vnum 55342
  elseif %speech% /= turban
    set armor_vnum 55343
  elseif %speech% /= vambraces
    set armor_vnum 55346
  elseif %speech% /= sleeves
    set armor_vnum 55347
  elseif %speech% /= pants
    set armor_vnum 55350
  elseif %speech% /= leggings
    set armor_vnum 55351
  elseif %speech% /= jerkin
    set armor_vnum 55354
  elseif %speech% /= robe
    set armor_vnum 55355
  endif
elseif %speech% /= tarnished
  if %speech% /= plate
    if %speech% /= boots
      set armor_vnum 55360
    elseif %speech% /= bracer
      set armor_vnum 55364
    else
      set armor_vnum 55380
    endif
  elseif %speech% /= boots
    set armor_vnum 55360
  elseif %speech% /= bracer
    set armor_vnum 55364
  elseif %speech% /= gauntlets
    set armor_vnum 55356
  elseif %speech% /= helm
    set armor_vnum 55368
  elseif %speech% /= vambraces
    set armor_vnum 55372
  elseif %speech% /= greaves
    set armor_vnum 55376
  endif
elseif %speech% /= worn
  if %speech% /= mittens
    set armor_vnum 55359
  elseif %speech% /= slippers
    set armor_vnum 55363
  elseif %speech% /= bracelet
    set armor_vnum 55367
  elseif %speech% /= turban
    set armor_vnum 55371
  elseif %speech% /= sleeves
    set armor_vnum 55375
  elseif %speech% /= leggings
    set armor_vnum 55379
  elseif %speech% /= robe
    set armor_vnum 55383
  endif
elseif %speech% /= corroded
  if %speech% /= gloves
    set armor_vnum 55358
  elseif %speech% /= boots
    set armor_vnum 55362
  elseif %speech% /= wristband
    set armor_vnum 55366
  elseif %speech% /= cap
    set armor_vnum 55370
  elseif %speech% /= vambraces
    set armor_vnum 55374
  elseif %speech% /= pants
    set armor_vnum 55378
  elseif %speech% /= jerkin
    set armor_vnum 55382
  endif
elseif %speech% /= creased
  if %speech% /= gloves
    set armor_vnum 55357
  elseif %speech% /= boots
    set armor_vnum 55361
  elseif %speech% /= bracer
    set armor_vnum 55365
  elseif %speech% /= hood
    set armor_vnum 55369
  elseif %speech% /= sleeves
    set armor_vnum 55373
  elseif %speech% /= leggings
    set armor_vnum 55377
  elseif %speech% /= tunic
    set armor_vnum 55381
  endif
endif
if !%armor_vnum%
  msend %actor% %self.name% tells you, 'I'm sorry, I don't know what that is.'
else
  msend %actor% %self.name% asks you, 'You want %get.obj_shortdesc[%armor_vnum%]%?'
  quest variable armor_exchange %actor% armor_vnum %armor_vnum%
endif
~
#3052
Armor Exchange confirm order~
0 d 100
yes no~
wait 2
set item %actor.quest_variable[armor_exchange:armor_vnum]%
if %speech% /= yes
  if %item% == 0
    msend %actor% %self.name% says, 'I don't think you asked me for anything, no sir.'
    halt
  endif
  msend %actor% %self.name% tells you, 'Oh my, yes dearie, of course.'
  if %item% <= 55303
    set class 1
    set tier 1
  elseif %item% <= 55307
    set class 1
    set tier 2
  elseif %item% <= 55311
    set class 1
    set tier 3
  elseif %item% <= 55315
    set class 1
    set tier 4
  elseif %item% <= 55319
    set class 1
    set tier 5
  elseif %item% <= 55323
    set class 1
    set tier 6
  elseif %item% <= 55327
    set class 1
    set tier 7
  elseif %item% <= 55331
    set class 2
    set tier 1
  elseif %item% <= 55335
    set class 2
    set tier 2
  elseif %item% <= 55339
    set class 2
    set tier 3
  elseif %item% <= 55343
    set class 2
    set tier 4
  elseif %item% <= 55347
    set class 2
    set tier 5
  elseif %item% <= 55351
    set class 2
    set tier 6
  elseif %item% <= 55355
    set class 2
    set tier 7
  elseif %item% <= 55359
    set class 3
    set tier 1
  elseif %item% <= 55363
    set class 3
    set tier 2
  elseif %item% <= 55367
    set class 3
    set tier 3
  elseif %item% <= 55371
    set class 3
    set tier 4
  elseif %item% <= 55375
    set class 3
    set tier 5
  elseif %item% <= 55379
    set class 3
    set tier 6
  elseif %item% <= 55383
    set class 3
    set tier 7
  endif
  msend %actor% %self.name% says, 'So %get.obj_shortdesc[%item%]% is a &3&bclass %class% tier %tier%&0 piece of gear.'
  wait 2s
  msend %actor% %self.name% says, 'I'll give it to you for any other armor of equal or greater rarity.'
  wait 2s
  msend %actor% %self.name% says, 'Consult the chart for degrees of rarity.'
  msend %actor% %self.name% points to the sign.
else
  quest variable gem_exchange %actor% gem_vnum 0
  msend %actor% %self.name% says, 'Alright, what DO you want then?'
endif
~
#3053
Armor Exchange receive exchange~
0 j 100
~
set item %actor.quest_variable[armor_exchange:armor_vnum]%
if %item% != 0
  if %item% <= 55303
    if %object.vnum% >= 55300 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55307
    if %object.vnum% >= 55304 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55311
    if %object.vnum% >= 55308 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55315
    if %object.vnum% >= 55312 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55319
    if %object.vnum% >= 55316 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55323
    if %object.vnum% >= 55320 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55327
    if %object.vnum% >= 55324 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55331
    if %object.vnum% >= 55328 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55335
    if %object.vnum% >= 55332 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55339
    if %object.vnum% >= 55336 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55343
    if %object.vnum% >= 55340 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55347
    if %object.vnum% >= 55344 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55351
    if %object.vnum% >= 55348 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55355
    if %object.vnum% >= 55352 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55359
    if %object.vnum% >= 55356 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55363
    if %object.vnum% >= 55360 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55367
    if %object.vnum% >= 55364 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55371
    if %object.vnum% >= 55368 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55375
    if %object.vnum% >= 55372 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55379
    if %object.vnum% >= 55376 && %object.vnum% <=55383
      set found 1
    endif
  elseif %item% <= 55783
    if %object.vnum% >= 55380 && %object.vnum% <=55383
      set found 1
    endif
  endif
  if %found% == 1
    wait 2
    msend %actor% %self.name% says, 'Here you are my dear!'
    mjunk %object%
    mload obj %item%
    give all %actor%
    wait 2
    msend %actor% %self.name% says, 'A pleasure connecting lost things to their owners!'
    quest variable armor_exchange %actor% armor_vnum 0
  else
    return 0
    msend %actor% %self.name% refuses to perform the exchange.
    wait 1s
    if %object.vnum% >= 55300 && %object.vnum% <=55383
      msend %actor% %self.name% says, 'That's not rare enough!  I won't take %object.shortdesc% for %get.obj_shortdesc[%item%]%!'
    else
      msend %actor% %self.name% says, 'Sorry, %object.shortdesc% isn't the kind of thing I keep around here...'
    endif
  endif
else
  return 0
  msend %actor% %self.name% refuses %object.shortdesc%.
  wait 1s
  msend %actor% %self.name% says, 'I don't have any lost treasures for you dearie.'
endif
~
#3054
Armor Exchange buy blocker~
0 c 100
buy~
switch %cmd%
  case b
  case bu
    return 0
    halt
done
msend %actor% %self.name% says, 'Betsy Boo's Treasure Palace is not a SHOP.  I don't buy, sell, list, or value goods.'
wait 1s
msend %actor% %self.name% says indignantly, 'This is not JUNK!'
~
#3055
Armor Exchange list blocker~
0 c 100
list~
switch %cmd%
  case l
  case li
    return 0
    halt
done
msend %actor% %self.name% says, 'Betsy Boo's Treasure Palace is not a SHOP.  I don't buy, sell, list, or value goods.'
wait 1s
msend %actor% %self.name% says indignantly, 'This is not JUNK!'
~
#3056
Armor Exchange sell blocker~
0 c 100
sell~
switch %cmd%
  case s
    return 0
    halt
done
msend %actor% %self.name% says, 'Betsy Boo's Treasure Palace is not a SHOP.  I don't buy, sell, list, or value goods.'
wait 1s
msend %actor% %self.name% says indignantly, 'This is not JUNK!'
~
#3057
Armor Exchange value blocker~
0 c 100
value~
msend %actor% %self.name% says, 'Betsy Boo's Treasure Palace is not a SHOP.  I don't buy, sell, list, or value goods.'
wait 1s
msend %actor% %self.name% says indignantly, 'This is not JUNK!'
~
#3075
MIelikki South Guard~
0 c 100
south~
if %actor.vnum% == -1
if %actor.level% < 30
whisper %actor.name% You are much too little to venture south of here.
wait 1
whisper %actor.name% Try other areas first.
nudge %actor.name%
emote points at the sign.
else
return 0
endif
else
return 0
endif
~
#3076
test_quest_banter1~
0 n 1
hi hello howdy quest~
if !%actor.has_completed[zzurs_funky_quest]%
say I wonder if my brother the Northern road ranger is still alive?
sigh
say If only I had some proof of his doings.
say Will you help me?
else
end
~
#3077
test_quest_rece_south~
0 j 100
~
If %actor.vnum% == -1
   if %object.vnum% == 3080
      if %actor.quest_stage[zzurs_funky_quest]% == 4
         wait 1
         gasp
         mjunk necklace
         say My brother is alive! woo hoo!
         wait 2
         mload obj 16009
         give shield %actor.name%
         Say take that as an expression of my grattitude!
	 quest complete zzurs_funky_quest %actor.name%
      else
	 wait 1
         say Yes! My brother is alive!
         wait 1
         say Thank you!
         mjunk necklace
      end
   else
   eye %actor.name%
   end
else
end
~
#3078
test_quest_speech_north~
0 n 1
brother~
if %actor.vnum% == -1
   if %actor.quest_stage[zzurs_funky_quest]% == 1
      msend %actor% %self.name% says to you, 'My brother is looking for me?'
      quest advance zzurs_funky_quest %actor.name%
   else
   end
else
end
~
#3079
test_quest_speech2_north~
0 n 1
yes~
if %actor.vnum% == -1
   if %actor.quest_stage[zzurs_funky_quest]% == 2
      msend %actor% %self.name% says to you, 'I would love to send him proof of my doings but I am so busy.'
      sigh
      msend %actor% %self.name% says to you, 'I need a diadem of bone.'
      quest advance zzurs_funky_quest %actor.name%
   else
   end
else
end
~
#3080
Dragon Slayer Isilynor Greet~
0 h 100
~
wait 2
set anti Anti-Paladin
if !%actor.has_completed[dragon_slayer]%
  if !%actor.quest_stage[dragon_slayer]%
    if %actor.level% > 4
      msend %actor% %self.name% says, 'Hail!  I am Isilynor, Grand Master of the Knights of Dragonfire, a guild of professional dragon slayers.  We're always seeking bold recruits to &6&b[hunt]&0 the great dragons of the world.  I reward those who put their mettle to the test.'
    endif
  elseif %actor.quest_variable[dragon_slayer:hunt]% == running
    msend %actor% %self.name% says, 'You're still on the hunt.  What are you doing here?  If you lost your notice say &6&b"I need a new notice"&0.'
  elseif %actor.quest_variable[dragon_slayer:hunt]% == dead
    msend %actor% %self.name% says, 'Welcome back!  If your hunt was successful give me your notice.  If you lost your notice say &6&b"I need a new notice"&0.'
  elseif %actor.quest_stage[dragon_slayer]% >= 1 && !%actor.has_completed[dragon_slayer]%
    msend %actor% %self.name% says, 'Ah, back for another dragon to &6&b[hunt]&0 I see.'
  endif
  if !%actor.has_completed[paladin_pendant]% && %actor.level% > 9 && (%actor.class% /= Paladin || %actor.class% /= %anti%)
    wait 1s
    msend %actor% %self.name% says, 'Or maybe you're here to discuss your &6&b[devotion]&0 to the cause.'
  endif
else
  if (%actor.class% /= Paladin || %actor.class% == %anti%) && !%actor.quest_stage[paladin_pendant]% && %actor.level% > 9
    msend %actor% %self.name% says, 'A new recruit looking to prove your &6&b[devotion]&0 to the cause.'
  elseif %actor.quest_stage[paladin_pendant]% && !%actor.has_completed[paladin_pendant]%
    msend %actor% %self.name% says, 'Ah, you must be looking to prove your &6&b[devotion]&0 again.'
  endif
endif
~
#3081
Dragon Slayers Isilynor Speech~
0 d 1
hunt dragon dragons~
wait 2
if %actor.has_completed[dragon_slayer]%
  msend %actor% %self.name% says, 'The only dragons remaining are beasts of legend!'
elseif %actor.level% < 5
  msend %actor% %self.name% says, 'You're not quite ready to start taking on dragons.  Come back when you've seen a little more.'
elseif (!%actor.quest_stage[dragon_slayer]% && %actor.level% >= 5) || (%actor.quest_stage[dragon_slayer]% == 1 && !%actor.quest_variable[dragon_slayer:hunt]%)
  msend %actor% %self.name% says, 'Before you go slaying any real dragons, let's start with a test kill.'
  wait 2
  msend %actor% %self.name% says, 'The hedges in the topiary have been brought to life somehow.  We don't know exactly what's causing it, but that's also not our concern at the moment.'
  wait 4
  msend %actor% %self.name% says, 'Some of the dragon hedges have become a real problem!  They're big, nasty, and put up one hell of a fight.  They might only be shrubbery, but they'll be a great way to prove you're ready for the real thing.'
  wait 4
  msend %actor% %self.name% says, 'Are you up to the challenge?'
elseif %actor.quest_variable[dragon_slayer:hunt]% == dead
  msend %actor% %self.name% says, 'Give me your current notice first.'
  halt
else
  if %actor.level% >= (%actor.quest_stage[dragon_slayer]% - 1) * 10
    switch %actor.quest_stage[dragon_slayer]% 
      case 1
        if %actor.quest_variable[dragon_slayer:hunt]% == running
          msend %actor% %self.name% says, 'You still have a dragon to slay.  If you can't even take out a dragon hedge, you'll never be ready for the real thing.'
        endif
        break
      case 2
        if %actor.quest_variable[dragon_slayer:hunt]% == running
          msend %actor% %self.name% says, 'You still have a dragon to slay.  Hunt down the green wyrmling in Morgan Hill.'
        else
          msend %actor% %self.name% says, 'We've heard there's a green dragon who's taken up residence below the old house on Morgan Hill.  Our scouts say it's still just a wyrmling, so now is the best time to take it out.  Big dragons come from little dragons afterall.'
          wait 4
          msend %actor% %self.name% says, 'But don't let your guard down!  A baby dragon is still a dragon!'
          wait 4
          msend %actor% %self.name% says, 'Are you ready for your first real kill?'
        endif
        break
      case 3
        if %actor.quest_variable[dragon_slayer:hunt]% == running
          msend %actor% %self.name% says, 'You still have a dragon to slay.  Hunt down Wug the Fiery Drakling.'
        else
          msend %actor% %self.name% says, 'I've been interested in the stories around Wug the Fiery Drakling, a fierce and fearsome beast who used to terrorize Mielikki with a brood of other lesser drakes in ages past.  But they all suddenly and mysteriously disappeared.'
          wait 4        
          msend %actor% %self.name% says, 'I hear the Templar Magistrate has uncovered some new information around the Wug legend.  Ask him about Wug and see what he knows.  You can become a legend by slaying a legend!'
          wait 4
          msend %actor% %self.name% says, 'Are you up for a little legendary discovery?'
        endif
        break
      case 4
        if %actor.quest_variable[dragon_slayer:hunt]% == running
          msend %actor% %self.name% says, 'You still have a dragon to slay.  Hunt down the young blue dragon near the Tower in the Wastes.'
        else
          msend %actor% %self.name% says, 'Another juvenile dragon has been spotted out near the old abandoned tower off the Black Rock Trail.  This one seems to be a lot more powerful than the little wyrmling under Morgan Hill though.'
          wait 4
          msend %actor% %self.name% says, 'This one is a blue dragon with considerable spellcasting power.  So be ready for lightning!'
          wait 4
          msend %actor% %self.name% says, 'So what do you think?  You ready to bring the thunder?'
        endif
        break
      case 5
        if %actor.quest_variable[dragon_slayer:hunt]% == running
          msend %actor% %self.name% says, 'You still have a dragon to slay.  Search the forests in South Caelia for a faerie dragon.'
        else
          msend %actor% %self.name% says, 'I just got news of something wild!  Apparently there's a section of forest in South Caelia that's just packed with magical beasts!  Incredibly rare faerie dragons can be found there in great numbers!'
          wait 4
          msend %actor% %self.name% says, 'While faerie dragons themselves aren't terribly strong, the other creatures that fill the forest are incredibly deadly.  Plus faerie dragons breathe a special kind of euphoric gas that confuses and disables anyone who breathes it.  It'll be a very nasty combo.'
          wait 4
          msend %actor% %self.name% says, 'Think you can hold your own against all manner of beasts?'
        endif
        break
      case 6
        if %actor.quest_variable[dragon_slayer:hunt]% == running
          msend %actor% %self.name% says, 'You still have a dragon to slay.  Kill that damn wyvern in the Highlands.'
        else
          msend %actor% %self.name% says, 'Since you've been able to hold out against the great beasts of the southern forest, I've got something closer to home to deal with.'
          wait 4
          msend %actor% %self.name% says, 'Out past the Gothra Desert is a stretch of highlands.  A very nasty wyvern has long haunted that region.  So many innocent travelers have met their end and this creature's claws.  Other knights have tried and failed to slay it but none have succeeded.'
          wait 4
          msend %actor% %self.name% says, 'Do you think you can finally fell the beast?'
        endif
        break
      case 7
        if %actor.quest_variable[dragon_slayer:hunt]% == running
          msend %actor% %self.name% says, 'You still have a dragon to slay.  Take down one of those ice lizards in Frost Valley.'
        else
          msend %actor% %self.name% says, 'A whole brood of giant ice reptiles slither through Frost Valley.  They seem to be somewhere between true dragons and just really huge lizards.  
          wait 4
          msend %actor% %self.name% says, 'That said, they seem like a great challenge for a well-experienced dragon slayer like yourself.'
          wait 4
          msend %actor% %self.name% says, 'You ready to brave the cold?'
        endif
        break
      case 8
        if %actor.quest_variable[dragon_slayer:hunt]% == running
          msend %actor% %self.name% says, 'You still have a dragon to slay.  Find and vanquish the Beast of Borgan.'
        else
          msend %actor% %self.name% says, 'I have a great mission for you.'
          wait 4
          msend %actor% %self.name% says, 'The ancient kingdom of Layveran fell to unholy forces in ages long past.  Its exact location is actually unknown.  But a strange planar rift out west takes you to the blasted ice desert surrounding the fallen castle.'
          wait 4
          msend %actor% %self.name% says, 'Rumors say some kind of unholy two-headed draconic abomination dwells in the depths of the ruins.  Reaching it, let alone slaying it, will be a mighty challenge fit for a true champion!'
          wait 4
          msend %actor% %self.name% says, 'You in?'
        endif
        break
      case 9
        if %actor.quest_variable[dragon_slayer:hunt]% == running
          msend %actor% %self.name% says, 'You still have a dragon to slay.  Eliminate the dragon the Ice Cult up north worships.'
        else
          msend %actor% %self.name% says, 'You're ready for a very high profile, high risk kill.  Up north is a cult dedicated to worshipping the white dragon Tri-Aszp.  Ending this cult is a high priority item and the only way to do that is the slay Tri-Aszp.'
          wait 4
          msend %actor% %self.name% says, 'But that's easier said than done.  Not only is Tri-Aszp a fully grown dragon with full command of ice and frost, the cult is heavily militarized and heavily armed, with powerful priests and magicians to boot.'
          wait 4
          msend %actor% %self.name% says, 'And to make matters worse, they're buried deep in the ice, surrounded by all manner of absolutely unstoppable monsters.'
          wait 4
          grin
          wait 4
          msend %actor% %self.name% says, 'It's going to be a thrill a minute!  Are you up to the challenge?'
        endif
        break
      case 10
        if %actor.quest_variable[dragon_slayer:hunt]% == running
          msend %actor% %self.name% says, 'You still have a dragon to slay.  Destroy the mighty Hydra - and watch out for all its heads!'
        else
          msend %actor% %self.name% says, 'I have one last great dragon for you to hunt down.'
          wait 4
          msend %actor% %self.name% says, 'The demon dragon Sagece has called to her several other deadly threats.  Among them is a massive Hydra - a many-headed monstrosity which Sagece entrusted with one of the keys to her lair.  Before one can even think about driving Sagece from Templace, the Hydra must fall.'
          wait 4
          msend %actor% %self.name% says, 'I want you to be the one to fell the Hydra.  Do so and you'll be welcomed to the echelons of the Grand Master Dragon Slayers!'
          wait 4
          msend %actor% %self.name% says, 'Ready for the fight of your life?'
        endif
    done
  else
    msend %actor% %self.name% says, 'More dragons exist, but they're too dangerous without more experience.  Come back when you've seen a little more.'
  endif
endif
~
#3082
Dragon Slayer Isilynor Speech yes~
0 d 1
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
    if !%person.quest_stage[dragon_slayer]% && %person.level% > 4
      quest start dragon_slayer %person%
    endif
    if %person.has_completed[dragon_slayer]%
      msend %person% %self.name% says, 'The only dragons remaining are beasts of legend!'
    elseif %person.level% < 5
      msend %person% %self.name% says, 'You're not quite ready to start taking on dragons.  Come back when you've seen a little more.'
    elseif %person.quest_variable[dragon_slayer:hunt]% == dead
      msend %person% %self.name% says, 'Give me your current notice first.'
    else
      if %person.level% >= (%person.quest_stage[dragon_slayer]% - 1) * 10
        if %person.quest_variable[dragon_slayer:hunt]% != running
          msend %person% %self.name% says, 'Excellent!'
          switch %person.quest_stage[dragon_slayer]%
            case 1
              set notice 3080
              break
            case 2
              set notice 3081
              break
            case 3
              set notice 3082
              break
            case 4
              set notice 3083
              break
            case 5
              set notice 3084
              break
            case 6
              set notice 3085
              break
            case 7
              set notice 3086
              break
            case 8
              set notice 3087
              break
            case 9
              set notice 3088
              break
            case 10
              set notice 3089
              break
          done
          mload obj %notice%
          give notice %person%
          msend %person% &0  
          msend %person% %self.name% says, 'When you've slayed the beast, bring that notice back to me.  I'll reward you then.'
          msend %person% &0  
          msend %person% %self.name% says, 'Be brave, be strong, and good luck!'
          quest variable dragon_slayer %person% hunt running
        else
          switch %person.quest_stage[dragon_slayer]% 
            case 1
              msend %person% %self.name% says, 'You still have a dragon to slay.  If you can't even take out a dragon hedge, you'll never be ready for the real thing.'
              break
            case 2
              msend %person% %self.name% says, 'You still have a dragon to slay.  Hunt down the green wyrmling in Morgan Hill.'
              break
            case 3
              msend %person% %self.name% says, 'You still have a dragon to slay.  Hunt down Wug the Fiery Drakling.'
              break
            case 4
              msend %person% %self.name% says, 'You still have a dragon to slay.  Hunt down the young blue dragon near the Tower in the Wastes.'
              break
            case 5
              msend %person% %self.name% says, 'You still have a dragon to slay.  Search the forests in South Caelia for a faerie dragon.'
              break
            case 6
              msend %person% %self.name% says, 'You still have a dragon to slay.  Kill that damn wyvern in the Highlands.'
              break
            case 7
              msend %person% %self.name% says, 'You still have a dragon to slay.  Take down one of those ice lizards in Frost Valley.'
              break
            case 8
              msend %person% %self.name% says, 'You still have a dragon to slay.  Find and vanquish the Beast of Borgan.'
              break
            case 9
              msend %person% %self.name% says, 'You still have a dragon to slay.  Eliminate the dragon the Ice Cult up north worships.'
              break
            case 10
              msend %person% %self.name% says, 'You still have a dragon to slay.  Destroy the mighty Hydra - and watch out for all its heads!'
          done
        endif
      else
        msend %person% %self.name% says, 'More dragons exist, but they're too dangerous without more experience.  Come back when you've seen a little more.'
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#3083
Dragon Slayer Isilynor Receive~
0 j 100
~
switch %object.vnum%
  * hunt notices here
  case 3080
    set stage 1
    set victim1 a dragon hedge
    set go hunt
    break
  case 3081
    set stage 2
    set victim1 the green wyrmling
    set go hunt
    break
  case 3082
    set stage 3
    set victim1 Wug the Fiery Drakling
    set go hunt
    break
  case 3083
    set stage 4
    set victim1 the young blue dragon
    set go hunt
    break
  case 3084
    set stage 5
    set victim1 a faerie dragon
    set go hunt
    break
  case 3085
    set stage 6
    set victim1 the wyvern
    set go hunt
    break
  case 3086
    set stage 7
    set victim1 an ice lizard
    set go hunt
    break
  case 3087
    set stage 8
    set victim1 the Beast of Borgan
    set go hunt
    break
  case 3088
    set stage 9
    set victim1 Tri-Aszp
    set go hunt
    break
  case 3089
    set stage 10
    set victim1 the Hydra
    set go hunt
    break
  * paladin pendant items start here
  case 360
    set pendantstage 1
    set item quest
    set go necklace
    break
  case 12003
    set pendantstage 1
    set item necklace
    set go necklace
    break
  case 55582
    set pendantstage 1
    set item gem
    set go necklace
    break
  case 361
    set pendantstage 2
    set item quest
    set go necklace
    break
  case 23708
    set pendantstage 2
    set item necklace
    set go necklace
    break
  case 55590
    set pendantstage 2
    set item gem
    set go necklace
    break
  case 362
    set pendantstage 3
    set item quest
    set go necklace
    break
  case 58005
    set pendantstage 3
    set item necklace
    set go necklace
    break
  case 55622
    set pendantstage 3
    set item gem
    set go necklace
    break
  case 363
    set pendantstage 4
    set item quest
    set go necklace
    break
  case 48123
    set pendantstage 4
    set item necklace
    set go necklace
    break
  case 55654
    set pendantstage 4
    set item gem
    set go necklace
    break
  case 364
    set pendantstage 5
    set item quest
    set go necklace
    break
  case 12336
    set pendantstage 5
    set item necklace
    set go necklace
    break
  case 55662
    set pendantstage 5
    set item gem
    set go necklace
    break
  case 365
    set pendantstage 6
    set item quest
    set go necklace
    break
  case 43019
    set pendantstage 6
    set item necklace
    set go necklace
    break  
  case 55677
    set pendantstage 6
    set item gem
    set go necklace
    break
  case 366
    set pendantstage 7
    set item quest
    set go necklace
    break
  case 37015
    set pendantstage 7
    set item necklace
    set go necklace
    break
  case 55709
    set pendantstage 7
    set item gem
    set go necklace
    break
  case 367
    set pendantstage 8
    set item quest
    set go necklace
    break
  case 58429
    set pendantstage 8
    set item necklace
    set go necklace
    break
  case 55738
    set pendantstage 8
    set item gem
    set go necklace
    break
  case 368
    set pendantstage 9
    set item quest
    set go necklace
    break
  case 52010
    set pendantstage 9
    set item necklace
    set go necklace
    break
  case 55739
    set pendantstage 9
    set item gem
    set go necklace
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
  if %actor.quest_stage[dragon_slayer]% == %stage% && %actor.quest_variable[dragon_slayer:hunt]% == dead
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
    quest variable dragon_slayer %actor% target1 0
    quest variable dragon_slayer %actor% hunt 0
    wait 2
    if %stage% < 10
      quest advance dragon_slayer %actor%
      msend %actor% %self.name% says, 'Check in again if you have time for more work.'
    else
      quest complete dragon_slayer %actor%
      msend %actor% %self.name% says, 'You have earned your place among the greatest dragon slayers in the realm!'
      wait 1s
      msend %actor% %self.name% says, 'I bestow upon you this crest in recognition of your might.  Wear this proudly.'
      mload obj 549
      give crest %actor%
    endif
    if (%actor.class% /= paladin || %actor.class% /= %anti%) && %actor.quest_stage[paladin_pendant]% == 0
      wait 2s
      msend %actor% %self.name% says, 'I think you've earned this too.'
      mload obj 360
      give necklace %actor%
      wait 1s
      msend %actor% %self.name% says, 'Necklaces like these are proof of your devotion to your causes.'
      quest start paladin_pendant %actor%
      wait 2s
      if %actor.level% > 9
        msend %actor% %self.name% says, 'This is an opportune time to further prove your &6&b[devotion]&0.'
      else
        msend %actor% %self.name% says, 'Come back with that necklace after you reach level 10.  We can discuss acts of devotion then.'
      endif
    endif
  elseif %actor.quest_stage[dragon_slayer]% > %stage%
    return 0
    shake
    mecho %self.name% refuses the notice.
    wait 2
    msend %actor% %self.name% says, 'You already killed this dragon!'
  elseif %actor.quest_stage[dragon_slayer]% < %stage%
    wait 2
    eye %actor%
    msend %actor% %self.name% says, 'How'd you get this?!  You steal it off someone else??'
    mecho %self.name% rips up the notice!
    mjunk %object%
  elseif %actor.quest_variable[dragon_slayer:hunt]% != dead
    return 0
    mecho %self.name% refuses the notice.
    wait 2
    msend %actor% %self.name% says, 'You have to slay the dragon first!  %victim1% is still out there.
  endif
elseif %go% == necklace
  if %actor.quest_stage[dragon_slayer]% < %actor.quest_stage[paladin_pendant]%
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'Slay some more dragons first.'
  elseif %actor.level% < (%actor.quest_stage[paladin_pendant]% * 10)
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'You need to gain some more experience first.'
  elseif %pendantstage% > %actor.quest_stage[paladin_pendant]%
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'Your devotional act doesn't involve that yet.  Be patient!'
  elseif %pendantstage% < %actor.quest_stage[paladin_pendant]%
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'You've already performed that act of devotion.'
  else
    if %item% == quest
      set job1 %actor.quest_variable[paladin_pendant:necklacetask1]%
      set job2 %actor.quest_variable[paladin_pendant:necklacetask2]%
      set job3 %actor.quest_variable[paladin_pendant:necklacetask3]%
      set job4 %actor.quest_variable[paladin_pendant:necklacetask4]%
      if %job1% && %job2% && %job3% && %job4%
        wait 2
        eval reward %object.vnum% + 1
        mjunk %object%
        nod
        msend %actor% %self.name% says, 'Well done!  You've proven your devotion.'
        mload obj %reward%
        give necklace %actor%
        eval expcap %pendantstage% * 10
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
          quest variable paladin_pendant %actor% necklacetask%number% 0
          eval number %number% + 1
        done
        if %actor.quest_stage[paladin_pendant]% < 9
          quest advance paladin_pendant %actor%
        else
          quest complete paladin_pendant %actor%
        endif
      else
        return 0
        shake
        mecho %self.name% refuses %object.shortdesc%.
        wait 2
        msend %actor% %self.name% says, 'You need to do everything else before you offer up your necklace!'
      endif
    elseif %item% == necklace
      if %actor.quest_variable[paladin_pendant:necklacetask2]% == %object.vnum%
        set accept no
      else
        set accept yes
        quest variable paladin_pendant %actor% necklacetask2 %object.vnum%
      endif
    elseif %item% == gem
      if %actor.quest_variable[paladin_pendant:necklacetask3]% == %object.vnum%
        set accept no
      else
        set accept yes
        quest variable paladin_pendant %actor% necklacetask3 %object.vnum%
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
      set job1 %actor.quest_variable[paladin_pendant:necklacetask1]%
      set job2 %actor.quest_variable[paladin_pendant:necklacetask2]%
      set job3 %actor.quest_variable[paladin_pendant:necklacetask3]%
      set job4 %actor.quest_variable[paladin_pendant:necklacetask4]%
      if %job1% && %job2% && %job3% && %job4%
        msend %actor% %self.name% says, 'Excellent.  Now turn over your current necklace as the final proof of your devotion.'
      else
        msend %actor% %self.name% says, 'Good, now finish the rest.'
      endif
    endif
  endif
endif
~
#3084
Dragon Slayer dragon death~
0 f 100
~
switch %self.vnum%
  case 18004
  * Dragon Hedge
    set stage 1
    set target1 dragon_hedge
    break
  case 13626
  * Green Wyrmling
    set target1 green_wyrmling
    set stage 2
    break
  case 8034
  * Wug
    set target1 wug
    set stage 3
    break
  case 12500
  * young blue dragon
    set target1 young_blue_dragon
    set stage 4
    break
  case 12322
  * faerie dragon
    set target1 faerie_dragon
    set stage 5
    break
  case 16309
  * wyvern
    set target1 wyvern
    set stage 6
    break
  case 53410
  * ice lizard
    set target1 ice_lizard
    set stage 7
    break
  case 4013
  * Beast of Borgan
    set target1 borgan
    set stage 8
    break
  case 53300
  * Tri-Aszp
    set target1 tri-aszp
    set stage 9
    break
  case 52010
  * Hydra
    set target1 hydra
    set stage 10
    break
done
set person %actor%
set i %actor.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[dragon_slayer]% == %stage% && %person.quest_variable[dragon_slayer:hunt]% == running
      quest variable dragon_slayer %person% target1 %target1%
      quest variable dragon_slayer %person% hunt dead
      msend %person% &1&bYou cross %self.name% off your list.&0
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#3085
Dragon Slayer notice look~
1 m 100
~
switch %self.vnum%
  case 3080
    set stage 1
    set victim1 a dragon hedge
    set go hunt
    break
  case 3081
    set stage 2
    set victim1 the green wyrmling
    set go hunt
    break
  case 3082
    set stage 3
    set victim1 Wug the Fiery Drakling
    set go hunt
    break
  case 3083
    set stage 4
    set victim1 the young blue dragon
    set go hunt
    break
  case 3084
    set stage 5
    set victim1 a faerie dragon
    set go hunt
    break
  case 3085
    set stage 6
    set victim1 the wyvern
    set go hunt
    break
  case 3086
    set stage 7
    set victim1 an ice lizard
    set go hunt
    break
  case 3087
    set stage 8
    set victim1 the Beast of Borgan
    set go hunt
    break
  case 3088
    set stage 9
    set victim1 Tri-Aszp
    set go hunt
    break
  case 3089
    set stage 10
    set victim1 the Hydra
    set go hunt
    break
done
return 0
osend %actor% This is a notice to slay %victim1%.
if %actor.quest_variable[dragon_slayer:hunt]% == dead && %actor.quest_stage[dragon_slayer]% == %stage%
  osend %actor% You have completed the hunt.  
  osend %actor% Return the notice to Isilynor for your reward!
endif
~
#3086
Dragon Slayer notice examine~
1 c 3
examine~
if %arg% == notice
  switch %self.vnum%
    case 3080
      set stage 1
      set victim1 a dragon hedge
      set go hunt
      break
    case 3081
      set stage 2
      set victim1 the green wyrmling
      set go hunt
      break
    case 3082
      set stage 3
      set victim1 Wug the Fiery Drakling
      set go hunt
      break
    case 3083
      set stage 4
      set victim1 the young blue dragon
      set go hunt
      break
    case 3084
      set stage 5
      set victim1 a faerie dragon
      set go hunt
      break
    case 3085
      set stage 6
      set victim1 the wyvern
      set go hunt
      break
    case 3086
      set stage 7
      set victim1 an ice lizard
      set go hunt
      break
    case 3087
      set stage 8
      set victim1 the Beast of Borgan
      set go hunt
      break
    case 3088
      set stage 9
      set victim1 Tri-Aszp
      set go hunt
      break
    case 3089
      set stage 10
      set victim1 the Hydra
      set go hunt
      break
  done
else
  return 0
  halt
endif
osend %actor% This is a notice to slay %victim1%.
if %actor.quest_variable[dragon_slayer:hunt]% == dead && %actor.quest_stage[dragon_slayer]% == %stage%
  osend %actor% You have completed the hunt.  
  osend %actor% Return the notice to Isilynor for your reward!
endif
~
#3087
Paladin pendant Isilynor speech~
0 d 100
devotion~
set anti Anti-Paladin
set pendantstage %actor.quest_stage[paladin_pendant]%
set huntstage %actor.quest_stage[dragon_slayer]%
set job1 %actor.quest_variable[paladin_pendant:necklacetask1]%
set job2 %actor.quest_variable[paladin_pendant:necklacetask2]%
set job3 %actor.quest_variable[paladin_pendant:necklacetask3]%
set job4 %actor.quest_variable[paladin_pendant:necklacetask4]%
wait 2
if %actor.class% != Paladin && %actor.class% != %anti%
  shake
  msend %actor% %self.name% says, 'Such acts are only for warriors of Justice or Destruction.'
  halt
elseif %actor.level% < 10
  msend %actor% %self.name% says, 'You aren't ready for such an act yet.  Come back when you've grown a bit.'
  halt
elseif %actor.level% < (%pendantstage% * 10)
  say You aren't ready for another devotional act yet.  Come back when you've gained some more experience.
  halt
elseif %actor.has_completed[paladin_pendant]%
  msend %actor% %self.name% says, 'You've already proven your devotion as much as possible!'
  halt
endif
if %pendantstage% == 0
  mecho %self.name% says, 'Sure.  You must &6&b[hunt]&0 a dragon first though.'
  halt
elseif (%pendantstage% >= %huntstage%) && !%actor.has_completed[dragon_slayer]%
  msend %actor% %self.name% says, 'Slay a few more dragons and then we can talk.'
  halt
elseif %job1% && %job2% && %job3% && %job4%
  msend %actor% %self.name% says, 'You're all ready, just give me your old necklace.'
  halt
endif
switch %pendantstage%
  case 1
    set necklace 12003
    set gem 55582
    set place The Mist Temple Altar
    set hint in the Misty Caverns.
    break
  case 2
    set necklace 23708
    set gem 55590
    set place Chamber of Chaos
    set hint in the Temple of Chaos.
    break
  case 3
    set necklace 58005
    set gem 55622
    set place Altar of Borgan
    set hint in the lost city of Nymrill.
    break
  case 4
    set necklace 48123
    set gem 55654
    set place A Hidden Altar Room
    set hint in a cave in South Caelia's Wailing Mountains.
    break
  case 5
    set necklace 12336
    set gem 55662
    set place The Altar of the Snow Leopard Order
    set hint buried deep in Mt. Frostbite
    break
  case 6
    set necklace 43019
    set gem 55677
    set place Chapel Altar
    set hint deep underground in a lost castle.
    break
  case 7
    set necklace 37015
    set gem 55709
    set place A Cliffside Altar
    set hint tucked away in the land of Dreams.
    break
  case 8
    set necklace 58429
    set gem 55738
    set place Dark Altar
    set hint entombed with an ancient evil king.
    break
  case 9
    set necklace 52010
    set gem 55739
    set place An Altar
    set hint far away in the Plane of Air.
    break
done
eval attacks %pendantstage% * 100
nod
msend %actor% %self.name% says, 'With each act of devotion you'll earn a new necklace.  Undertake the following:
msend %actor% - Attack &3&b%attacks%&0 times while wearing your current necklace.
msend %actor% - Find &3&b%get.obj_shortdesc[%necklace%]%&0 as the base for the new necklace.
msend %actor% - Find &3&b%get.obj_shortdesc[%gem%]%&0 for decoration.
msend %actor% &0    
msend %actor% You also need to take your necklace and &3&b[pray]&0 in a sanctified space.
msend %actor% Find "&3&b%place%&0".  It's %hint%
msend %actor% &0  
msend %actor% You can ask about your &6&b[progress]&0 at any time.'
~
#3088
Paladin pendant command pray~
1 c 3
pray~
switch %cmd%
  case p
    return 0
    halt
done
set anti Anti-Paladin
set pendantstage %actor.quest_stage[paladin_pendant]%
switch %self.vnum%
  case 360
    if %actor.room% == 5479 && %pendantstage% == 1
      set continue yes
    endif
    break
  case 361
    if (%actor.room% == 17345 || %actor.room% == 17346 || %actor.room% == 17351 || %actor.room% == 17352) && %pendantstage% == 2
      set continue yes
    endif
    break
  case 362
    if %actor.room% == 49516 && %pendantstage% == 3
      set continue yes
    endif
    break
  case 363
    if %actor.room% == 2408 && %pendantstage% == 4
      set continue yes
    endif
    break
  case 364
    if %actor.room% == 55105 && %pendantstage% == 5
      set continue yes
    endif
    break
  case 365
    if %actor.room% == 53159 && %pendantstage% == 6
      set continue yes
    endif
    break
  case 366
    if %actor.room% == 58424 && %pendantstage% == 7
      set continue yes
    endif
    break
  case 367
    if %actor.room% == 48032 && %pendantstage% == 8
      set continue yes
    endif
    break
  case 368
    if %actor.room% == 48880 && %pendantstage% == 9
      set continue yes
    endif
done
if %continue% == yes
  oforce %actor% pray
  wait 2
  if %actor.class% == Paladin
    osend %actor% &3&bYou have gained new insights on your righteous cause!&0
  elseif %actor.class% == %anti%
    osend %actor% &1&bYou have gained new insights on your ruinouse cause!&0
  endif
  quest variable paladin_pendant %actor% necklacetask4 1
else
  return 0
endif
~
#3089
Paladin Pendant progress tracker~
0 d 1
status progress~
wait 2
msend %actor% &2&bDragon Slayer&0
if %actor.has_completed[dragon_slayer]%
  msend %actor% %self.name% says, 'The only dragons remaining are beasts of legend!'
elseif !%actor.quest_stage[dragon_slayer]% && %actor.level% > 4
  msend %actor% %self.name% says, 'You aren't on a hunt at the moment.'
elseif %actor.level% < 5
  msend %actor% %self.name% says, 'You're not quite ready to start taking on dragons.  Come back when you've seen a little more.'
elseif %actor.quest_variable[dragon_slayer:hunt]% == dead
  msend %actor% %self.name% says, 'Give me your current notice first.'
elseif %actor.level% >= (%actor.quest_stage[dragon_slayer]% - 1) * 10
  if %actor.quest_variable[dragon_slayer:hunt]% != running
    msend %actor% %self.name% says, 'You aren't on a hunt at the moment.'
  else
    switch %actor.quest_stage[dragon_slayer]% 
      case 1
        msend %actor% %self.name% says, 'You still have a dragon to slay.  If you can't even take out a dragon hedge, you'll never be ready for the real thing.'
        break
      case 2
        msend %actor% %self.name% says, 'You still have a dragon to slay.  Hunt down the green wyrmling in Morgan Hill.'
        break
      case 3
        msend %actor% %self.name% says, 'You still have a dragon to slay.  Hunt down Wug the Fiery Drakling.'
        break
      case 4
        msend %actor% %self.name% says, 'You still have a dragon to slay.  Hunt down the young blue dragon near the Tower in the Wastes.'
        break
      case 5
        msend %actor% %self.name% says, 'You still have a dragon to slay.  Search the forests in South Caelia for a faerie dragon.'
        break
      case 6
        msend %actor% %self.name% says, 'You still have a dragon to slay.  Kill that damn wyvern in the Highlands.'
        break
      case 7
        msend %actor% %self.name% says, 'You still have a dragon to slay.  Take down one of those ice lizards in Frost Valley.'
        break
      case 8
        msend %actor% %self.name% says, 'You still have a dragon to slay.  Find and vanquish the Beast of Borgan.'
        break
      case 9
        msend %actor% %self.name% says, 'You still have a dragon to slay.  Eliminate the dragon the Ice Cult up north worships.'
        break
      case 10
        msend %actor% %self.name% says, 'You still have a dragon to slay.  Destroy the mighty Hydra - and watch out for all its heads!'
    done
  endif
else
  msend %actor% %self.name% says, 'More dragons exist, but they're too dangerous without more experience.  Come back when you've seen a little more.'
endif
set anti Anti-Paladin
if %actor.class% == paladin || %actor.class% == %anti%
  msend %actor%  &0
  msend %actor% &2&bDivine Devotion&0
  set huntstage %actor.quest_stage[dragon_slayer]%
  set pendantstage %actor.quest_stage[paladin_pendant]%
  set job1 %actor.quest_variable[paladin_pendant:necklacetask1]%
  set job2 %actor.quest_variable[paladin_pendant:necklacetask2]%
  set job3 %actor.quest_variable[paladin_pendant:necklacetask3]%
  set job4 %actor.quest_variable[paladin_pendant:necklacetask4]%
  if %actor.level% < 10
    msend %actor% %self.name% says, 'You aren't ready for such an act yet.  Come back when you've grown a bit.'
    halt
  elseif %actor.level% < (%pendantstage% * 10)
    msend %actor% %self.name% says, 'You aren't ready for another devotional act yet.  Come back when you've gained some more experience.'
    halt
  elseif %actor.has_completed[paladin_pendant]%
    msend %actor% %self.name% says, 'You've already proven your devotion as much as possible!'
    halt
  endif
  if %pendantstage% == 0
    msend %actor% %self.name% says, 'Your first act of devotion should be to &6&b[hunt]&0 a dragon.'
    halt
  elseif (%pendantstage% >= %huntstage%) && !%actor.has_completed[dragon_slayer]%
    msend %actor% %self.name% says, 'Slay a few more dragons and then we can talk.'
    halt
  endif
  switch %pendantstage%
    case 1
      set necklace 12003
      set gem 55582
      set place The Mist Temple Altar
      set hint in the Misty Caverns.
      break
    case 2
      set necklace 23708
      set gem 55590
      set place Chamber of Chaos
      set hint in the Temple of Chaos.
      break
    case 3
      set necklace 58005
      set gem 55622
      set place Altar of Borgan
      set hint in the lost city of Nymrill.
      break
    case 4
      set necklace 48123
      set gem 55654
      set place A Hidden Altar Room
      set hint in a cave in South Caelia's Wailing Mountains.
      break
    case 5
      set necklace 12336
      set gem 55662
      set place The Altar of the Snow Leopard Order
      set hint buried deep in Mt. Frostbite
      break
    case 6
      set necklace 43019
      set gem 55677
      set place Chapel Altar
      set hint deep underground in a lost castle.
      break
    case 7
      set necklace 37015
      set gem 55709
      set place A Cliffside Altar
      set hint tucked away in the land of Dreams.
      break
    case 8
      set necklace 58429
      set gem 55738
      set place Dark Altar
      set hint entombed with an ancient evil king.
      break
    case 9
      set necklace 52010
      set gem 55739
      set place An Altar
      set hint far away in the Plane of Air.
      break
  done
  eval attack %pendantstage% * 100
  if %job1% || %job2% || %job3% || %job4% 
    msend %actor% %self.name% says, 'You've done the following:'
    if %job1%
      msend %actor% - attacked %attack% times
    endif
    if %job2%
      msend %actor% - found %get.obj_shortdesc[%necklace%]%
    endif
    if %job3%
      msend %actor% - found %get.obj_shortdesc[%gem%]%
    endif
    if %job4%
      msend %actor% - prayed in %place%
    endif
  endif
  msend %actor%
  msend %actor% You need to:
  if %job1% && %job2% && %job3% && %job4%
    msend %actor% Just give me your old necklace.
    halt
  endif
  if !%job1%
    eval remaining %attack% - %actor.quest_variable[paladin_pendant:attack_counter]%
    msend %actor% - attack &3&b%remaining%&0 more times while wearing your necklace.
  endif
  if !%job2%
    msend %actor% - find &3&b%get.obj_shortdesc[%necklace%]%&0
  endif
  if !%job3%
    msend %actor% - find &3&b%get.obj_shortdesc[%gem%]%&0
  endif
  if !%job4%
    msend %actor% - &3&bpray&0 in a place called "&3&b%place%&0".
    msend %actor%&0   It's &3&b%hint%&0
  endif
endif
~
#3090
Isilynor new contract~
0 d 0
I need a new notice~
wait 2
if %actor.level% >= (%actor.quest_stage[dragon_slayer]% - 1) * 10
  if %actor.quest_stage[dragon_slayer]%
    switch %actor.quest_stage[dragon_slayer]%
      case 1
        set notice 3080
        break
      case 2
        set notice 3081
        break
      case 3
        set notice 3082
        break
      case 4
        set notice 3083
        break
      case 5
        set notice 3084
        break
      case 6
        set notice 3085
        break
      case 7
        set notice 3086
        break
      case 8
        set notice 3087
        break
      case 9
        set notice 3088
        break
      case 10
        set notice 3089
        break
    done
    grumble
    mload obj %notice%
    give notice %actor%
    msend %actor% %self.name% says, 'Truly, be less caprecious.'
  endif
endif
~
#3091
Rhode's Pool of Blood~
1 j 100
~
wait 15
oecho %self.shortdesc%  creates a small pool of blood on the ground.
if (%actor.vnum% == -1)
oload obj 34
end
wait 2
return 0
~
#3092
Bigby Assistant refuse~
0 j 0
300 310 320 330 55577 55575 55574 55576~
switch %object.vnum%
  default
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    shake
    say I don't need this.
done
~
#3093
research_assistant_random~
0 b 15
~
sigh
say She was suppose to be here already.
wait 20
say She must be lost.
wait 20
say Bigby's not going to be happy about this.
~
#3094
bigby_assistant_relocate~
0 d 100
lost~
if (%actor.quest_stage[relocate_spell_quest]% < 1)
   if (%actor.level% >=65)
      if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer)
         say Yes, one of our research mages.
         wait 15
         sigh
         say She was researching a very powerful transportation spell.
         wait 15
         say I believe Bigby called the spell 'Relocate'.
         wait 15
         say But she hasn't returned yet. Can you please find her?
         say She mentioned going to a dark desert to find something.
         say But that is all I know. I'm sorry I can not help more.
         say Please find her!
      else
         say I'm sorry but this doesn't concern you. Bigby would be very upset.
         shake
      end if
   else
      say I'm sorry young one, but I don't think you can help just yet.
      pat %actor.name%
   end if
else
   say Yes Yes, I heard that you found her! Please bring her back safely.
end if
~
#3095
druid_moonwell_clue~
0 b 8
~
sigh
wait 15
say Such a shame she had to be punished.
~
#3096
druid_moonwell_clue_answer~
0 d 100
punished~
if (%actor.class% == druid)
   if (%actor.level% >= 73)
      wait 15
      say Yes, yes, but hopefully one day she'll be able to leave that tree.
      wait 15
   else
      wait 15
      say I'm sorry, but you are still too young.
      wait 5
      smile %actor.name%
      ruffle %actor.name%
   end if
else
  say I'm sorry, but it's okay.  Don't worry about it.
  smile %actor.name%
end if
~
#3103
parrot_repeat~
0 bd 100
cracker~
say %arg%
say hi
~
#3109
leather_ball_bounce~
1 c 1
bounce~
return 1
if %arg%
   if %actor.room% != %arg.room%
      osend %actor% Bounce the ball on who?  They don't seem to be here!
   else
      oteleport %arg% 1100
      oechoaround %actor% %actor.name% throws %self.shortdesc% at %arg.name%, bouncing it off %arg.p% forehead!
      oteleport %arg% %actor.room%
      osend %arg% %actor.name% throws %self.shortdesc% at you, bouncing it off your forehead!
      osend %actor% You launch %self.shortdesc% at %arg.name%, bouncing it off %arg.p% forehead!
   endif
else
   oechoaround %actor% %actor.name% bounces %self.shortdesc% on the ground a few times.
   osend %actor% You dribble %self.shortdesc% around yourself a little bit.
endif
~
#3112
bird_whistle~
1 c 1
whistle~
switch %cmd%
  case w
  case wh
  case whi
  case whis
    return 0
    halt
done
oechoaround %actor% %actor.name% places a bird whistle to %actor.p% lips and lets out a twitter.
osend %actor% You blow on a bird whistle, making a twittering noise.
~
#3113
**UNUSED**~
1 c 1
whis~
return 0
* This trigger forces the default action for "whis" instead of activating the
* whistle trigger.
~
#3114
finger_trap_remove~
1 l 100
~
if %actor.level% < 100
  if %random.200% > %actor.real_int%
    return 0
    oechoaround %actor% %actor.name% struggles, trying to remove a finger trap!
    osend %actor% You struggle, but can't seem to remove the finger trap!
  endif
endif
~
#3115
bite or wag animals~
0 e 0
pets you lovingly.~
if %actor.cha% > 65
lick %actor%
wheal %actor% 2
else 
bite %actor%
wdamage %actor% -5
endif
~
#3116
UNUSED~
1 j 100
~
if ((%actor.worn[1]% == -1) && (%actor.worn[2] == -1))
    return 1
else
    return 0
    osend %actor% You need two fingers free to wear this.
endif
~
#3117
**UNUSED**~
1 c 1
s~
* This trigger makes sure the command "s" does its normal function,
* instead of triggering the squeeze trigger.
return 0
~
#3118
squeeze_zzur~
1 c 1
squeeze~
switch %cmd%
  case s
    return 0
    halt
done
if (%arg% != plushie) && (%arg% != zzur-plushie) && (%arg% != zzur) && (%arg% != toy) && (%arg% != little-plushie) && (%arg% != little)
  return 0
  halt
endif
return 1
switch %random.9%
  case 1
    set phrase Done yet?
    break
  case 2
    set phrase Be nice to the newbies.
    break
  case 3
    set phrase AFK.
    break
  case 4
    set phrase Uh huh.
    break
  case 5
    set phrase Thoughts? Suggestions?
    break
  case 6
    set phrase PHRASE6
    break
  case 7
    set phrase PHRASE7
    break
  case 8
    set phrase PHRASE8
    break
  case 9
    set phrase PHRASE9
    break
  default
    set phrase PHRASE0
    break
done
oecho %actor.name% squeezes the Zzur plushie's belly.
oecho The Zzur plushie says, '%phrase%'
~
#3119
**UNUSED**~
1 c 1
pr~
* This trigger makes sure the command "pr" does its normal function,
* instead of triggering the press trigger.
return 0
~
#3120
press_kourrya~
1 c 1
press~
switch %cmd%
  case p
  case pr
    return 0
    halt
done
if (%arg% != button) && (%arg% != figurine) && (%arg% != kourrya-figurine) && (%arg% != figurine) && (%arg% != toy) && (%arg% != tiny) && (%arg% != figure) && (%arg% != robed) && (%arg% != woman)
   return 0
   halt
endif
return 1
switch %random.9%
   case 1
      set phrase Find it yourself.
      break
   case 2
      set phrase Finish your existing quest first.
      break
   case 3
      set phrase Your soul belongs to me!
      break
   case 4
      set phrase PHRASE4
      break
   case 5
      set phrase PHRASE5
      break
   case 6
      set phrase PHRASE6
      break
   case 7
      set phrase PHRASE7
      break
   case 8
      set phrase PHRASE8
      break
   case 9
      set phrase PHRASE9
      break
   default
      set phrase PHRASE0
      break
done
oecho %actor.name% presses the button in the Kourrya figurine's back.
oecho The Kourrya figurine says, '%phrase%'
~
#3121
**UNUSED**~
1 c 1
pi~
* This trigger makes sure the command "pi" does its normal function
* instead of triggering the pinch trigger.
return 0
~
#3122
pinch_laoris~
1 c 1
pinch~
switch %cmd%
  case p
  case pi
    return 0
    halt
done
if (%arg% != puppet) && (%arg% != laoris) && (%arg% != laoris-puppet) && (%arg% != toy) && (%arg% != little) && (%arg% != colorful) && (%arg% != hand) && (%arg% != hand-puppet)
  return 0
  halt
endif
return 1
switch %random.9%
  case 1
    set phrase Rarr!
    break
  case 2
    set phrase Hey want to try my mob's new combat proc?
    break
  case 3
    set phrase Teeehehehehehe!
    break
  case 4
    set phrase PHASE4
    break
  case 5
    set phrase PHRASE5
    break
  case 6
    set phrase PHRASE6
    break
  case 7
    set phrase PHRASE7
    break
  case 8
    set phrase PHRASE8
    break
  case 9
    set phrase PHRASE9
    break
  default
    set phrase PHRASE0
    break
done
oecho %actor.name% pinches the Laoris puppet.
oecho The Laoris puppet says, '%phrase%'
~
#3123
squeeze_ehlissa~
1 c 1
squeeze~
switch %cmd%
  case s
    return 0
    halt
done
if (%arg% != doll) && (%arg% != ehlissa) && (%arg% != miniature-ehlissa-doll) && (%arg% != miniature) && (%arg% != little) && (%arg% != little-ehlissa-doll) && (%arg% != ehlissa-doll)
  return 0
  halt
endif
return 1
oecho %actor.name% squeezes %hisher% doll.
set i 99
while %i% > 0
  oecho %self.shortdesc% sings, '%i% bottles of beer on the wall...'
  wait 3s
  oecho %self.shortdesc% sings, '%i% bottles of beer...'
  wait 3s
  oecho %self.shortdesc% sings, 'Take one down, pass it around...'
  wait 3s
  eval i %i% - 1
  oecho %self.shortdesc% sings, '%i% bottles of beer on the wall!'
  wait 7s
done
~
#3124
**UNUSED**~
1 c 1
pa~
* This trigger makes sure the command "pa" does its normal function
* instead of triggering the pat trigger.
return 0
~
#3125
pat_chinok~
1 c 1
pat~
switch %cmd%
  case p
  case pa
    return 0
    halt
done
eval test1 (%arg% != doll) && (%arg% != chinok) && (%arg% != chinok-doll) && (%arg% != chinok-rag) && (%arg% != rag -doll) && (%arg% != rag) && (%arg% != chinok-rag-doll)
eval test2 (%arg% != little) && (%arg% != hooded) && (%arg% != figure) && (%arg% != little-hooded) && (%arg% != hooded -figure) && (%arg% != little-hooded-figure)
if %test1% && %test2%
  return 0
  halt
endif
return 1
oechoaround %actor% %actor.name% pats a Chinok rag doll on its head.
osend %actor% You pat a Chinok rag doll on its head.
oecho The Chinok rag doll swings its lightsabers dangerously, narrowly missing you!
~
#3130
Bells Exit~
1 q 100
~
oechoaround %actor% %actor.name% bells ring as she leaves the room.
osend %actor% The bells on your hat ring as you walk.
~
#3131
Bells Entry~
1 i 100
~
oechoaround %actor% %actor.name% bells ring as she enters the room.
~
#3133
Random Bells~
1 bgjl 100
~
if %actor%
    * this is a wear trigger
    set myowner %actor%
    global myowner
    oechoaround %myowner% Bells ring on %myowner.name%'s hat as %myowner.n% moves.
    osend %myowner% The bells on your hat ring as you shift your position.
else
    * this is a random trigger
    if %myowner%
        if %myowner.wearing[3306]%
            * the global variable was set (by the wear trigger)
            oechoaround %myowner% Bells ring on %myowner.name%'s hat as %myowner.n% moves.
            osend %myowner% The bells on your hat ring as you shift your position.
        else
            * hat not worn, so assume it was removed
            unset myowner
        end
    end
end
~
#3140
Freddy random load doll~
0 bd 1
refresh~
* Generate a random number
set p %random.15%
* If this is being triggered by an imm as a speech trig, ensure something loads
if %actor% && %actor.level% > 100
  set p 15
endif
if %p% != 15
  halt
endif
 
* Figure out which object to load.  Better chance for lower vnums.
set p %random.100%
if %p% < 31
  eval p 3139 + %random.7%
elseif %p% < 82
  eval p 3146 + %random.35%
elseif %p% < 93
  eval p 3181 + %random.10%
else
  eval p 3191 + %random.7%
endif
 
* In case one of the randoms returned 0 somehow
if %p% == 3139
  set p 3140 + %random.58%
endif
mload obj %p%
 
* Special handling for certain dolls:
* Imanhotep's sarcophagus
if %p% == 3182
  mload obj 3199
  mat 1100 open sarcophagus
  mat 1100 put imanhotep sarcophagus
  mat 1100 close sarcophagus
* The Chosen boxed set
elseif %p% == 3198
  mload obj 3175
  mload obj 3176
  mload obj 3177
  mload obj 3178
  mload obj 3179
  mat 1100 open boxed-set
  mat 1100 put chosen boxed-set
  mat 1100 put chosen boxed-set
  mat 1100 put chosen boxed-set
  mat 1100 put chosen boxed-set
  mat 1100 put chosen boxed-set
  mat 1100 close boxed-set
endif
~
#3150
Intercity transport questmasters greet~
0 g 100
~
* This is a greet trigger for the transportation assistants who
* move newbies between cities.
switch %self.vnum%
case 30075
case 3151
   set myname barbarian
   break
case 3152
   set myname dwarf
   break
case 30074
   set myname drow
   break
case 30076
   set myname human
   break
case 30077
   set myname orc
   break
case 3150
default
   set myname elf
   break
done
if %actor.level% < 16 && %actor.vnum% == -1 && %actor.quest_stage[intercity_transport]% == 0
   msend %actor% &bThe %myname% tells you, 'Would you like a trip to a faraway city?'&0
   msend %actor% &bThe %myname% tells you, 'If so, just ask me &7&bYes&0&b and I'll tell you all about it.'&0
end
~
#3151
Intercity transport questmasters talk~
0 n 100
yes Yes help Help tell~
if %actor.quest_stage[intercity_transport]% == 0
   quest start intercity_transport %actor.name%
endif
switch %self.vnum%
   case 3151
   case 30075
      set myname barbarian
      set dest Ickle
      break
   case 3152
      set myname dwarf
      set dest Anduin
      break
   case 30074
      set myname drow
      set dest Anduin
      break
   case 30076
      set myname human
      set dest Mielikki
      break
   case 30077
      set myname orc
      set dest Ogakh
      break
   case 3150
   default
      set myname elf
      set dest Mielikki
done
wait 3
if %actor.level% < 16
   msend %actor% &bThe %myname% tells you, 'As long as you're under level 16, I can transport you to %dest%.'&0
   msend %actor% &bThe %myname% tells you, 'Just ask me 'transport', and I'll send you.'&0
else
   msend %actor% &bThe %myname% tells you, 'I send people below level 16 to %dest%.'&0
   msend %actor% &bThe %myname% tells you, 'Unfortunately, you are too powerful.'&0
end
~
#3153
**UNUSED**~
0 c 100
tra~
return 0
~
#3154
Transport agents ask~
0 n 100
transport~
* This is the ask trigger for intercity transport questmasters.
* It allows level 1-15 people to get transport by asking to
* Anduin, Ickle, or Mielikki.
* Much of this trigger is a copy of 3152.
*******************
** GENERAL SETUP **
*******************
set minlevel 1
set maxlevel 15
switch %self.vnum%
   * This is the barbarian, who sends people to Ickle.
   case 3151
   case 30075
      set myname barbarian
      set dvnum 10046
      set dsname Ickle
      break
   case 3152
   * This is the dwarf, who sends people to Anduin.
      set myname dwarf
      set dvnum 6015
      set dsname Anduin
      break
   case 30074
      set myname drow
      set dvnum 6015
      set dsname Anduin
      break
   case 30076
      set myname human
      set dvnum 3016
      set dsname Mielikki
      break
   case 30077
      set myname orc
      set dvnum 30115
      set dsname Ogakh
      break
   case 3150
   default
   * This is the elf, who sends people to Mielikki.
      set myname elf
      set dvnum 3016
      set dsname Mielikki
      break
done
wait 4
***************************
** Check for eligibility **
***************************
if %actor.level% > 99
   eyebrow %actor.name%
   msend %actor% &bThe %myname% tells you, 'Just goto %dvnum%.'
   halt
elseif %minlevel% > %actor.level%
   shake
   msend %actor% &bThe %myname% tells you, 'You are not experienced enough to go to %dsname%.'&0
   halt
elseif %actor.level% > %maxlevel%
   eyebrow %actor.name%
   msend %actor% &bThe %myname% tells you, 'You are too powerful for me to transport.'&0
   halt
endif
***************************
** Set origination color **
***************************
* The color says where they came from.
* It is seen by anyone standing at the destination.
if %self.room% == 3016
   * Mielikki - GREEN
   set dcolor &2green&0
elseif %self.room% == 6015
   * Anduin - RED
   set dcolor &1red&0
elseif %self.room% == 30115
   * Ogakh - GRAY
   set dcolor &9&bgray&0
else
   * Ickle - BLUE
   set dcolor &4blue&0
end
   
***********************
** Perform transport **
***********************
mechoaround %actor% %self.name% nods briefly to %actor.name%.
msend %actor% %self.name% nods briefly to you.
wait 4
mechoaround %actor% %self.name% makes a magical gesture at %actor.name%.
msend %actor% %self.name% makes a magical gesture at you.
mechoaround %actor% %actor.name% disappears in a cloud of gray smoke.
mteleport %actor% %dvnum%
mat %dvnum% mechoaround %actor% %actor.name% arrives in a cloud of %dcolor% smoke.
mforce %actor% look
~
#3155
no training~
0 c 100
train~
switch %cmd%
  case t
  case tr
  case tra
    return 0
    halt
done
msend %actor% %self.name% says, 'You don't need to use a "training" or "practice" command.  Skills increase gradually as you use them.  The words don't change often, but your skills are getting better!'
~
#3156
trainers not level gain~
0 c 100
level~
switch %cmd%
  case l
  case le
    return 0
    halt
done
msend %actor% %self.name% says, 'Sorry, I'm not a guild master.'
wait 2
set warriors warrior paladin ranger monk berserker anti-paladin
set clerics cleric druid priest diabolist
set rogues rogue bard mercenary assassin thief
set sorcerers sorcerer cryomancer pyromancer illusionist necromancer
if %self.vnum% == 3165
  * Calken
  if %warriors% /= %actor.class%
    msend %actor% %self.name% says, 'Your guild master is right around the corner!  Go west and north.'
  elseif %clerics% /= %actor.class%
    msend %actor% %self.name% says, 'The Cleric Guild Masters are north of Town Center by the Temple to Mielikki.'
  elseif %rogues% /= %actor.class%
    msend %actor% %self.name% says, 'The Rogue Guild is known to be somewhere near the jewelry shop in the south-west corner of Town Center.  But don't go to Julk's training grounds!'
  elseif %sorcerers% /= %actor.class%
    msend %actor% %self.name% says, 'The Sorcerer Guild Masters are located near Bigby's Magic Shope just west and south of Town Center.  But don't go to Fecil's study hall!'
  endif
elseif %self.vnum% == 3170
  * Fecil
  if %warriors% /= %actor.class%
    msend %actor% %self.name% says, 'The Warrior Guild is located behind Santiago's Weapon Shop just east of Town Center.  But don't go to Calken in the training grounds!'
  elseif %clerics% /= %actor.class%
    msend %actor% %self.name% says, 'The Cleric Guild Masters are north of Town Center by the Temple to Mielikki.'
  elseif %rogues% /= %actor.class%
    msend %actor% %self.name% says, 'The Rogue Guild is known to be somewhere near the jewelry shop in the south-west corner of Town Center.'
  elseif %sorcerers% /= %actor.class%
    msend %actor% %self.name% says, 'You guild master is just around the corner!  Go north and east.'
  endif
elseif %self.vnum% == 3160
  * Julk
  if %warriors% /= %actor.class%
    msend %actor% %self.name% says, 'The Warrior Guild is located behind Santiago's Weapon Shop just east of Town Center.  But don't go to Calken in the training grounds!'
  elseif %clerics% /= %actor.class%
    msend %actor% %self.name% says, 'The Cleric Guild Masters are north of Town Center by the Temple to Mielikki.'
  elseif %rogues% /= %actor.class%
    msend %actor% %self.name% says, 'Your guild master is right around the corner!  Go north and down.'
  elseif %sorcerers% /= %actor.class%
    msend %actor% %self.name% says, 'The Sorcerer Guild Masters are located near Bigby's Magic Shope just west and south of Town Center.  But don't go to Fecil's study hall!'
  endif
endif
~
#3157
**UNUSED**~
0 c 100
l le~
return 0
~
#3158
**UNUSED**~
0 c 100
pra~
return 0
~
#3159
no practice~
0 c 100
practice~
switch %cmd%
  case p
  case pr
  case pra
    return 0
    halt
done
msend %actor% %self.name% says, 'You don't need to use a "training" or "practice" command.  Skills increase gradually as you use them.  The words don't change often, but your skills are getting better!'
~
#3160
***skill trainer speach***~
0 dn 100
training yes backstab bludgeoning shadow slashing sneak steal switch throatcut track conceal corner double dual eye switch retreat hide pick piercing bandage douse lure cartwheel rend~
wait 2
*make sure they have a quest record for saving variables
if %actor.quest_stage[trainer_3160]% < 1
   quest start trainer_3160 %actor.name%
else
   quest erase trainer_3160 %actor.name%
   quest start trainer_3160 %actor.name%
endif
*introductions: lists the skills available
if %speech% == training? || %speech% == training || %speech% == yes
   mecho %self.name% says, 'Sure, I can help you improve just about any necessary stealthy
   mecho &0talent.  But remember, I can't teach you skills you don't already know.'
   mecho  
   mecho &0Just ask me about a skill, and I'll give you a quote:
   mecho   
   mecho &0backstab, bandage, bludgeoning weapons, cartwheel, conceal, corner
   mecho &0double attack, douse, dual wield, eye gouge, group retreat, hide, lure, pick lock,
   mecho &0piercing weapons, rend, retreat, shadow, slashing weapons, sneak, sneak attack,
   mecho &0steal, switch, throatcut, track.
   halt
else
   *defining variables for this script
   set name %actor.name%
   set cha %actor.real_cha%
   set int %actor.real_int%
   *
   *Identify the skill to be trained
   if %speech% == backstab         
      set skill %actor.skill[backstab]%
   elseif %speech% == bludgeoning weapons
      set skill %actor.skill[bludgeoning weapons]%
      set word2 weapons
   elseif %speech% == cartwheel
      set skill %actor.skill[cartwheel]%
   elseif %speech% == conceal           
      set skill %actor.skill[conceal]%
   elseif %speech% == corner             
      set skill %actor.skill[corner]%
   elseif %speech% == double attack 
      set skill %actor.skill[double attack]%
      set word2 attack
   elseif %speech% == dual wield
      set skill %actor.skill[dual wield]%
      set word2 wield
   elseif %speech% == eye gouge      
      set word2 gouge 
      set skill %actor.skill[eye gouge]%
   elseif %speech% == group retreat   
      set word2 retreat
      set skill %actor.skill[group retreat]%
   elseif %speech% == hide 
      set skill %actor.skill[hide]% 
   elseif %speech% ==instant kill 
      set skill %actor.skill[instant kill]%
      set word2 kill
   elseif %speech% == lure
      set skill %actor.skill[lure]%
   elseif %speech% == pick lock
      set skill %actor.skill[pick lock]%
      set word2 lock
   elseif %speech% == piercing weapons 
      set skill %actor.skill[piercing weapons]%
      set word2 weapons
   elseif %speech% == rend
      set skill %actor.skill[rend]%
   elseif %speech% == retreat 
      set skill %actor.skill[retreat]%    
   elseif %speech% == shadow           
      set skill %actor.skill[shadow]%
   elseif %speech% == slashing weapons       
      set skill %actor.skill[slashing weapons]%
      set word2 weapons
   elseif %speech% == sneak         
      set skill %actor.skill[sneak]%
   elseif %speech% == sneak attack
      set skill %actor.skill[sneak attack]%
      set word2 attack
   elseif %speech% == steal                
      set skill %actor.skill[steal]%
   elseif %speech% == switch             
      set skill %actor.skill[switch]%     
   elseif %speech% == throatcut          
      set skill %actor.skill[throatcut]%
   elseif %speech% == track                
      set skill %actor.skill[track]%
   elseif %speech% == bandage                
      set skill %actor.skill[bandage]%
   elseif %speech% == douse
      set skill %actor.skill[douse]%
   else
      say I'm not familiar with %speech%.
      halt
   endif
   *
   * Check to see if a skill should be taught at all
   *
   eval skill 10 * %skill%
   eval maxskill %actor.level% * 10 + 60
   *check for capped skills
   eval cap 1000
   if %speech% == track
      switch %actor.class%
         case Mercenary
            eval cap 850
            break
         case Rogue
            eval cap 650
            break
         case Thief
            eval cap 600
            break
         case Assassin
            eval cap 750
      done
   elseif %speech% == dodge || %speech% == parry
      switch %actor.race%
         case ogre
         case troll
            eval cap 700
      done
   elseif %speech% == riposte
      switch %actor.race%
         case ogre
            set cap 700
      done
   endif
   if %skill% < 50
      mecho %self.name% says, 'I wouldn't know where to start.
      Lets talk about improving a skill you actually know.'
      halt
   elseif %skill% >= %cap% || %skill% >= %maxskill%
      say There is nothing left to teach you. You've mastered %speech%!
      halt
   endif
   if %cap% < %maxskill%
      set maxskill %cap%
   endif
   * This portion is for smooth speech indicating the effects of a players cha or int
score.
   if %cha% > 70
      say I like you.
      if %int% > 70
         mecho  
         mecho %self.name% says, '...and you're pretty bright.
         mecho &0I'll give you a good deal.'
      elseif %int% < 50
         mecho  
         say ..but you aren't the smartest.
      endif
   elseif %cha% < 50
      say I don't like you much.
      if %int% > 70
         mecho  
         say ...but you are pretty bright.
      elseif %int% < 50
         mecho  
         say ...and you aren't the smartest.
      else
      endif
   else
      say You're alright.
      if %int% > 70
         mecho  
         say ...and you're pretty bright.
      elseif %int% < 50
         mecho  
         say ...but you aren't the smartest.
      endif
   endif
   mecho   
   *Now a price is calculated
   eval change %maxskill% - %skill%
   eval opinion 200 - (%cha% + %int%)
   eval price %change% * %opinion% * %actor.level% * %actor.level% / (%skill% * 20)
   *
   *now the price in copper has to be divided into coinage.
   eval plat %price% / 1000
   eval gold %price% / 100 - %plat% * 10
   eval silv %price% / 10 - %plat% * 100 - %gold% * 10
   eval copp %price%  - %plat% * 1000 - %gold% * 100 - %silv% * 10
   *now the price can be reported
   say I'll teach you %speech% for %plat% platinum, %gold% gold, %silv% silver, %copp% copper.
   mecho  
   say Just bring me the money when you're ready to practice.
   quest variable trainer_3160 %actor.name% skill_name %speech%
   if %word2%
      quest variable trainer_3160 %actor.name% word2 %word2%
   endif
   quest variable trainer_3160 %actor.name% skill_level %skill%
   quest variable trainer_3160 %actor.name% price %price%
   quest variable trainer_3160 %actor.name% actor_level %actor.level%
endif
~
#3161
pay for training~
0 m 1
~
if %actor.quest_variable[trainer_3160:word2]%
   set full_skill %actor.quest_variable[trainer_3160:skill_name]% %actor.quest_variable[trainer_3160:word2]%
elseif  %actor.quest_variable[trainer_3160:skill_name]%
   set full_skill %actor.quest_variable[trainer_3160:skill_name]%
endif
wait 1s
if %actor.quest_variable[trainer_3160:actor_level]% == %actor.level%
   if %value% >= %actor.quest_variable[trainer_3160:price]%
      grin %actor.name%
      wait 1s
      msend %actor% %self.name% says, 'Fantastic. Let's get started...'
      wait 5s
      msend %actor% Some time passes...
      wait 3s
      msend %actor% You feel you are getting the hang of things.
      wait 3s
      msend %actor% You feel your skill in %full_skill% improving dramatically!
      mskillset %actor% %full_skill%
      quest erase trainer_3160 %actor.name%
   else
      msend %actor% %self.name% says, 'I appreciate your voluntary donation, but I'm afraid that's all it was.'
      snicker %actor%
      msend %actor% &0  
      msend %actor% %self.name% says, 'I don't accept installments.  It's got to be all up front.'
   endif
else
   return 0
   wait 2s
   consider %actor%
   wait 2s
   ponder
   wait 2s
   msend %actor% %self.name% says, 'Something's different about you.  What skill were you going to train again?'
   quest erase trainer_3160 %actor.name%
endif
~
#3162
Julks greet~
0 g 100
~
wait 1s
if %actor.quest_stage[trainer_3160]% == 1
  if %actor.quest_variable[trainer_3160:actor_level]% == %actor.level%
    msend %actor% %self.name% says, 'Have you returned to be trained in %actor.quest_variable[trainer_3160:skill_name]%?'
    halt
  else
    msend %actor% %self.name% says, 'You've been out adventuring, have you?  Perhaps you could use some training?'
  endif
else
  tell %actor.name% Greetings adventurer.  Improving your skills taking too long?  I can help you for a fee.
endif
~
#3163
julks fight trigger~
0 k 100
~
set mode %random.10%
set target %self.fighting%
set tank %target.fighting%
if %target%
   if %tank% != %self%
      if %mode% > 5
         backstab
      elseif %mode% > 2
         kick
      else
         corner
      endif
   elseif %tank% == %self%
      if %mode% > 6
         kick
      else
         bash
      endif
   endif
   wait 3s
endif
~
#3164
mrs julk fight trigger~
0 k 100
~
set mode %random.10%
set target %self.fighting%
set tank %target.fighting%
if %target%
   if %tank% != %self%
      if %mode% > 3
         backstab
      else
         kick
      endif
   elseif %tank% == %self%
      if %mode% > 6
         kick
      endif
   endif
   wait 3s
endif
~
#3165
Calken trainer speech~
0 d 100
training yes 2H dodge parry riposte hitall barehand bash disarm first guard kick mount rescue riding safefall springleap tame tantrum maul berserk ground battle roundhouse~
wait 2
*make sure they have a quest record for saving variables
if %actor.quest_stage[trainer_3165]% < 1
   quest start trainer_3165 %actor.name%
else
   quest erase trainer_3165 %actor.name%
   quest start trainer_3165 %actor.name%
endif
*introductions: lists the skills available
if %speech% == training? || %speech% == training || %speech% == yes
   mecho %self.name% says, 'Sure, I can help you improve just about any necessary martial
   mecho &0talent.  But remember, I can't teach you skills you don't already know.'
   mecho  
   mecho &0Just ask me about a skill, and I'll give you a quote:
   mecho   
   mecho &02H bludgeoning weapons, 2H piercing weapons, 2H slashing weapons, barehand,
   mecho &0bash, battle howl, berserk, disarm, dodge, first aid, ground shaker, guard,
   mecho &0hitall, kick, maul, mount, parry, rescue, riding, riposte, roundhouse, safefall, 
   mecho &0springleap, tame, tantrum.
   wait 2s
   say Feel free to ask me about one of them and I'll give you a price.
   halt
else
   *defining variables for this script
   set speech %speech%
   set name %actor.name%
   set cha %actor.real_cha%
   set int %actor.real_int%
   *
   *Identify the skill to be trained
   if %speech% == bash                
      set skill %actor.skill[bash]%
   elseif %speech% /= 2H
      if %speech% /= bludgeoning
         set word2 bludgeoning
         set skill %actor.skill[2H bludgeoning weapons]%
      elseif %speech% /= piercing 
         set word2 piercing
         set skill %actor.skill[2H piercing weapons]%
      elseif %speech% /= slashing 
         set word2 slashing
         set skill %actor.skill[2H slashing weapons]%
      else
         say Not sure that I've heard of that one.  What was it again?
         halt
      endif
   elseif %speech% == disarm
      set skill %actor.skill[disarm]%
   elseif %speech% == dodge
      set skill %actor.skill[dodge]%
   elseif %speech% == parry                
      set skill %actor.skill[parry]%
   elseif %speech% == riposte             
      set skill %actor.skill[riposte]%
   elseif %speech% == hitall             
      set skill %actor.skill[hitall]%
   elseif %speech% == barehand           
      set skill %actor.skill[barehand]%
   elseif %speech% == springleap             
      set skill %actor.skill[springleap]%
   elseif %speech% == safefall             
      set skill %actor.skill[safefall]%
   elseif %speech% == bash             
      set skill %actor.skill[bash]%
   elseif %speech% == guard
      set skill %actor.skill[guard]%   
   elseif %speech% == rescue            
      set skill %actor.skill[rescue]%
   elseif %speech% == kick                  
      set skill %actor.skill[kick]%
   elseif %speech% == first aid
      set word2 aid
      set skill %actor.skill[first aid]%
   elseif %speech% == mount
      set skill %actor.skill[mount]%
   elseif %speech% == riding            
      set skill %actor.skill[riding]%
   elseif %speech% == tame
      set skill %actor.skill[tame]%
   elseif %speech% == ground shaker
      set word2 shaker
      set skill %actor.skill[ground shaker]%
   elseif %speech% == battle howl
      set word2 howl
      set skill %actor.skill[battle howl]%
   elseif %speech% == maul
      set skill %actor.skill[maul]%
   elseif %speech% == tantrum
      set skill %actor.skill[tantrum]%
   elseif %speech% == berserk
      set skill %actor.skill[berserk]%
   elseif %speech% == roundhouse
      set skill %actor.skill[roundhouse]%
   else
      say I'm not familiar with %speech%.
      halt
   endif
   *
   * Check to see if a skill should be taught at all
   *
   eval skill 10 * %skill%
   eval maxskill %actor.level% * 10 + 60
   *check for capped skills
   eval cap 1000
   if %speech% == track
      switch %actor.class%
         case Mercenary
            eval cap 850
            break
         case Rogue
            eval cap 650
            break
         case Thief
            eval cap 600
            break
         case Assassin
            eval cap 750
      done
   elseif %speech% == dodge || %speech% == parry
      switch %actor.race%
         case ogre
         case troll
            eval cap 700
      done
   elseif %speech% == riposte
      switch %actor.race%
         case ogre
            set cap 700
      done
   endif
   if %skill% < 50
      mecho %self.name% says, 'I wouldn't know where to start.
      mecho &0Let's talk about improving a skill you actually know.'
      halt
   elseif %skill% >= %cap% || %skill% >= %maxskill%
      say There is nothing left to teach you - you've mastered %speech%!
      halt
   endif
   if %cap% < %maxskill%
      set maxskill %cap%
   endif
   * This portion is for smooth speech indicating the effects of a players cha or int
score.
   if %cha% > 70
      say I like you.
      if %int% > 70
         mecho   
         mecho %self.name% says, '...and you're pretty bright.
         mecho I'll give you a good deal.'
      elseif %int% < 50
         mecho   
         say ...but you aren't the smartest.
      endif
   elseif %cha% < 50
      say I don't like you much.
      if %int% > 70
         mecho   
         say ...but you are pretty bright.
      elseif %int% < 50
         mecho   
         say ...and you aren't the smartest.
      else
      endif
   else
      say You're alright.
      if %int% > 70
         mecho  
         say ...and you're pretty bright.
      elseif %int% < 50
         mecho  
         say ...but you aren't the smartest.
      endif
   endif
   mecho  
   *Now a price is calculated
   eval change %maxskill% - %skill%
   eval opinion 200 - (%cha% + %int%)
   eval price %change% * %opinion% * %actor.level% * %actor.level% / (%skill% * 20)
   *
   *now the price in copper has to be divided into coinage.
   eval plat %price% / 1000
   eval gold %price% / 100 - %plat% * 10
   eval silv %price% / 10 - %plat% * 100 - %gold% * 10
   eval copp %price%  - %plat% * 1000 - %gold% * 100 - %silv% * 10
   *now the price can be reported
   say I'll teach you %speech% for %plat% platinum, %gold% gold, %silv% silver, %copp% copper.
   mecho  
   say Just get me the money, and I'll get started.
   quest variable trainer_3165 %actor.name% skill_name %speech%
   if %word2%
      quest variable trainer_3165 %actor.name% word2 %word2%
   endif
   quest variable trainer_3165 %actor.name% skill_level %skill%
   quest variable trainer_3165 %actor.name% price %price%
   quest variable trainer_3165 %actor.name% actor_level %actor.level%
endif
~
#3166
pay for training~
0 m 1
~
if %actor.quest_variable[trainer_3165:word2]%
   set full_skill %actor.quest_variable[trainer_3165:skill_name]% %actor.quest_variable[trainer_3165:word2]%
elseif %actor.quest_variable[trainer_3165:skill_name]%
   set full_skill %actor.quest_variable[trainer_3165:skill_name]%
endif
wait 1s
if %actor.quest_variable[trainer_3165:actor_level]% == %actor.level%
   if %value% >= %actor.quest_variable[trainer_3165:price]%
      grin %actor.name%
      wait 1s
      msend %actor% %self.name% says, 'Fantastic. Let's get started...'
      wait 5s
      msend %actor% Some time passes...
      wait 3s
      msend %actor% You feel you are getting the hang of things.
      wait 3s
      msend %actor% You feel your skill in %full_skill% improving dramatically!
      mskillset %actor% %full_skill%
      quest erase trainer_3165 %actor.name%
   else
      msend %actor% %self.name% says, 'I appreciate your voluntary donation, but I'm afraid that's all it was.'
      snicker %actor%
      msend %actor% %self.name% says, 'I don't accept installments.  It's got to be all up front.'
   endif
else
   return 0
   wait 2s
   consider %actor%
   wait 2s
   ponder
   wait 2s
   msend %actor% %self.name% says, 'Something is different about you.  What skill were you going to train again?'
   quest erase trainer_3165 %actor.name%
endif
~
#3167
Calken greet~
0 g 100
~
wait 1s
if %actor.quest_stage[trainer_3165]% == 1
  if %actor.quest_variable[trainer_3165:lvl]%
    if %actor.quest_variable[trainer_3165:lvl]% == %actor.level%
      msend %actor% %self.name% says, 'Have you returned to be trained in %actor.quest_variable[trainer_3165:skill_name]%?
      halt
  endif
  endif
  msend %actor% %self.name% says, 'You've been out adventuring, have you?  Perhaps you could use some training?'
else
  tell %actor.name% Greetings adventurer.  Improving your skills taking too long?  I can help you for a fee.
endif
~
#3170
shaman trainer speech~
0 dn 100
training yes spell sphere chant scribe meditate vampiric breathe perform~
wait 2
*make sure they have a quest record for saving variables
if %actor.quest_stage[trainer_3170]% < 1
   quest start trainer_3170 %actor.name%
else
   quest erase trainer_3170 %actor.name%
   quest start trainer_3170 %actor.name%
endif
*introductions: lists the skills available
if %speech% == training? || %speech% == training || %speech% == yes
   mecho %self.name% says, 'Sure, I can help you improve just about any necessary magical
   mecho &0talent.  But remember, I can't teach you skills you don't already know.'
   mecho  
   mecho &0Just ask me about a skill, and I'll give you a quote:
   mecho  
   mecho &0breathe acid, breathe fire, breathe frost, breathe gas, breathe lightning,  
   mecho &0chant, meditate, perform, scribe, spell knowledge, sphere of air, 
   mecho &0sphere of death, sphere of divination, sphere of earth, sphere of enchantment, 
   mecho &0sphere of fire, sphere of generic, sphere of healing, sphere of protection,
   mecho &0sphere of summoning, sphere of water, quick chant. 
   halt
else
   *defining variables for this script
   set name %actor.name%
   set cha %actor.real_cha%
   set int %actor.real_int%
   *
   *Identify the skill to be trained
   if %speech% == spell knowledge
      set word2 knowledge
      set skill %actor.skill[spell knowledge]%
   elseif %speech% /= sphere
      if %speech% /= air
         set word2 air
         set skill %actor.skill[sphere of air]%
      elseif %speech% /= death   
         set word2 death
         set skill %actor.skill[sphere of death]%
      elseif %speech% /= divination   
         set word2 divination
         set skill %actor.skill[sphere of divination]%
      elseif %speech% /= earth   
         set word2 earth
         set skill %actor.skill[sphere of earth]%
      elseif %speech% /= enchantment
         set word2 enchantment
         set skill %actor.skill[sphere of enchantment]%
      elseif %speech% /= fire
         set word2 fire
         set skill %actor.skill[sphere of fire]%
      elseif %speech% /= generic   
         set word2 generic
         set skill %actor.skill[sphere of generic]%
      elseif %speech% /= healing
         set word2 healing
         set skill %actor.skill[sphere of healing]%
      elseif %speech% /= protection
         set word2 protection
         set skill %actor.skill[sphere of protection]%
      elseif %speech% /= summoning
         set word2 summoning
         set skill %actor.skill[sphere of summoning]%
      elseif %speech% /= water
         set word2 water
         set skill %actor.skill[sphere of water]%
      else
         say I've never heard of that sphere of magic before. 
      endif
  elseif %speech% /= breathe
      if %speech% /= fire
         set word2 fire
         set skill %actor.skill[breathe fire]%
      elseif %speech% /= frost
         set word2 frost
         set skill %actor.skill[breathe frost]%
      elseif %speech% /= lightning
         set word2 lightning
         set skill %actor.skill[breathe lightning]%
      elseif %speech% /= acid
         set word2 acid
         set skill %actor.skill[breathe acid]%
      elseif %speech% /= gas
         set word2 gas
         set skill %actor.skill[breathe gas]%
      else
         say Whatever that is, you can't breathe it!
      endif
   elseif %speech% == quick chant
      set word2 chant
      set skill %actor.skill[quick chant]%
   elseif %speech% == chant     
      set skill %actor.skill[chant]%
   elseif %speech% == scribe
      set skill %actor.skill[scribe]%
   elseif %speech% == meditate            
      set skill %actor.skill[meditate]%
   elseif %speech% == vampiric touch
      set word2 touch
      set skill %actor.skill[vampiric touch]%
   elseif %speech% == perform            
      set skill %actor.skill[perform]%
   else
      say I'm not familiar with %speech%.
      halt
   endif
   *
   * Check to see if a skill should be taught at all
   *
   eval skill 10 * %skill%
   eval maxskill (%actor.level% * 10) + 60
   *check for capped skills
   set cap 1000
   if %skill% < 50
      mecho %self.name% says, 'I wouldn't know where to start.
      Let's talk about improving a skill you actually know.'
      halt
   elseif %skill% >= %cap% || %skill% >= %maxskill%
      say There is nothing left to teach you. You've mastered %speech%!
      halt
   endif
   if %cap% < %maxskill%
      set maxskill %cap%
   endif
   * This portion is for smooth speech indicating the effects of a players cha or int
score.
   if %cha% > 70
      say You're a charming person.
      if %int% > 70
         mecho   
         mecho %self.name% says, '...and you are very quick witted.
         mecho &0I'll be delighted to give you a good rate.'
      elseif %int% < 50
         say ...but you aren't the smartest.
      endif
   elseif %cha% < 50
      say I don't like you much.
      if %int% > 70
         mecho  
         say ...but you are pretty bright.
      elseif %int% < 50
         mecho  
         mecho %self.name% says, '...and you aren't the smartest.
         mecho &0So this is going to cost you.'
      else
      endif
   else
      say I wouldn't mind working with you.
      if %int% > 70
         mecho  
         say ...and you're pretty bright.
      elseif %int% < 50
         mecho  
         mecho %self.name% says, '...but you aren't the smartest.
         mecho &0So, this is going to cost a bit more than usual.'
      endif
   endif
   mecho  
   *Now a price is calculated
   eval change %maxskill% - %skill%
   eval opinion 200 - (%cha% + %int%)
   eval price %change% * %opinion% * %actor.level% * %actor.level% / (%skill% * 20)
   *
   *now the price in copper has to be divided into coinage.
   eval plat %price% / 1000
   eval gold %price% / 100 - %plat% * 10
   eval silv %price% / 10 - %plat% * 100 - %gold% * 10
   eval copp %price%  - %plat% * 1000 - %gold% * 100 - %silv% * 10
   *now the price can be reported 
   say I'll teach you %speech% for %plat% platinum, %gold% gold, %silv% silver, %copp% copper.
   mecho   
   say Just get me the money, and I'll get started.
   quest variable trainer_3170 %actor.name% skill_name %speech%
   if %word2%
      quest variable trainer_3170 %actor.name% word2 %word2%
   endif
   quest variable trainer_3170 %actor.name% skill_level %skill%
   quest variable trainer_3170 %actor.name% price %price%
   quest variable trainer_3170 %actor.name% actor_level %actor.level%
endif
~
#3171
shaman pay for training~
0 m 1
~
if %actor.quest_variable[trainer_3170:word2]%
   if %actor.quest_variable[trainer_3170:skill_name]% /= sphere
      set full_skill %actor.quest_variable[trainer_3170:skill_name]% of %actor.quest_variable[trainer_3170:word2]%
   else
      set full_skill %actor.quest_variable[trainer_3170:skill_name]% %actor.quest_variable[trainer_3170:word2]%
   endif
elseif %actor.quest_variable[trainer_3170:skill_name]%
   set full_skill %actor.quest_variable[trainer_3170:skill_name]%
endif
wait 1s
if %actor.quest_variable[trainer_3170:actor_level]% == %actor.level%
   if %value% >= %actor.quest_variable[trainer_3170:price]%
      grin %actor.name%
      wait 1s
      msend %actor% %self.name% says, 'Fantastic.  Let's get started...'
      wait 5s
      msend %actor% Some time passes...
      wait 3s
      msend %actor% You feel you are getting the hang of things.
      wait 3s
      msend %actor% You feel your skill in %full_skill% improving dramatically!
      mskillset %actor% %full_skill%
      quest erase trainer_3170 %actor.name%
   else
      msend %actor% %self.name% says, 'I appreciate your voluntary donation, but I'm afraid that's all it was.'
      snicker %actor%
      msend %actor% &0   
      msend %actor% %self.name% says, 'I don't accept installments.  It's got to be all up front.'
   endif
else
   return 0
   wait 2s
   consider %actor%
   wait 2s
   ponder
   wait 2s
   msend %actor% %self.name% says, 'Something is different about you.  What skill were you going to train again?'
   quest erase trainer_3170 %actor.name%
endif
~
#3172
shaman trainer greet~
0 g 100
~
wait 1s
if %actor.quest_stage[trainer_3170]% == 1
  if %actor.quest_variable[trainer_3170:lvl]%
    if %actor.quest_variable[trainer_3170:lvl]% == %actor.level%
      msend %actor% %self.name% says, 'Have you returned to be trained in %actor.quest_variable[trainer_3170:skill_name]%?
      halt
    endif
  endif
  msend %actor% %self.name% says, 'You've been gaining power, have you?  Perhaps you could use some training?'
else
  tell %actor.name% Greetings adventurer.  Improving your skills taking too long?  I can help you for a fee.
endif
~
#3180
load random gems~
0 g 100
100~
if %self.aff_flagged[INVIS]%
   vis
endif
* number of gems to load -- starting at 3, but that might be too many
set loop 3
* gem vnums go from  to 55566-55751
* p1 vnums from 55566-55593
* p2 vnums from 55594-55670
* P3 cnums from 55671-55747 (there are gems up to 55751, but not used.
* random # -- 1-10 to create probabilities of good gem
* 0 = NO GEM
* 1 = NO GEM
* 2-6 = P1 Gem
* 7-9 = P2 Gem
* 10  = P3 Gem
* -- lets see if we should run process to get gems 
* -- we do that by looking for object 18701 -- if we are wearing it
* -- then we don't need to load gems again
* all the important stuff encased in this loop
if !%self.wearing[18701]%
     mload obj 18701
     mat 1100 wear lock
     set itt 1
     while %itt% <= %loop%
     set p %random.10%
         if %p% == 10
          *say p3! %p%
             set base 55671
             set extra %random.76%
         endif
         *p2 gem
         if (%p% <=9) && (%p%>=7)
          *say p2 %p%555
             set base 55594
             set extra %random.76%
         endif
         *p1 gem
         if (%p% <=6) && (%p%>=2)
         *say p1 %p%
             set base 55566
             set extra %random.27%
         endif
     *no gem
         if (%p% < 2)
         *say no gem - %p%
             set base 0
          set extra 0
         endif
         if %base% > 55560
             eval gem %base% + %extra%
     mload obj %gem%
 sell gem elspeth
         endif
         eval itt %itt% + 1
     done
chuckle
say you wouldn't believe who I had to beat down to get these
flex
endif
*
* Adding hint to find Rogue Guild for Elspeth
*
if %self.vnum% == 3010
   if %actor.class% == Rogue && %actor.level% < 10
      wait 4
      msend %actor% %self.name% notices you skulking about her shop.
      msend %actor% &7&b%self.name% tells you, 'If you're looking for a certain Guild Hall, &6search&7 behind&0
      msend %actor% &0&7&bthe &6curtain&7 on the wall.'&0
   endif
endif
~
#3181
corpse retrieval greet~
0 g 100
~
if %actor.vnum% == -1
  wait 4
  mecho %self.name% says, 'Welcome to the headquarters of the Bloody Red Cross.  We provide
  mecho &0corpse retrieval services for unlucky adventurers.'
  wait 1s
  say Are you in need of a corpse retrieval?
endif
~
#3182
corpse retrieval price set~
0 d 100
corpse retrieval services yes help~
if %actor.vnum% == -1
  wait 2
  set cha %actor.real_cha%
  *make sure they have a quest record for saving variables
  if %actor.quest_stage[corpse_retrieval]% < 1
    quest start corpse_retrieval %actor.name%
  endif
  say We can certainly help you with that...
  wait 1s
  say For a price.
  wait 1s
  mecho %self.name% says, 'But understand, if you don't have a corpse out there, this spell
  mecho &0will fail and you won't get your money back.'
  wait 2s
  eye %actor%
  * This portion is for smooth speech indicating the effects of a player's cha and level.
  if %actor.level% < 10
    say You're still a fresh-faced adventurer...
    mecho 
    if %cha% > 80
      mecho %self.name% says, '... and you're a sweet kid.
      mecho &0I'll give you a good rate.'
    elseif %cha% < 50
      say ... but your face is just so punchable.
      mecho &0So, this is really gonna cost ya.'
    else
      say ... and we all need a little help from time to time.
    endif
  elseif %actor.level% < 30
    say Be careful, things are tougher out there for you now...
    mecho  
    if %cha% > 80
      mecho %self.name% says, '... but you are pretty charming.
      mecho &0I'll give you a good rate.'
    elseif %cha% < 50
      mecho %self.name% says, '... and you probably had it coming.
      mecho &0So, this is really gonna cost ya.'
    else
      say ... but we all get into tight scrapes now and then.
    endif
  elseif %actor.level% < 50
    say You've seen your fair share of action...
    if %cha% > 80
      mecho %self.name% says, '... but you're still pretty likable.
      mecho &0I'll give you a good rate.'
    elseif %cha% < 50
      mecho %self.name% says, '... and you probably asked for most of it.
      mecho &0So, this is really gonna cost ya.'
    else
      say ... so take more precautions.
    endif
  elseif %actor.level% < 70
    say You really should know how to survive better by now...
    mecho 
    if %cha% > 80
      mecho %self.name% says, '... but you're pretty likable.
      mecho &0I'll give you a good rate.'
    elseif %cha% < 50
      mecho %self.name% says, '... and it must be hard when people dislike you so much.
      mecho &0So, this is really gonna cost ya.'
    else
      say ... so try harder in the future.
    endif
  elseif %actor.level% < 90
    say Seasoned heroes like you are hard to help...
    mecho 
    if %cha% > 80
      mecho %self.name% says, '... but you're very friendly.
      mecho &0I'll give you a good rate.'
    elseif %cha% < 50
      mecho %self.name% says, '... and plenty of people hate you.
      mecho &0So, this is really gonna cost ya.'
    else
      say ... so don't rely on me too much.
    endif
  else
    say Helping a legend like you is extremely difficult...
    mecho 
    if %cha% > 80
      mecho %self.name% says, '... but you have a way with people.'
      mecho &0I'll give you a good rate.'
    elseif %cha% < 50
      mecho %self.name% says, '... and every creature in the world wants you dead.
      mecho &0So, this is really gonna cost ya.'
    else
      say ... so you won't get much sympathy from me.
    endif
  endif
  mecho  
  *Now a price is calculated
  eval price (%actor.level% * %actor.level% * ((%actor.level% * 2) / 3) * (100 - (%cha% / 10))) / 100
  *now the price in copper has to be divided into coinage.
  eval plat %price% / 1000
  eval gold %price% / 100 - %plat% * 10
  eval silv %price% / 10 - %plat% * 100 - %gold% * 10
  eval copp %price%  - %plat% * 1000 - %gold% * 100 - %silv% * 10
  *now the price can be reported 
  say My fee is %speech% for %plat% platinum, %gold% gold, %silv% silver, %copp% copper.
  mecho   
  say Just get me the money and I'll cast the appropriate spells.
  quest variable corpse_retrieval %actor.name% price %price%
  quest variable corpse_retrieval %actor.name% actor_level %actor.level%
endif
~
#3183
corpse retrieval payment~
0 m 1
~
if %actor.quest_variable[corpse_retrieval:actor_level]% == %actor.level%
  if %value% >= %actor.quest_variable[corpse_retrieval:price]%
    mforce %actor% consent %self%
    wait 1s
    nod %actor.name%
    wait 1s
    say That looks right.  Let's get started...
    wait 1s
    cast 'shift corpse' %actor.name%
    mecho %self.name% looks exhausted after casting.
    quest erase corpse_retrieval %actor.name%
    mforce %actor% consent off
  else
    mecho %self.name% says, 'Thank you for your donation to the Bloody Red Cross, but I'm
    mecho &0afraid that's all it was.'
    snicker %actor%
    mecho   
    say I don't accept installments.  It's got to be all up front.
  endif
else
  return 0
  wait 1s
  consider %actor%
  wait 2s
  ponder
  wait 2s
  mecho %self.name% says, 'Something is different about you.  We have to adjust the spell
  mecho &0for your new strength.  Do you still want a corpse retrieval?'
  quest erase corpse_retrieval %actor.name%
endif
~
#3184
corpse retrieval load~
0 o 100
~
mskillset %self% shift corpse
~
#3296
Make_Fountain_heal~
1 c 100
drink~
if (%arg% == pool || %arg% == granite)
oheal %actor% 400
return 0
endif
~
#3298
Black_Rose_Wear_Tear~
1 j 100
~
**Trigger is meant as a tear drop in memory of Pergus**
osend %actor% A small tear drops from the corner of your eye as you garnish the rose.
oechoaround %actor% %actor.name% sheds a tear as %actor.n% adjusts the rose.
~
#3299
Black_Rose_Spawn_Pluck~
2 c 100
Pluck~
wsend %actor% You bend over and carefully pluck the &9&bblack&0 rose from between the two hearts.
**some command to directly load 3298 to player inventory**
wechoaround %actor% %actor.name% bends over and carefully plucks the &9&bblack&0 rose from between the two hearts.
wait 1
wecho Another identical &9&bblack&0 rose magically grows up in the same spot.
~
#3301
newbie trainer Rousel~
0 g 100
~
wait 5
sa If you would like to attend Newbie training, just say newbie.
sa but this isn't quite ready yet, so come back later.
~
$~
