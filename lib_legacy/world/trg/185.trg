#18500
anti_dia_necro_subclass_prior_head_death~
0 f 100
~
* Subclass drop trigger
set person %actor%
set i %actor.group_size%
if %i%
   set a 1
   unset person
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[nec_dia_ant_subclass]% == 3
            set drophead 1
         endif
      elseif %person%
        eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[nec_dia_ant_subclass]% == 3
   set drophead 1
endif
if %drophead% == 1
   mload obj 8504
endif
~
#18501
silania_welcome~
0 g 100
~
wait 2
switch %actor.quest_stage[pri_pal_subclass]%
  case 1
    msend %actor% %self.name% says, 'You've returned!  Let's talk about your &6&bquest&0.'
    break
  case 2
  case 3
    msend %actor% %self.name% says, 'Have you returned with the bronze chalice the diabolists stole?'
    break
  default
    if %actor.class% /= Warrior
      switch %actor.race%
        case drow
        case faerie_unseelie
          set classgreet no
          break
        default
          set classgreet yes
          set maxlevel 25
      done
    elseif %actor.class% /= Cleric
      switch %actor.race%
        case drow
        case faerie_unseelie
          set classgreet no
          break
        default
          set classgreet yes
          set maxlevel 35
      done
    endif
    if %classgreet% == yes
      if %actor.level% >= 10 && %actor.level% <= %maxlevel%
        msend %actor% %self.name% says, 'Some know not of their &6&bdestinies&0, others simply choose to ignore them.  Which of the two are you?'
        wait 2
        ponder %actor.name%
      endif
    endif
done
if %actor.quest_stage[%type%_wand]% == %wandstep%
  eval minlevel (%wandstep% - 1) * 10
  if %actor.level% >= %minlevel%
    if %actor.quest_variable[%type%_wand:greet]% == 0
      msend %actor% %self.name% says, 'I see you're crafting something.  If you want my help, we can talk about &6&b[upgrades]&0.'
    else
      if %actor.quest_variable[%type%_wand:wandtask1]% && %actor.quest_variable[%type%_wand:wandtask2]% && %actor.quest_variable[%type%_wand:wandtask3]%
        msend %actor% %self.name% says, 'Oh good, you're all set!  Let me see the staff.'
      else
        msend %actor% %self.name% says, 'Do you have what I need for the %weapon%?'
      endif
    endif
  endif
endif
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
#18502
quest_preamble_silania~
0 d 1
destiny destiny? destinies destinies? I~
if (%speech% /= destiny || %speech% /= destinies || %speech% /= destiny? || %speech% /= destinies? || %speech% /= I know) && !%actor.quest_stage[pri_pal_subclass]%
  if %actor.class% /= Cleric
    switch %actor.race%
      case drow
      case faerie_unseelie
        if %actor.level% >= 10 && %actor.level% <= 35
          msend %actor% &1Your race may not subclass to priest.&0
          halt
        endif
        break
      default
        set classquest yes
    done
  elseif %actor.class% /= Warrior
    switch %actor.race%
      case drow
      case faerie_unseelie
        if %actor.level% >= 10 && %actor.level% <= 25
          msend %actor% &1Your race may not subclass to paladin.&0
          halt
        endif
        break
      default
        set classquest yes
    done
  else
    if %actor.level% >= 10 && %actor.level% <= 25
      msend %actor% %self.name% says, 'Sorry, I cannot help you achieve your destiny.'
    endif
  endif
endif
wait 2
if %classquest% == yes
  if %use_subclass%
    msend %actor% %self.name% says, 'I am currently helping another supplicant, one moment my child.''
    halt
  endif
  if %actor.align% > 349
    if %actor.class% /= Warrior && %actor.level% >= 10 && %actor.level% <= 25
      nod %actor.name%
      msend %actor% %self.name% says, 'Do ye wish to join the ranks of the holy warriors?'
      set use_subclass Pal
      global use_subclass
    elseif %actor.class% /= Cleric && %actor.level% >= 10 && %actor.level% <= 35
      nod %actor.name%
      msend %actor% %self.name% says, 'Do ye wish to join the ranks of the priests and help drive darkness from our world?'
      set use_subclass Pri
      global use_subclass
    elseif %actor.level% < 10
      msend %actor% %self.name% says, 'I appreciate your aspiring virtue.  Come and see me once you've gained more experience.'
    else
      msend %actor% %self.name% says, 'You are already following your destiny.  I cannot help you any further.'
    endif
  else
    msend %actor% %self.name% says, 'I cannot help you if you are not good, little one.'
  endif
endif
~
#18503
start_quest_silania~
0 d 100
yes no~
if %actor.class% /= Cleric
  switch %actor.race%
    case drow
    case faerie_unseelie
      if %actor.level% >= 10 && %actor.level% <= 35
        msend %actor% &1Your race may not subclass to priest.&0
        halt
      endif
      break
    default
      if %actor.level% >= 10 && %actor.level% <= 35
        set classquest yes
      elseif %actor.level% < 10
        msend %actor% %self.name% says, 'I appreciate your virtue.  Come and see me once you've gained more experience.'
      else
        msend %actor% %self.name% says, 'You are already following your destiny.  I cannot help you any further.'
      endif      
  done
endif
if %actor.class% /= Warrior
  switch %actor.race%
    case drow
    case faerie_unseelie
      if %actor.level% >= 10 && %actor.level% <= 25
        msend %actor% &1Your race may not subclass to paladin.&0
        halt
      endif
      break
    default
      if %actor.level% >= 10 && %actor.level% <= 25
        set classquest yes
      elseif %actor.level% < 10
        msend %actor% %self.name% says, 'I appreciate your virtue.  Come and see me once you've gained more experience.'
      else
        msend %actor% %self.name% says, 'You are already following your destiny.  I cannot help you any further.'
      endif
  done
