#2300
statue guards chest~
0 c 100
unlock~
if ((%arg% == chest) || (%arg% == oak))
    return 1
    mechoaround %actor% %self.name% steps between %actor.name% and the oak chest.
    msend %actor% %self.name% steps between you and the oak chest.
    say I am charged with the protection of this chest.
    say I cannot let you have its contents.
else
    return 0
endif
~
#2301
patriarch receives truthstone~
0 j 100
~
*
* original Smoldering Trident trigger
*
if (%object.vnum% == 2300)
  wait 1
  set object_shortdesc %object.shortdesc%
  mjunk %object.name%
  wait 3
  mechoaround %actor% %self.name% stops tossing meat into the fire and peers at %actor.name%.
  msend %actor% %self.name% stops tossing meat into the fire and looks at you.
  wait 3s
  if (%actor.class% == Diabolist)
    if (%actor.level% >= 35)
      smile
      say Excellent...very, very excellent.
      emote pockets %object_shortdesc%.
      wait 2s
      say He will be very happy with this.
      emote turns towards the fire and begins chanting in an arcane tongue.
      wait 3s
      emote chants, 'Olarag hroshi nofaraio...'
      wait 2s
      mecho The flames begin to &4fl&bare&0, licking the cavern roof above!
      wait 2s
      emote chants, 'Dala kenthu sarijko dennal...'
      wait 2s
      mecho The &9&bun&0&5ho&bly&0 &4fi&bre&0 continues to grow, nearly engulfing %self.name%!
      wait 2s
      emote chants, 'Yarak tilaru kenzo jing!'
      mecho As %self.name% completes %self.p% chant, the &4fla&bmes&0 suddenly contract into a ball.
      wait 3s
      mecho A &3chi&bll&2ing&0 voice croaks something in a language you do not understand.
      wait 2s
      mecho The ball of &4fl&bame&0 slowly begins to change shape, elongating and pulsating.
      mecho The &4fl&bames&0 take the shape of a &1scar&blet &3trident&0.
      wait 3s
      mecho The trident falls to the ground by the &9&bsmoldering &1emb&bers&0.
      wait 1s
      emote gets a smoldering trident.
      wait 1s
      say Yes, I believe you have proven yourself indeed.
      smile
      wait 2s
      say The weapon is for you.
      wait 1s
      if (%actor.sex% == Female)
        say Go forth and smite those who might stand against us, sister.
      elseif (%actor.sex% == Male)
        say Go forth and smite those who might stand against us, brother.
      else
        say Go forth and smite those who might stand against us.
      endif
      mload obj 2334
      give smoldering-trident %actor.name%
      if !%actor.quest_stage[hell_trident]%
        quest start hell_trident %actor%
      endif
      wait 1s
*
* for Hellfire Trident 
*
      if %actor.level% >= 65
        say Hell hungers for more and will reward you greatly if you feed it.  Attack with that trident 666 times and then seek out the Black Priestess, the left hand of Ruin Wormheart.  She will guide your offerings.
      else
        say Other forces of Hell will eventually take notice of you too now.  Seek out the left hand of Ruin Wormheart, the Black Priestess, after you have grown more.  She will be your emissary.
        msend %actor% &1Reach level 65 to continue this quest.&0
      endif
*
* for Hellfire and Brimstone
*
      if (%actor.level% > 56) && !%actor.quest_stage[hellfire_brimstone]%
        wait 2s
        say You seem to be quite powerful already.  Powerful enough in fact to handle the Fire of the Dark One.
        wait 2s
        say Would you like to learn to rain fire and brimstone down on your enemies?
      endif
*
* original Smoldering Trident trigger
*
    else
      blink
      say Quite amazing that you were able to retrieve %object_shortdesc%.
      peer %actor.name%
      wait 2s
      say Unfortunately, I don't think the Dark One would want to see such a weakling.  Return here again when you are more powerful.
    endif
  else
    snicker %actor.name%
    say You are no diabolist!  You no more deserve this power than Mielikki herself!
    emote tosses %object_shortdesc% in the flames.
    wait 2s
    say Run along now, there is nothing more for you here.
  endif
