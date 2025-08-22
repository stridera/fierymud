#59000
build_mutter~
0 b 30
~
set val %random.10%
switch %val%
case 1
case 2
say Man, Rydack is a slave driver. 
case 3
case 4
emote starts using OLC.
break
case 5
case 6
emote looks at his watch.
break
default
emote looks around to see if anyone is watching.
done
~
#59001
mask_enmity_wear~
1 j 100
~
if (%actor.vnum% == -1)
   if %actor.align% > -350
      return 0
      wait 1
      oecho The mask of enmity starts to &9&bsmoke&0 violently! 
      wait 2 
      osend %actor% You feel the smoke dig into your skin and burn!
      oechoaround %actor% The smoke from the mask digs into %actor.name%'s skin and severely burns %actor.o%.  
      oechoaround %actor% %actor.name% rips the mask of enmity from %actor.p% face.
      osend %actor% You rip the mask from your face.
      odamage %actor% 50
   end
end
~
#59002
glasscase_break~
1 c 100
break~
* you must break glass to get reset sword out
if %arg% == glass || %arg% == case
   return 1
   * check to see what PC can break case with
   set item 0
   if %actor.worn[shield]%
      set item %actor.worn[shield]%
   elseif %actor.worn[wield2h]%
      set item %actor.worn[2hwield]%
   elseif %actor.worn[wield]% != -1
      set item %actor.worn[wield]%
   elseif %actor.worn[2wield]% != -1
      set item %actor.worn[wield2]%
   elseif %actor.worn[held]% != -1
      set item %actor.worn[held]%
   elseif %actor.worn[held2]% != -1
      set item %actor.worn[held2]%
   end
   * break case with item or hands
   if %item%
      osend %actor% You smash the glass with %item.shortdesc% and the dusty case shatters into small pieces on the ground.
      oechoaround %actor% %actor.name% smashes the glass with %item.shortdesc% and the dusty case shatters into small pieces on the ground.
   else
      odamage %actor% 150 slash
      if %damdone% != 0
         osend %actor% You smash the glass with your bare hands, shattering the glass and slicing your hands. (&3&b%damdone%&0)
         oechoaround %actor% %actor.name% smashes the glass with %actor.p% bare hands, shattering the glass and slicing deep grooves into %actor.p% hands. (&3&b%damdone%&0)
      else
         osend %actor% You smash the glass with your bare hands.
         oechoaround %actor% %actor.name% smashes the glass with %actor.p% bare hands.
      end
   end
   wait 1
   oecho A pristine iridescent sword falls out of the broken case, landing on the ground.
   unset vnum
   * destroy case and load it in rm 59091 to prevent it from loading here again, as it is a reset item
   oload obj 59024
   opurge dusty-glass-case
   oload obj 59024
else
   return 0
