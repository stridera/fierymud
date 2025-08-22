#48900
doom_minstrel_play~
0 b 5
~
if %self.wearing[48924]%
remove instrument-case
open instrument-case
get mandolin instrument-case
hold mandolin
sit
wait 2s
mecho %self.name% begins to play a mandolin softly.
wait 2s
mecho The music builds into a swelling crescendo.
masound Beautiful music floats in from somewhere nearby.
wait 4s
mecho The music begins to fade as %self.name% wraps up the piece.
masound The music fades away.
wait 2s
mecho %self.name% stops playing.
stand
bow
wait 2s
put mandolin instrument-case
close instrument-case
wear instrument-case
end
~
#48901
keep-maidservant~
0 g 100
~
set chance %random.10%
if %chance% > 5
   wait 1s
   mecho %self.name% says, 'Lokari bought me out of slavery and freed me,' more
   mecho &0to herself than you.
   wait 1s
   say I won't let you harm him.
endif
~
#48902
lokari init~
0 o 100
~
mgoto 1100
if %get.mob_count[48915]%
   mat maid-rogue mteleport maid-rogue 48980
   mat 48980 mheal maid-rogue 32000
else
   mload mob 48915
   if %get.obj_count[48926]%
      mforce maid-rogue mload obj 1012
      mforce maid-rogue wield thin-dagger
   else
      mforce maid-rogue mload obj 48926
      mforce maid-rogue wield wrist-dagger
   endif
   mteleport maid-rogue 48980
endif
if %get.mob_count[48922]%
   mat maid-sorcerer mteleport maid-sorcerer 48980
   mat 48980 mheal maid-sorcerer 32000
else
   mload mob 48922
   mteleport maid-sorcerer 48980
endif
if %get.mob_count[48923]%
   mat maid-cleric mteleport maid-cleric 48980
   mat 48980 mheal maid-cleric 32000
else
   mload mob 48923
   mteleport maid-cleric 48980
endif
mgoto 48980
~
#48903
lokari fight~
0 k 100
~
if !(%self.aff_flagged[STONE]%)
   stone
endif
rescue maid-rogue
rescue maid-sorcerer
rescue maid-cleric
if %actor.vnum% != -1
   if %get.mob_count[48915]%
      mforce maid-rogue assist lok
   endif
   if %get.mob_count[48922]%
      mforce maid-sorcerer assist lok
   endif
   if %get.mob_count[48923]%
      mforce maid-cleric assist lok
   endif
endif
set mode %random.100%
wait 1s
if %mode% < 16
   * 15% chance of echoes of justice, 300-400 damage
   m_run_room_trig 48905
elseif %mode% < 31
   * 15% chance to throw a player into another player
   m_run_room_trig 48906
elseif %mode% < 51
   * 20% chance to hear praises, heal 700-1000
   eval amount 500 + %random.300%
   mecho &3&bA chorus of praise echoes through the cell, bolstering Lokari's pride!&0
   mheal lokari %amount%
elseif (0) && (%mode% < 40) && %actor% && (%actor.vnum% != -1)
   * Temporarily disabled until I figure out wth mdamage
   * doesn't work on mobs.
   * 5% chance for foreknowledge: mob instadeath
   emote waves a hand in the air.
   mechoaround %actor% &6Ghostly images appear in the air, foretelling %actor.name%'s death!&0
   msend %actor% &6Ghostly images appear in the air, foretelling your death!&0
   wait 1s
   if %actor% && (%actor.room% != %self.room%)
      * Bring back the victim if he/she has left the room
      mteleport %actor% %self.room%
   endif
   if %actor%
      mecho &6&bLokari's prophecy comes to pass!&0
      mdamage %actor% 5000
   endif
endif
~
#48904
lokari hitprcnt 50~
0 l 50
50~
if %init% == 1
   m_run_room_trig 48913
   set init 2
   global init
