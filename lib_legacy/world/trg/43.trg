#4300
karla_count~
0 b 15
~
emote takes a deep breath
wait 3
say Here we go!  5, 6, 7, 8!
wait 2s
emote executes a flawless combination of steps.
wait 1s
nod
Say Okay, I think I got it.
~
#4301
catherine_key~
0 j 100
~
mmobflag %self% sentinel on
if %actor.quest_stage[theatre]% >= 1
  if %object.vnum% == 4351
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
        if %person.quest_stage[theatre]% >= 1 && !%person.quest_variable[theatre:lashes]%
          quest variable theatre %person% lashes 1
          if %person.quest_stage[theatre]% == 1
            msend %person% &7&bYou have helped return Catherine's eyelashes.&0
          endif
          set accept_lashes 1
        endif
      elseif %person%
        eval i %i% + 1
      endif
      eval a %a% + 1
    done
    if !%accept_lashes%
      set refuse lashes
    else
      wait 2
      say Thank you so much!
      wait 1s
      sit
      wear eyelashes
      emote flutters her eyelids a few times.
      wait 2s
      stand
      wait 2
      say Finally!  Now if I could just have my dressing room key back too please...
    endif
  elseif %object.vnum% == 4303
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
        if %person.quest_stage[theatre]% >= 1 && %person.quest_variable[theatre:lashes]% == 1
          if %person.quest_stage[theatre]% == 1
            quest advance theatre %person%
            msend %person% &7&bYou have advanced the quest!&0
          endif
          quest variable theatre %person% lashes 0
          set accept_key 1
        endif
      elseif %person%
        eval i %i% + 1  
      endif
      eval a %a% + 1
    done
    if !%accept_key%
      wait 2
      mecho %self.name% makes a weird, embarrassed face.
      wait 1s
      say Can you be a dear and bring me my eyelashes first?
      wait 1s
      say Thaaaaaaanks!
      give key %actor.name%
    else
      wait 2
      emote looks incredibly relieved.
      say I can't believe those monkeys made off with my key but not my costumes.
      wait 2s
      shrug
      say It's the little things I guess.
      wait 1s
      say Oh!  I think Lewis's key got locked in my dressing room...
      mat 4351 m_run_room_trig 4365
      wait 2s
      say He might want that back.
      wait 3s
      say Now, if only I could find my shoes...
      emote trails off and wanders away.
    endif
  else
    set refuse item
  endif
else
  set refuse person
endif
if %refuse%
  return 0
  if %refuse% == lashes
    mecho %self.name% blinks in confusion.
    wait 2
    say N...o?  You already gave me my eyelashes.
    msend %actor% %self.name% slowly pushes the eyelashes back to you.
    wait 2
    mechoaround %actor% %self.name% slowly pushes the eyelashes back to %actor.name%.
  elseif %refuse% == item
    mecho A confused look crosses Catherine's face as she refuses %object.shortdesc%.
    wait 2
    say This isn't what I was looking for!
    wait 1s
    say Please help me or I'll miss my cue!
  elseif %refuse% == person
    mecho %self.name% looks at %object.shortdesc% with confusion.
    wait 2
    say Ummm, what is that?
    wait 1s
    mechoaround %actor% %self.name% looks at %actor.name% with confusion.
    msend %actor% %self.name% looks at you with confusion.
    wait 2s
    say I'm sorry, who are you?
  endif
endif
mmobflag %self% sentinel off
~
#4302
charlemagne_key~
0 j 100
~
mmobflag %self% sentinel on
set i %actor.group_size%
set stage 4
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %actor.group_member[%a%]%
  if %person.room% == %self.room%
    if %object.vnum% == 4301
      if %person.quest_stage[theatre]% >= %stage%
        if !%person.quest_variable[theatre:charles]%
          quest variable theatre %person% charles 1
          set accept 1
          if %person.quest_stage[theatre]% == %stage%
            msend %person% &7&bYou have helped return Charlemagne's key.&0
          endif
        else
          set refuse key
        endif
      else
        set refuse person
      endif
    elseif %object.vnum% == 4320
      if %person.quest_stage[theatre]% <= %stage%
        set accept 3
        if %person.quest_stage[theatre]% == %stage%
          if !%person.quest_variable[theatre:sash]%
            quest variable theatre %person% sash 1
            msend %person% &7&bYou have helped return Charlemagne's sash.&0
          endif
        endif
      else
        if %accept% != 3
          set accept 2
        endif
      endif
    else
      set refuse item
    endif
    if %person.quest_stage[theatre]% == 4 && %person.quest_variable[theatre:sash]% && %person.quest_variable[theatre:charles]%
      quest advance theatre %person%
      quest variable theatre %person% charles 0
      quest variable theatre %person% sash 0
      msend %person% &7&bYou have advanced the quest!&0
    endif
  elseif %person%
    eval i %i% + 1
  endif
  eval a %a% + 1
done
if %actor.quest_stage[theatre]% == 4
  if !%actor.quest_variable[theatre:sash]%
    set outcome 2
  elseif !%actor.quest_variable[theatre:charles]%
    set outcome 3
  else
    set outcome 1
  endif
else
  set outcome 1
endif
if %accept%
  wait 2
  if %accept% == 1
    say Ah marvelous!
    wait 2s
  elseif %accept% > 1
    if %accept% == 2
      wait 2
      say Another?
      wait 2
      ponder
      wait 2s
      say Well I guess one never can be too prepared!
    elseif %accept% == 3
      say Where did you find this?  It must have been stolen in the scuffle this afternoon.  Thank you for returning it to me.
    endif
    remove sash
    wear charlemagne's-fire-sash
    mjunk sash
    wait 5
    bow
    wait 2s
    say Oh, since you seem to be so good at returning things, I think I might have something for you to do...
    wait 2
    emote goes searching around through the room, tossing aside bedding and clothing.
    wait 5s
    say Ah ha!  Here it is!
    mload obj 4305
    emote pulls the &1F&b&3i&b&1r&0&1e G&b&3o&b&1dd&0&b&3e&0&1ss&b&1's&0 skirt from a pile of clothing.
    wait 2s
    say The actress, umm, left this here by accident last night...
    wait 2
    cough
    wait 2s
    say I've been meaning to return it to her, but I've been locked in here all day.  I'm sure she would be most happy to have it back.
    wait 3s
    give skirt %actor.name%
    wait 2s
    say Thank you kindly.
    wait 2s
  endif
  if %outcome% == 1
    say Now be off!  I have to prepare for my performance!
    wait 2s
    msend %actor% %self.name% dismisses you.
  elseif %outcome% == 2
    say Now if only I could find my sash, I would be ready to go on tonight.
  elseif %outcome% == 3
    say If you have my dressing room key, please hand it over.
  endif
