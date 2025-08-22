#51000
Pawn Shop (Nordus)~
2 d 0
floor phrase is~
* switch on the global variable which was set when they entered
set flagit 0
switch %chosen%
case 1
if %speech% == floor phrase is open sesame
set flagit 1
endif
break
case 2
if %speech% == floor phrase is eingang bitte
set flagit 1
endif
break
case 3
if %speech% == floor phrase is traverser dedans
set flagit 1
endif
break
case 4
if %speech% == floor phrase is let me in dorkus
set flagit 1
endif
break
done
if %flagit% == 1
wecho The ground begins to rumble and the dirt begins to part.
wdoor 51030 d room 51075
wdoor 51030 d description You see flashes of colour in the darkness below...this looks unnatural.
wait 7 s
wecho The ground again begins to tremble as the passageway dissolves.
wait 5 s
wecho The floor returns to normal without a trace of the secret it holds.
wdoor 51030 d purge
endif
~
#51001
maze_room_one~
2 g 100
~
if %exitdone% != 1
* flag to prevent group members retriggering
set exitdone 1
global exitdone
wdoor %self.vnum% north purge
wdoor %self.vnum% south purge
wdoor %self.vnum% west purge
wdoor %self.vnum% east purge
wdoor %self.vnum% up purge
wdoor %self.vnum% down purge
set exitsreq %random.6%
set exitsdone 0
while %exitsdone% < %exitsreq
set exitdir %random.6%
eval toroom 51071 + %random.5%
if %toroom% != %self.vnum%
switch %exitdir%
case 1
wdoor %self.vnum% north room %toroom%
wdoor %self.vnum% north description The entrance wavers slightly as you look at it.
eval exitsdone %exitsdone% + 1
break
case 2
wdoor %self.vnum% south room %toroom%
wdoor %self.vnum% south description The entrance wavers slightly as you look at it.
eval exitsdone %exitsdone% + 1
break
case 3
wdoor %self.vnum% east room %toroom%
wdoor %self.vnum% east description The entrance wavers slightly as you look at it.
eval exitsdone %exitsdone% + 1
break
case 4
wdoor %self.vnum% west room %toroom%
wdoor %self.vnum% west description The entrance wavers slightly as you look at it.
eval exitsdone %exitsdone + 1
break
case 5
wdoor %self.vnum% up room %toroom%
wdoor %self.vnum% up description The entrance wavers slightly as you look at it.
eval exitsdone %exitsdone + 1
break
case 6
wdoor %self.vnum% down room %toroom%
wdoor %self.vnum% down description The entrance wavers slightly as you look at it.
eval exitsdone %exitsdone% + 1
break
done
endif
done
* now add a special case which exits from the maze totally
if %random.100% < 30
wdoor %self.vnum% up room 51077
wdoor %self.vnum% up description This doorway looks more solid than the others.
endif
wait 1 s
unset exitdone
endif
~
#51002
set_encrytped_phrase~
2 g 100
~
if %running% != "yes"
set running yes
global running
unset chosen
set chosen %random.4%
global chosen
* we have 4 known phrases..it would be nice if this were completely dynamic though :)
switch %chosen%
case 1
set descr PLTWK ZHWIL OIXWI ONXML KMJ
break
case 2
set descr PLTWK ZHWIL OIXMB XGFVZ LIYBX
break
case 3
set descr PLTWK ZHWIL OIXBK KVJZL ORIMW KNX
break
case 4
set descr PLTWK ZHWIL OIXTX DMJQG NOWSN C
break
done
wdoor 51030 n description %descr%
wait 2 s
unset running
endif
~
#51003
luchiaans_greeting~
0 g 100
~
if %actor.vnum% == -1 && %actor.level% < 100
   if (%actor.class% /= Necromancer || %actor.class% /= Sorcerer) && %magequest% < 2
   wait 5
   ponder
   wait 2 s
   mechoaround %actor% Luchiaans leans close to %actor.name%.
   whisper %actor.name% You and I could be partners in this, if you want? I could use an extra pair of hands.
   endif
   if %actor.class% /= Warrior
   wait 5
   ponder
   wait 2 s
   poke %actor.name%
   say Hmm... not bad stock for my experiments.  I may save your corpse.
   emote grins diabolically.
   cackle
   endif
   if (%actor.class% /= Cleric || %actor.class% /= Priest) && %clericquest% < 2
   wait 5
   ponder
   wait 2 s
   mechoaround %actor% Luchiaans leans close to %actor.name%.
   whisper %actor.name% Can you teach me some of your healing spells?
   whisper %actor.name% I may have a need for the theory of regeneration.
   endif
