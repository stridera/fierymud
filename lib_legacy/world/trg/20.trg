#2022
star_metal_neck_trans_to_odz~
1 j 100
~
if %actor.vnum% == -1
   wait 2
   oechoaround %actor% A strange little glow appears around %actor.name%, making you light-headed.
   osend %actor% A strange glow appears around you and you feel light headed.
   wait 2s
   oteleport %actor% 58002
   osend %actor% You blink and realize you are not where you started.
   osend %actor% You hear the crash of waves to the south.
   wait 2
   oforce %actor% look
endif
~
#2023
undefined~
1 c 100
~
Nothing.
~
$~
