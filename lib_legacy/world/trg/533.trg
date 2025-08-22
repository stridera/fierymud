#53300
hatching_dragon~
1 g 100
~
return 0
osend %actor% As you touch the egg it vibrates and cracks open!
oechoaround %actor% As %actor.name% touches the egg it hatches!
oload mob 53301
oforce dragon consider %actor.name%
oforce dragon hit %actor.name%
opurge %self.name%
~
#53301
sorcha_greet~
0 g 100
~
if %actor.vnum% == -1
   gasp
   say You dare to approach the altar of the mighty Tri-Aszp?!
   emote prays silently for a few seconds to her overlord.
endif
~
#53302
triaszp_helpers~
0 k 30
~
wait 1s
set value %random.10%
switch %value%
   case 1
      breath frost
      break
   case 2
   case 3
      sweep
      break
   case 4
   case 5
      roar
      break
   case 8
   case 9
      if %get.mob_count[53301]% < 6
          set victim %random.char%
          emote hisses in anger, calling to her children.
          wait 1s
          mload mob 53301
          mforce baby-dragon emote scampers in and attacks!
          if %victim.vnum% == -1
              mforce baby-dragon kill %victim.name%
          else
              mforce baby-dragon kill %actor.name%
          endif
      endif
      break
   default
      growl
done
~
#53303
reload_helpers~
0 l 50
~
wait 1s
set value %random.10%
if %value% < 2
   emote roars in pain and tries to locate more children.
   wait 1s
   if %get.mob_count[53301]% < 6
        mload mob 53301
        mforce baby-dragon emote scampers in and attacks!
        mforce baby-dragon kill %actor.name%
endif
endif
~
#53304
reload_helpers_2~
0 l 10
~
emote looks worried and tries to find some more children.
set loaded 0
global loaded
~
#53305
sorcha_fight~
0 k 33
~
wait 1s
cast 'unholy word'
~
#53306
frost_valley_entrance~
2 c 75
push~
wdoor 53302 west room 53401
wdoor 53302 west description The piles of ice slips allowing passage.
wdoor 53302 west name ice
wechoaround %actor% %actor.name% pushes some ice out of the way, opening a passage down the western tunnel.
wsend %actor% It took some effort, but the wall of ice has been moved from the west, allowing passage.
wat 53401 wecho The ice falls from the east allowing passage.
wait 1 t
wecho The ice seems to magically reform blocking the western passage.
wdoor 53302 west purge
~
#53307
wolf pack~
0 o 100
~
set room %self.room%
mgoto 1100
mload mob 53315
mforce wolf follow 2.wolf
mteleport wolf %room%
mload mob 53315
mforce wolf follow 2.wolf
mteleport wolf %room%
mload mob 53315
mforce wolf follow 2.wolf
mteleport wolf %room%
mgoto %room%
~
#53308
wall_ice_sculptor_greet~
0 g 100
~
wait 1s
if %actor.quest_stage[%type%_wand]% == %wandstep%
  eval minlevel (%wandstep% - 1) * 10
  if %actor.level% >= %minlevel%
    if %actor.quest_variable[%type%_wand:greet]% == 0
      mecho %self.name% says, 'I see you're crafting something.  If you want my help, we can talk about &6&b[upgrades]&0.'
      quest variable %type%_wand %actor% greet 1
    else
      say Do you have what I need for the wand?
    endif
    wait 1s
  endif
endif
set stage %actor.quest_stage[wall_ice]%
if %stage% == 0
  if %actor.class% /= Cryomancer && %actor.level% > 56
    wave
    say Hey, you there, you look like a capable cryomancer!  Lend me a hand with this wall, will ya?
    wait 2s
    say We had a few nasty surprises with creatures slipping out from Frost Valley.  So Suralla sent me over to reinforce the wall blocking off the tunnel.
    wait 2s
    say I ran out of supplies and my spells can't make permanent walls.  Can you help me get more ice for the wall?
  endif
elseif %stage% == 1
  eval ice (20 - %actor.quest_variable[wall_ice:blocks]%)
  if %ice% > 1
    say Do you have the %ice% remaining blocks of ice?
  elseif %ice% == 1
    say Do you have the last remaining block of ice?
  endif
