#8500
ziijhan_welcome~
0 g 100
~
wait 2
switch %actor.quest_stage[nec_dia_ant_subclass]%
  case 1
    msend %actor% %self.name% says, 'So you come crawling back.'
    wait 2
    msend %actor% %self.name% says, 'Only the most cunning and strong will complete the &5quest&0 I set before you.  I shall take great pleasure in your demise, but I will offer great rewards for your success.'
    break
  case 2
    msend %actor% %self.name% says, 'It is time to hunt!'
    wait 2
    msend %actor% %self.name% says, 'My pitiful waste of a &5brother&0 escaped my minions.'
    break
  case 3
  case 4
    msend %actor% %self.name% says, 'Have you brought proof that my brother is not beyond my reach?'
    break
  default
    if %actor.class% /= Warrior
      switch %actor.race%
        case elf
        case faerie_seelie
          set classgreet no
          break
        default
          set classgreet yes
          set maxlevel 25
      done
    elseif %actor.class% /= Cleric
      switch %actor.race%
        case elf
        case faerie_seelie
          set classgreet no
          break
        default
          set classgreet yes
          set maxlevel 35
      done
    elseif %actor.class% /= Sorcerer
      switch %actor.race%
        case elf
        case faerie_seelie
          set classgreet no
          break
        default
          set classgreet yes
          set maxlevel 45
      done
    endif
    if %classgreet% == yes
      if %actor.level% >= 10 && %actor.level% <= %maxlevel%
        peer %actor.name%
        emote places a most vile half-smile on his face.
        msend %actor% %self.name% says, 'Some know not of their &5destinies&0, while others simply choose to ignore them.'
        wait 2s
        msend %actor% %self.name% says, 'Which of the two are you?'
        glare %actor.name%
      endif
    endif
done
if %actor.quest_stage[phase_mace]% == %macestep%
  eval minlevel %macestep% * 10
  if %actor.level% >= %minlevel%
    if %actor.quest_variable[phase_mace:greet]% == 0
      msend %actor% %self.name% says, 'I sense a ghostly presence about your weapons.  If you want my help, we can talk about &6&b[upgrades]&0.'
    else
      msend %actor% %self.name% says, 'Do you have what I need?'
    endif
  endif
endif
~
#8501
quest_preamble_ziijhan~
0 d 100
destiny destiny? destinies destinies? I~
if %speech% /= destiny || %speech% /= destinies || %speech% /= I know
  if !%actor.quest_stage[nec_dia_ant_subclass]%
    if %actor.class% /= Warrior
      switch %actor.race%
        case elf
        case faerie_seelie
          if %actor.level% >= 10 && %actor.level% <= 25
            msend %actor% &1Your race may not subclass to anti-paladin.&0
            halt
          endif
          break
        default
          set classquest 1
      done
  elseif %actor.class% /= Cleric
    switch %actor.race%
      case elf
      case faerie_unseelie
        if %actor.level% >= 10 && %actor.level% <= 35
          msend %actor% &1Your race may not subclass to diabolist.&0
          halt
        endif
        break
      default
        set classquest 1
    done
  elseif %actor.class% /= Sorcerer
    switch %actor.race%
      case elf
      case faerie_seelie
        if %actor.level% >= 10 && %actor.level% <= 45
          msend %actor% &1Your race may not subclass to necromancer.&0
          halt
        endif
        break
      default
        set classquest 1
    done
  else
    if %actor.level% >= 10 && %actor.level% <= 25
      msend %actor% %self.name% says, 'Sorry, I cannot help you achieve your destiny.'
    endif
  endif
  if %classquest% == 1
    wait 2
    if %use_subclass%
      msend %actor% %self.name% says, 'Sorry little one.  I am already setting up one quest - come back in a moment.'
      halt
    endif
    if %actor.align% < -349
      nod %actor.name%
      if %actor.class% /= Warrior
        if %actor.level% >= 10 && %actor.level% <= 25
          msend %actor% %self.name% says, 'Do ye wish to join the ranks of the unholy warriors?'
          set use_subclass Ant
        elseif %actor.level% < 10
          msend %actor% %self.name% says, 'Strong thirst for slaughter for one so young.  Come back once you've gained some more experience.'
        else
          msend %actor% %self.name% says, 'You are already following your destiny.  I cannot help you any further.'
        endif
      elseif %actor.class% /= Cleric
        if %actor.level% >= 10 && %actor.level% <= 35
          msend %actor% %self.name% says, 'Do ye wish to join the ranks of the diabolists and derive power from madness and darkness?'
          set use_subclass Dia
        elseif 
          msend %actor% %self.name% says, 'Strong thirst for madness for one so young.  Come back once you've gained some more experience.'
        else
          msend %actor% %self.name% says, 'You are already following your destiny.  I cannot help you any further.'
        endif
      elseif %actor.class% /= Sorcerer
        if %actor.level% >= 10 && %actor.level% <= 45
          msend %actor% %self.name% says, 'Do ye wish to join the ranks of the necromancers and have power over death?'
          set use_subclass Nec
        elseif %actor.level% < 10
          msend %actor% %self.name% says, 'Strong thirst for death for one so young.  Come back once you've gained some more experience.'
        else
          msend %actor% %self.name% says, 'You are already following your destiny.  I cannot help you any further.'
        endif
      endif
      global use_subclass
    else
      msend %actor% %self.name% says, 'Begone, fool.  You are far too righteous to join our brotherhood.'
    endif
  endif
