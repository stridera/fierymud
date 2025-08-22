#5501
3bl_qm_greet~
0 g 100
~
wait 2
if (%actor.align% <= 150 && %actor.quest_stage[Black_Legion]% == 0) 
   msend %actor%
   msend %actor% %self.name% tells you, 'Hrm, a new recruit to fight the
   msend %actor% &0elven scum?'
   msend %actor%
endif
~
#5502
3bl_qm_initiate~
0 dn 100
quest recruit elven scum hi hello yes~
*
* This will recognize the acceptance of the Eldorian combat quests
* and set the quest variable for later interaction.
*
* This is for neutrals and evils only.
if %actor.quest_variable[Black_Legion:eg_ally]%
   msend %actor% %self.name% tells you, 'You have already chosen your side!
   msend %actor% &0Be gone, filth!'
   halt
endif
if (%actor.align% <= 150)
   wait 2
   if %actor.quest_stage[Black_Legion]% == 0
      quest start Black_Legion %actor.name%
      quest variable Black_Legion %actor.name% BL_FACTION 0
      quest variable Black_Legion %actor.name% EG_FACTION 0
      * Note that it is not necessary to initialize sub-quest
      * variables to 0 because non-existent quest variables
      * return the same as variables set to 0 in conditionals
      quest variable Black_Legion %actor.name% bl_ally 1
      * being added 2-18-2021 to prevent team switching
   endif
   msend %actor% %self.name% tells you, 'Yes, You need to go assist our
   msend %actor% &0fighters.  Go, invade the 3rd Eldorian Guard and lay waste to their ranks!'
   msend %actor%   
   msend %actor% %self.name% tells you, 'If you bring me back &7&b[trophies]&0 of
   msend %actor%your victories, I will &7&b[reward]&0 you!'
   msend %actor%   
   msend %actor% %self.name% tells you, 'If you like you can ask your 
   msend %actor% &0&7&b[faction status]&0.'
   msend %actor%
endif
~
#5503
3bl_turn_in~
0 j 100
~
*
* This is the main receive trigger for the Edorian
* combat quests installed on the mud. This trigger
* will be generic and the vnums and variables set
* to reflect which faction it applies to.
*
* 3bl is Third Black Legion - The player MUST be 
* on the quest already in order to turn in because
* the variables are not set yet!
if %actor.quest_variable[Black_Legion:eg_ally]%
   msend %actor% %self.name% tells you, 'You have already chosen your side!
   msend %actor% &0Be gone, filth!'
   halt
