#6200
Enter Illusionist Guild Anduin~
2 c 100
south~
return 1
wdoor 6204 south room 6233
wforce %actor% south
wdoor 6204 south purge
~
#6201
necromancer_quest_spell_hints_and_shift_corpse_start~
0 d 100
shift shift? corpse corpse? degeneration degeneration?~
wait 2
switch %speech%
  case shift
  case shift?
  case corpse
  case corpse?
  case shift corpse
  case shift corpse?
    if %self.class% /= Necromancer
      if %actor.class% /= Necromancer && %actor.quest_stage[shift_corpse]% == 0 
        say So you want to learn our most powerful secrets, eh?
        peer %actor%
        wait 1s
        if %actor.level% < 97
          mecho %self.name% says, 'Unfortunately you are not prepared to handle such magic.
          mecho &0There are easier ways to kill yourself.'
        else
          nod
          mecho %self.name% says, 'I will teach you the words to the spell.  But the words
          mecho &0alone are meaningless.  The true power to fuel Shift Corpse cannot be
          mecho &0taught - it must be taken.'
          wait 2s
          say You must steal the power of a god.
          quest start shift_corpse %actor.name%
        endif
      else
        mechoaround %actor% %self.name% sneers at %actor.name%.
        msend %actor% %self.name% sneers at you.
        say This magic is not for you!
      endif
    else
      say That spell is far outside the realm of our Guild.  
    endif
    break
  case degeneration
  case degeneration?
    if %self.class% /= Necromancer
      mecho %self.name% says, 'Controlling negative energy to bolster the dead or
      mecho &0degenerate the living is theoretically possible.  Someone is working to
      mecho &0develop a &9&bDegeneration&0 spell for use in the ongoing battle against
      mecho &0the Eldorian Guard.'
    else
      mecho %self.name% says, 'Only a powerful member of the Necromancer Guild
      mecho &0might know how to cast such a spell.'
    endif
    break
done
~
#6202
shift_corpse_guildmaster_speech2~
0 d 100
taken? god? who?~
if %actor.quest_stage[shift_corpse]% == 1
  wait 2
  mecho %self.name% says, 'Not just any god will do, either.  It cannot be one who is
  mecho &0imprisoned in mortal flesh, like Kannon in Odaishyozen, no.'
  shake
  mecho %self.name% says, 'It must be one unrestrained, at their full potential.
  mecho &0Fortunately there is a deity who has lost the support of the greater pantheon
  mecho &0leaving him vulnerable.'
  wait 4s
  mecho %self.name% says, 'Lokari, God of the Moonless Night, in his outer planar
  mecho &0fortress is already wanted by the other gods for interfering in the world of
  mecho &0mortal men.'
  wait 3s
  emote opens a pitch black box.
  wait 4
  emote takes a glowing black crystal from the box.
  wait 1s
  say Take this.
  mload obj 6228
  give glowing-black-crystal %actor%
  wait 2s
  mecho %self.name% says, 'Give it to Lokari and then banish him from the realm.  The
  mecho &0crystal will steal part of his divine spark and give you the energy you need,'
  wait 3s
  say This will not be easy.
  wait 1s
  if !%actor.has_completed[doom_entrance]%
    mecho %self.name% says, 'You must consult with the three Oracles at the ancient
    mecho &0pyramid where Severan went to reclaim his stolen bride.  They will give you
    mecho &0further instructions on gaining access to Lokari's keep.'
  else
    mecho %self.name% says, 'Find the Horn of the Hunt and the lost relics of Rhalean
    mecho &0and Timun, access Lokari's keep, fight past the horrors that await, and take
    mecho &0Lokari's power!'
  endif
  wait 3s
  say May Death be ever at your back.
  bow %actor.name%
endif
~
#6203
shift_corpse_lokari_receive~
0 j 100
~
if %object.vnum% == 6228 && %actor.quest_stage[shift_corpse]% == 1
  return 0
  quest advance shift_corpse %actor.name%
  mecho %get.obj_shortdesc[6228]% flares with &b&9darkness visible!&0
  wait 1s
  mecho &b&9%self.name%&0 &1ROARS&0 &b&9with divine fury!&0
  say You will never destroy me!!
  kill %actor.name%
