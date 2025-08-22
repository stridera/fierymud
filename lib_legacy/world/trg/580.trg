#58001
odai_kannon_receive~
0 j 100
~
* I'm going to restructure this one ever so
* Slightly to purge Kannon after she bolts so
* She's just not hanging out in the void.
*
* Adding in a gem/gear loop as well.
*
if %actor.vnum% == -1
   if %object.vnum% == 58200
      wait 1
      mjunk key
      mecho %self.name% stares in astonishment.
      say You... you're ... releasing me?
      mecho %self.name% looks around the cage, slowly regaining her composure.
      wait 1
      say Please, accept these tokens!
      * boss setup - trigger 55586
      *
      set count 0
      while %count% < 3
         set bonus %random.100%
         set will_drop %random.100%
         * 4 pieces of armor per sub_phase in phase_2
         set what_armor_drop %random.4%
         * 11 classes questing in phase_2
         set what_gem_drop %random.11%
         * 
         if %will_drop% <= 60 
         * Normal non-bonus drops
            if %bonus% <= 50
               * drop a gem from the previous wear pos set
               eval gem_vnum %what_gem_drop% + 55637
               mload obj %gem_vnum%
            elseif (%bonus% >= 51 &%bonus% <= 90)
            * We're in the Normal drops from current wear pos set
               * drop a gem from the current wear pos set 
               eval gem_vnum %what_gem_drop% + 55648
               mload obj %gem_vnum%
            else
            * We're in the BONUS ROUND!!
               * drop a gem from the next wear pos set 
               eval gem_vnum %what_gem_drop% + 55659
               mload obj %gem_vnum%
            endif
         elseif (%will_drop% >=61 &%will_drop% <= 80)
         * Normal non-bonus drops
            if %bonus% <= 50
               * drop destroyed armor 55299 is the vnum before the
               * first piece of armor.
               eval armor_vnum %what_armor_drop% + 55343
               mload obj %armor_vnum%
            elseif (%bonus% >= 51 &%bonus% <= 90)
            * We're in the Normal drops from current wear pos set
               * drop armor from the current wear pos set 
               eval armor_vnum %what_armor_drop% + 55347
               mload obj %armor_vnum%
            else
            * We're in the BONUS ROUND!!
               * drop a piece of armor from next wear pos
               eval armor_vnum %what_armor_drop% + 55351
               mload obj %armor_vnum%
            endif
         else
         * Normal non-bonus drops
            if %bonus% <= 50
               * drop armor and gem from previous wear pos
               eval gem_vnum %what_gem_drop% + 55637
               eval armor_vnum %what_armor_drop% + 55343
               mload obj %gem_vnum%
               mload obj %armor_vnum%
            elseif (%bonus% >= 51 &%bonus% <= 90)
            * We're in the Normal drops from current wear pos set
               * drop a gem and armor from the current wear pos set 
               eval armor_vnum %what_armor_drop% + 55347
               mload obj %armor_vnum%
               eval gem_vnum %what_gem_drop% + 55648
               mload obj %gem_vnum%
            else
            * We're in the BONUS ROUND!!
               * drop armor and gem from next wear pos
               eval gem_vnum %what_gem_drop% + 55659
               mload obj %gem_vnum%
               eval armor_vnum %what_armor_drop% + 55351
               mload obj %armor_vnum%
            endif
         endif
         eval count %count% + 1
      done
      remove all.pearl
      give all %actor.name%
      wait 1
      say I'm FREE!
      mecho The goddess disappears in a flash of blazing light!
   mpurge kannon
   endif
