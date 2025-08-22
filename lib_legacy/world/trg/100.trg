#10000
drunk_ickle_stunts~
0 b 20
~
set antic %random.3%
switch %antic%
  case 1
    shiver
    emote tries to huddle in the snow for warmth.
    hiccup
    say Need a little nip to warm the bones...
    break
  case 2
    glare
    say What are you looking at?
    emote puts up his fists.
    say Do you want some?  Do ya?
    fart
    break
  default
    drink drunkdrink
    burp
    emote wipes his mouth.
    say Aaaahhhh thas the good stuff...
done
~
#10075
Ickle East Guard~
0 c 100
east~
if %actor.vnum% == -1
  if %actor.level% < 20
     whisper %actor.name% You are much too little to venture east of here.
     wait 1
     whisper %actor.name% Try other areas first.
     nudge %actor.name%
  else
     return 0
  endif
else
   return 0
endif
~
$~