endif
if (%actor.align% <= 150 && %actor.quest_stage[Black_Legion]% > 0)
   quest variable Black_Legion %actor.name% bl_ally 1
   *
   * pertinent object definitions for this faction
   * trophies
   set vnum_3el_skull         5504
   set vnum_3el_ring          5506
   set vnum_3el_badge         5508
   set vnum_3el_token         5510
   set vnum_3el_insignia      5512
   set vnum_3el_wand          5514
   set vnum_3el_symbol        5516
   * gems for this faction
   set vnum_gem_3bl_cap       55570
   set vnum_gem_3bl_neck      55571
   set vnum_gem_3bl_arm       55572
   set vnum_gem_3bl_wrist     55573
   set vnum_gem_3bl_gloves    55574
   set vnum_gem_3bl_jerkin    55575
   set vnum_gem_3bl_robe      55589
   set vnum_gem_3bl_belt      55576
   set vnum_gem_3bl_legs      55577
   set vnum_gem_3bl_boots     55578
   set vnum_gem_3bl_mask      55579
   set vnum_gem_3bl_symbol    55580
   set vnum_gem_3bl_staff     55581
   set vnum_gem_3bl_ssword    55582
   set vnum_gem_3bl_whammer   55583
   set vnum_gem_3bl_flail     55584
   set vnum_gem_3bl_shiv      55585
   set vnum_gem_3bl_lsword    55586
   set vnum_gem_3bl_smace     55587
   set vnum_gem_3bl_light     55588
   set vnum_gem_3bl_food      55566
   set vnum_gem_3bl_drink     55567
   * rewards
   set vnum_3bl_cap           5517
   set vnum_3bl_neck          5519
   set vnum_3bl_arm           5521
   set vnum_3bl_wrist         5523
   set vnum_3bl_gloves        5525
   set vnum_3bl_jerkin        5527
   set vnum_3bl_belt          5529
   set vnum_3bl_legs          5531
   set vnum_3bl_boots         5533
   set vnum_3bl_mask          5535
   set vnum_3bl_robe          5537
   set vnum_3bl_symbol        5515
   set vnum_3bl_staff         5539
   set vnum_3bl_ssword        5540
   set vnum_3bl_whammer       5541
   set vnum_3bl_flail         5542
   set vnum_3bl_shiv          5543
   set vnum_3bl_lsword        5544
   set vnum_3bl_smace         5545
   set vnum_3bl_light         5553
   set vnum_3bl_food          5555
   set vnum_3bl_drink         5557
   *
   * attempt to reinitialize slutty dg variables to '' (nothing)
   * so this switch will work.
   *
   unset is_gem
   unset exp_multiplier
   unset vnum_trophy
   unset faction_multiplier
   unset vnum_reward
   unset faction_required
   *
   * These case set the variables for the quests.
   * Note the object.vnum is the vnum of the object handed to
   * the NPC by the player.
   *
   switch %object.vnum%   
      case %vnum_gem_3bl_cap%
         set is_gem 1
         set exp_multiplier 10
         set vnum_reward %vnum_3bl_cap%
         set faction_required 20
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_food%
         set is_gem 1
         set exp_multiplier 10
         set vnum_reward %vnum_3bl_food%
         set faction_required 20
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_drink%
         set is_gem 1
         set exp_multiplier 10
         set vnum_reward %vnum_3bl_drink%
         set faction_required 20
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_ssword%
         set is_gem 1
         set exp_multiplier 10
         set vnum_reward %vnum_3bl_ssword%
         set faction_required 20
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_neck%
         set is_gem 1
         set exp_multiplier 12
         set vnum_reward %vnum_3bl_neck%
         set faction_required 40
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_staff%
         set is_gem 1
         set exp_multiplier 12
         set vnum_reward %vnum_3bl_staff%
         set faction_required 40
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_arm%
         set is_gem 1
         set exp_multiplier 14
         set vnum_reward %vnum_3bl_arm%
         set faction_required 55
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_whammer%
         set is_gem 1
         set exp_multiplier 14
         set vnum_reward %vnum_3bl_whammer%
         set faction_required 55
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_wrist%
         set is_gem 1
         set exp_multiplier 16
         set vnum_reward %vnum_3bl_wrist%
         set faction_required 70
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_flail%
         set is_gem 1
         set exp_multiplier 16
         set vnum_reward %vnum_3bl_flail%
         set faction_required 70
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_gloves%
         set is_gem 1
         set exp_multiplier 18
         set vnum_reward %vnum_3bl_gloves%
         set faction_required 85
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_symbol%
         set is_gem 1
         set exp_multiplier 18
         set vnum_reward %vnum_3bl_symbol%
         set faction_required 85
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_belt%
         set is_gem 1
         set exp_multiplier 20
         set vnum_reward %vnum_3bl_belt%
         set faction_required 100
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_shiv%
         set is_gem 1
         set exp_multiplier 20
         set vnum_reward %vnum_3bl_shiv%
         set faction_required 100
         set faction_multiplier 1
         break
      case %vnum_gem_3bl_boots%
         set is_gem 1
         set exp_multiplier 22
         set vnum_reward %vnum_3bl_boots%
         set faction_required 115
         set faction_multiplier 2
         break
      case %vnum_gem_3bl_lsword%
         set is_gem 1
         set exp_multiplier 22
         set vnum_reward %vnum_3bl_lsword%
         set faction_required 115
         set faction_multiplier 2
         break
      case %vnum_gem_3bl_legs%
         set is_gem 1
         set exp_multiplier 24
         set vnum_reward %vnum_3bl_legs%
         set faction_required 130
         set faction_multiplier 2
         break
      case %vnum_gem_3bl_robe%
         set is_gem 1
         set exp_multiplier 26
         set vnum_reward %vnum_3bl_robe%
         set faction_required 145
         set faction_multiplier 2
         break
      case %vnum_gem_3bl_jerkin%
         set is_gem 1
         set exp_multiplier 26
         set vnum_reward %vnum_3bl_jerkin%
         set faction_required 145
         set faction_multiplier 2
         break
      case %vnum_gem_3bl_light%
         set is_gem 1
         set exp_multiplier 26
         set vnum_reward %vnum_3bl_light%
         set faction_required 85
         set faction_multiplier 2
         break
      case %vnum_gem_3bl_mask%
         set is_gem 1
         set exp_multiplier 28
         set vnum_reward %vnum_3bl_mask%
         set faction_required 160
         set faction_multiplier 3
         break
      case %vnum_gem_3bl_smace%
         set is_gem 1
         set exp_multiplier 28
         set vnum_reward %vnum_3bl_smace%
         set faction_required 160
         set faction_multiplier 3
         break
      case %vnum_3el_skull%
         set exp_multiplier 2
         set vnum_trophy %vnum_3el_skull%
          set faction_multiplier 1
         break
      case %vnum_3el_ring%
         set exp_multiplier 2
         set vnum_trophy %vnum_3el_ring%
         set faction_multiplier 1
         break
      case %vnum_3el_badge%
         set exp_multiplier 2
         set vnum_trophy %vnum_3el_badge%
         set faction_multiplier 1
         break
      case %vnum_3el_token%
         set exp_multiplier 2
         set vnum_trophy %vnum_3el_token%
         set faction_multiplier 2
         break
      case %vnum_3el_insignia%
         set exp_multiplier 2
         set vnum_trophy %vnum_3el_insignia%
         set faction_multiplier 2
         break
      case %vnum_3el_wand%
         set exp_multiplier 2
         set vnum_trophy %vnum_3el_wand%
         set faction_multiplier 2
         break
      case %vnum_3el_symbol%
         set exp_multiplier 2
         set vnum_trophy %vnum_3el_symbol%
         set faction_multiplier 3
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
   if !%is_gem%
      set faction_advance 0
      set exp_advance 0
      * hrmm Jelos' magical variable declaration
      if (%actor.quest_variable[black_legion:%vnum_trophy%_trophies]%)
      else
         quest variable black_legion %actor.name% %vnum_trophy%_trophies 0
      endif
      eval trophies %actor.quest_variable[black_legion:%vnum_trophy%_trophies]%
      quest variable black_legion %actor.name% %vnum_trophy%_trophies %trophies%
      * The highest faction a player can gain from interacting with the 3rd front creatures will
      * be 200.  For this section the trophy turn in will reply on this and other checks.
      if (%actor.quest_variable[black_legion:bl_faction]% < 200)
         eval trophies %actor.quest_variable[black_legion:%vnum_trophy%_trophies]% + 1
         quest variable black_legion %actor.name% %vnum_trophy%_trophies %trophies%
         wait 2
         msend %actor% %self.name% tells you, 'Hrm, yes.. you have been out
         msend %actor% &0fighting the Eldorian Guard.  I see from my records you have now given me
         msend %actor% &0&3&b%trophies%&0 &7&b%get.obj_shortdesc[%vnum_trophy%]%&0.'
         mjunk %object.name%
         msave %actor%
         if (%trophies% < 10)
            set faction_advance 1
            set exp_advance 1
         else
            set faction_advance 1
         endif
      else
         if (%trophies% < 10)
            set exp_advance 1
            eval trophies %actor.quest_variable[black_legion:%vnum_trophy%_trophies]% + 1
            quest variable black_legion %actor.name% %vnum_trophy%_trophies %trophies%
         else
            return 0
         endif       
         wait 2
         eye %actor.name%
         msend %actor% %self.name% tells you, 'You have garnered as much favor with
         msend %actor% &0the Black Legion as you possibly can by fighting the Third Eldrian Guard.  You
         msend %actor% &0may one day find other more difficult battles with those tree rats that will
         msend %actor% &0curry more favor with our lords.'
      endif
      *
      * Double check that the criteria for rewards for this section are met.
      *
      if (%exp_advance% > 0)
         *
         * loop for exp award.
         *
         msend %actor%   
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
      endif
      if (%faction_advance% > 0)
         *
         * loop for faction award.
         *
         msend %actor% &3&bYour curry favor with the Black Legion!&0
         msend %actor% &1&bYou feel more hated by the Eldorian Guard!&0
         set lap 1
         while %lap% <= %faction_multiplier%
            eval faction %actor.quest_variable[black_legion:bl_faction]% + 1
            quest variable black_legion %actor.name% bl_faction %faction%
            *
            * slight bug for now the trigger code can't seem to evaluate negative numbers?
            *
            eval faction2 %actor.quest_variable[black_legion:eg_faction]% - 1
            quest variable black_legion %actor.name% eg_faction %faction2%
            eval lap %lap% + 1
         done
         *
         * end faction loop
         *
         * all other items should be mpjunked or whatever by now
      endif
      * end of !if_gem section or trophy endif
      msave %actor.name%
   endif
   if (%is_gem%)
      * hrmm Jelos' magical variable declaration
      if (%actor.quest_variable[black_legion:%vnum_reward%_reward]%)
      else
         quest variable black_legion %actor.name% %vnum_reward%_reward 0
      endif
      eval rewards %actor.quest_variable[black_legion:%vnum_reward%_reward]%
      quest variable black_legion %actor.name% %vnum_reward%_reward %rewards%
      if (%actor.align% >= 151)  
         return 0
         wait 2
         msend %actor% %self.name% tells you, 'You aren't quite evil enough for
         msend %actor% &0these rewards.'
         halt
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= %faction_required%)
         eval rewards %actor.quest_variable[black_legion:%vnum_reward%_reward]% + 1
         quest variable black_legion %actor.name% %vnum_reward%_reward %rewards%
         wait 2
         msend %actor% %self.name% tells you, 'Ah yes, the Legion thanks you for
         msend %actor% &0your efforts.  Take this to aid you in your battles.'
         wait 1
         mload obj %vnum_reward%
         wait 1
         if (%actor.quest_variable[black_legion:%vnum_reward%_reward]% == 1)
            *
            * loop for exp award.
            *
            msend %actor%   
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
         endif
         if (%actor.quest_variable[black_legion:bl_faction]% < 200)
            *
            * loop for faction award.
            *
            msend %actor% &3&bYour curry favor with the Black Legion!&0
            msend %actor% &1&bYou feel more hated by the Eldorian Guard!&0
            set lap 1
            while %lap% <= %faction_multiplier%
               eval faction %actor.quest_variable[black_legion:bl_faction]% + 1
               quest variable black_legion %actor.name% bl_faction %faction%
               *
               * slight bug for now the trigger code can't seem to evaluate negative numbers?
               *
               eval faction2 %actor.quest_variable[black_legion:eg_faction]% - 1
               quest variable black_legion %actor.name% eg_faction %faction2%
               eval lap %lap% + 1
            done
            *
            * end faction loop
            *
            * all other items should be mpjunked or whatever by now
         endif    
         mjunk %object.name%
         mjunk all.eldoria-trophy
         give all %actor.name%      
         msave %actor.name%
      else 
         return 0
         wait 2
         msend %actor% %self.name% tells you, 'You aren't quite allied enough
         msend %actor% &0with our cause to earn that reward.'   
         halt        
      endif
      * end of if_gem section 
   endif
else
   * Alignment not low enough or hasn't started the quest yet.
   return 0
   wait 2
   msend %actor% %self.name% tells you, 'You aren't quite allied enough with
   msend %actor% &0our cause to earn our rewards.  You need a lower alignment or to start these
   msend %actor% &0quests by saying &7&b[yes]&0 or &b&7[quest]&0 or &b&7[hello]&0.'
endif
~
#5504
3bl_rewards_list~
0 dn 100
reward rewards~
if %actor.quest_variable[Black_Legion:eg_ally]%
   msend %actor% %self.name% tells you, 'You have already chosen your side!
   msend %actor% &0Be gone, filth!'
   halt