endif
~
#58002
charm_person_hinazuru_greet~
0 g 100
~
set stage %actor.quest_stage[charm_person]%
wait 2
switch %stage%
  case 1
    say I see you have returned.
    bow %actor.name%
    wait 1s
    say Have you found %get.obj_shortdesc[48008]%?
    break
  case 2
    say Welcome back.
    bow %actor.name%
    wait 1s
    say I hope you have been able to observe the methods of the troupe and return with a trophy.
    break
  case 3
    say I am delighted to see you again.
    bow %actor.name%
    wait 1s
    say How is your search for the instruments coming along?  I truly look forward to hearing your tales.
    break
  case 4
    mecho %self.name% says, 'Welcome back, I was not expecting to see you again so soon.  Are you having trouble locating your conversation partners?  I can update you on your &7&b[spell progress]&0 if you are.'
    break
  default
    say Welcome honored guest.
    bow
    wait 1s
    say I am delighted to host you.
    if (%actor.class% /= Sorcerer || %actor.class% /= Illusionist || %actor.class% /= Bard) && %actor.level% > 88 && %stage% == 0
      wait 1s
      say You seem like you might be interested in some of my specialty services.
    endif
done
~
#58003
charm_person_hinazuru_speech1~
0 d 1
specialty services yes like what? services? speciality?~
if (%actor.class% /= Sorcerer || %actor.class% /= Illusionist || %actor.class% /= Bard)
  if %actor.level% > 88
    if %actor.quest_stage[charm_person]% == 0
      wait 2
      say I am not only a master of conversational and social arts, I am also highly trained in the mystic arts. Sorcerers and illusionists come from all across the world to learn my signature spell for charming others.
      wait 2s
      say If you are interested, I could teach you.  For a price.  100 platinum, at once, in advance, is my fee.
    elseif %actor.quest_stage[charm_person]% == 1
      say Marvelous!
      wait 1s
      say If you would be so kind as to give it to me, I can demonstrate its power.
    endif
  else
    say Your determination is admirable, but I fear you still have some learning to do before I can teach you.
  endif
endif
~
#58004
charm_person_hinazuru_pay~
0 m 100000
~
if (%actor.class% /= Sorcerer || %actor.class% /= Illusionist || %actor.class% /= Bard) && %actor.level% > 88 && %actor.quest_stage[charm_person]% == 0
  quest start charm_person %actor.name%
  bow %actor.name%
  say I give humble thanks for your payment.
  wait 2s
  say The first thing you will need is an implement which casts the spell you seek to learn so you may experiment with it.
  wait 2s
  say Only one such item exists in the world: a simple black metal rod buried in an extensive crypt in the Iron Hills.
  wait 2s
  say Please retrieve it and bring it back to me.
  bow %actor.name%
  wait 1s
  mecho %self.name% says, 'During this training, if you need to check your &7&b[spell progress]&0, you may ask at any time.'
  bow %actor.name%
else
  say I am deeply honored by your gratuity.
  bow %actor.name%
endif
~
#58005
charm_person_hinazuru_receive~
0 j 100
~
set stage %actor.quest_stage[charm_person]%
if %stage% == 1
  if %object.vnum% == 48008
    quest advance charm_person %actor.name%
    wait 2
    mjunk discipline
    wait 1s
    say Thank you kindly for bringing this so quickly.
    bow %actor.name%
    wait 2s
    msend %actor% %self.name% gives you an explanation and demonstration of %get.obj_shortdesc[48008]%'s power and function.
    wait 3s
    say Having seen this magic in a controlled environment now, I believe it's best to see it in action.
    wait 2s
    say Many people underestimate the power of charms and enchantments.  They discard them as ineffective or weak compared to more violent magic.
    wait 4s
    say Yet there is a company of performers in Anduin that proves the most insidious spells do not harm people directly, but convince them to harm themselves.
    wait 4s
    say They perform a deadly song and dance number which results in the sacrifice of an unwitting accomplice.  They give out rings of fire to those who participated at the end of the show.
    wait 4s
    say Go and watch this in action.  Bring back the fire ring as a token of your experience.
    bow %actor.name%
  else
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    say This is not the Rod of Discipline.
  endif
