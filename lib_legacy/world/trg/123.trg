#12300
megalith_quest_priestess_greet~
0 g 100
~
* This trigger is different depending on stage and where in the stage the questor is at
*
set item1 (%actor.quest_variable[megalith_quest:item1]%)
set item2 (%actor.quest_variable[megalith_quest:item2]%)
set item3 (%actor.quest_variable[megalith_quest:item3]%)
set item4 (%actor.quest_variable[megalith_quest:item4]%)
if (%actor.vnum% == -1) &&(%actor.quest_stage[megalith_quest]% < 1) &&(%actor.level% < 100)
  wait 2
  mecho %self.name% looks up, startled to see new faces.
  say Goddess be praised!
  wait 2s
  mecho %self.name% says, 'We were attacked by the Unseelie Court and their monstrosities on our journey here through the forest.  I am deeply in need of &6&bassistance&0!'
*
* Check in during stage 1, seeing if the questor has her tools or not
*
elseif (%actor.quest_stage[megalith_quest]% == 1)
  wait 2
  mecho %self.name% says, 'Have you been able to find replacements for the sacred prophetic implements?  Do you need a reminder of your &6&b[progress]?&0'
* 
* If all the Keepers have been helped in stage 2, give the incantation to start stage 3
*
elseif (%actor.quest_stage[megalith_quest]% == 2) &&(%actor.quest_variable[megalith_quest:item1]%) &&(%actor.quest_variable[megalith_quest:item2]%) &&(%actor.quest_variable[megalith_quest:item3]%) &&(%actor.quest_variable[megalith_quest:item4]%)
  wait 2
  say We're ready to finish calling the elements!  If you wish to proceed, repeat after me:
  mecho &0 
  mecho &6&bUnder the watchful eye of Earth, Air, Fire, and Water, we awaken this hallowed ground!&0
*
* If all the Keepers have not been helped in stage 2, do a check in
*
elseif (%actor.quest_stage[megalith_quest]% == 2) 
  wait 2
  mecho %self.name% says, 'I hope your work with the Keepers is coming along well.  Do you need a reminder of your &6&b[progress]?&0'
*
* If all the reliquaries have been returned and stage 4 is ready to start, ask if ready to proceed
*
elseif (%actor.quest_stage[megalith_quest]% == 4) &&(%actor.quest_variable[megalith_quest:reliquary]%) 
  wait 2
  say It's time.  Are you ready to proceed?
*
* If all the reliquaries have not been returned yet in stage 3, do a check in
*
elseif (%actor.quest_stage[megalith_quest]% == 3) 
  wait 2
  say I hope you have had luck finding the reliquaries.
*
* If the quest was successfully completed, give a friendly welcome.
*
elseif %actor.has_completed[megalith_quest]%
  wait 2
  if %actor.sex% == female
    say Hail and well met, Sister!
  elseif %actor.sex% == male
    say Hail and well met, Brother!
  else
    say Hail and well met!
  endif
*
* If the quest was failed, 
*
elseif %actor.has_failed[megalith_quest]%
  wait 2
  say Welcome back.
  wait 2
  say Had that we reunited under better blessings...
  Wait 3s
  say Though perhaps we could try again?
endif
~
#12301
priestess_speech1~
0 d 1
assist assistance assist? assistance? how what~
if %speech% /= assist || %speech% /= assistance || %speech% /= assist? || %speech% /= assistance? || %speech% /= how can I help? ||  %speech% /= how can I assist? || %speech% /= what can I do? || %speech% /= what can I do to assist? 
  wait 2
  mecho %self.name% says, 'Our coven came to this megalith to perform the &7&bGreat Rite of Invocation&0 and summon our faerie goddess The Great Mother, the Lady of the Stars.'
endif
~
#12302
priestess_speech2~
0 dn 0
great rite~
if %speech% /= GREAT RITE OF INVOCATION? || %speech% /= GREAT RITE? || %speech% /= GREAT RITE OF INVOCATION || %speech% /= GREAT RITE
  wait 2
  mecho %self.name% says, 'For generations, our coven has scoured the face of Ethilien to find a place where the touch of the &6&bOld Gods&0 might still be found.'
  wait 3s
  mecho %self.name% says, 'Performed correctly, this ritual will allow us to call to our Goddess in the Dreaming and provide a bridge across space and time back into the mortal realm.'
endif
~
#12303
priestess_speech3~
0 dn 1
old why?~
If %speech% /= old gods || if %speech% /= old gods? || if %speech% /= why?
  wait 2
  say You see, when the Nine Hells cracked open to vomit forth Sagece on Templace, the putrid rift she crawled through so corrupted the world around it, many of the Old Gods found their holy sites defiled and their physical access to Ethilien severed.
  wait 6s
  say The Great Mother was one such unfortunate victim.
  wait 3s
  say She is cast adrift in the Deep Dreaming, lost in that birthplace and graveyard of primordial stars.  The near-total annihilation of the elven race, constant assault on our world by demons, gods turning on each other in bloody slaughter...
  mecho &0 
  mecho %self.name% says, 'All these calamities along with centuries of unearned exile are weakening Her bond with our world.  Our Mother is still capable of gifting us her divine magic, but She is &6&bslipping away&0 from us.'
endif
~
#12304
priestess_speech4~
0 dn 1
slipping how what~
if %speech% /= slipping away || %speech% /= slipping away? || %speech% /= how terrible || %speech% /= how sad  || %speech% /= what can be done?
  wait 2
  say We began to lose hope until we found this ancient circle of standing stones.  But I believe this place is powerful enough for us to perform the Great Invocation Rite.
  wait 4s
  say Several of our sacred prophetic implements were damaged navigating the maze.  My Sisters are too busy tending to their duties preparing this site for the Great Rite to find suitable replacements.
  wait 5s
  mecho %self.name% says, 'But perhaps you would be willing to &6&bhelp us?&0'
endif
~
#12305
megalith_quest_priestess_speech_start~
0 dn 1
yes yes?~
* This trigger serves as both the starting, restarting, and advancing from stage 3 to stage 4 trigger
* If starting for the first time
*
if (%actor.quest_stage[megalith_quest]% < 1)
  quest start megalith_quest %actor.name%
  wait 2
  say May the Goddess smile upon you!
  wait 1s
  mecho %self.name% says, 'First, I'll need your help replacing the &6&bimplements&0 destroyed in the maze getting here.'
*
* If restarting
*
elseif %actor.has_failed[megalith_quest]%
  quest restart megalith_quest %actor.name%
  mecho &0
  smile
  say Then let us give it another go!
  mecho  
  mecho %self.name% says, 'Some of our &6&bimplements&0 were destroyed in our previous attempt.'
endif
*
* List tools for stage 1
*
if (%actor.quest_stage[megalith_quest]% == 1)
  wait 2
  mecho %self.name% says, 'We need the following:
  mecho - &7&bSalt&0.
  mecho  
  mecho - A &6&bgoblet or chalice&0 to hold water.
  mecho &0    My home island in the Green Green Sea has several goblets, though do be
  mecho &0    careful which one you pick.  Some of them tend to be poisoned.
  mecho  
  mecho &0    I understand there is a beautiful chalice recently lost from the Abbey of
  mecho &0    St. George that might work as well.
  mecho  
  mecho - Some kind of &3censer&0 to burn incense in.
  mecho &0    There are a few other more nefarious religions which use incense burners in
  mecho &0    their rituals.  You may be able to steal one from them.
  mecho  
  mecho - A &1&bcandle&0.
  mecho &0    Candles are common to light the dark in the far north, and in some
  mecho &0    underground communities.'
  wait 6s
  say Bring these to me to begin the Great Rite!
  wait 1s
  mecho %self.name% says, 'If you want to know more about what my Sisters and I are doing, you can ask any of us &6&bwho are you?&0'
  wait 1s
  mecho %self.name% says, 'If you need a reminder of your &6&b[progress]&0, you may ask me at any time.'
  bow %actor.name%
*
* If ready to continue stage 4
*
elseif (%actor.quest_stage[megalith_quest] == 4) && (%self.room% == 12389) && (%actor.quest_variable[megalith_quest:reliquary]% == 1)
  wait 2
  say The Great Rite of Invocation has a great deal of call and response.  I will chant a line and you must repeat it to continue the ritual.  The coven will chant their response after you do.
  wait 4s
  say Repeat after me.
  wait 2s
  emote raises her arms to the sky.
  mecho %self.name% chants, '&6&bGreat Lady of the Stars, hear our prayer!&0'
  quest variable megalith_quest %actor.name% prayer 1
endif
~
#12306
megalith_quest_priestess_receive~
0 j 100
~
* This trigger serves as the basis for the returning of items to the high priestess and advancing stages 1 and 3.
* It is heavily copied from the Hot Springs quests.
* Several of the stages have multiple items the questor could return. 
* Some can result in the quest variables [bad1-3] being placed on the character.  
* Ideally, there will be 0 [bad]
* If there are 2 [bad] or more at the end, the final step of the quest will fail.  
* If bad4 is on, DISASTER will occur!! 
* There is currently no option to exchange items once they have been delivered
*
* What item is being turned in?
*
if %actor.quest_stage[megalith_quest]% == 1
  set step replace our ritual implements
  *
  * Set the potential items that could be turned in in Stage 1
  *
  * the expressions set item 2, set item 3, and set item 4 appear multiple times because multiple objects could satisfy the item requirement, but we only want to accept one.
  * Also create a variable so we can return the cup to the character in stage 3
  *
  switch %object.vnum%
    * salt
    case 23756
      set item 1
      set this %object.shortdesc%
      break
    * hemlock goblet - BAD1
    case 41111
      set item 2
      set goblet 41111
      set this a drinking vessel
      break
    * rowan goblet
    case 41110
      set this a drinking vessel
      set item 2
      set goblet 41110
      break
    * chalice
    case 18512
      set item 2
      set goblet 18512
      set this a drinking vessel
      break
    * censer 
    case 8507
      set item 3
      set this an incense burner
      break
    * thurible 
    case 17300
      set item 3
      set this an incense burner
      break
    * candle
    case 8612
      set item 4
      set this spare candles
      break
    * candle
    case 58809
      set item 4
      set this spare candles
      break
    default
      halt
  done
