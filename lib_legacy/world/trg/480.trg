#48000
infidel_dead~
0 f 100
~
mload obj 48031
mload obj 48037
~
#48001
timer_on_recall~
1 g 100
~
oechoaround %actor% As %actor.name% grabs the stone you feel a lurch in your stomach.
osend %actor% You feel the stone burn against your hand for a second and it is gone!
oteleport all 48080
oforce all look
return 0
~
#48002
lich_ring_compo_change~
1 j 100
~
osend %actor% As you slide %self.shortdesc% on your finger, your flesh starts to die and slump off.
osend %actor% You transform into a skeleton!
~
#48005
young_witch_greet1~
0 g 75
~
if %actor.vnum% == -1
   if %actor.level% < 60
      msend %actor.name% The young witch says, 'I may still be learning, but I know enough to destroy you.'
   else
      msend %actor.name% The young witch says, 'At last a chance to test myself on a worthy opponent.'
   end
end
~
#48006
young_witch_greet2~
0 g 100
~
if %actor.vnum% == -1
   msend %actor.name% The young witch drops her book and hisses at you.
endif
~
#48007
kingspriest_fight1_long~
0 f 100
~
if %actor.vnum% == -1
   shout You will never truely defeat me!
   mecho &3The Undead KingPriest throws up his head and&0 &3&bhis eyes&0 &1&bglow&0 &7&bwhite&0 &3with rage!&0
   mgoto 48084
   mjunk hammer
   mload mob 48027
   mforce wraith mload obj 48002
   mforce wraith mload obj 48036
   mforce wraith wear all
   mload mob 48028
   mload mob 48028
   mload mob 48028
   mteleport wraith 48045
   mteleport shadow-guardian 48045
   mteleport shadow-guardian 48045
   mteleport shadow-guardian 48045
   mgoto 48045
   mecho &3The Undead KingPriest Cracks in&0 &1&btwo,&0 &7&bsplit open by a white light.&0
   mecho &3From the shell emerges the&0 &6&bWraith of the KingPriest!&0
   mforce wraith shout Come to me my guardians!  We have souls to claim!
   mecho &2&bThe stone of the floor and wall erupt into statues of shadowy guardians!&0
   mforce wraith kill %actor.name%
   mforce shadow kill %actor.name%
   mforce 2.shadow kill %actor.name%
   mforce 3.shadow kill %actor.name%
   mgoto 48084
else
endif
~
#48008
kingpriest_allgreet1~
0 h 100
~
if %actor.align% > 350
say I think my god must have thought I had forgotten him.
grin %actor.name%
say If you can be my message.
endif
~
#48009
prince_receive1~
0 j 100
~
if %actor.vnum% == -1
if %object.vnum% == 48031
if %actor.align% > 350
say Ah, even though I did not strike the killing blow, I can feel the years of desire for revenge have been completed.
say Thank you %actor.name%, I hope this gift will help you against the king.
else
say Hmm, although your motives are not pure, you have done me a great service.
wait 1
say In return I will give you this staff, may you find it a help as you continue in your quest.
endif
mload obj 48019
give staff %actor.name%
mjunk head_of_the_infidel
endif
endif
~
#48010
prince_speak1~
0 d 1
trade?~
if %actor.align% > 350 && %actor.level% > 80
  wait 2
  say If you bring me the head of the king's dark champion, then I will give you an item that may help you in your battle with the king.
  wait 1
  say According to our rules of dueling, you must say 'the prince wishes to challenge you by proxy', before starting the fight.
  wait 1
  smile
  say That way, I can claim the victory as my own.
endif
~
#48011
prince_speak2~
0 d 1
destroy?~
if %actor.align% > 350
  if %actor.level% > 80
    wait 2
    say Yes.  He is still alive due to his magic arts, and so he can still be killed.
    wait 1
    say He is a coward, however, and I have heard that when the tides of battle turn against him, he is quick to disappear.
    say I may have something to help you with that if you are willing to... trade.
  else
    wait 2
    say Sadly yes, you have come too soon.  If you are able to return once you are stronger, perhaps you can be of great assistance then.
  endif
