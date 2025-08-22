#51900
academy_recruitor_speech_select~
0 d 100
basic combat none~
wait 2
if %speech% == basic
  if %actor.quest_stage[school]% == 0
    quest start school %actor%
    msend %actor% %self.name% tells you, 'Then welcome to the Ethilien Training Academy!
    msend %actor% Here you'll learn the basic commands to play FieryMUD.
    msend %actor% &0  
    msend %actor% &0&6&bBe sure to read everything your teachers tell you!&0
    msend %actor% If you just skip to the next command, you'll miss out on a lot!!!'
    wait 2s
    msend %actor% %self.name% tells you, 'Each command will have parenthesis around the first few letters.
    msend %actor% That is the shortest string you can use.'
    wait 2s
    msend %actor% %self.name% tells you, 'Tutorial commands will appear &2&bin green text&0.
    msend %actor% Type those phrases &2&bexactly&0 as they appear.
    msend %actor% Remember, &2&bspelling matters!!!&0'
    wait 2s
    msend %actor% %self.name% tells you, '&2&bSay ready&0 when you want to move on.'
  else
    unlock gates
    open gates
    msend %actor% %self.name% tells you, 'It's always a good idea to review the basics.'
    quest erase school %actor%
    quest start school %actor%
    msend %actor% %self.name% escorts you into the Academy.
    mforce %actor% east
    close gates
    lock gates
  endif
elseif %speech% == none
  if %actor.level% == 1
    if %actor.quest_stage[school]% == 0
      quest start school %actor%
      msend %actor% %self.name% tells you, 'Then you can begin your adventures at the Forest Temple of Mielikki!  Good luck and happy hunting!'&0
      wait 2s
      msend %actor% %self.name% tells you, 'You may come back train at the academy at any time.'&0
    else
      msend %actor% %self.name% tells you, 'You can resume or repeat your training at any time if you wish.'&0
    endif
  else
    msend %actor% %self.name% tells you, 'You seem to have a handle on what to do anyway.  You may come back and go through the training academy at any time.'&0
  endif
  wait 2s
  msend %actor% %self.name% waves good-bye and escorts you away.
  mforce %actor% up
  if !%actor.has_completed[school]%
    quest complete school %actor%
  endif
elseif %speech% == combat
  if %actor.quest_stage[school]% == 0
    quest start school %actor%
  else
    quest erase school %actor%
    quest start school %actor%
  endif
  quest advance school %actor%
  quest advance school %actor%
  msend %actor% %self.name% tells you, 'Tutorial commands will appear &2&bin green text&0.
  msend %actor% Type those phrases &2&bexactly&0 as they appear to advance.
  msend %actor% Remember, &2&bspelling matters!!!&0'
  wait 3s
  switch %actor.class%
    case rogue
    case thief
    case assassin
    case mercenary
    case bard
      msend %actor% %self.name% tells you, 'I see you're a stealthy type.  You'll do best in lessons with Doctor Mischief.'
      set direction down
      break
    case sorcerer
    case cryomancer
    case pyromancer
    case necromancer
    case illusionist
      msend %actor% %self.name% tells you, 'I see you're an arcane spell caster.  You would definitely benefit from the Chair of Arcane Studies' seminar on spellcasting.'
      set direction south
      break
    case cleric
    case priest
    case diabolist
    case druid
      msend %actor% %self.name% tells you, 'I see you're a divine spell caster.  You would definitely benefit from private classes with the Professor of Divinity.'
      set direction east
      break
    case warrior
    case paladin
    case anti-paladin
    case ranger
    case monk
    case berserker
    default
      msend %actor% %self.name% tells you, 'I see you're a fighter type.  You'll do best learning from the Academy's Warmaster.'&0
      set direction north
      break
    default
      msend %actor% %self.name% tells you, 'I have no idea what to do with your class.  Please contact a god!'
  done
  wait 2s
  mteleport %actor% 51908
  mat 51908 mforce %actor% %direction%
endif
~
#51901
academy_instructor_greet~
0 h 100
~
if %actor.quest_stage[school]% == 1
  set speech %actor.quest_variable[school:speech]%
  set gear %actor.quest_variable[school:gear]%
  set explore %actor.quest_variable[school:explore]%
  wait 1s
  msend %actor% %self.name% tells you, 'Greetings adventurer!  I'm your Intro to Adventuring instructor.  I teach the basics of &3&bCOMMUNICATION&0, &3&bGEAR&0, and &3&bEXPLORATION&0.'
  wait 3s
  msend %actor% %self.name% tells you, 'It looks like you still need to complete lessons on:
  if %speech% != complete
    msend %actor% &3&bCOMMUNICATION&0
  endif
  if %gear% != complete
    msend %actor% &3&bGEAR&0
  endif
  if %explore% != complete
    msend %actor% &3&bEXPLORATION&0
  endif
  msend %actor% &0 
  msend %actor% You can &2&bsay&0 any of these to start a lesson on it or pick up where you left off.'
  msend %actor% &0 
  if %speech% != complete && (%gear% == complete || !%gear%) && (%explore% == complete || !%explore%)
    msend %actor% %self.name% tells you, 'If you're new to MUDs, I recommend you start with &3&bCOMMUNICATION&0 by typing &2&bsay communication&0.'
    msend %actor% &0 
  endif
  msend %actor% %self.name% tells you, 'You can also say &5SKIP&0 to move on to the next instructor.'
endif
~
#51902
academy_instructor_hello_class~
0 n 0
hello class~
if %actor.quest_variable[school:speech]% == 1
  quest variable school %actor% speech 2
  wait 2
  msend %actor% %self.name% tells you, 'Exactly!'
  wait 1s
  msend %actor% %self.name% tells you, 'You can talk to one person at a time by using the &6&b(T)ELL&0 command.
  msend %actor% Only the person you TELL to will hear what you say.
  msend %actor% You can talk to anyone, anywhere in the world.
  msend %actor% You do it by typing &6&btell [person] [message]&0'
  wait 3s
  msend %actor% %self.name% tells you, 'Try typing &2&btell instructor hello teacher&0.'
endif
~
#51903
academy_instructor_speech2~
0 d 0
hello teacher~
if %actor.quest_variable[school:speech]% == 2
  quest variable school %actor% speech 3
  wait 2
  msend %actor% %self.name% tells you, 'Well hello yourself!  You're learning well!'
  wait 1s
  msend %actor% %self.name% tells you, 'Mass communication channels include:'
  msend %actor%  
  msend %actor% &0&6&b(GOS)SIP&0 which talks to everyone in the game at once.
  msend %actor% &0   It's definitely the most popular communication channel.
  msend %actor% &0   It's a great way to ask questions or get help.
  msend %actor%  
  msend %actor% &0&6&b(SH)OUT&0 which talks to everyone in your zone.
  msend %actor%  
  msend %actor% &0&6&b(PETI)TION&0 which talks to all the Gods currently online.
  msend %actor% &0   It's best for asking semi-private or admin questions.
  msend %actor%  
  msend %actor% &0&6&bGROUPSAY&0 or &6&bGSAY&0 talks to everyone you're grouped with.
  msend %actor% &0   Check &6&bHELP GROUP&0 for more information on grouping.
  msend %actor%   
  msend %actor% You can also use &6&bLASTGOS&0 and &6&bLASTTEL&0 to see what the last messages you received were.
  msend %actor%   
  msend %actor% To see who's logged on, use the &6&bWHO&0 command.
  wait 3s
  msend %actor% %self.name% tells you, 'Do you have any questions about communication?
  msend %actor% You can &2&bsay yes&0 or &2&bsay no&0.'
endif
~
#51904
academy_instructor_speech_yes_no_continue~
0 d 100
yes no continue~
wait 2
if %actor.quest_stage[school]% == 1
  if %speech% == yes
    if %actor.quest_variable[school:speech]% == 3
      msend %actor% %self.name% tells you, 'What would you like to review?  You can say:
      msend %actor% &3&bSay&0
      msend %actor% &3&bTell&0
      msend %actor% &3&bGossip&0&_
      msend %actor% I'll know you're ready to move on if you &2&bsay continue&0.'
    elseif %actor.quest_variable[school:gear]% == 16
      msend %actor% %self.name% tells you, 'What would you like to review?  You can say:
      msend %actor% &3&bInventory&0
      msend %actor% &3&bWear&0
      msend %actor% &3&bEquipment&0
      msend %actor% &3&bLight&0
      msend %actor% &3&bRemove&0
      msend %actor% &3&bGet&0
      msend %actor% &3&bDrop&0
      msend %actor% &3&bJunk&0
      msend %actor% &3&bGive&0
      msend %actor% &3&bPut&0&_
      msend %actor% I'll know you're ready to move on if you &2&bsay continue&0.'
    elseif %actor.quest_variable[school:explore]% == 6
      msend %actor% %self.name% tells you, 'What would you like to review?  You can say:
      msend %actor% &3&bLook&0
      msend %actor% &3&bSearch&0
      msend %actor% &3&bDoors&0
      msend %actor% &3&bScan&0
      msend %actor% &3&bMovement&0&_
      msend %actor% I'll know you're ready to move on if you &2&bsay continue&0.'
    endif
  else 
    if %actor.quest_variable[school:speech]% == 3
      quest variable school %actor% speech complete
    elseif %actor.quest_variable[school:gear]% == 16
      quest variable school %actor% gear complete
    elseif %actor.quest_variable[school:explore]% == 6
      quest variable school %actor% explore complete
    endif
    if %actor.quest_variable[school:speech]% == complete && %actor.quest_variable[school:gear]% == complete && %actor.quest_variable[school:explore]% == complete
      set advance yes
    else 
      set advance no
    endif
    if %advance% == yes
      quest advance school %actor%
      msend %actor% %self.name% tells you, 'Then you're ready to move on!
      msend %actor% Proceed &2&beast&0.
      msend %actor% In the next room you'll prepare for combat training!'
    elseif %advance% == no
      msend %actor% %self.name% tells you, 'It looks like you still need to complete lessons on:
      if %actor.quest_variable[school:speech]% != complete
        msend %actor% &3&bCOMMUNICATION&0
      endif
      if %actor.quest_variable[school:gear]% != complete
        msend %actor% &3&bGEAR&0
      endif
      if %actor.quest_variable[school:explore]% != complete
        msend %actor% &3&bEXPLORATION&0
      endif
      msend %actor% &0 
      msend %actor% You can &2&bsay&0 any of these to start a lesson on it or say &5SKIP&0 to move on to the next teacher.'
    endif
  endif
endif
~
#51905
academy_instructor_speech_say_tell_gossip~
0 d 100
say tell gossip~
wait 2
if %actor.quest_variable[school:speech]% == 3
  if %speech% == say
    msend %actor% %self.name% tells you, 'The most basic command is &6&b(SA)Y&0.
    msend %actor% It sends your message to everything in the room, even NPCs and monsters.'
  elseif %speech% == tell || %speech% == whisper
    msend %actor% %self.name% tells you, 'You can talk to one person by using &6&b(T)ELL&0.
    msend %actor% Only the person you talk to will hear what you say, just like how I'm talking to you right now.
    msend %actor% You use it by typing &6&bTELL [person] [message]&0.'
  elseif %speech% == gossip
    msend %actor% %self.name% tells you, '&6&b(GOS)SIP&0 talks to everyone in the game.
    msend %actor% It's definitely the most popular communication channel.
    msend %actor% It's a great way to ask questions or get help.'
  endif
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'Do you have any other questions about communication?
  msend %actor% %self.name% tells you, 'You can &2&bsay yes&0 or &2&bsay no&0.'
endif
~
#51906
academy_instructor_command_inventory~
0 c 100
inventory~
if %actor.quest_variable[school:gear]% == 1
  mforce %actor% inventory
  quest variable school %actor% gear 2
  wait 2
  msend %actor% %self.name% tells you, 'Good!  Now you can see everything you have.
  msend %actor% In order to gain benefits from items, you have to equip them.'
  wait 3s
  msend %actor% %self.name% tells you, 'There are three commands to equip items:
  msend %actor% &6&b(WEA)R&0, &6&b(WI)ELD&0, and &6&b(HO)LD&0.'&0
  msend %actor% 
  msend %actor% &6&bWEAR&0 will equip something from your inventory.
  msend %actor% You can equip most objects by typing &6&bWEAR [object]&0.
  msend %actor% Weapons can be equipped with either &6&bWEAR&0 or &6&bWIELD&0.
  msend %actor% &6&bWEAR ALL&0 will equip everything in your inventory at once.
  msend %actor% 
  msend %actor% Some items can only be equipped with the &6&bHOLD&0 command.
  msend %actor% That includes instruments, wands, staves, magic orbs, etc.
  msend %actor% They will not be equipped with the &6&bwear all&0 command.
  msend %actor% Once you are holding them they can be activated with the &6&bUSE&0 command.
  wait 7s
  msend %actor% %self.name% tells you, 'Go ahead and type &2&bwear all&0 and see what happens.'
endif
return 0
~
#51907
academy_instructor_command_wear~
0 c 100
wear~
switch %cmd%
  case w
  case we
    return 0
    halt
done
if %actor.quest_variable[school:gear]% == 2 && %arg% == all
  mforce %actor% wear all
  quest variable school %actor% gear 3
  wait 2
  msend %actor% %self.name% tells you, 'That's how you do it!'
  wait 2s
  msend %actor% %self.name% tells you, 'The &6&b(EQ)UIPMENT&0 command shows what gear you're using.
  msend %actor% You are gaining active benefits from these items.
  msend %actor% They will not show up in your inventory.'
  wait 2s
  msend %actor% %self.name% tells you, 'Type &2&bequipment&0 or just &2&beq&0 to try it out.'
endif
return 0
~
#51908
academy_instructor_command_equipment~
0 c 100
equipment~
switch %cmd%
  case e
    return 0
    halt
done
if %actor.quest_variable[school:gear]% == 3
  quest variable school %actor% gear 4
  mforce %actor% eq
  wait 2
  l %actor%
  msend %actor% %self.name% tells you, 'You look well equipped to me!'
  wait 2s
  msend %actor% %self.name% tells you, 'Another way to equip things is with the &6&b(HO)LD&0 command.
  if %actor.inventory[23]% || %actor.wearing[23]%
    msend %actor% You started play with a torch.'
  else
    msend %actor% Here, take this torch for example.'
    mload obj 23
    give torch %actor%
  endif
  wait 3s
  msend %actor% %self.name% tells you, 'Type &2&bhold torch&0 to equip it.'
endif
return 0
~
#51909
academy_torch_command_light~
1 c 3
light~
switch %cmd%
  case l
    return 0
    halt
done
if %actor.quest_variable[school:gear]% == 5 && %arg.vnum% == 23
  oforce %actor% light %arg%
  quest variable school %actor% gear 6
  wait 2
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'Now you can see in dark spaces or outside at night!
  osend %actor% Remember, most lights have a limited duration.
  osend %actor% It's best to turn them off when not using them.
  osend %actor% &0 
  osend %actor% Lights don't have to be equipped for you to see.
  osend %actor% They work just fine from your inventory.'
  wait 3s
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'You can stop using items by typing &6&bREMOVE [object]&0.'
  osend %actor% &0   
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'Stop wearing that torch by typing &2&bremove torch&0.'
endif
return 0
~
#51910
academy_torch_remove~
1 l 100
~
if %actor.quest_variable[school:gear]% == 6
  quest variable school %actor% gear 7
  wait 2
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'Good, save that light!'
  wait 2s
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'During your adventures, you can pick up stuff using the &6&b(G)ET&0 command.'
  wait 2s
  oforce instructor mload obj 51902
  oforce instructor mload obj 51902
  oforce instructor mload obj 51902
  oforce instructor mload obj 51902
  oforce instructor mload obj 51902
  oforce instructor drop all.stick
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'Pick up one of those sticks by typing &2&bget stick&0.'
endif
~
#51911
academy_instructor_command_get~
0 c 100
get~
if %actor.quest_variable[school:gear]% == 7
  if %arg.vnum% == 51902
    quest variable school %actor% gear 8
    mforce %actor% get %arg%
    wait 2
    msend %actor% %self.name% tells you, 'Well done!  It's as simple as that!'
    wait 2s
    msend %actor% %self.name% tells you, 'If you don't want to pick up the first one, you can target a different one by adding a number and a "." before the name.'
    wait 2s
    msend %actor% %self.name% tells you, 'Try typing &2&bget 2.stick&0 and see what happens.'
  endif
elseif %actor.quest_variable[school:gear]% == 8
  if %arg.vnum% == 51902 && %arg% /= 2.
    quest variable school %actor% gear 9
    mforce %actor% get %arg%
    wait 2
    msend %actor% %self.name% tells you, 'Yes, like that.'
    msend %actor% &0  
    msend %actor% You can also pick up all of one thing by typing &6&bGET all.[object]&0.
    msend %actor% Or you can be extra greedy by typing &6&bGET ALL&0.
    msend %actor% That will pick up everything in the room.'
    wait 2s
    msend %actor% %self.name% tells you, 'Type &2&bget all&0 with nothing after it.'
  endif
elseif %actor.quest_variable[school:gear]% == 9 && %arg% == all
  quest variable school %actor% gear 10
  mforce %actor% get all
  wait 2
  msend %actor% %self.name% tells you, 'Those are some sticky fingers!'
  laugh
  wait 2s
  msend %actor% %self.name% tells you, 'Now it's possible your inventory might get full.
  msend %actor% You can only carry so many items in your inventory at once but there are a few ways to deal with that.'
  wait 2s
  msend %actor% %self.name% tells you, 'First, you can &6&b(DRO)P&0 items with the command &6&bDROP [object]&0.'
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'Drop one of those sticks by typing &2&bdrop stick&0.'
elseif %actor.quest_variable[school:gear]% == 15 && %arg% /= stick bag
  quest variable school %actor% gear 16
  mforce %actor% get stick bag
  wait 2
  cheer
  msend %actor% %get.mob_shortdesc[51902]% tells you, 'Congratulations, you mastered equipment management!
  msend %actor% Would you like to review?
  msend %actor% You can &2&bsay yes&0 or &2&bsay no&0.'
endif
return 0
~
#51912
academy_stick_drop~
1 h 100
~
if %actor.quest_variable[school:gear]% == 10
  quest variable school %actor% gear 11
  wait 2
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'Exactly.  Dropping objects leaves them for anyone else to pick up.
  osend %actor% Be careful!  Sometimes monsters can pick up things too!
  osend %actor% &0 
  osend %actor% You can permanently destroy objects with the &6&b(J)UNK&0 command.'&0
  wait 2s
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'Try junking a stick by typing &2&bjunk stick&0.'&0
elseif %actor.quest_variable[school:gear]% == 13 && %target.vnum% == 18
  quest variable school %actor% gear 14
  wait 2
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'That's how it's done.'
  wait 2s
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'To see what's inside something, use &6&b(EXA)MINE [target]&0.
  osend %actor% &6&bEXAMINE&0 will also show if something is open or closed.'
  wait 2s
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'Go ahead and &2&bexamine bag&0 and see what you find.'
endif
~
#51913
academy_instructor_command_junk~
0 c 100
junk~
if %actor.quest_variable[school:gear]% == 11 && %arg.vnum% == 51902
  quest variable school %actor% gear 12
  mforce %actor% junk %arg%
  wait 2
  msend %actor% %self.name% tells you, 'Very good.'
  wait 2s
  msend %actor% %self.name% tells you, 'You can drop and junk everything with the same keyword by typing in &6&ball.[object]&0 as well.
  msend %actor% But &1&bbe careful&0!  You might end up junking something with a surprising keyword by accident!'&0
  wait 2s
  msend %actor% %self.name% tells you, 'Another way to deal with items is to &6&b(GI)VE&0 them away.
  msend %actor% You can do that by typing &6&bGIVE [object] [person]&0.'
  wait 2s
  msend %actor% %self.name% tells you, 'Give me a stick by typing &2&bgive stick instructor&0.'
endif
return 0
~
#51914
academy_instructor_command_give~
0 j 100
51902~
if %actor.quest_variable[school:gear]% == 12
  quest variable school %actor% gear 13
  wait 2
  mjunk %object%
  msend %actor% %self.name% tells you, 'Why thank you, what a lovely gift.'
  grin
  wait 2s
  msend %actor% %self.name% tells you, 'Lastly, you can &6&b(P)UT&0 objects in containers.
  msend %actor% The command is &6&bPUT [object] [container]&0.'
  wait 2s
  msend %actor% %self.name% tells you, 'You started with a bag, but just in case here's another.'
  mload obj 18
  give bag %actor%
  wait 3s
  msend %actor% %self.name% tells you, 'Put a stick in it by typing &2&bput stick bag&0.'
