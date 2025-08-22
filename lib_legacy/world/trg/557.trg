#55700
Phase Armor - Greet~
0 h 100
~
wait 2
set anti Anti-Paladin
if !%actor% || !%actor.can_be_seen%
  return 0
else
  if %actor.level% <= (20 * (%phase% - 1))
    return 0
  elseif !(%classes% /= %actor.class%) || (%classes% == %anti% && %actor.class% == Paladin)
    return 0
  elseif %actor.quest_stage[phase_armor]% == (%phase% - 1)
    wait 2
    msend %actor% %self.name% tells you, 'Welcome, would you like to do some armor quests?  If so, just ask me about them.'
  endif
endif
if %self.class% /= necromancer
  if %actor.quest_stage[shift_corpse]%
    wait 2
    msend %actor% %self.name% says, 'May Death guide your hunt for Lokari.'
    wait 1s
    msend %actor% %self.name% says, 'If you need a new crystal, msend %actor% %self.name% says, &9&b"I need a new crystal"&0.'
  endif
elseif %self.class% /= pyromancer
  if %actor.quest_stage[supernova]% == 2 && (%actor.inventory[48917]% || %actor.wearing[48917]%)
    quest advance supernova %actor.name%
    set rnd1 %random.3%
    switch %rnd1%
      case 1
        set step3 4318
        break
      case 2
        set step3 10316
        break
      case 3
        set step3 58062
    done
    quest variable supernova %actor.name% step3 %step3%
    set rnd2 %random.3%
    switch %rnd2%
      case 1
        set step4 18577
        break
      case 2
        set step4 17277
        break
      case 3
        set step4 8561
    done
    quest variable supernova %actor.name% step4 %step4%
    set rnd3 %random.3%
    switch %rnd3%
      case 1
        set step5 53219
        break
      case 2
        set step5 47343
        break
      case 3
        set step5 16278
    done
    quest variable supernova %actor.name% step5 %step5%
    set rnd4 %random.3%
    switch %rnd4%
      case 1
        set step6 58657
        break
      case 2
        set step6 35119
        break
      case 3
        set step6 55422
    done
    quest variable supernova %actor.name% step6 %step6%
    set step7 %random.3%
    quest variable supernova %actor.name% step7 %step7%
    wait 2
    msend %actor% %self.name% says, 'Ah I see you found one of Phayla's lamps!'
    wait 3s
    msend %actor% %self.name% says, 'Phayla likes to visit the material plane to engage in her favorite leisure activities.'
    wait 2s
    set clue %actor.quest_variable[supernova:step3]%
    switch %clue%
      case 4318
        msend %actor% %self.name% says, 'Recently, she was spotted in Anduin, taking in a show from the best seat in the house.'
        break
      case 10316
        msend %actor% %self.name% says, 'I understand she frequents the hottest spring at the popular resort up north.'
        break
      case 58062
        msend %actor% %self.name% says, 'She occasionally visits a small remote island theatre, where she enjoys meditating in their reflecting room.'
    done
    wait 2s
    msend %actor% %self.name% says, 'You may be able to find a clue to her whereabouts there.'
  endif
elseif (%self.class% == cleric || %self.class% == priest || %self.class% == Diabolist) && %macestep%
  if %actor.quest_stage[phase_mace]% == %macestep%
    eval minlevel %macestep% * 10
    if %actor.level% >= %minlevel%
      wait 2
      if %actor.quest_variable[phase_mace:greet]% == 0
        msend %actor% %self.name% says, 'I sense a ghostly presence about your weapons.  If you want my help, we can talk about &6&b[upgrades]&0.'
      else
        msend %actor% %self.name% says, 'Do you have what I need for the mace?'
      endif
    endif
  endif
endif
if %actor.quest_stage[ursa_quest] == 1 && %self.vnum% == 6007
  wait 1s
  msend %actor% %self.name% notices the concerned look on your face.
  wait 1s
  msend %actor% %self.name% tells you, 'So, the merchant has gotten himself in quite a bit of trouble.'
  wait 1s
  msend %actor% %self.name% tells you, 'I know what he must do, but he won't like it.'
  chuckle
  wait 1s
  msend %actor% %self.name% dips his quill in a well of blood and scratches out a sinister letter.
  wait 2s
  msend %actor% %self.name% tells you, 'Quickly, take this to him!  Perhaps the Darkness still finds him...  amusing.'
  mload obj 62510
  give letter %actor%
endif
if %actor.quest_stage[hell_trident]% == 1 && %self.vnum% == 6032
  if %actor.level% >= 65
    wait 2
    if !%actor.quest_variable[hell_trident:helltask5]%
      if %actor.has_completed[banish]%
        quest variable hell_trident %actor% helltask5 1
      endif
    endif
    if !%actor.quest_variable[hell_trident:helltask4]%
      if %actor.has_completed[hellfire_brimstone]%
        quest variable hell_trident %actor% helltask4 1
      endif
    endif
    if !%actor.quest_variable[hell_trident:helltask6]%
      if %actor.quest_stage[vilekka_stew]% > 3
        quest variable hell_trident %actor% helltask6 1
      endif
    endif
    if %actor.quest_variable[hell_trident:greet]% == 0
      msend %actor% %self.name% says, 'Hmmm, is that a demon's trident I sense on you?  Perhaps you would like some help with &6&b[upgrades]&0.'
    else
      set job1 %actor.quest_variable[hell_trident:helltask1]%
      set job2 %actor.quest_variable[hell_trident:helltask2]%
      set job3 %actor.quest_variable[hell_trident:helltask3]%
      set job4 %actor.quest_variable[hell_trident:helltask4]%
      set job5 %actor.quest_variable[hell_trident:helltask5]%
      set job6 %actor.quest_variable[hell_trident:helltask6]%
      if %job1% && %job2% && %job3% && %job4% && %job5% && %job6%
        msend %actor% %self.name% says, 'You return with victory in your eyes.  Hand me your trident.'
      else
        msend %actor% %self.name% says, 'Have you met Hell's demands?'
      endif
    endif
  endif
endif
~
#55701
Phase Armor - Start~
0 n 100
yes quests quest hello hi~
wait 2
set anti Anti-Paladin
if !%actor% || !%actor.can_be_seen%
   peer
   say I can't help you if I can't see you.
   halt
elseif !(%classes% /= %actor.class%) || (%classes% == %anti% && %actor.class% == Paladin)
   if %classes% /= and
      msend %actor% %self.name% tells you, 'Sorry, this quest is for the
      msend %actor% &0%classes%
      msend %actor% &0classes only.'
   else
      msend %actor% %self.name% tells you, 'Sorry, this quest is for the
      msend %actor% &0%classes%
      msend %actor% &0class only.'
   endif
   halt
