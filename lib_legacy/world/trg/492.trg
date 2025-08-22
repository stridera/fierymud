#49200
Sound damage~
2 g 100
~
wait 1s
wdamage %actor% 10
wsend %actor% The violent rage of sounds thrust into your body, leaving you breathless.
wechoaround %actor% %actor.name% reels in pain as %actor.n% gasps for air.
~
#49201
Sound damage2~
2 g 100
~
wait 3s
wsend %actor% The violent rage of sounds thrust into your body, leaving you breathless.
wechoaround %actor% %actor.name% slants over in a desperate attempt to breathe.
wdamage %actor% 15
~
#49202
Exit_switch~
2 g 100
~
wait 1s
wechoaround %actor% The winds shift violently ripping up the sands and ground, leaving the way unknown.
wdoor 49219 down room 49225
wait 15s
wdoor 49219 down purge
~
#49223
get_golden_quill~
1 g 100
~
   *Inspired by ranger/druid quest
   if %actor.quest_stage[relocate_spell_quest]% == 8
      oecho The quill begins to glow brightly!
      quest advance relocate_spell_quest %actor.name%
   elseif %actor.quest_stage[relocate_spell_quest]% == 9
   oecho The quill begins to glow brightly!
   else
   wait 5
      osend %actor% The quill glows so brightly it burns your hand!
      oechoaround %actor% %actor.name% burns %actor.p% hand on the quill!
      oforce %actor% drop golden-quill
   end if
~
#49224
load_quill_again~
2 g 100
~
   *Make sure it doesn't load more than once
   if %actor.quest_stage[relocate_spell_quest]% == 8
      wpurge golden-quill
      wload obj 49251
   end if
~
#49225
Golden_quill_load~
2 a 0
~
wat 58522 wload obj 49251
~
#49226
quest_relocate_druid_greet~
0 g 100
~
   *Loads staff on entry into room
   *note that it purges the staff first to ensure only one loads
   if (%actor.quest_stage[relocate_spell_quest]% == 1)
      mjunk mystics
      wait 10
      say All this way and still I am found!
      wait 5
      scream
      say Fine, if you want this staff so badly, come and get it!
      mload obj 49250
      hold staff
   end if
~
#49227
quest_relocate_staff_get~
1 gi 100
~
if %actor.quest_stage[relocate_spell_quest]% == 1
  quest advance relocate_spell_quest %actor.name%
endif
if %victim.quest_stage[relocate_spell_quest]% == 1
  quest advance relocate_spell_quest %victim.name%
endif
~
#49250
quest_relocate_greet~
0 g 100
~
wait 1s
if !(%actor.has_completed[relocate_spell_quest]%) && %actor.level% < 100
   if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer)
      if (%actor.level% >= 65)
         switch %actor.quest_stage[relocate_spell_quest]%
            case 1
            case 2
               msend %actor% A lost mage asks you, 'Have you dealt with the thieving druid?  Please if you
               msend %actor% &0have, give me the staff!'
               mechoaround %actor% A lost mage grins as she asks %actor.name% a question.
               break
            case 3
            case 4
            case 5
               msend %actor% A lost mage asks you, 'Did you get the Crystal Telescope?  Please give me the
               msend %actor% &0Telescope!'
               mechoaround %actor% A lost mage grins as she asks %actor.name% a question.
               break
            case 6
               msend %actor% A lost mage asks you, 'Did you get the silver-trimmed spellbook?  Please give
               msend %actor% &0me the spellbook!'
               mechoaround %actor% A lost mage grins as she asks %actor.name% a question.
               break
            case 7
               msend %actor% A lost mage asks you, 'Do you have the map?  Please give me the map!'
               mechoaround %actor% A lost mage grins as she asks %actor.name% a question.
               break
            case 8
            case 9
               msend %actor% A lost mage asks you, 'Do you have the Quill?  Please give me the Quill!'
               mechoaround %actor% A lost mage grins as she asks %actor.name% a question.
               break
            default
               msend %actor% A scared-looking mage comes running over to you!
               mechoaround %actor% A scared-looking woman gets up and runs over to %actor.name%.
               wait 10
               msend %actor% A lost mage asks you, 'Please, can you help me?'
               mechoaround %actor% A lost mage pleads with %actor.name%.
               wait 10
               cry
               wait 10
               mechoaround %actor% A lost mage pleads with %actor.name%.
               msend %actor% A lost mage asks you, 'I'm completely lost in this dark desert, &6&bhelp&0
               msend %actor% &0me get out!'
         done
      endif
   endif
