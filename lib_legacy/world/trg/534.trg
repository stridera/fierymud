#53400
avalanche~
2 b 5
~
if %actor.vnum% == -1
wecho You notice some larger clumps of snow slipping onto you.
wait 2s
wecho AVALANCHE!!! Run for your lives!
wait 1s
wteleport all 53567
endif
~
#53401
smothering_avalanche~
2 b 100
~
set person %self.people%
while %person%
   set next %people.next_in_room%
   eval tithe %person.level% * 2
   wsend %person% You are finding it harder to breathe now...and you can feel your life ebbing away.
   wdamage %person% %tithe%
   set person %next%
done
~
#53402
slip_on_trail~
2 g 5
~
wait 2
set rnd %random.char%
* this trigger is in multiple rooms, socheck we are in right one
if %rnd.room% == %actor.room%
* check for flying first...
if !(%rnd.aff_flagged[FLY]%)
set dam 0
wsend %rnd% You slip on the icy path and fall!
wechoaround %rnd% %rnd.name% slips and falls!
switch %actor.room%
case 53530
eval dam %dam% + 100
case 53529
eval dam %dam% + 75
case 53528
eval dam %dam% + 50
case 53527
eval dam %dam% + 25
done
wteleport %rnd% 53417
wdamage %rnd% %dam%
wat 53417 wforce %rnd% look
wat 53417 wechoaround %rnd% %rnd.name% comes tumbling down the ice trail from the east.
endif
endif
~
#53403
fv_ysgarran_greet1~
0 g 20
~
say Please enjoy the hospitality of our keep.
smile %actor.name%
~
#53404
fv_ysgarran_greet2~
0 g 30
~
msend %actor% %self.name% tells you, 'Hello Traveler, I am Ysgarran'
bow
~
#53405
fv_adwen_greet~
0 h 20
~
msend %actor% %self.name% tells you, 'Hello Traveller, my name is Adwen.'
curtsey %actor.name%
~
#53406
fv_Gwion_greet~
0 g 20
~
if %actor.vnum% == -1
wait 1
msend %actor% %self.name% tells you, 'Hello Traveler, my name is Gwion'
bow
else
wait 1
poke %actor.name%
say fancy a game of dice?
mforce %actor% emote mutters an excuse and looks around for an exit.
end
~
#53407
fv_Gwion_entry~
0 i 20
~
wait 1
peer
wait 2
sigh
say anyone fancy a game of dice?
~
#53408
fv_Aeron_greet~
0 g 20
~
msend %actor% %self.name% tells you, 'Hello Traveler, my name is Aeron.'
emote turns a page in his book.
~
#53409
fv_Aeron_entry~
0 i 10
~
set rnd %random.char%
if %rnd.vnum% == -1
mechoabout %rnd% %self.name% glances up at %rnd.name% then goes back to reading.
msend %rnd% %self.name% glances up at you then goes back to reading.
else
end
~
#53410
fv_Briant_greet~
0 g 20
~
if %actor.vnum% == -1
msend %actor% %self.name% tells you, 'Hello Traveler, my name is Briant.'
if %actor.sex% == male
msend %actor% As Briant smiles a welcome to you you feel yourself smiling like a fool!
else
msend %actor% As Briant smiles a welcome to you you feel an overwhelming peace.
end
else
end
~
#53411
fv_guard_entry~
0 i 20
~
emote peers around the room as if he expects to see an enemy in the shadows.
~
#53412
fv_guard_greet~
0 g 20
~
if %actor.vnum% == -1
consider %actor.name%
msend %actor% %self.name% tells you, 'Ok %actor.name%, we don't want any trouble here.'
else
bow
end
~
#53413
fv_guard_rand~
0 b 20
~
emote mutters to himself about his crappy posting to this forsaken corner of the world.
~
#53414
fv_poke_Aeron~
0 c 100
poke~
if %arg% /= aeron
  whap %actor.name%
  glare %actor.name%
  wait 2
  mecho %self.name% say, 'Don't interrupt me reading again, unless you have something better
  mecho &0for me than this book.'