endif
~
#51004
only_clerics_enter~
2 g 100
~
if %actor.level% < 100
   if (%actor.class% /= Cleric || %actor.class% /= Priest)
      wsend %actor% You feel a calmness come over you, as if the troubles of the world are washed away.
      if %entry% == ""
         set entry 1
         global entry
      else
         eval entry %entry% + 1
      endif
   else
      wsend %actor% You can't seem to enter the room!  It is like stepping against a solid wall,
      wsend %actor% &0but you can see in.
      wsend %actor% You seem to hear a voice whisper, 'This room is not for you.'
      return 0
   endif
endif
~
#51005
want_to_help_Luchiaans~
0 d 100
partners? teach?~
if %actor.vnum% == -1
   if (%actor.class% /= Cleric || %actor.class% /= Priest)
      set class C
   endif
   if (%actor.class% /= Necromancer || %actor.class% /= Sorcerer)
      set class S
   endif
   if %speech% /= teach?
      if %class% != C
         laugh %actor.name%
         say I have nothing to learn from you, %actor.name% you upstart!
      elseif %clericquest% == 3
         say Thanks, but I don't need any more cleric books.
      else
         say Yes, well, not exactly teach, there is a book of cleric spells in the town, but only a cleric can get
         it.
         chuckle
         say If you bring it to me I will reward you.
         if %clericquest% != 2
            set clericquest 1
            global clericquest
         endif
      endif
   else
      if %class% != S
         pat %actor.name%
         say No way would I want to be partners with anyone but a sorceror.
      elseif %actor.level% < 30
         say Sorry, %actor.name%.  I think you are a bit puny to be my partner at the moment.
      elseif %magequest% == 2
         say Sorry, I've already got everything I need in order to start building up my zombie army.
      else
         smile
         say Great, %actor.name%.
         say Let's get this relationship off to a flying start - bring me a phoenix heart.
         say I need it for my regeneration potions.
         emote rubs his hands together in glee.
         say I think Rana Theroxa has a pet phoenix somewhere near the council chambers.
         set magequest 1
         global magequest
      endif
   endif
endif
~
#51006
non_clerics_out~
2 b 100
~
if %self.people%
   set rnd %random.char%
   if (%rnd.room% == %self.vnum%) && (%rnd.vnum% == -1) && (%rnd.level% < 100) && !(%rnd.class% /= Cleric || %rnd.class% /= Priest)
      wsend %rnd% You feel a force pushing you out of the room - you are powerless to resist!
      wechoaround %rnd% %rnd.name% suddenly starts moving out of the room.
      wteleport %rnd% 51050
      wsend %rnd% You seem to hear a voice whisper, 'This room is not for you.'
      wait 8
      wforce %rnd% look
   endif
endif
~
#51007
open_display_case~
2 d 0
amehs~
if %got_book% == 1
  wsend %actor% You marvel at the resonant tones of your voice, perhaps you should be an actor.
else
  wecho The display case folds and collapses to the floor.
  wpurge display-case
  wload obj 51022
  * load the display case in the holding area to avoid a second oneloading
  wat 51099 wload obj 51021
  set got_book 1
  global got_book
