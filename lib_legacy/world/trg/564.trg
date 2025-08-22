#56400
hell_gate_diabolist_greet~
0 h 100
~
wait 2
set stage %actor.quest_stage[hell_gate]%
if %actor.class% /= Diabolist && %stage% == 0
  bow %actor%
  mecho %self.name% says, 'Welcome, dark disciple.
  mecho Has the hellish realm Garl'lixxil called you here as well?'
elseif %actor.class% /= Paladin || %actor.class% /= Priest
  say You are not welcome here!
  kill %actor.name%
endif
if %stage% == 1
  say Have you found the spider-shaped dagger?
elseif %stage% == 2
  say Have you been able to locate the seven keys?
elseif %stage% == 3
  mecho %self.name% says, 'Ah, have you returned with the blood of the
  mecho sacrifices? &7&b[Drop]&0 it on the ground.'
elseif %stage% == 4
  say Give me the dagger to finish the rite.
elseif %stage% == 5
  say Quickly, slay Larathiel!
endif
~
#56401
hell_gate_diabolist_speech1~
0 d 100
yes~
wait 2
if %actor.class% /= Diabolist && %actor.quest_stage[hell_gate]% == 0
  if %actor.level% > 80
    mecho %self.name% says, 'Then it must be our fate to open the gates of
    mecho &0Hell together.  Come, &7&b[enter]&0 the &7&b[circle]&0 and join me.'
  else
    mecho %self.name% says, 'Interesting...
    mecho &0You are not yet strong enough to join me in my task.  Continue your journey'
    mecho &0and return when you are more experienced.'
  endif
elseif %actor.quest_stage[hell_gate]% > 0
  say Let me see!
endif
~
#56402
hell_gate_diabolist_speech2~
0 d 100
no~
wait 2
if %actor.class% /= Diabolist && %actor.quest_stage[hell_gate]% == 0
  say Pity for you.  The demons must not think you worthy.
elseif %actor.quest_stage[hell_gate]% > 0
  say What are you waiting for?  Get to work!
endif
~
#56403
hell_gate_diabolist_command_enter~
2 c 100
enter~
switch %cmd%
  case e
    return 0
    halt
done
switch %arg%
  case c
  case ci
  case cir
  case circ
  case circl
  case circle
    if %actor.class% /= Diabolist && %actor.quest_stage[hell_gate]% == 0
      set priest %get.mob_shortdesc[56400]%
      if %actor.level% > 80
        wsend %actor% The circle of &1fire&0 burns hotly, enclosing you within.
        wechoaround %actor% The circle of &1fire&0 burns hotly, enclosing %actor.name% within.
        wait 1s
        wecho A rumbling voice as deep as the pits of hell groans out of the fiery earth!
        wecho '&1Delightful supplicants, I bid you welcome.  Open the door to Garl'lixxil and bring me to the world.  I will teach you powerful secrets so you may join me in slaughter.&0'
        wait 5s
        wecho '&1To begin, find a dagger shaped like an arachnid.  You, %actor.name%, shall seek it out and return here.  The other shall remain here and ensure the island is not disturbed.&0'
        wait 4s
        wecho The fires die down as the voice grows silent.
        wait 3s
        wsend %actor% %priest% says, 'I will do my part.  I count on you to do yours.  If you need a reminder of your &6&b[progress]&0 you can ask me at any time.'
        quest start hell_gate %actor.name%
      else
        wsend %actor% Your lack of experience prevents you from entering the circle of flames!
        wsend %actor% &0 
        wsend %actor% %priest% says, 'Your will is strong but you lack knowledge.  Return after you have grown more.'
      endif
    else
      Wsend %actor% There is no %arg% here.
    endif
    break
  default
    return 0
    halt