elseif %refuse%
  if %refuse% == item
    wait 2
    roll %actor%
    wait 1s
    say This isn't what I'm looking for.  Stop wasting time and help me look.
  else
    return 0
    if %refuse% == person
      gasp
      say Are you a Ceiling Monkey in disguise?!
      wait 1s
      point %actor.name%
    elseif %refuse% == key
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      say You already gave me my key back, thank you.
    endif
  endif
endif
mmobflag %self% sentinel off
~
#4303
lewis_key~
0 j 100
~
mmobflag %self% sentinel on
if %object.vnum% == 4300
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
      if %person.quest_stage[theatre]% == 2
        quest advance theatre %person%
        msend %person% &7&bYou have advanced the quest!&0
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
  if %actor.quest_stage[theatre]% >= 2
    wait 2
    say Hurray!  Thank you!
    clap
    wait 2s
    say Oh!  Mummy's going to want her key back too!
    wait 2
    mecho %self.name% fishes a key out of his armor.
    wait 1s
    say Please, give this to her.
    mload obj 4302
    give key %actor.name%
    wait 2s
    say Ta!
    wait 3
    emote makes air kisses.
  else
    set refuse quest
  endif
else
  set refuse item
endif
if %refuse%
  return 0
  if %refuse% == quest
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    say What is this??  How did you get in here??
  elseif %refuse% == item
    mecho %self.name% looks blankly at %object.shortdesc%.
    wait 2
    say Well yes that's nice but it doesn't exactly help the situation.
  endif
endif
mmobflag %self% sentinel off
~
#4304
fastrada_key~
0 j 100
~
mmobflag %self% sentinel on
if %object.vnum% == 4302
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
      if %person.quest_stage[theatre]% == 3
        quest advance theatre %person%
        msend %person% &7&bYou have advanced the quest!&0
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
  if %actor.quest_stage[theatre]% >= 3
    wait 3
    say Thank you sooooo much.
    mechoaround %actor% %self.name% drapes herself all over %actor.name%.
    msend %actor% %self.name% drapes herself all over you.
    wait 1s
    emote bends over slowly...
    wait 2s
    emote picks up a key.
    mload obj 4301
    wait 2
    give key %actor.name%
    wait 2s
    say My darling husband, the King, will probably want that.
    wait 4s
    say Go let him out of his cage, he'll enjoy some fresh air.
    wait 5
    wave
  else
    return 0
    mecho %self.name% refused %object.shortdesc%.
    wait 2
    say How did you get this?
    wait 1s
    peer %actor%
    say Are you stalking me?
  endif
else
  wait 2
  say Why thank you!  I love a good gift!
endif
mmobflag %self% sentinel off
~
#4305
gnome_king_random~
0 b 15
~
grumble
wait 1s
say I sure hope we rounded up the last of those stupid monkeys...
~
#4306
gnome_king_monkeys~
0 d 100
monkeys?~
say Yes, the Ceiling Monkeys!
wait 4
mutter
wait 3s
mecho %self.name% says, 'They managed to sneak off the catwalk this afternoon and ran amuck through the theater.'
wait 3s
say 
mecho %self.name% says, 'I believe my gnomes and I managed to get them all back up into their nesting grounds.  Unfortunately, they made off with a couple of the actors' personal belongings.'
wait 4s
eye %actor.name%
wait 5
mecho %self.name% says, 'However, you might be able to help.  Are you up to it?'
~
#4307
gnome_king_key~
0 d 100
yes~
if %actor.vnum% == -1
  mmobflag %self% sentinel on
  mat 4351 m_run_room_trig 4363
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
      if %person.quest_stage[theatre]% == 0
        quest start theatre %person%
        msend %person% &7&bYou have begun the theatre quest!&0
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
  wait 2
  grin
  wait 3s
  say Then you'll need this.
  mload obj 4304
  wait 4
  emote produces a key from his pocket.
  give key %actor.name%
  wait 3s
  say You should be able to reach their nest from our workshop in the back of the theater.  There's a second entrance above the balcony, but it's always locked from the other side.
  wait 4
  say Good luck!
  bow %actor.name%
  mmobflag %self% sentinel off
endif
~
#4308
karla_me~
0 b 15
~
Say Look at me look at me look at me now!
Wait 4
emote hits a pose!
Wait 2s
grin
~
#4309
karla_chichi~
0 d 0
Don't you think so?~
set room %self.room%
if %room.people[4301] != 0
emote gets up in a player's face.
say Oh no you di'in' Chichi!
Wait 5
Say You best step off!
emote struts around the room, waiving her finger.
end
~
#4310
chapman_rosa~
0 d 0
You best step off!~
if %actor.vnum% == 4305
emote puts his hand up in a player's face.
wait 3s
Say Oh yes I di' Rosa!
wait 3s
Say You don' get ta tell me what ta do!
emote puts one hand on his hip and flails the other hand around.
end
~
#4311
chapman_cute~
0 b 20
~
emote beams.
Wait 3
Say I'm just so CUTE!!
wait 5
Say Don't you think so?
~
#4312
chad_notgay~
0 d 100
Oh don't worry I can make it worth your time.~
If %actor.vnum% == 4302
          wait 5
          emote scoots away from his fellow player.
          wait 3s
          Say I just don't know...  I mean, I COULD be...
          wait 5
          emote is so confused by it all!
