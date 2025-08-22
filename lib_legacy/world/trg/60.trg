#6001
drunk_death_bottle_break~
0 f 100
~
mecho The bottle of whisky slips from %self.name%'s hand and shatters as it hits the ground!
mjunk all.drunkdrink
~
#6002
drunk_socail_stunts3~
0 b 30
~
sing
hiccup
say I used to be a big cheese round here you know.
hiccup
~
#6003
Green Woman Apothecary shop load~
0 o 100
~
wait 2s
set a 1
while %a% < 6
   set herb %random.12%
   switch %herb%
     case 1
       mload obj 12552
       break
     case 2
       mload obj 23750
       break
     case 3
       mload obj 23751
       break
     case 4
       mload obj 23752
       break
     case 5
       mload obj 23753
       break
     case 6
       mload obj 23754
       break
     case 7
       mload obj 23755
       break
     case 8
       mload obj 23756
       break
     case 9
       mload obj 23757
       break
     case 10
       mload obj 23758
       break
     case 11
       mload obj 23759
       break
     default
       mload obj 49022
   done
   eval a %a% + 1
done
unset a
set potion1 %random.12%
switch %potion1%
   case 1
     mload obj 3248
     break
   case 2
     mload obj 3251
     break
   case 3
     mload obj 3254
     break
   case 4
     mload obj 3257
     break
   case 5
   case 6
     mload obj 3261
     break
   case 7
   case 8
     mload obj 3264
     break
   case 9
   case 10
     mload obj 3267
     break
   default
     mload obj 3270
done
set potion2 %random.16%
switch %potion2%
   case 1
   case 2
      mload obj 3273
      break
   case 3
      mload obj 3275
      break
   case 4
   case 5
      mload obj 3277
      break
   case 6
      mload obj 3279
      break
   case 7
   case 8
      mload obj 3281
      break
   case 9
      mload obj 3283
      break
   case 10
   case 11
      mload obj 3285
      break
   case 12
      mload obj 3287
      break
   case 13
   case 14
      mload obj 3289
      break
   case 15
      mload obj 3293
      break
   default
      mload obj 3295
done
set a 1
while %a% < 4
   set potion3 %random.35%
   switch %potion3%
     case 1
       mload obj 1613
       break
     case 2
       mload obj 1620
       break
     case 3
       mload obj 1621
       break
     case 4
       mload obj 1622
       break
     case 5
       mload obj 3051
       break
     case 6
       mload obj 3059
       break
     case 7
       mload obj 3138
       break
     case 8
       mload obj 3200
       break
     case 9
       mload obj 3215
       break
     case 10
       mload obj 3216
       break
     case 11
       mload obj 3229
       break
     case 12
       mload obj 3230
       break
     case 13
       mload obj 3246
       break
     case 14
       mload obj 5100
       break
     case 15
       mload obj 5207
       break
     case 16
       mload obj 6331
       break
     case 17
       mload obj 6382
       break
     case 18
       mload obj 7302
       break
     case 19
       mload obj 8341
       break
     case 20
       mload obj 8342
       break
     case 21
       mload obj 8345
       break
     case 22
       mload obj 8346
       break
     case 23
       mload obj 8705
       break
     case 24
       mload obj 8706
       break
     case 25
       mload obj 11707
       break
     case 26
       mload obj 16303
       break
     case 27
       mload obj 16900
       break
     case 28
       mload obj 16906
       break
     case 29
       mload obj 17206
       break
     case 30
       mload obj 30214
       break
     case 31
       mload obj 41010
       break
     case 32
       mload obj 41115
       break
     case 33
       mload obj 51008
       break
     default
       mload obj 52106
   done
   eval a %a% + 1
done
set flower %random.6%
switch %flower%
   case 1
     mload obj 6907
     break
   case 2
     mload obj 18003
     break
   case 3
     mload obj 37013
     break
   case 4
     mload obj 61510
     break
   case 5
   default
     mload obj 58355
done
set bonus %random.3%
if %bonus% == 1
  set armor %random.40%
  if %armor% < 11
    mload obj 13647
  elseif %armor% < 21
    set item %random.4%
    switch %item%
      case 1
        mload obj 41116
        break
      case 2
        mload obj 2329
        break
      case 3
        mload obj 12337
        break
      default
        mload obj 12340
    done
  elseif %armor% < 31
    set item %random.5%
    switch %item%
      case 1
        mload obj 6223
        break
      case 2
        mload obj 12327
        break
      case 3
        mload obj 12325
        break
      case 4
        mload obj 12328
        break
      default
        mload obj 12326
    done
  elseif %armor% < 40
    set item %random.4%
    switch %item%
      case 1
      case 2
        mload obj 49014
        break
      default
        mload obj 49063
    done
  elseif %armor% == 40
    mload obj 58431
  endif
endif
~
#6005
quest_timulos_greet~
0 h 100
~
wait 2
set quest_name %actor.quest_variable[merc_ass_thi_subclass:subclass_name]%
switch %actor.quest_stage[merc_ass_thi_subclass]%
  case 1
    if %quest_name% == thief
      msend %actor% %self.name% says, 'Back to try your hand at being a thief?  There is a &6&bpackage&0 that someone could get back.'
    elseif %quest_name% == mercenary
      msend %actor% %self.name% says, 'If you want to continue training as a mercenary, listen up.  A &6&blord&0 needs our services.'
    elseif %quest_name% == assassin
      msend %actor% %self.name% says, 'Ah, you are back!  I believe I have a job for a trainee assassin.  One that would bring a good &6&bprice&0.'
    endif
    break
  case 2
    if %quest_name% == thief
      msend %actor% %self.name% says, 'You getting cold feet about being a thief?  It's just stealing a package from some farmers.'
      grumble
      wait 1s
      msend %actor% %self.name% says, 'Bloody &6&bfarmers&0.'
    elseif %quest_name% == mercenary
      msend %actor% %self.name% says, 'Back to continue your mercenary training?  You're probably wondering about the &6&bcloak&0.'
    elseif %quest_name% == assassin
      msend %actor% %self.name% says, 'Stop wasting my time and finish your assassin training. I have some rich men unhappy with the politics of the region in question.  You could help with those &6&bpolitics&0 if you wish.'
    endif
    break
  case 3
  case 4
    if %quest_name% == thief
      msend %actor% %self.name% says, 'Hurry up and finish your thief training.  Deliver the &3&bpackage&0 those Mielikki farmers walked off with!'
    elseif %quest_name% == mercenary
      msend %actor% %self.name% says, 'Have you put your mercenary potential to good use?  Give me the &3&bcloak&0 the insect warriors stole.'
    elseif %quest_name% == assassin
      msend %actor% %self.name% says, 'Is the job done?'
      wait 2
      msend %actor% %self.name% says, 'Shhhh, don't say anything!  Assassins keep their secrets.'
      wait 1s
      msend %actor% %self.name% says, 'But do give me the target's &3&bcane&0 if you have it.'
    endif
    break
  default
    if %actor.has_failed[merc_ass_thi_subclass]%
      msend %actor% %self.name% says, 'That could have gone better.  I see you failed your mission.'
      wait 2
      msend %actor% %self.name% says, 'I suppose you want to try again.'
    elseif %actor.class% /= Rogue && (%actor.level% >= 10 && %actor.level% <= 25)
      msend %actor% %self.name% says, '%actor.name%, have you come to me for training?'
      wait 2
      msend %actor% %self.name% says, 'Well?  Do you want &6&btraining&0 %actor.name% or are you just going to stand there?'
      tap
    elseif %actor.class% /= Rogue && %actor.level% < 10
      msend %actor% %self.name% says, '%actor.name%, have you come to me for trai- '
      wait 2
      emote stops cold.
      wait 1s
      eye %actor%
      wait 2s
      msend %actor% %self.name% says, 'You might be a bit green for my training, kid.'
    endif
done
~
#6006
quest_timulos_training~
0 d 100
training~
wait 2
if %actor.class% /= Rogue
  if (%actor.level% >= 10 && %actor.level% <= 25)
    if !%use_subclass%
      nod %actor.name%
      msend %actor% %self.name% says, 'Well which would you like to train as: &3mercenary&0, &1assassin&0, or &1&bthief&0?'
      wait 1s
      msend %actor% %self.name% says, 'Say one of them.'
    else
      msend %actor% %self.name% says, 'One moment, I'm getting someone else set up.'
      msend %actor% %self.name% ushers you into the corner.
      mechoaround %actor% %self.name% ushers %actor.name% into the corner.
      wait 2s
      msend %actor% %self.name% says, 'Wait there.'
    endif
  elseif %actor.level% < 10
    msend %actor% %self.name% says, 'I like your zeal, but it's a little too soon for you to subclass, kid.'
  else
    msend %actor% %self.name% says, 'It's waaaaaaay too late to train you.  That ship has sailed!'
  endif
else
  msend %actor% %self.name% says, 'Go find someone else to train with.'
  wait 2
  * this may need to be changed in the future if new classes are invented that aren't just <name>s for pluralizing.
  msend %actor% %self.name% says, 'I don't work with %actor.class%s.'
endif
~
#6007
quest_timulos_merc~
0 d 100
mercenary~
if %actor.class% /= Rogue
  switch %actor.race%
*    case ADD NEW RESTRICTED RACES HERE
*      if %actor.level% >= 10 && %actor.level% <= 25
*        msend %actor% &1Your race may not subclass to mercenary.&0
*        halt
*      endif
    default
      wait 2
      if %actor.level% >= 10 && %actor.level% <= 25
        if %use_subclass%
          msend %actor% %self.name% says, 'One moment, I'm getting someone else set up.'
          msend %actor% %self.name% ushers you into the corner.
          mechoaround %actor% %self.name% ushers %actor.name% into the corner.
          wait 2s
          msend %actor% %self.name% says, 'Wait there.'
        else
          msend %actor% %self.name% says, 'So, you wish to become a mercenary do you?'
          set use_subclass Mer
          global use_subclass
        endif
      elseif %actor.level% < 10
        msend %actor% %self.name% says, 'I like your zeal, but it's a little too soon for you to subclass kid.'
      else
        msend %actor% %self.name% says, 'It's waaaaaaay too late to train you.  That ship has sailed!'
      endif
  done
else
  msend %actor% %self.name% says, 'I do not know how to train you to be one, sorry, go away.'
endif
~
#6008
quest_timulos_assassin~
0 d 100
assassin~
if %actor.class% /= Rogue
  switch %actor.race%
*    case ADD NEW RESTRICTED RACES HERE
*      if %actor.level% <= 25
*        msend %actor% &1Your race may not subclass to assassin.&0
*        halt
*      endif
    default
      wait 2
      if (%actor.level% >= 10 && %actor.level% <= 25)
        if %actor.align% <= -350
          if %use_subclass%
            msend %actor% %self.name% says, 'One moment, I'm getting someone else set up.'
            msend %actor% %self.name% ushers you into the corner.
            mechoaround %actor% %self.name% ushers %actor.name% into the corner.
            wait 2s
            msend %actor% %self.name% says, 'Wait there.'
          else
            msend %actor% %self.name% says, 'So, you wish to become an assassin do you?'
            set use_subclass Ass
            global use_subclass
          endif
        else
          msend %actor% %self.name% says, 'You aren't nearly evil enough yet.'
          wait 1s
          msend %actor% %self.name% says, 'Go kill some more people.'
          wait 3s
          msend %actor% %self.name% says, 'Really good, really innocent people.'
        endif
      elseif %actor.level% < 10
        msend %actor% %self.name% says, 'I like your zeal, but it's a little too soon for you to subclass kid.'
      else
        msend %actor% %self.name% says, 'It's waaaaaaay too late to train you.  That ship has sailed!'
      endif
  done
else
  msend %actor% %self.name% says, 'I do not know how to train you to be one, sorry, go away.'
endif
~
#6009
quest_timulos_thief~
0 d 100
thief~
if %actor.class% /= Rogue
  switch %actor.race%
*    case ADD NEW RESTRICTED RACES HERE
*      if %actor.level% <= 25
*        msend %actor% &1Your race may not subclass to thief.&0
*        halt
*      endif
    default
      wait 2
      if (%actor.level% >= 10 && %actor.level% <= 25)
        if %use_subclass%
          msend %actor% %self.name% says, 'One moment, I'm getting someone else set up.'
          msend %actor% %self.name% ushers you into the corner.
          mechoaround %actor% %self.name% ushers %actor.name% into the corner.
          wait 2s
          msend %actor% %self.name% says, 'Wait there.'
        else
          msend %actor% %self.name% says, 'So, you wish to become a thief do you?'
          set use_subclass Thi
          global use_subclass
        endif
      elseif %actor.level% < 10
        msend %actor% %self.name% says, 'I like your zeal, but it's a little too soon for you too subclass kid.'
      else
        msend %actor% %self.name% says, 'It's waaaaaaay too late to train you.  That ship has sailed!'
      endif
  done
else
  msend %actor% %self.name% says, 'I do not know how to train you to be one, sorry, go away.'
endif
~
#6010
quest_timulous_yesno~
0 d 100
yes no~
wait 2
if (%actor.class% /= Rogue && (%actor.level% >= 10 && %actor.level% <= 25))
  if %speech% /= yes
    if %actor.has_failed[merc_ass_thi_subclass]%
      quest restart merc_ass_thi_subclass %actor%
      quest advance merc_ass_thi_subclass %actor%
      quest advance merc_ass_thi_subclass %actor%
      nod
      msend %actor% %self.name% says, 'Then try again.'
      wait 2
      if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == thief
        msend %actor% %self.name% says, 'Be careful this time!  Stay hidden getting in and out.'
        wait 2s
        msend %actor% %self.name% says, 'And no murdering anyone!'
      elseif %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == assassin
        msend %actor% %self.name% says, 'And be quick about it!'
      endif
      halt
    endif
    if %use_subclass%
      quest start merc_ass_thi_subclass %actor.name% %use_subclass%
      nod
      msend %actor% %self.name% says, 'So, you truly wish to continue.'
      wait 1
      if %use_subclass% /= Mer
        msend %actor% %self.name% says, 'Yes, a mercenary would serve well for that...'
        emote thinks back for a moment.
        wait 1s
        msend %actor% %self.name% says, 'He would pay well.  Yes, that &6&bLord&0 would pay well indeed.'
      elseif %use_subclass% /= Ass
        msend %actor% %self.name% says, 'An assassin would be perfect, but to show your true desire...'
        emote grins deeply, curling his lip up.
        wait 1s
        msend %actor% %self.name% says, 'Yes, that would bring a good &6&bprice&0.'
      elseif %use_subclass% /= Thi
        msend %actor% %self.name% says, 'Hmmm, a true thief would have to get something I think.'
        emote smiles cruelly.
        wait 2s
        msend %actor% %self.name% says, 'There is a &6&bpackage&0 that someone could get back.'
      endif
      wait 2s
      msend %actor% %self.name% says, 'You can check your &6&b[subclass progress]&0 if you need.'
    else
      msend %actor% %self.name% says, 'You have to pick one first!'
      wait 2s
      tap
      wait 1s
      msend %actor% %self.name% says, 'Well which would you like to train as: &3mercenary&0, &1assassin&0, or &1&bthief&0?'
      wait 2s
      msend %actor% %self.name% says, 'Say one of them.'
    endif
  elseif %speech% /= no
    msend %actor% %self.name% says, 'Well then go away.'
    emote points north.
    wait 2s
    msend %actor% %self.name% says, 'I said go.'
    emote pushes %actor.name% away.
    open fence
    mforce %actor.name% north
    close fence
  endif
endif
unset use_subclass
~
#6011
quest_timulos_merc_lord~
0 d 100
lord lord? who who? pay pay?~
wait 2
if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == mercenary && %actor.quest_stage[merc_ass_thi_subclass]% == 1
  quest advance merc_ass_thi_subclass %actor.name%
  msend %actor% %self.name% says, 'Well, a great Lord, who shall remain unnamed, has lost a cloak.'
  smirk
  wait 2s
  msend %actor% %self.name% says, 'He has come to me for its return.  If you went and procured it, he would be grateful.'
  grin
  wait 2
  msend %actor% %self.name% says, 'And if he is grateful, I would be as well, and your training would be finished.  It would be quite a payday for a &6&bcloak&0.'
endif
~
#6012
quest_timulos_ass_price~
0 d 100
price? price~
wait 2
if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == assassin && %actor.quest_stage[merc_ass_thi_subclass]% == 1
  quest advance merc_ass_thi_subclass %actor.name%
  msend %actor% %self.name% says, 'Yes, a great price.'
  grin
  wait 1s
  msend %actor% %self.name% says, 'I have some rich men unhappy with the politics of the region in question.  You could help with those &6&bpolitics&0 if you wish.'
endif
~
#6013
quest_timulos_thi_package~
0 d 100
package? package~
wait 2
if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == thief && %actor.quest_stage[merc_ass_thi_subclass]% == 1
  quest advance merc_ass_thi_subclass %actor.name%
  msend %actor% %self.name% says, 'Yes a package.'
  fume
  wait 2s
  msend %actor% %self.name% says, 'Some time ago it was sent and picked up by someone who should not have.'
  grumble
  wait 2s
  msend %actor% %self.name% says, 'Bloody &6&bfarmers&0.'
