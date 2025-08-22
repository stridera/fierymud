#30201
Druid spy moves once~
0 o 100
~
* Teleports the druid spy to a random location, once. Because I want it to be
* in different places, but it can't be a wanderer, because that would make it
* lose its hidden status.
* Applied to: m30214
eval destination 30213 + %random.35%
mteleport %self% %destination%
~
#30202
Sliding into the abyss~
2 bi 50
~
* Makes players slide off the edge of a precipice, where they'll fall in a
* ravine and get hurt.
* Applied to: r30241, r30242
set victim %random.char%
if %victim% == 0 || %victim.vnum% != -1
   halt
end
set startroom %victim.room%
* Don't bother flying or levitating people.
if %victim.aff_flagged[FLY]% || %victim.aff_flagged[FEATHER_FALL]%
   halt
end
* Consider rangers and druids as too sure-footed to lose their footing.
if %victim.class% == Ranger || %victim.class% == Druid
   wechoaround %victim% A rock shifts under %victim.name%'s foot, but %victim.n% quickly
steps to one side.
   wsend %victim% A rock shifts under your foot, and you step to steadier ground.
   halt
end
wait 4
if %victim.room% != %startroom%
   halt
end
wechoaround %victim% %victim.name% slips on a loose rock, and begins to slide downward!
wsend %victim% Your foot slips on a loose rock.  You are sliding downward!
wait 3 s
if %victim.room% != %startroom%
   halt
end
wechoaround %victim% %victim.name% is sliding straight toward a sheer dropoff!
wsend %victim% You are sliding right toward a deep ravine!
wait 2 s
if %victim.room% != %startroom%
   halt
end
wechoaround %victim% %victim.name% disappears off the edge with a yelp!
wsend %victim% You slide off the edge into thin air!
wteleport %victim% 30255
wforce %victim% look
~
#30203
Sliding into the abyss II~
2 bi 50
~
* Makes players slide off the edge of a precipice, where they'll fall into
* a ravine and get hurt.
* Applied to: r30229, r203217
set victim %random.char%
if %victim% == 0 || %victim.vnum% != -1
   halt
end
set startroom %victim.room%
* Don't bother flying or levitating people.
if %victim.aff_flagged[FLY]% || %victim.aff_flagged[FEATHER_FALL]%
   halt
end
* Consider rangers and druids as too sure-footed to lose their footing.
if %victim.class% == Ranger || %victim.class% == Druid
   wechoaround %victim% A rock shifts under %victim.name%'s foot, but %victim.n% quickly steps to one side.
   wsend %victim% A rock shifts under your foot, and you step to steadier ground.
   halt
end
wait 4
if %victim.room% != %startroom%
   halt
end
wechoaround %victim% %victim.name% slips on a loose rock, and begins to slide downward!
wsend %victim% Your foot slips on a loose rock.  You are sliding downward!
wait 3 s
if %victim.room% != %startroom%
   halt
end
wechoaround %victim% %victim.name% is sliding straight toward a sheer dropoff!
wsend %victim% You are sliding right toward a deep ravine!
wait 2 s
if %victim.room% != %startroom%
   halt
end
wechoaround %victim% %victim.name% disappears off the edge with a yelp!
wsend %victim% You slide off the edge into thin air!
wteleport %victim% 30251
wforce %victim% look
~
#30204
A mild avalanche~
2 b 50
~
* Starts an avalanche. One player in the room is targeted, and will be
* smacked by a rock and moved to a downhill room unless they escape.
* Applied to: r30216, r30220, r30230, r30234, r30239, r30244
set victim %random.char%
if %victim% == 0 || %victim.vnum% != -1
   halt
end
set startroom %victim.room%
wecho A quiet noise from above attracts your attention.
wait 2 s
wecho A few small pebbles and stones are falling down the hillside.
wecho They clatter as they tumble.
wait 2 s
wecho Large rocks are tumbling past you!  They look heavy!
wait 1 s
if %self.vnum% == 30234
   eval destroom %self.vnum% + 1
else
   eval destroom %self.vnum% - 1