endif
~
#48012
prince_speak3~
0 d 1
failed?~
if %actor.align% > 350
  wait 2
  say Yes.  I was too confident and his dark infidel champion overcame me.
  grumble
  wait 1
  say As a punishment, the King brought me back to life, and now I am compelled to do his bidding.
  frown
  if %actor.level% >80
    say But you might be powerful enough to destroy him.
  else
    say It is a shame you will not be powerful enough to destroy him.
  endif
endif
~
#48013
prince_speak4~
0 d 1
help?~
if %actor.align% > 349
  wait 2
  sigh
  say Many years ago when King Ureal threatened the land, I swore that I would try to stop him.
  wait 1
  say I was the paladin champion, and it was my duty to bring an end to his evil.
  wait 1
  emote sighs again loudly.
  say But I failed.
endif
~
#48014
prince_greet1~
0 g 100
~
if %actor.align% > 349
  say Are you here to help me?
elseif %actor.align% < -349
  say Are you here to challenge me?
endif
~
#48015
witch_rand1~
0 b 50
~
ponder elder
~
#48016
lich_speak1~
0 d 100
ureal~
ponder
say You know my name, but do you know my power?
cast 'disintegrate' %actor.name%
~
#48017
lich_greet1~
0 gl 1
~
cackle
say So, %actor.name% do you know who you face?
~
#48018
lich_rand1~
0 b 30
~
emote watches you closely, his hunger for your lifeforce palpable.
~
#48019
elder_rand1~
0 b 60
~
glare barrow-witch
say Stop trying to peep at the recipe youngster.  If I die, then you can see it.
cackle
wait 1
say If...
glare
~
#48020
elder_rand2~
0 b 50
~
whap barrow-witch
say Don't mix the eye-of-newt with the toad blood yet idiot!
say Do you want to kill us all??
~
#48021
infidel_speak1~
0 d 0
the prince wishes to challenge you by proxy~
rofl
say I beat him once already.
if %actor.level% > 70
   say Hmm, you look like you might give me a reasonable battle.
   say Well...let me even the odds a a little.
else
   say HAH!  His honor depends on you?!
   say However, this should put the result beyond doubt.
endif
rem scimitar
wait 3
emote waves his hands in the air in a mystical gesture.
emote murmurs a spell and suddenly looks younger!
mat 48084 mload mob 48026
wait 1
say Prepare to meet whichever god you believe in!
mat 48084 give polished-scimitar infidel-warrior-youthful
mat 48084 mforce infidel-warrior-youthful wield scimitar
mteleport infidel-warrior-youthful 48038
mforce infidel-warrior-youthful kill %actor.name%
mgoto 48084
mjunk all
mpurge %self%
~
#48022
infidel_speak2~
0 d 100
challenge?~
say Yes, challenge.  I presume you have been sent by that young upstart.
spit
wait 1
say If you haven't, then you had better leave before my patience wears thin.
ponder
wait 1
say And if you have, then you'd better leave before I kill you.
pat %actor.name%
say Run along now little one.
~
#48023
infidel_allgreet1~
0 g 100
~
if %actor.align% > -350
  say So %actor.name%, do you dare to challenge me?
else
  say So %actor.name%, will you help me seek my revenge?
endif
~
#48024
infidel2_fight~
0 k 100
~
set fight %time.stamp%
global fight
if %random.10% < 4
   mecho The body of the infidel shifts slightly but remains renewed.
endif
~
#48025
Jerajai_Attack~
0 k 20
~
set val %random.10%
switch %val%
case 1
breath acid
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
#48026
Thelriki_Attack~
0 k 20
~
set val %random.10%
switch %val%
case 1
breath gas
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
#48027
infidel2_random~
0 ab 60
~
set now %time.stamp%
if %now% - 1 > %fight%
   emote looks around for someone to fight.
   sniff
   remove scimitar
   emote waves his hands in the air in a mystical manner.
   wait 1
   mecho The body of the infidel ages rapidly to return to the decayed state.
   mat 48084 mload mob 48025
   mat 48084 give polished-scimitar infidel-warrior-necrotic
   mat 48084 mforce infidel-warrior-necrotic wield scimitar
   mat 48084 mteleport infidel-warrior-necrotic 48038
   mforce infidel-warrior-necrotic groan
   mforce infidel-warrior-necrotic say I hate this form.
   mgoto 48084
   mjunk all
   mpurge %self%
