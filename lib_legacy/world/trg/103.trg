#10300
thief_steal_stuff~
0 g 100
~
*Thief steal stuff trigger...
*Chose a set of common keywords and aligned percentage chances
*so that it has a random chance to steal any of the keywords
set rnd %random.100%
 if %rnd% > 99
 set loot gem
 elseif %rnd% == 99
 set loot iron 
 elseif %rnd% <= 98 && %rnd% >= 97
 set loot cloth
 elseif %rnd% <= 96 && %rnd% >= 95
 set loot wood
 elseif %rnd% <= 94 && %rnd% >= 93
 set loot white
 elseif %rnd% <= 92 && %rnd% >= 90
 set loot red
 elseif %rnd% <= 89 && %rnd% >= 85
 set loot blue
 elseif %rnd% <= 84 && %rnd% >= 80
 set loot green
 elseif %rnd% <= 79 && %rnd% >= 76
 set loot black
 elseif %rnd% <= 75 && %rnd% >= 71
 set loot dagger
 elseif %rnd% <= 70 && %rnd% >= 66
 set loot shield
 elseif %rnd% <= 65 && %rnd% >= 61
 set loot sword
 elseif %rnd% <= 60 && %rnd% >= 56
 set loot leather
 elseif %rnd% <= 55 && %rnd% >= 46
 set loot raft
 elseif %rnd% <= 45 && %rnd% >= 36
 set loot canoe
 elseif %rnd% <= 35 && %rnd% >= 26
 set loot sack
 elseif %rnd% <= 25 && %rnd% >= 16
 set loot bag
 elseif %rnd% <= 15
 set loot gem
 endif
*now the real command!
steal %loot% %actor.name%
~
#10301
fish-death~
0 f 100
~
mload obj 10311
~
#10302
rianne-greet~
0 g 100
~
wait 1s
if %actor.vnum% == -1
  if %actor.quest_stage[resort_cooking]% >= 1
    switch %actor.quest_stage[resort_cooking]%
      case 1
        set recipe Peach Cobbler
        break
      case 2
        set recipe Seafood Salad
        break
      case 3
        set recipe Fish Stew
        break
      case 4
        set recipe Honey-Glazed Ham
        break
      case 5
        set recipe Saffroned Jasmine Rice
        break
      default
        halt
    done
    mecho %self.name% says, 'Welcome back, %actor.name%!
    mecho &0Have you brought me the ingredients for &7&b%recipe%?&0'
    wait 1s
    mecho %self.name% says, 'Remember, you can look in the icebox to see what you've
    mecho &0already brought me.'
    wait 2s
    mecho %self.name% says, 'Oh, and the recipe for &7&b%recipe%&0 is on the wall!'
  else
    mecho %self.name% says, 'Welcome to Phoenix Feather Resort, %actor.name%!
    mecho &0My name is %self.name% and I am the cook here.'
    wait 2s
    mecho %self.name% says, 'But I could use a little help!  Being located up north
    mecho &0makes it hard to get everything I need for some of my more exotic dishes.'
    wait 2s
    say %actor.name% do you think you could help me?
    smile %actor.name%
  endif
endif
~
#10303
rianne-quest-start~
0 dn 1
yes yes?~
if %actor.vnum% == -1 && %actor.quest_stage[resort_cooking]% < 1
  quest start resort_cooking %actor.name%
  wait 5
  mecho %self.name% says, 'Excellent!  The dish I am making next is &7&bPeach Cobbler&0.'
  wait 2s
  say I will need you to find the following ingredients for me:
  mecho - &7&b%get.obj_shortdesc[61501]%&0
  mecho - &7&b%get.obj_shortdesc[23754]%&0
  mecho - &7&b%get.obj_shortdesc[3114]%&0
  mecho - &7&b%get.obj_shortdesc[35001]%&0
  wait 2s
  say Bring them to me quickly so that I may begin!
  wink %actor.name%
  wait 2s
  mecho %self.name% says, 'Oh!  And you can look at my recipe wall at any time to
  mecho &0see what else we need.'
