#55600
drop_phase_3_boss_55~
0 f 100
~
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* boss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 4 pieces of armor per sub_phase in phase_3
set what_armor_drop %random.4%
* 11 classes questing in phase_3
set what_gem_drop %random.11%
* 
if %will_drop% <= 20
   * drop nothing and bail
   halt
endif
if %will_drop% <= 60 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55714
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55725
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55736
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      *
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55714
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55725
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55736
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55601
drop_phase_3_mini_boss_58~
0 f 100
~
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 4 pieces of armor per sub_phase in phase_3
set what_armor_drop %random.4%
* 11 classes questing in phase_3
set what_gem_drop %random.11%
* 
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55725
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55736
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55736
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55725
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55736
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55736
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55602
drop_phase_3_boss_58~
0 f 100
~
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* boss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 4 pieces of armor per sub_phase in phase_3
set what_armor_drop %random.4%
* 11 classes questing in phase_3
set what_gem_drop %random.11%
* 
if %will_drop% <= 20
   * drop nothing and bail
   halt
endif
if %will_drop% <= 60 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55725
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 & %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55736
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55736
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 & %will_drop% <= 80)
   * Normal non-bonus drops from previous wear position
   if %bonus% <= 50
      *
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 & %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55725
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 & %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55736
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55736
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55603
phase_1_where~
0 n 100
where~
msend %actor% %self.name% tells you, 'You can find the items I seek by playing and'
msend %actor% %self.name% tells you, 'adventuring in areas appropriate to your level.'
msend %actor% %self.name% tells you, 'My quests are for those from level 1 to 20, so'
msend %actor% %self.name% tells you, 'the items I desire can be found in areas of that'
msend %actor% %self.name% tells you, 'difficulty.  Once you have agreed to take on my'
msend %actor% %self.name% tells you, 'quests you can seek another like me for tasks a'
msend %actor% %self.name% tells you, 'bit more difficult.'
~
#55604
phase_2_where~
0 n 100
where~
msend %actor% %self.name% tells you, 'You can find the items I seek by playing and'
msend %actor% %self.name% tells you, 'adventuring in areas appropriate to your level.'
msend %actor% %self.name% tells you, 'My quests are for those from level 21 to 40, so'
msend %actor% %self.name% tells you, 'the items I desire can be found in areas of that'
msend %actor% %self.name% tells you, 'difficulty.  Once you have agreed to take on my'
msend %actor% %self.name% tells you, 'quests you can seek another like me for tasks a'
msend %actor% %self.name% tells you, 'bit more difficult.'
~
#55605
phase_3_where~
0 n 100
where~
msend %actor% %self.name% tells you, 'You can find the items I seek by playing and'
msend %actor% %self.name% tells you, 'adventuring in areas appropriate to your level.'
msend %actor% %self.name% tells you, 'My quests are for those from level 41 to 60, so'
msend %actor% %self.name% tells you, 'the items I desire can be found in areas of that'
msend %actor% %self.name% tells you, 'difficulty.'
~
#55606
phase_1_warrior_status~
0 n 0
status~
set anti Anti-Paladin
eval right_class (%actor.class% == Warrior) || (%actor.class% == %anti%) || (%actor.class% == Paladin) || (%actor.class% == Ranger) || (%actor.class% == Monk)
set min_level 1
set quest_stage 1
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * armors for this class
      set vnum_destroyed_gloves 55300
      set vnum_destroyed_boots  55304
      set vnum_destroyed_bracer 55308
      set vnum_destroyed_helm   55312
      set vnum_destroyed_arm    55316
      set vnum_destroyed_legs   55320
      set vnum_destroyed_chest  55324
      * gems for this class
      set vnum_gem_gloves       55569
      set vnum_gem_boots        55573
      set vnum_gem_bracer       55577
      set vnum_gem_helm         55581
      set vnum_gem_arm          55585
      set vnum_gem_legs         55589
      set vnum_gem_chest        55593
      * rewards for this class
      set vnum_reward_helm      55384
      set vnum_reward_arms      55385
      set vnum_reward_chest     55386
      set vnum_reward_legs      55387
      set vnum_reward_boots     55388
      set vnum_reward_bracer    55389
      set vnum_reward_gloves    55390
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55607
phase_1_sorcerer_status~
0 n 0
status~
eval right_class (%actor.class% /= Sorcerer) || (%actor.class% /= Necromancer) || (%actor.class% /= Cryomancer) || (%actor.class% /= Pyromancer) || (%actor.class% /= Conjurer)
set min_level 1
set quest_stage 1
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * armors for this class
      set vnum_destroyed_gloves 55302
      set vnum_destroyed_boots  55306
      set vnum_destroyed_bracer 55310
      set vnum_destroyed_helm   55314
      set vnum_destroyed_arm    55318
      set vnum_destroyed_legs   55322
      set vnum_destroyed_chest  55326
      * gems for this class
      set vnum_gem_gloves       55567
      set vnum_gem_boots        55571
      set vnum_gem_bracer       55575
      set vnum_gem_helm 55579
      set vnum_gem_arm  55583
      set vnum_gem_legs 55587
      set vnum_gem_chest        55591
      * rewards for this class
      set vnum_reward_helm 55398
      set vnum_reward_arms 55399
      set vnum_reward_chest 55400
      set vnum_reward_legs 55401
      set vnum_reward_boots 55402
      set vnum_reward_bracer 55403
      set vnum_reward_gloves 55404
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55608
phase_1_rogue_status~
0 n 0
status~
eval right_class (%actor.class% /= Rogue) || (%actor.class% /= Assassin) || (%actor.class% /= Thief) || (%actor.class% /= Mercenary)
set min_level 1
set quest_stage 1
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55301
      set vnum_destroyed_chest 55325
      set vnum_destroyed_boots 55305
      set vnum_destroyed_bracer 55309
      set vnum_destroyed_helm 55313
      set vnum_destroyed_arm 55317
      set vnum_destroyed_legs 55321
      * gems for this class
      set vnum_gem_gloves 55568
      set vnum_gem_boots 55572
      set vnum_gem_bracer 55576
      set vnum_gem_helm 55580
      set vnum_gem_arm 55584
      set vnum_gem_legs 55588
      set vnum_gem_chest 55592
      * rewards for this class
      set vnum_reward_helm 55405
      set vnum_reward_arms 55406
      set vnum_reward_chest 55407
      set vnum_reward_legs 55408
      set vnum_reward_boots 55409
      set vnum_reward_bracer 55410
      set vnum_reward_gloves 55411
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55609
phase_1_cleric_status~
0 n 0
status~
eval right_class (%actor.class% /= Cleric) || (%actor.class% /= Priest) || (%actor.class% /= Diabolist) || (%actor.class% /= Druid)
set min_level 1
set quest_stage 1
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55300
      set vnum_destroyed_boots 55304
      set vnum_destroyed_bracer 55308
      set vnum_destroyed_helm 55312
      set vnum_destroyed_arm 55316
      set vnum_destroyed_legs 55320
      set vnum_destroyed_chest 55324
      * gems for this class
      set vnum_gem_gloves 55566
      set vnum_gem_boots 55570
      set vnum_gem_bracer 55574
      set vnum_gem_helm 55578
      set vnum_gem_arm 55582
      set vnum_gem_legs 55586
      set vnum_gem_chest 55590
      * rewards for this class
      set vnum_reward_helm 55391
      set vnum_reward_arms 55392
      set vnum_reward_chest 55393
      set vnum_reward_legs 55394
      set vnum_reward_boots 55395
      set vnum_reward_bracer 55396
      set vnum_reward_gloves 55397
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55610
phase_2_cleric_status~
0 n 0
status~
eval right_class (%actor.class% /= Cleric) || (%actor.class% /= Priest)
set min_level 21
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55328
      set vnum_destroyed_boots  55332
      set vnum_destroyed_bracer 55336
      set vnum_destroyed_helm   55340
      set vnum_destroyed_arm    55344
      set vnum_destroyed_legs   55348
      set vnum_destroyed_chest  55352
      * gems for this class
      set vnum_gem_gloves       55601
      set vnum_gem_boots        55612
      set vnum_gem_bracer       55623
      set vnum_gem_helm 55634
      set vnum_gem_arm  55645
      set vnum_gem_legs 55656
      set vnum_gem_chest        55667
      * rewards for this class
      set vnum_reward_helm 55433
      set vnum_reward_arms 55434
      set vnum_reward_chest 55435
      set vnum_reward_legs 55436
      set vnum_reward_boots 55437
      set vnum_reward_bracer 55438
      set vnum_reward_gloves 55439
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55611
phase_2_druid_status~
0 n 0
status~
eval right_class %actor.class% /= Druid
set min_level 21
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55330
      set vnum_destroyed_boots  55334
      set vnum_destroyed_bracer 55338
      set vnum_destroyed_helm   55342
      set vnum_destroyed_arm    55346
      set vnum_destroyed_legs   55350
      set vnum_destroyed_chest  55354
      * gems for this class
      set vnum_gem_gloves       55596
      set vnum_gem_boots        55607
      set vnum_gem_bracer       55618
      set vnum_gem_helm 55629
      set vnum_gem_arm  55640
      set vnum_gem_legs 55651
      set vnum_gem_chest        55662
      * rewards for this class
      set vnum_reward_helm 55447
      set vnum_reward_arms 55448
      set vnum_reward_chest 55449
      set vnum_reward_legs 55450
      set vnum_reward_boots 55451
      set vnum_reward_bracer 55452
      set vnum_reward_gloves 55453
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55612
phase_2_sorcerer_status~
0 n 0
status~
eval right_class %actor.class% == Sorcerer || %actor.class% == Cryomancer || %actor.class% == Pyromancer
set min_level 21
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55331
      set vnum_destroyed_boots  55335
      set vnum_destroyed_bracer 55338
      set vnum_destroyed_helm   55343
      set vnum_destroyed_arm    55347
      set vnum_destroyed_legs   55351
      set vnum_destroyed_chest  55355
      * gems for this class
      set vnum_gem_gloves       55602
      set vnum_gem_boots        55613
      set vnum_gem_bracer       55624
      set vnum_gem_helm 55631
      set vnum_gem_arm  55646
      set vnum_gem_legs 55657
      set vnum_gem_chest        55668
      * rewards for this class
      set vnum_reward_helm 55475
      set vnum_reward_arms 55476
      set vnum_reward_chest 55477
      set vnum_reward_legs 55478
      set vnum_reward_boots 55479
      set vnum_reward_bracer 55480
      set vnum_reward_gloves 55481
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55613
phase_2_paladin_status~
0 n 0
status~
eval right_class %actor.class% == Paladin
set min_level 21
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55328
      set vnum_destroyed_boots  55332
      set vnum_destroyed_bracer 55336
      set vnum_destroyed_helm   55340
      set vnum_destroyed_arm    55344
      set vnum_destroyed_legs   55348
      set vnum_destroyed_chest  55352
      * gems for this class
      set vnum_gem_gloves       55599
      set vnum_gem_boots        55610
      set vnum_gem_bracer       55621
      set vnum_gem_helm 55632
      set vnum_gem_arm  55643
      set vnum_gem_legs 55654
      set vnum_gem_chest        55665
      * rewards for this class
      set vnum_reward_helm 55419
      set vnum_reward_arms 55420
      set vnum_reward_chest 55421
      set vnum_reward_legs 55422
      set vnum_reward_boots 55423
      set vnum_reward_bracer 55424
      set vnum_reward_gloves 55425
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55614
phase_2_monk_status~
0 n 0
status~
eval right_class %actor.class% == Monk
set min_level 21
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55330
      set vnum_destroyed_boots  55334
      set vnum_destroyed_bracer 55338
      set vnum_destroyed_helm   55342
      set vnum_destroyed_arm    55346
      set vnum_destroyed_legs   55350
      set vnum_destroyed_chest  55354
      * gems for this class
      set vnum_gem_gloves       55597
      set vnum_gem_boots        55608
      set vnum_gem_bracer       55619
      set vnum_gem_helm 55630
      set vnum_gem_arm  55641
      set vnum_gem_legs 55652
      set vnum_gem_chest        55663
      * rewards for this class
      set vnum_reward_helm 55461
      set vnum_reward_arms 55462
      set vnum_reward_chest 55463
      set vnum_reward_legs 55464
      set vnum_reward_boots 55465
      set vnum_reward_bracer 55466
      set vnum_reward_gloves 55467
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55615
phase_2_necromancer_status~
0 n 0
status~
eval right_class %actor.class% == Necromancer
set min_level 21
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55331
      set vnum_destroyed_boots  55335
      set vnum_destroyed_bracer 55339
      set vnum_destroyed_helm   55343
      set vnum_destroyed_arm    55347
      set vnum_destroyed_legs   55351
      set vnum_destroyed_chest  55355
      * gems for this class
      set vnum_gem_gloves       55598
      set vnum_gem_boots        55609
      set vnum_gem_bracer       55620
      set vnum_gem_helm 55631
      set vnum_gem_arm  55642
      set vnum_gem_legs 55653
      set vnum_gem_chest        55664
      * rewards for this class
      set vnum_reward_helm 55454
      set vnum_reward_arms 55455
      set vnum_reward_chest 55456
      set vnum_reward_legs 55457
      set vnum_reward_boots 55458
      set vnum_reward_bracer 55459
      set vnum_reward_gloves 55460
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55616
phase_2_ranger_status~
0 n 0
status~
eval right_class %actor.class% == Ranger
set min_level 21
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55329
      set vnum_destroyed_boots  55333
      set vnum_destroyed_bracer 55337
      set vnum_destroyed_helm   55341
      set vnum_destroyed_arm    55345
      set vnum_destroyed_legs   55349
      set vnum_destroyed_chest  55353
      * gems for this class
      set vnum_gem_gloves       55600
      set vnum_gem_boots        55611
      set vnum_gem_bracer       55622
      set vnum_gem_helm 55633
      set vnum_gem_arm  55644
      set vnum_gem_legs 55655
      set vnum_gem_chest        55666
      * rewards for this class
      set vnum_reward_helm 55440
      set vnum_reward_arms 55441
      set vnum_reward_chest 55442
      set vnum_reward_legs 55443
      set vnum_reward_boots 55444
      set vnum_reward_bracer 55445
      set vnum_reward_gloves 55446
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55617
phase_3_anti-paladin_status~
0 n 0
status~
* For some stoooopid reason, the /= operator doesn't want to work in evals
set min_level 41
set quest_stage 2
if %actor.class% /= Anti
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55356
      set vnum_destroyed_boots  55360
      set vnum_destroyed_bracer 55364
      set vnum_destroyed_helm   55368
      set vnum_destroyed_arm    55372
      set vnum_destroyed_legs   55376
      set vnum_destroyed_chest  55380
      * gems for this class
      set vnum_gem_gloves       55671
      set vnum_gem_boots        55682
      set vnum_gem_bracer       55693
      set vnum_gem_helm 55704
      set vnum_gem_arm  55715
      set vnum_gem_legs 55726
      set vnum_gem_chest        55737
      * rewards for this class
      set vnum_reward_helm 55503
      set vnum_reward_arms 55504
      set vnum_reward_chest 55505
      set vnum_reward_legs 55506
      set vnum_reward_boots 55507
      set vnum_reward_bracer 55508
      set vnum_reward_gloves 55509
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55618
phase_3_monk_status~
0 n 0
status~
eval right_class %actor.class% == Monk
set min_level 41
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55358
      set vnum_destroyed_boots  55362
      set vnum_destroyed_bracer 55366
      set vnum_destroyed_helm   55370
      set vnum_destroyed_arm    55374
      set vnum_destroyed_legs   55378
      set vnum_destroyed_chest  55382
      * gems for this class
      set vnum_gem_gloves       55674
      set vnum_gem_boots        55685
      set vnum_gem_bracer       55696
      set vnum_gem_helm 55707
      set vnum_gem_arm  55718
      set vnum_gem_legs 55729
      set vnum_gem_chest        55740
      * rewards for this class
      set vnum_reward_helm 55538
      set vnum_reward_arms 55539
      set vnum_reward_chest 55540
      set vnum_reward_legs 55541
      set vnum_reward_boots 55542
      set vnum_reward_bracer 55543
      set vnum_reward_gloves 55544
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55619
phase_3_druid_status~
0 n 0
status~
eval right_class %actor.class% == Druid
set min_level 41
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55358
      set vnum_destroyed_boots  55362
      set vnum_destroyed_bracer 55366
      set vnum_destroyed_helm   55370
      set vnum_destroyed_arm    55374
      set vnum_destroyed_legs   55378
      set vnum_destroyed_chest  55382
      * gems for this class
      set vnum_gem_gloves       55673
      set vnum_gem_boots        55684
      set vnum_gem_bracer       55695
      set vnum_gem_helm 55706
      set vnum_gem_arm  55717
      set vnum_gem_legs 55728
      set vnum_gem_chest        55739
      * rewards for this class
      set vnum_reward_helm 55524
      set vnum_reward_arms 55525
      set vnum_reward_chest 55526
      set vnum_reward_legs 55527
      set vnum_reward_boots 55528
      set vnum_reward_bracer 55529
      set vnum_reward_gloves 55530
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55620
phase_3_paladin_status~
0 n 0
status~
eval right_class %actor.class% == Paladin
set min_level 41
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55356
      set vnum_destroyed_boots  55360
      set vnum_destroyed_bracer 55364
      set vnum_destroyed_helm   55368
      set vnum_destroyed_arm    55372
      set vnum_destroyed_legs   55376
      set vnum_destroyed_chest  55380
      * gems for this class
      set vnum_gem_gloves       55676
      set vnum_gem_boots        55687
      set vnum_gem_bracer       55698
      set vnum_gem_helm 55709
      set vnum_gem_arm  55720
      set vnum_gem_legs 55731
      set vnum_gem_chest        55742
      * rewards for this class
      set vnum_reward_helm 55496
      set vnum_reward_arms 55497
      set vnum_reward_chest 55498
      set vnum_reward_legs 55499
      set vnum_reward_boots 55500
      set vnum_reward_bracer 55501
      set vnum_reward_gloves 55502
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55621
phase_2_warrior_status~
0 n 0
status~
eval right_class %actor.class% == Warrior
set min_level 21
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55328
      set vnum_destroyed_boots  55332
      set vnum_destroyed_bracer 55336
      set vnum_destroyed_helm   55340
      set vnum_destroyed_arm    55344
      set vnum_destroyed_legs   55348
      set vnum_destroyed_chest  55352
      * gems for this class
      set vnum_gem_gloves       55604
      set vnum_gem_boots        55615
      set vnum_gem_bracer       55626
      set vnum_gem_helm 55637
      set vnum_gem_arm  55648
      set vnum_gem_legs 55659
      set vnum_gem_chest        55670
      * rewards for this class
      set vnum_reward_helm 55412
      set vnum_reward_arms 55413
      set vnum_reward_chest 55414
      set vnum_reward_legs 55415
      set vnum_reward_boots 55416
      set vnum_reward_bracer 55417
      set vnum_reward_gloves 55418
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55622
phase_2_anti-paladin_status~
0 n 0
status~
set min_level 21
set quest_stage 2
if %actor.class% /= Anti
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55328
      set vnum_destroyed_boots  55332
      set vnum_destroyed_bracer 55336
      set vnum_destroyed_helm   55340
      set vnum_destroyed_arm    55344
      set vnum_destroyed_legs   55348
      set vnum_destroyed_chest  55352
      * gems for this class
      set vnum_gem_gloves       55594
      set vnum_gem_boots        55605
      set vnum_gem_bracer       55616
      set vnum_gem_helm 55627
      set vnum_gem_arm  55638
      set vnum_gem_legs 55649
      set vnum_gem_chest        55660
      * rewards for this class
      set vnum_reward_helm 55426
      set vnum_reward_arms 55427
      set vnum_reward_chest 55428
      set vnum_reward_legs 55429
      set vnum_reward_boots 55430
      set vnum_reward_bracer 55431
      set vnum_reward_gloves 55432
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55623
phase_2_diabolist_status~
0 n 100
status~
eval right_class %actor.class% == Diabolist
set min_level 21
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55328
      set vnum_destroyed_boots  55332
      set vnum_destroyed_bracer 55336
      set vnum_destroyed_helm   55340
      set vnum_destroyed_arm    55344
      set vnum_destroyed_legs   55348
      set vnum_destroyed_chest  55352
      * gems for this class
      set vnum_gem_gloves       55595
      set vnum_gem_boots        55606
      set vnum_gem_bracer       55617
      set vnum_gem_helm 55628
      set vnum_gem_arm  55639
      set vnum_gem_legs 55650
      set vnum_gem_chest        55661
      * rewards for this class
      set vnum_reward_helm 55468
      set vnum_reward_arms 55469
      set vnum_reward_chest 55470
      set vnum_reward_legs 55471
      set vnum_reward_boots 55472
      set vnum_reward_bracer 55473
      set vnum_reward_gloves 55474
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55624
phase_2_rogue_status~
0 n 0
status~
eval right_class %actor.class% == Rogue || %actor.class% == Assassin || %actor.class% == Mercenary || %actor.class% == Thief
set min_level 21
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55329
      set vnum_destroyed_boots  55333
      set vnum_destroyed_bracer 55337
      set vnum_destroyed_helm   55341
      set vnum_destroyed_arm    55345
      set vnum_destroyed_legs   55349
      set vnum_destroyed_chest  55353
      * gems for this class
      set vnum_gem_gloves       55603
      set vnum_gem_boots        55614
      set vnum_gem_bracer       55625
      set vnum_gem_helm 55636
      set vnum_gem_arm  55647
      set vnum_gem_legs 55658
      set vnum_gem_chest        55669
      * rewards for this class
      set vnum_reward_helm 55482
      set vnum_reward_arms 55483
      set vnum_reward_chest 55484
      set vnum_reward_legs 55485
      set vnum_reward_boots 55486
      set vnum_reward_bracer 55487
      set vnum_reward_gloves 55488
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55625
phase_3_warrior_status~
0 n 0
status~
eval right_class %actor.class% == Warrior
set min_level 41
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55356
      set vnum_destroyed_boots  55360
      set vnum_destroyed_bracer 55364
      set vnum_destroyed_helm   55368
      set vnum_destroyed_arm    55372
      set vnum_destroyed_legs   55376
      set vnum_destroyed_chest  55380
      * gems for this class
      set vnum_gem_gloves       55681
      set vnum_gem_boots        55692
      set vnum_gem_bracer       55703
      set vnum_gem_helm 55714
      set vnum_gem_arm  55725
      set vnum_gem_legs 55736
      set vnum_gem_chest        55747
      * rewards for this class
      set vnum_reward_helm 55489
      set vnum_reward_arms 55490
      set vnum_reward_chest 55491
      set vnum_reward_legs 55492
      set vnum_reward_boots 55493
      set vnum_reward_bracer 55494
      set vnum_reward_gloves 55495
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55626
phase_3_ranger_status~
0 n 0
status~
eval right_class %actor.class% == Ranger
set min_level 41
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55357
      set vnum_destroyed_boots  55361
      set vnum_destroyed_bracer 55365
      set vnum_destroyed_helm   55369
      set vnum_destroyed_arm    55373
      set vnum_destroyed_legs   55377
      set vnum_destroyed_chest  55381
      * gems for this class
      set vnum_gem_gloves       55677
      set vnum_gem_boots        55688
      set vnum_gem_bracer       55699
      set vnum_gem_helm 55710
      set vnum_gem_arm  55721
      set vnum_gem_legs 55732
      set vnum_gem_chest        55743
      * rewards for this class
      set vnum_reward_helm 55517
      set vnum_reward_arms 55518
      set vnum_reward_chest 55519
      set vnum_reward_legs 55520
      set vnum_reward_boots 55521
      set vnum_reward_bracer 55522
      set vnum_reward_gloves 55523
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55627
**UNUSED**~
0 n 0
status~
eval right_class %actor.class% == Cleric || %actor.class% == Priest
set min_level 41
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55356
      set vnum_destroyed_boots  55360
      set vnum_destroyed_bracer 55364
      set vnum_destroyed_helm   55368
      set vnum_destroyed_arm    55372
      set vnum_destroyed_legs   55376
      set vnum_destroyed_chest  55380
      * gems for this class
      set vnum_gem_gloves       55678
      set vnum_gem_boots        55689
      set vnum_gem_bracer       55700
      set vnum_gem_helm 55711
      set vnum_gem_arm  55722
      set vnum_gem_legs 55733
      set vnum_gem_chest        55744
      * rewards for this class
      set vnum_reward_helm 55510
      set vnum_reward_arms 55511
      set vnum_reward_chest 55512
      set vnum_reward_legs 55513
      set vnum_reward_boots 55514
      set vnum_reward_bracer 55515
      set vnum_reward_gloves 55516
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55628
phase_3_necromancer_status~
0 n 0
status~
eval right_class %actor.class% == Necromancer
set min_level 41
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55359
      set vnum_destroyed_boots  55363
      set vnum_destroyed_bracer 55367
      set vnum_destroyed_helm   55371
      set vnum_destroyed_arm    55375
      set vnum_destroyed_legs   55379
      set vnum_destroyed_chest  55383
      * gems for this class
      set vnum_gem_gloves       55675
      set vnum_gem_boots        55686
      set vnum_gem_bracer       55697
      set vnum_gem_helm 55708
      set vnum_gem_arm  55719
      set vnum_gem_legs 55730
      set vnum_gem_chest        55741
      * rewards for this class
      set vnum_reward_helm 55531
      set vnum_reward_arms 55532
      set vnum_reward_chest 55533
      set vnum_reward_legs 55534
      set vnum_reward_boots 55535
      set vnum_reward_bracer 55536
      set vnum_reward_gloves 55537
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55629
phase_3_sorcerer_status~
0 n 0
status~
eval right_class %actor.class% == Sorcerer || %actor.class% == Pyromancer || %actor.class% == Cryomancer
set min_level 41
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55359
      set vnum_destroyed_boots 55363
      set vnum_destroyed_bracer 55367
      set vnum_destroyed_helm 55371
      set vnum_destroyed_arm 55375
      set vnum_destroyed_legs 55379
      set vnum_destroyed_chest 55383
      * gems for this class
      set vnum_gem_gloves 55679
      set vnum_gem_boots 55690
      set vnum_gem_bracer 55701
      set vnum_gem_helm 55712
      set vnum_gem_arm 55723
      set vnum_gem_legs 55734
      set vnum_gem_chest 55745
      * rewards for this class
      set vnum_reward_helm 55552
      set vnum_reward_arms 55553
      set vnum_reward_chest 55554
      set vnum_reward_legs 55555
      set vnum_reward_boots 55556
      set vnum_reward_bracer 55557
      set vnum_reward_gloves 55558
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55630
phase_3_diabolist_status~
0 n 0
status~
eval right_class %actor.class% == Diabolist
set min_level 41
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55356
      set vnum_destroyed_boots  55360
      set vnum_destroyed_bracer 55364
      set vnum_destroyed_helm   55368
      set vnum_destroyed_arm    55372
      set vnum_destroyed_legs   55376
      set vnum_destroyed_chest  55380
      * gems for this class
      set vnum_gem_gloves       55672
      set vnum_gem_boots        55683
      set vnum_gem_bracer       55694
      set vnum_gem_helm 55705
      set vnum_gem_arm  55716
      set vnum_gem_legs 55727
      set vnum_gem_chest        55738
      * rewards for this class
      set vnum_reward_helm 55545
      set vnum_reward_arms 55546
      set vnum_reward_chest 55547
      set vnum_reward_legs 55548
      set vnum_reward_boots 55549
      set vnum_reward_bracer 55550
      set vnum_reward_gloves 55551
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55631
phase_3_rogue_status~
0 n 0
status~
eval right_class %actor.class% == Rogue || %actor.class% == Assassin || %actor.class% == Mercenary || %actor.class% == Thief
set min_level 41
set quest_stage 2
if %right_class%
   if %actor.level% < %min_level%
      msend %actor% %self.name% tells you, 'You need to gain more experience first.'
   elseif %actor.quest_stage[phase_armor]% >= %quest_stage%
      * destroyed armor
      set vnum_destroyed_gloves 55357
      set vnum_destroyed_boots 55361
      set vnum_destroyed_bracer 55365
      set vnum_destroyed_helm 55369
      set vnum_destroyed_arm 55373
      set vnum_destroyed_legs 55377
      set vnum_destroyed_chest 55381
      * gems for this class
      set vnum_gem_gloves 55680
      set vnum_gem_boots 55691
      set vnum_gem_bracer 55702
      set vnum_gem_helm 55713
      set vnum_gem_arm 55724
      set vnum_gem_legs 55735
      set vnum_gem_chest 55746
      * rewards for this class
      set vnum_reward_helm 55559
      set vnum_reward_arms 55560
      set vnum_reward_chest 55561
      set vnum_reward_legs 55562
      set vnum_reward_boots 55563
      set vnum_reward_bracer 55564
      set vnum_reward_gloves 55565
      set gloves_armor %actor.quest_variable[phase_armor:%vnum_destroyed_gloves%_armor_acquired]%
      set boots_armor %actor.quest_variable[phase_armor:%vnum_destroyed_boots%_armor_acquired]%
      set bracer_armor %actor.quest_variable[phase_armor:%vnum_destroyed_bracer%_armor_acquired]%
      set helm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_helm%_armor_acquired]%
      set arm_armor %actor.quest_variable[phase_armor:%vnum_destroyed_arm%_armor_acquired]%
      set legs_armor %actor.quest_variable[phase_armor:%vnum_destroyed_legs%_armor_acquired]%
      set chest_armor %actor.quest_variable[phase_armor:%vnum_destroyed_chest%_armor_acquired]%
      set gloves_gems %actor.quest_variable[phase_armor:%vnum_gem_gloves%_gems_acquired]%
      set boots_gems %actor.quest_variable[phase_armor:%vnum_gem_boots%_gems_acquired]%
      set bracer_gems %actor.quest_variable[phase_armor:%vnum_gem_bracer%_gems_acquired]%
      set helm_gems %actor.quest_variable[phase_armor:%vnum_gem_helm%_gems_acquired]%
      set arm_gems %actor.quest_variable[phase_armor:%vnum_gem_arm%_gems_acquired]%
      set legs_gems %actor.quest_variable[phase_armor:%vnum_gem_legs%_gems_acquired]%
      set chest_gems %actor.quest_variable[phase_armor:%vnum_gem_chest%_gems_acquired]%
      eval done_gloves %gloves_armor% == 1 && %gloves_gems% == 3
      eval done_boots %boots_armor% == 1 && %boots_gems% == 3
      eval done_bracer %bracer_armor% == 1 && %bracer_gems% == 3
      eval done_helm %helm_armor% == 1 && %helm_gems% == 3
      eval done_arm %arm_armor% == 1 && %arm_gems% == 3
      eval done_legs %legs_armor% == 1 && %legs_gems% == 3
      eval done_chest %chest_armor% == 1 && %chest_gems% == 3
      eval given %gloves_armor% + %boots_armor% + %bracer_armor% + %helm_armor% + %arm_armor% + %legs_armor% + %chest_armor%
      eval given %given% + %gloves_gems% + %boots_gems% + %bracer_gems% + %helm_gems% + %arm_gems% + %legs_gems% + %chest_gems%
      if %given%
         msend %actor% %self.name% tells you, 'You have given me:'
      else
         msend %actor% %self.name% tells you, 'You haven't given me anything yet.'
         halt
      endif
      if %gloves_armor% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_gloves%]%'
      endif
      if %gloves_gems% && !%done_gloves%
         msend %actor% %self.name% tells you, '  %gloves_gems% of %get.obj_shortdesc[%vnum_gem_gloves%]%'
      endif
      if %boots_armor% && !%done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_boots%]%'
      endif
      if %boots_gems% && !%done_boots%
         msend %actor% %self.name% tells you, '  %boots_gems% of %get.obj_shortdesc[%vnum_gem_boots%]%'
      endif
      if %bracer_armor% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_bracer%]%'
      endif
      if %bracer_gems% && !%done_bracer%
         msend %actor% %self.name% tells you, '  %bracer_gems% of %get.obj_shortdesc[%vnum_gem_bracer%]%'
      endif
      if %helm_armor% && !%done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_helm%]%'
      endif
      if %helm_gems% && !%done_helm%
         msend %actor% %self.name% tells you, '  %helm_gems% of %get.obj_shortdesc[%vnum_gem_helm%]%'
      endif
      if %arm_armor% && !%done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_arm%]%'
      endif
      if %arm_gems% && !%done_arm%
         msend %actor% %self.name% tells you, '  %arm_gems% of %get.obj_shortdesc[%vnum_gem_arm%]%'
      endif
      if %legs_armor% && !%done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_legs%]%'
      endif
      if %legs_gems% && !%done_legs%
         msend %actor% %self.name% tells you, '  %legs_gems% of %get.obj_shortdesc[%vnum_gem_legs%]%'
      endif
      if %chest_armor% && !%done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_destroyed_chest%]%'
      endif
      if %chest_gems% && !%done_chest%
         msend %actor% %self.name% tells you, '  %chest_gems% of %get.obj_shortdesc[%vnum_gem_chest%]%'
      endif
      if %done_gloves% || %done_boots% || %done_bracer% || %done_helm% || %done_arm% || %done_legs% || %done_chest%
         msend %actor% %self.name% tells you, 'You have completed quests for:'
      endif
      if %done_gloves%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_gloves%]%'
      endif
      if %done_boots%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_boots%]%'
      endif
      if %done_bracer%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_bracer%]%'
      endif
      if %done_helm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_helm%]%'
      endif
      if %done_arm%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_arms%]%'
      endif
      if %done_legs%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_legs%]%'
      endif
      if %done_chest%
         msend %actor% %self.name% tells you, '  %get.obj_shortdesc[%vnum_reward_chest%]%'
      endif
   else
      msend %actor% %self.name% tells you, 'You haven't even talked to me about armor quests yet!'
   endif