endif
~
#8502
start_quest_ziijhan~
0 d 100
yes no~
if %actor.class% /= Cleric && %actor.level% >= 10 && %actor.level% <= 35
  switch %actor.race%
    case elf
    case faerie_seelie
      msend %actor% &1Your race may not subclass to diabolist.&0
      halt
      break
    default
      set classquest 1
  done
elseif %actor.class% /= Warrior && %actor.level% >= 10 && %actor.level% <= 25
  switch %actor.race%
    case elf
    case faerie_seelie
      msend %actor% &1Your race may not subclass to anti-paladin.&0
      halt
      break
    default
      set classquest 1
  done
elseif %actor.class% /= Sorcerer && %actor.level% >= 10 && %actor.level% <= 45
  switch %actor.race%
    case elf
    case faerie_seelie
      msend %actor% &1Your race may not subclass to necromancer.&0
      halt
      break
    default
      set classquest 1
  done
endif
if %classquest% == 1
  if %speech% /= yes
    if %actor.quest_stage[nec_dia_ant_subclass]% == 0
      quest start nec_dia_ant_subclass %actor.name% %use_subclass%
    endif
    unset use_subclass
    wait 2
    nod
    msend %actor% %self.name% says, 'Only the most cunning and strong will complete the &5quest&0 I set before you.  I shall take great pleasure in your demise, but I will offer great rewards for your success.'
    wait 2s
    msend %actor% %self.name% says, 'I can update you on your &5[subclass progress]&0 as well.'
  elseif %speech% /= no
    unset use_subclass
    msend %actor% %self.name% says, 'Begone from my sight before I lose patience with you!'
    whap %actor.name%
    msend %actor% Ziijhan waves his arms in a circle and you are blinded by a flash of light!
    mechoaround %actor% Ziijhan glares at %actor.name% and sends %actor.o% elsewhere!
    mteleport %actor% 8501
  endif
endif
~
#8503
quest_details_ziijhan~
0 d 100
quest~
if %actor.vnum% == -1 
  if %actor.quest_stage[nec_dia_ant_subclass]% == 1
    quest advance nec_dia_ant_subclass %actor.name%
  endif
  wait 2
  msend %actor% %self.name% says, 'Many years ago, my pact with the demon realm allowed me to be master of this domain.  All were subjugated, man, woman, and beast.'
  smirk
  wait 1s
  msend %actor% %self.name% says, 'One man would not bow though!'
  emote growls with an anger and fury with which many have never seen and lived!
  wait 1s
  msend %actor% %self.name% says, 'My pitiful waste of a &5brother&0 escaped my minions.'
  smi %actor.name%
  wait 1s
  msend %actor% %self.name% says, 'Perhaps you will remedy that.'
endif
~
#8504
more_quest_details_ziijhan~
0 d 100
brother brother? remedy remedy? how how?~
if %actor.vnum% == -1  && %actor.quest_stage[nec_dia_ant_subclass]% > 1
  if %actor.quest_stage[nec_dia_ant_subclass]% == 2
    quest advance nec_dia_ant_subclass %actor.name%
  endif
  nod %actor.name%
  msend %actor% %self.name% says, 'Yes!  My wretched sibling, Ber...  I shall not utter his name!'
  wait 2
  fume
  wait 3
  msend %actor% %self.name% says, 'I despise him and his reverent little life.'
  wait 2
  ponder
  msend %actor% %self.name% says, 'He thinks he is safe now, beyond my grasp.  That FOOL!'
  chuckle
  wait 3
  msend %actor% %self.name% says, 'Enough!  I grow tired of you!'
  wait 2
  msend %actor% %self.name% says, 'Bring back proof of the deed and you shall be accepted.'
endif
~
#8505
exit_from_ziijhan~
0 d 100
exit~
sigh
say Sniveling mortal!
msend %actor% Ziijhan waves his arms in a circle and you are blinded by a flash of light!
mechoaround %actor% Ziijhan glares at %actor.name% and sends %actor.o% elsewhere!
mteleport %actor% 8501
~
#8506
give_head_ziijhan~
0 j 100
8504~
wait 2
switch %actor.quest_stage[nec_dia_ant_subclass]%
  case 1
  case 2
  * not in final stage of quest
    msend %actor% %self.name% says, 'Hmmm, jumping the gun a bit aren't we?  I didn't even tell you the quest yet.'
    wait 2
    msend %actor% %self.name% says, 'Thanks for the gift, weirdo.'
    break  
  case 3
  * somehow they have the head without GETTING it
    msend %actor% %self.name% says, 'Hmmm, how did you get this head without GETTING it?'
    whap %actor.name%
    wait 2
    msend %actor% %self.name% says, 'I can't abide that sort of foolishness!'
    break
  case 4
    emote throws back his head and howls devilishly with pleasure.
    wait 3
    shake %actor.name%
    msend %actor% %self.name% says, 'You have done me a great service, and earned your place among the truly dark.
    smi
    wait 2
    msend %actor% %self.name% says, 'Type &1'subclass'&0 to proceed.'
    quest complete nec_dia_ant_subclass %actor.name%
    break
  default
    msend %actor% %self.name% says, 'Well, what a lovely gift, I must stuff it and mount it on my wall.'
    thank %actor.name%
    wait 2
    msend %actor% %self.name% says, 'Shame you weren't performing the quest, you seem to be a nasty piece of work.'
