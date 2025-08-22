#2200
Balor_death~
0 f 100
~
return 0
mecho As %actor.name% slays the Balor, the demon splits open with a bright crack!
m_run_room_trig 2201
mjunk balor-sword
mjunk flame-whip
~
#2201
Balor_death_damage~
2 a 100
~
set person %self.people%
while %person%
   set next %person.next_in_room%
   if %person.vnum% != 2216
      eval damage 100 + %random.50%
      wdamage %person% %damage% fire
      wsend %person% The Balor explodes in a &3&bblinding&0 flash, scorching the area! (&1&b%damdone%&0)
   endif
   set person %next%
done
wait 2s
wecho &7&bAs the smoke &7subsides, a stairway down appears.&0
wdoor 2215 down room 2216
wdoor 2215 down description Down the stairs, there is a faint sound of cheering...
~
#2202
Acid_swamp_damage~
2 b 50
~
set count %self.people[count]%
if %count% > 3
   set count 3
end
while %count% > 0
   eval victim %random.char%
   if %victim.vnum% != 2213
      eval damage 50 + %random.30%
      eval which %random.2%
      switch %which%
         case 1
            wdamage %victim% %damage% poison
            wsend %victim% You accidentally inhale some noxious gases!  Oops! (&1&b%damdone%&0)
            wechoaround %victim% %victim.name% suddenly chokes, coughing on the acrid swamp gases. (&4&b%damdone%&0)
            break
         case 2
            wdamage %victim% %damage% acid
            wsend %victim% A large bubble pops in the waters, spewing acid on you! (&1&b%damdone%&0)
            wechoaround %victim% A large bubble pops in the waters, spewing burning acid on %victim.name%! (&4&b%damdone%&0)
            break
         default
      done
   end
   eval count %count% - 1
   wait 3s
done
~
#2203
Fields_of_fire_damage~
2 b 100
~
set count %self.people[count]%
if %count% > 3
   set count 3
endif
while %count% > 0
   eval victim %random.char%
   if %victim.vnum% != 2214
      eval damage 40 + %random.20%
      eval which %random.3%
      eval damage %damage% * 2
      switch %which%
         case 1
            wdamage %victim% %damage% fire
            wsend %victim% Scorching &1flames&0 burst out of a nearby crack, scalding your skin! (&1&b%damdone%&0)
            wechoaround %victim% White-hot flames explode out of a crack in the ground right next to %victim.name%! (&4&b%damdone%&0)
            break
         case 2
            wdamage %victim% %damage% fire
            wsend %victim% Bubbling, hot lava roils out of the ground onto your feet! (&1&b%damdone%&0)
            wechoaround %victim% Burning lava bubbles out of the ground onto %victim.name%'s feet! Ouch! (&4&b%damdone%&0)
            break
         case 3
            wdamage %victim% %damage% fire
            wsend %victim% The sharp leaves of a lava bush cut at your legs! (&1&b%damdone%&0)
            wechoaround %victim% %victim.name% yelps as a lava bush's sharp leaves slice into %victim.p% legs. (&4&b%damdone%&0)
            break
         default
            eval damage %damage% * 1.5
            wdamage %victim% %damage% fire
            wsend %victim% Some of your hair spontaneously catches fire, burning your scalp! (&1&b%damdone%&0)
            wechoaround %victim% A caustic odor fills the air as %victim.name%'s hair suddenly bursts into flame. (&4&b%damdone%&0)
      done
   endif
   eval count %count% - 1
   wait 3s
done
~
#2204
Dark_ice_sea_damage~
2 b 100
~
set count %self.people[count]%
if %count% > 3
   set count 3
end
while %count% > 0
   eval victim %random.char%
   if %victim.vnum% != 2215
      eval damage 20 + %random.15%
      eval which %random.2%
      switch %which%
         case 1
            * Less damage for flying people
            if !(%victim.aff_flagged[fly]%)
               eval damage %damage% * 3
            end
            wdamage %victim% %damage% slash
            wsend %victim% A sharp-edged iceberg suddenly comes rushing out of the water and into you! (&1&b%damdone%&0)
            wechoaround %victim% A twisted iceberg suddely rushes out of the water, striking %victim.name%! (&4&b%damdone%&0)
            break
         default
            * More damage for people with stoneskin
            if !(%victim.aff_flagged[stone]%)
               eval damage %damage% * 3
               wdamage %victim% %damage% cold
               wsend %victim% Ice begins to form around your stony skin, dragging you downwards! (&1&b%damdone%&0)
               wechoaround %victim% Frost starts forming all over the skin of %victim.name%, freezing %victim.p% joints! (&4&b%damdone%&0)
            else
               eval damage %damage% * 2
               wdamage %victim% %damage% cold
               wsend %victim% The dark waters around you begin to chill your bones. (&1&b%damdone%&0)
               wechoaround %victim% %victim.name% grows a little blue and begins to shiver. (&4&b%damdone%&0)
            end
      done
   end
   eval count %count% - 1
   wait 3s
done
~
#2205
pit-fiend-death-exit~
0 f 100
~
mecho &3The ground shakes violently!&0
m_run_room_trig 2206
~
#2206
pit-fiend-room-exit~
2 a 100
~
wait 2s
wecho &3The trembling subsides and a &1&bfi&3er&1y pit&0 &3appears, leading down.&0
wdoor 2210 down room 2211
wdoor 2210 down description A &1&bburning&0 hole leads down.
~
#2207
yugoloth-death-exits~
0 f 100
~
mecho &2A horrid squelching can be heard as the swamp begins to drain!&0
m_run_room_trig 2208
~
#2208
swamp-exit-yugoloth~
2 a 100
~
wait 2s
wecho &2The swamp drains, leaving a &bmoss&0&2-covered staircase leading down.&0
wdoor 2212 down room 2213
wdoor 2212 down description &2A slippery staircase leads down.&0
~
#2209
Incubus_Succubus_Death_Exit~
0 f 100
~
if (%get.mob_count[2210]% + %get.mob_count[2211]% == 1)
    m_run_room_trig 2218