endif
*
* Hellfire and Brimstone continued
*
if %actor.quest_stage[hellfire_brimstone]% == 3
  if %object.vnum% == 4318 || %object.vnum% == 5211 || %object.vnum% == 5212 || %object.vnum% == 17308 || %object.vnum% == 48110 || %object.vnum% == 53000
    if %actor.quest_variable[hellfire_brimstone:%object.vnum%]%
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      say I have no need for a second %get.obj_noadesc[%object.vnum%]%!
    else
      wait 2
      quest variable hellfire_brimstone %actor.name% %object.vnum% 1
      mecho %self.name% holds %object.shortdesc% in his gnarled fist.
      say You have done well %actor.name%.
      mjunk %object.name%
    endif
    wait 2
    set item1 %actor.quest_variable[hellfire_brimstone:4318]%
    set item2 %actor.quest_variable[hellfire_brimstone:5211]%
    set item3 %actor.quest_variable[hellfire_brimstone:5212]%
    set item4 %actor.quest_variable[hellfire_brimstone:17308]%
    set item5 %actor.quest_variable[hellfire_brimstone:48110]%
    set item6 %actor.quest_variable[hellfire_brimstone:53000]%
    if %item1% && %item2% && %item3% && %item4% && %item5% && %item6%
      emote laughs with dark pleasure!
      say A fine set of tributes to the Dark One!
      wait 4s
      say Now behold as I set this cavern alight!
      wait 2s
      mecho &1&b%self.name% spreads the flaming tributes throughout the cavern.&0
      mecho &1&bThe piles of brimstone burn hot and bright!&0
      wait 3s
      mecho &1&bThe fires trace out lines on the cavern floor, forming a huge burning sigil.&0
      wait 4s
      msend %actor% &1&bGazing into the burning sigil, you feel the words to a prayer to the Dark One forming in your soul.&0&_
      msend %actor% You have learned the prayers for &1&bH&3e&1llf&3i&1re&0 &1and &bBr&3i&1mst&3o&1ne&0!
      quest complete hellfire_brimstone %actor.name%
      if !%actor.quest_variable[hell_trident:helltask4]% && %actor.quest_stage[hell_trident]% == 1
        quest variable hell_trident %actor% helltask4 1
      endif
      mskillset %actor% hellfire and brimstone
    else
      say Bring me the rest of the fiery tributes!
    endif
  else
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    say I have no need for this!
  endif
endif
~
#2302
west-blessing~
2 d 0
I pray for a blessing from mother earth, creator of life and bringer of death~
   wecho The &9&bstone monoliths&0 begin to &6&bglow&0 with power.
   wait 4s
   wload obj 2333
   wecho &2The power seems to coalesce into a shining ball.&0
   wait 20s
   wpurge
   wecho The &2shining ball&0 dissapates back into nothingness.
   wecho The &9&bstones&0 stop glowing.
~
#2303
south-blessing~
2 d 0
I pray for a blessing from mother earth, creator of life and bringer of death~
   wecho The &9&bstone monoliths&0 begin to &6&bglow&0 with power.
   wait 4s
   wload obj 2331
   wecho &7&bThe power seems to coalesce into a shining ball.&0
   wait 20s
   wpurge
   wecho The &7&bshining ball&0 dissapates back into nothingness.
   wecho The &9&bstones&0 stop glowing.
~
#2304
north-blessing~
2 d 0
I pray for a blessing from mother earth, creator of life and bringer of death~
   wecho The &9&bstone monoliths&0 begin to &6&bglow&0 with power.
   wait 4s
   wload obj 2332
   wecho &2The power seems to coalesce into a shining ball.&0
   wait 20s
   wpurge
   wecho The &2shining ball&0 dissapates back into nothingness.
   wecho The &9&bstones&0 stop glowing.