done
* junk the head
mjunk head
~
#8507
head_retrieval~
1 g 100
~
if %actor.quest_stage[nec_dia_ant_subclass]% == 3
  quest advance nec_dia_ant_subclass %actor.name%
endif
~
#8508
Ziijhan forgets someone~
0 b 100
~
if %start_q% == 1
   set counter 0
   set questerhere 0
   set room %get.room[%self.room%]%
   set person %room.people%
   while %person%
      if %person.name% /= %q_plyr%
         set questerhere 1
      endif
      set person %person.next_in_room%
   done
   if %questerhere% == 0
      say Hmm, where did %q_plyr% go?
      set start_q 0
      global start_q
   endif
endif
~
#8509
torturer_death~
0 f 100
~
set person %actor%
set i %actor.group_size%
if %i%
   set a 1
   unset person
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[resurrection_quest]% == 2
            quest advance resurrection_quest %person.name%
            set run yes
         elseif %person.quest_variable[resurrection_quest:new]% == yes
            quest variable resurrection_quest %person.name% new new
            set run yes
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[resurrection_quest]% == 2
   quest advance resurrection_quest %person.name%
   set run yes
elseif %person.quest_variable[resurrection_quest:new]% == yes
   quest variable resurrection_quest %person.name% new new
   set run yes
endif
if %run% /= yes
   m_run_room_trig 8553
endif
~
#8510
torturer_greet1~
0 g 100
~
if (%actor.quest_stage[resurrection_quest]% == 2 || %actor.quest_variable[resurrection_quest:new]% == yes) && bishop can_be_seen
   set quester %actor.name%
   say You've made a fatal mistake, bishop!
   wait 1
   say Your life is now forfeit!
   mforce bishop say &bPlease, %quester%, help me!&0
   backstab bishop
else
   backstab actor
endif
~
#8511
bishop_speak1~
0 d 100
defiler?~
nod
say Yes, the Defiler, Ziijhan, the Defiler.
wince
~
#8512
bishop_speak2~
0 d 100
responsible this? you?~
wince
say The defiler.
~
#8513
bishop_speak3~
0 d 100
escape!~
say Get out of here, don't worry about me!
wait 1
say Save yourself!!
wince
~
#8514
seer_rand1~
0 b 40
~
emote starts chanting a strange mantra.
~
#8515
seer_rand2~
0 b 50
~
emote grumbles about the lack of observation skills and such.
~
#8516
seer_speak1~
0 d 100
hourglass hourglass?~
grin
say Very well done indeed!
emote lifts his head and utters some obscure magical phrase.
mteleport %actor.name% 8583
~
#8517
seer_speak2~
0 d 100
riddle?~
say Yes.  Very well.
grin
wait 1
say The riddle:
mecho Two bodies have I,
mecho Though both joined in one.
mecho The more still I stand,
mecho The quicker I run.
wait 1
say What am I?
~
#8518
seer_speak3~
0 d 100
worthy?~
say Only the sharpest of minds can answer the riddle my master dictated to me.
sigh
wait 1
say You, I fear, are not of sharp mind.
emote casts his hollow eye sockets upon you.
say Or body.
smile seer
~
#8519
seer_speak4~
0 d 100
master?~
emote shakes his head in disgust.
say Yes, my master.
wait 1
say Only those that are worthy of mind and body may enter.
~
#8520
seer_speak5~
0 d 100
seek?~
say yes, to gain access to my master, you must call out his name.
~
#8521
newbie-safety-guard-north~
0 c 100
north~
if %actor.vnum% == -1
   if %actor.level% < 30
      return 1
      whisper %actor.name% Beyond this point is out of your grasp.
      whisper %actor.name% I suggest other places.
      wait 1s
      bow %actor.name%
   else
      return 0
   endif
else
   return 0
endif
~
#8522
newbie-safety-guard-west~
0 c 100
west~
if %actor.vnum% == -1
   if %actor.level% < 30
      return 1
      whisper %actor.name% Beyond this point is out of your grasp.
      whisper %actor.name% I suggest other places.
      wait 1s
      bow %actor.name%
   else
      return 0
   endif
else
   return 0
endif
~
#8523
newbie-safety-guard-south~
0 c 100
south~
if %actor.vnum% == -1
   if %actor.level% < 30
      return 1
      whisper %actor.name% Beyond this point is out of your grasp.
      whisper %actor.name% I suggest other places.
      wait 1s
      bow %actor.name%
   else
      return 0
   endif
else
   return 0