endif
~
#53415
timetravel_shake~
1 c 3
shake~
switch %cmd%
  case s
  case sh
    return 0
    halt
done
if %self.name% /= %arg%
  if %actor.wearing[53424]%
    osend %actor% You shake the glass globe up and down vigorously.
    oechoaround %actor% %actor.name% shakes a glass globe up and down vigorously.
    if %actor.room% == 53466 || %actor.room% == 53570
      oecho The snow in the globe swirls...and your vision blurs for a second!
      if %actor.room% == 53466
        oteleport all 53570
        oforce all look
        set person %actor%
        set i %person.group_size%
        if %i%
          set a 1
        else
          set a 0
        endif
        while %i% >= %a%
         set person %actor.group_member[%a%]%
         if %person.room% == %actor.room%
            if !%person.quest_stage[frost_valley_quest]%
               quest start frost_valley_quest %person%
            endif
            quest variable frost_valley_quest %person% shake 1
         elseif %person%
            eval i %i% + 1
         endif
         eval a %a% + 1
        done
      elseif %actor.room% == 53570
        oteleport all 53466
        oforce all look
      endif
    endif
  else
    osend %actor% You need to hold it to shake it!
  endif
else
  return 0
endif
~
#53416
look_into_globe~
1 m 100
~
wait 3
set snowglobe snow-globe
set glassglobe glass-globe
if %actor.room% == 53466
   osend %actor% Hmm, the view in the globe is very similar to the one in this room, except the tower is whole.
elseif %actor.room% == 53570
   osend %actor% Wow, the globe is a miniature model of this room, freaky!
endif
~
#53417
**UNUSED**~
2 c 100
sh~
return 0
~
#53418
chaos_demon_death_banish~
0 f 100
~
set person %actor%
set i %actor.group_size%
if %i%
   unset person
   while %i% > 0
      set person %actor.group_member[%i%]%
      if %person.room% == %self.room%
         if %person.quest_stage[banish]% == 5
            quest advance banish %person.name%
            msend %person% &5&bA single letter pops into your mind. - &6&bG&0
         endif
      endif
      eval i %i% - 1
   done
elseif %person.quest_stage[banish]% == 5
   quest advance banish %person.name%
   msend %person% &5&bA single letter pops into your mind. - &6&bG&0
endif
~
#53450
major_globe_greet~
0 g 100
~
if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer) && (%actor.level% >= 57)
  if %actor.has_completed[major_globe_spell]%
    halt
  endif
  wait 1s
  switch %actor.quest_stage[major_globe_spell]%
    case 1
    case 2
    case 3
    case 4
    case 5
      emote coughs fitfully.
      wait 1s
      msend %actor% %self.name% says, 'Have you brought me the salve from Earle?'
      msend %actor% %self.name% looks at you pleadingly.
      mechoaround %actor% %self.name% looks at %actor.name% pleadingly.
      break
    case 6
    case 7
      msend %actor% %self.name% turns to look at you.
      mechoaround %actor% %self.name% turns to look at %actor.name%.
      wait 1s
      msend %actor% %self.name% says, 'Welcome back, were you able to find the spellbook?'
      break
    case 8
      msend %actor% %self.name% hops up from his chair as you enter.
      mechoaround %actor% %self.name% hops up from his chair as %actor.name% enters.
      wait 1s
      msend %actor% %self.name% comes limping over to you.
      mechoaround %actor% %self.name% limps over to %actor.name%.
      eval wards_left 5 - %actor.quest_variable[major_globe_spell:ward_count]%
      if %wards_left% > 1
        msend %actor% %self.name% says, 'The wards!  The wards!  Do you have them?  We still need %wards_left% of them!
      else
        msend %actor% %self.name% says, 'Just one ward left!  Do you have it?'
      endif
      break
    case 9
    case 10
      msend %actor% %self.name% bounds over to you.
      mechoaround %actor% %self.name% runs over to %actor.name%.
      wait 1s
      set final_item %actor.quest_variable[major_globe_spell:final_item]%
      msend %actor% %self.name% says, 'Do you have %get.obj_shortdesc[%final_item%]% to channel the power?  Please say you do!'
      break
    default
      msend %actor% %self.name% turns his head to look at you.
      mechoaround %actor% %self.name% turns his head to look at %actor.name%.
      cough
      wait 2s
      msend %actor% %self.name% says, 'You're a %actor.class%!  Perhaps you can &6&bassist&0 me.'
  done
