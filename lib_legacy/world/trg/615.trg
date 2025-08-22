#61500
Portly gnome being tickled~
0 c 100
tickle gnome~
* This is a trigger for tickling the portly gnome in the Enchanted Hollow.
* Tickling makes him float up into the air with glee.
* If there's a cherry up in a tree, he'll grab it and end up dropping it,
* thus making it accessible to players.
if %arg% /= gnome
   set rightobj 1
endif
if %arg% /= portly
   set rightobj 1
endif
if %rightobj% ==1
   mechoaround %actor% %actor.name% tickles %self.name%.
   msend %actor% You tickle %self.name%.
   wait 1
   emote doubles over, giggling!
   wait 5
   emote loses contact with the ground, spinning upward into the air...
   wait 10
   get purple-cherry-in-tree
   wait 10
   emote descends slowly back to the ground.
else
   return 0
endif
~
#61501
Getting a purple cherry~
1 g 100
~
* This is a trigger for taking purple cherries in the Enchanted Hollow.
* Normally, they're up in the tree, so a player wouldn't be able to
* reach. The preferred way to access the cherries is to tickle a
* gnome, who will float up and grab one, and then drop it.
return 0
if %actor.vnum% == 61550 || %actor.vnum% == 61500
   oecho %actor.name% tries to grab a purple cherry, but accidentally knocks it down!
   wait 1
   oecho A purple cherry falls to the ground.
   oload obj 61551
   opurge %self%
else
   oechoaround %actor% %actor.name% tries to take a purple cherry, but it's out of reach.
   osend %actor.name% You can't reach that high!
end
~
#61502
Fairy tickles people~
0 bg 40
~
wait 4
set val %random.3%
switch %val%
case 1
set ch %random.char%
tickle %ch.name%
break
default
tickle gnome
done
~
#61503
Caterpillar munching~
0 b 10
~
set val %random.10%
switch %val%
case 1
emote rears up and twists around wildly, seeking a new grip.
break
case 2
emote crawls slowly up a stem.
break
case 3
emote crawls slowly down a stem.
break
default
emote munches on a milkweed leaf.
done
~
#61504
Spark pixy offers help~
0 g 100
~
if %direction% /= north && %heart_inplace% != 1 && %actor.vnum% == -1 && %actor.level% < 100
   wait 1 s
   if %actor.room% != %self.room%
      halt
   endif
   mechoaround %actor.name% %self.name% bows before %actor.name% (as much as a flying pixie can).
   msend %actor% %self.name% bows before you, rather well for someone flying in midair.
   wait 1 s
   if %actor.room% != %self.room%
      halt
   endif
   say If you're having trouble with the fog, I might be able to help.
   say Would you like me to help?
   set person_to_help %actor.name%
   global person_to_help
   set greeted_someone 1
   global greeted_someone
   wait 1 s
end
~
#61505
Spark pixy makes an offer~
0 d 100
yes~
if %greeted_someone% == 1 && %actor.name% /= %person_to_help% && %heart_inplace% != 1
   wait 1 s
   say Well, sometimes the dark fliers put something very bright on this menhir,
   say which burns away the fog.
   wait 3 s
   giggle
   wait 1 s
   say Whatever it is, its heat and light makes the poor dears whimper so piteously...
   wait 3 s
   whisper %actor.name% I think there's a secret tunnel hidden somewhere up to north.
   whisper %actor.name% But it's hard to find in all that fog.
   wait 3 s
   say I'm not sure what it is, exactly, but if you bring me something bright,
   say I'll put it up on top of the menhir.  Then we'll see what happens!
   wait 1 s
end
~
#61506
The faint sound of trickling water~
2 b 20
~
wecho You hear the faint sound of trickling water.
~
#61507
Sapsucker harassing tree~
0 b 20
~
set val %random.20%
switch %val%
case 1
emote pecks loudly on the tree trunk.
break
case 2
emote eyes you suspiciously.
break
case 3
emote hops up the trunk, looking for a good place to make a hole.
break
case 4
emote flutters over to an adjacent tree.
break
done
~
#61508
The sound of trickling water~
2 b 10
~
wecho You hear the distinct sound of trickling water.
~
#61509
Malpinscher blocks western movement~
0 c 100
west~
mechoaround %actor% %self.name% moves quickly to block %actor.name%.
msend %actor% %self.name% quickly blocks your path!
~
#61510
Fiery wisp tries to get attention~
0 b 10
~
if %self.room% == 61538
   mat 61537 mecho You notice a bright, moving light just to the north - too bright to be a firefly.
endif
~
#61511
Giving a wisp heart to someone~
0 j 100
~
if %heart_inplace% == 1
   * Note: the mud appears to prevent this trigger from being activated
   * again when it's in progress, so this really does nothing, but I've
   * left it here just in case.
   return 0
   msend %actor% &b%self.name% tells you, 'Hang on a minute!'&0
else
   return 1
   set heart_inplace 1
   global heart_inplace
   wait 2 s
   emote looks over %object.shortdesc% carefully.
   wait 3 s
   msend %actor% &b%self.name% tells you, 'Ok, let's see if this does any good.'&0
   wait 3 s
   emote flutters up to the top of the menhir and puts %object.shortdesc% in the depression.
   if %object.vnum% == 61504
      mjunk fiery-wisp-heart
      m_run_room_trig 61512
   else
      wait 5 s
      emote peers thoughtfully at %object.shortdesc%, with her tiny chin held between her thumb and forefinger.
      wait 3 s
      emote zips up to the top of the menhir and yanks %object.shortdesc% out of the hole.
      wait 2 s
      say I guess that wasn't it!
      drop %object.name%
   end
   set heart_inplace 0
   global heart_inplace
