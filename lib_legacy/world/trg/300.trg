#30000
Nasty sulfur fumes~
2 b 40
~
set victim %random.char%
if %victim% == 0 || %victim.vnum% != -1
   halt
end
set damage 2
switch %random.10%
   case 1
      set damage 1
   case 2
      wsend %victim% You just inhaled a lungful of sulphurous fumes!  It burns!
      wechoaround %victim% %victim.name% inhales a lungful of fumes and chokes and gags!
      wdamage %victim% %damage%
      break
   case 3
   case 4
      wsend %victim% The stinking yellow smoke wafts under your nose, making you sneeze.
      wechoaround %victim% %victim.name% breathes in a bit of yellow smoke, and sneezes.
      break
   case 5
   case 6
      wsend %victim% Your eyes water from the rotten-smelling smoke.
      wechoaround %victim% %victim.name% blinks as the nasty smoke gets in %victim.p% eyes.
      break
   default
      wecho Nasty fumes waft up from the wreckage.  They smell of rotten egs.
done
~
#30001
Sulfur smell from the east~
2 b 10
~
switch %random.8%
   case 1
      wecho A breath of sulphur drifts in from the east.
      break
   case 2
      wecho Stinking fumes waft in from the ruined building east of here.
      break
   case 3
      wecho Smoke drifts in from the east, bringing with it the stink of rotten eggs.
      break
   case 4
      wecho A sulphurous odor fills your nose.
      break
   case 5
      wecho Yellowish vapors creep in from the wreckage to the east.
      break
   case 6
      wecho A repulsive smell from the east invades your nose.
      break
   case 7
      wecho Swirling currents of air carry the odor of sulphur.
      break
   default
      wecho An offensive smell appears to be emanating from the building to the east.
done
~
#30002
**UNUSED**~
0 c 100
re~
return 0
~
#30003
prevent recite~
0 c 100
recite~
switch %cmd%
  case r
  case re
    return 0
    halt
done
return 1
msend %actor% You cannot recite scrolls in a shop or guild!
~
#30004
**UNUSED**~
0 c 100
u~
return 0
~
#30005
prevent use~
0 c 100
use~
switch %cmd%
  case u
    return 0
    halt
done
return 1
msend %actor% You cannot use wands or staves in a shop or guild!
~
#30006
don't be invisible~
0 g 100
~
if %self.aff_flagged[INVIS]%
vis
endif
~
#30007
receive enemy communication~
0 j 100
~
switch %object.vnum%
   case 30212
      return 1
      wait 4
      mjunk scroll
      say Why thank you... good spy.  I'll have my warlocks extract this message,
      say and find out what those self-righteous paladins are up to.
      wait 2 s
      say Here is something for your trouble.
      eval gemvnum 55581 + %random.8%
      mload obj %gemvnum%
      eval gemvnum 55581 + %random.8%
      mload obj %gemvnum%
      give all.gem %actor.name%
      break
   case 30208
      return 1
      wait 3
      mjunk parchment
      growl
      wait 2 s
      say What are you doing with this!  You spy!  Paladin!  GUARDS!
      wait 1 s
      open door
      wait 4
      eval damage %actor.level% * 2 + 5
      mdamage %actor% %damage% crush
      if %damdone% == 0
         mechoaround %actor% %self.name% tries to kick %actor.name%, but is having trouble with %self.p% foot!
         msend %actor% %self.name% tries to kick you, but is having trouble with %self.p% foot!
      else
         mechoaround %actor% %self.name% kicks %actor.name% solidly in the head! (&1%damdone%&0)
         msend %actor% %self.name% kicks you solidly in the head! (&1%damdone%&0)
         mechoaround %actor% %actor.name% is sent reeling through the doorway!
         msend %actor% You topple through the door into the next room!
         wait 3
         mteleport %actor% 30038
         mforce %actor% look
      end
      break
   default
      return 0
      mechoaround %actor% %actor.name% tries to give %object.shortdesc% to %self.name%.
      msend %actor% You try to give %object.shortdesc% to %self.name%, but %self.n% refuses.
      wait 1 s
      say I don't need your rubbish!  Show some respect, dog!
done
~
#30008
give things to the wrong officer~
0 j 100
~
switch %object.vnum%
case 30212
case 30208
   return 0
   mechoaround %actor% %actor.name% tries to give %object.shortdesc% to %self.name%.
   msend %actor% You try to give %object.shortdesc% to %self.name%, but %self.n% refuses.
   wait 1 s
   say Eh?  Don't bother me with this stuff.
   say Give it to the general, he does all the planning around here.
   break
default
   return 0
   mechoaround %actor% %actor.name% tries to give %object.shortdesc% to %self.name%.
   msend %actor% You try to give %object.shortdesc% to %self.name%, but %self.n% refuses.
   wait 1 s
   say What?  Don't bother me with such trifles!
done
~
#30009
General Angrugg babbles~
0 b 20
~
mutter
wait 4
say What ARE those filthy paladins up to?
wait 2 s
stomp
wait 1 s
say I would pay handsomely for information as to their activities...
wait 1 s
emote looks at you meaningfully.
~
#30010
General Angrugg greets orcs~
0 g 70
~
if %actor.race% /= orc
   wait 2 s
   mechoaround %actor% %self.alias% looks at %actor.alias%.
   msend %actor% %self.alias% looks at you.
   wait 1 s
   say Hrm, perhaps you can help me.
   say I need to know what those treacherous paladins are up to.
   wait 3 s
   say Bring me some of their messages, if you find any, and I will reward you.
endif
~
#30011
Enter Illusionist Guild Ogakh~
2 c 100
west~
return 1
wdoor 30020 west room 30120
wforce %actor% west
wdoor 30020 west purge
~
#30020
load random gems~
0 g 100
100~
* number of gems to load -- statring at 3, but that might be too many
set loop 3
* gem vnums go from  to 55566-55751
* p1 vnums from 55566-55593
* p2 vnums from 55594-55670
* P3 cnums from 55671-55747 (there are gems up to 55751, but not used.
* random # -- 1-10 to create probabilites of good gem
* 0 = NO GEM
* 1 = NO GEM
* 2-6 = P1 Gem
* 7-9 = P2 Gem
* 10  = P3 Gem
* -- lets see if we should run process to get gems 
* -- we do that by looking for object 18701 -- if we are wearing it
* -- then we don't need to load gems again
* all the important stuff incased in this loop
if !%self.wearing[18701]%
     mload obj 18701
     mat 1100 wear lock
     set itt 1
     while %itt% <= %loop%
     set p %random.10%
         if %p% == 10
          *say p3! %p%
             set base 55671
             set extra %random.76%
         endif
         *p2 gem
         if (%p% <=9) && (%p%>=7)
          *say p2 %p%555
             set base 55594
             set extra %random.76%
         endif
         *p1 gem
         if (%p% <=6) && (%p%>=2)
         *say p1 %p%
             set base 55566
             set extra %random.27%
         endif
     *no gem
         if (%p% < 2)
         *say no gem - %p%
             set base 0
          set extra 0
         endif
         if %base% > 55560
             eval gem %base% + %extra%
     mload obj %gem%
 sell gem Jhanna
         endif
         eval itt %itt% + 1
     done
endif
~
#30199
A test trigger~
0 g 100
~
if %actor.has_completed[relocate_spell_quest]%
say I see that %actor.name% has completed the relocate spell quest!
else
say So, %actor.name% has not completed the relocate spell quest.
endif
~
$~
