#3000
Receptionist_no_item_trigger~
0 j 100
~
return 0
msend %actor% %self.name% says, 'I'm sorry, but I cannot accept this.'
msend %actor% %self.name% refuses your item.
~
#3001
Drunk_Socail_stunts1~
0 b 70
~
sing
dance drunk
say Good day to you.
wave
~
#3002
MOB_RAND_SOCIAL_HICCUP~
0 b 14
~
hic
~
#3003
recep_vis_trig~
2 g 100
rent~
wait 2
if %self.people[count]%
   wforce recep vis
   wait 2s
endif
~
#3004
Enter Illusionist Guild Mielikki~
2 c 100
south~
return 1
wdoor 3063 south room 3207
wforce %actor% south
wdoor 3063 south purge
~
#3005
Illusionists sense illusion~
2 g 100
~
wait 1
if %actor.class% /= Illusionist
   wsend %actor% &5You sense the magic of illusion at work upon your senses.&0
end
~
#3006
Archmage responds to 'guild'~
0 n 100
guild~
wait 1 s
if %actor.class% /= Sorcerer
   smirk
   wait 3 s
   ruffle %actor.name%
elseif %actor.class% /= Cryomancer || %actor.class% /= Pyromancer
   say Silly bun!  It's right outside!
   wait 3 s
   pat %actor.name%
elseif %actor.class% /= Necromancer
   say Why, you're looking for creepy old Asiri!
   wait 2 s
   nod
   wait 3 s
   say I suppose he's around here somewhere.  Somewhere creepy!  I don't really know.
elseif %actor.class% /= Illusionist
   say Oh dear.  You're asking the wrong spellcaster.  You illusionists, so good at hiding.
   wait 4 s
   say But there is one thing.  Old Eamus - Ermie?  What was his name?
   wait 3 s
   ponder
   wait 3 s
   say Well anyway, he'd always head to the bank after tea.  But he didn't go into the bank.
   wait 4 s
   say Come to think of it, you illusionists are always sneaking off to the bank.   Or not.
   say Oh, I'm so confused!
else
   sigh
   wait 3 s
   say I'm so sorry, dearie, I've no earthly idea where your guild is.
end
~
#3007
Archmage responds to 'necromancer'~
0 n 100
necromancer~
wait 1 s
if %actor.class% /= Sorcerer && %actor.level% > 9 && %actor.level% < 46
   say So you want to take on the dark arts, eh?  The foul arts?  The stinky death arts?
   wait 3 s
   sniff
   wait 3 s
   say I suppose there's no accounting for taste.  You might want to find a pungent necromancer to help with this endeavour.  If you absolutely must.
   wait 4 s
   say Or a diabolist, even.  Seriously.  They're all the same, painted dark purple and wearing old umbrellas.
   wait 4 s
   say Yes, there IS a necromancer nearby.  I should think the odor would have given him away to you long ago.
   wait 4 s
   say Go look around the creepy nasty parts of town.  No, NOT underground!  This is Asiri we're talking about here!
   say He's vain and no doubt sips blood in a musty parlour, but he won't stand for mold.
   wait 4 s
   say I suppose a run-down shack of some kind would suit him.  I've never been to visit.  Never will.
elseif %actor.class% /= Sorcerer && %actor.level% < 10
   say You want to know about evil, scheming death mages?  A fine young sorcerer like yourself?
   wait 4 s
   say I canNOT imagine why.  They lead frightful lives.  Entertained and served by corpses.
   wait 3 s
   shudder
   wait 3 s
   say At any rate, Asiri won't do a thing for you at your age.  So you can just put the idea out of your mind.
   wait 4 s
   say Preferably forever.
elseif %actor.class% /= Necromancer
   msend %actor% %self.name% looks you over.
   mechoaround %actor% %self.name% looks %actor.name% over.
   wait 4 s
   say Indeed.  I suppose you are.  Well.
   wait 4 s
   say It can't be helped, can it?  You're... well, you're what you are, aren't you.
   wait 4 s
   say If you're looking for your guild, I can only assure you that it's nowhere nearby.
   wait 4 s
   say Well, it can't be TOO far away, as I can smell the rot of old Asiri.  It drifts over half the town.
   wait 3 s
   gag
   wait 3 s
   say Why don't you look for a few undead folks?  Asiri seems to discard as many corpses as he employs.
else
   say You smell it, too?
   wait 4 s
   say I'm going to start a petition to have Asiri evicted from our fine town.
   wait 4 s
   say I hope I can count on you to sign promptly.
   wait 4 s
   eyebrow %actor.name%
end
~
#3008
Archmage responds to 'illusionist'~
0 n 100
illusionist~
wait 1 s
if %actor.class% /= Sorcerer && %actor.level% > 9 && %actor.level% < 46
   say If you want to become an illusionist, you might want to talk to the Grand Master in the citadel.
   wait 3 s
   say Unfortunately, he has been in a foul mood of late.  He may not agree to help you.  But I know of no one else.
elseif %actor.class% /= Sorcerer && %actor.level% < 10
   say Illusionists?  Crafty folk, you can never be quite sure what's going on around them.
   wait 3 s
   say You could become one if you wanted, but you'll have to wait until you have achieved level 10.
elseif %actor.class% /= Illusionist
   say Are you looking for the guild?  I hear that they place crafty illusions over the entrances.  But only visual ones...
   wait 3 s
   say They always seem to be near banks, as well.  Sorry, I don't know anything more about them.
else
   say I always used to take along an illusionist when adventuring.  You never know when they'll come in handy!
end
~
#3009
Erasmus responds to "charm person"~
0 n 100
charm~
wait 2 s
if %actor.class% /= Illusionist
   if %actor.skill[charm person]% == 100
      say A nice spell, isn't it?  I hope you're enjoying it.
   elseif %actor.level% < 17
      say Ah, charm person.  A powerful spell, capable of great evil - or good.
      wait 2 s
      say Come back when you have a bit more experience, and I'll teach it to you.
   else
      say Oh, the charm person spell.  You're ready to learn that one!
      wait 2 s
      say Stand still there while I teach you.
      wait 2 s
      emote launches into a marathon of lectures and speeches about diverse arcana.
      wait 5 s
      emote is still spewing forth obscure knowledge at a furious rate!
      wait 5 s
      emote seems to be winding down, as if the lesson were almost over.
      wait 5 s
      if %actor.room% == %self.room%
         say I'm impressed with your attentiveness, %actor.name%.
         wait 3 s
         say I can see that you've learned charm person rather well.
         mskillset %actor% charm person
      else
         say Eh?  Where did %actor.name% go?  Must not have a thirst for knowledge!
      end
   end
elseif %actor.class% /= Sorcerer
   if %actor.skill[charm person]% == 100
      gasp
      wait 3 s
      say What's this?  An ordinary sorcerer with charm person?!
      wait 5 s
      say Will wonders never cease!
      wait 5 s
      shake
   else
      say Don't look at me!  I'm a legendary teacher, but some things are just beyond me!
      wait 5 s
      say You sorcerers require extra help learning this... rather simple spell.
      wait 5 s
      say And you'll get none of it for me, because I have better things to do.
   end
else
   sigh
   wait 3 s
   say Yes, yes, a wonderful spell, how incredible we illusionists are, certainly.
   wait 3 s
   roll
   wait 3 s
   say Why don't you go bash some poor rodents' heads in.  Or whatever it is you do.
end
~
#3010
Baked with love~
1 s 100
~
Nothing.
~
#3011
lets do get help~
0 k 50
~
roar
wait 2
em yells out for assistance from anyone that can help!
wait 2
mload mob 3052
mecho A half-elven town guard arrives so quickly, you're not sure what direction he came from.
~
#3025
Magistrate_intro~
0 g 15
~
if %actor.vnum% == -1
   msend %actor%
   wait 5
   msend %actor% The Templar magistrate looks up at you briefly, then returns to his duties.