end
~
#61512
Fog-burning light shines from menhir~
2 a 100
~
wait 5 s
wat 61547 wecho With a burst of blinding light, a fiery wisp heart begins to shine!
wat 61547 wecho Bright &3or&1an&3ge&0 rays of light shoot northward, burning away the fog.
wat 61548 wecho Bright rays of blinding orange light shine in from the south!
wat 61548 wecho The fog cannot withstand the light, and quickly dissipates.
wdoor 61548 north flags abe
wdoor 61548 north room 61553
wdoor 61548 north name slab
wdoor 61548 north description A massive carved slab with a small panel lies against the hillside.
wdoor 61553 south flags abcde
wdoor 61553 south room 61548
wdoor 61553 south name slab
wdoor 61553 south description A smooth, featureless slab covers the south exit.
wait 4 s
wat 61547 wforce spark-pixie emote gazes up at a fiery wisp heart, mesmerized.
wait 6 s
wat 61547 wecho Small wisps of fog try to drift in to the north, but are burned away.
wat 61548 wecho Small wisps of fog try to drift in, but are burned away.
wait 9 s
wat 61547 wecho The fog is still flowing into the area to the north, but it cannot survive the light.
wat 61548 wecho The fog is still flowing in, but it cannot survive the light.
wait 15 s
wat 61547 wecho The reddish-orange light of a fiery wisp heart is beginning to fade.
wat 61548 wecho The reddish-orange light from the south is beginning to fade.
wait 15 s
wat 61547 wecho The fog is starting to thicken again as the light from the obelisk fades.
wat 61548 wecho The fog is starting to thicken again as the light from the south fades.
wait 9 s
wat 61547 wecho The light of a fiery wisp heart finally blinks out.
wat 61548 wecho The fog drifts back into the area.
wat 61553 wecho With a low groaning sound, the slab to the south rises up and blocks the exit.
wdoor 61548 north purge
wdoor 61553 south purge
~
#61513
Wielding an emblazoned flint knife~
1 j 100
~
if %actor.vnum% == -1
   if %actor.room% == 61549 || %actor.room% == 61566
      return 0
      * If you wield the emblazoned flint knife in a room that
      * could have webs, it gets excited and flies out of your hand.
      wait 2
      oechoaround %actor% %self.shortdesc% suddenly wriggles, and %actor.name% drops it!
      osend %actor% %self.shortdesc% suddenly wriggles, and you drop it!
      oforce %actor% drop emblazoned-flint-knife
      * Now a drop trigger in the room will determine if some webs
      * get cut, and so forth.
   end
end
~
#61514
Magic webs get cut~
2 h 100
~
if %object.vnum% == 61505 && %web_present% == 1
   * An emblazoned flint knife has been dropped in this room, where
   * a web is blocking an exit.
   wait 1 s
   wecho The knife flies at the web, slicing back and forth.  It cuts easily through the strands.
   wecho The web is shredded.  Its translucent fragments drift away on a light breeze.
   * The web is implemented by a useless exit.  Replace the bad exit
   * with an ordinary one.
   wdoor 61549 east purge
   wdoor 61549 east room 61550
   wpurge blocking-web
   set web_present 0
   global web_present
   * The emblazoned flint knife's magic is used up:
   * replace it with the ordinary flint knife.
   wpurge emblazoned-flint-knife
   wload obj 61506
   * The spider, if present, may attempt to build a new web at any time.
   * However, we'd rather it attack the player who destroyed the web first.
   * Therefore, set this variable to prevent web-building for a while.
   set web_pause 1
   global web_pause
   * Determine whether the spider is present.
   set spider %self.people[61517]%
   if %spider%
      * Now, the spider's response to this insolence.
      wait 1 s
      wforce %spider% emote squeaks furiously!
      wait 4 s
      wforce %spider% glare %actor.name%
      wait 1 s
      wforce %spider% kill %actor.name%
   end
   wait 10 s
   set web_pause 0
   global web_pause
   * The spider may now build the web (it won't try while it's in combat).
end
~
#61515
Web blocking eastward movement~
2 c 100
east~
if %web_present% == 1
   return 1
   wechoaround %actor% %actor.name% tries to walk through a glistening web, and nearly cuts %actor.o%self!
   wsend %actor% You find the delicate-looking web completely impassable.
else
   return 0
end
~
#61516
Spider builds webs~
0 ab 100
~
if %in_battle% != 1
   if %self.room% == 61549
      m_run_room_trig 61517
   end
end
set in_battle 0
global in_battle
~
#61517
Room lets spider create east-blocking webs~
2 a 100
~
if %web_present% != 1 && %web_pause% != 1
   set web_present 1
   global web_present
   wforce potbellied-orb-spider emote carefully tosses a leader thread across the path.
   wait 4
   wforce potbellied-orb-spider emote crosses the path several more times, then spins the rest of its web.
   wdoor 61549 east flags bcd
   wdoor 61549 east description A delicate-looking web stretches between two trees, blocking the path.
   wload obj 61509
end
~
#61518
Mobs know they are in battle~
0 k 100
~
set in_battle 1
global in_battle
~
#61519
Wise leprechaun greets~
0 g 40
~
if %actor.vnum% == -1
   wait 6
   if %actor.sex% == female
      say Well hello there, little lassie!
   else
      say Well hello there, whippersnapper!
   end
   wait 5
   bow %actor.name%
end
~
#61520
Wise leprechaun responds to bowing~
0 c 100
bow~
switch %cmd%
  case b
    return 0
    halt
done
if %actor.vnum% == -1
  return 1
  mechoaround %actor% %actor.name% bows before %self.name%.
  msend %actor% you bow before %self.name%.
  wait 1 s
  if %actor.sex% == female
    set noun lassie
  elseif %actor.sex% == male
    set noun laddie
  else
    set noun my dear
  endif
    say Well now, %noun%, I don't suppose you have any fruit on ye?
  wait 2 s
  say If you bring me some, I can help you with those awful spiders, oh yes...  Mind you, I'm particular to cherries.
else
  return 0
endif
~
#61521
Wise leprechaun responds to curtseying~
0 c 100
curtsey~
switch %cmd%
  case c
  case cu
  case cur
    return 0
    halt
done
if %actor.vnum% == -1
  return 1
  mechoaround %actor% %actor.name% bows before %self.name%.
  msend %actor% you bow before %self.name%.
  wait 1 s
  if %actor.sex% == female
    set noun lassie
  elseif %actor.sex% == male
    set noun laddie
  else
    set noun my dear
  endif
    say Well now, %noun%, I don't suppose you have any fruit on ye?
  wait 2 s
  say If you bring me some, I can help you with those awful spiders, oh yes...  Mind you, I'm particular to cherries.
else
  return 0