endif
~
#51915
academy_instructor_speech_expl_commands~
0 d 100
look search doors scan movement~
if %actor.quest_variable[school:explore]% == 6
  switch %speech%
    case look
      msend %actor% %self.name% tells you, 'As you look around the room, you'll get four big pieces of information:'
      msend %actor% &0 
      msend %actor% &0&5&b1. The description of the room.&0
      msend %actor% &0   Tiny but crucial hints can appear in room descriptions - keep your eyes open!
      msend %actor% &0 
      msend %actor% &0&5&b2. The visible exits from the room.&0
      msend %actor% &0   FieryMUD uses six directions: north, south, east, west, up, and down.
      msend %actor% &0   If an exit is visible, it will appear after a hyphen.
      msend %actor% &0   Closed exits like doors and trapdoors will have a # sign after them.
      msend %actor% &0 
      msend %actor% &5&b3. Any visible objects.&0
      msend %actor% &0   By default, all identical objects will appear on one line with a number on the left showing how many there are.
      msend %actor% &0 
      msend %actor% &0&5&b4. All visible creatures.&0
      msend %actor% &0   All identical creatures will appear as one line.
      msend %actor% &0 
      msend %actor% You can &6&bLOOK&0 at anything to gain more information about it.
      msend %actor% That can be objects, creatures, even the directions!
      break
    case search
      msend %actor% %self.name% tells you, 'Throughout the world there are hundreds of hidden doors.
      msend %actor% There can be hints in the room description, or you can try to find them by typing &6&bLOOK [direction]&0.
      msend %actor% &0 
      msend %actor% To interact with the door, you have to &6&b(SEA)RCH&0 for it first.
      msend %actor% &0 
      msend %actor% If you do know what you're looking for, you can type &6&bSEARCH [keyword]&0 to find it automatically.
      msend %actor% You might be able to guess the keywords from what you see when you look at things or in directions!
      msend %actor% &0 
      msend %actor% If you don't know the keyword, you can just enter &6&bSEARCH&0.
      msend %actor% You'll have a random chance to find any hidden doors.
      msend %actor% &0 
      msend %actor% Be aware!  There is a small stun time after you &6&bSEARCH&0!
      msend %actor% It can be very risky to use it in dangerous areas!'
      break
    case door
      msend %actor% %self.name% tells you, 'Once you've uncovered a secret door you can interact with it like a normal door.
      msend %actor% You can use the &6&b(O)PEN&0 and &6&b(CL)OSE&0 commands to open doors or containers.
      msend %actor% Sometimes they might be locked though, and you'll need a key to unlock them.
      msend %actor% &0 
      msend %actor% If you &6&bLOOK EAST&0, you can see the curtain from this room is closed.
      msend %actor% To open it just use the &6&b(O)PEN&0 command.'&0
      break
    case scan
      msend %actor% %self.name% tells you, 'You can &6&b(SCA)N&0 to see what's in the areas around you.
      msend %actor% It's free to use and very helpful for anticipating threats.
      msend %actor% Some classes are even able to see more than one room away.
      msend %actor% There is a slight delay after giving the &6&bSCAN&0 command, so be careful.'
      break
    case movement
      msend %actor% %self.name% tells you, 'You can move in any direction there's an open exit.
      msend %actor% Just type &6&b(N)ORTH (S)OUTH (E)AST (W)EST (U)P&0 or &6&b(D)OWN&0.
      msend %actor% &0 
      msend %actor% In the lower-left corner of your screen you'll see a display that looks like this: 
      msend %actor% &3&b10h(10H) 100v(100V)&0
      msend %actor% The two numbers on the right are your &5&bMovement Points&0.
      msend %actor% The first number is your &5&bCurrent Movement Points&0.  
      msend %actor% The second number is your &5&bMaximum Movement Points&0.
      msend %actor% Your &5&bMaximum&0 will increase until level 50.
      msend %actor% &0 
      msend %actor% When you move from room to room, your Movement Points go down.
      msend %actor% The amount of Movement Points needed to move around varies by terrain.
      msend %actor% If your Movement Points reach 0 you can't move until you rest.'
  done
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'Would you like to review something else?
  msend %actor% You can &2&bsay yes&0 or &2&bsay no&0.'
endif
~
#51916
academy_instructor_speech_inv_commands~
0 d 100
inventory wear equipment light remove get drop junk give put examine~
wait 2
if %actor.quest_variable[school:gear]% == 16
  switch %speech%
    case inventory
      msend %actor% %self.name% tells you, 'You have two places you keep items:
      msend %actor% Your &6&b(I)NVENTORY&0 and your &6&b(EQ)UIPMENT&0.
      msend %actor% &0 
      msend %actor% Your &6&b(I)NVENTORY&0 is all the items you're currently carrying but not actively wearing or using.
      msend %actor% You must equip an item to receive benefits from it.'
      break
    case wear
      msend %actor% %self.name% tells you, 'There are three commands to equip items:
      msend %actor% &0&6&b(WEA)R&0, &6&b(WI)ELD&0, and &6&b(HO)LD&0.
      msend %actor% &0 
      msend %actor% &6&bWEAR&0 will equip something from your inventory.
      msend %actor% You can equip most objects by typing &6&bWEAR [object]&0.
      msend %actor% Weapons can be equipped with either &6&bWEAR&0 or &6&bWIELD&0.
      msend %actor% &0&6&bWEAR ALL&0 will equip everything in your inventory at once.
      msend %actor% &0 
      msend %actor% Some items can only be equipped with the &6&bHOLD&0 command.
      msend %actor% That includes instruments, wands, staves, magic orbs, etc.
      msend %actor% They will not be equipped with the &6&bwear all&0 command.
      msend %actor% Once you are holding them they can be activated with the &6&bUSE&0 command.'
      break
    case equipment
      msend %actor% %self.name% tells you, 'The &6&b(EQ)UIPMENT&0 command shows what gear you're using.
      msend %actor% You are gaining active benefits from these items.
      msend %actor% They will not show up in your inventory.'
      break
    case light
      msend %actor% %self.name% tells you, 'Lights are necessary to see in dark spaces or outside at night.  
      msend %actor% The &6&b(LI)GHT&0 command turns lights on and off.
      msend %actor% You can just type &6&blight torch&0 to light it up.
      msend %actor% If it's already lit, you can type &6&blight torch&0 again to extinguish it.
      msend %actor% &0 
      msend %actor% Remember, most lights have a limited duration.  
      msend %actor% It's best to turn them off when not using them.
      msend %actor% &0 
      msend %actor% Lights don't have to be equipped for you to see.
      msend %actor% They work just fine from your inventory.'
      break
    case remove
      msend %actor% %self.name% tells you, 'You can stop using items with &6&b(REM)OVE [object]&0.'
      break
    case get
      msend %actor% %self.name% tells you, 'During your adventures, you can pick up stuff using the &6&b(G)ET&0 command.'
      msend %actor% &0 
      msend %actor% %self.name% tells you, 'If there's more than one thing with the same keyword in the room, you can target a specific one by adding a number and a "." before the keyword like "2.stick" or "4.bread".
      msend %actor% &0 
      msend %actor% You can also pick up all of one thing by typing &6&bGET all.[object]&0.
      msend %actor% Or you can be extra greedy by typing &6&bGET ALL&0.
      msend %actor% &0 
      msend %actor% &6&bGET&0 is the command to take things out of containers.
      msend %actor% You can type &6&bGET [object] [container]&0 for one thing, or &6&bGET ALL [container]&0 to get everything out.'
      break
    case drop
      msend %actor% %self.name% tells you, 'You can &6&b(DRO)P&0 items in your inventory to get rid of them.
      msend %actor% Type &6&bDROP [object]&0 to drop them.
      msend %actor% You can also also drop everything with the same keywords by typing &6&bDROP all.[object]&0 or you can drop your whole inventory with &6&bDROP ALL&0.
      msend %actor% &0
      msend %actor% Dropping items leaves them for others to pick up.'
      break
    case give
      msend %actor% %self.name% tells you, 'Another way to deal with items is to &6&b(GI)VE&0 them away.
      msend %actor% You can do that by typing &6&bGIVE [object] [person]&0.'
      break
    case junk
      msend %actor% %self.name% tells you, 'The &6&b(J)UNK&0 command permanently destroys items in your inventory.
      msend %actor% You can drop and junk everything with the same keywords by typing &6&ball.[object]&0 as well.
      msend %actor% But &3&bbe careful&0!
      msend %actor% You might end up junking something with a surprising keyword by accident!'
      break
    case examine
      msend %actor% %self.name% tells you, 'To see what's inside something, use the &6&b(EXA)MINE&0.
      msend %actor% &6&bEXAMINE [target]&0 will also show if something is open or closed.
      msend %actor% You can check different bags by typing 2.bag, 3.bag, etc.'
      break
    case put
      msend %actor% %self.name% tells you, 'You can &6&b(P)UT&0 objects in containers to get them out of your inventory.
      msend %actor% The command is &6&bPUT [object] [container]&0.
      msend %actor% After that use &6&b(G)ET&0 to take the item back out.
      msend %actor% You can type &6&bGET [object] [container]&0 to get a specific thing, or you can type &6&bGET ALL [container]&0 to get everything out of the container.'
  done
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'Would you like to review anything else?
  msend %actor% You can &2&bsay yes&0 or &2&bsay no&0.'
endif
~
#51917
academy_instructor_command_look~
0 c 100
look~
if %actor.quest_variable[school:explore]% == 1 && !%arg%
  quest variable school %actor% explore 2
  mforce %actor% look
  wait 2
  msend %actor% %self.name% tells you, 'As you look around the room, you'll get four big pieces of information:'
  msend %actor% &0 
  msend %actor% &0&5&b1. The description of the room.&0
  msend %actor% &0   Tiny but crucial hints can appear in room descriptions - keep your eyes open!
  msend %actor% &0 
  msend %actor% &0&5&b2. The visible exits from the room.&0
  msend %actor% &0   FieryMUD uses six directions: north, south, east, west, up, and down.
  msend %actor% &0   If an exit is visible, it will appear after a hyphen.
  msend %actor% &0   Closed exits like doors and trapdoors will have a # sign after them.
  msend %actor% &0 
  msend %actor% &5&b3. Any visible objects.&0
  msend %actor% &0   By default, all identical objects will appear on one line with a number on the left showing how many there are.
  msend %actor% &0 
  msend %actor% &0&5&b4. All visible creatures.&0
  msend %actor% &0   All identical creatures will appear as one line.
  msend %actor% &0 
  msend %actor% You can &6&bLOOK&0 at anything to gain more information about it.
  msend %actor% That can be objects, creatures, even the directions!
  wait 2s
  msend %actor% %self.name% tells you, 'For example, type &2&blook curtain&0 to find more clues.'
elseif %actor.quest_variable[school:explore]% == 2 && %arg% == curtain
  quest variable school %actor% explore 3
  mforce %actor% look curtain
  wait 2
  msend %actor% %self.name% tells you, 'And THAT is a hidden door!'
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'Throughout the world there are hundreds of hidden doors.
  msend %actor% There can be hints in the room description, or you can try to find them by typing &6&bLOOK [direction]&0.
  msend %actor% &0 
  msend %actor% To interact with the door, you have to &6&b(SEA)RCH&0 for it first.
  msend %actor% &0 
  msend %actor% If you do know what you're looking for, you can type &6&bSEARCH [keyword]&0 to find it automatically.
  msend %actor% You might be able to guess the keywords from what you see when you look at things or in directions!
  msend %actor% &0 
  msend %actor% If you don't know the keyword, you can just enter &6&bSEARCH&0.
  msend %actor% You'll have a random chance to find any hidden doors.
  msend %actor% &0 
  msend %actor% Be aware!  There is a small stun time after you &6&bSEARCH&0!
  msend %actor% It can be very risky to use it in dangerous areas!'
  wait 2s
  msend %actor% %self.name% tells you, 'But for now, type &2&bsearch curtain&0 and take a look!'
endif
return 0
~
#51918
academy_instructor_command_search~
0 c 100
search~
switch %cmd%
  case s
    return 0
    halt
done
switch %arg%
  case c
  case cu
  case cur
  case curt
  case curta
  case curtai
  case curtain
    if %actor.quest_variable[school:explore]% == 3
      quest variable school %actor% explore 4
      mforce %actor% search curtain
      wait 2
      msend %actor% %self.name% tells you, 'Exactly, just like that!
      msend %actor% &6&bSEARCH&0 can also find hidden objects or creatures.
      msend %actor% &0 
      msend %actor% Once you've uncovered the door you can interact with it like a normal door.
      msend %actor% You can use the &6&b(O)PEN&0 and &6&b(CL)OSE&0 commands to open doors or containers.
      msend %actor% Sometimes they might be locked though, and you'll need a key to unlock them.
      msend %actor% &0 
      msend %actor% If you &6&bLOOK EAST&0, you can see the curtain is closed.
      msend %actor% To open it just use the &6&b(O)PEN&0 command.'&0
      wait 2s
      msend %actor% %self.name% tells you, 'Type &2&bopen curtain&0 to see how it works.'
    endif
    return 0
    break
  default
    if %actor.quest_variable[school:explore]% == 3
      msend %actor% %self.name% tells you, 'No no, you have to &2&bsearch curtain&0.'
    else
      return 0
    endif
done
~
#51919
academy_bag_examine~
1 m 100
~
wait 2
if %actor.quest_variable[school:gear]% == 14
  quest variable school %actor% gear 15
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'Now you can see everything inside the bag.
  osend %actor% You can check different bags by typing 2.bag, 3.bag, etc.
  osend %actor% &0 
  osend %actor% The last step is to &6&b(G)ET&0 the item back out.
  osend %actor% You can type &6&bGET [object] [container]&0 for one thing, or &6&bGET ALL [container]&0 to get everything out.'
  wait 2s
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'Take that stick back out by typing &2&bget stick bag&0.'
endif
~
#51920
academy_instructor_command_open~
0 c 100
open~
switch %cmd%
  case o
    return 0
    halt
done
switch %arg%
  case c
  case cu
  case cur
  case curt
  case curta
  case curtai
  case curtain
    if %actor.quest_variable[school:explore]% == 4
      quest variable school %actor% explore 5
      mforce %actor% open curtain
      wait 2
      msend %actor% %self.name% tells you, 'Perfect!'
      wait 2s
      msend %actor% %self.name% tells you, 'Now that the door is open, you can &6&b(SCA)N&0 to see what's in the areas around you.
      msend %actor% It's free to use and very helpful for anticipating threats.
      msend %actor% Some classes are even able to see more than one room away.
      msend %actor% There is a slight delay after giving the &6&bSCAN&0 command, so be careful.'
      wait 2s
      msend %actor% %self.name% tells you, 'Give it a go!  Type &2&bscan&0.'
    endif
done
return 0
~
#51921
academy_instructor_command_scan~
0 c 100
scan~
switch %cmd%
  case s
  case sc
    return 0
    halt
done
if %actor.quest_variable[school:explore]% == 5
  quest variable school %actor% explore 6
  mforce %actor% scan
  wait 2
  msend %actor% %self.name% tells you, 'You can move in any direction there's an open exit.
  msend %actor% Just type &6&b(N)ORTH (S)OUTH (E)AST (W)EST (U)P&0 or &6&b(D)OWN&0.
  msend %actor% &0 
  msend %actor% In the lower-left corner of your screen you'll see a display that looks like this: 
  msend %actor% &3&b10h(10H) 100v(100V)&0
  msend %actor% The two numbers on the right are your &5&bMovement Points&0.
  msend %actor% The first number is your &5&bCurrent Movement Points&0.  
  msend %actor% The second number is your &5&bMaximum Movement Points&0.
  msend %actor% Your &5&bMaximum&0 will increase until level 50.
  msend %actor% &0 
  msend %actor% When you move from room to room, your Movement Points go down.
  msend %actor% The amount of Movement Points needed to move around varies by terrain.
  msend %actor% If your Movement Points reach 0 you can't move until you rest.'
  wait 2s
  msend %actor% %self.name% tells you, 'Would you like to review exploration?
  msend %actor% You can &2&bsay yes&0 or &2&bsay no&0.'
endif
return 0
~
#51922
academy_sorting_room_postentry~
2 i 100
~
if %actor.vnum% == -1
  if %actor.quest_stage[school]% == 2
    if !%actor.quest_variable[school:prep]%
      quest variable school %actor% prep 1
      wdoor 51901 east flags abe
      wdoor 51901 east room 51902
      wdoor 51901 east name curtain
      wdoor 51901 east description The curtain conceals a hidden door to the east!
    endif
  endif
endif
~
#51923
academy_clerk_greet~
0 h 100
~
wait 2
if %actor.quest_stage[school]% == 2
  if %actor.quest_variable[school:prep]% == 1
    quest variable school %actor% prep complete
    msend %actor% %self.name% tells you, 'Aww, a fresh-faced adventurer, how sweet.'
    chuckle
    wait 2s
    msend %actor% %self.name% tells you, 'Well kid, let's get ya ready to fight!
    msend %actor% I have two quick things to introduce you to:
    msend %actor% &3&bHIT POINTS&0, and &3&b(SC)ORE&0.
    msend %actor% &0
    msend %actor% &2&bSay&0 one or the other to learn about it.
    msend %actor% You can also say &5SKIP&0 to move to the next lesson.'
  elseif %actor.quest_variable[school:score]% == 1
    msend %actor% %self.name% tells you, 'Let's pick up where we left off.'
    wait 2
    msend %actor% %self.name% tells you, '&6&bSCORE&0 is how you see all the numeric stuff about yourself:
    msend %actor% Experience, Hit and Movement Points, Stats, Saves, blah blah blah.'
    wait 8s
    msend %actor% %self.name% tells you, 'Check it out by typing &2&bscore&0 now.'
  endif
elseif %actor.quest_stage[school]% == 3
  switch %actor.class%
      case rogue
      case thief
      case assassin
      case mercenary
      case bard
        msend %actor% %self.name% tells you, 'I see you're a stealthy type.  You'll do best in lessons with Doctor Mischief.  Proceed &2&bdown&0 to their classroom.'
        break
      case sorcerer
      case cryomancer
      case pyromancer
      case necromancer
      case illusionist
        msend %actor% %self.name% tells you, 'I see you're an arcane spell caster.  You would definitely benefit from the Chair of Arcane Studies' seminar on spellcasting.  Proceed &2&bsouth&0 to his laboratory.'
        break
      case cleric
      case priest
      case diabolist
      case druid
        msend %actor% %self.name% tells you, 'I see you're a divine spell caster.  You would definitely benefit from private classes with the Professor of Divinity.  Proceed &2&beast&0 to his chapel.'
        break
      case warrior
      case paladin
      case anti-paladin
      case ranger
      case monk
      case berserker
        msend %actor% %self.name% tells you, 'I see you're a fighter type.  You'll do best learning from the Academy's Warmaster.  Proceed &2&bnorth&0 to her arena.'
        break
      default
        msend %actor% %self.name% tells you, 'I have no idea where to send you.  Ask a god for help!'
  done
endif
~
#51924
academy_clerk_command_score~
0 c 100
score~
switch %cmd%
  case s
    return 0
    halt
done
if %actor.quest_stage[school]% == 2
  if %actor.quest_variable[school:score]% == 1
    quest variable school %actor% score complete
    mforce %actor% score
    wait 2
    msend %actor% %self.name% tells you, 'Yep, just like that.  That great big bar at the
    msend %actor% &0bottom is your experience bar.  That will fill as you kill stuff.  When it
    msend %actor% &0reaches full, you'll be ready to level!'
    wait 2s
    msend %actor% %self.name% tells you, 'Are you ready to continue?
    msend %actor% You can &2&bsay yes&0 or &2&bsay no&0.'
  endif
