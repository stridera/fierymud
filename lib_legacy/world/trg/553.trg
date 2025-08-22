#55301
cleric_phase_1~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
eval CLERIC_SUB (%actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist || %actor.class% == druid)
if %CLERIC_SUB% && %actor.quest_stage[phase_armor]% >= 1
   *
   * pertinient object definitions for this class
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
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 17
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 29
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 24
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 4
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 7
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 2
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
            mexp %actor% 2640
            eval lap %lap% + 1
         done
         *
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
            mexp %actor% 2640
            eval lap %lap% + 1
         done
         *
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
#55302
phase_1_cleric_greet~
0 g 100
~
* This is for clerics only
eval CLERIC_SUB (%actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist || %actor.class% == druid)
if %CLERIC_SUB%
   if %actor.quest_stage[phase_armor]% == 0  
      wait2
      msend %actor% %self.name% tells you, "Welcome, would you like to do some [&7&barmor quests&0]?"
      msend %actor% %self.name% tells you, "If so, just ask me, &7&bYes&0 I would like to do some &7&barmor quests&0"
   endif
else
endif
~
#55303
Phase_1_cleric_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a cleric and set the quest variable for later interaction.
*
* This is for clerics only
eval CLERIC_SUB (%actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist || %actor.class% == druid)
if %CLERIC_SUB%
   wait 2
   if %actor.quest_stage[phase_armor]% == 0
      quest start phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice [&7&bboots&0], a [&7&bhelm&0], [&7&bgauntlets&0],"
   msend %actor% %self.name% tells you, "[&7&bvambraces&0], [&7&bgreaves&0], [&7&bplate&0], and a [&7&bbracer&0]."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Also, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for clerics only."
endif
~
#55304
phase_1_cleric_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for clerics only
set CLERIC_SUB (%actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist || %actor.class% == druid)
if %CLERIC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "cleric types but I'll need 3 crushed aquamarines, and a set of"
   msend %actor% %self.name% tells you, "Rusted Plate Boots. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55305
phase_1_cleric_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for clerics only
eval CLERIC_SUB (%actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist || %actor.class% == druid)
if %CLERIC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the cleric types"
   msend %actor% %self.name% tells you, "but I'll need 3 quantities of some chrysoberyl dust, and a Rusted"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55306
phase_1_cleric_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for clerics only
eval CLERIC_SUB (%actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist || %actor.class% == druid)
if %CLERIC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "cleric types but I'll need 3 crushed ambers, and a set of"
   msend %actor% %self.name% tells you, "Rusted Gauntlets. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55307
phase_1_cleric_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for clerics only
eval CLERIC_SUB (%actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist || %actor.class% == druid)
if %CLERIC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the cleric types but I'll need 3 quantities of some lapis-lazuli"
   msend %actor% %self.name% tells you, "dust, and a set of Rusted Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55308
phase_1_cleric_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for clerics only
eval CLERIC_SUB (%actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist || %actor.class% == druid)
if %CLERIC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the cleric types but I'll need 3 quantities of some"
   msend %actor% %self.name% tells you, "garnet dust, and a set of Rusted Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55309
phase_1_cleric_plate~
0 n 100
plate~
*
* Plate ya ask?
*
* This is for clerics only
eval CLERIC_SUB (%actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist || %actor.class% == druid)
if %CLERIC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine plate for the clerics but"
   msend %actor% %self.name% tells you, "I'll need 3 crushed rubies, and a Rusted Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55310
phase_1_cleric_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for clerics only
eval CLERIC_SUB (%actor.class% == cleric || %actor.class% == priest || %actor.class% == diabolist || %actor.class% == druid)
if %CLERIC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "cleric types but I'll need 3 crushed jades, and a Rusted Plate"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55311
phase_1_warrior_greet~
0 g 100
~
wait 2
set anti Anti-Paladin
eval WARRIOR_SUB (%actor.class% == warrior || %actor.class% == paladin || %actor.class% == %anti% || %actor.class% == ranger || %actor.class% == monk)
if %WARRIOR_SUB%
      if %actor.quest_stage[phase_armor]% == 0  
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some [&7&barmor quests&0]?"
         msend %actor% %self.name% tells you, "If so, just ask me, &7&bYes&0 I would like to do some &7&barmor quests&0"
   endif
else
endif
~
#55312
Phase_1_warrior_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
set anti Anti-Paladin
eval WARRIOR_SUB (%actor.class% == warrior || %actor.class% == paladin || %actor.class% == %anti% || %actor.class% == ranger || %actor.class% == monk)
if %WARRIOR_SUB%
   wait 2
   if %actor.quest_stage[phase_armor]% == 0
      quest start phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice [&7&bboots&0], a [&7&bhelm&0], [&7&bgauntlets&0],"
   msend %actor% %self.name% tells you, "[&7&bvambraces&0], [&7&bgreaves&0], [&7&bplate&0], and a [&7&bbracer&0]."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Also, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for warriors only."