endif
wait 2
if %classquest% == yes
  if %speech% /= yes
    if %actor.align% > 349 && !%actor.quest_stage[pri_pal_subclass]%
      if !%use_subclass%
        msend %actor% %self.name% says, 'First, we must discuss your destiny.'
      else
        quest start pri_pal_subclass %actor.name% %use_subclass%
        smile %actor.name%
        msend %actor% %self.name% says, 'You may check your &6&b[subclass progress]&0 or ask me to &6&b[repeat]&0 myself at any time.'
        wait 1s
        msend %actor% %self.name% says, 'It is necessary to make a quest such as this quite tough to ensure ensure you really want to do this.'
        wait 2s
        msend %actor% %self.name% says, 'I am sure you will complete the &6&bquest&0 though.'
      endif
    else
      msend %actor% %self.name% says, 'I cannot help you if you are not good little one.'
    endif
  else
    sigh
    msend %actor% %self.name% says, 'It is a shame we have so few willing to join us these days, but I cannot compel you.  Farewell, and I hope you come to reconsider your decision sometime.'
    wait 2
    msend %actor% Silania makes a gesture with her hand and you are blinded for a second.
    mechoaround %actor% Silania makes a gesture with her hand and %actor.name% disappears.
    mteleport %actor% 18566
  endif
  unset use_subclass
endif    
~
#18504
quest_details_silania~
0 d 100
quest quest?~
wait 2
if %actor.quest_stage[pri_pal_subclass]% == 1
  quest advance pri_pal_subclass %actor.name%
  msend %actor% %self.name% says, 'At this abbey we have always permitted any person to wander freely.'
  wait 1s
  ponder
  wait 1s
  msend %actor% %self.name% says, 'Well, not if they are evily aligned, but otherwise...'
  wait 2s
  msend %actor% %self.name% says, 'One of our guests made off with our most sacred bronze chalice.'
  steam
  wait 2
  msend %actor% %self.name% says, 'We have reason to believe it was a ruse by the filthy diabolists to try and weaken us.  Our Prior has offered to try and retrieve it for us, but...'
  wait 2
  think
  msend %actor% %self.name% says, 'I think perhaps that would be a bad idea and that you should find it instead.'
  wait 2
  msend %actor% %self.name% says, 'Good luck, your quest has begun!'
  bow
endif
~
#18505
give_chalice_silania~
0 j 100
18512~
wait 2
* stage is set to 3 when chalice retrieved
if %actor.quest_stage[pri_pal_subclass]% == 3
  emote beams a huge smile.
  wait 3
  hug %actor.name%
  msend %actor% %self.name% says, 'You have done me a great service, and earned your place among the truly holy.'
  smi
  wait 2
  msend %actor% %self.name% says, 'Type &7&b'subclass'&0 to proceed.'
  quest complete pri_pal_subclass %actor.name%
elseif %actor.quest_stage[pri_pal_subclass]% == 1
  msend %actor% %self.name% says, 'Hmmm, jumping the gun a bit aren't we?'
  wait 1s
  msend %actor% %self.name% says, 'I didn't even tell you the quest yet.'
else
  msend %actor% %self.name% says, 'Thank you greatly for returning this.'
  thank %actor.name%
  wait 2
  if %actor.quest_stage[pri_pal_subclass]% == 2
    * hmm..they got the chalice without running the appropriate trigger
    * could they be cheating?
    msend %actor% %self.name% says, 'Hmm... I don't know how you got hold of this, but I will give you the benefit of the doubt and not fail you.'
    poke %actor.name%
    wait 3s
    msend %actor% %self.name% says, 'Now... go and get the chalice properly yourself!'
  else
    msend %actor% %self.name% says, 'Shame you were not performing the quest.  Your selfless act would have qualified you for our holy brotherhood.'
  endif
endif
*junk the chalice
mjunk chalice
~
#18506
exit_from_silania~
0 d 100
exit exit?~
say Ooops, how embarassing, I forgot about the door!
m_run_room_trig 18507
~
#18507
create_door_18547_to_18566~
2 g 0
~
* only run from silanias trigger 18506
wait 2
wecho The walls seem to flow away from an opening.
wdoor 18547 east room 18566
wdoor 18566 west room 18547
~
#18508
close_door_18547_to_18566~
2 g 100
~
wait 5
set closeit 0
if %direction% /= east
if %self.vnum% == 18547
set closeit 1
endif
endif
if %direction% /= west
if %self.vnum% == 18566
set closeit 1
endif
endif
if %closeit% == 1
wecho The walls seem to flow together behind you, sealing the entrance!
wdoor 18547 east purge
wdoor 18566 west purge
endif
~
#18509
Priest_Paladin_Subclass_Quest_Silania_Status~
0 d 0
subclass progress~
wait 2
switch %actor.quest_stage[pri_pal_subclass]%
  case 1
    msend %actor% %self.name% says, 'You want to join the holy ranks of priests and paladins.'
    wait 1s
    msend %actor% %self.name% says, 'It is necessary to make a quest such as this quite tough to ensure you really want to do this.'
    wait 2s
    msend %actor% %self.name% says, 'I am sure you will complete the &6&bquest&0 though.'
    break
  case 2
    msend %actor% %self.name% says, 'This is your quest to join the ranks of priests and paladins.'
    wait 1s
    msend %actor% %self.name% says, 'One of our guests made off with our most sacred bronze chalice.'
    steam
    wait 2
    msend %actor% %self.name% says, 'We have reason to believe it was a ruse by the filthy diabolists to try and weaken us.  Our Prior has offered to try and retrieve it for us, but...'
    wait 2
    think
    msend %actor% %self.name% says, 'I think perhaps that would be a bad idea and that you should find it instead.'
    wait 2
    msend %actor% %self.name% says, 'Good luck, your quest has begun!'
    bow
    break
  case 3
    msend %actor% %self.name% says, 'Have you found the bronze chalice the diabolists stole?'
    break
  default
    if %actor.class% /= Cleric
      switch %actor.race%
        case drow
        case faerie_unseelie
          if %actor.level% >= 10 && %actor.level% <= 35
            msend %actor% &1Your race may not subclass to priest.&0
            halt
          endif
          break
        default
          set check yes
      done
    elseif %actor.class% /= Warrior
      switch %actor.race%
        case drow
        case faerie_unseelie
          if %actor.level% >= 10 && %actor.level% <= 25
            msend %actor% &1Your race may not subclass to paladin.&0
            halt
          endif
          break
        default
          set check yes
      done
    endif