endif
~
#6204
pyromancer_quest_spell_hints_and_supernova_start~
0 d 100
supernova supernova? super super? nova nova? meteorswarm meteorswarm? meteor meteor? swarm swarm?~
wait 2
switch %speech%
  case super
  case super?
  case nova
  case nova?
  case supernova
  case supernova?
    if %self.class% /= pyromancer
      if %actor.class% /= Pyromancer
        if %actor.level% > 88
          if %actor.quest_stage[supernova]% == 0
            quest start supernova %actor.name%
            mecho %self.name% says, 'Supernova is really only taught by one person:
            mecho &0Phayla, daughter of the Sun.'
            wait 1s
            mecho %self.name% says, 'Phayla is one of the most powerful sorceresses in
            mecho &0existence.  Beings from all across the universe have sought her out for just a
            mecho &0chance to study under her.  Most fail to find her.  She has agreed to teach
            mecho &0even fewer.'
            wait 5s
            mecho %self.name% says, 'She resides in a demiplane of her own creation.
            mecho &0It's constantly shifting and changing, so it's impossible for anyone but Phayla
            mecho &0to know where it is or how to access it without help.'
            wait 2s
            mecho %self.name% says, 'However, she likes to leave clues to accessing it
            mecho &0scattered throughout the world.'
            wait 4s
            mecho %self.name% says, 'On their own, the clues are illegible and
            mecho &0unintelligible.  The keys to reading them are lamps Phayla gives to her most
            mecho &0ardent and formidable followers.'
            wait 4s
            say If you find one I may be able to help you more.
            wait 2s
            mecho %self.name% says, 'You can check in with me about your &7&b[progress]&0
            mecho &0toward learning Supernova at any time.'
            mecho   
            mecho %self.name% says, 'If you want to know about your armor quests, ask
            mecho &0me about your &7&0[status]&0 instead.'
          endif
        else
          mecho %self.name% says, 'You're not ready to be asking about such an
          mecho &0awesome spell.  We can do a follow-up visit after you gain some more
          mecho &0experience.'
        endif
      else
        say Supernova is beyond your discipline.
      endif
    else
      say Supernova is far outside our discipline.
    endif
    break
  case meteor
  case meteor?
  case swarm
  case swarm?
  case meteorswarm
  case meteorswarm?
    if (%self.class% /= sorcerer || %self.class% /= pyromancer)
      mecho %self.name% says, 'Crazy old McCabe the Elementalist has developed
      mecho &0such a spell. If you can find him, you might be able to persuade him to teach
      mecho &0you &1&bMeteorswarm&0.'
      mecho   
      mecho &0He likes the devastating effects of volcanoes.'
    elseif %self.class% /= diabolist
      mecho %self.name% says, 'We too call flaming rocks from the sky, but through
      mecho &0different powers.'
    else
      mecho %self.name% says, 'I'm afraid I can't help you with that particular
      mecho &0spell.'
    endif
    break
  default
    return 0
done
~
#6205
supernova_sunchild_death~
0 f 100
~
set i %actor.group_size%
if %i%
   set a 1
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[supernova]% == 1
            quest advance supernova %person.name%
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %actor.quest_stage[supernova]% == 1
   quest advance supernova %actor.name%
endif
~
#6206
supernova_guildmaster_speech2~
0 d 100
phalya lamp help? lamp? phayla? help information information? clue clue?~
if %actor.quest_stage[supernova]% == 2 && (%actor.inventory[48917]% || %actor.wearing[48917]%)
  quest advance supernova %actor.name%
  set rnd1 %random.3%
  switch %rnd1%
    case 1
      set step3 4318
      break
    case 2
      set step3 10316
      break
    case 3
      set step3 58062
      break
    default
      return 0
  done
  quest variable supernova %actor.name% step3 %step3%
  set rnd2 %random.3%
  switch %rnd2%
    case 1
      set step4 18577
      break
    case 2
      set step4 17277
      break
    case 3
      set step4 8561
      break
    default
      return 0
  done
  quest variable supernova %actor.name% step4 %step4%
  set rnd3 %random.3%
  switch %rnd3%
    case 1
      set step5 53219
      break
    case 2
      set step5 47343
      break
    case 3
      set step5 16278
      break
    default
      return 0
  done
  quest variable supernova %actor.name% step5 %step5%
  set rnd4 %random.3%
  switch %rnd4%
    case 1
      set step6 58657
      break
    case 2
      set step6 35119
      break
    case 3
      set step6 55422
      break
    default
      return 0
  done
  quest variable supernova %actor.name% step6 %step6%
  set step7 %random.3%
  quest variable supernova %actor.name% step7 %step7%
  say Ah I see you found one of Phayla's lamps!
  wait 3s
  mecho %self.name% says, 'Phayla likes to visit the material plane to engage
  mecho &0in her favorite leisure activities.'
  wait 2s
  set clue %actor.quest_variable[supernova:step3]%
    switch %clue%
      case 4318
        mecho %self.name% says, 'Recently, she was spotted in Anduin, taking in a
        mecho &0show from the best seat in the house.'
        break
      case 10316
        mecho %self.name% says, 'I understand she frequents the hottest spring at 
        mecho &0the popular resort up north.'
        break
      case 58062
        mecho %self.name% says, 'She occasionally visits a small remote island
        mecho &0theatre, where she enjoys meditating in their reflecting room.'
        break
      default
        return 0
  done
  wait 2s
  mecho %self.name% says, 'You may be able to find a clue to her whereabouts
  mecho &0there.'
elseif %actor.quest_stage[supernova]% == 1
  mecho %self.name% says, 'If you return here with one of Phayla's lamps, I
  mecho &0may be able to give you some more insight as to her whereabouts.'
endif
~
#6207
supernova_clue_rooms~
2 i 100
~
set person %self.people%
set stage %person.quest_stage[supernova]%
while %person%
  if (%stage% < 6) && (%person.room% == %person.quest_variable[supernova:step%stage%]%) && (%person.inventory[48917]% || %person.wearing[48917]%)
    if %stage% == 3  
      wload obj 6229
    elseif %stage% == 4
      wload obj 6232
    elseif %stage% == 5
      wload obj 6233
    endif
    wecho &5%get.obj_shortdesc[48917]% begins emitting an eerie purple light!&0
    wechoaround %actor% %actor.name% has found a clue to Phayla's whereabouts!
    wsend %actor% You have found a clue to Phayla's whereabouts!
    quest advance supernova %person.name%
  elseif (%stage% == 6) && (%person.room% == %person.quest_variable[supernova:step6]%) && (%person.inventory[48917]% || %person.wearing[48917]%)
    wload obj 6230
    wecho &5Eerie purple light from %get.obj_shortdesc[48917]% reveals a gateway to another dimension!&0
    quest advance supernova %person.name%
  endif
  set person %person.next_in_room%