endif
~
#10304
rianne-quest-receive~
0 j 100
~
*
* Added by Daedela 3-9-2021 for Group Heal quest
*
if %actor.quest_stage[group_heal]% == 5
  if %actor.quest_variable[group_heal:%self.vnum%]%
    if %object.vnum% == 18514
      say I've told you everything I can.  Good luck!
      halt
    endif
  else
    if %object.vnum% == 18514
      quest variable group_heal %actor.name% %self.vnum% 1
      return 0
      say Well you've definitely come to the right place!
      wait 4
      emote reads over the ritual recipe.
      wait 4s
      mecho %self.name% says, 'Oh my, what a challenge.  No wonder someone would need help with this.'
      wait 3s
      mecho %self.name% says, 'The names for many of these ingredients have changed since...  How old is this recipe anyway??'
      wait 1s
      beam
      say I love it.
      wait 3s
      emote begins writing down a list of notes.
      wait 1s
      say ... and this is called this...
      wait 1s
      say ... this can be substituted with this...
      wait 2s
      mecho %self.name% says, '... that's been extinct for over a century, so perhaps this will do...?'
      wait 4s
      emote finishes her list.
      mload obj 18520
      give notes %actor.name%
      wait 2s
      say That should help you.  I can't wait to hear how it goes!
      wave %actor%
      halt
    endif
  endif
endif
*
* Original resumes here
* 
* Preset item and recipe vars
set item4 0
set item5 0
set item6 0
set item7 0
set stage %actor.quest_stage[resort_cooking]%
set recipe1 Peach Cobbler
set recipe2 Seafood Salad
set recipe3 Fish Stew
set recipe4 Honey-Glazed Ham
set recipe5 Saffroned Jasmine Rice
switch %stage%
  case 1
    set recipe %recipe1%
    set next %recipe2%
    set item1 61501
    set item2 23754
    set item3 3114
    set item4 35001
    break
  case 2
    set recipe %recipe2%
    set next %recipe3%
    set item1 49024
    set item2 23750
    set item3 23722
    set item4 8003
    set item5 12515
    set item6 1606
    break
  case 3
    set recipe %recipe3%
    set next %recipe4%
    set item1 55213
    set item2 30002
    set item3 10030
    set item4 12552
    set item5 23757
    set item6 18509
    set item7 10311
    break
  case 4
    set recipe %recipe4%
    set next %recipe5%
    set item1 41011
    set item2 8350
    set item3 2001
    set item4 50207
    set item5 6106
    break
  case 5
    set recipe %recipe5%
    set item1 58019
    set item2 37013
    set item3 23760
    break
  default
    return 0
    say I don't have time for that right now.
    emote refuses your item.
    halt
done
* What item is being turned in?
switch %object.vnum%
  case %item1%
    set item 1
    break
  case %item2%
    set item 2
    break
  case %item3%
    set item 3
    break
  case %item4%
    set item 4
    break
  case %item5%
    set item 5
    break
  case %item6%
    set item 6
    break
  case %item7%
    set item 7
    break
  default
    return 0
    mecho %self.name% says, 'The recipe doesn't call for this!  Perhaps you should consult the recipe on the wall to refresh your memory.'
    halt
done
if %actor.quest_variable[resort_cooking:item%item%]%
  return 0
  mecho %self.name% says, 'You already brought in %object.shortdesc%, so we don't need more.'
  emote hands your item back to you.
  halt
endif
wait 1
mjunk %object.name%
wait 4
mecho %self.name% says, 'Just what we need for &7&b%recipe%&0!'
quest variable resort_cooking %actor.name% item%item% 1
* See if we've turned in everything for this recipe
set item 1
while %item% <= 7
  set item%item% 0
  eval item %item% + 1
done
if %actor.quest_variable[resort_cooking:item1]%
  set item1 1
endif
if %actor.quest_variable[resort_cooking:item2]%
  set item2 1
endif
if %actor.quest_variable[resort_cooking:item3]%
  set item3 1
endif
if %stage% == 5 || %actor.quest_variable[resort_cooking:item4]%
  set item4 1
endif
if %stage% == 1 || %stage% == 5 || %actor.quest_variable[resort_cooking:item5]%
  set item5 1
endif
if (%stage% != 2 && %stage != 3) || %actor.quest_variable[resort_cooking:item6]%
  set item6 1
endif
if %stage% != 3 || %actor.quest_variable[resort_cooking:item7]%
  set item7 1