endif
~
#53451
major_globe_speech_assist~
0 d 100
assist assist? help help?~
if (%actor.vnum% == -1) && (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class%== pyromancer) && (%actor.level% >= 57) && !(%actor.has_completed[major_globe_spell]%) && (%actor.quest_stage[major_globe_spell]% == 0)
  quest start major_globe_spell %actor.name%
  wait 2
  emote nods slowly.
  wait 1s
  msend %actor% %self.name% says, 'Yes...  I was recently defeated by a rogue demon roving this valley.  He injured me terribly, but...'
  emote breaks into a coughing fit!
  wait 2s
  emote recomposes himself.
  msend %actor% %self.name% says, 'I can't let down Ysgarran, though.  I must defeat the demon.'
  wait 3
  msend %actor% %self.name% says, 'But not in this state.'
  sigh
  wait 3s
  msend %actor% %self.name% says, 'Earle of the druids owes me a favor.  Please visit him in my behalf and acquire a salve for my wounds.'
  wait 1s
  msend %actor% %self.name% says, 'Tell him &6&b"Lirne sends me."&0  I'm sure he will help.'
  wait 2s
  msend %actor% %self.name% says, 'You can check your &6&b[spell progress]&0 with me if you forget what to do.'
endif
~
#53452
major_globe_speech_lirne_sends_me~
0 ad 0
Lirne sends me~
wait 1
if %actor.quest_stage[major_globe_spell]% == 1
  quest advance major_globe_spell %actor.name%
  smirk
  msend %actor% %self.name% says, 'Ah yes, Lirne.  He never writes and only drops by when he's injured.'
  wait 3s
  msend %actor% %self.name% says, 'Well, what got him this time?  Lions?  Tigers?  Be- ah, it doesn't matter.'
  sigh
  wait 2s
  msend %actor% %self.name% says, 'I will prepare an all-purpose healing salve for him.  But you will need to retrieve the components for me.'
  ponder
  wait 3s
  msend %actor% %self.name% says, 'First, I will need some &3&bshale&0 to create a base.  You can probably find some on the nearby volcanic island.'
  wait 2s
  msend %actor% %self.name% says, 'Return to me when you have procured the shale.'
elseif %actor.quest_stage[major_globe_spell]% == 2
  msend %actor% %self.name% says, 'Well yes, I already know that.  Did you get the shale yet?'
elseif %actor.quest_stage[major_globe_spell]%
  msend %actor% %self.name% says, 'Yes, yes, I know!  You told me already!'
else
  smirk
  msend %actor% %self.name% says, 'I doubt it.'
endif
~
#53453
major_globe_greet_load_shale~
0 ah 100
~
if %actor.quest_stage[major_globe_spell]% == 2
   mjunk majorglobe-shale
   mload obj 53451
   wait 2s
   if %get.mob_count[48125]% && (%self.room% == 48112)
      mecho The savage children laugh and play, tossing a rock between themselves.
      wait 1s
      mecho One child, larger than the rest, suddenly grabs the rock out of the air.
      mecho She smiles wickedly and prances away from the group.
      hold shale
      wait 3s
      mecho The other savage children sniffle and start playing another game.
   else
      mecho The lone savage child looks around herself and sighs.
      hold shale
      wait 3s
      mecho The savage child plays with a rock in her hands.
   endif
