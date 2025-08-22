#2800
waterform_wave_greet~
0 h 100
~
wait 2
set stage %actor.quest_stage[waterform]%
if %stage% == 0
  if %actor.class% /= Cryomancer
    say How nice to see someone other than those dreadful pirates.
    if %actor.level% > 72
      wait 2s
      mecho %self.name% says, 'Oh, and such a capable water master too.  I could offer
      mecho &0you a modicum of protection against those pirates if you like.  Would you be
      mecho &0interested?'
    endif
  endif
elseif %stage% == 1
  say Did you find a piece of armor made of water?
elseif %stage% == 2
  say You should be out killing Tri-Aszp.
elseif %stage% == 3
  say Have you found a suitably large white dragon bone?
elseif %stage% == 4 || %stage% == 5
  say Ah, how are your water creature sample studies going?
elseif %stage% == 6
  say Do you need to confer about your notes on water sources?
elseif %stage% == 7
  peer %actor%
  wait 1s
  mecho %self.name% says, 'Ah I can see you have gleaned secrets from the waters!
  mecho &0Give me the dragon bone cup when you're ready.'
endif
~
#2801
waterform_wave_speech1~
0 d 1
yes protection? how? okay~
wait 2
set stage %actor.quest_stage[waterform]%
if %stage% == 0 && %actor.class% /= Cryomancer && %actor.level% > 72
  quest start waterform %actor.name%
  mecho %self.name% says, 'I can teach you to transform you into a raging torrent,
  mecho &0giving you the strength of the water itself!  All you need is a single piece of
  mecho &0armor made from water to wrap about you.'
*
* THIS. IS. A. LIE.  Yes, the quest master, a talking wave, gets the material components of its own spell wrong.
*
  wait 4s
  say Bring it to me and I will teach you the transformation!
  wait 2s
  mecho %self.name% says, 'If you forget, you can ask me for a reminder of your
  mecho &0&7&b[progress]&0.'
elseif %stage% == 1
  say Then please give it to me.
elseif %stage% == 2 || %stage% == 3
  say Then please give it to me.
elseif %stage% == 4
  say Splendid!  Please, keep at it!
elseif %stage% == 5
  say Splendid!  Give me the bone cup if you're done.
elseif %stage% == 6
  mecho %self.name% says, 'If you need, you may ask for a reminder of your
  mecho &0&7&b[progress]&0.'
endif
~
#2802
waterform_wave_receive~
0 j 100
~
set stage %actor.quest_stage[waterform]%
if %actor.quest_variable[waterform:new]% /= yes
  if %object.vnum% == 2807
    quest 
    wait 2
    say Yes, I can make a new dragon bone cup from this.
    mjunk %object%
    quest variable waterform %actor.name% new 0
    wait 2s
    mecho %self.name% transforms into a torrential swirling column!
    mecho The riptide whittles away at the ice white bone, carving it into a smooth cup.
    mecho %self.name% slowly resumes into its gentle undulating form, the cup floating on its crest.
    mload obj 2808
    wait 2s
    give dragon-bone-cup %actor.name%
    say Don't lose this again!
  else
    return 0
    wait 2
    say I can't make a new cup from this.
    mecho %self.name% refuses %object.shortdesc%.
  endif
elseif %stage% == 1
  if %object.vnum% == 51009
    quest advance waterform %actor.name%
    wait 2
    mjunk shield
    say Yes, this will do nicely.
    mecho %self.name% absorbs %get.obj_shortdesc[51009]% into its massive form!
    wait 2s
    say Now hold still.
    wait 2s
    mecho %self.name% crests up to a monumental height and pauses for just a moment.
    wait 4s
    msend %actor% %self.name% CRASHES down and engulfs you!
    mechoaround %actor% %self.name% CRASHES down and engulfs %actor.name%!
    wait 4s
    msend %actor% You feel your body start to soften and flow!
    msend %actor% You feel at one with the Great Wave!
    mechoaround %actor% %actor.name% to merges with the Great Wave!
    wait 6s
    msend %actor% Suddenly you feel your body solidify again.
    msend %actor% You begin to choke!
    mechoaround %actor% %actor.name% begins to struggle against the torrent!
    wait 5s
    mechoaround %actor% %self.name% recedes leaving %actor.name% heaving for breath.
    msend %actor% %self.name% recedes, leaving you gasping for air.
    wait 5s
    mecho %self.name% says, 'That didn't work quite right.  The Amorphous Shield alone
    mecho &0must not be powerful enough to sustain the transformation.  Perhaps we will
    mecho &0need to add a few other elements.'
    wait 3s
    mecho %self.name% says, 'I apologize, but if you wish to continue, you will have
    mecho &0to find a few other things.'
  else
    return 0
    say This isn't armor made of water.
    mecho %self.name% refuses %object.shortdesc%.
  endif