done
if %check% == yes
  if (%actor.class% /= Warrior && %actor.level% >= 10 && %actor.level% <= 25) || (%actor.class% /= Cleric && %actor.level% >= 10 && %actor.level% <= 35)
    msend %actor% %self.name% says, 'You are not working to join the holy order.'
  elseif %actor.level% < 10
    msend %actor% %self.name% says, 'I appreciate your aspiring virtue.  Come and see me once you've gained more experience.'
  else
    msend %actor% %self.name% says, 'You are already following your destiny.  I cannot help you any further.'
  endif
endif
~
#18510
abbot_rand1~
0 b 10
~
fume
say Not another blackmail note about my prior.
emote rips up a letter into tiny pieces.
stomp
~
#18511
abbot_speak1~
0 d 100
prior prior?~
peer %actor.name%
say Anything you heard about Berack is pure hearsay.
sigh
say That young man has enough to worry about without people bringing up his family all the time.
~
#18512
abbot_rand2~
0 b 10
~
emote mutters about the tribulations of monastic life.
sigh
~
#18513
chalice_retrieval~
1 g 100
~
if %self.room% == 8591
  * we are in chalice room so set the quest bit
  if %actor.quest_stage[pri_pal_subclass]% == 2
    if %already_got% == 1
      oechoaround %actor% The chalice slips from %actor.p% fingers!
      osend %actor% The chalice slips from your fingers!
      return 0
    else
      quest advance pri_pal_subclass %actor.name%
    endif
  endif
  set already_got 1
  global already_got
endif
~
#18514
chalice_dropped~
1 h 100
~
* if the chalice is dropped it could be some1 trying to get round
* the GET trigger...this should prevent that
oecho the DROP trigger for %self.name% is RUNNING
~
#18515
almoner_give_food~
0 b 25
test~
set victim %random.char%
if (%victim.vnum% == -1) && (%victim.level% < 25) && (%victim.can_be_seen%)
   smile %victim.name%
   say Hello there, young one.  You look like you could use a bite to eat.
   mjunk waybread
   mload obj 18508
   give waybread %victim.name%
endif
~
#18516
group_heal_doctor_greet~
0 h 100
~
wait 2
set stage %actor.quest_stage[group_heal]%
if %actor.vnum% == -1 && %stage% == 0
  say Greetings!
  wait 2
  say What kind of healing I can assist you with today?
elseif %stage% == 1
  say Welcome back!
  wait 2
  say Have you been able to track down the bandit raiders yet?
  if %get.mob_count[18522]% == 0
    mat 1100 mload mob 18522
    mat 1100 mload obj 16106
    mat 1100 mload obj 16106
    mat 1100 give scimitar bandit
    mat 1100 give scimitar bandit
    mat 1100 mforce bandit wear all
    mat 1100 mteleport bandit 16186
  endif
elseif %stage% == 2
  say Welcome back!
  wait 2
  say Do you have the stolen supplies?
elseif %stage% == 3 || %stage% == 4
  say Welcome back!
  wait 2
  say Do you have the records of the ritual?
elseif %stage% == 5
  eval total 6 - %actor.quest_variable[group_heal:total]%
  if %total% == 6
    say Ah you've returned!
    wait 2
    say Have you been able to consult with any of the chefs?
  elseif %total% == 1
    say Ah you've returned!
    wait 2
    say Have you been able to consult with the last chef yet?
  else
    say Ah you've returned!
    wait 2
    say Have you been able to consult with any of the %total% remaining chefs?
  endif
  mecho %self.name% says, 'And if you need a new copy of the Rite, just say:
  mecho &0&3&b"I lost the Rite"&0 and I will give you a new one.'
elseif %stage% == 6
  say Good to see you again!
  wait 2
  say I hope the distribution of those medical packets is going well.
endif
~
#18517
group_heal_doctor_speech1~
0 d 1
healing healing? group assist?~
wait 2
nod
  mecho %self.name% says, 'I specialize in group healing.  It's a powerful spell
  mecho &0involving complex prayers.  Not easy to learn.'
if ((%actor.class% /= Cleric || %actor.class% /= Priest || %actor.class% /= Diabolist) && %actor.level% > 56) && %actor.quest_stage[group_heal]% == 0
  wait 3s
  mecho %self.name% says, 'But you look like a very capable healer!  Perhaps you can
  mecho &0help me out.'
  wait 3s
  mecho %self.name% says, 'I've heard there once was a way to create a portable
  mecho &0version of the spell.  I've been trying to figure it out, but most of my time
  mecho &0is spent tending to the abbey's injured.'
  wait 4s
  mecho %self.name% says, 'Raiders from Split Skull and bandits from the Gothra Desert
  mecho &0have made it very difficult to get anything done around here!'
  wait 3s
  mecho %self.name% says, 'If you can help me work on it, I can teach you the proper
  mecho &0prayers in exchange.'
  wait 3s
  say What do you say, are you interested in working with me?