elseif %actor.quest_stage[megalith_quest]% == 3
  set step summon %get.mob_shortdesc[12300]%
  *
  * Set the potential items that could be returned in Stage 3
  *
  switch %object.vnum%
    * bowl
    case 23817
      set item 1
      set this %object.shortdesc%
      break
    * goddess skirt - BAD3
    case 4305
      set item 2
      set this something from a goddess's regalia
      break
    * goddess torch - ALSO BAD3
    case 4318
      set item 2
      set this something from a goddess's regalia
      break
    * goddess bracelet
    case 58015
      set item 2
      set this something from a goddess's regalia
      break
    * goddess ring
    case 58018
      set item 2
      set this something from a goddess's regalia
      break
    * faerie elixir - BAD4 - THE REALLY BAD ONE
    case 58426
      set this something to serve as Her icon
      set item 3
      break
    * faerie wings
    case 58418
      set item 3
      set this something to serve as Her icon
      break
    default
      halt
  done
endif
*
* if you already gave us this item.  The value %item% has been set to appends item here, resulting in item1, item2, item3, and item4.
*
if %actor.quest_variable[megalith_quest:item%item%]%
  return 0
  mecho %self.name% says, 'Thank you, but you already brought me %this%.'
  give %object.name% %actor.name%
  halt
endif
*
* Using the process above, if the thing turned in didn't match item1 item 2 item 3 or item4 we can accept it now and set the quest variable.
*
wait 2
mecho %self.name% says, 'Blessed be!  Just what we need to %step%!'
quest variable megalith_quest %actor.name% item%item% 1
if %object.vnum% == 41110 || %object.vnum% == 18512 || %object.vnum% == 41111
  quest variable megalith_quest %actor.name% goblet %goblet%
  if %object.vnum% == 41111
    quest variable megalith_quest %actor.name% bad1 1
  endif
elseif %object.vnum% == 4318 || %object.vnum% == 4305
  quest variable megalith_quest %actor.name% bad3 1
elseif %object.vnum% == 58426
  quest variable megalith_quest %actor.name% bad4 1
endif
mjunk %object.name%
*
* See if we've turned in everything for this step
*
set item 1
while %item% <= 4
  set item%item% 0
  eval item %item% + 1
done
if %actor.quest_variable[megalith_quest:item1]%
  set item1 1
endif
if %actor.quest_variable[megalith_quest:item2]%
  set item2 1
endif
if %actor.quest_variable[megalith_quest:item3]%
  set item3 1
endif
if (%actor.quest_stage[megalith_quest]% != 1) || %actor.quest_variable[megalith_quest:item4]%
  set item4 1
endif
*
* If all the items have been turned in
*
if %item1% && %item2% && %item3% && %item4%
  wait 2
  quest advance megalith_quest %actor.name%
  set item 1
  *
  * Reset item variables with a while-loop
  * This quest uses 5 item variables, but only 4 are ever checked in this receive trigger.  Clear all 5 here just to be safe.
  *
  while %item% <= 5
    quest variable megalith_quest %actor.name% item%item% 0
    eval item %item% + 1
  done
  wait 1s
  say I believe we're ready to proceed!
  wait 1s
  *
  * If turning everything in starts stage 2
  *
  if %actor.quest_stage[megalith_quest]% == 2
    emote carefully places each tool in its proper position on the altar.
    wait 2s
    mecho %self.name% proclaims, 'Rejoice!  The Great Work begins!'
    emote lights the candle.
    mecho %self.name% lifts her arms and turns her face skyward.
    mecho %self.name% chants:
    mecho 'O spirits of the ancient deep,
    mecho &0protect us as we undertake our magical working.'
    wait 6s
    msend %actor% %self.name% slowly fixes her gaze on you.
    mechoaround %actor% %self.name% slowly fixes her gaze on %actor.name%.
    wait 1s
    mecho %self.name% says, 'Next you'll work with the &7&bKeepers&0 to call the elements to protect us and awaken the sleeping land.  You can find them preparing the four enormous menhirs in the forest.  Please check in with them to see what resources they need.'
    wait 2s
    say Oh wait, I almost forgot!
    wait 2s    
    *
    * Return the same drinking vessel from Stage 1
    *
    set goblet %actor.quest_variable[megalith_quest:goblet]%
    mload obj %goblet%
    mecho The coven high priestess takes %get.obj_shortdesc[%goblet%]% from the altar.
    mecho   
    pour goblet out
    give goblet %actor.name%
    mecho 
    mecho %self.name% says, 'You'll need this vessel again.  I've consecrated it for use in the Great Rite.'
    wait 3s
    say Now, go seek out the Keepers!
  *
  * If turning everything in starts stage 4
  *
  elseif %actor.quest_stage[megalith_quest]% == 4
    quest variable megalith_quest %actor.name% reliquary 1
    emote reverentially places each of the reliquaries on the altar.
    wait 2
    emote whispers,
    mecho 'By the power of three times three,
    mecho &0As I will it, so shall it be.'
    wait 4s
    emote breathes deeply and flexes her hands.
    wait 3s
    say It's time.
    wait 2s
    emote says in a hushed but excited tone, 'Are you ready?'
  endif
*
* If we need more stuff
*
else
  wait 2
  say If you have the other necessaries, please give them to me.
endif
~
#12307
megalith_quest_priestess_speech_stage2~
0 dn 1
under~
* Preset item vars
set item 0
set speech1 Under the watchful eye of Earth, Air, Fire, and Water, we awaken this hallowed ground!
set speech2 Under the watchful eye of Earth Air Fire and Water we awaken this hallowed ground
set speech3 Under the watchful eye of Earth Air Fire and Water we awaken this hallowed ground!
set speech4 Under the watchful eye of Earth, Air, Fire, and Water, we awaken this hallowed ground
set item1 %actor.quest_variable[megalith_quest:item1]%
set item2 %actor.quest_variable[megalith_quest:item2]%
set item3 %actor.quest_variable[megalith_quest:item3]%
set item4 %actor.quest_variable[megalith_quest:item4]%
if %speech% /= %speech1% || %speech% /= %speech2% || %speech% /= %speech3% || %speech% /= %speech4%
  if %actor.quest_stage[megalith_quest]% == 2
    if %item1% && %item2% && %item3% && %item4%
      quest advance megalith_quest %actor.name%
      set item 1
      *
      * Reset quest 'item' variables
      *
      while %item% <= 5
        quest variable megalith_quest %actor.name% item%item% 0
        eval item %item% + 1
      done
      unset item
      wait 1s
      mecho &7&bThe menhir begin to hum with deep chthonic harmonics.&0
      wait 3s
      say It's working!  I can feel the energies of the stones returning to life.
      wait 1s
      emote closes her eyes and basks in the chthonic resonance.
      wait 3s 
      emotes opens her eyes, invigorated.
      say We're so close to success!
      wait 3s
      mecho %self.name% says, 'With the circle cast and the land prepared, I require three holy &6&breliquaries&0 to summon our Goddess.'
    endif
  endif
endif
~
#12308
megalith_quest_priestess_speech_stage3~
0 dn 1
reliquaries reliquaries?~
if %actor.quest_stage[megalith_quest]% == 3
  wait 2
  say Indeed.  Sources of divine power to guide the Invocation:
  mecho &0 
  mecho - A &3&bprayer bowl&0 to scry upon and pierce through the veil between the worlds.
  mecho &0    I believe greater Celestials bring such things from the Outer Planes.
  mecho &0 
  mecho - An &3&bicon to represent the divine feminine&0.
  mecho &0    Any piece of the sacred regalia of another Goddess would be an appropriate
  mecho &0    icon.
  mecho &0 
  mecho - Something to represent Her fey nature.
  mecho &0    There are several &3&bfaerie relics&0 where the King of Dreams' realm has fused
  mecho &0    with ours.  I believe any of them will do.
  mecho  
  say Bring me these and we shall perform the Invocation!
endif
~
#12309
megalith_quest_priestess_speech_invocation1~
0 dn 1
great~
set speech1 great lady of the stars hear our prayer
set speech2 great lady of the stars, hear our prayer
set speech3 great lady of the stars, hear our prayer!
Set stage %actor.quest_stage[megalith_quest]%
if %speech% /= %speech1% || %speech% /= %speech2% || %speech% /= %speech3%
  if (%stage% == 4) && (%self.room% == 12389) && (%actor.quest_variable[megalith_quest:prayer]% == 1)
    m_run_room_trig 12317
    wait 5s
    mecho %self.name% chants:
    mecho &0'We call you from the realm behind
    mecho &0One adrift beyond space and time.
    mecho  
    mecho &0Ringing now the bell in three
    mecho &0Hear our prayer
    mecho &0&6&bWe summon and stir thee&0!'
    quest variable megalith_quest %actor.name% prayer 2
    quest variable megalith_quest %actor.name% summon 1
  endif
endif
~
#12310
megalith_quest_priestess_speech_invocation2~
0 dn 0
we summon and stir thee~
set speech1 we summon and stir thee
set speech2 we summon and stir thee!
set stage %actor.quest_stage[megalith_quest]%
set summon %actor.quest_variable[megalith_quest:summon]%
if (%speech% /= %speech1%) || (%speech% /= %speech2%)
  if (%stage% == 4) &&(%self.room% == 12389) &&(%actor.quest_variable[megalith_quest:prayer]% == 2)
    switch %summon%
      case 1
        m_run_room_trig 12318
        wait 5s
        mecho %self.name% chants:
        mecho 'By Earth, Air, Water, and Fire,
        mecho &0To bring you home is our desire.'
        wait 4s  
        m_run_room_trig 12319
        quest variable megalith_quest %actor.name% summon 2
        break
      case 2
        m_run_room_trig 12320
        wait 2s
        mecho %self.name% chants:
        mecho 'Through the plane of vaulted sky
        mecho &0On shooting stars and moonbridge high.'
        wait 4s  
        m_run_room_trig 12321
        quest variable megalith_quest %actor.name% summon 3
        break
      case 3
        *
        * 12320 is a wecho of just the phrase 'We summon and stir thee'
        * It is intentionally repeated at the top of this case to echo the questor
        *
        m_run_room_trig 12320
        wait 2s
        mecho %get.mob_shortdesc[12301]% says, '&6&bWe invoke thee&0!
    done
  endif
