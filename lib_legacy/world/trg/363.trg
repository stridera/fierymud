#36300
chief_guard_rant~
0 i 100
~
wait 2
set rnd %random.char%
if %rnd.vnum% == 36303
poke 2.guard
glare 2.guard
say My granny could do a better job guarding this place than you!
whap 2.guard
say Chest out! Shoulders back!
steam
endif
~
#36301
Smugg_leader_greet~
0 g 100
~
* This is to engage anyone walking in
msend %actor% %self.name% looks around quickly.
if %actor.level% > 99
say Hehe, I _am_ honoured to have such a visitor
bow
else
msend %actor% %self.name% stares at you in disbelief.
mechoaround %actor% %self.name% stares at %actor.name% in disbelief.
say What the HELL do you think you are DOING!?
say You just made the last mistake of your miserable existence!
kill %actor.name%
end
~
#36302
Smugg_guard_rand1~
0 b 10
~
say How am I ever gonna spend my wages stuck in this hole.
sigh
emote looks dejected.
~
#36303
Smugg_guard_rand2~
0 b 10
~
emote looks around furtively.
emote hurriedly munches on a stolen donut.
burp
blush
~
#36304
Smugg_guard_greet1~
0 g 40
~
* This is a cute lil greet for the chief guard.
if %actor.vnum% == 36306
   emote snaps to attention.
else
   switch %actor.quest_stage[illusionist_subclass]%
      case 0
         break
      case 1
      case 2
         wait 2 s
         gasp
         wait 2 s
         say Cestia!  How nice it is to see you again.
         wait 3 s
         smile %actor.name%
         break
      case 3
         wait 2 s
         look %actor.name%
         wait 3 s
         ponder
         break
      case 4
         wait 1 s
         emote looks rather worried.
         wait 3 s
         say I hope they don't make it this far.  I'm not that good at fighting!
         break
      case 5
         wait 1 s
         glare %actor.name%
         wait 2 s
         say Cestia, I heard you brought a posse from Mielikki with you!
         wait 4 s
         say We ought to offer you up to them!
         break
      case 6
         wait 1 s
         say Aha!  If it isn't the little thief herself!
         wait 4
         kill %actor.name%
   done
end
~
#36305
Smugg_chief_rand1~
0 b 10
~
glare
say Something's wrong here, but I just can't put my finger on it.
stomp
~
#36306
Smugg_chief_greet1~
0 g 30
~
msent %actor% The chief guard frisks you quickly and expertly for donuts.
mechoaround %actor% The chief guard frisks %actor.name% quickly and expertly for donuts.
grumble
say Someone's stealing them...
fume
~
#36307
Smugg_chief_entry1~
0 i 30
~
say There'd better not be any pinching of donuts going on round, here.
peer 2.smuggler
say It will come out of your wages if there is.
glare
~
#36308
Smugg_chief_entry2~
0 i 30
~
emote shouts, 'Ok, lets get those crates moving people.'
whap 2.smuggler
emote shouts, 'Move it, move it, MOVE IT!'
~
#36309
Ill-subclass: Invasion illusion ***NOT USED***~
2 a 100
~
wait 3 s
wzoneecho &5The scent of magic is discernable, as a spell builds.&0
wecho &5The scent of magic is discernable, as a spell builds.&0
wait 7 s
wzoneecho &5The magical force is still spreading, and remains low-key.&0
wecho &5The magical force is still spreading, and remains low-key.&0
wait 7 s
wzoneecho &5A spell seems to be coming together.  Slowly, it builds.&0
wecho &5A spell seems to be coming together.  Slowly, it builds.&0
wait 5 s
wzoneecho &5Something supernatural is beginning to coalesce - and the sounds of a militant&0
wzoneecho &0&5crowd are beginning to rise above the threshold of hearing.&0
wecho &5Something supernatural is beginning to coalesce - and the sounds of a militant&0
wecho &0&5crowd are beginning to rise above the threshold of hearing.&0
wait 7 s
wzoneecho &7&bSuddenly, a shout is heard!  It is joined by others, and the sounds of battle commence!&0
wecho &7&bSuddenly, a shout is heard!  It is joined by others, and the sounds of battle commence!&0
wait 2 s
* Now, if Gannigan and the quester are in here, Gannigan should react accordingly.
set gannigan 0
set quester 0
set person %self.people%
while %person%
   if %person.vnum% == 36301
      set gannigan %person%
   elseif %person.vnum% == -1 && %person.quest_stage[illusionist_subclass]% > 1
      set quester %person%
   end
   set person %person.next_in_room%
done
if %quester% && %gannigan%
   if %quester.quest_stage[illusionist_subclass]% == 2
      wecho Test 3
      wforce %gannigan% say What?!  They attack?  Can Mielikki have gone mad?
      wait 2 s
      wforce %gannigan% say Cestia, you must hide.  Do you recall the incantation?
      wforce %gannigan% say The one that will reveal the passage above the falls?
      quest advance illusionist_subclass %quester.name%
      quest advance illusionist_subclass %quester.name%
   elseif %quester.quest_stage[illusionist_subclass]% == 3
      wforce %gannigan% say An attack?  Cestia!  What have you done?  It is betrayal!
      wait 2 s
      wforce %gannigan% glare %quester.name%
      wait 2 s
      wforce %gannigan% say You will not destroy me.  But I cannot bring myself to raise my sword against you.
      quest advance illusionist_subclass %quester.name%
      quest advance illusionist_subclass %quester.name%
   end
end
~
$~