endif
~
#6014
quest_timulos_merc_cloak~
0 d 100
cloak~
wait 2
if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == mercenary && %actor.quest_stage[merc_ass_thi_subclass]% == 2
  quest advance merc_ass_thi_subclass %actor.name%
  msend %actor% %self.name% says, 'Well yes, this cloak is worth much to him.'
  wait 1
  msend %actor% %self.name% says, 'It was made off with in a raid on his castle by some bothersome insect warriors.'
  mutter
  wait 3s
  msend %actor% %self.name% says, 'All the Lord was able to tell me is they said something about wanting it for their queen.'
  shrug
  wait 2s
  msend %actor% %self.name% says, 'I think you should go find it now.  Come back when you have the &3&bcloak&0, or do not come back at all.'
  open fence
  emote pushes %actor.name% away.
  mforce %actor.name% north
  close fence
endif
~
#6015
quest_timulos_ass_politics~
0 d 100
politics? politics~
wait 2
if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == assassin && %actor.quest_stage[merc_ass_thi_subclass]% == 2
  quest advance merc_ass_thi_subclass %actor.name%
  msend %actor% %self.name% says, 'Ah yes, the politics of it all.  Personally I am not one for them, but some people get all mixed up in those.'
  wait 2
  consider %actor.name%
  msend %actor% %self.name% says, 'Well, you seem fit, I guess.'
  wait 1
  msend %actor% %self.name% says, 'Go kill the Mayor of Mielikki.  He's probably holed up in his office in City Hall.'
  wait 1s
  msend %actor% %self.name% says, 'You'll have to break in, sneak past the guards, and &1kill&0 him.  Get his &3&bcane&0 as proof and come back and give it to me.'
  wait 4s
  msend %actor% %self.name% says, 'It is worth much to me if he dies, so get to it.  It will be worth it for you as well.'
  grin %actor.name%
  wait 2s
  msend %actor% %self.name% says, 'Well, go on.'
  open fence
  emote pushes %actor.name% away.
  mforce %actor.name% north
  close fence
endif
~
#6016
quest_timulos_thi_farmers~
0 d 100
farmers?  farmer? farmer farmers~
wait 2
if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == thief && %actor.quest_stage[merc_ass_thi_subclass]% == 2
  quest advance merc_ass_thi_subclass %actor.name%
  msend %actor% %self.name% says, 'That is right, a &3&bpackage&0 was taken by a farmer who should not have it.'
  grumble
  wait 2
  msend %actor% %self.name% says, 'I know this: he got it from the post office in Mielikki and he lives near there.'
  wait 4s
  msend %actor% %self.name% says, 'Go get it back and I will make it worth it to you.'
  wait 2s
  msend %actor% %self.name% says, 'Do not let anyone see you and do not leave a trail of bodies behind you.'
  wait 2s
  msend %actor% %self.name% says, 'And be careful!  If you jostle the package too much it just might explode.'
  wait 2
  msend %actor% %self.name% says, 'For now though, we are done.  Begone.'
  wait 2s
  open fence
  msend %actor% %self.name% says, 'Well, go on.'
  emote pushes %actor.name% harshly away.
  mforce %actor.name% north
  close fence
endif
~
#6018
quest_obj_cloak(41014)~
1 g 100
~
if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == mercenary && %actor.quest_stage[merc_ass_thi_subclass]% == 3
  quest advance merc_ass_thi_subclass %actor.name%
  osend %actor% You've located the cloak!
endif
~
#6019
quest_mayor_obj_cane(3033)~
1 g 100
~
if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == assassin
  if %actor.quest_stage[merc_ass_thi_subclass]% == 3
    if %actor.quest_variable[merc_ass_thi_subclass:mayor]% == 1
      quest advance merc_ass_thi_subclass %actor.name%
      wait 1
      osend %actor% &1&bYou've got the cane!&0
    else
      wait 1
      osend %actor% &3&bYou haven't killed the Mayor yet...&0
    endif
  endif
endif
~
#6020
quest_obj_package(8813)~
1 g 100
~
if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == thief && %actor.quest_stage[merc_ass_thi_subclass]% == 3
  if %self.room% != 8828
    return 0
    oechoaround %actor% %self.shortdesc% suddenly bursts into flames!
    osend %actor% %self.shortdesc% suddenly bursts into flames from being handled too much!
    opurge %self%
  else
    quest advance merc_ass_thi_subclass %actor.name%
    osend %actor% &9&bYou've secured the package!
    osend %actor% Now get out carefully!&0
  endif
endif
~
#6021
merc_subclass_timulos_receive~
0 j 100
41014~
wait 2
switch %actor.quest_stage[merc_ass_thi_subclass]%
  case 1
  case 2
    msend %actor% %self.name% says, 'Interesting, I have not even told you what to do yet...  So how did you do it?'
    break
  case 3
    msend %actor% %self.name% says, 'How ever did you get what I ask for without getting it yourself?'
    spank %actor.name%
    break
  case 4
    if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == mercenary
      emote grins from ear to ear.
      msend %actor% %self.name% says, 'You have done me a great service.  The lord will reward me kindly to get this back.'
      flex
      emote shouts, 'Yes, yes, yes!!!'
      wait 2
      msend %actor% %self.name% says, 'Type '&3subclass&0' to proceed.'
      quest complete merc_ass_thi_subclass %actor.name%
    endif
    break
  default
    msend %actor% %self.name% says, 'Well this is nifty, I think I will hold on to that, thank you.'
    wait 2
    msend %actor% %self.name% says, 'Shame you were not on a quest for me.'
done
mjunk questobject
~
#6022
assassin_subclass_timulos_receive~
0 j 100
3033~
wait 2
switch %actor.quest_stage[merc_ass_thi_subclass]%
  case 1
  case 2
    msend %actor% %self.name% says, 'Interesting, I have not even told you what to do yet...  So how did you do it?'
    break
  case 3
    msend %actor% %self.name% says, 'How ever did you get what I ask for without getting it yourself?'
    spank %actor.name%
    break
  case 4
    if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == assassin
      emote smiles widely.
      msend %actor% %self.name% says, 'Wonderful, now they shall reward me, excellent.'
      wait 1s 
      msend %actor% %self.name% says, 'You have done well by me, so in return your request is granted.'
      wait 2
      msend %actor% %self.name% says, 'Type '&1subclass&0' to proceed.'
      quest complete merc_ass_thi_subclass %actor.name%
    endif
    break
  default
    msend %actor% %self.name% says, 'Well this is nifty, I think I will hold on to that, thank you.'
    wait 2
    msend %actor% %self.name% says, 'Shame you were not on a quest for me.'
done
mjunk questobject
~
#6023
thief_subclass_timulos_receive~
0 j 100
8813~
wait 2
switch %actor.quest_stage[merc_ass_thi_subclass]%
  case 1
  case 2
    msend %actor% %self.name% says, 'Interesting, I have not even told you what to do yet...  So how did you do it?'
    break
  case 3
    msend %actor% %self.name% says, 'How ever did you get what I ask for without getting it yourself?'
    spank %actor.name%
    break
  case 4
    if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == thief
      msend %actor% %self.name% says, 'Wonderful, now I can return this to its rightful owner.'
      wait 2
      emote mutters something about how a true thief would try to take it back.
      emote clears his throat and says, 'You have done well, congratulations!'
      wait 2
      msend %actor% %self.name% says, 'Type '&1&bsubclass&0' to proceed.'
      quest complete merc_ass_thi_subclass %actor.name%
    endif
    break
  default
    msend %actor% %self.name% says, 'Well this is nifty, I think I will hold on to that, thank you.'
    wait 2
    msend %actor% %self.name% says, 'Shame you were not on a quest for me.'
  done
mjunk questobject
~
#6024
Timulos refuse~
0 j 0
41014 3033 8813~
switch %object.vnum%
  default
    if %actor.quest_stage[merc_ass_thi_subclass]% == 4
      wait 2
      msend %actor% %self.name% says, 'Well this is nifty, I think I will hold on to that, thank you.  Now go bring me what you were told.'
      mjunk %object%
    else
      set response What is this? Are you trying to trick me?
    endif
done
if %response%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  eye %actor.name%
  msend %actor% %self.name% says, '%response%'
endif
~
#6025
quest_timulos_status~
0 d 100
subclass progress~
wait 2
set quest_name %actor.quest_variable[merc_ass_thi_subclass:subclass_name]%
switch %actor.quest_stage[merc_ass_thi_subclass]%
  case 1
    if %quest_name% == mercenary
      msend %actor% %self.name% says, 'Yes, a mercenary would serve well for that...'
      emote thinks back for a moment.
      wait 2s
      msend %actor% %self.name% says, 'He would pay well.  Yes, that &6&bLord&0 would pay well indeed.'
    elseif %quest_name% == assassin
      msend %actor% %self.name% says, 'An assassin would be perfect, but to show your true desire...'
      emote grins deeply, curling his lip up.
      wait 2s
      msend %actor% %self.name% says, 'Yes, that would bring a good &6&bprice&0.'
    elseif %quest_name% == thief
      msend %actor% %self.name% says, 'Hmmm, a true thief would have to get something I think.'
      emote smiles cruelly.
      wait 2s
      msend %actor% %self.name% says, 'There is a &6&bpackage&0 that someone could get back.'
    endif
    break
  case 2
    if %quest_name% == mercenary
      msend %actor% %self.name% says, 'Well, a great Lord, who shall remain unnamed, has lost a cloak.'
      Smirk
      wait 2s
      msend %actor% %self.name% says, 'He has come to me for its return.  If you went and procured it, he would be grateful.'
      grin
      wait 2s
      msend %actor% %self.name% says, 'And if he is grateful, I would be as well, and your training would be finished.  It would be quite a payday for a &6&bcloak&0.'
    elseif %quest_name% == thief
      msend %actor% %self.name% says, 'Yes a package.'
      fume
      wait 1s
      msend %actor% %self.name% says, 'Some time ago it was sent and picked up by someone who should not have it.'
      grumble
      wait 3s
      msend %actor% %self.name% says, 'Bloody &6&bfarmers&0.'
    elseif %quest_name% == assassin
      msend %actor% %self.name% says, 'Yes, a great price.'
      grin
      wait 1s
      msend %actor% %self.name% says, 'I have some rich men unhappy with the politics of the region in question.  You could help with those &6&bpolitics&0 if you wish.'
    endif
    break
  case 3
  case 4
    if %quest_name% == mercenary
      msend %actor% %self.name% says, 'Well yes, this cloak is worth much to him.'
      wait 1
      msend %actor% %self.name% says, 'It was made off with in a raid on his castle by some bothersome insect warriors.'
      mutter
      wait 3s
      msend %actor% %self.name% says, 'All the Lord was able to tell me is they said something about wanting it for their queen.'
      shrug
      wait 2s
      msend %actor% %self.name% says, 'I think you should go find it now.  Come back when you have the &3&bcloak&0, or do not come back at all.'
      open fence
      emote pushes %actor.name% away.
      mforce %actor.name% north
      close fence
    elseif %quest_name% == assassin
      msend %actor% %self.name% says, 'Ah yes, the politics of it all.  Personally I am not one for them, but some people get all mixed up in those.'
      wait 2
      consider %actor.name%
      msend %actor% %self.name% says, 'Well, you seem fit, I guess.'
      wait 1
      msend %actor% %self.name% says, 'Go kill the Mayor of Mielikki.  He's probably holed up in his office in City Hall.'
      wait 1s
      msend %actor% %self.name% says, 'You'll have to break in, sneak past the guards, and &1kill&0 him.  Get his &3&bcane&0 as proof and come back and give it to me.'    
      wait 4s
      msend %actor% %self.name% says, 'It is worth much to me if he dies, so get to it.  It will be worth it for you as well.'
      wait 2s
      msend %actor% %self.name% says, 'Well, go on.'
      open fence
      emote pushes %actor.name% away.
      mforce %actor.name% north
      close fence
    elseif %quest_name% == thief
      msend %actor% %self.name% says, 'That is right, a &3&bpackage&0 was taken by a farmer who should not have it.'
      grumble
      wait 2
      msend %actor% %self.name% says, 'I know this: he got it from the post office in Mielikki and he lives near there.  Go get it back and I will make it worth it to you.'
      wait 2s
      msend %actor% %self.name% says, 'Do not let anyone see you and do not leave a trail of bodies behind you.'
      wait 2s
      msend %actor% %self.name% says, 'And be careful!  If you jostle the package too much it just might explode.'    
      wait 2
      msend %actor% %self.name% says, 'For now though, we are done, begone.'
      wait 2s
      open fence
      msend %actor% %self.name% says, 'Well, go on.'
      emote pushes %actor.name% harshly away.
      mforce %actor.name% north
      close fence
    endif
    break
  default
    if %actor.class% /= Rogue
      if %actor.level% >= 10 && %actor.level% <= 25
        msend %actor% %self.name% says, 'You are not on any quests from me.'
      elseif %actor.level% < 10
        msend %actor% %self.name% says, 'I like your zeal, but it's a little too soon for you to subclass kid.'
      else
        msend %actor% %self.name% says, 'It's waaaaaaay too late to train you.  That ship has sailed!'
      endif
    else
      msend %actor% %self.name% says, 'I don't train "your type."  Now get lost.'
    endif
done
~
#6026
thief_subclass_package_give~
1 i 100
~
if %victim.quest_variable[merc_ass_thi_subclass:subclass_name]% == thief && %victim.quest_stage[merc_ass_thi_subclass]% == 3
  if %self.room% != 8828
    return 0
    oechoaround %actor% %self.shortdesc% suddenly bursts into flames!
    osend %actor% %self.shortdesc% suddenly bursts into flames from being handled too much!
    opurge %self%
  else
    quest advance merc_ass_thi_subclass %victim.name%
    osend %victim% You've secured the package!
  endif
endif
~
#6027
thief_subclass_sneak_past~
0 b 100
~
if %self.fighting%
  halt
endif
set room %self.room%
set person %room.people%
while %person%
  if %person.quest_variable[merc_ass_thi_subclass:subclass_name]% == thief
    if %person.quest_stage[merc_ass_thi_subclass]% == 3 || %person.quest_stage[merc_ass_thi_subclass]% == 4
      if %person.can_be_seen% && %person.hiddenness% < 1
        msend %person% %self.name% notices you skulking about!
        msend %person% %self.name% says, 'Who are you?!  You weren't invited here!'
        mechoaround %person% %self.name% shoos %person.name% off the farm!
        msend %person% %self.name% shoos you off the farm!
        mteleport %person% 8006
        wait 1
        mforce %person% look
        quest fail merc_ass_thi_subclass %actor%
        msend %actor% &3&bYou have failed your quest!&0
        msend %actor% You'll have to go back to %get.mob_shortdesc[6050]% and start over!
      endif
    endif
  endif
  set person %person.next_in_room%
done
~
#6028
thief_subclass_farmer_death~
0 f 100
~
if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == thief
  if %actor.quest_stage[merc_ass_thi_subclass]% == 3 || %actor.quest_stage[merc_ass_thi_subclass]% == 4
    if %self.vnum% == 8804
      mecho The farmer's wife comes running!
      mecho The farmer's wife cries out, 'Sweet Mielikki, what have you done?!'
      mechoaround %actor% The farmer's wife chases %actor.name% off the farm!
      msend %actor%  
      msend %actor% The farmer's wife throws herself at you!
      msend %actor% The farmer's wife screams, 'Get out!  Get out!' while chasing you off the farm!
      mteleport %actor% 8006
      mat 8006 mforce %actor% look
    else
      mecho The farmer comes running!
      mecho The farmer cries out, 'Sweet Mielikki, what have you done?!'
      mechoaround %actor% The farmer chases %actor.name% off the farm!
      msend %actor%  
      msend %actor% The farmer throws himself at you!
      msend %actor% The farmer hollers, 'Get out!  Get out!' while chasing you off the farm!
      mteleport %actor% 8006
      mforce %actor% look
    endif
    quest fail merc_ass_thi_subclass %actor%
    msend %actor% &3&bYou have failed your quest!&0
    msend %actor% You'll have to go back to %get.mob_shortdesc[6050]% and start over!
  endif
endif
~
#6029
assassin_subclass_command_pick~
0 c 100
pick~
if %self.room% == 3062
  switch %arg%
    case d
    case do
    case doo
    case door
      msend %actor% You begin to pick the lock when %self.name% interrupts you.
      mecho %self.name% says, 'Hey!  You there!  What are you doing?!'
      mechoaround %actor% %self.name% shoves %actor.name% away from the door.
      msend %actor% %self.name% shoves you away from the door.
      break
    default
      return 0
  done
else
  return 0
endif
~
#6030
assassin_subclass_guards_can_see~
0 c 100
up~
if %actor.can_be_seen% && %actor.hiddenness% < 1
  msend %actor% %self.name% cuts you off from the stairs.
  msend %actor% %self.name% asks you, 'Do you have an appointment?'
  mechoaround %actor% %self.name% stands in %actor.name%'s way.
else
  return 0