endif
return 0
~
#51925
academy_clerk_command_toggle~
0 c 100
toggle~
if %actor.quest_variable[school:fight]% == 3 && %arg% /= autosplit
    quest variable school %actor% fight 4
    mforce %actor% toggle autosplit
    wait 2
    t %actor% Grand.
    t %actor% You're ready to continue.
    wait 1s
    eye %actor%
    wait 1s
    msend %actor% %self.name% considers your capabilities...
    wait 3s
    t %actor% Hmmmm...
    wait 2s
    switch %actor.class%
        case rogue
        case thief
        case assassin
        case mercenary
        case bard
            t %actor% I see you're a stealthy type.
            t %actor% You'll do best in lessons with Doctor Mischief.
            t %actor% Proceed down to her classroom.
            break
        case sorcerer
        case cryomancer
        case pyromancer
        case necromancer
        case illusionist
            t %actor% I see you're an arcane spell caster.
            t %actor% You would definitely benefit from
            t %actor% The Chair of Arcane Studies' seminar on spellcasting.
            t %actor% Proceed south to his laboratory.
            break
        case cleric
        case priest
        case diabolist
        case druid
            t %actor% I see you're a divine spell caster.
            t %actor% You would definitely benefit from
            t %actor% Private classes with the Professor of Divinity.
            t %actor% Proceed east to his chapel.
            break
        case warrior
        case paladin
        case anti-paladin
        case ranger
        case monk
        case berserker
        default
            t %actor% I see you're a fighter type.
            t %actor% You'll do best learning from the Academy's Warmaster.
            t %actor% Proceed north to her arena.
    done
else
    return 0
endif
~
#51926
academy_rogue_greet~
0 h 100
~
wait 2
set stage %actor.quest_stage[school]%
if %stage% == 3 || %stage% == 4
  if %stage% == 3
    switch %actor.quest_variable[school:fight]%
      case 1
        msend %actor% %self.name% tells you, 'First, I want to introduce you to &6&b(HID)E&0.
        msend %actor% If you're hidden, it makes it much harder for enemies to see you.
        msend %actor% If enemies can't see you, they can't attack you.
        msend %actor% It can get you out of some nasty scrapes.
        msend %actor% But if you do anything to draw attention to yourself, you'll stop hiding.
        msend %actor% &0 
        msend %actor% You automatically &6&bSNEAK&0 if you move while hidden.
        msend %actor% &6&bSNEAK&0 helps you stay hidden as you walk.
        msend %actor% &0 
        msend %actor% Both of these skills rely on the &6&bHIDE&0 command.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Go ahead and give it a try!  Type &2&bhide&0.'
        break
      case 2
        msend %actor% %self.name% tells you, 'Killing creatures is how you gain experience.
        msend %actor% Gaining experience is how you advance in level.
        msend %actor% Player killing is generally not allowed in FieryMUD.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Before you leap out of the shadows, take a moment to size up this creature.
        msend %actor% Use the &6&b(CON)SIDER&0 command to see what your chances are against it.
        msend %actor% Bare in mind FieryMUD is made for groups of 4-8, so the results of &6&bCONSIDER&0 aren't perfect.'
        wait 7s
        if %get.mob_count[51900]% == 0
          mload mob 51900
          mecho %self.name% summons a horrible little monster!
        endif
        msend %actor% %self.name% tells you, 'Type &2&bconsider monster&0 and see what happens.'
        break
      case last
        msend %actor% %self.name% tells you, 'To attack a creature use the &6&b(KIL)L&0 command.
        msend %actor% But as a rogue, you have a special opening attack!'
        wait 3s
        msend %actor% %self.name% tells you, 'The &6&b(B)ACKSTAB&0 command has a chance to do extreme damage.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Type &2&bbackstab monster&0 to start fighting.
        msend %actor% &0  
        msend %actor% You can always use the &6&b(FL)EE&0 command to try to run away.
        msend %actor% It's a good idea to &6&bFLEE&0 if you start to run low on hit points.
        msend %actor% But if you try to flee and fail, you'll be stunned for a little bit.
        msend %actor% So don't wait until the last second to run!'
        wait 3s
        if %get.mob_count[51900]% == 0
          mload mob 51900
          mecho %self.name% summons a horrible little monster!
        endif
        wait 1s
        msend %actor% %self.name% tells you, 'When you've killed the monster, &2&bsay loot&0.
        msend %actor% We'll continue from there.'
        break
      default
        msend %actor% You sense the presence of someone behind you.
        wait 2s
        msend %actor% %self.name% pops out of the shadows!
        msend %actor% %self.name% shouts, 'Boo!'
        grin %actor%
        wait 2s
        msend %actor% %self.name% tells you, 'Greetings %actor.name%.  They call me Doctor Mischief.  I am the Rogue Master of Ethilien Academy.  Pleasure to meet you.'
        bow %actor%
        wait 3s
        msend %actor% %self.name% tells you, 'I understand you're here to learn the basics of stealth &3&bCOMBAT&0.'
        nod
        msend %actor% %self.name% tells you, 'People with our skills do best in the shadows.  Start by typing &2&bskill&0 to see what you can do.'
    done
    msend %actor% &0 
    msend %actor% %self.name% tells you, 'You can also say &5SKIP&0 at any time to talk about &3&bLOOT&0 and &3&bTOGGLES&0 instead.'
  elseif %stage% == 4
    switch %actor.quest_variable[school:loot]%
      case 1
        msend %actor% %self.name% tells you, 'When something dies, it usually leaves behind a &3&bcorpse&0.'
        wait 1s
        poke %actor%
        msend %actor% %self.name% tells you, 'That goes for you too kid.'
        wait 3s
        msend %actor% %self.name% tells you, 'A corpse is like a container.
        msend %actor% You can &6&bEXAMINE corpse&0 it to see what it has on it.  You can
        msend %actor% &0&6&bGET [object] corpse&0 to take something specific, or you can &6&bGET ALL corpse&0 to take everything on it.
        msend %actor% Corpses keep their names as keywords so you can use those too.
        msend %actor% &0 
        msend %actor% You can't pick up a corpse, but you can &6&bDRAG&0 them from room to room.
        msend %actor% You need &6&bCONSENT&0 to drag a player corpse though.
        msend %actor% &0
        msend %actor% If YOU die, you have to trudge all the way back to the room you died in, then get everything from your body, like &6&bGET ALL corpse&0, to get your stuff.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'For now just type &2&bget all corpse&0 and we'll keep moving.'
        break
      case 2
        msend %actor% %self.name% tells you, 'The other thing I need to show you is the &6&bTOGGLE&0 command.
        msend %actor% Typing &6&bTOGGLE&0 alone will show you everything you can set.
        msend %actor% &6&bAUTOLOOT&0 picks up everything from a corpse instantly.
        msend %actor% &6&bAUTOTREAS&0 picks up only "treasure" like coins and gems.
        msend %actor% Each toggle has a &6&bHELP&0 file with more information.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'For now, let's get you ready to gather and share loot.
        msend %actor% Type &2&btoggle autoloot&0 to loot future kills!'
        break
      case 3
        msend %actor% %self.name% tells you, 'When playing with others it's considered polite to share the wealth.
        msend %actor% You can share money with the &6&bSPLIT&0 command.
        msend %actor% With &6&bTOGGLE AUTOSPLIT&0 on the game will automatically do that when you pick up money.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Type &2&btoggle autosplit&0 to set that.'
        break
      case 4
        msend %actor% %self.name% tells you, 'The last combat tip is checking your &6&b(TRO)PHY&0 list.
        msend %actor% &6&bTROPHY&0 shows a record of the last 24 creatures you've killed and how many times you've killed them.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Type &2&btrophy&0 and see your record.'
        break
      default
        msend %actor% %self.name% tells you, '&2&bSay loot&0 to learn about &3&bLOOT&0 and &3&bTOGGLES.'
    done
    msend %actor% &0 
    msend %actor% %self.name% tells you, 'You can also say &5SKIP&0 to move on to the next teacher.'
  endif
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'You can also say &5EXIT&0 at any time to leave the Academy.'
endif
~
#51927
academy_rogue_command_skill~
0 c 100
skills~
switch %cmd%
  case s
    return 0
    halt
done
if %actor.quest_stage[school]% == 3 && !%actor.quest_variable[school:fight]%
  quest variable school %actor% fight 1
  mforce %actor% skill
  wait 2
  msend %actor% %self.name% tells you, 'Unlike many MUDs you will not need to train or practice your skills with a trainer or Guild Master.
  msend %actor% Skills will improve gradually as you use them.
  msend %actor% The meter next to the skill name shows you two things:
  msend %actor% &6&b1. How close you are to maximum skill for your level, indicated by the "=" sign&0
  msend %actor% &6&b2. How close you are to overall maximum skill for your class and race, indicated by the "*" sign.&0'
  wait 3s
  msend %actor% %self.name% tells you, 'First, I want to introduce you to &6&b(HID)E&0.
  msend %actor% If you're hidden, it makes it much harder for enemies to see you.
  msend %actor% If enemies can't see you, they can't attack you.
  msend %actor% It can get you out of some nasty scrapes.
  msend %actor% But if you do anything to draw attention to yourself, you'll stop hiding.
  msend %actor% &0 
  msend %actor% You automatically &6&bSNEAK&0 if you move while hidden.
  msend %actor% &6&bSNEAK&0 helps you stay hidden as you walk.
  msend %actor% &0 
  msend %actor% Both of these skills rely on the &6&bHIDE&0 command.'
  wait 3s
  msend %actor% %self.name% tells you, 'Go ahead and give it a try!  Type &2&bhide&0.'
endif
return 0
~
#51928
academy_rogue_command_hide~
0 c 100
hide~
switch %cmd%
  case h
  case hi
    return 0
    halt
done
if %actor.quest_variable[school:fight]% == 1
  quest variable school %actor% fight 2
  mforce %actor% hide
  wait 2
  msend %actor% %self.name% tells you, 'Excellent job!'
  wait 2s
  mload mob 51900
  mecho %self.name% summons a horrible little monster!
  wait 2s
  msend %actor% %self.name% tells you, 'Killing creatures is how you gain experience.
  msend %actor% Gaining experience is how you advance in level.
  msend %actor% Player killing is generally not allowed in FieryMUD.'
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'Before you leap out of the shadows, take a moment to size up this creature.
  msend %actor% Use the &6&b(CO)NSIDER&0 command to see what your chances are against it.
  msend %actor% Bare in mind FieryMUD is made for groups of 4-8, so the results of &6&bCONSIDER&0 aren't perfect.'
  wait 3s
  msend %actor% %self.name% tells you, 'Type &2&bconsider monster&0 and see what happens.'
endif
return 0
~
#51929
academy_rogue_command_consider~
0 c 100
consider~
switch %cmd%
  case c
    return 0
    halt
done
if %actor.quest_variable[school:fight]% == 2
  quest variable school %actor% fight last
  mforce %actor% consider %arg%
  wait 2
  msend %actor% %self.name% tells you, 'To attack a creature use the &6&b(KIL)L&0 command.
  msend %actor% But as a rogue, you have a special opening attack!'
  wait 3s
  msend %actor% %self.name% tells you, 'The &6&b(B)ACKSTAB&0 command has a chance to do extreme damage.'
  msend %actor% 
  msend %actor% %self.name% tells you, 'Type &2&bbackstab monster&0 to start fighting.
  msend %actor%   
  msend %actor% You can always use the &6&b(FL)EE&0 command to try to run away.
  msend %actor% It's a good idea to &6&bFLEE&0 if you start to run low on hit points.
  msend %actor% But if you try to flee and fail, you'll be stunned for a little bit.
  msend %actor% So don't wait until the last second to run!'&0
  wait 3s
  msend %actor% %self.name% tells you, 'When you've killed the monster, I'll teach you about &3&bLOOT&0 and &3&bTOGGLES&0.'
  msend %actor% &2&bSay loot&0 when you're ready to continue.'
endif
return 0
~
#51930
academy_warrior_greet~
0 h 100
~
wait 2
set stage %actor.quest_stage[school]%
if %stage% == 3 || %stage% == 4
  if %stage% == 3
    switch %actor.quest_variable[school:fight]%
      case 1
        msend %actor% %self.name% tells you, 'You need to take your combat &6&b(SK)ILLS&0 into consideration as well.
        msend %actor% Warriors have a wide variety of special &6&bSKILLS&0.
        msend %actor% Type &2&bskill&0 to see what they are.'
        break
      case 2
        msend %actor% One of your most basic skills is &6&bKICK&0.  
        msend %actor% It's an extra attack that delivers some bonus damage.
        msend %actor% There is a short stun after you &6&bKICK&0.
        msend %actor% So be very, very careful about spamming the skill.
        msend %actor% &0 
        msend %actor% You probably won't land a kick for many levels, but keep trying.
        msend %actor% Practice makes perfect.'
        msend %actor% &0 
        wait 3s
        if %get.mob_count[51900]% == 0
          mload mob 51900
          mecho %self.name% summons a horrible little monster!
          wait 1s
        endif
        msend %actor% %self.name% tells you, 'Give it a try!
        msend %actor% Type &2&bkick monster&0.'
        break
      case 3
        if !%actor.inventory[1150]% && !%actor.wearing[1150]%
          msend %actor% %self.name% takes a wooden shield off a rack.
          mload obj 1150
          give shield %actor%
          msend %actor% &0 
          msend %actor% %self.name% tells you, 'Equip that with &2&bwear shield&0.'
          break
        elseif %actor.inventory[1150]%
          msend %actor% %self.name% tells you, 'Equip that %get.obj_noadesc[1150]% I gave you with &2&bwear shield&0.'
          break
        elseif %actor.wearing[1150]%
          quest variable school %actor% fight 4
        endif
      case 4
        msend %actor% %self.name% tells you, '&6&bBASH&0 deals some damage, but more importantly it &1&bknocks your opponent down&0.
        msend %actor% That &1&bprevents your opponent from attacking you&0 and &1&bstops spellcasters from casting spells&0.
        msend %actor% &0  
        msend %actor% Bashing is a complex maneuver.
        msend %actor% First, you must be wearing a shield.
        msend %actor% Second, you have to be similar sizes.
        msend %actor% Anything too big and you'll bounce off.
        msend %actor% Anything too small and you'll miss.
        msend %actor% &0 
        msend %actor% There's another big risk to using &6&bBASH&0.
        msend %actor% If you miss, which is very likely when you're starting out, &1&byou will be unable to fight back until you stand up.&0
        msend %actor% &0 
        msend %actor% Like most combat skills, there is a brief stun after using it.
        msend %actor% So spamming it can trap you in a very deadly situation.'
        wait 3s
        if %get.mob_count[51900]% == 0
          mload mob 51900
          mecho %self.name% summons a horrible little monster!
          wait 1s
        endif
        msend %actor% %self.name% tells you, 'I want you to practice bashing now.
        msend %actor% &0Type &2&bbash monster&0.  Don't worry, I'm here to protect you.'&0
        break
      case last
        if %get.mob_count[51900]% == 0
          mload mob 51900
          mecho %self.name% summons a horrible little monster!
          wait 1s
        endif
        msend %actor% %self.name% tells you, 'To attack a creature use the &6&b(KIL)L&0 command.
        msend %actor% Type &2&bkill monster&0 to start fighting.
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'When you've killed the monster, I'll teach you about &3&bLOOT&0 and &3&bTOGGLES&0.'
        msend %actor% &2&bSay loot&0 when you're ready to continue.'
        break
      default
        msend %actor% %self.name% turns to greet you.
        wait 2s
        salute %actor%
        msend %actor% %self.name% tells you, 'Well met soldier!'
        wait 2s
        msend %actor% %self.name% tells you, 'Greetings %actor.name%.  I am the Warmaster of Ethilien Academy.  Welcome to our ranks.'
        wait 3s
        msend %actor% %self.name% tells you, 'I understand you're here to learn &3&bCOMBAT&0 basics.'
        wait 2s
        msend %actor% %self.name% tells you, 'Well then grunt, fall in and listen up!'
        wait 3s
        mload mob 51900
        mecho %self.name% summons a horrible little monster!
        wait 3s
        msend %actor% %self.name% tells you, 'Killing creatures like this is how you gain experience.
        msend %actor% You gain experience to advance in level.  
        msend %actor% Player killing is generally not allowed in FieryMUD.'
        msend %actor% 
        msend %actor% Before you charge in, take a moment to size up your opponent.  
        msend %actor% Use the &6&b(CON)SIDER&0 command to see what your chances are against it.
        msend %actor% Bare in mind FieryMUD is made for groups of 4-8 players, so the results of &6&bCONSIDER&0 aren't perfect.'
        wait 3s
        msend %actor% %self.name% tells you, 'Type &2&bconsider monster&0 and see what happens.'
    done
    msend %actor% &0 
    msend %actor% %self.name% tells you, 'You can also say &5SKIP&0 at any time to talk about &3&bLOOT&0 and &3&bTOGGLES&0 instead.'
  elseif %stage% == 4
    switch %actor.quest_variable[school:loot]%
      case 1
        msend %actor% %self.name% tells you, 'When something dies, it usually leaves behind a &3&bcorpse&0.'
        wait 1s
        poke %actor%
        msend %actor% %self.name% tells you, 'That goes for you too kid.'
        wait 3s
        msend %actor% %self.name% tells you, 'A corpse is like a container.
        msend %actor% You can &6&bEXAMINE corpse&0 it to see what it has on it.  You can
        msend %actor% &0&6&bGET [object] corpse&0 to take something specific, or you can &6&bGET ALL corpse&0 to take everything on it.
        msend %actor% Corpses keep their names as keywords so you can use those too.
        msend %actor% &0 
        msend %actor% You can't pick up a corpse, but you can &6&bDRAG&0 them from room to room.
        msend %actor% You need &6&bCONSENT&0 to drag a player corpse though.
        msend %actor% &0
        msend %actor% If YOU die, you have to trudge all the way back to the room you died in, then get everything from your body, like &6&bGET ALL corpse&0, to get your stuff.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'For now just type &2&bget all corpse&0 and we'll keep moving.'
        break
      case 2
        msend %actor% %self.name% tells you, 'The other thing I need to show you is the &6&bTOGGLE&0 command.
        msend %actor% Typing &6&bTOGGLE&0 alone will show you everything you can set.
        msend %actor% &6&bAUTOLOOT&0 picks up everything from a corpse instantly.
        msend %actor% &6&bAUTOTREAS&0 picks up only "treasure" like coins and gems.
        msend %actor% Each toggle has a &6&bHELP&0 file with more information.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'For now, let's get you ready to gather and share loot.
        msend %actor% Type &2&btoggle autoloot&0 to loot future kills!'
        break
      case 3
        msend %actor% %self.name% tells you, 'When playing with others it's considered polite to share the wealth.
        msend %actor% You can share money with the &6&bSPLIT&0 command.
        msend %actor% With &6&bTOGGLE AUTOSPLIT&0 on the game will automatically do that when you pick up money.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Type &2&btoggle autosplit&0 to set that.'
        break
      case 4
        msend %actor% %self.name% tells you, 'The last combat tip is checking your &6&b(TRO)PHY&0 list.
        msend %actor% &6&bTROPHY&0 shows a record of the last 24 creatures you've killed and how many times you've killed them.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Type &2&btrophy&0 and see your record.'
        break
      default
        msend %actor% %self.name% tells you, '&2&bSay loot&0 to learn about &3&bLOOT&0 and &3&bTOGGLES.'
    done
    msend %actor% &0 
    msend %actor% %self.name% tells you, 'You can also say &5SKIP&0 to move on to the next teacher.'
  endif
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'You can also say &5EXIT&0 at any time to leave the Academy.'
endif
~
#51931
academy_warrior_command_consider~
0 c 100
consider~
switch %cmd%
  case c
    return 0
    halt
done
if %actor.quest_stage[school]% == 3 && !%actor.quest_variable[school:fight]%
  quest variable school %actor% fight 1
  mforce %actor% consider %arg%
  wait 2
  msend %actor% %self.name% tells you, 'Very good.'
  wait 2s
  msend %actor% %self.name% tells you, 'You need to take your combat &6&b(SK)ILLS&0 into consideration as well.
  msend %actor% Warriors have a wide variety of special &6&bSKILLS&0.
  msend %actor% Type &2&bskill&0 to see what they are.'
endif
return 0
~
#51932
academy_warrior_command_skill~
0 c 100
skills~
switch %cmd%
  case s
    return 0
    halt
done
if %actor.quest_variable[school:fight]% == 1
  quest variable school %actor% fight 2
  mforce %actor% skill
  wait 2
  msend %actor% %self.name% tells you, 'Unlike many MUDs you will not need to train or practice your skills with a trainer or Guild Master.
  msend %actor% Skills will improve gradually as you use them.
  msend %actor% The meter next to the skill name shows you two things:
  msend %actor% &6&b1. How close you are to maximum skill for your level, indicated by the "=" sign&0
  msend %actor% &6&b2. How close you are to overall maximum skill for your class and race, indicated by the "*" sign.&0
  msend %actor% &0 
  msend %actor% One of your most basic skills is &6&bKICK&0.  
  msend %actor% It's an extra attack that delivers some bonus damage.
  msend %actor% There is a short stun after you &6&bKICK&0.
  msend %actor% So be very, very careful about spamming the skill.
  msend %actor% &0 
  msend %actor% You probably won't land a kick for many levels, but keep trying.
  msend %actor% Practice makes perfect.'
  wait 3s
  msend %actor% %self.name% tells you, 'Give it a try!
  msend %actor% Type &2&bkick monster&0.'
