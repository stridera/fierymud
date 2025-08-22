#36400
illusory_wall_lyara_greet~
0 h 100
~
wait 1s
emote looks up in surprise.
say Well hello there!
stand
salute
wait 2s
if ((%actor.class% /= illusionist || %actor.class% /= bard) && %actor.level% > 56 && %actor.quest_stage[illusory_wall]% == 0)
    mecho %self.name% says, 'Well well well, an up-and-coming Illusionist!  The
    mecho &0Guard must need reinforcements.'
elseif %actor.quest_stage[illusory_wall]% == 1
    say Have you found the requisite supplies?
elseif %actor.quest_stage[illusory_wall]% > 1
    say I hope your studies are continuing well.
endif
~
#36401
illusory_wall_lyara_speech1~
0 d 100
reinforcements reinforcements? guard guard?~
if ((%actor.class% /= illusionist || %actor.class% /= bard) && %actor.level% > 56)
    wait 1s
    mecho %self.name% says, 'Yes, they still send requests for reinforcements
    mecho &0from time to time.'
    wait 4s
    say Wait, do you not know who I am?
    wait 4s
    laugh
    say The name's Lyara.  Post Commander Ruzhana Lyara.
    wait 2s
    mecho %self.name% says, 'I served with the Eldorian Guard for many years as
    mecho &0Post Commander, the master of fortifications.  My speciality is magical walls
    mecho &0and barriers, particularly &b&5I&6l&5l&6u&5s&6o&5r&6y &5W&6a&5l&6l&0.  Even after my retirement, many have
    mecho &0sought me out asking me to teach them.'
endif
~
#36402
illusory_wall_lyara_speech2~
0 d 100
teach teacher student teach? teacher? student?~
if ((%actor.class% /= illusionist || %actor.class% /= bard) && %actor.level% > 56)
    wait 2
    mecho %self.name% says, 'I don't get many students these days.  They just
    mecho &0don't have enough patience...'
    wait 4s
    grumble
    wait 3s
    mecho %self.name% says, 'But if you're willing to take the time to study, I
    mecho &0can take you on as my pupil for a bit.'
    wait 4s
    say Are you willing?
endif
~
#36403
illusory_wall_lyara_speech3~
0 d 100
yes willing sure okay~
if (((%actor.class% /= illusionist || %actor.class% /= bard) && %actor.level% > 56) && !%actor.quest_stage[illusory_wall]%)
    quest start illusory_wall %actor.name%
    wait 2
    say Splendid!  Let's get you set up.
    wait 2s
    mecho %self.name% says, 'Replicating walls with tangible illusions is
    mecho &0difficult.  Typically I assign my students an extensive study of walls and
    mecho &0doors so they can formulate their own theories and aesthetics.'
    wait 7s
    mecho %self.name% says, 'But it will require a special study tool:
    mecho &0&3&ba pair of magic spectacles&0.'
    wait 3s
    say To construct you a pair, I'll need three things:
    mecho - &3&bA pair of glasses&0 or &3&bsmall spectacles&0 to look through
    mecho - &3&bA prismatic leg spur&0 to refract light properly
    mecho - &3&bA small piece of petrified magic&0 to enhance the magical sight of the lenses
    wait 5s
    say Bring these to me and I'll get you outfitted.
    wait 2s
    mecho %self.name% says, 'And remember, you can always check your &7&b[progress]&0
    mecho &0with me at any time.'
endif
~
#36404
illusory_wall_lyara_receive~
0 j 100
~
if %actor.quest_stage[illusory_wall]% == 1
    set item1 %actor.quest_variable[illusory_wall:18511]%
    set item2 %actor.quest_variable[illusory_wall:10307]%
    set item3 %actor.quest_variable[illusory_wall:41005]%
    set item4 %actor.quest_variable[illusory_wall:51017]%
    if %actor.quest_variable[illusory_wall:%object.vnum%]% == 1
        return 0
        wait 1s
        mecho %self.name% refuses %obj_shortdesc%.
        shake
        say You have already given me this.
    elseif (%item1% == 1 && %object.vnum% == 10307) || (%item2% == 1 && %object.vnum% == 18511)
        return 0
        wait 1s
        mecho %self.name% refuses %obj_shortdesc%.
        shake 
        mecho %self.name% says, 'You have already given me something suitable for
        mecho &0magic lenses.'
    else
        if %object.vnum% == 10307 || %object.vnum% == 18511
            quest variable illusory_wall %actor.name% %object.vnum% 1
            wait 2
            mjunk %object%    
            wait 1s
            say These are excellent lenses.
        elseif %object.vnum% == 41005
            quest variable illusory_wall %actor.name% %object.vnum% 1
            wait 2
            mjunk %object%
            wait 1s
            mecho %self.name% says, 'Yes, this will do nicely.  Hopeflly you gained a
            mecho &0few insights on bending light from the prisms in the Hive as well.'
        elseif %object.vnum% == 51017
            quest variable illusory_wall %actor.name% %object.vnum% 1
            wait 2
            mjunk %object%
            wait 1s
            mecho %self.name% looks at the odd glowing lump.
            say Ah yes, plenty of extra juice in here.
        else
            return 0
            wait 1s
            mecho %self.name% refuses %obj_shortdesc%.
            shake
            say This won't help you see through illusions any better.
        endif
        set item1 %actor.quest_variable[illusory_wall:18511]%
        set item2 %actor.quest_variable[illusory_wall:10307]%
        set item3 %actor.quest_variable[illusory_wall:41005]%
        set item4 %actor.quest_variable[illusory_wall:51017]%
        if ((%item1% || %item2%) && %item3% && %item4%)
            quest advance illusory_wall %actor.name%
            quest variable illusory_wall %actor.name% total 0
            wait 2s
            say Wonderful, looks like this is everything.
            wait 2s
            mecho %self.name% holds the prismatic leg spur and the lenses next to each other and utters a few words.
            mecho &b&3The lenses fuse with the leg spur, taking on a prismatic quality!&0
            wait 4s
            mecho %self.name% holds the lump of magic over the prismatic lenses and whispers an incantation.
            mecho &3&bThe magic melds with the lenses!&0
            wait 3s
            mload obj 36400
            mecho %self.name% says, 'These lenses will show you little hidden details
            mecho &0about doors and exits.'
            give lenses %actor.name%
            wait 3s
            mecho %self.name% says, '&7&b[Examine]&0 20 different closeable exits in 20
            mecho &0different parts of the world.'
            wait 2s
            mecho %self.name% says, 'Once you have, you should know enough to replicate
            mecho &0such barriers on your own.'
            wait 4s
            say Good luck!
        else
            wait 2s
            say Do you have the other necessities?
        endif
    endif
