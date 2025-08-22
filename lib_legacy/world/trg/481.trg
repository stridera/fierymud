#48100
falling_from_48191~
2 g 100
~
if %direction% == west
   wait 5
   set rndm %random.char%
   if !(%rndm.eff_flagged[FLY]%)
      wsend %rndm% You feel your feet start slipping toward the pit!
      wait 3 s
      if %rndm.room% == 48191
         wsend %rndm% You slip into the pit!
         wechoaround %rndm% You watch in horror as %rndm.name% slips into the pit!
         wteleport %rndm% 48192
         wforce %rndm% look
      endif
      unset rndm
   endif
endif
~
#48101
open_volcano~
2 d 0
buntoi nakkarri karisto~
wat 48148 wecho There is a load groaning noise and a crack develops in the side of the volcano.
wat 48147 wecho There is a load groaning noise and a crack develops in the side of the volcano.
wait 2 s
wat 48147 wecho Even as you watch the crack widens to the size of a doorway.
wat 48148 wecho Even as you watch the crack widens to the size of a doorway.
wdoor 48147 north room 48148
wdoor 48147 north description A passage leads into the heart of the volcano.
wdoor 48148 south room 48147
wdoor 48148 south description You can see the sky through the doorway!
wait 10 s
wat 48147 wecho There is a load groaning noise and the doorway starts to close!
wat 48148 wecho There is a load groaning noise and the doorway starts to close!
wait 2 s
wat 48147 wecho The volcano shudders as it seals the entrance.
wat 48148 wecho The volcano shudders as it seals the entrance.
wdoor 48147 north purge
wdoor 48148 south purge
~
#48102
chimera_death~
0 f 100
~
mload obj 48122
mload obj 48128
mload obj 48129
mechoaround %actor% %self.name% lets out an arduous cry as %actor.name%'s attack lops its heads off!
msend %actor% %self.name% lets out a cry as your attack rends its heads from its body.
~
#48103
hold_bloody_paper~
1 j 100
~
if %actor.vnum% == -1
wait 4
osend %actor% You feel a burning pain in your hand as the magic tries to activate.
odamage %actor% 50
osend %actor% You gasp in pain and drop the paper.
oechoaround %actor% %actor.name% gasps in pain and drops the paper.
oforce %actor% rem bloody-parchment
oforce %actor% drop bloody-parchment
wait 1 s
osend %actor% The magic fails to activate, there is too much damage to the parchment.
oechoaround %actor% The parchment glows for a second then fades.
endif
~
#48104
hold_good_parchment~
1 j 100
~
if %actor.vnum% == -1
  wait 5
  osend %actor% You feel a slight tickling sensation as the parchment draws strength from you to activate.
  odamage %actor% 20
  osend %actor% You gasp in surprise and drop the paper.
  oechoaround %actor% %actor.name% gasps in surprise and drops the paper.
  oforce %actor% rem parchment-paper
  oforce %actor% drop parchment-paper
  wait 2
  oecho The parchment glows more and more brightly until you almost have to shield your eyes.
  if %actor.room% == 48197
    set room %self.room%
    if %room.people[48127]%
      oforce rock-monster say I can feel the magic working!
      set stage 5
      set person %actor%
      set i %person.group_size%
      if %i%
        set a 1
      else
        set a 0
      endif
      while %i% >= %a%
        set person %actor.group_member[%a%]%
        if %person.room% == %self.room%
          if %person.quest_stage[fieryisle_quest]% == %stage%
            quest advance fieryisle_quest %person.name%
            osend %person% &7&bYou have advanced your quest!&0
          endif
        elseif %person% && %person.vnum% == -1
          eval i %i% + 1
        endif
        eval a %a% + 1
      done
    endif
  endif
  wait 2 s
  oecho The parchment suddenly stops glowing.
endif
~
#48105
rock_monster_converting~
2 d 100
p I can feel the magic working!~
* this trigger only works if the rock-monster said the phrase
if %actor.vnum% == 48127
   wait 2 s
   wecho There is a swirling in the air and you see a face appear.
   wecho The face grins unpleasantly and then cackles.
   wait 1 s
   wecho The apparition says 'So, you dare to challenge Lokari?!'
   wecho The apparition glares at you.
   wait 1 s
   wecho The apparition says 'That was a pathetic piece of magic, did you think you could break my spell?  But I will remember that you tried...'
   wait 1 s
   wforce rock-monster emote wails in fear and pain as the transformation reasserts itself.
   wait 1 s
   wforce rock-monster emote turns to you in the instant before it returns to a pile of rock.
   wforce rock-monster mecho the rock monster says, 'Please kill me, and take the key I bear to Vulcera, my true love.'
   wait 1 s
   wecho The apparition says 'Pah, she could never love a mortal, your hearts are all made of stone.'
   wecho The apparition laughs at his own joke and fades.
endif
~
#48106
ash_crown-get~
1 g 100
~
if %actor.vnum% == -1
  set stage 3
  set person %actor%
  set i %person.group_size%
  if %i%
    set a 1
  else
    set a 0
  endif
  while %i% >= %a%
    set person %actor.group_member[%a%]%
    if %person.room% == %self.room%
      if %person.quest_stage[fieryisle_quest]% == %stage%
        quest advance fieryisle_quest %person.name%
        osend %person% &7&bYou have advanced your quest!&0
      endif
    elseif %person% && %person.vnum% == -1
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
endif
~
#48107
ivory-key-get~
1 gi 100
~
set stage 6
set person %actor%
set i %person.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %actor.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[fieryisle_quest]% == %stage%
      quest advance fieryisle_quest %person.name%
      osend %person% &7&bYou have advanced your quest!&0
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
if %victim.quest_stage[fieryisle_quest]% == %stage%
  quest advance fieryisle_quest %victim.name%
  osend %victim% &7&bYou have advanced your quest!&0
endif
~
#48108
Ash crown give~
1 i 100
~
if %victim.vnum% == -1
  if %victim.quest_stage[fieryisle_quest]% == 3
    quest advance fieryisle_quest %victim.name%
    osend %victim% &7&bYou have advanced your quest!&0
  endif