endif
~
#4313
chad_pinch~
0 b 10
~
emote pinches a player on the behind!
wait 4
Whistle
wait 3s
Say Wasn't me.  I like girls.
~
#4314
nick_get_him~
0 b 10
~
emote slinks up to his fellow player and tosses his arms around him.
wait 2s
emote plasters a sultry smile across his face.
Wait 3s
grope chad
wait 3s
Say Oh don't worry, I can make it worth your time.
wait 4
Wink
~
#4315
nick_why~
0 b 10
~
mecho %self.name% says, 'Oh if only you could see it the way I do, you would understand
mecho &0that of COURSE I'm right.'
wait 4s
emote fumes with rage!
wait 5
Say Why do I always let guys do this to me?!
~
#4316
amy_mean~
0 b 10
~
Roll
Wait 3s
mecho %self.name% says, 'God, my sister is so bossy.  I wish she would just shut her big
mecho &0fat mouth.  I can't stand her!'
Say I can't stand her.
wait 4
emote prattles on to no one in particular.
~
#4317
amy_stupid~
0 b 10
~
emote scoffs.
Wait 3s
mecho %self.name% says, 'My sister is so freaking stupid.  She has no idea what's going
mecho &0on.  Why can't she be a better person, like me?'
wait 4
emote mumbles something about being from the deep end of the gene pool.
~
#4318
lena_place~
0 b 10
~
emote screams, 'No, that's NOT where you're supposed to be!'
wait 5
emote points vehemently to a spot on the floor.
Wait 3s
Say THIS is where you're supposed to be!  
Wait 5
scream
wait 4s
mecho %self.name% says, 'God!  Why does no one know what they're supposed to be doing
mecho &0but me?!'
~
#4319
lena_cry~
0 b 10
~
emote screams, 'It's just not fair!'
wait 5
emote sobs hysterically.
Wait 3s
mecho %self.name% says, 'No one appreciates what I do here!  I never get recognized for
mecho &0ANYTHING!!'
~
#4320
lena_sister~
0 b 10
~
Scream
Wait 2
mecho %self.name% says, 'My sister is so MEAN to me!  I wish someone would just KILL
mecho &0her!'
~
#4321
lori_cry~
0 b 10
~
emote breaks down crying.
Wait 5s
emote forces herself to stand up straight.
Say I will NOT let this get to me.  I am better than this!
Wait 3s
Sniff
Wait 3s
mecho %self.name% says, 'If I can dance well enough, I'll win him back.  This show must
mecho &0be PERFECT!'
wait 2s
emote nods with conviction.
~
#4322
lori_dance~
0 b 10
~
emote closes her eyes and lifts her arms above her head.
Wait 3s
emote starts dancing while chanting
say Step kick kick leap kick touch...
wait 4s
sniff
wait 3s
say Can't start crying again...  I have to focus.
~
#4323
kristi_nail~
0 b 10
~
emote dramatically throws one hand to her brow.
mecho %self.name% says, 'Oh!  Oh!  The world is so miserable!  My hand is nailed to my
mecho &0forehead!'
Wait 5
emote sighs with the weight of a thousand elephants.
~
#4324
kristi_suck~
0 b 10
~
Sigh
Wait 3s
mecho %self.name% says, 'Sometimes the world just sucks you know?  No matter what you
mecho &0do, it's just a terrible place to be.'
Wait 4s
Sigh
wait 3s
Say If only everyone would stop being oh so cruel to me!
~
#4325
lewis_shine~
0 b 10
~
emote eyes himself in the mirror.
Wait 3s
Say Hello beautiful.
emote winks at his reflection.
wait 4s
Say Look how I shine!
emote bats his eyelashes for you to better bask in his glory.
Wait 5s
emote is quickly distracted by something else shiny in the room.
wait 3s
Say Visigoths!  Ha!
emote swings an invisible sword around at unseen foes.
~
#4326
fastrada_me~
0 b 10
~
emote adjusts her crown.
Wait 3s
Sigh
Wait 2s
Say It's good to be me.
wait 4
cackle
~
#4327
fastrada_book~
0 b 10
~
emote thumbs through a book on her shelf.
wait 2s
emote looks thoroughly bored.
wait 2s
sigh
wait 5s
mecho %self.name% says, 'If only I had one of those trashy harlequin novels to help pass
mecho &0the time.'
Wait 3s
Grin
Wait 3
Say Or a trashy harlequin man.
Wait 4
Cackle
Wait 2
Sigh
~
#4328
charlemagne_bored~
0 b 10
~
shake
Say So little and so few to do...
wait 3s
mecho %self.name% says, 'What is such a giant on the battlefield and in the bedroom
mecho &0to do?'
~
#4329
catherine_eyelashes~
0 b 20
~
emote frantically searches for something.
wait 4s
say Oh damn it, where is it?!
Wait 4
Fume
Wait 3
mecho %self.name% says, 'I bet those stupid monkeys ran off with my key...  That's why
mecho &0I can't find it.'
~
#4330
catherine_monkeys~
0 d 100
monkey monkey? monkeys monkeys? key key?~
if %actor.vnum% == -1
  wait 2
  emote points up to the ceiling.
  wait 1s
  say Ya see, there's these monkeys that live in the rafters...
  glare
  wait 2s
  say Those horrible monkeys!
  wait 4s
  msend %actor% %self.name% turns to look at you.
  mechoaround %actor% %self.name% turns to look at %actor.name%
  wait 2s
  say We call them "Ceiling Monkeys," for lack of a better term.
  wait 4s
  say Anyway the monkeys got loose this afternoon and attacked us as we came in!
  wait 1s
  say It was madness!  MADNESS!!
  wait 3s
  shudder
  wait 1s
  say In any case, they ran off with a few of our things, and I think my dressing room key was one of them!
  wait 4s
  say So now my eyelashes are still locked in my dressing room, Theo needs his duck, and the only person who can get to the stupid monkeys to get my key back is the House Gnome King.
  wait 3s
  say Find my key and my eyelashes and bring them back to me.
  wait 2
  say Help me %actor.name%, you're my only hope!
endif
~
#4331
catherine_theo~
0 d 100
I want my duck!~
if %actor.vnum% == 4310
  say Theo, I don't have time for this!
  wait 2s
  say We'll get your duck as soon as I can find my key!
endif
~
#4332
theo_whine~
0 b 15
~
if %self.worn[18]% == 4311
  emote pets a duck lovingly.
  wait 4
  emote quacks like a duck!  QUACK!
else
  emote plays around with a brilliantly colored piece of fabric.
  wait 3s
  say Whee!
  wait 4s
  say I wish Otto were here to play King with me too...
endif
~
#4333
theo_play~
0 b 20
~
if %self.worn[18]% == 4311
  emote pets a duck lovingly.
  wait 4
  pur
else
  whine
  wait 4s
  say I want my duck!!
