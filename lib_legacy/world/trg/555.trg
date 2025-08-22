#55500
sorcerer_phase_3~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
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
   *
   * attempt to reinitialize slutty dg variables to "" (nothing)
   * so this switch will work.
   *
   unset vnum_armor 
   unset vnum_gem
   unset vnum_reward
   *
   *
   * These cases set varialbes for the
   * quest.
   *
   switch %object.vnum%
     case %vnum_destroyed_helm%
       set is_armor 1
     case %vnum_gem_helm%
       set exp_multiplier 11
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 12
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 14
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 13
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_gloves%
       set vnum_gem %vnum_gem_gloves%
       set vnum_reward %vnum_reward_gloves%
       break
     default
       return 0
       wait 1
       eye %actor.name%
       msend %actor% %self.name% tells you, 'I am not interested in this from you.'
       msend %actor% %self.name% returns your item to you.
       halt
       break
     done
*
* need to force character saving after each turnin.
* 
   if !%is_armor%
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired 0
      endif
      eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
      if %gems% < 3
         eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
         wait 2
         msend %actor% %self.name% tells you, "Hey, very nice. It is good to see adventurers out conquering the"
         msend %actor% %self.name% tells you, "realm.  You have now given me %gems% %get.obj_shortdesc[%vnum_gem%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me 3 already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %gems% == 3 && %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% == 1
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   *
   * other if's for the armor turn-ins should go between here and the last object
   *
   else
      * here is where the armor section goes
      * that is true for is_armor == 1
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired 0
      endif
      eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
      if %armor% < 1
         eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
         wait 2
         msend %actor% %self.name% tells you, "Hey now. what have we here?!  I've been looking for some of this"
         msend %actor% %self.name% tells you, "for quite some time.  You have now given me %get.obj_shortdesc[%vnum_armor%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me %get.obj_shortdesc[%vnum_armor%]% already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %armor% == 1 && %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% == 3
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   endif
   * last object if before else that returns unwated objects to the player
else
   * not the correct class for this particular quest?
   *
   * This return will prevent the actual give from
   * the player in the first place and make it look
   * like homeslice is giving the object back.
   *
   return 0
   wait 1
   eye %actor.name%
   msend %actor% %self.name% tells you, 'I am not interested in this from you.'
   msend %actor% %self.name% returns your item to you.
endif
~
#55501
phase_3_monk_greet~
0 g 100
~
wait 2
if %actor.class% == monk && %actor.level% >= 41
      if %actor.quest_stage[phase_armor]% == 2 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some armor quests?"
         msend %actor% %self.name% tells you, "If so, just ask me, Yes I would like to do some armor quests"
   endif
else
endif
~
#55502
Phase_3_monk_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == monk
   wait 2
   if %actor.quest_stage[phase_armor]% == 2
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice boots, a cap, fistwraps,"
   msend %actor% %self.name% tells you, "sleeves, leggings, tunic, and a bracer."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me status at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for monks only."
endif
~
#55503
phase_3_monk_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "monk types but I'll need 3 enchanted pearls, and"
   msend %actor% %self.name% tells you, "a set of Corroded Boots. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55504
phase_3_monk_helm~
0 n 100
cap~
*
* helm ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A cap for ya? Well now I can make a nice cap for the monk types"
   msend %actor% %self.name% tells you, "but I'll need 3 cursed toapzs, and a Corroded Cap."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55505
phase_3_monk_gauntlets~
0 n 100
fist wraps fistwraps~
*
* Gauntlets ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of Fistwraps for the"
   msend %actor% %self.name% tells you, "monk types but I'll need 3 perfect chrysobeyrls, and a set of Corroded"
   msend %actor% %self.name% tells you, "Gloves. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55506
phase_3_monk_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the monk types but I'll need 3 enchaned garnets, and"
   msend %actor% %self.name% tells you, "a set of Corroded Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55507
phase_3_monk_greaves~
0 n 100
leggings~
*
* Greaves ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "leggings for the monk types but I'll need 3 cursed opals,"
   msend %actor% %self.name% tells you, "and a set of Corroded Pants. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55508
phase_3_monk_plate~
0 n 100
tunic~
*
* Plate ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine tunic for the monks but"
   msend %actor% %self.name% tells you, "I'll need 3 cursed rubies, and a Corroded Jerkin. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55509
