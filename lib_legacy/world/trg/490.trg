#49000
boulder_move~
2 a 100
~
wecho The boulder slowly starts to move back.
wdoor 49042 s room 49084
wdoor 49084 n purge
wdoor 49084 n room 49042
wait 8 s
wecho The boulder seems to be rolling slowly backward - better hurry!
wait 5 s
wecho The boulder rolls back and seals the entrance.
wdoor 49042 s purge
wdoor 49084 n flags abcd
wdoor 49084 n key 49036
wdoor 49084 n description The boulder seems to have been loosened.
wdoor 49084 n name boulder
~
#49001
test_remote_world~
1 h 100
~
opurge sheath
~
#49002
drop_sapling~
2 h 100
~
* The sapling is dropped. If Dagon is present, it will grow and destroy
* the altar, weakening him.
if %object.vnum% == 49045 && %self.people[49021]%
   wait 1 s
   wecho The sapling vibrates faster and faster and its roots start to grow!
   wecho Even as you watch, the sapling moves towards the altar and starts to grow into it.
   wpurge sapling
   wpurge griffin-altar
   wload obj 49044
   wait 1 s
   wecho CRACK!
   wait 1 s
   wecho The altar has been broken by the sapling!
   wforce dagon emote shrieks in rage and pain as the source of his power is destroyed.
   wforce dagon dagonisweaknow
end
~
#49003
dagon_is_weakened~
0 c 100
dagonisweaknow~
if %actor.vnum% == %self.vnum%
   set dagonisweak 1
   global dagonisweak
else
   return 0
endif
~
#49004
dagon_in_combat~
0 k 100
~
if %dagonisweak% != 1
   m_run_room_trig 49005
end
~
#49005
dagon_gets_healed~
2 a 100
~
wheal dagon 2000
~
#49006
death_of_dagon~
0 f 100
~
return 0
mecho Dagon utters a blood curdling scream as his demonic spirit returns to its realm.
m_run_room_trig 49007
if !(%get.mob_count[49010]%)
    mat 49190 mload mob 49010
    mat 49190 mforce adramalech mload obj 49034
endif
~
#49007
dagon_death_pt2~
2 a 100
~
set person %self.people%
while %person%
   if %person.quest_stage[griffin_quest]% == 6
      quest advance griffin_quest %person.name%
      wsend %person% &7&bYou have advanced the quest!&0
      wsend %person% &7&bProof of the deed must be delivered individually.&0
   endif
   set person %person.next_in_room%
done
~
#49008
adramalech_dies~
0 f 100
~
return 0
mecho %self.name% emits a bone-rattling roar that fades away into a low rattle.
mecho %self.name% disintegrates into darkly glowing spots that fade from view.
if %self.room% != 49190
  mteleport all 49190
  mecho &0  
  mecho &7&bA rift opens in the fabric of reality and pulls you through!&0
  mecho &0  
  set room %get.room[49190]%
  set person %room.people%
  while %person%
    if %person.vnum% == -1
      mforce %person% look
      set person %person.next_in_room%
    endif
  done
endif
unset person
set person %actor%
set i %person.group_size%
set stage 8
if %i%
   set a 1
else
   set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
      if %person.quest_stage[griffin_quest]% == %stage%
        quest advance griffin_quest %person.name%
      endif
  elseif %person% && %person.vnum% == -1
      eval i %i% + 1
  endif
  eval a %a% + 1
done
mat 49190 m_run_room_trig 49009
~
#49009
Griffin Island Quest exp rewards~
2 a 100
~
wait 1
set person %self.people%
set stage 9
while %person%
   if %person.quest_stage[griffin_quest]% == %stage%
      *
      * Set X to the level of the award - code does not run without it
      *  Griffin Isle, X = 60
      if %person.level% < 60
         set expcap %person.level%
      else
         set expcap 60
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
      * Griffin Isle, xexp = wexp
         wexp %person% %setexp%
         eval loop %loop% + 1
      done
      set gem 0
      while %gem% < 3
         eval drop %random.11% + 55736
         wload obj %drop%
         eval gem %gem% + 1
         wforce %person% get gem
      done
      quest complete griffin_quest %person.name%
   endif
   set person %person.next_in_room%