endif
set vnum_gem_3bl_cap       55570
set vnum_gem_3bl_neck      55571
set vnum_gem_3bl_arm       55572
set vnum_gem_3bl_wrist     55573
set vnum_gem_3bl_gloves    55574
set vnum_gem_3bl_jerkin    55575
set vnum_gem_3bl_robe      55589
set vnum_gem_3bl_belt      55576
set vnum_gem_3bl_legs      55577
set vnum_gem_3bl_boots     55578
set vnum_gem_3bl_mask      55579
set vnum_gem_3bl_symbol    55580
set vnum_gem_3bl_staff     55581
set vnum_gem_3bl_ssword    55582
set vnum_gem_3bl_whammer   55583
set vnum_gem_3bl_flail     55584
set vnum_gem_3bl_shiv      55585
set vnum_gem_3bl_lsword    55586
set vnum_gem_3bl_smace     55587
set vnum_gem_3bl_light     55588
set vnum_gem_3bl_food      55566
set vnum_gem_3bl_drink     55567
* rewards
set vnum_3bl_cap           5517
set vnum_3bl_neck          5519
set vnum_3bl_arm           5521
set vnum_3bl_wrist         5523
set vnum_3bl_gloves        5525
set vnum_3bl_jerkin        5527
set vnum_3bl_belt          5529
set vnum_3bl_legs          5531
set vnum_3bl_boots         5533
set vnum_3bl_mask          5535
set vnum_3bl_robe          5537
set vnum_3bl_symbol        5515
set vnum_3bl_staff         5539
set vnum_3bl_ssword        5540
set vnum_3bl_whammer       5541
set vnum_3bl_flail         5542
set vnum_3bl_shiv          5543
set vnum_3bl_lsword        5544
set vnum_3bl_smace         5545
set vnum_3bl_light         5553
set vnum_3bl_food          5555
set vnum_3bl_drink         5557
*
* Check faction and react accordingly
*
if (%actor.align% <= 150)
   if (%actor.quest_stage[Black_Legion]% > 0)
      if (%actor.quest_variable[black_legion:bl_faction]% < 20)
         msend %actor% 
         msend %actor% %self.name% tells you, 'You are a bit raw for a recruit,
         msend %actor% &0come back when you have defeated more of the Eldorian villainy.'
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= 20)
         msend %actor%
         * msend %actor% %self.name% tells you,'Your bl faction is %actor.quest_variable[black_legion:bl_faction]%. 
         msend %actor% %self.name% tells you, 'Though you are a bit wet behind the
         msend %actor% &0ears I suppose we can trust you with some of our goods.'
         msend %actor%
         msend %actor% &0You have access to:
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_food%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_food%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_drink%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_drink%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_cap%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_cap%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_ssword%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_ssword%]%&0
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= 40)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_neck%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_neck%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_staff%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_staff%]%&0
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= 55)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_arm%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_arm%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_whammer%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_whammer%]%&0
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= 70)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_wrist%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_wrist%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_flail%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_flail%]%&0
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= 85)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_gloves%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_gloves%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_symbol%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_symbol%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_light%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_light%]%&0
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= 100)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_belt%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_belt%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_shiv%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_shiv%]%&0
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= 115)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_boots%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_boots%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_lsword%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_lsword%]%&0
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= 130)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_legs%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_legs%]%&0
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= 145)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_robe%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_robe%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_jerkin%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_jerkin%]%&0
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= 160)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_mask%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_mask%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3bl_smace%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3bl_smace%]%&0
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% < 160)
         msend %actor%
         msend %actor% %self.name% tells you, 'As your standing with the Black
         msend %actor% &0Legion improves I will show you more rewards.'
      endif
   msend %actor%
   endif
endif
~
#5505
3eg_qm_greet~
0 g 100
~
wait 2
if (%actor.align% >= -150 && %actor.quest_stage[Black_Legion]% == 0) 
   msend %actor% %self.name% tells you, 'Hrm, a new recruit
   msend %actor% &0to fight the undead masses?'
endif
~
#5506
3eg_qm_initiate~
0 dn 100
quest recruit undead masses hi hello yes~
*
* This will recognize the acceptance of the Eldorian combat quests
* and set the quest variable for later interaction.
*
* This is for neutrals and good only.
if %actor.quest_variable[Black_Legion:bl_ally]%
   msend %actor% %self.name% tells you, 'You have pledged
   msend %actor% &0yourself to the forces of darkness!  Suffer under your choice!'
   halt
endif
if (%actor.align% >= -150)
   wait 2
   if %actor.quest_stage[Black_Legion]% == 0
      quest start Black_Legion %actor.name%
      quest variable Black_Legion %actor.name% BL_FACTION 0
      quest variable Black_Legion %actor.name% EG_FACTION 0
      * Note that it is not necessary to initialize sub-quest
      * variables to 0 because non-existent quest variables
      * return the same as variables set to 0 in conditionals
      quest variable Black_Legion %actor.name% eg_ally 1
   endif
   msend %actor% %self.name% tells you, 'Yes, You need to go
   msend %actor% &0assist our fighters.  Go, invade the 3rd Black Legion and lay waste to their
   msend %actor% &0ranks!'
   msend %actor%   
   msend %actor% %self.name% tells you, 'If you bring me back
   msend %actor% &0&7&b[trophies]&0 of your victories, I will &7&b[reward]&0 you!'
   msend %actor%   
   msend %actor% %self.name% tells you, 'If you like you can ask
   msend %actor% &0your &7&b[faction status]&0.'
   msend %actor%
endif
~
#5507
3eg_turn_in~
0 j 100
~
*
* This is the main receive trigger for the Edorian
* combat quests installed on the mud. This trigger
* will be generic and the vnums and variables set
* to reflect which faction it applies to.
*
* 3eg is Third Eldorian Guard
if %actor.quest_variable[Black_Legion:bl_ally]%
   msend %actor% %self.name% tells you, 'You have pledged
   msend %actor% &0yourself to the forces of darkness!  Suffer under your choice!'
   halt