elseif %actor.quest_stage[illusory_wall]% > 1
    return 0
    wait 1s
    mecho %self.name% refuses %obj_shortdesc%.
    say There's nothing else you need to bring me.
else
    return 0
    wait 1s
    mecho %self.name% refuses %obj_shortdesc%.
    say No need for supplies, soldier.
    salute %actor.name%
endif
~
#36405
illusory_wall_glasses_examine~
1 c 3
examine~
switch %cmd%
  case e
    return 0
    halt
done
if %arg%
   return 0
   halt
endif
set room %self.room%
eval zone (%room.vnum% / 100)
switch %zone%
    case 16
        set region outback
        break
    case 18
        set region shadows
        break
    case 20
        set region merchant
        break
    case 23
    case 24
    case 25
    case 26
    case 27
        set region caelia_west
        break
    case 28 
        set region river
        break
    case 30
    case 31
    case 32
    case 33
    case 34
    case 53
        set region mielikki
        break
    case 35
    case 36
    case 37
        set region mielikki_forest
        break
    case 40
        set region labyrinth
        break
    case 41
    case 42
        set region split
        break
    case 43
        set region theater
        break
    case 51
        set region rocky_tunnels
        break
    case 52
        set region lava
        break
    case 54
    case 127
    case 128
    case 361
        set region misty
        break
    case 55
        set region combat
        break
    case 60
    case 61
    case 62
        set region anduin
        break
    case 69
        set region pastures
        break
    case 70
        set region great_road
        break
    case 73
        set region nswamps
        break
    case 80
    case 81
    case 82
        set region farmlands
        break
    case 83
        set region frakati
        break
    case 85
        set region cathedral
        break
    case 86
        set region meercats
        break
    case 87
        set region logging
        break
    case 88
        set region dairy
        break
    case 100
        set region ickle
        break
    case 102
        set region frostbite
        break
    case 103
        set region phoenix
        break
    case 117
    case 118
    case 119
        set region blue_fog_trail
        break
    case 120
    case 121
    case 122
        set region twisted
        break
    case 123
    case 124 
        set region megalith
        break
    case 125
    case 126
        set region tower
        break
    case 133
        set region miner
        break
    case 136
        set region morgan
        break
    case 160
    case 164
        set region mystwatch
        break
    case 161
        set region desert
        break
    case 162
        set region pyramid
        break
    case 163
        set region highlands
        break
    case 169
        set region haunted
        break
    case 172
        set region citadel
        break
    case 173
        set region chaos
        break
    case 178
        set region canyon
        break
    case 180
        set region topiary
        break
    case 185
        set region abbey
        break
    case 203
        set region plains
        break
    case 237
        set region dheduu
        break
    case 238
        set region dargentan
        break
    case 300
    case 301
        set region ogakh
        break
    case 302
        set region bluebonnet
        break
    case 324
    case 325
        set region caelia_east
        break
    case 350
    case 351
        set region brush
        break
    case 360
        set region kaaz
        break
    case 362
    case 411
    case 412
        set region seawitch
        break
    case 363
        set region smuggler
        break
    case 364
        set region sirestis
        break
    case 365
        set region ancient_ruins
        break
    case 370
        set region minithawkin
        break
    case 390
    case 391
        set region arabel
        break
    case 410
        set region hive
        break
    case 430
    case 431
    case 432
        set region demise
        break
    case 462
        set region nukreth
        break
    case 464
        set region aviary
        break
    case 470
    case 471
    case 472
    case 473
    case 474
        set region graveyard
        break
    case 476
        set region earth
        break
    case 477
        set region water
        breal
    case 478
        set region fire
        break
    case 480
        set region barrow
        break
    case 481
    case 482
        set region fiery
        break
    case 484
        set region doom
        break
    case 488
        set region air
        break
    case 489
        set region lokari
        break
    case 490
    case 491
        set region griffin
        break
    case 492
        set region blackice
        break
    case 495
        set region nymrill
        break
    case 502
        set region bayou
        break
    case 510
    case 511
        set region nordus
        break
    case 520
        set region templace
        break
    case 530
    case 531
    case 532
        set region sunken
        break
    case 533
        set region cult
        break
    case 534
    case 535
        set region frost
        break
    case 550
    case 551
        set region tech
        break
    case 552
        set region black_woods
        break
    case 553
        set region kaas_plains
        break
    case 554
        set region dark_mountains
        break
    case 555
        set region cold_fields
        break
    case 556
        set region iron
        break
    case 557
        set region blackrock
        break
    case 558
    case 559
        set region eldorian
        break
    case 564
        set region blacklake
        break
    case 580
    case 581
    case 582
        set region odz
        break
    case 583
        set region syric
        break
    case 584
    case 585
        set region kod
        break
    case 586
    case 587
        set region beachhead
        break
    case 588
    case 589
        set region ice_warrior
        break
    case 590
        set region haven
        break
    case 615
        set region hollow
        break
    case 625
        set region rhell
        break
    default
        halt
