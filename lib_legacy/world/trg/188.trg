#18801
Teleport_to_clanhall~
1 c 1
sneak~
if (%arg% == out)
    if %actor.room% != 18800
        return 1
        oechoaround %actor% %actor.name% ducks quietly out of the room.
        osend %actor% Glancing around, you duck quietly out of the room.
        oteleport %actor% 18800
        oechoaround %actor% %actor.name% suddenly fades into existance, walking in from the east.
        oforce %actor% look
    else
        return 0
    endif
else
    return 0
endif
~
#18802
Observe_other_room~
2 c 100
observe~
*
* Observation trigger
* Player supplies an argument which evaluates to a vnum.  The player
* is then shown that room.
*
switch %arg%
    case inn
        set lookroom 3054
        break
    case board
        set lookroom 3002
        break
    case fountain
        set lookroom 3009
        break
    default
        wsend %actor% Observe &bwhere&0?
        halt
done
wsend %actor% You peer out the window into the world below.
wechoaround %actor% %actor.alias% peers out the window.
wteleport %actor% %lookroom%
wforce %actor% look
wteleport %actor% 18802
~
#18803
Block_entry~
2 g 100
~
*
* Block-Entry Trigger
* Checks to see if player is coming from the east and wearing the cloak.
* If not, doesn't allow entry.
*
if %direction% == east
    if %actor.level% > 99 || %actor.level% ==
    elseif %actor.worn[12]% == 18801
        wsend %actor% The TCD-GUARD squints at you and then nods and waves you in.
        wechoaround %actor% The TCD-GUARD squints at %actor.alias% and waves %actor.p% in.
        return 1
    else
        wsend %actor% The TCD-GUARD puts his hand out in front of you.
        wsend %actor% The TCD-GUARD says, 'Hmmph!  Think again, buddy.'
        wechoaround %actor% The TCD-GUARD puts his hand up, blocking %actor.alias% from entering the guild.
        return 0
    end
else
~
#18804
test_trigger~
2 g 100
~
set var 50
wecho var is %var%
if (%var% >= 1)
    wecho var more than 0
    if (%var <= 99)
        wecho var less than 100
    endif
endif
~
#18820
xcast_xcast~
1 c 3
xcast~
*
* X-cast
* This trigger works with 18821, x-decide, to cast custom spells with various
* effects.  It may only be used by mobiles in the 18820 to 18842 range,
* Laoris' dragonquest mobiles.
*
* Expected variables set by x-decide:
*  xid - unique integer identifier, like 1
*  xname - name of the spell, such as 'arctic blast'
*  xstars - duration to cast the spell, ie., 2
*  xmagic - magic words uttered at the end of the spell, like 'zimblo argis'
*  xeffect - actual effect of the spell, such as 'damage' or 'expel'
*  xmode - sets whether it's victim or self-only
*  xamount - only meaningful for some spell types, ie., 100
*
if ((%actor.vnum% >= 18820) && (%actor.vnum% <= 18842))
  return 1
  * Must use x-decide to select a spell first
  if (!%xname%)
    osend %actor% You have not chosen a spell to x-cast yet!
  * No x-casting damage on self, immortals, or people in other rooms
  elseif ((%xmode% == victim) && (%arg.name% == %actor.name%))
    osend %actor% You cannot x-cast at yourself.
  elseif ((%xmode% == victim) && (%arg.room% != %actor.room%))
    osend %actor% You must be in the same room as %arg.name%!
  elseif ((%xmode% == victim) && (%arg.level% > 99))
    osend %actor% You cannot x-cast at immortals!
  else
    if %xmode% != victim
      set notarg 1
    endif
    * Begin casting messages
    if (%xmode% == victim)
      osend %arg% %actor.name% starts casting &3&b'%xname%'&0 at &1&bYou&0!!!...
      oechoaround %arg% %actor.name% starts casting &3&b'%xname%'&0...
      osend %actor% You start casting &3&b'%xname%'&0...
    elseif (%xmode% == self)
      oechoaround %actor% %actor.name% starts casting &3&b'%xname%'&0...
    endif
    osend %actor% You start chanting...
    wait 2s
    * Imitate casting stars
    set stars %xstars%
    while %stars% > 0
      if %notarg% || (%arg.room% == %actor.room%)
        osend %actor% %stars% remaining on %xname%
        wait 2s
        eval stars %stars% - 1
      else
        set spell_stop 1
        set stars 0
      endif
    done
    * If player is still here, make with the magic
    if (%spell_stop% != 1)
      if %notarg% || (%arg.room% == %actor.room%)
        if (%xmode% == victim)
          osend %arg% %actor.name% stares at you and utters the words, '%xmagic%'.
          oechoaround %arg% %actor.name% closes %actor.p% eyes and utters the words, '%xmagic%'.
        elseif (%xmode% == self)
          oechoaround %actor% %actor.name% closes %actor.p% eyes and utters the words, '%xmagic%'.
        endif
        osend %actor% You complete your spell.
        if (%xeffect% == damage)
          eval damage %xamount% + %random.50%
        endif
        * Show the message
        if %xid% == 1
          oechoaround %actor% &3%actor.name% lets out a &1soul-rending&3 screech&0!
          osend %actor% &3You let out shattering screech!&0
        elseif %xid% == 2
          oechoaround %actor% &7&b%actor.name% waves a taloned paw, bring down a &0&6bli&bzz&0&6ard&7&b of snow!&0
          osend %actor% &7&bYou wave a taloned paw at the room, bringing down a &0&6bli&bzz&0&6ard&7&b of snow!&0
        elseif %xid% == 3
          oechoaround %actor% &7&b%actor.name% looks much healthier than before!&0
          osend %actor% &7&bYou feel MUCH healthier than before!&0
        elseif %xid% == 4
          oechoaround %arg% %actor.name% waves a hand at %arg.name%, whisking %arg.o% away!
          osend %actor% You wave a hand at %arg.name%, whisking %arg.o% away!
        elseif %xid% == 5
          oechoaround %actor% &7&b%actor.name% speaks a holy word of sanctification!&0
          osend %actor% &7&bYou speak a holy word of sanctification!&0
        elseif %xid% == 6
          osend %arg% &9&b%actor.name% chants an arcane word at you, locking you in an iron maiden!&0
          oechoaround %arg% &9&b%actor.name% chants an arcane word at %arg.name%, locking %arg.o% in an iron maiden!&0
          osend %actor% &9&bYou chant an arcane word at %arg.name%, locking %arg.o% in an iron maiden!&0
        elseif %xid% == 7
          oechoaround %actor% &4Lightning streaks out from %actor.name%'s body, sizzling through the air.&0
          osend %actor% &4Lightning streaks out from your body, crackling through the air.&0
        elseif %xid% == 8
          osend %arg% %actor.name% screams a commination at you, causing runes to appear on your skin!  (&1&b%damage%&0)
          oechoaround %arg% Runes appear on %arg.name% as %actor.name% screams an obloquy at %arg.o%!  (&4%damage%&0)
          osend %actor% You scream a curse at %arg.name%, causing runes to appear on %arg.p% skin! (&3%damage%&0)
        elseif %xid% == 9
          osend %arg% %actor.name% flings a ball of caustic acid at you, burning your flesh. (&1&b%damage%&0)
          oechoaround %arg% %actor.name% flings a ball of caustic acid at %arg.name%, burning %actor.p% flesh.  (&4%damage%&0)
          osend %actor% You throw a ball of caustic acid at %arg.name%, burning %arg.p% flesh. (&3%damage%&0)
        else
          oechoaround %actor% X-cast failure.  Nothing happens.
          osend %actor% Unrecognized xid number.
        endif
        * Do the deed
        if (%xeffect% == damage)
          odamage %arg% %damage%
        elseif (%xeffect% == heal)
          oheal %actor% %xamount%
        endif
        if (%xeffect% == transport)
          if (%xamount% == -1)
            osend %arg% %actor.name% waves a hand at you, whisking you away!
            set max_tries 20
            while %max_tries% > 0
              oteleport %arg% %random.60000%
              if %arg.room% != %actor.room%
                set max_tries 0
              endif
              eval max_tries %max_tries% - 1
            done
            if !%max_tries%
              oteleport %arg% 0
            endif
          else
            oteleport %arg% %xamount%
          endif
          oforce %arg% look
        elseif (%xeffect% == area)
          set max_tries 20
          while %max_tries% > 0
            set victim %random.char%
            if %victim%
              if (%victim.vnum% != %actor.vnum%) && !(%target_list% /= X%victim.name%X)
                set target_list %target_list%X%victim.name%X
                eval damage %xamount% + %random.50%
                if %xid% == 1
                  osend %victim% &3The screech pierces into your head, causing great pain!&0 (&1&b%damage%&0)
                  oechoaround %victim% &3%victim.name% grasps %victim.p% head in pain.&0 (&4%damage%&0)
                  odamage %victim% %damage%
                elseif %xid% == 2
                  osend %victim% &6The sudden blizzard &7&bchills&0&6 you to the bone!&0 (&1&b%damage%&0)
                  oechoaround %victim% &6%victim.name% shivers and turns blue in the blizzard.&0 (&4%damage%&0)
                  odamage %victim% %damage%
                elseif (%xid% == 5) && (%victim.align% <= -350)
                  osend %victim% &7&bYou cry out in pain upon hearing %actor.name%'s blessing!&0 (&1&b%damage%&0)
                  oechoaround %victim% &7&b%victim.name% grasps %victim.p% head upon hearing %actor.name%'s blessing!&0 (&4%damage%&0)
                  odamage %victim% %damage%
                elseif %xid% == 7
                  osend %victim% &4The lightning strikes you in the chest, shocking your body!&0 (&1&b%damage%&0)
                  oechoaround %victim% &4%victim.name% reels back, struck in the chest by the lightning!&0 (&4%damage%&0)
                  odamage %victim% %damage%                           
                endif
              endif
            else
              set max_tries 0
            endif
            eval max_tries %max_tries% - 1
          done
        endif
      else
        set spell_stop 1
      endif
    endif
    * If the player left somewhere along the way, fail spell
    if (%spell_stop% == 1)
      osend %actor% You stop invoking abruptly!
      oechoaround %actor% %actor.name% stops invoking abruptly!
    endif
  endif
