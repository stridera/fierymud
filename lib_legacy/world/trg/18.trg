#1800
traveller_greet_all~
0 h 100
~
if %self.room% == 1802 && %actor.level% < 100
   wait 1s
   msend %actor% Thelmor hugs you warmly.
   msend %actor% Thelmor tells you, 'Thank you for freeing me.'
   msend %actor% Thelmor mutters something about Izaro.
   wait 2s
   if %get.mob_count[1808]% == 0
      msend %actor% Thelmor wails pitifully.
      msend %actor% Thelmor says, 'My revenge is fulfilled, and I am free!'
      rem blade
      drop blade
      msend %actor% Thelmor dissipates before your very eyes.
      if %actor.class% == Ranger && %actor.level% > 80
        quest start blur %actor.name%
        msend %actor% A voice from the forest whispers, 'You have done the forest a great service.
        msend %actor% &0Meet me at the nearby spring.'
      endif
      mpurge self
      halt
   else
      msend %actor% Thelmor shouts, 'Izaro!  It is time to end this.'
      wait 2s
      msend %actor% Izaro streaks in from the forest.
      wait 1
      mteleport fallen-nymph thelmor
      kill fallen-nymph
   endif
endif
~
#1801
random room trigger for room 1888~
0 b 100
~
m_run_room_trig 1802
~
#1802
Room 1888 - massive harming~
2 d 100
(This script automatically called from mob 1850 in trigger 1801)~
set person %self.people%
while %person%
   set next %person.next_in_room%
   if %person.vnum% == -1
      wdamage %person% 100
   endif
   set person %next%
done
~
#1803
fountain_whisper~
1 c 100
drink~
return 0
if %random.100% > 75
oecho The wind whispers, 'Please... help us!'
endif
~
#1804
help_trigger~
2 d 100
help?~
wait 1s
wecho The wind whispers, 'Are you willing to help end the taint?'
~
#1805
yes_to_help~
2 d 100
yes Yes Yes? yes?~
if %actor.quest_stage[blur]% == 0 || %actor.has_completed[blur]%
  wait 2s
  wecho A beautiful woman emerges from amongst the trees.
  wload mob 1811
  wecho The woman smiles warmly.
  wait 4s
  wecho The woman says, 'Long ago, two men came here to duel.  Neither truly won or lost.  The forest is tainted by the malevolence each held for the other.'
  wait 4s
  wecho The woman sighs heavily.
  wait 2s
  wecho The woman says, 'One locked the other's soul in a hidden room, and threw the key into the forest.  He too had not long to live and soon perished, leaving behind a bit of himself.'
  wait 4s
  wecho The woman shakes her head, sadly.
  wait 2s
  wecho The woman says, 'The other's hatred infected the spirits of the forest, warping some of them.'
  wait 2s
  wecho The woman says, 'One in particular became so consumed by it we had to imprison her in a prison of thorns.'
  wait 4s
  wecho The woman says, 'If you could perhaps find and release the soul, perhaps they could end the duel.  Maybe then this forest will return to normal.'
  wait 3s
  wecho The woman retreats into the trees.
  wpurge nymph
  if %get.obj_count[1899]% < 2
     eval keyroom 1802 + %random.13%
     wat %keyroom% wload obj 1899
  endif
endif
~
#1806
evil_nymph_death~
0 f 100
~
mecho running
if %self.room% == 1802 && %get.mob_count[1813]%
   mecho As the nymph dies, the shade of Thelmor dissipates in peace.
   mpurge shade
endif
set person %actor%
set i %person.group_size%
if %i%
   set a 1
   while %i% > %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.class% == Ranger && %person.level% > 80 && !%person.quest_stage[blur]%
            quest start blur %person.name%
            msend %person% A voice from the forest whispers, 'You have done the forest a great service.  Meet me at the nearby spring.'
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.class% == Ranger && %person.level% > 80 && !%person.quest_stage[blur]%
   quest start blur %person.name%
   msend %person% A voice from the forest whispers, 'You have done the forest a great service.  Meet me at the nearby spring.'
endif
~
#1807
give_nymph_junkk~
0 j 100
~
wait 1s
junk %object.name%
~
#1808
fungus_spore~
0 b 20
~
emote releases a cloud of spores. 
~
#1809
thorn_trigger~
1 g 7
~
odamage %actor.name% 20 poison
if %damdone% > 0
   oechoaround %actor.name% %actor.name% cries out in pain! (&2%damdone%&0)
   osend %actor.name% You prick your finger on the poisoned thorn as you pick it up! (&2%damdone%&0)
