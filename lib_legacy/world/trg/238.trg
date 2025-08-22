#23800
dargentan-hug~
0 c 100
hug~
if %actor.align% > -350
msend %actor.name% The large dragon grins at you, displaying his teeth.
mechoaround %actor% %self.name% grins toothily at %actor.name%.
hug %actor.name%
else
growl %actor.name%
end
~
#23801
wake-dargentan~
0 c 100
wake~
    mutter
    msend %actor.name% %self.name% says, 'This had better be good!'
    mechoaround %actor.name% The dragon glares balefully at %actor.name%.
~
#23802
dargentan-sleep~
0 d 100
sleep nap naptime sleepytime~
yawn
wait 2s
msend %actor% %self.name% says, 'Very well, then.  I will go back to sleep.'
wait 1s
sleep
~
#23803
dar-return~
0 g 100
~
if %self.room% != 23892
blink
wait 1s
mecho &9&bDargentan slowly fades out of existence and is gone.&0
mteleport %self% 23892
mecho &7&bDargentan blinks into existence.&0
end
~
#23804
icicle-get~
1 g 100
~
if %actor.vnum% != 23814 && %actor.vnum% != 23815
   wait 1
   odamage %actor% 50 cold
   if %damdone% > 0
      osend %actor% The icicle freezes your fingers. (&4%damdone%&0)
   end
end
~
#23805
dargentan-battle~
0 k 25
~
   set val %random.10%
   switch %val%
   case 1
   breath frost
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
#23806
solar-greet~
0 g 100
~
if %actor.align% < -349
cast 'soulshield'
cast 'divine ray' %actor.name%
end
~
#23807
solar-battle~
0 k 15
~
if %random.10% < 5
cast 'divine ray'
end
~
#23808
mage_receive_refuse~
0 j 0
23822~
switch %object.vnum%
  case %wandgem%
  case %wandvnum%
  case %wandtask3%
  case %wandtask4%
    halt
    break
  default
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'What is this for?'
done
~
#23810
mage_greet~
0 g 100
~
wait 2
if %actor.quest_stage[%type%_wand]% == %wandstep%
  eval minlevel (%wandstep% - 1) * 10
  if %actor.level% >= %minlevel%
    if %actor.quest_variable[%type%_wand:greet]% == 0
      mecho %self.name% says, 'I see you're crafting something.  If you want my help, we can talk about &6&b[upgrades]&0.'
    else
      say Do you have what I need for the %weapon%?
    endif
  endif
endif
* Only greet people wearing the Sunfire crest
if %actor.wearing[23716]%
  wait 2s
  blink
  wait 2s
  bow %actor.name%
  wait 1s
  say Far be it from me to ask how you got that crest...  But it appears you have done a service for one of my kin.
  wait 2s
  say Perhaps you can help me also in solving a riddle.
endif
~
#23811
mage_speak1~
0 d 100
riddle quest hi hello~
wait 1s
frown
wait 2s
emote gestures at the symbols on the walls.
say There is a riddle here, yes.  It is...somewhat complicated.
emote takes a deep breath and begins to translate some runes.
wait 3s
say "Five friends from five towns liked five colors and stood in a line.  Each was a different race and class, and each fought a different enemy.  In my keep you will find all the information you need to solve the question I ask."
wait 3s
frown
wait 1s
emote gestures at the wall, looking frustrated.
say But the question referred to is obscured by ice.
~
#23812
mage_speak2~
0 d 100
question? question ice? ice~
* Start the quest and give the Tempest a blue flame
if !%get.mob_count[23803]%
   mat 23890 mload mob 23803
endif
if %get.mob_count[23803]%
    mat 23890 m_run_room_trig 23813
end
wait 1s
nod
wait 2s
say It is surely the revenge of the elemental that Lord Dargentan keeps imprisoned here.  The Tempest Manifest ensures that the Keep stays afloat, but such creatures are never happy about being kept in servitude.
wait 1s
emote thinks a moment.
wait 2s
say Such beings can never truly be destroyed, but perhaps if you banished his form from this realm for a while, the ice would clear up and the question would become visible.
wait 2s
emote nods confidently.
say Yes, perhaps that would be best.  If you would destroy the Tempest's mortal form and return to me with the remains, perhaps we could find the answer to this riddle.
wait 1s
frown
wait 2s
say But just in case I have not found all the correct clues, be sure to look for them throughout the keep.
emote points to a gently glowing inscribed pentagram.
wait 1s
say Look at the pentagrams scattered around.  I believe there are fifteen of them.  You will need to look at all of them to get the clues.
wait 2s
bow %actor.name%
say Good luck, and thank you for helping!
~
#23813
tempest_start_quest~
2 d 100
SecretCommandToStartQuest~
if !%has_flame%
    wforce tempest-manifest mload obj 23822
    wforce tempest-manifest hold blue-flame
    set has_flame 1
    global has_flame