endif
~
#61522
Wise leprechaun random babbling~
0 b 40
~
set val %random.5%
switch %val%
case 1
say I sure could use some fruit... mmmhmm, yep.
break
case 2
case 3
say I don't suppose you whippersnappers are having any trouble with spiders, eh?
break
case 4
say Golblasted fairies, always zipping around your head squeaking.
say It's enough to drive you crazy!
break
done
~
#61523
Wise leprechaun discusses webs~
0 d 100
web webs web? webs?~
wait 6
say If it's spiders that trouble ye, I'm the right one to help ye.
say Those webs are right tough, ain't they?
wait 4 s
say I'll tell ye what.  Bring me one o' them pointy flint-knives,
say and a nice cherry, and I'll enchant it for ye sure.
wait 3 s
peer %actor.name%
wait 4
say Well, what be ye still doing here?  Scurry off with ye!
~
#61524
Wise leprechaun receiving goodies~
0 j 100
~
switch %object.vnum%
case 61501
case 12001
case 146
case 147
   wait 4
   emote sniffs %object.shortdesc% carefully.
   wait 1 s
   emote munches heartily on %object.shortdesc%, juice running down his cheeks.
   mjunk %object.name%
   if %actor.sex% == male
      say Why thank you, good sir!
   else
      say Why thank you, my dear!
   end
   wait 1s
   say But that's not my favorite fruit is it, oh no.
   break
case 61502
   * spotted apple
   wait 8
   emote turns %object.shortdesc% over and peers at it thoughtfully.
   wait 1 s
   emote swiftly eats %object.shortdesc% with loud chewing noises.
   mjunk %object.name%
   if %actor.sex% == female
      say Well that was nice, wasn't it my darling?
   else
      say Well that was nice, wasn't it my boy?
   end
   wait 1s
   say But it just isn't what I'm looking for.
   wait 4
   sigh
   break
case 61503
   * This is the wizened apple.
   wait 8
   emote stares daggers at %object.shortdesc%, as if he'd been handed a spider.
   wait 1 s
   emote angrily hurls %object.shortdesc% off into the bushes!
   mjunk %object.name%
   wait 4
   if %actor.sex% == female
      say Well now what's the big idea, lassie?
   elseif %actor.sex% == male
      say Well now what's the big idea, laddie?
   else
      say Well now what's the big idea?
   end
   wait 4
   mecho %self.name% says, 'That was an insult of a fruit, if ever there was one!'
   break
case 61551
   wait 8
   emote gazes lovingly at %object.shortdesc%.
   wait 1 s
   if %got_cherry% == 1
      mecho %self.name% says, 'I've already had one of these... but I can't turn them down!'
   end
   wait 4
   set got_cherry 1
   global got_cherry
   emote happily pops %object.shortdesc% into his mouth and chews gleefully!
   wait 8
   mjunk %object.name%
   mecho %self.name% says, '%actor.name%, I thank you from the bottom of my heart.'
   wait 1s
   say That was a rare treat.
   wait 2 s
   say 
   mecho %self.name% says, 'Alright then, hand me a knife and I'll enchant it for ye.'
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
         if %person.quest_stage[enchanted_hollow_quest]% == 0
            quest start enchanted_hollow_quest %person.name%
         endif
         quest variable enchanted_hollow_quest %person.name% got_cherry 1
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
   break
case 61506
   if %got_cherry%
      unset got_cherry
   endif
   if %actor.quest_variable[enchanted_hollow_quest:got_cherry]% == 1
      wait 8
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
               if %person.quest_stage[enchanted_hollow_quest]% == 0
                  quest start enchanted_hollow_quest %person.name%
               endif
               quest variable enchanted_hollow_quest %person.name% got_cherry 0
            elseif %person%
               eval i %i% + 1
            endif
            eval a %a% + 1
         done
      say Well now!  I suppose I owe you an enchantment!
      wait 4 s
      emote turns and thrusts %object.shortdesc% into the earth.
      wait 2 s
      emote says a few magical words, then touches %object.shortdesc% on the hilt.
      wait 4 s
      mjunk %object.name%
      emote pulls an emblazoned flint knife out of the ground.
      mload obj 61505
      wait 2 s
      give emblazoned-flint-knife %actor.name%
      wait 1 s
      mecho %self.name% says, 'If you see a web, just drop this nearby it.  I've given it an innate hatred of webs, you'll see!'
   else
      return 0
      mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
      msend %actor% You give %object.shortdesc% to %self.name%.
      wait 8
      say Eh?  What have you done for me lately?
      wait 2 s
      mecho %self.name% gives %object.shortdesc% back.
   end
   break
case 61504
   return 1
   wait 8
   emote quickly shields his eyes from %object.shortdesc%'s intense glow.
   say Ach!  Are you trying to blind me?
   wait 8
   drop %object.name%
   break
case 61505
   return 0
   mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
   msend %actor% You give %object.shortdesc% to %self.name%.
   wait 8
   say What, not good enough for ye is it?  Well I'm afraid it'll have to do!'
   wait 12
   msend %actor% %self.name% returns %object.shortdesc% to you.
   mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
   break
case 55755
   * dried fruit
   return 0
   mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
   msend %actor% You give %object.shortdesc% to %self.name%.
   wait 1 s
   emote looks wistfully at %object.shortdesc%.
   mecho %self.name% says, 'Ach, what a waste.  Sorry, it's no use trying to eat this now!'
   wait 2 s
   msend %actor% %self.name% returns %object.shortdesc% to you.
   mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
   break
case 62076
   * banana
   return 0
   mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
   msend %actor% You give %object.shortdesc% to %self.name%.
   wait 4
   emote seems a bit perplexed.
   emote sniffs %object.shortdesc%.
   wait 2 s
   say Well now... it smells nice, doesn't it?
   peer %actor.name%
   emote licks %object.shortdesc% carefully.
   wait 3 s
   say I'm not sure I want to bite this, though, do I?
   msend %actor% %self.name% returns %object.shortdesc% to you.
   mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
   break
default
   return 0
   mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
   msend %actor% You give %object.shortdesc% to %self.name%.
   wait 8
   mecho %self.name% says, 'Now I wouldn't know what to do with this, now would I?'
   msend %actor% %self.name% returns %object.shortdesc% to you.
   mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
done
~
#61525
**UNUSED**~
0 c 100
bo cur~
return 0
~
#61526
Lost baby flier emotes~
0 b 30
~
switch %random.7%
case 1
emote tries to hop behind a tree trunk.
break
case 2
emote squeaks in terror!
break
case 3
emote flaps its little wings furiously.
break
case 4
emote peers up at you with frightened eyes.
break
case 5
emote tries desperately to crawl under a little pile of leaves.
break
case 6
emote pounces on an unsuspecting insect.
wait 3 s
emote crunches noisily for a few seconds, then emits a tiny *burp*.
break
done
~
#61527
Ordinary leprechaun greet~
0 g 60
~
if %actor.vnum% == -1 && %actor.level% < 50
   wait 8
   say Oh no you can't have it!  It's mine!
   wait 3 s
   glare %actor.name%
