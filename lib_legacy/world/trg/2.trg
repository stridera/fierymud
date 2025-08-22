#300
phase wand bigby research assistant greet~
0 h 100
~
if %self.aff_flagged[INVIS]%
  vis
endif
wait 2
set air %actor.quest_stage[air_wand]%
set airgreet %actor.quest_variable[air_wand:greet]%
set fire %actor.quest_stage[fire_wand]%
set firegreet %actor.quest_variable[fire_wand:greet]%
set ice %actor.quest_stage[ice_wand]%
set icegreet %actor.quest_variable[ice_wand:greet]%
set acid %actor.quest_stage[acid_wand]%
set acidgreet %actor.quest_variable[acid_wand:greet]%
if %air% > 2 || %fire% > 2 || %ice% > 2 || %acid% > 2 || %airgreet% || %firegreet% || %icegreet% || %acidgreet%
  say Ah, welcome back %actor.name%!
  wait 2
endif
if %actor.class% /= sorcerer || %actor.class% /= pyromancer || %actor.class% /= cryomancer || %actor.class% /= illusionist || %actor.class% /= necromancer
  if %actor.level% >= 10
    if (%actor.wearing[300]% || %actor.inventory[300]%) && %air% < 2 
      if !%air%
        quest start air_wand %actor%
      endif 
      quest variable air_wand %actor% greet 1
      set offer 1
    elseif %air% == 2
      set continue 1
    endif
    if (%actor.wearing[310]% || %actor.inventory[310]%) && %fire% < 2 
      if !%fire%
        quest start fire_wand %actor%
      endif 
      quest variable fire_wand %actor% greet 1
      set offer 1
    elseif %fire% == 2
      set continue 1
    endif
    if (%actor.wearing[320]% || %actor.inventory[320]%) && %ice% < 2 
      if !%ice%
        quest start ice_wand %actor%
      endif 
      quest variable ice_wand %actor% greet 1
      set offer 1
    elseif %ice% == 2
      set continue 1
    endif
    if (%actor.wearing[330]% || %actor.inventory[330]%) && %acid% < 2
      if !%acid%
        quest start acid_wand %actor%
      endif 
      quest variable acid_wand %actor% greet 1
      set offer 1
    elseif %acid% == 2
      set continue 1
    endif
    if %continue%
      say Have you been practicing and looking for the gems you need?
      wait 2
    endif
    if %offer%
      mecho %self.name% says, 'I see you have a new elemental wand!  I can &6&bupgrade&0 basic wands.'
      wait 2
    endif
    if %air% > 2 || %fire% > 2 || %ice% > 2 || %acid% > 2
      mecho %self.name% says, 'If you want to upgrade a wand again, I can point you in the right direction.  Just tell me which energy type you want to work on: &2acid&0, &7&bair&0, &1fire&0, or &4&bice&0.'
    endif
  endif
endif
~
#301
phase wand bigby research assistant speech~
0 d 100
wand earth air fire water upgrade lightning wind shock electricity smoldering burning cold ice frost acid tremors corrosion powerful yes~
wait 2
if %actor.class% /= sorcerer || %actor.class% /= pyromancer || %actor.class% /= cryomancer || %actor.class% /= illusionist || %actor.class% /= necromancer
  if %actor.level% >= 10
    switch %speech%
      case air
      case lightning
      case wind
      case shock
      case electricity
        set type air
        set wandgem 55577
        break
      case fire
      case smoldering
      case burning
        set type fire
        set wandgem 55575
        break
      case water
      case cold
      case ice
      case frost
        set type ice
        set wandgem 55574
        break
      case earth
      case acid
      case tremors
      case corrosion
        set type acid
        set wandgem 55576
        break
      default
        wait 2
        if %actor.quest_stage[%type%_wand]% < 2
          say Yeah, I can help you upgrade any basic wand!
          wait 1s
          mecho %self.name% says, 'Tell me what element you would like to improve.  You can say &2acid&0, &3&bair&0, &1fire&0, or &4&bice&0.'
        elseif %actor.quest_stage[%type%_wand]% == 2
          say Get some practice with the wand and bring me %get.obj_shortdesc[%wandgem%]%.
        elseif %actor.quest_stage[%type%_wand]% > 2
          mecho %self.name% says, 'I can't do anything more myself, but I can tell you where to go.  Tell me what element you would like to improve.  You can say &2acid&0, &7&bair&0, &1fire&0, or &4&bice&0.'
        endif
        halt
    done
    set stage %actor.quest_stage[%type%_wand]%
    wait 2
    if %actor.has_completed[%type%_wand]%
      mecho %self.name% says, 'It looks like you already have the most powerful weapon of %type% in existence!'
    elseif !%stage% || %stage% == 1
      if !%actor.quest_stage[%type%_wand]%
        quest start %type%_wand %actor%
      endif
      mecho %self.name% says, 'I can upgrade your %type% wand!  But what I will do is just the first step in a life-long journey.'
      wait 3s
      mecho %self.name% says, 'This step is relatively simple.  You can check your &6&b[wand progress]&0 with me as well.'
      wait 2s
      say First, let me see your wand.
    elseif %stage% == 2
      mecho %self.name% says, 'If you have the practice, give me %get.obj_shortdesc[%wandgem%]% and your current wand.'
    elseif %stage% > 2
      if %type% == air
        mecho %self.name% says, 'You'll be studying with a plethora of seers, mystics, and wisemen.  You can check your &6&b[wand progress]&0 as you go.'
        wait 1s
        if %stage% == 3
          say Speak with the old Abbot in the Abbey of St. George.
        elseif %stage% == 4
          say The keeper of a southern coastal tower will have advice for you.
        elseif %stage% == 5
          say A master of air near the megalith in South Caelia will be able to help next.
        elseif %stage% == 6
          say Seek out the warrior-witch at the center of the southern megalith.
        elseif %stage% == 7
          say She's hard to deal with, but the Seer of Griffin Isle should have some additional guidance for you.
        elseif %stage% == 8
          say In the diabolist's church is a seer who cannot see.  He's a good resource for this kind of work.
        elseif %stage% == 9
          say The guardian ranger of the Druid Guild in the Red City has some helpful crafting tips.
        elseif %stage% == 10
          say Silania will help you craft the finest of air weapons.
        endif
      elseif %type% == fire 
        mecho %self.name% says, 'You should seek out the most renowned pyromancers in the world.  You can check your &6&b[wand progress]&0 as you go.'
        wait 1s
        if %stage% == 3
          say The keeper of the temple to the dark flame out east will know what to do.
        elseif %stage% == 4
          say There's a fire master in the frozen north who likes to spend his time at the hot springs.
        elseif %stage% == 5
          say A master of fire near the megalith in South Caelia will be able to help next.
        elseif %stage% == 6
          say A seraph crafts with the power of the sun and sky.  It can be found in the floating fortress in South Caelia.
        elseif %stage% == 7
          say I hate to admit it, but Vulcera is your next crafter.  Good luck appeasing her though...
        elseif %stage% == 8
          say You're headed back to Fiery Island.  Crazy old McCabe can help you improve your staff further.
        elseif %stage% == 9
          say Seek out the one who speaks for the Sun near Anduin.  He can upgrade your staff.
        elseif %stage% == 10
          say Surely you've heard of Emmath Firehand.  He's the supreme artisan of fiery goods.  He can help you make the final improvements to your staff.
        endif
      elseif %type% == ice
        mecho %self.name% says, 'Masters of ice and water are highly varied in their professions.  You can check your &6&b[wand progress]&0 as you go.'
        wait 1s
        if %stage% == 3
          say The shaman near Three-Falls River has developed a powerful affinity for water from his life in the canyons.  Seek his advice.
        elseif %stage% == 4
          say Many of the best craftspeople aren't even mortal.  There is a water sprite of some renown deep in Anlun Vale.
        elseif %stage% == 5
          say A master of spirits in the far north will be able to help next.
        elseif %stage% == 6
          say Your next crafter is a distant relative of the Sunfire clan.  He's been squatting in a flying fortress for many months, trying to unlock its secrets.
        elseif %stage% == 7
          say You'll need the advice of a master ice sculptor.  One works regularly up in Mt. Frostbite.
        elseif %stage% == 8
          say There's another distant relative of the Sunfire clan who runs the hot springs near Ickle.  He's book smart and knows a thing or two about jewel crafting.
        elseif %stage% == 9
          say The guild guard for the Sorcerer Guild in Ickle has learned plenty of secrets from the inner sanctum.  Talk to him.
        elseif %stage% == 10
          say You must know Suralla Iceeye by now.  She's the master artisan of cold and ice.  She'll know how to make the final improvements to your staff.
        endif
      elseif %type% == acid
        mecho %self.name% says, 'Acid is the energy of earth.  The master earth crafters all belong to the ranger network that safeguards places around Caelia.  You can check your &6&b[wand progress]&0 as you go.
        wait 1s
        if %stage% == 3
          say First, seek the one who guards the eastern gates of Ickle.
        elseif %stage% == 4
          say The next two artisans dwell in the Rhell Forest south-east of Mielikki.
        elseif %stage% == 5
          say Your next crafter isn't exactly part of the ranger network...  It's not actually a person at all.  Find the treant in the Rhell forest and ask it for guidance.
        elseif %stage% == 6
          say The ranger who guards the massive necropolis near Anduin has wonderful insights on crafting with decay.
        elseif %stage% == 7
          say Your next guide may be hard to locate...  I believe they guard the entrance to a long-lost kingdom beyond a frozen desert.
        elseif %stage% == 8
          say Next, consult with another ranger who guards a place crawling with the dead.  The dwarf ranger in the iron hills will know how to help you.
        elseif %stage% == 9
          say The guard of the only known Ranger Guild in the world is also an excellent craftswoman.  Consult with her.
        elseif %stage% == 10
          say Your last guide is the head of the ranger network himself, Eleweiss.  He can help make the final improvements to your staff.
        endif
      endif
    endif
  else
    say Come back after you've gained some more experience.  I can help you then.
  endif
else
  say I'm sorry, only true wizards can use these weapons.
endif
~
#302
phase wands bigby research assistant receive~
0 j 100
300 310 320 330 55577 55575 55574 55576~
switch %object.vnum%
  case 300
    set type air
    set wandgem 55577
    set wand yes
    set next_vnum 301
    break
  case 310
    set type fire
    set wandgem 55575
    set wand yes
    set next_vnum 311
    break
  case 320
    set type ice
    set wandgem 55574
    set wand yes
    set next_vnum 321
    break
  case 330
    set type acid
    set wandgem 55576
    set wand yes
    set next_vnum 331
    break
  case 55577
    set type air
    set wandgem yes
    break
  case 55575
    set type fire
    set wandgem yes
    break
  case 55574
    set type ice
    set wandgem yes
    break
  case 55576
    set type acid
    set wandgem yes
    break
done
set times 50
* Is this a wand or gem?
if %wand% == yes || %wandgem% == yes
  * Are we on the crafting stage?
  if %actor.quest_stage[%type%_wand]% == 2
    if %wand% == yes
      if %actor.quest_variable[%type%_wand:wandtask1]% && %actor.quest_variable[%type%_wand:wandtask2]%
        quest advance %type%_wand %actor%
        quest variable %type%_wand %actor% greet 0
        quest variable %type%_wand %actor% attack_counter 0
        set num 1
        while %num% <= 5
          quest variable %type%_wand %actor% wandtask%num% 0
          eval num %num% + 1
        done
        wait 2
        say Fantastic work!  This is everything.
        wait 1s
        mecho %self.name% combines the materials to empower %object.shortdesc%!
        mjunk %object%
        wait 2s
        msend %actor% %self.name% presents you with a new wand!
        mload obj %next_vnum%
        give wand %actor%
        set expcap 10
        set expmod 690
        * Adjust exp award by class so all classes receive the same proportionate amount
        switch %actor.class%
          case Sorcerer
          case Pyromancer
          case Cryomancer
          case Illusionist
          case Bard
          * 120% of standard
              eval expmod (%expmod% + (%expmod% / 5))
              break
          case Necromancer
          * 130% of standard
              eval expmod (%expmod% + (%expmod% * 2) / 5)
              break
          default
              set expmod %expmod%
        done
        msend %actor% &3&bYou gain experience!&0
        eval setexp (%expmod% * 10)
        set loop 0
        while %loop% < 5
          mexp %actor% %setexp%
          eval loop %loop% + 1
        done
        wait 2s
        if %actor.level% < 20
          say In time, you'll be able to take another step.  Come back when you've seen some more of the world.
        else
          say You're ready to take the next step.
          wait 1s
          if %type% == air
            say Speak with the old Abbot in the Abbey of St. George.
          elseif %type% == fire
            say The keeper of the temple to the dark flame out east will know what to do.
          elseif %type% == ice
            say The shaman near Three-Falls River has developed a powerful affinity for water from his life in the canyons.  Seek his advice.
          elseif %type% == acid
            say First, seek the one who guards the eastern gates of Ickle.
          endif
        endif
      else
        return 0
        mecho %self.name% scrutinizes %object.shortdesc% with suspicion.
        wait 1s
        mecho %self.name% returns %object.shortdesc%.
        say You haven't finished all the tasks to prime the wand yet.
      endif
    elseif %wandgem% == yes
      quest variable %type%_wand %actor% wandtask2 1
      wait 2
      mjunk %object%
      say Yes, this is exactly what I need!
      wait 1s
      if %actor.quest_variable[%type%_wand:wandtask1]%
        say Give me the wand to improve.
      else
        say Keep practicing with your wand!
        wait 2
        say Come back when you've attacked %times% times.
      endif
    endif
  elseif %actor.quest_stage[%type%_wand]% < 2
    if %wand%
      wait 2
      if !%actor.quest_stage[%type%_wand]%
        quest start %type%_wand %actor%
        say I can upgrade your %type% wand!  But what I will do is just the first step in a life-long journey.
        wait 2s
        mecho %self.name% says, 'This step is relatively simple though.  You can check your &6&b[wand progress]&0 with me as well.'
        wait 1s
      endif
      quest advance %type%_wand %actor%
      nod
      say Yes, this is a good match for you.
      wait 1s
      say To upgrade your wand:
      mecho - attack &3&b%times%&0 times with it
      mecho - bring me &3&b%get.obj_shortdesc[%wandgem%]%&0
      wait 2s
      say Do that and I'll be able to improve it.
      give wand %actor%
    elseif %wandgem%
      return 0
      mecho %self.name% declines %object.shortdesc%.
      wait 1s
      say Before you give me anything else, give me the wand you wish to improve.
    endif
  else
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    say I've already upgraded your wand as much as I know how.
  endif