endif
~
#48905
lokari echoes of justice~
2 a 100
~
set majorglobe MAJOR_GLOBE
wecho Lokari starts casting &3&b'echoes of justice'&0...
wait 1s
wecho Lokari utters the words, 'sdorj lp kandiso'.
wecho &2Lokari's justice spreads through the room, striking down trespassers!&0
set person %self.people%
while %person%
   set next %person.next_in_room%
   if %person% && ((%person.vnum% < 48900) || (%person.vnum% > 48999)) && (%person.level% < 100)
      eval damage 150 + %random.100%
      if %person.aff_flagged[SANCT]%
         eval damage %damage% / 2
      endif
      if %person.aff_flagged[STONE]%
         eval damage %damage% / 2
      endif
      * Chance for critical hit
      set variant %random.15%
      if %variant% == 1
         eval damage %damage% / 2
      elseif %variant% == 15
         eval damage %damage% * 2
      endif
      set globed %person.aff_flagged[MAJOR_GLOBE]%
      if %globed%
         eval damage %damage% / 2
         wechoaround %person% &1&bThe shimmering globe around %person.name%'s body wavers under &0&3Lokari's justice&1&b.&0 (&4%damage%&0)
         wsend %person% &1&bYou shiver as &0&3Lokari's justice&1&b rips through the shimmering globe around your body, striking you!&0 (&1&b%damage%&0)
      else
         wechoaround %person% &3%person.name% slumps under the knowledge of %person.p% own wrongdoing.&0 (&4%damage%&0)
         wsend %person% &3You slump under the knowledge of your own wrongdoing.&0 (&1&b%damage%&0)
      endif
      wdamage %person% %damage%
   endif
   set person %next%
done
~
#48906
lokari throw player~
2 a 100
~
set max_tries 6
while %max_tries% > 0
   set victim1 %random.char%
   if ((%victim1.vnum% < 48900) || (%victim1.vnum% > 48999)) && (%victim1.level% < 100)
      set max_tries 0
   endif
   eval max_tries %max_tries% - 1
done
if %max_tries% != -1
   halt
endif
set max_tries 10
while %max_tries% > 0
   set victim2 %random.char%
   if ((%victim2.vnum% < 48900) || (%victim2.vnum% > 48999)) && (%victim2.level% < 100) && (%victim1.name% != %victim2.name%)
      set max_tries 0
   endif
   eval max_tries %max_tries% - 1
done
if %max_tries% != -1
   halt
endif
eval damage1 75 + %random.50%
eval damage2 75 + %random.50%
if %victim1.aff_flagged[SANCT]%
   * Half damage for sanc
   eval damage1 %damage1% / 2
endif
if %victim1.aff_flagged[STONE]%
   * Another 50 damage for target2 if target1 has stoneskin
   eval damage2 %damage2% + 50
endif
* Chance for critical hit
set variant %random.15%
if %variant% == 1
   eval damage1 %damage1% / 2
elseif %variant% == 15
   eval damage1 %damage1% * 2
endif
if %victim2.aff_flagged[SANCT]%
   * Half damage for sanc
   eval damage2 %damage2% / 2
endif
if %victim2.aff_flagged[STONE]%
   * Another 50 damage for target1 if target2 has stoneskin
   eval damage1 %damage1% + 50
endif
* Chance for critical hit
set variant %random.15%
if %variant% == 1
   eval damage2 %damage2% / 2
elseif %variant% == 15
   eval damage2 %damage2% * 2
endif
wechoaround %victim1% Lokari waves a hand at %victim1.name%, picking %victim1.o% up into the air!
wsend %victim1% Lokari waves a hand at you, picking you up into the air!
wteleport %victim2% 1100
wechoaround %victim1% Lokari smashes %victim1.name% into %victim2.name% with a flick of his wrist! (&4%damage1%&0) (&4%damage2%&0)
wsend %victim1% Lokari smashes you into %victim2.name% with a flick of his wrist! (&1&b%damage1%&0) (&4%damage2%&0)
wsend %victim2% Lokari smashes %victim1.name% into you with a flick of his wrist! (&1&b%damage2%&0) (&4%damage1%&0)
wteleport %victim2% 48980
wdamage %victim1% %damage1%
wdamage %victim2% %damage2%
~
#48907
lokari death~
0 f 100
~
return 0
mecho Lokari's form dissipates, banished from this realm.
mecho %get.obj_shortdesc[48914]% falls from Lokari's hand as he disappears.
mjunk glass-shard
m_run_room_trig 48916
set i %actor.group_size%
if %i%
  set a 1
  while %i% >= %a%
    set person %actor.group_member[%a%]%
    if %person.room% == %self.room%
      if %person.quest_stage[shift_corpse]% == 2
        mecho &b&9Swirling energy as black as the moonless night slithers from %get.obj_shortdesc[6228]%.&b&9
        msend %person% &b&9It fills you with limitless arcane power as it forces its way into your body!&0
        msend %person% &b&9Your awareness of death now stretches beyond the boundaries of the planes themselves!&0
        mechoaround %person% &b&9It forces its way into %person.name%'s body!&0
        quest complete shift_corpse %person.name%
        mskillset %person.name% shift corpse
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
elseif %actor.quest_stage[shift_corpse]% == 2
  mecho &b&9Swirling energy as black as the moonless night slithers from %get.obj_shortdesc[6228]%.&b&9
  msend %actor% &b&9It fills you with limitless arcane power as it forces its way into your body!&0
  msend %actor% &b&9Your awareness of death now stretches beyond the boundaries of planes themselves!&0
  mechoaround %actor% &b&9It forces its way into %actor.name%'s body!&0
  quest complete shift_corpse %actor.name%
  mskillset %actor.name% shift corpse
