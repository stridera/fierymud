#6100
dwarf_laborer_rand1~
0 b 30
~
say Ah, almost time to get back.
say I still haven't found anyone decent...
emote looks concerned.
comfort worker
~
#6101
dwarf_laborer_rand2~
0 b 30
~
emote glances at you for a second.
chuckle
say Nope, not strong enough for the mines, need dwarven miners.
~
#6102
dwarf_laborer_rand3~
0 b 30
~
emote mutters to himself and looks at his watch.
sigh
say So little time, and so much to do.
~
#6103
dwarven_merc_rand1~
0 b 15
~
emote mutters to himself.
say That stupid Ruborg, saying I wasn't even in the wars.
say If I see him...
grin
say Well, he won't like it.
~
#6104
dwarven_merc_rand2~
0 b 30
~
emote expounds loudly on the virtues of the battleaxe over the sword.
~
#6105
dwarven_merc_greet1~
0 g 30
~
if %actor.name% == -1
nudge %actor.name%
say Hey %actor.name%, did I tell you about the part I played in the Great Duergar Wars?
strut
say I captured three duergar captains single-handedly.
endif
~
#6106
drunk_dwarf_rand1~
0 b 40
~
set antic %random.3%
switch %antic%
  case 1
    emote whistles tunelessly to himself.
    say Another day of drinking for me please.
    hiccup
    hiccup
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
#6107
drunk_dwarf_rand2~
0 b 20
~
glare
say What are you looking at?
emote puts up his fists.
say Do you want some?  Do ya?
fart
~
#6108
dwarven_janitor_entry1~
0 i 30
~
sigh
say So much litter, this job is unending.
grumble
~
#6109
dwarven_janitor_rand1~
0 b 30
~
emote mutters about pay and conditions these days.
~
#6110
dwarven_janitor_rand2~
0 b 30
~
emote sweeps around your feet without even noticing you.
~
#6111
dog_greet1~
0 g 30
~
if %actor.vnum% == -1
emote stops scratching itself and looks up at %actor.name%
if %actor.align% > 349
lick %actor.name%
else
growl %actor.name%
bark
bite %actor.name%
endif
endif
~
#6112
dog_greet2~
0 g 30
~
msend %actor% %self.name% wanders up to you and sniffs your leg.
msend %actor% %self.name% whimpers and moves away from you with its tail between its legs.
~
#6113
dog_rand1~
0 b 30
~
bark
emote starts licking itself.
~
#6118
lame_beggar_rand1~
0 b 30
~
emote mutters to himself.
say Being a beggar just isn't such a good calling these days.
emote scratches at a sore.
~
#6119
lame_beggar_rand2~
0 b 40
~
emote holds his sore ridden hands out to you.
say Please spare some money, my children are starving.
~
#6120
lame_beggar_bribe~
0 m 1
~
wait 1
mecho %self.name%'s eyes light up.
if %actor.sex% == Female
   say Thank you so much, madam!
else
   say Thank you so much, sir!
endif
~
#6157
new trigger~
0 g 7
~
say My trigger commandlist is not complete!
~
$~