end
~
#23814
tempest_reset_quest~
2 d 100
SecretCommandToResetQuest~
wforce tempest-manifest mjunk blue-flame
set has_flame 0
global has_flame
~
#23815
mage_receive~
0 j 100
23822~
* Reset quest in tempest room
if %get.mob_count[23803]%
    mat 23890 m_run_room_trig 23814
end
wait 1
mjunk blue-flame
wait 1s
mecho The mage's face brightens.
msend %actor% %self.name% says, 'You have banished the Tempest!  Excellent!  I think I can use this to remove the ice...'
emote murmers some arcane phrases and the symbols on the wall begin to glow.
wait 2s
msend %actor% %self.name% says, 'It says...
eval question %random.4%
global question
switch %question%
  case 1
    msend %actor% What is the race of the one from Ickle?'
  break
  case 2
    msend %actor% What is the race of the one who fought demons?'
  break
  case 3
    msend %actor% What is the class of the one who stood fourth in line from the right?'
  break
  case 4
    msend %actor% What is the favored enemy of the barbarian?'
  break
  default
    msend %actor% %self.name% says, 'Hrm... I can't seem to read it.'
    wait 2s
    frown
    wait 1s
    msend %actor% %self.name% says, 'Something is terribly wrong.  I think you should notify a god immediately.'
done
wait 2s
msend %actor% %self.name% says, 'Do you have the answer?'
~
#23816
mage_speak3~
0 d 100
barbarian human half-elf half-elven halfling gnome sorcerer cleric warrior rogue mercenary bugs bug dragons dragon orcs orc demons demon undead~
set halfelf half-elf
set halfelven half-elven
if %question%
  if %question% == 1
    if %speech% == barbarian
      set correct yes
    elseif %speech% == human || %speech% == %halfelf% || %speech% == halfling || %speech% == gnome || %speech% == %halfelven%
      set correct no
    endif
  elseif %question% == 2
    if %speech% == human
      set correct yes
    elseif %speech% == barbarian || %speech% == %halfelf% || %speech% == halfling || %speech% == gnome || %speech% == %halfelven%
      set correct no
    endif
  elseif %question% == 3
    if %speech% == rogue
      set correct yes
    elseif %speech% == sorcerer || %speech% == cleric || %speech% == warrior || %speech% == mercenary
      set correct no
    endif
  elseif %question% == 4
    if %speech% == orcs || %speech% == orc
      set correct yes
    elseif %speech% == bugs || %speech% == dragons || %speech% == demons || %speech% == undead || %speech% == bug || %speech% == dragon || %speech% == demon
      set correct no
    endif
  else
    wince
    say Something is terribly wrong...please inform a god.
  endif
  if %correct% == no
    set question 0
    if !%get.mob_count[23803]%
      mat 23890 mload mob 23803
    endif
    if %get.mob_count[23803]%
      mat 23890 m_run_room_trig 23813
    end
    global question
    emote swears profusely as the ice reforms over the symbols on the wall.
    wait 1s
    msend %actor% %self.name% casts a dirty look at you.
    mechoaround %actor% %self.name% scowls angrily at %actor.name%.
    say That was wrong!
    wait 2s
    sigh
    say Now the Tempest is back...  We would need to banish him again for another chance.
  elseif %correct% == yes
    set question 0
    global question
    wait 1s
    emote nods slowly and traces one of the signs.
    wait 1s
    emote smiles happily.
    say Excellent!  That is indeed the correct answer!
    wait 1s
    thank %actor.name%
    say You have helped me more than I can say.  How can I ever repay you?
    wait 2s
    emote gets a big grin on his face.
    mload obj 23816
    give glowing-key %actor.name%
    drop glowing-key
    bow %actor.name%
    wait 1s
    say Use it wisely!
  endif
endif
~
#23817
tempest_fight~
0 k 35
~
if %random.10% < 4
    wait 1s
    mecho The electrical charge around the Tempest grows stronger than the air can handle.
    wait 1
    mecho Powerful arcs of &b&4lightning&0 jump from the Tempest Manifest, saturating the air with energy!
    m_run_room_trig 23818