end
~
#1810
no_flee~
2 c 100
flee~
wechoaround %actor% %actor.name% panics, but cannot flee the void!
wsend %actor% You PANIC as you cannot flee the void!
return 1
~
#1811
submit_command_trigger~
0 c 100
submit ~
blink
   mforce %actor.name% forget all
msend %actor.name% You feel drained.
set rm %random.10%
switch %rm%
case 1
mteleport %actor.name% 52070
break
case 2
mteleport %actor.name% 48080
break
case 3
mteleport %actor.name% 11834
break
case 4
mteleport %actor.name% 11835
break
case 5
eval dest 43144 + %random.13%
mteleport %actor.name% %dest%
break
case 6
mteleport %actor.name% 10100
break
case 7
mteleport %actor.name% 55105
break
case 8
mteleport %actor.name% 32589
break
case 9
mteleport %actor.name% 12701
break
~
#1812
Guard Trigger~
0 c 100
west~
if %actor.vnum% == -1
mechoaround %actor.name% The thorn monster blocks %actor.name%'s path!
msend %actor.name% The thorn monster pushes you back!
whisper %actor.name% Try that again, and I will have to slay you for my mistress.
endif
~
#1813
Room 1888 - Cruel teleportation ~
0 d 0
submit~
mforce %actor.name% forget all
msend %actor.name% You feel drained and disoriented.
mechoaround %actor.name% %actor.name% fades from existence and disappears. 
set rm %random.10%
switch %rm%
case 1
mteleport %actor.name% 52070
break
case 2
mteleport %actor.name% 48080
break
case 3
mteleport %actor.name% 11834
break
case 4
mteleport %actor.name% 11835
break
case 5
eval dest 43144 + %random.13%
mteleport %actor.name% %dest%
break
case 6
mteleport %actor.name% 10100
break
case 7
mteleport %actor.name% 55105
break
case 8
mteleport %actor.name% 32589
break
case 9
mteleport %actor.name% 12701
break
default: 
mteleport %actor.name% 52086
done
force %actor% look
~
#1814
UNUSED~
0 c 100
east~
Nothing.
~
#1815
thelmor_trigger_room_kill~
2 b 100
~
m_run_room_trig 1820
~
#1816
**UNUSED**~
0 c 100
enter~
   if %actor.vnum% == -1
mechoaround %actor.name% The thorn monster blocks %actor.name%'s path to the well!
msend %actor.name% The thorn monster pushes you back from the well!
whisper %actor.name% You're not getting away that easily!
endif
~
#1817
thorny_vine_attack~
0 g 14
~
if %actor.vnum% == -1 && %actor.level% < 100
msend %actor.name% You step on a thorny vine, which attempts to wrap around your leg!
mechoaround %actor.name% %actor.name% steps on a thorny vine, which flails wildly in response.
mkill %actor.name%
endif
~
#1820
Coup~
2 g 100
Automagically called by script 1815~
   eval numpeople %people.1802%
   while %numpeople% >= 0
   if %numpeople% > 0
   set prsn %self.people[%numpeople%]%
          if %prsn.vnum% == 1808
wecho King Thelmor delivers a final coup de grace, killing the nymph!
   wdamage %prsn% 10000
   endif
endif
   eval numpeople %numpeople% -1
   done
~
#1821
submit_prompt~
0 b 100
~
say Submit to my will, and I might let you escape alive. 
say Or perhaps if you give a sacrifice I like, I may send you out unscathed. 
m_run_room_trig 1802
~
#1822
sacrifice_trigger~
0 d 100
sacrifice Sacrifice~
msend %actor.name% The void guardians says to you, "Give me something valuable, and I might set you free."
~
#1823
drop_sacrifice~
0 j 100
~
if %object.vnum% > 1100
if %random.100% < 5
wait 1s
mecho The void guardian nods and says, "I will accept this from you.  Begone!"
mechoaround %actor.name% The void guardian envelops %actor.name%, who disappears.
msend %actor.name% The void guardian envelops you, and you find yourself elsewhere.
mteleport %actor.name% 1841
else 
wait 1s
mecho The void guardian growls and says, "You think this will appease me?  BAH!"
end
end
mjunk %object.name%
~
#1824
blur_ranger_greet~
0 g 100
~
wait 2
if %actor.class% /= Ranger
  bow %actor%
  say Welcome fellow warden.
  wait 1s