~
#2305
high-druid-random~
0 b 10
~
sigh
wait 2s
say If only the blessings hadn't been lost...
~
#2306
high-druid-blessings~
0 n 100
blessing blessings blessing? blessings?~
if %actor.vnum% == -1
if %actor.class% /= Druid
 wait 1
 nod
 wait 1s
 say Long ago, the founders of the Vale left gret stone monoliths in each of the four power points of nature.
 say There was one in each direction, and they were used to commune with the powers of nature.
 wait 2s
 say But the blessings of nature were lost, and we were forced to abandon the stones in order to protect our order.
 wait 2s
 say If you could prove yourself by bringing me the blessing of the south, perhaps you could be sent after the others as well.
 bow %actor.name%
else
 mecho %self.name% smiles sadly at %actor.name%.
 say I am afraid you cannot help me, my child.
 say This is a concern for the druids.
endif
endif
~
#2307
high-druid-receive~
0 j 100
~
if %actor.vnum% == -1
 if %actor.class% /= Druid
  if %object.vnum% == 2331
  smile %actor.name%
  set blessings 1
  global blessings
  wait 2s
  mjunk blessing
  say I am grateful the monoliths are still serving their purpose.
  say Perhaps the one to the west still retains its blessing as well?
  wait 2s
  say Please bring that to me as well!
  sigh
  say I would go myself, but I cannot leave the Vale.
  elseif %object.vnum% == 2333
  if %blessings% == 1
   set blessings 2
   global blessings
   mecho %self.name% closes his eyes and smiles.
   wait 2s
   mjunk blessing
   say This is heartening!  I do hope the sprites were not too much trouble.
   say They tricked us long ago and kept the powers of nature for themselves.
   wait 2s
   say Another power point is to the north.  Bring me that blessing and we shall be one step closer to restoring the stones.
   bow %actor.name%
  else
   say Why thank you.  But how did you know I wanted this?
   wait 2s
   mjunk blessing
endif
  elseif %object.vnum% == 2332
   if %blessings% == 2
   mecho The ancient druid's eyes light up as he receives the blessing.
   set blessings 3
   global blessings
   wait 2s
   mjunk blessing
   say At last, the restoration is almost complete!
   say The only one remaining is the blessing of the east.
   wait 2s
   say But be careful - it was placed far from the others.
   say Since many of the ancient roads are no more, you may have some difficulty finding it.
  else
   say Why thank you.  But this does me little good right now.
   wait 2s
   mjunk blessing
endif
  elseif %object.vnum% == 2330
   if %blessings% == 3
   mecho %self.name% heaves a great sigh of relief as he places the blessings on the stone slab.
   set blessings 0
   global blessings
wait 1s
   mecho The &9&bstone slab&0 begins to &7&bglow brightly!&0
   wait 2s
   mecho &2&bTendrils of power shoot out from the stone slab, lighting the vale!&0
   mecho %self.name% smiles hapily as he watches Nature's power at work.
   wait 4s
   mecho The &2&bglow&0 begins to fade.
   wait 1s
   msend %actor% %self.name% turns to you and bows.
   mechoaround %actor% %self.name% bows to %actor.name%.
   say Truly, you have performed a great service to the Druids of Anlun Vale.
   say Please, it is not much, but accept this s a token of my gratitude.
   wait 1s
   mecho %self.name% pulls something out of his robes.
   mload obj 2335
   give gaia-cloak %actor.name%
   wait 1s
   say Truly, you have the makings of a great druid within you.
   say Never lose sight of that, and you shall prosper.
   else
   say Very interesting - but I cannot use that just now.
   wait 2s
   mjunk blessing
  else
   eye %actor.name%
   say And why should I need this?
   return 0
  endif
  else
  return 0
say Thank you, but I do not need this.
msend %actor% %self.name% returns the item to you.
endif
else
  say I am sorry, but I am only looking for help from a fellow druid.
  msend %actor% %self.name% gives %object.shortdesc% back to you.
  endif
