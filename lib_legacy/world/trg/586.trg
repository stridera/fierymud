#58600
dragons_health_myorrhed_greeting~
0 g 100
~
wait 2
if %actor.quest_stage[dragons_health]% > 0
  wave %actor.name%
  say Welcome back!
elseif %actor.vnum% == -1
  growl %actor.name%
  say Stay back!  I will not allow you to harm this egg!
endif
~
#58601
dragons_health_myorrhed_speech~
0 d 100
egg egg? Yes yes? No no? Why why? Procured procured? Procure procure? Task task? Value value?~
if %actor.quest_stage[dragons_health]% == 0
  wait 2
  if %speech% /= egg? || %speech% /= egg
    say Yes, this egg here.
    emote gestures to the large bronze egg.
    mecho 
    mecho %self.name% says, 'It's the last egg of the old bronze wyrm Dormynalth.
    mecho &0I am sworn to guard it until it hatches.'
    mecho 
    emote utters a prayer of health and strength for the hatchling.
    wait 5s
    if %actor.level% > 88
      if (%actor.class% /= Cleric || %actor.class% /= Priest)
        mecho %self.name% says, 'However, I could use some assistance from someone as capable
        mecho &0as you are.  If you're willing to help me, I can teach you a prayer to invoke
        mecho &0the health of dragons in exchange.'
        wait 4s
        say Are you interested?
      endif
    endif
  elseif %speech% /= Yes || %speech% /= yes?
    wait 2
    if (%actor.class% /= Cleric || %actor.class% /= Priest) && %actor.class% > 88
      emote smiles with gratitude.
      quest start dragons_health %actor.name%
      wait 1s
      say Good.  Allow me to explain the plan.
      wait 2s
      mecho %self.name% says, 'Dragon eggs like this one need the proper environment to grow
      mecho &0and develop.  Often this is a slow process, taking months or even years for
      mecho &0certain species.  But this process can be sped up by accumulating wealth into
      mecho &0a hoard for the unhatched dragon.'
      wait 6s
      mecho %self.name% says, 'You may be wondering &6&bwhy&0.'
    endif
  endif
elseif %actor.quest_stage[dragons_health]% == 1
  if %speech% /= why || %speech% /= why?
    wait 2
    say Dragons love treasure.  This is well-known.
    wait 2s
    mecho %self.name% says, 'On one level, they just love having it.  To many, all dragons,
    mecho &0good or evil, are personifications of greed.'
    wait 4s
    mecho %self.name% says, 'What many don't know is this love of treasure is more than
    mecho &0just habitual; it is instinctual.'
    wait 4s
    mecho %self.name% says, 'Dragons are compelled to hoard treasure because they draw
    mecho &0power directly from the &6&bvalue&0 of the artifacts they claim dominion over.'
  elseif %speech% /= value || %speech% /= value?
    wait 2
    mecho %self.name% says, 'That value isn't always monetary.  Things imbued with precious
    mecho &0memories, little physical bits of ruins, even a plant that has been carried
    mecho &0from one land to another, all of that contributes to something's value.'
    wait 2s
    say And then of course there's the monetary value.
    laugh
    wait 6s
    say We help the egg mature by increasing the value of its hoard.
    wait 2s
    mecho %self.name%'s eyes twinkle with mischief.
    mecho %self.name% says, 'It would be... valuable if you &6&b"procured"&0 a few items from the
    mecho &0hoards of chromatic dragons.'
  elseif %speech% /= procured || %speech% /= procured? || %speech% /= procure || %speech% /= procure?
    wait 2
    say Yes, that probably means you would have to slay them.
    shrug
    wait 5s
    mecho %self.name% says, 'There is long-standing enmity between chromatic dragons, who
    mecho &0embody the abstract concepts of evil and maliciousness, and the metallic
    mecho &0dragons, who personify aloof goodness and righteousness.'
    wait 5s
    mecho %self.name% says, 'Fewer chromatic dragons means more space for metallic dragons.
    mecho &0To see if you're up to the &6&btask&0 though, let's start with something less
    mecho &0dangerous.'
  elseif %speech% /= task || %speech% /= task?
    wait 2
    mecho %self.name% says, 'A young blue dragon has made a lair near the old tower west of
    mecho &0Anduin.  Bring back the crystal it keeps in its hoard for our hatchling.'
    wait 2s
    mecho %self.name% says, 'You may ask me about your &6&b[progress]&0 if you need a reminder.'
  elseif %speech% /= No || %speech% /= no?
    if (%actor.class% /= Cleric || %actor.class% /= Priest) && %actor.class% > 88 && !%actor.quest_stage[dragons_health]%
      say That's alright.  We shall let nature take its course.
      emote gently pats the egg.
    endif
  endif