else
  mecho %self.name% says, 'Greetings traveler.  Be careful, this forest can
  mecho &0be... unpredictable.'
endif
if %actor.quest_stage[blur]% == 1
  mecho %self.name% says, 'I see you have brought peace to the forest.  I am
  mecho &0impressed.  Perhaps you are ready to learn the deeper ways of nature?'
elseif %actor.quest_stage[blur]% == 2
  mecho %self.name% says, 'The Syric Warder still lives.  Relieve him of his
  mecho &0endless vigil.'
elseif %actor.quest_stage[blur]% == 3
  say Let me have the Warder's sword.
elseif %actor.quest_stage[blur]% == 4
  say Are you ready to race the four winds?
elseif %actor.has_failed[blur]%
  say Back again I see.  Ready to try again?
endif
~
#1825
blur_ranger_speech1~
0 d 100
yes ready deeper ways nature deeper? ways? nature?~
wait 2
if %actor.quest_stage[blur]% == 1
  mecho A twinkle shines in %self.name%'s eye.
  say I had hoped so.
  quest advance blur %actor.name%
  wait 2s
  mecho %self.name% says, 'There is a very powerful spell only we warriors of
  mecho &0nature know.  If you prove yourself worthy, you will be able to learn it as
  mecho &0well.  Helping the forest was a demonstration of the purity of your spirit, but
  mecho &0you must now prove your strength.'
  wait 4s
  mecho %self.name% says, 'There is a warder who failed to protect a massive
  mecho &0swath of the west from being consumed by the Realm of Dreams.  Since his
  mecho &0failure, he has become part of the Dream, fallen completely to nightmare.'
  wait 4s
  say End his suffering and bring me his blade.
  wait 2s
  mecho %self.name% says, 'Check with me if you want to know your &7&b[progress]&0.'
elseif %actor.quest_stage[blur]% == 4
  if %speech% /= yes || %speech% /= ready
    mecho %self.name% says, 'You will only have one day to race all four winds,
    mecho &0and even less time for each wind individually.'
    mecho   
    mecho %self.name% says, 'If after one day you have not completed all four
    mecho &0of their challenges, you will have to come back to me to start again.'
    wait 4s
    mecho %self.name% says, 'When you are ready, say &7&b[let's begin]&0.'
  endif
elseif %actor.has_failed[blur]% && (%speech% /= yes || %speech% /= ready)
  quest restart blur %actor.name%
  quest advance blur %actor.name%
  quest advance blur %actor.name%
  quest advance blur %actor.name%
  mecho %self.name% says, 'Then you will need to find the winds again and race them.
  mecho &0You have one day to complete their challenges.'
  wait 3s
  mecho %self.name% says, 'When you are ready, say &7&b[let's begin]&0.'
endif
~
#1826
blur_syric_warder_death~
0 f 100
~
set i %actor.group_size%
set person %actor%
if %i%
   set a 1
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[blur]% == 2
            quest advance blur %person.name%
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[blur]% == 2
   quest advance blur %person.name%
endif
~
#1827
blur_ranger_receive~
0 j 100
~
if %object.vnum% == 58420
  if %actor.quest_stage[blur]% == 3
    quest advance blur %actor.name%
    wait 2
    mecho %self.name% says, 'Thank you for ending his suffering.  That was a
    mecho &0truly heroic effort.'
    wait 2s
    mecho %self.name% says, 'But this test is about more than just strength.
    mecho &0The true signature of a ranger is speed.'
    wait 2s
    emote whistles a complex tune to the sky and listens.
    wait 8s
    mecho A breeze rustles the leaves of the trees.
    wait 3s
    emote smiles.
    wait 1s
    nod
    wait 3s
    mecho %self.name% says, 'Your true teachers will be the four winds.  I have
    mecho &0called out to them.  They have agreed to teach you the spell if you can pass
    mecho &0the test we all have passed.'
    wait 5s
    say You must best them in a race.
    wait 4s
    mecho %self.name% says, 'The four winds are notoriously difficult to locate.
    mecho &0The North Wind blows through the frozen town of Ickle.
    mecho &0The South Wind blows around the hidden standing stones in South Caelia.
    mecho &0The East Wind blows through an enormous volcano in the sea.
    mecho &0The West Wind blows through ruins across the vast Gothra plains.'
    wait 7s
    mecho %self.name% says, 'You may challenge them in any order, but even if
    mecho &0you find where the winds blow, there may be additional challenges to connecting
    mecho &0with them.'
    wait 5s
    give forgotten-kings %actor%
    say You may need this.
    wait 4s
    mecho %self.name% says, 'Are you ready to try?'
  elseif %actor.quest_stage[blur]% == 2
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    mecho %self.name% says, 'It seems the Warder still roams.  Please go back and
    mecho &0destroy him.'
  elseif %actor.quest_stage[blur]% > 3
    return 0
    wait 2
    mecho %self.name% says, 'You have already brought me %object.name%.
    mecho &0Please, keep it.'
  endif