done
~
#6208
supernova clue 1~
1 m 100
~
return 0
set stage %actor.quest_stage[supernova]%
*
* Is the person on the quest?
*
if %stage% > 0
  *
  * Do they have the lamp?
  *
  if (%actor.inventory[48917]% || %actor.wearing[48917]%)
    if %actor.quest_stage[supernova]% >= 4
      switch %actor.quest_variable[supernova:step4]%
        case 18577
          set clue2 I continue my journey where the sun rises amidst a sea of swirling worlds.
          * The Abbey, the rising sun room
          break
        case 17277
          set clue2 Atop a tower I visit a master who waits to give his final examination.
          * Citadel of Testing
          break
        case 8561
          set clue2 I study in a secret place above a hall of misery beyond a gallery of horrors.
          * Cathedral of Betrayal near Norisent
      done
      osend %actor% Learning is a life-long process.
      osend %actor% %clue2%
    else
      set read no
    endif
    * ends the stage check
  else
    set read no
  endif
  * ends the lamp check
else
  set read no
endif
* ends the quest check
if %read% == no
  osend %actor% The writings are just a bunch of indecipherable squiggles and lines.
endif
~
#6209
supernova_phayla_greet~
0 g 100
~
wait 1s
if %actor.quest_stage[supernova]% == 7
  mecho %self.name% says, 'Ah, so you found me %actor.name%.  I was wondering if you would
  mecho &0make it.'
  wait 2s
  emote settles back in a chair.
  wait 1s
  mecho %self.name% says, 'So, why are you here?  Normal people don't just barge into
  mecho &0someone's home without good reason.  Come on, out with it!'
else
  eye %actor.name%
  wait 2s
  blink
  wait 1s
  say And who might you be?  You're not at all who I was expecting...
endif
~
#6210
supernova_phayla_speech1~
0 d 100
supernova supernova? super super? nova nova? teach teach?~
wait 2
if %actor.quest_stage[supernova]% == 7
  roll
  say Of course that's what you want.
  wait 1s
  mecho %self.name% says, 'No one ever comes just to ask, "How are you?  How's your day
  mecho &0going?"  Oh no, they always want something.  No sense of class or decorum.'
  sigh
  wait 2s
  say Alright, I'll teach you.
  wait 1s
  say But payment upfront.  Give me that miniature sun you found.
  wait 2s
  msend %actor% %self.name% shoots you with a harsh, critical look.
  mechoaround %actor% %self.name% shoots %actor.name% a harsh, critical look.
  wait 1s
  say Hand over the lamp too.  Don't think you deserve to have that...
endif
~
#6211
supernova_phayla_speech2~
0 d 100
how~
if %speech% /= how are you? || %speech% /= How's your day going?
  wait 2
  say Wonderful, thank you for asking!
  mload obj 23887
  give cookie %actor.name%
endif
~
#6212
supernova_phayla_receive~
0 j 100
~
if %object.vnum% == 51073 || %object.vnum% == 48917
  if %actor.quest_variable[supernova:%object.vnum%]% == 1
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    say You've already given me this.
  else
    quest variable supernova %actor.name% %object.vnum% 1
    wait 2
    nod
    mjunk %object%
    if %actor.quest_variable[supernova:51073]% && %actor.quest_variable[supernova:48917]%
      say Alright, get comfortable.  This may take a while.
      wait 3s
      mecho %self.name% demonstrates some of the basics.
      msend %actor% You take notes and try to follow along.
      wait 5s
      say No, the diagrams need to look like this...
      mechoaround %actor% %self.name% makes changes to %actor.name%'s notes.
      msend %actor% %self.name% makes changes to your notes.
      wait 5s
      say Yes, that's good.  Move your hands like this.
      mecho %self.name% waves her hands about.
      wait 5s
      say Yes, that's much closer.
      msend %actor% You feel you're starting to get the hang of it!
      mechoaround %actor% It seems %actor.name% is starting to get the hang of it!
      wait 9s
      msend %actor% You have a breakthrough!
      mechoaround %actor% %actor.name% has a breakthrough!
      wait 1s
      say That's it!!  You've got it!
      mechoaround %actor% %actor.name% radiates with inner arcane fire!
      msend %actor% &1&bYou have mastered Supernova!&0
      mskillset %actor.name% supernova
      quest complete supernova %actor.name%
    else
      say And the other?
    endif
  endif
else
  return 0
  say This isn't acceptable payment.
  mecho %self.name% refuses %object.shortdesc%.
endif
~
#6213
supernova_gateway_enter_command~
1 c 4
enter~
switch %cmd%
  case e
    return 0
    halt
