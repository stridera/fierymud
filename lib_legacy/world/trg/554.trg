#55400
necromancer_phase_2~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class necromancer
if %actor.class% == %class% && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55331
   set vnum_destroyed_boots	55335
   set vnum_destroyed_bracer	55339
   set vnum_destroyed_helm	55343
   set vnum_destroyed_arm	55347
   set vnum_destroyed_legs	55351
   set vnum_destroyed_chest	55355
   * gems for this class
   set vnum_gem_gloves	55598
   set vnum_gem_boots	55609
   set vnum_gem_bracer	55620
   set vnum_gem_helm	55631
   set vnum_gem_arm	55642
   set vnum_gem_legs	55653
   set vnum_gem_chest	55664
   * rewards for this class
   set vnum_reward_helm 55454
   set vnum_reward_arms 55455
   set vnum_reward_chest 55456
   set vnum_reward_legs 55457
   set vnum_reward_boots 55458
   set vnum_reward_bracer 55459
   set vnum_reward_gloves 55460
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
       set exp_multiplier 7
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 5
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 6
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 4
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
mexp %actor% 29880
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
mexp %actor% 29880
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
#55401
phase_2_paladin_greet~
0 g 100
~
wait 2
if %actor.class% == paladin && %actor.level% >= 21
   if %actor.quest_stage[phase_armor]% == 0
      quest start phase_armor %actor.name%
   endif
      if %actor.quest_stage[phase_armor]% == 1 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some &7&barmor quests&0?"
         msend %actor% %self.name% tells you, "If so, just ask me, &7&bYes&0 I would like to do some &7&barmor quests&0"
   endif
else
endif
~
#55402
Phase_2_paladin_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == paladin
   wait 2
   if %actor.quest_stage[phase_armor]% == 1
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice &7&bboots&0, a &7&bhelm&0, &7&bgauntlets&0,"
   msend %actor% %self.name% tells you, "&7&bvambraces&0, &7&bgreaves&0, &7&bplate&0, and a &7&bbracer&0."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for paladins only."
endif
~
#55403
phase_2_paladin_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "paladin types but I'll need 3 jade shards, and a set of"
   msend %actor% %self.name% tells you, "Crushed Plate Boots. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55404
phase_2_paladin_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the paladin types"
   msend %actor% %self.name% tells you, "but I'll need 3 perfect turquoises, and a Crushed"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55405
phase_2_paladin_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "paladin types but I'll need 3 uncut chrysobeyrls, and"
   msend %actor% %self.name% tells you, "a set of Crushed Gauntlets. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55406
phase_2_paladin_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the paladin types but I'll need 3 citrine shards, and a set of"
   msend %actor% %self.name% tells you, "Crushed Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55407
phase_2_paladin_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the paladin types but I'll need 3 radiant amythests, and"
   msend %actor% %self.name% tells you, "a set of Crushed Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55408
phase_2_paladin_plate~
0 n 100
plate~
*
* Plate ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine plate for the paladins but"
   msend %actor% %self.name% tells you, "I'll need 3 flawed diamonds, and a Crushed Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55409
phase_2_paladin_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for paladins only
if %actor.class% == paladin && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "paladin types but I'll need 3 amythest shards, and a Crushed Plate"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for paladins only.
endif
~
#55410
paladin_phase_2~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class paladin
if %actor.class% == %class% && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55328
   set vnum_destroyed_boots	55332
   set vnum_destroyed_bracer	55336
   set vnum_destroyed_helm	55340
   set vnum_destroyed_arm	55344
   set vnum_destroyed_legs	55348
   set vnum_destroyed_chest	55352
   * gems for this class
   set vnum_gem_gloves	55599
   set vnum_gem_boots	55610
   set vnum_gem_bracer	55621
   set vnum_gem_helm	55632
   set vnum_gem_arm	55643
   set vnum_gem_legs	55654
   set vnum_gem_chest	55665
   * rewards for this class
   set vnum_reward_helm 55419
   set vnum_reward_arms 55420
   set vnum_reward_chest 55421
   set vnum_reward_legs 55422
   set vnum_reward_boots 55423
   set vnum_reward_bracer 55424
   set vnum_reward_gloves 55425
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
       set exp_multiplier 7
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 5
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 6
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 4
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
mexp %actor% 29880
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
mexp %actor% 29880
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
#55411
phase_2_ranger_greet~
0 g 100
~
wait 2
if %actor.class% == ranger && %actor.level% >= 21
   if %actor.quest_stage[phase_armor]% == 0
      quest start phase_armor %actor.name%
   endif
      if %actor.quest_stage[phase_armor]% == 1 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some &7&barmor quests&0?"
         msend %actor% %self.name% tells you, "If so, just ask me, &7&bYes&0 I would like to do some &7&barmor quests&0"
   endif