phase_3_monk_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "monk types but I'll need 3 radiant jades, and a Corroded Wristband."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55510
monk_phase_3~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class monk
if %actor.class% == %class% && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55358
   set vnum_destroyed_boots	55362
   set vnum_destroyed_bracer	55366
   set vnum_destroyed_helm	55370
   set vnum_destroyed_arm	55374
   set vnum_destroyed_legs	55378
   set vnum_destroyed_chest	55382
   * gems for this class
   set vnum_gem_gloves	55674
   set vnum_gem_boots	55685
   set vnum_gem_bracer	55696
   set vnum_gem_helm	55707
   set vnum_gem_arm	55718
   set vnum_gem_legs	55729
   set vnum_gem_chest	55740
   * rewards for this class
   set vnum_reward_helm 55538
   set vnum_reward_arms 55539
   set vnum_reward_chest 55540
   set vnum_reward_legs 55541
   set vnum_reward_boots 55542
   set vnum_reward_bracer 55543
   set vnum_reward_gloves 55544
   *
   * attempt to reinitialize slutty dg variables to "" (nothing)
   * so this switch will work.
   *
   unset vnum_armor 
   unset vnum_gem
   unset vnum_reward
   *
   *
   * These cases set varialbes for the
   * quest.
   *
   switch %object.vnum%
     case %vnum_destroyed_helm%
       set is_armor 1
     case %vnum_gem_helm%
       set exp_multiplier 11
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 12
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 14
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 13
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_gloves%
       set vnum_gem %vnum_gem_gloves%
       set vnum_reward %vnum_reward_gloves%
       break
     default
       return 0
       wait 1
       eye %actor.name%
       msend %actor% %self.name% tells you, 'I am not interested in this from you.'
       msend %actor% %self.name% returns your item to you.
       halt
       break
     done
*
* need to force character saving after each turnin.
* 
   if !%is_armor%
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired 0
      endif
      eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
      if %gems% < 3
         eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
         wait 2
         msend %actor% %self.name% tells you, "Hey, very nice. It is good to see adventurers out conquering the"
         msend %actor% %self.name% tells you, "realm.  You have now given me %gems% %get.obj_shortdesc[%vnum_gem%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me 3 already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %gems% == 3 && %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% == 1
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   *
   * other if's for the armor turn-ins should go between here and the last object
   *
   else
      * here is where the armor section goes
      * that is true for is_armor == 1
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired 0
      endif
      eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
      if %armor% < 1
         eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
         wait 2
         msend %actor% %self.name% tells you, "Hey now. what have we here?!  I've been looking for some of this"
         msend %actor% %self.name% tells you, "for quite some time.  You have now given me %get.obj_shortdesc[%vnum_armor%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me %get.obj_shortdesc[%vnum_armor%]% already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %armor% == 1 && %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% == 3
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   endif
   * last object if before else that returns unwated objects to the player
else
   * not the correct class for this particular quest?
   *
   * This return will prevent the actual give from
   * the player in the first place and make it look
   * like homeslice is giving the object back.
   *
   return 0
   wait 1
   eye %actor.name%
   msend %actor% %self.name% tells you, 'I am not interested in this from you.'
   msend %actor% %self.name% returns your item to you.
endif
~
#55511
phase_3_necromancer_greet~
0 g 100
~
wait 2
if %actor.class% == necromancer && %actor.level% >= 41
      if %actor.quest_stage[phase_armor]% == 2 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some armor quests?"
         msend %actor% %self.name% tells you, "If so, just ask me, Yes I would like to do some armor quests"
   endif
else
endif
~
#55512
Phase_3_necromancer_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == necromancer
   wait 2
   if %actor.quest_stage[phase_armor]% == 2
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice slippers, a skullcap, gloves,"
   msend %actor% %self.name% tells you, "sleeves, pants, robe, and a bracelet."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me status at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for necromancers only."
endif
~
#55513
phase_3_necromancer_boots~
0 n 100
slippers~
*
* Boots ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of slippers for the"
   msend %actor% %self.name% tells you, "necromancer types but I'll need 3 radiant citrines, and"
   msend %actor% %self.name% tells you, "a set of Worn Slippers. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55514
phase_3_necromancer_helm~
0 n 100
cap skullcap~
*
* helm ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A skullcap for ya? Well now I can make a nice cap for the necromancer types"
   msend %actor% %self.name% tells you, "but I'll need 3 enchanted aquamarines, and a Worn Turban."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55515
phase_3_necromancer_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "necromancer types but I'll need 3 handfulls of amythests, and a set"
   msend %actor% %self.name% tells you, "of Worn Mittens. Return these things to me in any order at any"
   msend %actor% %self.name% tells you, "time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55516
phase_3_necromancer_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the necromancer types but I'll need 3 radiant pearls, and"
   msend %actor% %self.name% tells you, "a set of Worn Sleeves. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55517
phase_3_necromancer_greaves~
0 n 100
pants~
*
* Greaves ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "pants for the necromancer types but I'll need 3 enchanted sapphires,"
   msend %actor% %self.name% tells you, "and a set of Worn Leggings. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55518
phase_3_necromancer_plate~
0 n 100
robe~
*
* Plate ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine robe for the necromancers but"
   msend %actor% %self.name% tells you, "I'll need 3 cursed emeralds, and a Worn Robe. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55519
phase_3_necromancer_bracer~
0 n 100
bracelet~
*
* Bracer ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracelet for the"
   msend %actor% %self.name% tells you, "necromancer types but I'll need 3 perfect jades, and a"
   msend %actor% %self.name% tells you, "Worn Bracelet. Return these things to me in any order at any"
   msend %actor% %self.name% tells you, "time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55520