endif
~
#48109
wise_woman_give_parchment~
0 j 100
~
if %object.vnum% == 48124
  wait 2
  mjunk %object%
  set stage 4
  set person %actor%
  set i %person.group_size%
  if %i%
    set a 1
  else
    set a 0
  endif
  while %i% >= %a%
    set person %actor.group_member[%a%]%
    if %person.room% == %self.room%
      if %person.quest_stage[fieryisle_quest]% == %stage%
        set accept yes
        quest advance fieryisle_quest %person.name%
        msend %person% &7&bYou have advanced your quest!&0
      endif
    elseif %person% && %person.vnum% == -1
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
  if %accept% == yes
    if !%get.mob_count[48107]%
      mat 1100 mload mob 48107
      mat 1100 mforce ash-lord mload obj 48124
      mat 1100 mforce ash-lord wear ash-crown
      mteleport ash-lord 48157
    endif
    if !%get.mob_count[48127]%
      mat 48197 mload mob 48127
    endif
    emote almost looks happy for a moment.
    mecho %self.name% says, 'Thank you %actor.name%, killing the ash lord avenges many of my people who have died under his rule.  The only thing I have of any value is a spell of reversal, which can reverse a shape change spell.'
    emote seems to consider for a short while.
    wait 3s
    mecho %self.name% says, 'I already gave a copy of this to a young boy searching for his friend.'
    wait 3s
    sigh
    mecho %self.name% says, 'Perhaps he is dead now.  Here is another copy of it for you.'
    emote writes some words on a piece of parchment.
    mload obj 48125
    give parchment %actor.name%
    wait 4s
    mecho %self.name% says, 'Best of luck in finding whatever or whomever you have lost.
    wait 2s
    whap wise-woman
    mecho %self.name% says, 'I almost forgot, to activate the spell, you must hold the parchment.'
  else
    if %actor.quest_stage[fieryisle_quest]% > %stage%
      mecho %self.name% says, 'I am not going to write you another spell.'
    else
      * quest stage < 4 or not started..how did they get the crown?
      glare %actor.name%
      mecho %self.name% says, 'What is this insolence?  You hand me a crown without killing the Ash Lord.'
      emote crumbles the crown between her fingers.
      fume
    endif
  endif
endif
~
#48110
chimera_cast~
0 gk 30
~
   if %actor.vnum% == -1
   wait 2
   emote turns its dragon head to look at you.
   cast 'ray of enf' %actor%
   endif
~
#48111
bad_wedding_ring~
1 g 100
~
if %actor.vnum% != 48105
   wait 2
   oecho The ivory ring flares brightly!
   odamage %actor% 100 fire
   if %damdone% != 0
      osend %actor% You are burnt by the ring and drop it! (&1&b%damdone%&0)
      oechoaround %actor% %actor.name% shouts in surprise and pain. (&1&b%damdone%&0)
      oforce %actor% drop ivory-ring
      if %actor.room% == 48223
         oforce vulcera get ivory-ring
      endif
   end
endif
~
#48112
VULCERA_gets_ring~
2 h 100
~
if %object.vnum% == 48127
wait 2
wpurge ivory-ring
wteleport vulcera 48209
wat 48209 wforce ai say vulcera-load-ring
endif
~
#48120
Load rock monster~
2 g 100
~
if %actor.quest_stage[fieryisle_quest]% == 5 || %actor.quest_stage[fieryisle_quest]% == 6
  if !%get.mob_count[48127]%
    wload mob 48127
  endif
endif
~
#48121
give_ivory_vulcera~
0 j 100
48116~
if %actor.quest_stage[fieryisle_quest]% == 8
  wait 2
  quest advance fieryisle_quest %actor.name%
  msend %actor% &7&bYou have advanced your quest!&0
  if !%get.mob_count[48127]%
    mat 48197 mload mob 48127
  endif
  say You did well to find this, even with my powers I was unable to locate it.  On the other hand, you were a bit dumb to just hand it over to me!  It would seem Lokari taught me well.
  wait 3s
  unlock chest
  open chest
  get all chest
  emote examines the ivory ring.
  wait 2s
  say Pah, humans!
  if %self.inventory[48114]%
    mjunk ivory-ring
  endif
  mload obj 48127
  wear ring
  wait 2s
  say Heh, it still fits after all these years, but Lokari won't be happy.
  mjunk key
  peer %actor.name%
  wait 2s
  mload obj 48126
  wear cloak-fire
  say If you want this cloak, you better think again, begone mortal!
  mteleport all 48200
  mat 48200 mecho You can hear Vulcera's laughter ringing in your ears.
  mgoto 48223
endif
~
#48122
vulcera_speak2~
0 d 100
punishment punishment?~
wait 2
emote sighs loudly.
mecho %self.name% says, 'Many years ago, I fell in love with a mortal man who wanted to wed me, but Lokari found out.'
emot grimaces.
wait 1s
mecho %self.name% says, 'He was very jealous.  Even though I saw him as a father, he must have felt something more.'
wait 1s
mecho %self.name% says, 'I haven't seen my fiance since, although Lokari swore that he did not kill him, and gave him the key to this chest should he ever return to claim me.'
wait 1s
mecho %self.name% says, 'In this chest is the ring he gave me to proclaim his love, which Lokari left as a reminder of how fickle mortals can be.'
shrug
wait 1s
mecho %self.name% says, 'After all, if he really loved me, he would return and open the chest.'
emote sighs again.
wait 1s
say If only someone could give me the key to the chest.
~
#48123
Load ivory key~
0 o 100
~
mload obj 48116
~
#48124
vulcera_greet1~
0 gh 100
~
wait 6
peer %actor.name%
say So puny one, you dare to disturb me?
set stage 7
set person %actor%
set i %person.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %actor.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[%type%_wand]% == %wandstep%
      if %person.level% >= 60
        if %person.quest_variable[%type%_wand:greet]% == 0
          wait 2
          say What do you want pathetic mortal?
        else
          say Do you have what I require for the %weapon%?
        endif
      endif
    endif
    if %person.quest_stage[fieryisle_quest]% == %stage%
      quest advance fieryisle_quest %person.name%
      msend %person% &7&bYou have advanced your quest!&0
      if %person.inventory[48116]%
        wait 2
        emote does a double take at %person.name%.
        say Are you the one who has the key to the ivory chest?!
        wait 1s
        bow %person.name%
        say If so you could end my punishment, I would make it worth your while.
        wait 2s
        emote mutters a brief incantation and a cloak of flames appears.
        msend %person% &7&bGroup credit will not be awarded for the next step.&0
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#48125
pasty_receive1~
0 j 100
~
if %object.vnum% == 48101
  emote examines the shell.
  wait 2
  cry
  mecho %self.name% says, 'This is a wonderful gift from my father, he forgives me for my cowardice.'
  hol shell
  wait 2
  thanks %actor.name%
  mecho %self.name% says, 'You have brought me hope in this dark place, so I will help you.'
  wait 1s
  think
  mecho %self.name% says, 'There is a pit near here which is very deep, but it has a side tunnel partway down.'
  wait 1s
  smile
  say I hope you find this information useful.
endif
~
#48126
pasty_speak1~
0 d 100
help aid yes friend~
wait 2
mecho %self.name% says, 'Not long after I escaped in here, I met someone who had been changed into a pile of rock.'
wait 2
frown
mecho %self.name% says, 'It was strange though, it must have been Vulcera, but he never said a word against her.'
wait 1s
shrug
emote waves a bloody piece of paper in your face.
mecho %self.name% says, 'This was a spell of reversal to help him, but now I have no idea where he is.  Perhaps the wise woman will give you a spell too and we can search faster.'
set stage 1
set person %actor%
set i %person.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %actor.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[fieryisle_quest]% == %stage%
      quest advance fieryisle_quest %person.name%
      msend %person% &7&bYou have advanced your quest!&0
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#48127
pasty_greet1~
0 g 80
~
wait 2
if %actor.vnum% == -1
   emote mutters to himself.
   say Where has he gone?
   wait 2
   peer %actor.name%
   say Could you help me find my friend?
