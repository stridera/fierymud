#16300
quest_eleweiss_opening~
2 c 0
look~
if %arg% /= scrape || %arg% /= mark || %arg% /= branch
wsend %actor% A sudden gust of air catches hold of you, swirling around and moving you!
wechoaround %actor% %actor.name% is swallowed by swirling air and disappears.
wdoor 16374 up room 16375
wecho The wind whips around and around.
wat 16375 wecho There is a sudden gust of wind.
wat 16375 wecho %actor.name% is pushed in by the gust of wind which ceases promptly afterwards.
wforce %actor% up
wdoor 16374 up purge
else
return 0
endif
~
#16301
quest_eleweiss_ranger_druid_subclass_greet~
0 h 100
~
wait 2
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
switch %actor.race%
*  case ADD NEW RESTRICTED RACES HERE
*    halt
*    break
  default
    if (%actor.class% /= Warrior && (%actor.level% >= 10 && %actor.level% <= 25)) || (%actor.class% /= Cleric && (%actor.level% >= 10 && %actor.level% <= 35))
      if !%actor.quest_stage[ran_dru_subclass]%
        emote grins casually.
        msend %actor% %self.name% says, 'Some know the ways of the woods, others are ignorant.  Do you know the &6&bways&0 or not?'
        emote puts a more serious look on his face.
      elseif %actor.quest_stage[ran_dru_subclass]% == 4 && !%actor.has_completed[ran_dru_subclass]%
        msend %actor% %self.name% says, 'Have you the jewel of the heart?'
      endif
    endif
done
~
#16302
quest_eleweiss_ranger_druid_subclass_speak1~
0 d 100
ways ways? woods woods? I~
if (%speech% /= ways || %speech% /= ways? || %speech% /= I know) && !%actor.quest_stage[ran_dru_subclass]%
  if %actor.class% /= Cleric
    switch %actor.race%
*      case ADD NEW RESTRICTED RACES HERE
*        if %actor.level% >= 10 && %actor.level% <= 35
*          msend %actor% &1Your race may not subclass to druid.&0
*          halt
*        endif
*        break
      default
        wait 2
        if %actor.level% >= 10 && %actor.level% <= 35
          if %use_subclass%
            msend %actor% %self.name% says, 'I'm currently assisting someone else, one moment please.'
            halt
          endif
          if (%actor.align% > -350 && %actor.align% < 350)
            smile %actor.name%
            msend %actor% %self.name% says, 'Do you wish to join the ranks of the woodland healers with all the power there of?'
            set use_subclass Dru
            global use_subclass
          else
            msend %actor% %self.name% says, 'You are not properly aligned to be a woodland cleric.  Come back when you fix that.'
          endif
        elseif %actor.level% < 10
          msend %actor% %self.name% says, 'Seek me again later when you have gained some more experience.'
        else
          msend %actor% %self.name% says, 'You have traveled too far on your current path to change your way.'
        endif      
      done
  elseif %actor.class% /= Warrior
    switch %actor.race%
*    case ADD NEW RESTRICTED RACES HERE
*        if %actor.level% >= 10 && %actor.level% <= 25
*          msend %actor% &1Your race may not subclass to ranger.&0
*          halt
*        endif
*        break
      default
        wait 2
        if %actor.level% >= 10 && %actor.level% <= 25
          if %use_subclass%
            msend %actor% %self.name% says, 'I'm currently assisting someone else, one moment please.'
            halt
          endif
          if %actor.align% > 349
            nod %actor.name%
            msend %actor% %self.name% says, 'Do you wish to become a fighter who is one with the forests?'
            set use_subclass Ran
            global use_subclass
          else
            msend %actor% %self.name% says, 'Sorry, your alignment is not proper for what you wish to be.  Try again later.'
          endif
        elseif %actor.level% < 10
          msend %actor% %self.name% says, 'Seek me again later when you have gained some more experience.'
        else
          msend %actor% %self.name% says, 'You have traveled too far on your current path to change your way.'
        endif  
    done
  else
    wait 2
    msend %actor% %self.name% says, 'I can do nothing for you, I am sorry.'
    emote smiles a little bit.
  endif
endif
~
#16303
quest_eleweiss_ranger_druid_subclass_speak2~
0 d 100
yes yes? yes! no no? no!~
if %actor.quest_stage[ran_dru_subclass]% == 0 
  if %actor.class% /= Cleric