done
if %actor.quest_stage[illusory_wall]% == 2 && !%actor.has_completed[illusory_wall]%
    if (%room.up[bits]% /= DOOR || %room.down[bits]% /= DOOR || %room.east[bits]% /= DOOR || %room.west[bits]% /= DOOR || %room.north[bits]% /= DOOR || %room.south[bits]% /= DOOR)
        if %actor.quest_variable[illusory_wall:%region%]%
            osend %actor% &b&7You have already learned all you can from this region.&0
            return 0
            halt
        else
            quest variable illusory_wall %actor.name% %region% 1
            eval clue %actor.quest_variable[illusory_wall:total]% + 1
            osend %actor% &7&bYou begin to analyze the room.&0
            wait 3s
            osend %actor% &3&bAnalyzing...&0
            wait 3s
            osend %actor% &3&bAnalyzing...&0
            wait 3s
            osend %actor% &3&bAnalyzing...&0
            wait 3s
            osend %actor% &b&6You gain more insight on doors and barriers!&0            
            quest variable illusory_wall %actor.name% total %clue%
            if %actor.quest_variable[illusory_wall:total]% >= 20
                wait 2s
                oload mob 36402
                oforce post-commander mskillset %actor.name% illusory wall
                osend %actor% &b&6You have learned everything you need to cast illusory walls!&0
                quest complete illusory_wall %actor.name%
                wait 1
                opurge %mob%
            endif  
        endif
    else
        return 0
    endif
else
    return 0
endif
~
#36406
illusory_wall_lyara_status~
0 d 100
status status? progress progress?~
set stage %actor.quest_stage[illusory_wall]%
set item1 %actor.quest_variable[illusory_wall:10307]%
set item2 %actor.quest_variable[illusory_wall:18511]%
set item3 %actor.quest_variable[illusory_wall:41005]%
set item4 %actor.quest_variable[illusory_wall:51017]%
wait 2
if %actor.class% != illusionist && %actor.class% != bard
    mecho %self.name% says, 'I appreciate your interest but I have nothing I
    mecho &0can teach you.'
elseif %actor.has_completed[illusory_wall]%
    mecho %self.name% says, 'I have already taught you &b&5I&6l&5l&6u&5s&6o&5r&6y &5W&6a&5l&6l&0.
    mecho &0I have nothing else to teach you, soldier.'
    salute %actor.name%
elseif %stage% == 0
    say I haven't agreed to teach you yet.
elseif %stage% == 1
    say You're looking for things to make magical spectacles.
    if (%item1% || %item2% || %item3% || %item4%)
        mecho   
        mecho &0You have already brought me:
        if %item1%
            mecho - &7&b%get.obj_shortdesc[10307]%&0
        endif
        if %item2%
            mecho - &7&b%get.obj_shortdesc[18511]%&0
        endif
        if %item3%
            mecho - &7&b%get.obj_shortdesc[41005]%&0
        endif
        if %item4%
            mecho - &7&b%get.obj_shortdesc[51017]%&0
        endif
    endif
    mecho   
    mecho You still need to find:
    if (!%item1% && !%item2%)
        mecho - &3&b%get.obj_shortdesc[10307]%&0 or &3&b%get.obj_shortdesc[18511]%&0
    endif        
    if !%item3%
        mecho - &3&b%get.obj_shortdesc[41005]%&0
    endif
    if !%item4%
        mecho - &3&b%get.obj_shortdesc[51017]%&0
    endif