endif
~
#48128
UNUSED~
0 g 30
~
say Leave now, before Vulcera makes you hers!
scan
say Hurry!
~
#48129
warlord_speak1~
0 d 100
son?~
wait 2
mecho %self.name% says, 'Yes, he was kidnapped by the Bone Tribe head-hunters for a sacrifice.'
wait 1s
spit
mecho %self.name% says, 'Those headhunters are scum, and their chief is the worst of them.  If only I had warriors to spare, I could attack and kill him.'
glare
wait 1s
mecho %self.name% says, 'He carries my father's shriveled head on his belt, and now he has taken my son too.'
~
#48130
warlord_receive1~
0 j 100
~
if %object.vnum% == 48104
   wait 2
   mjunk severed-head
   smi
   say Ah, the enemy of my enemy is my friend.
   wait 1s
   sigh
   mecho %self.name% says, 'I cannot help but feel that my son is still alive, %actor.name%, but since none of my people have seen him he must be in the volcano.  I have too many responsibilities to leave and search for him.'
   sniff
   wait 1s
   mload obj 48101
   mecho %self.name% says, 'If you find him, please give him this, and do all you can to help him.'
   set person %actor%
   set i %person.group_size%
   if %i%
      set a 1
   else
      set a 0
   endif
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
        if !%person.quest_stage[fieryisle_quest]%
            quest start fieryisle_quest %person.name%
            msend %person% &7&bYou have now begun the Fiery Island quest!&0
        endif
        quest variable fieryisle_quest %person.name% shell 1
      elseif %person% && %person.vnum% == -1
        eval i %i% + 1
      endif
      eval a %a% + 1
   done
   give shell %actor.name%
   wait 2
   say The island shaman will know more.
endif
~
#48131
warlord_greet1~
0 g 100
~
if %actor.vnum% == -1
  emote looks up quickly as you enter.
  sigh
  wait 2
  mecho %self.name% says, 'I live in hope that my son will survive and return to me one day.'
endif
~
#48132
shaman_speak1~
0 d 100
son? warlord?~
wait 2
sigh
mecho %self.name% says, 'Yes, I remember the boy.  Small and pale skinned for his tribe, but he was fast enough when he got the chance to escape.  I was distracted for a second by Vulcera's pet and he nipped into the volcano I had opened.'
fume
wait 1s
mecho %self.name% says, 'I think the volcano god is getting angry now for lack of blood.  Will you help us?'
~
#48133
shaman_receive1~
0 j 100
~
if %object.vnum% == 48122
  wait 2
  mjunk head
  bow %actor.name%
  say You are indeed a mighty warrior.  Please have this gift as a token of my appreciation.
  mload obj 48123
  give stone-necklace %actor.name%
  wait 1s
  consider %actor.name%
  wait 1s
  set person %actor%
  set i %person.group_size%
  set levelcheck 0
  set queststart 0
  set lowlevel 0
  if %i%
    set a 1
  else
    set a 0
  endif
  while %i% >= %a%
    set person %actor.group_member[%a%]%
    if %person.room% == %self.room%
      if %person.level% >= 50
        set levelcheck 1
        if %person.quest_stage[fieryisle_quest]% <= 1
          if %person.quest_stage[fieryisle_quest]% == 0
            quest start fieryisle_quest %person.name%
            msend %person% &7&bYou have now begun the Fiery Island quest!&0
          endif
          set queststart 1
          quest variable fieryisle_quest %person.name% chimera dead
        endif
      else
        msend %person% &1You are too low level to continue this quest.&0
        set lowlevel 1
      endif
    elseif %person% && %person.vnum% == -1
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
endif
if %levelcheck%
  if %queststart%
    if %lowlevel%
      mecho %self.name% says, 'Some of you are still too inexperienced but...'
      wait 2s
    endif
    mecho %self.name% says, 'I wonder if you are mighty enough to take on Vulcera herself - we are sick of her spiteful acts against us.'
    wait 3s
    mecho %self.name% says, 'Of course, you would need the correct phrase to open the secret entrance to the volcano.  The phrase is &6&bbuntoi nakkarri karisto&0.'
    wait 4s
    mecho %self.name% says, 'If you find the son of the warlord inside, he may have gleaned some information about Vulcera.'
    wait 3s
    mecho %self.name% says, 'May the true volcano god look after you and help you destroy the usurper.'
    wait 5s
    mecho %self.name% says, 'If you forget what to do, you can come back to me and check on your &6&b[progress]&0.'
  else
    if %lowlevel%
      mecho %self.name% says, 'Some of you are still too inexperienced but...'
      wait 2s
    endif
    mecho %self.name% says, 'Remember, the mystic phrase to open the volcano is &6&b'buntoi nakkarri karisto'&0.  May the true volcano god guide you.'
  endif
else
  mecho %self.name% says, 'You are a bit inexperienced to take on Vulcera, although you have done well to kill the chimera.  Thank you for your help, %actor.name%.  Perhaps we will meet again when you have seen more of the world.'
endif
~
#48134
shaman_speak1~
0 d 100
help? aid? yes~
wait 2
mecho %self.name% says, 'Vulcera has prevented us from making our customary sacrifices by placing one of her...'
wait 2
ponder
wait 2
mecho %self.name% says, '"pets" in our sacred area before the volcano, and I fear that if we do not make a sacrifice soon...'
wait 1s
shrug
say Well, the volcano will most likely blow up the island.
sigh
wait 1s
mecho %self.name% says, 'If you can bring me the dragon head of the chimera, then I can reward you richly.'
wink %actor.name%
~
#48135
shaman_greet1~
0 g 100
~
wait 2
if %actor.vnum% == -1
  say Welcome, adventurer, have you come to help us in our time of need?
endif
~
#48136
headhunter_speak1~
0 d 100
son? warlord?~
wait 2
emote throws his head back and laughs heartily.
ponder %actor.name%
mecho %self.name% says, 'That wimp who dares to call himself a warlord sent you, did he?  Well you won't find his son here.'
wait 2
smirk
mecho %self.name% says, 'We gave him to the shaman for a sacrifice to the volcano god.'
wait 2
emote looks thoughtful.
wait 2
mecho %self.name% says, 'Although, the god does not seem to have been appeased yet, perhaps we need another candidate.'
poke %actor.name%
~
#48137
chimera_greet1~
0 g 100
~
*
*Added by Acerite oct 2004
*
*Check for moonwell quest
*
if %actor.quest_stage[moonwell_spell_quest]% == 1
   mjunk vine
   mload obj 16350