*    case ADD NEW RESTRICTED RACES HERE
*        if %actor.level% >= 10 && %actor.level% <= 35
*          msend %actor% &1Your race may not subclass to druid.&0
*          halt
*        endif
*        break
      default
        if %actor.level% >= 10 && %actor.level% <= 35
          set classquest yes
        elseif %actor.level% < 10
          msend %actor% %self.name% says, 'I cannot guide you yet.  Return to me when you have more experience.'
        elseif %actor.level% > 35
          msend %actor% %self.name% says, 'You have traveled too far on your current path to change your way.'
        endif
    done
  elseif %actor.class% /= Warrior
    switch %actor.race%
*    case ADD NEW RESTRICTED RACES HERE
*        if %actor.level% >= 10 && %actor.level% <= 25
*          msend %actor% &1Your race may not subclass to ranger.&0
*          halt
*        endif
*        break
      default
        if %actor.level% >= 10 && %actor.level% <= 25
          set classquest yes
        elseif %actor.level% < 10
          msend %actor% %self.name% says, 'I cannot guide you yet.  Return to me when you have more experience.'
        elseif %actor.level% > 25
          msend %actor% %self.name% says, 'You have traveled too far on your current path to change your way.'
        endif
    done
  endif
endif
wait 2
if %classquest% == yes
  if %speech% /= yes
    if !%use_subclass%
      msend %actor% %self.name% says, 'I cannot help until we talk about the ways of the woods.'
      halt
    endif
    quest start ran_dru_subclass %actor.name% %use_subclass%
    msend %actor% %self.name% says, 'Only the most dedicated to the forests shall complete the &6&bquest&0 I set upon you.  You may inquire about your &6&b[subclass progress]&0 or ask me to &6&b[repeat]&0 myself at any time.'
  else
    msend %actor% %self.name% says, 'Leave my sight, you tire me.'
    wave %actor.name%
    msend %actor% Eleweiss calls upon the air and moves you away.
    mechoaround %actor% Eleweiss removes %actor.name% from his presence.
    mteleport %actor% 16374
  endif
  unset use_subclass
endif
~
#16304
quest_eleweiss_ranger_druid_subclass_speak3~
0 d 100
quest quest?~
wait 2
if %actor.quest_stage[ran_dru_subclass]% == 1
  quest advance ran_dru_subclass %actor.name%
  msend %actor% %self.name% says, 'Yes, quest.  I do suppose it would help if I told you about it.'
  emote rubs his chin thoughtfully.
  wait 1s
  msend %actor% %self.name% says, 'Long ago I &6&blost something&0.  It is a shame, but it has never been recovered.'
  sigh
  msend %actor% %self.name% says, 'If you were to help me with that, then we could arrange something.'
  emote looks hopeful.
endif
~
#16305
quest_eleweiss_ranger_druid_subclass_speak4~
0 d 100
something something? lost lost? it it? thing thing? what what?~
wait 2
if %actor.quest_stage[ran_dru_subclass]% == 2
  sigh
  msend %actor% %self.name% says, 'It seems I am becoming forgetful in my age.'
  wait 2s
  msend %actor% %self.name% says, 'Well, you see now, I lost the jewel of my heart.  If you are up to it, getting that and returning it to me will get you your reward.'
  shrug
  wait 2
  msend %actor% %self.name% says, 'But for now, it is time for you to depart I think.'
  sigh
  msend %actor% %self.name% says, 'You have brought up painful memories for me to relive.'
  quest advance ran_dru_subclass %actor.name%
endif
~
#16306
quest_eleweiss_ranger_druid_subclass_exit~
0 d 100
exit exit? leave leave?~
say Very well, goodbye little one.
msend %actor% A gust of wind, commanded by Eleweiss, catches you and moves you away.
mechoaround %actor% A gust of wind from Eleweiss moves %actor.name% away.
mteleport %actor% 16374
~
#16307
quest_ranger_druid_subclass_heart_limiter~
1 g 100
~
if %actor.quest_stage[ran_dru_subclass]% == 3
   quest advance ran_dru_subclass %actor.name%
endif
~
#16308
quest_eleweiss_ranger_druid_subclass_receive~
0 j 100
17212~
wait 2
if %actor.quest_stage[ran_dru_subclass]% == 4
  emote throws back his head and howls loudly with pleasure.
  wait 3
  shake %actor.name%
  msend %actor% %self.name% says, 'You have done me a great service!  I cannot believe I have it back!'
  grin
  wait 2
  msend %actor% %self.name% says, 'Type &2&b'subclass'&0 to proceed.'
  quest complete ran_dru_subclass %actor.name%
else
  switch %actor.quest_stage[ran_dru_subclass]%
    case 1
    case 2
      msend %actor% %self.name% says, 'Woah, slow down, I have not even told you the quest yet!'
      break
    case 3
      msend %actor% %self.name% says, 'How on earth did you bring me the jewel of the heart without getting it yourself?'
      chuckle
      break
    default
      msend %actor% %self.name% says, 'Nice little gift, thanks much.'
      emote tucks it into his cloak.
      wait 2
      msend %actor% %self.name% says, 'Pity you were not on a quest for me, this could have been worth it for you.'
  done