else
endif
~
#55412
Phase_2_ranger_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == ranger
   wait 2
   if %actor.quest_stage[phase_armor]% == 1
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice &7&bboots&0, a &7&bcap&0, &7&bgloves&0,"
   msend %actor% %self.name% tells you, "&7&bsleeves&0, &7&bpants&0, &7&bjerkin&0, and a &7&bbracer&0."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for rangers only."
endif
~
#55413
phase_2_ranger_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "ranger types but I'll need 3 flawed tourmalines, and"
   msend %actor% %self.name% tells you, "a set of Crushed Chain Boots. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55414
phase_2_ranger_helm~
0 n 100
cap~
*
* helm ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A cap for ya? Well now I can make a nice cap for the ranger types"
   msend %actor% %self.name% tells you, "but I'll need 3 radiant lapis-lazulis, and a Crushed Coif."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55415
phase_2_ranger_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "ranger types but I'll need 3 uncut amythests, and a set of Crushed"
   msend %actor% %self.name% tells you, "Chain Gloves. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55416
phase_2_ranger_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the ranger types but I'll need 3 flawed chrysobeyrls, and"
   msend %actor% %self.name% tells you, "a set of Crushed Sleeves. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55417
phase_2_ranger_greaves~
0 n 100
pants~
*
* Greaves ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "pants for the ranger types but I'll need 3 enchanted chrysobeyrl,"
   msend %actor% %self.name% tells you, "and a set of Crushed Leggings. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55418
phase_2_ranger_plate~
0 n 100
jerkin~
*
* Plate ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine jerkin for the rangers but"
   msend %actor% %self.name% tells you, "I'll need 3 rubies, and a Crushed Tunic. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55419
phase_2_ranger_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for rangers only
if %actor.class% == ranger && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "ranger types but I'll need 3 lapis-lazuli shards, and a Crushed Chain"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for rangers only.
endif
~
#55420
ranger_phase_2~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class ranger
if %actor.class% == %class% && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55329
   set vnum_destroyed_boots	55333
   set vnum_destroyed_bracer	55337
   set vnum_destroyed_helm	55341
   set vnum_destroyed_arm	55345
   set vnum_destroyed_legs	55349
   set vnum_destroyed_chest	55353
   * gems for this class
   set vnum_gem_gloves	55600
   set vnum_gem_boots	55611
   set vnum_gem_bracer	55622
   set vnum_gem_helm	55633
   set vnum_gem_arm	55644
   set vnum_gem_legs	55655
   set vnum_gem_chest	55666
   * rewards for this class
   set vnum_reward_helm 55440
   set vnum_reward_arms 55441
   set vnum_reward_chest 55442
   set vnum_reward_legs 55443
   set vnum_reward_boots 55444
   set vnum_reward_bracer 55445
   set vnum_reward_gloves 55446
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
       set exp_multiplier 7
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 5
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 6
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 4
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
mexp %actor% 29880
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
mexp %actor% 29880
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
#55421
phase_2_rogue_greet~
0 g 100
~
* Rogue, thief, assassin, merc
wait 2
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 21
   if %actor.quest_stage[phase_armor]% == 0
      quest start phase_armor %actor.name%
   endif
      if %actor.quest_stage[phase_armor]% == 1 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some &7&barmor quests&0?"
         msend %actor% %self.name% tells you, "If so, just ask me, &7&bYes&0 I would like to do some &7&barmor quests&0"
   endif
else
endif
~
#55422
Phase_2_rogue_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary)
   wait 2
   if %actor.quest_stage[phase_armor]% == 1
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice &7&bboots&0, a &7&bcap&0, &7&bgloves&0,"
   msend %actor% %self.name% tells you, "&7&bsleeves&0, &7&bleggings&0, &7&btunic&0, and a &7&bbracer&0."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for rogues only."
endif
~
#55423
phase_2_rogue_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "rogue types but I'll need 3 uncut topazes, and"
   msend %actor% %self.name% tells you, "a set of Crushed Chain Boots. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55424