endif
~
#6031
assassin_subclass_mayor_calls_for_help~
0 k 100
~
wait 2
switch %round%
  case 1
    mecho %self.name% cries out in surprise!
    mecho %self.name% shouts, 'Help!  Help!!  Someone is trying to kill me!!'
    break
  case 3
    mecho A commotion echoes from downstairs!
    mecho The guards are coming to the Mayor's rescue!
    wait 2
    mecho Quick footsteps begin to approach the room!
    break
  case 5
    mecho Someone shouts, 'Quickly, rescue the Mayor!!'
    wait 2
    mecho The footsteps speed up!
    break
  case 8
    mecho %self.name% gurgles, 'Someone help!!!'
    wait 2
    mecho The sound of footsteps gets louder!
    break
  case 10
    mecho %self.name% gurgles, 'Hurry!  I'm dying!!!'
    wait 2
    mecho Someone shouts, 'I'm coming Mr. Mayor!'
    mecho The footsteps are almost here!
    break
  case 13
    mecho The footsteps are right outside the door!
    break
  case 15
    mecho One of the City Hall guards bursts into the room!
    mecho The City Hall guard cries, 'I'll save you Mr. Mayor!!'
    mechoaround %actor% The City Hall guard leaps on %actor.name% and drags %actor.himher% from the building!
    msend %actor% The City Hall guard throws himself on you and breaks up the fight.
    msend %actor% You are dragged out of the building and thrown out of town!
    *teleport the player anywhere in the farmlands other than the fields*
    eval place 8000 + %random.225%
    mteleport %actor% %place%
    wait 2
    mforce %actor% look
    wait 2
    msend %actor% The City Hall guard tells you, 'And don't you come back!'
    unset round
    quest fail merc_ass_thi_subclass %actor%
    msend %actor% &3&bYou have failed your quest!&0
    msend %actor% You'll have to go back to %get.mob_shortdesc[6050]% and start over!
    halt
done
eval round %round% + 1
global round
~
#6032
assassin_subclass_mayor_death~
0 f 100
~
set i %actor.group_size%
if %i%
  set a 1
  while %i% >= %a%
    set person %actor.group_member[%a%]%
    if %person.room% == %self.room%
      if %person.quest_variable[merc_ass_thi_subclass:subclass_name]% == assassin && %person.quest_stage[merc_ass_thi_subclass]% == 3
        quest variable merc_ass_thi_subclass %person.name% mayor 1
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
elseif %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == assassin && %actor.quest_stage[merc_ass_thi_subclass]% == 3
  quest variable merc_ass_thi_subclass %actor.name% mayor 1
endif
~
#6033
Green Woman refuse~
0 j 100
~
switch %object.vnum%
  default
    if %actor.quest_stage[wizard_eye]% == 7
      switch %object.vnum%
        case 23754
        case 3298
        case 23847
        case 18001
          halt
          break
        defalt
          set response This %get.obj_noadesc[%object.vnum%]% isn't roses or cinnamon.
      done
    else
      set response I only take coin, not trade.
    endif