elseif %actor.level% <= 20 * (%phase% - 1)
   msend %actor% %self.name% tells you, 'Sorry, why don't you come back when you've
   msend %actor% &0gained more experience?'
   halt
elseif %actor.quest_stage[phase_armor]% < %phase% - 1
   msend %actor% %self.name% tells you, 'I don't think you're ready for my quests yet.'
   halt
endif
if %actor.quest_stage[phase_armor]% == 0
   quest start phase_armor %actor.name%
elseif %actor.quest_stage[phase_armor]% == %phase% - 1
   quest advance phase_armor %actor.name%
endif
msend %actor% %self.name% tells you, 'Excellent.  I can make %hands_name%, %feet_name%,
msend %actor% &0a %wrist_name%, a %head_name%, %arms_name%, %legs_name%, or a %body_name%.'
wait 2
msend %actor% %self.name% tells you, 'If you want to know about how to quest for
msend %actor% &0one, ask me about it and I will tell you the components you need to get for me
msend %actor% &0in order to receive your reward.'
wait 2
msend %actor% %self.name% tells you, 'Remember, you can ask me &6&barmor progress&0 at any
msend %actor% &0time and I'll tell you what you have given me so far.'
wait 2
if !%actor.inventory[299]% && !%actor.wearing[299]%
  msend %actor% You can also track the progress of all your quests in this journal.
  mload obj 299
  give quest-journal %actor.name%
  wait 2
  msend %actor% &6&b[Look]&0 at the journal for instructions on how to use it.
endif
~
#55702
Phase Armor - Armor~
0 n 100
boots slippers sandals helm cap circlet coif turban gauntlets gloves fistwraps vambraces sleeves greaves pants leggings plate guard jerkin tunic robe bracer bracelet mittens crown doublet~
wait 2
* If you add more types of armor, you need to add their
* keywords in the argument field for this trigger.
if !%actor% || !%actor.can_be_seen%
   peer
   say I can't help you if I can't see you.
   halt
elseif !(%classes% /= %actor.class%) || (%classes% == %anti% && %actor.class% == Paladin)
   if %classes% /= and
      msend %actor% %self.name% tells you, 'Sorry, this quest is for the
      msend %actor% &0%classes%
      msend %actor% &0classes only.'
   else
      msend %actor% %self.name% tells you, 'Sorry, this quest is for the
      msend %actor% &0%classes%
      msend %actor% &0class only.'
   endif
   halt
elseif %actor.level% <= 20 * (%phase% - 1)
   msend %actor% %self.name% tells you, 'Sorry, why don't you come back when you've
   msend %actor% &0gained more experience?'
   halt
elseif %actor.quest_stage[phase_armor]% < %phase%
   msend %actor% %self.name% tells you, 'I don't think you're ready for my quests yet.'
   halt
endif
if %hands_name%? /= %speech%
   set name pair of %hands_name%
   set gem_vnum %hands_gem%
   set armor_vnum %hands_armor%
elseif %feet_name%? /= %speech%
   set name pair of %feet_name%
   set gem_vnum %feet_gem%
   set armor_vnum %feet_armor%
elseif %wrist_name%? /= %speech%
   set name %wrist_name%
   set gem_vnum %wrist_gem%
   set armor_vnum %wrist_armor%
elseif %head_name%? /= %speech%
   set name %head_name%
   set gem_vnum %head_gem%
   set armor_vnum %head_armor%
elseif %arms_name%? /= %speech%
   set name pair of %arms_name%
   set gem_vnum %arms_gem%
   set armor_vnum %arms_armor%
elseif %legs_name%? /= %speech%
   set name pair of %legs_name%
   set gem_vnum %legs_gem%
   set armor_vnum %legs_armor%
elseif %body_name%? /= %speech%
   set name %body_name%
   set gem_vnum %body_gem%
   set armor_vnum %body_armor%
endif
if %name%
   msend %actor% %self.name% tells you, 'Well, I can make a fine %name% for you, but
   msend %actor% &0I'll need &6&b%get.obj_shortdesc[%armor_vnum%]%&0 and &6&bthree %get.obj_pldesc[%gem_vnum%]%&0.
   msend %actor% &0Bring these things to me in any order, at any time, and I will reward you.'
   msend %actor%   
   msend %actor% %self.name% tells you, 'You will find them on creatures across the world.
   if %phase% == 1
      msend %actor% &0Creatures &6&bup to level 20&0 will drop them.'
   elseif %phase% == 2
      msend %actor% &0Creatures &b&6level 20 to 40&0 will drop them.'
   elseif %phase% == 3
      msend %actor% &0Creatures &b&6level 40 to 70&0 will drop them.'
   endif
else
   msend %actor% %self.name% peers at you oddly.
   mechoaround %actor% %self.name% peers at %actor.name% oddly.
   msend %actor% %self.name% tells you, 'I don't think I can make that for you.'