elseif %stage% == 3
  if %object.vnum% == 2807
    quest advance waterform %actor.name%
    wait 2
    mecho %self.name% examines %object.shortdesc%.
    mjunk thigh-bone
    say This bone will make an excellent water vessel.
    wait 2s
    mecho %self.name% transforms into a torrential swirling column!
    mecho The riptide whittles away at the ice white bone, carving it into a smooth cup.
    wait 5s
    mecho %self.name% slowly resumes into its gentle undulating form, the cup floating on its crest.
    mload obj 2808
    wait 1s
    give dragon-bone-cup %actor.name%
    mecho %self.name% says, 'Take this cup.  With it, take samples of &4&bfour living&0
    mecho &0water creatures&0.'
    mecho    
    mecho %self.name% says, 'Each creature must come from a &6&bdifferent region&0 of
    mecho &0the world.  So you'll need samples from more than just the denizens of the 
    mecho &0Blue-Fog river and road.'
    wait 6s
    mecho %self.name% says, 'Once you've gathered the four samples, return and give me
    mecho &0the cup.'
  else
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    say This won't make a usable vessel.
  endif
elseif %stage% == 4
  if %object.vnum% == 2808
    return 0
    say It seems you haven't collected all four samples yet.
    mecho %self.name% refuses %object.shortdesc%.
    wait 1s
    mecho %self.name% says, 'Do you need a reminder of your &7&b[progress]&0?'
  else
    return 0
    say Why are you bringing me this?
    mecho %self.name% refuses %object.shortdesc%.
    wait 1s
    say You should be out collecting living water samples.
  endif
elseif %stage% == 5
  if %object.vnum% == 2808
    quest advance waterform %actor.name%
    return 0
    say Yes, these samples are perfect.
    wait 2s
    emote rises up, coaxing wavering orbs of water out of the dragon bone cup.
    emote raises up the shield and merges the watery orbs with it.
    wait 3s
    mecho %self.name% says, 'One last thing, just to be sure.  Develop a deeper
    mecho &0personal insight on the nature of water.'
    wait 1s
    mecho %self.name% says, 'Using the cup again, &6&bexamine&0 six unique sources of
    mecho &0water.'
    mecho   
    mecho %self.name% says, 'Seek out:'
    mecho - &4&ba granite pool in the village of Mielikki&0
    mecho   
    mecho - &4&ba sparkling artesian well in the Realm of the King of Dreams&0
    mecho   
    mecho - &4&ba crystal clear fountain in the caverns of the Ice Cult&0
    mecho   
    mecho - &4&bthe creek in the Eldorian Foothills&0
    mecho  
    mecho - &4&bthe wishing well at the Dancing Dolphin in South Caelia&0
    mecho  
    mecho - &4&ban underground brook in the Minithawkin Mines&0
    wait 6s
    mecho %self.name% says, 'Once you have examined all six sites, return and I shall
    mecho &0try the transformation again.'
  else
    return 0
    mecho %self.name% says, 'You haven't been trying to collect samples in this have
    mecho &0you?'
  endif
elseif %stage% == 6
  if %object.vnum% == 2808
    return 0
    say You haven't completed all of your examinations yet!
    mecho %self.name% refuses %object.shortdesc%.
    wait 1s
    mecho %self.name% says, 'Do you need a reminder of your&7&b[progress]&0?'
  else
    return 0
    say What is this for?
    mecho %self.name% refuses %object.shortdesc%.
    wait 1s
    mecho %self.name% says, 'Do you need a reminder of your &7&b[progress]&0?'
  endif
elseif %stage%  == 7
  if %object.vnum% == 2808
    wait 2
    mjunk dragon-bone-cup
    mecho %self.name% says, 'Everything is ready!  I shall attempt the transformation
    mecho &0again.'
    wait 2s
    mecho %self.name% rises up like a tower on the water.
    mecho The Great Wave swells as the waters of the Blue Lake rise to meet it.
    wait 4s
    msend %actor% %self.name% crests and breaks, washing over you!
    mechoaround %actor% %self.name% crests and breaks, washing over %actor.name%!
    wait 6s
    msend %actor% You feel your body liquify as the Great Wave swirls over, in, and through you.
    msend %actor% You can feel the rushing waters of the world in every particle of your being as you join with the Great Wave and the Blue Lake.
    mechoaround %actor% %actor.name% liquifies and joins with the waters of the Great Wave and the Blue Lake.
    wait 5s
    mskillset %actor% waterform
    msend %actor% &4&bThe Great Wave imparts the method to transform your body into pure raging water!&0
    quest complete waterform %actor.name%
  else
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    say This is not the dragon bone cup.
  endif