endif
~
#55313
phase_1_warrior_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for warriors only
set anti Anti-Paladin
set WARRIOR_SUB (%actor.class% == warrior || %actor.class% == paladin || %actor.class% == %anti% || %actor.class% == ranger || %actor.class% == monk)
if %WARRIOR_SUB%
   if %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "warrior types but I'll need 3 crushed topazes, and a set of"
   msend %actor% %self.name% tells you, "Rusted Plate Boots. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55314
phase_1_warrior_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for warriors only
set anti Anti-Paladin
set WARRIOR_SUB (%actor.class% == warrior || %actor.class% == paladin || %actor.class% == %anti% || %actor.class% == ranger || %actor.class% == monk)
if %WARRIOR_SUB%
   if %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the warrior types"
   msend %actor% %self.name% tells you, "but I'll need 3 quantities of some jade dust, and a Rusted"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55315
phase_1_warrior_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for warriors only
set anti Anti-Paladin
set WARRIOR_SUB (%actor.class% == warrior || %actor.class% == paladin || %actor.class% == %anti% || %actor.class% == ranger || %actor.class% == monk)
if %WARRIOR_SUB%
   if %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "warrior types but I'll need 3 crushed amethysts, and a set of"
   msend %actor% %self.name% tells you, "Rusted Gauntlets. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55316
phase_1_warrior_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for warriors only
set anti Anti-Paladin
set WARRIOR_SUB (%actor.class% == warrior || %actor.class% == paladin || %actor.class% == %anti% || %actor.class% == ranger || %actor.class% == monk)
if %WARRIOR_SUB%
   if %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the warrior types but I'll need 3 quantities of some amber dust,"
   msend %actor% %self.name% tells you, "and a set of Rusted Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55317
phase_1_warrior_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for warriors only
set anti Anti-Paladin
set WARRIOR_SUB (%actor.class% == warrior || %actor.class% == paladin || %actor.class% == %anti% || %actor.class% == ranger || %actor.class% == monk)
if %WARRIOR_SUB%
   if %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the warrior types but I'll need 3 quantities of some"
   msend %actor% %self.name% tells you, "aquamarine dust, and a set of Rusted Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55318
phase_1_warrior_plate~
0 n 100
plate~
*
* Plate ya ask?
*
* This is for warriors only
set anti Anti-Paladin
set WARRIOR_SUB (%actor.class% == warrior || %actor.class% == paladin || %actor.class% == %anti% || %actor.class% == ranger || %actor.class% == monk)
if %WARRIOR_SUB%
   if %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine plate for the warriors but"
   msend %actor% %self.name% tells you, "I'll need 3 crushed emeralds, and a Rusted Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55319
phase_1_warrior_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for warriors only
set anti Anti-Paladin
set WARRIOR_SUB (%actor.class% == warrior || %actor.class% == paladin || %actor.class% == %anti% || %actor.class% == ranger || %actor.class% == monk)
if %WARRIOR_SUB%
   if %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "warrior types but I'll need 3 crushed citrines, and a Rusted Plate"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for warriors only.
endif
~
#55320
warrior_phase_1~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set anti Anti-Paladin
set WARRIOR_SUB (%actor.class% == warrior || %actor.class% == paladin || %actor.class% == %anti% || %actor.class% == ranger || %actor.class% == monk)
if %WARRIOR_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55300
   set vnum_destroyed_boots	55304
   set vnum_destroyed_bracer	55308
   set vnum_destroyed_helm	55312
   set vnum_destroyed_arm	55316
   set vnum_destroyed_legs	55320
   set vnum_destroyed_chest	55324
   * gems for this class
   set vnum_gem_gloves	55569
   set vnum_gem_boots	55573
   set vnum_gem_bracer	55577
   set vnum_gem_helm	55581
   set vnum_gem_arm	55585
   set vnum_gem_legs	55589
   set vnum_gem_chest	55593
   * rewards for this class
   set vnum_reward_helm 55384
   set vnum_reward_arms 55385
   set vnum_reward_chest 55386
   set vnum_reward_legs 55387
   set vnum_reward_boots 55388
   set vnum_reward_bracer 55389
   set vnum_reward_gloves 55390
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
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 17
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 29
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 24
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 4
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 7
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 2
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
mexp %actor% 2640
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
mexp %actor% 2640
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
#55321
phase_1_sorcerer_greet~
0 g 100
~
set SORC_SUB (%actor.class% == sorcerer || %actor.class% == necromancer || %actor.class% == conjurer || %actor.class% == cryomancer || %actor.class% == pyromancer) 
if %SORC_SUB%
      if %actor.quest_stage[phase_armor]% == 0  
	 wait 2
         msend %actor% %self.name% tells you, "Welcome, would you like to do some [&7&barmor quests&0]?"
         msend %actor% %self.name% tells you, "If so, just ask me, &7&bYes&0 I would like to do some &7&barmor quests&0"
   endif
