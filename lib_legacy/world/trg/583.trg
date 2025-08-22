#58375
KoD_demisekeep_newbie_guard~
0 c 100
east~
if %actor.vnum% == -1
   if %actor.level% < 30
      smile
      whisper %actor.name% I would not suggest going any further.
      wait 1
      grin %actor.name%
      whisper %actor.name% It is fraught with danger above your abilities.
   elseif %actor.level% < 60
      say I will let you pass, but still be careful beyond this point.
      emote points east.
      wait 5
      mforce %actor% east
   else
      return 0
   end
endif
~
#58380
holy_burn_the_wicked~
1 j 100
~
if %actor.class% == Paladin
  if %actor.align% < 750
     wait 2
     oecho the holy blade of Godly vigor dulls in appearance.
     osend %actor% You are no longer holy enough to use this sacred weapon, you can no longer wield it.
     oechoaround %actor% %actor.name%'s blade grows dull, unglowing.
     oforce %actor% remove vigor
  elseif %actor.align% >= 750
     wait 2
     oecho the holy blade of Godly vigor glows with a soft, white light.
     osend %actor% Your eyes widen with awe and splendor as you hold the holy blade of Godly vigor.
     oechoaround %actor% %actor.name% is surrounded by a holy glow.
  endif
elseif %actor.class% /= Paladin
  if %actor.align% < -350
     wait 2
     oecho the holy blade of Godly vigor glows violently!
     osend %actor% The pain in your body is excruciating, you feel death.
     eval var_dam = %random.50% + 1517
     oechoaround %actor% %actor.name% convulses and life drains from his body. (&3%var_dam%&0)
     odamage %actor% %var_dam%
     * Note: this oforce will cause syserror if damage kills victim *
     oforce %actor% remove vigor
  elseif %actor.align% >= -350
     wait 2
     oecho the holy blade of Godly vigor vibrates unsteadily.
     osend %actor% This blade clearly is not happy to be in your hands.  Take care.
     oechoaround %actor% %actor.name%'s hands shake unsteadily as he holds the blade.
     oforce %actor% remove vigor
  endif
endif
~
$~