endif
~
#53454
Lirne Refuse~
0 j 0
53450 53452 53453 53454 53455 53456 53457 53458 53459 53460 53461~
set stage %actor.quest_stage[major_globe_spell]%
switch %object.vnum%
  case 53451
    if %stage% == 2
      set response Give this to Earle, not me.
    else
      set response No thanks.
    endif
    break
  case 58002
    if %stage% == 3
      set response Give this to Earle, not me.
    else
      set response No thanks.
    endif
    break 
  case 58609
    if %stage% == 4
      set response Give this to Earle, not me.
    else
      set response No thanks.
    endif
    break
  default
    set response No thanks.
done
if %response%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  say %response%
endif
~
#53455
major_globe_command_search~
2 ac 100
search~
switch %cmd%
  case s
    return 0
    halt
done
if (%actor.quest_stage[major_globe_spell]% == 6) && (%actor.quest_variable[major_globe_spell:room]% == %actor.room%)
  wload obj 53452
  wechoaround %actor% %actor.name% finds %get.obj_shortdesc[53452]%!
  wsend %actor% &bYou have found %get.obj_shortdesc[53452]%!&0
  quest advance major_globe_spell %actor.name%
else
  return 0
endif
~
#53456
**UNUSED**~
2 ac 100
se~
* Allow sell and south to work in this room
return 0
~
#53457
major_globe_elemental_greet~
0 ah 100
~
switch %self.vnum%
  * plant elemental
  case 2328
    set load_ward 53453
    set wand %actor.quest_stage[acid_wand]%
    break
  * Mist elemental
  case 2806
  case 2807
    set load_ward 53454
    set wand %actor.quest_stage[air_wand]%
    break
  * Water elemental
  case 2808
  case 2809
  case 48631
    set load_ward 53455
    break
  * flame elemental
  case 5212
  case 12523
  case 48500
  case 48511
  case 48512
    set load_ward 53456
    set wand %actor.quest_stage[fire_wand]%
    break
  * ice elemental
  case 53312
  case 53313
  case 48630
  case 48632
    set load_ward 53457
    set wand %actor.quest_stage[ice_wand]%
    break
done
* If random is 1
if %actor.quest_stage[major_globe_spell]% == 8 || %wand% == 8
  set now %time.stamp%
  * If there was a previous time stamp, check against it
  if %last_enter%
    * If 2 minutes have passed, roll for chance to load again
    if %now% - %last_enter% >= 2
      set do_load %random.4%
    endif
  * If first time entering, roll for chance to load
  else
    set do_load %random.4%
  endif
  * If rolled 1, load
  if %do_load% == 1
    if (%load_ward%)
      mload obj %load_ward%
      if %actor.quest_stage[major_globe_spell]% == 8
        quest variable major_globe_spell %actor.name% ward_%load_ward% 1
      endif 
      wait 1
      msend %actor% &b%self.name% flares briefly as you approach.&0
      mechoaround %actor% &b%self.name% flares briefly as %actor.name% approaches.&0
    endif
  * If didn't roll 1, save time stamp so questor must wait 2 minutes
  else
    * Save time stamp
    set last_enter %now%
    global last_enter
  endif
endif
~
#53458
major_globe_channel_greet~
0 ah 100
~
if %actor.quest_stage[major_globe_spell]% == 9
   switch %self.vnum%
      case 2322
         set load_channel 53458
         break
      case 58008
         set load_channel 53459
         break
      case 16003
         set load_channel 53460
         break
      case 23711
         set load_channel 53461
   done
   if %load_channel%
      mjunk majorglobe-channel
      mload obj %load_channel%
      wait 1
      msend %actor% &b%self.name%'s eyes flash briefly as you approach.&0
      mechoaround %actor% &b%self.name%'s eyes flash briefly as %actor.name% approaches.&0
   endif