endif
~
#55632
phase_armor_refuse~
0 j 100
~
return 0
msend %actor% %self.name% tells you, 'Why are you handing me this stuff?  I don't want it!'
msend %actor% %self.name% refuses to accept %object.shortdesc% from you.
~
#55694
Phase 1 Miniboss Drops~
0 f 100
~
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 28 pieces of armor in phase_1
set what_armor_drop %random.28%
* 28 gems in phase_1
set what_gem_drop %random.28%
* 
if %will_drop% <= 30
   * 30% to drop nothing
elseif %will_drop% <= 70 
   * 40% to drop a gem
   eval gem_vnum %what_gem_drop% + 55565
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * 20% to drop armor
   * drop destroyed armor 55299 is the vnum before the
   * first piece of armor.
   eval armor_vnum %what_armor_drop% + 55299
   * do this because decayed medium armor isn't used, replace with warrior/cleric
   if %armor_vnum% == 55303
      set armor_vnum 55300
   elseif %armor_vnum% == 55307
      set armor_vnum 55304 
   elseif %armor_vnum% == 55311
      set armor_vnum 55308
   elseif %armor_vnum% == 55315
      set armor_vnum 55312
   elseif %armor_vnum% == 55319
      set armor_vnum 55316
   elseif %armor_vnum% == 55323
      set armor_vnum 55320 
   elseif %armor_vnum% == 55327
      set armor_vnum 55324 
   endif
   mload obj %armor_vnum%