endif
if (%actor.align% >= -150 && %actor.quest_stage[Black_Legion]% > 0)
   quest variable Black_Legion %actor.name% eg_ally 1
   *
   * pertinent object definitions for this faction
   * trophies
   set vnum_3bl_skull         5503
   set vnum_3bl_ring          5505
   set vnum_3bl_badge         5507
   set vnum_3bl_token         5509
   set vnum_3bl_insignia      5511
   set vnum_3bl_wand          5513
   set vnum_3bl_symbol        5515
   * gems for this faction
   set vnum_gem_3eg_cap       55570
   set vnum_gem_3eg_neck      55571
   set vnum_gem_3eg_arm       55572
   set vnum_gem_3eg_wrist     55573
   set vnum_gem_3eg_gloves    55574
   set vnum_gem_3eg_jerkin    55575
   set vnum_gem_3eg_robe      55589
   set vnum_gem_3eg_belt      55576
   set vnum_gem_3eg_legs      55577
   set vnum_gem_3eg_boots     55578
   set vnum_gem_3eg_mask      55579
   set vnum_gem_3eg_symbol    55580
   set vnum_gem_3eg_staff     55581
   set vnum_gem_3eg_ssword    55582
   set vnum_gem_3eg_whammer   55583
   set vnum_gem_3eg_flail     55584
   set vnum_gem_3eg_shiv      55585
   set vnum_gem_3eg_lsword    55586
   set vnum_gem_3eg_smace     55587
   set vnum_gem_3eg_light     55588
   set vnum_gem_3eg_food      55566
   set vnum_gem_3eg_drink     55567
   * rewards
   set vnum_3eg_cap           5518
   set vnum_3eg_neck          5520
   set vnum_3eg_arm           5522
   set vnum_3eg_wrist         5524
   set vnum_3eg_gloves        5526
   set vnum_3eg_jerkin        5528
   set vnum_3eg_belt          5530
   set vnum_3eg_legs          5532
   set vnum_3eg_boots         5534
   set vnum_3eg_mask          5536
   set vnum_3eg_robe          5538
   set vnum_3eg_symbol        5516
   set vnum_3eg_staff         5546
   set vnum_3eg_ssword        5547
   set vnum_3eg_whammer       5548
   set vnum_3eg_flail         5549
   set vnum_3eg_shiv          5550
   set vnum_3eg_lsword        5551
   set vnum_3eg_smace         5552
   set vnum_3eg_light         5554
   set vnum_3eg_food          5556
   set vnum_3eg_drink         5558
   *
   * attempt to reinitialize slutty dg variables to '' (nothing)
   * so this switch will work.
   *
   unset is_gem
   unset exp_multiplier
   unset vnum_trophy
   unset faction_multiplier
   unset vnum_reward
   unset faction_required
   *
   * These cases set the variables for the quests.
   * Note the object.vnum is the vnum of the object handed to
   * the NPC by the player.
   *
   switch %object.vnum%   
      case %vnum_gem_3eg_cap%
         set is_gem 1
         set exp_multiplier 10
         set vnum_reward %vnum_3eg_cap%
         set faction_required 20
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_food%
         set is_gem 1
         set exp_multiplier 10
         set vnum_reward %vnum_3eg_food%
         set faction_required 20
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_drink%
         set is_gem 1
         set exp_multiplier 10
         set vnum_reward %vnum_3eg_drink%
         set faction_required 20
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_ssword%
         set is_gem 1
         set exp_multiplier 10
         set vnum_reward %vnum_3eg_ssword%
         set faction_required 20
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_neck%
         set is_gem 1
         set exp_multiplier 12
         set vnum_reward %vnum_3eg_neck%
         set faction_required 40
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_staff%
         set is_gem 1
         set exp_multiplier 12
         set vnum_reward %vnum_3eg_staff%
         set faction_required 40
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_arm%
         set is_gem 1
         set exp_multiplier 14
         set vnum_reward %vnum_3eg_arm%
         set faction_required 55
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_whammer%
         set is_gem 1
         set exp_multiplier 14
         set vnum_reward %vnum_3eg_whammer%
         set faction_required 55
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_wrist%
         set is_gem 1
         set exp_multiplier 16
         set vnum_reward %vnum_3eg_wrist%
         set faction_required 70
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_flail%
         set is_gem 1
         set exp_multiplier 16
         set vnum_reward %vnum_3eg_flail%
         set faction_required 70
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_gloves%
         set is_gem 1
         set exp_multiplier 18
         set vnum_reward %vnum_3eg_gloves%
         set faction_required 85
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_symbol%
         set is_gem 1
         set exp_multiplier 18
         set vnum_reward %vnum_3eg_symbol%
         set faction_required 85
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_belt%
         set is_gem 1
         set exp_multiplier 20
         set vnum_reward %vnum_3eg_belt%
         set faction_required 100
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_shiv%
         set is_gem 1
         set exp_multiplier 20
         set vnum_reward %vnum_3eg_shiv%
         set faction_required 100
         set faction_multiplier 1
         break
      case %vnum_gem_3eg_boots%
         set is_gem 1
         set exp_multiplier 22
         set vnum_reward %vnum_3eg_boots%
         set faction_required 115
         set faction_multiplier 2
         break
      case %vnum_gem_3eg_lsword%
         set is_gem 1
         set exp_multiplier 22
         set vnum_reward %vnum_3eg_lsword%
         set faction_required 115
         set faction_multiplier 2
         break
      case %vnum_gem_3eg_legs%
         set is_gem 1
         set exp_multiplier 24
         set vnum_reward %vnum_3eg_legs%
         set faction_required 130
         set faction_multiplier 2
         break
      case %vnum_gem_3eg_robe%
         set is_gem 1
         set exp_multiplier 26
         set vnum_reward %vnum_3eg_robe%
         set faction_required 145
         set faction_multiplier 2
         break
      case %vnum_gem_3eg_jerkin%
         set is_gem 1
         set exp_multiplier 26
         set vnum_reward %vnum_3eg_jerkin%
         set faction_required 145
         set faction_multiplier 2
         break
      case %vnum_gem_3eg_light%
         set is_gem 1
         set exp_multiplier 26
         set vnum_reward %vnum_3eg_light%
         set faction_required 85
         set faction_multiplier 2
         break
      case %vnum_gem_3eg_mask%
         set is_gem 1
         set exp_multiplier 28
         set vnum_reward %vnum_3eg_mask%
         set faction_required 160
         set faction_multiplier 3
         break
      case %vnum_gem_3eg_smace%
         set is_gem 1
         set exp_multiplier 28
         set vnum_reward %vnum_3eg_smace%
         set faction_required 160
         set faction_multiplier 3
         break
      case %vnum_3bl_skull%
         set exp_multiplier 2
         set vnum_trophy %vnum_3bl_skull%
         set faction_multiplier 1
         break
      case %vnum_3bl_ring%
         set exp_multiplier 2
         set vnum_trophy %vnum_3bl_ring%
         set faction_multiplier 1
         break
      case %vnum_3bl_badge%
         set exp_multiplier 2
         set vnum_trophy %vnum_3bl_badge%
         set faction_multiplier 1
         break
      case %vnum_3bl_token%
         set exp_multiplier 2
         set vnum_trophy %vnum_3bl_token%
         set faction_multiplier 2
         break
      case %vnum_3bl_insignia%
         set exp_multiplier 2
         set vnum_trophy %vnum_3bl_insignia%
         set faction_multiplier 2
         break
      case %vnum_3bl_wand%
         set exp_multiplier 2
         set vnum_trophy %vnum_3bl_wand%
         set faction_multiplier 2
         break
      case %vnum_3bl_symbol%
         set exp_multiplier 2
         set vnum_trophy %vnum_3bl_symbol%
         set faction_multiplier 3
         break
      default
         return 0
         wait 1
         eye %actor.name%
         msend %actor% %self.name% tells you, 'I am not interested in
         msend %actor% &0this from you.'
         msend %actor% %self.name% returns your item to you.
         halt
         break
   done
   *
   * need to force character saving after each turnin.
   *
   if !%is_gem%
      set faction_advance 0
      set exp_advance 0
      * hrmm Jelos' magical variable declaration
      if (%actor.quest_variable[black_legion:%vnum_trophy%_trophies]%)
      else
         quest variable black_legion %actor.name% %vnum_trophy%_trophies 0
      endif
      eval trophies %actor.quest_variable[black_legion:%vnum_trophy%_trophies]%
      quest variable black_legion %actor.name% %vnum_trophy%_trophies %trophies%
      * The highest faction a player can gain from interacting with the 3rd front creatures will
      * be 200.  For this section the trophy turn in will reply on this and other checks.
      if (%actor.quest_variable[black_legion:eg_faction]% < 200)
         eval trophies %actor.quest_variable[black_legion:%vnum_trophy%_trophies]% + 1
         quest variable black_legion %actor.name% %vnum_trophy%_trophies %trophies%
         wait 2
         msend %actor% %self.name% tells you, 'Hrm, yes... you have
         msend %actor% &0been out fighting the Black Legion.  I see from my records you have now given
         msend %actor% &0me &3&b%trophies% &7%get.obj_shortdesc[%vnum_trophy%]%&0.'
         mjunk %object.name%
         msave %actor%
         if (%trophies% < 10)
            set faction_advance 1
            set exp_advance 1
         else
            set faction_advance 1
         endif
      else
         if (%trophies% < 10)
            set exp_advance 1
            eval trophies %actor.quest_variable[black_legion:%vnum_trophy%_trophies]% + 1
            quest variable black_legion %actor.name% %vnum_trophy%_trophies %trophies%
         else
            return 0
         endif       
         wait 2
         eye %actor.name%
         msend %actor% %self.name% tells you, 'You have garnered as
         msend %actor% &0much favor with the Eldorian Guard as you possibly can by fighting the Third
         msend %actor% &0Black Legion.  You may one day find other more difficult battles with those
         msend %actor% &0tree rats that will curry more favor with our masters.'
      endif
      *
      * Double check that the criteria for rewards for this section are met.
      *
      if (%exp_advance% > 0)
         *
         * loop for exp award.
         *
         msend %actor%   
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
      endif
      if (%faction_advance% > 0)
         *
         * loop for faction award.
         *
         msend %actor% &3&bYour curry favor with the Eldorian Guard!&0
         msend %actor% &1&bYou feel more hated by the Black Legion!&0
         set lap 1
         while %lap% <= %faction_multiplier%
            eval faction %actor.quest_variable[black_legion:eg_faction]% + 1
            quest variable black_legion %actor.name% eg_faction %faction%
            *
            * slight bug for now the trigger code can't seem to evaluate negative numbers?
            * 
            eval faction2 %actor.quest_variable[black_legion:bl_faction]% - 1
            quest variable black_legion %actor.name% bl_faction %faction2%
            eval lap %lap% + 1
         done
         *
         * end faction loop
         *
         * all other items should be mpjunked or whatever by now
      endif
      msave %actor.name%
      * end of !if_gem section or trophy endif
   endif
   if (%is_gem%)
      * hrmm Jelos' magical variable declaration
      if (%actor.quest_variable[black_legion:%vnum_reward%_reward]%)
      else
        quest variable black_legion %actor.name% %vnum_reward%_reward 0
      endif
      eval rewards %actor.quest_variable[black_legion:%vnum_reward%_reward]%
      quest variable black_legion %actor.name% %vnum_reward%_reward %rewards%
      if (%actor.align% <= -151)  
         return 0
         wait 2
         msend %actor% %self.name% tells you, 'You aren't quite good
         msend %actor% &0enough for these rewards.'   
         halt
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= %faction_required%)
         eval rewards %actor.quest_variable[black_legion:%vnum_reward%_reward]% + 1
         quest variable black_legion %actor.name% %vnum_reward%_reward %rewards%
         wait 2
         msend %actor% %self.name% tells you, 'Ah yes, the Guard
         msend %actor% &0thanks you for your efforts.  Take this to aid you in your battles.'
         wait 1
         mload obj %vnum_reward%
         wait 1
         if (%actor.quest_variable[black_legion:%vnum_reward%_reward]% == 1)
            *
            * loop for exp award.
            *
            msend %actor%  
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
         endif
         if (%actor.quest_variable[black_legion:eg_faction]% < 200)
            *
            * loop for faction award.
            *
            msend %actor% &3&bYour curry favor with the Eldorian Guard!&0
            msend %actor% &1&bYou feel more hated by the Black Legion!&0
            set lap 1
            while %lap% <= %faction_multiplier%
               eval faction %actor.quest_variable[black_legion:eg_faction]% + 1
               quest variable black_legion %actor.name% eg_faction %faction%
               *
               * slight bug for now the trigger code can't seem to evaluate negative numbers?
               *
               eval faction2 %actor.quest_variable[black_legion:bl_faction]% - 1
               quest variable black_legion %actor.name% bl_faction %faction2%
               eval lap %lap% + 1
            done
            *
            * end faction loop
            *
            * all other items should be mpjunked or whatever by now
         endif    
         mjunk %object.name%
         mjunk all.eldoria-trophy
         give all %actor.name%      
         msave %actor.name%
      else 
         return 0
         wait 2
         msend %actor% %self.name% tells you, 'You aren't quite allied
         msend %actor% &0enough with our cause to earn that reward.'
         halt        
      endif
      * end of if_gem section 
   endif
