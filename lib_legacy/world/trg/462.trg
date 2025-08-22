#46200
Nukreth Spire start path1 human~
2 a 100
~
if %get.mob_count[46220] < 1 && %get.mob_count[46221] < 1 && %get.mob_count[46222] < 1 && %get.mob_count[46223] < 1
  wait 2s
  wzoneecho 462 With the chieftain's death the slaves begin to rise up!
  wzoneecho 462 Mad cackling tears through the caverns followed by shouts and cries.
  wait 1s
  wat 46278 wload mob 46220
  wat 46278 wecho A woman rises up in defiance!
  wat 46278 wforce captive shout Help me!!  Where is my husband?!
  wait 4s
  wat 46278 wload mob 46224
  wat 46278 wecho A gnoll spiritbreaker appears to stop her!
  wat 46278 wforce spiritbreaker shout Kill the slave!
  wait 4s
  wat 46278 wforce captive shout You'll never take me alive!
  wat 46278 wforce spiritbreaker kill captive
endif
~
#46201
Nukreth Spire start path2 kobold~
2 a 100
~
if %get.mob_count[46220] < 1 && %get.mob_count[46221] < 1 && %get.mob_count[46222] < 1 && %get.mob_count[46223] < 1
  wait 2s
  wzoneecho 462 With the chieftain's death the slaves begin to rise up!
  wzoneecho 462 Mad cackling tears through the caverns followed by shouts and cries.
  wait 1s
  wat 46278 wload mob 46221
  wat 46278 wecho A kobold rises up in defiance!
  wat 46278 wforce captive shout Help meeeeee!!
  wait 4s
  wat 46278 wload mob 46224
  wat 46278 wecho A gnoll spiritbreaker appears to stop her!
  wat 46278 wforce spiritbreaker shout Kill the slave!
  wait 4s
  wat 46278 wforce captive shout Nooo!  Save meee!!
  wat 46278 wforce spiritbreaker kill captive
endif
~
#46202
Nukreth Spire start path3 orc~
2 a 100
~
if %get.mob_count[46220] < 1 && %get.mob_count[46221] < 1 && %get.mob_count[46222] < 1 && %get.mob_count[46223] < 1
  wait 2s
  wzoneecho 462 With the chieftain's death the slaves begin to rise up!
  wzoneecho 462 Mad cackling tears through the caverns followed by shouts and cries.
  wait 1s
  wat 46278 wload mob 46222
  wat 46278 wecho An orc rises up in defiance!
  wat 46278 wforce captive shout I'll kill every last one of you!
  wait 4s
  wat 46278 wload mob 46224
  wat 46278 wecho A gnoll spiritbreaker appears to stop him!
  wat 46278 wforce spiritbreaker shout Kill the slave!
  wait 4s
  wat 46278 wforce captive shout You'll never take me alive!
  wat 46278 wforce spiritbreaker kill captive
endif
~
#46203
Nukreth Spire start path4 goblin~
2 a 100
~
if %get.mob_count[46220] < 1 && %get.mob_count[46221] < 1 && %get.mob_count[46222] < 1 && %get.mob_count[46223] < 1
  wait 2s
  wzoneecho 462 With the chieftain's death the slaves begin to rise up!
  wzoneecho 462 Mad cackling tears through the caverns followed by shouts and cries.
  wait 1s
  wat 46278 wload mob 46223
  wat 46278 wecho A goblin rises up in defiance!
  wat 46278 wforce captive shout Now's my chance!  Where's my treasure!?
  wait 4s
  wat 46278 wload mob 46224
  wat 46278 wecho A gnoll spiritbreaker appears to stop her!
  wat 46278 wforce spiritbreaker shout Kill the slave!
  wait 4s
  wat 46278 wforce captive shout Help meeeee!
  wat 46278 wforce spiritbreaker kill captive
endif
~
#46204
Nukreth Spire chieftain death~
0 f 100
~
if !%actor%
  set rnd %random.4%