end
~
#61528
Brownie girl gives you a flower~
0 g 35
~
if %actor.vnum% == -1 && %actor.level% < 100 && %gave_flower% != 1
   wait 1 s
   smile %actor.name%
   wait 2 s
   remove buttercup
   give buttercup %actor.name%
   set gave_flower 1
   global gave_flower
   wait 2 s
   if %actor.room% != %self.room%
      halt
   end
   msend %actor.name% %self.name% curtseys daintily before you.
   mechoaround %actor.name% %self.name% curtseys daintily before %actor.name%.
end
   
~
#61529
Stonefang attacks~
2 g 60
~
if %actor.vnum% == -1 && %actor.level% < 31
   wait 2
   wechoaround %actor% %actor.name% triggered a hidden tripwire!
   wsend %actor% You triggered a hidden tripwire!
   wait 8
   wat 61599 wforce stonefang d
   wforce stonefang kill %actor%
end
~
#61530
Magic webs get cut (north)~
2 h 100
~
if %object.vnum% == 61505 && %web_present% == 1
   * An emblazoned flint knife has been dropped in this room, where
   * a web is blocking an exit.
   wait 1 s
   wecho The knife flies at the web, slicing back and forth.  It cuts easily through the strands.
   wecho The web is shredded.  Its translucent fragments drift away on a light breeze.
   * The web is implemented by a useless exit.  Replace the bad exit
   * with an ordinary one.
   wdoor 61566 north purge
   wdoor 61566 north room 61565
   wpurge blocking-web
   set web_present 0
   global web_present
   * The emblazoned flint knife's magic is used up:
   * replace it with the ordinary flint knife.
   wpurge emblazoned-flint-knife
   wload obj 61506
   * The spider, if present, may attempt to build a new web at any time.
   * However, we'd rather it attack the player who destroyed the web first.
   * Therefore, set this variable to prevent web-building for a while.
   set web_pause 1
   global web_pause
   * Determine whether the spider is present.
   set spider %self.people[61521]%
   if %spider%
      * Now, the spider's response to this insolence.
      wait 1 s
      wforce %spider% emote growls angrily!
      wait 4 s
      wforce %spider% glare %actor.name%
      wait 1 s
      wforce %spider% kill %actor.name%
   end
   wait 10 s
   set web_pause 0
   global web_pause
   * The spider may now build the web (it won't try while it's in combat).
end
~
#61531
Web blocking northward movement~
2 c 100
north~
if %web_present% == 1
   return 1
   wechoaround %actor% %actor.name% tries to walk through a glistening web, and nearly cuts %actor.o%self!
   wsend %actor% You find the delicate-looking web completely impassable.
else
   return 0
end
~
#61532
Orbweaver builds a web to the north~
0 ab 100
~
if %in_battle% != 1
   if %self.room% == 61566
      m_run_room_trig 61533
   end
end
set in_battle 0
global in_battle
~
#61533
Room lets spider create north-blocking web~
2 a 100
~
if %web_present% != 1 && %web_pause% != 1
   set web_present 1
   global web_present
   wforce orbweaver emote carefully tosses a leader thread across the path.
   wait 4
   wforce orbweaver emote crosses the path several more times, then spins the rest of its web.
   wdoor 61566 north flags bcd
   wdoor 61566 north description A delicate-looking web stretches between two trees, blocking the path.
   wload obj 61511
end
~
#61534
Shadow pixie's rambling~
0 k 20
~
switch %random.7%
case 1
say You dare to intrude?
break
case 2
say Niaxxa will hear about this...
break
case 3
say These shadows still hold dark shades, flatlander!
break
case 4
say You are clever to come so far, but the queen still awaits.
break
case 5
say This one is not like the others.
break
done
~
#61535
Niaxxa ejects her opponent~
0 k 100
~
if !%self.wearing[61514]%
   get singing
   wield singing
elseif %actor.level% < 20 && %self.wearing[61514]% && %self.room% == 61567
   set outcome 0
   switch %actor.level%
      case 1
      case 2
      case 3
      case 4
         eval outcome %random.8%
         break
      case 5
      case 6
      case 7
      case 8
         eval outcome %random.10%
         break
      case 9
      case 10
      case 11
      case 12
      case 13
      case 14
      case 15
         eval outcome %random.15%
         break
      default
         eval outcome %random.20%
   done
   if %outcome% == 1
      wait 3
      switch %random.3%
         case 1
            say I tire of your foolishness.
            break
         case 2
            say You waste my time.
            break
         default
            say You bore me, %actor.class%.
      done
      msend %actor% %self.name% swiftly loops her chain around your exposed wrist, and yanks!
      mechoaround %actor% %self.name% loops her chain around %actor.name%'s wrist, and yanks it!
      msend %actor% You are sent reeling out of the room!
      mechoaround %actor% %actor.name% is sent reeling out of the room!
      eval dest 61567 + %random.6%
      mteleport %actor% %dest%
   end
end
~
#61536
Wise leprechaun responds to 'no'~
0 d 100
no~
if %actor.vnum% == -1
   if %actor.sex% == female
      say Well skedaddle off and fetch some then, won't you lass?
   else
      say Well skedaddle off and fetch some then, won't you laddie?
   end
end
~
#61537
Wise leprechaun discusses spiders~
0 d 100
spiders spiders?~
wait 4
say They've been terrorizing us for a long time now.
say But I know how to defeat their webs.
wait 3 s
ponder
wait 2 s
say I suppose I could help you... if you give me a cherry!
say And if you do, I'll enchant a knife if you want.
say Of course, only one of these flint knives will do the trick.
~
#61538
Cannot drag hanging cherry~
1 c 100
drag~
switch %cmd%
  case d
    return 0
    halt
done
if %arg% /= purple || %arg% /= cherry
   return 1
   osend %actor% You can't reach that high!
else
   return 0
end
~
#61539
**UNUSED**~
1 c 100
down~
return 0
~
#61540
**UNUSED**~
2 c 100
down~
return 0
~
#61541
Intercept doorbash command~
2 c 100
doorbash~
switch %cmd%
  case d
  case do
    return 0
    halt
done
if %web_present% == 1
   return 1
   wechoaround %actor% %actor.name% charges at the web, but just bounces off.
   wsend %actor% You charge into the springy web and are tossed back.
else
   return 0