endif
mgoto 1100
~
#48908
maid-rogue fight~
0 k 100
~
if (%self.room% != 48980)
   mgoto 48980
endif
if (!%get.mob_count[48901]%) &!(%self.aff_flagged[BLUR]%)
   emote keens for her lost master, lashing out in an uncontrolled frenzy!
   mcast blur %self% 100
endif
if !(%self.worn[wield]%)
   wield wrist-dagger
endif
set chance %random.10%
if %chance% < 7
   backstab
endif
~
#48909
maid-sorcerer fight~
0 k 100
~
if (%self.room% != 48980)
   mgoto 48980
endif
if (!%get.mob_count[48901]%) && !(%self.aff_flagged[BLUR]%)
   emote keens for her lost master, lashing out in an uncontrolled frenzy!
   mcast blur %self% 100
endif
if %stone%
   cast 'stone skin' lokari
   unset stone
else
   set chance %random.10%
   if %chance% > 7
      cast 'chain lightning'
   endif
endif
~
#48910
maid-sorcerer stone~
0 c 100
stone~
if %actor.vnum% == 48901
   set stone 1
   global stone
else
   return 0
endif
~
#48911
maid-cleric fight~
0 k 100
~
if (%self.room% != 48980)
   mgoto 48980
endif
if (!%get.mob_count[48901]%) && !(%self.aff_flagged[BLUR]%)
   emote keens for her lost master, lashing out in an uncontrolled frenzy!
   mcast blur %self% 100
endif
set chance %random.10%
if %chance% > 7
   m_run_room_trig 48912
endif
~
#48912
maid-cleric spells~
2 a 100
~
* Clear stop-casting message
if %stop_casting%
  unset stop_casting 1