done
~
#56404
hell_gate_diabolist_receive~
0 j 100
~
set stage %actor.quest_stage[hell_gate]%
if %stage% == 1
  if %object.vnum% == 3213
    quest advance hell_gate %actor.name%
    wait 1
    say Yes, this seems to match the description.
    mecho %self.name% lifts %object.shortdesc% in offering.
    mjunk dagger
    wait 2s
    mecho The island rumbles and groans as the hellish voice bubbles up out of the lake.
    mecho &1'You have done well, dark ones.  Let us continue.'&0
    wait 2s
    mecho &0&1'To unlock the pathways, you must procure seven keys to seven gates.'&0
    wait 3s
    msend %actor% Your brain burns as a fiery collage of images is seared into your memory!
    msend %actor% One by one you receive a vision of each key:
    msend %actor%  
    msend %actor% &7&bA small, well-crafted key made of wood with the smell of rich sap&0
    msend %actor% &0&7&b  kept at the gate of a tribe's home.&0
    msend %actor%  
    msend %actor% &7&bA key made of light silvery metal which only elves can work&0
    msend %actor% &0&7&b  deep in a frozen valley.&0
    msend %actor%  
    msend %actor% &7&bA large, black key humming with magical energy&0
    msend %actor% &0&7&b  from a twisted cruel city in a huge underground cavern.&0
    msend %actor%  
    msend %actor% &7&bA simple lacquered iron key in the care of a radiant bird&0
    msend %actor% &0&7&b  on an emerald island.&0
    msend %actor%  
    msend %actor% &7&bA rusted but well cared for key held by a winged captain&0
    msend %actor% &0&7&b  on an island of magical beasts.&0
    msend %actor%  
    msend %actor% &7&bA golden plated, wrought-iron key&0
    msend %actor% &0&7&b  held at the gates to a desecrated city.&0
    msend %actor%  
    msend %actor% &7&bOne nearly impossible to see&0
    msend %actor% &0&7&b  guarded by a fiery beast with many heads.&0
    wait 5s
    mecho &1'With these keys you will be able to unlock the gate to the lower realms.&0
    mecho &0&1Go, find them!'&0
    wait 2s
    mecho %self.name% says, 'Let's split up.  See what you can find and
    mecho &0I'll meet you back here.'
  else
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    say I don't think this is what we need.
  endif
elseif %stage% == 2
  if %object.vnum% == 8303 || %object.vnum% == 23709 || %object.vnum% == 49008 || %object.vnum% == 52012 || %object.vnum% == 52013 || %object.vnum% == 53402 || %object.vnum% == 58109
    if %actor.quest_variable[hell_gate:%object.vnum%]%
      return 0
      say You already brought that key.
      mecho %self.name% refuses %object.shortdesc%.
    else
      quest variable hell_gate %actor.name% %object.vnum% 1
      wait 1
      mjunk key
    endif
    set key1 %actor.quest_variable[hell_gate:8303]%
    set key2 %actor.quest_variable[hell_gate:23709]%
    set key3 %actor.quest_variable[hell_gate:49008]%
    set key4 %actor.quest_variable[hell_gate:52012]%
    set key5 %actor.quest_variable[hell_gate:52013]%
    set key6 %actor.quest_variable[hell_gate:53402]%
    set key7 %actor.quest_variable[hell_gate:58109]%
    if %key1% && %key2% && %key3% && %key4% && %key5% && %key6% && %key7%
      mecho The rumbling demonic voice speaks.
      mecho &1'Yes, these keys are ideal.  Several bear the marks of goodness.  They shall be&0
      mecho &0&1perfect to corrupt and despoil!'&0
      mecho 
      mecho The voice bellows a horrific laugh.
      mecho 
      mecho &1'Once their essences have been defiled, they can open the locks on the&0
      mecho &0&1infernal realms. To corrupt the keys, bathe them in the blood of seven&0
      mecho &0&1different mortal children, slain in sacrifice to the demon lords.'&0
      wait 1s
      mecho &1&b'Use the spider-shaped dagger to kill each child.'&0
      mecho  
      mload obj 56407
      give dagger %actor.name%
      wait 1s
      mecho &1'Now go, seek out seven different kinds of children, sacrifice them, and &bdrop&0
      mecho &0&1their blood here.'&0
      quest advance hell_gate %actor.name%
    else
      eval keys %key1% + %key2% + %key3% + %key4% + %key5% + %key6% + %key7%
      if %keys% < 6
        say Let's keep looking for the rest of the keys!
      else
        say Let's look for the last key!
      endif
    endif
  else
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    say This is not one of the seven keys.
  endif