endif
~
#55703
Phase Armor - Receive~
0 j 100
~
switch %object.vnum%
  case %hands_armor%
  case %hands_gem%
  case %feet_armor%
  case %feet_gem% 
  case %wrist_armor%
  case %wrist_gem%
  case %head_armor%
  case %head_gem%
  case %arms_armor%
  case %arms_gem%
  case %legs_armor%
  case %legs_gem%
  case %body_armor%
  case %body_gem%
    set anti Anti-Paladin
    if !%actor% || !%actor.can_be_seen%
      return 0
      peer
      msend %actor% %self.name% says, 'I can't help you if I can't see you.'
      halt
    endif
    if !(%classes% /= %actor.class%) || (%classes% == %anti% && %actor.class% == Paladin)
      return 0
      if %classes% /= and
        msend %actor% %self.name% tells you, 'Sorry, this quest is for the %classes% classes only.'
      else
        msend %actor% %self.name% tells you, 'Sorry, this quest is for the %classes% class only.'
      endif
      halt
    elseif %actor.level% <= 20 * (%phase% - 1)
      return 0
      msend %actor% %self.name% tells you, 'Sorry, why don't you come back when you've gained more experience?'
      msend %actor% %self.name% refuses your item.
      halt
    elseif %actor.quest_stage[phase_armor]% < %phase%
      return 0
      msend %actor% %self.name% tells you, 'I don't think you're ready for my armor quests yet.'
      msend %actor% %self.name% refuses your item.
      halt
    end
    set object_vnum %object.vnum%
    switch %object_vnum%
      case %hands_armor%
        set is_armor 1
      case %hands_gem%
        set exp_x 1
        set armor_vnum %hands_armor%
        set gem_vnum %hands_gem%
        set reward_vnum %hands_reward%
        break
      case %feet_armor%
        set is_armor 1
      case %feet_gem%
        set exp_x 2
        set armor_vnum %feet_armor%
        set gem_vnum %feet_gem%
        set reward_vnum %feet_reward%
        break
      case %wrist_armor%
        set is_armor 1
      case %wrist_gem%
        set exp_x 3
        set armor_vnum %wrist_armor%
        set gem_vnum %wrist_gem%
        set reward_vnum %wrist_reward%
        break
      case %head_armor%
        set is_armor 1
      case %head_gem%
        set exp_x 4
        set armor_vnum %head_armor%
        set gem_vnum %head_gem%
        set reward_vnum %head_reward%
        break
      case %arms_armor%
        set is_armor 1
      case %arms_gem%
        set exp_x 6
        set armor_vnum %arms_armor%
        set gem_vnum %arms_gem%
        set reward_vnum %arms_reward%
        break
      case %legs_armor%
        set is_armor 1
      case %legs_gem%
        set exp_x 8
        set armor_vnum %legs_armor%
        set gem_vnum %legs_gem%
        set reward_vnum %legs_reward%
        break
      case %body_armor%
        set is_armor 1
      case %body_gem%
        set exp_x 10
        set armor_vnum %body_armor%
        set gem_vnum %body_gem%
        set reward_vnum %body_reward%
    done
    if %is_armor%
      if !%actor.quest_variable[phase_armor:%object_vnum%_armor_acquired]%
        quest variable phase_armor %actor.name% %object_vnum%_armor_acquired 0
      end
      set armor_count %actor.quest_variable[phase_armor:%object_vnum%_armor_acquired]%
      if %armor_count% < 1
        return 1
        wait 1
        mjunk %object.name%
        eval armor_count %armor_count% + 1
        quest variable phase_armor %actor.name% %object_vnum%_armor_acquired %armor_count%
        msend %actor% %self.name% tells you, 'Hey now, what have we here!?
        msend %actor% &0I've been looking for this for some time.
        msend %actor% &0You have now given me %get.obj_shortdesc[%object_vnum%]%.'
      else
        return 0
        wait 2
        eye %actor.name%
        msend %actor% %self.name% tells you, 'Hey now, you've already given me one of these!'
        msend %actor% %self.name% refuses your item.
        halt
      end
    else
      if !%actor.quest_variable[phase_armor:%object_vnum%_gems_acquired]%
        quest variable phase_armor %actor.name% %object_vnum%_gems_acquired 0
      end
      set gem_count %actor.quest_variable[phase_armor:%object_vnum%_gems_acquired]%
      if %gem_count% < 3
        return 1
        wait 1
        mjunk %object.name%
        eval gem_count %gem_count% + 1
        quest variable phase_armor %actor.name% %object_vnum%_gems_acquired %gem_count%
        msend %actor% %self.name% tells you, 'Hey, very nice.'
        wait 2
        msend %actor% %self.name% tells you, 'It is good to see that adventurers are out conquering the realm.'
        wait 2
        if %gem_count% == 1
            msend %actor% %self.name% tells you, 'You have given me 1 %get.obj_noadesc[%object_vnum%]%.'
        else
            msend %actor% %self.name% tells you, 'You have given me %gem_count% %get.obj_pldesc[%object_vnum%]%.'
        end
      else
        return 0
        wait 1
        eye %actor.name%
        msend %actor% %self.name% tells you, 'Hey now, you've already given me %gem_count% of these!'
        msend %actor% %self.name% refuses your item.
        halt
      end
    end
    *
    * Check to see if the quest is complete and if the reward can be given.
    *
    if (%actor.quest_variable[phase_armor:%gem_vnum%_gems_acquired]% == 3) && (%actor.quest_variable[phase_armor:%armor_vnum%_armor_acquired]% == 1)
      wait 2
      msend %actor% %self.name% tells you, 'Excellent work, intrepid adventurer!
      msend %actor% &0You have provided me with all I need to reward you with:
      msend %actor% &0%get.obj_shortdesc[%reward_vnum%]%!'
      wait 2
      msend %actor% &7&bYou gain experience!&0
      switch %actor.class%
        case Warrior
        case Berserker
          set exp 7590
          break
        case Paladin
        case Anti-Paladin
        case Ranger
          set exp 7935
          break
        case Sorcerer
        case Pyromancer
        case Cryomancer
        case Illusionist
        case Bard
          set exp 8280
          break
        case Necromancer
        case Monk
          set exp 8970
          break
        default
          set exp 6900
      done
      if %phase% == 2
        * eval exp (%exp% * 433) / 100
        eval exp (%exp% * 608) / 100
      elseif %phase% == 3
        * eval exp (%exp% * 870) / 100
        eval exp (%exp% * 1340) / 100
      end
      set lap 1
      while %lap% <= %exp_x%
        mexp %actor% %exp%
        eval lap %lap% + 1
      done
      mload obj %reward_vnum%
      give all %actor.name%
      drop all
      msave %actor%
    end
done
~
#55704
Phase Armor - Status~
0 n 0
armor progress~
wait 2
if !%actor% || !%actor.can_be_seen%
   peer
   say I can't help you if I can't see you.
   halt
elseif !(%classes% /= %actor.class%) || (%classes% == %anti% && %actor.class% == Paladin)
   if %classes% /= and
      msend %actor% %self.name% tells you, 'Sorry, this quest is for the
      msend %actor% &0%classes%
      msend %actor% &0classes only.'
   else
      msend %actor% %self.name% tells you, 'Sorry, this quest is for the
      msend %actor% &0%classes%
      msend %actor% &0class only.'
   endif
   halt
elseif %actor.level% <= 20 * (%phase% - 1)
   msend %actor% %self.name% tells you, 'Sorry, why don't you come back when you've
   msend %actor% &0gained more experience?'
   halt
elseif %actor.quest_stage[phase_armor]% < %phase%
   msend %actor% %self.name% tells you, 'I don't think you're ready for my quests yet.'
   halt