phase_2_rogue_helm~
0 n 100
cap~
*
* helm ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A cap for ya? Well now I can make a nice cap for the rogue types"
   msend %actor% %self.name% tells you, "but I'll need 3 perfect lapis-lazulis, and a Crushed Coif."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55425
phase_2_rogue_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "rogue types but I'll need 3 uncut jades, and a set of Crushed"
   msend %actor% %self.name% tells you, "Chain Gloves. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55426
phase_2_rogue_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the rogue types but I'll need 3 pearl shards, and"
   msend %actor% %self.name% tells you, "a set of Crushed Sleeves. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55427
phase_2_rogue_greaves~
0 n 100
leggings~
*
* Greaves ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "leggings for the rogue types but I'll need 3 radiant tourmalines,"
   msend %actor% %self.name% tells you, "and a set of Crushed Leggings. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55428
phase_2_rogue_plate~
0 n 100
tunic~
*
* Plate ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine tunic for the rogues but"
   msend %actor% %self.name% tells you, "I'll need 3 flawed rubies, and a Crushed Tunic. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55429
phase_2_rogue_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for rogues only
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "rogue types but I'll need 3 tourmaline shards, and a Crushed Chain"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55430
rogue_phase_2~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
if (%actor.class% == rogue || %actor.class% == thief || %actor.class% == assassin || %actor.class% == mercenary) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55329
   set vnum_destroyed_boots	55333
   set vnum_destroyed_bracer	55337
   set vnum_destroyed_helm	55341
   set vnum_destroyed_arm	55345
   set vnum_destroyed_legs	55349
   set vnum_destroyed_chest	55353
   * gems for this class
   set vnum_gem_gloves	55603
   set vnum_gem_boots	55614
   set vnum_gem_bracer	55625
   set vnum_gem_helm	55636
   set vnum_gem_arm	55647
   set vnum_gem_legs	55658
   set vnum_gem_chest	55669
   * rewards for this class
   set vnum_reward_helm 55482
   set vnum_reward_arms 55483
   set vnum_reward_chest 55484
   set vnum_reward_legs 55485
   set vnum_reward_boots 55486
   set vnum_reward_bracer 55487
   set vnum_reward_gloves 55488
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
       set exp_multiplier 7
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 5
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 6
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 4
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
mexp %actor% 29880
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
mexp %actor% 29880
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
#55431
phase_2_warrior_greet~
0 g 100
~
wait 2
if %actor.class% == warrior && %actor.level% >= 21
      if %actor.quest_stage[phase_armor]% == 1 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some &7&barmor quests&0?"
         msend %actor% %self.name% tells you, "If so, just ask me, &7&bYes&0 I would like to do some &7&barmor quests&0"
   endif
else
endif
~
#55432
Phase_2_warrior_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == warrior
   wait 2
   if %actor.quest_stage[phase_armor]% == 1
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice &7&bboots&0, a &7&bhelm&0, &7&bgauntlets&0,"
   msend %actor% %self.name% tells you, "&7&bvambraces&0, &7&bgreaves&0, &7&bplate&0, and a &7&bbracer&0."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for warriors only."
endif
~
#55433
phase_2_warrior_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for warriors only
if %actor.class% == Warrior && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
wait 2
msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
msend %actor% %self.name% tells you, "warrior types but I'll need 3 quantities of some opal dust, and"
msend %actor% %self.name% tells you, "a set of Crushed Plate Boots. Return these things to me in any"
msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
msend %actor% Sorry this quest is for warriors only.
endif
~
#55434
phase_2_warrior_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the warrior types"
   msend %actor% %self.name% tells you, "but I'll need 3 quantities of some emerald dust, and a Crushed"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55435
phase_2_warrior_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "warrior types but I'll need 3 quantities of some sapphire dust, and"
   msend %actor% %self.name% tells you, "a set of Crushed Gauntlets. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55436
phase_2_warrior_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the warrior types but I'll need 3 quantities of some diamond dust, and"
   msend %actor% %self.name% tells you, "a set of Crushed Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55437
phase_2_warrior_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the warrior types but I'll need 3 perfect amythests, and"
   msend %actor% %self.name% tells you, "a set of Crushed Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55438
phase_2_warrior_plate~
0 n 100
plate~
*
* Plate ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine plate for the warriors but"
   msend %actor% %self.name% tells you, "I'll need 3 emerald shards, and a Crushed Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55439