endif
emote turns its dragon head to examine you.
consider %actor.name%
if %actor.level% < 30
   cast 'dispel magic' %actor.name%
else
   cast 'ray of enf' %actor.name%
endif
~
#48138
chimera_fight1~
0 k 30
~
msend %actor.name% YOWZER, the dragon head's fiery breath singed the top of your head!
mechoaround %actor.name% The dragon head misses %actor.name% with its fiery breath, but their hair smokes slightly!
~
#48139
chimera_fight2~
0 k 30
~
msend %actor.name% The chimera's lion head snaps at your legs, but you dodge.
mechoaround %actor.name% %actor.name% dodges as the chimera's lion head snaps at them.
~
#48140
chimera_fight3~
0 k 30
~
msend %actor.name% The chimera's goat head snaps at you, just missing your arm!
mechoaround %actor.name% %actor.name% sweats profusely as the chimera's goat head almost takes off their arm!
~
#48141
elder_woman_speak1~
0 d 1
spell spell? help help?~
wait 2
set stage 2
set person %actor%
set i %person.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %actor.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[fieryisle_quest]% == %stage%
      quest advance fieryisle_quest %person.name%
      msend %person% &7&bYou have advanced your quest!&0
      set ash 1
    elseif %person.quest_stage[fieryisle_quest]% == 3 || %person.quest_stage[fieryisle_quest]% == 4
      set ash 2
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
if %ash% == 1
  mecho %self.name% says, 'If you want me to help you, then you must do me a favor.  Kill the ash lord and bring me his crown.'
  if %get.mob_count[48107]% < 1
    mat 1100 mload mob 48107
    mat 1100 mforce ash-lord mload obj 48124
    mat 1100 mforce ash-lord wear crown
    mat 1100 mteleport ash-lord 48157
  endif
elseif %ash% == 2
  say Bring me the ash lord's crown first.
else
  say I have no idea what you are talking about.
  eye %actor.name%
endif
~
#48142
container_mob_rand1~
0 b 100
~
give flag ai
mpurge container-mob
~
#48143
vulcera_dead~
0 df 100
run test~
mecho The ivory ring seems to shimmer for a second.
if %self.room% != 48223
  mteleport all 48223
  mecho &0  
  mecho &1&bA burning hole erupts, sucking everything through it!&0
  mecho &0  
  set room %get.room[48223]%
  set person %room.people%
  while %person%
    if %person.vnum% == -1
      mforce %person% look
      set person %person.next_in_room%
    endif
  done
endif
set person %actor%
set i %actor.group_size%
if %i%
   set a 1
else
   set a 0
endif
unset person
while %i% >= %a%
  set person %actor.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[fieryisle_quest]% == 9
      quest variable fieryisle_quest %person% reward yes
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
*
* Complete Fiery Island
*
m_run_room_trig 48145
~
#48144
shaman_speak3~
0 d 100
who? Vulcera Vulcera?~
wait 2
mecho %self.name% says, 'Vulcera is some kind of fiery demigoddess.  She appeared many years ago and took the volcano as her new palace.'
wait 2s
mecho %self.name% says, 'Vulcera enslaved the dwarrow, the gnome-dwarves of the Mountain Tribe who live inside the volcano itself, and forced them to build her an ivory tower.'
wait 2s
mecho %self.name% says, 'She quickly laid claim to the rest of the island as her dominion.  Vulcera claimed she was the new volcano god demanded all sacrifices be made to her.  She slaughters anyone who displeases or disobeys along with their whole family.'
wait 3s
mecho %self.name% says, 'But the true god grows angry.  Without our sacrifices, he will surely destroy the entire island.  The people of the Ocean Tribe turned to cannibal rituals to protect themselves.  We of the Jungle Tribe wish Vulcera removed but are not strong enough to destroy a demigoddess.'
~
#48145
Fiery_Island_Quest_Grant_rewards~
2 a 100
~
wait 1
set person %self.people%
while %person%
   if %person.quest_variable[fieryisle_quest:reward]% == yes && !%person.has_completed[fieryisle_quest]%
      wsend %person% You notice special glittering gems amongst the chamber's crystals!
      *
      * Set X to the level of the award - code does not run without it
      * Fiery Island, X = 55
      if %person.level% < 55
         set expcap %person.level%
      else
         set expcap 55
      endif
      set expmod 0
      if %expcap% < 9
         eval expmod (((%expcap% * %expcap%) + %expcap%) / 2) * 55
      elseif %expcap% < 17
         eval expmod 440 + ((%expcap% - 8) * 125)
      elseif %expcap% < 25
         eval expmod 1440 + ((%expcap% - 16) * 175)
      elseif %expcap% < 34
         eval expmod 2840 + ((%expcap% - 24) * 225)
      elseif %expcap% < 49
         eval expmod 4640 + ((%expcap% - 32) * 250)
      elseif %expcap% < 90
         eval expmod 8640 + ((%expcap% - 48) * 300)
      else
         eval expmod 20940 + ((%expcap% - 89) * 600)
      endif
      *
      * Adjust exp award by class so all classes receive the same proportionate amount
      *
      switch %person.class%
         case Warrior
         case Berserker
            *
            * 110% of standard
            * 
            eval expmod (%expmod% + (%expmod% / 10))
            break
         case Paladin
         case Anti-Paladin
         case Ranger
            *
            * 115% of standard
            *
            eval expmod (%expmod% + ((%expmod% * 2) / 15))
            break
         case Sorcerer
         case Pyromancer
         case Cryomancer
         case Illusionist
         case Bard
            *
            * 120% of standard
            *
            eval expmod (%expmod% + (%expmod% / 5))
            break
         case Necromancer
         case Monk
            *
            * 130% of standard
            *
            eval expmod (%expmod% + (%expmod% * 2) / 5)
            break
         default
            set expmod %expmod%
      done
      wsend %person% &3&bYou gain experience!&0
      eval setexp (%expmod% * 10)
      set loop 0
      while %loop% < 10
         *
         * Xexp must be replaced by mexp, oexp, or wexp for this code to work
         * Pick depending on what is running the trigger
         * Fiery Island, xexp = wexp
         wexp %person% %setexp%
         eval loop %loop% + 1
      done
      set gem 0
      while %gem% < 3
         eval drop %random.11% + 55736
         wload obj %drop%
         eval gem %gem% + 1
      done
      wforce %person% get all.gem
      quest complete fieryisle_quest %person.name%
   endif
    set person %person.next_in_room%
done
~
#48199
fieryisle_quest_status_checker~
0 d 100
progress progress? status status?~
set stage %actor.quest_stage[fieryisle_quest]%
wait 2
if %actor.level% < 55
  say You aren't ready to take on the volcano goddess yet.
  halt