necromancer_phase_3~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class necromancer
if %actor.class% == %class% && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55359
   set vnum_destroyed_boots	55363
   set vnum_destroyed_bracer	55367
   set vnum_destroyed_helm	55371
   set vnum_destroyed_arm	55375
   set vnum_destroyed_legs	55379
   set vnum_destroyed_chest	55383
   * gems for this class
   set vnum_gem_gloves	55675
   set vnum_gem_boots	55686
   set vnum_gem_bracer	55697
   set vnum_gem_helm	55708
   set vnum_gem_arm	55719
   set vnum_gem_legs	55730
   set vnum_gem_chest	55741
   * rewards for this class
   set vnum_reward_helm 55531
   set vnum_reward_arms 55532
   set vnum_reward_chest 55533
   set vnum_reward_legs 55534
   set vnum_reward_boots 55535
   set vnum_reward_bracer 55536
   set vnum_reward_gloves 55537
   *
   * attempt to reinitialize slutty dg variables to "" (nothing)
   * so this switch will work.
   *
   unset vnum_armor 
   unset vnum_gem
   unset vnum_reward
   *
   *
   * These cases set varialbes for the
   * quest.
   *
   switch %object.vnum%
     case %vnum_destroyed_helm%
       set is_armor 1
     case %vnum_gem_helm%
       set exp_multiplier 11
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 12
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 14
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 13
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots% 
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_gloves%
       set vnum_gem %vnum_gem_gloves%
       set vnum_reward %vnum_reward_gloves%
       break
     default
       return 0
       wait 1
       eye %actor.name%
       msend %actor% %self.name% tells you, 'I am not interested in this from you.'
       msend %actor% %self.name% returns your item to you.
       halt
       break
     done
*
* need to force character saving after each turnin.
* 
   if !%is_armor%
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired 0
      endif
      eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
      if %gems% < 3
         eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
         wait 2
         msend %actor% %self.name% tells you, "Hey, very nice. It is good to see adventurers out conquering the"
         msend %actor% %self.name% tells you, "realm.  You have now given me %gems% %get.obj_shortdesc[%vnum_gem%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me 3 already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %gems% == 3 && %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% == 1
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   *
   * other if's for the armor turn-ins should go between here and the last object
   *
   else
      * here is where the armor section goes
      * that is true for is_armor == 1
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired 0
      endif
      eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
      if %armor% < 1
         eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
         wait 2
         msend %actor% %self.name% tells you, "Hey now. what have we here?!  I've been looking for some of this"
         msend %actor% %self.name% tells you, "for quite some time.  You have now given me %get.obj_shortdesc[%vnum_armor%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me %get.obj_shortdesc[%vnum_armor%]% already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %armor% == 1 && %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% == 3
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   endif
   * last object if before else that returns unwated objects to the player
else
   * not the correct class for this particular quest?
   *
   * This return will prevent the actual give from
   * the player in the first place and make it look
   * like homeslice is giving the object back.
   *
   return 0
   wait 1
   eye %actor.name%
   msend %actor% %self.name% tells you, 'I am not interested in this from you.'
   msend %actor% %self.name% returns your item to you.
endif
~
#55521
phase_3_paladin_greet~
0 g 100
~
wait 2
if %actor.class% == paladin && %actor.level% >= 41
      if %actor.quest_stage[phase_armor]% == 2 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some armor quests?"
         msend %actor% %self.name% tells you, "If so, just ask me, Yes I would like to do some armor quests"
   endif
else
endif
~
#55522
Phase_3_paladin_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == paladin
   wait 2
   if %actor.quest_stage[phase_armor]% == 2
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice boots, a helm, gauntlets,"
   msend %actor% %self.name% tells you, "vambraces, greaves, plate, and a bracer."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me status at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for paladins only."
endif
~
#55523
phase_3_paladin_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "paladin types but I'll need 3 pearls, and a set of"
   msend %actor% %self.name% tells you, "Tarnished Plate Boots. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55524
phase_3_paladin_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the paladin types"
   msend %actor% %self.name% tells you, "but I'll need 3 perfect topazses, and a Tarnished"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55525
phase_3_paladin_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "paladin types but I'll need 3 jades, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Gauntlets. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55526
phase_3_paladin_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the paladin types but I'll need 3 handfulls of garnets, and a set of"
   msend %actor% %self.name% tells you, "Tarnished Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55527
phase_3_paladin_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the paladin types but I'll need 3 perfect opals, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55528
phase_3_paladin_plate~
0 n 100
plate~
*
* Plate ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine plate for the paladins but"
   msend %actor% %self.name% tells you, "I'll need 3 radiant diamonds, and a Tarnished Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55529