else
endif
~
#3026
test-random~
0 d 100
random~
* This is a test to generate a random number to be used
* in many ways
say My trigger commandlist is not complete!
set random_number %random.100%
if %random_number% >=51
say We're loading object.
else
say We're not loading object.
endif
say %random_number%
set mob %self.mexists[3055]%
set obj %self.oexists[1127]%
msend %actor% There are %mob% Druidic guards of 3055 in the game.
msend %actor% There are %obj% iron-banded girth's in the game.
~
#3027
quest_banter_magistrate1~
0 d 1
hi hello quest greetings~
wait 2
msend %actor% %self.name% says to you, "Greetings, %actor.name%.  It is good too see you."
msend %actor% %self.name% says to you, "I fear that I have little time to banter, for as you can see,"
msend %actor% %self.name% says to you, "I must prepare our defenses for yet another of these attacks."
~
#3028
quest_banter_magistrate2~
0 d 100
attack attacks~
wait 1
mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
msend %actor% %self.name% says to you, 'Yes yes.. That accursed Demon Lord who holds dominion over the'
msend %actor% %self.name% says to you, 'northern fortress of Mystwatch, and his fat headed general too.'
sigh
wait 1
msend %actor% %self.name% says to you, 'If only we could be rid of them once and for all, maybe these'
msend %actor% %self.name% says to you, 'attacks would stop.  Will you help rid us of this curse?'
~
#3029
quest_banter_magistrate3~
0 d 100
yes help~
if %actor.vnum% == -1
   wait 1
   * If any of the quest guys have spawned in Mystwatch don't give another totem.
   if %get.mob_count[16008]% || %get.mob_count[16010]% || %get.mob_count[16011]% || %get.mob_count[16015]% || %get.mob_count[16016]% || %get.mob_count[16017]% || %get.mob_count[16018]% || %get.mob_count[16019]%
      mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
      msend %actor% %self.name% says to you, 'Splendid, however, someone is currently after the Demon Lord's hide.'
      msend %actor% %self.name% says to you, 'You will have to wait until they are finished or if they fail you can finish for them.'
   else
      mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
      msend %actor% %self.name% says to you, 'Splendid, Mielikki be praised that one valiant enough'
      msend %actor% %self.name% says to you, 'has come amongst us to help rid us of this nuisance.'
      wait 1
      think
      msend %actor% %self.name% says to you, 'Here give this to that rat general in Mystwatch and'
      msend %actor% %self.name% says to you, 'it should start you on your way.'
      mload obj 3026
      give totem %actor.name%
   endif
endif
~
#3030
Myst_quest_reward~
0 j 100
~
if %actor.vnum% == -1
   if %object.vnum% == 16023
      wait 1
      mjunk shard
      * Create a random number by which to judge prize table off of.
      set rnd %random.100%
      if %rnd% >=99
         mload obj 1209
         shout Hark all and listen, %actor.name% has fended off a great evil for our fair town!
         wait 1
         say Here, %actor.name% you have earned this as a reward.
         wait 1
         give all %actor.name%
      elseif %rnd% < 99 && %rnd% >= 75
         mload obj 3040
         shout Hark all and listen, %actor.name% has fended off a great evil for our fair town!
         wait 1
         say Here, %actor.name% you have earned this as a reward.
         wait 1
         give all %actor.name%
      elseif %rnd% < 75 && %rnd% >=45
         set rnd_p %random.9%
         if %rnd_p% == 1
            mload obj 8342
         elseif %rnd_p% == 2
            mload obj 3051
         elseif %rnd_p% == 3
            mload obj 3216
         elseif %rnd_p% == 4
            mload obj 5207
         elseif %rnd_p% == 5
            mload obj 7302
         elseif %rnd_p% == 6
            mload obj 11701
         elseif %rnd_p% == 7
            mload obj 11702
         elseif %rnd_p% == 8
            mload obj 17808
         elseif %rnd_p% == 9
            mload obj 20014
         else
            Say This quest reward has a broken potion random chance, let a god know.
         endif
         shout Hark all and listen, %actor.name% has fended off a great evil for our fair town!
         wait 1
         say Here, %actor.name% you have earned this as a reward.
         wait 1
         give all %actor.name%
      elseif %rnd% < 45
         mload obj 3041
         shout Hark all and listen, %actor.name% has fended off a great evil for our fair town!
         wait 1
         say Here, %actor.name% you have earned this as a reward.
         wait 1 else
         give all %actor.name%
      else
         say The reward for this quest is broken, contact a god and let them know.
      endif
      if %get.mob_count[16007]% < 1
         mat 16064 mload mob 16007
      else
      endif
   else
      wait 1
      Say Thanks!
   endif
else
endif
~
#3034
TCD_Entrance~
2 c 100
knock~
wsend %actor% A dark and sinister looking portal opens suddenly and pulls you inside.
wechoaround %actor% A dark and sinister looking portal opens suddenly and pulls %actor.name% inside.
wdoor 3034 up room 18800
wforce %actor% up
wdoor 3034 up purge
~
#3040
blob-to-pawnbroker~
0 bd 1
drop off the junk~
   mgoto 6172
   mjunk all.key
   mteleport anduin_pawnbroker 6172
   remove all
   drop all
   mforce pawnbroker get all
   mteleport anduin_pawnbroker 6034
   set randmielrm %random.18%
   eval randloadrm %randmielrm% + 3051
   mgoto %randloadrm% 
~
#3041
blob_nofight~
0 k 100
~
set room %self.room%
emote seems to disintegrate, melting into the ground.
mteleport %self% 1100
wait 5s
mteleport %self% %room%
mecho Green gelatin suddenly bubbles out of the ground, wriggling into a blob.
~
#3050
set breathe fire~
0 o 100
~
mskillset %self% breathe fire 0
mskillset %self% fire breath 0
~
#3075
MIelikki South Guard~
0 c 100
south~
if %actor.vnum% == -1
if %actor.level% < 30
whisper %actor.name% You are much too little to venture south of here.
wait 1
whisper %actor.name% Try other areas first.
nudge %actor.name%
emote points at the sign.
else
return 0
endif
else
return 0
endif
~
#3076
test_quest_banter1~
0 n 1
hi hello howdy quest~
if !%actor.has_completed[zzurs_funky_quest]%
say I wonder if my brother the Northern road ranger is still alive?
sigh
say If only I had some proof of his doings.
say Will you help me?
else
end
~
#3077
test_quest_rece_south~
0 j 100
~
If %actor.vnum% == -1
   if %object.vnum% == 3080
      if %actor.quest_stage[zzurs_funky_quest]% == 4
         wait 1
         gasp
         mjunk necklace
         say My brother is alive! woo hoo!
         wait 2
         mload obj 16009
         give shield %actor.name%
         Say take that as an expression of my grattitude!
	 quest complete zzurs_funky_quest %actor.name%
      else
	 wait 1
         say Yes! My brother is alive!
         wait 1
         say Thank you!
         mjunk necklace
      end
   else
   eye %actor.name%
   end
else
end
~
#3078
test_quest_speech_north~
0 n 1
brother~
if %actor.vnum% == -1
   if %actor.quest_stage[zzurs_funky_quest]% == 1
      msend %actor% %self.name% says to you, 'My brother is looking for me?'
      quest advance zzurs_funky_quest %actor.name%
   else
   end
else
end
~
#3079
test_quest_speech2_north~
0 n 1
yes~
if %actor.vnum% == -1
   if %actor.quest_stage[zzurs_funky_quest]% == 2
      msend %actor% %self.name% says to you, 'I would love to send him proof of my doings but I am so busy.'
      sigh
      msend %actor% %self.name% says to you, 'I need a diadem of bone.'
      quest advance zzurs_funky_quest %actor.name%
   else
   end
else
end
~
#3080
test_quest_rece_north~
0 j 100
~
if %actor.vnum% == -1
   if %object.vnum% == 36023
      if %actor.quest_stage[zzurs_funky_quest]% == 3
         wait 1
         grin
         Say oh yea!
         wait 1
         thank %actor.name%
         Say This will help me immensely
         mjunk diadem
         mload obj 3080
         wait 3
         say Give this to my brother he will know I am alive.
         give necklace %actor.name%
         : moonwalks about the place.
         : does Michael Jackson.
         say Aren't test quests lovely?
         quest advance zzurs_funky_quest %actor.name%
      else
         wait 1
         Say wow thanks I've been looking for this.
      end
   else
   wait 1
   eye %actor.name%
   end