endif
~
#12311
megalith_quest_priestess_speech_end~
0 dn 0
we invoke thee~
set speech1 we invoke thee
set speech2 we invoke thee!
set stage %actor.quest_stage[megalith_quest]%
if (%stage% == 4) &&(%self.room% == 12389) &&(%actor.quest_variable[megalith_quest:summon]% == 3) && (%speech% /= %speech1% || %speech% /= %speech2%)
  set invoke %actor.quest_variable[megalith_quest:invoke]%
  eval chant %invoke% + 1
  switch %chant%
    case 1
      quest variable megalith_quest %actor.name% invoke 1
      wait 2
      m_run_room_trig 12322
      wait 1s
      say Again!
      mecho &6&bWe invoke thee!&0
      break
    case 2
      quest variable megalith_quest %actor.name% invoke 2
      wait 2
      m_run_room_trig 12322
      wait 1s
      say Once more!
      mecho &6&bWe invoke thee!&0
      break
    case 3
      quest variable megalith_quest %actor.name% invoke 0
      quest variable megalith_quest %actor.name% reliquary 0
      quest variable megalith_quest %actor.name% summon 0
      quest variable megalith_quest %actor.name% prayer 0
      if %actor.quest_variable[megalith_quest:bad1]%
        set bad1 1
      endif
      if %actor.quest_variable[megalith_quest:bad2]%
        set bad2 1
      endif
      if %actor.quest_variable[megalith_quest:bad3]%
        set bad3 1
      endif  
      if %actor.quest_variable[megalith_quest:bad4]%
        set bad4 1
      endif
      m_run_room_trig 12322
      wait 2s
      mecho %self.name% cries, 'Great Mother, come to me!'
      wait 2s
      mecho The chanting harmonizes with the vibrations of the standing stones.
      mecho &3&bRadiant energy washes out of the ancient stones in waves!&0
      wait 4s
      mecho &7&bStars &4begin to fall from the swirling vortex above.&0
      wait 5s
      *
      * The worst ending
      *
      if %bad4%
        mecho The energies of the megalith buckle suddenly as &5%get.obj_shortdesc[58426]%&0 shatters.
        wait 4s
        mecho &9&bThe harmonics shift into piercing discordant shrieks as&0 &1blood&0 &9&bbegins to pour from the altar!&0
        wait 2s
        mecho With a horrified look, %self.name% screams, 'The Great Invocation has been corrupted!  What was in that &1&belixir&0?!'
        wait 7s
        mecho Rays of &7&bs&0&3&bea&0&7&bring light&0 shoot out in all directions as the world spins!
        wait 6s
        mecho Suddenly &5bpitch black&0 energies erupt from the altar, engulfing %get.mob_shortdesc[12307]%!
        wait 8s  
        mecho %get.mob_shortdesc[12307]% is ripped apart from within in an explosion of blood and gore!
        mpurge celestial-envoy
        mecho Remaining in her place is a huge blood-covered abomination from beyond time and space made completely of iridescent black slime.
        wait 4s
        mecho Hundreds of eyes float just below the viscous membrane surrounding the ameboid creature.
        wait 5s
        mecho Suddenly it mutates into a huge creature with six enormous muscular legs and a massive maw!
        wait 3s
        mecho It roars with the voices of a thousand flayed beasts and attacks!
        mload mob 12321
        mforce shoggoth kill %actor.name%
        *
        * Clear all the bads, in case you try again later
        *
        set item 1
        while %item% <= 4
          quest variable megalith_quest %actor.name% bad%item% 0
          eval item %item% + 1
        done
        quest fail megalith_quest %actor.name%  
        halt
      else 
        eval total %bad1% + %bad2% + %bad3%
        *
        * The good ending - get two prizes
        *
        if %total% == 0 
          quest advance megalith_quest %actor.name%
          mecho &7&bCelestial light pours through the vortex.&0
          mecho &7&bIt forms a gentle &6pool of radiance &7in the eye of the cosmic maelstrom.&0
          wait 5s
          mecho &5&bA beautiful tiny being with six gossamer butterfly wings descends from the moonbridge.&0
          mecho &7&bShe looks as old as time itself yet younger than a child.&0
          wait 5s
          mecho %self.name% exclaims, 'O Great Mother, Goddess of the moon and stars, joyously we welcome you!'
          if %get.mob_count[12300]% == 0
            mload mob 12300
          endif
          mecho   
          mecho %self.name% and %get.mob_shortdesc[12307]% kneel before %get.mob_shortdesc[12300]%.
          wait 4s
          mecho %get.mob_shortdesc[12300]% embraces her children, weeping with joy.
          wait 5s
          mecho %get.mob_shortdesc[12300]% says, 'Your efforts shone a beacon in the dark, lighting my way back to you my children.'
          wait 4s
          mecho %get.mob_shortdesc[12300]% says, 'I have watched from afar as you and your mothers and your grandmothers struggled to keep our traditions alive.'
          wait 5s
          mecho %get.mob_shortdesc[12300]% proclaims, 'But now is our time to flourish again!'
          wait 2
          mecho %get.mob_shortdesc[12300]% proclaims, 'Let this sacred megalith be home to our coven from here after!'
          wait 5s
          mecho %get.mob_shortdesc[12300]% proclaims, 'Let us reignite the flames of passion, be the clay of reason, give the breath of life, be the ocean of serenity.'
          wait 5s
          mecho %get.mob_shortdesc[12300]% cries, 'So mote it be!'
          wait 3s
          mecho The coven cheers in unison, 'So mote it be!'
          wait 4s
          mechoaround %actor% Turning to %actor.name%, %get.mob_shortdesc[12300]% says, 'And you, %actor.name%.
          msend %actor% Turning to you, %get.mob_shortdesc[12300]% says, 'And you, %actor.name%.
          mecho &0I wish to thank you for helping my daughters.  &6&bKneel&0, and receive my blessing.'
          halt
        *
        * The okay ending - get one prize
        *
        elseif %total% == 1
          quest advance megalith_quest %actor.name%
          mecho The stars coalesce into the nearly transparent form of a celestial, gossamer-winged goddess.
          mecho %self.name% and %get.mob_shortdesc[12307]% bow before %get.mob_shortdesc[12300]%.
          mecho %self.name% says, 'O Great Mother, watcher from the deep, we come to seek thy blessing!'
          if %get.mob_count[12300]% == 0
            mload mob 12300
          endif
          mecho %get.mob_shortdesc[12300]% says, 'My daughters, you shall always have my blessing.'
          mecho %get.mob_shortdesc[12300]% gently kisses each member of the coven on their forehead, leaving a glowing mote of &b&7silvery light&0.
          wait 5s
          mecho %get.mob_shortdesc[12300]% says, 'Long has it been since the Nine Hells opened wide and I was cut off from the mortal world.'
          wait 3s
          mecho %get.mob_shortdesc[12300]% says, 'Yet through you and your mothers and grandmothers before you, my presence has been kept alive.'
          mecho %get.mob_shortdesc[12300]% smiles serenely.
          wait 6s
          mecho %get.mob_shortdesc[12300]% declares, 'Let this sacred megalith be home to our coven once again!  Thus I charge you with the tending and care of this holy monument.  May you keep well this sacred duty.'
          wait 7s
          mechoaround %actor% Turning to %actor.name%, %get.mob_shortdesc[12300]% says, 'And you, %actor.name%.
          msend %actor% Turning to you, %get.mob_shortdesc[12300]% says, 'And you, %actor.name%.
          mecho &0I would thank you for helping my daughters.  If you wish to receive my blessing, &6&bkneel&0.'
          halt
        *
        * The bad ending - get nothing
        *
        elseif %total% >= 2
          mecho &9&bThe harmonics of the monolith begin to shift, falling out of balance.&0
          wait 3s
          mecho The energy of the megalith begins to dim.
          mecho In a despondent voice, %self.name% cries out, 'The circle is failing!  &1&bThe tools or the reliquaries must have been flawed!&0'
          wait 4s
          mecho &3&bRapidly the radiance leaks out of the area.&0
          wait 2s
          mecho &7&bThe swirling vortex in the sky swiftly fades away.&0
          wait 4s
          mecho The Great Rite of Invocation ends as if nothing had ever happened.
          wait 4s
          mecho %self.name% says, 'So this is how it ends... not with a bang, but a whimper.'
          emote collapses, exhausted.
          *
          * Clear all the Bads, just in case you try again
          *
          set item 1
          while %item% <= 5
            quest variable megalith_quest %actor.name% bad%item% 0
            eval item %item% + 1
          done
          quest fail megalith_quest %actor.name%
        endif
      endif
  done
endif
~
#12312
megalith_quest_priestess_speech_progress~
0 d 1
status status? progress progress?~
wait 2
*make sure you're on the quest
if %actor.quest_stage[megalith_quest]% < 1
  return 0
  halt
endif
if %actor.has_completed[megalith_quest]%
  say We give thanks for your help in completing our summoning ritual.
  wait 2
  say Blessed be!
  halt
endif
if %actor.has_failed[megalith_quest]%
  say Sadly we failed in our efforts to invoke %get.mob_shortdesc[12300]%...
  wait 2
  say Would you like to try again?
  halt
endif
*clear variables
set item1 0
set item2 0
set item3 0
set item4 0
set receive1 0 
set receive2 0
set receive3 0
set receive4 0
set receive5 0
set stage %actor.quest_stage[megalith_quest]%
switch %stage%
  case 1
    set step replacing the sacred prophetic implements
* salt 23756
    set item1 salt
* Goblet or chalice 41110 or 41111 or 18512
    set item2 a goblet or chalice
