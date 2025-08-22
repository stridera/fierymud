#20000
figure_greet~
0 g 100
~
if %actor.vnum% == -1
if (%direction% == west)
wait 1
roar %actor.name%
say Are you friend or foe of Ruin Wormheart?
glare %actor.name%
~
#20001
black_glow~
1 j 100
~
oecho %self.shortdesc% emitts a dark glow through out the room.
wait 2
return 0
~
#20002
guard greet~
0 g 100
~
if %actor.vnum% == -1
   if %direction% == west
      if %actor.align% > -350
         msend %actor% %self.name% says to you, 'I cannot allow you to pass this point.'
         msend %actor% %self.name% stands in your way.
      else
         msend %actor% %self.name% nods at you.
         msend %actor% %self.name% says to you, 'Welcome to the Blackthorn Abbey.'
      endif
   endif
endif
~
#20003
friend?~
0 d 100
friend~
if %actor.vnum% == -1
say Then you are not welcome here, you must leave at once!
mforce %actor.name% w
endif
~
#20004
foe?~
0 d 100
foe~
say Then we are on the same side.
wait 1
grin
say When you come to the guard tell him 'I am an enemy of Ruin Wormheart
say and he will not bother you.
wave
mforce %actor.name% e
~
#20005
ent_ruin~
0 d 0
I am an enemy of Ruin Wormheart~
nod
say You may enter as an ally of the dark monks.
unlock gate
open gate
bow
~
#20006
boy_hay~
0 b 10
~
emote forks hay into a stall.
sigh
~
#20007
worn_teach~
0 b 10
~
emote cracks his whip.
say No no thats not how its done.
wait 2
say Like this.
emote sneaks around the room.
~
#20008
reach_oven~
1 g 100
~
odamage %actor% 15 fire
if %damdone% != 0
   osend %actor% As you reach into the oven your hand is suddenly burnt by the searing heat of the oven! (&1%damdone%&0)
   osend %actor% But you still manage to retreive the bread.
   oechoaround %actor% %actor.name% screams in pain as he reaches %actor.p% hand into the oven.
   oechoaround %actor% %actor.name% screams in pain as he reaches %actor.p% hand into the oven. (&1%damdone%&0)
