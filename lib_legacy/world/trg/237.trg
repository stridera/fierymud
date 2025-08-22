#23700
Block_trigger~
0 c 100
north~
msend %actor.name% The guard smacks you in the head and says, 'No'.
mechoaround %actor.name% The guard smacks %actor.name% in the head, and says 'No'. 
~
#23701
guard-west~
0 c 100
west~
msend %actor% The guard roars at you!
mechoaround %actor% The guard roars at %actor.name% and blocks the way.
~
#23705
shroom-attack~
0 c 100
get~
if %arg% /= mushroom
set rightobj 1
endif
if %arg% /= pink
set rightobj 1
endif
if %arg% /=purple
set rightobj 1
endif
if %arg% /= fungus
set rightobj 1
endif
if %arg% /= bulb
set rightobj 1
endif
if %arg% /= cap
set rightobj 1
endif
if %rightobj% ==1
mkill %actor.name%
else
return 0
endif
~
#23706
collar-shock~
1 l 100
~
if %actor.level% < 100
   odamage %actor% 15
   osend %actor% The collar shocks you as you try to remove it!
   oechoaround %actor% As %actor.name% reaches for a slave collar, %actor.n% suddenly spasms, arms lolling.
   return 0
endif
~
#23718
shadow-cloaking~
1 j 100
~
osend %actor.name% You feel secure, cloaked in the shadows.
oechoaround %actor% %actor.name% slowly fades into the shadows.
~
#23720
serin-greet~
0 g 100
~
* This is the beginning of the quest to rescue the elven prisoner.
if %actor.vnum% == -1
  if %actor.quest_stage[sunfire_rescue]% == 0
    msend %actor.name% The prisoner looks up at %actor.name% hopefully.
    msend %actor.name% The prisoner says, 'Can you help me?'
  elseif %actor.quest_stage[sunfire_rescue]% == 1
    * then if they have been sent on the quest already, they should have the boots
    * or at least be reminded of it.
    msend %actor.name% The prisoner says, 'Ah! Have you brought me the treasures from Templace?'
    msend %actor.name% 'Please, give them to me! If you forgot what you need to do, you can ask me
    msend %actor.name% 'for a &7&b[progress]&0 report.'
  endif
endif
~
#23721
Infernal Blade bonus damage~
1 d 100
~
*
* This trigger adds 10% damage as fire damage and 10% damage as unholy damage
*
if %damage%
  eval fire_dam %damage% / 10
  eval unholy_dam %damage% / 10
  eval bonus %fire_dam% + %unholy_dam%
  oecho &1%self.shortdesc% burns %victim.name% with unholy fire!&0 (&3%bonus%&0)
  odamage %victim% %fire_dam% fire
  odamage %victim% %unholy_dam% align
endif
~
#23722
help-confirmed~
0 dn 100
yes yes! ok ok! okay okay! sure sure! help help?~
wait 2
if (%actor.vnum% == -1) && (%actor.align% > 349) 
  if %actor.quest_stage[sunfire_rescue]% == 0
    * Responding to the trigger in 23721...rescuing poor elf boy.
    mecho The prisoner smiles.
    say My name is Serin Sunfire.  I was captured...
    wait 1s
    emote counts on his fingers, looks a bit puzzled, then shakes his head and continues.
    wait 1s
    mecho %self.name% says, 'I was captured from my hometown of Templace by the drow.
    mecho &0Please, tell me.  Is Templace still the land of peace and beauty?'
  elseif %actor.quest_stage[sunfire_rescue]% == 1
    emote smiles brightly at %actor.name% and looks hopeful.
    mecho %self.name% says, 'Thank you!  Please return to me with the items and I will
    mecho &0be most grateful.'
    wait 1s
    emote pauses a moment.
    wait 2s
    mecho %self.name% says, 'Oh, and do not bother with the cursed items.  I can tell
    mecho &0the difference.'
    wait 2s
    mecho %self.name% says, 'If you forget what you need to do, you can ask me for a
    mecho &0&7&b[progress]&0 report.'
    wait 3s
    mecho The prisoner looks apprehensive.
    wait 1s
    whisper %actor.name% You should leave here before the guards come!
    emote relaxes back into a position of pain, though he smiles at you.
  endif
elseif (%actor.vnum% == -1) && (%actor.align% <= 349)
  msend %actor.name% The prisoner sighs and turns away from you.
  say I cannot be helped by one such as you.