endif
if !%casting%
  set casting 1
  global casting
  set heal_chance %random.10%
  if (%healing% && (%heal_chance% > 2)) || (%heal_chance% == 10)
    wecho A maid in waiting starts casting &3&b'group heal'&0...
    wait 5s
    if %stop_casting%
      * Stop casting if we've been passed a stop-casting message
      unset stop_casting
      unset casting
      halt
    endif
    wecho A maid in waiting completes her spell...
    wecho A maid in waiting utters the words, 'craes poir'.
    set actor %random.char%
    if %actor%
      * Check to see if Lokari and his three maids exist, and heal each of them
      if %get.mob_count[48901]%
      wheal lokari 450
      endif
      if %get.mob_count[48915]%
      wheal maid-rogue 450
      endif
      if %get.mob_count[48922]%
      wheal maid-sorcerer 450
      endif
      if %get.mob_count[48923]%
      wheal maid-cleric 450
      endif
    endif
  else
    * Alternate between good and evil spells
    if %spell% == good
      set spell evil
    else
      set spell good
    endif
    global spell
    if %spell% == good
      wecho A maid in waiting starts casting &3&b'consecration'&0...
    else
      wecho A maid in waiting starts casting &3&b'sacrilege'&0...
    endif
    wait 2s
    if %stop_casting%
      * Stop casting if we've been passed a stop-casting message
      unset stop_casting
      unset casting
      halt
    endif
    if %spell% == good
      wecho A maid in waiting utters the words, 'parl xafm'.
      wecho &7&bA maid in waiting speaks a word of divine consecration!&0
    else
      wecho A maid in waiting utters the words, 'ebparl xafm'.
      wecho &b&9A maid in waiting speaks a word of demonic sacrilege!&0
    endif
    set person %self.people%
    while %person%
      set next %person.next_in_room%
      if ((%person.vnum% < 48900) || (%person.vnum% > 48999)) && (%person.level% < 100)
        if (%spell% == good) && (%person.align% < -349)
          set globed %person.aff_flagged[MAJOR_GLOBE]%
          if %globed%
            * No damage for major globe
            wechoaround %person% &1&bThe shimmering globe around %person.name%'s body flares as the maid's spell flows around it.&0
            wsend %person% &1&bThe shimmering globe around your body flares as the spell flows around it.&0
        else
          eval damage 210 + %random.20%
          if %person.aff_flagged[SANCT]%
            eval damage %damage% / 2
          endif
          if %person.aff_flagged[STONE]%
            eval damage %damage% / 2
          endif
          * Chance for critical hit
          set variant %random.20%
          if %variant% == 1
            eval damage %damage% / 2
          elseif %variant% == 20
            eval damage %damage% * 2
          endif
          wechoaround %person% &7&b%person.name% cries out in anguish upon hearing the maid's blessing!&0 (&4%damage%&0)
          wsend %person% &7&bYou cry out in anguish upon hearing the maid's blessing!&0 (&1&b%damage%&0)
          wdamage %person% %damage%
        endif
      elseif (%spell% == evil) && (%person.align% > 349)
        if %person.flags% /= MAJOR_GLOBE
          * No damage for major globe
          wechoaround %person% &1&bThe shimmering globe around %person.name%'s body flares as the maid's spell flows around it.&0
          wsend %person% &1&bThe shimmering globe around your body flares as the spell flows around it.&0
        else
        eval damage 210 + %random.20%
        if %person.aff_flagged[SANCT]%
          eval damage %damage% / 2
        endif
        if %person.aff_flagged[STONE]%
          eval damage %damage% / 2
        endif
        * Chance for critical hit
        set variant %random.20%
        if %variant% == 1
          eval damage %damage% / 2
        elseif %variant% == 20
          eval damage %damage% * 2
        endif
        wechoaround %person% &b&9%person.name% screams in torment upon hearing the maid's curse!&0 (&4%damage%&0)
        wsend %person% &b&9You scream in torment upon hearing the maid's curse!&0 (&1&b%damage%&0)
        wdamage %person% %damage%
        endif
      endif
    endif
    set person %next%
  done
  endif
  wait 2s
  unset casting
endif
~
#48913
maid-cleric healing~
2 a 100
~
set healing 1
global healing
~
#48914
maid death~
0 f 100
~
if %self.room% != 48980
   mgoto 48980
endif
m_run_room_trig 48915
~
#48915
maid death heal lokari~
2 a 100
~
set actor %random.char%
if %actor% && %get.mob_count[48901]%
   wecho Lokari absorbs the maid's spirit as it leaves her body.
   wat 48915 wheal lokari 2000
endif
wait 1
if %actor% && (%get.mob_count[48923]% == 0)
   set stop_casting 1
   global stop_casting
endif
~
#48916
lokari death load~
2 a 100
~
wload obj 48914
~
#48917
maid return~
0 b 100
~
if %self.room% != 48980
   mgoto 48980
endif
~
#48920
velocity init~
0 o 100
~
mat 1100 rem all
mat 1100 wear vest
get indigo-blade
get indigo-blade
mjunk indigo-blade
mjunk indigo-blade
mload obj 48003
mload obj 48003
mload obj 23826
mload obj 23826
mat 1100 recite bright-red-scroll 1.indigo-blade
mat 1100 recite bright-red-scroll 2.indigo-blade
wield indigo-blade
wield indigo-blade
~
#48921
velocity fight~
0 k 100
~
if !(%self.worn[wield]%)
   get indigo-blade
   wield indigo-blade
endif
if !(%self.worn[wield2]%)
   get indigo-blade
   wield indigo-blade
endif
~
#48922
nikita fight~
0 k 100
~
set mode %random.10%
if %mode% > 8
   * 20% chance for lay hands
   eval amount 1000 + %random.40%
   mecho The hands of Nikita glow as she lays them on herself. (&3&b%amount%&0)
   mcast 'full heal' %self% 100
   * Mini-wait so the trigger only goes off once per round
   wait 1
elseif %mode% > 7
   * 10% chance for hitall
   wait 1s
   hitall
elseif %mode% > 5
   * 20% chance for holy word
   wait 1s
   cast 'holy word'
endif
if !(%self.worn[2hwield]%)
   get rahmat
   wield rahmat
endif
~
#48923
sunchild speech~
0 g 100
~
set chance %random.10%
if %chance% > 5
   wait 1s
   mecho %self.name% says in a dulcet voice, 'The entire power of the sun is behind me.
   mecho &0You might as well give up.  I won't let you get to my mistress's lover.'