endif
~
#4334
theo_duck~
0 d 100
duck? Otto?~
say Mommy locked my duck Otto in her dressing room.
wait 4
sniff
wait 3s
say Otto must be getting so lonely!
~
#4335
theo_sash~
0 j 100
~
mmobflag %self% sentinel on
if %object.vnum% == 4311
  wait 2
  mecho Theo's face lights up with childish joy.
  wait 3
  cheer
  hug %actor.name%
  wait 4
  say Thank you!!
  wait 4
  hold duck
  wait 3s
  emote pets a stuffed duck lovingly.
  wait 3
  pur
  wait 4s
  say You can have this for being nice to me.
  mload obj 4320
  wait 3
  give sash %actor.name%
else
  wait 2
  say That's not a duck, dummy!
  pout
endif
mmobflag %self% sentinel off
~
#4336
fire_goddess_where~
0 b 20
~
: puts her hands on her hips and looks around, exasperated.
wait 4
Say Where is he?  We can't start the show without him.
~
#4337
fire_goddess_who~
0 d 100
who? he? him?~
wait 2
mecho %self.name% says, 'Pippin of course!  He's our star of this production.
mecho &0 He's the last thing we need for our grand finale.'
wait 2s
mecho %self.name% says, 'Of course we can't find him, what with the monkey
mecho &0invasion earlier.  All of our props and costumes got all messed up, and now our
mecho &0star has gone missing too!'
wait 3s
say Talk to the House Gnome King and see about getting that straightened out.
~
#4338
fire_goddess_skirt~
0 j 100
~
mmobflag %self% sentinel on
if %object.vnum% == 4305
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
      if %person.quest_stage[theatre]% >= 5
        set accept yes
        if %person.quest_stage[theatre]% == 5
          quest advance theatre %person%
          msend %person% &7&bYou have advanced the quest!&0
        endif
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
  wait 2
  blush
  emote says sheepishly, 'I was wondering where I left that.'
  rest
  wait 5
  remove sash
  drop sash
  wear skirt
  wait 3s
  emote composes herself.
  wait 2s
  if %accept% == yes
    emote clears her throat.
    wait 2s
    say Well, I suppose this means you dealt with the monkeys and helped our cast.  Thanks to you, our show has a chance of going on!
    wait 4s
    say However, we're still missing one important thing...
    wait 2s
    msend %actor% The Fire Goddess eyes you suspiciously.
    mechoaround %actor% The Fire Goddess eyes %actor.name% suspiciously.
    wait 2s
    msend %actor% The Fire Goddess gives you an approving nod.
    mechoaround %actor% %actor% The Fire Goddess gives %actor.name% an approving nod.
    say Yes, I think you can help.
    wait 4s
    say 
    say We need to find our Pippin in order to perform the grand Finale.  He's somewhere out in the world, trying to find his corner of the sky.
    wait 4s
    say I want you to bring him to us.
    wait 2s
    say 
    say Take this.  Use its magic to lure him back to the theater.
    mload obj 4318
    give torch %actor.name%
    wait 4s
    say Hold it in your hand when you find him.  He won't be able to resist the beauty of one perfect flame.
    wait 4s
    say Now, when you get him back here, order him to enter the Fire Box.  We've prepared and hidden it upstage center just for him.
    wait 3s
    mecho %self.name% says, &1&b'Be careful not to get inside it yourself!'&0
    wait 3s
    say It's only for an extraordinary person like Pippin.
    wait 4s
    say We'll be waiting for you.
  endif
else
  return 0
  msend %actor% The Fire Goddess looks at you funny.
  mechoaround %actor% The Fire Goddess looks at %actor.name% with an odd expression.
  wait 2
  say What is this?
endif
mmobflag %self% sentinel off
~
#4339
chad_lead~
0 b 100
~
if %get.mob_count[4302]% < 1
mload mob 4302
wait 1
mforce nick mload obj 4312
mforce nick mload obj 4313
mforce nick mload obj 4314
mforce nick mload obj 4315
mforce nick mload obj 4316
mforce nick mload obj 4317
mforce nick wear all
mforce nick follow chad
follow nick
end
~
#4340
catherine_lead~
0 o 100
~
if %get.mob_count[4310]% < 1
  mload mob 4310
  mforce theo mload obj 4312
  mforce theo mload obj 50205
  mforce theo mload obj 38
  mforce theo mload obj 4317
  mforce theo wear all
  mforce theo follow lauren
  follow theo
end
~
#4341
pippin_torch~
0 g 100
~
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
    if (%person.wearing[4318]% || %person.inventory[4318]%) && %person.quest_stage[theatre]% >= 6
      set check 1
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
if %check%
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
      if %person.quest_stage[theatre]% == 6
        quest advance theatre %person%
        msend %person% &7&bYou have advanced the quest!&0
      endif
      if !%leader%
        set leader %person%
      endif
    elseif %person% && %person.vnum% == -1
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
endif
if %self.consented%
  halt
elseif %leader%
  wait 2
  mmobflag %self% sentinel on
  wait 1
  mecho The &1F&b&3i&b&1r&0&1e G&b&3o&b&1dd&0&b&3e&0&1ss&b&1's&0 Torch erupts to life and spews a shower of sparks and flame!
  wait 1s
  mecho Pippin watches the flames, completely entranced.
  follow %leader.name%
  consent %leader.name%
endif
~
#4342
torch hold~
1 j 100
~
set room %actor.room%
if %room.people[4312]%
  set person %actor%
  set i %person.group_size%
  if %i%
    set a 1
  else
    set a 0
  endif
  while %i% >= %a%
    set person %actor.group_member[%a%]%
    if %person.room% == %room%
      if %person.quest_stage[theatre]% == 6
        quest advance theatre %person%
        osend %person% &7&bYou have advanced the quest!&0
        set leader %person%
      elseif %person.quest_stage[theatre]% >= 6
        if !%leader%
          set leader %person%
        endif
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
  if %leader%
    oforce pippin mmobflag pippin sentinel on
    wait 1
    oecho The &1F&b&3i&b&1r&0&1e G&b&3o&b&1dd&0&b&3e&0&1ss&b&1's&0 Torch erupts to life and spews a shower of sparks and flame!
    wait 1s
    oecho Pippin watches the flames, completely entranced.
    if %actor.quest_stage[theatre]% > 6
      oforce pippin follow %actor.name%
      oforce pippin consent %actor.name%
    else
      oforce pippin follow %leader.name%
      oforce pippin consent %leader.name%
    endif
  endif