endif
~
#303
phase wands attack counter~
1 d 100
~
if %self.vnum% >= 300 && %self.vnum% <= 309
  set type air
  if %self.vnum% >= 301 && %self.vnum% <= 305
    set spell 'shocking grasp'
    eval bonus %self.vnum% - 301
    set level %self.level%
  else
    set spell 'lightning bolt'
    eval bonus %self.vnum% - 306
    eval level %self.level% - 50
  endif
elseif %self.vnum% >= 310 && %self.vnum% <= 319
  set type fire
  if %self.vnum% >= 311 && %self.vnum% <= 315
    set spell 'burning hands'
    eval bonus %self.vnum% - 311
    set level %self.level%
  else
    set spell 'fireball'
    eval bonus %self.vnum% - 316
    eval level %self.level% - 50
  endif
elseif %self.vnum% >= 320 && %self.vnum% <= 329
  set type ice
  if %self.vnum% >= 321 && %self.vnum% <= 325
    set spell 'chill touch'
    eval bonus %self.vnum% - 321
    set level %self.level%
  else
    set spell 'cone of cold'
    eval bonus %self.vnum% - 326  
    eval level %self.level% - 50
  endif
elseif %self.vnum% >= 330 && %self.vnum% <= 339
  set type acid
  if %self.vnum% >= 331 && %self.vnum% <= 335
    set spell 'writhing weeds'
    eval bonus %self.vnum% - 331
    set level %self.level%
  else
    set spell 'acid burst'
    eval bonus %self.vnum% - 336
    eval level %self.level% - 50
  endif
endif
if %actor.quest_stage[%type%_wand]% > 1
  if !%actor.quest_variable[%type%_wand:wandtask1]% 
    eval attack_increase %actor.quest_variable[%type%_wand:attack_counter]% + 1
    quest variable %type%_wand %actor.name% attack_counter %attack_increase%
    if %actor.quest_variable[%type%_wand:attack_counter]% >= (%actor.quest_stage[%type%_wand]% - 1) * 50
      quest variable %type%_wand %actor% wandtask1 1
      osend %actor% &3&bYou have perfected your bond with %self.shortdesc%!&0
    endif
  endif
endif
if %spell%
  eval chance 1 + %bonus%
  if %random.20% <= %chance%
    ocast %spell% %victim% %level%
  endif
endif
~
#304
phase wands give owner check~
1 i 100
~
if %self.vnum% >= 300 && %self.vnum% =< 309
  set energy air
elseif %self.vnum% >= 310 && %self.vnum% =< 319
  set energy fire
elseif %self.vnum% >= 320 && %self.vnum% =< 329
  set energy ice
elseif %self.vnum% >= 330 && %self.vnum% =< 339
  set energy acid
endif
switch %victim.vnum%
  case -1
    halt
    break
  case 3013
    if %self.vnum% == 300 || %self.vnum% == 310 || %self.vnum% == 320 || %self.vnum% == 330
      if !%actor.quest_stage[%energy%_wand]%
        quest start %energy%_wand %actor%
      endif
      halt
    else
      set refuse 1
    endif
    break
  case 18500
    set type air
    set wandstep 3
    break
  case 4126
    set type fire
    set wandstep 3
    break
  case 17806
    set type ice
    set wandstep 3
    break
  case 10056
    set type acid
    set wandstep 3
    break
  case 58601
    set type air
    set wandstep 4
    break
  case 10306
    set type fire
    set wandstep 4
    break
  case 2337
    set type ice
    set wandstep 4
    break
  case 62504
    set type acid
    set wandstep 4
    break
  case 12305
    set type air
    set wandstep 5
    break
  case 12304
    set type fire
    set wandstep 5
    break
  case 55013
    set type ice
    set wandstep 5
    break
  case 62503
    set type acid
    set wandstep 5
    break
  case 12302
    set type air
    set wandstep 6
    break
  case 23811
    set type fire
    set wandstep 6
    break
  case 23802
    set type ice
    set wandstep 6
    break
  case 47075
    set type acid
    set wandstep 6
    break
  case 49003
    set type air
    set wandstep 7
    break
  case 48105
    set type fire
    set wandstep 7
    break
  case 53316
    set type ice
    set wandstep 7
    break
  case 4017
    set type acid
    set wandstep 7
    break
  case 8515
    set type air
    set wandstep 8
    break
  case 48250
    set type fire
    set wandstep 8
    break
  case 10300
    set type ice
    set wandstep 8
    break
  case 48029
    set type acid
    set wandstep 8
    break
  case 6216
    set type air
    set wandstep 9
    break
  case 48412
    set type fire
    set wandstep 9
    break
  case 10012
    set type ice
    set wandstep 9
    break
  case 3549
    set type acid
    set wandstep 9
    break
  case 18581
    set type air
    set wandstep 10
    break
  case 5230
    set type fire
    set wandstep 10
    break
  case 55020
    set type ice
    set wandstep 10
    break
  case 16315
    set type acid
    set wandstep 10
    break
  default
    set type none
    set wandstep 0
done
if %actor.vnum% == -1
  if %type% != %energy%
    set refuse 1
  elseif %actor.quest_stage[%energy%_wand]% < %wandstep%
    set refuse 2
  endif
  if %refuse% == 1
    return 0
    osend %actor% You shouldn't give away something so precious!
  elseif %refuse% == 2
    return 0
    oecho %victim.name% refuses %self.shortdesc%.
    wait 2
    osend %actor% %victim.name% tells you, 'This isn't yours!  I can't help you properly improve with a %weapon% that doesn't belong to you.'
  endif
endif
~
#305
phase weapon owner wield check~
1 j 100
~
if %self.vnum% >= 300 && %self.vnum% <= 339
  if %self.vnum% >= 300 && %self.vnum% <= 309
    set type air
  elseif %self.vnum% >= 310 && %self.vnum% <= 319
    set type fire
  elseif %self.vnum% >= 320 && %self.vnum% <= 329
    set type ice
  elseif %self.vnum% >= 330 && %self.vnum% <= 339
    set type acid
  endif
  if %actor.quest_stage[%type%_wand] < (%self.level% / 10) + 2
    return 0
    osend %actor% This weapon is bound to someone else!
  else
    switch %self.vnum%
      case 300
      case 310
      case 320
      case 330
        if !%actor.quest_stage[%type%_wand]%
          quest start %type%_wand %actor%
        endif
    done
  endif
elseif %self.vnum% >= 340 && %self.vnum% <= 349
  switch %self.vnum%
    case 340
      if !%actor.quest_stage[phase_mace]%
        quest start phase_mace %actor%
      endif
  done
  if %actor.quest_stage[phase_mace] < (%self.level% / 10) + 1
    return 0
    osend %actor% This weapon is bound to someone else!
  endif
endif
~
#306
phase wand general greet~
0 h 100
~
wait 2
eval minlevel (%wandstep% - 1) * 10
if %actor.quest_stage[%type%_wand]% == %wandstep%
  if %actor.level% >= %minlevel%
    if %actor.quest_variable[%type%_wand:greet]% == 0
      mecho %self.name% says, 'I see you're crafting something.  If you want my help, we can talk about &6&b[upgrades]&0.'
    else
      say Do you have what I need?
    endif
  endif
endif
~
#307
phase wand and phase mace upgrade general speech~
0 d 100
upgrade upgrading craft crafting improve improving improvements upgrades~
wait 2
if %self.vnum% == 18581 || %self.vnum% == 48412
  if %actor.class% == cleric || %actor.class% == priest
    eval minlevel %macestep% * 10
    if %actor.quest_stage[phase_mace]% == %macestep%
      if %actor.level% >= %minlevel%
        quest variable phase_mace %actor% greet 1
        msend %actor% %self.name% says, 'I can add benedictions to your mace, yes.'
        msend %actor%   
        msend %actor% %self.name% says, 'You'll need to use your mace &3&b%maceattack%&0 times.'
        msend %actor%  
        msend %actor% %self.name% says, 'I'll also need the following:'
        msend %actor% - &3&b%get.obj_shortdesc[%maceitem2%]%&0, for its spiritual protection.
        msend %actor% - &3&b%get.obj_shortdesc[%maceitem3%]%&0, as a model for the new mace.
        msend %actor% - &3&b%get.obj_shortdesc[%maceitem4%]%&0, for its power in fighting the undead.
        msend %actor% - &3&b%get.obj_shortdesc[%maceitem5%]%&0, for its guiding light.
        msend %actor%   
        msend %actor% %self.name% says, 'You can check your &6&b[mace progress]&0 at any time.'
      else
        msend %actor% %self.name% says, 'You'll need to be at least level %minlevel% before I can improve the blessings on your mace.'
      endif
    elseif %actor.has_completed[phase_mace]%
      msend %actor% %self.name% says, 'There is no weapon greater than the mace of disruption!'
    elseif %actor.quest_stage[phase_mace]% < %macestep%
      msend %actor% %self.name% says, 'Your mace isn't ready for improvement yet.'
    elseif %actor.quest_stage[phase_mace]% > %macestep%
      msend %actor% %self.name% says, 'I've done all I can already.'
      wait 1s
      switch %actor.quest_stage[phase_mace]%
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
          msend %actor% %self.name% says, 'Of the few remaining who are capable of improving your mace, one is a priest of a god most foul.  Find Ruin Wormheart, that most heinous of Blackmourne's servators.'
          break
        case 8
          msend %actor% %self.name% says, 'The most powerful force in the war against the dead is the sun itself.  Consult with the sun's Oracle in the ancient pyramid near Anduin.'
          break
        case 9
          msend %actor% %self.name% says, 'With everything prepared, return to the very beginning of your journey.  The High Priestess of Mielikki, the very center of the Cleric Guild, will know what to do.'
      done  
    endif
  endif