elseif %stage% == 2
  if %object.vnum% == 4319
    quest advance charm_person %actor.name%
    wait 2
    mjunk ring
    wait 1s
    say Thank you kindly for bringing this so quickly.
    bow %actor.name%
    wait 2s
    say It's horrific what charms can make people do, isn't it?
    wait 1s
    shake
    wait 1s
    say Yet I am glad you have seen it now so you understand the risks involved in charming and beguiling.
    wait 3s
    say I also had you observe the theatre because you need to have at least a limited knowledge of music.
    wait 3s
    say As a courtesan, I am expected to be a master of several instruments, as well as song and dance.  As a magician, I use this same mastery as a key element in my spells.
    wait 2s
    say You will need to seek out musical instruments to hone your charming skills.
    wait 4s
    mecho %self.name% says, 'There are five instruments you must find:
    mecho - One is merely the &3&bbiwa&0 held by Yoshino down the hall&_
    mecho - Two are &3&bflutes&0 hidden from the sun:
    mecho &0    One &3&bdeep in the sea to the east&0
    mecho &0    The other &3&bin a mine near Anduin&0&_
    mecho - One is a &3&bhand-fashioned reed pipe&0, popular with people far far to the west&_
    mecho - The final is &3&ba mandolin&0, found in the homes of the gods'
    wait 4s
    say Once you have found all five of these, come back to me so I may check their condition and ensure their proper tuning.
    bow %actor.name%
  else
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    say This is not one of the souvenirs the theatre company gives out after a performance.
  endif
elseif %stage% == 3
  if %object.vnum% == 58017 || %object.vnum% == 16312 || %object.vnum% == 48925 || %object.vnum% == 37012 || %object.vnum% == 41119
    if %actor.quest_variable[charm_person:%object.vnum%]%
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      say You have already brought me %object.shortdesc%.
    else
      quest variable charm_person %actor.name% %object.vnum% 1
      wait 2
      mecho %self.name% checks %object.shortdesc% for tuning and quality.
      wait 1s
      say Yes, this should do.  Now come sit, it is time for a lesson.
      wait 2s
      msend %actor.name% %self.name% begins to teach you several basic techniques and a few melodies.
      mechoaround %actor% %self.name% begins to teach %actor.name% several basic techniques and a few melodies.
      wait 5s
      msend %actor% You begin to get a feel for it...
      wait 7s
      msend %actor% After several hours, you seem to have picked up the basics!
      mechoaround %actor% After several hours, %actor.name% seems to have picked up the basics!
      give all %actor%
      say Hold on to that instrument, you'll need it later.
      wait 2
      if %actor.quest_variable[charm_person:58017]% && %actor.quest_variable[charm_person:16312]% && %actor.quest_variable[charm_person:48925]% && %actor.quest_variable[charm_person:37012]% && %actor.quest_variable[charm_person:41119]%
        quest advance charm_person %actor.name%
        wait 4
        say You now have at least a modicum of experience in five instruments.  It is time to put your skills to the test and see what you have learned!
        wait 2s
        mecho %self.name% says, 'There are five master charmers in the world.  Take these instruments out and see if you can impress them.  Each one prefers a different kind of music, so be ready to show off your skills!
        mecho &0 
        mecho &0- One runs the &5&bjewelry shop in Mielikki.&0
        mecho &0- One is a local &5&bactor&0 here on the &5&bEmerald Island.&0
        mecho &0- One works with the &5&bacting company in Anduin&0, playing their &5&bQueen.&0
        mecho &0- One is a &5&bhideous creature that enslaves minds&0 in the dark &5&bdrow city.&0
        mecho &0- The last can be any of the &5&briver nymphs&0 in the &5&bRealm of the King of Dreams.&0&_
        mecho &0Say to them &5&b[Let me serenade you]&0 and see what they do.'
        wait 5s
        say It is here our paths must split.
        wait 1s
        say It has been my pleasure to work with you.
        wait 1s
        say Farewell.
        bow %actor.name%
      else
        eval music (%actor.quest_variable[charm_person:58017]% + %actor.quest_variable[charm_person:16312]% + %actor.quest_variable[charm_person:48925]% + %actor.quest_variable[charm_person:37012]% + %actor.quest_variable[charm_person:41119]%)
        if %music% > 0 && %music% < 4
          say Do you have the other instruments?
        elseif %music% == 4
          say Do you have the last instrument?
        endif
      endif
    endif
  else
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    say %object.shortdesc% is not a musical instrument.
  endif