else
  return 0
  say I don't remember asking you to retrieve this.
  mecho %self.name% refuses %object.shortdesc%.
endif
~
#2803
waterform_wave_speech2~
0 d 100
elements elements? what what? things things? like like?~
wait 2
if %actor.quest_stage[waterform]% == 2
  mecho %self.name% says, 'I believe the shield needs more embodied magic to sustain
  mecho &0a transformation.'
  wait 2s
  mecho %self.name% says, 'To gather this magic though, you'll need an appropriate
  mecho &0vessel.  I can carve such a thing for you if you bring me the proper materials.
  mecho &0The best vessels for magic samples come from dragon bone, and the most
  mecho &0appropriate dragons for working with water are white dragons.'
  wait 4s
  mecho %self.name% says, 'Since this will need to come from &6&ba single bone&0, it will
  mecho &0need to be a decently sized one.'
  wait 5s
  mecho %self.name% says, 'Bones that large are only found in adult dragons, so
  mecho &0you'll need to find a &6&bvery large white dragon&0.'
  wait 3s
  say Good luck!
endif
~
#2804
waterform_tri_death~
0 f 100
~
set i %actor.group_size%
if %i%
   set a 1
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[waterform]% == 2 || %person.quest_variable[waterform:new]% /= yes
            mload obj 2807
            quest advance waterform %person.name%
         endif
      elseif %person%
         eval i %i% + 1   
      endif
      eval a %a% + 1
   done
elseif %actor.quest_stage[waterform]% == 2 || %actor.quest_variable[waterform:new]% /= yes
   mload obj 2807
   quest advance waterform %actor.name%
endif
~
#2805
waterform_water_death~
0 f 100
~
set i %actor.group_size%
if %i%
   set a 1
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[waterform]% == 4 && (%person.inventory[2808]% || %person.wearing[2808]%)
            switch %self.vnum%
               case 2805
               case 2808
               case 2809
               case 11805
                  set number 1
                  break
               case 51001
               case 51019
               case 51021
                  set number 2
                  break
               case 4002
                  set number 3
                  break
               case 53004
                  set number 5
                  break
               case 48631
               default
                  set number 4
            done
            if %person.quest_variable[waterform:region%number%]% == 0
               quest variable waterform %person.name% region%number% 1
               msend %person% &4&bYou gather part of %self.name% in %get.obj_shortdesc[2808]%.&0
               mechoaround %person% &4&b%person.name% gathers part of %self.name% in %get.obj_shortdesc[2808]%.&0
            endif
            set region1 %person.quest_variable[waterform:region1]%
            set region2 %person.quest_variable[waterform:region2]%
            set region3 %person.quest_variable[waterform:region3]%
            set region4 %person.quest_variable[waterform:region4]%
            set region5 %person.quest_variable[waterform:region5]%
            if %region1% + %region2% + %region3% + %region4% + %region5% > 3
               msend %person% &4&bYou have gathered all the samples of living water you need!&0
               quest advance waterform %person.name%
            endif
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %actor.quest_stage[waterform]% == 4 && (%actor.inventory[2808]% || %actor.wearing[2808]%)
   switch %self.vnum%
      case 2805
      case 2808
      case 2809
      case 11805
         set number 1
         break
      case 51001
      case 51019
      case 51021
         set number 2
         break
      case 4002
         set number 3
         break
      case 53004
         set number 5
         break
      case 48631
      default
         set number 4
   done
   if %actor.quest_variable[waterform:region%number%]% == 0
      quest variable waterform %actor.name% region%number% 1
      msend %actor% &4&bYou gather part of %self.name% in %get.obj_shortdesc[2808]%.&0
      mechoaround %actor% &4&b%actor.name% gathers part of %self.name% in %get.obj_shortdesc[2808]%.&0
   endif
   set region1 %actor.quest_variable[waterform:region1]%
   set region2 %actor.quest_variable[waterform:region2]%
   set region3 %actor.quest_variable[waterform:region3]%
   set region4 %actor.quest_variable[waterform:region4]%
   set region5 %actor.quest_variable[waterform:region5]%
   if %region1% + %region2% + %region3% + %region4% + %region5% > 3
      msend %actor% &4&bYou have gathered all the samples of living water you need!&0
      quest advance waterform %actor.name%
   endif