endif
set got_hands %actor.quest_variable[phase_armor:%hands_armor%_armor_acquired]%
set got_feet %actor.quest_variable[phase_armor:%feet_armor%_armor_acquired]%
set got_wrist %actor.quest_variable[phase_armor:%wrist_armor%_armor_acquired]%
set got_head %actor.quest_variable[phase_armor:%head_armor%_armor_acquired]%
set got_arms %actor.quest_variable[phase_armor:%arms_armor%_armor_acquired]%
set got_legs %actor.quest_variable[phase_armor:%legs_armor%_armor_acquired]%
set got_body %actor.quest_variable[phase_armor:%body_armor%_armor_acquired]%
set hands_count %actor.quest_variable[phase_armor:%hands_gem%_gems_acquired]%
set feet_count %actor.quest_variable[phase_armor:%feet_gem%_gems_acquired]%
set wrist_count %actor.quest_variable[phase_armor:%wrist_gem%_gems_acquired]%
set head_count %actor.quest_variable[phase_armor:%head_gem%_gems_acquired]%
set arms_count %actor.quest_variable[phase_armor:%arms_gem%_gems_acquired]%
set legs_count %actor.quest_variable[phase_armor:%legs_gem%_gems_acquired]%
set body_count %actor.quest_variable[phase_armor:%body_gem%_gems_acquired]%
eval done_hands %got_hands% == 1 && %hands_count% == 3
eval done_feet %got_feet% == 1 && %feet_count% == 3
eval done_wrist %got_wrist% == 1 && %wrist_count% == 3
eval done_head %got_head% == 1 && %head_count% == 3
eval done_arms %got_arms% == 1 && %arms_count% == 3
eval done_legs %got_legs% == 1 && %legs_count% == 3
eval done_body %got_body% == 1 && %body_count% == 3
eval given %got_hands% + %got_feet% + %got_wrist% + %got_head% + %got_arms% + %got_legs% + %got_body%
eval given %given% + %hands_count% + %feet_count% + %wrist_count% + %head_count% + %arms_count% + %legs_count% + %body_count%
eval unrewarded (%got_hands% + %hands_count% != 4) + (%got_feet% + %feet_count% != 4) + (%got_wrist% + %wrist_count% != 4)
eval unrewarded %unrewarded% + (%got_head% + %head_count% != 4) + (%got_arms% + %arms_count% != 4)
eval unrewarded %unrewarded% + (%got_legs% + %legs_count% != 4) + (%got_body% + %body_count% != 4)
msend %actor% %self.name% tells you, 'Look for treasures from creatures:
if %phase% == 1
   msend %actor% &6&bbelow level 20&0.'&_
elseif %phase% == 2
   msend %actor% &6&blevel 20 to 40&0.'&_
elseif %phase% == 3
   msend %actor% &6&blevel 40 to 70&0.'&_
endif
if !%given%
   msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
   halt
elseif %unrewarded%
   msend %actor% %self.name% tells you, 'You have given me:'
endif
if %got_hands% && !%done_hands%
   msend %actor% &0  %get.obj_shortdesc[%hands_armor%]%
endif
if %hands_count% && !%done_hands%
   msend %actor% &0  %hands_count% of %get.obj_shortdesc[%hands_gem%]%
endif
if %got_feet% && !%done_feet%
   msend %actor% &0  %get.obj_shortdesc[%feet_armor%]%
endif
if %feet_count% && !%done_feet%
   msend %actor% &0  %feet_count% of %get.obj_shortdesc[%feet_gem%]%
endif
if %got_wrist% && !%done_wrist%
   msend %actor% &0  %get.obj_shortdesc[%wrist_armor%]%
endif
if %wrist_count% && !%done_wrist%
   msend %actor% &0  %wrist_count% of %get.obj_shortdesc[%wrist_gem%]%
endif
if %got_head% && !%done_head%
   msend %actor% &0  %get.obj_shortdesc[%head_armor%]%
endif
if %head_count% && !%done_head%
   msend %actor% &0  %head_count% of %get.obj_shortdesc[%head_gem%]%
endif
if %got_arms% && !%done_arms%
   msend %actor% &0  %get.obj_shortdesc[%arms_armor%]%
endif
if %arms_count% && !%done_arms%
   msend %actor% &0  %arms_count% of %get.obj_shortdesc[%arms_gem%]%
endif
if %got_legs% && !%done_legs%
   msend %actor% &0  %get.obj_shortdesc[%legs_armor%]%
endif
if %legs_count% && !%done_legs%
   msend %actor% &0  %legs_count% of %get.obj_shortdesc[%legs_gem%]%
endif
if %got_body% && !%done_body%
   msend %actor% &0  %get.obj_shortdesc[%body_armor%]%
endif
if %body_count% && !%done_body%
   msend %actor% &0  %body_count% of %get.obj_shortdesc[%body_gem%]%
endif
if %done_hands% || %done_feet% || %done_wrist% || %done_head% || %done_arms% || %done_legs% || %done_body%
   msend %actor% &_
   msend %actor% %self.name% tells you, 'You have completed quests for:'
endif
if %done_hands%
   msend %actor% &0  %get.obj_shortdesc[%hands_reward%]%
endif
if %done_feet%
   msend %actor% &0  %get.obj_shortdesc[%feet_reward%]%
endif
if %done_wrist%
   msend %actor% &0  %get.obj_shortdesc[%wrist_reward%]%
endif
if %done_head%
   msend %actor% &0  %get.obj_shortdesc[%head_reward%]%
endif
if %done_arms%
   msend %actor% &0  %get.obj_shortdesc[%arms_reward%]%
endif
if %done_legs%
   msend %actor% &0  %get.obj_shortdesc[%legs_reward%]%
endif
if %done_body%
   msend %actor% &0  %get.obj_shortdesc[%body_reward%]%
endif
~
#55705
Phase Armor - Where~
0 n 100
where~
set anti Anti-Paladin
set max_phase 3
if !%actor% || !%actor.can_be_seen%
   peer
   say I can't help you if I can't see you.
   halt
elseif !(%classes% /= %actor.class%) || (%classes% == %anti% && %actor.class% == Paladin)
   if %classes% /= and
      msend %actor% %self.name% tells you, 'Sorry, this quest is for the %classes% classes only.'
   else
      msend %actor% %self.name% tells you, 'Sorry, this quest is for the %classes% class only.'
   endif
   halt
elseif %actor.level% <= 20 * (%phase% - 1)
   msend %actor% %self.name% tells you, 'Sorry, why don't you come back when you've gained more experience?'
   halt
elseif %actor.quest_stage[phase_armor]% < %phase% - 1
   msend %actor% %self.name% tells you, 'I don't think you're ready for my quests yet.'
   halt