else
end
~
#3081
test_quest_speech1_south~
0 d 1
yes yea sure~
* the quest begins
if %actor.quest_stage[zzurs_funky_quest]% < 1
quest start zzurs_funky_quest %actor.name%
wait 1
msend %actor% You have begun Zzur's test quest!
wait 1
msend %actor% %self.name% says to you, 'Great! He's somewhere north of Mielikki!'
else
end
~
#3082
test_quest_ask3_north~
0 n 1
hi hello howdy quest~
wait 1
eye %actor.name%
mechoabout %actor% %self.name% speaks to %actor.name% in a low voice.
msend %actor% %self.name% says to you, 'Greetings, any news traveler?'
~
#3091
Rhode's Pool of Blood~
1 j 100
~
wait 15
oecho %self.shortdesc%  creates a small pool of blood on the ground.
if (%actor.vnum% == -1)
oload obj 34
end
wait 2
return 0
~
#3092
Blocking .... Test~
0 c 100
~
if (%actor.class% == mercenary)
mforce %actor.name% e
else
em humiliates you by blocking your path!!!!!
end
~
#3093
research_assistant_random~
0 b 15
~
sigh
say She was suppose to be here already.
wait 20
say She must be lost.
wait 20
say Bigby's not going to be happy about this.
~
#3094
bigby_assistant_relocate~
0 d 100
lost~
if (%actor.quest_stage[relocate_spell_quest]% < 1)
   if (%actor.level% >=65)
      if (%actor.class% == sorcerer || %actor.class% == cryomancer || %actor.class% == pyromancer)
         say Yes, one of our research mages.
         wait 15
         sigh
         say She was researching a very powerful transportation spell.
         wait 15
         say I believe Bigby called the spell 'Relocate'.
         wait 15
         say But she hasn't returned yet. Can you please find her?
         say She mentioned going to a dark desert to find something.
         say But that is all I know. I'm sorry I can not help more.
         say Please find her!
      else
         say I'm sorry but this doesn't concern you. Bigby would be very upset.
         shake
      end if
   else
      say I'm sorry young one, but I don't think you can help just yet.
      pat %actor.name%
   end if
else
   say Yes Yes, I heard that you found her! Please bring her back safely.
end if
~
#3095
druid_moonwell_clue~
0 b 8
~
sigh
wait 15
say Such a shame she had to be punished.
~
#3096
druid_moonwell_clue_answer~
0 d 100
punished~
if (%actor.class% == druid)
   if (%actor.level% >= 73)
      wait 15
      say Yes, yes, but hopefully one day she'll be able to leave that tree.
      wait 15
   else
      wait 15
      say I'm sorry, but you are still too young.
      wait 5
      smile %actor.name%
      ruffle %actor.name%
   end if
else
  say I'm sorry, but it's okay.  Don't worry about it.
  smile %actor.name%
end if
~
#3103
parrot_repeat~
0 bd 100
cracker~
say %arg%
say hi
~
#3109
leather_ball_bounce~
1 c 1
bounce~
return 1
if %arg%
   if %actor.room% != %arg.room%
      osend %actor% Bounce the ball on who?  They don't seem to be here!
   else
      oteleport %arg% 1100
      oechoaround %actor% %actor.name% throws %self.shortdesc% at %arg.name%, bouncing it off %arg.p% forehead!
      oteleport %arg% %actor.room%
      osend %arg% %actor.name% throws %self.shortdesc% at you, bouncing it off your forehead!
      osend %actor% You launch %self.shortdesc% at %arg.name%, bouncing it off %arg.p% forehead!
   endif
else
   oechoaround %actor% %actor.name% bounces %self.shortdesc% on the ground a few times.
   osend %actor% You dribble %self.shortdesc% around yourself a little bit.
endif
~
#3112
bird_whistle~
1 c 1
whistle~
return 1
oechoaround %actor% %actor.name% places a bird whistle to %actor.p% lips and lets out a twitter.
osend %actor% You blow on a bird whistle, making a twittering noise.
~
#3113
bird_whistle_normalize~
1 c 1
whis~
return 0
* This trigger forces the default action for "whis" instead of activating the
* whistle trigger.
~
#3114
finger_trap_remove~
1 l 100
~
if %actor.level% < 100
  if %random.200% > %actor.real_int%
    return 0
    oechoaround %actor% %actor.name% struggles, trying to remove a finger trap!
    osend %actor% You struggle, but can't seem to remove the finger trap!
  endif
endif
~
#3115
UNUSED~
1 c 1
re~
return 0
* This trigger is to make 're' do rest instead of remove
~
#3116
UNUSED~
1 j 100
~
if ((%actor.worn[1]% == -1) && (%actor.worn[2] == -1))
    return 1
else
    return 0
    osend %actor% You need two fingers free to wear this.
endif
~
#3117
squeeze_normalizer~
1 c 1
s~
* This trigger makes sure the command "s" does its normal function,
* instead of triggering the squeeze trigger.
return 0
~
#3118
squeeze_zzur~
1 c 1
squeeze~
if (%arg% != plushie) && (%arg% != zzur-plushie) && (%arg% != zzur) && (%arg% != toy) && (%arg% != little-plushie) && (%arg% != little)
   return 0
   halt
endif
return 1
switch %random.9%
   case 1
      set phrase Done yet?
      break
   case 2
      set phrase Be nice to the newbies.
      break
   case 3
      set phrase AFK.
      break
   case 4
      set phrase Uh huh.
      break
   case 5
      set phrase Thoughts? Suggestions?
      break
   case 6
      set phrase PHRASE6
      break
   case 7
      set phrase PHRASE7
      break
   case 8
      set phrase PHRASE8
      break
   case 9
      set phrase PHRASE9
      break
   default
      set phrase PHRASE0
      break
done
oecho %actor.name% squeezes the Zzur plushie's belly.
oecho The Zzur plushie says, '%phrase%'
~
#3119
press_normalizer~
1 c 1
pr~
* This trigger makes sure the command "pr" does its normal function,
* instead of triggering the press trigger.
return 0
~
#3120
press_kourrya~
1 c 1
press~
if (%arg% != button) && (%arg% != figurine) && (%arg% != kourrya-figurine) && (%arg% != figurine) && (%arg% != toy) && (%arg% != tiny) && (%arg% != figure) && (%arg% != robed) && (%arg% != woman)
   return 0
   halt
endif
return 1
switch %random.9%
   case 1
      set phrase Find it yourself.
      break
   case 2
      set phrase Finish your existing quest first.
      break
   case 3
      set phrase Your soul belongs to me!
      break
   case 4
      set phrase PHRASE4
      break
   case 5
      set phrase PHRASE5
      break
   case 6
      set phrase PHRASE6
      break
   case 7
      set phrase PHRASE7
      break
   case 8
      set phrase PHRASE8
      break
   case 9
      set phrase PHRASE9
      break
   default
      set phrase PHRASE0
      break
done
oecho %actor.name% presses the button in the Kourrya figurine's back.
oecho The Kourrya figurine says, '%phrase%'
~
#3121
pinch_normalizer~
1 c 1
pi~
* This trigger makes sure the command "pi" does its normal function
* instead of triggering the pinch trigger.
return 0
~
#3122
pinch_laoris~
1 c 1
pinch~
if (%arg% != puppet) && (%arg% != laoris) && (%arg% != laoris-puppet) && (%arg% != toy) && (%arg% != little) && (%arg% != colorful) && (%arg% != hand) && (%arg% != hand-puppet)
   return 0
   halt
endif
return 1
switch %random.9%
   case 1
      set phrase Rarr!
      break
   case 2
      set phrase Hey want to try my mob's new combat proc?
      break
   case 3
      set phrase Teeehehehehehe!
      break
   case 4
      set phrase PHASE4
      break
   case 5
      set phrase PHRASE5
      break
   case 6
      set phrase PHRASE6
      break
   case 7
      set phrase PHRASE7
      break
   case 8
      set phrase PHRASE8
      break
   case 9
      set phrase PHRASE9
      break
   default
      set phrase PHRASE0
      break
done
oecho %actor.name% pinches the Laoris puppet.
oecho The Laoris puppet says, '%phrase%'
~
#3123
squeeze_ehlissa~
1 c 1
squeeze~
if (%arg% != doll) && (%arg% != ehlissa) && (%arg% != miniature-ehlissa-doll) && (%arg% != miniature) && (%arg% != little) && (%arg% != little-ehlissa-doll) && (%arg% != ehlissa-doll)
   return 0
   halt