endif
return 0
~
#51933
academy_warrior_command_kick~
0 c 100
kick~
if %actor.quest_variable[school:fight]% == 2
  quest variable school %actor% fight 3
  mforce %actor% kick %arg%
  wait 2
  mpurge monster
  mecho %self.name% rushes in to break up the fight!
  wait 1
  mload mob 51900
  wait 3
  msend %actor% %self.name% tells you, 'Excellent job!
  msend %actor% You're ready to try out your other important skill, &6&bBASH&0.'
  wait 2s
  msend %actor% %self.name% takes a wooden shield off a rack.
  mload obj 1150
  give shield %actor%
  wait 2s
  msend %actor% %self.name% tells you, 'Equip that with &2&bwear shield&0.'
endif
return 0
~
#51934
academy_shield_wear~
1 j 100
~
if %actor.quest_variable[school:fight]% == 3
  quest variable school %actor% fight 4
  wait 2
  osend %actor% %get.mob_shortdesc[51904]% tells you, '&6&b(BAS)H&0 deals some damage, but more importantly it &1&bknocks your opponent down&0.
  osend %actor% That &1&bprevents your opponent from attacking you&0 and &1&bstops spellcasters from casting spells&0.
  osend %actor% &0  
  osend %actor% Bashing is a complex maneuver.
  osend %actor% First, you must be wearing a shield.
  osend %actor% Second, you have to be similar sizes.
  osend %actor% Anything too big and you'll bounce off.
  osend %actor% Anything too small and you'll miss.
  osend %actor% &0 
  osend %actor% There's another big risk to using &6&bBASH&0.
  osend %actor% If you miss, which is very likely when you're starting out, &1&byou will be unable to fight back until you stand up.&0
  osend %actor% &0 
  osend %actor% Like most combat skills, there is a brief stun after using it.
  osend %actor% So spamming it can trap you in a very deadly situation.'
  wait 2s
  if %get.mob_count[51900]% == 0
    oforce warmaster mload mob 51900
    oecho %get.mob_shortdesc[51904]% summons a horrible little monster!
    wait 1s
  endif
  osend %actor% %get.mob_shortdesc[51904]% tells you, 'I want you to practice bashing now.
  osend %actor% Type &2&bbash monster&0.  Don't worry, I'm here to protect you.'&0
endif
~
#51935
academy_warrior_command_bash~
0 c 100
bash~
switch %cmd%
  case b
  case ba
    return 0
    halt
done
if %actor.quest_variable[school:fight]% == 4
  quest variable school %actor% fight last
  mforce %actor% bash %arg%
  wait 2
  mpurge monster
  wait 2
  mecho %self.name% breaks up the fight.
  msend %actor% %self.name% tells you, 'Good attempt!'
  mload mob 51900
  wait 2s
  msend %actor% %self.name% tells you, 'To attack a creature use the &6&b(KIL)L&0 command.
  msend %actor% &0 
  msend %actor% You have to be standing to fight though and you probably fell down after trying to bash.
  msend %actor% Type &6&b(ST)AND&0 to get back up.
  msend %actor% Then type &2&bkill monster&0 to start fighting.
  point monster
  msend %actor% &0 
  msend %actor% You can always use the &6&b(FL)EE&0 command to try to run away.
  msend %actor% It's a good idea to &6&bFLEE&0 if you start to run low on hit points.
  msend %actor% If you try to flee and fail, you'll be stunned for a little bit.
  msend %actor% So don't wait until the last second to run!
  msend %actor% &0 
  msend %actor% Feel free to use any of the skills you've learned to kill this monster.
  msend %actor% Be careful not to get locked by skill stun though!'
  wait 2s
  msend %actor% %self.name% tells you, 'When you've killed the monster, I'll teach you about &3&bLOOT&0 and &3&bTOGGLES&0.'
  msend %actor% &2&bSay loot&0 when you're ready to continue.'
endif
return 0
~
#51936
academy_cleric_greet~
0 h 100
~
wait 2
set stage %actor.quest_stage[school]%
if %stage% == 3 || %stage% == 4
  if %stage% == 3
    switch %actor.quest_variable[school:fight]%
      case 1
        msend %actor% %self.name% tells you, '&6&bSPELL&0 will show you all the spells you currently know.
        msend %actor% As a cleric, you know all the spells on the list.
        msend %actor% You don't need to write them down in a book to know them.'
        wait 2s
        msend %actor% %self.name% tells you, 'Check your spell list now by typing &2&bspell&0.'
        break
      case 2
        msend %actor% The syntax to cast is &6&b(c)ast '[spell]' [target]&0.
        msend %actor% &0 
        msend %actor% FieryMUD will try to match &6&babbreviations of spell names and targets.&0
        msend %actor% If a spell name has more than one word, you &6&bmust&0 put single quotation marks &6&b' '&0 around the spell name.
        msend %actor% &0 
        msend %actor% Spellcasting is not instaneous either.
        msend %actor% Each spell has a base length to cast.
        msend %actor% The &6&bQUICK CHANT&0 skill will help reduce casting time.'
        wait 3s
        msend %actor% %self.name% tells you, '&6&bCAST&0 the Cure Light spell, your most basic healing spell, on me.
        msend %actor% &0Type &2&bcast 'cure light' professor&0.'
        break
      case 3
        msend %actor% You can use the &6&b(STU)DY&0 command to see all the information about your spell slots, including:
        msend %actor%   &5&b1. How many spell slots you have
        msend %actor%   2. Which spell slots you've used
        msend %actor%   3. How long each slot will take to recover&0
        msend %actor% &0
        msend %actor% Check your current recovery status by typing &2&bstudy&0 now.'
        break
      case 4
        msend %actor% %self.name% tells you, 'Spell slots will recover significantly faster if you &6&b(MED)ITATE&0.
        msend %actor% I'll walk you through that process now.
        msend %actor% First, get comfortable.  
        msend %actor% Type &2&brest&0 to take a seat and settle down.'
        break
      case 5
        msend %actor% %self.name% tells you, 'You should &6&b(MED)ITATE&0 to get into the proper state of mind.
        msend %actor% As it goes up, the &6&bMEDITATE&0 skill increases your &6&bFOCUS&0 score when you &6&bMEDITATE&0.
        msend %actor% You can &6&bMEDITATE&0 as long as you're not in combat.'
        wait 2s
        msend %actor% %self.name% tells you, 'Type &2&bmeditate&0 to start.'
        break
      case 6
        quest variable school %actor% fight 7
      case 7
        mload mob 51900
        mecho %self.name% summons a horrible little monster!
        wait 2s
        msend %actor% %self.name% tells you, 'Killing creatures like this is how you gain experience.
        msend %actor% Gaining experience is how you go up in level.
        msend %actor% Player killing is generally not allowed in FieryMUD.
        msend %actor% This includes casting offensive spells at other players.
        msend %actor% &0 
        msend %actor% &0Before you strike, take a moment to size up your opponent.
        msend %actor% Use the &6&b(CO)NSIDER&0 command to see what your chances are.
        msend %actor% Keep in mind FieryMUD is made for groups of 4-8, so the results of &6&bCONSIDER&0 aren't perfect
        msend %actor% Also keep in mind most of a cleric's strength is support.  
        msend %actor% It will be harder for you to kill creatures alone.'
        msend %actor% &0
        msend %actor% %self.name% tells you, 'Type &2&bconsider monster&0 for see chances.'
        break
      case last
        if %get.mob_count[51900]% == 0
          mload mob 51900
          mecho %self.name% summons a horrible little monster!
          wait 1s
        endif
        msend %actor% %self.name% tells you, 'To attack a creature use the &6&b(KIL)L&0 command.
        msend %actor% Type &2&bkill monster&0 to start fighting.
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'When you've killed the monster, I'll teach you about &3&bLOOT&0 and &3&bTOGGLES&0.'
        msend %actor% &2&bSay loot&0 when you're ready to continue.'
        break
      default
        msend %actor% %self.name% turns to greet you.
        wait 1s
        bow %actor%
        msend %actor% %self.name% tells you, 'Peace be upon you.'
        wait 1s
        msend %actor% %self.name% tells you, 'Greetings %actor.name%.  I am Ethilien Academy's Professor of Divinity.  Welcome to our chapel.'
        wait 2s
        msend %actor% %self.name% tells you, 'I understand you're here to learn the basics of spiritual &3&bCOMBAT&0.'
        wait 2s
        msend %actor% %self.name% tells you, 'Let's start by looking at your &6&b(SK)ILLS&0.
        msend %actor% Type &2&bskill&0 to see what you can do.'
    done
    msend %actor% &0 
    msend %actor% %self.name% tells you, 'You can also say &5SKIP&0 at any time to talk about &3&bLOOT&0 and &3&bTOGGLES&0 instead.'
  elseif %stage% == 4
    switch %actor.quest_variable[school:loot]%
      case 1
        msend %actor% %self.name% tells you, 'When something dies, it usually leaves behind a &3&bcorpse&0.'
        wait 1s
        poke %actor%
        msend %actor% %self.name% tells you, 'That goes for you too kid.'
        wait 3s
        msend %actor% %self.name% tells you, 'A corpse is like a container.
        msend %actor% You can &6&bEXAMINE corpse&0 it to see what it has on it.  You can
        msend %actor% &0&6&bGET [object] corpse&0 to take something specific, or you can &6&bGET ALL corpse&0 to take everything on it.
        msend %actor% Corpses keep their names as keywords so you can use those too.
        msend %actor% &0 
        msend %actor% You can't pick up a corpse, but you can &6&bDRAG&0 them from room to room.
        msend %actor% You need &6&bCONSENT&0 to drag a player corpse though.
        msend %actor% &0
        msend %actor% If YOU die, you have to trudge all the way back to the room you died in, then get everything from your body, like &6&bGET ALL corpse&0, to get your stuff.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'For now just type &2&bget all corpse&0 and we'll keep moving.'
        break
      case 2
        msend %actor% %self.name% tells you, 'The other thing I need to show you is the &6&bTOGGLE&0 command.
        msend %actor% Typing &6&bTOGGLE&0 alone will show you everything you can set.
        msend %actor% &6&bAUTOLOOT&0 picks up everything from a corpse instantly.
        msend %actor% &6&bAUTOTREAS&0 picks up only "treasure" like coins and gems.
        msend %actor% Each toggle has a &6&bHELP&0 file with more information.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'For now, let's get you ready to gather and share loot.
        msend %actor% Type &2&btoggle autoloot&0 to loot future kills!'
        break
      case 3
        msend %actor% %self.name% tells you, 'When playing with others it's considered polite to share the wealth.
        msend %actor% You can share money with the &6&bSPLIT&0 command.
        msend %actor% With &6&bTOGGLE AUTOSPLIT&0 on the game will automatically do that when you pick up money.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Type &2&btoggle autosplit&0 to set that.'
        break
      case 4
        msend %actor% %self.name% tells you, 'The last combat tip is checking your &6&b(TRO)PHY&0 list.
        msend %actor% &6&bTROPHY&0 shows a record of the last 24 creatures you've killed and how many times you've killed them.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Type &2&btrophy&0 and see your record.'
        break
      default
        msend %actor% %self.name% tells you, '&2&bSay loot&0 to learn about &3&bLOOT&0 and &3&bTOGGLES.'
    done
    msend %actor% &0 
    msend %actor% %self.name% tells you, 'You can also say &5SKIP&0 to move on to the next teacher.'
  endif
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'You can also say &5EXIT&0 at any time to leave the Academy.'
endif
~
#51937
academy_cleric_command_skill~
0 c 100
skills~
switch %cmd%
  case s
    return 0
    halt
done
if %actor.quest_stage[school]% == 3 && !%actor.quest_variable[school:fight]%
  quest variable school %actor% fight 1
  mforce %actor% skill
  wait 2
  msend %actor% %self.name% tells you, 'Unlike many MUDs you will not need to train or practice your skills with a trainer or Guild Master.
  msend %actor% Skills will improve gradually as you use them.
  msend %actor% The meter next to the skill name shows you two things:
  msend %actor% &6&b1. How close you are to maximum skill for your level, indicated by the "=" sign&0
  msend %actor% &6&b2. How close you are to overall maximum skill for your class and race, indicated by the "*" sign.&0
  msend %actor% &0 
  msend %actor% Your most important skills are the &6&bspheres&0.  
  msend %actor% These are a measure of your proficieny in a type of magic.
  msend %actor% They increase whenever you &6&bCAST&0 a spell.
  msend %actor% The spheres are not spells or attacks themselves.'
  wait 3s
  msend %actor% %self.name% tells you, 'There are four steps to using divine magic:'
  msend %actor% 1. &6&b(R)EST&0
  msend %actor% 2. &6&b(MED)ITATE&0
  msend %actor% 3. &6&b(PR)AY&0
  msend %actor% 4. &6&b(C)AST&0&0
  wait 2s
  msend %actor% %self.name% tells you, 'I'll walk you through that process now.
  msend %actor% First, get comfortable.  
  msend %actor% Type &2&brest&0 to take a seat and settle down.'
endif
return 0
~
#51938
academy_cleric_command_spell~
0 c 100
spells~
switch %cmd%
  case s
  case sp
    return 0
    halt
done
if %actor.quest_variable[school:fight]% == 1
  quest variable school %actor% fight 2
  mforce %actor% spell
  wait 2
  msend %actor% %self.name% tells you, 'Spells are divided up by Circle.
  msend %actor% For now, you only have access to Circle 1 spells.
  msend %actor% You gain access to a new Circle every 8 levels.
  msend %actor% You can find out what a spell does by checking the &6&bHELP&0 file.
  msend %actor% &0 
  msend %actor% Unlike many other MUDs, FieryMUD does not use a mana system.
  msend %actor% You can cast any spell from your list as long as you have available &6&bSPELL SLOTS&0 of the same level or higher.
  msend %actor% &0
  msend %actor% At level 1, you have only one Circle 1 spell slot.
  msend %actor% You can use that spell slot to &6&bCAST&0 any Circle 1 spell.
  msend %actor% You gain more spells slots as you increase in level.
  msend %actor% &0 
  msend %actor% When you &6&bCAST&0 a spell, if you don't have any spell slots of the spell's Circle, you will automatically use a spell slot of the next highest Circle, if any.
  msend %actor% When you run out of higher Circle spell slots, the spell will fail to cast.
  msend %actor% If you use a higher Circle spell slot, there is no additional benefit or bonus to the spell cast.
  msend %actor% You cannot stop spells from using higher Circle slots or specify what Circle slot to consume.
  msend %actor% So be careful with your spell management!
  msend %actor% &0 
  msend %actor% The syntax to cast is &6&b(c)ast '[spell]' [target]&0.
  msend %actor% &0 
  msend %actor% FieryMUD will try to match &6&babbreviations of spell names and targets.&0
  msend %actor% If a spell name has more than one word, you &6&bmust&0 put single quotation marks &6&b' '&0 around the spell name.
  msend %actor% &0 
  msend %actor% Spellcasting is not instaneous either.
  msend %actor% Each spell has a base length to cast.
  msend %actor% The &6&bQUICK CHANT&0 skill will help reduce casting time.'
  wait 3s
  msend %actor% %self.name% tells you, '&6&bCAST&0 the Cure Light spell, your most basic healing spell, on me.
  msend %actor% &0Type &2&bcast 'cure light' professor&0.'
endif
return 0
~
#51939
academy_cleric_cast_cure_light~
0 p 100
~
if %actor.quest_variable[school:fight]% == 2 && %spell% == cure light
  quest variable school %actor% fight 3
  wait 2
  msend %actor% %self.name% tells you, 'You're a natural healer!'
  wait 2s
  msend %actor% %self.name% tells you, 'Once you cast a spell, the spell slot it used will automatically begin to &6&bRECOVER&0.
  msend %actor% The amount of time a spell slot takes to recover depends on the Circle of spell slot, your &6&bFOCUS&0 score, and whether or not you &6&bMEDITATE&0.
  msend %actor% Spell slots recover &3&bin the order they were used&0.
  msend %actor% &0  
  msend %actor% &6&bFOCUS&0 is a bonus gained from equipment and your Intelligence and Wisdom stats.
  msend %actor% You can see your &6&bFOCUS&0 score using the &6&bSCORE&0 command.
  msend %actor% &0
  msend %actor% You can use the &6&b(STU)DY&0 command to see all the information about your spell slots, including:
  msend %actor%   &5&b1. How many spell slots you have
  msend %actor%   2. Which spell slots you've used
  msend %actor%   3. How long each slot will take to recover&0
  msend %actor% &0
  msend %actor% Check your current recovery status by typing &2&bstudy&0 now.'
endif
return 0
~
#51940
academy_cleric_command_study~
0 c 100
study~
switch %cmd%
  case s
  case st
    return 0
    halt
done
if %actor.quest_variable[school:fight]% == 3 
  quest variable school %actor% fight 4
  mforce %actor% study
  wait 2
  msend %actor% %self.name% tells you, 'Spell slots will recover significantly faster if you &6&b(MED)ITATE&0.
  msend %actor% I'll walk you through that process now.
  msend %actor% First, get comfortable.  
  msend %actor% Type &2&brest&0 to take a seat and settle down.'
endif
return 0
~
#51941
academy_cleric_command_rest~
0 c 100
rest~
if %actor.quest_variable[school:fight]% == 4
  quest variable school %actor% fight 5
  mforce %actor% rest
  wait 2
  msend %actor% %self.name% tells you, 'Next, you should &6&b(MED)ITATE&0 to get into the proper state of mind.
  msend %actor% As it goes up, the &6&bMEDITATE&0 skill increases your &6&bFOCUS&0 score when you &6&bMEDITATE&0.
  msend %actor% You can &6&bMEDITATE&0 as long as you're not in combat.'
  wait 2s
  msend %actor% %self.name% tells you, 'Type &2&bmeditate&0 to start.'
endif
return 0
~
#51942
academy_cleric_command_meditate~
0 c 100
meditate~
switch %cmd%
  case m
  case me
    return 0  
    halt
done
if %actor.quest_variable[school:fight]% == 5
  quest variable school %actor% fight 6
  mforce %actor% meditate
  wait 2
  msend %actor% %self.name% tells you, 'It will take a while to finish recovering your spell slots.
  msend %actor% You will receive a notification each time you recover a spell slot and when you recover them all.'
  wait 2s
  msend %actor% %self.name% tells you, 'You do not gain a bonus to &6&bFOCUS&0 when sleeping or just resting.
  msend %actor% If you &6&b(ST)AND&0 or &6&b(SL)EEP&0 you'll immediately stop meditating.
  msend %actor% Don't worry, your spell slots will still recover at their normal rate.'
  wait 3s
  msend %actor% %self.name% tells you, 'When you're done, type &2&bstand&0 so I know you're ready to continue.'
endif
return 0
~
#51943
academy_cleric_command_stand~
0 c 100
stand~
switch %cmd%
  case s
    return 0
    halt
done
if %actor.quest_variable[school:fight]% == 6
  quest variable school %actor% fight 7
  mforce %actor% stand
  wait 2
  mload mob 51900
  mecho %self.name% summons a horrible little monster!
  wait 2s
  msend %actor% %self.name% tells you, 'Killing creatures like this is how you gain experience.
  msend %actor% Gaining experience is how you go up in level.
  msend %actor% Player killing is generally not allowed in FieryMUD.
  msend %actor% This includes casting offensive spells at other players.
  msend %actor% &0 
  msend %actor% &0Before you strike, take a moment to size up your opponent.
  msend %actor% Use the &6&b(CO)NSIDER&0 command to see what your chances are.
  msend %actor% Keep in mind FieryMUD is made for groups of 4-8, so the results of &6&bCONSIDER&0 aren't perfect
  msend %actor% Also keep in mind most of a cleric's strength is support.  
  msend %actor% It will be harder for you to kill creatures alone.'
  wait 2s
  msend %actor% %self.name% tells you, 'Type &2&bconsider monster&0 for see chances.'
endif
return 0
~
#51944
academy_cleric_command_consider~
0 c 100
consider~
set thing horrible-little-monster
switch %cmd%
  case c
    return 0
    halt