endif
~
#23723
serin-templace~
0 dn 100
no nope sorry~
wait 2
if %actor.vnum% == -1 && %actor.align% > 349
  if %actor.quest_stage[sunfire_rescue]% == 0
    * Responding to trigger 23722 in the prisoner quest.
    * This leads directly to the actual quest trigger.
    emote looks grief-stricken.
    wait 1s
    mecho %self.name% says, 'Well, perhaps you can still help me.  My peoples'
    mecho &0treasures must be in the hands of Templace's conquerors.  If you could return
    mecho &0to me a &2&bcloak&0, &2&bboots&0 and a &2&bring of the elves&0...'
    wait 1s
    emote smiles fiercely.
    wait 1s
    say I could finally escape.  Will you do this for me?
    quest start sunfire_rescue %actor.name%
  elseif %actor.quest_stage[sunfire_rescue]% == 1
    emote frowns at %actor.name% stormily.
    say Then waste my time no more, and begone!
    mforce %actor.name% north
    * Oooh, he gets really mad here.
    quest erase sunfire_quest %actor.name%
  endif
endif
~
#23724
***UNUSED***~
0 d 100
yes no~
if %actor.quest_stage[sunfire_rescue]% == 1
* Responding to trigger 23723 in the rescue quest.
* I figure he'd be able to tell the difference between cursed items
* and besides it's harder to get them all individually.
* Fairly straightforward...
if %speech% ==yes
if %actor.align% > 349
quest start sunfire_rescue %actor.name%
emote smiles brightly at %actor.name% and looks hopeful.
say Thank you! Please return to me with the items and I will be most grateful.
emote pauses a moment.
say Oh, and do not bother with the cursed items.  I can tell the difference.
mecho The prisoner looks apprehensive.
whisper %actor.name% You should leave here before the guards come!
emote relaxes back into a position of pain, though he smiles at you.
endif
endif
if %speech% ==no
emote frowns at %actor.name% stormily.
say Then waste my time no more, and begone!
mforce %actor.name% north
* Oooh, he gets really mad here.
endif
end
endif
~
#23725
boots-receive~
0 j 100
~
if %actor.vnum% == -1
wait 1s
if %actor.quest_stage[sunfire_rescue]% == 1
if %object.vnum% == 52008
quest advance sunfire_rescue %actor.name%
smile %actor.name%
emote slips his feet out of the shackles and wears the boots.
say Thank you.
wait 1s
say And the cloak? Do you have the cloak? Give it to me, please!
endif
if %object.vnum% == 52024
emote looks at the boots carefully.
say These are the cursed boots.  If you have the real ones, please...
say These cannot help me at all.
say Do you have the real ones? I need them.
endif
end
end
~
#23726
cloak_receive~
0 j 100
~
if %actor.vnum% == -1
if %actor.quest_stage[sunfire_rescue]% == 2
if %object.vnum% == 52009
wait 1s
quest advance sunfire_rescue %actor.name%
emote carefully looks at the cloak.
say This is it! Thank you!
emote unshackles his arms and wears the cloak on his shoulders.
wait 1s
say And the ring?  If you have the ring, please give it to me!
say Then I can finally escape....
endif
if %object.vnum% == 52026
emote runs his hands over the cloak quickly.
say This is the cursed cloak!
emote looks angry.
wait 1s
say Well? Do you have the real cloak to give to me?
emote taps his foot impatiently.
endif
end
end
~
#23727
ring-receive-complete~
0 j 100
~
if %actor.vnum% == -1
if %actor.quest_stage[sunfire_rescue]% ==3
if %object.vnum% == 52001
mecho Looking hesitant, the prisoner slowly slides the ring onto his finger.
quest advance sunfire_rescue %actor.name%
mload obj 23716
mjunk all.elven
wait 2s
emote vanishes from sight.
give %actor.name% badge
quest complete sunfire_rescue %actor.name%
whisper %actor.name% Thank you for your help!  Please wear this badge as a token of my respect.
mecho A voice softly echos goodbye...
mpurge serin
endif
if %object.vnum% == 52018
frown %actor.name%
say What kind of cruel trick is this?
wait 1s
say Well, do you have the real one to give to me?
endif
end
~
#23728
serin-receive~
0 j 100
~
* Responding to trigger 23723 after they actually have the items.
* This actually advances and ends the quest.
set boots %actor.quest_variable[sunfire_rescue:boots]%
set cloak %actor.quest_variable[sunfire_rescue:cloak]%
set ring %actor.quest_variable[sunfire_rescue:ring]%
if %actor.vnum% != -1
   return 0