endif
return 1
oecho %actor.name% squeezes her doll.
set i 99
while %i% > 0
   oecho %self.shortdesc% sings, '%i% bottles of beer on the wall...'
   wait 3s
   oecho %self.shortdesc% sings, '%i% bottles of beer...'
   wait 3s
   oecho %self.shortdesc% sings, 'Take one down, pass it around...'
   wait 3s
   eval i %i% - 1
   oecho %self.shortdesc% sings, '%i% bottles of beer on the wall!'
   wait 7s
done
~
#3124
pat_normalizer~
1 c 1
pa~
* This trigger makes sure the command "pa" does its normal function
* instead of triggering the pat trigger.
return 0
~
#3125
pat_chinok~
1 c 1
pat~
eval test1 (%arg% != doll) && (%arg% != chinok) && (%arg% != chinok-doll) && (%arg% != chinok-rag) && (%arg% != rag-doll) && (%arg% != rag) && (%arg% != chinok-rag-doll)
eval test2 (%arg% != little) && (%arg% != hooded) && (%arg% != figure) && (%arg% != little-hooded) && (%arg% != hooded-figure) && (%arg% != little-hooded-figure)
if %test1% && %test2%
   return 0
   halt
endif
return 1
oechoaround %actor% %actor.name% pats a Chinok rag doll on its head.
osend %actor% You pat a Chinok rag doll on its head.
oecho The Chinok rag doll swings its lightsabers dangerously, narrowly missing you!
~
#3130
Bells Exit~
1 q 100
~
oechoaround %actor% %actor.name% bells ring as she leaves the room.
osend %actor% The bells on your hat ring as you walk.
~
#3131
Bells Entry~
1 i 100
~
oechoaround %actor% %actor.name% bells ring as she enters the room.
~
#3133
Random Bells~
1 bgjl 100
~
if %actor%
    * this is a wear trigger
    set myowner %actor%
    global myowner
    oechoaround %myowner% Bells ring on %myowner.name%'s hat as %myowner.n% moves.
    osend %myowner% The bells on your hat ring as you shift your position.
else
    * this is a random trigger
    if %myowner%
        if %myowner.wearing[3306]%
            * the global variable was set (by the wear trigger)
            oechoaround %myowner% Bells ring on %myowner.name%'s hat as %myowner.n% moves.
            osend %myowner% The bells on your hat ring as you shift your position.
        else
            * hat not worn, so assume it was removed
            unset myowner
        end
    end
end
~
#3140
Freddy random load doll~
0 bd 1
refresh~
* Generate a random number
set p %random.15%
* If this is being triggered by an imm as a speech trig, ensure something loads
if %actor% && %actor.level% > 100
  set p 15
endif
if %p% != 15
  halt
endif
 
* Figure out which object to load.  Better chance for lower vnums.
set p %random.100%
if %p% < 31
  eval p 3139 + %random.7%
elseif %p% < 82
  eval p 3146 + %random.35%
elseif %p% < 93
  eval p 3181 + %random.10%
else
  eval p 3191 + %random.7%
endif
 
* In case one of the randoms returned 0 somehow
if %p% == 3139
  set p 3140 + %random.58%
endif
mload obj %p%
 
* Special handling for certain dolls:
* Imanhotep's sarcophagus
if %p% == 3182
  mload obj 3199
  mat 1100 open sarcophagus
  mat 1100 put imanhotep sarcophagus
  mat 1100 close sarcophagus
* The Chosen boxed set
elseif %p% == 3198
  mload obj 3175
  mload obj 3176
  mload obj 3177
  mload obj 3178
  mload obj 3179
  mat 1100 open boxed-set
  mat 1100 put chosen boxed-set
  mat 1100 put chosen boxed-set
  mat 1100 put chosen boxed-set
  mat 1100 put chosen boxed-set
  mat 1100 put chosen boxed-set
  mat 1100 close boxed-set
endif
~
#3150
Intercity transport questmasters greet~
0 g 100
~
* This is a greet trigger for the transportation assistants who
* move newbies between cities.
switch %self.vnum%
case 30075
case 3151
   set myname barbarian
   break
case 3152
   set myname dwarf
   break
case 30074
   set myname drow
   break
case 30076
   set myname human
   break
case 30077
   set myname orc
   break
case 3150
default
   set myname elf
   break
done
if %actor.level% < 16 && %actor.vnum% == -1 && %actor.quest_stage[intercity_transport]% == 0
   msend %actor% &bThe %myname% tells you, 'Would you like a trip to a faraway city?'&0
   msend %actor% &bThe %myname% tells you, 'If so, just ask me &7&bYes&0&b and I'll tell you all about it.'&0
end
~
#3151
Intercity transport questmasters talk~
0 n 100
yes Yes help Help tell~
if %actor.quest_stage[intercity_transport]% == 0
   quest start intercity_transport %actor.name%
endif
switch %self.vnum%
   case 3151
   case 30075
      set myname barbarian
      set dest Ickle
      break
   case 3152
      set myname dwarf
      set dest Anduin
      break
   case 30074
      set myname drow
      set dest Anduin
      break
   case 30076
      set myname human
      set dest Mielikki
      break
   case 30077
      set myname orc
      set dest Ogakh
      break
   case 3150
   default
      set myname elf
      set dest Mielikki
done
wait 3
if %actor.level% < 16
   msend %actor% &bThe %myname% tells you, 'As long as you're under level 16, I can transport you to %dest%.'&0
   msend %actor% &bThe %myname% tells you, 'Just ask me 'transport', and I'll send you.'&0
else
   msend %actor% &bThe %myname% tells you, 'I send people below level 16 to %dest%.'&0
   msend %actor% &bThe %myname% tells you, 'Unfortunately, you are too powerful.'&0
end
~
#3152
(UNUSED) Receive trigger for transport~
0 j 100
~
* This is the receive trigger for intercity transport questmasters.
* There are two systems of payment.  In the credit system, the scales
* are worth different values.  When 9 credits are submitted, transport
* takes place.
* In the token system, a single scale is payment for transport.
** Currently, the scales are used as follows:
**
** white  (vnum 3150) ... 1 credit
** blue   (vnum 3151) ... 3 credits
** amber  (vnum 3152) ... 9 credits
** black  (vnum 3153) ... token for Nymrill
** violet (vnum 3154) ... token for Dancing dolphin inn
**
** The credit scales are used for going to Mielikki, Anduin, and Ickle.
* The following switch statement has one case per transportation agent type.
* By setting tokenid to a particular object vnum, the case statement
* causes that agent to accept a single object of that type as full payment.
* When tokenid is 0, the agent goes with the credits system.
* Don't set maxlevel over 99. These guys cannot transport immortals, even
* if we wanted them to.
*******************
** GENERAL SETUP **
*******************
switch %self.vnum%
   * This is the barbarian, who sends people to Ickle.
   case 3151
      set myname barbarian
      set dvnum 10046
      set dname ickle
      set dsname Ickle
      set tokenid 0
      set minlevel 1
      set maxlevel 15
      break
   case 3152
   * This is the dwarf, who sends people to Anduin.
      set myname dwarf
      set dvnum 6015
      set dname anduin
      set dsname Anduin
      set tokenid 0
      set minlevel 1
      set maxlevel 15
      break
   case 3153
   * This is the human, who sends people to the Dancing Dolphin Inn.
      set myname human
      set dvnum 58704
      set dname dolphin
      set dsname the Dancing Dolphin Inn
      set tokenid 3154
      set minlevel 40
      set maxlevel 99
      break
   case 3154
   * This is the gnome, who sends people to Nymrill.
      set myname gnome
      set dvnum 49501
      set dname nymrill
      set dsname Nymrill
      set tokenid 3153
      set minlevel 60
      set maxlevel 99
      break
   case 3150
   default
   * This is the elf, who sends people to Mielikki.
      set myname elf
      set dvnum 3016
      set dname mielikki
      set dsname Mielikki
      set tokenid 0
      set minlevel 1
      set maxlevel 15
      break