endif
mjunk jewel
~
#16309
quest_eleweiss_ranger_druid_subclass_status~
0 d 0
subclass progress~
wait 2
set halfelf half-elf
set stage %actor.quest_stage[ran_dru_subclass]%
switch %stage%
  case 1
    msend %actor% %self.name% says, 'Only the most dedicated to the forests shall complete the &6&bquest&0 I set upon you.'
    break
  case 2
    msend %actor% %self.name% says, 'Yes, quest. I do suppose it would help if I told you about it.'
    emote rubs his chin thoughtfully.
    wait 1s
    msend %actor% %self.name% says, 'Long ago I &6&blost something&0.  It is a shame, but it has never been recovered.'
    sigh
    msend %actor% %self.name% says, 'If you were to help me with that, then we could arrange something.'
    emote looks hopeful.
    break
  case 3
  case 4
    sigh
    msend %actor% %self.name% says, 'It seems I am becoming forgetful in my age.'
    wait 2s
    msend %actor% %self.name% says, 'Well, you see now, I lost the jewel of my heart. If you are up to it, finding and returning it to me will get you your reward.'
    shrug
    wait 2
    msend %actor% %self.name% says, 'But for now, it is time for you to depart I think.'
    sigh
    msend %actor% %self.name% says, 'You have brought up painful memories for me to relive.'
    break
  default
    if %actor.class% /= Cleric
      switch %actor.race%
*    case ADD NEW RESTRICTED RACES HERE
*          if %actor.level% >= 10 && %actor.level% <= 35
*            msend %actor% &1Your race may not subclass to druid.&0
*            halt
*          endif
*          break
        default
          if %actor.level% >= 10 && %actor.level% <= 35
            msend %actor% %self.name% says, 'You are not on this quest.'
          elseif %actor.level% < 10
            msend %actor% %self.name% says, 'Seek me again later when you have gained some more experience.'
          else
            msend %actor% %self.name% says, 'You have traveled too far on your current path to change your way.'
          endif
      done
    elseif %actor.class% /= Warrior
      switch %actor.race%
*    case ADD NEW RESTRICTED RACES HERE
*          if %actor.level% >= 10 && %actor.level% <= 25
*            msend %actor% &1Your race may not subclass to ranger.&0
*            halt
*          endif
*          break
        default
          if %actor.level% >= 10 && %actor.level% <= 25
            msend %actor% %self.name% says, 'You are not on this quest.'
          elseif %actor.level% < 10
            msend %actor% %self.name% says, 'Seek me again later when you have gained some more experience.'
          else
            msend %actor% %self.name% says, 'You have traveled too far on your current path to change your way.'
          endif      
      done
    endif
done
~
#16310
Eleweiss refuse~
0 j 0
17212~
switch %object.vnum%
  case %wandgem%
  case %wandvnum%
    halt
    break
  default
    return 0
    wait 2
    msend %actor% %self.name% says, 'What is this?'
    wait 3
    msend %actor% %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'I have no need of %object.shortdesc%.'