else
endif
~
#55322
Phase_1_sorcerer_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
set SORC_SUB (%actor.class% == sorcerer || %actor.class% == necromancer || %actor.class% == conjurer || %actor.class% == cryomancer || %actor.class% == pyromancer) 
if %SORC_SUB%
   wait 2
   if %actor.quest_stage[phase_armor]% == 0
      quest start phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice [&7&bsandals&0], a [&7&bturban&0], [&7&bgloves&0],"
   msend %actor% %self.name% tells you, "[&7&bsleeves&0], [&7&bpants&0], [&7&btunic&0], and a [&7&bbracelet&0]."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Also, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for sorcerers only."
endif
~
#55323
phase_1_sorcerer_boots~
0 n 100
sandals~
*
* Sandals/slippers ya ask?
*
* This is for sorcerers only
set SORC_SUB (%actor.class% == sorcerer || %actor.class% == necromancer || %actor.class% == conjurer || %actor.class% == cryomancer || %actor.class% == pyromancer) 
if %SORC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Sandals ya ask? Well now I can make a fine pair of sandals for the"
   msend %actor% %self.name% tells you, "sorcerer types but I'll need 3 crushed garnets, and a set of"
   msend %actor% %self.name% tells you, "Decayed Slippers. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55324
phase_1_sorcerer_helm~
0 n 100
turban~
*
* helm ya ask?
*
* This is for sorcerers only
set SORC_SUB (%actor.class% == sorcerer || %actor.class% == necromancer || %actor.class% == conjurer || %actor.class% == cryomancer || %actor.class% == pyromancer) 
if %SORC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Turban ya ask? Well now I can make a nice turban for the sorcerer types"
   msend %actor% %self.name% tells you, "but I'll need 3 quantities of some tourmaline dust, and a Decayed"
   msend %actor% %self.name% tells you, "Turban. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55325
phase_1_sorcerer_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for sorcerers only
set SORC_SUB (%actor.class% == sorcerer || %actor.class% == necromancer || %actor.class% == conjurer || %actor.class% == cryomancer || %actor.class% == pyromancer) 
if %SORC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Mittens? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "sorcerer types but I'll need 3 crushed lapis-lazulis, and a set of"
   msend %actor% %self.name% tells you, "Decayed Gloves. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55326
phase_1_sorcerer_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for sorcerers only
set SORC_SUB (%actor.class% == sorcerer || %actor.class% == necromancer || %actor.class% == conjurer || %actor.class% == cryomancer || %actor.class% == pyromancer) 
if %SORC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the sorcerer types but I'll need 3 quantities of some turquoise dust,"
   msend %actor% %self.name% tells you, "and a set of Decayed Sleeves. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55327
phase_1_sorcerer_greaves~
0 n 100
pants~
*
* Greaves ya ask?
*
* This is for sorcerers only
set SORC_SUB (%actor.class% == sorcerer || %actor.class% == necromancer || %actor.class% == conjurer || %actor.class% == cryomancer || %actor.class% == pyromancer) 
if %SORC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "pants for the sorcerer types but I'll need 3 quantities of some"
   msend %actor% %self.name% tells you, "pearl dust, and a set of Decayed Leggings. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55328
phase_1_sorcerer_plate~
0 n 100
tunic~
*
* Plate ya ask?
*
* This is for sorcerers only
set SORC_SUB (%actor.class% == sorcerer || %actor.class% == necromancer || %actor.class% == conjurer || %actor.class% == cryomancer || %actor.class% == pyromancer) 
if %SORC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine tunic for the sorcerers but"
   msend %actor% %self.name% tells you, "I'll need 3 crushed diamonds, and a Decayed Robe. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55329
phase_1_sorcerer_bracer~
0 n 100
bracelet~
*
* Bracer ya ask?
*
* This is for sorcerers only
set SORC_SUB (%actor.class% == sorcerer || %actor.class% == necromancer || %actor.class% == conjurer || %actor.class% == cryomancer || %actor.class% == pyromancer) 
if %SORC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Bracelet ya ask? Well now I can make a protective bracelet for the"
   msend %actor% %self.name% tells you, "sorcerer types but I'll need 3 crushed chrysoberyls, and a Decayed"
   msend %actor% %self.name% tells you, "Bracelet. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for sorcerers only.
endif
~
#55330
sorcerer_phase_1~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set SORC_SUB (%actor.class% == sorcerer || %actor.class% == necromancer || %actor.class% == conjurer || %actor.class% == cryomancer || %actor.class% == pyromancer) 
if %SORC_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55302
   set vnum_destroyed_boots	55306
   set vnum_destroyed_bracer	55310
   set vnum_destroyed_helm	55314
   set vnum_destroyed_arm	55318
   set vnum_destroyed_legs	55322
   set vnum_destroyed_chest	55326
   * gems for this class
   set vnum_gem_gloves	55567
   set vnum_gem_boots	55571
   set vnum_gem_bracer	55575
   set vnum_gem_helm	55579
   set vnum_gem_arm	55583
   set vnum_gem_legs	55587
   set vnum_gem_chest	55591
   * rewards for this class
   set vnum_reward_helm 55398
   set vnum_reward_arms 55399
   set vnum_reward_chest 55400
   set vnum_reward_legs 55401
   set vnum_reward_boots 55402
   set vnum_reward_bracer 55403
   set vnum_reward_gloves 55404
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
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 17
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 29
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 24
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 4
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 7
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 2
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
mexp %actor% 2640
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
mexp %actor% 2640
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
#55331
phase_1_rogue_greet~
0 g 100
~
set ROG_SUB (%actor.class% == rogue || %actor.class% == assassin || %actor.class% == thief || %actor.class% == mercenary)
if %ROG_SUB%
   if %actor.quest_stage[phase_armor]% == 0  
      wait 2
      msend %actor% %self.name% tells you, "Welcome, would you like to do some [&7&barmor quests&0]?"
      msend %actor% %self.name% tells you, "If so, just ask me, &7&bYes&0 I would like to do some &7&barmor quests&0"
   endif