phase_2_warrior_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for warriors only
if %actor.class% == warrior && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "warrior types but I'll need 3 uncut aquamarines, and a Crushed Plate"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55440
warrior_phase_2~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class warrior
if %actor.class% == %class% && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55328
   set vnum_destroyed_boots	55332
   set vnum_destroyed_bracer	55336
   set vnum_destroyed_helm	55340
   set vnum_destroyed_arm	55344
   set vnum_destroyed_legs	55348
   set vnum_destroyed_chest	55352
   * gems for this class
   set vnum_gem_gloves	55604
   set vnum_gem_boots	55615
   set vnum_gem_bracer	55626
   set vnum_gem_helm	55637
   set vnum_gem_arm	55648
   set vnum_gem_legs	55659
   set vnum_gem_chest	55670
   * rewards for this class
   set vnum_reward_helm 55412
   set vnum_reward_arms 55413
   set vnum_reward_chest 55414
   set vnum_reward_legs 55415
   set vnum_reward_boots 55416
   set vnum_reward_bracer 55417
   set vnum_reward_gloves 55418
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
       set exp_multiplier 7
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 5
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 6
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 4
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
mexp %actor% 29880
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
mexp %actor% 29880
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
#55441
phase_2_sorcerer_greet~
0 g 100
~
wait 2
* Set up test mud to test if these parenthesis work on the conditionals.. They do!!
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 21
   if %actor.quest_stage[phase_armor]% == 0
      quest start phase_armor %actor.name%
   endif
   if %actor.quest_stage[phase_armor]% <= 1 
      wait 2
      msend %actor% %self.name% tells you, "Welcome, would you like to do some &7&barmor quests&0?"
      msend %actor% %self.name% tells you, "If so, just ask me, &7&bYes&0 I would like to do some &7&barmor quests&0"     
   endif
else
endif
~
#55442
Phase_2_sorcerer_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer)
   wait 2
   if %actor.quest_stage[phase_armor]% == 1
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice &7&bslippers&0, a &7&bcoif&0, &7&bgloves&0,"
   msend %actor% %self.name% tells you, "&7&bsleeves&0, &7&bpants&0, &7&brobe&0, and a &7&bbracelet&0."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for sorcerers only."
endif
~
#55443
phase_2_sorcerer_boots~
0 n 100
slippers~
*
* Boots ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of slippers for the"
   msend %actor% %self.name% tells you, "sorcerer types but I'll need 3 uncut sapphires, and"
   msend %actor% %self.name% tells you, "a set of Burned Slippers. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55444
phase_2_sorcerer_helm~
0 n 100
coif~
*
* helm ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A skullcap for ya? Well now I can make a nice coif for the sorcerer types"
   msend %actor% %self.name% tells you, "but I'll need 3 enchanted amythests, and a Burned Turban."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55445
phase_2_sorcerer_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "sorcerer types but I'll need 3 uncut citrines, and a set"
   msend %actor% %self.name% tells you, "of Burned Mittens. Return these things to me in any order at any"
   msend %actor% %self.name% tells you, "time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55446
phase_2_sorcerer_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the sorcerer types but I'll need 3 uncut diamonds, and"
   msend %actor% %self.name% tells you, "a set of Burned Sleeves. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55447
phase_2_sorcerer_greaves~
0 n 100
pants~
*
* Greaves ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "pants for the sorcerer types but I'll need 3 radiant chrysoberyls,"
   msend %actor% %self.name% tells you, "and a set of Burned Leggings. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55448
phase_2_sorcerer_plate~
0 n 100
robe~
*
* Plate ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine robe for the sorcerers but"
   msend %actor% %self.name% tells you, "I'll need 3 flawed emeralds, and a Burned Robe. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55449
phase_2_sorcerer_bracer~
0 n 100
bracelet~
*
* Bracer ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracelet for the"
   msend %actor% %self.name% tells you, "sorcerer types but I'll need 3 uncut pearls, and a"
   msend %actor% %self.name% tells you, "Burned Wristband. Return these things to me in any order at any"
   msend %actor% %self.name% tells you, "time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55450