else
  * Do nothing for non-dragonquest mobs
  return 0
endif
~
#18821
xcast_xdecide~
1 ac 3
xdecide~
*
* X-decide
* This trigger works with 18820, x-cast, to cast custom spells with various
* effects.  It may only be used by mobiles in the 18820 to 18842 range,
* Laoris' dragonquest mobiles.
*
* Expected variables set by x-decide:
*  xid - unique integer identifier, like 1
*  xname - name of the spell, such as 'arctic blast'
*  xstars - duration to cast the spell, ie., 2
*  xmagic - magic words uttered at the end of the spell, like 'zimblo argis'
*  xeffect - actual effect of the spell, such as 'damage' or 'expel'
*  xmode - sets whether it's victim or self-only
*  xamount - only meaningful for some spell types, ie., 100
*
if ((%actor.vnum% >= 18820) && (%actor.vnum% <= 18842))
   * Block 'Huh?!?' message
   return 1
   * Choose spell
   * These if statements allow the user to only type part of the spell
   * name, such as just 'arc' for 'arctic blast'.  However, this also
   * allows 'blast', so making another spell 'blast of fire' is a bad
   * idea.  Use unique spell names.
   if (deadly screech /= %arg%)
      set xid 1
      set xname deadly screech
      set xstars 0
      set xmagic shelak frhoonl
      set xeffect area
      set xmode self
      set xamount 300
   elseif (blizzard /= %arg%)
      set xid 2
      set xname blizzard
      set xstars 2
      set xmagic shelaki
      set xeffect area
      set xmode self
      set xamount 300
   elseif (reconstitution /= %arg%)
      set xid 3
      set xname reconstitution
      set xstars 2
      set xmagic mellagenipoir
      set xeffect heal
      set xmode self
      set xamount 1000
   elseif (hand of transport /= %arg%)
      set xid 4
      set xname hand of transport
      set xstars 2
      set xmagic franti ay sakchorish
      set xeffect transport
      set xmode victim
      set xamount -1
   elseif (defamation /= %arg%)
      set xid 5
      set xname defamation
      set xstars 2
      set xmagic rotulugeaf
      set xeffect area
      set xmode self
      set xamount 400
   elseif (iron maiden /= %arg%)
      set xid 6
      set xname iron maiden
      set xstars 1
      set xmagic grak oblithron
      set xeffect transport
      set xmode victim
      set xamount 18820
   elseif (bodily charge /= %arg%)
      set xid 7
      set xname bodily charge
      set xstars 3
      set xmagic corpeno elekar
      set xeffect area
      set xmode self
      set xamount 500
   elseif (archons curse /= %arg%)
      set xid 8
      set xname archons curse
      set xstars 2
      set xmagic colrio goladhr
      set xeffect damage
      set xmode victim
      set xamount 500
   elseif (caustic conflaguration /= %arg%)
      set xid 9
      set xname caustic conflaguration
      set xstars 2
      set xmagic akridsi donoeplarinius
      set xeffect damage
      set xmode victim
      set xamount 300
   else
      osend %actor% That is not a valid x-cast spell.
   endif
   if %xname%
      osend %actor% x-cast spell set to: %xname%
      global xname
   endif
   if %xid%
      global xid
   endif
   if %xstars%
      global xstars
   endif
   if %xmagic%
      global xmagic
   endif
   if %xeffect%
      global xeffect
   endif
   if %xmode%
      global xmode
   endif
   if %xamount%
      global xamount
   endif