done
switch %arg%
  case r
  case ri
  case rin
  case ring
  case g
  case ga
  case gat
  case gate
  case gatew
  case gatewa
  case gateway
    if %actor.inventory[51073]% || %actor.wearing[51073]%
      osend %actor% The gateway draws power from %get.obj_shortdesc[51073]% and activates!
      return 0
      wait 2s
      oecho The gateway folds in on itself and collapses!
      opurge %self%
    else
      return 1
      osend %actor% The gateway is inactive.
    endif
    break
  default
    return 0
done
~
#6214
supernova_status_checker~
0 d 100
progress progress?~
set stage %actor.quest_stage[supernova]%
wait 2
if %stage% == 1
  say You are trying to find one of Phayla's lamps.
elseif %stage% == 2
  say Have you found one of Phayla's lamps?
elseif %stage% == 3
  mecho %self.name% says, 'Phayla likes to visit the material plane to engage
  mecho &0in her favorite leisure activities.'
  wait 2s
  set clue %actor.quest_variable[supernova:step3]%
    switch %clue%
      case 4318
        mecho %self.name% says, 'Recently, she was spotted in Anduin, taking in a
        mecho &0show from the best seat in the house.'
        break
      case 10316
        mecho %self.name% says, 'I understand she frequents the hottest spring at 
        mecho &0the popular resort up north.'
        break
      case 58062
        mecho %self.name% says, 'She occasionally visits a small remote island
        mecho &0theatre, where she enjoys meditating in their reflecting room.'
        break
      default
        return 0
  done
  wait 2s
  say You may be able to find a clue to her whereabouts there.
elseif %stage% >= 4
  mecho %self.name% says, 'Consult the clues Phayla left behind!
  mecho &0Be sure to &7&blook&0 at them by the light of your lamp!'
endif
~
#6215
Shift_corpse_crystal_replacement~
0 d 0
I need a new crystal~
wait 2
if %actor.quest_stage[shift_corpse]%
  say I can assist.
  wait 1s
  emote opens a pitch black box.
  wait 4
  emote takes a glowing black crystal from the box.
  wait 1s
  say Take this.
  mload obj 6228
  give glowing-black-crystal %actor%
  wait 1s
  say Be more careful this time.
endif
~
#6216
supernova clue 2~
1 m 100
~
return 0
set stage %actor.quest_stage[supernova]%
*
* Is the person on the quest?
*
if %stage% > 0
  *
  * Do they have the lamp?
  *
  if (%actor.inventory[48917]% || %actor.wearing[48917]%)
    if %actor.quest_stage[supernova]% >= 5
      switch %actor.quest_variable[supernova:step5]%
        case 53219
          set clue3 Where DID the lizard men get that throne from?  I'll see if I can find out.
          * Lizard King's throne room, Sunken
          break
        case 47343
          set clue3 They often wonder what would happen if bones could talk.  I'll ask one who can make that happen!
          * Kryzanthor, Graveyard
          break
        case 16278
          set clue3 Waves of sand hold the remains of a child of the Sun God.  Supposedly.  I'll have to see for myself.
          * Imanhotep, Pyramid
      done
      * end clue3 switch
      osend %actor% History is so fascinating!
      osend %actor% %clue3%
    else
      set read no
    endif
    * ends the stage check
  else
    set read no
  endif
  * ends the lamp check
else
  set read no
endif
* ends the quest check
if %read% == no
  osend %actor% The writings are just a bunch of indecipherable squiggles and lines.
endif
~
#6217
supernova clue 3~
1 m 100
~
return 0
set stage %actor.quest_stage[supernova]%
*
* Is the person on the quest?
*
if %stage% > 0
  *
  * Do they have the lamp?
  *
  if (%actor.inventory[48917]% || %actor.wearing[48917]%)
    if %actor.quest_stage[supernova]% >= 6
      set step7 %actor.quest_variable[supernova:step7]%
      switch %actor.quest_variable[supernova:step6]%
        case 58657
          * A Hummock of Grass in the Beachhead
          if %step7% == 1
            set clue4 s pfqzqgc wq kecwk qy xug fwinlugev
          elseif %step7% == 2
            set clue4 d hlwzsuc rf xbnwk aq tyo oisukhvkq
          elseif %step7% == 3
            set clue4 s oidfgjy fy yyojl au hyx tlotazlou
          endif
          break
        case 35119
          * A Pile of Stones in the Brush Lands
          if %step7% == 1
            set clue4 s xtpr qj kbzrru mf bsi otykp weafw
          elseif %step7% == 2
            set clue4 d pzvr sx kwoeof mf lke sbhwz ddnuc
          elseif %step7% == 3
            set clue4 s wwcx gm gkhflg zg los skmzv ctfkg
          endif
          break
        case 55422
          * The Trail Overlooking the Falls in the dark mountains
          if %step7% == 1
            set clue4 lpp xecmd wgiensgstrt vlw nlpyu mf bsi qcvc uzyaveavd
          elseif %step7% == 2
            set clue4 whv deead ovvbysgclnx dui xsolj sa xzw gaiu zsmfwazxf
          elseif %step7% == 3
            set clue4 los kkspz fowyzfhcpbx mzl tredz we mzl rrkc tclglhwel
          endif
      done
      * end clue4 switch
      switch %step7%
        case 1
          set clue5 What disappears as soon as you say its name? 
          * Answer: Silence
          break
        case 2
          set clue5 The more there is, the less you see. What am I?
          * Answer: Darkness
          break
        case 3
          set clue5 What word becomes shorter when you add two to it?
          * Answer: Short
      done
      * end clue 5 switch
      osend %actor% I know you're following me.  Answer this:
      osend %actor% "%clue5%"
      osend %actor% With the answer you can find the gate to my home here:
      osend %actor% %clue4%
      osend %actor% &0 
      osend %actor% You will need additional solar energy to power the gate.
      osend %actor% Hidden in the dimensional folds around Nordus is an appropriate source.
    else
      set read no
    endif
    * ends the stage check
  else
    set read no
  endif
  * ends the lamp check
