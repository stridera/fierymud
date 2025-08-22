#39000
flood_lady_random~
0 b 10
~
switch %random.4%
  case 1
    emote screams in fury!
    break
  case 2
    emote screams, 'I WILL TAKE BACK WHAT YOU HAVE STOLEN!'
    emote bangs furiously on the gate.
    break
  case 3
    mecho The waves slam against the gate!
    emote screams, 'YOU WILL NOT ESCAPE THE SEA'S WRATH.'
    break
  default
    emote screams, 'THE SEA WILL RISE TO FLOOD YOU.'
    break
done
~
#39001
flood_lady_greet~
0 g 100
~
wait 1s
set stage %actor.quest_stage[flood]%
if %stage% == 0
  if %actor.class% /= Cryomancer && %actor.level% > 80
    point %actor%
    emote screams, 'YOU.'
    wait 2
    emote screams, 'YOU WILL HELP ME DESTROY THIS SETTLEMENT.'
  endif
else
  if %stage% == 1
    mecho %self.name% says, 'Why have you not yet convinced the other great
    mecho &0waters to assist me??'
    emote fumes.
  elseif %stage% == 2
    mecho %self.name% says, 'The waters are ready!  Give me back the heart and
    mecho &0the ocean will rage!'
  endif
endif
~
#39002
flood_lady_speech~
0 d 100
flood help? flood? How? Why?~
wait 2
set stage %actor.quest_stage[flood]%
if %stage% == 0
  if %actor.class% /= Cryomancer && %actor.level% > 80
    quest start flood %actor.name%
    mecho %self.name% says, 'The masters of this settlement stole my most
    mecho &0precious treasures.  I will punish them with a cataclysm of rising tide and
    mecho &0raging torrents by calling to the great waters of Ethilien to my aid.  You will
    mecho &0be my envoy to their domains.'
    wait 3s
    say The great waters are:
    mecho &7&bThe Blue-Fog River and Lake&0
    mecho &7&bPhoenix Feather Hot Springs&0
    mecho &7&bThree-Falls River in the canyon&0
    mecho &7&bThe Greengreen Sea&0
    mecho &7&bSea's Lullaby&0
    mecho &7&bFrost Lake&0
    mecho &7&bBlack Lake&0
    mecho &7&bThe Dreaming River in the Realm of the King of Dreams&0
    wait 4s
    mload obj 39000
    give heart-ocean %actor.name%
    mecho %self.name% says, 'Standing in the waters with this, say:
    mecho &4&bthe Arabel Ocean calls for aid&0.'
    wait 4s
    say They will respond.
    wait 2s
    mecho %self.name% says, 'Tell them I long for &4&brevenge&0.'
    wait 4s
    mecho %self.name% says, 'You will acquiesce to this request.  In exchange, I
    mecho &0will teach you to control the raging tides to demolish your enemies.'
    wait 2s
    mecho %self.name% says, 'If you need, I can update you on your &7&b[progress]&0.'
  endif
elseif %stage% == 1
  mecho %self.name% says, 'Why have you not yet convinced the other great
  mecho &0waters to assist me??'
  emote fumes.
elseif %stage% == 2
  mecho %self.name% says, 'The waters are ready!  Give me back the heart and
  mecho &0the ocean will rage!'
endif
~
#39003
flood_lady_speech2~
0 d 100
what stolen? Take?~
if %speech% == what did they take? || %speech% == stolen? || %speech% == what was stolen? || %speech% == stolen
  mecho %self.name% says, 'What they took matters not!  They have stolen from
  mecho &0the sea and must either return what they stole or be destroyed.'