else
    * Do nothing for non-dragonquest mobs
    return 0
endif
~
#18822
scepter_reform~
1 aj 100
~
if (%self.vnum% == 18822 && %actor.wearing[18823]%) || (%self.vnum% == 18823 && %actor.wearing[18822]%)
   wait 1
   oechoaround %actor% %actor.name% places the ruby in the mouth of the scepter's dragon.
   osend %actor% You place the ruby in the mouth of the dragon.
   oecho The ruby flares, and the scepter begins to glow &1red&0.
   wait 1s
   oechoaround %actor% %actor.name% yelps as the scepter burns %actor.o%, dropping it!
   osend %actor% The scepter burns you, and you drop it!
   if %self.vnum% == 18822
      opurge ruby-duclia
   elseif %self.vnum% == 18823
      opurge scepter-duclia
   endif
   oload obj 18824
   opurge %self%
endif
~
#18823
xcast_xlist~
1 c 3
xlist~
*
* X-list
* This trigger lists the spells available to x-decide and x-cast, triggers
* 18820 and 18821.  It may only be used by mobiles in the 18820 to 18842
* range, Laoris' dragonquest mobiles.
*
if ((%actor.vnum% >= 18820) && (%actor.vnum% <= 18842))
   return 1
   osend %actor% X-cast spells:           type       stars  amount
   osend %actor% &0 deadly screech          area       0      300
   osend %actor% &0 blizzard                area       2      300
   osend %actor% &0 reconstitution          heal       2      1000
   osend %actor% &0 hand of transport       transport  2
   osend %actor% &0 defamation              area       2      400
   osend %actor% &0 iron maiden             transport  1
   osend %actor% &0 bodily charge           area       3      500
   osend %actor% &0 archons curse           damage     2      500
   osend %actor% &0 caustic conflaguration  damage     2      300
else
   return 0
endif
~
#18824
**UNUSED**~
1 c 3
sha~
return 0
~
#18825
shapeshift~
1 c 3
shapeshift~
switch %cmd%
  case s
  case sh
  case sha
    return 0
    halt
done
switch %actor.vnum%
   case 18820
      set mob_vnum 18822
      set mob_name pryrian-duclia
      set shape_name the shape of a gargantuan black dragon
      set obj_vnum 18826
      break
   case 18821
      set mob_vnum 18823
      set mob_name eralshar-duclia
      set shape_name the shape of a sinewy golden dragon
      set obj_vnum 18827
      break
   case 18822
      set mob_vnum 18820
      set mob_name human-pryrian
      set shape_name human form
      set obj_vnum 18820
      break
   case 18823
      set mob_vnum 18821
      set mob_name human-eralshar
      set shape_name human form
      set obj_vnum 18825
      break 
   default
      return 0
      halt
done
oecho The cracks of bones reforming are heard as %actor.name% takes %shape_name%.
oload mob %mob_vnum%
oforce %mob_name% mload obj %obj_vnum%
osend %actor% Loaded %mob_name%; ready to be switched into.
opurge %actor%
opurge %self%
~
#18840
block_wield~
1 ac 1
wield~
switch %cmd%
  case w
    return 0
    halt
done
osend %actor% You cannot wield another weapon with %self.shortdesc%!
~
#18841
**UNUSED**~
1 ac 1
w~
return 0
~
#18842
talon_wear~
1 aj 100
~
if %actor.quest_variable[quest_items:%self.vnum%]%
  if !(%actor.worn[wield]%) && !(%actor.worn[wield2]%) && !(%actor.worn[2hwield]%)
    return 1
  else
    return 0
    osend %actor% You cannot wield another weapon with %self.shortdesc%!
  endif
else
  return 0
  osend %actor% You do not feel worthy enough to wield %self.shortdesc%!
endif
~
#18843
globe_drop~
1 ah 100
~
return 1
set owner %actor.name%
global owner
oechoaround %actor% %self.shortdesc% hovers away from %actor.name% slowly.
osend %actor% %self.shortdesc% slowly hovers away from you.
~
#18844
globe_get~
1 ag 100
~
if %actor.quest_variable[quest_items:%self.vnum%]%
   if %owner%
      if %owner% == %actor.name%
         if !(%actor.worn[held]%) && !(%actor.worn[wield]%) && !(%actor.worn[2hwield]%)
            set last_use %actor.quest_variable[quest_items:globe%self.vnum%_time]%
            set now %time.stamp%
            if %last_use%
               if %now% - %last_use% >= 1
                  set can_use yes
               else
                  return 1
                  oecho %self.shortdesc% flares brightly, but then fades.
               endif
            else
               set can_use yes
            endif
            if %can_use% == yes
               return 0
               oload obj %self.vnum%
               oforce %actor% get globe
               oforce %actor% hold globe
               oforce %actor% use globe
               quest variable quest_items %actor.name% globe%self.vnum%_time %now%
               opurge %self%
            endif
         else
            return 1
            osend %actor% You must have your primary hand free to activate %self.shortdesc%.
         endif
      else
         return 0
         osend %actor% %self.shortdesc%: you can't take that!
      endif
   else
      return 1
   endif
else
   return 0
   osend %actor% %self.shortdesc%: you can't take that!