done
~
#49010
rope_ladder~
2 h 100
~
if %object.vnum% == 49041
wait 1 s
wecho The ladder unrolls and forms a rigid structure down the cliff.
wdoor 49149 d room 49028
wdoor 49028 u room 49149
wpurge rope-ladder
endif
~
#49011
whisky_trig~
0 j 100
~
wait 2
if %object.type% == LIQ CONTAINER
  if %object.val1% == 0
    say An empty container?  How generous.
  elseif %object.val2% != 5
    say Whisky is what I wanted!
  else
    say Thanks %actor.name% and here is your ladder
    mload obj 49041
    give ladder %actor.name%
  endif
endif
~
#49012
reroll_ladder~
2 g 100
~
if ( %direction% == down )
  wait 1 s
  wecho The rope ladder rolls back up.
  wload obj 49041
  wdoor 49149 down purge
  wdoor 49028 up purge
  wdoor 49028 u room 49128
endif
~
#49013
UNUSED~
0 j 100
~
if (%object.vnum% == 49060)
wait 2
say Dagon is dead and the altar is broken.
cheer
say I felt the trees breathe a sigh of relief at his passing.
think
say If you were the mighty one that killed him then
say give me his skin and I will help you to ensure his complete destruction.
mat 49083 give dagon-quest-complete griffin-quest-controller
end
~
#49015
crowley_receive1~
0 j 100
~
if %object.vnum% == 49001
   return 1
   wait 4
   emote examines the griffin skin carefully.
   mecho %self.name% says, 'This is fantastic!  You have defeated his earthly form!
   mecho &0However, this was merely a material manifestation, and his true incarnation
   mecho &0continues its nefarious existence.'
   wait 4s
   mecho %self.name% says, 'Please, I beg of you, find and destroy him, before he
   mecho &0gathers the strength to return as Dagon yet again!'
   wait 4s
   mecho %self.name% says, 'Oh!  I nearly forgot.  This belongs to you, if you can
   mecho &0bear to touch it.'
   give griffin-skin %actor.name%
   wait 2s
   mecho %self.name% says, 'If you can't bear to use it, give it to Awura who will
   mecho &0destroy it.  Maybe she will give you something more to your taste.'
elseif %object.vnum% == 49042
   return 1
   wait 8
   emote looks rather surprised.
   wait 2s
   say Well now, you found it!  How very resourceful of you!
   wait 2s
   emote peers suspiciously into %object.shortdesc%.
   wait 2s
   if %object.val1% != 16
      glare %actor.name%
      wait 2s
      mecho %self.name% says, 'Well you had to help yourself, now, didn't you!
      mecho &0You have no idea how thirs*HIC* I am!  Bah.'
      wait 2s
      drop clear-glass-bottle
   elseif %object.val2% != 5
      say What's this?  I'm not drinking that!
      glare %actor.name
      wait 2s
      drop clear-glass-bottle
   else
      emote smiles and takes a healthy swig from %object.shortdesc%.
      wait 2s
      say Thank you, %actor.name%.  You're a lifesaver!
      mjunk clear-glass-bottle
      wait 2s
      emote staggers a bit as he carefully pours the whiskey into his waterskin.
      mload obj 49066
      drop mirror-lettered-scroll
   end
else
   return 0
   wait 1
   msend %actor% &b%self.name% tells you, 'No, thank you.'&0
end
~
#49016
Earle receive branch~
0 j 100
49037~
* Small oak branch given
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
    if %person.quest_stage[griffin_quest]% == 0
      quest start griffin_quest %person.name%
      msend %person% &7&bYou have now begun the Griffin Isle quest!&0
    endif
    quest variable griffin_quest %person% oak 1
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
mjunk small-oak-branch
if !%get.mob_count[49001]%
  mat 49081 mload mob 49001
endif
mat 49081 mforce undead-captain mload obj 49037
say Ah, excellent work, %actor.name%.
emote examines the oak branch.
wait 2 s
say The centuries have taken their toll, but it is still powerful.
emote utters some incantations and the branch starts to sprout roots and leaves!
mload obj 49045
give sapling %actor.name%
wait 3 s
say This sapling needs to be placed near the altar to be effective.  May the tree spirits guard you.
if %actor.quest_stage[griffin_quest]% == 1
  wait 2 s
  say Now, did you also retrieve the sword?  Give it to me.