elseif %actor.level% > 99
   return 0
   wait 1s
   eyebrow
else
  if %actor.quest_stage[sunfire_rescue]% == 1
    switch %object.vnum%
      case 52008
        if %boots% == 1
          return 0
          say You have already given me these.
        else
          quest variable sunfire_rescue %actor.name% boots 1
          wait 2
          mjunk %object%
          smile %actor.name%
          say Thank you.
          wait 1s
        endif
        break
      case 52024
        * Cursed boots
        return 0
        emote looks at the boots carefully.
        mecho %self.name% refuses %object.shortdesc%.
        mecho %self.name% says, 'These are the cursed boots.  If you have the real ones,
        mecho &0please...  These cannot help me at all.  Do you have the real ones?  I need
        mecho &0them.'
        halt
        break
      case 52018
        return 0
        frown %actor.name%
        mecho %self.name% refuses %object.shortdesc%.
        say What kind of cruel trick is this?
        wait 1s
        say Well, do you have the real one to give to me?
        halt
        break
        * covers the cursed ring...
      case 52009
        if %cloak% == 1
          return 0
          say You have already given me the cloak.
        else
          wait 2
          quest variable sunfire_rescue %actor.name% cloak 1
          mjunk %object%
          emote carefully looks at the cloak.
          say This is it!  Thank you!
          wait 1s
          say Then I can finally escape....
        endif
        break
      case 52026
        * Cursed cloak
        return 0
        emote runs his hands over the cloak quickly.
        say This is the cursed cloak!
        emote looks angry.
        msend %actor% shoves %object.shortdesc% back in your face.
        mechoaround %actor% shoves %object.shortdesc% back in %actor.name%'s face.
        wait 1s
        say Well?  Do you have the real cloak to give to me?
        emote taps his foot impatiently.
        halt
        break
      case 52001
        if %ring% == 1
          return 0
          say You have already given me this ring.
        else
          wait 2
          mecho Looking at the ring, the prisoner looks overwhelmed with emotion.
          quest variable sunfire_rescue %actor.name% ring 1
          say Escape is so close I can feel it!
          mjunk %object%
          * that ought to cover everything...
        endif
        break
    default
      return 0
      say This isnt going to help me!
      roll
    done
  elseif %actor.has_completed[sunfire_rescue]%
    return 0
    say You have already helped me.  I am grateful to you.
  else
    return 0
    say What are you doing?
  endif
  set boots %actor.quest_variable[sunfire_rescue:boots]%
  set cloak %actor.quest_variable[sunfire_rescue:cloak]%
  set ring %actor.quest_variable[sunfire_rescue:ring]%
  eval total %boots% + %cloak% + %ring%
  if %total% >= 3
    wait 2s
    emote slips his feet out of the shackles and wears the boots.
    wait 2s
    emote unshackles his arms and wears the cloak on his shoulders.
    wait 2s
    mecho Looking hesitant, the prisoner slowly slides the ring onto his finger.
    quest advance sunfire_rescue %actor.name%
    mload obj 23716
    mjunk all.elven
    wait 3s
    emote vanishes from sight.
    * The badge reward is level 50, but the players have to be 70 or so
    * to handle the boots, cloak etc anyway....:)
    *
    * Set X to the level of the award - code does not run without it
    * 
    if %actor.level% < 85
       set expcap %actor.level%
    else
       set expcap 85
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
    switch %actor.class%
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
        eval expmod (%expmod% + ((%expmod% * 2) / 5))
        break
      default
        set expmod %expmod%
    done
    msend %actor% &3&bYou gain experience!&0
    eval setexp (%expmod% * 10)
    set loop 0
    while %loop% < 10
    *
    * Xexp must be replaced by mexp, oexp, or wexp for this code to work
    * Pick depending on what is running the trigger
    *
       mexp %actor% %setexp%
       eval loop %loop% + 1
    done
    quest complete sunfire_rescue %actor.name%
    wait 2s
    msend %actor% %self.name% whispers to you, 'Thank you for your help!  Please wear this badge as a token of my respect.'
    give badge %actor.name%
    wait 2s
    mecho A voice softly echos, 'Good-bye...'
    mpurge serin
  else
    wait 1s
    say Do you have the other treasures of my people?
  endif
endif
~
#23750
vilekka-greet~
0 g 100
~
* Here we go, the big evil vilekka_stew quest of DEATH!! :)
* This is a quest designed for neutral and evil types only.
* If good players want a quest, see the sunfire_rescue quest.
if %actor.vnum% != -1 || %actor.level% > 99 || %actor.has_completed[vilekka_stew]%
   halt