endif
if %actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer || %actor.class% == illusionist || %actor.class% == necromancer
  eval minlevel (%wandstep% - 1) * 10
  if %actor.quest_stage[%type%_wand]% == %wandstep%
    if %actor.level% >= %minlevel%
      quest variable %type%_wand %actor% greet 1
      if %actor.quest_stage[%type%_wand]% < 7
        if %speech% /= staff
          msend %actor% %self.name% says, 'I can't help you create a staff but I can help improve your wand's powers.'
        else
          msend %actor% %self.name% says, 'I can definitely increase your wand's strength.'
        endif
        wait 2
        msend %actor% %self.name% says, 'You'll need to use your wand &3&b%wandattack%&0 times.'
      elseif %actor.quest_stage[%type%_wand]% == 7
        msend %actor% %self.name% says, 'Your wand has reached its maximum potential.  You're ready for a staff instead.'
        wait 2
        msend %actor% %self.name% says, 'You'll need to use your current wand &3&b%wandattack%&0 times.'
      else
        if %speech% /= wand
          msend %actor% %self.name% says, 'I can't help you create a wand but I can help improve your staff's powers.'
        else
          msend %actor% %self.name% says, 'I can definitely increase your staff's power.'
        endif
        wait 2
        msend %actor% %self.name% says, 'You'll need to use your staff &3&b%wandattack%&0 times.'
      endif
      msend %actor%  
      msend %actor% %self.name% says, 'I'll also need the following:'
      msend %actor% - &3&b%get.obj_shortdesc[%wandgem%]%&0
      if %wandstep% != 7 && %wandstep% != 10
        msend %actor% - &3&b%get.obj_shortdesc[%wandtask3%]%&0, for its resonance with %type% energies.
        if %wandstep% == 4
          msend %actor% &0    Blessings can be called at the smaller groups of standing stones in South Caelia.
          if %self.vnum% == 58601
            msend %actor% &0    Search the far eastern edge of the continent.
          elseif %self.vnum% == 10306
            msend %actor% &0    Search the south point beyond Anlun Vale.
          elseif %self.vnum% == 2337
            msend %actor% &0    Search the surrounding forest.
          elseif %self.vnum% == 62504
            msend %actor% &0    Search in the heart of the heart of the thorns.
          endif
          msend %actor% &0    The phrase to call the blessing is:
          msend %actor% &0    &6&bI pray for a blessing from mother earth, creator of life and bringer of death&0
        endif
      endif
      if %wandstep% == 4
        msend %actor% - &3&b%get.obj_shortdesc[%wandtask4%]%&0 for its sheer magical potential.
        msend %actor% &0    Be careful, thawkinixa is extremely dangerous!
      elseif %wandstep% == 5
        msend %actor% &0 
        msend %actor% Plus you'll need to imbue your wand in %wandtask4%.
      elseif %wandstep% == 6
        msend %actor% &0 
        msend %actor% This next wandstep also requires balancing harmonic frequencies.
        msend %actor% &0 
        msend %actor% Go find &3&b%get.obj_shortdesc[%wandtask4%]%&0.
      elseif %wandstep% == 7
        msend %actor% - &3&b%get.obj_shortdesc[%wandtask3%]%&0, which will form the body of your new staff.
        msend %actor% - &3&b%get.obj_shortdesc[%wandtask4%]%&0, as a fine head for your new staff.
      elseif %wandstep% == 8
        msend %actor% - &3&b%get.obj_shortdesc[%wandtask4%]%&0, which can be harvested from those kinds of elementals.
      elseif %wandstep% == 9
        msend %actor% - proof of your mastery over %type% by slaying &6&b%get.mob_shortdesc[%wandtask4%]%&0.
      elseif %wandstep% == 10
        msend %actor%  
        msend %actor% %self.name% says, 'Energize your staff by slaying &6&b%get.mob_shortdesc[%wandtask3%]%&0 in Templace, then return to me.'
      endif
      msend %actor% &0  
      msend %actor% %self.name% says, 'You can check your &6&b[wand progress]&0 at any time.'
    else
      msend %actor% %self.name% says, 'You'll need to be at least level %minlevel% before I can improve your bond with your weapon.'
    endif
  elseif %actor.quest_stage[%type%_wand]% < %wandstep%
    msend %actor% %self.name% says, 'Your %weapon% isn't ready for improvement yet.'
  elseif %actor.quest_stage[%type%_wand]% > %wandstep%
    msend %actor% %self.name% says, 'I've done all I can already.'
  endif
endif
~
#308
phase wand general receive~
0 j 100
~
set job1 %actor.quest_variable[%type%_wand:wandtask1]%
set job2 %actor.quest_variable[%type%_wand:wandtask2]%
set job3 %actor.quest_variable[%type%_wand:wandtask3]%
set job4 %actor.quest_variable[%type%_wand:wandtask4]%
set job5 %actor.quest_variable[%type%_wand:wandtask5]%
eval reward %wandvnum% + 1
set stage %actor.quest_stage[%type%_wand]%
if %object.vnum% == %wandgem% || %object.vnum% == %wandvnum% || %object.vnum% == %wandtask2% || %object.vnum% == %wandtask3% || %object.vnum% == %wandtask4%
  * Note: Mob 48105 is Vulcera.  She has slightly modified responses because she's a real asshole.
  if !%stage%
    * Are they on the quest?
    set response You haven't even begun this process yet!
  elseif %actor.level% < ((%wandstep% - 1) * 10)
    * Are they the right level?
    if %self.vnum% == 48105
      set response You're far too pathetic to help yet.
    else
      set response You'll need to be at least level %minlevel% before I can improve your bond with your weapon.
    endif
  elseif %actor.has_completed[%type%_wand]%
    * Have they already completed the quest?
    if %self.vnum% == 48105
      set response Idiot.  You already have the most powerful %type% staff a mere mortal can handle.
    else
      set response You already have the most powerful %type% staff in existence!
    endif
  elseif %stage% < %wandstep%
    * Are they below this step still?
    if %self.vnum% == 48105
      set response Your %weapon% is far too weak to craft.
    else
      set response Your %weapon% isn't ready for improvement yet.
    endif
  elseif %stage% > %wandstep%
    * Are they past this step but not yet completed?
    if %self.vnum% == 48105
      set response You still expect more of me??  Bah!  Begone.
    else
      set response I've done all I can already.
    endif
  elseif %actor.quest_variable[%type%_wand:greet]% == 0
    * Have they greeted the quest master properly?
    set response Tell me why you're here first.
  elseif (%object.vnum% == %wandgem% && %job2%) || (%object.vnum% == %wandtask3% && %job3%) || (%object.vnum% == %wandtask4% && %job4%) || ((%stage% == 6 || %stage% == 8) && %object.vnum% == %wandvnum% && %job5%)
    * Have they already given the crafter this item?
    set response You already gave me this.
  elseif %object.vnum% == %wandgem%
    quest variable %type%_wand %actor% wandtask2 1
    set check yes
  elseif %object.vnum% == %wandtask3%
    quest variable %type%_wand %actor% wandtask3 1
    set check yes
  elseif %object.vnum% == %wandtask4%
    quest variable %type%_wand %actor% wandtask4 1
    set check yes
  elseif %object.vnum% == %wandvnum%
    set job1 %actor.quest_variable[%type%_wand:wandtask1]%
    set job2 %actor.quest_variable[%type%_wand:wandtask2]%
    set job3 %actor.quest_variable[%type%_wand:wandtask3]%
    set job4 %actor.quest_variable[%type%_wand:wandtask4]%
    if %job1% && %job2% && %job3% && %job4%
      if %stage% != 3 && %stage% != 6 && %stage% != 8 && %stage% != 10 
        set continue yes
      elseif %stage% == 6 || %stage% == 8
        quest variable %type%_wand %actor% wandtask5 1
        mload obj %wandtask4%
        wait 2
        if %stage% == 6
          wait 2
          mecho %self.name% slides %get.obj_shortdesc[%wandtask3%]% around %get.obj_shortdesc[%wandvnum%]%.
          wait 3s
          mecho %self.name% whispers some arcane syllables.
          wait 3s
          mecho %get.obj_shortdesc[%wandvnum%]% begins to hum!
          wait 2s
          mecho %self.name% says, 'Now, &6&bplay&0 %get.obj_shortdesc[%wandtask4%]% to unlock your wand's full potential.'
        elseif %stage% == 8 
          if %type% == air
            set place the crystal megalith on Griffin Isle
          elseif %type% == acid
            set place the sacred grove in the Highlands
          elseif %type% == ice
            set place the Altar of the Snow Leopard
          elseif %type% == fire
            set place the Fire Temple altar
          endif
          wait 2
          mecho %self.name% utters a powerful enchantment over %object.shortdesc%.
          wait 3s
          mecho %object.shortdesc% begins to glow!
          wait 1s
          mecho %self.name% says, 'Now that your staff is primed, take both it and the ward to:
          mecho - &3&b%place%&0
          mecho &0and &6&bimbue&0 them both with the energies of that place to forge your new staff.'
        endif
        wait 2
        give all %actor%
      endif
    elseif %job1% && %job2% && %job3%
      if %stage% == 10
        set room %get.room[%wandtask4%]%
        quest variable %type%_wand %actor% wandtask4 1
        wait 2
        mecho %self.name% utters eldritch incantations over %object.shortdesc%.
        wait 2s
        mecho %object.shortdesc% begins to crackle with supreme elemental power!
        wait 1s
        mecho %self.name% says, 'Now that you've captured the demon's energies, you must make your way deep into the elemental planes.  There, in the full glory of elemental %type%, find &6&b%room.name%&0 and &6&bimbue&0 it with the energy of the plane.'
        wait 6s
        say It will forge the most powerful staff of %type% in all the realms!
        give all %actor%
      elseif %stage% == 3
        set continue yes
      else
        set continue no
      endif
    else
      set continue no
    endif
  else
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    say What's that for?
  endif
  if %continue% == yes
    wait 2
    quest advance %type%_wand %actor%
    mjunk %object%
    wait 1s
    say Perfect, let me get to work.
    wait 2
    if %stage% != 7
      mecho %self.name% inlays %get.obj_shortdesc[%wandgem%]% in %get.obj_shortdesc[%wandvnum%]%.
      wait 2s
    endif
    if %stage% == 3
      mecho %self.name% burns %get.obj_shortdesc[%wandtask3%]% and passes %get.obj_shortdesc[%wandvnum%]% through the smoke.
    elseif %stage% == 4
      mecho %self.name% melds %get.obj_shortdesc[%wandtask3%]% with %get.obj_shortdesc[%wandvnum%]%.
      wait 2s
      mecho %self.name% binds %get.obj_shortdesc[%wandtask4%]% to the top of %get.obj_shortdesc[%wandvnum%]%.
    elseif %stage% == 5
      mecho %self.name% discharges the potency of %get.obj_shortdesc[%wandtask3%]% into %get.obj_shortdesc[%wandvnum%]%.
    elseif %stage% == 7
      if %self.vnum% == 48105
        mecho %self.name% lavishly discharges the power of %get.obj_shortdesc[%wandvnum%]% into %get.obj_shortdesc[%wandtask3%]%, transferring its power.
      else
        mecho %self.name% discharges the power of %get.obj_shortdesc[%wandvnum%]% into %get.obj_shortdesc[%wandtask3%]%, transferring its power.
      endif
      wait 2s
      mecho %self.name% inlays %get.obj_shortdesc[%wandgem%]% in %get.obj_shortdesc[%wandtask3%]% and afixes %get.obj_shortdesc[%wandtask4%]% to the top.
    elseif %stage% == 9
      mecho %self.name% carefully speaks an incantation to unravel the magic bindings of %get.obj_shortdesc[%wandtask3%]%.
      wait 2s
      mecho %self.name% draws the power out of %get.obj_shortdesc[%wandtask3%]% and fuses it into %get.obj_shortdesc[%wandvnum%]%.
    endif
    wait 2s
    mecho %self.name% utters a few mystic words.
    wait 1s
    if %stage% == 7
      mecho %get.obj_shortdesc[%wandtask3%]% is transformed into %get.obj_shortdesc[%reward%]%!
    else
      mecho %get.obj_shortdesc[%wandvnum%]% is transformed into %get.obj_shortdesc[%reward%]%!
    endif
    wait 1s
    if %stage% == 7
      if %self.vnum% == 48105
        say Here.  Be grateful.  Leave.
      else
        say Here you go, your new staff!
      endif
    else
      say Here you go, your new %weapon%!
    endif
    mload obj %reward%
    give all %actor%
    eval expcap ((%wandstep% - 1) * 10)
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
    * Adjust exp award by class so all classes receive the same proportionate amount
    switch %actor.class%
      case Warrior
      case Berserker
      * 110% of standard 
          eval expmod (%expmod% + (%expmod% / 10))
          break
      case Paladin
      case Anti-Paladin
      case Ranger
      * 115% of standard
          eval expmod (%expmod% + ((%expmod% * 2) / 15))
          break
      case Sorcerer
      case Pyromancer
      case Cryomancer
      case Illusionist
      case Bard
      * 120% of standard
          eval expmod (%expmod% + (%expmod% / 5))
          break
      case Necromancer
      case Monk
      * 130% of standard
          eval expmod (%expmod% + (%expmod% * 2) / 5)
          break
      default
          set expmod %expmod%
    done
    msend %actor% &3&bYou gain experience!&0
    eval setexp (%expmod% * 10)
    set loop 0
    while %loop% < 5
      mexp %actor% %setexp%
      eval loop %loop% + 1
    done
    quest variable %type%_wand %actor% greet 0
    quest variable %type%_wand %actor% attack_counter 0
    set number 1
    while %number% <= 5
      quest variable %type%_wand %actor% wandtask%number% 0
      eval number %number% + 1
    done
  elseif %continue% == no
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    if !%job1% && (!%job2% || !%job3% || !%job4%)
      set counter 50
      eval remaining ((%actor.quest_stage[%type%_wand]% - 1) * %counter%) - %actor.quest_variable[%type%_wand:attack_counter]%
      mecho %self.name% says, 'You still need to attack &3&b%remaining%&0 more times to fully bond with your %weapon%!'
      wait 1s
      say I need the other items as well.
    elseif %job1% && (!%job2% || !%job3% || !%job4%)
      say You have to give me everything else first.
    endif
  elseif %check% == yes
    wait 2
    mjunk %object%
    say This is just what I need.
    wait 2
    set job1 %actor.quest_variable[%type%_wand:wandtask1]%
    set job2 %actor.quest_variable[%type%_wand:wandtask2]%
    set job3 %actor.quest_variable[%type%_wand:wandtask3]% 
    set job4 %actor.quest_variable[%type%_wand:wandtask4]%
    if %job1% && %job2% && %job3% && %job4%
      if %stage% != 3 && %stage% != 6 && %stage% != 8 && %stage% != 10 
        say That's everything!  Now just give me your %weapon%.
      elseif %stage% == 6 || %stage% == 8
        say Let me prime your %weapon%.
      endif
    elseif (%job1% && %job2% && %job3%) && (%stage% == 10 || %stage% == 5 || %stage% == 3 || %stage% == 9)
      if %stage% == 10
        say Let me prime your %weapon%.
      elseif %stage% == 3
        say That's everything!  Now just give me your %weapon%.
      elseif %stage% == 5
        mecho %self.name% says, 'Now go &6&bimbue&0 your %weapon% at %wandtask4% and bring it back to me.'
      elseif %stage% == 9
        say Now go defeat %get.mob_shortdesc[%wandtask4%]%.
      endif
    elseif %job1% && %job2% && %stage% == 10
      say Now go defeat %get.mob_shortdesc[%wandtask3%]%.
    elseif (%stage% == 3 || %stage% == 10) && (!%job1% && %job2% && %job3%)
      say Finish bonding with your %weapon% and bring it to me.
    elseif !%job1% && %job2% && %job3% && %job4%
      say Finish bonding with your %weapon% and bring it to me.
    else
      say Do you have the rest?
    endif
  elseif %response%
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    say %response%
  endif