endif
~
#58602
dragons_health_myorrhed_receive~
0 j 100
~
set stage %actor.quest_stage[dragons_health]%
if %stage% == 1
  if %object.vnum% == 12509
    wait 2
    emote examines %object.shortdesc%.
    mjunk %object%
    wait 1s
    nod
    msend %actor% %self.name% says, 'I had little doubt you would succeed.'
    quest advance dragons_health %actor.name%
    wait 2s
    msend %actor% %self.name% says, 'Your next target is considerably more powerful so be prepared.'
    wait 2s
    msend %actor% %self.name% says, 'The dragon Tri-Aszp has a cult dedicated to her in the
    msend %actor% &0northern reaches.  She uses them to commit heinous acts and keep the
    msend %actor% &0region in her clutches.'
    wait 5s
    emote places her hands on the egg.
    msend %actor% %self.name% says, 'It would be a boon to both this dragon's future and to all
    msend %actor% &0dragonkind if you could eliminate her.'
    wait 3s
    msend %actor% %self.name% says, 'Dragon scales have incredible monetary and ancestral value.
    msend %actor% &0A scale from Tri-Aszp will remind our hatchling of the legacy it bears.
    msend %actor% &0Bring back one of her scales as a trophy.'
  else
    return 0
    msend %actor% %self.name% says, 'Hmmm, this doesn't seem to have the proper draconic energies.'
    msend %actor% %self.name% refuses %object.shortdesc%.
  endif
elseif %stage% == 2
  if %object.vnum% == 53325
    quest advance dragons_health %actor.name%
    wait 2
    emote runs her eyes over %object.shortdesc%.
    mjunk %object%
    wait 1s
    msend %actor% %self.name% says, 'Excellent work.  With the Cult of the Ice Dragon in shambles,
    msend %actor% &0the world is that much safer.'
    wait 3s
    msend %actor% %self.name% says, 'Tri-Aszp's children may grow to be a threat in the future
    msend %actor% &0which is why this next generation is so important.'
    wait 4s
    msend %actor% %self.name% says, 'That next generation of chromatic dragons is actually what
    msend %actor% &0we should address next.'
    wait 2s
    msend %actor% %self.name% says, 'Sagece of Raymif, the half-demon dragon that rules Templace,
    msend %actor% &0had two spawn sometime after she crawled into our world.  They are entombed in
    msend %actor% &0a crypt far to the north as guardians of a soul-shredding lich.'
    wait 5s
    msend %actor% %self.name% says, 'In their position is a wondrous stone unlike anything else in
    msend %actor% &0existence.  It flickers with an inner green light.'
    wait 2s
    msend %actor% %self.name% says, 'Destroy her spawn, the black dragons Thelriki and Jerajai, and
    msend %actor% &0return with the stone.'
  else
    return 0
    msend %actor% %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, '%object.shortdesc% is not one of Tri-Aszp's scales.'
  endif