elseif %stage% == 3
  if %actor.quest_variable[hell_gate:new]% /= yes
    if %object.vnum% == 3213
      wait 2
      say Yes, this is a suitable replacement.
      mjunk dagger
      wait 1s
      mload obj 56407
      give dagger %actor.name%
      say Return to hunting your sacrifices.
    else
      return 0
      say This is not an appropriate dagger.
      mecho %self.name% refuses %object.shortdesc%.
    endif
  else
    return 0
    say I don't need anything from you right now.
    mecho %self.name% refuses %object.shortdesc%.
  endif
elseif %stage% == 4
  if %object.vnum% == 56407
    wait 1
    mjunk dagger
    quest advance hell_gate %actor.name%
    wait 2
    mecho %self.name% swirls the dagger through the pool of blood while reciting an incantation.
    mecho %self.name% smears each key with the crimson mixture.
    wait 2s
    mecho The seven keys turn jet black and start to sizzle.
    mecho The Black Lake starts to bubble and boil.
    wait 3s
    mecho The demonic voice says, &1'One more thing then all is done.  Spill the blood of a&0
    mecho &0&1celestial creature here in a final act of desecration.  I have been keeping&0
    mecho &0&1such a pet for just this purpose!'&0
    wait 5s
    mecho In a swirl of smoke and brimstone a radiant winged being materializes within the ring of keys.
    wait 3s
    mecho &1'Kill this worshiper of mine Larathiel, and I shall free you from Garl'lixxil!'&0
    mecho The demonic voice laughs cruelly.
    mecho  
    mecho %self.name% runs and hides!
    wait 10s
    mload mob 56401
    mecho The defiled angel screams with rage and attacks!
    mforce angel kill %actor%
    mteleport %self% 1100
  else
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    say No, the dagger, quickly!
  endif
else
  return 0
  say What is this for?
  mecho %self.name% refuses %object.shortdesc%.
endif
~
#56405
hell_gate_mob_death~
0 f 35
~
  if %actor.quest_stage[hell_gate]% == 3 && %actor.wearing[56407]%
    switch %self.vnum%
      case 12010
        set blood 56400
        break
      case 30054
        set blood 56401
        break
      case 32408
        set blood 56402
        break
      case 48125
        set blood 56403
        break
      case 48126
        set blood 56404
        break
      case 51003
      case 51018
      case 51023
        set blood 56405
        break
      case 55238
        set blood 56406
        break
    done
    if %actor.quest_variable[hell_gate:blood%blood%]% == 0
      quest variable hell_gate %actor% blood%blood% 1
      mload obj %blood%
    endif
  endif
~
#56406
hell_gate_island_drop~
2 h 100
~
if %actor.quest_stage[hell_gate]% == 3
  if %actor.quest_variable[hell_gate:%object.vnum%]%
    return 0
    wecho %get.mob_shortdesc[56400]% says, 'We have already gathered this blood.'
  elseif %object.vnum% == 56400 || %object.vnum% == 56401 || %object.vnum% == 56402 || %object.vnum% == 56403 || %object.vnum% == 56404 || %object.vnum% == 56405 || %object.vnum% == 56406
    quest variable hell_gate %actor.name% %object.vnum% 1
    wait 1
    wecho %object.shortdesc% spills on the ground, gathering in a pool.
    wpurge blood
    set blood1 %actor.quest_variable[hell_gate:56400]%
    set blood2 %actor.quest_variable[hell_gate:56401]%
    set blood3 %actor.quest_variable[hell_gate:56402]%
    set blood4 %actor.quest_variable[hell_gate:56403]%
    set blood5 %actor.quest_variable[hell_gate:56404]%
    set blood6 %actor.quest_variable[hell_gate:56405]%
    set blood7 %actor.quest_variable[hell_gate:56406]%
    if %blood1% && %blood2% && %blood3% && %blood4% && %blood5% && %blood6% && %blood7%
      quest advance hell_gate %actor.name%
      wait 2s
      wecho %get.mob_shortdesc[56400]% says, 'I shall need the dagger to finish this step&_of the unsealing.  Please give it to me.'
    else
      wecho The demonic voice says, &1'This pleases me.  Bring the rest.'&0
    endif
  endif