else
   * 10% chance to drop armor and gem
   eval gem_vnum %what_gem_drop% + 55565
   eval armor_vnum %what_armor_drop% + 55299
   * do this because decayed medium armor isn't used, replace with warrior/cleric
   if %armor_vnum% == 55303
      set armor_vnum 55300
   elseif %armor_vnum% == 55307
      set armor_vnum 55304 
   elseif %armor_vnum% == 55311
      set armor_vnum 55308
   elseif %armor_vnum% == 55315
      set armor_vnum 55312
   elseif %armor_vnum% == 55319
      set armor_vnum 55316
   elseif %armor_vnum% == 55323
      set armor_vnum 55320 
   elseif %armor_vnum% == 55327
      set armor_vnum 55324 
   endif
   mload obj %gem_vnum%
   mload obj %armor_vnum%
endif
~
#55695
Phase 1 Boss Drops~
0 f 100
~
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 28 pieces of armor in phase_1
set what_armor_drop %random.28%
* 28 gems in phase_1
set what_gem_drop %random.28%
* 
if %will_drop% <= 20
   * 20% to drop nothing
elseif %will_drop% <= 60 
   * 40% to drop a gem
   eval gem_vnum %what_gem_drop% + 55565
elseif (%will_drop% >= 61 && %will_drop% <= 80)
   * 20% to drop armor
   * drop destroyed armor 55299 is the vnum before the
   * first piece of armor.
   eval armor_vnum %what_armor_drop% + 55299
   * do this because decayed medium armor isn't used, replace with warrior/cleric
   if %armor_vnum% == 55303
      set armor_vnum 55300
   elseif %armor_vnum% == 55307
      set armor_vnum 55304 
   elseif %armor_vnum% == 55311
      set armor_vnum 55308
   elseif %armor_vnum% == 55315
      set armor_vnum 55312
   elseif %armor_vnum% == 55319
      set armor_vnum 55316
   elseif %armor_vnum% == 55323
      set armor_vnum 55320 
   elseif %armor_vnum% == 55327
      set armor_vnum 55324 
   endif
   mload obj %armor_vnum%