else
endif
~
#55332
Phase_1_rogue_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
set ROG_SUB (%actor.class% == rogue || %actor.class% == assassin || %actor.class% == thief || %actor.class% == mercenary)
if %ROG_SUB%
   wait 2
   if %actor.quest_stage[phase_armor]% == 0
      quest start phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice [&7&bboots&0], a [&7&bcoif&0], [&7&bgloves&0],"
   msend %actor% %self.name% tells you, "[&7&bsleeves&0], [&7&bleggings&0], [&7&btunic&0], and a [&7&bbracer&0]."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Also, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for rogues only."
endif
~
#55333
phase_1_rogue_boots~
0 n 100
boots~
*
* Sandals/slippers ya ask?
*
* This is for rogues only
set ROG_SUB (%actor.class% == rogue || %actor.class% == assassin || %actor.class% == thief || %actor.class% == mercenary)
if %ROG_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "rogue types but I'll need 3 crushed pearls, and a set of Rusted"
   msend %actor% %self.name% tells you, "Chain Boots. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55334
phase_1_rogue_helm~
0 n 100
coif~
*
* helm ya ask?
*
* This is for rogues only
set ROG_SUB (%actor.class% == rogue || %actor.class% == assassin || %actor.class% == thief || %actor.class% == mercenary)
if %ROG_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Headgear ya ask? Well now I can make a nice chain coif for the rogue types"
   msend %actor% %self.name% tells you, "but I'll need 3 quantities of some amythest dust, and a Rusted Coif."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55335
phase_1_rogue_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for rogues only
set ROG_SUB (%actor.class% == rogue || %actor.class% == assassin || %actor.class% == thief || %actor.class% == mercenary)
if %ROG_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "rogue types but I'll need 3 crushed turquoises, and a set of"
   msend %actor% %self.name% tells you, "Rusted Chain Gloves. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55336
phase_1_rogue_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for rogues only
set ROG_SUB (%actor.class% == rogue || %actor.class% == assassin || %actor.class% == thief || %actor.class% == mercenary)
if %ROG_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the rogue types but I'll need 3 crushed sapphires,"
   msend %actor% %self.name% tells you, "and a set of Rusted Sleeves. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55337
phase_1_rogue_greaves~
0 n 100
leggings~
*
* Greaves ya ask?
*
* This is for rogues only
set ROG_SUB (%actor.class% == rogue || %actor.class% == assassin || %actor.class% == thief || %actor.class% == mercenary)
if %ROG_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "pants for the rogue types but I'll need 3 quantities of some"
   msend %actor% %self.name% tells you, "citrine dust, and a set of Rusted Leggings. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55338
phase_1_rogue_plate~
0 n 100
tunic~
*
* Plate ya ask?
*
* This is for rogues only
set ROG_SUB (%actor.class% == rogue || %actor.class% == assassin || %actor.class% == thief || %actor.class% == mercenary)
if %ROG_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine tunic for the rogues but"
   msend %actor% %self.name% tells you, "I'll need 3 crushed opals, and a Rusted Tunic. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55339
phase_1_rogue_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for rogues only
set ROG_SUB (%actor.class% == rogue || %actor.class% == assassin || %actor.class% == thief || %actor.class% == mercenary)
if %ROG_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective wristgard for the"
   msend %actor% %self.name% tells you, "rogue types but I'll need 3 crushed tourmalines, and a Rusted"
   msend %actor% %self.name% tells you, "Chain Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for rogues only.