* censer 8507 or 17300
    set item3 a censer
* candles 8612 or 58809
    set item4 candles
* give to the priestess
    set receive5 12301
    msend %actor% %self.name% says, 'I need your help %step%.'
  break
  case 2
*
* Must be done East South West North
*
    set step call the elements
* thin sheet of cloud to Keeper of the East
    set receive1 %get.mob_shortdesc[12305]%
    set item1 %get.obj_shortdesc[8301]%
* the fiery eye to Keeper of the South
    set receive2 %get.mob_shortdesc[12304]%
    set item2 %get.obj_shortdesc[48109]%
* granite ring to Keeper of the North
    set receive3 %get.mob_shortdesc[12303]%
    set item3 %get.obj_shortdesc[55020]%
* water from room 12463 to Keeper of the West
    set receive4 %get.mob_shortdesc[12306]%
    set item4 water from %get.obj_shortdesc[12352]%
    wait 2
    msend %actor% %self.name% says, 'We're trying to %step%.'
    if (%actor.quest_variable[megalith_quest:item1]%) &&(%actor.quest_variable[megalith_quest:item2]%) &&(%actor.quest_variable[megalith_quest:item3]%) &&(%actor.quest_variable[megalith_quest:item4]%)
      mecho %self.name% says, 'We're ready to finish calling the elements!
      mecho &0If you wish to proceed, repeat after me:'
      mecho &7&bUnder the watchful eye of Earth, Air, Fire, and Water, we awaken this
      mecho &0hallowed ground!&0
      halt
    endif
  break
  case 3
    set step locate three reliquaries
    set receive5 12301
    set item1 a holy prayer bowl
    set item2 a piece of a goddess's regalia
    set item3 a faerie relic from the land of the Reverie made manifest
  msend %actor% %self.name% says, 'We need to &3&b%step%&0.'
  break
  case 4
    mecho %self.name% says, 'I've been awaiting your return!  Let us
    mecho &0perform the Great Rite of Invocation! Repeat after me:'
    mecho &7&bGreat Lady of the Stars, hear our prayer!&0
    halt
  default
    return 0
    halt
done
* list items already given
if %actor.quest_variable[megalith_quest:item1]% /= 1 || %actor.quest_variable[megalith_quest:item2]% /= 1 || %actor.quest_variable[megalith_quest:item3]% /= 1 || %actor.quest_variable[megalith_quest:item4]% /= 1
  msend %actor%  
  msend %actor% You have already retrieved:
endif
if %actor.quest_variable[megalith_quest:item1]% /= 1
  msend %actor% - &7&b%item1%&0
endif
if %actor.quest_variable[megalith_quest:item2]% /= 1
  msend %actor% - &7&b%item2%&0
endif
if %actor.quest_variable[megalith_quest:item3]% /= 1
  msend %actor% - &7&b%item3%&0
endif
if %actor.quest_variable[megalith_quest:item4]% != 0
  msend %actor% - &7&b%item4%&0
endif
* list items to be returned
msend %actor%  
if %stage% != 4
  msend %actor% We still need:
endif
if %stage% == 2 &&%actor.quest_variable[megalith_quest:item1]% /= 0
  msend %actor% to assist &7&b%receive1%&0.
  If %actor.quest_variable[megalith_quest:east]% /= 1
    msend %actor% She needs %item1%
  else
    msend %actor% Please check with her to see what she needs.
  endif
msend %actor%  
elseif %actor.quest_variable[megalith_quest:item1]% /= 0
  msend %actor% &7&b%item1%&0
endif
if %stage% == 2 &&%actor.quest_variable[megalith_quest:item2]% /= 0
  msend %actor% to assist &1&b%receive2%&0.
  If %actor.quest_variable[megalith_quest:south]% /= 1
    msend %actor% She needs %item2%
  else
    msend %actor% Please check with her to see what she needs.
  endif
msend %actor%  
elseif %actor.quest_variable[megalith_quest:item2]% /= 0
  msend %actor% &7&b%item2%&0
endif
if %stage% == 2 &&%actor.quest_variable[megalith_quest:item3]% /= 0
  msend %actor% to assist &2&b%receive3%&0.
  If %actor.quest_variable[megalith_quest:north]% /= 1
    msend %actor% She needs %item3%
  else
    msend %actor% Please check with her to see what she needs.
  endif  
msend %actor%  
elseif %actor.quest_variable[megalith_quest:item3]% /= 0
   msend %actor% &7&b%item3%&0
endif
if %stage% == 2 &&%actor.quest_variable[megalith_quest:item4]% /= 0
  msend %actor% to assist &6&b%receive4%&0.
  If %actor.quest_variable[megalith_quest:west]% /= 1
    msend %actor% She needs %item4%
  else
    msend %actor% Please check with her to see what she needs.
  endif
elseif %stage% == 1 && %actor.quest_variable[megalith_quest:item4]% == 0
  msend %actor% &7&b%item4%&0
endif
msend %actor%   
if %receive5%
   msend %actor% %self.name% says, 'Please bring these to me.'
endif
~
#12313
megalith_quest_keeper_greet~
0 g 100
~
wait 2
if %actor.quest_stage[%type%_wand]% == %wandstep% && !%actor.has_failed[megalith_quest]%
  eval minlevel (%wandstep% - 1) * 10
  if %actor.level% >= %minlevel%
    if %actor.quest_variable[%type%_wand:greet]% == 0
      mecho %self.name% says, 'I see you're crafting something.  If you want my help, we can talk about &6&b[upgrades]&0.'
    else
      say Do you have what I need for the wand?
    endif
    wait 1s
  endif
endif
set stage %actor.quest_stage[megalith_quest]%
switch %self.vnum%
  *
  * North
  *
  case 12303
    if %actor.has_completed[megalith_quest]%
      cheer
      wait 1s
      say I can feel the Earth rejoicing from here to Templace!  Surely with Her blessing we can move to cleanse the ancient home of my people.
    elseif %actor.has_failed[megalith_quest]%
      emote drops to the ground.
      wait 1s
      say All hope is lost...  Without Her blessing, the demons will rampage forever.
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item4]% == 1)
      bow %actor.name%
      wait 1s
      say I thank you most humbly for your assistance with calling the Spirits of Earth.
    elseif %stage% > 2
      say Finally, our Mother Goddess will be able to walk our world again!
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item4]% /= 0) &&(%actor.quest_variable[megalith_quest:north]% /= 0)
      bow %actor.name%
      mecho %self.name% speaks in voice like a gentle breeze across sunlit grass, 'Are you here to help me summon the Spirits of Earth to protect our coven?'
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item4]% /= 0) &&(%actor.quest_variable[megalith_quest:north]% /= 1)
      say Do you have the granite ring?
    endif
    break    
  *
  * South
  *
  case 12304
    if %actor.has_completed[megalith_quest]%
      say This land is again full with the pulse, the pull, the bloodpump of creation...
      bow %actor.name%
      wait 1s
      say Thanks to you.
    elseif %actor.has_failed[megalith_quest]%
      mecho %self.name% begins to mutter to the standing stone, 'Destruction, greater than you or I can ever imagine...'
      wait 1s
      mecho %self.name% continues to mutter... 'A wave of protean fire curls around the planet and bares the world clean as bone...'
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item2]% == 1)
      nod %actor.name%
      wait 1s
      say I thank you most humbly for your assistance with calling the Spirits of Fire.
    elseif %stage% > 2
      mecho %self.name% whispers, 'Feel that?  That tingle in your veins?'
      wait 1s
      say That is the Lady of the Stars.  She approaches...
      mecho %self.name% takes a deep breath and exhales a cloud of &1&bs&0&1p&0&3&ba&0&1r&0&3k&0&3&bs&0!
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item2]% /= 0) &&(%actor.quest_variable[megalith_quest:south]% /= 0)
      nod %actor.name%
      say Hail traveler.
      wait 2
      mecho In a whisper like smokey ash %self.name% says, 'Are you here to help me summon the Spirits of Fire to protect our coven?'
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item2]% /= 0) &&(%actor.quest_variable[megalith_quest:south]% /= 1)
      sat I hope you brought the flaming jewel from Vulcera.
    endif
    break
  *
  * East
  *
  case 12305
    if %actor.has_completed[megalith_quest]%
      dance %actor.name%  
      wait 4
      say Finally we might be able to heal my poor family in Nordus!
      wait 3s
      say Can't you feel it?  The breath, the pull...
      emote exclaims in a ringing voice, 'Glory to - !!'
      laugh
    elseif %actor.has_failed[megalith_quest]%
      emote stands completely still, staring off into the void.
      Wait 10
      say Huh.
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item3]% == 1)
      wave %actor.name%
      wait 4
      say Thanks so much for your help with the Air Spirits!
    elseif %stage% > 2
      mecho %self.name% proclaims, 'Rejoice, the Great Work continues!  Soon the Faerie Goddess will be with us!'
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item3]% /= 0) &&(%actor.quest_variable[megalith_quest:east]% /= 0)
      wave %actor.name%
      wait 4
      mecho In a jubilant tone, %self.name% says, 'Hi there! Are you here to help me summon the Spirits of Air to protect our coven?'
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item3]% /= 0) &&(%actor.quest_variable[megalith_quest:east]% /= 1)
      wait 2
      say I hope you were able to recover that cloud bracelet!
    endif
    break    
  *
  * West
  *
  case 12306
    if %actor.has_completed[megalith_quest]%
      emote stifles tears of joy.
      wait 1s
      say I am overjoyed to see my Mother once again!  I can hear the rivers sing across Ethilien.  Our coven may be able to bring balance where the Dreaming flooded into the mortal realm at last.
    elseif %actor.has_failed[megalith_quest]%
      say A catastrophe...  As terrible as the night the Dream engulfed the Syric Mountains...
      Wait 2s
      emote begins to slip into a fugue.
      Wait 4s
      say I still remember that night...
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item4]% == 1)
      curtsy %actor.name%
      wait 2s
      say Spirits of Water overflow from the Dreaming into this realm, thanks to you!
    elseif %stage% > 2 
      say Thanks to you, soon the Great Mother will be able to come from the Deep Dreaming once more!
      hug %actor.name%
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item4]% /= 0) &&(%actor.quest_variable[megalith_quest:west]% /= 0)
      curtsy %actor.name%
      wait 1s
      say Merry met!
      wait 2s
      mecho %self.name% speaks in a silver voice, like bells reverberating over water, 'Are you here to help me summon the Spirits of Water to protect our coven?'
    elseif (%stage% == 2) &&(%actor.quest_variable[megalith_quest:item4]% /= 0) &&(%actor.quest_variable[megalith_quest:west]% /= 1)
      say Have you been able to bring me some water?
    endif