endif
~
#2806
waterform_waters_examine~
1 m 100
~
if %actor.quest_stage[waterform]% == 6 && (%actor.wearing[2808]% || %actor.inventory[2808]%)
  if %actor.quest_variable[waterform:%self.vnum%]%
    wait 1
    osend %actor% &4&bYou have already examined this source.&0
    halt
  else
    wait 1
    osend %actor% &4&bYou gather some water from %self.shortdesc% in %get.obj_shortdesc[2808]%.&0
    quest variable waterform %actor.name% %self.vnum% 1
    oechoaround %actor% &4%actor.name% gathers some water from %self.shortdesc% in %get.obj_shortdesc[2808]%.&0
  endif
  set water1 %actor.quest_variable[waterform:3296]%
  set water2 %actor.quest_variable[waterform:58405]%
  set water3 %actor.quest_variable[waterform:53319]%
  set water4 %actor.quest_variable[waterform:55804]%
  set water5 %actor.quest_variable[waterform:58701]%
  set water6 %actor.quest_variable[waterform:37014]%
  if %water1% && %water2% && %water3% && %water4% && %water5% && %water6%
    quest advance waterform %actor%
    osend %actor% &4&bYour examinations of the water sources are complete!&0
  endif
endif
~
#2807
waterform_wave_status_checker~
0 d 100
status status? progress progress?~
wait 2
set stage %actor.quest_stage[waterform]%
if %stage% == 1
  mecho %self.name% says, 'Find a piece of armor made of water to serve as the basis
  mecho &0of your new form.'
elseif %stage% == 2
  mecho %self.name% says, 'You need a special vessel to gather water in.  Kill
  mecho &0Tri-Aszp and get a large bone from her.'
elseif %stage% == 3
  mecho %self.name% says, 'You need a special vessel to gather water in.  Give me
  mecho &0the large bone from a grown white dragon so I can make you one.'
elseif %stage% == 4
  set region1 %actor.quest_variable[waterform:region1]%
  set region2 %actor.quest_variable[waterform:region2]%
  set region3 %actor.quest_variable[waterform:region3]%
  set region4 %actor.quest_variable[waterform:region4]%
  set region5 %actor.quest_variable[waterform:region5]%
  say Collect samples of living water from four different regions.
  if %region1% || %region2% || %region3% || %region4% || %region5%
    mecho  
    mecho You already have samples from:
    if %region1%
      mecho - &4The Blue Fog trails and waters&0
    endif
    if %region2%
      mecho - &4Nordus&0
    endif
    if %region3%
      mecho - &4Layveran Labyrinth&0
    endif
    if %region4%
      mecho - &4The Elemental Plane of Water&0
    endif
    if %region5%
      mecho - &4The sunken castle&0
    endif
  endif
  mecho   
  eval samples 4 - (%region1% + %region2% + %region3% + %region4% + %region5%)
  mecho You need &4&b%samples%&0 more.
elseif %stage% == 5
  say Give me the cup so I can see the samples.
elseif %stage% == 6
  set water1 %actor.quest_variable[waterform:3296]%
  set water2 %actor.quest_variable[waterform:58405]%
  set water3 %actor.quest_variable[waterform:53319]%
  set water4 %actor.quest_variable[waterform:55804]%
  set water5 %actor.quest_variable[waterform:58701]%
  set water6 %actor.quest_variable[waterform:37014]%
  say You are looking for six unique sources of water.
  if %water1% || %water2% || %water3% || %water4% || %water5% || %water6%
    mecho  
    mecho You have already analyzed water from:
    if %water1%
      mecho - &4a granite pool in the village of Mielikki&0
    endif
    if %water2%
      mecho - &4a sparkling artesian well in the Realm of the King of Dreams&0
    endif
    if %water3%
      mecho - &4a crystal clear fountain in the caverns of the Ice Cult&0
    endif
    if %water4%
      mecho - &4the creek in the Eldorian Foothills&0
    endif
    if %water5%
      mecho - &4the wishing well at the Dancing Dolphin in South Caelia&0
    endif
    if %water6%
      mecho - &4an underground brook in the Minithawkin Mines&0
    endif
  endif
  mecho   
  mecho You still need to analyze water from:
  if !%water1%
    mecho - &6&ba granite pool in the village of Mielikki&0
  endif
  if !%water2%
    mecho - &6&ba sparkling artesian well in the Realm of the King of Dreams&0
  endif
  if !%water3%
    mecho - &6&ba crystal clear fountain in the caverns of the Ice Cult&0
  endif
  if !%water4%
    mecho - &6&bthe creek in the Eldorian Foothills&0
  endif
  if !%water5%
    mecho - &6&bthe wishing well at the Dancing Dolphin in South Caelia&0
  endif
  if !%water6%
    mecho - &6&ban underground brook in the Minithawkin Mines&0
  endif  