endif
~
#49017
crowley_speak1~
0 d 100
forget?~
wait 4
mecho %self.name% says, 'I drink to forget until I forget to drink.  The chapel
mecho &0was the most horrible place I have ever seen.  The sight of it is burned into
mecho &0my mind.'
wait 2 s
msend %actor.name% %self.name% falls to the ground before you.
mechoaround %actor.name% %self.name% falls to the ground before %actor.name%.
say I beg you to destroy that place utterly.
wait 3 s
mecho %self.name% stands and dusts off his clothes.
say May the tree spirits guard you.
~
#49018
crowley_speak2~
0 d 0
the seer requests your assistance~
wait 2
mmobflag %self% sentinel on
if %actor.quest_stage[griffin_quest]% < 3
   say Oh, does she now?  Are you certain?
   peer %actor.name%
elseif %actor.vnum% == -1
   say Oh, I had hoped this would never happen.
   hiccup
   wait 4
   mecho %self.name% says, 'I suppose she told you that I know where the cult's chapel is?'
   sigh
   wait 2s
   mecho %self.name% says, 'I rolled a boulder over the entrance and persuaded Derceta to guard it.  I used to be quite strong in my youth.'
   strut
   wait 2s
   say But drink has done away with that.
   wait 3s
   mecho %self.name% says, 'In fact, the only person who I can think of who could possibly move it would be Derceta.  I suggest you ask her to help move the boulder.'
   wait 3s
   emote takes a swig from his waterskin.
   say Ah, the drink helps me forget, but not for long.
endif
mmobflag %self% sentinel off
~
#49019
crowley_rand1~
0 b 30
~
emote shakes his empty watersack.
emote mutters something about topping up the whisky.
say I must have left the bottle at home...
~
#49020
seer_speak1~
0 d 0
Earle sends me~
wait 2
if %self.room% != 49078
  say I am powerless to help you outside my cave.
else
  switch %actor.quest_stage[griffin_quest]%
    case 0
      mecho %self.name% says, 'You say Earle sent you?  Hmm, he may have done, but there is nothing I can do for you.'
      break
    case 1
      mecho %self.name% says, 'Did you give the rune sword to Earle yet?  He has to verify that you can go through with this.'
      break
    default
      mecho %self.name% says, 'Ah... my vision is coming true.  The cult grows in strength and will soon be strong enough to summon Dagon, the powerful griffin god.'
      wait 3 s
      mecho %self.name% says, 'Seek ye that drunken lout, for he has found the path to the chapel.  Say to him, &7&b'the seer requests your assistance'&0 and he will tell you what he knows.'
      emote closers her eyes and goes into a trance.
      wait 3 s
      emote wails with fear.
      mecho %self.name% says, 'I see the future, when an evil greater than Dagon will walk the island.'
      emote opens her eyes again.
      wait 2 s
      sigh
      say There is much to do, and little time to do it.
  done
  set person %actor%
  set stage 2
  set i %person.group_size%
  if %i%
    set a 1
  else
    set a 0
  endif
  while %i% >= %a%
    set person %person.group_member[%a%]%
    if %person.room% == %self.room%
      if %person.quest_stage[griffin_quest]% == %stage%
        quest advance griffin_quest %person.name%
        msend %person% &7&bYou have advanced the quest!&0
      endif
    elseif %person% && %person.vnum% == -1
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
endif
~
#49021
seer_rand1~
0 b 50
~
emote moans and sways as she prepares to utter one of her cryptic sayings.
set rndm %random.100%
if %rndm% < 50
say The ship was carrying French wine beyond the Spanish sea.
else
if %rndm% < 75
say The ship finder?  Crazily sounds like there is about a thousand.
else
say Always wear sunscreen.
endif
endif
~
#49022
Seer refuse~
0 j 100
~
switch %object.vnum%
  case %wandgem%
  case %wandtask3%
  case %wandtask4%
  case %wandvnum%
    halt
    break
  case 2329
  case 23753
  case 48005
    if %actor.quest_stage[wizard_eye]% == 4
      halt
    else
      set response This isn't sunscreen, what use do I have for it?
    endif
    break
  default
    if %actor.quest_stage[wizard_eye]% == 4
      set response This isn't a dress, bay, or thyme!  I don't have thyme for this!
      set action cackle
    else
      set response This isn't sunscreen, what use do I have for it?
    endif