endif
if %actor.vnum% == -1
  set person %actor%
  set i %person.group_size%
  if %i%
     set a 1
     while %i% >= %a%
        set person %actor.group_member[%a%]%
        if %person.room% == %self.room%
           if !%person.quest_stage[nukreth_spire]%
              quest start nukreth_spire %person%
           endif
        elseif %person%
          eval i %i% + 1
        endif
        eval a %a% + 1
     done
  elseif !%person.quest_stage[nukreth_spire]%
     quest start nukreth_spire %person%
  endif
  set rnd %random.4%
  if %actor.quest_variable[nukreth_spire:path%rnd%]% == 1
    while %actor.quest_variable[nukreth_spire:path%rnd%]% == 1
      set rnd %random.4%
    done
    set start %rnd%
  endif
endif
if %rnd% == 1
  m_run_room_trig 46200
elseif %rnd% == 2
  m_run_room_trig 46201
elseif %rnd% == 3
  m_run_room_trig 46202
elseif %rnd% == 4
  m_run_room_trig 46203
endif
~
#46205
Nukreth Spire path2 3 4 captive greet~
0 g 100
~
wait 2
if %actor.quest_stage[nukreth_spire]%
  if !%actor.quest_variable[nukreth_spire:path%number%]%
    if %self.room% == 46278
      * the spiritbreaker is still alive 
      if %get.mob_count[46224]% > 0
        set kill 1
      else
        * the spiritbreaker is not alive, but they need to find something first
        if !%task%
          set job 1
        else
          * the spiritbreaker is not alive, and they have found the thing they need
          set follow 1
        endif
      endif
    elseif %self.room% == 46265 || %self.room% == 46220 || %self.room% == 46205
      if %get.mob_count[46201]% > 0
        * the gnoll trackers are alive
        set kill 2
      else
        if !%task%
          * the trackers are dead, but they need to find something first
          set job 1
        else
          * the trackers are dead, and they have found the thing they need
          set follow 1
        endif
      endif
    endif
  endif
endif
if %kill% == 1
  if %self.vnum% == 46220
    say Help me kill this thing!
  elseif %self.vnum% == 46221
    say Save me from this monster!!
  elseif %self.vnum% == 46222
    say WE TAKE REVENGE!!!
  elseif %self.vnum% == 46223
    say Get this beast off me!!
  endif
elseif %kill% == 2
  if %self.vnum% == 46220
    say Help me kill these things!
  elseif %self.vnum% == 46221
    say Save me from these monsters!!
  elseif %self.vnum% == 46222
    say WE TAKE REVENGE!!!
  elseif %self.vnum% == 46223
    say Get these beasts off me!!
  endif
elseif %job% == 1
  say Wait, I can't leave yet!
  mecho  
  if %self.vnum% == 46220
    say Please, help me find my husband.
  elseif %self.vnum% == 46221
    say Please, help me save my baby!
  elseif %self.vnum% == 46222
    say Bring me my axe!
  elseif %self.vnum% == 46223
    say Help me find my treasure!
  endif
elseif %follow% == 1
  say Say 'follow me' and I'll follow you out! 
endif
~
#46206
Nukreth Spire captive follow me~
0 d 0
follow me~
wait 2
if %actor.quest_stage[nukreth_spire]%
  if !%actor.quest_variable[nukreth_spire:path%number%]%
    if %self.room% == 46278
      * the spiritbreaker is still alive 
      if %get.mob_count[46224]% > 0
        set kill 1
      else
        * the spiritbreaker is not alive, but they need to find something first
        if !%task%
          set job 1
        else
          * the spiritbreaker is not alive, and they have found the thing they need
          set follow 1
        endif
      endif
    elseif %self.room% == 46265 || %self.room% == 46220 || %self.room% == 46205
      if %get.mob_count[46201]% > 0
        * the gnoll trackers are alive
        set kill 2
      else
        if !%task%
          * the trackers are dead, but they need to find something first
          set job 1
        else
          * the trackers are dead, and they have found the thing they need
          set follow 1
        endif
      endif
    else
      set follow 1
    endif
  else
    msend %actor% &1&bYou have already completed this option.&0
  endif
else
  msend %actor% &1&bYou must first start this quest before you can earn rewards.&0
endif
if %kill% == 1
  if %self.vnum% == 46220
    say Help me kill this thing!
  elseif %self.vnum% == 46221
    say Save me from this monster!!
  elseif %self.vnum% == 46222
    say WE TAKE REVENGE!!!
  elseif %self.vnum% == 46223
    say Get this beast off me!!
  endif
