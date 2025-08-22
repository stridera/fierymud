#4101
3bl_qm_greet~
0 g 100
~
wait 2
if (%actor.align% <= 150 && %actor.quest_stage[Black_Legion]% == 0)
   msend %actor% %self.name% tells you, 'Hrm, fresh meat to fight the reach of the
   msend %actor% &0elven scum?'
endif
~
#4102
3bl_qm_initiate~
0 dn 100
quest emissary elven scum hi hello yes~
*
* This will recognize the acceptance of the Eldorian combat quests
* and set the quest variable for later interaction.
*
if %actor.quest_variable[Black_Legion:eg_ally]%
   msend %actor% %self.name% tells you, 'You have already chosen your side!  Be
   msend %actor% &0gone, filth!'
   halt
endif
* This is for neutrals and evils only.
if (%actor.align% <= 150 && %actor.level% > 9)
   wait 2
   if %actor.quest_stage[Black_Legion]% == 0
      quest start Black_Legion %actor.name%
      quest variable Black_Legion %actor.name% BL_FACTION 0
      quest variable Black_Legion %actor.name% EG_FACTION 0
      quest variable Black_Legion %actor.name% bl_ally 1
      * Note that it is not necessary to initialize sub-quest
      * variables to 0 because non-existent quest variables
      * return the same as variables set to 0 in conditionals
   endif
   if (%actor.quest_stage[Black_Legion]% > 0 && %actor.quest_variable[black_legion:bl_faction]% > 99)
      msend %actor%
      msend %actor% %self.name% tells you, 'Yes, your service to the Legion has been
      msend %actor% &0acceptable.  Perhaps you can help on the front lines in our assault in Eldoria.
      msend %actor%   
      msend %actor% %self.name% tells you, 'Go at once to Tarelithis!  Seek out the      
      msend %actor% &0Third Black Legion Recruiter for further instructions!'
      halt
   endif
   msend %actor%
   msend %actor% %self.name% tells you, 'Yes, You will need to assist us in order to
   msend %actor% &0curry our favor.'
   msend %actor%    
   msend %actor% %self.name% tells you, 'We have word the monks of the western vales
   msend %actor% &0are harboring agents of the Eldorian guard.  Go, raid the Abbey and lay waste
   msend %actor% &0to them and their ilk!'
   msend %actor%  
   msend %actor% %self.name% tells you, 'If you bring me back &7&b[trophies]&0 of your
   msend %actor% &0victories, I will &7&b[reward]&0 you!'
   msend %actor%    
  msend %actor% %self.name% tells you, 'If you like you can ask your &7&b[faction
  msend %actor% &0&7&bstatus]&0.'
endif
~
#4103
3bl_turn_in~
0 j 100
~
*
* This is the main receive trigger for the Edorian
* combat quests installed on the mud. This trigger
* will be generic and the vnums and variables set
* to reflect which faction it applies to.
*
* The original zone using these quests was
* 55 Combat in Eldoria.  I'm adapting the
* format of the quest mechanics to spread the
* vast wealth of these quests to other zones
* such that there isn't so much loot in one zone.
*
* 3bl is Third Black Legion - The player MUST be 
* on the quest already in order to turn in because
* the variables are not set yet!
if %actor.quest_variable[Black_Legion:eg_ally]%
   msend %actor% %self.name% tells you, 'You have already chosen your side!  Be
   msend %actor% &0gone, filth!'
   halt