done
~
#16339
dryad_moonwell_greet~
0 g 100
~
if (%actor.class% == druid)
   if (%actor.level% >= 73)
      switch (%actor.quest_stage[moonwell_spell_quest]%)      
         case 1
         case 2
            wait 5
            if %actor.sex% == male
              msend %actor% %self.name% tells you, 'Welcome back Brother.'
            elseif %actor.sex% == female
              msend %actor% %self.name% tells you, 'Welcome back Sister.'
            endif
            wait 1s
            msend %actor% %self.name% tells you, 'Have you recovered the Vine of Mielikki?!  Please give it
            msend %actor% &0to me.'
            mechoaround %actor% %self.name% asks %actor.name% something.
            break
         case 3
            wait 5
            msend %actor% %self.name% tells you, 'Do you have the stone?  Please, give it to me!'
            smile %actor.name%
            mechoaround %actor% %self.name% asks %actor.name% something.
            break
            *line 20
         case 4
         case 5
            wait 5
            msend %actor% tells you, 'Do you have the flask?  Please give it to me if you do!'
            mechoaround %actor% %self.name% asks %actor.name% something.
            break
         case 6
            wait 5
            msend %actor% %self.name% tells you, 'Did you get the stone of power?  Please give it to me!'
            mechoaround %actor% %self.name% asks %actor.name% something.
            break
         case 7
            wait 5
            msend %actor% %self.name% tells you, 'Do you have the orb from the dark fortress?  Please give
            msend %actor% &0me the orb.'
            mechoaround %actor% %self.name% asks %actor.name% something.
            break
         case 8
         case 9
            wait 5
            msend %actor% %self.name% tells you, 'The second stone of power, you have it?  Please give it
            msend %actor% &0to me, we are almost complete!'
            mechoaround %actor% %self.name% asks %actor.name% something.
            break
         case 10
            wait 5
            msend %actor% %self.name% tells you, 'The orb to balance the ritual, do you have it?  Please
            msend %actor% &0give me the orb!'
            mechoaround %actor% %self.name% asks %actor.name% something.
            break
         case 11
         case 12
            wait 5
            msend %actor% %self.name% tells you, 'The last stone of power, do you have it?  Please give me
            msend %actor% &0the last relic and we are finished!'
            mechoaround %actor% %self.name% asks %actor.name% something.
            break
         default
            wait 5
            mecho %self.name% weeps lightly.
            wait 10
            mechoaround %actor% %self.name% takes notice of %actor.name% and quickly wipes her tears.
            msend %actor% %self.name% takes notice of you and quickly wipes her tears.
            wait 5
            msend %actor% %self.name% tells you, 'Hello fellow of the Order.'
            mechoaround %actor% %self.name% tells something to %actor.name%.
            wait 10
            msend %actor% %self.name% tells you, 'It is good to see a promising druid again.'
            mecho %self.name% smiles through her tears.
      done
   else
      wait 5
      msend %actor% %self.name% looks you over and says, 'It is good to see a druid again, but you
      msend %actor% &0have not yet seen enough of the world to help me.  Please come back when you
      msend %actor% &0are more experienced.'
      *line 80
      mechoaround %actor% %self.name% tells %actor.name% something.
   endif
endif
~
#16340
dryad_random~
0 b 8
~
whine
~
#16341
dryad_moonwell_again~
0 d 100
druid? promising? again?~
*
* Created by Acerite Oct, 2004
* Check to see if Actor is eligible, then set their name in the global moon_name for future trigger.
*
if (%actor.class% == druid)
   if (%actor.level% >= 73)
      set moon_name %actor.name%
      global moon_name
      wait 5
      sigh
      wait 10
      msend %actor% %self.name% tells you, 'Yes, well...'
      mechoaround %actor% %self.name% tells something to %actor.name%.
      wait 5
      mecho %self.name% starts to reminisce about her past.
      wait 5
      msend %actor% %self.name% tells you, 'You see, I was not always like this.  Many years ago I
      msend %actor% &0too was a young mortal druid.  But while on my journeys I made a grave error in
      msend %actor% &0judgement.'
      wait 3s
      msend %actor% %self.name% tells you, 'I rediscovered powerful magics that allowed me to travel
      msend %actor% &0the world on a whim.'
      wait 3s
      msend %actor% %self.name% tells you, 'But I squandered that magic on frivolous desires and
      msend %actor% &0furthering my own ambitions, rather than using it to better perform my sworn
      msend %actor% &0duties.'
      wait 4s
      msend %actor% %self.name% tells you, 'Thus the great goddess Mielikki punished me by binding me
      msend %actor% &0to this tree as a dryad so I might never travel again.'
      wait 4s
      msend %actor% %self.name% tells you, 'Now, if I do not perform my duties properly, I ensure my
      msend %actor% &0own death.'
      mechoaround %actor% %self.name% tells her story to %actor.name%.
      wait 7s
      sigh
      wait 5
      ponder
      wait 5
      msend %actor% %self.name% tells you, 'But perhaps if I repent by teaching you these ancient
      msend %actor% &0magics, the goddess will release me from this place!'
      wait 4s
      msend %actor% %self.name% tells you, 'Yes, if you are willing I will teach you the power of
      msend %actor% &0druidic travel.'
      mechoaround %actor% %self.name% seems excited while telling something to %actor.name%.
      wait 4s
      msend %actor% %self.name% tells you, 'Are you willing to learn?'
   endif
