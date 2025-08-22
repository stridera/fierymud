#1400
GoT_Arch_Exit_Trigger~
2 d 100
aderci~
wait 1
if %actor.wearing[1406]%
    * Check to see if the door field is open/unlocked, if not lets
    * set a variable and unlock/open it.  If so, close/lock it
    * and unset the variable.
   if %got_hall_open% != 1
      * Door closed and locked, lets open it!
      set got_hall_open 1
      global got_hall_open
      wdoor 1400 south flags acd
      wdoor 1400 south room 2610
      wdoor 1400 south name entry arch
      wdoor 1400 south description A large archway opens up to one of Caelia's southern caravan routes.
      wecho The barrier protecting the entry archway flares momentarily as it powers down.
      * create the other side and link.
      wdoor 2610 north flags acd
      wdoor 2610 north room 1400
      wdoor 2610 north description A large archway leads into an inviting, stone structure.
      wat 2610 wecho The northern archway's protective barrier flares momentarily as it powers down.
    else
      * Door open, lets close and lock it!
      set got_hall_open 0
      global got_hall_open
      wdoor 1400 south flags abcd
      wdoor 1400 south room 2610
      wdoor 1400 south description A large archway is protected by a near-ethereal, humming, force field.
      wecho The barrier protecting the entry archway flares momentarily as it powers up.
      * create the other side and link
      wdoor 2610 north flags abcd
      wdoor 2610 north room 1400
      wdoor 2610 north name entry arch
      wdoor 2610 north description The archway leading into the structure is protected by a force field.
      wat 2610 wecho The northern archway's protective barrier flares momentarily as it powers up.
   endif
else
   * No Eyes of Truth, no exit!
endif
~
#1401
GoT_Arch_Entry_Trigger~
2 d 100
aderci~
wait 1
if %actor.wearing[1406]%
    * Check to see if the door field is open/unlocked, if not lets
    * set a variable and unlock/open it.  If so, close/lock it
    * and unset the variable.
   if %got_hall_open% != 1
      * Door closed and locked, lets open it!
      set got_hall_open 1
      global got_hall_open
      wdoor 1400 south flags acd
      wdoor 1400 south room 2610
      wdoor 1400 south name entry arch
      wdoor 1400 south description A large archway opens up to one of Caelia's southern caravan routes.
      wat 1400 wecho The barrier protecting the entry archway flares momentarily as it powers down.
      * create the other side and link.
      wdoor 2610 north flags acd
      wdoor 2610 north room 1400
      wdoor 2610 north description A large archway leads into an inviting, stone structure.
      wecho The northern archway's protective barrier flares momentarily as it powers down.
    else
      * Door open, lets close and lock it!
      set got_hall_open 0
      global got_hall_open
      wdoor 1400 south flags abcd
      wdoor 1400 south room 2610
      wdoor 1400 south description A large archway is protected by a near-ethereal, humming, force field.
      wat 1400 wecho The barrier protecting the entry archway flares momentarily as it powers up.
      * create the other side and link
      wdoor 2610 north flags abcd
      wdoor 2610 north room 1400
      wdoor 2610 north name entry arch
      wdoor 2610 north description The archway leading into the structure is protected by a force field.
      wecho The northern archway's protective barrier flares momentarily as it powers up.
   endif
else
   * No Eyes of Truth, no exit!
endif
~
#1402
GoT recall 1401~
1 c 1
rub~
if %arg% /= truth ||  %arg% /= eye || %arg% /= eyes
osend %actor% You gently rub %self.shortdesc%
oechoaround %actor% %actor.name% gently rubs %self.shortdesc%
wait 1s
oechoaround %actor% A bright white portal appears, and draws %actor.name%, in.
osend %actor% A white light appears and embraces you, to your very soul.
wait 2s
oteleport %actor% 1401
osend %actor% The bright lights embrace you only for a moment before setting you back into your world.
osend %actor% The words "Guard the Truth well" repeat in your mind.
oechoaround %actor% A bright white portal flashes into view as %actor.name%, steps out of it.
osend %actor% You blink and realize you are not where you started.
wait 1s
oforce %actor% look
elseif
osend %actor% Huh?!?
endif
~
#1403
new trigger~
0 g 3
rub~
wait 1s
if %arg% /= truth ||  %arg% /= eye || %arg% /= eyes 
~
$~
