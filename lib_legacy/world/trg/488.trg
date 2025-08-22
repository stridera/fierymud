#48801
shrynn fight~
0 k 100
~
set mode %random.10%
if %mode% < 7
   wait 2s
   if %actor% && (%actor.room% == %self.room%) && (%actor.vnum% == -1)
      set max_tries 5
      while %max_tries% > 0
         set victim %random.char%
         if (%victim.vnum% == -1) && (%victim.name% != %actor.name%)
            set okay 1
            set max_tries 0
         endif
         eval max_tries %max_tries% - 1
      done
      if !%okay%
         halt
      endif
      eval actor_damage 310 + %random.100%
      if %actor.aff_flagged[SANCT]%
         eval actor_damage %actor_damage% / 2
      endif
      eval victim_damage 150 + %random.100%
      if %victim.aff_flagged[SANCT]%
         eval victim_damage %victim_damage% / 2
      endif
      msend %actor% %self.name% sucks you into his vortex, spinning you around in a blur!
      mechoaround %actor% %self.name% sucks %actor.name% into a vortex, spinning %actor.o% around vigorously!
      mdamage %actor% %actor_damage% crush
      eval actor_damage %damdone%
      mdamage %victim% %victim_damage% crush
      eval victim_damage %damdone%
      msend %actor% The vortex spits you out and flings you into headlong into %victim.name%! (&1&b%actor_damage%&0) (&4%victim_damage%&0)
      msend %victim% The vortex flings %actor.name% into you! (&1&b%victim_damage%&0) (&4%actor_damage%&0)
      mteleport %victim% 1100
      mechoaround %actor% The vortex flings %actor.name% out, and right into %victim.name%! (&4%actor_damage%&0) (&4%victim_damage%&0)
      mteleport %actor% 1100
      * Teleport back and forth to break combat
      mteleport %actor% %self.room%
      mteleport %victim% %self.room%
   endif
else
   wait 2s
   if %actor% && (%actor.room% == %self.room%) && (%actor.vnum% == -1)
      eval damage 120 + %random.100%
      if %actor.aff_flagged[SANCT]%
         eval damage %damage% / 2
      endif
      mdamage %actor% %damage% crush
      msend %actor% &6%self.name% throws a tremendous burst of wind at you, throwing you from the area!&0 (&1&b%damdone%&0)
      mechoaround %actor% &6%self.name% throws a tremendous burst of wind at %actor.name%, throwing %actor.o% from the area!&0 (&4%damdone%&0)
      if %actor% &(%actor.room% == %self.room%)
         eval location 48801 + %random.29%
         mteleport %actor% %location%
         mforce %actor% look
      endif
   endif
endif
~
#48805
blue dragon fight~
0 k 100
~
wait 2s
set mode %random.10%
if %mode% <= 1
   breath lightning
elseif %mode% <= 2
   sweep
elseif %mode% <= 4
   roar
elseif (%mode% <= 5) && %actor% && (%actor.room% == %self.room%) && (%actor.vnum% == -1)
   eval damage 290 + %random.50%
   if %actor.aff_flagged[SANCT]%
      eval damage %damage% / 2
   endif
   if %actor.aff_flagged[STONE]%
      eval damage %damage% / 2
   endif
   mdamage %actor% %damage% shock
   mecho &4&bLightning crackles across the scales of %self.name%!&0
   if %damdone% == 0
      msend %actor% &4A blast of lightning flows off one of the dragon's claws and harmlessly into you!&0
      mechoaround %actor% &4A blast of lightning flows off one of the dragon's claws and harmlessly into %actor.name%!&0
   else
      msend %actor% &4A blast of lightning flows off one of the dragon's claws and into your body!&0 (&1&b%damdone%&0)
      mechoaround %actor% &4A blast of lightning flows off one of the dragon's claws and into %actor.name%'s body!&0 (&4%damdone%&0)
   end
end
~
#48806
stormchild fight~
0 k 50
~
wait 1s
set mode %random.10%
if %mode% < 5
   mecho &6A sudden gale starts from nowhere, throwing lightning everywhere!&0
   wait 1s
   m_run_room_trig 48851
elseif %mode% < 8
   wait 1s
   m_run_room_trig 48852