elseif %stage% == 2
    say Complete your study of doors in 20 regions.
    mecho 
    set doors %actor.quest_variable[illusory_wall:total]%
    mecho You have examined doors in &5&b%doors%&0 regions:
        if %actor.quest_variable[illusory_wall:Outback]%
            mecho - &bRocky Outback&0
        endif
        if %actor.quest_variable[illusory_wall:Shadows]%
            mecho - &bForest of Shadows&0
        endif
        if %actor.quest_variable[illusory_wall:Merchant]%
            mecho - &bOld Merchant Trail&0
        endif
        if %actor.quest_variable[illusory_wall:Caelia_West]%
            mecho - &bSouth Caelia West&0
        endif
        if %actor.quest_variable[illusory_wall:River]%
            mecho - &bBlue-Fog River&0
        endif
        if %actor.quest_variable[illusory_wall:Mielikki]%
            mecho - &bThe Village of Mielikki&0
        endif
        if %actor.quest_variable[illusory_wall:Mielikki_Forest]%
            mecho - &bMielikki's Forests&0
        endif
        if %actor.quest_variable[illusory_wall:Labyrinth]%
            mecho - &bLaveryn Labyrinth&0
        endif
        if %actor.quest_variable[illusory_wall:Split]%
            mecho - &bSplit Skull&0
        endif
        if %actor.quest_variable[illusory_wall:Theatre]%
            mecho - &bThe Theatre in Anduin&0
        endif
        if %actor.quest_variable[illusory_wall:Rocky_Tunnels]%
            mecho - &bRocky Tunnels&0
        endif
        if %actor.quest_variable[illusory_wall:Lava]%
            mecho - &bLava Tunnels&0
        endif
        if %actor.quest_variable[illusory_wall:Misty]%
            mecho - &bMisty Caverns&0
        endif
        if %actor.quest_variable[illusory_wall:Combat]%
            mecho - &bCombat in Eldoria&0
        endif
        if %actor.quest_variable[illusory_wall:Anduin]%
            mecho - &bThe City of Anduin&0
        endif
        if %actor.quest_variable[illusory_wall:Pastures]%
            mecho - &bAnduin Pastures&0
        endif
        if %actor.quest_variable[illusory_wall:Great_Road]%
            mecho - &bThe Great Road&0
        endif
        if %actor.quest_variable[illusory_wall:Nswamps]%
            mecho - &bThe Northern Swamps&0
        endif
        if %actor.quest_variable[illusory_wall:Farmlands]%
            mecho - &bMielikki Farmlands&0
        endif
        if %actor.quest_variable[illusory_wall:Frakati]%
            mecho - &bFrakati Reservation&0
        endif
        if %actor.quest_variable[illusory_wall:Cathedral]%
            mecho - &bCathedral of Betrayal&0
        endif
        if %actor.quest_variable[illusory_wall:Meercats]%
            mecho - &bKingdom of the Meer Cats&0
        endif
        if %actor.quest_variable[illusory_wall:Logging]%
            mecho - &bThe Logging Camp&0
        endif
        if %actor.quest_variable[illusory_wall:Dairy]%
            mecho - &bThe Dairy Farm&0
        endif
        if %actor.quest_variable[illusory_wall:Ickle]%
            mecho - &bIckle&0
        endif
        if %actor.quest_variable[illusory_wall:Frostbite]%
            mecho - &bMount Frostbite&0
        endif
        if %actor.quest_variable[illusory_wall:Phoenix]%
            mecho - &bPhoenix Feather Hot Spring&0
        endif
        if %actor.quest_variable[illusory_wall:Blue_Fog_Trail]%
            mecho - &bBlue-Fog Trail&0
        endif
        if %actor.quest_variable[illusory_wall:Twisted]%
            mecho - &bTwisted Forest&0
        endif
        if %actor.quest_variable[illusory_wall:Megalith]%
            mecho - &bThe Sacred Megalith&0
        endif
        if %actor.quest_variable[illusory_wall:Tower]%
            mecho - &bThe Tower in the Wasted&0
        endif
        if %actor.quest_variable[illusory_wall:Miner]%
            mecho - &bThe Miner's Cavern&0
        endif
        if %actor.quest_variable[illusory_wall:Morgan]%
            mecho - &bMorgan Hill&0
        endif
        if %actor.quest_variable[illusory_wall:Mystwatch]%
            mecho - &bThe Fortress of Mystwatch&0
        endif
        if %actor.quest_variable[illusory_wall:Desert]%
            mecho - &bGothra Desert&0
        endif
        if %actor.quest_variable[illusory_wall:Pyramid]%
            mecho - &bGothra Pyramid&0
        endif
        if %actor.quest_variable[illusory_wall:Highlands]%
            mecho - &bHighlands&0
        endif
        if %actor.quest_variable[illusory_wall:Haunted]%
            mecho - &bThe Haunted House&0
        endif
        if %actor.quest_variable[illusory_wall:Citadel]%
            mecho - &bThe Citadel of Testing&0
        endif
        if %actor.quest_variable[illusory_wall:Chaos]%
            mecho - &bTemple of Chaos&0
        endif
        if %actor.quest_variable[illusory_wall:Canyon]%
            mecho - &bCanyon&0
        endif
        if %actor.quest_variable[illusory_wall:Topiary]%
            mecho - &bMielikki's Topiary&0
        endif
        if %actor.quest_variable[illusory_wall:Abbey]%
            mecho - &bThe Abbey&0
        endif
        if %actor.quest_variable[illusory_wall:Plains]%
            mecho - &bGothra Plains&0
        endif
        if %actor.quest_variable[illusory_wall:Dheduu]%
            mecho - &bDheduu&0
        endif
        if %actor.quest_variable[illusory_wall:Dargentan]%
            mecho - &bDargentan's Lair&0
        endif
        if %actor.quest_variable[illusory_wall:Ogakh]%
            mecho - &bOgakh&0
        endif
        if %actor.quest_variable[illusory_wall:Bluebonnet]%
            mecho - &bBluebonnet Pass&0
        endif
        if %actor.quest_variable[illusory_wall:Caelia_East]%
            mecho - &bSouth Caelia East&0
        endif
        if %actor.quest_variable[illusory_wall:Brush]%
            mecho - &bBrush Lands&0
        endif
        if %actor.quest_variable[illusory_wall:Kaaz]%
            mecho - &bTemple of the Kaaz&0
        endif
        if %actor.quest_variable[illusory_wall:SeaWitch]%
            mecho - &bSea's Lullaby&0
        endif
        if %actor.quest_variable[illusory_wall:Smuggler]%
            mecho - &bSmuggler's Hideout&0
        endif
        if %actor.quest_variable[illusory_wall:Sirestis]%
            mecho - &bSirestis' Folly&0
        endif
        if %actor.quest_variable[illusory_wall:Ancient_Ruins]%
            mecho - &bAncient Ruins&0
        endif
        if %actor.quest_variable[illusory_wall:Minithawkin]%
            mecho - &bMinithawkin Mines&0
        endif
        if %actor.quest_variable[illusory_wall:Arabel]%
            mecho - &bArabel Ocean&0
        endif
        if %actor.quest_variable[illusory_wall:Hive]%
            mecho - &bHive&0
        endif
        if %actor.quest_variable[illusory_wall:Demise]%
            mecho - &bDemise Keep&0
        endif
        if %actor.quest_variable[illusory_wall:Aviary]%
            mecho - &bIckle's Aviary&0
        endif
        if %actor.quest_variable[illusory_wall:Graveyard]%
            mecho - &bThe Graveyard&0
        endif
        if %actor.quest_variable[illusory_wall:Earth]%
            mecho - &bThe Plane of Earth&0
        endif
        if %actor.quest_variable[illusory_wall:Water]%
            mecho - &bThe Plane of Water&0
        endif
        if %actor.quest_variable[illusory_wall:Fire]%
            mecho - &bThe Plane of Fire&0
        endif
        if %actor.quest_variable[illusory_wall:Barrow]%
            mecho - &bThe Barrow&0
        endif
        if %actor.quest_variable[illusory_wall:Fiery]%
            mecho - &bFiery Island&0
        endif
        if %actor.quest_variable[illusory_wall:Nukreth]%
            mecho - &bNukreth Spire&0
        endif
        if %actor.quest_variable[illusory_wall:Doom]%
            mecho - &bAn Ancient Forest and Pyramid&0
        endif
        if %actor.quest_variable[illusory_wall:Air]%
            mecho - &bThe Plane of Air&0
        endif
        if %actor.quest_variable[illusory_wall:Lokari]%
            mecho - &bLokari's Keep&0
        endif
        if %actor.quest_variable[illusory_wall:Griffin]%
            mecho - &bGriffin Island&0
        endif
        if %actor.quest_variable[illusory_wall:BlackIce]%
            mecho - &bBlack-Ice Desert&0
        endif
        if %actor.quest_variable[illusory_wall:Nymrill]%
            mecho - &bThe Lost City of Nymrill&0
        endif
        if %actor.quest_variable[illusory_wall:Bayou]%
            mecho - &bThe Bayou&0
        endif
        if %actor.quest_variable[illusory_wall:Nordus]%
            mecho - &bThe Enchanted Village of Nordus&0
        endif
        if %actor.quest_variable[illusory_wall:Templace]%
            mecho - &bTemplace&0
        endif
        if %actor.quest_variable[illusory_wall:Sunken]%
            mecho - &bSunken Castle&0
        endif
        if %actor.quest_variable[illusory_wall:Cult]%
            mecho - &bIce Cult&0
        endif
        if %actor.quest_variable[illusory_wall:Frost]%
            mecho - &bFrost Valley&0
        endif
        if %actor.quest_variable[illusory_wall:Technitzitlan]%
            mecho - &bTechnitzitlan&0
        endif
        if %actor.quest_variable[illusory_wall:Black_Woods]%
            mecho - &bBlack Woods&0
        endif
        if %actor.quest_variable[illusory_wall:Kaas_Plains]%
            mecho - &bKaas Plains&0
        endif
        if %actor.quest_variable[illusory_wall:Dark_Mountains]%
            mecho - &bDark Mountains&0
        endif
        if %actor.quest_variable[illusory_wall:Cold_Fields]%
            mecho - &bCold Fields&0
        endif
        if %actor.quest_variable[illusory_wall:Iron]%
            mecho - &bIron Hills&0
        endif
        if %actor.quest_variable[illusory_wall:Blackrock]%
            mecho - &bBlack Rock Trail&0
        endif
        if %actor.quest_variable[illusory_wall:Eldorian]%
            mecho - &bEldorian Foothills&0
        endif
        if %actor.quest_variable[illusory_wall:Blacklake]%
            mecho - &bBlack Lake&0
        endif
        if %actor.quest_variable[illusory_wall:Odz]%
            mecho - &bOdaishyozen&0
        endif
        if %actor.quest_variable[illusory_wall:Syric]%
            mecho - &bSyric Mountain Trail&0
        endif
        if %actor.quest_variable[illusory_wall:KoD]%
            mecho - &bKingdom of Dreams&0
        endif
        if %actor.quest_variable[illusory_wall:Beachhead]%
            mecho - &bBeachhead&0
        endif
        if %actor.quest_variable[illusory_wall:Ice_Warrior]%
            mecho - &bThe Ice Warrior's Compound&0
        endif
        if %actor.quest_variable[illusory_wall:Haven]%
            mecho - &bSacred Haven&0
        endif
        if %actor.quest_variable[illusory_wall:Hollow]%
            mecho - &bEnchanted Hollow&0
        endif
        if %actor.quest_variable[illusory_wall:Rhell]%
            mecho - &bThe Rhell Forest&0
        endif
    mecho 
    eval remaining (20 - %doors%)
    mecho Locate doors in &5&b%remaining%&0 more regions.
    mecho  
    mecho If you need new lenses say, &5&b"I need new glasses"&0.