endif
~
#4343
pippin_enter~
0 c 100
order~
if ((%actor.wearing[4318]% || %actor.inventory[4318]%) && (%self.room% == 4333) && (%arg% == pippin enter box))
  enter box
else
  return 0
endif
~
#4344
pippin_fulfillment~
0 b 20
~
Say Ultimate fulfillment...
Wait 1s
Mecho Pippin's eyes gleam with determination.
Wait 1s
Say And I'm going to find it!
~
#4345
pippin_corner~
0 b 20
~
: looks out into the horizon.
Wait 1s
Say Gotta find my corner of the sky...
~
#4346
pippin_death~
0 f 100
~
if %self.room% == 4336
mat 4333 m_run_room_trig 4398
end
~
#4347
box_crumble~
1 b 100
~
wait 60s
oecho The wreckage of the fire box burns itself out, leaving behind only ash.
opurge %self%
~
#4348
LP_join_us~
0 g 100
~
wait 2
if %random.3% == 1
  emote croons gently, 'Join us, leave your fields to flower...'
endif
switch %actor.quest_stage[bard_subclass]%
  case 1
  case 2
  case 3
  case 4
    msend %actor% %self.name% says, 'Ready to continue your audition?'
    break
  default
    if %actor.class% /= Rogue
      switch %actor.race%
*       case ADD RESTRICTED RACES HERE
*          break
        default
          wait 1s
          if %actor.level% >= 10 && %actor.level% <= 25
            msend %actor% %self.name% says, 'The guardians of splendor are calling out your name, %actor.name%.  Do you want to know what they're saying?'
          endit
      done
    endif
done
~
#4349
LP_magic~
0 b 15
~
emote brushes up to you, singing,
mecho 'We've got magic to do, just for you.
mecho &0We've got miracle plays to play.'
wait 3s
mecho Smoothly gliding up and around you, the Leading Player keeps on enticing you to join him.
wait 3s
emote sings, 
mecho 'We've got parts to perform, hearts to warm.
mecho Kings and things to take by storm as we go along our way, hey!'
~
#4350
LP_finale~
0 d 100
join?~
say Watch our performance!  Come and waste an hour or two.
emote closes his eyes and smiles a seductively evil smile.
wait 3s
say All the preparations are set for the only perfect act in our repertoire.
~
#4351
lp_perfect~
0 d 100
perfect act?  repertoire?~
wait 2
emote presents himself with grand poise.
mecho %self.name% says, 'Yes, our grand Finale, never before seen on a public
mecho &0stage!'
wait 3s
eye %actor.name%
wait 3s
mecho %self.name% says, 'This is not something for just anyone.  The Finale is
mecho &0only for someone spectacular, with great dreams and aspirations.  One who is
mecho &0ready for one last, perfect act.  For one brief moment, they will shine like
mecho &0the sun itself.'
wait 4s
emote gives you a very warning look.
wait 3s
say Once you enter that place, there is no going back.
~
#4352
lp_box~
0 d 100
place?  Where?~
wait 2
msend %actor% With a deft motion, the Leading Player places himself behind you, whispering in your ear.
mechoaround %actor% With a deft motion, the Leading Player places himself behind %actor.name%.
wait 2s
mecho %self.name% says, 'Our great Fire Box!  It is the piece de resistance.
mecho And it is ready for the person we have picked to perform our Finale.'
wait 4s
msend %actor% The Leading Player softly caresses your face, his lips inches from your ear.
mechoaround %actor% The Leading Player softly caresses %actor.name%'s face.
wait 2s
say If YOU want to be that person, you may try.
wait 4s
emote waves his hand in front of an invisible point in space.
say Think about the sun; join in one perfect flame.
wait 4s
msend %actor% With a powerful thrust, the Leading Player presses himself up very close to you.
mechoaround %actor% With a powerful thrust, the Leading Player presses himself up very close to %actor.name%.
wait 3s
mecho %self.name% says, 'Become one with the flame - and in that flame become
mecho &0a glorious synthesis of life and death, and life again.'
wait 3s
emote releases his embrace, breaking free into a powerful pose.
wait 4s
say Think about the sun.
wait 2s
mecho %self.name% says, 'Unfortunately we can't get this show on the road
mecho &0until the theatre is reorganized after that monkey attack.'
wait 3s
say Talk to the House Gnome King.
~
#4353
Leading Player refuse~
0 j 0
4357~
switch %object.vnum%
  default
    if %actor.quest_stage[bard_subclass]% == 4
      return 0
      msend %actor% %self.name% says, 'This isn't the script I sent you for.'
      mecho %self.name% refuses %object.shortdesc%.
      Wait 1s
      msend %actor% %self.name% says, 'I sure hope you didn't memorize anything from that!'
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 1s
      msend %actor% %self.name% says, 'Ummmmm, what exactly is this for?'
    endif
done
~
#4354
LP_bard_subclass_speech1~
0 d 100
yes no~
if (%actor.class% /= Rogue && (%actor.level% >= 10 && %actor.level% <= 25))
  switch %actor.race%
*   case ADD RESTRICTED RACES HERE
*      msend %actor% &1Your race cannot subclass to bard.&0
*      halt
*      break
    default
      wait 2
      if %speech% == yes
        msend %actor% %self.name% says, 'Right this way then!'
        switch %actor.quest_stage[bard_subclass]%
          case 1
            set next Let's hear you &5&bsing&0!
            break
          case 2
            set next Let's see you &5&bdance&0!
            break
          case 3
          case 4
            set next Do you have the &3&bscript&0?
            break
          case 5
            set next Let's hear some &6&bdialogue&0!
            break
          default
            set next So, let's discuss your &6&baudition&0.
            wait 2
            msend %actor% %self.name% says, 'Seems like you got real star potential kid.'
            wait 2s
            msend %actor% %self.name% says, 'Follow me to the rehearsal studio.  We want you to audition for us.'
        done
        wait 2s
        msend %actor% %self.name% escorts you backstage to a smaller dance studio.
        mechoaround %actor% %self.name% escorts %actor.name% backstage.
        wait 2s
        mteleport %actor% 4369
        mforce %actor% look
        wait 2s
        msend %actor% %self.name% settles in.
        wait 1s
        mat 4369 msend %actor% %self.name% says, '%next%'
      else
        msend %actor% %self.name% says, 'Too bad.  You have real start potential kid.  You could be big.'
        wait 2s
        msend %actor% %self.name% says, 'Huge!'
        wait 2s
        msend %actor% %self.name% says, 'Lemme know if you change your mind.  It's not too late for you.'
      endif
  done