else
   * Alignment not high enough or hasn't started the quest yet.
   return 0
   wait 2
   msend %actor% %self.name% tells you, 'You aren't quite allied
   msend %actor% &0enough with our cause to earn our rewards.  You need a higher alignment or to
   msend %actor% &0start these quests by saying &7&b[yes]&0 or &7&b[quest]&0 or &7&b[hello]&0.'
endif
~
#5508
3eg_rewards_list~
0 dn 100
reward rewards~
set vnum_gem_3eg_cap         55570
set vnum_gem_3eg_neck        55571
set vnum_gem_3eg_arm         55572
set vnum_gem_3eg_wrist       55573
set vnum_gem_3eg_gloves      55574
set vnum_gem_3eg_jerkin      55575
set vnum_gem_3eg_robe        55589
set vnum_gem_3eg_belt        55576
set vnum_gem_3eg_legs        55577
set vnum_gem_3eg_boots       55578
set vnum_gem_3eg_mask        55579
set vnum_gem_3eg_symbol      55580
set vnum_gem_3eg_staff       55581
set vnum_gem_3eg_ssword      55582
set vnum_gem_3eg_whammer     55583
set vnum_gem_3eg_flail       55584
set vnum_gem_3eg_shiv        55585
set vnum_gem_3eg_lsword      55586
set vnum_gem_3eg_smace       55587
set vnum_gem_3eg_light       55588
set vnum_gem_3eg_food        55566
set vnum_gem_3eg_drink       55567
* rewards
set vnum_3eg_cap             5518
set vnum_3eg_neck            5520
set vnum_3eg_arm             5522
set vnum_3eg_wrist           5524
set vnum_3eg_gloves          5526
set vnum_3eg_jerkin          5528
set vnum_3eg_belt            5530
set vnum_3eg_legs            5532
set vnum_3eg_boots           5534
set vnum_3eg_mask            5536
set vnum_3eg_robe            5538
set vnum_3eg_symbol          5516
set vnum_3eg_staff           5546
set vnum_3eg_ssword          5547
set vnum_3eg_whammer         5548
set vnum_3eg_flail           5549
set vnum_3eg_shiv            5550
set vnum_3eg_lsword          5551
set vnum_3eg_smace           5552
set vnum_3eg_light           5554
set vnum_3eg_food            5556
set vnum_3eg_drink           5558
*
* Check faction and react accordingly
*
if %actor.quest_variable[Black_Legion:bl_ally]%
   msend %actor% %self.name% tells you, 'You have pledged
   msend %actor% &0yourself to the forces of darkness!  Suffer under your choice!'
   halt
endif
if (%actor.align% >= -150)
   if (%actor.quest_stage[Black_Legion]% > 0)
      if (%actor.quest_variable[black_legion:eg_faction]% < 20)
         msend %actor% 
         msend %actor% %self.name% tells you, 'You are a bit raw for a
         msend %actor% &0recruit, come back when you have defeated more of the Black Legion hordes.'
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 20)
         msend %actor%
         * msend %actor% %self.name% tells you,'Your bl faction is %actor.quest_variable[black_legion:eg_faction]%. 
         msend %actor% %self.name% tells you, 'Though you are a bit
         msend %actor% &0wet behind the ears I suppose we can trust you with some of our goods.'
         msend %actor%  
         msend %actor% &0You have access to:
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_food%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_food%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_drink%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_drink%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_cap%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_cap%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_ssword%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_ssword%]%&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 40)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_neck%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_neck%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_staff%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_staff%]%&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 55)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_arm%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_arm%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_whammer%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_whammer%]%&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 70)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_wrist%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_wrist%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_flail%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_flail%]%&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 85)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_gloves%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_gloves%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_symbol%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_symbol%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_light%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_light%]%&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 100)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_belt%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_belt%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_shiv%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_shiv%]%&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 115)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_boots%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_boots%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_lsword%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_lsword%]%&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 130)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_legs%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_legs%]%&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 145)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_robe%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_robe%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_jerkin%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_jerkin%]%&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 160)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_mask%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_mask%]%&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_smace%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_smace%]%&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% < 160)
         msend %actor%
         msend %actor% %self.name% tells you, 'As your standing with
         msend %actor% &0the 3rd Eldorian Guard improves I will show you more rewards.' 
      endif
   endif
endif
~
#5509
3eg_trophy~
0 dn 100
trophy trophies~
if %actor.quest_variable[Black_Legion:bl_ally]%
   msend %actor% %self.name% tells you, 'You have pledged
   msend %actor% &0yourself to the forces of darkness!  Suffer under your choice!'
   halt
endif
set vnum_trophy1 5503
set vnum_trophy2 5505
set vnum_trophy3 5507
set vnum_trophy4 5509
set vnum_trophy5 5511
set vnum_trophy6 5513
set vnum_trophy7 5515
if (%actor.align% >= -150 && %actor.quest_stage[Black_Legion]% == 1) 
   msend %actor%
   msend %actor% %self.name% tells you, 'As you fight the Black
   msend %actor% &0Legion you will periodically find goods on their bodies that we will want in
   msend %actor% &0order to prove that you are working with us.'
   msend %actor%
   msend %actor% &0Items we're interested in are:
   msend %actor% - %get.obj_shortdesc[%vnum_trophy1%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy2%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy3%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy4%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy5%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy6%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy7%]%
endif
~
#5510
3bl_trophy~
0 dn 100
trophy trophies~
if %actor.quest_variable[Black_Legion:eg_ally]%
   msend %actor% %self.name% tells you, 'You have already chosen your side!
   msend %actor% &0Be gone, filth!'
   halt
endif
set vnum_trophy1 5504
set vnum_trophy2 5506
set vnum_trophy3 5508
set vnum_trophy4 5510
set vnum_trophy5 5512
set vnum_trophy6 5514
set vnum_trophy7 5516
if (%actor.align% <= 150 && %actor.quest_stage[Black_Legion]% == 1) 
   msend %actor%
   msend %actor% %self.name% tells you, 'As you fight the Eldorian Guard you
   msend %actor% &0will periodically find goods on their bodies that we will want in order to
   msend %actor% &0prove that you are working with us.'
   msend %actor%
   msend %actor% &0Items we're interested in are:
   msend %actor% - %get.obj_shortdesc[%vnum_trophy1%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy2%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy3%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy4%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy5%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy6%]%
   msend %actor% - %get.obj_shortdesc[%vnum_trophy7%]%
endif
~
#5511
p1_3eg_death~
0 f 100
~
mload obj 17210
recite all.scroll
mecho %self.name%'s gear is destroyed in the battle!
mjunk all.eldoria-reward
mjunk all.scroll
set vnum_trophy1 5504
set vnum_trophy2 5506
set vnum_trophy3 5508
set vnum_trophy4 5510
   *
   * Death trigger for random trophy drops
   *
   * set a random number to determine if a drop will
   * happen.
   *
   set will_drop %random.100%
   *
   if %will_drop% <= 10
      * drop nothing and bail
      halt
   endif
   if %will_drop% <= 50
      mload obj %vnum_trophy1%
   elseif (%will_drop% >= 51 && %will_drop% <= 70)
      mload obj %vnum_trophy2%
   elseif (%will_drop% >= 71 && %will_drop% <= 90)
      mload obj %vnum_trophy3%
   else
      mload obj %vnum_trophy4%
   endif
