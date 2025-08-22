#12000
Hooded druid responds to 'hello'~
0 d 100
hello hi Hello Hi~
wait 1s
if %actor.has_completed[twisted_sorrow]%
   bow %actor.name%
   wait 1s
   say Hello my friend.  The trees are still thankful for your service.
else
   say Hello, little one.
   wait 2s
   mecho The hooded druid says, 'I take it you have witnessed the the devastation beyond
   mecho &0this grove?  A great evil befell the land, while we were sleeping.  Now that it
   mecho &0has consumed even the one who wrought it, what hope is there of lifting it?
   mecho &0Surely, I have no idea.'
   wait 5s
   sigh
   wait 2s
   mecho The hooded druid continues, 'But still, these trees are not so easily
   mecho &0perverted.  Though the corruption of the forest has harmed and saddened them,
   mecho &0they live on.  And indeed, they thirst, each in its own way.  But it has been
   mecho &0so long since they were slaked, that none can remember what satisfies them.'
   wait 5s
   mecho The hooded druid says, 'Tell me friend, do you think you may want to help
   mecho &0them?'
end
~
#12001
Druid responds to 'yes'~
0 d 100
yes Yes~
wait 1s
if %actor.has_completed[twisted_sorrow]%
   smile %actor.name%
   wait 1s
   say The trees are satisfied, my friend.
else
   msend %actor% %self.name% looks over you carefully.
   mechoaround %actor% %self.name% looks over %actor.name% carefully.
   wait 2s
   mecho The hooded druid says, 'You seem the bright sort, and I sense some compassion
   mecho &0in you.  If you can muster the kindness to minister to the sorrow that these
   mecho &0trees bear, who knows what good shall follow?  Surely, the curse upon this
   mecho &0forest cannot so easily be lifted, but still, kind deeds never go wasted in
   mecho &0this world.'
   wait 5s
   emote thinks deeply for a moment.
   wait 2s
   mecho The hooded druid asks, 'So: would you care to ease the loneliness of the
   mecho &0Rhells?  If you wish to do so, say &7&b"I will assist"&0.'
end
~
#12002
Druid responds to 'assist'~
0 d 100
assist~
wait 1s
if %actor.has_completed[twisted_sorrow]%
   smile %actor.name%
   wait 1s
   say The trees are satisfied, my friend.
else
   if %actor.quest_stage[twisted_sorrow]% == 0
      quest start twisted_sorrow %actor.name%
   endif
   mecho The hooded druid says, 'Very well.  Each tree sups of its own liquid, that much
   mecho &0I know.  But what specifically they desire, I'm afraid, is knowledge from
   mecho &0before I began my watch.'
   wait 5 s
   emote rubs his forehead thoughtfully.
   wait 2 s
   mecho The hooded druid says, 'A prayer of greeting must be spoken to each tree
   mecho &0before it will awaken, so you will need my help.  Here is what you must do.'
   wait 5 s
   emote looks sharply to ensure that you are paying attention.
   wait 2 s
   mecho The hooded druid says, 'When you have a liquid to offer, say in my presence,
   mecho &0&7&b"follow me"&0.  Then I will accompany you to whichever tree you have chosen.
   mecho &0Then go to the tree, and give me the vessel.  I will commune with the tree and
   mecho &0provide it with the offering.'
   wait 5 s
   emote stops to cough a few times, and leans up against a tree for support.
   wait 2 s
   mecho The hooded druid says, 'Now go, and return when you have what is needed.'
end
~
#12003
Druid receive~
0 j 100
~
if %actor.quest_stage[twisted_sorrow]% > 1
   return 0
   mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
   msend %actor% You give %object.shortdesc% to %self.name%.
   wait 8
   say No further offerings are necessary.
   msend %actor% %self.name% returns %object.shortdesc% to you.
   mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