endif
~
#8524
seer_rand3~
0 b 40
~
smirk %actor.name%
say Perhaps you are not worthy.
~
#8525
Anti-Necro-Dia subclass status checker~
0 d 0
subclass progress~
switch %actor.quest_stage[nec_anti_dia_subclass]%
  case 1
    msend %actor% %self.name% says, 'So you come crawling back.'
    wait 2
    msend %actor% %self.name% says, 'Only the most cunning and strong will complete the &5quest&0 I set before you.  I shall take great pleasure in your demise, but I will offer great rewards for your success.'
    break
  case 2
    msend %actor% %self.name% says, 'Many years ago, my pact with the demon realm allowed me to be master of this domain.  All were subjugated, man, woman, and beast.'
    smirk
    wait 1s
    msend %actor% %self.name% says, 'One man would not bow though!'
    emote growls with an anger and fury with which many have never seen and lived!
    wait 1s
    msend %actor% %self.name% says, 'My pitiful waste of a &5brother&0 escaped my minions.'
    smi %actor.name%
    wait 1s
    msend %actor% %self.name% says, 'Perhaps you will remedy that.'
    break
  case 3
  case 4
    msend %actor% %self.name% says, 'My wretched sibling, Ber...  I shall not utter his name!'
    wait 2
    fume
    wait 3
    msend %actor% %self.name% says, 'I despise him and his reverent little life.'
    wait 2
    ponder
    msend %actor% %self.name% says, 'He thinks he is safe now, beyond my grasp.  That FOOL!'
    wait 2
    msend %actor% %self.name% says, 'Bring back proof of the deed and you shall be accepted.'
    break
  default
    if %actor.align% < -349
      if %actor.class% /= Warrior && %actor.level% <= 25
        switch %actor.race%
          case elf
          case faerie_seelie
            set classgreet no
            break
          default
            set classgreet yes
            set maxlevel 25
        done
      elseif %actor.class% /= Cleric && %actor.level% <= 35
        switch %actor.race%
          case elf
          case faerie_seelie
            set classgreet no
            break
          default
            set classgreet yes
            set maxlevel 35
        done
      elseif %actor.class% /= Sorcerer && %actor.level% <= 45
        switch %actor.race%
          case elf
          case faerie_seelie
            set classgreet no
            break
          default
            set classgreet yes
        done
      endif
    endif
    if %classgreet% == yes
      if %actor.level% >= 10
        peer %actor.name%
        emote places a most vile half-smile on his face.
        msend %actor% %self.name% says, 'Shame you aren't performing this quest, you seem to be a nasty piece of work.'
      else
        msend %actor% %self.name% says, 'Come back when you have a few more kills, you seem to be a nasty piece of work.'
      endif
    else
      msend %actor% %self.name% says, 'You don't work for me and you probably never will.'
    endif
done
~
#8526
Ziijhan general receive~
0 j 0
8504~
switch %object.vnum%
  case %maceitem2%
  case %maceitem3%
  case %maceitem4%
  case %maceitem5%
  case %macevnum%
    halt
    break
  default
    set response What is this garbage?
done
if %response%
  return 0
  msend %actor%%self.name% scoffs at %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, '%response%'
endif
~
#8550
Resurrect_greet~
0 g 100
~
wait 1
if %actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist
   if %actor.level% < 81
      wait 1s
      say You've done well for yourself, little one.  Come back when you are stronger and perhaps I'll have something for you.
      pat %actor.name%
   elseif %actor.level% < 100
      if %actor.quest_variable[resurrection_quest:new]% /= new
         say I've heard what happened.  Ziijhan will be furious!
         cackle
         wait 2s
         roll
         say Fine, I suppose I can help you.
         wait 1s
         emote pulls another talisman from his robes.
         wait 1s
         mload obj 8550
         give talisman %actor%
      elseif %actor.quest_stage[resurrection_quest]% < 1
         smile
         say What brings a young cleric to my home?  Has your order sent you to steal my books again?  Or could it be that your order has forgotten how the resurrection incantations are to be performed?
      elseif %actor.quest_stage[resurrection_quest]% == 3
         wait 1 s
         grin %actor%
         wait 3s
         say I've heard what happened.  Ziijhan will be furious!
         cackle
         wait 4s
         ponder
         say Perhaps you could be useful to me after all. 
         halt
      endif
   else
      Say Come to steal my books again?!  You will not learn the art of resurrection by arcane magic.  You will only become an abomination like myself.  Go seek out the divine arts.
      wait 2s
      say NOW LEAVE OR ATTACK ME, IF YOU DARE!
   endif
