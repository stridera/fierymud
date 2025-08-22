#1500
Entrance opens for members~
2 g 100
~
if %actor.wearing[1500]%
   if %wof_exit% != 1
      set wof_exit 1
      global wof_exit
      wait 4
      wechoaround %actor% %actor.name%'s &5Soul Gem&0 begins to glow.
      wsend %actor% Your &5Soul Gem&0 begins to glow.
      wecho A light shimmering develops to the east, and resolves itself into a portal.
      wdoor 39023 east purge
      wdoor 39023 east room 1500
      wdoor 39023 east description A slowly shimmering portal leads east.
      wait 8 s
      wecho The shimmering of the eastern exit is a bit faster now.
      wait 8 s
      wecho The eastern portal is positively spinning, and seems to be fading.
      wait 4 s
      wecho There is a sharp *snap* and the portal collapses into nothingness.
      wdoor 39023 east purge
      wdoor 39023 east room 39022
      wdoor 39023 east description The Blue Fog Sea rolls on to the East.
      set wof_exit 0
      global wof_exit
   end
end
~
#1501
Entry prevention: members only~
2 g 100
~
if %direction% == west
    if %actor.level% > 99 || %actor.wearing[1500]%
    else
        wsend %actor% You pass through the portal without seeming to go anywhere.
        wechoaround %actor% %actor.name% walks into the portal, but stays among the waves.
        return 0
    endif
endif
~
#1502
Allow command "west" to bypass command trigger~
2 c 100
we~
return 0
~
#1503
Wear membership ring and portal opens~
2 c 100
wear~
return 0
wait 2
if %actor.wearing[1500]%
   if %wof_exit% != 1
      set wof_exit 1
      global wof_exit
      wait 4
      wechoaround %actor% %actor.name%'s &5Soul Gem&0 begins to glow.
      wsend %actor% Your &5Soul Gem&0 begins to glow.
      wecho A light shimmering develops to the east, and resolves itself into a portal.
      wdoor 39023 east purge
      wdoor 39023 east room 1500
      wdoor 39023 east description A slowly shimmering portal leads east.
      wait 8 s
      wecho The shimmering of the eastern exit is a bit faster now.
      wait 8 s
      wecho The eastern portal is positively spinning, and seems to be fading.
      wait 4 s
      wecho There is a sharp *snap* and the portal collapses into nothingness.
      wdoor 39023 east purge
      wdoor 39023 east room 39022
      wdoor 39023 east description The Blue Fog Sea rolls on to the East.
      set wof_exit 0
      global wof_exit
   end
end
~
#1504
wof-hall-exit~
2 d 0
exit~
if %actor.vnum% == -1
 wait 1s
 wecho The &5runes&0 fly off the walls and float to the center of the room.
 wait 2s
 wsend %actor% &7&bYou feel your essence melting away into nothingness and reforming...&0
 wechoaround %actor% &7&b%actor.name% seems to melt away!&0
 wteleport %actor% 3009
 wechoaround %actor% &7&b%actor.name% appears out of nowhere!&0
 wat 3009 wforce %actor% look
end
~
$~
