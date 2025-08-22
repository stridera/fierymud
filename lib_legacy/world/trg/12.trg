#1201
Long live the king~
0 d 1
chinok~
say Long live his fame and long live his glory.
emote bows their head briefly.
~
#1203
nexus_clock_pin_reload~
1 c 100
Rock well demon~
oload obj 1203
oecho The &9&bNexus&0 &1Cloak&0 &9&bPin&0 beings to &4&bgl&6&bow&0 mysteriously then fades back to normal.
opurge %self%
~
#1210
Zzur's test trigger~
1 dp 100
~
cas 'fireball'
~
#1212
calian_slayer~
1 gj 100
~
if %actor.quest_variable[quest_items:%self.vnum%]%
   return 1
   oechoaround %actor% A &9&bblack &7haze &0surrounds %actor.name%, obscuring %actor.o% from view.
   osend %actor% A &9&bblack &7haze &0surrounds you, momentarily obscuring your view.
else
   return 0
   osend %actor% You do not feel worthy enough to wield %self.shortdesc%!
endif
~
#1222
tanles_testing_trigger~
0 g 100
~
say My trigger commandlist is not complete!
~
#1233
test_trig~
0 d 100
hello~
say %actor.name% is a %actor.class%.
if %actor.quest_stage[moonwell_spell_quest]% > 0
say %actor.name% is on the moonwell quest.
else
say %actor.name% is not on the moonwell quest.
endif
~
#1240
Noisy Weapon~
1 d 20
~
set rndm %random.10%
if %damage% == 0
switch %rndm%
  case 1
  oecho %self.shortdesc% says, "Wow, totally missed that one."
    break
  case 2
  oecho %self.shortdesc% says, "Where were you swinging?"
    break
  case 3
  oecho %self.shortdesc% says, "Are you even trying?"
    break
  case 4
  oecho %self.shortdesc% says, "No. Not like that.  You're supposed to HIT them!"
    break
  case 5
  oecho %self.shortdesc% says, "Why couldn't that other warrior have picked me up."
    break
  case 6
  oecho %self.shortdesc% says, "Welp, I guess I will stay cleaner if you never hit."
    break
  case 7
  oecho %self.shortdesc% says, "Missed again, big surprise."
    break
  case 8
  oecho %self.shortdesc% says, "Oh, so close."
    break
  case 9
  oecho %self.shortdesc% says, "Not even close that time."
    break
  case 10
  oecho %self.shortdesc% says, "Well, at least you tried."
    break
  default
oecho %self.shortdesc% says, "Wut?"
    break
done
else
switch %rndm%
  case 1
  oecho %self.shortdesc% says, "Ohh, the blood!  I'll need a bath after this."
    break
  case 2
  oecho %self.shortdesc% says, "Wow, look at that hit!"
    break
  case 3
  oecho %self.shortdesc% says, "You're so strong!  Amazing."
    break
  case 4
  oecho %self.shortdesc% says, "Oh yea, that's how you do it!"
    break
  case 5
  oecho %self.shortdesc% says, "Booyah!"
    break
  case 6
  oecho %self.shortdesc% says, "Ohh, the blood!  I'll need a bath after this."
    break
  case 7
  oecho %self.shortdesc% says, "He won't be coming back from that hit."
    break
  case 8
  oecho %self.shortdesc% says, "Yeah.  Much hit.  Very strong.  Wow!"
    break
  case 9
  oecho %self.shortdesc% says, "Just like that!"
    break
  case 10
  oecho %self.shortdesc% says, "Look at your glistening muscles!"
    break
  default
oecho %self.shortdesc% says, "How did you do that?"
    break
