#4900
TD WR Init~
2 d 0
TDCommand Start~
* Team Domination War Room Init (Speech) Trigger
set teams 4
set team0 Mielikki's Champions
set abbr0 MC
set team1 Rangers of the North
set abbr1 RN
set team2 The Invasion Army
set abbr2 IA
set team3 The Miner's Guild
set abbr3 MG
global teams
set i 0
while %i% < %teams%
  global team%i%
  global abbr%i%
  eval i %i% + 1
done
set pylons 10
global pylons
set i 0
while %i% < %pylons%
  set pylon%i% -1
  global pylon%i%
  eval i %i% + 1
done
set pylonname Caelian Pylon
global pylonname
if %actor.mexists[4900]% < 1
  wload mob 4900
endif
wecho Team Domination War Room variables initialized to defaults.
~
#4901
TD WR Reset~
2 d 0
TDCommand Purge~
* Team Domination War Room Reset (Speech) Trigger
if %teams%
  set i 0
  while %i% < %teams%
    unset team%i%
    unset abbr%i%
    eval i %i% + 1
  done
  unset teams
endif
if %pylons%
  set i 0
  while %i% < %pylons%
    unset pylon%i%
    eval i %i% + 1
  done
  unset pylons
endif
unset pylonname
if %actor.mexists[4900]% > 0
  wpurge teamdominationmc
endif
wecho Team Domination War Room variables reset.
~
#4902
TD WR Capture~
2 d 0
TDCommand Capture~
* Team Domination War Room Capture (Speech) Trigger
set i 0
while %i% < %pylons%
  if %speech% /= P%i%P
    set j 0
    while %j% < %teams%
      if %speech% /= T%j%T
        set pylon%i% %j%
        global pylon%i%
        switch %j%
          case 0
            set team %team0%
            break
          case 1
            set team %team1%
            break
          case 2
            set team %team2%
            break
          case 3
            set team %team3%
            break
        done
        eval num %i% + 1
        switch %num%
          case 1
            set suffix st
            break
          case 2
            set suffix nd
            break
          case 3
            set suffix rd
            break
          default
            set suffix th
        done
        wforce teamdominationmc gossip %team% captures the %num%%suffix% %pylonname%!
        wecho Team Domination pylon %i% captured by team %j%.
        halt
      endif
      eval j %j% + 1
    done
    if %j% >= %teams%
      log TD Error: Bad team identifier to WR Capture trigger
    endif
    halt
  endif
  eval i %i% + 1
done
~
#4903
TD AB Normalizer~
1 c 1
ca~
* Team Domination Armband Capture Normalization (Command) Trigger
return 0
~
#4904
TD AB Capture~
1 c 1
capture~
* Team Domination Armband Capture (Command) Trigger
if T%team%T != TT
  oforce %actor% xcapture T%team%T
  return 1
else
  return 0
endif
~
#4905
TD PY Init~
1 h 100
~
* Team Domination Pylon Init (Drop) Trigger
set teams 4
global teams
set owner -1
global owner
set pylonname Caelian Pylon
global pylonname
set pylon 0
global pylon
~
#4906
TD PY Normalize~
1 c 4
xcaptur~
return 0
~
#4907
TD PY Capture~
1 c 4
xcapture~
* Team Domination Pylon Capture (Command) Trigger
if !%arg%
  return 0
  halt
endif
set i 0
while %i% < %teams%
  if %arg% /= T %i%T
    if %candidate% == %i%
      eval seconds %timeout% * 12
      osend %actor% Your team will capture this %pylonname% in %seconds% seconds!
    elseif %owner% == %i%
      if %candidate%
        osend %actor% You touch the %pylonname%, canceling team %candidate%'s attempt to capture your %pylonname%!
        oechoaround %actor% %actor.name% touches the %pylonname%, disrupting its pulsing.
        oforce teamdominationmc say TDCommand Cancel T%i%T P%pylon%P
        unset candidate
      else
        osend %actor% But your team already controls this %pylonname%!
      endif
    else
      set timeout 4
      global timeout
      set candidate %i%
      global candidate
      eval seconds %timeout% * 12
      osend %actor% You touch the %pylonname%, and it starts pulsating.
      osend %actor% Your team will capture this %pylonname% in %seconds% seconds!
      oechoaround %actor% %actor.name% touches the %pylonname%, and it starts pulsating.
      oforce teamdominationmc say TDCommand Countdown T%candidate%T P%pylon%P
    endif
    return 1
    halt
  endif
  eval i %i% + 1
done
return 0
~
#4908
TD PY Countdown~
1 b 100
~
* Team Domination Pylon Countdown (Random) Trigger
if %timeout%
  eval timeout %timeout% - 1
  global timeout
  if %timeout% < 1
    oecho The %pylonname% glows brightly, erupting in light!
    oforce teamdominationmc say TDCommand Capture T%candidate%T P%pylon%P
    set owner %candidate%
    global owner
    unset candidate
    unset timeout
  endif
endif
~
#4909
TD WR Countdown~
2 d 0
TDCommand Countdown~
* Team Domination War Room Countdown (Speech) Trigger
set i 0
while %i% < %pylons%
  if %speech% /= P%i%P
    set j 0
    while %j% < %teams%
      if %speech% /= T%j%T
        set pylon%i% %j%
        global pylon%i%
        switch %j%
          case 0
            set team %team0%
            break
          case 1
            set team %team1%
            break
          case 2
            set team %team2%
            break
          case 3
            set team %team3%
            break
        done
        eval num %i% + 1
        switch %num%
          case 1
            set suffix st
            break
          case 2
            set suffix nd
            break
          case 3
            set suffix rd
            break
          default
            set suffix th
        done
        wforce teamdominationmc gossip %team% infiltrates the %num%%suffix% %pylonname%!  1 minute to capture!
        wecho Team Domination countdown started for pylon %i%'s capture by team %j%.
        halt
      endif
      eval j %j% + 1
    done
    if %j% >= %teams%
      log TD Error: Bad team identifier to WR Countdown trigger
    endif
    halt
  endif
  eval i %i% + 1
done
if %i% >= %pylons%
  log TD Error: Bad pylon identifier to WR Countdown trigger
endif
~
#4910
TD WR Cancel~
2 d 0
TDCommand Cancel~
* Team Domination War Room Cancel (Speech) Trigger
set i 0
while %i% < %pylons%
  if %speech% /= P%i%P
    set j 0
    while %j% < %teams%
      if %speech% /= T%j%T
        set pylon%i% %j%
        global pylon%i%
        switch %j%
          case 0
            set team %team0%
            break
          case 1
            set team %team1%
            break
          case 2
            set team %team2%
            break
          case 3
            set team %team3%
            break
        done
        eval num %i% + 1
        wforce teamdominationmc gossip %team% disrupts the countdown for %pylonname% %num%'s capture!
        wecho Team Domination countdown cancelled for pylon %i%'s capture.
        halt
      endif
      eval j %j% + 1
    done
    if %j% >= %teams%
      log TD Error: Bad team identifier to WR Cancel trigger
    endif
    halt
  endif
  eval i %i% + 1
done
if %i% >= %pylons%
  log TD Error: Bad pylon identifier to WR Cancel trigger
endif
~
$~