endif
~
#2210
Incubus_Succubus_Load~
0 g 100
~
if !%done%
wait 2s
   say So you wish to play do you?
   wait 2s
   cackle
   wait 2s
   mload mob 2211
   mload mob 2211
   mload mob 2211
   mload mob 2211
   mecho Sounds of screetching can be heard from all around as mounds of cloth fly in from all directions.
   set done 1
   global done
else
   wait 2s
   say Back again are you?!
   growl
end
~
#2211
Succubus_Emot_Damage~
2 a 100
~
set target 0
set loop 0
while (%target% == 0)
   set dice %random.50%
   eval damage 75 + %dice%
   set victim %random.char%
   if ((%victim.vnum% != -1) || (%victim.sex% != Male))
      if %loop% > 10
         set target 1
      else
         eval loop %loop% + 1
      endif
   else
      set target 1
      set message %random.4%
      switch %message%
         case 1
            wechoaround %victim% A Succubus reaches in and fondles %victim.name% gently. (&4%damage%&0)
            wsend %victim% A Succubus reaches in and fondles you gently. (&1&b%damage%&0)
            break
         case 2
            wechoaround %victim% A Succubus kisses %victim.name% passionately... (&4%damage%&0)
            wsend %victim% A Succubus kisses you passionately, it seems to last forever... (&1&b%damage%&0)
            break
         case 3
            wechoaround %victim% A Succubus gropes at %victim.name%. How inappropriate! (&4%damage%&0)
            wsend %victim% A Succubus gropes you wildly! Boy does that feel good! (&1&b%damage%&0)
            break
         default
            wechoaround %victim% A Succubus strokes %victim.name%'s inner thigh. (&4%damage%&0)
            wsend %victim% A Succubus strokes your inner thigh. She's getting frisky! (&1&b%damage%&0)
            break
      done
      wdamage %victim% %damage%
      set target 1
   endif
done
~
#2212
Succubus_Call_Emot~
0 k 20
~
wait 2s
m_run_room_trig 2211
~
#2213
Incubus_Emot_Damage~
2 a 100
~
set dice %random.50%
eval damage 100 + %dice%
set victim %random.char%
if %victim.vnum% == -1
   if %victim.sex% == Female
      wechoaround %victim% The Incubus grabs hold of %victim.name% and gives her a passionate kiss. (&4%damage%&0)
      wsend %victim% The Incubus grabs you tenderly and gives you an amazingly passionate kiss... (&1&b%damage%&0)
      wdamage %victim% %damage%
   endif
endif
~
#2214
Succubus_Call_Emot~
0 k 40
~
wait 2s
m_run_room_trig 2213
~
#2215
Random_Illness_Trigger~
2 b 30
~
if %self.people%
   set victim %random.char%
   if %victim.vnum% == -1
      set message %random.3%
      switch %message%
         case 1
            wechoaround %victim% A tiny, vicious bug buzzes around %victim.name%, landing on %victim.p% arm and biting %victim.o%.
            wsend %victim% A tiny, vicious bug buzzes around you before landing on your arm and biting you!
            wait 15s
            wechoaround %victim% %victim.name% begins to look pale and starts shivering. (&450&0)
            wsend %victim% You turn pale and shiver as nausea begins to set in. (&1&b50&0)
            wdamage %victim% 50
            wait 15s
            wechoaround %victim% %victim.name% falls over in pain as %victim.n% begins to purge his stomach (&4100&0)
            wsend %victim% You fall over in pain as you being to vomit all over the floor. (&1&b100&0)
            wdamage %victim% 100
            break
         case 2
            wechoaround %victim% %victim.name% trips on a rock and cuts %victim.o%self on a rusted blade. (&450&0)
            wsend %victim% You trip on a rock, cutting yourself on a rusted blade. (&1&b50&0)
            wait 15s
            wdamage %victim% 50
            wechoaround %victim% %victim.name% beings to twitch uncontrollably. (&4100&0)
            wsend %victim% You begin to twitch uncomfortably as a cold shiver runs through your body. (&1&b100&0)
            wdamage %victim% 100
            wait 15s
            wechoaround %victim% %victim.name% arches %victim.p% back painfully and loses all control. (&4150&0)
            wsend %victim% Uncontrollably you arch your back, causing extreme pain! (&1&b150&0)
            wdamage %victim% 150
            break
         default
            wechoaround %victim% %victim.name% beings to cough endlessly. (&450&0)
            wsend %victim% Something seems caught in your throat as you coughing harshly. (&1&b50&0)
            wdamage %victim% 50
            wait 15s
            wechoaround %victim% %victim.name% gasps for air as %victim.n% continues to cough. (&4100&0)
            wsend %victim% You gasp wildly for air as you continue to cough. (&1&b100&0)
            wdamage %victim% 100
            wait 15s
            wechoaround %victim% %victim.name% chokes wildly in an attempt to get air. Nearly passing out! (&4150&0)
            wsend %victim% Lack of air causes you to nearly pass out as you choke in an attempt to obtain oxygen. (&1&b150&0)
            wdamage %victim% 150
            break
         done
      done
   endif
