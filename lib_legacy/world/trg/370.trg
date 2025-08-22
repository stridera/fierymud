#37000
phantom-guardian-block-north~
0 c 100
north~
msend %actor% The phantom turns into a wall and blocks your way!
mechoaround %actor% Suddenly, the phantom guardian takes the shape of a wall, blocking passage.
~
#37001
mezmeriz_death_speech~
0 f 100
~
say You'll never get my treasure!
~
#37002
Phantom_guard_greet~
0 g 100
~
* This is a prog to greet players
if %actor.vnum% == -1
msend %actor% %self.name% says, 'This is no place for puny mortals, like yourself!'
msend %actor% %self.name% says, 'Go home at once!'
else
end
~
#37003
Engaja_speech1~
0 d 100
expected expected?~
mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
msend %actor% %self.name% says to you, 'He knew you were on your way, so he tried to make your'
msend %actor% %self.name% says to you, 'journey as easy as possible, but I guess you aren't as'
msend %actor% %self.name% says to you, 'good as he thought. He always overestimates his opponents.'
~
#37004
Engaja_all_greet1~
0 h 100
~
if %actor.vnum% == -1
msend %actor% %self.name% says, 'What took you so long?'
msend %actor% %self.name% says, 'Mesmeriz expected you at least a week ago...'
else
end
~
#37005
Cirion_speech1~
0 d 1
slaves slaves?~
mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
msend %actor% %self.name% says to you, 'Our bodies are getting old, and are starting to decay, but'
msend %actor% %self.name% says to you, 'our minds and souls are still strong. Now he can kill us,'
msend %actor% %self.name% says to you, 'and set our spirits free. Freedom here I come!'
thank %actor.name%
~
#37006
Cirion_speech2~
0 d 1
summon summoned~
mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
msend %actor% %self.name% says to you, 'Don't think you came here on your own free will.'
msend %actor% %self.name% says to you, 'If you do then you are only deceiving yourself even more.'
msend %actor% %self.name% says to you, 'Mesmeriz brought you here.'
~
#37007
Cirion_all_greet1~
0 h 100
~
* All greet trig for players
if %actor.vnum% == -1
msend %actor% %self.name% says, 'Ah, finally Mesmeriz has summoned someone else here to be his slaves.'
else
end
~
#37008
Minithawk_messenger_speech1~
0 d 1
killed killed?~
mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
msend %actor% %self.name% says to you, 'Yes, killed...as in dead.  Mesmeriz would have my head if he'
msend %actor% %self.name% says to you, 'knew I were talking to you.  His direct orders were for me to'
msend %actor% %self.name% says to you, 'go to Templace, and deliver this rock.'
~
#37009
Messenger_test_chatter~
0 h 100
~
* This was probably to be intended as a serial
* monologue that the messenger spoke at a timed
* interval.  Maybe this could be a random type
* of trig that said each of these lines or pieces
* of related topic at random times.
wait 1 s
say Which way to town!?
wait 5 s
say I must get this to town!
wait 5 s
say No, wait a second...
wait 30
say What am I thinking? I hold the only piece of Thawkinixa!
wait 30
say I am rich!!! I'm going to keep it.
wait 5 s
say Wait a minute, if I don't return this to town I will be killed!
wait 5 s
say Oh, what to do
wait 20
say what to do...
~
#37010
Minithawk_messenger_speech2~
0 d 1
north east south west~
mechoabout %actor% %self.name% speaks to %actor.name% in a low voice.
msend %actor% %self.name% says to you 'Thank you very much!  Now if I could just find a way out of this mine!'
~
#37011
Minithawk_messenger_speech3~
0 d 1
town town?~
mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
msend %actor% %self.name% says to you, 'Yes, I must go to Templace. Which way?'
~
#37012
minithawk_entrance_closedoff~
2 g 100
~
if %direction% == south
    wecho Rays of light creep in as the lumber around the mine's entrance is pulled back.
    wait 1
    wecho The lumber around the mine's entrance suddenly grows unstable, creaking loudly.
    wecho In a great cloud of dust the mine entrance collapses, leaving no way back out.