endif
~
#36407
berserker_command_dig~
2 c 100
dig~
switch %cmd%
  case d
  case di
    return 0
    halt
done
if %actor.vnum% == -1
    wsend %actor% You dig out a path through the snow.
    wechoaround %actor% %actor.name% digs out a path through the snow.
    wat 36429 wecho The way south has been cleared.
    wat 36431 wecho The way north has been cleared.
    wdoor 36429 south room 36431
    wdoor 36431 north room 36429
    wait 15s
    wat 36429 wecho The snow begins to drift back in...
    wat 36431 wecho The snow begins to drift back in...
    wait 10s
    wat 36429 wecho The snow has completely covered the path south.
    wat 36431 wecho The snow has completely covered the path north.
    wdoor 36429 south purge
    wdoor 36431 north purge
endif
~
#36408
berserker_hjordis_greet~
0 g 100
~
wait 2
switch %actor.quest_stage[berserker_subclass]%
  case 1
    msend %actor% %self.name% says, 'You return!'
    wait 4
    msend %actor% %self.name% says, 'Let us discuss your &6&bWild Hunt&0!'
    break
  case 2
    msend %actor% %self.name% says, 'Will you answer the call?'
    wait 4
    msend %actor% %self.name% says, 'Let us challenge the Spirits for the right to prove ourselves!  If they deem you worthy, the Spirits send you a vision of a mighty &6&bbeast&0.'
    break
  case 3
    msend %actor% %self.name% says, 'You return!'
    wait 4
    msend %actor% %self.name% says, '&1&bHowl&0 to the spirits and make your song known!'
    break
  case 4
    msend %actor% %self.name% says, 'Why are you here when you should be out hunting your prey?'
    break
  default
    if %actor.class% /= Warrior 
      switch %actor.race%