endif
* If all the items have been turned in, start mixing
if %item1% && %item2% && %item3% && %item4% && %item5% && %item6% && %item7%
  quest advance resort_cooking %actor.name%
  set item 1
  * Reset item variables
  while %item% <= 7
    quest variable resort_cooking %actor.name% item%item% 0
    eval item %item% + 1
  done
  say I think I can start preparing it now.
  wait 1s
  emote gets the other ingredients from the icebox.
  emote begins mixing them together.
  wait 2s
  if %stage% == 5
    mecho %self.name% says, 'You've been such a great help, %actor.name%!  Now I'm almost prepared for this huge dinner party, thanks!'
    wait 3s
    say I'd like you to have this.
    wait 5
    mecho %self.name% says, 'It was enchanted by a good friend of mine, so I hope it will help you as much as you've helped me.'
    mload obj 10315
    set gem 0
    while %gem% < 3
       eval drop %random.11% + 55736
       mload obj %drop%
       eval gem %gem% + 1
    done
    give all.gem %actor%
    *
    * Set X to the level of the award - code does not run without it
    * 
    if %actor.level% < 65
      set expcap %actor.level%
    else
      set expcap 65
    endif
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
    *
    * Adjust exp award by class so all classes receive the same proportionate amount
    *
    switch %actor.class%
      case Warrior
      case Berserker
      *
      * 110% of standard
      * 
        eval expmod (%expmod% + (%expmod% / 10))
        break
      case Paladin
      case Anti-Paladin
      case Ranger
      *
      * 115% of standard
      *
        eval expmod (%expmod% + ((%expmod% * 2) / 15)
        break
      case Sorcerer
      case Pyromancer
      case Cryomancer
      case Illusionist
      case Bard
      *
      * 120% of standard
      *
        eval expmod (%expmod% + (%expmod% / 5))
        break
      case Necromancer
      case Monk
      *
      * 130% of standard
      *
        eval expmod (%expmod% + (%expmod% * 2) / 5)
        break
      default
        set expmod %expmod%
    done
    msend %actor% &3&bYou gain experience!&0
    eval setexp (%expmod% * 10)
    set loop 0
    while %loop% < 10
    *
    * Xexp must be replaced by mexp, oexp, or wexp for this code to work
    * Pick depending on what is running the trigger
    *
      mexp %actor% %setexp%
      eval loop %loop% + 1
    done
    quest complete resort_cooking %actor.name%
  else
    mecho %self.name% says, 'I'll keep preparing this while you collect items for the next recipe.'
    ponder
    wait 1s
    say The next dish is %next%.
    wait 2   
    say Here's the recipe.  Please take a look at it.
    emote points out a slip of paper hanging on the wall.
  endif
else
  emote puts the item into the icebox.
  wait 1s
  say Thanks!  Quickly, bring the rest!
endif
drop all
~
#10305
rianne-quest-look~
2 c 100
look~
if %actor.quest_stage[resort_cooking]% < 1 || %actor.quest_stage[resort_cooking]% > 5
  return 0
  halt
endif
switch %actor.quest_stage[resort_cooking]%
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
    break
  default
    return 0
    halt
done
if %arg% /= recipe || %arg% /= wall || %arg% /= paper || %arg% /= slip
  wsend %actor% The wall is covered in slips of paper, each with a different recipe.  One
  wsend %actor% &0especially stands out among the mess.
  wsend %actor% &0
  wsend %actor% &0==========&7&b%recipe%&0==========
  if %item1%
    wsend %actor% &0  %get.obj_shortdesc[%item1%]%
  endif
  if %item2%
    wsend %actor% &0  %get.obj_shortdesc[%item2%]%
  endif
  if %item3%
    wsend %actor% &0  %get.obj_shortdesc[%item3%]%
  endif
  if %item4%
    wsend %actor% &0  %get.obj_shortdesc[%item4%]%
  endif
  if %item5%
    wsend %actor% &0  %get.obj_shortdesc[%item5%]%
  endif
  if %item6%
    wsend %actor% &0  %get.obj_shortdesc[%item6%]%
  endif
  if %item7%
    wsend %actor% &0  %get.obj_shortdesc[%item7%]%
  endif
elseif %arg% /= icebox
  wsend %actor% Looking inside the icebox, you see:
  set nothing 1
  if %actor.quest_variable[resort_cooking:item1]%
    wsend %actor% &0  %get.obj_shortdesc[%item1%]%
    set nothing 0
  endif
  if %actor.quest_variable[resort_cooking:item2]%
    wsend %actor% &0  %get.obj_shortdesc[%item2%]%
    set nothing 0
  endif
  if %actor.quest_variable[resort_cooking:item3]%
    wsend %actor% &0  %get.obj_shortdesc[%item3%]%
    set nothing 0
  endif
  if %actor.quest_variable[resort_cooking:item4]%
    wsend %actor% &0  %get.obj_shortdesc[%item4%]%
    set nothing 0
  endif
  if %actor.quest_variable[resort_cooking:item5]%
    wsend %actor% &0  %get.obj_shortdesc[%item5%]%
    set nothing 0
  endif
  if %actor.quest_variable[resort_cooking:item6]%
    wsend %actor% &0  %get.obj_shortdesc[%item6%]%
    set nothing 0
  endif
  if %actor.quest_variable[resort_cooking:item7]%
    wsend %actor% &0  %get.obj_shortdesc[%item7%]%
    set nothing 0
  endif
  if %nothing%
    wsend %actor% &0  Nothing.
  endif
else
  return 0
endif
~
#10306
men-only~
2 g 100
~
if %actor.level% < 100
 if %actor.sex% == female
 wsend %actor% That room is for men only!
 return 0
 endif
endif
~
#10307
women-only~
2 g 100
~
if %actor.level% < 100
 if %actor.sex% == male
 wsend %actor% That room is for women only!
 return 0
 endif
endif
~
#10308
hot-spring~
2 bdg 100
.~
if !%actor%
   set actor %random.char%
endif
if !%actor%
   halt
endif
set fireproof %actor.aff_flagged[!HEAT]%
if %actor.vnum% == -1
   if %actor.class% == Pyromancer
   elseif %fireproof%
   else
      eval damage %random.10% + %random.10%
      wechoaround %actor% %actor.name% starts to look a bit woozy from the extreme heat.
      wsend %actor% The water is too hot for you....
      wdamage %actor% %damage%
   end
end
~
#10309
khysan-greet~
0 g 100
~
if %actor.quest_stage[ice_shards]%
  set return yes
endif
if %actor.quest_stage[%type%_wand]% > %wandstep%
  set return yes
endif
if %actor.quest_stage[%type%_wand]% == %wandstep% && %actor.quest_variable[%type%_wand:greet]% == 1
  set return yes
endif
wait 1s
msend %actor% %self.name% looks up at your approach.
if %return% == yes
  say Welcome back my friend.
  wait 2
  if %actor.quest_stage[ice_shards]% && !%actor.has_completed[ice_shards]%
    say Did you find any more clues?
    if %actor.quest_stage[%type%_wand]% == %wandstep%
      eval minlevel (%wandstep% - 1) * 10
      wait 1s
      if %actor.level% >= %minlevel%
        if %actor.quest_variable[%type%_wand:greet]% == 0
          say Or is there something else that brings you back?
        else
          say Or do you have what I need for a new staff?
        endif
      endif
    endif
  else
    if %actor.quest_stage[%type%_wand]% == %wandstep%
      eval minlevel (%wandstep% - 1) * 10
      if %actor.level% >= %minlevel%
        if %actor.quest_variable[%type%_wand:greet]% == 0
          say Is there something else that brings you back?
        else
          say Do you have what I need for a new staff?
        endif
      endif
    endif
  endif
else
  mecho %self.name% smiles warmly and says, 'Welcome to Phoenix Feather Resort.'
  wait 1s
  say We are currently offering free services to all adventurers.
  wait 1s
  bow %actor%
  say Please enjoy your stay.  You may enter the hot springs to the south.
  if %actor.quest_stage[%type%_wand]% == %wandstep%
    wait 1s
    eval minlevel (%wandstep% - 1) * 10
    if %actor.level% >= %minlevel%
      if %actor.quest_variable[%type%_wand:greet]% == 0
        mecho %self.name% says, 'I see you're crafting something.  If you want my help, we can talk about &6&b[upgrades]&0.'
      endif
    endif
  endif
  if !%actor.quest_stage[ice_shards]%
    wait 3
    if %actor.class% /= Cryomancer
      if %actor.has_completed[major_globe_spell]% && %actor.has_completed[relocate_spell_quest]% && %actor.has_completed[wall_ice]% && %actor.has_completed[waterform]% && %actor.has_completed[flood]%
        say Oh, %actor.name% %actor.title%!  I've heard of you!  You're quite talked about amongst our fellow cryomancers.
        if %actor.level% > 88
          mecho &0  
          say Makes me wonder if you could have mastered the most powerful cryomantic spell ever known.
        endif
      endif
    endif
  endif
endif
~
#10310
ice_shards_khysan_speech1~
0 d 100
spell spell? powerful powerful?  Ever ever? ~
if %actor.class% /= Cryomancer
  wait 2
  mecho %self.name% says, 'The spell was called Ice Shards.  Supreme cryomancers
  mecho &0could conjure a multitude of glistening ice fragments to annihilate their
  mecho &0foes.'
  wait 2s
  say But that magic is lost to us now.
endif
~
#10311
ice_shards_khysan_speech2~
0 d 100
lost lost?~
if %actor.class% /= Cryomancer
  wait 2
  nod
  mecho %self.name% says, 'Even my elven family, the Sunfire clan, has lost the
  mecho &0secrets to the spell.'
  wait 1s
  mecho %self.name% says, 'Legend has it the last copy of Ice Shards was in the
  mecho &0fabled lost library of Shiran.  But perhaps the spell survived in some ancient
  mecho &0text.'
  wait 3s
  mecho %self.name% says, 'I've heard of four books that could potentially hold the information:'
  mecho &3&bthe Book of Kings&0
  mecho &3&bthe Book of Discipline&0
  mecho &3&bThe Xapizan Codex&0
  mecho &3&bthe Enchiridion&0
  mecho 
  mecho %self.name% says, 'Maybe if I could get my hands on each of them, between
  mecho &0them I could piece together fragments of the spell!'
  wait 3s
  say Will you track down these books and bring them to me?
endif
~
#10312
ice_shards_khysan_speech3~
0 d 100
yes sure yep okay~
wait 2
if %actor.class% /= Cryomancer
  if %actor.level% > 88 && %actor.quest_stage[ice_shards]% == 0
    quest start ice_shards %actor.name%
    beam
    say Excellent!  I'm excited!
    wait 2s
    mecho %self.name% says, 'If you need, you can check your &6&b[ice shards progress]&0 with me.'
  elseif %actor.level% > 88 && %actor.quest_stage[ice_shards]% > 0
    say Oh please do share!
  elseif %actor.level% =< 88
    mecho %self.name% says, 'I appreciate your enthusiasm, but it's probably too
    mecho &0dangerous, right?'
  endif
endif
~
#10313
ice_shards_khysan_receive1~
0 j 100
16209 18505 55003 58415~
set stage %actor.quest_stage[ice_shards]%
if %stage% == 1
  if %actor.quest_variable[ice_shards:%object.vnum%]% == 1
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    say Thanks, but you already delivered %object.shortdesc% to me.
  else
    quest variable ice_shards %actor.name% %object.vnum% 1
    mjunk book
    wait 2
    mecho %self.name%'s eyes grow wide.
    mecho   
    say This is incredible!  I never imagined I'd be holding a copy of %object.shortdesc%!
    wait 2
    set book1 %actor.quest_variable[ice_shards:16209]%
    set book2 %actor.quest_variable[ice_shards:18505]%
    set book3 %actor.quest_variable[ice_shards:55003]%
    set book4 %actor.quest_variable[ice_shards:58415]%
    if %book1% && %book2% && %book3% && %book4%
      quest advance ice_shards %actor.name%
      emote slowly reads through the contents of each book.
      wait 4s
      emote takes copious notes.
      wait 1s
      say Hmmmm, I wonder if...
      wait 1s
      say No, that won't work...
      wait 4s
      emote sighs with frustration.
      say I hate to say it, but this isn't going to cut it.  The texts just don't have enough information about the spell.
      wait 3s
      ponder
      mecho %self.name% says, 'There is a mention of Shiran being near Mt. Frostbite in the Xapizan Codex however.  It also mentions a brother codex called &3&b"The Codex of War"&0.
      wait 2s
      say Supposedly, the Codex of War contains a detailed history of battles in the region.  Maybe if Shiran was destroyed in some kind of conflict, the Codex of War might have more clues.
      wait 2s
      say Try to find a copy and bring it back so we can take a look at it!
    else
      say Did you manage to find any of the other books?
    endif
  endif
endif
~
#10314
ice_shards_pawnbroker_speech~
0 d 0
butcher~
wait 2
if %actor.quest_stage[ice_shards]% == 4
  say Sorry, never heard of 'im.
  wait 1s
  cough
  emote very conspicuously lays his open hand on the counter.
  wait 2s
  mecho %self.name% says, 'I have some great old boots for sale though.
  mecho &0Only &7&b100 platinum&0.'
else
  say Ya might not wanna be so loud with that name round 'ere.
endif
~
#10315
ice_shards_pawnbroker_bribe~
0 m 100000
~
if %actor.quest_stage[ice_shards]% == 4
  quest advance ice_shards %actor.name%
  wait 2
  mecho %self.name% says, 'I don't know much, but I do know someone paid me to help
  mecho &0'im slip outta the city.  I'm talkin', they paid A LOT.  Was a dark-skinned elf
  mecho &0with a buncha orcs.'
  wait 5s
  mecho %self.name% says, 'Not the guy ya see skulkin' round the neighborhood or
  mecho &0nothin' neither, nah.  This guy looked like 'e worked with money.'
  wait 4s
  mecho %self.name% says, 'Dunno where they was goin' but I got a feelin' it were
  mecho &0pretty far.  They was all talkin' 'bout Ogakh or some such.  I dunno, my Orcish
  mecho &0ain't too good these days.'
  wait 4s
  say Best of luck catchin' 'im.  Now get out.
  mecho 
  emote points towards the door.
endif
~
#10316
ice_shards_slevvirik_speech~
0 d 100
butcher~
if %actor.quest_stage[ice_shards]% == 5
  quest advance ice_shards %actor.name%
  wait 2
  sigh
  mecho %self.name% says, 'Yes, I do know where he is and before you ask, no I don't mind
  mecho &0telling you.  My contractual obligation to him is fulfilled and if I do tell
  mecho &0you where he is, I doubt it will make much of a difference.'
  wait 5s
  mecho %self.name% says, 'That beast knew his reputation was going to catch up to him
  mecho &0sooner or later.  He paid us to hide him where no one in the world would find
  mecho &0him.  So we hid him off world.'
  wait 6s
  mecho %self.name% says, 'We opened a gateway to the Plane of Earth and sent him on his
  mecho &0way.  I suppose he's still there.'
  wait 4s
  say If he's still alive.
  wait 1s
  tip %actor%
endif
~
#10317
ice_shards_library_search~
2 c 100
search~
if %actor.quest_stage[ice_shards]% == 9
  quest advance ice_shards %actor.name%
  wload obj 10325
  wsend %actor% You find %get.obj_shortdesc[10325]%!
  wechoaround %actor% %actor.name% finds %get.obj_shortdesc[10325]%!
else
  return 0
endif
~
#10318
ice_shards_status_tracker~
0 d 0
ice shards progress~
wait 2
set stage %actor.quest_stage[ice_shards]%
if %stage% == 1
  set book1 %actor.quest_variable[ice_shards:16209]%
  set book2 %actor.quest_variable[ice_shards:18505]%
  set book3 %actor.quest_variable[ice_shards:55003]%
  set book4 %actor.quest_variable[ice_shards:58415]%
  say You are looking for four books of mystic knowledge.
  if %book1% || %book2% || %book3% || %book4%
    mecho  
    mecho You have brought me:
    if %book1%
      mecho - &3&b%get.obj_shortdesc[16209]%&0
    endif
    if %book2%
      mecho - &3&b%get.obj_shortdesc[18505]%&0
    endif
    if %book3%
      mecho - &3&b%get.obj_shortdesc[55003]%&0
    endif
    if %book4%
      mecho - &3&b %get.obj_shortdesc[58415]%&0
    endif
  endif
  mecho 
  mecho You still need to find:
  if !%book1%
    mecho - &3&b%get.obj_shortdesc[16209]%&0
  endif
  if !%book2%
    mecho - &3&b%get.obj_shortdesc[18505]%&0
  endif
  if !%book3%
    mecho - &3&b%get.obj_shortdesc[55003]%&0
  endif
  if !%book4%
    mecho - &3&b%get.obj_shortdesc[58415]%&0
  endif
elseif %stage% == 2
  say I need you to find the Codex of War.
elseif %stage% == 3
  mecho %self.name% says, 'You are looking for any records or journals Commander
  mecho &0Thraja keeps.'
elseif %stage% == 4
  mecho %self.name% says, 'Talk to the pawnbroker in Anduin about the Butcher of
  mecho &0Anduin so you can find his map.'
elseif %stage% == 5
  mecho %self.name% says, 'Talk to Slevvirik in Ogakh about the Butcher of Anduin
  mecho &0so you can find his map.'
elseif %stage% == 6
  say Bring the map of Ickle from the Butcher of Anduin.
elseif %stage% == 7
  mecho %self.name% says, 'You are looking for any kind of written clues about the
  mecho &0library at Shiran in Ysgarran's Keep in Frost Valley.'
elseif %stage% == 8
  mecho %self.name% says, 'You are looking for the Book of Redemption, whatever
  mecho &0that is.'
elseif %stage% == 9
  mecho %self.name% says, 'You are looking for the lost library of Shiran in Frost
  mecho &0Valley!'
elseif %stage% == 10
  say Have you found the magic book in the lost library??
elseif %actor.has_completed[ice_shards]%
  mecho %self.name% says, 'You've already done a miraculous thing by bring the
  mecho &0Aqua Mundi to me!
elseif !%stage%
  say Progress on what?  There's no fee to use the springs.
  smile %actor%
endif
~
#10319
ice_shards_khysan_receive2~
0 j 100
55004~
set stage %actor.quest_stage[ice_shards]%
if %stage% == 2
  wait 2
  quest advance ice_shards %actor.name%
  mjunk book
  emote cautiously takes %object.shortdesc% and places it on the reception desk.
  wait 2
  say Yikes, this looks pretty dangerous.
  emote opens %object.shortdesc% and begins to read.
  wait 3s
  emote quietly continues to read.
  wait 2s
  say The Codex talks extensively about the wars fought in Frost Valley and the surrounding areas.  It says the Frost Elves have been here since long before people arrived in Technitzitlan.
  wait 1s
  say The Codex also says something catastrophic happened to the elves' majestic city Shiran "ages ago," though it doesn't say what or when exactly.
  wait 7s
  emote closes %object.shortdesc%.
  wait 3s
  say But the fact that the elves are still here must mean something, right?
  wait 3s
  say Frost Valley is one of the only places left in Caelia where elves are frequently spotted.
  wait 3s
  say They've been locked in combat with a group of Amazons just east of here.  Why, I don't know, but I do know they've been skirmishing for a long time.
  wait 4s
  say The commander of the ice warriors, Thraja, certainly knows more.  Go and see if she keeps any records or journals.
  wait 3s
  say Bring back anything you find and we can look over it together.
endif
~
#10320
ice_shards_khysan_receive3~
0 j 100
58806~
set stage %actor.quest_stage[ice_shards]%
if %stage% == 3
  wait 2
  quest advance ice_shards %actor.name%
  mjunk book
  emote sits down and reads through Commander Thraja's journal.
  wait 3s
  say There is quite a lot of detail about the Frost Elves and their general movements, but it doesn't look like Thraja ever figured out where they were coming from.  She also notes the elves don't seem to be invading so much as patrolling the area.
  wait 3s
  emote flips a few more pages.
  wait 3s
  say Thraja also notes maps of the region are unreliable as the terrain can subtly shift, almost like the valley is changing somehow.
  wait 3s
  say She says the best map she ever had of the region was a slightly magical one she received from Kara-Sithri but it was stolen by someone called "The Butcher of Anduin."
  wait 6s
  emote looks up from the journal.
  say Oh my, that's not a subtle name at all is it?
  wait 3s
  say Apparently some kind of serial killer, the Butcher may have wanted the map to expand his reign of terror into the region around Ickle.  Or he may have just been looking for some valuables to pawn near his hideout.
  wait 6s
  say Thraja hasn't seen the map since.
  wait 4s
  emote closes the journal.
  wait 3s
  say Well if this map really was some kind of ancient magical device, it might help us determine where Shiran was.
  wait 4s
  say I guess you could ask around the pawnshop in Anduin, see if you can figure out what the Butcher did with the map.
endif
~
#10321
ice_shards_khysan_receive4~
0 j 100
48502~
set stage %actor.quest_stage[ice_shards]%
if %stage% == 6
  wait 2
  quest advance ice_shards %actor.name%
  mjunk map
  say Incredible, you found it!
  emote excitedly lays out the map.
  wait 1s
  emote scours every tiny detail of the map.
  wait 3s
  say Let me compare...
  mecho 
  emote opens to a passage in the Codex of War.
  emote glances between the Codex and the map.
  wait 4s
  say And in Thraja's journal...
  emote compares a passage in Thraja's journal.
  wait 3s
  say The Codex mentions a site of incredible significance in the middle of Frost Lake, and both the map and Thraja's journal point to the remains of a tower of some kind on an island in the lake.
  wait 6s
  say The map indicates the edges of a huge city that once overlooked the plateau at the west edge of the valley.  Thraja notes a man named Ysgarran built some kind of fortification over a preexisting structure out there...
  wait 7s
  say Go check it out.  Assuming that was Shiran, maybe there's a clue there?  Bring back anything you find, especially if you find any written records of the region.
endif
~
#10322
ice_shards_khysan_receive5~
0 j 100
53423~
set stage %actor.quest_stage[ice_shards]%
if %stage% == 7
  wait 2
  quest advance ice_shards %actor.name%
  mjunk book
  mecho %self.name% reads aloud, '"The Lost Library of Shiran".'
  say I... can't believe it's that obvious.
  wait 3s
  emote combs through the pages of the book.
  wait 4s
  say Okay, it's not that obvious. The book does talk about the library being destroyed for being a repository of too much mystical knowledge.  But it doesn't definitively say it was in Frost Valley.
  wait 6s
  say It does say the gods destroyed the library, claiming it was out of an abundance of caution, but was really an act of jealousy.
  wait 5s
  say What's even more interesting is what happened after!  A few of the gods repented and sought to atone for their crimes against Shiran.  They chronicled stories of good and evil as parables for what they had done and wrote those stories down into something called the "Book of Redemption."
  wait 7s
  say This book serves as the basis for a mystic path to salvation for wayward beings.  The writings themselves are so powerful they pose to drive the reader insane if they're unable to withstand the strain on their will.
  wait 6s
  say I hate to say it, but this sounds like a 'no stone unturned' situation.  If you can find a copy of this book, I'm willing to try to read it.
endif
~
#10323
ice_shards_khysan_receive6~
0 j 100
43013~
set stage %actor.quest_stage[ice_shards]%
if %stage% == 8
  wait 2
  quest advance ice_shards %actor.name%
  mecho Khysan carefully opens %object.shortdesc% and starts to read.
  wait 2s
  mecho &3&bLetters and images float up out of %object.shortdesc% and begin to dance around Khysan.&0
  mjunk book
  wait 3s
  mecho Khysan's eyes glaze over and his jaw goes slack.
  zone
  wait 5s
  mecho Khysan shakes his head and snaps back into reality.
  wait 2s
  say Here's what it says: The gods were so afraid of the sheer amount of mystical knowledge in Shiran they tried to destroy it.
  wait 3s
  say Only it turns out, by then the library was so magically defended even the gods couldn't just obliterate it.
  wait 2s
  say So instead, they ripped open the time stream and tossed the library into it, causing the Time Cataclysm that destroyed Shiran and loosed the Time Elementals on Ethilien!
  wait 6s
  say But according to the book, the people trapped in the library were eventually able to create a sort of "time key".  They convinced a mighty Time Elemental Lord to bring it to our epoch where, in theory, it should still be!  By bringing the key to the ruins of the tower and somehow activating it, one can traverse between the ages!!
  wait 9s
  say WHAT?!
  bounce
  wait 2s
  say This is AMAZING!!
  wait 3s
  say Well, I guess that means there's one more place to go!  Find that key, travel into the past, find a copy of Ice Shards, and bring it back to the future!
  wait 2s
  salute %actor.name%
  say Good luck my friend!
endif
~
#10324
ice_shards_khysan_receive7~
0 j 100
10325~
set stage %actor.quest_stage[ice_shards]%
if %stage% == 10
  wait 2
  mjunk book
  hug %actor.name%
  say I don't know how I can ever express my gratitude to you for bringing this to me.  The Sunfire clan will be sure this knowledge is never lost again.
  wait 4s
  say Come, let's read this together.
  emote opens the book and flips forward a few pages.
  wait 3s
  emote points to a page.
  mecho   
  say Here it is!
  wait 2s
  msend %actor% Although the spell is written in ancient Elvish, the intent and meaning are clear enough to replicate it!
  mecho   
  quest complete ice_shards %actor.name%
  mskillset %actor.name% ice shards
  msend %actor% &6&bYou have learned Ice Shards!&0
endif
~
#10325
Khysan refuse~
0 j 100
~
switch %object.vnum%
  case %wandgem%
  case %wandtask3%
  case %wandtask4%
  case %wandvnum%
    halt
    break
  default
    set stage %actor.quest_stage[ice_shards]%
    switch %stage%
      case 1
        switch %object.vnum%
          case 16209
          case 18505
          case 55003
          case 58415
            halt
            break
          default
            set response This isn't one of the four books I need to consult.
        done
        break
      case 2
        if %object.vnum% != 55004
          set response This isn't the Codex of War.
        endif
        break
      case 3
        if %object.vnum% != 58806
          set response This isn't Commander Thraja's journal...
        endif   
        break
      case 6
        if %object.vnum% != 48502
          set response Weird, this doesn't look like a map of Ickle.
        endif
        break
      case 7
        if %object.vnum% != 53423
          set response Do you think we can get new information from %object.shortdesc%?  I doubt it...
        endif
        break
      case 8
        if %object.vnum% != 43013
          set response Is this a book?
        endif
        break
      case 10
        if %object.vnum% != 10325
          set response Is this all you could find?
        endif
        break
      default
        set response Oh, is this a gift for me?  I appreciate it, but I'm fine for now.
    done
done
if %response%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  say %response%
endif
~
$~