endif
~
#51008
complete_the_book~
1 j 100
~
*we wont hold the magic...
return 0
* actor must be holding the magic book
if %actor.wearing[51022]%
osend %actor% You feel a strong attraction between the book and the magic - you can't hold them apart!
oechoaround %actor% %actor.name% appears to be struggling to keep the book and the magic apart, but vainly.
wait 1 s
oecho The magic melts into the book, and makes it whole!
wait 6
oecho The book glows more and more brightly!
osend %actor% Ack - the book is heating up too! You don't think you can hold it much longer...
oforce %actor% remove book-of-healing
opurge book-of-healing
osend %actor% You drop the book hurriedly to avoid burning yourself!
oechoaround %actor% %actor.name% drops a large book.
oload obj 51023
opurge %self%
else
osend %actor% You can't seem to get a proper grip on it, it almost squirms.
endif
~
#51009
guardian_of_book~
1 g 100
~
if %actor.room% >= 51000 && %actor.room% <= 51099 && %alreadyrun% != 1
   oload mob 51025
   oforce guardian say You are not worthy to handle the book of Nordus!
   set alreadyrun 1
   global alreadyrun
   oforce guardian hit %actor.name%
endif
~
#51010
guardian_of_book_reset~
1 h 100
~
unset alreadyrun
* remove global variable so next get loads guardian >:-)
~
#51011
give_to_luchiaans~
0 j 100
~
switch %object.vnum%
   * damaged book...there is another part to do yet
   case 51022
      wait 5
      say Hmm...a damaged spellbook.
      wait 2 s
      say Look, %actor.name%, why don't you fix this and then maybe we can talk.
      return 0
      set clericquest 2
      global clericquest
      break
   * complete spellbook - did we do this in one go (bonus)
   case 51023
      wait 5
      grin
      say %actor.name%, you've done me proud.
      say Although I guess you haven't really done any favours for the rest of the world!
      cackle
      mpurge book-of-healing
      if %clericquest% == 1
         say And you did it all on your own initiative!  Well, here's a gift you can use.
         mload obj 51024
         give wand %actor.name%
      elseif %clericquest% == 2
         say Even though I had to help you, you did succeed, so here is a small reward.
         mload obj 51025
         give wand %actor.name%
      else
         say Best of all, I didn't even ask you to do this, so I don't owe you anything.
         cackle
      endif
      set clericquest 3
      global clericquest
      break
   case 51028
      wait 5
      say Thanks, %actor.name%.
      say Now I can generate an army of undead, unkillable zombies to help in my plans.
      cackle
      mpurge phoenix-heart
      if %magequest% == 1
         say Oh... about that partner thing.  You know I was joking right?
         emote looks a bit sheepish.
         say Well... here's a token of my thanks, but that's all you're getting.
         mload obj 51029
         give green %actor.name%
         set magequest 2
         global magequest
      else
         say It was very generous of you to give this when I didn't ask for it!
         grin
      endif
      break
   case 8550
      halt
      break
   default
      return 0
      mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
      msend %actor% You give %object.shortdesc% to %self.name%.
      wait 8
      say What am I supposed to do with this?
      msend %actor% %self.name% returns %object.shortdesc% to you.
      mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
done
~
#51012
kill_rana~
2 g 100
~
if %actor.vnum% == 51010
wait 1
wecho An large image of a face appears in the room.
wforce rana say I'll kill you Luchiaans, but slowly
wecho %actor.name% waves the petrified magic defiantly.
wforce rana say This will protect me against you.
wait 5
wecho The aparition laughs and says a couple of words.
wdamage %actor% 10000
wecho The aparition chuckles and says, 'Well, HE underestimated my power bigtime.'
wait 5
wecho The aparition fades.
endif
~
#51013
phoenix_croaked~
0 f 100
~
return 0
mecho The phoenix closes its eyes and bursts into flame!
mload obj 51028
mexp %actor% -28000
~
#51014
get_heart_of_phoenix~
1 g 100
~
* this is only an issue if the heart has not already been got...
   if %already_got% != 1
* need to be wearing (on hands) object 51026 to get heart
      if %actor.wearing[51026]%
         osend %actor% The corpse is extremely hot and may combust soon!
         oexp %actor% 30000
         return 1
         set already_got 1
         global already_got
    else
         osend %actor% The corpse is too hot to touch without special protection!
         return 0
   endif
         wait 2 s
         oecho The corpse suddenly crumbles to ash.