endif
~
#16342
dryad_moonwell_yesno~
0 d 100
yes no~
*
*Created By Acerite Oct, 2004
*Okay we check to see if the same person that we've been talking to is the one responding
*Then we free the variable
*
if (%actor.name% == %moon_name%)
   unset moon_name
   if (%speech% /=no)
      frown
      msend %actor% %self.name% tells you, 'Very well, that is your choice.'
      mechoaround %actor% %self.name% seems disappointed as she says something to %actor.name%.
   endif
   if (%speech% /= yes)
      mecho %self.name% smiles with a slight twinkle in her eye.
      wait 5
      msend %actor% %self.name% tells you, 'Very well, I will teach you what I know.'
      mechoaround %actor% %self.name% seems pleased as she speaks with %actor.name%.
      wait 20
      msend %actor% %self.name% tells you 'I will guide you through the proper performance of a
      msend %actor% &0complex ceremony to create a well of moonlight.'
      wait 4s
      msend %actor% %self.name% tells you, 'This ceremony requires several powerful symbols and
      msend %actor% &0materials.  You must attain these materials since I cannot leave this place.'
      mechoaround %actor% %self.name% excitedly tells %actor.name% something.
      wait 5
      msend %actor% %self.name% tells you, 'First we need something to mark the well's outline.'
      mechoaround %actor% %self.name% begins telling something to %actor.name%.
      wait 5
      msend %actor% %self.name% tells you, 'Long ago during the Rift Wars, the goddess Mielikki was
      msend %actor% &0injured by another god, and one of the vines that surround her body was cut
      msend %actor% &0off.  As this was more than a mere plant but rather part of Her divine body, it
      msend %actor% &0survived.'
      wait 6s
      msend %actor% %self.name% tells you, 'I have heard tell this vine has proliferated near molten
      msend %actor% &0mountains, but is guarded by fearsome fiery beasts.'
      wait 4s
      msend %actor% %self.name% tells you, 'Go, recover part of this vine and bring it back safely!'
      mechoaround %actor% %self.name% pleads with %actor.name% desperately.
      quest start moonwell_spell_quest %actor.name% 
   endif
endif
~
#16343
vine_got~
1 g 100
~
if (%actor.quest_stage[moonwell_spell_quest]% == 1)
   if %got_vine% == 1
      oecho The Vine of Mielikki glows brightly then fades...its holy glow fading.
      return 0
   else
   quest advance moonwell_spell_quest %actor.name%
   endif
endif
~
#16344
chimera_load~
0 g 100
~
say My trigger commandlist is not complete!
~
#16345
tri-asp_load~
0 g 100
~
if (%actor.quest_stage[moonwell_spell_quest]% ==4)
   mjunk flask
   mload obj 16356
end if
~
#16346
got_flask~
1 g 100
~
if (%actor.quest_stage[moonwell_spell_quest]% == 4)
   if %got_flask% == 1
      oecho Eleweiss' Flask grows dark as its power fades.
      return 0
   else
   quest advance moonwell_spell_quest %actor.name%
   endif
elseif
set got_flask 1
global got_flask
endif
~
#16347
jade_got~
1 g 100
~
if %actor.quest_stage[moonwell_spell_quest]% == 8
   quest advance moonwell_spell_quest %actor.name%
   oecho %self.shortdesc% begins to glow mysteriously.
endif
~
#16348
create_moonwell~
2 a 0
~
wload obj 33
~
#16349
Moonwell Vine Receive~
0 j 100
16350~
if %actor.quest_stage[moonwell_spell_quest]% == 1
   wait 2
   mjunk vine
   msend %actor% %self.name% tells you, 'How did you get this?  No, go get it yourself.'
elseif %actor.quest_stage[moonwell_spell_quest]% == 2
   wait 2
   mjunk vine
   quest advance moonwell_spell_quest %actor.name%
   msend %actor% %self.name% tells you, 'Very good.  I can now lay out the well's boundaries.'
   wait 10
   mecho %self.name% places the vine in a small circle on the ground.
   smile %actor.name%
   wait 10
   msend %actor% %self.name% tells you, 'Excellent.  Now we must have stone.  Of course there is
   msend %actor% &0plenty of stone here, but we need to have a stone with significant energy to
   msend %actor% &0act as a magical anchor point.'
   wait 15
   msend %actor% %self.name% tells you, 'Far to the north there is an ancient burial site.  Within
   msend %actor% &0its catacombs lays a stone of great power, shaped like a heart.  Obtain this
   msend %actor% &0stone and it will surely increase our well's power.'
   wait 5
   msend %actor% %self.name% tells you, 'Go now while I continue construction of our well.'
   wait 20
   mecho %self.name% begins to collect some rocks and place them around the vine circle.
endif
~
#16350
moonwell_quest_status_checker~
0 d 100
status status? progress progress?~
wait 2
if %actor.has_completed[moonwell_spell_quest]%
  say I have already taught you to travel through moonwells.
  halt
endif
set stage %actor.quest_stage[moonwell_spell_quest]%
switch %stage%
  case 1
  case 2
    * Vine of Mielikki
    set item 16350
    set place an island of lava and fire.
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
    * update map
    say Please give me your map to update.
    halt
    break
  case 9
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
    break
   case 12
    * update diagram
    say Please give me your map to make the final markings.
    halt
    break
  default
    say You haven't agreed to help me yet.
    halt