endif
~
#55340
rogue_phase_1~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set ROG_SUB (%actor.class% == rogue || %actor.class% == assassin || %actor.class% == thief || %actor.class% == mercenary)
if %ROG_SUB% && %actor.level% >= 1 && %actor.quest_stage[phase_armor]% >= 1
   *
   * pertinient object definitions for this class
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
       set exp_multiplier 10
       set vnum_armor %vnum_destroyed_helm%
       set vnum_gem %vnum_gem_helm%
       set vnum_reward %vnum_reward_helm%
       break
     case %vnum_destroyed_arm%
       set is_armor 1
     case %vnum_gem_arm%
       set exp_multiplier 17
       set vnum_armor %vnum_destroyed_arm%
       set vnum_gem %vnum_gem_arm%
       set vnum_reward %vnum_reward_arms%
       break
     case %vnum_destroyed_chest%
       set is_armor 1
     case %vnum_gem_chest%
       set exp_multiplier 29
       set vnum_armor %vnum_destroyed_chest%
       set vnum_gem %vnum_gem_chest%
       set vnum_reward %vnum_reward_chest%
       break
     case %vnum_destroyed_legs%
       set is_armor 1
     case %vnum_gem_legs%
       set exp_multiplier 24
       set vnum_armor %vnum_destroyed_legs%
       set vnum_gem %vnum_gem_legs%
       set vnum_reward %vnum_reward_legs%
       break
     case %vnum_destroyed_boots%
       set is_armor 1
     case %vnum_gem_boots%
       set exp_multiplier 4
       set vnum_armor %vnum_destroyed_boots%
       set vnum_gem %vnum_gem_boots%
       set vnum_reward %vnum_reward_boots%
       break
     case %vnum_destroyed_bracer%
       set is_armor 1
     case %vnum_gem_bracer%
       set exp_multiplier 7
       set vnum_armor %vnum_destroyed_bracer%
       set vnum_gem %vnum_gem_bracer%
       set vnum_reward %vnum_reward_bracer%
       break
     case %vnum_destroyed_gloves%
       set is_armor 1
     case %vnum_gem_gloves%
       set exp_multiplier 2
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
mexp %actor% 2640
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
mexp %actor% 2640
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
#55341
phase_2_anti-paladin_greet~
0 g 100
~
wait 2
if %actor.class% /= Anti && %actor.level% >= 21
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
#55342
Phase_2_anti-paladin_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% /= Anti
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
   msend %actor% %self.name% tells you, "Sorry, this quest is for anti-paladins only."
endif
~
#55343
phase_2_anti-paladin_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "anti-paladin types but I'll need 3 chrysobeyrl shards, and a set of"
   msend %actor% %self.name% tells you, "Crushed Plate Boots. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55344
phase_2_anti-paladin_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the anti-paladin types"
   msend %actor% %self.name% tells you, "but I'll need 3 radiant ambers, and a Crushed"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55345
phase_2_anti-paladin_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "anti-paladin types but I'll need 3 uncut tourmalines, and"
   msend %actor% %self.name% tells you, "a set of Crushed Gauntlets. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55346
phase_2_anti-paladin_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the anti-paladin types but I'll need 3 flawed jades, and a set of"
   msend %actor% %self.name% tells you, "Crushed Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55347
phase_2_anti-paladin_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the anti-paladin types but I'll need 3 enchanted jades, and"
   msend %actor% %self.name% tells you, "a set of Crushed Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55348
phase_2_anti-paladin_plate~
0 n 100
plate~
*
* Plate ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine plate for the anti-paladins but"
   msend %actor% %self.name% tells you, "I'll need 3 emeralds, and a Crushed Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55349
phase_2_anti-paladin_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for anti-paladins only
if %actor.class% /= Anti && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "anti-paladin types but I'll need 3 amber shards, and a Crushed Plate"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for anti-paladins only.
endif
~
#55350
anti-paladin_phase_2~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class anti-paladin
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
   set vnum_gem_gloves	55594
   set vnum_gem_boots	55605
   set vnum_gem_bracer	55616
   set vnum_gem_helm	55627
   set vnum_gem_arm	55638
   set vnum_gem_legs	55649
   set vnum_gem_chest	55660
   * rewards for this class
   set vnum_reward_helm 55426
   set vnum_reward_arms 55427
   set vnum_reward_chest 55428
   set vnum_reward_legs 55429
   set vnum_reward_boots 55430
   set vnum_reward_bracer 55431
   set vnum_reward_gloves 55432
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
#55351
phase_2_cleric_greet~
0 g 100
~
wait 2
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 21
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
#55352
Phase_2_cleric_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if (%actor.class% == cleric || %actor.class% == priest)
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
   msend %actor% %self.name% tells you, "Sorry, this quest is for clerics only."
endif
~
#55353
phase_2_cleric_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 21 
>= 2
wait 2
msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
msend %actor% %self.name% tells you, "cleric types but I'll need 3 uncut opals, and a set of"
msend %actor% %self.name% tells you, "Crushed Plate Boots. Return these things to me in any order at"
msend %actor% %self.name% tells you, "any time and I will reward you."
else
msend %actor% Sorry this quest is for clerics only.
endif
~
#55354
phase_2_cleric_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the cleric types"
   msend %actor% %self.name% tells you, "but I'll need 3 quantities of some ruby dust, and a Crushed"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55355
phase_2_cleric_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "cleric types but I'll need 3 quantities of some topaz dust, and"
   msend %actor% %self.name% tells you, "a set of Crushed Gauntlets. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55356
phase_2_cleric_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the cleric types but I'll need 3 uncut rubies, and a set of"
   msend %actor% %self.name% tells you, "Crushed Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55357