elseif %stage% == 3
  if %object.vnum% == 48024
    if %actor.quest_variable[dragons_health:thelriki]% && %actor.quest_variable[dragons_health:jerajai]%
      quest advance dragons_health %actor.name%
      wait 2
      emote looks suspiciously at %object.shortdesc%.
      mjunk %object%
      wait 1s
      msend %actor% %self.name% says, 'Thank you for ending them.  They shall blight the world no more.'
      wait 3s
      msend %actor% %self.name% says, 'Now for the hardest task of all.  When Templace fell and demons
      msend %actor% &0from Garl'lixxil were loosed on Ethilien, the balance of the Dragonwars
      msend %actor% &0was forever changed.'
      wait 4s
      msend %actor% %self.name% says, 'The time has come to right that balance.'
      wait 4s
      msend %actor% %self.name% says, 'Lay siege to Templace and slay the ancient black dragon Sagece
      msend %actor% &0of Raymif.  Bring back her skin and a shield she keeps, plus the two decoys
      msend %actor% &0she keeps in her cursed hoard.'
      wait 2s
      msend %actor% %self.name% says, 'And know, you must be there when the demon dragon falls.'
      wait 4s
      msend %actor% %self.name% says, 'May Bahamut watch over you and grant you fortune!'
    elseif %actor.quest_variable[dragons_health:thelriki]% && !%actor.quest_variable[dragons_health:jerajai]%
      return 0
      msend %actor% %self.name% refuses %object.shortdesc%.
      msend %actor% %self.name% says, 'Jerajai still lives!  You must destroy him first!'
    elseif !%actor.quest_variable[dragons_health:thelriki]% && %actor.quest_variable[dragons_health:jerajai]%
      return 0
      msend %actor% %self.name% refuses %object.shortdesc%.
      msend %actor% %self.name% says, 'Thelriki still lives!  You must destroy her first!'
    else
      return 0
      msend %actor% %self.name% refuses %object.shortdesc%.
      msend %actor% %self.name% says, 'The hell-dragon spawn still live!  You must destroy them first!'
    endif
  else
    return 0
    msend %actor% %self.name% refuses %object.shortdesc%.
    msend %actor% %self.name% says, 'This is not part of their hoard.'
  endif
elseif %stage% == 4
  set sagece %actor.quest_variable[dragons_health:sagece]%
  if %object.vnum% == 52016 || %object.vnum% == 52017 || %object.vnum% == 52022 || %object.vnum% == 52023
    if !%sagece%
      return 0
      msend %actor% %self.name% refuses %object.shortdesc%.
      msend %actor% %self.name% says, 'You must slay Sagece first!'
    else
      if %actor.quest_variable[dragons_health:%object.vnum%]%
        return 0
        msend %actor% %self.name% refuses %object.shortdesc%.
        msend %actor% %self.name% says, 'You have already given this to me.'
        halt
      else
        quest variable dragons_health %actor.name% %object.vnum% 1
        wait 2
        set emote %random.4%
        switch %emote%
          case 1
            emote looks at %object.shortdesc% with disdain.
            break
          case 2
            emote regards %object.shortdesc% with caution.
            break 
          case 3
            emote grimaces at %object.shortdesc%.
            break
          default
            emote sneers at %object.shortdesc%.
        done
        mjunk %object%
      endif
    endif   
    set item1 %actor.quest_variable[dragons_health:52016]%
    set item2 %actor.quest_variable[dragons_health:52017]%
    set item3 %actor.quest_variable[dragons_health:52022]%
    set item4 %actor.quest_variable[dragons_health:52023]%
    wait 1s
    if %item1% && %item2% && %item3% && %item4%
      quest advance dragons_health %actor.name%
      msend %actor% %self.name% says, 'I can't believe she's finally gone.'
      wait 2s
      emote places the remnants of the other dragons' hoards around the egg.'
      wait 2
      msend %actor% The egg shifts and rocks slightly with signs of life!
      way 3s
      msend %actor% %self.name% says, 'The last thing the egg needs is money.  Lots of it.'
      msend %actor% 
      msend %actor% %self.name% says, 'Bring anything of value you can find. You can even offer coin.
      msend %actor% &0About 10,000 platinum worth of treasure should be satisfactory.  That's about
      msend %actor% &010,000,000 copper, in case it needed to be said.'
      wait 4s
      msend %actor% %self.name% says, 'Some very powerful objects are so unique they are priceless.
      msend %actor% &0Bring them to me and I shall see if they are worth including as well.'
      wait 3s
      msend %actor% %self.name% says, 'And remember, just because someone sells something for a
      msend %actor% &0certain price, doesn't mean it's actually worth that much!'
    else
      msend %actor% %self.name% says, 'Good.  Do you have the remaining artifacts?'
    endif
  else
    return 0
    msend %actor% %self.name% refuses %object.shortdesc%.
    msend %actor% %self.name% says, 'This is not significant enough to Sagece.'
    halt
  endif