endif
~
#2216
glabrezu-death-exits~
0 f 100
~
mecho &1&bThe fires begin to &3burn &7white-hot!&0
m_run_room_trig 2217
~
#2217
glabrezu-exits~
2 a 100
~
wait 2s
wecho &7&bThe white-hot&3 fires begin &1to recede and &0&1a path down is revealed.&0
wdoor 2213 down room 2214
wdoor 2213 down description &1A red staircase leads down.&0
~
#2218
Succubus_Incubus_Exit~
2 a 100
~
wait 2s
wecho &1The &bunbearable &3heat&0 &1&bsuddenly burns away the carpeting,&0&1 revealing a trapdoor.&0
wdoor 2211 down room 2212
wdoor 2211 down description &1A red &bhot &3iron&0&1&b staircase&0&1 leads downwards.&0
~
#2219
archon_sceptre_wield~
1 j 100
~
if %actor.vnum% == -1
   wait 1 s
   if %actor.class% /= Anti
      osend %actor% You feel a great surge of power rush through your body.
      oechoaround %actor% The eyes of %actor.name% begin to burn brightly.
      wait 1 s
      oechoaround %actor% Blackened horns protrude from atop the head of %actor.name%.
   else
      set dmg %random.100%
      osend %actor% Your mind is %actor.class% by horrific images and a sense of extreme pain.
      eval dmg %dmg% + 150
      odamage %actor% %dmg%
      oforce %actor% remove sceptre
      oforce %actor% drop sceptre
   endif
else
   wait 1 s
   osend %actor% You can't have the Sceptre of the Archon.  You're not worthy!
   oforce %actor% remove sceptre
   oforce %actor% drop sceptre
endif
~
#2220
Nezer_Entry_Trigger~
2 g 100
~
*Nezer entry messages
if (%alreadydone% != 1)
   set alreadydone 1
   global alreadydone
   wait 10s
   wecho A large flapping noice can be heard coming from above.
   wait 8s
   wecho The blood red sky is suddenly blocked out by a flying silhouette.
   wait 8s
   wecho You're forced to duck suddenly, as a massive figure swoops low, nearly taking your head off.
   wait 10s
   wecho With a giant thump Nezer of Raymif hits the ground sending a shockwave across the ground!
   * start of damage loop
   set victim %self.people%
   while %victim%
      set next %victim.next_in_room%
      if (%victim.vnum% == -1) &(%victim.level% < 100)
         eval damage 350 + %random.50%
         wdamage %victim% %damage% crush
         if %damdone% == 0
            wsend %victim% Nezer's shockwave passes through you like a shot, momentarily disorienting you.
         elseif (%victim.aff_flagged[fly]%)
            eval damage %damage% / 2
            wsend %victim% Nezer's shockwave sends you flying into the arena wall! (&1&b%damdone%&0)
            wechoaround %victim% %victim.name% is thrown violently into the arena's wall! (&4%damdone%&0)
         else
            wsend %victim% You are slammed violently into the ground after being thrown into the air by Nezer's shockwave! (&1&b%damdone%&0)
            wechoaround %victim% %victim.name% is slammed into the ground after being thrown into the air by Nezer's shockwave! (&4%damdone%&0)
         endif
      endif
      set victim %next%
   done
   wload mob 1200
endif
~
#2221
Nezer_Dragon_trigger~
0 k 15
~
set val %random.10%
switch %val%
   case 1
      breath fire
      break
   case 2
   case 3
      sweep
      break
   case 4
   case 5
      roar
      break
   default
      growl
done
~
#2222
Nezer_head_rip_1~
2 a 100
dont-let-me-catch-you-testing-this~
wait 5s
* Let's see how many casters we have in the room.
set casters 0
set victim %self.people%
while %victim%
   if (%victim.vnum% == -1) && (%victim.level% < 100)
      if (%victim.class% /= Cleric || %victim.class% /= Priest || %victim.class% /= Druid || %victim.class% /= Diabolist || %victim.class% /= Sorcerer || %victim.class% /= Cryomancer || %victim.class% /= Pyromancer || %victim.class% /= Necromancer)
         eval casters %casters% + 1
      endif
   endif
   set victim %victim.next_in_room%
done
* So if we have casters, pick one, we don't want the same
* one or to have a set order, so lets pick a random caster.
if (%casters% > 0)
   set gotyou 0
   while %gotyou% == 0
      set victim %random.char%
      if (%victim.class% /= Cleric || %victim.class% /= Priest || %victim.class% /= Druid || %victim.class% /= Diabolist || %victim.class% /= Sorcerer || %victim.class% /= Cryomancer || %victim.class% /= Pyromancer || %victim.class% /= Necromancer)
         eval gotyou %gotyou% + 1
      endif
   done
   * Hello target, now you must die!
   set message 1
   set keepgoing 1
   while %keepgoing% == 1
      if (%victim.room% == 2216)
         switch %message%
         case 1
            wechoaround %victim% %victim.name%'s magic catches the attention of one of Nezer's heads.
            wsend %victim% One of Nezer's heads suddenly takes notice of you.'
            break
         case 2
            wechoaround %victim% Nezer's head darts at %victim.name%, who only barely side steps the attack.
            wsend %victim% You just barely side step a bite by Nezer's head. Lucky you.
            break
         case 3
            set damage %random.20%
            eval damage %damage% + 105
            wechoaround %victim% Nezer's head darts at %victim.name% again, this time grabbing %victim.p% in his razor sharp teeth! (&4%damage%&0)
            wsend %victim% Nezer's head darts at you again, this time grabbing at you with it's shapr teeth, OUCH! (&1&b%damage%&0)
            wdamage %victim% %damage%
            break
         case 4
            set damage %random.20%
            eval damage %damage% + 150
            wechoaround %victim% Nezer's second head notices the first chewing on something. (&4%damage%&0)
            wsend %victim% Nezer's second head notices you as the first chomps down on you hard, OUCH! (&1&b%damage%&0)
            wdamage %victim% %damage%
            break
         case 5
            wechoaround %victim% Nezer's second head bites onto the half of %victim.name% that was sticking out before RIPPING %victim.p% in half! (&4DEAD&0)
            wsend %victim% Nezer's second head bites onto the half of you that wasn't already being consumed before RIPPING you in half! You are DEAD! (&1&bDEAD&0)
            wdamage %victim% 10000
            eval keepgoing %keepgoing% + 1
            break
         default
         done
      else
         wecho Nezer looks around for something that is no longer there.
         eval keepgoing %keepgoing% + 1
      endif
      wait 8s
      eval message %message% + 1
   done