elseif %stage% == 7
  say Just return the cup to me and you're done!
elseif %actor.has_completed[waterform]%
  say I already showed you how to transform into water.
elseif !%stage%
  say But we're not doing anything together!
endif
if %stage% > 3
  mecho  
  mecho %self.name% says, 'If you need a new cup, say "&3&bI need a new cup&0".'
endif
~
#2808
**UNUSED**~
0 f 100
~
set i %actor.group_size%
if %i%
   while %i% > 0
      set person %actor.group_member[%i%]%
      if %person.room% == %self.room%
         if %person.quest_stage[waterform]% == 4 && (%person.inventory[2808]% || %person.wearing[2808]%)
            switch %self.vnum%
               case 2805
               case 2808
               case 2809
               case 11805
                  set number 1
                  break
               case 51001
               case 51019
               case 51021
                  set number 2
                  break
               case 4002
                  set number 3
                  break
               case 48631
                  set number 4
                  break
               case 53004
                  set number 5
                  break
               default
                  return 0
            done
            if %person.quest_variable[waterform:region%number%]% == 0
               quest variable waterform %person.name% region%number% 1
               msend %person% &4&bYou gather part of %self.name% in %get.obj_shortdesc[2808]%.&0
               mechoaround %person% &4&b%person.name% gathers part of %self.name% in %get.obj_shortdesc[2808]%.&0
            endif
            set region1 %person.quest_variable[waterform:region1]%
            set region2 %person.quest_variable[waterform:region2]%
            set region3 %person.quest_variable[waterform:region3]%
            set region4 %person.quest_variable[waterform:region4]%
            set region5 %person.quest_variable[waterform:region5]%
            if %region1% + %region2% + %region3% + %region4% + %region5% > 3
               msend %person% &4&bYou have gathered all the samples of living water you need!&0
               quest advance waterform %person.name%
            endif
         endif
      endif
      eval i %i% - 1
   done
elseif %actor.quest_stage[waterform]% == 4 && (%actor.inventory[2808]% || %actor.wearing[2808]%)
   switch %self.vnum%
      case 2805
      case 2808
      case 2809
      case 11805
         set number 1
         break
      case 51001
      case 51019
      case 51021
         set number 2
         break
      case 4002
         set number 3
         break
      case 48631
         set number 4
         break
      case 53004
         set number 5
         break
   default
      return 0
   done
   if %actor.quest_variable[waterform:region%number%]% == 0
      quest variable waterform %actor.name% region%number% 1
      msend %actor% &4&bYou gather part of %self.name% in %get.obj_shortdesc[2808]%.&0
      mechoaround %actor% &4&b%actor.name% gathers part of %self.name% in %get.obj_shortdesc[2808]%.&0
   endif
   set region1 %actor.quest_variable[waterform:region1]%
   set region2 %actor.quest_variable[waterform:region2]%
   set region3 %actor.quest_variable[waterform:region3]%
   set region4 %actor.quest_variable[waterform:region4]%
   set region5 %actor.quest_variable[waterform:region5]%
   if %region1% + %region2% + %region3% + %region4% + %region5% > 3
      msend %actor% &4&bYou have gathered all the samples of living water you need!&0
      quest advance waterform %actor.name%
   endif
endif
*
* p3 mini boss death trigger 55599
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 4 pieces of armor per sub_phase in phase_3
set what_armor_drop %random.4%
* 11 classes questing in phase_3
set what_gem_drop %random.11%
* 
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70 
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55714
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55725
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55736
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 &%will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      *
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55714
      eval armor_vnum %what_armor_drop% + 55371
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55725
      eval armor_vnum %what_armor_drop% + 55375
      mload obj %armor_vnum%
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55736
      eval armor_vnum %what_armor_drop% + 55379
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   endif
endif
~
#2809
**UNUSED**~
1 c 3
e ex~
return 0
~
#2810
waterform_cup_replacement~
0 d 0
I need a new cup~
wait 2
if %actor.quest_stage[waterform]% > 3 && %actor.quest_variable[waterform:new]% == 0
  quest variable waterform %actor.name% new yes
  say Oh no, you lost the cup??
  wait 1s
  mecho %self.name% says, 'Well, I can make a new one, but you'll need to find the
  mecho &0base materials again.'
  wait 2s
  say Go find another acceptable dragon bone.
endif
~
$~