else
  return 0
  Shake
  mecho %self.name% refuses %object.shortdesc%.
  say I am in need of nothing at the moment, thank you.
endif
~
#58006
charm_person_serenade_speech~
0 d 0
let me serenade you~
if %actor.quest_stage[charm_person]% == 4
  wait 2
  switch %self.vnum%
    case 3010
      say A little music sounds delightful.  I enjoy the gentle strum of a mandolin.
      break
    case 58017
      say How unconventional!  Usually I'm the one doing the wooing.  I dearly love the willowy call of the flute.
      break
    case 4353
      say Oh of course.
      lick
      wait 1s
      say What can I say, I'm a sucker for a hot, long pipe.
      grin
      break
    case 23721
      msend %actor% %self.name% tells you telepathically, 'A very strange request.  The only surface-dwelling music I can tolerate is the lute.'
      break
    case 58406
      say I would love to hear a tune!  Instruments that sound like the wind meeting the water are my favorite.
      break
    default
      return 0
  done
endif
~
#58007
charm_person_instruments_command~
1 n 100
~
if %actor.quest_stage[charm_person]% == 4
  return 0
  set room %actor.room%
  switch %self.vnum%
    case 48925
      if %room.people[3010]%
        wait 2
        oecho %get.mob_shortdesc[3010]% hums along dreamily.
        quest variable charm_person %actor.name% charm1 1
        osend %actor% &5&b%get.mob_shortdesc[3010]% is charmed by your playing!&0
      endif
      break
    case 37012
      if %room.people[58017]%
        wait 2
        oecho %get.mob_shortdesc[58017]% blushes furiously.
        quest variable charm_person %actor.name% charm2 1
        osend %actor% &5&b%get.mob_shortdesc[58017]% is charmed by your playing!&0
      endif
      break
    case 41119
      if %room.people[58406]%
        wait 2
        oecho %get.mob_shortdesc[58406]% sighs sweetly.
        quest variable charm_person %actor.name% charm5 1
        osend %actor% &5&b%get.mob_shortdesc[58406]% is charmed by your playing!&0
      endif
      break
    case 16312
      if %room.people[4353]%
        wait 2
        oecho %get.mob_shortdesc[4353]% closes her eyes and smiles.
        quest variable charm_person %actor.name% charm3 1
        osend %actor% &5&b%get.mob_shortdesc[4353]% is charmed by your playing!&0
      endif
      break
    case 58017
      if %room.people[23721]%
        wait 2
        oecho %get.mob_shortdesc[23721]% burbles with contentment.
        quest variable charm_person %actor.name% charm4 1
        osend %actor% &5&b%get.mob_shortdesc[23721]% is charmed by your playing!&0
      endif
  done
  if %actor.quest_variable[charm_person:charm1]% && %actor.quest_variable[charm_person:charm2]% && %actor.quest_variable[charm_person:charm3]% && %actor.quest_variable[charm_person:charm4]% && %actor.quest_variable[charm_person:charm5]%
    wait 4
    osend %actor% Your skill in charming has greatly improved!
    osend %actor% Hinazuru's training has paid off!
    quest complete charm_person %actor.name%
    osend %actor% &5&bYou have learned Charm Person!&0
    oskillset %actor.name% charm person
  endif
endif
~
#58008
charm_person_status_checker~
0 d 0
spell progress~
wait 2
set stage %actor.quest_stage[charm_person]%
if %actor.has_completed[charm_person]%%
  say I have already taught you my signature spell.
elseif %stage% == 0
  say I have not yet agreed to train you.
elseif %stage% == 1
  mecho %self.name% says, 'You must find the rod that casts Charm Person in the crypt in
  mecho &0the Iron Hills.'
elseif %stage% == 2
  mecho %self.name% says, 'Help the theatre company in Anduin perform their grand finale
  mecho &0and bring back the unique fire ring they give out afterward.'
  wait 1s
  mecho %self.name% says, 'They have a number of problems which you will need to work out
  mecho &0before you can seek out their replacement "Pippin" and lure him to his fiery grave.'