endif
~
#18518
group_heal_doctor_speech2~
0 d 1
yes yes! yep yep! okay sure~
wait 2
if ((%actor.class% /= Cleric || %actor.class% /= Priest || %actor.class% /= Diabolist) && %actor.level% > 56)) && %actor.quest_stage[group_heal]% == 0
  quest start group_heal %actor.name%
  say Thank you so much!
  wait 1s
  say There is an immediate problem I need your help with.
  wait 2s
  mecho %self.name% says, 'A shipment of medical supplies was coming to us from Anduin
  mecho &0via the Black Rock Trail.  Unfortunately the transport was robbed and the
  mecho &0entire shipment was stolen by bandits.'
  wait 4s
  mecho %self.name% says, 'The few who survived said a fierce bandit raider attacked
  mecho &0their wagon and headed off into the desert with their belongings.  They said
  mecho &0he fought like nothing they had ever seen before.  Some kind of frenzy of
  mecho &0swords and daggers.'
  wait 6s
  mecho %self.name% says, 'Please, do whatever you need to recover those supplies and
  mecho &0bring them to me.'
  if %get.mob_count[18522]% == 0
    mat 1100 mload mob 18522
    mat 1100 mload obj 16106
    mat 1100 mload obj 16106
    mat 1100 give scimitar bandit
    mat 1100 give scimitar bandit
    mat 1100 mforce bandit wear all
    mat 1100 mteleport bandit 16186
  elseif %actor.quest_stage[group_heal]% == 3 || %actor.quest_stage[group_heal]% == 4
    say May I see them please?
  elseif %actor.quest_stage[group_heal]% == 5
    say May I see what they have to say?
  endif
endif
~
#18519
group_heal_bandit_death~
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
         if %person.quest_stage[group_heal]% == 1
            quest advance group_heal %person.name%
            mload obj 18513
         elseif %person.vnum% == -1
            set what_gem_drop %random.11%
            eval gem_vnum %what_gem_drop% + 55736
            mload obj %gem_vnum%
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[group_heal]% == 1
   quest advance group_heal %person.name%
   mload obj 18513
endif
~
#18520
group_heal_doctor_receive~
0 j 100
~
set stage %actor.quest_stage[group_heal]%
if %stage% == 2
  if %object.vnum% == 18513
    mjunk supplies
    wait 2
    say Thank you so much!  This is an enormous relief!
    wait 3s
    mecho %self.name% says, 'Now I can get to working on that ritual to create a portable
    mecho &0healing spell and teach you about healing groups of people in the process,
    mecho &0%actor.name%.'
    wait 4s
    mecho %self.name% says, 'Based on books I found in our library, historical records
    mecho &0indicate the last anyone heard of this spell a hearth priest was working on
    mecho &0perfecting it.  He worked out of a kitchen somewhere in the
    mecho &0&2Great Northern Swamps&0.'
    wait 4s
    mecho %self.name% says, 'I'm not sure how helpful that is, but maybe if you &7&bsearch&0
    mecho &0through some of the abandoned kitchens out there, you'll find something useful.'
    switch %random.3%
      case 1
      * Mystwatch
        set room 16051
        break
      case 2
      * Nordus
        set room 51012
        break
      default
      * Sunken
        set room 53178
        break
    done
    quest variable group_heal %actor.name% room %room%
    quest advance group_heal %actor.name%
  else
    return 0
    wait 2
    mecho %self.name% says, 'These aren't our missing supplies.
    msend %actor% %self.name% returns %object.shortdesc% to you.
  endif
elseif %stage% == 4
  if %object.vnum% == 18514
    quest advance group_heal %actor.name%
    wait 2
    emote scans through the book.
    wait 2s
    mecho %self.name% says, 'Strange, this doesn't look like a ritual at all, at least
    mecho &0not in the strictest sense.  It looks more like... like a meal.'
    wait 1s
    say A very BIG meal...
    wait 2s
    mecho %self.name% says, 'Huh... Well those sure are some unusual dishes.  I've never
    mecho &0seen anything quite like this before.  I don't even know what half these
    mecho &0ingredients are.'
    wait 4s
    say I'm a doctor, dammit, not a chef!
    wait 2s
    ponder
    wait 2s
    say A chef!  That's it!
    wait 3s
    mecho %self.name% says, 'Could you go consult with different &7&bchefs&0 and &7&bcooks&0 to
    mecho &0figure out what these things are?  I'm sure I can procure the ingredients once
    mecho &0I know what I actually need.'
    wait 5s
    say It looks like there are six parts to this.
    wait 1s
    give rite %actor.name%
    wait 2s
    mecho %self.name% says, 'Seek out a professional for each part, &7&bgive them the rite&0,
    mecho &0and get some notes from them.'
    wait 3s
    say Thank you for your help!
  else
    return 0
    wait 2
    say This doesn't look like it will be of any help.
    msend %actor% %self.name% returns %object.shortdesc% to you.
  endif