phase_2_cleric_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the cleric types but I'll need 3 perfect ambers, and"
   msend %actor% %self.name% tells you, "a set of Crushed Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55358
phase_2_cleric_plate~
0 n 100
plate~
*
* Plate ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine plate for the clerics but"
   msend %actor% %self.name% tells you, "I'll need 3 ruby shards, and a Crushed Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55359
phase_2_cleric_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for clerics only
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "cleric types but I'll need 3 uncut garnets, and a Crushed Plate"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for clerics only.
endif
~
#55360
cleric_phase_2~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
if (%actor.class% == cleric || %actor.class% == priest) && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
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
   set vnum_gem_gloves	55601
   set vnum_gem_boots	55612
   set vnum_gem_bracer	55623
   set vnum_gem_helm	55634
   set vnum_gem_arm	55645
   set vnum_gem_legs	55656
   set vnum_gem_chest	55667
   * rewards for this class
   set vnum_reward_helm 55433
   set vnum_reward_arms 55434
   set vnum_reward_chest 55435
   set vnum_reward_legs 55436
   set vnum_reward_boots 55437
   set vnum_reward_bracer 55438
   set vnum_reward_gloves 55439
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
#55361
phase_2_diabolist_greet~
0 g 100
~
wait 2
if %actor.class% == diabolist && %actor.level% >= 21
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
#55362
Phase_2_diabolist_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == diabolist
   wait 2
   if %actor.quest_stage[phase_armor]% == 1
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice &7&bboots&0, a &7&bhelm&0, &7&bgauntlets&0,"
   msend %actor% %self.name% tells you, "&7&bvambraces&0, &7&bgreaves&0, &7&bchestguard&0, and a &7&bbracer&0."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for diabolists only."
endif
~
#55363
phase_2_diabolist_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "diabolist types but I'll need 3 enchanted turquioses, and a set of"
   msend %actor% %self.name% tells you, "Crushed Plate Boots. Return these things to me in any order at"
   msend %actor% %self.name% tells you, "any time and I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55364
phase_2_diabolist_helm~
0 n 100
helm~
*
* helm ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Helm ya ask? Well now I can make a nice helm for the diabolist types"
   msend %actor% %self.name% tells you, "but I'll need 3 cursed tourmalines, and a Crushed"
   msend %actor% %self.name% tells you, "Helm. Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55365
phase_2_diabolist_gauntlets~
0 n 100
gauntlet gauntlets~
*
* Gauntlets ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gauntlets? Well now I can make a fine pair of gauntlets for the"
   msend %actor% %self.name% tells you, "diabolist types but I'll need 3 turquoise shards, and"
   msend %actor% %self.name% tells you, "a set of Crushed Gauntlets. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55366
phase_2_diabolist_vambraces~
0 n 100
vambraces~
*
* Vambraces ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Vambraces ya ask? Well now I can make a fine set of vambraces for"
   msend %actor% %self.name% tells you, "the diabolist types but I'll need 3 enchanted lapis-lazulis, and a set of"
   msend %actor% %self.name% tells you, "Crushed Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55367
phase_2_diabolist_greaves~
0 n 100
greaves~
*
* Greaves ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "greaves for the diabolist types but I'll need 3 cursed chrysobeyrls, and"
   msend %actor% %self.name% tells you, "a set of Crushed Greaves. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55368
phase_2_diabolist_plate~
0 n 100
chest chestguard~
*
* Plate ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine chestguard for the diabolists but"
   msend %actor% %self.name% tells you, "I'll need 3 handfulls of diamonds, and a Crushed Plate. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55369
phase_2_diabolist_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for diabolists only
if %actor.class% == diabolist && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "diabolist types but I'll need 3 turquoises, and a Crushed Plate"
   msend %actor% %self.name% tells you, "Bracer. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for diabolists only.
endif
~
#55370
diabolist_phase_2~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class diabolist
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
   set vnum_gem_gloves	55595
   set vnum_gem_boots	55606
   set vnum_gem_bracer	55617
   set vnum_gem_helm	55628
   set vnum_gem_arm	55639
   set vnum_gem_legs	55650
   set vnum_gem_chest	55661
   * rewards for this class
   set vnum_reward_helm 55468
   set vnum_reward_arms 55469
   set vnum_reward_chest 55470
   set vnum_reward_legs 55471
   set vnum_reward_boots 55472
   set vnum_reward_bracer 55473
   set vnum_reward_gloves 55474
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
#55371
phase_2_druid_greet~
0 g 100
~
wait 2
if %actor.class% == druid && %actor.level% >= 21
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
#55372
Phase_2_druid_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == druid
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
   msend %actor% %self.name% tells you, "Sorry, this quest is for druids only."
endif
~
#55373
phase_2_druid_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "druid types but I'll need 3 flawed amythests, and"
   msend %actor% %self.name% tells you, "a set of Burned Boots. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55374
phase_2_druid_helm~
0 n 100
cap~
*
* helm ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A cap for ya? Well now I can make a nice cap for the druid types"
   msend %actor% %self.name% tells you, "but I'll need 3 radiant turquoises, and a Burned Cap."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55375