phase_3_paladin_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "paladin types but I'll need 3 citrines, and a Tarnished Plate"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55530
paladin_phase_3~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class paladin
if %actor.class% == %class% && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55356
   set vnum_destroyed_boots	55360
   set vnum_destroyed_bracer	55364
   set vnum_destroyed_helm	55368
   set vnum_destroyed_arm	55372
   set vnum_destroyed_legs	55376
   set vnum_destroyed_chest	55380
   * gems for this class
   set vnum_gem_gloves	55676
   set vnum_gem_boots	55687
   set vnum_gem_bracer	55698
   set vnum_gem_helm	55709
   set vnum_gem_arm	55720
   set vnum_gem_legs	55731
   set vnum_gem_chest	55742
   * rewards for this class
   set vnum_reward_helm 55496
   set vnum_reward_arms 55497
   set vnum_reward_chest 55498
   set vnum_reward_legs 55499
   set vnum_reward_boots 55500
   set vnum_reward_bracer 55501
   set vnum_reward_gloves 55502
   *
   * attempt to reinitialize slutty dg variables to "" (nothing)
   * so this switch will work.
   *
   unset vnum_armor 
   unset vnum_gem
   unset vnum_reward
   *
   *
   * These cases set varialbes for the
   * quest.
   *
   switch %object.vnum%
     case %vnum_destroyed_helm%
       set is_armor 1
     case %vnum_gem_helm%
       set exp_multiplier 11
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 12
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 14
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 13
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_gloves%
       set vnum_gem %vnum_gem_gloves%
       set vnum_reward %vnum_reward_gloves%
       break
     default
       return 0
       wait 1
       eye %actor.name%
       msend %actor% %self.name% tells you, 'I am not interested in this from you.'
       msend %actor% %self.name% returns your item to you.
       halt
       break
     done
*
* need to force character saving after each turnin.
* 
   if !%is_armor%
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired 0
      endif
      eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
      if %gems% < 3
         eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
         wait 2
         msend %actor% %self.name% tells you, "Hey, very nice. It is good to see adventurers out conquering the"
         msend %actor% %self.name% tells you, "realm.  You have now given me %gems% %get.obj_shortdesc[%vnum_gem%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me 3 already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %gems% == 3 && %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% == 1
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   *
   * other if's for the armor turn-ins should go between here and the last object
   *
   else
      * here is where the armor section goes
      * that is true for is_armor == 1
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired 0
      endif
      eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
      if %armor% < 1
         eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
         wait 2
         msend %actor% %self.name% tells you, "Hey now. what have we here?!  I've been looking for some of this"
         msend %actor% %self.name% tells you, "for quite some time.  You have now given me %get.obj_shortdesc[%vnum_armor%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me %get.obj_shortdesc[%vnum_armor%]% already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %armor% == 1 && %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% == 3
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   endif
   * last object if before else that returns unwated objects to the player
else
   * not the correct class for this particular quest?
   *
   * This return will prevent the actual give from
   * the player in the first place and make it look
   * like homeslice is giving the object back.
   *
   return 0
   wait 1
   eye %actor.name%
   msend %actor% %self.name% tells you, 'I am not interested in this from you.'
   msend %actor% %self.name% returns your item to you.
endif
~
#55531
phase_3_ranger_greet~
0 g 100
~
wait 2
if %actor.class% == ranger && %actor.level% >= 41
      if %actor.quest_stage[phase_armor]% == 2 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some armor quests?"
         msend %actor% %self.name% tells you, "If so, just ask me, Yes I would like to do some armor quests"
   endif
else
endif
~
#55532
Phase_3_ranger_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == ranger
   wait 2
   if %actor.quest_stage[phase_armor]% == 2
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice boots, a cap, gloves,"
   msend %actor% %self.name% tells you, "sleeves, pants, jerkin, and a bracer."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me status at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for rangers only."
endif
~
#55533
phase_3_ranger_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "ranger types but I'll need 3 handfulls of citrines, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Chain Boots. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55534
phase_3_ranger_helm~
0 n 100
cap~
*
* helm ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A cap for ya? Well now I can make a nice cap for the ranger types"
   msend %actor% %self.name% tells you, "but I'll need 3 radiant aquamarines, and a Tarnished Coif."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55535
phase_3_ranger_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "ranger types but I'll need 3 tourmalines, and a set of Tarnished"
   msend %actor% %self.name% tells you, "Chain Gloves. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55536
phase_3_ranger_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the ranger types but I'll need 3 parfect garnets, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Sleeves. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55537
phase_3_ranger_greaves~
0 n 100
pants~
*
* Greaves ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "pants for the ranger types but I'll need 3 radiant sapphires,"
   msend %actor% %self.name% tells you, "and a set of Tarnished Leggings. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55538
phase_3_ranger_plate~
0 n 100
jerkin~
*
* Plate ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine jerkin for the rangers but"
   msend %actor% %self.name% tells you, "I'll need 3 enchanted rubies, and a Tarnished Tunic. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55539
phase_3_ranger_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "ranger types but I'll need 3 handfulls of chrysobeyrls, and a Tarnished"
   msend %actor% %self.name% tells you, "Chain Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55540