done
if %response%
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  msend %actor% %self.name% says, '%response%'
  if %action%
    %action%
  endif
endif
~
#49023
derceta_receive1~
0 j 100
~
if %object.vnum% == 49016
  mmobflag %self% sentinel on
  set person %actor%
  set stage 3
  set i %person.group_size%
  if %i%
    set a 1
  else
    set a 0
  endif
  while %i% >= %a%
    set person %person.group_member[%a%]%
    if %person.room% == %self.room%
      if %person.quest_stage[griffin_quest]% == %stage%
        quest advance griffin_quest %person.name%
        msend %person% &7&bYou have advanced the quest!&0
      endif
      if %actor.quest_stage[griffin_quest]% < %stage%
        set leader %person%
      else
        set leader %actor%
      endif
    elseif %person% && %person.vnum% == -1
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
  wait 2
  smile
  say Thanks %leader.name%.  Now, lead me to the boulder.
  wait 1s
  mecho %self.name% says, 'When you want it shifted, say &7&b'push now'&0 and I will get to work.'
  follow %leader.name%
  wait 4
  say Be ready to move quick - I won't be able to hold it forever.
  emote polishes the earring.
  emote peers at the earring and says 'Hello little midget.'
  wait 8
  wear earring ear
else
  return 0
  shake
  msend %actor% %self.name% returns %object.shortdesc% to you.
endif
~
#49024
can_go_to_adramalech~
2 c 100
enter~
switch %cmd%
  case e
    return 0
    halt
done
if %actor.quest_stage[griffin_quest]% > 7
  return 0
else
  return 1
  wechoaround %actor% %actor.name% hops into the pool, but gets %actor.p% feet wet.
  wsend %actor% You hop into the pool, but only get your feet wet.
endif
~
#49025
derceta_speak1~
0 d 100
rock boulder~
wait 2
switch %actor.quest_stage[griffin_quest]%
   case 0
   case 1
      ponder %actor.name%
      mecho %self.name% says, 'I'm not sure you're ready to face the cultists.  Speak to Earle
      mecho &0first.
      break
   case 2
      mecho %self.name% says, 'Have you spoken to the seer?  You cannot hope to prevail
      mecho &0otherwise.'
      break
   case 3
      peer %actor.name%
      emote flexes her mighty muscles.
      wait 2 s
      mecho %self.name% says, 'I can push any boulder you choose, but first you must do something
      mecho &0for me.'
      wait 2 s
      mecho %self.name% says, 'Get me back my crystal midget that that thief Derrick stole, and I
      mecho &0will help you.'
      break
   default
      mmobflag %self% sentinel on
      mecho %self.name% says, 'Certainly, just lead me to the rock and say &7&b'push now'&0.'
      follow %actor.name%
      break
done
~
#49026
derceta_speak2~
0 d 100
push now~
set person %actor%
set location 49042
set stage 4
set i %person.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_stage[griffin_quest]% >= %stage%
      set accept yes
      if %self.room% == %location%
        set level yes
        if %person.quest_stage[griffin_quest]% == %stage%
          quest advance griffin_quest %person.name%
          msend %person% &7&bYou have advanced the quest!&0
        endif
      endif
    elseif %person% && %person.vnum% == -1
      eval i %i% + 1
    endif
  endif
  eval a %a% + 1
done
wait 2
if %level%
  if %accept%
    emote spits on her hands and starts to move the boulder.
    say Phew this is one heavy rock!
    m_run_room_trig 49000
    mmobflag %self% sentinel off
    fol self
  else
    say What?  The boulder isn't here!
  endif
else !%level%
  peer %actor.name%
  say And why would I do a thing like that?
endif   
~
#49027
Earle receive sword~
0 j 100
49029~
* Rune sword given
return 0
mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
msend %actor% You give %object.shortdesc% to %self.name%.
set person %actor%
set oak 0
set accept 0
set stage 1
set i %person.group_size%
if %i%
  set a 1