else
    wait 1s
    mecho Lightning crackles through the Tempest, electrifying the air around its body.
end
~
#23818
tempest_fight_room~
2 d 100
SecretCommandToHurtPeople~
*
* Lightning strikes half the number of people in the room
* but not necessarily different people (mwahaha)
*
eval howmany %people.23890% / 2
set count 1
set truecount 1
while %count% < %howmany%
    eval victim %random.char%
    if %victim.vnum% != 23803
        eval damage 150 + %random.30%
        if %victim.flags% /= SANCT
            eval damage %damage% - 75
        elseif %victim.flags% /= STONE
            eval damage %damage% - 75
        endif
        wdamage %victim% %damage% shock
        if %damage% == 0
            wsend %victim% &b&4The lightning arcs off the Tempest Manifest, but is repelled from you!&0
            wechoaround %victim% &b&4Lightning arcs off the Tempest Manifest, but is repelled from %victim.name%!&0
        elseif %victim.flags% /= MAJOR_GLOBE
            wsend %victim% &b&1The &4lightning &1rips through the shimmering globe around your body and right into you!&0 (&1&b%damdone%&0)
            wechoaround %victim% &b&1The shimmering globe around %victim.name%'s body wavers as the &4lightning &1rips through it.&0 (&3%damdone%&0)
        else
            wsend %victim% &b&4The lightning arcs off the Tempest Manifest and into your body, causing your muscles to spasm wildly!&0 (&1&b%damdone%&0)
            wechoaround %victim% &b&4Lightning arcs off the Tempest Manifest, jolting %victim.name% with massive force!&0 (&3%damdone%&0)
        endif
        eval count %count% + 1
    endif
    eval truecount %truecount% + 1
    if %truecount% > 15
        eval %count% %howmany%
    endif
done
~
#23819
dargentan_drop_treasure~
0 f 100
~
return 0
m_run_room_trig 23820
~
#23820
dargentan_chest_load~
2 d 0
donteverletmecatchyouusingthisspeechtrigger~
wload obj 23848
wecho As the dragon falls, you notice something that wasn't there before...
~
#23821
temptest-eq-drop~
0 f 100
~
return 0
mload obj 23825
drop pants
~
#23840
minstrel_test~
0 d 100
test~
   mecho Wheeeeeeee
   if (%self.worn[12]% == 48924)
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
   else
   say I'm not wearing that
   end
~
#23841
half-elf_encoded_trigger~
1 c 2
pentagram star inscribed circle~
if %actor.aff_flagged[COMP-LANG]%
osend %actor% The circle around this pentagram is not a line, but rather tiny script. It
osend %actor% reads: "The one who wore purple stood next to the half-elf".
else
osend %actor% this is a test message for now
end if
~
#23842
wall-refit~
2 h 100
~
   if (%object.vnum% == 23841)
      wait 1 s
wdoor 23841 south description   "The one who wore purple stood next to the half-elf".
wdoor 23841 south flags f
wecho The wall flares with a pure white light as the broken fragment is joined to the whole.
wecho The pentagram is completed.
wait 60s
wecho A piece of the wall slowly dissolves.
wdoor 23841 south description The wall is broken here, interrupting the pentagram carved into it.
wdoor 23841 south purge
end
~
#23857
painting-speech~
2 d 100
hi hello hi? greetings greeting~
if %actor.aff_flagged[DET_MAGIC]% == 1
   wsend %actor% The painting speaks to you. 'Oh, are you here to ask about the quest?'
   wechoaround %actor% The painting on the ceiling speaks softly in a foreign tongue to %actor.name%.
else
   wsend %actor% The painting speaks, but you do not feel that you understand its magical words.
end
~
#23858
painting-clue~
2 d 100
yes quest quest?~
if %actor.aff_flagged[DET-MAGIC]% == TRUE
  wsend %actor% The painting sighs. 'Yes, someone painted over the pentagram here.  Most unfortunate.  The clue read, "The one who fought dragons stood to the right of the sorcerer".'
  wait 1s
  wsend %actor% The painting says, 'I hope you find that useful.'
  wsend %actor% The painting goes back to studying the floor.
  wechoaround %actor% The painting speaks softly to %actor.name%.
else
  wecho The painting speaks in a magical language.
endif
~
#23875
genasi-port~
0 k 15
~
eval dest_room %random.37% + 23831
sigh
say I really don't have time for this right now.
mecho &9&b%self.name% slowly fades out of existence and is gone.&0
mteleport %self% %dest_room%
mecho &7&b%self.name% blinks into existence.&0
~
$~