endif
~
#48028
Infidel speech revenge~
0 d 100
revenge?~
if %actor.align% < -349
  wait 2
  grin
  say Many centuries ago, I was the champion of King Ureal, the mighty sorcerer who would conquer Ethilien.
  wait 1
  say Some pathetic paladin sought to slay our King, claiming it was his duty to bring an end to his reign.
  wait 1
  say He challenged me to a duel.
  chuckle
  wait 1
  say And lost.
endif
~
#48029
Infidel speech lost~
0 d 100
lost?~
if %actor.align% < -349
  wait 2
  laugh
  say Yes.  He was overconfident and reckless.  I easily snuffed him out.
  flex
  wait 1
  say As a punishment, the King brought him back to life, and toys with him deep in this barrow.
  wait 1
  frown
  say But he continues to be a thorn in my King's side...  The prince continues to try to destroy my King, even as he become Eternal in Death.
  wait 1
  if %actor.level% >80
    say But you might be powerful enough to destroy the prince for good.
  else
    say It is a shame you will not be powerful enough to destroy the prince forever.
  endif
endif
~
#48030
Infidel speech destroy?~
0 d 100
destroy?~
if %actor.align% < -349
  if %actor.level% > 80
    wait 2
    say If you bring me the prince's skull, I can reward you.
    wait 1
    say He is bound by certain rules.  If you say 'the champion wishes to challenge you by proxy', he will be forced to fight you.
    wait 1
    smile
    say That way, my victory will be absolute.
  else
    say If you are able to return once you are stronger, perhaps you can be of great assistance then.
    wait 1
    say Now leave.
  endif
endif
~
#48031
Prince speech challenge~
0 d 0
the champion wishes to challenge you by proxy~
mecho %self.name% shutters at the challenge.
set drop_head 1
global drop_head
if %actor.level% > 70
   say You look like a true threat.  But do not wonder, I have grown greatly since my death.
else
   say A shame he sends one still so inexperienced.  However, this should put the result beyond doubt.
endif
if %get.mob_count[48012]% < 2
  eval needed 2 - %get.mob_count[48012]%
  mecho %self.name% calls out the spirits of his Royal Guard!
  set loop 0
  while %loop% < %needed%
    mecho %self.name% summons %get.mob_shortdesc[48012]%!
    mload mob 48012
    eval loop %loop% + 1
  done
endif
if !%self.wearing[48003]%
  mecho %self.name% lifts %get.obj_shortdesc[48003]% from the chamber floor.
  mload obj 48003
  wie indigo-blade
endif
if !%self.wearing[48010]%
  mecho %self.name% lifts %get.obj_shortdesc[48010]% from the chamber floor.
  mload obj 48010
  wear breastplate
endif
say En garde!
wait 1s
kill %actor%
~
#48032
Prince death~
0 f 100
~
if %drop_head%
  mload obj 48025
endif
~
#48033
Infidel receive~
0 j 100
48025~
wait 2
mjunk %object%
mecho %self.name% grins as he crushes the Prince's skull to dust.
wait 1s
laugh
say Good, just as it should be.
wait 2s
mecho %self.name% dusts off his hands and produces a sickly gnarled staff from his belongings.
wait 1s
say Take this as a reward.
mload obj 48039
give infidels-staff %actor%
set person %actor%
set i %person.group_size%
if %i%
  while %i% > 0
    set person %actor.group_member[%i%]%
    if %person.room% == %self.room%
      if %person.quest_stage[hell_trident]% == 2
        if !%person.quest_variable[hell_trident:helltask6]%
          quest variable hell_trident %person% helltask6 1
        endif
      endif
      msend %person% &3&bYou have finished the infidel's duel!&0
    endif
    eval i %i% - 1
  done
else
  if %person.quest_stage[hell_trident]% == 2
    if !%person.quest_variable[hell_trident:helltask6]%
      quest variable hell_trident %person% helltask6 1
    endif
  endif
  msend %person% &3&bYou have finished the infidel's duel!&0
endif
~
#48099
Barrow guard~
0 c 100
down~
if %actor.vnum% == -1
    if %actor.level% < 60
        whisper %actor.name% This barrow is far too dangerous without more experience.
        whisper %actor.name% Very few who venture further have ever returned.
        wait 1
        whisper %actor.name% Come back when you're ready for imminent danger.
    else
        return 0
    endif
else
    return 0
endif
~
$~