done
******************************
** Check for proper payment **
******************************
set credit_value 0
switch %object.vnum%
   case 3150
      set credit_value 1
      break
   case 3151
      set credit_value 3
      break
   case 3152
      set credit_value 9
      break
done
if %tokenid% != 0
   * Here we have an agent who wants a specific item.
   if %object.vnum% == %tokenid%
      set credit_value 9
   else
      set credit_value 0
   end
endif
if %credit_value% == 0
   return 0
   mechoaround %actor% %actor.name% tries to give %object.shortdesc% to %self.name%.
   msend %actor% %self.name% refuses to take %object.shortdesc%.
   wait 2
   peer %actor.name%
   msend %actor% &bThe %myname% tells you, 'I have no use for this!'&0
   halt
end
***************************
** Check for eligibility **
***************************
if %actor.level% > %maxlevel%
   return 0
   mechoaround %actor% %actor.name% tries to give %object.shortdesc% to %self.name%.
   msend %actor% %self.name% refuses to accept %object.shortdesc%.
   wait 2
   eyebrow %actor.name%
   msend %actor% &bThe %myname% tells you, 'You are too powerful for me to transport.'&0
   halt
elseif %actor.level% < %minlevel%
   return 0
   mechoaround %actor% %actor.name% tries to give %object.shortdesc% to %self.name%.
   msend %actor% %self.name% ignores your offer.
   wait 2
   shake
   msend %actor% &bThe %myname% tells you, 'You are not experienced enough to go to %dsname%.'&0
   halt
endif
*********************
** Process payment **
*********************
*** Check for unnecessary payment ***
if %minlevel% == 1 && %actor.level% < 6
   return 0
   mechoaround %actor% %actor.name% tries to give %object.shortdesc% to %self.name%.
   msend %actor% %self.name% briskly refuses your offer.
   wait 4
   msend %actor% &bThe %myname% tells you, 'You don't need to pay me yet.  Just ask me 'transport','&0
   msend %actor% &bThe %myname% tells you, 'and I'll send you on your way.'&0
   halt
end
mjunk %object.name%
* Now do not attempt to access *object* below here!
wait 4
if %tokenid% != 0
   * A specific token is always worth the full price, so don't talk about credits.
   mechoaround %actor% %self.name% nods briefly to %actor.name%.
   msend %actor% %self.name% nods briefly to you.
else
   * Work with the credits.
   * Ensure we have the quest variable for keeping track of submissions to this agent.
   if %actor.quest_variable[intercity_transport:%dname%]%
   else
      quest variable intercity_transport %actor.name% %dname% 0
   end
   eval total_credits %credit_value% + %actor.quest_variable[intercity_transport:%dname%]%
   if %credit_value% == 1
      msend %actor% &bThe %myname% tells you, 'That's worth one credit.'&0
   else
      msend %actor% &bThe %myname% tells you, 'That's worth %credit_value% credits.'&0
   end
   if %total_credits% < 9
      quest variable intercity_transport %actor.name% %dname% %total_credits%
      eval credits_needed 9 - %total_credits%
      if %credits_needed% == 1
         msend %actor% &bThe %myname% tells you, 'One more credit and I'll send you.'&0
      else
         msend %actor% &bThe %myname% tells you, '%credits_needed% more credits and I'll send you.'&0
      end
      halt
   end
   eval remaining_credits %total_credits% - 9
   msend %actor% &bThe %myname% tells you, 'Alright then, off you go!'&0
   if %remaining_credits% == 1
      msend %actor% &bThe %myname% tells you, 'You have one credit left over.'&0
   elseif %remaining_credits% > 1
      msend %actor% &bThe %myname% tells you, 'You have %remaining_credits% credits left over.'&0
   end
   quest variable intercity_transport %actor.name% %dname% %remaining_credits%
end
***************************
** Set origination color **
***************************
* The color says where they came from.
* It is seen by anyone standing at the destination.
if %self.room% == 3016
   * Mielikki - GREEN
   set dcolor &2green&0
elseif %self.room% == 6015
   * Anduin - RED
   set dcolor &1red&0
elseif %self.room% == 49501
   * Nymrill - BLACK
   set dcolor &9&bblack&0
elseif %self.room% == 58704
   * Dancing Dolphin Inn - PURPLE
   set dcolor &5purple&0
elseif %self.room% == 16011
   * Mystwatch - GRAY
   set dcolor &8&bgray&0
else
   * Ickle - BLUE
   set dcolor &4blue&0
end
   
***********************
** Perform transport **
***********************
wait 4
mechoaround %actor% %self.name% makes a magical gesture at %actor.name%.
msend %actor% %self.name% makes a magical gesture at you.
mechoaround %actor% %actor.name% disappears in a cloud of gray smoke.
mteleport %actor% %dvnum%
mat %dvnum% mechoaround %actor% %actor.name% arrives in a cloud of %dcolor% smoke.
mforce %actor% look
~
#3153
(UNUSED) DDI agent calling out~
0 bg 20
~
mecho The purple-jacketed man cries out "Tropical paradise vacation!  Get away from the snow!"
~
#3154
Transport agents ask~
0 n 100
transport~
* This is the ask trigger for intercity transport questmasters.
* It allows level 1-15 people to get transport by asking to
* Anduin, Ickle, or Mielikki.
* Much of this trigger is a copy of 3152.
*******************
** GENERAL SETUP **
*******************
set minlevel 1
set maxlevel 15
switch %self.vnum%
   * This is the barbarian, who sends people to Ickle.
   case 3151
   case 30075
      set myname barbarian
      set dvnum 10046
      set dsname Ickle
      break
   case 3152
   * This is the dwarf, who sends people to Anduin.
      set myname dwarf
      set dvnum 6015
      set dsname Anduin
      break
   case 30074
      set myname drow
      set dvnum 6015
      set dsname Anduin
      break
   case 30076
      set myname human
      set dvnum 3016
      set dsname Mielikki
      break
   case 30077
      set myname orc
      set dvnum 30115
      set dsname Ogakh
      break
   case 3150
   default
   * This is the elf, who sends people to Mielikki.
      set myname elf
      set dvnum 3016
      set dsname Mielikki
      break
done
wait 4
***************************
** Check for eligibility **
***************************
if %actor.level% > 99
   eyebrow %actor.name%
   msend %actor% &bThe %myname% tells you, 'Just goto %dvnum%.'
   halt
elseif %minlevel% > %actor.level%
   shake
   msend %actor% &bThe %myname% tells you, 'You are not experienced enough to go to %dsname%.'&0
   halt
elseif %actor.level% > %maxlevel%
   eyebrow %actor.name%
   msend %actor% &bThe %myname% tells you, 'You are too powerful for me to transport.'&0
   halt
endif
***************************
** Set origination color **
***************************
* The color says where they came from.
* It is seen by anyone standing at the destination.
if %self.room% == 3016
   * Mielikki - GREEN
   set dcolor &2green&0
elseif %self.room% == 6015
   * Anduin - RED
   set dcolor &1red&0
elseif %self.room% == 30115
   * Ogakh - GRAY
   set dcolor &9&bgray&0
else
   * Ickle - BLUE
   set dcolor &4blue&0
end
   
***********************
** Perform transport **
***********************
mechoaround %actor% %self.name% nods briefly to %actor.name%.
msend %actor% %self.name% nods briefly to you.
wait 4
mechoaround %actor% %self.name% makes a magical gesture at %actor.name%.
msend %actor% %self.name% makes a magical gesture at you.
mechoaround %actor% %actor.name% disappears in a cloud of gray smoke.
mteleport %actor% %dvnum%
mat %dvnum% mechoaround %actor% %actor.name% arrives in a cloud of %dcolor% smoke.
mforce %actor% look
~
#3155
(NOT USED) Spirit scales decay quickly~
1 ab 100
~
* This trigger is NOT USED because this system of trying to make
* scales decay rapidly is far too unreliable. It doesn't catch on
* to being picked up/put down very well. So instead, a timer is
* put on the scales. It's rather long, but the scales will decay
* even if held in inventory. Oh well.
*
* This is the trigger that causes spirit scales to decay rapidly
* if not taken.
oecho Scale: my room is %self.room%
if %now_held% != 1
   if %decay_counter% > 5
      opurge %self%
   elseif %decay_counter% > 0
      eval decay_counter %decay_counter% + 1
      global decay_counter
   else
      set decay_counter 1
      global decay_counter
   endif
