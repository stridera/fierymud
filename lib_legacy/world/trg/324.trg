#32401
North_Ker_road_bandit_fight~
0 k 100
~
* This trigger is the fight trigger that transforms
* a poor lil ole lil female merchant into...
* The Bandit leader! HA!
   mecho &0A &b&5feeble merchant&0 tears off her cloak, revealing
   mecho her true nature as the &b&9bandit leader&0!
   mteleport %self% 32436
   rem brooch
   give brooch bledq
   mforce leader hold brooch
   mteleport leader 32435
   mteleport bandit 32435
   mteleport bandit 32435
   mteleport bandit 32435
   mat 32435 mforce leader shout Come on out! We got us some killin to do!
   mat 32435 mecho &0&b&2The surrounding foilage errupts into movement as&0
   mat 32435 mecho &0&b&2three bandits jump out to assist their leader!&0
   mat 32435 mforce leader kill %actor.name%
   mat 32435 mforce bandit kill %actor.name%
   mat 32435 mforce 2.bandit kill %actor.name%
   mat 32435 mforce 3.bandit kill %actor.name%
~
#32402
North_Road_bribe1~
0 m 1
~
say My trigger commandlist is not complete!
~
#32403
Kerristone_north_travel_echo~
0 b 100
~
mat 32597 mecho You catch a glimpse of the passing countryside!
mat 32597 mecho  
mat 32597 mecho &0&3Dirt Road&0 cutting through &0&2Vast Grasslands&0
mat 32597 mecho &3   This simple yet well mantained road runs a relativly straight direction,
mat 32597 mecho both north and south, though many twists and bends add to its character. Its
mat 32597 mecho brown surface cuts a path through the vast grasslands on either side, their
mat 32597 mecho endless lengths appearing to stretch on forever. Though the scenary is
mat 32597 mecho serene, danger could be lurking close by in many forms, including bandits.
mat 32597 mecho An occasional patrol from Kerristone passes through the area, though for the most
mat 32597 mecho part travelers are left to fend for themselves.&0
mat 32597 mecho  
wait 4
mat 32597 mecho The world rushes by at incredible speed!
~
#32404
Kerristone_north_travel_echo2~
0 b 100
~
mat 32597 mecho You catch a glimpse of the passing countryside!
mat 32597 mecho  
mat 32597 mecho &0&3Dirt Road&0 cutting through &0&2Vast Grasslands&0
mat 32597 mecho &3   This simple yet well mantained road runs a relativly straight direction,
mat 32597 mecho both north and south, though many twists and bends add to its character. Its
mat 32597 mecho brown surface cuts a path through the vast grasslands on either side, their
mat 32597 mecho endless lengths appearing to stretch on forever. Though the scenary is
mat 32597 mecho serene, danger could be lurking close by in many forms, including bandits.
mat 32597 mecho An occasional patrol from Kerristone passes through the area, though for the most
mat 32597 mecho part travelers are left to fend for themselves.&0
mat 32597 mecho  
wait 4
mat 32597 mecho The world rushes by at incredible speed!
~
#32405
Kerristone_north_travel_echo3~
0 b 100
~
mat 32597 mecho You catch a glimpse of the passing countryside!
mat 32597 mecho  
mat 32597 mecho &0&3Dirt Road&0 cutting through &0&2Vast Grasslands&0
mat 32597 mecho &3   This simple yet well mantained road runs a relativly straight direction,
mat 32597 mecho both north and south, though many twists and bends add to its character. Its
mat 32597 mecho brown surface cuts a path through the vast grasslands on either side, their
mat 32597 mecho endless lengths appearing to stretch on forever. Though the scenary is
mat 32597 mecho serene, danger could be lurking close by in many forms, including bandits.
mat 32597 mecho An occasional patrol from Kerristone passes through the area, though for the most
mat 32597 mecho part travelers are left to fend for themselves.&0
mat 32597 mecho  
wait 4
mat 32597 mecho The world rushes by at incredible speed!
~
#32406
Kerristone_north_travel_echo4~
0 b 100
~
mat 32597 mecho You catch a glimpse of the passing countryside!
mat 32597 mecho  
mat 32597 mecho &0&3Dirt Road&0 cutting through &0&2Vast Grasslands&0
mat 32597 mecho &3   This simple yet well mantained road runs a relativly straight direction,
mat 32597 mecho both north and south, though many twists and bends add to its character. Its
mat 32597 mecho brown surface cuts a path through the vast grasslands on either side, their
mat 32597 mecho endless lengths appearing to stretch on forever. Though the scenary is
mat 32597 mecho serene, danger could be lurking close by in many forms, including bandits.
mat 32597 mecho An occasional patrol from Kerristone passes through the area, though for the most
mat 32597 mecho part travelers are left to fend for themselves.&0
mat 32597 mecho  
wait 4
mat 32597 mecho The world rushes by at incredible speed!
~
#32407
Kerriston_north_horse_look~
0 c 100
l lo loo look~
wait 1
mecho You can't see anything, the world blurrs around you!
~
#32408
Kerriston_north_horse_timer~
0 j 100
~
* This is the reception of the carrot
* that kicks off the whole affair.
if %actor.vnum% == -1
   if %object.vnum% == 32421
      wait 1
      mteleport %actor% 32597
      mteleport %self% 32597
      mjunk carrot
      do_it_to_it
      wait 5 s
      mecho The world spins by you as you struggle to hang on!
      wait 5 s
      mecho The sound of thundering hooves is almost deafening!
      wait 5 s
      mecho You slip and nearly fall off!
      wait 5 s
      mecho The horse bucks wildly almost throwing you!
      wait 5 s
      mecho The tall grass whips by your feet so quickly it stings!
      wait 5 s
      mecho Are you slowing down?
      wait 2 s
      mteleport %actor% 32461
      mteleport %self% 32461
      wait 1
      mforce %actor% look
      pant
      mecho The trained &3horse&0 wiggles from underneath you.
      wait 1
      mpecho The trained &3horse&0 is summoned by his master.
      mpecho The trained &3horse&0 disappears in a puff of smoke.
   else
      wait 1
      eye %actor.name%
      wait 3
      bite %actor.name%
      smile me
   end