sorcerer_phase_2~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) &%actor.level% >= 21 &%actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves55331
   set vnum_destroyed_boots55335
   set vnum_destroyed_bracer55339
   set vnum_destroyed_helm55343
   set vnum_destroyed_arm55347
   set vnum_destroyed_legs55351
   set vnum_destroyed_chest55355
   * gems for this class
   set vnum_gem_gloves55602
   set vnum_gem_boots55613
   set vnum_gem_bracer55624
   set vnum_gem_helm55631
   set vnum_gem_arm55646
   set vnum_gem_legs55657
   set vnum_gem_chest55668
   * rewards for this class
   set vnum_reward_helm 55475
   set vnum_reward_arms 55476
   set vnum_reward_chest 55477
   set vnum_reward_legs 55478
   set vnum_reward_boots 55479
   set vnum_reward_bracer 55480
   set vnum_reward_gloves 55481
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
       set exp_multiplier 7
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 8
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 9
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 5
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 6
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 4
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
      If %gems% == 3 &%actor.quest_variable[phase_armor:%vnum_armor%_armor_acquired]% == 1
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
mexp %actor% 29880
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
      If %armor% == 1 &%actor.quest_variable[phase_armor:%vnum_gem%_gems_acquired]% == 3
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
mexp %actor% 29880
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
#55451
phase_3_anti-paladin_greet~
0 g 100
~
wait 2
if %actor.class% /= Anti && %actor.level% >= 41
      if %actor.quest_stage[phase_armor]% == 2 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some armor quests?"
         msend %actor% %self.name% tells you, "If so, just ask me, Yes I would like to do some armor quests"
   endif
else
endif
~
#55452
Phase_3_anti-paladin_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% /= Anti
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
   msend %actor% %self.name% tells you, "Sorry, this quest is for anti-paladins only."
endif
~
#55453
phase_3_anti-paladin_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "anti-paladin types but I'll need 3 handfulls of pearls, and a set of"
   msend %actor% %self.name% tells you, "Tarnished Plate Boots. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55454
phase_3_anti-paladin_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the anti-paladin types"
   msend %actor% %self.name% tells you, "but I'll need 3 radiant topazs, and a Tarnished"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55455
phase_3_anti-paladin_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "anti-paladin types but I'll need 3 chrysoberyls, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Gauntlets. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55456
phase_3_anti-paladin_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the anti-paladin types but I'll need 3 perfect aquamarines, and a set of"
   msend %actor% %self.name% tells you, "Tarnished Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55457
phase_3_anti-paladin_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the anti-paladin types but I'll need 3 radiant opals, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55458
phase_3_anti-paladin_plate~
0 n 100
plate~
*
* Plate ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine plate for the anti-paladins but"
   msend %actor% %self.name% tells you, "I'll need 3 enchanted emeralds, and a Tarnished Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55459
phase_3_anti-paladin_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "anti-paladin types but I'll need 3 handfulls of jade, and a Tarnished Plate"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55460
anti-paladin_phase_3~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class anti-paladin
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
   set vnum_gem_gloves	55671
   set vnum_gem_boots	55682
   set vnum_gem_bracer	55693
   set vnum_gem_helm	55704
   set vnum_gem_arm	55715
   set vnum_gem_legs	55726
   set vnum_gem_chest	55737
   * rewards for this class
   set vnum_reward_helm 55503
   set vnum_reward_arms 55504
   set vnum_reward_chest 55505
   set vnum_reward_legs 55506
   set vnum_reward_boots 55507
   set vnum_reward_bracer 55508
   set vnum_reward_gloves 55509
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
#55461
phase_3_cleric_greet~
0 g 99
~
wait 2
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 41
      if %actor.quest_stage[phase_armor]% == 2 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some armor quests?"
         msend %actor% %self.name% tells you, "If so, just ask me, Yes I would like to do some armor quests"
   endif
else
endif
~
#55462
Phase_3_cleric_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if (%actor.class% == cleric || %actor.class% == priest)
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
   msend %actor% %self.name% tells you, "Sorry, this quest is for clerics only."
endif
~
#55463
phase_3_cleric_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "cleric types but I'll need 3 flawed sapphires, and a set of"
   msend %actor% %self.name% tells you, "Tarnished Plate Boots. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55464
phase_3_cleric_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the cleric types"
   msend %actor% %self.name% tells you, "but I'll need 3 handfulls of topazes, and a Tarnished"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55465
phase_3_cleric_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "cleric types but I'll need 3 garnet shards, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Gauntlets. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55466
phase_3_cleric_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the cleric types but I'll need 3 flawed opals, and a set of"
   msend %actor% %self.name% tells you, "Tarnished Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55467