~
#5512
p2_3eg_death~
0 f 100
~
mload obj 17210
recite all.scroll
mecho %self.name%'s gear is destroyed in the battle!
mjunk all.eldoria-reward
mjunk all.scroll
set vnum_trophy1 5508
set vnum_trophy2 5510
set vnum_trophy3 5512
set vnum_trophy4 5514
   *
   * Death trigger for random trophy drops
   *
   * set a random number to determine if a drop will
   * happen.
   *
   set will_drop %random.100%
   *
   if %will_drop% <= 10
      * drop nothing and bail
      halt
   endif
   if %will_drop% <= 50
      mload obj %vnum_trophy1%
   elseif (%will_drop% >= 51 && %will_drop% <= 70)
      mload obj %vnum_trophy2%
   elseif (%will_drop% >= 71 && %will_drop% <= 90)
      mload obj %vnum_trophy3%
   else
      mload obj %vnum_trophy4%
   endif
~
#5513
p3_3eg_death~
0 f 100
~
mload obj 17210
recite all.scroll
mecho %self.name%'s gear is destroyed in the battle!
mjunk all.eldoria-reward
mjunk all.scroll
set vnum_trophy1 5510
set vnum_trophy2 5512
set vnum_trophy3 5514
set vnum_trophy4 5516
   *
   * Death trigger for random trophy drops
   *
   * set a random number to determine if a drop will
   * happen.
   *
   set will_drop %random.100%
   *
   if %will_drop% <= 10
      * drop nothing and bail
      halt
   endif
   if %will_drop% <= 50
      mload obj %vnum_trophy1%
   elseif (%will_drop% >= 51 && %will_drop% <= 70)
      mload obj %vnum_trophy2%
   elseif (%will_drop% >= 71 && %will_drop% <= 90)
      mload obj %vnum_trophy3%
   else
      mload obj %vnum_trophy4%
   endif
~
#5514
p1_3bl_death~
0 f 100
~
mload obj 17210
recite all.scroll
mecho %self.name%'s gear is destroyed in the battle!
mjunk all.eldoria-reward
mjunk all.scroll
set vnum_trophy1 5503
set vnum_trophy2 5505
set vnum_trophy3 5507
set vnum_trophy4 5509
   *
   * Death trigger for random trophy drops
   *
   * set a random number to determine if a drop will
   * happen.
   *
   set will_drop %random.100%
   *
   if %will_drop% <= 10
      * drop nothing and bail
      halt
   endif
   if %will_drop% <= 50
      mload obj %vnum_trophy1%
   elseif (%will_drop% >= 51 && %will_drop% <= 70)
      mload obj %vnum_trophy2%
   elseif (%will_drop% >= 71 && %will_drop% <= 90)
      mload obj %vnum_trophy3%
   else
      mload obj %vnum_trophy4%
   endif
~
#5515
p2_3bl_death~
0 f 100
~
mload obj 17210
recite all.scroll
mecho %self.name%'s gear is destroyed in the battle!
mjunk all.eldoria-reward
mjunk all.scroll
set vnum_trophy1 5507
set vnum_trophy2 5509
set vnum_trophy3 5511
set vnum_trophy4 5513
   *
   * Death trigger for random trophy drops
   *
   * set a random number to determine if a drop will
   * happen.
   *
   set will_drop %random.100%
   *
   if %will_drop% <= 10
      * drop nothing and bail
      halt
   endif
   if %will_drop% <= 50
      mload obj %vnum_trophy1%
   elseif (%will_drop% >= 51 && %will_drop% <= 70)
      mload obj %vnum_trophy2%
   elseif (%will_drop% >= 71 && %will_drop% <= 90)
      mload obj %vnum_trophy3%
   else
      mload obj %vnum_trophy4%
   endif
~
#5516
p3_3bl_death~
0 f 100
~
mload obj 17210
recite all.scroll
mecho %self.name%'s gear is destroyed in the battle!
mjunk all.eldoria-reward
mjunk all.scroll
set vnum_trophy1 5509
set vnum_trophy2 5511
set vnum_trophy3 5513
set vnum_trophy4 5515
   *
   * Death trigger for random trophy drops
   *
   * set a random number to determine if a drop will
   * happen.
   *
   set will_drop %random.100%
   *
   if %will_drop% <= 10
      * drop nothing and bail
      halt
   endif
   if %will_drop% <= 50
      mload obj %vnum_trophy1%
   elseif (%will_drop% >= 51 && %will_drop% <= 70)
      mload obj %vnum_trophy2%
   elseif (%will_drop% >= 71 && %will_drop% <= 90)
      mload obj %vnum_trophy3%
   else
      mload obj %vnum_trophy4%
   endif
~
#5517
bl_status~
0 dn 0
faction status~
wait 2
if %actor.quest_variable[Black_Legion:bl_ally]%
   msend %actor% %self.name% tells you, 'You are pledged to the Black
   msend %actor% &0Legion.'
endif
if %actor.quest_variable[Black_Legion:eg_ally]%
   msend %actor% %self.name% tells you, 'You are pledged to the
   msend %actor% &0Eldorian Guard.'
endif
msend %actor%   
msend %actor% Your progress with our cause is as follows:&0
msend %actor% Faction with The Black Legion: %actor.quest_variable[black_legion:bl_faction]%&0
msend %actor% Faction with The Eldorian Guard: %actor.quest_variable[black_legion:eg_faction]%&0
~
#5518
degeneration_cat_greet~
0 g 100
~
wait 2
set stage %actor.quest_stage[degeneration]%
if %actor.class% /= Necromancer
  if %actor.level% > 80
    if %stage% == 0
      mecho %self.name% says, 'Ah, finally a visitor worth my attention.  Have you come to
      mecho &0advance your knowledge of the deathly arts?'
    elseif %stage% == 1
      say Do you have Yajiro's book?
    elseif %stage% == 2
      say Do you have Mesmeriz's necklace?
    elseif %stage% == 3
      say Do you have Luchiaans' mask?
    elseif %stage% == 4
      say Do you have Voliangloch's rod?
    elseif %stage% == 5
      say Do you have Kryzanthor's robe?
    elseif %stage% == 6
      say Do you have Ureal's statuette?
    elseif %stage% == 7 || %stage% == 8
      say Do you have Norisent's book?
    elseif %stage% == 9
      say Do you have the ruby?
    endif
  endif
endif
~
#5519
degeneration_cat_speech1~
0 d 100
yes arts?~
wait 2
if %actor.class% /= Necromancer && %actor.level% > 80 && %actor.quest_stage[degeneration]% == 0
  mecho %self.name% says, 'I've been working for many years on a new spell to suffuse
  mecho &0beings with the energy of the dead.'
  wait 3s
  mecho %self.name% says, 'There are seven dark wizards whose work I believe will help
  mecho &0me complete the spell.  Each one carries a focus I need you to bring back so I
  mecho &0can better understand their spellcraft techniques.'
  wait 6s
  say I'll start you off with the weakest of the targets.
  wait 3s
  mecho %self.name% says, 'On the Emerald Isle is a man named Yajiro.  Although his
  mecho &0magic is underdeveloped, he has somehow managed to summon an impressive demon
  mecho &0and imprison a goddess.'
  wait 6s
  mecho %self.name% says, 'Bring me his book so I may glean what information I can
  mecho &0about his techniques.'
  quest start degeneration %actor.name%
  wait 3s
  mecho %self.name% says, 'If you need, I can remind you of your &7&b[spell progress]&0.'
  wait 2s
  mecho %self.name% says, 'Say &7&b[faction status]&0 if you wish to speak with the Third
  mecho &0Black Legion Quartermaster over there.'
elseif %actor.quest_stage[degeneration]% > 0
  say Give it to me then!
endif
~
#5520
degeneration_cat_speech2~
0 d 100
cat cat?~
wait 2
say Yes, I'm a cat.  A talking cat.  What of it?
wait 4
say I still know more about magic than you do.
hiss %actor.name%
~
#5521
degeneration_cat_receive~
0 j 100
~
set stage %actor.quest_stage[degeneration]%
if %stage% == 0
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  hiss actor
  wait 2
  say Who are you?  Why are you giving me this?
elseif %stage% == 1 && %object.vnum% == 58008
  quest advance degeneration %actor.name%
  wait 1s
  mjunk book
  emote studies the book closely.
  wait 3s
  mecho %self.name% says, 'Hmmm, that's an unusual configuration...  The cage must have
  mecho &0been part of the secret.'
  wait 2s
  emote closes the book.
  wait 2s
  mecho %self.name% says, 'Yajiro was on to something, though his understanding was
  mecho &0quite rudimentary.  I need to dig a little deeper.'
  wait 4s
  say Literally and figuratively!
  wait 2s
  mecho %self.name% says, 'A troll sorcerer by the name of Mesmeriz has set up an
  mecho &0illusory lair in the Minithawkin Mines.'
  wait 4s
  mecho %self.name% says, 'Word among the dark practitioners is, he's trying to perform
  mecho &0the Ritual of Night and become a lich.'
  wait 4s
  mecho %self.name% says, 'Bring back the necklace he carries so I can see how much
  mecho &0progress he's made.'