endif
~
#2308
hellfire_brimstone_patriarch_speech~
0 d 100
yes~
if %actor.class% /= Diabolist && %actor.level% > 56 && %actor.quest_stage[hellfire_brimstone]% == 0
  quest start hellfire_brimstone %actor.name%
  wait 2
  grin
  mecho %self.name% says, 'Then first, you must prove your dedication to the dark
  mecho &0gods.'
  wait 2s
  say I need more meat to stoke the sacrificial bonfire!
  wait 2s
  mecho %self.name% says, 'Slaughter paladins at the Sacred Haven, bring back
  mecho &0their flesh, and &1&b[drop]&0 it on the fire.'
  wait 4s
  say Six pounds should keep the fire burning long enough.
  wait 2s
  mecho %self.name% says, 'If you need guidance, ask me about your &7&b[spell progress]&0.'
endif
~
#2309
hellfire_brimstone_meat_death~
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
         if %person.quest_stage[hellfire_brimstone]% == 1
            set chance %random.10%
            if %chance% > 7
               mload obj 2338
            endif
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[hellfire_brimstone]% == 1
   set chance %random.10%
   if %chance% > 7
      mload obj 2338
   endif
endif
~
#2310
hellfire_brimstone_drop~
2 h 100
~
wait 1
if %actor.quest_stage[hellfire_brimstone]% == 1
  if %object.vnum% == 2338
    wecho The bonfire crackles and roars!
    wpurge %object%
    wait 1s
    wecho %get.mob_shortdesc[2311]% says, 'Ah, such a pleasing sacrifice.'
    eval meat %actor.quest_variable[hellfire_brimstone:meat]% + 1
    quest variable hellfire_brimstone %actor.name% meat %meat%
    if %actor.quest_variable[hellfire_brimstone:meat]% >= 6
      wait 1s
      wecho %get.mob_shortdesc[2311]% says, 'This should be enough.  The Dark One is paying&_attention now.'
      quest advance hellfire_brimstone %actor.name%
      quest variable hellfire_brimstone %actor.name% drop 0
      wait 2s
      wecho %get.mob_shortdesc[2311]% says, 'The next step is collecting the very soil of Hell&_itself: brimstone.  Brimstone can be found in and around volcanoes.  The fiery&_spirits that dwell there should collect it naturally.'
      wait 5s
      wecho %get.mob_shortdesc[2311]% says, 'Six quantities of brimstone are necessary to make the&_proper diagram for summoning hellfire.  Bring them here and &1&b[drop]&0 them'
    else
      wait 2
      wecho %get.mob_shortdesc[2311]% says, 'Bring some more.'
    endif
  endif
elseif %actor.quest_stage[hellfire_brimstone]% == 2
  if %object.vnum% == 2337
    wecho %get.mob_shortdesc[2311]% draws out lines with the brimstone.
    wpurge %object%
    wecho %get.mob_shortdesc[2311]% flashes a wicked grin.
    wecho %get.mob_shortdesc[2311]% says, 'It will burn like the very bowels of Hell.'
    eval brimstone %actor.quest_variable[hellfire_brimstone:brimstone]% + 1
    quest variable hellfire_brimstone %actor.name% brimstone %brimstone%
    if %actor.quest_variable[hellfire_brimstone:brimstone]% >= 6
      wait 2s
      nod
      wecho %get.mob_shortdesc[2311]% says, 'The Dark One is pleased with your efforts.'
      quest advance hellfire_brimstone %actor.name%
      quest variable hellfire_brimstone %actor.name% drop 0
      wait 2s
      wecho %get.mob_shortdesc[2311]% says, 'Finally, gather six different types of fiery tribute:'
      wecho - three colors of flame:
      wecho &0&9&b    black&0 from Chaos incarnate
      wecho &0&b    gray&0 from a devotee of neutrality
      wecho &0&7&b    white&0 from a beam of starlight
      wecho - a torch carried by an actress pretending to be a goddess
      wecho - a dagger carried by the volcano goddess
      wecho - a fiery sword wielded by a king in his throne room crypt
      wait 6s
      wecho %get.mob_shortdesc[2311]% says, 'Return with these six fires and by six given thrice,&_the spell shall be complete.'
    else
      wait 2
      wecho %get.mob_shortdesc[2311]% says, 'Bring some more.'
    endif
  endif