endif
~
#53459
major_globe_hint~
0 ab 20
~
scratch
say Lirne has been gone for quite a while now...
wait 2s
say I wonder where that rascal has gotten to.
ponder
~
#53460
major_globe_hint_speech~
0 ad 1
gone? gone while? while rascal? rascal lirne? lirne~
wait 2
msend %actor% %self.name% says, 'Yeah, Lirne.  He disappeared a little while ago on a job in Frost Valley.'
frown
wait 2s
msend %actor% %self.name% says, 'None of us have heard from him since.  I hope that ol' warmage hasn't gotten himself into more trouble than he can handle.'
~
#53461
major_globe_channel_advance~
1 agi 100
~
if %victim%
   if %victim.quest_stage[major_globe_spell]% == 9
      quest advance major_globe_spell %victim.name%
   endif
else
   if %actor.quest_stage[major_globe_spell]% == 9
      quest advance major_globe_spell %actor.name%
   endif
endif
~
#53462
major_globe_earle_receive_shale~
0 j 100
53451~
if %actor.quest_stage[major_globe_spell]% == 2
  * chunk of shale
  wait 1
  quest advance major_globe_spell %actor.name%
  mjunk majorglobe-shale
  nod
  msend %actor% %self.name% says, 'This will do.'
  ponder
  wait 2s
  msend %actor% %self.name% says, 'Next you will need to retrieve some alcohol for the salve.  Make it &3&bsake&0, actually.  That would be best.'
  wait 5s
  msend %actor% %self.name% says, 'Well, go on then!  Don't keep Lirne waiting.'
  emote starts crushing the shale in a bowl.
endif
~
#53463
major_globe_earle_receive_sake~
0 j 100
58002~
if %actor.quest_stage[major_globe_spell]% == 3
  * bottle of sake
  wait 1
  quest advance major_globe_spell %actor.name%
  mjunk sake
  msend %actor% %self.name% says, 'Good, good.  This will mix well.  Just one more ingredient.  We'll need some &3&bmarigold poultice&0 to complete this salve.'
  think
  wait 3s
  msend %actor% %self.name% says, 'It's commonly used by healers, perhaps you should track one down.'
endif
~
#53464
major_globe_earle_receive_marigold~
0 j 100
58609~
if %actor.quest_stage[major_globe_spell]% == 4
  * marigold salve
  wait 1
  quest advance major_globe_spell %actor.name%
  mjunk marigold-poultice
  msend %actor% %self.name% says, 'Excellent, let me just finish this salve then...'
  emote begins mixing the items in a clay bowl.
  wait 2s
  emote recites a short incantation over the bowl.
  wait 2s
  emote pours the salve into a tiny jar.
  msend %actor% %self.name% says, 'There we are, this will surely cure whatever ails Lirne.'
  mload obj 53450
  give herbal-salve %actor.name%
  wait 5s
  msend %actor% %self.name% says, 'Well, don't keep him waiting.  Go, go!'
endif
~
#53465
Major Globe Lirne receive 1~
0 j 100
53450~
set stage %actor.quest_stage[major_globe_spell]%
if (%stage% == 5)
  quest advance major_globe_spell %actor.name%
  set room %random.5%
  switch %room%
    * Haunted House
    case 1
      eval room 16903
      break
    * Tower in the Wastes
    case 2
      eval room 12553
      break
    * Mystwatch
    case 3
      eval room 16063
      break
    * Abbey
    case 4
      eval room 18582
      break
    * Sunken
    case 5
      eval room 53079
      break
    * Sunken
    default
      eval room 53078
  done
  quest variable major_globe_spell %actor.name% room %room%
  wait 1
  mjunk majorglobe-salve
  smile
  emote sniffs %get.obj_shortdesc[53450]%.
  msend %actor% %self.name% says, 'Smells powerful.'
  wait 1s
  emote begins spreading the salve across his lesions.
  wait 2s
  msend %actor% %self.name% says, 'I can feel it working.'
  smile
  wait 1s
  msend %actor% %self.name% says, 'Well, no time to waste.  The demon still walks the valley.  To defeat it, we will need to combat its magic.'
  ponder
  wait 3s
  msend %actor% %self.name% says, 'I think I know a way...  Yes...'
  wait 1s
  msend %actor% %self.name% says, 'Lore speaks of a spell powerful enough to deflect great magic.'
  ponder
  wait 2s
  msend %actor% %self.name% says, 'The spellbook containing the spell was lost, though.  Perhaps you can find it.'
  wait 3s
  msend %actor% %self.name% says, '&b&6Search&0 in each &6&bstack&0 and &6&blibrary&0!  Surely it will be found in one of them.'
  wait 2s
  msend %actor% %self.name% says, 'I'm sure it still exists...  It must.'
  wait 5s
  msend %actor% %self.name% says, 'Well?  Quickly, now!'