endif
~
#8551
norisent speech~
0 dn 1
resurrection teach resurrect yes  payment how how? please useful command souls~
set speech %speech%
if %actor.vnum% /= -1
   if %actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist
      if %actor.quest_stage[resurrection_quest]% <= 1 || %actor.level% >= 81
         if %speech% /= resurrection || %speech% /= resurrection?
            quest start resurrection_quest %actor.name%
            wait 1s
            emote smirks ever so slightly, with a creepy glint of foreboding. 
            say So it's happened.  I knew someday your order would forget how.  Though, it's happened much sooner than I would have thought.  It only took a thousand years.
            emote furrows his brow.
            wait 1s
            say Is this true?  Have you come hoping that I will teach you how to resurrect your allies?
            halt
         endif
      endif
      if %actor.quest_stage[resurrection_quest]% /= 1
         if %speech% /= teach ||  %speech% /= resurrect || %speech% /= yes
            wait 1s
            say And what could you possibly offer me as payment?  I require nothing of food or drink.  Riches and knowledge are mine.  Glory and honor are lost on me.  I owe no living man my loyalty.
            wait 3s
            say 'I know of only one thing that would please me more than your death that I might consider payment.
            halt
         elseif %speech% /= payment || %speech% /= please
            wait 1s
            say There is an emissary from the Abbey of St. George, a bishop, being held in the dungeon.  Ziijhan would have her tortured and killed.  He's always trying to settle an old score with his brother, Barak.
            wait 3s
            say The old knight is being a petty child, but what could you possibly do to upset him?  Be gone, and trouble me no more.
            quest advance resurrection_quest %actor%
            halt
         endif
      elseif %actor.quest_stage[resurrection_quest]% /= 3
         if %speech% /= useful || %speech% /= how || %speech% /= how?
            quest advance resurrection_quest %actor.name%
            wait 1s
            say Yes, there is something more you could do for me.
            wait 1s
            say A beastly dark mage left an experiment... unresolved.  Very unprofessional of him.
            wait 3s
            say In his attempts to reunite a body and soul, he left them each alive, severed from one another.
            wait 3s
            mecho %self.name% says, 'You must go and find Lajon's two severed halves, loose them of their undead states, and then destroy them both.  The soul must be rebuked with the words of power &4&bDhewsost Konre&0, before purging him.'
            wait 5s
            say To purge the rogue body, however, you'll need to give him this to help you finalize his death.
            wait 4s
            mload obj 8550
            give death %actor%
            wait 3s
            say And as for the old mage...
            chuckle
            wait 2s
            say That talisman should aid his passing as well.  The ring of souls is the source of his power.  Bring it to me when you're finished.
            wait 2s
            mecho %self.name% says, 'And if you forget, you can ask about your &7&b[progress]&0.  Maybe I'll consider reminding you.'
         endif
      endif
   endif
endif
~
#8552
Torturer_death_resurrect_quest~
0 g 100
~
if %actor.vnum% == -1
   if %actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist
      if %actor.quest_stage[resurrection_quest]% == 2
         mload obj 8504
      endif
   endif
endif
~
#8553
bishop room trig~
2 a 100
~
wait 1s
wforce bishop abort
wecho %get.mob_shortdesc[8514]% says, 'You've done it!  Norisent must have sent you.  He has great admiration for those of us who dedicate ourselves to restoration magic.  He's sure to be pleased to hear of your involvement.'
wait 3s
wforce bishop say I must go now, while I have a chance.  Thank you so much!
wait 3s
wforce bishop abort
wecho The bishop starts casting &3&b'word of recall'&0...
wecho The bishop completes her spell.
wecho The bishop closes her eyes and utters the words, 'word of recall'.
wecho the bishop vanishes in a flash of light.
wpurge bishop
~
#8554
Resurrection Death Talisman Give~
1 i 100
~
* As mobs are handed this object, they are recorded on the object itself as a global,
* Then when it's turned in to Norisent, he checks the object for the appropriate global variables.
*
* If the item has been assigned to a quester, it does its tricks, otherwise it skips to the bottom.
switch %victim.vnum%
   case 8550
      if %actor.quest_stage[resurrection_quest]% > 10
         oecho %victim.name% mutters an incantation and %self.shortdesc% bursts into flame and is gone.
         opurge %self%
         halt
      endif
      switch %actor.quest_stage[resurrection_quest]%
         case 4
            if !%actor.quest_variable[resurrection_quest:4003]% 
               oforce %victim% say You have not banished Lajon's Soul.
            elseif %actor.quest_variable[resurrection_quest:4004]% == 0
               oforce %victim% say You have not destroyed Lajon's Corruption.
            elseif %actor.quest_variable[resurrection_quest:4016]% == 0
               oecho %victim.name% says, 'You must still stop the one responsible, the Tres Keeper, and return his ring of souls.'
            else
               quest advance resurrection_quest %actor%
               oforce %victim% say Well done.  Now the ring?
            endif
            break
         case 5
            oforce %victim% say I need to have that ring.
            break
         case 6
            if %actor.quest_variable[resurrection_quest:53411]% < 2
               oforce %victim% say You have not yet removed enough of the Xeg-Yi from this realm.
            elseif %actor.quest_variable[resurrection_quest:53308]% == 0 
               oecho %victim.name% says, 'Aelfric is still out there, with the white robes.  Go finish the job!'
            else
               quest advance resurrection_quest %actor%
               oforce %victim% say Well done.  Now the robes?
            endif
            break
         case 7
            oforce %victim% say Before we may continue, I must have the dragon robes.
            break
         case 8
            if %actor.quest_variable[resurrection_quest:53001]% == 0
               oforce %victim% say The spectral man is still out there.  Why are you here?
            elseif %actor.quest_variable[resurrection_quest:51005]% == 0
               oforce %victim% say The poor man's bloody remains must be dispelled.  Go now!
            elseif %actor.quest_variable[resurrection_quest:51014]% == 0
               oecho %victim.name% says, 'Go stop Luchiaans, and bring the source of power before he finds it.'
            else
               quest advance resurrection_quest %actor%
               oecho %victim.name% says, 'Excellent.  And the object of power, do you have it?  Give it to me.'
            endif
            break
         case 9
            oforce %victim% say Luchiaans' object of power, do you have it?  Give it to me.
            break
         case 10
            if %actor.quest_variable[resurrection_quest:52015]% == 0
               oforce %victim% say The weaponsmith's spirit still dwells.  Go set her free.
            elseif %actor.quest_variable[resurrection_quest:52003]% == 0
               oecho %victim.name% says, 'The crazed mage must be destroyed.  Go, end this.  Return with his ring.'
            else
               quest advance resurrection_quest %actor%
               oecho %victim.name% mutters an incantation and %self.shortdesc% vanishes into flames, the smoke drifting into %victim.name%'s nostrils.
               wait 1s
               oecho %victim.name% says, 'We are nearly finished.  Do you have the mage's artifact of longevity?'
               opurge self
               halt
            endif
            break
         default
            oforce %victim% emote briskly refuses.
            oforce %victim% say I don't want this from you.  Do not lose that talisman.
      done
      return 0
      oforce %victim% emote returns the death talisman to you.
      halt
   case 4003
   case 4004
   case 4016
   case 53308
   case 53001
   case 51005
   case 51014
   case 52015
   case 52003
      quest variable resurrection_quest %actor% %victim.vnum% 1
      break
   case 53411
      if %actor.quest_variable[resurrection_quest:53411]%
         eval count %actor.quest_variable[resurrection_quest:53411]% + 1
         quest variable resurrection_quest %actor% %victim.vnum% %count%
      else
         quest variable resurrection_quest %actor% %victim.vnum% 1
      endif
      break
   default
      halt