endif
~
#2311
hellfire_brimstone_fiery_death~
0 f 100
~
set room %self.room%
set person %room.people%
while %person%
  if %person.quest_stage[hellfire_brimstone]% 
    set drop %person.quest_variable[hellfire_brimstone:drop]%
    set chance %random.10%
    if %chance% > 6
         mload obj 2337
    endif
  endif
  set person %person.next_in_room%
done
~
#2312
hellfire_brimstone_status_checker~
0 d 0
spell progress~
set stage %actor.quest_stage[hellfire_brimstone]%
set meat %actor.quest_variable[hellfire_brimstone:meat]%
set brimstone %actor.quest_variable[hellfire_brimstone:brimstone]%
set item1 %actor.quest_variable[hellfire_brimstone:4318]%
set item2 %actor.quest_variable[hellfire_brimstone:5211]%
set item3 %actor.quest_variable[hellfire_brimstone:5212]%
set item4 %actor.quest_variable[hellfire_brimstone:17308]%
set item5 %actor.quest_variable[hellfire_brimstone:48110]%
set item6 %actor.quest_variable[hellfire_brimstone:53000]%
wait 2
switch %stage%
  case 1
    mecho %self.name% says, 'I need &1&bmeat&0 for the fire.'
    eval total (6 - %meat%)
    if %total% == 1
      mecho Bring me &1&b%total%&0 more pound of flesh from the paladins at the Sacred Haven.
    else
      mecho Bring me &1&b%total%&0 more pounds of flesh from the paladins at the Sacred Haven.
    endif
    break
  case 2
    mecho %self.name% says, 'I need &3&bbrimstone&0 to trace out the sigils.'
    mecho &0  
    eval total (6 - %brimstone%)
    if %total% == 1
      mecho Bring me &3&b%total%&0 more quantity of brimstone from fiery spirits on the
      mecho &0volcanic island to the north.
    else
      mecho Bring &3&b%total%&0 more quantities of brimstone from fiery spirits on the
      mecho &0volcanic island to the north.
    endif
    break
  case 3
    say You are to bring me fiery tributes to the Dark One.
    mecho &0  
    if %item1% || %item2% || %item3% || %item4% || %item5% || %item6%
      mecho You have already given me:
      if %item1%
        mecho - %get.obj_shortdesc[4318]%
      endif
      if %item2%
        mecho - %get.obj_shortdesc[5211]%
      endif
      if %item3%
        mecho - %get.obj_shortdesc[5212]%
      endif
      if %item4%
        mecho - %get.obj_shortdesc[17308]%
      endif
      if %item5%
        mecho - %get.obj_shortdesc[48110]%
      endif
      if %item6%
        mecho - %get.obj_shortdesc[53000]%
      endif
    endif
    mecho &0  
    mecho Now bring me:
    if !%item1%
      mecho - %get.obj_shortdesc[4318]%&0from an actress in Anduin.&0
    endif
    if !%item2%
      mecho - &7&b%get.obj_shortdesc[5211]%&0 from a beam of starlight deep in mine.&0
    endif
    if !%item3%
      mecho - &b%get.obj_shortdesc[5212]%&0 from a devotee of neutrality on a hill.&0
    endif
    if !%item4%
      mecho - &9&b%get.obj_shortdesc[17308]% from Chaos incarnate.&0
    endif
    if !%item5%
      mecho - &1&b%get.obj_shortdesc[48110]%&0 from the volcano goddess.&0
    endif
    if !%item6%
      mecho - %get.obj_shortdesc[53000]%&0 from a king in a throne room crypt.&0
    endif
    break
  default
    if %actor.has_completed[hellfire_brimstone]%
      say You have already earned the favor of the Dark One.
    endif