end
eval damage 80 + %random.50%
if %victim.level% < 10
   eval damage 3 + %random.5%
elseif %victim.level% < 20
   eval damage 10 + %random.8%
elseif %victim.level% < 40
   eval damage 30 + %random.30%
end
if %victim.room% == %startroom%
   if %victim.class% == Ranger
      wechoaround %victim% %victim.name% is nearly smacked by a large rock, but %victim.n% steps aside at the last moment.
      wsend %victim% A big rock comes hurtling toward you, but you step smoothly aside.
   else
      wdamage %victim% %damage% crush
      if %damdone% == 0
         wechoaround %victim% The rocks seem to pass right through %victim.name%.
         wsend %victim% The rocks seem to pass right through you.
      else
         wechoaround %victim% A big rock hits %victim.name% right in the chest, and %victim.n% is knocked over! (&4%damdone%&0)
         wechoaround %victim% %victim.n% tumbles downhill!
         wsend %victim% You try to dodge the boulders, but a large stone whacks you in the chest! (&1%damdone%&0)
         wsend %victim% You are knocked down!
         wteleport %victim% %destroom%
         wechoaround %victim% %victim.name% tumbles down the trail from above, and comes to a rest.
         wait 1 s
         wforce %victim% look
      end
   end
end
wecho The remnants of the avalanche tumble past, and then silence returns to the mountains.
~
#30205
exploding bag~
1 g 100
~
* Makes the red leather bag explode when taken, hurting just the person who took it.
* Applied to: o30209
return 0
oechoaround %actor% %actor.alias% reaches for %self.shortdesc%, but it suddenly &1&bexplodes!&0
osend %actor% As you reach for %self.shortdesc%, it suddenly &1&bexplodes!&0
eval damage %actor.level% * 3 + %random.19%
odamage %actor% %damage% slash
if %damdone% == 0
   oechoaround %actor% Shards of metal fly right by %actor.name%!
   osend %actor% Shards of metal fly by!  Luckily, none of them hit you!
else
   oechoaround %actor% Shards of metal hit %actor.alias% in the legs, causing serious wounds! (&1%damdone%&0)
   osend %actor% &1OUCH!&0  The shards cut your legs painfully! (&1%damdone%&0)
end
opurge %self%
~
#30206
Try to drag exploding bag: bang~
1 c 100
drag~
switch %cmd%
  case d
    return 0
    halt
done
return 1
oechoaround %actor% %actor.alias% reaches for %self.shortdesc%, but it suddenly &1&bexplodes!&0
osend %actor% As you reach for %self.shortdesc%, it suddenly &1&bexplodes!&0
eval damage %actor.level% * 3 + %random.19%
odamage %actor% %damage% slash
if %damdone% == 0
   oechoaround %actor% Shards of metal fly right by %actor.name%!
   osend %actor% Shards of metal fly by!  Luckily, none of them hit you!
else
   oechoaround %actor% Shards of metal hit %actor.alias% in the legs, causing serious wounds! (&1%damdone%&0)
   osend %actor% OUCH!  The shards cut your legs painfully! (&1%damdone%&0)
end
opurge %self%
~
#30207
**UNUSED**~
1 c 100
d~
* Allows normal use of 'd' command around red leather bag (it intercepts
* the "drag" command in trigger 30206).
* Applied to: o30209
return 0
~
#30208
Try to get rid of red bag, splode~
1 hi 100
~
* Makes the red leather bag explode when you try to drop or give it.
* Applied to: o30209
return 0
oechoaround %actor% %actor.alias% tries to let go of %self.shortdesc%, but it suddenly &1&bexplodes!&0
osend %actor% As you release %self.shortdesc%, it suddenly &1&bexplodes!&0
eval damage %actor.level% * 3 + %random.19%
odamage %actor% %damage% slash
if %damdone% == 0
   oechoaround %actor% Shards of metal fly right by %actor.name%!
   osend %actor% Shards of metal fly by!  Luckily, none of them hit you!