endif
~
#18870
smart combat~
0 k 100
~
eval now ((((%time.year% * 16) + %time.month%) * 35) + %time.day%) * 24) + %time.hour%
set level %self.level%
set class %self.class%
set flags %self.flags%
eval is_sor %class% == Sorcerer
eval is_cry %class% == Cryomancer
eval is_pyr %class% == Pyromancer
eval is_nec %class% == Necromancer
eval is_arc %is_sor% || %is_cry% || %is_pyr% || %is_nec%
eval is_war %class% == Warrior
eval is_ran %class% == Ranger
eval is_pal %class% == Paladin
eval is_ant %class% /= Anti
eval is_mon %class% == Monk
eval is_com %is_ran% || %is_pal% || %is_ant%
eval is_fig %is_war% || %is_mon% || %is_com%
eval is_rog %class% == Rogue
eval is_thi %class% == Thief
eval is_ass %class% == Assassin
eval is_mer %class% == Mercenary
eval is_bac %is_rog% || %is_thi% || %is_ass% || %is_mer%
eval is_cle %class% == Cleric
eval is_pri %class% == Priest
eval is_dia %class% == Diabolist
eval is_dru %class% == Druid
eval is_div %is_cle% || %is_pri% || %is_dia% || %is_dru%
eval is_mag %is_arc% || %is_div% || %is_com%
eval cir_2 %level% >= 9
eval cir_3 %level% >= 17
eval cir_4 %level% >= 25
eval cir_5 %level% >= 33
eval cir_6 %level% >= 41
eval cir_7 %level% >= 49
eval cir_8 %level% >= 57
eval cir_9 %level% >= 65
eval cir_10 %level% >= 73
eval cir_11 %level% >= 81
eval cir_12 %level% >= 89
eval cir_13 %level% >= 97
wait 1s
set mode %random.10%
* Do fighter/rogue type stuff
if %is_fig% || %is_bac%
   if (%mode% < 4) && ((%is_ran% && (%level% > 34)) || (%is_war% && (%level% > 14)) || ((%is_ant% || %is_pal%) && (%level% > 9)))
      set max_tries 5
      while %max_tries > 0
         set victim %random.char%
         if (%victim.vnum% != -1) && (%victim.class% != Warrior) && (%victim.class% != Ranger) && (!(%victim.class% /= Anti)) && (%victim.class% != Paladin) && (%victim.class% != Monk)
            rescue %victim.name%
            set attempted 1
         endif
         eval max_tries %max_tries% - 1
      done
      if %attempted%
         halt
      endif
   endif
   if (%is_war% || %is_ran% || %is_pal% || %is_ant% || %is_mer%) && (%level% - %actor.level% >= 0) && (%self.worn[11]% != -1)
      bash
   elseif (%is_ass% || %is_thi% || (%is_rog% && (%level% > 9)) || (%is_mer% && (%level% > 10))) && (%self.worn[16]% != -1)
      backstab
   elseif (%is_war% && (%level% > 49)) || ((%is_pal% || %is_ant%) && (%level% > 79))
      hitall
   elseif ((%is_war% || %is_ran% || %is_pal% || %is_ant% || %is_mon% || %is_mer%) && (%level% >= 1)) || (%is_ass% && (%level% >= 36))
      kick
   endif
   halt
endif
* Initalize chance to do support spells
if !%defensive%
   set defensive 5
   global defensive
endif
* Attempt to cast support spells
if %mode% <= %defensive%
   if ((%is_nec% && %cir_7%) || (%is_arc% && (!%is_nec) && %cir_6%)) && (!(%flags% /= HASTE))
      cast 'haste'
   elseif ((%is_nec% && %cir_12%) || (%is_arc% && (!%is_nec) && %cir_6%)) && (!(%flags% /= STONE))
      cast 'stone skin'
   elseif ((%is_ran% && %cir_3%) || %is_dru%) && (%barkskin% + 6 + (%level% / 10) < %now%)
      cast 'barkskin'
      set barkskin %now%
      global barkskin
   elseif (%is_cle% || (%is_pal% && %cir_2%) || %is_pri%) && (%armor% + 10 < %now%)
      cast 'armor'
      set armor %now%
      global armor
   elseif (%is_dia% || (%is_ant% && %cir_2%)) && (%demonskin% + 10 + (%level% / 40) < %now%)
      cast 'demonskin'
      set demonskin %now%
      global demonskin
   elseif %is_dia% && %cir_4% && (%demonic% + 11 < %now%)
      cast 'demonic aspect'
      set demonic %now%
      global demonic
   elseif %is_pyr% && %cir_4% && (%mirage + 10 < %now%)
      cast 'mirage'
      set mirage %now%
      global mirage
   else
      set mode 6
   endif
endif
if %mode% > 5
   if (%is_sor% || %is_cry%) && %cir_8%
      cast 'chain lightning'
   elseif %is_pyr% && %cir_6%
      cast 'firestorm'
   elseif (%is_sor% || %is_cry%) && %cir_6%
      cast 'ice storm'
   elseif (%self.align% > 350) && ((%is_cle% && %cir_6%) || (%is_pri% && %cir_9%) || (%is_pal% && %cir_10%))
      cast 'holy word'
   elseif (%self.align% < -350) && ((%is_cle% && %cir_6%) || (%is_dia% && %cir_9%) || (%is_ant% && %cir_11%))
      cast 'unholy word'
   elseif %is_sor% && %cir_9% && %mode% <= 2
      cast 'disintegrate'
*   elseif %is_cry% && %cir_9%
*      cast 'iceball'
*   elseif %is_pyr% && %cir_9%
*      cast 'immolate'
   elseif %is_dru% && %cir_9% && ((!(%actor.flags% /= BLIND)) || !%outside%)
      cast 'sunray'
   elseif %is_sor% && %cir_7%
      cast 'bigbys clenched fist'
   elseif %outside% && %is_dru% && %cir_7%
      cast 'call lightning'
   elseif (%is_cle% && %cir_7%) || ((%is_pri% || %is_dia%) && %cir_10%)
      cast 'full harm'
   elseif %is_arc% && (!%is_nec%) && %cir_4% && (!(%actor.flags% /= ENFEEB))
      cast 'ray of enfeeblement'
   elseif %outdoors% && ((%is_dru% && %cir_4%) || (%is_div% && (!%is_dru%) && %cir_5%))
      cast 'earthquake'
   elseif %is_pyr% && %cir_7% && %mode% <= 2
      cast 'melt'
   elseif %is_dia% && %cir_7% && (!(%actor.flags% /= INSANITY))
      cast 'insanity'
   elseif %is_pri% && %cir_6% && (%actor.align% < 350)
      cast 'divine ray'
   elseif %is_dia% && %cir_6%
      cast 'stygian eruption'
   elseif %is_nec% && %cir_5%
      cast 'energy drain'
   elseif (%is_sor% || %is_cry%) && %cir_5%
      cast 'cone of cold'
   elseif (%is_cle% || %is_dru) && %cir_5%
      cast 'harm'
   elseif %is_pyr% && %cir_5%
      cast 'heatwave'
   elseif (%is_cle% || %is_pri%) && %cir_4% && (%actor.align% <= 350)
      cast 'dispel evil'
   elseif (%is_cle% || %is_dia%) && %cir_4% && (%actor.align% >= 350)
      cast 'dispel good'
   elseif ((%is_div% && (!%is_dru%) && %cir_4%) || (%is_ant% && %cir_6%)) && (!(%actor.flags% /= BLIND))
      cast 'blindness'
   elseif (%is_sor% && %cir_6%) || (%is_pyr% && %cir_4%)
      cast 'fireball'
   elseif %is_cle% && %cir_4% && (%self.align% >= 350)
      cast 'flamestrike'
   elseif (%is_arc% && (!%is_pyr%) && %cir_4%) || (%is_dru% && %cir_6%)
      cast 'lightning bolt'
   elseif %is_pri% && %cir_3% && (%self.align% >= 350) && (%actor.align% < 350)
      cast 'divine bolt'
   elseif %is_dia% && %cir_3% && (%actor.align% > 350)
      cast 'hell bolt'
   elseif %is_div% && (!%is_dru%) && %cir_3%
      cast 'cause critical'
   elseif %is_arc% && (!%is_pyr%) && %cir_3%
      cast 'shocking grasp'
   elseif %is_pyr% && %cir_3% && (!(%actor.flags% /= BLIND))
      cast 'smoke'
   elseif %is_dru% && %cir_3%
      cast 'writhing weeds'
   elseif %is_div% && (!%is_dru%) && %cir_2%
      cast 'cause serious'
   elseif %is_arc% && (!%is_pyr%) && %cir_2%
      cast 'chill touch'
   elseif %is_pyr% && %cir_2%
      cast 'fire darts'
   elseif %is_pyr%
      cast 'burning hands'
   elseif %is_cry%
      cast 'ice darts'
   elseif %is_arc%
      cast 'magic missile'
   elseif %is_div% && !%is_dru%
      cast 'cause light'
   endif