endif
~
#3156
(NOT USED) Spirit scale stops decaying when taken~
1 g 100
~
* This trigger prevents transport quest spirit scales from
* decaying after they've been taken.
set now_held 1
global now_held
~
#3157
(NOT USED) Spirit scale decays again when dropped~
1 h 100
~
* This trigger causes transport quest spirit scales to decay
* again once dropped.
set decay_counter 1
global decay_counter
set now_held 0
global now_held
~
#3158
(UNUSED) Drop spirit scale on death~
0 f 100
~
* This trigger causes the mob that it's on to drop spirit scales
* for the newbie city transport quest, sometimes.
* Note: setting numeric arg on death trig seems to have no effect,
* so the randomness must be handled thus
if %random.10% < 4
   if %self.level% < 3 && %random.10% < 7
      set scale_vnum 3150
   elseif %self.level% < 6 && %random.10% < 7
      set scale_vnum 3151
   else
      set scale_vnum 3152
   endif
   mload obj %scale_vnum%
end
~
#3160
***skill trainer speach***~
0 dn 100
training yes backstab bludgeoning shadow slashing sneak steal switch throatcut track conceal corner double dual eye switch retreat hide pick piercing bandage douse~
*make sure they have a quest record for saving variables
if %actor.quest_stage[trainer_3160]% < 1
   quest start trainer_3160 %actor.name%
else
   quest restart trainer_3160 %actor.name%
endif
*introductions: lists the skills available
if %speech% == training? || %speech% == training || %speech% == yes
   say Sure, I can teach you just about any necessary fighting skill.
   say If youre interested, ask me about one of the skills I can teach you:
   say backstab, bludgeoning weapons, conceal, corner, double attack, dual wield, eye gouge,
   say retreat, group retreat, hide, pick lock, bandage, piercing weapons, shadow,
   say slashing weapons, sneak, steal, switch, throatcut, track, douse.
   halt
else
   *defining variables for this script
   set name %actor.name%
   set cha %actor.real_cha%
   set int %actor.real_int%
   *
   *Identify the skill to be trained
   if %speech% == backstab         
      set skill %actor.skill[backstab]%
   elseif %speech% /= bludgeoning weapons
      set skill %actor.skill[bludgeoning weapons]%
      set word2 weapons
   elseif %speech% == conceal           
      set skill %actor.skill[conceal]%
   elseif %speech% == corner             
      set skill %actor.skill[corner]%
   elseif %speech% == double attack 
      set skill %actor.skill[double attack]%
      set word2 attack
   elseif %speech% == dual wield
      set skill %actor.skill[dual wield]%
      set word2 wield
   elseif %speech% == eye gouge      
      set word2 gouge 
      set skill %actor.skill[eye gouge]%
   elseif %speech% == group retreat   
      set word2 retreat
      set skill %actor.skill[group retreat]%
   elseif %speech% == hide 
      set skill %actor.skill[hide]% 
   elseif %speech% ==instant kill 
      set skill %actor.skill[instant kill]%
      set word2 kill
   elseif %speech% == pick lock
      set skill %actor.skill[pick lock]%
      set word2 lock
   elseif %speech% == piercing weapons 
      set skill %actor.skill[piercing weapons]%
      set word2 weapons
   elseif %speech% == retreat 
      set skill %actor.skill[retreat]%    
   elseif %speech% == shadow           
      set skill %actor.skill[shadow]%
   elseif %speech% == slashing weapons       
      set skill %actor.skill[slashing weapons]%
      set word2 weapons
   elseif %speech% == sneak         
      set skill %actor.skill[sneak]%
   elseif %speech% == steal                
      set skill %actor.skill[steal]%
   elseif %speech% == switch             
      set skill %actor.skill[switch]%     
   elseif %speech% == throatcut          
      set skill %actor.skill[throatcut]%
   elseif %speech% == track                
      set skill %actor.skill[track]%
   elseif %speech% == bandage                
      set skill %actor.skill[bandage]%
   elseif %speech% == douse
      set skill %actor.skill[douse]%
   else
      say Im not familiar with %speech%.
      halt
   endif
   *
   * Check to see if a skill should be taught at all
   *
   eval skill 10 * %skill%
   eval maxskill %actor.level% * 10 + 60
   *check for capped skills
   eval cap 1000
   if %speech% == track
      switch %actor.class%
         case Mercenary
            eval cap 850
            break
         case Rogue
            eval cap 650
            break
         case Thief
            eval cap 600
            break
         case Assassin
            eval cap 750
      done
   elseif %speech% == dodge || %speech% == parry
      switch %actor.race%
         case ogre
         case troll
            eval cap 700
      done
   elseif %speech% == riposte
      switch %actor.race%
         case ogre
            set cap 700
      done
   endif
   if %skill% < 50
      say I wouldnt know where to start.
      say Lets talk about improving a skill you actually know.
      halt
   elseif %skill% >= %cap% || %skill% >= %maxskill%
      say There is nothing left to teach you. You've mastered %speech%!
      halt
   endif
   if %cap% < %maxskill%
      set maxskill %cap%
   endif
   * This portion is for smooth speech indicating the effects of a players cha or int score.
   if %cha% > 70
      say I like you.
      if %int% > 70
         say ..and youre pretty bright.
         say I'll give you a good deal.
      elseif %int% < 50
         say ..but you aren't the smartest.
      endif
   elseif %cha% < 50
      say I dont like you much.
      if %int% > 70
         say ..but you are pretty bright.
      elseif %int% < 50
         say ..and you aren't the smartest.
      else
      endif
   else
      say You're alright.
      if %int% > 70
         say ..and you're pretty bright.
      elseif %int% < 50
         say ..but you aren't the smartest.
      endif
   endif
   *Now a price is calculated
   eval change %maxskill% - %skill%
   eval opinion 200 - (%cha% + %int%)
   eval price %change% * %opinion% * %actor.level% * %actor.level% / (%skill% * 20)
   *
   *now the price in copper has to be divided into coinage.
   eval plat %price% / 1000
   eval gold %price% / 100 - %plat% * 10
   eval silv %price% / 10 - %plat% * 100 - %gold% * 10
   eval copp %price%  - %plat% * 1000 - %gold% * 100 - %silv% * 10
   *now the price can be reported
   say I'll teach you %speech% for %plat% platinum, %gold% gold, %silv% silver, %copp% copper.
   say Just bring me the money when youre ready to practice.
   quest variable trainer_3160 %actor.name% skill_name %speech%
   if %word2%
      quest variable trainer_3160 %actor.name% word2 %word2%
   endif
   quest variable trainer_3160 %actor.name% skill_level %skill%
   quest variable trainer_3160 %actor.name% price %price%
   quest variable trainer_3160 %actor.name% actor_level %actor.level%
endif
~
#3161
pay for training~
0 m 1
~
if %actor.quest_variable[trainer_3160:word2]%
   set full_skill %actor.quest_variable[trainer_3160:skill_name]% %actor.quest_variable[trainer_3160:word2]%
elseif  %actor.quest_variable[trainer_3160:skill_name]%
   set full_skill %actor.quest_variable[trainer_3160:skill_name]%
endif
wait 1s
if %actor.quest_variable[trainer_3160:actor_level]% == %actor.level%
   if %value% >= %actor.quest_variable[trainer_3160:price]%
      grin %actor.name%
      wait 1s
      say Fantastic. Let's get started..
      wait 5s
      msend %actor% Some time passes..
      wait 3s
      msend %actor% You feel you are getting the hang of things.
      wait 3s
      msend %actor% You feel your skill in %full_skill% improving dramatically!
      mskillset %actor% %full_skill%
      quest erase trainer_3160 %actor.name%
   else
      say I appreciate your volentary donation, but I'm afraid thats all it was.
      snicker %actor%
      say I don't accept installments. It's got to be all up front.
   endif
else
   return 0
   wait 2s
   consider %actor%
   wait 2s
   ponder
   wait 2s
   say Somethings different about you. What skill were you going to train again?
   quest erase trainer_3160 %actor.name%
