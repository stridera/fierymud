#52101
pirate-entity_greet~
0 g 25
~
wait 1
mecho The shade mourns out, 'Help us! Free us from her!'
wait 3s
mecho The shade yells out, 'She is enternal evil! Beware!'
~
#52102
pirate_captain_greet~
0 g 25
~
wait 1
shout Leave here, or you shall die!
say She will kill you!
~
#52103
rower_shout_random~
0 b 2
~
shout Help us!
~
#52104
pirate_drummer_rand~
0 b 2
~
masound Drumming can be heard nearby.
~
#52105
Angel_Room~
0 b 2
~
mecho A warmth feels the room.
~
#52106
pirate_feyd_rand~
0 b 2
~
emote chants, 'Farvium, freedium, requriet.' 
emote chants, 'Tulosta, dyriist, omniscientia, Feyd.'
~
#52107
Soldier_Generate_Assist~
0 k 6
~
eval soldiers %loaded% + 1
if %kids% < 6
set rnd %random.char%
mecho The Platinum Knight lets out a horrid SCREAM!
wait 2 s
emote starts to hum a strange mantra.
wait 2
mecho A Bright flash of light fills the room and two soldiers crawl forth through it.
mload mob 52118
mload mob 52118
if %rnd.vnum% == -1
mforce soldier hit %rnd.name%
else
mforce dragon hit %actor.name%
end if
set loaded %kids%
global loaded
endif
~
#52108
Knight_Bash~
0 k 15
~
remove sword
wear shield
bash    
stand
wait 2
remove shield
wield sword
~
#52109
Knight_Grum~
0 g 100
~
if %actor.align% < -349
roar
glance %actor.name%
say I see you have lowered yourself to the ways of evil
wait 2s
say For this, you shall die a terrible death
kill %actor.name%
elseif %actor.align% > 349
bow %actor.name%
say Finally a good soul has made its way down through this terrible ship
say Perhaps you would care to aid me in killing the awful Queen Tira
end
~
#52110
Arctic_Deathroom~
2 g 100
~
wait 1
wsend %actor% The loud CRACK of a door slamming chills your bones.
~
#52111
Hatch_Slam~
2 g 100
~
if (%actor.vnum% == -1)
if (%direction% == up)
wait 1
wsend %actor% The Hatch slams shut above you, creating a smooth airtight ceiling.
endif
endif
~
#52112
Random_Wind_Damage~
2 b 25
~
if %actor.vnum% == -1
   wait 1
   set rndm %random.char%
   wdamage %rndm% 65 cold
   if %damdone% == 0
      wsend %rndm% A strong gust blows in from the north, cooling you.
      wechoaround %rndm% A strong gust blows in from the north, which seems to soothe %rndm.name%.
   else
      wsend %rndm% A strong gust blows in from the north, giving you an awful chill. (&1%damdone%&0)
      wechoaround %rndm% A strong gust blows in from the north, which seems to chill %rndm.name%. (&4%damdone%&0)
   end
end
~
#52130
action_figure_chop~
1 c 1
squeeze~
if %arg% == legs
    return 1
    oecho %actor.name% squeezes the legs of a Dakhod action figure.
    oecho The Dakhod action figure swings its arm in a wicked karate chop!
else
    return 0
end
~
#52131
action_figure_squeeze_normalizer~
1 c 1
s~
* This trigger is needed on the action figure to return the command "s"
* back to its default function, rather than triggering the squeeze trigger.
return 0
~
#52132
action_figure_pull~
1 c 1
pull~
if %arg% == string
    return 1
    switch %random.9%
        case 1
            set phrase Ahoy scurvey mateys!
            break
        case 2
            set phrase Arrr, its the plank for ye!
            break
        case 3
            set phrase Combat is too fricking slow here.
            break
        case 4
            set phrase What the devil are you scallawags doing on my ship!?
            break
        case 5
            set phrase You there!  Return the &bBlack Pearl&0 to me, I say!
            break
        case 6
            set phrase Bring me one noggin of rum, now, won't you?
            break
        case 7
            set phrase Shiver me timbers!
            break
        case 8
            set phrase Fifteen gnomes on the dead man's chest, yo ho ho and a bottle of rum!
            break
        case 9
            set phrase Do you buckle your swash or swash your buckler?
            break
        default
            set phrase Yaaarrrrr!
    done
    oecho %actor.name% pulls the string on a Dakhod action figure.
    oecho The Dakhod action figure says, '%phrase%'
else
    return 0
end
~
#52133
action_figure_pull_normalizer~
1 c 1
pu~
return 0
* This trigger is required to make the command "pu" return to its default
* behavior, rather than setting off the pull trigger.
~
$~