endif
set action %now%
global action
~
#18880
ctf_armband_tag~
1 c 1
tag~
**** Set vnum variables ****
* Mobiles
set referee 18880
* Objects
set vnum_a 18880
set vnum_b 18881
set flag_a 18882
set flag_b 18883
* Rooms
set zone_a_start 3500
set zone_a_end 3599
set jail_a 18880
set home_a 3500
set flag_room_a 3500
set zone_b_start 8000
set zone_b_end 8199
set jail_b 18881
set home_b 8025
set flag_room_b 8050
* Player has been forced to deactivate their armband
if (%arg% == DeactivateArmband)
    set inactive yes
    global inactive
* Player has been forced to reactivate their armband
elseif (%arg% == ReactivateArmband)
    set inactive no
    global inactive
* Player is wearing an inactive armband
elseif (%inactive% == yes)
    osend %actor% Sorry, you can't tag anyone right now!
* Player tries to tag self
elseif ((%arg% == self) || (%actor.name% == %arg.name%))
    osend %actor% There's really no point to that, now is there?
* Player tries to tag someone who isn't in room or doesn't exist
elseif (%arg.room% != %actor.room%)
    osend %actor% Tag whom?  They're not here!
* Player tries to tag immortal
elseif (%arg.level% > 99)
    osend %actor% It's not very nice to tag immortals.
* Player is holding the flag
elseif (%actor.wearing[%flag_a%]%) || (%actor.wearing[%flag_b%]%)
    osend %actor% You cannot tag someone while holding the flag!
* Player tries to tag player or referee mob
elseif ((%arg.vnum% == -1) || (%arg.vnum% == %referee%))
    * Player tries to tag someone on team A
    if (%arg.wearing[%vnum_a%]%)
        eval arg_team %vnum_a%
    * Player tries to tag someone on team B
    elseif (%arg.wearing[%vnum_b%]%)
        eval arg_team %vnum_b%
    * Player tries to tag someone who isn't playing
    else
        osend %actor% %arg.name% doesn't seem to be playing.
    endif
    * Player tries to tag someone who is playing
    if (%arg_team%)
        * Player is on team A
        if (%self.vnum% == %vnum_a%)
            eval actor_home %home_a%
            eval actor_jail %jail_a%
            eval actor_flag_room %flag_room_a%
            eval actor_flag %flag_a%
            * Player is in home zone
            if ((%actor.room% >= %zone_a_start%) && (%actor.room% <= %zone_a_end%))
                set at_home yes
            endif
        * Player is on team B
        elseif (%self.vnum% == %vnum_b%)
            eval actor_home %home_b%
            eval actor_jail %jail_b%
            eval actor_flag_room %flag_room_b%
            eval actor_flag %flag_b%
            * Player is in home zone
            if ((%actor.room% >= %zone_b_start%) && (%actor.room% <= %zone_b_end%))
                set at_home yes
            endif
        endif
        * Player tries to tag someone in jail
        if ((%actor.room% == %jail_a%) || (%actor.room% == %jail_b%))
            set in_jail yes
        endif
        * Player tries to tag someone on the same team
        if (%arg_team% == %self.vnum%)
            * Player tags teammate in jail (Return both to team home room)
            if (%in_jail% == yes)
                oteleport %arg% %actor_home%
                osend %arg% %actor.name% tags you, rescuing you from jail!
                oechoaround %arg% %actor.name% appears in a bright flash of light!
                oechoaround %arg% %arg.name% appears in a bright flash of light!
                oechoaround %actor% %actor.name% tags %arg.name%, rescuing %arg.o% from jail!
                osend %actor% You tag %arg.name%, rescuing %arg.o% from jail!
                oteleport %actor% %actor_home%
                oforce %arg% tag ReactivateArmband
                oforce %actor% look
                oforce %arg% look
            * Player tries to tag teammate in home zone
            elseif (%at_home% == yes)
                osend %actor% %arg.name% doesn't need saving in your own zone!
            * Player tries to tag teammate not in home zone, but not in jail
            else
                osend %actor% %arg.name% doesn't need saving right now!
            endif
        * Player tries to tag someone on an enemy team
        else
            * Player tries to tag enemy in jail
            if (%in_jail% == yes)
                * Player tries to tag enemy in player's jail
                if (%actor.room% == %actor_jail%)
                    osend %actor% %arg.name% is already in your jail!
                * Player tags enemy in enemy's jail (Send enemy to jail and player to home)
                else
                    oteleport %arg% %actor_jail%
                    osend %arg% %actor.name% tags you, banishing you for jail-guarding!
                    oechoaround %arg% %arg.name% appears in a bright flash of light!
                    oechoaround %actor% %actor.name% tags %arg.name%, banishing %arg.o% for jail-guarding!
                    osend %actor% You tag %arg.name%, banishing %arg.o% for jail-guarding!
                    oteleport %actor% %actor_home%
                    oechoaround %actor% %actor.name% appears in a bright flash of light!
                    oforce %arg% tag DeactivateArmband
                    oforce %actor% look
                    oforce %arg% look
                endif
            * Player tries to tag enemy in player's flag room
            elseif (%actor.room% == %actor_flag_room%)
                osend %actor% You can't tag %arg.name% in your flag room!
            * Player tags enemy in player's zone (Send enemy to player's jail)
            elseif (%at_home% == yes)
                osend %arg% %actor.name% tags you, sending you to %actor.o% jail!
                oteleport %arg% %actor_jail%
                oechoaround %arg% %arg.name% appears in a bright flash of light!
                oechoaround %actor% %actor.name% tags %arg.name%, sending %arg.o% to jail!
                osend %actor% You tag %arg.name%, sending %arg.o% to your jail!
                oforce %arg% tag DeactivateArmband
                oforce %arg% look
            * Player tries to tag enemy outside of player's zone
            else
                osend %actor% Tagging %arg.name% has no effect outside your zone!
            endif
            * Player tags enemy holding flag
            if (%arg.wearing[%flag_a%]) || (%arg.wearing[%flag_b%]%)
                set actor_room %actor.room%
                oforce %arg% remove ctf-flag
                oteleport %actor% %actor_flag_room%
                oload obj %actor_flag%
                oteleport %actor% %actor_room%
            endif
        endif
    endif