elseif (%stage% < 5)
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, 'Perhaps you should do the steps of this quest in order.  Go talk to Earle.'
elseif (%stage% > 5)
  wait 1
  mjunk majorglobe-salve
  msend %actor% %self.name% says, 'But... you've already given me this.  I'm healed already.'
endif
~
#53466
Major Globe Lirne receive 2~
0 j 100
53452~
set stage %actor.quest_stage[major_globe_spell]%
if (%stage% == 7)
  quest advance major_globe_spell %actor.name%
  wait 1
  mjunk majorglobe-spellbook
  gasp
  msend %actor% %self.name% says, 'This is fantastic!  I knew you would find it.'
  emote opens the spellbook and begins leafing through it.
  wait 3s
  emote begins to look very excited.
  msend %actor% %self.name% says, 'Ah ha!  Here it is...  The spell to protect against powerful magic.  We'll need some &3&belemental wards&0 to power it, though.'
  snap
  wait 3s
  msend %actor% %self.name% says, 'You must banish some elementals and return their energies to me.  I think five should do it: mist, water, ice, flame, or plant.'
  wait 2s
  msend %actor% %self.name% says, 'Yes... go and &6&bbanish five unique elementals&0 and bring back their energies!'
elseif (%stage% < 7)
  return 0
  eyebrow
  wait 2
  msend %actor% %self.name% says, 'How could you have found this?  Do the quest in order!'
elseif (%stage% > 7)
  wait 1
  mjunk majorglobe-spellbook
  msend %actor% %self.name% says, 'You've already brought me this!
endif
~
#53467
Major Globe Lirne receive 3~
0 j 100
53453 53454 53455 53456 53457~
set stage %actor.quest_stage[major_globe_spell]%
if (%stage% == 8)
  wait 1
  set ward %actor.quest_variable[major_globe_spell:ward_%object.vnum%]%
  set number %object.vnum%
  mjunk majorglobe-ward
  if %ward% == 0
    frown
    msend %actor% %self.name% says, 'How could you have gotten this?  Do the quest yourself!'
  elseif %ward% == 1
    eval ward %actor.quest_variable[major_globe_spell:ward_%number%]% + 1
    quest variable major_globe_spell %actor.name% ward_%number% %ward%
    unset ward
    eval wards %actor.quest_variable[major_globe_spell:ward_count]% + 1
    quest variable major_globe_spell %actor.name% ward_count %wards%
    eval wards_left 5 - %wards%
    smile
    if %wards_left%
      msend %actor% %self.name% says, 'Excellent, only %wards_left% more, and we'll be almost ready.'
    else
      quest advance major_globe_spell %actor.name%
      msend %actor% %self.name% says, 'Okay!  That's enough elemental wards to power the spell.  We only need one more item to channel the power...'
      think
      emote quickly studies the spellbook again.
      set item %random.4%
      switch %item%
        case 1
          set item 53458
          set place in a border keep
          break
        case 2
          set item 53459
          set place on an emerald isle
          break
        case 3
          set item 53460
          set place within a misty fortress
          break
        default
          set item 53461
          set place in an underground city
      done
      quest variable major_globe_spell %actor.name% final_item %item%
      wait 3s
      msend %actor% %self.name% says, 'Yes, the last item for the spell is here.  It is &3&b%get.obj_shortdesc[%item%]%&0.'
      emote thinks hard for a moment.
      wait 2s
      msend %actor% %self.name% says, 'It is rumored that it can be found &6&b%place%&0.'
      wait 4s
      msend %actor% %self.name% says, 'Quickly, go now and retrieve it so that we might defeat the demon!'
    endif
  elseif %ward% == 2
    eval wards_left 5 - %actor.quest_variable[major_globe_spell:ward_count]%
    eyebrow
    if %wards_left% > 1
      msend %actor% %self.name% says, 'You've already given me this ward.  We still need %wards_left% different ones!
    else
      msend %actor% %self.name% says, 'You've already given me this ward.  We still need %wards_left% different one!
    endif
  endif