endif
if %actor.has_completed[fieryisle_quest]%
  mecho %self.name% says, 'You have freed us from Vulcera's tyranny!  The true
  mecho &0volcano god is pleased!'
elseif %stage% > 0
  say Currently you are trying to:
  switch %stage%
    case 1
      mecho Enter the volcano and find the warlord's son.
      mecho Defeat the guardian to learn the mystic phrase to open the volcano.
      break
    case 2
      mecho Find the dwarrow woman and ask for a spell of reversal.
      break 
    case 3
      mecho Kill the ash lord and retrieve his crown.
      break
    case 4
      mecho Return the crown of the ash lord to the dwarrow woman.
      break
    case 5
      mecho Find the person turned to rock and hold the spell the dwarrow woman gave you.
      break
    case 6
      mecho Retrieve the ivory key.
      break
    case 7
      mecho Find Vulcera in the ivory tower.
      break
    case 8
      mecho Give the ivory key to Vulcera.
      break
    case 9
      mecho Defeat Vulcera!
      break
  done
  if %stage% > 1
     mecho The mystic phrase to open the volcano is &7&bbuntoi nakkarri karisto&0.
  endif
else
  say You are not on this quest yet.
endif
~
#48200
Jemnon Load Trigger~
2 g 100
~
if %actor.quest_stage[meteorswarm]% == 1
   if %actor.quest_variable[meteorswarm:bar_num]% == %self.vnum% && !%get.mob_count[48200]%
      wload mob 48200
      wait 2
      wforce Jemnon emote mutters something about %actor.class%s.
   endif
endif
~
#48201
jemnon_greet~
0 g 100
~
if %actor.vnum% == -1
  wait 2s
  msend %actor% %self.name% says, 'Aaaaaaaaand who - '
  wait 4
  emote hiccups.
  wait 4
  msend %actor% %self.name% says, '- are you?'
  wait 2s
  msend %actor% %self.name% says, 'You 'ere to 'ear the tales o' Jemnon the Lionhearted?!'
  wait 1s
  msend %actor% %self.name% says, 'Yeah?'
  wait 1s
  msend %actor% %self.name% says, 'Yeah??'
  nudge %actor%
endif
~
#48202
jemnon_speak1~
0 d 100
yes no~
wait 4
if %speech% == yes
  msend %actor% Very inebriated, %self.name% says, 'Then you come t' the right place!  Pull up a chair an' siddown!'
  wait 2s
  msend %actor% %self.name% says, 'Now, whaddya wan' kno 'bout??'
else
  msend %actor% %self.name% says, 'Well I dun wanna talk t' yous anyway!'
  Emote crosses his arms in a huff and turns away.
endif
~
#48203
jemnon_speak2~
0 d 100
rock monster stone guardian demon~
wait 2
if %actor.quest_stage[meteorswarm]% == 1
  msend %actor% %self.name% says, 'Itsss made outta rocks!  So much rocks!  A rack MONSTER!'
  wait 1s
  emote looks confused.
  wait 1s
  msend %actor% %self.name% says, 'Nuuuuuhhh a ROOK mahnster!'
  wait 1s
  shake
  wait 1s
  msend %actor% %self.name% says, 'Uuuuhhhhhh ROCK monster!  Yeah, a rock monster.  Terrible place I saw it.'
endif
~
#48204
jemnon_speak3~
0 d 1
where? place? where place~
wait 2
if %actor.quest_stage[meteorswarm]% == 1
  quest advance meteorswarm %actor.name%
  msend %actor% %self.name% says, 'Ooohhhhh you wann kno wheeeeere itis?!'
  msend %actor% %self.name% angrily shouts, 'You outta jus' say so!!'
  wait 2s
  emote blinks drunkenly.
  wait 2s
  msend %actor% %self.name% says, 'Wat wuz I sayin'?'
  wait 1
  msend %actor% %self.name% says, 'Oh yeah, the ROOK MONSTER.'
  wait 2
  msend %actor% %self.name% says, 'Itz in a hole.'
  wait 1
  msend %actor% %self.name% says, 'Yep.  Great big hole.'
  wait 2s
  emote becomes deathly still.
  wait 6s
  msend %actor% In a suddenly clear voice, %self.name% speaks.  'I remember the smoke and ruins of the city. A fountain of blood gushed up from the town square.  I avoided the demons as best I could, but by accident I stumbled into a collapsed house at the end of a road...'
  wait 2
  shiver
  wait 4s
  msend %actor% %self.name% says, 'That's where I saw it, in a cave under the rubble...'
  wait 4s
  msend %actor% %self.name% says, 'I wish I could forget...'
  cry
  wait 3s
  msend %actor% %self.name% says, 'Please, let me forget...'
  emote quickly downs another drink.
  wait 4s
  sleep
  wait 20s
  wake
  emote looks around very confused.
endif
~
#48205
meteorite_get~
1 g 100
~
if %actor.quest_stage[meteorswarm]% == 2
  quest advance meteorswarm %actor.name%
endif
~
#48249
McCabe refuse~
0 j 0
48252 48251~
switch %object.vnum%
  case %wandgem%
  case %wandtask3%
  case %wandtask4%
  case %wandvnum%
    halt
    break
  default
    return 0
    eye
    wait 1s
    msend %actor% %self.name% tells you, 'And what exactly am I supposed to do with this?'
done
~
#48250
burning wall~
2 g 100
~
if %actor.level% < 100
   if %direction% /= south
      if %actor.class% /= Sorcerer || %actor.class% /= Pyromancer || %actor.quest_stage[fire_wand]% > 7
         wsend %actor% The flames and lava bend around you as you pass through unharmed. 
      else
         eval damage (%random.40% + 40)
         wdamage %actor% %damage% fire
         wsend %actor% The flames scorch you as you try to approach the wall. (%damdone%)
         wsend %actor% You seem to hear a voice whisper, 'This room is not for you.'
         return 0
      endif
   endif
endif
~
#48251
meteor grants sworm~
1 n 100
~
if %actor.quest_stage[meteorswarm]% == 5
  wait 1
  oskillset %actor.name% meteorswarm
  quest complete meteorswarm %actor.name%
  osend %actor% &1&bYou have learned the ability to call meteors from the sky!&0
  wait 1
  oecho The meteorite crumbles to ash and blows away.
  opurge %self%
endif
~
#48252
mccabe's fight trig~
0 k 100
~
set line %random.10%
if %line% == 1
   say You are a fool!
elseif %line% == 2
   say Do you really think you can match my power?!
elseif %line% == 3
   say It would have been wise to try to learn from me.
   say ...But now you must die!
endif
set heat !HEAT
if %actor.eff_flags% /= %heat% || %actor.eff_flagged[sanct]%
   if %random.2% == 2
      say Your magic is powerful, but not powerful enough to stop this force!
   endif
   c 'meteorswarm' %actor%
elseif %actor.eff_flagged[stone]%
   c 'dispel mag' %actor%