end
wait 1s
* If they are coming back with the items she said to fetch...
* Starting with the heart of the drow master from Anduin...
if %actor.quest_stage[vilekka_stew]% == 1
   mecho The High Priestess says, 'Well? Do you have the heart of that traitor?'
   mecho She looks greedy.  'Give it to me if you do.  You may ask about your &7&b[progress]&0 if you need.'
elseif %actor.quest_stage[vilekka_stew]% == 2
   wink %actor.name%
   say Well?  Continue now or stop?
elseif %actor.quest_stage[vilekka_stew]% == 3
   * Here we have the second stage, the head of the drider king
   say Well?  Have you brought me the head of my enemy?
   mecho The High Priestess licks her lips.
   wait 1s
   mecho %self.name% says, 'Give it to me.  You may ask about your &7&b[progress]&0 if you need.'
elseif %actor.quest_stage[vilekka_stew]% == 4
   grin %actor.name%
   say Well?  Do you wish to continue now?
elseif %actor.quest_stage[vilekka_stew]% == 5
   * Now here we have the player supposedly returning with spices and herbs.
   mecho %self.name% says, 'At last, I can carry out the Spider Queen's orders!  Give me the spices and herbs!  The finest of the realm!  Ask for a reminder of your &7&b[progress]&0 if you need.'
else
   * IE, if they haven't started the quest.
   if %actor.align% < 349
      msend %actor.name% %self.name% looks up at you.
      mechoaround %actor% %self.name% looks up at %actor.name%.
      mecho She says, 'Welcome to my city!  All who are not followers of the path of good are welcome here.'
      wait 2s
      msend %actor.name% She looks closely at you.
      mechoaround %actor% She looks closely at %actor.name%.
      mecho %self.name% says, 'In fact, perhaps you can help me.  Yes, I think you can help me perform a great service to the city.'
   else
      msend %actor.name% %self.name% sneers at you.
      mechoaround %actor% %self.name% sneers at %actor.name%.
      say So, you think you can just wander around my city?
      mecho %self.name% laughs.
      wait 1s
      mecho %self.name% says, 'Get out of my sight.  I find your presence annoying.'
      wait 1s
      mechoaround %actor% &5Vilekka gestures at %actor.name%, and %actor.n% disappears.&0
      mteleport %actor.name% 23793
      wait 1
      mat 23793 msend %actor.name% &5Vilekka gestures at you, and your vision wavers...&0
      * Can't use mat while forcing them to look - they will see the
      * priestess as she is temporarily in the room due to mat.
      mat 23793 msend %actor.name% You find yourself back in the temple.
   endif
endif
~
#23751
vilekka-service~
0 d 100
service service?~
* OK, this is in response to 23750, the beginning of the vilekka_stew quest.
* By responding favorably to her, they begin. 23755 is the drow master
* head-spawning death trigger.
if %actor.vnum% != -1 || %actor.level% > 99 || %actor.has_completed[vilekka_stew]%
   halt
end
wait 1s
if %actor.quest_stage[vilekka_stew]% < 2
   if %actor.align% < 349
      con %actor.name%
      emote smiles slowly.
      say Yes, you can be of great service to my city.
      wait 2s
      mecho %self.name% says, 'I am the High Priestess of the Spider Queen, and therefore I rule here.  But there is one who escaped my power...'
      growl
      wait 5s
      say He must be destroyed.
      wait 3s
      mecho %self.name% says, 'I believe he is residing in one of your surface cities.  You should easily be able to recognize any drow up there, but this one...'
      wait 5s
      sigh
      say He is always trying to pick fights.
      emote grins nastily.
      wait 2s
      say Or at least he was when he left here.
      wait 3s
      say Bring me his heart!  Then I shall reward you.
      wait 3s
      mecho %self.name% says, 'You may ask about your &7&b[progress]&0 if you need.'
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
            if %person.quest_stage[vilekka_stew]% == 0
               quest start vilekka_stew %person.name%
               msend %person% &7&bThe quest has now begun!&0
            endif
         elseif %person% && %person.vnum% == -1
            eval i %i% + 1
         endif
         eval a %a% + 1
      done
   end
elseif %actor.quest_stage[vilekka_stew]% == 2
   say Well, do you want to continue or stop?
elseif %actor.quest_stage[vilekka_stew]% == 3
   say You must bring me the head of the drider king.