endif
~
#49251
quest_relocate_speak_help~
0 d 100
help~
if (%actor.level% >= 65)
   if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer)
      wait 10
      msend %actor% A lost mage tells you, 'I used to know a spell, yes, quite the spell!'
      mechoaround %actor% A lost mage tells something to %actor.name%.
      wait 10
      msend %actor% A lost mage tells you, 'But I do not have the materials I need...'
      mechoaround %actor% A lost mage tells something to %actor.name%.
      wait 5
      ponder
      msend %actor% A lost mage tells you, 'Please, help me find the &6&bitems&0.'
      mechoaround %actor% A lost mage pleads with %actor.name%.
   endif
endif
~
#49252
quest_relocate_speak_items~
0 d 100
items~
wait 2
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && (%actor.class% > 64)
   msend %actor% A lost mage asks you, 'Yes, please get me the items I need to get out of here!
   msend %actor% &0Do this for me and I'll share this great spell with you.'
   mechoaround %actor% A lost mage tells something to %actor.name%.
   wink %actor.name%
   wait 10
   mecho A lost mage begins taking inventory of the items she needs.
   wait 10
   msend %actor% A lost mage tells you, 'The first item I need...  A great mage once created a
   msend %actor% &0very powerful staff, but the druids stole it!'
   wait 10
   msend %actor% A lost mage tells you, 'The first item I'll need will be the Staff of the
   msend %actor% &0Mystics.  That druid, having angered the mage, fled as far as he could go, all
   msend %actor% &0the way past the Vale of Anlun.'
   wait 2s
   msend %actor% A lost mage tells you, 'If you need to remember your &7&b[progress]&0 you can'
   msend %actor% &0come back and check with me.'
   mechoaround %actor% A lost mage tells something to %actor.name%.
   wait 10
   msend %actor% A lost mage tells you, 'Please go, go get me the staff I require!'
   quest start relocate_spell_quest %actor.name%
endif
~
#49253
quest_relocate_druid_death~
0 f 100
~
mecho A slender druid screams, 'No! I had made my escape already!'
mecho A slender druid chokes on some blood and dies.
~
#49254
quest_relocate_receive~
0 j 100
~
*
* edit 3-7-2021 by Daedela: Acerites code was throwing errors because of the logic flow.
* It worked when the questor did exactly as told, but deviating risked system failure.
* The quest is identical, but the logic has been completely re-written to eliminate the errors.
* Comments about Acerites hatred of ampersands is preserved for posterity and the previous code is logged on the Wiki.
set stage %actor.quest_stage[relocate_spell_quest]%
if %stage% == 1
  if %object.vnum% == 49250
     wait 1s
     mjunk mystics
     mecho A lost mage examines the Druidstaff carefully.
     wait 1s
     glare %actor.name%
     mecho %self.name% says, 'No no, it is of no use to me unless you destroy the druid
     mecho &0YOURSELF!  Try again.'
     wait 1s
     frown
  endif
elseif %stage% == 2
  if %object.vnum% == 49250
     wait 2
     quest advance relocate_spell_quest %actor.name%
     msend %actor% A lost mage tells you, 'Excellent, thank you! Thank you!'
     wait 10
     hold mystics
     wait 10
     msend %actor% A lost mage tells you, 'Ok, the next thing required is the Crystal
     msend %actor% &0Telescope.  An observer of the cold village holds it, but you must not harm
     msend %actor% &0him.  Convince him to give it to you, then bring it back to me.'
     mechoaround %actor% A lost mage tells something to %actor.name%.
     wait 20
     msend %actor% A lost mage tells you, 'Go now, I must start with the staff!'
     mechoaround %actor% A lost mage tells something to %actor.name%.
  endif
elseif %stage% == 4
  if %object.vnum% == 49252
     wait 1s
     mjunk telescope
     mecho A lost mage examines the telescope carefully.
     wait 1s
     frown %actor.name%
     mecho %self.name% says, 'How did you get this? There's something missing here.
     mecho &0You must earn this properly. Go NOW!'
   endif