elseif %actor.eff_flagged[coldshield]%
   c 'melt' %actor%
elseif %actor.eff_flagged[fireshield]%
   c 'immolate' %actor%
else
   switch %random.3%
      case 1
         c 'fireball' %actor%
      case 2
         c 'acid burst' %actor%
      default
         c 'positive field' %actor%
   done
endif
wait 2s
~
#48253
mccabe greet~
0 g 100
~
***McCabe's greet Trigger***
***
set stage %actor.quest_stage[meteorswarm]%
wait 1s
***No one should be here but sorcs and pyros, so lets throw the rest back out***
if %actor.class% != sorcerer && %actor.class% != pyromancer && %actor.quest_stage[%type%_wand]% < %wandstep%
  con %actor%
  wait 2s
  say What are you doing here?! Get out!
  wait 1s
  mechoaround %actor% &4&b%self.name% &0&6 sends a blast of air at &9&b%actor.name%&6 sending %actor.o%&b tumbling!&0
  msend %actor% &4&b%self.name% &0&6 sends a blast of air at &1&bYOU&6 sending you&b tumbling!&0
  mteleport %actor% 48254
  mforce %actor% look
  msend %actor% &4&b%self.name%'s &0&6blast of air sends you&b tumbling!&0
  mteleport %actor% 48253
  mforce %actor% look
  msend %actor% &4&b%self.name%'s &0&6blast of air sends you&b tumbling!&0
  mteleport %actor% 48252
  mforce %actor% look
  msend %actor% &4&b%self.name%'s &0&6blast of air comes at you from a new angle, sending you&b tumbling!&0
  mteleport %actor% 48251
  eval damage (%actor.level% * 2) + %random.30%
  mdamage %actor% %damage% fire
  msend %actor% &1&bYou sustain &0&1severe burns&b as you plunge head first through drizzling &0&1lava.&0 (&1&b%damdone%&0)
  mforce %actor% look
elseif (%actor.class% /= sorcerer && %actor.level% > 72) || (%actor.class% /= pyromancer && %actor.level% > 80) || %actor.quest_stage[%type%_wand]% == %wandstep%
  eval minlevel (%wandstep% - 1) * 10
  if (%actor.class% /= sorcerer && %actor.level% > 72) || (%actor.class% /= pyromancer && %actor.level% > 80)
    if %actor.quest_variable[meteorswarm:new]% /= no
      msend %actor% %self.name% tells you, 'Do you have the new meteorite?'
    elseif !%stage%
      mload mob 48251
      peer seagull
      emote focuses closely on the seagull flying overhead.
      mecho %self.name% starts casting '&3&bmeteorswarm&0' at an unsuspecting seagull.
      wait 1s
      m_run_room_trig 48257
      mforce seagull panic
      mforce seagull emote panics and flees to the north.
      mpurge seagull
      wait 2s
      giggle
      wait 5s
      msend %actor% %self.name% tells you, 'I see you admire my conjuring.  Perhaps you've never seen anyone calling &6&bmeteors&0?'
    elseif %stage% == 1
      msend %actor% %self.name% tells you, 'Have you spoken to Jemnon?  He's in some tavern, no doubt, waiting for some new blunder to embark on.'
    elseif %stage% == 2
      msend %actor% %self.name% tells you, 'Well, did Jemnon tell you where the rock demon is?'
    elseif %stage% == 3
      msend %actor% %self.name% tells you, 'Have you found a suitable focus?'
    elseif %stage% == 4
      msend %actor% %self.name% tells you, 'I can see from the singe marks you accomplished your task!  Are you ready to press on?'
    elseif %stage% == 5
      msend %actor% %self.name% tells you, 'Were you able to glean something from Dargentan's teachings?'
    endif
    if %actor.quest_stage[%type%_wand]% == %wandstep%
      if %actor.level% >= %minlevel%
        if %actor.quest_variable[%type%_wand:greet]% == 0
          msend %actor% %self.name% tells you, 'Or is there even MORE you want from me?  You seem to be in need of a crafting &6&b[upgrade]&0.'
        else
          msend %actor% %self.name% says, 'Do you have what I need for the %weapon%?'
        endif
      endif
    endif
  else
    if %actor.level% >= %minlevel%
      if %actor.quest_variable[%type%_wand:greet]% == 0
        msend %actor% %self.name% tells you, 'I see you're crafting something.  If you want my help, we can talk about &6&b[upgrades]&0.'
      else
        msend %actor% %self.name% tells you, 'Do you have what I need for the %weapon%?'
      endif
    endif
  endif