endif
~
#309
phase wand command imbue~
1 c 3
imbue~
switch %cmd%
  case i
  case im
    return 0
    halt
done
switch %self.vnum%
  case 303
    set type air
    set area howling winds
    break
  case 313
    set type fire
    set area sweltering tunnels
    break
  case 323
    set type ice
    set area vast ocean
    break
  case 333
    set type acid
    set area murky swamp
    break
  case 306
    set type air
    set place 49007
    set ward 53454
    set wardname ward-mist
    set nextname staff-gales
    set crafted 8515
    break
  case 316
    set type fire
    set place 5272
    set ward 53456
    set wardname ward-flames
    set nextname staff-infernos
    set crafter 48250
    break
  case 326
    set type ice
    set place 55105
    set ward 53457
    set wardname ward-ice
    set nextname staff-blizzards
    set crafter 10300
    break
  case 336
    set type acid
    set place 16355
    set ward 53453
    set wardname ward-plants
    set nextname staff-tremors
    set crafter 48029
    break
  case 308
    set type air
    set place 48862
    set color &3&b
    set nextname staff-maelstrom
    break
  case 318
    set type fire
    set place 47800
    set color &1&b
    set nextname staff-glorious-flame
    break
  case 328
    set type ice
    set place 47708
    set color &6&b
    set nextname staff-frozen-deep
    break
  case 338
    set type acid
    set place 47672
    set color &2&b
    set nextname staff-disintegration
    break
done
switch %self.vnum%
  case 306
  case 308
  case 316
  case 318
  case 326
  case 328
  case 336
  case 338
    set weapon staff
    break
  default
    set weapon wand
done
if %actor.room% == %place%
  if !%actor.quest_variable[%type%_wand:wandtask1]%
    set counter 50
    eval remaining ((%actor.quest_stage[%type%_wand]% - 1) * %counter%) - %actor.quest_variable[%type%_wand:attack_counter]%
    osend %actor% You need to attack %remaining% more times to fully bond with your %weapon%!
    halt
  else
    if %self.vnum% == 306 || %self.vnum% == 316 || %self.vnum% == 326 || %self.vnum% == 336
      if %actor.quest_variable[%type%_wand:wandtask5]%
        if %actor.inventory[%ward%]%
          osend %actor% You raise %self.shortdesc% and %get.obj_shortdesc[%ward%]% above your head.
          wait 1s
          osend %actor% The energies of this place start to flow through you!
          wait 3s
          oforce %actor% drop %wardname%
          opurge %wardname%
          osend %actor% %get.obj_shortdesc[%ward%]% floats into the air and unravels into wispy tendrils of elemental energy!
          wait 2s
          osend %actor% The energies wrap around %self.shortdesc%.
          wait 3s
          osend %actor% %self.shortdesc% is transformed!
          set reward yes
          quest advance %type%_wand %actor%
          quest variable %type%_wand %actor% greet 0
          quest variable %type%_wand %actor% attack_counter 0
          set number 1
          while %number% <= 5
            quest variable %type%_wand %actor% wandtask%number% 0
            eval number %number% + 1
          done
        else
          osend %actor% You don't have %get.obj_shortdesc[%ward%]% with you!
        endif
      else
        osend %actor% &4&b%self.shortdesc% must be primed by %get.mob_shortdesc[%crafter%]% before you can imbue it!&0
      endif
    elseif %self.vnum% == 308 || %self.vnum% == 318 || %self.vnum% == 328 || %self.vnum% == 338
      if %actor.quest_variable[%type%_wand:wandtask4]%
        osend %actor% You close your eyes and hold %self.shortdesc% before you.
        wait 2s
        osend %actor% You feel the wheel of the cosmos rotating around you here in this place.
        wait 3s
        osend %actor% You begin to draw from the vast elemental energies of the plane...
        wait 3s
        osend %actor% Drawing all the raw primordial power you can muster, you suffuse the very essence of the realm into your staff!
        osend %actor% %color%You have perfected your staff!&0
        quest complete %type%_wand %actor%
        set reward yes
      endif
    endif
  endif
elseif %self.vnum% == 303
  if %actor.room% >= 55001 && %actor.room% <= 55003 && %actor.quest_variable[%type%_wand:greet]% == 1
    set continue yes
  else
    return 0
  endif
elseif %self.vnum% == 313
  if %actor.room% >= 5200 && %actor.room% <= 5299 && %actor.quest_variable[%type%_wand:greet]% == 1
    set continue yes
  else
    return 0
  endif
elseif %self.vnum% == 323
  if %actor.room% >= 39001 && %actor.room% <= 39189 && %actor.quest_variable[%type%_wand:greet]% == 1
    set continue yes
  else
    return 0
  endif
elseif %self.vnum% == 333
  if %actor.room% >= 7300 && %actor.room% <= 7457 && %actor.quest_variable[%type%_wand:greet]% == 1
    set continue yes
  else
    return 0
  endif
else
  return 0
endif
if %continue% == yes
  osend %actor% You imbue your wand with the energy of the %area%!
  quest variable %type%_wand %actor% wandtask4 1
elseif %reward% == yes
  eval nextstaff %self.vnum% + 1
  oload obj %nextstaff%
  oforce %actor% get %nextname%
  set expcap %self.level%
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
  * Adjust exp award by class so all classes receive the same proportionate amount
  switch %person.class%
    case Warrior
    case Berserker
    * 110% of standard 
      eval expmod (%expmod% + (%expmod% / 10))
      break
    case Paladin
    case Anti-Paladin
    case Ranger
    * 115% of standard
      eval expmod (%expmod% + ((%expmod% * 2) / 15))
      break
    case Sorcerer
    case Pyromancer
    case Cryomancer
    case Illusionist
    case Bard
    * 120% of standard
      eval expmod (%expmod% + (%expmod% / 5))
      break
    case Necromancer
    case Monk
    * 130% of standard
      eval expmod (%expmod% + (%expmod% * 2) / 5)
      break
    default
      set expmod %expmod%
  done
  osend %actor% &3&bYou gain experience!&0
  eval setexp (%expmod% * 10)
  set loop 0
  while %loop% < 5
    oexp %actor% %setexp%
    eval loop %loop% + 1
  done
  opurge %self%
else
  return 0
endif
~
#310
Hell Trident attack manager~
1 d 100
~
if %victim.hit% >= -10
  *
  * adds 5% alignment damage against neutral targets and 10% against good targets
  *
  if %damage% >= 1
    set bonus 0
    if %victim.align% < 350 && %victim.align% > -350
      eval bonus %damage% / 2
    elseif %victim.alignment% > 350
      eval bonus %damage%
    endif
    if %bonus%
      oecho &9&b%self.shortdesc% smites %victim.name% with unholy might!&0 (&3%bonus%&0)
      odamage %victim% %bonus% align
    endif
  endif
  if %random.10% == 1
    if %self.vnum% == 2339
      ocast 'hell bolt' %victim%
    elseif %self.vnum% == 2340
      ocast 'stygian eruption' %victim%
    endif
  endif
endif
if %actor.quest_stage[hell_trident]% == 1 || %actor.quest_stage[hell_trident]% == 2
  if !%actor.quest_variable[hell_trident:helltask1]% 
    eval attack_increase %actor.quest_variable[hell_trident:attack_counter]% + 1
    quest variable hell_trident %actor.name% attack_counter %attack_increase%
    if %actor.quest_variable[hell_trident:attack_counter]% >= 666
      quest variable hell_trident %actor% helltask1 1
      quest variable hell_trident %actor.name% attack_counter 0
      osend %actor% &1&bYou have made sufficient blood offerings with %self.shortdesc%!&0
    endif
  endif
endif
~
#311
phase wand questmaster setup load~
0 o 100
~
switch %self.vnum%
  case 18500
    set type air
    set wandgem 55591
    set wandstep 3
    set wandtask3 23750
    set wandvnum 301
    break
  case 4126
    set type fire
    set wandgem 55590
    set wandstep 3
    set wandtask3 23752
    set wandvnum 311
    break
  case 17806
    set type ice
    set wandgem 55592
    set wandstep 3
    set wandtask3 23753
    set wandvnum 321
    break
  case 10056
    set type acid
    set wandgem 55593
    set wandstep 3
    set wandtask3 23751
    set wandvnum 331
    break
  case 58601
    set type air
    set wandgem 55605
    set wandstep 4
    set wandtask3 2330
    set wandtask4 37006
    set wandvnum 302
    break
  case 10306
    set type fire
    set wandgem 55612
    set wandstep 4
    set wandtask3 2331
    set wandtask4 37006
    set wandvnum 312
    break
  case 2337
    set type ice
    set wandgem 55607
    set wandstep 4
    set wandtask3 2333
    set wandtask4 37006
    set wandvnum 322
    break
  case 62504
    set type acid
    set wandgem 55606
    set wandstep 4
    set wandtask3 2332
    set wandtask4 37006
    set wandvnum 332
    break
  case 12305
    set type air
    set wandgem 55644
    set wandstep 5
    set wandtask3 12509
    set wandtask4 &7&bthe icy ledge outside Technitzitlan&0
    set wandvnum 303
    break
  case 12304
    set type fire
    set wandgem 55639
    set wandstep 5
    set wandtask3 12526
    set wandtask4 &1&bthe Lava Tunnels&0
    set wandvnum 313
    break
  case 55013
    set type ice
    set wandgem 55640
    set wandstep 5
    set wandtask3 58018
    set wandtask4 &6&bthe Arabel Ocean&0
    set wandvnum 323
    break
  case 62503
    set type acid
    set wandgem 55647
    set wandstep 5
    set wandtask3 16303
    set wandtask4 &9&bthe Northern Swamps&0
    set wandvnum 333
    break
  case 12302
    set type air
    set wandgem 55665
    set wandstep 6
    set wandtask3 23800
    set wandtask4 59040
    set wandvnum 304
    break
  case 23811
    set type fire
    set wandgem 55662
    set wandstep 6
    set wandtask3 5201
    set wandtask4 32412
    set wandvnum 314
    break
  case 23802
    set type ice
    set wandgem 55666
    set wandstep 6
    set wandtask3 49011
    set wandtask4 17309
    set wandvnum 324
    break
  case 47075
    set type acid
    set wandgem 55663
    set wandstep 6
    set wandtask3 55020
    set wandtask4 16107
    set wandvnum 334
    break
  case 49003
    set type air
    set wandgem 55682
    set wandstep 7
    set wandtask3 51014
    set wandtask4 23710
    set wandvnum 305
    break
  case 48105
    set type fire
    set wandgem 55689
    set wandstep 7
    set wandtask3 43018
    set wandtask4 11705
    set wandvnum 315
    break
  case 53316
    set type ice
    set wandgem 55684
    set wandstep 7
    set wandtask3 55016
    set wandtask4 23815
    set wandvnum 325
    break
  case 4017
    set type acid
    set wandgem 55683
    set wandstep 7
    set wandtask3 16305
    set wandtask4 37082
    set wandvnum 335
    break
  case 8515
    set type air
    set wandgem 55721
    set wandstep 8
    set wandtask3 11799
    set wandtask4 53454
    set place 49007
    set wandvnum 306
    break
  case 48250
    set type fire
    set wandgem 55716
    set wandstep 8
    set wandtask3 53000
    set wandtask4 53456
    set place 5272
    set wandvnum 316
    break
  case 10300
    set type ice
    set wandgem 55717
    set wandstep 8
    set wandtask3 23847
    set wandtask4 53457
    set place 55105
    set wandvnum 326
    break
  case 48029
    set type acid
    set wandgem 55724
    set wandstep 8
    set wandtask3 58414
    set wandtask4 53453
    set place 16355
    set wandvnum 336
    break
  case 6216
    set type air
    set wandgem 55742
    set wandstep 9
    set wandtask3 49019
    set wandtask4 23803
    set wandvnum 307
    break
  case 48412
    set type fire
    set wandgem 55739
    set wandstep 9
    set wandtask3 48126
    set wandtask4 4013
    set wandvnum 317
    break
  case 10012
    set type ice
    set wandgem 55743
    set wandstep 9
    set wandtask3 48018
    set wandtask4 53300
    set wandvnum 327
    break
  case 3549
    set type acid
    set wandgem 55740
    set wandstep 9
    set wandtask3 47006
    set wandtask4 52018
    set wandvnum 337
    break
  case 18581
    set type air
    set wandgem 11811
    set wandstep 10
    set wandtask3 52001
    set wandtask4 48862
    set wandvnum 308
    break
  case 5230
    set type fire
    set wandgem 23822
    set wandstep 10
    set wandtask3 52002
    set wandtask4 47800
    set wandvnum 318
    break
  case 55020
    set type ice
    set wandgem 53314
    set wandstep 10
    set wandtask3 52005
    set wandtask4 47708
    set wandvnum 328
    break
  case 16315
    set type acid
    set wandgem 52031
    set wandstep 10
    set wandtask3 52007
    set wandtask4 47672
    set wandvnum 338
    break