endif
~
#4355
LP_bard_subclass_speech2~
0 d 100
audition audition? process process?~
if (%actor.class% /= Rogue && (%actor.level% >= 10 && %actor.level% <= 25))
  switch %actor.race%
*   case ADD RESTRICTED RACES HERE
*      msend %actor% &1Your race cannot subclass to bard.&0
*      halt
*      break
    default
      if !%actor.quest_stage[bard_subclass]%
        wait 2
        msend %actor% %self.name% says, 'We're looking for someone who can sing and dance and act - a real triple threat.  It'll earn you a place in the spotlight and the Bard Guild.  You'll be a real professional!'
        wait 2s
        msend %actor% %self.name% says, 'Whadda ya say kid?  You wanna give it a shot?'
      endif
  done
endif
~
#4356
LP2_bard_subclass_speech1~
0 d 100
yes no~
if (%actor.class% /= Rogue && (%actor.level% >= 10 && %actor.level% <= 25))
  switch %actor.race%
*   case ADD RESTRICTED RACES HERE
*      msend %actor% &1Your race cannot subclass to bard.&0
*      halt
*      break
    default
      if !%actor.quest_stage[bard_subclass]%
        wait 2
        if %speech% /= yes
          quest start bard_subclass %actor% bar
          msend %actor% %self.name% says, 'Great!  From now on, you can check your &6&b[subclass progress]&0 with me.'
          wait 2s
          msend %actor% %self.name% says, 'First, let's hear you &5&bsing&0.'
        else
          nod
          msend %actor% %self.name% says, 'Come back if you want to go through with it.  My door is always open.'
        endif
      endif
  done
endif
~
#4357
LP2_bard_subclass_command_sing~
0 c 100
sing~
switch %cmd%
  case s
  case si
    return 0
    halt
done
mforce %actor% sing
if %actor.quest_stage[bard_subclass]% == 1
  quest advance bard_subclass %actor%
  wait 3s
  emote blinks very slowly.
  wait 2s
  msend %actor% %self.name% says, 'Okay, not the best I've ever heard but not the worst either.'
  wait 2s
  msend %actor% %self.name% says, 'Maybe your &5&bdance&0 skills are better.'
  wait 1s
  msend %actor% %self.name% says, 'Show me what ya got!'
endif
~
#4358
LP2_bard_subclass_command_dance~
0 c 100
dance~
switch %cmd%
  case d
    return 0
    halt
done
if %actor.quest_stage[bard_subclass]% == 2
  if !%actor.wearing[4315]%
    if %actor.worn[feet]%
      set shoes %actor.worn[feet]%
    endif
    msend %actor% You start to dance when %self.name% abruptly stops you.
    mechoaround %actor% %actor.name% starts to dance when %self.name% abruptly stops %himher%.
    msend %actor% %self.name% says, 'Woah woah woah %actor.name%!
    wait 2s
    if %shoes%
      msend %actor% %self.name% says, 'How in the world do you expect to dance in %get.obj_shortdesc[%shoes.vnum%]%??'
    else
      msend %actor% %self.name% says, 'How in the world do you expect to dance barefoot??'
    endif
    wait 2s
    msend %actor% %self.name% says, 'Put on some proper &3&bdance shoes&0.  If you didn't bring any maybe you can borrow a pair from someone in the company.'
    wait 2s
    msend %actor% %self.name% says, 'Forcibly if you have to.'
  else
    mforce %actor% dance
    quest advance bard_subclass %actor%
    wait 3s
    msend %actor% %self.name% says, 'Well that routine was certainly a... choice.'
    wait 2s
    msend %actor% %self.name% says, 'Last thing, I need to see some of your acting work.  But I suppose you don't have a script yet...'
    wait 1s
    think
    wait 3s
    snap
    wait 2
    msend %actor% %self.name% says, 'I know someone who had one you might be able to &6&bborrow&0.'
  endif
else
  return 0
endif
~
#4359
LP2_bard_subclass_speech2~
0 d 100
borrow borrow? someone someone? who who?~
wait 2
if %actor.quest_stage[bard_subclass]% == 2
  roll
  msend %actor% %self.name% says, 'I mean go beat up an actor and take their shoes!'
elseif %actor.quest_stage[bard_subclass]% == 3
  nod
  msend %actor% %self.name% says, 'She used to play Berthe, the granddam of our little show.  Retired some years back to a big house on a hill just west of Mielikki.  Heard she passed away, but all her stuff is still just sitting in the house.'
  wait 6s
  msend %actor% %self.name% says, 'I'd go get back the script, but...'
  wait 2s
  msend %actor% %self.name% says, 'I can't go back to Mielikki, I'm a bad bad man.'
  grin
  wait 2s
  msend %actor% %self.name% says, 'Really though, those guards are not joking around!'
  wait 3s
  msend %actor% %self.name% says, 'Why don't you go see if you can find her &3&bscript&0.  She would probably have kept it with her most prized possessions.  When you find it, look through it and see if any lines stick with you.'
  wait 6s
  msend %actor% %self.name% says, 'Come back, give me the script, and &6&bsay a line or two&0 for me.'
endif
~
#4360
script_bard_subclass_get~
1 g 100
~
if %actor.quest_stage[bard_subclass]% == 3
  quest advance bard_subclass %actor%