else
end
purge_me
~
#32409
Horse_ride_room_spam~
2 c 100
do_it_to_it~
   set counter 25
   while %counter%
         wat 32597 wecho You catch a glimpse of the passing countryside!
	 wat 32597 wecho &0&3Dirt Road&0 cutting through &0&2Vast Grasslands&0
	 wat 32597 wecho &3   This simple yet well mantained road runs a relativly straight direction,
 	 wat 32597 wecho both north and south, though many twists and bends add to its character. Its
	 wat 32597 wecho brown surface cuts a path through the vast grasslands on either side, their
	 wat 32597 wecho endless lengths appearing to stretch on forever. Though the scenary is
	 wat 32597 wecho serene, danger could be lurking close by in many forms, including bandits.
	 wat 32597 wecho An occasional patrol from Kerristone passes through the area, though for the most
	 wat 32597 wecho part travelers are left to fend for themselves.&0
	 wait 4
	 wat 32597 wecho &0&3Dirt Road&0 cutting through &0&2Vast Grasslands&0
	 wat 32597 wecho &3   This simple yet well mantained road runs a relativly straight direction,
 	 wat 32597 wecho both north and south, though many twists and bends add to its character. Its
	 wat 32597 wecho brown surface cuts a path through the vast grasslands on either side, their
	 wat 32597 wecho endless lengths appearing to stretch on forever. Though the scenary is
	 wat 32597 wecho serene, danger could be lurking close by in many forms, including bandits.
	 wat 32597 wecho An occasional patrol from Kerristone passes through the area, though for the most
	 wat 32597 wecho part travelers are left to fend for themselves.&0
         eval counter %counter% -1
         wait 1 s
         if %counter% == 0
            break
         else
         end
   done
~
#32410
Kerristone_north_trainer_rand1~
0 b 40
~
say &0I only deal in &b&3gold&0!
say &0And No! I dont have change.&0
~
#32411
Kerristone_north_trainer_rand2~
0 b 40
~
* Hrmmm
wait 1
say Fine horses! Why walk?
smile
wait 1
say You can use one of my fine horses and enjoy the trip!
mecho %self.name% says, 'Only 30 &b&3gold&0 pieces to get to Morgan Hill!'
mecho %self.name% says, '&0&0Twelve&0 more and you can go all the way to the &b&6&0&b&9Rugged&0 Moutains!&0'
ponder
~
#32412
Kerristone_north_thing_rand~
0 b 49
~
* This was a prog that mppurged on the
* random check, no idea why... If it 
* becomes obvious someone add the purge
* below after commenting the plan.
grin
~
#32413
Kerristone_north_trainer_bribeMG~
0 m 42
~
* This is one of the bribe progs that will
* let players buy a horse to bolt to Mugnork
* The horse trigs have been altered in concept
* on how they deliver the players to the destination
* but there's no reason why this trig can't exist
* as is to kick it off
if %actor.vnum% == -1
   wait 1
   mecho %self.name% says, '&0Ahh Thankyou! You will not be disappointed.&0'
   mecho %self.name% says, '&0I have one of the fastest &0&b&9stallions&0 in the land for you!'
   wait 2
   mecho &0A &3horse&0 trainer leads a fine looking &b&9stallion&0 into the room.
   mload mob 32423
   wait 2
   mecho &0A horse trainer gets under you and pushes you up into the saddle.&0
   mload obj 32425
   mecho %self.name% says, 'Feed him this &1strawberry&0 and he will know that you want to go to &b&6Mugnork&0.'
   give strawberry %actor.name%
   wait 2
   mecho &0A &3horse&0 trainer forces your hand towards the mouth of the horse.
   mforce %actor% give strawberry stallion
   mecho %self.name% says, 'Good journey!'