ranger_phase_3~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class ranger
if %actor.class% == %class% && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55357
   set vnum_destroyed_boots	55361
   set vnum_destroyed_bracer	55365
   set vnum_destroyed_helm	55369
   set vnum_destroyed_arm	55373
   set vnum_destroyed_legs	55377
   set vnum_destroyed_chest	55381
   * gems for this class
   set vnum_gem_gloves	55677
   set vnum_gem_boots	55688
   set vnum_gem_bracer	55699
   set vnum_gem_helm	55710
   set vnum_gem_arm	55721
   set vnum_gem_legs	55732
   set vnum_gem_chest	55743
   * rewards for this class
   set vnum_reward_helm 55517
   set vnum_reward_arms 55518
   set vnum_reward_chest 55519
   set vnum_reward_legs 55520
   set vnum_reward_boots 55521
   set vnum_reward_bracer 55522
   set vnum_reward_gloves 55523
   *
   * attempt to reinitialize slutty dg variables to "" (nothing)
   * so this switch will work.
   *
   unset vnum_armor 
   unset vnum_gem
   unset vnum_reward
   *
   *
   * These cases set varialbes for the
   * quest.
   *
   switch %object.vnum%
     case %vnum_destroyed_helm%
       set is_armor 1
     case %vnum_gem_helm%
       set exp_multiplier 11
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 12
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 14
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 13
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_gloves%
       set vnum_gem %vnum_gem_gloves%
       set vnum_reward %vnum_reward_gloves%
       break
     default
       return 0
       wait 1
       eye %actor.name%
       msend %actor% %self.name% tells you, 'I am not interested in this from you.'
       msend %actor% %self.name% returns your item to you.
       halt
       break
     done
*
* need to force character saving after each turnin.
* 
   if !%is_armor%
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired 0
      endif
      eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
      if %gems% < 3
         eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
         wait 2
         msend %actor% %self.name% tells you, "Hey, very nice. It is good to see adventurers out conquering the"
         msend %actor% %self.name% tells you, "realm.  You have now given me %gems% %get.obj_shortdesc[%vnum_gem%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me 3 already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %gems% == 3 && %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% == 1
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   *
   * other if's for the armor turn-ins should go between here and the last object
   *
   else
      * here is where the armor section goes
      * that is true for is_armor == 1
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired 0
      endif
      eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
      if %armor% < 1
         eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
         wait 2
         msend %actor% %self.name% tells you, "Hey now. what have we here?!  I've been looking for some of this"
         msend %actor% %self.name% tells you, "for quite some time.  You have now given me %get.obj_shortdesc[%vnum_armor%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me %get.obj_shortdesc[%vnum_armor%]% already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %armor% == 1 && %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% == 3
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   endif
   * last object if before else that returns unwated objects to the player
else
   * not the correct class for this particular quest?
   *
   * This return will prevent the actual give from
   * the player in the first place and make it look
   * like homeslice is giving the object back.
   *
   return 0
   wait 1
   eye %actor.name%
   msend %actor% %self.name% tells you, 'I am not interested in this from you.'
   msend %actor% %self.name% returns your item to you.
endif
~
#55541
phase_3_rogue_greet~
0 g 100
~
wait 2
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 41
      if %actor.quest_stage[phase_armor]% == 2 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some armor quests?"
         msend %actor% %self.name% tells you, "If so, just ask me, Yes I would like to do some armor quests"
   endif
else
endif
~
#55542
Phase_3_rogue_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary)
   wait 2
   if %actor.quest_stage[phase_armor]% == 2
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice boots, a cap, gloves,"
   msend %actor% %self.name% tells you, "sleeves, leggings, tunic, and a bracer."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me status at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for rogues only."
endif
~
#55543
phase_3_rogue_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "rogue types but I'll need 3 garnets, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Chain Boots. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55544
phase_3_rogue_helm~
0 n 100
cap~
*
* helm ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A cap for ya? Well now I can make a nice cap for the rogue types"
   msend %actor% %self.name% tells you, "but I'll need 3 perfect sapphires, and a Tarnished Coif."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55545
phase_3_rogue_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "rogue types but I'll need 3 flawed citrines, and a set of Tarnished"
   msend %actor% %self.name% tells you, "Chain Gloves. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55546
phase_3_rogue_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the rogue types but I'll need 3 aquamarines, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Sleeves. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55547
phase_3_rogue_greaves~
0 n 100
leggings~
*
* Greaves ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "leggings for the rogue types but I'll need 3 perfect diamonds,"
   msend %actor% %self.name% tells you, "and a set of Tarnished Leggings. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55548
phase_3_rogue_plate~
0 n 100
tunic~
*
* Plate ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine tunic for the rogues but"
   msend %actor% %self.name% tells you, "I'll need 3 radiant rubies, and a Tarnished Tunic. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55549