elseif %kill% == 2
  if %self.vnum% == 46220
    say Help me kill these things!
  elseif %self.vnum% == 46221
    say Save me from these monsters!!
  elseif %self.vnum% == 46222
    say WE TAKE REVENGE!!!
  elseif %self.vnum% == 46223
    say Get these beasts off me!!
  endif
elseif %job% == 1
  say Wait I can't leave yet!
  mecho  
  if %self.vnum% == 46220
    say Please, help me find my husband.
  elseif %self.vnum% == 46221
    say Please, help me save my baby!
  elseif %self.vnum% == 46222
    say Bring me my axe!
  elseif %self.vnum% == 46223
    say Help me find my treasure!
  endif
elseif %follow% == 1
  if %self.vnum% == 46220
    say Lead us out!
  elseif %self.vnum% == 46221
    say You got it!
  elseif %self.vnum% == 46222
    say ONWARD!!
  elseif %self.vnum% == 46223
    say Yes, we go now!
  endif
  fol %actor%
  set leader %actor.name%
  global leader
endif
~
#46207
Nukreth Spire captive load~
0 o 100
~
if %self.vnum% == 46220
  set number 1
elseif %self.vnum% == 46221
  set number 2
elseif %self.vnum% == 46222
  set number 3
elseif %self.vnum% == 46223
  set number 4
endif
global number
~
#46208
Nukreth Spire spiritbreaker special death~
0 f 100
~
if %self.room% == 46278
  if %get.mob_count[46220]%
    m_run_room_trig 46209
  elseif %get.mob_count[46221]%
    m_run_room_trig 46210
  elseif %get.mob_count[46222]%
    m_run_room_trig 46211
  elseif %get.mob_count[46223]%
    m_run_room_trig 46212
  endif
endif
~
#46209
Nukreth Spire rescue path1 human~
2 a 100
~
wait 1s
wecho With the death of the gnoll spiritbreaker, the woman looks around intently.
wecho She says, 'I can't leave without my husband.  Please, help me find him!'
~
#46210
Nukreth Spire rescue path2 kobold~
2 a 100
~
wait 1s
wecho With the death of the gnoll spiritbreaker, the kobold looks up pleadingly.
wecho She says, 'I can't leave without my baby!  Please, help me!'
~
#46211
Nukreth Spire rescue path3 orc~
2 a 100
~
wait 1s
wecho With the death of the gnoll spiritbreaker, the orc looks around menacingly.
wecho He says, 'Find my axe!  We'll cut our way out together!!'
~
#46212
Nukreth Spire rescue path4 goblin~
2 a 100
~
wait 1s
wecho With the death of the gnoll spiritbreaker, the goblin looks around frantically.
wecho He says, 'I can't leave without my treasure!  Help me find it!'
~
#46213
Nukreth Spire human help speech~
0 d 100
husband help yes okay who where~
wait 2
if %actor.quest_stage[nukreth_spire]%
  if %actor.quest_variable[nukreth_spire:path1]% == 0
    mecho %self.name% says, 'They took us both captive.  But just a moment ago three
    mecho &0of their cultists took him as a sacrifice to their Demon Lord...'
    wait 2
    mecho Her voice trails off.
    wait 2s
    shake
    say No, he's still alive, I know it.  Please, save him!
    wait 1s
    mecho %self.name% says, 'They took him deeper into the den.  Find him and bring
    mecho &0him back here.  I'll keep this area safe.'
    if !%actor.quest_variable[nukreth_spire:rescue]%
      quest variable nukreth_spire %actor% rescue 1
    endif
    if !%running%
      set running yes
      global running
      if !%get.mob_count[46206]%
        mat 46262 mload mob 46206
      endif
      if !%get.mob_count[46207]%
        mat 46262 mload mob 46207
      endif
      if !%get.mob_count[46208]%
        mat 46262 mload mob 46208
      endif
    endif
  else
    msend %actor% &1&bYou have already completed this option.&0
  endif
else
  msend %actor% &1&bYou must first start this quest before you can earn rewards.&0
