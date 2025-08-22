#8300
Load_Guard~
2 g 100
~
if %actor.align% < -349
wait 2
wecho A giant guard pops out from behind the coffin.
wat 8423 wteleport frakatiguard 8363
end if
~
#8301
Random_mob_generate~
0 k 25
~
roar
wait 2
em yells out for a guard to assist him.
mload mob 8300
~
#8302
Give_pork~
0 h 100
~
if %actor.vnum% == -1
     if %actor.align% > 349
          mload obj 8350
         say Eat up while it's still hot.
         wait 1s
         give pork %actor.name%
         mforce %actor.name% eat pork
     endif
endif
~
#8303
Death_reload~
0 f 100
~
mat 8423 mload mob 8309
~
#8304
Frakati guard blocker~
2 g 100
~
* This is a general trigger for mobiles guarding certain exits.
* To use it, make a copy of the trigger, and modify these two variables.
* Then apply the trigger to the room beyond the guard.
set entryroom 8352
set guardvnum 8332
if %get.mob_count[%guardvnum%]% && %actor.room% == %entryroom% && %actor.vnum% == -1
   set blocked 0
   set person %self.people%
   while %person%
      if %person.vnum% == %guardvnum%
         set guard %person%
         if %guard.position% /= Standing || %guard.position% /= Fighting || %guard.position% /= Flying
            set blocked 1
         end
      end
      set person %person.next_in_room%
   done
   if %blocked%
      if %actor.level% < 100
         set entrydir %get.opposite_dir[%direction%]%
         wat %actor.room% wechoaround %actor% %guard.name% blocks %actor.name% as %actor.heshe% tries to go %entrydir%.
         wat %actor.room% wsend %actor% %guard.name% steps purposefully into your way.
         return 0
      else
         wat %actor.room% wechoaround %actor% %guard.name% makes no move as %actor.name% passes.
         return 1
      end
   else
      return 1
   end
else
   return 1
end
~
$~