phase_2_druid_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "druid types but I'll need 3 uncut ambers, and a set of Burned"
   msend %actor% %self.name% tells you, "Gloves. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55376
phase_2_druid_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the druid types but I'll need 3 amythests, and"
   msend %actor% %self.name% tells you, "a set of Burned Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55377
phase_2_druid_greaves~
0 n 100
pants~
*
* Greaves ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "pants for the druid types but I'll need 3 enchanted tourmalines,"
   msend %actor% %self.name% tells you, "and a set of Burned Pants. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55378
phase_2_druid_plate~
0 n 100
jerkin~
*
* Plate ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine jerkin for the druids but"
   msend %actor% %self.name% tells you, "I'll need 3 diamonds, and a Burned Jerkin. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55379
phase_2_druid_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for druids only
if %actor.class% == druid && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "druid types but I'll need 3 flawed ambers, and a Burned Wristband."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for druids only.
endif
~
#55380
druid_phase_2~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class druid
if %actor.class% == %class% && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55330
   set vnum_destroyed_boots	55334
   set vnum_destroyed_bracer	55338
   set vnum_destroyed_helm	55342
   set vnum_destroyed_arm	55346
   set vnum_destroyed_legs	55350
   set vnum_destroyed_chest	55354
   * gems for this class
   set vnum_gem_gloves	55596
   set vnum_gem_boots	55607
   set vnum_gem_bracer	55618
   set vnum_gem_helm	55629
   set vnum_gem_arm	55640
   set vnum_gem_legs	55651
   set vnum_gem_chest	55662
   * rewards for this class
   set vnum_reward_helm 55447
   set vnum_reward_arms 55448
   set vnum_reward_chest 55449
   set vnum_reward_legs 55450
   set vnum_reward_boots 55451
   set vnum_reward_bracer 55452
   set vnum_reward_gloves 55453
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
#55381
phase_2_monk_greet~
0 g 100
~
wait 2
if %actor.class% == monk && %actor.level% >= 21
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
#55382
Phase_2_monk_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == monk
   wait 2
   if %actor.quest_stage[phase_armor]% == 1
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice &7&bboots&0, a &7&bcap&0, &7&bfistwraps&0,"
   msend %actor% %self.name% tells you, "&7&bsleeves&0, &7&bleggings&0, &7&btunic&0, and a &7&bbracer&0."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for monks only."
endif
~
#55383
phase_2_monk_boots~
0 n 100
boots~
*
* Boots ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of boots for the"
   msend %actor% %self.name% tells you, "monk types but I'll need 3 handfull of turquoises, and"
   msend %actor% %self.name% tells you, "a set of Burned Boots. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55384
phase_2_monk_helm~
0 n 100
cap~
*
* helm ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A cap for ya? Well now I can make a nice cap for the monk types"
   msend %actor% %self.name% tells you, "but I'll need 3 enchanted ambers, and a Burned Cap."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55385
phase_2_monk_gauntlets~
0 n 100
fist wraps fistwraps~
*
* Gauntlets ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of Fistwraps for the"
   msend %actor% %self.name% tells you, "monk types but I'll need 3 uncut turquoises, and a set of Burned"
   msend %actor% %self.name% tells you, "Gloves. Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55386
phase_2_monk_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the monk types but I'll need 3 handfulls of lapis-lazulis, and"
   msend %actor% %self.name% tells you, "a set of Burned Vambraces. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55387
phase_2_monk_greaves~
0 n 100
leggings~
*
* Greaves ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "leggings for the monk types but I'll need 3 cursed jades,"
   msend %actor% %self.name% tells you, "and a set of Burned Pants. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55388
phase_2_monk_plate~
0 n 100
tunic~
*
* Plate ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine tunic for the monks but"
   msend %actor% %self.name% tells you, "I'll need 3 handful of rubies, and a Burned Jerkin. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55389
phase_2_monk_bracer~
0 n 100
bracer~
*
* Bracer ya ask?
*
* This is for monks only
if %actor.class% == monk && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracer for the"
   msend %actor% %self.name% tells you, "monk types but I'll need 3 flawed turquoises, and a Burned Wristband."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and"
   msend %actor% %self.name% tells you, "I will reward you."
else
   msend %actor% Sorry this quest is for monks only.