endif
~
#37013
baldathor_greet~
0 g 100
~
msend %actor% The warrior's eyes begin to glow &1&bred&0 as he looks straight at you!
wait 1
snarl %actor%
~
#37014
mes-greet~
0 h 100
~
if %actor.race% /= troll
   if %actor.quest_stage[troll_quest]% != 1
      msend %actor% %self.name% says to you, 'Welcome!  It has been many years indeed since I have had the pleasure of conversing with another troll from the lands above.'
      msend %actor% %self.name% says to you, 'Unfortunately my research has kept me belowground and  far from the swamps of our home.'
      sigh
      wait 3s
      consider %actor.name%
      if %actor.level% >= 55
         msend %actor% %self.name% says to you, 'It looks like we are still the strongest of the clans, my friend.  Perhaps we can help each other.'
         quest start troll_quest %actor.name%
         msend %actor% %self.name% says to you, 'Long ago, some powerful items of the trolls were stolen by jealous Shamen from different tribes and hidden away.'
         msend %actor% %self.name% says to you, 'If you were to bring the objects back to me, I could reward you quite handsomely.'
         wait 1s
         msend %actor% %self.name% says to you, 'One is the the bough of a sacred mangrove tree that stood in the courtyard of a great troll palace before it was destroyed by a feline god of the snows.'
         wait 1s
         msend %actor% %self.name% says to you, 'One is a vial of red dye made from the blood of our enemies, stolen by a tribe in a canyon who wished to unlock its power.'
         wait 1s
         msend %actor% %self.name% says to you, 'One is a large hunk of malachite, a stone we have always valued for its deep green color.  Our enemies the swamp lizards also liked its color, however, and guard it jealously.'
         wait 2s
         msend %actor% %self.name% says to you, 'I am sure you can restore our lost honor.  Return them to me and the power of the Trolls shall be known once again!'
         set troll_quest 1
         global troll_quest
         else
         wink %actor.name%
         msend %actor% %self.name% says to you, 'Come back to me when you have grown a bit and I shall see what we can do for each other, young one.'
   endif
  elseif %actor.quest_stage[troll_quest]% == 1
         msend %actor% %self.name% says to you, 'Welcome back!  If you have the items, give them to me.'
   endif
 else
      frown
      msend %actor% %self.name% says to you, 'And what exactly are YOU doing here?!'
      wait 1s
      sigh
      msend %actor% %self.name% says to you, 'No, do not answer.  It seems I shall have to deal with you myself after all.'
      msend %actor% %self.name% says to you, 'You must be quite resourceful to have found me, but I cannot be bothered just now.  Leave me.'
      wait 2s
      mecho %self.name% turns away from you.
      msend %actor% %self.name% says, 'Or attack me, if you are feeling particularly stupid.'