phase_3_cleric_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the cleric types but I'll need 3 handfulls of opals, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55468
phase_3_cleric_plate~
0 n 100
plate~
*
* Plate ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine plate for the clerics but"
   msend %actor% %self.name% tells you, "I'll need 3 perfect rubies, and a Tarnished Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55469
phase_3_cleric_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "cleric types but I'll need 3 aquamarine shards, and a Tarnished Plate"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55470
**UNUSED**~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
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
   set vnum_gem_gloves	55678
   set vnum_gem_boots	55689
   set vnum_gem_bracer	55700
   set vnum_gem_helm	55711
   set vnum_gem_arm	55722
   set vnum_gem_legs	55733
   set vnum_gem_chest	55744
   * rewards for this class
   set vnum_reward_helm 55510
   set vnum_reward_arms 55511
   set vnum_reward_chest 55512
   set vnum_reward_legs 55513
   set vnum_reward_boots 55514
   set vnum_reward_bracer 55515
   set vnum_reward_gloves 55516
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
#55471
phase_3_diabolist_greet~
0 g 100
~
wait 2
if %actor.class% == diabolist && %actor.level% >= 41
      if %actor.quest_stage[phase_armor]% == 2 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some armor quests?"
         msend %actor% %self.name% tells you, "If so, just ask me, Yes I would like to do some armor quests"
   endif
else
endif
~
#55472
Phase_3_diabolist_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == diabolist
   wait 2
   if %actor.quest_stage[phase_armor]% == 2
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice boots, a helm, gauntlets,"
   msend %actor% %self.name% tells you, "vambraces, greaves, chestguard, and a bracer."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me status at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for diabolists only."
endif
~
#55473
phase_3_diabolist_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "diabolist types but I'll need 3 cursed pearls, and a set of"
   msend %actor% %self.name% tells you, "Tarnished Plate Boots. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55474
phase_3_diabolist_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the diabolist types"
   msend %actor% %self.name% tells you, "but I'll need 3 cursed aquamarines, and a Tarnished"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55475
phase_3_diabolist_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "diabolist types but I'll need 3 perfect tourmalines, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Gauntlets. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55476
phase_3_diabolist_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the diabolist types but I'll need 3 cursed garnets, and a set of"
   msend %actor% %self.name% tells you, "Tarnished Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55477
phase_3_diabolist_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the diabolist types but I'll need 3 cursed sapphires, and"
   msend %actor% %self.name% tells you, "a set of Tarnished Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55478
phase_3_diabolist_plate~
0 n 100
chest chestguard~
*
* Plate ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine chestguard for the diabolists but"
   msend %actor% %self.name% tells you, "I'll need 3 cursed diamonds, and a Tarnished Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55479
phase_3_diabolist_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "diabolist types but I'll need 3 enchanted citrines, and a Tarnished"
   msend %actor% %self.name% tells you, "Plate Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55480
diabolist_phase_3~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class diabolist
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
   set vnum_gem_gloves	55672
   set vnum_gem_boots	55683
   set vnum_gem_bracer	55694
   set vnum_gem_helm	55705
   set vnum_gem_arm	55716
   set vnum_gem_legs	55727
   set vnum_gem_chest	55738
   * rewards for this class
   set vnum_reward_helm 55545
   set vnum_reward_arms 55546
   set vnum_reward_chest 55547
   set vnum_reward_legs 55548
   set vnum_reward_boots 55549
   set vnum_reward_bracer 55550
   set vnum_reward_gloves 55551
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
#55481
phase_3_druid_greet~
0 g 100
~
wait 2
if %actor.class% == druid && %actor.level% >= 41
      if %actor.quest_stage[phase_armor]% == 2 
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some armor quests?"
         msend %actor% %self.name% tells you, "If so, just ask me, Yes I would like to do some armor quests"
   endif
else
endif
~
#55482
Phase_3_druid_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == druid
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
   msend %actor% %self.name% tells you, "Sorry, this quest is for druids only."
endif
~
#55483
phase_3_druid_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "druid types but I'll need 3 perfect pearls, and"
   msend %actor% %self.name% tells you, "a set of Corroded Boots. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55484
phase_3_druid_helm~
0 n 100
cap~
*
* helm ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A cap for ya? Well now I can make a nice cap for the druid types"
   msend %actor% %self.name% tells you, "but I'll need 3 enchanted topazs, and a Corroded Cap."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55485