endif
~
#48924
wiseman speech~
0 g 100
~
set chance %random.10%
if %chance% > 5
   wait 1s
   grumble
   emote protests, 'I'm not a well man.'
   emote says slowly, 'I shouldn't have to be doing this.'
endif
~
#48925
dark servant fight~
0 k 100
~
if (%actor.vnum% >= 48900) && (%actor.vnum% <= 48999)
  * Stop combat if fighting another doom mobile
  wait 1
  mat 1100 mheal dark-servant 1000
endif
wait 2s
set chance %random.10%
if %chance% > 3
  * Short-circuit
  halt
elseif %chance% == 1
  * 10% chance to hitall
  hitall
  halt
elseif (%chance% == 2) && (%self.vnum% == 48909)
  * 10% chance to switch to an assassin, if this is the warrior dark servant
  set max_tries 9
  while %max_tries% > 0
    set victim %random.char%
    if (%victim.class% == Assassin) || (%victim.class% == Thief)
      set max_tries 0
    endif
    eval max_tries %max_tries% - 1
  done
  if (%max_tries% == -1) && (%victim.vnum% == -1)
    kill %victim.name%
    halt
  endif
  halt
else
  * If warrior servant, 10% chance to chill someone, stealing their hp
  * If nonwarrior servant, 20% chance
  * Attempt to get a player no more than max_tries times
  set max_tries 5
  while %max_tries% > 0
    set victim %random.char%
    if %victim% &((%victim.vnum% < 48900) || (%victim.vnum% > 48999))
      set max_tries 0
    endif
    eval max_tries %max_tries% - 1
  done
  if %max_tries% == -1
    if (%victim.vnum% > 48900) && (%victim.vnum% < 48999) &%actor%
      set victim %actor%
    else
      halt
    endif
  else
    halt
  endif
  set casters Sorcerer Cryomancer Pyromancer Necromancer Cleric Priest Diabolist Druid Conjurer Shaman
  * Be nice to casters
  if %casters% /= %victim.class%
    eval damage 200 + %random.30%
  else
    eval damage 380 + %random.30%
  endif
  mdamage %victim% %damage% heal
  if %damdone% == 0
    mechoaround %victim% %self.name% reaches a &b&9shadowy&0 limb towards %victim.name%, but cannot draw out any life. (&4%damdone%&0)
    msend %victim% %self.name% reaches a &b&9shadowy&0 limb towards you, but cannot draw out any life. (&1&b%damdone%&0)
  else
    mechoaround %victim% %self.name% reaches a &b&9shadowy&0 limb towards %victim.name%, &4&bchilling&0 %victim.o% to the &7&bbone&0! (&4%damdone%&0)
    msend %victim% %self.name% reaches a &b&9shadowy&0 limb towards you, &4&bchilling&0 you to the &7&bbone&0! (&1&b%damdone%&0)
  end
  eval amount %damdone% * 2
  mheal dark-servant %amount%
endif
~
#48926
sunchild fight~
0 k 100
~
set chance %random.10%
if (%actor.vnum% >= 48900) && (%actor.vnum% <= 48999)
   * Stop combat if fighting another doom mobile
   wait 1
   mat 1100 mheal sunchild 1000
elseif (%chance% <= 2)
   wait 2s
   mecho &3A Sunchild &bflares brightly&0&3, casting rays of &7light&3 everywhere!&0
   set room %self.room%
   set person %room.people%
   while %person%
      if %person.vnum% == -1
         mcast sunray %person% 100
      endif
   set person %person.next_in_room%
   done
endif
~
#48927
jann warrior fight~
0 k 100
~
if (%actor.vnum% >= 48900) && (%actor.vnum% <= 48999)
   * Stop combat if fighting another doom mobile
   wait 1
   mat 1100 mheal jann 1000
endif
wait 2s
set action %random.10%
if %action% > 9
   * 10% chance
   hitall
elseif %action% > 7
   * 20% chance
   kick
elseif %action% > 5
   * 10% chance to dispel magic
   if %actor.vnum% == -1
      if %self.vnum% == 48911
         mload obj 48928
      else
         mload obj 48927
      endif
      recite dispel-scroll %actor.name%
      mjunk dispel-scroll
   else
      roar
   endif
endif
~
#48928
keep servant fight~
0 k 100
~
if (%actor.vnum% >= 48900) && (%actor.vnum% <= 48999)
   * Stop combat if fighting another doom mobile
   wait 1
   mat 1100 mheal keep-servant 1000