*        case ADD RESTRICTED RACES HERE
*          break
        default
          if %actor.level% >= 10 && %actor.level% <= 25
            msend %actor% %self.name% says, 'Hail and well met!'
            msend %actor% %self.name% claps you on the back.
            mechoaround %actor% %self.name% claps %actor.name% on the back.
            wait 1s
            msend %actor% %self.name% says, 'Only the most ferocious souls find their way here.'
            wait 2s
            peer %actor%
            wait 2s
            msend %actor% %self.name% says, 'I see you could be &6&bamong&0 our number!'
          endif
      done
    endif
done
~
#36409
berserker_hjordis_speech1~
0 d 100
among among? number number? who who?~
if %actor.class% /= Warrior
  switch %actor.race%
*   case ADD RESTRICTED RACES HERE
*      if %actor.level% >= 10 && %actor.level% <= 25
*        msend %actor% &1Your race may not subclass to berserker.&0
*        halt
*      endif
*      break
    default
      wait 2
      if %actor.level% >= 10 && %actor.level% <= 25
        msend %actor% %self.name% says, 'Aye, you have the mettle of the most fearsome of all warriors, a &9&bber&1ser&9ker&0!'
        wait 2s
        msend %actor% %self.name% says, 'Do you think you have what it takes?'
      elseif %actor.level% < 10
        msend %actor% %self.name% says, 'Build your rage further, then seek me out again.'
      endif
  done
endif
~
#36410
berserker_hjordis_speech2~
0 d 100
yes~
if %actor.class% /= Warrior
  switch %actor.race%