else
   * 20% chance to drop armor and gem
   eval gem_vnum %what_gem_drop% + 55565
   eval armor_vnum %what_armor_drop% + 55299
   * do this because decayed medium armor isn't used, replace with warrior/cleric
   if %armor_vnum% == 55303
      set armor_vnum 55300
   elseif %armor_vnum% == 55307
      set armor_vnum 55304 
   elseif %armor_vnum% == 55311
      set armor_vnum 55308
   elseif %armor_vnum% == 55315
      set armor_vnum 55312
   elseif %armor_vnum% == 55319
      set armor_vnum 55316
   elseif %armor_vnum% == 55323
      set armor_vnum 55320 
   elseif %armor_vnum% == 55327
      set armor_vnum 55324 
   endif
   mload obj %gem_vnum%
   mload obj %armor_vnum%
endif
~
#55696
Phase 2 Miniboss Drops~
0 f 100
~
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 28 pieces of armor in phase_2
set what_armor_drop %random.28%
* 77 gems in phase_2
set what_gem_drop %random.77%
* 
if %will_drop% <= 30
   * 30% to drop nothing
elseif %will_drop% <= 70 
   * 40% to drop a gem
   eval gem_vnum %what_gem_drop% + 55593
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * 20% to drop armor
   eval armor_vnum %what_armor_drop% + 55327
   mload obj %armor_vnum%