endif
~
#56407
hell_gate_larathiel_death~
0 f 100
~
return 0
mecho With an anguished cry, Larathiel dies screaming in agony!
set person %actor%
set i %actor.group_size%
if %i%
   set a 1
   unset person
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[hell_gate]% == 5
            quest advance hell_gate %person.name%
         endif
      elseif %person%
        eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[hell_gate]% == 5
   quest advance hell_gate %person.name%
endif
m_run_room_trig 56408
~
#56408
hell_gate_island_set_spell~
2 a 100
~
wecho %get.mob_shortdesc[56400]% comes out of hiding.
wat 1100 wteleport diabolist 56431
if %get.mob_count[56402]% == 0
  wait 1s
  wecho Larathiel's golden celestial blood seeps into the steaming ground.
  wait 3s
  wecho The earth shudders and shifts as it cracks and breaks apart!
  wecho A gout of &1&bfire&0 erupts from a fissure leading into the bowels of the earth!
  wait 2s
  wecho An enormous demonic entity claws its way out of the hole.
  wait 2s
  *
  * Yes, Brolgoroth remains in game until killed or purged via reset
  *
  wload mob 56402
  wecho The demon looks around itself and roars victoriously!
  wecho Brolgoroth says, &1&b'At last, Garl'lixxil is connected to Ethilien again!&0
  wecho &0&1&bNow to add this world my dominion!'&0
endif
set person %self.people%
while %person%
  if %person.quest_stage[hell_gate]% == 6
    wait 2s
    wsend %person% Brolgoroth tells you, &1&b'Thank you, %person.name%, for your unholy service.&0
    wsend %person% &0&1&bAs promised, I shall teach you a great secret.'&0&_
    wsend %person% Your mind is flooded with images of fire and pain as Brolgoroth's mind connects with yours.
    wsend %person% &1&bThe secrets of Hell Gate are seared into your memory!&0
    quest complete hell_gate %person.name%
    if !%actor.quest_variable[hell_trident:helltask4]% && %actor.quest_stage[hell_trident]% == 2
      quest variable hell_trident %actor% helltask4 1
    endif
    wforce diabolist mskillset %person.name% hell gate
  endif
  set person %person.next_in_room%