endif
~
#48929
gardener mystic fight~
0 k 100
~
if (%actor.vnum% >= 48900) && (%actor.vnum% <= 48999)
   * Stop combat if fighting another doom mobile
   wait 1
   mat 1100 mheal gardener-mystic 1000
endif
~
#48930
lokari's wiseman fight~
0 k 100
~
if (%actor.vnum% >= 48900) && (%actor.vnum% <= 48999)
   * Stop combat if fighting another doom mobile
   wait 1
   mat 1100 mheal lokaris-wiseman 1000
endif
~
#48931
wandering minstrel fight~
0 k 100
~
if (%actor.vnum% >= 48900) &(%actor.vnum% <= 48999)
   * Stop combat if fighting another doom mobile
   wait 1
   mat 1100 mheal wandering-minstrel 1000
endif
if %self.eff_flagged[silence]%
  halt
endif
if %self.level% >= 70
   if (%now2% && (%time.stamp% - %now2% >= 5)) || !%now2%
      if %actor.group_size% > 1
         set room %self.room%
         set person %room.people%
         while %person%
            if %person.vnum% == -1
               if !%self.has_spell[terror]% && !%self.has_spell[ballad of tears]%
                  mperform 'ballad of tears' %person% %self.level%
                  set now2 %time.stamp%
                  global now2
                  halt
               endif
            endif
            set person %person.next_in_room%
         done
      endif
   endif
elseif %self.level% >= 10
   if (%now% && (%time.stamp% - %now% >= 5)) || !%now%
      if !%actor.has_spell[terror]% && !%actor.has_spell[ballad of tears]%
         mperform terror %actor% %self.level%
         set now %time.stamp%
         global now
      endif
   endif
endif
~
#48932
severan fight~
0 k 100
~
if %self.room% != 48960
   emote emits a loud wail, and returns to the site where he is bound.
   mgoto 48960
endif
if (%actor.vnum% >= 48900) && (%actor.vnum% <= 48999)
   * Stop combat if fighting another doom mobile
   wait 1
   mat 1100 mheal severan 1000
endif
wait 1s
set action %random.10%
if %action% > 9
   * 10% chance
   wait 1s
   hitall
elseif %action% > 6
   * 20% chance for Shockwave area attack, 150-300 damage
   m_run_room_trig 48933
endif
* 70% chance to do nothing!
~
#48933
severan shockwave~
2 a 100
~
wecho &7&bThe white aura around Severan's body intensifies, increasing in brightness.&0
wait 2s
wecho &7&bA powerful shockwave leaps off Severan's body as the aura flares wildly!&0
set casters Sorcerer Necromancer Cryomancer Pyromancer Cleric Druid Diabolist Priest Shaman Conjurer
set person %self.people%
while %person%
  set next %person.next_in_room%
  if ((%person.vnum% < 48900) || (%person.vnum% > 48999)) &(%person.level% < 100)
    if %casters% /= %person.class%
      eval damage 100 + %random.50%
    else
      eval damage 250 + %random.50%
    endif
    if %person.aff_flagged[SANCT]%
      eval damage %damage% / 2
    endif
    if %person.aff_flagged[STONE]%
      eval damage %damage% / 2
    endif
    * Chance for critical hit
    set variant %random.15%
    if %variant% == 1
      eval damage %damage% / 2
    elseif %variant% == 15
      eval damage %damage% * 2
    endif
    * Halve damage AGAIN for major globe
    set globed %person.aff_flagged[MAJOR_GLOBE]%
    if %globed%
      eval damage %damage% / 2
    end
    wdamage %person% %damage% crush
    if %damdone% == 0
      wechoaround %person% &7&bThe blast passes through %person.name%, causing no damage.&0
      wsend %person% &7&bThe blast passes through you harmlessly.&0
    elseif %globed%
      wechoaround %person% &1&bThe shimmering globe around %person.name%'s body wavers as the blast overwhelms it!&0 (&4%damdone%&0)
      wsend %person% &1&bThe &7blast&1 passes through the shimmering globe around your body, striking you!&0 (&1&b%damdone%&0)
    else
      wechoaround %person% &7&bThe blast strikes %person.name% violently, tearing at %person.p% flesh!&0 (&4%damdone%&0)
      wsend %person% &7&bThe blast strikes you violently, rending your flesh!&0 (&1&b%damdone%&0)
    endif
  endif
  set person %next%
done
~
$~