done
say We need:
mecho &0%get.obj_shortdesc[%item%]% from
mecho &0%place%.
if %stage% > 6
  mecho  
  mecho %self.name% says, 'If you need a new map, say "&2&bI lost my map&0".'
endif
~
#16351
moonwell_dryad_map_replacement~
0 d 0
I lost my map~
wait 2
set stage %actor.quest_stage[moonwell_spell_quest]%
if %stage% > 6
  sigh
  say It will be difficult to complete this ritual without it.
  wait 1s
  emote begins to draw a new map.
  wait 2s
  emote continues to draw...
  wait 2s
  emote continues to draw...
  wait 4s
  say There!  All set!
  wait 1s
  switch %stage%
    case 8
      mload obj 16352
      break
    case 9
    case 10
      mload obj 16353
      break
    case 11
      mload obj 16354
      break
    case 12
      mload obj 16355
      break
    case 7
    default
      mload obj 16351
      break
  done
  give map %actor.name%
endif
~
#16352
Moonwell Heartstone receive~
0 j 100
48024~
if %actor.quest_stage[moonwell_spell_quest]% == 3
   wait 2
   quest advance moonwell_spell_quest %actor.name%
   msend %actor% %self.name% tells you, 'Very good! This will work perfectly as an anchor stone.'
   mecho %self.name% places %object.shortdesc% in the center of the well's outline.
   mjunk jewel
   wait 15
   msend %actor% %self.name% tells you, 'Back when I was still mortal, I frequently traveled with
   msend %actor% &0Eleweiss, a ranger of great skill and magical acumen.  We once ventured north
   msend %actor% &0into the frozen mountains, and came across a cult that worshipped a
   msend %actor% &0particularly nasty white dragon.  Due to some unfortunate circumstances, we
   msend %actor% &0ended up having to battle the dragon and were forced to retreat.'
   wait 20
   msend %actor% %self.name% tells you, 'In our retreat, Eleweiss lost a flask which contained
   msend %actor% &0water blessed by the Goddess Herself.  That water would be ideal for our magic
   msend %actor% &0well.'
   wait 2s
   msend %actor% %self.name% tells you, 'I imagine the flask is the kind of thing a dragon would
   msend %actor% &0add to its treasure hoard.  Please take revenge for us and obtain the flask.'
endif
~
#16353
Moonwell Flask Receive~
0 j 100
16356~
if %actor.quest_stage[moonwell_spell_quest]% == 4
   wait 2
   mjunk flask
   msend %actor% %self.name% tells you, 'How did you get this?  No no, go get it yourself.'
elseif %actor.quest_stage[moonwell_spell_quest]% == 5
   wait 2
   mjunk flask
   quest advance moonwell_spell_quest %actor.name%
   msend %actor% %self.name% tells you, 'Thank you, thank you.'
   wait 15
   mecho %self.name% opens the flask and pours the water into the boundary of the well while reciting a prayer to Mielikki.
   wait 4
   mecho The boundary of the well becomes translucent and starts to glow!
   wait 15
   msend %actor% %self.name% tells you, 'Okay now we need some items of power to place around the
   msend %actor% &0outside edge of the well.'
   wait 15
   msend %actor% %self.name% tells you, 'Each needs to be either an orb or circle to reflect the
   msend %actor% &0circular boundary of the well itself.'
   wait 15
   msend %actor% %self.name% tells you, 'The first item shall be a ring representing Fire.  Fire
   msend %actor% &0is most closely associated with rubies, so a ruby ring would be good.  One from
   msend %actor% &0a place connected with fire would be ideal!'
   wait 5
   msend %actor% %self.name% tells you, 'Oh, before you leave.  Here takes this.'
   mload obj 16351
   give map %actor.name%
   msend %actor% %self.name% tells you, 'That will help you keep track of everything.'
   wait 3s
   msend %actor% %self.name% tells you, 'Now go!'
endif
~
#16354
Moonwell Ruby Receive~
0 j 100
5201~
if %actor.quest_stage[moonwell_spell_quest]% == 6
   wait 2
   quest variable moonwell_spell_quest %actor.name% map 1
   msend %actor% %self.name% tells you, 'Good.  Thank you!'
   wait 15
   mecho %self.name% places %object.shortdesc% along the circumference of the circle and utters a prayer.
   wait 1s
   mecho %object.shortdesc% starts flickering with sparking lights!
   wait 2s
   msend %actor% %self.name% tells you, 'Now that we have the first symbol, we can go for the
   msend %actor% &0second.  The second item we need is an orb, representing the element of Air.
   msend %actor% &0This orb is somewhere in a dark fortress to the east.  Please obtain the orb.'
   wait 15
   msend %actor% %self.name% tells you, 'Oh wait, please allow me to update your map.'
   mjunk %object%