done
if %response%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, '%response%'
endif
~
#6034
random gem for broker~
0 g 100
100~
* number of gems to load -- statring at 3, but that might be too many
set loop 3
* gem vnums go from  to 55566-55751
* p1 vnums from 55566-55593
* p2 vnums from 55594-55670
* P3 cnums from 55671-55747 (there are gems up to 55751, but not used.
* random # -- 1-10 to create probabilites of good gem
* 0 = NO GEM
* 1 = NO GEM
* 2-6 = P1 Gem
* 7-9 = P2 Gem
* 10  = P3 Gem
* -- lets see if we should run process to get gems 
* -- we do that by looking for object 18701 -- if we are wearing it
* -- then we don't need to load gems again
* all the important stuff incased in this loop
if !%self.wearing[18701]%
     mload obj 18701
     mat 1100 wear lock
     set itt 1
     while %itt% <= %loop%
     set p %random.10%
         if %p% == 10
          *say p3! %p%
             set base 55671
             set extra %random.76%
         endif
         *p2 gem
         if (%p% <=9) && (%p%>=7)
          *say p2 %p%555
             set base 55594
             set extra %random.76%
         endif
         *p1 gem
         if (%p% <=6) && (%p%>=2)
         *say p1 %p%
             set base 55566
             set extra %random.27%
         endif
     *no gem
         if (%p% < 2)
         *say no gem - %p%
             set base 0
          set extra 0
         endif
         if %base% > 55560
             eval gem %base% + %extra%
     mload obj %gem%
 sell gem broker
         endif
         eval itt %itt% + 1
     done
endif
~
#6050
Berix bounty hunt greet~
0 h 100
~
wait 2
if !%actor.has_completed[bounty_hunt]%
  if !%actor.quest_stage[bounty_hunt]%
    if %actor.align% =< -350 || %actor.class% /= Assassin || %actor.class% /= Mercenary
      msend %actor% %self.name% says, 'Well hello there.  You look like quite a piece of work.  I think I have a &6&b[job]&0 for you.'
    else
      msend %actor% %self.name% says, 'What do you want?'
    endif
  elseif %actor.quest_variable[bounty_hunt:bounty]% == running
    msend %actor% %self.name% says, 'You're still on the hunt.  What are you doing here?  If you lost your contract like a moron, say, &6&b"I need a new contract"&0.'
  elseif %actor.quest_variable[bounty_hunt:bounty]% == dead
    msend %actor% %self.name% says, 'If you got the job done, give me your contract.  If you lost your contract like a moron, say, &6&b"I need a new contract"&0.'
  elseif %actor.quest_stage[bounty_hunt]% >= 1 && !%actor.has_completed[bounty_hunt]%
    msend %actor% %self.name% says, 'Ah, back for another &6&b[job]&0 I see.'
  endif
  if !%actor.has_completed[assassin_mask]% && %actor.level% > 9 && %actor.class% /= Assassin
    wait 1s
    msend %actor% %self.name% says, 'Or maybe you're here to discuss a &6&b[promotion]&0.'
  endif
else
  if %actor.class% /= Assassin && !%actor.quest_stage[assassin_mask]% && %actor.level% > 9
    msend %actor% %self.name% says, 'Oh look, someone else in line for a &6&b[promotion]&0.'
  elseif %actor.quest_stage[assassin_mask]% && !%actor.has_completed[assassin_mask]%
    msend %actor% %self.name% says, 'Ah, you must be looking for another &6&b[promotion]&0.'
  endif
endif
~
#6051
Berix bounty hunt speech~
0 d 100
job jobs what contract contracts~
wait 2
if %actor.has_completed[bounty_hunt]%
  msend %actor% %self.name% says, 'You know, I'm fresh out of work for you.  Good luck!'
elseif !%actor.quest_stage[bounty_hunt]% || (%actor.quest_stage[bounty_hunt]% == 1 && !%actor.quest_variable[bounty_hunt:bounty]%)
  msend %actor% %self.name% says, 'Sure, I have something easy to start with.'
  wait 2
  msend %actor% %self.name% says, 'I have benefactors who would like certain individuals to... disappear.  The resulting chaos is to their benefit.  They're willing to pay handsomely for that to occur.  Ours is not ask questions but simply see that it happen.'
  wait 4s
  msend %actor% %self.name% says, 'One such individual is the king of some cat colony or somme such down near the town of Mielikki.  I don't know why they say he's "merely" a king, but again, our job isn't to ask questions.'
  wait 4s
  msend %actor% %self.name% says, 'Can I count on you to get this done?'
elseif %actor.quest_variable[bounty_hunt:bounty]% == dead
  msend %actor% %self.name% says, 'Give me your current contract first.'
  halt
else
  if %actor.level% >= (%actor.quest_stage[bounty_hunt]% - 1) * 10
    switch %actor.quest_stage[bounty_hunt]%
      case 1
        if %actor.quest_variable[bounty_hunt:bounty]% == running
          msend %actor% %self.name% says, 'You still have a job to do.  Best get on killing that cat-king or whatever first.'
        endif
        break
      case 2
        if %actor.quest_variable[bounty_hunt:bounty]% == running
          msend %actor% %self.name% says, 'You still have a job to do.  Take out the Noble and the Abbot sheltering him at the Abbey of St. George.'
        else
          msend %actor% %self.name% says, 'There's a noble who's gone into hiding because a lot of people want him dead.  But good news: I've been able to locate him!  He's hiding in the Abbey of St. George.'
          wait 4s
          msend %actor% %self.name% says, 'I've got someone who's so angry, they're asking us to kill not just the noble, but the Abbot who took him in!  Double payday!'
          wait 3s
          msend %actor% %self.name% says, 'You in?'
        endif
        break
      case 3
      if %actor.quest_variable[bounty_hunt:bounty]% == running
          msend %actor% %self.name% says, 'You still have a job to do.  Take out the three Chieftains in the southwestern Highlands.'
        else
          msend %actor% %self.name% says, 'There are three warring clans down in the Highlands past the Gothra Desert: O'Connor, McLeod, and Cameron.  Each has taken out a contract on the other, but get this: they all paid in advance!'
          wait 4s
          msend %actor% %self.name% says, 'It's a big job, but it'll be totally worth it if we pull it off.  And it'll be a legendary triple-cross!  Think of how impressive that'll be!'
          wait 3s
          msend %actor% %self.name% says, 'So whadda ya say?'
        endif
        break
      case 4
        if %actor.quest_variable[bounty_hunt:bounty]% == running
          msend %actor% %self.name% says, 'You still have a job to do.  Find the Frakati Leader and kill him.'
        else
          msend %actor% %self.name% says, 'I've got a challenge for you.  There's a hidden reservation near the town of Mielikki for a group of tribal hunters.'
          wait 4s
          msend %actor% %self.name% says, 'I have a contract for their leader.  Elusive bastard, but could be an interesting job.  Plus you can keep whatever you find.'
          wait 3s
          msend %actor% %self.name% says, 'You interested?'
        endif
        break
      case 5
        if %actor.quest_variable[bounty_hunt:bounty]% == running
          msend %actor% %self.name% says, 'You still have a job to do.  Infiltrate the Sacred Haven and take out the number two in command, Cyrus.'
        else
          msend %actor% %self.name% says, 'I got a big contract this time.  Someone from Ogakh is trying to tip the scales in South Caelia by destablizing the Sacred Haven.  They want the head of Cyrus, the number two in command.'
          wait 4s
          msend %actor% %self.name% says, 'Only problem is the Sacred Haven is a literal fortress.  Wall to wall paladins with platemail and holy swords, the whole nine yards.  Getting to Cyrus could be a mission all on its own.'
          wait 3s
          msend %actor% %self.name% says, 'Are you up for it?'
        endif
        break
      case 6
        if %actor.quest_variable[bounty_hunt:bounty]% == running
          msend %actor% %self.name% says, 'You still have a job to do.  Disappear Lord Venth down south.'
        else
          msend %actor% %self.name% says, 'With the success of the Sacred Haven mission, someone is feeling bold.  They're looking for Lord Venth in the Tolder Borderhold Keep to disappear.'
          wait 4s
          msend %actor% %self.name% says, 'The borderhold isn't too hard to get into, but Venth is a real tough character.  Be ready for a knockdown drag out fight.'
          wait 3s
          msend %actor% %self.name% says, 'What do you think?  Do you want it?'
        endif
        break
      case 7
        if %actor.quest_variable[bounty_hunt:bounty]% == running
          msend %actor% %self.name% says, 'You still have a job to do.  Send the high druid on a permanent pilgrimage.'
        else
          msend %actor% %self.name% says, 'The South Caelia jobs just keep coming.  Another religious leader for you.  One of the most influential druids in the world makes his home in Anlun Vale.'
          wait 4s
          msend %actor% %self.name% says, 'Making him vanish will be a warning to all the other sects to keep clear of worldly affairs.'
          wait 3s
          msend %actor% %self.name% says, 'Sound like something you could be interested in?'
        endif
        break
      case 8
        if %actor.quest_variable[bounty_hunt:bounty]% == running
          msend %actor% %self.name% says, 'You still have a job to do.  Kill the Lizard King.  If you can even find him...'
        else
          msend %actor% %self.name% says, 'Really odd request this next one.'
          wait 2s
          msend %actor% %self.name% says, 'The Northern Swamps seem to be under the domain of a king of the lizard men.  Recon suggests he makes his home in a castle that sunk into the the swamp ages ago.'
          wait 4s
          msend %actor% %self.name% says, 'I've got a contract to hunt down and kill the Lizard King.  Not sure what the reason could be, but it sounds like a thrillride anyway.'
          wait 3s
          msend %actor% %self.name% says, 'You down?'
        endif
        break
      case 9
        if %actor.quest_variable[bounty_hunt:bounty]% == running
          msend %actor% %self.name% says, 'You still have a job to do.  Take out the leader of the Ice Cult up north before they get wind of it.'
        else
          msend %actor% %self.name% says, 'Got a really difficult job for you next.  Up north is a very secretive, very deadly cult dedicated to worshipping some kinda dragon.  These are some extremely nasty customers, I mean the worst!'
          wait 4s
          msend %actor% %self.name% says, 'We've been asked to take out their high priestess, a woman by the name of Sorcha.  I'm sure she's well guarded, given the cult's resources, but that's why I need someone like you on the job.'
          wait 3s
          msend %actor% %self.name% says, 'Think you can handle it?'
        endif
        break
      case 10
        if %actor.quest_variable[bounty_hunt:bounty]% == running
          msend %actor% %self.name% says, 'You still have a job to do.  End the reign of the Goblin King.  We'll all probably dream a little more soundly then.'
        else
          msend %actor% %self.name% says, 'The last job I have is for a tyrant so terrible he can hardly be real.  We've been offered a contract for the Goblin King up in the Syric Mountains.'
          wait 4s
          msend %actor% %self.name% says, 'Some wild magic nonsense happened up there a few years ago so now the whole place is like a waking nightmare.  I understand it well enough to know I don't understand it!  'Fraid I can't be of much help to you.'
          wait 4s
          msend %actor% %self.name% says, 'You got this one?'
        endif
    done
  else
    msend %actor% %self.name% says, 'All my other jobs are too risky for someone without more experience.  Come back when you've seen a little more.'
  endif
endif
~
#6052
Berix bounty hunt speech2~
0 d 100
yes~
wait 2
set person %actor%
set i %person.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
    if !%person.quest_stage[bounty_hunt]%
      quest start bounty_hunt %person%
    endif
    if %person.has_completed[bounty_hunt]%
      msend %person% %self.name% says, 'You know, I'm fresh out of work for you.  Good luck!'
    elseif %person.quest_variable[bounty_hunt:bounty]% == dead
      msend %person% %self.name% says, 'Give me your current contract first.'
    else
      if %person.level% >= (%person.quest_stage[bounty_hunt]% - 1) * 10
        if %person.quest_variable[bounty_hunt:bounty]% != running
          msend %person% %self.name% says, 'Splendid.'
          switch %person.quest_stage[bounty_hunt]%
            case 1
              set contract 6050
              break
            case 2
              set contract 6051
              break
            case 3
              set contract 6052
              break
            case 4
              set contract 6053
              break
            case 5
              set contract 6054
              break
            case 6
              set contract 6055
              break
            case 7
              set contract 6056
              break
            case 8
              set contract 6057
              break
            case 9
              set contract 6058
              break
            case 10
              set contract 6059
              break
            default
              set contract 6050
          done
          mload obj %contract%
          give contract %person%
          set accept yes
          msend %person% &0  
          msend %person% %self.name% says, 'When you've completed your task, bring that contract back to me.  I'll reward you then.'
          msend %person% &0  
          msend %person% %self.name% says, 'Keep a low profile and good luck.'
          quest variable bounty_hunt %person% bounty running
        else
          switch %person.quest_stage[bounty_hunt]%
            case 1
              msend %person% %self.name% says, 'You still have a job to do.  Best get on killing that cat-king or whatever first.'
              break
            case 2
              msend %person% %self.name% says, 'You still have a job to do.  Take out the Noble and the Abbot  sheltering him at the Abbey of St. George.'
              break
            case 3
              msend %person% %self.name% says, 'You still have a job to do.  Take out the three Chieftains in the southwestern Highlands.'
              break
            case 4
              msend %person% %self.name% says, 'You still have a job to do.  Find the Frakati Leader and kill him.'
              break
            case 5
              msend %person% %self.name% says, 'You still have a job to do.  Infiltrate the Sacred Haven and take out the number two in command, Cyrus.'
              break
            case 6
              msend %person% %self.name% says, 'You still have a job to do.  Disappear Lord Venth down south.'
              break
            case 7
              msend %person% %self.name% says, 'You still have a job to do.  Send the high druid on a permanent pilgrimage.'
              break
            case 8
              msend %person% %self.name% says, 'You still have a job to do.  Kill the Lizard King.  If you can even find him...'
              break
            case 9
              msend %person% %self.name% says, 'You still have a job to do.  Take out the leader of the Ice Cult up north before they get wind of it.'
              break
            case 10
              msend %person% %self.name% says, 'You still have a job to do.  End the reign of the Goblin King.  We'll all probably dream a little more soundly then.'
          done
        endif
      else
        msend %person% %self.name% says, 'All my other jobs are too risky for someone without more experience.  Come back when you've seen a little more.'
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#6053
Berix bounty hunt receive~
0 j 100
~
switch %object.vnum%
  * bounty contracts here
  case 6050
    set stage 1
    set victim1 The King of the Meer Cats
    set go hunt
    break
  case 6051
    set stage 2
    set victim1 the Noble
    set victim2 the Abbot
    set go hunt
    break
  case 6052
    set stage 3
    set victim1 the O'Connor Chieftain
    set victim2 the McLeod Chieftain
    set victim3 the Cameron Chieftain
    set go hunt
    break
  case 6053
    set stage 4
    set victim1 The Frakati Leader
    set go hunt
    break
  case 6054
    set stage 5
    set victim1 Cyrus
    set go hunt
    break
  case 6055
    set stage 6
    set victim1 Lord Venth
    set go hunt
    break
  case 6056
    set stage 7
    set victim1 The high druid of Anlun Vale
    set go hunt
    break
  case 6057
    set stage 8
    set victim1 The Lizard King
    set go hunt
    break
  case 6058
    set stage 9
    set victim1 Sorcha
    set go hunt
    break
  case 6059
    set stage 10
    set victim1 The Goblin King
    set go hunt
    break
  * assassin mask items start here
  case 350
    set maskstage 1
    set item quest
    set go mask
    break
  case 4500
    set maskstage 1
    set item mask
    set go mask
    break
  case 55592
    set maskstage 1
    set item gem
    set go mask
    break
  case 351
    set maskstage 2
    set item quest
    set go mask
    break
  case 17809
    set maskstage 2
    set item mask
    set go mask
    break
  case 55594
    set maskstage 2
    set item gem
    set go mask
    break
  case 352
    set maskstage 3
    set item quest
    set go mask
    break
  case 59023
    set maskstage 3
    set item mask
    set go mask
    break
  case 55620
    set maskstage 3
    set item gem
    set go mask
    break
  case 353
    set maskstage 4
    set item quest
    set go mask
    break
  case 10304
    set maskstage 4
    set item mask
    set go mask
    break
  case 55638
    set maskstage 4
    set item gem
    set go mask
    break
  case 354
    set maskstage 5
    set item quest
    set go mask
    break
  case 16200
    set maskstage 5
    set item mask
    set go mask
    break
  case 55666
    set maskstage 5
    set item gem
    set go mask
    break
  case 355
    set maskstage 6
    set item quest
    set go mask
    break
  case 43017
    set maskstage 6
    set item mask
    set go mask
    break  
  case 55675
    set maskstage 6
    set item gem
    set go mask
    break
  case 356
    set maskstage 7
    set item quest
    set go mask
    break
  case 51075
    set maskstage 7
    set item mask
    set go mask
    break
  case 55693
    set maskstage 7
    set item gem
    set go mask
    break
  case 357
    set maskstage 8
    set item quest
    set go mask
    break
  case 49062
    set maskstage 8
    set item mask
    set go mask
    break
  case 55719
    set maskstage 8
    set item gem
    set go mask
    break
  case 358
    set maskstage 9
    set item quest
    set go mask
    break
  case 48427
    set maskstage 9
    set item mask
    set go mask
    break
  case 55743
    set maskstage 9
    set item gem
    set go mask
    break
  default
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    say What is this for?
    halt
done
if %go% == hunt
  if %actor.quest_stage[bounty_hunt]% == %stage% && %actor.quest_variable[bounty_hunt:bounty]% == dead
    wait 2
    mjunk %object%
    nod
    msend %actor% %self.name% says, 'Well done.  Here's your payment.'
    eval money %stage% * 10
    give %money% platinum %actor%
    if %stage% == 1
      set expcap 5
    else
      eval bonus (%stage% - 1) * 10
      set expcap %bonus%
    endif
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
    switch %person.class%
      case Warrior
      case Berserker 
          eval expmod (%expmod% + (%expmod% / 10))
          break
      case Paladin
      case Anti-Paladin
      case Ranger
          eval expmod (%expmod% + ((%expmod% * 2) / 15)
          break
      case Sorcerer
      case Pyromancer
      case Cryomancer
      case Illusionist
      case Bard
          eval expmod (%expmod% + (%expmod% / 5))
          break
      case Necromancer
      case Monk
          eval expmod (%expmod% + (%expmod% * 2) / 5)
          break
      default
          set expmod %expmod%
    done
    msend %actor% &3&bYou gain experience!&0
    eval setexp (%expmod% * 10)
    set loop 0
    while %loop% < 3
      mexp %actor% %setexp%
      eval loop %loop% + 1
    done
    quest variable bounty_hunt %actor% target1 0
    quest variable bounty_hunt %actor% target2 0
    quest variable bounty_hunt %actor% target3 0
    quest variable bounty_hunt %actor% bounty 0
    wait 2
    if %stage% < 10
      quest advance bounty_hunt %actor%
      msend %actor% %self.name% says, 'Check in again if you have time for more work.'
    else
      quest complete bounty_hunt %actor%
      msend %actor% %self.name% says, 'Congratulations, you've really proven yourself out there!  'Fraid there ain't much more we can toss your way.'
      wait 1s
      msend %actor% %self.name% says, 'Take this to commemorate your hunt.'
      mload obj 443
      give shroud %actor%
    endif
    if %actor.class% /= Assassin && %actor.quest_stage[assassin_mask]% == 0
      wait 2s
      msend %actor% %self.name% says, 'I think you've earned this too.'
      mload obj 350
      give mask %actor%
      wait 1s
      msend %actor% %self.name% says, 'Masks like these show the rank of members of the Assassin Guild.'
      quest start assassin_mask %actor%
      wait 2s
      if %actor.level% > 9
        msend %actor% %self.name% says, 'This might be a good time to talk about a &6&b[promotion]&0.'
      else
        msend %actor% %self.name% says, 'Come back with that mask after you reach level 10 and let's talk about a promotion.'
      endif
    endif
  elseif %actor.quest_stage[bounty_hunt]% > %stage%
    return 0
    shake
    mecho %self.name% refuses the contract.
    wait 2
    msend %actor% %self.name% says, 'You already completed this contract!'
  elseif %actor.quest_stage[bounty_hunt]% < %stage%
    wait 2
    eye %actor%
    msend %actor% %self.name% says, 'How'd you get this?!  You steal it off someone else??'
    mecho %self.name% rips up the contract!
    mjunk %object%
  elseif %actor.quest_variable[bounty_hunt:bounty]% != dead
    return 0
    mecho %self.name% refuses the contract.
    wait 2
    msend %actor% %self.name% says, 'You have to finish the job first!'
    if %stage% != 2 && %stage% != 3
      msend %actor% %victim1% is still out there.
    elseif %stage% == 2 || %stage% == 3
      if !%actor.quest_variable[bounty_hunt:target1]%
        msend %actor% %victim1% is still out there.
      endif
      if !%actor.quest_variable[bount_hunt:target2]%
        msend %actor% %victim2% is still out there.
      endif
      if %stage% == 3 && !%actor.quest_variable[bounty_hunt:target3]%
        msend %actor% %victim3% is still out there.
      endif
    endif
  endif
elseif %go% == mask
  if %actor.quest_stage[bounty_hunt]% < %actor.quest_stage[assassin_mask]%
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'Complete some more contract jobs first.'
  elseif %actor.level% < (%actor.quest_stage[assassin_mask]% * 10)
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'You need to gain some more experience first.'
  elseif %maskstage% > %actor.quest_stage[assassin_mask]%
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'Your mask doesn't involve that yet.  Be patient!'
  elseif %maskstage% < %actor.quest_stage[assassin_mask]%
    return 0
    shake
    mecho %self.name% refuses %object.shortdesc%.
    wait 2
    msend %actor% %self.name% says, 'You've already gained that rank.'
  else
    if %item% == quest
      set job1 %actor.quest_variable[assassin_mask:masktask1]%
      set job2 %actor.quest_variable[assassin_mask:masktask2]%
      set job3 %actor.quest_variable[assassin_mask:masktask3]%
      set job4 %actor.quest_variable[assassin_mask:masktask4]%
      if %job1% && %job2% && %job3% && %job4%
        wait 2
        eval reward %object.vnum% + 1
        mjunk %object%
        nod
        msend %actor% %self.name% says, 'Well done!  You've proven your qualifications.'
        mload obj %reward%
        give mask %actor%
        eval expcap %maskstage% * 10
        if %expcap% < 17
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
        msend %actor% &3&bYou gain experience!&0
        eval setexp (%expmod% * 10)
        set loop 0
        while %loop% < 7
          mexp %actor% %setexp%
          eval loop %loop% + 1
        done
        set number 1
        while %number% < 5
          quest variable assassin_mask %actor% masktask%number% 0
          eval number %number% + 1
        done
        if %actor.quest_stage[assassin_mask]% < 9
          quest advance assassin_mask %actor%
        else
          quest complete assassin_mask %actor%
        endif
      else
        return 0
        shake
        mecho %self.name% refuses %object.shortdesc%.
        wait 2
        msend %actor% %self.name% says, 'You need to do everything else before you give me your guild mask!'
      endif
    elseif %item% == mask
      if %actor.quest_variable[assassin_mask:masktask2]% == %object.vnum%
        set accept no
      else
        set accept yes
        quest variable assassin_mask %actor% masktask2 %object.vnum%
      endif
    elseif %item% == gem
      if %actor.quest_variable[assassin_mask:masktask3]% == %object.vnum%
        set accept no
      else
        set accept yes
        quest variable assassin_mask %actor% masktask3 %object.vnum%
      endif
    endif
    if %accept% == no
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      msend %actor% %self.name% says, 'You already gave me that.'
    elseif %accept% == yes
      wait 2
      mjunk %object%
      set job1 %actor.quest_variable[assassin_mask:masktask1]%
      set job2 %actor.quest_variable[assassin_mask:masktask2]%
      set job3 %actor.quest_variable[assassin_mask:masktask3]%
      set job4 %actor.quest_variable[assassin_mask:masktask4]%
      if %job1% && %job2% && %job3% && %job4%
        msend %actor% %self.name% says, 'Excellent.  Now give me your guild mask for your promotion.'
      else
        msend %actor% %self.name% says, 'Good, now finish the rest.'
      endif
    endif
  endif
endif
~
#6054
Bounty hunt death triggers~
0 f 100
~
switch %self.vnum%
  case 8608
  * Meer Cat King
    set stage 1
    set target1 meer_cat_king
    break
  case 18509
  * Noble
    set target1 Noble
    set stage 2
    break
  case 18500
  * Abbot
    set target2 Abbot
    set stage 2
    break
  case 16300
  * O'Connor
    set target1 O'Connor
    set stage 3
    break
  case 16301
  * Cameron
    set target3 Cameron
    set stage 3
    break
  case 16302
  *  McLeod
    set target2 McLeod
    set stage 3
    break
  case 8316
  * Frakati Leader
    set target1 Frakati
    set stage 4
    break
  case 59015
  * Cyrus
    set target1 Cyrus
    set stage 5
    break
  case 2322
  * Lord Venth
    set target1 Venth
    set stage 6
    break
  case 2330
  * high druid
    set target1 Anlun_High_Druid
    set stage 7
    break
  case 53009
  * lizard king
    set target1 Lizard_King
    set stage 8
    break
  case 53309
  * Sorcha
    set target1 Sorcha
    set stage 9
    break
  case 58411
  * Goblin King
    set target1 Goblin_King
    set stage 10
    break
done
set person %actor%
set i %actor.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %actor.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[bounty_hunt]% == %stage% && %person.quest_variable[bounty_hunt:bounty]% == running
      if %target1%
        quest variable bounty_hunt %person% target1 %target1%
        msend %person% &1&bYou cross %self.name% off your list.&0
      elseif %target2%
        quest variable bounty_hunt %person% target2 %target2%
        msend %person% &1&bYou cross %self.name% off your list.&0
      elseif %target3%
        quest variable bounty_hunt %person% target3 %target3%
        msend %person% &1&bYou cross %self.name% off your list.&0
      endif
      if %stage% != 2 && %stage% != 3
        quest variable bounty_hunt %person% bounty dead
      elseif %stage% == 2
        if %person.quest_variable[bounty_hunt:target1]% && %person.quest_variable[bounty_hunt:target2]%
          quest variable bounty_hunt %person% bounty dead
        endif
      elseif %stage% == 3
        if %person.quest_variable[bounty_hunt:target1]% && %person.quest_variable[bounty_hunt:target2]% && %person.quest_variable[bounty_hunt:target3]%
          quest variable bounty_hunt %person% bounty dead
        endif
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
#6055
Bounty hunt contract look~
1 m 100
~
switch %self.vnum%
  case 6050
    set stage 1
    set victim1 the King of the Meer Cats
    set go hunt
    break
  case 6051
    set stage 2
    set victim1 the Noble
    set victim2 the Abbot
    set go hunt
    break
  case 6052
    set stage 3
    set victim1 the O'Connor Chieftain
    set victim2 the McLeod Chieftain
    set victim3 the Cameron Chieftain
    set go hunt
    break
  case 6053
    set stage 4
    set victim1 the Frakati Leader
    set go hunt
    break
  case 6054
    set stage 5
    set victim1 Cyrus
    set go hunt
    break
  case 6055
    set stage 6
    set victim1 Lord Venth
    set go hunt
    break
  case 6056
    set stage 7
    set victim1 the high druid of Anlun Vale
    set go hunt
    break
  case 6057
    set stage 8
    set victim1 the Lizard King
    set go hunt
    break
  case 6058
    set stage 9
    set victim1 Sorcha
    set go hunt
    break
  case 6059
    set stage 10
    set victim1 the Goblin King
    set go hunt
    break
done
return 0
if %stage% != 2 && %stage% != 3
  osend %actor% This is a contract for the death of %victim1%.
elseif %stage% == 2
  osend %actor% This is a contract for the death of %victim1% and %victim2%.
elseif %stage% == 3
  osend %actor% This is a contract for the death of %victim1%, %victim2%, and %victim3%.
endif
if %actor.quest_variable[bounty_hunt:bounty]% == dead
  osend %actor% You have completed the contract.  
  osend %actor% Return it to Berix for your payment!
elseif %stage% == 2
  if %actor.quest_variable[bounty_hunt:target1]%
    osend %actor% You have scratched %victim1% off the list.
  endif
  if %actor.quest_variable[bounty_hunt:target2]%
    osend %actor% You have scratched %victim2% off the list.
  endif
elseif %stage% == 3
  if %actor.quest_variable[bounty_hunt:target1]%
    osend %actor% You have scratched %victim1% off the list.
  endif
  if %actor.quest_variable[bounty_hunt:target2]%
    osend %actor% You have scratched %victim2% off the list.
  endif
  if %actor.quest_variable[bounty_hunt:target3]%
    osend %actor% You have scratched %victim3% off the list.
  endif
endif
~
#6056
Bount hunt contract examine~
1 c 3
examine~
if %arg% /= contract
  switch %self.vnum%
    case 6050
      set stage 1
      set victim1 the King of the Meer Cats
      set go hunt
      break
    case 6051
      set stage 2
      set victim1 the Noble
      set victim2 the Abbot
      set go hunt
      break
    case 6052
      set stage 3
      set victim1 the O'Connor Chieftain
      set victim2 the McLeod Chieftain
      set victim3 the Cameron Chieftain
      set go hunt
      break
    case 6053
      set stage 4
      set victim1 the Frakati Leader
      set go hunt
      break
    case 6054
      set stage 5
      set victim1 Cyrus
      set go hunt
      break
    case 6055
      set stage 6
      set victim1 Lord Venth
      set go hunt
      break
    case 6056
      set stage 7
      set victim1 the high druid of Anlun Vale
      set go hunt
      break
    case 6057
      set stage 8
      set victim1 the Lizard King
      set go hunt
      break
    case 6058
      set stage 9
      set victim1 Sorcha
      set go hunt
      break
    case 6059
      set stage 10
      set victim1 the Goblin King
      set go hunt
      break
  done
else
  return 0
  halt
endif
if %stage% != 2 && %stage% != 3
  osend %actor% This is a contract for the death of %victim1%.
elseif %stage% == 2
  osend %actor% This is a contract for the death of %victim1% and %victim2%.
elseif %stage% == 3
  osend %actor% This is a contract for the death of %victim1%, %victim2%, and %victim3%.
endif
if %actor.quest_variable[bounty_hunt:bounty]% == dead
  osend %actor% You have completed the contract.  
  osend %actor% Return it to Berix for your payment!
elseif %stage% == 2
  if %actor.quest_variable[bounty_hunt:target1]%
    osend %actor% You have scratched %victim1% off the list.
  endif
  if %actor.quest_variable[bounty_hunt:target2]%
    osend %actor% You have scratched %victim2% off the list.
  endif
elseif %stage% == 3
  if %actor.quest_variable[bounty_hunt:target1]%
    osend %actor% You have scratched %victim1% off the list.
  endif
  if %actor.quest_variable[bounty_hunt:target2]%
    osend %actor% You have scratched %victim2% off the list.
  endif
  if %actor.quest_variable[bounty_hunt:target3]%
    osend %actor% You have scratched %victim3% off the list.
  endif
endif
~
#6057
Berix assassin mask promotion speech~
0 d 100
promotion~
set maskstage %actor.quest_stage[assassin_mask]%
set bountystage %actor.quest_stage[bounty_hunt]%
set job1 %actor.quest_variable[assassin_mask:masktask1]%
set job2 %actor.quest_variable[assassin_mask:masktask2]%
set job3 %actor.quest_variable[assassin_mask:masktask3]%
set job4 %actor.quest_variable[assassin_mask:masktask4]%
wait 2
if %actor.class% != Assassin
  mecho %self.name% scoffs in disgust.
  msend %actor% %self.name% says, 'You ain't part of the Guild.  Get lost.'
  halt
elseif %actor.level% < 10
  msend %actor% %self.name% says, 'You ain't ready for a promotion yet.  Come back when you've grown a bit.'
  halt
elseif %actor.level% < (%maskstage% * 10)
  say You ain't ready for another promotion yet.  Come back when you've gained some more experience.
  halt
elseif %actor.has_completed[assassin_mask]%
  msend %actor% %self.name% says, 'You've already gone as high as you can go!'
  halt
endif
if %maskstage% == 0
  mecho %self.name% says, 'Sure.  You gotta do a &6&b[job]&0 for me first though.'
  halt
elseif (%maskstage% >= %bountystage%) && !%actor.has_completed[bounty_hunt]%
  msend %actor% %self.name% says, 'Complete some more contract jobs and then we can talk.'
  halt
elseif %job1% && %job2% && %job3% && %job4%
  msend %actor% %self.name% says, 'You're all ready, just give me your old mask.'
  halt
endif
switch %maskstage%
  case 1
    set mask 4500
    set gem 55592
    set place The Shadowy Lair
    set hint in the Misty Caverns.
    break
  case 2
    set mask 17809
    set gem 55594
    set place The Dark Chamber
    set hint behind a desert door.
    break
  case 3
    set mask 59023
    set gem 55620
    set place A Dark Tunnel
    set hint on the way to a dark, hidden city.
    break
  case 4
    set mask 10304
    set gem 55638
    set place Dark Chamber
    set hint hidden below a ghostly fortress.
    break
  case 5
    set mask 16200
    set gem 55666
    set place Darkness......
    set hint inside an enchanted closet.
    break
  case 6
    set mask 43017
    set gem 55675
    set place Surrounded by Darkness
    set hint in a volcanic shaft.
    break
  case 7
    set mask 51075
    set gem 55693
    set place Dark Indecision
    set hint before an altar in a fallen maze.
    break
  case 8
    set mask 49062
    set gem 55719
    set place Heart of Darkness
    set hint buried deep in an ancient tomb.
    break
  case 9
    set mask 48427
    set gem 55743
    set place A Dark Room
    set hint under the ruins of a shop in an ancient city.
    break
done
eval attacks %maskstage% * 100
nod
msend %actor% %self.name% says, 'Sure, I can help you climb the ranks.  Each new station you'll earn a new mask.  Do the following:
msend %actor% - Attack &9&b%attacks%&0 times while wearing your current mask.
msend %actor% - Find &9&b%get.obj_shortdesc[%mask%]%&0 as the base for the new mask.
msend %actor% - Find &9&b%get.obj_shortdesc[%gem%]%&0 for decoration.
msend %actor% &0    
msend %actor% You also need to take your mask and &9&b[hide]&0 in a secret, dark, shadowy place.
msend %actor% Find "&9&b%place%&0".  It's %hint%
msend %actor% &0  
msend %actor% You can ask about your &6&b[mask progress]&0 at any time.'
~
#6058
Assassin mask command hide~
1 c 3
hide~
switch %cmd%
  case h
  case hi
    return 0
    halt
done
set maskstage %actor.quest_stage[assassin_mask]%
switch %self.vnum%
  case 350
    if (%actor.room% >= 5436 && %actor.room% <= 5440) && %maskstage% == 1
      set continue yes
    endif
    break
  case 351
    if (%actor.room% >= 16165 && %actor.room% <= 16173) && %maskstage% == 2
      set continue yes
    endif
    break
  case 352
    if (%actor.room% >= 23711 && %actor.room% <= 23715) && %maskstage% == 3
      set continue yes
    endif
    break
  case 353
    if %actor.room% == 16087 && %maskstage% == 4
      set continue yes
    endif
    break
  case 354
    if %actor.room% == 51023 && %maskstage% == 5
      set continue yes
    endif
    break
  case 355
    if ((%actor.room% >= 48191 && %actor.room% <= 48196) || (%actor.room% >= 48225 && %actor.room% <= 48232)) && %maskstage% == 6
      set continue yes
    endif
    break
  case 356
    if %actor.room% == 4064 && %maskstage% == 7
      set continue yes
    endif
    break
  case 357
    if %actor.room% == 48045 && %maskstage% == 8
      set continue yes
    endif
    break
  case 358
    if %actor.room% == 52046 && %maskstage% == 9
      set continue yes
    endif
done
if %continue% == yes
  oforce %actor% hide
  wait 2
  osend %actor% &9&b%self.shortdesc% seems to absorb the shadowy darkness around you!
  quest variable assassin_mask %actor% masktask4 1
else
  return 0
endif
~
#6059
Assassin mask progress checker~
0 d 1
status progress~
wait 2
msend %actor% &2&bContract Killers&0
if %actor.has_completed[bounty_hunt]%
  msend %actor% %self.name% says, 'You know, I'm fresh out of work for you.  Good luck!'
elseif !%actor.quest_stage[bounty_hunt]%
  msend %actor% %self.name% says, 'You aren't doing a job for me.'
elseif %actor.quest_variable[bounty_hunt:bounty]% == dead
  msend %actor% %self.name% says, 'Give me your current contract.'
elseif %actor.level% >= (%actor.quest_stage[bounty_hunt]% - 1) * 10
  if %actor.quest_variable[bounty_hunt:bounty]% != running
    msend %actor% %self.name% says, 'You aren't doing a job for me.'
  else
    switch %actor.quest_stage[bounty_hunt]%
      case 1
        msend %actor% %self.name% says, 'You still have a job to do.  Best get on killing that cat-king or whatever first.'
        break
      case 2
        msend %actor% %self.name% says, 'You still have a job to do.  Take out the Noble and the Abbot  sheltering him at the Abbey of St. George.'
        break
      case 3
        msend %actor% %self.name% says, 'You still have a job to do.  Take out the three Chieftains in the southwestern Highlands.'
        break
      case 4
        msend %actor% %self.name% says, 'You still have a job to do.  Find the Frakati Leader and kill him.'
        break
      case 5
        msend %actor% %self.name% says, 'You still have a job to do.  Infiltrate the Sacred Haven and take out the number two in command, Cyrus.'
        break
      case 6
        msend %actor% %self.name% says, 'You still have a job to do.  Disappear Lord Venth down south.'
        break
      case 7
        msend %actor% %self.name% says, 'You still have a job to do.  Send the high druid on a permanent pilgrimage.'
        break
      case 8
        msend %actor% %self.name% says, 'You still have a job to do.  Kill the Lizard King.  If you can even find him...'
        break
      case 9
        msend %actor% %self.name% says, 'You still have a job to do.  Take out the leader of the Ice Cult up north before they get wind of it.'
        break
      case 10
        msend %actor% %self.name% says, 'You still have a job to do.  End the reign of the Goblin King.  We'll all probably dream a little more soundly then.'
    done
  endif
else
  msend %actor% %self.name% says, 'All my other jobs are too risky for someone without moreexperience.  Come back when you've seen a little more.'
endif
if %actor.class% == Assassin
  msend %actor%  &0
  msend %actor% &2&bDeadly Promotion&0
  set bountystage %actor.quest_stage[bounty_hunt]%
  set maskstage %actor.quest_stage[assassin_mask]%
  set job1 %actor.quest_variable[assassin_mask:masktask1]%
  set job2 %actor.quest_variable[assassin_mask:masktask2]%
  set job3 %actor.quest_variable[assassin_mask:masktask3]%
  set job4 %actor.quest_variable[assassin_mask:masktask4]%
  wait 2
  if %actor.class% != Assassin
    mecho %self.name% scoffs in disgust.
    msend %actor% %self.name% says, 'You ain't part of the Guild.  Get lost.'
    halt
  elseif %actor.level% < 10
    msend %actor% %self.name% says, 'You ain't ready for a promotion yet.  Come back when you've grown a bit.'
    halt
  elseif %actor.level% < (%maskstage% * 10)
    msend %actor% %self.name% says, 'You ain't ready for another promotion yet.  Come back when you've gained some more experience.'
    halt
  elseif %actor.has_completed[assassin_mask]%
    msend %actor% %self.name% says, 'You've already gone as high as you can go!'
    halt
  endif
  if %maskstage% == 0
    msend %actor% %self.name% says, 'Sure.  You gotta do a &6&b[job]&0 for me first though.'
    halt
  elseif (%maskstage% >= %bountystage%) && !%actor.has_completed[bounty_hunt]%
    msend %actor% %self.name% says, 'Complete some more contract jobs and then we can talk.'
    halt
  endif
  switch %maskstage%
    case 1
      set mask 4500
      set gem 55592
      set place The Shadowy Lair
      set hint in the Misty Caverns.
      break
    case 2
      set mask 17809
      set gem 55594
      set place The Dark Chamber
      set hint behind a desert door.
      break
    case 3
      set mask 59023
      set gem 55620
      set place A Dark Tunnel
      set hint on the way to a dark, hidden city.
      break
    case 4
      set mask 10304
      set gem 55638
      set place Dark Chamber
      set hint hidden below a ghostly fortress.
      break
    case 5
      set mask 16200
      set gem 55666
      set place Darkness......
      set hint inside an enchanted closet.
      break
    case 6
      set mask 43017
      set gem 55675
      set place Surrounded by Darkness
      set hint in a volcanic shaft.
      break
    case 7
      set mask 51075
      set gem 55693
      set place Dark Indecision
      set hint before an altar in a fallen maze.
      break
    case 8
      set mask 49062
      set gem 55719
      set place Heart of Darkness
      set hint buried deep in an ancient tomb.
      break
    case 9
      set mask 48427
      set gem 55743
      set place A Dark Room
      set hint under the ruins of a shop in an ancient city.
      break
  done
  eval attack %maskstage% * 100
  if %job1% || %job2% || %job3% || %job4% 
    msend %actor% %self.name% says, You've done the following:'
    if %job1%
      msend %actor% - attacked %attack% times
    endif
    if %job2%
      msend %actor% - found %get.obj_shortdesc[%mask%]%
    endif
    if %job3%
      msend %actor% - found %get.obj_shortdesc[%gem%]%
    endif
    if %job4%
      msend %actor% - hidden in %place%
    endif
  endif
  msend %actor%
  msend %actor% You need to:
  if %job1% && %job2% && %job3% && %job4%
    msend %actor% Just give me your old mask.
    halt
  endif
  if !%job1%
    eval remaining %attack% - %actor.quest_variable[assassin_mask:attack_counter]%
    msend %actor% - attack &9&b%remaining%&0 more times while wearing your mask.
  endif
  if !%job2%
    msend %actor% - find &9&b%get.obj_shortdesc[%mask%]%&0
  endif
  if !%job3%
    msend %actor% - find &9&b%get.obj_shortdesc[%gem%]%&0
  endif
  if !%job4%
    msend %actor% - &9&bhide in a place called "&9&b%place%&0".
    msend %actor%&0   It's &9&b%hint%&0
  endif
endif
~
#6060
Berix bounty hunt new contract~
0 d 0
I need a new contract~
wait 2
if %actor.level% >= (%actor.quest_stage[bounty_hunt]% - 1) * 10
  if %actor.quest_variable[bounty_hunt:bounty]%
    switch %actor.quest_stage[bounty_hunt]%
      case 1
        set contract 6050
        break
      case 2
        set contract 6051
        break
      case 3
        set contract 6052
        break
      case 4
        set contract 6053
        break
      case 5
        set contract 6054
        break
      case 6
        set contract 6055
        break
      case 7
        set contract 6056
        break
      case 8
        set contract 6057
        break
      case 9
        set contract 6058
        break
      case 10
        set contract 6059
        break
      default
        set contract 6050
    done
    whap %actor%
    msend %actor% %self.name% says, 'Don't be such an idiot again!'
    mload obj %contract%
    give contract %actor%
  endif
endif
~
#6061
connectfour start~
1 c 4
start~
switch %cmd%
  case s
  case st
  case sta
  case star
    return 0
    halt
done
if %player1%
   if %player2%
      osend %actor% %player1.name% and %player2.name% are already playing ConnectFour!
   else
      oechoaround %actor% %actor.name% joins the ConnectFour game as player 2.
      osend %actor% You are now joining the ConnectFour game!
      if %p2col% == &1&b
         osend %actor% You are the &1&bred&0 pieces!
         oechoaround %actor% %actor.name% gets the &1&bred&0 pieces.
         osend %player1 You get to go first!
         set status 1
         global status
         set turn 1
         global turn
      elseif %p2col% == &b&9
         osend %actor% You are the &b&9black&0 pieces!
         oechoaround %actor% %actor.name% gets the &b&9black&0 pieces.
         osend %player1% You get to go first!
         set status 1
         global status
         set turn 1
         global turn
      endif
      set player2 %actor%
      global player2
   endif
else
   oechoaround %actor% %actor.name% starts a new ConnectFour game as player 1.
   osend %actor% Alright!  Starting new ConnectFour game.  You are player 1.
   osend %actor% Do you want to be red or black?  Use the select command to choose.
   set player1 %actor%
   global player1
endif
~
#6062
**UNUSED**~
1 c 4
sel~
return 0
~
#6063
connectfour select color~
1 c 4
select~
switch %cmd%
  case s
  case se
  case sel
    return 0
    halt
done
if (%status% == 0) && %player1% && (%actor.name% == %player1.name%)
   if %arg% == red
      osend %actor% You are the &1&bred&0 pieces!
      oechoaround %actor% %actor.name% chooses to play with the &1&bred&0 pieces!
      if %player2%
         osend %player2% You are the &b&9black&0 pieces!
         oechoaround %player2% %player2.name% gets the &b&9black&0 pieces.
         osend %actor% You get to go first!
         set status 1
         global status
         set turn 1
         global turn
      endif
      set p1col &1&b
      global p1col
      set p2col &b&9
      global p2col
      set p1desc &1&bred&0
      global p1desc
      set p2desc &b&9black&0
      global p2desc
   elseif %arg% == black
      osend %actor% You are the &b&9black&0 pieces!
      oechoaround %actor% %actor.name% chooses to play with the &b&9black&0 pieces!
      if %player2%
         osend %player2% You are the &1&bred&0 pieces!
         oechoaround %player2% %player2.name% gets the &1&bred&0 pieces.
         osend %actor% You get to go first!
         set status 1
         global status
         set turn 1
         global turn
      endif
      set p1col &b&9
      global p1col
      set p2col &1&b
      global p2col
      set p1desc &b&9black&0
      global p1desc
      set p2desc &1&bred&0
      global p2desc
   else
      osend %actor% You may only select 'red' or 'black'.
   endif
   set a1 O
   set a2 O
   set a3 O
   set a4 O
   set a5 O
   set a6 O
   set a7 O
   set b1 O
   set b2 O
   set b3 O
   set b4 O
   set b5 O
   set b6 O
   set b7 O
   set c1 O
   set c2 O
   set c3 O
   set c4 O
   set c5 O
   set c6 O
   set c7 O
   set d1 O
   set d2 O
   set d3 O
   set d4 O
   set d5 O
   set d6 O
   set d7 O
   set e1 O
   set e2 O
   set e3 O
   set e4 O
   set e5 O
   set e6 O
   set e7 O
   set f1 O
   set f2 O
   set f3 O
   set f4 O
   set f5 O
   set f6 O
   set f7 O
   set g1 O
   set g2 O
   set g3 O
   set g4 O
   set g5 O
   set g6 O
   set g7 O
   global a1
   global a2
   global a3
   global a4
   global a5
   global a6
   global a7
   global b1
   global b2
   global b3
   global b4
   global b5
   global b6
   global b7
   global c1
   global c2
   global c3
   global c4
   global c5
   global c6
   global c7
   global d1
   global d2
   global d3
   global d4
   global d5
   global d6
   global d7
   global e1
   global e2
   global e3
   global e4
   global e5
   global e6
   global e7
   global f1
   global f2
   global f3
   global f4
   global f5
   global f6
   global f7
   global g1
   global g2
   global g3
   global g4
   global g5
   global g6
   global g7
else
   return O
endif
~
#6064
connectfour look board~
1 c 4
look~
if (%arg% != board) && (%arg% != connectfour) && (%arg% != game)
   return 0
   halt
endif
osend %actor% &b Connect Four!&0
osend %actor% &0
osend %actor% &b+-+-+-+-+-+-+-+&0
osend %actor% &b|1|2|3|4|5|6|7|&0
osend %actor% &b+-+-+-+-+-+-+-+&0
osend %actor% &b|%a1%&b|%a2%&b|%a3%&b|%a4%&b|%a5%&b|%a6%&b|%a7%&b|&0
osend %actor% &b+-+-+-+-+-+-+-+&0
osend %actor% &b|%b1%&b|%b2%&b|%b3%&b|%b4%&b|%b5%&b|%b6%&b|%b7%&b|&0
osend %actor% &b+-+-+-+-+-+-+-+&0
osend %actor% &b|%c1%&b|%c2%&b|%c3%&b|%c4%&b|%c5%&b|%c6%&b|%c7%&b|&0
osend %actor% &b+-+-+-+-+-+-+-+&0
osend %actor% &b|%d1%&b|%d2%&b|%d3%&b|%d4%&b|%d5%&b|%d6%&b|%d7%&b|&0
osend %actor% &b+-+-+-+-+-+-+-+&0
osend %actor% &b|%e1%&b|%e2%&b|%e3%&b|%e4%&b|%e5%&b|%e6%&b|%e7%&b|&0
osend %actor% &b+-+-+-+-+-+-+-+&0
osend %actor% &b|%f1%&b|%f2%&b|%f3%&b|%f4%&b|%f5%&b|%f6%&b|%f7%&b|&0
osend %actor% &b+-+-+-+-+-+-+-+&0
osend %actor% &b|%g1%&b|%g2%&b|%g3%&b|%g4%&b|%g5%&b|%g6%&b|%g7%&b|&0
osend %actor% &b+-+-+-+-+-+-+-+&0
osend %actor% &0
if %status% == 0
   if %player1%
      if %player2%
         if %player1.name% == %actor.name%
            osend %actor% You and %player2.name% are getting ready to start a game!
            osend %actor% You need to choose a color!  Say 'red' or 'black'.
         elseif %player2.name% == %actor.name%
            osend %actor% You and %player1.name% are getting ready to start a game!
            osend %actor% We are waiting for %player1.name% to choose a color.
         else
            osend %actor% %player1.name% and %player2.name% are getting ready to start a game.
         endif
      else
         if %player1.name% == %actor.name%
            osend %actor% You are waiting for someone to join you.
            if %p1desc%
               osend %actor% You are playing as %p1desc%.
            else
               osend %actor% You need to choose a color!  Say 'red' or 'black'.
            endif
         else
            osend %actor% %player1.name% is waiting for someone to join %player1.o%.
            if %p1desc%
               osend %actor% %player1.name% is playing as %p1desc%.
            endif            
         endif
      endif
   endif
elseif %status% == 1
   if %player1.name% == %actor.name%
      osend %actor% You and %player2.name% are playing.
      osend %actor% You are %p1desc% and %player2.name% is %p2desc%.
      if %turn% == 1
         osend %actor% It's your turn!
      else
         osend %actor% It's %player2.name%'s turn.
      endif
   elseif %player2.name% == %actor.name%
      osend %actor% You and %player1.name% are playing.
      osend %actor% You are %p2desc% and %player1.name% is %p1desc%.
      if %turn% == 2
         osend %actor% It's your turn!
      else
         osend %actor% It's %player1.name%'s turn.
      endif
   else
      osend %actor% %player1.name% and %player2.name% are playing.
      osend %actor% %player1.name% is %p1desc% and %player2.name% is %p2desc%.
      if %turn% == 1
         osend %actor% It's %player1.name%'s turn.
      else
         osend %actor% It's %player2.name%'s turn.
      endif
   endif
elseif %status% == 2
   if %player1.name% == %actor.name%
      osend %actor% You and %player2.name% were playing.
   elseif %player2.name% == %actor.name%
      osend %actor% You and %player1.name% were playing.
   else
      osend %actor% %player1.name% and %player2.name% were playing.
   endif
   if %p1win%
      if %player1.name% == %actor.name%
         osend %actor% You won the game!
      else
         osend %actor% %player1.name% won the game.
      endif
   elseif %p2win%
      if %player2.name% == %actor.name%
         osend %actor% You won the game!
      else
         osend %actor% %player2.name% won the game.
      endif
   elseif %p1forfeit% == 2
      if %player1.name% == %actor.name%
         osend %actor% You forfeited the game.
      else
         osend %actor% %player1.name% forfeited the game.
      endif
   elseif %p2forfeit% == 2
      if %player2.name% == %actor.name%
         osend %actor% You forfeited the game.
      else
         osend %actor% %player2.name% forfeited the game.
      endif
   else
      osend %actor% The game is over.
   endif
endif
~
#6065
**UNUSED**~
1 c 4
dr~
return 0
~
#6066
connectfour drop piece~
1 c 4
drop~
switch %cmd%
  case d
  case dr
    return 0
    halt
done
if (%actor.name% == %player1.name%) || (%actor.name% == %player2.name%)
   if %status% == 0
      osend %actor% The game hasn't started yet!
      halt
   elseif %status% == 2
      osend %actor% The game is already over!
      halt
   endif
endif
if (%actor.name% == %player1.name%)
   if %turn% != 1
      osend %actor% It's not your turn!
      halt
   endif
   set piece %p1col%O&0
elseif %actor.name% == %player2.name%
   if %turn% != 2
      osend %actor% It's not your turn!
      halt
   endif
   set piece %p2col%O&0
else
   return 0
   halt
endif
if %arg% == 1
   if %g1% == O
      set g1 %piece%
      global g1
   elseif %f1% == O
      set f1 %piece%
      global f1
   elseif %e1% == O
      set e1 %piece%
      global e1
   elseif %d1% == O
      set d1 %piece%
      global d1
   elseif %c1% == O
      set c1 %piece%
      global c1
   elseif %b1% == O
      set b1 %piece%
      global b1
   elseif %a1% == O
      set a1 %piece%
      global a1
   else
      set full 1
   endif
elseif %arg% == 2
   if %g2% == O
      set g2 %piece%
      global g2
   elseif %f2% == O
      set f2 %piece%
      global f2
   elseif %e2% == O
      set e2 %piece%
      global e2
   elseif %d2% == O
      set d2 %piece%
      global d2
   elseif %c2% == O
      set c2 %piece%
      global c2
   elseif %b2% == O
      set b2 %piece%
      global b2
   elseif %a2% == O
      set a2 %piece%
      global a2
   else
      set full 1
   endif
elseif %arg% == 3
   if %g3% == O
      set g3 %piece%
      global g3
   elseif %f3% == O
      set f3 %piece%
      global f3
   elseif %e3% == O
      set e3 %piece%
      global e3
   elseif %d3% == O
      set d3 %piece%
      global d3
   elseif %c3% == O
      set c3 %piece%
      global c3
   elseif %b3% == O
      set b3 %piece%
      global b3
   elseif %a3% == O
      set a3 %piece%
      global a3
   else
      set full 1
   endif
elseif %arg% == 4
   if %g4% == O
      set g4 %piece%
      global g4
   elseif %f4% == O
      set f4 %piece%
      global f4
   elseif %e4% == O
      set e4 %piece%
      global e4
   elseif %d4% == O
      set d4 %piece%
      global d4
   elseif %c4% == O
      set c4 %piece%
      global c4
   elseif %b4% == O
      set b4 %piece%
      global b4
   elseif %a4% == O
      set a4 %piece%
      global a4
   else
      set full 1
   endif
elseif %arg% == 5
   if %g5% == O
      set g5 %piece%
      global g5
   elseif %f5% == O
      set f5 %piece%
      global f5
   elseif %e5% == O
      set e5 %piece%
      global e5
   elseif %d5% == O
      set d5 %piece%
      global d5
   elseif %c5% == O
      set c5 %piece%
      global c5
   elseif %b5% == O
      set b5 %piece%
      global b5
   elseif %a5% == O
      set a5 %piece%
      global a5
   else
      set full 1
   endif
elseif %arg% == 6
   if %g6% == O
      set g6 %piece%
      global g6
   elseif %f6% == O
      set f6 %piece%
      global f6
   elseif %e6% == O
      set e6 %piece%
      global e6
   elseif %d6% == O
      set d6 %piece%
      global d6
   elseif %c6% == O
      set c6 %piece%
      global c6
   elseif %b6% == O
      set b6 %piece%
      global b6
   elseif %a6% == O
      set a6 %piece%
      global a6
   else
      set full 1
   endif
elseif %arg% == 7
   if %g7% == O
      set g7 %piece%
      global g7
   elseif %f7% == O
      set f7 %piece%
      global f7
   elseif %e7% == O
      set e7 %piece%
      global e7
   elseif %d7% == O
      set d7 %piece%
      global d7
   elseif %c7% == O
      set c7 %piece%
      global c7
   elseif %b7% == O
      set b7 %piece%
      global b7
   elseif %a7% == O
      set a7 %piece%
      global a7
   else
      set full 1
   endif
else
   osend %actor% Drop a piece in where!?
   halt
endif
if %full%
   osend %actor% Column %arg% is full!  Try a different one.
   halt
endif
osend %actor% You drop a piece in column %arg%.
oechoaround %actor% %actor.name% drops a piece in column %arg%.
set row1 %a1%%a2%%a3%%a4%%a5%%a6%%a7%
set row2 %b1%%b2%%b3%%b4%%b5%%b6%%b7%
set row3 %c1%%c2%%c3%%c4%%c5%%c6%%c7%
set row4 %d1%%d2%%d3%%d4%%d5%%d6%%d7%
set row5 %e1%%e2%%e3%%e4%%e5%%e6%%e7%
set row6 %f1%%f2%%f3%%f4%%f5%%f6%%f7%
set row7 %g1%%g2%%g3%%g4%%g5%%g6%%g7%
set col1 %a1%%b1%%c1%%d1%%e1%%f1%%g1%
set col2 %a2%%b2%%c2%%d2%%e2%%f2%%g2%
set col3 %a3%%b3%%c3%%d3%%e3%%f3%%g3%
set col4 %a4%%b4%%c4%%d4%%e4%%f4%%g4%
set col5 %a5%%b5%%c5%%d5%%e5%%f5%%g5%
set col6 %a6%%b6%%c6%%d6%%e6%%f6%%g6%
set col7 %a7%%b7%%c7%%d7%%e7%%f7%%g7%
set dnw1 %d1%%e2%%f3%%g4%
set dnw2 %c1%%d2%%e3%%f4%%g5%
set dnw3 %b1%%c2%%d3%%e4%%f5%%g6%
set dnw4 %a1%%b2%%c3%%d4%%e5%%f6%%g7%
set dnw5 %a2%%b3%%c4%%d5%%e6%%f7%
set dnw6 %a3%%b4%%c5%%d6%%e7%
set dnw7 %a4%%b5%%c6%%d7%
set dne1 %a4%%b3%%c2%%d1%
set dne2 %a5%%b4%%c3%%d2%%e1%
set dne3 %a6%%b5%%c4%%d3%%e2%%f1%
set dne4 %a7%%b6%%c5%%d4%%e3%%f2%%g1%
set dne5 %b7%%c6%%d5%%e4%%f3%%g2%
set dne6 %c7%%d6%%e5%%f4%%g3%
set dne7 %d7%%e6%%f5%%g4%
set streak %piece%%piece%%piece%%piece%
if (%row1% /= %streak%) || (%row2% /= %streak%) || (%row3% /= %streak%) || (%row4% /= %streak%) || (%row5% /= %streak%) || (%row6% /= %streak%) || (%row7% /= %streak%)
   set win 1
elseif (%col1% /= %streak%) || (%col2% /= %streak%) || (%col3% /= %streak%) || (%col4% /= %streak%) || (%col5% /= %streak%) || (%col6% /= %streak%) || (%col7% /= %streak%)
   set win 1
elseif (%dnw1% /= %streak%) || (%dnw2% /= %streak%) || (%dnw3% /= %streak%) || (%dnw4% /= %streak%) || (%dnw5% /= %streak%) || (%dnw6% /= %streak%) || (%dnw7% /= %streak%)
   set win 1
elseif (%dne1% /= %streak%) || (%dne2% /= %streak%) || (%dne3% /= %streak%) || (%dne4% /= %streak%) || (%dne5% /= %streak%) || (%dne6% /= %streak%) || (%dne7% /= %streak%)
   set win 1
endif
if %win%
   osend %actor% You win!
   oechoaround %actor% %actor.name% wins!
   if %player1.name% == %actor.name%
      set p1win 1
      global p1win
   else
      set p2win 1
      global p2win
   endif
   set status 2
   global status
endif
if %turn% == 1
   set turn 2
else
   set turn 1
endif
global turn
~
#6067
**UNUSED**~
1 c 4
res~
return 0
~
#6068
connectfour reset game~
1 c 4
reset~
switch %cmd%
  case r
  case re
  case res
    return 0
    halt
done
if %status% == 2
   oecho Resetting ConnectFour.
   set a1 0
   set a2 0
   set a3 0
   set a4 0
   set a5 0
   set a6 0
   set a7 0
   set b1 0
   set b2 0
   set b3 0
   set b4 0
   set b5 0
   set b6 0
   set b7 0
   set c1 0
   set c2 0
   set c3 0
   set c4 0
   set c5 0
   set c6 0
   set c7 0
   set d1 0
   set d2 0
   set d3 0
   set d4 0
   set d5 0
   set d6 0
   set d7 0
   set e1 0
   set e2 0
   set e3 0
   set e4 0
   set e5 0
   set e6 0
   set e7 0
   set f1 0
   set f2 0
   set f3 0
   set f4 0
   set f5 0
   set f6 0
   set f7 0
   set g1 0
   set g2 0
   set g3 0
   set g4 0
   set g5 0
   set g6 0
   set g7 0
   set player1 0
   set player2 0
   set status 0
   set p1col 0
   set p2col 0
   set p1desc 0
   set p2desc 0
   set p1win 0
   set p2win 0
   set p1forfeit 0
   set p2forfeit 0
   set turn
   global a1
   global a2
   global a3
   global a4
   global a5
   global a6
   global a7
   global b1
   global b2
   global b3
   global b4
   global b5
   global b6
   global b7
   global c1
   global c2
   global c3
   global c4
   global c5
   global c6
   global c7
   global d1
   global d2
   global d3
   global d4
   global d5
   global d6
   global d7
   global e1
   global e2
   global e3
   global e4
   global e5
   global e6
   global e7
   global f1
   global f2
   global f3
   global f4
   global f5
   global f6
   global f7
   global g1
   global g2
   global g3
   global g4
   global g5
   global g6
   global g7
   global player1
   global player2
   global status
   global p1col
   global p2col
   global p1desc
   global p2desc
   global p1win
   global p2win
   global p1forfeit
   global p2forfeit
   global turn
   oecho Board cleared.
elseif %status% == 1
   if (%actor.name% == %player1.name%) || (%actor.name% == %player2.name%)
      osend %actor% If you want to end the game, forfeit first, then reset.
   else
      osend %actor% Wait until %player1.name% and %player2.name% are done playing!
   endif
elseif %status% == 0
   if %player1% && (%actor.name% == %player1.name%)
      osend %actor% You can't reset the board until the game is over.
   else
      osend %actor% The board is already cleared!
   endif
endif
~
#6069
**UNUSED**~
1 c 4
for~
return 0
~
#6070
connectfour forfeit game~
1 c 4
forfeit~
switch %cmd%
  case f
  case fo
  case for
    return 0
    halt
done
if (%status% == 0) || (!%player1) || (!%player2)
   osend %actor% But the game hasn't even started yet!
elseif (%player1.name% == %actor.name%) || (%player2.name% == %actor.name%)
   if %status% == 2
      osend %actor% But the game's already over!
      halt
   endif
   if %arg% == yes
      osend %actor% You forfeit the game!
      oechoaround %actor% %actor.name% forfeits the game!
      if %player1.name% == %actor.name%
         set p1forfeit 2
         global p1forfeit
      elseif %player2.name% == %actor.name%
         set p2forfeit 2
         global p2forfeit
      endif
      set status 2
      global status
   else
      osend %actor% Are you absolutely sure you want to forfeit?  If so, type 'forfeit yes'.
      if %player1.name% == %actor.name%
         set p1forfeit 1
         global p1forfeit
      elseif %player2.name% == %actor.name%
         set p2forfeit 1
         global p2forfeit
      endif
   endif
else
   osend %actor% You can't forfeit a game you're not even playing!
endif
~
#6071
**UNUSED**~
1 c 4
ru~
return 0
~
#6072
connectfour rules~
1 c 4
rules~
switch %cmd%
  case r
  case ru
    return 0
    halt
done
osend %actor% Playing Connect Four is simple!  Your goal is simply to get four of your pieces
osend %actor% in a line before your opponent.  The pieces can be horizontal, vertical, or
osend %actor% even diagonal.
osend %actor% &0
osend %actor% To begin a game, you and a friend must type 'start' in the room with the game.
osend %actor% Whoever starts first gets to select their color, red or black.  The second
osend %actor% player automatically gets the other color.  If you type 'start' first, you also
osend %actor% get the first turn.
osend %actor% &0
osend %actor% At any time, you can 'look board' to see what the board looks like.
osend %actor% &0
osend %actor% On your turn, you can 'drop' a piece into one of the 7 columns.  For instance,
osend %actor% if you want to drop a piece in the third column, you'd type 'drop 3'.  The
osend %actor% piece will then fall down to the bottom of the third column.  If the column is
osend %actor% full, you'll have to pick another one.
osend %actor% &0
osend %actor% During the game, you may use the 'forfeit' command to end the game.  If you
osend %actor% leave the room, you automatically forfeit.
osend %actor% &0
osend %actor% Once a game is over, either by someone winning or forfeiture, anyone in the
osend %actor% room can use the 'reset' command to clear the board to start a new game.
~
#6075
hearts start~
1 c 4
start~
switch %cmd%
  case s
  case st
  case sta
  case star
    return 0
    halt
done
if %status%
   osend %actor% The game has already started.  You must wait for it to finish.
endif
if %player1%
   if %player2%
      if %player3%
         if %player4%
            osend %actor% Sorry, four people are already playing Hearts.
            osend %actor% You'll have to wait for the next game.
         else
            if (%actor.name% == %player1.name%) || (%actor.name% == %player2.name%) || (%actor.name% == %player3.name%)
               osend %actor% You're already in the game!
            else
               oechoaround %actor% %actor.name% joins the Hearts game as player 4.
               osend %actor% You are now joining the Hearts game as player 4.
               set player4 %actor%
               global player4
               osend %player1% The cards are now ready to be dealt.
               osend %player2% The cards are now ready to be dealt.
               osend %player3% The cards are now ready to be dealt.
               osend %actor% The cards are now ready to be dealt.
               set status 1
               global status
            endif
         endif
      else
         if (%actor.name% == %player1.name%) || (%actor.name% == %player2.name%)
            osend %actor% You're already in the game!
         else
            oechoaround %actor% %actor.name% joins the Hearts game as player 3.
            osend %actor% You are now joining the Hearts game as player 3.
            set player3 %actor%
            global player3
         endif
      endif
   else
      if %actor.name% == %player1.name%
         osend %actor% You're already in the game!
      else
         oechoaround %actor% %actor.name% joins the Hearts game as player 2.
         osend %actor% You are now joining the Hearts game as player 2.
         set player2 %actor%
         global player2
      endif
   endif
else
   oechoaround %actor% %actor.name% starts a new game of Hearts as player 1.
   osend %actor% Alright!  Starting a new game of Hearts.  You are player 1.
   set player1 %actor%
   global player1
endif
~
#6076
**UNUSED**~
1 c 4
de~
return 0
~
#6077
hearts deal~
1 c 4
deal~
switch %cmd%
  case d
  case de
    return 0
    halt
done
if %player1% && %player2% && %player3% && %player4%
   if (%player1.name% != %actor.name%) && (%player2.name% != %actor.name%) && (%player3.name% != %actor.name%) &&
(%player4.name% != %actor.name%)
      return 0
      halt
   endif
   if %status% > 1
      osend %actor% Wait for this hand to end before dealing again!
      halt
   endif
   osend %actor% You deal the cards across the table.
   oechoaround %actor% %actor.name% deals the cards, beginning a new round.
   set card 1
   while %card% <= 52
      set card%card% 0
      eval rank %card% - 13 * ((%card% - 1)/ 13)
      eval suit ((%card% - 1)/ 13) + 1
      if %rank% == 1
         set rank Two
      elseif %rank% == 2
         set rank Three
      elseif %rank% == 3
         set rank Four
      elseif %rank% == 4
         set rank Five
      elseif %rank% == 5
         set rank Six
      elseif %rank% == 6
         set rank Seven
      elseif %rank% == 7
         set rank Eight
      elseif %rank% == 8
         set rank Nine
      elseif %rank% == 9
         set rank Ten
      elseif %rank% == 10
         set rank Jack
      elseif %rank% == 11
         set rank Queen
      elseif %rank% == 12
         set rank King
      elseif %rank% == 13
         set rank Ace
      endif
      if %suit% == 1
         set suit Clubs
      elseif %suit% == 2
         set suit Diamonds
      elseif %suit% == 3
         set suit Spades
      elseif %suit% == 4
         set suit Hearts
      endif
      set name%card% %rank% of %suit%
      global name%card%
      eval card %card% + 1
   done
   set count 0
   set plyr 1
   while %plyr% <= 4
      set cnt 1
      while %cnt% <= 13
         set dlt 0
         set dlt_cnt 0
         while !%dlt%
            eval dlt_cnt %dlt_cnt% + 1
            set card %random.52%
            set desc X%card%X
            if !(%cards% /= %desc%)
               set card%card% %plyr%
               global card%card%
               set cards %cards%%desc%
               set dlt 1
            endif
         done
         while !%dlt%
            eval dlt_cnt %dlt_cnt% + 1
            set card %random.52%
            set desc X%card%X
            if !(%cards% /= %desc%)
               set card%card% %plyr%
               global card%card%
               set cards %cards%%desc%
               set dlt 1
            endif
         done
         eval cnt %cnt% + 1
      done
      if %cnt% < 13
         osend %actor% Dealing of cards failed; please attempt again.
         halt
      endif
      eval plyr %plyr% + 1
   done
   if %card1% == 1
      set turn 1
      osend %player1% It's your turn first; you must play the %name1%.
   elseif %card1% == 2
      set turn 2
      osend %player2% It's your turn first; you must play the %name1%.
   elseif %card1% == 3
      set turn 3
      osend %player3% It's your turn first; you must play the %name1%.
   elseif %card1% == 4
      set turn 4
      osend %player4% It's your turn first; you must play the %name1%.
   endif
   global turn
   set first_turn 1
   global first_turn
   set status 3
   global status
else
   if (%player1% && (%player1.name% == %actor.name%)) || (%player2% && (%player2.name% == %actor.name%)) || (%player3% && (%player3.name% == %actor.name%))
      osend %actor% You can't deal until four people have joined!      
   else
      return 0
   endif
endif
~
#6078
**UNUSED**~
1 c 4
ha~
return 0
~
#6079
hearts hand view~
1 c 4
hand~
switch %cmd%
  case h
  case ha
    return 0
    halt
done
if %player1% && (%actor.name% == %player1.name%)
   set plyr 1
elseif %player2% && (%actor.name% == %player2.name%)
   set plyr 2
elseif %player3% && (%actor.name% == %player3.name%)
   set plyr 3
elseif %player4% && (%actor.name% == %player4.name%)
   set plyr 4
else
   return 0
   halt
endif
if %status% < 2
   osend %actor% The game hasn't started yet!  You don't have any cards.
   halt
elseif %status% > 3
   osend %actor% The game is over already!
   halt
endif
set count 1
osend %actor% Your hand contains:
if %card1% == %plyr%
   osend %actor% &0 %count%. %name1%
   eval count %count% + 1
endif
if %card2% == %plyr%
   osend %actor% &0 %count%. %name2%
   eval count %count% + 1
endif
if %card3% == %plyr%
   osend %actor% &0 %count%. %name3%
   eval count %count% + 1
endif
if %card4% == %plyr%
   osend %actor% &0 %count%. %name4%
   eval count %count% + 1
endif
if %card5% == %plyr%
   osend %actor% &0 %count%. %name5%
   eval count %count% + 1
endif
if %card6% == %plyr%
   osend %actor% &0 %count%. %name6%
   eval count %count% + 1
endif
if %card7% == %plyr%
   osend %actor% &0 %count%. %name7%
   eval count %count% + 1
endif
if %card8% == %plyr%
   osend %actor% &0 %count%. %name8%
   eval count %count% + 1
endif
if %card9% == %plyr%
   osend %actor% &0 %count%. %name9%
   eval count %count% + 1
endif
if %card10% == %plyr%
   osend %actor% &0 %count%. %name10%
   eval count %count% + 1
endif
if %card11% == %plyr%
   osend %actor% &0 %count%. %name11%
   eval count %count% + 1
endif
if %card12% == %plyr%
   osend %actor% &0 %count%. %name12%
   eval count %count% + 1
endif
if %card13% == %plyr%
   osend %actor% &0 %count%. %name13%
   eval count %count% + 1
endif
if %card14% == %plyr%
   osend %actor% &0 %count%. %name14%
   eval count %count% + 1
endif
if %card15% == %plyr%
   osend %actor% &0 %count%. %name15%
   eval count %count% + 1
endif
if %card16% == %plyr%
   osend %actor% &0 %count%. %name16%
   eval count %count% + 1
endif
if %card17% == %plyr%
   osend %actor% &0 %count%. %name17%
   eval count %count% + 1
endif
if %card18% == %plyr%
   osend %actor% &0 %count%. %name18%
   eval count %count% + 1
endif
if %card19% == %plyr%
   osend %actor% &0 %count%. %name19%
   eval count %count% + 1
endif
if %card20% == %plyr%
   osend %actor% &0 %count%. %name20%
   eval count %count% + 1
endif
if %card21% == %plyr%
   osend %actor% &0 %count%. %name21%
   eval count %count% + 1
endif
if %card22% == %plyr%
   osend %actor% &0 %count%. %name22%
   eval count %count% + 1
endif
if %card23% == %plyr%
   osend %actor% &0 %count%. %name23%
   eval count %count% + 1
endif
if %card24% == %plyr%
   osend %actor% &0 %count%. %name24%
   eval count %count% + 1
endif
if %card25% == %plyr%
   osend %actor% &0 %count%. %name25%
   eval count %count% + 1
endif
if %card26% == %plyr%
   osend %actor% &0 %count%. %name26%
   eval count %count% + 1
endif
if %card27% == %plyr%
   osend %actor% &0 %count%. %name27%
   eval count %count% + 1
endif
if %card28% == %plyr%
   osend %actor% &0 %count%. %name28%
   eval count %count% + 1
endif
if %card29% == %plyr%
   osend %actor% &0 %count%. %name29%
   eval count %count% + 1
endif
if %card30% == %plyr%
   osend %actor% &0 %count%. %name30%
   eval count %count% + 1
endif
if %card31% == %plyr%
   osend %actor% &0 %count%. %name31%
   eval count %count% + 1
endif
if %card32% == %plyr%
   osend %actor% &0 %count%. %name32%
   eval count %count% + 1
endif
if %card33% == %plyr%
   osend %actor% &0 %count%. %name33%
   eval count %count% + 1
endif
if %card34% == %plyr%
   osend %actor% &0 %count%. %name34%
   eval count %count% + 1
endif
if %card35% == %plyr%
   osend %actor% &0 %count%. %name35%
   eval count %count% + 1
endif
if %card36% == %plyr%
   osend %actor% &0 %count%. %name36%
   eval count %count% + 1
endif
if %card37% == %plyr%
   osend %actor% &0 %count%. %name37%
   eval count %count% + 1
endif
if %card38% == %plyr%
   osend %actor% &0 %count%. %name38%
   eval count %count% + 1
endif
if %card39% == %plyr%
   osend %actor% &0 %count%. %name39%
   eval count %count% + 1
endif
if %card40% == %plyr%
   osend %actor% &0 %count%. %name40%
   eval count %count% + 1
endif
if %card41% == %plyr%
   osend %actor% &0 %count%. %name41%
   eval count %count% + 1
endif
if %card42% == %plyr%
   osend %actor% &0 %count%. %name42%
   eval count %count% + 1
endif
if %card43% == %plyr%
   osend %actor% &0 %count%. %name43%
   eval count %count% + 1
endif
if %card44% == %plyr%
   osend %actor% &0 %count%. %name44%
   eval count %count% + 1
endif
if %card45% == %plyr%
   osend %actor% &0 %count%. %name45%
   eval count %count% + 1
endif
if %card46% == %plyr%
   osend %actor% &0 %count%. %name46%
   eval count %count% + 1
endif
if %card47% == %plyr%
   osend %actor% &0 %count%. %name47%
   eval count %count% + 1
endif
if %card48% == %plyr%
   osend %actor% &0 %count%. %name48%
   eval count %count% + 1
endif
if %card49% == %plyr%
   osend %actor% &0 %count%. %name49%
   eval count %count% + 1
endif
if %card50% == %plyr%
   osend %actor% &0 %count%. %name50%
   eval count %count% + 1
endif
if %card51% == %plyr%
   osend %actor% &0 %count%. %name51%
   eval count %count% + 1
endif
if %card52% == %plyr%
   osend %actor% &0 %count%. %name52%
   eval count %count% + 1
endif
~
#6080
hearts look table~
1 c 4
look~
if (%arg% == table) || (%arg% == hearts)
   if %status% < 3
      osend %actor% The deck of cards is undealt.
      halt
   endif
   if %status% > 3
      osend %actor% The table is empty.
      halt
   endif
   if %trick% == 1
      osend %actor% No cards have been played yet this trick.
      halt
   endif
   osend %actor% The following cards have been played this trick:
   if %card1% == 9
      osend %actor% &0  %name1%
      if %trick1% == 1
         set leader %name1%
      endif
   endif
   if %card2% == 9
      osend %actor% &0  %name2%
      if %trick1% == 2
         set leader %name2%
      endif
   endif
   if %card3% == 9
      osend %actor% &0  %name3%
      if %trick1% == 3
         set leader %name3%
      endif
   endif
   if %card4% == 9
      osend %actor% &0  %name4%
      if %trick1% == 4
         set leader %name4%
      endif
   endif
   if %card5% == 9
      osend %actor% &0  %name5%
      if %trick1% == 5
         set leader %name5%
      endif
   endif
   if %card6% == 9
      osend %actor% &0  %name6%
      if %trick1% == 6
         set leader %name6%
      endif
   endif
   if %card7% == 9
      osend %actor% &0  %name7%
      if %trick1% == 7
         set leader %name7%
      endif
   endif
   if %card8% == 9
      osend %actor% &0  %name8%
      if %trick1% == 8
         set leader %name8%
      endif
   endif
   if %card9% == 9
      osend %actor% &0  %name9%
      if %trick1% == 9
         set leader %name9%
      endif
   endif
   if %card10% == 9
      osend %actor% &0  %name10%
      if %trick1% == 10
         set leader %name10%
      endif
   endif
   if %card11% == 9
      osend %actor% &0  %name11%
      if %trick1% == 11
         set leader %name12%
      endif
   endif
   if %card12% == 9
      osend %actor% &0  %name12%
      if %trick1% == 12
         set leader %name11%
      endif
   endif
   if %card13% == 9
      osend %actor% &0  %name13%
      if %trick1% == 13
         set leader %name13%
      endif
   endif
   if %card14% == 9
      osend %actor% &0  %name14%
      if %trick1% == 14
         set leader %name14%
      endif
   endif
   if %card15% == 9
      osend %actor% &0  %name15%
      if %trick1% == 15
         set leader %name15%
      endif
   endif
   if %card16% == 9
      osend %actor% &0  %name16%
      if %trick1% == 16
         set leader %name16%
      endif
   endif
   if %card17% == 9
      osend %actor% &0  %name17%
      if %trick1% == 17
         set leader %name17%
      endif
   endif
   if %card18% == 9
      osend %actor% &0  %name18%
      if %trick1% == 18
         set leader %name18%
      endif
   endif
   if %card19% == 9
      osend %actor% &0  %name19%
      if %trick1% == 19
         set leader %name19%
      endif
   endif
   if %card20% == 9
      osend %actor% &0  %name20%
      if %trick1% == 20
         set leader %name20%
      endif
   endif
   if %card21% == 9
      osend %actor% &0  %name21%
      if %trick1% == 21
         set leader %name21%
      endif
   endif
   if %card22% == 9
      osend %actor% &0  %name22%
      if %trick1% == 22
         set leader %name22%
      endif
   endif
   if %card23% == 9
      osend %actor% &0  %name23%
      if %trick1% == 23
         set leader %name23%
      endif
   endif
   if %card24% == 9
      osend %actor% &0  %name24%
      if %trick1% == 24
         set leader %name24%
      endif
   endif
   if %card25% == 9
      osend %actor% &0  %name25%
      if %trick1% == 25
         set leader %name25%
      endif
   endif
   if %card26% == 9
      osend %actor% &0  %name26%
      if %trick1% == 26
         set leader %name26%
      endif
   endif
   if %card27% == 9
      osend %actor% &0  %name27%
      if %trick1% == 27
         set leader %name27%
      endif
   endif
   if %card28% == 9
      osend %actor% &0  %name28%
      if %trick1% == 28
         set leader %name28%
      endif
   endif
   if %card29% == 9
      osend %actor% &0  %name29%
      if %trick1% == 29
         set leader %name29%
      endif
   endif
   if %card30% == 9
      osend %actor% &0  %name30%
      if %trick1% == 30
         set leader %name30%
      endif
   endif
   if %card31% == 9
      osend %actor% &0  %name31%
      if %trick1% == 31
         set leader %name31%
      endif
   endif
   if %card32% == 9
      osend %actor% &0  %name32%
      if %trick1% == 32
         set leader %name32%
      endif
   endif
   if %card33% == 9
      osend %actor% &0  %name33%
      if %trick1% == 33
         set leader %name33%
      endif
   endif
   if %card34% == 9
      osend %actor% &0  %name34%
      if %trick1% == 34
         set leader %name34%
      endif
   endif
   if %card35% == 9
      osend %actor% &0  %name35%
      if %trick1% == 35
         set leader %name35%
      endif
   endif
   if %card36% == 9
      osend %actor% &0  %name36%
      if %trick1% == 36
         set leader %name36%
      endif
   endif
   if %card37% == 9
      osend %actor% &0  %name37%
      if %trick1% == 37
         set leader %name37%
      endif
   endif
   if %card38% == 9
      osend %actor% &0  %name38%
      if %trick1% == 38
         set leader %name38%
      endif
   endif
   if %card39% == 9
      osend %actor% &0  %name39%
      if %trick1% == 39
         set leader %name39%
      endif
   endif
   if %card40% == 9
      osend %actor% &0  %name40%
      if %trick1% == 40
         set leader %name40%
      endif
   endif
   if %card41% == 9
      osend %actor% &0  %name41%
      if %trick1% == 41
         set leader %name41%
      endif
   endif
   if %card42% == 9
      osend %actor% &0  %name42%
      if %trick1% == 42
         set leader %name42%
      endif
   endif
   if %card43% == 9
      osend %actor% &0  %name43%
      if %trick1% == 43
         set leader %name43%
      endif
   endif
   if %card44% == 9
      osend %actor% &0  %name44%
      if %trick1% == 44
         set leader %name44%
      endif
   endif
   if %card45% == 9
      osend %actor% &0  %name45%
      if %trick1% == 45
         set leader %name45%
      endif
   endif
   if %card46% == 9
      osend %actor% &0  %name46%
      if %trick1% == 46
         set leader %name46%
      endif
   endif
   if %card47% == 9
      osend %actor% &0  %name47%
      if %trick1% == 47
         set leader %name47%
      endif
   endif
   if %card48% == 9
      osend %actor% &0  %name48%
      if %trick1% == 48
         set leader %name48%
      endif
   endif
   if %card49% == 9
      osend %actor% &0  %name49%
      if %trick1% == 49
         set leader %name49%
      endif
   endif
   if %card50% == 9
      osend %actor% &0  %name50%
      if %trick1% == 50
         set leader %name50%
      endif
   endif
   if %card51% == 9
      osend %actor% &0  %name51%
      if %trick1% == 51
         set leader %name51%
      endif
   endif
   if %card52% == 9
      osend %actor% &0  %name52%
      if %trick1% == 52
         set leader %name52%
      endif
   endif
   osend %actor% The trick was led by the %leader%.
   if %turn% == 1
      osend %actor% It is %player1.name%'s turn.
   elseif %turn% == 2
      osend %actor% It is %player2.name%'s turn.
   elseif %turn% == 3
      osend %actor% It is %player3.name%'s turn.
   elseif %turn% == 4
      osend %actor% It is %player4.name%'s turn.
   endif
else
   return 0
endif
~
#6081
**UNUSED**~
1 c 4
p~
return 0
~
#6082
hearts play~
1 c 4
play~
switch %cmd%
  case p
  case pl
  case pla
    return 0
    halt
done
if %player1% && (%actor.name% == %player1.name%)
   set plyr 1
elseif %player2% && (%actor.name% == %player2.name%)
   set plyr 2
elseif %player3% && (%actor.name% == %player3.name%)
   set plyr 3
elseif %player4% && (%actor.name% == %player4.name%)
   set plyr 4
else
   return 0
   halt
endif
if %status% < 3
   osend %actor% The game hasn't started yet!
   halt
elseif %status% > 3
   osend %actor% The game is over already!
   halt
endif
if %turn% != %plyr%
   osend %actor% It's not your turn.
   halt
endif
if (!%arg%) || (%arg% < -1) || (%arg% > 13)
   osend %actor% Play what!?
   halt
endif
if %first_turn%
   if %arg% == 1
      unset first_turn
   else
      osend %actor% You must start the game by playing the %name1%!
      halt
   endif
endif
set count 1
if %card1% == %plyr%
   if %count% == %arg%
      set card 1
      set name %name1%
   endif
   eval count %count% + 1
endif
if %card2% == %plyr%
   if %count% == %arg%
      set card 2
      set name %name2%
   endif
   eval count %count% + 1
endif
if %card3% == %plyr%
   if %count% == %arg%
      set card 3
      set name %name3%
   endif
   eval count %count% + 1
endif
if %card4% == %plyr%
   if %count% == %arg%
      set card 4
      set name %name4%
   endif
   eval count %count% + 1
endif
if %card5% == %plyr%
   if %count% == %arg%
      set card 5
      set name %name5%
   endif
   eval count %count% + 1
endif
if %card6% == %plyr%
   if %count% == %arg%
      set card 6
      set name %name6%
   endif
   eval count %count% + 1
endif
if %card7% == %plyr%
   if %count% == %arg%
      set card 7
      set name %name7%
   endif
   eval count %count% + 1
endif
if %card8% == %plyr%
   if %count% == %arg%
      set card 8
      set name %name8%
   endif
   eval count %count% + 1
endif
if %card9% == %plyr%
   if %count% == %arg%
      set card 9
      set name %name9%
   endif
   eval count %count% + 1
endif
if %card10% == %plyr%
   if %count% == %arg%
      set card 10
      set name %name10%
   endif
   eval count %count% + 1
endif
if %card11% == %plyr%
   if %count% == %arg%
      set card 11
      set name %name11%
   endif
   eval count %count% + 1
endif
if %card12% == %plyr%
   if %count% == %arg%
      set card 12
      set name %name12%
   endif
   eval count %count% + 1
endif
if %card13% == %plyr%
   if %count% == %arg%
      set card 13
      set name %name13%
   endif
   eval count %count% + 1
endif
if %card14% == %plyr%
   if %count% == %arg%
      set card 14
      set name %name14%
   endif
   eval count %count% + 1
endif
if %card15% == %plyr%
   if %count% == %arg%
      set card 15
      set name %name15%
   endif
   eval count %count% + 1
endif
if %card16% == %plyr%
   if %count% == %arg%
      set card 16
      set name %name16%
   endif
   eval count %count% + 1
endif
if %card17% == %plyr%
   if %count% == %arg%
      set card 17
      set name %name17%
   endif
   eval count %count% + 1
endif
if %card18% == %plyr%
   if %count% == %arg%
      set card 18
      set name %name18%
   endif
   eval count %count% + 1
endif
if %card19% == %plyr%
   if %count% == %arg%
      set card 19
      set name %name19%
   endif
   eval count %count% + 1
endif
if %card20% == %plyr%
   if %count% == %arg%
      set card 20
      set name %name20%
   endif
   eval count %count% + 1
endif
if %card21% == %plyr%
   if %count% == %arg%
      set card 21
      set name %name21%
   endif
   eval count %count% + 1
endif
if %card22% == %plyr%
   if %count% == %arg%
      set card 22
      set name %name22%
   endif
   eval count %count% + 1
endif
if %card23% == %plyr%
   if %count% == %arg%
      set card 23
      set name %name23%
   endif
   eval count %count% + 1
endif
if %card24% == %plyr%
   if %count% == %arg%
      set card 24
      set name %name24%
   endif
   eval count %count% + 1
endif
if %card25% == %plyr%
   if %count% == %arg%
      set card 25
      set name %name25%
   endif
   eval count %count% + 1
endif
if %card26% == %plyr%
   if %count% == %arg%
      set card 26
      set name %name26%
   endif
   eval count %count% + 1
endif
if %card27% == %plyr%
   if %count% == %arg%
      set card 27
      set name %name27%
   endif
   eval count %count% + 1
endif
if %card28% == %plyr%
   if %count% == %arg%
      set card 28
      set name %name28%
   endif
   eval count %count% + 1
endif
if %card29% == %plyr%
   if %count% == %arg%
      set card 29
      set name %name29%
   endif
   eval count %count% + 1
endif
if %card30% == %plyr%
   if %count% == %arg%
      set card 30
      set name %name30%
   endif
   eval count %count% + 1
endif
if %card32% == %plyr%
   if %count% == %arg%
      set card 31
      set name %name31%
   endif
   eval count %count% + 1
endif
if %card32% == %plyr%
   if %count% == %arg%
      set card 32
      set name %name32%
   endif
   eval count %count% + 1
endif
if %card33% == %plyr%
   if %count% == %arg%
      set card 33
      set name %name33%
   endif
   eval count %count% + 1
endif
if %card34% == %plyr%
   if %count% == %arg%
      set card 34
      set name %name34%
   endif
   eval count %count% + 1
endif
if %card35% == %plyr%
   if %count% == %arg%
      set card 35
      set name %name35%
   endif
   eval count %count% + 1
endif
if %card36% == %plyr%
   if %count% == %arg%
      set card 36
      set name %name36%
   endif
   eval count %count% + 1
endif
if %card37% == %plyr%
   if %count% == %arg%
      set card 37
      set name %name37%
   endif
   eval count %count% + 1
endif
if %card38% == %plyr%
   if %count% == %arg%
      set card 38
      set name %name38%
   endif
   eval count %count% + 1
endif
if %card39% == %plyr%
   if %count% == %arg%
      set card 39
      set name %name39%
   endif
   eval count %count% + 1
endif
if %card40% == %plyr%
   if %count% == %arg%
      set card 40
      set name %name40%
   endif
   eval count %count% + 1
endif
if %card41% == %plyr%
   if %count% == %arg%
      set card 41
      set name %name41%
   endif
   eval count %count% + 1
endif
if %card42% == %plyr%
   if %count% == %arg%
      set card 42
      set name %name42%
   endif
   eval count %count% + 1
endif
if %card43% == %plyr%
   if %count% == %arg%
      set card 43
      set name %name43%
   endif
   eval count %count% + 1
endif
if %card44% == %plyr%
   if %count% == %arg%
      set card 44
      set name %name44%
   endif
   eval count %count% + 1
endif
if %card45% == %plyr%
   if %count% == %arg%
      set card 45
      set name %name45%
   endif
   eval count %count% + 1
endif
if %card46% == %plyr%
   if %count% == %arg%
      set card 46
      set name %name46%
   endif
   eval count %count% + 1
endif
if %card47% == %plyr%
   if %count% == %arg%
      set card 47
      set name %name47%
   endif
   eval count %count% + 1
endif
if %card48% == %plyr%
   if %count% == %arg%
      set card 48
      set name %name48%
   endif
   eval count %count% + 1
endif
if %card49% == %plyr%
   if %count% == %arg%
      set card 49
      set name %name49%
   endif
   eval count %count% + 1
endif
if %card50% == %plyr%
   if %count% == %arg%
      set card 50
      set name %name50%
   endif
   eval count %count% + 1
endif
if %card51% == %plyr%
   if %count% == %arg%
      set card 51
      set name %name51%
   endif
   eval count %count% + 1
endif
if %card52% == %plyr%
   if %count% == %arg%
      set card 52
      set name %name52%
   endif
   eval count %count% + 1
endif
if %arg% >= %count%
   osend %actor% You don't have that many cards!
   halt
endif
if !%trick%
   set trick 1
elseif %trick% > 1
   set lead_suit (%trick1% - 1) / 13
   set card_suit (%card% - 1) / 13
   if %card_suit% != %lead_suit%
      if %lead_suit% == 0 && (%card1% == %plyr% || %card2% == %plyr% || %card3% == %plyr% || %card4% == %plyr% || %card5% == %plyr% || %card6% == %plyr% || %card7% == %plyr% || %card8% == %plyr% || %card9% == %plyr% || %card10% == %plyr%)
         set no 1
      elseif %lead_suit% == 0 && (%card11% == %plyr% || %card12% == %plyr% || %card13% == %plyr%)
         set no 1
      elseif %lead_suit% == 1 && (%card14% == %plyr% || %card15% == %plyr% || %card16% == %plyr% || %card17% == %plyr% || %card18% == %plyr% || %card19% == %plyr% || %card20% == %plyr% || %card21% == %plyr% || %card22% == %plyr%)
         set no 1
      elseif %lead_suit% == 1 && (%card23% == %plyr% || %card24% == %plyr% || %card25% == %plyr% || %card26% == %plyr%)
         set no 1
      elseif %lead_suit% == 2 && (%card27% == %plyr% || %card28% == %plyr% || %card29% == %plyr% || %card30% == %plyr% || %card31% == %plyr% || %card32% == %plyr% || %card33% == %plyr% || %card34% == %plyr% || %card35% == %plyr%)
         set no 1
      elseif %lead_suit% == 2 && (%card36% == %plyr% || %card37% == %plyr% || %card38% == %plyr% || %card39% == %plyr%)
         set no 1
      elseif %lead_suit% == 3 && (%card40% == %plyr% || %card41% == %plyr% || %card42% == %plyr% || %card43% == %plyr% || %card44% == %plyr% || %card45% == %plyr% || %card46% == %plyr% || %card47% == %plyr% || %card48% == %plyr%)
         set no 1
      elseif %lead_suit% == 3 && (%card49% == %plyr% || %card50% == %plyr% || %card51% == %plyr% || %card52% == %plyr%)
         set no 1
      endif
      if %no%
         osend %actor% You must follow suit!
         halt
      endif
   endif
endif
set card%card% 9
global card%card%
set trick%trick% %card%
global trick%trick%
osend %actor% You play the %name%.
oechoaround %actor% %actor.name% plays the %name%.
eval trick %trick% + 1
eval played %played% + 1
global played
eval turn %turn% + 1
if %turn% == 5
   set turn 1
endif
if %trick% == 5
   set lead_suit (%trick1% - 1) / 13
   set top_rank (%trick1% - (13 * ((%trick1% - 1) / 13))
   set taker %turn%
   if ((%trick2% - 1) / 13) == %lead_suit%
      set rank %trick2% - (13 * ((%trick2% - 1) / 13))
      if %rank% > %top_rank%
         set taker %turn% + 1
         set top_rank %rank%
      endif
   endif
   if ((%trick3% - 1) / 13) == %lead_suit%
      set rank %trick3% - (13 * ((%trick3% - 1) / 13))
      if %rank% > %top_rank%
         set taker %turn% + 2
         set top_rank %rank%
      endif
   endif
   if ((%trick4% - 1) / 13) == %lead_suit%
      set rank %trick4% - (13 * ((%trick4% - 1) / 13))
      if %rank% > %top_rank%
         set taker %turn% + 3
      endif
   endif
   if %taker% > 4
      eval %taker% - 4
   endif
   eval card%trick1% %taker% + 4
   eval card%trick2% %taker% + 4
   eval card%trick3% %taker% + 4
   eval card%trick4% %taker% + 4
   global card%trick1%
   global card%trick2%
   global card%trick3%
   global card%trick4%
   if %taker% == 1
      osend %player1% You take the trick.
      oechoaround %player1% %player1.name% takes the trick.
   elseif %taker% == 2
      osend %player2% You take the trick.
      oechoaround %player2% %player2.name% takes the trick.
   elseif %taker% == 3
      osend %player3% You take the trick.
      oechoaround %player3% %player3.name% takes the trick.
   elseif %taker% == 4
      osend %player4% You take the trick.
      oechoaround %player4% %player4.name% takes the trick.
   endif
   set turn %taker%
   set trick 1
   unset trick1
   unset trick2
   unset trick3
   unset trick4
endif
global turn
global trick
if %played% == 52
   set status 4
   global status
   oforce %actor% endgame
   halt
endif
if %turn% == 1
   osend %player1% It's now your turn.
   oechoaround %player1% It's now %player1.name%'s turn.
elseif %turn% == 2
   osend %player2% It's now your turn.
   oechoaround %player2% It's now %player2.name%'s turn.
elseif %turn% == 3
   osend %player3% It's now your turn.
   oechoaround %player3% It's now %player3.name%'s turn.
elseif %turn% == 4
   osend %player4% It's now your turn.
   oechoaround %player4% It's now %player4.name%'s turn.
endif
~
#6083
hearts endgame~
1 c 4
endgame~
switch %cmd%
  case e
  case en
    return 0
    halt
done
if %status% == 4
   set plyr 1
   while %plyr% <= 4
      if %card37% == %plyr% + 4
         set score 13
      endif
      if %card40% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card41% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card42% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card43% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card44% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card45% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card46% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card47% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card48% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card49% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card50% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card51% == %plyr% + 4
         eval score %score% + 1
      endif
      if %card52% == %plyr% + 4
         eval score %score% + 1
      endif
      if %plyr% == 1
         set score1 %score%
      elseif %plyr% == 2
         set score2 %score%
      elseif %plyr% == 3
         set score3 %score%
      elseif %plyr% == 4
         set score4 %score%
      endif
   done
   if (%score1% == 26) || (%score2% == 26) || (%score3% == 26) || (%score4% == 26)
      if %score1% == 26
         osend %player1% You took all the point cards!  Everyone else gets 26 points.
         osend %player2% %player1.name% took all the point cards, so you get 26 points.
         osend %player3% %player1.name% took all the point cards, so you get 26 points.
         osend %player4% %player1.name% took all the point cards, so you get 26 points.
         set score1 0
         set score2 26
         set score3 26
         set score4 26
      elseif %score2% == 26
         osend %player1% %player2.name% took all the point cards, so you get 26 points.
         osend %player2% You took all the point cards!  Everyone else gets 26 points.
         osend %player3% %player2.name% took all the point cards, so you get 26 points.
         osend %player4% %player2.name% took all the point cards, so you get 26 points.
         set score1 26
         set score2 0
         set score3 26
         set score4 26
      elseif %score3% == 26
         osend %player1% %player3.name% took all the point cards, so you get 26 points.
         osend %player2% %player3.name% took all the point cards, so you get 26 points.
         osend %player3% You took all the point cards!  Everyone else gets 26 points.
         osend %player4% %player3.name% took all the point cards, so you get 26 points.
         set score1 26
         set score2 26
         set score3 0
         set score4 26
      elseif %score4% == 26
         osend %player1% %player4.name% took all the point cards, so you get 26 points.
         osend %player2% %player4.name% took all the point cards, so you get 26 points.
         osend %player3% %player4.name% took all the point cards, so you get 26 points.
         osend %player4% You took all the point cards!  Everyone else gets 26 points.
         set score1 26
         set score2 26
         set score3 26
         set score4 0
      endif
   endif
   eval total1 %total1% + %score1%
   eval total2 %total2% + %score2%
   eval total3 %total3% + %score3%
   eval total4 %total4% + %score4%
   osend %player1% You scored %score1% points this round, bringing your total to %total1%.
   osend %player2% You scored %score2% points this round, bringing your total to %total2%.
   osend %player3% You scored %score3% points this round, bringing your total to %total3%.
   osend %player4% You scored %score4% points this round, bringing your total to %total4%.
   if (%total1% > 99) || (%total2% > 99) || (%total3% > 99) || (%total4% > 99)
      oecho The game is over!
      oecho %player1.name% scored a total of %total1% points.
      oecho %player2.name% scored a total of %total2% points.
      oecho %player3.name% scored a total of %total3% points.
      oecho %player4.name% scored a total of %total4% points.
      unset player1
      unset player2
      unset player3
      unset player4
      unset status
   else
      set status 1
      global status
   endif
else
   return 0
endif
~
$~