else
  set read no
endif
* ends the quest check
if %read% == no
  osend %actor% The writings are just a bunch of indecipherable squiggles and lines.
endif
~
#6292
Gem Exchange greet~
0 h 100
~
wait 2
if %actor.vnum% == -1
set item %actor.quest_variable[gem_exchange:gem_vnum]%
if %actor.quest_stage[gem_exchange]% == 1
  say Welcome back!
  bow %actor%
  wait 2s
  if %item% == 0
    msend %actor% %self.name% says, 'Tell me, what gemstone are you interested in trading for
    msend %actor% &0today?'
  else
    msend %actor% %self.name% says, 'Are you still looking for %get.obj_shortdesc[%item%]%?'
  endif
else
  say Greetings!  Welcome to the Soltan Gem Exchange!
  wait 1s
  mecho %self.name% says, 'We specialize in exchanging your less useful gemstones
  mecho &0for ones you might need.  Our reserves are fully stocked.  All you have to do
  mecho &0is tell us what kind of gem you're looking for.'
  wait 4s
  say So what can I help you find today?
endif
endif
~
#6293
Gem Exchange set type~
0 d 100
amber agate amethyst aquamarine beryl bloodstone blood carnelian citrine diamond emerald garnet hematite jade jasper labradorite lapis lazuli lapis-lazuli malachite moonstone moon onyx opal pearl peridot sapphire ruby topaz tourmaline turquoise~
wait 2
if !%actor.quest_stage[gem_exchange]%
  quest start gem_exchange %actor%
endif
if %speech% /= amber
  if %speech% /= crushed
    set gem_vnum 55575
  elseif %speech% /= dust
    set gem_vnum 55567
  elseif %speech% /= uncut
    set gem_vnum 55602
  elseif %speech% /= flawed
    set gem_vnum 55624
  elseif %speech% /= shard
    set gem_vnum 55583
  elseif %speech% /= enchanted
    set gem_vnum 55723
  elseif %speech% /= radiant
    set gem_vnum 55701
  elseif %speech% /= perfect
    set gem_vnum 55679
  else
    set gem_vnum 55646
  endif
elseif %speech% /= agate
  if %speech% /= uncut
    set gem_vnum 55599
  elseif %speech% /= flawed
    set gem_vnum 55621
  elseif %speech% /= perfect
    set gem_vnum 55676
  elseif %speech% /= radiant
    set gem_vnum 55698
  elseif %speech% /= enchanted
    set gem_vnum 55720
  else
    set gem_vnum 55643
  endif
elseif %speech% /= amethyst
  if %speech% /= crushed
    set gem_vnum 55574
  elseif %speech% /= dust
    set gem_vnum 55566
  elseif %speech% /= uncut
    set gem_vnum 55601
  elseif %speech% /= flawed
    set gem_vnum 55623
  elseif %speech% /= shard
    set gem_vnum 55582
  elseif %speech% /= enchanted
    set gem_vnum 55722
  elseif %speech% /= radiant
    set gem_vnum 55700
  elseif %speech% /= perfect
    set gem_vnum 55678
  else
    set gem_vnum 55645
  endif
elseif %speech% /= aquamarine
  if %speech% /= crushed
    set gem_vnum 55579
  elseif %speech% /= dust
    set gem_vnum 55571
  elseif %speech% /= uncut
    set gem_vnum 55613
  elseif %speech% /= flawed
    set gem_vnum 55635
  elseif %speech% /= shard
    set gem_vnum 55587
  elseif %speech% /= enchanted
    set gem_vnum 55734
  elseif %speech% /= radiant
    set gem_vnum 55712
  elseif %speech% /= perfect
    set gem_vnum 55690
  else
    set gem_vnum 55657
  endif
elseif %speech% /= beryl
  if %speech% /= uncut
    set gem_vnum 55606
  elseif %speech% /= flawed
    set gem_vnum 55628
  elseif %speech% /= shard
    set gem_vnum 55605
  elseif %speech% /= enchanted
    set gem_vnum 55727
  elseif %speech% /= radiant
    set gem_vnum 55705
  elseif %speech% /= perfect
    set gem_vnum 55683
  else
    set gem_vnum 55650
  endif
elseif %speech% /= bloodstone || %speech% /= blood
  if %speech% /= uncut
    set gem_vnum 55598
  elseif %speech% /= flawed
    set gem_vnum 55620
  elseif %speech% /= perfect
    set gem_vnum 55675
  elseif %speech% /= radiant
    set gem_vnum 55697
  elseif %speech% /= enchanted
    set gem_vnum 55719
  else
    set gem_vnum 55642
  endif