endif
~
#2223
Nezer_head_rip_start~
0 kl 10
~
m_run_room_trig 2222
~
#2224
nezer-death~
0 f 100
~
mecho &9&bThe giant black dragon collapses to the ground with a mighty thunder!&0
m_run_room_trig 2225
~
#2225
arena-exits~
2 a 100
~
wait 2s
wecho &9&bA gate opens on the floor of the arena, leading down into darkness.&0
wdoor 2216 down room 2217
wdoor 2216 down description &9&bA grate has opened, leading down...&0
~
#2226
baatezu-death~
0 f 100
~
if %self.room% == 2217
   mecho &1A hideous &broar&0&1 fades into &9&bnothingness.&0
   m_run_room_trig 2227
endif
~
#2227
vault-exits~
2 a 100
~
wait 2s
wecho &1&bA trapdoor clicks open, &0&1leading down.&0
wdoor 2217 down room 2218
wdoor 2217 down description &1&bRed stairs lead &1down to the unknown.&0
~
#2228
balor-fight~
0 hk 100
~
if !(%self.aff_flagged[fireshield]%)
   mload obj 52053
   mat 1100 quaff sagece-fireshield
   mjunk sagece-fireshield
endif
~
#2229
RESET THE NINE HELLS DOORS~
2 d 0
Restart the Nine Hells quest.~
wdoor 2201 down purge
wdoor 2210 down purge
wdoor 2211 down purge
wdoor 2212 down purge
wdoor 2213 down purge
wdoor 2214 down purge
wdoor 2215 down purge
wdoor 2216 down purge
wdoor 2217 down purge
wecho Doors reset.
~
#2230
nalfeshnee-death-exits~
0 f 100
~
mecho &6A fierce &bwail&0&6 echoes over the sea!&0
m_run_room_trig 2231
~
#2231
nalfeshnee-load-mirror~
2 a 100
~
wait 2s
wecho &6Out of nowhere, a &bmirror&0&6 appears!&0
wload obj 23772
~
#2232
START THE NINE HELLS QUEST~
2 d 0
Start the Nine Hells quest.~
wdoor 2201 down room 2210
wdoor 2201 down name hole
wdoor 2201 down description &1The hole in the earth seems to be a bottomless shaft, with the roiling flames of hell licking the edges.&0
wat 2201 wecho &1The earth &3heaves&1 and suddenly breaks open to reveal a &bfiery &0&1pit.&0
wecho Quest started.
~
#2233
Mog Death Log~
0 f 100
~
log %actor.name% has killed %self.name%
~
#2240
belial_quest_start~
0 h 100
~
* Set the quest in motion, prevent retriggering stages
* until completion or reboot/crash/etc.
if %belial_queue%
   m_run_room_trig 2242
else
   set belial_queue 1
   global belial_queue
   m_run_room_trig 2241