endif
~
#37015
phantom-guardian-block-east~
0 c 100
east~
msend %actor% The phantom turns into a wall and blocks your way!
mechoaround %actor% Suddenly, the phantom guardian takes the shape of a wall, blocking passage.
~
#37016
phantom-guardian-block-west~
0 c 100
west~
msend %actor% The phantom turns into a wall and blocks your way!
mechoaround %actor% Suddenly, the phantom guardian takes the shape of a wall, blocking passage.
~
#37017
phantom-guardian-block-south~
0 c 100
south~
msend %actor% The phantom turns into a wall and blocks your way!
mechoaround %actor% Suddenly, the phantom guardian takes the shape of a wall, blocking passage.
~
#37018
mes-rec~
0 j 100
~
*first lets check the PC is the right race!
if %actor.race% /= troll
*now lets check to make sure the item is correct and we didn't already have it, if both are good then lets mark it gotten.
if (%object.vnum% == 37080)
set gooditem 1
if (%actor.quest_variable[troll_quest:got_item:37080] != 1)
set gotitem 1
quest variable troll_quest %actor.name% got_item:37080 1
endif
elseif (%object.vnum% == 37081) 
set gooditem 1
if (%actor.quest_variable[troll_quest:got_item:37081] != 1)
set gotitem 1
quest variable troll_quest %actor.name% got_item:37081 1
endif
elseif (%object.vnum% == 37082)
set gooditem 1
if (%actor.quest_variable[troll_quest:got_item:37082] != 1)
set gotitem 1
quest variable troll_quest %actor.name% got_item:37082 1
endif
else
set gooditem 0
endif
*alright, do we have everything now?
if (%actor.quest_variable[troll_quest:got_item:37080]% == 1 && %actor.quest_variable[troll_quest:got_item:37081]% == 1 && %actor.quest_variable[troll_quest:got_item:37082]% == 1 && !%actor.has_completed[troll_quest]%)
*if everything is handed in we're done!
mecho %self.name% smiles broadly as he looks at the gathered ingredients. 
say Yes, these will do quite nicely indeed.  Excellent work. 
wait 2s 
mecho %self.name% opens the seal on the vial of dye and a sweet scent fills the  room. 
mecho Taking the vial on one hand, he slowly pours it over the mangrove branch, creating a slight hissing noise and a black, oily-smelling smoke. 
wait 1s 
mecho %self.name% places the hunk of malachite amidst the smoking concoction and 
waves his hands over it, muttering a strange mantra. 
mecho A red light erupts from the center of the branch, nearly blinding you! 
wait 2s 
mecho When the smoke clears and the light is gone, %self.name% smiles proudly. 
quest complete troll_quest %actor.name%
say Here you go, little one.  May your ambitions guide you and lead you to prosperity and rulership. 
mload obj 37083
give troll-mask %actor.name% 
bow %actor% 
wait 1 
say I am certain we shall meet again. 
*okay maybe we've already done this quest
elseif (%actor.has_completed[troll_quest]%)
wait 3s
say You have already helped me once, please let others help me now.
give %object.name% %actor.name%
*what if we don't have everything but did have a good item?
elseif (%gotitem% == 1)
wait 3s 
say Why thank you.  Run along now.
mjunk %object.name%
*so what if it's a good item but we already have it?
elseif (%gooditem% == 1)
wait 3s
say I'm sorry but you already gave me this item.
give %object.name% %actor.name%
*uh oh, bad item, didn't need this one
else
wait 3s
say What is this? I can not use this.
give %object.name% %actor.name%
endif
*ending from the race check.
else
if %object.vnum% == 12526
wait 3s
mjunk gem
blink
say How did you know I needed this?!
wait 2s
mecho %self.name% quickly pockets the ruby.
say Here, my thanks.
set lap 1
while %lap% < 11
mexp %actor% 10000
eval lap %lap% + 1
done
set random_gem %random.11%
eval which_gem %random_gem% + 55736
mload obj %which_gem%
set random_gem %random.11%
eval which_gem %random_gem% + 55736
mload obj %which_gem%
set random_gem %random.11%
eval which_gem %random_gem% + 55736
mload obj %which_gem%
give all.gem %actor.name%
say Here is a small token of my appreciation.
say Now move along.
else
wait 1s
say What is this? I can not use this.
return 0
end
end
~
#37019
thawkinixa_madness_curse~
1 b 1
~
set worn %self.worn_by%
set carried %self.carried_by%
if !%worn% && !%carried%
    halt
elseif %worn.vnum% == -1
    osend %worn% Desire for thawkinixa overwhelms your thoughts!
    ocast confusion %worn%
elseif %carried.vnum% == -1
    osend %carried% Desire for thawkinixa overwhelms your thoughts!
    ocast confusion %carried%
endif
~
#37050
troll-quest_shaman-load-mangrove~
0 gh 100
~
if %actor.race% /= troll
mjunk mangrove-branch
mload obj 37080
say You're a troll
else
say You're not a troll!
end
~
#37051
troll-quest_shaman-load-dye~
0 g 100
~
if %actor.race% /= troll
mjunk red-dye
mload obj 37081
end
~
#37052
troll-quest_malachite-load~
0 g 100
~
if %actor.race% /= troll || %actor.quest_stage[acid_wand]% == 7
  mjunk malachite
  mload obj 37082
endif
~
#37060
room_37060_randoms~
2 b 25
~
wecho The flashing lights dance rhythmically across your vision, slowly hypnotizing you into a trance.
wait 4s
wecho A loud bang explodes in the center of the circle, snapping you out of your trance!
wecho The glowing circle drawn on the floor intensifies, nearly blinding you with its brightness.
wait 3s
wload mob 37060
wecho As you look on, a small black portal opens in the center of the circle.
wecho Suddenly, a black phantasmic shape flies up out of the portal.
wecho The portal subsides to nothingness behind the phantom.
wait 3s
wecho The phantasmic shape melts through the ceiling above you.
wpurge phantasmic-phantom
~
#37080
troll-mask~
1 j 100
~
if %actor.race% /= troll
   return 1
   oechoaround %actor% %actor.name% looks suddenly more fierce.
   osend %actor% You feel the strength of the Trolls in your blood!
else
   return 0
   osend %actor% You cannot wear the trollish mask.
endif
~
$~
