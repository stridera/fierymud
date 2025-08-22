#100
set Breathe Fire~
0 o 100
~
mskillset %self% breathe fire
mskillset %self% fire breath
~
#101
set Breathe Frost~
0 o 100
~
mskillset %self% breathe frost
mskillset %self% frost breath
~
#102
set Breathe Acid~
0 o 100
~
mskillset %self% breathe acid
mskillset %self% acid breath
~
#103
set Breathe Gas~
0 o 100
~
mskillset %self% breathe gas
mskillset %self% gas breath
~
#104
set Breathe Lightning~
0 o 100
~
mskillset %self% breathe lightning
mskillset %self% lightning breath
~
#105
set Roar~
0 o 100
~
mskillset %self% roar
~
#106
Cleric Quest Spell Hints~
0 d 100
heal heal? resurrect resurrect? group group? dragon dragon? dragons dragons? health health?~
wait 2
switch %speech%
  case heal
  case heal?
  case group
  case group?
  case armor
  case armor?
    if %self.class% /= cleric || %self.class% /= priest 
      say Ah yes, there are two powerful group spells:
      mecho '&3&bGroup Heal&0 was a potent tool in various religious practices. There is a'
      mecho 'doctor who had been researching at his infirmary out west. He may know more.'
      mecho    
      mecho 'The other is &7&bGroup Armor&0, powerful war magic. The paladins of South'
      mecho 'Caelia make ample use of it.'
    elseif %self.class% /= diabolist
      say Ah yes, there are two powerful group spells:
      mecho '&3&bGroup Heal&0 can be learned by members of the diabolical orders, but'
      mecho 'unfortunately it comes from outside our religious traditions. As loathe as I'
      mecho 'am to admit it, you may have to talk to some of the monks of the more'
      mecho 'priestly orders for more information.'
      mecho   
      mecho 'The other is a spell our demonic patrons will not grant us. Don't waste your'
      mecho 'time trying to pursue it.'
    else
      say I'm afraid I know little about more advanced
      mecho 'group support magics.'
    endif
    break
  case resurrect
  case resurrect?
    if %self.class% /= cleric || %self.class% /= priest || %self.class% /= diabolist
      mecho %self.name% says, 'Yes, it is possible to &6&bResurrect&0 the dead.'
      mecho 'However the secrets of it have been lost over the centuries. Only'
      mecho 'someone with knowledge of ages past may have a chance of still knowing such'
      mecho 'magics.'
      mecho   
      mecho 'There are rumors that nigh-immortal supplicants orship in the darkest of'
      mecho 'traditions. Perhaps someone in the most trecherous of religious houses would'
      mecho 'know where to begin.'
    elseif %self.class% /= necromancer
      say Just use &9&bAnimate Dead&0. Isn't that enough for you?
    else
      say I'm afraid I know little about returning the dead
      mecho 'to life.'
    endif
    break
  case dragon
  case dragon?
  case dragons
  case dragons?
  case health
  case health?
    if %self.class% /= cleric || %self.class% == priest
      say &3&bDragons Health&0 is a prayer derived from a
      mecho 'song passed down amongst true dragon kind. Occassionally, highly trusted'
      mecho 'dragonborn are taught the song while they care of the nests of their larger'
      mecho 'cousins.'
      mecho   
      mecho 'Often these nesting grounds are near warm dunes or beaches.'
    else
      say As much as I wish I knew more about dragons,
      mecho 'my knowledge of that spell is limited.'
    endif
    break  
  default
    return 0