* Player tries to tag other mob
else
    osend %actor% You can only tag players!
endif
~
#18881
ctf_flag_get~
1 g 100
~
**** Set vnum variables ****
* Mobiles
set referee 18880
* Objects
set vnum_a 18880
set vnum_b 18881
set flag_a 18882
set flag_b 18883
* Player is on team A
if (%actor.wearing[18880]%)
    * Player is trying to pick up team A's flag
    if (%self.vnum% != %flag_a%)
        set enemy_flag yes
    endif
* Player is on team B
elseif (%actor.wearing[18881])
    * Player is trying to pick up team B's flag
    if (%self.vnum% != %flag_b%)
        set enemy_flag yes
    endif
endif
if (%enemy_flag% == yes)
    set hands 0
    if (%actor.worn[held]%)
        set hands 1
    endif
    if (%actor.worn[held2]%)
        eval hands %hands% + 1
    endif
    if (%actor.worn[wield]%)
        eval hands %hands% + 1
    endif
    if (%actor.worn[wield2]%)
        eval hands %hands% + 1
    endif
    if (%actor.worn[2hwield])
        eval hands %hands% + 1
    endif
    if (%actor.worn[shield]%)
        set hands 2
    endif
    if (%hands% < 2)
        return 1
        wait 1
        oforce %actor% hold flag
    else
        return 0
        osend %actor% You must have a free hand to pick up the flag!
    endif
else
    return 0
    osend %actor% You can't pick that up!
endif
~
#18882
ctf_flag_pass~
1 c 1
pass~
**** Set vnum variables ****
* Mobiles
set referee 18880
* Objects
set vnum_a 18880
set vnum_b 18881
* Rooms
set flag_room_a 3520
set flag_room_b 8600
* Player tries to pass to self
if ((%arg% == self) || (%actor% == %arg%))
    osend %actor% There's really no point to that, now is there?
* Player tries to pass to someone who isn't in room or doesn't exist
elseif (%arg.room% != %actor.room%)
    osend %actor% Pass the flag to whom?  They're not here!
* Player tries to pass to an immortal
elseif (%arg.level% > 99)
    osend %actor% You can't pass to an immortal!
* Player tries to pass to a player or the referee mob
elseif ((%arg.vnum% == -1) || (%arg.vnum% == %referee%))
    * Player tries to pass to someone on team A
    if (%arg.wearing[%vnum_a%]%)
        eval arg_team %team_a%
    * Player tries to pass to someone on team B
    elseif (%arg.wearing[%vnum_b%]%)
        eval arg_team %team_b%
    * Player tries to pass to someone who isn't playing
    else
        osend %actor% %arg.name% doesn't seem to be playing.
    endif
    * Player passes to someone who is playing
    if (%arg_team%)
        * Player is on team A
        if (%self.vnum% == %vnum_a%)
            eval actor_flag_room %flag_room_a%
        * Player is on team B
        elseif (%self.vnum% == %vnum_b%)
            eval actor_flag_room %flag_room_b%
        endif
        * Player passes to someone on the same team
        if (%arg_team% == %self.vnum%)
            osend %arg% %actor.name% quickly passes %self.shortdesc% to you.
            oteleport %arg% 0
            osend %actor% You slyly hand off %self.shortdesc% to %arg.name%.
            oechoaround %actor% %actor.name% quietly passes a flag to %arg.name%.
            oteleport %arg% %actor.room%
            oload obj %self.vnum%
            oforce %arg% get ctf-flag
            opurge %self%
        * Player passes to someone on an enemy team (Reset flag)
        else
            osend %actor% You accidentally pass the flag to %arg.name%, resetting it.
            oteleport %actor% %actor_flag_room%
            oload obj %self.vnum%
            oteleport %actor% %arg.room%
            opurge %self%
        endif
    endif
* Player tries to pass to other mob
else
    osend %actor% You can only pass the flag to players!
endif
~
#18883
test~
0 d 1
eval~
if XX%speech% /= XXeval
   eval %speech%
   mecho %eval%
endif
~
#18884
test trigger~
0 dm 100
10~
log test test this is a test
mecho %actor.class% %actor.race%
~
#18885
vampiric_bite~
1 c 1
bite~
switch %cmd%
  case b
    return 0
    halt
done
if %arg% == self || %arg.name% == %actor.name%
   return 0
elseif %arg% && %arg.room% == %actor.room%
   osend %actor% You bite %arg.name% on the neck!
   oteleport %actor% 1100
   oechoaround %arg% %actor.name% bites %arg.name% on the neck!
   oteleport %actor% %arg.room%
   osend %arg% %actor.name% bites you on the neck!
   return 1
else
   return 0