elseif %stage% == 5
  if %object.vnum% == 49252
     wait 2
     quest advance relocate_spell_quest %actor.name%
     msend %actor% A lost mage tells you, 'Yes, yes! Thank you!
     wait 10
     hold telescope
     wait 10
     msend %actor% A lost mage tells you, 'Ok ok, you've done well, the next item I need is the
     msend %actor% &0spellbook.'
     wait 20
     msend %actor% A lost mage tells you, 'But not just any spellbook.  This one is held deep
     msend %actor% &0inside a very powerful tower.  To find the tower you must look within the
     msend %actor% &0destroyed land.  That is all I can say.  Go now!'
     mechoaround %actor% A lost mage tells something to %actor.name%.
  endif
elseif %stage% == 6
   if (%object.vnum% == 12520)
     quest advance relocate_spell_quest %actor.name%
     msave %actor%
     wait 2
     msend %actor% A lost mage tells you, 'Perfect! Only two items left!'
     wait 10
     rem mystics
     mjunk mystics
     hold spellbook
     wait 10
     msend %actor% A lost mage tells you, 'There is something I need from South Caelia.  I don't
     msend %actor% &0know the area well, but I know someone who does.  I once met a man that mapped
     msend %actor% &0everything he saw.  Please get me his map.'
     mechoaround %actor% A lost mage tells something to %actor.name%.
     wait 10
     ponder
     wait 10
     msend %actor% A lost mage tells you, 'I think he said something about the ocean and the
     msend %actor% &0beach, that's all I know.'
     mechoaround %actor% A lost mage tells something to %actor.name%.
   endif
elseif %stage% == 7
   if (%object.vnum% == 58608)
     quest advance relocate_spell_quest %actor.name%
     wait 2
     msend %actor% A lost mage tells you, 'Almost there, one more!'
     wait 10
     rem telescope
     mjunk telescope
     hold map
     wait 10
     msend %actor% A lost mage tells you, 'The only thing left that I need is something to write
     msend %actor% &0with.  A while back I dropped something while engaged in a little confrontation
     msend %actor% &0with that witch Baba.  Luckily, I don't think she saw me drop it, so it should
     msend %actor% &0still be hidden around there.'
     mechoaround %actor% A lost mage tells something to %actor.name%.
     wait 4s
     sigh
     wait 1s
     msend %actor% A lost mage tells you, 'Please get my quill back, then I shall share the spell
     msend %actor% &0with you!'
     mechoaround %actor% A lost mage tells something to %actor.name%.
     m_run_room_trig 49225
   endif
elseif %stage% == 8
  if %object.vnum% == 49251
     wait 1s
     mecho A lost mage examines the golden quill.
     wait 1s
     sigh
     mecho %self.name% says, 'You did not get this yourself.  Unfortunately this is a
     mecho &0problem, as now that another has touched this it has lost its power.'
     frown
  endif
elseif %stage% == 9 
   if (%object.vnum% == 49251)
     wait 20
     msend %actor% A lost mage tells you, 'Excellent, Thank you so much, please wait while I
     msend %actor% &0scribe my spell!'
     mechoaround %actor% A lost mage tells something to %actor.name%.
     wait 1s
     sit
     rem map
     mjunk map
     hold quill
     mecho A lost mage puts on her spectacles and begins to scribe.
     wait 20
     mecho A lost mage puts away her spectacles and stops scribing.
     sta
     fly
     wait 1s
     msend %actor% A lost mage tells you, 'Very well, I'm done!  Thank you so much,
     msend %actor% &0%actor.name%!'
     wait 20
     msend %actor% A lost mage tells you, 'And, as I promised, here is the spell!'
     msend %actor% A lost mage shows you the spell scribed in her spellbook.
     mskillset %actor.name% relocate
     quest complete relocate_spell_quest %actor.name%
     mechoaround %actor% A lost mage shows %actor.name% her spellbook, teaching %actor.p% her secrets.
     wait 20
     mecho A lost mage starts casting &3&b'relocate'&0...
     wait 15
     mecho &7&bA lost mage's molecules loosen and eventually dissipate into thin air.&0
     mgoto 49299
     mpurge self
  endif
else
   return 0
   wait 2
   msend %actor% A lost mage tells you, 'This item doesn't help me.'
   msend %actor% A lost mage returns your item.
   mechoaround %actor% A lost mage returns %actor.name%'s item.