else
   * 10% chance to drop armor and gem
   eval gem_vnum %what_gem_drop% + 55593
   eval armor_vnum %what_armor_drop% + 55327
   mload obj %gem_vnum%
   mload obj %armor_vnum%
endif
~
#55697
Phase 2 Boss Drops~
0 f 100
~
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 28 pieces of armor in phase_2
set what_armor_drop %random.28%
* 77 gems in phase_2
set what_gem_drop %random.28%
* 
if %will_drop% <= 20
   * 20% to drop nothing
elseif %will_drop% <= 60 
   * 40% to drop a gem
   eval gem_vnum %what_gem_drop% + 55593
elseif (%will_drop% >= 61 && %will_drop% <= 80)
   * 20% to drop armor
   eval armor_vnum %what_armor_drop% + 55327
   mload obj %armor_vnum%
else
   * 20% chance to drop armor and gem
   eval gem_vnum %what_gem_drop% + 55593
   eval armor_vnum %what_armor_drop% + 55327
   mload obj %gem_vnum%
   mload obj %armor_vnum%
endif
~
#55698
Phase 3 Miniboss Drops~
0 f 100
~
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 28 pieces of armor in phase_3
set what_armor_drop %random.28%
* 77 gems in phase_3
set what_gem_drop %random.28%
* 
if %will_drop% <= 30
   * 30% to drop nothing