elseif %actor.quest_stage[blur]% == 3
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  say This is not the proper blade.
else
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  say I have no need for this at the moment.
endif
~
#1828
blur_ranger_speech2~
0 d 0
let's begin~
quest variable blur %actor.name% east 0
quest variable blur %actor.name% west 0
quest variable blur %actor.name% south 0
quest variable blur %actor.name% north 0
if %actor.quest_stage[blur]% == 4
  wait 2
  say Ready...
  wait 3s
  say Set...
  wait 3s
  mecho %self.name% says, '&2&bGO!!&0  Your time has begun!'
  quest variable blur %actor% race go
  mecho %self.name% fades into the trees.
  mteleport %self% 1100
  set count 1800
  msend %actor% &7&b24 hours (30:00 minutes) remain.&0
  while %count% > 0
    if %actor.has_completed[blur]%
      eval time %count% - 1800
      set count %time%
    else
      quest variable blur %actor% timer %count%
      eval time %count% - 25
      wait 25s
      set count %time%
      if %count% == 900
        msend %actor% &7&b12 hours (15:00 minutes) remain.&0
      elseif %count% == 450 
        msend %actor% &7&b6 hours (7:30 minutes) remain.&0
      elseif %count% == 300 
        msend %actor% &7&b4 hours (5:00 minutes) remain.&0
      elseif %count% == 150 
        msend %actor% &7&b2 hours (2:30 minutes) remain.&0
      elseif %count% == 75 
        msend %actor% &7&b1 hour (1:15 minutes) remain.&0
      elseif %count% == 0 
        msend %actor% &1&bTime's up!!&0
      endif
    endif
  done
  if !%actor.has_completed[blur]%
    quest fail blur %actor.name%
    quest variable blur %actor% race off
    t %actor% You'll have to be a little quicker on your toes next time.
    wait 2
    t %actor% 'Come back to me if you want to try again.'
  endif
endif
mpurge %self%
~
#1829
blur_winds_greet~
0 g 100
~
wait 2
if %actor.quest_stage[blur]% == 4
  switch %self.vnum%
    case 1819
      mecho A mighty chill blows through the temple.
      wait 2s
      say So you come to test your speed against me?
      break
    case 1820
      mecho The sound of rustling leaves coheres into the sounds of words.
      wait 2s
      say You wish to race me?
      break
    case 1821
      mecho A thin willowy voice moans in pain.
      wait 1s
      say Have you come to challenge me?
      break
    case 1822
      mecho The wind whistles by your ears!
      wait 3s
      say So you want to race huh?
      break
    default
      return 0
  done
endif
~
#1830
blur_cheetah_death~
0 f 100
~
set room %self.room%
set person %room.people%
set lair %get.room[4236]%
while %person%
  if (%person.quest_stage[blur]% == 4) && (!%person.quest_variable[blur:west]%)
    msend %person% &7&b%get.mob_shortdesc[1822]% says, 'Well that was fun!&0
    msend %person% &0&7&bNow see if you can reach &6&b%lair.name%&0&7&b before I do!'&0
    msend %person% %get.mob_shortdesc[1822]% blasts away into the sky!
    quest variable blur %person.name% west 1
  endif
  set person %person.next_in_room%
done
~
#1831
blur_ranger_load~
0 o 100
~
mecho Leaves rustle as a hooded figure steps out of the forest and sits by the spring.
~
#1832
blur_ranger_preentry~
2 g 100
~
if (%actor.quest_stage[blur]% > 0 || %actor.has_failed[blur]%) && %self.people[1818]% == 0
  wload mob 1818
endif
~
#1833
blur_winds_preentry~
2 g 100
~
if %actor.quest_stage[blur]% == 4
  switch %self.vnum%
    case 10001
      set direction north
      set mob 1819
      break
    case 2460
      set direction south
      set mob 1820
      break
    case 48223
      set direction east
      set mob 1821
      break
    case 20379
      set direction west
      set mob 1822
      break
    default
      return 0
  done
  if %actor.quest_variable[blur:%direction%]% == 0 && %get.mob_count[%mob%]% == 0
    wload mob %mob%
  endif