endif
~
#3162
Julks greet~
0 g 100
~
wait 1s
if %actor.quest_stage[trainer_3160]% == 1
   if %actor.quest_variable[trainer_3160:actor_level]% == %actor.level%
      say Have you returned to be trained in %actor.quest_variable[trainer_3160:skill_name]%?
      halt
   else
      say You've been out adventuring, have you? Perhaps you could use some training?
   endif
else
   tell %actor.name% Greetings adventurer. Are you looking for training?
endif
~
#3163
julks fight trigger~
0 k 100
~
set mode %random.10%
set target %self.fighting%
set tank %target.fighting%
if %target%
   if %tank% != %self%
      if %mode% > 5
         backstab
      elseif %mode% > 2
         kick
      else
         corner
      endif
   elseif %tank% == %self%
      if %mode% > 6
         kick
      else
         bash
      endif
   endif
   wait 3s
endif
~
#3164
mrs julk fight trigger~
0 k 100
~
set mode %random.10%
set target %self.fighting%
set tank %target.fighting%
if %target%
   if %tank% != %self%
      if %mode% > 3
         backstab
      else
         kick
      endif
   elseif %tank% == %self%
      if %mode% > 6
         kick
      endif
   endif
   wait 3s
endif
~
#3165
Calken trainer speech~
0 d 100
training yes 2H dodge parry riposte hitall barehand bash disarm first guard kick mount rescue riding safefall springleap tame~
*make sure they have a quest record for saving variables
if %actor.quest_stage[trainer_3165]% < 1
   quest start trainer_3165 %actor.name%
else
   quest restart trainer_3165 %actor.name%
endif
*introductions: lists the skills available
if %speech% == training? || %speech% == training || %speech% == yes
   say Sure, I can teach you just about any necessary battle skill.
   say I am currently training all of the following: 2H bludgeoning weapons, 2H piercing weapons,
   say 2H slashing weapons, disarm, dodge, parry, riposte, hitall, barehand, springleap, safefall,
   say bash, guard, rescue, kick, first aid, mount, riding, tame.
   say Feel free to ask me about one of them and I'll give you a price.
   halt
else
   *defining variables for this script
   set speech %speech%
   set name %actor.name%
   set cha %actor.real_cha%
   set int %actor.real_int%
   *
   *Identify the skill to be trained
   if %speech% == bash                
      set skill %actor.skill[bash]%
   elseif %speech% /= 2H
      if %speech% /= bludgeoning
         set word2 bludgeoning
         set skill %actor.skill[2H bludgeoning weapons]%
      elseif %speech% /= piercing 
         set word2 piercing
         set skill %actor.skill[2H piercing weapons]%
      elseif %speech% /= slashing 
         set word2 slashing
         set skill %actor.skill[2H slashing weapons]%
      else
         say Not sure that I've heard of that one. What was it again?
         halt
      endif
   elseif %speech% == disarm
      set skill %actor.skill[disarm]%
   elseif %speech% == dodge
      set skill %actor.skill[dodge]%
   elseif %speech% == parry                
      set skill %actor.skill[parry]%
   elseif %speech% == riposte             
      set skill %actor.skill[riposte]%
   elseif %speech% == hitall             
      set skill %actor.skill[hitall]%
   elseif %speech% == barehand           
      set skill %actor.skill[barehand]%
   elseif %speech% == springleap             
      set skill %actor.skill[springleap]%
   elseif %speech% == safefall             
      set skill %actor.skill[safefall]%
   elseif %speech% == bash             
      set skill %actor.skill[bash]%
   elseif %speech% == guard
      set skill %actor.skill[guard]%   
   elseif %speech% == rescue            
      set skill %actor.skill[rescue]%
   elseif %speech% == kick                  
      set skill %actor.skill[kick]%
   elseif %speech% == first aid
      set word2 aid
      set skill %actor.skill[first aid]%
   elseif %speech% == mount
      set skill %actor.skill[mount]%
   elseif %speech% == riding            
      set skill %actor.skill[riding]%
   elseif %speech% == tame
      set skill %actor.skill[tame]%
   else
      say Im not familiar with %speech%.
      halt
   endif
   *
   * Check to see if a skill should be taught at all
   *
   eval skill 10 * %skill%
   eval maxskill %actor.level% * 10 + 60
   *check for capped skills
   eval cap 1000
   if %speech% == track
      switch %actor.class%
         case Mercenary
            eval cap 850
            break
         case Rogue
            eval cap 650
            break
         case Thief
            eval cap 600
            break
         case Assassin
            eval cap 750
      done
   elseif %speech% == dodge || %speech% == parry
      switch %actor.race%
         case ogre
         case troll
            eval cap 700
      done
   elseif %speech% == riposte
      switch %actor.race%
         case ogre
            set cap 700
      done
   endif
   if %skill% < 50
      say I wouldn't know where to start.
      say Let's talk about improving a skill you actually know.
      halt
   elseif %skill% >= %cap% || %skill% >= %maxskill%
      say There is nothing left to teach you. You've mastered %speech%!
      halt
   endif
   if %cap% < %maxskill%
      set maxskill %cap%
   endif
   * This portion is for smooth speech indicating the effects of a players cha or int score.
   if %cha% > 70
      say I like you.
      if %int% > 70
         say ..and you're pretty bright.
         say I'll give you a good deal.
      elseif %int% < 50
         say ..but you aren't the smartest.
      endif
   elseif %cha% < 50
      say I don't like you much.
      if %int% > 70
         say ..but you are pretty bright.
      elseif %int% < 50
         say ..and you aren't the smartest.
      else
      endif
   else
      say You're alright.
      if %int% > 70
         say ..and you're pretty bright.
      elseif %int% < 50
         say ..but you aren't the smartest.
      endif
   endif
   *Now a price is calculated
   eval change %maxskill% - %skill%
   eval opinion 200 - (%cha% + %int%)
   eval price %change% * %opinion% * %actor.level% * %actor.level% / (%skill% * 20)
   *
   *now the price in copper has to be divided into coinage.
   eval plat %price% / 1000
   eval gold %price% / 100 - %plat% * 10
   eval silv %price% / 10 - %plat% * 100 - %gold% * 10
   eval copp %price%  - %plat% * 1000 - %gold% * 100 - %silv% * 10
   *now the price can be reported
   say I'll teach you %speech% for %plat% platinum, %gold% gold, %silv% silver, %copp% copper.
   say Just get me the money, and I'll get started.
   quest variable trainer_3165 %actor.name% skill_name %speech%
   if %word2%
      quest variable trainer_3165 %actor.name% word2 %word2%
   endif
   quest variable trainer_3165 %actor.name% skill_level %skill%
   quest variable trainer_3165 %actor.name% price %price%
   quest variable trainer_3165 %actor.name% actor_level %actor.level%
endif
~
#3166
pay for training~
0 m 1
~
if %actor.quest_variable[trainer_3165:word2]%
   set full_skill %actor.quest_variable[trainer_3165:skill_name]% %actor.quest_variable[trainer_3165:word2]%
elseif %actor.quest_variable[trainer_3165:skill_name]%
   set full_skill %actor.quest_variable[trainer_3165:skill_name]%
endif
wait 1s
if %actor.quest_variable[trainer_3165:actor_level]% == %actor.level%
   if %value% >= %actor.quest_variable[trainer_3165:price]%
      grin %actor.name%
      wait 1s
      say Fantastic. Let's get started..
      wait 5s
      msend %actor% Some time passes..
      wait 3s
      msend %actor% You feel you are getting the hang of things.
      wait 3s
      msend %actor% You feel your skill in %full_skill% improving dramatically!
      mskillset %actor% %full_skill%
      quest erase trainer_3165 %actor.name%
   else
      say I appreciate your volentary donation, but I'm afraid thats all it was.
      snicker %actor%
      say I don't except installments. It's got to be all up front.
   endif
else
   return 0
   wait 2s
   consider %actor%
   wait 2s
   ponder
   wait 2s
   say Something is different about you. What skill were you going to train again?
   quest erase trainer_3165 %actor.name%