done
wait 1s
oecho %victim.name% feels compelled by %self.shortdesc%.
oforce %victim% hold %self%
oforce %victim% wear %self% neck
~
#8555
Norisent Receive~
0 j 100
~
if %object.vnum% == 4008
   if %actor.quest_stage[resurrection_quest]% == 4
      return 0
      emote refuses to take it.
      say First, give me the talisman, to be sure this was done right.
      halt
   elseif %actor.quest_stage[resurrection_quest]% == 5
      wait 1s
      mpurge %object%
      quest advance resurrection_quest %actor.name%
      say Excellent.  The Tres Keeper was using this ring to gain power over the connection between soul and body.  It should not be in such careless hands as his.
      wait 3s
      say Another of the dark mages focused his research on extending life.  He is using robes imbued with the power of a mighty dragon to manipulate the flow of time and extend his life.'
      sigh
      wait 3s
      say However, his tempering with the mysterious time elementals left behind from the Time Cataclysm opened a rift that he could not control.  Monstrous tentacled abominations wriggled through the rift and are now wreaking havoc on the material plane.
      wait 5s
      mecho %self.name% says, 'With that talisman, you must remove the &4&btwo Xeg-Yi&0 from this world.  They do not belong.'
      wait 3s
      mecho %self.name% says, 'When you're finished, remove the &4&bdragon-cult mage&0 by the same means.  His meddling is over.'
      halt
   endif
   halt
elseif %object.vnum% == 53307
   if %actor.quest_stage[resurrection_quest]% == 6
      return 0
      emote refuses your item.
      say First, give me the talisman.  I want to be sure things are in order.
      halt
   elseif %actor.quest_stage[resurrection_quest]% == 7
      wait 1s
      mpurge %object%
      quest advance resurrection_quest %actor.name%
      say Good.  Aelfric's work with lengthening life was disrupting the flow of time itself.  Very foolish of him.
      wait 3s
      say There is one whose necromancy has become quite advanced, as brutish as it is.
      chuckle
      wait 3s
      say His works have sunken a castle into the swamps, leaving it to the local lizard men.  But more critically, he is continuing his vulgar practice in a nearby village, severing spirits and bodies, magic and reality, and even losing demons into the world.  The situation is utterly out of his control.  He seeks a relic that he believes will aid him in his research.
      wait 5s
      if %actor.class% == diabolist
         say Help him find the heart of the phoenix that he seeks, but bring it here.
      else
         say Find the book he seeks, before he discovers its resting place.  You must bring it to me.
      endif
      wait 3s
      mecho %self.name% says, 'There is a man that resists the oppressor, with both his &4&bsoul in the castle&0, and his &4&banimated bloody remains&0 in the village where the &4&bdark mage&0 has made his dwelling.'
      wait 3s
      say Remove them all!  Dark mage and the bloody abomination by means of the talisman, and the remaining spirit by means of the divine utterance, "Dhewsost Konre."
      halt
   endif
   halt