end
~
#61542
Wise leprechaun responds to spoken greetings~
0 d 100
hi hello~
if %actor.vnum% == -1
   return 1
   wait 1s
   say Why hello to you too!
   wait 1 s
   if %actor.sex% == female
      mecho %self.name% says, 'Well now, lassie, I don't suppose you have any fruit on ye?'
   elseif %actor.sex% == male
      mecho %self.name% says, 'Well now, laddie, I don't suppose you have any fruit on ye?'
   else
      mecho %self.name% says, 'Well now, I dont suppose you have any fruit on ye?'
   end
   wait 2 s
   mecho %self.name% says, 'If you bring me some, I can help you with those awful spiders, oh yes...  Mind you, I'm particular to cherries.'
else
   return 0
end
~
#61543
The tickler~
0 bg 40
~
wait 1 s
set val %random.3%
switch %val%
case 1
set ch %random.char%
tickle %ch.name%
break
default
done
~
#61544
grig_play_random~
0 b 8
~
set play %random.3%
switch %play%
  case 1
    emote fiddles a jaunty tune!
    break
  case 2
    emote dances a merry jig!
    break
  case 3
    emote plays a soulful, wistful number of ages long past.
    break
  default
    emote tunes his fiddle.
done
~
#61545
Violin music~
1 c 3
throw~
switch %cmd%
  case t
  case th
  case thr
  case thro
    return 0
    halt
done
oecho I'm throwing!
~
#61550
creeping_doom_pixie_random~
0 b 12
~
switch %random.4%
  case 1
    emote grumbles about mortals shattering the natural order.
    break
  case 2
    say I swear I will find a vessel for nature's vengeance!
    break
  case 3
    say Someday we'll take revenge on the mortal races...
    break
  case 4
  default
    emote makes a mystic gesture and surrounds herself with a swirling swarm of insects!
    break
done
~
#61551
creeping_doom_pixie_greet~
0 h 100
~
wait 2
if %actor.class% /= Druid
  if %actor.level% > 80
    if %actor.quest_stage[creeping_doom]% == 0
      emote grumbles about pathetic mortals ruining the world.
      say Oh what's this?
      peer %actor%
      wait 1s
      say A servant of Nature!
      wait 2s
      say And a powerful one at that!
      wait 1s
      mecho %self.name% says, 'You!
      mecho &0You can help us remind the mortals how to treat the majesty of Nature!'
    elseif %actor.quest_stage[creeping_doom]% == 5
      msend %actor% %self.name% rushes to congratulate you!
      wait 1s
      mecho %self.name% says, 'The forest sings of your deed!  Raining doom down on
      mecho &0that place of genocide has restored balance to part of the Black Woods.'
      wait 2s
      mecho %self.name% says, 'You earned the right to be called Nature's Avenger
      mecho &0and the power that brings.'
      wait 2s
      mskillset %actor% creeping doom
      msend %actor% &b&9The nightmare of the Deep Dreaming touches your soul!
      msend %actor% You have mastered&0 &1Creeping &2Doom&b&9!&0
      quest complete creeping_doom %actor.name%
    elseif %actor.quest_stage[creeping_doom]% == 4
      mecho %self.name% says, 'Is everything alright?
      mecho &0Do you need another seed?'
    elseif %actor.has_completed[creeping_doom]%
      say Welcome back Avenger.
    else
      say Welcome back Dreamer.
    endif
  else
    say Oh what's this?
    peer %actor%
    wait 1s
    say A servant of Nature!
    wait 1s
    mecho %self.name% says, 'You certainly have promise.  But you're too
    mecho &0inexperienced to act as Nature's Avenger.  Come back when you're ready for the
    mecho &0darkest of dreams.'
  endif
endif
~
#61552
creeping_doom_pixie_speech~
0 d 100
yes~
wait 2
if %actor.class% /= Druid && %actor.quest_stage[creeping_doom]% == 0 && %actor.level% > 80 
  grin
  quest start creeping_doom %actor.name%
  wait 1s
  mecho %self.name% says, 'This spell sends a carpet of living death made out of
  mecho &0Nature's Rage and Doom at your enemies and makes it EAT THEIR FACES OFF.  It's
  mecho &0unbridled and horrible, but needs very careful preparation.'
  wait 3s
  mecho %self.name% says, 'First, we choose violence and find symbols of
  mecho &0Nature's Rage.'
  wait 1s
  mecho %self.name% says, 'Bring back a &2cutting of the deadly vines&0 on Mist
  mecho &0Mountain.'
  wait 3s
  mecho %self.name% says, 'Also, bring a &1ruby scarab&0 and a &2&bceramic green spider&0'
  mecho &0because bug foci are sweet.'
  mecho   
  say Both're in different monuments to the dead.
  wait 2s
  mecho %self.name% says, 'If you need, you can check your &7&b[progress]&0 with me.'
elseif %actor.quest_stage[creeping_doom]% == 4
  if %actor.inventory[61518]%
    msend %actor% &2&bYou already have %get.obj_shortdesc[61518]%!&0
  else
    mload obj 61518
    give essence-natures-vengeance %actor.name%
    say Be careful with it this time!
  endif
endif
~
#61553
creeping_doom_pixie_receive~
0 j 100
~
set stage %actor.quest_stage[creeping_doom]%
if %stage% == 1
  if %object.vnum% == 11812 || %object.vnum% == 16213 || %object.vnum% == 48029
    if %actor.quest_variable[creeping_doom:%object.vnum%]% == 1
      return 0
      say You already brought me %get.obj_shortdesc[%object.vnum%]%
      msend %actor% %self.name% refuses %object.shortdesc%.
    else
      quest variable creeping_doom %actor.name% %object.vnum% 1
      wait 2
      mjunk %object%
      say Just what we need.
      if %actor.quest_variable[creeping_doom:11812]% && %actor.quest_variable[creeping_doom:16213]% && %actor.quest_variable[creeping_doom:48029]%
        quest advance creeping_doom %actor.name%
        wait 1s
        say Yesssss... Raaaaaaage...
        wait 2s
        say Now we need all the doom!  Swarms of DOOM!!
        wait 2s
        mecho %self.name% says, 'Ethilien is chock-full of creepy-crawlies:
        mecho &0Flies, bugs, spiders, scorpions, giant ant creatures...
        wait 2s
        say The list goes on.
        wait 2s
        mecho %self.name% says, 'Creeping Doom is just swarms of these things.  You'll
        mecho &0need to gather &2&b11 essence of swarms&0 from these creatures, one for every circle
        mecho &0of magic the spell requires.'
        wait 4s
        mecho %self.name% says, 'Harvesting these essences'll take time and patience.
        mecho &0The more powerful the insect you kill, the more likely you'll find acceptable
        mecho &0essences.  Bring those essences to me.'
      else
        wait 2
        say Where's the rest?
      endif
    endif
  else
    return 0
    wait 2
    say This isn't going to help.
    msend %actor% %self.name% refuses %object.shortdesc%.
  endif