elseif %stage% == 5
  if %object.cost% == 0 && %object.level% < 60
    return 0
    wait 2
    emote examines %object.shortdesc% closely.
    wait 1s
    msend %actor% %self.name% says, 'Unfortunately there isn't enough inherent value to &3&b%object.shortdesc% to be part of the hatchling's hoard.'
    halt
  else
    wait 2
    emote examines %object.shortdesc% closely.
    if (%object.level% >= 60) && (%object.cost% == 0)
      eval price (%object.level% * 1000)
      msend %actor% %self.name% says, 'Ah, %object.shortdesc%.  Although it has no inherent price, the mystic value of it is: &b&3%price%&0 copper.'
    elseif (%object.level% >= 60 && %object.cost% < (%object.level% * 1000))
      eval price (%object.level% * 1000)
      msend %actor% %self.name% says, 'Ah, %object.shortdesc%.  Even though it has some material value, the mystic value of it is: &3&b%price%&0 copper.'
    elseif (%object.level% < 60 && %object.cost% > 0) || (%object.level% >= 60 && %object.cost% >= (%object.level% * 1000))
      eval price %object.cost%
      msend %actor% %self.name% says, 'Ah, %object.shortdesc%.  It has a value of: &3&b%object.cost%&0 copper.'
    endif
    msend %actor% &0  
    msend %actor% %self.name% says, 'It shall be included in the offerings.'
    emote places %object.shortdesc% next to the egg.
    mjunk %object%
    wait 2
  endif
  set hoard %actor.quest_variable[dragons_health:hoard]%
  eval wealth (%hoard% + %price%)
  quest variable dragons_health %actor.name% hoard %wealth%
  set value %actor.quest_variable[dragons_health:hoard]%
  if %value% >= 10000000
    quest advance dragons_health %actor.name%
    m_run_room_trig 58604
  else 
    eval total (10000000 - %value%)
    eval plat (%total% / 1000)
    eval gold (%total% / 100) - (%plat% * 10)
    eval silv (%total% / 10) - (%plat% * 100) - (%gold% * 10)
    eval copp %total%  - (%plat% * 1000) - (%gold% * 100) - (%silv% * 10)
   *now the price can be reported 
    msend %actor% %self.name% says, 'We need %plat% platinum, %gold% gold, %silv% silver, %copp% copper more in treasure and coins.'
  endif
else
  return 0
  msend %actor% %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, 'I'm not looking for anything right now.'
endif
~
#58603
dragons_health_myorrhed_bribe~
0 m 1
~
if %actor.quest_stage[dragons_health]% == 5
  wait 2
  mecho %self.name% says, 'Ah, coin itself!  I count:'
  if %platinum%
    mecho %platinum% platinum
  endif
  if %gold%
    mecho %gold% gold
  endif
  if %silver%
    mecho %silver% silver
  endif
  if %copper%
    mecho %copper% copper
  endif
  wait 2
  say This shall be included in the offerings.
  emote places the money next to the egg.
  set hoard %actor.quest_variable[dragons_health:hoard]%
  eval wealth %hoard% + ((%platinum% * 1000) + (%gold% * 100) + (%silver% * 10) + %copper%)
  quest variable dragons_health %actor.name% hoard %wealth%
  set value %actor.quest_variable[dragons_health:hoard]%
  if %value% >= 10000000
    quest advance dragons_health %actor.name%
    m_run_room_trig 58604
  else 
    eval total (10000000 - %value%)
    eval plat (%total% / 1000)
    eval gold ((%total% / 100) - (%plat% * 10))
    eval silv ((%total% / 10) - (%plat% * 100) - (%gold% * 10))
    eval copp (%total%  - (%plat% * 1000) - (%gold% * 100) - (%silv% * 10))
   *now the price can be reported 
    mecho %self.name% says, 'We need %plat% platinum, %gold% gold, %silv% silver, %copp% copper
    mecho &0more in treasure and coins.'
  endif
else
  say I appreciate the gesture, but I am in no need of money.
  give %platinum% platinum %gold% gold %silver% silver %copper% copper %actor.name% 