elseif %stage% == 2 && %object.vnum% == 37015
  quest advance degeneration %actor.name%
  wait 1s
  mjunk necklace
  mecho %self.name% utters the words, '&3&boculoinfra kariq&0'.
  mecho %self.name%'s eyes flash bright yellow!
  wait 1s
  emote examines %get.obj_shortdesc[37015]%
  wait 3s
  mecho %self.name% says, 'Mesmeriz was remarkably close to his goal.  The principles
  mecho &0of energy transfer he was using were quite unique.'
  wait 4s
  say It seems all he was missing was a suitable phylactery.
  wait 3s
  mecho %self.name% says, 'There's another wizard who has been working in body
  mecho &0modification, but on a massive scale.'
  wait 4s
  mecho %self.name% says, 'He's enchanted the entire town of Nordus and is
  mecho &0experimenting on the villagers.  Rumor is he's trying to grow bodies as fodder
  mecho &0for mass animation.'
  wait 5s
  say I must admit, I'm impressed.
  wait 2s
  mecho %self.name% says, 'He has a mask made from the face of one of his many victims
  mecho &0which he uses as his focus.  It's a bit vulgar, but it should serve my
  mecho &0purposes.  Secure it and bring it back.'
elseif %stage% == 3 && %object.vnum% == 51075
  quest advance degeneration %actor.name%
  wait 1
  mjunk mask
  wait 1s
  emote greedily devours the %get.obj_noadesc[51075]%!
  wait 3s
  lick
  say Nothing makes you more familiar with magic than the taste!
  wait 3s
  mecho %self.name% says, 'Some impressive potency to use transmutation at that scale.
  mecho &0Plus some powerful evocation.'
  wait 4s
  burp
  say Spicy!
  wait 2s
  mecho %self.name% says, 'But Luchiaans wasn't a very skilled necromancer it seems.
  mecho &0Still, it gives me some ideas.'
  wait 4s
  mecho %self.name% says, 'I need something of a more demonic nature to compare notes
  mecho &0to.'
  wait 3s
  mecho %self.name% says, 'Cyprianum, the ruler of Demise Keep, has a particularly
  mecho &0prized servant, Voliangloch the Evil.  As a devotee of the demonic Cyprianum,
  mecho &0Voliangloch may have struck that special balance between arcane and divine.'
  wait 7s
  mecho %self.name% says, 'Bring me his magical focus, %get.obj_shortdesc[43020]%.
  mecho &0It should give me sufficient insight to Voliangloch's magic.'
elseif %stage% == 4 && %object.vnum% == 43020
  quest advance degeneration %actor.name%
  wait 1
  mjunk rod
  wait 1s
  emote bats %get.obj_shortdesc[43020]% around a bit.
  wait 3s
  emote bats %get.obj_shortdesc[43020]% around some more.
  wait 5s
  emote frantically swats %get.obj_shortdesc[43020]% around and sends it flying!
  hiss
  wait 3s
  say Well that was most informative.
  wait 3s
  say Interesting blending technique Voliangloch employed.
  wait 3s
  mecho %self.name% says, 'It looks like we have to approach the big players for more
  mecho &0information now.'
  wait 3s
  mecho %self.name% says, 'The first of the true necromancers I need you to "visit" is
  mecho &0Kryzanthor.  He's created a vast necropolis near Anduin, the likes of which
  mecho &0most beings can only dream about!'
  wait 6s
  mecho %self.name% says, 'He wears a unique robe that I'd like to get my paws on.  Get
  mecho &0it and bring it back to me.'
elseif %stage% == 5 && %object.vnum% == 47003
  quest advance degeneration %actor.name%
  wait 1s
  mjunk robe
  emote gingerly sniffs %get.obj_shortdesc[47003]%.
  wait 2s
  mecho %self.name% says, 'Yes, this makes more sense.  Needing a physical conduit to
  mecho &0focus the transference of energies through while simultaneously shielding the
  mecho &0body appears to be critical here.'
  wait 6s
  mecho %self.name% says, 'Kryzanthor had mastered suffusing the dead with animating
  mecho &0energies, but he didn't quite have the mastery of draining life force from
  mecho &0the living.'
  wait 6s
  say There is one whom I have heard does have such a power.
  wait 3s
  mecho %self.name% says, 'In the Iron Hills there is an ancient barrow where several
  mecho &0kings have been laid to rest.  What's more, those of us who deal in death know
  mecho &0it's the lair of one of the two known liches in the world, King Ureal.'
  wait 7s
  mecho %self.name% says, 'In particular, he has a statuette I'm most interested in.
  mecho &0It supposedly can drain the energy of the living.'
  wait 5s
  mecho %self.name% says, 'If that's true, then it would be a huge missing piece of the
  mecho &0puzzle.'
  wait 3s
  say I'm quite excited for this one, so hurry up!
elseif %stage% == 6 && %object.vnum% == 48009
  quest advance degeneration %actor.name%
  wait 1s
  mjunk statuette
  say Well done my little one, well done.
  emote meows in gratitude.
  wait 2s
  emote repeatedly rubs up against the statuette.
  mecho The statuette begins to glow &2&bbright green!&0
  wait 1s
  say Fascinating...
  wait 3s
  say This is definitely helpful.
  wait 4s
  mecho %self.name% says, 'The last person I need information from is a curmudgeonly
  mecho &0old thing, Norisent.'
  wait 4s
  mecho %self.name% says, 'He sought to raise the dead, not just in a state of undeath,
  mecho &0but to truly restore them to life.'
  wait 4s
  mecho %self.name% says, 'Unfortunately for him, his experiments failed and he turned
  mecho &0himself into a lich instead.'
  wait 2s
  roll
  wait 4s
  mecho %self.name% says, 'Regardless, he's been hiding out at the Cathedral of
  mecho &0Betrayal for decades now.'
  wait 4s
  mecho %self.name% says, 'Find Norisent and whatever book he's keeping notes in these
  mecho &0days.  While I think I could manage this without his notes, I want to be
  mecho &0completely sure.'
elseif %stage% == 8 && %object.vnum% == 8551
  quest advance degeneration %actor.name%
  wait 1s
  mjunk book
  emote flips through the pages of the book.
  wait 2s
  gasp
  wait 1s
  mecho %self.name% says, 'It's a good thing I have these notes or my planned matrix
  mecho &0configuration would have completely backfired!  I'll make the necessary
  mecho &0adjustments to the final casting.'
  wait 2s
  mecho %self.name% says, 'The other thing these notes show me is I'm missing
  mecho &0something to channel the transferring energy through.'
  wait 4s
  say Hmmmmm...
  wait 2s
  mecho %self.name% says, 'The statuette is too strongly attuned to Ureal to be
  mecho &0suitable...'
  wait 3s
  say Let me see...
  wait 3s
  mecho %self.name% utters the words, &3&b'hiqi avykamina'&0.
  wait 4s
  mecho %self.name% says, 'It seems there is a dangerous ruby hidden under some kind
  mecho &0of stairway?  That makes no sense.  Yet my divination indicates it would be an
  mecho &0acceptable conduit for this spell.'
  wait 6s
  say Seek it out!
elseif %stage% == 9 && %object.vnum% == 12526
  wait 1s
  mjunk ruby
  mecho %self.name%'s eyes widen and gleam at the sight of the ruby!
  say Magnificent!  I've never seen anything quite like it.
  wait 3s
  emote scratches out a huge diagram in the earth inside the tent.
  mecho %self.name% carefully places %get.obj_shortdesc[12526]% in the center of the diagram.
  wait 4s
  mecho %self.name% utters the word, &3&b'oculotunsofihuab'&0.
  mecho Waves of necrotic energy rush blast out from the diagram bolstering the forces of the Third Black Legion!
  wait 4s
  cackle
  mecho %self.name% says, 'I've done it!  I've perfected the formula for Degeneration!!
  mecho &0Here, see how it works!!'
  wait 4s
  msend %actor% Looking at the notes, you understand what %self.name% has done.
  msend %actor% &7&bYou have learned &9Degeneration&7!&0
  mskillset %actor.name% degeneration
  quest complete degeneration %actor.name%
else
  return 0
  hiss %actor%
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  mecho %self.name% says, 'This isn't what I asked for. What are you, stupid?
  mecho &0Do you need me to remind you of your &7&b[spell progress]&0?'
endif
~
#5522
degeneration_status_checker~
0 d 0
spell progress~
wait 2
set stage %actor.quest_stage[degeneration]%
say You are trying to help me finish the spell Degeneration.
mecho   
if %stage% == 1
  say Find Yajiro in Odaishyozen and bring back his book.