done
~
#12314
megalith_quest_keeper_speech~
0 dn 1
yes sure no I~
wait 2
if (%actor.quest_stage[megalith_quest]% == 2)
  if %speech% == yes || %speech% == sure || %speech% /= I will || %speech% /= I can
    switch %self.vnum%
      *
      * North - get ring from Tech
      *
      case 12303
        if %actor.quest_variable[megalith_quest:north]% == 0
          say Thank you very kindly.
          wait 5
          mecho %self.name% says, 'Far to the north, there is a civilization dedicated to the Great Snow Leopard.  There, they make &2rings of simple granite&0 which grant their constructs powerful mystic protection.  Such energies should make an ideal offering to the Earth Spirits.  Please retrieve one for me.'
          wait 6s
          mecho %self.name% says, 'Remember, we must call the elements in order:
          mecho &0&3&bEast&0, &1South&0, &6&bWest&0, &2&bNorth&0
          mecho &0so please start with &3&b%get.mob_shortdesc[12305]%&0.'
          bow %actor.name%
          quest variable megalith_quest %actor.name% north 1
          halt
        endif
        break
      *
      * South - Fiery Eye from Vulcera
      *
      case 12304
        If %actor.quest_variable[megalith_quest:south]% == 0
          say Excellent.
          wait 4
          mecho %self.name% says, 'There is an island in the sea where fires of the deep meet the outer world, ruled by a demigoddess.  My people know her magic to be the ultimate blending of divine spark and protean fire.  She carries &1&ba jewel that burns with that same fire&0.  If you can retrieve it, I believe that would be the perfect offering to the Spirits of Fire.'
          quest variable megalith_quest %actor.name% south 1
          wait 4s
          mecho %self.name% says, 'Remember, we must call the elements in order:
          mecho &0&3&bEast&0, &1South&0, &6&bWest&0, &2&bNorth&0
          mecho &0So please start with &3&b%get.mob_shortdesc[12305]%&0.'
          wait 2s
          nod %actor.name%
          say Good hunting.
          halt
        endif
        break
      *
      *  East - cumulus bracelet
      *
      case 12305
        if %actor.quest_variable[megalith_quest:east]%  == 0
          say Hurray!
          clap
          wait 1s
          say There are a handful of places in the world where the clouds reach from the ground to the heavens.  One such place is the home to an ancient silver dragon.
          wait 1s
          laugh
          say Don't worry, you don't need to mess with him on my behalf.
          wait 2s
          say His guardians on the other hand...
          grin
          wait 4s
          mecho %self.name% says, 'Some carry &7&bcumulus cloud bracelets&0.  If you can get your hands on one, it'll boost my call to the Air Spirits right through the stratosphere!'
          wait 6s
          mecho %self.name% says, 'Remember, we have to cast in order:
          mecho &0&0&3&bEast&0, &1South&0, &6&bWest&0, &2&bNorth&0
          mecho &0So when you return the offerings, please start with &3&bme&0!'
          quest variable megalith_quest %actor.name% east 1
          wait 5s
          say Why are you still standing here?!
          wait 2
          say Go!  Go!
          halt
        endif
        break
      *
      * West - Spring Water
      *
      case 12306
        if %actor.quest_variable[megalith_quest:west]% == 0
          say Wonderful!
          wait 2s
          say As a creature of the Dreaming, my magic is especially fueled by energies that thin the veil between the worlds.
          wait 3s
          mecho %self.name% says, 'My faerie sisters have told me there is a &6&bhidden spring in the surrounding forest&0.  Over the ages, it has absorbed the gentle radiance of the moon, making it ideally sympathetic with the power of the Reverie.  Even the waters from my home in the Kingdom of Dreams would pale in comparison!'
          wait 6s
          mecho %self.name% says, 'Please, take the cup you gave to the priestess, &6&bfill it with water from that spring&0, and bring it back with water for me to offer the Spirits of the West.'
          wait 6s
          ponder
          say Although this spring sounds ideal, I suppose any water will do.
          quest variable megalith_quest %actor.name% west 1
          wait 3s
          mecho %self.name% says, 'Remember, we must call the elements in order:
          mecho &0&3&bEast&0, &1South&0, &6&bWest&0, &2&bNorth&0
          mecho &0So please start with &3&b%get.mob_shortdesc[12305]%&0.'
          curtsy %actor.name%
          halt 
        endif
    done
  elseif %speech% == no
    wait 2
    shrug
    say Alright then.
  endif
endif
~
#12315
megalith_quest_keeper_receive~
0 j 100
55020 48109 8301 41110 41111 18512~
* This trigger is used to collect all the items for stage 2
set item %object.vnum%
set me %self.vnum%
switch %self.vnum%
  * North, Earth - Granite Ring from Tech
  case 12303
    set direction north
    set part 4
    set next 12301
    set need 55020
    break
  * South, Fire - Fiery Eye from Fiery Island
  case 12304
    set direction south
    set part 2
    set next 12306
    set need 48109
    break
  * East, Air - cumulus bracelet from Dargentan
  case 12305
    set direction east
    set part 1
    set next 12304
    set need 8301
    break
  * West, Water - water from the hidden spring, or not - BAD STUFF
  * This Keeper's request is the one to watch out for.
  case 12306
    set direction west
    set part 3
    set next 12303
    set need %actor.quest_variable[megalith_quest:goblet]%
done
if %actor.quest_stage[megalith_quest]% == 2 
  if (%item% == %need%) || (%me% == 12306 && (%item% == 41110 || %item% == 41111 || %item% == 18512))
    * Already given, right item
    if %actor.quest_variable[megalith_quest:item%part%]% == 1
      return 0
      shake
      say You've already given me this.
      halt
    elseif %actor.quest_variable[megalith_quest:%direction%]% == 1
      * Right item, wrong order
      if (%part% > 1) 
        eval check %part% - 1
        if (%actor.quest_variable[megalith_quest:item%check%]% == 0)
          return 0
          shake
          mecho %self.name% says, 'We must cast the circle in order:
          mecho East, South, West, North.
          mecho Please deliver your offerings in that order.'
          halt
        endif
      endif
      * Right set, wrong specific vessel at West
      if ((%me% == 12306) && ((%item% != %need%) || (%object.val2% != 0)))
        return 0
        shake
        say I need water returned in the cup %get.mob_shortdesc[12301]% has consecrated.
        halt
      * unique part of the incantation
      else
        wait 2
        quest variable megalith_quest %actor.name% %direction% 0
        quest variable megalith_quest %actor.name% item%part% 1
        if %direction% == north
          say Ah, excellent!  This will make a perfect offering.
          wait 6
          mecho %self.name% places %object.shortdesc% before the menhir and kneels.
          wait 2s
          mecho %self.name% chants:
          mecho 'Hail to the guardian Spirits of the North
          mecho &0Spirits of Earth and Determination
        elseif %direction% == south
          say Marvelous!  This will make a perfect offering.
          wait 6
          mecho %self.name% raises %object.shortdesc% above her head.
          wait 2s
          mecho %self.name% chants:
          mecho 'Hail to the guardian Spirits of the South
          mecho &0Spirits of Fire and Feeling
        elseif %direction% == east
          say Great!  This will make a perfect offering.
          wait 6
          mecho %self.name% shakes out %object.shortdesc% like a blanket.
          mecho She watches with delight as it dissolves into wisps of cloudy mist.
          wait 4s
          mecho %self.name% chants:
          mecho 'Hail to the guardian Spirits of the East
          mecho &0Spirits of Air and Intellect
        elseif %direction% == west
          if %actor.quest_variable[megalith_quest:item5]% == 1
            say Ah, delightful!  This water will make a perfect offering.
            wait 6
          else
            say This will do.
            quest variable megalith_quest %actor.name% bad2 1          
            wait 6
          endif
          mecho %self.name% pours the water from %object.shortdesc% before the menhir.
          wait 2s
          mecho %self.name% chants:
          mecho 'Hail to the guardian Spirits of the West
          mecho &0Spirits of Water and Intuition
        endif
        mecho &0I present this offering to you.
        mecho &0Aid us in our magical working on this great day.'
        wait 5s
        mjunk %object.name%
        mecho The menhir begins to hum and glow!
        wait 3s
        * Next person
        if (%actor.quest_variable[megalith_quest:item1]%) && (%actor.quest_variable[megalith_quest:item2]%) &&(%actor.quest_variable[megalith_quest:item3]%) &&(%actor.quest_variable[megalith_quest:item4]%)
          say Now, return to %get.mob_shortdesc[12301]% at the altar and help her finish the call!
        else
          say Now, proceed to %get.mob_shortdesc[%next%]% and continue invoking the elements.
        endif
      endif
    endif
  endif
endif
~
#12316
megalith_quest_act_fill~
1 c 3
fill~
* Questor should fill a vessel from room 12401
* Doing so will set item5 if the questor is on stage 2
*
switch %cmd%
  case f
  case fi
    return 0
    halt
done
if (%actor.quest_stage[megalith_quest]% == 2) && (%actor.room% == 12401) && (%actor.quest_variable[megalith_quest:goblet]% == %self.vnum%)
  oforce %actor.name% fill %arg%
  quest variable megalith_quest %actor.name% item5 1
else
  return 0