done
~
#107
Priest Quest Spell Hints~
0 d 100
banish banish? heaven heaven? heavens heavens? gate gate?~
wait 2
switch %speech%
  case gate
    if %self.class% /= diabolist
      if %actor.class% /= diabolist
        say Our infernal lords can grant us the power to open a
        mecho '&1&bHell Gate&0 o travel anywhere instantly via the Nine Hells.'
        mecho   
        mecho 'Places where Garl'lixxil have touched Ethilien like Templace are particularly'
        mecho 'suitable for attuning to the proper energies.'
      elseif %actor.class% /= priest
        say We don't trifle with such silly things.
        mecho 'Our diabolical patrons shift us through the very bowels of Hell instead!'
      elseif %actor.class% /= druid
        say Our traditions use inferal methods of travel, not lunar ones.
      endif
    elseif %self.class% /= priest
      if %actor.class /= diabolist
        say Members of priestly orders travel by the Will of
        mecho 'Heaven. We do not spread the knowledge of Infernal magics.'
      elseif %actor.class% /= priest
        say The secrets to opening &6&bHeavens Gate&0 are
        mecho 'taught by the stars themselves.'
        mecho   
        mecho 'There is a place near Anduin where starlight reaches deep into the bowels of'
        mecho 'Ethilien and whispers to those who listen.'
      elseif %actor.class% /= druid
        say As much as we too revere the Moon in the Heavens,
        mecho 'we're unable to provide you with the secrets of your Order.'
      endif
    elseif %self.class% /= druid
      if %actor.class% /= druid
        mecho %self.name% says, 'The ability to call &6&bMoonwells&0 is a gift
        mecho 'bestowed on us by the greatest powers in Nature.'
        mecho   
        mecho 'The western-most branch of our order is well known for their lunar rites.'
      elseif %actor.class% /= diabolist
        say We move by the power of the Moon,
        mecho 'not the Nine Hells.'
      elseif %actor.class% /= priest
        say We move by the power of the Moon,
        mecho 'not the Will of Heaven.'
      endif
    else
      say Opening gates and wells is the works of other
      mecho 'magical traditions, not ours.'
    endif
    break
  case banish
  case banish?
    if %self.class% /= priest || %self.class% /= diabolist
      say There's a diabolist known to skulk about South
      mecho 'Caelia who is said to be quite adept at &9&bBanish&0.'
      mecho   
      mecho 'They say she makes common use of it in the conflict between the military city'
      mecho 'Ogakh and the paladins of Sacred Haven.'
    else
      say Unfortunately that spell is outside of our
      mecho 'guild traditions.'
    endif
    break
  case heaven
  case heavens
  case heaven?
  case heavens?
    if %self.class% /= priest
      say The secrets to opening &6&bHeavens Gate&0 are
      mecho 'taught by the stars themselves.'
      mecho   
      mecho 'There is a place near Anduin where starlight reaches deep into the bowels of'
      mecho 'Ethilien and whispers to those who listen.'
    elseif %self.class% /= diabolist
      say We don't trifle with such silly things.
      mecho 'Our diabolical patrons shift us through the very bowels of of Hell instead!'
    elseif %self.class% /= druid
      say We move by the power of the Moon,
      mecho 'not the Will of Heaven.'
    else
      say Apologies, I haven't the faintest clue where to begin looking.
    endif
    break
  default
    return 0
done
~
#108
Diabolist Quest Spell Hints~
0 d 100
hell hell? hellfire hellfire? brimstone brimstone? fire fire?~
wait 2
switch %speech%
  case hell
  case hell?
    if %self.class% /= diabolist
      say Our infernal lords do grant us the power to open a
      mecho '&1&bHell Gate&0 to travel anywhere instantly via the Nine Hells.'
      mecho   
      mecho 'Places where Garl'lixxil have touched Ethilienm, like Templace, are'
      mecho 'particularly suitable for attuning to the proper energies.'
    elseif %self.class% /= priest
      say Members of priestly orders travel by the Will of
      mecho 'Heaven. We do not spread the knowledge of Infernal magics.'
    elseif %self.class% /= druid
      say We move by the power of the Moon,
      mecho 'not the Nine Hells.'
    else
      say We have no knowledge of that spell in our Guild.
    endif
    break
  case hellfire
  case hellfire?
  case brimstone
  case brimstone?
  case fire
  case fire?
    if %self.class% /= diabolist
      say There is a cult on the edge of South Caelia known
      mecho 'for conjuring &1&bHellfire and Brimstone&0. Their dark leader holds the keys'
      mecho 'to the spell.'
    elseif %self.class% /= priest || %self.class% /= cleric
      say We do not dabble in such unholy magics.
    elseif %self.class% /= pyromancer
      say Unfortunately I don't know much about that spell,
      mecho 'but I sure would love to...'
    else
      say That spell isn't part of our magical tradition.
    endif
    break
  default
    return 0