endif
~
#1834
blur_winds_speech~
0 d 100
yes sure yep okay~
wait 2
if %actor.quest_stage[blur]% == 4
  switch %self.vnum%
    case 1819
      set direction north
      set home 30262
      set color &2&b
      set room %get.room[30262]%
      mecho %self.name% says, 'Then you will have to beat me to: %color%%room.name%&0
      mecho &0in Bluebonnet Pass.'
      emote laughs mightily as it vanishes!
      quest variable blur %actor.name% %direction% 1
      break
    case 1820
      set direction south
      set home 53557
      set color &1&b
      set room %get.room[53557]%
      mecho %self.name% says, 'Long has it been since one such as yourself has made this
      mecho &0request.  Now show me your speed!'
      wait 1s
      mecho %self.name% says, 'Try to reach %color%%room.name%&0
      mecho &0before I do!'
      emote rustles away with the sound of the leaves.
      quest variable blur %actor.name% %direction% 1
      break
    case 1821
      set direction east
      set home 12597
      set color &3&b
      set room %get.room[12597]%
      if %get.mob_count[48105]% == 0
        say Then let it be so!
        wait 1s
        mecho %self.name% says, 'Reach %color%%room.name%&0
        mecho &0before me!'
        quest variable blur %actor.name% %direction% 1  
      else
        say Then please help me escape from the clutches of this madwoman!
        wait 1s
        mecho Vulcera throws back her head and cackles!
      endif
      break
    case 1822
      set direction west
      set home 4236
      set color &6&b
      set room %get.room[4236]%
      say Well, let's play a different game first...
      wait 3s
      mecho %self.name% says, 'I'll be riding the fastest animal in Gothra!
      mecho &0If you can find me, then we can race!'
      eval load 20308 + %random.46%
      mat %load% mload mob 20321
      break
    default
      return 0
  done
  mteleport %self% 1100
  set count 450
  while %count% > 0
    if %actor.room% == %home% 
      eval time %count% - 450
      set count %time%
    else
      quest variable blur %actor% %direction%_timer %count%
      eval time %count% - 5
      wait 5s
      set count %time%
    endif
  done
  if %actor.quest_variable[blur:%direction%]% == 2
    mpurge %self%
  else
    if %actor.room% == %home%
      if %actor.quest_variable[blur:%direction%]% == 1
        msend %actor% %self.name% tells you, '%color%Wow you're fast!  Incredible!&0'
        quest variable blur %actor.name% %direction% 2
        if %actor.quest_variable[blur:east]% == 2 && %actor.quest_variable[blur:west]% == 2 && %actor.quest_variable[blur:north]% == 2 && %actor.quest_variable[blur:south]% == 2
          mskillset %actor.name% blur
          wait 2
          msend %actor% You have matched the greatest speeds of nature!
          msend %actor% You have learned &1Blur&0!
          quest complete blur %actor.name%
        endif
      else
        if %self.vnum% == 1822
          msend %actor% %self.name% tells you, '%color%You never found me, too bad!&0'
          quest variable blur %actor.name% %direction% 0
        elseif %self.vnum% == 1821
          msend %actor% %self.name% tells you, '%color%You didn't rescue me in time!&0'
          quest variable blur %actor.name% %direction% 0
        endif
      endif
    else
      msend %actor% %self.name% tells you, '%color%Sorry, too slow!&0'
      msend %actor% %self.name% tells you, '%color%Come back if you want a rematch!&0'
      quest variable blur %actor.name% %direction% 0
    endif
  endif
endif
mpurge %self%
~
#1835
blur_ranger_status_checker~
0 d 100
status status? progress progress?~
wait 2
set stage %actor.quest_stage[blur]%
if %stage% == 2
  mecho %self.name% says, 'Seek out and kill the Syric Warder and bring me his
  mecho &0sword.'
elseif %stage% == 3
  say Give me the Blade of the Forgotten Kings.
elseif %stage% == 4
  say It's time to race the four winds!
  mecho  
  mecho The North Wind blows near the frozen town of Ickle.
  mecho The South Wind blows around the hidden standing stones in South Caelia.
  mecho The East Wind blows through an enormous volcano in the sea.
  mecho The West Wind blows through ruins across the vast Gothra plains.
  wait 1s
  say Are you ready?