endif
*
if (%actor.align% <= 150 && %actor.quest_stage[Black_Legion]% > 0)
   quest variable Black_Legion %actor.name% bl_ally 1
   *
   * pertinent object definitions for this faction
   * trophies
   *
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
   * These cases set the variables for the quests.
   * Note the object.vnum is the vnum of the object handed to
   * the NPC by the player.
   *
   * These cases will only have the objects rewarded by this
   * questmaster though the full set of objects will be defined
   * by the script.
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
      if (%actor.quest_variable[black_legion:bl_faction]% < 100)
         eval trophies %actor.quest_variable[black_legion:%vnum_trophy%_trophies]% + 1
         quest variable black_legion %actor.name% %vnum_trophy%_trophies %trophies%
         wait 2
         msend %actor% %self.name% tells you, 'Hrm, I see you have been out raiding the
         msend %actor% &0monks assisting the Eldorians.  According to my records you have now turned in
         msend %actor% &0have now turned in &3&b%trophies%&0 &7&b%get.obj_shortdesc[%vnum_trophy%]%&0.'
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
         msend %actor% %self.name% tells you, 'You have garnered as much favor with the
         msend %actor% &0Black Legion as you possibly can by raiding the Abbey.'
         msend %actor%   
         msend %actor% %self.name% tells you, 'You may find other more difficult battles
         msend %actor% &0in Eldoria.'
         msend %actor%   
         msend %actor% %self.name% tells you, 'Seek out the Third Black Legion Recruiter
         msend %actor% &0at the base of Tarelithis in order to curry more favor with our
         msend %actor% &0lords.'
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
            *
            * slight bug for now the trigger code can't seem to evaluate negative numbers?
            *
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
         msend %actor% %self.name% tells you, 'You aren't quite evil enough for these
         msend %actor% &0rewards.'
         halt
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= %faction_required%)
         eval rewards %actor.quest_variable[black_legion:%vnum_reward%_reward]% + 1
         quest variable black_legion %actor.name% %vnum_reward%_reward %rewards%
         wait 2
         msend %actor% %self.name% tells you, 'Ah yes, the Legion thanks you for your
         msend %actor% &0efforts.  Take this to aid you in your battles.'
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
             *
             * slight bug for now the trigger code can't seem to evaluate negative numbers?
             *
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
         msend %actor% %self.name% tells you, 'You aren't quite allied enough with our
         msend %actor% &0cause to earn that reward.'
         halt        
      endif
      * end of if_gem section 
   endif
else
   * Alignment not low enough or hasn't started the quest yet.
   return 0
   wait 2
   msend %actor% %self.name% tells you, 'You aren't quite allied enough with our
   msend %actor% &0cause to earn our rewards.  You need a lower alignement, or to start these
   msend %actor% &0quests by saying &7&b[yes]&0, &7&b[quest]&0, or &7&b[hello]&0.'
endif
~
#4104
3bl_rewards_list~
0 dn 100
reward rewards~
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
if %actor.quest_variable[Black_Legion:eg_ally]%
   msend %actor% %self.name% tells you, 'You have already chosen your side!  Be
   msend %actor% &0gone, filth!'
   halt
endif
if (%actor.align% <= 150)
   if (%actor.quest_stage[Black_Legion]% > 0)
      if (%actor.quest_variable[black_legion:bl_faction]% < 20)
         msend %actor% 
         msend %actor% %self.name% tells you, 'You are a bit raw for a recruit, come back
         msend %actor% &0when you have defeated more of the Eldorian villainy.'
      endif
      if (%actor.quest_variable[black_legion:bl_faction]% >= 20)
         msend %actor%
         * msend %actor% %self.name% tells you,'Your bl faction is %actor.quest_variable[black_legion:bl_faction]%. 
         msend %actor% %self.name% tells you, 'Though you are a bit wet behind the ears I
         msend %actor% &0suppose we can trust you with some of our goods.'
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
      if (%actor.quest_variable[black_legion:bl_faction]% < 70)
         msend %actor%
         msend %actor% %self.name% tells you, 'As your standing with the Black Legion
         msend %actor% &0improves I will show you more rewards.'
      endif
   endif
endif
~
#4105
3eg_qm_greet~
0 g 100
~
wait 2
if (%actor.align% >= -150 && %actor.quest_stage[Black_Legion]% == 0) 
   msend %actor% %self.name% tells you, 'Hrm, a poor sod to fight the undead masses?'
endif
~
#4106
3eg_qm_initiate~
0 dn 100
quest sod undead masses hi hello yes~
*
* This will recognize the acceptance of the Eldorian combat quests
* and set the quest variable for later interaction.
*
* This is for neutrals and goods only.
*
wait 2
if %actor.quest_variable[Black_Legion:bl_ally]%
   msend %actor% %self.name% tells you, 'You have pledged yourself to the forces of
   msend %actor% &0darkness!  Suffer under your choice!'
   halt