*   case ADD RESTRICTED RACES HERE
*      if %actor.level% >= 10 && %actor.level% <= 25
*        msend %actor% &1Your race may not subclass to berserker.&0
*        halt
*      endif
*      break
    default
      wait 2
      if %actor.level% >= 10 && %actor.level% <= 25
        quest start berserker_subclass %actor.name% Ber
        msend %actor% %self.name% says, 'Then prove it!'
        laugh
        wait 2s
        msend %actor% %self.name% says, 'There are a few shared rites that bind us together.  None is more revered than the &6&bWild Hunt&0.'
        wait 2s
        msend %actor% %self.name% says, 'Ask your &6&b[subclass progress]&0 if you need guidance.'
      endif
  done
endif
~
#36411
berserker_hjordis_speech3~
0 d 0
wild hunt~
wait 2
if %actor.quest_stage[berserker_subclass]% == 1
  quest advance berserker_subclass %actor%
  nod
  msend %actor% %self.name% says, 'It is a tradition passed down through the generations.  We challenge the Spirits for the right to prove ourselves.  If they deem you worthy, the Spirits send you a vision of a mighty &6&bbeast&0.'
endif
~
#36412
berserker_hjordis_speech4~
0 d 100
beast beast?~
wait 2
if %actor.quest_stage[berserker_subclass]% == 2
  quest advance berserker_subclass %actor%
  msend %actor% %self.name% says, 'The target of our Wild Hunt is always left to the Spirits to determine.  It is always a ferocious predator.  But it could be just about anything.'
  wait 2s
  msend %actor% %self.name% says, 'The task, however, is always the same: Hunt down your quarry and slay it in &1&bsingle combat&0.'
  wait 3s
  msend %actor% %self.name% says, 'If you do, the Spirits will herald you as a berserker.'
  wait 3s
  msend %actor% %self.name% says, 'When you are ready, &1&bhowl&0 to the spirits and make your song known!'
endif
~
#36413
berserker_hjordis_command_howl~
0 c 100
howl~
switch %cmd%
  case h
  case ho
    return 0
    halt
done
if %actor.quest_stage[berserker_subclass]% == 3
  quest advance berserker_subclass %actor%
  msend %actor% You raise your voice in a mighty howl to the Spirits!
  mechoaround %actor% %actor.name% raises %hisher% voice in a mighty howl to the Spirits!
  wait 2
  switch %random.4%
    case 1
      set target 16105
      set place a desert cave
      break
    case 2
      set target 16310
      set place some forested highlands
      break
    case 3
      set target 20311
      set place a vast plain
      break
    case 4
    default
      set target 55220
      set place the frozen tundra
  done
  msend %actor% %self.name% throws her head back and howls along with you!
  mechoaround %actor% %self.name% throws her head back and howls along with %actor.name%!
  wait 2s
  quest variable berserker_subclass %actor% target %target%
  msend %actor% The Spirits reveal to you a vision of &3&b%get.mob_shortdesc[%target%]%&0!
  msend %actor% You see it is in &3&b%place%&0!
  wait 6s
  msend %actor% %self.name% says, 'The Spirits have spoken!'
  wait 2s
  msend %actor% %self.name% says, 'Find the beast of your Wild Hunt and join our ranks.  Remember, this quest must be undertaken &1&bALONE&0.  If you are grouped when you fight your prey, there will be consequences!'
else
  return 0
endif
~
#36414
berserker_target_fight~
0 k 100
~
set room %self.room%
set person %room.people%
while %person%
  if %person.quest_stage[berserker_subclass]% == 4
    if %person.group_size% != 0 && %person.quest_variable[berserker_subclass:target]% == %self.vnum%
      wait 2
      msend %person% &1&bYou must hunt this creature alone!  No groups allowed!&0
      eval zone ((%self.vnum% / 100) - 1)
      switch %zone%
        case 160
          msend %person% %self.name% whips you with her tail and sends you flying into a sandstorm!
          mechoaround %person% %self.name% whips %person.name% with her tail and sends %himher% flying into a sandstorm!
          mechoaround %person% %person.name% is gone!
          set random %random.86%
          break
        case 162
          msend %person% %self.name% howls and sends you fleeing in terror!
          mechoaround %person% %self.name% howls and sends %person.name% fleeing in terror!
          mechoaround %person% %person.name% is gone!
          set random %random.81%
          break
        case 202
          msend %person% %self.name% roars and hurdles you into the blinding heat!
          mechoaround %person% %self.name% roars and hurdles %person.name% into the blinding heat!
          mechoaround %person% %person.name% is gone!
          set random %random.79%
          break
        case 551
        default
          msend %person% %self.name% roars and hurdles you into the blinding snow!
          mechoaround %person% %self.name% roars and hurdles %person.name% into the blinding snow!
          mechoaround %person% %person.name% is gone!
          set random %random.100%
          set zone 554
      done
      eval place ((%zone% * 100) + 99 + %random%)
      mteleport %person% %place%
      mforce %person% look
    endif
  endif
  set person %person.next_in_room%
done
~
#36415
berserker_target_death~
0 f 100
~
if %actor.quest_stage[berserker_subclass]% == 4 && %actor.quest_variable[berserker_subclass:target]% == %self.vnum%
  msend %actor% &6&bCongratulations, you have succeeded in your Wild Hunt!&0
  msend %actor% &6&bYou have earned the right to become a &9&bBer&1ser&9ker&6&b!&0
  msend %actor% Type '&3&bsubclass&0' to proceed.
  quest complete berserker_subclass %actor.name%