elseif %will_drop% <= 70
   * 40% to drop a gem
   eval gem_vnum %what_gem_drop% + 55670
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * 20% to drop armor
   eval armor_vnum %what_armor_drop% + 55355
   mload obj %armor_vnum%
else
   * 10% chance to drop armor and gem
   eval gem_vnum %what_gem_drop% + 55670
   eval armor_vnum %what_armor_drop% + 55355
   mload obj %gem_vnum%
   mload obj %armor_vnum%
endif
~
#55699
Phase 3 Boss Drops~
0 f 100
~
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 28 pieces of armor in phase_1
set what_armor_drop %random.28%
* 28 gems in phase_1
set what_gem_drop %random.28%
* 
if %will_drop% <= 20
   * 20% to drop nothing
elseif %will_drop% <= 60 
   * 40% to drop a gem
   eval gem_vnum %what_gem_drop% + 55670
elseif (%will_drop% >= 61 && %will_drop% <= 80)
   * 20% to drop armor
   eval armor_vnum %what_armor_drop% + 55355
   mload obj %armor_vnum%
else
   * 20% chance to drop armor and gem
   eval gem_vnum %what_gem_drop% + 55670
   eval armor_vnum %what_armor_drop% + 55355
   mload obj %gem_vnum%
   mload obj %armor_vnum%
endif
~
$~