endif
~
#3167
Calken greet~
0 g 100
~
wait 1s
if %actor.quest_stage[trainer_3165]% == 1
   if %actor.quest_variable[trainer_3165:lvl]%
      if %actor.quest_variable[trainer_3165:lvl]% == %actor.level%
         say Have you returned to be trained in %actor.quest_variable[trainer_3165:skill_name]%?
         halt
      endif
   endif
   say You've been out adventuring, have you? Perhaps you could use some training?
else
   tell %actor.name% Greetings adventurer. Are you looking for training?
endif
~
#3170
shaman trainer speech~
0 dn 100
training yes spell sphere chant scribe meditate vampiric~
*make sure they have a quest record for saving variables
if %actor.quest_stage[trainer_3170]% < 1
   quest start trainer_3170 %actor.name%
endif
*introductions: lists the skills available
if %speech% == training? || %speech% == training || %speech% == yes
   say Sure, I can teach you just about any necessary magical talent.
   say Just ask me about a skill, and I'll give you a quote: spell knowledge, sphere of air,
   say sphere of death, sphere of divination, sphere of earth, sphere of enchantment,
   say sphere of fire, sphere of generic, sphere of healing, sphere of protection,
   say sphere of summoning, sphere of water, quick chant, chant, scribe, meditate. 
   halt
else
   *defining variables for this script
   set name %actor.name%
   set cha %actor.real_cha%
   set int %actor.real_int%
   *
   *Identify the skill to be trained
   if %speech% == spell knowledge
      set word2 knowledge
      set skill %actor.skill[spell knowledge]%
   elseif %speech% /= sphere
      if %speech% /= air
         set word2 air
         set skill %actor.skill[sphere of air]%
      elseif %speech% /= death   
         set word2 death
         set skill %actor.skill[sphere of death]%
      elseif %speech% /= divination   
         set word2 divination
         set skill %actor.skill[sphere of divination]%
      elseif %speech% /= earth   
         set word2 earth
         set skill %actor.skill[sphere of earth]%
      elseif %speech% /= enchantment
         set word2 enchantment
         set skill %actor.skill[sphere of enchantment]%
      elseif %speech% /= fire
         set word2 fire
         set skill %actor.skill[sphere of fire]%
      elseif %speech% /= generic   
         set word2 generic
         set skill %actor.skill[sphere of generic]%
      elseif %speech% /= healing
         set word2 healing
         set skill %actor.skill[sphere of healing]%
      elseif %speech% /= protection
         set word2 protection
         set skill %actor.skill[sphere of protection]%
      elseif %speech% /= summoning
         set word2 summoning
         set skill %actor.skill[sphere of summoning]%
      elseif %speech% /= water
         set word2 water
         set skill %actor.skill[sphere of water]%
      else
         say I've never heard of that sphere of magic before. 
      endif
   elseif %speech% == quick chant
      set word2 chant
      set skill %actor.skill[quick chant]%
   elseif %speech% == chant     
      set skill %actor.skill[chant]%
   elseif %speech% == scribe
      set skill %actor.skill[scribe]%
   elseif %speech% == meditate            
      set skill %actor.skill[meditate]%
   elseif %speech% == vampiric touch
      set word2 touch
      set skill %actor.skill[vampiric touch]%
   else
      say I'm not familiar with %speech%.
      halt
   endif
   *
   * Check to see if a skill should be taught at all
   *
   eval skill 10 * %skill%
   eval maxskill %actor.level% * 10 + 60
   * This portion is for smooth speech indicating the effects of a players cha or int score.
   if %cha% > 70
      say You're a charming person.
      if %int% > 70
         say ..and you are very quick witted.
         say I'll be delighted to give you a good rate.
      elseif %int% < 50
         say ..but you aren't the smartest.
      endif
   elseif %cha% < 50
      say I don't like you much.
      if %int% > 70
         say ..but you are pretty bright.
      elseif %int% < 50
         say ..and you aren't the smartest.
         say So this is going to cost you.
      else
      endif
   else
      say I wouldn't mind working with you.
      if %int% > 70
         say ..and you're pretty bright.
      elseif %int% < 50
         say ..but you aren't the smartest.
         say So, this is going to cost a bit more than usual.
      endif
   endif
   *Now a price is calculated
   eval change %maxskill% - %skill%
   eval opinion 200 - (%cha% - %int%)
   eval price %change% * %opinion% * %actor.level% * %actor.level% / (%skill% * 20)
   *
   *now the price in copper has to be divided into coinage.
   eval plat %price% / 1000
   eval gold %price% / 100 - %plat% * 10
   eval silv %price% / 10 - %plat% * 100 - %gold% * 10
   eval copp %price%  - %plat% * 1000 - %gold% * 100 - %silv% * 10
   *now the price can be reported 
   say I'll teach you %speech% for %plat% platinum, %gold% gold, %silv% silver, %copp% copper.
   say Just get me the money, and I'll get started.
   quest variable trainer_3170 %actor.name% skill_name %speech%
   if %word2%
      quest variable trainer_3170 %actor.name% word2 %word2%
   endif
   quest variable trainer_3170 %actor.name% skill_level %skill%
   quest variable trainer_3170 %actor.name% price %price%
   quest variable trainer_3170 %actor.name% actor_level %actor.level%
endif
~
#3171
shaman pay for training~
0 m 100
~
if %actor.quest_variable[trainer_3170:word2]%
   if %actor.quest_variable[trainer_3170:skill_name]% /= sphere
      set full_skill %actor.quest_variable[trainer_3170:skill_name]% of %actor.quest_variable[trainer_3170:word2]%
   else
      set full_skill %actor.quest_variable[trainer_3170:skill_name]% %actor.quest_variable[trainer_3170:word2]%
   endif
elseif %actor.quest_variable[trainer_3170:skill_name]%
   set full_skill %actor.quest_variable[trainer_3170:skill_name]%
endif
wait 1s
if %actor.quest_variable[trainer_3170:actor_level]% == %actor.level%
   if %value% >= %actor.quest_variable[trainer_3170:price]%
      grin %actor.name%
      wait 1s
      say Fantastic. Let's get started..
      wait 5s
      msend %actor% Some time passes..
      wait 3s
      msend %actor% You feel you are getting the hang of things.
      wait 3s
      msend %actor% You feel your skill in %full_skill% improving dramatically!
      mskillset %actor% %full_skill%
      quest erase trainer_3170 %actor.name%
   else
      say I appreciate your voluntary donation, but I'm afraid thats all it was.
      snicker %actor%
      say I don't except installments. It's got to be all up front.
   endif
else
   return 0
   wait 2s
   consider %actor%
   wait 2s
   ponder
   wait 2s
   say Something is different about you. What skill were you going to train again?
   quest erase trainer_3170 %actor.name%
endif
~
#3172
shaman trainer greet~
0 g 100
~
wait 1s
if %actor.quest_stage[trainer_3170]% == 1
   if %actor.quest_variable[trainer_3170:lvl]%
      if %actor.quest_variable[trainer_3170:lvl]% == %actor.level%
         say Have you returned to be trained in %actor.quest_variable[trainer_3170:skill_name]%?
         halt
      endif
   endif
   say You've been gaining power, have you? Perhaps you could use some training?
else
   tell %actor.name% Greetings adventurer. Are you looking for training?
endif
~
#3296
Make_Fountain_heal~
1 c 100
drink~
if (%arg% == pool || %arg% == granite)
oheal %actor% 400
return 0
endif
~
#3298
Black_Rose_Wear_Tear~
1 j 100
~
**Trigger is meant as a tear drop in memory of Pergus**
osend %actor% A small tear drops from the corner of your eye as you garnish the rose.
oechoaround %actor% %actor.name% sheds a tear as %actor.n% adjusts the rose.
~
#3299
Black_Rose_Spawn_Pluck~
2 c 100
Pluck~
wsend %actor% You bend over and carefully pluck the &9&bblack&0 rose from between the two hearts.
**some command to directly load 3298 to player inventory**
wechoaround %actor% %actor.name% bends over and carefully plucks the &9&bblack&0 rose from between the two hearts.
wait 1
wecho Another identical &9&bblack&0 rose magically grows up in the same spot.
~
#3301
newbie trainer Rousel~
0 g 100
~
wait 5
sa If you would like to attend Newbie training, just say newbie.
sa but this isn't quite ready yet, so come back later.
~
$~