elseif %stage% == 3
  say Locate five musical instruments and bring them to me.
  if %actor.quest_variable[charm_person:58017]% || %actor.quest_variable[charm_person:16312]% || %actor.quest_variable[charm_person:48925]% || %actor.quest_variable[charm_person:37012]% || %actor.quest_variable[charm_person:41119]%
    mecho 
    mecho You have already brought me:
    if %actor.quest_variable[charm_person:58017]%
      mecho - &5%get.obj_shortdesc[58017]%&0
    endif
    if %actor.quest_variable[charm_person:16312]%
      mecho - &5%get.obj_shortdesc[16312]%&0
    endif
    if %actor.quest_variable[charm_person:48925]%
      mecho - &5%get.obj_shortdesc[48925]%&0
    endif
    if %actor.quest_variable[charm_person:37012]%
      mecho - &5%get.obj_shortdesc[37012]%&0
    endif
    if %actor.quest_variable[charm_person:41119]%
      mecho - &5%get.obj_shortdesc[41119]%&0
    endif
  endif
  mecho   
  mecho You still need to find:
  if !%actor.quest_variable[charm_person:58017]%
    mecho - &5&b%get.obj_shortdesc[58017]%&0
  endif
  if !%actor.quest_variable[charm_person:16312]%
    mecho - &5&b%get.obj_shortdesc[16312]%&0
  endif
  if !%actor.quest_variable[charm_person:48925]%
    mecho - &5&b%get.obj_shortdesc[48925]%&0
  endif
  if !%actor.quest_variable[charm_person:37012]%
    mecho - &5&b%get.obj_shortdesc[37012]%&0
  endif
  if !%actor.quest_variable[charm_person:41119]%
    mecho - &5&b%get.obj_shortdesc[41119]%&0
  endif
elseif %stage% == 4
  say You must charm the five master charmers.
  mecho Ask them &7&b[Let me serenade you]&0.
  if %actor.quest_variable[charm_person:charm1]% || %actor.quest_variable[charm_person:charm2]% || %actor.quest_variable[charm_person:charm3]% || %actor.quest_variable[charm_person:charm4]% || %actor.quest_variable[charm_person:charm5]%
    mecho 
    mecho You have already charmed:
    if %actor.quest_variable[charm_person:charm1]%
      mecho - &5%get.mob_shortdesc[3010]%&0
    endif
    if %actor.quest_variable[charm_person:charm2]%
      mecho - &5%get.mob_shortdesc[58017]%&0
    endif
    if %actor.quest_variable[charm_person:charm3]%
      mecho - &5%get.mob_shortdesc[4353]%&0
    endif
    if %actor.quest_variable[charm_person:charm4]%
      mecho - &5%get.mob_shortdesc[23721]%&0
    endif
    if %actor.quest_variable[charm_person:charm5]%
      mecho - &5%get.mob_shortdesc[58406]%&0
    endif
  endif
  mecho   
  mecho You still need to find:
  if !%actor.quest_variable[charm_person:charm1]%
    mecho - &5&b%get.mob_shortdesc[3010]%&0
  endif
  if !%actor.quest_variable[charm_person:charm2]%
    mecho - &5&b%get.mob_shortdesc[58017]%&0
  endif
  if !%actor.quest_variable[charm_person:charm3]%
    mecho - &5&b%get.mob_shortdesc[4353]%&0
  endif
  if !%actor.quest_variable[charm_person:charm4]%
    mecho - &5&b%get.mob_shortdesc[23721]%&0
  endif
  if !%actor.quest_variable[charm_person:charm5]%
    mecho - &5&b%get.mob_shortdesc[58406]%&0
  endif
endif
~
#58009
play command normalizer~
1 c 3
p~
return 0
~
#58010
charm_person_mobs_play_command~
0 c 100
play~
if %actor.quest_stage[charm_person]% != 4
  return 0
  halt