endif
~
#18886
**UNUSED**~
1 c 1
b~
return 0
~
#18887
**UNUSED**~
1 j 100
~
Nothing.
~
#18888
quest_item_binding~
1 j 100
~
* This trigger checks if a player is "registered" to wear certain quest eq.
* In order to be registered, a player must have the quest_items quest and a
* quest variable whose name is the vnum of the object this trigger is on
* and whose value is true (a simple 1 or yes will suffice).
*
* To register a player for equipment, first check to see if they already have
* the quest_item quest by typing "quest stage quest_items <player>" in their
* presence.  If they do not have it or it is failed, it will say failed.  In
* this case, attempt to start or restart quest_items on the player ("quest
* start quest_items <player>" and "quest restart quest_items <player>").  If
* the 'quest stage' command said anything besides failed, the player is ready!
*
* Once a player has the quest_items quest active, add a quest variable for
* the object.  For instance, if I wanted to allow Laoris to wear a golden
* dragonhelm, I would type:
*
*     quest variable quest_items laoris 18890 1
*
* If I no longer want Laoris to be able to wear the helm, I would type:
*
*     quest variable quest_items laoris 18890 0
*
if %actor.quest_variable[quest_items:%self.vnum%]%
    return 1
else
    return 0
    if %self.type% == WEAPON
        osend %actor% You do not feel worthy enough to wield %self.shortdesc%!
    else
        osend %actor% You do not feel worthy enough to wear %self.shortdesc%!
    endif
endif
~
#18889
DoRemortQuest~
0 d 0
Yes, I am sure I want to remort.~
if (%actor.level% == 99)
   msend %actor% Your remort will take effect in 30 seconds.
   mforce %actor% petition I am committing a REMORT!
   mforce %actor% petition After this completes, I will need my class reset to my base class.
   mforce %actor% petition The deleveling will begin in 30 seconds.
   mforce %actor% petition Kill the RemortQuestHandler before then to cancel the remort.
   wait 30s
   wizn Now starting!
   while (%actor.level% > 1)
      set count 99
      while ((%actor.level% > 1) && (%count% > 2))
         mexp %actor% -500000
         eval count %count% - 1
      done
   done
   mforce %actor% forget all
else
   msend %actor% You do not have enough experience to remort!
endif
mpurge %self%
~
#18890
**UNUSED**~
1 c 3
su~
return 0
~
#18891
summon_dragon~
1 c 3
summon~
switch %cmd%
  case s
  case su
    return 0
    halt
done
set last_summon %actor.quest_variable[quest_items:dragonhelm_time]%
eval now ((((%time.year% * 16) + %time.month%) * 35) + %time.day%) * 24) + %time.hour%
if %last_summon%
    if %now% - %last_summon% >= 168
        set can_summon yes
    else
        osend %actor% You may only summon one mount per week!
    endif
else
    set can_summon yes
endif
if %can_summon% == yes
    * The knight doesn't have a mount yet, allow them to get one
    if %actor.class% == Paladin
        * Knight is a paladin, give him/her a golden dragon
        oload mob 18890
        oechoaround %actor% A brilliant golden dragon flies in from nowhere, and nuzzles %actor.name%'s side.
        osend %actor% You begin calling for a mount..
        osend %actor% A brilliant golden dragon answers your summons.
        set summoned yes
    elseif %actor.class% /= Anti
        * Knight is an anti-paladin, give him/her a black dragon
        oload mob 18891
        oechoaround %actor% A dusky black dragon flies in, seemingly from nowhere, and sits by %actor.name%'s side.
        osend %actor% You begin calling for a mount..
        osend %actor% A dusky black dragon answers your summons.
        set summoned yes
    else
        osend %actor% You begin calling for a mount...but nothing happens.
        oechoaround %actor% %actor.name% whistles loudly.
    endif
    * Saddle up the dragon mount
    if %summoned%
        oforce dragon-mount follow %actor.name%
        oload mob 18892
        oforce dragonsquire mload obj 18892
        oforce dragonsquire give dragonsaddle dragon-mount
        oforce dragon-mount wear dragonsaddle
        opurge dragonsquire
        * Timestamp
        quest variable quest_items %actor.name% dragonhelm_time %now%
    endif
endif
~
#18892
dragonsaddle_melt~
1 gj 100
~
if ((%actor.vnum% == 18890) || (%actor.vnum% == 18891))
else
    return 0
    if %actor.canbeseen%
        osend %actor% As you take hold of %self.shortdesc%, it melts between your fingers.
        oechoaround %actor% As %actor.name% takes hold of %self.shortdesc%, it melts between %actor.p% fingers.
    else
        oecho %self.shortdesc% spontaneously combusts.
    endif
    opurge %self%
endif
~
#18893
dragonegg_hatch~
1 c 3
hatch~
switch %cmd%
  case h
  case ha
    return 0
    halt
done
if (%actor.class% == Paladin)
    set color golden
    set vnum 18890
elseif (%actor.class% /= Anti)
    set color black
    set vnum 18891
else
    osend %actor% You get the feeling that this egg is not meant for you.
endif
if %color%
    osend %actor% As you rub the egg, a crack begins to form...
    oechoaround %actor% As %actor.name% rubs %self.shortdesc%, a crack begins to form...
    wait 3s
    oecho The crack continues to grow, branching into hundreds of smaller cracks!
    wait 6s
    oecho A talon suddenly breaks through the egg shell!
    wait 3s
    oecho Slowly, the head of a %color% dragon pushes through the shell.
    wait 2s
    oecho The dragon egg continues to split into pieces.
    wait 2s
    oecho A small, %color% dragon emerges from the broken egg shell.
    oload mob %vnum%
    osend %actor% The %color% dragon looks at you.
    oechoaround %actor% The %color% dragon looks at %actor.name%.
    wait 3s
    oforce %color%-dragon bow
    if (%actor.sex% == Female)
        oforce %color%-dragon say Mistress, thank you for protecting me.
    else
        oforce %color%-dragon say Master, thank you for protecting me.
    endif
    oforce %color%-dragon say Henceforth shall I be at your command.  Merely call and I shall answer!
    wait 5s
    oforce %color%-dragon emote looks around itself.
    oecho In a rush of wind, the dragon beats its wings, launching itself into the air.
    opurge %color%-dragon
    wait 2s
    osend %actor% A strange-looking helmet falls from behind the dragon, landing near you.
    oechoaround %actor% A strange-looking helmet falls from behind the dragon, landing near %actor.name%.
    oload obj %vnum%
    if %actor.quest_stage[quest_items]% == 0
       quest start quest_items %actor.name%
    endif
    quest variable quest_items %actor.name% %vnum% 1
    oforce %actor% get dragonhelm
    opurge %self%
endif
~
#18894
**UNUSED**~
1 c 3
ha~
return 0
~
#18895
vampiric_no_remove~
1 l 100
bite~
osend %actor% The teeth have grown themselves into your jaws and won't budge!
return 0
~
#18896
**UNUSED**~
1 c 3
t~
return 0
~
#18897
fierytag_bat_capturetheflag~
1 c 3
tag~
switch %cmd%
  case t
  case ta
    return 0
    halt