else
   oechoaround %actor% Shards of metal hit %actor.alias% in the legs, causing serious wounds! (&1%damdone%&0)
   osend %actor% OUCH!  The shards cut your legs painfully! (&1%damdone%&0)
end
opurge %self%
~
#30209
new trigger~
0 g 100
~
Nothing.
~
#30212
banish_murgbol_greet~
0 g 100
~
set stage %actor.quest_stage[banish]%
wait 4
if %actor.class% /= priest || %actor.class% /= diabolist
  if %actor.quest_variable[banish:greet]% == 0
    if %stage% == 0
      say Wanna learn some spells kid?
      mecho   
      mecho %self.name% says, 'I can teach you to hurl your enemies far into the outer planes
      mecho &0so they can never bother you again.'
    else
      if %stage% == 2
        mecho %self.name% says, 'I can already tell you have made progress along your journey.
        mecho &0Let's keep going!'
        wait 1s
        emote closes her eyes and prays for further guidance.
        wait 3s
        say Yes, I see...
        emote nods.  'Quite challenging...'
        wait 1s
        emote opens her eyes.
        wait 2s
        say The gods wish to test your sway over the elemental forces.
        wait 1s
        mecho %self.name% says, 'Ice elementals wander deep in the frozen tunnels near the nest
        mecho &0of a mighty dragon.  Destroy the &6&bmost powerful of these elementals&0 to receive
        mecho &0your next vision, then return to me.'
      elseif %stage% == 3
        mecho %self.name% says, 'It seems you've managed to destroy the ice elemental lord.
        mecho &0Let's press on!'
        wait 1s
        emote closes her eyes and prays for further guidance.
        wait 3s
        say Yes, most appropriate.
        wait 1s
        emote opens her eyes.
        wait 2s
        say The gods demand a sacrifice of life energy.
        wait 1s
        mecho %self.name% says, 'A powerful &5&btroll wizard&0 warps the tunnels of an ancient mine.
        mecho &0His blood will most please the gods.  After this is done, return to me.'
      elseif %stage% == 4
        mecho %self.name% says, 'The gods are greatly pleased with your sacrifice.
        mecho &0Let's continue!'
        wait 1s
        emote closes her eyes and prays for further guidance.
        wait 3s
        say Naturally, such a thing must be done.
        wait 1s
        emote opens her eyes.
        wait 2s
        mecho %self.name% says, 'Since you've dispatched something living, it's time to banish
        mecho &0something lingering after death.'
        wait 1s
        mecho %self.name% says, 'In a room of art in an ancient burial site waits a &5long-dead&0
        mecho &0&5apparition&0.  Its threads to this world are tenuous at best.  Send it to the
        mecho &0next world to receive the next part of the prayer.  Afterward, come back and
        mecho &0we shall continue.'
      elseif %stage% == 5
        mecho %self.name% says, 'Your awareness continues to grow.
        mecho &0Let's keep moving!'
        wait 1s
        emote closes her eyes and prays for further guidance.
        wait 3s
        say Certainly, as you wish...
        wait 1s
        emote opens her eyes.
        wait 2s
        mecho %self.name% says, 'The gods of light will have you undertake the next step and
        mecho &0prove mastery over demonic energy.'
        wait 1s
        mecho %self.name% says, 'Far to the north in Frost Valley, there is a &9&bdemon corrupting&0
        mecho &0&9&bthe world around it&0 through its very presence.  Exorcise it from Ethilien and
        mecho &0receive the reward of the gods, then return here.'
      elseif %stage% == 6
        mecho %self.name% says, 'The gods of light are satisfied.
        mecho Let's see the final test!'
        wait 1s
        emote closes her eyes and prays for further guidance.
        wait 3s
        grin
        say Delightful choice.
        wait 1s
        emote opens her eyes.
        wait 2s
        mecho %self.name% says, 'Destroying celestial energy for the dark gods shall be your
        mecho &0final task.'
        wait 1s
        mecho %self.name% says, 'Many celestial beings have made their home in a floating
        mecho &0crystal fortress on South Caelia.  One in particular, a &7&bsix-winged seraph&0,
        mecho &0stands sentinel over the uppermost floors.  Banish it from Ethilien and the
        mecho &0gods will reward you with the final part of the prayer.  Return to me after,
        mecho &0and I shall help you perform it.
      elseif %stage% == 7
        say I see you have been successful!
        wait 1s
        say Now, speak aloud the mystic word your visions revealed!
      endif
      quest variable banish %actor% greet 1
    endif
  endif