done
~
#109
Sorcerer Quest Spell Hints~
0 d 100
major major? globe globe? relocate relocate? charm charm? person person? wizard wizard? eye eye?~
wait 2
switch %speech%
  case major
  case major?
  case globe
  case globe?
    if (%self.class% /= sorcerer || %self.class% /= pyromancer || %self.class% /= cryomancer)
      say The battle mage Lirne was talking about
      mecho 'developing a new spell called &4&bMajor Globe&0 to defend himself against'
      mecho 'powerful magicians.'
      mecho   
      mecho 'Last I heard, he was going to test it out against a powerful demonic agent of'
      mecho 'chaos in the far north.'
    else
      say I'm afraid Major Globe isn't a spell I know how
      mecho 'to cast.'
    endif
    break
  case relocate
  case relocate?
    if (%self.class% /= sorcerer || %self.class% /= pyromancer || %self.class% /= cryomancer)
      say A student of Bigby's in Mielikki was working on
      mecho 'A powerful teleportation spell she called &5&bRelocate&0.'
      mecho   
      mecho 'Bigby or one of his lab assistants might know more.'
    else
      say Our Guild doesn't keep records of esoteric
      mecho 'magics exclusive to other guilds. Sorry.'
    endif
    break
  case charm
  case charm?
  case person
  case person?
    if (%self.class% /= sorcerer || %self.class% /= illusionist || %self.class% /= bard)
      say Charming is both a magical and a personal skill.
      mecho '&5&bCharm Person&0 relies on mixing both.'
      mecho   
      mecho 'However, those that make charming others a profession, particularly'
      mecho 'courtesans, are the best teachers.'
    else
      say As charming as you are, I cannot point you in a
      mecho 'better direction.'
    endif
    break
  case wizard
  case wizard?
  case eye
  case eye?
    if %self.class% /= sorcerer
      say A simple but highly effective spell! The shamans of the
      mecho 'Great Snow Leopard are particularly adept At teaching &6&bWizard Eye&0 to'
      mecho 'interested scholars.'
      mecho   
      mecho 'Seek them out to learn more.'
    else
      say That is a highly exclusive sorcerer spell. I am
      mecho 'unable to help with that.'
    endif
    break
  default
    return 0
done
~
#110
Cryomancer Quest Spell Hints~
0 d 100
waterform waterform? flood flood? ice ice? shards shards? water water? form form? wall wall?~
wait 2
switch %speech%
  case waterform
  case waterform?
  case water
  case water?
  case form
  case form?
    if %self.class% /= cryomancer
      mecho %self.name% says, '&4&bWaterform&0 is more absorbed via osmosis than
      mecho 'taught in the traditional sense. The spell is typically imparted by powerful'
      mecho 'water elementals.'
      mecho    
      mecho 'You can find such elementals in the vast lakes and rivers of Ethilien.'
    else
      say That spell is unique to the Cryomancer's Guild.
    endif
    break
  case ice
  case ice?
  case shards
  case shards?
    if %self.class% /= cryomancer
      mecho %self.name% says, 'Unfortunately, &6&bIce Shards&0'
      mecho 'has been lost to our guild for many centuries. Khysan, a cyromancer from a'
      mecho 'smaller branch of the elven Sunfire family, has been doing research into the'
      mecho 'last known records of the spell, but hasn't reported any findings or updates'
      mecho 'in many years.'
      mecho   
      mecho 'Perhaps you could check in with him. He works at the springs.'
    else
      say I've heard even the cryomancers don't know where
      mecho 'to look for that spell. There's no chance anyone in our Guild would know.'
    endif
    break
  case flood
  case flood?
    if %self.class% /= cryomancer
      say The power to call the roaring waters of Ethilien
      mecho 'in a cataclysmic &4&bFlood&0 is taught by the ocean herself. You would be
      mecho 'either very lucky or very cursed to find her avatar.
    elseif %self.class% /= druid
      say We commune with the waters of the world in a very,
      mecho 'very different way.'
    else
      say Unfortunately I don't have any guidance for you.
    endif
    break
  case wall
  case wall?
    if %self.class% /= cryomancer
      if %actor.class% /= cryomancer
        say They regularly make use of &6&bWall of Ice&0 to
        mecho 'keep The most dangerous creatures from Frost Valley out of Mount Frostbite.'
        mecho   
        mecho 'Someone up there will know what to do.'
      elseif %actor.class% /= illusionist
        mecho %self.name% says, 'There is an &5&bIllusory Wall&0 spell out there,'
        mecho 'but I don't know much about it.'
      endif
    elseif %self.class% /= illusionist
      if %actor.class% /= illusionist
        say The best teacher of &5&bIllusory Wall&0 was a Post Commander
        mecho 'for the Eldorian Guard.'
        mecho   
        mecho 'I hear she retired to the far north.'
      elseif %actor.class% /= cryomancer
        mecho %self.name% says, 'Your &6&bWall of Ice&0 spell is extremely similar to our wall'
        mecho 'spell, but I don't know much about it.'
      endif
    else
      say Wall spells are not part of our Guild teachings.
    endif
    break
  default
    return 0