endif
eval low_level (%phase% - 1) * 20 + 1
eval high_level %phase% * 20
wait 2
msend %actor% %self.name% tells you, 'You can find the items I seek by playing and adventuring in areas appropriate to your level.'
msend %actor% %self.name% tells you, 'My quests are for those from level %low_level% to %high_level%, so the items I desire can be found in areas of that difficulty.'
if %phase% < %max_phase%
  msend %actor% %self.name% tells you, 'Once you have agreed to take on my quests you can seek another like me for tasks a bit more difficult.'
endif
~
#55706
Guild Master refuse~
0 j 100
~
switch %self.vnum%
  case 3021
  case 10000
  case 6218
  case 6007
    switch %object.vnum%
      case %maceitem2%
      case %maceitem3%
      case %maceitem4%
      case %maceitem5%
      case %macevnum%
        halt
    done
  case 6032
    switch %object.vnum%
      case 55662
      case 2334
        halt
    done
done
switch %object.vnum%
  case %hands_armor%
  case %hands_gem%
  case %feet_armor%
  case %feet_gem%
  case %wrist_armor%
  case %wrist_gem%
  case %head_armor%
  case %head_gem%
  case %arms_armor%
  case %arms_gem%
  case %legs_armor%
  case %legs_gem%
  case %body_armor%
  case %body_gem%
    halt
    break
  default
    return 0
    wait 1
    eye %actor.name%
    msend %actor% %self.name% tells you, 'I am not interested in this from you.'
    msend %actor% %self.name% returns your item to you.