elseif %actor.quest_stage[vilekka_stew]% == 4
   say Well, do you want to continue or stop?
elseif %actor.quest_stage[vilekka_stew]% == 5
   mecho %self.name% says, 'Bring me some spices so that I can make this head and heart palatable.'
else
   say %actor.name%, you puzzle me.
end
~
#23752
receive-rewards~
0 j 100
~
set accepted 0
if %object.vnum% == 23721
   * Received the heart
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
         if %person.quest_stage[vilekka_stew]% == 1
            quest advance vilekka_stew %person.name%
            msend %person% &7&bYou have advanced the quest!&0
            set accepted 1
         endif
      elseif %person% && %person.vnum% == -1
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
   if %accepted%
      wait 1s
      mjunk all.heart
      say Excellent!
      wait 1s
      mecho The High Priestess takes the heart and places it gently on the altar.
      smirk
      wait 1s
      say You have done me and my city a great service!  Do you wish to continue or stop now?
      tap
      mecho &7&bAll characters must choose individually.&0
   endif
end
if %object.vnum% == 23720
   * Received the head
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
         if %person.quest_stage[vilekka_stew]% == 3
            quest advance vilekka_stew %person.name%
            msend %person% &7&bYou have advanced the quest!&0
            set accepted 1
            if !%person.quest_variable[hell_trident:helltask6]% && %person.quest_stage[hell_trident]% == 1
               quest variable hell_trident %person% helltask6 1
               msend %person% &5You have successfully assisted %self.name% in service of Lolth!&0
            endif
         endif
      elseif %person% && %person.vnum% == -1
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
   if %accepted%
      wait 1s
      mjunk all.head
      msend %actor% %self.name% says, 'You are quite resourceful!  This is a great benefit to the entire city.'
      emote places the severed head on the altar, where it lolls around.
      wait 1s
      msend %actor% %self.name% says, 'But your work may not yet be finished...  Do you wish to continue, or stop here?'
      mecho &7&bAll characters must choose individually.&0
   endif
end
set isspice 0
if %actor.quest_stage[vilekka_stew]% == 5
   set num %object.vnum%
   if %num% == 12552
      set accepted 1
      set isspice 1
   end
   if %num% == 49022
      set accepted 1
      set isspice 1
   end
   if %num% > 23749
      if %num% < 23761
         set accepted 1
         set isspice 1
      end
   end
   * Make sure it's valid, and we haven't gotten it yet
   if %accepted% == 1 && %actor.quest_variable[vilekka_stew:got_spice:%num%]% == 1
      set accepted 0
   end
   if %accepted% == 0
      return 0
      emote refuses to accept %object.shortdesc%.
      wait 1s
      if %isspice% == 1
         msend %actor% %self.name% says, 'You have already given me %object.shortdesc%.'
      else
         msend %actor% %self.name% says, 'What is this?  I need the spices!'
      end
      halt
   end
   wait 1s
   eval num_spices 1 + %actor.quest_variable[vilekka_stew:num_spices]%
   quest variable vilekka_stew %actor.name%  num_spices %num_spices%
   quest variable vilekka_stew %actor.name%  got_spice:%num% 1
   if %num_spices% < 10
      mjunk herb
      * ok, here we have a way for the player to keep track
      * of the number of spices he/she has given already
      if %num_spices% == 1
         msend %actor% %self.name% says, 'Very good!  But I still need nine more.'
      end
      if %num_spices% == 2
         lick
         msend %actor% %self.name% says, 'Eight more....'
      end
      if %num_spices% == 3
         msend %actor% %self.name% says, 'Seven more spices, and my stew will be perfect!'
      end
      if %num_spices% == 4
         emote is starting to look excited.
         msend %actor% %self.name% says, 'Six more!'
      end
      if %num_spices% == 5
         grin
         msend %actor% %self.name% says, 'You're halfway there!  Five more!'
      end
      if %num_spices% == 6
         emote sniffs the spices carefully.
         msend %actor% %self.name% says, 'Yes!  Only four more!'
      end
      if %num_spices% == 7
         msend %actor% %self.name% says, 'Three more!  Give them to me!'
      end
      if %num_spices% == 8
         emote looks very excited now.
         msend %actor% %self.name% says, 'Two more spices!'
      end
      if %num_spices% == 9
         msend %actor% %self.name% says, 'One more!  Give it to me!'
         * k, I think that's all the spices and stuff
         * now back to your regularly scheduled quest
      end
   end
   if %num_spices% >=10
      cackle
      msend %actor% %self.name% says, 'Most excellent!  Now I may complete the will of my Queen!'
      wait 1s
      emote puts all the ingredients for the nasty stew into a small bag.
      mecho A servant silently enters and bows.
      emote gives a small bag to the servant.
      mecho The servant leaves.
      wait 1s
      msend %actor% %self.name% says, 'Oh yes, your reward....'
      if %actor.quest_variable[vilekka_stew:got_spice:23760]% == 1
         if %actor.quest_variable[vilekka_stew:got_spice:23756]% == 1
            * saffron and salt
            mload obj 23798
            if %actor.level% < 60
               set expcap %actor.level%
            else
               set expcap 60
            endif
         else
            * Saffron and no salt
            mload obj 23799
            if %actor.level% < 85
               set expcap %actor.level%
            else
               set expcap 85
            endif
         end
      else
         if %actor.quest_variable[vilekka_stew:got_spice:23756]% == 1
            * No Saffron and salt
            mload obj 23796
            if %actor.level% < 50
               set expcap %actor.level%
            else
               set expcap 50
            endif
         else
            * No Saffron and no salt
            mload obj 23797
            if %actor.level% < 55
               set expcap %actor.level%
            else
               set expcap 55
            endif
         end
      end
      *
      * Set X to the level of the award - code does not run without it
      * 
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
      switch %actor.class%
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
      msend %actor% &3&bYou gain experience!&0
      eval setexp (%expmod% * 10)
      set loop 0
      while %loop% < 10
      *
      * Xexp must be replaced by mexp, oexp, or wexp for this code to work
      * Pick depending on what is running the trigger
      *
         mexp %actor% %setexp%
         eval loop %loop% + 1
      done
      give boots %actor.name%
      wait 1s
      smile
      msend %actor% %self.name% says, 'Wear them proudly!'
      quest complete vilekka_stew %actor.name%
   end