endif
~
#12317
megalith_quest_ritual_effects1~
2 a 0
~
wait 2
wzoneecho 123 The Keepers call out, 'Great Lady of the Stars, hear our prayer!'
wait 2
wzoneecho 123 The voices of the coven call out in unison, 'Great Lady of the Stars, hear our prayer!'
~
#12318
megalith_quest_ritual_effects2~
2 a 0
~
wait 2
wzoneecho 123 The coven chants, 'We summon and stir thee!'
wait 1s
wzoneecho 123 &2The harmonic rumbling intensifies as the ground starts to shake as if awakening.&0
~
#12319
megalith_quest_ritual_effects3~
2 a 100
~
wait 2
wzoneecho 123 &7&bBeams of &3light &7shoot up from the menhirs meet and unite above the altar.&0&_
wzoneecho 123 &4&bThe fabric of the sky &3rips &4open like a flame burning through paper.&0
wait 4s
wecho %get.mob_shortdesc[12301]% chants: 
wecho 'Ringing now the bell in three
wecho Hear our prayer.'
wait 3s
wecho %get.mob_shortdesc[12301]% says, 'Chant it with us!'&_
wecho '&6&bWe summon and stir thee&0!'
~
#12320
megalith_quest_ritual_effects4~
2 a 100
~
wait 2
wzoneecho 123 The coven chants, 'We summon and stir thee!'
~
#12321
megalith_quest_ritual_effects5~
2 a 100
~
wait 2
wzoneecho 123 &4&bThe rift &3in the sky begins&4 to churn and swirl.&0&_
wzoneecho 123 &4&bMillions of &3tiny stars &4and &6alien moons &4twinkle in the depths of the vortex.&0
wait 4s
wecho %get.mob_shortdesc[12301]% chants: 
wecho 'Ringing now the bell in three
wecho Hear our prayer.'
wait 3s
wecho %get.mob_shortdesc[12301]% says, 'Chant it with us!'&_
wecho '&6&bWe summon and stir thee&0!'
~
#12322
megalith_quest_ritual_effects6~
2 a 100
~
wait 2
wzoneecho 123 The voices of the coven cry out, 'We invoke thee!'
~
#12323
megalith_quest_mother_act_kneel_rewards~
0 c 100
kneel~
return 0
switch %cmd%
  case k
    halt
done
wait 2
if %actor.quest_variable[megalith_quest:bad1]% == 1
  set bad1 1
endif
if %actor.quest_variable[megalith_quest:bad2]% == 1
  set bad2 1
endif
if %actor.quest_variable[megalith_quest:bad3]% == 1
  set bad3 1
endif  
if %actor.quest_stage[megalith_quest]% == 5
  eval total %bad1% + %bad2% + %bad3%
  if %total% /= 0
    msend %actor% %self.name% gently leans forward and kisses your brow.
    mechoaround %actor% %self.name% gently leans forward and kisses %actor.name%'s brow.
    wait 2s
    mecho %self.name% says, 'I bestow upon you the gifts of stars.  May their light
    mecho &0guide you the rest of your days.'
    mload obj 12398
    mload obj 12399
    give belt-stars %actor%
    give starseed %actor%
    set gem 0
    while %gem% < 3
       eval drop %random.11% + 55736
       mload obj %drop%
       eval gem %gem% + 1
    done
    give all.gem %actor.name%
  elseif %total% > 0
    msend %actor% %self.name% places her hand on your shoulder.
    mechoaround %actor% %self.name% places her hand on %actor.name%'s shoulder.
    wait 2s
    mecho %self.name% says, 'I bestow upon you a gift of stars.  May its light
    mecho &0guide you the rest of your days.'
    set random %random.2%
    if %random% == 1
      mload obj 12398
    elseif %random% == 2
      mload obj 12399    
    endif
    set gem 0
    while %gem% < 3
       eval drop %random.11% + 55736
       mload obj %drop%
       eval gem %gem% + 1
    done
    give all %actor.name%
    wait 4
    bow %actor.name%
  endif
*
* Clear all the Bads, just in case
*
  set item 1
  while %item% <= 5
    quest variable megalith_quest %actor.name% bad%item% 0
    unset bad%item%
    eval item %item% + 1
  done
  quest complete megalith_quest %actor.name%
*
* Set X to the level of the award - code does not run without it
* 
  if %actor.level% < 70
    set expcap %actor.level%
  else
    set expcap 70
  endif
  set expmod 0
      if %expcap% < 9
         eval expmod (((%expcap% * %expcap%) + %expcap%) / 2) * 55
      elseif %expcap% < 17
         eval expmod 440 + ((%expcap% - 8) * 125)
      elseif %expcap% < 25
         eval expmod 1440 + ((%expcap% - 16) * 175)
      elseif %expcap% < 34
         eval expmod 2840 + ((%expcap% - 24) * 225)
      elseif %expcap% < 49
         eval expmod 4640 + ((%expcap% - 32) * 250)
      elseif %expcap% < 90
         eval expmod 8640 + ((%expcap% - 48) * 300)
      else
         eval expmod 20940 + ((%expcap% - 89) * 600)
      endif