endif
~
#30213
banish_murgbol_speech1~
0 d 100
yes yes? sure yep okay yeah cool good~
wait 2
set stage %actor.quest_stage[banish]%
if %actor.class% /= priest || %actor.class% /= diabolist 
  if %stage% == 0
    if %actor.level% > 64
      say Good, good...
      quest start banish %actor.name%
      wait 1s
      say I shall see how the gods best wish me to teach you.
      mecho She closes her eyes and prays to the gods above and below.
      wait 7s
      say The gods have decreed you must go on a vision quest.
      wait 2s
      mecho %self.name% says, 'They demand you demonstrate power over the six animating forces.
      mecho &0After each demonstration, they'll provide you with a different part of the
      mecho &0spell to banish your foes.  &6&bRecord those visions&0 - they're critical to
      mecho &0unlocking the spell.'
      wait 7s
      mecho %self.name% says, 'Your first vision will be granted when you prove mastery over
      mecho &0magical life.  Find the &4mighty witch of the sea&0 in her lair beneath the waves
      mecho &0and defeat her.  The gods will give you part of the prayer.'
      wait 6s
      say Return to me after and I will guide you to your next act.
      wait 2s
      mecho %self.name% says, 'And if you need a reminder, check in with me on your &7&b[progress]&0
      mecho &0any time you feel like it.'
      wait 2s
      say Now go!
      quest variable banish %actor% greet 1
    else
      say Give it time kid.  You aren't ready to wield such power.
    endif
  endif
endif
~
#30214
uaine_witch_death_banish~
0 f 100
~
set person %actor%
set i %actor.group_size%
if %i%
   unset person
   while %i% > 0
      set person %actor.group_member[%i%]%
      if %person.room% == %self.room%
         if %person.quest_stage[banish]% == 1
            quest advance banish %person.name%
            msend %person% &5&bA single letter pops into your mind - &6&bV&0
         endif
      endif
      eval i %i% - 1
   done
elseif %person.quest_stage[banish]% == 1
   quest advance banish %person.name%
   msend %person% &5&bA single letter pops into your mind - &6&bV&0
endif
~
#30215
ice_lord_death_banish~
0 f 100
~
set person %actor%
set i %actor.group_size%
if %i%
   unset person
   while %i% > 0
      set person %actor.group_member[%i%]%
      if %person.room% == %self.room%
         if %person.quest_stage[banish]% == 2
            quest advance banish %person.name%
            msend %person% &5&bA single letter pops into your mind - &6&bI&0
         endif
      endif
      eval i %i% - 1
   done
elseif %person.quest_stage[banish]% == 2
   quest advance banish %person.name%
   msend %person% &5&bA single letter pops into your mind - &6&bI&0
endif
~
#30216
banish_eidolon_death~
0 f 100
~
set person %actor%
set i %actor.group_size%
if %i%
   unset person
   while %i% > 0
      set person %actor.group_member[%i%]%
      if %person.room% == %self.room%
         if %person.quest_stage[banish]% == 4
            quest advance banish %person.name%
            msend %person% &5&bA single letter pops into your mind - &6&bU&0
         endif
      endif
      eval i %i% - 1
   done
elseif %person.quest_stage[banish]% == 4
   quest advance banish %person.name%
   msend %person% &5&bA single letter pops into your mind - &6&bU&0
endif
~
#30217
banish_seraph_death~
0 f 100
~
set person %actor%
set i %actor.group_size%
if %i%
   unset person
   while %i% > 0
      set person %actor.group_member[%i%]%
      if %person.room% == %self.room%
         if %person.quest_stage[banish]% == 6
            quest advance banish %person.name%
            msend %person% &5&bA single letter pops into your mind - &6&bP&0
         endif
      endif
      eval i %i% - 1
   done
