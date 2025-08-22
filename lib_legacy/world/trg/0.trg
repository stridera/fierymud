#1
command_trig_test~
0 d 100
kneel~
   say running the ASK trigger
   wait 3 s
   say the speech was %speech%
   wait 10
   say finished
~
#2
obj get test~
1 g 100
~
oecho You hear, 'Please put me down, %actor.name%'
~
#3
no littering~
2 h 100
~
wsend %actor% You cannot litter here!
return 0
~
#4
new trigger~
0 c 0
kneel~
return 0
mecho Command trigger (cackle) running
~
#5
weather speech~
2 d 100
weather~
wdoor 1210 n room 1250
wecho &6The air wrinkles and swirls into a door to the north&0
return 0
~
#6
firecaster~
1 gj 100
~
oecho A &1Flame&0 snakes up and down the blade of %self.shortdesc%
return 1
~
#7
Kerristone Castle (South)~
0 j 100
(null)~
if (%object.vnum% == 14750)
wait 5
say Good!
else
wait 5
say bad!
end
~
#8
Shadow Doom 1~
0 g 100
~
if (%actor.vnum% == -1)
if (%direction% == east)
mload obj 9036
give badge %actor.name%%
close gate east
lock gate east
endif
~
#9
Shadow Doom 2~
0 j 100
~
if (%object.vnum% == 9036)
wait 5
msend %actor% The &0&9&bShadow Doom&0 guard grins wickedly, and escourts you from the hall.&0
mteleport %actor% 9120
mechoaround %actor% %actor.name% is thrown out from the &0&9&bShadow Doom&0 hall.&0
else
wait 5
say What the hell is this?!
drop %object.name%
return 0
endif
~
#10
Obelisk (Food)~
2 d 0
All Hail Uklor~
if (%actor.vnum% == -1)
wecho &0&9&bThe ancient &0&5obelisk&0&9&b hums with pleasure.&0
wecho A gust of &0&7&ndivine&0&3&b energy&0 sweeps through the room, leaving a loaf of &0&3bread&0 in its wake.
wload obj 14711
end
~
#11
Kerristone (Royal Stables)~
0 m 25
~
mecho &0&3&bLicub&0 nods to %actor.name% and goes to fetch a horse.
wait 5
mecho &0&3&bLicub&0 returns with a &0&3well-bred warhorse&0.
~
#12
arena~
0 k 100
~
cast 'heal'%mobile%
wait 15
~
#13
Templace Gate~
0 g 60
~
if level ($n) >= 80
mpecho $n $I tells you, 'Hi $n, Welcome to city of Templace!'
mpechoaround $n $I welcomes $n
endif
if level ($n) <= 79
mpechoat $n $I tells you, 'Hey, this is too dangerous a place for you!'
mpechoaround $n $I looks at $n doubtfully and makes a strange gesture.
MPTRANSFER $n 3001
if isimmort ($n)
mpecho $n $I falls to the ground and starts worshiping you!
mpechoaround $n $I falls to the ground in total awe of $n's power.
endif
~
#14
King of dreams (Swan Princess)~
0 j 100
~
if (%object.vnum% == 58417)
emote sighs in great relief.
say Thank you from the bottom of my heart, kind adventurer.
say Your deeds shall not go unrewarded.
mload obj 58401
give feather $n
else
say No, I thank you but this will not do.
say Not do at all.
mpjunk $o
endif
~
#15
Obz Kwan Yin Release~
0 j 100
~
if (%object.vnum% == 58200)
emote stares at the key in astonishment.
say You.. you're releasing me?
remove pearl
remove pearl
say Take these as tokens of my appreciation, kind one.
give pearl $n
give pearl $n
emote turns and flies out of the cage, in the blink of an eye, and is gone.
mpjunk $o
mpurge
~
#16
Eye peck~
0 k 10
~
msend %actor% %self.name% pecks out your eyes!
mechoaround %self.name% pecks out %actor.name%'s eyes!
~
#25
instant reboot~
2 d 0
reboot now~
wecho %people.3054%
wecho %people.51036%
~
#40
Minor Life Restore~
1 k 100
~
eval heal %actor.maxhit% / 5
log %actor.name% would have died.  %self.name% broke to restore 20%% hp.  %heal% hp healed.
oheal %actor% %heal%
oecho Your %self.shortdesc% shatters and heals you for %heal% hp.
opurge %self%
~
#85
Mausloeum trigger~
2 d 100
Ziijhan~
wdoor 8524 w room 8525
wecho You hear the slow grind of rock against rock. A doorway appears to the west. 
return 0
~
#86
Mausoleum 2~
2 d 100
path~
wdoor 8525 d room 8583
wecho The Blood of the Evil Runes begins to boil, and solidifies into a trapdoor.
return 0
~
#87
antipaladin quest~
0 j 100
~
if (%object.vnum% == 8504)
wait 5
emote throws his head back in childish delight!
else
wait 5
say You are a miserable failure!
~
#88
run~
2 b 25
~
wecho The wind howls around the Cathedral.
wecho The wind seems to whisper run....run!
~
#89
blind seer~
0 i 25
~
emote grumbles some nonsense about his master.
~
#90
blind seer~
0 i 25
~
emote seems to be looking for someone.
~
#91
howls~
2 b 25
~
wecho You hear the screams of the tortured.
~
#92
shadows~
2 b 15
~
wecho Out of the corner of your eye you notice a strange shadow through the window.
~
#93
gargoyles~
2 b 15
~
wecho High above a gargoyle is perched, ready to pounce!
~
#94
vines~
2 b 25
~
wecho A vine nips at your heels!
~
#95
tree branch~
2 b 20
~
wecho A tree branch makes a lunge for your head!
~
#96
Evil energy~
2 b 15
~
wecho The balcony crackles with evil energy!
~
#97
crystalline monument (griffin)~
1 b 10
~
oecho The crystalline monument begins to glow and hum, then stops abruptly.
~
$~