endif    
~
#46214
Nukreth Spire kobold help speech~
0 d 100
baby help yes okay what where~
wait 2
if %actor.quest_stage[nukreth_spire]%
  if %actor.quest_variable[nukreth_spire:path2]% == 0
    mecho %self.name% says, 'I hid an egg in the corner animal pen.  It should still
    mecho &0be there.'
    wait 1s
    mecho %self.name% says, 'Please find it and bring it back to me!  I'll hide here
    mecho &0while you go look.'
    wait 2
    mecho %self.name% hides herself out of sight.
    hide
    if !%actor.quest_variable[nukreth_spire:baby]%
      quest variable nukreth_spire %actor% baby 1
    endif
  else
    msend %actor% &1&bYou have already completed this quest path.&0
  endif
else
  msend %actor% &1&bYou must first start this quest before you can earn rewards.&0
endif   
~
#46215
Nukreth Spire orc help speech~
0 d 100
axe help yes okay where~
wait 2
if %actor.quest_stage[nukreth_spire]%
  if %actor.quest_variable[nukreth_spire:path3]% == 0
    nod
    mecho %self.name% says, 'They took my axe when they captured me.  Gave it to
    mecho &0one of the chieftain's favorite mates.'
    wait 2
    snarl
    wait 1s
    mecho %self.name% says, 'Was a gift from an order of dark monks.  Sure would
    mecho &0love it back.'
    wait 2
    say For some revenge...
    wait 1s
    say Go find it and bring it to me.  I'll hold this area.
    if !%running%
      set running yes
      global running
      mat 1100 mload mob 46205
      mat 1100 mforce mate mload obj 46213
      mat 1100 mforce mate wield axe
      mat 1100 mteleport mate 46240
    endif
  else
    msend %actor% &1&bYou have already completed this quest path.&0
  endif
else
  msend %actor% &1&bYou must first start this quest before you can earn rewards.&0
endif  
~
#46216
Nukreth Spire goblin help speech~
0 d 100
treasure help yes okay what where~
wait 2
if %actor.quest_stage[nukreth_spire]%
  if %actor.quest_variable[nukreth_spire:path4]% == 0
    mecho %self.name% says, 'Yessss, had me a shiny treasure I did.  Hid it on some
    mecho &0bodies before they could eats 'em.  Or me.'
    wait 1s
    say Should still be up there in the larder.
    wait 1s
    mecho %self.name% says, 'Please find it and bring it back to me!  I'll hide here
    mecho &0while you go look.'
    wait 2
    mecho %self.name% hides himself out of sight.
    hide
    if !%actor.quest_variable[nukreth_spire:treasure]%
      quest variable nukreth_spire %actor% treasure 1
    endif
  else
    msend %actor% &1&bYou have already completed this quest path.&0
  endif
else
  msend %actor% &1&bYou must first start this quest before you can earn rewards.&0
endif 
~
#46217
Nukreth Spire fangs greet rescuer~
2 i 100
~
wait 2
if %actor.quest_variable[nukreth_spire:rescue]%
  wecho The Fangs of Yeenoghu look up and cackle with delight!
  wecho They cry, 'Fresh meat for Yeenoghu!'
  wait 1s
  wecho A weak man laying at the feet of the statue raises his hand in desperation.
  wecho He whispers, 'Help me...!'
  quest variable nukreth_spire %actor% rescue 0
endif
~
#46218
Nukreth Spire fangs death~
0 f 100
~
m_run_room_trig 46234
~
#46219
Nukreth Spire sacrifice load~
0 o 100
~
wait 1s
mecho The man slowly climbs to his feet.
mecho %self.name% says, 'Did my wife send you?  Please, take me to her!
mecho &0Say 'follow me' and I'll go with you.'
~
#46220
Nukreth Spire sacrifice follow~
0 d 0
follow me~
wait 2
if %get.mob_count[46201]% == 0 && %get.mob_count[46224]% == 0
  if !%leader%
    fol %actor%
    say Lead on.
    set leader %actor.name%
    global leader
  else
    halt
  endif
else
  say Save us from these beasts first!
endif
~
#46221
Nukreth Spire egg search~
2 c 100
search~
switch %cmd%
  case s
    return 0
    halt