endif
~
#48254
mccabe load~
0 o 100
~
mskillset %self% meteorswarm
mskillset %self% major globe
~
#48255
Mccabe casts swarm~
0 b 30
~
m_run_room_trig 48257
wait 10 s
~
#48256
mccabe dialog~
0 d 100
meteor meteors meteors? meteorswarm yes no~
***McCabes Speech Trigger***
*This trigger represents the launching point, ending point, and intersection of the 3 parts of the meteor quest.
*stage 1 deals with finding Jemnon
*stage 2 is getting a mastery of earth, and ends back here.
*stage 3 sends the quester off to the fire temple high priest, and ends back here.
*stage 4 sends the quester to dargentans lair to master air, which then ends back here.
*stage 5 means the player has mastered all 3 elements, combined them into an item, and can now cast a meteorswarm with that item.
*the quest is actually completed by the item itself, when the player casts its one and only charge of meteorswarm.
set stage %actor.quest_stage[meteorswarm]%
wait 1s
if (%actor.class% /= sorcerer || %actor.class% /= pyromancer) && %actor.level% > 72
   if !%stage% && (%speech% /= meteor || %speech% /= meteors || %speech% /= meteors? || %speech% /= no)
      msend %actor% %self.name% tells you, 'Yes, they are the culmination of my life's work.  They are the perfect balance of earth, fire, and air.'
      wait 2s
      msend %actor% %self.name% tells you, 'Now leave me to my fun.  I have a seagull to obliterate.'
      wait 3s
      msend %actor% %self.name% tells you, 'If you like, you may stand by and observe.'
      wait 3s
      mecho %self.name% starts casting '&3&bcelneptin&0'...
      wait 2s
      m_run_room_trig 48257
      wait 3s
      sigh
      msend %actor% %self.name% tells you, 'I suppose you wish to do more than observe, am I right?  %actor.name%, would you like to try to learn this rather difficult spell?'
   elseif %speech% /= yes
      if %actor.quest_variable[meteorswarm:new]% /= no
         say Show me the meteorite.
      elseif !%stage%
         quest start meteorswarm %actor.name%
         msend %actor% %self.name% tells you, 'Yes, well first you need a powerful piece of earth.  Not just any rock will do.  It needs to be positively alive with great energy.'
         wait 2s
         consider %actor%
         wait 2s
         msend %actor% %self.name% tells you, 'I doubt that you would be able to summon such earth yourself...  At least, not until you have mastered the element.'
         wait 5s
         ponder
         wait 4s
         msend %actor% %self.name% tells you, 'There was once a powerful mage that created a massive guardian of stone to bar the way into the Nine Hells.  Such a creature would be a perfect source of powerfully imbued earth.'
         dream
         wait 3s
         msend %actor% %self.name% tells you, 'Unfortunately, I do not know where the location of that entry is.  But I know someone who might.'
         wait 1s
         grumble
         wait 5s
         msend %actor% %self.name% tells you, 'There's a drunken lout, goes by the name Jemnon the Lionhearted.'
         wait 2s
         msend %actor% %self.name% tells you, 'His exploits are as idiotic as the greatest heroes are brave.  While he causes far more problems than he solves, he has stumbled upon more secret knowledge than I care to admit.'
         wait 3s
         msend %actor% %self.name% tells you, 'He may very well know the location of this rocky behemoth.'
         wait 5s
         msend %actor% %self.name% tells you, 'Your first challenge will be &6&blocating Jemnon&0.  He's almost certainly drinking himself stupid in a &6&bbar&0 or &6&btavern&0 somewhere.  If you do find him, you'll have to &6&bask him about the rock monster&0.'
         switch %random.13%
            case 1
               * Tavern of the Fallen Star in Southern Borderhold
               set bar_num 2334
               break
            case 2
               * Sloshed Squirrel in Mielikki
               set bar_num 3033
               break
            case 3
               * Forest Tavern in Mielikki
               set bar_num 3053
               break
            case 4
               * Ole Witch Tavern in Anduin
               set bar_num 6037
               break
            case 5
               * Phillips Backdoor Bar in Anduin
               set bar_num 6044
               break
            case 6
               * Red Feathered Nest Anduin
               set bar_num 6054
               break
            case 7
               * Shawns Tavern in Anduin
               set bar_num 6112
               break
            case 8
               * Drunken Ogre Inn in Anduin
               set bar_num 6131
               break
            case 9
               * Golden Goblet Inn and Tavern in Anduin
               set bar_num 6226
               break
            case 10
               * Karrs Pub in Ickle
               set bar_num 10029
               break
            case 11
               * Mermaid's Tail in Ogakh
               set bar_num 30008
               break
            case 12 
               * Biergarten in Ogakh
               set bar_num 30089
               break
            default
               *Flirting Puppy
               set bar_vnum 51045
         done
         quest variable meteorswarm %actor.name% bar_num %bar_num%
         wait 2s
         msend %actor% %self.name% tells you, 'You can check on your &6&b[spell progress]&0 with me at any time.'
      elseif %stage% == 2
         msend %actor% %self.name% tells you, 'Magnificent!  Bring back a piece of it as a magical focus and I can continue teaching you this spell.'
      elseif %stage% == 3
         msend %actor% %self.name% tells you, 'Splendid!  May I see it?'
         wait 2s
      elseif %stage% == 4
         quest variable meteorswarm %actor% fire 2
         msend %actor% %self.name% tells you, 'The last element you must master is the sky itself.  But this must be no mere demonstration of the power of flight, or even a large gust of wind.'
         wait 2s
         msend %actor% %self.name% tells you, 'No, in order to understand the sheer magnitude of energy involved, you will need to learn from a true master of air.'
         wait 3s
         msend %actor% %self.name% tells you, 'But who would be most appropriate...?'
         think
         wait 4s
         msend %actor% %self.name% tells you, 'Wait, I've got it!  It's a long shot...'
         wait 1s
         msend %actor% %self.name% tells you, 'But...'
         wait 1s
         msend %actor% %self.name% tells you, 'You should &6&bmeet with the dragon who created the floating fortress in southern Caelia&0.'
         wait 2s
         msend %actor% %self.name% tells you, 'Ask him to teach you.  And be certain to show him the proper respect when you do!'
         wait 2s
         msend %actor% %self.name% tells you, 'Good luck!  You'll need it...'
      elseif %stage% == 5
         msend %actor% %self.name% tells you, 'Let me check the meteorite.'
      endif
   endif
endif
~
#48257
meteor splash~
2 a 100
~
wecho McCabe completes his spell...
wecho McCabe closes his eyes and utters the words, 'meteorswarm'
wecho &1McCabe conjures up a controlled shower of meteors &9&bwhich &4splash &0&6harmlessly down in the ocean.&0
wat 48252 wecho &1A swarm of burning &9&bmeteors stream down from the sky, &4&bsplashing &0&6harmlessly into the ocean.&0
wat 48253 wecho &1A swarm of burning &9&bmeteors stream down from the sky, &4&bsplashing &0&6harmlessly into the ocean.&0
wat 48254 wecho &1A swarm of burning &9&bmeteors stream down from the sky, &4&bsplashing &0&6harmlessly into the ocean.&0
~
#48258
mccabe_receive~
0 j 100
48252 48251~
set stage %actor.quest_stage[meteorswarm]%
set earth %actor.quest_variable[meteorswarm:earth]%
set air %actor.quest_variable[meteorswarm:air]%
if %object.vnum% == 48251
  return 0
  shake
  wait 2
  msend %actor% %self.name% tells you, 'Go &6&buse&0 this new treasure; don't give it back to me!'
elseif %actor.quest_variable[meteorswarm:new]% /= no
  return 0
  mecho %self.name% accepts the meteorite.
  wait 2
  msend %actor% %self.name% tells you, 'Good, this will work.'
  mecho %self.name% returns %object.shortdesc%.
  wait 2s
  msend %actor% %self.name% tells you, 'Go, resume your studies.'
  quest variable meteorswarm %actor% new 0
else
  switch %stage%
    case 1
      return 0
      eye
      wait 1s
      msend %actor% %self.name% tells you, 'No, go find Jemnon first.'
      break
    case 2
      return 0
      eye
      wait 1s
      msend %actor% %self.name% tells you, 'No, go find the rock demon first.'
      break
    case 3
      if %earth% == 0
        return 0
        msend %actor% You give %object.shortdesc% to %self.name%.
        msend %actor% %self.name% tells you, 'Such wonderfully powerful stone.  This will do perfectly.'
        wait 3s
        msend %actor% %self.name% tells you, 'Next, you need to demonstrate mastery over fire.'
        wait 2s
        msend %actor% %self.name% tells you, 'The northern cult of fire provides an excellent opportunity to do so.'
        wait 1s
        msend %actor% %self.name% tells you, 'Take the meteor, &6&bslay the high priest, and find the secret lava well&0.'
        wait 3s
        msend %actor% %self.name% tells you, 'The charge from such energy will be ideal for conjuring.'
        wait 2
        msend %actor% %self.name% returns %object.shortdesc% to you.
        quest variable meteorswarm %actor.name% earth 1
      else
        return 0
        msend %actor% %self.name% tells you, 'Yes yes, you have already proven it's a lovely focus.'
      endif
      break
    case 4
      return 0
      eye
      wait 1s
      msend %actor% %self.name% tells you, 'No, go find Dargentan first.'
      break
    case 5
      if %air% == 0
        mjunk meteorite
        wait 1s
        msend %actor% %self.name% tells you, 'This meteorite, representing the force of earth, has been imbued with the spirits of fire and air.  It will serve as the perfect focus for casting meteorswarm.'
        wait 4s
        msend %actor% %self.name% tells you, 'You are ready.'
        wait 3s
        mload obj 48251
        give meteorite %actor.name%
        wait 1s
        msend %actor% %self.name% tells you, 'Go somewhere safe, and &6&buse it&0 to unlock its potential.'
        quest variable meteorswarm %actor.name% air 1
      else
        return 0
        msend %actor% %self.name% tells you, 'Where did you find a second one of these?  You don't need it.'
      endif
      break
    default
      return 0
      wait 1s
      eye
      msend %actor% %self.name% tells you, 'And what exactly am I supposed to do with this?'
  done