elseif %stage% == 2
  if %object.vnum% == 61517
    wait 2
    say Yesssss...  More essences!
    eval spiders %actor.quest_variable[Creeping_doom:spiders]% + 1
    quest variable creeping_doom %actor.name% spiders %spiders%
    mjunk essence-of-swarms
    wait 2
    if %actor.quest_variable[Creeping_doom:spiders]% == 11
      quest advance creeping_doom %actor.name%
      mecho %self.name% says, 'The Rage and Doom are hyped, but we need to keep 'em
      mecho &0from going too aggro.  Trophies from ending Nature's suffering'll temper them
      mecho &0into a focused, deadly weapon.  The scythe to cut down Nature's enemies...'
      wait 3s
      mecho %self.name% says, 'There are three particular trees that suffer in
      mecho &0unnatural agony.  Help them and bring a trophy of the experience.'
      wait 3s
      mecho %self.name% says, 'One is a treant plagued by invasive dogs in the
      mecho &0forest of the oldest Rhell.  Ease the tree's suffering and bring back a limb
      mecho &0as a relic of peace.'
      wait 4s
      mecho %self.name% says, 'Another is an eternally burning tree in the ruined
      mecho &0city Templace.  End the tree's suffering and bring back one of its burning
      mecho &0branches as a relic of succor.'
      wait 3s
      mecho %self.name% says, 'The final is an elder tremaen in an ancient forest
      mecho &0beset by the elemental Plane of Fire.  The tremaen carries the holy Seed of
      mecho &0the Silveroak.  Return with the Seed as a relic of the promise of new life
      mecho &0even in the face of extreme burnination.'
    else
      eval remaining 11 - %actor.quest_variable[Creeping_doom:spiders]%
      mecho %self.name% says, 'You have brought me %actor.quest_variable[Creeping_doom:spiders]% &2&bessence of swarms&0.
      mecho %remaining% more to go.'
    endif
  else
    return 0
    say This isn't a swarm!!
    msend %actor% %self.name% refuses %object.shortdesc%.
  endif
elseif %stage% == 3
  if %object.vnum% == 48416 || %object.vnum% == 52034 || %object.vnum% == 62503
    if %actor.quest_variable[creeping_doom:%object.vnum%]% == 1
      return 0
      say You already found this.
      msend %actor% %self.name% refuses %object.shortdesc%.
    else
      quest variable creeping_doom %actor.name% %object.vnum% 1
      wait 1s
      say This is it!
      mjunk %object%
      if %actor.quest_variable[creeping_doom:48416]% && %actor.quest_variable[creeping_doom:52034]% && %actor.quest_variable[creeping_doom:62503]%
        say These are all the parts we need!
        quest advance creeping_doom %actor.name%
        wait 1s
        mecho %self.name% ties the bit of vine to the treant limb.
        wait 2s
        mecho Using the burning branch, she lights the vine on fire and crushes the scarab and spider.
        wait 3s
        mecho %self.name% sprinkles the Seed of the Silveroak with the ash mixture.
        wait 1s
        mecho One by one %self.name% blows the essences of swarms onto the Seed of the Silveroak.
        wait 2s
        mecho The Seed of the Silveroak glows &2green&0 and &1red&0!
        wait 1s
        mload obj 61518
        give essence-natures-vengeance %actor.name%
        wait 2s
        say 
        mecho %self.name% says, 'Now to rain doom on the doomed heads of our doomed
        mecho &0enemies!'
        wait 2s
        say In the Black Woods is a logging camp clearcutting the forest.
        wait 2s
        mecho %self.name% says, 'Take this new seed and drop it at the
        mecho &0&7&bentrance to the logging camp&0.'
        wait 3s
        mecho %self.name% says, 'Death will wash over the camp and annihilate
        mecho &0everything there.  Stay and watch.  Hear every last scream.  Feel
        mecho &0every last death.  Then you'll understand Creeping Doom.'
        wait 4s
        say That is the strength of Nature's vengeance.
        wait 4s
        say Once you've done the deed, come back.
      else
        wait 1s
        say Help the other trees and bring the trophies to me.
      endif
    endif
  else
    return 0
    say The heck is this??
    msend %actor% %self.name% refuses %object.shortdesc%.
  endif
else
  return 0
  say The heck is this??
  msend %actor% %self.name% refuses %object.shortdesc%.
endif
~
#61554
creeping_doom_bugs_death~
0 f 100
~
set person %actor%
set i %actor.group_size%
if %i%
   set a 1
   unset person
   while %i% >= %a%
      set person %actor.group_member[%a%]%
      if %person.room% == %self.room%
         if %person.quest_stage[creeping_doom]% == 2 || (%person.level% > 80 && (%person.vnum% >= 1000 && %person.vnum% <= 1038))
            set rnd %random.50%
            if %rnd% <= %self.level%
               mload obj 61517
            endif
         endif
      elseif %person%
         eval i %i% + 1
      endif
      eval a %a% + 1
   done
elseif %person.quest_stage[creeping_doom]% == 2 || (%person.level% > 80 && (%person.vnum% >= 1000 && %person.vnum% <= 1038))
   set rnd %random.50%
   if %rnd% <= %self.level%
     mload obj 61517
   endif
endif
~
#61555
creeping_doom_logging_camp_drop~
2 h 100
~
if %object.vnum% == 61518
  wait 1
  wpurge %object%
  if %actor.quest_stage[creeping_doom]% == 4
    quest advance creeping_doom %actor.name%
  endif
  wecho &9&b%The get.obj_shortdesc[61518]% bursts open in a flood of insects!&0
  wait 2
  wzoneecho 87 &b&9An endless wave of crawling &0&1arachnoids&b&9 and &0&2insects&b&9 pours through the camp consuming everything in sight!&0
  wait 1s
  wzoneecho 87 &b&9Screams pierce the quiet of the camp as everything is swallowed by a blanket of death!&0
  set number 8703
  while %number% < 8760
    set area %get.room[%number%]%
    set person %area.people%
    while %person%
      if %person.vnum% != -1 && %person.vnum% != 8708 && %person.vnum% != 8710 && %person.vnum% != 8711 && %person.vnum% != 8714
        wat %area% wdamage %person% 5000
        wat %area% wecho &b&9An endless wave of crawling &0&1arachnoids&b&9 and &0&2insects&b&9 consumes %person.name%!&0
        wzoneecho 87 Someone screams as he is consumed by an endless wave of crawling &1arachnoids&0 and &2insects&0!
      endif
      set person %person.next_in_room%
    done
    wait 2
    eval number %number% + 1
  done
  wzoneecho 87 &b&9Eerie silence falls over the camp as the deluge of death subsides...&b&9