elseif %stage% == 5
  if %object.vnum% == 18515 || %object.vnum% == 18516 || %object.vnum% == 18517 || %object.vnum% == 18518 || %object.vnum% == 18519 || %object.vnum% == 18520
    if %actor.quest_variable[group_heal:%object.vnum%]% == 1
      return 0
      wait 2
      shake
      say You already brought me %object.shortdesc%.
      msend %actor% %self.name% returns %object.shortdesc% to you.
    else 
      mjunk notes
      quest variable group_heal %actor.name% %object.vnum% 1
      eval total %actor.quest_variable[group_heal:total]% + 1
      quest variable group_heal %actor.name% total %total%
      wait 2
      say Thank you!
      wait 1s
      emote studies the notes.
      wait 2s
      say Ah yes, I see.
      wait 1s
      switch %random.4%
        case 1
         say I can substitute that with that?  Interesting!
         break
        case 2
          say That looks like a very reasonable suggestion!
          break 
        case 3
          say Oh so that's what that thing is called now...
          break
        default
          say I see how this works!
      done
    endif
    if %total% == 6
      mjunk notes
      quest advance group_heal %actor.name%
      wait 1s
      mecho %self.name% says, 'That looks like everything!  May I have that copy of the Rite
      mecho &0please?'
    else 
      wait 1s
      say Please, bring me the notes on the rest of the rite!
    endif
  endif
elseif %stage% == 6
  if %object.vnum% == 18514
    quest variable group_heal %actor.name% total 0
    wait 2
    mjunk heroes-feast
    wait 2
    mecho %self.name% says, 'I think I can put this together now!!  Wait here while I
    mecho &0prepare the feast.'
    wait 2s
    emote leaves for the kitchens.
    mteleport %self% 1100
    wait 8s
    mat 18530 mecho Delicious smells and the sound of chanting begin to waft in from the courtyard.
    wait 10s
    mteleport %self% 18530
    emote returns with an incredible feast!
    wait 4s
    emote carefully divides up the feast and neatly packages it along with medical supplies.
    mload obj 18521
    mload obj 18521
    mload obj 18521
    mload obj 18521
    mload obj 18521
    wait 1s
    mecho %self.name% says, 'The final thing I ask is for you to go out into the world
    mecho &0and help those in need.'
    wait 4s
    say Here, take these care packages of food and medicine.
    wait 2
    give all.food %actor.name%
    wait 4s
    mecho %self.name% says, 'Seek out five creatures who are &b&7sick&0, &7&binjured&0, &7&bdying&0,
    mecho &0&7&bwounded&0, or &7&bhobbling&0. Bring them succor with those packages.'
    wait 4s
    say Except for Lirne.
    wait 2
    grumble
    wait 2s
    say He needs to learn his lesson eventually.
    wait 4s
    say May St. George ever smile upon your endeavors!
    wait 2
    wave %actor.name%
  else
    return 0
    say I'm afraid this won't be of much use to me right now.
    msend %actor% %self.name% returns %object.shortdesc% to you.
  endif
else 
  return 0
  say I'm afraid this won't be of much use to me right now.
  msend %actor% %self.name% returns %object.shortdesc% to you.
endif
~
#18521
group_heal_room_search~
2 c 100
search~
switch %arg%
  case s
  case se
    return 0
    halt
    break
done  
if (%actor.quest_stage[group_heal]% == 3) && (%actor.quest_variable[group_heal:room]% == %self.vnum%)
   wload obj 18514
   wechoaround %actor% %actor.name% finds %get.obj_shortdesc[18514]%!
   wsend %actor% &bYou have found %get.obj_shortdesc[18514]%!&0
   quest advance group_heal %actor.name%
else
   return 0
endif
~
#18522
group_heal_chefs_receive~
0 j 100
~
return 0
if %object.vnum% == 18514 && %actor.quest_stage[group_heal]% == 5
  if %actor.quest_variable[group_heal:%self.vnum%]%
    if %self.vnum% == 50203
      msend %actor% %self.name% moans and swats your hand away.
      mechoaround %actor% %self.name% moans and swats %actor.name%'s hand away.
    elseif %self.vnum% == 51007
      cackle %actor.name%
      msend %actor% %self.name% inches away from you.
      mechoaround %actor% %self.name% inches away from %actor.name%.
    else
      say I've told you everything I can.  Good luck!
    endif
  else
    quest variable group_heal %actor.name% %self.vnum% 1
    switch %self.vnum%
      case 8307
      * the Frakati Chef
        say So you would like some help?  Looks like quite a meal.
        wait 2s
        mecho %self.name% says, 'Roasting methods may be a little difficult to control.
        mecho &0I can share with you some methods the Frakati people use to prepare and roast
        mecho &0wild game.'
        wait 4s
        emote writes down several notes.
        wait 3s
        mload obj 18515
        give notes %actor.name%
        wait 1s
        mecho %self.name% says, 'These small tips will make sure your meats stay tender
        mecho &0and juicy while soaking up as much flavor as possible.'
        wait 2s
        say Good luck.
        break
      case 51007
      * the crazy chef
        emote begins to run in a circle laughing hysterically.
        wait 4s
        emote stops running and starts writing.
        wait 2s
        mecho %self.name% keeps laughing and writing...
        wait 2s
        mecho ... and writing...
        wait 2s
        mecho ... and writing...
        wait 2s
        mecho ... and writing...
        wait 4s
        mload obj 18516
        give notes %actor.name%
        laugh %actor.name%
        break
      case 18512
      * a scruffy cook
        mecho %self.name% says, 'Well what is this?  I heard the doctor talking about a
        mecho &0feast of some kind...'
        wait 2
        emote trails off while reading the ritual recipe.
        wait 4s
        mecho %self.name% says, 'Well I can definitely help out with some of this.
        mecho &0Desserts aren't considered a specialty of our Abbey because most bakers
        mecho &0prefer techniques and ingredients no longer suited for monastic life.'
        wait 3s
        mecho %self.name% says, 'But since we still see the value in what you can grow
        mecho &0and process by hand, I can easily help replicate a recipe this old.'
        wait 4s
        mload obj 18517
        emote writes down a list of notes.
        give notes %actor.name%
        wait 1s
        say This should meet the doctor's needs.
        break
      case 30003
      * Dugrik
        mecho %self.name% says, 'Normally I'd charge for this.  But since that's "illegal" here
        mecho &0now, I guess you got lucky.'
        wait 3s
        say Yeah, I see something I can help with.
        wait 3s
        emote points to several spots on the paper.
        say Make this adjustment here, here, and here.
        wait 4s
        emote points to another section.
        mecho %self.name% says, 'This word just means "boil."  It will do exactly the same thing if
        mecho &0you just boil it.'
        wait 4s
        mecho %self.name% says, 'Oh and be real careful with that one there.  Unless you got an
        mecho &0orcish constitution, cook it wrong and you can poison yourself.'
        chuckle
        wait 1s
        mload obj 18518
        give notes %actor.name%
        break
      case 50203
      * the zombified pirate cook
        mecho %self.name% moans.
        wait 2s
        emote shambles into the wall with a great THUD!
        mecho A book of barely preserved pieces of paper falls from a shelf onto what remains of %self.name%'s head.
        mload obj 18519
        wait 1s
        drop notes
        break
      default
        return 0
    done
    give notes %actor.name%
  endif