else
  set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
    if %person.quest_variable[griffin_quest:oak]%
      eval oak %oak% + 1
      if %person.quest_stage[griffin_quest]% == %stage%
        quest advance griffin_quest %person.name%
        msend %person% &7&bYou have advanced the quest!&0
        eval accept %accept% + 1
      endif
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
wait 2
if !%oak%
  say A fine weapon, but it will be of no use without the holy tree.
  emote returns the sword.
else
  say Ah... this sword would have saved a lot of trouble.
  ponder
  wait 2 s
  mecho %self.name% says, 'If you didn't have too much trouble getting this sword, then perhaps you can help.  Go to the seer and say &6&b"Earle sends me"&0 and she will give you what aid she can.'
  wait 3 s
  say Please return to me when you have defeated Dagon.  And be sure to bring proof of the deed!
  emote hands back the sword.
  wait 2s
  if %accept%
    mecho %self.name% says, 'If you need, you can come to me and check your &6&b[quest progress]&0.'
  else
    emote glances back with a thoughtful look.
    wait 1 s
    say Did I give you the sapling for the altar?  I can be so forgetful...'
  end
end
~
#49028
Earle receive skin~
0 j 100
49001~
* griffin skin given
return 0
mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
msend %actor% You give %object.shortdesc% to %self.name%.
wait 2
emote grimaces a bit as he accepts %object.shortdesc%.
wait 4
say An ugly thing, is it not?  It must be utterly destroyed, for it carries Dagon's taint and stench wherever it goes.
wait 8
say Take it to Awura.  She will know how to deal with it.
emote lifts %object.shortdesc% on one finger, and hands it back.
msend %actor% &7&bYou have advanced the quest!&0
msend %actor% &7&bGroup credit will not be given when delivering the skin to Awura.&0
~
#49029
Earle receive sapling~
0 j 100
49045~
* sapling given back
return 0
wait 4
say Keep it, %actor.name%!  You'll need to drop it by the altar.  Otherwise you will never defeat Dagon's material form!
wait 2s
emote returns %object.shortdesc%.
~
#49030
Earle refuse~
0 j 0
49001 49029 49037 49045~
switch %object.vnum%
  case 53451
    if %actor.quest_stage[major_globe_spell]% == 2
      halt
    endif
    break
  case 58002
    if %actor.quest_stage[major_globe_spell]% == 3
      halt
    endif
    break
  case 58609
    if %actor.quest_stage[major_globe_spell]% == 4
      halt
    endif
done
return 0
mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
msend %actor% You give %object.shortdesc% to %self.name%.
wait 4
shake
say Sorry, I don't think this will be of any use.
wait 8
emote returns %object.shortdesc%.
~
#49031
eldest_speak1~
0 d 1
cult? cult cult ?~
mecho %self.name% says, 'You don't know about the cult?  You need to read some
mecho &0history.'
mload obj 49038
give book %actor.name%
say Come back when you can help us.
~
#49032
eldest_speak2~
0 d 1
griffin? griffins? griffins griffin griffins ? griffin ?~
peer %actor.name%
if %actor.level% < 45
  mecho %self.name% says, 'Hehe, thanks for asking, but I really don't think you
  mecho &0can help %actor.name%.
  ruffle %actor.name%
  wait 1
  mecho %self.name% says, 'But find me a powerful hero and maybe the cult can be
  mecho &0stopped.'
else
  mecho %self.name% says, 'Yes, you may have noticed a lot of griffins around this
  mecho &0island, well their nature has brought an imbalance to this place, and our holy
  mecho &0mistletoe is dying.'
  wait 1
  mecho %self.name% says, 'They distract my druids from tending to their trees
  mecho &0too.  We must stop the cult of griffin worshippers before it is too late.
endif
~
#49033
eldest_greet1~
0 g 100
~
if %actor.vnum% == -1 && %actor.level% < 100
  wait 1
  switch %actor.quest_stage[major_globe_spell]%
    case 2
      say Have you brought shale from the nearby island, %actor.name%?
      break
    case 3
      say Do you have the sake, %actor.name%?
      break       
    case 4
      say Did you find the healer's poultice, %actor.name%?
      break
    default
      say Looks like another bad year for mistletoe.  If only we could get rid of those griffins, we might have a chance.
  done