done
if %actor.quest_stage[nukreth_spire]%
  if %actor.quest_variable[nukreth_spire:path2]% == 0
    if %actor.quest_variable[nukreth_spire:baby]% == 1
      quest variable nukreth_spire %actor% baby 2
      wsend %actor% You begin searching...
      wait 1s
      wecho A sudden yip comes from outside the pen!
      wecho A gnoll beastmaster says, 'What's going on in there??'
      Wait 1s
      wecho The beastmaster and a dire hyena enter the pen and attack!
      wsend %actor% &3&bYou better kill these things and search again!&0
      wload mob 46226
      wforce beastmaster kill %actor%
      wload mob 46227
      wforce dire-hyena kill %actor%
    elseif %actor.quest_variable[nukreth_spire:baby]% == 4
      quest variable nukreth_spire %actor% baby 0
      wsend %actor% You find a speckled kobold egg hidden amongst the straw!
      wechoaround %actor% %actor.name% finds a speckled kobold egg hidden amongst the straw!
      wload obj 46214
    else
      return 0
    endif
  else
    return 0
  endif
else
  return 0
endif
~
#46222
Nukreth Spire beastmaster death~
0 f 100
~
set i %actor.group_size%
if %i%
  set a 1
  while %i% >= %a%
    set person %actor.group_member[%a%]%
    if %person.room% == %self.room%
      if %person.quest_variable[nukreth_spire:baby]% == 2
        quest variable nukreth_spire %person% baby 3
      elseif %person.quest_variable[nukreth_spire:baby]% == 3
        quest variable nukreth_spire %person% baby 4
      endif
    elseif %person% 
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
else
  if %actor.quest_variable[nukreth_spire:baby]% == 2
    quest variable nukreth_spire %actor% baby 3
  elseif %actor.quest_variable[nukreth_spire:baby]% == 3
    quest variable nukreth_spire %actor% baby 4
  endif
endif
~
#46223
Nukreth Spire larder search~
2 c 100
search~
switch %cmd%
  case s
    return 0
    halt
done
if %actor.quest_stage[nukreth_spire]%
  if %actor.quest_variable[nukreth_spire:path4]% == 0
    if %actor.quest_variable[nukreth_spire:treasure]% == 1
      wsend %actor% You begin searching...
      quest variable nukreth_spire %actor% treasure 2
      wait 1s
      wecho A sudden bark comes from 
      wecho A gnoll spiritbreaker says, 'What's going on in there??'
      wait 1s
      wecho The spiritbreaker roars and attacks!
      wsend %actor% &3&bYou better kill this thing and search again!&0
      wload mob 46224
      wforce spiritbreaker kill %actor%
    elseif %actor.quest_variable[nukreth_spire:treasure]% == 3
      quest variable nukreth_spire %actor% treasure 0
      wsend %actor% You find a strange stone hidden amongst the bodies!
      wechoaround %actor% %actor.name% finds a strange stone hidden amongst the bodies!
      wload obj 46215
    else
      return 0
    endif
  else
    return 0
  endif
else
  return 0
endif
~
#46224
Nukreth Spire spiritbreaker special death2~
0 f 100
~
set i %actor.group_size%
if %i%
  set a 1
  while %i% >= %a%
    set person %actor.group_member[%a%]%
    if %person.room% == %self.room%
      if %person.quest_variable[nukreth_spire:treasure]% == 2
        quest variable nukreth_spire %person% treasure 3
      endif
    elseif %person%
      eval i %i% + 1
    endif
    eval a %a% + 1
  done
else
  if %actor.quest_variable[nukreth_spire:treasure]% == 2
    quest variable nukreth_spire %actor% treasure 3
  endif
endif
~
#46225
Nukreth Spire kobold receive~
0 j 100
~
if %actor.quest_stage[nukreth_spire]%
  if %actor.quest_variable[nukreth_spire:path2]% == 0
    if %task% != done
      if %object.vnum% == 46214
        wait 2
        mjunk %object%
        mecho %self.name% trills excitedly!
        wait 1s
        say It's okay, momma's got you now.
        mecho %self.name% coos at the egg as she cradles it in her arms.
        wait 2s
        say Thank you!!  Now let's get out of here.
        fol %actor%
        set task done
        global task
        set leader %actor.name%
        global leader
      else
        return 0
        mecho %self.name% refuses %object.shortdesc%.
        shake
        say I only need my egg.
      endif
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      shake
      say I already have my egg.
    endif
  else
    return 0
    msend %actor% &1&bYou have already completed this quest path.&0
  endif
else
  return 0
  msend %actor% &1&bYou must first start this quest before you can earn rewards.&0