phase_3_rogue_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "rogue types but I'll need 3 flawed garnets, and a Tarnished Chain"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55550
rogue_phase_3~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
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
   *
   * attempt to reinitialize slutty dg variables to "" (nothing)
   * so this switch will work.
   *
   unset vnum_armor
   unset vnum_gem
   unset vnum_reward
   *
   *
   * These cases set varialbes for the
   * quest.
   *
   switch %object.vnum%
     case %vnum_destroyed_helm%
       set is_armor 1
     case %vnum_gem_helm%
       set exp_multiplier 11
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 12
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 14
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 13
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_gloves%
       set vnum_gem %vnum_gem_gloves%
       set vnum_reward %vnum_reward_gloves%
       break
     default
       return 0
       wait 1
       eye %actor.name%
       msend %actor% %self.name% tells you, 'I am not interested in this from you.'
       msend %actor% %self.name% returns your item to you.
       halt
       break
     done
*
* need to force character saving after each turnin.
* 
   if !%is_armor%
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired 0
      endif
      eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
      if %gems% < 3
         eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
         wait 2
         msend %actor% %self.name% tells you, "Hey, very nice. It is good to see adventurers out conquering the"
         msend %actor% %self.name% tells you, "realm.  You have now given me %gems% %get.obj_shortdesc[%vnum_gem%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me 3 already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %gems% == 3 && %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% == 1
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   *
   * other if's for the armor turn-ins should go between here and the last object
   *
   else
      * here is where the armor section goes
      * that is true for is_armor == 1
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired 0
      endif
      eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
      if %armor% < 1
         eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
         wait 2
         msend %actor% %self.name% tells you, "Hey now. what have we here?!  I've been looking for some of this"
         msend %actor% %self.name% tells you, "for quite some time.  You have now given me %get.obj_shortdesc[%vnum_armor%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me %get.obj_shortdesc[%vnum_armor%]% already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %armor% == 1 && %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% == 3
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   endif
   * last object if before else that returns unwated objects to the player
else
   * not the correct class for this particular quest?
   *
   * This return will prevent the actual give from
   * the player in the first place and make it look
   * like homeslice is giving the object back.
   *
   return 0
   wait 1
   eye %actor.name%
   msend %actor% %self.name% tells you, 'I am not interested in this from you.'
   msend %actor% %self.name% returns your item to you.
endif
~
#55551
phase_3_warrior_greet~
0 g 100
~
wait 2
if %actor.class% == warrior && %actor.level% >= 41
      if %actor.quest_stage[phase_armor]% == 2 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some armor quests?"
         msend %actor% %self.name% tells you, "If so, just ask me, Yes I would like to do some armor quests"
   endif
else
endif
~
#55552
Phase_3_warrior_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == warrior
   wait 2
   if %actor.quest_stage[phase_armor]% == 2
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice boots, a helm, gauntlets,"
   msend %actor% %self.name% tells you, "vambraces, greaves, plate, and a bracer."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me status at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for warriors only."
endif
~
#55553
phase_3_warrior_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "warrior types but I'll need 3 opal shards, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Plate Boots. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55554
phase_3_warrior_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the warrior types"
   msend %actor% %self.name% tells you, "but I'll need 3 sapphires, and a Tarnished"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55555
phase_3_warrior_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "warrior types but I'll need 3 uncut emeralds, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Gauntlets. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55556
phase_3_warrior_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the warrior types but I'll need 3 diamond shards, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55557
phase_3_warrior_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the warrior types but I'll need 3 opals, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55558
phase_3_warrior_plate~
0 n 100
plate~
*
* Plate ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine plate for the warriors but"
   msend %actor% %self.name% tells you, "I'll need 3 perfect emeralds, and a Tarnished Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55559
phase_3_warrior_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "warrior types but I'll need 3 sapphire shards, and a Tarnished Plate"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55560
warrior_phase_3~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class warrior
if %actor.class% == %class% && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55356
   set vnum_destroyed_boots	55360
   set vnum_destroyed_bracer	55364
   set vnum_destroyed_helm	55368
   set vnum_destroyed_arm	55372
   set vnum_destroyed_legs	55376
   set vnum_destroyed_chest	55380
   * gems for this class
   set vnum_gem_gloves	55681
   set vnum_gem_boots	55692
   set vnum_gem_bracer	55703
   set vnum_gem_helm	55714
   set vnum_gem_arm	55725
   set vnum_gem_legs	55736
   set vnum_gem_chest	55747
   * rewards for this class
   set vnum_reward_helm 55489
   set vnum_reward_arms 55490
   set vnum_reward_chest 55491
   set vnum_reward_legs 55492
   set vnum_reward_boots 55493
   set vnum_reward_bracer 55494
   set vnum_reward_gloves 55495
   *
   * attempt to reinitialize slutty dg variables to "" (nothing)
   * so this switch will work.
   *
   unset vnum_armor 
   unset vnum_gem
   unset vnum_reward
   *
   *
   * These cases set varialbes for the
   * quest.
   *
   switch %object.vnum%
     case %vnum_destroyed_helm%
       set is_armor 1
     case %vnum_gem_helm%
       set exp_multiplier 11
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 12
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 14
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 13
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_gloves%
       set vnum_gem %vnum_gem_gloves%
       set vnum_reward %vnum_reward_gloves%
       break
     default
       return 0
       wait 1
       eye %actor.name%
       msend %actor% %self.name% tells you, 'I am not interested in this from you.'
       msend %actor% %self.name% returns your item to you.
       halt
       break
     done