elseif %speech% /= carnelian
  if %speech% /= uncut
    set gem_vnum 55595
  elseif %speech% /= flawed
    set gem_vnum 55617
  elseif %speech% /= perfect
    set gem_vnum 55672
  elseif %speech% /= radiant
    set gem_vnum 55694
  elseif %speech% /= enchanted
    set gem_vnum 55716
  else
    set gem_vnum 55639
  endif
elseif %speech% /= citrine
  if %speech% /= crushed
    set gem_vnum 55577
  elseif %speech% /= dust
    set gem_vnum 55569
  elseif %speech% /= uncut
    set gem_vnum 55604
  elseif %speech% /= flawed
    set gem_vnum 55626
  elseif %speech% /= shard
    set gem_vnum 55585
  elseif %speech% /= enchanted
    set gem_vnum 55725
  elseif %speech% /= radiant
    set gem_vnum 55703
  elseif %speech% /= perfect
    set gem_vnum 55681
  else
    set gem_vnum 55648
  endif
elseif %speech% /= diamond
  if %speech% /= crushed
    set gem_vnum 55591
  elseif %speech% /= uncut
    set gem_vnum 55665
  elseif %speech% /= flawed
    set gem_vnum 55664
  elseif %speech% /= enchanted
    set gem_vnum 55741
  elseif %speech% /= radiant
    set gem_vnum 55742
  elseif %speech% /= perfect
    set gem_vnum 55745
  else
    set gem_vnum 55668
  endif
elseif %speech% /= emerald
  if %speech% /= crushed
    set gem_vnum 55593
  elseif %speech% /= uncut
    set gem_vnum 55663
  elseif %speech% /= flawed
    set gem_vnum 55660
  elseif %speech% /= enchanted
    set gem_vnum 55737
  elseif %speech% /= radiant
    set gem_vnum 55740
  elseif %speech% /= perfect
    set gem_vnum 55747
  else
    set gem_vnum 55670
  endif
elseif %speech% /= garnet
  if %speech% /= crushed
    set gem_vnum 55578
  elseif %speech% /= dust
    set gem_vnum 55570
  elseif %speech% /= uncut
    set gem_vnum 55612
  elseif %speech% /= flawed
    set gem_vnum 55634
  elseif %speech% /= shard
    set gem_vnum 55586
  elseif %speech% /= enchanted
    set gem_vnum 55733
  elseif %speech% /= radiant
    set gem_vnum 55711
  elseif %speech% /= perfect
    set gem_vnum 55689
  else
    set gem_vnum 55656
  endif
elseif %speech% /= hematite
  if %speech% /= uncut
    set gem_vnum 55594
  elseif %speech% /= flawed
    set gem_vnum 55616
  elseif %speech% /= perfect
    set gem_vnum 55671
  elseif %speech% /= radiant
    set gem_vnum 55693
  elseif %speech% /= enchanted
    set gem_vnum 55715
  else
    set gem_vnum 55638
  endif
elseif %speech% /= jade
  if %speech% /= uncut
    set gem_vnum 55608
  elseif %speech% /= flawed
    set gem_vnum 55630
  elseif %speech% /= shard
    set gem_vnum 55610
  elseif %speech% /= enchanted
    set gem_vnum 55729
  elseif %speech% /= radiant
    set gem_vnum 55707
  elseif %speech% /= perfect
    set gem_vnum 55685
  else
    set gem_vnum 55652
  endif
elseif %speech% /= jasper
  msend %actor% %self.name% says, 'I'm sorry, we don't stock jasper.'
  half
elseif %speech% /= labradorite
  if %speech% /= uncut
    set gem_vnum 55611
  elseif %speech% /= flawed
    set gem_vnum 55633
  elseif %speech% /= perfect
    set gem_vnum 55688
  elseif %speech% /= radiant
    set gem_vnum 55710
  elseif %speech% /= enchanted
    set gem_vnum 55732
  else
    set gem_vnum 55655
  endif
elseif %speech% /= lapis || %speech% /= lapis-lazuli || %speech% /= lazuli
  if %speech% /= uncut
    set gem_vnum 55600
  elseif %speech% /= flawed
    set gem_vnum 55622
  elseif %speech% /= enchanted
    set gem_vnum 55721
  elseif %speech% /= radiant
    set gem_vnum 55699
  elseif %speech% /= perfect
    set gem_vnum 55677
  else
    set gem_vnum 55644
  endif
elseif %speech% /= malachite
  if %speech% /= uncut
    set gem_vnum 55603
  elseif %speech% /= dust
    set gem_vnum 55568
  elseif %speech% /= crushed
    set gem_vnum 55576
  elseif %speech% /= shard
    set gem_vnum 55584
  elseif %speech% /= flawed
    set gem_vnum 55625
  elseif %speech% /= perfect
    set gem_vnum 55680
  elseif %speech% /= radiant
    set gem_vnum 55702
  elseif %speech% /= enchanted
    set gem_vnum 55724
  elseif %speech% /= chunk
    msend %actor% %self.name% says, 'That is not an item we trade in the gem exchange.'
    half
  else
    set gem_vnum 55647
  endif