endif
~
#36416
berserker_bear_death~
0 f 100
~
if %actor.quest_stage[berserker_subclass]% == 4 && %actor.quest_variable[berserker_subclass:target]% == %self.vnum%
    msend %actor% &6&bCongratulations, you have succeeded in your Wild Hunt!&0
    msend %actor% &6&bYou have earned the right to become a &9&bBer&1ser&9ker&6&b!&0
    msend %actor% Type '&3&bsubclass&0' to proceed.
    quest complete berserker_subclass %actor.name%
endif
*
* Death trigger for random gem and armor drops
*
* set a random number to determine if a drop will
* happen.
*
* Miniboss setup
*
set bonus %random.100%
set will_drop %random.100%
* 3 pieces of armor per sub_phase in phase_1
set what_armor_drop %random.3%
* 4 classes questing in phase_1
set what_gem_drop %random.4%
*
if %will_drop% <= 30
   * drop nothing and bail
   halt
endif
if %will_drop% <= 70
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop a gem from the previous wear pos set
      eval gem_vnum %what_gem_drop% + 55573
      mload obj %gem_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem from the current wear pos set
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a gem from the next wear pos set
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
   endif
elseif (%will_drop% >= 71 &%will_drop% <= 90)
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop destroyed armor 55299 is the vnum before the
      * first piece of armor.
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop armor from the current wear pos set
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop a piece of armor from next wear pos
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   endif
else
   * Normal non-bonus drops
   if %bonus% <= 50
      * drop armor and gem from previous wear pos
      eval gem_vnum %what_gem_drop% + 55573
      eval armor_vnum %what_armor_drop% + 55307
      mload obj %gem_vnum%
      mload obj %armor_vnum%
   elseif (%bonus% >= 51 &%bonus% <= 90)
   * We're in the Normal drops from current wear pos set
      * drop a gem and armor from the current wear pos set
      eval armor_vnum %what_armor_drop% + 55311
      mload obj %armor_vnum%
      eval gem_vnum %what_gem_drop% + 55577
      mload obj %gem_vnum%
   else
   * We're in the BONUS ROUND!!
      * drop armor and gem from next wear pos
      eval gem_vnum %what_gem_drop% + 55581
      mload obj %gem_vnum%
      eval armor_vnum %what_armor_drop% + 55315
      mload obj %armor_vnum%
   endif
endif
~
#36417
**UNUSED**~
2 c 100
d di~
return 0
~
#36418
**UNUSED**~
0 c 100
h ho how~
return 0
~
#36419
illusory_wall_new_lenses_replacement~
0 d 0
I need new glasses~
wait 2
if %actor.quest_stage[illusory_wall]% > 1
  say That's unfortunate!  You'll have to make new ones.
  wait 2s
  say Bring me:
  mecho - &3&bA pair of glasses&0 or &7&bsmall spectacles&0 to look through
  mecho - &3&bA prismatic leg spur&0 to refract light properly
  mecho - &3&bA small piece of petrified magic&0 to enhance the magical sight of the lenses
  wait 4s
  say Once you do, you can resume your studies.
  quest restart illusory_wall %actor.name%
  quest variable illusory_wall %actor.name% 10307 0
  quest variable illusory_wall %actor.name% 18511 0
  quest variable illusory_wall %actor.name% 41005 0
  quest variable illusory_wall %actor.name% 51017 0
endif
~
#36420
berserker subclass progress checker~
0 d 0
subclass progress~
wait 2
switch %actor.quest_stage[berserker_subclass]%
  case 1
    msend %actor% %self.name% says, 'There are a few shared rites that bind us together.  None is more revered than the &6&bWild Hunt&0.'
    break
  case 2
    msend %actor% %self.name% says, 'Let us challenge the Spirits for the right to prove ourselves!  If they deem you worthy, the Spirits send you a vision of a mighty &6&bbeast&0.'
    break
  case 3
    msend %actor% %self.name% says, '&1&bHowl&0 to the spirits and make your song known!'
    break
  case 4
    switch %actor.quest_variable[berserker_subclass:target]%
      case 16105
        set target 16105
        set place a desert cave
        break
      case 16310
        set target 16310
        set place some forested highlands
        break
      case 20311
        set target 20311
        set place a vast plain
        break
      case 55220
        set target 55220
        set place the frozen tundra
        break
    done
    msend %actor% The Spirits reveal to you a vision of %get.mob_shortdesc[%target%]%!
    msend %actor% You see it is in %place%!
    break
  default
    if %actor.class% /= Warrior 
      switch %actor.race%
*        case ADD RESTRICTED RACES HERE
*          set classquest no
          break
        default
          if %actor.level% >= 10 && %actor.level% <= 25
            msend %actor% %self.name% says, 'You aren't trying to join us yet!'
          elseif %actor.level% < 10
            msend %actor% %self.name% says, 'You aren't ready to join us yet!'
          else
            set classquest no
          endif
      done
    else
      set classquest no
    endif
    if %classquest% == no
      msend %actor% %self.name% says, 'You won't be able to become a berserker, I'm afraid.'
    endif
done
~
$~