endif
~
#2241
belial_pre_combat_banter~
2 a 100
~
* Belial's Pre Combat Banter
set %belial_queue% 2
set victim %self.people%
while %victim%
  if %victim.vnum% != 2219
    if %victim.class% == Paladin
      if %victim.sex% == Female
        wait 1s
        wsend %victim% Belial slowly approaches, lifting your chin to meet his piercing gaze.
        wechoaround %victim% Belial slowly approaches %victim.name%, lifting her chin to his gaze.
        wait 5s
        wecho Belial says in common, 'A rare and beautiful thing...'
        wait 3s
        wecho Belial says in common, 'Such a pity you should end here.'
        wsend %victim% Belial gently presses a finger to your lips.
        wechoaround %victim% Belial gently presses a finger to the lips of %victim.name%'.
        wait 5s
        wecho Belial says in common, 'I could offer you such pleasures beyond imagination.'
        wait 3s
        wsend %victim% Belial stares deeply into your eyes.
        wechoaround %victim% Belial stares deeply into the eyes of %victim.name%
        wecho Belial says in common, 'But that would not be enough, no.?'
        wait 5s
        wecho Belial says in common, 'No... your soul is too puerile to understand such!'
        wait 3s
        wsend %victim% Belial casts your head away from his sudden, baleful sneer.
        wechoaround %victim% Belial balefully sneers at %victim.name%, casting her head aside.
        wecho Belial says in common, 'Instead you shall all die by my hand!'
        wait 1s
        wforce belial kill %victim.name%
        halt
      elseif %victim.sex% == Male
        wait 1s
        wsend %victim% Belial slowly paces about you, sizing up your mettle.
        wechoaround %victim% slowly paces about %victim.name%, sizing him up.
        wait 5s
        wecho Belial says in common, 'Such pious arrogance...'
        wait 3s
        wecho Belial says in common, 'Fitting that you should end here.'
        wecho Belial pauses for a moment, pressing his finger to his lips.
        wait 5s
        wecho Belial breaths in heavily, raising his hands skyward.
        wecho Belial says in common, 'Do you hear them... do you hear their screams?'
        wait 5s
        wecho Belial says in common, 'They sing to me...'
        wait 3s
        wsend %victim% Belial snaps around, sneering at you with loathing and contempt.
        wechoaround %victim% Belial snaps around, sneering at %victim.name% with loathing and contempt.
        wait 5s
        wecho Belial says in common, 'And now I shall add your screams to the concerto!'
        wait 1s
        wforce belial kill %victim.name%
        halt
      endif
    else
      wait 2s
      wecho Belial says in common, 'How charming, someone come to throw themselves on my blade!'
      wait 3s
      wecho Belial says in common, 'Agents of a lesser perhaps... hmmm?'
      wecho Belial says in common, 'Come to usurp my throne, yes?'
      wait 5s
      wecho Belial paces about observingly, one hand clenched around a blood-red ranseur.
      wait 3s
      wecho Belial says in common, 'What's the matter, hellcat got your tongue?'
      wait 3s
      wecho Belial says in common, 'Don't be fooled, I see you trembling with fear...'
      wecho Belial says in common, 'I can smell it on you like a rotting, fetid wound!'
      wait 5s
      wecho Belial's eyes flair black as coal as he spins the blood-red ranseur about.
      wait 3s
      wecho Belial says in common, 'No matter, for your life ends here!'
      wecho Belial growls, 'Have at thee!'
      wait 1s
      wforce belial kill %victim.name%
      halt
    endif
  endif
  set victim %victim.next_in_room%
done
~
#2242
belial_return_banter~
2 a 100
~
* Belial's Return Banter
set banter %random.6
set victim %random.char%
switch %banter%
  case 1
    wait 2s
    wecho Belial says in common, 'Once more must we play this game?'
    wecho Belial says in common, 'By now, you know you can't defeat a crowned Prince of the Nines.' 
    break
  case 2
    wait 2s
    wecho Belial says in common, 'How I tired of this foolishness...'
    wecho Belial says in common, 'You are but a gnat to be squished forthwith.'
    break
  case 3
    wait 2s
    wecho Belial says in common, 'It will be pleasure to show you unimaginable pain once more!
    break
  case 4
    wait 2s
    wecho Belial says in common, 'Your death will be as meaningless as your pathetic life!'
    break
  case 5
    wait 2s
    wecho Belial says in common, 'I grow weary of your intrusions...'
    wecho Belial says in common, 'This time I shaill see you stay dead!'
    break
  case 6
    wait 2s
    wecho Belial says in common, 'The Nine Hells take thee...'
    wecho Belial says in common, 'For your soul now belongs to me!'
    break
  default
    wait 2s
    wecho Belial says in common, 'Your life ends here!'
    break
done
wforce belial kill %victim.name%
~
#2243
belial_spawn_demon_belial~
0 l 100
5~
* Spawn Demon Belial on 5% HP
set %belial_queue% 3
set victim %random.char%
mload mob 2221
mecho Belial falls to one knee, his chest heaving in exhaustion from battle.
wait 3s
mecho Belial says in common, 'Fools!  You have yet to witness the true power of the Nines!'
mecho Belial contorts momentarily as wings and shadow tentacles sprout from his back.
wait 3s
mecho Belial slowly rises up, his demonic form dwarfing you in size.
mecho Belial throws back his head and roars with ferocity, causing your soul to quake. 
wait 1s
force destruction kill %victim.name%
mpurge %self%
~
#2244
belial_combat_ai_script~
0 dk 20
test~
* random combat events
set timer %random.4%
if %belial_ai% > 0
switch %action%
case 1
* Deadly Spell
*line 10
m_run_room_trig 2246
break
case 2
* Severe Spell
m_run_room_trig 2247
break
case 3
* Severe Spell
m_run_room_trig 2248
*line 20
break
case 4
mecho Belial says in common, 'Fear my wrath, puny mortal!'
break
case 5
* Kick / Switch Opponents
eval victim %random.char%
eval which %random.2%
switch %which%
*line 30
case 1
kill %victim.name%
break
case 2
kick
break
done
break
case 6
*line 40
* Show random caster Fun Lovin's!
if (%self.mexists[15]% < 1)
m_run_room_trig 2249
endif
break
case 7
* Kick / Switch Opponents
eval victim %random.char%
eval which %random.2%
switch %which%
case 1
*line 50
kill %victim.name%
break
case 2
kick
break
done
break
case 8
* Random Banter
*line 60
break
case 9
* Severe Spell
m_run_room_trig 2248
break
case 10
* Severe Spell
m_run_room_trig 2247
break
*line 70
case 11
* Deadly Spell
m_run_room_trig 2246
break
default
say Your soul belongs to the Nines!
kick
done
*line 80
endif
if %belial_ai% >= 1
eval belial_ai %belial_ai% - 1
else 
set belial_ai %timer%
endif
global belial_ai
~
#2245
belial_insanity_script~
2 a 100
~
* find a caster, cause some brain pain!
~
#2246
belial_blasphemy_script~
2 a 100
~
* blasphemy = 325-400hp demonic unholy word!
wait 1s
wecho Belial throws his hands in the air, uttering in demonic, 'Verai Thak!
set victim %self.people%
while %victim%
  if (%victim.vnum% == -1) && (%victim.level% < 100)
    eval damage 325 + %random.75%
    wsend %victim% You cover your ears in horror upon hearing the demonic oath! (&1&b%damage%&0)
    wechoaround %victim% %victim.name% covers %victim.p% ears in horror upon hearing the demonic oath! (&4%damage%&0)
    wdamage %victim% %damage%
  endif
  set victim %victim.next_in_room%