endif
~
#4361
LP2_bard_subclass_receive~
0 j 100
4357~
switch %actor.quest_stage[bard_subclass]%
  case 3
    return 0
    msend %actor% %self.name% says, 'Ummmmm, what exactly is this for?'
    msend %actor% %self.name% slaps %object.shortdesc% out of your hand!
    mechoaround %actor% %self.name% slaps %object.shortdesc% out of %actor.name%'s hand!
    mforce %actor% drop berthe-script
    wait 2
    msend %actor% %self.name% says, 'Pick it up and take a look again.  Make sure it's the the right thing.'
    break
  case 4
    quest advance bard_subclass %actor%
    wait 2
    mjunk berthe-script
    mecho %self.name% flips through the script.
    wait 2s
    msend %actor% %self.name% says, 'Yeah, this is it for sure.'
    wait 2s
    msend %actor% %self.name% says, 'Alright, let's &6&bhear a line or two&0.'
    wait 2s
    msend %actor% %self.name% says, 'Since you already gave me the script I sure hope you're off book!'
    wait 2
    msend %actor% %self.name% says, 'That means "memorized" in the biz.'
    wink %actor%
    break
  default
    wait 2
    mjunk berthe-script
    msend %actor% %self.name% says, 'Huh, I was looking for that.  Thanks!'
done
~
#4362
LP2_bard_subclass_speech3~
0 d 1
I~
if (%speech% /= I believe if I refuse to grow old I can stay young til I die || %speech% /= I believe if I refuse to grow old, I can stay young til I die)
  if %actor.quest_stage[bard_subclass]% == 5
    wait 1s
    mecho %self.name% wipes a tear from his eye.
    msend %actor% %self.name% says, 'Yeah, that's a great one.'
    wait 2s
    applaud %actor%
    msend %actor% %self.name% says, 'You did great kid, congratulations.  If you want a spot in the Guild, it's yours.  Here's your first spellbook and pen to commemorate the occassion.'
    wait 1s
    mload obj 1029
    give spellbook %actor%
    mload obj 1030
    give quill %actor%
    quest complete bard_subclass %actor%
    msend %actor% Type '&5&bsubclass&0' to proceed.
  endif
endif
~
#4363
Load lashes~
2 a 100
~
if !%self.objects[4351]%
  wload obj 4351
endif
if !%self.objects[4311]%
  wload obj 4311
endif
~
#4364
LP_receive~
0 j 100
~
switch %object.vnum%
  default
    if %actor.quest_stage[bard_subclass]%
      set response Nah nah nah.  We need to go down to the studio before you do anything else.  You ready to keep going?
    else
      set response Ummmmm, what exactly is this for?
    endif
done
if %response%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  msend %actor% %self.name% says, '%response%'
endif
~
#4365
load_lewis_key~
2 a 100
~
wload obj 4300
~
#4366
LP2_bard_subclass_status_checker~
0 d 100
status status? progress progress?~
wait 2
switch %actor.quest_stage[bard_subclass]%
  case 1
    msend %actor% %self.name% says, 'You're at the &5&bsinging&0 portion of your audition.'
    break
  case 2
    msend %actor% %self.name% says, 'You're at the &5&bdancing&0 portion of your audition.'
    break
  case 3
  case 4
    msend %actor% %self.name% says, 'You were looking for an old &3&bscript&0 in Morgan Hill.'
    if %self.vnum% == 4398
      wait 2
      msend %actor% %self.name% says, 'Give it to me when you find it.'
    endif
    break
  case 5
    msend %actor% %self.name% says, 'Time to gimme some &6&bdialogue&0 work.  I sure hope you're off book!'
    wait 2
    msend %actor% %self.name% says, 'That means "memorized" in the business.'
    wink %actor%
    break
  default
    if %actor.class% /= Rogue
      switch %actor.race%
*       case ADD RESTRICTED RACES HERE
*          set classquest no
          break
        default
          if %actor.level% > 10
            msend %actor% %self.name% says, 'You're a bit too green now, but come back soon!'
          elseif %actor.level% >= 10 && %actor.level% <= 25
            msend %actor% %self.name% says, 'You haven't even started your audition yet!'
            if %self.vnum% == 4399
              wait 2
              msend %actor% %self.name% says, 'Say &6&b'yes'&0 if you're ready to continue.'
            endif
          else
            msend %actor% %self.name% says, 'It's too late to start your career now buddy, sorry.'
          endif
      done
    else
      set classquest no
    endif
    if %classquest% == no
      msend %actor% %self.name% says, 'You aren't cut out for a life on stage.'
    endif
done
~
#4367
lewis_greet~
0 g 100
~
if %self.room% == 4345 && %actor.quest_stage[theatre]% == 2
  wait 2
  say You mean I can leave now?  Oh thank heaven!
  bounce
  wait 2s
  say I'm positively famished!
  wait 1s
  say May I please have my dressing room key back now?
endif
~
#4368
fastrada_greet~
0 g 100
~
if %self.room% == 4353 && %actor.quest_stage[theatre]% == 3
  wait 2
  msend %actor% Fastrada tosses a sultry smile your way.
  mechoaround %actor% Fastrada puts on a sultry smile.
  wait 3
  mecho %self.name% says, 'Thank you for letting me out of here.  Please, allow me to
  mecho &0to repay you in the future.'
  wink %actor.name%
  wait 1s
  mecho %self.name% says, 'But first things first - may I have my dressing room key back
  mecho &0please?'
endif
~
#4369
**UNUSED**~
0 d 100
restart~
if %actor.vnum% == -1
  mmobflag %self% sentinel on
  mat 4351 m_run_room_trig 4363
  if %actor.quest_stage[theatre]% == 0
    quest start theatre %actor%
    msend %actor% &7&bYou have begun the theatre quest!&0
  endif
  set person %actor%
  set i %person.group_size%
  if %i%
    set a 1
    while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person% == %actor%
        eval a %a% + 1
      else
        if %person.room% == %self.room%
          if %person.quest_stage[theatre]% == 0
            quest start theatre %person%
            msend %person% &7&bYou have begun the theatre quest!&0
          endif
        elseif %person%
          eval i %i% + 1
        endif
      endif
      eval a %a% + 1
    done
  endif
  wait 2
  grin
  wait 3s
  say Then you'll need this.
  mload obj 4304
  wait 4
  emote produces a key from his pocket.
  give key %actor.name%
  wait 3s
  say You should be able to reach their nest from our workshop in the back of the theater.  There's a second entrance above the balcony, but it's always locked from the other side.
  wait 4
  say Good luck!
  bow %actor.name%
  mmobflag %self% sentinel off
endif
~
#4370
charlemagne_greet~
0 g 100
~
if %self.room% == 4352 && %actor.quest_stage[theatre]% == 4
  wait 2
  emote breathes a sigh of relief.
  wait 1s
  say Finally!  I was about to break down the door to escape!
  wait 1s
  grumble
  wait 3s
  say If you have my dressing room key, please hand it over.