done
if %wandstep% < 8
  set weapon wand
else
  set weapon staff
endif
eval wandattack (%wandstep% - 1) * 50  
global wandattack weapon type wandgem wandstep wandtask3 wandvnum
if %wandtask4%
  global wandtask4
endif
~
#312
phase wand general death~
0 f 100
~
switch %self.vnum% 
  case 23803
    set type air
    set color &7&b
    set wandvnum 307
    break
  case 52001
    set type air
    set color &7&b
    set wandvnum 308
    break
  case 4013
    set type fire
    set color &1
    set wandvnum 317
    break
  case 52002
    set type fire
    set color &1
    set wandvnum 318
    break
  case 53300
    set type ice
    set color &6&b
    set wandvnum 327
    break
  case 52005
    set type ice
    set color &6&b
    set wandvnum 328
    break
  case 52018
    set type acid
    set color &2&b
    set wandvnum 337
    break
  case 52007
    set type acid
    set color &2&b
    set wandvnum 338
done
set i %actor.group_size%
if %i%
  set a 1
  while %i% >= %a%
    set person %actor.group_member[%a%]%
    set stage %person.quest_stage[%type%_wand]%
    if %person.room% == %self.room%
      if %person.quest_stage[%type%_wand]% == 9
        if !%person.quest_variable[%type%_wand:wandtask4]%
          quest variable %type%_wand %person% wandtask4 1
          msend %person% %color%%get.obj_shortdesc[%wandvnum%]% crackles with vibrant energy!&0
          msend %person% %color%It is primed for reforging!&0
        endif
      elseif %person.quest_stage[%type%_wand]% == 10
        if !%person.quest_variable[%type%_wand:wandtask3]%
          quest variable %type%_wand %person% wandtask3 1
          msend %person% %color%%get.obj_shortdesc[%wandvnum%]% crackles with vibrant energy!&0
          msend %person% %color%It is primed for reforging!&0
        endif
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
else
  set stage %actor.quest_stage[%type%_wand]%
  if %actor.quest_stage[%type%_wand]% == 9
    if !%actor.quest_variable[%type%_wand:wandtask4]%
      quest variable %type%_wand %actor% wandtask4 1
      msend %actor% %color%%get.obj_shortdesc[%wandvnum%]% crackles with vibrant energy!&0
      msend %actor% %color%It is primed for reforging!&0
    endif
  elseif %actor.quest_stage[%type%_wand]% == 10
    if !%actor.quest_variable[%type%_wand:wandtask3]%
      quest variable %type%_wand %actor% wandtask3 1
      msend %actor% %color%%get.obj_shortdesc[%wandvnum%]% crackles with vibrant energy!&0
      msend %actor% %color%It is primed for reforging!&0
    endif
  endif
endif
~
#313
phase wand bigby research assistant progress checker~
0 d 100
progress progress? status status?~
wait 2
set weapon wand
set times 50
if %actor.quest_stage[air_wand]%
  mecho &3&bAIR&0
  if %actor.has_completed[air_wand]%
    say It looks like you already have the most powerful staff of air in existence!
  elseif %actor.quest_stage[air_wand]% == 1
    say Let me see %get.obj_shortdesc[300]%.
  elseif %actor.quest_stage[air_wand]% == 2
    say I am crafting a new air wand for you.
    if %actor.quest_variable[air_wand:wandtask1]% && %actor.quest_variable[air_wand:wandtask2]%
      mecho   
      mecho Simply give me your %get.obj_noadesc[300]%.
    else
      if %actor.quest_variable[air_wand:wandtask1]% || %actor.quest_variable[air_wand:wandtask2]%
        mecho  
        mecho You have done the following:
        if %actor.quest_variable[air_wand:wandtask1]%
          mecho - used your wand %times% times
        endif
        if %actor.quest_variable[air_wand:wandtask2]%
          mecho - brought %get.obj_shortdesc[55577]%
        endif
      endif
      mecho  
      mecho You still need to:
      if !%actor.quest_variable[air_wand:wandtask1]%
        eval remaining %times% - %actor.quest_variable[air_wand:attack_counter]%
        mecho - &3&battack %remaining% more times with your %weapon%&0
      endif
      if !%actor.quest_variable[air_wand:wandtask2]%
        mecho - &3&bbring me %get.obj_shortdesc[55577]%&0
      endif
    endif
  else
    if %actor.quest_stage[air_wand]% > 2
      say I can't help you with air anymore, but I know who can.
      mecho  
      set stage %actor.quest_stage[air_wand]%
      if %stage% == 3
        say Speak with the old Abbot in the Abbey of St. George.
      elseif %stage% == 4
        say The keeper of a southern coastal tower will have advice for you.
      elseif %stage% == 5
        say A master of air near the megalith in South Caelia will be able to help next.
      elseif %stage% == 6
        say Seek out the warrior-witch at the center of the southern megalith.
      elseif %stage% == 7
        say She's hard to deal with, but the Seer of Griffin Isle should have some additional guidance for you.
      elseif %stage% == 8
        say In the diabolist's church is a seer who cannot see.  He's a good resource for this kind of work.
      elseif %stage% == 9
        say The guardian ranger of the Druid Guild in the Red City has some helpful crafting tips.
      elseif %stage% == 10
        say Silania will help you craft the finest of air weapons.
      endif
    endif
  endif
  msend %actor%   
endif
if %actor.quest_stage[fire_wand]%
  mecho &1FIRE&0
  if %actor.has_completed[fire_wand]%
    say It looks like you already have the most powerful staff of fire in existence!
  elseif %actor.quest_stage[fire_wand]% == 1
    say Let me see %get.obj_shortdesc[310]%.
  elseif %actor.quest_stage[fire_wand]% == 2
    say I am crafting a new fire wand for you.
    if %actor.quest_variable[fire_wand:wandtask1]% && %actor.quest_variable[fire_wand:wandtask2]%
      mecho   
      mecho Simply give me your %get.obj_noadesc[310]%.
    else
      if %actor.quest_variable[fire_wand:wandtask1]% || %actor.quest_variable[fire_wand:wandtask2]%
        mecho  
        mecho You have done the following:
        if %actor.quest_variable[fire_wand:wandtask1]%
          mecho - used your wand %times% times
        endif
        if %actor.quest_variable[fire_wand:wandtask2]%
          mecho - brought %get.obj_shortdesc[55575]%
        endif
      endif
      mecho  
      mecho You still need to:
      if !%actor.quest_variable[fire_wand:wandtask1]%
        eval remaining %times% - %actor.quest_variable[fire_wand:attack_counter]%
        mecho - &3&battack %remaining% more times with your %weapon%&0
      endif
      if !%actor.quest_variable[fire_wand:wandtask2]%
        mecho - &3&bbring me %get.obj_shortdesc[55575]%&0
      endif
    endif
  else
    if %actor.quest_stage[fire_wand]% > 2
      say I can't help you with fire anymore, but I know who can.
      mecho  
      set stage %actor.quest_stage[fire_wand]% 
      if %stage% == 3
        say A minion of the dark flame out east will know what to do.
      elseif %stage% == 4
        say There's a fire master in the frozen north who likes to spend his time at the hot springs.
      elseif %stage% == 5
        say A master of fire near the megalith in South Caelia will be able to help next.
      elseif %stage% == 6
        say A seraph crafts with the power of the sun and sky.  It can be found in the floating fortress in South Caelia.
      elseif %stage% == 7
        say I hate to admit it, but Vulcera is your next crafter.  Good luck appeasing her though...
      elseif %stage% == 8
        say You're headed back to Fiery Island.  Crazy old McCabe can help you improve your staff further.
      elseif %stage% == 9
        say Seek out the one who speaks for the Sun near Anduin.  He can upgrade your staff.
      elseif %stage% == 10
        say Surely you've heard of Emmath Firehand.  He's the supreme artisan of fiery goods.  He can help you make the final improvements to your staff.
      endif
    endif
  endif
  msend %actor%    
endif
if %actor.quest_stage[ice_wand]%
  mecho &6&bICE&0
  if %actor.has_completed[ice_wand]%
    say It looks like you already have the most powerful staff of ice in existence!
  elseif %actor.quest_stage[ice_wand]% == 1
    say Let me see %get.obj_shortdesc[320]%.
  elseif %actor.quest_stage[ice_wand]% == 2
    say I am crafting a new ice wand for you.
    if %actor.quest_variable[ice_wand:wandtask1]% && %actor.quest_variable[ice_wand:wandtask2]%
      mecho   
      mecho Simply give me your %get.obj_noadesc[320]%.
    else
      if %actor.quest_variable[ice_wand:wandtask1]% || %actor.quest_variable[ice_wand:wandtask2]%
        mecho  
        mecho You have done the following:
        if %actor.quest_variable[ice_wand:wandtask1]%
          mecho - used your wand %times% times
        endif
        if %actor.quest_variable[ice_wand:wandtask2]%
          mecho - brought %get.obj_shortdesc[55574]%
        endif
      endif
      mecho  
      mecho You still need to:
      if !%actor.quest_variable[ice_wand:wandtask1]%
        eval remaining %times% - %actor.quest_variable[ice_wand:attack_counter]%
        mecho - &3&battack %remaining% more times with your %weapon%&0
      endif
      if !%actor.quest_variable[ice_wand:wandtask2]%
        mecho - &3&bbring me %get.obj_shortdesc[55574]%&0
      endif
    endif
  else
    if %actor.quest_stage[ice_wand]% > 2
      say I can't help you with ice anymore, but I know who can.
      mecho  
      set stage %actor.quest_stage[ice_wand]%
      if %stage% == 3
        say The shaman near Three-Falls River has developed a powerful affinity for water from his life in the canyons.  Seek his advice.
      elseif %stage% == 4
        say Many of the best craftspeople aren't even mortal.  There is a water sprite of some renown deep in Anlun Vale.
      elseif %stage% == 5
        say A master of spirits in the far north will be able to help next.
      elseif %stage% == 6
        say Your next crafter is a distant relative of the Sunfire clan.  He's been squatting in a flying fortress for many months, trying to unlock its secrets.
      elseif %stage% == 7
        say You'll need the advice of a master ice sculptor.  One works regularly up in Mt. Frostbite.
      elseif %stage% == 8
        say There's another distant relative of the Sunfire clan who runs the hot springs near Ickle.  He's book smart and knows a thing or two about jewel crafting.
      elseif %stage% == 9
        say The guild guard for the Sorcerer Guild in Ickle has learned plenty of secrets from the inner sanctum.  Talk to him.
      elseif %stage% == 10
        say You must know Suralla Iceeye by now.  She's the master artisan of cold and ice.  She'll know how to make the final improvements to your staff.
      endif
    endif
  endif
  msend %actor%  