endif
~
#61556
creeping_doom_status_checker~
0 d 100
progress progress? status status?~
set stage %actor.quest_stage[creeping_doom]%
wait 2
switch %stage%
  case 1
    set item1 11812
    set place1 an assassin vine on Mist Mountain
    set item2 16213
    set place2 the great pyramid
    set item3 48029
    set place3 Rhalean's evil sister in the northern barrow
    set step gathering Nature's Rage
    break
  case 2
    set essence %actor.quest_variable[creeping_doom:spiders]%
    eval total 11 - %actor.quest_variable[creeping_doom:spiders]%
    mecho %self.name% says, 'You are collecting essences of swarms from:
    mecho &0Flies, insects, spiders, bugs, scorpions, giant ant people, etc.'
    mecho    
    mecho You have found %essence% essences.
    mecho You need to find %total% more.
    mecho   
    mecho Remember, the tougher the bug, the better your chances of finding essences.
    halt
    break
  case 3
    set step locate three sources of Nature's Vengeance.
    set item1 48416
    set place1 the elder tremaen in the elemental Plane of Fire
    set item2 52034
    set place2 the burning tree in Templace
    set item3 62503
    set place3 the Treant in the eldest Rhell's forest
    break
  case 4
    mecho %self.name% says, 'Take the Essence of Nature's Vengeance and
    mecho &0drop it at the entrance to the logging camp.'
    halt
    break
  default
    if %actor.has_completed[creeping_doom]%
      say I already taught you Creeping Doom!
    else
      say You haven't started to dream my Dream yet.
    endif
    halt
done
mecho You are trying to %step%.
mecho 
if %actor.quest_variable[creeping_doom:%item1%]% || %actor.quest_variable[creeping_doom:%item2%]% || %actor.quest_variable[creeping_doom:%item3%]%
  mecho You have brought me:
  if %actor.quest_variable[creeping_doom:%item1%]%
    mecho - %get.obj_shortdesc[%item1%]%
  endif
  if %actor.quest_variable[creeping_doom:%item2%]%
    mecho - %get.obj_shortdesc[%item2%]%
  endif
  if %actor.quest_variable[creeping_doom:%item3%]%
    mecho - %get.obj_shortdesc[%item3%]%
  endif
endif
mecho 
mecho I still need:
if !%actor.quest_variable[creeping_doom:%item1%]%
  mecho - %get.obj_shortdesc[%item1%]% from %place1%
endif
if !%actor.quest_variable[creeping_doom:%item2%]%
  mecho - %get.obj_shortdesc[%item2%]% from %place2%
endif
if !%actor.quest_variable[creeping_doom:%item3%]%
  mecho - %get.obj_shortdesc[%item3%]% from %place3%
endif
~
#61557
creeping_doom_pixie_speech2~
0 d 100
vengeance revenge avenger vessel? Abuses? vengeance? revenge? avenger? vessel abuses~
wait 2
mecho %self.name% says, 'Yes, we need a powerful priest of Nature to be our
mecho &0avenging avatar!'
wait 2
if %actor.class% /= Druid
  if %actor.level% > 80 
    say Will you be our vessel?
  else
    say Come back after your power has grown some more.
  endif
endif
~
#61558
creeping_doom_pixie_speech3~
0 d 100
nature majesty treat remind nature? majesty? treat? remind?~
if %actor.class% /= Druid
  if %actor.level% > 80
    if %actor.quest_stage[creeping_doom]% == 0
      wait 2
      mecho %self.name% says, 'They have lost their respect for Nature and us,
      mecho &0its fey emissaries.  They slaughter our families like pigs.  They burn the
      mecho &0forests and overhunt the fields.'
      wait 4s
      mecho %self.name% says, 'They no longer fear us.
      mecho &0We who come for their children in the night.
      mecho &0We who bring nightmares to generations.
      mecho &0We who dream a Dream they should pray we never awaken from.'
      wait 2s
      say They have forgotten.
      wait 4s
      say We will make them remember.
    endif
  endif
endif
~
#61559
creeping_doom_pixie_speech4~
0 d 100
remember remember? how how? forgotten forgotten?~
if %actor.class% /= Druid
  if %actor.level% > 80
    if %actor.quest_stage[creeping_doom]% == 0
      say I can teach you the power from Nature's darkest of
      mecho 'dreams, Creeping Doom.'
      wait 3s
      say Use it to annihilate those who would defile the
      mecho 'natural order.'
      wait 3s
      say Are you ready to be our avenger?
    endif
  endif
endif
~
#61571
Niaxxa death~
0 f 100
~
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
      if %person.quest_stage[enchanted_hollow_quest]% && !%person.has_completed[enchanted_hollow_quest]%
         quest complete enchanted_hollow_quest %person.name%
      endif
   elseif %person%
      eval i %i% + 1
   endif
   eval a %a% + 1