done
~
#56409
hell_gate_status_checker~
0 d 100
status status? progress progress?~
set stage %actor.quest_stage[hell_gate]%
wait 2
switch %stage%
  case 1
    mecho %self.name% says, 'We are preparing to open a door to
    mecho &0Garl'lixxil and release one of our demon lords.  Find %get.obj_shortdesc[3213]%.'
    break 
  case 2
    set key1 %actor.quest_variable[hell_gate:8303]%
    set key2 %actor.quest_variable[hell_gate:23709]%
    set key3 %actor.quest_variable[hell_gate:49008]%
    set key4 %actor.quest_variable[hell_gate:52012]%
    set key5 %actor.quest_variable[hell_gate:52013]%
    set key6 %actor.quest_variable[hell_gate:53402]%
    set key7 %actor.quest_variable[hell_gate:58109]%
    say You must find seven keys to seven gates.
    mecho  
    if %key1% || %key2% || %key3% || %key4% || %key5% || %key6% || %key7%
      mecho You have already found:
      if %key1%
        mecho &1%get.obj_shortdesc[8303]%&0
      endif
      if %key2%
        mecho &1%get.obj_shortdesc[23709]%&0
      endif
      if %key3%
        mecho &1%get.obj_shortdesc[49008]%&0
      endif
      if %key4%
        mecho &1%get.obj_shortdesc[52012]%&0
      endif
      if %key5%
        mecho &1%get.obj_shortdesc[52013]%&0
      endif
      if %key6%
        mecho &1%get.obj_shortdesc[53402]%&0
      endif
      if %key7%
        mecho &1%get.obj_shortdesc[58109]%&0
      endif
    endif
    mecho   
    mecho You must still find:
    if !%key1%
      mecho &1&bA small, well-crafted key made of wood with the smell of rich sap&0
      mecho &1&bKept at the gate of a tribe's home.&0
      mecho  
    endif
    if !%key6%
      mecho &1&bA key made of light silvery metal which only elves can work&0
      mecho &1&bDeep in a frozen valley.&0
      mecho   
    endif
    if !%key2%
      mecho &1&bA large, black key humming with magical energy&0
      mecho &1&bFrom a twisted cruel city in a huge underground cavern.&0
      mecho  
    endif
    if !%key7%
      mecho &1&bA simple lacquered iron key&0
      mecho &1&bIn the care of a radiant bird on an emerald island.&0
      mecho    
    endif
    if !%key3%
      mecho &1&bA rusted but well cared for key&0
      mecho &1&bHeld by a winged captain on an island of magical beasts.&0
      mecho   
    endif
    if !%key5%
      mecho &1&bA golden plated, wrought-iron key&0
      mecho &1&bheld at the gates to a desacrated city.&0
      mecho   
    endif
    if !%key4%
      mecho &1&bOne nearly impossible to see&0
      mecho &1&bguarded by a fiery beast with many heads.&0
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
    mecho %self.name% says, 'Sacrifice seven different &1&bchildren&0.
    mecho &0&7&b[Drop]&0 their &1&bblood&0 here to defile the keys.'
    mecho 
    if %blood1% || %blood2% || %blood3% || %blood4% || %blood5% || %blood6% || %blood7%
      mecho You have already found:
      if %blood1%
        mecho &1%get.obj_shortdesc[56400]%&0
      endif
      if %blood2%
        mecho &1%get.obj_shortdesc[56401]%&0
      endif
      if %blood3%
        mecho &1%get.obj_shortdesc[56402]%&0
      endif
      if %blood4%
        mecho &1%get.obj_shortdesc[56403]%&0
      endif
      if %blood5%
        mecho &1%get.obj_shortdesc[56404]%&0
      endif
      if %blood6%
        mecho &1%get.obj_shortdesc[56405]%&0
      endif
      if %blood7%
        mecho &1%get.obj_shortdesc[56406]%&0
      endif
    endif
    mecho 
    eval total (7 - (%blood1% + %blood2% + %blood3% + %blood4% + %blood5% + %blood6% + %blood7%))
    if %total% == 1
      mecho Sacrifice the last child!
    else
      mecho %self.name% says, 'Bring the blood of &1%total%&0 more children.'
    endif
    mecho   
    mecho %self.name% says, 'If you need a new dagger, say &1&b"I need a new&0
    mecho &0&1&bdagger"&0.'
    break
  case 4
    say Give the spider-shaped dagger back to me.
    break
  case 5
    say Slay Larathiel and release our demon lord!
    break
  default
    if %actor.has_completed[hell_gate]%
      say You have already learned to open the gates of Hell.
    else
      say You're not working with me.
    endif
done
~
#56410
hell_gate_armor_p2_doppelganger_55585~
0 f 100
~
*
* For Hell Gate
*
  if %actor.quest_stage[hell_gate]% == 3 && %actor.wearing[56407]%
    set blood %random.100%
    if %blood% > 65
      if %actor.quest_variable[hell_gate:blood56404]% == 0
        quest variable hell_gate %actor% blood56404 1
        mload obj 56404
      endif
    endif
  endif
*
* Death trigger for random gem and armor drops - 55585
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup 
*
set bonus %random.100%
set will_drop %random.100%
* 4 pieces of armor per sub_phase in phase_2
set what_armor_drop %random.4%
* 11 classes questing in phase_2
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
      eval gem_vnum %what_gem_drop% + 55637
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 &%will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55637
      eval armor_vnum %what_armor_drop% + 55343
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55347
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55648
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55659
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55351
      mload obj %armor_vnum%
   endif
endif
~
#56411
hell_gate_dagger_replacement~
0 d 0
I need a new dagger~
wait 2
if %actor.quest_stage[hell_gate]% == 3
  quest variable hell_gate %actor% new yes
  grumble
  mecho %self.name% says, 'I don't have another.  You'll have to find a
  mecho &0replacement.'
  wait 2s
  mecho %self.name% says, 'If you can find another, I will verify it
  mecho &0will work.'
endif
~
$~