elseif %actor.quest_stage[resurrection_quest]% == 9 && (%object.vnum% == 51023 || %object.vnum% == 51028 || %object.vnum% == 51022)
   if %object.vnum% == 51023
      switch %actor.class% 
         case CLERIC
         case PRIEST
            wait 2
            mjunk %object%
            say Ah, the angelic book!  You have done well to keep this from him.
            quest advance resurrection_quest %actor.name%
            break
         case DIABOLIST
            return 0
            say Wrong one, dummy.
         default
            return 0
            say What's your class?
            halt
      done
   elseif %object.vnum% == 51028
      switch %actor.class%
         case diabolist
            wait 2
            mjunk %object%
            say Ah, the phoenix heart!  You've done well to keep this from him.
            quest advance resurrection_quest %actor.name%
            break
         case cleric
         case priest
            return 0
            say Wrong one, dummy.
         default
            return 0
            say What's your class?
            halt
      done
   elseif %object.vnum% == 51022
      switch %actor.class% 
         case CLERIC
         case PRIEST
            return 0
            say This book is incomplete!  Something has drained its magic!  Fix that and then bring the book back.
            break
         case DIABOLIST
            return 0
            say Wrong one, dummy.
         default
            return 0
            say What's your class?
            halt
      done
      return 0
      say This isn't the power Luchiaans seeks.  Go find it.
      halt
   endif
*
* the old version where you had to go to Doom for the quest
*
*   wait 3s
*   say There is another mage trapped by a malevolent god between life and death.  The continued existence of the mighty battlemage, Solek, is the cause of great instability.  Along with him, his compatriot Velocity, named for the speeds she harnessed in life, is bound by the same god in his keep.  Their continued existence as puppets of a cruel god tears at the very fabric of order and structure.
*   wait 3s
*   say You must find the power to stop Velocity. The utterance will help you.  Bring swift and lasting death to Solek, and return the source of his arcane wisdoms.
*elseif %actor.quest_stage[resurrection_quest]% == 10
*   if %object.vnum% == 48906
*      say Give me the talisman first. I must know that Solek is truly dead.
*      return 0
*      emote refuses your item.
*   endif
*
   wait 3s
   say There is another mage who has managed to ensconce himself in a place where no living being should be.  As a side effect of making his home at the boundary between the mortal realm and the Nine Hells, he has been driven insane.  He still survives all these centuries later through magic stolen from the elves of old.  Along with him, several souls have been marooned in the city's ruins.
   wait 5s
   mecho %self.name% says, 'One in particular, &4&bthe city's weaponsmith&0, cries out for release from her eternal torment.  You must find the power to grant her wish.  The utterance will help you.'
   wait 5s
   mecho %self.name% says, 'Bring swift and lasting death to &4&bthe crazed mage&0, and return the source of his arcane longevity.'
elseif %actor.quest_stage[resurrection_quest]% == 10
   if %object.vnum% == 52001
      say Give me the talisman first.  I must know the crazed mage is truly dead at last.
      return 0
      emote refuses your item.
   endif
elseif %actor.quest_stage[resurrection_quest]% == 11
   if %object.vnum% == 52001
      wait 2
      mpurge %object%
      say The ring is secure again.  It holds more power than a mortal can imagine.
      wait 3s
      say One yet remains of the foolish necromancers, who threatened the very existence of the world with their foolish search for immortality.  End me now, and let this be finished!
      quest advance resurrection_quest %actor%
      set complete 1
      global complete
   endif
else
   return 0
   wait 1
   emote refuses your offering.
   say This isn't what we're after.
endif
~
#8556
Norisent_death_resurrection~
0 af 100
~
if %complete%
   mload obj 8551
   mecho &2&bA small book slips from %self.name%'s robes.&0
endif
~
#8557
ghosts hear Dhewsost Konre~
0 d 0
dhewsost konre~
if %actor.quest_stage[resurrection_quest]% > 0
   mechoaround %actor% &6&b%self.name% is shaken by %actor.name%'s powerful utterance!&0
   msend %actor% &6&b%self.name% is shaken by your powerful utterance!&0
   set quester %actor.name%
   global quester
endif
~
#8558
ghosts death~
0 f 100
~
if %quester%
   quest variable resurrection_quest %quester% %self.vnum% 1
   mecho &6&b%self.name% is purged from this existance.&0
   msend %quester% &5The death talisman twitches slightly. Creepy!&0
endif
~
#8559
Resurrection_quest_grant_spell_get~
1 g 100
~
if %actor.quest_stage[resurrection_quest]% > 10
  wait 1s
  osend %actor% You thumb through the book and find Norisent has taught you all you need to know.
  oload mob 8551
  oforce mob mskillset %actor.name% resurrect
  wait 2
  opurge ai
  osend %actor% &6&bYou have learned Resurrect.&0
  oecho %self.shortdesc% crumbles to dust and blows away.
  quest complete resurrection_quest %actor.name%
  if !%actor.quest_variable[hell_trident:helltask5]% && %actor.quest_stage[hell_trident]% == 2
    quest variable hell_trident %actor% helltask5 1
  endif
  opurge %self%