else
   emote cries, 'Stop it, %actor.name%!'
   if %actor.vnum% == -1
      eval damage 390 + %random.40%
   else
      * If a mob is tanking, hit it for massive damage!
      eval damage 1000 + %random.200%
   endif
   * Halve damage for sanc
   if %actor.aff_flagged[SANCT]%
      eval damage %damage% / 2
   endif
   mechoaround %actor% Lightning crackles around the Stormchild as she points a finger at %actor.name%.
   msend %actor% Lightning crackles around the Stormchild as she points a finger at you!
   wait 2s
   if %actor% && (%actor.room% == 48851)
      msend %actor% The lightning overloads, flowing into a shocking blast flowing straight for you!
      mechoaround %actor% The lightning overloads, flowing into a shocking blast flowing straight for %actor.name%!
      mdamage %actor% %damage% shock
      if %damdone% == 0
         msend %actor% &4The blast plays harmlessly over your body.&0
         mechoaround %actor% &4The blast plays harmlessly over %actor.name%'s body.&0
      else
         mechoaround %actor% &4The blast strikes %actor.name% in the chest, throwing %actor.o% into the wall!&0 (&4%damdone%&0)
         msend %actor% &4The blast strikes you square in the chest, throwing you into the wall!&0 (&1&b%damdone%&0)
      end
   else
      mecho The lightning fizzles out around the Stormchild.
   endif
endif
~
#48807
cloud dragon fight~
0 k 100
~
set mode %random.10%
if %mode% <= 3
   wait 2s
   breath frost
elseif %mode% <= 6
   wait 2s
   sweep
elseif %mode% <= 8
   wait 2s
   roar
elseif %actor% && (%actor.room% == %self.room%) && (%actor.vnum% == -1)
   wait 1s
   mecho %self.name% suddenly breaks apart into &6millions of droplets of moisture&0!
   wait 1s
   if %actor% && (%actor.room% == %self.room%)
      eval damage 350 + %random.50%
      if %actor.aff_flagged[SANCT]%
         eval damage %damage% / 2
      endif
      mdamage %actor% %damage% cold
      if %damdone% == 0
         msend %actor% &6The cloud of droplets envelopes you, cooling you.&0
         mechoaround %actor% &6%actor.name% enjoys a nice misting from the cloud.&0
      else
         msend %actor% &6The cloud of droplets envelopes you, draining your energy!&0 (&1&b%damdone%&0)
         mechoaround %actor% &6%actor.name% gasps for breath as the cloud of droplets envelopes %actor.o%!&0 (&4%damdone%&0)
      end
      wait 1
   endif
   mecho &6The cloud of droplets coalesces back into the form of %self.name%.&0
endif
~
#48809
stormlord fight~
0 k 100
~
set mode %random.10%
if %mode% < 5
   wait 1s
   mecho &6Hurricane-force winds suddenly howl, throwing around lightning bolts!&0
   wait 1s
   mecho &4A massive lightning bolt strikes %self.name% in the chest, causing %self.p% eyes to glow!&0
   mheal stormlord 1000
elseif %mode% < 8
   wait 2s
   emote lets out a thundering howl, causing the air around to vibrate.
   set max_hits 5
   while %max_hits% > 0
      set victim %random.char%
      if (%victim.vnum% == -1) &&! (%victim_list% /= %victim.name%)
         eval damage 150 + %random.50%
         if %victim.aff_flagged[SANCT]%
            eval damage %damage% / 2
         endif
         if %victim.aff_flagged[STONE]%
            * More damage for stoneskin
            eval damage %damage% + 80
            mdamage %victim% %damage%
            mechoaround %victim% &3%victim.name%'s stone-like skin grows massive cracks as the thunder rolls into
            %victim.o%.&0 (&4%damdone%&0)
            msend %victim% &3The thundering howl shatters your stone-like skin, causing immense pain!&0 (&1&b%damdone%&0)
         else
            mdamage %victim% %damage% crush
            if %damdone% == 0
               mechoaround %victim% &3%victim.name% holds %victim.p% ground.&0
               msend %victim% &3You hold your ground.&0
            else
               mechoaround %victim% &3%victim.name% holds %victim.p% head and cries out in pain!&0 (&4%damage%&0)
               msend %victim% &3Pain breaks out in your head as the thunder pounds your ears!&0 (&1&b%damage%&0)
            end
         endif
         set victim_list %victim_list% %victim.name%
      endif
      eval max_hits %max_hits% - 1
   done
else
   wait 1s
   mecho &4Lightning begins to crackle around %self.name%'s right arm.&0
   wait 1s
   if %actor% && (%actor.room% == %self.room%) && (%actor.vnum% == -1)
      mecho &4The glowing lightning flows into %self.name%'s index finger.&0
      mechoaround %actor% %self.name% points %self.p% finger at %actor.name%.
      msend %actor% %self.name% points %self.p% finger at you.
      eval damage 350 + %random.100%
      if %actor.aff_flagged[SANCT]%
         eval damage %damage% / 2
      endif
      mdamage %actor% %damage% shock
      if %damdone% == 0
         msend %actor% A blast of lighting strikes you, lighting you up!
         mechoaround %actor% A blast of lighting strikes %actor.name%, lighting %actor.o% up!
      else
         mechoaround %actor% A tremendous blast of lightning strikes %actor.name%, burning %actor.p% skin! (&4%damdone%&0)
         msend %actor% A tremendous blast of lightning strikes you, tearing at your flesh! (&1&b%damdone%&0)
      end
   endif