done
if %actor.quest_variable[school:fight]% == 7
  quest variable school %actor% fight last
  mforce %actor% con %arg%
  wait 2
  msend %actor% %self.name% tells you, 'Very good.'
  wait 2s
  msend %actor% %self.name% tells you, 'To attack a creature, use the &6&b(KIL)L&0 command. Type &2&bkill monster&0 to start fighting.
  msend %actor% &0
  msend %actor% Feel free to use any of the skills you've learned.
  msend %actor% &0
  msend %actor% You can always use the &6&b(FL)EE&0 command to try to run away.
  msend %actor% It's a good idea if you start to run low on hit points.
  msend %actor% If you try to flee and fail, you'll be stunned for a little bit.
  msend %actor% You cannot flee while casting a spell either!
  msend %actor% So don't wait until the last second to run!'
  wait 2s
  msend %actor% %self.name% tells you, 'When you've killed the monster, I'll teach you about &3&bLOOT&0 and &3&bTOGGLES&0.'
  msend %actor% &2&bSay loot&0 when you're ready to continue.'
endif
return 0
~
#51945
academy_sorcerer_greet~
0 h 100
~
wait 2
set stage %actor.quest_stage[school]%
if %stage% == 3 || %stage% == 4
  if %stage% == 3
    switch %actor.quest_variable[school:fight]%
      case 1
        msend %actor% As a sorcerer you need extensive notes to cast correctly, which you must &6&bSCRIBE&0 in a &6&bSPELLBOOK&0.
        msend %actor% This means always carrying your &6&bSPELLBOOK&0 with you to cast spells.'
        wait 3s
        msend %actor% %self.name% tells you, 'I'll walk you through the &6&bSCRIBE&0 process now.
        msend %actor% &0First, you have to get comfortable.  Type &2&brest&0 to take a seat and settle down.'
        break
      case 2
        msend %actor% %self.name% tells you, 'As a sorcerer, you need to &6&bSCRIBE&0 spells in your spellbook.
        msend %actor% &6&b(SPE)LL&0 will show you all the spells you can &6&bSCRIBE&0.
        msend %actor% You must &6&bHOLD&0 a &3&bSPELLBOOK&0 with blank pages and a &3&bPEN&0 of some kind to scribe.
        if !%actor.wearing[1029]% && !%actor.inventory[1029]% && !%actor.wearing[1154]% && !%actor.inventory[1154]%
          msend %actor% Looks like you need a book and pen!
          mload obj 1154
          mload obj 1029
          give book %actor%
          give pen %actor%
        else
          msend %actor% All new sorcerers start with an empty book and quill.
        endif
        msend %actor% &0 
        msend %actor% Once you leave the Academy, you many only &6&bSCRIBE&0 spells in the presence of your &3&bGuild Master&0.'
        msend %actor% &0
        msend %actor% %self.name% tells you, 'Check your spell list by typing &2&bspell&0.'
        break
      case 3
        msend %actor% %self.name% tells you, 'For now, you're going to learn &3&bmagic missile&0.
        msend %actor% &0
        msend %actor% &6&b(REM)OVE&0 anything in your hands.
        msend %actor% Then &6&b(H)OLD&0 your spellbook.'
        msend %actor% &0
        if %actor.worn[wield]%
          msend %actor% %self.name% tells you, 'You need to &2&bremove&0 the weapon in your hands.'
        endif
        if %actor.worn[hold]%
          msend %actor% %self.name% tells you, 'You need to &2&bremove&0 the item in your hands.'
        endif
        if %actor.worn[hold2]%
          msend %actor% %self.name% tells you, 'You need to &2&bremove&0 the item in your hands.'
        endif
        if %actor.worn[2hwield]%
          msend %actor% %self.name% tells you, 'You need to &2&bremove&0 the item in your hands.'
        endif
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Type &2&bhold spellbook&0 to grab your book.'
        break
      case 4
        msend %actor% %self.name% tells you, 'Next, &6&b(H)OLD&0 your quill.
        if !%actor.wearing[1154]% && !%actor.inventory[1154]%
          msend %actor% Looks like you need a new one.'
          mload obj 1154
          give pen %actor%
        else
          msend %actor% You started with one.'
        endif
        wait 1s
        msend %actor% &0Type &6&bhold quill&0 to grab your quill.'
        break
      case 5
        msend %actor% %self.name% tells you, 'Now you can &6&bSCRIBE&0 any spell on your spell list.
        msend %actor% Each spell takes up pages in your spellbook.
        msend %actor% The number of pages is affected by your &6&bSCRIBE&0 skill.
        msend %actor% You can write any spell you know in your spellbook as long as you're with your Guild Master and your spellbook enough blank pages left in it.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'For now, just type &2&bscribe magic missile&0.'
        break
      case 6
        quest variable school %actor% fight 6
      case 7
        mload mob 51900
        mecho %self.name% summons a horrible little monster!
        wait 2s
        msend %actor% %self.name% tells you, 'Killing creatures like this is how you gain experience.
        msend %actor% Gaining experience is how you go up in level.
        msend %actor% Player killing is generally not allowed in FieryMUD.
        msend %actor% This includes casting offensive spells at other players.
        msend %actor% &0 
        msend %actor% &0Before you strike, take a moment to size up your opponent.
        msend %actor% Use the &6&b(CO)NSIDER&0 command to see what your chances are.
        msend %actor% Keep in mind FieryMUD is made for groups of 4-8, so the results of &6&bCONSIDER&0 aren't perfect
        msend %actor% &0
        msend %actor% %self.name% tells you, 'Type &2&bconsider monster&0 for see chances.'
        break
      case 8
        if %get.mob_count[51900]% == 0
          mload mob 51900
          mecho %self.name% summons a horrible little monster!
          wait 1s
        endif
        msend %actor% %self.name% tells you, 'To attack a creature use the &6&b(KIL)L&0 command.
        msend %actor% As a sorcerer however, your strength comes from your magic.
        msend %actor% It's often best to start combat with an offensive spell.
        msend %actor% If you're lucky, you'll kill your opponent out-right.'
        wait 2s
        msend %actor% %self.name% tells you, 'To &6&bCAST&0 type &6&b(c)ast '[spell]' [target]&0.
        msend %actor% &0 
        msend %actor% &0FieryMUD will try to match &6&babbreviations of spell names and targets.&0
        msend %actor% If a spell name has more than one word you &6&bmust&0 put single quotation marks &6&b' '&0 around it.
        msend %actor% &0 
        msend %actor% &0Spellcasting is not instaneous either.
        msend %actor% Each spell has a base length to cast.
        msend %actor% Your proficiency in &6&bQUICK CHANT&0 will reduce casting time.
        msend %actor% &0 
        msend %actor% Once you &6&bCAST&0 a spell you have to &6&bMEMORIZE&0 it before you can cast it again.'
        wait 2s
        point monster
        msend %actor% &0 
        msend %actor% %self.name% tells you, '&6&bCAST&0 your spell at that monster.
        msend %actor% Type &2&bcast 'magic missile' monster&0.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'When you've killed the monster, &2&bsay recovery&0 I'll teach you about &3&bSPELL RECOVERY&0.'
        break
      case 9
        msend %actor% You can use the &6&b(STU)DY&0 command to see all the information about your spell slots, including:
        msend %actor%   &5&b1. How many spell slots you have
        msend %actor%   2. Which spell slots you've used
        msend %actor%   3. How long each slot will take to recover&0
        msend %actor% &0
        msend %actor% Check your current recovery status by typing &2&bstudy&0 now.'
        break
      case 10
        msend %actor% %self.name% tells you, 'Spell slots will recover significantly faster if you &6&b(MED)ITATE&0.
        msend %actor% I'll walk you through that process now.
        msend %actor% First, get comfortable.  
        msend %actor% Type &2&brest&0 to take a seat and settle down.'
        break
      case 11
        msend %actor% %self.name% tells you, 'You should &6&b(MED)ITATE&0 to get into the proper state of mind.
        msend %actor% As it goes up, the &6&bMEDITATE&0 skill increases your &6&bFOCUS&0 score when you &6&bMEDITATE&0.
        msend %actor% You can &6&bMEDITATE&0 as long as you're not in combat.'
        wait 2s
        msend %actor% %self.name% tells you, 'Type &2&bmeditate&0 to start.'
        break
      case 12
        quest variable school %actor% fight complete
        quest advance school %actor%
        wait 2
        msend %actor% %self.name% tells you, 'Next I'll teach you about &3&bLOOT&0 and &3&bTOGGLES&0.'
        msend %actor% &2&bSay loot&0 when you're ready to continue.'
        break
      default
        msend %actor% %self.name% turns to greet you.
        wait 2s
        nod %actor%
        msend %actor% %self.name% tells you, 'Ah a new student!'
        wait 2s
        msend %actor% %self.name% tells you, 'Greetings %actor.name%.  I am the Chair of Arcane Studies at Ethilien Academy.  Welcome to my laboratory.'
        bow %actor%
        wait 3s
        msend %actor% %self.name% tells you, 'I assume you're here for the basics of magical &3&bcombat&0.'
        wait 3s
        msend %actor% %self.name% tells you, 'Let's start by looking at your &6&b(SK)ILLS&0.
        msend %actor% &0Type &2&bskill&0 to see what you can do.'
    done
    msend %actor% &0 
    msend %actor% %self.name% tells you, 'You can also say &5SKIP&0 at any time to talk about &3&bLOOT&0 and &3&bTOGGLES&0 instead.'
  elseif %stage% == 4
    switch %actor.quest_variable[school:loot]%
      case 1
        msend %actor% %self.name% tells you, 'When something dies, it usually leaves behind a &3&bcorpse&0.'
        wait 1s
        poke %actor%
        msend %actor% %self.name% tells you, 'That goes for you too kid.'
        wait 1s
        msend %actor% %self.name% tells you, 'A corpse is like a container.
        msend %actor% You can &6&bEXAMINE corpse&0 it to see what it has on it.  You can
        msend %actor% &0&6&bGET [object] corpse&0 to take something specific, or you can &6&bGET ALL corpse&0 to take everything on it.
        msend %actor% Corpses keep their names as keywords so you can use those too.
        msend %actor% &0 
        msend %actor% You can't pick up a corpse, but you can &6&bDRAG&0 them from room to room.
        msend %actor% You need &6&bCONSENT&0 to drag a player corpse though.
        msend %actor% &0
        msend %actor% If YOU die, you have to trudge all the way back to the room you died in, then get everything from your body, like &6&bGET ALL corpse&0, to get your stuff.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'For now just type &2&bget all corpse&0 and we'll keep moving.'
        break
      case 2
        msend %actor% %self.name% tells you, 'The other thing I need to show you is the &6&bTOGGLE&0 command.
        msend %actor% Typing &6&bTOGGLE&0 alone will show you everything you can set.
        msend %actor% &6&bAUTOLOOT&0 picks up everything from a corpse instantly.
        msend %actor% &6&bAUTOTREAS&0 picks up only "treasure" like coins and gems.
        msend %actor% Each toggle has a &6&bHELP&0 file with more information.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'For now, let's get you ready to gather and share loot.
        msend %actor% Type &2&btoggle autoloot&0 to loot future kills!'
        break
      case 3
        msend %actor% %self.name% tells you, 'When playing with others it's considered polite to share the wealth.
        msend %actor% You can share money with the &6&bSPLIT&0 command.
        msend %actor% With &6&bTOGGLE AUTOSPLIT&0 on the game will automatically do that when you pick up money.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Type &2&btoggle autosplit&0 to set that.'
        break
      case 4
        msend %actor% %self.name% tells you, 'The last combat tip is checking your &6&b(TRO)PHY&0 list.
        msend %actor% &6&bTROPHY&0 shows a record of the last 24 creatures you've killed and how many times you've killed them.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Type &2&btrophy&0 and see your record.'
        break
      default
        msend %actor% %self.name% tells you, '&2&bSay loot&0 to learn about &3&bLOOT&0 and &3&bTOGGLES.'
    done
    msend %actor% &0 
    msend %actor% %self.name% tells you, 'You can also say &5SKIP&0 to move on to the next teacher.'
  endif
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'You can also say &5EXIT&0 at any time to leave the Academy.'
endif
~
#51946
academy_sorcerer_command_skill~
0 c 100
skills~
switch %cmd%
  case s
    return 0
    halt
done
if %actor.quest_stage[school]% == 3 && !%actor.quest_variable[school:fight]%
  quest variable school %actor% fight 1
  mforce %actor% skill
  wait 2
  msend %actor% %self.name% tells you, 'Unlike many MUDs you will not need to train or practice your skills with a trainer or Guild Master.
  msend %actor% Skills will improve gradually as you use them.
  msend %actor% The meter next to the skill name shows you two things:
  msend %actor% &6&b1. How close you are to maximum skill for your level, indicated by the "=" sign&0
  msend %actor% &6&b2. How close you are to overall maximum skill for your class and race, indicated by the "*" sign.&0
  msend %actor% &0 
  msend %actor% Your most important skills are the &6&bspheres&0.
  msend %actor% These indicate of your proficiency in a type of magic.
  msend %actor% They increase whenever you &6&bCAST&0 a spell.
  msend %actor% The spheres are not spells or attacks themselves.
  msend %actor% &0 
  msend %actor% As a sorcerer you need extensive notes to cast correctly.
  msend %actor% This means always carrying your &6&bSPELLBOOK&0 with you so you can refer to and &6&b(ME)MORIZE&0 your notes.'
  wait 3s
  msend %actor% %self.name% tells you, 'There are five steps to casting:'
  msend %actor% 1. &5&b(R)EST&0
  msend %actor% 2. &5&bSCRIBE&0
  msend %actor% 3. &5&b(MED)ITATE&0
  msend %actor% 4. &5&b(ME)MORIZE&0
  msend %actor% 5. &5&b(C)AST&0&0
  wait 2s
  msend %actor% %self.name% tells you, 'I'll walk you through that process now.
  msend %actor% &0First, you have to get comfortable.  Type &2&brest&0 to take a seat and settle down.'
endif
return 0
~
#51947
academy_sorcerer_command_rest~
0 c 100
rest~
if %actor.quest_variable[school:fight]% == 1
  quest variable school %actor% fight 2
  mforce %actor% rest
  wait 2
  msend %actor% %self.name% tells you, 'As a sorcerer, you need to &6&bSCRIBE&0 spells in your spellbook.
  msend %actor% &6&b(SPE)LL&0 will show you all the spells you can &6&bSCRIBE&0.
  msend %actor% You must &6&bHOLD&0 a &3&bSPELLBOOK&0 with blank pages and a &3&bPEN&0 of some kind to scribe.
  if !%actor.wearing[1029]% && !%actor.inventory[1029]% && !%actor.wearing[1154]% && !%actor.inventory[1154]%
    msend %actor% Looks like you need a book and pen!
    mload obj 1154
    mload obj 1029
    give book %actor%
    give pen %actor%
  else
    msend %actor% All new sorcerers start with an empty book and quill.
  endif
  msend %actor% &0 
  msend %actor% Once you leave the Academy, you many only &6&bSCRIBE&0 spells in the presence of your &3&bGuild Master&0.'
  wait 2s
  msend %actor% %self.name% tells you, 'Check your spell list by typing &2&bspell&0.'
elseif %actor.quest_variable[school:fight]% == 10
  quest variable school %actor% fight 11
  mforce %actor% rest
  wait 2
  msend %actor% %self.name% tells you, 'Next, you should &6&b(MED)ITATE&0 to get into the proper state of mind.
  msend %actor% As it goes up, the &6&bMEDITATE&0 skill increases your &6&bFOCUS&0 score when you &6&bMEDITATE&0.
  msend %actor% You can &6&bMEDITATE&0 as long as you're not in combat.'
  wait 2s
  msend %actor% %self.name% tells you, 'Type &2&bmeditate&0 to start.'
endif
return 0
~
#51948
academy_sorcerer_command_spell~
0 c 100
spell~
switch %cmd%
  case s
  case sp
    return 0
    halt
done
if %actor.quest_variable[school:fight]% == 2
  quest variable school %actor% fight 3
  mforce %actor% spell
  wait 2
  msend %actor% %self.name% tells you, 'Spells are divided up by Circle.
  msend %actor% For now you only have access to Circle 1 spells.
  msend %actor% You gain access to a new Circle every 8 levels.
  msend %actor% You can find out what any spell does by consulting the &6&bHELP&0 file.
  msend %actor% &0 
  msend %actor% For now, you're going to learn &3&bmagic missile&0.
  msend %actor% &0
  msend %actor% &6&b(REM)OVE&0 anything in your hands.
  msend %actor% Then &6&b(H)OLD&0 your spellbook.'
  wait 2s
  if %actor.worn[wield]%
    msend %actor% %self.name% tells you, 'You need to &2&bremove&0 the weapon in your hands.'
  endif
  if %actor.worn[hold]%
    msend %actor% %self.name% tells you, 'You need to &2&bremove&0 the item in your hands.'
  endif
  if %actor.worn[hold2]%
    msend %actor% %self.name% tells you, 'You need to &2&bremove&0 the item in your hands.'
  endif
  if %actor.worn[2hwield]%
    msend %actor% %self.name% tells you, 'You need to &2&bremove&0 the item in your hands.'
  endif
  msend %actor% &0 
  if !%actor.wearing[1029]% && !%actor.inventory[1029]%
    msend %actor% %self.name% tells you, 'Looks like you need a new book too.'
    mload obj 1029
    give book %actor%
    msend %actor% &0
  endif
  msend %actor% %self.name% tells you, 'Type &2&bhold spellbook&0 to grab your book.'
endif
return 0
~
#51949
academy_wear_spellbook~
1 j 100
~
if %actor.quest_variable[school:fight]% == 3
  quest variable school %actor% fight 4
  wait 2
  osend %actor% %get.mob_shortdesc[51906]% tells you, 'Next, &6&b(H)OLD&0 your quill.
  if !%actor.wearing[1154]% && !%actor.inventory[1154]%
    osend %actor% Looks like you need a new one.'
    oforce chair mload obj 1154
    oforce chair give pen %actor%
  else
    osend %actor% You started with one.'
  endif
  wait 1s
  osend %actor% &0Type &6&bhold quill&0 to grab your quill.'
endif
~
#51950
academy_sorcerer_command_scribe~
0 c 100
scribe~
switch %cmd%
  case s
  case sc
    return 0
    halt
done
switch %arg%
  case magic m
  case magic mi
  case magic mis
  case magic miss
  case magic missi
  case magic missil
  case magic missile
    if %actor.quest_variable[school:fight]% == 5
      quest variable school %actor% fight 6
      mforce %actor% scribe magic missile
      wait 2
      msend %actor% %self.name% tells you, 'This will take some time.
      msend %actor% When you're done writing, &2&bstand&0 so I know you're ready to continue.'
    endif
done
return 0
~
#51951
academy_sorcerer_command_stand~
0 c 100
stand~
switch %cmd%
  case s
    return 0
    halt
done
if %actor.quest_variable[school:fight]% == 6
  quest variable school %actor% fight 7
  mforce %actor% stand
  wait 2
  msend %actor% %self.name% tells you, 'With the spell scribed you can &6&bCAST&0 it.
  msend %actor% Unlike many other MUDs, &1FieryMUD does not use mana.&0
  msend %actor% You can cast any spell from your list as long as you have available &6&bSPELL SLOTS&0 of the same level or higher.
  msend %actor% &0
  msend %actor% At level 1, you have only one Circle 1 spell slot.
  msend %actor% You can use that spell slot to &6&bCAST&0 any Circle 1 spell.
  msend %actor% You gain more spells slots as you increase in level.'
  wait 2s
  mload mob 51900
  mecho %self.name% summons a horrible little monster!
  wait 1s
  msend %actor% %self.name% tells you, 'Killing creatures like this is how you gain experience.
  msend %actor% Gaining experience is how you go up in level.
  msend %actor% Player killing is generally not allowed in FieryMUD.
  msend %actor% This includes casting offensive spells at other players.
  msend %actor% &0 
  msend %actor% &0Before you strike, take a moment to size up your opponent.
  msend %actor% Use the &6&b(CO)NSIDER&0 command to see what your chances are.
  msend %actor% Keep in mind FieryMUD is made for groups of 4-8, so the results of &6&bCONSIDER&0 aren't perfect
  msend %actor% &0
  msend %actor% %self.name% tells you, 'Type &2&bconsider monster&0 for see chances.'
elseif %actor.quest_variable[school:fight]% == 12
  quest variable school %actor% fight complete
  quest advance school %actor%
  mforce %actor% stand
  wait 2
  msend %actor% %self.name% tells you, 'Next I'll teach you about &3&bLOOT&0 and &3&bTOGGLES&0.'
  msend %actor% &2&bSay loot&0 when you're ready to continue.'  