done
~
#2313
hellfire_brimstone_patriarch_greet~
0 g 100
~
wait 2
set stage %actor.quest_stage[hellfire_brimstone]%
switch %stage%
  case 1
    mecho %self.name% says, 'Have you brought me more meat?  &6&b[Drop]&0 it here if you have.'
    break
  case 2
    mecho %self.name% says, 'Have you found brimstone?  &6&b[Drop]&0 it here if you have.'
    break
  case 3
    say Give me any appropriate tributes you have found.
    break
  default
  if %actor.level% > 57 && %actor.class% /= Diabolist
    say You seem to be quite powerful...  Powerful enough in fact to handle the Fire of the Dark One.
    wait 2s
    say Would you like to learn to rain fire and brimstone down on your enemies?
  endif
done
~
#2314
Hell Trident speech trident upgrades~
0 d 100
trident upgrades~
wait 2
set hellstep %actor.quest_stage[hell_trident]%
if %hellstep% == 1
  set level 65
elseif %hellstep% == 2
  set level 90
endif
if %hellstep%
  switch %self.vnum%
    case 2311
      switch %hellstage%
        case 1
          set response stage2
          break
        case 2
          set response stage3
          break
        default
          if %actor.has_completed[hell_trident]%
            set response complete
          endif
      done
      break
    case 6032
      set step 1
      set spell1 %actor.has_completed[hellfire_brimstone]%
      set spell2 %actor.has_completed[banish]%
      if !%actor.quest_variable[hell_trident:helltask6]%
        if %actor.quest_stage[vilekka_stew]% > 3
          quest variable hell_trident %actor% helltask6 1
        endif
      endif
      break
    case 12526
      set step 2
      set spell1 %actor.has_completed[resurrection_quest]%
      set spell2 %actor.has_completed[hell_gate]%
  done
  if %hellstage% == %phase%
    if !%actor.quest_variable[hell_trident:helltask5]%
      if %spell1%
        quest variable hell_trident %actor% helltask5 1
      endif
    endif
    if !%actor.quest_variable[hell_trident:helltask4]%
      if %spell2%
        quest variable hell_trident %actor% helltask4 1
      endif
    endif
  endif
  if %actor.quest_stage[hell_trident]% == %step%
    if %actor.level% >= %level%
      if !%actor.quest_variable[hell_trident:greet]%
        quest variable hell_trident %actor% greet 1
        if %self.vnum% == 6032
          msend %actor% In the hissed whisper of ecstatic fervor %self.name% says, 'Six thrice is the number of the greater demons.  Undertaking thus six and six, and six again, their interest can be piqued.'
          msend %actor% &0  
          msend %actor% - Attempting to land &3&bsix and six and six&0 in thrusts of your trident shall provide the pattern of the number.
          msend %actor% &0  
          msend %actor% - &3&bSix rubies, all uncut in nature, does double the signature.
          msend %actor% &0  
          msend %actor% - Slay &3&bsix beings called angels&0, be they of any make or countenance, the number being counted thrice.
          msend %actor% &0  
          msend %actor% - Sup on the grand powers of Hell to learn the most esoteric diablery.  Complete the quests for &3&bBanish&0 and &3&bHellfire and Brimstone&0.
          msend %actor% &0  
          msend %actor% - Assist the High Priestess of Lolth in hunting down and destroying the heretics of her Goddess.
          wait 2s
          msend %actor% %self.name% says, 'In undertaking six in all will Hell be at your beck and call.'
        elseif %self.vnum% == 12526
          msend %actor% %self.name% says, 'Perhaps I will grant you a boon if you carry out sacrifices in my name.'
          msend %actor% &0  
          msend %actor% - &3&bAttack with %get.obj_shortdesc[2339]% 666 times&0 as a sacrifice of power.
          msend %actor% &0  
          msend %actor% - Bring me &3&bsix radiant rubies&0 as a sacrifice of wealth.
          msend %actor% &0  
          msend %actor% - Slay a combination of &3&bsix ghaeles, solars, or lesser seraphs&0.  Their suffering shall please me as a sacrifice of divinity.
          msend %actor% &0  
          msend %actor% - Learn the most unholy of prayers.  Complete the quests for &3&bResurrection&0 and &3&bHell Gate&0 as a sacrifice of knowledge.
          msend %actor% &0  
          msend %actor% - Finally, find one long-buried and branded an infidel.  &3&bFinish his undying duel for him&0 as a sacrifice of honor.
          wait 2s
          msend %actor% %self.name% says, 'Do this and I will lend you a greater share of my power.'
        endif
        wait 2s
        msend %actor% %self.name% says, 'You can ask about your &6&b[weapon progress]&0 at any time.'
      else
        msend %actor% %self.name% says, 'Remember your six sacrifices.  Ask about your &6&b[weapon progress]&0 if you must.'
      endif
    else
      set response level
    endif
  else
    if %actor.has_completed[hell_trident]%
      set response complete
    elseif %actor.quest_stage[hell_trident]% < %step%
      set response stage1
    else
      set response stage3
    endif
  endif
  if %response% == stage1
    msend %actor% %self.name% says, 'You must make the initial offerings with another dark priest.'
  elseif %response% == stage2
    if %actor.level% >= %level%
      msend %actor% %self.name% says, 'Hell hungers for more and will reward you greatly if you feed it.  Attack with that trident 666 times and then seek out the Black Priestess, the left hand of Ruin Wormheart.  She will guide your offerings.'
    else
      msend %actor% %self.name% says, 'Other forces of Hell will eventually take notice of you too now.  Seek out the left hand of Ruin Wormheart, the Black Priestess, after you have grown more.  She will be your emissary.'
      msend %actor% &1You must be level %level% or greater to continue this quest.&0
    endif
  elseif %response% == stage3
    if %actor.level% >= %level%
    msend %actor% %self.name% says, 'The Demon Lord Krisenna is known to traffic with mortals from time to time.  Impress him and perhaps he will grant you a boon.'
    else
      msend %actor% %self.name% says, 'Continue to prove your value to Hell and perhaps a Demon Lord might be willing to grant your their patronage.'
      msend %actor% &1You must be level %level% or greater to continue this quest.&0
    endif
  elseif %response% == complete
    msend %actor% %self.name% says, 'You've already marshalled the forces of Hell and Damnation to your side!'
  elseif %response% == level
    msend %actor% %self.name% say, 'Stand before me again when you have achieved a larger measure of greatness.'
    msend %actor% &1You must be level %level% or greater to continue this quest.&0
  endif
