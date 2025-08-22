#58401
princess_receive~
0 j 100
~
if %object.vnum% == 58417
   wait 1
   mecho %self.name% sighs in great relief.
   wait 2
   say Thank you from the bottom of my heart, kind adventurer.
   say Your deeds shall not go unrewarded.
   wait 2
   mjunk feathers
   mload obj 58401
   give feather %actor.name%
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
         if !%person.quest_stage[kod_quest]%
            quest start kod_quest %person.name%
         endif
         quest variable kod_quest %person% feather 1
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
endif
~
#58402
Unicorn girl speech~
0 d 1
wrong~
wait 1
mechoaround %actor% %self.name% speaks to %actor.name% in a low voice.
msend %actor% %self.name% says to you, 'The King will not free me, even though my obligation of service to him is done.  He shall never have my hand in marriage, thus I shall remain his slave forever.'
~
#58403
swan_unic_rand~
0 b 5
~
mecho The slave girl sniffs sadly.
~
#58404
Unicorn Girl receive~
0 j 100
~
if %object.vnum% == 58435
   wait 1
   mjunk legal
   mecho %self.name% begins to sob hysterically.
   say Thank you, thank you so very much!
   wait 2
   rem fetter
   drop fetter
   say I am free!
   wait 2
   say Please accept this gift, as reward for setting me free.
   wait 2
   mecho The slave girl pulls a necklace with a spiral horn from her tattered clothes.
   wait 2
   mload obj 58429
   give alicorn %actor.name%
   wait 2
   say I must return to my family!
   mecho The slave girl quickly runs away!
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
         if !%person.quest_stage[kod_quest]%
            quest start kod_quest %person.name%
         endif
         quest variable kod_quest %person% alicorn 1
      elseif %person%
            eval i %i% + 1
      endif
      eval a %a% + 1
   done
   mpurge girl
endif
~
#58410
dancer_quest_banter~
0 bg 13
~
emote sighs and returns to cleaning the wagon, barely noticed by the prince.
~
#58411
dancer_quest_banter2~
0 bg 10
~
emote sweeps dirt from a corner.
wait 4
sigh
~
#58412
Dancer_quest_ASK~
0 n 1
hi help hello howdy quest~
*  This is the initial quest start to the
*  relocate quest, other quests can be added
*  here with some work but this is going to 
*  be geared to the classes that get relocate
mecho DEBUG: Trigger running for %actor.name% of class (%actor.class%)
*  for the time being
if %actor.vnum% == -1
if %actor.level% >= 65
   if %actor.quest_stage[major_spell_quest]% < 1
      if %actor.has_completed[major_spell_quest]% /= false
set gogogo 0
if  %actor.class% /= Sorcerer
set gogogo 1
end
if %actor.class% /= Cryomancer
set gogogo 1
end
if %actor.class% /= Pyromancer
set gogogo 1
end
if %gogogo% == 1
	    wait 1
            think
            wait 1
            msend %actor% %self.name% says to you, 'I've been trapped here for so long.'
            sigh
            wait 1
msend %actor% %self.name% says to you, 'The Prince has me enslaved against my will, will you help set me free?'
            Say Yer a %actor.class%
            quest start major_spell_quest %actor.name%
         else
            grumble
            Say Ok, I can talk, I swear %actor.name% is a doofus.
         end
      else
      end
   else
   end
else
end
else
end
~
#58413
Dancer_quest_ASK2~
0 n 1
yes~
* This is to confirm the quest and advance more
if %actor.vnum% == -1
   if %actor.quest_stage[major_spell_quest]% == 1
      wait 1
      msend %actor% %self.name% says to you, 'Oh thank you!'
      msend %actor% %self.name% says to you, 'You must gain the princes favor..'
      msend %actor% %self.name% says to you, 'then maybe he will give me to you so you can set me free.'
      mechoabout %actor% %self.name% whispers something to %actor.name%
      quest advance major_spell_quest %actor.name%
      * this sets the player to level 2 in the quest
      wait 5
      mforce prince say Hey, what are you talking to them about?!
      mforce prince sigh
      wait 2
      mforce prince say You can't get good help now days...
   else
   end
else
end
~
#58414
Gypsy_Prince_ask1~
0 n 1
greeting hi hello quest~
if %actor.quest_stage[major_spell_quest]% == 2
   wait 1
   eye %actor.name%
   wait 1
   msend %actor% %self.name% says to you, 'Greetings, traveler what brings you to my camp?'
   mechoabout %actor% %self.name% speaks to %actor.name% in a low voice.
else
end
~
#58415
Gypsy_prince_ask2~
0 n 1
favor~
if %actor.quest_stage[major_spell_quest]% == 2
   wait 1
   msend %actor% %self.name% says to you, 'Ah, you are looking for my favor?'
   mechoabout %actor% %self.name% speaks to %actor.name% in a low voice.
   wait 1
   smile me
   msend %actor% %self.name% says to you, 'I get that quite a bit, actually... The last adventurer to'
   msend %actor% %self.name% says to you, 'seek my favor left our contract imcomplete... If you could'
   msend %actor% %self.name% says to you, 'track down that scoundrel and get my prize he is keeping..'
   msend %actor% %self.name% says to you, 'I would consider myself in your debt..'
   wait 1
   bow %actor.name%
   wait 1
   ask %actor.name% Will you gon on this quest for me?
   quest advance major_spell_quest %actor.name%
   * This sets the player to level 3 in the quest
else
end
~
#58416
gypsy_prince_ask3~
0 n 1
yes~
if %actor.quest_stage[major_spell_quest]% == 3
   wait 2
   mechoabout %actor% %self.name% speaks to %actor.name% in a low voice.
   msend %actor% %self.name% says to you, 'Excellent!  That rogue of a guard, a monstrous bronze statue'
   msend %actor% %self.name% says to you, 'that now guards the entrance of that accursed village was to'
   msend %actor% %self.name% says to you, 'search the Underdark for an item for me.  Go to him and demand'
   msend %actor% %self.name% says to you, 'it in the name of the Gypsy Prince of Calia and I shall reward you.'
   quest advance major_spell_quest %actor.name%
   * This sets the player to level 4 in the quest
else
end
~
#58417
Bronze_statue_ask~
0 n 1
gypsy prince task prize~
if %actor.quest_stage[major_spell_quest]% == 4
   wait 1
   emote snorts flames from his nostrils!
   growl
   wait 2
   shout What?! That backstabbing rogue wants his prize?!
   think
   wait 2
   Say Well that's just to bad, I guess I'll just dispatch you myself.
   quest advance major_spell_quest %actor.name%
   * This sets the player to level 5 in the quest
   wait 2
   mkill %actor.name%
   mload obj 52050
else
end
~
#58418
Bronze_statue_death~
0 f 100
~
if %actor.quest_stage[major_spell_quest]% == 5
   
else
end
~
$~