endif
~
#1836
blur_vulcera_death~
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
      if (%person.quest_stage[blur]% == 4) && (!%person.quest_variable[blur:east]%) && %get.mob_count[1821]%
        set lair %get.room[12597]%
        msend %person% %get.mob_shortdesc[1821]% thanks you heartily!
        mechoaround %person% %get.mob_shortdesc[1821]% thanks %person.name% heartily!
        msend %person% %get.mob_shortdesc[1821]% tells you, 'See if you can get to&3&b %lair.name% &0first!'
        msend %person% 'I already started the clock...'
        mecho %get.mob_shortdesc[1821]% takes off like a rocket and vanishes!
        quest variable blur %person.name% east 1
        set person %person.next_in_room%
      elseif %person%
        eval i %i% + 1
      endif
      eval a %a% + 1
    endif
  done
else
  if (%person.quest_stage[blur]% == 4) && (!%person.quest_stage[blur:east]%) && %get.mob_count[1821]%
    set lair %get.room[12597]%
    msend %person% %get.mob_shortdesc[1821]% thanks you heartily!
    mechoaround %person% %get.mob_shortdesc[1821]% thanks %person.name% heartily!
    msend %person% %get.mob_shortdesc[1821]% tells you, 'See if you can get to&3&b %lair.name% &0first!'
    msend %person% 'I already started the clock...'
    mecho %get.mob_shortdesc[1821]% takes off like a rocket and vanishes!
    quest variable blur %person.name% east 1
    set person %person.next_in_room%
  endif
endif
~
#1837
new trigger~
2 b 100
~
wecho There are %people.self% people here
~
#1850
no_monk~
1 j 100
~
if %actor.class% /= Monk
osend %actor% You cannot use  %self.shortdesc%.
return 0
endif
~
#1855
test_for_nok~
2 g 100
~
   if %actor.vnum% == -1
wait 1
wsend %actor% You land with a loud thump on the fiery floor.
wechoaround %actor% %actor.name% lands on the fiery floor with a loud thump.
wait 1
wsend %actor% You feel yourself burning!
wdamage %actor% 200
wforce %actor.name% shout Help me!
wait 5s
   if %actor.room% != 1841
halt
endif
   wsend %actor% You feel a burst of flame rip into your skin!
wechoaround %actor% A burst of flame rips into %actor.name%, almost burning him alive!
wdamage %actor% 100
   wait 10s
   if %actor.room% != 1841
halt
endif
   wsend %actor% You feel a burst of flame rip into your skin!
wechoaround %actor% A burst of flame rips into %actor.name%, almost burning him alive!
wdamage %actor% 100
wait 15s
   if %actor.room% != 1841
halt
endif
   wforce %actor.name% shout help, I'm burning!
   wait 1
   if %actor.room% != 1841
halt
endif
   wsend %actor% You feel a burst of flame rip into your skin!
wechoaround %actor% A burst of flame rips into %actor.name%, almost burning him alive!
wdamage %actor% 100
wait 20s
if %actor.room% != 1841
halt
endif
   wsend %actor% The flames consume you.
wechoaround %actor% The searing flames consume %actor.name%.
wdamage %actor% 1000
end
~
#1860
new trigger~
0 m 100
10~
say My trigger commandlist is not complete!
~
#1866
UNUSED~
0 c 100
give 100 copper hunter~
say thank you :>
~
#1888
Enter the nymphs lair~
2 g 100
~
if %self.people[1808]%
   if %actor.level% < 61
      wait 2s
      wsend %actor.name% The nymph cackles, 'So, Thelmor has sent you fools to kill me...'
   elseif %actor.level% < 100
      wait 3
      wsend %actor.name% The nymph says to you, 'You do not belong here.  Begone!'
      wteleport %actor.name% 1813
      wait 1s
      wat 1813 wsend %actor.name% Your vision fades, and you find yourself at the end of a gravel path.
   end
end
~
#1897
down _REMOVE_ME_ON_4k~
2 d 100
downme~
wexp %actor.name% -100000
wexp %actor.name% -100000
wexp %actor.name% -100000
wexp %actor.name% -100000
wexp %actor.name% -100000
~
#1898
experience_trigger_STRIP_ON_FOUR_k~
2 d 100
upme~
wexp %actor% 100000
wexp %actor% 100000
wexp %actor% 100000
wexp %actor% 100000
~
#1899
unused~
2 cd 100
reset_the_quest~
Nothing.
~
$~