elseif %actor.quest_stage[twisted_sorrow]% == 1
   if %object.type% == LIQCONTAINER
      if %self.room% == 12015
         return 0
         mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
         msend %actor% You give %object.shortdesc% to %self.name%.
         wait 4
         say Let us move to one of the mighty Rhells first, friend.
         wait 4
         msend %actor% %self.name% returns %object.shortdesc% to you.
         mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
      else
         if %object.val1% == 0
            return 0
            mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
            msend %actor% You give %object.shortdesc% to %self.name%.
            wait 4
            emote peers into %object.shortdesc%.
            wait 4
            say This won't do at all!  It's empty.
            wait 4
            msend %actor% %self.name% returns %object.shortdesc% to you.
            mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
         elseif %actor.quest_variable[twisted_sorrow:satisfied_tree:%self.room%]% == 1
            return 0
            mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
            msend %actor% You give %object.shortdesc% to %self.name%.
            wait 4
            say This tree is already satisfied, my friend.
            wait 4
            msend %actor% %self.name% returns %object.shortdesc% to you.
            mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
         else
            return 1
            wait 1 s
            emote peers into %object.shortdesc%.
            wait 2 s
            nod
            wait 2 s
            kneel
            emote places his hands on the mighty Rhell's roots.
            wait 5 s
            emote continues to kneel, making no sound.
            wait 7 s
            stand
            set success 0
            * The tree of Luck: tea
            if %self.room% == 12016 && %object.val2% == 11
               set success 1
            * The tree of Reverence: moderate alcohol
            elseif %self.room% == 12017 && %object.val2% > 0 && %object.val2% < 5
               set success 1
            * The tree of self-reliance: water
            elseif %self.room% == 12018 && %object.val2% == 0
               set success 1
            * The tree of nimbleness: coffee
            elseif %self.room% == 12014 && %object.val2% == 12
               set success 1
            * The tree of kindness: milk
            elseif %self.room% == 12046 && %object.val2% == 10
               set success 1
            end
            mecho %self.name% carefully pours out %object.shortdesc% onto the Rhell's roots.
            wait 1
            mjunk %object.name%
            wait 8
            if %success% == 1
               eval num_trees 1 + %actor.quest_variable[twisted_sorrow:num_trees]%
               quest variable twisted_sorrow %actor.name% num_trees %num_trees%
               quest variable twisted_sorrow %actor.name% satisfied_tree:%self.room% 1
               mecho A deep throbbing hum is emanating from the ground.
               wait 3 s
               mecho The hum gets louder and louder, causing twigs and leaves to dance upon the ground!
               mecho It is overwhelming, yet soothing.
               wait 5 s
               mecho The hum fades slowly away, and all is quiet again.
               wait 3 s
               smile
               wait 2 s
               if %num_trees% < 4
                  say You have done this tree a great service.
               elseif %num_trees% == 4
                  say I sense that yet another tree waits in loneliness.
                  wait 1s
                  say There can be no peace until it, too, is satisfied.
               elseif %num_trees% == 5
                  say Excellent, my friend!  The trees are satisfied.
                  wait 8
                  say Please take this gift on their behalf.
                  wait 8
                  mload obj 12018
                  give sleeves-sorrow %actor.name%
                  follow me
                  wait 4
                  emote walks away quietly.
                  quest complete twisted_sorrow %actor.name%
                  *
                  * Set X to the level of the award - code does not run without it
                  * 
                  if %actor.level% < 10
                      set expcap %actor.level%
                  else
                      set expcap 10
                  endif
                  set expmod 0
                  if %expcap% < 9
                     eval expmod (((%expcap% * %expcap%) + %expcap%) / 2) * 55
                  elseif %expcap% < 17
                     eval expmod 440 + ((%expcap% - 8) * 125)
                  elseif %expcap% < 25
                     eval expmod 1440 + ((%expcap% - 16) * 175)
                  elseif %expcap% < 34
                     eval expmod 2840 + ((%expcap% - 24) * 225)
                  elseif %expcap% < 49
                     eval expmod 4640 + ((%expcap% - 32) * 250)
                  elseif %expcap% < 90
                     eval expmod 8640 + ((%expcap% - 48) * 300)
                  else
                     eval expmod 20940 + ((%expcap% - 89) * 600)
                  endif
                  *
                  * Adjust exp award by class so all classes receive the same proportionate amount
                  *
                  switch %actor.class%
                      case Warrior
                      case Berserker
                      *
                      * 110% of standard
                      * 
                          eval expmod (%expmod% + (%expmod% / 10))
                          break
                      case Paladin
                      case Anti-Paladin
                      case Ranger
                      *
                      * 115% of standard
                      *
                          eval expmod (%expmod% + ((%expmod% * 2) / 15)
                          break
                      case Sorcerer
                      case Pyromancer
                      case Cryomancer
                      case Illusionist
                      case Bard
                      *
                      * 120% of standard
                      *
                          eval expmod (%expmod% + (%expmod% / 5))
                          break
                      case Necromancer
                      case Monk
                      *
                      * 130% of standard
                      *
                          eval expmod (%expmod% + (%expmod% * 2) / 5)
                          break
                      default
                          set expmod %expmod%
                  done
                  msend %actor% &3&bYou gain experience!&0
                  eval setexp (%expmod% * 10)
                  set loop 0
                  while %loop% < 10
                      *
                      * Xexp must be replaced by mexp, oexp, or wexp for this code to work
                      * Pick depending on what is running the trigger
                      *
                      mexp %actor% %expmod%
                      eval loop %loop% + 1
                  done
                  mteleport %self% 12015
               end
            else
               sigh
               wait 8
               say The tree is not responding.
               wait 1s
               say I fear it has little liking for that drink.
            end
            * Here is where the object could be returned, if a way to
            * empty it could be devised.
         end
      end
   else
      return 0
      mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
      msend %actor% You give %object.shortdesc% to %self.name%.
      wait 4
      say This is not a liquid container.
      wait 4
      msend %actor% %self.name% returns %object.shortdesc% to you.
      mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
   end
else
   return 0
   mechoaround %actor% %actor.name% gives %object.shortdesc% to %self.name%.
   msend %actor% You give %object.shortdesc% to %self.name%.
   wait 8
   say Why do you give me this?
   msend %actor% %self.name% returns %object.shortdesc% to you.
   mechoaround %actor% %self.name% returns %object.shortdesc% to %actor.name%.
end
~
#12004
Druid responds to 'follow'~
0 d 100
follow~
if %actor.quest_stage[twisted_sorrow]% == 1
   wait 4
   emote nods solemnly.
   wait 8
   follow %actor.name%
end
~
#12005
Druid is prevented from going up~
2 g 100
~
if %actor.vnum% == 12018
   return 0
end
~
#12006
Haggard brownie greets~
0 g 100
~
if %actor.vnum% == -1 && %self.room% == 12103
   wait 3
   emote looks at you with desperation.
   wait 8
   set room %get.room[%self.room%]%
   if (%room.people[12020]%)
      say Please, help me escape from these fiends!
      wait 3 s
      mforce dark-pixie-tormentor slap haggard-brownie
      mforce dark-pixie-tormentor emote hisses, 'It will be ssssilent!'
      wait 2 s
      emote cringes away, uttering a little yelp.
   else
      say I am so lost!  Will you help me get home?
      say Say 'I will escort you' if so.  But beware!
      say The forces of darkness hunt me...
      wait 4 s
      scan
   end
end
~
#12007
Dark pixies torment brownie~
2 b 60
~
if (%self.people[12019]%) && (%self.people[12020]%)
   switch %random.5%
      case 1
         wforce dark-pixie-tormentor emote hisses, 'Where is your forest magic now, tree-turd?'
         break
      case 2
         wforce dark-pixie-tormentor emote growls, 'Tell us why you intrude, clod... and we may slay you swiftly.'
         break
      case 3
         wforce dark-pixie-tormentor emote swiftly kicks the haggard brownie in the kidney.
         break
      case 4
         wforce dark-pixie-tormentor emote hisses, 'You sicken me, dirty-skin.'
         break
      default
         wecho Snap!  A dark pixie raises another welt on the haggard brownie's arm.
   done
   wait 8
   switch %random.4%
      case 1
         wforce haggard-brownie emote whimpers piteously.
         break
      case 2
         wforce haggard-brownie emote glares resentfully at a dark pixie.
         break
      case 3
         wforce haggard-brownie emote bites his lip, though a tear seems to be forming in his eye.
         break
      default
         wforce haggard-brownie emote covers his head with his arms, moaning, 'No!'
   done
end
~
#12008
Check whether brownie freed from tormentors~
2 a 100
~
wait 3
if (%self.people[12019]%) && !(%self.people[12020]%)
   * This means that the pixies have been slayed, and the haggard brownie can be rescued.
   wait 8
   wforce haggard-brownie say Thank goodness!  You've killed them!
   wait 3 s
   wforce haggard-brownie say I must return home... but there will be others...
   wait 3 s
   wforce haggard-brownie say Please friend, help me!  If you can take me home, my family
   wforce haggard-brownie say will surely reward you!  And if you will do this thing, just
   wforce haggard-brownie say say 'I will escort you' and I will follow.
end
~
#12009
Dark pixie tormentor dies~
0 f 100
~
if %self.room% == 12103
   m_run_room_trig 12008
end
~
#12010
Haggard brownie hears 'escort'~
0 d 100
escort~
set rescuer %actor.name%
global rescuer
if %self.room% != 12103 && %self.room% != 12095 && %self.room% != 12030
   wait 4
   follow %actor.name%
end
~
#12011
Someone says 'escort' in torture room~
2 d 100
escort~
return 0
if (%self.people[12019]%) && !(%self.people[12020]%)
   wforce haggard-brownie nod %actor.name%
   wforce haggard-brownie say Please take me back to the light forest!
   wait 8
   wforce haggard-brownie follow %actor.name%
elseif (%self.people[12019]%) && (%self.people[12020]%)
   wforce dark-pixie-tormentor snicker
   wait 2 s
   wforce dark-pixie-tormentor say Stay out of this, grain-eater.
end
~
#12012
UNUSED~
2 a 100
~
Nothing.
~
#12013
Dark pixies ambush~
2 g 100
~
if %actor.vnum% == 12019
   wait 1
   wforce haggard-brownie follow me
   wait 3
   if %self.vnum% == 12030
      wforce haggard-brownie mload obj 12001
      wforce haggard-brownie mat 12050 drop passionfruit
   else
      wforce haggard-brownie mload obj 12002
      wforce haggard-brownie mat 12050 drop meat-pie
   end
   wecho &9&bDark shadows&0 move swiftly toward the haggard brownie.
end
~
#12014
Someone says 'escort' in an ambush room 95~
2 d 100
escort~
return 0
wait 4
if (%self.people[12019]%) && !(%self.people[12021]%) && !(%self.people[12022]%)
   wforce haggard-brownie nod %actor.name%
   wforce haggard-brownie say Please take me back to the light forest!
   wait 8
   wforce haggard-brownie follow %actor.name%
elseif (%self.people[12019]%) && ((%self.people[12021]%) || (%self.people[12022]%))
   wforce haggard-brownie emote glances fearfully at a dark pixie.
end
~
#12015
Someone says 'escort' in an ambush room 30~
2 d 100
escort~
return 0
wait 4
if (%self.people[12019]%) && !(%self.people[12021]%) && !(%self.people[12022]%)
   wforce haggard-brownie nod %actor.name%
   wforce haggard-brownie say Please take me back to the light forest!
   wait 8
   wforce haggard-brownie follow %actor.name%
elseif (%self.people[12019]%) && ((%self.people[12021]%) || (%self.people[12022]%))
   wforce haggard-brownie emote glances fearfully at a dark pixie.
end
~
#12016
Drop trigger for ambush staging room~
2 h 100
~
if %actor.vnum% == 12019
   wait 1
   set direction d
   set destination 12030
   if %object.vnum% == 12002
      set direction u
      set destination 12095
   end
   wpurge %object%
   if (%self.people[12021]%)
      wforce dark-pixie-ambusher-male %direction%
      wait 1
      wat %destination% wforce dark-pixie-ambusher-male kill haggard-brownie
   end
   if (%self.people[12022]%)
      wforce dark-pixie-ambusher-female %direction%
      wait 1
      wat %destination% wforce dark-pixie-ambusher-female kill haggard-brownie
   end
done
~
#12017
UNUSED~
2 g 100
~
Nothing.
~
#12018
Push aside planking~
2 c 100
push~
switch %cmd%
  case p
  case pu
    return 0
    halt
done
if %arg% /= planking
   set planking_open 1
   global planking_open
   wdoor 12093 south room 12039
   wdoor 12093 south description The broken planking hangs askew.
   wdoor 12093 south name planking
   wechoaround %actor% %actor.name% pushes aside some planking on the south wall.
   wsend %actor% You push aside some planking, revealing an opening to the south.
   wat 12039 wecho Some planks in the wall are swung aside.
   wait 5 s
   wecho The planking swings back down.
   wdoor 12093 south purge
   set planking_open 0
   global planking_open
else
   return 0
endif
~
#12019
**UNUSED**~
2 c 100
pu~
return 0
~
#12020
Message about planking~
2 g 100
~
if %direction% == south && %planking_open% != 1
   wait 3
   wsend %actor% As you enter, a bit of planking falls back, obscuring the opening to the south.
   wsend %actor% It looks like you could push it aside, should the need arise.
end
~
#12021
Haggard brownie reaches safety~
0 i 100
~
if %destination% == 12000
   follow me
   wait 4
   say Thank you!  Thank you at last I am safe!
   mexp %rescuer% 15000
   msend %rescuer% You gain experience!
   wait 2 s
   say Ah, here is the travel pack I lost in that wretched forest.
   emote rummages in the pack for a few moments.
   mload obj 12020
   mload obj 12020
   mload obj 12021
   mload obj 12022
   wait 4 s
   say Please take this.  It's not much, but it is all I can spare at the moment.
   wait 2 s
   give all.cookie %rescuer%
   give cup %rescuer%
   drop all.cookie
   drop cup
   wait 2 s
   say Goodbye now!
   emote trots off into the undergrowth.
   mpurge self
end
~
#12022
unused trigger~
0 d 100
Please take this.~
mecho OOO - avnum=%actor.vnum% -- give all %rescuer%
wait 8
if %actor.vnum% == 12019
   give all %rescuer%
end
~
#12099
twisted_sorrow_status_tracker~
0 d 100
progress progress?~
set luck %actor.quest_variable[twisted_sorrow:satisfied_tree:12016]%
set reverence %actor.quest_variable[twisted_sorrow:satisfied_tree:12017]%
set reliance %actor.quest_variable[twisted_sorrow:satisfied_tree:12018]%
set nimbleness %actor.quest_variable[twisted_sorrow:satisfied_tree:12014]%
set kindness %actor.quest_variable[twisted_sorrow:satisfied_tree:12046]%
set tree1 %get.room[12016]%
set tree2 %get.room[12017]%
set tree3 %get.room[12018]%
set tree4 %get.room[12014]%
set tree5 %get.room[12046]%
wait 4
if %actor.quest_stage[twisted_sorrow]% > 1
   smile %actor.name%
   wait 1 s
   say The trees are satisfied, my friend.
elseif %actor.quest_stage[twisted_sorrow]% == 1
  say Bring drink to awaken the trees from the corruption.
  if (%luck% == 1 || %reverence% == 1 || %reliance% == 1 || %nimbleness% == 1 || %kindness% == 1)
    mecho   
    mecho You have already awakened the following trees:
    if %luck% == 1
      mecho - &9&b%tree1.name%&0
    endif
    if %reverence% == 1
      mecho - &9&b%tree2.name%&0
    endif
    if %reliance% == 1
      mecho - &9&b%tree3.name%&0
    endif
    if %nimbleness% == 1
      mecho - &9&b%tree4.name%&0
    endif
    if %kindness% == 1
      mecho - &9&b%tree5.name%&0
    endif
  endif
  mecho   
  mecho Offerings are still needed for:
  if %luck% == 0
    mecho - &2%tree1.name%&0
  endif
  if %reverence% == 0
    mecho - &2%tree2.name%&0
  endif
  if %reliance% == 0
    mecho - &2%tree3.name%&0
  endif
  if %nimbleness% == 0
    mecho - &2%tree4.name%&0
  endif
  if %kindness% == 0
    mecho - &2%tree5.name%&0
  endif
  mecho &0 
  say Say &6&b"follow me"&0 when you have an offering to present.
endif
~
$~