endif
return 0
~
#51952
academy_sorcerer_command_consider~
0 c 100
consider~
switch %cmd%
  case c
    return 0
    halt
done
set thing horrible-little-monster
if %actor.quest_variable[school:fight]% == 7
  quest variable school %actor% fight 8
  mforce %actor% consider %arg%
  wait 2
  msend %actor% %self.name% tells you, 'Very good.'
  wait 2s
  msend %actor% %self.name% tells you, 'Typically combat starts with the command:
  msend %actor% &6&b(KIL)L [target]&0'
  wait 1s
  msend %actor% %self.name% tells you, 'As a sorcerer however, your strength comes from your magic.
  msend %actor% It's often best to start combat with an offensive spell.
  msend %actor% If you're lucky, you'll kill your opponent out-right.'
  wait 2s
  msend %actor% %self.name% tells you, 'To &6&bCAST&0 type &6&b(c)ast '[spell]' [target]&0.
  msend %actor% &0 
  msend %actor% &0FieryMUD will try to match &6&babbreviations of spell names and targets.&0
  msend %actor% If a spell name has more than one word you &6&bmust&0 put single quotation marks &6&b' '&0 around it.
  msend %actor% &0 
  msend %actor% &0Spellcasting is not instaneous either.
  msend %actor% Each spell has a base length to cast.
  msend %actor% Your proficiency in &6&bQUICK CHANT&0 will reduce casting time.'
  wait 2s
  point monster
  msend %actor% &0 
  msend %actor% %self.name% tells you, '&6&bCAST&0 your spell at that monster.
  msend %actor% Type &2&bcast 'magic missile' monster&0.'
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'You can always use the &6&b(FL)EE&0 command to try to run away.
  msend %actor% It's a good idea to &6&bFLEE&0 if you start to run low on hit points.
  msend %actor% If you try to flee and fail, you'll be stunned for a little bit.
  msend %actor% You also cannot flee while casting a spell! Don't wait until the last second to run!'
  wait 3s
  msend %actor% %self.name% tells you, 'When you've killed the monster, &2&bsay recovery&0 I'll teach you about &3&bSPELL RECOVERY&0.'
endif
return 0
~
#51953
academy_sorcerer_speech_recovery~
0 d 100
recovery~
wait 2
if %actor.quest_variable[school:fight]% == 8
  quest variable school %actor% fight 9
  msend %actor% %self.name% tells you, 'Once you cast a spell, the spell slot it used will automatically begin to &6&bRECOVER&0.
  msend %actor% The amount of time a spell slot takes to recover depends on the Circle of spell slot, your &6&bFOCUS&0 score, and whether or not you &6&bMEDITATE&0.
  msend %actor% Spell slots recover &3&bin the order they were used&0.
  msend %actor% &0  
  msend %actor% &6&bFOCUS&0 is a bonus gained from equipment and your Intelligence and Wisdom stats.
  msend %actor% You can see your &6&bFOCUS&0 score using the &6&bSCORE&0 command.
  msend %actor% &0
  msend %actor% You can use the &6&b(STU)DY&0 command to see all the information about your spell slots, including:
  msend %actor%   &5&b1. How many spell slots you have
  msend %actor%   2. Which spell slots you've used
  msend %actor%   3. How long each slot will take to recover&0
  msend %actor% &0
  msend %actor% Check your current recovery status by typing &2&bstudy&0 now.'
endif
return 0
~
#51954
academy_sorcerer_command_study~
0 c 100
study~
switch %cmd%
  case s
  case st
    return 0
    halt
done
if %actor.quest_variable[school:fight]% == 9
  quest variable school %actor% fight 10
  mforce %actor% study
  wait 2
  msend %actor% %self.name% tells you, 'Spell slots will recover significantly faster if you &6&b(MED)ITATE&0.
  msend %actor% I'll walk you through that process now.
  msend %actor% First, get comfortable.  
  msend %actor% Type &2&brest&0 to take a seat and settle down.'
endif
return 0
~
#51955
academy_classes_speech1~
0 d 100
loot~
wait 2
if %actor.quest_stage[school]% == 4 && !%actor.quest_variable[school:loot]% 
  quest variable school %actor% loot 1
  msend %actor% %self.name% tells you, 'When something dies, it usually leaves behind a &3&bcorpse&0.'
  wait 1s
  poke %actor%
  msend %actor% %self.name% tells you, 'That goes for you too kid.'
  wait 1s
  msend %actor% %self.name% tells you, 'A corpse is like a container.
  msend %actor% You can &6&bEXAMINE corpse&0 it to see what it has on it.
  msend %actor% You can &6&bGET [object] corpse&0 to take something specific, or you can &6&bGET ALL corpse&0 to take everything on it.
  msend %actor% Corpses keep their names as keywords so you can use those too.
  msend %actor% &0 
  msend %actor% You can't pick up a corpse, but you can &6&bDRAG&0 them from room to room.
  msend %actor% You need &6&bCONSENT&0 to drag a player corpse though.
  msend %actor% &0
  msend %actor% If YOU die, you have to trudge all the way back to the room you died in, then get everything from your body, like &6&bGET ALL corpse&0, to get your stuff.'
  wait 2s
  msend %actor% %self.name% tells you, 'For now loot this monster by typing &2&bget all corpse&0.'
elseif %actor.quest_stage[school]% == 3 && %actor.quest_variable[school:fight]% == last
  msend %actor% %self.name% tells you, 'You need to &2&bkill monster&0 first!'
  mload mob 51900
endif
~
#51956
academy_classes_command_get~
0 c 100
get~
switch %arg%
  case all c
  case all co
  case all cor
  case all corp
  case all corps
  case all corpse
    if %actor.quest_variable[school:loot]% == 1
      quest variable school %actor% loot 2
      mforce %actor% get all corpse
      wait 2
      msend %actor% %self.name% tells you, 'And now you have some money!
      msend %actor% Remember, use &6&bSCORE&0 to check your money.
      msend %actor% &0 
      msend %actor% The other thing I need to show you is the &6&bTOGGLE&0 command.
      msend %actor% Typing &6&bTOGGLE&0 alone will show you everything you can set.
      msend %actor% &6&bAUTOLOOT&0 picks up everything from a corpse instantly.
      msend %actor% &6&bAUTOTREAS&0 picks up only "treasure" like coins and gems.
      msend %actor% Each toggle has a &6&bHELP&0 file with more information.'
      wait 2s
      msend %actor% %self.name% tells you, 'For now, let's get you ready to gather and share loot.
      msend %actor% Type &2&btoggle autoloot&0 to loot future kills!'
    endif
done
return 0
~
#51957
academy_classes_command_trophy~
0 c 100
trophy~
switch %cmd%
  case t
  case tr
    return 0
    halt
done
if %actor.quest_variable[school:loot]% == 4
  quest variable school %actor% loot complete
  quest advance school %actor%
  mforce %actor% trophy
  wait 2
  msend %actor% %self.name% tells you, 'Your most recent kills appear at the top.
  msend %actor% &0 
  msend %actor% You earn 1.00 share of each monster you kill solo.
  msend %actor% If you're in a group, that share is evenly divided among all group members present for the kill.
  msend %actor% &0 
  msend %actor% At &3&b5.00&0, you earn less experience per kill.
  msend %actor% At &1&b10.00&0, you earn far less experience per kill.
  msend %actor% &0 
  msend %actor% You can clear out your trophy list by killing creatures in a different area.
  msend %actor% Creatures move down the list as you kill new monsters.
  msend %actor% After 24 new monsters, they disappear completely.
  msend %actor% Your trophy list is also cleared when you die.'
  wait 2s
  msend %actor% %self.name% tells you, 'Looks like you're ready for some rest!
  msend %actor% Head &2&beast&0 to the banquet hall.'
endif
return 0
~
#51958
academy_revel_greet~
0 h 100
~
wait 2
if %actor.quest_stage[school]% == 5
  msend %actor% %self.name% tells you, 'Congratulations on completing your combat training!
  msend %actor% I'm the last instructor in the Academy.
  msend %actor% I'm here to help you get down and party!'
  dance
  wait 2s
  msend %actor% %self.name% tells you, 'After combat you'll probably have some downtime.
  msend %actor% It's a good time for &3&bRESTING&0 to recover health and dealing with &3&bMONEY&0.'
  wait 2s
  msend %actor% %self.name% tells you, 'It looks like you still need to complete lessons on:
  if %actor.quest_variable[school:rest]% != complete
    msend %actor% &3&bRESTING&0
  endif
  if %actor.quest_variable[school:money]% != complete
    msend %actor% &3&bMONEY&0
  endif
  msend %actor% &0 
  msend %actor% &2&bSay&0 one of these to start or resume your lesson where you left off.
  msend %actor% &0 
  msend %actor% Or you can say &5SKIP&0 to skip to the end of the Academy.'
elseif %actor.quest_stage[school]% == 6
  msend %actor% %self.name% tells you, 'You're ready to graduate from Ethilien Academy!
  msend %actor% &2&bSay finish&0 to end the school.'
endif
~
#51959
academy_revel_command_rest~
0 c 100
rest~
if !%actor.quest_variable[school:rest]%
  quest variable school %actor% rest 1
  mforce %actor% rest
  wait 2
  msend %actor% %self.name% tells you, 'Yeah, that's it!'
  wait 2s
  msend %actor% %self.name% tells you, 'If you want to speed things up even more, you can lay down and go to &6&bSLEEP&0.
  msend %actor% There're some risks with sleeping.
  msend %actor% You won't be able to hear or see the world around you, so you won't know if danger is approaching.
  msend %actor% You won't be able to use &6&bSAY&0 or &6&bSHOUT&0, or do anything most people can't normally do in their sleep.
  msend %actor% You can still &6&bGOSSIP&0, &6&bTELL&0, and check your &6&bINVENTORY&0 or &6&bEQUIPMENT&0 though!'
  wait 3s
  msend %actor% %self.name% tells you, 'Take a quick nap!
  msend %actor% Type &2&bsleep&0 to lay down.'
endif
return 0
~
#51960
academy_revel_command_sleep~
0 c 100
sleep~
switch %cmd%
  case s
    return 0
    halt
done
if %actor.quest_variable[school:rest]% == 1
  quest variable school %actor% rest 2
  mforce %actor% sleep
  wait 2
  msend %actor% %self.name% tells you, 'Sweet dreams!'
  wait 1s
  msend %actor% %self.name% tells you, 'Remember, you may want to enter commands every now and then to update your display and see how close you are to full health and movement.
  msend %actor% The display won't update on its own.'
  wait 2s
  msend %actor% %self.name% tells you, 'The &6&bWAKE&0 command makes you stop sleeping and sit up.
  msend %actor% When you wake up, you'll automatically go to a sitting &6&bREST&0 position.'
  wait 2s
  msend %actor% %self.name% tells you, 'So type &2&bwake&0 and get up!'&0
endif
return 0
~
#51961
academy_revel_command_wake~
0 c 100
wake~
switch %cmd%
  case w
    return 0
    halt
done
if %actor.quest_variable[school:rest]% == 2
  quest variable school %actor% rest 3
  mforce %actor% wake
  wait 2
  msend %actor% %self.name% tells you, 'Good morning sunshine!'
  wait 2s
  msend %actor% %self.name% tells you, 'You'll frequently notice yourself getting hungry or thirsty.
  msend %actor% You can speed up your recovery if you &6&bEAT&0 and &6&b(DRI)NK&0.
  msend %actor% &0 
  msend %actor% When you get thirsty it's time to &6&bDRINK&0!  
  msend %actor% Ethilien has a huge variety of drinkable liquids.
  msend %actor% Any of them will slake your thirst.
  msend %actor% The command is &6&bDRINK [container]&0.
  msend %actor% &0 
  msend %actor% When you &6&bDRINK&0, you'll also regain some Movement Points, so make sure you carry a full waterskin so you can keep moving when you explore!
  msend %actor% &0 
  msend %actor% And remember, magic potions are &1not&0 drinks.
  msend %actor% If you want to consume a potion, the commmand is &6&b(Q)UAFF&0, not &6&b(DRI)NK&0.'
  wait 2s
  if !%actor.inventory[20]% && !%actor.wearing[20]%
    msend %actor% %self.name% tells you, 'Here's a new waterskin for you.
    mload obj 20
    give waterskin %actor%
  else
    msend %actor% %self.name% tells you, 'You started play with a full waterskin.
  endif
  msend %actor% Type &2&bdrink waterskin&0 now to drink out of it.'
endif
return 0
~
#51962
academy_revel_command_drink~
0 c 100
drink~
switch %cmd%
  case d
  case dr
    return 0
    halt
done
if %actor.quest_variable[school:rest]% == 3 && (waterskin /= %arg% || leather /= %arg% || skin /= %arg%)
  quest variable school %actor% rest 4
  mforce %actor% drink %arg%
  wait 2
  msend %actor% %self.name% tells you, 'Cheers!'
  wait 1s
  msend %actor% %self.name% tells you, 'Every now and then you'll have to &6&bFILL&0 your drinking vessel.'
  wait 1s
  msend %actor% %self.name% tells you, 'Throughout the world you'll find many springs and fountains.'
  wait 1s
  msend %actor% %self.name% tells you, 'Like this one!'
  point fountain
  wait 4s
  msend %actor% %self.name% tells you, 'You can &6&bFILL&0 a drink container from a source like this by typing &6&bFILL [container] [source]&0.'
  msend %actor% &0
  msend %actor% %self.name% tells you, 'But first you have to &2&bstand&0 up.'
endif
return 0
~
#51963
academy_revel_command_fill~
0 c 100
fill~
switch %cmd%
  case f
  case fi
    return 0
    halt
done
if %actor.quest_variable[school:rest]% == 5
  quest variable school %actor% rest 6
  mforce %actor% fill %arg%
  wait 2
  msend %actor% %self.name% tells you, 'Just like that, perfect!'
  wait 2s
  msend %actor% %self.name% tells you, 'Now we feast!'
  wait 2s
  mload obj 20305
  give meat %actor.name%
  wait 2s
  msend %actor% %self.name% tells you, 'Ethilien is filled with edible foods.
  msend %actor% &0You can &6&bEAT&0 your way across the world!
  msend %actor% Being full increases your regeneration rate.
  msend %actor% Every time you &6&bEAT&0 you also immediately regain some Hit Points!
  msend %actor% Food has no other effects though.'
  wait 3s
  msend %actor% %self.name% tells you, 'Come, feast with me!  Type &2&beat meat&0!'
  mload obj 20305
endif
return 0
~
#51964
academy_revel_command_eat~
0 c 100
eat~
switch %cmd%
  case e
  case ea
    return 0
    halt
done
set meat large-chunk-meat 
if %actor.quest_variable[school:rest]% == 6 && (%meat% /= %arg% || food /= %arg%) && %actor.inventory[20305]%
  quest variable school %actor% rest 7
  mforce %actor% eat %arg%
  wait 2
  eat meat
  lick
  msend %actor% %self.name% tells you, 'Mmmmm, delicious!'
  wait 1s
  msend %actor% %self.name% tells you, 'When you're all rested up you can head back out into the world.'
  wait 2s
  msend %actor% %self.name% tells you, 'The best place to start your adventures is the &3&bFARMLAND&0 immediately west of the town of Mielikki, the starting town above us.'
  wait 2s
  msend %actor% %self.name% tells you, 'The &6&bHELP ZONE&0 file will show you lots of places to explore.
  msend %actor% Type &2&bhelp zone&0 and check it out.'
endif
return 0
~
#51965
academy_revel_command_deposit~
0 c 100
deposit~
switch %arg%
  case d
    return 0
    halt
done
if %actor.quest_variable[school:money]% == 1 && %arg% /= 1 gold 1 silver 1 copper
  quest variable school %actor% money complete
  mforce %actor% deposit 1 gold 1 silver 1 copper
  wait 2
  msend %actor% %self.name% tells you, 'Thank you for using Academy Bank.
  msend %actor% You can &6&bWITHDRAW&0 your funds at any time.'
  wait 1s
endif
if %actor.quest_stage[school]% == 5
  if %actor.quest_variable[school:money]% == complete && %actor.quest_variable[school:rest]% == complete
    quest advance school %actor%
    msend %actor% %self.name% tells you, 'You're ready to graduate from Ethilien Academy!
    msend %actor% %self.name% says, '&2&bSay finish&0 to end the school.'
  else
    msend %actor% %self.name% tells you, '&2&bSay resting&0 to finish your last lesson, or say &5SKIP&0 to jump to the end of the Academy.'
  endif
endif
return 0
~
#51966
academy_revel_command_help~
0 c 100
help~
if %actor.quest_variable[school:rest]% == 7
  switch %arg%
    case zo
    case zon
    case zone
      quest variable school %actor% rest complete
      mforce %actor% help zone
      wait 2
      msend %actor% %self.name% tells you, 'When you're ready to stop playing, to safely log out, you have to find an inn.
      msend %actor% Talk to the inn's receptionist and &6&bRENT&0 a room. 
      msend %actor% Think of inns like save points.
      msend %actor% Renting stores all your items for free.
      msend %actor% When you log back in, you'll be at the inn.'
      wait 1s
  done
endif
if %actor.quest_stage[school]% == 5
  if %actor.quest_variable[school:money]% == complete && %actor.quest_variable[school:rest]% == complete
    quest advance school %actor%
    msend %actor% %self.name% tells you, 'You're ready to graduate from Ethilien Academy!
    msend %actor% &2&bSay finish&0 to end the school.'
  else
    msend %actor% %self.name% tells you, '&2&bSay money&0 to finish your last lesson, or say &5SKIP&0 to jump to the end of the Academy.'
  endif
endif
return 0
~
#51967
academy_recruiter_greet~
0 h 100
~
if %actor.vnum% == -1
  wait 3
  if %actor.quest_stage[school]% == 0
    msend %actor% %self.name% tells you, 'Hello there!  What kind of training are you here for?
    msend %actor% Type &2&bsay basic&0 if you're brand new to MUDding.
    msend %actor% Type &2&bsay combat&0 if you want to skip straight to FieryMUD's combat and spellcasting system.
    msend %actor% Type &2&bsay none&0 to leave.'
  elseif !%actor.has_completed[school]%
    msend %actor% %self.name% tells you, 'I see you had already started your lessons.
    msend %actor% &2&bSay resume&0 to continue where you left off.'
  else
    msend %actor% %self.name% tells you, 'I see you already finished your lessons.  Would you like to do them again?
    msend %actor% &2&bSay basic&0 to restart them.'
  endif
endif
~
#51968
academy_clerk_block_east~
0 c 100
east~
switch %actor.class%
    case warrior
    case paladin
    case anti-paladin
    case ranger
    case berserker
    case monk
        msend %actor% %self.name% tells you, 'Your trainer is to the north.'
        break
    case sorcerer
    case cryomancer
    case pyromancer
    case illusionist
    case necromancer
        msend %actor% %self.name% tells you, 'Your trainer is to the south.'
        break
    case rogue
    case thief
    case assassin
    case mercenary
    case bard
        msend %actor% %self.name% tells you, 'Your trainer is down.'
        break
    case cleric
    case priest
    case diabolist
    case druid
        mforce %actor% east
        break
    default
        msend %actor% %self.name% tells you, 'Hmmm, I don't actually know where to send you...  Talk to a god.'
done
~
#51969
academy_clerk_block_south~
0 c 100
south~
switch %actor.class%
    case warrior
    case paladin
    case anti-paladin
    case ranger
    case berserker
    case monk
        msend %actor% %self.name% tells you, 'Your trainer is to the north.'
        break
    case rogue
    case thief
    case assassin
    case mercenary
    case bard
        msend %actor% %self.name% tells you, 'Your trainer is down.'
        break
    case cleric
    case priest
    case diabolist
    case druid
        msend %actor% %self.name% tells you, 'Your trainer is to the east.'
        break
    case sorcerer
    case cryomancer
    case pyromancer
    case illusionist
    case necromancer
        mforce %actor% south
        break
    default
        msend %actor% %self.name% tells you, 'Hmmm, I don't actually know where to send you...  Talk to a god.'
done
~
#51970
academy_clerk_block_north~
0 c 100
north~
switch %actor.class%
    case rogue
    case thief
    case assassin
    case mercenary
    case bard
        msend %actor% %self.name% tells you, 'Your trainer is down.'
        break
    case cleric
    case priest
    case diabolist
    case druid
        msend %actor% %self.name% tells you, 'Your trainer is to the east.'
        break
    case sorcerer
    case cryomancer
    case pyromancer
    case illusionist
    case necromancer
        msend %actor% %self.name% tells you, 'Your trainer is to the south.'
        break
    case warrior
    case paladin
    case anti-paladin
    case ranger
    case berserker
    case monk
        mforce %actor% north
        break
    default
        msend %actor% %self.name% tells you, 'Hmmm, I don't actually know where to send you...  Talk to a god.'