endif
if (%actor.align% >= -150)
   if %actor.quest_stage[Black_Legion]% == 0
      quest start Black_Legion %actor.name%
      quest variable Black_Legion %actor.name% BL_FACTION 0
      quest variable Black_Legion %actor.name% EG_FACTION 0
      quest variable Black_Legion %actor.name% eg_ally 1
      * Note that it is not necessary to initialize sub-quest
      * variables to 0 because non-existent quest variables
      * return the same as variables set to 0 in conditionals
   endif
   if (%actor.quest_stage[Black_Legion]% > 0 && %actor.quest_variable[black_legion:eg_faction]% > 99)
      msend %actor%
      msend %actor% %self.name% tells you, 'Your help to us has not gone unnoticed.  I 
      msend %actor% &0wonder...  Perhaps you can help on the front lines in our assault in Eldoria.'
      msend %actor%  
      msend %actor% %self.name% tells you, 'Go at once to Tarelithis!  Seek out the Third
      msend %actor% &0Eldorian Guard Recruiter for further instructions!
      halt
   endif
   msend %actor%
   msend %actor% %self.name% tells you, 'Hrm, you will need to go assist us in our
   msend %actor% efforts in thwarting the Black Legion's influence.'
   msend %actor%   
   msend %actor% %self.name% tells you, 'We understand that the Legion is in cahoots
   msend %actor% &0with the trolls of Split Skull.  If you wish to gain favor with the Eldorian
   msend %actor% &0Court then raid Split Skull.  Lay waste to their ranks!'
   msend %actor%  
   msend %actor% %self.name% tells you, 'If you bring me back &7&b[trophies]&0 of your
   msend %actor% &0victories, I will &7&b[reward]&0 you!'
   msend %actor%  
   msend %actor% %self.name% tells you, 'If you like you can ask your &7&b[faction
   msend %actor% &0&7&bstatus]&0.'
   msend %actor%
endif
~
#4107
3eg_turn_in~
0 j 100
~
*
* This is the main receive trigger for the Edorian
* combat quests installed on the mud. This trigger
* will be generic and the vnums and variables set
* to reflect which faction it applies to.
*
if %actor.quest_variable[Black_Legion:bl_ally]%
   msend %actor% %self.name% tells you, 'You have pledged yourself to the forces of
   msend %actor% &0darkness!  Suffer under your choice!'
   halt
endif
* 3eg is Third Eldorian Guard
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
      if (%actor.quest_variable[black_legion:eg_faction]% < 100)
         eval trophies %actor.quest_variable[black_legion:%vnum_trophy%_trophies]% + 1
         quest variable black_legion %actor.name% %vnum_trophy%_trophies %trophies%
         wait 2
         msend %actor% %self.name% tells you, 'Hrm, yes... you have been out fighting the
         msend %actor% &0influence of the Black Legion.  I see from my records you have now turned in
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
         msend %actor% %self.name% tells you, 'You have garnered as much favor with the
         msend %actor% &0Eldorian Guard as you possibly can by raiding Split Skull.'
         msend %actor%   
         msend %actor% %self.name% tells you, 'You will find other more difficult battles
         msend %actor% &0with the Legion that will curry more favor with our masters in Eldoria.'
         msend %actor%   
         msend %actor% %self.name% tells you, 'Go to Tarelithis and seek the Recruiter of
         msend %actor% &0the Third Eldorian Guard!'
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
         msend %actor% %self.name% tells you, 'You aren't quite good enough for these
         msend %actor% &0rewards.'
         halt
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= %faction_required%)
         eval rewards %actor.quest_variable[black_legion:%vnum_reward%_reward]% + 1
         quest variable black_legion %actor.name% %vnum_reward%_reward %rewards%
         wait 2
         msend %actor% %self.name% tells you, 'Ah yes, the Guard thanks you for your
         msend %actor% &0efforts.  Take this to aid you in your battles.'
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
            *
            * slight bug for now the trigger code can't seem to evaluate negative numbers?
            *
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
         msend %actor% %self.name% tells you, 'You aren't quite allied enough with our cause
         msend %actor% &0to earn that reward.' 
         halt        
      endif
   * end of if_gem section
   endif
else
   * Alignment not high enough or hasn't started the quest yet.
   return 0
   wait 2
   msend %actor% %self.name% tells you, 'You aren't quite allied enough with our cause
   msend %actor% &0to earn our rewards.  You need a higher alignment or to start these quests by
   msend %actor% &0saying &7&b[yes]&0 or &7&b[quest]&0 or &7&b[hello]&0.'
endif
~
#4108
3eg_rewards_list~
0 dn 100
reward rewards~
if %actor.quest_variable[Black_Legion:bl_ally]%
   msend %actor% %self.name% tells you, 'You have pledged yourself to the forces of
   msend %actor% &0darkness!  Suffer under your choice!'
   halt