endif
~
#4397
test trigger~
0 d 100
run~
set person %actor%
mecho person is %person.name%
set i %person.group_size%
mecho group size is %person.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  mecho we are in the while loop
  set person %actor.group_member[%a%]%
  mecho actor.group_member[a] is %person.name%
  if %person.room% == %self.room%
    mecho Person is in the room
  elseif %person%
    eval i %i% + 1
    mecho Person is not in the room
  endif
  eval a %a% + 1
  mecho Evaluating a
done
mecho done
~
#4398
leading_player~
2 a 100
~
Wait 1s
wecho The players CHEER madly!!
Wait 5s
wecho Turning to you, the Leading Player gestures to the burning wreckage of the fire box.
wecho The Leading Player grins evilly.
Wait 4s
wecho The Leading Player winks as he says, 'You're one of us now kid, part of the troupe.'
Wait 3s
wecho The Leading Player summons the burning wreckage together, creating a small burning circle.
wait 2s
wecho It hovers in his outstretched hand for a moment before he lowers his hand, leaving it suspended in the air.
Wait 4s
wecho The Leading Player says, 'My gift to you,' as he turns to leave.
set person %self.people%
while %person%
   if %person.quest_stage[theatre]% >= 7
      wload obj 4319
      quest variable theatre %person% fire_ring 1
      quest complete theatre %person%
      wforce %person% get fire-ring
   endif
   set person %person.next_in_room%
done
Wait 4s
wecho The Leading Player blows a kiss over his shoulder and slinks off into the shadows.
wait 8
wecho One by one the other players follow, slipping off into the theater.
Wait 6s
wecho You blink and the theater has returned to normal.
set holding %get.room[1100]%
if %holding.people[4399]%
   wat 1100 wteleport leading 4333
endif
~
#4399
The_Finale~
2 b 100
~
if %get.people[4336]% == 0
  halt
else
  if %get.people[4333]%
    set room %get.room[4333]%
    if %room.people[4399]%
      wat 4333 wteleport leading-player 1100
    endif
  endif
  if %self.people[4312]%
    set char %self.people[4312]%
  else
    set char %self.people%
  endif
  wecho The Fire Goddess shouts, 'Ladies and Gentlemen!  We present to you a spectacle never before seen on a public stage!  The only completely perfect act in our repertoire!'
  Wat 4333 wecho The Fire Goddess shouts, 'Ladies and Gentlemen!  We present to you a spectacle never before seen on a public stage!  The only completely perfect act in our repertoire!'
  Wait 4s
  Wecho A chorus of voices shouts out in response, 'THE FINALE!!'
  Wat 4333 Wecho A chorus of voices shouts out in response, 'THE FINALE!!'
  Wait 5s
  Wecho Deep, stirring chords are struck on numerous instruments, resounding like a death knell in the box.
  Wat 4333 wecho Instruments strum to life all around.  
  wait 3s
  wat 4333 wecho Deep, stirring chords break loose from the darkness, haunting the stage.
  Wecho Voices starting to sing outside the box, faintly in the distance.
  wait 3s
  Wat 4333 wecho Ghostly voices float through theater, singing and whispering unintelligibly.
  Wait 7s
  Wecho A single, strong male voice sounds out in the darkness, calling out your name, inviting you to dance.
  Wat 4333 wecho From the darkness of the fire box emerges the Leading Player in all his glory, inviting you to dance.
  Wat 4333 wecho The Leading Player stalks the room with his eyes, crooning, '%char.name%, think about the sun.'
  Wait 6s
  Wat 4333 wecho Pointing to the rafters, the Leading Player screams, 'Let's give this angel some more light!!'
  Wat 4333 wecho Oppressively bright light bursts down from the enormous sun on the theater ceiling.
  wait 6
  Wecho A powerful, driving chord strikes and strikes hard!
  Wat 4333 wecho A powerful, driving chord strikes and strikes hard!
  wait 2s
  wecho The music starts to blast a new, driving beat.
  wat 4333 wecho The music starts to blast a new, driving beat.
  Wait 5s
  Wecho The rhythm starts to pick up, pounding faster and faster, drawing you into the melody, begging your body to dance.
  Wat 4333 wecho The players emerge from all around, moving to the now pounding rhythm.
  Wat 4333 wecho You find yourself swept up into the dance, unable to resist the music.
  wait 3s
  wat 4333 wecho The players swirl about you, tumbling together into a giant clump.
  Wait 5s
  Wecho Suddenly, the box begins to get hotter and hotter, the inside starting to smoke.
  Wat 4333 wecho Moving into a circle around the stage, the players sing, 'When the power and the glory are there at your command!'
  wait 3s
  Wat 4333 wecho They wave their rings at the fire box as the Fire Goddess heats the box with her touch.
  Wait 6s
  Wecho Lights explode through the walls of the box, sparking the wood and lighting the box on fire!
  Wat 4333 wecho Glowing with an inner fire as they dance, the players call out, '%char.name%!!'
  Wait 6s
  Wecho &4Br&b&3i&0&4ll&b&3ia&0&4nt&0 flash charges go off all around, exploding in a barrage of colors!
  Wat 4333 Wecho &4Br&b&3i&0&4ll&b&3ia&0&4nt&0 flash charges go off all around, exploding in a barrage of colors!
  wait 4s
  Wat 4333 wecho The players belt out in unison 'THINK ABOUT THE SUN!!'
  Wait 10
  Wecho A voice cackles from the stage as the box &1E&b&3X&b&1P&0&1L&1O&b&3D&1E&0&1S&0 into flames!!!!
  Wat 4333 wecho The Leading Player cackles as the box &1E&b&3X&b&1P&0&1L&1O&b&3D&1E&0&1S&0 into flames!!!
  set person %self.people%
  while %person%
    if %person.vnum% != -1
      wdamage %person% 1000
    else
      wdamage %person% 200
    endif
    set person %person.next_in_room%
  done
  wait 2
  wteleport all 4333
  wat 4333 wpurge box
  wat 4333 wload obj 4322
  wdoor 4336 down room 4333
  wait 2s
  wdoor 4336 down purge
  if %get.people[1100]%
    set room %get.room[1100]%
    if %room.people[4399]%
      wat 1100 wteleport leading-player 4333
    endif
  endif
endif
~
$~