end
if %accepted% == 0
   return 0
   msend %actor% %self.name% haughtily refuses your gift.
   mechoaround %actor% %self.name% refuses to accept %object.shortdesc% from %actor.name%.
   wait 1s
   say No thank you.
   halt
end
~
#23753
vilekka-eeew~
0 d 100
disgusting disgusting?~
* OK, this is in response to 23752, the returning of the Drider King's
* head. (Why is she questioning? Check 23754 part 2..her goddess
* is requiring that she eat the head and heart the player returns to her.
wait 2
if (%actor.vnum% == -1) && (%actor.quest_stage[vilekka_stew]% == 5)
  emote looks shrewdly around the room.
  wait 2s
  say It is not that I question the will of my Goddess...
  wait 2s
  mecho %self.name% says, 'But perhaps it can be made easier.
  wait 3s  
  mecho %self.name% says, 'The Spider Queen requires of me that I eat this head and this heart.'
  wait 4s
  say But she did not say I should eat them plain.
  emote gets a wicked expression on her face.
  wait 4s
  mecho %self.name% says, 'You!  %actor.name%!  Bring me the ten finest herbs and spices from the realm!  I do not care where you get them from, but make sure they are the finest!'
  wait 5s
  say Return them to me and I will surely reward you.
  wink %actor.name%
  wait 3s
  mecho %self.name% says, 'Ask for a reminder of your &7&b[progress]&0 if you need.'
  mecho &7&bQuest credit will only be awarded individually&0.
end
~
#23754
continue-advancing~
0 d 100
continue~
* OK, this is in two parts.  It represents the player wishing to
* continue the quest rather than stopping and accepting
* a lesser prize.  The first one is after returning the
* drow master's heart (the dude from Anduin) and the second
* is after returning the drider king's head (who's in
* this area).  It follows trigger 23752 parts 1 and 2.
wait 2
if %actor.vnum% == -1
  if %actor.quest_stage[vilekka_stew]% == 4
    quest advance vilekka_stew %actor.name%
    msend %actor% &7&bYou have advanced the quest!&0
    emote looks pleased.
    mecho The High Priestess begins chanting to her goddess.
    wait 2s
    emote continues to chant...
    wait 4s
    emote looks like she just swallowed something very unpleasant.
    say Disgusting.
  elseif %actor.quest_stage[vilekka_stew]% == 2
    quest advance vilekka_stew %actor.name%
    msend %actor% &7&bYou have advanced the quest!&0
    say Yes, the driders....
    shudder
    wait 2s
    mecho %self.name% says, 'Driders are ancient enemies of all true drow.  A drider is a half-drow, half-spider abomination.  When a young drow fails the coming of age test that the Spider Queen requires, it is turned into a drider as punishment and exiled from our race.'
    wait 4s
    mecho %self.name% says, 'I say "it", but all drow who have ever failed the test have been male.'
    wait 3s
    mecho %self.name% says, 'Males are weak and inferior.  So they often become driders.'
    wait 3s
    sigh
    say They have lately been harassing Dheduu.
    wait 2s
    mecho %self.name% says, 'Bring me the head of their king!  That should discourage them.'
    cackle
    wait 2s
    mecho %self.name% says, 'You may ask about your &7&b[progress]&0 if you need.'
    wait 2s
    emote waves you away.  'Go on.'
  endif