endif
~
#16355
Moonwell Map 1 Receive~
0 j 100
16351~
if %actor.quest_stage[moonwell_spell_quest]% == 6
   if %actor.quest_variable[moonwell_spell_quest:map]% == 1
      quest advance moonwell_spell_quest %actor.name%
      quest variable moonwell_spell_quest %actor.name% map 0
      wait 2
      mjunk map
      mload obj 16352
      wait 15
      mecho %self.name% quickly marks something on the bark map.
      give map %actor.name%
      wait 5
      msend %actor% %self.name% tells you, 'There you go!'
   else
      return 0
      shake
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      msend %actor% %self.name% tells you, 'I need the ring first.'
   endif
endif
~
#16356
Moonwell Winds Receive~
0 j 100
16006~
if %actor.quest_stage[moonwell_spell_quest]% == 7
   wait 2
   quest variable moonwell_spell_quest %actor.name% map 1
   wait 15
   mecho %self.name% places %object.shortdesc% along the circumference of the circle and utters a prayer.
   wait 1s
   mecho %object.shortdesc% begins to swirl like the howling winds!
   wait 2s
   msend %actor% %self.name% tells you, 'We're making good progress.'
   wait 15
   msend %actor% %self.name% tells you, 'Now that you have obtained the second element, please let
   msend %actor% &0me mark your map.'
   mjunk orb
endif
~
#16357
Moonwell Map 2 Receive~
0 j 100
16352~
if %actor.quest_stage[moonwell_spell_quest]% == 7
   if %actor.quest_variable[moonwell_spell_quest:map]% == 1
      quest advance moonwell_spell_quest %actor.name%
      quest variable moonwell_spell_quest %actor.name% map 0
      wait 15
      mjunk map
      mload obj 16353
      mecho %self.name% marks something on the bark map.
      wait 15
      give map %actor.name%
      wait 5
      msend %actor% %self.name% tells you, 'Okay, the third element in this ritual is Water.  In
      msend %actor% &0magic, water is often represented by jade.'
      wait 4s
      msend %actor% %self.name% tells you, 'I know of a jade ring held by a wood nymph on an island
      msend %actor% &0where other members of our Order are beset by beasts.'
      wait 4s
      msend %actor% &0But be careful, for 'tis a dangerous place for the unprepared!'
   else
      return 0
      shake
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      msend %actor% %self.name% tells you, 'I need the orb first.'
   endif
endif
~
#16358
Moonwell Jade Receive~
0 j 100
49011~
if %actor.quest_stage[moonwell_spell_quest]% == 9
   quest variable moonwell_spell_quest %actor.name% map 1
   wait 5
   mecho %self.name% places %object.shortdesc% along the circumference of the circle and utters a prayer.
   wait 1s
   mecho %object.shortdesc% dissipates into a circle of undulating soft blue-green light.
   wait 2s
   msend %actor% %self.name% tells you, 'Good.  Please give me your map so I may update it.'
   mjunk ring
endif
~
#16359
Moonwell Map 3 Receive~
0 j 100
16353~
if %actor.quest_stage[moonwell_spell_quest]% == 9
   if %actor.quest_variable[moonwell_spell_quest:map]% == 1
      quest advance moonwell_spell_quest %actor.name%
      quest variable moonwell_spell_quest %actor.name% map 0
      wait 2
      mjunk map
      wait 15
      mload obj 16354
      mecho %self.name% quickly marks another check on the bark map.
      wait 15
      give map %actor.name%
      msend %actor% %self.name% tells you, 'Okay, there are two items left.  This next might seem
      msend %actor% &0odd, but we need a single item of evil and chaos to mislead forces that wish to
      msend %actor% &0harm us while traveling through the well.'
      wait 5s
      msend %actor% %self.name% tells you, 'There is an orb that will serve this function well.  But
      msend %actor% &0as with so many such powerful relics, it is guarded by a great dragon hidden in
      msend %actor% &0a hellish labyrinth.'
      wait 5s
      msend %actor% %self.name% tells you, 'Get this orb that we might protect ourselves!'
   else
      return 0
      shake
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      msend %actor% %self.name% tells you, 'I need the ring first.' 
   endif     
endif
~
#16360
Moonwell Chaos Receive~
0 j 100
4003~
if %actor.quest_stage[moonwell_spell_quest]% == 10
   wait 2
   quest variable moonwell_spell_quest %actor.name% map 1
   wait 10
   mecho %self.name% places %object.shortdesc% along the circumference of the circle and utters a prayer.
   wait 1s
   mecho %object.shortdesc% crackles and shatters, shooting streaks of red lightning around the periphery of the well.
   wait 2s
   msend %actor% %self.name% tells you, 'Please give me the map to update!'
   mjunk orb