done
* blasphemy = 325-400hp demonic unholy word!
wait 1s
wecho Belial throws his hands in the air, uttering in demonic, 'Verai Thak!
set victim %self.people%
while %victim%
  set next %victim.next_in_room%
  if (%victim.vnum% == -1) && (%victim.level% < 100)
    eval damage 325 + %random.75%
    wsend %victim% You cover your ears in horror upon hearing the demonic oath! (&1&b%damage%&0)
    wechoaround %victim% %victim.name% covers %victim.p% ears in horror upon hearing the demonic oath! (&4%damage%&0)
    wdamage %victim% %damage%
  endif
  set victim %next%
done
~
#2247
belial_enervation_script~
2 a 100
~
* enervation spell = 375-450 random damage
set count 3
wait 1s
wecho Belial cackles madly, spreading his open hands apart!
wecho A blackened bolts of energy crackles from Belial's fingertips.
while %count% > 0
eval victim %random.char%
if (%victim.vnum% == -1)
eval damage 375 + %random.75%
eval saved %random.2%
switch %saved%
case 1
eval damage = %damage%/2
wsend %victim% You narrowly avoid a blackened bolt of energy! (&1&b%damage%&0)
wechoaround %victim% %victim.name% narrowly avoids a blackend bolt of energy! (&4%damage%&0)
break
case 2
wsend %victim% You are hit dead on by a blackened bolt of energy! (&1&b%damage%&0)
wechoaround %victim% %victim.name% is hit dead on by a blackend bolt of energy! (&4%damage%&0)
break
default
done
wdamage %victim% %damage%
endif
eval count %count% - 1
done
~
#2248
belial_fissure_script~
2 a 100
~
* trigger info here
set count %self.people[count]%
if (%count% > 3)
  set count 3
endif
while %count% > 0
  set victim %random.char%
  if (%victim.vnum% == -1)
    eval damage 225 + %random.75%
    eval which %random.3%
    set fireproof %victim.aff_flagged[!heat]%
    switch %which%
    case 1
      if !(%fireproof%)
        eval damage %damage% * 2
      endif
      wsend %victim% Scorching flames burst out of a nearby fissure, scalding your skin! (&1&b%damage%&0)
      wechoaround %victim% White-hot flames explode out of a fissure in the ground next to %victim.name%! (&4%damage%&0)
      break
    case 2
      if !(%fireproof%)
        eval damage %damage% * 2
      endif
      wsend %victim% Bubbling, hot lava roils out of the ground onto your feet! (&1&b%damage%&0)
      wechoaround %victim% Burning lava bubbles out of the ground onto %victim.name%'s feet! Ouch! (&4%damage%&0)
      break
    default
      if !(%fireproof%)
        eval damage %damage% * 3
      endif
      wsend %victim% A torrent of molten lava consumes you, engulging you in flame! (&1&b%damage%&0)
      wechoaround %victim% A torrent of molten lava consumes %victim.name%, engulfing %victim.n% in flame. (&4%damage%&0)
    done
    wdamage %victim% %damage%
  endif
  eval count %count% - 1
  wait 2s