endif
~
#49034
adramalech_fight1~
0 k 20
~
emote utters a short gutteral command.
mecho A small bird-like demon swoops in to %self.name%'s aid!
mload mob 49041
~
#49035
adramalech_allgreet1~
0 h 100
~
wait 2
if %actor.vnum% == -1 && %actor.level% < 100
   emote throws his head back and laughs a deep booming laugh.
   mecho %self.name% says, 'So %actor.name%, my form in your plane has been destroyed
   mecho &0for now, but this is MY domain.'
   wait 1
   emote grins an evil grin, showing a lot of teeth.
   say Perhaps I will invite a few friends to help me destroy you.
endif
~
#49036
hermit_receive1~
0 j 100
~
if %object.type% == LIQCONTAINER
  wait 2
  if %object.val2% == 5
    cheer
    say Thank you %actor.name%, you have made an old man very happy.
    mload obj 49041
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
        if !%person.quest_stage[griffin_quest]%
          quest start griffin_quest %person%
        endif
        quest variable griffin_quest %person% ladder 1
      elseif %person% && %person.vnum% == -1
        eval i %i% + 1
      endif
      eval a %a% + 1
    done
    give ladder %actor.name%
    say That completes my part of the bargain.
    wait 2
    drink bottle
    smile
  else
    say I'm not drinking this!
    pour %object% out
    drop %object%
  endif
else
  return 0
  say What's this for?
  mecho %self.name% refuses %object.shortdesc%.
endif
~
#49037
hermit_speak1~
0 d 100
trade?~
if %actor.quest_stage[griffin_quest]%
  sigh
  say The one thing that I miss from the life I used to lead is whisky.
  wait 1s
  say If you bring me a bottle of whisky, the ladder is yours.
endif
~
#49038
hermit_speak2~
0 d 100
ladder?~
if %actor.quest_stage[griffin_quest]%
  say I do have a slightly used rope ladder, which I will trade with you.
  wait 1
  say It's magical and will automatically deploy if you drop it in the right area to get down to St. Marvin.
endif
~
#49039
hermit_speak3~
0 d 100
marvin marvin?~
if %actor.quest_stage[griffin_quest]%
  sigh
  say Ah, the St. Marvin was lost with all hands many years ago.  They say the wreck has never been found.
  peer
  wait 1s
  say I can guess where it is though, but you will need a ladder to get to it.
endif
~
#49040
hermit_greet1~
0 g 100
~
if %actor.quest_stage[griffin_quest]%
  say How may I help you %actor.name%?
endif
~
#49041
awura_rand1~
0 b 30
~
emote mutters to herself about the state of the trees these days.
say Must go and talk to Earle about this.
~
#49042
awura_receive1~
0 j 100
~
if %object.vnum% == 49001
  wait 2
  mjunk griffin-skin
  if !(%get.mob_count[49010]%)
    mat 49190 mload mob 49010
  endif
  if %actor.quest_stage[griffin_quest]% == 7
    mat 49190 mforce adramalech mload obj 49062
    mat 49190 mforce adramalech mload obj 49019
    quest advance griffin_quest %actor%
    msend %actor% &7&bYou have advanced the quest!&0
  endif
  mecho a look of disgust crosses Awura's features as she handles the skin.
  emote utters a short spell.
  wait 8
  mecho The griffin skin turns to smoke and blows away in the breeze.
  wait 4
  msend %actor.name% Awura studies you closely, to be sure of your intentions.
  mechoaround %actor.name% Awura studies %actor.name% closely, to be sure of %actor.p% intentions. 
  mecho %self.name% says, '%actor.name%, thank you so much for saving our home from Dagon's rule.'
  wait 4
  mecho %self.name% says, 'As a small token of gratitude, please accept this helmet.  It is not much, but I pray it will help you in the future.'
  mload obj 49061
  give helmet %actor.name%
  wait 4
  curtsey %actor.name%
  wait 4
  mecho %self.name% says, 'Now, seek out and destroy Dagon's demonic essence, Adramalech.  A portal to its home realm is locked beneath the well.  The cult leader must have had a key of some kind.'
else
  return 0
  msend %actor% &7&b%self.name% tells you, 'No, thank you.'&0
end
~
#49043
cult_leader_load_dagon~
0 h 100
~
if %self.room% != 49091
  mgoto 49091