endif
~
#48259
fire_priest_death~
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
      if %person.quest_stage[meteorswarm]% == 3
        quest variable meteorswarm %person.name% fire 1
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
elseif %person.quest_stage[meteorswarm]% == 3
  quest variable meteorswarm %person.name% fire 1
endif
~
#48260
dargentan_meteorswarm_quest_speak1~
0 d 100
teach help meteor meteorswarm meteorite air~
wait 1s
if %actor.quest_stage[meteorswarm]% == 4
  if %actor.align% > -349
    nod
    msend %actor% %self.name% says, 'The depths of thy training doth shine through.  I shalt teach thee for thou art stout and bold.'
    wait 3s
    msend %actor% %self.name% raises up, wings spread.
    wait 2
    msend %actor% %self.name% begins to cast...
    wait 3s
    msend %actor% The sky swirls around the lair as all the crystals begin to hum and glow.
    msend %actor% You hear the music of the sky sing in your ears.
    wait 1s
    msend %actor% &7&bThe meteorite begins to hum in tune.&0
    wait 4s
    msend %actor% %self.name% says, 'Dost thine ears perceive thus?  Tis the music of the spheres!  Remember it upon thy conjuring!'
    msend %actor% &7&bYou feel your mastery of the air growing!&0
    quest advance meteorswarm %actor.name%
    wait 5s
    say Alloweth this one to return thus to rest anon.
    wait 2
    sleep
  else
    say I shall help thee... TO SOAR 'MONGST THE BIRDS!!
    wait 2
    msend %actor% %self.name% grabs you in his mighty claws and throws you bodily from the tower!
    mechoaround %actor% %self.name% grabs %actor.name% in his mighty claws and throws %actor.himher% bodily from the tower!
    mforce %actor% sit
    mteleport %actor% 23893
    wait 8s
    t %actor% I doth desire thee didst enjoy thy lesson!
    wait 1s
    msend %actor% &7&bThe meteorite begins to hum!
    msend %actor% Strangely enough, you do feel you have learned something significant about the power of the air.&0
    quest advance meteorswarm %actor.name%
  endif
endif
~
#48261
jemnon_exit~
0 q 100
~
if %actor.quest_stage[meteorswarm]% == 2
  mecho %self.name% wanders off.
  mpurge %self% 
endif
~
#48262
meteorswarm_dargentan_bow~
0 c 100
bow~
if %arg% /= dargentan || %arg% /= dar || %arg% /= huge || %arg% /= large || %arg% /= dragon || %arg% /= sleeping || %arg% /= peaceful
  msend %actor% You bow before him.
  mechoaround %actor% %actor.name% bows before Dargentan.
  if %actor.quest_stage[meteorswarm]% == 4
    wait 2
    glare
    msend %actor% %self.name% says, 'Wherefore hast thou awakened one such as this from deepest slumber?'
  endif
else
  return 0
endif
~
#48263
meteorswarm_new_meteorite_replacement~
0 d 0
I lost the meteorite~
if %actor.quest_stage[meteorswarm]% > 2 && !%actor.quest_variable[meteorswarm:air]%
  wait 2
  gasp
  msend %actor% %self.name% says, 'WHAT?!'
  wait 1s
  msend %actor% %self.name% says, 'How could you be so careless??''
  wait 3s
  msend %actor% %self.name% says, 'You have no choice, you have to get a new meteorite.'
  quest variable meteorswarm %actor% new yes
  wait 2s
  msend %actor% %self.name% says, 'You already know where the rock demon is.'
  wait 2s
  msend %actor% %self.name% says, 'Good luck.'
endif
~
#48299
meteorswarm_status_checker~
0 d 0
spell progress~
set stage %actor.quest_stage[meteorswarm]%
wait 2
if %actor.quest_variable[meteorswarm:new]% /= yes
  msend %actor% %self.name% says, 'Go find a new meteorite.'
  halt
elseif %actor.quest_variable[meteorswarm:new]% /= no
  msend %actor% %self.name% says, 'Show me the meteorite again.''
  halt
endif
switch %stage%
  case 1
    msend %actor% %self.name% tells you, 'You need to find Jemnon and &6&bask about the rock demon&0.  He's in some &6&btavern&0, no doubt, waiting for some new blunder to embark on.'
    break
  case 2
    msend %actor% %self.name% tells you, 'Find a suitable focus by defeating the rock demon in Templace.'
    break
  case 3
    if %actor.quest_variable[meteorswarm:earth]% == 0
      msend %actor% %self.name% tells you, 'Show me the meteorite.'
    else
      if %actor.quest_variable[meteorswarm:fire]% == 0
        msend %actor% %self.name% tells you, 'Find and kill the high fire priest in the Lava Tunnels.  Then enter the lava bubble below his secret chambers.'
      else
        msend %actor% %self.name% tells you, 'Find the lava bubble in the high fire priest's secret chambers.'
      endif
    endif
    wait 1s
    msend %actor% %self.name% says, 'If you somehow lost the meteorite, say &1&b"I lost the meteorite"&0.'
    break
  case 4 
    msend %actor% %self.name% tells you, 'Convince the ancient dragon Dargentan to teach you the ways of air magic.'
    wait 1s
    msend %actor% %self.name% says, 'If you somehow lost the meteorite, say &1&b"I lost the meteorite"&0.'
    break
  case 5
    if %actor.quest_variable[meteorswarm:air]% == 0
      msend %actor% %self.name% tells you, 'Show me the meteorite now that you have mastered earth, fire, and air.'
      wait 1s
      msend %actor% %self.name% says, 'If you somehow lost the meteorite, say &1&b"I lost the meteorite"&0.'
    else
      msend %actor% %self.name% tells you, 'Take your finished focus and unleash its potential!'
    endif
done
~
$~