done
~
#111
Druid Quest Spell Hints~
0 d 100
creeping creeping? doom doom? moon moon? well well? moonwell moonwell?~
wait 2
switch %speech%
  case creeping
  case creeping?
  case doom
  case doom?
    if %self.class% /= druid
      say The rage of the fair folk can summon a carpet of
      mecho 'living death.'
      mecho   
      mecho 'Seek out the weakest-seeming of faeries and sprites to learn &2&bCreeping Doom&0.'
    else
      say Invoking the wrath of nature is exclusive to the
      mecho 'Druid Guild.'
    endif
    break
  case moonwell
  case moonwell?
  case well
  case well?
  case moon
  case moon?
    if %self.class% /= diabolist
      say Our traditions use infernal methods of travel,
      mecho 'not lunar ones.'
    elseif %self.class% /= priest
      say As much as we too revere the Moon, we're unable
      mecho 'to provide you with the secrets of your Order.'
    elseif %self.class% /= druid
      if %actor.class% /= druid
        mecho %self.name% says, 'The ability to call &6&bMoonwells&0 is a gift'
        mecho 'bestowed on us by the greatest powers in Nature. The western-most branch of'
        mecho 'our order is well known for their lunar rites.'
      elseif %actor.class% /= diabolist
        say We move by the power of the Moon,
        mecho 'not the Nine Hells.'
      elseif %actor.class% /= priest
        say We move by the power of the Moon,
        mecho 'not the Will of Heaven.'
      endif
    else
      say Opening gates and wells is the works of other
      mecho 'magical traditions, not ours.'
    endif
    break
  default
    return 0
done
~
#112
Ranger Quest Spell Hints~
0 d 100
blur~
wait 2
switch %speech%
  case blur
    if %self.class% /= ranger
      say After proving your dedication to the spirits of
      mecho 'nature you must triumph in a challenge against the Four Winds to learn &2&bBlur&0.'
      mecho   
      mecho 'Investigating and ending an eternal conflict in a forest would be an ideal'
      mecho 'way to demonstrate your devotion to the natural world.'
    else
      say That spell is fiercely guarded by
      mecho 'the Ranger Guild.'
    endif
    break
  default
    return 0
done
~
#113
object casts hellbolt~
1 d 10
~
ocast 'hell bolt' %victim%
~
#114
object casts stygian eruption~
1 d 10
~
ocast 'stygian eruption' %victim%
~
#115
Bard AI~
0 b 100
~
if !%self.has_spell[inspiration]%
   mperform inspiration %self% %self.level%
endif
~
#116
Bard combat songs AI~
0 k 100
~
if %self.level% >= 10
   if (%now% && (%time.stamp% - %now% >= 5)) || !%now%
      if !%actor.has_spell[terror]% && !%actor.has_spell[ballad of tears]%
         mperform terror %actor% %self.level%
         set now %time.stamp%
         global now
      endif
   endif
endif
if %self.level% >= 70
   if (%now2% && (%time.stamp% - %now2% >= 5)) || !%now2%
      if %actor.group_size% > 1
         set room %self.room%
         set person %room.people%
         while %person%
            if %person.vnum% == -1
               if !%self.has_spell[terror]% && !%self.has_spell[ballad of tears]%
                  mperform 'ballad of tears' %person% %self.level%
                  set now2 %time.stamp%
                  global now2
                  halt
               endif
            endif
            set person %person.next_in_room%
         done
      endif
   endif
endif
~
#117
**UNUSED**~
0 c 100
b bu~
return 0
~
#118
**UNUSED**~
0 c 100
s~
return 0
~
#119
vampiric weapon effect~
1 d 3
~
osend %actor% A &9&bblack haze&0 forms around your sword as you strike %victim.name%!
osend %victim% A &9&bblack haze&0 forms around %actor.name%'s sword as %heshe% strikes you!
ocast 'vampiric breath' %victim% %actor.level%
~
#120
Bonus fire damage~
1 d 100
~
*
* This trigger adds 10% damage as fire damage
*
if %damage%
  eval bonus %damage% / 10
  oecho &1%self.shortdesc% burns %victim.name%!&0 (&3%bonus%&0)
  odamage %victim% %bonus% fire
endif
~
#121
Bonus cold damage~
1 d 100
~
*
* This trigger adds 10% damage as cold damage
*
if %damage%
  eval bonus %damage% / 10
  oecho &6&b%self.shortdesc% freezes %victim.name%!&0 (&3%bonus%&0)
  odamage %victim% %bonus% cold