done
~
#2249
belial_soul_swarm_script~
2 a 100
~
wecho Start of 2249
wait 2s
wecho Belial closes his eyes, raises his hands in the air and shouts, 'Fadre Zarku!'
wait 1s
set person 0
while %person% < 1
eval victim %random.char%
if (%victim.vnum% == -1)
if (%victim.class% == sorcerer || %victim.class% == cryomancer || %victim.class% == pyromancer || %victim.class% == priest || %victim.class% == druid || %victim.class% == cleric)
wsend %victim% To Victim test
wechoaround %victim% To Room test
wload mob 15
wforce wraith kill %victim.name%
wload mob 15
wforce wraith kill %victim.name%
wload mob 15
wforce wraith kill %victim.name%
eval person %person% + 1
endif
endif
done
~
#2250
Icicle_Melt~
1 c 2
melt~
osend %actor% The long &4&bicicle&0 starts to soften up and begins to melt.
oechoaround %actor% %actor.name% squeezes the &4&bicicle&0 and it begins to melt.
wait 5s
oecho &3&0Gratz on finding the 1st piece!  This &1&bIS&7&0 the format for whole the quest.&0
wait 3s
oecho The water &6&ddroplets&0 flow from the &4&0icicle&0 to the ground, forming a &7&bpuddle&0.
wait 5s
oecho Suddenly, &3&bwords&0 begin to slowly take form in the &7&bpuddle&0.
wait 2s
oecho Held by a peaceful man oft found meditating in his favorite sacred grove.
oecho A difficult opponent if tested, his wand wisks anything away with a few taps.
~
#2251
Dandelion_Blow~
1 c 2
blow~
osend %actor% You &7&bblow&0 the seeds from the small &6&bdandelion&0 in all directions.
oechoaround %actor% %actor.name% &7&bblows&0 on the small &6&bdandelion&0, sending seeds everywhere.
wait 5s
oecho A &6&bbreeze&0 picks up, swirling the &3&bseeds&0 about and whispering in you ear...
wait 5s
oecho Resting in the &4&bfrigid&0 pools, this 75' long lizard keeps a &1&bdemented&0 grin.
oecho Though aged, her vicious &1&broar&0 is more menacing than the greatest of &3&blions&0.
~
#2252
RubberBall_Bounce~
1 c 2
bounce~
osend %actor% The &2&bgreen ball&0 ricochets off the ground and &1&bsmacks&0 your chin.
oechoaround %actor% %actor.name% bounces the &2&bball&0.  It ricochets off the ground, &1&bsmacking&0 %actor.o% in the &1&b&3&bjaw&0&0.
wait 3s
osend %actor% You slump to the ground, &1&bknocked out&0 from the sudden blow from a &2&bgreen ball&0.
oechoaround %actor% %actor.name% slumps to the ground, &1&bknocked out&0 from the sudden blow from a &2&bgreen ball&0.
wait 8s
osend %actor% As you are &6&bawaken&0, you feel obligated to relay your &7&bvision&0 to the group...
oechoaround %actor%  As %actor.name% & &6&bawakens&0, %actor.n% tells you of %actor.p% &7&bvision&0...
wait 4s
oecho Held by a &3&bscarred hunter&0 who has hunted nearly as many as have &1&bhunted&0 him!
oecho His body witness to his &3&bwarrior prowess&0 and acts of &4&bpride&0 for his fellow people.
~
#2253
Snowglobe_Shake~
1 c 2
shake~
osend %actor% The old &7&bsnowglobe&0 glows and the small town inside comes &6&balive&0.
oechoaround %actor% %actor.name% shakes the &7&bsnowglobe&0 and the town inside comes &6&balive&0.
wait 7s
oecho The &5&bsmall town&0 continues to glow as you make out a visage in the &7&bsnow&0...
wait 5
oecho A &3&btall&0 and &3muscular&0 &7&bcleric&0 who could almost pass for that of a human.
oecho His eyes &1&b&1&bscorch&0&0 the &4&bsoul&0 of all those who dare gaze deeply into them.
~
#2254
Stylus_Write~
1 c 2
write~
osend %actor% The &3&bc&2&bo&4&bl&5&bo&3&br&6&bf&5&bu&1&bl&0 stylus glows briefly, filling the room with light.
oechoaround %actor% %actor.name% writes with a &3&bc&2&bo&4&bl&5&bo&3&br&6&bf&5&bu&1&bl&0 stylus and it begins to glow brightly.
wait 5s
osend %actor% The &3&bc&2&bo&4&bl&5&bo&3&br&6&bf&5&bu&1&bl&0 stylus flies from your hands and begins writing in the air.
oechoaround %actor% The &3&bc&2&bo&4&bl&5&bo&3&br&6&bf&5&bu&1&bl&0 stylus suddenly begins writing in the air by itself.
wait 3
oecho Held by a hidden man, he was the only one to achieve &7&bimmortality&0 via &4&bwizardry&0.
oecho Though &4&bmagic&0 slowed his age, endlessly he searchs for a new &7&bsoul&0 to &1&bleach&0.
~
#2255
Lamp_Rub~
1 c 2
rub~
osend %actor% The &3&bold lamp&0 begins to shake as a cloud of &7&bsmoke&0 comes out.
oechoaround %actor% %actor.name% rubs the &3&bold lamp&0 and a cloud of &7&bsmoke&0 comes out.
wait 5s
oecho The cloud of &7&bsmoke&0 begins to thin out and forms into &6&bwords&0.
wait 7s
oecho Held by a &6&bpoltergeist&0 that is about as far from &2&bMielikki&0 as possible.
oecho In his &3&bmaster's chamber&0 he does rest, expelling his energy &7&bpeacefully&0 about.
~
#2256
Incense_Burn~
1 c 2
burn~
osend %actor% The old &6&bi&3&bn&6&bc&3&be&6&bn&3&bs&6&be&0 lights briefly and begins to emit a &5&bnoxious&0 odor.
oechoaround %actor% %actor.name% lights the old &6&bi&3&bn&6&bc&3&be&6&bn&3&bs&6&be&0 and it begins to emit a &5&bnoxious&0 odor.
wait 5s
oecho The old &6&bi&3&bn&6&bc&3&be&6&bn&3&bs&6&be&0 starts to flash brightly, sending sparks everywhere!
wait 5s
oecho The &7&bsparks&0 spray all over the ground in some oddly apparent &3&bpattern&0.
wait 5s
oecho As the &7&bsmoke&0 clears, a message is revealed in the &1&bscorched&0 ground...
wait 5s
oecho Held by an &2&bold hag&0 who sits above her &7&bcauldron&0 brewing evil potions.
oecho Though &6&bformidable&0, you must first pass the &3&bdoor&0 that is without a knot.
~
#2257
Orchid_Smell~
1 c 2
smell~
osend %actor% You reach down and smell the &4&bblue orchid&0, which gives off a &6&bdelictable&0 scent.
oechoaround %actor% %actor.name% smells a &4&bblue orchid&0, which sends the scent all through the room.
wait 5s
oecho The &5&bscent&0 from the &4&bblue orchid&0 permeates through the area.
wait 5ss
oecho For a moment, you a struck with a soothing &6&bclairity&0 and &7&bcalm&0.
wait 5s
oecho A strange &7&bvision&0 prevades your &3&bmoment&0 of &2&benlightenment&0...
wait 5ss
oecho In a land where &4&bliquid&0 is rare and &1&bwarriors&0 master thier skills well.
oecho Seek out the &7&bhidden woman&0, but beware your imminent &3&bimprisonment&0.
~
#2258
Coin_Flip~
1 c 2
flip~
oechoaround %actor% %actor.name% flips an &2&bworn coin&0, sending it high in the &6&bair&0.
osend %actor% The &3&bworn&0 coin flips high into the air, &1&btumbling&0 over and over.
wait 5s
oecho You wait what seems an &7&beternity&0 as the worn &3&bcoin soars&0 through the &6&bair&0.
wait 7s
osend %actor% With a &2&bflick&0 of your wrist, you reach out and snag the &3&bworn coin&0 in flight.
oechoaround %actor% %actor.name% reaches out and catches the &3&bworn coin&0 while on the &1&bdescent&0.
wait 5s
oecho Where once there were two different &2&bsides&0 there is only a &5&bsingle&0 image...
wait 3s
oecho Far from &2&bMielikki&0, a man of much &6&bjubilation&0 and a happy &1&blover&0.
oecho He roams &7&bpeacefully&0 about his &2&bpavillion&0 hoping to see his &5&bprincess&0.
~
#2259
Cards_Shuffle~
1 c 2
shuffle~
osend %actor% You &3&bshuffle&0 the cards around in your hands and a &1&bJoker&0 ends up on top.
oechoaround %actor% %actor.name% &3&bshuffles&0 the cards around and a &1&bJoker&0 ends up on top.
wait 5s
osend %actor% You &3&bshuffle&0 the cards once more, before cutting to an &1&bunusual&0 card...
oechoaround %actor% %actor.name% &3&bshuffles&0 the cards once more, before cutting to an &1&bunusual&0 card...
wait 5s
oecho The &3&bcard&0 shows an &7&bold ship&0, though once a &2&bprosperous&0 merchant vessel.
oecho As well, a &7&bresurrected&0 man who forever dwells like a good &5&bcaptain&0 should.
~
#2260
Match_Light~
1 c 2
light~
osend %actor% You strike the &1&bmatch&0 on the ground as &7&bbright sparks&0 shoot forth.
oechoaround %actor% %actor.name% strikes the &1&bmatch&0 on the ground as &7&bbright sparks&0 shoot forth.
wait 7s
oecho The &1&bmatch&0 suddenly flares into a briliant blinding &3&bexplosion&0 of &7&blight&0.
wait 5s
oecho As your eyes adjust, you recall a &6faint&0 image &1&bburned&0 in your &4&bmind&0...
wait 5
oecho The &3crumbled&0 remains of a once &7&bgreat city&0 and endless billowing &6&bash&0.
oecho A strong &2&boaf&0, and would be &7&bnobler&0 if it weren't for his &1&bevil passion&0.
~
#2261
Disc_Toss~
1 g 2
5~
Nothing.
~
#2262
Mobs_Death~
0 f 100
~
mforce %actor.name% petition %actor.name%'s group has killed %self.name%!!
~
#2263
Get_12th_piece~
1 g 100
~
wait 2
oload mob 2202
oteleport tsaeldin 3002
oforce tsaeldin goss At last!  The lost artifacts have been recovered!
wait 10
oforce tsaeldin goss You MUST come see me at once.  I shall await your arrival in Mielikki.
~
#2265
lylaith~
1 j 5
lylaith~
wait 2s
osend %actor% As you wield %self.shortdesc%, the blade glows a &7&bbright white&0!
oechoaround %actor% As %actor.name% wields %self.shortdesc%, the blade glows a &7&bbright white&0!
~
#2267
pfarlysia_cloak~
1 j 100
~
wait 2s
osend %actor% As you wear %self.shortdesc%, the cloak shimmers a &1&bredish&0-&3&byellow&0.
oechoaround %actor% As %actor.name% wears %self.shortdesc%, the cloak shimmers a &1&bredish&0-&3&byellow&0.
~
#2270
vang_gauntlets~
1 j 100
~
wait 2
osend %actor% As you wear %self.shortdesc%, they begin to burn a &1&bflaming red&0.
oechoaround %actor% As %actor.name% wears %self.shortdesc%, they begin to burn a &1&bflaming red&0.
~
#2272
krystophus_ring~
1 j 100
~
wait 2
osend %actor% Your head beings to reel momentarily as you wear %self.shortdesc%.
oechoaround %actor% %actor.name% suddenly looks off balance for a moment.
~
#2274
sysmaith~
1 j 5
sysmaith~
wait 2s
osend %actor% As you wield %self.shortdesc%, a &4&bbright blue&0 flame burns along the blade!
oechoaround %actor% As %actor.name% wields %self.shortdesc%, a &4&bbright blue&0 flame burns along the blade!
~
#2280
dual_axe_blur_script~
1 c 100
Niamh~
say My trigger commandlist is not complete!
~
#2293
heartbeat~
1 g 100
~
wait 2s
osend %actor.name% as you grab the heart, you notice it starts to beat in your hand. 
oechoaround %actor.name% %actor.name% grabs a ruby heart, which starts beating within his hand!
~
#2294
cats_eyes_wear~
1 j 100
~
wait 1s
osend %actor.name% As you lift the cat's eyes towards your own, they are absorbed into your eye sockets.
oechoaround %actor.name% %actor.name% lifts a pair of green eyes to his own, and they are absorbed into %actor.p% own. 
return 1
~
$~