else
  if %self.vnum% == 50203
    msend %actor% %self.name% moans and swats your hand away.
    mechoaround %actor% %self.name% moans and swats %actor.name%'s hand away.
  elseif %self.vnum% == 51007
    cackle %actor.name%
    msend %actor% %self.name% inches away from you.
    mechoaround %actor% %self.name% inches away from %actor.name%.
  else
    say Sorry, what is this?
    msend %actor% %self.name% returns %object.shortdesc% to you.
  endif
endif
~
#18523
group_heal_injured_give~
1 i 100
~
if %actor.quest_stage[group_heal]% == 6
  set healed %actor.quest_variable[group_heal:total]%
  if %victim.vnum% == -1
    halt
  else
    return 0
    if %actor.quest_variable[group_heal:%victim.vnum%]%
      osend %actor% you have already helped %victim.name%
    elseif %victim.vnum% == 18506 || %victim.vnum% == 43020 || %victim.vnum% == 12513 || %victim.vnum% == 58803 || %victim.vnum% == 30054 || %victim.vnum% == 46414 || %victim.vnum% == 36103
      quest variable group_heal %actor.name% %victim.vnum% 1
      eval heal %healed% + 1
      quest variable group_heal %actor.name% total %heal%
      osend %actor% You give %self.shortdesc% to %victim.name% and apply the medicine to %victim.himher%.
      wait 1s
      oecho %victim.name%'s wounds begin to heal as they consume the magical feast.
      wait 2s
      if %victim.vnum% == 18506 || %victim.vnum% == 43020 || %victim.vnum% == 12513 || %victim.vnum% == 58803 || %victim.vnum% == 30054 ||
        oecho %victim.name% says, 'Thank you so much for coming to my aid!'
        wait 1s
        oecho %victim.name% bows and departs.
      elseif %victim.vnum% == 46414 || %victim.vnum% == 36103
        oforce %victim% nuzzle %actor.name%
        wait 1s
        oecho %victim.name% turns and departs.
      endif
      if %heal% >= 5
        oforce %victim% mskillset %actor% group heal
        opurge %victim%
        wait 1s
        osend %actor% The miraculous power of St. George washes over you!
        osend %actor% The appropriate prayers to beseech the gods for group heal well up in your soul.
        quest complete group_heal %actor.name%
        osend %actor% &7&bYou have learned Group Heal&0!
      else
        opurge %victim%
      endif
      opurge self
    elseif %victim.vnum% == 62506 || %victim.vnum% == 53450
      wait 1
      osend %actor% It is not possible to help %victim.name%.  
    else
      wait 1
      osend %actor% %victim.name% does not appear to be hurt.
    endif
  endif
endif
~
#18524
group_heal_rite_give~
1 i 100
~
if %actor.quest_stage[group_heal]% == 5
  if %victim.vnum% == 50203 || %victim.vnum% == 51007 || %victim.vnum% == 8307 || %victim.vnum% == 18512 || %victim.vnum% == 10308 || %victim.vnum% == 30003
    osend %actor% You show %victim.name% %self.shortdesc%.
    oechoaround %actor% %actor.name% shows %victim.name% %self.shortdesc%.
    halt
  else
    return 0
    osend %actor% You should not give away something so precious!
  endif
elseif %actor.quest_stage[group_heal]% == 6
  if %victim.vnum% == 18521
    halt
  else
    return 0
    osend %actor% You should not give away something so precious!
  endif
endif
~
#18525
group_heal_new_rite~
0 d 0
I lost the Rite~
wait 2
if %actor.quest_stage[group_heal]% == 5 || elseif %actor.quest_stage[group_heal]% == 6
  say You need to be more careful!
  wait 1s
  say Fortunately I made a copy of the original.
  mload obj 18514
  give rite-heroes-feast %actor%