endif
~
#39004
flood_heart_speech~
1 c 3
say~
return 0
wait 2
if %actor.quest_stage[flood]% == 1
  set room %actor.room%
  set zone %room.vnum%
  if %arg% /= the Arabel ocean calls for aid || %arg% /= spirit I have returned || %arg% /= spirit, I have returned
    *
    * for Blue-Fog River and Lake
    *
    if %zone% >= 2800 && %zone% <= 2910
        set color &4
        set spirit %get.mob_shortdesc[39013]%
      if %get.mob_count[39013]% == 0 && !%actor.quest_variable[flood:water1]%
        oecho &6%self.shortdesc% shimmers with pale light!&0
        wait 1s
        oload mob 39013
        oecho %color%A misty blue spirit rises from the water.&0
      endif
    *
    * for Phoenix Feather Hot Springs
    *
    elseif %zone% == 10314 || %zone% == 10316 || %zone% >= 10318 && %zone% <= 10335
        set color &6
        set spirit %get.mob_shortdesc[39014]%
      if %get.mob_count[39014]% == 0 && !%actor.quest_variable[flood:water2]%
        oecho &6%self.shortdesc% shimmers with pale light!&0
        wait 1s
        oload mob 39014
        oecho %color%A shining watery bird rises from the water.&0
      endif
    *
    * for Canyon - multiple lines because one is too long
    *
    elseif %zone% == 17802 || (%zone% >= 17811 && %zone% <= 17813) || %zone% == 17816 || %zone% == 17817 || (%zone% >= 17823 && %zone% <= 17827) 
        set color &4&b
        set spirit %get.mob_shortdesc[39015]%
      if %get.mob_count[39015]% == 0 && !%actor.quest_variable[flood:water3]%
        oecho &6%self.shortdesc% shimmers with pale light!&0
        wait 1s
        oload mob 39015
        oecho %color%A three-faced humanoid figure rises from the water.&0
      endif
    elseif %zone% == 17834 || (%zone% >= 17839 && %zone% <= 17841 || %zone% == 17847 || %zone% == 17850 || (%zone% >= 17853 && %zone% <= 17856)
        set color &4&b
        set spirit %get.mob_shortdesc[39015]%
      if %get.mob_count[39015]% == 0 && !%actor.quest_variable[flood:water3]%
        oecho &6%self.shortdesc% shimmers with pale light!&0
        wait 1s
        oload mob 39015
        oecho %color%A three-faced humanoid figure rises from the water.&0
      endif
    elseif %zone% == 17862 || %zone% == 17867
        set color &4&b
        set spirit %get.mob_shortdesc[39015]%
      if %get.mob_count[39015]% == 0 && !%actor.quest_variable[flood:water3]%
        oecho &6%self.shortdesc% shimmers with pale light!&0
        wait 1s
        oload mob 39015
        oecho %color%A three-faced humanoid figure rises from the water.&0
      endif
    *
    * for Greengreen Sea
    *
    elseif %zone% >= 36200 && %zone% <= 36231
        set color &2
        set spirit %get.mob_shortdesc[39016]%
      if %get.mob_count[39016]% == 0 && !%actor.quest_variable[flood:water4]%
        oecho &6%self.shortdesc% shimmers with pale light!&0
        wait 1s
        oload mob 39016
        oecho %color%A hideous creature with distended mouth and belly rises from the water.&0
      endif
    * 
    * for SeaWitch
    *
    elseif %zone% >= 41100 && %zone% <= 41242
      set color &4&b
      set spirit %get.mob_shortdesc[39017]%
      osend %actor% Watery song whispers in your ear, %color%'The Sea Witch must be removed before I may&0
      osend %actor% &0%color%speak to thee...  Call to me from the bottom of the sea...'&0
    elseif %zone% == 41243
      set color &4&b
      set spirit %get.mob_shortdesc[39017]%
      if %get.mob_count[41119]% == 0
        if %get.mob_count[39017]% == 0 && !%actor.quest_variable[flood:water5]%
          oecho &6%self.shortdesc% shimmers with pale light!&0
          wait 1s
          oload mob 39017
          oecho %color%The sea's currents noticeably ripple and flux in response.&0
        endif
      else
        osend %actor% %color%Watery song whispers in your ear, 'The Sea Witch must be removed before I may&0
        osend %actor% &0%color%speak to thee...'&0
      endif
    *
    * for Frost Valley - two lines
    *
    elseif %zone% >= 53438 && %zone% <= 53440 || %zone% >= 53445 && %zone% <= 53449 || %zone% == 53452 || %zone% == 53455 || %zone% == 53456 || %zone% == 53464 
        set color &7&b
        set spirit %get.mob_shortdesc[39018]%
      if %get.mob_count[39018]% == 0 && !%actor.quest_variable[flood:water6]%
        oecho &6%self.shortdesc% shimmers with pale light!&0
        wait 1s
        oload mob 39018
        oecho %color%A willowy white woman rises from the lake.&0
      endif
    elseif %zone% == 53467 || %zone% == 53468 || %zone% >= 53472 && %zone% <= 53475 || %zone% == 53481 || %zone% == 53482
        set color &7&b
        set spirit %get.mob_shortdesc[39018]%
      if %get.mob_count[39018]% == 0 && %actor.quest_variable[flood:water6]%
        oecho &6%self.shortdesc% shimmers with pale light!&0
        wait 1s
        oload mob 39018
        oecho %color%A willowy icy white woman rises from the lake.&0
      endif
    *
    * for Black Lake
    *
    elseif (%zone% >= 56402 && %zone% <= 56404) || (%zone% >= 56406 && %zone% <= 56431) || %zone% == 37072
        set color &9&b
        set spirit %get.mob_shortdesc[39019]%
      if %get.mob_count[39019]% == 0 && !%actor.quest_variable[flood:water7]%
        oecho &6%self.shortdesc% shimmers with pale light!&0
        wait 1s
        oload mob 39019
        oecho %color%An inky black shade rises from the depths.&0
      endif
    *
    * for KoD
    *
    elseif (%zone% >= 58511 && %zone% <= 58519)
          set color &6&b
          set spirit %get.mob_shortdesc[39020]%
      if (%time.hour% > 19 || %time.hour% < 5)
        if %get.mob_count[39020]% == 0 && !%actor.quest_variable[flood:water8]%
          oecho &6%self.shortdesc% shimmers with pale light!&0
          wait 1s
          oload mob 39020
          oecho %color%In a spray of moon-lit iridescent mist, a beautiful translucent blue woman emerges from the stream.&0
        endif
      else
        oecho A giggling voice says, %color%'None dream while the sun shines.'&0
      endif
    endif
    wait 1s
    if %arg% /= the arabel ocean calls for aid
      oforce spirit mecho %spirit% says, %color%'Why does the ocean call for aid?'&0
    else
      oforce spirit mecho %spirit% says, %color%'Do you have what I asked for?'&0
    endif
  endif
endif
~
#39005
flood_spirits_speech1~
0 d 100
help~
switch %self.vnum%
  case 39013
    set color &4
    break
  case 39014
    set color &6
    break
  case 39016
    set color &2
    break
  case 39018
    set color &7&b
    break
  case 39020
    set color &6&b
    break
  case 39019
    set color &9&b
    break
  case 39015
  case 39017
  default
    set color &4&b
done
if %actor.quest_stage[flood]% == 1
  mecho %self.name% says, %color%'Help with what?'&0
endif
~
#39006
flood_spirits_speech2~
0 d 100
stolen revenge destroy flood return~
if %actor.quest_stage[flood]% == 1
  wait 1s
*
* Blue-fog wants nothing.
*
  if %self.vnum% == 39013
    set color &4
    mecho %self.name% says, %color%'The ocean is a sister to me.  Long have our waters mingled&0
    mecho &0%color%and met.'&0
    wait 3s
    mecho %self.name% says coldly, %color%'Hundreds rest in watery graves in my depths.  What&0
    mecho &0%color%are a few more?  I shall rally to her cause.'&0
    wait 2
    shrug
    quest variable flood %actor.name% water1 1
    wait 3s
    mecho %color%%self.name% vanishes back into the water.&0
*
* Phoenix wants the feather
*
  elseif %self.vnum% == 39014
    set color &6
    mecho %self.name% says, %color%'I will join this effort if heat can be provided for my&0
    mecho &0%color%spring while I am away.'&0
    wait 2s
    mecho %self.name% says, %color%'Bring me a glowing phoenix feather from the Realm of&0
    mecho &0%color%the King of Dreams and I shall join the ocean's cause.'&0
    mecho   
    mecho %self.name% says, %color%'Say &4&b[Spirit I have returned]&0%color% when you return with what&0
    mecho &0%color%I ask.'&0
    wait 2s 
    mecho %color%With a mighty cry %self.name% dives back into the water.&0
    quest variable flood %actor.name% item2 1
*
* Three-Falls wants a water dance
*
  elseif %self.vnum% == 39015
    set color &4&b
    mecho %self.name% speaks in three voices at once:
    mecho %color%'We only receive petitions from those that perform the proper dances.  Observe&0
    mecho &0%color%the traditions of the Canyon tribes.  Stand before us with a &6&bbell&0%color%, the&0
    mecho &0%color%instrument of water, and &6&bdance&0%color% your supplications.  Only then will we heed the&0
    mecho &0%color%ocean's request.'&0
    mecho   
    Mecho %self.name% says, %color%'Say &4&b[Spirit I have returned]&0%color% when you return&0
    mecho &0%color%with what I ask.'&0
    wait 3s
    quest variable flood %actor.name% item3 1
    mecho %color%%self.name% rushes away with the river rapids.&0
*
* Greengreen wants to devour things
*
  elseif %self.vnum% == 39016
    set color &2
    mecho With a voice like rattling bones %self.name% speaks:
    mecho %color%'I move only as my hunger is sated.  Feed me the gluttonous pleasures my&0
    mecho &0%color%victims once knew in life!'&0
    wait 2s
    mecho %self.name% says, %color%'Bring me foods that I may devour them!  But never the&0
    mecho &0%color%same thing twice!'&0
    mecho   
    mecho %self.name% says, %color%'Say &4&b[Spirit I have returned]&0%color% when you bring me&0
    mecho &0%color%what I ask.'&0
    wait 3s
    mecho %color%%self.name% sinks beneath the waves.&0
    quest variable flood %actor.name% item4 1
*
* Lullaby is free once the Sea Witch is defeated and is called from the Witch's lair
*
  elseif %self.vnum% == 39017
    set color &4&b
    mecho In sonorous tones %self.name% says, %color%'As the mighty Sea Witch has been defeated&0
    mecho &0%color%I shall join thee in thy cause.'&0
    mecho    
    mecho %color%Music echoes through the waves as the currents shift and rush back into the ocean!&0
    quest variable flood %actor.name% water5 1
*
* Frozen wants a fight
*
  elseif %self.vnum% == 39018
    set color &7&b
    quest variable flood %actor.name% item6 1
    mecho %self.name% laughs in a voice as freezing as the blasting wind.
    wait 1s
    mecho %self.name% says, %color%'The Ocean thinks itself so mighty that the&0
    mecho &0%color%great Frozen Lake should heed its call?'&0
    wait 2
    mecho %self.name% scoffs.
    wait 2
    mecho %self.name% says, %color%'Ridiculous.'&0
    wait 4s
    mecho %self.name% descends shrieking,%color% 'I shall destroy you for your&0
    mecho &0%color%insolence, Envoy!'&0
    wait 3s
    mkill %actor.name%
    halt
*
* Black Lake wants an infinite light
*
  elseif %self.vnum% == 39019
    set color &9&b
    mecho The sound of %self.name%'s laughter terrifies you to your very soul!
    wait 1s
    mecho %self.name% speaks in sounds like hot steam forced through shredded flesh:
    mecho %color%'The Black Lake will assist the Arabel Ocean on one condition:&0
    mecho &0%color%Bring me an eternal light to snuff out so the world may be a touch darker.&0
    mecho &0%color%Then the Lake shall assist in consuming any lives you ask.'&0
    mecho   
    mecho %self.name% says, %color%'Say &4&b[Spirit I have returned]&0%color% when you&0 
    mecho &0%color%return with what I ask.'&0
    quest variable flood %actor.name% item7 1
    mecho   
    mecho %color%%self.name% descends back into the darkness from whence it came.&0
*
* Dreaming Undine wants nothing but only comes out at night.
*
  elseif %self.vnum% == 39020
    set color &6&b
    mecho %self.name% giggles like peeling bells.
    mecho %self.name% traces the dream-like reflection of the moon on the surface of the stream.
    mecho   
    mecho %self.name% says, %color%'May She be witness to this agreement.'&0
    shake %actor%
    wait 2s
    mecho %self.name% chimes:
    mecho %color%'If the moon doth shine then I shall go&0
    mecho &0%color%following Dreaming's ebb and flow.'&0
    quest variable flood %actor.name% water8 1
    mecho   
    mecho %color%%self.name% drifts away on the moonlit current.&0
  endif
  set water1 %actor.quest_variable[flood:water1]%
  set water2 %actor.quest_variable[flood:water2]%
  set water3 %actor.quest_variable[flood:water3]%
  set water4 %actor.quest_variable[flood:water4]%
  set water5 %actor.quest_variable[flood:water5]%
  set water6 %actor.quest_variable[flood:water6]%
  set water7 %actor.quest_variable[flood:water7]%
  set water8 %actor.quest_variable[flood:water8]%
  if %water1% && %water2% && %water3% && %water4% && %water5% && %water6% && %water7% && %water8%
    quest advance flood %actor%
    wait 1s
    msend %actor% &4&bYou have garnered the support of all the great waters!&0
  endif
  mpurge %self%
endif
~
#39007
flood_spirits_receive~
0 j 100
~
set stage %actor.quest_stage[flood]%
switch %self.vnum%
  case 39012
    if %stage% == 2
      if %object.vnum% == 39000
        mjunk heart-ocean
        wait 1s
        mecho %self.name% screams, 'LET THE SIEGE BEGIN!!'
        mecho   
        mecho The ocean erupts in a violent frenzy.
        wait 3s
        mecho Massive waves begin to crest and smash against the settlement gate.
        mecho %self.name% screams with reckless abandon like the most terrifying of barbarian berserkers.
        wait 6s
        mecho Spirits rise up through the churning ocean.
        mecho Giant black waves and enormous icebergs assault the rocky shores!
        mecho %self.name% wails, 'YOU SHALL DIE FOR STEALING FROM ME.'
        wait 7s
        mecho The nine Great Waters pull away from the settlement's borders as if granting a moment of reprieve.
        wait 4s
        mecho The ocean's level drops as the waters quickly rush away.
        wait 4s
        mecho On the horizon, a titanic wall of water rises up like a hellish behemoth.
        wait 2s
        mecho The gargantuan tsunami rushes toward the rocks and SMASHES through the settlement gate!
        wait 6s
        mecho Screaming voices are instantly snuffed out.
        mecho Dozens of bodies smash into the rocks and shatter like glass as the tsunami obliterates the settlement.
        wait 8s
        mecho As quickly as it began, the flood waters recede leaving nothing but chilling silence and carnage in its wake.
        wait 6s
        mecho The Lady of the Sea vanishes beneath the waves.
        m_run_room_trig 39013
        wait 4s
        msend %actor% A watery voice floats past your ear:
        msend %actor% 'Never forget what you have witnessed here today.'
        wait 1s
        msend %actor% It whispers the eldritch formula for summoning such a cataclysm again.
        quest complete flood %actor.name%
        mskillset %actor.name% flood
        msend %actor% &4&bYou have learned Flood.&0
        wait 2s
        msend %actor% 'Thank you for the role you played.'
        mpurge %self%
      else
        return 0
        mecho %self.name% refuses %object.shortdesc%.
        say This is not my heart!
        wait 1s
        mecho %self.name% bellows, 'GIVE IT BACK OR I WILL DESTROY YOU.'
      endif
    endif
    break
  case 39014
    set color &6
    if %stage% == 1
      if %object.vnum% == 58401
        mjunk feather
        wait 1s
        mecho %self.name% submerges the feather in the steaming water.
        mecho %self.name% says, %color%'I can entrust the safety of the springs to this&0
        mecho &0%color%feather's magical energy for a short while.  I will join the ocean's crusade.'&0
        quest variable flood %actor.name% water2 1
        wait 2s
        mecho %color%With a mighty cry %self.name% dives back into the water.&0
      else
        return 0
        mecho %self.name% says, %color%'This will not keep my spring sufficiently warm.'&0
      endif
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      mecho %self.name% says, %color%'I need nothing from you.'&0
    endif
    break
  case 39016
    set color &2
    if %stage% == 1
      if %object.type% == food
        if %actor.quest_variable[flood:%object.vnum%]%
          wait 2
          mecho %self.name% wails in anger as she thrashes about!
          mecho %self.name% throws %object.shortdesc% into the sea!
          mecho %self.name% says, %color%'I have already eaten that!  I refuse to eat it again!'&0
        else
          quest variable flood %actor.name% %object.vnum% 1
          set full %actor.quest_variable[flood:hunger]%
          eval hunger (%full% + %object.val0%)
          quest variable flood %actor.name% hunger %hunger%
          mjunk food
          mecho %self.name% greedily devours %object.shortdesc%.
          if %hunger% >= 200
            wait 2
            mecho %self.name% licks her massive chops.&0
            wait 1s
            burp
            mecho %self.name% says, %color%'The dead within me are sated.'
            wait 1s
            mecho %self.name% says, %color%'For now.'
            wait 1s
            mecho %self.name% says, %color%'We will join with the ocean in her revenge.'
            wait 1s
            mecho %color%%self.name% sinks beneath the waves.&0
            quest variable flood %actor.name% water4 1
          else
            wait 2
            mecho %self.name% licks her massive chops.
            mecho %self.name% says, %color%'Bring me more!'&0
            halt
          endif
        endif
      else
        return 0
        mecho %self.name% refuses %object.shortdesc%.
        mecho %self.name% says, %color%'This is not food!!'&0
      endif
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      mecho %self.name% says, %color%'I want nothing from you.'&0
    endif
    break
  case 39019
    set color &9&b
    if %stage% == 1
      if %object.type% == light
        if %object.val1% == -1
          quest variable flood %actor.name% water7 1
          wait 2
          mecho %self.name% grins with a sick malevolence.
          wait 2s
          mecho %self.name% says, %color%'I'll take great joy in this!'&0
          wait 2s
          mecho %self.name% plunges the light into the depths of the Black Lake.
          mjunk %object%
          wait 3s
          mecho The light twinkles for a few moments as it sinks into the darkness before fading forever.
          wait 4s
          laugh
          mecho %self.name% says, %color%'Intoxicating.'&0
          wait 2s
          mecho %self.name% says,%color% 'I will join the Arabel Ocean with pleasure.'&0
          bow
          wait 2s
          mecho %color%%self.name% splashes back into the inky blackness of the lake.&0
        else
          return 0
          mecho %self.name% scoffs.
          mecho %self.name% refuses %object.shortdesc%.
          mecho %self.name% says, %color%'This wouldn't glow forever even before I consume it.'&0
        endif
      else
        return 0
        mecho %self.name% refuses %object.shortdesc%.
        mecho %self.name% says, %color%'This isn't even a light!'&0
      endif
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      mecho %self.name% says, %color%'You have nothing to offer me.'&0
    endif
    break
  default
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    mecho %self.name% says, %color%'I have no need for this.'&0
done
if %stage% == 1
  set water1 %actor.quest_variable[flood:water1]%
  set water2 %actor.quest_variable[flood:water2]%
  set water3 %actor.quest_variable[flood:water3]%
  set water4 %actor.quest_variable[flood:water4]%
  set water5 %actor.quest_variable[flood:water5]%
  set water6 %actor.quest_variable[flood:water6]%
  set water7 %actor.quest_variable[flood:water7]%
  set water8 %actor.quest_variable[flood:water8]%
  if %water1% && %water2% && %water3% && %water4% && %water5% && %water6% && %water7% && %water8%
    quest advance flood %actor.name%
    wait 1s
    msend %actor% &4&bYou have garnered the support of all the great waters!&0
  endif
endif
mpurge %self%
~
#39008
flood_totem_dance~
0 c 100
dance~
switch %cmd%
  case d
    return 0
    halt
done
set color &4&b
if %actor.quest_stage[flood]% == 1
  set bell1 %actor.inventory[6903]%
  set bell2 %actor.inventory[12311]%
  set bell3 %actor.inventory[12316]%
  set bell4 %actor.wearing[6903]%
  set bell5 %actor.wearing[12311]% 
  set bell6 %actor.wearing[12316]%
  set bell7 %actor.inventory[17309]%
  set bell8 %actor.wearing[17309]%
  if %bell1% || %bell2% || %bell3% || %bell4% || %bell5% || %bell6% || %bell7% || %bell8%
    mechoaround %actor% %actor.name% dances an ancient circle dance, calling to the great Spirits of the Canyon.
    msend %actor% You dance an ancient circle dance, calling to the great Spirits of the canyon.
    wait 1s
    msend %actor% %color%%self.name% begins to dance with you, complementing and accentuating your movement and rhythms.&0
    mechoaround %actor% %color%%self.name% begins to dance with %actor.name%, complementing and accentuating %hisher% movement and rhythms.&0
    wait 3s
    mecho %self.name% speaks in three voices:
    mecho %color%'You show proper respect for the Spirits of the Canyon and their children.  In turn, we will respect your position as Envoy and heed the call of the Ocean.'&0
    quest variable flood %actor.name% water3 1
    wait 2s
    mecho %color%%self.name% continues to dance, gradually uniting with the river.&0
    set water1 %actor.quest_variable[flood:water1]%
    set water2 %actor.quest_variable[flood:water2]%
    set water3 %actor.quest_variable[flood:water3]%
    set water4 %actor.quest_variable[flood:water4]%
    set water5 %actor.quest_variable[flood:water5]%
    set water6 %actor.quest_variable[flood:water6]%
    set water7 %actor.quest_variable[flood:water7]%
    set water8 %actor.quest_variable[flood:water8]%
    if %water1% && %water2% && %water3% && %water4% && %water5% && %water6% && %water7% && %water8%
      quest advance flood %actor.name%
      wait 1s
      msend %actor% &4&bYou have garnered the support of all the great waters!&0
    endif
    mpurge %self%
  else
    msend %actor% You begin to dance but %self.name% is unresponsive.
    mechoaround %actor% %actor.name% begins to dance but %self.name% is unresponsive.
    msend %actor% Something seems to be off about your preparations.
  endif
else
  return 0
endif
~
#39009
flood_frozen_death~
0 f 100
~
return 0
mecho &6&b%self.name% shatters into a thousand pieces!&0
set person %actor%
set i %actor.group_size%
if %i%
  set a 1
  unset person
  while %i% >= %a%
    set person %actor.group_member[%a%]%
    if %person.room% == %self.room%
        if %person.quest_stage[flood]% == 1
          quest variable flood %person.name% water6 1
          set envoy yes
        endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
elseif %person.quest_stage[flood]% == 1
  quest variable flood %person.name% water6 1
  set envoy yes
endif
if %envoy% == yes
  mecho A chilly voice says, &6&b'You have bested me, Envoy.  I acquiesce to your request.'&0
endif
set room %self.room%
set person %room.people%
while %person%
  if %person.quest_stage[flood]% == 1
    set water1 %person.quest_variable[flood:water1]%
    set water2 %person.quest_variable[flood:water2]%
    set water3 %person.quest_variable[flood:water3]%
    set water4 %person.quest_variable[flood:water4]%
    set water5 %person.quest_variable[flood:water5]%
    set water6 %person.quest_variable[flood:water6]%
    set water7 %person.quest_variable[flood:water7]%
    set water8 %person.quest_variable[flood:water8]%
    if %water1% && %water2% && %water3% && %water4% && %water5% && %water6% && %water7% && %water8%
      quest advance flood %person.name%
      msend %person.name% &4&bYou have garnered the support of all the great waters!&0
    endif
  endif
  set person %person.next_in_room%
done
mgoto 1100
~
#39010
flood_lady_status_checker~
0 d 100
status status? progress progress?~
wait 2
if %actor.quest_stage[flood]% == 1
  set fog The Blue-Fog
  set phoenix Phoenix Feather Hot Springs
  set falls Three-Falls River
  set green The Greengreen Sea
  set witch Sea's Lullaby
  set frost Frost Lake
  set black The Black Lake
  set kod The Dreaming River
  set water1 %actor.quest_variable[flood:water1]%
  set water2 %actor.quest_variable[flood:water2]%
  set water3 %actor.quest_variable[flood:water3]%
  set water4 %actor.quest_variable[flood:water4]%
  set water5 %actor.quest_variable[flood:water5]%
  set water6 %actor.quest_variable[flood:water6]%
  set water7 %actor.quest_variable[flood:water7]%
  set water8 %actor.quest_variable[flood:water8]%
  set item2 %actor.quest_variable[flood:item2]%
  set item3 %actor.quest_variable[flood:item3]%
  set item4 %actor.quest_variable[flood:item4]%
  set item6 %actor.quest_variable[flood:item6]%
  set item7 %actor.quest_variable[flood:item7]%
  say As my Envoy, rally the Great Waters of Ethilien.
  mecho   
  if %water1% || %water2% || %water3% || %water4% || %water5% || %water6% || %water7% || %water8%
    mecho You have rallied:
    if %water1%
      mecho - &4%fog%&0
    endif
    if %water2%
      mecho - &4%phoenix%&0
    endif
    if %water3%
      mecho - &4%falls%&0
    endif
    if %water4%
      mecho - &4%green%&0
    endif
    if %water5%
      mecho - &4%witch%&0
    endif
    if %water6%
      mecho - &4%frost%&0
    endif
    if %water7%
      mecho - &4%black%&0
    endif
    if %water8%
      mecho - &4%kod%&0
    endif
    mecho   
  endif 
  * list items to be returned
  mecho You must still convince:
  if !%water1%
    mecho - &4&b%fog%&0
    mecho   
  endif
  if !%water2%
    mecho - &4&b%phoenix%&0
    If %item2% == 1
      mecho &0    Bring it %get.obj_shortdesc[58401]% to heat its springs.
    endif
    mecho   
  endif
  if !%water3%
    mecho - &4&b%falls%&0
    If %item3% == 1
      mecho &0    Find a bell and dance for them.
    endif
    mecho    
  endif
  if !%water4%
    mecho - &4&b%green%&0
    if %item4% == 1
      mecho &0    Feed her as many different foods as you can until she is full.
    endif
    mecho   
  endif
  if !%water5%
    mecho - &4&b%witch%&0
    mecho   
  endif
  if !%water6%
    mecho - &4&b%frost%&0
    If %item6% == 1
      mecho &0    Force her to join the cause.
    endif
    mecho   
  endif
  if !%water7%
    mecho - &4&b%black%&0
    if %item7% == 1
      mecho &0    Bring it an eternal light to swallow into its blackness.
    endif
    mecho   
  endif
  if !%water8%
    mecho - &4&b%kod%&0
    mecho   
  endif
  mecho %self.name% says, 'Tell them: &4&bthe Arabel Ocean calls for aid&0.'
  mecho   
  mecho %self.name% says, 'If you lost the Heart, say &4&bI lost the heart&0.'
elseif %actor.quest_stage[flood]% ==2
  say Return my heart to me!
  mecho   
  mecho %self.name% says, 'If you lost the Heart, say &4&bI lost the heart&0.'
elseif %actor.has_completed[flood]%
  say I have already enacted my revenge, Envoy.
elseif !%actor.quest_stage[flood]%
  say You are not yet my Envoy.
endif
~
#39011
flood_spirit_load~
0 o 100
~
m_run_room_trig 39012
~
#39012
flood_south_gate~
2 a 100
~
wdoor 39187 south flags f
wdoor 39187 south description A sturdy gate holds back the sea.
~
#39013
flood_south_destroyed~
2 a 100
~
wdoor 39187 south flags f
wdoor 39187 south description The ruins of a decimated settlement lay beyond.
~
#39014
**UNUSED**~
0 c 100
d~
return 0
~
#39015
flood_frozen_fight~
0 k 33
~
wait 1s
set cast %random.3%
if %cast% == 1
  c 'iceball' %actor%
elseif %cast% == 2
  c 'chain lightning'
elseif %cast% == 3
  c 'freeze' %actor%
endif
~
#39016
flood_lady_heart_replacement~
0 d 0
I lost the heart~
wait 2
if %actor.quest_stage[flood]%
  scream
  wait 2
  mecho %self.name% shrieks, 'HOW DARE YOU!!!'
  wait 1s
  mdamage %actor% 400
  msend %actor% %self.name% fills your lungs with water! (&4&b%damdone%&0)
  mechoaround %actor% %self.name% fills %actor.name%'s lungs with water! (&4&b%damdone%&0)
  wait 2s
  emote holds her hands in front of her.
  wait 1s
  mecho Water flows up %self.name%'s body and swirls around her arms.
  wait 1s
  mecho The water coalesces into a shimmering jewel in the Lady's outstreched hands.
  wait 1s
  mload obj 39000
  give heart %actor%
  say Do not make this mistake again, Envoy.
endif
~
$~