endif
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
* Check faction and react accordingly
*
if (%actor.align% >= -150)
   if (%actor.quest_stage[Black_Legion]% > 0)
      if (%actor.quest_variable[black_legion:eg_faction]% < 20)
         msend %actor% 
         msend %actor% %self.name% tells you, 'You are a bit raw for a recruit, come back
         msend %actor% &0when you have defeated more of the Black Legion hordes.'
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 20)
         msend %actor%
         * msend %actor% %self.name% tells you,'Your bl faction is %actor.quest_variable[black_legion:eg_faction]%. 
         msend %actor% %self.name% tells you, 'Though you are a bit wet behind the ears I
         msend %actor% suppose we can trust you with some of our goods.'
         msend %actor%
         msend %actor% You have access to:
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_food%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_food%]%.&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_drink%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_drink%]%.&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_cap%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_cap%]%.&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_ssword%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_ssword%]%.&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 40)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_neck%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_neck%]%.&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_staff%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_staff%]%.&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 55)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_arm%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_arm%]%.&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_whammer%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_whammer%]%.&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% >= 70)
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_wrist%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_wrist%]%.&0
         msend %actor% - &3&b%get.obj_shortdesc[%vnum_3eg_flail%]%&0 for &5%get.obj_shortdesc[%vnum_gem_3eg_flail%]%.&0
      endif
      if (%actor.quest_variable[black_legion:eg_faction]% < 70)
         msend %actor%
         msend %actor% %self.name% tells you, 'As your standing with the Eldorian Guard
         msend %actor% &0improves I will show you more rewards.'
      endif
      msend %actor%
   endif
endif
~
#4109
3eg_trophy~
0 dn 100
trophy trophies~
if %actor.quest_variable[Black_Legion:bl_ally]%
   msend %actor% %self.name% tells you, 'You have pledged yourself to the forces of
   msend %actor% &0darkness!  Suffer under your choice!'
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
   msend %actor% %self.name% tells you, 'As you fight the allies of the Black Legion
   msend %actor% &0you will periodically find goods on their bodies that we will want in order to
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
#4110
3bl_trophy~
0 dn 100
trophy trophies~
if %actor.quest_variable[Black_Legion:eg_ally]%
   msend %actor% %self.name% tells you, 'You have already chosen your side!  Be
   msend %actor% &0gone, filth!'
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
   msend %actor% %self.name% tells you, 'As you fight the allies of Eldorian Guard
   msend %actor% &0you will periodically find goods on their bodies that we will want in order to
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
#4111
p1_3eg_death~
0 f 100
~
* Adapted from Eldorian quest.  There is no gear to destroy in this rendition.
* mecho %self.name%'s gear is destroyed in the battle!
* mjunk all.eldoria-reward
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
#4112
p2_3eg_death~
0 f 100
~
* Adapted from Eldorian quest.  There is no gear to destroy in this rendition.
* mecho %self.name%'s gear is destroyed in the battle!
* mjunk all.eldoria-reward
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
#4113
p3_3eg_death~
0 f 100
~
* Adapted from Eldorian quest.  There is no gear to destroy in this rendition.
* mecho %self.name%'s gear is destroyed in the battle!
* mjunk all.eldoria-reward
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
#4114
p1_3bl_death~
0 f 100
~
* Adapted from Eldorian quest.  There is no gear to destroy in this rendition.
* mecho %self.name%'s gear is destroyed in the battle!
* mjunk all.eldoria-reward
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
#4115
p2_3bl_death~
0 f 100
~
* Adapted from Eldorian quest.  There is no gear to destroy in this rendition.
* mecho %self.name%'s gear is destroyed in the battle!
* mjunk all.eldoria-reward
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
#4116
p3_3bl_death~
0 f 100
~
* Adapted from Eldorian quest.  There is no gear to destroy in this rendition.
* mecho %self.name%'s gear is destroyed in the battle!
* mjunk all.eldoria-reward
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
#4117
bl_status~
0 dn 0
faction status~
msend %actor%   
if %actor.quest_variable[Black_Legion:bl_ally]%
   msend %actor% %self.name% tells you, 'You are pledged to the Black Legion.'
endif
if %actor.quest_variable[Black_Legion:eg_ally]%
   msend %actor% %self.name% tells you, 'You are pledged to the Eldorian Guard.'
endif
msend %actor% &0Your progress with our cause is as follows:
msend %actor% &0Faction with The Black Legion: %actor.quest_variable[black_legion:bl_faction]%
msend %actor% &0Faction with The Eldorian Guard: %actor.quest_variable[black_legion:eg_faction]%
~
$~