endif
switch %arg%
  case mandolin
    if %self.vnum% != 3010
      return 0
      halt
    elseif %actor.quest_stage[charm_person]% == 4 && %self.vnum% ==3010 && (%actor.wearing[48925]% || %actor.inventory[48925]%)
      msend %actor% You strum a beautiful tune on the mandolin.
      mechoaround %actor% %actor.name% strums a beautiful tune on the mandolin.
      wait 2
      mecho %self.name% hums along dreamily.
      quest variable charm_person %actor.name% charm1 1
      msend %actor% &5&b%self.name% is charmed by your playing!&0
    else
      return 0
      halt
    endif
    break
  case flute
    if %actor.quest_stage[charm_person]% == 4 && %self.vnum% == 58017 &&  (%actor.wearing[37012]% || %actor.inventory[37012]%)
      msend %actor% You play a melodious tune on the flute.
      mechoaround %actor% %actor.name% plays a melodious tune on the flute.
      wait 2
      mecho %self.name% blushes furiously.
      quest variable charm_person %actor.name% charm2 1
      msend %actor% &5&b%self.name% is charmed by your playing!&0
    elseif %actor.quest_stage[charm_person]% == 4 && %self.vnum% == 58406 && (%actor.wearing[41119]% || %actor.inventory[41119]%)
      msend %actor% You play a haunting, dulcet tune on the Sea's Flute.
      mechoaround %actor% %actor.name% plays a haunting, dulcet tune on the Sea's Flute.
      wait 2
      mecho %self.name% sighs with nostalgia and longing.
      quest variable charm_person %actor.name% charm5 1
      msend %actor% &5&b%self.name% is charmed by your playing!&0
    else
      return 0
      halt
    endif
    break
  case pipe
    if %actor.quest_stage[charm_person]% == 4 && %self.vnum% == 4353 && (%actor.wearing[16312]% || %actor.inventory[16312]%)
      msend %actor% You blow a wistful melody on the pipe.
      mechoaround %actor% %actor.name% blows a wistful melody on the pipe.
      wait 2
      mecho %self.name% closes her eyes and smiles.
      quest variable charm_person %actor.name% charm3 1
      msend %actor% &5&b%self.name% is charmed by your playing!&0
    else
      return 0
      halt
    endif
    break
  case biwa
    if %actor.quest_stage[charm_person]% == 4 && %self.vnum% == 23721 && (%actor.wearing[58017]% || %actor.inventory[58017]%)
      msend %actor% You pluck out a strange, complex arrangement on the biwa.
      mechoaround %actor% %actor.name% plucks out a strange, complex arrangement on the biwa.
      wait 2
      mecho %self.name% burbles with contentment.
      quest variable charm_person %actor.name% charm4 1
      msend %actor% &5&b%self.name% is charmed by your playing!&0
    else
      return 0
      halt
    endif
    break
  case sea
  case seas
    if %actor.quest_stage[charm_person]% == 4 && %self.vnum% == 58406 && (%actor.wearing[41119]% || %actor.inventory[41119]%)
      msend %actor% You play a haunting, dulcet tune on the Sea's Flute.
      mechoaround %actor% %actor.name% plays a haunting, dulcet tune on the Sea's Flute.
      wait 2
      mecho %self.name% sighs with nostalgia and longing.
      quest variable charm_person %actor.name% charm5 1
      msend %actor% &5&b%self.name% is charmed by your playing!&0
    else
      return 0
      halt
    endif
    break
  default
   return 0
done
if %actor.quest_variable[charm_person:charm1]% && %actor.quest_variable[charm_person:charm2]% && %actor.quest_variable[charm_person:charm3]% && %actor.quest_variable[charm_person:charm4]% && %actor.quest_variable[charm_person:charm5]%
  wait 4
  msend %actor% Your skill in charming has greatly improved!
  msend %actor% Hinazuru's training has paid off!
  quest complete charm_person %actor.name%
  msend %actor% &5&bYou have learned Charm Person!&0
  mskillset %actor.name% charm person