else
end
~
#32414
Kerristone_north_trainer_bribe_mh~
0 m 30
~
* This is one of the bribe trigs that will
* let a player buy a horse to take them to
* morgan hill.  The horse portion will change
* radically from the prog format but will 
* achieve the same goals.
if %actor.vnum% == -1
   wait 1
   mecho %self.name% says, 'Ahh Thankyou! You will not be disappointed.'
   mecho %self.name% says, 'I have one of the fastest horses in the land for you!'
   wait 2
   mpecho &0The &3horse&0 trainer leads a fine looking horse into the room.&0
   mload mob 32421
   mecho &0The &3horse&0 trainer gets under you and pushes you up into the saddle.&0
   wait 2
   mload obj 32421
   mecho %self.name% says, '&0Feed him this &0&3carrot&0 and he will know that you want to go to Morgan Hill.&0'
   give carrot %actor.name%
   mecho &0The &3horse&0 trainer forces your hand towards the mouth of the &3horse&0.
   wait 2
   mecho %self.name% says, 'Good journey!'
else
end
~
#32415
Kerristone_north_purge~
2 c 100
purge_me~
* this is a room purge to see if a world 
* purge will dump a mobile as mpurge %self%
* seems broken
if %actor.vnum% == 32421
   wait 1
   wpurge %actor%
else
end
if %actor.vnum% == 32423
   wait 1
   wpurge %actor%
else
end
~
#32416
Horse_ride_room_spam2~
2 c 100
do_it_to_it2~
   set counter 7
   while %counter%
	 wat 32598 wecho You catch a glimpse of the passing countryside!
	 wat 32598 wecho &3Dirt Road&0 through the Sloping &0&3Highlands&0
         wat 32598 wecho &3   A dirt road cuts through the uneven landscape, over rolling hills and
         wat 32598 wecho shallow valleys. Though not as well kept as some of the southern roads, it
         wat 32598 wecho still surves its purpose. Wagon tracks cover most of its surface, both
         wat 32598 wecho recent and ancient, their existance testimony to the traffic this road has
         wat 32598 wecho sustained over the many years. The surrounding hills are mostly bare, though
         wat 32598 wecho some contain small groups of trees or thick underbrush. The occasional
         wat 32598 wecho glint of water can be seen, coming from one of the many small ponds that
         wat 32598 wecho are tucked between the steep hillsides.&0
	 wait 4
	 wat 32598 wecho You catch a glimpse of the passing countryside!
	 wat 32598 wecho &3Dirt Road&0 through the Sloping &0&3Highlands&0
         wat 32598 wecho &3   A dirt road cuts through the uneven landscape, over rolling hills and
         wat 32598 wecho shallow valleys. Though not as well kept as some of the southern roads, it
         wat 32598 wecho still surves its purpose. Wagon tracks cover most of its surface, both
         wat 32598 wecho recent and ancient, their existance testimony to the traffic this road has
         wat 32598 wecho sustained over the many years. The surrounding hills are mostly bare, though
         wat 32598 wecho some contain small groups of trees or thick underbrush. The occasional
         wat 32598 wecho glint of water can be seen, coming from one of the many small ponds that
         wat 32598 wecho are tucked between the steep hillsides.&0
         eval counter %counter% -1
         wait 1 s
         if %counter% == 0
            break
         else
         end
   done
wforce horse do_it_to_it3
~
#32417
Horse_ride_room_spam3~
2 c 100
do_it_to_it3~
   set counter 5
   while %counter%
	 wat 32598 wecho You catch a glimpse of the passing countryside!
	 wat 32598 wecho &3Road through the &0&9&bRugged Hills&0
         wat 32598 wecho &3   This simple dirt road cuts through the hilly landscape with great
         wat 32598 wecho precision, winding through the obstacles with perfect planning. The road
         wat 32598 wecho engineer must have spent ages mapping out this road system, though over the
         wat 32598 wecho years it has started to show signs of disrepair. The rocky hills surrounding
         wat 32598 wecho the road look cold and bleak, the gloomy terrain only broken by the
         wat 32598 wecho occasional patch of snow or copse of pine trees. Black boulders cover many
         wat 32598 wecho of the hills, their outline weathered into grotesque shapes.&0
	 wait 4
	 wat 32598 wecho You catch a glimpse of the passing countryside!
	 wat 32598 wecho &3Road through the &0&9&bRugged Hills&0
         wat 32598 wecho &3   This simple dirt road cuts through the hilly landscape with great
         wat 32598 wecho precision, winding through the obstacles with perfect planning. The road
         wat 32598 wecho engineer must have spent ages mapping out this road system, though over the
         wat 32598 wecho years it has started to show signs of disrepair. The rocky hills surrounding
         wat 32598 wecho the road look cold and bleak, the gloomy terrain only broken by the
         wat 32598 wecho occasional patch of snow or copse of pine trees. Black boulders cover many
         wat 32598 wecho of the hills, their outline weathered into grotesque shapes.&0
         eval counter %counter% -1
         wait 1 s
         if %counter% == 0
            break
            do_it_to_it4
         else
         end
   done