endif
~
#18526
Sexton greet~
0 g 100
~
wait 2
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
#18527
**UNUSED**~
0 j 100
~
Nothing.
~
#18528
Phase mace Sexton speech upgrade~
0 d 100
upgrade upgrades upgrading mace improvement improvements improving~
18528 - Sexton upgrade speech
wait 2
if %actor.class% == cleric || %actor.class% == priest
  eval minlevel %macestep% * 10
  if %actor.quest_stage[phase_mace]% == %macestep%
    if %actor.level% >= %minlevel%
      quest variable phase_mace %actor% greet 1
      mecho %self.name% says, 'I can add benedictions to your mace, yes.'
      wait 1s
      say You'll need to use your mace %maceattack% times.
      mecho  
      say I'll also need the following:
      mecho - &6&b%get.obj_shortdesc[%maceitem3%]%&0, for its spiritual protection.
      if %macestep% != 2
        mecho - &6&b%get.obj_shortdesc[%maceitem2%]%&0, as a model for the new mace.
        mecho - &6&b%get.obj_shortdesc[%maceitem3%]%&0, for its power in fighting the undead.
        mecho - &6&b%get.obj_shortdesc[%maceitem4%]%&0, for its guiding light.
      else
        mecho  
        mecho %self.name% says, 'And finally, you'll need to make a pilgrimage to four burial
        mecho &0grounds to see what lies ahead on your journey:'
        mecho - &5the necropolis near Anduin&0
        mecho - &5the pyramid in the Gothra desert&0
        mecho - &5the ancient barrow in the Iron Hills&0
        mecho - &5the cemetary outside the Cathedral of Betrayal&0
        mecho 
        mecho %self.name% says, 'Be warned, these places are &1exceedingly dangerous!&0  All you
        mecho &0have to do is &6&b[dig]&0 up a handful of dirt from each place and bring it
        mecho &0back to me.'
      endif
      mecho   
      mecho %self.name% says, 'You can check your &6&b[mace progress]&0 at
      mecho &0any time.'
    else
      mecho %self.name% says, 'You'll need to be at least level %minlevel% before I
      mecho &0can improve your bond with your weapon.'
    endif
  elseif %actor.has_completed[phase_mace]%
    say There is no weapon greater than the mace of disruption!
  elseif %actor.quest_stage[phase_mace]% < %macestep%
    say Your mace isn't ready for improvement yet.
  elseif %actor.quest_stage[phase_mace]% > %macestep%
    say I've done all I can already.
    wait 1s
    switch %actor.quest_stage[phase_mace]%
      case 3
        mecho %self.name% says, 'The Cleric Guild is capable of some miraculous
        mecho &0crafting.  Visit the Cleric Guild Master in the Arctic Village of Ickle
        mecho &0and talk to High Priest Zalish.  He should be able to help you.'
        break
      case 4
        mecho %self.name% says, 'Continue with the Cleric Guild Masters.  Check in
        mecho &0with the High Priestess in the City of Anduin.'
        break
      case 5
        mecho %self.name% says, 'Sometimes to battle the dead, we need to use their
        mecho &0own dark natures against them.  Few are as knowledgeable about the dark
        mecho &0arts as Ziijhan, the Defiler, in the Cathedral of Betrayal.'
        break
      case 6
        mecho %self.name% says, 'Return again to the Abbey of St. George and seek out
        mecho &0Silania.  Her mastry of spiritual matters will be necessary to improve
        mecho &0this mace any further.'
        break
      case 7
        mecho %self.name% says, 'Of the few remaining who are capable of improving your
        mecho &0mace, one is a priest of a god most foul.  Find Ruin Wormheart, that
        mecho &0most heinous of Blackmourne's servators.'
        break
      case 8
        mecho %self.name% says, 'The most powerful force in the war against the dead
        mecho &0is the sun itself.  Consult with the sun's Oracle in the ancient pyramid
        mecho &0near Anduin.'
        break
      case 9
        mecho %self.name% says, 'With everything prepared, return to the very
        mecho &0beginning of your journey.  The High Priestess of Mielikki, the very
        mecho &0center of the Cleric Guild, will know what to do.'
    done  
  endif
endif
~
#18529
Sexton refuse~
0 j 100
~
switch %object.vnum%
  case %maceitem2%
  case %maceitem3%
  case %maceitem4%
  case %maceitem5%
  case %macevnum%
  case %maceitem6%
    halt
    break
  default
    set response I'm sorry, what is this for?
done
if %response%
  return 0
  msend %actor% %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, '%response%'
endif
~
#18531
Silania refuse~
0 j 0
18512~
switch %object.vnum%
  case %maceitem2%
  case %maceitem3%
  case %maceitem4%
  case %maceitem5%
  case %wandgem%
  case %wandvnum%
  case %macevnum%
    halt
    break
  default
    set response What is this for?
done
if %response%
  return 0
  msend %actor%%self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor
  msend %actor% %self.name% says, '%response%'