endif
if %actor.quest_stage[acid_wand]%
  mecho &2&bACID&0
  if %actor.has_completed[acid_wand]%
    say It looks like you already have the most powerful staff of acid in existence!
  elseif %actor.quest_stage[acid_wand]% == 1
    say Let me see %get.obj_shortdesc[330]%.
  elseif %actor.quest_stage[acid_wand]% == 2
    say I am crafting a new acid wand for you.
    if %actor.quest_variable[acid_wand:wandtask1]% && %actor.quest_variable[acid_wand:wandtask2]%
      mecho   
      mecho Simply give me your %get.obj_noadesc[330]%.
    else
      if %actor.quest_variable[acid_wand:wandtask1]% || %actor.quest_variable[acid_wand:wandtask2]%
        mecho  
        mecho You have done the following:
        if %actor.quest_variable[acid_wand:wandtask1]%
          mecho - used your wand %times% times
        endif
        if %actor.quest_variable[acid_wand:wandtask2]%
          mecho - brought %get.obj_shortdesc[55576]%
        endif
      endif
      mecho  
      mecho You still need to:
      if !%actor.quest_variable[acid_wand:wandtask1]%
        eval remaining %times% - %actor.quest_variable[acid_wand:attack_counter]%
        mecho - &3&battack %remaining% more times with your %weapon%&0
      endif
      if !%actor.quest_variable[acid_wand:wandtask2]%
        mecho - &3&bbring me %get.obj_shortdesc[55576]%&0
      endif
    endif
  else
    if %actor.quest_stage[acid_wand]% > 2
      say I can't help you with acid anymore, but I know who can.
      mecho  
      set stage %actor.quest_stage[acid_wand]%
      if %stage% == 3
        say First, seek the one who guards the eastern gates of Ickle.
      elseif %stage% == 4
        say The next two artisans dwell in the Rhell Forest south-east of Mielikki.
      elseif %stage% == 5
        say Your next crafter isn't exactly part of the ranger network...  It's not actually a person at all.  Find the treant in the Rhell forest and ask it for guidance.
      elseif %stage% == 6
        say The ranger who guards the massive necropolis near Anduin has wonderful insights on crafting with decay.
      elseif %stage% == 7
        say Your next guide may be hard to locate...  I believe they guard the entrance to a long-lost kingdom beyond a frozen desert.
      elseif %stage% == 8
        say Next, consult with another ranger who guards a place crawling with the dead.  The dwarf ranger in the iron hills will know how to help you.
      elseif %stage% == 9
        say The guard of the only known Ranger Guild in the world is also an excellent craftswoman.  Consult with her.
      elseif %stage% == 10
        say Your last guide is the head of the ranger network himself, Eleweiss.  He can help make the final improvements to your staff.
      endif
    endif
  endif
endif
~
#314
phase wand status checker questmasters~
0 d 0
wand progress~
wait 2
if %actor.class% != sorcerer && %actor.class% != cryomancer && %actor.class% != pyromancer && %actor.class% != illusionist && %actor.class% != necromancer
  say This weapon is only for students of the arcane arts.
  halt
endif
set stage %actor.quest_stage[%type%_wand]%
set job1 %actor.quest_variable[%type%_wand:wandtask1]%
set job2 %actor.quest_variable[%type%_wand:wandtask2]%
set job3 %actor.quest_variable[%type%_wand:wandtask3]%
set job4 %actor.quest_variable[%type%_wand:wandtask4]%
set job5 %actor.quest_variable[%type%_wand:wandtask5]%
if %actor.has_completed[%type%_wand]%
  say It looks like you already have the most powerful staff of %type% in existence!
elseif %stage% == %wandstep% && %actor.level% >= (%wandstep% - 1) * 10
  if %actor.quest_variable[%type%_wand:greet]% == 0
    say Tell me why you're here first.
  else
    say I'm improving your %type% weapon.
    if %job1% || %job2% || %job3% || %job4% 
      mecho  
      say You've done the following:
      if %job1%
        mecho - attacked %wandattack% times
      endif
      if %job2%
        mecho - found %get.obj_shortdesc[%wandgem%]%
      endif
      if %job3%
        if %wandstep% != 10
          mecho - found %get.obj_shortdesc[%wandtask3%]%
        else
          mecho - slayed %get.mob_shortdesc[%wandtask3%]%
        endif
      endif
      if %job4%
        if %wandstep% != 5 && %wandstep% != 9 && %wandstep% != 10
          mecho - found %get.obj_shortdesc[%wandtask4%]%
        elseif %wandstep% == 5
          mecho - communed in %wandtask4%
        elseif %wandstep% == 9
          mecho - slayed %get.mob_shortdesc[%wandtask4%]%
        endif
      endif
    endif
    mecho  
    mecho You need to:
    if %job1% && %job2% && %job3% && %job4%
      if %wandstep% != 6 && %wandstep% != 8 && %wandstep% !=10
        mecho Just give me your %weapon%.
      elseif %wandstep% == 6
        if !%job5%
          mecho Just give me your %weapon%.
        else
          mecho - &6&bplay&0 %get.obj_shortdesc[%wandtask4%]%
        endif
      elseif %wandstep% == 8
        if !%job5%
          mecho Just give me your %weapon%.
        else
          set room %get.room[%place%]%
          mecho - imbue your %weapon% at %room.name%.
        endif
      elseif %wandstep% == 10
        set room %get.room[%wandtask4%]%
        mecho - imbue your %weapon% at %room.name%.
      endif
    endif
    if !%job1%
      set counter 50
      eval remaining ((%actor.quest_stage[%type%_wand]% - 1) * %counter%) - %actor.quest_variable[%type%_wand:attack_counter]%
      mecho - &3&battack %remaining% more times with your %weapon%&0
    endif
    if !%job2%
      mecho - &3&bfind %get.obj_shortdesc[%wandgem%]%&0
    endif
    if !%job3%
      if %wandstep% != 10
        mecho - &3&bfind %get.obj_shortdesc[%wandtask3%]%&0
        if %wandstep% == 4
          mecho &0    Blessings can be called at the smaller groups of standing stones in South Caelia.
          if %self.vnum% == 58601
            mecho &0    Search the far eastern edge of the continent.
          elseif %self.vnum% == 10306
            mecho &0    Search the south point beyond Anlun Vale.
          elseif %self.vnum% == 2337
            mecho &0    Search the surrounding forest.
          elseif %self.vnum% == 62504
            mecho &0    Search in the heart of the heart of the thorns.
          endif
          mecho &0    The phrase to call the blessing is:
          mecho &0    &6&bI pray for a blessing from mother earth, creator of life and bringer of death&0
        endif
      else
        mecho - &3&bslay %get.mob_shortdesc[%wandtask3%]%&0
      endif
    endif
    if !%job4%
      if %wandstep% != 3 && %wandstep% != 5 && %wandstep% != 9 && %wandstep% != 10
        mecho - &3&bfind %get.obj_shortdesc[%wandtask4%]%&0
      elseif %wandstep% == 5
        if %type% == %air%
          mecho - &3&bimbue your %weapon% on %wandtask4%&0
        else
          mecho - &3&bimbue your %weapon% in %wandtask4%&0
        endif
      elseif %wandstep% == 9
        mecho - &3&bslay %get.mob_shortdesc[%wandtask4%]%&0
      elseif %wandstep% == 10
        if %job1% && %job2% && %job3%
          mecho Just give me your %weapon%.
        endif
      endif
    endif
  endif
elseif %stage% > %wandstep%
  say I can't help you anymore, but I know who can.
  if %type% == air
    mecho  
    if %stage% == 3
      say Speak with the old Abbot in the Abbey of St. George.
    elseif %stage% == 4
      say The keeper of a southern coastal tower will have advice for you.
    elseif %stage% == 5
      say A master of air near the megalith in South Caelia will be able to help next.
    elseif %stage% == 6
      say Seek out the warrior-witch at the center of the southern megalith.
    elseif %stage% == 7
      say She's hard to deal with, but the Seer of Griffin Isle should have some additional guidance for you.
    elseif %stage% == 8
      say In the diabolist's church is a seer who cannot see.  He's a good resource for this kind of work.
    elseif %stage% == 9
      say The guardian ranger of the Druid Guild in the Red City has some helpful crafting tips.
    elseif %stage% == 10
      say Silania will help you craft the finest of air weapons.
    endif
  elseif %type% == fire 
    mecho  
    if %stage% == 3
      say A minion of the dark flame out east will know what to do.
    elseif %stage% == 4
      say There's a fire master in the frozen north who likes to spend his time at the hot springs.
    elseif %stage% == 5
      say A master of fire near the megalith in South Caelia will be able to help next.
    elseif %stage% == 6
      say A seraph crafts with the power of the sun and sky.  It can be found in the floating fortress in South Caelia.
    elseif %stage% == 7
      say I hate to admit it, but Vulcera is your next crafter.  Good luck appeasing her though...
    elseif %stage% == 8
      say You're headed back to Fiery Island.  Crazy old McCabe can help you improve your staff further.
    elseif %stage% == 9
      say Seek out the one who speaks for the Sun near Anduin.  He can upgrade your staff.
    elseif %stage% == 10
      say Surely you've heard of Emmath Firehand.  He's the supreme artisan of fiery goods.  He can help you make the final improvements to your staff.
    endif
  elseif %type% == ice
    mecho  
    if %stage% == 3
      say The shaman near Three-Falls River has developed a powerful affinity for water from his life in the canyons.  Seek his advice.
    elseif %stage% == 4
      say Many of the best craftspeople aren't even mortal.  There is a water sprite of some renown deep in Anlun Vale.
    elseif %stage% == 5
      say A master of spirits in the far north will be able to help next.
    elseif %stage% == 6
      say Your next crafter is a distant relative of the Sunfire clan.  He's been squatting in a flying fortress for many months, trying to unlock its secrets.
    elseif %stage% == 7
      say You'll need the advice of a master ice sculptor.  One works regularly up in Mt. Frostbite.
    elseif %stage% == 8
      say There's another distant relative of the Sunfire clan who runs the hot springs near Ickle.  He's book smart and knows a thing or two about jewel crafting.
    elseif %stage% == 9
      say The guild guard for the Sorcerer Guild in Ickle has learned plenty of secrets from the inner sanctum.  Talk to him.
    elseif %stage% == 10
      say You must know Suralla Iceeye by now.  She's the master artisan of cold and ice.  She'll know how to make the final improvements to your staff.
    endif
  elseif %type% == acid
    mecho  
    if %stage% == 3
      say First, seek the one who guards the eastern gates of Ickle.
    elseif %stage% == 4
      say The next two artisans dwell in the Rhell Forest south-east of Mielikki.
    elseif %stage% == 5
      say Your next crafter isn't exactly part of the ranger network...  It's not actually a person at all.  Find the treant in the Rhell forest and ask it for guidance.
    elseif %stage% == 6
      say The ranger who guards the massive necropolis near Anduin has wonderful insights on crafting with decay.
    elseif %stage% == 7
      say Your next guide may be hard to locate...  I believe they guard the entrance to a long-lost kingdom beyond a frozen desert.
    elseif %stage% == 8
      say Next, consult with another ranger who guards a place crawling with the dead.  The dwarf ranger in the iron hills will know how to help you.
    elseif %stage% == 9
      say The guard of the only known Ranger Guild in the world is also an excellent craftswoman.  Consult with her.
    elseif %stage% == 10
      say Your last guide is the head of the ranger network himself, Eleweiss.  He can help make the final improvements to your staff.
    endif
  endif
elseif %stage% < %wandstep%
  say You need to make more improvements to your %weapon% before I can work with it.'
elseif (%actor.level% < (%wandstep% - 1) * 10)
  say Come back after you've gained some more experience.  I can help you then.
endif
~
#315
phase mace speech upgrade~
0 d 100
upgrade upgrades upgrading improvement improvements improving~
if %self.vnum% == 18502
  mmobflag %self% sentinel on
endif
wait 2
if %actor.class% == cleric || %actor.class% == priest
  eval minlevel %macestep% * 10
  if %actor.quest_stage[phase_mace]% == %macestep%
    if %actor.level% >= %minlevel%
      quest variable phase_mace %actor% greet 1
      msend %actor% %self.name% says, 'I can add benedictions to your mace, yes.'
      msend %actor% &0  
      msend %actor% %self.name% says, 'You'll need to use your mace &3&b%maceattack%&0 times.'
      msend %actor% &0 
      msend %actor% %self.name% says, 'I'll also need the following:'
      msend %actor% - &3&b%get.obj_shortdesc[%maceitem2%]%&0, for its spiritual protection.
      if %macestep% != 2
        msend %actor% - &3&b%get.obj_shortdesc[%maceitem3%]%&0, as a model for the new mace.
        msend %actor% - &3&b%get.obj_shortdesc[%maceitem4%]%&0, for its power in fighting the undead.
        msend %actor% - &3&b%get.obj_shortdesc[%maceitem5%]%&0, for its guiding light.
      else
        msend %actor% &0  
        msend %actor% %self.name% says, 'And you'll need to make a pilgrimage to four burial grounds to see what lies ahead on your journey:'
        msend %actor% - &3&bthe necropolis near Anduin&0
        msend %actor% - &3&bthe pyramid in the Gothra desert&0
        msend %actor% - &3&bthe ancient barrow in the Iron Hills&0
        msend %actor% - &3&bthe cemetary outside the Cathedral of Betrayal&0
        msend %actor% &0 
        msend %actor% %self.name% says, 'Be warned, these places are &1exceedingly dangerous!&0'
        wait 2s
        mload obj 18526
        give grave-spade %actor%
        msend %actor% %self.name% says, 'Holding this spade, &6&b[dig]&0 up a handful of dirt from each of these places and bring it back to me as a token of your journey.'
      endif
      msend %actor% &0  
      msend %actor% %self.name% says, 'You can check your &6&b[mace progress]&0 at any time.'
    else
      msend %actor% %self.name% says, 'You'll need to be at least level %minlevel% before I can improve your bond with your weapon.'
    endif
  elseif %actor.has_completed[phase_mace]%
    msend %actor% %self.name% says, 'There is no weapon greater than the mace of disruption!'
  elseif %actor.quest_stage[phase_mace]% < %macestep%
    msend %actor% %self.name% says, 'Your mace isn't ready for improvement yet.'
  elseif %actor.quest_stage[phase_mace]% > %macestep%
    msend %actor% %self.name% says, 'I've done all I can already.'
    wait 1s
    switch %actor.quest_stage[phase_mace]%
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
        msend %actor% %self.name% says, 'Of the few remaining who are capable of improving your mace, one is a priest of a god most foul.  Find Ruin Wormheart, that most heinous of Blackmourne's servators.'
        break
      case 8
        msend %actor% %self.name% says, 'The most powerful force in the war against the dead is the sun itself.  Consult with the sun's Oracle in the ancient pyramid near Anduin.'
        break
      case 9
        msend %actor% %self.name% says, 'With everything prepared, return to the very beginning of your journey.  The High Priestess of Mielikki, the very center of the Cleric Guild, will know what to do.'
    done  
  endif