endif
~
#48810
mdamage~
1 c 3
mdamage~
if (%actor.vnum% > 0) && !(%actor.aff_flagged[CHARM]%)
   odamage %arg%
else
   return 0
endif
~
#48811
mheal~
1 c 3
mheal~
if (%actor.vnum% > 0) && !(%actor.aff_flagged[CHARM]%)
   oheal %arg%
else
   return 0
endif
~
#48812
baton_cone_of_cold~
1 d 5
~
ocast 'cone of cold' %victim%
~
#48814
birdmaster fight~
0 k 20
~
if %get.mob_count[48815]% < 6
   wait 1s
   emote holds a bird whistle to his lips and lets out a loud whistle!
   wait 1s
   mload mob 48815
   mforce cavern-hawk emote swoops in to the aid of %self.name%!
   mforce cavern-hawk hit %actor.name%
endif
~
#48851
stormchild heal~
2 a 100
~
wecho &4The Stormchild's eyes open wide as a massive lightning bolt strikes her in the chest!&0
wheal stormchild 1000
~
#48852
**UNUSED**~
2 a 100
~
wecho The Stormchild opens her mouth in a scream, and a wave of thunder rolls out!
set person %self.people%
while %person%
   set next %person.next_in_room%
   if (%person.vnum% < 48800) || (%person.vnum% > 48899)
      eval damage 150 + %random.50%
      * Halve damage for sanc
      if %person.aff_flagged[SANCT]%
         eval damage %damage% / 2
      endif
      if %person.aff_flagged[STONE]%
         * Double damage for stoneskin
         eval damage %damage% + %damage%
      end
      wdamage %person% %damage% crush
      if %damdone% == 0
         wsend %person% The blast passes through you harmlessly.
         wechoaround %person% The blast passes harmlessly through %person.name%.
      elseif %person.aff_flagged[STONE]%
         wechoaround %person% &3%person.name% writhes in agony as the thunder shatters %person.p% stony skin!&0 (&4%damdone%&0)
         wsend %person% &3You writhe in agony as the thunder shatters your stony skin!&0 (&1&b%damdone%&0)
      else
         wechoaround %person% &3%person.name% cries out in pain as the thunderclap pounds %person.p% eardrums!&0 (&4%damdone%&0)
         wsend %person% &3You cry out in pain as the thunderclap pounds your eardrums!&0 (&1&b%damdone%&0)
      endif
   endif
   set person %next%
done
~
#48853
**UNUSED**~
2 a 100
~
* This trigger is now handled in 48806
if !%tank%
   halt
endif
set person %self.people%
while %person%
   set next %person.next_in_room%
   if %tank% /= %person.name%
      if %person.vnum% == -1
         eval damage 390 + %random.40%
      else
         * If a mob is tanking, hit it for massive damage!
         eval damage 1000 + %random.200%
      endif
      * Halve damage for sanc
      if %person.aff_flagged[SANCT]%
         eval damage %damage% / 2
      endif
      wechoaround %person% Lightning crackles around the Stormchild as she points a finger at %person.name%.
      wsend %person% Lightning crackles around the Stormchild as she points a finger at you!
      wait 2s
      if %person% && (%person.room% == 48851)
         wdamage %person% %damage% shock
         wsend %person% The lightning overloads, flowing into a shocking blast flowing straight for you!
         wechoaround %person% The lightning overloads, flowing into a shocking blast flowing straight for %person.name%!
         if %damdone% == 0
            wechoaround %person% &4The blast passes right through %person.name%'s chest!&0
            wsend %person% &4The blast goes right through you, striking the wall!&0
         else
            wechoaround %person% &4The blast strikes %person.name% in the chest, throwing %person.o% into the wall!&0 (&4%damdone%&0)
            wsend %person% &4The blast strikes you square in the chest, throwing you into the wall!&0 (&1&b%damdone%&0)
         end
      else
         wecho The lightning fizzles out around the Stormchild.
      endif
      unset tank
      halt
   endif
   set person %next%
done
~
#48854
stormchild tank~
2 d 0
Stop it,~
if %actor.vnum% == 48806
   set tank %speech%
   global tank
endif
~
$~