endif
~
#58011
**UNUSED**~
0 c 100
p~
return 0
~
#58100
exit_room_58196~
2 g 100
~
if %direction% == north
    if !%running%
        set running 1
        global running
        wait 1s
        set rnd %random.char%
        wsend %rnd% An image of a goddess appears before you for a second - she holds her hands out to you.
        wait 1 s
        wsend %rnd% The goddess says, 'Speak my name to leave this room, Yajiro was ever the joker...'
        wait 1s
        wsend %rnd% The goddess gets enfolded into a large pearl and then suddenly she disappears.
        unset running
    endif
endif
~
#58101
exit_room_58196_part_2~
2 d 100
kannon kwan yin~
wecho You can hear a faint voice say, 'I wish I could do more to help you...'
wecho Your vision blurs for a moment and there is a noise as of a rushing wind.
wteleport all 58194
wat 58194 wforce all look
~
#58102
Sunbird_greet~
0 g 100
~
if %actor.vnum% == -1 &&%actor.level% < 100
  Wait 1
  Bow %actor%
  say Come, take shelter in the holy light of Kannon, Goddess of Mercy.
end
~
#58103
Sunbird_speech1~
0 d 100
kannon kannon? who? goddess goddes? mercy mercy?~
Wait 7
Say Through her holy benevolence, I once protected this entire island.
Mecho The Sunbird spreads its wings and begins to radiate a &b&7glowing&0 &b&3l&0&b&7i&b&3g&0&b&7h&0&b&3t.&0
Wait 15
Mecho &b&7The light grows...&0
Wait 15
Mecho The light suddenly &b&7FL&0&b&3AR&0&b&7ES!&0
Wait 7
Mecho The Sunbird falters and stumbles before the altar.
wait 8
Say But now her divine presence has waned
say Now I can only shelter this small space near her shrine.
Say I fear something terrible has happened to her...
~
#58104
Sunbird_speech2~
0 d 100
what? terrible? something? happened?~
Wait 5
Nod
Wait 10
Say A conspiracy has infected Odaishyozen.
Say Shadowy figures have been plotting with Yajiro,
Say the court sorcerer, to capture Kannons light.
Say It seems they have even ensnared innocent souls in their trap.
Wait 12
Ponder
Wait 12
Say But perhaps you can help untangle these threads of karma
say Will you investigate what has happened to Kannon?
~
#58105
Sunbird_speech3~
0 d 100
yes~
Wait 1
Emote beams with joy.
Wait 5
Say The tea master recently left this here
say after offering prayers to Kannon for forgiveness.  
Say He muttered something about a ceremony with Yajiro.
Wait 3
Mload obj 58109
Give key %actor%
Say Perhaps he holds some kind of key to the conspiracy.
~
#58106
Chajin_speech~
0 d 100
conspiracy conspiracy? key key? bowl bowl? Kannon kannon? Plot plot? Yajiro yajiro? Ceremony ceremony?~
Wait 2
Emote sighs with the heavy weight of his burden.
Wait 6
Say So the day has come.
say Surely I will be punished for this karma in my next life.
Say I shall atone through my blood.
Wait 4
Emote readies himself for combat.
~
#58107
Sunbird_speech1_bow~
0 c 100
bow~
return 0
switch %cmd%
  case b
    halt
done
if %self.name% /= %arg%
  Wait 7
  Say Through her holy benevolence, I once protected this entire island.
  Mecho The Sunbird spreads its wings and begins to radiate a &b&7glowing&0 &b&8l&0&b&7i&b&3g&0&b&7h&0&b&8t.&0
  Wait 15
  Mecho &b&7The light grows...&0
  Wait 15
  Mecho The light suddenly &b&7FL&0&b&3AR&0&b&7ES!&0
  Wait 7
  Mecho The Sunbird falters and stumbles before the altar.
  wait 8
  Say But now her divine presence has waned.
  say I can only shelter this small space near her shrine.
  Say I fear something terrible has happened to her...
endif
~
#58108
**UNUSED**~
0 e 100
bo~
return 0
~
$~
