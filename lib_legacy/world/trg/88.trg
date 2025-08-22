#8800
thief_subclass_farmer_greeting~
0 g 100
~
if %actor.quest_variable[merc_ass_thi_subclass:subclass_name]% == thief
  if %actor.quest_stage[merc_ass_thi_subclass]% == 3 || %actor.quest_stage[merc_ass_thi_subclass]% == 4
    if %actor.can_be_seen% && %actor.hiddenness% < 1
      msend %actor% %self.name% notices you skulking about!
      msend %actor% %self.name% says, 'Who are you?!  You weren't invited here!'
      msend %actor% %self.name% shoos you off the farm!
      mteleport %actor% 8006
      wait 1
      mforce %actor% look
      quest fail merc_ass_thi_subclass %actor%
      msend %actor% &3&bYou have failed your quest!&0
      msend %actor% You'll have to go back to %get.mob_shortdesc[6050]% and start over!
    endif
  endif
endif
~
$~