elseif %person.quest_stage[banish]% == 6
   quest advance banish %person.name%
   msend %person% &5&bA single letter pops into your mind - &6&bP&0
endif
~
#30218
banish_murgbol_speech2~
0 d 100
vibugp~
wait 2
if %actor.quest_stage[banish]% == 7
  msend %actor% %self.name% chants along with you.
  mechoaround %actor% %self.name% chants along with %actor.name%.
  mecho %self.name% starts casting, &3&b'Vibugp'&0...
  wait 2s
  msend %actor% There is a bright flash as you feel the power of the spell well up in the words, &1&b'I banish thee!'&0
  mskillset %actor.name% banish
  wait 1s
  msend %actor% &1&bYou have mastered the prayer for Banish!&0
  quest complete banish %actor.name%
  if !%actor.quest_variable[hell_trident:helltask5]% && %actor.quest_stage[hell_trident]% == 1
    quest variable hell_trident %actor% helltask5 1
  endif
  mechoaround %actor% There is a bright flash as the spell goes off!
  mechoaround %actor% %actor.name% seems to be more connected to the universe!
endif
~
#30219
banish_murgbol_status_checker~
0 d 100
status status? progress progress?~
wait 2
set stage %actor.quest_stage[banish]%
if %actor.has_completed[banish]%
  say I have tought you all I can.
  halt
elseif %stage% == 0
  say I'm not teaching you right now...
elseif %stage%
  switch %stage%
    case 1
      set mob 41119
      set place her chamber under the ocean waves
      set known Nothing.
      break
    case 2
      set mob 53313
      set place the frozen tunnels of the north
      set known v
      break
    case 3
      set mob 37000
      set place a deep and ancient mine
      set known vi
      break
    case 4
      set mob 48005
      set place a room filled with art in an ancient barrow
      set known vib
      break
    case 5
      set mob 53417
      set place the cold valley of the far north
      set known vibu
      break
    case 6
      set mob 23811
      set place a nearby fortress of clouds and crystals
      set known vibug
      break
    case 7
      mecho %self.name% says, 'Come, speak the prayer aloud: &5&bvibugp&0!'
      halt
      break
    default
      return 0
  done
  mecho %self.name% says, 'To learn Banish you must next:'
  mecho - kill %get.mob_shortdesc[%mob%]% in %place%.
  mecho   
  mecho &0Your knowledge of the prayer so far: "&6&b%known%&0"
endif
~
#30220
mesmeriz_death_banish~
0 f 100
~
set person %actor%
set i %actor.group_size%
if %i%
   unset person
   while %i% > 0
      set person %actor.group_member[%i%]%
      if %person.room% == %self.room%
         if %person.quest_stage[banish]% == 3
            quest advance banish %person.name%
            msend %person% &5&bA single letter pops into your mind - &6&bB&0
         endif
      endif
      eval i %i% - 1
   done
elseif %person.quest_stage[banish]% == 3
    quest advance banish %person.name%
    msend %person% &5&bA single letter pops into your mind - &6&bB&0
endif
~
#30221
Banish death trigger~
0 f 100
~
switch %self.vnum%
  case 41119
    * Sea Witch
    set stage 1
    set letter V
    break
  case 53313
    * Ice Lord
    set stage 2
    set letter I
    break
  case 37000
    * Mesmeriz
    set stage 3
    set letter B
    break
  case 48005
    * Eidolon
    set stage 4
    set letter U
    break
  case 53417
    * Chaos Demon
    set stage 5
    set letter G
    break
  case 23811
    * lesser seraph
    set stage 6
    set letter P
done
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
    if %person.quest_stage[banish]% == %stage%
      quest advance banish %person.name%
      quest variable banish %person% greet 0
      msend %person% &5&bA single letter pops into your mind - &6&b%letter%&0
    endif
  elseif %person%
    eval i %i% + 1
  endif
  eval a %a% + 1
done
~
$~