done
set zone_a_start 3000
set zone_a_end 3199
set zone_a_jail 3009
set zone_a_home 3054
set zone_b_start 8000
set zone_b_end 8199
set zone_b_jail 8133
set zone_b_home 8049
set zone_c_start 3500
set zone_c_end 3699
set zone_c_jail 3514
set zone_c_home 3500
if ((%arg% == self) || (%actor.name% == %arg.name%))
    osend %actor% Now that seems a little pointless, doesn't it?
elseif (%actor.room% != %arg.room%)
    osend %actor% Tag who? They don't seem to be here!
elseif (%arg.vnum% != -1)
    osend %actor% You can only tag players!
elseif (%arg.level% > 99)
    osend %actor% You cannot tag immortals!
else
    if (%actor.sex% == Male)
        set actor_home %zone_a_home%
        set actor_jail %zone_a_jail%
        if (%actor.room% >= %zone_a_start%)
            if (%actor.room% <= %zone_a_end%)
                set actor_at_home yes
            endif
        endif
    elseif (%actor.sex% == Female)
        set actor_home %zone_b_home%
        set actor_jail %zone_b_jail%
        if (%actor.room% >= %zone_b_start%)
            if (%actor.room% <= %zone_b_end%)
                set actor_at_home yes
            endif
        endif
    elseif (%actor.sex% == Neutral)
        set actor_home %zone_c_home%
        set actor_jail %zone_c_jail%
        if (%actor.room% >= %zone_c_start%)
            if (%actor.room% <= %zone_c_end%)
                set actor_at_home yes
            endif
        endif
    endif
    if (%arg.sex% == %actor.sex%)
        set arg_same_team yes
    endif
    if ((%arg.room% == %zone_a_jail%) || (%arg.room% == %zone_b_jail%) || (%arg.room% == %zone_c_jail%))
        set arg_in_jail yes
    endif
    if (%actor.room% == %actor_jail%)
        if (%arg_same_team% == yes)
            * Tagging teammate in opponent jail, rescue them
            oteleport %arg% %actor_home%
            osend %arg% %actor.name% tags you, returning you to home!
            oechoaround %actor% %actor.name% tags %arg.name%, returning %arg.o% to home!
            osend %actor% You tag %arg.name%, returning %arg.o% to home!
            oteleport %actor% %actor_home%
            oforce %arg% look
            oforce %actor% look
        else
            osend %actor% That's no good.  %arg.name% is already in your jail.
        endif
    elseif (%actor_at_home% == yes)
        if (%arg_same_team% == yes)
            * Tagging teammate in home zone, do nothing
            osend %actor% But you're in your own zone!  %arg.name% is on your team!
        else
            * Tagging opponent in home zone, teleport opponent to jail
            oforce %arg% remove fiery-tag-bat
            oforce %arg% junk fiery-tag-bat
            oteleport %arg% %actor_jail%
            oechoaround %actor% %actor.name% tags %arg.name%!  To jail %arg.n% goes!
            osend %actor% You tag %arg.name%, sending %arg.o% to jail!
            osend %arg% %actor.name% tags you, sending you to jail!
            oforce %arg% look
        endif
    elseif (%arg_same_team% == yes)
        if (%arg_in_jail% == yes)
            * Tagging teammate in opponent jail, rescue them
            oload obj 18897
            oheal %arg% 10
            oforce %arg% wake
            oforce %arg% get fiery-tag-bat
            oteleport %arg% %actor_home%
            osend %arg% %actor.name% tags you, rescuing you from your imprisonment!
            oechoaround %actor% %actor.name% tags %arg.name%, returning both of them to their zone!
            osend %actor% You tag %arg.name%, rescuing %arg.o% from %arg.p% imprisonment!
            oteleport %actor% %actor_home%
            oforce %arg% look
            oforce %actor% look
        else
            * Tagging teammate in opponent zone, but not jail, do nothing
            osend %actor% %arg.name% doesn't need rescuing right now.
        endif
    else
        * Tagging opponent in opponent zone, do nothing
        osend %actor% No point in tagging %arg.o% if you're not in your zone.
    endif
endif
~
#18898
fierytag_bat_tag~
1 c 2
tag~
switch %cmd%
  case t
  case ta
    return 0
    halt
done
if ((%arg% == self) || (%actor.name% == %arg.name%))
    osend %actor% Now that seems a little pointless, doesn't it?
elseif (%actor.room% != %arg.room%)
    osend %actor% Tag who? They don't seem to be here!
elseif (%arg.vnum% != -1)
    osend %actor% You can only tag players!
elseif (%arg.level% > 99)
    osend %actor% You cannot tag immortals!
else
    oteleport %arg% 0
    oechoaround %actor% %actor.name% tags %arg.name% with a FieryTag bat!
    oteleport %arg% %actor.room%
    osend %actor% You tag %arg.name% with a whack of your FieryTag bat!
    osend %arg% Tag! %actor.name% tags you with a whack of %actor.p% FieryTag bat!
    oload mob 18896
    oforce fiery-tagger-announcer gossip %actor.name% has tagged %arg.name%!  %arg.name% is it!
    opurge fiery-tagger-announcer
    oload obj 18898
    oheal %arg% 10
    oforce %arg% wake
    oforce %arg% get bat
    opurge %self%
endif
~
#18899
fierytag_bat_shippotori~
1 c 1
tag~
switch %cmd%
  case t
  case ta
    return 0
    halt
done
if ((%arg% == self) || (%actor.name% == %arg.name%))
    osend %actor% Now that seems a little pointless, doesn't it?
elseif (%actor.room% != %arg.room%)
    osend %actor% Tag who? They don't seem to be here!
elseif (%arg.vnum% != -1)
    osend %actor% You can only tag players!
elseif (%arg.level% > 99)
    osend %actor% You cannot tag immortals!
elseif (%arg.wearing[18899]%)
    oteleport %arg% 0
    oechoaround %actor% %actor.name% tags %arg.name% with a FieryTag bat!
    oteleport %arg% %actor.room%
    osend %actor% You tag %arg.name% with a whack of your FieryTag bat!
    osend %arg% Tag! %actor.name% tags you with a whack of %actor.p% FieryTag bat!
    oload mob 18896
    oforce fiery-tagger-announcer gossip %actor.name% has tagged %arg.name%!  %arg.name% is out!
    opurge fiery-tagger-announcer
    oforce %arg% rem bat
else
    osend %actor% %arg.name% isn't in the game!
endif
~
$~