endif
~
#49255
relocate_telescope_subquest~
0 g 100
~
wait 2
if %actor.quest_stage[relocate_spell_quest]% == 3
   quest variable relocate_spell_quest %actor% greet 1
   msend %actor% The observer tells you, 'You were sent here for my crystal instrument.  I'll be
   msend %actor% &0more then happy to give it to you, but...'
   mechoaround %actor% The observer tells something to %actor.name%.
   wait 5
   msend %actor% The observer tells you, 'I cannot just give it away for free.'
   mechoaround %actor% The observer tells something to %actor.name%.
   wait 10
   msend %actor% The observer tells you, 'I have heard of a powerful artifact that I would
   msend %actor% &0greatly value...  For research purposes, of course.'
   mechoaround %actor% The observer tells something to %actor.name%.
   wink %actor.name%
   wait 2s
   msend %actor% The observer tells you, 'I hear tell of a glass globe in the Valley of the
   msend %actor% &0Frost Elves that has the power to alter time!  Bring it to me and I will let 
   msend %actor% &0you have my telescope.'
elseif (%actor.quest_stage[relocate_spell_quest]% == 4)
   msend %actor% The observer tells you, 'Do you have the globe?!  Please give it to me!'
   mechoaround %actor% The observer tells something to %actor.name%.
   bounce
endif
~
#49256
telescope_subquest_receive~
0 j 100
~
* Check for proper actor stage and item
if (%object.vnum% == 53424)
   if (%actor.quest_stage[relocate_spell_quest]% == 4)   
      wait 10
      msend %actor% The observer tells you, 'Thank you so much, you have recovered the lost globe!'
      thank %actor.name%
      rem telescope
      msend %actor% The observer tells you, 'Here, you may now have my Crystal Telescope.'
      give telescope %actor.name%
      quest advance relocate_spell_quest %actor.name%
      bow %actor.name%
      mecho The observer leaves down.
      mpurge globe
      mpurge observer
      halt
   else
      wait 10
      eyebrow %actor.name%
      msend %actor% The observer tells you, 'Yes, this is what I seek, but how did you come by it?
      msend %actor% &0Only the one that did retrieve the glass for me can be rewarded!'
      mechoaround %actor% The observer questions %actor.name% harshly.
      junk snow-globe
   endif
else
   return 0
   wait 2
   msend %actor% The observer tells you, 'This isn't my globe!'
   msend %actor% The observer returns your item to you.
   mechoaround %actor% The observer refuses %obj.shortdesc% from %actor.name%.
endif
~
#49257
relocate_get_sphere~
1 gi 100
~
if %actor.quest_stage[relocate_spell_quest]% == 3
  quest advance relocate_spell_quest %actor.name%
  set echo 1
endif
if %victim.quest_stage[relocate_spell_quest]% == 3
  quest advance relocate_spell_quest %victim.name%
  set echo 1
endif
if %echo%
  wait 2
  oecho The globe glares brightly with power!
endif
~
#49258
relocate_spell_quest_status_checker~
0 d 100
status status? progress progress?~
set stage %actor.quest_stage[relocate_spell_quest]%
wait 1s
if %actor.has_completed[relocate_spell_quest]%
  say I've already taught you my greatest spell.
  halt
endif
switch %stage%
  case 1
  case 2
    set next the Staff of the Mystics
    set place a druid past the Vale of Anlun
    break
  case 3
    set next the Crystal Telescope
    set place an observer of the cold village
    break
  case 4
    set next a glass globe
    set place the Valley of the Frost Elves
    break
  case 5
    set next the Crystal Telescope by returning the glass globe
    set place an observer of the cold village
    break
  case 6
    set next a silver-trimmed spellbook
    set place a tower within a destroyed land
    break
  case 7
    set next a map
    set place from a mapper in South Caelia
    break
  case 8
    set next the Golden Quill
    set place the forest near Baba Yaga's hut
    break
  default
    say Please help me! 
done
say You are trying to retrieve:
mecho &2&b%next%&0
mecho &0from &7&b%place%.&0
~
#49299
nexus_cloak_pin_reload~
1 c 3
rock well demon~
oload obj 49299
oecho The &9&bNexus &1Cloak&0 &9&bPin&0 begins to &4&bgl&6&bow&0 mysteriously then fades back to normal.
oforce %actor% get pin
opurge %self%
~
$~