endif
~
#122
Bonus shock damage~
1 d 100
~
*
* This trigger adds 10% damage as shock damage
*
if %damage%
  eval bonus %damage% / 10
  oechoaround %victim% &3%self.shortdesc% shocks %victim.name%!&0 (&3%bonus%&0)
  osend %victim% &3%self.shortdesc% shocks you!&0 (&3%bonus%&0)
  odamage %victim% %bonus% shock
endif
~
#123
Bonus holy damage~
1 d 100
~
*
* adds 5% alignment damage against neutral targets and 10% against evil targets
*
if %damage%
  if %victim.align% < 350 && %victim.align% > -350
    eval bonus %damage% / 20
  elseif %victim.alignment% < -350
    eval bonus %damage% / 10
  endif
  if %bonus% && %victim.hit% >= -10
    oechoaround %victim% &7&b%self.shortdesc% smites %victim.name% with radiant light!&0 (&3%bonus%&0)
    osend %victim% &7&b%self.shortdesc% smites you with radiant light!&0 (&3%bonus%&0)
    odamage %victim% %bonus% align
  endif
endif
~
#124
Bonus unholy damage~
1 d 100
~
*
* adds 5% alignment damage against neutral targets and 10% against good targets
*
if %damage%
  if %victim.align% < 350 && %victim.align% > -350
    eval bonus %damage% / 20
  elseif %victim.alignment% > 350
    eval bonus %damage% / 10
  endif
  if %bonus%
    oecho &9&b%self.shortdesc% smites %victim.name% with unholy might!&0 (&3%bonus%&0)
    odamage %victim% %bonus% align
  endif
endif
~
#125
Major fire weapon~
1 d 100
~
*
* This trigger adds 100% damage as fire damage or 200% fire damage against air, plant, or bone that evade
*
if %damage%
  eval bonus %damage%
  oecho &1%self.shortdesc% burns %victim.name%!&0 (&3%bonus%&0)
  odamage %victim% %bonus% fire
else
  if %victim.composition% == air || %victim.composition% == bone || %victim.composition% == plant
    if %actor.real_str% >= 61 && %actor.real_str% < 63
      set strmod 1
    elseif %actor.real_str% >= 64 && %actor.real_str% < 66
      set strmod 2
    elseif %actor.real_str% >= 67 && %actor.real_str% < 69
      set strmod 3
    elseif %actor.real_str% >= 70 && %actor.real_str% < 72
      set strmod 4
    elseif %actor.real_str% >= 73 && %actor.real_str% < 75
      set strmod 5
    elseif %actor.real_str% >= 76 && %actor.real_str% < 78
      set strmod 6
    elseif %actor.real_str% >= 79 && %actor.real_str% < 81
      set strmod 7
    elseif %actor.real_str% >= 82 && %actor.real_str% < 84
      set strmod 8
    elseif %actor.real_str% >= 85 && %actor.real_str% < 87
      set strmod 9
    elseif %actor.real_str% >= 88 && %actor.real_str% < 90
      set strmod 10
    elseif %actor.real_str% >= 91 && %actor.real_str% < 93
      set strmod 11
    elseif %actor.real_str% >= 94 && %actor.real_str% < 96
      set strmod 12
    elseif %actor.real_str% >= 97 && %actor.real_str% < 99
      set strmod 13
    elseif %actor.real_str% == 100
      set strmod 14
    endif
    eval bonus %strmod% + %actor.damroll% + ((random.%self.val2%) * %self.val1%) * 2
    if %random.20% == 20
      eval bonus %bonus% * 2
    endif
    oecho &1%self.shortdesc% still burns %victim.name%!&0 (&3%bonus%&0)
    odamage %victim% %bonus% fire
  endif
endif
~
#126
weapon_cast_fireball~
1 d 10
~
ocast fireball %victim%
~
#127
turn to shock~
1 j 100
~
ocast 'tempest of saint augustine' %actor% 10
~
#129
**UNUSED**~
1 c 100
hi~
return 0
~
#130
**UNUSED**~
1 c 100
p~
return 0
~
#131
**UNUSED**~
1 c 100
for~
return 0
~
#132
**UNUSED**~
1 c 100
se~
return 0
~
#150
object_cast_insanity~
1 b 5
~
set worn %self.worn_by%
if !%worn%
    halt
else
    if %worn.vnum% == -1
        osend %self.worn_by% %self.shortdesc% whispers words of madness to you!
        ocast insanity %self.worn_by%
    endif
endif
~
$~