endif
~
#8560
load trigger for res AI mob~
0 o 100
~
Nothing.
~
#8561
resurrection_talisman_replacement~
0 d 0
I need a new talisman~
wait 2
if %actor.quest_stage[resurrection_quest]% > 4
  grumble
  wait 1s
  say I said not to lose that!
  wait 2s
  quest variable resurrection_quest %actor% new yes
  mecho %self.name% says, 'Go harass Ziijhan again and maybe I'll consider giving you
  mecho &0another.'
endif
~
#8599
resurrection_quest_status_checker~
0 d 100
status status? progress progress?~
wait 2
if %actor.has_completed[resurrection_quest]%
  say You have already learned the secrets of the Resurrection.
  halt
else
  set stage %actor.quest_stage[resurrection_quest]%
  switch %stage%
    case 3
      say Do something to interfere with Ziijhan.
      wait 1s
      say I hear he has a bishop locked up in the dungeon.
      halt
      break
    case 4
      set hunt %get.mob_shortdesc[4004]%, %get.mob_shortdesc[4003]%, and %get.mob_shortdesc[4016]%
      set mob1 4004
      set mob2 4003
      set mob3 4016
      set item %get.obj_shortdesc[4008]%
      break
    case 5
      set item %get.obj_shortdesc[4008]%
      break 
    case 6
      set hunt 2 Xeg-Yi and %get.mob_shortdesc[53308]%
      set mob2 53411
      set mob1 53308
      set item %get.obj_shortdesc[53307]%
      break
    case 7
      set item %get.obj_shortdesc[53307]%
      break
    case 8
      set hunt %get.mob_shortdesc[51005]%, %get.mob_shortdesc[53001]%, and %get.mob_shortdesc[51014]%
      set mob1 51005
      set mob2 53001
      set mob3 51014
      if %actor.class% /= Cleric || %actor.class% /= Priest
        set item a large book on healing from Nordus
      elseif %actor.class% /= Diabolist
        set item %get.obj_shortdesc[51028]%
      endif
      break
    case 9
      if %actor.class% /= Cleric || %actor.class% /= Priest
        set item a large book on healing from Nordus
      elseif %actor.class% /= Diabolist
        set item %get.obj_shortdesc[51028]%
      endif
      break
    case 10
      set hunt %get.mob_shortdesc[52003]% and %get.mob_shortdesc[52015]%
      set mob1 52003
      set mob2 52015
      set item %get.obj_shortdesc[52001]%
      break
    case 11
      set item %get.obj_shortdesc[52001]%
      break
    default 
      say Did you want to do a quest of some kind?  Because you're not at the moment.  Get lost.
  done
endif
if %stage% == 4 || %stage% == 6 || %stage% == 8 || %stage% == 10
  set target1 %actor.quest_variable[resurrection_quest:%mob1%]%
  set target2 %actor.quest_variable[resurrection_quest:%mob2%]%
  if %stage% == 6 || %stage% == 10
    set target3 1
  else 
    set target3 %actor.quest_variable[resurrection_quest:%mob3%]%
  endif
  say You must eliminate:
  mecho %hunt%
  if %stage% == 4 || %stage% == 8
    if %target1% && %target2% && %target3%
      set done 1
    endif
  elseif %stage% == 6
    if %target1% && (%target2% == 2)
      set done 1
    endif
  else
    if %target1% && %target2%
      set done 1
    endif
  endif   
  if %done%
    mecho   
    say You have dispatched them.  Give me the death talisman!
    halt
  endif
  if %target1% || %target2% || %target3%
    mecho   
    mecho You have destroyed:
    if %target1%
      mecho - %get.mob_shortdesc[%mob1%]%
    endif
    if %stage% == 6
      if %target2% == 2
        mecho - %target2% %get.mob_shortdesc[%mob2%]%
      endif
    else
      if %target2%
        mecho - %get.mob_shortdesc[%mob2%]%
      endif
    endif
    if %stage% == 4 || %stage% == 8
      if %target3%
        mecho - %get.mob_shortdesc[%mob3%]%
      endif
    endif
  endif
  if (%stage% != 6 && (!%target1% || !%target2% || !%target3%)) || (%stage% == 6 && (!%target1% || %target2% < 2))
    mecho 
    mecho You still need to dispatch:
    if !%target1%
      mecho - %get.mob_shortdesc[%mob1%]%
    endif
    if %stage% == 6
      if %target2% < 2
        eval xeg (2 - %target2%)
        mecho - %xeg% %get.mob_shortdesc[%mob2%]%
      endif
    else
      if !%target2%
        mecho - %get.mob_shortdesc[%mob2%]%
      endif
    endif
    if !%target3%
      mecho - %get.mob_shortdesc[%mob3%]%
    endif
    mecho 
    mecho Return %item% as proof.
  endif
  mecho 
  mecho Don't forget the banishment phrase: &4&bDhewsost Konre&0
elseif %stage% == 5 || %stage% == 7 || %stage% == 9 || %stage% == 11
  say Bring me back %item%.
endif
if %stage% >= 4
  mecho   
  mecho %self.name% says, 'If you need a new talisman, say &9&b"I need a new talisman"&0.'
endif
~
$~