elseif %speech% /= moonstone || %speech% /= moon
  if %speech% /= uncut
    set gem_vnum 55596
  elseif %speech% /= flawed
    set gem_vnum 55618
  elseif %speech% /= perfect
    set gem_vnum 55673
  elseif %speech% /= radiant
    set gem_vnum 55695
  elseif %speech% /= enchanted
    set gem_vnum 55717
  else
    set gem_vnum 55640
  endif
elseif %speech% /= onyx
  if %speech% /= uncut
    set gem_vnum 55609
  elseif %speech% /= flawed
    set gem_vnum 55631
  elseif %speech% /= perfect
    set gem_vnum 55686
  elseif %speech% /= radiant
    set gem_vnum 55708
  elseif %speech% /= enchanted
    set gem_vnum 55730
  else
    set gem_vnum 55653
  endif
elseif %speech% /= opal
  if %speech% /= uncut
    set gem_vnum 55610
  elseif %speech% /= flawed
    set gem_vnum 55632
  elseif %speech% /= enchanted
    set gem_vnum 55731
  elseif %speech% /= radiant
    set gem_vnum 55709
  elseif %speech% /= perfect
    set gem_vnum 55687
  else
    set gem_vnum 55654
  endif
elseif %speech% /= pearl
  if %speech% /= uncut
    set gem_vnum 55607
  elseif %speech% /= flawed
    set gem_vnum 55629
  elseif %speech% /= enchanted
    set gem_vnum 55728
  elseif %speech% /= radiant
    set gem_vnum 55706
  elseif %speech% /= perfect
    set gem_vnum 55684
  else
    set gem_vnum 55651
  endif
elseif %speech% /= peridot
  if %speech% /= uncut
    set gem_vnum 55615
  elseif %speech% /= crushed
    set gem_vnum 55581
  elseif %speech% /= dust
    set gem_vnum 55573
  elseif %speech% /= shard
    set gem_vnum 55589
  elseif %speech% /= flawed
    set gem_vnum 55637
  elseif %speech% /= perfect
    set gem_vnum 55692
  elseif %speech% /= radiant
    set gem_vnum 55714
  elseif %speech% /= enchanted
    set gem_vnum 55736
  else
    set gem_vnum 55659
  endif
elseif %speech% /= ruby
  if %speech% /= crushed
    set gem_vnum 55590
  elseif %speech% /= uncut
    set gem_vnum 55662
  elseif %speech% /= flawed
    set gem_vnum 55661
  elseif %speech% /= enchanted
    set gem_vnum 55738
  elseif %speech% /= radiant
    set gem_vnum 55739
  elseif %speech% /= perfect
    set gem_vnum 55744
  else
    set gem_vnum 55667
  endif
elseif %speech% /= sapphire
  if %speech% /= crushed
    set gem_vnum 55592
  elseif %speech% /= uncut
    set gem_vnum 55666
  elseif %speech% /= flawed
    set gem_vnum 55689
  elseif %speech% /= radiant
    set gem_vnum 55743
  elseif %speech% /= perfect
    set gem_vnum 55746
  else
    set gem_vnum 55669
  endif
elseif %speech% /= topaz
  if %speech% /= uncut
    set gem_vnum 55605
  elseif %speech% /= flawed
    set gem_vnum 55627
  elseif %speech% /= enchanted
    set gem_vnum 55726
  elseif %speech% /= radiant
    set gem_vnum 55704
  elseif %speech% /= perfect
    set gem_vnum 55682
  else
    set gem_vnum 55649
  endif
elseif %speech% /= tourmaline
  if %speech% /= crushed
    set gem_vnum 55580
  elseif %speech% /= dust
    set gem_vnum 55572
  elseif %speech% /= uncut
    set gem_vnum 55614
  elseif %speech% /= flawed
    set gem_vnum 55636
  elseif %speech% /= shard
    set gem_vnum 55588
  elseif %speech% /= enchanted
    set gem_vnum 55735
  elseif %speech% /= radiant
    set gem_vnum 55713
  elseif %speech% /= perfect
    set gem_vnum 55691
  else
    set gem_vnum 55658
  endif
elseif %speech% /= turquoise
  if %speech% /= uncut
    set gem_vnum 55597
  elseif %speech% /= flawed
    set gem_vnum 55619
  elseif %speech% /= enchanted
    set gem_vnum 55718
  elseif %speech% /= radiant
    set gem_vnum  55696
  elseif %speech% /= perfect
    set gem_vnum 55674
  else
    set gem_vnum 55641
  endif