endif
~
#2315
**UNUSED**~
0 f 100
~
*
* for Creeping Doom
*
set person %actor%
set i %actor.group_size%
if %i%
   unset person
   while %i% > 0
      set person %actor.group_member[%i%]%
      if %person.room% == %self.room%
         if %person.quest_stage[creeping_doom]% == 2 || (%person.level% > 80 && (%person.vnum% >= 1000 && %person.vnum% <= 1038))
            set rnd %random.50%
            if %rnd% <= %self.level%
               mload obj 61517
            endif
         endif
      endif
      eval i %i% - 1
   done
elseif %person.quest_stage[creeping_doom]% == 2 || (%person.level% > 80 && (%person.vnum% >= 1000 && %person.vnum% <= 1038))
   set rnd %random.50%
   if %rnd% <= %self.level%
     mload obj 61517
   endif
endif
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* boss setup - 55580 boss_27
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
      eval gem_vnum %what_gem_drop% + 55604
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set 
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set 
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >=61 &%will_drop% <= 80)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55339
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55604
      eval armor_vnum %what_armor_drop% + 55331
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set 
      eval armor_vnum %what_armor_drop% + 55335
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55615
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55626
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55639
      mload obj %armor_vnum%
   endif
endif
~
#2348
stablehand_list~
0 c 100
list~
return 1
msend %actor% Available pets are:
msend %actor% a steady warhorse - &340&0 c
~
#2349
**UNUSED**~
0 c 100
li~
return 0
~
#2350
stablehand_buy~
0 c 100
buy~
return 1
if %arg% == steady || %arg% == warhorse || %arg% == horse
   mteleport %actor% 3091
   mat 3091 mforce %actor% buy warhorse
   mat 3091 mteleport %actor% 2348
   mat 3091 mteleport warhorse 2348
   mechoaround %actor% A steady warhorse starts following %actor.name%.
else
   msend %actor% There is no such pet!
endif
~
$~