endif
~
#51015
phoenix_lives_again~
2 d 100
p PHOENIX IS TOAST~
* We purge phoenix a lot here. This is to ensure that the phoenix's
* corpse goes away. Don't want to "purge corpse" because there might
* be a player corpse. Purge multiple times on the random off
* chance that there might be a phoenix key or something on the ground.
wat 51079 wpurge phoenix
wat 51079 wpurge phoenix
wat 51079 wpurge phoenix
wat 51079 wpurge phoenix
wat 51079 wpurge phoenix
wat 51079 wecho The heat of the combustion scorches everything!
wait 2s
wat 51079 wecho The phoenix rises again from the ashes!
wat 51079 wload mob 51026
wat 51079 wforce phoenix whistle
~
#51016
rana_receive1~
0 j 100
51017~
emote examines her new prize briefly.
say So...this is it eh?  The legendary protection spell!
remove page
drop page
hold magic
follow %actor.name%
say Take me to Luchiaans now - I am ready to terminate his sorry existence!
~
#51017
rana_speak1~
0 d 100
cleaner?~
say Yes...
ponder
say I wonder where he is now, or even if he survived? The only reason I am still here is as a punishment.
growl
wait 1
say But if I ever catch that Luchiaans, I'll make him regret this.
think
say Of course, as soon as I got within striking distance he'd fry me with his magic.
sigh
wait 1
say I will have to just plan my revenge for now.
daydream
~
#51018
rana_speak2~
0 d 100
shema?~
look %actor.name%
nod
wait 1
say Yes, Shema.  She used to be the cleric on the council until that upstart sorcerer came along.
sigh
say He fooled us all, and then a cleaner came across a book in his office that suddenly shed light on a lot of things.
shake
wait 1
say All that stuff about the insane and how he might be able to help them.
whap me
say But now he has gone to ground and we are left to pick up the pieces of our once proud town.
cry %actor.name%
~
#51019
rana_rand1~
0 b 20
~
sigh
say I should have known it was all too good to be true.  I should have listened to Shema.
whap me
~
#51020
new trigger~
2 d 100
run test~
wecho trigger running
wait until 00:00
wecho trigger over
~
#51021
crazed_survivor_speak1~
0 d 100
book?~
say Yes, I used to be a cleaner in the Council chambers and one day I found a book in the corner of Luchiaans' office.
emote scratches his chin.
say I told the Council leader about it, and three days later the town is like it is now.
say Nothing has happened since, but I can feel something brewing.
wait 1
shrug
say I presume the book is still there, leastways I didn't touch it, not after what I'd read from it.
emote rubs his arms as if he is suddenly cold.
~
#51022
crazed_survivor_speak2~
0 d 100
magic?~
peer %actor.name%
say Hmm..yes..it's been in my family for years and we were taught as children that if we held it, we would be protected from some spells.
say Although, this is the only spell that has been affected by it.  All my neighbors became zombies in one night and I was the only one to remain sane.
shiver
say I wish I'd never found that stupid book now.
sigh
~
#51023
crazed_survivor_greet1~
0 g 100
~
panic
say You have come for my magic haven't you?
say Well you can't have it, you can't!  I need it!
~
#51024
Unused trigger~
0 g 100
incarusht~
ponder %actor.name%
say You are attentive, very impressive.
mteleport %actor.name% 51071
~
#51025
luchiaans_death~
0 f 100
~
mecho Before his death, Luchiaans silently chants his final incantation.
mecho You briefly see a flash of light, and you hear a loud BOOOM!
mteleport all 51000
mat 51000 mforce all look
~
#51026
common_child_speak1~
0 d 100
kafit?~
mecho Both the child's heads focus their attention on you.
mecho One head says, 'That is the key to decoding Luchiaans' spells.'
mecho The other head says, 'Avenge us, adventurer!'
beg %actor.name%
~
#51027
common_child_rand1~
0 b 50
~
mecho One head looks at the other and says, 'So how do you know the phrase is kafit, and I don't?'
mecho The second head smirks disturbingly, its eyes flat and lifeless.
mecho The second head responds, 'Because I was awake before you when he had to decode his final spell.'
~
$~