else
  if %actor.vnum% == -1
    set stage 5
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
        if %person.quest_stage[griffin_quest]% >= %stage%
          set load yes
          if %person.quest_stage[griffin_quest]% == %stage%
            quest advance griffin_quest %person.name%
            msend %person% &7&bYou have advanced the quest!&0
          endif
        endif
      elseif %person% && %person.vnum% == -1
        eval i %i% + 1
      endif
      eval a %a% + 1
    done
  endif
endif
if %load% == yes
  if %get.mob_count[49021]% == 0
    wait 2
    cackle
    say You are too late!  It is time to call the great Dagon!
    wait 2
    emote makes some mystical moves with his hands and whispers an ancient spell.
    mecho There is a shimmer in the air and suddenly Dagon stands before you.
    mat 1100 mload mob 49021
    mload obj 49001
    mat 1100 give griffin-skin dagon
    mat 1100 mforce dagon wear skin
    mat 1100 mteleport dagon 49091
    mforce dagon roar
    wait 4
    mforce dagon say At last, summoned to the aid of my faithful.
  endif
endif
~
#49044
cultleader_allgreet1~
0 h 100
~
if %actor.vnum% == -1
mat 49083 mforce ai give load-dagon-flag cult
endif
~
#49045
skuran~
0 g 50
~
peer %actor.vnum%
if %actor.vnum% == -1
say I hope you are not here to cause any trouble, %actor.name%.
msend %actor.name% Skuran flexes a very large set of claws in your face.
endif
~
#49046
UNUSED~
0 k 40
~
circle
~
#49047
demon_rand1~
0 b 60
~
emote grinds its beak menacingly and stares around itself.
~
#49048
eldest_speak3~
0 d 1
weapon weapon? oak oak? items items? dagon dagon?~
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
    if !%person.quest_stage[griffin_quest]%
      quest start griffin_quest %person.name%
      msend %person% &7&bYou have begun the Griffin Isle quest!&0
    endif
  elseif %person% && %person.vnum% == -1
    eval i %i% + 1
  endif
  eval a %a% + 1
done
say Yes, a branch was taken from the holy oak with a mystic sword.  It was bound for the island aboard the St. Marvin, but the ship was lost in a storm.
wait 2s
say If only those sacred implements could be recovered, we may be able to finally move against the cult...  The island hermit may know more.
~
#49049
hermit_speak4~
0 d 1
where where?~
wait 2s
say I believe the St. Marvin wrecked near the cliffs on the western shores.
~
#49099
griffin_quest_status_checker~
0 d 0
quest progress~
set stage %actor.quest_stage[griffin_quest]%
wait 2
if %actor.has_completed[griffin_quest]%
  mecho %self.name% says, 'You have destroyed Adramalech and his cult!  Thank you, %actor.name%!'
  halt
endif
switch %stage%
  case 0
    mecho %self.name% says, 'I need to see if it is still possible to destroy the cult of the griffin.  Find the wreck of the St. Marvin and retrieve the cutting from the sacred oak.'
    break
  case 1
    mecho %self.name% says, 'The sword used to cut the oak is also critical.  Bring me the mystic sword from the wreck of the St. Marvin.'
    break
  case 2
    mecho %self.name% says, 'You'll need oracular guidance.  Go to the Seer and say &7&b'Earle sends me'&0'
    break
  case 3
    mecho %self.name% says, 'Get assistance from the strongest person on the island.  Find Derceta and return her crystal earring to her.'
    break
  case 4
    mecho %self.name% says, 'The entrance to the cult's lair is hidden under a massive boulder.  Find Derceta, return her earring again, and ask her to move the boulder.'
    break
  case 5
    mecho %self.name% says, 'Find the cult's altar and destroy it.  Go through the gate Derceta unearthed and drop the sapling at the cult's altar.'
    break
  case 6
    say It is time to destroy the cult!  Slay Dagon!
    break
  case 7
    say Bring the griffin skin to Awura.
  case 8
    mecho %self.name% says, 'Now you can strike the final blow and destroy the essence of the god of the cult itself.  Seek out the hidden entrance to the other realms and destroy Adramalech.'
done
~
#49101
Bash_door~
0 c 100
doorbash~
return 0
~
$~