endif
~
#53309
ice_wall_sculptor_speech~
0 d 100
Yes okay wall? How? Ice? supplies? ice supplies how~
wait 2
if %actor.quest_stage[wall_ice]% == 0
  if %actor.class% /= Cryomancer && %actor.level% > 56
    quest start wall_ice %actor.name%
    if %actor.sex% == female
      say Thank you so much ma'am!
    elseif %actor.sex% == male
      say Thank you so much sir!
    else
      say Thank you so much!
    endif
    wait 1s
    say What I need are blocks of living ice.  Using Wall of Ice I can fuse them together so they make a permanent barrier.  I chewed through my stock pile of living ice faster than I anticipated, so I need more.
    wait 2s
    emote writes some notes down on a piece of paper.
    mload obj 53326
    give notes %actor.name%
    wait 2s
    mecho %self.name% says, 'This spell has a chance to capture animated energies of ice creatures in their remains.  Have the spell on you and say &6&b'crystalize'&0 in the presence of an ice creature before you defeat it.'
    wait 4s
    say The more powerful the creature, the more likely this spell will create a block of living ice from it.  But be careful, it only works on creatures actually made of ice!!
    wait 4s
    say Though there are a number of ice creatures in Frost Valley, any ice creature is fine.  I need 20 more blocks to finish this wall.
    wait 3s
    mecho %self.name% says, 'You can check your &7&b[spell progress]&0 at any time.'
  elseif %actor.quest_stage[wall_ice]% == 1
    say Hand 'em on over then!
  endif
endif
~
#53310
wall_ice_crystalize_speech~
0 d 1
crystalize crystalize!~
set drop %actor.quest_variable[wall_ice:drop]%
if %actor.quest_stage[wall_ice]% == 1 && %actor.inventory[53326]% && %self.vnum% != 53316
  if %ice%
    msend %actor% &6You have already cast this spell on %self.name%!&0
  else
    set ice %random.100%
    if %drop% < 21
      if %ice% <= %self.level%
        mecho &6A dark glow surrounds &6&b%self.name%&0&6 briefly!&0
        set ice 1
        global ice
        eval count (%drop% + 1)
        quest variable wall_ice %actor.name% drop %count%
      else
        mecho &6The spell seems to have no effect!&0
        set ice 2
        global ice
      endif
    endif
  endif
elseif %self.vnum% == 53316 && %actor.quest_stage[wall_ice]% == 2 && %actor.inventory[53326]%
  mecho &6&bThe blocks of living ice slowly fuse together!&0
  wait 1s
  say Well done!
  nod %actor.name%
  wait 1s
  say All in a good day's work!
  wait 1s
  say Keep the notes.  It should be everything you need to cast Wall of Ice yourself.
  msend %actor.name% &6&bYou have learned Wall of Ice!&0
  quest complete wall_ice %actor.name%
  mskillset %actor.name% wall of ice
endif
~
#53311
wall_ice_mobs_death~
0 f 100
~
if %ice% == 1
  mload obj 53327
endif
~
#53312
wall_ice_sculptor_receive~
0 j 100
53327~
if %actor.quest_stage[wall_ice]% == 1
  wait 2
  eval ice %actor.quest_variable[wall_ice:blocks]% + 1
  quest variable wall_ice %actor.name% blocks %ice%
  say Great, this is exactly what I need!
  mecho %self.name% adds %object.shortdesc% to the wall blocking the tunnel.
  mjunk all.block-living-ice
  if %actor.quest_variable[wall_ice:blocks]% == 20
    wait 1s
    quest advance wall_ice %actor.name%
    mecho %self.name% says, 'Excellent, this looks like enough ice!  Now all we have to do is cast the spell.  If you'd like to do the honors, just command the ice to &6&bcrystalize&0.'
  else
    wait 2
    say Got any more?
  endif
else
  wait 2
  mjunk all.block-living-ice
  say Hey thanks!  I could always use more of those.
endif
~
#53313
spell progress~
0 d 0
status status? progress progress?~
if %actor.quest_stage[wall_ice]% == 1
  set have %actor.quest_variable[wall_ice:blocks]%
  eval need (20 - %have%)
  wait 2
  say Let me see...
  mecho %self.name% counts the number of blocks.
  wait 2s
  say You have brought me &6&b%have% blocks of living ice.&0
  mecho   
  say I still need &6&b%need%&0 more.
  mecho  
  mecho %self.name% says, 'If you need a new copy of the spell of living ice, say "&6&bplease replace the spell&0".'
elseif %actor.has_completed[wall_ice]%
  say We've already completed the repairs here.  Good work!
elseif !%actor.quest_stage[wall_ice]%
  say Did you want to help me work?
endif
~
#53314
Sculptor refuse~
0 j 0
53327~
switch %object.vnum%
  case %wandgem%
  case %wandtask3%
  case %wandtask4%
  case %wandvnum%
    halt
    break
  default
    if %actor.quest_stage[wall_ice]% && %actor.quest_stage[%type%_wand]% == %wandstep%
      set response This won't help us build this wall and I can't craft with it.
    elseif %actor.quest_stage[%type%_wand]% == %wandstep%
      set response I can't craft with this.
    elseif %actor.quest_stage[wall_ice]%
      set response This won't help us build this wall.
    else
      set response What is this for?
    endif
done
if %response%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  say %response%
endif
~
#53315
wall_of_ice_spell_replacement~
0 d 0
please replace the spell~
wait 2
if %actor.quest_stage[wall_ice]%
  say Oh sure.  But be careful.  Don't lose this spell again.
  mload obj 53326
  give spell-living-ice %actor%
endif
~
$~