endif
~
#23755
drow-heart-spawn~
0 f 100
~
* This is for the vilekka_stew quest in the drow city of Dheduu.
set person %actor%
set i %actor.group_size%
if %i%
   set a 1
   unset person
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[vilekka_stew]% == 1
            set quest yes
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[vilekka_stew]% == 1
   set quest yes
endif
if %quest% == yes
  return 0
  mload obj 23721
  mecho The drow master's last breath echoes softly as he dies.
  mecho 'Mother...why...'
end
~
#23756
drider-king-death~
0 f 100
~
* This is for the vilekka_stew quest, stage 2.
set person %actor%
set i %actor.group_size%
if %i%
   set a 1
   unset person
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[vilekka_stew]% == 3
            set quest yes
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[vilekka_stew]% == 3
   set quest yes
endif
if %quest% == yes
    return 0
    mecho With a horrible shriek, the drider king's body melts!
    mload obj 23720
endif
~
#23757
vilekka_stop~
0 d 100
stop~
* OK, here we have the other side of trigger 23754
* where the player does not wish to continue the quest at this time
* but they can come back later. :)
if (%actor.quest_stage[vilekka_stew]% == 2) && !(%actor.quest_variable[vilekka_stew:awarded_23718]%)
   nod
   wait 1s
   mecho %self.name% says, 'It is fine if you do not think you are yet worthy of more service.  Take this as a token of my appreciation.'
   mload obj 23718
   give cloak %actor.name%
   quest variable vilekka_stew %actor.name% awarded_23718 1
   wait 1s
   emote smiles fiercely.
   mecho %self.name% says, 'And should you ever wish to continue your service, just return to me.  Say you wish to continue, and I will give you more to do.'
elseif (%actor.quest_stage[vilekka_stew]% == 4) && !(%actor.quest_variable[vilekka_stew:awarded_23717]%)
   mload obj 23717
   grin
   say I suppose I could let you off, this time...
   wait 1s
   say Wear this to show my thanks for your help.
   give bracelet %actor.name%
   quest variable vilekka_stew %actor.name% awarded_23717 1
   mecho %self.name% says, 'But do return and say that you wish to continue when you are more experienced.'
endif
~
#23781
glazed-stiletto-wield~
1 j 100
~
if %actor.quest_variable[quest_items:%self.vnum%]%
   return 1
   osend %actor% You flip out the blade of %self.shortdesc%.
   oechoaround %actor% %actor.name% flips the blade out of %self.shortdesc%.
else
   return 0
   osend %actor% You try to wield %self.shortdesc%, but can't figure out how to open it.
endif
~
#23790
recall_nymrill~
1 c 2
quaff~
if %actor.vnum% == -1
   if %actor.align% <= -350
      return 1
      osend %actor% You quaff a potion and begin to feel a little light-headed...
      oechoaround %actor% %actor.name% quaffs a black potion of recall.
      oechoaround %actor% %actor.name% disappears in a flash of darkness!
      if (%actor.class% /= Thief) || (%actor.class% /= Assassin) || (%actor.class% /= Mercenary) || (%actor.class% /= Rogue)
         oteleport %actor% 49525
      elseif (%actor.class% /= Sorcerer) || (%actor.class% /= Cryomancer) || (%actor.class% /= Pyromancer)
         oteleport %actor% 49522
      elseif (%actor.class% /= Necromancer)
         oteleport %actor% 49514
      else
         oteleport %actor% 49512
      endif
      oforce %actor% look
      opurge %self%
   else
      osend %actor% As you quaff a potion, you get a funny burning sensation in your stomach...
      return 1
      oforce %actor% quaff black-potion-recall
   endif
else
   return 1
   oforce %actor% quaff black-potion-recall