elseif (%stage% < 8)
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, 'I'm not quite ready to deal with those yet.  Do the quest in order.'
elseif (%stage% > 8)
  wait 1
  mjunk majorglobe-ward
  msend %actor% %self.name% says, 'You already gave me all the elemental wards!'
endif
~
#53468
Major Globe Lirne receive 4~
0 j 100
53458 53459 53460 53461~
set stage %actor.quest_stage[major_globe_spell]%
if (%stage% == 10)
  if %actor.quest_variable[major_globe_spell:final_item]% == %object.vnum%
    wait 1
    mjunk majorglobe-channel
    mskillset %actor.name% major globe
    emote grins excitedly.
    msend %actor% %self.name% says, 'Yes!  Now I can cast the spell... and defeat the demon!'
    emote holds the wards in one hand.
    wait 2s
    msend %actor% %self.name% starts casting &3&b'major globe'&0...
    wait 2s
    emote completes his spell...
    emote closes his eyes and utters the words, 'wiyaf travo'.
    mecho &1&bA shimmering globe of force wraps around %self.name%'s body.&0
    wait 2s
    smile
    msend %actor% %self.name% says, 'I feel much better too!  My sincerest thanks for your help.  As thanks, I offer you knowledge of this ancient spell also.'
    wait 1s
    msend %actor% %self.name% stares at you and utters a quick incantation.
    msend %actor% &7&bYou feel your skill in spell knowledge improving!&0
    mechoaround %actor% %self.name% stares at %actor.name% and utters a quick incantation.
    wait 1s
    quest complete major_globe_spell %actor.name%
    wait 1s
    msend %actor% %self.name% says, 'Now I must take my leave.  Wish me well!'
    emote hurries out of the room.
    mgoto 1100
    mpurge %self%
  else
    set response This isn't what the spell calls for...
  endif
elseif (%stage% < 10)
  set response I'm not ready for that yet.  Do the quest in the right order.