endif
~
#46226
Nukreth Spire orc receive~
0 j 100
~
if %actor.quest_stage[nukreth_spire]%
  if %actor.quest_variable[nukreth_spire:path3]% == 0
    if %task% != done
      if %object.vnum% == 46213
        wait 2
        mecho %self.name% grins wickedly as he wraps his fingers around the axe.
        wie axe
        wait 1s
        say Revenge will be sweet...
        wait 2s
        say Now move out!
        fol %actor%
        set task done
        global task
        set leader %actor.name%
        global leader
      else
        return 0
        mecho %self.name% refuses %object.shortdesc%.
        shake
        say I only need my axe.
      endif
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      shake
      say I already have my axe.
    endif
  else
    return 0
    msend %actor% &1&bYou have already completed this quest path.&0
  endif
else
  return 0
  msend %actor% &1&bYou must first start this quest before you can earn rewards.&0
endif
~
#46227
Nukreth Spire goblin receive~
0 j 100
~
if %actor.quest_stage[nukreth_spire]%
  if %actor.quest_variable[nukreth_spire:path4]% == 0
    if %task% != done
      if %object.vnum% == 46215
        wait 2
        mjunk %object%
        cheer
        say Come to papa!
        mecho %self.name% grins like a fool at the stone.
        wait 2s
        say Thanks matey.  Now get me outta 'ere.
        fol %actor%
        set task done
        global task
        set leader %actor.name%
        global leader
      else
        return 0
        mecho %self.name% refuses %object.shortdesc%.
        shake
        say I only want me stone.
      endif
    else
      return 0
      mecho %self.name% refuses %object.shortdesc%.
      shake
      say I already have me stone.
    endif
  else
    return 0
    msend %actor% &1&bYou have already completed this quest path.&0
  endif
else
  return 0
  msend %actor% &1&bYou must first start this quest before you can earn rewards.&0
endif
~
#46228
Nukreth Spire ambush~
2 i 100
~
if %actor.vnum% == 46220 || %actor.vnum% == 46221 || %actor.vnum% == 46222 || %actor.vnum% == 46223
  wait 2
  wforce captive follow me
  wecho Two gnoll trackers leap out of the shadows and attack!
  wecho A gnoll tracker says, 'Devour it before it escapes!!'
  wload mob 46201
  wload mob 46201
  wforce captive say Help me!!
endif
~
#46229
Nukreth Spire captive death~
0 f 100
~
if %self.vnum% == 46220 && %get.mob_count[46225]%
  mecho A limping slave tries to save his wife but is killed by the attack!
  mpurge limping-slave
elseif %self.vnum% == 46225 && %get.mob_count[46220]%
  mecho A Soltan peasant tries to save her husband but is killed by the attack!
  mpurge soltan-captive
endif
mat 46278 m_run_room_trig 46236
~
#46230
Nukreth Spire quest end~
0 i 100
~
wait 2
if %destination% == 46200
  quest variable nukreth_spire %leader% path%number% 1
  if %self.vnum% == 46220
    emote breathes a deep sigh of relief.
    mecho %self.name% says, 'I can't believe it...  I never thought we would see the
    mecho &0sky again.  And it's all thanks to you, %leader%.'
    wait 1s
    mecho %self.name% says, 'I don't know how we'll ever be able to repay you.'
    wait 1s
    say Please, take these.  It's all we have left.
    mload obj 536
    give sandals %leader%
    wait 2s
    say Now, we'll have to try to start over.  Come on honey.
    wait 1s
    mecho %self.name% and her husband make their way into the pine barrens headed for Solta.
    mpurge limping-slave
  elseif %self.vnum% == 46221
    emote cries tears of joy.
    mecho %self.name% says, 'I thought I was going to die in there.  But now I can
    mecho &0take my baby home and hatch it safely.'
    wait 1s
    say I don't know how I can thank you.
    wait 1s
    say Please, take this.  It's all I have.
    mload obj 46211
    give kobold %leader%
    wait 2
    mecho %self.name% scurries off into the pine barrens.
  elseif %self.vnum% == 46222
    emote bellows a mighty roar!!
    wait 1s
    mecho %self.name% says, 'I will be back, with all of Ogakh at my side, and we will
    mecho &0CRUSH these mongrels!'
    wait 2s
    say I would not have survived without you, %leader%.
    wait 1s
    say Take this.  Saved it from my time in the army.
    mload obj 46210
    give faceplate %leader%
    wait 2s
    say Good hunting.
    salute
    wait 2s
    mecho %self.name% marches into the pine barrens.
  elseif %self.vnum% == 46223
    shiver
    wait 1s
    mecho %self.name% says, 'Some things can never be unseen.  I'm a changed man, I
    mecho &0is.'
    wait 1s
    say 'Ere.  May it bring you more luck than it did me.'
    mload obj 46209
    give ioun %leader%
    wait 2s
    say I best be on me way.
    tip
    mecho %self.name% turns and scuttles off into the pine barrens.
  endif
  set path1 %leader.quest_variable[nukreth_spire:path1]%
  set path2 %leader.quest_variable[nukreth_spire:path2]%
  set path3 %leader.quest_variable[nukreth_spire:path3]%
  set path4 %leader.quest_variable[nukreth_spire:path4]%
  eval complete %path1% + %path2% + %path3% + %path4%
  if %complete% == 4
    set loop 1
    while %loop% < 5
      quest variable nukreth_spire %leader% path%loop% 0
      eval loop %loop% + 1
    done
  endif
  mpurge %self%