phase_3_druid_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "druid types but I'll need 3 handfulls of tourmalines, and a"
   msend %actor% %self.name% tells you, "set of Corroded Gloves. Return these things to me in any order"
   msend %actor% %self.name% tells you, "at any time and I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55486
phase_3_druid_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the druid types but I'll need 3 radiant garnets, and"
   msend %actor% %self.name% tells you, "a set of Corroded Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55487
phase_3_druid_greaves~
0 n 100
pants~
*
* Greaves ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "pants for the druid types but I'll need 3 enchanted opals,"
   msend %actor% %self.name% tells you, "and a set of Corroded Pants. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55488
phase_3_druid_plate~
0 n 100
jerkin~
*
* Plate ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine jerkin for the druids but"
   msend %actor% %self.name% tells you, "I'll need 3 enchanted diamonds, and a Corroded Jerkin. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55489
phase_3_druid_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "druid types but I'll need 3 perfect citrines, and a Corroded Wristband."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55490
druid_phase_3~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class druid
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
   set vnum_gem_gloves	55673
   set vnum_gem_boots	55684
   set vnum_gem_bracer	55695
   set vnum_gem_helm	55706
   set vnum_gem_arm	55717
   set vnum_gem_legs	55728
   set vnum_gem_chest	55739
   * rewards for this class
   set vnum_reward_helm 55524
   set vnum_reward_arms 55525
   set vnum_reward_chest 55526
   set vnum_reward_legs 55527
   set vnum_reward_boots 55528
   set vnum_reward_bracer 55529
   set vnum_reward_gloves 55530
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
#55491
phase_3_sorcerer_greet~
0 g 100
~
wait 2
* Set up test mud to test if these parenthesis work on the conditionals.. They Do!!
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 41
   if %actor.quest_stage[phase_armor]% == 2 
      wait 2
      msend %actor% %self.name% tells you, "Welcome, would you like to do some armor quests?"
      msend %actor% %self.name% tells you, "If so, just ask me, Yes I would like to do some armor quests"     
   endif
else
endif
~
#55492
Phase_3_sorcerer_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer
   wait 2
   if %actor.quest_stage[phase_armor]% == 2
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice slippers, a coif, gloves,"
   msend %actor% %self.name% tells you, "sleeves, pants, robe, and a bracelet."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me status at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for sorcerers only."
endif
~
#55493
phase_3_sorcerer_boots~
0 n 100
slippers~
*
* Boots ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of slippers for the"
   msend %actor% %self.name% tells you, "sorcerer types but I'll need 3 flawed topazes, and"
   msend %actor% %self.name% tells you, "a set of Worn Slippers. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55494
phase_3_sorcerer_helm~
0 n 100
coif~
*
* helm ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A skullcap for ya? Well now I can make a nice coif for the sorcerer types"
   msend %actor% %self.name% tells you, "but I'll need 3 handfulls of aquamarines, and a Worn Turban."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55495
phase_3_sorcerer_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "sorcerer types but I'll need 3 flawed pearls, and a set"
   msend %actor% %self.name% tells you, "of Worn Mittens. Return these things to me in any order at any"
   msend %actor% %self.name% tells you, "time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55496
phase_3_sorcerer_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the sorcerer types but I'll need 3 topazes, and"
   msend %actor% %self.name% tells you, "a set of Worn Sleeves. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55497
phase_3_sorcerer_greaves~
0 n 100
pants~
*
* Greaves ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "pants for the sorcerer types but I'll need 3 handfulls of sapphires,"
   msend %actor% %self.name% tells you, "and a set of Worn Leggings. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55498
phase_3_sorcerer_plate~
0 n 100
robe~
*
* Plate ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine robe for the sorcerers but"
   msend %actor% %self.name% tells you, "I'll need 3 radiant emeralds, and a Worn Robe. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55499
phase_3_sorcerer_bracer~
0 n 100
bracelet~
*
* Bracer ya ask?
*
* This is for sorcerers only
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && %actor.level% >= 41 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracelet for the"
   msend %actor% %self.name% tells you, "sorcerer types but I'll need 3 flawed aquamarines, and a"
   msend %actor% %self.name% tells you, "Worn Bracelet. Return these things to me in any order at any"
   msend %actor% %self.name% tells you, "time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
$~