done
~
#55710
Phase Armor - Init - Sor/Cry/Pyr/Nec/Ill Phase 1~
0 o 100
~
* global variables
set classes Sorcerer, Cryomancer, Pyromancer, Illusionist, and Necromancer
set phase 1
* gem variables
set feet_gem 55571
set head_gem 55579
set hands_gem 55567
set arms_gem 55583
set legs_gem 55587
set body_gem 55591
set wrist_gem 55575
* armor variables
set feet_armor 55306
set head_armor 55314
set hands_armor 55302
set arms_armor 55318
set legs_armor 55322
set body_armor 55326
set wrist_armor 55310
* reward variables
set feet_reward 55402
set head_reward 55398
set hands_reward 55404
set arms_reward 55399
set legs_reward 55401
set body_reward 55400
set wrist_reward 55403
* name variables
set feet_name sandals
set head_name turban
set hands_name gloves
set arms_name sleeves
set legs_name pants
set body_name tunic
set wrist_name bracelet
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55711
Phase Armor - Init - Sor/Cry/Pyr/Ill Phase 2~
0 o 100
~
* global variables
set classes Sorcerer, Cryomancer, Pyromancer, and Illusionist
set phase 2
* gem variables
set feet_gem 55613
set head_gem 55635
set hands_gem 55602
set arms_gem 55646
set legs_gem 55657
set body_gem 55668
set wrist_gem 55624
* armor variables
set feet_armor 55335
set head_armor 55343
set hands_armor 55331
set arms_armor 55347
set legs_armor 55351
set body_armor 55355
set wrist_armor 55339
* reward variables
set feet_reward 55479
set head_reward 55475
set hands_reward 55481
set arms_reward 55476
set legs_reward 55478
set body_reward 55477
set wrist_reward 55480
* name variables
set feet_name slippers
set head_name coif
set hands_name gloves
set arms_name sleeves
set legs_name pants
set body_name robe
set wrist_name bracelet
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem 
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor 
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward 
global feet_name head_name hands_name arms_name legs_name body_name wrist_name 
~
#55712
Phase Armor - Init - Sor/Cry/Pyr/Ill Phase 3~
0 o 100
~
* global variables
set classes Sorcerer, Cryomancer, Pyromancer, and Illusionist
set phase 3
* gem variables
set feet_gem 55690
set head_gem 55712
set hands_gem 55679
set arms_gem 55723
set legs_gem 55734
set body_gem 55745
set wrist_gem 55701
* armor variables
set feet_armor 55363
set head_armor 55371
set hands_armor 55359
set arms_armor 55375
set legs_armor 55379
set body_armor 55383
set wrist_armor 55367
* reward variables
set feet_reward 55556
set head_reward 55552
set hands_reward 55558
set arms_reward 55553
set legs_reward 55555
set body_reward 55554
set wrist_reward 55557
* name variables
set feet_name slippers
set head_name cap
set hands_name gloves
set arms_name sleeves
set legs_name leggings
set body_name robe
set wrist_name bracelet
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55713
Phase Armor - Init - Nec Phase 2~
0 o 100
~
* global variables
set classes Necromancer
set phase 2
* gem variables
set feet_gem 55609
set head_gem 55631
set hands_gem 55598
set arms_gem 55642
set legs_gem 55653
set body_gem 55664
set wrist_gem 55620
* armor variables
set feet_armor 55335
set head_armor 55343
set hands_armor 55331
set arms_armor 55347
set legs_armor 55351
set body_armor 55355
set wrist_armor 55339
* reward variables
set feet_reward 55458
set head_reward 55454
set hands_reward 55460
set arms_reward 55455
set legs_reward 55457
set body_reward 55456
set wrist_reward 55459
* name variables
set feet_name slippers
set head_name cap
set hands_name gloves
set arms_name sleeves
set legs_name pants
set body_name robe
set wrist_name bracelet
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55714
Phase Armor - Init - Nec Phase 3~
0 o 100
~
* global variables
set classes Necromancer
set phase 3
* gem variables
set feet_gem 55686
set head_gem 55708
set hands_gem 55675
set arms_gem 55719
set legs_gem 55730
set body_gem 55741
set wrist_gem 55697
* armor variables
set feet_armor 55363
set head_armor 55371
set hands_armor 55359
set arms_armor 55375
set legs_armor 55379
set body_armor 55383
set wrist_armor 55367
* reward variables
set feet_reward 55535
set head_reward 55531
set hands_reward 55537
set arms_reward 55532
set legs_reward 55534
set body_reward 55533
set wrist_reward 55536
* name variables
set feet_name slippers
set head_name cap
set hands_name gloves
set arms_name sleeves
set legs_name leggings
set body_name robe
set wrist_name bracelet
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55715
Phase Armor - Init - Cle/Pri/Dia/Dru Phase 1~
0 o 100
~
* global variables
set classes Cleric, Priest, Druid, and Diabolist
set phase 1
* gem variables
set feet_gem 55570
set head_gem 55578
set hands_gem 55566
set arms_gem 55582
set legs_gem 55586
set body_gem 55590
set wrist_gem 55574
* armor variables
set feet_armor 55304
set head_armor 55312
set hands_armor 55300
set arms_armor 55316
set legs_armor 55320
set body_armor 55324
set wrist_armor 55308
* reward variables
set feet_reward 55395
set head_reward 55391
set hands_reward 55397
set arms_reward 55392
set legs_reward 55394
set body_reward 55393
set wrist_reward 55396
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name plate
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55716
Phase Armor - Init - Cle/Pri Phase 2~
0 o 100
~
* global variables
set classes Cleric and Priest
set phase 2
* gem variables
set feet_gem 55612
set head_gem 55634
set hands_gem 55601
set arms_gem 55645
set legs_gem 55656
set body_gem 55667
set wrist_gem 55623
* armor variables
set feet_armor 55332
set head_armor 55340
set hands_armor 55328
set arms_armor 55344
set legs_armor 55348
set body_armor 55352
set wrist_armor 55336
* reward variables
set feet_reward 55437
set head_reward 55433
set hands_reward 55439
set arms_reward 55434
set legs_reward 55436
set body_reward 55435
set wrist_reward 55438
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name plate
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55717
Phase Armor - Init - Cle/Pri Phase 3~
0 o 100
~
* global variables
set classes Cleric and Priest
set phase 3
* gem variables
set feet_gem 55689
set head_gem 55711
set hands_gem 55678
set arms_gem 55722
set legs_gem 55733
set body_gem 55744
set wrist_gem 55700
* armor variables
set feet_armor 55360
set head_armor 55368
set hands_armor 55356
set arms_armor 55372
set legs_armor 55376
set body_armor 55380
set wrist_armor 55364
* reward variables
set feet_reward 55514
set head_reward 55510
set hands_reward 55516
set arms_reward 55511
set legs_reward 55513
set body_reward 55512
set wrist_reward 55515
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name guard
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55718
Phase Armor - Init - Dia Phase 2~
0 o 100
~
* global variables
set classes Diabolist
set phase 2
* gem variables
set feet_gem 55606
set head_gem 55628
set hands_gem 55595
set arms_gem 55639
set legs_gem 55650
set body_gem 55661
set wrist_gem 55617
* armor variables
set feet_armor 55332
set head_armor 55340
set hands_armor 55328
set arms_armor 55344
set legs_armor 55348
set body_armor 55352
set wrist_armor 55336
* reward variables
set feet_reward 55472
set head_reward 55468
set hands_reward 55474
set arms_reward 55469
set legs_reward 55471
set body_reward 55470
set wrist_reward 55473
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name guard
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55719
Phase Armor - Init - Dia Phase 3~
0 o 100
~
* global variables
set classes Diabolist
set phase 3
* gem variables
set feet_gem 55683
set head_gem 55705
set hands_gem 55672
set arms_gem 55716
set legs_gem 55727
set body_gem 55738
set wrist_gem 55694
* armor variables
set feet_armor 55360
set head_armor 55368
set hands_armor 55356
set arms_armor 55372
set legs_armor 55376
set body_armor 55380
set wrist_armor 55364
* reward variables
set feet_reward 55549
set head_reward 55545
set hands_reward 55551
set arms_reward 55546
set legs_reward 55548
set body_reward 55547
set wrist_reward 55550
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name guard
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55720
Phase Armor - Init - Dru Phase 2~
0 o 100
~
* global variables
set classes Druid
set phase 2
* gem variables
set feet_gem 55607
set head_gem 55629
set hands_gem 55596
set arms_gem 55640
set legs_gem 55651
set body_gem 55662
set wrist_gem 55618
* armor variables
set feet_armor 55334
set head_armor 55342
set hands_armor 55330
set arms_armor 55346
set legs_armor 55350
set body_armor 55354
set wrist_armor 55338
* reward variables
set feet_reward 55451
set head_reward 55447
set hands_reward 55453
set arms_reward 55448
set legs_reward 55450
set body_reward 55449
set wrist_reward 55452
* name variables
set feet_name boots
set head_name cap
set hands_name gloves
set arms_name sleeves
set legs_name pants
set body_name jerkin
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55721
Phase Armor - Init - Dru Phase 3~
0 o 100
~
* global variables
set classes Druid
set phase 3
* gem variables
set feet_gem 55684
set head_gem 55706
set hands_gem 55673
set arms_gem 55717
set legs_gem 55728
set body_gem 55739
set wrist_gem 55695
* armor variables
set feet_armor 55362
set head_armor 55370
set hands_armor 55358
set arms_armor 55374
set legs_armor 55378
set body_armor 55382
set wrist_armor 55366
* reward variables
set feet_reward 55528
set head_reward 55524
set hands_reward 55530
set arms_reward 55525
set legs_reward 55527
set body_reward 55526
set wrist_reward 55529
* name variables
set feet_name boots
set head_name helm
set hands_name gloves
set arms_name sleeves
set legs_name pants
set body_name tunic
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55722
Phase Armor - Init - War/Ant/Ran/Pal/Mon Phase 1~
0 o 100
~
* global variables
set classes Warrior, Anti-Paladin, Ranger, Paladin, Monk, and Berserker
set phase 1
* gem variables
set feet_gem 55573
set head_gem 55581
set hands_gem 55569
set arms_gem 55585
set legs_gem 55589
set body_gem 55593
set wrist_gem 55577
* armor variables
set feet_armor 55304
set head_armor 55312
set hands_armor 55300
set arms_armor 55316
set legs_armor 55320
set body_armor 55324
set wrist_armor 55308
* reward variables
set feet_reward 55388
set head_reward 55384
set hands_reward 55390
set arms_reward 55385
set legs_reward 55387
set body_reward 55386
set wrist_reward 55389
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name plate
set wrist_name bracer
* globalize everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55723
Phase Armor - Init - War Phase 2~
0 o 100
~
* global variables
set classes Warrior
set phase 2
* gem variables
set feet_gem 55615
set head_gem 55637
set hands_gem 55604
set arms_gem 55648
set legs_gem 55659
set body_gem 55670
set wrist_gem 55626
* armor variables
set feet_armor 55332
set head_armor 55340
set hands_armor 55328
set arms_armor 55344
set legs_armor 55348
set body_armor 55352
set wrist_armor 55336
* reward variables
set feet_reward 55416
set head_reward 55412
set hands_reward 55418
set arms_reward 55413
set legs_reward 55415
set body_reward 55414
set wrist_reward 55417
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name plate
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55724
Phase Armor - Init - War Phase 3~
0 o 100
~
* global variables
set classes Warrior
set phase 3
* gem variables
set feet_gem 55692
set head_gem 55714
set hands_gem 55681
set arms_gem 55725
set legs_gem 55736
set body_gem 55747
set wrist_gem 55703
* armor variables
set feet_armor 55360
set head_armor 55368
set hands_armor 55356
set arms_armor 55372
set legs_armor 55376
set body_armor 55380
set wrist_armor 55364
* reward variables
set feet_reward 55493
set head_reward 55489
set hands_reward 55495
set arms_reward 55490
set legs_reward 55492
set body_reward 55491
set wrist_reward 55494
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name plate
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55725
Phase Armor - Init - Ant Phase 2~
0 o 100
~
* global variables
set classes Anti-Paladin
set phase 2
* gem variables
set feet_gem 55605
set head_gem 55627
set hands_gem 55594
set arms_gem 55638
set legs_gem 55649
set body_gem 55660
set wrist_gem 55616
* armor variables
set feet_armor 55332
set head_armor 55340
set hands_armor 55328
set arms_armor 55344
set legs_armor 55348
set body_armor 55352
set wrist_armor 55336
* reward variables
set feet_reward 55430
set head_reward 55426
set hands_reward 55432
set arms_reward 55427
set legs_reward 55429
set body_reward 55428
set wrist_reward 55431
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name plate
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55726
Phase Armor - Init - Ant Phase 3~
0 o 100
~
* global variables
set classes Anti-Paladin
set phase 3
* gem variables
set feet_gem 55682
set head_gem 55704
set hands_gem 55671
set arms_gem 55715
set legs_gem 55726
set body_gem 55737
set wrist_gem 55693
* armor variables
set feet_armor 55360
set head_armor 55368
set hands_armor 55356
set arms_armor 55372
set legs_armor 55376
set body_armor 55380
set wrist_armor 55364
* reward variables
set feet_reward 55507
set head_reward 55503
set hands_reward 55509
set arms_reward 55504
set legs_reward 55506
set body_reward 55505
set wrist_reward 55508
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name plate
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55727
Phase Armor - Init - Ran Phase 2~
0 o 100
~
* global variables
set classes Ranger
set phase 2
* gem variables
set feet_gem 55611
set head_gem 55633
set hands_gem 55600
set arms_gem 55644
set legs_gem 55655
set body_gem 55666
set wrist_gem 55622
* armor variables
set feet_armor 55333
set head_armor 55341
set hands_armor 55329
set arms_armor 55345
set legs_armor 55349
set body_armor 55353
set wrist_armor 55337
* reward variables
set feet_reward 55444
set head_reward 55440
set hands_reward 55446
set arms_reward 55441
set legs_reward 55443
set body_reward 55442
set wrist_reward 55445
* name variables
set feet_name boots
set head_name cap
set hands_name gloves
set arms_name sleeves
set legs_name pants
set body_name jerkin
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55728
Phase Armor - Init - Ran Phase 3~
0 o 100
~
* global variables
set classes Ranger
set phase 3
* gem variables
set feet_gem 55688
set head_gem 55710
set hands_gem 55677
set arms_gem 55721
set legs_gem 55732
set body_gem 55743
set wrist_gem 55699
* armor variables
set feet_armor 55361
set head_armor 55369
set hands_armor 55357
set arms_armor 55373
set legs_armor 55377
set body_armor 55381
set wrist_armor 55365
* reward variables
set feet_reward 55521
set head_reward 55517
set hands_reward 55523
set arms_reward 55518
set legs_reward 55520
set body_reward 55519
set wrist_reward 55522
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name sleeves
set legs_name leggings
set body_name tunic
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55729
Phase Armor - Init - Pal Phase 2~
0 o 100
~
* global variables
set classes Paladin
set phase 2
* gem variables
set feet_gem 55610
set head_gem 55632
set hands_gem 55599
set arms_gem 55643
set legs_gem 55654
set body_gem 55665
set wrist_gem 55621
* armor variables
set feet_armor 55332
set head_armor 55340
set hands_armor 55328
set arms_armor 55344
set legs_armor 55348
set body_armor 55352
set wrist_armor 55336
* reward variables
set feet_reward 55423
set head_reward 55419
set hands_reward 55425
set arms_reward 55420
set legs_reward 55422
set body_reward 55421
set wrist_reward 55424
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name plate
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55730
Phase Armor - Init - Pal Phase 3~
0 o 100
~
* global variables
set classes Paladin
set phase 3
* gem variables
set feet_gem 55687
set head_gem 55709
set hands_gem 55676
set arms_gem 55720
set legs_gem 55731
set body_gem 55742
set wrist_gem 55698
* armor variables
set feet_armor 55360
set head_armor 55368
set hands_armor 55356
set arms_armor 55372
set legs_armor 55376
set body_armor 55380
set wrist_armor 55364
* reward variables
set feet_reward 55500
set head_reward 55496
set hands_reward 55502
set arms_reward 55497
set legs_reward 55499
set body_reward 55498
set wrist_reward 55501
* name variables
set feet_name boots
set head_name helm
set hands_name gauntlets
set arms_name vambraces
set legs_name greaves
set body_name plate
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55731
Phase Armor - Init - Mon Phase 2~
0 o 100
~
* global variables
set classes Monk
set phase 2
* gem variables
set feet_gem 55608
set head_gem 55630
set hands_gem 55597
set arms_gem 55641
set legs_gem 55652
set body_gem 55663
set wrist_gem 55619
* armor variables
set feet_armor 55334
set head_armor 55342
set hands_armor 55330
set arms_armor 55346
set legs_armor 55350
set body_armor 55354
set wrist_armor 55338
* reward variables
set feet_reward 55465
set head_reward 55461
set hands_reward 55467
set arms_reward 55462
set legs_reward 55464
set body_reward 55463
set wrist_reward 55466
* name variables
set feet_name boots
set head_name cap
set hands_name fistwraps
set arms_name sleeves
set legs_name leggings
set body_name tunic
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55732
Phase Armor - Init - Mon Phase 3~
0 o 100
~
* global variables
set classes Monk
set phase 3
* gem variables
set feet_gem 55685
set head_gem 55707
set hands_gem 55674
set arms_gem 55718
set legs_gem 55729
set body_gem 55740
set wrist_gem 55696
* armor variables
set feet_armor 55362
set head_armor 55370
set hands_armor 55358
set arms_armor 55374
set legs_armor 55378
set body_armor 55382
set wrist_armor 55366
* reward variables
set feet_reward 55542
set head_reward 55538
set hands_reward 55544
set arms_reward 55539
set legs_reward 55541
set body_reward 55540
set wrist_reward 55543
* name variables
set feet_name slippers
set head_name circlet
set hands_name fistwraps
set arms_name sleeves
set legs_name leggings
set body_name tunic
set wrist_name guard
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55733
Phase Armor - Init - Rog/Mer/Ass/Thi Phase 1~
0 o 100
~
* global variables
set classes Rogue, Mercenary, Assassin, Thief, and Bard
set phase 1
* gem variables
set feet_gem 55572
set head_gem 55580
set hands_gem 55568
set arms_gem 55584
set legs_gem 55588
set body_gem 55592
set wrist_gem 55576
* armor variables
set feet_armor 55305
set head_armor 55313
set hands_armor 55301
set arms_armor 55317
set legs_armor 55321
set body_armor 55325
set wrist_armor 55309
* reward variables
set feet_reward 55409
set head_reward 55405
set hands_reward 55411
set arms_reward 55406
set legs_reward 55408
set body_reward 55407
set wrist_reward 55410
* name variables
set feet_name boots
set head_name coif
set hands_name gloves
set arms_name sleeves
set legs_name leggings
set body_name tunic
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55734
Phase Armor - Init - Rog/Mer/Ass/Thi Phase 2~
0 o 100
~
* global variables
set classes Rogue, Mercenary, Assassin, and Thief
set phase 2
* gem variables
set feet_gem 55614
set head_gem 55636
set hands_gem 55603
set arms_gem 55647
set legs_gem 55658
set body_gem 55669
set wrist_gem 55625
* armor variables
set feet_armor 55333
set head_armor 55341
set hands_armor 55329
set arms_armor 55345
set legs_armor 55349
set body_armor 55353
set wrist_armor 55337
* reward variables
set feet_reward 55486
set head_reward 55482
set hands_reward 55488
set arms_reward 55483
set legs_reward 55485
set body_reward 55484
set wrist_reward 55487
* name variables
set feet_name boots
set head_name helm
set hands_name gloves
set arms_name sleeves
set legs_name leggings
set body_name tunic
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55735
Phase Armor - Init - Rog/Mer/Ass/Thi Phase 3~
0 o 100
~
* global variables
set classes Rogue, Mercenary, Assassin, and Thief
set phase 3
* gem variables
set feet_gem 55691
set head_gem 55713
set hands_gem 55680
set arms_gem 55724
set legs_gem 55735
set body_gem 55746
set wrist_gem 55702
* armor variables
set feet_armor 55361
set head_armor 55369
set hands_armor 55357
set arms_armor 55373
set legs_armor 55377
set body_armor 55381
set wrist_armor 55365
* reward variables
set feet_reward 55563
set head_reward 55559
set hands_reward 55565
set arms_reward 55560
set legs_reward 55562
set body_reward 55561
set wrist_reward 55564
* name variables
set feet_name boots
set head_name coif
set hands_name gloves
set arms_name sleeves
set legs_name greaves
set body_name tunic
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55736
Phase Armor - Init - Ber Phase 2~
0 o 100
~
* global variables
set classes Berserker
set phase 2
* gem variables
set feet_gem 55607
set head_gem 55631
set hands_gem 55595
set arms_gem 55638
set legs_gem 55655
set body_gem 55665
set wrist_gem 55619
* armor variables
set feet_armor 55334
set head_armor 55342
set hands_armor 55330
set arms_armor 55346
set legs_armor 55350
set body_armor 55354
set wrist_armor 55338
* reward variables
set feet_reward 55771
set head_reward 55767
set hands_reward 55773
set arms_reward 55768
set legs_reward 55770
set body_reward 55769
set wrist_reward 55772
* name variables
set feet_name boots
set head_name helm
set hands_name gloves
set arms_name sleeves
set legs_name pants
set body_name tunic
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55737
Phase Armor - Init - Ber Phase 3~
0 o 100
~
* global variables
set classes Berserker
set phase 3
* gem variables
set feet_gem 55686
set head_gem 55710
set hands_gem 55672
set arms_gem 55717
set legs_gem 55726
set body_gem 55742
set wrist_gem 55696
* armor variables
set feet_armor 55362
set head_armor 55370
set hands_armor 55358
set arms_armor 55374
set legs_armor 55378
set body_armor 55382
set wrist_armor 55366
* reward variables
set feet_reward 55778
set head_reward 55774
set hands_reward 55780
set arms_reward 55775
set legs_reward 55777
set body_reward 55776
set wrist_reward 55779
* name variables
set feet_name boots
set head_name helm
set hands_name gloves
set arms_name sleeves
set legs_name pants
set body_name tunic
set wrist_name bracer
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55738
Phase Armor - Init - Bar Phase 2  ~
0 o 100
~
* global variables
set classes Bard
set phase 2
* gem variables
set feet_gem 55606
set head_gem 55630
set hands_gem 55600
set arms_gem 55640
set legs_gem 55649
set body_gem 55664
set wrist_gem 55621
* armor variables
set feet_armor 55334
set head_armor 55342
set hands_armor 55330
set arms_armor 55346
set legs_armor 55350
set body_armor 55354
set wrist_armor 55338
* reward variables
set feet_reward 55785
set head_reward 55781
set hands_reward 55787
set arms_reward 55782
set legs_reward 55784
set body_reward 55783
set wrist_reward 55786
* name variables
set feet_name boots
set head_name cap
set hands_name gloves
set arms_name sleeves
set legs_name leggings
set body_name tunic
set wrist_name bracelet
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
#55739
Phase Armor - Init - Bar Phase 3~
0 o 100
~
* global variables
set classes Bard
set phase 3
* gem variables
set feet_gem 55685
set head_gem 55708
set hands_gem 55671
set arms_gem 55720
set legs_gem 55728
set body_gem 55743
set wrist_gem 55694
* armor variables
set feet_armor 55362
set head_armor 55370
set hands_armor 55358
set arms_armor 55374
set legs_armor 55378
set body_armor 55382
set wrist_armor 55366
* reward variables
set feet_reward 55792
set head_reward 55788
set hands_reward 55794
set arms_reward 55789
set legs_reward 55791
set body_reward 55790
set wrist_reward 55793
* name variables
set feet_name boots
set head_name crown
set hands_name gloves
set arms_name sleeves
set legs_name pants
set body_name doublet
set wrist_name bracelet
* globalify everything
global classes phase
global feet_gem head_gem hands_gem arms_gem legs_gem body_gem wrist_gem
global feet_armor head_armor hands_armor arms_armor legs_armor body_armor wrist_armor
global feet_reward head_reward hands_reward arms_reward legs_reward body_reward wrist_reward
global feet_name head_name hands_name arms_name legs_name body_name wrist_name
~
$~