endif
if %response%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, '%response%'
endif
~
#53499
major_globe_quest_status_checker~
0 d 0
spell progress~
wait 2
set stage %actor.quest_stage[major_globe_spell]%
switch %stage%
  case 1
    msend %actor% %self.name% says, 'Find Earle and tell him &6&b"Lirne sends me."&0'
    break
  case 2
    msend %actor% %self.name% says, 'Find &3&bshale&0 on Fiery Island.  Bring it to Earle on Griffin Island.'
    break
  case 3
    msend %actor% %self.name% says, 'Find &3&bsake&0 in Odaishyozen.  Bring it to Earle on Griffin Island.'
    break
  case 4
    msend %actor% %self.name% says, 'Find a &3&bmarigold poultice&0 on a healer in South Caelia.  Bring it to Earle on Griffin Island.'
    break
  case 5
    msend %actor% %self.name% says, 'Bring me the &3&bsalve&0 Earle prepared.'
    break
  case 6
    msend %actor% %self.name% says, 'Find the &3&blost spellbook&0 in a &6&blibrary&0 or &6&bstack&0.'
    break
  case 7
    msend %actor% %self.name% says, 'Bring &3&b%get.obj_shortdesc[53452]%&0 to me.'
    break
  case 8
    set plant %actor.quest_variable[major_globe_spell:ward_53453]%
    set mist %actor.quest_variable[major_globe_spell:ward_53454]%
    set water %actor.quest_variable[major_globe_spell:ward_53455]%
    set flame %actor.quest_variable[major_globe_spell:ward_53456]%
    set ice %actor.quest_variable[major_globe_spell:ward_53457]%
    msend %actor% %self.name% says, 'Bring &3&b5 elemental wards&0, one each from a mist, a water, an ice, a flame, and a plant elemental.'
    eval wards %actor.quest_variable[major_globe_spell:ward_count]%
    if %wards%
      msend %actor% You have found: &3&b%wards% wards&0
    else
      msend %actor% You have found: &3&b0 wards&0
    endif
    if %plant% == 2
      msend %actor% %get.obj_shortdesc[53453]%
    endif
    if %mist% == 2
      msend %actor% %get.obj_shortdesc[53454]%
    endif
    if %water% == 2
      msend %actor% %get.obj_shortdesc[53455]%
    endif
    if %flame% == 2
      msend %actor% %get.obj_shortdesc[53456]%
    endif
    if %ice% == 2
      msend %actor% %get.obj_shortdesc[53457]%
    endif
    break
  case 9
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
    msend %actor% %self.name% says, 'Find &3&b%get.obj_shortdesc[%final_item%]%&0 in %place%.'
    break
  case 10
    set final_item %actor.quest_variable[major_globe_spell:final_item]%
    msend %actor% %self.name% says, 'Bring me &6&b%get.obj_shortdesc[%final_item%]%&0.'
done
~
#53500
Block player exit~
2 g 100
~
if %actor.vnum% == -1
   if %actor.level% < 100
      return 0
      wsend %actor% &bA mysterious powerful force pushes you back.&0
   else
      return 1
      wsend %actor% &bThe room you are entering is part of the past elven tower illusion.&0
      wsend %actor% &bIt should not be reachable by any player. Look BUILDNOTE for more info.&0
   endif
endif
~
#53501
Destroy past elf~
2 g 100
~
if (%actor.vnum% >= 53500) & (%actor.vnum% <= 53508)
   return 0
   wsend %actor% You can't go that way!
endif
~
#53502
Frost elf wield blade~
0 k 100
~
if !%wielded%
   if %self.wearing[53420]%
      emote whispers, 'Trespassers!'
      rem elven-blade
      wield elven-blade
      set wielded %time.stamp%
      global wielded
   endif
endif
~
#53503
Frost elf remove blade~
0 b 100
~
if %wielded%
   set now %time.stamp%
   if %now% - 1 > %wielded%
      scan
      wait 1s
      rem blade
      wear blade belt
      emote returns to a more relaxed posture, watching the vicinity carefully.
      set wielded 0
      global wielded
   endif
endif
~
#53504
Staff damage user~
1 ac 1
use~
return 0
switch %cmd%
  case u
    halt
done
set aliases elaborately-chiseled-staff deep-brown-maple-staff
if %self.val2% && (%arg% != ) && (%aliases% /= %arg%) && (%actor.wearing[53504]%)
   eval damage 480 + %random.50%
   odamage %actor% %damage%
   osend %actor% You double over in pain as %self.shortdesc% touches the ground! (&1&b%damage%&0)
   oechoaround %actor% %actor.name% doubles over in pain as %self.shortdesc% touches the ground! (&4%damage%&0)
endif
~
#53505
**UNUSED**~
1 ac 1
u~
return 0
~
#53506
Frost elf quest death~
0 f 100
~
set person %actor%
set i %person.group_size%
if %i%
   set a 1
else
   set a 0
endif
while %i% >= %a%
   set person %actor.group_member[%a%]%
   if %person.room% == %self.room%
      if %person.quest_variable[frost_valley_quest:shake]% == 1
         quest variable frost_valley_quest %person.name% elf 1
      endif
   elseif %person%
      eval i %i% + 1
   endif
   eval a %a% + 1
done
~
$~
