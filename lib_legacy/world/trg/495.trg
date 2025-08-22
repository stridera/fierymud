#49500
Outside temple sounds~
2 g 100
~
wait 3s
 wsend %actor% Sounds of chains and grunts rasp outwards from the temple entrance.
~
#49501
Stairyway sound trig~
2 g 100
~
wait 2s
wsend %actor% The dark mist thickens as the shallow air thrust upwards from the deepening stairway.
~
#49502
Transfer trig, 49531~
2 c 0
west w~
if %actor.vnum% == -1
if %actor.class% /= Necromancer || Anti-Paladin || Thief || Assassin || Mercenary
if %actor.align% < -349
wsend %actor% Reality blurs and melts as you find yourself in a more dark and confined place.
wteleport %actor% 49533
else
wsend %actor% A cloak of shadow surrounds your being, choking your very breathe. Now you awake elsewhere.
wteleport %actor% 3002
endif
endif
endif
~
#49503
Auto door opener~
2 d 0
I am child of Borgan!~
if %actor.class% /= Necromancer
wdoor 49513 up room 49514
wecho With a terrifying crash the ceiling above falls downward, ceasing. A stairway leads upwards.
wait 20
wdoor 49513 u purge
endif
~
#49504
Transfer trig~
2 c 0
east e~
if %actor.vnum% == -1
if %actor.class% /= Priest || Cleric || Paladin || Ranger || Monk || Druid
if %actor.align% < +350
wsend %actor% You have no business here in the realm of shadow and death.
wsend %actor% You find yourself in a lighter place at peace.
wteleport %actor% 3002
endif
endif
~
$~