endif
if %self.vnum% == 18502
  mmobflag %self% sentinel off
endif
~
#316
Phase mace progress~
0 d 0
mace progress~
wait 2
if %actor.class% != cleric && %actor.class% != priest
  msend %actor% %self.name% says, 'This weapon is only for servants of the gods.'
  halt
endif
set stage %actor.quest_stage[phase_mace]%
set job1 %actor.quest_variable[phase_mace:macetask1]%
set job2 %actor.quest_variable[phase_mace:macetask2]%
set job3 %actor.quest_variable[phase_mace:macetask3]%
set job4 %actor.quest_variable[phase_mace:macetask4]%
set job5 %actor.quest_variable[phase_mace:macetask5]%
set job6 %actor.quest_variable[phase_mace:macetask6]%
if %actor.has_completed[phase_mace]%
  msend %actor% %self.name% says, 'There is no weapon greater than the mace of disruption!'
elseif %stage% == %macestep% && %actor.level% >= %macestep% * 10
  if %actor.quest_variable[phase_mace:greet]% == 0
    msend %actor% %self.name% says, 'Tell me why you're here first.'
  else
    msend %actor% %self.name% says, 'I'm improving your mace.'
    if %job1% || %job2% || %job3% || %job4% || %job5%
      msend %actor%  
      msend %actor% %self.name% says, 'You've done the following:'
      if %job1%
        msend %actor% - attacked %maceattack% times
      endif
      if %job2%
        msend %actor% - found %get.obj_shortdesc[%maceitem2%]%
      endif
      if %job3%
        msend %actor% - found %get.obj_shortdesc[%maceitem3%]%
      endif
      if %job4%
        msend %actor% - found %get.obj_shortdesc[%maceitem4%]%
      endif
      if %job5%
        msend %actor% - found %get.obj_shortdesc[%maceitem5%]%
      endif
    endif
    msend %actor%  
    msend %actor% You need to:
    if %job1% && %job2% && %job3% && %job4% && %job5%
      if %macestep% != 2
        msend %actor% Just give me your mace.
        halt
      else
        if %job6%
          msend %actor% Just give me your mace.
          halt
        endif
      endif
    endif
    if !%job1%
      eval remaining (%stage% * 50) - %actor.quest_variable[phase_mace:attack_counter]%
      if %remaining% > 1
        msend %actor% - &3&battack %remaining% more times with your mace&0
      else 
        msend %actor% - &3&battack %remaining% more time with your mace&0
      endif
    endif
    if !%job2%
      msend %actor% - &3&bfind %get.obj_shortdesc[%maceitem2%]%&0
    endif
    if !%job3%
      msend %actor% - &3&bfind %get.obj_shortdesc[%maceitem3%]%&0
    endif
    if !%job4%
      msend %actor% - &3&bfind %get.obj_shortdesc[%maceitem4%]%&0
    endif
    if !%job5%
      msend %actor% - &3&bfind %get.obj_shortdesc[%maceitem5%]%&0
    endif
    if %macestep% == 2
      if !%job6%
        msend %actor% - &3&bfind %get.obj_shortdesc[%maceitem6%]%&0
      endif
    endif
  endif
elseif %stage% > %macestep%
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
      msend %actor% %self.name% says, 'Of the few remaining who are capable of improving your mace, one is a priest of a god most foul.  Find Ruin Wormheart, that most heinous of Blackmourne's servators.'
      break
    case 8
      msend %actor% %self.name% says, 'The most powerful force in the war against the dead is the sun itself.  Consult with the sun's Oracle in the ancient pyramid near Anduin.'
      break
    case 9
      msend %actor% %self.name% says, 'With everything prepared, return to the very beginning of your journey.  The High Priestess of Mielikki, the very center of the Cleric Guild, will know what to do.'
  done
elseif %stage% < %macestep%
  msend %actor% %self.name% says, 'You need to make more improvements to your mace before I can work with it.' 
elseif %actor.level% < %macestep% * 10
  msend %actor% %self.name% says, 'Come back after you've gained some more experience.  I can help you then.'
endif
~
#317
Phase mace set owner give~
1 i 100
~
switch %victim.vnum%
  case 3025
    set macestep 1
    if !%actor.quest_stage[phase_mace]%
      quest start phase_mace %actor%
    endif
    break
  case 18502
    set macestep 2
    break
  case 10000
    set macestep 3
    break
  case 6218
    set macestep 4
    break
  case 8501
    set macestep 5
    break
  case 18581
    set macestep 6
    break
  case 6007
    set macestep 7
    break
  case 48412
    set macestep 8
    break
  case 3021
    set macestep 9
    break
  default
    if %victim.vnum% != -1
      return 0
      osend %actor% You shouldn't give away something so precious!
    endif
    halt
done
if %actor.quest_stage[phase_mace]% < %macestep%
  set response This isn't yours!  I can't help you properly improve a mace that doesn't belong to you.
elseif %actor.quest_stage[phase_mace]% > %macestep%
  set response I've already done everything I can to help you.
endif
if %response%
  return 0
  oecho %victim.name% refuses %self.shortdesc%.
  wait 2
  osend %actor% %victim.name% tells you, '%response%'
endif
~
#318
Phase mace load variables~
0 o 100
~
switch %self.vnum%
  case 3025
    set maceitem2 55577
    set maceitem3 55211
    set maceitem4 13614
    set maceitem5 58809
    set macestep 1
    break
  case 18502
    set maceitem2 55593
    set maceitem3 18522
    set maceitem4 18523
    set maceitem5 18524
    set maceitem6 18525
    set macestep 2
    break
  case 10000
    set maceitem2 55604
    set maceitem3 32409
    set maceitem4 59022
    set maceitem5 2327
    set maceattack 150
    set macestep 3
    break
  case 6218
    set maceitem2 55631
    set maceitem3 16030
    set maceitem4 47002
    set maceitem5 5211
    set macestep 4
    break
  case 8501
    set maceitem2 55660
    set maceitem3 43007
    set maceitem4 59012
    set maceitem5 17308
    set macestep 5
    break
  case 18581
    set macestep 6
    set maceitem2 55681
    set maceitem3 23824
    set maceitem4 53016
    set maceitem5 16201
    break
  case 6007
    set maceitem2 55708
    set maceitem3 49502
    set maceitem4 4008
    set maceitem5 47017
    set maceattack 350
    set macestep 7
    break
  case 48412
    set maceitem2 55737
    set maceitem3 53305
    set maceitem4 12307
    set maceitem5 51073
    set macestep 8
    break
  case 3021
    set maceitem2 55738
    set maceitem3 48002
    set maceitem4 52010
    set maceitem5 3218
    set macestep 9
done
eval maceattack %macestep% * 50
eval macevnum %macestep% + 339
eval reward_mace %macevnum% + 1
global maceitem2 maceitem3 maceitem4 maceitem5 maceattack macestep macevnum reward_mace
if %maceitem6%
  global maceitem6
endif
~
#319
Phase Mace attack counter and damage~
1 d 100
~
if %actor.quest_stage[phase_mace]% >= 1
  if !%actor.quest_variable[phase_mace:macetask1]% 
    eval attack_increase %actor.quest_variable[phase_mace:attack_counter]% + 1
    quest variable phase_mace %actor.name% attack_counter %attack_increase%
    if %actor.quest_variable[phase_mace:attack_counter]% >= %actor.quest_stage[phase_mace]% * 50
      quest variable phase_mace %actor% macetask1 1
      osend %actor% &7&bYou have perfected your bond with %self.shortdesc%!&0
    endif
  endif
endif
if %random.4% > 1
  set skill %self.level%
  if %victim.composition% == ether && %victim.lifeforce% == undead
    switch %self.vnum%
      case 340
      case 341
        halt
        break
      case 342
      case 343
      case 344
      case 345
        set spell 'divine bolt'
        break
      case 346
      case 347
      case 348
      case 349
        set spell 'divine ray'
        break
    done
    ocast %spell% %victim% %skill%
  elseif %victim.lifeforce% == undead || %victim.lifeforce% == demonic || %victim.race% == demon
    eval skill %skill% / 2
    switch %self.vnum%
      case 340
      case 341
      case 342
        halt
        break
      case 343
      case 344
      case 345
      case 346
      case 347
      case 348
      case 349
        set spell 'divine bolt'
        break
    done
    ocast %spell% %victim% %skill%
  endif
endif
~
#320
Phase mace dig trigger~
1 c 1
dig~
switch %cmd%
  case d
  case di
    return 0
    halt
done
if %actor.quest_stage[phase_mace]% == 2
  if %actor.quest_variable[phase_mace:graves]% == done
    osend %actor% &3&bYou have already completed your pilgrimage.&0
  endif
  set room %actor.room%
  *Graveyard*
  if %room.vnum% >= 47000 && %room.vnum% <= 47404
    set dig yes
    set item 18522
    set num 3
  *Cathedral*
  elseif %room.vnum% >= 8504 && %room.vnum% <= 8509
    set dig yes
    set item 18523
    set num 4
  *Pyramid*
  elseif %room.vnum% >= 16200 && %room.vnum% <= 16299
    set dig yes
    set item 18524
    set num 5
  *Barrow*
  elseif %room.vnum% >= 48000 && %room.vnum% <= 48099
    set dig yes
    set item 18525
    set num 6
  endif
  if %dig% == yes
    osend %actor% You dig up a handful of dirt.
    oload obj %item%
    quest variable phase_mace %actor% dirt%num% 1
    oforce %actor% get dirt
    set dirt3 %actor.quest_variable[phase_mace:dirt3]%
    set dirt4 %actor.quest_variable[phase_mace:dirt4]% 
    set dirt5 %actor.quest_variable[phase_mace:dirt5]%
    set dirt6 %actor.quest_variable[phase_mace:dirt6]%  
    if %dirt3% && %dirt4% && %dirt5% && %dirt6%
      if !%actor.quest_variable[phase_mace:graves]%
        osend %actor% &3&bYou have completed your pilgrimage.&0
        quest variable phase_mace %actor% graves done
      endif
    endif
  else
    osend %actor% This isn't the proper place to dig for grave dirt.
  endif
else
  return 0
endif
~
#321
Monk vision attack counter~
1 d 100
~
if %actor.quest_stage[monk_vision]% >= 1
  if !%actor.quest_variable[monk_vision:visiontask1]% 
    eval attack_increase %actor.quest_variable[monk_vision:attack_counter]% + 1
    quest variable monk_vision %actor.name% attack_counter %attack_increase%
    if %actor.quest_variable[monk_vision:attack_counter]% >= %actor.quest_stage[monk_vision]% * 100
      quest variable monk_vision %actor% visiontask1 1
      quest variable monk_vision %actor.name% attack_counter 0
      osend %actor% &3&bYou have perfected your bond with %self.shortdesc%!&0
    endif
  endif
endif
~
#322
Assassin mask attack counter~
1 d 100
~
if %actor.quest_stage[assassin_mask]% >= 1
  if !%actor.quest_variable[assassin_mask:masktask1]% 
    eval attack_increase %actor.quest_variable[assassin_mask:attack_counter]% + 1
    quest variable assassin_mask %actor.name% attack_counter %attack_increase%
    if %actor.quest_variable[assassin_mask:attack_counter]% >= %actor.quest_stage[assassin_mask]% * 100
      quest variable assassin_mask %actor% masktask1 1
      quest variable assassin_mask %actor.name% attack_counter 0
      osend %actor% &3&bYou have perfected your bond with %self.shortdesc%!&0
    endif
  endif
endif
~
#323
Paladin pendant attack counter~
1 d 100
~
if %actor.quest_stage[paladin_pendant]% >= 1
  if !%actor.quest_variable[paladin_pendant:necklacetask1]% 
    eval attack_increase %actor.quest_variable[paladin_pendant:attack_counter]% + 1
    quest variable paladin_pendant %actor.name% attack_counter %attack_increase%
    if %actor.quest_variable[paladin_pendant:attack_counter]% >= %actor.quest_stage[paladin_pendant]% * 100
      quest variable paladin_pendant %actor% necklacetask1 1
      quest variable paladin_pendant %actor.name% attack_counter 0
      osend %actor% &3&bYou have perfected your bond with %self.shortdesc%!&0
    endif
  endif