endif
~
#16361
Moonwell Map 4 Receive~
0 j 100
16354~
if %actor.quest_stage[moonwell_spell_quest]% == 10
   if %actor.quest_variable[moonwell_spell_quest:map]% == 1
      quest advance moonwell_spell_quest %actor.name%
      quest variable moonwell_spell_quest %actor.name% map 0
      wait 2
      mjunk map
      wait 15
      mload obj 16355
      mecho %self.name% marks something on the bark map.
      wait 15
      give map %actor.name%
      msend %actor% %self.name% tells you, 'Lastly, we need the item for the Earth.  There is a ring
      msend %actor% &0made of mundane stone kept in a large temple hidden in a mountain.  Obtain this
      msend %actor% &0final ring and we can finish this ceremony.'
   else
      return 0
      shake
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      msend %actor% %self.name% tells you, 'I need the orb first.'
   endif
endif
~
#16362
Moonwell Granite Receive~
0 j 100
55020~
if %actor.quest_stage[moonwell_spell_quest]% == 11
   wait 2
   mjunk ring
   quest variable moonwell_spell_quest %actor.name% map 1
   wait 15
   msend %actor% %self.name% tells you, 'Excellent, hand me your map so that I may finish this.'
endif
~
#16363
Moonwell Map 5 Receive~
0 j 100
16355~
if %actor.quest_stage[moonwell_spell_quest]% == 11
   if %actor.quest_variable[moonwell_spell_quest:map]% == 1
      wait 15
      mjunk map
      mload obj 16366
      mecho %self.name% completes her drawing on the bark map.
      wait 15
      give map %actor.name%
      if %actor.sex% == female
         msend %actor% %self.name% tells you, 'Keep that map Sister, that you may never forget how to
         msend %actor% &0conduct this ceremony.'
      else
         msend %actor% %self.name% tells you, 'Keep that map Brother, that you may never forget how to
         msend %actor% &0conduct this ceremony.'
      endif
      wait 20
      msend %actor% %self.name% tells you, 'Let us finish the ceremony.'
      mecho %self.name% places %get.obj_shortdesc[55020]% at the edge of the circle and prays.
      wait 5
      mecho &5A &0moonwell &5appears at %self.name%'s feet!&0
      wait 20
      mecho &6&bThe moonwell beings to &5&bswirl&0!
      wait 25
      * mecho &5A swirling &0moonwell &5appears at %self.name%'s feet!&0
      m_run_room_trig 16348
      wait 30
      mecho A soft yet loud voice says, 'You have done well young dryad.  Perhaps you truly
      mecho &0are repentant.  For your service, I release you from your shackles.'
      wait 10
      mecho The voice says, 'Remember this always.'
      wait 15
      mecho A bright flash of light pierces the area and the dryad is transformed into a beautiful mortal woman!
      wait 10
      mecho The beautiful druid says, 'Thank you my Goddess, Mielikki!  At last, I am free
      mecho &0again!'
      wait 2s
      mecho The beautiful druid bows deeply, enters the moonwell and disappears!
      quest complete moonwell_spell_quest %actor.name%
      mskillset %actor.name% moonwell
      mpurge %self%
   else
      return 0
      shake
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      msend %actor% %self.name% tells you, 'I need the ring first.'
   endif
endif
~
#16364
Moonwell receive decline~
0 j 100
~
set stage %actor.quest_stage[moonwell_spell_quest]%
if ((%stage% == 1 || %stage% == 2 || %stage% == 3) && %object.vnum% == 16350)
   halt
elseif ((%stage% == 3 || %stage% == 4) && %object.vnum% == 48024)
   halt
elseif ((%stage% == 4 || %stage% == 5 || %stage% == 6) && %object.vnum% == 16356)
   halt
elseif (%stage% == 6 && %object.vnum% == 5201)
   halt
elseif (%stage% == 7 && (%object.vnum% == 16006 || %object.vnum% == 16351))
   halt
elseif %stage% == 8 && %object.vnum% == 16352
   halt
elseif %stage% == 9 && %object.vnum% == 49011
   halt
elseif %stage% == 10 && (%object.vnum% == 4003 || %object.vnum% == 16353)
   halt
elseif (%stage% == 11 && (%object.vnum% == 55020 || %object.vnum% == 16354 || %object.vnum% == 16355))
   halt
else
   return 0
   wait 2
   msend %actor% %self.name% tells you, 'I do not want this from you.'
   msend %actor% %self.name% returns your item to you.
endif
~
$~