endif
~
#18550
font load holy water~
1 o 100
~
oload obj 18528
~
#18551
**UNUSED**~
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
   endif
   if %will_drop% <= 50
      mload obj %vnum_trophy1%
   elseif (%will_drop% >= 51 &%will_drop% <= 70)
      mload obj %vnum_trophy2%
   elseif (%will_drop% >= 71 &%will_drop% <= 90)
      mload obj %vnum_trophy3%
   else
      mload obj %vnum_trophy4%
   endif
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
   elseif (%bonus% >= 51 &%bonus% <= 90)
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
elseif (%will_drop% >=61 &%will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55323
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
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
   elseif (%bonus% >= 51 &%bonus% <= 90)
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
#18552
p2_3eg_death_p2_55574_boss_18~
0 g 100
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
   endif
   if %will_drop% <= 50
      mload obj %vnum_trophy1%
   elseif (%will_drop% >= 51 &%will_drop% <= 70)
      mload obj %vnum_trophy2%
   elseif (%will_drop% >= 71 &%will_drop% <= 90)
      mload obj %vnum_trophy3%
   else
      mload obj %vnum_trophy4%
   endif
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* boss setup - 55574 boss
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
   elseif (%bonus% >= 51 &%bonus% <= 90)
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
elseif (%will_drop% >=61 &%will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55319
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
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
   elseif (%bonus% >= 51 &%bonus% <= 90)
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
#18566
kneel_door_open~
2 c 100
kneel~
return 0
switch %cmd%
  case k
    halt
done
if %actor.align% > -349
  wsend %actor.name% A bright white beam of light descends upon you.
  wechoaround %actor.name% A bright beam of white light descends upon %actor.name%.
  wdoor 18566 west flags a
  wdoor 18566 west room 18547
  wdoor 18566 west name large wooden door
  wdoor 18566 west key -1
  wdoor 18566 west description A large wooden door bars your way.
  wait 3
  wsend %actor.name% A door in the cross to the west opens silently.
  wechoaround %actor.name% A door in the cross to the west opens silently.
else
  wsend %actor.name% A bright white beam of light descends upon you.
  wechoaround %actor.name% A bright white beam of light descends upon %actor.name%.
  wait 2
  wsend %actor.name% The light becomes increasingly bright, and turns painful!
  wdamage %actor.name% 179
  wechoaround %actor.name% The light brightens significantly, burning %actor.name%'s skin.
endif
~
#18567
**UNUSED**~
2 c 100
k~
return 0
~
#18599
group_heal_status_check~
0 d 100
status status? progress progress?~
set stage %actor.quest_stage[group_heal]%
wait 2
switch %stage%
  case 1
mecho %self.name% says, 'Please track down the bandit raider somewhere in the
    mecho &0Gothra desert and recover our stolen medical supplies.'
    if %get.mob_count[18522]% == 0
      mat 1100 mload mob 18522
      mat 1100 mload obj 16106
      mat 1100 mload obj 16106
      mat 1100 give scimitar bandit
      mat 1100 give scimitar bandit
      mat 1100 mforce bandit wear all
      mat 1100 mteleport bandit 16186
    endif
    break
  case 2
    say Please find the medical supplies stolen by the bandit raider.
    break
  case 3
  case 4
mecho %self.name% says, 'You are currently trying to locate the records of a group
    mecho &0healing ritual in a lost kitchen in the Great Northern Swamp.'
    break
  case 5
    mecho %self.name% says, 'You are visiting every &7&bchef&0 and &7&bcook&0 to get their notes on
    mecho &0the healing ritual.'
    mecho 
    set recipe1 %actor.quest_variable[group_heal:18515]%
    set recipe2 %actor.quest_variable[group_heal:18516]%
    set recipe3 %actor.quest_variable[group_heal:18517]%
    set recipe4 %actor.quest_variable[group_heal:18518]%
    set recipe5 %actor.quest_variable[group_heal:18519]%
    set recipe6 %actor.quest_variable[group_heal:18520]%
    if %recipe1% || %recipe2% || %recipe3% || %recipe4% || %recipe5% || %recipe6%
      mecho &0You have already brought me notes from:
      if %recipe1%
        mecho - %get.mob_shortdesc[8307]%
      endif
      if %recipe2%
        mecho - %get.mob_shortdesc[51007]%
      endif
      if %recipe3%
        mecho - %get.mob_shortdesc[18512]%
      endif
      if %recipe4%
        mecho - %get.mob_shortdesc[30003]%
      endif
      if %recipe5%
        mecho - %get.mob_shortdesc[50203]%
      endif
      if %recipe6%
        mecho - %get.mob_shortdesc[10308]%
      endif
      mecho 
      eval total 6 - %actor.quest_variable[group_heal:total]%
      mecho &0Bring me notes from %total% more chefs.
      mecho    
      mecho %self.name% says, 'And if you need a new copy of the Rite, just say:
      mecho &0&3&b"I lost the Rite"&0 and I will give you a new one.'
      break
  case 6
    mecho %self.name% says, 'You are delivering the medical packages to &7&binjured&0, &7&bwounded&,
    mecho &0&7&bsick&0, or &7&bhobbling&0 creatures.'
    eval total (5 - %actor.quest_variable[group_heal:total]%)
    set person1 %actor.quest_variable[group_heal:18506]%
    set person2 %actor.quest_variable[group_heal:46414]%
    set person3 %actor.quest_variable[group_heal:43020]%
    set person4 %actor.quest_variable[group_heal:12513]%
    set person5 %actor.quest_variable[group_heal:36103]%
    set person6 %actor.quest_variable[group_heal:58803]%
    set person7 %actor.quest_variable[group_heal:30054]%
    mecho 
    if %person1% || %person2% || %person3% || %person4% || %person5% || %person6% || %person7% 
      mecho You have aided:
      if %person1%
        mecho - %get.mob_shortdesc[18506]%
      endif
      if %person2%
        mecho - %get.mob_shortdesc[46414]%
      endif
      if %person3%
        mecho - %get.mob_shortdesc[43020]%
      endif
      if %person4%
        mecho - %get.mob_shortdesc[12513]%
      endif
      if %person5%
        mecho - %get.mob_shortdesc[36103]%
      endif
      if %person6%
        mecho - %get.mob_shortdesc[58803]%
      endif
      if %person7%
        mecho - %get.mob_shortdesc[30054]%
      endif
    endif
    mecho 
    if %total% == 1
      mecho You need to deliver %total% more packet.
    else
      mecho You need to deliver %total% more packets.
    break
  default
    if %actor.has_completed[group_heal]%
      say You finished the quest to learn Group Heal already.
    else
      say You aren't working on a quest with me.
    endif
done
~
$~