wforce horse do_it_to_it4
~
#32418
Horse_ride_room_spam4~
2 c 100
do_it_to_it4~
   set counter 3
   while %counter%
	 wat 32598 wecho You catch a glimpse of the passing countryside!
	 wat 32598 wecho &3Dirt&0 Road along the Shores of &0&4&bLake Fedis&0
	 wat 32598 wecho &3   This curving dirt road leads along the shores of Lake Fedis, one of
	 wat 32598 wecho the largest lakes found in the highlands. Its icy waters are constantly fed by
	 wat 32598 wecho the WhiteFox river, which is created by melting snow in the Wailing
	 wat 32598 wecho Mountains. The road itself is wide enough to allow two or three merchant
	 wat 32598 wecho wagons to pass at once, though if there should be an accident the sloping
	 wat 32598 wecho ditches could provide a problem. Rumors persist about Lake Fedis, some
	 wat 32598 wecho speaking of an ancient lake beast that lives within its depths. The Lakefolk
	 wat 32598 wecho who have made their home on its shores do not speak of such things, fearing
	 wat 32598 wecho that it will bring bad luck.&0
	 wait 4
	 wat 32598 wecho You catch a glimpse of the passing countryside!
	 wat 32598 wecho &3Dirt&0 Road along the Shores of &0&4&bLake Fedis&0
	 wat 32598 wecho &3   This curving dirt road leads along the shores of Lake Fedis, one of
	 wat 32598 wecho the largest lakes found in the highlands. Its icy waters are constantly fed by
	 wat 32598 wecho the WhiteFox river, which is created by melting snow in the Wailing
	 wat 32598 wecho Mountains. The road itself is wide enough to allow two or three merchant
	 wat 32598 wecho wagons to pass at once, though if there should be an accident the sloping
	 wat 32598 wecho ditches could provide a problem. Rumors persist about Lake Fedis, some
	 wat 32598 wecho speaking of an ancient lake beast that lives within its depths. The Lakefolk
	 wat 32598 wecho who have made their home on its shores do not speak of such things, fearing
	 wat 32598 wecho that it will bring bad luck.&0
         eval counter %counter% -1
         wait 1 s
         if %counter% == 0
             break
         else
         end
   done
~
#32419
Kerriston_north_horse_timer2~
0 j 100
~
* This is the reception of the strawberry
* that kicks off the whole affair to Mugnork
if %actor.vnum% == -1
   if %object.vnum% == 32425
      wait 1
      mteleport %actor% 32597
      mteleport %self% 32597
      mjunk strawberry
      do_it_to_it
      wait 5 s
      mecho The world spins by you as you struggle to hang on!
      wait 5 s
      mecho The sound of thundering hooves is almost deafening!
      wait 5 s
      mecho You slip and nearly fall off!
      wait 5 s
      mecho The horse bucks wildly almost throwing you!
      wait 5 s
      mecho The tall grass whips by your feet so quickly it stings!
      wait 5 s
      mteleport %actor% 32598
      mteleport %self% 32598
      wait 1
      do_it_to_it2
      mecho Are you slowing down?
      wait 5 s
      mecho A small speckled rabbit runs between the horses legs! SPLAT
      wait 5 s
      mecho The world spins by you as you struggle to hang on!
      wait 5 s
      mecho You slip and nearly fall off!
      wait 5 s
      mecho A Northern Deer darts across the trail infront of you!
      wait 5
      mecho PHEW Nearly teeth, hair, and eyeballs everywhere!
      wait 3
      mteleport %actor% 32565
      mteleport %self% 32565
      wait 1
      mforce %actor% look
      pant
      mecho %self.name% wiggles from underneath you.
      wait 1
      mpecho %self.name% is summoned by his master.
      mpecho %self.name% disappears in a puff of smoke.
   else
      wait 1
      eye %actor.name%
      wait 3
      bite %actor.name%
      smile me
   end
else
end
purge_me
~
$~