endif
~
#55390
monk_phase_2~
0 j 100
~
*
* This is the main receive trigger for the phased
* armor quests installed on the mud. This trigger
* is class and phase specific as defined below.
*
set class monk
if %actor.class% == %class% && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   *
   * pertinient object definitions for this class
   * destroyed armor
   set vnum_destroyed_gloves	55330
   set vnum_destroyed_boots	55334
   set vnum_destroyed_bracer	55338
   set vnum_destroyed_helm	55342
   set vnum_destroyed_arm	55346
   set vnum_destroyed_legs	55350
   set vnum_destroyed_chest	55354
   * gems for this class
   set vnum_gem_gloves	55597
   set vnum_gem_boots	55608
   set vnum_gem_bracer	55619
   set vnum_gem_helm	55630
   set vnum_gem_arm	55641
   set vnum_gem_legs	55652
   set vnum_gem_chest	55663
   * rewards for this class
   set vnum_reward_helm 55461
   set vnum_reward_arms 55462
   set vnum_reward_chest 55463
   set vnum_reward_legs 55464
   set vnum_reward_boots 55465
   set vnum_reward_bracer 55466
   set vnum_reward_gloves 55467
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
#55391
phase_2_necromancer_greet~
0 g 100
~
wait 2
if %actor.class% == necromancer && %actor.level% >= 21
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
#55392
Phase_2_necromancer_accept~
0 n 100
yes armor quests quest hello hi~
*
* This will recognize the acceptance of the phase armor quest
* by a player and set the quest variable for later interaction.
*
if %actor.class% == necromancer
   wait 2
   if %actor.quest_stage[phase_armor]% == 1
      quest advance phase_armor %actor.name%
   endif
   msend %actor% %self.name% tells you, "Excellent, I can make nice &7&bslippers&0, a &7&bskullcap&0, &7&bgloves&0,"
   msend %actor% %self.name% tells you, "&7&bsleeves&0, &7&bpants&0, &7&brobe&0, and a &7&bbracelet&0."
   wait 2
   msend %actor% %self.name% tells you, "If you want to know about how to quest for one, ask me about"
   msend %actor% %self.name% tells you, "it and I will tell you the components you need to get for me"
   msend %actor% %self.name% tells you, "in order to receive your reward."
   wait 2
   msend %actor% %self.name% tells you, "Remember, you can ask me &7&bstatus&0 at any time,"
   msend %actor% %self.name% tells you, "and I'll tell you what you have given me so far."
else
   msend %actor% %self.name% tells you, "Sorry, this quest is for necromancers only."
endif
~
#55393
phase_2_necromancer_boots~
0 n 100
slippers~
*
* Boots ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Boots ya ask? Well now I can make a fine pair of slippers for the"
   msend %actor% %self.name% tells you, "necromancer types but I'll need 3 lapis-lazulis, and"
   msend %actor% %self.name% tells you, "a set of Burned Slippers. Return these things to me in any"
   msend %actor% %self.name% tells you, "order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55394
phase_2_necromancer_helm~
0 n 100
cap skullcap~
*
* helm ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "A skullcap for ya? Well now I can make a nice cap for the necromancer types"
msend %actor% %self.name% tells you, "but I'll need 3 enchanted amethysts, and a Burned Turban."
   msend %actor% %self.name% tells you, "Return these things to me in any order at any time and I"
   msend %actor% %self.name% tells you, "will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55395
phase_2_necromancer_gauntlets~
0 n 100
glove gloves~
*
* Gauntlets ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Gloves? Well now I can make a fine pair of gloves for the"
   msend %actor% %self.name% tells you, "necromancer types but I'll need 3 uncut lapis-lazulis, and a set"
   msend %actor% %self.name% tells you, "of Burned Mittens. Return these things to me in any order at any"
   msend %actor% %self.name% tells you, "time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55396
phase_2_necromancer_vambraces~
0 n 100
sleeves~
*
* Vambraces ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Sleeves ya ask? Well now I can make a fine set of sleeves for"
   msend %actor% %self.name% tells you, "the necromancer types but I'll need 3 ambers, and"
   msend %actor% %self.name% tells you, "a set of Burned Sleeves. Return these things to me in"
   msend %actor% %self.name% tells you, "any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55397
phase_2_necromancer_greaves~
0 n 100
pants~
*
* Greaves ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Ah, protection for yer legs? Well now I can make a fine set of"
   msend %actor% %self.name% tells you, "pants for the necromancer types but I'll need 3 cursed citrines,"
   msend %actor% %self.name% tells you, "and a set of Burned Leggings. Return these things"
   msend %actor% %self.name% tells you, "to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55398
phase_2_necromancer_plate~
0 n 100
robe~
*
* Plate ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Body armor? Well now I can make a fine robe for the necromancers but"
   msend %actor% %self.name% tells you, "I'll need 3 handfulls of emeralds, and a Burned Robe. Return these"
   msend %actor% %self.name% tells you, "things to me in any order at any time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
#55399
phase_2_necromancer_bracer~
0 n 100
bracelet~
*
* Bracer ya ask?
*
* This is for necromancers only
if %actor.class% == necromancer && %actor.level% >= 21 && %actor.quest_stage[phase_armor]% >= 2
   wait 2
   msend %actor% %self.name% tells you, "Bracer ya ask? Well now I can make a protective bracelet for the"
   msend %actor% %self.name% tells you, "necromancer types but I'll need 3 flawed lapis-lazulis, and a"
   msend %actor% %self.name% tells you, "Burned Bracelet. Return these things to me in any order at any"
   msend %actor% %self.name% tells you, "time and I will reward you."
else
   msend %actor% Sorry this quest is for necromancers only.
endif
~
$~