endif
~
#23798
sunfire_rescue_status_checker~
0 d 100
progress progress?~
wait 2
set boots %actor.quest_variable[sunfire_rescue:boots]%
set cloak %actor.quest_variable[sunfire_rescue:cloak]%
set ring %actor.quest_variable[sunfire_rescue:ring]%
eval total %boots% + %cloak% + %ring%
if %actor.quest_stage[sunfire_rescue]% == 1
  say Please find the treasures of my people!
  if %total% > 0
    mecho   
    mecho &0You have retrieved the following treasures:
    if %boots%
      mecho %get.obj_shortdesc[52008]%.
    endif
    if %cloak%
      mecho %get.obj_shortdesc[52009]%.
    endif
    if %ring%
      mecho %get.obj_shortdesc[52001]%.
    endif
  endif
  mecho   
  mecho &0You still need to bring me:
  if %boots% == 0
    mecho %get.obj_shortdesc[52008]%.
  endif
  if %cloak% == 0
    mecho %get.obj_shortdesc[52009]%.
  endif
  if %ring% == 0
    mecho %get.obj_shortdesc[52001]%.
  endif
elseif %actor.has_completed[sunfire_rescue]%
  say You have already helped me greatly.
endif
~
#23799
vilekka_stew_status_check~
0 d 100
progress progress?~
wait 2
set num_spices %actor.quest_variable[vilekka_stew:num_spices]%
set spice1 %actor.quest_variable[vilekka_stew:got_spice:12552]%
set spice2 %actor.quest_variable[vilekka_stew:got_spice:49022]%
set spice3 %actor.quest_variable[vilekka_stew:got_spice:23750]%
set spice4 %actor.quest_variable[vilekka_stew:got_spice:23751]%
set spice5 %actor.quest_variable[vilekka_stew:got_spice:23752]%
set spice6 %actor.quest_variable[vilekka_stew:got_spice:23753]%
set spice7 %actor.quest_variable[vilekka_stew:got_spice:23754]%
set spice8 %actor.quest_variable[vilekka_stew:got_spice:23755]%
set spice9 %actor.quest_variable[vilekka_stew:got_spice:23756]%
set spice10 %actor.quest_variable[vilekka_stew:got_spice:23757]%
set spice11 %actor.quest_variable[vilekka_stew:got_spice:23758]%
set spice12 %actor.quest_variable[vilekka_stew:got_spice:23759]%
set spice13 %actor.quest_variable[vilekka_stew:got_spice:23760]%
if %actor.has_completed[vilekka_stew]%
  say You have already done me a great service.
endif
set stage %actor.quest_stage[vilekka_stew]%
switch %stage%
  case 1
    mecho %self.name% says, 'Bring me the heart of the drow living in the
    mecho &0surface city!'
    break
  case 2
    say Well, do you want to continue or stop?
    break
  case 3
    say You must bring me the head of the drider king.
    break
  case 4
    say Well, do you want to continue or stop?
    break
  case 5
    mecho %self.name% says, 'Bring me some spices so that I can make this head
    mecho &0and heart palatable.'
    if %num_spices% > 0
      mecho  
      mecho So far you have brought me:
      if %spice1%
        mecho - %get.obj_shortdesc[12552]%
      endif
      if %spice2%
        mecho - %get.obj_shortdesc[49022]%
      endif
      if %spice3%
        mecho - %get.obj_shortdesc[23750]%
      endif
      if %spice4%
        mecho - %get.obj_shortdesc[23751]%
      endif
      if %spice5%
        mecho - %get.obj_shortdesc[23752]%
      endif
      if %spice6%
        mecho - %get.obj_shortdesc[23753]%
      endif
      if %spice7%
        mecho - %get.obj_shortdesc[23754]%
      endif
      if %spice8%
        mecho - %get.obj_shortdesc[23755]%
      endif
      if %spice9%
        mecho - %get.obj_shortdesc[23756]%
      endif
      if %spice10%
        mecho - %get.obj_shortdesc[23757]%
      endif
      if %spice11%
        mecho - %get.obj_shortdesc[23758]%
      endif
      if %spice12%
        mecho - %get.obj_shortdesc[23759]%
      endif
      if %spice13%
        mecho - %get.obj_shortdesc[23760]%
      endif
    endif    
    mecho    
    eval total 10 - %num_spices%
    mecho Bring me %total% more spices to prepare this stew.
    break
  default
    say You are not performing a service for me.
done
~
$~