end
~
#20009
stew~
0 g 100
(null)~
if %actor.vnum% == -1
say Would you like some stew?
endif
~
#20010
give_stew~
0 d 100
yes~
nod
mload obj 20045
give bowl %actor.name%
~
#20011
path_twig~
2 b 20
~
wecho A twig snaps somewhere close by.
~
#20012
baker_eat~
0 b 5
~
emote looks all around the room.
emote opens the oven and gets a piece of bread out.
emote eats a dark loaf of bread hurridly.
burp
~
#20013
guard_mutter~
0 b 10
~
mutter
say She has to come this way to get out.
peer
~
#20014
woman_greet~
0 g 100
~
whisper %actor.name% Can you help me?
~
#20015
woman_help~
0 d 100
help?~
nod
say I am trapped in this chamber and there are guards all over looking for me.
wait 1
say You see, i came to the living place of the dark monks to see my husband. 
say But i was caught and the dark monks do not allow women in their abbey. 
wait 1
say So they imprisoned me.
sigh
wait 1
say But i have managed to escape my cell, 
say and have only one more thing between me and freedom.   
say Will you help me escape?
~
#20016
woman_yes~
0 d 100
yes nods ok~
smile %actor.name%
wait 1
say There is a guard to the north that is blocking the way to my escape.
say I need you to go kill him and bring back his axe,
say so i have proof you have killed him.
say Go now.
mforce %actor.name% n
vis
~
#20017
rec_axe~
0 j 100
~
if %actor.vnum% == -1
if %object.vnum% == 20046
wait 1
mecho %self.name% begins to jump up and down excitedly.
say Thank you very much!
say Here is your reward for assisting me.
mload obj 20047
wait 2
mecho The small hurt woman pulls out a bright ball of light from inside her 
clothes.
wait 2
give sun %actor.name%
mjunk axe
wait 2
say I must return to my children!
mecho The small hurt woman quickly sneaks to the north.
mpurge woman
else
e
n
d
else
e
n
d
~
#20018
guard_ask~
0 g 100
~
say Are you here to see Yix'Xyua the great leader of the dark monks?
~
#20019
guard_yes~
0 d 100
yes~
nod
say Tell the prior 'I must see the abbot.
mload obj 20042
give key %actor.name%
bow
~
#20020
prior_see_abbot~
0 d 0
i must see the abbot~
nod
say You may see him at once!
unlock door w
open door w
~
#20021
abbot_talk~
0 gh 100
~
bow
wait 1
say So you have come too see our great leader.
ponder
wait 1
say His quarters are in the next room but we have moved him to safer quarters.
wait 1
say To see him you must prove yourself worthy by finding is quarters.
say I shall give you a small hint.
whisper %actor.name% He is located somewhere in the library.
mload obj 20043
give key %actor.name%
say Now on you go!
mteleport %actor.name% 20005
~
#20022
statue_made_it~
0 g 100
~
grin %actor.name%
say So you have made it.
wait 1
say You have proved yourself so now you will see the leader.
say When you are ready say the name of our great leader and you will go to him.
~
#20023
load _portal~
2 d 100
Yix'Xyua~
wecho The giant obsidian statue makes a magical gesture and a large black portal erupts from the ground!
wload obj 20049
wecho a giant obsidian statue says, 'When you want to leave just say exit.'
~
#20024
exit_lea~
2 d 100
exit~
wecho The room suddenly becomes pitch dark and a bright flash of light erupts from the ground.
wload obj 20052
wait 1
wecho Yix'Xyua says, 'Very well, on your way!'
wforce %actor.name% enter portal
~
#20025
guard_assist_leader~
0 k 28
~
panic
emot screams for his guardians to come to his aid.
wait 2
mload mob 20034
~
#20026
leader_talk~
0 g 100
~
say We where told by the guardians that someone was coming to see us.
mecho Yix'Xyua whispers something to the assistant.
say The master wants to know what have you come here for?
~
#20027
assistant_for?~
0 d 100
for?~
say Yes for.
sigh
wait 1
mecho Yix'Xyua whispers something to the assistant.
say Have you come here to assist us in our problem with Ruin Wormheart?
~
#20028
assistant_yes~
0 d 100
yes~
smile
wait 1
say Very good.
wait 1
say But first you must prove your worth.
~
#20029
assistant_worth?~
0 d 100
prove?~
wait 1
say Yes, you must prove to us that you are able to complete any job we give you.
wait 1
ponder
say This is what you are to do..
say You will go to the place where Ruin is located,
say and there you will find a ball of light.
wait 1
say Bring it to us and you will receive a prize and we will talk further.
whisper %actor.name% If you want to show us extra skill,
whisper %actor.name% you will bring us an oak staff,
whisper %actor.name% carved in the shape of a serpent.
wait 1
grin
wait 1
say Now on your way.
mteleport %actor.name% 20094
~
#20030
assistant_receive ball~
0 j 100
~
if %actor.vnum% == -1
if %object.vnum% == 3218
pat %actor.name%
say You have done well
mecho Yix'Xyua pulls something out of his clothes and gives it to his assistant.
mload obj 20053
wait 1
say Here is your reward for your bravery.
give gaze %actor.name%
say When we come up with a plan to destroy Ruin we will call for you.
mpurge ball
elseif %object.vnum% == 3217
wait 1s
say The leader will deal with this, give it to him.
give staff %actor.name%
endif
endif
~
#20031
leader_rec staff~
0 j 100
~
if %object.vnum% == 3217
wait 1
say Very Good!
wait 1
say Here is your extra prize for working so hard.
emote pulls something out from under the bed.
mload obj 20054
wait 1
say These will help you in fights against your enemy's.
wait 1
give bronze %actor.name%
wait 1
grin
mpurge staff
~
#20032
Assistant_return_staff~
0 j 100
~
Nothing.
~
#20033
palidan_ran~
0 b 15
~
bleed
wait 3
say why did i agree to fight against the dark monks, I knew they where to strong.
shake
wait 4
choke
wait 2
say now i must face my foolish mistakes.
~
#20034
blade_glow2~
1 j 100
~
oecho A white glow flows through out the room dousing all darkness!
~
#20035
bread_purge~
1 c 100
CURSED!~
ojunk bread
~
#20036
prisoner_quest~
0 j 100
~
wait 1
emote eyes light up with joy!
Wait 1
Say Thank you so much for giving me my freedom!
Wait 1
Emote pulls a small metal key out from his shirt.
mload obj 20066
give metal %actor.name%
mjunk obsidian
wait 1
say This key opens the small hovel that I dug in one of the cells of this prison.
say In it is hidden a great treasure.
Wait 1
say Good luck finding it.
Emote unlocks the cell door.
Emote opens the cell door.
Emote sneaks away quietly.
mpurge prisoner
~
#20037
guard_block_cell~
0 c 100
open~
Frown
emote spreads out his arms and blocks your way.
Say What do you think your doing?
~
#20038
drink_fountain~
1 c 100
drink~
return 0
wait 1
if %actor.level% < 99
   odamage %actor% 50
   oechoaround %actor% %actor.name% is caught in a fit of choking. (&1%damdone%&0)
   osend %actor% You are caught in a fit of choking. (&1%damdone%&0)
end
~
#20039
corpse disin~
1 g 100
~
if %actor.vnum% == -1
return 0
wait 1
osend %actor% As you grab the corpse from the coffin, it disintigrates in your hands!
oechoaround %actor% %actor.name% grabs the corpse and it disintigrates in %actor.p% hands!
opurge corpse
end
~
$~
$~