done
end
~
#1246
valentine_box_open~
1 c 2
open box~
oecho %arg%
~
#1260
summon_dragon~
1 c 1
summon~
if %has_one% != yes
   if %actor.class% == Paladin
      oload mob 1260
      oechoaround %actor% A brilliant golden dragon flies in, seemingly from nowhere, and nuzzles %actor.name%'s side.
      osend %actor% You begin calling for a mount..
      osend %actor% A brilliant golden dragon answers your summons.
      oforce golden-dragon follow %actor.name%
      oforce golden-dragon mload obj 1262
      oforce golden-dragon wear dragonsaddle
   elseif %actor.class% /= Anti
      oload mob 1263
      oechoaround %actor% A dusky black dragon flies in, seemingly from nowhere, and sits by %actor.name%'s side.
      osend %actor% You begin calling for a mount..
      osend %actor% A dusky black dragon answers your summons.
      oforce black-dragon follow %actor.name%
      oforce black-dragon mload obj 1262
      oforce black-dragon wear dragonsaddle
   else
      osend %actor% You begin calling for a mount...but nothing happens.
      oechoaround %actor% %actor.name% whistles loudly.
   end
   set has_one yes
   global has_one
else
end
~
#1261
sulk_and_south~
1 c 1
sulk~
return 0
~
#1265
lylaith~
1 bcj 15
lylaith~
oecho The blade of %self.shortdesc% flares a &7&bbright white&0.
~
#1269
blue_recall_test~
1 c 2
recite~
osend %actor% In a flash of blinding light, you find yourself wisked away.
oechoaround %actor% In a flash of blinding light, %actor.name% disappears!
oteleport %actor.name% 10001
oforce %actor% look
~
#1270
8ball message generator~
1 c 1
shake~
if %arg% == 8ball
    return 1
oecho %actor.name% shakes a magic 8ball.
     set rndm %random.20%
     switch %rndm%
          case 1
          oecho %self.shortdesc% shows, "As I see it, yes."
          break
          case 2
          oecho %self.shortdesc% shows, "Ask again later."
          break
          case 3
          oecho %self.shortdesc% shows, "Better not tell you now."
          break
          case 4
          oecho %self.shortdesc% shows, "Cannot predict now."
          break
          case 5
          oecho %self.shortdesc% shows, "Concentrate and ask again."
          break
          case 6
          oecho %self.shortdesc% shows, "Dont count on it."
          break
          case 7
          oecho %self.shortdesc% shows, "It is certain."
          break
          case 8
          oecho %self.shortdesc% shows, "It is decidedly so."
          break
          case 9
          oecho %self.shortdesc% shows, "Most likely."
          break
          case 10
          oecho %self.shortdesc% shows, "My reply is no."
          break
          case 11
          oecho %self.shortdesc% shows, "My sources say no."
          break
          case 12
          oecho %self.shortdesc% shows, "Outlook not so good."
          break
          case 13
          oecho %self.shortdesc% shows, "Outlook good."
          break
          case 14
          oecho %self.shortdesc% shows, "Reply hazy, try again."
          break
          case 15
          oecho %self.shortdesc% shows, "Signs point to yes."
          break
          case 16
          oecho %self.shortdesc% shows, "Very doubtful."
          break
          case 17
          oecho %self.shortdesc% shows, "Without a doubt."
          break
          case 18
          oecho %self.shortdesc% shows, "Yes."
          break
          case 19
          oecho %self.shortdesc% shows, "Yes  definitely."
          break
          case 20
          oecho %self.shortdesc% shows, "You may rely on it."
          break
     done
else
     return 0
end
~
#1275
red_blood_cell_pillow~
1 ai 100
~
if (%actor.name% == Laoris) && (%victim.level% <= 99)
   return 1
   wait 1
   oforce %victim% wear red-blood-cell-pillow
endif
~
#1281
you suck~
0 c 100
look fighter~
say YOU SUCK!
~
#1285
laoris test~
2 ad 0
angel~
wecho AFF: %actor.aff_flags%
wecho FLAGS: %actor.flags%
wecho DET-MAGIC?: %actor.aff_flagged[DET-MAGIC]%
~
#1298
acerites_testing_trigger~
1 c 3
fire~
Nothing.
~
#1299
Acerite_belt_trigger~
1 l 100
~
if %actor.can_be_seen% && %actor.level% < 100
  return 0
  osend %actor% The broad silver belt flares white, burning you as you attempt to remove it!
  oechoaround %actor% A broad silver belt flares and lets off smoke as %actor.name% reaches for its buckle.
  odamage %actor% 100 fire
endif
~
$~