done
~
#61595
Lighting a string of firecrackers~
1 c 100
light~
if %arg% /= firework || %arg% /= firecracker || %arg% /= string || %arg% /= firecrackers
   if %burning% == 1
      return 1
      osend %actor% It's already lit!
   elseif %on_ground% == 0
      return 1
      osend %actor% To avoid being horribly disfigured by fire, dropping it first might be a good idea.
   else
      return 1
      set burning 1
      global burning
      oechoaround %actor% %actor.name% lights %self.shortdesc%.
      osend %actor% You light %self.shortdesc%.
      wait 2 s
      oecho The fuse on %self.shortdesc% burns slowly, giving off some smoke.
      wait 1 s
      set counter 20
      while %counter%
         oecho %self.shortdesc% explodes!
         switch %random.20%
 
         case 1
            oecho &bBANG!&0  &bBANG!&0  &5&bBANG!&0  &bBANG!&0  &3&bBANG!&0  &1&bBANG!&0  &5BANG!&0  &3&bBANG!&0  &4&bBANG!&0  &4&bBANG!&0  
            break
         case 2
            oecho &4&bBANG!&0  &bBANG!&0  &bBANG!&0  &1BANG!&0  &1BANG!&0  &bBANG!&0  &1&bBANG!&0  &4&bBANG!&0  
            break
         case 3
            oecho &3&bBANG!&0  &4BANG!&0  &bBANG!&0  &5&bBANG!&0  &4BANG!&0  &4&bBANG!&0  
            break
         case 4
            oecho &4&bBANG!&0  &4&bBANG!&0  &bBANG!&0  &1&bBANG!&0  &3&bBANG!&0  &bBANG!&0  &bBANG!&0  &bBANG!&0  &1BANG!&0  &1&bBANG!&0  
            break
         case 5
            oecho &5&bBANG!&0  &4&bBANG!&0  &4&bBANG!&0  &4&bBANG!&0  &bBANG!&0  &3&bBANG!&0  &4BANG!&0  &5BANG!&0  &4BANG!&0  &1BANG!&0  
            break
         case 6
            oecho &bBANG!&0  &4&bBANG!&0  &bBANG!&0  &bBANG!&0  &bBANG!&0  &1&bBANG!&0  
            break
         case 7
            oecho &bBANG!&0  &3BANG!&0  &4&bBANG!&0  &1&bBANG!&0  &3BANG!&0  &3&bBANG!&0  &bBANG!&0  &1&bBANG!&0  
            break
         case 8
            oecho &1&bBANG!&0  &4BANG!&0  &4&bBANG!&0  &1&bBANG!&0  &bBANG!&0  &5&bBANG!&0  &1BANG!&0  &bBANG!&0  
            break
         case 9
            oecho &5&bBANG!&0  &1&bBANG!&0  &1&bBANG!&0  &4&bBANG!&0  
            break
         case 10
            oecho &1&bBANG!&0  &4BANG!&0  &4&bBANG!&0  &5BANG!&0  &bBANG!&0  &4&bBANG!&0  
            break
         case 11
            oecho &4&bBANG!&0  &5&bBANG!&0  &4BANG!&0  &bBANG!&0  &3&bBANG!&0  &bBANG!&0  &4&bBANG!&0  &5&bBANG!&0  
            break
         case 12
            oecho &4&bBANG!&0  &bBANG!&0  &1&bBANG!&0  &bBANG!&0  
            break
         case 13
            oecho &4&bBANG!&0  &bBANG!&0  &3&bBANG!&0  &bBANG!&0  &1&bBANG!&0  &1&bBANG!&0  &bBANG!&0  &bBANG!&0  &bBANG!&0  &4BANG!&0  
            break
         case 14
            oecho &bBANG!&0  &bBANG!&0  &1BANG!&0  &4BANG!&0  
            break
         case 15
            oecho &3&bBANG!&0  &4&bBANG!&0  
            break
         case 16
            oecho &4&bBANG!&0  &5BANG!&0  &4BANG!&0  &bBANG!&0  &1&bBANG!&0  &bBANG!&0  
            break
         case 17
            oecho &3BANG!&0  &1BANG!&0  &1BANG!&0  &4&bBANG!&0  &4&bBANG!&0  &bBANG!&0  
            break
         case 18
            oecho &bBANG!&0  &bBANG!&0  &5BANG!&0  &4BANG!&0  &1BANG!&0  &1&bBANG!&0  &bBANG!&0  &1&bBANG!&0  
            break
         case 19
            oecho &1&bBANG!&0  &3&bBANG!&0  &bBANG!&0  &5BANG!&0  &4BANG!&0  &3BANG!&0  &5BANG!&0  &4BANG!&0  
            break
         default
            oecho &4&bBANG!&0  &1&bBANG!&0  
            break
         done
         wait 3
         eval counter %counter% - 1
      done
      wait 1 s
      oecho %self.shortdesc% shoots out a few sputtering sparks.
      opurge %self.name%
   end
else
   return 0
end
~
#61596
Firework being dropped~
1 h 100
~
return 1
set on_ground 1
global on_ground
~
#61597
Firework being taken~
1 g 100
~
if %burning% == 1
   return 0
   odamage %actor% 2 fire
   if %damdone% != 0
      oechoaround %actor% %actor.name% tries to take %self.shortdesc%, but burns %actor.p% fingers! (&1%damdone%&0)
      osend %actor% Ouch!! You burnt your fingers on %self.shortdesc%! (&1%damdone%&0)
   else
      osend %actor% An angry smoke pixie comes out of nowhere and kicks %self.shortdesc% away from your grubby, grabby fingers.
      oechoaround %actor% %actor.name% reaches for %self.shortdesc%, but a smoke pixie swoops down and kicks it away!
   end
else
   return 1
   set on_ground 0
   global on_ground
end
~
#61598
roman candle normalizer~
1 c 100
ligh~
return 0
~
#61599
Lighting a roman candle~
1 c 7
light~
switch %cmd%
  case l
    return 0
    halt
done
if %arg.vnum% /= %self.vnum%
  if %burning% == 1
    return 1
    osend %actor% It's already lit!
  elseif %on_ground% == 0
    return 1
    osend %actor% To avoid being horribly disfigured by fire, dropping it first might be a good idea.
  else
    return 1
    set burning 1
    global burning
    oechoaround %actor% %actor.name% lights %self.shortdesc%.
    osend %actor% You light %self.shortdesc%.
    wait 2 s
    oecho A few small sparks begin flying up out of %self.shortdesc%.
    wait 1 s
    set counter 10
    while %counter%
      switch %random.9%
        case 1
          oecho %self.shortdesc% sends up a shower of &3brilliant &bgolden&7 sparks&0!
          break
        case 2
          oecho A blazing stream of &1&bcrimson&0 and &2&bvermillion&7 sparks&0 shoots out of %self.shortdesc%!
          break
        case 3
          oecho A brilliant river of &bshining &4&bblue&0 and &3&byellow&0 &bmotes&0 streams out of %self.shortdesc%!
          break
        case 4
          oecho Multitudes of brightly &1bu&3rn&1in&3g &9&bs&0&bp&0&9&ba&0&br&0&9&bk&0&bs&0 shoot up into the sky from %self.shortdesc%!
          break
        case 5
          oecho %self.shortdesc% lets loose with an impressive surge of &7&bwhite&0 and &4&bblue&0 &bstars&0!
          break
        default
          oecho A stream of &b&1m&3u&4l&5t&2i&6c&3o&1&4&2l&5o&3r&4e&1d&0 &bsparks&0 is surging up out of %self.shortdesc%!
          break
      done
      wait 1 s
      eval counter %counter% - 1
    done
    wait 1 s
    oecho %self.shortdesc% shoots out a few sputtering sparks.
    wait 3
    oecho %self.shortdesc% quietly burns out.
    opurge %self.name%
  end
else
  return 0
endif
~
$~