endif
~
#46231
Nukreth Spire beastmaster load~
0 o 100
~
set room %self.room%
mgoto 1100
mload mob 46213
mforce hyena follow beastmaster
mteleport hyena %room%
mload mob 46213
mforce hyena follow beastmaster
mteleport hyena %room%
mload mob 46215
mforce hyena follow beastmaster
mteleport hyena %room%
mgoto %room%
~
#46232
Nukreth Spire tracker death refollow~
0 f 100
~
if %get.mob_count[46201]% == 1
  m_run_room_trig 46233
endif
~
#46233
Nukreth Spire tracker death follow~
2 a 100
~
wait 4
if %get.mob_count[46201]% == 0
  wforce captive say Alright, who do I follow?
endif
~
#46234
Nukreth Spire load husband~
2 a 100
~
wait 2
if !%get.mob_count[46206]% && !%get.mob_count[46207]% && !%get.mob_count[46208]% && %get.mob_count[46220]%
  if !%get.mob_count[46225]%
    wload mob 46225
  endif
endif
~
#46235
Nukreth Spire path1 captive greet~
0 h 100
~
if %actor.vnum% == 46225 && %task% != done
  set task done
  global task
  wait 2
  mecho %self.name% rushes to her husband.
  mecho Choking back tears she says, 'I can't believe it!'
  wait 1s
  say How can I ever thank you?
  mecho She wraps her arms around her husband in a tight embrace.
  wait 1s
  say Now, who's leading us out of this hellhole?
  halt
elseif %actor.vnum% == -1
  if %actor.quest_stage[nukreth_spire]%
    if !%actor.quest_variable[nukreth_spire:path%number%]%
      if %self.room% == 46278
        * the spiritbreaker is still alive 
        if %get.mob_count[46224]% > 0
          set kill 1
        else
          * the spiritbreaker is not alive, but they need to find something first
          if !%task%
            set job 1
          else
            * the spiritbreaker is not alive, and they have found the thing they need
            set follow 1
          endif
        endif
      elseif %self.room% == 46265 || %self.room% == 46220 || %self.room% == 46205
        if %get.mob_count[46201]% > 0
          * the gnoll trackers are alive
          set kill 2
        else
          if !%task%
            * the trackers are dead, but they need to find something first
            set job 1
          else
            * the trackers are dead, and they have found the thing they need
            set follow 1
          endif
        endif
      endif
    endif
  endif
  if %kill% == 1
    msend %actor% %self.name% says, 'Help me kill this thing!'
  elseif %kill% == 2
    msend %actor% %self.name% says, 'Help me kill these things!'
  elseif %job% == 1
    msend %actor% %self.name% says, 'Wait, I can't leave yet!'
    mecho  
    msend %actor% %self.name% says, 'Please, help me find my husband.'
  elseif %follow% == 1
    msend %actor% %self.name% says, 'Say 'follow me' and I'll follow you out!'
  endif
endif
~
#46236
Nukreth Spire uprising failed~
2 a 100
~
wait 1s
wzoneecho 462 &1&bThe slave uprising has failed!&0
~
#46237
Nukreth Spire tracker load kill captive~
0 o 100
~
wait 2
kill captive
~
$~