*
* Adjust exp award by class so all classes receive the same proportionate amount
*
  switch %actor.class%
    case Warrior
    case Berserker
    *
    * 110% of standard
    * 
        eval expmod (%expmod% + (%expmod% / 10))
        break
    case Paladin
    case Anti-Paladin
    case Ranger
    *
    * 115% of standard
    *
        eval expmod (%expmod% + ((%expmod% * 2) / 15)
        break
    case Sorcerer
    case Pyromancer
    case Cryomancer
    case Illusionist
    case Bard
    *
    * 120% of standard
    *
        eval expmod (%expmod% + (%expmod% / 5))
        break
    case Necromancer
    case Monk
    *
    * 130% of standard
    *
        eval expmod (%expmod% + (%expmod% * 2) / 5)
        break
    default
        set expmod %expmod%
  done
  msend %actor% &3&bYou gain experience!&0
  eval setexp (%expmod% * 10)
  set loop 0
  while %loop% < 10
  *
  * Xexp must be replaced by mexp, oexp, or wexp for this code to work
  * Pick depending on what is running the trigger
  *
     mexp %actor% %setexp%
     eval loop %loop% + 1
  done
endif
~
#12324
vityaz_random_spark~
0 ab 0
~
say hi
~
#12325
vityaz_random_move~
0 ab 40
~
* Pretty fluff.  Makes sparks.  Pure aesthetics.
*
set rndm %random.5%
if %rndm% == 1
  mecho %get.mob_shortdesc[12302]% utters a few arcane words in the old Tzigane tongue. 
elseif %rndm% == 2
  if %self.wearing[12308]%
    mecho &b&7S&0&3&bP&0&6&bA&0&3&bR&0&7&bK&0&3&bS&0 fly as %get.mob_shortdesc[12302]% trails %get.obj_shortdesc[12308]% along the ground.
  endif
elseif %rndm% >= 3
  switch %self.room%
    case 12347
      south
      halt 
    break
    case 12348
      south
      halt
    break
    case 12363
      south
      halt
    break
    case 12358
      south
      halt
    break
    case 12345
      south
      halt
    break
    case 12346
      south
      halt
    break
    case 12349
      west
      halt
    break
    case 12364
      west 
      halt
    break
    case 12378
      west
      halt
    break
    case 12392
      west
      halt
    break
    case 12412
      west
      halt
    break
    case 12426
      west
      halt
    break
    case 12427
      north
      halt
    break
    case 12441
      north
      halt
    break
    case 12440
      north
      halt
    break
    case 12439
      north
      halt
    break
    case 12438
      north
      halt
    break
    case 12422
      north
      halt
    break
    case 12437
      east
      halt
    break
    case 12421
      east
      halt
    break
    case 12406
      east
      halt
    break
    case 12386
      east
      halt
    break
    case 12372
      east
      halt
    break
    case 12359
      east
      halt
    break
  default
    return 0
  done
endif
~
#12326
witch_priestess_exposition~
0 dn 1
who continue~
if (%speech% /= who are you || %speech% /= continue) && %actor.vnum% == -1
  wait 2
  if %actor.quest_variable[megalith_quest:hexpo]% == 0 
    say I am the high priestess of a sacred sisterhood of witches dedicated to the worship of the faerie goddess the Great Mother, Lady of Stars.
    wait 4s
    say We strive to keep the Old Ways alive, as our foremothers did, and their foremothers before them, and their foremothers before them, back through the centuries.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% hexpo 1
  elseif %actor.quest_variable[megalith_quest:hexpo]% == 1
    say Witchcraft has always been in my blood.
    wait 2s
    say The people of my homeland in the Green Green Sea still observe the Old Ways.  The Fair Folk still walk among us.  A bocan hobgoblin or two playing tricks on you is part of our daily routine.  Every woman in the village knows how to mix herbs and what to do when you see a ghost.
    wait 4s
    say I was, and still am, nothing special.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% hexpo 2
  elseif %actor.quest_variable[megalith_quest:hexpo]% == 2
    say Then I heard the call.
    wait 2s
    say Nightly dreams led me on a global pilgrimage.  I found myself seeking out clandestine lore and lost secrets from a myriad of other traditions.
    wait 4s
    say I trained under the Priests of Mielikki.
    wait 3s
    say I was honored with a meeting for tea with the Hierophant of the Highlands.
    wait 3s
    say I've read the texts of Seblan, and the Enchiridion.
    wait 3s
    say I even learned a thing or two from the lizard shamans in the Northern Swamps.
    wait 3s
    say And after many years, I returned home where the Great Mother revealed herself to me.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% hexpo 3
  elseif %actor.quest_variable[megalith_quest:hexpo]% == 3
    say She charged me with seeking out this coven.  I dedicated myself to Her service and through years of loving care and gentle insight, I have become Her priestess as we push to revitalize the Old Ways.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% hexpo 4
  elseif %actor.quest_variable[megalith_quest:hexpo]% == 4
    say Like myself, my Sisters hail from every corner of Ethilien.
    wait 2s
    say Some are just women called to our coven to provide succor to others.
    wait 3s
    say Others have more... unusual stories which have led them to the Faerie Mother.
    wait 3s
    mecho %self.name% says, 'Yet today, we have all come to this place to &b&6perform the Great Rite of Invocation.&0'
    quest variable megalith_quest %actor% hexpo 0 
  endif
endif
~
#12327
manticore_fight~
0 k 5
~
wait 2
if %actor% &&(%actor.room% == %self.room%) && (%actor.vnum% == -1)
  eval hit (%self.hitroll% + %random.100%) + (%actor.armor% + ((%actor.real_dex% / 2) - 24))
  if %hit% >= 0
    mecho A manticore lashes out with its tail!
    mcast poison %actor% %self.level%
  else 
    mechoaround %actor% %self.name% misses %actor.name% with a powerful thrust of its tail!
    msend %actor% %self.name% misses you with a powerful thrust of its tail!
  endif
endif
~
#12328
gorgon_fight~
0 k 1
~
Wait 2
mecho A gorgon exhales a cloud of &6paralyzing gas!&0
set room %self.room%
set person %room.people%
while %person%
   if %person.vnum% == -1
      mcast 'major paralysis' %person% 2
   endif
   set person %person.next_in_room%
done
~
#12329
cave_open~
2 ab 100
~
if %time.hour% > 19 || %time.hour% < 5
  if %self.south% != 12401
    wdoor 12399 south room 12401
    wecho Gentle moonlight begins to seep through cracks in the wall of the cave, revealing an exit to the south.
  endif
elseif %time.hour% > 5 && %time.hour% < 19
  if %self.south% == 12401
    wdoor 12399 south purge
    wecho As the sun rises, the exit to the forest beyond the cave fades from view.
  endif
endif
~
#12330
spring_vanish~
2 b 100
~
if %time.hour% > 5 && %time.hour% < 19
if %self.people% != 0
    wecho As the moon sets, the world shifts and the spring fades from view.
    wteleport all 12400
  endif
endif
~
#12331
faerie_dragon_fight~
0 k 20
~
Wait 2
mecho A faerie dragon exhales a cloud of &1e&3&bu&1p&2h&4o&6r&5i&0&1c&0 gas!
set room %self.room%
set person %room.people%
while %person%
   if %person.vnum% == -1
      mcast confusion %person% %self.level%
   endif
   set person %person.next_in_room%
done
~
#12332
aggro_good~
0 g 100
~
if %actor.align% >= 350 && %actor.level% < 100 && %actor.vnum% == -1
wait 4
kill %actor.name%
endif
~
#12333
sunblade_wear~
1 j 100
~
oecho A brilliant blade of &3&bsunlight&0 erupts from the pommel!
~
#12334
moonblade_wear~
1 j 100
~
oecho An ephemeral blade of glittering &6&bmoonlight&0 pours from the pommel!
~
#12335
faerie-dragon-death~
0 f 100
~
mjunk all.breath
~
#12336
vityaz_exposition~
0 dn 100
who continue~
if (%speech% /= who are you || %speech% /= continue) && %actor.vnum% == -1
  wait 2
  if %actor.quest_variable[megalith_quest:vexpo]% == 0 
    say I am a sorcerer-warrior of the Tzigane people.  Though others call us by a cruder name, you've certainly seen us around the world before.  I myself come from a family caravan which is usually camped not far to the east of here.
    wait 4s
    say When I was a girl, the old witchy woman of my caravan saw mystic potential in me.  She took me under her wing and taught me to utilize that potential to defend my family and our home.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% vexpo 1
  elseif %actor.quest_variable[megalith_quest:vexpo]% == 1
    say Shortly after I came of age, I heard a voice calling to me in my dreams.  It told me new Sisters were waiting for me, in need of my defense.
    wait 4s
    say I came upon several sisters of the coven besieged by the walking dead outside the graveyard near Anduin.  My added strength turned the tide of battle.
    wait 5s
    say They styled me "Vityaz," meaning "Hero" in the old tongue of my people, and I have guarded them ever since.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% vexpo 2
  elseif %actor.quest_variable[megalith_quest:vexpo]% == 2
    say To assist in the Great Rite of Invocation, in my duty duty as defender, I am casting and reinforcing a circle of protection between these smaller menhir.
    wait 4s
    say I will maintain it throughout the rite.
    quest variable megalith_quest %actor% vexpo 0
  endif
endif
~
#12337
north_exposition~
0 dn 100
who continue~
if (%speech% /= who are you || %speech% /= continue) && %actor.vnum% == -1
  wait 2
  if %actor.quest_variable[megalith_quest:nexpo]% == 0 
    say After Sagece's siege on our ancestral home, my family sought refuge with our frost relatives near the land of the Great Snow Leopard.
    wait 3s
    say The Children of the Snow Leopard instilled a love of stone in me.  Not just a love of magical stone, their enchanted jades or mystic crystals, but mundane stone too, because it so often is more than meets the eye.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% nexpo 1
  elseif %actor.quest_variable[megalith_quest:nexpo]% == 1 
    say As it turns out, that love was an extension of my natural affinity for Earth and nature.
    wait 2s
    say When I was a young woman of about 140 or so, I joined an order of arctic druids.  We worshiped in the Old Ways, paying respect to the cosmos from before the New Gods.
    wait 4s
    say In the Old Ways, I heard the voices of my ancestors.
    wait 3s
    say In the Old Ways, I heard the voice of my ancient Mother.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% nexpo 2
  elseif %actor.quest_variable[megalith_quest:nexpo]% == 2
    say She revealed to me that, just as we elves and the Fair Folk once shared a home in the Dreaming, our various traditions have common roots as well.
    wait 5s
    say Druidry and witchcraft are the same traditions, just viewed through different lenses.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% nexpo 3
  elseif %actor.quest_variable[megalith_quest:nexpo]% == 3 
    say After many years, the Great Mother called upon me to find this Sisterhood.
    wait 2s
    say I believe She is guiding me to bring peace to my elven ancestors by bringing Her back to this realm and, with the aid of my Sisters, cleansing the demonic corruption of Templace.
    wait 4s
    say By growing up in a valley of frost and snow, I learned even the coldest winter is part of the natural cycle.  Even in the coldest places, spring eventually comes and the world is reborn.
    wait 5s
    say For beneath it all, the Earth rests.  Timeless.
    wait 1s 
    say Just like I am.
    quest variable megalith_quest %actor% nexpo 0
  endif
endif
~
#12338
south_exposition~
0 dn 100
who continue~
if (%speech% /= who are you || %speech% /= continue) && %actor.vnum% == -1
  wait 2
  if %actor.quest_variable[megalith_quest:sexpo]% == 0 
    say I am Keeper of the South, the warden of fire.  As befits my station, I hail from an island where molten fire reigns supreme.  It flows from the mountain, touches the sky, and meets the ocean.
    wait 6s
    say When murderous "adventurers" come from the Three Cities to loot my homeland, they see a hostile land subjugated by a petty and cruel demigoddess where the people live without stone monuments for homes.
    wait 6s
    say They, marauding lunatics that they are, call us "savage" and "uncivilized".  They think us inferior for living in the ways of the land and not claiming dominion over the volcano.
    wait 5s
    say That the volcano goddess was once beloved by the God of the Moonless Night - 
    mecho &0 
    emote %self.name% makes a face and spits on the ground and utters,
    say - may he ever suffer terrible vengeance - 
    mecho &0 
    mecho %self.name% continues, ' - does not help either.'
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% sexpo 1
  elseif %actor.quest_variable[megalith_quest:sexpo]% == 1 
    say But living in the ways of the jungle does not make us "savage".  To my Jungle Tribe, we cannot stop Vulcera or the volcano, for it is the natural ebb and flow of creation.
    wait 6s
    say I spent several years apprenticed to the old island shaman learning not how to prevent this process, but how to shape it.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% sexpo 2
  elseif %actor.quest_variable[megalith_quest:sexpo]% == 2
    say It was during those years I first felt the primordial churn in the stellar inferno of the cosmos.  Just like within the fires of the volcano, I could feel the pulse of creation beating in the heart of each star.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% sexpo 3
  elseif %actor.quest_variable[megalith_quest:sexpo]% == 3 
    say One summer, several Sisters of the coven stopped at my village on their way to seek guidance from the old wise woman of the Mountain Tribe.
    wait 5s
    say I expected they would bring nothing but death and belittlement like all outsiders do.
    wait 2s
    say But when they expressed great concern for the volcano and the creatures within, I was greatly surprised.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% sexpo 4
  elseif %actor.quest_variable[megalith_quest:sexpo]% == 4
    say They became welcomed guests and gradually friends.  They returned many times, each time asking me to join them, calling me Sister in the Old Ways.  After many meetings, I finally relented and left my home for the first time.
    wait 6s
    say I was reluctant at first, but I have come to see we walk the same paths as keepers of the land.  The sacred duties we share unite us in purpose beyond blood - we are truly Sisters.
    quest variable megalith_quest %actor% sexpo 0
  endif
endif
~
#12339
east_exposition~
0 dn 100
who continue~
if (%speech% /= who are you || %speech% /= continue) && %actor.vnum% == -1
  wait 2
  if %actor.quest_variable[megalith_quest:eexpo]% == 0
    say Who am I?  Are you sure you want to know?
    emote looks around slightly confused.
    wait 4s
    say Well, okay!  I come from the "Fourth City", Nordus.  Ever heard of it?  We used to be known for -- 
    wait 2
    mecho %self.name% suddenly lets out a blood curdling scream!
    wait 3s
    mecho She continues to scream.
    wait 3s
    Mecho And scream.
    wait 3s
    mecho And scream.
    wait 4s
    mecho %self.name% quickly falls silent.
    mecho %self.name% looks around herself, blinking.
    wait 4s
    say I'm sorry, did something weird just happen?  If you've ever been to Nordus, you may have seen stuff like that before...
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% eexpo 1
  elseif %actor.quest_variable[megalith_quest:eexpo]% == 1
    say I was an acolyte under the most respected cleric in Nordus, the great Shema.
    wait 4s
    say Shema was brilliant.  She loved languages and codes, though her favorite magic password was just spelling her name backward!
    wait 5s
    say She talked about all sorts of languages, like ones called "Gir-main" and "Fence".
    laugh
    wait 6s
    say Have you ever heard of anything that ridiculous?!  A talking fence?!  Talking front doors, sure, but a fence??  Come on!!
    rofl
    wait 2
    emote wipes a tear from her eye.
    mecho %self.name% says, 'Aaaahhh Shema, good times...  Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% eexpo 2
  elseif %actor.quest_variable[megalith_quest:eexpo]% == 2
    say Anyway, when Luchiaans arrived in Nordus and the "sickness" started, Shema was the only one with enough wit to suspect him.  There was a book she -- 
    wait 1s
    emote suddenly focuses intently on a vacant space, as if listening to someone.
    wait 4s
    say Yes Umberto, the only one besides you.
    wait 4s
    mecho %self.name% whispers under her breath, 'He doesn't like to be left out...  It makes him feel invisible.'
    wait 3s
    sat Point is, after he arrived, things changed.  For example, I met Umberto here!'
    wait 3s
    emote points at the empty space next to her.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% eexpo 3
  elseif %actor.quest_variable[megalith_quest:eexpo]% == 3
    say Turns out, Luchiaans had been conducting necromantic experiments on us while looking for a powerful tome about healing the body beyond death hidden in Nordus.
    wait 5s
    say Shema caught on to his plot, so she warded the book with her favorite code, bundled up a few of us and hit the road!
    wait 4s
    say We had plenty of adventures before Shema finally kicked the bucket a few years back.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% eexpo 4
  elseif %actor.quest_variable[megalith_quest:eexpo]% == 4
    say We eventually discovered we carried a part of Nordus within us everywhere we went.
    wait 3s
    say Luchiaans' experimentations had permanently changed us.  Our magics never really worked the same way again.
    wait 3s
    say I started... hearing things...  Whispers in the dark, voices when no one was around...
    wait 3s
    say At first I thought I was finally succumbing to the madness of Nordus, but then I was visited with a revelation from the stars.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% eexpo 5
  elseif %actor.quest_variable[megalith_quest:eexpo]% == 5
    say Luchiaans' meddling had somehow opened my mind to a deeper consciousness.  By total accident, I connected to the Lady of the Stars on the other side of the Veil!
    wait 5s
    say She explained the voices I was hearing was the wind calling me to dance.
    wait 5s
    say Unfit for service to the New Gods anymore, I followed the guidance of our Lady until I ran right into the coven.
    wait 5s
    say Literally, I ran face-first into %get.mob_shortdesc[12303]%!
    laugh
    wait 4s
    say Suddenly everything made sense.  I had found my purpose and a new family.  Umberto and I have been part of the coven ever since.
    quest variable megalith_quest %actor% eexpo 0
  endif
endif
~
#12340
west_exposition~
0 dn 100
who continue~
if (%speech% /= who are you || %speech% /= continue) && %actor.vnum% == -1
  wait 2
  if %actor.quest_variable[megalith_quest:wexpo]% == 0
    say My home realm, the Reverie, the Dreaming, is a plane of the dreams of mortals made manifest.
    wait 3s
    say Unlike most of the other fey creatures in Ethilien, I am not in this world by choice.
    wait 3s
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% wexpo 1
  elseif %actor.quest_variable[megalith_quest:wexpo]% == 1
    say I am also unique from my coven Sisters because I knew our Mother.  I was shaped by Her hand directly.  Reaching out from the deepest parts of the Dreaming, The Great Mother formed me from a river in part of the Reverie ruled by a creature known as the Goblin King.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% wexpo 2
  elseif %actor.quest_variable[megalith_quest:wexpo]% == 2
    say Driven by an insatiable hunger for power and wealth, the Goblin King grew tired of being confined to the Dreaming.
    wait 4s
    say So, he approached the troll witch Baba Yaga.  Together they concocted a way to pierce the veil between the world and merge his court with the mortal realm.
    wait 4s
    say Not caring what the consequences might be, they tore the Goblin King's land from the Reverie and smashed it into the Syric Mountains.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% wexpo 3
  elseif %actor.quest_variable[megalith_quest:wexpo]% == 3
    say In the chaos that ensued, I could still hear my Mother's voice.  She whispered to me that there was a new world waiting beyond the Dreaming and She would be there, with a new family if I dared to dream it.
    wait 4s
    say I chose to leave.
    wait 2
    mecho %self.name% says, 'Say &6&bcontinue&0 when you want to hear more.'
    quest variable megalith_quest %actor% wexpo 4
  elseif %actor.quest_variable[megalith_quest:wexpo]% == 4
    say The Great Mother guided me to the coven, who welcomed me with open arms and introduced this world to me.
    wait 3s
    say Being among mortals, meeting the long-descended elven children of my ancient siblings, climbing the rocky peaks, dancing in the golden pastures of Ethilien...
    wait 5s
    say I have awakened to a splendor I feared I would lose forever, with a kind of family I never had.
    wait 3s
    say She had taken me from one Dream and set me free in another.
    quest variable megalith_quest %actor% wexpo 0
  endif
endif
~
#12341
witch_exposition~
0 dn 100
who continue~
if (%speech% /= who are you || %speech% /= continue) && %actor.vnum% == -1
  switch %self.vnum%
    case 12309
      switch %random.4% 
        case 1
          say No one special, merely a Sister in sacred trust.
          break
        case 2
          say Just a peasant from Ickle.
          break
        case 3
          mecho %self.name% says, 'That is an excellent question...
          mecho &0Who are any of us?'
          ponder
          break
        case 4
        default
          say I'm quite busy right now, that's who I am.
      done
      halt
      break
    case 12310
      switch %random.4% 
        case 1
          say No one special, merely a Sister in sacred trust.
          break
        case 2
          say Just a peasant from Mielikki.
          break
        case 3
          mecho %self.name% says, 'That is an excellent question...
          mecho &0Who are any of us?'
          ponder
          break
        case 4
        default
          say I'm quite busy right now, that's who I am.
      done
      halt
      break
    case 12324
      switch %random.4% 
        case 1
          say No one special, merely a Sister in sacred trust.
          break
        case 2
          say Just a peasant from Anduin.
          break
        case 3
          mecho %self.name% says, 'That is an excellent question...
          mecho &0Who are any of us?'
          ponder
          break
        case 4
        default
          say I'm quite busy right now, that's who I am.
      done
  done
endif
~
#12342
Keeper refuse~
0 j 100
~
switch %self.vnum%
  case 12303
    set direction north
    set part 4
    set need 55020
    break
  case 12304
    set direction south
    set part 2
    set need 48109
    break
  case 12305
    set direction east
    set part 1
    set need 8301
    break
  case 12306
    set direction west
    set part 3
    set need %actor.quest_variable[megalith_quest:goblet]%
done
switch %object.vnum%
  case %wandgem%
  case %wandtask3%
  case %wandtask4%
  case %wandvnum%
    halt
    break
  default
    if %actor.quest_stage[megalith_quest]% < 2
      return 0
      shake
      mecho %self.name% refuses %object.shortdesc%.
      wait 2
      say I'm sorry, I'm quite busy right now.
    elseif %actor.quest_stage[megalith_quest]% > 2 || (%actor.quest_stage[megalith_quest]% == 2 && %actor.quest_variable[megalith_quest:item%part%]% == 1)
      return 0
      shake
      say I don't need any additional assistance at the moment, thank you.
      if %self.vnum% == 12306
        curtsy %actor.name%
      else
        bow %actor.name%
      endif
    elseif %actor.quest_stage[megalith_quest]% == 2
      if (%actor.quest_variable[megalith_quest:%direction%]% == 0) && (!%actor.quest_variable[megalith_quest:item%part%]%)
        return 0
        if %self.vnum% == 12303
          eye %actor.name%
          say I haven't yet told you what I need.
        elseif %self.vnum% == 12304
          sigh
          say You have yet to receive instructions.  A little patience, please!
        elseif %self.vnum% == 12305
          emote yelps in surprise.
          say Did Umberto tell you what I need already?!
        elseif %self.vnum% == 12306
          emote looks confused.
          say I have yet to tell you what I need.
        endif
      else
        if (%self.vnum% == 12303 && %object.vnum% == 55020) || (%self.vnum% == 12304 && %object.vnum% == 48109) || (%self.vnum% == 12305 && %object.vnum% == 8301) || (%self.vnum% == 12306 && ((%object.vnum% == 41110) || (%object.vnum% == 41111) || (%object.vnum% == 18512)))
          halt
        else
          return 0
          shake
          mecho %self.name% refuses %object.shortdesc%.
          wait 2
          if %self.vnum% == 12306
            say Unfortunately, I need %get.obj_shortdesc[%need%]% filled with water, not this.
          else
            say Unfortunately, I need %get.obj_shortdesc[%need%]%, not this.
          endif
        endif
      endif
    endif
done
~
#12343
megalith_priestess_refuse~
0 j 100
~
set stage %actor.quest_stage[megalith_quest]%
if %stage% != 1 && %stage% != 3
  return 0
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  set response I need your help with something else.
elseif %stage% == 1
  switch %object.vnum%
    case 23756
    case 41111
    case 41110
    case 18512
    case 8507
    case 17300
    case 8612
    case 58809
      halt
      break
    default
      set response Unfortunately, that's not going to help empower the ritual.
  done
elseif %stage% == 3
  switch %object.vnum%
    case 23817
    case 4305
    case 4318
    case 58015
    case 58018
    case 58426
    case 58418
      halt
      break
    default
      set response Unfortunately, that's not going to help empower the ritual.
  done
endif
if %response%
  return 0
  shake
  mecho %self.name% refuses %object.shortdesc%.
  wait 2
  say %response%
endif
~
#12398
**UNUSED**~
0 c 100
kn kne knee~
return 0
~
#12399
menhir_purge~
2 b 100
~
if %self.objects[12350]% && %self.objects[12351]%
  wpurge awakened-menhir
  wecho %get.obj_shortdesc[12350]% gradually stops glowing and falls silent.
endif
~
$~