endif
msend %actor% %self.name% asks you, 'You want %get.obj_shortdesc[%gem_vnum%]%?'
quest variable gem_exchange %actor% gem_vnum %gem_vnum%
~
#6294
Gem Exchange confirm order~
0 d 100
yes no~
wait 2
set item %actor.quest_variable[gem_exchange:gem_vnum]%
if %speech% /= yes
  if %item% == 0
    msend %actor% %self.name% says, 'Forgive me, I don't have an order under your name.'
    halt
  endif
  msend %actor% %self.name% writes your exchange order down.
  if %item% <= 55569
    set class 1
    set tier 1
  elseif %item% <= 55573
    set class 1
    set tier 2
  elseif %item% <= 55577
    set class 1
    set tier 3
  elseif %item% <= 55581
    set class 1
    set tier 4
  elseif %item% <= 55585
    set class 1
    set tier 5
  elseif %item% <= 55589
    set class 1
    set tier 6
  elseif %item% <= 55593
    set class 1
    set tier 7
  elseif %item% <= 55604
    set class 2
    set tier 1
  elseif %item% <= 55615
    set class 2
    set tier 2
  elseif %item% <= 55626
    set class 2
    set tier 3
  elseif %item% <= 55637
    set class 2
    set tier 4
  elseif %item% <= 55648
    set class 2
    set tier 5
  elseif %item% <= 55659
    set class 2
    set tier 6
  elseif %item% <= 55670
    set class 2
    set tier 7
  elseif %item% <= 55681
    set class 3
    set tier 1
  elseif %item% <= 55692
    set class 3
    set tier 2
  elseif %item% <= 55703
    set class 3
    set tier 3
  elseif %item% <= 55714
    set class 3
    set tier 4
  elseif %item% <= 55725
    set class 3
    set tier 5
  elseif %item% <= 55736
    set class 3
    set tier 6
  elseif %item% <= 55747
    set class 3
    set tier 7
  endif
  msend %actor% %self.name% says, 'So %get.obj_shortdesc[%item%]% is a &3&bclass %class% tier %tier%&0 gemstone.'
  wait 2s
  msend %actor% %self.name% says, 'We can exchange any gemstone of equal or greater rarity for it.'
  wait 2s
  msend %actor% %self.name% says, 'Consult the chart for degrees of rarity.'
  msend %actor% %self.name% points to the sign.
else
  quest variable gem_exchange %actor% gem_vnum 0
  msend %actor% %self.name% says, 'Alright, what DO you want then?'
endif
~
#6295
Gem Exchange receive exchange~
0 j 100
~
set item %actor.quest_variable[gem_exchange:gem_vnum]%
if %item% != 0
  if %item% <= 55569
    if %object.vnum% >= 55566 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55573
    if %object.vnum% >= 55570 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55577
    if %object.vnum% >= 55574 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55581
    if %object.vnum% >= 55578 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55585
    if %object.vnum% >= 55582 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55589
    if %object.vnum% >= 55586 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55593
    if %object.vnum% >= 55590 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55604
    if %object.vnum% >= 55594 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55615
    if %object.vnum% >= 55605 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55626
    if %object.vnum% >= 55616 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55637
    if %object.vnum% >= 55627 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55648
    if %object.vnum% >= 55638 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55659
    if %object.vnum% >= 55649 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55670
    if %object.vnum% >= 55660 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55681
    if %object.vnum% >= 55671 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55692
    if %object.vnum% >= 55682 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55703
    if %object.vnum% >= 55693 && %object.vnum% <=55751
      set found 1
    endif
  elseif %item% <= 55714
    if %object.vnum% >= 55704 && %object.vnum% <=55747
      set found 1
    endif
  elseif %item% <= 55725
    if %object.vnum% >= 55715 && %object.vnum% <=55747
      set found 1
    endif
  elseif %item% <= 55736
    if %object.vnum% >= 55726 && %object.vnum% <=55747
      set found 1
    endif
  elseif %item% <= 55747
    if %object.vnum% >= 55737 && %object.vnum% <=55747
      set found 1
    endif
  endif
  if %found% == 1
    wait 2
    msend %actor% %self.name% says, 'Here you are, as requested!'
    mjunk %object%
    mload obj %item%
    give all %actor%
    wait 2
    msend %actor% %self.name% says, 'A pleasure doing business with you!'
    quest variable gem_exchange %actor% gem_vnum 0
  else
    return 0
    msend %actor% %self.name% refuses to perform the exchange.
    wait 1s
    if %object.vnum% >= 55566 && %object.vnum% <=55751
      msend %actor% %self.name% says, 'I'm afraid %object.shortdesc% isn't of high enough rarity 
      msend %actor% &0to exchange for %get.obj_shortdesc[%item%]%.'
    else
      msend %actor% %self.name% says, 'Sorry, %object.shortdesc% isn't the kind of thing
      msend %actor% &0we trade around here...'
    endif
  endif
else
  return 0
  msend %actor% %self.name% refuses %object.shortdesc%.
  wait 1s
  msend %actor% %self.name% says, 'I don't have any exchange orders listed for you at the
  msend %actor% &0moment...'
endif
~
#6296
Gem Exchange buy blocker~
0 c 100
buy~
switch %cmd%
  case b
  case bu
    return 0
    halt
done
msend %actor% %self.name% says, 'The Soltan Gem Exchange is not a traditional shop.  We do not buy, sell, list, or value goods.'
~
#6297
Gem Exchange list blocker~
0 c 100
list~
switch %cmd%
  case l
  case li
    return 0
    halt
done
msend %actor% %self.name% says, 'The Soltan Gem Exchange is not a traditional shop.  We do not buy, sell, list, or value goods.'
~
#6298
Gem Exchange sell blocker~
0 c 100
sell~
switch %cmd%
  case s
    return 0
    halt
done
msend %actor% %self.name% says, 'The Soltan Gem Exchange is not a traditional shop.  We do not buy, sell, list, or value goods.'
~
#6299
Gem Exchange value blocker~
0 c 100
value~
msend %actor% %self.name% says, 'The Soltan Gem Exchange is not a traditional shop.  We do not buy, sell, list, or value goods.'
~
$~