done
~
#51971
academy_clerk_block_down~
0 c 100
down~
switch %actor.class%
    case warrior
    case paladin
    case anti-paladin
    case ranger
    case berserker
    case monk
        msend %actor% %self.name% tells you, 'Your trainer is to the north.'
        break
    case cleric
    case priest
    case diabolist
    case druid
        msend %actor% %self.name% tells you, 'Your trainer is to the east.'
        break
    case sorcerer
    case cryomancer
    case pyromancer
    case illusionist
    case necromancer
        msend %actor% %self.name% tells you, 'Your trainer is to the south.'
        break
    case rogue
    case thief
    case assassin
    case mercenary
    case bard
        mforce %actor% down
        break
    default
        msend %actor% %self.name% tells you, 'Hmmm, I don't actually know where to send you...  Talk to a god.'
done
~
#51972
academy_sorcerer_command_meditate~
0 c 100
meditate~
switch %cmd%
  case m
  case me
    return 0  
    halt
done
if %actor.quest_variable[school:fight]% == 11
  quest variable school %actor% fight 12
  mforce %actor% meditate
  wait 2
  msend %actor% %self.name% tells you, 'It will take a while to finish recovering your spell slots.
  msend %actor% You will receive a notification each time you recover a spell slot and when you recover them all.'
  wait 2s
  msend %actor% %self.name% tells you, 'You do not gain a bonus to &6&bFOCUS&0 when sleeping or just resting.
  msend %actor% If you &6&b(ST)AND&0 or &6&b(SL)EEP&0 you'll immediately stop meditating.
  msend %actor% Don't worry, your spell slots will still recover at their normal rate.'
  wait 3s
  msend %actor% %self.name% tells you, 'When you're done, type &2&bstand&0 so I know you're ready to continue.'
endif
return 0
~
#51973
academy_revel_speech_finish~
0 d 100
finish~
wait 2
if %actor.quest_stage[school]% == 6
  quest complete school %actor%
  if %actor.level% == 1
    switch %actor.class%
      case paladin
      case anti-paladin
      case ranger
        set goal 6323
        break
      case sorcerer
      case cryomancer
      case pyromancer
      case illusionist
      case bard
        set goal 6599
        break
      case monk
      case necromancer
        set goal 7149
        break
      case warrior
      case berserker
        set goal 6049
        break
      case cleric
      case priest
      case diabolist
      case druid
      case rogue
      case thief
      case assassin
      case mercenary
      default
        set goal 5499
    done
    eval cap (%goal% / 5)
    eval reward (%goal% - %actor.exp%)
    eval loops (%reward% / %cap%)
    eval diff (%reward% - (%loops% * %cap%))
    set lap 1
    while %lap% <= %loops%
      mexp %actor% %cap%
      eval lap %lap% + 1
    done
    mexp %actor% %diff%
    switch %actor.class%
      case paladin
      case anti-paladin
      case ranger
      case warrior
      case monk
      case berserker
      case paladin
        set direction the Warrior's Guild
        set direction2 south&_ south&_ south&_ east&_ east&_ north&_ east&_ north&_
        set master Warrior Coach
        break
      case rogue
      case thief
      case mercenary
      case assassin
      case bard
        set direction the Rogue's Guild
        set direction2 south&_ south&_ south&_ west&_ south&_ east&_ down&_
        set master the Master Rogue
        break
      case sorcerer
      case cryomancer
      case pyromancer
      case necromancer
      case illusionist
        set direction the Mage's Guild
        set direction2 south&_ south&_ south&_ west&_ west&_ south&_ south&_ east&_
        set master the Archmage
        break
      case cleric
      case priest
      case druid
      case diabolist
        set direction the Cleric's Guild
        set direction2 west&_ north&_
        set master the High Priestess
        break
      default
        say Oops, you broke, find a god.
        halt
    done
    wait 2s
    msend %actor% %self.name% tells you, 'Now that you're ready to level, it's time to find your Guild Master!
    msend %actor% They are waiting in your guild's Inner Sanctum, just beyond the entrance room to your guild.'
    wait 2s
    msend %actor% %self.name% tells you, '&6&bLook&0 at the &6&bmap&0 in your inventory.
    msend %actor% It will show you a layout of town.'
    wait 2s
    msend %actor% %self.name% tells you, '&2&bEast&0 is the Forest Temple of Mielikki.
    msend %actor% Your guild is &5&b%direction%&0.
    msend %actor% From the Temple of Mielikki it is:
    msend %actor% &b&3 %direction2%&0
    msend %actor% Your Guild Master is &5&b%master%&0.
    msend %actor% &0 
    msend %actor% Type &6&bLEVEL&0 when you find them to advance to the next level.
    msend %actor% You won't be able to gain any more experience until you do!
    msend %actor% If you ever forget anything you can always consult the &6&bHELP&0 file.'
    wait 3s
  endif
  msend %actor% %self.name% tells you, 'Your Guild Master here in the Town of Mielikki will grant you your first quest.
  msend %actor% They will ask you for &3&bgems&0 and &3&barmor&0 which drop randomly from creatures across the world.
  msend %actor% Bring back what they ask for and you'll be well rewarded!'
  wait 4s
  msend %actor% %self.name% tells you, 'Congratulations again, graduate!  May you climb to ever greater fortune!'
endif
~
#51974
academy_classes_command_toggle_split~
0 c 100
toggle~
switch %cmd%
  case t
    return 0
    halt
done
if %actor.quest_variable[school:loot]% == 2
  switch %arg%
    case autol
    case autolo
    case autoloo
    case autoloot
      quest variable school %actor% loot 3
      mforce %actor% toggle autoloot
      wait 2
      msend %actor% %self.name% tells you, 'Grand.'
      wait 1s
      msend %actor% %self.name% tells you, 'When playing with others it's considered polite to share the wealth.
      msend %actor% You can share money with the &6&bSPLIT&0 command.
      msend %actor% With &6&bTOGGLE AUTOSPLIT&0 on the game will automatically do that when you pick up money.'
      wait 2s
      msend %actor% %self.name% tells you, 'Type &2&btoggle autosplit&0 to set that.'
  done
elseif %actor.quest_variable[school:loot]% == 3
  switch %arg% 
    case autos
    case autosp
    case autospl
    case autospli
    case autosplit
      quest variable school %actor% loot 4
      mforce %actor% toggle autosplit
      wait 2
      msend %actor% %self.name% tells you, 'Now you'll play nice with other people!'
      wait 1s
      msend %actor% %self.name% tells you, 'The last combat tip is checking your &6&b(TRO)PHY&0 list.
      msend %actor% &6&bTROPHY&0 shows a record of the last 24 creatures you've killed and how many times you've killed them.'
      wait 2s
      msend %actor% %self.name% tells you, 'Type &2&btrophy&0 and see your record.'
  done 
endif
return 0
~
#51975
academy_recruitor_speech_ready~
0 d 100
ready~
if %actor.quest_stage[school]% == 1
  wait 2
  unlock gates
  open gates
  msend %actor% %self.name% tells you, 'Your first lesson is with the Instructor of Adventure.'
  msend %actor% %self.name% escorts you into the Academy.
  msend %actor% %self.name% tells you, 'You can say &5EXIT&0 at any time to leave.'
  wait 2s
  mforce %actor% east
  close gates
  lock gates
endif
~
#51976
academy_revel_command_stand~
0 c 100
stand~
switch %cmd%
  case s
    return 0
    halt
done
if %actor.quest_variable[school:rest]% == 4
  quest variable school %actor% rest 5
  mforce %actor% stand
  wait 2
  msend %actor% %self.name% tells you, 'Now you can &2&bfill waterskin fountain&0.'
endif
return 0
~
#51977
academy_class_blocker~
0 c 100
east~
if %actor.quest_stage[school]% < 5
  msend %actor% %self.name% tells you, 'You're not quite ready to move on yet!'
else
  mforce %actor% e
endif
~
#51978
academy exit~
0 d 100
exit exit?~
if %actor.vnum% == -1
  wait 2
  nod %actor%
  msend %actor% %self.name% tells you, 'Certainly.  Come back any time.'
  mteleport %actor% 3001
  msend %actor% %self.name% escorts you out of the Academy.
  wait 2
  mat 3001 mforce %actor% look
endif
~
#51979
academy_monster_death~
0 f 100
~
set fight %actor.quest_variable[school:fight]%
if %fight% == last
  quest variable school %actor% fight complete
  quest advance school %actor%
else
  if %get.mob_count[51900]% == 0
    mload mob 51900
    mecho Another horrible monster appears!
  endif
endif
~
#51980
academy_recruiter_speech_resume~
0 d 100
resume~
wait 2
if %actor.has_completed[school]% || !%actor.quest_stage[school]%
  halt
else
  switch %actor.quest_stage[school]%
    case 1
      set lesson &3&bCOMMUNICATION&0, &3&bGEAR&0, and &3&bEXPLORATION&0
      set holding 51900
      set direction east
      unlock gates
      open gates
      break
    case 2
      set lesson &3&bHIT POINTS&0 and &3&bSCORE&0
      set holding 51909
      set direction east
      break
    case 3
    case 4
      if %actor.quest_stage[school]% == 3
        set lesson &3&bCOMBAT&0
      elseif %actor.quest_stage[school]% == 4
        set lesson &3&bLOOT&0 and &3&bTOGGLES&0
      endif
      switch %actor.class%
        case rogue
        case thief
        case assassin
        case mercenary
        case bard
          set direction down
          break
        case sorcerer
        case cryomancer
        case pyromancer
        case necromancer
        case illusionist
          set direction south
          break
        case cleric
        case priest
        case diabolist
        case druid
          set direction east
          break
        case warrior
        case paladin
        case anti-paladin
        case ranger
        case monk
        case berserker
          set direction north
          break
        default
          msend %actor% %self.name% tells you, 'I don't know how you got to this point.  Please find a god!'
          halt
      done
      set holding 51908
      break
    case 5
      set lesson &3&bRESTING&0 and &3&bMONEY&0
      set holding 51910
      set direction east
      break
    case 6
      set lesson &3&bLEVELING&0
      set holding 51910
      set direction east
  done
  msend %actor% %self.name% tells you, 'You are taking lessons on %lesson%.
  msend %actor% Tutorial commands will appear &2&bin green text&0.
  msend %actor% Type those phrases &2&bexactly&0 as they appear to advance.
  msend %actor% Remember, &2&bspelling matters!!!&0.'
  msend %actor% %self.name% escorts you into the Academy.
  msend %actor% &0 
  msend %actor% %self.name% tells you, 'You can say &5EXIT&0 at any time to leave.'
  mteleport %actor% %holding%
  mat %holding% mforce %actor% %direction%
  if %actor.quest_stage[school]% == 1
    close gates
    lock gates
  endif
endif
~
#51981
academy_instructor_speech_communication~
0 d 100
communication~
wait 2
if %actor.quest_stage[school]% == 1
  if %actor.quest_variable[school:speech]% != complete
    if (%actor.quest_variable[school:gear]% && %actor.quest_variable[school:gear]% != complete) || (%actor.quest_variable[school:explore]% && %actor.quest_variable[school:explore]% != complete)
      msend msend %actor% %self.name% tells you, 'You have to finish your other lesson first.'
      halt
    endif
    if %actor.quest_variable[school:speech]%
      msend %actor% %self.name% tells you, 'Let's resume your &3&bCOMMUNICATION&0 lessons.'
      wait 1s
    endif
    switch %actor.quest_variable[school:speech]%
      case 2
        msend %actor% %self.name% tells you, 'You can talk to one person at a time by using the &6&b(T)ELL&0 command.
        msend %actor% Only the person you TELL to will hear what you say.
        msend %actor% You can talk to anyone, anywhere in the world.
        msend %actor% You do it by typing &6&btell [person] [message]&0.'
        wait 2s
        msend %actor% %self.name% tells you, 'Try typing &2&btell instructor hello teacher&0.'
        break
      case 3
        msend %actor% %self.name% tells you, 'Mass communication channels include:
        msend %actor%  
        msend %actor% &0&6&b(GOS)SIP&0 which talks to everyone in the game at once.
        msend %actor% &0   It's definitely the most popular communication channel.
        msend %actor% &0   It's a great way to ask questions or get help.
        msend %actor%  
        msend %actor% &0&6&b(SH)OUT&0 which talks to everyone in your zone.
        msend %actor%  
        msend %actor% &0&6&b(PETI)TION&0 which talks to all the Gods currently online.
        msend %actor% &0   It's best for asking semi-private or admin questions.
        msend %actor%  
        msend %actor% &0&6&bGROUPSAY&0 or &6&bGSAY&0 talks to everyone you're grouped with.
        msend %actor% &0   Check &6&bHELP GROUP&0 for more information on grouping.
        msend %actor%   
        msend %actor% You can also use &6&bLASTGOS&0 and &6&bLASTTEL&0 to see what the last messages you received were.
        msend %actor%   
        msend %actor% To see who's logged on, use the &6&bWHO&0 command.'
        wait 2s
        msend %actor% %self.name% tells you, 'Do you have any questions about communication?
        msend %actor% You can &2&bsay yes&0 or &2&bsay no&0.'
        break
      case 1
      default
        quest variable school %actor% speech 1
        msend %actor% %self.name% tells you, 'The most basic communication channel is the &6&b(SA)Y&0 command.
        msend %actor% It sends your message to everyone in the room.
        msend %actor% You'll need to use it to complete your lessons here.'
        wait 2s
        msend %actor% %self.name% tells you, 'Try it out.  &2&bSay hello class&0.
        msend %actor% I'm listening for you to say exactly that, no extra words or punctuation.'
    done
  endif
endif
~
#51982
academy_instructor_speech_inventory~
0 d 100
gear~
wait 2
if %actor.quest_stage[school]% == 1
  if %actor.quest_variable[school:gear]% != complete
    if (%actor.quest_variable[school:speech]% && %actor.quest_variable[school:speech]% != complete) || (%actor.quest_variable[school:explore]% && %actor.quest_variable[school:explore]% != complete)
      msend msend %actor% %self.name% tells you, 'You have to finish your other lesson first.'
      halt
    endif
    if %actor.quest_variable[school:gear]%
      msend %actor% %self.name% tells you, 'Let's resume your &3&bGEAR&0 lessons.'
    endif
    switch %actor.quest_variable[school:gear]%
      case 2
        msend %actor% %self.name% tells you, 'In order to gain benefits from items, you have to equip them.'
        wait 1s
        msend %actor% %self.name% tells you, 'There are three commands to equip items:
        msend %actor% &6&b(WEA)R&0, &6&b(WI)ELD&0, and &6&b(HO)LD&0.'&0
        msend %actor% 
        msend %actor% &6&bWEAR&0 will equip something from your inventory.
        msend %actor% You can equip most objects by typing &6&bWEAR [object]&0.
        msend %actor% Weapons can be equipped with either &6&bWEAR&0 or &6&bWIELD&0.
        msend %actor% &6&bWEAR ALL&0 will equip everything in your inventory at once.
        msend %actor% 
        msend %actor% Some items can only be equipped with the &6&bHOLD&0 command.
        msend %actor% That includes instruments, wands, staves, magic orbs, etc.
        msend %actor% They will not be equipped with the &6&bWEAR ALL&0 command.
        msend %actor% Once you are holding them they can be activated with the &6&bUSE&0 command.
        wait 3s
        msend %actor% %self.name% tells you, 'Go ahead and type &2&bwear all&0 and see what happens.'
        break
      case 3
        msend %actor% %self.name% tells you, 'The &6&b(EQ)UIPMENT&0 command shows what gear you're using.
        msend %actor% You are gaining active benefits from these items.
        msend %actor% They will not show up in your inventory.'
        wait 3s
        msend %actor% %self.name% tells you, 'Type &2&bequipment&0 or just &2&beq&0 to try it out.'
        break
      case 4
        msend %actor% %self.name% tells you, 'Another way to equip things is with the &6&b(HO)LD&0 command.
        msend %actor% Here, take this torch for example.'
        load obj 1005
        give torch %actor%
        wait 3s
        msend %actor% %self.name% tells you, 'Type &2&bhold torch&0 to equip it.'
        break
      case 5
        msend %actor% %self.name% tells you, 'The &6&b(LI)GHT&0 command turns lights on and off.
        msend %actor% You can just type &6&blight torch&0 to light it up.
        msend %actor% If it's already lit, you can type &6&blight torch&0 again to extinguish it.'
        wait 3s
        osend %actor% %self.name% tells you, 'Give it a try and type &2&blight torch&0.'&0
        break
      case 6
        msend %actor% %self.name% tells you, 'Remember, most lights have a limited duration.
        msend %actor% It's best to turn them off when not using them.
        msend %actor% &0 
        msend %actor% Lights don't have to be equipped for you to see.
        msend %actor% They work just fine from your inventory.'
        wait 3s
        msend %actor% %self.name% tells you, 'You can stop using items by typing &6&bREMOVE [object]&0.'
        msend %actor%    
        msend %actor% %self.name% tells you, 'Stop wearing that torch by typing &2&bremove torch&0.'
        break
      case 7
        msend %actor% %self.name% tells you, 'During your adventures, you can pick up stuff using the &6&b(G)ET&0 command.'
        wait 1s
        mload obj 51902
        mload obj 51902
        mload obj 51902
        mload obj 51902
        mload obj 51902
        drop all.stick
        msend %actor% %self.name% tells you, 'Pick up one of those sticks by typing &2&bget stick&0.'
        break
      case 8
        mload obj 51902
        mload obj 51902
        mload obj 51902
        mload obj 51902
        mload obj 51902
        drop all.stick
        msend %actor% %self.name% tells you, 'If you don't want to pick up the first one, you can target a different one by adding a number and a "." before the name.'
        wait 2s
        msend %actor% %self.name% tells you, 'Try typing &2&bget 2.stick&0 and see what happens.'
        break
      case 9
        mload obj 51902
        mload obj 51902
        mload obj 51902
        mload obj 51902
        mload obj 51902
        drop all.stick
        msend %actor% %self.name% tells you, 'You can pick up all of one thing by typing &6&bGET all.[object]&0.
        msend %actor% Or you can be extra greedy by typing &6&bGET ALL&0.
        msend %actor% That will pick up everything in the room.'
        wait 3s
        msend %actor% %self.name% tells you, 'Type &2&bget all&0 with nothing after it.'
        break
      case 10
        mload obj 51902
        mload obj 51902
        mload obj 51902
        mload obj 51902
        mload obj 51902
        give all.stick %actor%
        msend %actor% %self.name% tells you, 'Now it's possible your inventory might get full.
        msend %actor% You can only carry so many items in your inventory at once but there are a few ways to deal with that.'
        wait 3s
        msend %actor% %self.name% tells you, 'First, you can &6&b(DRO)P&0 items with the command &6&bDROP [object]&0.'
        msend %actor% &0 
        msend %actor% %self.name% tells you, 'Drop one of those sticks by typing &2&bdrop stick&0.'
        break
      case 11
        msend %actor% %self.name% tells you, 'You can permanently destroy objects with the &6&b(J)UNK&0 command.'&0
        wait 2s
        msend %actor% %self.name% tells you, 'Try junking a stick by typing &2&bjunk stick&0.'&0
        break
      case 12
        msend %actor% %self.name% tells you, 'Another way to deal with items is to &6&b(GI)VE&0 them away.
        msend %actor% You can do that by typing &6&bGIVE [object] [person]&0.'
        wait 2s
        msend %actor% %self.name% tells you, 'Give me a stick by typing &2&bgive stick instructor&0.'
        break
      case 13  
        msend %actor% %self.name% tells you, 'You can &6&b(P)UT&0 objects in containers.
        msend %actor% The command is &6&bPUT [object] [container]&0.'
        wait 2s
        msend %actor% %self.name% tells you, 'You started with a bag.
        msend %actor% Put a stick in it by typing &2&bput stick bag&0.'
        break
      case 14
        msend %actor% %self.name% tells you, 'To see what's inside something, use &6&b(EXA)MINE [target]&0.
        msend %actor% &6&bEXAMINE&0 will also show if something is open or closed.'
        wait 2s
        msend %actor% %self.name% tells you, 'Go ahead and &2&bexamine bag&0 and see what you find.'
        break
      case 15
        msend %actor% %self.name% tells you, 'You can type &6&bGET [object] [container]&0 to take one thing out of a container, or &6&bGET ALL [container]&0 to get everything out.'
        wait 2s
        msend %actor% %self.name% tells you, 'Take a stick out of a bag by typing &2&bget stick bag&0.'
        break
      case 16
          msend %actor% %self.name% tells you, 'Would you like to review equipment management?
          msend %actor% You can &2&bsay yes&0 or &2&bsay no&0.'
        break
      case 1
      default
        quest variable school %actor% gear 1
        msend %actor% %self.name% tells you, 'Then let's talk about &3&bGEAR&0!'
        wait 1s
        msend %actor% %self.name% tells you, 'Your &6&b(I)NVENTORY&0 is all the items you're currently carrying but not actively wearing or using.
        msend %actor% You do not gain any bonuses from items in your inventory.'
        wait 2s
        msend %actor% %self.name% tells you, 'Type &2&binventory&0 or just &2&bi&0 to check what you currently have.
        msend %actor% Go ahead and try it out!'&0
    done
  endif