end
~
#58604
dragons_health_room_hatch~
2 a 100
~
wait 2s
wecho As %get.mob_shortdesc[58610]% turns to walk away from the egg, a crack begins to form...
wait 3s
wecho The crack continues to grow, branching into hundreds of smaller cracks!
wait 6s
wecho A talon suddenly breaks through the egg shell!
wait 3s
wecho Slowly, the head of a bronze dragon pushes through the shell.
wait 2s
wecho The dragon egg continues to split into pieces.
wait 2s
wecho A small, bronze dragon emerges from the broken egg shell.
wecho The tiny dragon looks at %get.mob_shortdesc[58610]%.
wpurge dragon-egg
wait 3s
wecho The bronze hatchling says, 'Mistress, thank you for protecting me.
wecho &0You have done a great service to our kind.'
wait 2s
wecho The bronze dragon looks around itself.
wecho In a rush of wind, the dragon beats its wings, launching itself into the air.
Wait 6s
wecho %get.mob_shortdesc[58610]% watches the hatchling soar through the sky and skim across the ocean's surface.
wait 2s
wecho %get.mob_shortdesc[58610]% beams with joy!
wait 3s
wecho %get.mob_shortdesc[58610]% says, 'I will be forever grateful to you for helping dragonkind.
wecho &0To show my gratitude, I gift you the Song of the Dragons.'
wait 3s
wecho %get.mob_shortdesc[58610]% sings an ancient Draconic prayer.
wait 1s
set person %self.people%
while %person%
   if %person.quest_stage[dragons_health]% == 6
      wforce dragonborn mskillset %person% dragons health
      wsend %person% &3&bFrom the Song of the Dragons you have learned Dragons Health!&0
      quest complete dragons_health %person.name%
   endif
   set person %person.next_in_room%
done
~
#58605
dragons_health_myorrhed_status_checker~
0 d 100
progress progress? status status?~
set stage %actor.quest_stage[dragons_health]%
wait 2
if %actor.has_completed[dragons_health]%
  mecho %self.name% says, 'You have already kept vigil with me.
  mecho &0May the Song of the Dragons give you strength.'
  halt
else
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
      mecho %self.name% says, 'The new hatchling's hoard needs enriching.
      mecho &0%plat% platinum, %gold% gold, %silv% silver, %copp% copper
      mecho &0more in treasure and coins ought to do it.'
      halt
      break
    default
      say Progress of what?  You're not keeping guard with me...
  done
  if %stage% == 4
    mecho %self.name% says, 'You are undertaking your hardest task:'
  elseif %stage% != 5
    mecho %self.name% says, 'You are trying to:'
  endif
  mecho - kill %dragon% and bring me %treasure%.
  if %stage% == 3
    set thelriki %actor.quest_variable[dragons_health:thelriki]%
    set jerajai %actor.quest_variable[dragons_health:jerajai]%
    mecho 
    if %thelriki% || %jerajai%
      mecho You have slain:
      if %thelriki%
        mecho - Thelriki
      endif
      if %jerajai%
        mecho - Jerajai
      endif
    endif
    mecho 
    mecho You must still:
    if !%thelriki%
      mecho - kill Thelriki
    endif
    if !%actor.quest_variable[dragons_health:jerajai]%
      mecho - kill Jerajai
    endif
    mecho - bring the heartstone
  elseif %stage% == 4
    mecho 
    set item1 %actor.quest_variable[dragons_health:52016]%
    set item2 %actor.quest_variable[dragons_health:52017]%
    set item3 %actor.quest_variable[dragons_health:52022]%
    set item4 %actor.quest_variable[dragons_health:52023]%
    set sagece %actor.quest_variable[dragons_health:sagece]%
    if %item1% || %item2% || %item3% || %item4% || %sagece%
      mecho You have already:
      if %sagece%
        mecho - slain Sagece of Raymif
      endif
      if %item1%
        mecho - brought Sagece's skin
      endif
      if %item2%
      mecho - brought Sagece's shield
      endif
      if %item3%
        mecho - brought the skin from Sagece's hoard
      endif
      if %item4%
        mecho - brought the shield from Sagece's hoard
      endif
    endif
    mecho 
    mecho You must still:
    if !%sagece%
      mecho - kill Sagece of Raymif
    endif
    if !%item1%
      mecho - bring Sagece's skin
    endif
    if !%item2%
      mecho - bring Sagece's shield
    endif
    if !%item3%
      mecho - find the skin in Sagece's hoard
    endif
    if !%item4%
      mecho - find the shield in Sagece's hoard
    endif
  endif
endif
~
#58606
dragons_health_thelriki_jerajai_death~
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
         if %person.quest_stage[dragons_health]% == 3
            quest variable dragons_health %person.name% %self.name% 1
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[dragons_health]% == 3
   quest variable dragons_health %person.name% %self.name% 1
endif
~
#58607
sagece_dragons_health_death~
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
         if %person.quest_stage[dragons_health]% == 4
            quest variable dragons_health %person.name% sagece 1
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[dragons_health]% == 4
   quest variable dragons_health %person.name% sagece 1
endif
~
$~