elseif %stage% == 2
  mecho %self.name% says, 'Find Mesmeriz in the Minithawkin Mines and bring back his
  mecho &0necklace.'
elseif %stage% == 3
  say Find Luchiaans in Nordus and bring back his mask.
elseif %stage% == 4
  say Find Voliangloch in Demise Keep and bring back his rod.
elseif %stage% == 5
  say Find Kryzanthor in the Graveyard and bring back his robe.
elseif %stage% == 6
  mecho %self.name% says, 'Find Ureal the Lich in the Barrow and bring back his
  mecho &0statuette.'
elseif %stage% == 7 || %stage% == 8
  mecho %self.name% says, 'Find Norisent in the Cathedral of Betrayal and bring back
  mecho &0his book.'
elseif %stage% == 9
  say Find the enormous ruby hidden under a stairway.
endif
~
#5523
Norisent_death_degeneration~
0 f 100
~
set i %actor.group_size%
if %i%
   set a 1
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[degeneration]% == 7
            quest advance degeneration %person%
            mload obj 8551
            mecho &2&bA small book slips from %self.name%'s robes.&0
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %actor.quest_stage[degeneration]% == 7
   quest advance degeneration %actor%
   mload obj 8551
   mecho &2&bA small book slips from %self.name%'s robes.&0
endif
~
#5524
Eldoria Quartermasters load~
0 o 100
~
if %self.vnum% == 5512
  set vnum_trophy1           5504
  set vnum_trophy2           5506
  set vnum_trophy3           5508
  set vnum_trophy4           5510
  set vnum_trophy5           5512
  set vnum_trophy6           5514
  set vnum_trophy7           5516
  set vnum_gem_3bl_cap       55570
  set vnum_gem_3bl_neck      55571
  set vnum_gem_3bl_arm       55572
  set vnum_gem_3bl_wrist     55573
  set vnum_gem_3bl_gloves    55574
  set vnum_gem_3bl_jerkin    55575
  set vnum_gem_3bl_robe      55589
  set vnum_gem_3bl_belt      55576
  set vnum_gem_3bl_legs      55577
  set vnum_gem_3bl_boots     55578
  set vnum_gem_3bl_mask      55579
  set vnum_gem_3bl_symbol    55580
  set vnum_gem_3bl_staff     55581
  set vnum_gem_3bl_ssword    55582
  set vnum_gem_3bl_whammer   55583
  set vnum_gem_3bl_flail     55584
  set vnum_gem_3bl_shiv      55585
  set vnum_gem_3bl_lsword    55586
  set vnum_gem_3bl_smace     55587
  set vnum_gem_3bl_light     55588
  set vnum_gem_3bl_food      55566
  set vnum_gem_3bl_drink     55567
  set vnum_3bl_cap           5517
  set vnum_3bl_neck          5519
  set vnum_3bl_arm           5521
  set vnum_3bl_wrist         5523
  set vnum_3bl_gloves        5525
  set vnum_3bl_jerkin        5527
  set vnum_3bl_belt          5529
  set vnum_3bl_legs          5531
  set vnum_3bl_boots         5533
  set vnum_3bl_mask          5535
  set vnum_3bl_robe          5537
  set vnum_3bl_symbol        5515
  set vnum_3bl_staff         5539
  set vnum_3bl_ssword        5540
  set vnum_3bl_whammer       5541
  set vnum_3bl_flail         5542
  set vnum_3bl_shiv          5543
  set vnum_3bl_lsword        5544
  set vnum_3bl_smace         5545
  set vnum_3bl_light         5553
  set vnum_3bl_food          5555
  set vnum_3bl_drink         5557
  global vnum_gem_3bl_cap vnum_gem_3bl_neck vnum_gem_3bl_arm vnum_gem_3bl_wrist vnum_gem_3bl_gloves vnum_gem_3bl_jerkin vnum_gem_3bl_robe vnum_gem_3bl_belt vnum_gem_3bl_legs vnum_gem_3bl_boots vnum_gem_3bl_mask vnum_gem_3bl_symbol vnum_gem_3bl_staff vnum_gem_3bl_ssword vnum_gem_3bl_whammer vnum_gem_3bl_flail vnum_gem_3bl_shiv vnum_gem_3bl_lsword vnum_gem_3bl_smace vnum_gem_3bl_light vnum_gem_3bl_food vnum_gem_3bl_drink vnum_3bl_cap vnum_3bl_neck vnum_3bl_arm vnum_3bl_wrist vnum_3bl_gloves vnum_3bl_jerkin 
num_3bl_belt vnum_3bl_legs vnum_3bl_boots vnum_3bl_mask vnum_3bl_robe vnum_3bl_symbol vnum_3bl_staff vnum_3bl_ssword vnum_3bl_whammer vnum_3bl_flail vnum_3bl_shiv vnum_3bl_lsword vnum_3bl_smace vnum_3bl_light vnum_3bl_food vnum_3bl_drink
elseif %self.vnum% == 5524
  set vnum_trophy1           5503
  set vnum_trophy2           5505
  set vnum_trophy3           5507
  set vnum_trophy4           5509
  set vnum_trophy5           5511
  set vnum_trophy6           5513
  set vnum_trophy7           5515
  set vnum_gem_3eg_cap       55570
  set vnum_gem_3eg_neck      55571
  set vnum_gem_3eg_arm       55572
  set vnum_gem_3eg_wrist     55573
  set vnum_gem_3eg_gloves    55574
  set vnum_gem_3eg_jerkin    55575
  set vnum_gem_3eg_robe      55589
  set vnum_gem_3eg_belt      55576
  set vnum_gem_3eg_legs      55577
  set vnum_gem_3eg_boots     55578
  set vnum_gem_3eg_mask      55579
  set vnum_gem_3eg_symbol    55580
  set vnum_gem_3eg_staff     55581
  set vnum_gem_3eg_ssword    55582
  set vnum_gem_3eg_whammer   55583
  set vnum_gem_3eg_flail     55584
  set vnum_gem_3eg_shiv      55585
  set vnum_gem_3eg_lsword    55586
  set vnum_gem_3eg_smace     55587
  set vnum_gem_3eg_light     55588
  set vnum_gem_3eg_food      55566
  set vnum_gem_3eg_drink     55567
  set vnum_3eg_cap           5518
  set vnum_3eg_neck          5520
  set vnum_3eg_arm           5522
  set vnum_3eg_wrist         5524
  set vnum_3eg_gloves        5526
  set vnum_3eg_jerkin        5528
  set vnum_3eg_belt          5530
  set vnum_3eg_legs          5532
  set vnum_3eg_boots         5534
  set vnum_3eg_mask          5536
  set vnum_3eg_robe          5538
  set vnum_3eg_symbol        5516
  set vnum_3eg_staff         5546
  set vnum_3eg_ssword        5547
  set vnum_3eg_whammer       5548
  set vnum_3eg_flail         5549
  set vnum_3eg_shiv          5550
  set vnum_3eg_lsword        5551
  set vnum_3eg_smace         5552
  set vnum_3eg_light         5554
  set vnum_3eg_food          5556
  set vnum_3eg_drink         5558
  global vnum_gem_3eg_cap vnum_gem_3eg_neck vnum_gem_3eg_arm vnum_gem_3eg_wrist vnum_gem_3eg_gloves vnum_gem_3eg_jerkin vnum_gem_3eg_robe vnum_gem_3eg_belt vnum_gem_3eg_legs vnum_gem_3eg_boots vnum_gem_3eg_mask vnum_gem_3eg_symbol vnum_gem_3eg_staff vnum_gem_3eg_ssword vnum_gem_3eg_whammer vnum_gem_3eg_flail vnum_gem_3eg_shiv vnum_gem_3eg_lsword vnum_gem_3eg_smace vnum_gem_3eg_light vnum_gem_3eg_food vnum_gem_3eg_drink vnum_3eg_cap vnum_3eg_neck vnum_3eg_arm vnum_3eg_wrist vnum_3eg_gloves vnum_3eg_jerkin 
num_3eg_belt vnum_3eg_legs vnum_3eg_boots vnum_3eg_mask vnum_3eg_robe vnum_3eg_symbol vnum_3eg_staff vnum_3eg_ssword vnum_3eg_whammer vnum_3eg_flail vnum_3eg_shiv vnum_3eg_lsword vnum_3eg_smace vnum_3eg_light vnum_3eg_food vnum_3eg_drink
endif
global vnum_trophy1 vnum_trophy2 vnum_trophy3 vnum_trophy4 vnum_trophy5 vnum_trophy6 vnum_trophy7
~
$~