endif
~
#51983
academy_instructor_speech_exploration~
0 d 100
exploration~
wait 2
if %actor.quest_stage[school]% == 1
  if %actor.quest_variable[school:explore]% != complete
    if (%actor.quest_variable[school:speech]% && %actor.quest_variable[school:speech]% != complete) || (%actor.quest_variable[school:gear]% && %actor.quest_variable[school:gear]% != complete)
      msend msend %actor% %self.name% tells you, 'You have to finish your other lesson first.'
      halt
    endif
    if %actor.quest_variable[school:explore]%
      msend %actor% %self.name% tells you, 'Let's resume your &3&bEXPLORATION&0 lessons.'
    endif
    switch %actor.quest_variable[school:explore]%
      case 2
        msend %actor% %self.name% tells you, 'As you look around the room, you'll get four big pieces of information:'
        msend %actor% &0 
        msend %actor% &0&5&b1. The description of the room.&0
        msend %actor% &0   Tiny but crucial hints can appear in room descriptions - keep your eyes open!
        msend %actor% &0 
        msend %actor% &0&5&b2. The visible exits from the room.&0
        msend %actor% &0   FieryMUD uses six directions: north, south, east, west, up, and down.
        msend %actor% &0   If an exit is visible, it will appear after a hyphen.
        msend %actor% &0   Closed exits like doors and trapdoors will have a # sign after them.
        msend %actor% &0 
        msend %actor% &5&b3. Any visible objects.&0
        msend %actor% &0   By default, all identical objects will appear on one line with a number on the left showing how many there are.
        msend %actor% &0 
        msend %actor% &0&5&b4. All visible creatures.&0
        msend %actor% &0   All identical creatures will appear as one line.
        msend %actor% &0 
        msend %actor% You can &6&bLOOK&0 at anything to gain more information about it.
        msend %actor% That can be objects, creatures, even the directions!
        wait 3s
        msend %actor% %self.name% tells you, 'For example, type &2&blook curtain&0 to find more clues.'
        break
      case 3
        msend %actor% %self.name% tells you, 'Throughout the world there are hundreds of hidden doors.
        msend %actor% There can be hints in the room description, or you can try to find them by typing &6&bLOOK [direction]&0.
        msend %actor% &0 
        msend %actor% To interact with the door, you have to &6&b(SEA)RCH&0 for it first.
        msend %actor% &0 
        msend %actor% If you do know what you're looking for, you can type &6&bSEARCH [keyword]&0 to find it automatically.
        msend %actor% You might be able to guess the keywords from what you see when you look at things or in directions!
        msend %actor% &0 
        msend %actor% If you don't know the keyword, you can just enter &6&bSEARCH&0.
        msend %actor% You'll have a random chance to find any hidden doors.
        msend %actor% &0 
        msend %actor% Be aware!  There is a small stun time after you &6&bSEARCH&0!
        msend %actor% It can be very risky to use it in dangerous areas!'
        wait 3s
        msend %actor% %self.name% tells you, 'But for now, type &2&bsearch curtain&0 and take a look!'
        break
      case 4
        mforce %actor% search curtain
        msend %actor% %self.name% tells you, 'Once you've uncovered a secret door you can interact with it like a normal door.
        msend %actor% You can use the &6&b(O)PEN&0 and &6&b(CL)OSE&0 commands to open doors or containers.
        msend %actor% Sometimes they might be locked though, and you'll need a key to unlock them.
        msend %actor% &0 
        msend %actor% If you &6&bLOOK EAST&0, you can see the curtain from this room is closed.
        msend %actor% To open it just use the &6&b(O)PEN&0 command.'&0
        wait 3s
        msend %actor% %self.name% tells you, 'Type &2&bopen curtain&0 to see how it works.'
        break
      case 5
        msend %actor% %self.name% tells you, 'You can &6&b(SCA)N&0 to see what's in the areas around you.
        msend %actor% It's free to use and very helpful for anticipating threats.
        msend %actor% Some classes are even able to see more than one room away.
        msend %actor% There is a slight delay after giving the &6&bSCAN&0 command, so be careful.'
        wait 3s
        msend %actor% %self.name% tells you, 'Give it a go!  Type &2&bscan&0.'
        break
      case 6
        msend %actor% %self.name% tells you, 'You can move in any direction there's an open exit.
        msend %actor% Just type &6&b(N)ORTH (S)OUTH (E)AST (W)EST (U)P&0 or &6&b(D)OWN&0.
        msend %actor% &0 
        msend %actor% In the lower-left corner of your screen you'll see a display that looks like this: 
        msend %actor% &3&b10h(10H) 100v(100V)&0
        msend %actor% The two numbers on the right are your &5&bMovement Points&0.
        msend %actor% The first number is your &5&bCurrent Movement Points&0.  
        msend %actor% The second number is your &5&bMaximum Movement Points&0.
        msend %actor% Your &5&bMaximum&0 will increase until level 50.
        msend %actor% &0 
        msend %actor% When you move from room to room, your Movement Points go down.
        msend %actor% The amount of Movement Points needed to move around varies by terrain.
        msend %actor% If your Movement Points reach 0 you can't move until you rest.'
        wait 3s
        msend %actor% %self.name% tells you, 'Would you like to review exploration?
        msend %actor% You can &2&bsay yes&0 or &2&bsay no&0.'
        break
      case 1
      default
        quest variable school %actor% explore 1
        msend %actor% %self.name% tells you, 'Let's learn how to &3&bEXPLORE&0!'
        wait 1s
        msend %actor% %self.name% tells you, 'First things first. It's critical to understand your surroundings.
        msend %actor% Your basic investigation command is &6&b(L)OOK&0.
        msend %actor% For now, just try typing &2&blook&0 and see what you find.'
    done
  endif
endif
~
#51984
academy_torch_wear~
1 j 100
~
if %actor.quest_variable[school:gear]% == 4
  quest variable school %actor% gear 5
  wait 2
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'Notice you have a torch equipped now.
  osend %actor% However, you need to turn it on before it provides you any light.
  osend %actor% The &6&b(LI)GHT&0 command turns lights on and off.
  osend %actor% You can just type &6&blight torch&0 to light it up.
  osend %actor% If it's already lit, you can type &6&blight torch&0 again to extinguish it.'
  wait 3s
  osend %actor% %get.mob_shortdesc[51902]% tells you, 'Give it a try and type &2&blight torch&0.'
endif
~
#51985
academy_instructor_speech_skip~
0 d 100
skip~
wait 2
if %actor.quest_stage[school]% == 1
  quest advance school %actor%
  quest variable school %actor% speech complete
  quest variable school %actor% gear complete
  quest variable school %actor% exploration complete
  msend %actor% %self.name% tells you, 'Certainly.  You'll continue to the east.'
  mforce %actor% search curtain
  open curtain
  mforce %actor% east
endif
~
#51986
academy_clerk_speech_hit_points~
0 d 0
hit points~
wait 2
if %actor.quest_stage[school]% == 2
  quest variable school %actor% hp complete
  msend %actor% %self.name% tells you, 'Look at the prompt in the lower-left corner of your screen again.
  msend %actor% The two numbers on the left are your &1&bHit Points&0.
  msend %actor% The first number is your &1&bCurrent Hit Points&0.
  msend %actor% The second number is your &1&bMaximum Hit Points&0.
  msend %actor% Your &1&bMaximum&0 increases every time you level.
  msend %actor% &0  
  msend %actor% When you get hit in combat, your current Hit Points go down.  
  msend %actor% If your Hit Points reach 0, you are knocked unconscious.
  msend %actor% If your Hit Points reach -10, you die.
  wait 1s
  msend %actor% %self.name% tells you, 'Are you ready to continue?
  msend %actor% You can &2&bsay yes&0 or &2&bsay no&0.'
endif
~
#51987
academy_clerk_speech_score~
0 d 100
score~
if %actor.quest_stage[school]% == 2
  quest variable school %actor% score 1
  wait 2
  msend %actor% %self.name% tells you, '&6&bSCORE&0 is how you see all the numeric stuff about yourself:
  msend %actor% Experience, Hit and Movement Points, Stats, Saves, blah blah blah.'
  wait 2s
  msend %actor% %self.name% tells you, 'Check it out by typing &2&bscore&0 now.'
endif
~
#51988
academy_clerk_speech_yes_no_skip~
0 d 100
yes no skip~
wait 2
if %actor.quest_stage[school]% == 2
  if %speech% == yes
    if %actor.quest_variable[school:score]% == complete && %actor.quest_variable[school:hp]% == complete
      set advance yes
    else
      set advance no
    endif
  elseif %speech% == skip
    set advance yes
  elseif %speech% == no
    msend %actor% %self.name% tells you, 'What would you like to review?  You can say:
    msend %actor% &3&bHit Points&0
    msend %actor% &3&bScore&0&_
    msend %actor% %self.name% tells you, &2&bSay continue&0 when you're ready to move on.'
  endif
  if %advance% == yes
    quest advance school %actor%
    eye %actor%
    wait 1s
    msend %actor% %self.name% considers your capabilities...
    wait 3s
    msend %actor% %self.name% tells you, 'Hmmmm...'
    wait 2s
    switch %actor.class%
        case rogue
        case thief
        case assassin
        case mercenary
        case bard
          msend %actor% %self.name% tells you, 'I see you're a stealthy type.  You'll do best in lessons with Doctor Mischief.  Proceed &2&bdown&0 to their classroom.'
          break
        case sorcerer
        case cryomancer
        case pyromancer
        case necromancer
        case illusionist
          msend %actor% %self.name% tells you, 'I see you're an arcane spell caster.  You would definitely benefit from the Chair of Arcane Studies' seminar on spellcasting.  Proceed &2&bsouth&0 to his laboratory.'
          break
        case cleric
        case priest
        case diabolist
        case druid
          msend %actor% %self.name% tells you, 'I see you're a divine spell caster.  You would definitely benefit from private classes with the Professor of Divinity.  Proceed &2&beast&0 to his chapel.'
          break
        case warrior
        case paladin
        case anti-paladin
        case ranger
        case monk
        case berserker
          msend %actor% %self.name% tells you, 'I see you're a fighter type.  You'll do best learning from the Academy's Warmaster.  Proceed &2&bnorth&0 to her arena.'
          break
        default
          msend %actor% %self.name% tells you, 'I have no idea what to do with you.  Find a god and ask for help!'
    done
  elseif %advance% == no
    if %actor.quest_variable[school:score]% == complete
      msend %actor% %self.name% tells you, 'Let's talk about &3&bHIT POINTS&0 next.
      msend %actor% &2&bSay hit points&0 to begin.'
    elseif %actor.quest_variable[school:hp]% == complete
      msend %actor% %self.name% tells you, 'Let's talk about &3&bSCORE&0 next.
      msend %actor% &2&bSay score&0 to begin.'
    endif
  endif
endif
~
#51989
academy_combat_speech_skip~
0 d 100
skip~
wait 2
if %actor.quest_stage[school]% == 3
  quest variable school %actor% fight complete
  quest advance school %actor%
  msend %actor% %self.name% tells you, 'Certainly.  We'll skip combat and talk about &3&bLOOT&0 and &3&bTOGGLES&0.
  mload mob 51900
  kill monster
  msend %actor% &2&bSay loot&0 when you're ready to continue.'
elseif %actor.quest_stage[school]% == 4
  quest variable school %actor% loot complete
  quest advance school %actor%
  msend %actor% %self.name% tells you, 'Certainly.  You'll continue to the &2&beast&0.'
endif
~
#51990
academy_wear_pen~
1 j 100
~
if %actor.quest_variable[school:fight]% == 4
  quest variable school %actor% fight 5
  wait 2
  osend %actor% %get.mob_shortdesc[51906]% tells you, 'Wonderful, you're all set!
  osend %actor% Now you can &6&bSCRIBE&0 any spell on your spell list.
  osend %actor% Each spell takes up pages in your spellbook.
  osend %actor% The number of pages is affected by your &6&bSCRIBE&0 skill.'
  wait 1s
  osend %actor% %get.mob_shortdesc[51906]% tells you, 'You can write any spell you know in your spellbook as long as you're with your Guild Master and your spellbook enough blank pages left in it.'
  wait 2s
  osend %actor% %get.mob_shortdesc[51906]% tells you, 'For now, just type &2&bscribe magic missile&0.'
endif
~
#51991
academy_revel_speech_resting~
0 d 100
resting~
wait 2
if %actor.quest_stage[school]% == 5
  switch %actor.quest_variable[school:rest]%
    case 1
      msend %actor% %self.name% tells you, 'If you want to speed up your recovery, you can lay down and go to &6&bSLEEP&0.
      msend %actor% There're some risks with sleeping.
      msend %actor% You won't be able to hear or see the world around you, so you won't know if danger is approaching.
      msend %actor% You won't be able to use &6&bSAY&0 or &6&bSHOUT&0, or do anything most people can't normally do in their sleep.
      msend %actor% You can still &6&bGOSSIP&0, &6&bTELL&0, and check your &6&bINVENTORY&0 or &6&bEQUIPMENT&0 though!'
      msend %actor% &0 
      msend %actor% %self.name% tells you, 'Take a quick nap!  Type &2&bsleep&0 to lay down.'
      break
    case 2
      quest variable school %actor% rest 3
    case 3
      msend %actor% %self.name% tells you, 'You'll frequently notice yourself getting hungry or thirsty.
      msend %actor% You can speed up your recovery if you &6&bEAT&0 and &6&b(DRI)NK&0.
      msend %actor% &0 
      msend %actor% When you get thirsty it's time to &6&bDRINK&0!  
      msend %actor% Ethilien has a huge variety of drinkable liquids.
      msend %actor% Any of them will slake your thirst.
      msend %actor% The command is &6&bDRINK [container]&0.
      msend %actor% &0 
      msend %actor% When you &6&bDRINK&0, you'll also regain some Movement Points, so make sure you carry a full waterskin so you can keep moving when you explore!
      msend %actor% &0 
      msend %actor% And remember, magic potions are &1not&0 drinks.
      msend %actor% If you want to consume a potion, the commmand is &6&b(Q)UAFF&0, not &6&b(DRI)NK&0.'
      wait 3s
      if !%actor.inventory[20]% && !%actor.wearing[20]%
        msend %actor% %self.name% tells you, 'Here's a new waterskin for you.
        mload obj 20
        give waterskin %actor%
      else
        msend %actor% %self.name% tells you, 'You started play with a full waterskin.
      endif
      msend %actor% &0Type &2&bdrink waterskin&0 now to drink out of it.'
      break
    case 4
      quest variable school %actor% rest 5
    case 5
      msend %actor% %self.name% tells you, 'Now you can &2&bfill waterskin fountain&0.'
      break
    case 6
      msend %actor% %self.name% tells you, 'Now we feast!'
      wait 2s
      mload obj 20305
      give meat %actor.name%
      wait 2s
      msend %actor% %self.name% tells you, 'Ethilien is filled with edible foods.
      msend %actor% &0You can &6&bEAT&0 your way across the world! Being full increases your regeneration rate.
      msend %actor% Every time you &6&bEAT&0 you also immediately regain some Hit Points!
      msend %actor% Food has no other effects though.'
      msend %actor% &0
      msend %actor% %self.name% tells you, 'Come, feast with me!  Type &2&beat meat&0!'
      mload obj 20305
      break
    case 7
      msend %actor% %self.name% tells you, 'There are just a few final things to know before you head out into the world.'
      wait 2s
      msend %actor% %self.name% tells you, 'The best place to start your adventures is the &3&bFARMLAND&0 immediately west of the town of Mielikki, the starting town above us.'
      wait 2s
      msend %actor% %self.name% tells you, 'The &6&bHELP ZONE&0 file will show you lots of places to explore.
      msend %actor% Type &2&bhelp zone&0 and check it out.'
      break
    case complete
      msend %actor% %self.name% tells you, 'When you're ready to stop playing, to safely log out, you have to find an inn.
      msend %actor% Talk to the inn's receptionist and &6&bRENT&0 a room. 
      msend %actor% Think of inns like save points.
      msend %actor% Renting stores all your items for free.
      msend %actor% When you log back in, you'll be at the inn.'
      msend %actor% &0 
      if %actor.quest_variable[school:money]% == complete && %actor.quest_variable[school:rest]% == complete
        msend %actor% %self.name% tells you, '&2&bSay finish&0 to end the school.'
      else
        msend %actor% %self.name% tells you, '&2&bSay money&0 to finish your last lesson, or say &5SKIP&0 to jump to the end of the Academy.'
      endif
      break
    default
      msend %actor% %self.name% tells you, 'Hit Points and Movement Points regenerate naturally over time.
      msend %actor% The rate is influenced by a number of factors, including position and nutrition.'
      wait 2s
      msend %actor% %self.name% tells you, 'You might not realize you're healing though, because your display only updates when you get some kind of input!'
      wait 1s
      msend %actor% %self.name% tells you, 'Your regeneration speeds up if you &6&b(R)EST&0 or &6&b(SL)EEP&0.
      msend %actor% When you &6&bREST&0, you sit down and relax.
      msend %actor% You remain awake, and can hear and see the world around you.
      msend %actor% You can do most things, including talking, eating, changing gear, and recovering spells.'
      wait 2s
      msend %actor% %self.name% tells you, 'Take a load off!
      msend %actor% Type &2&brest&0 to relax.'
  done
endif
~
#51992
academy_revel_speech_money~
0 d 100
money~
wait 2
if %actor.quest_stage[school]% == 5 && !%actor.quest_variable[school:money]%
  quest variable school %actor% money 1
  msend %actor% %self.name% tells you, 'You can &6&bGET&0, &6&bDROP&0, and &6&bGIVE&0 money like items.
  msend %actor% To check your current wealth, use &6&bSCORE&0.'
  wait 2s
  msend %actor% %self.name% tells you, 'During your downtime you may want to visit a bank.
  msend %actor% It's important to keep money in the bank so that when you die, you'll have emergency funds to get back to your corpse!
  msend %actor% &0  
  msend %actor% I myself happen to be a certified banker! 
  msend %actor% &0 
  msend %actor% At a bank you can &6&bDEPOSIT&0 and &6&bWITHDRAW&0 money.
  wait 3s
  give 5 gold 5 silver 5 copper %actor.name%
  wait 2s
  msend %actor% %self.name% tells you, 'Let's try out depositing money, like this:
  msend %actor% &6&bDEPOSIT [amount] platinum [amount] gold [amount] silver [amount] copper'
  wait 2s
  msend %actor% %self.name% tells you, 'Type &2&bdeposit 1 gold 1 silver 1 copper&0.'
endif
~
#51993
academy_revel_skip~
0 d 100
skip~
if %actor.quest_stage[school]% == 5
  wait 2
  quest variable school %actor% money complete
  quest variable school %actor% rest complete
  quest advance school %actor%
  msend %actor% %self.name% tells you, 'You're ready to graduate from Ethilien Academy!
  msend %actor% %self.name% says, &2&bSay finish&0 to end the school.'
endif
~
#51994
academy_bag_command_get~
1 c 7
get~
if %actor.quest_variable[school:gear]% == 15 && %arg% /= stick
  switch %arg%
    case b
    case ba
    case bag
      quest variable school %actor% gear 16
      oforce %actor% get stick bag
      wait 2
      cheer
      osend %actor% %get.mob_shortdesc[51902]% tells you, 'Congratulations, you mastered equipment management!
      osend %actor% Would you like to review?
      osend %actor% You can &2&bsay yes&0 or &2&bsay no&0.'
  done
endif
return 0
~
$~
