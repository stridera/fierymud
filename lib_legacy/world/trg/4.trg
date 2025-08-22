#400
Quest Journal~
1 m 100
journal quest quest-journal~
if !%arg%
  osend %actor% This journal tracks all your quest information as you journey through Ethilien!
  osend %actor% &3=======================================&0
  osend %actor% &3OVERVIEW&0
  osend %actor% Quests will be revealed in this journal as you become eligible to take them on.  There are main indexes organized around the type of quest, and then detailed entries for each individual quest.
  osend %actor% The quest journal will always have your most recent personal quest records in it, no matter which copy you look at.
  osend %actor% &0 
  osend %actor% To look at a category of quests, type "look journal (category name)".
  osend %actor% To look at a specific quest, type: "look journal (quest name)".
  osend %actor% &0
  osend %actor% For more on the general quest system, see &6&b[HELP QUEST]&0.
  osend %actor% &3=======================================&0
  osend %actor% There are five types of quests:
  osend %actor% 1. &3&bADVENTURE&0 quests
  osend %actor% - These quests take you through areas and explore the story behind each place you encounter.  These quests often culminate in a boss battle and have excellent rewards!&_
  osend %actor% 2. &3&bEQUIPMENT&0 quests
  osend %actor% - These quests take you all across the world reward you with top-tier equipment.  These often take many levels to complete and require vast exploration, but some are focused on a particular zone.
  osend %actor% 3. &3&bHEROES FOR HIRE&0 quests
  osend %actor% - These quests are types of bounty hunts, tasking you with hunting down a particular target.  They increase in difficulty across a hero's lifetime!
  osend %actor% 4. &3&bSPELL&0 quests
  osend %actor% - These quests challenge you to take on the strongest monsters, find the most valuable treasures, and delve into the deepest secrets Fiery has to offer.  The rewards are supreme magic, unrivaled magic.  Quest songs and chants are included in this category.
  osend %actor% 5. &3&bSUBCLASS&0 quests.
  osend %actor% - These quests can be undertaken by a low-level character to change from a core class to a subclass.  Much more information can be found in &6&b[HELP SUBCLASS]&0.
  osend %actor% - Only characters in core classes within the proper level range will see what quests are available to them on this page.
  osend %actor% &_
  osend %actor% Read or look at any of the five sections above to see what's currently available: ADVENTURE, EQUIPMENT, HIRE, SPELL, SUBCLASS.
  return 0
endif
~
#401
Adventure quests~
1 m 100
journal quest quest-journal~
if %arg% /= adventure || %arg% /= adventures
  return 0
  osend %actor% &3=========== ADVENTURE QUESTS ==========&0
  osend %actor% Adventure is waiting!
  osend %actor% Explore the regions of Ethilien.
  osend %actor% Fight epic monsters, earn legendary rewards, and increase your might.
  osend %actor% &3&b[Look]&0 at the key words in a quest title for your current status.
  osend %actor% &3=======================================&0&_
  osend %actor% &2&bAVAILABLE QUESTS:&0&_
  osend %actor% &2&b&uTwisted Sorrow&0
  osend %actor% Can you heal the sorrows of the Twisted Forest?
  osend %actor% Recommended Level: 10
  if %actor.has_completed[twisted_sorrow]%
    set status Completed!
  elseif %actor.quest_stage[twisted_sorrow]%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  osend %actor% &2&b&uCombat in Eldoria&0
  osend %actor% The Third Black Legion and the Eldorian Guard, along with their allies in Split Skull and the Abbey, are locked in eternal warfare.
  osend %actor% Characters may align themselves with the forces of good or the forces of evil.
  osend %actor% But beware, once made that decision cannot be changed!
  osend %actor% Minimum Level: 10
  if %actor.quest_stage[black_legion]%
    set status Continuous
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  osend %actor% &2&b&uThe Finale&0
  osend %actor% Defeat wild monkeys, recover keys, and help the theatre troupe perform their fiery Finale!
  osend %actor% Recommended Level: 10
  if %actor.quest_stage[theatre]%
    set status Repeatable
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  if %actor.level% >= 10
    osend %actor% &2&b&uThe Horrors of Nukreth Spire&0
    osend %actor% Slay the gnolls of Nukreth Spire and liberate their captives.
    osend %actor% Multiple outcomes and multiple rewards await!
    osend %actor% This quest is infinitely repeatable.
    osend %actor% Recommended Level: 20
    if %actor.quest_stage[nukreth_spire]%
      set status Repeatable
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.level% >= 35
    osend %actor% &2&b&uLiberate Fiery Island&0
    osend %actor% Save Fiery Island from the usurping demigod.
    osend %actor% Minimum Level: 55
    osend %actor% - Some rewards can be received starting at level 35.
    if %actor.has_completed[fieryisle_quest]%
      set status Completed!
    elseif %actor.quest_stage[fieryisle_quest]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.level% >= 30
    osend %actor% &2&b&uTower in the Wastes&0
    osend %actor% Help the injured halfling find his brother!
    osend %actor% Recommended Level: 40
    if %actor.has_completed[krisenna_quest]%
      set status Completed!
    elseif %actor.quest_stage[krisenna_quest]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.level% >= 35
    osend %actor% &2&b&uSiege Mystwatch Fortress&0
    osend %actor% The dead fortify their position just outside the town of Mielikki.  
    osend %actor% Stop them before they can invade!
    osend %actor% Recommended Level: 45
    if %actor.quest_stage[mystwatch_quest]%
      set status Repeatable
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.level% >= 45
    osend %actor% &2&b&uDestroy the Cult of the Griffin&0
    osend %actor% A sect of griffin-worshiping cultists has invaded a druid enclave in the middle of the Arabel Ocean.
    osend %actor% Smash the cult before they achieve their nefarious goals!
    osend %actor% Recommended Level: 60
    if %actor.has_completed[griffin_quest]%
      set status Completed!
    elseif %actor.quest_stage[griffin_quest]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.level% >= 85
    osend %actor% &2&b&uThe Planes of Doom&0
    osend %actor% Prepare yourself to leave the Prime Material Plane for the elemental planes!
    osend %actor% This quest begins your voyage through the outer planes to banish Lokari, God of the Moonless night.
    osend %actor% Minimum Level: 85
    osend %actor% - This quest begins a storyline intended for characters of level 95+
    if %actor.has_completed[doom_entrance]%
      set status Completed!
    elseif %actor.quest_stage[doom_entrance]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
endif
~
#402
Equipment quests~
1 m 100
journal quest quest-journal~
if %arg% /= equipment
  return 0
  set anti Anti-Paladin
  set hunterclasses Warrior Ranger Berserker Mercenary
  set sorcererclasses Sorcerer Illusionist Cryomancer Pyromancer Necromancer
  osend %actor% &3=========== EQUIPMENT QUESTS ==========&0
  osend %actor% These quests often take a large amount of time to complete, spanning dozens of levels.
  osend %actor% Characters should expect to need large numbers of unique and hard-to-find items.
  osend %actor% They are generally not intended to be completed quickly.
  osend %actor% &3&b[Look]&0 at the key words in a quest title for your current status.
  osend %actor% &3=======================================&0&_
  osend %actor% &2&bAVAILABLE QUESTS:&0&_
  osend %actor% &2&b&uGuild Armor Phase One&0
  osend %actor% Crafters across Ethilien will ask you to bring them gems and junked up armor so they can fix it up and present you with something new.
  osend %actor% Your home guild master is always the best place to start.
  osend %actor% Min Level: 1
  if %actor.quest_stage[phase_armor]%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  osend %actor% &2&b&uGuild Armor Phase Two&0
  osend %actor% Crafters across Ethilien will ask you to bring them gems and junked up armor so they can fix it up and present you with something new.
  osend %actor% Min Level: 21
  if %actor.quest_stage[phase_armor]% >= 2
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  osend %actor% &2&b&uGuild Armor Phase Three&0
  osend %actor% Crafters across Ethilien will ask you to bring them gems and junked up armor so they can fix it up and present you with something new.
  osend %actor% Min Level: 41
  if %actor.quest_stage[phase_armor]% >= 3
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  if %sorcererclasses% /= %actor.class%
    osend %actor% &2&b&uAcid Wand&0
    osend %actor% Masters of earth will help you create and upgrade a new mystic weapon.
    if %actor.has_completed[acid_wand]%
      set status Completed!
    elseif %actor.quest_stage[acid_wand]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    osend %actor% &2&b&uAir Wand&0
    osend %actor% Masters of air will help you create and upgrade a new mystic weapon.
    if %actor.has_completed[air_wand]%
      set status Completed!
    elseif %actor.quest_stage[air_wand]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    osend %actor% &2&b&uFire Wand&0
    osend %actor% Masters of fire will help you create and upgrade a new mystic weapon.
    if %actor.has_completed[fire_wand]%
      set status Completed!
    elseif %actor.quest_stage[fire_wand]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    osend %actor% &2&b&uIce Wand&0
    osend %actor% Masters of ice and water will help you create and upgrade a new mystic weapon.
    if %actor.has_completed[ice_wand]%
      set status Completed!
    elseif %actor.quest_stage[ice_wand]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.class% /= Assassin
    osend %actor% &2&b&uDeadly Promotion&0
    osend %actor% Assassins who have proven to be efficient bounty hunters can work their way up in the Guild to earn special masks.
    if %actor.has_completed[assassin_mask]%
      set status Completed!
    elseif %actor.quest_stage[assassin_mask]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.class% /= Priest || %actor.class% /= Cleric
    osend %actor% Clerics and priests can make pilgrimages to various spiritual masters to craft weapons to smite the undead.
    osend %actor% &2&b&uSpirit Maces&0
    if %actor.has_completed[phase_mace]%
      set status Completed!
    elseif %actor.quest_stage[phase_mace]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.class% /= Paladin || %actor.class% == %anti%
    osend %actor% &2&b&uDivine Devotion&0
    osend %actor% Prove your devotion to your cause after slaying a dragon, be it justtice or destruction, and receive a divine reward.
    if %actor.has_completed[paladin_pendant]%
      set status Completed!
    elseif %actor.quest_stage[paladin_pendant]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %hunterclasses% /= %actor.class%
    osend %actor% &2&b&uEye of the Tiger&0
    osend %actor% Mighty warriors who have bested the biggest beasts can further prove their skills to earn special trophies.
    if %actor.has_completed[ranger_trophy]%
      set status Completed!
    elseif %actor.quest_stage[ranger_trophy]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.class% /= thief || %actor.class% /= rogue || %actor.class% /= bard
    osend %actor% &2&b&uCloak and Shadow&0
    osend %actor% Work your way up the ranks of the cloak and dagger guilds.
    if %actor.has_completed[ranger_trophy]%
      set status Completed!
    elseif %actor.quest_stage[ranger_trophy]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.level% >= 25
    osend %actor% &2&b&uIn Service of Lolth&0
    osend %actor% Carry out the bidding of the hideous Spider Queen and her malevolent servants.
    osend %actor% This quest is only available to neutral and evil-aligned characters.
    osend %actor% Recommended Level: 90
    osend %actor% - This quest starts at level 25 and continues through level 90.
    if %actor.has_completed[vilekka_stew]%
      set status Completed!
    elseif %actor.quest_stage[vilekka_stew]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    osend %actor% &2&b&uInfiltrate the Sacred Haven&0
    osend %actor% A shadowy figure lurks outside a holy fortress, intent on getting back what belongs to them.
    osend %actor% This quest is only available to neutral and evil-aligned characters.
    osend %actor% Recommended Level: 35
    if %actor.quest_stage[sacred_haven]%
      set status Repeatable
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.level% >= 35
    osend %actor% &2&b&uMystery of the Rhell Forest&0
    osend %actor% Find the sick merchant in the Rhell Forest and help end his distress.
    osend %actor% Recommended Level: 45
    if %actor.has_completed[ursa_quest]%
      set status Completed!
    elseif %actor.quest_stage[ursa_quest]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.level% >= 45
    if %actor.race% /= Troll
      osend %actor% &2&b&uTribal Trouble&0
      osend %actor% Trolls need to stick together!
      osend %actor% Return lost symbols of station and be greatly rewarded.
      osend %actor% Minimum Level: 55
      if %actor.has_completed[troll_quest]%
        set status Completed!
      elseif %actor.quest_stage[troll_mask]%
        set status In Progress
      else
        set status Not Started
      endif
      osend %actor% &6Status: %status%&0&_
    endif
  endif
  if %actor.level% >= 50
    osend %actor% &2&b&uThe Great Rite&0
    osend %actor% A group of witches has uncovered a powerful set of massive standing stones.
    osend %actor% Help them with their mystic ritual.
    osend %actor% Recommended Level: 70
    osend %actor% - This quest can be started at any level but requires level 70 to finish.
    if %actor.has_completed[megalith_quest]%
      set status Completed!
    elseif %actor.has_failed[megalith_quest]%
      set status Failed
    elseif %actor.quest_stage[megalith_quest]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    osend %actor% &2&b&uPhoenix Sous Chef&0
    osend %actor% Create the meal of a lifetime!
    osend %actor% Recommended Level: 90
    osend %actor% - This quest can be started at any level but level 90+ is the intended finishing point.
    if %actor.has_completed[resort_cooking]%
      set status Completed!
    elseif %actor.quest_stage[resort_cooking]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.level% >= 75
    osend %actor% &2&b&uPower of Flame&0
    osend %actor% Reunite the flame to hold its power in your hand.
    osend %actor% Recommended Level: 85
    osend %actor% - This quest can be started at any level but requires level 85 to finish.
    osend %actor% - This quest is repeatable.
    if %actor.has_completed[emmath_flameball]%
      set status Completed!
    elseif %actor.quest_stage[emmath_flameball]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
  if %actor.level% >= 80
    osend %actor% &2&b&uSunfire Rescue&0
    osend %actor% Recover ancient relics and rescue a long-lost prisoner.
    osend %actor% This quest is only available to good-aligned characters.
    osend %actor% Recommended Level: 85
    osend %actor% - This quest can be started at any level but requires level 85 to finish.
    if %actor.has_completed[sunfire_rescue]%
      set status Completed!
    elseif %actor.quest_stage[sunfire_rescue]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
  endif
endif
~
#403
Heroes for Hire quests~
1 m 100
journal quest quest-journal~
if %arg% /= heroes || %arg% /= hire
  return 0
  osend %actor% &3=========== HEROES FOR HIRE ===========&0
  osend %actor% These hired jobs provide some quick cash and good thrills with some of Ethilien's most iconic creatures.
  osend %actor% &3&b[Look]&0 at the key words in a quest title for your current status.
  osend %actor% &3=======================================&0&_
  osend %actor% &2&bAVAILABLE QUESTS:&0&_
  osend %actor% &2&b&uBeast Masters&0
  osend %actor% Battle the most ferocious animals on Ethilien, mundane and magical alike.
  osend %actor% This quest spans from level 1 to 90.
  if %actor.has_completed[beast_master]%
    set status Completed!
  elseif %actor.quest_stage[bounty_hunt]%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  osend %actor% &2&b&uContract Killers&0
  osend %actor% Sow some chaos by taking out key figures across the world.
  osend %actor% This quest spans from level 1 to 90.
  if %actor.has_completed[bounty_hunt]%
    set status Completed!
  elseif %actor.quest_stage[bounty_hunt]%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  osend %actor% &2&b&uDragon Slayers&0
  osend %actor% Take on those most awe-inspiring of creatures: the dragons!
  osend %actor% This quest spans from level 5 to 90.
  if %actor.has_completed[dragon_slayer]%
    set status Completed!
  elseif %actor.quest_stage[dragon_slayer]%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  osend %actor% &2&b&uTreasure Hunter&0
  osend %actor% Delve deep for legendary treasures!
  osend %actor% This quest spans from level 1 to 90.
  if %actor.has_completed[treasure_hunter]%
    set status Completed!
  elseif %actor.quest_stage[treasure_hunter]%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  osend %actor% &2&b&uElemental Chaos&0
  osend %actor% Disperse the forces of Chaos and restore Balance to Ethilien.
  osend %actor% This quest spans from level 1 to 90.
  if %actor.has_completed[elemental_chaos]%
    set status Completed!
  elseif %actor.quest_stage[elemental_chaos]%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
endif
~
#404
Spell quests~
1 m 100
journal quest quest-journal~
if %arg% /= spell || %arg% /= spells || %arg% /= chant || %arg% /= chants || %arg% /= song || %arg% /= songs || %arg% /= music
  return 0
  set relocateclasses Sorcerer Cryomancer Pyromancer
  set spellquestclasses Ranger Druid Sorcerer Illusionist Cryomancer Pyromancer Diabolist Cleric Priest Bard Necromancer Monk
  osend %actor% &3==== SPELL, CHANT, AND SONG QUESTS ====&0
  if %spellquestclasses% /= %actor.class%
    osend %actor% &3&b[Look]&0 at the key words in a quest title for your current status.
    osend %actor% &3=======================================&0&_
    osend %actor% &2&bAVAILABLE QUESTS:&0&_
    if %actor.level% >= 30
      if %actor.class% /= Monk
        osend %actor% &2&b&uTremors of Saint Augustine&0
        osend %actor% Minimum Level: 30
        if %actor.quest_stage[monk_chants]% > 1
          set status Completed!
        elseif %actor.quest_stage[monk_chants]% == 1
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
        osend %actor% &2&b&uTempest of Saint Augustine&0
        osend %actor% Minimum Level: 40
        if %actor.quest_stage[monk_chants]% > 2
          set status Completed!
        elseif %actor.quest_stage[monk_chants]% == 2
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
        osend %actor% &2&b&uBlizzards of Saint Augustine&0
        osend %actor% Minimum Level: 50
        if %actor.quest_stage[monk_chants]% > 3
          set status Completed!
        elseif %actor.quest_stage[monk_chants]% == 3
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %relocateclasses% /= %actor.class%
        osend %actor% &2&b&uMajor Globe&0
        osend %actor% Minimum Level: 57
        if %actor.has_completed[major_globe_spell]%
          set status Completed!
        elseif %actor.quest_stage[major_globe_spell]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Priest || %actor.class% /= Cleric || %actor.class% /= Diabolist
        osend %actor% &2&b&uGroup Heal&0
        osend %actor% Minimum Level: 57
        if %actor.has_completed[group_heal]%
          set status Completed!
        elseif %actor.quest_stage[group_heal]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Priest
        osend %actor% &2&b&uGroup Armor&0
        osend %actor% Minimum Level: 57
        if %actor.has_completed[group_armor]%
          set status Completed!
        elseif %actor.quest_stage[group_armor]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Cryomancer
        osend %actor% &2&b&uWall of Ice&0
        osend %actor% Minimum Level: 57
        if %actor.has_completed[wall_ice]%
          set status Completed!
        elseif %actor.quest_stage[wall_ice]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Illusionist || %actor.class% /= Bard
        osend %actor% &2&b&uIllusory Wall&0
        osend %actor% Minimum Level: 57
        if %actor.has_completed[illusory_wall]%
          set status Completed!
        elseif %actor.quest_stage[illusory_wall]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Diabolist
        osend %actor% &2&b&uHellfire and Brimstone&
        osend %actor% Minimum Level: 57
        if %actor.has_completed[hellfire_brimstone]%
          set status Completed!
        elseif %actor.quest_stage[hellfire_brimstone]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Monk
        osend %actor% &2&b&uAria of Dissonance&0
        osend %actor% Minimum Level: 60
        if %actor.quest_stage[monk_chants]% > 4
          set status Completed!
        elseif %actor.quest_stage[monk_chants]% == 4
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
    endif
    if %actor.level% >= 60
      if %relocateclasses% /= %actor.class%
        osend %actor% &2&b&uRelocate&0
        osend %actor% Minimum Level: 65
        if %actor.has_completed[relocate_spell_quest]%
          set status Completed!
        elseif %actor.quest_stage[relocate_spell_quest]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Priest || %actor.class% /= Diabolist
        osend %actor% &2&b&uBanish&0
        osend %actor% Minimum Level: 65
        if %actor.has_completed[banish]%
          set status Completed!
        elseif %actor.quest_stage[banish]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
    endif
    if %actor.level% >= 65
      if %actor.class% /= Bard
        osend %actor% &2&b&uHearthsong&0
        osend %actor% Minimum Level: 70
        osend %actor% Note: This chant must be granted directly by the gods and cannot be tracked in the quest journal.&_
      endif
      if %actor.class% /= Cleric
        osend %actor% &2&b&uGroup Armor&0
        osend %actor% Minimum Level: 73
        if %actor.has_completed[group_armor]%
          set status Completed!
        elseif %actor.quest_stage[group_armor]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
        osend %actor% &2&b&uGroup Recall&0
        osend %actor% Minimum Level: 73
        osend %actor% Note: This chant must be granted directly by the gods and cannot be tracked in the quest journal.&_
      endif
      if %actor.class% /= Druid
        osend %actor% &2&b&uMoonwell&0
        osend %actor% Minimum Level: 73
        if %actor.has_completed[moonwell_spell_quest]%
          set status Completed!
        elseif %actor.quest_stage[moonwell_spell_quest]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif  
      if %actor.class% /= Sorcerer
        osend %actor% &2&b&uMeteorswarm&0
        osend %actor% Minimum Level: 73
        if %actor.has_completed[meteorswarm]%
          set status Completed!
        elseif %actor.quest_stage[meteorswarm]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Bard
        osend %actor% &2&b&uMajor Paralysis&0
        osend %actor% Minimum Level: 73
        osend %actor% Note: This chant must be granted directly by the gods and cannot be tracked in the quest journal.&_
      endif
      if %actor.class% /= diabolist || %actor.class% /= Priest
        osend %actor% &2&b&uWord of Command&0
        if %actor.has_completed[word_command]%
          set status Completed!
        elseif %actor.quest_stage[word_command]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Cryomancer
        osend %actor% &2&b&uWaterform&0
        osend %actor% Minimum Level: 73
        if %actor.has_completed[waterform]%
          set status Completed!
        elseif %actor.quest_stage[waterform]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Monk
        osend %actor% &2&b&uApocalyptic Anthem&0
        osend %actor% Minimum Level: 75
        if %actor.quest_stage[monk_chants]% > 5
          set status Completed!
        elseif %actor.quest_stage[monk_chants]% == 5
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
    endif
    if %actor.level% >= 75
      if %actor.class% /= Bard
        osend %actor% &2&b&uCrown of Madness&0
        osend %actor% Minimum Level: 80
        osend %actor% Note: This chant must be granted directly by the gods and cannot be tracked in the quest journal.&_
      endif
      if %actor.class% /= Monk
        osend %actor% &2&b&uFires of Saint Augustine&0
        osend %actor% Minimum Level: 80
        if %actor.quest_stage[monk_chants]% > 6
          set status Completed!
        elseif %actor.quest_stage[monk_chants]% == 6
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Ranger
        osend %actor% &2&b&uBlur&0
        osend %actor% Minimum Level: 81
        if %actor.has_completed[blur]%
          set status Completed!
        elseif %actor.has_failed[blur]%
          set status Failed
        elseif %actor.quest_stage[blur]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif  
      if %actor.class% /= Druid
        osend %actor% &2&b&uCreeping Doom&0
        osend %actor% Minimum Level: 81
        if %actor.has_completed[creeping_doom]%
          set status Completed!
        elseif %actor.quest_stage[creeping_doom]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Necromancer
        osend %actor% &2&b&uDegeneration&0
        osend %actor% Minimum Level: 81
        if %actor.has_completed[degeneration]%
          set status Completed!
        elseif %actor.quest_stage[degeneration]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Cryomancer
        osend %actor% &2&b&uFlood&0
        osend %actor% Minimum Level: 81
        if %actor.has_completed[flood]%
          set status Completed!
        elseif %actor.quest_stage[flood]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Priest
        osend %actor% &2&b&uHeavens Gate&0
        osend %actor% Minimum Level: 81
        if %actor.has_completed[heavens_gate]%
          set status Completed!
        elseif %actor.quest_stage[heavens_gate]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Diabolist
        osend %actor% &2&b&uHell Gate&0
        osend %actor% Minimum Level: 81
        if %actor.has_completed[hell_gate]%
          set status Completed!
        elseif %actor.quest_stage[hell_gate]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Pyromancer
        osend %actor% &2&b&uMeteorswarm&0
        osend %actor% Minimum Level: 81
        if %actor.has_completed[meteorswarm]%
          set status Completed!
        elseif %actor.quest_stage[meteorswarm]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Cleric || %actor.class% /= Priest || %actor.class% /= Diabolist
        osend %actor% &2&b&uResurrection&0
        osend %actor% Minimum Level: 81
        if %actor.has_completed[resurrection_quest]%
          set status Completed!
        elseif %actor.quest_stage[resurrection_quest]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Sorcerer
        osend %actor% &2&b&uWizard Eye&0
        osend %actor% Minimum Level: 81
        if %actor.has_completed[wizard_eye]%
          set status Completed!
        elseif %actor.quest_stage[wizard_eye]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
    endif
    if %actor.level% >= 85
      if %actor.class% /= Sorcerer || %actor.class% /= Bard || %actor.class% /= Illusionist
        osend %actor% &2&b&uCharm Person&0
        osend %actor% Minimum Level: 89
        if %actor.has_completed[charm_person]%
          set status Completed!
        elseif %actor.quest_stage[charm_person]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Cleric || %actor.class% /= Priest
        osend %actor% &2&b&uDragons Health&0
        osend %actor% Minimum Level: 89
        if %actor.has_completed[dragons_health]%
          set status Completed!
        elseif %actor.quest_stage[dragons_health]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Ranger
        osend %actor% &2&b&uGreater Displacement&0
        osend %actor% Minimum Level: 89
        osend %actor% Note: This chant must be granted directly by the gods and cannot be tracked in the quest journal.&_
      endif
      if %actor.class% /= Cryomancer
        osend %actor% &2&b&uIce Shards&0
        osend %actor% Minimum Level: 89
        if %actor.has_completed[ice_shards]%
          set status Completed!
        elseif %actor.quest_stage[ice_shards]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Pyromancer
        osend %actor% &2&b&uSupernova&0
        osend %actor% Minimum Level: 89
        if %actor.has_completed[supernova]%
          set status Completed!
        elseif %actor.quest_stage[supernova]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Bard
        osend %actor% &2&b&uEnrapture&0
        osend %actor% Minimum Level: 90
        osend %actor% Note: This chant must be granted directly by the gods and cannot be tracked in the quest journal.&_
      endif
    endif
    if %actor.level% >= 90
      if %actor.class% /= Necromancer
        osend %actor% &2&b&uShift Corpse&0
        osend %actor% Minimum Level: 97
        if %actor.has_completed[shift_corpse]%
          set status Completed!
        elseif %actor.quest_stage[shift_corpse]%
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
      if %actor.class% /= Monk
        osend %actor% &2&b&uSeed of Destruction&0
        osend %actor% Minimum Level: 99
        if %actor.quest_stage[monk_chants]% > 7
          set status Completed!
        elseif %actor.quest_stage[monk_chants]% == 7
          set status In Progress
        else
          set status Not Started
        endif
        osend %actor% &6Status: %status%&0&_
      endif
    endif
  else
    osend %actor% There are no spell, song, or chant quests available for %actor.class% characters.&_
    osend %actor% &3=======================================&0
  endif
endif
~
#405
Subclass quests~
1 m 100
journal quest quest-journal~
if %arg% /= subclass
  return 0
  set anti Anti-Paladin
  set necromancerraces dragonborn_fire dragonborn_frost dragonborn_gas dragonborn_acid dragonborn_lightning unseelie_faerie human orc sverfneblin
  set priestraces dragonborn_fire dragonborn_frost dragonborn_gas dragonborn_acid dragonborn_lightning dwarf gnome halfelf halfling human elf
  set diabolistraces dragonborn_fire dragonborn_frost dragonborn_gas dragonborn_acid dragonborn_lightning duergar human orc sverfneblin drow
  set druidraces Gnome Halfflf Human Nymph Seelie_Faerie Unseelie_Faerie elf
  set paladinraces dragonborn_fire dragonborn_frost dragonborn_gas dragonborn_acid dragonborn_lightning dwarf elf human
  set antipaladinraces dragonborn_fire dragonborn_frost dragonborn_gas dragonborn_acid dragonborn_lightning Drow Duergar Human Orc 
  set rangerraces elf halfelf human Seelie_Faerie nymph
  set assassinraces Duergar drow Human Orc Sverfneblin
  set mercenaryraces Barbarian Duergar Dwarf Human Ogre Orc Troll drow
  set thiefraces Dwarf Duergar Gnome Halfelf Halfling Human Orc Seelie_Faerie Sverfneblin Unseelie_Faerie elf
  set bardraces Dwarf Elf Gnome Halfelf Halfling Human Nymph Seelie_Faerie Sverfneblin Unseelie_Faerie
  set berserkerraces Barbarian Duergar Dwarf Ogre Orc Troll
  osend %actor% &3=========== SUBCLASS QUESTS ===========&0
  if (%actor.class% /= cleric && %actor.level% < 35) || (%actor.class% /= sorcerer && %actor.level% < 45) || ((%actor.class% /= warrior ||  %actor.class% /= rogue) && %actor.level% < 25)
    osend %actor% Subclass quests involve finding a specific quest master and performing some kind of special deed.
    osend %actor% Eligibility for subclasses depends on your base class, your race, and your alignment.  
    osend %actor% Characters must be at least level 10 to subclass.&_
    osend %actor% A character may only ever undertake a single subclass quest.
    osend %actor% Once a subclass quest has begun, it is not possible to start a different one.
    osend %actor% &3=======================================&0&_
    osend %actor% &2&bAVAILABLE QUESTS:&0
    if %actor.quest_variable[nec_dia_ant_subclass:subclass_name]% == Nec
      osend %actor% - &9&bNecromancer&0
      set status running
    elseif %actor.quest_variable[nec_dia_ant_subclass:subclass_name]% == Diabolist
      osend %actor% - &5Diabolist&0
      set status running
    elseif %actor.quest_variable[nec_dia_ant_subclass:subclass_name]% == %anti%
      osend %actor% - &1&bAnti-Paladin&0
      set status running
    elseif %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == Mercernary
      osend %actor% - &9&bMercenary&0
      set status running
    elseif %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == Assassin
      osend %actor% - &1Assassin&0
      set status running
    elseif %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == Thief
      osend %actor% - &1&bThief&0
      set status running
    elseif %actor.quest_variable[pri_pal_subclass:subclass_name]% == Priest
      osend %actor% - &6&bPriest&0
      set status running
    elseif %actor.quest_variable[pri_pal_subclass:subclass_name]% == Paladin
      osend %actor% - &7&bPaladin&0
      set status running
    elseif %actor.quest_variable[ran_dru_subclass:subclass_name]% == Ranger
      osend %actor% - &2&bRanger&0
      set status running
    elseif %actor.quest_variable[ran_dru_subclass:subclass_name]% == Druid
      osend %actor% - &2Druid&0
      set status running
    elseif %actor.quest_stage[monk_subclass]%
      osend %actor% - &3Monk&0
      set status running
    elseif %actor.quest_stage[pyromancer_subclass]%
      osend %actor% - &1&bPyromancer&0
      set status running
    elseif %actor.quest_stage[cryomancer_subclass]%
      osend %actor% - &4&bCryomancer&0
      set status running
    elseif %actor.quest_stage[illusionist_subclass]%
      osend %actor% - &5&bIllusionist&0
      set status running
    elseif %actor.quest_stage[bard_subclass]%
      osend %actor% - &5&bBard&0
      set status running
    elseif %actor.quest_stage[berserker_subclass]%
      osend %actor% - &9&bBerserker&0
      set status running
    else
      if %actor.class% /= Sorcerer
        if %actor.race% != dragonborn_fire
          osend %actor% - &4&bCryomancer&0&_
        endif
        osend %actor% - &5&bIllusionist&0&_
        if %necromancerraces% /= %actor.race%
          osend %actor% - &9&bNecromancer&0
          osend %actor% &0   &6(This class is for evil characters only)&0&_
        endif
        if %actor.race% != dragonborn_frost
          osend %actor% - &1&bPyromancer&0&_
        endif
      elseif %actor.class% /= Cleric
        if %diabolistraces% /= %actor.race%
          osend %actor% - &5Diabolist&0
          osend %actor% &0   &6(This class is for evil characters only)&0&_
        endif
        if %druidraces% /= %actor.race%
          osend %actor% - &2Druid&0
          osend %actor% &0   &6(This class is for neutral characters only)&0&_
        endif
        if %priestraces% /= %actor.race%
          osend %actor% - &6&bPriest&0
          osend %actor% &0   &6(This class is for good characters only)&0&_
        endif
      elseif %actor.class% /= Rogue
        if %assassinraces% /= %actor.race%
          osend %actor% - &1Assassin&0
          osend %actor% &0   &6(This class is for evil characters only)&0&_
        endif
        if %bardraces% /= %actor.race%
          osend %actor% - &5&bBard&0&_
        endif
        if %mercenaryraces% /= %actor.race%
          osend %actor% - &9&bMercenary&0&_
        endif
        if %thiefraces% /= %actor.race%
          osend %actor% - &1&bThief&0&_
        endif
      elseif %actor.class% /= Warrior
        if %antipaladinraces% /= %actor.race%
          osend %actor% - &1&bAnti-Paladin&0
          osend %actor% &0   &6(This class is for evil characters only)&0&_
        endif
        if %berserkerraces% /= %actor.race%
          osend %actor% - &9&bBerserker&0&_
        endif
        if %actor.race% /= halfelf || %actor.race% /= human
          osend %actor% - &3Monk&0&_
        endif
        if %paladinraces% /= %actor.race%
          osend %actor% - &7&bPaladin&0
          osend %actor% &0   &6(This class is for good characters only)&0&_
        endif
        if %rangerraces% /= %actor.race%
          osend %actor% - &2&bRanger&0
          osend %actor% &0   &6(This class is for good characters only)&0&_
        endif
      endif
    endif
    if %status% == running
      osend %actor% &6Status: In Progress&0&_
    endif
  else
    if %actor.class% /= Warrior || %actor.class% /= Cleric || %actor.class% /= Sorcerer || %actor.class% /= Rogue
      osend %actor% You have no subclass quests available.
    else
      osend %actor% You have already completed the quest to subclass to %actor.class%.
    endif
  endif
endif
~
#406
Troll Mask progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= tribal || %arg% /= trouble || %arg% /= tribal_trouble || %arg% /= troll_mask || %arg% /= troll || %arg% /= troll mask quest
  if %actor.race% /= troll
    if %actor.level% >= 45
      return 0
      set job1 %actor.quest_variable[troll_quest:got_item:37080]%
      set job2 %actor.quest_variable[troll_quest:got_item:37081]%
      set job3 %actor.quest_variable[troll_quest:got_item:37082]%
      osend %actor% &2&b&uTribal Trouble&0
      osend %actor% Minimum Level: 55
      if !%actor.quest_stage[troll_quest]%
        set status Not Started
      elseif %actor.has_completed[troll_quest]%
        set status Completed!
      elseif %actor.quest_stage[troll_quest]% == 1
        set status In Progress
      endif
      osend %actor% &6Stats: %status%&0&_
      if %actor.quest_stage[troll_quest]% == 1
        osend %actor% Quest Master: %get.mob_shortdesc[37000]%
        osend %actor% &0
        osend %actor% %get.mob_shortdesc[37000]% told you:
        osend %actor% Long ago, three powerful items of the trolls were stolen by jealous shamans from different tribes and hidden away.
        osend %actor% If you were to bring the objects back to me, I could reward you quite handsomely.&_
        osend %actor% - One is the bough of a sacred mangrove tree that stood in the courtyard of a great troll palace before it was destroyed by a feline god of the snows.
        osend %actor% - One is a vial of red dye made from the blood of our enemies, stolen by a tribe in a canyon who wished to unlock its power.
        osend %actor% - One is a large hunk of malachite, a stone we have always valued for its deep green color.  Our enemies the swamp lizards also liked its color, however, and guard it jealously.
        if %job1% || %job2% || %job3%
          osend %actor% &0
          osend %actor% You have found the following items:
          if %job1%
            osend %actor% - %get.obj_shortdesc[37080]%
          endif
          if %job2%
            osend %actor% - %get.obj_shortdesc[37081]%
          endif
          if %job3%
            osend %actor% - %get.obj_shortdesc[37082]%
          endif
        endif
        osend %actor% &0 
        osend %actor% You still need to find:
        if !%job1%
          osend %actor% - %get.obj_shortdesc[37080]%
        endif
        if !%job2%
          osend %actor% - %get.obj_shortdesc[37081]%
        endif
        if !%job3%
          osend %actor% - %get.obj_shortdesc[37082]%
        endif
      endif
    endif
  endif
endif
~
#407
Fiery Island Progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= liberate || %arg% /= fiery || %arg% /= liberate fiery island || %arg% /= fieryisle_quest || %arg% /= fieryisle || %arg% /= liberate_fiery_island || %arg% /= fiery_isle || %arg% /= fiery island quest
  if %actor.level% >= 35
    return 0
    set stage %actor.quest_stage[fieryisle_quest]%
    osend %actor% &2&b&uLiberate Fiery Island&0
    osend %actor% Recommended Level: 55
    osend %actor% Initial rewards can be received at level 35.
    if %actor.has_completed[fieryisle_quest]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[fieryisle_quest]%
      osend %actor% Quest Master: the island shaman
      osend %actor% &0
      osend %actor% You are trying to:
      switch %stage%
        case 1
          osend %actor% Enter the volcano and find the warlord's son.
          break
        case 2
          osend %actor% Find the dwarrow woman and ask for a spell of reversal.
          break 
        case 3
          osend %actor% Kill the ash lord and retrieve his crown.
          break
        case 4
          osend %actor% Return the crown of the ash lord to the dwarrow woman.
          break
        case 5
          osend %actor% Find the person turned to rock and hold the spell the dwarrow woman gave you.
          break
        case 6
          osend %actor% Retrieve the ivory key.
          break
        case 7
          osend %actor% Find Vulcera in the ivory tower.
          break
        case 8
          osend %actor% Give the ivory key to Vulcera.
          break
        case 9
          osend %actor% Defeat Vulcera!
          break
      done
      osend %actor% &0 
      if %actor.quest_variable[fieryisle_quest:chimera]%
        osend %actor% The mystic phrase to open the volcano is &7&bbuntoi nakkarri karisto&0.
      else
        osend %actor% Defeat the guardian to learn the mystic phrase to open the volcano.
      endif
    endif
  endif
endif
~
#408
Griffin quest progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= griffin || %arg% /= destroy || %arg% /= cult || %arg% /= destroy_the_cult_of_the_griffin || %arg% /= griffin_quest || %arg% /= griffin_island_quest || %arg% /= griffin_island || %arg% /= griffin island quest
  if %actor.level% >= 45
    return 0
    set stage %actor.quest_stage[griffin_quest]%
    osend %actor% &6Status: %status%&0&_
    osend %actor% &2&b&uDestroy the Cult of the Griffin&0
    osend %actor% Recommended Level: 60
    if %actor.has_completed[griffin_quest]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[griffin_quest]%
      osend %actor% Quest Master: %get.mob_shortdesc[49008]%
      osend %actor% &0
      osend %actor% %get.mob_shortdesc[49008]% has told you:
      switch %stage%
        case 1
          osend %actor% The sword used to cut the oak is also critical.  Bring the eldest druid the mystic sword from the wreck of the St. Marvin.
          break
        case 2
          osend %actor% You'll need oracular guidance.  Go to the Seer and say &7&b"Earle sends me"&0.
          break
        case 3
          osend %actor% Get assistance from the strongest person on the island.  Find Derceta and return her crystal earring to her.
          break
        case 4
          osend %actor% The entrance to the cult's lair is hidden under a massive boulder.  Find Derceta again and ask her to move the boulder.
          break
        case 5
          osend %actor% Find the cult's altar and destroy it.  Go into the cave Derceta unearthed and drop the sapling at the cult's altar.
          break
        case 6
          osend %actor% It is time to destroy the cult!  Slay Dagon!
          break
        case 7
          osend %actor% Now you can strike the final blow and destroy the essence of the god of the cult itself.  Give the griffin skin to Awura, then seek out the hidden entrance to the other realms and destroy Adramalech.
          break
      done
    elseif !%stage%
      osend %actor% Find the wreck of the St. Marvin and bring the cutting of the sacred oak to the eldest druid.
    endif
  endif
endif
~
#409
Sunfire Rescue progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= sunfire || %arg% /= serin || %arg% /= rescue || %arg% /= sunfire_rescue || %arg% /= sunfire_crest || %arg% /= serin_sunfire_rescue || %arg% /= sunfire crest
  set stage %actor.quest_stage[sunfire_rescue]%
  if %actor.level% >= 80 || %stage%
    return 0
    osend %actor% &2&b&uSunfire Rescue&0
    osend %actor% This quest is only available to good-aligned characters.
    osend %actor% Minimum Level: 85
    osend %actor% - This quest can be started at any level but requires level 85 to finish.
    if %actor.has_completed[sunfire_rescue]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% == 1
      osend %actor% Quest Master: %get.mob_shortdesc[23719]%
      osend %actor% &0
      osend %actor% You must bring the elven cloak, ring, and boots from Templace to Serin Sunfire to help him escape!
      osend %actor% &0
      set boots %actor.quest_variable[sunfire_rescue:boots]%
      set cloak %actor.quest_variable[sunfire_rescue:cloak]%
      set ring %actor.quest_variable[sunfire_rescue:ring]%
      eval total %boots% + %cloak% + %ring%
      if %total% > 0
        osend %actor% You have retrieved the following treasures:
        if %boots%
          osend %actor% %get.obj_shortdesc[52008]%.
        endif
        if %cloak%
          osend %actor% %get.obj_shortdesc[52009]%.
        endif
        if %ring%
          osend %actor% %get.obj_shortdesc[52001]%.
        endif
        osend %actor% &0
      endif
      osend %actor% You still need to find:
      if %boots% == 0
        osend %actor% %get.obj_shortdesc[52008]%.
      endif
      if %cloak% == 0
        osend %actor% %get.obj_shortdesc[52009]%.
      endif
      if %ring% == 0
        osend %actor% %get.obj_shortdesc[52001]%.
      endif
    endif
  endif
endif
~
#410
Vilekka Stew progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= service || %arg% /= lolth || %arg% /= vilekka_stew || %arg% /= vilekka || %arg% /= stew || %arg% /= drow boots || %arg% /= in_service_of_lolth || %arg% /= drow_boots || %arg% /= drow
  if %actor.level% >= 25
    return 0
    set stage %actor.quest_stage[vilekka_stew]%
    osend %actor% &2&b&uIn Service of Lolth&0
    osend %actor% This quest is only available to neutral and evil-aligned characters.
    osend %actor% Recommended Level: 90
    osend %actor% - This quest starts at level 25 and continues through level 90.
    if %actor.has_completed[vilekka_stew]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[vilekka_stew]%
      osend %actor% Quest Master: %get.mob_shortdesc[23728]%
      osend %actor% &0
      switch %stage%
        case 1
          osend %actor% Bring Vilekka the heart of the drow living in the surface city!
          break
        case 2
          osend %actor% Tell Vilekka if you wish to stop or continue.
          break
        case 3
          osend %actor% You must bring Vilekka the head of the drider king.
          break
        case 4
          osend %actor% Tell Vilekka if you wish to stop or continue.
          break
        case 5
          set num_spices %actor.quest_variable[vilekka_stew:num_spices]%
          set spice1 %actor.quest_variable[vilekka_stew:got_spice:12552]%
          set spice2 %actor.quest_variable[vilekka_stew:got_spice:49022]%
          set spice3 %actor.quest_variable[vilekka_stew:got_spice:23750]%
          set spice4 %actor.quest_variable[vilekka_stew:got_spice:23751]%
          set spice5 %actor.quest_variable[vilekka_stew:got_spice:23752]%
          set spice6 %actor.quest_variable[vilekka_stew:got_spice:23753]%
          set spice7 %actor.quest_variable[vilekka_stew:got_spice:23754]%
          set spice8 %actor.quest_variable[vilekka_stew:got_spice:23755]%
          set spice9 %actor.quest_variable[vilekka_stew:got_spice:23756]%
          set spice10 %actor.quest_variable[vilekka_stew:got_spice:23757]%
          set spice11 %actor.quest_variable[vilekka_stew:got_spice:23758]%
          set spice12 %actor.quest_variable[vilekka_stew:got_spice:23759]%
          set spice13 %actor.quest_variable[vilekka_stew:got_spice:23760]%
          osend %actor% Bring Vilekka 10 herbs and spices so that she can make a stew out of the head and heart.
          if %num_spices% > 0
            osend %actor% &0 
            osend %actor% So far you have brought:
            if %spice1%
              osend %actor% - %get.obj_shortdesc[12552]%
            endif
            if %spice2%
              osend %actor% - %get.obj_shortdesc[49022]%
            endif
            if %spice3%
              osend %actor% - %get.obj_shortdesc[23750]%
            endif
            if %spice4%
              osend %actor% - %get.obj_shortdesc[23751]%
            endif
            if %spice5%
              osend %actor% - %get.obj_shortdesc[23752]%
            endif
            if %spice6%
              osend %actor% - %get.obj_shortdesc[23753]%
            endif
            if %spice7%
              osend %actor% - %get.obj_shortdesc[23754]%
            endif
            if %spice8%
              osend %actor% - %get.obj_shortdesc[23755]%
            endif
            if %spice9%
              osend %actor% - %get.obj_shortdesc[23756]%
            endif
            if %spice10%
              osend %actor% - %get.obj_shortdesc[23757]%
            endif
            if %spice11%
              osend %actor% - %get.obj_shortdesc[23758]%
            endif
            if %spice12%
              osend %actor% - %get.obj_shortdesc[23759]%
            endif
            if %spice13%
              osend %actor% - %get.obj_shortdesc[23760]%
            endif
          endif    
          osend %actor% &0 
          eval total 10 - %num_spices%
          osend %actor% Bring %total% more spices to prepare the stew.
      done
    endif
  endif
endif
~
#411
Guild Armor Phase One progress journal~
1 m 100
journal quest quest-journal~
if ((%arg% /= guild || %arg% /= phase || %arg% /= armor) && (%arg% /= one || %arg% /= 1)) || %arg% /= guild_armor_phase_one || %arg% /= guild_armor_phase_1 || %arg% /= phase_armor_one || %arg% /= phase_armor_1
  return 0
  osend %actor% &2&b&uGuild Armor Phase One&0
  osend %actor% Mininum Level: 1
  osend %actor% Items for this quest drop from mobs between level 1 and 20.
  if %actor.quest_stage[phase_armor]%
    set sorcererclasses Sorcerer Cryomancer Pyromancer Illusionist Necromancer
    set clericclasses Cleric Priest Druid Diabolist
    set warriorclasses Warrior Anti-Paladin Ranger Paladin Monk Berserker
    set rogueclasses Rogue Mercenary Assassin Thief Bard
    if %sorcererclasses% /= %actor.class%
      set feet_gem 55571
      set head_gem 55579
      set hands_gem 55567
      set arms_gem 55583
      set legs_gem 55587
      set body_gem 55591
      set wrist_gem 55575
      set feet_armor 55306
      set head_armor 55314
      set hands_armor 55302
      set arms_armor 55318
      set legs_armor 55322
      set body_armor 55326
      set wrist_armor 55310
      set feet_reward 55402
      set head_reward 55398
      set hands_reward 55404
      set arms_reward 55399
      set legs_reward 55401
      set body_reward 55400
      set wrist_reward 55403
      set master the Archmage of Mielikki and Gagar
    elseif %clericclasses% /= %actor.class%
      set feet_gem 55570
      set head_gem 55578
      set hands_gem 55566
      set arms_gem 55582
      set legs_gem 55586
      set body_gem 55590
      set wrist_gem 55574
      set feet_armor 55304
      set head_armor 55312
      set hands_armor 55300
      set arms_armor 55316
      set legs_armor 55320
      set body_armor 55324
      set wrist_armor 55308
      set feet_reward 55395
      set head_reward 55391
      set hands_reward 55397
      set arms_reward 55392
      set legs_reward 55394
      set body_reward 55393
      set wrist_reward 55396
      set master the High Priestess of Mielikki and Rorgdush
    elseif %warriorclasses% /= %actor.class%
      set feet_gem 55573
      set head_gem 55581
      set hands_gem 55569
      set arms_gem 55585
      set legs_gem 55589
      set body_gem 55593
      set wrist_gem 55577
      set feet_armor 55304
      set head_armor 55312
      set hands_armor 55300
      set arms_armor 55316
      set legs_armor 55320
      set body_armor 55324
      set wrist_armor 55308
      set feet_reward 55388
      set head_reward 55384
      set hands_reward 55390
      set arms_reward 55385
      set legs_reward 55387
      set body_reward 55386
      set wrist_reward 55389
      set master the Warrior Coach of Mielikki and Grort
    elseif %rogueclasses% /= %actor.class%
      set feet_gem 55572
      set head_gem 55580
      set hands_gem 55568
      set arms_gem 55584
      set legs_gem 55588
      set body_gem 55592
      set wrist_gem 55576
      set feet_armor 55305
      set head_armor 55313
      set hands_armor 55301
      set arms_armor 55317
      set legs_armor 55321
      set body_armor 55325
      set wrist_armor 55309
      set feet_reward 55409
      set head_reward 55405
      set hands_reward 55411
      set arms_reward 55406
      set legs_reward 55408
      set body_reward 55407
      set wrist_reward 55410
      set master the Master Rogue of Mielikki and Tinilas
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
    if %done_hands% && %done_feet% && %done_wrist% && %done_head% && %done_arms% && %done_legs% && %done_body%
      osend %actor% &6Status: Completed!&0
      halt
    endif
    osend %actor% &6Status: In Progress&0
    osend %actor% Quest Master is: %master%
    osend %actor% You need to retrieve:
    osend %actor% &0  %get.obj_shortdesc[%hands_armor%]% and 3 of %get.obj_shortdesc[%hands_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%feet_armor%]% and 3 of %get.obj_shortdesc[%feet_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%wrist_armor%]% and 3 of %get.obj_shortdesc[%wrist_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%head_armor%]% and 3 of %get.obj_shortdesc[%head_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%arms_armor%]% and 3 of %get.obj_shortdesc[%arms_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%legs_armor%]% and 3 of %get.obj_shortdesc[%legs_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%body_armor%]% and 3 of %get.obj_shortdesc[%body_gem%]%
    if !%given%
      osend %actor% You haven't retrieved anything yet.
      halt
    elseif %unrewarded%
      osend %actor% You have found:
    endif
    if %got_hands% && !%done_hands%
      osend %actor% &0  %get.obj_shortdesc[%hands_armor%]%
    endif
    if %hands_count% && !%done_hands%
      osend %actor% &0  %hands_count% of %get.obj_shortdesc[%hands_gem%]%
    endif
    if %got_feet% && !%done_feet%
      osend %actor% &0  %get.obj_shortdesc[%feet_armor%]%
    endif
    if %feet_count% && !%done_feet%
      osend %actor% &0  %feet_count% of %get.obj_shortdesc[%feet_gem%]%
    endif
    if %got_wrist% && !%done_wrist%
      osend %actor% &0  %get.obj_shortdesc[%wrist_armor%]%
    endif
    if %wrist_count% && !%done_wrist%
      osend %actor% &0  %wrist_count% of %get.obj_shortdesc[%wrist_gem%]%
    endif
    if %got_head% && !%done_head%
      osend %actor% &0  %get.obj_shortdesc[%head_armor%]%
    endif
    if %head_count% && !%done_head%
      osend %actor% &0  %head_count% of %get.obj_shortdesc[%head_gem%]%
    endif
    if %got_arms% && !%done_arms%
      osend %actor% &0  %get.obj_shortdesc[%arms_armor%]%
    endif
    if %arms_count% && !%done_arms%
      osend %actor% &0  %arms_count% of %get.obj_shortdesc[%arms_gem%]%
    endif
    if %got_legs% && !%done_legs%
      osend %actor% &0  %get.obj_shortdesc[%legs_armor%]%
    endif
    if %legs_count% && !%done_legs%
      osend %actor% &0  %legs_count% of %get.obj_shortdesc[%legs_gem%]%
    endif
    if %got_body% && !%done_body%
      osend %actor% &0  %get.obj_shortdesc[%body_armor%]%
    endif
    if %body_count% && !%done_body%
      osend %actor% &0  %body_count% of %get.obj_shortdesc[%body_gem%]%
    endif
    if %done_hands% || %done_feet% || %done_wrist% || %done_head% || %done_arms% || %done_legs% || %done_body%
      osend %actor% &0
      osend %actor% You have completed quests for:
    endif
    if %done_hands%
      osend %actor% &0  %get.obj_shortdesc[%hands_reward%]%
    endif
    if %done_feet%
      osend %actor% &0  %get.obj_shortdesc[%feet_reward%]%
    endif
    if %done_wrist%
      osend %actor% &0  %get.obj_shortdesc[%wrist_reward%]%
    endif
    if %done_head%
      osend %actor% &0  %get.obj_shortdesc[%head_reward%]%
    endif
    if %done_arms%
      osend %actor% &0  %get.obj_shortdesc[%arms_reward%]%
    endif
    if %done_legs%
      osend %actor% &0  %get.obj_shortdesc[%legs_reward%]%
    endif
    if %done_body%
      osend %actor% &0  %get.obj_shortdesc[%body_reward%]%
    endif
  else
    osend %actor% &6Status: Not Started&0
  endif
endif
~
#412
Guild Armor Phase Two progress journal~
1 m 100
journal quest quest-journal~
if ((%arg% /= guild || %arg% /= phase || %arg% /= armor) && (%arg% /= two || %arg% /= 2)) || %arg% /= guild_armor_phase_two || %arg% /= guild_armor_phase_2 || %arg% /= phase_armor_two || %arg% /= phase_armor_2
  return 0
  osend %actor% &2&b&uGuild Armor Phase Two&0
  osend %actor% Mininum Level: 21
  osend %actor% Items for this quest drop from mobs between level 21 and 40.
  if %actor.quest_stage[phase_armor]% > 1
    set sorcererclasses Sorcerer Cryomancer Pyromancer Illusionist
    set clericclasses Cleric Priest
    set rogueclasses Rogue Mercenary Assassin Thief
    set anti Anti-Paladin
    if %sorcererclasses% /= %actor.class%
      set feet_gem 55613
      set head_gem 55635
      set hands_gem 55602
      set arms_gem 55646
      set legs_gem 55657
      set body_gem 55668
      set wrist_gem 55624
      set feet_armor 55335
      set head_armor 55343
      set hands_armor 55331
      set arms_armor 55347
      set legs_armor 55351
      set body_armor 55355
      set wrist_armor 55339
      set feet_reward 55479
      set head_reward 55475
      set hands_reward 55481
      set arms_reward 55476
      set legs_reward 55478
      set body_reward 55477
      set wrist_reward 55480
      if %actor.class% /= Sorcerer
        set master the Archmage of Ickle
      elseif %actor.class% /= Cryomancer
        set master the Archmage of Ickle, the High Cryomancer, and the master cryomancer
      elseif %actor.class% /= Pyromancer
        set master the Archmage of Ickle, the High Pyromancer, and the master pyromancer
      elseif %actor.class% /= Illusionist
        set master the Archmage of Ickle, Erasmus, and Esh
      endif
    elseif %actor.class% /= Necromancer
      set feet_gem 55609
      set head_gem 55631
      set hands_gem 55598
      set arms_gem 55642
      set legs_gem 55653
      set body_gem 55664
      set wrist_gem 55620
      set feet_armor 55335
      set head_armor 55343
      set hands_armor 55331
      set arms_armor 55347
      set legs_armor 55351
      set body_armor 55355
      set wrist_armor 55339
      set feet_reward 55458
      set head_reward 55454
      set hands_reward 55460
      set arms_reward 55455
      set legs_reward 55457
      set body_reward 55456
      set wrist_reward 55459
      set master Asiri'Qaxt and Schkerra
    elseif %clericclasses% /= %actor.class%
      set feet_gem 55612
      set head_gem 55634
      set hands_gem 55601
      set arms_gem 55645
      set legs_gem 55656
      set body_gem 55667
      set wrist_gem 55623
      set feet_armor 55332
      set head_armor 55340
      set hands_armor 55328
      set arms_armor 55344
      set legs_armor 55348
      set body_armor 55352
      set wrist_armor 55336
      set feet_reward 55437
      set head_reward 55433
      set hands_reward 55439
      set arms_reward 55434
      set legs_reward 55436
      set body_reward 55435
      set wrist_reward 55438
      set master the High Priestess of Anduin and Zeno the Priest
    elseif %actor.class% /= Diabolist
      set feet_gem 55606
      set head_gem 55628
      set hands_gem 55595
      set arms_gem 55639
      set legs_gem 55650
      set body_gem 55661
      set wrist_gem 55617
      set feet_armor 55332
      set head_armor 55340
      set hands_armor 55328
      set arms_armor 55344
      set legs_armor 55348
      set body_armor 55352
      set wrist_armor 55336
      set feet_reward 55472
      set head_reward 55468
      set hands_reward 55474
      set arms_reward 55469
      set legs_reward 55471
      set body_reward 55470
      set wrist_reward 55473
      set master the Black Priestess
    elseif %actor.class% /= Druid
      set feet_gem 55607
      set head_gem 55629
      set hands_gem 55596
      set arms_gem 55640
      set legs_gem 55651
      set body_gem 55662
      set wrist_gem 55618
      set feet_armor 55334
      set head_armor 55342
      set hands_armor 55330
      set arms_armor 55346
      set legs_armor 55350
      set body_armor 55354
      set wrist_armor 55338
      set feet_reward 55451
      set head_reward 55447
      set hands_reward 55453
      set arms_reward 55448
      set legs_reward 55450
      set body_reward 55449
      set wrist_reward 55452
      set master the Black Priestess and the diabolist
    elseif %actor.class% /= Warrior
      set feet_gem 55615
      set head_gem 55637
      set hands_gem 55604
      set arms_gem 55648
      set legs_gem 55659
      set body_gem 55670
      set wrist_gem 55626
      set feet_armor 55332
      set head_armor 55340
      set hands_armor 55328
      set arms_armor 55344
      set legs_armor 55348
      set body_armor 55352
      set wrist_armor 55336
      set feet_reward 55416
      set head_reward 55412
      set hands_reward 55418
      set arms_reward 55413
      set legs_reward 55415
      set body_reward 55414
      set wrist_reward 55417
      set master the high druid and a serene druidess
    elseif %actor.class% == %anti%
      set feet_gem 55605
      set head_gem 55627
      set hands_gem 55594
      set arms_gem 55638
      set legs_gem 55649
      set body_gem 55660
      set wrist_gem 55616
      set feet_armor 55332
      set head_armor 55340
      set hands_armor 55328
      set arms_armor 55344
      set legs_armor 55348
      set body_armor 55352
      set wrist_armor 55336
      set feet_reward 55430
      set head_reward 55426
      set hands_reward 55432
      set arms_reward 55427
      set legs_reward 55429
      set body_reward 55428
      set wrist_reward 55431
      set master the warrior coach of Anduin
    elseif %actor.class% /= Ranger
      set feet_gem 55611
      set head_gem 55633
      set hands_gem 55600
      set arms_gem 55644
      set legs_gem 55655
      set body_gem 55666
      set wrist_gem 55622
      set feet_armor 55333
      set head_armor 55341
      set hands_armor 55329
      set arms_armor 55345
      set legs_armor 55349
      set body_armor 55353
      set wrist_armor 55337
      set feet_reward 55444
      set head_reward 55440
      set hands_reward 55446
      set arms_reward 55441
      set legs_reward 55443
      set body_reward 55442
      set wrist_reward 55445
      set master Galithel Silverwing
    elseif %actor.class% == Paladin
      set feet_gem 55610
      set head_gem 55632
      set hands_gem 55599
      set arms_gem 55643
      set legs_gem 55654
      set body_gem 55665
      set wrist_gem 55621
      set feet_armor 55332
      set head_armor 55340
      set hands_armor 55328
      set arms_armor 55344
      set legs_armor 55348
      set body_armor 55352
      set wrist_armor 55336
      set feet_reward 55423
      set head_reward 55419
      set hands_reward 55425
      set arms_reward 55420
      set legs_reward 55422
      set body_reward 55421
      set wrist_reward 55424
      set master the Grey Knight
    elseif %actor.class% /= Monk
      set feet_gem 55608
      set head_gem 55630
      set hands_gem 55597
      set arms_gem 55641
      set legs_gem 55652
      set body_gem 55663
      set wrist_gem 55619
      set feet_armor 55334
      set head_armor 55342
      set hands_armor 55330
      set arms_armor 55346
      set legs_armor 55350
      set body_armor 55354
      set wrist_armor 55338
      set feet_reward 55465
      set head_reward 55461
      set hands_reward 55467
      set arms_reward 55462
      set legs_reward 55464
      set body_reward 55463
      set wrist_reward 55466
      set master the Head Monk
    elseif %actor.class% /= Berserker
      set feet_gem 55607
      set head_gem 55631
      set hands_gem 55595
      set arms_gem 55638
      set legs_gem 55655
      set body_gem 55665
      set wrist_gem 55619
      set feet_armor 55334
      set head_armor 55342
      set hands_armor 55330
      set arms_armor 55346
      set legs_armor 55350
      set body_armor 55354
      set wrist_armor 55338
      set feet_reward 55771
      set head_reward 55767
      set hands_reward 55773
      set arms_reward 55768
      set legs_reward 55770
      set body_reward 55769
      set wrist_reward 55772
      set master Tozug, Khargol, Jora Granitearm
    elseif %rogueclasses% /= %actor.class%
      set feet_gem 55614
      set head_gem 55636
      set hands_gem 55603
      set arms_gem 55647
      set legs_gem 55658
      set body_gem 55669
      set wrist_gem 55625
      set feet_armor 55333
      set head_armor 55341
      set hands_armor 55329
      set arms_armor 55345
      set legs_armor 55349
      set body_armor 55353
      set wrist_armor 55337
      set feet_reward 55486
      set head_reward 55482
      set hands_reward 55488
      set arms_reward 55483
      set legs_reward 55485
      set body_reward 55484
      set wrist_reward 55487
      set master Princess Signess, Haren, and Blackhaven
    elseif %actor.class% /= Bard
      set feet_gem 55606
      set head_gem 55630
      set hands_gem 55600
      set arms_gem 55640
      set legs_gem 55649
      set body_gem 55664
      set wrist_gem 55621
      set feet_armor 55334
      set head_armor 55342
      set hands_armor 55330
      set arms_armor 55346
      set legs_armor 55350
      set body_armor 55354
      set wrist_armor 55338
      set feet_reward 55785
      set head_reward 55781
      set hands_reward 55787
      set arms_reward 55782
      set legs_reward 55784
      set body_reward 55783
      set wrist_reward 55786
      set master the Master Herald and the Bard Union Master
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
    if %done_hands% && %done_feet% && %done_wrist% && %done_head% && %done_arms% && %done_legs% && %done_body%
      osend %actor% &6Status: Completed!&0
      halt
    endif
    osend %actor% &6Status: In Progress&0
    osend %actor% Quest Master is: %master%
    osend %actor% You need to retrieve:
    osend %actor% &0  %get.obj_shortdesc[%hands_armor%]% and 3 of %get.obj_shortdesc[%hands_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%feet_armor%]% and 3 of %get.obj_shortdesc[%feet_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%wrist_armor%]% and 3 of %get.obj_shortdesc[%wrist_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%head_armor%]% and 3 of %get.obj_shortdesc[%head_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%arms_armor%]% and 3 of %get.obj_shortdesc[%arms_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%legs_armor%]% and 3 of %get.obj_shortdesc[%legs_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%body_armor%]% and 3 of %get.obj_shortdesc[%body_gem%]%
    if !%given%
      osend %actor% You haven't retrieved anything yet.
      halt
    elseif %unrewarded%
      osend %actor% You have found:
    endif
    if %got_hands% && !%done_hands%
      osend %actor% &0  %get.obj_shortdesc[%hands_armor%]%
    endif
    if %hands_count% && !%done_hands%
      osend %actor% &0  %hands_count% of %get.obj_shortdesc[%hands_gem%]%
    endif
    if %got_feet% && !%done_feet%
      osend %actor% &0  %get.obj_shortdesc[%feet_armor%]%
    endif
    if %feet_count% && !%done_feet%
      osend %actor% &0  %feet_count% of %get.obj_shortdesc[%feet_gem%]%
    endif
    if %got_wrist% && !%done_wrist%
      osend %actor% &0  %get.obj_shortdesc[%wrist_armor%]%
    endif
    if %wrist_count% && !%done_wrist%
      osend %actor% &0  %wrist_count% of %get.obj_shortdesc[%wrist_gem%]%
    endif
    if %got_head% && !%done_head%
      osend %actor% &0  %get.obj_shortdesc[%head_armor%]%
    endif
    if %head_count% && !%done_head%
      osend %actor% &0  %head_count% of %get.obj_shortdesc[%head_gem%]%
    endif
    if %got_arms% && !%done_arms%
      osend %actor% &0  %get.obj_shortdesc[%arms_armor%]%
    endif
    if %arms_count% && !%done_arms%
      osend %actor% &0  %arms_count% of %get.obj_shortdesc[%arms_gem%]%
    endif
    if %got_legs% && !%done_legs%
      osend %actor% &0  %get.obj_shortdesc[%legs_armor%]%
    endif
    if %legs_count% && !%done_legs%
      osend %actor% &0  %legs_count% of %get.obj_shortdesc[%legs_gem%]%
    endif
    if %got_body% && !%done_body%
      osend %actor% &0  %get.obj_shortdesc[%body_armor%]%
    endif
    if %body_count% && !%done_body%
      osend %actor% &0  %body_count% of %get.obj_shortdesc[%body_gem%]%
    endif
    if %done_hands% || %done_feet% || %done_wrist% || %done_head% || %done_arms% || %done_legs% || %done_body%
      osend %actor% &0
      osend %actor% You have completed quests for:
    endif
    if %done_hands%
      osend %actor% &0  %get.obj_shortdesc[%hands_reward%]%
    endif
    if %done_feet%
      osend %actor% &0  %get.obj_shortdesc[%feet_reward%]%
    endif
    if %done_wrist%
      osend %actor% &0  %get.obj_shortdesc[%wrist_reward%]%
    endif
    if %done_head%
      osend %actor% &0  %get.obj_shortdesc[%head_reward%]%
    endif
    if %done_arms%
      osend %actor% &0  %get.obj_shortdesc[%arms_reward%]%
    endif
    if %done_legs%
      osend %actor% &0  %get.obj_shortdesc[%legs_reward%]%
    endif
    if %done_body%
      osend %actor% &0  %get.obj_shortdesc[%body_reward%]%
    endif
  else
    osend %actor% &6Status: Not Started&0
  endif
endif
~
#413
Guild Armor Phase Three progress journal~
1 m 100
journal quest quest-journal~
if ((%arg% /= guild || %arg% /= phase || %arg% /= armor) && (%arg% /= three || %arg% /= 3)) || %arg% /= guild_armor_phase_three || %arg% /= guild_armor_phase_3 || %arg% /= phase_armor_three || %arg% /= phase_armor_3
  return 0
  osend %actor% &2&b&uGuild Armor Phase Three&0
  osend %actor% Mininum Level: 41
  osend %actor% Items for this quest drop from mobs above level 41.
  if %actor.quest_stage[phase_armor]% > 2
    set sorcererclasses Sorcerer Cryomancer Pyromancer Illusionist
    set clericclasses Cleric Priest
    set rogueclasses Rogue Mercenary Assassin Thief
    set anti Anti-Paladin
    if %sorcererclasses% /= %actor.class%
      set feet_gem 55690
      set head_gem 55712
      set hands_gem 55679
      set arms_gem 55723
      set legs_gem 55734
      set body_gem 55745
      set wrist_gem 55701
      set feet_armor 55363
      set head_armor 55371
      set hands_armor 55359
      set arms_armor 55375
      set legs_armor 55379
      set body_armor 55383
      set wrist_armor 55367
      set feet_reward 55556
      set head_reward 55552
      set hands_reward 55558
      set arms_reward 55553
      set legs_reward 55555
      set body_reward 55554
      set wrist_reward 55557
      if %actor.class% /= Sorcerer
        set master the Archmage of Anduin
      elseif %actor.class% /= Cryomancer
        set master a cryomancer in Anduin
      elseif %actor.class% /= Pyromancer
        set master a pyromancer in Anduin
      elseif %actor.class% /= Illusionist
        set master Aylana in Anduin
      endif
    elseif %actor.class% /= Necromancer
      set feet_gem 55686
      set head_gem 55708
      set hands_gem 55675
      set arms_gem 55719
      set legs_gem 55730
      set body_gem 55741
      set wrist_gem 55697
      set feet_armor 55363
      set head_armor 55371
      set hands_armor 55359
      set arms_armor 55375
      set legs_armor 55379
      set body_armor 55383
      set wrist_armor 55367
      set feet_reward 55535
      set head_reward 55531
      set hands_reward 55537
      set arms_reward 55532
      set legs_reward 55534
      set body_reward 55533
      set wrist_reward 55536
      set master a necromancer in Anduin
    elseif %clericclasses% /= %actor.class%
      set feet_gem 55689
      set head_gem 55711
      set hands_gem 55678
      set arms_gem 55722
      set legs_gem 55733
      set body_gem 55744
      set wrist_gem 55700
      set feet_armor 55360
      set head_armor 55368
      set hands_armor 55356
      set arms_armor 55372
      set legs_armor 55376
      set body_armor 55380
      set wrist_armor 55364
      set feet_reward 55514
      set head_reward 55510
      set hands_reward 55516
      set arms_reward 55511
      set legs_reward 55513
      set body_reward 55512
      set wrist_reward 55515
      set master the High Priest Zalish
    elseif %actor.class% /= Diabolist
      set feet_gem 55683
      set head_gem 55705
      set hands_gem 55672
      set arms_gem 55716
      set legs_gem 55727
      set body_gem 55738
      set wrist_gem 55694
      set feet_armor 55360
      set head_armor 55368
      set hands_armor 55356
      set arms_armor 55372
      set legs_armor 55376
      set body_armor 55380
      set wrist_armor 55364
      set feet_reward 55549
      set head_reward 55545
      set hands_reward 55551
      set arms_reward 55546
      set legs_reward 55548
      set body_reward 55547
      set wrist_reward 55550
      set master Ruin Wormheart
    elseif %actor.class% /= Druid
      set feet_gem 55684
      set head_gem 55706
      set hands_gem 55673
      set arms_gem 55717
      set legs_gem 55728
      set body_gem 55739
      set wrist_gem 55695
      set feet_armor 55362
      set head_armor 55370
      set hands_armor 55358
      set arms_armor 55374
      set legs_armor 55378
      set body_armor 55382
      set wrist_armor 55366
      set feet_reward 55528
      set head_reward 55524
      set hands_reward 55530
      set arms_reward 55525
      set legs_reward 55527
      set body_reward 55526
      set wrist_reward 55529
      set master the Heirophant
    elseif %actor.class% /= Warrior
      set feet_gem 55692
      set head_gem 55714
      set hands_gem 55681
      set arms_gem 55725
      set legs_gem 55736
      set body_gem 55747
      set wrist_gem 55703
      set feet_armor 55360
      set head_armor 55368
      set hands_armor 55356
      set arms_armor 55372
      set legs_armor 55376
      set body_armor 55380
      set wrist_armor 55364
      set feet_reward 55493
      set head_reward 55489
      set hands_reward 55495
      set arms_reward 55490
      set legs_reward 55492
      set body_reward 55491
      set wrist_reward 55494
      set master the Warrior Coach in Ickle
    elseif %actor.class% == %anti%
      set feet_gem 55682
      set head_gem 55704
      set hands_gem 55671
      set arms_gem 55715
      set legs_gem 55726
      set body_gem 55737
      set wrist_gem 55693
      set feet_armor 55360
      set head_armor 55368
      set hands_armor 55356
      set arms_armor 55372
      set legs_armor 55376
      set body_armor 55380
      set wrist_armor 55364
      set feet_reward 55507
      set head_reward 55503
      set hands_reward 55509
      set arms_reward 55504
      set legs_reward 55506
      set body_reward 55505
      set wrist_reward 55508
      set master the Avatar of Zzur
    elseif %actor.class% /= Ranger
      set feet_gem 55688
      set head_gem 55710
      set hands_gem 55677
      set arms_gem 55721
      set legs_gem 55732
      set body_gem 55743
      set wrist_gem 55699
      set feet_armor 55361
      set head_armor 55369
      set hands_armor 55357
      set arms_armor 55373
      set legs_armor 55377
      set body_armor 55381
      set wrist_armor 55365
      set feet_reward 55521
      set head_reward 55517
      set hands_reward 55523
      set arms_reward 55518
      set legs_reward 55520
      set body_reward 55519
      set wrist_reward 55522
      set master the Avatar of Haddixx
    elseif %actor.class% == Paladin
      set feet_gem 55687
      set head_gem 55709
      set hands_gem 55676
      set arms_gem 55720
      set legs_gem 55731
      set body_gem 55742
      set wrist_gem 55698
      set feet_armor 55360
      set head_armor 55368
      set hands_armor 55356
      set arms_armor 55372
      set legs_armor 55376
      set body_armor 55380
      set wrist_armor 55364
      set feet_reward 55500
      set head_reward 55496
      set hands_reward 55502
      set arms_reward 55497
      set legs_reward 55499
      set body_reward 55498
      set wrist_reward 55501
      set master Belward
    elseif %actor.class% /= Monk
      set feet_gem 55685
      set head_gem 55707
      set hands_gem 55674
      set arms_gem 55718
      set legs_gem 55729
      set body_gem 55740
      set wrist_gem 55696
      set feet_armor 55362
      set head_armor 55370
      set hands_armor 55358
      set arms_armor 55374
      set legs_armor 55378
      set body_armor 55382
      set wrist_armor 55366
      set feet_reward 55542
      set head_reward 55538
      set hands_reward 55544
      set arms_reward 55539
      set legs_reward 55541
      set body_reward 55540
      set wrist_reward 55543
      set master the Almoner
    elseif %actor.class% /= Berserker
      set feet_gem 55686
      set head_gem 55710
      set hands_gem 55672
      set arms_gem 55717
      set legs_gem 55726
      set body_gem 55742
      set wrist_gem 55696
      set feet_armor 55362
      set head_armor 55370
      set hands_armor 55358
      set arms_armor 55374
      set legs_armor 55378
      set body_armor 55382
      set wrist_armor 55366
      set feet_reward 55778
      set head_reward 55774
      set hands_reward 55780
      set arms_reward 55775
      set legs_reward 55777
      set body_reward 55776
      set wrist_reward 55779
      set master Avaldr Mountainhelm
    elseif %rogueclasses% /= %actor.class%
      set feet_gem 55691
      set head_gem 55713
      set hands_gem 55680
      set arms_gem 55724
      set legs_gem 55735
      set body_gem 55746
      set wrist_gem 55702
      set feet_armor 55361
      set head_armor 55369
      set hands_armor 55357
      set arms_armor 55373
      set legs_armor 55377
      set body_armor 55381
      set wrist_armor 55365
      set feet_reward 55563
      set head_reward 55559
      set hands_reward 55565
      set arms_reward 55560
      set legs_reward 55562
      set body_reward 55561
      set wrist_reward 55564
      set master an Elite Mercenary in Ickle
    elseif %actor.class% /= Bard
      set feet_gem 55685
      set head_gem 55708
      set hands_gem 55671
      set arms_gem 55720
      set legs_gem 55728
      set body_gem 55743
      set wrist_gem 55694
      set feet_armor 55362
      set head_armor 55370
      set hands_armor 55358
      set arms_armor 55374
      set legs_armor 55378
      set body_armor 55382
      set wrist_armor 55366
      set feet_reward 55792
      set head_reward 55788
      set hands_reward 55794
      set arms_reward 55789
      set legs_reward 55791
      set body_reward 55790
      set wrist_reward 55793
      set master Grand Diva Belissica
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
    if %done_hands% && %done_feet% && %done_wrist% && %done_head% && %done_arms% && %done_legs% && %done_body%
      osend %actor% &6Status: Completed&0
      halt
    endif
    osend %actor% &6Status: In Progress&0
    osend %actor% Quest Master is: %master%
    osend %actor% You need to retrieve:
    osend %actor% &0  %get.obj_shortdesc[%hands_armor%]% and 3 of %get.obj_shortdesc[%hands_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%feet_armor%]% and 3 of %get.obj_shortdesc[%feet_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%wrist_armor%]% and 3 of %get.obj_shortdesc[%wrist_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%head_armor%]% and 3 of %get.obj_shortdesc[%head_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%arms_armor%]% and 3 of %get.obj_shortdesc[%arms_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%legs_armor%]% and 3 of %get.obj_shortdesc[%legs_gem%]%
    osend %actor% &0  %get.obj_shortdesc[%body_armor%]% and 3 of %get.obj_shortdesc[%body_gem%]%
    if !%given%
      osend %actor% You haven't retrieved anything yet.
      halt
    elseif %unrewarded%
      osend %actor% You have found:
    endif
    if %got_hands% && !%done_hands%
      osend %actor% &0  %get.obj_shortdesc[%hands_armor%]%
    endif
    if %hands_count% && !%done_hands%
      osend %actor% &0  %hands_count% of %get.obj_shortdesc[%hands_gem%]%
    endif
    if %got_feet% && !%done_feet%
      osend %actor% &0  %get.obj_shortdesc[%feet_armor%]%
    endif
    if %feet_count% && !%done_feet%
      osend %actor% &0  %feet_count% of %get.obj_shortdesc[%feet_gem%]%
    endif
    if %got_wrist% && !%done_wrist%
      osend %actor% &0  %get.obj_shortdesc[%wrist_armor%]%
    endif
    if %wrist_count% && !%done_wrist%
      osend %actor% &0  %wrist_count% of %get.obj_shortdesc[%wrist_gem%]%
    endif
    if %got_head% && !%done_head%
      osend %actor% &0  %get.obj_shortdesc[%head_armor%]%
    endif
    if %head_count% && !%done_head%
      osend %actor% &0  %head_count% of %get.obj_shortdesc[%head_gem%]%
    endif
    if %got_arms% && !%done_arms%
      osend %actor% &0  %get.obj_shortdesc[%arms_armor%]%
    endif
    if %arms_count% && !%done_arms%
      osend %actor% &0  %arms_count% of %get.obj_shortdesc[%arms_gem%]%
    endif
    if %got_legs% && !%done_legs%
      osend %actor% &0  %get.obj_shortdesc[%legs_armor%]%
    endif
    if %legs_count% && !%done_legs%
      osend %actor% &0  %legs_count% of %get.obj_shortdesc[%legs_gem%]%
    endif
    if %got_body% && !%done_body%
      osend %actor% &0  %get.obj_shortdesc[%body_armor%]%
    endif
    if %body_count% && !%done_body%
      osend %actor% &0  %body_count% of %get.obj_shortdesc[%body_gem%]%
    endif
    if %done_hands% || %done_feet% || %done_wrist% || %done_head% || %done_arms% || %done_legs% || %done_body%
      osend %actor% &0
      osend %actor% You have completed quests for:
    endif
    if %done_hands%
      osend %actor% &0  %get.obj_shortdesc[%hands_reward%]%
    endif
    if %done_feet%
      osend %actor% &0  %get.obj_shortdesc[%feet_reward%]%
    endif
    if %done_wrist%
      osend %actor% &0  %get.obj_shortdesc[%wrist_reward%]%
    endif
    if %done_head%
      osend %actor% &0  %get.obj_shortdesc[%head_reward%]%
    endif
    if %done_arms%
      osend %actor% &0  %get.obj_shortdesc[%arms_reward%]%
    endif
    if %done_legs%
      osend %actor% &0  %get.obj_shortdesc[%legs_reward%]%
    endif
    if %done_body%
      osend %actor% &0  %get.obj_shortdesc[%body_reward%]%
    endif
  else
    osend %actor% &6Status: Not Started&0
  endif
endif
~
#414
Relocate progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= relocate || %arg% /= relocate_spell_quest
  set relocateclasses Sorcerer Cryomancer Pyromancer
  if (%relocateclasses% /= %actor.class%) && %actor.level% >= 60
    return 0
    set stage %actor.quest_stage[relocate_spell_quest]%
    osend %actor% &2&b&uRelocate&0
    osend %actor% Minimum Level: 65
    if %actor.has_completed[relocate_spell_quest]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[relocate_spell_quest]%
      set master1 %get.mob_shortdesc[49250]% in the Black Ice Desert
      set master2 %get.mob_shortdesc[49251]%
      switch %stage%
        case 1
        case 2
          set next the Staff of the Mystics
          set place a druid hiding out beyond Anlun Vale
          set master %master1%
          break
        case 3
        case 4
          if %actor.quest_variable[relocate_spell_quest:greet]% == 0
            set next the Crystal Telescope
            set place an observer of the cold village
            set master %master1%
          else
            set next a glass globe
            set place the Valley of the Frost Elves
            set master %master2%
          endif
          break
        case 5
          set next the Crystal Telescope
          set master %master1%
          break
        case 6
          set next a silver-trimmed spellbook
          set place a tower within a destroyed land
          set master %master1%
          break
        case 7
          set next a map
          set place from a mapper in South Caelia
          set master %master1%
          break
        case 8
        case 9
          set next the Golden Quill
          set place the forest near Baba Yaga's hut
          set master %master1%
      done
      osend %actor% Quest Master: %master%
      osend %actor% &0
      if %stage% != 2 && %stage% != 4 && %stage% != 5 && %stage% != 9
        osend %actor% You are trying to retrieve:
        osend %actor% &3&b%next%&0 from &2&b%place%.&0
      else
        osend %actor% Return &3&b%next%&0 to %master%.
      endif
    endif
  endif
endif
~
#415
Moonwell progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= moonwell || %arg% /= moonwell_spell_quest
  if %actor.class% /= Druid && %actor.level% >= 65
    return 0
    set stage %actor.quest_stage[moonwell_spell_quest]%
    set master %get.mob_shortdesc[16316]%
    osend %actor% &2&b&uMoonwell&0
    osend %actor% Minimum Level: 73
    if %actor.has_completed[moonwell_spell_quest]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[moonwell_spell_quest]%
      osend %actor% Quest Master: %master%
      osend %actor% &0
      switch %stage%
        case 1
        case 2
          * Vine of Mielikki
          set item 16350
          set place an island of lava and fire
          break
        case 3
          * The Heartstone
          set item 48024
          set place an ancient burial site far to the north
          break
        case 4
          * Flask of Eleweiss
          set item 16356
          set place the cult of the ice dragon
          break
        case 5
          * Flask of Eleweiss
          set item 16356
          set place the cult of the ice dragon
          break
        case 6
          * Glittering ruby ring
          set item 5201
          set place a temple dedicated to fire
          break
        case 7
          * Orb of Winds
          set item 16006
          set place a dark fortress to the east
          break
        case 8
          * Jade ring
          set item 49011
          set place a wood nymph on an island of our brothers beset by beasts
          break
        case 10
          * Chaos Orb
          set item 4003
          set place a great dragon hidden in a hellish labyrinth
          break
        case 11
          * Granite Ring
          set item 55020
          set place a large temple hidden in a mountain
      done
      if %stage% < 6 || (%stage% >= 6 && !%actor.quest_variable[moonwell_spell_quest:map]%)
        osend %actor% Bring %get.obj_shortdesc[%item%]% from %place%.
      else
        osend %actor% Bring %master% your bark map.
      endif
      if %stage% > 6
        osend %actor% &0
        osend %actor% If you need a new map, return to %master% and say "&2&bI lost my map&0".
      endif
    endif
  endif
endif
~
#416
Tower in the Wastes progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= tower || %arg% /= wastes || %arg% /= tower_in_the_wastes || %arg% /= krisenna_quest || %arg% /= krisenna quest
  if %actor.level% >= 30
    return 0
    set stage %actor.quest_stage[krisenna_quest]%
    osend %actor% &2&b&uTower in the Wastes&0
    osend %actor% Recommended Level: 40
    if %actor.has_completed[krisenna_quest]%
      set status Completed!
    elseif %actor.quest_stage[krisenna_quest]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[krisenna_quest]%
      osend %actor% Quest Master: %get.mob_shortdesc[12513]%
      osend %actor% &0
      osend %actor% The injured halfling said:
      switch %stage%
        case 1
          set phrase Have you found my brother yet?  He must be in the tower somewhere!
          break
        case 2
          set phrase He carried our grandfather's warhammer.  The warhammer is very precious to my family.  Would you please find it?  I will reward you as best I can!
          break
        case 3
        case 4
          set phrase A... demon has the warhammer, you say?  I must have the warhammer!  Losing my brother is bad enough!
      done
      osend %actor% The halfling said, '%phrase%'
    endif
  endif
endif
~
#417
Emmath Flameball progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= power || %arg% /= flame || %arg% /= flameball || %arg% /= emmath_flameball || %arg% /= emmath
  if %actor.level% >= 75
    return 0
    set stage %actor.quest_stage[emmath_flameball]%
    osend %actor% &2&b&uPower of Flame&0
    osend %actor% Recommended Level: 85
    osend %actor% - This quest can be started at any level but requires level 85 to finish.
    if %actor.has_completed[emmath_flameball]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[emmath_flameball]%
      osend %actor% Quest Master: %get.mob_shortdesc[5230]%
      osend %actor% &0
      switch %stage%
        case 1
          osend %actor% Return to Emmath and ask to prove your &1&bworth&0.
          break
        case 2
          osend %actor% Prove your mastery over fire.
          osend %actor% Bring the three parts of flame: &7&bWhite&0, &bGray&0, and &9&bBlack&0.
          set black %actor.quest_variable[emmath_flameball:17308]%
          set white %actor.quest_variable[emmath_flameball:5211]%
          set gray %actor.quest_variable[emmath_flameball:5212]%
          if %black% || %white% || %gray%
            osend %actor% &0 
            osend %actor% You have delivered:
            if %white%
                osend %actor% - &7&b%get.obj_shortdesc[5211]%&0
            endif
            if %gray%
                osend %actor% - &b%get.obj_shortdesc[5212]%&0
            endif
            if %black%
                osend %actor% - &9&b%get.obj_shortdesc[17308]%&0
            endif
          endif
          osend %actor% &0
          osend %actor% You still need:
          if !%white%
            osend %actor% - &7&b%get.obj_shortdesc[5211]%&0
          endif
          if !%gray%
            osend %actor% - &b%get.obj_shortdesc[5212]%&0
          endif
          if !%black%
            osend %actor% - &9&b%get.obj_shortdesc[17308]%&0
          endif
          break
        case 3
          osend %actor% Bring the renegade &4&bblue flame&0.
      done
    endif
  endif
endif
~
#418
Twisted Sorrow progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= twisted || %arg% /= sorrow || %arg% /= twisted_sorrow || %arg% /= twisted forest quest
  return 0
  osend %actor% &2&b&uTwisted Sorrow&0
  osend %actor% Recommended Level: 10
  if %actor.has_completed[twisted_sorrow]%
    set status Completed!
  elseif %actor.quest_stage[twisted_sorrow]%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  set luck %actor.quest_variable[twisted_sorrow:satisfied_tree:12016]%
  set reverence %actor.quest_variable[twisted_sorrow:satisfied_tree:12017]%
  set reliance %actor.quest_variable[twisted_sorrow:satisfied_tree:12018]%
  set nimbleness %actor.quest_variable[twisted_sorrow:satisfied_tree:12014]%
  set kindness %actor.quest_variable[twisted_sorrow:satisfied_tree:12046]%
  set tree1 %get.room[12016]%
  set tree2 %get.room[12017]%
  set tree3 %get.room[12018]%
  set tree4 %get.room[12014]%
  set tree5 %get.room[12046]%
  if %actor.quest_stage[twisted_sorrow]% == 1
    osend %actor% Quest Master: %get.mob_shortdesc[30214]%
    osend %actor% &0
    osend %actor% Bring drink to awaken the trees from the corruption.
    if (%luck% == 1 || %reverence% == 1 || %reliance% == 1 || %nimbleness% == 1 || %kindness% == 1)
      osend %actor% &0
      osend %actor% You have already awakened the following trees:
      if %luck% == 1
        osend %actor% - &9&b%tree1.name%&0
      endif
      if %reverence% == 1
        osend %actor% - &9&b%tree2.name%&0
      endif
      if %reliance% == 1
        osend %actor% - &9&b%tree3.name%&0
      endif
      if %nimbleness% == 1
        osend %actor% - &9&b%tree4.name%&0
      endif
      if %kindness% == 1
        osend %actor% - &9&b%tree5.name%&0
      endif
    endif
    osend %actor% &0
    osend %actor% Offerings are still needed for:
    if %luck% == 0
      osend %actor% - &2%tree1.name%&0
    endif
    if %reverence% == 0
      osend %actor% - &2%tree2.name%&0
    endif
    if %reliance% == 0
      osend %actor% - &2%tree3.name%&0
    endif
    if %nimbleness% == 0
      osend %actor% - &2%tree4.name%&0
    endif
    if %kindness% == 0
      osend %actor% - &2%tree5.name%&0
    endif
    osend %actor% &0
    osend %actor% Say "follow me" to the hooded druid when you have an offering to present.
  endif
endif
~
#419
Major Globe progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= major globe || %arg% /= globe || %arg% /= major_globe || %arg% /= major_globe_spell
  set relocateclasses Sorcerer Cryomancer Pyromancer
  if %actor.level% >= 50 && %relocateclasses% /= %actor.class%
    return 0
    set stage %actor.quest_stage[major_globe_spell]%
    osend %actor% &2&b&uMajor Globe&0
    osend %actor% Minimum Level: 57
    if %actor.has_completed[major_globe_spell]%
      set status Completed!
    elseif %actor.quest_stage[major_globe_spell]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[major_globe_spell]%
      switch %stage%
        case 1
          set master %get.mob_shortdesc[53450]%
          set task Find Earle and tell him "Lirne sends me."
          break
        case 2
          set master %get.mob_shortdesc[49008]%
          set task Find shale on Fiery Island.
          break
        case 3
          set master %get.mob_shortdesc[49008]%
          set task Find sake in Odaishyozen.
          break
        case 4
          set master %get.mob_shortdesc[49008]%
          set task Find a marigold poultice on a healer in South Caelia.
          break
        case 5
          set master %get.mob_shortdesc[53450]%
          set task Bring %master% the salve Earle prepared.
          break
        case 6
          set master %get.mob_shortdesc[53450]%
          set task &6&bSearch&0 in each &2&blibrary&0 or &2&bstack&0 to find the lost spellbook.
          break
        case 7
          set master %get.mob_shortdesc[53450]%
          set task Bring %get.obj_shortdesc[53452]% to %master%.
          break
        case 8
          set master %get.mob_shortdesc[53450]%
          set plant %actor.quest_variable[major_globe_spell:ward_53453]%
          set mist %actor.quest_variable[major_globe_spell:ward_53454]%
          set water %actor.quest_variable[major_globe_spell:ward_53455]%
          set flame %actor.quest_variable[major_globe_spell:ward_53456]%
          set ice %actor.quest_variable[major_globe_spell:ward_53457]%
          eval wards_left 5 - %actor.quest_variable[major_globe_spell:ward_count]%
          set task Bring %master% &3&b%wards_left% more elemental wards&0, one each from a mist, a water, an ice, a flame, and a plant elemental.
          break
        case 9
          set master %get.mob_shortdesc[53450]%
          set final_item %actor.quest_variable[major_globe_spell:final_item]%
          switch %final_item%
            case 53458
              set place in a border keep
              break
            case 53459
              set place on an emerald isle
              break
            case 53460
              set place within a misty fortress
              break
            default
              set place in an underground city
          done
          set task Find %get.obj_shortdesc[%final_item%]% in %place%.
          break
        case 10
          set master %get.mob_shortdesc[53450]%
          set final_item %actor.quest_variable[major_globe_spell:final_item]%
          set task Deliver %get.obj_shortdesc[%final_item%]% to %master%.
      done
      osend %actor% Quest Master: %master%
      osend %actor% &0
      osend %actor% %task%
      if %stage% == 8
        if %wards_left% < 5
          osend %actor% You have found:
          if %plant% == 2
            osend %actor% %get.obj_shortdesc[53453]%
          endif
          if %mist% == 2
            osend %actor% %get.obj_shortdesc[53454]%
          endif
          if %water% == 2
            osend %actor% %get.obj_shortdesc[53455]%
          endif
          if %flame% == 2
            osend %actor% %get.obj_shortdesc[53456]%
          endif
          if %ice% == 2
            osend %actor% %get.obj_shortdesc[53457]%
          endif
        endif
      endif
    endif
  endif
endif
~
#420
Doom Entrance progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= doom entrance || %arg% /= the planes of doom || %arg% /= planes of doom || %arg% /= doom_entrance || %arg% /= the_planes_of_doom || %arg% /= planes_of_doom || %arg% /= planes
  if %actor.level% >= 85
    return 0
    set stage %actor.quest_stage[doom_entrance]%
    osend %actor% &2&b&uThe Planes of Doom&0
    osend %actor% Minimum Level: 85
    osend %actor% - This quest begins a storyline intended for characters of level 95+
    if %actor.has_completed[doom_entrance]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[doom_entrance]%
      switch %stage%
        case 1
          set master %get.mob_shortdesc[48411]%
          set task Track down the White Hart in the Eldorian Foothills and hold the bloody rag in its presence
          break
        case 2
          set master %get.mob_shortdesc[48411]%
          set task Slay the White Hart and bring its antlers to %master%
          break
        case 3
          set master %get.mob_shortdesc[48410]%
          set task Slay Rhalean's Evil Sister, trapped deep underground
          break
        case 4
          set master %get.mob_shortdesc[48410]%
          set task Return to %master%
          break
        case 5
          set master %get.mob_shortdesc[48412]%
          set task Drop the vial of sunlight before a mockery of the sun in a place steeped in darkness
          break
        case 6
          set master %get.mob_shortdesc[48412]%
          set task Return to %master%
      done
      osend %actor% Quest Master: %master%
      osend %actor% &0
      osend %actor% %task%.
    endif
  endif
endif
~
#421
Sacred Haven progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= infiltrate the sacred haven || %arg% /= sacred_haven || %arg% /= infiltrate_the_sacred_haven || %arg% /= sacred haven || %arg% /= haven
  if %actor.level% >= 25
    return 0
    set stage %actor.quest_stage[sacred_haven]%
    osend %actor% &2&b&uInfiltrate the Sacred Haven&0
    osend %actor% This quest is only available to neutral and evil-aligned characters.
    osend %actor% This quest is infinitely repeatable.
    osend %actor% Recommended Level: 35
    if %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage%
      osend %actor% Quest Master: %get.mob_shortdesc[59029]%
      osend %actor% &0
      set light %actor.quest_variable[sacred_haven:given_light]%
      set key %actor.quest_variable[sacred_haven:find_key]% 
      set blood %actor.quest_variable[sacred_haven:given_blood]%
      set trinket %actor.quest_variable[sacred_haven:given_trinket]%
      set earring %actor.quest_variable[sacred_haven:given_earring]%
      if %stage% == 1 && %light% == 0
        osend %actor% Prove yourself.  Bring %get.obj_shortdesc[59026]% from a priest on the second floor of the Haven.
      elseif %stage% == 1 && %light% == 1
        osend %actor% Ask the figure about their artifacts.
      elseif %stage% == 2 && %key% == 0
        osend %actor% Break the figure's ally out of jail.
      elseif %stage% == 2 && %key% == 1
        osend %actor% Find the key the prisoner stashed in the Haven courtyard.
      elseif %stage% == 2 && %key% == 2
        osend %actor% Bring %get.mob_shortdesc[59029]% their artifacts.
        if %blood% || %trinket% || %earring%
          osend %actor% &0 
          osend %actor% You have brought:
          if %blood% == 1
            osend %actor% - %get.obj_shortdesc[59028]%
          endif
          if %trinket% == 1
            osend %actor% - %get.obj_shortdesc[59029]%
          endif
          if %earring% == 1
            osend %actor% - %get.obj_shortdesc[59030]%
          endif
        endif
        osend %actor% &0 
        osend %actor% You still need to find:
        if %blood% == 0
          osend %actor% - %get.obj_shortdesc[59028]%
        endif
        if %trinket% == 0
          osend %actor% - %get.obj_shortdesc[59029]%
        endif
        if %earring% == 0
          osend %actor% - %get.obj_shortdesc[59030]%
        endif
      endif
    endif
  endif
endif
~
#422
Combat in Eldoria progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= combat in eldoria || %arg% /= combat_in_eldoria || %arg% /= third eldorian guard || %arg% /= third black legion || %arg% /= black_legion || %arg% /= eldorian_guard || %arg% /= third_black_legion || %arg% /= third_eldorian_guard || %arg% /= combat || %arg% /= eldoria
  return 0
  osend %actor% &2&b&uCombat in Eldoria&0
  osend %actor% The Third Black Legion and the Eldorian Guard, along with their allies in Split Skull and the Abbey, are locked in eternal warfare.
  osend %actor% Characters may align themselves with the forces of good or the forces of evil.
  osend %actor% But beware, once made that decision cannot be changed!
  osend %actor% Minimum Level: 10
  if %actor.quest_stage[black_legion]%
    osend %actor% &6Status: Continuous&0&_
    if %actor.quest_variable[Black_Legion:bl_ally]%
      set vnum_trophy1           5504
      set vnum_trophy2           5506
      set vnum_trophy3           5508
      set vnum_trophy4           5510
      set vnum_trophy5           5512
      set vnum_trophy6           5514
      set vnum_trophy7           5516
      set vnum_gem_cap       55570
      set vnum_gem_neck      55571
      set vnum_gem_arm       55572
      set vnum_gem_wrist     55573
      set vnum_gem_gloves    55574
      set vnum_gem_jerkin    55575
      set vnum_gem_robe      55589
      set vnum_gem_belt      55576
      set vnum_gem_legs      55577
      set vnum_gem_boots     55578
      set vnum_gem_mask      55579
      set vnum_gem_symbol    55580
      set vnum_gem_staff     55581
      set vnum_gem_ssword    55582
      set vnum_gem_whammer   55583
      set vnum_gem_flail     55584
      set vnum_gem_shiv      55585
      set vnum_gem_lsword    55586
      set vnum_gem_smace     55587
      set vnum_gem_light     55588
      set vnum_gem_food      55566
      set vnum_gem_drink     55567
      set vnum_cap           5517
      set vnum_neck          5519
      set vnum_arm           5521
      set vnum_wrist         5523
      set vnum_gloves        5525
      set vnum_jerkin        5527
      set vnum_belt          5529
      set vnum_legs          5531
      set vnum_boots         5533
      set vnum_mask          5535
      set vnum_robe          5537
      set vnum_symbol        5515
      set vnum_staff         5539
      set vnum_ssword        5540
      set vnum_whammer       5541
      set vnum_flail         5542
      set vnum_shiv          5543
      set vnum_lsword        5544
      set vnum_smace         5545
      set vnum_light         5553
      set vnum_food          5555
      set vnum_drink         5557
      set legion                 Black Legion
      set master                 %get.mob_shortdesc[4127]% and %get.mob_shortdesc[5512]%
      set status                 %actor.quest_variable[black_legion:bl_faction]%
    elseif %actor.quest_variable[Black_Legion:eg_ally]%
      set vnum_trophy1           5503
      set vnum_trophy2           5505
      set vnum_trophy3           5507
      set vnum_trophy4           5509
      set vnum_trophy5           5511
      set vnum_trophy6           5513
      set vnum_trophy7           5515
      set vnum_gem_cap       55570
      set vnum_gem_neck      55571
      set vnum_gem_arm       55572
      set vnum_gem_wrist     55573
      set vnum_gem_gloves    55574
      set vnum_gem_jerkin    55575
      set vnum_gem_robe      55589
      set vnum_gem_belt      55576
      set vnum_gem_legs      55577
      set vnum_gem_boots     55578
      set vnum_gem_mask      55579
      set vnum_gem_symbol    55580
      set vnum_gem_staff     55581
      set vnum_gem_ssword    55582
      set vnum_gem_whammer   55583
      set vnum_gem_flail     55584
      set vnum_gem_shiv      55585
      set vnum_gem_lsword    55586
      set vnum_gem_smace     55587
      set vnum_gem_light     55588
      set vnum_gem_food      55566
      set vnum_gem_drink     55567
      set vnum_cap           5518
      set vnum_neck          5520
      set vnum_arm           5522
      set vnum_wrist         5524
      set vnum_gloves        5526
      set vnum_jerkin        5528
      set vnum_belt          5530
      set vnum_legs          5532
      set vnum_boots         5534
      set vnum_mask          5536
      set vnum_robe          5538
      set vnum_symbol        5516
      set vnum_staff         5546
      set vnum_ssword        5547
      set vnum_whammer       5548
      set vnum_flail         5549
      set vnum_shiv          5550
      set vnum_lsword        5551
      set vnum_smace         5552
      set vnum_light         5554
      set vnum_food          5556
      set vnum_drink         5558
      set legion                 Eldorian Guard
      set master                 %get.mob_shortdesc[18699]% and %get.mob_shortdesc[5524]%
      set status                 %actor.quest_variable[black_legion:eg_faction]%
    endif
    osend %actor% You are pledged to the %legion%.
    osend %actor% Quest Master: %master%
*   osend %actor% &0
*   osend %actor% The %legion% is interested in:
*   osend %actor% - %get.obj_shortdesc[%vnum_trophy1%]%
*   osend %actor% - %get.obj_shortdesc[%vnum_trophy2%]%
*   osend %actor% - %get.obj_shortdesc[%vnum_trophy3%]%
*   osend %actor% - %get.obj_shortdesc[%vnum_trophy4%]%
*   osend %actor% - %get.obj_shortdesc[%vnum_trophy5%]%
*   osend %actor% - %get.obj_shortdesc[%vnum_trophy6%]%
*   osend %actor% - %get.obj_shortdesc[%vnum_trophy7%]%
    osend %actor% &0  
    osend %actor% You have turned in:
    osend %actor% %actor.quest_variable[black_legion:%vnum_trophy1%_trophies]% %get.obj_pldesc[%vnum_trophy1%]%
    osend %actor% %actor.quest_variable[black_legion:%vnum_trophy2%_trophies]% %get.obj_pldesc[%vnum_trophy2%]% 
    osend %actor% %actor.quest_variable[black_legion:%vnum_trophy3%_trophies]% %get.obj_pldesc[%vnum_trophy3%]%
    osend %actor% %actor.quest_variable[black_legion:%vnum_trophy4%_trophies]% %get.obj_pldesc[%vnum_trophy4%]%   
    osend %actor% %actor.quest_variable[black_legion:%vnum_trophy5%_trophies]% %get.obj_pldesc[%vnum_trophy5%]%     
    osend %actor% %actor.quest_variable[black_legion:%vnum_trophy6%_trophies]% %get.obj_pldesc[%vnum_trophy6%]%
    osend %actor% %actor.quest_variable[black_legion:%vnum_trophy7%_trophies]% %get.obj_pldesc[%vnum_trophy7%]%
    osend %actor% &0  
    osend %actor% Your current %legion% faction status is %status%.
    if %status% >= 200
      osend %actor% You have reached the maximum faction status.
    endif
    osend %actor% &0 
    if %status% >= 20
      osend %actor% You have access to:
      osend %actor% &3&b%get.obj_shortdesc[%vnum_food%]%&0 for &5%get.obj_shortdesc[%vnum_gem_food%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_food%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_drink%]%&0 for &5%get.obj_shortdesc[%vnum_gem_drink%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_drink%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_cap%]%&0 for &5%get.obj_shortdesc[%vnum_gem_cap%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_cap%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_ssword%]%&0 for &5%get.obj_shortdesc[%vnum_gem_ssword%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_ssword%_reward]%)&_
    endif
    if (%status% >= 40)
      osend %actor% &3&b%get.obj_shortdesc[%vnum_neck%]%&0 for &5%get.obj_shortdesc[%vnum_gem_neck%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_neck%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_staff%]%&0 for &5%get.obj_shortdesc[%vnum_gem_staff%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_staff%_reward]%)&_
    endif
    if (%status% >= 55)
      osend %actor% &3&b%get.obj_shortdesc[%vnum_arm%]%&0 for &5%get.obj_shortdesc[%vnum_gem_arm%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_arm%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_whammer%]%&0 for &5%get.obj_shortdesc[%vnum_gem_whammer%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_whammer%_reward]%)&_
    endif
    if (%status% >= 70)
      osend %actor% &3&b%get.obj_shortdesc[%vnum_wrist%]%&0 for &5%get.obj_shortdesc[%vnum_gem_wrist%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_wrist%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_flail%]%&0 for &5%get.obj_shortdesc[%vnum_gem_flail%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_flail%_reward]%)&_
    endif
    if (%status% >= 85)
      osend %actor% &3&b%get.obj_shortdesc[%vnum_gloves%]%&0 for &5%get.obj_shortdesc[%vnum_gem_gloves%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_gloves%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_symbol%]%&0 for &5%get.obj_shortdesc[%vnum_gem_symbol%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_symbol%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_light%]%&0 for &5%get.obj_shortdesc[%vnum_gem_light%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_light%_reward]%)&_
    endif
    if (%status% >= 100)
      osend %actor% &3&b%get.obj_shortdesc[%vnum_belt%]%&0 for &5%get.obj_shortdesc[%vnum_gem_belt%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_belt%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_shiv%]%&0 for &5%get.obj_shortdesc[%vnum_gem_shiv%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_shiv%_reward]%)&_
    endif
    if (%status% >= 115)
      osend %actor% &3&b%get.obj_shortdesc[%vnum_boots%]%&0 for &5%get.obj_shortdesc[%vnum_gem_boots%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_boots%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_lsword%]%&0 for &5%get.obj_shortdesc[%vnum_gem_lsword%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_lsword%_reward]%)&_
    endif
    if (%status% >= 130)
      osend %actor% &3&b%get.obj_shortdesc[%vnum_legs%]%&0 for &5%get.obj_shortdesc[%vnum_gem_legs%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_legs%_reward]%)&_
    endif
    if (%status% >= 145)
      osend %actor% &3&b%get.obj_shortdesc[%vnum_robe%]%&0 for &5%get.obj_shortdesc[%vnum_gem_robe%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_robe%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_jerkin%]%&0 for &5%get.obj_shortdesc[%vnum_gem_jerkin%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_jerkin%_reward]%)&_
    endif
    if (%status% >= 160)
      osend %actor% &3&b%get.obj_shortdesc[%vnum_mask%]%&0 for &5%get.obj_shortdesc[%vnum_gem_mask%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_mask%_reward]%)&_
      osend %actor% &3&b%get.obj_shortdesc[%vnum_smace%]%&0 for &5%get.obj_shortdesc[%vnum_gem_smace%]%&0
      osend %actor% - times claimed: (%actor.quest_variable[black_legion:%vnum_smace%_reward]%)&_
    endif
    if (%statu% < 160)
      osend %actor% As your standing with the %legion% improves you will have access to more rewards.
    endif
  else
    osend %actor% &6Status: Not Started&0
  endif
endif
~
#423
Phoenix Sous Chef progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= phoenix || %arg% /= sous || %arg% /= chef || %arg% /= resort_cooking || %arg% /= resort || %arg% /= cooking || %arg% /= phoenix_sous_chef
  if %actor.level% >= 50
    return 0
    set stage %actor.quest_stage[resort_cooking]%
    osend %actor% &2&b&uPhoenix Sous Chef&0
    osend %actor% Recommended Level: 90
    osend %actor% - This quest can be started at any level but requires level 90+ to finish.
    if %actor.has_completed[resort_cooking]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0
    if %stage% > 0 && !%actor.has_completed[resort_cooking]%
      osend %actor% &0
      osend %actor% Quest Master: %get.mob_shortdesc[10308]%
      osend %actor% &0 
      switch %stage%
        case 1
          set recipe Peach Cobbler
          set item1 61501
          set item2 23754
          set item3 3114
          set item4 35001
          break
        case 2
          set recipe Seafood Salad
          set item1 49024
          set item2 23750
          set item3 23722
          set item4 8003
          set item5 12515
          set item6 1606
          break
        case 3
          set recipe Fish Stew
          set item1 55213
          set item2 30002
          set item3 10030
          set item4 12552
          set item5 23757
          set item6 18509
          set item7 10311
          break
        case 4
          set recipe Honey-Glazed Ham
          set item1 41011
          set item2 8350
          set item3 2001
          set item4 50207
          set item5 6106
          break
        case 5
          set recipe Saffroned Jasmine Rice
          set item1 58019
          set item2 37013
          set item3 23760
      done
      osend %actor% You are trying to make:
      osend %actor% &0==========&7&b%recipe%&0==========
      if %item1%
        osend %actor% &0  %get.obj_shortdesc[%item1%]%
      endif
      if %item2%
        osend %actor% &0  %get.obj_shortdesc[%item2%]%
      endif
      if %item3%
        osend %actor% &0  %get.obj_shortdesc[%item3%]%
      endif
      if %item4%
        osend %actor% &0  %get.obj_shortdesc[%item4%]%
      endif
      if %item5%
        osend %actor% &0  %get.obj_shortdesc[%item5%]%
      endif
      if %item6%
        osend %actor% &0  %get.obj_shortdesc[%item6%]%
      endif
      if %item7%
        osend %actor% &0  %get.obj_shortdesc[%item7%]%
      endif
      osend %actor% &0 
      osend %actor% You have retrieved:
      set nothing 1
      if %actor.quest_variable[resort_cooking:item1]%
        osend %actor% &0  %get.obj_shortdesc[%item1%]%
        set nothing 0
      endif
      if %actor.quest_variable[resort_cooking:item2]%
        osend %actor% &0  %get.obj_shortdesc[%item2%]%
        set nothing 0
      endif
      if %actor.quest_variable[resort_cooking:item3]%
        osend %actor% &0  %get.obj_shortdesc[%item3%]%
        set nothing 0
      endif
      if %actor.quest_variable[resort_cooking:item4]%
        osend %actor% &0  %get.obj_shortdesc[%item4%]%
        set nothing 0
      endif
      if %actor.quest_variable[resort_cooking:item5]%
        osend %actor% &0  %get.obj_shortdesc[%item5%]%
        set nothing 0
      endif
      if %actor.quest_variable[resort_cooking:item6]%
        osend %actor% &0  %get.obj_shortdesc[%item6%]%
        set nothing 0
      endif
      if %actor.quest_variable[resort_cooking:item7]%
        osend %actor% &0  %get.obj_shortdesc[%item7%]%
        set nothing 0
      endif
      if %nothing%
        osend %actor% &0  Nothing.
      endif
    endif
  endif
endif
~
#424
Rhell Forest progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= rhell || %arg% /= forest || %arg% /= mystery || %arg% /= ursa_quest || %arg% /= mystery_of_the_rhell_forest
  if %actor.level% >= 35
    return 0
    set stage %actor.quest_stage[ursa_quest]%
    set path %actor.quest_variable[ursa_quest:choice]%
    osend %actor% &2&b&uMystery of the Rhell Forest&0
    osend %actor% Find the sick merchant in the Rhell Forest and help end his distress.
    osend %actor% Recommended Level: 45
    if %actor.has_completed[ursa_quest]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[ursa_quest]%
      osend %actor% Quest Master: %get.mob_shortdesc[62506]%
      osend %actor% &0
      osend %actor% The merchant told you:
      switch %stage%
        case 1
          if !%path%
            osend %actor% Please visit one of these powerful people, and ask for a cure:
            osend %actor% &0 
            osend %actor% - The Emperor, on a nearby island of very refined people 
            osend %actor% - Ruin Wormheart in the Red City
            osend %actor% - The crazy hermit of the swamps
          else
            osend %actor% Return with the cure.
          endif
        case 2
          if %path% == 1
            osend %actor% Please bring me some pepper.  Prices are always cheapest up near Anduin.
          elseif %path% == 2
            osend %actor% Return to me a particular sceptre of gold that symbolized a king's undying leadership.
          elseif %path% == 3
            osend %actor% A devourer has a ring with power to heal.  Please bring me this ring.
          endif
          break
        case 3
          if %path% == 1
            osend %actor% Please bring me a plant, found as "a bit of bones and plants" from Blue-Fog Trail.
          elseif %path% == 2
            osend %actor% Please bring me an emblem of a king's power.  An emblem of the sun.  The one written of in legends of the warring gods in the far north.
          elseif %path% == 3
            osend %actor% Find the Golhen DrubStatt or whatever that the hermit wrote about from the Highlands, and bring it back to me quickly.
          endif
          break
        case 4
          if %path% == 1
            osend %actor% I need a particular thorny wood.  Because of it's unpleasant nature there are some unpleasant people that make staves and walking sticks out of it.  Please find one, and bring it back here.
          elseif %path% == 2
            osend %actor% Please bring be the dagger that radiates glorious light.  The priests of South Caelia know its fierce beauty.
          elseif %path% == 3
            osend %actor% Bring me milk, in any container.
          endif
          break
        case 5
          if %path% == 1
            osend %actor% Bring me a pitcher from the hot springs or the Dancing Dolphin Inn.  Either will do.
          elseif %path% == 2
            osend %actor% I can't do this sober!  Fetch me something to drink, and make it strong!
          elseif %path% == 3
            osend %actor%  Off of the great road is a lumber mill.  Their smith has an anvil that will do our work perfectly.  Please fetch it for me.
          endif
          break
        case 6
          if %path% == 2
            osend %actor% I need a large container for a sarcophagus, like a body-bag or a large chest.
          endif
      done
    endif
  endif
endif
~
#425
Resurrection progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= resurrection || %arg% /= resurrect || %arg% /= resurrection_quest || %arg% /= res
  if %actor.class% /= Cleric || %actor.class% /= Priest || %actor.class% /= Diabolist && %actor.level% >= 75
    return 0
    set stage %actor.quest_stage[resurrection_quest]%
    osend %actor% &2&b&uResurrection&0
    osend %actor% Minimum Level: 81
    if %actor.has_completed[resurrection_quest]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[resurrection_quest]%
      osend %actor% Quest Master: %get.mob_shortdesc[8550]%
      osend %actor% &0
      switch %stage%
        case 1
          osend %actor% Ask Norisent about payment.
          halt
          break
        case 2
          osend %actor% Norisent told you:
          osend %actor% Do something to interfere with Ziijhan.  I hear he has a bishop locked up in the Cathedral dungeon.
          halt
          break
        case 3
          osend %actor% Return to Norisent.
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
        case 9
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
        case 10
          set hunt %get.mob_shortdesc[52003]% and %get.mob_shortdesc[52015]%
          set mob1 52003
          set mob2 52015
          set item %get.obj_shortdesc[52001]%
          break
        case 11
          set item %get.obj_shortdesc[52001]%
          break
        case 12
          osend %actor% Destroy Norisent!
          halt
      done
    endif
    if %stage% == 4 || %stage% == 6 || %stage% == 8 || %stage% == 10
      set target1 %actor.quest_variable[resurrection_quest:%mob1%]%
      set target2 %actor.quest_variable[resurrection_quest:%mob2%]%
      if %stage% != 6 && %stage% != 10
        set target3 %actor.quest_variable[resurrection_quest:%mob3%]%
      endif
      osend %actor% You must eliminate:
      osend %actor% %hunt%
      if %stage% == 4 || %stage% == 8
        if %target1% && %target2% && %target3%
          osend %actor% &0
          osend %actor% Show Norisent the death talisman.
          halt
        endif
      elseif %stage% == 6
        if %target1% && (%target2% == 2)
          osend %actor% &0
          osend %actor% Show Norisent the death talisman.
          halt
        endif
      else
        if %target1% && %target2%
          osend %actor% &0
          osend %actor% Show Norisent the death talisman.
          halt
        endif
      endif   
      if %target1% || %target2% || %target3%
        osend %actor% &0 
        osend %actor% You have destroyed:
        if %target1%
          osend %actor% - %get.mob_shortdesc[%mob1%]%
        endif
        if %stage% == 6
          osend %actor% - %target2% %get.mob_shortdesc[%mob2%]%
        else
          if %target2%
            osend %actor% - %get.mob_shortdesc[%mob2%]%
          endif
        endif
        if %stage% == 4 || %stage% == 8
          if %target3%
            osend %actor% - %get.mob_shortdesc[%mob3%]%
          endif
        endif
      endif
      if (%stage% != 6 && (!%target1% || !%target2% || !%target3%)) || (%stage% == 6 && (!%target1% || %target2% < 2))
        osend %actor% &0
        osend %actor% You still need to dispatch:
        if !%target1%
          osend %actor% - %get.mob_shortdesc[%mob1%]%
        endif
        if %stage% == 6
          if %target2% < 2
            eval xeg (2 - %target2%)
            osend %actor% - %xeg% %get.mob_shortdesc[%mob2%]%
          endif
        else
          if !%target2%
            osend %actor% - %get.mob_shortdesc[%mob2%]%
          endif
        endif
        if %stage% != 6 && %stage% != 10
          if !%target3%
            osend %actor% - %get.mob_shortdesc[%mob3%]%
          endif
        endif
        osend %actor% &0
        osend %actor% Return %item% as proof.
      endif
      osend %actor% &0
      osend %actor% Don't forget the banishment phrase: &4&bDhewsost Konre&0
    elseif %stage% == 5 || %stage% == 7 || %stage% == 9 || %stage% == 11
      osend %actor% Bring Norisent back %item%.
    endif
    if %stage% >= 4 && !%actor.has_completed[resurrection_quest]%
      osend %actor% &0  
      osend %actor% If you need a new talisman, return to Norisent and say &9&b"I need a new talisman"&0.
    endif
  endif
endif
~
#426
Group Heal progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= group heal || %arg% /= heal || %arg% /= group_heal
  if %actor.level% >= 50
    if %actor.class% /= Priest || %actor.class% /= Cleric || %actor.class% /= Diabolist
      return 0
      set stage %actor.quest_stage[group_heal]%
      osend %actor% &2&b&uGroup Heal&0
      osend %actor% Minimum Level: 57
      if %actor.has_completed[group_heal]%
        set status Completed!
      elseif %stage%
        set status In Progress
      else
        set status Not Started
      endif
      osend %actor% &6Status: %status%&0&_
      if %stage% > 0 && !%actor.has_completed[group_heal]%
        osend %actor% Quest Master: %get.mob_shortdesc[18521]%
        osend %actor% &0
        switch %stage%
          case 1
            osend %actor% Track down the bandit raider in the Gothra desert and recover the stolen medical supplies.
            break
          case 2
            osend %actor% Return the medical supplies stolen by the bandit raider.
            break
          case 3
          case 4
            osend %actor% Locate the records of a group healing ritual in a lost kitchen in the Great Northern Swamp.
            break
          case 5
            osend %actor% Visit every &7&bchef&0 and &7&bcook&0 to get their notes on the healing ritual.
            osend %actor% &0
            set recipe1 %actor.quest_variable[group_heal:18515]%
            set recipe2 %actor.quest_variable[group_heal:18516]%
            set recipe3 %actor.quest_variable[group_heal:18517]%
            set recipe4 %actor.quest_variable[group_heal:18518]%
            set recipe5 %actor.quest_variable[group_heal:18519]%
            set recipe6 %actor.quest_variable[group_heal:18520]%
            if %recipe1% || %recipe2% || %recipe3% || %recipe4% || %recipe5% || %recipe6%
              osend %actor% You have already brought notes from:
              if %recipe1%
                osend %actor% - %get.mob_shortdesc[8307]%
              endif
              if %recipe2%
                osend %actor% - %get.mob_shortdesc[51007]%
              endif
              if %recipe3%
                osend %actor% - %get.mob_shortdesc[18512]%
              endif
              if %recipe4%
                osend %actor% - %get.mob_shortdesc[30003]%
              endif
              if %recipe5%
                osend %actor% - %get.mob_shortdesc[50203]%
              endif
              if %recipe6%
                osend %actor% - %get.mob_shortdesc[10308]%
              endif
              osend %actor% &0
            endif
            eval total 6 - %actor.quest_variable[group_heal:total]%
            if %total% == 1
              osend %actor% Bring notes from %total% more chef.
            else
              osend %actor% Bring notes from %total% more chefs.
            endif
            osend %actor% &0 
            osend %actor %If you need a new copy of the Rite, go to the doctor and say: &3&b"I lost the Rite"&0.
            break
          case 6
            if %actor.quest_variable[group_heal:total]% == 6
              osend %actor% Give %get.obj_shortdesc[18514]% to the doctor.
              halt
            endif
            osend %actor% You are delivering the medical packages to &7&binjured&0, &7&bwounded&0, &7&bsick&0, or &7&bhobbling&0 creatures.
            eval total (5 - %actor.quest_variable[group_heal:total]%)
            set person1 %actor.quest_variable[group_heal:18506]%
            set person2 %actor.quest_variable[group_heal:46414]%
            set person3 %actor.quest_variable[group_heal:43020]%
            set person4 %actor.quest_variable[group_heal:12513]%
            set person5 %actor.quest_variable[group_heal:36103]%
            set person6 %actor.quest_variable[group_heal:58803]%
            set person7 %actor.quest_variable[group_heal:30054]%
            osend %actor% &0
            if %person1% || %person2% || %person3% || %person4% || %person5% || %person6% || %person7% 
              osend %actor% You have aided:
              if %person1%
                osend %actor% - %get.mob_shortdesc[18506]%
              endif
              if %person2%
                osend %actor% - %get.mob_shortdesc[46414]%
              endif
              if %person3%
                osend %actor% - %get.mob_shortdesc[43020]%
              endif
              if %person4%
                osend %actor% - %get.mob_shortdesc[12513]%
              endif
              if %person5%
                osend %actor% - %get.mob_shortdesc[36103]%
              endif
              if %person6%
                osend %actor% - %get.mob_shortdesc[58803]%
              endif
              if %person7%
                osend %actor% - %get.mob_shortdesc[30054]%
              endif
              osend %actor% &0
            endif
            if %total% == 1
              osend %actor% You need to deliver %total% more packet.
            else
              osend %actor% You need to deliver %total% more packets.
            endif
        done
      endif
    endif
  endif
endif
~
#427
Group Armor progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= group armor || %arg% /= group_armor
  if (%actor.level% >= 50 && %actor.class% /= Priest) || (%actor.level% >= 65 && %actor.class% /= Cleric)
    return 0
    set stage %actor.quest_stage[group_armor]%
    osend %actor% &2&b&uGroup Armor&0
    if %actor.class% /= Priest
      osend %actor% Minimum Level: 57
    elseif %actor.class% /= Cleric
      osend %actor% Minimum Level: 65
    endif
    if %actor.has_completed[group_armor]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[group_armor]%
      osend %actor% Quest Master: %get.mob_shortdesc[59099]%
      osend %actor% &0
      switch %stage%
        case 1
          set item1 %actor.quest_variable[group_armor:6118]%
          set item2 %actor.quest_variable[group_armor:11704]%
          set item3 %actor.quest_variable[group_armor:11707]%
          set item4 %actor.quest_variable[group_armor:16906]%
          set obj1 %get.obj_shortdesc[6118]%
          set obj2 %get.obj_shortdesc[11704]%
          set obj3 %get.obj_shortdesc[11707]%
          set obj4 %get.obj_shortdesc[16906]%
          set step locate items that cast the spell &7&barmor&0
          break
        case 2
          set step find a new &7&bforging hammer&0
          break
        case 3
        case 4
          set step take the forging hammer where light reaches deep underground and &7&b[commune]&0
          break
        case 5
          set step find a suitable amulet to be the focus of this spell
          set item1 %actor.quest_variable[group_armor:12500]%
          set obj1 %get.obj_shortdesc[12500]%
          break
        case 6
          set step locate ethereal items to provide protective energy to the amulet
          set item1 %actor.quest_variable[group_armor:47004]%
          set item2 %actor.quest_variable[group_armor:47018]%
          set item3 %actor.quest_variable[group_armor:53003]%
          set obj1 %get.obj_shortdesc[47004]%
          set obj2 %get.obj_shortdesc[47018]%
          set obj3 %get.obj_shortdesc[53003]%
      done
      osend %actor% You are trying to %step%.
      if %stage% == 1 || %stage% == 5 || %stage% == 6
        if %item1% || %item2% || %item3% || %item4%
          osend %actor% &0
          osend %actor% You have already brought:
          if %item1%
            osend %actor% - %obj1%
          endif
          if %item2%
            osend %actor% - %obj2%
          endif
          if %item3%
            osend %actor% - %obj3%
          endif
          if %item4%
            osend %actor% - %obj4%
          endif
        endif
        osend %actor% &0 
        osend %actor% You still need to locate:
        if !%item1%
          osend %actor% - &3&b%obj1%&0
        endif
        if %stage% == 1 || %stage% == 6
          if !%item2%
            osend %actor% - &3&b%obj2%&0
          endif
          if !%item3%
            osend %actor% - &3&b%obj3%&0
          endif
          if %stage% == 1
            if !%item4%
              osend %actor% - &3&b%obj4%&0
            endif
          endif
        endif
      endif
    endif
  endif
endif
~
#428
Supernova progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= supernova || %arg% /= nova
  if %actor.level% >= 85 && %actor.class% /= Pyromancer
    return 0
    set stage %actor.quest_stage[supernova]%
    osend %actor% &2&b&uSupernova&0
    osend %actor% Minimum Level: 89
    if %actor.has_completed[supernova]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[supernova]%
      osend %actor% Quest Master: the pyromancer guildmasters
      osend %actor% &0
      osend %actor% Your task is to:
      switch %stage%
        case 1
          osend %actor% Find one of Phayla's lamps.
          break
        case 2
          osend %actor% Return to a pyromancer guildmaster with Phayla's lamp and ask about Supernova.
          break
        case 3
          osend %actor% Find clues to Phayla's whereabouts.
          osend %actor% &0
          osend %actor% She likes to visit the material plane to engage in her favorite leisure activities.
          set clue %actor.quest_variable[supernova:step3]%
          switch %clue%
            case 4318
              osend %actor% Recently, she was spotted in Anduin, taking in a show from the best seat in the house.
              break
            case 10316
              osend %actor% I understand she frequents the hottest spring at the popular resort up north.
              break
            case 58062
              osend %actor% She occasionally visits a small remote island theatre, where she enjoys meditating in their reflecting room.
              break
          done
          break
        case 4
          osend %actor% Find further clues to Phayla's whereabouts.
          osend %actor% &0
          switch %actor.quest_variable[supernova:step4]%
            case 18577
              set clue2 I continue my journey where the sun rises amidst a sea of swirling worlds.
              * The Abbey, the rising sun room
              break
            case 17277
              set clue2 Atop a tower I visit a master who waits to give his final examination.
              * Citadel of Testing
              break
            case 8561
              set clue2 I study in a secret place above a hall of misery beyond a gallery of horrors.
              * Cathedral of Betrayal near Norisent
          done
          * end clue2 switch
          break
        case 5
          osend %actor% Find yet another clue to Phayla's whereabouts.
          osend %actor% &0 
          switch %actor.quest_variable[supernova:step5]%
            case 53219
              set clue3 Where DID the lizard men get that throne from?  I'll see if I can find out.
              * Lizard King's throne room, Sunken
              break
            case 47343
              set clue3 They often wonder what would happen if bones could talk.  I'll ask one who can make that happen!
              * Kryzanthor, Graveyard
              break
            case 16278
              set clue3 Waves of sand hold the remains of a child of the Sun God.  Supposedly.  I'll have to see for myself.
              * Imanhotep, Pyramid
          done
          * end clue3 switch
          break
        case 6
          osend %actor% Solve the riddle to deduce the location of the gateway to Phayla's realm.
          osend %actor% &0 
          set step7 %actor.quest_variable[supernova:step7]%
          switch %actor.quest_variable[supernova:step6]%
            case 58657
              * A Hummock of Grass in the Beachhead
              if %step7% == 1
                set clue4 s pfqzqgc wq kecwk qy xug fwinlugev
              elseif %step7% == 2
                set clue4 d hlwzsuc rf xbnwk aq tyo oisukhvkq
              elseif %step7% == 3
                set clue4 s oidfgjy fy yyojl au hyx tlotazlou
              endif
              break
            case 35119
              * A Pile of Stones in the Brush Lands
              if %step7% == 1
                set clue4 s xtpr qj kbzrru mf bsi otykp weafw
              elseif %step7% == 2
                set clue4 d pzvr sx kwoeof mf lke sbhwz ddnuc
              elseif %step7% == 3
                set clue4 s wwcx gm gkhflg zg los skmzv ctfkg
              endif
              break
            case 55422
              * The Trail Overlooking the Falls in the dark mountains
              if %step7% == 1
                set clue4 lpp xecmd wgiensgstrt vlw nlpyu mf bsi qcvc uzyaveavd
              elseif %step7% == 2
                set clue4 whv deead ovvbysgclnx dui xsolj sa xzw gaiu zsmfwazxf
              elseif %step7% == 3
                set clue4 los kkspz fowyzfhcpbx mzl tredz we mzl rrkc tclglhwel
              endif
          done
          * end clue4 switch
          switch %step7%
            case 1
              set clue5 What disappears as soon as you say its name? 
              * Answer: Silence
              break
            case 2
              set clue5 The more there is, the less you see. What am I?
              * Answer: Darkness
              break
            case 3
              set clue5 What word becomes shorter when you add two to it?
              * Answer: Short
          done
          * end clue 5 switch
          break
        case 7
          osend %actor% Talk to Phayla.
          halt
      done
      * ends the stage switch
      if %stage% > 3
        if (%actor.inventory[48917]% || %actor.wearing[48917]%)
          if %stage% == 4
            osend %actor% Learning is a life-long process.
            osend %actor% %clue2%
          elseif %stage% == 5
            osend %actor% History is so fascinating!
            osend %actor% %clue3%
          elseif %stage% == 6
            osend %actor% I know you're following me.  Answer this:
            osend %actor% "%clue5%"
            osend %actor% With the answer you can find the gate to my home here:
            osend %actor% %clue4%
            osend %actor% &0 
            osend %actor% You will need additional solar energy to power the gate.
            osend %actor% Hidden in the dimensional folds around Nordus is an appropriate source.
          endif
        else
          osend %actor% Your notes are a jumble of unintelligible squiggles.
          osend %actor% You must have %get.obj_shortdesc[48917]% to read them!
        endif
      endif
    endif
  endif
endif
~
#429
Meteorswarm progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= meteor || %arg% /= meteorswarm
  if (%actor.class% /= Sorcerer && %actor.level% >= 65) || (%actor.class% /= Pyromancer && %actor.level% >= 75)
    return 0
    set stage %actor.quest_stage[meteorswarm]%
    osend %actor% &2&b&uMeteorswarm&0
    if %actor.class% /= Sorcerer
      osend %actor% Minimum Level: 73
    elseif %actor.class% /= Pyromancer
      osend %actor% Minimum Level: 81
    endif
    if %actor.has_completed[meteorswarm]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[meteorswarm]%
      osend %actor% Quest Master: %get.mob_shortdesc[48250]%
      osend %actor% &0
      if %actor.quest_variable[meteorswarm:new]% /= yes
        osend %actor% Go find a new meteorite.
        halt
      elseif %actor.quest_variable[meteorswarm:new]% /= no
        osend %actor% Bring the new meteorite to McCabe.
        halt
      endif
      switch %stage%
        case 1
          set task Find Jemnon and ask about the rock demon.  McCabe said, "He's in some tavern, no doubt, waiting for some new blunder to embark on."
          break
        case 2
          set task Find a suitable meteor focus from the rock demon in Templace.
          break
        case 3
          if %actor.quest_variable[meteorswarm:earth]% == 0
            set task Show him the meteorite.
          else
            if %actor.quest_variable[meteorswarm:fire]% == 0
              set task Find and kill the high fire priest in the Lava Tunnels.  Then enter the lava bubble below his secret chambers.
            else
              set task Find the lava bubble in the high fire priest's secret chambers.
            endif
          endif
          break
        case 4
          if %actor.quest_variable[meteorswarm:fire]% == 1
            set task Return to McCabe.
          elseif %actor.quest_variable[meteorswarm:fire]% == 2
            set task Convince the ancient dragon Dargentan to teach you the ways of air magic.
          endif
          break
        case 5
          if %actor.quest_variable[meteorswarm:air]% == 0
            set task Show him the meteorite now that you have mastered earth, fire, and air.
          else
            set task Take your finished focus and unleash its potential!
          endif
      done
      osend %actor% McCabe wants you to:
      osend %actor% %task%
      if %actor.quest_variable[meteorswarm:earth]%
        osend %actor% &0 
        osend %actor% If you somehow lost the meteorite, return to McCabe and say &1&b"I lost &1&bthe meteorite"&0.
      endif
    endif
  endif
endif
~
#430
Banish progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= banish
  if %actor.level% >= 50 && (%actor.class% /= Priest || %actor.class% /= Diabolist)
    return 0
    set stage %actor.quest_stage[banish]%
    set master %get.mob_shortdesc[30216]%
    osend %actor% &2&b&uBanish&0
    osend %actor% Minimum Level: 65
    if %actor.has_completed[banish]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[banish]%
      osend %actor% Quest Master: %master%
      osend %actor% &0
      switch %stage%
        case 1
          set mob 41119
          set place her chamber under the ocean waves
          set known Nothing.
          break
        case 2
          set mob 53313
          set place the frozen tunnels of the north
          set known v
          break
        case 3
          set mob 37000
          set place a deep and ancient mine
          set known vi
          break
        case 4
          set mob 48005
          set place a room filled with art in an ancient barrow
          set known vib
          break
        case 5
          set mob 53417
          set place the cold valley of the far north
          set known vibu
          break
        case 6
          set mob 23811
          set place a nearby fortress of clouds and crystals
          set known vibug
          break
        case 7
          osend %actor% Return to %get.mob_shortdesc[30216]% and speak the prayer aloud: &5&bvibugp&0
          halt
      done
      if %actor.quest_variable[banish:greet]% == 1
        osend %actor% To learn Banish you must next kill %get.mob_shortdesc[%mob%]% in %place%.
      else
        osend %actor% Return to %master% for further instruction.
      endif
      osend %actor% &0 
      osend %actor% &0Your knowledge of the prayer so far: &6&b%known%&0
    endif
  endif
endif
~
#431
The Great Rite progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= great || %arg% /= rite || %arg% /= the great rite || %arg% /= megalith || %arg% /= megalith_quest || %arg% /= the_great_rite || %arg% /= sacred megalith quest || %arg% /= sacred_megalith_quest || %arg% /= sacred megalith
  if %actor.level% >= 50
    return 0
    set stage %actor.quest_stage[megalith_quest]%
    set job1 %actor.quest_variable[megalith_quest:item1]%
    set job2 %actor.quest_variable[megalith_quest:item2]%
    set job3 %actor.quest_variable[megalith_quest:item3]%
    set job4 %actor.quest_variable[megalith_quest:item4]%
    osend %actor% &2&b&uThe Great Invocation&0
    osend %actor% A group of witches has uncovered a powerful set of massive standing stones.
    osend %actor% Help them with their mystic ritual.
    osend %actor% Recommended Level: 70
    osend %actor% - This quest can be started at any level but requires level 70 to finish.
    if %actor.has_completed[megalith_quest]%
      set status Completed!
    elseif %actor.has_failed[megalith_quest]%
      set status Failed
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if (%stage% > 0 && !%actor.has_completed[megalith_quest]%) || %actor.has_failed[megalith_quest]%
      set master %get.mob_shortdesc[12301]%
      switch %stage% 
        case 1
          set task Replace the sacred prophetic implements.
          * salt 23756
          set item1 salt
          * Goblet or chalice 41110 or 41111 or 18512
          set item2 a goblet or chalice
          * censer 8507 or 17300
          set item3 a censer
          * candles 8612 or 58809
          set item4 candles
          * give to the priestess
          break
        case 2
          *
          * Must be done East South West North
          *
          * thin sheet of cloud to Keeper of the East
          set receive1 %get.mob_shortdesc[12305]%
          set item1 %get.obj_shortdesc[8301]%
          * the fiery eye to Keeper of the South
          set receive2 %get.mob_shortdesc[12304]%
          set item2 %get.obj_shortdesc[48109]%
          * water from room 12463 to Keeper of the West
          set receive3 %get.mob_shortdesc[12306]%
          set item3 water from %get.obj_shortdesc[12352]%
          * granite ring to Keeper of the North
          set receive4 %get.mob_shortdesc[12303]%
          set item4 %get.obj_shortdesc[55020]%
          if %job1% && %job2% && %job3% && %job4%
            set task Finish calling the elements!  Return to %get.mob_shortdesc[12301]% and say "&7&bUnder the watchful eye of Earth, Air, Fire, and Water, we awaken this hallowed ground!&0"
          else
            set task Visit the Keepers and call the elements
            unset master
            set master The Keepers of East, South, West, and North
          endif
          break
        case 3
          set task Locate three holy reliquaries.
          set item1 a holy prayer bowl
          set item2 a piece of a goddess's regalia
          set item3 a faerie relic from the land of the Reverie made manifest
          break
        case 4
          set prayer %actor.quest_variable[megalith_quest:prayer]%
          set summon %actor.quest_variable[megalith_quest:summon]%
          set invoke %actor.quest_variable[megalith_quest:invoke]%
          if %prayer% == 1
            set task Return to %master% and say, "&7&bGreat Lady of the Stars, hear our prayer!&0"
          elseif %summon% == 1 || %summon% == 2 || %summon% == 3
            set task Return to %master% and say, "&7&bWe summon and stir thee!&0"
          elseif %invoke% == 1 || %invoke% == 2 || %invoke% == 3
            set task Return to %master% and say, "&7&bWe invoke thee!&0"
          endif
          break
        case 5
          set task Kneel before the High Mother to receive Her blessing.
      done
      osend %actor% Quest Master: %master%
      osend %actor% &0
      if %actor.has_failed[megalith_quest]%
        osend %actor% Return to %master% and ask to try again.
        halt
      else
        osend %actor% %task%
      endif
      if %stage% < 4
        * list items already given
        if %job1%% || %job2% || %job3% || %job4%
          osend %actor% &0 
          osend %actor% You have already retrieved:
          if %job1%
            osend %actor% - &7&b%item1%&0
          endif
          if %job2%
            osend %actor% - &7&b%item2%&0
          endif
          if %job3%
            osend %actor% - &7&b%item3%&0
          endif
          if %job4%
            osend %actor% - &7&b%item4%&0
          endif
        endif
        * list items to be returned
        osend %actor% &0
        if %stage% == 2
          if !%job1%
            osend %actor% Assist &7&b%receive1%&0.
            If %actor.quest_variable[megalith_quest:east]%
              osend %actor% She needs %item1%.  Help her first.
            else
              osend %actor% Check with her to see what she needs.
            endif
            osend %actor% &0
          endif
          if !%job2%
            osend %actor% Assist &1&b%receive2%&0.
            If %actor.quest_variable[megalith_quest:south]%
              osend %actor% She needs %item2%.  Help her second.
            else
              osend %actor% Check with her to see what she needs.
            endif
            osend %actor% &0 
          endif  
          if !%job3%
            osend %actor% Assist &6&b%receive3%&0.
            If %actor.quest_variable[megalith_quest:west]%
              osend %actor% She needs %item3%.  Help her third.
            else
              osend %actor% Check with her to see what she needs.
            endif  
            osend %actor% &0
          endif
          if !%job4%
            osend %actor% Assist &2&b%receive4%&0.
            If %actor.quest_variable[megalith_quest:north]%
              osend %actor% She needs %item4%.  Help her last.
            else
              osend %actor% Check with her to see what she needs.
            endif
            osend %actor% &0
          endif
        else
          osend %actor% You still need to retrieve:
          if !%job1%
            osend %actor% - &7&b%item1%&0
          endif
          if !%job2%
            osend %actor% - &7&b%item2%&0
          endif
          if !%job3%
            osend %actor% - &7&b%item3%&0
          endif
          if %stage% == 1
            if !%job4%
              osend %actor% - &7&b%item4%&0
            endif
          endif
        endif
      endif
    endif
  endif
endif
~
#432
Word of Command progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= word || %arg% /= command || %arg% /= word of command || %arg% /= word_command
  if %actor.level% >= 65
    if %actor.class% /= diabolist || %actor.class% /= Priest
      return 0
      set stage %actor.quest_stage[word_command]%
      osend %actor% &2&b&uWord of Command&0
      if %actor.has_completed[word_command]%
        set status Completed!
      elseif %stage%
        set status In Progress
      else
        set status Not Started
      endif
      osend %actor% &6Status: %status%&0&_
    endif
    if %stage% > 0 && !%actor.has_completed[word_command]%
      osend %actor% Quest Master: %get.mob_shortdesc[43021]%
      osend %actor% &0
      osend %actor% Help %get.mob_shortdesc[43021]% escape from Demise Keep!
      osend %actor% %get.mob_shortdesc[43017]% and its minions will stop at nothing to keep %get.mob_shortdesc[43021]% trapped in its clutches!
    endif
  endif
endif
~
#433
Heavens Gate progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= heavens gate || %arg% /= heavens || %arg% /= heaven's gate || %arg% /= heaven's || %arg% /= heavens_gate || %arg% /= heaven's_gate
  if %actor.level% >= 75 && %actor.class% /= Priest
    return 0
    set stage %actor.quest_stage[heavens_gate]%
    osend %actor% &2&b&uHeavens Gate&0
    osend %actor% Minimum Level: 81
    if %actor.has_completed[heavens_gate]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[heavens_gate]%
      osend %actor%: Quest Master: %get.mob_shortdesc[13333]%
      osend %actor% &0 
      switch %stage%
        case 1
          osend %actor% You received a vision a silver prayer bowl brought before the starlight and &6&b[put] &7&bon the &6&b[pedestal]&0.
          break
        case 2
          set key1 %actor.quest_variable[heavens_gate:4005]%
          set key2 %actor.quest_variable[heavens_gate:12142]%
          set key3 %actor.quest_variable[heavens_gate:23709]%
          set key4 %actor.quest_variable[heavens_gate:47009]%
          set key5 %actor.quest_variable[heavens_gate:49008]%
          set key6 %actor.quest_variable[heavens_gate:52012]%
          set key7 %actor.quest_variable[heavens_gate:52013]%
          osend %actor% You received a vision of seven keys to seven gates brought to the pedestal before the starlight.
          if %key1% || %key2% || %key3% || %key4% || %key5% || %key6% || %key7%
            osend %actor% &0 
            osend %actor% You have returned:
            if %key1%
              osend %actor% %get.obj_shortdesc[4005]%
            endif
            if %key2%
              osend %actor% %get.obj_shortdesc[12142]%
            endif
            if %key3%
              osend %actor% %get.obj_shortdesc[23709]%
            endif
            if %key4%
              osend %actor% %get.obj_shortdesc[47009]%
            endif
            if %key5%
              osend %actor% %get.obj_shortdesc[49008]%
            endif
            if %key6%
              osend %actor% %get.obj_shortdesc[52012]%
            endif
            if %key7%
              osend %actor% %get.obj_shortdesc[52013]%
            endif
          endif
          osend %actor% &0
          osend %actor% You must still seek out:
          if !%key1%
            osend %actor% &6&bA small skeleton key forged of night and shadow&0
            osend %actor% &0hidden deep in a twisted labyrinth.&0
          endif
          if !%key2%
            osend %actor% &0
            osend %actor% &6&bA key made from a piece of the black and pitted wood&0
            osend %actor% &0typical of trees in the Twisted Forest near Mielikki.&0
          endif
          if !%key3%
            osend %actor% &0
            osend %actor% &6&bA large, black key humming with magical energy&0
            osend %actor% &0from a twisted cruel city in a huge underground cavern.&0
          endif
          if !%key4%
            osend %actor% &0
            osend %actor% &6&bA key covered in oil&0
            osend %actor% &0kept by a long-dead caretaker in a necropolis.&0
          endif
          if !%key5%
            osend %actor% &0
            osend %actor% &6&bA rusted but well cared for key&0
            osend %actor% &0carried by an enormous griffin.&0
          endif
          if !%key7%
            osend %actor% &0
            osend %actor% &6&bA golden plated, wrought-iron key&0
            osend %actor% &0held at the gates to a desecrated city.&0
          endif
          if !%key6%
            osend %actor% &0
            osend %actor% &6&bOne nearly impossible to see&0
            osend %actor% &0guarded by a fiery beast with many heads.&0
          endif
          break
        case 3
          set sealed %actor.quest_variable[heavens_gate:sealed]%
          set seal1 %actor.quest_variable[heavens_gate:51077]%
          set seal2 %actor.quest_variable[heavens_gate:16407]%
          set seal3 %actor.quest_variable[heavens_gate:16094]%
          set seal4 %actor.quest_variable[heavens_gate:55735]%
          set seal5 %actor.quest_variable[heavens_gate:49024]%
          set seal6 %actor.quest_variable[heavens_gate:55126]%
          set seal7 %actor.quest_variable[heavens_gate:55112]%
          osend %actor% You saw visions of seven rifts in the fabric of reality which you must &6&bseal&0.
          if %seal1% || %seal2% || %seal3% || %seal4% || %seal5% || %seal6% || %seal7%
            osend %actor% &0
            osend %actor% You have already sealed the rifts in:
            if %seal1%
              * Nordus
              set room %get.room[51077]%
              osend %actor% %room.name% 
            endif
            if %seal2%
              * Mystwatch demon
              set room %get.room[16407]%
              osend %actor% %room.name%
            endif
            if %seal3%
              * Mystwatch fortress
              set room %get.room[16094]%
              osend %actor% %room.name%
            endif
            if %seal4%
              * Black rock trail
              set room %get.room[55735]%
              osend %actor% %room.name%
            endif
            if %seal5%
              * Griffin
              set room %get.room[49024]%
              osend %actor% %room.name%
            endif
            if %seal6%
              * Huitzipia - war
              set room %get.room[55126]%
              osend %actor% %room.name%
            endif
            if %seal7%
              * Xapizo - death
              set room %get.room[55112]%
              osend %actor% %room.name%
            endif
          endif
          osend %actor% &0
          osend %actor% You received visions of:
          if !%seal1%
            osend %actor% &6&bAn arch hidden in another plane&0
            osend %actor% &0granting demons access to an enchanted village of mutants.&0
            osend %actor% &0
          endif
          if !%seal2%
            osend %actor% &6&bAn archway that delivers demons&0
            osend %actor% &0to the fortress of the dead.&0
            osend %actor% &0
          endif
          if !%seal3%
            osend %actor% &6&bA portal from a fortress of the undead&0
            osend %actor% &0to a realm of demons.&0
            osend %actor% &0
          endif
          if !%seal4%
            osend %actor% &6&bA portal from black rock&0
            osend %actor% &0to black ice.&0
            osend %actor% &0
          endif
          if !%seal5%
            osend %actor% &6&bA pool hidden under a well&0
            osend %actor% &0on an island filled with ferocious beasts.&0
            osend %actor% &0
          endif
          if !%seal6%
            osend %actor% &6&bA pool in a temple of ice and stone&0
            osend %actor% &0leading to the realm of a war god.&0
            osend %actor% &0
          endif
          if !%seal7%
            osend %actor% &6&bA pool in a temple of ice and stone&0
            osend %actor% &0leading to the realm of a death god.&0
            osend %actor% &0  
          endif
          switch %sealed%
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
              halt
          done
          if %sealed%
            osend %actor% The words you captured from your vision: &6&b%phrase%&0
          endif
          osend %actor% &0 
          osend %actor% If you need a new Key, beseech the starlight, &6&b"Grant me a new key"&0.
          break
        case 4
          osend %actor% The starlight manifested as a heavenly raven.
          osend %actor% This time, at last, it spake:&_
          osend %actor% &6&b'I I I I am the book.  Open me prophet; read; decypher.&0
          osend %actor% &6&bOn you, in you, in your blood, they write, have written.&0
          osend %actor% &6&bSpeak it but aloud to know the path of heaven for I I I I am the final key.&0
          osend %actor% &6&bI I I I have shown you visions, and through me you shall read.'&0&_
          osend %actor% &3yamo lv soeeiy vrtvln eau okia khz lrrvzryp gvxrj bzjbie hi&0
      done
    endif
  endif
endif
~
#434
Dragons Health Progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= dragons || %arg% /= dragon's || %arg% /= health || %arg% /= dragons_health || %arg% /= dragon's_health
  if %actor.level% >= 85 && %actor.class% /= Cleric || %actor.class% /= Priest
    return 0
    set stage %actor.quest_stage[dragons_health]%
    osend %actor% &2&b&uDragons Health&0
    osend %actor% Minimum Level: 89
    if %actor.has_completed[dragons_health]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[dragons_health]%
      osend %actor% Quest Master: %get.mob_shortdesc[58610]%
      osend %actor% &0
      switch %stage%
        case 1
          set dragon the blue dragon in the Tower in the Wastes
          set treasure its crystal
          break
        case 2
          set dragon Tri-Aszp
          set treasure one of her scales
          break
        case 3
          set dragon Thelriki and Jerajai
          set treasure the jewel in their hoard
          break
        case 4
          set dragon Sagece
          set treasure her skins and shields
          break
        case 5
          eval total 10000000 - %actor.quest_variable[dragons_health:hoard]%
          eval plat %total% / 1000
          eval gold %total% / 100 - %plat% * 10
          eval silv %total% / 10 - %plat% * 100 - %gold% * 10
          eval copp %total%  - %plat% * 1000 - %gold% * 100 - %silv% * 10
          *now the price can be reported 
          osend %actor% The new hatchling's hoard needs enriching.&_
          osend %actor% &0&3&b%plat% platinum, %gold% gold, %silv% silver, %copp% copper&0&_
          osend %actor% &0more in treasure or coins ought to do it.
          halt
      done
      osend %actor% You are trying to:
      osend %actor% - kill %dragon% and return with %treasure%.
      if %stage% == 3
        set thelriki %actor.quest_variable[dragons_health:thelriki]%
        set jerajai %actor.quest_variable[dragons_health:jerajai]%
        if %thelriki% || %jerajai%
          osend %actor% &0 
          osend %actor% You have slain:
          if %thelriki%
            osend %actor% - Thelriki
          endif
          if %jerajai%
            osend %actor% - Jerajai
          endif
        endif
        osend %actor% &0 
        osend %actor% You must still:
        if !%thelriki%
          osend %actor% - kill Thelriki
        endif
        if !%jerajai%
          osend %actor% - kill Jerajai
        endif
        osend %actor% - bring the jewel in their hoard
      elseif %stage% == 4
        set item1 %actor.quest_variable[dragons_health:52016]%
        set item2 %actor.quest_variable[dragons_health:52017]%
        set item3 %actor.quest_variable[dragons_health:52022]%
        set item4 %actor.quest_variable[dragons_health:52023]%
        set sagece %actor.quest_variable[dragons_health:sagece]%
        if %item1% || %item2% || %item3% || %item4% || %sagece%
          osend %actor% &0
          osend %actor% You have already:
          if %sagece%
            osend %actor% - slain Sagece of Raymif
          endif
          if %item1%
            osend %actor% - brought Sagece's skin
          endif
          if %item2%
          osend %actor% - brought Sagece's shield
          endif
          if %item3%
            osend %actor% - brought the skin from Sagece's hoard
          endif
          if %item4%
            osend %actor% - brought the shield from Sagece's hoard
          endif
        endif
        osend %actor% &0 
        osend %actor% You must still:
        if !%sagece%
          osend %actor% - kill Sagece of Raymif
        endif
        if !%item1%
          osend %actor% - bring Sagece's skin
        endif
        if !%item2%
          osend %actor% - bring Sagece's shield
        endif
        if !%item3%
          osend %actor% - find the skin in Sagece's hoard
        endif
        if !%item4%
          osend %actor% - find the shield in Sagece's hoard
        endif
      endif
    endif
  endif
endif
~
#435
Creeping Doom progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= creeping || %arg% /= creeping doom || %arg% /= creeping_doom
  if %actor.level% >= 75 && %actor.class% /= Druid
    return 0
    set stage %actor.quest_stage[creeping_doom]%
    osend %actor% &2&b&uCreeping Doom&0
    osend %actor% Minimum Level: 81
    if %actor.has_completed[creeping_doom]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[creeping_doom]%
      osend %actor% Quest Master: %get.mob_shortdesc[61527]%
      osend %actor% &0
      switch %stage%
        case 1
          set item1 11812
          set place1 an assassin vine on Mist Mountain
          set item2 16213
          set place2 the great pyramid
          set item3 48029
          set place3 Rhalean's evil sister in the northern barrow
          set step gathering Nature's Rage
          break
        case 2
          set essence %actor.quest_variable[creeping_doom:spiders]%
          eval total 11 - %actor.quest_variable[creeping_doom:spiders]%
          osend %actor% You are collecting essences of swarms from flies, insects, spiders, bugs, scorpions, giant ant people, etc.
          osend %actor% &0
          osend %actor% You have found %essence% essences.
          osend %actor% You need to find %total% more.
          osend %actor% &0
          osend %actor% Remember, the tougher the bug, the better your chances of finding essences.
          halt
          break
        case 3
          set step locate three sources of Nature's Vengeance.
          set item1 48416
          set place1 the elder tremaen in the elemental Plane of Fire
          set item2 52034
          set place2 the burning tree in Templace
          set item3 62503
          set place3 the Treant in the eldest Rhell's forest
          break
        case 4
          osend %actor% Take the Essence of Nature's Vengeance and drop it at the entrance to the logging camp, then return to the angry pixie.
          halt
          break
        case 5
          osend %actor% return to the very angry pixie.
          halt
      done
      set job1 %actor.quest_variable[creeping_doom:%item1%]%
      set job2 %actor.quest_variable[creeping_doom:%item2%]%
      set job3 %actor.quest_variable[creeping_doom:%item3%]%
      osend %actor% You are trying to %step%.
      osend %actor% &0
      if %job1% || %job2% || %job3%
        osend %actor% You have brought me:
        if %job1%
          osend %actor% - %get.obj_shortdesc[%item1%]%
        endif
        if %job2%
          osend %actor% - %get.obj_shortdesc[%item2%]%
        endif
        if %job3%
          osend %actor% - %get.obj_shortdesc[%item3%]%
        endif
      endif
      osend %actor% &0 
      osend %actor% You still need:
      if !%job1%
        osend %actor% - %get.obj_shortdesc[%item1%]% from %place1%.
      endif
      if !%job2%
        osend %actor% - %get.obj_shortdesc[%item2%]% from %place2%.
      endif
      if !%job3%
        osend %actor% - %get.obj_shortdesc[%item3%]% from %place3%.
      endif
    endif
  endif
endif
~
#436
Wall of Ice progress journal~
1 m 100
journal quest quest-journal~
if (%arg% /= wall && %arg% /= ice) || %arg% /= wall of ice || %arg% /= wall_ice || %arg% /= wall_of_ice
  if %actor.class% /= Cryomancer && %actor.level% >= 50
    return 0
    osend %actor% &2&b&uWall of Ice&0
    osend %actor% Minimum Level: 57
    if %actor.has_completed[wall_ice]%
      set status Completed!
    elseif %actor.quest_stage[wall_ice]%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.quest_stage[wall_ice]% == 1
      osend %actor% Quest Master: %get.mob_shortdesc[53316]%
      osend %actor% &0
      osend %actor% Collect 20 blocks of living ice from ice creatures.
      osend %actor% Say "Crystalize" in their presence to cast the spell of living ice.
      set have %actor.quest_variable[wall_ice:blocks]%
      eval need (20 - %have%)
      osend %actor% &0
      osend %actor% You have brought &6&b%have% blocks of living ice.&0
      osend %actor% &0
      osend %actor% You still need &6&b%need%&0 more.
      osend %actor% &0
      osend %actor% If you need a new copy of the spell of living ice, return to the sculptor and say, "&6&bplease replace the spell&0".
    endif
  endif
endif
~
#437
Flood progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= flood
  if %actor.class% /= Cryomancer && %actor.level% >= 75
    return 0
    set stage %actor.quest_stage[flood]%
    osend %actor% &2&b&uFlood&0
    osend %actor% Minimum Level: 81
    if %actor.has_completed[flood]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[flood]%
      osend %actor% Quest Master: %get.mob_shortdesc[39012]%
      osend %actor% &0
      if %stage% == 1
        set fog The Blue-Fog
        set phoenix Phoenix Feather Hot Springs
        set falls Three-Falls River
        set green The Greengreen Sea
        set witch Sea's Lullaby
        set frost Frost Lake
        set black The Black Lake
        set kod The Dreaming River
        set water1 %actor.quest_variable[flood:water1]%
        set water2 %actor.quest_variable[flood:water2]%
        set water3 %actor.quest_variable[flood:water3]%
        set water4 %actor.quest_variable[flood:water4]%
        set water5 %actor.quest_variable[flood:water5]%
        set water6 %actor.quest_variable[flood:water6]%
        set water7 %actor.quest_variable[flood:water7]%
        set water8 %actor.quest_variable[flood:water8]%
        set item2 %actor.quest_variable[flood:item2]%
        set item3 %actor.quest_variable[flood:item3]%
        set item4 %actor.quest_variable[flood:item4]%
        set item6 %actor.quest_variable[flood:item6]%
        set item7 %actor.quest_variable[flood:item7]%
        osend %actor% Rally the Great Waters of Ethilien for %get.mob_shortdesc[39012]%.
        osend %actor% &0
        if %water1% || %water2% || %water3% || %water4% || %water5% || %water6% || %water7% || %water8%
          osend %actor% You have rallied:
          if %water1%
            osend %actor% - &4%fog%&0
          endif
          if %water2%
            osend %actor% - &4%phoenix%&0
          endif
          if %water3%
            osend %actor% - &4%falls%&0
          endif
          if %water4%
            osend %actor% - &4%green%&0
          endif
          if %water5%
            osend %actor% - &4%witch%&0
          endif
          if %water6%
            osend %actor% - &4%frost%&0
          endif
          if %water7%
            osend %actor% - &4%black%&0
          endif
          if %water8%
            osend %actor% - &4%kod%&0
          endif
          osend %actor% &0
        endif 
        * list items to be returned
        osend %actor% You must still convince:
        if !%water1%
          osend %actor% - &4&b%fog%&0
          osend %actor% &0
        endif
        if !%water2%
          osend %actor% - &4&b%phoenix%&0
          If %item2% == 1
            osend %actor% &0    Bring it %get.obj_shortdesc[58401]% to heat its springs.
            osend %actor% &0    Say &4&bSpirit I have returned&0 when you return.
          endif
          osend %actor% &0
        endif
        if !%water3%
          osend %actor% - &4&b%falls%&0
          If %item3% == 1
            osend %actor% &0    Find a bell and dance for them.
            osend %actor% &0    Say &4&bSpirit I have returned&0 when you return.
          endif
          osend %actor% &0
        endif
        if !%water4%
          osend %actor% - &4&b%green%&0
          if %item4% == 1
            osend %actor% &0    Feed her as many different foods as you can until she is full.
            osend %actor% &0    Say &4&bSpirit I have returned&0 when you return.
          endif
          osend %actor% &0
        endif
        if !%water5%
          osend %actor% - &4&b%witch%&0
          osend %actor% &0
        endif
        if !%water6%
          osend %actor% - &4&b%frost%&0
          If %item6% == 1
            osend %actor% &0    Force her to join the cause.
          endif
          osend %actor% &0
        endif
        if !%water7%
          osend %actor% - &4&b%black%&0
          if %item7% == 1
            osend %actor% &0    Bring it an eternal light to swallow into its blackness.
            osend %actor% &0    Say &4&bSpirit I have returned&0 when you return.
          endif
          osend %actor% &0
        endif
        if !%water8%
          osend %actor% - &4&b%kod%&0
          osend %actor% &0
        endif
        osend %actor% Tell them: &4&bthe Arabel Ocean calls for aid&0 and longs for &4&brevenge&0.
      elseif %stage% ==2
        osend %actor% Return the Heart of the Ocean to %get.mob_shortdesc[39012]%!
      endif
      osend %actor% &0
      osend %actor% If you lost the Heart, say &4&bI lost the heart&0.
    endif
  endif
endif
~
#438
Ice Shards progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= ice shards || %arg% /= shards || %arg% /= ice_shards
  if %actor.class% /= Cryomancer && %actor.level% >= 85
    return 0
    set stage %actor.quest_stage[ice_shards]%
    osend %actor% &2&b&uIce Shards&0
    osend %actor% Minimum Level: 89
    if %actor.has_completed[ice_shards]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[ice_shards]%
      osend %actor% Quest Master: %get.mob_shortdesc[10300]%
      osend %actor% &0
      switch %stage%
        case 1
          set book1 %actor.quest_variable[ice_shards:16209]%
          set book2 %actor.quest_variable[ice_shards:18505]%
          set book3 %actor.quest_variable[ice_shards:55003]%
          set book4 %actor.quest_variable[ice_shards:58415]%
          osend %actor% You are looking for four books of mystic knowledge.
          if %book1% || %book2% || %book3% || %book4%
            osend %actor% &0
            osend %actor% You have brought me:
            if %book1%
              osend %actor% - &3&b%get.obj_shortdesc[16209]%&0
            endif
            if %book2%
              osend %actor% - &3&b%get.obj_shortdesc[18505]%&0
            endif
            if %book3%
              osend %actor% - &3&b%get.obj_shortdesc[55003]%&0
            endif
            if %book4%
              osend %actor% - &3&b %get.obj_shortdesc[58415]%&0
            endif
          endif
          osend %actor% &0
          osend %actor% You still need to find:
          if !%book1%
            osend %actor% - &3&b%get.obj_shortdesc[16209]%&0
          endif
          if !%book2%
            osend %actor% - &3&b%get.obj_shortdesc[18505]%&0
          endif
          if !%book3%
            osend %actor% - &3&b%get.obj_shortdesc[55003]%&0
          endif
          if !%book4%
            osend %actor% - &3&b%get.obj_shortdesc[58415]%&0
          endif
          break
        case 2
          osend %actor% Find the Codex of War.
          break
        case 3
          osend %actor% You are looking for any records or journals Commander Thraja keeps.
          break
        case 4
          osend %actor% Talk to the pawnbroker in Anduin about the Butcher of Anduin so you can find his map.
          break
        case 5
          osend %actor% Talk to Slevvirik in Ogakh about the Butcher of Anduin so you can find his map.
          break
        case 6
          osend %actor% Bring the map of Ickle from the Butcher of Anduin.
          break
        case 7
          osend %actor% You are looking for any kind of written clues about the library at Shiran in Ysgarran's Keep in Frost Valley.
          break
        case 8
          osend %actor% You are looking for the Book of Redemption, whatever that is.
          break
        case 9
          osend %actor% You are looking for the lost library of Shiran in Frost Valley!
          break
        case 10
          osend %actor% Bring Aqua Mundi to Khysan!
      done
    endif
  endif
endif
~
#439
Waterform progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= waterform
  if %actor.class% /= Cryomancer && %actor.level% >= 65
    return 0
    set stage %actor.quest_stage[waterform]%
    osend %actor% &2&b&uWaterform&0
    osend %actor% Minimum Level: 73
    if %actor.has_completed[waterform]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[waterform]%
      set master %get.mob_shortdesc[2810]%
      osend %actor% Quest Master: %master%
      osend %actor% &0 
      switch %stage%
        case 1
          osend %actor% Find a piece of armor made of water to serve as the basis of your new form.
          break
        case 2
          osend %actor% You need a special vessel to gather water in.  Kill Tri-Aszp and get a large bone from her.
          break
        case 3
          osend %actor% You need a special vessel to gather water in.  Give %master% the large bone from a grown white dragon so it can make you one.
          break
        case 4
          set region1 %actor.quest_variable[waterform:region1]%
          set region2 %actor.quest_variable[waterform:region2]%
          set region3 %actor.quest_variable[waterform:region3]%
          set region4 %actor.quest_variable[waterform:region4]%
          set region5 %actor.quest_variable[waterform:region5]%
          osend %actor% Collect samples of living water from four different regions.
          if %region1% || %region2% || %region3% || %region4% || %region5%
            osend %actor% &0 
            osend %actor% You already have samples from:
            if %region1%
              osend %actor% - &4The Blue Fog trails and waters&0
            endif
            if %region2%
              osend %actor% - &4Nordus&0
            endif
            if %region3%
              osend %actor% - &4Layveran Labyrinth&0
            endif
            if %region4%
              osend %actor% - &4The Elemental Plane of Water&0
            endif
            if %region5%
              osend %actor% - &4The sunken castle&0
            endif
          endif
          osend %actor% &0 
          eval samples 4 - (%region1% + %region2% + %region3% + %region4% + %region5%)
          osend %actor% You need &4&b%samples%&0 more.
          break
        case 5
          osend %actor% Give %master% the cup so it can see the samples.
          break
        case 6
          set water1 %actor.quest_variable[waterform:3296]%
          set water2 %actor.quest_variable[waterform:58405]%
          set water3 %actor.quest_variable[waterform:53319]%
          set water4 %actor.quest_variable[waterform:55804]%
          set water5 %actor.quest_variable[waterform:58701]%
          set water6 %actor.quest_variable[waterform:37014]%
          osend %actor% You are looking for six unique sources of water.
          if %water1% || %water2% || %water3% || %water4% || %water5% || %water6%
            osend %actor% &0 
            osend %actor% You have already analyzed water from:
            if %water1%
              osend %actor% - &4a granite pool in the village of Mielikki&0
            endif
            if %water2%
              osend %actor% - &4a sparkling artesian well in the Realm of the King of Dreams&0
            endif
            if %water3%
              osend %actor% - &4a crystal clear fountain in the caverns of the Ice Cult&0
            endif
            if %water4%
              osend %actor% - &4the creek in the Eldorian Foothills&0
            endif
            if %water5%
              osend %actor% - &4the wishing well at the Dancing Dolphin in South Caelia&0
            endif
            if %water6%
              osend %actor% - &4an underground brook in the Minithawkin Mines&0
            endif
          endif
          osend %actor% &0 
          osend %actor% You still need to analyze water from:
          if !%water1%
            osend %actor% - &6&ba granite pool in the village of Mielikki&0
          endif
          if !%water2%
            osend %actor% - &6&ba sparkling artesian well in the Realm of the King of Dreams&0
          endif
          if !%water3%
            osend %actor% - &6&ba crystal clear fountain in the caverns of the Ice Cult&0
          endif
          if !%water4%
            osend %actor% - &6&bthe creek in the Eldorian Foothills&0
          endif
          if !%water5%
            osend %actor% - &6&bthe wishing well at the Dancing Dolphin in South Caelia&0
          endif
          if !%water6%
            osend %actor% - &6&ban underground brook in the Minithawkin Mines&0
          endif
          break
        case 7
          osend %actor% Just return the cup to %master% and you're done!
      done
      if %stage% > 3
        osend %actor% &0 
        osend %actor% If you need a new cup, return to %master% and say "&3&bI need a new cup&0".
      endif
    endif
  endif
endif
~
#440
Shift Corpse progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= shift || %arg% /= corpse || %arg% /= shift_corpse
  if %actor.class% /= Necromancer && %actor.level% >= 90
    return 0
    set stage %actor.quest_stage[shift_corpse]%
    osend %actor% &2&b&uShift Corpse&0
    osend %actor% Minimum Level: 97
    if %actor.has_completed[shift_corpse]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[shift_corpse]%
      osend %actor% Quest Master: the necromancer guild masters
      osend %actor% &0
      if %stage% == 1
        osend %actor% Steal the divine spark of Lokari, God of the Moonless Night.
        osend %actor% &0
        osend %actor% Give him %get.obj_shortdesc[6228]%, then destroy him.
        osend %actor% &0
        osend %actor% If you need a new crystal, say &9&b"I need a new crystal"&0.
      elseif %stage% == 2
        osend %actor% Destroy Lokari!
      endif
    endif
  endif
endif
~
#441
Degeneration progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= degeneration
  if %actor.class% /= Necromancer && %actor.level% >= 75
    return 0
    set stage %actor.quest_stage[degeneration]%
    osend %actor% &2&b&uDegeneration&0
    osend %actor% Minimum Level: 81
    if %actor.has_completed[degeneration]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% && !%actor.has_completed[degeneration]%
      osend %actor% Quest Master: %get.mob_shortdesc[5526]%
      osend %actor% &0 
      switch %stage%
        case 1
          osend %actor% Find Yajiro in Odaishyozen and bring back his book.
          break
        case 2
          osend %actor% Find Mesmeriz in the Minithawkin Mines and bring back his necklace.
          break
        case 3
          osend %actor% Find Luchiaans in Nordus and bring back his mask.
          break
        case 4
          osend %actor% Find Voliangloch in Demise Keep and bring back his rod.
          break
        case 5
          osend %actor% Find Kryzanthor in the Graveyard and bring back his robe.
          break
        case 6
          osend %actor% Find Ureal the Lich in the Barrow and bring back his statuette.
          break
        case 7
        case 8
          osend %actor% Find Norisent in the Cathedral of Betrayal and bring back his book.
          break
        case 9
          osend %actor% Find the enormous ruby hidden under a stairway.
          break
      done
    endif
  endif
endif
~
#442
Wizard Eye progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= wizard eye || %arg% /= wizard || %arg% /= wizard_eye
  if %actor.class% /= Sorcerer && %actor.level% >= 75
    return 0
    set stage %actor.quest_stage[wizard_eye]%
    set item1 %actor.quest_variable[wizard_eye:item1]%
    set item2 %actor.quest_variable[wizard_eye:item2]%
    set item3 %actor.quest_variable[wizard_eye:item3]%
    set item4 %actor.quest_variable[wizard_eye:item4]%
    osend %actor% &2&b&uWizard Eye&0
    osend %actor% Minimum Level: 81
    if %actor.has_completed[wizard_eye]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[wizard_eye]%
      set master %get.mob_shortdesc[55013]%
      switch %stage%
        case 1
          set master %get.mob_shortdesc[32410]%
          set task You are trying to find %master% in South Caelia Highlands to ask her about Wizard Eye.
          break
        case 2
          set task Find marigold poultice from a healer on the beachhead and give it to %master%.
          break
        case 3
          set master %get.mob_shortdesc[49003]%
          set task Go visit %master% of Griffin Isle to ask her about Wizard Eye and see what you need to do next.
          break
        case 4
          set master %get.mob_shortdesc[49003]%
          set task Have %master% make you an herbal sachet.
          set thing1 %get.obj_shortdesc[2329]%
          set thing2 %get.obj_shortdesc[23753]%
          set thing3 %get.obj_shortdesc[48005]%
          break
        case 5
          set task Give %master% the sachet.
          break
        case 6
          set task You are looking for the apothecary in Anduin.
          break
        case 7
          set master %get.mob_shortdesc[6022]%
          set task Get The Green Woman to make you incense.
          set thing1 %get.obj_shortdesc[23754]%
          set thing2 %get.obj_shortdesc[3298]%
          set thing3 %get.obj_shortdesc[23847]%
          set thing4 %get.obj_shortdesc[18001]%
          break
        case 8
          set task Give %master% the incense.
          break
        case 9
        case 10
          set master %get.mob_shortdesc[48410]%
          set thing1 %get.obj_shortdesc[3218]%
          set thing2 %get.obj_shortdesc[53424]%
          set thing3 %get.obj_shortdesc[43021]%
          set thing4 %get.obj_shortdesc[4003]%
          set task See the Oracle of Justice.
          break
        case 11
          set task Give %master% the crystal ball.
          break
        case 12
          set task Return to %master% and lay back and go to sleep.
      done
      osend %actor% Quest Master: %master%
      osend %actor% &0 
      osend %actor% %task%
      if %stage% == 4 || %stage% == 7 || %stage% == 10
        if %item1% || %item2% || %item3% || %item4%
          osend %actor% &0
          osend %actor% You have already brought:
          if %item1%
            osend %actor% - %thing1%
          endif
          if %item2%
            osend %actor% - %thing2%
          endif
          if %item3%
            osend %actor% - %thing3%
          endif
          if %item4%
            osend %actor% - %thing4%
          endif
        endif
        osend %actor% &0 
        osend %actor% You still need to bring:
        if !%item1%
          osend %actor% - %thing1%
        endif
        if !%item2%
          osend %actor% - %thing2%
        endif
        if !%item3%
          osend %actor% - %thing3%
        endif
        if %stage% != 4
          if !%item4%
            osend %actor% - %thing4%
          endif
        endif
      endif
    endif
  endif
endif
~
#443
Blur progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= blur
  if %actor.class% /= Ranger && %actor.level% >= 75
    return 0
    set east %actor.quest_variable[blur:east]%
    set etimer %actor.quest_variable[blur:east_timer]%
    set west %actor.quest_variable[blur:west]%
    set wtimer %actor.quest_variable[blur:west_timer]%
    set north %actor.quest_variable[blur:north]%
    set ntimer %actor.quest_variable[blur:north_timer]%
    set south %actor.quest_variable[blur:south]%
    set stimer %actor.quest_variable[blur:south_timer]% 
    set timer %actor.quest_variable[blur:timer]% 
    set stage %actor.quest_stage[blur]%
    set master %get.mob_shortdesc[1818]%
    osend %actor% &2&b&uBlur&0
    osend %actor% Minimum Level: 81
    if %actor.has_completed[blur]%
      set status Completed!
    elseif %actor.has_failed[blur]%
      set status Failed
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if !%stage%
      osend %actor% Heal the spirits in the Forest of Shadows.
    elseif %actor.has_failed[blur]%
      osend %actor% Find %master% to restart the races.
    elseif !%actor.has_completed[blur]%
      osend %actor% Quest Master: %master%
      osend %actor% &0 
      switch %stage%
        case 1
          osend %actor% Seek out the ranger of the Forest of Shadows.
          break
        case 2
          osend %actor% Seek out and kill the Syric Warder and bring me his sword.
          break
        case 3
          osend %actor% Give %master% the Blade of the Forgotten Kings.
          break
        case 4
          osend %actor% It's time to race the four winds!
          if %actor.quest_variable[blur:race]% != go
            osend %actor% Return to %master% and say "Let's begin" to start the race.
          else
            eval minutes (%timer% / 60)
            eval seconds %timer% - (%minutes% * 60)
            osend %actor% You have %minutes% mins %seconds% secs remaining to race all four winds!
            osend %actor% &0 
            if !%north%
              osend %actor% The North Wind blows near the frozen town of Ickle.
            elseif %north% == 1
              set room %get.room[30262]%
              osend %actor% Beat the North Wind to %room.name% in Bluebonnet Pass.
              eval minutes (%ntimer% / 60)
              eval seconds %ntimer% - (%minutes% * 60)
              osend %actor% You have %minutes% mins %seconds% secs remaining to race the North Wind!
            elseif %north% == 2
              osend %actor% You have won the race against the North Wind!
            endif
            osend %actor% &0 
            if !%south%
              osend %actor% The South Wind blows around the hidden standing stones in South Caelia.
            elseif %south% == 1
              set room %get.room[53557]%
              osend %actor% Beat the South Wind to %room.name%.
              eval minutes (%stimer% / 60)
              eval seconds %stimer% - (%minutes% * 60)
              osend %actor% You have %minutes% mins %seconds% secs remaining to race the South Wind!
            elseif %south% == 2
              osend %actor% You have won the race against the South Wind!
            endif
            osend %actor% &0 
            if !%east%
              osend %actor% The East Wind blows through an enormous volcano in the sea.
            elseif %east% == 1
              set room %get.room[12597]%
              osend %actor% Beat the East Wind to %room.name%.
              eval minutes (%etimer% / 60)
              eval seconds %etimer% - (%minutes% * 60)
              osend %actor% You have %minutes% mins %seconds% secs remaining to race the East Wind!
            elseif %east% == 2
              osend %actor% You have won the race against the East Wind!
            endif
            osend %actor% &0 
            if !%west%
              osend %actor% The West Wind blows through ruins across the vast Gothra plains.
            elseif %west% == 1
              set room %get.room[4236]%
              osend %actor% Beat the West Wind to %room.name%.
              eval minutes (%wtimer% / 60)
              eval seconds %wtimer% - (%minutes% * 60)
              osend %actor% You have %minutes% mins %seconds% secs remaining to race the West Wind!
            elseif %west% == 2
              osend %actor% You have won the race against the West Wind!
            endif
          endif
      done
    endif
  endif
endif
~
#444
Charm Person progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= charm || %arg% /= person || %arg% /= charm_person
  if %actor.level% >= 85
    if %actor.class% /= Sorcerer || %actor.class% /= Bard || %actor.class% /= Illusionist
      return 0
      set master %get.mob_shortdesc[58038]%
      set stage %actor.quest_stage[charm_person]%
      osend %actor% &2&b&uCharm Person&0
      osend %actor% Minimum Level: 89
      if %actor.has_completed[charm_person]%
        set status Completed!
      elseif %stage%
        set status In Progress
      else
        set status Not Started
      endif
      osend %actor% &6Status: %status%&0&_
      if %stage% > 0 && !%actor.has_completed[charm_person]%
        osend %actor% Quest Master: %master%
        osend %actor% &0 
        switch %stage%
          case 1
            osend %actor% You must find the rod that casts Charm Person in the crypt in the Iron Hills.
            break
          case 2
            osend %actor% Help the theatre company in Anduin perform their grand finale and bring back the unique fire ring they give out afterward.
            osend %actor% &0
            osend %actor% They have a number of problems which you will need to work out before you can seek out their replacement "Pippin" and lure him to his fiery grave.
            break
          case 3
            set item1 %actor.quest_variable[charm_person:58017]%
            set item2 %actor.quest_variable[charm_person:16312]%
            set item3 %actor.quest_variable[charm_person:48925]%
            set item4 %actor.quest_variable[charm_person:37012]%
            set item5 %actor.quest_variable[charm_person:41119]%
            osend %actor% Locate five musical instruments and bring them to %master%.
            if %item1% || %item2% || %item3% || %item4% || %item5%
              osend %actor% &0 
              osend %actor% You have already brought:
              if %item1%
                osend %actor% - &5%get.obj_shortdesc[58017]%&0
              endif
              if %item2%
                osend %actor% - &5%get.obj_shortdesc[16312]%&0
              endif
              if %item3%
                osend %actor% - &5%get.obj_shortdesc[48925]%&0
              endif
              if %item4%
                osend %actor% - &5%get.obj_shortdesc[37012]%&0
              endif
              if %item5%
                osend %actor% - &5%get.obj_shortdesc[41119]%&0
              endif
            endif
            osend %actor% &0 
            osend %actor% You still need to find:
            if !%item1%
              osend %actor% - &5&b%get.obj_shortdesc[58017]%&0
            endif
            if !%item2%
              osend %actor% - &5&b%get.obj_shortdesc[16312]%&0
            endif
            if !%item3%
              osend %actor% - &5&b%get.obj_shortdesc[48925]%&0
            endif
            if !%item4%
              osend %actor% - &5&b%get.obj_shortdesc[37012]%&0
            endif
            if !%item5%
              osend %actor% - &5&b%get.obj_shortdesc[41119]%&0
            endif
            break
          case 4
            set charm1 %actor.quest_variable[charm_person:charm1]%
            set charm2 %actor.quest_variable[charm_person:charm2]%
            set charm3 %actor.quest_variable[charm_person:charm3]%
            set charm4 %actor.quest_variable[charm_person:charm4]%
            set charm5 %actor.quest_variable[charm_person:charm5]%
            osend %actor% You must charm the five master charmers.  Ask them &7&b[Let me serenade you]&0.
            if %charm1% || %charm2% || %charm3% || %charm4% || %charm5%
              osend %actor% &0 
              osend %actor% You have already charmed:
              if %charm1%
                osend %actor% - &5%get.mob_shortdesc[3010]%&0
              endif
              if %charm2%
                osend %actor% - &5%get.mob_shortdesc[58017]%&0
              endif
              if %charm3%
                osend %actor% - &5%get.mob_shortdesc[4353]%&0
              endif
              if %charm4%
                osend %actor% - &5%get.mob_shortdesc[23721]%&0
              endif
              if %charm5%
                osend %actor% - &5%get.mob_shortdesc[58406]%&0
              endif
            endif
            osend %actor% &0 
            osend %actor% You still need to find:
            if !%charm1%
              osend %actor% - &5&b%get.mob_shortdesc[3010]%&0
            endif
            if !%charm2%
              osend %actor% - &5&b%get.mob_shortdesc[58017]%&0
            endif
            if !%charm3%
              osend %actor% - &5&b%get.mob_shortdesc[4353]%&0
            endif
            if !%charm4%
              osend %actor% - &5&b%get.mob_shortdesc[23721]%&0
            endif
            if !%charm5%
              osend %actor% - &5&b%get.mob_shortdesc[58406]%&0
            endif
        done
      endif
    endif
  endif
endif
~
#445
Hell Gate progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= hell gate || %arg% /= hell_gate
  if %actor.class% /= Diabolist && %actor.level% >= 75
    return 0
    set stage %actor.quest_stage[hell_gate]%
    set master %get.mob_shortdesc[56400]%
    osend %actor% &2&b&uHell Gate&0
    osend %actor% Minimum Level: 81
    if %actor.has_completed[hell_gate]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[hell_gate]%
      osend %actor% Quest Master: %master%
      osend %actor% &0 
      switch %stage%
        case 1
          osend %actor% You are preparing to open a door to Garl'lixxil and release one of the demon lords.  Find %get.obj_shortdesc[3213]%.
          break 
        case 2
          set key1 %actor.quest_variable[hell_gate:8303]%
          set key2 %actor.quest_variable[hell_gate:23709]%
          set key3 %actor.quest_variable[hell_gate:49008]%
          set key4 %actor.quest_variable[hell_gate:52012]%
          set key5 %actor.quest_variable[hell_gate:52013]%
          set key6 %actor.quest_variable[hell_gate:53402]%
          set key7 %actor.quest_variable[hell_gate:58109]%
          osend %actor% Find seven keys to seven gates.
          if %key1% || %key2% || %key3% || %key4% || %key5% || %key6% || %key7%
            osend %actor% &0 
            osend %actor% You have already found:
            if %key1%
              osend %actor% &1%get.obj_shortdesc[8303]%&0
            endif
            if %key2%
              osend %actor% &1%get.obj_shortdesc[23709]%&0
            endif
            if %key3%
              osend %actor% &1%get.obj_shortdesc[49008]%&0
            endif
            if %key4%
              osend %actor% &1%get.obj_shortdesc[52012]%&0
            endif
            if %key5%
              osend %actor% &1%get.obj_shortdesc[52013]%&0
            endif
            if %key6%
              osend %actor% &1%get.obj_shortdesc[53402]%&0
            endif
            if %key7%
              osend %actor% &1%get.obj_shortdesc[58109]%&0
            endif
          endif
          osend %actor% &0 
          osend %actor% You must still find:
          if !%key1%
            osend %actor% &1&bA small, well-crafted key made of wood with the smell of rich sap&0
            osend %actor% &1&bKept at the gate of a tribe's home.&0
            osend %actor% &0
          endif
          if !%key6%
            osend %actor% &1&bA key made of light silvery metal which only elves can work&0
            osend %actor% &1&bDeep in a frozen valley.&0
            osend %actor% &0
          endif
          if !%key2%
            osend %actor% &1&bA large, black key humming with magical energy&0
            osend %actor% &1&bFrom a twisted cruel city in a huge underground cavern.&0
            osend %actor% &0 
          endif
          if !%key7%
            osend %actor% &1&bA simple lacquered iron key&0
            osend %actor% &1&bIn the care of a radiant bird on an emerald island.&0
            osend %actor% &0 
          endif
          if !%key3%
            osend %actor% &1&bA rusted but well cared for key&0
            osend %actor% &1&bHeld by a winged captain on an island of magical beasts.&0
            osend %actor% &0 
          endif
          if !%key5%
            osend %actor% &1&bA golden plated, wrought-iron key&0
            osend %actor% &1&bheld at the gates to a desacrated city.&0
            osend %actor% &0 
          endif
          if !%key4%
            osend %actor% &1&bOne nearly impossible to see&0
            osend %actor% &1&bguarded by a fiery beast with many heads.&0
          endif
          break
        case 3
          set blood1 %actor.quest_variable[hell_gate:56400]%
          set blood2 %actor.quest_variable[hell_gate:56401]%
          set blood3 %actor.quest_variable[hell_gate:56402]%
          set blood4 %actor.quest_variable[hell_gate:56403]%
          set blood5 %actor.quest_variable[hell_gate:56404]%
          set blood6 %actor.quest_variable[hell_gate:56405]%
          set blood7 %actor.quest_variable[hell_gate:56406]%
          osend %actor% Sacrifice seven different &1&bchildren&0.
          osend %actor% &0&7&b[Drop]&0 their &1&bblood&0 before %master% to defile the keys.
          if %blood1% || %blood2% || %blood3% || %blood4% || %blood5% || %blood6% || %blood7%
            osend %actor% &0 
            osend %actor% You have already found:
            if %blood1%
              osend %actor% &1%get.obj_shortdesc[56400]%&0
            endif
            if %blood2%
              osend %actor% &1%get.obj_shortdesc[56401]%&0
            endif
            if %blood3%
              osend %actor% &1%get.obj_shortdesc[56402]%&0
            endif
            if %blood4%
              osend %actor% &1%get.obj_shortdesc[56403]%&0
            endif
            if %blood5%
              osend %actor% &1%get.obj_shortdesc[56404]%&0
            endif
            if %blood6%
              osend %actor% &1%get.obj_shortdesc[56405]%&0
            endif
            if %blood7%
              osend %actor% &1%get.obj_shortdesc[56406]%&0
            endif
          endif
          osend %actor% &0 
          eval total (7 - (%blood1% + %blood2% + %blood3% + %blood4% + %blood5% + %blood6% + %blood7%))
          if %total% == 1
            osend %actor% Sacrifice the last child!
          else
            osend %actor% Bring the blood of &1%total%&0 more children.
          endif
          osend %actor% &0
          osend %actor% If you need a new dagger, return to %master% and say, &1&b"I need a new dagger"&0.
          break
        case 4
          osend %actor% Give the spider-shaped dagger back to %master%.
          break
        case 5
          osend %actor% Slay Larathiel and release the demon lord!
      done
    endif
  endif
endif
~
#446
Hellfire and Brimstone progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= hellfire || %arg% /= brimstone || %arg% /= hellfire_and_brimstone || %arg% /= hellfire_brimstone
  if %actor.class% /= Diabolist && %actor.level% >= 50
    return 0
    set master %get.mob_shortdesc[2311]%
    set stage %actor.quest_stage[hellfire_brimstone]%
    osend %actor% &2&b&uHellfire and Brimstone&0
    osend %actor% Minimum Level: 57
    if %actor.has_completed[hellfire_brimstone]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[hellfire_brimstone]%
      osend %actor% Quest Master: %master%
      osend %actor% &0 
      switch %stage%
        case 1
          set meat %actor.quest_variable[hellfire_brimstone:meat]%
          osend %actor% %master% needs &1&bmeat&0 for the fire.
          eval total (6 - %meat%)
          if %total% == 1
            osend %actor% Drop &1&b%total%&0 more pound of flesh from the paladins at the Sacred Haven into the bonfire.
          else
            osend %actor% Drop &1&b%total%&0 pounds of flesh from the paladins at the Sacred Haven into the bonfire.
          endif
          break
        case 2
          set brimstone %actor.quest_variable[hellfire_brimstone:brimstone]%
          osend %actor% %master% needs &3&bbrimstone&0 to trace out the sigils.
          osend %actor%   
          eval total (6 - %brimstone%)
          if %total% == 1
            osend %actor% Bring &3&b%total%&0 more quantity of brimstone from fiery spirits on the volcanic island to the north and drop them into the bonfire.
          else
            osend %actor% Bring &3&b%total%&0 quantities of brimstone from fiery spirits on the volcanic island to the north and drop it into the bonfire.
          endif
          break
        case 3
          set item1 %actor.quest_variable[hellfire_brimstone:4318]%
          set item2 %actor.quest_variable[hellfire_brimstone:5211]%
          set item3 %actor.quest_variable[hellfire_brimstone:5212]%
          set item4 %actor.quest_variable[hellfire_brimstone:17308]%
          set item5 %actor.quest_variable[hellfire_brimstone:48110]%
          set item6 %actor.quest_variable[hellfire_brimstone:53000]%
          osend %actor% You are to bring %master% fiery tributes to the Dark One.
          osend %actor% &0 
          if %item1% || %item2% || %item3% || %item4% || %item5% || %item6%
            osend %actor% You have already given him:
            if %item1%
              osend %actor% - %get.obj_shortdesc[4318]%
            endif
            if %item2%
              osend %actor% - %get.obj_shortdesc[5211]%
            endif
            if %item3%
              osend %actor% - %get.obj_shortdesc[5212]%
            endif
            if %item4%
              osend %actor% - %get.obj_shortdesc[17308]%
            endif
            if %item5%
              osend %actor% - %get.obj_shortdesc[48110]%
            endif
            if %item6%
              osend %actor% - %get.obj_shortdesc[53000]%
            endif
          endif
          osend %actor% &0 
          osend %actor% Now bring him:
          if !%item1%
            osend %actor% - %get.obj_shortdesc[4318]%&0 from an actress in Anduin.&0
          endif
          if !%item2%
            osend %actor% - &7&b%get.obj_shortdesc[5211]%&0 from a beam of starlight deep in mine.&0
          endif
          if !%item3%
            osend %actor% - &b%get.obj_shortdesc[5212]%&0 from a devotee of neutrality on a hill.&0
          endif
          if !%item4%
            osend %actor% - &9&b%get.obj_shortdesc[17308]% from Chaos incarnate.&0
          endif
          if !%item5%
            osend %actor% - &1&b%get.obj_shortdesc[48110]%&0 from the volcano goddess.&0
          endif
          if !%item6%
            osend %actor% - %get.obj_shortdesc[53000]%&0 from a king in a throne room crypt.&0
          endif
      done
    endif
  endif
endif
~
#447
Illusory Wall progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= illusory || %arg% /= illusory wall || %arg% /= illusory_wall
  if (%actor.class% /= Illusionist || %actor.class% /= Bard) && %actor.level% >= 50
    return 0
    set stage %actor.quest_stage[illusory_wall]%
    set master %get.mob_shortdesc[36402]%
    osend %actor% &2&b&uIllusory Wall&0
    osend %actor% Minimum Level: 57
    if %actor.has_completed[illusory_wall]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[illusory_wall]%
      osend %actor% Quest Master: %master%
      switch %stage%
        case 1
          set item1 %actor.quest_variable[illusory_wall:10307]%
          set item2 %actor.quest_variable[illusory_wall:18511]%
          set item3 %actor.quest_variable[illusory_wall:41005]%
          set item4 %actor.quest_variable[illusory_wall:51017]%
          osend %actor% You're looking for things to make magical spectacles.
          if (%item1% || %item2% || %item3% || %item4%)
            osend %actor% &0 
            osend %actor% &0You have already brought me:
            if %item1%
              osend %actor% - &7&b%get.obj_shortdesc[10307]%&0
            endif
            if %item2%
              osend %actor% - &7&b%get.obj_shortdesc[18511]%&0
            endif
            if %item3%
              osend %actor% - &7&b%get.obj_shortdesc[41005]%&0
            endif
            if %item4%
              osend %actor% - &7&b%get.obj_shortdesc[51017]%&0
            endif
          endif
          osend %actor% &0 
          osend %actor% You still need to find:
          if (!%item1% && !%item2%)
            osend %actor% - &3&b%get.obj_shortdesc[10307]%&0 or &3&b%get.obj_shortdesc[18511]%&0
          endif        
          if !%item3%
            osend %actor% - &3&b%get.obj_shortdesc[41005]%&0
          endif
          if !%item4%
            osend %actor% - &3&b%get.obj_shortdesc[51017]%&0
          endif
          break
        case 2
          osend %actor% Complete your study of doors in 20 regions.
          osend %actor% &0 
          set doors %actor.quest_variable[illusory_wall:total]%
          osend %actor% You have examined doors in &5&b%doors%&0 regions:
          if %actor.quest_variable[illusory_wall:Outback]%
            osend %actor% - &bRocky Outback&0
          endif
          if %actor.quest_variable[illusory_wall:Shadows]%
            osend %actor% - &bForest of Shadows&0
          endif
          if %actor.quest_variable[illusory_wall:Merchant]%
            osend %actor% - &bOld Merchant Trail&0
          endif
          if %actor.quest_variable[illusory_wall:Caelia_West]%
            osend %actor% - &bSouth Caelia West&0
          endif
          if %actor.quest_variable[illusory_wall:River]%
            osend %actor% - &bBlue-Fog River&0
          endif
          if %actor.quest_variable[illusory_wall:Mielikki]%
            osend %actor% - &bThe Village of Mielikki&0
          endif
          if %actor.quest_variable[illusory_wall:Mielikki_Forest]%
            osend %actor% - &bMielikki's Forests&0
          endif
          if %actor.quest_variable[illusory_wall:Labyrinth]%
            osend %actor% - &bLaveryn Labyrinth&0
          endif
          if %actor.quest_variable[illusory_wall:Split]%
            osend %actor% - &bSplit Skull&0
          endif
          if %actor.quest_variable[illusory_wall:Theatre]%
            osend %actor% - &bThe Theatre in Anduin&0
          endif
          if %actor.quest_variable[illusory_wall:Rocky_Tunnels]%
            osend %actor% - &bRocky Tunnels&0
          endif
          if %actor.quest_variable[illusory_wall:Lava]%
            osend %actor% - &bLava Tunnels&0
          endif
          if %actor.quest_variable[illusory_wall:Misty]%
            osend %actor% - &bMisty Caverns&0
          endif
          if %actor.quest_variable[illusory_wall:Combat]%
            osend %actor% - &bCombat in Eldoria&0
          endif
          if %actor.quest_variable[illusory_wall:Anduin]%
            osend %actor% - &bThe City of Anduin&0
          endif
          if %actor.quest_variable[illusory_wall:Pastures]%
            osend %actor% - &bAnduin Pastures&0
          endif
          if %actor.quest_variable[illusory_wall:Great_Road]%
            osend %actor% - &bThe Great Road&0
          endif
          if %actor.quest_variable[illusory_wall:Nswamps]%
            osend %actor% - &bThe Northern Swamps&0
          endif
          if %actor.quest_variable[illusory_wall:Farmlands]%
            osend %actor% - &bMielikki Farmlands&0
          endif
          if %actor.quest_variable[illusory_wall:Frakati]%
            osend %actor% - &bFrakati Reservation&0
          endif
          if %actor.quest_variable[illusory_wall:Cathedral]%
            osend %actor% - &bCathedral of Betrayal&0
          endif
          if %actor.quest_variable[illusory_wall:Meercats]%
            osend %actor% - &bKingdom of the Meer Cats&0
          endif
          if %actor.quest_variable[illusory_wall:Logging]%
            osend %actor% - &bThe Logging Camp&0
          endif
          if %actor.quest_variable[illusory_wall:Dairy]%
            osend %actor% - &bThe Dairy Farm&0
          endif
          if %actor.quest_variable[illusory_wall:Ickle]%
            osend %actor% - &bIckle&0
          endif
          if %actor.quest_variable[illusory_wall:Frostbite]%
            osend %actor% - &bMount Frostbite&0
          endif
          if %actor.quest_variable[illusory_wall:Phoenix]%
            osend %actor% - &bPhoenix Feather Hot Spring&0
          endif
          if %actor.quest_variable[illusory_wall:Blue_Fog_Trail]%
            osend %actor% - &bBlue-Fog Trail&0
          endif
          if %actor.quest_variable[illusory_wall:Twisted]%
            osend %actor% - &bTwisted Forest&0
          endif
          if %actor.quest_variable[illusory_wall:Megalith]%
            osend %actor% - &bThe Sacred Megalith&0
          endif
          if %actor.quest_variable[illusory_wall:Tower]%
            osend %actor% - &bThe Tower in the Wasted&0
          endif
          if %actor.quest_variable[illusory_wall:Miner]%
            osend %actor% - &bThe Miner's Cavern&0
          endif
          if %actor.quest_variable[illusory_wall:Morgan]%
            osend %actor% - &bMorgan Hill&0
          endif
          if %actor.quest_variable[illusory_wall:Mystwatch]%
            osend %actor% - &bThe Fortress of Mystwatch&0
          endif
          if %actor.quest_variable[illusory_wall:Desert]%
            osend %actor% - &bGothra Desert&0
          endif
          if %actor.quest_variable[illusory_wall:Pyramid]%
            osend %actor% - &bGothra Pyramid&0
          endif
          if %actor.quest_variable[illusory_wall:Highlands]%
            osend %actor% - &bHighlands&0
          endif
          if %actor.quest_variable[illusory_wall:Haunted]%
            osend %actor% - &bThe Haunted House&0
          endif
          if %actor.quest_variable[illusory_wall:Citadel]%
            osend %actor% - &bThe Citadel of Testing&0
          endif
          if %actor.quest_variable[illusory_wall:Chaos]%
            osend %actor% - &bTemple of Chaos&0
          endif
          if %actor.quest_variable[illusory_wall:Canyon]%
            osend %actor% - &bCanyon&0
          endif
          if %actor.quest_variable[illusory_wall:Topiary]%
            osend %actor% - &bMielikki's Topiary&0
          endif
          if %actor.quest_variable[illusory_wall:Abbey]%
            osend %actor% - &bThe Abbey&0
          endif
          if %actor.quest_variable[illusory_wall:Plains]%
            osend %actor% - &bGothra Plains&0
          endif
          if %actor.quest_variable[illusory_wall:Dheduu]%
            osend %actor% - &bDheduu&0
          endif
          if %actor.quest_variable[illusory_wall:Dargentan]%
            osend %actor% - &bDargentan's Lair&0
          endif
          if %actor.quest_variable[illusory_wall:Ogakh]%
            osend %actor% - &bOgakh&0
          endif
          if %actor.quest_variable[illusory_wall:Bluebonnet]%
            osend %actor% - &bBluebonnet Pass&0
          endif
          if %actor.quest_variable[illusory_wall:Caelia_East]%
            osend %actor% - &bSouth Caelia East&0
          endif
          if %actor.quest_variable[illusory_wall:Brush]%
            osend %actor% - &bBrush Lands&0
          endif
          if %actor.quest_variable[illusory_wall:Kaaz]%
            osend %actor% - &bTemple of the Kaaz&0
          endif
          if %actor.quest_variable[illusory_wall:SeaWitch]%
            osend %actor% - &bSea's Lullaby&0
          endif
          if %actor.quest_variable[illusory_wall:Smuggler]%
            osend %actor% - &bSmuggler's Hideout&0
          endif
          if %actor.quest_variable[illusory_wall:Sirestis]%
            osend %actor% - &bSirestis' Folly&0
          endif
          if %actor.quest_variable[illusory_wall:Ancient_Ruins]%
            osend %actor% - &bAncient Ruins&0
          endif
          if %actor.quest_variable[illusory_wall:Minithawkin]%
            osend %actor% - &bMinithawkin Mines&0
          endif
          if %actor.quest_variable[illusory_wall:Arabel]%
            osend %actor% - &bArabel Ocean&0
          endif
          if %actor.quest_variable[illusory_wall:Hive]%
            osend %actor% - &bHive&0
          endif
          if %actor.quest_variable[illusory_wall:Demise]%
            osend %actor% - &bDemise Keep&0
          endif
          if %actor.quest_variable[illusory_wall:Aviary]%
            osend %actor% - &bIckle's Aviary&0
          endif
          if %actor.quest_variable[illusory_wall:Graveyard]%
            osend %actor% - &bThe Graveyard&0
          endif
          if %actor.quest_variable[illusory_wall:Earth]%
            osend %actor% - &bThe Plane of Earth&0
          endif
          if %actor.quest_variable[illusory_wall:Water]%
            osend %actor% - &bThe Plane of Water&0
          endif
          if %actor.quest_variable[illusory_wall:Fire]%
            osend %actor% - &bThe Plane of Fire&0
          endif
          if %actor.quest_variable[illusory_wall:Barrow]%
            osend %actor% - &bThe Barrow&0
          endif
          if %actor.quest_variable[illusory_wall:Fiery]%
            osend %actor% - &bFiery Island&0
          endif
          if %actor.quest_variable[illusory_wall:Nukreth]%
            osend %actor% - &bNukreth Spire&0
          endif
          if %actor.quest_variable[illusory_wall:Doom]%
            osend %actor% - &bAn Ancient Forest and Pyramid&0
          endif
          if %actor.quest_variable[illusory_wall:Air]%
            osend %actor% - &bThe Plane of Air&0
          endif
          if %actor.quest_variable[illusory_wall:Lokari]%
            osend %actor% - &bLokari's Keep&0
          endif
          if %actor.quest_variable[illusory_wall:Griffin]%
            osend %actor% - &bGriffin Island&0
          endif
          if %actor.quest_variable[illusory_wall:BlackIce]%
            osend %actor% - &bBlack-Ice Desert&0
          endif
          if %actor.quest_variable[illusory_wall:Nymrill]%
            osend %actor% - &bThe Lost City of Nymrill&0
          endif
          if %actor.quest_variable[illusory_wall:Bayou]%
            osend %actor% - &bThe Bayou&0
          endif
          if %actor.quest_variable[illusory_wall:Nordus]%
            osend %actor% - &bThe Enchanted Village of Nordus&0
          endif
          if %actor.quest_variable[illusory_wall:Templace]%
            osend %actor% - &bTemplace&0
          endif
          if %actor.quest_variable[illusory_wall:Sunken]%
            osend %actor% - &bSunken Castle&0
          endif
          if %actor.quest_variable[illusory_wall:Cult]%
            osend %actor% - &bIce Cult&0
          endif
          if %actor.quest_variable[illusory_wall:Frost]%
            osend %actor% - &bFrost Valley&0
          endif
          if %actor.quest_variable[illusory_wall:Technitzitlan]%
            osend %actor% - &bTechnitzitlan&0
          endif
          if %actor.quest_variable[illusory_wall:Black_Woods]%
            osend %actor% - &bBlack Woods&0
          endif
          if %actor.quest_variable[illusory_wall:Kaas_Plains]%
            osend %actor% - &bKaas Plains&0
          endif
          if %actor.quest_variable[illusory_wall:Dark_Mountains]%
            osend %actor% - &bDark Mountains&0
          endif
          if %actor.quest_variable[illusory_wall:Cold_Fields]%
            osend %actor% - &bCold Fields&0
          endif
          if %actor.quest_variable[illusory_wall:Iron]%
            osend %actor% - &bIron Hills&0
          endif
          if %actor.quest_variable[illusory_wall:Blackrock]%
            osend %actor% - &bBlack Rock Trail&0
          endif
          if %actor.quest_variable[illusory_wall:Eldorian]%
            osend %actor% - &bEldorian Foothills&0
          endif
          if %actor.quest_variable[illusory_wall:Blacklake]%
            osend %actor% - &bBlack Lake&0
          endif
          if %actor.quest_variable[illusory_wall:Odz]%
            osend %actor% - &bOdaishyozen&0
          endif
          if %actor.quest_variable[illusory_wall:Syric]%
            osend %actor% - &bSyric Mountain Trail&0
          endif
          if %actor.quest_variable[illusory_wall:KoD]%
            osend %actor% - &bKingdom of Dreams&0
          endif
          if %actor.quest_variable[illusory_wall:Beachhead]%
            osend %actor% - &bBeachhead&0
          endif
          if %actor.quest_variable[illusory_wall:Ice_Warrior]%
            osend %actor% - &bThe Ice Warrior's Compound&0
          endif
          if %actor.quest_variable[illusory_wall:Haven]%
            osend %actor% - &bSacred Haven&0
          endif
          if %actor.quest_variable[illusory_wall:Hollow]%
            osend %actor% - &bEnchanted Hollow&0
          endif
          if %actor.quest_variable[illusory_wall:Rhell]%
            osend %actor% - &bThe Rhell Forest&0
          endif
          osend %actor% &0 
          eval remaining (20 - %doors%)
          osend %actor% Locate doors in &5&b%remaining%&0 more regions.
          osend %actor% &0 
          osend %actor% If you need new lenses return to %master% and say, &5&b"I need new glasses"&0.
      done
    endif
  endif
endif
~
#448
The Finale progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= pippin || %arg% /= theatre || %arg% /= theater || %arg% /= finale
  return 0
  set stage %actor.quest_stage[theatre]%
  osend %actor% &2&b&uThe Finale&0
  osend %actor% Defeat wild monkeys, recover keys, and help the theatre troupe perform their fiery Finale!
  osend %actor% Recommended Level: 10
  if %stage%
    osend %actor% &6Status: Repeatable&0
    switch %stage%
      case 1
        set master %get.mob_shortdesc[4351]%
        break
      case 2
        set master %get.mob_shortdesc[4345]%
        break
      case 3
        set master %get.mob_shortdesc[4353]%
        break
      case 4
        set master %get.mob_shortdesc[4352]%
        break
      case 5
        set master %get.mob_shortdesc[4311]%
        break
      case 6
        set master %get.mob_shortdesc[4312]%
        break
      case 7
        set master %get.mob_shortdesc[4399]%
    done
    osend %actor% Quest Master: %master%
    osend %actor% &0 
    switch %stage%
      case 1
        osend %actor% Get back Catherine's dressing room key from the ceiling monkeys, then bring her her eyelashes from her dressing room.
        if !%actor.quest_variable[theatre:lashes]
          osend %actor% Give Catherine her eyelashes.
        elseif %actor.quest_variable[theatre:lashes] == 1
          osend %actor% Give Catherine her dressing room key.
        endif
        break
      case 2
        osend %actor% Give Lewis his dressing room key.
        break
      case 3
        osend %actor% Give Fastrada her dressing room key.
        break
      case 4
        set item1 4320
        set item2 4301
        osend %actor% Give Charlemagne his dressing room key and his missing sash.
        if %actor.quest_variable[theatre:sash]% || %actor.quest_variable[theatre:charles]%
          osend %actor% &0
          osend %actor% You have brought him:
          if %actor.quest_variable[theatre:sash]%
            osend %actor% %get.obj_shortdesc[%item1%]%
          endif
          if %actor.quest_variable[theatre:charles]%
            osend %actor% %get.obj_shortdesc[%item2%]%
          endif
        endif
        osend %actor% &0
        osend %actor% You still need:
        if !%actor.quest_variable[theatre:sash]%
          osend %actor% %get.obj_shortdesc[%item1%]%
        endif
        if !%actor.quest_variable[theatre:charles]%
          osend %actor% %get.obj_shortdesc[%item2%]%
        endif
        break
      case 5
        osend %actor% Give the Fire Goddess her skirt.
        break
      case 6
        osend %actor% The Fire Goddess told you:
        osend %actor% We need to find our Pippin in order to perform the grand Finale.  He's somewhere out in the world, trying to find his corner of the sky.
        osend %actor% &0 
        osend %actor% I want you to bring him to us.
        osend %actor% &0
        osend %actor% Take %get.obj_shortdesc[4318]%.
        osend %actor% &0 
        osend %actor% Hold it in your hand when you find him.  He won't be able to resist the beauty of one perfect flame.
        osend %actor% &0 
        osend %actor% Now, when you get him back here, &6&b[order]&0 him to &6&b[enter]&0 the Fire Box.  We've prepared and hidden it upstage center just for him.
        osend %actor% &0 
        osend %actor% &1&bBe careful not to get inside it yourself!&0
        osend %actor% &0
        osend %actor% It's only for an extraordinary person like Pippin.
        break
      case 7
        osend %actor% Lead Pippin back to the theatre and order him to get in the Fire Box.
    done
  else
    osend %actor% &6Status: Not Started&0
  endif
endif
~
#449
Air Wand progress journal~
1 m 100
journal quest quest-journal~
if (%arg% /= air && (%arg% /= wand || %arg% /= wands || %arg% /= staff || %arg% /= staves)) || %arg% /= air_wand || %arg% /= air_wands || %arg% /= air_staff
  set sorcererclasses Sorcerer Illusionist Cryomancer Pyromancer Necromancer
  if %sorcererclasses% /= %actor.class%
    return 0
    set stage %actor.quest_stage[air_wand]%
    eval minlevel (%stage% - 1) * 10
    if %minlevel% < 1
      set minlevel 1
    endif
    osend %actor% &2&b&uAir Wand&0
    osend %actor% Masters of air will help you create and upgrade a new mystic weapon.
    if !%actor.has_completed[air_wand]%
      osend %actor% Minimum Level: %minlevel%
    endif
    if %actor.has_completed[air_wand]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[air_wand]%
      set job1 %actor.quest_variable[air_wand:wandtask1]%
      set job2 %actor.quest_variable[air_wand:wandtask2]%
      set job3 %actor.quest_variable[air_wand:wandtask3]%
      set job4 %actor.quest_variable[air_wand:wandtask4]%
      set job5 %actor.quest_variable[air_wand:wandtask5]%
      eval attack (%stage% - 1) * 50  
      if %stage% < 8
        set weapon wand
      else
        set weapon staff
      endif
      eval remaining ((%attack%) - %actor.quest_variable[air_wand:attack_counter]%)
      switch %stage%
        case 2
          set master %get.mob_shortdesc[3013]%
          set wandgem 55577
          break
        case 3
          set master %get.mob_shortdesc[18500]%
          set hint Speak with the old Abbot in the Abbey of St. George.
          set wandgem 55591
          set wandtask3 23750
          break
        case 4
          set master %get.mob_shortdesc[58601]%
          set hint The keeper of a southern coastal tower will have advice for you.
          set wandgem 55605
          set wandtask3 2330
          set wandtask4 37006
          break
        case 5
          set master %get.mob_shortdesc[12305]%
          set hint A master of air near the megalith in South Caelia will be able to help next.
          set wandgem 55644
          set wandtask3 12509
          set wandtask4 &7&bthe icy ledge outside Technitzitlan&0
          break      
        case 6
          set master %get.mob_shortdesc[12302]%
          set wandgem 55665
          set hint Seek out the warrior-witch at the center of the southern megalith.
          set wandtask3 23800
          set wandtask4 59040
          break      
        case 7
          set master %get.mob_shortdesc[49003]%
          set wandgem 55682
          set hint She's hard to deal with, but the Seer of Griffin Isle should have some additional guidance for you.
          set wandtask3 51014
          set wandtask4 23710
          break      
        case 8
          set master %get.mob_shortdesc[8515]%
          set hint In the diabolist's church is a seer who cannot see.  He's a good resource for this kind of work.
          set wandgem 55721
          set wandtask3 11799
          set wandtask4 53454
          break      
        case 9
          set master %get.mob_shortdesc[6216]%
          set hint The guardian ranger of the Druid Guild in the Red City has some helpful crafting tips.
          set wandgem 55742
          set wandtask3 49019
          set wandtask4 23803
          break
        case 10
          set master %get.mob_shortdesc[18581]%
          set hint Silania will help you craft the finest of air weapons.
          set wandgem 11811
          set wandtask3 52001
          set wandtask4 48862
      done
      if %stage% == 1
        osend %actor% Find %get.mob_shortdesc[3013]% and show him your wand.
        halt
      else
        if %actor.level% >= %minlevel%
          if %actor.quest_variable[air_wand:greet]% == 0 && %stage% != 2
            osend %actor% Find the next master crafter and tell them why you have come.
            osend %actor% %hint%
            halt
          else
            osend %actor% Quest Master: %master%
            osend %actor% &0
            if %job1% || %job2% || %job3% || %job4%
              osend %actor% You've done the following:
              if %job1%
                osend %actor% - used your %weapon% %attack% times
              endif
              if %job2%
                osend %actor% - found %get.obj_shortdesc[%wandgem%]%
              endif
              if %job3%
                if %stage% != 10
                  osend %actor% - found %get.obj_shortdesc[%wandtask3%]%
                else
                  osend %actor% - slayed %get.mob_shortdesc[%wandtask3%]%
                endif
              endif
              if %job4%
                if %stage% != 5 && %stage% != 9 && %stage% != 10
                  osend %actor% - found %get.obj_shortdesc[%wandtask4%]%
                elseif %stage% == 5
                  osend %actor% - communed in %wandtask4%
                elseif %stage% == 9
                  osend %actor% - slayed %get.mob_shortdesc[%wandtask4%]%
                endif
              endif
              osend %actor% &0 
            endif
            osend %actor% You need to:
            if %job1% && %job2%
              if %stage% == 2
                osend %actor% bring %master% your %weapon%.
              else
                if %job3% 
                  if %stage% == 3
                    osend %actor% bring %master% your %weapon%.
                  else
                    if %job4%
                      if %stage% != 6 && %stage% != 8 && %stage% !=10
                        osend %actor% Bring %master% your %weapon%.
                      elseif %stage% == 6
                        if !%job5%
                          osend %actor% bring %master% your %weapon%.
                        else
                          osend %actor% - &6&bplay&0 %get.obj_shortdesc[%wandtask4%]%
                        endif
                      elseif %stage% == 8
                        if !%job5%
                          osend %actor% Bring %master% your %weapon%.
                        else
                          set room %get.room[%place%]%
                          osend %actor% - imbue your %weapon% at %room.name%.
                        endif
                      elseif %stage% == 10
                        set room %get.room[%wandtask4%]%
                        osend %actor% - imbue your %weapon% at %room.name%.
                      endif
                    endif
                  endif
                endif
              endif
            endif
            if !%job1%
              if %remaining% > 1
                osend %actor% - &3&battack %remaining% more times with your %weapon%&0
              else 
                osend %actor% - &3&battack %remaining% more time with your %weapon%&0
              endif
            endif
            if !%job2%
              osend %actor% - &3&bfind %get.obj_shortdesc[%wandgem%]%&0
            endif
            if %stage% > 2
              if !%job3%
                if %stage% != 10
                  osend %actor% - &3&bfind %get.obj_shortdesc[%wandtask3%]%&0
                  if %stage% == 4
                    osend %actor% &0    Blessings can be called at the smaller groups of standing stones in South Caelia.
                    osend %actor% &0    Search the far eastern edge of the continent.
                    osend %actor% &0    The phrase to call the blessing is:
                    osend %actor% &0    &7&bI pray for a blessing from mother earth, creator of life and bringer of death&0
                  endif
                else
                  osend %actor% - &3&bslay %get.mob_shortdesc[%wandtask3%]%&0
                endif
              endif
              if !%job4%
                if %stage% != 3 && %stage% != 5 && %stage% != 9 && %stage% != 10
                  osend %actor% - &3&bfind %get.obj_shortdesc[%wandtask4%]%&0
                elseif %stage% == 5
                  osend %actor% - &3&bimbue your %weapon% on %wandtask4%&0
                elseif %stage% == 9
                  osend %actor% - &3&bslay %get.mob_shortdesc[%wandtask4%]%&0
                elseif %stage% == 10
                  if %job1% && %job2% && %job3%
                    osend %actor% Bring %master% your %weapon%.
                  endif
                endif
              endif
            endif
          endif
        endif  
      endif
    endif
  endif
endif
~
#450
Fire Wand progress journal~
1 m 100
journal quest quest-journal~
if (%arg% /= fire && (%arg% /= wand || %arg% /= wands || %arg% /= staff || %arg% /= staves)) || %arg% /= fire_wand || %arg% /= fire_wands || %arg% /= fire_staff
  set sorcererclasses Sorcerer Illusionist Cryomancer Pyromancer Necromancer
  if %sorcererclasses% /= %actor.class%
    return 0
    set stage %actor.quest_stage[fire_wand]%
    eval minlevel (%stage% - 1) * 10
    if %minlevel% < 1
      set minlevel 1
    endif
    osend %actor% &2&b&uFire Wand&0
    osend %actor% Masters of fire will help you create and upgrade a new mystic weapon.
    if !%actor.has_completed[fire_wand]%
      osend %actor% Minimum Level: %minlevel%
    endif
    if %actor.has_completed[fire_wand]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[fire_wand]%
      set job1 %actor.quest_variable[fire_wand:wandtask1]%
      set job2 %actor.quest_variable[fire_wand:wandtask2]%
      set job3 %actor.quest_variable[fire_wand:wandtask3]%
      set job4 %actor.quest_variable[fire_wand:wandtask4]%
      set job5 %actor.quest_variable[fire_wand:wandtask5]%
      eval attack (%stage% - 1) * 50  
      if %stage% < 8
        set weapon wand
      else
        set weapon staff
      endif
      eval remaining ((%attack%) - %actor.quest_variable[fire_wand:attack_counter]%)
      switch %stage%
        case 2
          set master %get.mob_shortdesc[3013]%
          set wandgem 55575
          break
        case 3
          set master %get.mob_shortdesc[4126]%
          set hint A minion of the dark flame out east will know what to do.
          set wandgem 55590
          set wandtask3 23752
          set wandvnum 311
          break
        case 4
          set master %get.mob_shortdesc[10306]%
          set hint There's a fire master in the frozen north who likes to spend his time at the hot springs.
          set wandgem 55612
          set wandtask3 2331
          set wandtask4 37006
          break
        case 5
          set master %get.mob_shortdesc[12304]%
          set hint A master of fire near the megalith in South Caelia will be able to help next.
          set wandgem 55639
          set wandtask3 12526
          set wandtask4 &1&bthe Lava Tunnels&0
          break
        case 6
          set master %get.mob_shortdesc[23811]%
          set hint A seraph crafts with the power of the sun and sky.  It can be found in the floating fortress in South Caelia.
          set wandgem 55662
          set wandtask3 5201
          set wandtask4 32412
          break
        case 7
          set master %get.mob_shortdesc[48105]%
          set hint I hate to admit it, but Vulcera is your next crafter.  Good luck appeasing her though...
          set wandgem 55689
          set wandtask3 43018
          set wandtask4 11705
          break
        case 8
          set master %get.mob_shortdesc[48250]%
          set hint You're headed back to Fiery Island.  Crazy old McCabe can help you improve your staff further.
          set wandgem 55716
          set wandtask3 53000
          set wandtask4 53456
          set place 5272
          break
        case 9
          set master %get.mob_shortdesc[48412]%
          set hint Seek out the one who speaks for the Sun near Anduin.  He can upgrade your wand.
          set wandgem 55739
          set wandtask3 48126
          set wandtask4 4013
          break
        case 10
          set master %get.mob_shortdesc[5230]%
          set hint Surely you've heard of Emmath Firehand.  He's the supreme artisan of fiery goods.  He can help you make the final improvements to your staff.
          set wandgem 23822
          set wandtask3 52002
          set wandtask4 47800
          break
      done
      if %stage% == 1
        osend %actor% Find %get.mob_shortdesc[3013]% and show him your wand.
        halt
      else
        if %actor.level% >= %minlevel%
          if %actor.quest_variable[fire_wand:greet]% == 0 && %stage% != 2
            osend %actor% Find the next master crafter and tell them why you have come.
            osend %actor% %hint%
            halt
          else
            osend %actor% Quest Master: %master%
            osend %actor% &0
            if %job1% || %job2% || %job3% || %job4%
              osend %actor% You've done the following:
              if %job1%
                osend %actor% - used your %weapon% %attack% times
              endif
              if %job2%
                osend %actor% - found %get.obj_shortdesc[%wandgem%]%
              endif
              if %job3%
                if %stage% != 10
                  osend %actor% - found %get.obj_shortdesc[%wandtask3%]%
                else
                  osend %actor% - slayed %get.mob_shortdesc[%wandtask3%]%
                endif
              endif
              if %job4%
                if %stage% != 5 && %stage% != 9 && %stage% != 10
                  osend %actor% - found %get.obj_shortdesc[%wandtask4%]%
                elseif %stage% == 5
                  osend %actor% - communed in %wandtask4%
                elseif %stage% == 9
                  osend %actor% - slayed %get.mob_shortdesc[%wandtask4%]%
                endif
              endif
              osend %actor% &0 
            endif
            osend %actor% You need to:
            if %job1% && %job2%
              if %stage% == 2
                osend %actor% bring %master% your %weapon%.
              else
                if %job3% 
                  if %stage% == 3
                    osend %actor% bring %master% your %weapon%.
                  else
                    if %job4%
                      if %stage% != 6 && %stage% != 8 && %stage% !=10
                        osend %actor% Bring %master% your %weapon%.
                      elseif %stage% == 6
                        if !%job5%
                          osend %actor% bring %master% your %weapon%.
                        else
                          osend %actor% - &6&bplay&0 %get.obj_shortdesc[%wandtask4%]%
                        endif
                      elseif %stage% == 8
                        if !%job5%
                          osend %actor% Bring %master% your %weapon%.
                        else
                          set room %get.room[%place%]%
                          osend %actor% - imbue your %weapon% at %room.name%.
                        endif
                      elseif %stage% == 10
                        set room %get.room[%wandtask4%]%
                        osend %actor% - imbue your %weapon% at %room.name%.
                      endif
                    endif
                  endif
                endif
              endif
            endif
            if !%job1%
              if %remaining% > 1
                osend %actor% - &3&battack %remaining% more times with your %weapon%&0
              else 
                osend %actor% - &3&battack %remaining% more time with your %weapon%&0
              endif
            endif
            if !%job2%
              osend %actor% - &3&bfind %get.obj_shortdesc[%wandgem%]%&0
            endif
            if %stage% > 2
              if !%job3%
                if %stage% != 10
                  osend %actor% - &3&bfind %get.obj_shortdesc[%wandtask3%]%&0
                  if %stage% == 4
                    osend %actor% &0    Blessings can be called at the smaller groups of standing stones in South Caelia.
                    osend %actor% &0    Search the south point beyond Anlun Vale.
                    osend %actor% &0    The phrase to call the blessing is:
                    osend %actor% &0    &7&bI pray for a blessing from mother earth, creator of life and bringer of death&0
                  endif
                else
                  osend %actor% - &3&bslay %get.mob_shortdesc[%wandtask3%]%&0
                endif
              endif
              if !%job4%
                if %stage% != 3 && %stage% != 5 && %stage% != 9 && %stage% != 10
                  osend %actor% - &3&bfind %get.obj_shortdesc[%wandtask4%]%&0
                elseif %stage% == 5
                  osend %actor% - &3&bimbue your %weapon% in %wandtask4%&0
                elseif %stage% == 9
                  osend %actor% - &3&bslay %get.mob_shortdesc[%wandtask4%]%&0
                elseif %stage% == 10
                  if %job1% && %job2% && %job3%
                    osend %actor% Bring %master% your %weapon%.
                  endif
                endif
              endif
            endif
          endif
        endif  
      endif
    endif
  endif
endif
~
#451
Ice Wand progress journal~
1 m 100
journal quest quest-journal~
if ((%arg% /= ice || %arg% /= water || %arg% /= cold || %arg% /= frost) && (%arg% /= wand || %arg% /= wands || %arg% /= staff || %arg% /= staves)) || %arg% /= ice_wand || %arg% /= ice_wands || %arg% /= ice_staff || %arg% /= water_wand || %arg% /= water_wands || %arg% /= water_staff
  set sorcererclasses Sorcerer Illusionist Cryomancer Pyromancer Necromancer
  if %sorcererclasses% /= %actor.class%
    return 0
    set stage %actor.quest_stage[ice_wand]%
    eval minlevel (%stage% - 1) * 10
    if %minlevel% < 1
      set minlevel 1
    endif
    osend %actor% &2&b&uIce Wand&0
    osend %actor% Masters of ice and water will help you create and upgrade a new mystic weapon.
    if ! %actor.has_completed[ice_wand]%
      osend %actor% Minimum Level: %minlevel%
    endif
    if %actor.has_completed[ice_wand]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[ice_wand]%
      set job1 %actor.quest_variable[ice_wand:wandtask1]%
      set job2 %actor.quest_variable[ice_wand:wandtask2]%
      set job3 %actor.quest_variable[ice_wand:wandtask3]%
      set job4 %actor.quest_variable[ice_wand:wandtask4]%
      set job5 %actor.quest_variable[ice_wand:wandtask5]%
      eval attack (%stage% - 1) * 50  
      if %stage% < 8
        set weapon wand
      else
        set weapon staff
      endif
      eval remaining ((%attack%) - %actor.quest_variable[ice_wand:attack_counter]%)
      switch %stage%
        case 2
          set master %get.mob_shortdesc[3013]%
          set wandgem 55574
          break
        case 3
          set master %get.mob_shortdesc[17806]%
          set hint The shaman near Three-Falls River has developed a powerful affinity for water from his life in the canyons.  Seek his advice.
          set wandgem 55592
          set wandtask3 23753
          break
        case 4
          set master %get.mob_shortdesc[2337]%
          set hint Many of the best craftspeople aren't even mortal.  There is a water sprite of some renown deep in Anlun Vale.
          set wandgem 55607
          set wandtask3 2333
          set wandtask4 37006
          break
        case 5
          set master %get.mob_shortdesc[55013]%
          set hint A master of spirits in the far north will be able to help next.
          set wandgem 55640
          set wandtask3 58018
          set wandtask4 &6&bthe Arabel Ocean&0
          break
        case 6
          set master %get.mob_shortdesc[23802]%
          set hint Your next crafter is a distant relative of the Sunfire clan.  He's been squatting in a flying fortress for many months, trying to unlock its secrets.
          set wandgem 55666
          set wandtask3 49011
          set wandtask4 17309
          break
        case 7
          set master %get.mob_shortdesc[53316]%
          set hint You'll need the advice of a master ice sculptor.  One works regularly up in Mt. Frostbite.
          set wandgem 55684
          set wandtask3 55016
          set wandtask4 23815
          break
        case 8
          set master %get.mob_shortdesc[10300]%
          set hint There's another distant relative of the Sunfire clan who runs the hot springs near Ickle.  He's book smart and knows a thing or two about jewel crafting.
          set wandgem 55717
          set wandtask3 23847
          set wandtask4 53457
          set place 55105
          break
        case 9
          set master %get.mob_shortdesc[10012]%
          set hint The guild guard for the Sorcerer Guild in Ickle has learned plenty of secrets from the inner sanctum.  Talk to him.
          set wandgem 55743
          set wandtask3 48018
          set wandtask4 53300
          break
        case 10
          set master %get.mob_shortdesc[55020]%
          set hint You must know Suralla Iceeye by now.  She's the master artisan of cold and ice.  She'll know how to make the final improvements to your staff.
          set wandgem 53314
          set wandtask3 52005
          set wandtask4 47708
          break
      done
      if %stage% == 1
        osend %actor% Find %get.mob_shortdesc[3013]% and show him your wand.
        halt
      else
        if %actor.level% >= %minlevel%
          if %actor.quest_variable[ice_wand:greet]% == 0 && %stage% != 2
            osend %actor% Find the next master crafter and tell them why you have come.
            osend %actor% %hint%
            halt
          else
            osend %actor% Quest Master: %master%
            osend %actor% &0
            if %job1% || %job2% || %job3% || %job4%
              osend %actor% You've done the following:
              if %job1%
                osend %actor% - used your %weapon% %attack% times
              endif
              if %job2%
                osend %actor% - found %get.obj_shortdesc[%wandgem%]%
              endif
              if %job3%
                if %stage% != 10
                  osend %actor% - found %get.obj_shortdesc[%wandtask3%]%
                else
                  osend %actor% - slayed %get.mob_shortdesc[%wandtask3%]%
                endif
              endif
              if %job4%
                if %stage% != 5 && %stage% != 9 && %stage% != 10
                  osend %actor% - found %get.obj_shortdesc[%wandtask4%]%
                elseif %stage% == 5
                  osend %actor% - communed in %wandtask4%
                elseif %stage% == 9
                  osend %actor% - slayed %get.mob_shortdesc[%wandtask4%]%
                endif
              endif
              osend %actor% &0 
            endif
            osend %actor% You need to:
            if %job1% && %job2%
              if %stage% == 2
                osend %actor% bring %master% your %weapon%.
              else
                if %job3% 
                  if %stage% == 3
                    osend %actor% bring %master% your %weapon%.
                  else
                    if %job4%
                      if %stage% != 6 && %stage% != 8 && %stage% !=10
                        osend %actor% Bring %master% your %weapon%.
                      elseif %stage% == 6
                        if !%job5%
                          osend %actor% bring %master% your %weapon%.
                        else
                          osend %actor% - &6&bplay&0 %get.obj_shortdesc[%wandtask4%]%
                        endif
                      elseif %stage% == 8
                        if !%job5%
                          osend %actor% Bring %master% your %weapon%.
                        else
                          set room %get.room[%place%]%
                          osend %actor% - imbue your %weapon% at %room.name%.
                        endif
                      elseif %stage% == 10
                        set room %get.room[%wandtask4%]%
                        osend %actor% - imbue your %weapon% at %room.name%.
                      endif
                    endif
                  endif
                endif
              endif
            endif
            if !%job1%
              if %remaining% > 1
                osend %actor% - &3&battack %remaining% more times with your %weapon%&0
              else 
                osend %actor% - &3&battack %remaining% more time with your %weapon%&0
              endif
            endif
            if !%job2%
              osend %actor% - &3&bfind %get.obj_shortdesc[%wandgem%]%&0
            endif
            if %stage% > 2
              if !%job3%
                if %stage% != 10
                  osend %actor% - &3&bfind %get.obj_shortdesc[%wandtask3%]%&0
                  if %stage% == 4
                    osend %actor% &0    Blessings can be called at the smaller groups of standing stones in South Caelia.
                    osend %actor% &0    Search near the crystalline pool in Anlun Vale.
                    osend %actor% &0    The phrase to call the blessing is:
                    osend %actor% &0    &7&bI pray for a blessing from mother earth, creator of life and bringer of death&0
                  endif
                else
                  osend %actor% - &3&bslay %get.mob_shortdesc[%wandtask3%]%&0
                endif
              endif
              if !%job4%
                if %stage% != 3 && %stage% != 5 && %stage% != 9 && %stage% != 10
                  osend %actor% - &3&bfind %get.obj_shortdesc[%wandtask4%]%&0
                elseif %stage% == 5
                  osend %actor% - &3&bimbue your %weapon% in %wandtask4%&0
                elseif %stage% == 9
                  osend %actor% - &3&bslay %get.mob_shortdesc[%wandtask4%]%&0
                elseif %stage% == 10
                  if %job1% && %job2% && %job3%
                    osend %actor% Bring %master% your %weapon%.
                  endif
                endif
              endif
            endif
          endif
        endif  
      endif
    endif
  endif
endif
~
#452
Acid Wand progress journal~
1 m 100
journal quest quest-journal~
if ((%arg% /= acid || %arg% /= earth) && (%arg% /= wand || %arg% /= wands || %arg% /= staff || %arg% /= staves)) || %arg% /= acid_wand || %arg% /= acid_wands || %arg% /= acid_staff || %arg% /= earth_wand || %arg% /= earth_wands || %arg% /= earth_staff
  set sorcererclasses Sorcerer Illusionist Cryomancer Pyromancer Necromancer
  if %sorcererclasses% /= %actor.class%
    return 0
    set stage %actor.quest_stage[acid_wand]%
    eval minlevel (%stage% - 1) * 10
    if %minlevel% < 1
      set minlevel 1
    endif
    osend %actor% &2&b&uAcid Wand&0
    osend %actor% Masters of earth will help you create and upgrade a new mystic weapon.
    if !%actor.has_completed[acid_wand]%
      osend %actor% Minimum Level: %minlevel%
    endif
    if %actor.has_completed[acid_wand]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[acid_wand]%
      set job1 %actor.quest_variable[acid_wand:wandtask1]%
      set job2 %actor.quest_variable[acid_wand:wandtask2]%
      set job3 %actor.quest_variable[acid_wand:wandtask3]%
      set job4 %actor.quest_variable[acid_wand:wandtask4]%
      set job5 %actor.quest_variable[acid_wand:wandtask5]%
      eval attack (%stage% - 1) * 50  
      if %stage% < 8
        set weapon wand
      else
        set weapon staff
      endif
      eval remaining ((%attack%) - %actor.quest_variable[acid_wand:attack_counter]%)
      switch %stage%
        case 2
          set master %get.mob_shortdesc[3013]%
          set wandgem 55576
          break
        case 3
          set master %get.mob_shortdesc[10056]% in Ickle
          set hint First, seek the one who guards the eastern gates of Ickle.
          set wandgem 55593
          set wandtask3 23751
          break
        case 4
          set master %get.mob_shortdesc[62504]% in the Rhell Forest
          set hint The next two artisans dwell in the Rhell Forest south-east of Mielikki.
          set wandgem 55606
          set wandtask3 2332
          set wandtask4 37006
          break
        case 5
          set master %get.mob_shortdesc[62503]%
          set hint Your next crafter isn't exactly part of the ranger network...  It's not actually a person at all.  Find the treant in the Rhell forest and ask it for guidance.
          set wandgem 55647
          set wandtask3 16303
          set wandtask4 &9&bthe Northern Swamps&0
          break
        case 6
          set master %get.mob_shortdesc[47075]% in the Graveyard
          set hint The ranger who guards the massive necropolis near Anduin has wonderful insights on crafting with decay.
          set wandgem 55663
          set wandtask3 55020
          set wandtask4 16107
          break
        case 7
          set master %get.mob_shortdesc[4017]% in the Black-Ice Desert
          set hint Your next guide may be hard to locate...  I believe they guard the entrance to a long-lost kingdom beyond a frozen desert.
          set wandgem 55683
          set wandtask3 16305
          set wandtask4 37082
          break
        case 8
          set master %get.mob_shortdesc[48029]% in the Barrow
          set hint Next, consult with another ranger who guards a place crawling with the dead.  The dwarf ranger in the iron hills will know how to help you.
          set wandgem 55724
          set wandtask3 58414
          set wandtask4 53453
          set place 16355
          break
        case 9
          set master %get.mob_shortdesc[3549]% at the Ranger Guild
          set hint The guard of the only known Ranger Guild in the world is also an excellent craftswoman.  Consult with her.
          set wandgem 55740
          set wandtask3 47006
          set wandtask4 52018
          break
        case 10
          set master %get.mob_shortdesc[16315]%
          set hint Your last guide is the head of the ranger network himself, Eleweiss.  He can help make the final improvements to your staff.
          set wandgem 52031
          set wandtask3 52007
          set wandtask4 47672
          break
      done
      if %stage% == 1
        osend %actor% Find %get.mob_shortdesc[3013]% and show him your wand.
        halt
      else
        if %actor.level% >= %minlevel%
          if %actor.quest_variable[acid_wand:greet]% == 0 && %stage% != 2
            osend %actor% Find the next master crafter and tell them why you have come.
            osend %actor% %hint%
            halt
          else
            osend %actor% Quest Master: %master%
            osend %actor% &0
            if %job1% || %job2% || %job3% || %job4%
              osend %actor% You've done the following:
              if %job1%
                osend %actor% - used your %weapon% %attack% times
              endif
              if %job2%
                osend %actor% - found %get.obj_shortdesc[%wandgem%]%
              endif
              if %job3%
                if %stage% != 10
                  osend %actor% - found %get.obj_shortdesc[%wandtask3%]%
                else
                  osend %actor% - slayed %get.mob_shortdesc[%wandtask3%]%
                endif
              endif
              if %job4%
                if %stage% != 5 && %stage% != 9 && %stage% != 10
                  osend %actor% - found %get.obj_shortdesc[%wandtask4%]%
                elseif %stage% == 5
                  osend %actor% - communed in %wandtask4%
                elseif %stage% == 9
                  osend %actor% - slayed %get.mob_shortdesc[%wandtask4%]%
                endif
              endif
              osend %actor% &0 
            endif
            osend %actor% You need to:
            if %job1% && %job2%
              if %stage% == 2
                osend %actor% bring %master% your %weapon%.
              else
                if %job3% 
                  if %stage% == 3
                    osend %actor% bring %master% your %weapon%.
                  else
                    if %job4%
                      if %stage% != 6 && %stage% != 8 && %stage% !=10
                        osend %actor% Bring %master% your %weapon%.
                      elseif %stage% == 6
                        if !%job5%
                          osend %actor% bring %master% your %weapon%.
                        else
                          osend %actor% - &6&bplay&0 %get.obj_shortdesc[%wandtask4%]%
                        endif
                      elseif %stage% == 8
                        if !%job5%
                          osend %actor% Bring %master% your %weapon%.
                        else
                          set room %get.room[%place%]%
                          osend %actor% - imbue your %weapon% at %room.name%.
                        endif
                      elseif %stage% == 10
                        set room %get.room[%wandtask4%]%
                        osend %actor% - imbue your %weapon% at %room.name%.
                      endif
                    endif
                  endif
                endif
              endif
            endif
            if !%job1%
              if %remaining% > 1
                osend %actor% - &3&battack %remaining% more times with your %weapon%&0
              else 
                osend %actor% - &3&battack %remaining% more time with your %weapon%&0
              endif
            endif
            if !%job2%
              osend %actor% - &3&bfind %get.obj_shortdesc[%wandgem%]%&0
            endif
            if %stage% > 2
              if !%job3%
                if %stage% != 10
                  osend %actor% - &3&bfind %get.obj_shortdesc[%wandtask3%]%&0
                  if %stage% == 4
                    osend %actor% &0    Blessings can be called at the smaller groups of standing stones in South Caelia.
                    osend %actor% &0    Search in the heart of the heart of the thorns.
                    osend %actor% &0    The phrase to call the blessing is:
                    osend %actor% &0    &7&bI pray for a blessing from mother earth, creator of life and bringer of death&0
                  endif
                else
                  osend %actor% - &3&bslay %get.mob_shortdesc[%wandtask3%]%&0
                endif
              endif
              if !%job4%
                if %stage% != 3 && %stage% != 5 && %stage% != 9 && %stage% != 10
                  osend %actor% - &3&bfind %get.obj_shortdesc[%wandtask4%]%&0
                elseif %stage% == 5
                  osend %actor% - &3&bimbue your %weapon% in %wandtask4%&0
                elseif %stage% == 9
                  osend %actor% - &3&bslay %get.mob_shortdesc[%wandtask4%]%&0
                elseif %stage% == 10
                  if %job1% && %job2% && %job3%
                    osend %actor% Bring %master% your %weapon%.
                  endif
                endif
              endif
            endif
          endif
        endif  
      endif
    endif
  endif
endif
~
#453
Spirit Maces progress journal~
1 m 100
journal quest quest-journal~
if (%arg% /= spirit && (%arg% /= mace || %arg% /= maces)) || %arg% /= phase_mace || %arg% /= spirit_mace
  if %actor.class% /= Priest || %actor.class% /= Cleric
    return 0
    set stage %actor.quest_stage[phase_mace]%
    eval minlevel %stage% * 10
    osend %actor% Clerics and priests can make pilgrimages to various spiritual masters to craft weapons to smite the undead.
    osend %actor% &2&b&uSpirit Maces&0
    if !%actor.has_completed[phase_mace]%
      osend %actor% Minimum Level: %minlevel%
    endif
    if %actor.has_completed[phase_mace]%
      set status Completed!
    elseif %stage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage% > 0 && !%actor.has_completed[phase_mace]%
      set job1 %actor.quest_variable[phase_mace:macetask1]%
      set job2 %actor.quest_variable[phase_mace:macetask2]%
      set job3 %actor.quest_variable[phase_mace:macetask3]%
      set job4 %actor.quest_variable[phase_mace:macetask4]%
      set job5 %actor.quest_variable[phase_mace:macetask5]%
      set job6 %actor.quest_variable[phase_mace:macetask6]%
      eval attack %stage% * 50
      eval remaining ((%attack%) - %actor.quest_variable[phase_mace:attack_counter]%)
      switch %stage%
        case 1
          set master %get.mob_shortdesc[3025]%
          set maceitem2 55577
          set maceitem3 55211
          set maceitem4 13614
          set maceitem5 58809
          set hint The Holy Templar Magistrate is a master of spiritual combat.  Perhaps he knows more.
          break
        case 2
          set master %get.mob_shortdesc[18502]%
          set maceitem2 55593
          set maceitem3 18522
          set maceitem4 18523
          set maceitem5 18524
          set maceitem6 18525
          set hint Someone familiar with the grave will be able to work on this mace.  Seek out the Sexton in the Abbey west of the Village of Mielikki.
          break
        case 3
          set master %get.mob_shortdesc[10000]%
          set maceitem2 55604
          set maceitem3 32409
          set maceitem4 59022
          set maceitem5 2327
          set hint The Cleric Guild is capable of some miraculous crafting.  Visit the Cleric Guild Master in the Arctic Village of Ickle and talk to High Priest Zalish.  He should be able to help you.
          break
        case 4
          set master %get.mob_shortdesc[6218]%
          set maceitem2 55631
          set maceitem3 16030
          set maceitem4 47002
          set maceitem5 5211
          set hint Continue with the Cleric Guild Masters.  Check in with the High Priestess in the City of Anduin.
          break
        case 5
          set master %get.mob_shortdesc[8501]%
          set maceitem2 55660
          set maceitem3 43007
          set maceitem4 59012
          set maceitem5 17308
          set hint Sometimes to battle the dead, we need to use their own dark natures against them.  Few are as knowledgeable about the dark arts as Ziijhan, the Defiler, in the Cathedral of Betrayal.
          break      
        case 6
          set master %get.mob_shortdesc[18581]%
          set maceitem2 55681
          set maceitem3 23824
          set maceitem4 53016
          set maceitem5 16201
          set hint Return again to the Abbey of St. George and seek out Silania.  Her mastry of spiritual matters will be necessary to improve this mace any further.
          break
        case 7
          set master %get.mob_shortdesc[6007]%
          set maceitem2 55708
          set maceitem3 49502
          set maceitem4 4008
          set maceitem5 47017
          set hint Of the few remaining who are capable of improving your mace, one is a priest of a god most foul.  Find Ruin Wormheart, that most heinous of Blackmourne's servators.
          break
        case 8
          set master %get.mob_shortdesc[48412]%
          set maceitem2 55737
          set maceitem3 53305
          set maceitem4 12307
          set maceitem5 51073
          set hint The most powerful force in the war against the dead is the sun itself.  Consult with the sun's Oracle in the ancient pyramid near Anduin.
          break
        case 9
          set master %get.mob_shortdesc[3021]%
          set maceitem2 55738
          set maceitem3 48002
          set maceitem4 52010
          set maceitem5 3218
          set hint With everything prepared, return to the very beginning of your journey.  The High Priestess of Mielikki, the very center of the Cleric Guild, will know what to do.
      done
      if %actor.level% >= %minlevel%
        if %actor.quest_variable[phase_mace:greet]% == 0
          osend %actor% Find the next master crafter and tell them why you have come.
          osend %actor% %hint%
          halt
        else
          osend %actor% Quest Master: %master%
          osend %actor% &0
          if %job1% || %job2% || %job3% || %job4% || %job5% || %job6%
            osend %actor% &0 
            osend %actor% You've done the following:
            if %job1%
              osend %actor% - attacked %attack% times
            endif
            if %job2%
              osend %actor% - found %get.obj_shortdesc[%maceitem2%]%
            endif
            if %job3%
              osend %actor% - found %get.obj_shortdesc[%maceitem3%]%
            endif
            if %job4%
              osend %actor% - found %get.obj_shortdesc[%maceitem4%]%
            endif
            if %job5%
              osend %actor% - found %get.obj_shortdesc[%maceitem5%]%
            endif
            if %job6%
              osend %actor% - found %get.obj_shortdesc[%maceitem6%]%
            endif
          endif
          osend %actor% &0 
          osend %actor% You need to:
          if %job1% && %job2% && %job3% && %job4% && %job5%
            if %macestep% != 2
              osend %actor% Bring %master% your mace.
              halt
            else
              if %job6%
                osend %actor% Bring %master% your mace.
                halt
              endif
            endif
          endif
          if !%job1%
            if %remaining% > 1
              osend %actor% - &3&battack %remaining% more times with your mace&0
            else 
              osend %actor% - &3&battack %remaining% more time with your mace&0
            endif
          endif
          if !%job2%
            osend %actor% - &3&bfind %get.obj_shortdesc[%maceitem2%]%&0
          endif
          if !%job3%
            osend %actor% - &3&bfind %get.obj_shortdesc[%maceitem3%]%&0
          endif
          if !%job4%
            osend %actor% - &3&bfind %get.obj_shortdesc[%maceitem4%]%&0
          endif
          if !%job5%
            osend %actor% - &3&bfind %get.obj_shortdesc[%maceitem5%]%&0
          endif
          if %stage% == 2
            if !%job6%
              osend %actor% - &3&bfind %get.obj_shortdesc[%maceitem6%]%&0
            endif
          endif
        endif
      endif
    endif
  endif
endif
~
#454
The Horrors of Nukreth Spire~
1 m 100
journal quest quest-journal~
if %arg% /= horrors || %arg% /= nukreth || %arg% /= spire || %arg% /= nukreth_spire
  if %actor.level% >= 10
    return 0
    osend %actor% &2&b&uThe Horrors of Nukreth Spire&0
    osend %actor% This quest is infinitely repeatable.
    osend %actor% Recommended Level: 20
    if %actor.quest_stage[nukreth_spire]%
      set status Repeatable
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.quest_stage[nukreth_spire]%
      osend %actor% This quest has multiple paths and outcomes.
      set path1 %actor.quest_variable[nukreth_spire:path1]%
      set path2 %actor.quest_variable[nukreth_spire:path2]%
      set path3 %actor.quest_variable[nukreth_spire:path3]%
      set path4 %actor.quest_variable[nukreth_spire:path4]%
      if %path1% || %path2% || %path3% || %path4%
        osend %actor% &0 
        osend %actor% You have assisted the following people:
        if %path1%
          osend %actor% - the human rebel
        endif
        if %path2%
          osend %actor% - the kobold rebel
        endif
        if %path3%
          osend %actor% - the orc rebel
        endif
        if %path4%
          osend %actor% - the goblin rebel
        endif
      endif
    endif
  endif
endif
~
#455
Contract Killers progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= contract || %arg% /= killers || %arg% /= bounty_hunt || %arg% /= contract_killers
  return 0
  set stage %actor.quest_stage[bounty_hunt]%
  set master %get.mob_shortdesc[6051]%
  set bounty %actor.quest_variable[bounty_hunt:bounty]%
  set target1 %actor.quest_variable[bounty_hunt:target1]%
  set target2 %actor.quest_variable[bounty_hunt:target2]%
  set target3 %actor.quest_variable[bounty_hunt:target3]%  
  osend %actor% &2&b&uContract Killers&0
  switch %stage%
    case 1
      set victim1 the King of the Meer Cats
      break
    case 2
      set victim1 the Noble
      set victim2 the Abbot
      break
    case 3
      set victim1 the O'Connor Chieftain
      set victim2 the McLeod Chieftain
      set victim3 the Cameron Chieftain
      break
    case 4
      set victim1 the Frakati Leader
      break
    case 5
      set victim1 Cyrus
      break
    case 6
      set victim1 Lord Venth
      break
    case 7
      set victim1 the high druid of Anlun Vale
      break
    case 8
      set victim1 the Lizard King
      break
    case 9
      set victim1 Sorcha
      break
    case 10
      set victim1 the Goblin King
  done
  if %stage% == 1 || !%stage%
    set level 1
  else
    eval level (%stage% - 1) * 10
  endif
  if !%actor.has_completed[bounty_hunt]%
    osend %actor% Minimum Level: %level%
  endif
  if %actor.has_completed[bounty_hunt]%
    set status Completed!
  elseif %stage%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  if %stage% > 0 && !%actor.has_completed[bounty_hunt]%
    if %actor.level% >= %level%
      osend %actor% Quest Master: %master%
      if %bounty% == running || %bounty% == dead
        if %stage% != 2 && %stage% != 3
          osend %actor% You have a contract for the death of %victim1%.
        elseif %stage% == 2
          osend %actor% You have a contract for the death of %victim1% and %victim2%.
        elseif %stage% == 3
          osend %actor% You have a contract for the death of %victim1%, %victim2%, and %victim3%.
        endif
        if %bounty% == dead
          osend %actor% You have completed the contract.  
          osend %actor% Return it to %master% for your payment!
        elseif %stage% == 2
          if %target1%
            osend %actor% You have scratched %victim1% off the list.
          endif
          if %target2%
            osend %actor% You have scratched %victim2% off the list.
          endif
        elseif %stage% == 3
          if %target1%
            osend %actor% You have scratched %victim1% off the list.
          endif
          if %target2%
            osend %actor% You have scratched %victim2% off the list.
          endif
          if %target3%
            osend %actor% You have scratched %victim3% off the list.
          endif
        endif
      endif
    endif
  endif
endif
~
#456
Assassin Mask progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= deadly || %arg% /= promotion || %arg% /= deadly_promotion || %arg% /=assassin_mask
  if %actor.class% /= Assassin
    return 0
    set bountystage %actor.quest_stage[bounty_hunt]%
    set maskstage %actor.quest_stage[assassin_mask]%
    set master %get.mob_shortdesc[6051]%
    set job1 %actor.quest_variable[assassin_mask:masktask1]%
    set job2 %actor.quest_variable[assassin_mask:masktask2]%
    set job3 %actor.quest_variable[assassin_mask:masktask3]%
    set job4 %actor.quest_variable[assassin_mask:masktask4]%
    osend %actor% &2&b&uDeadly Promotion&0
    if !%stage%
      set level 10
    else
      eval level %maskstage% * 10
    endif
    if !%actor.has_completed[assassin_mask]%
      osend %actor% Minimum Level: %level%
    endif
    if %actor.has_completed[assassin_mask]%
      set status Completed!
    elseif %maskstage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.level% >= %level%
      if %maskstage% == 0
        osend %actor% You have to do a &6&b[job]&0 for %master% first though.
        halt
      elseif (%maskstage% >= %bountystage%) && !%actor.has_completed[bounty_hunt]%
        osend %actor% You need to complete some more Contract Killer jobs before you can receive another promotion.
        halt
      endif
      switch %maskstage%
        case 1
          set mask 4500
          set gem 55592
          set place The Shadowy Lair
          set hint in the Misty Caverns.
          break
        case 2
          set mask 17809
          set gem 55594
          set place The Dark Chamber
          set hint behind a desert door.
          break
        case 3
          set mask 59023
          set gem 55620
          set place A Dark Tunnel
          set hint on the way to a dark, hidden city.
          break
        case 4
          set mask 10304
          set gem 55638
          set place Dark Chamber
          set hint hidden below a ghostly fortress.
          break
        case 5
          set mask 16200
          set gem 55666
          set place Darkness......
          set hint inside an enchanted closet.
          break
        case 6
          set mask 43017
          set gem 55675
          set place Surrounded by Darkness
          set hint in a volcanic shaft.
          break
        case 7
          set mask 51075
          set gem 55693
          set place Dark Indecision
          set hint before an altar in a fallen maze.
          break
        case 8
          set mask 49062
          set gem 55719
          set place Heart of Darkness
          set hint buried deep in an ancient tomb.
          break
        case 9
          set mask 48427
          set gem 55743
          set place A Dark Room
          set hint under the ruins of a shop in an ancient city.
      done
      eval attack %maskstage% * 100
      osend %actor% Quest Master: %master%
      if %job1% || %job2% || %job3% || %job4%
        osend %actor% &0 
        osend %actor% You've done the following:
        if %job1%
          osend %actor% - attacked %attack% times
        endif
        if %job2%
          osend %actor% - found %get.obj_shortdesc[%mask%]%
        endif
        if %job3%
          osend %actor% - found %get.obj_shortdesc[%gem%]%
        endif
        if %job4%
          osend %actor% - hidden in %place%
        endif
      endif
      osend %actor% &0 
      osend %actor% You need to:
      if %job1% && %job2% && %job3% && %job4%
        osend %actor% Give %master% your old mask.
        halt
      endif
      if !%job1%
        eval remaining %attack% - %actor.quest_variable[assassin_mask:attack_counter]%
        osend %actor% - attack &9&b%remaining%&0 more times while wearing your mask.
      endif
      if !%job2%
        osend %actor% - find &9&b%get.obj_shortdesc[%mask%]%&0
      endif
      if !%job3%
        osend %actor% - find &9&b%get.obj_shortdesc[%gem%]%&0
      endif
      if !%job4%
        osend %actor% - &9&bhide in a place called "&9&b%place%&0".
        osend %actor%&0   It's &9&b%hint%&0
      endif
    endif
  endif
endif
~
#457
Dragon Slayers progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= dragon_slayers || %arg% /= slayers || %arg% /= slayer || %arg% /= dragon slayer || %arg% /= dragon slayers
  return 0
  set stage %actor.quest_stage[dragon_slayer]%
  set master %get.mob_shortdesc[3080]%
  set hunt %actor.quest_variable[dragon_slayer:hunt]%
  set target1 %actor.quest_variable[dragon_slayer:target1]%
  osend %actor% &2&b&uDragon Slayers&0
  switch %stage%
    case 1
      set victim1 a dragon hedge
      break
    case 2
      set victim1 the green wyrmling
      break
    case 3
      set victim1 Wug the Fiery Drakling
      break
    case 4
      set victim1 the young blue dragon
      break
    case 5
      set victim1 a faerie dragon
      break
    case 6
      set victim1 the wyvern
      break
    case 7
      set victim1 an ice lizard
      break
    case 8
      set victim1 the Beast of Borgan
      break
    case 9
      set victim1 Tri-Aszp
      break
    case 10
      set victim1 the Hydra
  done
  if %stage% == 1 || !%stage%
    set level 5
  else
    eval level (%stage% - 1) * 10
  endif
  if !%actor.has_completed[dragon_slayer]%
    osend %actor% Minimum Level: %level%
  endif
  if %actor.has_completed[dragon_slayer]%
    set status Completed!
  elseif %stage%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  if %stage% > 0 && !%actor.has_completed[dragon_slayer]%
    if %actor.level% >= %level%
      osend %actor% Quest Master: %master%
      if %hunt% == dead || %hunt% == running
        osend %actor% You have a contract for the death of %victim1%.
        if %hunt% == dead
          osend %actor% You have completed the hunt.  
          osend %actor% Return the notice to %master% for your reward!
        endif
      endif
    endif
  endif
endif
~
#458
Divine Devotion progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= divine || %arg% /= devotion || %arg% /= divine_devotion || %arg% /= paladin_pendant || %arg% /= paladin pendant
  set anti Anti-Paladin
  if %actor.class% /= Paladin || %actor.class% == %anti%
    return 0
    set huntstage %actor.quest_stage[dragon_slayer]%
    set pendantstage %actor.quest_stage[paladin_pendant]%
    set master %get.mob_shortdesc[3080]%
    set job1 %actor.quest_variable[paladin_pendant:necklacetask1]%
    set job2 %actor.quest_variable[paladin_pendant:necklacetask2]%
    set job3 %actor.quest_variable[paladin_pendant:necklacetask3]%
    set job4 %actor.quest_variable[paladin_pendant:necklacetask4]%
    osend %actor% &2&b&uDivine Devotion&0
    if !%pendantstage%
      set level 10
    else
      eval level %pendantstage% * 10
    endif
    if !%actor.has_completed[paladin_pendant]%
      osend %actor% Minimum Level: %level%
    endif
    if %actor.has_completed[paladin_pendant]%
      set status Completed!
    elseif %pendantstage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.level% >= %level%
      if %pendantstage% == 0
        osend %actor% Your first act of devotion should be to &6&b[hunt]&0 a dragon.
        halt
      elseif (%pendantstage% >= %huntstage%) && !%actor.has_completed[dragon_slayer]%
        osend %actor% Slay a few more dragons before you can receive another promotion.
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
      done
      eval attack %pendantstage% * 100
      osend %actor% Quest Master: %master%
      if %job1% || %job2% || %job3% || %job4%
        osend %actor% &0 
        osend %actor% You've done the following:
        if %job1%
          osend %actor% - attacked %attack% times
        endif
        if %job2%
          osend %actor% - found %get.obj_shortdesc[%necklace%]%
        endif
        if %job3%
          osend %actor% - found %get.obj_shortdesc[%gem%]%
        endif
        if %job4%
          osend %actor% - prayed in %place%
        endif
      endif
      osend %actor% &0
      osend %actor% You need to:
      if %job1% && %job2% && %job3% && %job4%
        osend %actor% Give %master% your old necklace.
        halt
      endif
      if !%job1%
        eval remaining %attack% - %actor.quest_variable[paladin_pendant:attack_counter]%
        osend %actor% - attack &3&b%remaining%&0 more times while wearing your necklace.
      endif
      if !%job2%
        osend %actor% - find &3&b%get.obj_shortdesc[%necklace%]%&0
      endif
      if !%job3%
        osend %actor% - find &3&b%get.obj_shortdesc[%gem%]%&0
      endif
      if !%job4%
        osend %actor% - &3&bpray&0 in a place called "&3&b%place%&0".
        osend %actor%&0   It's &3&b%hint%&0
      endif
    endif
  endif
endif
~
#459
Beast Masters progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= beast || %arg% /= masters || %arg% /= beast_masters
  return 0
  set stage %actor.quest_stage[beast_master]%
  set master %get.mob_shortdesc[5306]%
  set bounty %actor.quest_variable[beast_master:hunt]%
  set target1 %actor.quest_variable[beast_master:target1]%
  osend %actor% &2&b&uBeast Masters&0
  switch %stage%
    case 1
      set victim1 an abominable slime creature
      break
    case 2
      set victim1 a large buck
      break
    case 3
      set victim1 the giant scorpion
      break
    case 4
      set victim1 a monstrous canopy spider
      break
    case 5
      set victim1 the chimera
      break
    case 6
      set victim1 the drider king
      break
    case 7
      set victim1 a beholder
      break
    case 8
      set victim1 the Banshee
      break
    case 9
      set victim1 Baba Yaga
      break
    case 10
      set victim1 the medusa
  done
  if %stage% == 1 || !%stage%
    set level 1
  else
    eval level (%stage% - 1) * 10
  endif
  if !%actor.has_completed[beast_master]%
    osend %actor% Minimum Level: %level%
  endif
  if %actor.has_completed[beast_master]%
    set status Completed!
  elseif %stage%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  if %stage% > 0 && !%actor.has_completed[bounty_hunt]%
    if %actor.level% >= %level%
      osend %actor% Quest Master: %master%
      if %bounty% == running || %bounty% == dead
        osend %actor% You have an assignment to slay %victim1%.
        if %bounty% == dead
          osend %actor% You have completed the hunt.  
          osend %actor% Return your assignment to Pumahl for your reward!
        endif
      endif
    endif
  endif
endif
~
#460
Eye of the Tiger progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= eye of the tiger || %arg% /= tiger
  set hunterclasses Warrior Ranger Berserker Mercenary
  if %hunterclasses% /= %actor.class%
    return 0
    set huntstage %actor.quest_stage[beast_master]%
    set trophystage %actor.quest_stage[ranger_trophy]%
    set job1 %actor.quest_variable[ranger_trophy:trophytask1]%
    set job2 %actor.quest_variable[ranger_trophy:trophytask2]%
    set job3 %actor.quest_variable[ranger_trophy:trophytask3]%
    set job4 %actor.quest_variable[ranger_trophy:trophytask4]%
    osend %actor% &2&b&uEye of the Tiger&0
    if !%trophystage%
      set level 10
    else
      eval level %trophystage% * 10
    endif
    if !%actor.has_completed[ranger_trophy]%
      osend %actor% Minimum Level: %level%
    endif
    if %actor.has_completed[ranger_trophy]%
      set status Completed!
    elseif %trophystage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.level% >= %level%
      if %trophystage% == 0
        osend %actor% You must &6&b[hunt]&0 a great beast to demonstrate your skills first.
        halt
      elseif (%trophystage% >= %huntstage%) && !%actor.has_completed[beast_master]%
        osend %actor% Prove your dominion over some more great beasts before you can demonstrate your skills.
        halt
      endif
      switch %trophystage%
        case 1
          set trophy 1607
          set gem 55579
          set place A Coyote's Den
          set hint near the Kingdom of the Meer Cats.
          break
        case 2
          set trophy 17806
          set gem 55591
          set place In the Lions' Den
          set hint in the western reaches of Gothra.
          break
        case 3
          set trophy 1805
          set gem 55628
          set place either of the two Gigantic Roc Nests
          set hint in the Wailing Mountains.
          break
        case 4
          set trophy 62513
          set gem 55652
          set place Chieftain's Lair
          set hint in Nukreth Spire in South Caelia.
          break
        case 5
          set trophy 23803
          set gem 55664
          set place The Heart of the Den
          set hint where the oldest unicorn in South Caelia makes its home.
          break
        case 6
          set trophy 43009
          set gem 55685
          set place Giant Lynx's Lair
          set hint far to the north beyond Mt. Frostbite.
          break
        case 7
          set trophy 47008
          set gem 55705
          set place Giant Griffin's Nest
          set hint tucked away in a secluded and well guarded corner of Griffin Island.
          break
        case 8
          set trophy 53323
          set gem 55729
          set place Witch's Den
          set hint entombed with an ancient evil king.
          break
        case 9
          set trophy 52014
          set gem 55741
          set place Dargentan's Lair
          set hint at the pinnacle of his flying fortress.
      done
      eval attack %trophystage% * 100
      if %job1% || %job2% || %job3% || %job4% 
        osend %actor% You've done the following:
        if %job1%
          osend %actor% - attacked %attack% times
        endif
        if %job2%
          osend %actor% - found %get.obj_shortdesc[%trophy%]%
        endif
        if %job3%
          osend %actor% - found %get.obj_shortdesc[%gem%]%
        endif
        if %job4%
          osend %actor% - foraged in %place%
        endif
      endif
      osend %actor% &0 
      osend %actor% You need to:
      if %job1% && %job2% && %job3% && %job4%
        osend %actor% Give %master% your old trophy.
        halt
      endif
      if !%job1%
        eval remaining %attack% - %actor.quest_variable[ranger_trophy:attack_counter]%
        osend %actor% - attack &2&b%remaining%&0 more times while wearing your trophy.
      endif
      if !%job2%
        osend %actor% - find &2&b%get.obj_shortdesc[%trophy%]%&0
      endif
      if !%job3%
        osend %actor% - find &2&b%get.obj_shortdesc[%gem%]%&0
      endif
      if !%job4%
        osend %actor% - &2&bforage&0 in a place called "&2&b%place%&0".
        osend %actor%&0   It's &2&b%hint%&0
      endif
    endif
  endif
endif
~
#461
Siege Mystwatch Fortress progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= siege || %arg% /= mystwatch || %arg% /= fortress || %arg% /= mystwatch_quest || %arg% /= siege_mystwatch_fortress
  if %actor.level% >= 35
    return 0
    set stage %actor.quest_stage[mystwatch_quest]%
    osend %actor% &2&b&uSiege Mystwatch Fortress&0
    osend %actor% Recommended Level: 45
    if %stage%
      set status Repeatable
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %stage%
      osend %actor% Quest Master: %get.mob_shortdesc[3025]%
      switch %actor.quest_variable[mystwatch_quest:step]%
        case totem
          set task Give %get.obj_shortdesc[3026]% to %get.mob_shortdesc[16007]%.
          break
        case general
          set task Kill %get.mob_shortdesc[16007]%.
          break
        case skeleton
          set task Kill %get.mob_shortdesc[16015]%.
          break
        case warrior
          set task Kill %get.mob_shortdesc[16016]%.
          break
        case sentry
          set task Kill %get.mob_shortdesc[16017]%.
          break
        case warlord
          set task Kill %get.mob_shortdesc[16018]%.
          break
        case blacksmith
          set task Kill %get.mob_shortdesc[16019]%.
          break
        case shadow
          set task Kill %get.mob_shortdesc[16010]%.
          break
        case storm
          set task Kill %get.mob_shortdesc[16008]%.
          break
        case lord
          set task Kill %get.mob_shortdesc[16011]%.
          break
        case shard
          set task Give %get.obj_shortdesc[16023]% to %get.mob_shortdesc[3025]%.
          break
        default
          set task Visit %get.mob_shortdesc[3025]% to restart this quest.
      done
      osend %actor% &0 
      osend %actor% Your next step: %task%
    endif
  endif
endif
~
#462
Anti-Paladin Diabolist Necromancer Subclass progress journal~
1 m 100
journal quest quest-journal~
if %actor.level% > 10
  set anti Anti-Paladin
  set antipaladinraces dragonborn_fire dragonborn_frost dragonborn_gas dragonborn_acid dragonborn_lightning Drow Duergar Human Orc 
  set diabolistraces dragonborn_fire dragonborn_frost dragonborn_gas dragonborn_acid dragonborn_lightning duergar human orc sverfneblin drow
  set necromancerraces dragonborn_fire dragonborn_frost dragonborn_gas dragonborn_acid dragonborn_lightning unseelie_faerie human orc sverfneblin
  if %arg% /= %anti% && %actor.class% /= Warrior && %actor.level% <= 25 && %antipaladinraces% /= %actor.race%
    osend %actor% &1&bAnti-Paladin&0
    set check yes
  elseif %arg% /= Diabolist && %actor.class% /= Cleric && %actor.level% <= 35 && %diabolistraces% /= %actor.race%
    osend %actor% &5Diabolist&0
    set check yes
  elseif %arg% /= Necromancer && %actor.class% /= Sorcerer && %actor.level% <= 45 && %necromancerraces% /= %actor.race%
    osend %actor% &9&bNecromancer&0
    set check yes
  endif
  if %check% /= yes
    return 0
    osend %actor% Quest Master: %get.mob_shortdesc[8501]%
    osend %actor% &0
    osend %actor% Minimum Level: 10
    osend %actor% This class is for evil characters only.
    if %actor.quest_stage[nec_anti_dia_subclass]%
      osend %actor% %get.mob_shortdesc[8501]% said to you:
      switch %actor.quest_stage[nec_anti_dia_subclass]%
        case 1
          osend %actor% Only the most cunning and strong will complete the &5quest&0 I set before you.
          osend %actor% I shall take great pleasure in your demise, but I will offer great rewards for your success.
          break
        case 2
          osend %actor% Many years ago, my pact with the demon realm allowed me to be master of this domain.
          osend %actor% All were subjugated, man, woman, and beast.&_
          osend %actor% One man would not bow though!
          osend %actor% My pitiful waste of a &5brother&0 escaped my minions.&_
          osend %actor% Perhaps you will remedy that.
          break
        case 3
        case 4
          osend %actor% My wretched sibling, Ber...  I shall not utter his name!&_
          osend %actor% I despise him and his reverent little life.&_
          osend %actor% He thinks he is safe now, beyond my grasp.  That FOOL!&_
          osend %actor% Bring back proof of the deed and you shall be accepted.
      done
    endif
  endif
endif
~
#463
Monk Subclass progress journal~
1 m 100
journal quest quest-journal~
set monkraces human halfelf
if %actor.level% > 10
  if %arg% /= Monk && %actor.class% /= Warrior && %actor.level% <= 25 && (%monkraces% /= %actor.race%)
    return 0
    osend %actor% &3Monk&0
    osend %actor% Quest Master: %get.mob_shortdesc[5130]%
    osend %actor% &0
    osend %actor% Minimum Level: 10
    if %actor.quest_stage[monk_subclass]%
      osend %actor% %get.mob_shortdesc[5130]% said to you:
      switch %actor.quest_stage[monk_subclass]%
        case 1
          osend %actor% It is wonderful to hear of others wanting to join the Brotherhood of the Monks.&_
          osend %actor% You will be rewarded for your success with wonderful training, but only those pure of mind will complete the &6&bquest&0.
          break
        case 2
          osend %actor% Usually people come in here promising me the return of something long lost of mine.
          osend %actor% The thing is, I have not always lead this life.  When I was young I was quite the rabble-rouser.&_
          osend %actor% Well, it is rather embarrassing, but...  I miss my old sash, and I want it back.&_
          osend %actor% I was told it looked wonderful on me.&_
          osend %actor% %actor.name%, can you recover it?&_
          osend %actor% Please?&_
          osend %actor% Long ago some ruthless &6&bfiends&0 made off with it.
          break
        case 3
        case 4
          osend %actor% Have you recovered my sash from those thieves out west?&_
          osend %actor% If you return my sash, I will complete your initiation as a monk.
      done
    endif
  endif
endif
~
#464
Pyromancer Subclass progress journal~
1 m 100
journal quest quest-journal~
if %actor.level% > 10
  if %arg% /= Pyromancer && %actor.class% /= Sorcerer && %actor.level% <= 45 && %actor.race% != dragonborn_frost
    return 0
    osend %actor% &1&bPyromancer&0
    osend %actor% Quest Master: %get.mob_shortdesc[5230]%
    osend %actor% &0
    osend %actor% Minimum Level: 10
    if %actor.quest_stage[pyromancer_subclass]%
      osend %actor% %get.mob_shortdesc[5230]% said to you:
      switch %actor.quest_stage[pyromancer_subclass]%
        case 1
          osend %actor% Only the best and most motivated of mages will complete the &1&bquest&0 I lay before you.&_
          osend %actor% However, I am sure it is in you, if it is truly your desire, to complete this quest and become a pyromancer.
          break
        case 2
          osend %actor% Part of the essence of fire is no longer under my power.
          osend %actor% I once controlled all three parts of the flame: &7&bWhite&0, &bGray&0, and &9&bBlack&0.&_
          osend %actor% But one of them was taken from my &1&bcontrol&0.
          break
        case 3
        case 4
          osend %actor% The %actor.quest_variable[pyromancer_subclass:part]% flame was taken from me.&_
          osend %actor% To truly help, I suggest you stop loitering and go recover it.'
          switch %actor.quest_variable[pyromancer_subclass:part]%
            case white
              set place &bin some kind of mine&0
              break
            case black
              set place &bin some kind of temple&0
              break
            case gray
            default
              set place &bnear some kind of hill&0
          done
          osend %actor% Last I heard, it was &6&b%place%&0, or something of the like.
      done
    endif
  endif
endif
~
#465
Mercenary Assassin Thief Subclass progress journal~
1 m 100
journal quest quest-journal~
if %actor.level% > 10
  set assassinraces Duergar drow Human Orc Sverfneblin
  set mercenaryraces Barbarian Duergar Dwarf Human Ogre Orc Troll drow
  set thiefraces Dwarf Duergar Gnome Halfelf Halfling Human Orc Seelie_Faerie Sverfneblin Unseelie_Faerie elf
  if %actor.class% /= Rogue && %actor.level% <= 25
    if %arg% /= Mercenary && %mercenaryraces% /= %actor.race%
      osend %actor% &9&bMercenary&0
      set questname mercenary
      set check yes
    elseif %arg% /= Assassin && %assassinraces% /= %actor.race%
      osend %actor% &1Assassin&0
      set questname assassin
      set check yes
    elseif %arg% /= Thief && %thiefraces% /= %actor.race%
      osend %actor% &1&bThief&0
      set questname thief
      set check yes
    endif
  endif
  if %check% /= yes
    return 0
    osend %actor% Quest Master: %get.mob_shortdesc[6050]%
    osend %actor% &0
    osend %actor% Minimum Level: 10
    if %questname% == assassin
      osend %actor% This class is for evil characters only.
    endif
    if %actor.quest_stage[merc_ass_thi_subclass]%
      osend %actor% %get.mob_shortdesc[6050]% said to you:
      switch %actor.quest_stage[merc_ass_thi_subclass]%
        case 1
          if %quest_name% == mercenary
            osend %actor% He would pay well.  Yes, that &6&bLord&0 would pay well indeed.
          elseif %quest_name% == assassin
            osend %actor% Yes, that would bring a good &6&bprice&0.
          elseif %quest_name% == thief
            osend %actor% There is a &6&bpackage&0 that someone could get back.
          endif
          break
        case 2
          if %quest_name% == mercenary
            osend %actor% Well, a great Lord, who shall remain unnamed, has lost a cloak.
            osend %actor% He has come to me for its return.
            osend %actor% If you went and procured it, he would be grateful.
            osend %actor% And if he is grateful, I would be as well, and your training would be finished.&_
            osend %actor% It would be quite a payday for a &6&bcloak&0.
          elseif %quest_name% == thief
            osend %actor% Some time ago it was sent and picked up by someone who should not have it.&_
            osend %actor% Bloody &6&bfarmers&0.
          elseif %quest_name% == assassin
            osend %actor% I have some rich men unhappy with the politics of the region in question.
            osend %actor% You could help with those &6&bpolitics&0 if you wish.
          endif
          break
        case 3
        case 4
          if %quest_name% == mercenary
            osend %actor% It was made off with in a raid on his castle by some bothersome insect warriors.
            osend %actor% All the Lord was able to tell me is they said something about wanting it for their queen.&_
            osend %actor% I think you should go find it now.&_
            osend %actor% Come back when you ave the &3&bcloak&0, or do not come back at all.
          elseif %quest_name% == assassin
            osend %actor% Ah yes, the politics of it all.
            osend %actor% Personally I am not one for them, but some people get all mixed up in those.&_
            osend %actor% Go kill the Mayor of Mielikki.&_
            osend %actor% He's probably holed up in his office in City Hall.
            osend %actor% You'll have to break in, sneak past the guards, and kill him.&_
            osend %actor% Get his &3&bcane&0 as proof and come back and give it to me.
          elseif %quest_name% == thief
            osend %actor% That is right, a &3&bpackage&0 was taken by a farmer who hould not have it.&_
            osend %actor% I know this: he got it from the post office in Mielikki and he lives near there.
            osend %actor% Go get it back and I will make it worth it to you.&_
            osend %actor% Do not let anyone see you and do not leave a trail of bodies behind you.
            osend %actor% And be careful!  If you jostle the package too much it just might explode.
          endif
      done
    endif
  endif
endif
~
#466
Illusionist Subclass progress journal~
1 m 100
journal quest quest-journal~
if %actor.level% > 10
  if %arg% /= Illusionist && %actor.class% /= Sorcerer && %actor.level% <= 45
    return 0
    osend %actor% &5&bIllusionist&0
    osend %actor% Quest Master: %get.mob_shortdesc[17200]%
    osend %actor% &0
    osend %actor% Minimum Level: 10
    if %actor.quest_stage[illusionist_subclass]%
      osend %actor% %get.mob_shortdesc[17200]% said to you:
      switch %actor.quest_stage[illusionist_subclass]%
        case 1
          osend %actor% This is what I need you to do...
          osend %actor% That ruffian in the smuggler's hideout once stole a woman from me, on the very eve of our betrothal announcement.
          osend %actor% Or my pride, perhaps. 
          osend %actor% It appears that she was his from the very beginning.&_
          osend %actor% She has departed, she is lost, my dear Cestia...
          osend %actor% She boarded a ship for the southern seas and never returned.
          osend %actor% I fear she is dead.
          osend %actor% But she may have survived, and on that fact this plan hinges.&_
          osend %actor% I gave her a valuable onyx choker as a betrothal gift.
          osend %actor% She took it in her deceit.&_
          osend %actor% The smuggler leader took the choker, and keeps it as a prize.
          osend %actor% To humiliate me, perhaps.
          osend %actor% And now he has raised defenses that even I cannot penetrate.&_
          osend %actor% He knows my fatal weakness: hay fever.&_
          osend %actor% So he has placed flowers, to reveal me should I enter his home in disguise.&_
          osend %actor% This is where you come in.&_
          osend %actor% I would like you to enter the hideout in disguise and take back the choker.
          osend %actor% But beware: it is well hidden.&_
          osend %actor% Somehow that brutish fool has engaged the services of an illusionist...
          osend %actor% I would like to know who helped him... but no matter.&_
          osend %actor% I will enchant you to resemble dear Cestia when the smugglers look upon you.
          osend %actor% Their leader will no doubt welcome you with open arms.&_
          osend %actor% But we must ensure that he will reveal its hiding place to you.
          osend %actor% For that, we will make it appear as if the guards of Mielikki have discovered their hideout.&_
          osend %actor% It is my hope that the leader will hide you - Cestia - for safekeeping, in his most secure location.&_
          osend %actor% There, perhaps, you will find the choker.&_
          osend %actor% I will give you a &3&bvial of disturbance&0.
          osend %actor% &6&bDrop it in their hideout&0 and once you find their leader Gannigan sounds of shouting and fighting will resonate throughout the area.
          osend %actor% It will convince the smugglers that they are under attack.&_
          osend %actor% You must take care to drop it out of sight of any smugglers, or they may become suspicious.
          osend %actor% Then go immediately to the leader, and stall him until you hear the sounds of invasion.&_
          osend %actor% Come back and ask for &6&bhelp&0 if you get stuck!
          break
        case 2
          osend %actor% Did you meet with the leader?
          osend %actor% I hope the disguise was sufficient.&_
          osend %actor% Perhaps it would be worthwhile to try again.
          osend %actor% The smuggler leader should be searching high and low for Cestia, now.&_
          osend %actor% If you are willing, I will refresh your disguise for another attempt.
          osend %actor% Say &6&b'begin'&0 when you are ready.
          break
        default
          osend %actor% Do you have the choker?  Did you lose it?&_
          osend %actor% If you want to try again, say &6&b'restart'&0 and I will refresh your magical disguise.
      done
    endif
  endif
endif
~
#467
Paladin Priest Subclass progress journal~
1 m 100
journal quest quest-journal~
if %actor.level% > 10
  set priestraces dragonborn_fire dragonborn_frost dragonborn_gas dragonborn_acid dragonborn_lightning dwarf gnome halfelf halfling human elf
  set paladinraces dragonborn_fire dragonborn_frost dragonborn_gas dragonborn_acid dragonborn_lightning dwarf elf human
  if %arg% /= Paladin && %actor.class% /= Warrior && %actor.level% <= 25 && %paladinraces% /= %actor.race%
    osend %actor% &7&bPaladin&0
    set check yes
  elseif %arg% /= Priest && %actor.class% /= Cleric && %actor.level% <= 35 && %priestraces% /= %actor.race%
    osend %actor% &6&bPriest&0
    set check yes
  endif
  if %check% /= yes
    return 0
    osend %actor% Quest Master: %get.mob_shortdesc[18581]%
    osend %actor% &0
    osend %actor% Minimum Level: 10
    osend %actor% This class is for good characters only.
    if %actor.quest_stage[pri_pal_subclass]%
      osend %actor% %get.mob_shortdesc[18581]% said to you:
      switch %actor.quest_stage[pri_pal_subclass]%
        case 1
          osend %actor% It is necessary to make a quest such as this quite tough to ensure you really want to do this.&_
          osend %actor% I am sure you will complete the &6&bquest&0 though.
          break
        case 2
          osend %actor% One of our guests made off with our most sacred bronze chalice.&_
          osend %actor% We have reason to believe it was a ruse by the filthy diabolists to try and weaken us.
          osend %actor% &0Our Prior has offered to try and retrieve it for us, but...&_
          osend %actor% I think perhaps that would be a bad idea and that you should find it instead.
          break
        case 3
          osend %actor% Have you found the bronze chalice the diabolists stole?
      done
    endif
  endif
endif
~
#468
Ranger Druid Subclass progress journal~
1 m 100
journal quest quest-journal~
if %actor.level% > 10
  set druidraces Gnome Halfflf Human Nymph Seelie_Faerie Unseelie_Faerie elf
  set rangerraces elf halfelf human Seelie_Faerie nymph
  if %arg% /= Ranger && %actor.class% /= Warrior && %actor.level% <= 25 && %rangerraces% /= %actor.race%
    osend %actor% &2&bRanger&0
    set check yes
  elseif %arg% /= Druid && %actor.class% /= Cleric && %actor.level% <= 35 && %druidraces% /= %actor.race%
    osend %actor% &2Druid&0
    set check yes
  endif
  if %check% /= yes
    return 0
    osend %actor% Quest Master: %get.mob_shortdesc[16315]%
    osend %actor% &0
    osend %actor% Minimum Level: 10
    if %arg% /= Ranger
      osend %actor% This class is for good characters only.
    else
      osend %actor% This class is for neutral characters only.
    endif
    if %actor.quest_stage[ran_dru_subclass]%
      osend %actor% %get.mob_shortdesc[16315]% said to you:
      switch %actor.quest_stage[ran_dru_subclass]%
        case 1
          osend %actor% Only the most dedicated to the forests shall complete the &6&bquest&0 I set upon you.
          break
        case 2
          osend %actor% Yes, quest. I do suppose it would help if I told you about it.&_
          osend %actor% Long ago I &6&blost something&0.
          osend %actor% It is a shame, but it has never been recovered.&_
          osend %actor% If you were to help me with that, then we could arrange something.
          break
        case 3
        case 4
          osend %actor% It seems I am becoming forgetful in my age.&_
          osend %actor% Well, you see now, I lost the jewel of my heart.
          osend %actor% If you are up to it, finding and returning it to me will get you your reward.&_
          osend %actor% But for now, it is time for you to depart I think.&_
          osend %actor% You have brought up painful memories for me to relive.
      done
    endif
  endif
endif
~
#469
Cryomancer Subclass progress journal~
1 m 100
journal quest quest-journal~
if %actor.level% > 10
  if %arg% /= Cryomancer && %actor.class% /= Sorcerer && %actor.level% <= 45 && %actor.race% != dragonborn_fire
    return 0
    osend %actor% &4&bCryomancer&0
    osend %actor% Quest Master: %get.mob_shortdesc[55020]%
    osend %actor% &0
    osend %actor% Minimum Level: 10
    if %actor.quest_stage[cryomancer_subclass]%
      osend %actor% %get.mob_shortdesc[55020]% said to you:
      switch %actor.quest_stage[cryomancer_subclass]%
        case 1
          osend %actor% It will take a great mage with a dedication to the cold arts to complete the &6&bquest&0 I lay before you.
          osend %actor% Your reward is simple if you succeed, and I am sure you will enjoy a life of the cold.
          break
        case 2
          osend %actor% My counterpart, the great Emmath Firehand, long ago battled with me once.&_
          osend %actor% It was not serious by any means, but it did end in a stalemate.
          osend %actor% The catch however...&_
          osend %actor% Is that what we battled over may still be &6&bsuffering&0.      
          break
        case 3
          osend %actor% It is a shame really, that poor shrub, it really was an innocent in all of that.&_
          osend %actor% I do feel bad about it.
          osend %actor% The poor thing tried to flee us and sought the shaman who created him.&_
          osend %actor% I do not know if that will help you end his suffering or not, but I hope it does.&_
          osend %actor% And the reward will be great if you do.&_
          osend %actor% The shrub muttered something about a place with rushing water and some odd warriors being his safety.&_
          osend %actor% Oh!  One last thing.
          osend %actor% When you return to claim your reward, be sure to say to me &6&b"The shrub suffers no longer"&0, and the prize will be yours.
          break
        case 4
          osend %actor% Say &6&b"The shrub suffers no longer"&0, and the prize will be yours.
      done
    endif
  endif
endif
~
#470
Berserker Subclass progress journal~
1 m 100
journal quest quest-journal~
if %actor.level% > 10
  set berserkerraces Barbarian Duergar Dwarf Ogre Orc Troll
  if %arg% /= Berserker && %actor.class% /= Warrior && %actor.level% <= 25 && %berserkerraces% /= %actor.race%
    return 0
    osend %actor% &9&bBerserker&0
    osend %actor% Quest Master: %get.mob_shortdesc[36430]%
    osend %actor% &0
    osend %actor% Minimum Level: 10
    if %actor.quest_stage[berserker_subclass]%
      osend %actor% %get.mob_shortdesc[36430]% said to you:
      switch %actor.quest_stage[berserker_subclass]%
        case 1
          osend %actor% There are a few shared rites that bind us together.
          osend %actor% None is more revered than the &6&bWild Hunt&0.
          break
        case 2
          osend %actor% Let us challenge the Spirits for the right to prove ourselves!
          osend %actor% If they deem you worthy, the Spirits send you a vision of a mighty &6&bbeast&0.
          break
        case 3
          osend %actor% Howl to the spirits before %get.mob_shortdesc[36430]% and make your song known!
          break
        case 4
          switch %actor.quest_variable[berserker_subclass:target]%
            case 16105
              set target 16105
              set place a desert cave
              break
            case 16310
              set target 16310
              set place some forested highlands
              break
            case 20311
              set target 20311
              set place a vast plain
              break
            case 55220
              set target 55220
              set place the frozen tundra
              break
          done
          osend %actor% The Spirits revealed to you a vision of %get.mob_shortdesc[%target%]%!
          osend %actor% You saw it is in %place%!
      done
    endif
  endif
endif
~
#471
Bard Subclass progress journal~
1 m 100
journal quest quest-journal~
if %actor.level% > 10
  set bardraces Dwarf Elf Gnome Halfelf Halfling Human Nymph Seelie_Faerie Sverfneblin Unseelie_Faerie
  if %arg% /= Bard && %actor.class% /= Rogue && %actor.level% <= 25 && %bardraces% /= %actor.race%
    return 0
    osend %actor% &5&bBard&0
    osend %actor% Quest Master: %get.mob_shortdesc[4398]%
    osend %actor% &0
    osend %actor% Minimum Level: 10
    if %actor.quest_stage[bard_subclass]%
      switch %actor.quest_stage[bard_subclass]%
        case 1
          osend %actor% Go to the audition room and &5&bsing&0 for %get.mob_shortdesc[4398]%.
          break
        case 2
          osend %actor% Go to the audition room and &5&bdance&0 for %get.mob_shortdesc[4398]%.
          break
        case 3
        case 4
          osend %actor% You were looking for an old &3&bscript&0 in Morgan Hill.
          osend %actor% Give it to %get.mob_shortdesc[4398]% when you find it.
          break
        case 5
          osend %actor% %get.mob_shortdesc[4398]% said to you:
          osend %actor% Time to gimme some &6&bdialogue&0 work.
          osend %actor% I sure hope you're off book!&_
          osend %actor% That means "memorized" in the business.
      done
    endif
  endif
endif
~
#472
Armor clarification~
1 m 100
journal quest quest-journal~
if %arg% == armor
  return 0
  osend %actor% Please specify:
  osend %actor% Guild Armor Phase One
  osend %actor% Guild Armor Phase Two
  osend %actor% Guild Armor Phase Three
  if (%actor.class% /= Priest && %actor.level% >= 50) || (%actor.class% /= Cleric && %actor.level% >= 65)
    osend %actor% Group Armor
  endif
endif
~
#473
Wand clarification~
1 m 100
journal quest quest-journal~
if %arg% == wand
  set sorcererclasses Sorcerer Cryomancer Pyromancer Illusionist Necromancer
  if %sorcererclasses% /= %actor.class%
    return 0
    osend %actor% Please specify:
    osend %actor% Acid Wand
    osend %actor% Air Wand
    osend %actor% Fire Wand
    osend %actor% Ice Wand
  endif
endif
~
#474
Ice clarification~
1 m 100
journal quest quest-journal~
if %arg% == ice
  set sorcererclasses Sorcerer Cryomancer Pyromancer Illusionist Necromancer
  if %sorcererclasses% /= %actor.class%
    return 0
    osend %actor% Please specify:
    osend %actor% Ice Wand
    if %actor.class% /= Cryomancer
      if %actor.level% >= 50
        osend %actor% Wall of Ice
      endif
      if %actor.level% >= 85
        osend %actor% Ice Shards
      endif
    endif
  endif
endif
~
#475
Group clarification~
1 m 100
journal quest quest-journal~
if %arg% == group
  if %actor.level% >= 50 && (%actor.class% /= Priest || %actor.class% /= Cleric || %actor.class% /= Diabolist)
    return 0
    osend %actor% Please specify:
    osend %actor% Group Heal
    if %actor.class% /= Priest || (%actor.level% >= 65 && %actor.class% /= Cleric)
      osend %actor% Group Armor
    endif
    if (%actor.level% >= 65 && %actor.class% /= Cleric)
      osend %actor% Group Recall
    endif
  endif
endif
~
#476
Major clarification~
1 m 100
journal quest quest-journal~
if %arg% == major
  set relocateclasses Sorcerer Cryomancer Pyromancer
  if (%actor.level% >= 50 && %relocateclasses% /= %actor.class%) || (%actor.level% >= 65 && %actor.class% /= Bard)
    return 0
    osend %actor% Please specify:
    if %actor.level% >= 50 && %relocateclasses% /= %actor.class%
      osend %actor% Major Globe
    endif
    if %actor.level% >= 65 && %actor.class% /= Bard
      osend %actor% Major Paralysis
    endif
  endif
endif
~
#477
Guild/Phase clarification~
1 m 100
journal quest quest-journal~
if %arg% == guild || %arg% == phase || %arg% == guild phase armor || %arg% == guild armor || %arg% == phase armor || %arg% == phase_armor
  return 0
  osend %actor% Please specify:
  osend %actor% Guild Armor Phase One
  osend %actor% Guild Armor Phase Two
  osend %actor% Guild Armor Phase Three
endif
~
#478
Doom clarification~
1 m 100
journal quest quest-journal~
if %arg% == doom
  if (%actor.level% >= 75 && %actor.class% /= Druid) || %actor.level% >= 85
    return 0
    osend %actor% Please Specify:
    if %actor.level% >= 75 && %actor.class% /= Druid
      osend %actor% Creeping Doom
    endif
    if %actor.level% >= 85
      osend %actor% The Planes of Doom
    endif
  endif
endif
~
#479
Wall clarification~
1 m 100
journal quest quest-journal~
if %arg% == wall
  if %actor.level% >= 50 && (%actor.class% /= Cryomancer || %actor.class% /= Illusionist || %actor.class% /= Bard)
    return 0
    osend %actor% Please specify:
    if %actor.class% /= Cryomancer
      osend %actor% Wall of Ice
    endif
    if %actor.class% /= Illusionist || %actor.class% /= Bard
      osend %actor% Illusory Wall
    endif
  endif
endif
~
#480
Eye clarification~
1 m 100
journal quest quest-journal~
if %arg% == eye
  set hunterclasses Warrior Ranger Berserker Mercenary
  if (%actor.level% >= 75 && %actor.class% /= Sorcerer) || (%actor.level% >= 10 && %hunterclasses% /= %actor.class%)
    return 0
    osend %actor% Please specify:
    if %actor.level% >= 75 && %actor.class% /= Sorcerer
      osend %actor% Wizard Eye
    endif
    if %actor.level% >= 10 && %hunterclasses% /= %actor.class%
      osend %actor% Eye of the tiger
    endif
  endif
endif
~
#481
Gate clarification~
1 m 100
journal quest quest-journal~
if %arg% == gate
    if %actor.level% >= 75 && (%actor.class% /= Priest || %actor.class% /= Diabolist)
    return 0
    osend %actor% Please specify:
    if %actor.class% /= Priest
      osend %actor% Heavens Gate
    endif
    if %actor.class% /= Diabolist
      osend %actor% Hell Gate
    endif
  endif
endif
~
#482
Island, Isle clarification~
1 m 100
journal quest quest-journal~
if %arg% == island || %arg% == isle
  if %actor.level% >= 35
    return 0
    osend %actor% Please specify:
    if %actor.level% >= 35
      osend %actor% Liberate Fiery Island
    endif
    if %actor.level% >= 45
      osend %actor% Destroy the Cult of the Griffin
    endif
  endif
endif
~
#483
Sacred clarification~
1 m 100
journal quest quest-journal~
if %arg% == sacred
  if %actor.level% >= 25
    return 0
    osend %actor% Please specify:
    osend %actor% Infiltrate the Sacred Haven
    if %actor.level% >= 50
      osend %actor% The Great Rite
    endif
  endif
endif
~
#484
Major Paralysis progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= major paralysis || %arg% /= paralysis
  return 0
  osend %actor% This quest is not yet available in game.
  osend %actor% Petition the gods for a quest.
endif
~
#485
Monk Chants progress journal~
1 m 100
journal quest quest-journal~
if %actor.class% /= Monk
  if %arg% /= tremors || %arg% /= tremors of saint augustine || %arg% /= tremors_of_saint_augustine
    set read yes
    set title Tremors of Saint Augustine
    set level 30
    set minstage 1
    set clue1 a book surrounded by trees and shadows.
    set clue2 in a place that is both natural and urban, serenely peaceful and profoundly sorrowful.
  elseif %arg% /= tempest || %arg% /= tempest of saint augustine || %arg% /= tempest_of_saint_augustine
    set read yes
    set title Tempest of Saint Augustine
    set level 40
    set minstage 2
    set clue1 a scroll, dedicated to this particular chant, guarded by a creature of the same elemental affinity.
    set clue2 on the peak of Urchet Pass.
  elseif %arg% /= blizzards || %arg% /= blizzards of saint augustine || %arg% /= blizzards_of_saint_augustine
    set read yes
    set title Blizzards of Saint Augustine
    set level 50
    set minstage 3
    set clue1 a book held by a master who in turn is a servant of a beast of winter.
    set clue2 in a temple shrouded in mists.
  elseif %arg% /= aria || %arg% /= dissonance || %arg% /= aria of dissonance
    set read yes
    set title Aria of Dissonance
    set level 60
    set minstage 4
    set clue1 a book on war, held by a banished war god.
    set clue2 in a dark cave before a blasphemous book, near an unholy fire.
  elseif %arg% /= apocalyptic || %arg% /= Anthem
    set read yes
    set title Apocalyptic Anthem
    set level 75
    set minstage 5
    set clue1 a scroll where illusion is inscribed over and over held by a brother who thirsts for escape.
    set clue2 in a chapel of the walking dead.
  elseif %arg% /= fires || %arg% /= fires of saint augustine || %arg% /= fires_of_saint_augustine
    set read yes
    set title Fires of Saint Augustine
    set level 80
    set minstage 6
    set clue1 a scroll of curses, carried by children of air in a floating fortress.
    set clue2 at an altar dedicated to fire's destructive forces.
  elseif %arg% /= seed || %arg% /= destruction || %arg% /= seed of destruction
    set read yes
    set title Seed of Destruction
    set level 99
    set minstage 7
    set clue1 the eye of one caught in an eternal feud.
    set clue2 at an altar deep in the outer realms surrounded by those who's vengeance was never satisfied.
  endif
  if %read% == yes
    return 0
    set chantstage %actor.quest_stage[monk_chants]%
    set visionstage %actor.quest_stage[monk_vision]%
    set master %get.mob_shortdesc[5308]%
    osend %actor% &2&b&u%title%&0
    osend %actor% Minimum Level: %level%
    if %chantstage% > %minstage%
      set status Completed!
    elseif %chantstage% == %minstage% && (%chantstage% < (%visionstage% - 2))
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.level% >= %level% && %chantstage% == %minstage%
      if (%chantstage% >= (%visionstage% - 2))
        osend %actor% You must walk further along the Way in service of Balance first.
        halt
      endif
      osend %actor% You are looking for %clue1%
      osend %actor% Take it and &6&b[meditate]&0 %clue2%
    elseif %actor.level% >= %level% && !%chantstage%
      osend %actor% Ask %master% about &6&b[chants]%0 to get started.
    endif
  endif
endif
~
#486
Infernal Weaponry progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= trident || %arg% /= hell trident || %arg% /= hell_trident || %arg% /= Infernal Weaponry || %arg% /= infernal || %arg% /= weaponry || %arg% /= infernal_weaponry
  if %actor.class% /= Diabolist
    return 0
    osend %actor% &2&b&uInfernal Weaponry&0
    osend %actor% Weapons of the lower realms await a diabolist dedicated enough to claim them.
    set hellstage %actor.quest_stage[hell_trident]%
    if !%hellstage%
      set minlevel 35
    elseif %hellstage% == 1
      set minlevel 65
    elseif %hellstage% == 2
      set minlevel 90
    endif
    if !%actor.has_completed[hell_trident]%
      osend %actor% Minimum Level: %minlevel%
    endif
    if %actor.has_completed[hell_trident]%
      set status Completed!
    elseif %hellstage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.quest_stage[hell_trident]%
      set job1 %actor.quest_variable[hell_trident:helltask1]%
      set job2 %actor.quest_variable[hell_trident:helltask2]%
      set job3 %actor.quest_variable[hell_trident:helltask3]%
      set job4 %actor.quest_variable[hell_trident:helltask4]%
      set job5 %actor.quest_variable[hell_trident:helltask5]%
      set job6 %actor.quest_variable[hell_trident:helltask6]%
      switch %hellstage%
        case 1
          set master %get.mob_shortdesc[6032]%
          set gem 55662
          set pl_word angels
          set word angel
          set spell2 Banish
          set spell1 Hellfire and Brimstone
          set task6 assisted Vilekka Kar'Shezden
          set task6do &3&bAssist the High Priestess of Lolth in hunting down and destroying the heretics of her Goddess.&0
          break
        case 2
          set master %get.mob_shortdesc[12526]%
          set gem 55739
          set pl_word ghaeles, solars, or seraphs
          set word ghaele, solar, or seraph
          set spell1 Resurrect
          set spell2 Hell Gate
          set task6 Defeated the Undead Prince
          set task6do find one long-buried and branded an infidel.  &3&bFinish his undying duel for him&0 as a sacrifice of honor.
          break
        default
          if !%actor.has_completed[hell_trident]%
            osend %actor% Only someone mighty enough to claim the ancient truthstone and present it unto the leader of chaos may be given the dark powers of hell.  These diabolists will be rewarded for their unquenchable devotion by the dark gods themselves!
            halt
          endif
      done
      if %actor.level% >= %minlevel%
        osend %actor% Quest Master: %master%
        if !%actor.quest_variable[hell_trident:greet]%
          osend %actor% Speak with %master%.
          halt
        else
          if %job1% || %job2% || %job3% || %job4% || %job5% || %job6%
            osend %actor% You have done the following:
            if %job1%
              osend %actor% - attacked 666 times
            endif
            if %job2%
              osend %actor% - slayed 6 %pl_word%
            endif
            if %job3%
              osend %actor% - found 6 %get.obj_shortdesc[%gem%]%
            endif
            if %job5%
              osend %actor% - learned %spell2%
            endif
            if %job4%
              osend %actor% - learned %spell1%
            endif
            if %job6%
              osend %actor% - %task6%
            endif
            osend %actor% &0  
          endif
          if %job1% && %job2% && %job3% && %job4% && %job5% && %job6%
            osend %actor% Give %master% your trident to finalize the pact.
          else
            osend %actor% You must still:
            if !%job1%
              eval remaining 666 - %actor.quest_variable[hell_trident:attack_counter]%
              if %remaining% > 1
                osend %actor% - &3&battack %remaining% more times with your trident&0
              else 
                osend %actor% - &3&battack %remaining% more time with your trident&0
              endif
            endif
            if !%job2%
              eval kills 6 - %actor.quest_variable[hell_trident:celestials]%
              if %kills% > 1
                osend %actor% - &3&bslay %kills% more %pl_word%&0
              else
                osend %actor% - &3&bslay %kills% more %word%&0
              endif
            endif
            if !%job3%
              eval gems 6 - %actor.quest_variable[hell_trident:gems]%
              if %gems% > 1
                osend %actor% - &3&bfind %gems% more %get.obj_pldesc[%gem%]%&0
              else
                osend %actor% - &3&bfind %gems% more %get.obj_noadesc[%gem%]%&0
              endif
            endif
            if !%job5%
              osend %actor% - &3&blearn %spell2%&0
            endif
            if !%job4%
              osend %actor% - &3&blearn %spell1%&0
            endif
            if !%job6%
              osend %actor% - %task6do%
            endif
          endif
        endif
      endif
    endif
  endif
endif
~
#487
Hell clarification~
1 m 100
journal quest quest-journal~
if %arg% == hell
  if %actor.class% /= Diabolist
    return 0
    osend %actor% Please specify:
    osend %actor% Hellfire and Brimstone
    osend %actor% Infernal Weaponry
    osend %actor% Hell Gate
  endif
endif
~
#488
Hearthsong progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= Hearthsong || %arg% /= hearth
  return 0
  osend %actor% This quest is not yet available in game.
  osend %actor% Petition the gods for a quest.
endif
~
#489
Crown of Madness progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= crown || %arg% /= madness || %arg% /= crown of madness
  return 0
  osend %actor% This quest is not yet available in game.
  osend %actor% Petition the gods for a quest.
endif
~
#490
Enrapture progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= entrapture
  return 0
  osend %actor% This quest is not yet available in game.
  osend %actor% Petition the gods for a quest.
endif
~
#491
Greater Displacement progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= greater displacement || %arg% /= displacement
  return 0
  osend %actor% This quest is not yet available in game.
  osend %actor% Petition the gods for a quest.
endif
~
#492
Major Sanctuary progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= major Sanctuary || %arg% /= major sanc
  return 0
  osend %actor% This quest is not yet available in game.
  osend %actor% Petition the gods for a quest.
endif
~
#493
Greater Sanctuary~
1 m 100
journal quest quest-journal~
if %arg% /= greater sanctuary || %arg% /= greater sanc
  return 0
  osend %actor% This quest is not yet available in game.
  osend %actor% Petition the gods for a quest.
endif
~
#494
greater clarification~
1 m 100
journal quest quest-journal~
if %arg% == greater
  return 0
  osend %actor% Please specify:
  if %actor.class% /= ranger
    osend %actor% Greater Displacement
  endif
  if %actor.class% /= cleric || %actor.class% /= priest || %actor.class% /= diabolist
    osend %actor% Greater Sanctuary
  endif
endif
~
#495
sanctuary clarification~
1 m 100
journal quest quest-journal~
if %arg% == sanctuary || %arg% == sanc
  set majorsancclasses Cleric Priest Diabolist Paladin Anti-Paladin
  set greatersancclasses Cleric Priest Diabolist
  if %majorsancclasses% /= %actor.class%
    return 0
    osend %actor% Please specify:
    if %actor.level% >= 75
      osend %actor% Major Sanctuary
    endif
    if %actor.level% >= 85 && %greatersancclasses% /= %actor.class%
      osend %actor% Greater Sanctuary
    endif
  endif
endif
~
#496
Treasure Hunter progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= treasure || %arg% /= hunter || %arg% /= treasure hunter
  return 0
  set stage %actor.quest_stage[treasure_hunter]%
  set master %get.mob_shortdesc[5307]%
  set hunt %actor.quest_variable[treasure_hunter:hunt]%
  set target1 %actor.quest_variable[treasure_hunter:treasure1]%
  osend %actor% &2&b&uTreasure Hunt&0
  switch %stage%
    case 1
      set treasure1 a singing chain
      break
    case 2
      set treasure1 a true fire ring
      break
    case 3
      set treasure1 a sandstone ring
      break
    case 4
      set treasure1 a crimson-tinged electrum hoop
      break
    case 5
      set treasure1 a Rainbow Shell
      break
    case 6
      set treasure1 the Stormshield
      break
    case 7
      set treasure1 the Snow Leopard Cloak
      break
    case 8
      set treasure1 a coiled rope ladder
      break
    case 9
      set treasure1 a glowing phoenix feather
      break
    case 10
      set treasure1 a piece of sleet armor
      break
  done
  if %stage% == 1 || !%stage%
    set level 1
  else
    eval level (%stage% - 1) * 10
  endif
  if !%actor.has_completed[treasure_hunter]%
    osend %actor% Minimum Level: %level%
  endif
  if %actor.has_completed[treasure_hunter]%
    set status Completed!
  elseif %stage%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  if %stage% > 0 && !%actor.has_completed[bounty_hunt]%
    if %actor.level% >= %level%
      osend %actor% Quest Master: %master%
      if %hunt% == running || %hunt% == found || %hunt% == returned
        osend %actor% You have an order to find %treasure1%.
        if %actor.quest_variable[treasure_hunter:hunt]% == found
          osend %actor% You have found the treasure.
          osend %actor% Return it and your order to Honus for your reward!
        elseif %actor.quest_variable[treasure_hunter:hunt]% == returned
          osend %actor% You have returned the treasure to Honus.
          osend %actor% Return your order to him for your reward!
        endif
      endif
    endif
  endif
endif
~
#497
Rogue Cloak progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= cloak || %arg% /= dagger || %arg% /= cloak and dagger || %arg% /= rogue_cloak
  if %actor.class% /= Rogue || %actor.class% /= Bard || %actor.class% != Thief
    return 0
    set huntstage %actor.quest_stage[treasure_hunter]%
    set cloakstage %actor.quest_stage[rogue_cloak]%
    set job1 %actor.quest_variable[rogue_cloak:cloaktask1]%
    set job2 %actor.quest_variable[rogue_cloak:cloaktask2]%
    set job3 %actor.quest_variable[rogue_cloak:cloaktask3]%
    set job4 %actor.quest_variable[rogue_cloak:cloaktask4]%
    osend %actor% &2&b&uCloak and Dagger&0
    if !%cloakstage%
      set level 10
    else
      eval level %cloakstage% * 10
    endif
    if !%actor.has_completed[rogue_cloak]%
      osend %actor% Minimum Level: %level%
    endif
    if %actor.has_completed[rogue_cloak]%
      set status Completed!
    elseif %cloakstage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.level% >= %level%
      if %cloakstage% == 0
        osend %actor% You must &6&b[hunt]&0 down more treasure first.
        halt
      elseif (%cloakstage% >= %huntstage%) && !%actor.has_completed[treasure_hunter]%
        osend %actor% Find some more treasures before you seek another promotion.
        halt
      endif
      switch %cloakstage%
        case 1
          set cloak 58801
          set gem 55585
          set place A Storage Room
          set hint in the house on the hill.
          break
        case 2
          set cloak 17307
          set gem 55593
          set place A Small Alcove
          set hint in the holy library.
          break
        case 3
          set cloak 10308
          set gem 55619
          set place either Treasure Room
          set hint in the paladin fortress.
          break
        case 4
          set cloak 12325
          set gem 55659
          set place The Treasure Room
          set hint beyond the Tower in the Wastes.
          break
        case 5
          set cloak 43022
          set gem 55663
          set place Treasury
          set hint in the ghostly fortress.
          break
        case 6
          set cloak 23810
          set gem 55674
          set place either Treasure Room with a chest
          set hint lost in the sands.
          break
        case 7
          set cloak 51013
          set gem 55714
          set place Mesmeriz's Secret Treasure Room
          set hint hidden deep underground.
          break
        case 8
          set cloak 58410
          set gem 55740
          set place Treasure Room
          set hint sunk in the swamp.
          break
        case 9
          set cloak 52009
          set gem 55741
          set place Treasure Room
          set hint buried with an ancient king.
          break
      done
      eval attack %cloakstage% * 100
      if %job1% || %job2% || %job3% || %job4% 
        osend %actor% You've done the following:
        if %job1%
          osend %actor% - attacked %attack% times
        endif
        if %job2%
          osend %actor% - found %get.obj_shortdesc[%cloak%]%
        endif
        if %job3%
          osend %actor% - found %get.obj_shortdesc[%gem%]%
        endif
        if %job4%
          osend %actor% - searched in %place%
        endif
      endif
      osend %actor% &0 
      osend %actor% You need to:
      if %job1% && %job2% && %job3% && %job4%
        osend %actor% Give Honus your old cloak.
        halt
      endif
      if !%job1%
        eval remaining %attack% - %actor.quest_variable[rogue_cloak:attack_counter]%
        osend %actor% - attack &3&b%remaining%&0 more times while wearing your cloak.
      endif
      if !%job2%
        osend %actor% - find &3&b%get.obj_shortdesc[%cloak%]%&0
      endif
      if !%job3%
        osend %actor% - find &3&b%get.obj_shortdesc[%gem%]%&0
      endif
      if !%job4%
        osend %actor% - &3&bsearch&0 in a place called "&3&b%place%&0".
        osend %actor%&0   It's &3&b%hint%&0
      endif
    endif
  endif
endif
~
#498
Group Recall progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= group recall
  if %actor.level% >= 65 && %actor.class% /= Cleric
    return 0
    osend %actor% This quest is not yet available in game.
    osend %actor% Petition the gods for a quest.
  endif
endif
~
#499
Eldoria variable load~
1 m 100
journal quest quest-journal~
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
global vnum_gem_3bl_cap vnum_gem_3bl_neck vnum_gem_3bl_arm vnum_gem_3bl_wrist vnum_gem_3bl_gloves vnum_gem_3bl_jerkin vnum_gem_3bl_robe vnum_gem_3bl_belt vnum_gem_3bl_legs vnum_gem_3bl_boots vnum_gem_3bl_mask vnum_gem_3bl_symbol vnum_gem_3bl_staff vnum_gem_3bl_ssword vnum_gem_3bl_whammer vnum_gem_3bl_flail vnum_gem_3bl_shiv vnum_gem_3bl_lsword vnum_gem_3bl_smace vnum_gem_3bl_light vnum_gem_3bl_food vnum_gem_3bl_drink vnum_3bl_cap vnum_3bl_neck vnum_3bl_arm vnum_3bl_wrist vnum_3bl_gloves vnum_3bl_jerkin vn
m_3bl_belt vnum_3bl_legs vnum_3bl_boots vnum_3bl_mask vnum_3bl_robe vnum_3bl_symbol vnum_3bl_staff vnum_3bl_ssword vnum_3bl_whammer vnum_3bl_flail vnum_3bl_shiv vnum_3bl_lsword vnum_3bl_smace vnum_3bl_light vnum_3bl_food vnum_3bl_drink
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
global vnum_gem_3eg_cap vnum_gem_3eg_neck vnum_gem_3eg_arm vnum_gem_3eg_wrist vnum_gem_3eg_gloves vnum_gem_3eg_jerkin vnum_gem_3eg_robe vnum_gem_3eg_belt vnum_gem_3eg_legs vnum_gem_3eg_boots vnum_gem_3eg_mask vnum_gem_3eg_symbol vnum_gem_3eg_staff vnum_gem_3eg_ssword vnum_gem_3eg_whammer vnum_gem_3eg_flail vnum_gem_3eg_shiv vnum_gem_3eg_lsword vnum_gem_3eg_smace vnum_gem_3eg_light vnum_gem_3eg_food vnum_gem_3eg_drink vnum_3eg_cap vnum_3eg_neck vnum_3eg_arm vnum_3eg_wrist vnum_3eg_gloves vnum_3eg_jerkin vn
m_3eg_belt vnum_3eg_legs vnum_3eg_boots vnum_3eg_mask vnum_3eg_robe vnum_3eg_symbol vnum_3eg_staff vnum_3eg_ssword vnum_3eg_whammer vnum_3eg_flail vnum_3eg_shiv vnum_3eg_lsword vnum_3eg_smace vnum_3eg_light vnum_3eg_food vnum_3eg_drink
~
#500
Elemental Chaos progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= elemental_chaos || %arg% /= elemental || %arg% /= chaos || %arg% /= elemental chaos
  return 0
  set stage %actor.quest_stage[elemental_chaos]%
  set master %get.mob_shortdesc[5308]%
  set hunt %actor.quest_variable[elemental_chaos:bounty]%
  set target1 %actor.quest_variable[elemental_chaos:target1]%
  set target2 %actor.quest_variable[elemental_chaos:target2]%
  set target3 %actor.quest_variable[elemental_chaos:target3]%
  osend %actor% &2&b&uElemental Chaos&0
  switch %actor.quest_stage[elemental_chaos]% 
    case 1
      set mission investigate the news of an imp and dispatch it if you find one
      break
    case 2
      set mission silence the seductive song of the Leading Player
      break
    case 3
      set mission destroy the Chaos and the cult worshipping it
      break
    case 4
      set mission undertake the vision quest from the shaman in Three-Falls Canyon and defeat whatever awaits at the end
      break
    case 5
      set mission dispatch the Fangs of Yeenoghu.  Be sure to destroy all of them
      set victim1 the shaman Fang of Yeenoghu
      set victim2 the necromancer Fang of Yeenoghu
      set victim3 the diabolist Fang of Yeenoghu
      break
    case 6
      set mission extinguish the fire elemental lord who serves Krisenna
      break
    case 7
      set mission stop the acolytes in the Cathedral of Betrayal
      break
    case 8
      set mission destroy Cyprianum the Reaper in the heart of his maze
      break
    case 9
      set mission banish the Chaos Demon in Frost Valley
      break
    case 10
      set mission slay one of the Norhamen
  done
  if %stage% == 1
    set level 1
  else
    eval level (%stage% - 1) * 10
  endif
  if !%actor.has_completed[dragon_slayer]%
    osend %actor% Minimum Level: %level%
  endif
  if %actor.has_completed[dragon_slayer]%
    set status Completed!
  elseif %stage%
    set status In Progress
  else
    set status Not Started
  endif
  osend %actor% &6Status: %status%&0&_
  if %stage% > 0 && !%actor.has_completed[elemental_chaos]]%
    if %actor.level% >= %level%
      osend %actor% Quest Master: %master%
      if %hunt% == running || %hunt% == dead
        osend %actor% You have a mission to %mission%.
        if %hunt% == dead
          osend %actor% You have completed the mission.
          osend %actor% Return it to %master%!
        elseif %stage% == 5
          if %target1%
            osend %actor% You have scratched %victim1% off the list.
          endif
          if %target2%
            osend %actor% You have scratched %victim2% off the list.
          endif
          if %target3%
            osend %actor% You have scratched %victim3% off the list.
          endif
        endif
      else
        osend %actor% Report to Hakujo for further news.
      endif
    endif
  endif
endif
~
#501
Enlightenment progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= enlightenment || %arg% /= monk vision || %arg% /= monk_vision
  if %actor.class% /= Monk
    return 0
    set missionstage %actor.quest_stage[elemental_chaos]%
    set visionstage %actor.quest_stage[monk_vision]%
    set master %get.mob_shortdesc[5308]%
    set job1 %actor.quest_variable[monk_vision:visiontask1]%
    set job2 %actor.quest_variable[monk_vision:visiontask2]%
    set job3 %actor.quest_variable[monk_vision:visiontask3]%
    set job4 %actor.quest_variable[monk_vision:visiontask4]%
    osend %actor% &2&b&uEnlightenment&0
    if !%visionstage%
      set level 10
    else
      eval level %visionstage% * 10
    endif
    if !%actor.has_completed[monk_vision]%
      osend %actor% Minimum Level: %level%
    endif
    if %actor.has_completed[monk_vision]%
      set status Completed!
    elseif %visionstage%
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.level% >= %level%
      if %visionstage% == 0
        osend %actor% You must undertake a &6&b[mission]&0 in service of Balance first.
        halt
      elseif (%visionstage% >= %missionstage%) && !%actor.has_completed[elemental_chaos]%
        osend %actor% You must walk further along the Way in service of Balance first.
        halt
      endif
      switch %visionstage%
        case 1
          set book 59006
          set gem 55582
          set room %get.room[4328]%
          set place %room.name%
          set hint in a place to perform.
          break
        case 2
          set book 18505
          set gem 55591
          set room %get.room[58707]%
          set place %room.name%
          set hint near a sandy beach.
          break
        case 3
          set book 8501
          set gem 55623
          set room %get.room[18597]%
          set place %room.name%
          set hint in a cloistered library.
          break
        case 4
          set book 12532
          set gem 55655
          set room %get.room[58102]%
          set place %room.name%
          set hint on Hakujo's home island.
          break
        case 5
          set book 16209
          set gem 55665
          set room %get.room[16057]%
          set place %room.name%
          set hint in the ghostly fortress.
          break
        case 6
          set book 43013
          set gem 55678
          set room %get.room[59054]%
          set place %room.name%
          set hint in the fortress of the zealous.
          break
        case 7
          set book 53009
          set gem 55710
          set room %get.room[49079]%
          set place %room.name%
          set hint off-shore of the island of great beasts.
          break
        case 8
          set book 58415
          set gem 55722
          set room %get.room[11820]%
          set place %room.name%
          set hint beyond the Blue-Fog Trail.
          break
        case 9
          set book 58412
          set gem 55741
          set room %get.room[52075]%
          set place %room.name%
          set hint in the shattered citadel of Templace.
      done
      eval attack %visionstage% * 100
      if %job1% || %job2% || %job3% || %job4% 
        osend %actor% You've done the following:
        if %job1%
          osend %actor% - attacked %attack% times
        endif
        if %job2%
          osend %actor% - found %get.obj_shortdesc[%gem%]%
        endif
        if %job3%
          osend %actor% - found %get.obj_shortdesc[%book%]%
        endif
        if %job4%
          osend %actor% - read in %place%
        endif
      endif
      osend %actor% &0  
      osend %actor% You need to:
      if %job1% && %job2% && %job3% && %job4%
        osend %actor% Give %master% your current vision mark.
        halt
      endif
      if !%job1%
        eval remaining %attack% - %actor.quest_variable[monk_vision:attack_counter]%
        osend %actor% - attack &9&b%remaining%&0 more times while wearing your vision mark.
      endif
      if !%job4%
        osend %actor% - find &3&b%get.obj_shortdesc[%gem%]%&0 and &3&b%get.obj_shortdesc[%book%]%&0 and &3&bread&0 in a place called "&3&b%place%&0".
        osend %actor%&0   It's &2&b%hint%&0
      else
        if !%job2%
          osend %actor% - bring %master% &3&b%get.obj_shortdesc[%gem%]%&0
        endif
        if !%job3%
          osend %actor% - bring %master% &3&b%get.obj_shortdesc[%book%]%&0
        endif
      endif
    endif
  endif
endif
~
#502
**UNUSED**~
1 m 100
journal quest quest-journal~
if %arg% /= tremors || %arg% /= tremors of saint augustine || %arg% /= tremors_of_saint_augustine
  if %actor.class% /= Monk
    return 0
    set chantstage %actor.quest_stage[monk_chants]%
    set visionstage %actor.quest_stage[monk_vision]%
    set master %get.mob_shortdesc[5308]%
    set level 30
    osend %actor% &2&b&uTremors of Saint Augustine&0
    osend %actor% Minimum Level: %level%
    if %chantstage% > 1
      set status Completed!
    elseif %chantstage% == 1
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.level% >= %level% && %chantstage% == 1
      if (%chantstage% >= (%visionstage% - 1))
        osend %actor% You must walk further along the Way in service of Balance first.
        halt
      endif
      osend %actor% You are looking for a book surrounded by trees and shadows.
      osend %actor% Take it and &6&b[meditate]%&0 in a place that is both natural and urban, serenely peaceful and profoundly sorrowful.
    elseif %actor.level% >= %level% && !%chantstage%
      osend %actor% Ask %master% about &6&b[chants]%0 to get started.
    endif
  endif
endif
~
#503
Tempest of Saint Augustine progress journal~
1 m 100
journal quest quest-journal~
if %arg% /= tempest || %arg% /= tempest of saint augustine || %arg% /= tempest_of_saint_augustine
  if %actor.class% /= Monk
    return 0
    set chantstage %actor.quest_stage[monk_chants]%
    set visionstage %actor.quest_stage[monk_vision]%
    set master %get.mob_shortdesc[5308]%
    set level 40
    osend %actor% &2&b&uTempest of Saint Augustine&0
    osend %actor% Minimum Level: %level%
    if %chantstage% > 2
      set status Completed!
    elseif %chantstage% == 2
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.level% >= %level% && %chantstage% == 2
      if (%chantstage% >= (%visionstage% - 1))
        osend %actor% You must walk further along the Way in service of Balance first.
        halt
      endif
      osend %actor% You are looking for a scroll, dedicated to this particular chant, guarded by a creature of the same elemental affinity.
      osend %actor% Take it and &6&b[meditate]%&0 on the peak of Urchet Pass.
    endif
  endif
endif
~
#504
**UNUSED**~
1 m 100
journal quest quest-journal~
if %arg% /= blizzards || %arg% /= blizzards of saint augustine || %arg% /= blizzards_of_saint_augustine
  if %actor.class% /= Monk
    return 0
    set chantstage %actor.quest_stage[monk_chants]%
    set visionstage %actor.quest_stage[monk_vision]%
    set master %get.mob_shortdesc[5308]%
    set level 50
    osend %actor% &2&b&uBlizzards of Saint Augustine&0
    osend %actor% Minimum Level: %level%
    if %chantstage% > 3
      set status Completed!
    elseif %chantstage% == 3
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.level% >= %level% && %chantstage% == 3
      if (%chantstage% >= (%visionstage% - 1))
        osend %actor% You must walk further along the Way in service of Balance first.
        halt
      endif
      osend %actor% You are looking for a book held by a master who in turn is a servant of a beast of winter.
      osend %actor% Take it and &6&b[meditate]%&0 in a temple shrouded in mists.
    endif
  endif
endif
~
#505
Saint Augustine clarification~
1 m 100
journal quest quest-journal~
if %arg% == saint || %arg% == augustine || %arg% == saint augustine
  if %actor.level% >= 30 && %actor.class% /= Monk
    return 0
    osend %actor% Please specify:
    if %actor.level% >= 30
      osend %actor% Tremors of Saint Augustine
    endif
    if %actor.level% >= 40
      osend %actor% Tempest of Saint Augustine
    endif
    if %actor.level% >= 50
      osend %actor% Blizzards of Saint Augustine
    endif
    if %actor.level% >= 80
      osend %actor% Fires of Saint Augustine
    endif
  endif
endif
~
#506
**UNUSED**~
1 m 100
journal quest quest-journal~
if %arg% /= fires || %arg% /= fires of saint augustine || %arg% /= fires_of_saint_augustine
  if %actor.class% /= Monk
    return 0
    set chantstage %actor.quest_stage[monk_chants]%
    set visionstage %actor.quest_stage[monk_vision]%
    set master %get.mob_shortdesc[5308]%
    set level 80
    osend %actor% &2&b&uFires of Saint Augustine&0
    osend %actor% Minimum Level: %level%
    if %chantstage% > 6
      set status Completed!
    elseif %chantstage% == 6
      set status In Progress
    else
      set status Not Started
    endif
    osend %actor% &6Status: %status%&0&_
    if %actor.level% >= %level% && %chantstage% == 6
      if (%chantstage% >= (%visionstage% - 1))
        osend %actor% You must walk further along the Way in service of Balance first.
        halt
      endif
      osend %actor% You are looking for a scroll of curses, carried by children of air in a floating fortress.
      osend %actor% Take it and &6&b[meditate]%&0 at an altar dedicated to fire's destructive forces.
    endif
  endif
endif
~
$~