end
~
#59003
Rydack_test~
1 c 100
fire~
return 1
if %actor.worn[shield]% != -1
oecho ok, not wearing shield
elseif
set vnum %actor.worn[shield]%
oecho %vnum%
endif
unset vnum
~
#59004
glasscase_break_return0~
1 c 100
brea~
return 0
*returns normal value so nothing but break glass sets trigger off
~
#59005
dark_robed_greet1~
0 g 60
~
if %actor.vnum% == -1 && %actor.level% < 100
   if %actor.quest_stage[sacred_haven]% == 1 &%actor.quest_variable[sacred_haven:given_light]% != 1
      wait 6
      msend %actor% %self.name% whispers to you, 'Have you located the adornment of light
      msend %actor% &0for me?'
   elseif %actor.quest_stage[sacred_haven]% > 1 &(%actor.quest_variable[sacred_haven:given_blood]%
   elseif %actor.quest_stage[sacred_haven]% > 1 &(%actor.quest_variable[sacred_haven:given_blood]%
%actor.quest_variable[sacred_haven:given_earring]% != 1)
      wait 4
      whisper %actor.name% Have you brought me my artifacts?
      halt
   elseif (%actor.quest_stage[sacred_haven]% < 1)
      wait 4
      set room %get.room[%self.room%]%
      set target %room.people%
      while %target%
         if (%target.align% <= -350) && (%target.vnum% == -1) &(%target.level% < 100)
            whisper %target.name% Ah, I sense a wicked aura around your soul.
            wait 5
            msend %target% %self.name% slowly walks up and leans in close towards you.
            mechoaround %target% %self.name% slowly walks up and leans close towards %target.name%.
            wait 7
            msend %target% %self.name% whispers to you, 'I am willing to offer you a reward,
            msend %target% &0if only you can help me.'
            halt
         else
            set target %target.next_in_room%
         endif
      done
      if %target% &%target.align% > -350
         emote slowly turns away from you, pulling his hood farther down to cover his face.
      endif
   endif
endif
~
#59006
dark_robed_help~
0 d 100
help?~
set target %actor%
if %target.align% <=-350 && %target.level% <100
    wait 3
    if %target.room% == %self.room%
        msend %target% %self.name% squints their eyes and peers at you.
        mechoaround %target.name% %self.name% squints their eyes and peers at %target.name%.
        wait 6
        msend %target.name% %self.name% whispers to you, 'Are you strong enough to assist me?
        msend %target.name% &0I have a couple of allies that have breached the Sacred Haven and secured me
        msend %target.name% &0a key of great importance.'
        wait 2
        emote flashes a dingy key, held tight in their skinny, grey hands.
        wait 5
        msend %target% %self.name% whispers to you, 'Before I release my key I need you to
        msend %target% &0prove that you are worthy of this deed.  Do you feel strong enough to handle
        msend %target% &0this task?'
    endif
endif
~
#59007
dark_robed_yes~
0 d 100
yes~
wait 3 
set person %actor%
set i %person.group_size%
if %i%
   set a 1
else
   set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
      if %person.align% <= -350
        set continue yes
        if !%person.quest_stage[sacred_haven]%
            quest start sacred_haven %person.name%
            msend %person% &7&bYou have begun the Sacred Haven quest!&0
        endif
      endif
  elseif %person%
      eval i %i% + 1
  endif
  eval a %a% + 1
done
if %continue%
   msend %actor% %self.name% steps in closer towards you and rests his hand on your shoulder.
   mechoaround %actor% %self.name% steps in closer towards %actor.name% and rests its hand on %actor.p% shoulder.
   wait 6
   msend %actor% %self.name% tells you, 'Bring me an adornment of light from one of the sanctuary priests and I will be convinced that you are capable enough that my key will not be wasted.'
   wait 6
   msend %actor% %self.name% tells you, 'Ask me &7&b[what am I doing?]&0 if you forget.'
   wait 9
   mecho %self.name% says, 'Now be gone with you, and hurry before they destroy my artifacts.'
endif
~
#59008
dark_robed_recieve1~
0 j 100
~
* check to see what is being given!
set obj %object.vnum%
switch %obj%
   * adornment of light
   case 59026
      * check to see if quest is started and PC hasent given light already
      if (%actor.vnum% == -1) && (%actor.quest_stage[sacred_haven]% == 1) && (%actor.quest_variable[sacred_haven:given_light]% != 1)
         wait 5
         emote slowly rubs its hands together and cackles with a wicked sounding glee.
         mjunk %object.name%
         mload obj 59031
         wait 6
         give dingy-key %actor.name%
         msend %actor% %self.name% whispers to you, 'Here is a key that will help you finish
         msend %actor% &0my task.  Don't waste my efforts acquiring it.'
         wait 3
         msend %actor% %self.name% whispers to you, 'I have lost a few of my dark artifacts to the
         msend %actor% &0clutches of the brothers from the Sacred Haven.'
         wait 2
         emote grumbles loudly. 
         quest variable sacred_haven %actor.name% given_light 1
         * last line just set var to let code know light has been given
      elseif  %actor.quest_variable[sacred_haven:given_light]% == 1
         *return light if already given
         wait 2
         give adornment %actor.name%
         wait 3
         say You already gave me this!
         mecho %self.name% mumbles something about %actor.name%.
      else
         *junk it if quest hasn't started
         wait 6
         say I don't want this.
         mecho %self.name% throws %object.shortdesc% into the woods.
         mjunk %object.name%
      endif
      break
   &3&b* vial of blood&0
   case 59028
      if  %actor.quest_variable[sacred_haven:given_blood]% == 1
         wait 2
         say You have already given me this.
         give vial-dragons-blood %actor.name%
      elseif %actor.quest_stage[sacred_haven]% < 2 
         *junk it if quest isn't at stage 2 yet
         wait 6
         say I don't want this.
         mecho %self.name% throws %object.shortdesc% into the woods.
         mjunk %object.name%
      else
         wait 5
         grin
         wait 2
         say Good, now I have the vial of dragon's blood.
         mjunk vial-dragons-blood
         quest variable sacred_haven %actor.name%  given_blood 1
         *check to see if all 3 items have been returned
         if (%actor.quest_variable[sacred_haven:given_blood]% == 1) && (%actor.quest_variable[sacred_haven:given_trinket]% == 1) && (%actor.quest_variable[sacred_haven:given_earring]% == 1)
            set reward yes
         else
            set reward no
         endif
      endif
      break
   &3&b*trinket of tattered leather&0
   case 59029
      if  %actor.quest_variable[sacred_haven:given_trinket]% == 1
         wait 2
         say You have already given me this.
         give trinket-tattered-leather %actor.name%
      elseif %actor.quest_stage[sacred_haven]% < 2 
         *junk it if quest isn't at stage 2 yet
         wait 6
         say I don't want this.
         mecho %self.name% throws %object.shortdesc% into the woods.
         mjunk %object.name%
      else
         wait 5
         grin
         wait 3
         msend %actor% %self.name% says, 'Ahhh, I savor the feel of my skin against my trinket
         mecho &0of tattered leather.'
         mjunk %object.name%
         quest variable sacred_haven %actor.name% given_trinket 1
         *check to see if all 3 items have been returned
         if (%actor.quest_variable[sacred_haven:given_blood]% == 1) && (%actor.quest_variable[sacred_haven:given_trinket]% == 1) && (%actor.quest_variable[sacred_haven:given_earring]% == 1)
            set reward yes
         else
            set reward no
         endif
      endif
      break
   &3&b*earring&0
   case 59030
      if  %actor.quest_variable[sacred_haven:given_earring]% == 1
         wait 2
         say You have already given me this.
         give shadow-forged-earring %actor.name%
      elseif %actor.quest_stage[sacred_haven]% < 2 
         *junk it if quest isn't at stage 2 yet
         wait 6
         say I don't want this.
         mecho %self.name% throws %object.shortdesc% into the woods.
         mjunk %object.name%
      else
         wait 9
         mjunk %object.name%
         cackle
         wait 5
         msend %actor% %self.name% says, 'Yes... I have not seen the shadow forged earring in
         mecho &0quite some time.'
         quest variable sacred_haven %actor.name% given_earring 1
         *check to see if all 3 items have been returned
         if (%actor.quest_variable[sacred_haven:given_blood]% == 1) && (%actor.quest_variable[sacred_haven:given_trinket]% == 1) && (%actor.quest_variable[sacred_haven:given_earring]% == 1)
            set reward yes
         else
            set reward no
         endif
      endif
      break
   default
      *junk all other obj's
      wait 6
      say I don't want this.
      mecho %self.name% throws %object.shortdesc% into the woods.
      mjunk %object.name%
      break
done
if %reward% == yes
   *reward
   wait 3s
   look %actor.name%
   emote nods their head and slowly pulls down their hood revealing a grey face.
   wait 2s
   mecho %self.name% says, 'Now that I have both the earring and vial of blood,
   mecho &0I will create a great reward with them.'
   wait 3s
   mechoaround %self% %self.name% rubs the earring with their wrinkly fingers and uncaps the vial of dragon's &1blood&0.
   wait 5s
   mechoaround %self% %self.name% pours the vial onto the earring, which begins to release a thick black &9&bsmoke&0.
   wait 4s
   mechoaround %self% The smoke clears and the blood has been dried and baked onto the earring, giving it a &1&bred shine&0.  
   wait 4s
   set rnd %random.10%
   switch %rnd%
      case 1
      case 2
      case 3
      case 4
      case 5
      case 6
      case 7
         cackle %actor.name%
         say And now my prize!
         wait 1s
         mload obj 59002
         wear dragons-blood-earring 
         wait 3s
         mecho %self.name% says, 'Ahhh, you've been such a foolish one to complete
         mecho &0my deeds for me.'
         wait 2
         quest complete sacred_haven %actor.name%
         quest erase sacred_haven %actor.name%
         cast 'cause crit' %actor.name%
         break
      case 8
      case 9
      case 10
      default
         emote sighs.
         wait 2s
         say At least I have the remainder of the vial of
         mecho 'dragon's blood.'
         wait 5s
         mload obj 59002
         give dragons-blood-earring %actor.name%
         wait 6
         mecho %self.name% says, 'And it is rumored that there are two more shadow
         mecho &0forged earrings still within the realms.'
         wait 3s
         msend %actor% %self.name% turns away from you.
         mechoaround %actor.name% %self.name% turns away from %actor.name%.
         quest complete sacred_haven %actor.name%
         quest erase sacred_haven %actor.name%
   done
elseif %reward% == no
   * ask for more stuff
   wait 4
   whisper %actor.name% Ok, now bring me the rest of my artifacts.
endif
~
#59009
dark_robed_artifacts~
0 d 100
artifacts?~
wait 4
set stage 1
set person %actor%
set i %person.group_size%
if %i%
   set a 1
else
   set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
      if %person.quest_variable[sacred_haven:given_light]% == 1 && %person.align% <= -350
        set continue yes
        quest variable sacred_haven %person.name% find_blood 1
        if %person.quest_stage[sacred_haven]% == %stage%
            quest advance sacred_haven %person.name%
            msend %person% &7&bYou have advanced the quest!&0
        endif
      endif
  elseif %person%
      eval i %i% + 1
  endif
  eval a %a% + 1
done
if %continue%
   msend %actor% %self.name% whispers to you, 'I had a vial of dragon's blood, a trinket of tattered leather, and a small shadow forged earring stolen from me.  They are held somewhere inside the Sacred Haven.'
   wait 6
   msend %actor% %self.name% whispers to you, 'Once you find them all, I can meld two of them together to form a reward for you.'
   wait 2
   msend %actor% %self.name% whispers to you, 'You will find a prisoner who is an ally of mine, and he might be able to help you.'
   wait 6
   msend %actor% %self.name% tells you, 'Ask me &7&b[what am I doing?]&0 if you forget.'
   wait 1
   whisper %actor% Now hurry, I need the items I have lost.
endif
~
#59010
prisoner_greet~
0 bg 100
~
wait 4
if %actor.quest_stage[sacred_haven]% >= 2 && %actor.vnum% == -1 && %actor.level% < 100
   look %actor.name%
   wait 4
   consider %actor.name%
   wait 7
   whisper %actor.name% Did my dark friend send you?
else
   if %actor.level% < 100 && %actor.vnum% == -1
      wait 5
      set rndm %random.10%
      switch %rndm%
         case 1
         case 2
         case 3
         case 4
            mechoaround %self% %self.name% quietly mumbles, 'Please, don't hurt me.'
            break
         case 5
         case 6
         case 7
            groan
            break
         default
            mechoaround %actor% %self.name% pleads with %actor.name% in hopes he will spare his life.
            msend %actor% %self.name% pleads with you to spare him his life.
      done
   endif
endif
~
#59011
prisoner_yes~
0 d 100
yes~
wait 4
if %actor.quest_stage[sacred_haven]% >= 2 && %actor.vnum% == -1 && %actor.level% < 100
   grin
   msend %actor% %self.name% whispers to you, 'Good, we have been trying to retrieve
   msend %actor% &0our stolen artifacts when I was captured and place here.'
   wait 5
   msend %actor% %self.name% whispers to you, 'I've learned of the location of
   msend %actor% &0the earring.  It's locked in a chest.'
endif
~
#59012
prisoner_chest~
0 d 100
chest?~
wait 4
set stage 2
set person %actor%
set i %person.group_size%
if %i%
   set a 1
else
   set a 0
endif
while %i% >= %a%
  set person %person.group_member[%a%]%
  if %person.room% == %self.room%
      if %person.quest_stage[sacred_haven]% >= %stage%
        set continue yes
        quest variable sacred_haven %person.name% find_key 1
        msend %person% &7&bYou have advanced the quest!&0
      endif
  elseif %person%
      eval i %i% + 1
  endif
  eval a %a% + 1
done
if %continue%
   msend %actor% %self.name% whispers to you, 'Yes, I swiped the key for the chest off of one of the guards before I was confined, and hid it in the courtyard.'
   wait 2s
   say Now I must go, since I have been freed.
   wait 3
   emote leaves east.
   mteleport %self% 59091
   mpurge %self%
endif
~
#59013
find_key~
2 c 4
move~
if %arg% == stones
   return 1
   wsend %actor% You move the stone from the wall, and a bent key falls to the ground.
   wechoaround %actor% %actor.name% moves a stone from the wall, and a bent key falls to the ground.
   wload obj 59027
   if %actor.quest_variable[sacred_haven:find_key]% == 1
      quest variable sacred_haven %actor.name% find_key 2
   endif
else
   return 0
endif
~
#59014
find_blood~
2 c 100
search~
set now %time.stamp%
if %actor.quest_variable[sacred_haven:blood_time]% == 0
   eval last %now% - 1
else    
   eval last %actor.quest_variable[sacred_haven:blood_time]% + 20
endif
if %actor.quest_stage[sacred_haven]% == 0
set now 0
endif
if %now% > %last% 
    Wsend %actor% You find a vial of dark red dragons blood setting on top of the table.
    Wechoaround %actor% %actor.name% finds a vial of dark red dragons blood setting on top of the table.
    Wload obj 59028
quest variable sacred_haven %actor.name%  blood_time %now%
    If %actor.quest_variable[sacred_haven:find_blood]% == 1 
        quest variable sacred_haven %actor.name%  find_blood 2
    endif
else 
    wsend %actor% You don't see anything you didn't see before.
endif
~
#59015
stop_south_38~
0 c 100
south~
if %actor.class% == Paladin
  return 0
else
msend %actor% %self.name% puts a hand in your face, stopping you in your tracks.
mechoaround %actor% %self.name% puts a hand in %actor.name%'s face, stopping %actor.o% from going
south.
endif
~
#59016
rylee_wield~
0 k 100
0~
if %wie_dagger% != 2
   wait 2
   mechoaround %self% %self.name% reaches under her robe and pulls a dagger from a sheath strapped to her leg.
   wait 1
   mload obj 59012
   wie radiant-dagger
   wait 2
   say Lets see if you can defend against this.
   wait 2
   cast 'divine bolt' %actor.name%
   set wie_dagger 2
   global wie_dagger
endif
~
#59017
stop_east_58~
0 c 100
open~
if %actor.class% == Paladin
  return 0
else
switch %arg%
case door
case door e
case door ea
case door eas
case door east
mechoaround %actor% %self.name% slaps %actor.name%'s hand away from the door.
msend %actor% %self.name% slaps your hand away from the door.
break
default
msend %actor% There doesn't seem to be an %arg% here.
done
endif
~
#59018
three_lever_pull~
2 ac 100
pull~
* check to see if levers are pulled in right order
If %first_kill% !=2 && (%arg% == left || %arg% == center || %arg% == right)
   wat 59091  wforce sacred-haven-ai mat 59056 m_run_room_trig 59022
endif
if %get.obj_count[59035]% == 1 
   if %actor.level% <=40
      eval dmg (2 * %actor.level%) +  %random.70%
   Elseif %actor.level% >=41 && %actor.level% <=70
      eval dmg (3 * %actor.level%) +  %random.100%
   Elseif %actor.level% >=71 && %actor.level% <=100
      eval dmg (4 * %actor.level%) +  %random.130%
   endif
   switch %arg%
      case left
         if %last% == 1 && %first_pin% == 3 && %secnd_pin% == 3 
            if %last_pin% == 3
               wsend %actor% The lever seems to have already been pulled.
               Wechoaround %actor% %actor.name% tries to pull the lever that has already been pulled.
            else
               wsend %actor% You pull the lever, which puts into motion what sounds like chains behind the door. 
               wechoaround %actor% %actor.name% pulls the lever, which leads to the noise of chains moving behind the door.
               wait 5
               wecho *Click*
               wait 5
               wecho The large stone door slowly slides open.
               Wat 59055 wecho The large stone door slowly slides open.
               wdoor 59056 south flags a
               wdoor 59055 north flags a
               wpurge large-silver-levers
               unset first_kill
               unset last_pin
               unset secnd_pin
               unset first_pin
            endif
         elseif %secnd% == 1 && %first_pin% == 3
            if %secnd_pin% == 3
               wsend %actor% The lever seems to have already been pulled.
               Wechoaround %actor% %actor.name% tries to pull the lever that has already been pulled.
            else
               wsend %actor% You pull the lever, which leads to a series of loud clicks behind the door. 
               wechoaround %actor% %actor.name% pulls the lever, which causes a series of loud clicks to reverberate from the door.
               wait 5
               wecho You hear something in the doors locking mechanism drop.
               set secnd_pin 3
               global secnd_pin
            endif
         elseif %first% == 1
            if %first_pin% == 3
               wsend %actor% The lever seems to have already been pulled.
               Wechoaround %actor% %actor.name% tries to pull the lever that has already been pulled.
            else
               wsend %actor% As you pull the lever, the faint sound of gears turning echo out from the door. 
               wechoaround %actor% %actor.name% pulls the lever and the faint sound of gears turning echo from the door.
               wait 5
               wecho You hear something in the doors locking mechanism drop.
               set first_pin 3
               global first_pin
            endif
         else
            wechoaround %actor% %actor.name% reaches over and pulls the left lever.
            wsend %actor% You pull the left lever.
            wechoaround %actor% %actor.name% bends over in agony as a searing jolt of pain grasps %actor.o% body.  (&4%dmg%&0)
            Wsend %actor% You bend over in extreme anguish as a jolt of pain tears through your body.  (&1&b%dmg%&0)
            Wdamage %actor%  %dmg%   
         endif
         break
      case center
         if %last% == 2 && %first_pin% == 3 && %secnd_pin% == 3 
            if %last_pin% == 3
               wsend %actor% The lever seems to have already been pulled.
               Wechoaround %actor% %actor.name% tries to pull the lever that has already been pulled.
            else
               wsend %actor% You pull the lever, which puts into motion what sounds like chains behind the door. 
               wechoaround %actor% %actor.name% pulls the lever, which leads to the noise of chains moving behind the door.
               wait 5
               wecho *Click*
               wait 5
               wecho The large stone door slowly slides open.
               Wat 59055 wecho The large stone door slowly slides open.
               wdoor 59056 south flags a
               wdoor 59055 north flags a
               wpurge large-silver-levers
               unset first_kill
               unset last_pin
               unset secnd_pin
               unset first_pin
            endif
         elseif %secnd% == 2 && %first_pin% == 3
            if %secnd_pin% == 3
               wsend %actor% The lever seems to have already been pulled.
               Wechoaround %actor% %actor.name% tries to pull the lever that has already been pulled.
            else
               wsend %actor% You pull the lever, which leads to a series of loud clicks behind the door. 
               wechoaround %actor% %actor.name% pulls the lever, which causes a series of loud clicks to reverberate from the door.
               wait 5
               wecho You hear something in the doors locking mechanism drop.
               set secnd_pin 3
               global secnd_pin
            endif
         elseif %first% == 2 
            if %first_pin% == 3
               wsend %actor% The lever seems to have already been pulled.
               Wechoaround %actor% %actor.name% tries to pull the lever that has already been pulled.
            else
               wsend %actor% As you pull the lever, the faint sound of gears turning echo out from the door. 
               wechoaround %actor% %actor.name% pulls the lever and the faint sound of gears turning echo from the door.
               wait 5
               wecho You hear something in the doors locking mechanism drop.
               set first_pin 3
               global first_pin
            endif
         else
            wechoaround %actor% %actor.name% reaches out and pulls the center lever.
            wsend %actor% You pull the center lever.
            wechoaround %actor% A sudden rush of a smoky grey air blasts %actor.name%, hitting %actor.o% squre in the chest and knocking %actor.o% back.  (&4%dmg%&0)
            Wsend %actor% A sudden rush of a smoky grey air comes from nowhere and plows into your chest with the force of a war hammer.  (&1&b%dmg%&0)
            Wdamage %actor%  %dmg%   
         endif
         break
      case right
         if %last% == 3 && %first_pin% == 3 && %secnd_pin% == 3 
            if %last_pin% == 3
               wsend %actor% The lever seems to have already been pulled.
               Wechoaround %actor% %actor.name% tries to pull the lever that has already been pulled.
            else
               wsend %actor% You pull the lever, which puts into motion what sounds like chains behind the door. 
               wechoaround %actor% %actor.name% pulls the lever, which leads to the noise of chains moving behind the door.
               wait 5
               wecho *Click*
               wait 5
               wecho The large stone door slowly slides open.
               Wat 59055 wecho The large stone door slowly slides open.
               wdoor 59056 south flags a
               wdoor 59055 north flags a
               wpurge large-silver-levers
               unset first_kill
               unset last_pin
               unset secnd_pin 
               unset first_pin
            endif
         elseif %secnd% == 3 && %first_pin% == 3
            if %secnd_pin% == 3
               wsend %actor% The lever seems to have already been pulled.
               Wechoaround %actor% %actor.name% tries to pull the lever that has already been pulled.
            else
               wsend %actor% You pull the lever, which leads to a series of loud clicks behind the door. 
               wechoaround %actor% %actor.name% pulls the lever, which causes a series of loud clicks to reverberate from the door.
               wait 5
               wecho You hear something in the doors locking mechanism drop.
               set secnd_pin 3
               global secnd_pin
            endif
         elseif %first% == 3 
            if %first_pin% == 3
               wsend %actor% The lever seems to have already been pulled.
               Wechoaround %actor% %actor.name% tries to pull the lever that has already been pulled.
            else
               wsend %actor% As you pull the lever, the faint sound of gears turning echo out from the door. 
               wechoaround %actor% %actor.name% pulls the lever and the faint sound of gears turning echo from the door.
               wait 5
               wecho You hear something in the doors locking mechanism drop.
               set first_pin 3
               global first_pin
            endif
         else
            wechoaround %actor% %actor.name% reaches out and pulls the right lever.
            wsend %actor% You pull the right lever.
            wechoaround %actor% Blue arcs of electricity flow out of the lever and travel up %actor.name%'s arm, sending %actor.o% reeling with a jolt.  (&4%dmg%&0)
            Wsend %actor% A blue arc of electricity jumps out of the lever and travels through your body, giving you quite a jolt.  (&1&b%dmg%&0)
            Wdamage %actor%  %dmg%   
         endif
         break
      default
         wsend %actor% Pull what?
   done
else 
   return 0
endif
~
#59019
keeper_stop~
0 c 100
open~
switch %arg%
case door
case door e
case door ea
case door eas
case door east
case door s
case door so
case door sou
case door sout
case door south
Msend %actor% You reach towards the door to open it.
 Mechoaround %actor% %actor.name% leans towards the door to try and open it.
wait 2
Msend %actor% %self.name% pushes you away from the door.
Mechoaround %actor% %self.name% gives %actor.name% a swift shove sending %actor.o% away from the door.
break
default
return 0
done
~
#59020
keeper_stop_ret0~
0 c 100
op~
return 1
~
#59021
stop_west_R69~
0 c 100
west~
if %actor.class% == Paladin
  return 0
else
msend %actor% %self.name% jumps in front of you, preventing you from getting by.
mechoaround %actor% %self.name% jumps in front of %actor.name%, preventing %actor.o% from getting
past.
endif
~
#59022
set_levers~
2 a 100
~
If %first_kill% != 2  
set rnd %random.6%
switch %rnd%
case 1
set first 1
set secnd 2
set last 3
global first
global secnd
global last
break
case 2
set first 1
set secnd 3
set last 2
global first
global secnd
global last
break
case 3
set first 2
set secnd 3
set last 1
global first
global secnd
global last
break
case 4
set first 2
set secnd 1
set last 3
global first
global secnd
global last
break
case 5
set first 3
set secnd 2
set last 1
global first
global secnd
global last
break
case 6
set first 3
set secnd 1
set last 2
global first
global secnd
global last
break
done
set first_kill 2
global first_kill
set first_pin 1
set secnd_pin 1
set last_pin 1
global first_pin
global secnd_pin
global last_pin
endif
~
#59023
chesters_testers~
2 h 100
0~
quest variable sacred_haven %actor.name%  find_key 1
~
#59024
fight_kick~
0 k 100
~
set rdm %random.10%
switch %rdm%
case 1
case 2
case 3
wait 3
kick
done
~
#59030
poet_talk~
0 b 15
~
daydream
wait 6
say My thoughts seem to be my only friend.
~
#59031
notused~
0 c 100
fl~
return 1
set rnd %random.10%
switch %rnd%
case 1
case 2
case 3
case 4
case 5
mechoaround %actor% %actor.name% panics, and attempts to flee!
mechoaround %actor% %actor.name% leaves north.
mteleport %actor% 59035
mechoaround %actor% %actor.name% enters from the south.
mforce %actor% look
msend %actor% You panic and flee north!
break
case 6
case 7
case 8
case 9
case 10
msend %actor% PANIC!  You couldn't escape!
break
done
 
~
#59032
notused2~
0 c 100
fle~
return 1
set rnd %random.10%
switch %rnd%
case 1
case 2
case 3
case 4
case 5
   mechoaround %actor% %actor.name% panics, and attempts to flee!
   mechoaround %actor% %actor.name% leaves north.
 mteleport %actor% 59035
mechoaround %actor% %actor.name% enters from the south.
mforce %actor% look
   msend %actor% You panic and flee north!
break
case 6
case 7
case 8
case 9
case 10
msend %actor% PANIC!  You couldn't escape!
break
done
~
#59033
stop_flee3~
0 c 100
flee~
if %actor.class% == Paladin
  return 0
else
return 1
set rnd %random.10%
switch %rnd%
case 1
case 2
case 3
case 4
case 5
mechoaround %actor% %actor.name% panics, and attempts to flee!
mechoaround %actor% %actor.name% leaves north.
mteleport %actor% 59035
mechoaround %actor% %actor.name% enters from the south.
mforce %actor% look
msend %actor% You panic and flee north!
break
case 6
case 7
case 8
case 9
case 10
msend %actor% PANIC!  You couldn't escape!
break
done
endif
~
#59034
flee_stop~
0 c 100
flee~
return 1
set rnd %random.10%
switch %rnd%
case 1
case 2
case 3
case 4
case 5
mechoaround %actor% %actor.name% panics, and attempts to flee!
mechoaround %actor% %actor.name% leaves south.
mteleport %actor% 59068
mechoaround %actor% %actor.name% enters from the north.
mforce %actor% look
msend %actor% You panic and flee south!
break
case 6
case 7
case 8
case 9
case 10
 msend %actor% PANIC!  You couldn't escape!
break
done
~
#59035
flee_return0~
0 c 100
f~
return 0
~
#59036
agressive_attack~
0 g 100
~
if %actor.align% <= -350 && %actor.level% < 100 && %actor.vnum% == -1
wait 4
say There is no place for your kind around here.
wait 6
kill %actor.name%
end if
~
#59037
agressive_attack2~
0 g 100
~
if %actor.align% <= -350 && %actor.level% < 100 && %actor.vnum% == -1
wait 4
kill %actor.name%
endif
~
#59038
agressive_attack3~
0 g 100
~
wait 8 
set victim %random.char%
if %victim.level% <100 && %victim.vnum% == -1
kill %victim.name%
endif
~
#59039
agressive_mobs_enter~
0 b 100
~
Nothing.
~
#59040
old_man_steal~
0 g 100
~
set target %random.char%
set rnd %random.100%
if %rnd% > 99
set loot gem
    elseif %rnd% == 99
    set loot iron 
elseif (%rnd% <= 98) && (%rnd% >= 97)
    set loot cloth
elseif (%rnd% <= 96) && (%rnd% >= 95)
    set loot wood
elseif (%rnd% <= 94) && (%rnd% >= 93)
    set loot white
elseif (%rnd% <= 92) && (%rnd% >= 90)
    set loot red
elseif (%rnd% <= 89) && (%rnd% >= 85)
    set loot blue
elseif (%rnd% <= 84) && (%rnd% >= 80)
    set loot green
elseif (%rnd% <= 79) && (%rnd% >= 76)
    set loot black
elseif (%rnd% <= 75) && (%rnd% >= 71)
    set loot dagger
elseif (%rnd% <= 70) && (%rnd% >= 66)
    set loot shield
elseif (%rnd% <= 65) && (%rnd% >= 61)
    set loot sword
elseif (%rnd% <= 60) && (%rnd% >= 56)
    set loot leather
elseif (%rnd% <= 55) && (%rnd% >= 46)
    set loot raft
elseif (%rnd% <= 45) && (%rnd% >= 36)
    set loot canoe
elseif (%rnd% <= 35) && (%rnd% >= 26)
    set loot sack
elseif (%rnd% <= 25) && (%rnd% >= 16)
    set loot bag
    elseif %rnd% <= 15
    set loot gem
    endif
if (%target.vnum% == -1) && (%target.level% < 100)
steal %loot% %target.name%
endif
 
~
#59041
find_blood_ret_S~
2 c 100
s~
return 0
~
#59042
roll_dice_chance~
0 c 100
roll~
say My trigger commandlist is not complete!
~
#59043
sacred_haven_paladin_enter~
0 h 100
~
if %actor.align% >= 350
  if %actor.class% == Paladin || %actor.class% == Priest || %actor.class% == Cleric
    wait 2
    rem key
    unlock door
    wait 2
    open door
    hold key
    wait 2
    bow %actor.name%
    say Welcome to the Sacred Haven, fellow warrior.
  endif
endif
~
#59044
group_armor_forgemaster_greet~
0 g 100
~
wait 2
set stage %actor.quest_stage[group_armor]%
if ((%actor.class% /= Cleric && %actor.level% > 72) || (%actor.class% /= Priest && %actor.level% > 56)) && %stage% == 0
  mecho %self.name% says, 'Well hello and welcome to the Sacred Haven's forge!
  mecho &0My name's Galen, son of Thorgrim, of the clan Grugnir.  I'm the resident
  mecho &0Forgemaster of the Sacred Haven.'
  wait 3s
  mecho %self.name% says, 'I'm working to prepare the paladins of the Haven
  mecho &0for their campaign against the evil races staking out Bluebonnet Pass by
  mecho &0forging an icon that will cast Group Armor.'
  wait 3s
  mecho %self.name% says, 'Are you interested in helping out?'
  mecho  
  mecho %self.name% says, 'You'll be able to learn the spell for yourself in the
  mecho &0process.'
elseif %stage% == 1
  say Have you returned with items that cast armor?
elseif %stage% == 2
  say I hope you've sourced a new forging hammer!
elseif %stage% == 3 || %stage% == 4
  say Any luck communing with the spirits of the forge?
elseif %stage% == 5
  say How goes the search for a protective amulet?
elseif %stage% == 6
  mecho %self.name% says, 'Welcome back!  I see the ghosts haven't eaten you yet!
  mecho &0Have you brought any ethereal armor?'
endif
~
#59045
group_armor_forgemaster_speech~
0 d 100
yes yes! yep okay sure~
set stage %actor.quest_stage[group_armor]%
wait 2
if ((%actor.class% /= Cleric && %actor.level% > 72) || (%actor.class% /= Priest && %actor.level% > 56))
  if %stage% == 0
    quest start group_armor %actor.name%
    say Then welcome aboard!
    wait 1s
    mecho %self.name% says, 'First, I need four things that cast the Armor
    mecho &0spell to amplify my prayers to Moradin:'
    mecho &3&b%get.obj_shortdesc[6118]%&0
    mecho &3&b%get.obj_shortdesc[11704]%&0
    mecho &3&b%get.obj_shortdesc[11707]%&0
    mecho &3&b%get.obj_shortdesc[16906]%&0
    wait 4s
    mecho %self.name% says, 'Moradin watch over your travels!
    mecho &0If you need an update, ask me about your &7&b[progress]&0.'
  elseif %stage% == 1
    say Let me see what you have!
  elseif %stage% == 2
    say Thank you so much!
    wait 1s
    mecho %self.name% says, 'Bring back a forging hammer and then I can tell
    mecho &0you what to do next.'
  elseif %stage% == 3 || %stage% == 4
    say Let me test out the new hammer then!
  elseif %stage% == 6
    say Let me see what you've brought!
  endif
elseif ((%actor.class% /= Cleric && %actor.level% < 72) || (%actor.class% /= Priest && %actor.level% < 56))
  mecho %self.name% says, 'I don't think you're quite ready to be a forge
  mecho &0acolyte yet.'
  laugh
  msend %actor% %self.name% claps you on the back with his mighty hand!
  mechoaround %actor% %self.name% claps %actor.name% on the back with his mighty hand.
elseif %actor.class% != Cleric || %actor.class% != Priest
  mecho %self.name% says, 'You may wish to prove your worth but I can only
  mecho &0work with those dedicated to divinity.'
endif
~
#59046
group_armor_forgemaster_receive~
0 j 100
~
set stage %actor.quest_stage[group_armor]%
switch %stage%
  case 1
    if %object.vnum% == 6118 || %object.vnum% == 11704 || %object.vnum% == 11707 || %object.vnum% == 16906
      if %actor.quest_variable[group_armor:%object.vnum%]% == 1
        return 0
        say You already brought me %object.shortdesc%!
      else
        quest variable group_armor %actor.name% %object.vnum% 1
        wait 2
        mjunk %object.name%
        wait 2
        say Aye, this'll do.
        set item1 %actor.quest_variable[group_armor:6118]%
        set item2 %actor.quest_variable[group_armor:11704]%
        set item3 %actor.quest_variable[group_armor:11707]%
        set item4 %actor.quest_variable[group_armor:16906]%
        if %item1% && %item2% && %item3% &&%item4%
          quest advance group_armor %actor.name%
          wait 2
          say That looks like everything!
          emote places the four objects near the altar.
          wait 4s
          say Bad news though.
          wait 1s
          mecho %self.name% says, 'The Haven was attacked by orcs from Bluebonnet Pass
          mecho &0while you were gone!'
          wait 4s
          mecho %self.name% says, 'Fortunately no one was killed, but I had to use
          mecho &0my consecrated forging hammer to defend myself.  It's no good for works of
          mecho &0holy creation now.'
          wait 4s
          mecho %self.name% says, 'Since I need to fix up the forge after the attack
          mecho &0would you be willing to find me another forging hammer?'
        else
          eval total (%item1% + %item2% + %item3% + %item4%)
          wait 2
          if %total% <= 2
            say Do you have any of the other items?
          elseif %total% == 3
            say Do you have the last one?
          endif
        endif
      endif
    else
      return 0
      say %object.shortdesc% doesn't cast Armor ya daffy spriggan!
      mecho %self.name% refuses %object.shortdesc%.
      laugh
    endif
    break
  case 2
    if %object.vnum% == 8701
      wait 2
      emote examines the hammer.
      say This will do!
      wait 1s
      mecho %self.name% says, 'To consecrate the hammer again you'll need to
      mecho &0find a place where the light of the sun reaches deep into the bowels of
      mecho &0Ethilien and &7&b[commune]&0 with the spirits there.'
      wait 2
      give hammer %actor.name%
      wait 4s
      mecho %self.name% says, 'Places where ore and stone are mined often become
      mecho &0sacred places for us dwarves.'
      quest advance group_armor %actor.name%
    else 
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      say This isn't a forging hammer!
    endif
    break
  case 3
    return 0
    if %object.vnum% == 59039
      mecho %self.name% refuses %object.shortdesc%.
      say Where did you find this?
      wait 1s
      say This can't be the same hammer you brought me before.
    elseif %object.vnum% == 8701
      mecho %self.name% refuses %object.shortdesc%.
      mecho %self.name% says, 'You need to bring me the consecrated hammer.
      mecho &0Another mundane forging hammer is useless.'
    endif
    break
  case 4
    if %object.vnum% == 59039
      quest advance group_armor %actor.name%
      wait 2
      mjunk %object.name%
      wait 2
      say Moradin's flames, this is splendid!
      wait 1s
      mecho %self.name% gives the new hammer a few practice swings.
      say Aye, that's a fine hammer!
      wait 3s
      say Let us continue!
      wait 1s
      mecho %self.name% says, 'This spell needs to be contained in a holy symbol of
      mecho &0some kind.  There is an amulet, an Aegis amulet, that would serve well for this
      mecho &0purpose.'
      wait 3s
      say Find it and bring it back here.
    else
      return 0
      say What in the heck is this?
      msend %actor% %self.name% refuses %object.shortdesc%.
    endif
    break
  case 5
    if %object.vnum% == 12500
      wait 2
      mecho %self.name% scrutinizes %object.shortdesc% with a discerning glance.
      mjunk %object.name%
      wait 2s
      nod
      say Mmhmmm.  Mhmmm.
      wait 1s
      say Yes, this will be an excellent divine focus.
      wait 2s
      mecho %self.name% says, 'Finally, I need to capture the intangible protective
      mecho &0essence of ethereal armor to finish the amulet.  I know of three pieces:'
      mecho &7&ba breastplate&0
      mecho &7&ba ring&0
      mecho &7&ba bracelet&0
      wait 4s
      mecho %self.name% says, 'See if you can find them.  Items like these are
      mecho &0often worn by the dead as they can interact with them.'
      wait 2s
      say Good luck and Moradin guard you!
      quest advance group_armor %actor.name%
    else
      return 0
      say This won't be a very effective focus.
      mecho %self.name% refuses %object.shortdesc%.
    endif
    break
  case 6
    if %object.vnum% == 47004 || %object.vnum% == 47018 || %object.vnum% == 53003 
      if %actor.quest_variable[group_armor:%object.vnum%]% == 1
        return 0
        say You already brought me %object.shortdesc%!
      else
        wait 2
        quest variable group_armor %actor.name% %object.vnum% 1
        mecho %self.name% places %object.shortdesc% near the altar.
        mjunk %object.name%
        wait 1s
        set item1 %actor.quest_variable[group_armor:47004]%
        set item2 %actor.quest_variable[group_armor:47018]%
        set item3 %actor.quest_variable[group_armor:53003]%
        if %item1% && %item2% && %item3%
          grin
          say With everything gathered, I can get to work!
          wait 2s
          mecho %self.name% lays the ethereal breastplate on the anvil.
          wait 4s
          mecho %self.name% thrusts his hammer into the forge.
          mecho %self.name% entones, 'Dwarffather, hear my prayer.'
          wait 4s
          mecho %self.name% raises the &1&bglowing&0 hamer.
          wait 4s
          mecho %self.name% entones, 'Bless my hammer in this act of Creation.'
          wait 3s
          mecho The hammer turns &7&bwhite&0 with holy energy.
          wait 5s
          mecho %self.name% brings the hammer down on the ethereal breastplate!
          mecho The breastplate ripples from the force of the hit like the surface of a lake struck by a drop of water.
          wait 5s
          say Now we add the extra magic!
          wait 2s
          mecho %self.name% grabs the broom and the hardened stick of leather.
          mecho %self.name% points them at the ethereal breastplate and releases their magic.
          wait 4s
          mecho The ethereal breastplate glows white-hot and starts to sizzle!
          wait 4s
          mecho %self.name% takes a pair of tongs and lays the ethereal bracelet on the radiating breastplate.
          wait 5s
          mecho %self.name% uncorks the violet potion and pours it over the breastplate.
          mecho The glow of the breastplate begins to seep into the bracelet!
          wait 5s
          say Let Creation ring throughout this hallowed forge!
          wait 4s
          mecho %self.name% strikes the ethereal metals again, melding the bracelet into the armor.
          wait 4s
          mecho The armor begins to decohere.
          wait 5s
          mecho %self.name% places the ethereal ring on the shifting energies.
          mecho %self.name% pours the green potion over the ring, which quickly dissolves.
          wait 5s
          mecho %self.name% strikes the ethereal mass over and over, chanting prayers to Moradin.
          wait 5s
          mecho The ethereal energies completely decohere!
          mecho The forge is filled with hot energy like a basin filled with water.
          wait 5s
          mecho %self.name% raises the Aegis amulet over his head.
          wait 2s
          say Soul Forger, let this amulet grant us Your protection!
          wait 4s
          mecho The Aegis amulet begins to &3&bglow!&0
          wait 4s
          mecho All sound falls away as the radiant energies of the forge are absorbed into the amulet!
          wait 4s
          mecho %self.name% places the amulet on the forge anvil.
          wait 4s
          mecho With a final ceremonial blow, %self.name% seals the protective energies into their new form!
          wait 7s
          mecho Slowly sound returns as the ringing of an anvil.
          wait 2s
          mecho %self.name% inspects the new holy amulet.
          wait 2s
          say Aye, our work is finished.  Thank you acolyte.
          emote bows and says, 'May the Dwarffather shield you ever more!'
          wait 2s
          quest complete group_armor %actor.name%
          msend %actor% You feel a stirring in your soul!
          msend %actor% &7&bMoradin blesses you with the knowledge of Group Armor!&0
          mskillset %actor.name% group armor
        else
          say Did ya find anything else?
        endif
      endif
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      say This isn't going to serve for Group Armor.
    endif    
    break
  default
    return 0
    mecho %self.name% refuses %object.shortdesc%.
    say What is this for?
done
~
#59047
group_armor_hammer_commune~
1 c 3
commune~
switch %cmd%
  case c
  case co
  case com
  case comm
    return 0
    halt
done
if %actor.quest_stage[group_armor]% == 3 && %self.room% == 13358
  quest advance group_armor %actor.name%
  oecho Holding %self.shortdesc% up to the shaft of light, it begins to hum!
  wait 3s
  oecho The hammer grows warm.
  wait 3s
  oecho %self.shortdesc% begins emitting a soft glowing light!
  wait 3s
  oecho %self.shortdesc% becomes as light as a feather but remains as solid as iron.
  wait 3s
  oecho %self.shortdesc% floats in the air for a moment before sinking to the ground.
  wait 2s
  oecho The humming ceases, leaving the hammer glowing with a faint, ethereal light.
  oload obj 59039
  oforce %actor% get mystic-forging-hammer
  wait 2
  opurge %self%
else
  return 0
endif
~
#59048
group_armor_forgemaster_status_checker~
0 d 100
status status? progress progress?~
wait 2
set stage %actor.quest_stage[group_armor]%
switch %stage%
  case 1
    set item1 %actor.quest_variable[group_armor:6118]%
    set item2 %actor.quest_variable[group_armor:11704]%
    set item3 %actor.quest_variable[group_armor:11707]%
    set item4 %actor.quest_variable[group_armor:16906]%
    set obj1 %get.obj_shortdesc[6118]%
    set obj2 %get.obj_shortdesc[11704]%
    set obj3 %get.obj_shortdesc[11707]%
    set obj4 %get.obj_shortdesc[16906]%
    set step locate items that cast the spell &7&barmor&0
    break
  case 2
    set step replace my &7&bforging hammer&0
    break
  case 3
  case 4
    set step Take the forging hammer where light reaches deep underground and &7&b[commune]&0
    break
  case 5
    set step find a suitable amulet to be the focus of this spell
    set item1 %actor.quest_variable[group_armor:12500]%
    set obj1 %get.obj_shortdesc[12500]%
    break
  case 6
    set step locate ethereal items to provide protective energy to the amulet
    set item1 %actor.quest_variable[group_armor:47004]%
    set item2 %actor.quest_variable[group_armor:47018]%
    set item3 %actor.quest_variable[group_armor:53003]%
    set obj1 %get.obj_shortdesc[47004]%
    set obj2 %get.obj_shortdesc[47018]%
    set obj3 %get.obj_shortdesc[53003]%
    break
  default
    if %actor.has_completed[group_armor]%
       if %actor.sex% == male
         say You've already helped me lad.
       elseif %actor.sex% == female
         say You've already helped me lass.
       endif
       halt
    else
       say You aren't doing anything for me at the moment.
       halt
    endif
done
wait 2
say You are trying to:
mecho %step%.
if %stage% == 1 || %stage% == 5 || %stage% == 6
  if %item1% || %item2% || %item3% || %item4%
    mecho 
    mecho You have already brought me:
    if %item1%
      mecho - %obj1%
    endif
    if %item2%
      mecho - %obj2%
    endif
    if %item3%
      mecho - %obj3%
    endif
    if %item4%
      mecho - %obj4%
    endif
  endif
  mecho   
  mecho You need to locate:
  if !%item1%
    mecho - &3&b%obj1%&0
  endif
  if %stage% == 1 || %stage% == 6
    if !%item2%
      mecho - &3&b%obj2%&0
    endif
    if !%item3%
      mecho - &3&b%obj3%&0
    endif
    if %stage% == 1
      if !%item4%
        mecho - &3&b%obj4%&0
      endif
    endif
  endif
endif
~
#59049
**UNUSED**~
1 c 100
commun~
return 0
~
#59099
sacred_haven_status_check~
0 d 0
what am I doing?~
set stage %actor.quest_stage[sacred_haven]%
set light %actor.quest_variable[sacred_haven:given_light]%
set key %actor.quest_variable[sacred_haven:find_key]% 
set blood %actor.quest_variable[sacred_haven:given_blood]%
set trinket %actor.quest_variable[sacred_haven:given_trinket]%
set earring %actor.quest_variable[sacred_haven:given_earring]%
wait 2
if %stage% == 1 && %light% == 0
  mecho %self.name% says, 'Prove yourself.  Bring me %get.obj_shortdesc[59026]%
  mecho &0from a priest on the second floor.'
elseif %stage% == 1 && %light% == 1
  say Ask me about my artifacts.
elseif %stage% == 2 && %key% == 0
  say Break my ally out of jail.
  mecho   
  mecho Then bring me:
  mecho - %get.obj_shortdesc[59028]%
  mecho - %get.obj_shortdesc[59029]%
  mecho - %get.obj_shortdesc[59030]%
elseif %stage% == 2 && %key% == 1
  say Find the key my ally stashed in the courtyard.
  mecho   
  mecho Then bring me:
  mecho - %get.obj_shortdesc[59028]%
  mecho - %get.obj_shortdesc[59029]%
  mecho - %get.obj_shortdesc[59030]%
elseif %stage% == 2 && %key% == 2
  say Bring me my artifacts.
  if %blood% || %trinket% || %earring%
    mecho   
    mecho You have brought me:
    if %blood% == 1
      mecho - %get.obj_shortdesc[59028]%
    endif
    if %trinket% == 1
      mecho - %get.obj_shortdesc[59029]%
    endif
    if %earring% == 1
      mecho - %get.obj_shortdesc[59030]%
    endif
  endif
  mecho   
  mecho You still need to find:
  if %blood% == 0
    mecho - %get.obj_shortdesc[59028]%
  endif
  if %trinket% == 0
    mecho - %get.obj_shortdesc[59029]%
  endif
  if %earring% == 0
    mecho - %get.obj_shortdesc[59030]%
  endif
  wait 2
  say Bring me my artifacts! Chop chop!
endif
~
$~