*
* need to force character saving after each turnin.
* 
   if !%is_armor%
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired 0
      endif
      eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]%
      quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
      if %gems% < 3
         eval gems %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_gem%_gems_acquired %gems%
         wait 2
         msend %actor% %self.name% tells you, "Hey, very nice. It is good to see adventurers out conquering the"
         msend %actor% %self.name% tells you, "realm.  You have now given me %gems% %get.obj_shortdesc[%vnum_gem%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me 3 already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %gems% == 3 && %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% == 1
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   *
   * other if's for the armor turn-ins should go between here and the last object
   *
   else
      * here is where the armor section goes
      * that is true for is_armor == 1
      * hrmm Jelos' magical variable declaration
      if %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      else
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired 0
      endif
      eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]%
      quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
      if %armor% < 1
         eval armor %actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% + 1
         quest variable phase_armor %actor.name% %vnum_armor%_armor_acquired %armor%
         wait 2
         msend %actor% %self.name% tells you, "Hey now. what have we here?!  I've been looking for some of this"
         msend %actor% %self.name% tells you, "for quite some time.  You have now given me %get.obj_shortdesc[%vnum_armor%]%."
         mjunk %object.name%
         msave %actor%
      else
         return 0
         wait 2
         eye %actor.name%
         wait 1
         msend %actor% %self.name% tells you, "Hey now, you have given me %get.obj_shortdesc[%vnum_armor%]% already!"
         msend %actor% %self.name% returns your item to you.
         *
         * Halt here so that the reward section isn't checked if the max
         * number of armor pieces has already been turned in so the reward
         * can't be given multiple times.
         *
         halt
      endif
      *
      * check to see if the quest is complete and the reward can be given..
      *
      If %armor% == 1 && %actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% == 3
         wait 2
         msend %actor% %self.name% tells you, "Excellent intrepid adventurer, you have provided me with all"
         msend %actor% %self.name% tells you, "I need in order to reward you with %get.obj_shortdesc[%vnum_reward%]%!"
         wait 1
         mload obj %vnum_reward%
         wait 1
         *
         * loop for exp award.
         *
msend %actor% &3&bYou gain experience!&0
set lap 1
while %lap% <= %exp_multiplier%
mexp %actor% 42840
eval lap %lap% + 1
done
         * Note while loops can't be indented, due to dumbass
         * coders.
         * end exp loop
         *
         * all other items should be mpjunked or whatever by now
         give all %actor.name%
      endif
   endif
   * last object if before else that returns unwated objects to the player
else
   * not the correct class for this particular quest?
   *
   * This return will prevent the actual give from
   * the player in the first place and make it look
   * like homeslice is giving the object back.
   *
   return 0
   wait 1
   eye %actor.name%
   msend %actor% %self.name% tells you, 'I am not interested in this from you.'
   msend %actor% %self.name% returns your item to you.
endif
~
#55561
drop_phase_1_mini_boss_2~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70 
   * Normal non-bonus drops
   if %bonus% <= 90
      * drop a gem 55565 is the vnum before very first gem
      eval gem_vnum %what_gem_drop% + 55565
      mload obj %gem_vnum%
   else
      * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55569
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 90
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55299
      mload obj %armor_vnum%
   else
      * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 90
      * drop armor and gem
      eval gem_vnum %what_gem_drop% + 55565
      eval armor_vnum %what_armor_drop% + 55299
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
      * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55569
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %armor_vnum%
   endif
endif
~
#55562
drop_phase_1_boss_2~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 20
   * drop nothing and bail
   halt
endif
if %will_drop% <= 60 
   * Normal non-bonus drops
   if %bonus% <= 80
      * drop a gem 55565 is the vnum before very first gem
      eval gem_vnum %what_gem_drop% + 55565
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55569
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 80
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55299
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 80
      * drop armor and gem
      eval gem_vnum %what_gem_drop% + 55565
      eval armor_vnum %what_armor_drop% + 55299
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55569
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %armor_vnum%
   endif
endif
~
#55563
drop_phase_1_mini_boss_4~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55565
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55569
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55299
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55565
      eval armor_vnum %what_armor_drop% + 55299
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55569
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
   endif
endif
~
#55564
drop_phase_1_boss_4~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 20
   * drop nothing and bail
   halt
endif
if %will_drop% <= 60 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55565
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55569
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55299
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55565
      eval armor_vnum %what_armor_drop% + 55299
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55569
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
   endif
endif
~
#55565
drop_phase_1_mini_boss_7~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55569
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55569
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   endif
endif
~
#55566
drop_phase_1_boss_7~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 20
   * drop nothing and bail
   halt
