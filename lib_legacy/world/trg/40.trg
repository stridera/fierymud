#4000
Random_Poison~
2 g 100
null~
if %actor.level% < 100
   wait 1s
   wdamage %actor% 5 poison
   if %damdone% != 0
      wsend %actor% The tainted air fills your lungs with its cryptic essence. (&2%damdone%&0)
      wechoaround %actor% %actor.name% begins to appear ill and faint. (&2%damdone%&0)
      wait 4s
      wdamage %actor% 15 poison
      if %damdone% != 0
         wsend %actor% The poison rips into your body corrupting your blood. (&2%damdone%&0)
         wechoaround %actor% %actor.name% bends over rasping and grabbing %actor.p% chest in pain. (&2%damdone%&0)
         wait 30s
         wdamage %actor% 12 poison
         if %damdone% != 0
            wsend %actor% The poison rips into your body corrupting your blood. (&2%damdone%&0)
            wechoaround %actor% %actor.name% bends over rasping and grabbing %actor.p% chest in pain. (&2%damdone%&0)
            wait 30s
            wdamage %actor% 15 poison
            if %damdone% != 0
               wsend %actor% The poison rips into your body corrupting your blood. (&2%damdone%&0)
               wechoaround %actor% %actor.name% bends over rasping and grabbing %actor.p% chest in pain. (&2%damdone%&0)
            end
         end
      end
   end
end
~
#4001
Door opener~
2 d 0
norhamen beneti sovering tarlon campri~
wdoor 4010 down room 4087
wecho The wall screams in vain as air thrust towards you.
wecho The wall fades out of view revealing a staircase downward.
wait 10s
wdoor 4010 down purge
~
#4002
Auto door opener~
2 d 0
darkness hear my call unto you~
wdoor 4019 u room 4063
wecho The southern wall crumbles and falls as darkness flees from behind it.
wecho Unknowing steps are revealed ascending.
wait 10s
wdoor 4019 up purge
~
#4003
Domain_Newbie_Guard~
0 c 100
down~
if %actor.vnum% == -1
   if %actor.level% < 30
      whisper %actor.name% Greetings young one, thou art brave but not strong enough to stride forth into these depths.
      wait 1s
      whisper %actor.name% Move towards the blackened sands to find battle for now.
      wink %actor.name%
   else
      return 0
   endif
else
   return 0
endif
~
#4004
soul capture~
1 bc 1
kill hit cast kick backstab~
return 0
wait 1s
set player %self.worn_by%
if %player.fighting%
   if %player.fighting% == %victim%
   else
      oecho reseting variables
      set victim %player.fighting%
      set soul %victim.name%
      eval heal (%victim.level% * 5) + %random.10%
      set validate 0
      global victim
      global soul
      global heal
      global validate
   endif
else
   halt
endif
eval health %victim.hit% / %victim.maxhit% * 100
if %validate% == 2
   if %victim% == 0
      osend %player% &1The soul of %soul% is captured by the ring of souls.&0 (&2&b%heal%&0)
      oheal %player% %heal%
   elseif %health% <= 5
      odamage %victim% %victim.hit%
      oheal %player% %heal%
      osend %player% &1&b%soul%'s soul is torn from its mortal frame.&0 (&3%damdone%&0) (&2&b%heal%&0)
   else
      halt
   endif
   unset validate
   unset victim
elseif %validate% == 1
   if %health% <= 20
      set validate 2
      global validate
      osend %player% &5You feel the ring of souls work more deeply on %victim.name%.&0
   endif
elseif %validate% == 0
   if %health% >= 50
      set validate 1
      global validate
      osend %player% &5You feel the ring of souls begin to work on %victim.name%.&0
   endif
endif
~
#4005
soul siphon~
1 b 100
~
if %self.worn_by%
   set actor %self.worn_by%
   if %actor.fighting%
      if %count%
         eval count %count% + 1
         global count
         if %count% == 4
            oecho &5The ring of souls &bglows &1radiantly!&0
         endif
      else
         oecho &5The ring of souls begins &bglowing.&0
         set count 1
         global count
      endif
   else
      if %count%
         if %count% > 100
            eval heal 100 * 3 + %random.20%
         elseif %count% > 50
            eval heal %count% * 4 + %random.20%
         elseif %count% > 30
            eval heal %count% * 6 + %random.20%
         elseif %count% > 20
            eval heal %count% * 8 + %random.20%
         elseif %count% > 10
            eval heal %count% * 10 + %random.20%
         elseif %count% > 3
            eval heal %count% * 11 + %random.20%
         else
            unset count
            halt
         endif
         oheal %actor% %heal%
         osend %actor% &5Your ring of souls captures the essence of death and channels strength into you!&0 (&2&b%heal%&0)
         oechoaround %actor% &5&b%actor.name%'s ring of souls captures the essence of death and channels strength into its master!&0 (&2&b%heal%&0)
         unset count
      endif
   endif
endif
~
#4013
Borgan_Attack~
0 k 20
~
set val %random.10%
switch %val%
case 1
breath fire
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
$~