endif
~
#324
Ranger trophy attack counter~
1 d 100
~
if %actor.quest_stage[ranger_trophy]% >= 1
  if !%actor.quest_variable[ranger_trophy:trophytask1]% 
    eval attack_increase %actor.quest_variable[ranger_trophy:attack_counter]% + 1
    quest variable ranger_trophy %actor.name% attack_counter %attack_increase%
    if %actor.quest_variable[ranger_trophy:attack_counter]% >= %actor.quest_stage[ranger_trophy]% * 100
      quest variable ranger_trophy %actor% trophytask1 1
      quest variable ranger_trophy %actor.name% attack_counter 0
      osend %actor% &2&bYou have perfected your bond with %self.shortdesc%!&0
    endif
  endif
endif
~
#325
Rogue cloak attack counter~
1 d 100
~
if %actor.quest_stage[rogue_cloak]% >= 1
  if !%actor.quest_variable[rogue_cloak:cloaktask1]% 
    eval attack_increase %actor.quest_variable[rogue_cloak:attack_counter]% + 1
    quest variable rogue_cloak %actor.name% attack_counter %attack_increase%
    if %actor.quest_variable[rogue_cloak:attack_counter]% >= %actor.quest_stage[rogue_cloak]% * 100
      quest variable rogue_cloak %actor% cloaktask1 1
      quest variable rogue_cloak %actor.name% attack_counter 0
      osend %actor% &3&bYou have perfected your bond with %self.shortdesc%!&0
    endif
  endif
endif
~
#326
phase mace receive~
0 j 100
~
if %self.vnum% == 18502
  mmobflag %self% sentinel on
endif
if %object.vnum% == %maceitem2% || %object.vnum% == %maceitem3% || %object.vnum% == %maceitem4% || %object.vnum% == %maceitem5% || %object.vnum% == %maceitem6% || %object.vnum% == %macevnum%
  if %actor.class% == cleric || %actor.class% == priest
    if %actor.level% >= (%macestep% * 10)
      if %actor.quest_stage[phase_mace]% == %macestep%
        if %actor.quest_variable[phase_mace:greet]% == 1
          if %object.vnum% == %maceitem2% || %object.vnum% == %maceitem3% || %object.vnum% == %maceitem4% || %object.vnum% == %maceitem5% || %object.vnum% == %maceitem6%
            switch %object.vnum%
              case %maceitem2%
                set number 2
                break
              case %maceitem3%
                set number 3
                break
              case %maceitem4%
                set number 4
                break
              case %maceitem5%
                set number 5
                break
              case %maceitem6%
                set number 6
            done
            if %actor.quest_variable[phase_mace:task%number%]% == 1
              set response You already gave me %object.shortdesc%.
            elseif %macestep% == 2 && %number% > 2 && %actor.quest_variable[phase_mace:dirt%number%]% == 0
              wait 2
              shake
              mjunk %object%
              msend %actor%%self.name% scatters the dirt in the wind.
              wait 2
              msend %actor% %self.name% says, 'You need to make the pilgrimage to these places yourself.'
              wait 1s
              msend %actor% %self.name% says, 'Return with fresh earth when you have.'
            else
              quest variable phase_mace %actor% macetask%number% 1
              wait 2
              mjunk %object%
              msend %actor% %self.name% says, 'Ah yes, this will do.'
              set job1 %actor.quest_variable[phase_mace:macetask1]% 
              set job2 %actor.quest_variable[phase_mace:macetask2]%
              set job3 %actor.quest_variable[phase_mace:macetask3]%
              set job4 %actor.quest_variable[phase_mace:macetask4]%
              set job5 %actor.quest_variable[phase_mace:macetask5]%
              set job6 %actor.quest_variable[phase_mace:macetask6]%
              if %macestep% == 2
                if %job1% && %job2% && %job3% && %job4% && %job5% && %job6%
                  set check continue
                elseif !%job1% && %job2% && %job3% && %job4% && %job5% && %job6%
                  set check count
                else
                  set check stop
                endif
              else
                if %job1% && %job2% && %job3% && %job4% && %job5%
                  set check continue
                elseif !%job1% && %job2% && %job3% && %job4% && %job5%
                  set check count
                else
                  set check stop
                endif
              endif
            endif
          elseif %object.vnum% == %macevnum%
            if %self.vnum% == 3025 && !%actor.quest_stage[phase_mace]%
              quest start phase_mace %actor%
              wait 2
              msend %actor% %self.name% says, 'What an unusual mace...'
              give %object% %actor%
              wait 1s
              ponder
              wait 1s
              msend %actor% %self.name% says, 'I could bless it against the undead, if I had the proper materials.  Bring me the following:
              msend %actor%- &3&b%get.obj_shortdesc[55211]%&0 to use as a model
              msend %actor%- &3&b%get.obj_shortdesc[55577]%&0 and
              msend %actor%- &3&b%get.obj_shortdesc[13614]%&0 for their protection against malevolent spirits
              msend %actor%- &3&b%get.obj_shortdesc[58809]%&0 as a flame to ward against the dark
              msend %actor% &0 
              msend %actor% Also attack with %get.obj_shortdesc[340]% &3&b50&0 times to fully bond with it.'
              msend %actor% &0 
              msend %actor% %self.name% says, 'You can ask about your &6&b[mace progress]&0 at any time.'  
              halt
            endif
            set job1 %actor.quest_variable[phase_mace:macetask1]% 
            set job2 %actor.quest_variable[phase_mace:macetask2]%
            set job3 %actor.quest_variable[phase_mace:macetask3]%
            set job4 %actor.quest_variable[phase_mace:macetask4]%
            set job5 %actor.quest_variable[phase_mace:macetask5]%
            set job6 %actor.quest_variable[phase_mace:macetask6]%
            if %macestep% == 2
              if %job1% && %job2% && %job3% && %job4% && %job5% && %job6%
                set reward yes
              elseif !%job1% && %job3% && %job4% && %job5% && %job6%
                set reward count
              else
                set reward stop
              endif
            else
              if %job1% && %job2% && %job3% && %job4% && %job5%
                set reward yes
              elseif !%job1% && %job2% && %job3% && %job4% && %job5%
                set reward count
              else
                set reward stop
              endif
            endif
          endif
        else
          set response Tell me why you're here first.
        endif
      elseif %actor.quest_stage[phase_mace]% > %macestep%
        switch %actor.quest_stage[phase_mace]%
          case 2
            set response Someone familiar with the grave will be able to work on this mace.  Seek out the Sexton in the Abbey west of the Village of Mielikki.
            break
          case 3
            set response The Cleric Guild is capable of some miraculous crafting.  Visit the Cleric Guild Master in the Arctic Village of Ickle and talk to High Priest Zalish.  He should be able to help you.
            break
          case 4
            set response Continue with the Cleric Guild Masters.  Check in with the High Priestess in the City of Anduin.
            break
          case 5
            set response Sometimes to battle the dead, we need to use their own dark natures against them.  Few are as knowledgeable about the dark arts as Ziijhan, the Defiler, in the Cathedral of Betrayal.
            break
          case 6
            set response Return again to the Abbey of St. George and seek out Silania.  Her mastery of spiritual matters will be necessary to improve this mace any further.
            break
          case 7
            set response I'm loathe to admit it, but of the few remaining who are capable of improving your weapon, one is a priest of a god most foul.  Find Ruin Wormheart, that most heinous of Blackmourne's servitors.
            break
          case 8
            set response The most powerful force in the war against the dead is the sun itself.  Consult with the sun's Oracle in the ancient pyramid near Anduin.
            break
          case 9
            set response With everything prepared, return to the very beginning of your journey.  The High Priestess of Mielikki, the very center of the Cleric Guild, will know what to do.
        done
      else
        set response Your mace isn't advanced enough for me to work on yet.
      endif
    else
      set response Come back after you've grown some more.
    endif
  else
    set response This is only needed for the priestly classes.
  endif
endif
if %check% == continue
  wait 1s
  nod
  msend %actor% %self.name% says, 'Give me the %get.obj_noadesc[%macevnum%]%.'
elseif %check% == count
  wait 2
  msend %actor% %self.name% says, 'Now just keep practicing with %get.obj_shortdesc[%macevnum%]%.'
elseif %check% == stop
  wait 2
  msend %actor% %self.name% says, 'Do you have the other materials?'
elseif %reward% == yes
  quest advance phase_mace %actor%
  quest variable phase_mace %actor% attack_counter 0
  quest variable phase_mace %actor% greet 0
  set loop 1
  while %loop% < 7
    quest variable phase_mace %actor% macetask%loop% 0
    eval loop %loop% + 1
  done
  wait 1s
  mjunk %object%
  nod
  msend %actor% %self.name% says, 'Yes, this is everything.'
  wait 1s
  msend %actor% %self.name% inlays %get.obj_shortdesc[%maceitem2%]% into %get.obj_shortdesc[%macevnum%]%.
  wait 2s
  if %self.vnum% == 18502
    msend %actor%%self.name% recites several long prayers as he sprinkles the handfuls of dirt over %get.obj_shortdesc[%macevnum%]%.
  else
    msend %actor% %self.name% etches the markings from %get.obj_shortdesc[%maceitem3%]% to %get.obj_shortdesc[%macevnum%]%.
    wait 3s
    msend %actor% By the light of %get.obj_shortdesc[%maceitem5%]%, %self.name% sets recites several long prayers.
  endif
  wait 2s
  msend %actor% %get.obj_shortdesc[%macevnum%]% is transformed!
  wait 1s
  msend %actor% %self.name% says, 'Here you are, %get.obj_shortdesc[%reward_mace%]%!'
  mload obj %reward_mace%
  give mace %actor%
  eval expcap (%macestep% * 10)
  if %expcap% < 25
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
  msend %actor% &3&bYou gain experience!&0
  eval setexp (%expmod% * 10)
  set loop 0
  while %loop% < 10
    mexp %actor% %setexp%
    eval loop %loop% + 1
  done
elseif %reward% == count
  set response Keep practicing with %get.obj_shortdesc[%macevnum%]%.
elseif %reward% == stop
  set response Bring me the other materials first.
endif
if %response%
  return 0
  msend %actor%%self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, '%response%'
endif
if %self.vnum% == 18502
  mmobflag %self% sentinel off
endif
~
#327
phase wand play~
1 n 100
~
switch %self.vnum%
  case 59040
    set type air
    set wandnum 304
    set reward 305
    set wandname wand-sky
    break
  case 16107
    set type acid
    set wandnum 334
    set reward 335
    set wandname wand-tomb
    break
  case 32412
    set type fire
    set wandnum 314
    set reward 315
    set wandname wand-blazes
    break
  case 17309
    set type ice
    set wandnum 324
    set reward 325
    set wandname wand-snow
done
if %actor.quest_stage[%type%_wand]% == 6 && %actor.quest_variable[%type%_wand:wandtask5]% && (%actor.wearing[%wandnum%]% || %actor.inventory[%wandnum%]%)
  return 0
  quest advance %type%_wand %actor%
  oecho %get.obj_shortdesc[%wandnum%]% begins to resonate in harmony with %self.shortdesc%!
  wait 4s
  oecho %get.obj_shortdesc[%wandnum%]% vibrates right out of your hands!
  if %actor.wearing[%wandnum%]%
    oforce %actor% remove %wandname%
  endif
  oforce %actor% drop %wandname%
  opurge %wandname%
  oecho In a brilliant &7&bFLASH&0 %get.obj_shortdesc[%wandnum%]% transforms into %get.obj_shortdesc[%reward%]%!
  oload obj %reward%
  oforce %actor% get wand
  set expmod 9240
  * Adjust exp award by class so all classes receive the same proportionate amount
  switch %person.class%
    case Warrior
    case Berserker
    * 110% of standard 
        eval expmod (%expmod% + (%expmod% / 10))
        break
    case Paladin
    case Anti-Paladin
    case Ranger
    * 115% of standard
        eval expmod (%expmod% + ((%expmod% * 2) / 15))
        break
    case Sorcerer
    case Pyromancer
    case Cryomancer
    case Illusionist
    case Bard
    * 120% of standard
        eval expmod (%expmod% + (%expmod% / 5))
        break
    case Necromancer
    case Monk
    * 130% of standard
        eval expmod (%expmod% + (%expmod% * 2) / 5)
        break
    default
        set expmod %expmod%
  done
  osend %actor% &3&bYou gain experience!&0
  eval setexp (%expmod% * 10)
  set loop 0
  while %loop% < 5
    oexp %actor% %setexp%
    eval loop %loop% + 1
  done
  quest variable %type%_wand %actor% greet 0
  quest variable %type%_wand %actor% attack_counter 0
  set number 1
  while %number% <= 5
    quest variable %type%_wand %actor% wandtask%number% 0
    eval number %number% + 1
  done
endif
~
$~