endif
if %will_drop% <= 60 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55569
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55569
      eval armor_vnum %what_armor_drop% + 55303
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   endif
endif
~
#55567
drop_phase_1_mini_boss_9~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55573
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   endif
endif
~
#55568
drop_phase_1_boss_9~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 20
   * drop nothing and bail
   halt
endif
if %will_drop% <= 60 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55573
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   endif
endif
~
#55569
drop_phase_1_mini_boss_12~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55577
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   endif
endif
~
#55570
drop_phase_1_boss_12~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 20
   * drop nothing and bail
   halt
endif
if %will_drop% <= 60 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55577
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   endif
endif
~
#55571
drop_phase_1_mini_boss_15~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55581
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
   endif
endif
~
#55572
drop_phase_1_boss_15~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 20
   * drop nothing and bail
   halt
endif
if %will_drop% <= 60 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55581
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
   endif
endif
~
#55573
drop_phase_1_mini_boss_18~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55585
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
   endif
endif
~
#55574
drop_phase_1_boss_18~
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
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
* 
if %will_drop% <= 20
   * drop nothing and bail
   halt
endif
if %will_drop% <= 60 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55585
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55585
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
   endif
endif
~
#55575
drop_phase_2_mini_boss_22~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55604
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55589
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55604
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %armor_vnum%
   endif
endif
~
#55576
drop_phase_2_boss_22~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55589
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55604
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55589
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55604
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %armor_vnum%
   endif
endif
~
#55577
drop_phase_2_mini_boss_24~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55604
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55593
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55604
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
   endif
endif
~
#55578
drop_phase_2_boss_24~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55593
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55604
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55593
      eval armor_vnum %what_armor_drop% + 55327
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55604
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
   endif
endif
~
#55579
drop_phase_2_mini_boss_27~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55604
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55604
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %armor_vnum%
   endif
endif
~
#55580
drop_phase_2_boss_27~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55604
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55604
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55639
      mload obj %armor_vnum%
   endif
endif
~
#55581
drop_phase_2_mini_boss_29~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55637
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55615
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55637
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %armor_vnum%
   endif
endif
~
#55582
drop_phase_2_boss_29~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55637
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55615
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55637
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %armor_vnum%
   endif
endif
~
#55583
drop_phase_2_mini_boss_32~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55637
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55626
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55637
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
   endif
endif
~
#55584
drop_phase_2_boss_32~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55637
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55626
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55637
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
   endif
endif
~
#55585
drop_phase_2_mini_boss_35~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55637
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      say normal < 90
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      say normal 50 to 90
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      say normal > 90
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      say bonus < 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55637
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      say bonus 50 to 90
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      say bonus > 90
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
   endif
endif
~
#55586
drop_phase_2_boss_35~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55637
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55637
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
   endif
endif
~
#55587
drop_phase_2_mini_boss_38~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55670
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55648
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55670
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %armor_vnum%
   endif
endif
~
#55588
drop_phase_2_boss_38~
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
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55670
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      *
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55648
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55670
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %armor_vnum%
   endif
endif
~
#55589
drop_phase_3_mini_boss_42~
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
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55670
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55681
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55659
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55670
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55681
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55590
drop_phase_3_boss_42~
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
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55670
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55681
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      *
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55659
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55670
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55681
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55591
drop_phase_3_mini_boss_44~
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
      eval gem_vnum %what_gem_drop% + 55670
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55681
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55692
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55670
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55681
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55692
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55592
drop_phase_3_boss_44~
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
      eval gem_vnum %what_gem_drop% + 55670
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55681
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55692
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      *
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55670
      eval armor_vnum %what_armor_drop% + 55355
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55681
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55692
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55593
drop_phase_3_mini_boss_47~
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
      eval gem_vnum %what_gem_drop% + 55681
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55692
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55703
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55681
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55692
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55703
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55594
drop_phase_3_boss_47~
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
      eval gem_vnum %what_gem_drop% + 55681
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55692
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55703
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      *
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55681
      eval armor_vnum %what_armor_drop% + 55359
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55692
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55703
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55595
drop_phase_3_mini_boss_49~
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
      eval gem_vnum %what_gem_drop% + 55692
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55703
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55714
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55692
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55703
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55714
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55596
drop_phase_3_boss_49~
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
      eval gem_vnum %what_gem_drop% + 55692
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55703
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55714
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      *
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55692
      eval armor_vnum %what_armor_drop% + 55363
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55703
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55714
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55597
drop_phase_3_mini_boss_52~
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
      eval gem_vnum %what_gem_drop% + 55703
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55714
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55725
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 && %will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      *
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55703
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55714
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55725
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55598
drop_phase_3_boss_52~
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
      eval gem_vnum %what_gem_drop% + 55703
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55714
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55725
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 && %will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      *
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55703
      eval armor_vnum %what_armor_drop% + 55367
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 && %bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55714
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55725
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#55599
drop_phase_3_mini_boss_55~
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
elseif (%will_drop% >= 71 && %will_drop% <= 90)
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
$~
